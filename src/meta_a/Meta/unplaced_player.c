/*
UNPLACED_PLAYER.C  <-  gap functions 0x00151040-0x00153020 of C:\BlitzParty\Game\Src\Meta\
(no source path in the XBE, except CModule_Meta.cpp for 0x00151070 and 0x00151090)

- CModule_Meta (vtable 0x0026f678): the CGame object that starts the board game.
- The CMetaPlayer state machine (the charlib AI player, see meta_a.h): the condition
  table 0x002b06d4 and the state table 0x002b0710 (15 entries each, by state:
  STAND IDLE WALK JUMP LAND ATTACK JUMP_ATTACK PRESS FLAG FALL HURT TAUNT SCORE
  KNOCKOUT DROWN), which the CMetaPlayer constructor 0x00157100 installs.
- Level helpers: player input maps, the debug object, puzzle timers.
*/

#include "meta_a.h"
#include "core_data.h"

#define MA_MODESTACK ((struct CGameModeStack *)CORE_ADDR(0x002ba750u))
#define MA_DEBUG_TEXT ((void (*)(const void *, ...))FUN_001d5950)

/* charlib animation callbacks (Actor.cpp, not in the function table; defined by charlib) */
int Actor_AnimLoopCallback(int reason, void *inst, void *node, void *anim, void *actor);  /* 0x000ae6a0 */
int Actor_AnimQueueCallback(int reason, void *inst, void *node, void *anim, void *actor); /* 0x000ae780 */

/* charlib AI_PlayVoice (FUN_000b0320, thiscall AI player): play a voice sample of its
   character (kind, volume, float cooldown, ...) */
#define MA_CHAR_VOICE(ai, kind, volume, cooldown) \
	FUN_000b0320((int)(ai), (kind), (volume), (cooldown), 0, 0xffffffff, 0)

/* charlib FUN_000b02f0 (thiscall AI player): set the state */
#define MA_SET_STATE(ai, state) FUN_000b02f0((int)(ai), (ushort)(state))

static void ma_zero_push(void *ai)
{
	MA_I32(ai, 0x84) = 0;
	MA_I32(ai, 0x80) = 0;
	MA_I32(ai, 0x7c) = 0;
}

/* the push velocity from the stick: direction * stick amount * speed scale */
static void ma_push_from_stick(void *ai)
{
	float speed = MA_F32(ai, 0x10b0) * MA_F32(ai, 0x54);

	MA_F32(ai, 0x7c) = speed * MA_F32(ai, 0x44);
	MA_F32(ai, 0x80) = speed * MA_F32(ai, 0x48);
	MA_F32(ai, 0x84) = speed * MA_F32(ai, 0x4c);
}

/* the actor's velocity (+0x194) times k */
static void ma_scale_velocity(int actor, float k)
{
	MA_F32(actor, 0x194) = k * MA_F32(actor, 0x194);
	MA_F32(actor, 0x198) = k * MA_F32(actor, 0x198);
	MA_F32(actor, 0x19c) = k * MA_F32(actor, 0x19c);
}

static void ma_char_blend(int ch, int anim, float blend)
{
	MA_DUAL(FUN_000af580, void (*)(int, int, undefined4), void (*)(void *, int, float),
		(ch, anim, ma_fbits(blend)), ((void *)ch, anim, blend));
}

/* ---------- CModule_Meta */

/* 0x0026f678  vtable of CModule_Meta (a CGame) */
const void *const vtable_0026f678[4] = {
	(const void *)FUN_00151070, (const void *)FUN_000a6fa0, (const void *)FUN_001d5950,
	(const void *)FUN_00151090,
};

/* 0x00151040  constructor (static constructor of the "Meta" game, core) */
undefined4 *FUN_00151040(undefined4 *self)
{
	CGame_Ctor((unsigned int *)self, "Meta");
	*(const void **)self = vtable_0026f678;
	return self;
}

/* 0x00151060  destructor */
void FUN_00151060(undefined4 *self)
{
	*(const void **)self = vtable_0026f678;
	CGame_Dtor((unsigned int *)self);
}

/* 0x00151070  Meta/CModule_Meta.cpp: scalar deleting destructor */
undefined4 FUN_00151070(undefined4 self, byte flags)
{
	FUN_00151060((undefined4 *)self);
	if ((flags & 1) != 0)
		MA_DELETE(self);
	return self;
}

/* 0x00151090  Meta/CModule_Meta.cpp: Start (vtable +0x0c): push the board game mode for
   `level` (1-6 picks a level; a negative value -n restarts level n-1 and clears the
   played flags of every level) and the level's intro screen */
int FUN_00151090(void *self, int level)
{
	char name[128];
	unsigned short *entry;
	unsigned short *game;
	void *memory;
	void *mode;
	int i;

	(void)self;
	memory = MA_NEW(0xe8, "C:\\BlitzParty\\Game\\Src\\Meta\\CModule_Meta.cpp", 0x57, 0x1001);
	DAT_0030564c = memory == NULL ? 0 : (int)FUN_0014dbd0((undefined4 *)memory);
	if (DAT_0030564c == 0)
		return 0;
	FUN_000c39b0((undefined4)DAT_003055e8, DAT_00304558, 0x18, 0x18);
	if (level < 0) {
	clear:
		if (-10 < level) {
			level = -1 - level;
			/* the level table (core 0x00283c70, 0x20 bytes per level, 6 levels) */
			for (entry = (unsigned short *)DAT_00283c7c;
				(char *)entry < (char *)DAT_00283c7c + 6 * 0x20; entry += 0x10) {
				entry[1] = 0;
				game = entry;
				for (i = 0; i < (int)(unsigned int)entry[0]; i++) {
					game += 4;
					game[0] = 0;
					game[1] = 0;
				}
			}
		}
	} else {
		if (level < 7)
			level = level - 1;
		if (level < 0)
			goto clear;
	}
	if (DAT_002baea0 == 0)
		DAT_002bae88 = DAT_002bae88 - 1;
	MA_I32(DAT_0030564c, 0x88) = level;
	DAT_002baeb4 = level;
	MA_I32(DAT_0030564c, 0x80) = 2;
	if (CGameModeStack_Push(MA_MODESTACK, (struct CGameMode *)DAT_0030564c, 1) == 0)
		return 0;
	sprintf(name, "meta%d.bmp", MA_I32(DAT_0030564c, 0x88) + 1);
	memory = MA_NEW(0x300, "C:\\BlitzParty\\Game\\Src\\Meta\\CModule_Meta.cpp", 0x97, 0x1001);
	mode = memory == NULL ? NULL :
		CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)memory,
			MA_I32(DAT_0030564c, 0x88) + 0x26, name);
	return CGameModeStack_Push(MA_MODESTACK, (struct CGameMode *)mode, 1) != 0;
}

/* ---------- condition table 0x002b06d4 (nonzero: change state) */

/* 0x00151270  condition [12]: score the carried orb at a goal in reach */
undefined4 FUN_00151270(int ai)
{
	unsigned char temp[64];
	int ch;
	int goal;
	int matrix;
	float *at;

	if (MA_I32(MA_PTR(ai, 0x6c), 0x14) != 0 || DAT_002b05b0 != 0) {
		/* meta_b FUN_00166a60: the nearest goal (thiscall; ECX unused) */
		goal = MA_DUAL(FUN_00166a60, int (*)(float, undefined4), int (*)(void *, float *),
			(0.0f, (undefined4)(MA_CHAR(ai) + 0xd0)), ((void *)MA_CHAR(ai), MA_FP(MA_CHAR(ai), 0xd0)));
		if (goal != 0) {
			ch = MA_CHAR(ai);
			if (MA_F32(goal, 0xe4) - 40.0f < MA_F32(ch, 0xd4) && /* 0x00255548 */
				MA_F32(ch, 0xd4) < MA_F32(goal, 0xe4) + 70.0f && /* 0x00262ef4 */
				MA_I32(ai, 0x10bc) != 0 && MA_I32(MA_PTR(ai, 0x10bc), 0x21c) == 10) {
				at = MA_FP(goal, 0xe0);
				if (CLVec3_DistanceSqXZ(at, MA_FP(ch, 0xd0)) < DAT_002b0598 * DAT_002b0598) {
					if (MA_I32(goal, 0x460) == 0) {
						FUN_0015be40(MA_U16(MA_CHAR(ai), 0x230), (undefined4 *)at);
					} else {
						/* a goal on a moving object: its matrix in world space */
						baPrepActorInstance((struct BActorInstance *)(goal + 0x20), 1);
						baPrepActorInstance((struct BActorInstance *)(MA_I32(goal, 0x464) + 0x10), 1);
						matrix = MA_I32(goal, 0x460);
						if (((matrix | goal | (int)temp) & 0xf) == 0)
							bmMatrixMultiplyCopy((float *)temp, (float *)(matrix + 0x80), (float *)(goal + 0x20));
						else
							bmMatrixMultiply((float *)temp, (float *)(matrix + 0x80), (float *)(goal + 0x20));
						matrix = MA_I32(goal, 0x464);
						if (((matrix | goal | (int)temp) & 0xf) == 0)
							bmMatrixMultiplyCopy((float *)(goal + 0x20), (float *)(matrix + 0x10), (float *)temp);
						else
							bmMatrixMultiply((float *)(goal + 0x20), (float *)(matrix + 0x10), (float *)temp);
						FUN_0015be40(MA_U16(MA_CHAR(ai), 0x230), (undefined4 *)(goal + 0x50));
					}
					MA_DEBUG_TEXT(MA_FP(MA_CHAR(ai), 0xd0), 120.0f, 9);
					MA_F32(MA_CHAR(ai), 0x1d8) = CLMath_HeadingTo(MA_FP(MA_CHAR(ai), 0xd0), at);
					CLSamples_PlayGeneric(0x13, 0x7f, 0, 0, NULL);
					return 1;
				}
			}
		}
	}
	return 0;
}

/* 0x00151450  condition [0]: back to STAND */
undefined4 FUN_00151450(int ai)
{
	if (MA_I16(ai, 0x36) == 2) {
		if (MA_F32(ai, 0x54) < MA_F32(ai, 0x10e8))
			return 1;
	} else if (MA_I16(ai, 0x36) == 0xd) {
		if (MA_I16(MA_CHAR(ai), 0x234) != 0 && MA_I32(ai, 0x1120) < 0)
			return 1;
	} else if (MA_I16(MA_CHAR(ai), 0x234) != 0) {
		return 1;
	}
	return 0;
}

/* 0x001514b0  condition [2]: WALK (stick pushed) */
undefined4 FUN_001514b0(int ai)
{
	short state = MA_I16(ai, 0x36);

	if (state == 0 || state == 4 || state == 0xb) {
		if (MA_F32(ai, 0x54) < MA_F32(ai, 0x10e8))
			return 0;
	} else if (MA_F32(ai, 0x54) < MA_F32(ai, 0x10e8) || MA_I16(MA_CHAR(ai), 0x234) == 0) {
		return 0;
	}
	return 1;
}

/* 0x00151500  condition [7]: PRESS a switch in reach */
undefined4 FUN_00151500(int ai)
{
	int button;

	if (MA_I32(MA_PTR(ai, 0x6c), 0x14) != 0) {
		button = (int)FUN_001621d0((undefined4)MA_CHAR(ai));
		if (button != 0) {
			FUN_00162150(button);
			MA_DEBUG_TEXT(MA_FP(MA_CHAR(ai), 0xd0), 100.0f, 7);
			return 1;
		}
	}
	return 0;
}

/* 0x00151550  condition [8]: take the FLAG tile under the player */
undefined4 FUN_00151550(int ai)
{
	int tile;

	tile = FUN_00159380(MA_CHAR(ai) + 0xd0, 0);
	if ((MA_I32(MA_PTR(ai, 0x6c), 0x14) != 0 || DAT_002b05ac != 0) &&
		MA_I32(ai, 0x1140) < 4 && tile != 0 && MA_I32(tile, 0x1c) != 5 &&
		(MA_I32(tile, 0x1c) != 4 ||
			(unsigned int)MA_U16(tile, 0x12) == (unsigned int)DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10]) &&
		(MA_I16(tile, 0x10) == -1 || (int)MA_I16(tile, 0x10) == (int)MA_U16(MA_CHAR(ai), 0x230)))
		return 1;
	return 0;
}

/* 0x001515e0  condition [9]: FALL */
undefined4 FUN_001515e0(int ai)
{
	int ch;

	if (MA_I16(ai, 0x36) == 6) {
		if (MA_F32(MA_PTR(MA_CHAR(ai), 0x18c), 0x1ac) == 0.0f)
			return 0;
	} else {
		ch = MA_CHAR(ai);
		if (MA_I32(ch, 0x214) != 0)
			return 0;
		if (!(MA_F32(ch, 0x198) < MA_F32(ai, 0x10ec))) {
			if (MA_I16(ai, 0x36) != 3)
				return 0;
			if (0.0f <= MA_F32(ch, 0x198))
				return 0;
		}
	}
	return 1;
}

/* 0x00151650  condition that never fires (shared by many tables) */
undefined4 FUN_00151650(void)
{
	return 0;
}

/* 0x00151660  condition: the action button is down */
undefined1 FUN_00151660(int ai)
{
	return MA_I32(MA_PTR(ai, 0x6c), 0x14) != 0;
}

/* ---------- state table 0x002b0710 (ai, entered) */

/* 0x00151680  state [4] LAND */
undefined4 FUN_00151680(int ai, int entered)
{
	if (entered != 0) {
		FUN_000af690(MA_CHAR(ai), 5, 0, 0, 1.0f, 0.1f, 1);
		MA_CHAR_VOICE(ai, 1, DAT_00293928, 0.0f);
	}
	ma_zero_push((void *)ai);
	return 0;
}

/* 0x001516f0  state [10] HURT */
undefined4 FUN_001516f0(int *ai, int entered)
{
	if (entered != 0) {
		MA_CHAR_VOICE(ai, 2, DAT_00293928, 0.0f);
		FUN_000af690(ai[0xc], 0x11, 0, 0, DAT_002b05a4, 0.1f, 1);
		MA_VFN(ai, 0x18, void (*)(void *, int))(ai, -1);
		if (DAT_00305658 != 0)
			FUN_00156ba0((int)ai, 0, 0);
	}
	ai[0x21] = 0;
	ai[0x20] = 0;
	ai[0x1f] = 0;
	return 0;
}

/* 0x00151770  state [11] TAUNT */
undefined4 FUN_00151770(int ai, int entered)
{
	if (entered != 0) {
		MA_CHAR_VOICE(ai, 0xb, DAT_00293928, 180.0f);
		FUN_000af690(MA_CHAR(ai), 7, 0, 0, 1.0f, 0.1f, 1);
	}
	ma_zero_push((void *)ai);
	return 0;
}

/* 0x001517e0  play the animation with name hash `hash` on the actor (and on its
   attachment at +0x180): `loop` picks the looping callback, `keep` keeps the queued
   animations, a nonzero `blend` blends it in; `rate` goes to +0x23c */
void FUN_001517e0(int actor, undefined4 hash, int loop, int keep, undefined4 rate, float blend)
{
	int inst;
	struct BActorSubObject *anim;
	struct BActorSubObject *other;
	int (*callback)();

	inst = actor + 0x10;
	anim = baFindSubObjectByHash((struct BActorInstance *)inst, hash);
	MA_I16(actor, 0x236) = 0;
	MA_I16(actor, 0x234) = 0;
	MA_I16(actor, 0x232) = 0;
	if (anim == NULL) {
		MA_PRINTF("**Error*** couldn't find anim segment\n");
		return;
	}
	if (MA_I32(actor, 0x180) != 0) {
		other = baFindSubObjectByHash((struct BActorInstance *)MA_PTR(actor, 0x180), hash);
		if (other != NULL) {
			if (keep == 0)
				baFlushAnimations(MA_PTR(actor, 0x180), (struct BActorNode *)MA_PTR(MA_PTR(actor, 0x180), 0x134), 1, 0);
			callback = loop == 0 ? (int (*)())Actor_AnimQueueCallback : (int (*)())Actor_AnimLoopCallback;
			if (blend == 0.0f)
				baQueueAnimation((struct BActorInstance *)MA_PTR(actor, 0x180),
					(struct BActorNode *)MA_PTR(MA_PTR(actor, 0x180), 0x134), (struct BAnim *)other,
					0x400, -1, 0, callback, (void *)actor);
			else
				baQueueAnimationBlendIn((struct BActorInstance *)MA_PTR(actor, 0x180),
					(struct BActorNode *)MA_PTR(MA_PTR(actor, 0x180), 0x134), (struct BAnim *)other,
					0x400, -1, blend, 0, callback, (void *)actor);
		}
	}
	if (keep == 0)
		baFlushAnimations((void *)inst, (struct BActorNode *)MA_PTR(actor, 0x144), 1, 0);
	callback = loop == 0 ? (int (*)())Actor_AnimQueueCallback : (int (*)())Actor_AnimLoopCallback;
	if (blend != 0.0f) {
		baQueueAnimationBlendIn((struct BActorInstance *)inst, (struct BActorNode *)MA_PTR(actor, 0x144),
			(struct BAnim *)anim, 0x400, -1, blend, 0, callback, (void *)actor);
		MA_U32(actor, 0x23c) = rate;
		return;
	}
	baQueueAnimation((struct BActorInstance *)inst, (struct BActorNode *)MA_PTR(actor, 0x144),
		(struct BAnim *)anim, 0x400, -1, 0, callback, (void *)actor);
	MA_U32(actor, 0x23c) = rate;
}

/* 0x001519a0  the score of a ball type (1-4) */
undefined4 FUN_001519a0(int kind)
{
	return (undefined4)DAT_002b07c4[kind];
}

/* 0x001519b0  condition [5]: ATTACK a player in front and in reach (1), or attack
   something hittable in front (1), else 2 */
undefined4 FUN_001519b0(int ai)
{
	int hit;
	int ch;
	int other;
	unsigned int player;
	float heading;
	float turn;
	float distance;
	double reach;
	float ray_from[4];
	float ray[4];
	unsigned char result[16];
	float k;
	double t;

	hit = 0;
	if (MA_I32(MA_PTR(ai, 0x6c), 0x14) == 0)
		return 0;
	if (0 < DAT_002bae94) {
		player = 0;
		do {
			ch = MA_CHAR(ai);
			if (player != MA_U16(ch, 0x230) && MA_I16(DAT_003075bc[player], 0x36) != 0xd) {
				other = MA_CHAR(DAT_003075bc[player]);
				if (fabsf(MA_F32(ch, 0xd4) - MA_F32(other, 0xd4)) < MA_F32(ai, 0x10f8)) {
					heading = CLMath_HeadingTo(MA_FP(ch, 0xd0), MA_FP(other, 0xd0));
					turn = CLMath_AngleDistance(heading, MA_F32(MA_CHAR(ai), 0x1e8));
					if ((double)turn < (double)DAT_002b074c * (double)0.017453292f) { /* 0x002640f8 */
						distance = FUN_00051500(MA_FP(MA_CHAR(ai), 0xd0), MA_FP(MA_CHAR(DAT_003075bc[player]), 0xd0));
						reach = (double)*(float *)((char *)&DAT_00290728 +
							DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60);
						if (MA_F32(ai, 0x10a4) != 0.0f)
							reach = reach * (double)DAT_00293a38;
						/* 0x00253850 = 1.0, 0x00257c78 = 0.8 */
						if (((double)turn + 1.0) * (double)distance < reach * (double)0.8f) {
							MA_U16(DAT_003075bc[player], 0x1088) = MA_U16(MA_CHAR(ai), 0x230);
							MA_I16(ai, 0x1084) = (short)player;
							MA_F32(MA_CHAR(ai), 0x1d8) = heading;
							MA_DEBUG_TEXT(MA_FP(MA_CHAR(ai), 0xd0), 100.0f, 8);
							return 1;
						}
					}
				}
			}
			player++;
		} while ((int)player < DAT_002bae94);
	}
	/* a ray in front of the player, at chest height, as long as the attack reach */
	ch = MA_CHAR(ai);
	ray_from[0] = MA_F32(ch, 0xd0);
	ray_from[2] = MA_F32(ch, 0xd8);
	ray_from[1] = MA_F32(ch, 0xd4) + 80.0f; /* 0x0026556c */
	ray_from[3] = MA_F32(ch, 0xdc);
	FUN_00051170(ray, DAT_0028fdd8, MA_F32(ch, 0x1e8));
	k = *(float *)((char *)&DAT_00290728 + DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60);
	ray[0] = ray[0] * k;
	ray[1] = ray[1] * k;
	ray[2] = ray[2] * k;
	t = (double)FUN_000b43e0(ray_from, ray, (undefined4 *)result, NULL, &hit);
	if (hit != 0 && MA_I32(hit, 0x440) != 0 && 0.0 < t) {
		MA_U16(ai, 0x1084) = 0xffff;
		return 1;
	}
	MA_U16(ai, 0x1084) = 0xffff;
	return 2;
}

/* 0x00151c60  state [14] DROWN: sink, gurgle, then respawn */
undefined4 FUN_00151c60(int *ai, int entered)
{
	unsigned int flags;
	int ch;
	int frames;
	int level;
	float y;

	if (entered != 0) {
		FUN_000af690(ai[0xc], 0x1f, 1, 0, 1.0f, 0.1f, 1);
		MA_U32(ai[0xc], 0x220) = MA_U32(ai[0xc], 0x220) & 0xfffffffe;
		ch = ai[0xc];
		MA_I32(ch, 0x194) = 0;
		MA_I32(ch, 0x198) = 0;
		MA_I32(ch, 0x19c) = 0;
		MA_F32(ch, 0x1a0) = 1.0f;
		CLSamples_Play((int)bLoadResource(NULL, "gen_splash_03a.wav", 2, 0xdefa), 0xff, 0, 0, NULL);
		FUN_00156ba0((int)ai, 0, 1);
	}
	/* bob: up when over a vent (meta_b FUN_00163120), else sink */
	if (FUN_00163120((int)ai, 150.0f, (float)(ma_rand() % DAT_002b0764)) == 0)
		y = MA_F32(ai[0xc], 0x198) - DAT_002b0760 * DAT_0028fda8;
	else
		y = DAT_0028fda8 * DAT_002b075c + MA_F32(ai[0xc], 0x198);
	MA_F32(ai[0xc], 0x198) = y;
	if (DAT_002b0754 <= MA_F32(ai, 0x40)) {
		flags = MA_U32(ai[0xc], 0x220);
		if ((flags & 1) == 0) {
			MA_U32(ai[0xc], 0x220) = flags | 1;
			CLSamples_Play((int)bLoadResource(NULL, "gurgle1.wav", 2, 0xdefa), 0xff, 0, 0, NULL);
		}
		MA_U32(ai[0xc], 0x220) = MA_U32(ai[0xc], 0x220) | 0x200;
		/* fade out over [0x002b0754, 0x002b0758] */
		frames = (int)((((double)MA_F32(ai, 0x40) - DAT_002b0754) * 256.0) /
			((double)DAT_002b0758 - DAT_002b0754)); /* 0x00257b90 = 256 */
		level = 0x100 - frames;
		MA_I32(ai[0xc], 0x224) = level < 0 ? 0 : level;
	}
	if (DAT_002b0758 <= MA_F32(ai, 0x40)) {
		MA_VFN(ai, 0x24, void (*)(void *))(ai);
		MA_U32(ai[0xc], 0x220) = MA_U32(ai[0xc], 0x220) & 0xfffffdff;
		MA_I32(ai[0xc], 0x224) = 0x100;
	}
	ai[0x21] = 0;
	ai[0x20] = 0;
	ai[0x1f] = 0;
	ch = ai[0xc];
	/* 0x0026f6f0: 0.8 (double), pow(0.8, frame time) */
	MA_F32(ch, 0x198) = (float)(pow((double)0.800000011920929, (double)DAT_0028fda8) * (double)MA_F32(ch, 0x198));
	return 0;
}

/* 0x00151e70  state [12] SCORE */
undefined4 FUN_00151e70(int ai, int entered)
{
	if (entered != 0) {
		if (2 < MA_I32(ai, 0x1140))
			MA_CHAR_VOICE(ai, 4, DAT_00293928, 180.0f);
		FUN_000af690(MA_CHAR(ai), 0xc, 0, 0, 1.0f, 0.1f, 1);
		ma_scale_velocity(MA_CHAR(ai), DAT_002b0750);
	}
	ma_zero_push((void *)ai);
	return 0;
}

/* 0x00151f20  state [0] STAND */
undefined4 FUN_00151f20(int ai, int entered)
{
	if (entered != 0) {
		FUN_000af690(MA_CHAR(ai), 0, 1, 0, 1.0f, 0.1f, 1);
		ma_scale_velocity(MA_CHAR(ai), DAT_002b0750);
	}
	ma_zero_push((void *)ai);
	/* after a while: IDLE, 1 in 30 frames */
	if (DAT_002b0768 <= MA_F32(ai, 0x40) && ma_rand() % 0x1e == 0)
		MA_SET_STATE(ai, 1);
	return 0;
}

/* 0x00151ff0  state [1] IDLE */
undefined4 FUN_00151ff0(int ai, int entered)
{
	if (entered != 0)
		FUN_000af690(MA_CHAR(ai), 0xb, 1, 0, 1.0f, 0.1f, 1);
	ma_zero_push((void *)ai);
	if (DAT_002b076c < MA_F32(ai, 0x40)) {
		if (ma_rand() % 0x1e == 0)
			MA_SET_STATE(ai, 0);
	}
	return 0;
}

/* 0x00152080  state [2] WALK */
undefined4 FUN_00152080(int ai, int entered)
{
	int ch;
	float rate;

	if (entered != 0) {
		MA_I32(ai, 0x10e4) = 1;
		if (DAT_00305650 == 0 || MA_I16(ai, 0x38) != 4) {
			FUN_000af690(MA_CHAR(ai), 1, 1, 0, 0.0f, DAT_0028fda8 * 0.1f, 1); /* 0x002536cc */
		} else {
			baFlushAnimations((void *)(MA_CHAR(ai) + 0x10), (struct BActorNode *)MA_PTR(MA_CHAR(ai), 0x144), 1, 0);
			MA_I16(MA_CHAR(ai), 0x236) = 0;
			ch = MA_CHAR(ai);
			baQueueBlendedAnimation((struct BActorInstance *)(ch + 0x10), (struct BActorNode *)MA_PTR(ch, 0x144),
				(struct BAnim *)MA_PTR(ch, 0x2c4), (struct BAnim *)MA_PTR(ch, 0x278), DAT_002b0770,
				DAT_002b0774, -1, -1, DAT_002b0778, 3, 0, 0);
		}
	}
	if (DAT_00305650 != 0 && MA_I16(ai, 0x38) == 4 && MA_I32(ai, 0x3c) == DAT_002b0780)
		baChangeAnimation((struct BActorNode *)MA_PTR(MA_CHAR(ai), 0x144), 1, DAT_002b0770, DAT_002b0774,
			DAT_002b077c, 0.0f, 7);
	if (MA_I32(ai, 0x10e4) != 0) {
		if (DAT_002b059c < MA_F32(MA_CHAR(ai), 0x254)) {
			ma_char_blend(MA_CHAR(ai), 0x18, 0.05f);
			MA_I32(ai, 0x10e4) = 0;
			goto speed;
		}
		if (MA_I32(ai, 0x10e4) != 0)
			goto speed;
	}
	if (MA_F32(MA_CHAR(ai), 0x254) < DAT_002b05a0) {
		ma_char_blend(MA_CHAR(ai), 1, 0.05f);
		MA_I32(ai, 0x10e4) = 1;
	}
speed:
	ch = MA_CHAR(ai);
	if (MA_I32(ai, 0x10e4) == 0)
		rate = MA_F32(MA_PTR(ch, 0x324), 0x6a0);
	else
		rate = MA_F32(MA_PTR(ch, 0x324), 0x84);
	MA_F32(ch, 0x23c) = (rate * MA_F32(ch, 0x254)) / DAT_0028fda8;
	ma_push_from_stick((void *)ai);
	MA_F32(MA_CHAR(ai), 0x1d8) = CLMath_HeadingXZ(MA_FP(ai, 0x7c));
	return 0;
}

/* 0x00152260  state [3] JUMP */
undefined4 FUN_00152260(int ai, int entered)
{
	int ch;
	int platform;

	if (entered != 0) {
		MA_F32(MA_CHAR(ai), 0x198) = DAT_00292c98;
		ch = MA_CHAR(ai);
		platform = MA_I32(MA_PTR(ch, 0x18c), 0x1b0);
		if (platform != 0) {
			MA_F32(ch, 0x194) = MA_F32(platform, 0x1a4) + MA_F32(ch, 0x194);
			ch = MA_CHAR(ai);
			MA_F32(ch, 0x198) = MA_F32(MA_PTR(MA_PTR(ch, 0x18c), 0x1b0), 0x1a8) + MA_F32(ch, 0x198);
			ch = MA_CHAR(ai);
			MA_F32(ch, 0x19c) = MA_F32(MA_PTR(MA_PTR(ch, 0x18c), 0x1b0), 0x1ac) + MA_F32(ch, 0x19c);
			MA_I32(MA_PTR(MA_CHAR(ai), 0x18c), 0x1b0) = 0;
			MA_I32(MA_PTR(MA_CHAR(ai), 0x18c), 0x1b8) = 0;
		}
		FUN_000af690(MA_CHAR(ai), 2, 0, 0, 1.0f, 0.3f, 1);
		if (MA_F32(ai, 0x10e8) <= MA_F32(ai, 0x54))
			MA_F32(MA_CHAR(ai), 0x1d8) = CLMath_HeadingXZ(MA_FP(ai, 0x7c));
		if (MA_I16(MA_CHAR(ai), 0x230) == 0) {
			DAT_002f2894 = MA_F32(MA_CHAR(ai), 0xd4);
			DAT_002f2890 = 0.0f;
		}
		MA_CHAR_VOICE(ai, 0, DAT_00293928, 0.0f);
		MA_F32(MA_CHAR(ai), 0x238) = -1.0f;
	}
	ma_push_from_stick((void *)ai);
	return 0;
}

/* 0x001523d0  state [5] ATTACK */
undefined4 FUN_001523d0(int *ai, int entered)
{
	unsigned int kind;
	int anim;
	int sample;
	int sync;
	float speed;
	float blend;
	float k;
	float heading;

	if (entered != 0) {
		if (MA_F32(ai, 0x10a4) == 0.0f) {
			kind = (unsigned int)ma_rand() % 3;
			if (kind == 0)
				sample = (ma_rand() & 1) + 0xf;
			else
				sample = (ma_rand() & 1) + 0x11;
			CLSamples_PlayGeneric(sample, 0x7f, 0, 0, NULL);
			blend = DAT_002b078c[kind];
			speed = DAT_002b079c[kind];
			sync = 1;
			anim = kind + 0xd;
		} else {
			/* super attack */
			CLSamples_PlayGeneric((ma_rand() & 1) + 0x11, 0x7f, 0, 0, NULL);
			sync = 0;
			speed = DAT_002b079c[3];
			blend = DAT_002b078c[3];
			anim = MA_I32(ai[0xc], 0x274) == 0 ? 0xe : 4;
		}
		FUN_000af690(ai[0xc], anim, 0, 0, speed, blend, sync);
		MA_I16(ai, 0x1092) = 0;
	}
	if (DAT_002b0788 <= MA_F32(ai, 0x40) || MA_F32(ai, 0x40) < DAT_00305654) {
		ai[0x21] = 0;
		ai[0x20] = 0;
		ai[0x1f] = 0;
	} else {
		FUN_00051170(MA_FP(ai, 0x7c), DAT_0028fdd8, MA_F32(ai[0xc], 0x1d8));
		k = DAT_002b0784;
		MA_F32(ai, 0x7c) = DAT_002b0784 * MA_F32(ai, 0x7c);
		MA_F32(ai, 0x80) = k * MA_F32(ai, 0x80);
		MA_F32(ai, 0x84) = k * MA_F32(ai, 0x84);
	}
	if (MA_I16(ai, 0x1084) != -1) {
		heading = CLMath_HeadingTo(MA_FP(ai[0xc], 0xd0), MA_FP(MA_CHAR(DAT_003075bc[MA_I16(ai, 0x1084)]), 0xd0));
		MA_F32(ai[0xc], 0x1d8) = heading;
		if (entered != 0)
			MA_F32(ai[0xc], 0x1e8) = heading;
	}
	if (MA_F32(ai, 0x1104) <= MA_F32(ai, 0x40) &&
		(MA_F32(ai, 0x40) <= MA_F32(ai, 0x1108) || MA_I16(ai, 0x1092) == 0))
		MA_VFN(ai, 0x10, int (*)(void *))(ai);
	return 0;
}

/* 0x00152600  state [6] JUMP_ATTACK: push the nearest player in front */
undefined4 FUN_00152600(int *ai, int entered)
{
	unsigned int best;
	unsigned int player;
	float value;
	float best_turn;

	best = 0xffffffff;
	best_turn = 1.5707963f;
	MA_F32(ai, 0x1128) = 1.6022427f; /* 0x3fcd154a */
	if (entered != 0) {
		FUN_000af690(ai[0xc], 0x10, 0, 0, 1.0f, 0.1f, 1);
		MA_I16(ai, 0x1092) = 0;
		player = 0;
		do {
			if (player != MA_U16(ai[0xc], 0x230) && MA_I16(DAT_003075bc[player], 0x36) != 0xd) {
				value = CLVec3_DistanceSqXZ(MA_FP(ai[0xc], 0xd0), MA_FP(MA_CHAR(DAT_003075bc[player]), 0xd0));
				if ((double)value < (double)DAT_002b07b0 * (double)DAT_002b07b0) {
					value = CLMath_HeadingTo(MA_FP(ai[0xc], 0xd0), MA_FP(MA_CHAR(DAT_003075bc[player]), 0xd0));
					value = CLMath_AngleDistance(value, MA_F32(ai[0xc], 0x1e8));
					if (best_turn == -1.0f || value < best_turn) { /* 0x00253854 */
						best_turn = value;
						best = player;
					}
				}
			}
			player++;
		} while ((int)player < 4);
		if (best == 0xffffffff) {
			MA_U16(ai, 0x1084) = 0xffff;
		} else {
			MA_F32(ai[0xc], 0x1d8) = CLMath_HeadingTo(MA_FP(ai[0xc], 0xd0),
				MA_FP(MA_CHAR(DAT_003075bc[best]), 0xd0));
			MA_I16(ai, 0x1084) = (short)best;
		}
	}
	if (MA_I16(ai, 0x1084) == -2) {
		ai[0x1f] = 0;
		ai[0x20] = 0;
		ai[0x21] = 0;
		MA_F32(ai, 0x88) = 1.0f;
	} else {
		MA_F32(ai, 0x84) = DAT_002b07ac;
		ai[0x1f] = 0;
		ai[0x20] = 0;
		FUN_00051170(MA_FP(ai, 0x7c), MA_FP(ai, 0x7c), MA_F32(ai[0xc], 0x1d8));
	}
	if (MA_I16(ai, 0x1084) != -2 &&
		MA_F32(ai, 0x110c) <= MA_F32(ai, 0x40) &&
		(MA_F32(ai, 0x40) <= MA_F32(ai, 0x1110) || MA_I16(ai, 0x1092) == 0) &&
		MA_VFN(ai, 0x10, int (*)(void *))(ai) != 0)
		MA_U16(ai, 0x1084) = 0xfffe;
	return 0;
}

/* 0x00152800  state [7] PRESS */
undefined4 FUN_00152800(int ai, int entered)
{
	int ch;

	if (entered != 0) {
		FUN_000af690(MA_CHAR(ai), 6, 0, 0, 1.0f, 0.1f, 1);
		ma_scale_velocity(MA_CHAR(ai), DAT_002b0750);
	}
	ch = MA_CHAR(ai);
	ma_zero_push((void *)ai);
	MA_F32(ch, 0x194) = MA_F32(ch, 0x194) * 0.5f; /* 0x002536d0 */
	MA_F32(ch, 0x198) = MA_F32(ch, 0x198) * 0.5f;
	MA_F32(ch, 0x19c) = MA_F32(ch, 0x19c) * 0.5f;
	return 0;
}

/* 0x001528c0  state [8] FLAG: take the tile under the player */
undefined4 FUN_001528c0(int ai, int entered)
{
	int ch;

	if (entered != 0) {
		FUN_000af690(MA_CHAR(ai), 6, 0, 0, 1.0f, 0.1f, 1);
		FUN_0015ac80((int *)ai, FUN_00159380(MA_CHAR(ai) + 0xd0, 0));
		ma_scale_velocity(MA_CHAR(ai), DAT_002b0750);
	}
	if (DAT_002b07b4 == 0) {
		ch = MA_CHAR(ai);
		MA_F32(ch, 0x194) = MA_F32(ch, 0x194) * 0.5f; /* 0x002536d0 */
		MA_F32(ch, 0x198) = MA_F32(ch, 0x198) * 0.5f;
		MA_F32(ch, 0x19c) = MA_F32(ch, 0x19c) * 0.5f;
		ma_zero_push((void *)ai);
	} else {
		ma_push_from_stick((void *)ai);
		if (MA_F32(ai, 0x10e8) <= MA_F32(ai, 0x54)) {
			MA_F32(MA_CHAR(ai), 0x1d8) = CLMath_HeadingXZ(MA_FP(ai, 0x7c));
			return 0;
		}
	}
	return 0;
}

/* 0x001529f0  state [9] FALL (also in Hopper's table) */
undefined4 FUN_001529f0(int ai, int entered)
{
	if (entered != 0)
		FUN_000af690(MA_CHAR(ai), 10, 1, 0, 1.0f, 0.3f, 1);
	ma_push_from_stick((void *)ai);
	return 0;
}

/* 0x00152a50  a player was hit by `attacker`: drop a carried tile, and the voice */
void FUN_00152a50(int *attacker, int victim)
{
	if (0 < MA_I32(victim, 0x1140)) {
		/* the attacker steals the victim's tiles */
		FUN_00158050((int)attacker, MA_I32(victim, 0x1140));
		FUN_00156ba0(victim, 0, 0);
		if (ma_rand() % 100 < 0x32) {
			CLSamples_PlayFromSet(*(char *)&DAT_002baed4[MA_U16(MA_CHAR(victim), 0x230) * 10], 7,
				MA_FP(MA_CHAR(victim), 0xd0), DAT_00293928, 0, -1, 0);
			MA_VFN(attacker, 0x18, void (*)(void *, int))(attacker, -1);
			return;
		}
		CLSamples_PlayFromSet(*(char *)&DAT_002baed4[MA_U16(attacker[0xc], 0x230) * 10], 6,
			MA_FP(attacker[0xc], 0xd0), DAT_00293928, 0, -1, 0);
		MA_VFN(attacker, 0x18, void (*)(void *, int))(attacker, -1);
	}
}

/* 0x00152b30  state [13] KNOCKOUT */
undefined4 FUN_00152b30(int *ai, int entered)
{
	float k;

	if (entered != 0) {
		MA_CHAR_VOICE(ai, 3, DAT_00293928, 0.0f);
		FUN_000af690(ai[0xc], 0x13, 0, 0, DAT_002b05a8, 0.1f, 1);
		ai[0x448] = 1;
		MA_VFN(ai, 0x18, void (*)(void *, int))(ai, -1);
	}
	if (DAT_002b07bc <= MA_F32(ai, 0x40)) {
		ai[0x21] = 0;
		ai[0x20] = 0;
		ai[0x1f] = 0;
	} else {
		FUN_00051170(MA_FP(ai, 0x7c), DAT_0028fdd8, MA_F32(ai[0xc], 0x1d8));
		k = DAT_002b07c0;
		if (ai[0x448] != 0)
			k = -DAT_002b07c0;
		MA_F32(ai, 0x7c) = k * MA_F32(ai, 0x7c);
		MA_F32(ai, 0x80) = k * MA_F32(ai, 0x80);
		MA_F32(ai, 0x84) = k * MA_F32(ai, 0x84);
	}
	if (-1 < ai[0x448] && MA_F32(ai, 0x10d8) <= MA_F32(ai, 0x40)) {
		FUN_000af690(ai[0xc], ai[0x448] == 0 ? 0x16 : 0x15, 0, 0, 1.0f, 0.1f, 1);
		ai[0x448] = -1;
	}
	return 0;
}

/* (the condition table 0x002b06d4 and the state table 0x002b0710 are in the Meta data
   block, Meta/meta_a_data.c) */

/* ---------- level helpers */

/* 0x00152c60  the input maps of the four players (stick, jump, action, jump cheat) */
void FUN_00152c60(void)
{
	unsigned int pad;
	int player;
	int i;

	for (i = 0; i < DAT_002bae94; i++) {
		player = DAT_003075bc[i];
		pad = (unsigned int)DAT_002baee0[i * 10];
		MA_PTR(player, 0x5c) = biCreateMap(4);
		MA_PTR(player, 0x60) = biCreateChannel((struct biMap *)MA_PTR(player, 0x5c), "Forward", 3, 0, 0, 0, 0);
		MA_PTR(player, 0x64) = biCreateChannel((struct biMap *)MA_PTR(player, 0x5c), "leftRight", 3, 0, 0, 0, 0);
		MA_PTR(player, 0x68) = biCreateChannel((struct biMap *)MA_PTR(player, 0x5c), "jump", 1, 1, 0, 0, 0);
		MA_PTR(player, 0x6c) = biCreateChannel((struct biMap *)MA_PTR(player, 0x5c), "action", 1, 1, 0, 0, 0);
		MA_PTR(player, 0x74) = biCreateMap(1);
		MA_PTR(player, 0x78) = biCreateChannel((struct biMap *)MA_PTR(player, 0x74), "jumpcheat", 1, 0, 0, 0, 0);
		biBindInput((struct biChannel *)MA_PTR(player, 0x60), pad | 0xba, 0);
		biBindInput((struct biChannel *)MA_PTR(player, 0x64), pad | 0xb9, 0);
		biBindInput((struct biChannel *)MA_PTR(player, 0x60), pad | 0xbe, pad | 0xbd);
		biBindInput((struct biChannel *)MA_PTR(player, 0x64), pad | 0xc0, pad | 0xbf);
		biBindInput((struct biChannel *)MA_PTR(player, 0x68), pad | 0xc5, 0);
		biBindInput((struct biChannel *)MA_PTR(player, 0x6c), pad | 199, 0);
		biBindInput((struct biChannel *)MA_PTR(player, 0x6c), pad | 0xc6, 0);
	}
}

/* 0x00152e50  load the level script (meta_b FUN_00166db0, Meta/LevelLoad.cpp) */
undefined1 FUN_00152e50(int self)
{
	int i;

	MA_PRINTF("loading meta level\n");
	for (i = 0; i < 1000; i++)
		DAT_003045c8[i] = 0;
	return FUN_00166db0(MA_I32(self, 0x88), MA_I32(self, 0x90) + 1) != 0;
}

/* 0x00152e90  delete the extra camera object */
void FUN_00152e90(void)
{
	if (DAT_002f7a34 != 0)
		MA_VFN(DAT_002f7a34, 0, void (*)(void *, int))((void *)DAT_002f7a34, 1);
}

/* 0x00152eb0  update the extra camera and turn the players' stick frames to face it */
void FUN_00152eb0(int *camera)
{
	int ch;
	int i;

	if (camera == NULL)
		return;
	MA_VFN(camera, 4, void (*)(void *, float))(camera, 1.0f);
	_DAT_002f7a2c = camera[0x1f];
	for (i = 0; i < DAT_002bae94; i++) {
		if (DAT_002f289c[i] != 0) {
			ch = MA_CHAR(DAT_002f289c[i]);
			MA_F32(DAT_002f289c[i], 0x1118) = CLMath_WrapAngle((float)atan2(
				(double)MA_F32(ch, 0xd0) - (double)MA_F32(camera, 0x68),
				(double)MA_F32(ch, 0xd8) - (double)MA_F32(camera, 0x70)));
		}
	}
}

/* 0x00152f30  constructor of the debug object (0x58 bytes, vtable 0x0026f72c) */
const void *const vtable_0026f72c[1] = { (const void *)FUN_00152f60 };

undefined4 *FUN_00152f30(undefined4 *self)
{
	*(const void **)self = vtable_0026f72c;
	self[0x11] = 0;
	self[0x10] = 0;
	self[0x12] = 1;
	self[0x13] = 5;
	return self;
}

/* 0x00152f50  debug object: reset */
undefined4 FUN_00152f50(int self)
{
	MA_I32(self, 0x3c) = 0;
	return 1;
}

/* 0x00152f60  debug object: scalar deleting destructor */
undefined4 *FUN_00152f60(undefined4 *self, byte flags)
{
	*(const void **)self = vtable_0026f72c;
	if ((flags & 1) != 0)
		MA_DELETE(self);
	return self;
}

/* 0x00152f80  (empty in this build) */
void FUN_00152f80(void)
{
	MA_DEBUG_TEXT(NULL);
}

/* 0x00152f90  clear the level's object table (1000 entries) */
void FUN_00152f90(void)
{
	int i;

	for (i = 0; i < 1000; i++)
		DAT_00305660[i] = 0;
}

/* 0x00152fb0  reset the puzzles: every action's timer to -1, the fired flags off */
void FUN_00152fb0(void)
{
	unsigned int group;  /* puzzle */
	unsigned int light;  /* action */

	for (group = CLList_Next((unsigned int *)DAT_003075b0, NULL); group != 0;
		group = CLList_Next((unsigned int *)DAT_003075b0, (unsigned int *)group)) {
		for (light = CLList_Next((unsigned int *)(group + 0xc), NULL); light != 0;
			light = CLList_Next((unsigned int *)(group + 0xc), (unsigned int *)light))
			MA_F32(light, 0xc) = -1.0f;
		MA_I32(group, 0x1c) = 0;
	}
}

/* 0x00153020  a puzzle's action timers: those at or below 0 count up by their speed */
void FUN_00153020(int group)
{
	unsigned int light;
	float level;

	for (light = CLList_Next((unsigned int *)(group + 0xc), NULL); light != 0;
		light = CLList_Next((unsigned int *)(group + 0xc), (unsigned int *)light)) {
		if (MA_F32(light, 0xc) <= 0.0f) {
			level = MA_F32(light, 0x10) + MA_F32(light, 0xc);
			if (level < 0.0001f) /* 0x00266ff4 */
				level = 0.0001f;
			MA_F32(light, 0xc) = level;
		}
	}
}
