/*
XISO_FS.C

Read-only XDVDFS reader (see xiso_fs.h). openfrenzy vfs module.

The format handling follows extract-xiso
(https://github.com/XboxDev/extract-xiso, extract-xiso.c), as OpenCE's
xiso.c did before it: 2048-byte sectors; a volume descriptor at 0x10000
that starts with "MICROSOFT*XBOX*MEDIA" and gives the root directory's
sector and size; and directories whose entries form a binary tree (each
entry: left and right subtree offsets in 4-byte units, the start sector, the
size, attributes, the name's length and the name, 4-byte aligned; a left
offset of 0xFFFF marks an empty directory or sector padding). Images made
from a whole disc put the game partition further in, at one of the offsets
below. extract-xiso's license:

 * Copyright (c) 2003 in <in@fishtank.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *    This product includes software developed by in <in@fishtank.com>.
 *
 * 4. Neither the name of "in" nor the email address "in@fishtank.com"
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED `AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * AUTHOR OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This product includes software developed by in <in@fishtank.com>.
*/

#include "xiso_fs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#define XISO_SECTOR 2048ull
#define XISO_HEADER 0x10000ull
#define XISO_MAGIC "MICROSOFT*XBOX*MEDIA"
#define XISO_MAGIC_LENGTH 20
#define XISO_ENTRY_HEADER 14
/* directory tables are a few sectors; anything much larger is not one */
#define XISO_MAXIMUM_DIRECTORY (4u << 20)
#define XISO_MAXIMUM_DEPTH 256

/* where the game partition starts: a plain image, and whole-disc images
(extract-xiso's XGD1, XGD2 and XGD3 offsets) */
static const unsigned long long partition_offsets[] = { 0ull, 0x18300000ull, 0x0FD90000ull, 0x02080000ull };

static unsigned int read_u32(const unsigned char *bytes)
{
	return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | ((unsigned int)bytes[3] << 24);
}

static unsigned int read_u16(const unsigned char *bytes)
{
	return bytes[0] | (bytes[1] << 8);
}

/* reads exactly size bytes at an offset of the image; 0 or -1 */
static int read_at(const struct xiso_fs *fs, unsigned long long offset, void *buffer, unsigned long size)
{
	unsigned long done = 0;

	while (done < size)
	{
		ssize_t result = pread(fs->descriptor, (char *)buffer + done, size - done, (off_t)(offset + done));

		if (result < 0 && errno == EINTR)
			continue;
		if (result <= 0)
			return -1;
		done += (unsigned long)result;
	}
	return 0;
}

int xiso_fs_open(struct xiso_fs *fs, const char *image_path)
{
	unsigned int index;
	off_t end;

	memset(fs, 0, sizeof(*fs));
	fs->descriptor = open(image_path, O_RDONLY | O_CLOEXEC);
	if (fs->descriptor < 0)
		return -1;
	end = lseek(fs->descriptor, 0, SEEK_END);
	fs->image_size = end > 0 ? (unsigned long long)end : 0;
	for (index = 0; index < sizeof(partition_offsets) / sizeof(partition_offsets[0]); index++)
	{
		unsigned char header[XISO_MAGIC_LENGTH + 8];

		if (read_at(fs, partition_offsets[index] + XISO_HEADER, header, sizeof(header)))
			continue;
		if (!memcmp(header, XISO_MAGIC, XISO_MAGIC_LENGTH))
		{
			fs->partition = partition_offsets[index];
			fs->root_sector = read_u32(header + XISO_MAGIC_LENGTH);
			fs->root_size = read_u32(header + XISO_MAGIC_LENGTH + 4);
			return 0;
		}
	}
	xiso_fs_close(fs);
	return -1;
}

void xiso_fs_close(struct xiso_fs *fs)
{
	if (fs->descriptor >= 0)
		close(fs->descriptor);
	fs->descriptor = -1;
}

void xiso_fs_root(const struct xiso_fs *fs, struct xiso_entry *entry)
{
	memset(entry, 0, sizeof(*entry));
	entry->sector = fs->root_sector;
	entry->size = fs->root_size;
	entry->is_directory = 1;
	entry->attributes = XISO_ATTRIBUTE_DIRECTORY;
}

/* the directory's table, malloc'd; NULL for an empty or unreadable one */
static unsigned char *load_directory(const struct xiso_fs *fs, const struct xiso_entry *directory)
{
	unsigned char *data;

	if (!directory->is_directory || !directory->size || directory->size > XISO_MAXIMUM_DIRECTORY)
		return NULL;
	data = malloc(directory->size);
	if (data && read_at(fs, fs->partition + directory->sector * XISO_SECTOR, data, directory->size))
	{
		free(data);
		data = NULL;
	}
	return data;
}

/* decodes the entry at offset; 0, or -1 if there is none (padding, end) */
static int decode_entry(const unsigned char *data, unsigned int size, unsigned int offset, struct xiso_entry *entry,
	unsigned int *left, unsigned int *right)
{
	unsigned int length;

	if (offset + XISO_ENTRY_HEADER > size)
		return -1;
	*left = read_u16(data + offset);
	*right = read_u16(data + offset + 2);
	if (*left == 0xFFFF)
		return -1;
	length = data[offset + 13];
	if (offset + XISO_ENTRY_HEADER + length > size)
		return -1;
	entry->sector = read_u32(data + offset + 4);
	entry->size = read_u32(data + offset + 8);
	entry->attributes = data[offset + 12];
	entry->is_directory = (entry->attributes & XISO_ATTRIBUTE_DIRECTORY) != 0;
	memcpy(entry->name, data + offset + XISO_ENTRY_HEADER, length);
	entry->name[length] = '\0';
	return 0;
}

/* in-order walk of one directory's tree (gives the disc's sorted order);
returns nonzero when visit asked to stop */
static int walk(const unsigned char *data, unsigned int size, unsigned int offset, int depth,
	xiso_fs_visit_proc visit, void *context, int *count)
{
	struct xiso_entry entry;
	unsigned int left, right;

	if (depth > XISO_MAXIMUM_DEPTH || decode_entry(data, size, offset, &entry, &left, &right))
		return 0;
	if (left && walk(data, size, left * 4, depth + 1, visit, context, count))
		return 1;
	(*count)++;
	if (visit(context, &entry))
		return 1;
	return right && walk(data, size, right * 4, depth + 1, visit, context, count);
}

int xiso_fs_list(struct xiso_fs *fs, const struct xiso_entry *directory,
	xiso_fs_visit_proc visit, void *context)
{
	unsigned char *data;
	int count = 0;

	if (!directory->is_directory)
		return -1;
	if (!directory->size)
		return 0;
	data = load_directory(fs, directory);
	if (!data)
		return -1;
	walk(data, directory->size, 0, 0, visit, context, &count);
	free(data);
	return count;
}

/* finds name (without case) in one directory's table. The tree is ordered,
but by the Xbox's own comparison, so a full walk is used: directories hold
few entries */
static int find_in_directory(const unsigned char *data, unsigned int size, unsigned int offset,
	const char *name, unsigned int name_length, struct xiso_entry *entry, int depth)
{
	unsigned int left, right;

	if (depth > XISO_MAXIMUM_DEPTH || decode_entry(data, size, offset, entry, &left, &right))
		return -1;
	if (strlen(entry->name) == name_length && !strncasecmp(entry->name, name, name_length))
		return 0;
	if (left && !find_in_directory(data, size, left * 4, name, name_length, entry, depth + 1))
		return 0;
	if (right && !find_in_directory(data, size, right * 4, name, name_length, entry, depth + 1))
		return 0;
	return -1;
}

int xiso_fs_find(struct xiso_fs *fs, const char *path, struct xiso_entry *entry)
{
	/* the directories from the root down to the current one, for ".." */
	struct xiso_entry *stack;
	int depth = 0;
	int result = -1;

	stack = malloc(sizeof(*stack) * (XISO_MAXIMUM_DEPTH + 1));
	if (!stack)
		return -1;
	xiso_fs_root(fs, &stack[0]);
	for (;;)
	{
		const char *end;
		unsigned int length;
		unsigned char *data;
		int found;

		while (*path == '\\' || *path == '/')
			path++;
		if (!*path)
		{
			*entry = stack[depth];
			result = 0;
			break;
		}
		for (end = path; *end && *end != '\\' && *end != '/'; end++)
			;
		length = (unsigned int)(end - path);
		if (length == 1 && path[0] == '.')
		{
			path = end;
			continue;
		}
		if (length == 2 && path[0] == '.' && path[1] == '.')
		{
			if (depth)
				depth--;
			path = end;
			continue;
		}
		if (length >= XISO_FS_NAME_SIZE || depth >= XISO_MAXIMUM_DEPTH || !stack[depth].is_directory)
			break;
		data = load_directory(fs, &stack[depth]);
		if (!data)
			break;
		found = find_in_directory(data, stack[depth].size, 0, path, length, &stack[depth + 1], 0);
		free(data);
		if (found)
			break;
		depth++;
		path = end;
	}
	free(stack);
	return result;
}

long xiso_fs_read(struct xiso_fs *fs, const struct xiso_entry *entry,
	unsigned long long offset, void *buffer, unsigned long size)
{
	if (entry->is_directory || offset >= entry->size)
		return 0;
	if (size > entry->size - offset)
		size = (unsigned long)(entry->size - offset);
	if (read_at(fs, fs->partition + entry->sector * XISO_SECTOR + offset, buffer, size))
		return -1;
	return (long)size;
}
