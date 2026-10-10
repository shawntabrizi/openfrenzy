/*
XISO_FS.H

Read-only access to the files of an Xbox disc image (XDVDFS), for the
D:\ drive of the game. Format (as extract-xiso reads it; see xiso_fs.c):
2048-byte sectors, a volume descriptor at 0x10000 (plus the game partition
offset of a whole-disc image), and directories stored as binary trees of
entries.

openfrenzy (vfs module). Not from OpenCE: OpenCE's xiso.c only extracted
Halo's maps folder; the game here reads every file from the disc in place.

Reads use pread() on one descriptor, so any number of threads may read the
same image at once.
*/

#ifndef OPENFRENZY_XISO_FS_H
#define OPENFRENZY_XISO_FS_H

#define XISO_FS_NAME_SIZE 256

/* XDVDFS attribute bits (the FAT/Win32 ones) */
#define XISO_ATTRIBUTE_READ_ONLY 0x01
#define XISO_ATTRIBUTE_DIRECTORY 0x10

struct xiso_fs
{
	int descriptor;
	unsigned long long partition;   /* byte offset of the game partition */
	unsigned long long image_size;  /* bytes in the image file */
	unsigned int root_sector;
	unsigned int root_size;
};

struct xiso_entry
{
	unsigned int sector;
	unsigned int size;
	unsigned int is_directory;
	unsigned int attributes;        /* XISO_ATTRIBUTE_* as stored */
	char name[XISO_FS_NAME_SIZE];   /* the on-disc spelling; "" for the root */
};

/* opens the image; 0 on success, -1 if it cannot be read or is not XDVDFS */
int xiso_fs_open(struct xiso_fs *fs, const char *image_path);
void xiso_fs_close(struct xiso_fs *fs);

/* the root directory */
void xiso_fs_root(const struct xiso_fs *fs, struct xiso_entry *entry);

/* finds a path relative to the disc root; '\\' and '/' both separate names,
"." is skipped and ".." goes up, and names compare without case, as on the
Xbox. "" and "\\" give the root. 0 on success, -1 if absent */
int xiso_fs_find(struct xiso_fs *fs, const char *path, struct xiso_entry *entry);

/* reads up to size bytes at offset of a file; returns the count read (0 at
or after the end) or -1 on an I/O error */
long xiso_fs_read(struct xiso_fs *fs, const struct xiso_entry *entry,
	unsigned long long offset, void *buffer, unsigned long size);

/* calls visit for each entry of a directory, in the disc's (sorted) order,
until visit returns nonzero; returns the count visited, or -1 on an error */
typedef int (*xiso_fs_visit_proc)(void *context, const struct xiso_entry *entry);
int xiso_fs_list(struct xiso_fs *fs, const struct xiso_entry *directory,
	xiso_fs_visit_proc visit, void *context);

#endif
