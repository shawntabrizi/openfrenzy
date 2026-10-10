/*
bKernel odds and ends between xbCache.cpp and package.cpp: a profiling timer, the
build-expiry check, the date and a printable-text helper.

The original source file is not known (gap functions in C:\Babel\Xbox\Src\bKernel).
*/

#include "bkernel_core.h"

/* a profiling timer: the average of count timed intervals */
typedef struct bkProfileTimer
{
	ulonglong average; /* 0x00 ticks */
	ulonglong total;   /* 0x08 */
	ulonglong start;   /* 0x10 */
	int samples;       /* 0x18 */
	int count;         /* 0x1c */
} bkProfileTimer;
OF_ASSERT_SIZE(bkProfileTimer, 0x20);

/* 0x002d08a0: bkInit's third argument (passed on to biInit) */
int DAT_002d08a0;

/* 0x0004f7a0
bkInit: starts the kernel, a heap of size bytes (in memory, or in new physical memory when
memory is NULL), the display, input, the file cache, actors and sound. On a failure what
was started is shut down again and the result is 0. */
int bkInit(void *memory, uint size, int inputFlags)
{
	uint freeMemory = bkGetFreePhysicalMemory();

	DAT_002d08a0 = inputFlags;
	FUN_001d5950();
	if (bkInitKernel() == 0)
		return 0;
	if (bInitHeap(memory, size) != 0)
	{
		bkGetCommandLine();
		if (FUN_00041d70() == 0)
		{
			bkFreeCommandLine();
			bkShutdownHeap();
			bkShutdownKernel();
			return 0;
		}
		FUN_00048880(inputFlags);
		bInitCache();
		if (FUN_00034730() != 0)
		{
			if (FUN_000571c0() != 0)
			{
				bkPrintf("bkInit: %d Kb of memory is free at startup\n", (int)freeMemory >> 10);
				return 1;
			}
			FUN_00034780();
		}
		bkShutdownCache();
		bkFreeCommandLine();
		bkShutdownHeap();
		biShutdownInput();
		FUN_00041e40();
	}
	bkShutdownKernel();
	return 0;
}

/* 0x0004f860
bkShutdown: the reverse of bkInit. */
void bkShutdown(void)
{
	FUN_000571f0();
	FUN_00034780();
	bkShutdownCache();
	biShutdownInput();
	FUN_00041e40();
	bkShutdownKernel();
	bkFreeCommandLine();
	bkShutdownHeap();
	FUN_001d5950();
}

/* 0x0004f890 */
void bkInitProfileTimer(struct bkProfileTimer *timer, int count)
{
	timer->samples = 0;
	timer->total = 0;
	timer->average = 0;
	timer->count = count;
}

/* 0x0004f8b0 */
void bkStartProfileTimer(struct bkProfileTimer *timer)
{
	timer->start = bkTimerRead();
}

/* 0x0004f8c0
bkStopProfileTimer: adds the interval since the start; 1 when count intervals are in and
the average was updated. */
int bkStopProfileTimer(struct bkProfileTimer *timer)
{
	ulonglong delta = bkTimerDelta(timer->start, bkTimerRead());

	timer->total += delta;
	timer->samples++;
	if (timer->samples == timer->count)
	{
		timer->average = (ulonglong)((longlong)timer->total / (longlong)timer->count);
		timer->total = 0;
		timer->samples = 0;
		return 1;
	}
	return 0;
}

/* 0x0004f930
bkCheckExpiry: a preview build carries an expiry date in its "sig" XBE section (xored
year, month and day, and a check word). When the section is there and the date has
passed, or the check fails, the screen goes white for four seconds and the machine
reboots to the dashboard. The retail XBE has no "sig" section, and the port loads no
sections, so this returns at once. */
void bkCheckExpiry(void)
{
	uint *signature = (uint *)XLoadSection("sig");
	byte time[6];
	uint limit;
	ulonglong start;

	if (signature == NULL || signature[0x140 / 4] == 0)
		return;
	if (signature[0x14c / 4] == (signature[0x144 / 4] ^ signature[0x148 / 4] ^ signature[0x140 / 4]))
	{
		bkGetTime(time);
		limit = (signature[0x140 / 4] ^ 0x937f256c) - 2000;
		if (time[5] < limit)
			return;
		if (time[5] == limit)
		{
			limit = signature[0x144 / 4] ^ 0x372cde35;
			if ((int)time[4] < (int)limit)
				return;
			if (time[4] == limit && time[3] < (signature[0x148 / 4] ^ 0xd3a749b9))
				return;
		}
	}
	XFreeSection("sig");
	FUN_00041ee0(0, 0xff, 0xff, 0xff);
	FUN_00041ee0(0, 0xff, 0xff, 0xff);
	start = bkTimerRead();
	while ((longlong)bkTimerDelta(start, bkTimerRead()) < bkTimerFrequency * 4)
		;
	XLaunchNewImageA(NULL, NULL);
}

/* 0x0004fa60
bkGetTime: the local time as six bytes: second, minute, hour, day, month, year % 100.
Always 1. */
int bkGetTime(byte *time)
{
	SYSTEMTIME now;

	GetLocalTime(&now);
	time[1] = (byte)now.wMinute;
	time[0] = (byte)now.wSecond;
	time[3] = (byte)now.wDay;
	time[2] = (byte)now.wHour;
	time[4] = (byte)now.wMonth;
	time[5] = (byte)(now.wYear % 100);
	return 1;
}

/* 0x0004fab0
bkMakePrintable: copies up to size - 1 bytes of data as text, control characters as '.'. */
void bkMakePrintable(const byte *data, int length, char *text, int size)
{
	int count;

	if (data == NULL)
	{
		*text = '\0';
		return;
	}
	count = size - 1;
	if (length < count)
		count = length;
	for (; count > 0; count--)
	{
		*text++ = *data < 0x20 ? '.' : (char)*data;
		data++;
	}
	*text = '\0';
}
