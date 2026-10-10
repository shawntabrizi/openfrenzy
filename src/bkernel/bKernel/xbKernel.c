/*
C:\Babel\Xbox\Src\bKernel\xbKernel.cpp: kernel start-up and shutdown of the subsystems,
OS events polled by the main loop, locks, and the debugger command line.

0x0004e710 and 0x0004e800 name this file (assert paths). Their neighbours from 0x0004e480
(after file.cpp) to 0x0004e960 (before xbCache.cpp) are placed here too.
*/

#include "bkernel_core.h"

#include <string.h>

/* an OS event polled by bkPollOSEvents */
typedef struct bkOSEvent
{
	int (*callback)(int parameter); /* returns how far to advance in the table */
	int parameter;
	int extra;
	int unused;
} bkOSEvent;
OF_ASSERT_SIZE(bkOSEvent, 0x10);

/* 0x002d0750: launch data type (XGetLaunchInfo) */
DWORD DAT_002d0750;
/* 0x002d0758: root folder of relative file names */
char DAT_002d0758[0x100];
/* 0x002d0860: the OS events' handles */
HANDLE DAT_002d0860[16];
/* 0x002d08a8: launch data (0xc00 bytes) */
char DAT_002d08a8[0xc00];
/* 0x002d14a8: the OS events, 0x002d15a8: their number */
bkOSEvent DAT_002d14a8[16];
int DAT_002d15a8;
/* 0x002d15ac: command line argument count, 0x002d15b0: the arguments */
int DAT_002d15ac;
char **DAT_002d15b0;

/* 0x0004e480
bkInitKernel: the log, the timer, events, the crc table, resources, the root folder
"d:\" and background loading. */
int bkInitKernel(void)
{
	bkInitLog();
	bInitTimer();
	bkInitEvents();
	bkInitCRC();
	bkInitResources();
	memcpy(bkRootPath, "d:\\", 4);
	bkInitBackgroundLoad();
	return 1;
}

/* 0x0004e4b0
bkShutdownKernel: frees the filename tables, reports and deletes the resources left, and
deletes the events. */
void bkShutdownKernel(void)
{
	bkFreeFilenameTable(0);
	if (bkResourceList.next != &bkResourceList)
	{
		bkPrintf("\n*** RESOURCE LEAKS DETECTED :\n");
		bkListResources(0xffffffff);
		bkPrintf("\n");
	}
	else
	{
		bkPrintf("Resource list is clean\n");
	}
	bkDeleteAllResources();
	bkShutdownEvents();
	FUN_001d5950();
	FUN_001d5950();
}

/* 0x0004e520
bAddOSEvent: a manual-reset event polled by bkPollOSEvents, which calls
callback(parameter) when it is signalled. Returns the event handle. */
HANDLE bAddOSEvent(int (*callback)(int), int parameter, int extra)
{
	char name[32];
	HANDLE handle;

	if (DAT_002d15a8 == 16)
	{
		bkPrintf("bAddOSEvent: *** INTERNAL ERROR : maximum OS events exceeded ***\n");
		return NULL;
	}
	sprintf(name, "BabelOSEvent_%d", DAT_002d15a8);
	handle = CreateEventA(NULL, TRUE, FALSE, name);
	DAT_002d14a8[DAT_002d15a8].callback = callback;
	DAT_002d14a8[DAT_002d15a8].extra = extra;
	DAT_002d0860[DAT_002d15a8] = handle;
	DAT_002d14a8[DAT_002d15a8].parameter = parameter;
	DAT_002d15a8++;
	return handle;
}

/* 0x0004e5a0 */
void bkRemoveOSEvent(HANDLE handle)
{
	int index;

	for (index = 0; index < DAT_002d15a8; index++)
	{
		if (DAT_002d0860[index] == handle)
			break;
	}
	if (index == DAT_002d15a8)
		return;
	CloseHandle(DAT_002d0860[index]);
	if (index < DAT_002d15a8 - 1)
	{
		memmove(&DAT_002d14a8[index], &DAT_002d14a8[index + 1],
			(DAT_002d15a8 - index - 1) * sizeof(bkOSEvent));
		memmove(&DAT_002d0860[index], &DAT_002d0860[index + 1],
			(DAT_002d15a8 - index) * sizeof(HANDLE) - sizeof(HANDLE));
	}
	DAT_002d15a8--;
}

/* 0x0004e630
bkPollOSEvents: for each signalled OS event: reset it and call its callback. */
void bkPollOSEvents(void)
{
	int index = 0;

	while (index < DAT_002d15a8)
	{
		if (WaitForSingleObject(DAT_002d0860[index], 0) == 0)
		{
			ResetEvent(DAT_002d0860[index]);
			index += DAT_002d14a8[index].callback(DAT_002d14a8[index].parameter);
		}
		else
		{
			index++;
		}
	}
}

/* 0x0004e690
bkRun: calls function(argument). main() starts the game through it with core_init.

Port: on since integration round 1. The test guest_guest_runs sets OPENFRENZY_BOOT_ONLY=1
(src/runtime/of_boot.c), so it does not reach this endless game loop. */
void bkRun(void (*function)(int), int argument)
{
	function(argument);
}

/* 0x0004e6a0 */
HANDLE bkCreateLock(void)
{
	return CreateMutexA(NULL, FALSE, NULL);
}

/* 0x0004e6b0
bkLock: waits up to 10 seconds; 1 when locked. */
int bkLock(HANDLE lock)
{
	return WaitForSingleObject(lock, 10000) == 0;
}

/* 0x0004e6d0 */
int bkUnlock(HANDLE lock)
{
	return ReleaseMutex(lock) != 0;
}

/* 0x0004e6f0 */
int bkDestroyLock(HANDLE lock)
{
	return CloseHandle(lock) != 0;
}

/* 0x0004e710  bKernel/xbKernel.cpp
bkParseCommandLine: splits a command line at spaces and tabs outside double quotes (the
quotes stay). Counts the arguments; with argv, also copies them. */
int bkParseCommandLine(const char *line, char **argv)
{
	const char *start = line;
	const char *end;
	int count = 0;
	int outsideQuotes;
	uint length;
	char c = *line;

	for (;;)
	{
		if (c == '\0')
			return count;
		c = *start;
		if (c == ' ' || c == '\t')
		{
			while (c == ' ' || c == '\t')
			{
				c = start[1];
				start++;
				if (c == '\0')
					return count;
			}
		}
		else
		{
			outsideQuotes = 1;
			end = start;
			while (c != '\0' && ((c != ' ' && c != '\t') || !outsideQuotes))
			{
				if (c == '"')
					outsideQuotes ^= 1;
				c = end[1];
				end++;
			}
			if (argv != NULL)
			{
				length = (uint)(end - start);
				argv[count] = (char *)bkHeapAllocEx(length + 1,
					"C:\\Babel\\Xbox\\Src\\bKernel\\xbKernel.cpp", 0x131, 0x2001, "Command Line", 0);
				memcpy(argv[count], start, length);
				argv[count][length] = '\0';
			}
			count++;
			start = end;
		}
		c = *start;
	}
}

/* 0x0004e800  bKernel/xbKernel.cpp
bkGetCommandLine: when the title was started by the debugger with a command line, splits
it into DAT_002d15ac / DAT_002d15b0. 1 when there was one. */
int bkGetCommandLine(void)
{
	memset(DAT_002d08a8, 0, sizeof(DAT_002d08a8));
	DAT_002d0750 = 0xffffffff;
	if (XGetLaunchInfo(&DAT_002d0750, (PLAUNCH_DATA)DAT_002d08a8) == 0 && DAT_002d0750 != 0 &&
		DAT_002d0750 == 3)
	{
		DAT_002d15ac = bkParseCommandLine(DAT_002d08a8, NULL);
		DAT_002d15b0 = (char **)bkHeapAllocEx(DAT_002d15ac * 4,
			"C:\\Babel\\Xbox\\Src\\bKernel\\xbKernel.cpp", 0x166, 0x2001, "Command Line", 0);
		DAT_002d15ac = bkParseCommandLine(DAT_002d08a8, DAT_002d15b0);
		return 1;
	}
	return 0;
}

/* 0x0004e890 */
void bkFreeCommandLine(void)
{
	int index;

	for (index = 0; index < DAT_002d15ac; index++)
	{
		bkHeapFree(DAT_002d15b0[index]);
		DAT_002d15b0[index] = NULL;
	}
	bkHeapFree(DAT_002d15b0);
	DAT_002d15b0 = NULL;
}

/* 0x0004e8f0 */
void bkNothing(void)
{
}

/* 0x0004e900
bkFlushWriteCombine: a store fence, then rewrites one word in every 32 bytes of memory
(count in words), which flushes the write-combining buffers. */
void bkFlushWriteCombine(uint *memory, int count)
{
	volatile uint *words = memory;
	int index;

	__sync_synchronize();
	for (index = 0; index < count; index += 8)
	{
		words[index] ^= 0xffff;
		words[index] ^= 0xffff;
	}
}

/* 0x0004e960
bkCacheDebugf: bkPrintf when the file cache is verbose (DAT_002da2a0). */
void bkCacheDebugf(const char *format, ...)
{
	va_list arguments;

	if (DAT_002da2a0 != 0)
	{
		va_start(arguments, format);
		bkVPrintf(format, arguments);
		va_end(arguments);
	}
}
