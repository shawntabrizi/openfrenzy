/*
FoxHunt/CFHunt_GenericGoal.c  <-  C:\BlitzParty\Game\Src\FoxHunt\CFHunt_GenericGoal.cpp
(module foxhunt)

The goal markers of variations 1-2 (CFHunt_GenericGoal, 0x24 bytes, owned by the game mode at
+0x27c): a list of goals, each a grey goal model with an "active" animation and a helix
particle effect that is coloured by the player who scores.

The manager:                           A goal (0x4c bytes):
  +0x00 the base model's +0x5c           +0x00 active
  +0x04 the goal list                    +0x14 position (vec4), +0x24 size
  +0x08 the grey goal model              +0x2c user value, +0x30 colour (-1: none)
  +0x0c the goal base model              +0x38 animation time
  +0x10 the base actor instance          +0x3c actor instance, +0x40 its "active" node
  +0x14 (unused texture slot)            +0x44 the helix particle system (CParticleSystem)
  +0x18 activegoalanim.gif               +0x48 next goal
  +0x1c helixtwist.bmp
  +0x20 particletwo00.bmp
*/

#include "foxhunt.h"

static const char s_GoalFile[] = "C:\\BlitzParty\\Game\\Src\\FoxHunt\\CFHunt_GenericGoal.cpp";

/* the particle parameters of classlib_fx (CPSParams, 0x40 bytes) */
typedef struct FHPSParams {
	unsigned char active;
	unsigned char unknown51;
	short pad52;
	float angleSpeed, riseAccel, gravity, riseSpeed, speed, drag, radiusSpeed, radius;
	int numRings, ringLength, maxEmit, lifeMin, lifeRange;
	float pointSize, emitTime;
} FHPSParams;
OF_ASSERT_SIZE(FHPSParams, 0x40);

#define GOAL_ACTIVE(g) FH_AI(g, 0x00)
#define GOAL_INST(g) FH_AP(g, 0x3c)
#define GOAL_FX(g) FH_AP(g, 0x44)
#define GOAL_NEXT(g) FH_AP(g, 0x48)

/* 0x00133530  CFHunt_GenericGoal.cpp: CFHunt_GenericGoal::CFHunt_GenericGoal: loads the
goal models and textures and makes the base actor instance */
void *CFHunt_GenericGoal_Ctor(void *self)
{
	char *inst;

	memset(self, 0, 0x24);
	FH_AP(self, 0x8) = (char *)bLoadResource(NULL, "genericgoal\\models\\greygenericgoal.obe", 1, 0xdefa);
	FH_AP(self, 0xc) = (char *)bLoadResource(NULL, "genericgoal\\models\\goalbase.obe", 1, 0xdefa);
	if (FH_AP(self, 0xc) == NULL) {
		bkPrintf("%s(%d) : Constructor - Could not find generic goal base actor\n", s_GoalFile, 0x24);
	} else {
		inst = (char *)baInitActorInstance(NULL, (struct BActor *)FH_AP(self, 0xc), 1, s_GoalFile, 0x18);
		FH_AP(self, 0x10) = inst;
		if (inst != NULL) {
			baPrepActorInstance((struct BActorInstance *)inst, 1);
			FH_AI(self, 0x0) = FH_AI(FH_AP(self, 0xc), 0x5c);
		} else {
			bkPrintf("%s(%d) : Constructor - Could not create generic goal base actor instance\n", s_GoalFile, 0x1f);
		}
	}
	FH_AI(self, 0x14) = 0;
	FH_AP(self, 0x18) = (char *)bLoadResource(NULL, "genericgoal\\textures\\activegoalanim.gif", 0, 0xdefa);
	FH_AP(self, 0x1c) = (char *)bLoadResource(NULL, "genericgoal\\textures\\helixtwist.bmp", 0, 0xdefa);
	FH_AP(self, 0x20) = (char *)bLoadResource(NULL, "texturesl\\particletwo00.bmp", 0, 0xdefa);
	return self;
}

/* 0x00133620  (not in the function table; called by the game mode's exit):
CFHunt_GenericGoal::~CFHunt_GenericGoal: frees every goal (its actor instance and particle
system) and the base actor instance */
void FUN_00133620(void *self)
{
	char *goal;
	char *next;
	char *fx;

	if (FH_AP(self, 0x4) != NULL) {
		do {
			goal = FH_AP(self, 0x4);
			next = GOAL_NEXT(goal);
			if (GOAL_INST(goal) != NULL)
				baDeleteActorInstance((struct BActorInstance *)GOAL_INST(goal));
			fx = GOAL_FX(goal);
			if (fx != NULL) {
				FH_VFN(fx, 0x0, void (*)(void *))(fx);
				fx = GOAL_FX(goal);
				if (fx != NULL)
					FH_VFN(fx, 0x14, void (*)(void *, int))(fx, 1);
			}
			bkHeapFree(FH_AP(self, 0x4));
			FH_AP(self, 0x4) = next;
		} while (next != NULL);
	}
	FH_AP(self, 0x4) = NULL;
	if (FH_AP(self, 0x10) != NULL)
		baDeleteActorInstance((struct BActorInstance *)FH_AP(self, 0x10));
}

/* 0x00133690  CFHunt_GenericGoal.cpp: switches a goal on (plays its "active" animation) or
off (flushes the animations) */
void CFHunt_GenericGoal_SetActive(void *self, char *goal, int on)
{
	char *inst;

	(void)self;
	if (goal == NULL)
		return;
	if (on != 0) {
		if (GOAL_ACTIVE(goal) != 0)
			return;
		if (FH_AP(goal, 0x40) != NULL) {
			inst = GOAL_INST(goal);
			baQueueAnimation((struct BActorInstance *)inst, (struct BActorNode *)FH_AP(inst, 0x134),
				(struct BAnim *)FH_AP(goal, 0x40), 0x400, -1, 1, NULL, NULL);
		}
		GOAL_ACTIVE(goal) = 1;
	} else if (GOAL_ACTIVE(goal) == 1) {
		inst = GOAL_INST(goal);
		baFlushAnimations(inst, (struct BActorNode *)FH_AP(inst, 0x134), 1, 0);
		GOAL_ACTIVE(goal) = 0;
	}
}

/* 0x00133700  CFHunt_GenericGoal.cpp: sets a goal's colour (0x00rrggbb; -1 for none) */
void CFHunt_GenericGoal_SetColour(void *self, char *goal, int colour)
{
	(void)self;
	if (goal != NULL)
		FH_AI(goal, 0x30) = colour;
}

/* 0x00133720  CFHunt_GenericGoal.cpp: advances the active goals' animations and every
goal's particle system by `dt` frames */
void CFHunt_GenericGoal_Update(void *self, float dt)
{
	char *goal;
	char *inst;
	char *fx;

	for (goal = FH_AP(self, 0x4); goal != NULL; goal = GOAL_NEXT(goal)) {
		if (GOAL_ACTIVE(goal) == 1) {
			inst = GOAL_INST(goal);
			FH_AF(goal, 0x38) = dt * (1.0f / 60.0f) + FH_AF(goal, 0x38);
			if (inst != NULL)
				baUpdateAnimations((struct BActorInstance *)inst, (struct BActorNode *)FH_AP(inst, 0x134), dt, NULL);
		}
		fx = GOAL_FX(goal);
		if (fx != NULL)
			FH_VFN(fx, 0x4, void (*)(void *, float))(fx, dt);
	}
}

/* 0x00133780  CFHunt_GenericGoal.cpp: starts a goal's helix effect in colour (r, g, b);
`level` 0-4 sets how many particles it emits and how fast and high the helix turns */
void CFHunt_GenericGoal_StartEffect(void *self, char *goal, int r, int g, int b, unsigned int level)
{
	char *fx;

	(void)self;
	if (goal == NULL || GOAL_FX(goal) == NULL)
		return;
	CParticleSystem_SetPosition((struct CPSHeader *)GOAL_FX(goal), FH_AV(goal, 0x14));
	FH_AF(GOAL_FX(goal), 0x60) = FH_AF(goal, 0x24) * 5.5555553f;
	FH_AF(GOAL_FX(goal), 0x6c) = FH_AF(goal, 0x24) * 1.1111112f;
	FH_AF(GOAL_FX(goal), 0x70) = FH_AF(goal, 0x24) * 111.111115f;
	fx = GOAL_FX(goal);
	switch (level) {
	case 0:
		FH_AI(fx, 0x7c) = 4;
		FH_AU(fx, 0x54) = 0x3d7a35ddu;   /* 0.0611 */
		FH_AF(fx, 0x60) = FH_AF(fx, 0x60) * 1.6f;
		break;
	case 1:
		FH_AI(fx, 0x7c) = 6;
		FH_AU(fx, 0x54) = 0x3db2b8c2u;   /* 0.0873 */
		FH_AF(fx, 0x60) = FH_AF(fx, 0x60) * 1.4f;
		break;
	case 2:
		FH_AI(fx, 0x7c) = 10;
		FH_AU(fx, 0x54) = 0x3e20d97cu;   /* 0.157 */
		break;
	case 3:
		FH_AI(fx, 0x7c) = 0x14;
		FH_AU(fx, 0x54) = 0x3ea0d97cu;   /* 0.314 */
		FH_AF(fx, 0x60) = FH_AF(fx, 0x60) * 0.5f;
		break;
	case 4:
		FH_AI(fx, 0x7c) = 0x28;
		FH_AU(fx, 0x54) = 0x3ec49809u;   /* 0.384 */
		FH_AF(fx, 0x60) = FH_AF(fx, 0x60) * 0.4f;
		break;
	default:
		break;
	}
	CParticleSystem_Start((struct CParticleSystem *)GOAL_FX(goal), (short)r, (short)g, (short)b);
	FH_AB(GOAL_FX(goal), 0x50) = 1;
}

/* 0x001338c0  CFHunt_GenericGoal.cpp: adds a goal at `pos` (vec4, raised by 4) of `size`,
with its "active" animation and its helix particle system. Returns the goal (NULL without
the base model). */
char *CFHunt_GenericGoal_AddGoal(void *self, float *pos, float size, int user)
{
	char *goal;
	char *inst;
	char *fx;
	FHPSParams params;

	if (FH_AP(self, 0xc) == NULL)
		return NULL;
	goal = (char *)bkHeapAlloc(0x4c, s_GoalFile, 0x5a, 0x2001);
	memset(goal, 0, 0x4c);
	FH_AI(goal, 0x2c) = user;
	FH_AF(goal, 0x14) = pos[0];
	FH_AF(goal, 0x18) = pos[1];
	FH_AF(goal, 0x1c) = pos[2];
	FH_AF(goal, 0x20) = pos[3];
	FH_AF(goal, 0x24) = size;
	GOAL_ACTIVE(goal) = 0;
	FH_AI(goal, 0x38) = 0;
	FH_AI(goal, 0x30) = -1;
	FH_AF(goal, 0x18) = FH_AF(goal, 0x18) + 4.0f;
	if (FH_AP(self, 0x8) == NULL) {
		bkPrintf("%s(%d) : AddGoal - Could not find activation actor\n", s_GoalFile, 0x76);
	} else {
		inst = (char *)baInitActorInstance(NULL, (struct BActor *)FH_AP(self, 0x8), 2, s_GoalFile, 0x69);
		GOAL_INST(goal) = inst;
		if (inst != NULL) {
			baPrepActorInstance((struct BActorInstance *)inst, 1);
			FH_AP(goal, 0x40) = (char *)baFindSubObject((struct BActorInstance *)GOAL_INST(goal), "active");
			baFlushAnimations(GOAL_INST(goal), (struct BActorNode *)FH_AP(GOAL_INST(goal), 0x134), 1, 0);
		} else {
			bkPrintf("%s(%d) : AddGoal - Could not create activation actor instance\n", s_GoalFile, 0x71);
		}
	}
	fx = (char *)bkHeapAlloc(0xa8, s_GoalFile, 0x7a, 0x1001);
	if (fx != NULL) {
		CParticleSystem_Construct((struct CParticleSystem *)fx);
	}
	GOAL_FX(goal) = fx;
	if (fx == NULL) {
		bkPrintf("%s(%d) : AddGoal - Could not create helixTwist particle effect\n", s_GoalFile, 0x9b);
	} else {
		params.active = 0;
		params.unknown51 = 0;
		params.pad52 = 0;
		params.angleSpeed = fh_bitsf(0x3e20d97cu);   /* 0.157 */
		params.riseAccel = 0.3f;
		params.gravity = -0.01f;
		params.riseSpeed = 20.0f;
		params.speed = 5.0f;
		params.drag = 0.99f;
		params.radiusSpeed = 2.0f;
		params.radius = 200.0f;
		params.numRings = 4;
		params.ringLength = 0x14;
		params.maxEmit = 10;
		params.lifeMin = 0x1e;
		params.lifeRange = 0x1e;
		params.pointSize = 80.0f;
		params.emitTime = 250.0f;
		CParticleSystem_Init((struct CParticleSystem *)fx, (struct CPSParams *)&params, 600);
		CParticleSystem_SetTexture2((struct CParticleSystem *)GOAL_FX(goal), FH_AI(self, 0x1c));
		CParticleSystem_SetTexture((struct CPSHeader *)GOAL_FX(goal), FH_AI(self, 0x20));
	}
	GOAL_NEXT(goal) = FH_AP(self, 0x4);
	FH_AP(self, 0x4) = goal;
	return goal;
}

/* 0x00133b00  (gap after CFHunt_GenericGoal.cpp): draws the goals. Pass 0 draws the base
model at every goal; pass 1 draws the active goals' models in their colour (grey when none)
and then every goal's particle system. */
void CFHunt_GenericGoal_Draw(void *self, int pass)
{
	char *goal;
	char *inst;
	int ambR, ambG, ambB;
	unsigned int colour, r, g, b;

	bdPushRenderState(3, 1, 0);
	bdPushRenderState(4, 0, 0);
	bdPushRenderState(7, 3, 0);
	bdPushRenderState(0, 0, 0);
	bdPushRenderState(0xd, 0, 1);
	ambR = DAT_00284b10;
	ambG = DAT_00284b14;
	ambB = DAT_00284b18;
	for (goal = FH_AP(self, 0x4); goal != NULL; goal = GOAL_NEXT(goal)) {
		bdSetWorldIdentity();
		if (FH_AP(self, 0x14) != NULL)
			bdTextureSetTime((struct bdTexture *)FH_AP(self, 0x14), FH_FTOL(FH_AF(goal, 0x38) * 1000.0f));
		if (FH_AP(self, 0x18) != NULL)
			bdTextureSetTime((struct bdTexture *)FH_AP(self, 0x18), FH_FTOL(FH_AF(goal, 0x38) * 1000.0f));
		if (pass == 0) {
			inst = FH_AP(self, 0x10);
			FH_AI(inst, 0xc0) = FH_AI(goal, 0x14);
			FH_AI(inst, 0xc4) = FH_AI(goal, 0x18);
			FH_AI(inst, 0xc8) = FH_AI(goal, 0x1c);
			FH_AI(inst, 0xcc) = FH_AI(goal, 0x20);
			FH_AI(FH_AP(self, 0x10), 0xd8) = FH_AI(goal, 0x24);
			FH_AI(FH_AP(self, 0x10), 0xd4) = FH_AI(FH_AP(self, 0x10), 0xd8);
			FH_AI(FH_AP(self, 0x10), 0xd0) = FH_AI(FH_AP(self, 0x10), 0xd4);
			baPrepActorInstance((struct BActorInstance *)FH_AP(self, 0x10), 1);
			bdPushRenderState(6, 0, 0);
			baDrawActorInstance((struct BActorInstance *)FH_AP(self, 0x10), 1);
			bdPopRenderState(6);
		} else if (GOAL_ACTIVE(goal) == 1 && GOAL_INST(goal) != NULL) {
			colour = FH_AU(goal, 0x30);
			if (colour == 0xffffffffu) {
				g = 0x7f;
				r = 0;
				b = 0x7f;
			} else {
				g = colour >> 8 & 0xff;
				b = colour & 0xff;
				r = colour >> 0x10 & 0xff;
			}
			bdPushRenderState(6, 1, 0);
			bdPushRenderState(10, 1, 0);
			bdPushRenderState(5, 0, 0);
			inst = GOAL_INST(goal);
			FH_AI(inst, 0xc0) = FH_AI(goal, 0x14);
			FH_AI(inst, 0xc4) = FH_AI(goal, 0x18);
			FH_AI(inst, 0xc8) = FH_AI(goal, 0x1c);
			FH_AI(inst, 0xcc) = FH_AI(goal, 0x20);
			FH_AI(GOAL_INST(goal), 0xd8) = FH_AI(goal, 0x24);
			FH_AI(GOAL_INST(goal), 0xd4) = FH_AI(GOAL_INST(goal), 0xd8);
			FH_AI(GOAL_INST(goal), 0xd0) = FH_AI(GOAL_INST(goal), 0xd4);
			baPrepActorInstance((struct BActorInstance *)GOAL_INST(goal), 1);
			baSetAmbientLight((int)r, (int)g, (int)b);
			baDrawActorInstance((struct BActorInstance *)GOAL_INST(goal), 7);
			baSetAmbientLight(ambR, ambG, ambB);
			bdPopRenderState(6);
			bdPopRenderState(10);
			bdPopRenderState(5);
		}
	}
	bdPopRenderState(0xd);
	bdPopRenderState(0);
	bdPopRenderState(7);
	bdPopRenderState(4);
	bdPopRenderState(3);
	bdPushRenderState(3, 0, 0);
	for (goal = FH_AP(self, 0x4); goal != NULL; goal = GOAL_NEXT(goal)) {
		if (GOAL_FX(goal) != NULL && pass != 0)
			FH_VFN(GOAL_FX(goal), 0x8, void (*)(void *))(GOAL_FX(goal));
	}
	bdPopRenderState(3);
}
