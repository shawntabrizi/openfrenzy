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
