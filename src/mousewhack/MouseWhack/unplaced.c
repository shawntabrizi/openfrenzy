/*
MouseWhack/unplaced.c  (module mousewhack)

Gap functions of C:\BlitzParty\Game\Src\MouseWhack\ whose source file is not known: they
lie between CBbash_Game.cpp and CBbash_Player.cpp (0x00176100-0x0017d170), so they come
from another file of the directory.

  0x00176100-0x00176230  the frame timer of the arena (vtable 0x00272d64)
*/

#include "mousewhack.h"

/* bkernel: timestamp ticks per second */
extern longlong DAT_002cd7a0;

/* the frame timer (0xa0 bytes):
  +0x80 longlong ticks per second, +0x88 ulonglong now, +0x90 ulonglong the previous frame,
  +0x98 float milliseconds of the last frame, +0x9c float the same x 0.06 (frames at 60 Hz) */

/* 0x00272d64  the frame timer vtable */
const void *const vtable_00272d64[1] = {
	(const void *)FUN_00176230, /* [0] scalar deleting destructor */
};

/* 0x00176100  (gap): constructor */
void *MW_Timer_Ctor(void *self)
{
	*(const void *const **)self = vtable_00272d64;
	MW_AI(self, 0x80) = 0;
	MW_AI(self, 0x84) = 0;
	MW_AI(self, 0x88) = 0;
	MW_AI(self, 0x8c) = 0;
	MW_AI(self, 0x90) = 0;
	MW_AI(self, 0x94) = 0;
	MW_AI(self, 0x98) = 0;
	MW_AI(self, 0x9c) = 0;
	return self;
}

/* 0x00176140  (gap): destructor */
void MW_Timer_Dtor(void *self)
{
	*(const void *const **)self = vtable_00272d64;
}

/* 0x00176150  (gap): reset: reads the frequency and the time now */
int MW_Timer_Reset(void *self)
{
	ulonglong now;

	MW_AT(self, 0x80, longlong) = DAT_002cd7a0;
	now = bkTimerRead();
	MW_AT(self, 0x88, ulonglong) = now;
	MW_AT(self, 0x90, ulonglong) = now;
	return 1;
}

/* 0x001761a0  (gap): a frame: the time since the previous frame in milliseconds */
void MW_Timer_Update(void *self)
{
	ulonglong delta;
	double ms;

	/* _clearfp() (0x001e0a24): clears the x87 exception flags; nothing to do in the port */
	MW_AT(self, 0x90, ulonglong) = MW_AT(self, 0x88, ulonglong);
	MW_AT(self, 0x88, ulonglong) = bkTimerRead();
	delta = bkTimerDelta(MW_AT(self, 0x90, ulonglong), MW_AT(self, 0x88, ulonglong));
	ms = (double)(longlong)(delta * 1000u) / (double)MW_AT(self, 0x80, longlong);
	MW_AF(self, 0x98) = (float)ms;
	MW_AF(self, 0x9c) = (float)(ms * (double)0.06f);
}

/* 0x00176230  (gap): scalar deleting destructor */
void *MW_Timer_Destroy(void *self, unsigned char flags)
{
	*(const void *const **)self = vtable_00272d64;
	if ((flags & 1) != 0)
		MW_DELETE(self);
	return self;
}
