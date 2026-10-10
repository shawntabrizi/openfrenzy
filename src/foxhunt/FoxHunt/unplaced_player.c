/*
FoxHunt/unplaced_player.c  (module foxhunt; gap functions 0x0012e9a0-0x001334f0 of
C:\BlitzParty\Game\Src\FoxHunt\, file unknown)

The Fox Hunt player (0x11fc bytes, vtable 0x0026d690): a CharLib AI character (charlib's
0x1130-byte AI, see charlib_ai.h) with the Fox Hunt rules and AI on top. Human players use
the same object; the AI parts run only for computer players.

Fields of the derived class (after the 0x1130-byte CharLib AI):
  +0x1130 spawn position              +0x1140 on the right half (x >= 0)
  +0x1144 target on the right half     +0x1148 escalator state 0-4 (beach)
  +0x114c, +0x1150 patrol points       +0x1154 patrol timer
  +0x115c..+0x1168 scores of the 4 players as targets (also +0x1164 etc.)
  +0x116c jump step, +0x1170 jump      +0x1174 bonus (gate) to fetch
  +0x1178 vengeance, +0x117c aggression, +0x1180 greed (AI tweaks, by difficulty)
  +0x11ac in the round                 +0x11b0 place when out
  +0x11b4 escalator direction          +0x11b8 at a goal
  +0x11bc holds a throwable, +0x11c0 its index
  +0x11c4 has the flag/ball (2: dropped, 3: scored), +0x11c8 the flag's id
  +0x11cc, +0x11d4 timers              +0x11d0 score
  +0x11d8 place                        +0x11dc..+0x11e4 the three gate bonuses
  +0x11e8 splashed / splash timer      +0x11ec, +0x11f0 bonus points, +0x11f4, +0x11f8
*/

#include "foxhunt.h"

#define ACT(p) FH_AP(p, 0x30)                /* the CharLib actor */
#define POS(a) FH_AV(a, 0xd0)                /* an actor's position (vec4) */
#define REC(p) FH_AUS(ACT(p), 0x230)         /* the player record of an actor */
#define CHAR_OF(rec) DAT_002baed4[(rec) * 10]
#define CHAR_I(rec, off) (*(int *)(DAT_00290718 + CHAR_OF(rec) * 0x60 + (off)))
#define CHAR_F(rec, off) (*(float *)(DAT_00290718 + CHAR_OF(rec) * 0x60 + (off)))
#define STAGE() FH_AI(DAT_002ba96c, 0x18)    /* the variation being played (core) */
#define PRESS(p, button) (FH_AI(FH_AP(p, (button)), 0x14) = 1)   /* +0x68 jump, +0x6c action */

extern char *DAT_002ba96c;            /* core: the game (+0x18 stage) */
extern int DAT_00292c4c;              /* charlib: route grid columns */
extern int DAT_00292c54;              /* charlib: route cost of a visited cell */
extern float DAT_00292c58;            /* charlib: the highest step a player can jump */
extern float DAT_00292c68;            /* charlib: route grid width */
extern int DAT_00293948;              /* charlib: AI mode-6 time */
extern float DAT_002939bc;            /* charlib: respawn point wait */
extern char *DAT_002f27d8;            /* charlib: the route cells in use (0x18 bytes each) */
extern int DAT_002f27e4;              /* charlib: route cost of a free cell */
extern int DAT_00290e8c;              /* bumpercars_a: colour index of the next debris */
extern unsigned char DAT_00291734[];  /* bumpercars_a: the "RESPAWNCHAR" debris type */
extern unsigned int DAT_002bc490[256], DAT_002bce18[256], DAT_002bd640[256];  /* bdisplay: colour channels */
extern unsigned int DAT_002bca94;     /* bdisplay: opaque alpha */

extern const void *const vtable_0026d690[];

/* 0x0012e9a0  (gap): the player's destructor */
void FoxHunt_Player_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026d690;
	AI_Dtor(self);
}

/* 0x0012e9b0  (gap): plans a route from the player to `to` */
void FoxHunt_Player_RouteTo(void *self, float *to)
{
	CLRoute_Start((struct CLRoute *)((char *)self + 0x8c), POS(ACT(self)), to, FH_AF(ACT(self), 0xd4));
}

/* 0x0012e9e0  (gap): variation 1: a player away from the goals goes to an empty goal
(mode 0xb) when fewer than two goals are taken or when it is not the one at a goal */
void FoxHunt_Player_CheckGoal(void *self)
{
	unsigned int rec;
	int n;

	if (FH_AI(self, 0x11b8) != 0)
		return;
	n = FH_AU(FH_P(0x00302194), 0x34) < 0x80000000u;
	if (FH_AI(FH_P(0x0030219c), 0x34) > -1)
		n++;
	if (n == 0 || n == 1) {
		FH_AI(self, 0x4) = 0xb;
		FH_AI(self, 0x28) = 0;
	}
	rec = REC(self);
	if (FH_AU(FH_P(0x00302194), 0x34) != rec && FH_AU(FH_P(0x0030219c), 0x34) != rec) {
		FH_AI(self, 0x28) = 0;
		FH_AI(self, 0x4) = 0xb;
		return;
	}
	FH_AI(self, 0x20) = 0;
}

/* 0x0012ea60  (gap): the beach (variations 3-4): takes the escalators to reach `target`
(a point on the upper floor): walks to the escalator of its side, rides it up or down
(state +0x1148: 0 choose, 1/2 walk to escalator 1/2, 3 ride, 4 upstairs) */
void FoxHunt_Player_UseEscalator(void *self, float *target)
{
	float pos[4];
	float dir[4];
	int esc1;
	char *actor;

	FH_AI(self, 0x1144) = target[0] >= 0.0f ? 1 : 0;
	FH_AI(self, 0x1140) = FH_AF(ACT(self), 0xd0) >= 0.0f ? 1 : 0;
	esc1 = FUN_00137fa0(1);
	FUN_00137fa0(2);
	switch (FH_AI(self, 0x1148)) {
	case 0:
		if (FH_AI(self, 0x14) > 0) {
			FH_AI(self, 0x1148) = (esc1 != 1) + 1;
			return;
		}
		if (FH_AI(self, 0x14) < 1) {
			FH_AI(self, 0x1148) = (FH_AI(self, 0x1140) != 0) + 1;
			return;
		}
		break;
	case 1:
	case 2:
		if (FH_AI(self, 0x1148) == 1) {
			pos[1] = FH_F(0x002ab384);
			pos[0] = FH_F(0x002ab380);
			pos[2] = FH_F(0x002ab388);
		} else {
			pos[1] = FH_F(0x002ab39c);
			pos[0] = FH_F(0x002ab398);
			pos[2] = FH_F(0x002ab3a0);
		}
		if (bmVectorDistance(POS(ACT(self)), pos) < 100.0f || FH_AI(self, 0x11b4) != -1) {
			FH_AI(self, 0x1148) = 3;
			return;
		}
		if (CLRoute_CellAt(pos) != FH_AS(self, 0x1042))
			FoxHunt_Player_RouteTo(self, pos);
		break;
	case 3:
		actor = ACT(self);
		if (FH_AI(self, 0x1140) == 0) {
			if (FH_AF(actor, 0xd4) >= 10.0f || FH_AF(actor, 0xd8) <= -650.0f) {
				pos[0] = FH_F(0x002ab38c);
				pos[1] = FH_F(0x002ab390);
				pos[2] = FH_F(0x002ab394);
				dir[0] = -1.0f;
				dir[1] = 0.0f;
				dir[2] = 1.0f;
				if (FH_AI(self, 0x11b4) == -1 || FH_AI(self, 0x11b4) == 1)
					AI_SetStick(self, dir, 1.0f);
				if (FH_AI(self, 0x11b4) == 0)
					FH_AI(self, 0x1148) = 2;
			} else {
				FH_AI(self, 0x1148) = 1;
			}
		} else if (FH_AI(self, 0x1140) == 1) {
			if (FH_AF(actor, 0xd4) >= 10.0f || FH_AF(actor, 0xd8) <= -650.0f) {
				pos[2] = FH_F(0x002ab3ac);
				pos[0] = FH_F(0x002ab3a4);
				pos[1] = FH_F(0x002ab3a8);
				dir[0] = 1.0f;
				dir[1] = 0.0f;
				dir[2] = 1.0f;
				if (FH_AI(self, 0x11b4) == -1 || FH_AI(self, 0x11b4) == 1)
					AI_SetStick(self, dir, 1.0f);
				if (FH_AI(self, 0x11b4) == 0)
					FH_AI(self, 0x1148) = 1;
			} else {
				FH_AI(self, 0x1148) = 2;
			}
		}
		if (FH_AF(ACT(self), 0xd4) > 395.0f && FH_AF(ACT(self), 0xd8) > 680.0f)
			FH_AI(self, 0x1148) = 4;
		break;
	}
}

/* 0x0012edb0  (gap): makes each player's input map (Forward, leftRight, jump, action,
action2) bound to its pad (player record +0xc) */
void FoxHunt_CreatePlayerInputs(void)
{
	char *p;
	unsigned int pad;
	int i;

	for (i = 0; i < DAT_002bae94; i++) {
		pad = (unsigned int)DAT_002baed4[i * 10 + 3];
		FH_AP(fh_Players[i], 0x5c) = (char *)biCreateMap(5);
		p = fh_Players[i];
		FH_AP(p, 0x60) = (char *)biCreateChannel((struct biMap *)FH_AP(p, 0x5c), "Forward", 3, 0, 0, 0, 0);
		p = fh_Players[i];
		FH_AP(p, 0x64) = (char *)biCreateChannel((struct biMap *)FH_AP(p, 0x5c), "leftRight", 3, 0, 0, 0, 0);
		p = fh_Players[i];
		FH_AP(p, 0x68) = (char *)biCreateChannel((struct biMap *)FH_AP(p, 0x5c), "jump", 1, 1, 0, 0, 0);
		p = fh_Players[i];
		FH_AP(p, 0x6c) = (char *)biCreateChannel((struct biMap *)FH_AP(p, 0x5c), "action", 1, 1, 0, 0, 0);
		p = fh_Players[i];
		FH_AP(p, 0x70) = (char *)biCreateChannel((struct biMap *)FH_AP(p, 0x5c), "action2", 1, 1, 0, 0, 0);
		p = fh_Players[i];
		biBindInput((struct biChannel *)FH_AP(p, 0x60), pad | 0xba, 0);
		biBindInput((struct biChannel *)FH_AP(p, 0x64), pad | 0xb9, 0);
		biBindInput((struct biChannel *)FH_AP(p, 0x60), pad | 0xbe, pad | 0xbd);
		biBindInput((struct biChannel *)FH_AP(p, 0x64), pad | 0xc0, pad | 0xbf);
		biBindInput((struct biChannel *)FH_AP(p, 0x68), pad | 0xc5, 0);
		biBindInput((struct biChannel *)FH_AP(p, 0x6c), pad | 0xc7, 0);
		biBindInput((struct biChannel *)FH_AP(p, 0x6c), pad | 0xc6, 0);
	}
}

/* 0x0012ef90  (gap): rumbles the player's pad: 0 a hit, 1 a big hit, 2 out of the round,
3 scored */
void FoxHunt_Player_Rumble(void *self, int kind)
{
	int amplitude, sustain, style;
	int rec = (int)(short)FH_AUS(ACT(self), 0x230);

	switch (kind) {
	case 0:
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, rec, 0, 0x7fff, 500, 10, 0x46, 0, 3);
		return;
	case 1:
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, rec, 0, 0x7fff, 600, 10, 0x32, 0, 3);
		style = 4;
		sustain = 0x32;
		amplitude = 500;
		break;
	case 2:
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, rec, 0, 0x7fff, 0x5dc, 0x1e, 0x28, 0, 3);
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, rec, 1, 0x7fff, 0x5dc, 10, 0x3c, 0, 4);
		return;
	case 3:
		style = 0;
		sustain = 0x1e;
		amplitude = 600;
		break;
	default:
		return;
	}
	CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, rec, 1, 0x7fff, amplitude, 0x1e, sustain, 0, style);
}

/* 0x0012f0a0  (gap): FoxHunt player constructor: player `index` at `pos` with its
character's tag model, the Fox Hunt AI state tables (0x002ae770 states, 0x002ae728
conditions, 0x002ae600 priorities) and the AI tweaks of the difficulty */
void *FoxHunt_Player_Ctor(void *self, int index, float *pos)
{
	unsigned int *w = (unsigned int *)self;
	char *actor;
	int i, off;

	AI_Ctor(self);
	*(const void *const **)self = vtable_0026d690;
	actor = (char *)Actor_Create(FH_PV(0x002ab26c)[CHAR_OF(index) * 4], pos, NULL, 1, 0, 1);
	FH_AP(self, 0x30) = actor;
	bkPrintf("Created actor \n");
	FH_AI(ACT(self), 0x220) = 0x19;
	FH_AS(ACT(self), 0x230) = (short)index;
	if (DAT_00283ea4 != 0)
		Core_SetCharacterColours(ACT(self) + 0x10, DAT_00283ea4);
	FH_AP(self, 0x1094) = FH_A(0x002ae770);
	FH_AP(self, 0x1098) = FH_A(0x002ae728);
	FH_AP(self, 0x109c) = FH_A(0x002ae600);
	Actor_InitCharData(ACT(self));
	FH_AP(ACT(self), 0x324) = FH_A(0x002ab440);   /* "idle_ready", 0x44 bytes per animation */
	bkPrintf("Created anim data \n");
	for (i = 0, off = 0x264; off < 800; off += 4, i += 0x44)
		FH_AP(ACT(self), off) = (char *)baFindSubObject((struct BActorInstance *)(ACT(self) + 0x10),
			FH_AP(ACT(self), 0x324) + i);
	w[0x42d] = (unsigned int)(*(int *)(DAT_00290718 + 0x38 + CHAR_OF(index) * 0x60) / (DAT_002bae70 * 5 + 5));
	if (STAGE() < 3) {
		if (STAGE() < 3) {
			FH_AU(ACT(self), 0x1d8) = 0x40060a91u;   /* 2.0944 */
			FH_AU(ACT(self), 0x1d8) = 0x40060a91u;
		}
	} else {
		FH_AU(ACT(self), 0x1d8) = 0x40490fdau;       /* pi */
		FH_AU(ACT(self), 0x1e8) = 0x40490fdau;
	}
	w[0x43e] = 0x437a0000u;   /* +0x10f8 250 */
	w[0x440] = 0x437a0000u;   /* +0x1100 250 */
	w[0x477] = 0;
	w[0x478] = 0;
	w[0x479] = 0;
	w[0x46f] = 0;
	w[0x470] = 0xffffffffu;
	w[0x471] = 0;
	w[0x472] = 0xffffffffu;
	w[0x46b] = 1;
	w[0x43c] = 0x3f860a91u;   /* +0x10f0 1.0472 */
	w[0x43d] = 0x43610000u;   /* +0x10f4 225: attack range */
	w[0x43f] = 0x3f860a91u;   /* +0x10fc 1.0472: attack angle */
	w[0x441] = 3;
	w[0x442] = 5;
	w[0x47a] = 0;
	w[0x443] = 5;
	w[0x444] = 10;
	w[0x46c] = 4;
	w[0x438] = 0;
	w[0x462] = 0;
	w[0x47d] = 0;
	w[0x461] = 0;
	w[0x45c] = 0x42480000u;   /* +0x1170 jump 50 */
	w[0x45b] = 0x28;          /* +0x116c jump step */
	if (DAT_002bae70 == 0) {
		w[5] = 0;
		w[0x45e] = 0x3d4ccccdu;
		w[0x45f] = 0x3e4ccccdu;
		w[0x460] = 0x3ecccccdu;
		w[0x47e] = 0x3e6b851fu;
	} else if (DAT_002bae70 == 1) {
		w[5] = 1;
		w[0x45e] = 0x3e4ccccdu;
		w[0x45f] = 0x3e99999au;
		w[0x460] = 0x3f19999au;
		w[0x47e] = 0x3e2e147bu;
	} else if (DAT_002bae70 == 2) {
		w[5] = 3;
		w[0x45e] = 0x3eb33333u;
		w[0x45f] = 0x3ecccccdu;
		w[0x460] = 0x3f4ccccdu;
		w[0x47e] = 0x3db851ecu;
	}
	actor = ACT(self);
	w[0x46d] = 0xffffffffu;
	w[0x451] = 0xffffffffu;
	w[0x450] = 0xffffffffu;
	w[0x46e] = 0;
	w[0x47b] = 0;
	w[0x452] = 0;
	w[0x455] = 0;
	w[0x44c] = FH_AU(actor, 0xd0);
	w[0x44d] = FH_AU(actor, 0xd4);
	w[0x44e] = FH_AU(actor, 0xd8);
	w[0x44f] = FH_AU(actor, 0xdc);
	FH_AS(self, 0x98) = 0;
	FH_AS(self, 0x34) = 0x11;
	FH_AS(self, 0x36) = 1;
	w[1] = 0;
	w[2] = 0;
	AI_SetState(self, 0);
	return self;
}

/* 0x0012f410  (gap): a computer player walks along its route: updates the route when the
cell changes, steers to the next cell (with a random sway when stuck for 5 frames),
jumps up steps and over walls it walks into, and jumps when the gate rules or the box
at (620, 30, 170) ask for it */
void FoxHunt_Player_FollowRoute(void *self)
{
	float y0;
	int cell, next, i;
	float centre[4];
	float d[4];
	float v[4];
	float len, m, amount, scale;
	char *actor;
	char *cells;
	char *cur;
	char *nw;
	char **tri;
	float box[4];
	int climb;

	if (FH_AS(self, 0x98) != 0)
		return;
	y0 = FH_AF(ACT(self), 0xd4);
	cell = CLRoute_CellAt(POS(ACT(self)));
	if (cell == FH_AS(self, 0x1042) && FH_AS(FH_AP(ACT(self), 0x18c), 100) != 0)
		FH_AF(self, 0x1078) = DAT_0028fda8 + FH_AF(self, 0x1078);
	else
		FH_AF(self, 0x1078) = 0.0f;
	i = FH_AI(ACT(self), 0x214);
	if (i == 1 || i == 2 || (cell != FH_AS(self, 0x1040) && i != 0)) {
		FH_AI(self, 0x90) = DAT_002f27e4;
		if (cell != FH_AS(self, 0x1040) && (int)FH_AS(self, 0x1050) < DAT_00292c78) {
			int n = FH_AS(self, 0x1050);
			short *visited = (short *)FH_AP(self, 0x104c);
			int k;

			for (k = 0; k < n; k++) {
				if (cell == visited[k]) {
					FH_AI(self, 0x90) = DAT_00292c54;
					break;
				}
			}
			((short *)FH_AP(self, 0x104c))[n] = (short)cell;
			FH_AS(self, 0x1050) = (short)(FH_AS(self, 0x1050) + 1);
		}
		if (cell != FH_AS(self, 0x1040))
			FH_AI(self, 0x8c) = 0;
		FH_AS(self, 0x1042) = (short)CLRoute_NextCell(POS(ACT(self)), FH_AV(self, 0x1058), y0,
			(struct CLRoute *)((char *)self + 0x8c));
		FH_AS(self, 0x1040) = (short)cell;
		FH_AS(self, 0x1082) = 0;
	}
	if (FH_AI(ACT(self), 0x214) != 0 && (FH_AS(self, 0x1040) == FH_AS(self, 0x1042) || FH_AS(self, 0x1042) == -1)) {
		FH_AS(self, 0x98) = 1;
		goto done;
	}
	next = FH_AS(self, 0x1042);
	if (next < 0) {
		if (((short *)FH_AP(self, 0x103c))[FH_AS(self, 0x1044)] == 32000 || FH_AI(self, 0x8c) <= FH_AI(self, 0x90))
			goto done;
		next = CLRoute_PathStep((struct CLRoute *)((char *)self + 0x8c), cell, FH_AS(self, 0x1046), y0);
		if (next == -1)
			next = ((short *)FH_AP(self, 0x1054))[cell];
	}
	CLRoute_CellCentre(centre, next);
	CLVec3_SubXZ(d, centre, POS(ACT(self)));
	if (FH_AF(self, 0x1078) > 5.0f) {
		int rx = fh_rand() % 0x79 - 0x3c;
		int rz = fh_rand() % 0x79 - 0x3c;

		FH_AF(self, 0x1078) = 0.0f;
		d[0] = (float)rx + d[0];
		d[2] = (float)rz + d[2];
	}
	d[1] = 0.0f;
	len = sqrtf(d[2] * d[2] + d[0] * d[0]);
	if (len < FH_AF(self, 0x107c)) {
		CLVec3_SubXZ(d, FH_AV(self, 0x1068), POS(ACT(self)));
		len = sqrtf(d[2] * d[2] + d[0] * d[0]);
	}
	d[1] = 0.0f;
	if (len == 0.0f)
		goto done;
	actor = ACT(self);
	m = len;
	if (FH_AF(actor, 0x254) <= len)
		m = FH_AF(actor, 0x254);
	if (m >= 1.0f) {
		m = len;
		if (FH_AF(actor, 0x254) <= len)
			m = FH_AF(actor, 0x254);
		if (m == 0.0f)
			goto done;
	} else {
		m = 1.0f;
	}
	bmVectorSetLength(d, d, m);
	v[0] = d[0];
	v[1] = d[1];
	v[2] = d[2];
	v[3] = d[3];
	if (FH_VFN(self, 0x34, int (*)(void *))(self) == 0)
		return;
	if (FH_AF(self, 0x10d4) <= len)
		amount = 1.0f;
	else
		amount = len / FH_AF(self, 0x10d4);
	AI_SetStick(self, v, amount);
	actor = ACT(self);
	scale = DAT_00292c68 / ((float)DAT_00292c4c * m);
	v[0] = d[0] * scale;
	v[1] = d[1] * scale;
	v[2] = scale * d[2];
	d[0] = v[0] + FH_AF(actor, 0xd0);
	d[1] = v[1] + FH_AF(actor, 0xd4);
	d[2] = v[2] + FH_AF(actor, 0xd8);
	next = CLRoute_CellAt(d);
	actor = ACT(self);
	if (FH_AI(actor, 0x214) == 0 || next == FH_AS(self, 0x1040)) {
		FH_AS(self, 0x1082) = 0;
		goto done;
	}
	/* a step up: jump when it is in reach, else climb after 4 tries */
	cells = DAT_002f27d8;
	cur = cells + FH_AS(self, 0x1040) * 0x18;
	nw = cells + next * 0x18;
	if (FH_AF(cur, 0xc) != FH_AF(cur, 0x8) && FH_AF(cur, 0x8) - FH_AF(cur, 0xc) <= 100.0f)
		climb = 1;
	else if (FH_AF(nw, 0xc) == FH_AF(nw, 0x8) || FH_AF(nw, 0x8) - FH_AF(nw, 0xc) > 100.0f ||
		FH_AF(nw, 0xc) > FH_AF(cur, 0xc))
		climb = 0;
	else
		climb = 1;
	if (climb) {
		if (FH_AF(actor, 0x1b8) <= FH_AF(actor, 0xd4)) {
			if (y0 + 80.0f >= FH_AF(nw, 0xc))
				goto walls;
			FH_AS(self, 0x1082) = (short)(FH_AS(self, 0x1082) + 1);
			if (FH_AS(self, 0x1082) <= 3)
				goto walls;
			PRESS(self, 0x68);
		}
		FH_AS(self, 0x1082) = 0;
	} else {
		FH_AS(self, 0x1082) = 0;
		if (y0 + 80.0f < FH_AF(nw, 0x8) && FH_AF(nw, 0x8) <= DAT_00292c58 + y0)
			PRESS(self, 0x68);
	}
walls:
	/* walking into a steep wall: jump after a while */
	d[0] = v[0];
	d[2] = v[2];
	d[3] = v[3];
	d[1] = 0.0f;
	bmVectorNormalise(d, d);
	tri = (char **)FH_AP(ACT(self), 0x18c);
	for (i = 0; i < (int)FH_AUS(FH_AP(ACT(self), 0x18c), 100); i++) {
		char *t = tri[i];

		if (FH_AF(t, 0x34) < 0.7f && FH_AF(t, 0x30) * d[0] + FH_AF(t, 0x38) * d[2] < -0.3f) {
			FH_AS(self, 0x1080) = (short)(FH_AS(self, 0x1080) + 5);
			if (FH_AS(self, 0x1080) > 0x14) {
				PRESS(self, 0x68);
				FH_AS(self, 0x1080) = 0;
			}
			goto done;
		}
	}
	if (FH_AS(self, 0x1080) != 0)
		FH_AS(self, 0x1080) = (short)(FH_AS(self, 0x1080) - 1);
done:
	if (FH_AS(self, 0x1042) == -2)
		FH_AS(self, 0x1042) = (short)CLRoute_NextCell(POS(ACT(self)), FH_AV(self, 0x1058), y0,
			(struct CLRoute *)((char *)self + 0x8c));
	if (FH_AI(self, 0x1148) == 4) {
		if (FH_AI(self, 0x11b4) == 1)
			PRESS(self, 0x68);
	} else if (FH_AI(self, 0x11b4) == 0) {
		PRESS(self, 0x68);
	}
	if (STAGE() < 3) {
		box[0] = 620.0f;
		box[1] = 30.0f;
		box[2] = 170.0f;
		box[3] = 1.0f;
		if (bmVectorDistance(box, POS(ACT(self))) < 150.0f && FH_AI(ACT(self), 0x218) != 0)
			PRESS(self, 0x68);
	}
}

/* 0x0012fb60  (gap): the computer player's brain each frame: clears the buttons, throws
or picks up throwables, re-plans every think interval, avoids standing on other players
(mode 6), and attacks at random (aggression) */
void FoxHunt_Player_Think(void *self)
{
	char *actor;
	char **slot;
	unsigned int i;
	int k;

	for (k = 0; k < FH_AI(FH_AP(self, 0x5c), 8); k++)
		FH_AI(FH_AP(FH_AP(self, 0x5c), 0), 0x14 + k * 0xf0) = 0;
	if (FH_AI(FH_AP(ACT(self), 0x18c), 0x1d0) < 1 || FH_AI(self, 0x11bc) != 1) {
		if (FH_AI(self, 0x11bc) == 0 && FH_AI(self, 0x11f4) == 1) {
			for (slot = (char **)FH_ADDR(0x003022d4); slot < (char **)FH_ADDR(0x003022f4); slot++) {
				if (bmVectorDistance(POS(ACT(self)), POS(FH_AP(*slot, 0x14))) < 17000.0f)
					PRESS(self, 0x6c);
			}
		}
	} else {
		PRESS(self, 0x6c);
	}
	FH_AF(self, 0x20) = DAT_0028fda8 + FH_AF(self, 0x20);
	if (FH_AF(self, 0x10b4) <= FH_AF(self, 0x20) || FH_AI(self, 0x28) != 0) {
		if (FH_VFN(self, 0x1c, int (*)(void *))(self) != 0)
			FH_VFN(self, 0x18, void (*)(void *, int))(self, -1);
		if (FH_AI(self, 0x1148) != 3)
			FoxHunt_Player_FollowRoute(self);
		if (FH_AS(self, 0x1042) == -1)
			FH_VFN(self, 0x18, void (*)(void *, int))(self, -1);
	}
	if (FH_AI(self, 0x2c) != 0)
		FH_AI(self, 0x2c)--;
	for (i = 0; (int)i < DAT_002bae94; i++) {
		char *other;
		float dx, dy, dz, h;

		actor = ACT(self);
		if (i == FH_AUS(actor, 0x230))
			continue;
		other = FH_AP(DAT_002f289c[i], 0x30);
		dx = FH_AF(actor, 0xd0) - FH_AF(other, 0xd0);
		dy = FH_AF(actor, 0xd4) - FH_AF(other, 0xd4);
		dz = FH_AF(actor, 0xd8) - FH_AF(other, 0xd8);
		h = dx * dx + dz * dz;
		if (sqrtf(dy * dy + h) <= FH_AF(FH_AP(other, 0x18c), 0x9c) + FH_AF(FH_AP(actor, 0x18c), 0x9c) + 10.0f &&
			sqrtf(h) < fabsf(dy)) {
			k = FH_AI(self, 0x2c);
			FH_AI(self, 0x2c) = k + 10;
			if (k + 10 > 0x32) {
				if (FH_AI(self, 0x4) != 6) {
					FH_VFN(self, 0x18, void (*)(void *, int))(self, 6);
					FH_AI(self, 0x24) = 0x14;
				}
				FH_AP(self, 0x1c) = POS(FH_AP(DAT_002f289c[i], 0x30));
			}
			break;
		}
	}
	if (fh_randf() < FH_AF(self, 0x117c) && (FH_AI(self, 0x4) != 0xb || FH_AI(self, 0x11bc) == 1))
		FH_VFN(self, 0x14, void (*)(void *))(self);
	if (FH_AI(self, 0x11b8) != 0 && fh_randf() < FH_AF(self, 0x117c))
		FH_VFN(self, 0x14, void (*)(void *))(self);
	FH_AI(self, 0x24) = 200;
}

/* 0x0012fe30  (gap; vtable slot 5, the end of AI_Think): the action button: throw what it
holds when the throwable allows it; otherwise press it when the AI state allows an attack
(priorities 5, 6) and, by the character's attack chance, keep it pressed only when
condition 5 holds */
void FoxHunt_Player_Act(void *self)
{
	int saved = FH_AI(FH_AP(self, 0x6c), 0x14);
	char *prio;

	if (FH_AI(self, 0x11bc) == 1) {
		if (FoxHunt_Throwable_CanThrow(fh_Throwables[FH_AI(self, 0x11c0)]) != 0) {
			PRESS(self, 0x6c);
			return;
		}
	} else {
		prio = FH_AP(self, 0x109c) + (unsigned int)FH_AUS(self, 0x36) * (unsigned int)FH_AUS(self, 0x34);
		if (prio[5] != 0 || prio[6] != 0) {
			PRESS(self, 0x6c);
			if (fh_rand() % 100 <= CHAR_I(REC(self), 0x24) &&
				((int (**)(void *))FH_AP(self, 0x1098))[5](self) != 0)
				return;
			FH_AI(FH_AP(self, 0x6c), 0x14) = saved;
		}
	}
}

/* scores the players as targets (+0x115c: -2 itself, -1 out or a team mate, else 0 plus
bonuses) */
static void score_targets(void *self)
{
	unsigned int i;

	for (i = 0; i < 4; i++) {
		if (i == FH_AUS(ACT(self), 0x230))
			FH_AI(self, 0x115c + i * 4) = -2;
		else
			FH_AI(self, 0x115c + i * 4) = (FH_AI(fh_Players[i], 0x11ac) != 0) - 1;
		if (DAT_002baebc != 0 && i != FH_AUS(ACT(self), 0x230) &&
			DAT_002baef4[i * 10] == DAT_002baef4[FH_AUS(ACT(self), 0x230) * 10])
			FH_AI(self, 0x115c + i * 4) = -1;
	}
}

/* 0x0012ff00  (gap): variation 1: picks what to do: carry the flag to a goal, take a
flag, or hunt the most important player (mode 7) */
void FoxHunt_Player_ChooseCooperation(void *self)
{
	float flag[4];
	int best, who, i;

	if (FH_AI(self, 0x11c4) == 1) {
		if (fh_randf() < FH_AF(self, 0x1180)) {
			FH_AI(self, 0x11f4) = 1;
			FoxHunt_Player_CheckGoal(self);
			return;
		}
		FH_AI(self, 0x11f4) = 2;
		FoxHunt_Player_CheckGoal(self);
		return;
	}
	i = FUN_001369c0(POS(ACT(self)), (undefined4 *)flag);
	if (i > 0) {
		FH_AI(self, 0x4) = 5;
		return;
	}
	if (i != 0)
		return;
	score_targets(self);
	for (i = 0; i < 4; i++) {
		char *p = fh_Players[i];
		int *s = &FH_AI(self, 0x115c + i * 4);

		if (*s > -1) {
			if (FH_AI(p, 0x11c4) == 1)
				*s += 2;
			if (FH_AI(p, 0x4) == 0xb)
				*s += 1;
			if (FH_AI(self, 0x11d0) < FH_AI(p, 0x11d0))
				*s += 1;
			if (FH_AI(self, 0x11d0) + 0x14 < FH_AI(p, 0x11d0))
				*s += 1;
			if (FH_AI(p, 0x11b8) == 1)
				*s += 2;
			if (FH_AS(p, 0x36) == 0xd || FH_AS(p, 0x36) == 0x10)
				*s -= 4;
		}
	}
	best = FH_AI(self, 0x115c);
	if (best < 0) {
		best = -1;
		who = -1;
	} else {
		who = 0;
	}
	if (best < FH_AI(self, 0x1160)) {
		who = 1;
		best = FH_AI(self, 0x1160);
	}
	if (best < FH_AI(self, 0x1164)) {
		who = 2;
		best = FH_AI(self, 0x1164);
	}
	if (best < FH_AI(self, 0x1168)) {
		FH_AI(self, 0x18) = 3;
		FH_VFN(self, 0x18, void (*)(void *, int))(self, 7);
		return;
	}
	if (who > -1) {
		FH_AI(self, 0x18) = who;
		FH_VFN(self, 0x18, void (*)(void *, int))(self, 7);
		return;
	}
	bkPrintf("Tried to find an important player but couldnt\n");
}

/* 0x00130310  (gap): variation 2: with the ball, goes for a gate bonus it lacks (mode 9,
by greed) or to a goal (mode 0xb) */
void FoxHunt_Player_ChooseBonus(void *self)
{
	int idx = 0;   /* the original leaves the player pointer here when it has all three */
	unsigned char n;

	n = FH_AI(self, 0x11dc) == 1;
	if (FH_AI(self, 0x11dc) != 1)
		idx = 0;
	if (FH_AI(self, 0x11e0) == 1)
		n++;
	else
		idx = 1;
	if (FH_AI(self, 0x11e4) == 1)
		n++;
	else
		idx = 2;
	if (n < 3)
		idx = FUN_001380d0(REC(self));
	if ((n == 0 || (n != 3 && n != 0)) && fh_randf() < FH_AF(self, 0x1180)) {
		FH_AI(self, 0x1174) = idx;
		FH_AI(self, 0x4) = 9;
		return;
	}
	FH_AI(self, 0x4) = 0xb;
}

/* the patrol points of the beach (12 at 0x002ab3b0) */
static void patrol_point(float *out, int i)
{
	out[0] = FH_FV(0x002ab3b0)[i * 3];
	out[1] = FH_FV(0x002ab3b0)[i * 3 + 1];
	out[2] = FH_FV(0x002ab3b0)[i * 3 + 2];
}

/* 0x00130400  (gap): the beach: moves on to the next patrol point (two steps either way
around the ring), the one farther from the player to avoid (variation 4: the one with the
flag; 3: the last one without it). Returns 1. */
int FoxHunt_Player_NextPatrolPoint(void *self)
{
	float point[4];
	float avoid[4] = { 0.0f, 0.0f, 0.0f, 0.0f };   /* uninitialised when nobody matches */
	int from, prev, next, i;
	float dprev, dnext;
	char *other;

	patrol_point(point, FH_AI(self, 0x1150));
	if (bmVectorDistance(POS(ACT(self)), point) >= 150.0f)
		from = FH_AI(self, 0x114c);
	else
		from = FH_AI(self, 0x1150);
	for (i = 0; i < 4; i++) {
		other = ACT(fh_Players[i]);
		if (FH_AS(other, 0x230) != FH_AS(ACT(self), 0x230) &&
			((FH_AI(fh_Players[i], 0x11c4) == 1 && STAGE() == 4) ||
				(FH_AI(fh_Players[i], 0x11c4) == 0 && STAGE() == 3))) {
			avoid[0] = FH_AF(other, 0xd0);
			avoid[1] = FH_AF(other, 0xd4);
			avoid[2] = FH_AF(other, 0xd8);
			avoid[3] = FH_AF(other, 0xdc);
		}
	}
	prev = from - 2;
	if (prev < 0)
		prev = 0xb;
	patrol_point(point, prev);
	dprev = bmVectorDistance(avoid, point);
	next = from + 2;
	if (next > 0xb)
		next = 0;
	patrol_point(point, next);
	dnext = bmVectorDistance(avoid, point);
	if (dprev + 75.0f < dnext)
		prev = next;
	i = FH_AI(self, 0x1150);
	FH_AI(self, 0x1150) = prev;
	FH_AI(self, 0x114c) = i;
	FH_AI(self, 0x1154) = 0;
	return 1;
}

/* 0x001306b0  (gap): revenge: when hit by a player of another team (+0x1088), attacks it
back (mode 4) by the character's revenge chance. Returns 1 when it does. */
int FoxHunt_Player_Retaliate(void *self)
{
	if (FH_AS(self, 0x1088) == -1)
		return 0;
	if (DAT_002baebc == 0 || DAT_002baef4[FH_AS(self, 0x1088) * 10] != DAT_002baef4[REC(self) * 10]) {
		if (STAGE() != 4 && FH_AI(self, 0x11c4) != 1 && fh_rand() % 100 < CHAR_I(REC(self), 0x28)) {
			FH_AI(self, 0x4) = 4;
			return 1;
		}
	} else {
		bkPrintf("DO not retaliate against someone on your own team AI dude / dudette\n");
		FH_AS(self, 0x1088) = -1;
	}
	return 0;
}

/* 0x00130780  (gap): a random other player still in the round (-1 when only `skip_down`
players that are down (state 10) are left, or the round has fewer than two) */
int FoxHunt_Player_RandomOther(void *self, int skip_down)
{
	unsigned int i;

	if (skip_down != 0) {
		for (i = 0; (int)i < DAT_002bae94; i++) {
			if (i != FH_AUS(ACT(self), 0x230) && FH_AS(fh_Players[i], 0x36) != 10)
				break;
		}
		if (i == (unsigned int)DAT_002bae94)
			return -1;
	}
	if (FH_AI(self, 0x11b0) < 2)
		return -1;
	do {
		do {
			i = (unsigned int)((long long)(unsigned long long)(unsigned int)fh_rand() % (long long)DAT_002bae94);
		} while (i == FH_AUS(ACT(self), 0x230));
	} while (FH_AI(fh_Players[i], 0x11ac) != 1);
	return (int)i;
}

/* 0x00130830  (gap): steps aside from the nearest player of another team (mode 6) */
void FoxHunt_Player_AvoidNearest(void *self)
{
	float best = 1e+07f;
	int who = 0;   /* uninitialised in the original when nobody is near */
	unsigned int i;
	char *actor = ACT(self);
	char *other;
	float dx, dz, d;

	for (i = 0; (int)i < DAT_002bae94; i++) {
		if (i == FH_AUS(actor, 0x230))
			continue;
		if (DAT_002baebc != 0 && DAT_002baef4[i * 10] == DAT_002baef4[FH_AUS(actor, 0x230) * 10])
			continue;
		other = FH_AP(DAT_002f289c[i], 0x30);
		dx = FH_AF(other, 0xd0) - FH_AF(actor, 0xd0);
		dz = FH_AF(other, 0xd8) - FH_AF(actor, 0xd8);
		d = sqrtf(dx * dx + dz * dz);
		if (d < best) {
			best = d;
			who = (int)i;
		}
	}
	if (FH_AI(self, 0x4) != 6) {
		FH_VFN(self, 0x18, void (*)(void *, int))(self, 6);
		FH_AI(self, 0x24) = DAT_00293948;
		other = ACT(fh_Players[who]);
		FH_AI(self, 0x118c) = FH_AI(other, 0xd0);
		FH_AI(self, 0x1190) = FH_AI(other, 0xd4);
		FH_AI(self, 0x1194) = FH_AI(other, 0xd8);
		FH_AI(self, 0x1198) = FH_AI(other, 0xdc);
	}
}

/* 0x00130930  (not in the function table; the tail of 0x00130f30): one step of the
player's AI state machine (the CharLib AI update with the Fox Hunt speed rules): the
stick, the best condition by priority, the state handler, pushes and the speed limit,
and the escalators of the beach */
void FUN_00130930(void *self)
{
	char *actor = ACT(self);
	int coff = CHAR_OF(FH_AUS(actor, 0x230)) * 0x60;
	signed char best = 0;
	signed char pr;
	int idx = 0;
	int i, e;
	float maxspeed, h;
	float stick[4];
	char *v;

	if (FH_AS(self, 0x10b8) != 0)
		return;
	if (*(float *)(DAT_00290718 + coff + 0x8) <= FH_AF(self, 0x10dc))
		maxspeed = *(float *)(DAT_00290718 + coff + 0x8);
	else
		maxspeed = FH_AF(self, 0x10dc);
	FH_AF(self, 0x10b0) = (1.0f - (1.0f - *(float *)(DAT_00290718 + coff + 0x18)) * FH_AF(self, 0x10e0)) *
		*(float *)(DAT_00290718 + coff + 0x4);
	stick[1] = 0.0f;
	stick[0] = (float)FH_AI(FH_AP(self, 0x64), 0x14) * 3.05250323e-05f;
	stick[2] = (float)FH_AI(FH_AP(self, 0x60), 0x14) * -3.05250323e-05f;
	h = sqrtf(stick[0] * stick[0] + stick[2] * stick[2]);
	if (h > 1.0f)
		h = 1.0f;
	FH_AF(self, 0x54) = h;
	bmVectorRotateY(FH_AV(self, 0x44), stick, FH_AF(self, 0x1118));
	bmVectorNormalise(FH_AV(self, 0x44), FH_AV(self, 0x44));
	FH_AF(self, 0x58) = atan2f(FH_AF(self, 0x44), FH_AF(self, 0x4c));
	for (i = 0; i < (int)FH_AUS(self, 0x34); i++) {
		pr = (signed char)FH_AP(self, 0x109c)[(unsigned int)FH_AUS(self, 0x36) * (unsigned int)FH_AUS(self, 0x34) + i];
		if (best < pr && ((int (**)(void *))FH_AP(self, 0x1098))[i](self) != 0) {
			best = pr;
			idx = i;
		}
	}
	FH_AI(self, 0x3c)++;
	FH_AF(self, 0x40) = DAT_0028fda8 + FH_AF(self, 0x40);
	if (best != 0) {
		FH_AS(self, 0x38) = FH_AS(self, 0x36);
		FH_AS(self, 0x36) = (short)idx;
		FH_AI(self, 0x40) = 0;
		FH_AI(self, 0x3c) = 0;
	}
	((void (**)(void *, int))FH_AP(self, 0x1094))[FH_AUS(self, 0x36)](self, best != 0);
	if (FH_AF(FH_AP(ACT(self), 0x18c), 0x1ac) == 0.0f) {
		v = ACT(self) + 0x194;
		bmVectorBlend((float *)v, (float *)v, 1.0f, FH_AV(self, 0x7c), DAT_0028fda8);
	}
	if (FH_AF(self, 0x84) * FH_AF(self, 0x84) + FH_AF(self, 0x80) * FH_AF(self, 0x80) +
		FH_AF(self, 0x7c) * FH_AF(self, 0x7c) != 0.0f) {
		actor = ACT(self);
		h = sqrtf(FH_AF(actor, 0x19c) * FH_AF(actor, 0x19c) + FH_AF(actor, 0x194) * FH_AF(actor, 0x194));
		if (maxspeed < h) {
			maxspeed = maxspeed / h;
			FH_AF(actor, 0x194) = maxspeed * FH_AF(actor, 0x194);
			FH_AF(ACT(self), 0x19c) = maxspeed * FH_AF(ACT(self), 0x19c);
		}
	}
	if (STAGE() >= 3) {
		actor = ACT(self);
		FH_AI(self, 0x11b4) = -1;
		if (FH_AF(actor, 0xd8) <= -700.0f || 0.0f <= FH_AF(actor, 0xd8) ||
			(FH_AI(actor, 0x214) == 0 && FH_AI(actor, 0x218) == 0))
			goto speed;
		if (-949.0f <= FH_AF(actor, 0xd0) || FH_AF(actor, 0xd0) <= -1200.0f) {
			if (FH_AF(actor, 0xd0) <= 949.0f || 1200.0f <= FH_AF(actor, 0xd0) ||
				(FH_AI(actor, 0x214) == 0 && FH_AI(actor, 0x218) == 0))
				goto speed;
			e = FUN_00137fa0(2);
		} else {
			e = FUN_00137fa0(1);
		}
		if (e == 1)
			FH_AF(ACT(self), 0x19c) = DAT_0028fda8 * 16.5f + FH_AF(ACT(self), 0x19c);
		else if (e == 0)
			FH_AF(ACT(self), 0x19c) = FH_AF(ACT(self), 0x19c) - DAT_0028fda8 * 16.5f;
		FH_AI(self, 0x11b4) = e;
	}
speed:
	if (FH_AF(ACT(self), 0x198) > 50.0f)
		FH_AF(ACT(self), 0x198) = 50.0f;
	actor = ACT(self);
	FH_AF(actor, 0x254) = sqrtf(FH_AF(actor, 0x19c) * FH_AF(actor, 0x19c) +
		FH_AF(actor, 0x198) * FH_AF(actor, 0x198) + FH_AF(actor, 0x194) * FH_AF(actor, 0x194));
}

/* 0x00130d10  (gap): the player is hit from `from` with `power` (`side` 0: work out the
side from the heading; 1: from behind): adds the power to the damage (+0x111c); up to the
character's toughness it staggers (state 10), else it is knocked down (state 0xd) and the
damage resets. With `side` != 0 the original compares the bits of `from` as the angle (its
stack slot is reused); that is kept. */
void FoxHunt_Player_Hit(void *self, float *from, float power, int side)
{
	float angle = fh_bitsf((uint32_t)(uintptr_t)from);
	float heading = fh_bitsf((uint32_t)side);
	float damage;

	if (side == 0) {
		heading = CLMath_HeadingTo(POS(ACT(self)), from);
		angle = CLMath_AngleDistance(heading, FH_AF(ACT(self), 0x1e8));
	}
	damage = power + FH_AF(self, 0x111c);
	FH_AF(self, 0x111c) = damage;
	if (damage <= CHAR_F(REC(self), 0x20)) {
		AI_SetState(self, 10);
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, (short)FH_AUS(ACT(self), 0x230), 0, 0x7fff, 500, 10, 0x46, 0, 3);
		if (angle <= 1.57079625f && side != 1) {
			Actor_PlayAnim(ACT(self), 0x11, 0, 0, 1.0f, 0.1f, 1);
			return;
		}
		Actor_PlayAnim(ACT(self), 0x12, 0, 0, 1.0f, 0.1f, 1);
	} else {
		AI_SetState(self, 0xd);
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, (short)FH_AUS(ACT(self), 0x230), 0, 0x7fff, 600, 10, 0x32, 0, 3);
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, (short)FH_AUS(ACT(self), 0x230), 1, 0x7fff, 500, 0x1e, 0x32, 0, 4);
		FH_AF(self, 0x111c) = 0.0f;
		if (1.57079625f < angle || side == 1) {
			FH_AI(self, 0x1120) = 0;
			Actor_PlayAnim(ACT(self), 0x14, 0, 0, 1.0f, 0.1f, 1);
			if (side == 0)
				FH_AF(ACT(self), 0x1d8) = CLMath_WrapAngle(heading + 3.1415925f);
		} else {
			FH_AI(self, 0x1120) = 1;
			Actor_PlayAnim(ACT(self), 0x13, 0, 0, 1.0f, 0.1f, 1);
			if (side == 0)
				FH_AF(ACT(self), 0x1d8) = heading;
		}
	}
}

/* 0x00130f30  (gap): the player's update: the AI tweaks of its AI mode (with or without
the flag), the escalator state, the difficulty speed bonus (+0x11f8) and the skill
(+0x10e0, higher with the flag or a throwable), then the AI state machine step
(0x00130930, a tail call) */
void FoxHunt_Player_Update(void *self)
{
	char *actor = ACT(self);
	int mode;
	unsigned int v;

	if (DAT_002baed8[FH_AUS(actor, 0x230) * 10] == 0) {
		if (STAGE() < 3 || (STAGE() == 4 && FH_AI(self, 0x11c4) == 0) || (STAGE() == 3 && FH_AI(self, 0x11c4) == 1)) {
			mode = FH_AI(self, 0x14);
			if (mode == 0) {
				FH_AI(self, 0x1180) = 0;
				FH_AU(self, 0x1178) = 0x3d4ccccdu;   /* 0.05 */
				FH_AU(self, 0x117c) = 0x3dcccccdu;   /* 0.1 */
			} else if (mode == 1) {
				FH_AU(self, 0x1180) = 0x3e99999au;   /* 0.3 */
				FH_AU(self, 0x1178) = 0x3e19999au;   /* 0.15 */
				FH_AU(self, 0x117c) = 0x3dcccccdu;
			} else if (mode == 2 || mode == 3) {
				if (mode == 2) {
					FH_AU(self, 0x1180) = 0x3f19999au;   /* 0.6 */
					v = 0x3dcccccdu;
				} else {
					FH_AU(self, 0x1180) = 0x3f4ccccdu;   /* 0.8 */
					v = 0x3e19999au;
				}
				FH_AU(self, 0x1178) = v;
				FH_AU(self, 0x117c) = v;
			}
		} else {
			mode = FH_AI(self, 0x14);
			if (mode == 0) {
				FH_AU(self, 0x1180) = 0x3e99999au;   /* 0.3 */
				FH_AU(self, 0x1178) = 0x3e19999au;   /* 0.15 */
				FH_AU(self, 0x117c) = 0x3ee66666u;   /* 0.45 */
			} else if (mode == 1) {
				FH_AU(self, 0x1180) = 0x3f000000u;   /* 0.5 */
				FH_AU(self, 0x1178) = 0x3e99999au;   /* 0.3 */
				FH_AU(self, 0x117c) = 0x3f0ccccdu;   /* 0.55 */
			} else if (mode == 2) {
				FH_AU(self, 0x1180) = 0x3f19999au;   /* 0.6 */
				FH_AU(self, 0x1178) = 0x3ecccccdu;   /* 0.4 */
				FH_AU(self, 0x117c) = 0x3f266666u;   /* 0.65 */
			} else if (mode == 3) {
				FH_AU(self, 0x1180) = 0x3f666666u;   /* 0.9 */
				FH_AU(self, 0x1178) = 0x3ecccccdu;
				FH_AU(self, 0x117c) = 0x3f666666u;
			}
		}
	}
	if (FH_AF(actor, 0xd4) > 390.0f && FH_AI(self, 0x1148) != 3 && FH_AF(actor, 0xd8) > 680.0f)
		FH_AI(self, 0x1148) = 4;
	if (DAT_002baed8[FH_AUS(actor, 0x230) * 10] == 0) {
		if (DAT_002bae70 == 0) {
			if (STAGE() < 3)
				FH_AU(self, 0x11f8) = 0x3e6b851fu;   /* 0.23 */
			else
				FH_AU(self, 0x11f8) = 0x3deb851fu;   /* 0.115 */
		} else if (DAT_002bae70 == 1) {
			if (STAGE() > 2)
				FH_AI(self, 0x11f8) = 0;
			else
				FH_AU(self, 0x11f8) = 0x3e2e147bu;   /* 0.17 */
		} else if (DAT_002bae70 == 2) {
			if (STAGE() < 3)
				FH_AU(self, 0x11f8) = 0x3db851ecu;   /* 0.09 */
			else
				FH_AU(self, 0x11f8) = 0xbe4ccccdu;   /* -0.2 */
		}
	} else {
		FH_AI(self, 0x11f8) = 0;
	}
	if (FH_AI(self, 0x11ac) != 1)
		return;
	{
		float skill = FH_F(0x00302150) + FH_AF(self, 0x11f8);

		if (STAGE() == 4)
			skill = skill + 0.17f;
		FH_AF(self, 0x10e0) = skill;
	}
	if (FH_AI(self, 0x11c4) == 1) {
		if (STAGE() != 4)
			FH_AF(self, 0x10e0) = FH_F(0x00302150) + FH_AF(self, 0x11f8) + 0.17f;
		if (FH_AI(self, 0x11c4) == 1 && STAGE() == 4)
			FH_AF(self, 0x10e0) = FH_F(0x00302150) + FH_AF(self, 0x11f8);
	}
	if (FH_AI(self, 0x11bc) == 1)
		FH_AF(self, 0x10e0) = FH_F(0x00302150) + FH_AF(self, 0x11f8) + 0.2f;
	if (FH_AI(self, 0x11c4) == 1 && STAGE() != 4 && FH_AI(self, 0x11bc) == 1)
		FH_AF(self, 0x10e0) = FH_F(0x00302150) + FH_AF(self, 0x11f8) + 0.37f;
	FUN_00130930(self);
}

/* 0x00131240  (gap): updates every player */
void FoxHunt_UpdatePlayers(void)
{
	int i;

	for (i = 0; i < DAT_002bae94; i++)
		FoxHunt_Player_Update(fh_Players[i]);
}

/* 0x00131270  (gap; vtable slot 17): respawns the player at respawn point `point` (or high
in the air without one), with a small random offset and the player standing in the way
pushed aside */
void FoxHunt_Player_RespawnAt(void *self, char *point)
{
	char *actor;
	char *other;
	float d[4];
	float h;
	unsigned int i;

	if (point == NULL) {
		FH_AF(ACT(self), 0xd8) = 0.0f;
		FH_AF(ACT(self), 0xd0) = 0.0f;
		FH_AF(ACT(self), 0xd4) = 8000.0f;
		return;
	}
	actor = ACT(self);
	FH_AI(actor, 0xd0) = FH_AI(point, 0x8);
	FH_AI(actor, 0xd4) = FH_AI(point, 0xc);
	FH_AI(actor, 0xd8) = FH_AI(point, 0x10);
	FH_AI(actor, 0xdc) = FH_AI(point, 0x14);
	FH_AS(point, 0x18) = FH_AS(ACT(self), 0x230);
	FH_AF(point, 0x1c) = DAT_002939bc;
	FH_AF(ACT(self), 0xd0) = (float)(fh_rand() % 0x65 - 0x32) * 0.01f + FH_AF(ACT(self), 0xd0);
	FH_AF(ACT(self), 0xd8) = (float)(fh_rand() % 0x65 - 0x32) * 0.01f + FH_AF(ACT(self), 0xd8);
	actor = ACT(self);
	FH_AF(ACT(self), 0xd4) = FH_AF(actor, 0xd4) - CLColl_GroundProbe((float *)(point + 8), 0.0f, NULL, NULL, 0);
	actor = ACT(self);
	FH_AI(actor, 0x1a4) = FH_AI(actor, 0xd0);
	FH_AI(actor, 0x1a8) = FH_AI(actor, 0xd4);
	FH_AI(actor, 0x1ac) = FH_AI(actor, 0xd8);
	FH_AI(actor, 0x1b0) = FH_AI(actor, 0xdc);
	FH_AI(actor, 0x1b4) = FH_AI(actor, 0xd0);
	FH_AI(actor, 0x1b8) = FH_AI(actor, 0xd4);
	FH_AI(actor, 0x1bc) = FH_AI(actor, 0xd8);
	FH_AI(actor, 0x1c0) = FH_AI(actor, 0xdc);
	FH_AI(ACT(self), 0x19c) = 0;
	FH_AI(ACT(self), 0x198) = 0;
	FH_AI(ACT(self), 0x194) = 0;
	FH_AS(self, 0x10b8) = 1;
	AI_SetState(self, 0);
	for (i = 0; (int)i < DAT_002bae94; i++) {
		actor = ACT(self);
		if (i == FH_AUS(actor, 0x230))
			continue;
		other = FH_AP(DAT_002f289c[i], 0x30);
		d[0] = FH_AF(other, 0xd0) - FH_AF(actor, 0xd0);
		d[2] = FH_AF(other, 0xd8) - FH_AF(actor, 0xd8);
		d[1] = 0.0f;
		other = FH_AP(DAT_002f289c[i], 0x30);
		h = sqrtf(d[0] * d[0] + d[2] * d[2]);
		if (h < FH_AF(FH_AP(other, 0x18c), 0x9c) + FH_AF(FH_AP(actor, 0x18c), 0x9c)) {
			if (h == 0.0f)
				FH_AF(other, 0x194) = FH_AF(other, 0x194) + 20.0f;
			else
				bmVectorBlend(FH_AV(other, 0x194), FH_AV(other, 0x194), 1.0f, d, 20.0f / h);
		}
	}
}

/* 0x00131530  (gap): debug: draws two lines from above the player to the centres of its
route cells (+0x1046 the goal, +0x1042 the next) */
void FoxHunt_Player_DrawRoute(void *self)
{
	char *actor = ACT(self);
	float x = FH_AF(actor, 0xd0);
	float z = FH_AF(actor, 0xd8);
	short goal = FH_AS(self, 0x1046);
	float ground;
	float y;
	float centre[4];

	ground = CLColl_GroundProbe(POS(actor), 50.0f, NULL, NULL, 0);
	CLRoute_CellCentre(centre, (int)goal);
	bdSetRenderState(5, 0, 0);
	bdSetWorldIdentity();
	y = ground + 50.0f;
	FH_F(0x00302290) = centre[1] + 100.0f;
	FH_U(0x00302280) = DAT_002bca94 | DAT_002bd640[0] | DAT_002bc490[0x7f] | DAT_002bce18[0];
	FH_F(0x0030228c) = centre[0];
	FH_F(0x00302294) = centre[2];
	FH_U(0x003022a4) = DAT_002bc490[0] | DAT_002bce18[0x7f] | DAT_002bca94 | DAT_002bd640[0];
	FH_F(0x00302268) = x;
	FH_F(0x0030226c) = y;
	FH_F(0x00302270) = z;
	bdDrawPrimitiveFVF(1, FH_ADDR(0x00302268), 2, 0x152);
	CLRoute_CellCentre(centre, (int)FH_AS(self, 0x1042));
	FH_U(0x00302238) = DAT_002bca94 | DAT_002bd640[0] | DAT_002bc490[0x7f] | DAT_002bce18[0];
	FH_F(0x00302248) = centre[1] + 100.0f;
	FH_F(0x00302244) = centre[0];
	FH_F(0x0030224c) = centre[2];
	FH_U(0x0030225c) = DAT_002bc490[0] | DAT_002bce18[0x7f] | DAT_002bca94 | DAT_002bd640[0];
	FH_F(0x00302220) = x;
	FH_F(0x00302224) = y;
	FH_F(0x00302228) = z;
	bdDrawPrimitiveFVF(1, FH_ADDR(0x00302220), 2, 0x152);
	bdSetRenderState(5, 1, 0);
}

/* 0x001316e0  (gap; vtable slot 17): switches the player's model: 0 the tag model (with
the flag), 1 the normal one; keeps its place and colours */
void FoxHunt_Player_SetModel(void *self, int normal)
{
	char *actor = ACT(self);
	char *model;
	float pos[4];

	if (normal == 0)
		model = FH_PV(0x002ab26c)[CHAR_OF(FH_AUS(actor, 0x230)) * 4];
	else
		model = FH_PV(0x002ab274)[CHAR_OF(FH_AUS(actor, 0x230)) * 4];
	if (model != NULL) {
		pos[0] = FH_AF(actor, 0xd0);
		pos[1] = FH_AF(actor, 0xd4);
		pos[2] = FH_AF(actor, 0xd8);
		pos[3] = FH_AF(actor, 0xdc);
		baDeleteActorInstance((struct BActorInstance *)(actor + 0x10));
		Actor_Create(model, pos, ACT(self), 1, 0, 0);
		Actor_PlayAnim(ACT(self), 0, 0, 0, 1.0f, 0.0f, 0);
		baUpdateAnimations((struct BActorInstance *)(ACT(self) + 0x10), (struct BActorNode *)FH_AP(ACT(self), 0x144), 0.0f, NULL);
		baPrepActorInstance((struct BActorInstance *)(ACT(self) + 0x10), 1);
	}
	if (normal != 0 && model != NULL) {
		if (FH_PV(0x002ab26c)[CHAR_OF(FH_AUS(ACT(self), 0x230)) * 4] != NULL)
			baSetInstanceAnimations((struct BActor *)FH_PV(0x002ab26c)[CHAR_OF(FH_AUS(ACT(self), 0x230)) * 4],
				(struct BActorInstance *)(ACT(self) + 0x10));
	}
	if (DAT_00283ea4 != 0) {
		FH_I(0x003021a0) = DAT_00283ea4;
		Core_SetCharacterColours(ACT(self) + 0x10, DAT_00283ea4);
	}
}

/* 0x00131810  (gap): variation 2: picks what to do: with the ball, a bonus or a goal; else
fetch the ball, or hunt the most important player (mode 7) */
void FoxHunt_Player_ChooseEnergyBall(void *self)
{
	float ball[4];
	int best = -1, who, i;

	if (FH_AI(self, 0x11c4) == 1) {
		FoxHunt_Player_ChooseBonus(self);
		return;
	}
	if (FH_AF(self, 0x1180) <= fh_randf())
		FH_AI(self, 0x11f4) = 2;
	else
		FH_AI(self, 0x11f4) = 1;
	i = FUN_001369c0(POS(ACT(self)), (undefined4 *)ball);
	if (i > 0)
		FH_AI(self, 0x4) = 5;
	if (i != 0)
		return;
	score_targets(self);
	for (i = 0; i < 4; i++) {
		char *p = fh_Players[i];
		int *s = &FH_AI(self, 0x115c + i * 4);

		if (*s > -1) {
			if (FH_AI(p, 0x11c4) == 1)
				*s += 2;
			if (FH_AI(p, 0x4) == 0xb)
				*s += 1;
			if (FH_AI(p, 0x4) == 7)
				*s -= 1;
			if (FH_AI(p, 0x11dc) == 1)
				*s += 1;
			if (FH_AI(p, 0x11e0) == 1)
				*s += 1;
			if (FH_AI(p, 0x11e4) == 1)
				*s += 1;
			if (FH_AI(self, 0x11d0) < FH_AI(p, 0x11d0))
				*s += 1;
		}
	}
	if (FH_AI(self, 0x115c) < 0) {
		who = -1;
	} else {
		who = 0;
		best = FH_AI(self, 0x115c);
	}
	if (best < FH_AI(self, 0x1160)) {
		who = 1;
		best = FH_AI(self, 0x1160);
	}
	if (best < FH_AI(self, 0x1164)) {
		who = 2;
		best = FH_AI(self, 0x1164);
	}
	if (best < FH_AI(self, 0x1168)) {
		FH_AI(self, 0x18) = 3;
		FH_VFN(self, 0x18, void (*)(void *, int))(self, 7);
		return;
	}
	if (who > -1) {
		FH_AI(self, 0x18) = who;
		FH_VFN(self, 0x18, void (*)(void *, int))(self, 7);
	}
}

/* 0x00131a20  (gap): variation 3: with the flag, keep away from the nearest player; else
retaliate (vengeance) or chase the flag (mode 5) */
void FoxHunt_Player_ChooseOddOneOut(void *self)
{
	if (FH_AI(self, 0x11c4) == 1) {
		if (FH_AF(self, 0x1180) <= fh_randf()) {
			FH_AI(self, 0x11f4) = 2;
			FoxHunt_Player_AvoidNearest(self);
			return;
		}
		FH_AI(self, 0x11f4) = 1;
		FoxHunt_Player_AvoidNearest(self);
		return;
	}
	if (fh_randf() < FH_AF(self, 0x1178) && FoxHunt_Player_Retaliate(self) != 0)
		return;
	FH_AI(self, 0x4) = 5;
	FH_AI(self, 0x28) = 0;
}

/* 0x00131af0  (gap): variation 4: with the flag (the victim), retaliate or pass the flag on
to the best target (mode 7); else run away (mode 6) */
void FoxHunt_Player_ChooseVictim(void *self)
{
	int best = 0;
	unsigned int who, i;
	int r;

	if (FH_AI(self, 0x11c4) == 1) {
		r = fh_rand();
		if (FH_AF(self, 0x1178) <= (float)r * 3.05185094e-05f || FoxHunt_Player_Retaliate(self) == 0) {
			FH_AI(self, 0x11ec) = 0;
			FH_AI(self, 0x18) = -1;
			if (FH_AI(self, 0x14) > 0) {
				for (i = 0; i < 4; i++) {
					if (i == FH_AUS(ACT(self), 0x230))
						FH_AI(self, 0x115c + i * 4) = -2;
					else
						FH_AI(self, 0x115c + i * 4) = (FH_AI(fh_Players[i], 0x11ac) != 0) - 1;
				}
				for (i = 0; i < 4; i++) {
					char *p = fh_Players[i];
					int *s = &FH_AI(self, 0x115c + i * 4);
					float d;

					if (*s > -1) {
						if (FH_AI(self, 0x1148) == 4 && FH_AI(p, 0x1148) == 4)
							*s += 1;
						if (FH_AS(p, 0x36) == 0xd)
							*s += 1;
						if (FH_AI(self, 0x1148) != 4 && FH_AI(p, 0x1148) != 4)
							*s += 1;
						d = bmVectorDistance(POS(ACT(self)), POS(ACT(p)));
						if (d < 600.0f)
							*s += 1;
						if (d < 300.0f)
							*s += 1;
					}
				}
				who = (unsigned int)r;   /* the original reuses the rand() slot here */
				if (FH_AI(self, 0x115c) > 0) {
					who = 0;
					best = FH_AI(self, 0x115c);
				}
				if (best < FH_AI(self, 0x1160)) {
					who = 1;
					best = FH_AI(self, 0x1160);
				}
				if (best < FH_AI(self, 0x1164)) {
					who = 2;
					best = FH_AI(self, 0x1164);
				}
				if (best < FH_AI(self, 0x1168)) {
					FH_AI(self, 0x18) = 3;
					FH_AI(self, 0x4) = 7;
					return;
				}
				if ((int)who > -1 && who < 4)
					FH_AI(self, 0x18) = (int)who;
			}
			FH_AI(self, 0x4) = 7;
			return;
		}
		return;
	}
	if (fh_randf() < FH_AF(self, 0x1180)) {
		FH_AI(self, 0x11f4) = 1;
		FH_AI(self, 0x4) = 6;
		return;
	}
	FH_AI(self, 0x11f4) = 2;
	FH_AI(self, 0x4) = 6;
}

static void copy4(float *dst, const float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}

/* the shared route target (0x00302200) */
static void route_to_shared(void *self, const float *a, int with_w, float w)
{
	FH_F(0x00302200) = a[0];
	FH_F(0x00302204) = a[1];
	FH_F(0x00302208) = a[2];
	if (with_w)
		FH_F(0x0030220c) = w;
	FoxHunt_Player_RouteTo(self, FH_FV(0x00302200));
}

/* the nearest other player (team mates too unless `skip_team`); returns -1 for none */
static int nearest_player(void *self, float *best, int skip_team)
{
	unsigned int i;
	int who = -1;
	float d;

	*best = 100000.0f;
	FH_AI(self, 0x84) = 0;
	FH_AI(self, 0x80) = 0;
	FH_AI(self, 0x7c) = 0;
	for (i = 0; i < 4; i++) {
		unsigned int me = FH_AUS(ACT(self), 0x230);

		if (i == me)
			continue;
		if (skip_team && DAT_002baebc != 0 && DAT_002baef4[i * 10] == DAT_002baef4[me * 10])
			continue;
		d = bmVectorDistance(POS(ACT(self)), POS(ACT(fh_Players[i])));
		if (d < *best) {
			*best = d;
			who = (int)i;
		}
	}
	return who;
}

/* 0x00131d30  (gap; vtable slot 7, asked by AI_Think): plans for the player's mode
(+0x04): 4 retaliate, 5 fetch the flag/ball, 6 avoid / patrol, 7 hunt a player, 9 fetch a
gate bonus, 0xb take the flag to a goal. Returns 1 to ask for a new mode. */
int FoxHunt_Player_Plan(void *self)
{
	float a[4] = { 0.0f, 0.0f, 0.0f, 0.0f };   /* stack vectors of the original */
	float b[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float c[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float best = 0.0f;
	float d1, d2;
	float *src;
	char *goal_a = FH_P(0x00302194);
	char *goal_b = FH_P(0x0030219c);
	char *other;
	int who, kind = 0, ga;
	short t;
	unsigned int me;

	if ((float)FH_I(0x002ac0c0) < FH_AF(self, 0x20))
		return 1;
	if (FH_AI(self, 0x24) != -1 && (float)FH_AI(self, 0x24) < FH_AF(self, 0x20))
		return 1;
	if (FH_AI(self, 0x4) != 4 && FH_AS(self, 0x36) == 10 && FH_AI(self, 0x3c) == 1)
		return 1;
	switch (FH_AI(self, 0x4)) {
	case 0:
		return 1;
	default:
		return 0;

	case 4:
		/* retaliate against +0x1088 */
		t = FH_AS(self, 0x1088);
		if (t == -1)
			return 1;
		if (FH_AS(self, 0x1086) != t &&
			(FH_AS(fh_Players[t], 0x36) != 0xd || FH_AF(self, 0x117c) <= Core_RandUnit())) {
			if ((float)FH_I(0x002ac0bc) <= FH_AF(self, 0x20)) {
				FH_AS(self, 0x1088) = -1;
				return 1;
			}
			if (DAT_002baebc == 0 ||
				DAT_002baef4[REC(self) * 10] != DAT_002baef4[FH_AS(self, 0x1088) * 10]) {
				src = POS(ACT(fh_Players[FH_AS(self, 0x1088)]));
				route_to_shared(self, src, 1, src[3]);
				return 0;
			}
		}
		FH_AS(self, 0x1088) = -1;
		FH_AS(self, 0x1086) = -1;
		return 1;

	case 5:
		/* fetch the flag (ball) */
		if (FH_AI(self, 0x11c4) != 1) {
			if (FH_AI(self, 0x11c4) == 0 && STAGE() < 3) {
				if (FUN_001369c0(POS(ACT(self)), (undefined4 *)a) == 0) {
					FUN_00136a80(POS(ACT(self)), (undefined4 *)a);
					route_to_shared(self, a, 1, a[3]);
					return 0;
				}
				FoxHunt_Player_RouteTo(self, a);
				return 0;
			}
			if (FUN_001369c0(POS(ACT(self)), (undefined4 *)a) == 0)
				FUN_00136a80(POS(ACT(self)), (undefined4 *)a);
			a[1] = a[1] - CLColl_GroundProbe(a, 50.0f, NULL, NULL, 0);
			if (a[1] > 390.0f && FH_AI(self, 0x1148) != 4) {
				FoxHunt_Player_UseEscalator(self, a);
				return 0;
			}
			if (CLRoute_CellAt(a) != FH_AS(self, 0x1042))
				FoxHunt_Player_RouteTo(self, a);
			if (FH_AF(ACT(self), 0xd4) >= 390.0f && FH_AI(self, 0x1148) != 3) {
				FH_AI(self, 0x1148) = 4;
				return 0;
			}
			FH_AI(self, 0x1148) = 0;
			return 0;
		}
		if (STAGE() == 1) {
			FoxHunt_Player_ChooseCooperation(self);
			return 0;
		}
		if (STAGE() == 2) {
			FoxHunt_Player_ChooseEnergyBall(self);
			return 0;
		}
		if (STAGE() == 3)
			FoxHunt_Player_ChooseOddOneOut(self);
		return 0;

	case 6:
		/* keep away: the town wanders, the beach patrols (or faces and hits) */
		FH_AI(self, 0x1158) = 0;
		if (STAGE() < 3) {
			if (FH_AS(self, 0x1046) != FH_AS(self, 0x1040))
				return 0;
			CLRoute_RandomPoint(a, 1);
			route_to_shared(self, a, 1, a[3]);
			return 0;
		}
		if (STAGE() < 3)
			return 0;
		if (FH_AI(self, 0x11bc) == 1 && FH_AI(self, 0x11f4) == 4)
			PRESS(self, 0x6c);
		if (FoxHunt_Button_IsNear(FH_P(0x003022fc), ACT(self)) != 0 && Core_RandUnit() < FH_AF(self, 0x1180))
			PRESS(self, 0x6c);
		if (FH_AI(self, 0x11c4) == 1 && STAGE() == 4) {
			FH_AI(self, 0x4) = 7;
			return 1;
		}
		if (FH_AI(self, 0x11bc) != 1 || FH_AI(self, 0x11f4) != 3) {
			FH_AF(self, 0x1154) = DAT_0028fda8 + FH_AF(self, 0x1154);
			if (60.0f < FH_AF(self, 0x1154))
				FoxHunt_Player_NextPatrolPoint(self);
			b[1] = FH_FV(0x002ab3b0)[FH_AI(self, 0x1150) * 3 + 1];
			b[0] = FH_FV(0x002ab3b0)[FH_AI(self, 0x1150) * 3];
			b[2] = FH_FV(0x002ab3b0)[FH_AI(self, 0x1150) * 3 + 2];
			if (b[1] > 390.0f && FH_AI(self, 0x1148) != 4) {
				FoxHunt_Player_UseEscalator(self, b);
				return 0;
			}
			if (CLRoute_CellAt(b) == FH_AS(self, 0x1042))
				return 0;
			FoxHunt_Player_RouteTo(self, b);
			return 0;
		}
		who = nearest_player(self, &best, 0);
		if (who < 0)
			return 0;
		FH_AF(ACT(self), 0x1e8) = CLMath_HeadingTo(POS(ACT(self)), POS(ACT(fh_Players[who])));
		FH_AF(ACT(self), 0x1e8) = CLMath_WrapAngle(FH_AF(ACT(self), 0x1e8));
		goto throw_check;

	case 7:
		/* hunt player +0x18 */
		if (FH_AI(self, 0x18) == -1 || FH_AS(fh_Players[FH_AI(self, 0x18)], 0x36) == 10) {
			FH_AS(self, 0x1086) = -1;
			FH_AI(self, 0x18) = FoxHunt_Player_RandomOther(self, 1);
		}
		if ((DAT_002baebc != 0 && DAT_002baef4[REC(self) * 10] == DAT_002baef4[FH_AI(self, 0x18) * 10]) ||
			(int)FH_AS(self, 0x1086) == FH_AI(self, 0x18) || FH_AS(fh_Players[FH_AI(self, 0x18)], 0x36) == 0xd) {
			FH_AI(self, 0x18) = -1;
			FH_AS(self, 0x1086) = -1;
			return 1;
		}
		other = fh_Players[FH_AI(self, 0x18)];
		src = POS(ACT(other));
		if (STAGE() > 2) {
			copy4(a, src);
			if (a[1] > 380.0f && FH_AI(self, 0x1148) != 4) {
				FoxHunt_Player_UseEscalator(self, a);
				return 0;
			}
			copy4(a, POS(ACT(other)));
			FoxHunt_Player_RouteTo(self, a);
			if (390.0f < FH_AF(ACT(self), 0xd4)) {
				FH_AI(self, 0x1148) = 4;
				return 0;
			}
			FH_AI(self, 0x1148) = 0;
			return 0;
		}
		route_to_shared(self, src, 1, src[3]);
		return 0;

	case 9:
		/* fetch gate bonus +0x1174 */
		if (FH_AI(self, 0x11d8 + FH_AI(self, 0x1174) * 4) == 1 || FH_AI(self, 0x11c4) != 1) {
			FoxHunt_Player_ChooseEnergyBall(self);
			return 0;
		}
		if (FH_AI(self, 0x11bc) == 1 && FH_AI(self, 0x11f4) == 4)
			PRESS(self, 0x6c);
		FUN_001385d0((undefined4 *)a, FH_AI(self, 0x1174));
		route_to_shared(self, a, 1, a[3]);
		return 0;

	case 0xb:
		/* take the flag to a goal */
		if (FH_AI(self, 0x11c4) != 1) {
			if (STAGE() != 2 && STAGE() != 1)
				return 0;
			FH_AI(self, 0x4) = 5;
			return 0;
		}
		if (FH_AI(self, 0x11bc) == 1 && FH_AI(self, 0x11f4) == 4)
			PRESS(self, 0x6c);
		kind = FH_AI(self, 0x24);   /* the original's register still holds +0x24 here */
		if (FH_AI(goal_a, 0x34) == -1) {
			copy4(a, FH_AV(goal_a, 0x14));
			kind = 1;
		}
		if (FH_AI(goal_b, 0x34) == -1) {
			copy4(a, FH_AV(goal_b, 0x14));
			kind = 1;
		}
		ga = FH_AI(goal_a, 0x34);
		if (ga == -1 && FH_AI(goal_b, 0x34) == -1)
			kind = 2;
		if (STAGE() == 2) {
			d1 = bmVectorDistance(POS(ACT(self)), FH_AV(goal_a, 0x14));
			d2 = bmVectorDistance(POS(ACT(self)), FH_AV(goal_b, 0x14));
			if (d1 < d2) {
				copy4(a, FH_AV(goal_a, 0x14));
				FoxHunt_Player_RouteTo(self, a);
				return 0;
			}
			copy4(a, FH_AV(goal_b, 0x14));
			FoxHunt_Player_RouteTo(self, a);
			return 0;
		}
		if (STAGE() != 1)
			return 0;
		if (FH_AI(self, 0x11b8) == 0) {
			me = REC(self);
			if (kind != 1) {
				char *far;

				if (kind != 2)
					return 0;
				d1 = bmVectorDistance(POS(ACT(self)), FH_AV(goal_a, 0x14));
				d2 = bmVectorDistance(POS(ACT(self)), FH_AV(goal_b, 0x14));
				if (d2 <= d1) {
					copy4(a, FH_AV(goal_b, 0x14));
					far = goal_a;
				} else {
					copy4(a, FH_AV(goal_a, 0x14));
					far = goal_b;
				}
				copy4(b, FH_AV(far, 0x14));
				FUN_00136960((undefined4 *)c, FH_AI(self, 0x11c8) == 1 ? 2 : 1);
				d1 = bmVectorDistance(POS(ACT(self)), a);
				d2 = bmVectorDistance(c, a);
				if (d2 < d1) {
					FoxHunt_Player_RouteTo(self, b);
					return 0;
				}
				FoxHunt_Player_RouteTo(self, a);
				return 0;
			}
			if (DAT_002baebc == 0) {
				if (0.9f < FH_AF(self, 0x1180)) {
					if (ga < 0) {
						if (FH_AI(goal_b, 0x34) < 0)
							return 0;
						if (FH_AI(fh_Players[FH_AI(goal_b, 0x34)], 0x11d0) <= FH_AI(self, 0x11d0)) {
							FoxHunt_Player_RouteTo(self, a);
							return 0;
						}
					} else if (FH_AI(fh_Players[ga], 0x11d0) <= FH_AI(self, 0x11d0)) {
						FoxHunt_Player_RouteTo(self, a);
						return 0;
					}
					goto hunt_goal_a;
				}
			} else {
				if (ga > -1) {
					if (DAT_002baef4[ga * 10] != DAT_002baef4[me * 10]) {
						FH_AI(self, 0x4) = 7;
						FH_AI(self, 0x18) = FH_AI(goal_a, 0x34);
						return 0;
					}
					FoxHunt_Player_RouteTo(self, a);
					return 0;
				}
				if (FH_AI(goal_b, 0x34) < 0) {
					bkPrintf("No goals occupied?! something is wrong with this picture\n");
					return 0;
				}
				if (DAT_002baef4[FH_AI(goal_b, 0x34) * 10] != DAT_002baef4[me * 10])
					goto hunt_goal_a;
			}
			FoxHunt_Player_RouteTo(self, a);
			return 0;
hunt_goal_a:
			FH_AI(self, 0x4) = 7;
			FH_AI(self, 0x18) = FH_AI(goal_a, 0x34);
			return 0;
		}
		/* at a goal: face the nearest enemy and hit or throw */
		who = nearest_player(self, &best, 1);
		if (who > -1) {
			float h = CLMath_HeadingTo(POS(ACT(self)), POS(ACT(fh_Players[who])));

			FH_AF(ACT(self), 0x1e8) = h;
			FH_AF(ACT(self), 0x1d8) = h;
			FH_AF(ACT(self), 0x1e8) = CLMath_WrapAngle(FH_AF(ACT(self), 0x1e8));
		}
		if (FH_AI(self, 0x11bc) != 1) {
			if (FH_AF(self, 0x117c) <= Core_RandUnit())
				return 0;
			FH_VFN(self, 0x14, void (*)(void *))(self);
			return 0;
		}
		goto throw_check;
	}
throw_check:
	if (FoxHunt_Throwable_CanThrow(fh_Throwables[FH_AI(self, 0x11c0)]) == 0)
		return 0;
	if (400.0f <= best)
		return 0;
	PRESS(self, 0x6c);
	return 0;
}

/* the victim `i` is marked as hit by the player (for revenge and the hit counts) */
static void mark_hit(void *self, unsigned int i)
{
	short *count;

	FH_AS(self, 0x1086) = (short)i;
	FH_AS(fh_Players[i], 0x1088) = FH_AS(ACT(self), 0x230);
	count = (short *)(fh_Players[i] + 0x108a + (unsigned int)FH_AUS(ACT(self), 0x230) * 2);
	*count = (short)(*count + 1);
}

/* 0x00132a40  (gap; vtable slot 4): the player's attack: the first other player in reach
in front of it is pushed back and hit; the flag (ball) changes hands by the rules of the
variation (Victim: the flag passes to the one hit; 1-hit or 3-hit steal in the town).
Returns 1 when it hit someone. */
int FoxHunt_Player_Attack(void *self)
{
	unsigned int i;
	char *me;
	char *victim;
	char *va;
	char *node;
	unsigned int rec;
	float angle, dist, len;
	float push[4];
	const char *msg;

	FH_AS(self, 0x1092) = 1;
	for (i = 0; (int)i < DAT_002bae94; i++) {
		me = ACT(self);
		if (i == FH_AUS(me, 0x230))
			continue;
		victim = fh_Players[i];
		if (FH_AS(victim, 0x36) == 0xd) {
			/* down: only Victim's flag carrier can pass the flag on */
			if (STAGE() != 4 || FH_AI(self, 0x11c4) != 1 ||
				!(fabsf(FH_AF(me, 0xd4) - FH_AF(ACT(victim), 0xd4)) < FH_AF(self, 0x1100)))
				continue;
			angle = CLMath_AngleDistance(CLMath_HeadingTo(POS(me), POS(ACT(victim))), FH_AF(ACT(self), 0x1e8));
			if (!(angle < FH_AF(self, 0x10fc)))
				continue;
			dist = bmVectorDistance(POS(ACT(self)), POS(ACT(fh_Players[i])));
			if (!((angle + 1.0f) * dist < CHAR_F(REC(self), 0x10)))
				continue;
			me = ACT(self);
			Core_Vec3Sub(push, POS(ACT(fh_Players[i])), POS(me));
			rec = FH_AUS(me, 0x230);
			if (FH_AF(self, 0x10a4) == 0.0f)
				len = CHAR_F(rec, 0x14);
			else
				len = CHAR_F(rec, 0x14) + CHAR_F(rec, 0x14);
			bmVectorSetLength(push, push, len);
			va = ACT(fh_Players[i]) + 0x194;
			cmb_vec3_add((float *)va, (float *)va, push);
			FH_AI(self, 0x11c4) = 0;
			FUN_00136b40((int)i, FH_AI(self, 0x11c8));
			FH_AI(self, 0x11c8) = -1;
			FH_AI(fh_Players[i], 0x11cc) = FH_AI(fh_Players[i], 0x11d4);
			mark_hit(self, i);
			return 1;
		}
		if (FH_AF(victim, 0x10a8) != 0.0f)
			continue;
		if (!(fabsf(FH_AF(me, 0xd4) - FH_AF(ACT(victim), 0xd4)) < FH_AF(self, 0x1100)))
			continue;
		angle = CLMath_AngleDistance(CLMath_HeadingTo(POS(me), POS(ACT(victim))), FH_AF(ACT(self), 0x1e8));
		if (!(angle < FH_AF(self, 0x10fc)))
			continue;
		dist = bmVectorDistance(POS(ACT(self)), POS(ACT(fh_Players[i])));
		if (!((angle + 1.0f) * dist < FH_AF(self, 0x10f4)))
			continue;
		/* hit: push the victim back */
		va = ACT(fh_Players[i]);
		me = ACT(self);
		push[0] = FH_AF(va, 0xd0) - FH_AF(me, 0xd0);
		push[1] = FH_AF(va, 0xd4) - FH_AF(me, 0xd4);
		push[2] = FH_AF(va, 0xd8) - FH_AF(me, 0xd8);
		rec = FH_AUS(me, 0x230);
		if (FH_AF(self, 0x10a4) == 0.0f)
			len = CHAR_F(rec, 0x14);
		else
			len = CHAR_F(rec, 0x14) + CHAR_F(rec, 0x14);
		bmVectorSetLength(push, push, len);
		va = ACT(fh_Players[i]);
		FH_AF(va, 0x194) = push[0] + FH_AF(va, 0x194);
		FH_AF(va, 0x198) = push[1] + FH_AF(va, 0x198);
		FH_AF(va, 0x19c) = push[2] + FH_AF(va, 0x19c);
		victim = fh_Players[i];
		if (FH_AI(victim, 0x11bc) == 1) {
			/* it drops what it holds */
			FH_AI(fh_Throwables[FH_AI(victim, 0x11c0)], 0xc) = 2;
			FH_AI(fh_Players[i], 0x11bc) = 0;
			FH_AI(fh_Players[i], 0x11c0) = -1;
		}
		victim = fh_Players[i];
		if (FH_AI(victim, 0x11c4) == 1 && STAGE() != 4) {
			if (FH_AI(self, 0x11c4) == 1) {
				/* both carry a ball: the victim drops it */
				FH_AI(victim, 0x11c4) = 2;
				FH_AI(fh_Players[i], 0x11dc) = 0;
				FH_AI(fh_Players[i], 0x11e0) = 0;
				FH_AI(fh_Players[i], 0x11e4) = 0;
				if (STAGE() == 2)
					FUN_001382a0();
			} else if (FH_AI(self, 0x11c4) == 0) {
				if (FH_I(0x00302160) == 1) {
					if (CHAR_F(REC(self), 0x1c) + FH_AF(victim, 0x111c) <=
						CHAR_F(FH_AUS(ACT(victim), 0x230), 0x20)) {
						bkPrintf("3hit mode but no steal\n");
						goto steal_done;
					}
					msg = "3 hit mode STEAL!\n";
				} else if (FH_I(0x00302160) == 0) {
					msg = "1 hit steal mode gotta be a steal \n";
				} else {
					goto steal_done;
				}
				/* steal the flag and its gate bonuses */
				bkPrintf(msg);
				FH_AI(fh_Players[i], 0x11c4) = 0;
				FH_AI(self, 0x11dc) = FH_AI(fh_Players[i], 0x11dc);
				FH_AI(fh_Players[i], 0x11dc) = 0;
				FH_AI(self, 0x11e0) = FH_AI(fh_Players[i], 0x11e0);
				FH_AI(fh_Players[i], 0x11e0) = 0;
				FH_AI(self, 0x11e4) = FH_AI(fh_Players[i], 0x11e4);
				FH_AI(fh_Players[i], 0x11e4) = 0;
				FUN_00136b40((int)(short)FH_AUS(ACT(self), 0x230), FH_AI(fh_Players[i], 0x11c8));
				FH_AI(fh_Players[i], 0x11c8) = -1;
				FH_AI(fh_Players[i], 0x11c4) = 0;
				FH_AI(self, 0x11cc) = FH_AI(self, 0x11d4);
				CLSamples_PlayFromSet((char)DAT_002baed4[FH_AUS(ACT(self), 0x230) * 10], 6, NULL,
					DAT_00293928, 0, -1, 0);
			}
		}
steal_done:
		if (FH_AI(self, 0x11c4) == 1 && STAGE() == 4) {
			/* Victim: the flag passes to the one hit */
			FH_AI(self, 0x11c4) = 0;
			FUN_00136b40((int)i, FH_AI(self, 0x11c8));
			FH_AI(self, 0x11c8) = -1;
			FH_AI(fh_Players[i], 0x11cc) = FH_AI(fh_Players[i], 0x11d4);
		}
		if (DAT_002baebc == 0 || DAT_002baef4[i * 10] != DAT_002baef4[FH_AUS(ACT(self), 0x230) * 10])
			FoxHunt_Player_Hit(fh_Players[i], POS(ACT(self)), CHAR_F(REC(self), 0x1c), 0);
		else
			bkPrintf("Cant hurt YOU my friend!\n");
		mark_hit(self, i);
		return 1;
	}
	/* (the original also measures the actors of the second actor list; nothing comes of it) */
	for (node = fh_ListNext((unsigned int *)((char *)DAT_002ea488 + 0x18), NULL); node != NULL;
		node = fh_ListNext((unsigned int *)((char *)DAT_002ea488 + 0x18), node)) {
		if (FH_AI(node, 0x440) != 0 && fabsf(FH_AF(ACT(self), 0xd4) - FH_AF(node, 0xe4)) < FH_AF(self, 0x1100)) {
			angle = CLMath_AngleDistance(CLMath_HeadingTo(POS(ACT(self)), FH_AV(node, 0xe0)), FH_AF(ACT(self), 0x1e8));
			if (angle < FH_AF(self, 0x10fc))
				(void)bmVectorDistance(POS(ACT(self)), FH_AV(node, 0xe0));
		}
	}
	return 0;
}

/* 0x001331f0  (gap): picks the respawn point farthest from the other players and respawns
there (without respawn points: the default respawn, vtable slot 10) */
void FoxHunt_Player_Respawn(void *self)
{
	char *point;
	char *best_point = NULL;
	float best = 0.0f;
	float nearest, d;
	unsigned int i;

	FH_AI(ACT(self), 0x220) = 0x59;
	FH_AI(self, 0x11e8) = 0;
	if (FH_AI(DAT_002ea488, 0x38) == 0) {
		FH_VFN(self, 0x28, void (*)(void *, int))(self, 0);
		return;
	}
	for (point = fh_ListNext((unsigned int *)((char *)DAT_002ea488 + 0x30), NULL); point != NULL;
		point = fh_ListNext((unsigned int *)((char *)DAT_002ea488 + 0x30), point)) {
		nearest = 1e+16f;
		for (i = 0; (int)i < DAT_002bae94; i++) {
			if (i == FH_AUS(ACT(self), 0x230))
				continue;
			d = CLVec3_DistanceSqXZ(POS(FH_AP(DAT_002f289c[i], 0x30)), (float *)(point + 8));
			if (d < nearest)
				nearest = d;
		}
		if (best < nearest) {
			best = nearest;
			best_point = point;
		}
	}
	if (FH_AS(best_point, 0x18) != -1) {
		FH_AF(best_point, 0x1c) = DAT_0028fda8;
		RespawnPoint_Update(best_point);
	}
	FoxHunt_Player_RespawnAt(self, best_point);
}

/* 0x00133310  (gap; vtable slot 6): sets the AI mode (+0x04); -1 asks the variation's
chooser */
void FoxHunt_Player_SetMode(void *self, int mode)
{
	FH_AI(self, 0x8) = 0;
	FH_AI(self, 0x10) = 0;
	FH_AI(self, 0x20) = 0;
	FH_AI(self, 0x28) = 0;
	FH_AI(self, 0x24) = -1;
	if (mode != -1) {
		FH_AI(self, 0x4) = mode;
		return;
	}
	switch (STAGE()) {
	case 1:
		FoxHunt_Player_ChooseCooperation(self);
		return;
	case 2:
		FoxHunt_Player_ChooseEnergyBall(self);
		return;
	case 3:
		FoxHunt_Player_ChooseOddOneOut(self);
		return;
	case 4:
		FoxHunt_Player_ChooseVictim(self);
		return;
	}
}

/* 0x00133370  (gap): respawns the player (a "RESPAWNCHAR" sparkle in its colour and a
sound), resets its splash and HUD timer, and picks the nearest patrol point */
void FUN_00133370(int player)
{
	void *self = (void *)(uintptr_t)player;
	int best_i = -1;
	float best = 10000.0f;
	float point[4];
	float d;
	unsigned int a;
	int i;

	FoxHunt_Player_Respawn(self);
	DAT_00290e8c = DAT_002baed4[FH_AUS(ACT(self), 0x230) * 10];
	Debris_Add((struct DebrisType *)DAT_00291734, POS(ACT(self)), DAT_002ea478, 0, 0xffffffffu);
	DAT_00290e8c = -1;
	CLSamples_PlayGeneric(5, 0x7f, 0, 1, NULL);
	FH_AI(self, 0x11e8) = 0;
	FH_AF(DAT_002ecfac, 0xb8 + (unsigned int)FH_AUS(ACT(self), 0x230) * 4) = 180.0f;
	FH_AI(ACT(self), 0x220) = 0x59;
	for (a = 0x002ab3b4u, i = 0; a < 0x002ab468u; a += 0xc, i++) {
		point[0] = FH_F(a - 4);
		point[1] = FH_F(a);
		point[2] = FH_F(a + 4);
		d = bmVectorDistance(POS(ACT(self)), point);
		if (d < best) {
			best = d;
			best_i = i;
		}
	}
	if (best_i != -1) {
		FH_AI(self, 0x114c) = best_i;
		FH_AI(self, 0x1150) = best_i;
		return;
	}
	FH_AI(self, 0x114c) = 0;
	FH_AI(self, 0x1150) = 0;
}

/* 0x00133490  (gap): the position and heading of shark `id` (list 0x003022bc) */
void FoxHunt_SharkView(int id, float *pos, float *heading)
{
	char *node;
	char *actor;

	node = fh_ListNext(FH_LIST(0x003022bc), NULL);
	if (node == NULL)
		return;
	while (FH_AI(node, -8) != id) {
		node = fh_ListNext(FH_LIST(0x003022bc), node);
		if (node == NULL)
			return;
	}
	actor = FH_AP(node, 8);
	pos[0] = FH_AF(actor, 0xd0);
	pos[1] = FH_AF(actor, 0xd4);
	pos[2] = FH_AF(actor, 0xd8);
	pos[3] = FH_AF(actor, 0xdc);
	*heading = FH_AF(node, 0x1c);
}

/* 0x001334f0  (gap; used by other minigames too): the heading of the direction (x, z),
atan2 wrapped to [0, 2 pi) */
float FUN_001334f0(float x, float z)
{
	double a = atan2((double)x, (double)z);

	if ((double)6.28318501f < a)
		a = a - (double)6.28318501f;
	if (a < 0.0)
		a = a + (double)6.28318501f;
	return (float)a;
}

/* 0x0026d690  FoxHunt player vtable (the CharLib AI virtuals; see charlib_ai.h) */
const void *const vtable_0026d690[18] = {
	(const void *)FUN_001d5950, /* [0] update pre (empty) */
	(const void *)FUN_001d5950, /* [1] update post (empty) */
	(const void *)FUN_001d5950, /* [2] paused (empty) */
	(const void *)FUN_000b1860, /* [3] think (charlib) */
	(const void *)FUN_00132a40, /* [4] attack */
	(const void *)FUN_0012fe30, /* [5] act */
	(const void *)FUN_00133310, /* [6] set mode */
	(const void *)FUN_00131d30, /* [7] plan (wants a new mode) */
	(const void *)FUN_000b0560, /* [8] respawn at (charlib) */
	(const void *)FUN_000b0370, /* [9] respawn farthest (charlib) */
	(const void *)FUN_000b0cb0, /* [10] spawn (charlib) */
	(const void *)FUN_001d5950, /* [11] pre act (empty) */
	(const void *)FUN_000b06c0, /* [12] can move (charlib) */
	(const void *)FUN_000a6fa0, /* [13] returns 1 */
	(const void *)FUN_001ec260, /* [14] timer (empty) */
	(const void *)FUN_001ec260, /* [15] (empty) */
	(const void *)FUN_000b0770, /* [16] hold end (charlib) */
	(const void *)FUN_001316e0, /* [17] set model */
};
