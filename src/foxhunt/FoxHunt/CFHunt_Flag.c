/*
FoxHunt/CFHunt_Flag.c  <-  C:\BlitzParty\Game\Src\FoxHunt\CFHunt_Flag.cpp
(module foxhunt)

The flag (the energy ball in variation 2; CFHunt_Flag, 0x54 bytes, in the list 0x0030230c
with its node at +0): a CharLib actor with collision, debris effects that follow it and,
outside Victim, a beam of rays (classlib_fx CRayControl) above it. The constructor is
tagged with this file; the gap functions after it that work on the flag list are placed
here too.

  +0x08 the actor              +0x0c id             +0x10 user value
  +0x14 state: 0 free, 1 carried, 2 dropped, 3 respawn, 4 dropped (Victim)
  +0x18 carrier (-1)           +0x20 pick-up radius
  +0x24, +0x28, +0x2c debris pieces       +0x30 ray anchor, +0x40 the ray control,
  +0x44 ray target
*/

#include "foxhunt.h"

static const char s_FlagFile[] = "C:\\BlitzParty\\Game\\Src\\FoxHunt\\CFHunt_Flag.cpp";

extern char *DAT_002ba96c;            /* core: the game (+0x18 stage) */
extern int DAT_002ba99c;              /* core: frame counter */

#define STAGE() FH_AI(DAT_002ba96c, 0x18)
#define FLAGS() FH_LIST(0x0030230c)

/* 0x00136570  CFHunt_Flag.cpp: CFHunt_Flag::CFHunt_Flag: the flag `id` at `pos` */
void *CFHunt_Flag_Ctor(void *self, float *pos, int id, int user)
{
	char *model;
	char *actor;
	float *at;
	int type;
	char *rays;

	model = FH_P(0x002ae10c);
	if (STAGE() != 4)
		model = FH_P(0x002ae0f4);
	actor = (char *)Actor_Create(model, pos, NULL, 5, 0, 1);
	FH_AP(self, 0x8) = actor;
	FH_AF(actor, 0xd0) = pos[0];
	FH_AF(actor, 0xd4) = pos[1];
	FH_AF(actor, 0xd8) = pos[2];
	FH_AF(actor, 0xdc) = pos[3];
	CLList_Insert((int *)FLAGS(), (int *)self, 0);
	FH_AI(FH_AP(self, 0x8), 0x220) = 1;
	FH_AF(self, 0x20) = 50.0f;
	FH_AU(FH_AP(self, 0x8), 0xe0) = 0x3a83126fu;   /* 0.001 */
	FH_AU(FH_AP(self, 0x8), 0xe4) = 0x3a83126fu;
	FH_AU(FH_AP(self, 0x8), 0xe8) = 0x3a83126fu;
	FH_AS(FH_AP(self, 0x8), 0x230) = 5;
	FH_AP(FH_AP(self, 0x8), 0x18c) = (char *)bkHeapCalloc(0x204, 0, s_FlagFile, 0x5f, 0x2001);
	FH_AF(FH_AP(FH_AP(self, 0x8), 0x18c), 0x9c) = 50.0f;
	FH_AI(FH_AP(FH_AP(self, 0x8), 0x18c), 0x90) = 0;
	FH_AF(FH_AP(FH_AP(self, 0x8), 0x18c), 0x80) = 20.0f;
	FH_AF(FH_AP(FH_AP(self, 0x8), 0x18c), 0xa0) = 50.0f;
	FH_AI(FH_AP(FH_AP(self, 0x8), 0x18c), 0x1ac) = 0;
	FH_AB(FH_AP(FH_AP(self, 0x8), 0x18c), 0x15d) = 1;
	FH_AP(FH_AP(self, 0x8), 0x188) = (char *)bkHeapCalloc(0x40, 0, s_FlagFile, 0x66, 0x2001);
	FH_AF(FH_AP(FH_AP(self, 0x8), 0x188), 0) = 10.0f;
	FH_AI(self, 0xc) = id;
	FH_AI(self, 0x14) = 0;
	FH_AI(self, 0x10) = user;
	FH_AI(self, 0x18) = -1;
	at = FH_AV(FH_AP(self, 0x8), 0xd0);
	if (STAGE() == 4) {
		type = 0x1b;
	} else {
		FH_AP(self, 0x24) = FoxHunt_AddDebris(0, at, DAT_002ea478, 0, 0xffffffffu);
		FH_AP(self, 0x28) = FoxHunt_AddDebris(1, FH_AV(FH_AP(self, 0x8), 0xd0), DAT_002ea478, 0, 0xffffffffu);
		at = FH_AV(FH_AP(self, 0x8), 0xd0);
		type = 0x1a;
	}
	FH_AP(self, 0x2c) = FoxHunt_AddDebris(type, at, DAT_002ea478, 0, 0xffffffffu);
	baPrepActorInstance((struct BActorInstance *)(FH_AP(self, 0x8) + 0x10), 1);
	FH_AF(self, 0x44) = 800.0f;
	FH_AF(self, 0x48) = 800.0f;
	FH_AF(self, 0x4c) = 800.0f;
	if (STAGE() != 4) {
		rays = (char *)CRay_CreateOrbRayControl(1, FH_AV(self, 0x30), FH_AV(self, 0x44), (void *)CRColl_RayCastNormal);
		FH_AP(self, 0x40) = rays;
		if (rays != NULL) {
			CRayControl_AddRay((struct CRayControl *)rays, fh_bitsf(0x4016cbe4u), 0.0f, 400.0f, 1.0f, 25.0f);
			CRayControl_AddRay((struct CRayControl *)FH_AP(self, 0x40), fh_bitsf(0x40afeddfu), 0.0f, 500.0f, 1.0f, 30.0f);
			CRayControl_AddRay((struct CRayControl *)FH_AP(self, 0x40), 0.0f, 0.0f, 500.0f, 1.0f, 45.0f);
			CRayControl_AddRay((struct CRayControl *)FH_AP(self, 0x40), 0.4f, 0.0f, 525.0f, 1.0f, 50.0f);
			CRayControl_AddRay((struct CRayControl *)FH_AP(self, 0x40), fh_bitsf(0x4062a974u), 0.0f, 700.0f, 1.0f, 60.0f);
			CRayControl_AddRay((struct CRayControl *)FH_AP(self, 0x40), 1.57079625f, 0.0f, 450.0f, 1.0f, 42.0f);
			FH_AI(FH_AP(self, 0x40), 0x4c) = 0x7f;
			FH_AI(FH_AP(self, 0x40), 0x50) = 0x7f;
			FH_AI(FH_AP(self, 0x40), 0x54) = 0x7f;
			FH_AI(FH_AP(self, 0x40), 0x58) = 100;
		}
	}
	return self;
}

/* 0x00136890  (gap): another flag lies within its pick-up radius of this one (on the
ground plane). Returns 1 then. */
int CFHunt_Flag_NearOther(void *self)
{
	char *f;
	float a[4];
	float b[4];

	for (f = fh_ListNext(FLAGS(), NULL); f != NULL; f = fh_ListNext(FLAGS(), f)) {
		if (FH_AI(f, 0xc) == FH_AI(self, 0xc))
			continue;
		a[0] = FH_AF(FH_AP(self, 0x8), 0xd0);
		a[2] = FH_AF(FH_AP(self, 0x8), 0xd8);
		a[3] = FH_AF(FH_AP(self, 0x8), 0xdc);
		a[1] = 0.0f;
		b[0] = FH_AF(FH_AP(f, 0x8), 0xd0);
		b[2] = FH_AF(FH_AP(f, 0x8), 0xd8);
		b[3] = FH_AF(FH_AP(f, 0x8), 0xdc);
		b[1] = 0.0f;
		if (bmVectorDistance(a, b) < FH_AF(f, 0x20))
			return 1;
	}
	return 0;
}

/* 0x00136960  (gap): the position of flag `id` (unchanged without it) */
void FUN_00136960(undefined4 *out, int id)
{
	char *f;
	char *a;

	for (f = fh_ListNext(FLAGS(), NULL); f != NULL; f = fh_ListNext(FLAGS(), f)) {
		if (FH_AI(f, 0xc) == id) {
			a = FH_AP(f, 0x8);
			out[0] = FH_AU(a, 0xd0);
			out[1] = FH_AU(a, 0xd4);
			out[2] = FH_AU(a, 0xd8);
			out[3] = FH_AU(a, 0xdc);
		}
	}
}

/* 0x001369c0  (gap): the nearest free flag to `pos` (written to `out` as it goes);
returns the number of free flags */
int FUN_001369c0(float *pos, undefined4 *out)
{
	float best = 1e+06f;
	char *f;
	char *near = NULL;   /* the original starts with a bogus pointer (0x49742400) */
	char *a;
	float dx, dy, dz, d;
	int n = 0;

	for (f = fh_ListNext(FLAGS(), NULL); f != NULL; f = fh_ListNext(FLAGS(), f)) {
		if (FH_AI(f, 0x14) != 0)
			continue;
		a = FH_AP(f, 0x8);
		dx = pos[0] - FH_AF(a, 0xd0);
		dy = pos[1] - FH_AF(a, 0xd4);
		dz = pos[2] - FH_AF(a, 0xd8);
		d = sqrtf(dx * dx + dy * dy + dz * dz);
		if (d < best) {
			near = f;
			best = d;
		}
		a = FH_AP(near, 0x8);
		out[0] = FH_AU(a, 0xd0);
		out[1] = FH_AU(a, 0xd4);
		out[2] = FH_AU(a, 0xd8);
		out[3] = FH_AU(a, 0xdc);
		n++;
	}
	return n;
}

/* 0x00136a80  (gap): the nearest flag to `pos`, free or not */
void FUN_00136a80(float *pos, undefined4 *out)
{
	float best = 1e+06f;
	char *f;
	char *near = NULL;   /* the original starts with a bogus pointer (0x49742400) */
	char *a;
	float dx, dy, dz, d;

	for (f = fh_ListNext(FLAGS(), NULL); f != NULL; f = fh_ListNext(FLAGS(), f)) {
		a = FH_AP(f, 0x8);
		dx = pos[0] - FH_AF(a, 0xd0);
		dy = pos[1] - FH_AF(a, 0xd4);
		dz = pos[2] - FH_AF(a, 0xd8);
		d = sqrtf(dx * dx + dy * dy + dz * dz);
		if (d < best) {
			near = f;
			best = d;
		}
		a = FH_AP(near, 0x8);
		out[0] = FH_AU(a, 0xd0);
		out[1] = FH_AU(a, 0xd4);
		out[2] = FH_AU(a, 0xd8);
		out[3] = FH_AU(a, 0xdc);
	}
}

/* 0x00136b40  (gap): player `player` takes flag `id` */
void FUN_00136b40(int player, int id)
{
	char *f;
	char *pa;
	char *fa;

	for (f = fh_ListNext(FLAGS(), NULL); f != NULL; f = fh_ListNext(FLAGS(), f)) {
		if (FH_AI(f, 0xc) != id)
			continue;
		pa = FH_AP(fh_Players[player], 0x30);
		fa = FH_AP(f, 0x8);
		FH_AI(fa, 0xd0) = FH_AI(pa, 0xd0);
		FH_AI(fa, 0xd4) = FH_AI(pa, 0xd4);
		FH_AI(fa, 0xd8) = FH_AI(pa, 0xd8);
		FH_AI(fa, 0xdc) = FH_AI(pa, 0xdc);
		FH_AI(f, 0x14) = 1;
		FH_AI(f, 0x18) = player;
		FH_AI(fh_Players[player], 0x11c4) = 1;
		FH_AI(fh_Players[player], 0x11c8) = id;
	}
}

/* 0x00136be0  (gap): respawns the flag at a random place of the variation's table (on a
route cell centre, away from the other flags), dropping in from y 2800 with a projector
spiral where it lands */
void CFHunt_Flag_Respawn(void *self)
{
	unsigned int k;
	int cell;
	float land[4];
	char *a;

	for (;;) {
		bmSetRandomSeed((uint)bkTimerRead() + (uint)STAGE() + (uint)FH_AI(self, 0xc) + (uint)DAT_002ba99c + 1 +
			(uint)DAT_002bae70);
		if (STAGE() == 1) {
			k = (unsigned int)fh_rand() % 6 * 0xc;
			FH_AI(FH_AP(self, 0x8), 0xd0) = FH_I(0x002ae110 + k);
			FH_AI(FH_AP(self, 0x8), 0xd4) = 0;
			FH_AI(FH_AP(self, 0x8), 0xd8) = FH_I(0x002ae118 + k);
		} else if (STAGE() != 2) {
			k = (unsigned int)fh_rand() % 7 * 0xc;
			FH_AI(FH_AP(self, 0x8), 0xd0) = FH_I(0x002ae1a0 + k);
			FH_AI(FH_AP(self, 0x8), 0xd4) = 0;
			FH_AI(FH_AP(self, 0x8), 0xd8) = FH_I(0x002ae1a8 + k);
		} else {
			k = ((unsigned int)fh_rand() & 3) * 0xc;
			FH_AI(FH_AP(self, 0x8), 0xd0) = FH_I(0x002ae164 + k);
			FH_AI(FH_AP(self, 0x8), 0xd4) = 0;
			FH_AI(FH_AP(self, 0x8), 0xd8) = FH_I(0x002ae16c + k);
		}
		cell = CLRoute_CellAt(FH_AV(FH_AP(self, 0x8), 0xd0));
		CLRoute_CellCentre(FH_AV(FH_AP(self, 0x8), 0xd0), cell);
		if (CFHunt_Flag_NearOther(self) != 1)
			break;
	}
	FH_AF(FH_AP(self, 0x8), 0xd4) = 2800.0f;
	a = FH_AP(self, 0x8);
	land[0] = FH_AF(a, 0xd0);
	land[1] = FH_AF(a, 0xd4);
	land[2] = FH_AF(a, 0xd8);
	land[3] = FH_AF(a, 0xdc);
	land[1] = land[1] - CLColl_GroundProbe(FH_AV(a, 0xd0), 50.0f, NULL, NULL, 0);
	FH_AI(self, 0x14) = 0;
	FH_AI(self, 0x18) = -1;
	FH_AI(FH_AP(self, 0x8), 0x220) = 1;
	CProjector_SpiralAt(land, -1);
}

/* frees one flag: its rays and the debris that follow it */
static void free_flag(char *f)
{
	char *rays;

	if (FH_AI(DAT_002ba96c, 0x18) != 4) {
		rays = FH_AP(f, 0x40);
		if (rays != NULL) {
			CRayControl_Destroy((struct CRayControl *)rays);
			bkHeapFree(rays);
		}
		Debris_StopPiece((struct DebrisPiece *)FH_AP(f, 0x24));
		Debris_StopPiece((struct DebrisPiece *)FH_AP(f, 0x28));
	}
	Debris_StopPiece((struct DebrisPiece *)FH_AP(f, 0x2c));
	bkHeapFree(f);
}

/* 0x00136de0  (gap): frees every flag */
void FUN_00136de0(void)
{
	char *f = fh_ListNext(FLAGS(), NULL);
	char *prev;

	while (f != NULL) {
		prev = fh_ListRemove(FLAGS(), f);
		free_flag(f);
		f = fh_ListNext(FLAGS(), prev);
	}
}

/* 0x00136e60  (gap): frees the flags with id `id` */
void FUN_00136e60(int id)
{
	char *f = fh_ListNext(FLAGS(), NULL);
	char *prev;

	while (f != NULL) {
		prev = f;
		if (FH_AI(f, 0xc) == id) {
			prev = fh_ListRemove(FLAGS(), f);
			free_flag(f);
		}
		f = fh_ListNext(FLAGS(), prev);
	}
}
