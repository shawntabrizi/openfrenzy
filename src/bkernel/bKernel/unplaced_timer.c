/*
bKernel timer: 64-bit timestamps, their frequency and differences, and a sleep.

The original source file is not known (gap functions in C:\Babel\Xbox\Src\bKernel).

Port note: the Xbox read the CPU's time stamp counter (rdtsc). The port reads
QueryPerformanceCounter instead, scaled to the Xbox rate (see bkTimerRead); bInitTimer
still measures the counter against the performance counter over half a second, so
bkTimerFrequency stays the measured rate.
*/

#include "bkernel_core.h"

/* 0x002cd7a0 (initialised, so that it is not a common symbol and can carry the alias of
its high word, 0x002cd7a4, which Ghidra-shaped code reads as a separate global) */
longlong bkTimerFrequency = 0;
BK_ALIAS(DAT_002cd7a4, DAT_002cd7a0, 4);

/* 0x0004c510
bInitTimer: measures the timestamp frequency over half a second. */
void bInitTimer(void)
{
	LARGE_INTEGER frequency;
	LARGE_INTEGER start;
	LARGE_INTEGER now;
	ulonglong stampStart;
	ulonglong stampEnd;

	QueryPerformanceFrequency(&frequency);
	stampStart = bkTimerRead();
	QueryPerformanceCounter(&start);
	do
	{
		QueryPerformanceCounter(&now);
	} while (now.QuadPart - start.QuadPart < frequency.QuadPart / 2);
	stampEnd = bkTimerRead();
	bkTimerFrequency = (longlong)(stampEnd - stampStart) * 2;
	bkPrintf("bInitTimer: Using *timestamp* timer (~%f Mhz)\n", (double)bkTimerFrequency * 1e-06);
}

/* 0x0004c5f0
bkTimerRead: the current timestamp.

Port: the original returns rdtsc, which runs at the Xbox CPU clock (733 MHz). The port's
performance counter runs at 1 MHz (src/xkernel/xbox_kernel.c), so it is scaled to the
Xbox rate here. The rate matters: bkTimerDelta adds 0xffffffff ticks to a negative
interval. At 733 MHz that is 5.9 s; at 1 MHz it was 71 minutes. Hopper's clock
(0x0014c4a0) takes such an interval while the game is paused before the round starts
(its start time is one frame newer than its "now"), and at 1 MHz the round ended at once
as a 4-way draw with the clock at 1:00. */
#define BK_TIMER_XBOX_RATE 733333333ull

ulonglong bkTimerRead(void)
{
	LARGE_INTEGER count;
	LARGE_INTEGER frequency;
	ulonglong ticks;
	ulonglong rate;

	QueryPerformanceCounter(&count);
	QueryPerformanceFrequency(&frequency);
	ticks = (ulonglong)count.QuadPart;
	rate = (ulonglong)frequency.QuadPart;
	return ticks / rate * BK_TIMER_XBOX_RATE + ticks % rate * BK_TIMER_XBOX_RATE / rate;
}

/* 0x0004c620
bkTimerRate: how many times per second an interval of ticks fits, as a float (0 for an
empty interval). */
float bkTimerRate(longlong ticks)
{
	if (ticks != 0)
		return (float)((bkTimerFrequency * 100) / ticks) * 0.01f;
	return 0.0f;
}

/* 0x0004c670
bkTimerDelta: end - start; when end is before start (a wrapped low word) the result gets
0xffffffff added, as in the original. */
ulonglong bkTimerDelta(ulonglong start, ulonglong end)
{
	if ((longlong)end < (longlong)start)
		return end - start + 0xffffffffull;
	return end - start;
}

/* 0x0004c6a0 */
void bkSleep(uint milliseconds)
{
	Sleep(milliseconds);
}
