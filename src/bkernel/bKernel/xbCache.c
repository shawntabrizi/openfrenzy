/*
C:\Babel\Xbox\Src\bKernel\xbCache.cpp: the Z: drive file cache. Files named on a copy
request list are copied from the disc to the utility drive in 256 KB blocks through the
background loader (when it is idle); a cached file is named after the crc of its source
name and its group, "z:\XXXXXXXX_GGGGGGGG.___", and bkFindFile opens it instead of the
disc file. The cache is off when D: is writable (a development kit).

0x0004eed0 and 0x0004f5e0 name this file (assert paths); 0x0004ef40-0x0004f510 lie between
them. Their neighbours from 0x0004e980 (after xbKernel.cpp) to 0x0004f710, and the hex
parser 0x0004d9c0 that only this file uses, are placed here too.
*/

#include "bkernel_core.h"

#include <string.h>

/* port: 1 turns the Z: cache on as in the original (see bInitCache); 0 (default) keeps it off */
#ifndef OPENFRENZY_Z_CACHE
#define OPENFRENZY_Z_CACHE 0
#endif

/* 0x002d15b8: copy state: 0 idle, 1 reading a block, 2 writing it */
int DAT_002d15b8;
/* 0x002d15bc: a file is being copied */
int DAT_002d15bc;
/* 0x002d15c0: its source name, group and priority */
const char *DAT_002d15c0;
int DAT_002d15c4;
int DAT_002d15c8;
/* 0x002d15cc: source handle, 0x002d15d0: destination (z:\tempcopy.___) handle */
HANDLE DAT_002d15cc;
HANDLE DAT_002d15d0;
/* 0x002d15d4: bytes left to read, 0x002d15d8: position, 0x002d15dc: current block */
int DAT_002d15d4;
int DAT_002d15d8;
int DAT_002d15dc;
/* 0x002d15e0: the 256 KB copy buffer */
void *DAT_002d15e0;
/* 0x002d15e8: the cached files, 0x002d73a8: their number */
bkCacheEntry DAT_002d15e8[BK_CACHE_MAX_FILES];
int DAT_002d73a8;
/* 0x002d73ac: the cache is on */
int DAT_002d73ac;
/* 0x002d73b0, 0x002d73b4: queues of the "_FileCacheRead" / "_FileCacheWrite" events */
bkEventClient *DAT_002d73b0;
bkEventClient *DAT_002d73b4;
/* 0x002d73b8: the copy requests, 0x002da298: their number */
bkCacheRequest DAT_002d73b8[BK_CACHE_MAX_FILES];
int DAT_002da298;
/* 0x002da2a0: print the cache's progress (bkCacheDebugf) */
int DAT_002da2a0;
/* 0x002da2a4: bkCacheFileName's result */
char DAT_002da2a4[32];

/* 0x002853f8 (based at 0x00285338 + 4 * '0'): hex digit values of '0' to 'f' */
static const int bkHexDigitValues['f' - '0' + 1] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15,
};

/* the value of one hex digit. The original reads its table at any character; the port
gives 0 outside '0'-'f' (cache file names are hex digits). */
static uint bkHexDigit(char c)
{
	if (c < '0' || c > 'f')
		return 0;
	return (uint)bkHexDigitValues[c - '0'];
}

/* 0x0004d9c0
bkParseHex8: the value of eight hex digits. */
uint bkParseHex8(const char *text)
{
	uint value = 0;
	int index;

	for (index = 0; index < 8; index++)
		value = (value << 4) | bkHexDigit(text[index]);
	return value;
}

/* 0x0004e980
bkCacheAbortCopy: stops copying a file (waits for the block in flight). */
void bkCacheAbortCopy(void)
{
	if (DAT_002d15bc == 0)
		return;
	if (DAT_002d15b8 == 1)
	{
		while (bkGetEventMessage(DAT_002d73b0, NULL) == 0)
			Sleep(0);
		DAT_002d15b8 = 0;
	}
	else if (DAT_002d15b8 == 2)
	{
		while (bkGetEventMessage(DAT_002d73b4, NULL) == 0)
			Sleep(0);
		DAT_002d15b8 = 0;
	}
	CloseHandle(DAT_002d15cc);
	CloseHandle(DAT_002d15d0);
	DAT_002d15bc = 0;
}

/* 0x0004ea30
bkCacheDump: prints the cached files and the copy requests (when force or verbose). */
void bkCacheDump(int force)
{
	int index;

	if (force == 0 && DAT_002da2a0 == 0)
		return;
	bkPrintf("\n");
	bkPrintf("---------- File Cache Contents ----------\n");
	for (index = 0; index < DAT_002d73a8; index++)
	{
		bkPrintf("#%d : crc 0x%08x, group %d, flags 0x%08x\n", index, DAT_002d15e8[index].crc,
			DAT_002d15e8[index].group, DAT_002d15e8[index].flags);
	}
	bkPrintf("----------- Copy Request List -----------\n");
	for (index = 0; index < DAT_002da298; index++)
	{
		bkPrintf("#%d : src '%s', group %d, priority %d\n", index, DAT_002d73b8[index].source,
			DAT_002d73b8[index].group, DAT_002d73b8[index].priority);
	}
	bkPrintf("-----------------------------------------\n\n");
}

/* 0x0004eaf0 (static in the original: the time in EAX, the name in ECX)
bkCacheAddEntry: adds a cached file from its name "XXXXXXXX_GGGGGGGG.___". */
void bkCacheAddEntry(const FILETIME *time, const char *name)
{
	bkCacheEntry *entry;

	if (DAT_002d73a8 == BK_CACHE_MAX_FILES)
		return;
	entry = &DAT_002d15e8[DAT_002d73a8];
	entry->flags = 0;
	entry->crc = bkParseHex8(name);
	entry->group = bkParseHex8(name + 9);
	entry->time = *time;
	DAT_002d73a8++;
}

/* 0x0004eb50
bkCacheBuildFileList: finds the cached files on Z: (deleting ones that do not open). */
void bkCacheBuildFileList(void)
{
	WIN32_FIND_DATAA found;
	struct vfs_by_handle_file_information information;
	char path[128];
	HANDLE find;
	HANDLE file;
	const char *extension;

	find = FindFirstFileA("z:\\*.*", &found);
	if (find == INVALID_HANDLE_VALUE)
		return;
	bkPrintf("BuildFileList: Scanning Z drive for cached files...\n");
	do
	{
		bkCacheDebugf("  %s\n", found.cFileName);
		if (strcmp(found.cFileName, ".") == 0 || strcmp(found.cFileName, "..") == 0)
			continue;
		if (_stricmp(found.cFileName, "tempcopy.___") == 0 ||
			_stricmp(found.cFileName, "debuglog.txt") == 0)
			continue;
		extension = strrchr(found.cFileName, '.');
		if (extension == NULL || _stricmp(extension, ".___") != 0)
			continue;
		sprintf(path, "z:\\%s", found.cFileName);
		file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
		{
			DeleteFileA(path);
			continue;
		}
		GetFileInformationByHandle(file, &information);
		bkCacheAddEntry((const FILETIME *)information.ftCreationTime, found.cFileName);
		CloseHandle(file);
		bkCacheDebugf("    Added '%s' to cache as entry #%d, crc 0x%08x, group %d\n", found.cFileName,
			DAT_002d73a8, DAT_002d15e8[DAT_002d73a8 - 1].crc, DAT_002d15e8[DAT_002d73a8 - 1].group);
		if (DAT_002d73a8 == BK_CACHE_MAX_FILES)
			break;
	} while (FindNextFileA(find, &found));
	CloseHandle(find);
}

/* 0x0004ecf0
bkCacheFileName: "XXXXXXXX_GGGGGGGG.___" for a source name and group (a static buffer). */
char *bkCacheFileName(const char *source, int group)
{
	sprintf(DAT_002da2a4, "%08X_%08X.___", bkCRCStringNoCase(source), group);
	return DAT_002da2a4;
}

/* 0x0004ed20
bkShutdownCache: deletes the cache's events and frees the copy buffer. */
void bkShutdownCache(void)
{
	bkDeleteEvent("_FileCacheRead");
	bkDeleteEvent("_FileCacheWrite");
	if (DAT_002d15e0 != NULL)
	{
		bkHeapFree(DAT_002d15e0);
		DAT_002d15e0 = NULL;
	}
}

/* 0x0004ed60
bkCacheFlush: everything 0 deletes the cached files of the list (when the cache is on);
otherwise every file on Z: is deleted. */
void bkCacheFlush(int everything)
{
	WIN32_FIND_DATAA found;
	char path[64];
	HANDLE find;
	int index;

	if (everything == 0)
	{
		if (DAT_002d73ac == 0)
			return;
		for (index = 0; index < DAT_002d73a8; index++)
		{
			sprintf(path, "z:\\%08X_%08X.___", DAT_002d15e8[index].crc, DAT_002d15e8[index].group);
			DeleteFileA(path);
		}
		DAT_002d73a8 = 0;
		return;
	}
	bkCacheAbortCopy();
	find = FindFirstFileA("z:\\*.*", &found);
	if (find != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(found.cFileName, ".") != 0 && strcmp(found.cFileName, "..") != 0)
			{
				sprintf(path, "z:\\%s", found.cFileName);
				bkCacheDebugf("bkCacheFlush: Deleting '%s'...\n", path);
				DeleteFileA(path);
			}
		} while (FindNextFileA(find, &found));
		CloseHandle(find);
	}
	DAT_002d73a8 = 0;
}

/* 0x0004ee90
bkCacheFindFile: the index of a source name's cached file, or -1. */
int bkCacheFindFile(const char *source)
{
	uint crc = bkCRCStringNoCase(source);
	int index;

	for (index = 0; index < DAT_002d73a8; index++)
	{
		if (DAT_002d15e8[index].crc == crc)
			return index;
	}
	return -1;
}

/* 0x0004eed0  bKernel/xbCache.cpp
bkCacheEnable: turns the cache on (with a new copy buffer) or off. */
void bkCacheEnable(int enable)
{
#if !OPENFRENZY_Z_CACHE
	/* port: the game turns the cache on and off around its modes (PressStart,
	MiniGameSelect, core.c); keep it off (see bInitCache) */
	enable = 0;
#endif
	if (DAT_002d73ac == enable)
		return;
	DAT_002d73ac = enable;
	if (enable == 0)
	{
		bkCacheAbortCopy();
		bkHeapFree(DAT_002d15e0);
		DAT_002d15e0 = NULL;
		return;
	}
	DAT_002d15e0 = bkHeapAlloc(BK_CACHE_BLOCK, "C:\\Babel\\Xbox\\Src\\bKernel\\xbCache.cpp", 0x261, 0x2006);
	if (DAT_002d15e0 == NULL)
	{
		bkPrintf("bkCacheEnable: *** OUT OF MEMORY - cache has been disabled ***\n");
		DAT_002d73ac = 0;
	}
}

/* 0x0004ef40  bKernel/xbCache.cpp
bkCacheStartCopy: takes the copy request of highest priority and starts copying its file
to z:\tempcopy.___ (the size must be a multiple of 2 KB and Z: must have room). */
void bkCacheStartCopy(void)
{
	char path[64];
	ULARGE_INTEGER freeBytes;
	ULARGE_INTEGER totalBytes;
	ULARGE_INTEGER totalFreeBytes;
	int best = 0;
	int priority = DAT_002d73b8[0].priority;
	int index;

	for (index = 1; index < DAT_002da298; index++)
	{
		if (DAT_002d73b8[index].priority > priority)
		{
			priority = DAT_002d73b8[index].priority;
			best = index;
		}
	}
	if (bkCacheFindFile(DAT_002d73b8[best].source) != -1)
		return; /* already cached: the request stays on the list */
	DAT_002d15c0 = DAT_002d73b8[best].source;
	DAT_002d15c4 = DAT_002d73b8[best].group;
	DAT_002d15c8 = DAT_002d73b8[best].priority;
	if (best < DAT_002da298 - 1)
	{
		/* the original moves 0x18 bytes per request (requests are 0xc bytes) */
		memmove(&DAT_002d73b8[best], &DAT_002d73b8[best + 1], (DAT_002da298 - best - 1) * 0x18);
	}
	DAT_002da298--;
	sprintf(path, "d:\\%s", DAT_002d15c0);
	DAT_002d15cc = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL, NULL);
	if (DAT_002d15cc == INVALID_HANDLE_VALUE)
	{
		bkPrintf("FileCache: *** Could not open file '%s' for reading ***\n", DAT_002d15c0);
		return;
	}
	DAT_002d15d4 = (int)GetFileSize(DAT_002d15cc, NULL);
	if (DAT_002d15d4 % 0x800 != 0)
	{
		CloseHandle(DAT_002d15cc);
		bkPrintf("FileCache: *** File '%s' cannot be cached because it's size is not 2Kb aligned ***\n",
			DAT_002d15c0);
		return;
	}
	GetDiskFreeSpaceExA("z:\\", &freeBytes, &totalBytes, &totalFreeBytes);
	if ((int)freeBytes.LowPart < DAT_002d15d4 + 0x1f400)
	{
		CloseHandle(DAT_002d15cc);
		bkPrintf("FileCache: *** File '%s' cannot be cached because Z: drive is full ***\n",
			DAT_002d15c0);
		return;
	}
	DAT_002d15d0 = CreateFileA("z:\\tempcopy.___", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL, NULL);
	if (DAT_002d15d0 == INVALID_HANDLE_VALUE)
	{
		CloseHandle(DAT_002d15cc);
		bkPrintf("FileCache: *** Could not open file '%s' for reading ***\n", DAT_002d15c0);
		return;
	}
	SetFilePointer(DAT_002d15d0, DAT_002d15d4, NULL, FILE_BEGIN);
	SetEndOfFile(DAT_002d15d0);
	SetFilePointer(DAT_002d15d0, 0, NULL, FILE_BEGIN);
	DAT_002d15dc = DAT_002d15d4;
	if (DAT_002d15d4 > BK_CACHE_BLOCK)
		DAT_002d15dc = BK_CACHE_BLOCK;
	DAT_002d15d8 = 0;
	bQueueBackgroundLoad(0, DAT_002d15e0, DAT_002d15cc, DAT_002d15c0, 0, DAT_002d15dc, 0,
		"_FileCacheRead", 0);
	DAT_002d15b8 = 1;
	bkCacheDebugf("FileCache: Started copying file '%s' of %d Kb...\n", DAT_002d15c0, DAT_002d15d4);
	DAT_002d15d4 -= DAT_002d15dc;
	DAT_002d15bc = 1;
}

/* 0x0004f210  bKernel/xbCache.cpp
bkCacheUpdate: one step of the copy, called every frame: a block read goes out to be
written; a block written starts the next read, or finishes the file (renamed to its cache
name); when idle and the DATA channel is free, the next block or request starts. */
void bkCacheUpdate(void)
{
	char path[64];
	LARGE_INTEGER now;

	if (DAT_002d15b8 == 1)
	{
		if (bkGetEventMessage(DAT_002d73b0, NULL) != 0)
		{
			bkCacheDebugf("FileCache: Block of %d bytes has been loaded from +%d, writing to dest...\n",
				DAT_002d15dc, DAT_002d15d8);
			bQueueBackgroundWrite(0, DAT_002d15e0, DAT_002d15d0, DAT_002d15d8, DAT_002d15dc, 0,
				"_FileCacheWrite", 0);
			DAT_002d15b8 = 2;
		}
		return;
	}
	if (DAT_002d15b8 == 2)
	{
		if (bkGetEventMessage(DAT_002d73b4, NULL) == 0)
			return;
		bkCacheDebugf("FileCache: Block Written\n");
		if (DAT_002d15d4 != 0)
		{
			DAT_002d15d8 += DAT_002d15dc;
			DAT_002d15b8 = 0;
			return;
		}
		CloseHandle(DAT_002d15cc);
		CloseHandle(DAT_002d15d0);
		DAT_002d15bc = 0;
		bkPrintf("FileCache: Finished caching file '%s'\n", DAT_002d15c0);
		sprintf(path, "z:\\%s", bkCacheFileName(DAT_002d15c0, DAT_002d15c4));
		if (MoveFileA("z:\\tempcopy.___", path) != 0)
		{
			KeQuerySystemTime(&now);
			bkCacheAddEntry((const FILETIME *)&now, path + 3);
			bkCacheDump(0);
			DAT_002d15b8 = 0;
			return;
		}
		bkPrintf("CacheFile: *** WARNING : Could not rename cached file '%s' (%s) ***\n", path,
			DAT_002d15c0);
		DAT_002d15b8 = 0;
		return;
	}
	if (DAT_002d73ac == 0 || bkIsChannelBusy(0) != 0 || bkIsChannelIdle(0) == 0)
		return;
	if (DAT_002d15bc != 0)
	{
		DAT_002d15dc = DAT_002d15d4;
		if (DAT_002d15d4 > BK_CACHE_BLOCK)
			DAT_002d15dc = BK_CACHE_BLOCK;
		bQueueBackgroundLoad(0, DAT_002d15e0, DAT_002d15cc, DAT_002d15c0, DAT_002d15d8,
			DAT_002d15dc, 0, "_FileCacheRead", 0);
		bkCacheDebugf("FileCache: Reading block of %d bytes from '%s' @ +%d\n", DAT_002d15dc,
			DAT_002d15c0, DAT_002d15d8);
		DAT_002d15d4 -= DAT_002d15dc;
		DAT_002d15b8 = 1;
		return;
	}
	if (DAT_002da298 > 0)
		bkCacheStartCopy();
}

/* 0x0004f470  bKernel/xbCache.cpp
bCacheLoadingFileNotification: the background loader is about to read fileName from the
disc; if it is the file being copied, the copy stops and its request goes back to the
front of the list. */
void bkCacheLoadingFileNotification(const char *fileName)
{
	const char *name = fileName[1] == ':' ? fileName + 3 : fileName;

	if (DAT_002d15bc == 0 || _stricmp(name, DAT_002d15c0) != 0)
		return;
	bkPrintf("bCacheLoadingFileNotification: Aborted caching of '%s' due to background load\n",
		fileName);
	bkCacheAbortCopy();
	if (DAT_002da298 < BK_CACHE_MAX_FILES)
	{
		if (DAT_002da298 != 0)
			memmove(&DAT_002d73b8[1], &DAT_002d73b8[0], DAT_002da298 * sizeof(bkCacheRequest));
		DAT_002d73b8[0].source = DAT_002d15c0;
		DAT_002da298++;
		DAT_002d73b8[0].group = DAT_002d15c4;
		DAT_002d73b8[0].priority = DAT_002d15c8;
	}
}

/* 0x0004f510  bKernel/xbCache.cpp
bkCacheOpenFile: opens the cached copy of a file (when the cache is on and has it); its
path goes to path when not NULL. */
int bkCacheOpenFile(const char *name, FILE **file, char *path)
{
	char cached[64];
	FILE *opened;
	int index;

	if (DAT_002d73ac == 0)
		return 0;
	index = bkCacheFindFile(name);
	if (index == -1)
		return 0;
	sprintf(cached, "z:\\%08X_%08X.___", DAT_002d15e8[index].crc, DAT_002d15e8[index].group);
	opened = bkFopen(cached, "rb");
	if (opened == NULL)
		return 0;
	if (path != NULL)
		strcpy(path, cached);
	bkPrintf("Opened '%s' '%s' from cache\n", cached, name);
	*file = opened;
	return 1;
}

/* 0x0004f5e0  bKernel/xbCache.cpp
bInitCache: finds the cached files, turns the cache on unless D: is writable, and makes
its events and copy buffer. */
void bInitCache(void)
{
	ULARGE_INTEGER freeBytes;
	ULARGE_INTEGER totalBytes;
	ULARGE_INTEGER totalFreeBytes;
	FILE *test;

	/* the original clears 0x002d15e8-0x002da2a0 in one go (the entries up to the request
	count) and then 0x002d15b8-0x002d15e4 */
	memset(DAT_002d15e8, 0, sizeof(DAT_002d15e8));
	DAT_002d73a8 = 0;
	DAT_002d73ac = 0;
	DAT_002d73b0 = NULL;
	DAT_002d73b4 = NULL;
	memset(DAT_002d73b8, 0, sizeof(DAT_002d73b8));
	DAT_002da298 = 0;
	DAT_002d15b8 = 0;
	DAT_002d15bc = 0;
	DAT_002d15c0 = NULL;
	DAT_002d15c4 = 0;
	DAT_002d15c8 = 0;
	DAT_002d15cc = NULL;
	DAT_002d15d0 = NULL;
	DAT_002d15d4 = 0;
	DAT_002d15d8 = 0;
	DAT_002d15dc = 0;
	DAT_002d15e0 = NULL;
	bkCacheBuildFileList();
#if OPENFRENZY_Z_CACHE
	test = bkFopen("d:\\_T_E_S_T.___", "wb");
	if (test == NULL)
		DAT_002d73ac = 1;
	else
		fclose(test);
#else
	/* port: the cache stays off, as on a development kit. Disc reads on the host are fast,
	and the cache copied every package to $OPENFRENZY_SAVE_ROOT/z (about 300 MB per minute
	of play). Build with -DOPENFRENZY_Z_CACHE=1 for the original behaviour above. */
	(void)test;
#endif
	bkCreateEvent("_FileCacheRead");
	DAT_002d73b0 = bkAddEventQueue("_FileCacheRead", 4, 0);
	bkCreateEvent("_FileCacheWrite");
	DAT_002d73b4 = bkAddEventQueue("_FileCacheWrite", 4, 0);
	if (DAT_002d73ac == 0)
	{
		bkPrintf("bInitCache: Z Drive file cache is DISABLED\n");
	}
	else
	{
		DAT_002d15e0 = bkHeapAlloc(BK_CACHE_BLOCK, "C:\\Babel\\Xbox\\Src\\bKernel\\xbCache.cpp", 0x162,
			0x2006);
		if (DAT_002d15e0 == NULL)
		{
			bkPrintf("bInitCache: *** OUT OF MEMORY - cache has been disabled ***\n");
			DAT_002d73ac = 0;
		}
		bkPrintf("bInitCache: Z Drive file cache is ENABLED\n");
	}
	bkCacheDump(1);
	GetDiskFreeSpaceExA("z:\\", &freeBytes, &totalBytes, &totalFreeBytes);
	bkPrintf("bInitCache: %d files in cache list, %d Kb of disc space free\n", DAT_002d73a8,
		freeBytes.LowPart >> 10);
}

/* 0x0004f710
bkCacheFileListAdd: asks for source to be cached (with a group and a priority). */
int bkCacheFileListAdd(const char *source, int priority, int group)
{
	int count;

	if (DAT_002d73ac == 0 || DAT_002da298 == BK_CACHE_MAX_FILES)
		return 0;
	if (bkCacheFindFile(source) != -1)
	{
		bkCacheDebugf("bkCacheFileListAdd: File '%s' is already cached, doing nothing\n", source,
			group);
		return 1;
	}
	count = DAT_002da298;
	DAT_002d73b8[count].source = source;
	DAT_002d73b8[count].group = group;
	DAT_002d73b8[count].priority = priority;
	DAT_002da298++;
	bkCacheDebugf("bkCacheFileListAdd: Added file '%s' with group %d to copy request list\n", source,
		group);
	bkCacheDump(0);
	return 1;
}
