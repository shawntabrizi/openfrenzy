/*
Rowing/CRowing_Player.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_Player.cpp

A jet boat and its driver (CRowing_Player, 1000 bytes, four of them at 0x0030f490).
0x001a7ac0 and 0x001abc70 are tagged with this file; the functions between them are
inferred to be in it (CRowing_Player.cpp, "bracketed").

The player (fields at original offsets):
  +0x000 the driver's actor, +0x004 the driver's simple actor, +0x008 the boat's actor,
  +0x00c the boat's instance, +0x010 the plug actor, +0x014 the plug instance
  +0x018, +0x01c the engine bones, +0x020, +0x024 the steering sticks
  +0x028 the boat's push_forward animation, +0x02c/+0x030/+0x034 its reverse/middle/forward
  frames, +0x038 the "boing" animation, +0x03c/+0x040 its frames
  +0x044 the driver's push_forward animation, +0x048 the driver's 18 animations
  (neutral_back, push_forward, win start/loop/end, lose start/loop/end, right, left, idle,
  bump f/b/l/r, victory start/loop/end), +0x090 the playing one, +0x094 the one before a
  bump, +0x098 a counter, +0x09c the animation ended
  +0x0a0, +0x0a4 the shoulder nodes, +0x0a8/+0x0ac/+0x0b0 the driver's frames
  +0x0b4 the engine sound, +0x0c0 the time of the last turbo press
  +0x0c4 the driver's instance, +0x0c8 the simple instance, +0x0cc the one drawn
  +0x0e8 the place (1 = first), +0x110 the gates passed (one byte each, 1 or 2),
  +0x124 distance base, +0x128 distance, +0x130 gates missed, +0x134 gates passed
  +0x170 the input channels (+0x178 leftX, +0x17c leftY, +0x180 rightX, +0x184 rightY,
  +0x188 action1, +0x18c action2, +0x190 action1 (no debounce), +0x194 action2 ...)
  +0x1a8 eight hull deformations of 0x20 bytes (angle, -, -, depth, push x, push z, wobble,
  the node)
  +0x2a8 the floating body (Rowing/unplaced_body.c): +0x2b4 position, +0x2c8 heading,
  +0x2fc mass, +0x300 spin, +0x304 velocity, +0x314 move, +0x348 airborne, +0x349 sunk,
  +0x34a fell, +0x34b frozen, +0x358 the instance
  +0x370 the player index, +0x374 the handicap, +0x378 the start slot, +0x37c in the game,
  +0x380 the state (0 race, 1 finished, 2 results, 3 out), +0x3a4 the collider,
  +0x3a8 the sun texture, +0x3ac, +0x3b0 two helix trails, +0x3b4 the simple model is
  drawn, +0x3b8 finished, +0x3c0 the control-change counter
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"

#define PL_I32(off) RA_I32(self, off)
#define PL_F32(off) RA_F32(self, off)
#define PL_U8(off) RA_U8(self, off)
#define PL_PTR(off) RA_PTRF(self, off)

#define g_RowGameFlags RW_VAR(unsigned int, 0x0030f0b0)
#define g_RowWorld RW_VAR(void *, 0x0030f12c)
#define g_RowWater RW_VAR(void *, 0x0030f130)
#define g_RowPlayer(i) RW_VAR(void *, 0x0030f490 + (i) * 4)
#define RA_VARIANT ((g_RowGameFlags >> 2) & 3)
#define g_GameState RW_VAR(int, 0x0030f0b4)

extern int DAT_002baebc;   /* core: team game */
extern int DAT_002baef4[]; /* core: per player record [i * 10]: team */
extern int DAT_00293928;   /* classlib_ui: the voice volume */

/* the character of player INDEX (0-5) */
#define RA_CHARACTER(index) DAT_002baed4[(index) * 10]

/* 2 pi, fixed-point wrap of an angle to [0, 2 pi) as the original does (24 bits) */
static float ra_WrapAngle(float a)
{
	unsigned int n = (unsigned int)(long long)(a * 0.159154937f * 16777216.0f) & 0xffffff;

	return (float)(int)n * 5.96046448e-08f * 6.28318548f;
}

/* 0x001a7ac0  Rowing/CRowing_Player.cpp
Loads the shared "cone boned.obe" and its instance once (0x0030f4ac, 0x0030f4b0). */
int FUN_001a7ac0(void *package)
{
	char name[16];

	if (RW_VAR(void *, 0x0030f4ac) != NULL)
		return 1;
	strcpy(name, "cone boned.obe");
	RW_VAR(void *, 0x0030f4ac) = bLoadResource((struct bkPackage *)package, name, RA_RES_ACTOR, 0xdefa);
	if (RW_VAR(void *, 0x0030f4ac) == NULL) {
		bkPrintf("Error: Could not load [%s]\n", name);
		return 0;
	}
	RW_VAR(void *, 0x0030f4b0) = baInitActorInstance((struct BActorInstance *)RW_VAR(void *, 0x0030f4b0),
		(struct BActor *)RW_VAR(void *, 0x0030f4ac), 0, RA_FILE_PLAYER, 0x17d9);
	if (RW_VAR(void *, 0x0030f4b0) == NULL) {
		bkPrintf("Error: Could not get an actor instance for [%s]\n", name);
		return 0;
	}
	RA_F32(RW_VAR(void *, 0x0030f4b0), 0xd0) = 0.55000001f;
	RA_F32(RW_VAR(void *, 0x0030f4b0), 0xd4) = 0.55000001f;
	RA_F32(RW_VAR(void *, 0x0030f4b0), 0xd8) = 0.55000001f;
	return 1;
}

/* 0x001a7bb0  Rowing/CRowing_Player.cpp
Did this player win the round? 1 alone in front, 0 tied in front, -1 behind. *COUNT gets
the number of leaders. Team games: the team with the higher total (2) or a draw (4). */
int FUN_001a7bb0(void *self, int *count)
{
	int best, n, i, team[2];

	if (DAT_002baebc == 0) {
		best = DAT_002baee8[0];
		for (i = 1; i < DAT_002bae94; i++) {
			if (best < DAT_002baee8[i * 10])
				best = DAT_002baee8[i * 10];
		}
		n = 0;
		for (i = 0; i < DAT_002bae94; i++) {
			if (DAT_002baee8[i * 10] == best)
				n++;
		}
		if (count != NULL)
			*count = n;
		if (DAT_002baee8[PL_I32(0x370) * 10] != best)
			return -1;
		return n == 1;
	}
	team[0] = 0;
	team[1] = 0;
	if (DAT_002bae94 > 0) {
		for (i = 0; i < DAT_002bae94; i++)
			team[DAT_002baef4[i * 10]] += DAT_002baee8[i * 10];
		if (team[0] != team[1]) {
			if (DAT_002baef4[PL_I32(0x370) * 10] == (team[0] < team[1])) {
				if (count != NULL)
					*count = 2;
				return 1;
			}
			if (count == NULL)
				return -1;
			*count = 2;
			return -1;
		}
	}
	if (count != NULL)
		*count = 4;
	return 0;
}

/* 0x001a7cf0  Rowing/CRowing_Player.cpp
Puts the boat back at POS facing HEADING: a fresh body, speeds and deformations cleared,
the collider on again. */
void FUN_001a7cf0(void *self, float *pos, float heading)
{
	float p[4], rot[4];

	PL_I32(0x10c) = 0;
	p[0] = pos[0];
	p[1] = pos[1];
	p[3] = pos[3];
	p[2] = pos[2];
	rot[0] = 0.0f;
	rot[1] = heading;
	rot[2] = 0.0f;
	rot[3] = 0.0f;
	bmQuaternionRotationY(RA_FP(PL_PTR(0x358), 0xe0), heading);
	FUN_001a5780(RA_P(self, 0x2a8));
	FUN_001a59f0(RA_P(self, 0x2a8), PL_PTR(0xc), p, rot, 0);
	PL_I32(0x198) = 0;
	PL_I32(0x19c) = 0;
	PL_I32(0x1a0) = 0;
	PL_I32(0x1a4) = 0;
	PL_U8(0x37c) = 1;
	PL_I32(0x150) = 0;
	PL_U8(0x154) = 0;
	PL_I32(0x158) = -1;
	PL_I32(0x15c) = 0;
	PL_I32(0x160) = 0;
	PL_I32(0x138) = 0;
	PL_I32(0x13c) = 0;
	PL_I32(0xd0) = 0;
	PL_I32(0xd4) = 0;
	PL_I32(0xd8) = 0;
	FUN_001a5990(PL_I32(0x3a4), 1);
	FUN_001a59b0(PL_I32(0x3a4), RA_FP(self, 0x2b4), RA_FP(self, 0x304));
	FUN_001a6f20(PL_I32(0x3a4));
}

/* 0x001a7e10  Rowing/CRowing_Player.cpp
The boat's position. */
void FUN_001a7e10(void *self, float *out)
{
	out[0] = PL_F32(0x2b4);
	out[1] = PL_F32(0x2b8);
	out[2] = PL_F32(0x2bc);
	out[3] = PL_F32(0x2c0);
}

/* a hull deformation hit with free fraction T (1: nothing in reach): the dent deepens,
pushes the boat away from the wall and wobbles with the speed along it (DVX, DVZ: the
speed relative to the obstacle). Returns 1 when the dent got deeper. */
static int ra_Dent(void *self, float *slot, float t, float dvx, float dvz)
{
	float f, d, a, speed, rel, wobble;

	if (!(t >= 0.800755918f))
		f = 0.0f;
	else if (t == 1.0f)
		f = 1.0f;
	else
		f = (t - 0.800755918f) * 5.01896906f;
	d = 1.0f - f;
	if (d <= slot[3])
		return 0;
	slot[3] = d;
	a = slot[0] - 3.1415925f;
	slot[4] = sinf(a - PL_F32(0x2c8)) * d * PL_F32(0x2f8) * 10.0f;
	slot[5] = cosf(a - PL_F32(0x2c8)) * d * PL_F32(0x2f8) * 10.0f;
	speed = sqrtf(dvz * dvz + dvx * dvx);
	rel = ra_WrapAngle(-PL_F32(0x2c8) - 3.1415925f);
	rel = ra_WrapAngle(rel - ra_WrapAngle(atan2f(dvx, dvz)));
	if (rel > 3.1415925f)
		rel = -(6.28318548f - rel);
	rel = ra_WrapAngle(rel + slot[0]);
	if (rel > 3.1415925f)
		rel = -(6.28318548f - rel);
	wobble = d * sinf(rel) * speed * 0.0049999999f + slot[6];
	slot[6] = wobble;
	slot[6] = PL_F32(0x300) * 0.050000001f + wobble;
	return 1;
}

/* 0x001a7e40  Rowing/CRowing_Player.cpp
The eight hull deformations against the course walls: a ray of 70.44 from the boat in
each slot's direction. Returns 1 when a dent got deeper. */
int FUN_001a7e40(void *self)
{
	void *world = g_RowWorld;
	float ray[4], seg[4], t, minX, maxX, minZ, maxZ, a, *slot, *pa, *pb;
	struct BHit2D hit;
	unsigned char *outline;
	int k, i, n, changed;

	changed = 0;
	hit.flags = 1;
	slot = RA_FP(self, 0x1a8);
	for (k = 0; k < 8; k++, slot += 8) {
		ray[0] = PL_F32(0x2b4);
		ray[1] = PL_F32(0x2bc);
		a = (slot[0] - 3.1415925f) - PL_F32(0x2c8);
		ray[2] = sinf(a) * 70.4410019f + ray[0];
		ray[3] = cosf(a) * 70.4410019f + ray[1];
		minX = ray[2];
		maxX = ray[0];
		if (ray[0] <= ray[2]) {
			minX = ray[0];
			maxX = ray[2];
		}
		minZ = ray[3];
		maxZ = ray[1];
		if (ray[1] <= ray[3]) {
			minZ = ray[1];
			maxZ = ray[3];
		}
		t = 1.0f;
		for (i = 0; i < RA_I32(world, 0x134); i++) {
			outline = (unsigned char *)RA_PTRF(world, 0x138) + i * 0xc;
			if (RA_U8(outline, 8) == 0) {
				pa = (float *)RA_PTRF(outline, 4);
				n = RA_I32(outline, 0) - 2;
				pb = pa + 2;
			} else {
				pb = (float *)RA_PTRF(outline, 4);
				n = RA_I32(outline, 0) - 1;
				pa = pb + RA_I32(outline, 0) * 2 - 2;
			}
			for (; n >= 0; n--, pa = pb, pb += 2) {
				if ((minX <= pa[0] || minX <= pb[0]) && (pa[0] <= maxX || pb[0] <= maxX) &&
						(minZ <= pa[1] || minZ <= pb[1]) && (pa[1] <= maxZ || pb[1] <= maxZ)) {
					seg[0] = pa[0];
					seg[1] = pa[1];
					seg[2] = pb[0];
					seg[3] = pb[1];
					if (bmSegmentSegment(ray, seg, &hit) != 0 && hit.t < t)
						t = hit.t;
				}
			}
		}
		if (ra_Dent(self, slot, t, PL_F32(0x304), PL_F32(0x30c)))
			changed = 1;
	}
	return changed;
}

/* 0x001a8260  Rowing/CRowing_Player.cpp
The eight hull deformations against the other boats and objects (the colliders, a ray of
62.44); in variant 4 a touch of the waterfall's lip (6) shakes the boat, of the bottom (9)
throws it back. Returns 1 when a dent got deeper. */
int FUN_001a8260(void *self)
{
	float ray[4], t, impulse[4], a, *slot;
	int k, changed, touch;

	changed = 0;
	slot = RA_FP(self, 0x1a8);
	for (k = 0; k < 8; k++, slot += 8) {
		ray[0] = PL_F32(0x2b4);
		ray[1] = PL_F32(0x2bc);
		a = (slot[0] - 3.1415925f) - PL_F32(0x2c8);
		t = 1.0f;
		ray[2] = sinf(a) * 62.4410019f + ray[0];
		ray[3] = cosf(a) * 62.4410019f + ray[1];
		FUN_001a6bf0(self, ray, &t, impulse, PL_I32(0x3a4));
		if (ra_Dent(self, slot, t, PL_F32(0x304) - impulse[0], PL_F32(0x30c) - impulse[2]))
			changed = 1;
	}
	if (PL_F32(0x3c4) == 0.0f && (g_RowGameFlags & 0xc) == 0xc) {
		touch = RowFloaty_CheckTouch(RA_FP(self, 0x2b4));
		if (touch == 6) {
			PL_I32(0x3c4) = RW_VAR(int, 0x002b5678);
			PL_I32(0x3c8) = 0;
			PL_I32(0x3cc) = 0;
			PL_I32(0x3d0) = 0;
			PL_I32(0x3d4) = 0;
			PL_I32(0x3d8) = 0;
			baSetRenderPass((struct BActorInstance *)PL_PTR(0xc), 1, 5, (int)(uintptr_t)RA_P(self, 0x3c8));
			CLSamples_PlayVoice((char)RA_CHARACTER(PL_I32(0x370)), 8, RA_FP(self, 0x2b4), DAT_00293928, 180.0f,
				0, -1, 0);
		} else if (touch == 9) {
			PL_F32(0x308) = 80.0f;
			PL_F32(0x30c) = PL_F32(0x30c) - 20.0f;
			if (PL_F32(0x30c) > -10.0f)
				PL_F32(0x30c) = -10.0f;
			CLSamples_PlayVoice((char)RA_CHARACTER(PL_I32(0x370)), 0xe, RA_FP(self, 0x2b4), DAT_00293928, 180.0f,
				0, -1, 0);
			return changed;
		}
	}
	return changed;
}

/* 0x001a8600  Rowing/CRowing_Player.cpp
The dents push the boat: each deformation's push divided by the mass leaves the speed. */
void FUN_001a8600(void *self)
{
	float k;
	int i;

	for (i = 0; i < 8; i++) {
		k = 1.0f / PL_F32(0x2fc);
		PL_F32(0x304) = PL_F32(0x304) - k * PL_F32(0x1b8 + i * 0x20);
		PL_F32(0x30c) = PL_F32(0x30c) - k * PL_F32(0x1bc + i * 0x20);
	}
}

/* 0x001a8810  Rowing (gap; CRowing_Player.cpp inferred)
The driver's animation callback (bactor): when an animation ends (DONE == 0) the next
one starts (FUN_001a75c0). */
int FUN_001a8810(int done, int a2, int a3, int a4, void *player)
{
	(void)a2;
	(void)a3;
	(void)a4;
	if (done == 0)
		FUN_001a75c0(player);
	return 1;
}

/* 0x001a8830  Rowing/CRowing_Player.cpp
Plays the driver's animation ANIM (0: none; 15 victory falls back to winstart without
one), blending in over BLEND (0: at once). The bumps (11-14) remember the animation they
interrupt. */
void FUN_001a8830(void *self, int anim, float blend)
{
	void *inst;

	if (anim == 0) {
		PL_I32(0x98) = 0;
		PL_I32(0x90) = 0;
		PL_I32(0x9c) = 1;
		return;
	}
	if (anim == 0xf && PL_I32(0x84) == 0)
		anim = 2;
	if (PL_PTR(0x48 + anim * 4) == NULL)
		return;
	if (anim < 0xb || anim > 0xe)
		PL_I32(0x94) = 0;
	else
		PL_I32(0x94) = PL_I32(0x90);
	inst = PL_PTR(0xc4);
	if (blend == 0.0f) {
		baQueueAnimation((struct BActorInstance *)inst, (struct BActorNode *)RA_PTRF(inst, 0x134),
			(struct BAnim *)PL_PTR(0x48 + anim * 4), 0x400, -1, 0, (int (*)())FUN_001a8810, self);
		PL_I32(0x90) = anim;
		PL_I32(0x9c) = 0;
		return;
	}
	baFlushAnimations(inst, (struct BActorNode *)RA_PTRF(inst, 0x134), 1, 0);
	baQueueAnimationBlendIn((struct BActorInstance *)PL_PTR(0xc4),
		(struct BActorNode *)RA_PTRF(PL_PTR(0xc4), 0x134), (struct BAnim *)PL_PTR(0x48 + anim * 4), 0x400, -1, blend,
		0, (int (*)())FUN_001a8810, self);
	PL_I32(0x90) = anim;
	PL_I32(0x9c) = 0;
}

/* 0x001a8940  Rowing/CRowing_Player.cpp
State changes: 0 the race starts (a turbo start when the button was pressed within 15
ticks), 1 finished (variant 2: the open gates count as missed; variants 1 and 2: the
sparkles when it came first), 2 the results (variants 3 and 4: the boat is moved to its podium place),
3 out, 4 the engine sound, 5 and 6 the results animations. */
void FUN_001a8940(void *self, int mode)
{
	float t, a, pos[4];
	int i, total, ours;
	unsigned char *gate;

	switch (mode) {
	case 0:
		PL_I32(0x380) = 1;
		PL_I32(0x9c) = 1;
		PL_I32(0x384) = 0;
		PL_I32(0x388) = 0;
		PL_U8(0x34b) = 0;
		PL_I32(0x98) = 0;
		PL_I32(0x90) = 0;
		t = RW_VAR(float, 0x0030f0bc) - PL_F32(0xc0);
		if (PL_I32(0x370) == 0)
			bkPrintf(" turbo test %2.2f\n", (double)(t * 0.13333334f));
		if (t < 15.0f && DAT_002baeb8 == 0) {
			bkPrintf("TURBO!\n");
			a = 6.28318548f - PL_F32(0x2c8);
			PL_F32(0x30c) = cosf(a) * -8.0f;
			PL_F32(0x304) = sinf(a) * -8.0f;
			RowHelix_Flash((struct RowHelix *)PL_PTR(0x3ac));
			RowHelix_Flash((struct RowHelix *)PL_PTR(0x3b0));
			CLSamples_PlayVoice((char)RA_CHARACTER(PL_I32(0x370)), 8, RA_FP(self, 0x2b4), DAT_00293928, 180.0f, 0,
				-1, 0);
		}
		break;
	case 1:
		PL_I32(0x380) = 0;
		PL_I32(0x384) = 0;
		PL_I32(0x388) = 0;
		PL_U8(0x34b) = 0;
		if ((g_RowGameFlags & 0xc) == 4) {
			for (i = 0; i < RA_I32(g_RowWorld, 0x19c); i++) {
				gate = (unsigned char *)RA_P(self, 0x110 + i);
				if (*gate == 0)
					*gate = 2;
			}
			FUN_001a7540(self);
		}
		if (RA_VARIANT == 0 || RA_VARIANT == 1)
			FUN_0019d9b0(RA_GPTR(void, 0x0030f0b0), PL_I32(0x370), PL_I32(0xe8));
		break;
	case 2:
		if (RA_VARIANT == 2) {
			/* line up behind the finish, side by side in finishing order */
			ours = 0;
			total = 0;
			for (i = 0; i < 4; i++) {
				if (RA_I32(g_RowPlayer(i), 0x3b8) != 0) {
					if (i == PL_I32(0x370))
						ours = total;
					total++;
				}
			}
			pos[0] = RA_F32(g_RowWorld, 0xd4);
			pos[2] = RA_F32(g_RowWorld, 0xdc);
			pos[3] = RA_F32(g_RowWorld, 0xe0);
			bkPrintf("total %d, ours %d\n", total, ours);
			pos[0] = ((float)ours - (float)(total - 1) * 0.5f) * 150.0f + pos[0];
			pos[1] = FUN_001a4b10(g_RowWater, pos[0], pos[2]);
			FUN_001a7cf0(self, pos, RA_F32(g_RowWorld, 0xe4) + 3.1415925f);
		} else if (RA_VARIANT == 3) {
			i = PL_I32(0x378);
			pos[0] = RA_F32(g_RowWorld, 0x84 + i * 0x14);
			pos[2] = RA_F32(g_RowWorld, 0x8c + i * 0x14) + 500.0f;
			pos[1] = RA_F32(g_RowWorld, 0x88 + i * 0x14);
			pos[3] = RA_F32(g_RowWorld, 0x90 + i * 0x14);
			FUN_001a7cf0(self, pos, RA_F32(g_RowWorld, 0x94 + i * 0x14) + 3.1415925f);
		}
		PL_I32(0x380) = 2;
		PL_I32(0x384) = 0;
		PL_I32(0x388) = 0;
		PL_U8(0x34b) = 1;
		FUN_001a8830(self, 0xf, 0.2f);
		break;
	case 3:
		PL_I32(0x380) = 3;
		PL_I32(0x384) = 0;
		PL_I32(0x388) = 0;
		PL_U8(0x34b) = 0;
		FUN_001a8830(self, 5, 0.2f);
		break;
	case 4:
		PL_I32(0xb4) = rwb_Sound_PlayAt(0x14, RA_FP(self, 0x2b4), 0x80);
		rwb_Sound_Track(g_RowSound, PL_I32(0xb4));
		break;
	case 5:
		if (PL_I32(0x380) == 2)
			FUN_001a8830(self, 2, 0.2f);
		break;
	case 6:
		if (PL_I32(0x380) == 2)
			FUN_001a8830(self, 0xf, 0.2f);
		break;
	}
}

/* the AI tweaks by difficulty (DAT_002bae70: 0 easy, 1 medium, 2 hard), see the "EZ:/MD:/HD:"
   tweak menu in Rowing/CRowing_Game.c; the hard ConcernAng lives in .bss at 0x0030f4a8 */
#define AI_CONCERN(d) RW_VAR(float, (d) == 2 ? 0x0030f4a8 : 0x002b5724 + (d) * 4)
#define AI_REVERSE(d) RW_VAR(float, 0x002b572c + (d) * 4)
#define AI_SPEED(d) RW_VAR(float, 0x002b5738 + (d) * 4)
#define AI_DELTASPD(d) RW_VAR(float, 0x002b5744 + (d) * 4)
#define AI_EASEOFFDIST RW_VAR(float, 0x002b5720)

/* 0x001a8d30  Rowing/CRowing_Player.cpp (bracketed)
The computer driver. Picks ConcernAng / ReverseAng / Speed / DeltaSpd by difficulty and
adjusts them by variant; aims at the point (+0x140) from the boat's position 10 frames ahead
and turns the angle to it into the two stick values (+0x138 right, +0x13c left; a turn
behind the boat on medium/hard reverses). Eases off near the target, halves in variant 3
when no human is still in the game (x1.2 on hard when +0x3dc), and moves each stick at
most DeltaSpd * DT. Output: the sticks as -32768 * value (*LEFTY, *RIGHTY), *LEFTX and
*RIGHTX 0. */
void FUN_001a8d30(void *self, float dt, int *leftX, int *leftY, int *rightX, int *rightY)
{
	int diff = DAT_002bae70, variant, reverse, ease, i;
	int canReverse;
	float rev, concern, speed, delta;
	float x, y, z, dist, a, right, left, f;

	if (PL_F32(0x16c) > 0.0f) {
		PL_F32(0x16c) -= dt;
		if (PL_F32(0x16c) <= 0.0f)
			PL_F32(0x16c) = 0.0f;
	}
	canReverse = diff != 0;
	rev = AI_REVERSE(diff == 0 ? 0 : diff == 2 ? 2 : 1);
	concern = AI_CONCERN(diff == 0 ? 0 : diff == 2 ? 2 : 1);
	speed = AI_SPEED(diff == 0 ? 0 : diff == 2 ? 2 : 1);
	delta = AI_DELTASPD(diff == 0 ? 0 : diff == 2 ? 2 : 1);

	variant = RA_VARIANT;
	if (variant == 1) {
		if (diff == 1) {
			rev -= (AI_REVERSE(1) - AI_REVERSE(0)) * 0.1f;
			concern -= (AI_CONCERN(1) - AI_CONCERN(0)) * 0.1f;
			delta -= (AI_DELTASPD(1) - AI_DELTASPD(0)) * 0.1f;
			speed -= (AI_SPEED(1) - AI_SPEED(0)) * 0.1f;
		} else if (diff == 2) {
			rev = (rev - (AI_REVERSE(2) - AI_REVERSE(1)) * 0.1f) * 0.25f;
			concern = (concern - (AI_CONCERN(2) - AI_CONCERN(1)) * 0.1f) * 0.5f;
			delta = 2.0f - (2.0f - (delta - (AI_DELTASPD(2) - AI_DELTASPD(1)) * 0.1f)) * 0.5f;
			speed = 1.0f - (1.0f - (speed - (AI_SPEED(2) - AI_SPEED(1)) * 0.1f)) * 0.5f;
		} else if (diff == 0) {
			rev *= 0.5f;
			delta = 2.0f - (2.0f - delta) * 0.5f;
		}
	}
	if (variant == 0) {
		if (diff == 1) {
			rev += (AI_REVERSE(2) - AI_REVERSE(1)) * 0.5f;
			concern += (AI_CONCERN(2) - AI_CONCERN(1)) * 0.5f;
			delta += (AI_DELTASPD(2) - AI_DELTASPD(1)) * 0.5f;
			speed += (AI_SPEED(2) - AI_SPEED(1)) * 0.9f;
		} else if (diff == 2) {
			rev -= (AI_REVERSE(2) - AI_REVERSE(1)) * 0.1f;
			concern -= (AI_CONCERN(2) - AI_CONCERN(1)) * 0.1f;
			delta -= (AI_DELTASPD(2) - AI_DELTASPD(1)) * 0.1f;
			speed -= (AI_SPEED(2) - AI_SPEED(1)) * 0.1f;
		}
	}
	if (variant == 3) {
		speed = 1.0f;
		if (diff == 2) {
			rev *= 0.25f;
			concern *= 0.5f;
			delta = 2.0f;
		} else if (diff == 1) {
			rev *= 0.25f;
			concern *= 0.5f;
			delta = 2.0f - (2.0f - delta) * 0.5f;
		} else if (diff == 0) {
			rev *= 0.5f;
			delta = 2.0f - (2.0f - delta) * 0.5f;
		}
	}
	if (variant == 2 && diff == 0)
		delta *= 0.5f;

	/* the target, seen from 10 frames ahead */
	x = PL_F32(0x304) * 10.0f + (PL_F32(0x2b4) - PL_F32(0x140));
	y = PL_F32(0x308) * 10.0f + (PL_F32(0x2b8) - PL_F32(0x144));
	z = PL_F32(0x30c) * 10.0f + (PL_F32(0x2bc) - PL_F32(0x148));
	dist = sqrtf(z * z + y * y + x * x);
	a = atan2f(-x, z) - PL_F32(0x2c8);
	while (a >= 3.1415925f)
		a -= 6.28318548f;
	while (a < -3.1415925f)
		a += 6.28318548f;
	reverse = 0;
	if (canReverse && (a > 2.09230065f || a < -2.09230065f)) {
		reverse = 1;
		a = a > 0.0f ? 3.1415925f - a : -3.1415925f - a;
	}

	ease = 1;
	if (a >= 0.0f) {
		a -= concern;
		if (a < 0.0f)
			a = 0.0f;
		if (reverse) {
			right = -speed;
			if (a < rev)
				left = a / rev * (speed + 1.0f) - speed;
			else {
				left = 1.0f;
				ease = 0;
			}
		} else {
			right = speed;
			if (a < rev)
				left = speed - a / rev * (speed + 1.0f);
			else {
				left = -1.0f;
				ease = 0;
			}
		}
	} else {
		a += concern;
		if (a > 0.0f)
			a = 0.0f;
		if (reverse) {
			left = -speed;
			if (a <= -rev) {
				right = 1.0f;
				ease = 0;
			} else
				right = -1.0f / rev * a * (speed + 1.0f) - speed;
		} else {
			left = speed;
			if (a <= -rev) {
				right = -1.0f;
				ease = 0;
			} else
				right = speed - -1.0f / rev * a * (speed + 1.0f);
		}
	}
	if (variant == 3)
		ease = 0;
	if (dist < AI_EASEOFFDIST && ease) {
		f = dist * 0.5f / AI_EASEOFFDIST + 0.5f;
		right *= f;
		left *= f;
	}
	if (variant == 3) {
		for (i = 0; i < 4; i++)
			if (DAT_002baed4[i * 10 + 1] != 0 && RA_U8(g_RowPlayer(i), 0x37c) != 0)
				break;
		if (i == 4) {
			left *= 0.5f;
			right *= 0.5f;
		} else if (diff == 2 && PL_I32(0x3dc) != 0) {
			left *= 1.2f;
			right *= 1.2f;
		}
	}

	delta *= dt;
	if (right > delta + PL_F32(0x138))
		right = delta + PL_F32(0x138);
	if (right < PL_F32(0x138) - delta)
		right = PL_F32(0x138) - delta;
	if (left > delta + PL_F32(0x13c))
		left = delta + PL_F32(0x13c);
	if (left < PL_F32(0x13c) - delta)
		left = PL_F32(0x13c) - delta;
	PL_F32(0x138) = right;
	PL_F32(0x13c) = left;
	*leftX = 0;
	*rightX = 0;
	*leftY = (int)(left * -32768.0f);
	*rightY = (int)(right * -32768.0f);
}

/* 0x001a9480  Rowing/CRowing_Player.cpp (bracketed)
Picks the computer driver's target on PATH ({int count; float (*pts)[2] (x, z); char closed}).
Finds the path point nearest the boat (+0x140/+0x148 = that point minus the boat), then walks
forward from the next point to a look-ahead end (closed paths: table 0x002b5760 by character
and difficulty in variant 0, else 8; open paths in variant 1: table 0x002b57a8 when the end is
far, else the path's end) and takes as target (+0x140/+0x148) the last point that the boat
sees without crossing a world outline (world +0x134 outlines {count, pts, closed}). Returns
that point's index, -1 if none. */
int FUN_001a9480(void *self, void *path)
{
	void *world = g_RowWorld;
	int count = RA_I32(path, 0);
	float *pts = (float *)RA_PTRF(path, 4);
	int closed = RA_U8(path, 8);
	/* the original leaves the nearest index unset when no point is nearer than 1e8; the
	   register then holds the path pointer */
	int nearest = (int)(uintptr_t)path;
	int cur, end, look, found, hit, o, n;
	float best = 1.0e8f, dx, dz, d, seg[4], wall[4], minX, maxX, minZ, maxZ, *p, *q;
	struct BHit2D h;

	for (cur = 0; cur < count; cur++) {
		dx = pts[cur * 2] - PL_F32(0x2b4);
		dz = pts[cur * 2 + 1] - PL_F32(0x2bc);
		d = dz * dz + dx * dx;
		if (d < best) {
			best = d;
			PL_F32(0x140) = dx;
			PL_F32(0x148) = dz;
			nearest = cur;
		}
	}
	seg[0] = PL_F32(0x2b4);
	seg[1] = PL_F32(0x2bc);
	if (!closed) {
		cur = nearest;
		if (nearest != count - 1)
			cur = nearest + 1;
		end = count;
		if ((g_RowGameFlags & 0xc) == 4) {
			dx = pts[count * 2 - 2] - pts[cur * 2];
			dz = pts[count * 2 - 1] - pts[cur * 2 + 1];
			look = RW_VAR(int, 0x002b57a8 + (RA_CHARACTER(PL_I32(0x370)) + DAT_002bae70 * 6) * 4);
			if (dz * dz + dx * dx > 800.0f && count > 6 && count - cur > look) {
				end = look + cur;
				if (end == cur)
					end = cur + 1;
			}
		}
	} else {
		if ((g_RowGameFlags & 0xc) == 0)
			look = RW_VAR(int, 0x002b5760 + (RA_CHARACTER(PL_I32(0x370)) + DAT_002bae70 * 6) * 4);
		else
			look = 8;
		cur = (nearest + 1) % count;
		if (count > look)
			end = (look + cur) % count;
		else
			end = nearest;
	}

	found = -1;
	hit = 0;
	h.flags = 1;
	if (cur == end)
		return found;
	do {
		if (hit)
			return found;
		seg[2] = pts[cur * 2];
		seg[3] = pts[cur * 2 + 1];
		if (seg[0] <= seg[2]) {
			minX = seg[0];
			maxX = seg[2];
		} else {
			minX = seg[2];
			maxX = seg[0];
		}
		if (seg[1] <= seg[3]) {
			minZ = seg[1];
			maxZ = seg[3];
		} else {
			minZ = seg[3];
			maxZ = seg[1];
		}
		for (o = 0; o < RA_I32(world, 0x134) && !hit; o++) {
			void *outline = (char *)RA_PTRF(world, 0x138) + o * 0xc;

			if (RA_U8(outline, 8)) {
				n = RA_I32(outline, 0) - 1;
				q = (float *)RA_PTRF(outline, 4);
				p = q + RA_I32(outline, 0) * 2 - 2;
			} else {
				n = RA_I32(outline, 0) - 2;
				p = (float *)RA_PTRF(outline, 4);
				q = p + 2;
			}
			for (; n >= 0; n--, p = q, q += 2) {
				if (p[0] < minX && q[0] < minX)
					continue;
				if (p[0] > maxX && q[0] > maxX)
					continue;
				if (p[1] < minZ && q[1] < minZ)
					continue;
				if (p[1] > maxZ && q[1] > maxZ)
					continue;
				wall[0] = p[0];
				wall[1] = p[1];
				wall[2] = q[0];
				wall[3] = q[1];
				if (bmSegmentSegment(seg, wall, &h) != 0) {
					hit = 1;
					break;
				}
			}
		}
		if (!hit) {
			found = cur;
			PL_F32(0x140) = seg[2];
			PL_F32(0x148) = seg[3];
		}
		cur++;
		if (closed)
			cur %= count;
	} while (cur != end);
	return found;
}

/* 0x001a99f0  Rowing/CRowing_Player.cpp (bracketed)
The computer driver's target in variant 0: the course path (world +0x150) when there is one,
else player 0's boat. */
void FUN_001a99f0(void *self)
{
	void *path = RA_PTRF(g_RowWorld, 0x150);
	float *p;

	if (path != NULL) {
		FUN_001a9480(self, path);
		return;
	}
	p = RA_FP(g_RowPlayer(0), 0x2b4);
	PL_F32(0x140) = p[0];
	PL_F32(0x144) = p[1];
	PL_F32(0x148) = p[2];
	PL_F32(0x14c) = p[3];
}

/* 0x001a9a40  Rowing/CRowing_Player.cpp (bracketed)
Measures the point (X, Z) against the gate GATE (+4: its two posts (x, z)). OUT: [0] the
distance along the gate from its centre, [1] across it, [2] the gate's width, [3], [4] its
centre, [5], [6] its direction, [7], [8] its normal (-dir z, dir x). */
void FUN_001a9a40(float x, float z, void *gate, float *out)
{
	float *posts = (float *)RA_PTRF(gate, 4);
	float dx = posts[2] - posts[0], dz = posts[3] - posts[1];
	float len = sqrtf(dz * dz + dx * dx), inv = 1.0f / len;

	out[5] = inv * dx;
	out[6] = inv * dz;
	out[2] = len;
	out[8] = out[5];
	out[7] = -out[6];
	x -= posts[0];
	z -= posts[1];
	out[0] = (x * out[5] + z * out[6]) - len * 0.5f;
	out[1] = x * out[7] + z * out[8];
	out[3] = (posts[2] + posts[0]) * 0.5f;
	out[4] = (posts[3] + posts[1]) * 0.5f;
}

/* 0x001a9af0  Rowing/CRowing_Player.cpp (bracketed)
The computer driver's target in variant 2 (the gates, world +0x19c/+0x1a0; the finish
+0x128 once all are passed): a point in front of or beyond the next gate, steering round it
when the boat is past or outside it; then, for each gate close to the boat (within ~135),
a point that leads round that gate's post. */
void FUN_001a9af0(void *self)
{
	void *gate;
	float g[9], fwd, side, h;
	int i, n, right;

	PL_F32(0x140) = 0.0f;
	PL_F32(0x144) = 0.0f;
	PL_F32(0x148) = 0.0f;
	n = RA_I32(g_RowWorld, 0x19c);
	for (i = 0; i < n; i++)
		if (PL_U8(0x110 + i) == 0)
			break;
	if (i == n)
		gate = (char *)g_RowWorld + 0x128;
	else
		gate = (char *)RA_PTRF(g_RowWorld, 0x1a0) + i * 0xc;
	FUN_001a9a40(PL_F32(0x2b4), PL_F32(0x2bc), gate, g);
	if (g[1] > 0.0f) {
		/* past the gate: come round to its front */
		h = g[2] * -0.5f;
		if (g[0] <= h - 100.0f || g[2] * 0.5f + 100.0f <= g[0]) {
			fwd = -350.0f;
			side = g[0] < 0.0f ? h - 80.0f : g[2] * 0.5f + 80.0f;
		} else {
			fwd = 100.0f;
			side = g[0] < 0.0f ? h - 120.0f : g[2] * 0.5f + 120.0f;
		}
	} else {
		h = g[2] * 0.5f;
		fwd = 500.0f;
		if (g[0] <= 80.0f - h || h - 80.0f <= g[0]) {
			/* outside the posts: aim inside them first when the gate is still near */
			if (g[0] < 0.0f) {
				g[0] -= 80.0f - h;
				if (g[0] <= g[1] * 0.2f)
					fwd = -165.0f;
			} else {
				g[0] -= h + 80.0f;
				if (g[1] * -0.2f <= g[0])
					fwd = -165.0f;
			}
		}
		side = fwd == -165.0f ? 0.0f : -g[0];
	}
	PL_F32(0x140) = g[7] * fwd + g[5] * side + g[3];
	PL_F32(0x148) = g[8] * fwd + g[6] * side + g[4];

	for (i = 0; i < RA_I32(g_RowWorld, 0x19c); i++) {
		FUN_001a9a40(PL_F32(0x2b4), PL_F32(0x2bc), (char *)RA_PTRF(g_RowWorld, 0x1a0) + i * 0xc, g);
		h = g[2] * 0.5f;
		right = g[0] >= 0.0f;
		if (right)
			g[0] -= h;
		else
			g[0] += h;
		if (g[0] * g[0] + g[1] * g[1] < 18193.1543f) {
			if (right) {
				side = g[1] * 1.5f + h;
				fwd = -1.5f;
			} else {
				side = g[1] * -1.5f - h;
				fwd = 1.5f;
			}
			PL_F32(0x140) = g[7] * g[0] * fwd + g[5] * side + g[3];
			PL_F32(0x148) = g[8] * g[0] * fwd + g[6] * side + g[4];
		}
	}
}

/* 0x001a9e60  Rowing/CRowing_Player.cpp (bracketed)
The computer driver's target in variant 3 (the whirlpool): a point on its circle a quarter
turn ahead (the radius from table 0x002b57f0 by difficulty: {min, radius below min, add});
every 5000 time units (+0x160, run down by the clock 0x0030f148 +0x98) it picks the nearest
token (+0x15c, CToken_FindNearest) and, while it has one, aims at it unless it is more than
pi/8 ahead. */
void FUN_001a9e60(void *self)
{
	float a, dist, radius, target, pos[3];
	float *tab = RW_ADDR(0x002b57f0 + DAT_002bae70 * 0xc);
	void *token;

	a = atan2f(PL_F32(0x2bc), PL_F32(0x2b4));
	dist = sqrtf(PL_F32(0x2bc) * PL_F32(0x2bc) + PL_F32(0x2b4) * PL_F32(0x2b4));
	if (dist >= tab[0])
		dist += tab[2];
	else
		dist = tab[1];
	token = PL_PTR(0x15c);
	pos[1] = 0.0f;
	pos[0] = cosf(a + 1.57079625f) * dist;
	pos[2] = sinf(a + 1.57079625f) * dist;
	if (token == NULL) {
		PL_PTR(0x15c) = NULL;
		PL_F32(0x160) = 0.0f;
	} else {
		PL_F32(0x160) -= RA_F32(RW_VAR(void *, 0x0030f148), 0x98);
		if (PL_F32(0x160) >= 0.0f) {
			target = atan2f(RA_F32(token, 0x188), RA_F32(token, 0x180));
			while (target < a)
				target += 6.28318501f;
			radius = sqrtf(RA_F32(token, 0x188) * RA_F32(token, 0x188) +
				RA_F32(token, 0x180) * RA_F32(token, 0x180));
			if (target - a > 0.392699063f)
				target = a + 0.392699063f;
			goto aim;
		}
	}
	PL_F32(0x160) = 5000.0f;
	PL_PTR(0x15c) = CToken_FindNearest(pos, 100000.0f, 3, 1);
	target = a + 0.785398126f;
	radius = 1000.0f;
aim:
	PL_F32(0x144) = 0.0f;
	PL_F32(0x140) = cosf(target) * radius;
	PL_F32(0x148) = sinf(target) * radius;
}

/* the course of variant 1: its centre x (0x002b5634) and width (0x002b5630) */
#define RA_LANE_WIDTH RW_VAR(float, 0x002b5630)
#define RA_LANE_CENTRE RW_VAR(float, 0x002b5634)

/* a player that the computer driver can ram: in the game, not fallen, not sunk, at least
   200 behind SELF and within 60 sideways; returns its score (further behind and more in
   line is better), or 0 when it is not a candidate */
static int ra_RamCandidate(void *self, void *other, float *score)
{
	float dx;

	if (RA_U8(other, 0x37c) == 0 || RA_U8(other, 0x34a) != 0 || RA_U8(other, 0x349) != 0)
		return 0;
	if (RA_F32(other, 0x2bc) + 200.0f > PL_F32(0x2bc))
		return 0;
	dx = PL_F32(0x2b4) - RA_F32(other, 0x2b4);
	if (dx < -60.0f || dx > 60.0f)
		return 0;
	if (dx < 0.0f)
		dx = -dx;
	*score = (PL_F32(0x2bc) - RA_F32(other, 0x2bc)) + (120.0f - dx) * 10.0f;
	return 1;
}

/* 0x001aa000  Rowing/CRowing_Player.cpp (bracketed)
The computer driver's target in variant 1 (the straight course; z is forward). A mood
(+0x3dc: 0 weave, 1 race, 2 overtake the leader, 3 follow the leader) changes when its
timer +0x3e0 (run down by DT / 60 once the race is on) ends. Racing, it keeps to a lane
pushed by the floaties (RowFloaty_SumPull / AddPull) and, every 240 units (+0x16c), picks a
boat just behind to ram (+0x168) and aims at it while it stays in reach. */
void FUN_001aa000(void *self, float dt)
{
	float *pos = RA_FP(self, 0x2b4);
	float best, v, s, off, lo, hi, z, dx;
	void *leader, *p;
	int i, open, dead, ram;

	if (PL_I32(0x380) != 0)
		PL_F32(0x3e0) -= dt * 0.0166666675f;
	if (PL_F32(0x3e0) < 0.0f) {
		switch (PL_I32(0x3dc)) {
		case 0:
			PL_I32(0x3dc) = 1;
			PL_F32(0x3e0) = Core_RandUnit() * 10.0f + 5.0f;
			break;
		case 1:
			if (PL_F32(0x16c) > 0.0f) {
				PL_F32(0x3e0) = 0.0f;
				break;
			}
			switch (Core_Rand() & 3) {
			case 0:
				PL_I32(0x3dc) = 2;
				PL_F32(0x3e0) = Core_RandUnit() * 5.0f + 5.0f;
				break;
			case 1:
			case 2:
				PL_I32(0x3dc) = 1;
				PL_F32(0x3e0) = Core_RandUnit() * 5.0f + 5.0f;
				break;
			case 3:
				PL_I32(0x3dc) = 0;
				v = Core_RandUnit();
				PL_F32(0x3e0) = v + v + 2.0f;
				break;
			}
			break;
		case 2:
		case 3:
			PL_I32(0x3dc) = 1;
			PL_F32(0x3e0) = Core_RandUnit() * 10.0f + 10.0f;
			break;
		}
	}
	if (PL_F32(0x2bc) > -200.0f && (PL_I32(0x3dc) == 0 || PL_I32(0x3dc) == 1)) {
		PL_I32(0x3dc) = 3;
		PL_F32(0x3e0) = (float)rw_rand() * 0.000152592547f + 5.0f;
	}
	PL_F32(0x164) = (float)rw_rand() * 3.05185097e-07f + PL_F32(0x164) + 0.00999999978f;
	if (PL_F32(0x164) >= 6.28318501f) {
		v = PL_F32(0x164);
		do
			v -= 6.28318501f;
		while (v >= 6.28318501f);
		PL_F32(0x164) = v;
	}
	s = sinf(PL_F32(0x164));

	/* the leader among the others that have not fallen */
	leader = NULL;
	best = -1000000.0f;
	for (i = 0; i < 4; i++) {
		p = g_RowPlayer(i);
		if (PL_I32(0x370) == i || RA_U8(p, 0x34a) != 0)
			continue;
		if (leader == NULL || RA_F32(p, 0x3e4) > best) {
			leader = p;
			best = RA_F32(p, 0x3e4);
		}
	}
	if (leader == NULL) {
		PL_I32(0x3dc) = 1;
		PL_F32(0x3e0) = (float)rw_rand() * 0.000305185094f + 5.0f;
	}

	switch (PL_I32(0x3dc)) {
	case 0: /* weave across the course */
		PL_F32(0x144) = PL_F32(0x2b8);
		PL_F32(0x140) = RA_LANE_CENTRE - RA_LANE_WIDTH * 0.5f * s;
		PL_F32(0x148) = PL_F32(0x2bc) + 1000.0f;
		return;
	case 2: /* overtake the leader on the near side */
		PL_F32(0x144) = PL_F32(0x2b8);
		PL_F32(0x148) = RA_F32(leader, 0x2bc) + 200.0f;
		if (PL_F32(0x2b4) < RA_F32(leader, 0x2b4))
			PL_F32(0x140) = RA_F32(leader, 0x2b4) - 140.0f;
		else
			PL_F32(0x140) = RA_F32(leader, 0x2b4) + 140.0f;
		if (PL_F32(0x2bc) > RA_F32(leader, 0x2bc)) {
			PL_I32(0x3dc) = 3;
			PL_F32(0x3e0) = (float)rw_rand() * 0.000152592547f + 5.0f;
		}
		return;
	case 3: /* follow the leader (overtake when it is 100 ahead) */
		if (PL_F32(0x2bc) + 100.0f < RA_F32(leader, 0x2bc)) {
			PL_I32(0x3dc) = 2;
			PL_F32(0x3e0) = (float)rw_rand() * 6.10370189e-05f + 2.0f;
		}
		PL_F32(0x140) = RA_F32(leader, 0x2b4);
		if (RA_F32(leader, 0x2b4) - PL_F32(0x2b4) > 80.0f)
			PL_F32(0x140) = PL_F32(0x2b4) + 80.0f;
		if (PL_F32(0x2b4) - PL_F32(0x140) > 80.0f)
			PL_F32(0x140) = PL_F32(0x2b4) - 80.0f;
		PL_F32(0x144) = PL_F32(0x2b8);
		PL_F32(0x148) = RA_F32(leader, 0x2bc) + 30.0f;
		return;
	}

	/* race: keep a lane, pushed by the floaties and the other boats */
	lo = (RA_LANE_CENTRE - RA_LANE_WIDTH * 0.5f) + 50.0f;
	hi = (RA_LANE_WIDTH * 0.5f + RA_LANE_CENTRE) - 50.0f;
	off = FUN_001b8fe0(pos);
	open = 0;
	if (off > -250.0f && off < 250.0f) {
		for (i = 0; i < 4; i++)
			if (i != PL_I32(0x370))
				FUN_001b8df0(&off, pos, RA_FP(g_RowPlayer(i), 0x2b4), -1);
		if (off > -250.0f && off < 250.0f)
			open = 1;
	}
	PL_F32(0x140) = off + pos[0];
	if (PL_F32(0x140) > hi)
		PL_F32(0x140) = hi;
	if (PL_F32(0x140) < lo)
		PL_F32(0x140) = lo;
	PL_F32(0x144) = 0.0f;
	z = 450.0f - fabsf(off);
	if (z < 200.0f)
		z = 200.0f;
	PL_F32(0x148) = z + PL_F32(0x2bc);

	if (PL_F32(0x16c) > 0.0f) {
		/* ramming: aim at the victim while it stays 30 behind and within 60 sideways */
		if (open) {
			p = g_RowPlayer(PL_I32(0x168));
			dead = RA_U8(p, 0x37c) == 0 || RA_U8(p, 0x34a) != 0 || RA_U8(p, 0x349) != 0;
			if (PL_F32(0x2bc) - RA_F32(p, 0x2bc) >= 30.0f && !dead) {
				dx = pos[0] - RA_F32(p, 0x2b4);
				if (dx >= -60.0f && dx <= 60.0f) {
					PL_F32(0x140) = RA_F32(p, 0x2b4);
					PL_F32(0x144) = RA_F32(p, 0x2b8);
					PL_F32(0x148) = RA_F32(p, 0x2bc);
					PL_F32(0x14c) = RA_F32(p, 0x2c0);
					PL_F32(0x148) += 35.0f;
					return;
				}
			}
		}
		PL_F32(0x16c) = 0.0f;
		return;
	}
	if (!open)
		return;
	ram = -1;
	best = 0.0f;
	for (i = 0; i < 4; i++) {
		if (PL_I32(0x370) == i)
			continue;
		if (ra_RamCandidate(self, g_RowPlayer(i), &v) && v > best) {
			ram = i;
			best = v;
		}
	}
	if (ram != -1) {
		PL_I32(0x168) = ram;
		PL_F32(0x16c) = 240.0f;
	}
}

/* 0x001aaa30  Rowing/CRowing_Player.cpp (bracketed)
The boat's flash after a hit (+0x3c4, counting down by DT; at 0 the boat goes back to render
pass 1): +0x3c8 the glow (pulsing; blinking in the last 120), +0x3cc..+0x3d8 the flash
colour, waving with +0x388 (factors and offsets from .data 0x002b5814..0x002b5840). */
void FUN_001aaa30(void *self, float dt)
{
	float glow;

	if (PL_F32(0x3c4) == 0.0f)
		return;
	PL_F32(0x3c4) -= dt;
	if (PL_F32(0x3c4) <= 0.0f) {
		PL_F32(0x3c4) = 0.0f;
		baSetRenderPass((struct BActorInstance *)PL_PTR(0xc), 1, 0, 0);
	}
	glow = sinf((float)DAT_002ba99c / RW_VAR(float, 0x002b584c)) * RW_VAR(float, 0x002b5848) +
		RW_VAR(float, 0x002b5844);
	PL_F32(0x3c8) = glow;
	if (PL_F32(0x3c4) < 120.0f)
		PL_F32(0x3c8) = (float)((int)PL_F32(0x3c4) % 20 >= 10) * glow;
	PL_I32(0x3cc) = (int)(sinf(PL_F32(0x388) / RW_VAR(float, 0x002b5840)) * RW_VAR(float, 0x002b583c) +
		RW_VAR(float, 0x002b5838));
	PL_I32(0x3d0) = (int)(sinf(PL_F32(0x388) / RW_VAR(float, 0x002b5834)) * RW_VAR(float, 0x002b5830) +
		RW_VAR(float, 0x002b582c));
	PL_I32(0x3d4) = (int)(sinf(PL_F32(0x388) / RW_VAR(float, 0x002b5828)) * RW_VAR(float, 0x002b5824) +
		RW_VAR(float, 0x002b5820));
	PL_I32(0x3d8) = (int)(sinf(PL_F32(0x388) / RW_VAR(float, 0x002b581c)) * RW_VAR(float, 0x002b5818) +
		RW_VAR(float, 0x002b5814));
}

/* .rdata 0x002758a8: the debug colour (r, g, b) of each character */
static const int ra_CharacterDebugColour[6][3] = {
	{0x2e, 0xa7, 0x24}, {0xd2, 0x00, 0x00}, {0x20, 0x71, 0xcd},
	{0xf8, 0xce, 0x00}, {0xf2, 0x82, 0x1e}, {0xec, 0x6b, 0xae},
};

#define RA_DEBUG_COLOUR(self) ra_CharacterDebugColour[RA_CHARACTER(RA_I32(self, 0x370))]

/* 0x001aab80  Rowing/CRowing_Player.cpp (bracketed)
Draws the player. The driver (+0xc4) follows the boat's instance; with a single human
playing, that human's driver is only prepared during the race states 1-5 (not drawn); else
it is drawn in detail 2 or for player 0. The boat is drawn in detail 1 or 2 or for player 0
(lit by render state 6 while flashing); otherwise (debug) its outline and hull are drawn as
circles and lines (the hull points .data 0x002b5850, turned by the heading). Then the plug
(+0x14) with render state 0xb off, and, with "AILines" (0x0030f0ac) on for a computer
player, a line to its target coloured by its mood. */
void FUN_001aab80(void *self, int detail)
{
	struct BActorInstance *boat = (struct BActorInstance *)PL_PTR(0xc);
	float pos[4], hull[9][4], *v, *src;
	int i, r, b;

	if (PL_PTR(0xc4) != NULL) {
		memcpy(RA_FP(PL_PTR(0xc4), 0xc0), RA_FP(boat, 0xc0), 0x10);
		memcpy(RA_FP(PL_PTR(0xc4), 0xe0), RA_FP(boat, 0xe0), 0x10);
		if (RW_VAR(int, 0x0030f170) != 0 && PL_I32(0x370) == RW_VAR(int, 0x0030f174) &&
			(g_GameState == 5 || g_GameState == 1 || g_GameState == 2 || g_GameState == 3 ||
				g_GameState == 4)) {
			baPrepActorInstance((struct BActorInstance *)PL_PTR(0xc4), 1);
		} else if (detail == 2 || PL_I32(0x370) == 0) {
			baPrepActorInstance((struct BActorInstance *)PL_PTR(0xc4), 1);
			baDrawActorInstance((struct BActorInstance *)PL_PTR(0xc4), 1);
		}
	}
	baPrepActorInstance(boat, 1);
	if (detail == 1 || detail == 2 || PL_I32(0x370) == 0) {
		if (PL_F32(0x3c4) != 0.0f)
			bdSetRenderState(6, 1, 0);
		baDrawActorInstance(boat, 1);
	} else {
		memcpy(pos, RA_FP(PL_PTR(0x358), 0xc0), sizeof(pos));
		FUN_001afba0(pos, 62.4410019f, RA_DEBUG_COLOUR(self)[0], RA_DEBUG_COLOUR(self)[1],
			RA_DEBUG_COLOUR(self)[2]);
		pos[1] += 15.0f;
		FUN_001afba0(pos, 47.4410019f, RA_DEBUG_COLOUR(self)[0], RA_DEBUG_COLOUR(self)[1],
			RA_DEBUG_COLOUR(self)[2]);
		pos[1] -= 30.0f;
		FUN_001afba0(pos, 47.4410019f, RA_DEBUG_COLOUR(self)[0], RA_DEBUG_COLOUR(self)[1],
			RA_DEBUG_COLOUR(self)[2]);
		for (i = 0; i < 9; i++) {
			v = hull[i];
			src = (float *)RW_ADDR(0x002b5850 + i * 0x10);
			bmVectorRotateY(v, src, -PL_F32(0x2c8));
			v[0] += RA_F32(PL_PTR(0x358), 0xc0);
			v[1] += RA_F32(PL_PTR(0x358), 0xc4);
			v[2] += RA_F32(PL_PTR(0x358), 0xc8);
		}
		FUN_001afc80(RA_FP(self, 0x2b4), hull[0], RA_DEBUG_COLOUR(self)[0], RA_DEBUG_COLOUR(self)[1],
			RA_DEBUG_COLOUR(self)[2]);
		for (i = 1; i < 9; i += 2)
			FUN_001afc80(hull[i], hull[i + 1], RA_DEBUG_COLOUR(self)[0], RA_DEBUG_COLOUR(self)[1],
				RA_DEBUG_COLOUR(self)[2]);
	}
	if (PL_PTR(0x14) != NULL) {
		bdSetRenderState(0xb, 0, 0);
		memcpy(RA_FP(PL_PTR(0x14), 0xc0), RA_FP(boat, 0xc0), 0x10);
		memcpy(RA_FP(PL_PTR(0x14), 0xe0), RA_FP(boat, 0xe0), 0x10);
		baPrepActorInstance((struct BActorInstance *)PL_PTR(0x14), 1);
		baDrawActorInstance((struct BActorInstance *)PL_PTR(0x14), 1);
		bdSetRenderState(0xb, 1, 0);
	}
	if (DAT_002baed4[PL_I32(0x370) * 10 + 1] == 0 && RW_VAR(int, 0x0030f0ac) != 0) {
		switch (PL_I32(0x3dc)) {
		case 0:
			r = 0;
			b = 0;
			break;
		case 1:
			r = 0;
			b = 0xff;
			break;
		case 2:
			r = 0xff;
			b = 0;
			break;
		default:
			r = 0xff;
			b = 0xff;
			break;
		}
		FUN_001afc80(RA_FP(self, 0x2b4), RA_FP(self, 0x140), r, 0xff, b);
	}
}

/* the boat's matrix times a bone's matrix (node +0x80); the original takes the aligned copy
   variant when all three addresses are 16-byte aligned */
static void ra_BoneMatrix(float *out, void *inst, void *node)
{
	float *b = (float *)((char *)node + 0x80);

	if ((((uintptr_t)inst | (uintptr_t)b | (uintptr_t)out) & 0xf) == 0)
		bmMatrixMultiplyCopy(out, (const float *)inst, b);
	else
		bmMatrixMultiply(out, (const float *)inst, b);
}

/* 0x001ab0c0  Rowing/CRowing_Player.cpp (bracketed)
Drives the boat from the two sticks (LX, LY, RX, RY; -32768..32767). Both Y sticks push it
forward (Speed .data 0x002b5758; x(1 + 0.1 handicap) in variant 2; in variant 0 a human
behind the leader gets up to x2), their difference spins it (+0x300, limited by 0x002b5750 /
0x002b5754), the X sticks push it sideways (x0.3) unless airborne. The two oars (+0x38c
left, +0x390 right; +0x394/+0x398 their strength) follow the sticks at a limited rate and
set the engine sound (volume and pitch), the oar and driver animation frames, and the two
helix trails (+0x3ac, +0x3b0) from the oar tips. */
void FUN_001ab0c0(void *self, int lx, int ly, int rx, int ry, float dt)
{
	const float k = 3.05175781e-05f;
	float lxk = (float)lx * k, lyk = (float)ly * k, rxk = (float)rx * k, ryk = (float)ry * k;
	float sumk = (float)(ly + ry) * k, diffk = (float)(ry - ly) * k;
	float side, thrust, spinRate, spinMax, f, spin, fwd, ang, c, s;
	float left, leftPow, right, rightPow, d, vol;
	float posL[4], posR[4], velL[4], velR[4], v[3], offL[4], offR[4], matL[16], matR[16];
	struct BActorInstance *boat = (struct BActorInstance *)PL_PTR(0xc);
	int mid, frameL, frameR, drvMid, drvL, drvR, alpha;
	unsigned int variant;

	side = 0.0f;
	if (lx > 0x14)
		side = -lxk;
	if (rx < 0x14)
		side -= rxk;
	variant = RA_VARIANT;
	if (variant == 2) {
		f = (float)PL_I32(0x374) * 0.1f + 1.0f;
		thrust = f * RW_VAR(float, 0x002b5758);
		spinRate = f * RW_VAR(float, 0x002b5750);
		spinMax = f * RW_VAR(float, 0x002b5754);
	} else {
		spinRate = RW_VAR(float, 0x002b5750);
		spinMax = RW_VAR(float, 0x002b5754);
		thrust = RW_VAR(float, 0x002b5758);
	}
	if (variant == 0 && DAT_002baed4[PL_I32(0x370) * 10 + 1] != 0) {
		/* catch-up for a human behind the leader */
		f = (RA_F32(g_RowPlayer(RA_I32(RW_VAR(void *, 0x0030f13c), 0)), 0x104) - PL_F32(0x104)) *
			RW_VAR(float, 0x002b56d4) / (float)RA_I32(g_RowWorld, 0x144);
		if (f < 0.0f)
			f = 0.0f;
		else if (f > 1.0f)
			f = 1.0f;
		thrust = (f + 1.0f) * thrust;
	}
	fwd = thrust * sumk * dt;
	side = side * RW_VAR(float, 0x002b575c) * dt;
	spin = spinRate * diffk * dt;
	if (PL_U8(0x348) == 0) {
		ang = 6.28318548f - PL_F32(0x2c8);
		c = cosf(ang);
		s = sinf(ang);
		PL_F32(0x30c) = (c * fwd + PL_F32(0x30c)) - s * side;
		PL_F32(0x304) = c * side + s * fwd + PL_F32(0x304);
		if (spin > 0.0f) {
			if (-spinMax < PL_F32(0x300)) {
				PL_F32(0x300) -= spin;
				if (PL_F32(0x300) < -spinMax)
					PL_F32(0x300) = -spinMax;
			}
		} else if (PL_F32(0x300) < spinMax) {
			PL_F32(0x300) -= spin;
			if (PL_F32(0x300) > spinMax)
				PL_F32(0x300) = spinMax;
		}
	}

	/* the oars: position (-1..1) and strength */
	if (lxk >= 0.000610351562f) {
		leftPow = lxk;
		left = (1.0f - lxk) * lyk;
	} else {
		left = lyk;
		leftPow = lyk < 0.0f ? -lyk : lyk;
	}
	if (rxk <= -0.000610351562f) {
		rightPow = -rxk;
		right = (rxk - 1.0f) * ryk;
	} else {
		right = ryk;
		rightPow = ryk < 0.0f ? -ryk : ryk;
	}
	d = dt * 0.030517578f;
	if (left > d + PL_F32(0x38c))
		left = d + PL_F32(0x38c);
	if (left < PL_F32(0x38c) - d)
		left = PL_F32(0x38c) - d;
	if (right > d + PL_F32(0x390))
		right = d + PL_F32(0x390);
	if (right < PL_F32(0x390) - d)
		right = PL_F32(0x390) - d;
	d = dt * 0.00999999978f;
	if (leftPow > d + PL_F32(0x394))
		leftPow = d + PL_F32(0x394);
	if (leftPow < PL_F32(0x394) - d)
		leftPow = PL_F32(0x394) - d;
	if (rightPow > d + PL_F32(0x398))
		rightPow = d + PL_F32(0x398);
	if (rightPow < PL_F32(0x398) - d)
		rightPow = PL_F32(0x398) - d;
	PL_F32(0x38c) = left;
	PL_F32(0x390) = right;
	PL_F32(0x394) = leftPow;
	PL_F32(0x398) = rightPow;

	if (PL_I32(0xb4) != -1) {
		vol = (rightPow + leftPow) * 0.5f;
		if (vol < 0.0f)
			vol = 0.0f;
		else if (vol > 1.0f)
			vol = 1.0f;
		bsSetChannelVolume(PL_I32(0xb4), (unsigned short)(int)(vol * 63.0f + 128.0f));
		rwb_Sound_SetPitch(PL_I32(0xb4), 0x14, vol * 0.7f + 1.0f);
	}

	if (PL_PTR(0x18) == NULL || PL_PTR(0x1c) == NULL || PL_PTR(0x20) == NULL || PL_PTR(0x24) == NULL) {
		c = cosf(PL_F32(0x2c8)) * 80.0f;
		s = sinf(PL_F32(0x2c8)) * 80.0f;
		posL[0] = c + PL_F32(0x2b4);
		posL[1] = PL_F32(0x2b8) + 30.0f;
		posL[2] = s + PL_F32(0x2bc);
		posL[3] = 1.0f;
		posR[0] = PL_F32(0x2b4) - c;
		posR[1] = posL[1];
		posR[2] = PL_F32(0x2bc) - s;
		posR[3] = 1.0f;
	} else {
		baGetNodePosition(boat, (struct BActorNode *)PL_PTR(0x18), posL);
		baGetNodePosition(boat, (struct BActorNode *)PL_PTR(0x1c), posR);
		if (PL_PTR(0x28) != NULL) {
			mid = PL_I32(0x30);
			drvMid = PL_I32(0xac);
			if (left > 0.0f) {
				frameL = (int)((float)(PL_I32(0x2c) - mid) * left) + mid;
				drvL = (int)((float)(PL_I32(0xa8) - drvMid) * left) + drvMid;
			} else {
				frameL = mid - (int)((float)(PL_I32(0x34) - mid) * left);
				drvL = drvMid - (int)((float)(PL_I32(0xb0) - drvMid) * left);
			}
			if (right > 0.0f) {
				frameR = (int)((float)(PL_I32(0x2c) - mid) * right) + mid;
				drvR = (int)((float)(PL_I32(0xa8) - drvMid) * right) + drvMid;
			} else {
				frameR = mid - (int)((float)(PL_I32(0x34) - mid) * right);
				drvR = drvMid - (int)((float)(PL_I32(0xb0) - drvMid) * right);
			}
			baSetNodeTime(boat, (struct BActorNode *)PL_PTR(0x18), frameL, 1);
			baSetNodeTime(boat, (struct BActorNode *)PL_PTR(0x1c), frameR, 1);
			baSetNodeTime(boat, (struct BActorNode *)PL_PTR(0x20), (int)((unsigned int)(frameL + frameR) >> 1), 1);
			baSetNodeTime(boat, (struct BActorNode *)PL_PTR(0x24), (int)((unsigned int)(frameL + frameR) >> 1), 1);
			if (PL_PTR(0x44) != NULL) {
				if (PL_I32(0x9c) != 0)
					baSetNodeTime((struct BActorInstance *)PL_PTR(0xc4),
						(struct BActorNode *)RA_PTRF(PL_PTR(0xc4), 0x134),
						(int)((unsigned int)(drvR + drvL) >> 1), 1);
				else if (PL_I32(0x90) != 0)
					baUpdateAnimations((struct BActorInstance *)PL_PTR(0xc4),
						(struct BActorNode *)RA_PTRF(PL_PTR(0xc4), 0x134), dt, NULL);
			}
		}
	}

	/* the trails: from 35 out and 25 up of each oar bone, moving with the oar */
	ang = left * -1.57079625f;
	v[0] = cosf(ang) * 10.0f;
	v[1] = 0.0f;
	v[2] = sinf(ang) * 10.0f;
	bmVectorRotateY(velL, v, -PL_F32(0x2c8));
	velL[3] = 1.0f;
	ang = right * -1.57079625f;
	v[0] = -cosf(ang) * 10.0f;
	v[1] = 0.0f;
	v[2] = sinf(ang) * 10.0f;
	bmVectorRotateY(velR, v, -PL_F32(0x2c8));
	velR[3] = 1.0f;
	offL[0] = 35.0f;
	offL[1] = 25.0f;
	offL[2] = 0.0f;
	offL[3] = 1.0f;
	offR[0] = -35.0f;
	offR[1] = 25.0f;
	offR[2] = 0.0f;
	offR[3] = 1.0f;
	ra_BoneMatrix(matL, boat, PL_PTR(0x18));
	ra_BoneMatrix(matR, boat, PL_PTR(0x1c));
	bmTransformVector4(posL, matL, offL);
	bmTransformVector4(posR, matR, offR);
	alpha = (int)((rightPow + leftPow) * 63.0f);
	FUN_001b2a50((struct RowHelix *)PL_PTR(0x3ac), dt, posL, velL, alpha);
	FUN_001b2a50((struct RowHelix *)PL_PTR(0x3b0), dt, posR, velR, alpha);
}

/* 0x001aba70  Rowing/CRowing_Player.cpp (bracketed)
Draws the shared cone (0x0030f4b0) at the boat's instance, additive (blend mode 1), with
the brightness at 0xff, keeping the render states. */
void FUN_001aba70(void *self)
{
	unsigned int saved[30];
	struct BActorInstance *cone = (struct BActorInstance *)RW_VAR(void *, 0x0030f4b0);

	bdSaveRenderStates(saved);
	memcpy(RA_FP(cone, 0xc0), RA_FP(PL_PTR(0x358), 0xc0), 0x10);
	memcpy(RA_FP(cone, 0xe0), RA_FP(PL_PTR(0x358), 0xe0), 0x10);
	baPrepActorInstance(cone, 1);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(1);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(7, 2, 0);
	baSetBrightness(0xff);
	baDrawActorInstance(cone, 7);
	baSetBrightness(0x100);
	bdRestoreRenderStates(saved);
}

/* 0x001abb70  Rowing/CRowing_Player.cpp (bracketed)
Switches the driver between the full (+0xc8) and the simple (+0xcc) instance (always the
full one when 0x002f26c4 is set): copies the animation state, the colours, position and
rotation over. UNUSED is not read. */
void FUN_001abb70(void *self, int simple, int unused)
{
	void *from, *to;

	(void)unused;
	if (DAT_002f26c4 != NULL)
		simple = 0;
	if (simple == PL_I32(0x3b4) || PL_PTR(0x4) == NULL)
		return;
	from = PL_PTR(0xc4);
	PL_I32(0x3b4) = simple;
	to = simple != 0 ? PL_PTR(0xc8) : PL_PTR(0xcc);
	if (to != from) {
		baCopyAnimationState((struct BActorInstance *)to, (struct BActorNode *)RA_PTRF(to, 0x134),
			(struct BActorInstance *)from, (struct BActorNode *)RA_PTRF(from, 0x134), 1);
		Core_SetCharacterColours(PL_PTR(0xc4), DAT_00283ea4);
		memcpy(RA_FP(to, 0xc0), RA_FP(from, 0xc0), 0x10);
		memcpy(RA_FP(to, 0xe0), RA_FP(from, 0xe0), 0x10);
		baPrepActorInstance((struct BActorInstance *)to, 1);
	}
	PL_PTR(0xc4) = to;
}
