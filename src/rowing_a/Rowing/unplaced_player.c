/*
Rowing/unplaced_player.c  <-  Rowing gaps 0x001a74a0-0x001a7ac0 and 0x001ac560-0x001ada40
(no file tag)

Player (CRowing_Player) methods just before the first and just after the last function
tagged CRowing_Player.cpp (0x001a7ac0, 0x001abc70). They use the player's fields (see
Rowing/CRowing_Player.c) and are probably in CRowing_Player.cpp, but no string proves it.

"Variant N" is the game's variant N (1-4); the code tests (game flags >> 2) & 3 = N - 1.
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"

#define PL_I32(off) RA_I32(self, off)
#define PL_F32(off) RA_F32(self, off)
#define PL_U8(off) RA_U8(self, off)
#define PL_PTR(off) RA_PTRF(self, off)

#define g_RowGameFlags RW_VAR(unsigned int, 0x0030f0b0)
#define g_GameState RW_VAR(int, 0x0030f0b4)
#define g_RowWorld RW_VAR(void *, 0x0030f12c)
#define g_RowWater RW_VAR(void *, 0x0030f130)
#define g_RowPlayer(i) RW_VAR(void *, 0x0030f490 + (i) * 4)
#define g_RowPlayers RW_VAR(int, 0x0030f0d4) /* game +0x24: the number of players */
#define RA_VARIANT ((g_RowGameFlags >> 2) & 3)
#define RA_CHARACTER(index) DAT_002baed4[(index) * 10]

extern int DAT_00283dc0; /* core: the steering type (0: two sticks) */
extern int DAT_00293928; /* classlib_ui: the voice volume */
extern int DAT_00290e8c;           /* bumpercars_a: the debris colour index */
extern unsigned char DAT_00291734[]; /* bumpercars_a: the debris type */
extern float DAT_0029394c;         /* classlib_ui: 180.0 */
extern int DAT_002ed324[];           /* classlib_ui: the rumble object */

/* a voice of the player's character at the boat */
#define PL_VOICE(self, kind) \
	CLSamples_PlayVoice((char)RA_CHARACTER(RA_I32(self, 0x370)), (kind), RA_FP(self, 0x2b4), DAT_00293928, \
		180.0f, 0, -1, 0)

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

/* 0x001ac560  Rowing (gap; after CRowing_Player.cpp's last tagged function)
Lap progress (variant 1, world +0x144 checkpoints): finds the checkpoint segment the boat is
in (+0xf8; the current one, the next, the previous, then all; +0xfc the fraction along it,
+0x36c the course heading). Crossing from the last quarter into the first counts a lap
(+0xf4; the last-lap jingle once, a random cheer 11-13); after the last lap (3, or 1 in the
other variants) the boat finishes (+0xe8 place, +0xf0 time, +0xec rank shared by equal
times; a voice for first and fourth place). Crossing back takes the lap off again; +0x100 is
set while going backwards. */
void FUN_001ac560(void *self)
{
	int n = RA_I32(g_RowWorld, 0x144);
	int old, cur, i, laps, snd, best, time, bestTime;
	float frac;

	if (n == 0)
		return;
	old = PL_I32(0xf8);
	cur = old;
	if ((unsigned char)FUN_001b9490(cur, PL_F32(0x2b4), PL_F32(0x2bc), &frac, RA_FP(self, 0x36c)) == 0) {
		cur = (old + 1) % n;
		if ((unsigned char)FUN_001b9490(cur, PL_F32(0x2b4), PL_F32(0x2bc), &frac, RA_FP(self, 0x36c)) == 0) {
			cur = (n - 1 + old) % n;
			if ((unsigned char)FUN_001b9490(cur, PL_F32(0x2b4), PL_F32(0x2bc), &frac, RA_FP(self, 0x36c)) == 0) {
				for (cur = 0; cur < n; cur++)
					if ((unsigned char)FUN_001b9490(cur, PL_F32(0x2b4), PL_F32(0x2bc), &frac,
						    RA_FP(self, 0x36c)) != 0)
						break;
				if (cur >= n) {
					bkPrintf("This Never Happens\n");
					cur = 0;
					frac = 0.0f;
				}
			}
		}
	}
	if (old >= n * 3 / 4 && cur <= n / 4) {
		/* a lap */
		PL_I32(0xf4)++;
		PL_U8(0x100) = 0;
		laps = (g_RowGameFlags & 0xc) != 0 ? 1 : 3;
		if (PL_I32(0xf4) == laps && RA_I32(g_RowSound, 0x57c) == 0) {
			rwb_Sound_PlayRandom(0x12, 0x80);
			RA_I32(g_RowSound, 0x57c) = 1;
		}
		snd = rw_rand() % 3 + 0xb;
		if (PL_I32(0xf4) > laps && PL_I32(0xe8) == 0) {
			best = 0;
			for (i = 0; i < g_RowPlayers; i++)
				if (RA_I32(g_RowPlayer(i), 0xe8) > best)
					best = RA_I32(g_RowPlayer(i), 0xe8);
			time = PL_I32(0x39c);
			PL_I32(0xe8) = best + 1;
			PL_I32(0xf0) = time;
			if (best + 1 == 1) {
				snd = (rw_rand() & 1) + 7;
				PL_I32(0xec) = 1;
				PL_VOICE(self, 9);
			} else {
				best = 0;
				bestTime = 0;
				for (i = 0; i < g_RowPlayers; i++) {
					if (RA_I32(g_RowPlayer(i), 0xec) > best) {
						best = RA_I32(g_RowPlayer(i), 0xec);
						bestTime = RA_I32(g_RowPlayer(i), 0xf0);
					}
				}
				if (time != bestTime)
					best++;
				PL_I32(0xec) = best;
				if (PL_I32(0xe8) == 4)
					PL_VOICE(self, 0x10);
			}
			FUN_001a8940(self, 1);
		}
		rwb_Sound_PlayRandom(snd, 0x80);
	} else if (old <= n / 4 && cur >= n * 3 / 4) {
		/* back over the line */
		if (--PL_I32(0xf4) < 0)
			PL_I32(0xf4) = 0;
		PL_U8(0x100) = 1;
	} else
		PL_U8(0x100) = cur <= old;
	if (cur >= n)
		cur = 0;
	PL_I32(0xf8) = cur;
	PL_F32(0xfc) = frac;
}

/* 0x001ac8f0  Rowing (gap; after CRowing_Player.cpp's last tagged function)
The player's frame. Computer drivers pick a target by variant and steer (0x001a8d30);
humans (or all players with 0x0030f0a0 set) read the sticks; both set the camera's
look-ahead point (+0x35c). Then the flash, the state clock, sticks centred outside the race
states, the variant's rules (2: gates and penalty time; 3: tokens, falling in and respawning;
4: distance, sinking places), the controls, the hull dents (spread to the neighbours, a bump
animation, rumble, voice and sound), the boing bones, the body's physics and collisions, the
move, the lap progress, the sticks kept for next frame and a splash on the water. */
void FUN_001ac8f0(void *self, float dt)
{
	float v[4], d[4], start[4], s[8], max, heading;
	char text[16];
	int lx, ly, rx, ry, i, score, keep, lost, dentA, dentB, best, idx;
	const int *col;
	void *token, *body;

	PL_I32(0x39c)++;
	if (DAT_002baed4[PL_I32(0x370) * 10 + 1] == 0 && RW_VAR(int, 0x0030f0a0) == 0) {
		switch (RA_VARIANT) {
		case 0:
			FUN_001a99f0(self);
			PL_F32(0x144) = PL_F32(0x2b8);
			PL_F32(0x14c) = PL_F32(0x2c0);
			Core_Vec3Sub(d, RA_FP(self, 0x140), RA_FP(self, 0x2b4));
			bmVectorSetLength(v, d, 200.0f);
			cmb_vec3_add(RA_FP(self, 0x35c), RA_FP(self, 0x2b4), v);
			break;
		case 1:
			FUN_001a9af0(self);
			Core_Vec4Copy(RA_FP(self, 0x35c), RA_FP(self, 0x2b4));
			PL_F32(0x35c) = sinf(PL_F32(0x36c)) * 300.0f + PL_F32(0x35c);
			PL_F32(0x364) = cosf(PL_F32(0x36c)) * 300.0f + PL_F32(0x364);
			break;
		case 2:
			FUN_001a9e60(self);
			Core_Vec4Copy(RA_FP(self, 0x35c), RA_FP(self, 0x2b4));
			break;
		case 3:
			FUN_001aa000(self, dt);
			Core_Vec4Copy(RA_FP(self, 0x35c), RA_FP(self, 0x2b4));
			break;
		}
		FUN_001a8d30(self, dt, &lx, &ly, &rx, &ry);
	} else {
		switch (RA_VARIANT) {
		case 0:
			FUN_001a99f0(self);
			PL_F32(0x144) = PL_F32(0x2b8);
			PL_F32(0x14c) = PL_F32(0x2c0);
			d[0] = PL_F32(0x140) - PL_F32(0x2b4);
			d[1] = PL_F32(0x144) - PL_F32(0x2b8);
			d[2] = PL_F32(0x148) - PL_F32(0x2bc);
			bmVectorSetLength(v, d, 300.0f);
			PL_F32(0x35c) = v[0] + PL_F32(0x2b4);
			PL_F32(0x360) = v[1] + PL_F32(0x2b8);
			PL_F32(0x364) = v[2] + PL_F32(0x2bc);
			break;
		case 1:
			memcpy(RA_FP(self, 0x35c), RA_FP(self, 0x2b4), 0x10);
			PL_F32(0x35c) = sinf(PL_F32(0x36c)) * 600.0f + PL_F32(0x35c);
			PL_F32(0x364) = cosf(PL_F32(0x36c)) * 600.0f + PL_F32(0x364);
			break;
		case 2:
		case 3:
			memcpy(RA_FP(self, 0x35c), RA_FP(self, 0x2b4), 0x10);
			break;
		}
		FUN_001a7710(self, &lx, &ly, &rx, &ry);
	}
	FUN_001aaa30(self, dt);
	PL_I32(0x384)++;
	PL_F32(0x388) = dt + PL_F32(0x388);
	switch (PL_I32(0x380)) {
	case 0:
	case 2:
	case 3:
	case 5:
		lx = -0x8000;
		rx = 0x8000;
		ly = 0;
		ry = 0;
		break;
	}

	switch (RA_VARIANT) {
	case 1: /* variant 2: the gates */
		i = FUN_001b9ac0(PL_F32(0x2b4), PL_F32(0x2bc), (int *)RA_P(self, 0x10c), (char *)RA_P(self, 0x110),
			RA_I32(g_RowWorld, 0x19c), (struct RowPath *)RA_PTRF(g_RowWorld, 0x1a0));
		if (i == 1) {
			FUN_001b6ee0(0xf, RA_FP(self, 0x2b4), 0xff);
		} else if (i == 2) {
			FUN_001b6ee0(0x10, RA_FP(self, 0x2b4), 0xff);
			PL_VOICE(self, 0xe);
			CLRumble_PlayNamed((int)(uintptr_t)DAT_002ed324, PL_I32(0x370), "JB_WrongGate");
		}
		if (PL_I32(0x380) == 1)
			PL_F32(0x124) = RA_F32(RW_VAR(void *, 0x0030f148), 0x98) + PL_F32(0x124);
		FUN_001a7540(self);
		break;
	case 2: /* variant 3: the tokens */
		if (PL_I32(0x380) == 1 && PL_U8(0x34a) == 0 && PL_U8(0x349) == 0)
			token = FUN_001b5740(PL_F32(0x2b4), PL_F32(0x2bc), 62.4410019f);
		else
			token = NULL;
		score = 0;
		if (g_GameState == 5 && token != NULL) {
			switch (RA_I32(token, 0x1ec)) {
			case 0:
				score = 1;
				break;
			case 1:
				score = 3;
				break;
			case 2:
				score = 5;
				PL_VOICE(self, 4);
				break;
			}
			for (i = 0; i < 4; i++)
				if (RA_PTRF(g_RowPlayer(i), 0x15c) == token)
					RA_PTRF(g_RowPlayer(i), 0x15c) = NULL;
			CToken_Collect((struct CToken *)token);
			if (score != 0) {
				PL_I32(0x3a0) += score;
				sprintf(text, "%d", score);
				col = ra_CharacterColour[RA_CHARACTER(PL_I32(0x370))];
				baAddExtraItem(*(struct BExtraSystem **)DAT_002f27c4, RW_VAR(void *, 0x0030f160),
					(double)PL_F32(0x2b4), (double)PL_F32(0x2b8), (double)PL_F32(0x2bc),
					(unsigned int)col[0] >> 2, (unsigned int)col[1] >> 2, (unsigned int)col[2] >> 2, text);
			}
		}
		if (PL_U8(0x349) != 0 && PL_I32(0xbc) == 0) {
			PL_I32(0xbc) = 1;
			PL_VOICE(self, 0xd);
		}
		if (PL_U8(0x34a) != 0 && PL_I32(0x380) != 5) {
			/* fallen in: lose a quarter of the score */
			score = PL_I32(0x3a0);
			PL_I32(0xbc) = 0;
			if (g_GameState == 5) {
				keep = (int)((1.0f - RW_VAR(float, 0x002b56c8)) * (float)score);
				lost = score - keep;
				PL_I32(0x3a0) = keep;
				if (lost > 0) {
					sprintf(text, "-%d", lost);
					col = ra_CharacterColour[RA_CHARACTER(PL_I32(0x370))];
					baAddExtraItem(*(struct BExtraSystem **)DAT_002f27c4, RW_VAR(void *, 0x0030f160),
						0.0, -100.0, 0.0, (unsigned int)col[0] >> 2, (unsigned int)col[1] >> 2,
						(unsigned int)col[2] >> 2, text);
				}
			}
			PL_I32(0x380) = 5;
			PL_I32(0x384) = 0;
			PL_I32(0x388) = 0;
			PL_U8(0x37c) = 0;
			PL_I32(0x3bc) = 0;
		}
		if (PL_I32(0x380) == 5 && RW_VAR(float, 0x0030f4a0) * 60.0f < PL_F32(0x388)) {
			/* respawn 200 above the start slot, then drop in */
			i = PL_I32(0x378);
			start[0] = RA_F32(g_RowWorld, 0x84 + i * 0x14);
			start[1] = RA_F32(g_RowWorld, 0x88 + i * 0x14) + 200.0f;
			start[2] = RA_F32(g_RowWorld, 0x8c + i * 0x14);
			start[3] = RA_F32(g_RowWorld, 0x90 + i * 0x14);
			heading = RA_F32(g_RowWorld, 0x94 + i * 0x14);
			if (PL_I32(0x3bc) == 0) {
				start[1] -= 200.0f;
				DAT_00290e8c = RA_CHARACTER(PL_I32(0x370));
				Debris_Add((struct DebrisType *)DAT_00291734, start, DAT_002ea478, 0, 0xffffffffu);
				start[1] += 200.0f;
				PL_I32(0x3bc) = 1;
				FUN_001a7cf0(self, start, heading);
			}
			PL_U8(0x34b) = 1;
			PL_F32(0x2b8) = start[1];
			PL_F32(0x304) = 0.0f;
			PL_F32(0x308) = 0.0f;
			PL_F32(0x30c) = 0.0f;
			if (RW_VAR(float, 0x002b56cc) * 60.0f < PL_F32(0x388)) {
				PL_U8(0x34b) = 0;
				RA_F32((void *)(uintptr_t)DAT_002ecfac, 0xb8 + PL_I32(0x370) * 4) = DAT_0029394c;
				PL_I32(0x380) = 1;
				PL_I32(0x384) = 0;
				PL_I32(0x388) = 0;
				PL_VOICE(self, 0xf);
			}
		}
		break;
	case 3: /* variant 4: distance; places by sinking order */
		PL_F32(0x3e4) = PL_F32(0x3e4) + PL_F32(0x2bc);
		if (PL_U8(0x349) != 0 && PL_I32(0xe8) == 0) {
			best = 5;
			for (i = 0; i < g_RowPlayers; i++)
				if (RA_I32(g_RowPlayer(i), 0xe8) != 0 && RA_I32(g_RowPlayer(i), 0xe8) < best)
					best = RA_I32(g_RowPlayer(i), 0xe8);
			PL_I32(0xe8) = best - 1;
			PL_I32(0x108) = PL_I32(0x39c);
			PL_VOICE(self, 0xd);
		}
		if (PL_U8(0x34a) != 0) {
			if (PL_U8(0x37c) != 0 && PL_I32(0xe8) >= 3)
				CLSamples_PlayCommentary(0x37, RA_CHARACTER(PL_I32(0x370)), 0xff, 0, 0, NULL);
			PL_U8(0x37c) = 0;
		}
		if (PL_U8(0x34a) != 0 || PL_U8(0x349) != 0) {
			if (PL_F32(0x30c) > 0.0f)
				PL_F32(0x30c) = 0.0f;
			if (PL_F32(0x2bc) >= -1860.09998f)
				PL_F32(0x2bc) = -1860.09998f;
		}
		if (PL_F32(0x2c8) < -3.1415925f) {
			heading = PL_F32(0x2c8);
			do
				heading += 6.28318548f;
			while (heading < -3.1415925f);
			PL_F32(0x2c8) = heading;
		}
		if (PL_F32(0x2c8) >= 3.1415925f) {
			heading = PL_F32(0x2c8);
			do
				heading -= 6.28318548f;
			while (heading >= 3.1415925f);
			PL_F32(0x2c8) = heading;
		}
		break;
	}

	FUN_001ab0c0(self, lx, ly, rx, ry, dt);
	if (PL_I32(0xb8) != 0)
		PL_I32(0xb8)--;
	for (i = 0; i < 8; i++) {
		PL_I32(0x1b4 + i * 0x20) = 0;
		PL_I32(0x1b8 + i * 0x20) = 0;
		PL_I32(0x1bc + i * 0x20) = 0;
		PL_I32(0x1c0 + i * 0x20) = 0;
	}
	if (PL_U8(0x37c) != 0) {
		dentA = FUN_001a8260(self);
		dentB = FUN_001a7e40(self);
		/* each dent spreads half into an undented neighbour */
		for (i = 0; i < 8; i++)
			s[i] = PL_F32(0x1b4 + i * 0x20);
		for (i = 0; i < 8; i++) {
			float prev = s[(i + 7) & 7], next = s[(i + 1) & 7];

			if (next == 0.0f && s[i] < prev)
				PL_F32(0x1b4 + i * 0x20) = prev * 0.5f;
			if (PL_F32(0x1b4 + i * 0x20) < next && prev == 0.0f)
				PL_F32(0x1b4 + i * 0x20) = next * 0.5f;
		}
		if ((dentA | dentB) != 0 && PL_I32(0xb8) == 0) {
			max = PL_F32(0x1b4);
			idx = 0;
			for (i = 1; i < 8; i++) {
				if (PL_F32(0x1b4 + i * 0x20) > max) {
					max = PL_F32(0x1b4 + i * 0x20);
					idx = i;
				}
			}
			FUN_001a8830(self, RW_VAR(int, 0x002b58e0 + idx * 4), 0.2f);
			if (PL_U8(0x34a) == 0 && PL_U8(0x349) == 0)
				CLRumble_PlayNamed((int)(uintptr_t)DAT_002ed324, PL_I32(0x370), "JB_Collide");
			if (max > 0.349999994f)
				PL_VOICE(self, 2);
			FUN_001b6ee0(0x13, RA_FP(self, 0x2b4), 0xff);
			PL_I32(0xb8) = 0x1e;
		}
	}
	for (i = 0; i < 8; i++) {
		void *node = PL_PTR(0x1c4 + i * 0x20);

		if (node != NULL)
			baSetNodeTime((struct BActorInstance *)PL_PTR(0x358), (struct BActorNode *)node,
				(int)((float)(PL_I32(0x40) - PL_I32(0x3c)) * PL_F32(0x1b4 + i * 0x20)) + PL_I32(0x3c), 1);
	}
	FUN_001a8600(self);
	body = RA_P(self, 0x2a8);
	FUN_001a5ad0(body);
	FUN_001a5f00(body, dt);
	FUN_001a6f50(body, 50.0f, PL_I32(0x3a4));
	FUN_001a6500(body, RA_I32(g_RowWorld, 0x134), (unsigned char *)RA_PTRF(g_RowWorld, 0x138), 50.0f);
	PL_F32(0x2b4) = PL_F32(0x2b4) + PL_F32(0x314);
	PL_F32(0x2b8) = PL_F32(0x2b8) + PL_F32(0x318);
	PL_F32(0x2bc) = PL_F32(0x2bc) + PL_F32(0x31c);
	FUN_001ac560(self);
	PL_I32(0x198) = lx;
	PL_I32(0x1a4) = ry;
	PL_I32(0x19c) = ly;
	PL_I32(0x1a0) = rx;
	if (PL_U8(0x34a) == 0 && g_RowWater != NULL && PL_F32(0x2b8) - PL_F32(0x344) < 50.0f)
		RA_VFN(g_RowWater, 0x28, void (*)(void *, float, float, float, float, float))(g_RowWater,
			PL_F32(0x2b4), PL_F32(0x2bc), -5.0f, 32.4410019f, 62.4410019f);
}
