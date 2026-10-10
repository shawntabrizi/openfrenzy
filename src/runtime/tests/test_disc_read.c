/*
TEST_DISC_READ.C

The game reads its data from D:\ with the XDK file API (bKernel file.cpp,
package.cpp: "%s.xbp", "Packages\\BootSeq"). This test reads real files from
the player's disc image (OPENFRENZY_ISO) through that same API, as decompiled
code will: CreateFileA, GetFileSize, ReadFile, SetFilePointer, CloseHandle.

- D:\default.xbe starts with "XBEH", and its size is the XBE header's
  SizeOfImage-independent file size (2834432 bytes for the USA retail disc,
  README.md);
- the disc is case-insensitive: d:\packages\bootseq.xbp is Packages\BootSeq.xbp;
- seeking then reading returns the bytes at that offset (same as a read from 0);
- a missing file fails with INVALID_HANDLE_VALUE and a "not found" error.

Without a disc image (OPENFRENZY_ISO unset) the test reports "skipped" and passes.
*/

#include "platform.h"
#include "of_test.h"

#include <string.h>

int main(void)
{
	const char *iso = getenv("OPENFRENZY_ISO");
	unsigned char header[16], again[8];
	DWORD read = 0, size, error;
	HANDLE file;

	setvbuf(stdout, NULL, _IONBF, 0);
	if (!iso || !*iso)
	{
		printf("test_disc_read: skipped (no OPENFRENZY_ISO)\n");
		return 0;
	}
	printf("test_disc_read: disc image %s\n", iso);

	file = CreateFileA("D:\\default.xbe", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	OF_CHECK(file != INVALID_HANDLE_VALUE, "CreateFileA D:\\default.xbe (error %lu)",
		file == INVALID_HANDLE_VALUE ? (unsigned long)GetLastError() : 0UL);
	if (file != INVALID_HANDLE_VALUE)
	{
		size = GetFileSize(file, NULL);
		OF_CHECK(size == 2834432, "GetFileSize is the retail XBE's (%lu)", (unsigned long)size);
		OF_CHECK(ReadFile(file, header, 16, &read, NULL) && read == 16, "ReadFile 16 bytes (%lu)", (unsigned long)read);
		OF_CHECK(!memcmp(header, "XBEH", 4), "the XBE magic is XBEH (%02x %02x %02x %02x)", header[0], header[1],
			header[2], header[3]);
		/* the base address field (offset 0x104) is 0x10000 on every XBE */
		OF_CHECK(SetFilePointer(file, 0x104, NULL, FILE_BEGIN) == 0x104, "SetFilePointer to 0x104");
		OF_CHECK(ReadFile(file, again, 4, &read, NULL) && read == 4 &&
			(again[0] | again[1] << 8 | again[2] << 16 | (unsigned long)again[3] << 24) == 0x10000,
			"the XBE base address is 0x10000");
		OF_CHECK(SetFilePointer(file, 0, NULL, FILE_BEGIN) == 0 && ReadFile(file, again, 8, &read, NULL) &&
			!memcmp(again, header, 8), "reading again from 0 gives the same bytes");
		OF_CHECK(SetFilePointer(file, 0, NULL, FILE_END) == size && ReadFile(file, again, 8, &read, NULL) && read == 0,
			"a read at the end returns 0 bytes");
		OF_CHECK(CloseHandle(file), "CloseHandle");
	}

	file = CreateFileA("d:\\packages\\bootseq.xbp", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	OF_CHECK(file != INVALID_HANDLE_VALUE, "case-insensitive d:\\packages\\bootseq.xbp (error %lu)",
		file == INVALID_HANDLE_VALUE ? (unsigned long)GetLastError() : 0UL);
	if (file != INVALID_HANDLE_VALUE)
	{
		size = GetFileSize(file, NULL);
		OF_CHECK(size > 0 && size % 2048 == 0, "BootSeq.xbp has a 2 KB-multiple size (xbCache.cpp needs it) (%lu)",
			(unsigned long)size);
		OF_CHECK(ReadFile(file, header, 8, &read, NULL) && read == 8, "ReadFile of the package header");
		CloseHandle(file);
	}

	SetLastError(0);
	file = CreateFileA("D:\\Packages\\NoSuchPackage.xbp", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	error = GetLastError();
	OF_CHECK(file == INVALID_HANDLE_VALUE && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND),
		"a missing file fails with not found (error %lu)", (unsigned long)error);

	return of_test_result("test_disc_read");
}
