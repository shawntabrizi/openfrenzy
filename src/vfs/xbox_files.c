/*
XBOX_FILES.C

Win32 file API of the Xbox, for the game's drives (vfs_files.h):

  d:\ (and a path without a drive)  the disc image, read in place (xiso_fs.c)
  every other drive X:\             the directory <save root>/x (t:\ title
                                    data, u:\ saves, z:\ utility drive cache)

From OpenCE port/linux/src/xbox_files.c (commit 231708d, CC0). openfrenzy
changes: d:\ reads the disc image instead of a folder of extracted files;
the roots come from the host (OPENFRENZY_DATA_ROOT, OPENFRENZY_SAVE_ROOT);
Halo's maps-folder search and Custom Edition drive (h:\) are removed;
XMountUtilityDrive, XFormatUtilityDrive, GetFileInformationByHandle and
vfs_fopen are added.
*/

#include "platform.h"
#include "posix.h"
#include "port_config.h"
#include "vfs_files.h"
#include "xiso_fs.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef ERROR_NEGATIVE_SEEK
#define ERROR_NEGATIVE_SEEK 131L
#endif

/* ---------- roots */

static BOOL directory_exists(const char *path)
{
	struct posix_file_information information;

	return posix_stat(path, &information) == 0;
}

static void trim_separators(char *path)
{
	while (strlen(path) > 1 && path[strlen(path) - 1] == '/')
		path[strlen(path) - 1] = '\0';
}

const char *platform_data_root(void)
{
	static char root[MAX_PATH];

	if (!root[0])
	{
		const char *environment = getenv("OPENFRENZY_DATA_ROOT");

		if (!environment || !*environment)
			environment = config_string("paths.data");
		snprintf(root, sizeof(root), "%s", *environment ? environment : ".");
		trim_separators(root);
		platform_log("data root: %s", root);
	}
	return root;
}

/* creates every missing directory along path */
static void make_directories(const char *path)
{
	char partial[MAX_PATH];
	unsigned long index;

	snprintf(partial, sizeof(partial), "%s", path);
	for (index = 1; partial[index]; index++)
	{
		if (partial[index] == '/')
		{
			partial[index] = '\0';
			if (!directory_exists(partial))
				posix_make_directory(partial);
			partial[index] = '/';
		}
	}
	if (!directory_exists(partial))
		posix_make_directory(partial);
}

const char *platform_save_root(void)
{
	static char root[MAX_PATH];

	if (!root[0])
	{
		const char *environment = getenv("OPENFRENZY_SAVE_ROOT");
		const char *home = getenv("HOME");

		if (!environment || !*environment)
			environment = config_string("paths.saves");
		if (*environment)
			snprintf(root, sizeof(root), "%s", environment);
		else if (home && *home)
			snprintf(root, sizeof(root), "%s/Library/Application Support/OpenFrenzy", home);
		else
			snprintf(root, sizeof(root), "%s/saves", platform_data_root());
		trim_separators(root);
		make_directories(root);
		platform_log("save root: %s", root);
	}
	return root;
}

/* ---------- drives */

/* the drive of an Xbox path in lower case ('d' when it has none, as the
title's default directory is the disc); rest is set past "x:" */
static char path_drive(const char *path, const char **rest)
{
	if (((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':')
	{
		*rest = path + 2;
		return (char)(path[0] | 0x20);
	}
	*rest = path;
	return 'd';
}

int vfs_is_disc_path(const char *xbox_path)
{
	const char *rest;

	return path_drive(xbox_path, &rest) == 'd';
}

static struct xiso_fs disc;
/* the image file's times, read once: every disc entry reports them */
static struct posix_file_information disc_times;
static BOOL disc_tried, disc_opened;
/* (not pthread_once: the guest's C library leaves out pthread_cleanup_push,
which musl's pthread_once needs) */
static pthread_mutex_t disc_lock = PTHREAD_MUTEX_INITIALIZER;

static void disc_open(void)
{
	char path[MAX_PATH];
	const char *environment = getenv("OPENFRENZY_ISO");

	if (environment && *environment)
		snprintf(path, sizeof(path), "%s", environment);
	else
		snprintf(path, sizeof(path), "%s/game.xiso.iso", platform_data_root());
	if (xiso_fs_open(&disc, path) == 0)
	{
		disc_opened = TRUE;
		if (posix_fstat(disc.descriptor, &disc_times) != 0)
			memset(&disc_times, 0, sizeof(disc_times));
		platform_log("disc image: %s (partition at 0x%llx)", path, disc.partition);
	}
	else
	{
		platform_log("*** no disc: cannot open %s as an Xbox disc image (XDVDFS); set OPENFRENZY_ISO "
			"or put game.xiso.iso in the data root. Every d:\\ access will fail. ***", path);
	}
}

struct xiso_fs *vfs_disc(void)
{
	pthread_mutex_lock(&disc_lock);
	if (!disc_tried)
	{
		disc_tried = TRUE;
		disc_open();
	}
	pthread_mutex_unlock(&disc_lock);
	return disc_opened ? &disc : NULL;
}

/* the last '\\' or '/' in path, or NULL */
static char *last_separator(const char *path)
{
	const char *backslash = strrchr(path, '\\');
	const char *slash = strrchr(path, '/');

	return (char *)(backslash > slash ? backslash : slash);
}

/* finds a disc path (after its drive); TRUE, or FALSE with the Win32 error
set (a missing file in an existing directory is ERROR_FILE_NOT_FOUND, else
ERROR_PATH_NOT_FOUND) */
static BOOL disc_find(const char *path, struct xiso_entry *entry)
{
	struct xiso_fs *fs = vfs_disc();
	char parent[1024];
	char *separator;
	struct xiso_entry directory;

	if (fs && xiso_fs_find(fs, path, entry) == 0)
		return TRUE;
	snprintf(parent, sizeof(parent), "%s", path);
	while (parent[0] && strchr("\\/", parent[strlen(parent) - 1]))
		parent[strlen(parent) - 1] = '\0';
	separator = last_separator(parent);
	if (separator)
		*separator = '\0';
	else
		parent[0] = '\0';
	SetLastError(fs && xiso_fs_find(fs, parent, &directory) == 0 && directory.is_directory ?
		ERROR_FILE_NOT_FOUND : ERROR_PATH_NOT_FOUND);
	return FALSE;
}

/* a disc entry as file information: the image's times, read only */
static void disc_information(const struct xiso_entry *entry, struct posix_file_information *information)
{
	*information = disc_times;
	information->flags = _posix_file_is_read_only | (entry->is_directory ? _posix_file_is_directory : 0);
	information->size_low = entry->is_directory ? 0 : entry->size;
	information->size_high = 0;
}

/* host path of a path on a drive other than d:\ (or below the data root
for d:\, which only the C library's fopen() family still uses: see
vfs_fopen) */
void platform_translate_path(const char *xbox_path, char *host_path, unsigned long host_path_size)
{
	char resolved[1024];
	const char *cursor;
	char drive = path_drive(xbox_path, &cursor);
	unsigned long length;

	if (drive == 'd')
	{
		snprintf(resolved, sizeof(resolved), "%s", platform_data_root());
	}
	else
	{
		snprintf(resolved, sizeof(resolved), "%s/%c", platform_save_root(), drive);
		/* every Xbox drive always exists; create its directory on first use */
		if (!directory_exists(resolved))
			posix_make_directory(resolved);
	}

	while (*cursor)
	{
		char component[256];
		char on_disk[256];
		unsigned long component_length = 0;

		while (*cursor == '\\' || *cursor == '/')
			cursor++;
		while (*cursor && *cursor != '\\' && *cursor != '/' && component_length + 1 < sizeof(component))
			component[component_length++] = *cursor++;
		component[component_length] = '\0';
		/* . and .. (and Windows' spellings of them, with trailing dots and
		spaces) are skipped: FATX names are never only dots, and .. would
		leave the root */
		if (!component_length || strspn(component, ". ") == component_length)
			continue;

		if (posix_find_entry_case_insensitive(resolved, component, on_disk, sizeof(on_disk)))
		{
			/* prefer an exact match when several spellings exist */
			char exact[1100];
			struct posix_file_information information;

			snprintf(exact, sizeof(exact), "%s/%s", resolved, component);
			if (posix_stat(exact, &information) == 0)
				snprintf(on_disk, sizeof(on_disk), "%s", component);
		}
		else
		{
			snprintf(on_disk, sizeof(on_disk), "%s", component);
		}
		length = strlen(resolved);
		snprintf(resolved + length, sizeof(resolved) - length, "/%s", on_disk);
	}
	snprintf(host_path, host_path_size, "%s", resolved);
}

/* ---------- file handles */

struct platform_file
{
	/* a host file, or -1 for a disc file */
	int descriptor;
	/* disc files */
	struct xiso_entry entry;
	unsigned long long position;
	char path[1024];
};

static void file_destroy(struct platform_handle *handle)
{
	struct platform_file *file = handle->data;

	if (file->descriptor >= 0)
		close(file->descriptor);
	free(file);
}

static struct platform_file *file_from_handle(HANDLE handle)
{
	struct platform_handle *record = platform_handle_get(handle, _platform_handle_file);

	return record ? record->data : NULL;
}

static BOOL is_disc_file(const struct platform_file *file)
{
	return file->descriptor < 0;
}

static HANDLE file_handle_new(int descriptor, const struct xiso_entry *entry, const char *path)
{
	struct platform_file *file = calloc(1, sizeof(*file));
	struct platform_handle *handle = file ? platform_handle_new(_platform_handle_file, file, file_destroy) : NULL;

	if (!handle)
	{
		if (descriptor >= 0)
			close(descriptor);
		free(file);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return INVALID_HANDLE_VALUE;
	}
	file->descriptor = descriptor;
	if (entry)
		file->entry = *entry;
	snprintf(file->path, sizeof(file->path), "%s", path);
	return handle;
}

/* the disc is read only: opening for writing, or creating, is denied */
static HANDLE create_disc_file(const char *path, DWORD desired_access, DWORD creation_disposition)
{
	struct xiso_entry entry;

	switch (creation_disposition)
	{
	case CREATE_NEW:
	case CREATE_ALWAYS:
	case OPEN_EXISTING:
	case OPEN_ALWAYS:
	case TRUNCATE_EXISTING:
		break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}
	if (!disc_find(path, &entry))
	{
		if (creation_disposition != OPEN_EXISTING && creation_disposition != TRUNCATE_EXISTING &&
			GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			SetLastError(ERROR_ACCESS_DENIED);
		}
		return INVALID_HANDLE_VALUE;
	}
	if (entry.is_directory || (desired_access & GENERIC_WRITE) ||
		(creation_disposition != OPEN_EXISTING && creation_disposition != OPEN_ALWAYS))
	{
		SetLastError(ERROR_ACCESS_DENIED);
		return INVALID_HANDLE_VALUE;
	}
	{
		HANDLE handle = file_handle_new(-1, &entry, path);

		if (handle != INVALID_HANDLE_VALUE)
			SetLastError(creation_disposition == OPEN_ALWAYS ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS);
		return handle;
	}
}

HANDLE WINAPI CreateFileA(LPCSTR file_name, DWORD desired_access, DWORD share_mode,
	LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition,
	DWORD flags_and_attributes, HANDLE template_file)
{
	HANDLE handle;
	struct posix_file_information information;
	char path[1024];
	const char *disc_path;
	int flags = 0;
	int existed;
	int descriptor;

	(void)share_mode;
	(void)security_attributes;
	(void)flags_and_attributes;
	(void)template_file;

	if (path_drive(file_name, &disc_path) == 'd')
		return create_disc_file(disc_path, desired_access, creation_disposition);

	platform_translate_path(file_name, path, sizeof(path));
	existed = posix_stat(path, &information) == 0;

	if ((desired_access & GENERIC_READ) && (desired_access & GENERIC_WRITE))
		flags = O_RDWR;
	else if (desired_access & GENERIC_WRITE)
		flags = O_WRONLY;
	else
		flags = O_RDONLY;

	switch (creation_disposition)
	{
	case CREATE_NEW:
		flags |= O_CREAT | O_EXCL;
		break;
	case CREATE_ALWAYS:
		flags |= O_CREAT | O_TRUNC;
		break;
	case OPEN_EXISTING:
		break;
	case OPEN_ALWAYS:
		flags |= O_CREAT;
		break;
	case TRUNCATE_EXISTING:
		flags |= O_TRUNC;
		break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}
	if ((flags & O_TRUNC) && (flags & O_ACCMODE) == O_RDONLY)
		flags = (flags & ~O_ACCMODE) | O_RDWR;

	if (existed && (information.flags & _posix_file_is_directory))
	{
		SetLastError(ERROR_ACCESS_DENIED);
		return INVALID_HANDLE_VALUE;
	}

	descriptor = open(path, flags | O_CLOEXEC, 0644);
	if (descriptor < 0)
	{
		platform_set_last_error_from_errno(errno);
		if (errno == ENOENT && !existed && (flags & O_CREAT))
			SetLastError(ERROR_PATH_NOT_FOUND);
		return INVALID_HANDLE_VALUE;
	}

	handle = file_handle_new(descriptor, NULL, path);
	if (handle == INVALID_HANDLE_VALUE)
		return handle;
	/* Win32 reports whether an OPEN_ALWAYS/CREATE_ALWAYS target existed */
	SetLastError((existed && (creation_disposition == OPEN_ALWAYS || creation_disposition == CREATE_ALWAYS)) ?
		ERROR_ALREADY_EXISTS : ERROR_SUCCESS);
	return handle;
}

#define READ_BOUNCE_SIZE 0x40000UL

static ssize_t read_some(struct platform_file *file, void *buffer, size_t count, BOOL positioned,
	unsigned long long offset)
{
	if (is_disc_file(file))
	{
		long result = xiso_fs_read(vfs_disc(), &file->entry, positioned ? offset : file->position, buffer, count);

		if (result < 0)
		{
			errno = EIO;
			return -1;
		}
		if (!positioned)
			file->position += (unsigned long long)result;
		return result;
	}
	return positioned ?
		pread(file->descriptor, buffer, count, (off_t)offset) :
		read(file->descriptor, buffer, count);
}

static BOOL read_at(struct platform_file *file, LPVOID buffer, DWORD count, LPDWORD bytes_read,
	BOOL positioned, unsigned long long offset)
{
	DWORD total = 0;
	/* The renderer write-protects guest memory it caches (memory_watch.c),
	and the kernel fails a read() into a protected page instead of
	faulting. Guest memory is therefore filled through a bounce buffer: the
	copy faults like any other write, at the moment the data really lands,
	so a texture uploaded while the read is in flight is refreshed. */
	BOOL bounce = platform_is_contiguous(buffer) ||
		platform_is_contiguous((char *)buffer + (count ? count - 1 : 0));
	char *staging = bounce ? malloc(count < READ_BOUNCE_SIZE ? count : READ_BOUNCE_SIZE) : NULL;

	if (bounce && !staging)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	while (total < count)
	{
		size_t wanted = count - total;
		ssize_t result;

		if (bounce && wanted > READ_BOUNCE_SIZE)
			wanted = READ_BOUNCE_SIZE;
		result = read_some(file, bounce ? staging : (char *)buffer + total, wanted, positioned, offset + total);
		if (result < 0)
		{
			if (errno == EINTR)
				continue;
			platform_set_last_error_from_errno(errno);
			if (bytes_read)
				*bytes_read = total;
			free(staging);
			return FALSE;
		}
		if (result == 0)
			break;
		if (bounce)
			memcpy((char *)buffer + total, staging, (size_t)result);
		total += (DWORD)result;
	}
	free(staging);
	if (bytes_read)
		*bytes_read = total;
	return TRUE;
}

static BOOL write_at(struct platform_file *file, LPCVOID buffer, DWORD count, LPDWORD bytes_written,
	BOOL positioned, unsigned long long offset)
{
	DWORD total = 0;

	if (is_disc_file(file))
	{
		if (bytes_written)
			*bytes_written = 0;
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	while (total < count)
	{
		ssize_t result = positioned ?
			pwrite(file->descriptor, (const char *)buffer + total, count - total, (off_t)(offset + total)) :
			write(file->descriptor, (const char *)buffer + total, count - total);

		if (result < 0)
		{
			if (errno == EINTR)
				continue;
			platform_set_last_error_from_errno(errno);
			if (bytes_written)
				*bytes_written = total;
			return FALSE;
		}
		total += (DWORD)result;
	}
	if (bytes_written)
		*bytes_written = total;
	return TRUE;
}

static unsigned long long overlapped_offset(LPOVERLAPPED overlapped)
{
	return ((unsigned long long)overlapped->OffsetHigh << 32) | overlapped->Offset;
}

BOOL WINAPI ReadFile(HANDLE handle, LPVOID buffer, DWORD count, LPDWORD bytes_read, LPOVERLAPPED overlapped)
{
	struct platform_file *file = file_from_handle(handle);
	BOOL result;
	DWORD done = 0;

	if (!file)
		return FALSE;
	result = read_at(file, buffer, count, &done, overlapped != NULL, overlapped ? overlapped_offset(overlapped) : 0);
	if (bytes_read)
		*bytes_read = done;
	if (overlapped)
	{
		overlapped->Internal = result ? 0 : GetLastError();
		overlapped->InternalHigh = done;
		if (overlapped->hEvent)
			SetEvent(overlapped->hEvent);
	}
	return result;
}

BOOL WINAPI WriteFile(HANDLE handle, LPCVOID buffer, DWORD count, LPDWORD bytes_written, LPOVERLAPPED overlapped)
{
	struct platform_file *file = file_from_handle(handle);
	BOOL result;
	DWORD done = 0;

	if (!file)
		return FALSE;
	result = write_at(file, buffer, count, &done, overlapped != NULL, overlapped ? overlapped_offset(overlapped) : 0);
	if (bytes_written)
		*bytes_written = done;
	if (overlapped)
	{
		overlapped->Internal = result ? 0 : GetLastError();
		overlapped->InternalHigh = done;
		if (overlapped->hEvent)
			SetEvent(overlapped->hEvent);
	}
	return result;
}

/* ReadFileEx/WriteFileEx complete immediately; as on Win32, the completion
routine runs during the issuing thread's next alertable wait. */

static void file_completion_apc(void *routine, void *overlapped, void *unused)
{
	LPOVERLAPPED request = overlapped;

	(void)unused;
	((LPOVERLAPPED_COMPLETION_ROUTINE)routine)((DWORD)request->Internal, (DWORD)request->InternalHigh, request);
}

BOOL WINAPI ReadFileEx(HANDLE handle, LPVOID buffer, DWORD count, LPOVERLAPPED overlapped,
	LPOVERLAPPED_COMPLETION_ROUTINE completion_routine)
{
	struct platform_file *file = file_from_handle(handle);
	DWORD done = 0;
	BOOL result;

	if (!file || !overlapped)
	{
		if (file)
			SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	result = read_at(file, buffer, count, &done, TRUE, overlapped_offset(overlapped));
	overlapped->Internal = result ? ERROR_SUCCESS : GetLastError();
	if (result && done == 0 && count > 0)
		overlapped->Internal = ERROR_HANDLE_EOF;
	overlapped->InternalHigh = done;
	platform_queue_apc(file_completion_apc, (void *)completion_routine, overlapped, NULL);
	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

BOOL WINAPI WriteFileEx(HANDLE handle, LPCVOID buffer, DWORD count, LPOVERLAPPED overlapped,
	LPOVERLAPPED_COMPLETION_ROUTINE completion_routine)
{
	struct platform_file *file = file_from_handle(handle);
	DWORD done = 0;
	BOOL result;

	if (!file || !overlapped)
	{
		if (file)
			SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (is_disc_file(file))
	{
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	result = write_at(file, buffer, count, &done, TRUE, overlapped_offset(overlapped));
	overlapped->Internal = result ? ERROR_SUCCESS : GetLastError();
	overlapped->InternalHigh = done;
	platform_queue_apc(file_completion_apc, (void *)completion_routine, overlapped, NULL);
	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

static DWORD disc_set_file_pointer(struct platform_file *file, LONG distance, PLONG distance_high, DWORD method)
{
	long long move = distance_high ?
		(long long)(((unsigned long long)(unsigned long)*distance_high << 32) | (unsigned long)distance) :
		(long long)distance;
	long long base;
	long long position;

	switch (method)
	{
	case FILE_BEGIN: base = 0; break;
	case FILE_CURRENT: base = (long long)file->position; break;
	case FILE_END: base = (long long)file->entry.size; break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_SET_FILE_POINTER;
	}
	position = base + move;
	if (position < 0)
	{
		SetLastError(ERROR_NEGATIVE_SEEK);
		return INVALID_SET_FILE_POINTER;
	}
	file->position = (unsigned long long)position;
	if (distance_high)
		*distance_high = (LONG)(file->position >> 32);
	SetLastError(ERROR_SUCCESS);
	return (DWORD)file->position;
}

DWORD WINAPI SetFilePointer(HANDLE handle, LONG distance, PLONG distance_high, DWORD method)
{
	struct platform_file *file = file_from_handle(handle);
	unsigned long low, high;
	int whence;

	if (!file)
		return INVALID_SET_FILE_POINTER;
	if (is_disc_file(file))
		return disc_set_file_pointer(file, distance, distance_high, method);
	switch (method)
	{
	case FILE_BEGIN: whence = SEEK_SET; break;
	case FILE_CURRENT: whence = SEEK_CUR; break;
	case FILE_END: whence = SEEK_END; break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_SET_FILE_POINTER;
	}
	if (posix_seek(file->descriptor, distance,
		distance_high ? *distance_high : (distance < 0 ? -1 : 0), whence, &low, &high) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return INVALID_SET_FILE_POINTER;
	}
	if (distance_high)
		*distance_high = (LONG)high;
	SetLastError(ERROR_SUCCESS);
	return (DWORD)low;
}

/* stat() of an open file, disc or host */
static BOOL file_information(struct platform_file *file, struct posix_file_information *information)
{
	if (is_disc_file(file))
	{
		disc_information(&file->entry, information);
		return TRUE;
	}
	if (posix_fstat(file->descriptor, information) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI GetFileSize(HANDLE handle, LPDWORD size_high)
{
	struct platform_file *file = file_from_handle(handle);
	struct posix_file_information information;

	if (!file)
		return INVALID_FILE_SIZE;
	if (!file_information(file, &information))
		return INVALID_FILE_SIZE;
	if (size_high)
		*size_high = information.size_high;
	SetLastError(ERROR_SUCCESS);
	return information.size_low;
}

BOOL WINAPI SetEndOfFile(HANDLE handle)
{
	struct platform_file *file = file_from_handle(handle);
	unsigned long low, high;

	if (!file)
		return FALSE;
	if (is_disc_file(file))
	{
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if (posix_seek(file->descriptor, 0, 0, SEEK_CUR, &low, &high) != 0 ||
		posix_truncate(file->descriptor, low, high) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI GetFileTime(HANDLE handle, LPFILETIME creation_time, LPFILETIME last_access_time, LPFILETIME last_write_time)
{
	struct platform_file *file = file_from_handle(handle);
	struct posix_file_information information;

	if (!file)
		return FALSE;
	if (!file_information(file, &information))
		return FALSE;
	if (creation_time)
		platform_unix_time_to_filetime(information.creation_seconds, information.creation_nanoseconds, creation_time);
	if (last_access_time)
		platform_unix_time_to_filetime(information.access_seconds, information.access_nanoseconds, last_access_time);
	if (last_write_time)
		platform_unix_time_to_filetime(information.modification_seconds, information.modification_nanoseconds, last_write_time);
	return TRUE;
}

BOOL WINAPI SetFileTime(HANDLE handle, CONST FILETIME *creation_time, CONST FILETIME *last_access_time,
	CONST FILETIME *last_write_time)
{
	struct platform_file *file = file_from_handle(handle);
	unsigned long access_seconds = 0, access_nanoseconds = 0;
	unsigned long write_seconds = 0, write_nanoseconds = 0;

	(void)creation_time;
	if (!file)
		return FALSE;
	if (is_disc_file(file))
	{
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if (last_access_time)
		platform_filetime_to_unix_time(last_access_time, &access_seconds, &access_nanoseconds);
	if (last_write_time)
		platform_filetime_to_unix_time(last_write_time, &write_seconds, &write_nanoseconds);
	if (posix_set_file_times(file->path, access_seconds, access_nanoseconds, write_seconds, write_nanoseconds) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

static void fill_attribute_data(const struct posix_file_information *information, WIN32_FILE_ATTRIBUTE_DATA *data)
{
	data->dwFileAttributes = 0;
	if (information->flags & _posix_file_is_directory)
		data->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
	if (information->flags & _posix_file_is_read_only)
		data->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
	if (!data->dwFileAttributes)
		data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	platform_unix_time_to_filetime(information->creation_seconds, information->creation_nanoseconds, &data->ftCreationTime);
	platform_unix_time_to_filetime(information->access_seconds, information->access_nanoseconds, &data->ftLastAccessTime);
	platform_unix_time_to_filetime(information->modification_seconds, information->modification_nanoseconds, &data->ftLastWriteTime);
	data->nFileSizeHigh = information->size_high;
	data->nFileSizeLow = information->size_low;
}

/* 0x001e91ca in the XBE (XAPI) */
BOOL WINAPI GetFileInformationByHandle(HANDLE handle, struct vfs_by_handle_file_information *result)
{
	struct platform_file *file = file_from_handle(handle);
	struct posix_file_information information;
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (!file || !file_information(file, &information))
		return FALSE;
	fill_attribute_data(&information, &data);
	memset(result, 0, sizeof(*result));
	result->dwFileAttributes = data.dwFileAttributes;
	memcpy(result->ftCreationTime, &data.ftCreationTime, sizeof(result->ftCreationTime));
	memcpy(result->ftLastAccessTime, &data.ftLastAccessTime, sizeof(result->ftLastAccessTime));
	memcpy(result->ftLastWriteTime, &data.ftLastWriteTime, sizeof(result->ftLastWriteTime));
	result->nFileSizeHigh = data.nFileSizeHigh;
	result->nFileSizeLow = data.nFileSizeLow;
	result->nNumberOfLinks = 1;
	/* a disc file's index is its first sector, which is unique */
	result->nFileIndexLow = is_disc_file(file) ? file->entry.sector : 0;
	return TRUE;
}

/* ---------- operations on paths */

/* stat() by Xbox path, disc or host; FALSE with the Win32 error set */
static BOOL path_information(LPCSTR file_name, struct posix_file_information *information)
{
	const char *disc_path;
	char path[1024];

	if (path_drive(file_name, &disc_path) == 'd')
	{
		struct xiso_entry entry;

		if (!disc_find(disc_path, &entry))
			return FALSE;
		disc_information(&entry, information);
		return TRUE;
	}
	platform_translate_path(file_name, path, sizeof(path));
	if (posix_stat(path, information) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

/* TRUE (with ERROR_ACCESS_DENIED set) if a path is on the read-only disc */
static BOOL deny_disc_write(LPCSTR file_name)
{
	if (!vfs_is_disc_path(file_name))
		return FALSE;
	SetLastError(ERROR_ACCESS_DENIED);
	return TRUE;
}

DWORD WINAPI GetFileAttributesA(LPCSTR file_name)
{
	struct posix_file_information information;
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (!path_information(file_name, &information))
		return (DWORD)-1;
	fill_attribute_data(&information, &data);
	return data.dwFileAttributes;
}

BOOL WINAPI GetFileAttributesExA(LPCSTR file_name, GET_FILEEX_INFO_LEVELS level, LPVOID file_information)
{
	struct posix_file_information information;

	if (level != GetFileExInfoStandard)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (!path_information(file_name, &information))
		return FALSE;
	fill_attribute_data(&information, file_information);
	return TRUE;
}

BOOL WINAPI SetFileAttributesA(LPCSTR file_name, DWORD attributes)
{
	char path[1024];
	struct posix_file_information information;

	if (deny_disc_write(file_name))
		return FALSE;
	platform_translate_path(file_name, path, sizeof(path));
	if (posix_stat(path, &information) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	/* only the read-only bit has a POSIX equivalent */
	if (posix_set_read_only(path, (attributes & FILE_ATTRIBUTE_READONLY) != 0) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI DeleteFileA(LPCSTR file_name)
{
	char path[1024];

	if (deny_disc_write(file_name))
		return FALSE;
	platform_translate_path(file_name, path, sizeof(path));
	if (unlink(path) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI MoveFileA(LPCSTR existing_file_name, LPCSTR new_file_name)
{
	char from[1024], to[1024];
	struct posix_file_information information;

	if (deny_disc_write(existing_file_name) || deny_disc_write(new_file_name))
		return FALSE;
	platform_translate_path(existing_file_name, from, sizeof(from));
	platform_translate_path(new_file_name, to, sizeof(to));
	if (posix_stat(to, &information) == 0)
	{
		SetLastError(ERROR_ALREADY_EXISTS);
		return FALSE;
	}
	if (rename(from, to) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI CopyFileA(LPCSTR existing_file_name, LPCSTR new_file_name, BOOL fail_if_exists)
{
	char to[1024];
	char buffer[65536];
	HANDLE source;
	int destination;
	BOOL success = TRUE;

	if (deny_disc_write(new_file_name))
		return FALSE;
	/* the source may be on the disc: read it through the file API */
	source = CreateFileA(existing_file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (source == INVALID_HANDLE_VALUE)
		return FALSE;
	platform_translate_path(new_file_name, to, sizeof(to));
	destination = open(to, O_WRONLY | O_CREAT | O_CLOEXEC | (fail_if_exists ? O_EXCL : O_TRUNC), 0644);
	if (destination < 0)
	{
		platform_set_last_error_from_errno(errno);
		CloseHandle(source);
		return FALSE;
	}
	for (;;)
	{
		DWORD count = 0;

		if (!ReadFile(source, buffer, sizeof(buffer), &count, NULL))
		{
			success = FALSE;
			break;
		}
		if (!count)
			break;
		if (write(destination, buffer, count) != (ssize_t)count)
		{
			platform_set_last_error_from_errno(errno);
			success = FALSE;
			break;
		}
	}
	CloseHandle(source);
	close(destination);
	return success;
}

BOOL WINAPI CreateDirectoryA(LPCSTR path_name, LPSECURITY_ATTRIBUTES security_attributes)
{
	char path[1024];

	(void)security_attributes;
	if (deny_disc_write(path_name))
		return FALSE;
	platform_translate_path(path_name, path, sizeof(path));
	if (posix_make_directory(path) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI RemoveDirectoryA(LPCSTR path_name)
{
	char path[1024];

	if (deny_disc_write(path_name))
		return FALSE;
	platform_translate_path(path_name, path, sizeof(path));
	if (rmdir(path) != 0)
	{
		platform_set_last_error_from_errno(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI GetDiskFreeSpaceExA(LPCSTR directory_name, PULARGE_INTEGER free_bytes_available,
	PULARGE_INTEGER total_bytes, PULARGE_INTEGER total_free_bytes)
{
	char path[1024];
	unsigned long free_low, free_high, total_low, total_high;

	if (!directory_name || vfs_is_disc_path(directory_name))
	{
		/* the disc: full, as big as the image */
		struct xiso_fs *fs = vfs_disc();
		unsigned long long total = fs ? fs->image_size : 0;

		if (!fs)
		{
			SetLastError(ERROR_PATH_NOT_FOUND);
			return FALSE;
		}
		free_low = free_high = 0;
		total_low = (unsigned long)total;
		total_high = (unsigned long)(total >> 32);
	}
	else
	{
		platform_translate_path(directory_name, path, sizeof(path));
		if (posix_disk_space(path, &free_low, &free_high, &total_low, &total_high) != 0)
		{
			platform_set_last_error_from_errno(errno);
			return FALSE;
		}
	}
	if (free_bytes_available)
	{
		free_bytes_available->LowPart = free_low;
		free_bytes_available->HighPart = free_high;
	}
	if (total_free_bytes)
	{
		total_free_bytes->LowPart = free_low;
		total_free_bytes->HighPart = free_high;
	}
	if (total_bytes)
	{
		total_bytes->LowPart = total_low;
		total_bytes->HighPart = total_high;
	}
	return TRUE;
}

/* ---------- directory enumeration */

struct platform_find
{
	/* a host directory, or NULL for a disc directory */
	void *directory;
	char directory_path[1024];
	char pattern[MAX_PATH];
	/* a disc directory's entries */
	struct xiso_entry *entries;
	int entry_count;
	int entry_capacity;
	int next_entry;
};

static void find_destroy(struct platform_handle *handle)
{
	struct platform_find *find = handle->data;

	if (find->directory)
		posix_directory_close(find->directory);
	free(find->entries);
	free(find);
}

/* Win32 wildcard match: '*' any run, '?' one character, case-insensitive;
"*.*" also matches names without a dot */
static BOOL wildcard_match(const char *pattern, const char *name)
{
	if (!strcmp(pattern, "*.*") || !strcmp(pattern, "*"))
		return TRUE;
	while (*pattern)
	{
		if (*pattern == '*')
		{
			while (*pattern == '*')
				pattern++;
			if (!*pattern)
				return TRUE;
			for (; *name; name++)
			{
				if (wildcard_match(pattern, name))
					return TRUE;
			}
			return FALSE;
		}
		if (!*name)
			return FALSE;
		if (*pattern != '?' && tolower((unsigned char)*pattern) != tolower((unsigned char)*name))
			return FALSE;
		pattern++;
		name++;
	}
	return *name == '\0';
}

static void fill_find_data(const struct posix_file_information *information, const char *name,
	LPWIN32_FIND_DATAA data)
{
	WIN32_FILE_ATTRIBUTE_DATA attributes;

	fill_attribute_data(information, &attributes);
	memset(data, 0, sizeof(*data));
	data->dwFileAttributes = attributes.dwFileAttributes;
	data->ftCreationTime = attributes.ftCreationTime;
	data->ftLastAccessTime = attributes.ftLastAccessTime;
	data->ftLastWriteTime = attributes.ftLastWriteTime;
	data->nFileSizeHigh = attributes.nFileSizeHigh;
	data->nFileSizeLow = attributes.nFileSizeLow;
	snprintf(data->cFileName, sizeof(data->cFileName), "%s", name);
}

static BOOL find_next_entry(struct platform_find *find, LPWIN32_FIND_DATAA data)
{
	char name[MAX_PATH];
	struct posix_file_information information;

	if (!find->directory)
	{
		while (find->next_entry < find->entry_count)
		{
			const struct xiso_entry *entry = &find->entries[find->next_entry++];

			if (!wildcard_match(find->pattern, entry->name))
				continue;
			disc_information(entry, &information);
			fill_find_data(&information, entry->name, data);
			return TRUE;
		}
		SetLastError(ERROR_NO_MORE_FILES);
		return FALSE;
	}
	while (posix_directory_next(find->directory, name, sizeof(name)))
	{
		char full_path[1400];

		if (!wildcard_match(find->pattern, name))
			continue;
		snprintf(full_path, sizeof(full_path), "%s/%s", find->directory_path, name);
		if (posix_stat(full_path, &information) != 0)
			continue;
		fill_find_data(&information, name, data);
		return TRUE;
	}
	SetLastError(ERROR_NO_MORE_FILES);
	return FALSE;
}

static int collect_disc_entry(void *context, const struct xiso_entry *entry)
{
	struct platform_find *find = context;

	if (find->entry_count == find->entry_capacity)
	{
		int capacity = find->entry_capacity ? find->entry_capacity * 2 : 32;
		struct xiso_entry *entries = realloc(find->entries, sizeof(*entries) * (size_t)capacity);

		if (!entries)
			return 1;
		find->entries = entries;
		find->entry_capacity = capacity;
	}
	find->entries[find->entry_count++] = *entry;
	return 0;
}

/* lists a disc directory into find; FALSE with the Win32 error set */
static BOOL open_disc_directory(struct platform_find *find, const char *xbox_directory)
{
	const char *disc_path;
	struct xiso_entry directory;

	path_drive(xbox_directory, &disc_path);
	if (!disc_find(disc_path, &directory))
		return FALSE;
	if (!directory.is_directory)
	{
		SetLastError(ERROR_PATH_NOT_FOUND);
		return FALSE;
	}
	if (xiso_fs_list(vfs_disc(), &directory, collect_disc_entry, find) < 0)
	{
		SetLastError(ERROR_PATH_NOT_FOUND);
		return FALSE;
	}
	return TRUE;
}

HANDLE WINAPI FindFirstFileA(LPCSTR file_name, LPWIN32_FIND_DATAA data)
{
	struct platform_find *find = calloc(1, sizeof(*find));
	struct platform_handle *handle;
	char xbox_directory[MAX_PATH];
	const char *separator = last_separator(file_name);

	if (!find)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return INVALID_HANDLE_VALUE;
	}
	if (separator)
	{
		snprintf(xbox_directory, sizeof(xbox_directory), "%.*s", (int)(separator - file_name), file_name);
		snprintf(find->pattern, sizeof(find->pattern), "%s", separator + 1);
	}
	else
	{
		/* "x:pattern" or "pattern": the drive's root */
		const char *rest;
		char drive = path_drive(file_name, &rest);

		snprintf(xbox_directory, sizeof(xbox_directory), "%c:", drive);
		snprintf(find->pattern, sizeof(find->pattern), "%s", rest);
	}
	if (vfs_is_disc_path(xbox_directory))
	{
		if (!open_disc_directory(find, xbox_directory))
		{
			free(find->entries);
			free(find);
			return INVALID_HANDLE_VALUE;
		}
	}
	else
	{
		platform_translate_path(xbox_directory, find->directory_path, sizeof(find->directory_path));
		find->directory = posix_directory_open(find->directory_path);
		if (!find->directory)
		{
			platform_set_last_error_from_errno(errno);
			free(find);
			return INVALID_HANDLE_VALUE;
		}
	}
	handle = platform_handle_new(_platform_handle_find, find, find_destroy);
	if (!handle)
	{
		if (find->directory)
			posix_directory_close(find->directory);
		free(find->entries);
		free(find);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return INVALID_HANDLE_VALUE;
	}
	if (!find_next_entry(find, data))
	{
		CloseHandle(handle);
		SetLastError(ERROR_FILE_NOT_FOUND);
		return INVALID_HANDLE_VALUE;
	}
	return handle;
}

BOOL WINAPI FindNextFileA(HANDLE find_file, LPWIN32_FIND_DATAA data)
{
	struct platform_handle *handle = platform_handle_get(find_file, _platform_handle_find);

	if (!handle)
		return FALSE;
	return find_next_entry(handle->data, data);
}

/* not declared by the XDK headers here (vfs_files.h) */
BOOL WINAPI FindClose(HANDLE find_file)
{
	if (!platform_handle_get(find_file, _platform_handle_find))
		return FALSE;
	return CloseHandle(find_file);
}

/* ---------- the utility drive (z:\) */

/* deletes everything inside a host directory, not the directory itself.
A link is removed, never followed: unlink() succeeds on it, and only a real
directory (where unlink() fails) is entered */
static void remove_contents(const char *path, int depth)
{
	void *directory;
	char name[MAX_PATH];

	if (depth > 32 || !(directory = posix_directory_open(path)))
		return;
	while (posix_directory_next(directory, name, sizeof(name)))
	{
		char child[1400];

		snprintf(child, sizeof(child), "%s/%s", path, name);
		if (unlink(child) == 0)
			continue;
		remove_contents(child, depth + 1);
		rmdir(child);
	}
	posix_directory_close(directory);
}

/* 0x00030e25 in the XBE (XAPI). On the Xbox it maps z:\ to the title's
cache partition on the hard disk; here z:\ is <save root>/z, which
platform_translate_path also creates on first use. */
BOOL WINAPI XMountUtilityDrive(BOOL format_clean)
{
	char path[1024];

	platform_translate_path("z:\\", path, sizeof(path));
	if (!directory_exists(path))
	{
		SetLastError(ERROR_PATH_NOT_FOUND);
		return FALSE;
	}
	if (format_clean)
		remove_contents(path, 0);
	return TRUE;
}

BOOL WINAPI XFormatUtilityDrive(void)
{
	return XMountUtilityDrive(TRUE);
}

/* ---------- the C library's fopen() by Xbox path */

struct disc_stream
{
	struct xiso_entry entry;
	unsigned long long position;
};

static ssize_t disc_stream_read(void *cookie, char *buffer, size_t size)
{
	struct disc_stream *stream = cookie;
	long result = xiso_fs_read(vfs_disc(), &stream->entry, stream->position, buffer, size);

	if (result < 0)
	{
		errno = EIO;
		return -1;
	}
	stream->position += (unsigned long long)result;
	return result;
}

static int disc_stream_seek(void *cookie, off_t *offset, int whence)
{
	struct disc_stream *stream = cookie;
	long long base;

	switch (whence)
	{
	case SEEK_SET: base = 0; break;
	case SEEK_CUR: base = (long long)stream->position; break;
	case SEEK_END: base = (long long)stream->entry.size; break;
	default:
		errno = EINVAL;
		return -1;
	}
	if (base + (long long)*offset < 0)
	{
		errno = EINVAL;
		return -1;
	}
	stream->position = (unsigned long long)(base + (long long)*offset);
	*offset = (off_t)stream->position;
	return 0;
}

static int disc_stream_close(void *cookie)
{
	free(cookie);
	return 0;
}

FILE *vfs_fopen(const char *xbox_path, const char *mode)
{
	const char *disc_path;
	struct disc_stream *stream;
	static const cookie_io_functions_t functions = { disc_stream_read, NULL, disc_stream_seek, disc_stream_close };
	FILE *file;

	if (path_drive(xbox_path, &disc_path) != 'd')
	{
		char host_path[1024];

		platform_translate_path(xbox_path, host_path, sizeof(host_path));
		return fopen(host_path, mode);
	}
	if (strpbrk(mode, "wa+"))
	{
		errno = EROFS;
		return NULL;
	}
	stream = calloc(1, sizeof(*stream));
	if (!stream)
	{
		errno = ENOMEM;
		return NULL;
	}
	if (!disc_find(disc_path, &stream->entry))
	{
		free(stream);
		errno = ENOENT;
		return NULL;
	}
	if (stream->entry.is_directory)
	{
		free(stream);
		errno = EISDIR;
		return NULL;
	}
	file = fopencookie(stream, "r", functions);
	if (!file)
		free(stream);
	return file;
}
