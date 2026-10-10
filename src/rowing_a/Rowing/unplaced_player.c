/*
Rowing/unplaced_player.c  <-  Rowing gap 0x001a74a0-0x001a7ac0 (no file tag)

Player (CRowing_Player) methods between the floating body (Rowing/unplaced_body.c) and the
first function tagged CRowing_Player.cpp (0x001a7ac0). They use the player's fields (see
Rowing/CRowing_Player.c) and are probably in CRowing_Player.cpp, but no string proves it.
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"

#define PL_I32(off) RA_I32(self, off)
#define PL_F32(off) RA_F32(self, off)
#define PL_U8(off) RA_U8(self, off)
#define PL_PTR(off) RA_PTRF(self, off)

#define g_RowWorld RW_VAR(void *, 0x0030f12c)

extern int DAT_00283dc0; /* core: the steering type (0: two sticks) */
extern int DAT_00293928; /* classlib_ui: the voice volume */

/* an input channel's value (channel +0x14) */
#define PL_INPUT(off) RA_I32(PL_PTR(off), 0x14)

/* 0x001a74a0  Rowing (gap)
The boat's position (+0x2b4). */
float *FUN_001a74a0(void *self)
{
	return RA_FP(self, 0x2b4);
}

/* 0x001a74b0  Rowing (gap)
Sets up hull deformation slot INDEX (0-7) of the boat instance INST: its angle (INDEX/8 of
a turn), cleared state and the node "deform0<INDEX+1>". Returns 0 for a bad index. */
int FUN_001a74b0(void *slot, void *inst, int index)
{
	char *name = (char *)RW_ADDR(0x0030f4b4);
	float angle;

	if (index < 0 || index >= 8)
		return 0;
	angle = (float)index * 0.125f * 6.28318548f;
	sprintf(name, index < 9 ? "deform0%d" : "deform%d", index + 1);
	RA_PTRF(slot, 0x1c) = baFindNode((struct BActorNode *)RA_PTRF(inst, 0x134), name);
	RA_I32(slot, 0xc) = 0;
	RA_I32(slot, 0x10) = 0;
	RA_I32(slot, 0x14) = 0;
	RA_I32(slot, 0x18) = 0;
	RA_F32(slot, 0) = angle;
	return 1;
}

/* 0x001a7540  Rowing (gap)
Counts the gates (world +0x19c) missed (+0x130, mark 2) and passed (+0x134, mark 1) and
sets the distance (+0x128) to the base (+0x124) plus a penalty per missed gate (.data
0x002b56d0 x 1000). */
void FUN_001a7540(void *self)
{
	int i;

	PL_I32(0x130) = 0;
	PL_I32(0x134) = 0;
	for (i = 0; i < RA_I32(g_RowWorld, 0x19c); i++) {
		if (PL_U8(0x110 + i) == 2)
			PL_I32(0x130)++;
		if (PL_U8(0x110 + i) == 1)
			PL_I32(0x134)++;
	}
	PL_F32(0x128) = (float)PL_I32(0x130) * RW_VAR(float, 0x002b56d0) * 1000.0f + PL_F32(0x124);
}

/* 0x001a75c0  Rowing (gap)
The driver's animation (+0x90) has ended: start the next one. The start animations (2, 5,
15) go to their loops (3, 6, 16), which end after 15 loops (+0x98) in their end animations
(4, 7, 17); a bump (11-14) goes back to the animation before it (+0x94); "idle" (10)
repeats; anything else goes back to neutral (0). */
void FUN_001a75c0(void *self)
{
	switch (PL_I32(0x90)) {
	case 2:
		PL_I32(0x98) = 0;
		FUN_001a8830(self, 3, 0.2f);
		return;
	case 3:
		if (++PL_I32(0x98) < 15)
			FUN_001a8830(self, 3, 0.0f);
		else
			FUN_001a8830(self, 4, 0.0f);
		return;
	case 5:
		PL_I32(0x98) = 0;
		FUN_001a8830(self, 6, 0.2f);
		return;
	case 6:
		if (++PL_I32(0x98) < 15)
			FUN_001a8830(self, 6, 0.2f);
		else
			FUN_001a8830(self, 7, 0.2f);
		return;
	case 10:
		FUN_001a8830(self, 10, 0.0f);
		return;
	case 11:
	case 12:
	case 13:
	case 14:
		if (PL_I32(0x94) != 0) {
			FUN_001a8830(self, PL_I32(0x94), 0.2f);
			return;
		}
		break;
	case 15:
		PL_I32(0x98) = 0;
		FUN_001a8830(self, 0x10, 0.2f);
		return;
	case 16:
		if (++PL_I32(0x98) < 15)
			FUN_001a8830(self, 0x10, 0.0f);
		else
			FUN_001a8830(self, 0x11, 0.0f);
		return;
	}
	FUN_001a8830(self, 0, 0.0f);
}

/* 0x001a76e0  Rowing (gap)
Nonzero when a human player holds either action button (+0x190, +0x194, no debounce). */
int FUN_001a76e0(void *self)
{
	if (DAT_002baed4[PL_I32(0x370) * 10 + 1] == 0)
		return 0;
	return PL_INPUT(0x194) | PL_INPUT(0x190);
}

static int ra_ClampStick(int v)
{
	if (v > 0x8000)
		return 0x8000;
	if (v < -0x8000)
		return -0x8000;
	return v;
}

/* 0x001a7710  Rowing (gap)
Reads a human player's sticks into *LX, *LY, *RX, *RY (scaled by 32768 / 29000, clamped to
+-0x8000). A fallen or sunk boat gets zeros and its collider off. Action 1 notes the time
(+0xc0 = 0x0030f0bc). With the simple controls (.data 0x002b567c, steering type 0x00283dc0
0) the action buttons drive both oars and the left stick steers; flicking the right stick Y
10 times (+0x3c0) plays the "controls" voice. Else the sticks map directly (0x0030f4a4 = 0)
or turned (1); flicking the right stick X 20 times plays another voice. */
void FUN_001a7710(void *self, int *lx, int *ly, int *rx, int *ry)
{
	int t, v;

	if (PL_U8(0x34a) != 0 || PL_U8(0x349) != 0) {
		FUN_001a5990(PL_I32(0x3a4), 0);
		*lx = 0;
		*rx = 0;
		*ly = 0;
		*ry = 0;
		return;
	}
	if (PL_INPUT(0x190) != 0)
		PL_I32(0xc0) = RW_VAR(int, 0x0030f0bc);
	if (RW_VAR(int, 0x002b567c) != 0 && PL_I32(0x3c0) < 10 && DAT_00283dc0 == 0) {
		*ry = 0;
		*ly = 0;
		if (PL_INPUT(0x188) != 0 && PL_INPUT(0x18c) == 0) {
			*ry = -0x8000;
			*ly = -0x8000;
		}
		if (PL_INPUT(0x18c) != 0 && PL_INPUT(0x188) == 0) {
			*ry = 0x8000;
			*ly = 0x8000;
		}
		t = (int)(float)PL_INPUT(0x178);
		*ly -= t;
		*ry += t;
		*ly = ra_ClampStick(*ly);
		*rx = 0;
		*lx = 0;
		v = PL_INPUT(0x184);
		if (v > 20000 && (PL_I32(0x3c0) & 1) != 0)
			PL_I32(0x3c0)++;
		else if (v < -20000 && (PL_I32(0x3c0) & 1) == 0)
			PL_I32(0x3c0)++;
		if (PL_I32(0x3c0) >= 10)
			CLSamples_PlayVoice((char)DAT_002baed4[PL_I32(0x370) * 10], 8, RA_FP(self, 0x2b4),
				DAT_00293928, 180.0f, 0, -1, 0);
	} else {
		if (RW_VAR(int, 0x0030f4a4) == 0) {
			*lx = PL_INPUT(0x178);
			*ly = PL_INPUT(0x17c);
			*rx = PL_INPUT(0x180);
			*ry = PL_INPUT(0x184);
		} else if (RW_VAR(int, 0x0030f4a4) == 1) {
			*lx = -PL_INPUT(0x17c);
			*ly = PL_INPUT(0x178);
			*rx = PL_INPUT(0x184);
			*ry = -PL_INPUT(0x180);
		}
		v = PL_INPUT(0x180);
		if ((v > 20000 && (PL_I32(0x3c0) & 1) != 0) || (v < -20000 && (PL_I32(0x3c0) & 1) == 0))
			PL_I32(0x3c0)++;
		if (PL_I32(0x3c0) >= 20) {
			CLSamples_PlayVoice((char)DAT_002baed4[PL_I32(0x370) * 10], 0xe, RA_FP(self, 0x2b4),
				DAT_00293928, 180.0f, 0, -1, 0);
			PL_I32(0x3c0) = 0;
		}
	}
	*lx = ra_ClampStick((*lx * 32768) / 29000);
	*ly = ra_ClampStick((*ly * 32768) / 29000);
	*rx = ra_ClampStick((*rx * 32768) / 29000);
	*ry = ra_ClampStick((*ry * 32768) / 29000);
}
