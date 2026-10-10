/*
VFS_FILES.H

openfrenzy vfs module: the Xbox drives of the game.

  D:\        the player's disc image (read only): OPENFRENZY_ISO, else
             <data root>/game.xiso.iso. A path without a drive is on D:\, as
             on the Xbox (the title's default directory).
  T:\ U:\    title data and user data (saves): <save root>/t, <save root>/u
  Z:\        the utility drive (bKernel's xbCache.cpp file cache), mounted by
             XMountUtilityDrive: <save root>/z
  any other  <save root>/<letter>

The data root is OPENFRENZY_DATA_ROOT (the host sets it), else the setting
paths.data, else ".". The save root is OPENFRENZY_SAVE_ROOT (the host sets
it), else paths.saves, else ~/Library/Application Support/OpenFrenzy.
Names compare without case on every drive.

The Win32 file API on these drives (CreateFileA, ReadFile, SetFilePointer,
GetFileSize, FindFirstFileA, ...) is declared by the XDK headers (xtl.h) and
defined in xbox_files.c. This header declares what the XDK headers lack.
Other modules may include it.
*/

#ifndef OPENFRENZY_VFS_FILES_H
#define OPENFRENZY_VFS_FILES_H

#include <stdio.h>

struct xiso_fs;

/* the disc image, opened on first use; NULL (logged once) if it is missing
or not XDVDFS */
struct xiso_fs *vfs_disc(void);

/* nonzero if an Xbox path names the disc (d:\..., or no drive) */
int vfs_is_disc_path(const char *xbox_path);

/* fopen() by Xbox path. A disc file opens read only (a write mode fails
with EROFS) as a stdio stream over the image; other drives open the host
file platform_translate_path() gives. The runtime module's fopen()
(halo_linux_fopen in msvc_crt.c) is meant to call this. */
FILE *vfs_fopen(const char *xbox_path, const char *mode);

/* XAPI (XDK 3911), not declared by the XDK headers here. The game calls
XMountUtilityDrive (0x00030e25) from core_init. format_clean empties the
drive first. TRUE on success. */
int __stdcall XMountUtilityDrive(int format_clean);
int __stdcall XFormatUtilityDrive(void);

/* Win32: ends a FindFirstFileA search */
int __stdcall FindClose(void *find_file);

/* Win32 (0x001e91ca in the XBE); BY_HANDLE_FILE_INFORMATION, 0x34 bytes */
struct vfs_by_handle_file_information
{
	unsigned long dwFileAttributes;
	unsigned long ftCreationTime[2];
	unsigned long ftLastAccessTime[2];
	unsigned long ftLastWriteTime[2];
	unsigned long dwVolumeSerialNumber;
	unsigned long nFileSizeHigh;
	unsigned long nFileSizeLow;
	unsigned long nNumberOfLinks;
	unsigned long nFileIndexHigh;
	unsigned long nFileIndexLow;
};
int __stdcall GetFileInformationByHandle(void *file, struct vfs_by_handle_file_information *information);

#endif
