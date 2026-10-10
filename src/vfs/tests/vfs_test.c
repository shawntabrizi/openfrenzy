/*
VFS_TEST.C

Guest test of the vfs module (src/vfs): the Xbox file API on the player's
disc image and on the save drives, as the game uses it. Built and run by
src/vfs/tests/run_tests.sh, which also checks the LIST and CRC lines below
against scripts/xiso_list.py, an independent reader of the same image.

Why these checks matter:
- the game opens every package as d:\Packages\<name>.xbp in any case, so a
  case-insensitive, read-in-place D: drive is the whole data path;
- bKernel's xbCache.cpp enables its z:\ file cache only when
  d:\_T_E_S_T.___ does NOT open, so a missing disc file must fail cleanly;
- the disc is read only, as on the Xbox: writes there must fail and must
  never touch the host;
- saves (t:\, u:\) and the cache (z:\) must persist on the host.

Prints "ok <check>" or "FAIL <check>" per check; the exit code is the number
of failures.
*/

#include "platform.h"
#include "vfs_files.h"
#include "xiso_fs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void check(int condition, const char *name)
{
	printf("%s %s\n", condition ? "ok" : "FAIL", name);
	if (!condition)
		failures++;
}

static unsigned int crc_table[256];

static unsigned int crc32_update(unsigned int crc, const unsigned char *bytes, unsigned long count)
{
	unsigned long index;

	if (!crc_table[1])
	{
		unsigned int value, bit;

		for (value = 0; value < 256; value++)
		{
			unsigned int c = value;

			for (bit = 0; bit < 8; bit++)
				c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			crc_table[value] = c;
		}
	}
	crc = ~crc;
	for (index = 0; index < count; index++)
		crc = crc_table[(crc ^ bytes[index]) & 0xFF] ^ (crc >> 8);
	return ~crc;
}

/* reads a whole file with ReadFile in odd-sized chunks (so reads cross
sector boundaries); prints its CRC-32 for run_tests.sh */
static void crc_of_file(const char *path)
{
	static unsigned char buffer[100003];
	HANDLE file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	unsigned int crc = 0;
	unsigned long total = 0;
	DWORD count;

	if (file == INVALID_HANDLE_VALUE)
	{
		check(0, "crc: open");
		return;
	}
	while (ReadFile(file, buffer, sizeof(buffer), &count, NULL) && count)
	{
		crc = crc32_update(crc, buffer, count);
		total += count;
	}
	check(total == GetFileSize(file, NULL), "crc: read the whole file");
	CloseHandle(file);
	printf("CRC %s %08x %lu\n", path, crc, total);
}

static int completions;

static VOID WINAPI vfs_test_completion(DWORD error, DWORD count, LPOVERLAPPED overlapped)
{
	(void)error;
	(void)count;
	(void)overlapped;
	completions++;
}

static void test_disc_read(void)
{
	HANDLE file;
	unsigned char bytes[8];
	DWORD count = 0, high = 1;
	DWORD size;

	file = CreateFileA("d:\\default.xbe", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	check(file != INVALID_HANDLE_VALUE, "disc: open d:\\default.xbe");
	if (file == INVALID_HANDLE_VALUE)
		return;
	check(ReadFile(file, bytes, 4, &count, NULL) && count == 4 && !memcmp(bytes, "XBEH", 4),
		"disc: default.xbe starts with XBEH");
	size = GetFileSize(file, &high);
	check(size == 2834432 && high == 0, "disc: GetFileSize(default.xbe) is the XBE's 2834432 bytes");
	check(SetFilePointer(file, -4, NULL, FILE_END) == size - 4, "disc: SetFilePointer from the end");
	check(ReadFile(file, bytes, 8, &count, NULL) && count == 4, "disc: a read at the end is short");
	check(ReadFile(file, bytes, 8, &count, NULL) && count == 0, "disc: a read past the end gives 0 bytes");
	check(SetFilePointer(file, 0, NULL, FILE_BEGIN) == 0 && SetFilePointer(file, 2, NULL, FILE_CURRENT) == 2 &&
		ReadFile(file, bytes, 2, &count, NULL) && count == 2 && !memcmp(bytes, "EH", 2),
		"disc: SetFilePointer from the start and the current position");
	check(SetFilePointer(file, -1, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER,
		"disc: a negative position is refused");
	{
		OVERLAPPED overlapped;

		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.Offset = 1;
		check(ReadFile(file, bytes, 3, &count, &overlapped) && count == 3 && !memcmp(bytes, "BEH", 3),
			"disc: ReadFile at an OVERLAPPED offset");
	}
	{
		struct vfs_by_handle_file_information information;

		check(GetFileInformationByHandle(file, &information) && information.nFileSizeLow == size &&
			(information.dwFileAttributes & FILE_ATTRIBUTE_READONLY),
			"disc: GetFileInformationByHandle gives the size, read only");
	}
	check(!WriteFile(file, bytes, 1, &count, NULL) && GetLastError() == ERROR_ACCESS_DENIED,
		"disc: WriteFile on a disc file is denied");
	check(!SetEndOfFile(file) && GetLastError() == ERROR_ACCESS_DENIED, "disc: SetEndOfFile is denied");
	CloseHandle(file);
}

static void test_disc_names(void)
{
	static const char *const spellings[] =
	{
		"d:\\Packages\\BootSeq.xbp", "D:\\PACKAGES\\bootseq.XBP", "d:/packages/BootSeq.xbp",
		"Packages\\BootSeq.xbp", "d:\\Packages\\.\\..\\Packages\\\\BootSeq.xbp",
	};
	unsigned int index;

	for (index = 0; index < sizeof(spellings) / sizeof(spellings[0]); index++)
	{
		HANDLE file = CreateFileA(spellings[index], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		char name[160];

		snprintf(name, sizeof(name), "names: %s opens (4096 bytes)", spellings[index]);
		check(file != INVALID_HANDLE_VALUE && GetFileSize(file, NULL) == 4096, name);
		if (file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
	}
	check(CreateFileA("d:\\nothing.bin", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL) == INVALID_HANDLE_VALUE &&
		GetLastError() == ERROR_FILE_NOT_FOUND, "names: a missing file is ERROR_FILE_NOT_FOUND");
	check(CreateFileA("d:\\nowhere\\x.bin", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL) == INVALID_HANDLE_VALUE &&
		GetLastError() == ERROR_PATH_NOT_FOUND, "names: a missing directory is ERROR_PATH_NOT_FOUND");
	check(CreateFileA("d:\\Packages", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL) == INVALID_HANDLE_VALUE &&
		GetLastError() == ERROR_ACCESS_DENIED, "names: a directory does not open as a file");
	check(GetFileAttributesA("d:\\packages") == (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY),
		"names: GetFileAttributesA(d:\\packages) is a read-only directory");
	check(GetFileAttributesA("d:\\nothing.bin") == (DWORD)-1, "names: GetFileAttributesA of a missing file fails");
}

static void test_disc_read_only(void)
{
	check(CreateFileA("d:\\new.bin", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL) == INVALID_HANDLE_VALUE &&
		GetLastError() == ERROR_ACCESS_DENIED, "read only: creating a disc file is denied");
	check(CreateFileA("d:\\default.xbe", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL) ==
		INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED,
		"read only: opening a disc file for writing is denied");
	check(!DeleteFileA("d:\\default.xbe") && GetLastError() == ERROR_ACCESS_DENIED,
		"read only: DeleteFileA on the disc is denied");
	check(!CreateDirectoryA("d:\\newdir", NULL) && GetLastError() == ERROR_ACCESS_DENIED,
		"read only: CreateDirectoryA on the disc is denied");
	/* xbCache.cpp's probe: the cache is enabled only when this fails */
	errno = 0;
	check(vfs_fopen("d:\\_T_E_S_T.___", "rb") == NULL && errno == ENOENT,
		"read only: vfs_fopen(d:\\_T_E_S_T.___) fails (z:\\ cache enabled)");
	errno = 0;
	check(vfs_fopen("d:\\debugLog.txt", "w") == NULL && errno == EROFS,
		"read only: vfs_fopen for writing on the disc fails with EROFS");
	{
		ULARGE_INTEGER free_bytes, total_bytes;

		check(GetDiskFreeSpaceExA("d:\\", &free_bytes, &total_bytes, NULL) && free_bytes.QuadPart == 0 &&
			total_bytes.QuadPart > 2834432, "read only: GetDiskFreeSpaceExA(d:\\) is full");
	}
}

/* bKernel's xbBkgload.c finishes a channel only when ReadFileEx returns
FALSE; its completion routine ignores an error and leaves the loader
waiting. As in XAPI (0x001e9307), a read that starts at the end must fail
at once and queue no completion. */
static void test_disc_read_ex(void)
{
	HANDLE file = CreateFileA("d:\\default.xbe", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	OVERLAPPED overlapped;
	unsigned char bytes[8];

	check(file != INVALID_HANDLE_VALUE, "ReadFileEx: open d:\\default.xbe");
	if (file == INVALID_HANDLE_VALUE)
		return;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.Offset = GetFileSize(file, NULL);
	check(!ReadFileEx(file, bytes, sizeof(bytes), &overlapped, vfs_test_completion) &&
		GetLastError() == ERROR_HANDLE_EOF,
		"ReadFileEx: a read at the end returns FALSE with ERROR_HANDLE_EOF");
	SleepEx(0, TRUE);
	check(completions == 0, "ReadFileEx: a failed read queues no completion routine");
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.Offset = 1;
	check(ReadFileEx(file, bytes, 3, &overlapped, vfs_test_completion), "ReadFileEx: a read inside the file starts");
	SleepEx(0, TRUE);
	check(completions == 1 && !memcmp(bytes, "BEH", 3), "ReadFileEx: its completion routine runs in an alertable wait");
	CloseHandle(file);
}

static void test_disc_fopen(void)
{
	FILE *file = vfs_fopen("D:\\DEFAULT.XBE", "rb");
	char bytes[4];

	check(file != NULL, "fopen: vfs_fopen(D:\\DEFAULT.XBE)");
	if (!file)
		return;
	check(fread(bytes, 1, 4, file) == 4 && !memcmp(bytes, "XBEH", 4), "fopen: fread gives XBEH");
	check(fseek(file, 0, SEEK_END) == 0 && ftell(file) == 2834432, "fopen: fseek/ftell give the size");
	check(fseek(file, 1, SEEK_SET) == 0 && fgetc(file) == 'B', "fopen: fseek back and fgetc");
	fclose(file);
}

static void test_disc_find(void)
{
	WIN32_FIND_DATAA data;
	HANDLE find = FindFirstFileA("d:\\Packages\\*", &data);
	int count = 0;

	check(find != INVALID_HANDLE_VALUE, "find: FindFirstFileA(d:\\Packages\\*)");
	if (find == INVALID_HANDLE_VALUE)
		return;
	do
	{
		printf("LIST %s %lu %s\n", data.cFileName, (unsigned long)data.nFileSizeLow,
			(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "dir" : "file");
		count++;
	} while (FindNextFileA(find, &data));
	check(GetLastError() == ERROR_NO_MORE_FILES, "find: the listing ends with ERROR_NO_MORE_FILES");
	FindClose(find);
	check(count > 0, "find: d:\\Packages has entries");

	count = 0;
	find = FindFirstFileA("D:\\packages\\BUMP-V0?.xbp", &data);
	if (find != INVALID_HANDLE_VALUE)
	{
		do
			count++;
		while (FindNextFileA(find, &data));
		FindClose(find);
	}
	check(count == 4, "find: d:\\packages\\BUMP-V0?.xbp matches bump-v01..v04");
	check(FindFirstFileA("d:\\Packages\\*.none", &data) == INVALID_HANDLE_VALUE &&
		GetLastError() == ERROR_FILE_NOT_FOUND, "find: no match is ERROR_FILE_NOT_FOUND");
}

static void test_save_drives(void)
{
	static const char text[] = "openfrenzy save";
	char buffer[64];
	DWORD count = 0;
	HANDLE file;
	char host_path[1024];
	FILE *host;

	file = CreateFileA("t:\\Settings.dat", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	check(file != INVALID_HANDLE_VALUE && WriteFile(file, text, sizeof(text), &count, NULL) &&
		count == sizeof(text), "saves: write t:\\Settings.dat");
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	file = CreateFileA("T:\\SETTINGS.DAT", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	check(file != INVALID_HANDLE_VALUE && ReadFile(file, buffer, sizeof(buffer), &count, NULL) &&
		count == sizeof(text) && !memcmp(buffer, text, sizeof(text)), "saves: read it back in another case");
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	/* the file is on the host, below the save root */
	snprintf(host_path, sizeof(host_path), "%s/t/Settings.dat", getenv("OPENFRENZY_SAVE_ROOT"));
	host = fopen(host_path, "rb");
	check(host != NULL, "saves: t:\\ is <save root>/t on the host");
	if (host)
		fclose(host);
	check(CreateDirectoryA("u:\\SaveGame", NULL) || GetLastError() == ERROR_ALREADY_EXISTS,
		"saves: CreateDirectoryA(u:\\SaveGame)");
	check(GetFileAttributesA("U:\\savegame") == FILE_ATTRIBUTE_DIRECTORY, "saves: u:\\savegame is a directory");
	check(CopyFileA("d:\\Packages\\BootSeq.xbp", "u:\\SaveGame\\copy.bin", FALSE),
		"saves: CopyFileA from the disc");
	file = CreateFileA("u:\\savegame\\copy.bin", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	check(file != INVALID_HANDLE_VALUE && GetFileSize(file, NULL) == 4096, "saves: the copy has 4096 bytes");
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	{
		ULARGE_INTEGER free_bytes;

		check(GetDiskFreeSpaceExA("u:\\", &free_bytes, NULL, NULL) && free_bytes.QuadPart > 0,
			"saves: u:\\ has free space");
	}
}

static void test_utility_drive(void)
{
	HANDLE file;

	check(XMountUtilityDrive(FALSE), "utility: XMountUtilityDrive(FALSE)");
	CreateDirectoryA("z:\\cache", NULL);
	file = CreateFileA("z:\\cache\\file.bin", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	check(file != INVALID_HANDLE_VALUE, "utility: create z:\\cache\\file.bin");
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	check(XMountUtilityDrive(FALSE) && GetFileAttributesA("z:\\cache\\file.bin") != (DWORD)-1,
		"utility: mounting again keeps the cache");
	check(XMountUtilityDrive(TRUE) && GetFileAttributesA("z:\\cache\\file.bin") == (DWORD)-1 &&
		GetFileAttributesA("z:\\cache") == (DWORD)-1 && GetFileAttributesA("z:\\") == FILE_ATTRIBUTE_DIRECTORY,
		"utility: XMountUtilityDrive(TRUE) empties z:\\ and keeps it");
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	setvbuf(stdout, NULL, _IONBF, 0);
	check(vfs_disc() != NULL, "disc: the image opens");
	if (vfs_disc())
	{
		test_disc_read();
		test_disc_names();
		test_disc_read_only();
		test_disc_fopen();
		test_disc_find();
		crc_of_file("d:\\default.xbe");
		crc_of_file("d:\\Packages\\BugBash.xbp");
	}
	test_save_drives();
	test_utility_drive();
	if (vfs_disc())
		test_disc_read_ex();
	printf("vfs_test: %d failure(s)\n", failures);
	return failures;
}
