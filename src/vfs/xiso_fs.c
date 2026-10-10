/*
XISO_FS.C

Read-only XDVDFS reader (see xiso_fs.h). openfrenzy vfs module.
Format handling follows xiso.c (OpenCE, from extract-xiso; license in xiso.c).
*/

#include "xiso_fs.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define XISO_SECTOR 2048ull
#define XISO_HEADER 0x10000ull
#define XISO_MAGIC "MICROSOFT*XBOX*MEDIA"
#define XISO_MAGIC_LENGTH 20
#define XISO_ATTRIBUTE_DIRECTORY 0x10

/* where a whole-disc image keeps the game partition (as extract-xiso) */
static const unsigned long long partition_offsets[] =
	{ 0ull, 0x18300000ull, 0xFD90000ull, 0x89D80000ull, 0x2080000ull };

static unsigned int read_u32(const unsigned char *bytes)
{
	return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | ((unsigned int)bytes[3] << 24);
}

static unsigned int read_u16(const unsigned char *bytes)
{
	return bytes[0] | (bytes[1] << 8);
}

static int read_at(struct xiso_fs *fs, unsigned long long offset, void *buffer, unsigned long size)
{
	if (fseeko(fs->file, (off_t)offset, SEEK_SET))
		return -1;
	return fread(buffer, 1, size, fs->file) == size ? 0 : -1;
}

int xiso_fs_open(struct xiso_fs *fs, const char *image_path)
{
	unsigned int index;

	memset(fs, 0, sizeof(*fs));
	fs->file = fopen(image_path, "rb");
	if (!fs->file)
		return -1;
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
	if (fs->file)
		fclose(fs->file);
	fs->file = NULL;
}

/* searches one directory's entry tree for name (without case); the tree is
ordered, but the order is the Xbox's case-insensitive one, so a full walk is
used: directories hold few entries */
static int find_in_directory(const unsigned char *data, unsigned int size, unsigned int offset,
	const char *name, unsigned int name_length, struct xiso_entry *entry, int depth)
{
	unsigned int left, right, length;

	if (depth > 64 || offset + 14 > size)
		return -1;
	left = read_u16(data + offset);
	right = read_u16(data + offset + 2);
	if (left == 0xFFFF)
		return -1;
	length = data[offset + 13];
	if (offset + 14 + length <= size && length == name_length &&
		!strncasecmp((const char *)data + offset + 14, name, name_length))
	{
		entry->sector = read_u32(data + offset + 4);
		entry->size = read_u32(data + offset + 8);
		entry->is_directory = (data[offset + 12] & XISO_ATTRIBUTE_DIRECTORY) != 0;
		return 0;
	}
	if (left && !find_in_directory(data, size, left * 4, name, name_length, entry, depth + 1))
		return 0;
	if (right && !find_in_directory(data, size, right * 4, name, name_length, entry, depth + 1))
		return 0;
	return -1;
}

int xiso_fs_find(struct xiso_fs *fs, const char *path, struct xiso_entry *entry)
{
	struct xiso_entry current = { fs->root_sector, fs->root_size, 1 };

	while (*path == '\\' || *path == '/')
		path++;
	while (*path)
	{
		const char *end = path;
		unsigned char *data;
		int found;

		while (*end && *end != '\\' && *end != '/')
			end++;
		if (!current.is_directory || !current.size)
			return -1;
		data = malloc(current.size);
		if (!data)
			return -1;
		if (read_at(fs, fs->partition + current.sector * XISO_SECTOR, data, current.size))
		{
			free(data);
			return -1;
		}
		found = find_in_directory(data, current.size, 0, path, (unsigned int)(end - path), &current, 0);
		free(data);
		if (found)
			return -1;
		path = end;
		while (*path == '\\' || *path == '/')
			path++;
	}
	*entry = current;
	return 0;
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
