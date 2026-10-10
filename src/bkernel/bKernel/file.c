/*
C:\Babel\Xbox\Src\bKernel\file.cpp: package indexes, filename tables, single files.

0x0004dec0 and 0x0004e250 name this file (assert paths); 0x0004e010-0x0004e1f0 lie between
them. 0x0004e2f0 and 0x0004e390 follow and use the same helpers, so they are placed here
too.
*/

#include "bkernel_core.h"

#include <string.h>

/* the link fields every bkFilenameTable starts with; the list sentinel has only these */
typedef struct bkFilenameLink
{
	uint id;
	struct bkFilenameTable *prev;
	struct bkFilenameTable *next;
} bkFilenameLink;

/* 0x002854d8: the list of loaded filename tables */
bkFilenameLink DAT_002854d8 = {
	0, (bkFilenameTable *)&DAT_002854d8, (bkFilenameTable *)&DAT_002854d8,
};
#define bkFilenameTables ((bkFilenameTable *)&DAT_002854d8)

/* the names that follow the { crc, nameOffset } pairs */
static char *bkFilenameTableNames(bkFilenameTable *table)
{
	return (char *)&table->files[table->numFiles * 2];
}

/* 0x0004dec0  bKernel/file.cpp
bkLoadFilenameTable: keeps a copy of a package's file names (when it has them), shared by
every opener of the same package. 1 when a new table was loaded. */
int bkLoadFilenameTable(struct bkPackage *package, const char *packageName)
{
	bkFilenameTable *table;
	int index;

	if (package->namesOffset == 0)
		return 0;
	for (table = DAT_002854d8.next; table != bkFilenameTables; table = table->next)
	{
		if ((table->id & 0x7fffffff) == (package->id & 0x7fffffff))
		{
			table->refCount++;
			return 0;
		}
	}
	table = (bkFilenameTable *)bkHeapAllocEx(package->namesSize + 0x114 + package->numFiles * 8,
		"C:\\Babel\\Xbox\\Src\\bKernel\\file.cpp", 0x1a3, 0x2001, "Filename Table", 0);
	if (table == NULL)
	{
		bkPrintf("bkLoadFilenameTable: Out of memory!\n");
		return 0;
	}
	table->id = package->id;
	table->numFiles = package->numFiles;
	table->prev = DAT_002854d8.prev;
	table->next = bkFilenameTables;
	table->refCount = 1;
	DAT_002854d8.prev->next = table;
	DAT_002854d8.prev = table;
	strcpy(table->packageName, packageName);
	for (index = 0; index < package->numFiles; index++)
	{
		table->files[index * 2] = package->entries[index].crc;
		table->files[index * 2 + 1] = package->entries[index].nameOffset;
	}
	if ((int)package->id < 0)
	{
		memcpy(bkFilenameTableNames(table),
			(char *)package + package->blockSize * package->namesOffset, package->namesSize);
		return 1;
	}
	bkSeekFile(package->file, package->blockSize * package->namesOffset, 0);
	bkReadFile(package->file, bkFilenameTableNames(table), package->namesSize);
	return 1;
}

/* 0x0004e010  bKernel/file.cpp
bkFreeFilenameTable: drops one reference to a package's table (id 0: frees them all).
Returns the references left, or -1 when there is no table. */
int bkFreeFilenameTable(uint packageId)
{
	bkFilenameTable *table;
	bkFilenameTable *next;

	if ((packageId & 0x7fffffff) == 0)
	{
		for (table = DAT_002854d8.next; table != bkFilenameTables; table = next)
		{
			next = table->next;
			bkHeapFree(table);
		}
		DAT_002854d8.prev = bkFilenameTables;
		DAT_002854d8.next = bkFilenameTables;
		return 0;
	}
	for (table = DAT_002854d8.next; table != bkFilenameTables; table = table->next)
	{
		if ((table->id & 0x7fffffff) == (packageId & 0x7fffffff))
		{
			table->refCount--;
			if (table->refCount != 0)
				return table->refCount;
			table->prev->next = table->next;
			table->next->prev = table->prev;
			bkHeapFree(table);
			return 0;
		}
	}
	return -1;
}

/* 0x0004e0c0  bKernel/file.cpp
bkGetFilename: the name of a file (by crc) in a package (id 0: any package); crc 0 gives
the package's name. NULL when unknown. */
const char *bkGetFilename(uint packageId, uint crc)
{
	bkFilenameTable *table;
	uint index;

	for (table = DAT_002854d8.next; table != bkFilenameTables; table = table->next)
	{
		if ((packageId & 0x7fffffff) == 0 || (packageId & 0x7fffffff) == (table->id & 0x7fffffff))
		{
			if (crc == 0)
				return table->packageName;
			for (index = 0; index < table->numFiles; index++)
			{
				if (table->files[index * 2] == crc)
					return bkFilenameTableNames(table) + table->files[index * 2 + 1];
			}
		}
	}
	return NULL;
}

/* 0x0004e150  bKernel/file.cpp
bkFindPackageEntry: binary search of a package's index for a crc. */
struct bkPackageEntry *bkFindPackageEntry(struct bkPackage *package, uint crc)
{
	bkPackageEntry *entries = package->entries;
	int low = 0;
	int high = package->numFiles - 1;
	int middle = high >> 1;
	uint found;

	for (;;)
	{
		found = entries[middle].crc;
		if (found == crc)
			return &entries[middle];
		if (found < crc)
			low = middle + 1;
		else
			high = middle - 1;
		if (high < low)
			break;
		middle = (high + low) >> 1;
	}
	bkPrintf("bkLoadFile: Could not find 0x%08x in package\n", crc);
	return NULL;
}

/* 0x0004e1c0  bKernel/file.cpp */
struct bkPackageEntry *bkFindPackageEntryByName(struct bkPackage *package, const char *name)
{
	bkPackageEntry *entry = bkFindPackageEntry(package, bkCRCString(name));

	if (entry == NULL)
	{
		bkPrintf("Could not find '%s' in package\n", name);
		return NULL;
	}
	return entry;
}

/* 0x0004e1f0  bKernel/file.cpp
bkGetFileSize: the length of a file on disc; flags bit 0 searches like bkFindFile. 0 when
the file is not there. */
int bkGetFileSize(const char *name, uint flags)
{
	FILE *file;
	int length;

	if ((flags & 1) == 0)
	{
		if (bkOpenFile(name, &file) == 0)
			return 0;
	}
	else
	{
		if (bkFindFile(name, &file, NULL, 0) == 0)
			return 0;
	}
	length = bkFileLength(file);
	bkCloseFile(file);
	return length;
}

/* 0x0004e250  bKernel/file.cpp (static in the original: size in EAX, buffer in ECX)
bkEnsureAlloc: allocates a 128 byte aligned buffer for a file, or checks that the
caller's buffer is big enough and aligned. */
void *bkEnsureAlloc(int size, void *buffer)
{
	const char *context;
	int largest;
	int available;

	if (buffer == NULL)
	{
		context = bkGetHeapContext();
		if (context == BK_HEAP_CONTEXT_DEFAULT)
			context = "Package";
		buffer = bkHeapAllocEx(size, "C:\\Babel\\Xbox\\Src\\bKernel\\file.cpp", 0x2b7, 0x2006,
			context, 0x80);
		if (buffer == NULL)
		{
			bkPrintf("EnsureAlloc: *** Out of memory on Babel heap for file (need %d bytes) ***\n",
				size);
			available = bkHeapFreeSpace(&largest);
			bkPrintf("EnsureAlloc: (only %d bytes available: %d more required, max %d bytes (%d short)) ***\n",
				available, size - available, largest, size - largest);
		}
		return buffer;
	}
	if (bkHeapBlockSize(buffer) < size)
		return NULL;
	if (((uint)buffer & 0x7f) == 0)
		return buffer;
	bkPrintf("bEnsureAlloc: memory for packages must be 128 byte aligned\n");
	DbgBreakPoint();
	return NULL;
}

/* 0x0004e2f0
bkLoadSingleFile: loads a whole file (found like bkFindFile) into buffer, or into a new
buffer with room for extra more bytes. */
void *bkLoadSingleFile(const char *name, void *buffer, uint *size, int extra)
{
	FILE *file;
	int length;

	if (bkFindFile(name, &file, NULL, 0) == 0)
	{
		bkPrintf("LoadSingleFile: *** Could not load file '%s' ***\n", name);
		return NULL;
	}
	length = bkFileLength(file);
	buffer = bkEnsureAlloc(length + extra, buffer);
	if (buffer == NULL)
		return NULL;
	bkReadFile(file, buffer, length);
	bkCloseFile(file);
	if (size != NULL)
		*size = length;
	bkPrintf("LoadSingleFile: Loaded file '%s' of %d bytes OK\n", name, length);
	return buffer;
}

/* 0x0004e390
bkLoadPackageFile: a file of a package by crc. *size gets its size, tags[0..1] its tag
list and tag count. From a package in memory, with no buffer, the result points into the
package; otherwise the file is copied or read into buffer (or a new one with room for
extra more bytes). */
void *bkLoadPackageFile(struct bkPackage *package, uint crc, void *buffer, uint *size,
	uint *tags, int extra)
{
	bkPackageEntry *entry = bkFindPackageEntry(package, crc);
	char *data;

	if (entry == NULL)
		return NULL;
	if (size != NULL)
		*size = entry->size;
	if (tags != NULL)
	{
		tags[0] = (uint)&package->tags[entry->tagOffset >> 2];
		tags[1] = entry->tagCount;
	}
	if ((int)package->id < 0)
	{
		data = (char *)package->file + package->blockSize * entry->offset;
		if (buffer == NULL)
			return data;
		if (bkEnsureAlloc(entry->size + extra, buffer) == NULL)
			return NULL;
		memcpy(buffer, data, entry->size);
		return buffer;
	}
	buffer = bkEnsureAlloc(entry->size + extra, buffer);
	if (buffer == NULL)
		return NULL;
	bkSeekFile(package->file, package->blockSize * entry->offset, 0);
	bkReadFile(package->file, buffer, entry->size);
	return buffer;
}
