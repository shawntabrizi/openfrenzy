/*
EFFECTS.C  <-  C:\BlitzParty\Game\Src\Meta\Effects.cpp (0x0015b7a0-0x0015c7a0 tagged or
bracketed) and the gap functions next to it that use the same data: 0x0015b500-0x0015b760
(before the first tag) and 0x0015c840, 0x0015c890 (after the last). Their file is
inferred from the data they share; the XBE has no source path for them.

The board's effects:

- the goal effects: when a player scores orbs, a burst at the goal. Per player one
  "goalsfx.obe" flare actor (DAT_00308370) and one CParticleSystem (helix rings and
  sparks, the array DAT_00308420); started by FUN_0015be40 after a 20-frame delay.
- the fish: "harbourmesh_fish.obe" (one, at load) and the leaping "fish.obe" actors that
  a puzzle throws (charlib actors of type 0xf; they sink, and splash into the water).
- the glow sprites (CLList DAT_00308440, 0x5c bytes each): camera-facing sprites from
  the level script (CAMERA_ALIGNED_SPRITE) and the tiles, optionally following an actor
  node.
- the sliding-texture models (CLList DAT_0030844c, 0x34 bytes each): actors drawn with a
  scrolling texture transform (ADD_SLIDDING_TEXTURE_MODEL).

Layouts (original offsets):

  glow sprite (0x5c)                         sliding-texture model (0x34)
  +0x00 CLList node (next, prev)             +0x00 CLList node
  +0x08 float pos[4]                         +0x08 float speed u, +0x0c speed v
  +0x18 float offset[4] from the node        +0x10 float u, +0x14 v (wrapped to [0, 1))
  +0x28 float width, +0x2c height            +0x18 int tag
  +0x30 float pull toward the camera         +0x1c int on
  +0x34 float angle                          +0x20 float pos[4]
  +0x3c int visible (set by the creator)     +0x30 the charlib actor
  +0x40..+0x4c int r, g, b, a
  +0x50 texture, +0x54 node, +0x58 actor
*/

#include "meta_a.h"

#define MA_EFFECTS_CPP "C:\\BlitzParty\\Game\\Src\\Meta\\Effects.cpp"

#define MA_GLOWS ((unsigned int *)DAT_00308440)
#define MA_SLIDERS ((unsigned int *)DAT_0030844c)
#define MA_NEXT(list, node) CLList_Next((unsigned int *)(list), (unsigned int *)(node))

/* the Meta .data block values used here */
#define MA_GOAL_ANIM_SPEED (*(float *)MA_META_ADDR(0x002b0a20u))  /* 1.6 */
#define MA_FISH_SPREAD (*(int *)MA_META_ADDR(0x002b0a24u))        /* 100 */
#define MA_FISH_LEAP (*(int *)MA_META_ADDR(0x002b0a28u))          /* 20 */
#define MA_FISH_0A2C (*(unsigned int *)MA_META_ADDR(0x002b0a2cu)) /* 40.0f */
#define MA_FISH_0A30 (*(unsigned int *)MA_META_ADDR(0x002b0a30u)) /* 300.0f */
#define MA_FISH_0A34 (*(unsigned int *)MA_META_ADDR(0x002b0a34u)) /* 0.8f */
#define MA_FISH_0A38 (*(unsigned int *)MA_META_ADDR(0x002b0a38u)) /* -1.0f */
#define MA_FISH_ANIM_SPEED (*(int *)MA_META_ADDR(0x002b0a3cu))    /* 0x640 */
#define MA_FISH_SINK (*(int *)MA_META_ADDR(0x002b0a40u))          /* 5 */
#define MA_SPLASH_PITCH (*(int *)MA_META_ADDR(0x002b0a44u))       /* 21000 */
#define MA_SPLASH_PITCH_RANGE (*(int *)MA_META_ADDR(0x002b0a48u)) /* 5000 */

/* a goal effect's flare actor (0x18 bytes) */
struct MaGoalFx
{
	struct BActorInstance *inst; /* +0x00 */
	struct BAnim *anim;          /* +0x04 the "flare" segment */
	int r, g, b;                 /* +0x08 ambient light while drawn (player colour) */
	int active;                  /* +0x14 the flare animation runs */
};
OF_ASSERT_SIZE(struct MaGoalFx, 0x18);

/* CParticleSystem parameters (classlib_fx CPSParams, 0x40 bytes) */
struct MaPSParams
{
	unsigned char active, unknown01;
	unsigned short pad02;
	float angleSpeed, riseAccel, gravity, riseSpeed, speed, drag, radiusSpeed, radius;
	int numRings, ringLength, maxEmit, lifeMin, lifeRange;
	float pointSize, emitTime;
};
OF_ASSERT_SIZE(struct MaPSParams, 0x40);

#define MA_PS_SIZE 0xa8 /* sizeof(CParticleSystem) */
#define MA_PS(i) ((struct CParticleSystem *)((char *)DAT_00308420 + (i) * MA_PS_SIZE))

/* ---------- data (.bss) */

/* 0x00308370  the goal effects' flare actors, one per player */
struct MaGoalFx DAT_00308370[4];
/* 0x003083d8  the goal positions */
float DAT_003083d8[4][4];
/* 0x0030841c  "goalsfx.obe" */
struct bkResource *DAT_0030841c;
/* 0x00308420  the goal particle systems (new[4] CParticleSystem) */
void *DAT_00308420;
/* 0x00308424  frames until the goal effect starts */
float DAT_00308424[4];
/* 0x00308434 */
int DAT_00308434;
/* 0x0030843c  the harbour fish actor */
void *DAT_0030843c;

/* 0x0015b5e0  (missing from the function table) Meta/Effects.cpp: animation callback of
   a goal flare: the animation ended */
static int Effects_GoalFlareDone(int reason, void *instance, void *node, struct BAnim *anim,
	void *context)
{
	(void)reason;
	(void)instance;
	(void)node;
	(void)anim;
	((struct MaGoalFx *)context)->active = 0;
	return 1;
}

/* 0x0015b500  Meta (file inferred: Effects.cpp): draw the goal effects: the particle
   systems, then the running flares in their player's ambient colour */
void FUN_0015b500(void)
{
	int ambient_r, ambient_g, ambient_b;
	int running;
	int i;

	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(4, 0, 0);
	if (DAT_00308420 == NULL)
		return;
	running = 0;
	for (i = 0; i < 4; i++) {
		MA_VFN(MA_PS(i), 8, void (*)(void *))(MA_PS(i)); /* Draw */
		if (DAT_00308370[i].active != 0)
			running++;
	}
	if (running == 0)
		return;
	ambient_r = DAT_00284b10;
	ambient_g = DAT_00284b14;
	ambient_b = DAT_00284b18;
	bdSetRenderState(3, 1, 0);
	for (i = 0; i < 4; i++) {
		if (DAT_00308370[i].active == 1) {
			baSetAmbientLight(DAT_00308370[i].r, DAT_00308370[i].g, DAT_00308370[i].b);
			baDrawActorInstance(DAT_00308370[i].inst, 7);
		}
	}
	bdSetRenderState(3, 0, 0);
	baSetAmbientLight(ambient_r, ambient_g, ambient_b);
}

/* 0x0015b600  Meta (file inferred: Effects.cpp): load the harbour fish */
void FUN_0015b600(void)
{
	float pos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	DAT_0030843c = Actor_LoadAny("harbourmesh_fish.obe", pos, 0xe, NULL, 0);
	MA_U32(DAT_0030843c, 0x220) = MA_U32(DAT_0030843c, 0x220) | 0x100000;
}

/* 0x0015b650  Meta (file inferred: Effects.cpp): delete all glow sprites */
void FUN_0015b650(void)
{
	unsigned int node;
	int prev;

	node = MA_NEXT(MA_GLOWS, NULL);
	while (node != 0) {
		prev = CLList_Remove((int *)MA_GLOWS, (int *)node);
		MA_DELETE(node);
		node = MA_NEXT(MA_GLOWS, prev);
	}
}

/* 0x0015b690  Meta (file inferred: Effects.cpp): a sliding-texture model's update
   (fastcall): scroll the texture while on, wrapping u and v into [0, 1) */
void FUN_0015b690(int self)
{
	if (MA_I32(self, 0x1c) == 0)
		return;
	MA_F32(self, 0x10) = DAT_0028fda8 * MA_F32(self, 8) + MA_F32(self, 0x10);
	MA_F32(self, 0x14) = DAT_0028fda8 * MA_F32(self, 0xc) + MA_F32(self, 0x14);
	if (!(0.0f <= MA_F32(self, 0x10)))             /* 0x002536e0 */
		MA_F32(self, 0x10) = MA_F32(self, 0x10) + 1.0f; /* 0x00253850 */
	else if (!(MA_F32(self, 0x10) < 1.0f))
		MA_F32(self, 0x10) = MA_F32(self, 0x10) - 1.0f;
	if (!(0.0f <= MA_F32(self, 0x14)))
		MA_F32(self, 0x14) = MA_F32(self, 0x14) + 1.0f;
	else if (1.0f <= MA_F32(self, 0x14))
		MA_F32(self, 0x14) = MA_F32(self, 0x14) - 1.0f;
}

/* 0x0015b730  Meta (file inferred: Effects.cpp): update the sliding-texture models */
void FUN_0015b730(void)
{
	unsigned int node;

	for (node = MA_NEXT(MA_SLIDERS, NULL); node != 0; node = MA_NEXT(MA_SLIDERS, node))
		FUN_0015b690((int)node);
}

/* 0x0015b760  Meta (file inferred: Effects.cpp): switch the sliding-texture models with
   `tag` on or off (puzzle action) */
void FUN_0015b760(int tag, undefined4 on)
{
	unsigned int node;

	for (node = MA_NEXT(MA_SLIDERS, NULL); node != 0; node = MA_NEXT(MA_SLIDERS, node)) {
		if (MA_I32(node, 0x18) == tag)
			MA_U32(node, 0x1c) = on;
	}
}

/* 0x0015b7a0  Meta/Effects.cpp: create the goal effects: the flare actors and the four
   particle systems (helixtwist.bmp rings, sparkburst.bmp particles) */
void FUN_0015b7a0(void)
{
	struct MaPSParams params;
	int *block;
	struct bkResource *helix;
	struct bkResource *spark;
	int i;

	DAT_0030841c = bLoadResource(NULL, "goalsfx.obe", 1, 0xdefa);
	if (DAT_0030841c == NULL)
		bkPrintf("ERROR can't find goalsfx.obe");
	for (i = 0; i < 4; i++) {
		DAT_00308370[i].inst = baInitActorInstance(NULL, (struct BActor *)DAT_0030841c, 1,
			MA_EFFECTS_CPP, 0x3d);
		DAT_00308370[i].anim = (struct BAnim *)baFindSubObject(DAT_00308370[i].inst, "flare");
		baFlushAnimations(DAT_00308370[i].inst, MA_PTR(DAT_00308370[i].inst, 0x134), 1, 0);
		DAT_00308370[i].active = 0;
		if (DAT_00308370[i].inst == NULL)
			bkPrintf("goalfxactor instance fail!\n");
		if (DAT_00308370[i].anim == NULL)
			bkPrintf("goalfxactor anim segment missing!\n");
	}
	DAT_00308434 = 0;

	memset(&params, 0, sizeof params); /* (bytes 2-3 are not set by the original) */
	params.active = 0;
	params.unknown01 = 0;
	params.angleSpeed = 0.0698131695f;   /* 0x3d8efa35, 4 degrees */
	params.riseAccel = 0.3f;             /* 0x3e99999a */
	params.gravity = -0.01f;             /* 0xbc23d70a */
	params.riseSpeed = 16.0f;
	params.speed = 5.0f;
	params.drag = 0.99f;                 /* 0x3f7d70a4 */
	params.radiusSpeed = 2.0f;
	params.radius = 250.0f;
	params.numRings = 6;
	params.ringLength = 30;
	params.maxEmit = 10;
	params.lifeMin = 30;
	params.lifeRange = 30;
	params.pointSize = 40.0f;
	params.emitTime = 100.0f;

	/* new CParticleSystem[4] (count word in front of the array) */
	block = (int *)MA_NEW(4 + 4 * MA_PS_SIZE, MA_EFFECTS_CPP, 0x56, 0x1001);
	if (block == NULL) {
		DAT_00308420 = NULL;
	} else {
		block[0] = 4;
		DAT_00308420 = block + 1;
		for (i = 0; i < 4; i++)
			CParticleSystem_Construct(MA_PS(i));
	}

	helix = bLoadResource(NULL, "helixtwist.bmp", 0, 0xdefa);
	bkPrintf(helix == NULL ? "COULDN'T FIND helix texture\n" : "FOUND helix texture\n");
	spark = bLoadResource(NULL, "sparkburst.bmp", 0, 0xdefa);
	bkPrintf(spark == NULL ? "COULDN'T FIND helixTwist part texture\n"
		: "FOUND helixtwist particle texture\n");
	for (i = 0; i < 4; i++) {
		CParticleSystem_Init(MA_PS(i), (struct CPSParams *)&params, 0);
		CParticleSystem_SetTexture((struct CPSHeader *)MA_PS(i), (int)spark);
		CParticleSystem_SetTexture2(MA_PS(i), (int)helix);
	}
}

/* 0x0015b9f0  Meta/Effects.cpp: free the goal effects */
void FUN_0015b9f0(void)
{
	int i;

	if (DAT_00308420 == NULL)
		return;
	for (i = 0; i < 4; i++) {
		MA_VFN(MA_PS(i), 0, void (*)(void *))(MA_PS(i)); /* Release */
		baDeleteActorInstance(DAT_00308370[i].inst);
		DAT_00308370[i].inst = NULL;
		DAT_00308370[i].active = 0;
	}
	if (DAT_00308420 != NULL) {
		if (((int *)DAT_00308420)[-1] != 0) {
			/* delete[]: the vector deleting destructor (flags 3) */
			MA_VFN(DAT_00308420, 0x14, void (*)(void *, int))(DAT_00308420, 3);
			DAT_00308420 = NULL;
			return;
		}
		MA_DELETE((int *)DAT_00308420 - 1);
	}
	DAT_00308420 = NULL;
}

/* 0x0015ba80  Meta/Effects.cpp: update the goal effects. When a player's delay runs
   out, start its burst; its size follows the number of orbs scored (+0x1140, 1-4); 0
   orbs stops the whole update for this frame (as the original) */
void FUN_0015ba80(void)
{
	char *ps;
	struct MaGoalFx *fx;
	int player;
	int character;
	unsigned int r, g, b;
	int i;

	if (DAT_00308420 == NULL)
		return;
	for (i = 0; i < 4; i++) {
		ps = (char *)MA_PS(i);
		fx = &DAT_00308370[i];
		if (DAT_00308424[i] > 0.0f) {
			DAT_00308424[i] = DAT_00308424[i] - DAT_0028fda8;
			if (DAT_00308424[i] <= 0.0f) {
				DAT_00308424[i] = 0.0f;
				player = DAT_003075bc[i];
				switch (MA_I32(player, 0x1140)) {
				case 0:
					return;
				case 1:
					MA_I32(ps, 0x74) = 2;          /* numRings */
					MA_I32(ps, 0x78) = 4;          /* ringLength */
					MA_I32(ps, 0x7c) = 1;          /* maxEmit */
					MA_F32(ps, 0x54) = 0.279252678f; /* angleSpeed 0x3e8efa35, 16 degrees */
					MA_F32(fx->inst, 0xd0) = 0.5f;  /* the flare's colour */
					MA_F32(fx->inst, 0xd4) = 0.5f;
					MA_F32(fx->inst, 0xd8) = 0.5f;
					MA_F32(fx->inst, 0xdc) = 1.0f;
					break;
				case 2:
					MA_I32(ps, 0x74) = 2;
					MA_I32(ps, 0x78) = 0x14;
					MA_I32(ps, 0x7c) = 1;
					MA_F32(ps, 0x54) = 0.279252678f;
					MA_F32(fx->inst, 0xd0) = 0.7f;  /* 0x3f333333 */
					MA_F32(fx->inst, 0xd4) = 0.7f;
					MA_F32(fx->inst, 0xd8) = 0.7f;
					MA_F32(fx->inst, 0xdc) = 1.0f;
					break;
				case 3:
					MA_I32(ps, 0x74) = 2;
					MA_I32(ps, 0x78) = 0x1c;
					MA_I32(ps, 0x7c) = 4;
					MA_F32(ps, 0x54) = 0.279252678f;
					MA_F32(fx->inst, 0xd0) = 0.9f;  /* 0x3f666666 */
					MA_F32(fx->inst, 0xd4) = 0.9f;
					MA_F32(fx->inst, 0xd8) = 0.9f;
					MA_F32(fx->inst, 0xdc) = 1.0f;
					break;
				case 4:
					MA_I32(ps, 0x74) = 6;
					MA_I32(ps, 0x78) = 0x1e;
					MA_I32(ps, 0x7c) = 10;
					MA_F32(ps, 0x54) = 0.34906584f; /* 0x3eb2b8c2, 20 degrees */
					MA_F32(fx->inst, 0xd0) = 1.0f;
					MA_F32(fx->inst, 0xd4) = 1.0f;
					MA_F32(fx->inst, 0xd8) = 1.0f;
					MA_F32(fx->inst, 0xdc) = 1.0f;
					break;
				default:
					MA_I32(ps, 0x74) = 2;
					MA_I32(ps, 0x78) = 4;
					MA_I32(ps, 0x7c) = 1;
					MA_F32(ps, 0x54) = 0.0698131695f; /* 0x3d8efa35 */
					break;
				}
				FUN_00156ba0(player, 1, 1);
				CLSamples_PlayGeneric(2, 0xb4, 0, 0, NULL);
				DAT_00308424[i] = 0.0f;
				CParticleSystem_SetPosition((struct CPSHeader *)ps, DAT_003083d8[i]);
				character = DAT_002baed4[i * 10];
				r = DAT_00290718[character * 0x60];
				g = DAT_00290718[character * 0x60 + 1];
				b = DAT_00290718[character * 0x60 + 2];
				CParticleSystem_Start((struct CParticleSystem *)ps, (short)r, (short)g, (short)b);
				MA_U8(ps, 0x50) = 1; /* p.active */
				Light_NewFlash(DAT_003083d8[i], 5000.0f, 0x50, (int)r, (int)g, (int)b, 0.0f);
				baFlushAnimations(fx->inst, MA_PTR(fx->inst, 0x134), 1, 0);
				baQueueAnimation(fx->inst, MA_PTR(fx->inst, 0x134), fx->anim, 0x400, -1, 0,
					(int (*)())Effects_GoalFlareDone, fx);
				baPrepActorInstance(fx->inst, 1);
				MA_F32(fx->inst, 0xc0) = DAT_003083d8[i][0];
				MA_F32(fx->inst, 0xc4) = DAT_003083d8[i][1];
				MA_F32(fx->inst, 0xc8) = DAT_003083d8[i][2];
				MA_F32(fx->inst, 0xcc) = DAT_003083d8[i][3];
				fx->active = 1;
				fx->r = (int)r;
				fx->b = (int)b;
				fx->g = (int)g;
			}
		}
		if (fx->active != 0) {
			baUpdateAnimations(fx->inst, MA_PTR(fx->inst, 0x134), MA_GOAL_ANIM_SPEED * DAT_0028fda8,
				NULL);
			baPrepActorInstance(fx->inst, 1);
		}
		MA_VFN(ps, 4, void (*)(void *, float))(ps, MA_GOAL_ANIM_SPEED * DAT_0028fda8); /* Update */
	}
}

/* 0x0015be40  Meta/Effects.cpp: start player `player`'s goal effect at `pos` in 20 frames */
void FUN_0015be40(int player, undefined4 *pos)
{
	memcpy(DAT_003083d8[player], pos, 4 * sizeof(float));
	DAT_003083d8[player][3] = 1.0f;
	DAT_00308424[player] = 20.0f;
}

/* 0x0015be90  Meta/Effects.cpp: throw `count` leaping fish ("fish.obe", charlib type 0xf)
   around `pos` (puzzle action) */
void FUN_0015be90(float *pos, int count)
{
	float at[4];
	int spread;
	int fish;
	int *anim;
	int n;
	char *move;

	for (n = count; n > 0; n--) {
		spread = MA_FISH_SPREAD;
		at[0] = ((float)(ma_rand() % (spread * 2 + 1)) + pos[0]) - (float)spread;
		at[1] = ((float)(ma_rand() % (spread * 2 + 1)) + pos[1]) - (float)spread;
		at[2] = ((float)(ma_rand() % (spread * 2 + 1)) + pos[2]) - (float)spread;
		at[3] = 0.0f; /* (not set by the original) */
		fish = (int)Actor_LoadAny("fish.obe", at, 0xf, NULL, 1);
		MA_F32(fish, 0x194) = 0.0f; /* velocity */
		MA_F32(fish, 0x198) = 0.0f;
		MA_I32(fish, 0x220) = 0x11;
		MA_F32(fish, 0x19c) = (float)(ma_rand() % (MA_FISH_LEAP * 2 + 1) - MA_FISH_LEAP);
		MA_F32(fish, 0x1e8) = (float)(ma_rand() % 0x274) * 0.01f; /* 0x0025372c */
		MA_F32(fish, 0x1d8) = MA_F32(fish, 0x1e8);
		move = (char *)bkHeapCalloc(0x204, 0, MA_EFFECTS_CPP, 0x1c4, 0x2001);
		MA_PTR(fish, 0x18c) = move;
		MA_U32(move, 0x9c) = MA_FISH_0A2C;
		MA_U8(MA_PTR(fish, 0x18c), 0x15d) = 1;
		move = (char *)bkHeapCalloc(0x40, 0, MA_EFFECTS_CPP, 0x1c8, 0x2001);
		MA_PTR(fish, 0x188) = move;
		MA_F32(move, 0) = 0.5f;
		MA_U32(MA_PTR(fish, 0x188), 0x24) = MA_FISH_0A38;
		MA_F32(MA_PTR(fish, 0x188), 0x28) = 0.95f; /* 0x3f733333 */
		MA_U32(fish, 0x258) = MA_FISH_0A30;
		MA_U32(fish, 0x25c) = MA_FISH_0A34;
		MA_F32(fish, 0x260) = 10.0f;
		MA_I32(fish, 0x224) = 0x100;
		MA_F32(fish, 0xe0) = (float)(ma_rand() % 100 + 0x96) * 0.005f; /* 0x002632d8; scale */
		MA_F32(fish, 0xe4) = MA_F32(fish, 0xe0);
		MA_F32(fish, 0xe8) = MA_F32(fish, 0xe0);
		anim = (int *)baFindSubObject((struct BActorInstance *)(fish + 0x10), "flop");
		if (anim != NULL)
			baQueueAnimation((struct BActorInstance *)(fish + 0x10), MA_PTR(fish, 0x144),
				(struct BAnim *)anim, MA_FISH_ANIM_SPEED, ma_rand() % (anim[2] - anim[1]) + anim[1],
				1, NULL, NULL);
		MA_F32(fish, 0x23c) = (float)(ma_rand() % 2000) * 0.001f + 0.5f; /* 0x0025767c, 0x002536d0 */
	}
}

/* 0x0015c1c0  Meta/Effects.cpp: a fish in one of the water areas (meta_b, under the
   surface +0x14; area +0x22 == 0: rectangle +0x04..+0x10, else circle at +0x04/+0x08 with
   the squared radius at +0x0c) splashes it with a third of its speed. Returns 1 if so */
undefined4 FUN_0015c1c0(int fish)
{
	int i;
	int water;
	float dx, dz;
	int inside;
	double speed;

	for (i = 0; i < DAT_003090b4; i++) {
		water = (int)DAT_003090b8[i];
		if (!(MA_F32(fish, 0xd4) < MA_F32(water, 0x14)))
			continue;
		if (MA_I16(water, 0x22) == 0) {
			if (MA_F32(fish, 0xd0) < MA_F32(water, 4) || MA_F32(fish, 0xd8) < MA_F32(water, 8)
				|| MA_F32(water, 0xc) < MA_F32(fish, 0xd0))
				continue;
			inside = MA_F32(fish, 0xd8) <= MA_F32(water, 0x10);
		} else {
			dx = MA_F32(fish, 0xd0) - MA_F32(water, 4);
			dz = MA_F32(fish, 0xd8) - MA_F32(water, 8);
			inside = dx * dx + dz * dz <= MA_F32(water, 0xc);
		}
		if (inside) {
			speed = sqrt((double)MA_F32(fish, 0x194) * MA_F32(fish, 0x194)
				+ (double)MA_F32(fish, 0x198) * MA_F32(fish, 0x198)
				+ (double)MA_F32(fish, 0x19c) * MA_F32(fish, 0x19c));
			CMetaWater_Splash((struct CMetaWater *)water, MA_FP(fish, 0xd0),
				(float)(speed * 0.333333343f)); /* 0x0025df94 */
			return 1;
		}
	}
	return 0;
}

/* 0x0015c2c0  Meta/Effects.cpp: a leaping fish's update: a fish at rest sinks; in the
   water it slows down (0.8 per frame) and, at most every 30 frames, splashes
   (gen_splash_01a.wav). Returns the fish */
int FUN_0015c2c0(int fish)
{
	if (MA_F32(fish, 0x198) == 0.0f)
		MA_F32(fish, 0xd8) = MA_F32(fish, 0xd8)
			- (float)(ma_rand() % MA_FISH_SINK + MA_FISH_SINK) * DAT_0028fda8;
	if (FUN_0015c1c0(fish) != 0) {
		/* pow(0.8f, frame factor): the double at 0x0026f6f0 is 0.8f widened */
		MA_F32(fish, 0x198) = (float)(pow((double)0.8f, (double)DAT_0028fda8)
			* (double)MA_F32(fish, 0x198));
		if (MA_F32(fish, 0x258) > 30.0f) { /* 0x002553a8 */
			MA_F32(fish, 0x258) = 30.0f;
			CLSamples_Play((int)bLoadResource(NULL, "gen_splash_01a.wav", 2, 0xdefa), 0x5a,
				ma_rand() % MA_SPLASH_PITCH_RANGE + MA_SPLASH_PITCH, 0, NULL);
		}
	}
	return fish;
}

/* 0x0015c3c0  Meta/Effects.cpp: update the leaping fish (charlib actors of type 0xf) */
void FUN_0015c3c0(void)
{
	unsigned int actor;

	for (actor = MA_NEXT(DAT_002ea488, NULL); actor != 0; actor = MA_NEXT(DAT_002ea488, actor)) {
		if (MA_I32(actor, 0x21c) == 0xf)
			actor = (unsigned int)FUN_0015c2c0((int)actor);
	}
}

/* 0x0015c400  Meta/Effects.cpp: add a glow sprite (camera-facing; `offset` pulls it
   toward the camera) */
int FUN_0015c400(float *pos, float width, float height, char *texture, int r, int g, int b,
	int a, float offset)
{
	char *sprite;

	sprite = (char *)MA_NEW(0x5c, MA_EFFECTS_CPP, 0x26a, 0x2001);
	memcpy(sprite + 8, pos, 4 * sizeof(float));
	MA_F32(sprite, 0x14) = 1.0f;
	MA_F32(sprite, 0x28) = width;
	MA_F32(sprite, 0x2c) = height;
	MA_PTR(sprite, 0x50) = bLoadResource(NULL, texture, 0, 0xdefa);
	MA_I32(sprite, 0x44) = g;
	MA_I32(sprite, 0x40) = r;
	MA_F32(sprite, 0x30) = offset;
	MA_I32(sprite, 0x48) = b;
	MA_I32(sprite, 0x4c) = a;
	MA_I32(sprite, 0x58) = 0;
	MA_I32(sprite, 0x54) = 0;
	MA_F32(sprite, 0x34) = 0.0f;
	CLList_Insert((int *)MA_GLOWS, (int *)sprite, 0);
	return (int)sprite;
}

/* 0x0015c4b0  Meta/Effects.cpp: draw a glow sprite, pulled toward the camera (the
   original passes the sprite in ESI) */
void FUN_0015c4b0(int sprite)
{
	float dir[4];
	float at[4];
	float pull;

	dir[0] = MA_F32(DAT_003055d4, 0x534) - MA_F32(sprite, 8);
	dir[1] = MA_F32(DAT_003055d4, 0x538) - MA_F32(sprite, 0xc);
	dir[2] = MA_F32(DAT_003055d4, 0x53c) - MA_F32(sprite, 0x10);
	bmVectorNormalise(dir, dir);
	pull = MA_F32(sprite, 0x30);
	at[0] = dir[0] * pull + MA_F32(sprite, 8);
	at[1] = dir[1] * pull + MA_F32(sprite, 0xc);
	dir[3] = 1.0f;
	at[3] = 1.0f;
	at[2] = dir[2] * pull + MA_F32(sprite, 0x10);
	bdDrawSprite3D(at, MA_F32(sprite, 0x28), MA_F32(sprite, 0x2c), MA_F32(sprite, 0x34),
		(struct bdTexture *)MA_PTR(sprite, 0x50), 0, 0, MA_I32(sprite, 0x40), MA_I32(sprite, 0x44),
		MA_I32(sprite, 0x48), MA_I32(sprite, 0x4c), NULL, 0.0f);
}

/* 0x0015c580  Meta/Effects.cpp: draw the glow sprites; a sprite on an actor node
   follows it (node position + offset turned by the node's rotation) */
void FUN_0015c580(void)
{
	unsigned int sprite;
	float offset[4];
	float rotation[4];
	float node_pos[4];

	bdSetProjectionMode(1);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	for (sprite = MA_NEXT(MA_GLOWS, NULL); sprite != 0; sprite = MA_NEXT(MA_GLOWS, sprite)) {
		if (MA_I32(sprite, 0x54) != 0) {
			baGetNodePosition((struct BActorInstance *)(MA_I32(sprite, 0x58) + 0x10),
				(struct BActorNode *)MA_PTR(sprite, 0x54), node_pos);
			offset[0] = MA_F32(sprite, 0x18);
			offset[1] = MA_F32(sprite, 0x1c);
			offset[2] = MA_F32(sprite, 0x20);
			offset[3] = MA_F32(sprite, 0x24);
			bmMatrixToQuaternion(rotation, (const float *)(MA_I32(sprite, 0x54) + 0x80));
			rotation[3] = -rotation[3];
			CLVec3_Rotate(offset, offset, rotation);
			MA_F32(sprite, 8) = node_pos[0] + offset[0];
			MA_F32(sprite, 0xc) = node_pos[1] + offset[1];
			MA_F32(sprite, 0x10) = node_pos[2] + offset[2];
		}
		if (MA_I32(sprite, 0x3c) != 0)
			FUN_0015c4b0((int)sprite);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetProjectionMode(1);
}

/* 0x0015c6a0  Meta/Effects.cpp: draw a sliding-texture model (fastcall) with the texture
   translated by (u, v) */
void FUN_0015c6a0(int self)
{
	float matrix[16];

	if (MA_I32(self, 0x1c) == 0)
		return;
	baPrepActorInstance((struct BActorInstance *)(MA_I32(self, 0x30) + 0x10), 1);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(7, 3, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 0, 0);
	memcpy(matrix, DAT_002855f0, sizeof matrix);
	matrix[8] = MA_F32(self, 0x10); /* _31 */
	matrix[9] = MA_F32(self, 0x14); /* _32 */
	bdSetRenderState(2, 3, 0);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)9, 2); /* TEXTURETRANSFORMFLAGS: COUNT2 */
	D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)2, (const D3DMATRIX *)matrix); /* TEXTURE0 */
	baDrawActorInstance((struct BActorInstance *)(MA_I32(self, 0x30) + 0x10), 7);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)9, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, MA_U32(DAT_003055d0, 0x48), 0);
}

/* 0x0015c7a0  Meta/Effects.cpp: add a sliding-texture model (level script
   ADD_SLIDDING_TEXTURE_MODEL); the actor leaves the charlib list, the effect draws it */
void FUN_0015c7a0(char *model, float *pos, float speed_u, float speed_v, int tag)
{
	char *self;
	void *actor;

	self = (char *)MA_NEW(0x34, MA_EFFECTS_CPP, 0x348, 0x1001);
	actor = Actor_Create(bLoadResource(NULL, model, 1, 0), pos, NULL, 4, 0, 1);
	MA_PTR(self, 0x30) = actor;
	CLList_Remove((int *)DAT_002ea488, (int *)actor);
	memcpy(self + 0x20, pos, 4 * sizeof(float));
	MA_I32(self, 0x18) = tag;
	MA_F32(self, 0x14) = 0.0f;
	MA_F32(self, 0x10) = 0.0f;
	MA_F32(self, 8) = speed_u;
	MA_F32(self, 0xc) = speed_v;
	MA_I32(self, 0x1c) = 1;
	CLList_Insert((int *)MA_SLIDERS, (int *)self, 0);
}

/* 0x0015c840  Meta (file inferred: Effects.cpp): delete the sliding-texture models */
void FUN_0015c840(void)
{
	unsigned int node;
	int prev;

	node = MA_NEXT(MA_SLIDERS, NULL);
	while (node != 0) {
		prev = CLList_Remove((int *)MA_SLIDERS, (int *)node);
		Actor_Delete(MA_PTR(node, 0x30));
		MA_DELETE(node);
		node = MA_NEXT(MA_SLIDERS, prev);
	}
}

/* 0x0015c890  Meta (file inferred: Effects.cpp): draw the sliding-texture models */
void FUN_0015c890(void)
{
	unsigned int node;

	for (node = MA_NEXT(MA_SLIDERS, NULL); node != 0; node = MA_NEXT(MA_SLIDERS, node))
		FUN_0015c6a0((int)node);
}
