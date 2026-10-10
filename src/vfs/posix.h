/*
POSIX.H

Boundary between the XDK-facing platform layer and glibc.

Everything except posix_*.c is compiled with the game's MSVC-compatible ABI
(-malign-double, 16-bit wchar_t, XDK headers), under which glibc structures
with 64-bit members, such as struct stat and struct dirent, have the wrong
layout. The posix_*.c files are compiled with the host ABI instead and expose
these helpers, whose parameters are all 32-bit scalars or pointers to structs
made only of 32-bit members so both sides agree on the layout.
*/

#ifndef __HALO_LINUX_POSIX_H
#define __HALO_LINUX_POSIX_H

/* 32-bit on both sides of the boundary. The Android port compiles these
files into its 64-bit host, where long is 64-bit, and calls them from its
ILP32 guest (port/android/README.md). */
#ifdef __LP64__
typedef int posix_long;
typedef unsigned int posix_ulong;
#else
typedef long posix_long;
typedef unsigned long posix_ulong;
#endif

enum
{
	_posix_file_is_directory = 1 << 0,
	_posix_file_is_read_only = 1 << 1,
};

struct posix_file_information
{
	posix_ulong flags;
	posix_ulong size_low;
	posix_ulong size_high;
	/* seconds and nanoseconds since the Unix epoch */
	posix_ulong modification_seconds;
	posix_ulong modification_nanoseconds;
	posix_ulong access_seconds;
	posix_ulong access_nanoseconds;
	posix_ulong creation_seconds;
	posix_ulong creation_nanoseconds;
};

/* stat()/fstat(); return 0 on success or -1 with errno set */
int posix_stat(const char *path, struct posix_file_information *information);
int posix_fstat(int descriptor, struct posix_file_information *information);

/* set access and modification times; a zero seconds value leaves it alone */
int posix_set_file_times(const char *path,
	posix_ulong access_seconds, posix_ulong access_nanoseconds,
	posix_ulong modification_seconds, posix_ulong modification_nanoseconds);

/* 64-bit file positioning on a descriptor */
int posix_seek(int descriptor, posix_long offset_low, posix_long offset_high, int whence,
	posix_ulong *position_low, posix_ulong *position_high);
int posix_truncate(int descriptor, posix_ulong size_low, posix_ulong size_high);

/* free and total bytes on the file system holding path */
int posix_disk_space(const char *path,
	posix_ulong *free_low, posix_ulong *free_high,
	posix_ulong *total_low, posix_ulong *total_high);

/* permissions and directories */
int posix_set_read_only(const char *path, int read_only);
int posix_make_directory(const char *path);

/* directory enumeration; the handle is opaque */
void *posix_directory_open(const char *path);
/* copies the next entry name (excluding . and ..); returns 0 at the end */
int posix_directory_next(void *directory, char *name, posix_ulong name_size);
void posix_directory_close(void *directory);

/* case-insensitive lookup of one path component inside directory;
copies the on-disk spelling into result and returns nonzero if found */
int posix_find_entry_case_insensitive(const char *directory, const char *name,
	char *result, posix_ulong result_size);

/* openfrenzy: the OpenCE socket, UPnP, URL-scheme, secret and Discord helpers
(posix_net.c) are left out. The game has no networking in round 1. */

#endif
