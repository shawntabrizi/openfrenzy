/*
FoxHunt/CFHunt_Throwable.c  <-  C:\BlitzParty\Game\Src\FoxHunt\CFHunt_Throwable.cpp
(module foxhunt)

The throwables (CFHunt_Throwable, 0x50 bytes, at 0x003022d4-0x003022f0): chairs, bins and
the like that a player picks up (action button), carries between its palms and throws at
the player in front of it. The constructor is tagged with this file; the target search and
the update right after it in the XBE are gap functions placed here.

  +0x08 alive (0: fell out)            +0x0c state: 0 lying, 1 held, 2 dropped, 3 thrown
  +0x10 size class (1 small, 2 big)    +0x14 the actor (a CharLib actor with collision)
  +0x18 type (the throwable's slot)    +0x28 velocity before the move, +0x38 after
  +0x48 holder (player, -1)            +0x4c splashed
*/

#include "foxhunt.h"

static const char s_ThrowableFile[] = "C:\\BlitzParty\\Game\\Src\\FoxHunt\\CFHunt_Throwable.cpp";

extern char *DAT_002ba96c;            /* core: the game (+0x18 stage) */
extern float DAT_00292c84;            /* charlib: gravity */

#define THR_ACTOR(t) FH_AP(t, 0x14)
#define STAGE() FH_AI(DAT_002ba96c, 0x18)

/* 0x00134460  CFHunt_Throwable.cpp: CFHunt_Throwable::CFHunt_Throwable: an actor of the
prop model (types 0-4 the small one, else the big one) at `pos`, turned to `heading`, with
its own collision data */
void *CFHunt_Throwable_Ctor(void *self, int type, float *pos, float heading)
{
	char *actor;
	int model;

	if (type < 0 || (model = 1, 4 < type))
		model = 2;
	actor = (char *)Actor_Create(FH_PV(0x002adf3c)[model * 2], pos, NULL, 5, 0, 1);
	THR_ACTOR(self) = actor;
	Actor_InitCharData(actor);
	FH_AI(THR_ACTOR(self), 0x220) = 0x400019;
	FH_AP(THR_ACTOR(self), 0x18c) = (char *)bkHeapCalloc(0x204, 0, s_ThrowableFile, 0x212, 0x2001);
	FH_AF(FH_AP(THR_ACTOR(self), 0x18c), 0x9c) = 60.0f;
	FH_AI(FH_AP(THR_ACTOR(self), 0x18c), 0x90) = 0;
	FH_AF(FH_AP(THR_ACTOR(self), 0x18c), 0x80) = 60.0f;
	FH_AF(FH_AP(THR_ACTOR(self), 0x18c), 0xa0) = 50.0f;
	FH_AI(FH_AP(THR_ACTOR(self), 0x18c), 0x1ac) = 0;
	FH_AB(FH_AP(THR_ACTOR(self), 0x18c), 0x15d) = 1;
	FH_AP(THR_ACTOR(self), 0x188) = (char *)bkHeapCalloc(0x40, 0, s_ThrowableFile, 0x219, 0x2001);
	FH_AF(FH_AP(THR_ACTOR(self), 0x188), 0) = 10.0f;
	FH_AI(THR_ACTOR(self), 0x21c) = 5;
	FH_AI(self, 0x28) = 0;
	FH_AI(self, 0x2c) = 0;
	FH_AI(self, 0x30) = 0;
	FH_AI(THR_ACTOR(self), 0x1e4) = 0;
	FH_AF(THR_ACTOR(self), 0x1e8) = heading;
	FH_AI(THR_ACTOR(self), 0x1ec) = 0;
	FH_AI(self, 0x18) = type;
	FH_AI(self, 0xc) = 0;
	FH_AI(self, 0x48) = -1;
	FH_AI(self, 0x8) = 1;
	FH_AI(self, 0x4c) = 0;
	if (type < 6) {
		FH_AI(self, 0x10) = 1;
		FH_AF(THR_ACTOR(self), 0xe0) = 1.0f;
		FH_AF(THR_ACTOR(self), 0xe4) = 1.0f;
		FH_AF(THR_ACTOR(self), 0xe8) = 1.0f;
		return self;
	}
	FH_AI(self, 0x10) = 2;
	FH_AF(THR_ACTOR(self), 0xe0) = 1.1f;
	FH_AF(THR_ACTOR(self), 0xe4) = 1.1f;
	FH_AF(THR_ACTOR(self), 0xe8) = 1.1f;
	return self;
}

/* 0x00134620  (gap): the player to throw at: the one with the flag in front of the
throwable (within 0.8976 rad of its heading), else the nearest one in front. Returns the
player or -1 (callers test it for nonzero, so player 0 counts as no target). */
int FoxHunt_Throwable_FindTarget(void *self)
{
	int target = -1;
	float best = 1e+07f;
	int seen[4] = { 0, 0, 0, 0 };
	int n = DAT_002bae94;
	int i;
	float angle, dx, dz, d;

	for (i = 0; i < n; i++) {
		if (i == FH_AI(self, 0x48) || FH_AI(fh_Players[i], 0x11ac) != 1)
			continue;
		angle = CLMath_AngleDistance(CLMath_HeadingTo(FH_AV(THR_ACTOR(self), 0xd0), FH_AV(FH_AP(fh_Players[i], 0x30), 0xd0)),
			FH_AF(THR_ACTOR(self), 0x1e8));
		n = DAT_002bae94;
		if ((double)angle < 0.897597858) {
			seen[i] = 1;
			if (FH_AI(fh_Players[i], 0x11c4) == 1)
				seen[i] = 2;
		}
	}
	for (i = 0; i < n; i++) {
		if (seen[i] == 2) {
			best = 0.0f;
			target = i;
		} else if (seen[i] == 1) {
			char *a = FH_AP(DAT_002f289c[i], 0x30);

			dx = FH_AF(a, 0xd0) - FH_AF(THR_ACTOR(self), 0xd0);
			dz = FH_AF(a, 0xd8) - FH_AF(THR_ACTOR(self), 0xd8);
			d = sqrtf(dx * dx + dz * dz);
			if (d < best) {
				target = i;
				best = d;
			}
		}
	}
	return target;
}

/* gravity and ground friction of a free throwable; returns 0 when it is on the ground */
static int fall(void *self)
{
	char *a = THR_ACTOR(self);
	float ground;

	FH_AF(a, 0x198) = FH_AF(a, 0x198) - DAT_0028fda8 * DAT_0028fda8 * DAT_00292c84 * 0.5f;
	ground = CLColl_GroundProbe(FH_AV(THR_ACTOR(self), 0xd0), FH_AF(FH_AP(THR_ACTOR(self), 0x18c), 0x9c) * 0.5f, NULL, NULL, 0);
	a = THR_ACTOR(self);
	if (2.0f <= ground - FH_AF(FH_AP(a, 0x18c), 0x80))
		return 1;
	FH_AF(a, 0x194) = FH_AF(a, 0x194) * 0.8f;
	FH_AF(THR_ACTOR(self), 0x19c) = FH_AF(THR_ACTOR(self), 0x19c) * 0.8f;
	return 0;
}

/* moves the throwable by its velocity with collision (keeping the velocities before and
after the move) */
static void move(void *self)
{
	char *a = THR_ACTOR(self);

	FH_AI(a, 0x1a4) = FH_AI(a, 0xd0);
	FH_AI(a, 0x1a8) = FH_AI(a, 0xd4);
	FH_AI(a, 0x1ac) = FH_AI(a, 0xd8);
	FH_AI(a, 0x1b0) = FH_AI(a, 0xdc);
	a = THR_ACTOR(self);
	FH_AI(self, 0x28) = FH_AI(a, 0x194);
	FH_AI(self, 0x2c) = FH_AI(a, 0x198);
	FH_AI(self, 0x30) = FH_AI(a, 0x19c);
	FH_AI(self, 0x34) = FH_AI(a, 0x1a0);
	a = THR_ACTOR(self);
	FH_AF(a, 0xd0) = FH_AF(a, 0x194) + FH_AF(a, 0xd0);
	a = THR_ACTOR(self);
	FH_AF(a, 0xd8) = FH_AF(a, 0x19c) + FH_AF(a, 0xd8);
	a = THR_ACTOR(self);
	FH_AF(a, 0xd4) = FH_AF(a, 0x198) + FH_AF(a, 0xd4);
}

static void after_move(void *self)
{
	char *a;

	CRColl_MoveActor(THR_ACTOR(self));
	a = THR_ACTOR(self);
	FH_AI(self, 0x38) = FH_AI(a, 0x194);
	FH_AI(self, 0x3c) = FH_AI(a, 0x198);
	FH_AI(self, 0x40) = FH_AI(a, 0x19c);
	FH_AI(self, 0x44) = FH_AI(a, 0x1a0);
}

/* 0x00134770  (gap): the throwable's frame: falls out below -600; lying: gravity, the
escalators, being picked up (a player pressing action within reach); held: between the
holder's palms, thrown at the target (or straight ahead) when the action is pressed in the
air; dropped: bounces off; thrown: flies, breaking on the first player it hits (debris and
a "smash") */
void FoxHunt_Throwable_Update(void *self)
{
	char *a = THR_ACTOR(self);
	char *p;
	char *holder;
	int i, e, type;
	float v;
	float r[4], l[4];
	float d[4];
	float vel[4];
	float pos[4];
	float h;

	if (FH_AF(a, 0xd4) < -600.0f) {
		FH_AI(self, 0x8) = 0;
		FH_AI(self, 0x48) = -1;
	}
	if (STAGE() < 3 && 0 < FH_AI(FH_AP(a, 0x18c), 0x1d0) && FH_AI(self, 0xc) == 3)
		FUN_00137cb0(-100.0f <= FH_AF(a, 0xd0) ? 2 : 1, (undefined4)(uintptr_t)((char *)self + 0x28),
			(undefined4)(uintptr_t)FH_AV(a, 0xd0));
	if (FH_AI(self, 0x8) == 0) {
		FH_AF(THR_ACTOR(self), 0xd4) = -550.0f;
		FH_AI(THR_ACTOR(self), 0x220) = 0;
		return;
	}
	if (FH_AI(self, 0xc) == 0 && FH_AI(self, 0x8) == 1) {
		/* lying */
		if (0.0f < FH_AF(THR_ACTOR(self), 0x198))
			FH_AF(THR_ACTOR(self), 0x198) = 0.0f;
		{
			char *t = THR_ACTOR(self);
			float ground;

			FH_AF(t, 0x198) = FH_AF(t, 0x198) - DAT_0028fda8 * DAT_0028fda8 * DAT_00292c84 * 0.5f;
			FH_AI(THR_ACTOR(self), 0x220) = 0x40001c;
			ground = CLColl_GroundProbe(FH_AV(THR_ACTOR(self), 0xd0), FH_AF(FH_AP(THR_ACTOR(self), 0x18c), 0x9c) * 0.5f,
				NULL, NULL, 0);
			t = THR_ACTOR(self);
			if (ground - FH_AF(FH_AP(t, 0x18c), 0x80) < 2.0f) {
				FH_AF(t, 0x194) = FH_AF(t, 0x194) * 0.8f;
				FH_AF(THR_ACTOR(self), 0x19c) = FH_AF(THR_ACTOR(self), 0x19c) * 0.8f;
				t = THR_ACTOR(self);
				/* the escalators of the beach carry it */
				if (STAGE() >= 3 && -700.0f < FH_AF(t, 0xd8) && FH_AF(t, 0xd8) < 0.0f) {
					if (-1000.0f <= FH_AF(t, 0xd0) || FH_AF(t, 0xd0) <= -1180.0f) {
						if (FH_AF(t, 0xd0) > 1000.0f && FH_AF(t, 0xd0) < 1180.0f) {
							e = FUN_00137fa0(2);
							t = THR_ACTOR(self);
							v = FH_AF(t, 0x19c);
							FH_AF(t, 0x19c) = e == 1 ? v + 27.0f : v - 27.0f;
						}
					} else {
						e = FUN_00137fa0(1);
						t = THR_ACTOR(self);
						v = FH_AF(t, 0x19c);
						FH_AF(t, 0x19c) = e == 1 ? v + 27.0f : v - 27.0f;
					}
				}
			}
		}
		move(self);
		after_move(self);
		if (FH_AI(self, 0xc) != 0)
			return;
		for (i = 0; i < DAT_002bae94; i++) {
			p = fh_Players[i];
			if (FH_AI(p, 0x11bc) != 0 || FH_AI(p, 0x11ac) != 1 || FH_AS(p, 0x36) == 0xd ||
				FH_AI(FH_AP(p, 0x6c), 0x14) == 0 ||
				!(fh_DistanceSquared(FH_AV(THR_ACTOR(self), 0xd0), FH_AV(FH_AP(p, 0x30), 0xd0)) < 17000.0f))
				continue;
			/* picked up */
			FH_AI(FH_AP(THR_ACTOR(self), 0x18c), 0x1d0) = 0;
			AI_SetState(fh_Players[i], 0xf);
			FH_AI(fh_Players[i], 0x11bc) = 1;
			FH_AI(fh_Players[i], 0x11c0) = FH_AI(self, 0x18);
			FH_AI(self, 0xc) = 1;
			p = fh_Players[i];
			if (Core_RandUnit() <= FH_AF(p, 0x117c)) {
				if (0.8f <= Core_RandUnit())
					FH_AI(p, 0x11f4) = 3;
				else
					FH_AI(p, 0x11f4) = 5;
			} else {
				FH_AI(p, 0x11f4) = 4;
			}
			FH_AI(self, 0x48) = i;
			FH_AI(THR_ACTOR(self), 0x220) = 0;
			FH_AI(THR_ACTOR(self), 0x220) = 0x400010;
			FH_AI(fh_Players[i], 0x1188) = 0;
			FH_AF(FH_AP(fh_Players[i], 0x30), 0x1d8) =
				CLMath_HeadingTo(FH_AV(FH_AP(fh_Players[i], 0x30), 0xd0), FH_AV(THR_ACTOR(self), 0xd0));
		}
		return;
	}
	if (FH_AI(self, 0xc) == 1 && FH_AI(self, 0x8) == 1) {
		/* held between the holder's palms */
		holder = FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30);
		{
			struct BActorNode *node = baFindNode((struct BActorNode *)FH_AP(holder, 0x144), "rpalm");

			if (node == NULL) {
				bkPrintf("Didnt find node skipping that stuff\n");
			} else {
				baGetNodePosition((struct BActorInstance *)(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30) + 0x10), node, r);
				node = baFindNode((struct BActorNode *)FH_AP(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30), 0x144), "lpalm");
				baGetNodePosition((struct BActorInstance *)(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30) + 0x10), node, l);
				a = THR_ACTOR(self);
				FH_AF(a, 0xdc) = 1.0f;   /* a stack word the original never sets on this path */
				FH_AF(a, 0xd8) = (r[2] + l[2]) * 0.5f;
				FH_AF(a, 0xd0) = (r[0] + l[0]) * 0.5f;
				FH_AF(a, 0xd4) = (r[1] + l[1]) * 0.5f;
			}
		}
		holder = FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30);
		FH_AI(THR_ACTOR(self), 0x1e4) = FH_AI(holder, 0x1e4);
		FH_AI(THR_ACTOR(self), 0x1e8) = FH_AI(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30), 0x1e8);
		FH_AI(THR_ACTOR(self), 0x1ec) = FH_AI(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30), 0x1ec);
		if (FH_AI(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x6c), 0x14) == 0)
			return;
		a = THR_ACTOR(self);
		if (FH_AI(FH_AP(a, 0x18c), 0x1d0) != 0)
			return;
		if (FH_AF(a, 0xd4) - FH_AF(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30), 0xd4) <= 200.0f)
			return;
		/* thrown */
		FH_AI(self, 0xc) = 3;
		CRColl_Step(a);
		a = THR_ACTOR(self);
		if (STAGE() < 3 && 0 < FH_AI(FH_AP(a, 0x18c), 0x1d0))
			FUN_00137cb0(-100.0f <= FH_AF(a, 0xd0) ? 2 : 1, (undefined4)(uintptr_t)((char *)self + 0x28),
				(undefined4)(uintptr_t)FH_AV(a, 0xd0));
		i = FoxHunt_Throwable_FindTarget(self);
		if (i < 0) {
			FH_AI(THR_ACTOR(self), 0x194) = 0;
			FH_AF(THR_ACTOR(self), 0x19c) = DAT_0028fda8 * 70.0f;
			FH_AI(THR_ACTOR(self), 0x198) = 0;
			bmVectorRotateXYZ(FH_AV(THR_ACTOR(self), 0x194), FH_AV(THR_ACTOR(self), 0x194), 0.0f,
				FH_AF(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30), 0x1e8), 0.0f);
			AI_SetState(fh_Players[FH_AI(self, 0x48)], 0xe);
		} else {
			h = CLMath_HeadingTo(FH_AV(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30), 0xd0),
				FH_AV(FH_AP(fh_Players[i], 0x30), 0xd0));
			CLMath_AngleDistance(h, FH_AF(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30), 0x1e8));
			FH_AI(THR_ACTOR(self), 0x194) = 0;
			FH_AF(THR_ACTOR(self), 0x19c) = DAT_0028fda8 * 50.0f;
			FH_AI(THR_ACTOR(self), 0x198) = 0;
			bmVectorRotateXYZ(FH_AV(THR_ACTOR(self), 0x194), FH_AV(THR_ACTOR(self), 0x194), 0.0f, h, 0.0f);
			AI_SetState(fh_Players[FH_AI(self, 0x48)], 0xe);
			FH_AF(FH_AP(fh_Players[FH_AI(self, 0x48)], 0x30), 0x1d8) = h;
		}
		FH_AI(fh_Players[FH_AI(self, 0x48)], 0x11bc) = 0;
		FH_AI(fh_Players[FH_AI(self, 0x48)], 0x11c0) = -1;
		return;
	}
	if (FH_AI(self, 0xc) == 2 && FH_AI(self, 0x8) == 1) {
		/* dropped: a little hop away */
		FH_AI(self, 0x48) = -1;
		FH_AI(THR_ACTOR(self), 0x220) = 0x400019;
		FH_AF(THR_ACTOR(self), 0x194) = FH_AF(THR_ACTOR(self), 0x194) + 0.8f;
		FH_AF(THR_ACTOR(self), 0x198) = FH_AF(THR_ACTOR(self), 0x198) + 20.0f;
		FH_AF(THR_ACTOR(self), 0x19c) = FH_AF(THR_ACTOR(self), 0x19c) - 0.8f;
		move(self);
		after_move(self);
		FH_AI(self, 0xc) = 0;
		return;
	}
	if (FH_AI(self, 0xc) != 3 || FH_AI(self, 0x8) != 1)
		return;
	/* thrown: flying */
	FH_AI(THR_ACTOR(self), 0x220) = 0x400019;
	if (fall(self) == 0) {
		a = THR_ACTOR(self);
		if (2 < STAGE() && -705.0 < (double)FH_AF(a, 0xd8) && (double)FH_AF(a, 0xd8) < 0.0) {
			if (-1000.0 <= (double)FH_AF(a, 0xd0) || (double)FH_AF(a, 0xd0) <= -1180.0) {
				if (!((double)FH_AF(a, 0xd0) <= 1000.0 || 1180.0 <= (double)FH_AF(a, 0xd0))) {
					e = FUN_00137fa0(3);
					v = DAT_0028fda8 * 19.0f;
					a = THR_ACTOR(self);
					FH_AF(a, 0x19c) = e == 1 ? v + FH_AF(a, 0x19c) : FH_AF(a, 0x19c) - v;
				}
			} else {
				e = FUN_00137fa0(4);
				v = DAT_0028fda8 * 19.0f;
				a = THR_ACTOR(self);
				FH_AF(a, 0x19c) = e == 1 ? v + FH_AF(a, 0x19c) : FH_AF(a, 0x19c) - v;
			}
		}
		a = THR_ACTOR(self);
		if (FH_AF(a, 0x194) < 2.0f && FH_AF(a, 0x19c) < 2.0f)
			FH_AI(self, 0xc) = 0;
	}
	move(self);
	for (i = 0; i < DAT_002bae94; i++) {
		char *pa;
		char *ta;

		if (i == FH_AI(self, 0x48) || CRColl_PushApart(THR_ACTOR(self), FH_AP(fh_Players[i], 0x30)) <= 0)
			continue;
		/* hit player i: push it, hurt it, smash */
		pa = FH_AP(fh_Players[i], 0x30);
		ta = THR_ACTOR(self);
		d[0] = FH_AF(pa, 0xd0) - FH_AF(ta, 0xd0);
		d[1] = FH_AF(pa, 0xd4) - FH_AF(ta, 0xd4);
		d[2] = FH_AF(pa, 0xd8) - FH_AF(ta, 0xd8);
		bmVectorSetLength(d, d, 80.0f);
		pa = FH_AP(fh_Players[i], 0x30);
		FH_AF(pa, 0x194) = d[0] + FH_AF(pa, 0x194);
		FH_AF(pa, 0x198) = d[1] + FH_AF(pa, 0x198);
		FH_AF(pa, 0x19c) = d[2] + FH_AF(pa, 0x19c);
		FoxHunt_Player_Hit(fh_Players[i], FH_AV(THR_ACTOR(self), 0xd0), 210.0f, 0);
		ta = THR_ACTOR(self);
		FH_AI(self, 0x8) = 0;
		vel[0] = FH_AF(ta, 0x194);
		vel[1] = FH_AF(ta, 0x198);
		vel[2] = FH_AF(ta, 0x19c);
		vel[3] = FH_AF(ta, 0x1a0);
		pos[0] = FH_AF(ta, 0xd0);
		pos[1] = FH_AF(ta, 0xd4);
		pos[2] = FH_AF(ta, 0xd8);
		pos[3] = FH_AF(ta, 0xdc);
		vel[0] = Core_RandUnit() * 10.0f - 5.0f;
		vel[1] = Core_RandUnit() * 20.0f;
		vel[2] = Core_RandUnit() * 10.0f - 5.0f;
		pos[1] = Core_RandUnit() * 100.0f + pos[1];
		if (FH_AI(self, 0x10) == 0)
			FoxHunt_AddDebris(0xb, pos, vel, 0, 0xffffffffu);
		vel[1] = 20.0f;
		vel[0] = Core_RandUnit() * 10.0f - 5.0f;
		vel[2] = Core_RandUnit() * 10.0f - 5.0f;
		type = FH_AI(self, 0x10);
		if (type == 0)
			FoxHunt_AddDebris(0xc, pos, vel, 0, 0xffffffffu);
		else if (type == 1)
			FoxHunt_AddDebris(0x12, pos, vel, 0, 0xffffffffu);
		else if (type == 2)
			FoxHunt_AddDebris(0x14, pos, vel, 0, 0xffffffffu);
		vel[0] = Core_RandUnit() * 10.0f - 5.0f;
		vel[1] = Core_RandUnit() * 10.0f - 5.0f;
		vel[2] = Core_RandUnit() * 10.0f - 5.0f;
		type = FH_AI(self, 0x10);
		if (type == 0) {
			FoxHunt_AddDebris(0xd, pos, vel, 0, 0xffffffffu);
		} else if (type == 1) {
			int k;

			pos[1] = pos[1] + 100.0f;
			for (k = 0; k < 10; k++) {
				float spread = (float)k + 15.0f;

				vel[0] = Core_RandUnit() * 30.0f - spread;
				vel[1] = Core_RandUnit() * 20.0f - 5.0f;
				vel[2] = Core_RandUnit() * 30.0f - spread;
				FoxHunt_AddDebris(0x16, pos, vel, k, 0xffffffffu);
			}
		} else if (type == 2) {
			FoxHunt_AddDebris(0x15, pos, vel, 0, 0xffffffffu);
		}
		vel[0] = (float)fh_rand() * 0.000305185094f - 5.0f;
		vel[1] = (float)fh_rand() * 0.000305185094f - 5.0f;
		vel[2] = (float)fh_rand() * 0.000305185094f - 5.0f;
		type = FH_AI(self, 0x10);
		if (type == 0)
			FoxHunt_AddDebris(0xe, pos, vel, 0, 0xffffffffu);
		else if (type == 1)
			FoxHunt_AddDebris(0x13, pos, vel, 0, 0xffffffffu);
		FHunt_Sound_Play(fh_Speech, 0xc, 0xff, 0);
		p = fh_Players[i];
		if (FH_AI(p, 0x11bc) == 1) {
			FH_AI(fh_Throwables[FH_AI(p, 0x11c0)], 0xc) = 2;
			FH_AI(fh_Players[i], 0x11bc) = 0;
			FH_AI(fh_Players[i], 0x11c0) = -1;
		} else if (FH_AI(p, 0x11c4) != 1) {
			continue;
		}
		if (FH_AI(fh_Players[i], 0x11c4) == 1 && STAGE() != 4) {
			FH_AI(fh_Players[i], 0x11c4) = 2;
			FH_AI(fh_Players[i], 0x11dc) = 0;
			FH_AI(fh_Players[i], 0x11e0) = 0;
			FH_AI(fh_Players[i], 0x11e4) = 0;
			if (STAGE() == 2)
				FUN_001382a0();
		}
	}
	after_move(self);
}
