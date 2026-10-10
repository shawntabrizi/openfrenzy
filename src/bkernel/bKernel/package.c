/*
C:\Babel\Xbox\Src\bKernel\package.cpp: packages (.xbp).

A package file starts with a 0x140-byte header (bkPackage in bkernel_core.h): its id, the
block size its offsets count in, the number of files, the block offsets of the index
(0x20 bytes per file, sorted by name crc), of the tag table and of the filename table,
the format build (0x36) and its name. A package is either opened (index read into
memory, files read on demand from the open file) or loaded whole into memory (the index
and tag pointers are patched to point into it; bit 31 of the id is set).

0x0004fb00 names this file (an assert path). 0x0004fd50-0x0004fef0 follow it and work on
the same headers; the jump thunks 0x0004db20-0x0004db40 lead to them. All are placed here.
*/

#include "bkernel_core.h"

#include <string.h>

/* 0x0004fb00  bKernel/package.cpp
bkOpenPackage: opens name.xbp (found like bkFindFile) and reads its header and index. */
struct bkPackage *bkOpenPackage(const char *name)
{
	char path[0x100];
	FILE *file;
	bkPackage *package;
	int needed;
	int numFiles;

	sprintf(path, "%s%s", name, ".xbp");
	package = (bkPackage *)bkHeapAlloc(sizeof(bkPackage), "C:\\Babel\\Xbox\\Src\\bKernel\\package.cpp",
		0x21, 0x2006);
	if (package == NULL)
	{
		bkPrintf("bkOpenPackage: Out of memory '%s' (%d bytes wanted)\n", name, 0x140);
		needed = 0x140 - bkHeapFreeSpace(NULL);
		bkPrintf("bkOpenPackage: (only %d bytes available: %d more required) ***\n",
			bkHeapFreeSpace(NULL), needed);
		return NULL;
	}
	if (bkFindFile(path, &file, NULL, 0) == 0)
	{
		bkPrintf("bkOpenPackage: Could not open file '%s'\n", path);
		bkHeapFree(package);
		return NULL;
	}
	bkReadFile(file, package, sizeof(bkPackage));
	strcpy(package->name, name);
	package->file = file;
	if (package->numFiles == 0)
	{
		bkCloseFile(file);
		bkPrintf("bkOpenPackage: Package is empty! '%s'\n", path);
		bkHeapFree(package);
		return NULL;
	}
	if (package->build != BK_PACKAGE_BUILD)
	{
		bkPrintf("\nbkOpenPackage: *** WARNING : package '%s' from build %d used with build %d (package should be rebuilt) ***\n\n",
			path, package->build, BK_PACKAGE_BUILD);
	}
	package->id &= ~BK_PACKAGE_IN_MEMORY;
	bkSeekFile(package->file, package->blockSize * (uint)package->entries, 0);
	package->entries = (bkPackageEntry *)bkHeapAlloc(package->numFiles << 5,
		"C:\\Babel\\Xbox\\Src\\bKernel\\package.cpp", 0x45, 0x2006);
	if (package->entries == NULL)
	{
		numFiles = package->numFiles;
		bkCloseFile(package->file);
		bkHeapFree(package);
		/* the original reads the file count from address 0xc here (from a NULL package);
		the port uses the count read before the package was freed */
		bkPrintf("bkOpenPackage: Out of memory for index '%s' (wanted %d bytes)\n", name, numFiles << 5);
		needed = (numFiles << 5) - bkHeapFreeSpace(NULL);
		bkPrintf("bkOpenPackage: (only %d bytes available: %d more required) ***\n",
			bkHeapFreeSpace(NULL), needed);
		return NULL;
	}
	bkReadFile(package->file, package->entries, package->numFiles << 5);
	if (package->numTags != 0)
	{
		bkSeekFile(package->file, (uint)package->tags * package->blockSize, 0);
		package->tags = (uint *)bkHeapAlloc(package->numTags << 2,
			"C:\\Babel\\Xbox\\Src\\bKernel\\package.cpp", 0x53, 0x2006);
		bkReadFile(package->file, package->tags, package->numTags << 2);
	}
	bkLoadFilenameTable(package, path);
	bkPrintf("Opened package '%s' OK (%d files)\n", name, package->numFiles);
	return package;
}

/* 0x0004fd50
bkLoadPackage: loads name.xbp whole (from disc, or from inside parent) into buffer, or
into new memory when buffer is NULL. */
struct bkPackage *bkLoadPackage(struct bkPackage *parent, const char *name, void *buffer)
{
	char path[0x100];
	bkPackage *package;

	sprintf(path, "%s%s", name, ".xbp");
	package = (bkPackage *)bkLoadFile(parent, path, buffer, NULL, NULL, 0);
	if (package == NULL)
	{
		bkPrintf("bkLoadPackage: Could not load file '%s'\n", path);
		return NULL;
	}
	strcpy(package->name, name);
	if (package->numFiles == 0)
	{
		bkHeapFree(package);
		bkPrintf("bkLoadPackage: Package is empty! '%s'\n", name);
		return NULL;
	}
	if (package->build != BK_PACKAGE_BUILD)
	{
		bkPrintf("\nbkLoadPackage: *** WARNING : package '%s' from build %d used with build %d (package should be rebuilt) ***\n\n",
			path, package->build, BK_PACKAGE_BUILD);
	}
	package->id |= BK_PACKAGE_IN_MEMORY;
	package->entries = (bkPackageEntry *)((uint)package->entries * package->blockSize + (char *)package);
	if (package->numTags != 0)
		package->tags = (uint *)((uint)package->tags * package->blockSize + (char *)package);
	package->file = (FILE *)package;
	if (buffer != NULL)
		package->flags |= BK_PACKAGE_STATIC;
	bkLoadFilenameTable(package, path);
	bkPrintf("Loaded package '%s' OK (%d files)\n", name, package->numFiles);
	return package;
}

/* 0x0004fe80
bkClosePackage: releases a package: its filename table; for a loaded package, its
resources; for an opened one, the file and the index; then the memory unless it was the
caller's. */
void bkClosePackage(struct bkPackage *package)
{
	if (package == NULL)
		return;
	bkFreeFilenameTable(package->id);
	if ((int)package->id < 0)
	{
		bkDeletePackageResources(package->id, 0xffffffff);
	}
	else
	{
		bkCloseFile(package->file);
		bkHeapFree(package->entries);
		package->entries = NULL;
		if (package->tags != NULL)
		{
			bkHeapFree(package->tags);
			package->tags = NULL;
		}
	}
	if ((package->flags & BK_PACKAGE_STATIC) == 0)
		bkHeapFree(package);
}

/* 0x0004fef0
bkGetPackageSize: the size of name.xbp on disc (flags as bkGetFileSize). */
int bkGetPackageSize(const char *name, uint flags)
{
	char path[0x100];

	sprintf(path, "%s%s", name, ".xbp");
	return bkGetFileSize(path, flags);
}

/* 0x0004db20 (a jump to bkOpenPackage) */
struct bkPackage *bkOpenPackageThunk(const char *name)
{
	return bkOpenPackage(name);
}

/* 0x0004db30 (a jump to bkLoadPackage) */
struct bkPackage *bkLoadPackageThunk(struct bkPackage *parent, const char *name, void *buffer)
{
	return bkLoadPackage(parent, name, buffer);
}

/* 0x0004db40 (a jump to bkClosePackage) */
void bkClosePackageThunk(struct bkPackage *package)
{
	bkClosePackage(package);
}
