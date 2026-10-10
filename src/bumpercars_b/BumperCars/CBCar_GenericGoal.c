/*
BumperCars/CBCar_GenericGoal.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_GenericGoal.cpp

The generic goal: a goal base actor and a list of goals, each with an activation actor
("active" animation) and a ring of helix particle systems (one per player). TankedUp uses it
too. 0x000a8030 and 0x000a8250 are tagged with CBCar_GenericGoal.cpp, 0x000a8170 and
0x000a8200 are bracketed; 0x000a8020 is a gap function before them (file inferred).
The other methods of the class (0x000a8520-) belong to another module.
*/

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"
#include "classlib_fx.h"

#define BCB_SRC_GOAL "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_GenericGoal.cpp"

/* The generic goal is a CParticleEffect of classlib_fx (whose Update 0x000a85f0 and Draw
0x000a86e0 run it) with two more fields; each goal is a CParticleEmitter:
  +0x0c  the activation actor "greygenericgoal.obe"   +0x10  the base actor "goalbase.obe"
  +0x14  the base actor's instance                     +0x18  "greygoalanim.gif"
  +0x1c  "activegoalanim.gif"                          +0x20  "helixtwist.bmp" (ring texture)
  +0x24  "sparkburst.bmp" (particle texture) */
#define BCB_GOAL_ACTIVE_ACTOR(e) BCB_AT(e, 0x0c, struct BActor *)
#define BCB_GOAL_BASE_ACTOR(e) BCB_AT(e, 0x10, struct BActor *)
#define BCB_GOAL_HELIX_TEXTURE(e) BCB_AT(e, 0x20, int)
#define BCB_GOAL_SPARK_TEXTURE(e) BCB_AT(e, 0x24, int)

/* 0x000a8020  (gap; file inferred)
Sets the texture of a goal (the waypoints use it). */
void BCarGoal_SetTexture(CParticleEmitter *goal, int texture)
{
	goal->texture = texture;
}

/* 0x000a8030  BumperCars/CBCar_GenericGoal.cpp
Constructor: loads the actors and textures; `count` helix systems per goal. */
CParticleEffect *BCarGenericGoal_Construct(CParticleEffect *self, int count)
{
	struct BActorInstance *inst;

	memset(self, 0, 0x28);
	self->emitters = NULL;
	BCB_GOAL_ACTIVE_ACTOR(self) = (struct BActor *)bLoadResource(NULL,
		"genericgoal\\models\\greygenericgoal.obe", 1, 0xdefa);
	BCB_GOAL_BASE_ACTOR(self) = (struct BActor *)bLoadResource(NULL,
		"genericgoal\\models\\goalbase.obe", 1, 0xdefa);
	if (BCB_GOAL_BASE_ACTOR(self) == NULL)
		bkPrintf("%s(%d) : Constructor - Could not find generic goal base actor\n", BCB_SRC_GOAL, 0x5a);
	else
	{
		inst = baInitActorInstance(NULL, BCB_GOAL_BASE_ACTOR(self), 1, BCB_SRC_GOAL, 0x4d);
		self->actor = (int)inst;
		if (inst == NULL)
			bkPrintf("%s(%d) : Constructor - Could not create generic goal base actor instance\n",
				BCB_SRC_GOAL, 0x55);
		else
		{
			inst->rot[0] = DAT_00285634[0];
			inst->rot[1] = DAT_00285634[1];
			inst->rot[2] = DAT_00285634[2];
			inst->rot[3] = DAT_00285634[3];
			baPrepActorInstance((struct BActorInstance *)self->actor, 1);
			self->scale = BCB_F32(BCB_GOAL_BASE_ACTOR(self), 0x5c);
		}
	}
	self->texture1 = (int)bLoadResource(NULL, "genericgoal\\textures\\greygoalanim.gif", 0, 0xdefa);
	self->texture2 = (int)bLoadResource(NULL, "genericgoal\\textures\\activegoalanim.gif", 0, 0xdefa);
	BCB_GOAL_HELIX_TEXTURE(self) = (int)bLoadResource(NULL, "genericgoal\\textures\\helixtwist.bmp",
		0, 0xdefa);
	BCB_GOAL_SPARK_TEXTURE(self) = (int)bLoadResource(NULL, "genericgoal\\textures\\sparkburst.bmp",
		0, 0xdefa);
	self->numSystems = count;
	return self;
}

/* 0x000a8170  BumperCars/CBCar_GenericGoal.cpp (bracketed)
Deletes what a goal owns: the activation actor instance and the helix systems (their
destructors, then the array through the vector deleting destructor, slot 5). */
void BCarGenericGoal_DeleteGoal(CParticleEffect *self, CParticleEmitter *goal)
{
	CParticleSystem *helix;
	int i;

	if (goal->actor != 0)
		baDeleteActorInstance((struct BActorInstance *)goal->actor);
	if (goal->systems == NULL)
		return;
	for (i = 0; i < self->numSystems; i++)
	{
		helix = &goal->systems[i];
		((void (*)(CParticleSystem *))helix->h.vtable[0])(helix);
	}
	helix = goal->systems;
	if (helix != NULL)
	{
		if (((int *)helix)[-1] != 0)
		{
			((void (*)(CParticleSystem *, int))helix->h.vtable[5])(helix, 3);
			goal->systems = NULL;
			return;
		}
		BCB_DELETE((int *)helix - 1);
	}
	goal->systems = NULL;
}

/* 0x000a8200  BumperCars/CBCar_GenericGoal.cpp (bracketed)
Destructor: deletes every goal and the base actor instance. */
void BCarGenericGoal_Destruct(CParticleEffect *self)
{
	CParticleEmitter *next;

	while (self->emitters != NULL)
	{
		next = self->emitters->link;
		BCarGenericGoal_DeleteGoal(self, self->emitters);
		BCB_DELETE(self->emitters);
		self->emitters = next;
	}
	self->emitters = NULL;
	if (self->actor != 0)
		baDeleteActorInstance((struct BActorInstance *)self->actor);
}

/* 0x000a8250  BumperCars/CBCar_GenericGoal.cpp
AddGoal: a goal `id` at `pos` (raised by 4) of size `scale`, with its activation actor and
numSystems helix particle systems. NULL when there is no base actor. */
CParticleEmitter *BCarGenericGoal_AddGoal(CParticleEffect *self, float *pos, float scale, int id)
{
	CParticleEmitter *goal;
	struct BActorInstance *inst;
	int *block;
	int count;
	int i;
	CPSParams params;

	if (BCB_GOAL_BASE_ACTOR(self) == NULL)
		return NULL;
	goal = (CParticleEmitter *)bkHeapAlloc(0x40, BCB_SRC_GOAL, 0x93, 0x2001);
	memset(goal, 0, 0x40);
	goal->id = id;
	goal->pos[0] = pos[0];
	goal->pos[1] = pos[1];
	goal->pos[2] = pos[2];
	goal->pos[3] = pos[3];
	goal->scale = scale;
	goal->on = 0;
	goal->time = 0.0f;
	goal->colour = 0xffffffffu;
	goal->texture = 0;
	goal->pos[1] = goal->pos[1] + 4.0f;                          /* 0x00257c40 */
	goal->next = 0;
	if (BCB_GOAL_ACTIVE_ACTOR(self) == NULL)
		bkPrintf("%s(%d) : AddGoal - Could not find activation actor\n", BCB_SRC_GOAL, 0xb4);
	else
	{
		inst = baInitActorInstance(NULL, BCB_GOAL_ACTIVE_ACTOR(self), 2, BCB_SRC_GOAL, 0xa6);
		goal->actor = (int)inst;
		if (inst == NULL)
			bkPrintf("%s(%d) : AddGoal - Could not create activation actor instance\n",
				BCB_SRC_GOAL, 0xaf);
		else
		{
			inst->rot[0] = DAT_00285634[0];
			inst->rot[1] = DAT_00285634[1];
			inst->rot[2] = DAT_00285634[2];
			inst->rot[3] = DAT_00285634[3];
			baPrepActorInstance(inst, 1);
			goal->anim = (int)baFindSubObject(inst, "active");
			baFlushAnimations(inst, inst->root, 1, 0);
		}
	}

	/* new CParticleSystem[numSystems] (the count in front of the array) */
	count = self->numSystems;
	block = (int *)BCB_NEW(count * 0xa8 + 4, BCB_SRC_GOAL, 0xb8);
	if (block == NULL)
		goal->systems = NULL;
	else
	{
		block[0] = count;
		for (i = 0; i < count; i++)
			CParticleSystem_Construct((CParticleSystem *)((char *)(block + 1) + i * 0xa8));
		goal->systems = (CParticleSystem *)(block + 1);
	}
	if (goal->systems == NULL)
		bkPrintf("%s(%d) : AddGoal - Could not create helixTwist particle effect\n", BCB_SRC_GOAL,
			0xde);
	else
	{
		params.active = 0;
		params.unknown51 = 0;
		params.angleSpeed = 0.15707964f;                         /* 0x3e20d97c */
		params.riseAccel = 0.3f;                                 /* 0x3e99999a */
		params.gravity = -0.01f;                                 /* 0xbc23d70a */
		params.riseSpeed = 20.0f;
		params.speed = 5.0f;
		params.drag = 0.99f;                                     /* 0x3f7d70a4 */
		params.radiusSpeed = 2.0f;
		params.radius = 200.0f;
		params.numRings = 4;
		params.ringLength = 0xf;
		params.maxEmit = 10;
		params.lifeMin = 0x1e;
		params.lifeRange = 0x1e;
		params.pointSize = 80.0f;
		params.emitTime = 270.0f;
		for (i = 0; i < self->numSystems; i++)
		{
			CParticleSystem_Init(&goal->systems[i], &params, 600);
			CParticleSystem_SetTexture2(&goal->systems[i], BCB_GOAL_HELIX_TEXTURE(self));
			CParticleSystem_SetTexture(&goal->systems[i].h, BCB_GOAL_SPARK_TEXTURE(self));
		}
		goal->drift = NULL;
	}
	goal->link = self->emitters;
	self->emitters = goal;
	return goal;
}
