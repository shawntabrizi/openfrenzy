/*
UNPLACED_METAPLAYER.C  <-  gap functions 0x00155ce0-0x00157be0 of
C:\BlitzParty\Game\Src\Meta\ (no source path in the XBE; probably CMetaPlayer.cpp)

- The puzzle runner (0x00155ce0-0x00155de0): checks each puzzle's condition and runs its
  actions (see puzzles.c).
- CMetaPlayer (0x1164 bytes, vtable 0x0026f890): the board game's player, a charlib AI
  player (AI_Ctor 0x000b01d0) with the state machine of unplaced_player.c. Fields past
  the AI player (see also meta_a.h):
    +0x1086 short last player hit      +0x1088 short last attacker
    +0x108a short[4] hits taken from each player
    +0x10a4 float super attack time    +0x10a8 float shield (speed-up) time
    +0x10bc carried orb (charlib actor, type 10 at +0x21c)
    +0x10c0 carried object (hammer)    +0x10d0 float hurt cooldown
    +0x1130 float AI goal cooldown     +0x1140 int orbs carried (0-4)
    +0x1144 float AI attack cooldown   +0x1148 CRayControl (orb beam)
    +0x114c float glow, +0x1150..0x115c glow colour (ints)
    +0x1160 the speed-up overlay (COverlay)
*/

#include "meta_a.h"

/* charlib AI_PlayVoice (FUN_000b0320, thiscall AI player: kind, volume, cooldown, ...) */
#define MA_VOICE(ai, kind, volume, cooldown) \
	FUN_000b0320((int)(ai), (kind), (volume), (cooldown), 0, 0xffffffff, 0)

/* the squared distance (bkernel FUN_00051540; see puzzles.c) */
#define MA_DISTANCE_SQ(a, b) (((float (*)(const float *, const float *))FUN_00051540)((a), (b)))

extern int *DAT_002ea488;

/* drop the orb beam (+0x1148) */
static void ma_free_beam(int ai)
{
	int beam = MA_I32(ai, 0x1148);

	if (beam != 0) {
		CRayControl_Destroy((struct CRayControl *)beam);
		MA_DELETE(beam);
		MA_I32(ai, 0x1148) = 0;
	}
}

/* ---------- the puzzle runner */

/* 0x00155ce0  run the due actions of a puzzle; nonzero while some are not done yet */
undefined1 FUN_00155ce0(int puzzle)
{
	unsigned int action;
	int done;

	done = 0;
	for (action = CLList_Next((unsigned int *)(puzzle + 0xc), NULL); action != 0;
		action = CLList_Next((unsigned int *)(puzzle + 0xc), (unsigned int *)action)) {
		if (MA_F32(action, 0xc) <= 0.0f) {
			done++;
			continue;
		}
		if (DAT_003075a0 != 0 || MA_F32(action, 0x10) == 0.0f)
			MA_F32(action, 0xc) = MA_F32(action, 0xc) - DAT_0028fda8;
		if (MA_F32(action, 0xc) <= 0.0f) {
			FUN_00154360((int)action);
			done++;
		}
	}
	return done != MA_I32(puzzle, 0x14);
}

/* 0x00155d70  update the current puzzle (DAT_003075a4) */
void FUN_00155d70(void)
{
	int puzzle;
	int running;

	puzzle = DAT_003075a4;
	for (;;) {
		if (MA_I16(puzzle, 0x18) != 0) {
			running = (int)FUN_001542a0((undefined4 *)MA_PTR(puzzle, 8));
			puzzle = DAT_003075a4;
			if (running != 0) {
				if (0 < MA_I16(DAT_003075a4, 0x18))
					MA_I16(DAT_003075a4, 0x18) = MA_I16(DAT_003075a4, 0x18) - 1;
				FUN_00153020(DAT_003075a4);
				puzzle = DAT_003075a4;
				MA_I32(DAT_003075a4, 0x1c) = 1;
			}
		}
		if (MA_I32(puzzle, 0x1c) == 0)
			return;
		running = FUN_00155ce0(DAT_003075a4);
		puzzle = DAT_003075a4;
		MA_I32(DAT_003075a4, 0x1c) = running;
		if (running != 0)
			return;
		DAT_003075a0 = 0;
	}
}

/* 0x00155de0  update all puzzles */
void FUN_00155de0(void)
{
	unsigned int puzzle;

	for (puzzle = CLList_Next((unsigned int *)DAT_003075b0, NULL); puzzle != 0;
		puzzle = CLList_Next((unsigned int *)DAT_003075b0, (unsigned int *)puzzle)) {
		DAT_003075a0 = 1;
		DAT_003075a4 = (int)puzzle;
		FUN_00155d70();
	}
}

/* ---------- CMetaPlayer */

/* 0x0026f890  vtable of CMetaPlayer (charlib AI player) */
const void *const vtable_0026f890[18] = {
	(const void *)FUN_001d5950, (const void *)FUN_00156690, (const void *)FUN_00156080,
	(const void *)FUN_000b1860, (const void *)FUN_001577c0, (const void *)FUN_00157bd0,
	(const void *)FUN_00158d90, (const void *)FUN_001586d0, (const void *)FUN_000b0560,
	(const void *)FUN_00156e90, (const void *)FUN_000b0cb0, (const void *)FUN_00155e40,
	(const void *)FUN_00155f70, (const void *)FUN_000a6fa0, (const void *)FUN_001560a0,
	(const void *)FUN_00156120, (const void *)FUN_00156f30, (const void *)FUN_001ec260,
};

/* 0x00155e30  destructor */
void FUN_00155e30(undefined4 *ai)
{
	*(const void **)ai = vtable_0026f890;
	AI_Dtor(ai);
}

/* 0x00155e40  vtable +0x2c: before acting: go for a bomb in reach (meta_b list
   0x00308f48), or, with a goal cooldown, for the nearest goal */
void FUN_00155e40(int *ai)
{
	unsigned int bomb;
	float range;
	int goal;
	float *at;

	for (bomb = CLList_Next((unsigned int *)DAT_00308f48, NULL); bomb != 0;
		bomb = CLList_Next((unsigned int *)DAT_00308f48, (unsigned int *)bomb)) {
		if (0.0f <= MA_F32(bomb, 0x10) && MA_F32(bomb, 0x10) < 60.0f) { /* 0x00254e98 */
			range = MA_F32(bomb, 0x18);
			if (MA_DISTANCE_SQ(MA_FP(MA_PTR(bomb, 8), 0xd0), MA_FP(ai[0xc], 0xd0)) < range * range) {
				MA_VFN(ai, 0x18, void (*)(void *, int))(ai, 6);
				ai[9] = DAT_00293948;
				ai[10] = 1;
				ai[7] = MA_I32(bomb, 8) + 0xd0;
				break;
			}
		}
	}
	if (0.0f < MA_F32(ai, 0x1130)) {
		goal = MA_DUAL(FUN_00166a60, int (*)(float, undefined4), int (*)(void *, float *),
			(ma_bitsf((unsigned int)DAT_00293948), (undefined4)(ai[0xc] + 0xd0)),
			((void *)DAT_00293948, MA_FP(ai[0xc], 0xd0)));
		if (goal != 0) {
			at = MA_FP(goal, 0xe0);
			/* 0x002657ec = 9 */
			if ((double)CLVec3_DistanceSqXZ(at, MA_FP(ai[0xc], 0xd0)) <
				(double)DAT_002b0598 * (double)DAT_002b0598 * 9.0) {
				MA_VFN(ai, 0x18, void (*)(void *, int))(ai, 6);
				ai[9] = DAT_00293948;
				ai[10] = 1;
				ai[7] = (int)at;
			}
		}
	}
}

/* 0x00155f70  vtable +0x30: can the player move to `to` (no vent in the way pushes it
   back)? 0: a vent blocks the way */
undefined4 FUN_00155f70(int ai, float *to)
{
	unsigned int vent;
	unsigned int flags;
	float point[4];
	float t;

	for (vent = CLList_Next((unsigned int *)DAT_00308e60, NULL); vent != 0;
		vent = CLList_Next((unsigned int *)DAT_00308e60, (unsigned int *)vent)) {
		if (MA_I16(vent, 0x32) == 0)
			continue;
		flags = MA_U32(vent, 0x80);
		if ((flags & 1) == 0) {
			if (MA_I16(vent, 0x34) != 0) {
				if ((flags & 2) != 0)
					goto check;
				if (MA_I16(vent, 0x34) != 0)
					continue;
			}
			if ((flags & 4) == 0)
				continue;
		}
	check:
		if (CMetaVent_Push((struct CMetaVent *)vent, MA_U16(MA_CHAR(ai), 0x230), MA_FP(MA_CHAR(ai), 0xd0), 1) == 0) {
			t = 1.0f;
			do {
				bmVectorBlend(point, MA_FP(MA_CHAR(ai), 0xd0), 1.0f, to, t);
				if (CMetaVent_Push((struct CMetaVent *)vent, MA_U16(MA_CHAR(ai), 0x230), point, 1) != 0)
					return 0;
				t = t + 1.0f;
			} while ((double)t < 5.0); /* 0x0026f8d8 */
		}
	}
	return 1;
}

/* 0x00156080  vtable +0x08: the jump cheat */
void FUN_00156080(int ai)
{
	if (MA_I32(MA_PTR(ai, 0x78), 0x14) != 0)
		MA_F32(MA_CHAR(ai), 0x198) = DAT_00292c98;
}

/* 0x001560a0  vtable +0x38: AI timer `timer` ran out */
void FUN_001560a0(int ai, int timer)
{
	if (timer == 0) {
		MA_I32(MA_PTR(ai, 0x1160), 0x2bc) = -0xc;
		MA_I32(MA_PTR(ai, 0x1160), 0x2f4) = 1;
	} else if (timer == 1) {
		Actor_Delete(MA_PTR(ai, 0x10c0));
	} else if (timer == 2) {
		baSetRenderPass((struct BActorInstance *)(MA_CHAR(ai) + 0x10), 1, 0, 0);
		MA_U32(MA_CHAR(ai), 0x220) = MA_U32(MA_CHAR(ai), 0x220) & 0xfffffdff;
	}
}

/* 0x00156120  vtable +0x3c: AI timer `timer` started: 0 speed-up, 1 hammer, 2 shield */
void FUN_00156120(int ai, int timer)
{
	void *hammer;
	int anim;

	CLHud_ShowMarkerText(DAT_002ecfac, MA_U16(MA_CHAR(ai), 0x230), timer);
	if (timer == 0) {
		MA_I32(MA_PTR(ai, 0x1160), 0x2bc) = 0xc;
		MA_I32(MA_PTR(ai, 0x1160), 0x2f4) = 1;
	} else if (timer == 1) {
		if (MA_I32(ai, 0x10c0) == 0) {
			/* the character table (classlib_ui 0x002906f8): +0x1c the hammer model */
			hammer = Actor_LoadAny(*(const char **)((char *)&PTR_s_malletdub_obe_00290714 +
				DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60), (float *)DAT_002ea478, 0xc, NULL, 0);
			AI_Hold((void *)ai, hammer, 1);
			MA_I32(hammer, 0xe0) = 0;
			MA_I32(hammer, 0xe4) = 0;
			MA_I32(hammer, 0xe8) = 0;
			AI_SetState((void *)ai, 0);
			baFlushAnimations((void *)(MA_CHAR(ai) + 0x10), (struct BActorNode *)MA_PTR(ai, 0x10cc), 1, 0);
			anim = MA_I32(MA_CHAR(ai), 0x270);
			if (anim != 0) {
				baQueueAnimation((struct BActorInstance *)(MA_CHAR(ai) + 0x10),
					(struct BActorNode *)MA_PTR(ai, 0x10cc), (struct BAnim *)anim, 0x400, -1, 1, 0, 0);
				return;
			}
		}
	} else if (timer == 2) {
		baSetRenderPass((struct BActorInstance *)(MA_CHAR(ai) + 0x10), 1, 5, ai + 0x114c);
		CLList_Remove((int *)DAT_002ea488, (int *)MA_CHAR(ai));
		CLList_Insert((int *)DAT_002ea488, (int *)MA_CHAR(ai), 1);
		MA_U32(MA_CHAR(ai), 0x220) = MA_U32(MA_CHAR(ai), 0x220) | 0x200;
		MA_I32(MA_CHAR(ai), 0x224) = 0x100;
	}
}

/* 0x001562a0  AI: retaliate against the last attacker? (mode 4) */
undefined4 FUN_001562a0(int ai)
{
	int attacker;

	if (MA_I16(ai, 0x1088) != -1) {
		attacker = DAT_003075bc[MA_I16(ai, 0x1088)];
		/* (5 - the attacker's orbs) * 100: 0 when it carries 5 (as in the original) */
		if (ma_rand() % ((5 - MA_I32(attacker, 0x1140)) * 100) <
			*(int *)((char *)&DAT_00290740 + DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60)) {
			MA_I32(ai, 4) = 4;
			MA_I16(ai, 0x98) = 0;
			return 1;
		}
	}
	return 0;
}

/* 0x00156330  AI: a random other player (alive only: not HURT or KNOCKOUT); -1 none */
uint FUN_00156330(int ai, int alive_only)
{
	uint player;
	short state;

	if (alive_only != 0) {
		player = 0;
		if (0 < (int)DAT_002bae94) {
			do {
				if (player != MA_U16(MA_CHAR(ai), 0x230)) {
					state = MA_I16(DAT_003075bc[player], 0x36);
					if (state != 10 && state != 0xd)
						break;
				}
				player++;
			} while ((int)player < (int)DAT_002bae94);
		}
		if (player == (uint)DAT_002bae94)
			return 0xffffffff;
	}
	do {
		do {
			player = (uint)(ma_rand() % (int)DAT_002bae94);
		} while (player == MA_U16(MA_CHAR(ai), 0x230));
	} while (alive_only != 0 &&
		(MA_I16(DAT_003075bc[player], 0x36) == 10 || MA_I16(DAT_003075bc[player], 0x36) == 0xd));
	return player;
}

/* 0x001563e0  AI: the player that hit this one most (else a random one) */
uint FUN_001563e0(int ai, int alive_only)
{
	uint best;
	uint player;
	short most;
	short state;

	most = 0;
	best = 0xffffffff;
	if (0 < DAT_002bae94) {
		for (player = 0; (int)player < DAT_002bae94; player++) {
			if (player == MA_U16(MA_CHAR(ai), 0x230))
				continue;
			if (alive_only != 0) {
				state = MA_I16(DAT_003075bc[player], 0x36);
				if (state == 10 || state == 0xd)
					continue;
			}
			if (most < MA_I16(ai, 0x108a + player * 2)) {
				best = player;
				most = MA_I16(ai, 0x108a + player * 2);
			}
		}
		if (best != 0xffffffff)
			return best;
	}
	return FUN_00156330(ai, alive_only);
}

/* 0x00156460  AI: press attack when the state allows it, by aggression */
void FUN_00156460(int ai, int force)
{
	int saved;
	int limit;
	int roll;
	signed char *priority;

	saved = MA_I32(MA_PTR(ai, 0x6c), 0x14);
	if (MA_F32(ai, 0x1144) != 0.0f)
		return;
	priority = (signed char *)(MA_U16(ai, 0x36) * MA_U16(ai, 0x34) + MA_I32(ai, 0x109c));
	if (priority[5] == 0 && priority[6] == 0)
		return;
	MA_I32(MA_PTR(ai, 0x6c), 0x14) = 1;
	if (MA_F32(ai, 0x10a4) == 0.0f && MA_I32(ai, 0x18) < 0)
		limit = (5 - DAT_002bae70) * 100;
	else
		limit = 100;
	roll = force == 0 ? ma_rand() % limit : 0;
	if (roll <= *(int *)((char *)&DAT_0029073c + DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60) &&
		((int (*)(void *))(*(void ***)MA_PTR(ai, 0x1098))[5])((void *)ai) == 1) {
		/* 0x0025552c = 8 */
		MA_F32(ai, 0x1144) = (DAT_002b0850 + 8.0f) - (float)(DAT_002bae70 * 8);
		return;
	}
	MA_I32(MA_PTR(ai, 0x6c), 0x14) = saved;
}

/* 0x00156580  the speed-up overlay of the player (a COverlay child of the HUD) */
void FUN_00156580(int ai)
{
	unsigned int desc[0xec];

	memset(desc, 0, sizeof desc);
	desc[0x288 / 4] = 0;
	MA_F32(desc, 0x250) = 127.0f;
	MA_F32(desc, 0x254) = 127.0f;
	MA_F32(desc, 0x258) = 127.0f;
	desc[0x25c / 4] = 0;
	desc[0x268 / 4] = 0;
	desc[0x264 / 4] = 0x7f;
	desc[0x28c / 4] = desc[0x28c / 4] | 1;
	MA_F32(desc, 0x2b4) = 15.0f;
	MA_F32(desc, 0x2b8) = 20.0f;
	desc[0x2bc / 4] = 0;
	MA_F32(desc, 0x310) = 32.0f;
	MA_F32(desc, 0x308) = 32.0f;
	MA_F32(desc, 0x30c) = 32.0f;
	MA_F32(desc, 0x304) = 1.0f;
	desc[1] = (unsigned int)bLoadResource(NULL, "speedup.bmp", 0, 0xdefa);
	desc[0x28c / 4] = 1;
	MA_I32(ai, 0x1160) = COverlay_CreateChild(MA_I32(DAT_002ecfac, 0xf0 + MA_U16(MA_CHAR(ai), 0x230) * 4),
		(undefined4 *)desc);
}

/* the HUD orb markers of a player (classlib_ui 0x002ecfac: 4 per player from +0x110) */
#define MA_HUD_MARKER(player, n) MA_PTR(DAT_002ecfac, (player) * 0x10 + 0x110 + (n) * 4)

/* 0x00156690  vtable +0x04: after the AI update: timers, HUD markers, glow */
void FUN_00156690(int *ai)
{
	int player;
	int frames;
	float t;
	float glow;

	/* the goal cooldown (+0x1130): counts to 0; a negative one restarts at 120 */
	if (0.0f < MA_F32(ai, 0x1130)) {
		t = MA_F32(ai, 0x1130) - DAT_0028fda8;
		MA_F32(ai, 0x1130) = t;
		if (t <= 0.0f)
			MA_I32(ai, 0x1130) = 0;
	} else if (MA_F32(ai, 0x1130) < 0.0f) {
		t = DAT_0028fda8 + MA_F32(ai, 0x1130);
		MA_F32(ai, 0x1130) = t;
		if (0.0f <= t) {
			MA_F32(ai, 0x1130) = 120.0f;
			if (ai[1] == 0xb)
				MA_VFN(ai, 0x18, void (*)(void *, int))(ai, -1);
		}
	}
	if (MA_I16(ai, 0x36) != 0xe)
		MA_U32(ai[0xc], 0x220) = MA_U32(ai[0xc], 0x220) | 1;
	if (MA_F32(ai, 0x1144) != 0.0f) {
		t = MA_F32(ai, 0x1144) - DAT_0028fda8;
		MA_F32(ai, 0x1144) = t;
		if (t <= 0.0f)
			MA_I32(ai, 0x1144) = 0;
	}
	/* the speed-up overlay: hidden for a computer player, blinks at the end */
	player = MA_U16(ai[0xc], 0x230);
	if (DAT_002baed8[player * 10] == 0) {
		MA_U16(MA_PTR(ai, 0x1160), 0x54) = 1;
	} else if (120.0f <= MA_F32(ai, 0x10a0)) { /* 0x00255b50 */
		MA_U16(MA_PTR(ai, 0x1160), 0x54) = 0;
	} else {
		frames = (int)MA_F32(ai, 0x10a0);
		MA_U16(MA_PTR(ai, 0x1160), 0x54) = (unsigned short)(frames % 0x14 >= 10);
	}
	/* the orb markers: the one for the number of orbs carried is visible */
	player = MA_U16(ai[0xc], 0x230);
	MA_U16(MA_HUD_MARKER(player, 0), 0x54) = MA_I32(ai, 0x1140) == 1 ? 0 : 1;
	player = MA_U16(ai[0xc], 0x230);
	MA_U16(MA_HUD_MARKER(player, 1), 0x54) = MA_I32(ai, 0x1140) == 2 ? 0 : 1;
	player = MA_U16(ai[0xc], 0x230);
	MA_U16(MA_HUD_MARKER(player, 2), 0x54) = MA_I32(ai, 0x1140) == 3 ? 0 : 1;
	player = MA_U16(ai[0xc], 0x230);
	MA_U16(MA_HUD_MARKER(player, 3), 0x54) = MA_I32(ai, 0x1140) == 4 ? 0 : 1;
	player = MA_U16(ai[0xc], 0x230);
	if (MA_I32(ai, 0x1140) == 4)
		/* 0x002630c8 = 63, 0x00253f38 = 64 */
		MA_F32(MA_HUD_MARKER(player, 3), 0x2b8) =
			(float)(sin((double)MA_F32(DAT_0030564c, 100) / (double)DAT_002b08b8) * 63.0 + 64.0);
	else
		MA_F32(MA_HUD_MARKER(player, 3), 0x2b8) = 127.0f;
	/* carrying an orb slows the player */
	if (MA_I32(ai, 0x10bc) != 0 && MA_I32(MA_PTR(ai, 0x10bc), 0x21c) == 10) {
		MA_F32(ai[0xc], 0x194) = DAT_002b0874 * MA_F32(ai[0xc], 0x194);
		MA_F32(ai[0xc], 0x19c) = DAT_002b0874 * MA_F32(ai[0xc], 0x19c);
	}
	/* the shield glow */
	if (MA_F32(ai, 0x10a8) != 0.0f) {
		baSetRenderPass((struct BActorInstance *)(ai[0xc] + 0x10), 1, 5, (int)MA_FP(ai, 0x114c));
		CLList_Remove((int *)DAT_002ea488, (int *)ai[0xc]);
		CLList_Insert((int *)DAT_002ea488, (int *)ai[0xc], 1);
		MA_U32(ai[0xc], 0x220) = MA_U32(ai[0xc], 0x220) | 0x200;
		MA_I32(ai[0xc], 0x224) = 0x100;
		glow = (float)(sin((double)DAT_002ba99c / (double)DAT_002b0880) * (double)DAT_002b087c +
			(double)DAT_002b0878);
		MA_F32(ai, 0x114c) = glow;
		if (MA_F32(ai, 0x10a8) < 120.0f) {
			frames = (int)MA_F32(ai, 0x10a8);
			MA_F32(ai, 0x114c) = (float)((double)(frames % 0x14 >= 10) * (double)glow);
		}
		MA_I32(ai, 0x1150) = (int)(sin((double)MA_F32(DAT_0030564c, 100) / (double)DAT_002b088c) *
			(double)DAT_002b0888 + (double)DAT_002b0884);
		MA_I32(ai, 0x1154) = (int)(sin((double)MA_F32(DAT_0030564c, 100) / (double)DAT_002b0898) *
			(double)DAT_002b0894 + (double)DAT_002b0890);
		MA_I32(ai, 0x1158) = (int)(sin((double)MA_F32(DAT_0030564c, 100) / (double)DAT_002b08a4) *
			(double)DAT_002b08a0 + (double)DAT_002b089c);
		MA_I32(ai, 0x115c) = (int)(sin((double)MA_F32(DAT_0030564c, 100) / (double)DAT_002b08b0) *
			(double)DAT_002b08ac + (double)DAT_002b08a8);
	}
	/* the carried hammer grows in during the super attack, shrinks after it */
	if (MA_I32(ai, 0x10c0) != 0) {
		if (1.0f / DAT_002b08b4 <= MA_F32(ai, 0x10a4)) {
			MA_F32(MA_PTR(ai, 0x10c0), 0xe0) = DAT_002b08b4 * DAT_0028fda8 + MA_F32(MA_PTR(ai, 0x10c0), 0xe0);
			if (1.0f < MA_F32(MA_PTR(ai, 0x10c0), 0xe0))
				MA_F32(MA_PTR(ai, 0x10c0), 0xe0) = 1.0f;
		} else {
			MA_F32(MA_PTR(ai, 0x10c0), 0xe0) = MA_F32(MA_PTR(ai, 0x10c0), 0xe0) - DAT_002b08b4 * DAT_0028fda8;
			if (MA_F32(MA_PTR(ai, 0x10c0), 0xe0) < 0.0f)
				MA_I32(MA_PTR(ai, 0x10c0), 0xe0) = 0;
		}
		MA_I32(MA_PTR(ai, 0x10c0), 0xe8) = MA_I32(MA_PTR(ai, 0x10c0), 0xe0);
		MA_I32(MA_PTR(ai, 0x10c0), 0xe4) = MA_I32(MA_PTR(ai, 0x10c0), 0xe8);
	}
}

/* 0x00156ba0  drop the carried orbs: `score` scores them, `splash` (without score)
   throws the carried orb into the water */
void FUN_00156ba0(int ai, int score, int splash)
{
	ma_free_beam(ai);
	if (score != 0)
		FUN_00159250(ai, (int)FUN_001519a0(MA_I32(ai, 0x1140)));
	if (MA_I32(ai, 0x10bc) != 0 && MA_I32(MA_PTR(ai, 0x10bc), 0x21c) == 10) {
		if (score == 0 && splash != 0)
			FUN_0015c910((undefined4)(MA_CHAR(ai) + 0xd0));
		Actor_Delete(MA_PTR(ai, 0x10bc));
		MA_I32(ai, 0x10bc) = 0;
	}
	MA_I32(ai, 0x1140) = 0;
}

/* 0x00156c40  the player is hit: damage adds up; over the character's KO limit it is
   knocked out (and loses its orbs when `drop`), else hurt. With mode 0, `target` is the
   hit's origin and the player turns to it; with another mode the original compares the
   pointer's bits as a float with pi/2 (front or back animation) */
void FUN_00156c40(int ai, float *target, float damage, int mode, int drop)
{
	float behind;
	float heading;
	float total;
	int player;

	MA_F32(ai, 0x10d8) = DAT_002b07b8;
	memcpy(&behind, &target, sizeof behind);
	heading = 0.0f;
	if (mode == 0) {
		heading = CLMath_HeadingTo(MA_FP(MA_CHAR(ai), 0xd0), target);
		behind = CLMath_AngleDistance(heading, MA_F32(MA_CHAR(ai), 0x1e8));
	}
	total = damage + MA_F32(ai, 0x111c);
	MA_F32(ai, 0x111c) = total;
	player = MA_U16(MA_CHAR(ai), 0x230);
	if (total <= *(float *)((char *)&DAT_00290738 + DAT_002baed4[player * 10] * 0x60)) {
		/* hurt */
		AI_SetState((void *)ai, 10);
		if (behind <= 1.5707963f && mode != 1) /* 0x00253898 */
			FUN_000af690(MA_CHAR(ai), 0x11, 0, 0, DAT_002b05a4, 0.1f, 1);
		else
			FUN_000af690(MA_CHAR(ai), 0x12, 0, 0, DAT_002b05a4, 0.1f, 1);
		return;
	}
	/* knocked out */
	CLRumble_PlayMotor((int)DAT_002ed324, player, 0, 0xfff0, 200, 0, 100, 0, 1);
	CLRumble_PlayMotor((int)DAT_002ed324, MA_U16(MA_CHAR(ai), 0x230), 1, 0xfff0, 200, 0, 100, 100, 1);
	AI_SetState((void *)ai, 0xd);
	if (drop != 0) {
		ma_free_beam(ai);
		if (MA_I32(ai, 0x10bc) != 0 && MA_I32(MA_PTR(ai, 0x10bc), 0x21c) == 10) {
			FUN_0015c910((undefined4)(MA_CHAR(ai) + 0xd0));
			Actor_Delete(MA_PTR(ai, 0x10bc));
			MA_I32(ai, 0x10bc) = 0;
		}
		MA_I32(ai, 0x1140) = 0;
	}
	MA_I32(ai, 0x111c) = 0;
	if (1.5707963f < behind || mode == 1) {
		MA_I32(ai, 0x1120) = 0;
		FUN_000af690(MA_CHAR(ai), 0x14, 0, 0, DAT_002b05a8, 0.1f, 1);
		if (mode == 0)
			MA_F32(MA_CHAR(ai), 0x1d8) = CLMath_WrapAngle(heading + 3.1415925f); /* 0x00256750 */
	} else {
		MA_I32(ai, 0x1120) = 1;
		FUN_000af690(MA_CHAR(ai), 0x13, 0, 0, DAT_002b05a8, 0.1f, 1);
		if (mode == 0)
			MA_F32(MA_CHAR(ai), 0x1d8) = heading;
	}
}

/* 0x00156e90  vtable +0x24: respawn at the player's start point */
void FUN_00156e90(int ai)
{
	AI_RespawnAt((void *)ai, MA_U16(MA_CHAR(ai), 0x230) + 1);
	MA_F32(MA_CHAR(ai), 0x1d8) = CLMath_WrapAngle(DAT_00309ef8 + 3.1415925f); /* 0x00256750 */
	MA_I32(MA_CHAR(ai), 0x1e8) = MA_I32(MA_CHAR(ai), 0x1d8);
	ma_free_beam(ai);
	if (MA_I32(ai, 0x10bc) != 0 && MA_I32(MA_PTR(ai, 0x10bc), 0x21c) == 10) {
		Actor_Delete(MA_PTR(ai, 0x10bc));
		MA_I32(ai, 0x10bc) = 0;
	}
	MA_I32(ai, 0x1140) = 0;
}

/* the character's model names (classlib_ui character table 0x002906f8, 0x60 bytes per
   character): +0x10 left palm, +0x14 right palm, +0x18 right shoulder */
#define MA_CHAR_NAME(character, offset) \
	(*(const char **)((char *)&PTR_s_lpalm_00290708 + (character) * 0x60 + (offset) - 0x10))

static void ma_find_hold_nodes(int ai, int character)
{
	int i;

	for (i = 0; i < 2; i++)
		MA_PTR(ai, 0x10c4 + i * 4) = baFindNode((struct BActorNode *)MA_PTR(MA_CHAR(ai), 0x144),
			MA_CHAR_NAME(character, 0x10 + i * 4));
	MA_PTR(ai, 0x10cc) = baFindNode((struct BActorNode *)MA_PTR(MA_CHAR(ai), 0x144),
		MA_CHAR_NAME(character, 0x18));
}

/* 0x00156f30  vtable +0x40: switch between the high- and low-detail model, play `anim` */
void FUN_00156f30(int ai, int high_detail, undefined4 anim)
{
	int ch;
	int model;
	int character;
	float position[4];

	ch = MA_CHAR(ai);
	character = DAT_002baed4[MA_U16(ch, 0x230) * 10];
	/* the character models (classlib_ui 0x00290148, 0x108 bytes each): low, high */
	if (high_detail == 0)
		model = *(int *)((char *)DAT_00290148 + character * 0x108);
	else
		model = *(int *)((char *)DAT_00290148 + 0x84 + character * 0x108);
	DAT_002b0590 = high_detail == 0;
	if (model == 0)
		return;
	position[0] = MA_F32(ch, 0xd0);
	position[1] = MA_F32(ch, 0xd4);
	position[2] = MA_F32(ch, 0xd8);
	position[3] = MA_F32(ch, 0xdc);
	baDeleteActorInstance((struct BActorInstance *)(ch + 0x10));
	Actor_Create((void *)model, position, (void *)MA_CHAR(ai), 1, 0, 0);
	if (high_detail != 0) {
		character = DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10];
		if (*(int *)((char *)DAT_00290148 + character * 0x108) != 0)
			baSetInstanceAnimations((struct BActor *)*(int *)((char *)DAT_00290148 + character * 0x108),
				(struct BActorInstance *)(MA_CHAR(ai) + 0x10));
	}
	ma_find_hold_nodes(ai, DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10]);
	FUN_000af690(MA_CHAR(ai), (int)anim, 0, 0, 1.0f, 0.0f, 0);
	baUpdateAnimations((struct BActorInstance *)(MA_CHAR(ai) + 0x10),
		(struct BActorNode *)MA_PTR(MA_CHAR(ai), 0x144), 0.0f, NULL);
	baPrepActorInstance((struct BActorInstance *)(MA_CHAR(ai) + 0x10), 1);
	Core_SetCharacterColours((void *)(MA_CHAR(ai) + 0x10), DAT_00283ea4);
}

/* 0x00157100  constructor (player, start position) */
undefined4 *FUN_00157100(undefined4 *ai, int player, undefined4 position)
{
	int ch;
	int table;
	int i;
	const char *name;
	int character;

	AI_Ctor(ai);
	*(const void **)ai = vtable_0026f890;
	DAT_002f2898 = 1.5f;
	ai[0x451] = 0;
	character = DAT_002baed4[player * 10];
	ch = (int)Actor_Create((void *)*(int *)((char *)DAT_00290148 + character * 0x108),
		(float *)position, NULL, 1, 0, 1);
	ai[0xc] = ch;
	if (ch != 0) {
		MA_I32(ch, 0x220) = 0x59;
		MA_I16(ai[0xc], 0x230) = (short)player;
	}
	ai[0x44c] = 0;
	ai[0x44d] = 0;
	ai[0x452] = 0;
	MA_I16(ai, 0x34) = 0xf;
	MA_I16(ai, 0x36) = 0;
	ai[1] = 0;
	ai[2] = 0;
	ai[0x425] = (undefined4)PTR_LAB_002b0710;
	ai[0x426] = (undefined4)PTR_LAB_002b06d4;
	ai[0x427] = (undefined4)DAT_002b05f0;
	if (ai[0xc] != 0) {
		Actor_InitCharData((void *)ai[0xc]);
		MA_PTR(ai[0xc], 0x324) = DAT_00292ca0;
		/* the animations (charlib table, 0x44 bytes per entry: name, then a speed) */
		for (i = 0; 0x264 + i * 4 < 800; i++) {
			table = MA_I32(ai[0xc], 0x324);
			MA_PTR(ai[0xc], 0x264 + i * 4) = baFindSubObject((struct BActorInstance *)(ai[0xc] + 0x10),
				(const char *)(table + i * 0x44));
			ch = ai[0xc];
			if (MA_I32(ch, 0x264 + i * 4) == 0) {
				name = (const char *)(MA_I32(ch, 0x324) + i * 0x44);
				if (MA_F32(MA_PTR(ch, 0x324), 0x40 + i * 0x44) != 0.0f && name[0] != '\0')
					MA_PRINTF("WARNING! %s missing anim %s\n",
						*(const char **)((char *)&PTR_DAT_002906f8 +
							DAT_002baed4[MA_U16(ch, 0x230) * 10] * 0x60), name);
			}
		}
		ma_find_hold_nodes((int)ai, character);
		MA_F32(ai, 0x114c) = 5.0f;
		ai[0x454] = 0xff;
		ai[0x455] = 200;
		ai[0x456] = 0;
		ai[0x457] = 200;
	}
	i = DAT_002bae70 * 5;
	table = *(int *)((char *)&DAT_00290750 + character * 0x60);
	ai[0x44f] = 0;
	ai[0x450] = 0;
	MA_F32(ai, 0x10d4) = 200.0f;
	ai[0x443] = 0;
	ai[0x42d] = table / (i + 5);
	if (DAT_0030a098 != 0)
		FUN_00156f30((int)ai, 1, 0x22);
	Core_SetCharacterColours((void *)(ai[0xc] + 0x10), DAT_00283ea4);
	return ai;
}

/* 0x001573d0  AI: the player carrying the most orbs (ties and chances by rand; else a
   random one) */
uint FUN_001573d0(int ai, int alive_only)
{
	int most;
	uint best;
	uint player;
	int other;
	short state;

	most = 0;
	best = 0xffffffff;
	if (0 < DAT_002bae94) {
		for (player = 0; (int)player < DAT_002bae94; player++) {
			if (player == MA_U16(MA_CHAR(ai), 0x230))
				continue;
			if (alive_only != 0) {
				state = MA_I16(DAT_003075bc[player], 0x36);
				if (state == 10 || state == 0xd)
					continue;
			}
			other = DAT_003075bc[player];
			if (most < MA_I32(other, 0x1140))
				goto take;
			if (MA_I32(other, 0x1140) == most && ma_rand() % 100 < 0x32)
				goto take;
			if (MA_I32(other, 0x1140) != 0 && DAT_002baed8[player * 10] != 0 &&
				ma_rand() % 100 < (DAT_002bae70 * 5 + 5) * 4)
				goto take;
			continue;
		take:
			most = MA_I32(other, 0x1140);
			best = player;
		}
		if (best != 0xffffffff)
			return best;
	}
	return FUN_00156330(ai, alive_only);
}

/* 0x00157510  `ai` hits `victim`: knock back, damage, steal orbs */
void FUN_00157510(int ai, int victim)
{
	int ch;
	int character;
	int saved;
	float push[3];
	float k;

	ch = MA_CHAR(ai);
	push[0] = MA_F32(MA_CHAR(victim), 0xd0) - MA_F32(ch, 0xd0);
	push[1] = MA_F32(MA_CHAR(victim), 0xd4) - MA_F32(ch, 0xd4);
	push[2] = MA_F32(MA_CHAR(victim), 0xd8) - MA_F32(ch, 0xd8);
	character = DAT_002baed4[MA_U16(ch, 0x230) * 10];
	if (MA_F32(ai, 0x10a4) == 0.0f || MA_I16(ai, 0x36) == 6)
		k = *(float *)((char *)&DAT_0029072c + character * 0x60);
	else
		k = DAT_00293a2c * *(float *)((char *)&DAT_0029072c + character * 0x60);
	bmVectorSetLength(push, push, k);
	push[1] = 0.0f;
	ch = MA_CHAR(victim);
	MA_F32(ch, 0x194) = push[0] + MA_F32(ch, 0x194);
	MA_F32(ch, 0x198) = push[1] + MA_F32(ch, 0x198);
	MA_F32(ch, 0x19c) = push[2] + MA_F32(ch, 0x19c);
	MA_U16(ai, 0x1086) = MA_U16(MA_CHAR(victim), 0x230);
	if (ma_rand() % 100 < 2)
		MA_VOICE(victim, 0xc, DAT_00293928, 180.0f);
	MA_U16(victim, 0x1088) = MA_U16(MA_CHAR(ai), 0x230);
	MA_I16(victim, 0x108a + MA_U16(MA_CHAR(ai), 0x230) * 2) =
		MA_I16(victim, 0x108a + MA_U16(MA_CHAR(ai), 0x230) * 2) + 1;
	saved = MA_I32(victim, 0x1140);
	character = DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10];
	if (MA_F32(ai, 0x10a4) == 0.0f || MA_I16(ai, 0x36) == 6) {
		FUN_00156c40(victim, MA_FP(MA_CHAR(ai), 0xd0), *(float *)((char *)&DAT_00290734 + character * 0x60), 0, 0);
	} else {
		FUN_00156c40(victim, MA_FP(MA_CHAR(ai), 0xd0),
			DAT_00293a30 * *(float *)((char *)&DAT_00290734 + character * 0x60), 0, 0);
		MA_F32(victim, 0x10d8) = DAT_00293a34 * DAT_002b07b8;
	}
	if (MA_I16(victim, 0x36) == 0xd || (DAT_00305658 != 0 && MA_I16(victim, 0x36) == 10)) {
		MA_I32(victim, 0x1140) = saved;
		FUN_00152a50((int *)ai, victim);
		ma_free_beam(victim);
		if (MA_I32(victim, 0x10bc) != 0 && MA_I32(MA_PTR(victim, 0x10bc), 0x21c) == 10) {
			Actor_Delete(MA_PTR(victim, 0x10bc));
			MA_I32(victim, 0x10bc) = 0;
		}
		MA_I32(victim, 0x1140) = 0;
	}
}

/* 0x001577c0  vtable +0x10: the attack lands: on a player standing on this one's head
   (jump attack), on a player in front, or on an object in front */
undefined4 FUN_001577c0(int ai)
{
	int ground;
	int other;
	uint player;
	int i;
	int ch;
	int hit;
	float heading;
	float turn;
	float distance;
	double reach;
	double t;
	float ray_from[4];
	float ray[4];
	float result[4];
	float k;

	hit = 0;
	MA_I16(ai, 0x1092) = 1;
	if (MA_I16(ai, 0x36) == 6 && MA_U8(MA_PTR(MA_CHAR(ai), 0x18c), 0x198) != 0) {
		for (i = 0; i < (int)MA_U8(MA_PTR(MA_CHAR(ai), 0x18c), 0x198); i++) {
			ground = MA_I32(MA_CHAR(ai), 0x18c);
			other = MA_I32(ground, 0x17c + i * 4);
			if (MA_I32(other, 0x21c) == 1) {
				player = MA_U16(other, 0x230);
				if (MA_I16(DAT_003075bc[player], 0x36) != 0xd) {
					CLSamples_PlayGeneric((ma_rand() & 1) + 0xd, 0x7f, 0, 0, NULL);
					if (MA_I16(DAT_003075bc[player], 0x36) != 10 &&
						MA_F32(DAT_003075bc[player], 0x10a8) == 0.0f) {
						FUN_00157510(ai, DAT_003075bc[player]);
						return 1;
					}
				}
			}
		}
	}
	for (player = 0; (int)player < DAT_002bae94; player++) {
		ch = MA_CHAR(ai);
		if (player == MA_U16(ch, 0x230))
			continue;
		if (!(fabsf(MA_F32(ch, 0xd4) - MA_F32(MA_CHAR(DAT_003075bc[player]), 0xd4)) < MA_F32(ai, 0x1100)))
			continue;
		heading = CLMath_HeadingTo(MA_FP(ch, 0xd0), MA_FP(MA_CHAR(DAT_003075bc[player]), 0xd0));
		turn = CLMath_AngleDistance(heading, MA_F32(MA_CHAR(ai), 0x1e8));
		if (!(turn < MA_F32(ai, 0x10fc)))
			continue;
		distance = bmVectorDistance(MA_FP(MA_CHAR(ai), 0xd0), MA_FP(MA_CHAR(DAT_003075bc[player]), 0xd0));
		reach = (double)*(float *)((char *)&DAT_00290728 + DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60);
		if (MA_F32(ai, 0x10a4) != 0.0f)
			reach = reach * (double)DAT_00293a38;
		/* 0x00253850 = 1.0, 0x002536d0 = 0.5 */
		if (((double)turn + 1.0) * (double)distance < reach * 0.5 &&
			MA_I16(DAT_003075bc[player], 0x36) != 0xd) {
			CLSamples_PlayGeneric((ma_rand() & 3) + 0xb, 0x7f, 0, 0, NULL);
			if (MA_F32(DAT_003075bc[player], 0x10a8) == 0.0f) {
				FUN_00157510(ai, DAT_003075bc[player]);
				return 1;
			}
		}
	}
	/* an object in front, at half the reach */
	ch = MA_CHAR(ai);
	ray_from[0] = MA_F32(ch, 0xd0);
	ray_from[2] = MA_F32(ch, 0xd8);
	ray_from[1] = MA_F32(ch, 0xd4) + 50.0f; /* 0x002559b8 */
	ray_from[3] = MA_F32(ch, 0xdc);
	bmVectorRotateY(ray, DAT_0028fdd8, MA_F32(ch, 0x1e8));
	k = *(float *)((char *)&DAT_00290728 + DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60) * 0.5f;
	ray[0] = ray[0] * k;
	ray[1] = ray[1] * k;
	ray[2] = ray[2] * k;
	t = (double)CRColl_RayCast(ray_from, ray, result, NULL, (void **)&hit);
	if (hit != 0 && MA_I32(hit, 0x440) != 0 && 0.0 < t) {
		CLSamples_PlayGeneric((ma_rand() & 3) + 0xb, 0x7f, 0, 0, NULL);
		FUN_00153f60(hit);
		FUN_000b9880(hit, 0);
		FUN_00165f10(hit);
		return 1;
	}
	return 0;
}

/* 0x00157bd0  vtable +0x14: at the end of the AI think: maybe attack */
void FUN_00157bd0(int ai)
{
	FUN_00156460(ai, 0);
}

/* 0x00157be0  the platforms the player touches: squashed between two (respawn),
   conveyors, moving platforms carry it, hurting platforms */
void FUN_00157be0(int *ai, undefined4 player)
{
	int ground;
	int platform;
	int contact;
	int i;
	int n;
	int up;
	int down;
	float k;
	float carry[3];
	unsigned int flags;
	float damage;

	ground = MA_I32(ai[0xc], 0x18c);
	if (MA_I32(ground, 0x1d0) <= 0)
		return;
	for (i = 0; i < MA_I32(ground, 0x1d0); i++) {
		platform = MA_I32(ground, 0x1bc + i * 4);
		up = 0;
		down = 0;
		/* a platform moving down, with contacts above and below: squashed */
		if (MA_F32(platform, 0x1a8) < 0.0f && (MA_U32(platform, 0x450) & 0x8000) == 0 &&
			MA_U16(ground, 0x64) != 0) {
			n = MA_U16(MA_PTR(ai[0xc], 0x18c), 0x64);
			for (contact = 0; contact < n; contact++) {
				if ((double)MA_F32(MA_PTR(ground, contact * 4), 0x34) > 0.2) /* 0x0026f918 */
					up = 1;
				if ((double)MA_F32(MA_PTR(ground, contact * 4), 0x34) < -0.2) /* 0x0026f910 */
					down = 1;
			}
			if (up && down) {
				CLRumble_PlayMotor((int)DAT_002ed324, (int)player, 0, 0xfff0, 200, 0, 100, 0, 1);
				CLRumble_PlayMotor((int)DAT_002ed324, (int)player, 1, 0xfff0, 200, 0, 100, 100, 1);
				MA_VFN(ai, 0x24, void (*)(void *))(ai);
				return;
			}
		}
		/* a conveyor */
		if ((MA_U32(platform, 0x450) & 0x200) != 0)
			Platform_AddVelocity(MA_FP(ai[0xc], 0x194), MA_FP(platform, 0x42c));
		platform = MA_I32(MA_PTR(ai[0xc], 0x18c), 0x1bc + i * 4);
		if ((MA_U32(platform, 0x450) & 0x1000) != 0 &&
			0.0f <= MA_F32(platform, 0x42c) * MA_F32(platform, 0x1a4) +
				MA_F32(platform, 0x430) * MA_F32(platform, 0x1a8) +
				MA_F32(platform, 0x434) * MA_F32(platform, 0x1ac)) {
			/* carried by the platform's movement this frame */
			k = 1.0f / DAT_0028fda8;
			carry[0] = (MA_F32(platform, 0xe0) - MA_F32(platform, 0x1b4)) * k * MA_F32(platform, 0x438);
			carry[1] = k * (MA_F32(platform, 0xe4) - MA_F32(platform, 0x1b8)) * MA_F32(platform, 0x438);
			carry[2] = k * (MA_F32(platform, 0xe8) - MA_F32(platform, 0x1bc)) * MA_F32(platform, 0x438);
			if (carry[0] != 0.0f || carry[1] != 0.0f || carry[2] != 0.0f) {
				Platform_AddVelocity(MA_FP(ai[0xc], 0x194), carry);
				MA_F32(ai[0xc], 0xd0) = DAT_0028fda8 * MA_F32(ai[0xc], 0x194) + MA_F32(ai[0xc], 0x1a4);
				MA_F32(ai[0xc], 0xd8) = DAT_0028fda8 * MA_F32(ai[0xc], 0x19c) + MA_F32(ai[0xc], 0x1ac);
				if ((MA_U32(MA_PTR(MA_PTR(ai[0xc], 0x18c), 0x1bc + i * 4), 0x450) & 0x100) != 0 &&
					MA_F32(ai, 0x10d0) == 0.0f) {
					ma_free_beam((int)ai);
					if (ai[0x42f] != 0 && MA_I32(ai[0x42f], 0x21c) == 10) {
						FUN_0015c910((undefined4)(ai[0xc] + 0xd0));
						Actor_Delete((void *)ai[0x42f]);
						ai[0x42f] = 0;
					}
					ai[0x450] = 0;
				}
			}
		}
		ground = MA_I32(ai[0xc], 0x18c);
		platform = MA_I32(ground, 0x1bc + i * 4);
		flags = MA_U32(platform, 0x450);
		if ((flags & 0x100) != 0 && MA_I16(ai, 0x36) != 10 && MA_I16(ai, 0x36) != 0xd &&
			MA_F32(ai, 0x10d0) == 0.0f && (flags & 0x1000) == 0) {
			/* a hurting platform: hit and thrown back */
			MA_F32(ai, 0x10d0) = 80.0f;
			if ((MA_U32(MA_PTR(MA_PTR(ai[0xc], 0x18c), 0x1bc + i * 4), 0x450) & 0x8000) != 0)
				damage = 10000.0f;
			else
				damage = DAT_002b0868;
			FUN_00156c40((int)ai, MA_FP(platform, 0xe0), damage, 0, 1);
			MA_F32(ai[0xc], 0x194) = MA_F32(ai[0xc], 0x194) * -1.2f;  /* 0x0026f90c */
			MA_F32(ai[0xc], 0x198) = MA_F32(ai[0xc], 0x198) * -0.95f; /* 0x0026a7ec */
			MA_F32(ai[0xc], 0x19c) = MA_F32(ai[0xc], 0x19c) * -1.2f;
			MA_F32(ai[0xc], 0xd0) = DAT_0028fda8 * MA_F32(ai[0xc], 0x194) + MA_F32(ai[0xc], 0xd0);
			MA_F32(ai[0xc], 0xd4) = DAT_0028fda8 * MA_F32(ai[0xc], 0x198) + MA_F32(ai[0xc], 0xd4);
			MA_F32(ai[0xc], 0xd8) = DAT_0028fda8 * MA_F32(ai[0xc], 0x19c) + MA_F32(ai[0xc], 0xd8);
			return;
		}
	}
}
