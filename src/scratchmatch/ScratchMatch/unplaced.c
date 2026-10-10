/*
ScratchMatch/unplaced.c  (module scratchmatch)

Gap functions of ScratchMatch without a known file: the 15 timers of the game (performance
counter based, "elapsed seconds since reset"; they can be paused as a whole) and the frame
time. They sit between ScratchMatch_GenActors.cpp and CScratchmatch_Render.cpp.

A timer is 0x10 bytes from 0x003100cc: the elapsed seconds of the last query (float), then
the counter value of the last reset (64 bits).
*/

#include "scratchmatch.h"

/* 0x001bd2e0  (gap): resets all timers and the frame clock */
void SMTimer_InitAll(void)
{
	int i;

	for (i = 0; i < SM_NUM_TIMERS; i++) {
		sm_TimerElapsed(i) = 0.0f;
		QueryPerformanceCounter(sm_TimerStart(i));
	}
	QueryPerformanceFrequency(sm_ClockFrequency);
	QueryPerformanceCounter(sm_ClockNow);
	sm_ClockLast->QuadPart = sm_ClockNow->QuadPart;
}

/* 0x001bd330  (gap): the seconds since timer `timer` was reset (also stored as a float; the
x87 result was the unrounded quotient) */
double SMTimer_Elapsed(int timer)
{
	LARGE_INTEGER now;
	double seconds;

	QueryPerformanceCounter(&now);
	seconds = (double)(now.QuadPart - sm_TimerStart(timer)->QuadPart) / (double)sm_ClockFrequency->QuadPart;
	sm_TimerElapsed(timer) = (float)seconds;
	return seconds;
}

/* 0x001bd380  (gap): resets timer `timer` */
void SMTimer_Reset(int timer)
{
	sm_TimerElapsed(timer) = 0.0f;
	QueryPerformanceCounter(sm_TimerStart(timer));
}

/* 0x001bd3a0  (gap): the time since the last frame */
void SMTimer_UpdateFrame(void)
{
	LONGLONG delta;

	QueryPerformanceCounter(sm_ClockNow);
	delta = sm_ClockNow->QuadPart - sm_ClockLast->QuadPart;
	sm_ClockLast->QuadPart = sm_ClockNow->QuadPart;
	sm_FrameTime = (double)delta / (double)sm_ClockFrequency->QuadPart;
}

/* 0x001bd400  (gap): the seconds the last frame took (a double, loaded with fld qword) */
double SMTimer_FrameTime(void)
{
	return sm_FrameTime;
}

/* 0x001bd410  (gap): remembers when the game was paused */
void SMTimer_Pause(void)
{
	QueryPerformanceCounter(sm_PauseStart);
}

/* 0x001bd420  (gap): moves every timer's start by the length of the pause */
void SMTimer_Resume(void)
{
	LARGE_INTEGER now;
	LONGLONG paused;
	int i;

	QueryPerformanceCounter(&now);
	paused = now.QuadPart - sm_PauseStart->QuadPart;
	sm_PausedTime->QuadPart = paused;
	for (i = 0; i < SM_NUM_TIMERS; i++)
		sm_TimerStart(i)->QuadPart += paused;
}
