/*
C:\Babel\Xbox\Src\bKernel\xbDebug.cpp: the execution log (bkPrintf).

Only 0x0004b160 names this file (an assert path). The other functions here are its
neighbours and work on the same log records, so they are placed here too.
*/

#include "bkernel_core.h"

#include <string.h>

/* 0x00285158: the default log, "z:\debugLog.txt" after bkInitLog */
bkLogFile bkDefaultLog = { "", BK_LOG_DEBUGGER };
/* 0x0028525c: the log bkPrintf writes to */
bkLogFile *bkCurrentLog = &bkDefaultLog;
/* 0x00285260: month names, indexed from 1 (bkCurrentLog sits at index 0) */
static const char *const bkMonthNames[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};
/* 0x002cc4f0 (cleared by bkInitLog, not read in bKernel) */
int DAT_002cc4f0;
/* 0x002cc4f8: the line being printed */
char bkPrintBuffer[0x800];
/* 0x002cccf8: when not 0, every bkPrintf waits this many milliseconds (a debug aid) */
int bkPrintDelayMs;

/* 0x0004b160  bKernel/xbDebug.cpp
bkCreateLogFile: fills (or allocates) a log record for name; a name without a drive is
relative to the root folder. BK_LOG_TRUNCATE empties the file now. */
bkLogFile *bkCreateLogFile(bkLogFile *log, const char *name, uint flags)
{
	FILE *file;

	if (log == NULL)
	{
		log = (bkLogFile *)bkHeapAlloc(sizeof(bkLogFile), "C:\\Babel\\Xbox\\Src\\bKernel\\xbDebug.cpp",
			0x5d, 0x2006);
		if (log == NULL)
			return NULL;
		flags |= BK_LOG_ALLOCATED;
	}
	if (name == NULL || name[0] == '\0')
		log->path[0] = '\0';
	else if (name[1] == ':')
		strcpy(log->path, name);
	else
		sprintf(log->path, "%s%s", bkRootPath, name);
	log->flags = flags;
	if (log->path[0] != '\0' && (flags & BK_LOG_TRUNCATE) != 0)
	{
		file = fopen(log->path, "w");
		if (file != NULL)
			fclose(file);
	}
	return log;
}

/* 0x0004b210
bkSetLogFile: makes log the current log (NULL: the default log). */
void bkSetLogFile(bkLogFile *log)
{
	bkCurrentLog = log;
	if (log == NULL)
		bkCurrentLog = &bkDefaultLog;
}

/* the output half of bkPrintf and bkVPrintf: debugger, file, then the optional delay */
static void bkPrintOutput(void)
{
	FILE *file;
	ulonglong start;
	longlong wait;

	if (bkCurrentLog->flags & BK_LOG_DEBUGGER)
		OutputDebugStringA(bkPrintBuffer);
	if (bkCurrentLog->path[0] != '\0')
	{
		file = fopen(bkCurrentLog->path, "a");
		if (file != NULL)
		{
			fprintf(file, "%s", bkPrintBuffer);
			fclose(file);
		}
	}
	if (bkPrintDelayMs != 0)
	{
		start = bkTimerRead();
		wait = ((longlong)bkPrintDelayMs * bkTimerFrequency) / 1000;
		while ((longlong)bkTimerDelta(start, bkTimerRead()) < wait)
			;
	}
}

/* 0x0004b230
bkPrintf: prints to the current log (with a time stamp when the log asks for one). */
void bkPrintf(const char *format, ...)
{
	va_list arguments;
	SYSTEMTIME time;

	va_start(arguments, format);
	if ((bkCurrentLog->flags & BK_LOG_TIMESTAMP) == 0)
	{
		vsprintf(bkPrintBuffer, format, arguments);
	}
	else
	{
		GetLocalTime(&time);
		sprintf(bkPrintBuffer, "%02d/%02d/%02d %02d:%02d:%02d.%03d ", time.wDay, time.wMonth,
			time.wYear - 2000, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
		vsprintf(bkPrintBuffer + 22, format, arguments);
	}
	va_end(arguments);
	bkPrintOutput();
}

/* 0x0004b380
bkVPrintf: bkPrintf with a va_list (no time stamp). */
void bkVPrintf(const char *format, va_list arguments)
{
	vsprintf(bkPrintBuffer, format, arguments);
	bkPrintOutput();
}

/* 0x0004b450
bkInitLog: starts "z:\debugLog.txt" (also sent to the debugger) and prints the banner. */
void bkInitLog(void)
{
	char name[16];
	byte time[6];

	memcpy(name, "z:\\debugLog.txt", 16);
	bkCreateLogFile(&bkDefaultLog, name, BK_LOG_DEBUGGER | BK_LOG_TRUNCATE);
	bkCurrentLog = &bkDefaultLog;
	bkPrintf("\n-----------------------------------------\n");
	bkPrintf("Babel Execution Log, (c) 2001 Blitz Games\n");
	bkPrintf("  Babel 54.05, Thu Aug  9 09:46:20 2001\n");
	bkPrintf("-----------------------------------------\n");
	if (bkGetTime(time) == 0)
	{
		bkPrintf("Current time is unavailable\n");
	}
	else
	{
		/* time: second, minute, hour, day, month, year % 100. The original indexes the
		month names from the word before them (bkCurrentLog), so a month of 0 would print
		the log pointer as a string; months are 1 to 12 here. */
		bkPrintf("Current time is %02d:%02d:%02d, %d %s 20%02d\n", time[2], time[1], time[0],
			time[3], time[4] >= 1 && time[4] <= 12 ? bkMonthNames[time[4] - 1] : "",
			time[5]);
	}
	DAT_002cc4f0 = 0;
	_controlfp(0, 8);
}

/* 0x0004b540
bkDeleteLogFile: stops using a log (back to the default) and frees an allocated record. */
void bkDeleteLogFile(bkLogFile *log)
{
	if (log == bkCurrentLog)
		bkCurrentLog = &bkDefaultLog;
	if (log->flags & BK_LOG_ALLOCATED)
		bkHeapFree(log);
}
