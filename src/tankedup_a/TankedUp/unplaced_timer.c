/*
TankedUp/unplaced_timer.c  (gap functions of C:\BlitzParty\Game\Src\TankedUp\, file unknown)

The timer (struct TuTimer, tankedup_a.h): the timer system (CTank_Game +0xf78,
"timerSystem"), the camera's shot timer (camera +0x20) and other timers of tankedup_b.
It counts bKernel timer ticks from the start, keeps the length of the last frame (in
1/60 s units) and the time left of a length. 0x001c36e0 (SetLength) is a folded function
that other games' timers call too.
*/

#include "tankedup_a.h"

/* 0x001c3570  TankedUp (gap)
Resets the timer: stopped, not paused, no length, fraction 1. Returns 1. */
int CTankTimer_Reset(struct TuTimer *self)
{
	self->rate = ((unsigned long long)DAT_002cd7a4 << 32) | DAT_002cd7a0;
	self->running = 0;
	self->paused = 0;
	self->pausedAt = 0;
	self->start = 0;
	self->limit = 0.0f;
	self->remaining = 0.0f;
	self->limitMs = 0.0f;
	self->elapsed = 0.0f;
	self->fraction = 1.0f;
	return 1;
}

/* 0x001c35b0  TankedUp (gap)
Once a frame (unless paused): the elapsed time, the part of the length that has passed,
and the last frame's length in 1/60 s units (1.5 times as long with the Turbo Mode cheat,
core 0x00283e58). A stopped timer reads 0. */
void CTankTimer_Tick(struct TuTimer *self)
{
	unsigned long long now;
	long long ticks;

	if (self->paused != 0)
		return;
	if (self->running == 0) {
		self->elapsed = 0.0f;
		self->fraction = 0.0f;
		self->frame = 0.0f;
	} else {
		now = bkTimerRead();
		ticks = (long long)bkTimerDelta(self->start, now);
		if (self->limitMs == 0.0f)
			self->fraction = 1.0f;
		else
			self->fraction = (float)(((double)ticks * 1000.0f / (double)(long long)self->rate)
				/ self->limitMs);
		self->elapsed = (float)((double)ticks / (double)(long long)self->rate);
		if (DAT_00283e58 != 0)
			ticks = (long long)((double)(long long)bkTimerDelta(self->last, now) * 1.5);
		else
			ticks = (long long)bkTimerDelta(self->last, now);
		self->last = now;
		self->frame = (float)((double)ticks / (double)(long long)self->rate * 60.0f);
	}
	self->remaining = self->limit - self->elapsed;
	if (self->limit - self->elapsed < 0.0f)
		self->remaining = 0.0f;
	if (self->fraction > 1.0f)
		self->fraction = 1.0f;
}

/* 0x001c36e0  TankedUp (gap; shared with other games' timers)
Sets the length in seconds and stops the timer. */
void CTankTimer_SetLength(struct TuTimer *self, float seconds)
{
	float ms = seconds * 1000.0f;

	self->limit = seconds;
	self->running = 0;
	self->paused = 0;
	self->limitMs = ms;
}

/* 0x001c3700  TankedUp (gap)
Starts the timer, or resumes it when it is paused (the paused time is added to the start
and to the last frame, so it does not count). */
void CTankTimer_StartOrResume(struct TuTimer *self)
{
	unsigned long long pausedFor;

	if (self->paused != 0) {
		self->paused = 0;
		pausedFor = bkTimerDelta(self->pausedAt, bkTimerRead());
		self->start += pausedFor;
		self->last += pausedFor;
		return;
	}
	self->running = 1;
	self->start = bkTimerRead();
	self->last = bkTimerRead();
	self->elapsed = 0.0f;
	self->fraction = 1.0f;
}

/* 0x001c3770  TankedUp (gap)
Pauses the timer. */
void CTankTimer_Pause(struct TuTimer *self)
{
	if (self->paused == 0) {
		self->paused = 1;
		self->pausedAt = bkTimerRead();
	}
}

/* 0x001c3790  TankedUp (gap)
The constructor. */
void CTankTimer_Ctor(struct TuTimer *self)
{
	self->running = 0;
	self->paused = 0;
	self->pausedAt = 0;
	self->start = 0;
	self->limit = 0.0f;
	self->remaining = 0.0f;
	self->limitMs = 0.0f;
}

/* 0x001c37b0  TankedUp (gap; "timerSystem.Initialise")
Initialise with a length of `seconds` (game.cfg TIMER). Returns 1. */
int CTankTimer_Initialise(struct TuTimer *self, int seconds)
{
	float ms;

	self->seconds = seconds;
	self->rate = ((unsigned long long)DAT_002cd7a4 << 32) | DAT_002cd7a0;
	self->limit = (float)seconds;
	ms = (float)seconds * 1000.0f;
	self->pausedAt = 0;
	self->limitMs = ms;
	self->start = 0;
	self->remaining = 0.0f;
	self->elapsed = 0.0f;
	self->running = 0;
	self->paused = 0;
	self->fraction = 1.0f;
	return 1;
}

/* 0x001c3800  TankedUp (gap)
Ticks the timer and returns the last frame's length (1.0 = 1/60 s). */
float CTankTimer_GetFrame(struct TuTimer *self)
{
	CTankTimer_Tick(self);
	return self->frame;
}

/* 0x001c3810  TankedUp (gap; a jump to 0x001c3700)
The same as CTankTimer_StartOrResume (another method folded onto it). */
void CTankTimer_Resume(struct TuTimer *self)
{
	CTankTimer_StartOrResume(self);
}
