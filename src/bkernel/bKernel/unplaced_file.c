/*
bKernel file helpers: stdio wrappers, the name crc, and the search for a file on the root
folder and the search folders.

The original source file is not known (gap functions in C:\Babel\Xbox\Src\bKernel; most
likely file.cpp, which tags its neighbours 0x0004dec0-0x0004e250).
*/

#include "bkernel_core.h"

#include <string.h>

/* 0x002d0330: crc table (bkInitCRC) */
uint DAT_002d0330[256];
/* 0x002d0730: search folders tried after (or before) the root folder */
const char *DAT_002d0730[4];
/* 0x002d0740: the language (index of bkLanguageSuffixes) */
int DAT_002d0740;
/* 0x002d0744: number of search folders */
int DAT_002d0744;
/* 0x002d0748: 0 root folder first, 1 root folder last, else search folders only */
int DAT_002d0748;
/* 0x002d0858: appended to every file name (normally empty) */
char DAT_002d0858[8];

/* 0x0004c6b0
bkLoadFile: loads a file from a package (by the crc of its name) or, without a package,
from disc. See bkLoadPackageFile and bkLoadSingleFile. */
void *bkLoadFile(struct bkPackage *package, const char *name, void *buffer, uint *size,
	uint *tags, int extra)
{
	if (package == NULL)
		return bkLoadSingleFile(name, buffer, size, extra);
	return bkLoadPackageFile(package, bkCRCString(name), buffer, size, tags, extra);
}

/* 0x0004c710
bkOpenFile: opens a file for reading; 1 and *file on success. */
int bkOpenFile(const char *name, FILE **file)
{
	FILE *opened = bkFopen(name, "rb");

	if (opened != NULL)
	{
		*file = opened;
		return 1;
	}
	return 0;
}

/* 0x0004c740 */
void bkReadFile(FILE *file, void *buffer, uint size)
{
	fread(buffer, 1, size, file);
}

/* 0x0004c760
bkSeekFile: origin 0 start, 1 current, 2 end. */
void bkSeekFile(FILE *file, int offset, int origin)
{
	if (origin == 0)
		fseek(file, offset, SEEK_SET);
	else if (origin == 1)
		fseek(file, offset, SEEK_CUR);
	else if (origin == 2)
		fseek(file, offset, SEEK_END);
}

/* 0x0004c7a0 (a jump to the C library's fclose) */
int bkCloseFile(FILE *file)
{
	return fclose(file);
}

/* 0x0004c7b0
bkFileLength: the length of an open file (the position is kept). */
int bkFileLength(FILE *file)
{
	long position = ftell(file);
	long length;

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, position, SEEK_SET);
	return (int)length;
}

/* 0x0004db50
bkInitCRC: the table of the MSB-first CRC-32 (polynomial 0x04c11db7). */
void bkInitCRC(void)
{
	int index;
	int bit;
	uint crc;

	for (index = 0; index < 0x100; index++)
	{
		crc = (uint)index << 24;
		for (bit = 0; bit < 8; bit++)
			crc = (crc & 0x80000000u) ? (crc << 1) ^ 0x04c11db7u : crc << 1;
		DAT_002d0330[index] = crc;
	}
}

/* 0x0004dbf0
bkDelayLoop: an empty counting loop. */
void bkDelayLoop(int unused, int count)
{
	volatile int remaining = count;

	(void)unused;
	for (; remaining != 0; remaining--)
		;
}

/* 0x0004dc30
bkCRCString: the crc of a name (file names in packages, event names, ...). */
uint bkCRCString(const char *string)
{
	uint crc = 0;
	byte c;

	for (; (c = (byte)*string) != 0; string++)
		crc = (crc << 8) ^ DAT_002d0330[((crc >> 24) ^ c) & 0xff];
	return crc;
}

/* 0x0004dc70
bkCRCStringNoCase: bkCRCString with A-Z taken as a-z. */
uint bkCRCStringNoCase(const char *string)
{
	uint crc = 0;
	int c;

	for (; *string != '\0'; string++)
	{
		c = (signed char)*string;
		if (c >= 'A' && c <= 'Z')
			c += 0x20;
		crc = (crc << 8) ^ DAT_002d0330[((crc >> 24) ^ (uint)c) & 0xff];
	}
	return crc;
}

/* 0x0004dcc0 */
struct bkPackageEntry *bkGetPackageEntry(struct bkPackage *package, int index)
{
	return &package->entries[index];
}

/* 0x0004dce0 */
uint bkGetPackageId(struct bkPackage *package)
{
	return package->id;
}

/* 0x0004dcf0 */
void bkSetLanguage(int language)
{
	DAT_002d0740 = language;
}

/* 0x0004dd00
bkFindFile: opens a file for reading: from the Z: cache, by its own drive, from the root
folder, or from the search folders (DAT_002d0748 sets the order). On success the path
used goes to path (at most pathSize characters) and the result is 1. */
int bkFindFile(const char *name, FILE **file, char *path, int pathSize)
{
	char buffer[256];
	int result = 0;
	int index;

	if (bkCacheOpenFile(name, file, path) != 0)
		return 1;
	if (name[1] == ':')
	{
		sprintf(buffer, "%s%s", name, DAT_002d0858);
		result = bkOpenFile(buffer, file);
	}
	else if (name[0] == '\\')
	{
		sprintf(buffer, "%s%s%s", bkRootPath, name, DAT_002d0858);
		result = bkOpenFile(buffer, file);
	}
	else
	{
		if (DAT_002d0748 == 0)
		{
			sprintf(buffer, "%s%s%s", bkRootPath, name, DAT_002d0858);
			result = bkOpenFile(buffer, file);
			if (result == 1)
				goto found;
		}
		if (DAT_002d0744 != 0)
		{
			for (index = 0; index < DAT_002d0744; index++)
			{
				if (DAT_002d0730[index][1] == ':')
					sprintf(buffer, "%s\\%s%s", DAT_002d0730[index], name, DAT_002d0858);
				else
					sprintf(buffer, "%s%s\\%s%s", bkRootPath, DAT_002d0730[index], name,
						DAT_002d0858);
				result = bkOpenFile(buffer, file);
				if (result == 1)
					goto found;
			}
		}
		if (DAT_002d0748 == 1)
		{
			sprintf(buffer, "%s%s%s", bkRootPath, name, DAT_002d0858);
			result = bkOpenFile(buffer, file);
		}
	}
	if (result != 1)
		return result;
found:
	if (path != NULL)
	{
		*path = '\0';
		strncat(path, buffer, pathSize);
	}
	return 1;
}
