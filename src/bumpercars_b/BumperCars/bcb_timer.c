/*
BumperCars/bcb_timer.c  <-  gap functions 0x00096320-0x00096670 (file unknown)

A high resolution timer class (struct BCarTimer): the game timer of BumperCars (0x002f1fa8)
and the two timers of every entity. Other minigames call the getters too. The ticks come
from bKernel (bkTimerRead, bkTimerDelta, bkTimerRate).
*/

#include "bumpercars_b.h"

/* 0x00096320  (gap)
Constructor: stopped, nothing elapsed. The frequency is not set (see BCarTimer_Reset). */
void BCarTimer_Construct(struct BCarTimer *self)
{
	self->running = 0;
	self->paused = 0;
	self->pauseStart = 0;
	self->start = 0;
	self->duration = 0.0f;
	self->remaining = 0.0f;
	self->durationMs = 0.0f;
	self->elapsed = 0.0f;
	self->fraction = 1.0f;
	self->averageFrame = 0.0f;
	self->totalFrameTime = 0.0f;
}

/* 0x00096350  (gap)
Stops the timer and takes the tick frequency of bKernel. Returns 1. */
int BCarTimer_Reset(struct BCarTimer *self)
{
	self->frequency = DAT_002cd7a0;
	self->running = 0;
	self->paused = 0;
	self->pauseStart = 0;
	self->start = 0;
	self->duration = 0.0f;
	self->remaining = 0.0f;
	self->durationMs = 0.0f;
	self->elapsed = 0.0f;
	self->averageFrame = 0.0f;
	self->totalFrameTime = 0.0f;
	self->fraction = 1.0f;
	return 1;
}

/* 0x000963a0  (gap)
Updates the elapsed time, the fraction of the duration, the frame time and its average.
A paused timer does not change. */
void BCarTimer_Update(struct BCarTimer *self)
{
	unsigned long long now;
	longlong ticks;
	float frameTime;
	float total;

	if (self->paused != 0)
		return;
	if (self->running == 0)
	{
		self->elapsed = 0.0f;
		self->fraction = 0.0f;
		self->frames = 0.0f;
		self->frameTime = 0.0f;
		self->averageFrame = 0.0f;
	}
	else
	{
		now = bkTimerRead();
		ticks = (longlong)bkTimerDelta(self->start, now);
		if (self->durationMs == 0.0f)       /* 0x002536e0 */
			self->fraction = 1.0f;
		else
			self->fraction = (((float)ticks * 1000.0f) / (float)(longlong)self->frequency) /
				self->durationMs;           /* 1000: 0x00255898 */
		self->elapsed = (float)((double)ticks / (double)(longlong)self->frequency);
		ticks = (longlong)bkTimerDelta(self->last, now);
		self->last = now;
		self->frames = (float)(((double)ticks / (double)(longlong)self->frequency) * 60.0); /* 0x00254e98 */
		frameTime = bkTimerRate(ticks);
		self->frameTime = frameTime;
		total = self->totalFrameTime;
		self->totalFrameTime = frameTime + total;
		self->updates++;
		self->averageFrame = (float)(((double)frameTime + (double)total) /
			(double)(longlong)self->updates);
	}
	self->remaining = self->duration - self->elapsed;
	if (self->duration - self->elapsed < 0.0f)
		self->remaining = 0.0f;
	if (1.0f < self->fraction)                /* 0x00253850 */
		self->fraction = 1.0f;
}

/* 0x000964d0  (gap)
Starts the timer; a paused timer resumes instead (the pause does not count). */
void BCarTimer_Start(struct BCarTimer *self)
{
	unsigned long long ticks;

	if (self->paused != 0)
	{
		self->paused = 0;
		ticks = bkTimerDelta(self->pauseStart, bkTimerRead());
		self->start += ticks;
		self->last += ticks;
		return;
	}
	self->running = 1;
	self->start = bkTimerRead();
	self->last = bkTimerRead();
	self->updates = 0;
	self->elapsed = 0.0f;
	self->averageFrame = 0.0f;
	self->totalFrameTime = 0.0f;
	self->fraction = 1.0f;
}

/* 0x00096550  (gap)
Stops the game timer and sets a count down of `seconds`. Returns 1. */
int BCarGameTimer_Init(struct BCarGameTimer *self, int seconds)
{
	self->seconds = seconds;
	self->timer.frequency = DAT_002cd7a0;
	self->timer.duration = (float)seconds;
	self->timer.pauseStart = 0;
	self->timer.durationMs = (float)seconds * 1000.0f; /* 0x00255898 */
	self->timer.start = 0;
	self->timer.remaining = 0.0f;
	self->timer.elapsed = 0.0f;
	self->timer.averageFrame = 0.0f;
	self->timer.totalFrameTime = 0.0f;
	self->timer.running = 0;
	self->timer.paused = 0;
	self->timer.fraction = 1.0f;
	return 1;
}

/* 0x000965b0  (gap)
Updates the timer; returns the frames (at 60 Hz) since the last update, x1.5 with the
turbo cheat (0x00283e58). */
float BCarTimer_GetFrames(struct BCarTimer *self)
{
	float frames;

	BCarTimer_Update(self);
	frames = self->frames;
	if (DAT_00283e58 != 0)
		frames = frames * 1.5f;              /* 0x002538a0 */
	return frames;
}

/* 0x000965d0  (gap)
A second entry of BCarTimer_Start (an incremental-link thunk in the original). */
void BCarTimer_Start2(struct BCarTimer *self)
{
	BCarTimer_Start(self);
}

/* 0x000965e0  (gap) */
float BCarTimer_GetFraction(struct BCarTimer *self)
{
	return self->fraction;
}

/* 0x000965f0  (gap) */
float BCarTimer_GetRemaining(struct BCarTimer *self)
{
	return self->remaining;
}

/* 0x00096600  (gap) */
float BCarTimer_GetElapsed(struct BCarTimer *self)
{
	return self->elapsed;
}

/* 0x00096610  (gap) */
float BCarTimer_GetFrameTime(struct BCarTimer *self)
{
	return self->frameTime;
}

/* 0x00096620  (gap) */
float BCarTimer_GetAverageFrame(struct BCarTimer *self)
{
	return self->averageFrame;
}

/* 0x00096630  (gap)
Restarts a running timer from zero; the whole duration remains. */
void BCarTimer_Restart(struct BCarTimer *self)
{
	self->paused = 0;
	self->start = bkTimerRead();
	self->last = bkTimerRead();
	self->updates = 0;
	self->elapsed = 0.0f;
	self->totalFrameTime = 0.0f;
	self->averageFrame = 0.0f;
	self->fraction = 1.0f;
	self->remaining = self->duration;
}

/* 0x00096670  (gap) */
unsigned long long BCarTimer_GetUpdates(struct BCarTimer *self)
{
	return self->updates;
}
