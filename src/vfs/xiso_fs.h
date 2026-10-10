/*
XISO_FS.H

Read-only access to the files of an Xbox disc image (XDVDFS), for the
D:\ drive of the game. The format is the one xiso.c describes (from
extract-xiso): 2048-byte sectors, a volume descriptor at 0x10000 (plus the
game partition offset of a whole-disc image), and directories stored as
binary trees of entries.

openfrenzy (vfs module). Not from OpenCE: OpenCE's xiso.c only extracts
Halo's maps folder; the game here reads every file from the disc.
*/

#ifndef OPENFRENZY_XISO_FS_H
#define OPENFRENZY_XISO_FS_H

#include <stdio.h>

struct xiso_fs
{
	FILE *file;
	unsigned long long partition;   /* byte offset of the game partition */
	unsigned int root_sector;
	unsigned int root_size;
};

struct xiso_entry
{
	unsigned int sector;
	unsigned int size;
	unsigned int is_directory;
};

/* opens the image; 0 on success, -1 if it cannot be read or is not XDVDFS */
int xiso_fs_open(struct xiso_fs *fs, const char *image_path);
void xiso_fs_close(struct xiso_fs *fs);

/* finds a path relative to the disc root; '\\' and '/' both separate names,
and names compare without case, as on the Xbox. 0 on success, -1 if absent */
int xiso_fs_find(struct xiso_fs *fs, const char *path, struct xiso_entry *entry);

/* reads up to size bytes at offset of a file; returns the count read or -1 */
long xiso_fs_read(struct xiso_fs *fs, const struct xiso_entry *entry,
	unsigned long long offset, void *buffer, unsigned long size);

#endif
