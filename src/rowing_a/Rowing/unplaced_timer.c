/*
Rowing/unplaced_timer.c  <-  Rowing gap 0x001af380-0x001af720 (no file tag)

The frame timer (game +0x98, 0x240 bytes, vtable 0x00275ba8): it times each frame against
the tick frequency and, while "Toggle Timer" has turned it on, draws up to 50 coloured
marks of the frame as bars (the debug text object, game +0x9c, draws them).

The timer (fields at original offsets):
  +0x00 the vtable, +0x10 five section start times (0x18 bytes each)
  +0x80 the tick frequency, +0x88 now, +0x90 the frame start (64-bit ticks)
  +0x98 the frame time in ms (at most 80), +0x9c the same x 0.06
  +0xa0 the marks' colours [50], +0x168 the marks' times in ms [50], +0x230 the marks
  +0x234 the bars are shown, +0x238 the input map, +0x23c the "Toggle Timer" channel
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"
#include "d3d8_extra.h"

#define TM_I32(off) RA_I32(self, off)
#define TM_U32(off) RA_U32(self, off)
#define TM_F32(off) RA_F32(self, off)
#define TM_I64(off) RA_AT(self, off, long long)


/* .rdata 0x00275ba8: the timer's vtable (rebuilt) */
const void *const vtable_00275ba8[1] = {
	(const void *)FUN_001af660, /* +0x00 deleting destructor */
};

/* the ms from START to now */
static float ra_MsSince(void *self, unsigned long long start, unsigned long long now)
{
	long long d = (long long)bkTimerDelta(start, now);

	return (float)((double)(d * 1000) / (double)TM_I64(0x80));
}

/* 0x001af380  Rowing (gap)
The timer's constructor. */
void *FUN_001af380(void *self)
{
	RA_AT(self, 0, const void *) = vtable_00275ba8;
	TM_I32(0x80) = 0;
	TM_I32(0x84) = 0;
	TM_I32(0x88) = 0;
	TM_I32(0x8c) = 0;
	TM_I32(0x90) = 0;
	TM_I32(0x94) = 0;
	TM_I32(0x98) = 0;
	TM_I32(0x9c) = 0;
	TM_I32(0x230) = 0;
	TM_I32(0x234) = 0;
	return self;
}

/* 0x001af3d0  Rowing (gap)
Starts the timer: the tick frequency, the frame start now, and the "Toggle Timer" input
channel. Returns 1. */
int FUN_001af3d0(void *self)
{
	unsigned long long now;

	TM_I64(0x80) = DAT_002cd7a0;
	now = bkTimerRead();
	TM_I64(0x88) = (long long)now;
	TM_I64(0x90) = (long long)now;
	RA_PTRF(self, 0x238) = biCreateMap(1);
	RA_PTRF(self, 0x23c) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x238), "Toggle Timer", 1, 1, 0, 0, 0);
	return 1;
}

/* 0x001af450  Rowing (gap)
Frees the timer's input map. */
void FUN_001af450(void *self)
{
	biDeleteMap((struct biMap *)RA_PTRF(self, 0x238));
}

/* 0x001af460  Rowing (gap)
A new frame: the last frame's time (+0x98, at most 80 ms; +0x9c x 0.06), no marks yet, and
"Toggle Timer" switches the bars. */
void FUN_001af460(void *self)
{
	unsigned long long now = bkTimerRead();
	void *channel;

	TM_I64(0x88) = (long long)now;
	TM_F32(0x98) = ra_MsSince(self, (unsigned long long)TM_I64(0x90), now);
	TM_I64(0x90) = TM_I64(0x88);
	if (TM_F32(0x98) > 80.0f)
		TM_F32(0x98) = 80.0f;
	channel = RA_PTRF(self, 0x23c);
	TM_I32(0x230) = 0;
	TM_F32(0x9c) = TM_F32(0x98) * 0.0600000024f;
	if (RA_I32(channel, 0x14) != 0) {
		TM_I32(0x234) = TM_I32(0x234) == 0;
		RA_I32(channel, 0x14) = 0;
	}
}

/* 0x001af540  Rowing (gap)
Notes the start of section SECTION (0-4). */
void FUN_001af540(void *self, int section)
{
	if (section < 5)
		RA_AT(self, 0x10 + section * 0x18, unsigned long long) = bkTimerRead();
}

/* 0x001af570  Rowing (gap)
Draws the marks as bars at y -205 (400 units per 66.7 ms) and two red ends. */
void FUN_001af570(void *self)
{
	float prev = 0.0f, t;
	int i;

	if (TM_I32(0x230) == 0 || TM_I32(0x234) == 0 || TM_I32(0x230) >= 0x32)
		return;
	for (i = 0; i < TM_I32(0x230); i++) {
		t = TM_F32(0x168 + i * 4) * 0.0149999997f;
		FUN_001afdc0(t * 400.0f - 200.0f, -205.0f, (prev - t) * 400.0f, 10.0f,
			TM_U32(0xa0 + i * 4));
		prev = t;
	}
	FUN_001afdc0(-200.0f, -207.0f, 1.0f, 14.0f, 0xffff0000u);
	FUN_001afdc0(-100.0f, -207.0f, 1.0f, 14.0f, 0xffff0000u);
}

/* 0x001af660  Rowing (gap) (vtable 0x00275ba8 slot 0)
The timer's deleting destructor. */
void *FUN_001af660(void *self, unsigned int flags)
{
	RA_AT(self, 0, const void *) = vtable_00275ba8;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x001af680  Rowing (gap)
Adds a mark in COLOUR at the time since the frame start (waiting for the GPU first while
the bars are shown and 0x0030f768 is set). */
void FUN_001af680(void *self, unsigned int colour)
{
	unsigned long long now;

	if (TM_I32(0x230) >= 0x32)
		return;
	if (TM_I32(0x234) != 0 && RW_VAR(int, 0x0030f768) != 0)
		D3D_KickOffAndWaitForIdle();
	now = bkTimerRead();
	TM_U32(0xa0 + TM_I32(0x230) * 4) = colour;
	TM_F32(0x168 + TM_I32(0x230) * 4) = ra_MsSince(self, (unsigned long long)TM_I64(0x90), now);
	TM_I32(0x230)++;
}
