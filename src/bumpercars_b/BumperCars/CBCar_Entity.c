/*
BumperCars/CBCar_Entity.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Entity.cpp

The moving things of the arena (struct BCarEntity, 0x8c0 bytes): cars, pucks, keepsakes and
powerups. Every entity is on the list g_Entities (0x002f0764). This file has the physics
(thrust, gravity, friction, the collision with the world mesh and between entities and the
bounce), scoring, death and respawn, and the drawing with the floor reflection.

0x0009af70 is tagged with CBCar_Entity.cpp; the functions after it up to the world file
(0x0009e3b0) work on entities (file inferred), as do the gap functions 0x0009acb0-0x0009aee0
before it.
*/

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"

/* |v|^2 of a vector (0x00051470; bkernel names it bmNothing1 and makes it empty, but the
original returns x*x + y*y + z*z on the x87 stack) */
static float bcb_length_sq(const float *v)
{
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

/* the player slot of an entity, -1 when it is not a player's car (the original repeats the
four compares with the last match winning) */
static int bcb_player_of(struct BCarEntity *entity)
{
	int index = -1;

	if (g_BCarPlayers[0].entity == entity)
		index = 0;
	if (g_BCarPlayers[1].entity == entity)
		index = 1;
	if (g_BCarPlayers[2].entity == entity)
		index = 2;
	if (g_BCarPlayers[3].entity == entity)
		index = 3;
	return index;
}

/* 0x0009acb0  (gap; file inferred)
Constructor: the two timers. */
struct BCarEntity *BCarEntity_Construct(struct BCarEntity *self)
{
	BCarTimer_Construct(&self->growTimer);
	BCarTimer_Construct(&self->boostTimer);
	return self;
}

/* 0x0009ad00  (gap; file inferred)
Destructor: the two timers. */
void BCarEntity_Destruct(struct BCarEntity *self)
{
	BCarTimer_Destruct(&self->boostTimer);
	BCarTimer_Destruct(&self->growTimer);
}

/* 0x0009ad60  (gap; file inferred)
Turns the car by `delta` radians (not while it spins or respawns). */
void BCarEntity_Turn(struct BCarEntity *self, float delta)
{
	if (self->spinTime == 0.0f && self->respawnTime == 0.0f)
	{
		delta = delta + self->targetHeading;
		self->targetHeading = delta;
		self->targetHeading = CLMath_WrapAngle(delta);
	}
}

/* 0x0009adc0  (gap; file inferred)
Sets the heading the car turns to (not while it spins or respawns). */
void BCarEntity_SetHeading(struct BCarEntity *self, float heading)
{
	if (self->spinTime == 0.0f && self->respawnTime == 0.0f)
		self->targetHeading = CLMath_WrapAngle(heading);
}

/* 0x0009ae00  (gap; file inferred)
No score below zero. */
void BCarEntities_ClampScores(void)
{
	struct BCarEntity *entity;

	for (entity = g_Entities; entity != NULL; entity = entity->next)
	{
		if (entity->score < 0)
			entity->score = 0;
	}
}

/* 0x0009ae30  (gap; file inferred) */
void BCarEntity_SetType(struct BCarEntity *self, int type)
{
	self->type = type;
}

/* 0x0009ae40  (gap; file inferred)
Pauses the running grow and boost timers (the game is paused). */
void BCarEntities_PauseTimers(void)
{
	struct BCarEntity *entity;

	for (entity = g_Entities; entity != NULL; entity = entity->next)
	{
		if (entity->growing != 0)
			BCarTimer_Pause(&entity->growTimer);
		if (entity->boosting != 0)
			BCarTimer_Pause(&entity->boostTimer);
	}
}

/* 0x0009ae90  (gap; file inferred)
Resumes the grow and boost timers. */
void BCarEntities_ResumeTimers(void)
{
	struct BCarEntity *entity;

	for (entity = g_Entities; entity != NULL; entity = entity->next)
	{
		if (entity->growing != 0)
			BCarTimer_Start(&entity->growTimer);
		if (entity->boosting != 0)
			BCarTimer_Start(&entity->boostTimer);
	}
}

/* 0x0009aee0  (gap; file inferred)
Adds `points` to the score of an entity; a player's car shows them as a floaty score in the
colour of its character. */
void BCarEntity_AddScore(struct BCarEntity *self, int points)
{
	int index = -1;
	unsigned char *colour;

	if (points == 0)
		return;
	self->score = self->score + points;
	if (g_BCarPlayers[0].entity == self)
		index = 0;
	if (g_BCarPlayers[1].entity == self)
		index = 1;
	if (g_BCarPlayers[2].entity == self)
		index = 2;
	if (g_BCarPlayers[3].entity == self)
		index = 3;
	else if (index == -1)
		return;
	colour = &DAT_00290718[DAT_002baed4[index * 10] * 0x60];
	BCarFloaty_Add(g_FloatyScore, self->pos, points, colour[0], colour[1], colour[2]);
}

/* the length of an animation in seconds ((end - start) / 1024 frames at 60 Hz) */
static float bcb_anim_seconds(struct BAnim *anim)
{
	return (float)(anim->endTime - anim->startTime) * 0.0009765625f * 0.016666668f; /* 0x002593b0, 0x00262b7c */
}

/* 0x0009af70  BumperCars/CBCar_Entity.cpp
Creates an entity of `actor` (and the "grow" effect actor `extraActor`) scaled by `scale`,
from the start parameters `params` (passed by value in the original), puts it on the ground
and on the entity list. */
struct BCarEntity *BCarEntity_Create(struct BActor *actor, struct BActor *extraActor, float scale,
	struct BCarEntityParams *params)
{
	struct BCarEntity *self;
	struct BActorInstance *inst;
	struct BAnim *anim;
	void *memory;
	float height;

	memory = BCB_NEW(0x8c0, BCB_SRC_ENTITY, 0x17);
	if (memory == NULL)
		return NULL;
	self = BCarEntity_Construct((struct BCarEntity *)memory);
	if (self == NULL)
		return NULL;
	self->player = NULL;
	self->type = 0;
	self->vel[0] = 0.0f;
	self->vel[1] = 0.0f;
	self->vel[2] = 0.0f;
	self->vel[3] = 1.0f;
	self->oldVel[0] = 0.0f;
	self->oldVel[1] = 0.0f;
	self->oldVel[2] = 0.0f;
	self->oldVel[3] = 1.0f;
	self->force[0] = 0.0f;
	self->force[1] = 0.0f;
	self->force[2] = 0.0f;
	self->force[3] = 1.0f;
	self->accel[0] = 0.0f;
	self->accel[1] = 0.0f;
	self->accel[2] = 0.0f;
	self->accel[3] = 1.0f;
	self->rot[0] = 0.0f;
	self->rot[1] = 0.0f;
	self->rot[2] = 0.0f;
	self->rot[3] = 1.0f;
	self->pos[0] = params->pos[0];
	self->pos[1] = params->pos[1];
	self->pos[2] = params->pos[2];
	self->pos[3] = params->pos[3];
	self->oldPos[0] = self->pos[0];
	self->oldPos[1] = self->pos[1];
	self->oldPos[2] = self->pos[2];
	self->oldPos[3] = self->pos[3];
	self->heading = params->angle;
	self->targetHeading = params->angle;
	self->mass = params->mass;
	self->radius = params->radius;
	self->respawnDelay = params->respawnDelay;
	height = BCarWorld_GetGroundHeight(self->pos, params->radius, NULL);
	if (height < 10000.0f)                                      /* 0x002657a0 */
		self->pos[1] = self->pos[1] - (height - 1.0f);
	if (params->mass == 0.0f)
		self->invMass = 32767.0f;                               /* 0x46fffe00 */
	else
		self->invMass = 1.0f / params->mass;
	self->drawOffset = params->drawOffset;
	self->carriedOffset = params->carriedOffset;
	self->actor = actor;
	inst = baInitActorInstance(NULL, actor, 1, BCB_SRC_ENTITY, 0x3b);
	self->inst = inst;
	inst->rot[0] = DAT_00285634[0];
	inst->rot[1] = DAT_00285634[1];
	inst->rot[2] = DAT_00285634[2];
	inst->rot[3] = DAT_00285634[3];
	baPrepActorInstance(self->inst, 1);
	self->inst->scale[2] = scale;
	self->inst->scale[1] = scale;
	self->inst->scale[0] = scale;
	anim = (struct BAnim *)baFindSubObject(self->inst, "default");
	self->animDefault = anim;
	if (anim != NULL)
	{
		baFlushAnimations(self->inst, self->inst->root, 1, 0);
		baQueueAnimation(self->inst, self->inst->root, self->animDefault, 0x400, -1, 1, NULL, NULL);
	}
	self->extraActor = extraActor;
	if (extraActor == NULL)
		self->extraInst = NULL;
	else
	{
		inst = baInitActorInstance(NULL, extraActor, 2, BCB_SRC_ENTITY, 0x4a);
		self->extraInst = inst;
		if (inst != NULL)
		{
			inst->rot[0] = DAT_00285634[0];
			inst->rot[1] = DAT_00285634[1];
			inst->rot[2] = DAT_00285634[2];
			inst->rot[3] = DAT_00285634[3];
			baPrepActorInstance(self->extraInst, 1);
			anim = (struct BAnim *)baFindSubObject(self->extraInst, "1-grow");
			self->animGrow = anim;
			if (anim != NULL)
				self->animTime[0] = bcb_anim_seconds(anim);
			anim = (struct BAnim *)baFindSubObject(self->extraInst, "3-idle");
			self->animIdle = anim;
			if (anim != NULL)
				self->animTime[1] = bcb_anim_seconds(anim);
			anim = (struct BAnim *)baFindSubObject(self->extraInst, "2-rumble");
			self->animRumble = anim;
			if (anim != NULL)
				self->animTime[2] = bcb_anim_seconds(anim);
		}
	}
	self->score = 0;
	self->kingCounter = 0;
	self->carried = NULL;
	self->carrier = NULL;
	self->carryCounter = 0;
	self->lap = 0;
	self->team = -1;
	self->frozen = 0;
	self->unk048 = 0;
	self->lag = 0;
	self->pickup = 0;
	self->scored = 0;
	self->respawnTime = 0.0f;
	self->respawnBlink = 0;
	self->growing = 0;
	BCarTimer_Reset(&self->growTimer);
	BCarTimer_Reset(&self->boostTimer);
	self->boosting = 0;
	self->dead = 0;
	self->falling = 0;
	self->active = 1;
	self->deathTime = 0;
	self->speedScale = 1.0f;
	self->other = NULL;
	self->lastHit = NULL;
	self->spinTime = 0.0f;
	self->groundNormal[0] = 0.0f;
	self->groundNormal[1] = 1.0f;
	self->groundNormal[2] = 0.0f;
	self->onGround = 0;
	self->lives = g_Lives;
	self->next = g_Entities;
	g_Entities = self;
	self->instPos[0] = self->inst->pos[0];
	self->instPos[1] = self->inst->pos[1];
	self->instPos[2] = self->inst->pos[2];
	self->instPos[3] = self->inst->pos[3];
	self->instPos[1] = self->radius * 0.5f + self->instPos[1];  /* 0x002536d0 */
	self->instRot[0] = self->inst->rot[0];
	self->instRot[1] = self->inst->rot[1];
	self->instRot[2] = self->inst->rot[2];
	self->instRot[3] = self->inst->rot[3];
	return self;
}

/* 0x0009b4c0  (file inferred: CBCar_Entity.cpp)
Pushes the car forward along its heading by `amount` when it is on the ground (or `force`
is set), not while it spins or respawns. */
void BCarEntity_Thrust(struct BCarEntity *self, float amount, int force)
{
	float dir[4];
	float out[3];

	if ((self->onGround != 0 || force != 0) && self->spinTime == 0.0f && self->respawnTime == 0.0f)
	{
		dir[2] = amount;
		dir[0] = 0.0f;
		dir[1] = 0.0f;
		dir[3] = 1.0f;
		bmVectorRotateY(out, dir, self->targetHeading);
		self->force[0] = out[0] + self->force[0];
		self->force[1] = out[1] + self->force[1];
		self->force[2] = out[2] + self->force[2];
	}
}

/* 0x0009b580  (file inferred: CBCar_Entity.cpp)
Kills an entity for good (no lives left): it stops and the death time is kept. */
void BCarEntity_Kill(struct BCarEntity *self)
{
	self->active = 0;
	self->dead = 1;
	self->falling = 0;
	self->deathTime = BCarTimer_GetUpdates(&g_BCarTimer.timer);
	self->rot[0] = 0.0f;
	self->rot[1] = 0.0f;
	self->rot[2] = 0.0f;
	self->rot[3] = 1.0f;
	BCarEntity_DropCarried(self);
}

/* 0x0009b5e0  (file inferred: CBCar_Entity.cpp)
Finds the first entity that this moving one runs into (spheres of twice the radius,
approaching): sets the contact normal and depth and both entities' "other". Returns 1 on a
collision. */
int BCarEntity_CollideEntities(struct BCarEntity *self)
{
	float radius = self->radius;
	struct BCarEntity *other;
	float otherY;
	float selfOffset;
	float otherOffset;
	float delta[4];
	float depth;

	if (self->carrier != NULL ||
		sqrtf(self->vel[2] * self->vel[2] + self->vel[1] * self->vel[1] +
			self->vel[0] * self->vel[0]) == 0.0f ||
		self->dead != 0 || self->respawnTime != 0.0f)
		return 0;
	for (other = g_Entities; other != NULL; other = other->next)
	{
		if (other == self || other->active == 0 || other->carrier != NULL ||
			other == self->carried || other->dead != 0 || other->respawnTime != 0.0f)
			continue;
		if (!(other->pickup == 0 || other->type != BCAR_ENT_KEEPSAKE || self->carried == NULL))
			continue;
		if (!(self->pickup == 0 || self->type != BCAR_ENT_KEEPSAKE || other->carried == NULL))
			continue;
		otherY = other->pos[1];
		selfOffset = self->pickup == 0 ? self->drawOffset : self->carriedOffset;
		otherOffset = other->pickup == 0 ? other->drawOffset : other->carriedOffset;
		delta[0] = self->pos[0] - other->pos[0];
		delta[1] = (self->pos[1] + selfOffset) - (otherY + otherOffset);
		delta[2] = self->pos[2] - other->pos[2];
		depth = bcb_length_sq(delta) - (other->radius + other->radius) * (radius + radius);
		if (depth < 0.1f)                                       /* 0x002536cc */
		{
			bmVectorNormalise(delta, delta);
			if (delta[0] * self->vel[0] + delta[1] * self->vel[1] + delta[2] * self->vel[2] < 0.0f)
			{
				self->contactDepth = depth;
				self->contactNormal[0] = delta[0];
				self->contactNormal[1] = delta[1];
				self->contactNormal[2] = delta[2];
				self->contactNormal[3] = delta[3];
				self->other = other;
				other->other = self;
				if (other->type != BCAR_ENT_PUCK || self->type != BCAR_ENT_PUCK)
				{
					other->lastHit = self;
					self->lastHit = other;
				}
				self->lastHitFrames = 0;
				other->lastHitFrames = 0;
				return 1;
			}
		}
	}
	return 0;
}

/* 0x0009b850  (file inferred: CBCar_Entity.cpp)
Sweeps the entity's sphere along its velocity through the world collision mesh: up to 30
hits in self->hits. Returns 1 on a hit. */
int BCarEntity_CollideWorld(struct BCarEntity *self)
{
	struct BCollMesh *mesh = g_WorldCollision;
	struct BSphere sphere;
	int count;

	if (self->carrier != NULL)
		return 0;
	if (sqrtf(self->vel[2] * self->vel[2] + self->vel[1] * self->vel[1] +
		self->vel[0] * self->vel[0]) == 0.0f)
		return 0;
	DAT_00285644 = 0.01f;                                      /* 0x3c23d70a */
	self->hits[0].flags = 0xd;
	sphere.centre[0] = self->pos[0];
	sphere.centre[1] = self->pos[1];
	sphere.centre[2] = self->pos[2];
	sphere.centre[3] = 1.0f;
	sphere.radius = self->radius;
	count = 0x1e;
	if (bmSweepSphereMesh(&sphere, mesh, self->vel, &count, self->hits) != 0)
	{
		self->numHits = count;
		return 1;
	}
	self->numHits = 0;
	return 0;
}

/* 0x0009b930  (file inferred: CBCar_Entity.cpp)
Respawns an entity at the best spawn point (the one farthest from the others): stopped,
facing the spawn angle, blinking, with a sound and (for a player's car) the respawn debris,
speech and HUD flash. */
void BCarEntity_Respawn(struct BCarEntity *self)
{
	struct BCarSpawn *spawn;
	struct BCarPlayer *player;
	void *debris;
	float height;
	float y;
	float pos[4];
	int index;
	float now;

	spawn = BCarWorld_FindSpawn(self);
	if (spawn == NULL)
		return;
	index = bcb_player_of(self);
	self->rot[0] = 0.0f;
	self->rot[1] = 0.0f;
	self->rot[2] = 0.0f;
	self->rot[3] = 1.0f;
	self->accel[0] = 0.0f;
	self->accel[1] = 0.0f;
	self->accel[2] = 0.0f;
	self->accel[3] = 1.0f;
	self->vel[0] = 0.0f;
	self->vel[1] = 0.0f;
	self->vel[2] = 0.0f;
	self->vel[3] = 1.0f;
	self->oldVel[0] = 0.0f;
	self->oldVel[1] = 0.0f;
	self->oldVel[2] = 0.0f;
	self->oldVel[3] = 1.0f;
	self->pos[0] = spawn->pos[0];
	self->pos[1] = spawn->pos[1];
	self->pos[2] = spawn->pos[2];
	self->pos[3] = spawn->pos[3];
	self->oldPos[0] = self->pos[0];
	self->oldPos[1] = self->pos[1];
	self->oldPos[2] = self->pos[2];
	self->oldPos[3] = self->pos[3];
	y = self->pos[1];
	height = BCarWorld_GetGroundHeight(self->pos, self->radius, NULL);
	self->pos[1] = y - (height - 1.0f);
	self->targetHeading = spawn->angle;
	self->heading = spawn->angle;
	BCarSound_Play(BCB_SOUND, 9, 0xff, 0, -1);
	pos[1] = self->pos[1] + self->drawOffset;
	pos[0] = self->pos[0];
	pos[2] = self->pos[2];
	pos[3] = self->pos[3];
	if (index == -1)
		BCarEffects_Burst(BCB_EFFECTS, 1, pos, DAT_002ea478, 0, -1);
	else
	{
		DAT_00290e8c = DAT_002baed4[index * 10];
		debris = Debris_Add((struct DebrisType *)DAT_00291734, pos,
			DAT_002ea478, 0, 0xffffffffu);
		BCB_F32(debris, 0x8c) = 3.0f;                          /* 0x40400000 */
		if (bcb_rand() % 0x19 == 0)
			BCarSound_Speak(BCB_SOUND, index, 0xf, 0xff, self->pos, 0.0f);
		BCB_F32(DAT_002ecfac, 0xb8 + index * 4) = 180.0f;      /* 0x43340000 */
		BCB_F32(DAT_002ecfac, 0xa8 + index * 4) = 50.0f;       /* 0x42480000 */
	}
	BCB_CALL(FUN_00082b20, void (*)(float *, float *, float *, float *))(
		g_BCarPlayers[0].entity->pos, g_BCarPlayers[1].entity->pos,
		g_BCarPlayers[2].entity->pos, g_BCarPlayers[3].entity->pos);
	self->other = NULL;
	self->lastHit = NULL;
	self->groundNormal[0] = 0.0f;
	self->groundNormal[1] = 1.0f;
	self->groundNormal[2] = 0.0f;
	self->onGround = 0;
	self->spinTime = 0.0f;
	player = self->player;
	if (player != NULL)
	{
		player->boostReadyTime = 0.0f;
		if (player->boost != NULL)
			biClearChannel(player->boost);
		self->boosting = 0;
		BCarTimer_Restart(&self->boostTimer);
	}
	self->respawnBlink = 1;
	now = BCB_NOW();
	self->respawnTime = self->respawnDelay;
	self->growing = 0;
	self->dead = 0;
	self->respawnBlinkEnd = now + self->respawnDelay + g_RestartWait;
	self->falling = 0;
	self->speedScale = 1.0f;
	BCarEntity_DropCarried(self);
}

/* draw `inst` mirrored in the floor (see CBCar_Player.c) */
static void bcb_entity_reflected(struct BActorInstance *inst, int pass)
{
	float rot[4];
	float normal[4];
	float height;

	rot[0] = inst->rot[0];
	rot[1] = inst->rot[1];
	rot[2] = inst->rot[2];
	rot[3] = inst->rot[3];
	height = BCarWorld_GetGroundHeight(inst->pos, 0.0f, normal);
	BCarMath_TiltToNormal(inst->rot, inst->rot, normal);
	inst->scale[1] = inst->scale[1] * -1.0f;                  /* 0x00253854 */
	inst->pos[1] = inst->pos[1] - (height + height);
	baPrepActorInstance(inst, 1);
	baDrawActorInstance(inst, pass);
	inst->pos[1] = (height + height) + inst->pos[1];
	inst->scale[1] = inst->scale[1] * -1.0f;
	inst->rot[0] = rot[0];
	inst->rot[1] = rot[1];
	inst->rot[2] = rot[2];
	inst->rot[3] = rot[3];
}

/* 0x0009bc50  (file inferred: CBCar_Entity.cpp)
Draws the cars (not the pucks, keepsakes and powerups) with their floor reflections and the
grow effect. `shadow` selects the second render pass; `ghost` draws the speed ghost instead
(only cars faster than half their top speed, dimmer when slower). The car of the player
viewed from inside is not drawn. */
void BCarEntities_Draw(int shadow, int ghost)
{
	struct BCarEntity *entity;
	int index;
	int view;
	int i;
	int pass = shadow != 0 ? 6 : 1;
	float speed;
	float half;

	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 0, 0);
	if ((BCB_DEBUG_GET(FUN_0008c000) & 0xff) == 0 && BCB_VAR(int, 0x002f0a6c) != 0)
		bdSetRenderState(3, 1, 0);
	else
		bdSetRenderState(3, 0, 0);
	bdSetWorldIdentity();
	for (entity = g_Entities; entity != NULL; entity = entity->next)
	{
		if (entity->dead != 0 || entity->respawnTime != 0.0f || entity->type == BCAR_ENT_KEEPSAKE ||
			entity->type == BCAR_ENT_PUCK || entity->type == BCAR_ENT_POWERUP)
			continue;
		index = -1;
		for (i = 0; i < BCB_NUM_PLAYERS; i++)
		{
			if (entity == g_BCarPlayers[i].entity)
				index = i;
		}
		view = i;
		if (BCarCamera_GetViewPlayer(BCB_CAMERA, &view) != 0 && view == index)
			continue;
		if (ghost != 0)
		{
			speed = sqrtf(entity->vel[2] * entity->vel[2] + entity->vel[1] * entity->vel[1] +
				entity->vel[0] * entity->vel[0]);
			if (entity->type == BCAR_ENT_CAR)
			{
				if (!(BCarPlayer_GetMaxSpeed(&g_BCarPlayers[index]) * 0.5f <= speed))
					continue;
				speed = speed - BCarPlayer_GetMaxSpeed(&g_BCarPlayers[index]) * 0.5f;
				half = BCarPlayer_GetMaxSpeed(&g_BCarPlayers[index]);
			}
			else
			{
				if (!(BCarWorld_GetMaxSpeed() * 0.5f <= speed))
					continue;
				speed = speed - BCarWorld_GetMaxSpeed() * 0.5f;
				half = BCarWorld_GetMaxSpeed();
			}
			baSetBrightness(BCB_FTOL(speed / (half * 0.5f) * 127.0f * entity->alpha)); /* 0x00255348 */
		}
		if (entity->type == BCAR_ENT_CAR)
			BCarLight_Enable(BCB_LIGHTS, 1);
		if (BCB_DEBUG_GET(FUN_0008c040) == 0)
		{
			if (ghost == 0)
				baSetBrightness(BCB_FTOL(entity->alpha * 255.0f)); /* 0x002537d0 */
		}
		else if (ghost == 0)
		{
			FUN_001d6820();
			FUN_001d6860();
			BCarRender_BeginReflection(ghost);
			bcb_entity_reflected(entity->inst, pass);
			if (entity->extraInst != NULL && entity->growing != 0)
			{
				bdPushRenderState(10, 1, 0);
				bcb_entity_reflected(entity->extraInst, pass);
				bdPopRenderState(10);
			}
			BCarRender_EndReflection();
			FUN_0018e640();
			baSetBrightness(BCB_FTOL(entity->alpha * 255.0f));
		}
		if (entity->growing != 0 && entity->extraInst != NULL)
		{
			bdPushRenderState(10, 1, 0);
			baPrepActorInstance(entity->extraInst, 1);
			baDrawActorInstance(entity->extraInst, pass);
			bdPopRenderState(10);
		}
		baPrepActorInstance(entity->inst, 1);
		baDrawActorInstance(entity->inst, pass);
		if (entity->type == BCAR_ENT_CAR)
			BCarLight_Enable(BCB_LIGHTS, 0);
		else
			BCarLight_Restore(BCB_LIGHTS);
	}
	bdPopRenderState(10);
	bdPopRenderState(6);
	baSetBrightness(0x100);
}

/* 0x0009c170  (file inferred: CBCar_Entity.cpp)
The centre of the live cars (raised by their draw offset) in `out`; returns their number. */
int BCarEntities_GetCentre(float *out)
{
	struct BCarEntity *entity;
	int count = 0;
	float scale;

	out[0] = 0.0f;
	out[1] = 0.0f;
	out[2] = 0.0f;
	out[3] = 1.0f;
	entity = g_Entities;
	if (entity == NULL)
		return 0;
	do
	{
		if (entity->type == BCAR_ENT_CAR && entity->active != 0)
		{
			count++;
			out[0] = entity->pos[0] + out[0];
			out[1] = entity->pos[1] + out[1];
			out[2] = entity->pos[2] + out[2];
			out[1] = entity->drawOffset + out[1];
		}
		entity = entity->next;
	} while (entity != NULL);
	if (count == 0)
		return 0;
	scale = 1.0f / (float)count;
	out[0] = scale * out[0];
	out[1] = scale * out[1];
	out[2] = scale * out[2];
	return count;
}

/* 0x0009c210  (file inferred: CBCar_Entity.cpp)
Spins a car: it turns by a random 150-270 degrees either way and cannot steer for `time`
seconds (BRAKE_HIT_TIMEOUT when `time` is not positive). */
void BCarEntity_Spin(struct BCarEntity *self, float time)
{
	float angle;
	float spin;
	int r1 = bcb_rand();
	int r2 = bcb_rand();

	angle = (float)r1 * 0.0036622211f + 150.0f;               /* 0x002657d8, 0x00262df8 */
	if ((r2 & 1) == 0)
		angle = -angle;
	angle = angle * 0.017453292f;                              /* 0x002640f8 */
	spin = g_BrakeHitTimeout;
	if (0.0f < time)
		spin = time;
	self->spinTime = spin;
	angle = angle + self->targetHeading;
	self->targetHeading = angle;
	self->targetHeading = CLMath_WrapAngle(angle);
}

/* 0x0009c2d0  (file inferred: CBCar_Entity.cpp)
Deletes every entity. */
void BCarEntities_DeleteAll(void)
{
	struct BCarEntity *entity = g_Entities;
	struct BCarEntity *next;

	while (entity != NULL)
	{
		if (entity->inst != NULL)
		{
			baDeleteActorInstance(entity->inst);
			entity->inst = NULL;
		}
		if (entity->extraInst != NULL)
		{
			baDeleteActorInstance(entity->extraInst);
			entity->extraInst = NULL;
		}
		BCarTimer_Destruct(&entity->growTimer);
		BCarTimer_Destruct(&entity->boostTimer);
		next = entity->next;
		BCarEntity_Destruct(entity);
		BCB_DELETE(entity);
		entity = next;
	}
	g_Entities = NULL;
}

/* the scores of a hit: what the hitting car gets (index 0: normal, 1: when it leads) */
#define BCB_SCORE(lead, normal, leader) ((lead) ? g_Score(leader) : g_Score(normal))

/* take the puck (or keepsake) the other entity carries; returns 1 when taken */
static int bcb_entity_steal(struct BCarEntity *self, int *selfIndex, int *otherIndex)
{
	struct BCarEntity *carried;

	if (BCarEntity_SameTeam(self, self->other, selfIndex, otherIndex) != 0 || self->carried != NULL)
		return 0;
	carried = self->other->carried;
	if (carried == NULL)
		return 0;
	self->carried = carried;
	carried->carrier = self;
	self->other->carried = NULL;
	return 1;
}

/* 0x0009c360  (file inferred: CBCar_Entity.cpp)
The response to this frame's collision (self->collisionFlags): against the world, slide
along the nearest hit (bouncing by WORLDRESTITUTION off walls) and drop what it carries on a
hard wall hit; against another entity, the hit scores (front, side, back; for both cars),
stealing the puck, picking items up, sparks, the crash sound, spinning out (DISABLE_VEL)
and the bounce of both (ENTITYBOUNCE, shared by mass and ENTITYCOLLISIONBIAS). `control`:
the game is running (scores count). */
void BCarEntity_Bounce(struct BCarEntity *self, int control)
{
	unsigned int flags = self->collisionFlags;
	int kind = 1;
	int selfIndex;
	int index;
	int count;
	int i;
	int lead;
	int sameTeam;
	int stealIndex;
	int otherIndex;
	int points;
	int leadPoints;
	int volume;
	int sample;
	float best;
	float n[4];
	float contact[4];
	float selfVel;
	float otherVel;
	float angle;
	float selfAngle;
	float otherAngle;
	float bounce;
	float ratio;
	float size;
	float a9;
	float b24;
	float d26;
	int sparks;
	float share;
	float selfShare;
	float otherShare;
	float bias;
	struct BCarEntity *other;

	if ((flags & BCAR_HIT_WORLD) == 0)
	{
		if ((flags & BCAR_HIT_ENTITY) != 0)
			kind = 2;
	}
	else if ((flags & BCAR_HIT_ENTITY) != 0 && self->contactDepth < self->hits[0].t)
		kind = 2;
	selfIndex = bcb_player_of(self);

	if (kind != 2)
	{
		/* the world: the nearest hit */
		best = self->hits[0].t;
		index = 0;
		for (i = 1; i < self->numHits; i++)
		{
			if (self->hits[i].t < best)
			{
				best = self->hits[i].t;
				index = i;
			}
		}
		self->groundPoint[0] = self->hits[index].point[0];
		self->groundPoint[1] = self->hits[index].point[1];
		self->groundPoint[2] = self->hits[index].point[2];
		self->groundPoint[3] = self->hits[index].point[3];
		n[0] = self->hits[index].normal[0];
		n[1] = self->hits[index].normal[1];
		n[2] = self->hits[index].normal[2];
		n[3] = self->hits[index].normal[3];
		self->groundNormal[0] = n[0];
		self->groundNormal[3] = n[3];
		self->groundNormal[2] = n[2];
		self->groundNormal[1] = n[1];
		selfVel = n[0] * self->vel[0] + n[1] * self->vel[1] + n[2] * self->vel[2];
		if (0.0f < g_WallCollisionVel && selfVel < -g_WallCollisionVel && self->carried != NULL)
		{
			/* a hard wall hit: what the car carries flies back */
			other = self->carried;
			other->pickup = 1;
			other->carrier = NULL;
			other->vel[0] = -self->vel[0];
			other->vel[1] = -self->vel[1];
			other->vel[2] = -self->vel[2];
			other->vel[1] = other->vel[1] + 10.0f;              /* 0x002536d8 */
			share = self->mass / other->mass;
			other->vel[0] = share * other->vel[0];
			other->vel[1] = share * other->vel[1];
			other->vel[2] = share * other->vel[2];
			self->carried = NULL;
		}
		if ((n[0] + n[2]) * 0.0f + n[1] <= 0.8f)                /* 0x00257c78 */
			bounce = BCarWorld_GetRestitution();
		else
			bounce = 0.0f;
		bounce = (bounce + 1.0f) * selfVel;
		self->vel[0] = self->vel[0] - n[0] * bounce;
		self->vel[1] = self->vel[1] - n[1] * bounce;
		self->vel[2] = self->vel[2] - n[2] * bounce;
		return;
	}

	if (self->active == 0)
		return;
	other = self->other;
	n[0] = self->contactNormal[0];
	n[1] = self->contactNormal[1];
	n[2] = self->contactNormal[2];
	n[3] = self->contactNormal[3];
	selfVel = n[0] * self->vel[0] + n[1] * self->vel[1] + n[2] * self->vel[2];
	otherVel = n[0] * other->vel[0] + n[1] * other->vel[1] + n[2] * other->vel[2];
	angle = BCarMath_Atan2(n[0], n[2]);
	selfAngle = CLMath_WrapAngle((angle - self->heading) + 3.1415925f);   /* 0x00256750 */
	otherAngle = CLMath_WrapAngle(angle - self->other->heading);
	lead = g_LeadPlayer != NULL && g_LeadPlayer->entity == self;
	stealIndex = 0;
	otherIndex = 0;

	if (control != 0)
	{
		int stolen = 0;

		if (selfAngle < 0.7853982f || 5.497787f < selfAngle)  /* 0x002640f4, 0x00265800 */
		{
			/* hit in its front */
			if (g_EntCollisionVel < otherVel)
				BCarEntity_AddScore(self, BCB_SCORE(lead, 0x002efa2c, 0x002efa68)); /* SELF_HIT_FRONT */
			if (selfVel < -g_EntCollisionVel)
			{
				if (otherAngle < 0.7853982f || 5.497787f < otherAngle)
				{
					sameTeam = BCarEntity_SameTeam(self, self->other, NULL, NULL);
					points = g_Score(0x002efa20);              /* OTHER_HIT_FRONT */
					leadPoints = g_Score(0x002efa5c);
				}
				else if (otherAngle <= 2.3561945f || 3.926991f <= otherAngle) /* 0x002657fc, 0x002657f8 */
				{
					sameTeam = BCarEntity_SameTeam(self, self->other, NULL, NULL);
					points = g_Score(0x002efa24);              /* OTHER_HIT_SIDE */
					leadPoints = g_Score(0x002efa60);
				}
				else
				{
					sameTeam = BCarEntity_SameTeam(self, self->other, NULL, NULL);
					points = g_Score(0x002efa28);              /* OTHER_HIT_BACK */
					leadPoints = g_Score(0x002efa64);
				}
				if (sameTeam == 0)
					BCarEntity_AddScore(self, lead ? leadPoints : points);
				stolen = bcb_entity_steal(self, &stealIndex, &otherIndex);
			}
		}
		else if (selfAngle <= 2.3561945f || 3.926991f <= selfAngle)
		{
			/* hit in its side */
			if (g_EntCollisionVel < otherVel)
				BCarEntity_AddScore(self, BCB_SCORE(lead, 0x002efa30, 0x002efa6c)); /* SELF_HIT_SIDE */
			if (selfVel < -g_EntCollisionVel)
				stolen = bcb_entity_steal(self, &stealIndex, &otherIndex);
		}
		else
		{
			/* hit in its back */
			if (g_EntCollisionVel < otherVel)
				BCarEntity_AddScore(self, BCB_SCORE(lead, 0x002efa34, 0x002efa70)); /* SELF_HIT_BACK */
			if (selfVel < -g_EntCollisionVel)
				stolen = bcb_entity_steal(self, &stealIndex, &otherIndex);
		}
		if (stolen != 0)
		{
			BCarSound_Play(BCB_SOUND, 0x1b, 0xff, 0, -1);
			if ((int)Core_Rand() % 0x19 == 0)
				BCarSound_Speak(BCB_SOUND, stealIndex, 6, 0xff, self->pos, 0.0f);
			if ((int)Core_Rand() % 0x19 == 0)
				BCarSound_Speak(BCB_SOUND, otherIndex, 7, 0xff, self->other->pos, 0.0f);
		}

		/* picking up a puck, keepsake or powerup */
		other = self->other;
		if (other->pickup != 0)
		{
			if (other->type == BCAR_ENT_POWERUP)
			{
				BCarPowerup_Hit(self, other);
				return;
			}
			if (self->carried == NULL)
			{
				other->pickup = 0;
				self->other->carrier = self;
				self->carried = self->other;
				BCarSound_Play(BCB_SOUND, 0x1c, 0xff, 0, -1);
				if (selfIndex != -1)
				{
					BCarSound_Speak(BCB_SOUND, selfIndex, 5, 0xff, self->pos, 0.0f);
					return;
				}
			}
			return;
		}
	}

	/* the crash: sparks and sound at the contact point */
	other = self->other;
	contact[0] = other->pos[0] - self->pos[0];
	contact[1] = other->pos[1] - self->pos[1];
	contact[2] = other->pos[2] - self->pos[2];
	bmVectorNormalise(contact, contact);
	size = self->radius;
	contact[0] = contact[0] * size + self->pos[0];
	contact[2] = contact[2] * size + self->pos[2];
	contact[1] = size * contact[1] + self->pos[1] + self->drawOffset;
	switch (g_BCarVariant)
	{
	case 1:
		ratio = bmVectorDistance(self->other->vel, self->vel) / (BCarWorld_GetMaxSpeed() * 2.255f); /* 0x002657f4 */
		break;
	case 2:
		ratio = bmVectorDistance(self->other->vel, self->vel) / (BCarWorld_GetMaxSpeed() * 2.75f);  /* 0x002657f0 */
		break;
	case 3:
		ratio = bmVectorDistance(self->other->vel, self->vel) / (BCarWorld_GetMaxSpeed() * 5.0f);   /* 0x002536c4 */
		break;
	default:
		ratio = bmVectorDistance(self->other->vel, self->vel) / (BCarWorld_GetMaxSpeed() + BCarWorld_GetMaxSpeed());
		break;
	}
	size = ratio * 1.5f;                                        /* 0x002538a0 */
	if (!(size <= 1.5f))
		size = 1.5f;
	BCarEffects_Burst(BCB_EFFECTS, 0, contact, DAT_002ea478, 0, -1);
	sparks = BCB_FTOL(size * 10.0f);                            /* 0x002536d8 */
	a9 = size * 9.0f;                                           /* 0x002657ec */
	b24 = size * 24.0f;                                         /* 0x002657e8 */
	BCarEffects_Spark(BCB_EFFECTS, 2, contact, size * 12.0f, b24, a9, sparks); /* 0x00254f78 */
	BCarEffects_Spark(BCB_EFFECTS, 2, contact, size * 0.3f, size * 48.0f, size * 15.0f, sparks); /* 0x00265580, 0x002657e4, 0x002553a0 */
	d26 = size * 26.0f;                                         /* 0x002657e0 */
	BCarEffects_Spark(BCB_EFFECTS, 2, contact, b24, d26, size * 6.0f, BCB_FTOL(size * 4.0f)); /* 0x00255334, 0x00257c40 */
	BCarEffects_Spark(BCB_EFFECTS, 2, contact, d26, b24, a9, BCB_FTOL(size * 7.0f)); /* 0x002657dc */
	if (-1.0f < g_DisableVel)
	{
		if (g_DisableVel < otherVel)
			BCarEntity_Spin(self, -1.0f);
		if (g_DisableVel < -selfVel)
			BCarEntity_Spin(self->other, -1.0f);
	}
	if (self->type == BCAR_ENT_PUCK || self->other->type == BCAR_ENT_PUCK)
	{
		DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
		volume = 0x7f - BCB_FTOL(ratio * -127.0f);              /* 0x002632d0 */
		sample = ((int)((DAT_00285630 >> 16) & 0x7fff) % 2) + 4;
	}
	else
	{
		DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
		volume = 0x7f - BCB_FTOL(ratio * -127.0f);
		sample = (int)((DAT_00285630 >> 16) & 0x7fff) % 3;
	}
	BCarSound_Play(BCB_SOUND, sample, volume, 0, -1);

	/* a human player's car hit on its flat spot (straight on) stops dead */
	if (selfIndex != -1 && g_BCarPlayers[selfIndex].human != 0 &&
		(selfAngle < g_FlatspotAngle * 0.017453292f ||
			6.2831855f - g_FlatspotAngle * 0.017453292f < selfAngle)) /* 0x002640f8, 0x00255930 */
	{
		bmVectorNormalise(n, self->vel);
		n[0] = -n[0];
		self->contactNormal[0] = n[0];
		n[1] = -n[1];
		self->contactNormal[1] = n[1];
		n[2] = -n[2];
		self->contactNormal[3] = n[3];
		self->contactNormal[2] = n[2];
	}

	/* the bounce */
	n[0] = n[0] * selfVel;
	n[1] = n[1] * selfVel;
	n[2] = n[2] * selfVel;
	share = 1.0f / (self->mass + self->other->mass);
	selfShare = 1.0f - share * self->mass;
	otherShare = 1.0f - share * self->other->mass;
	bias = (g_EntityCollisionBias + g_EntityCollisionBias) - 1.0f;
	if (-selfVel <= otherVel)
	{
		if (0.0f < bias)
		{
			selfShare = selfShare - bias * selfShare;
			otherShare = (1.0f - otherShare) * bias + otherShare;
		}
		else if (bias < 0.0f)
		{
			selfShare = selfShare - (1.0f - selfShare) * bias;
			otherShare = bias * otherShare + otherShare;
		}
	}
	else if (0.0f < bias)
	{
		selfShare = (1.0f - selfShare) * bias + selfShare;
		otherShare = otherShare - bias * otherShare;
	}
	else if (bias < 0.0f)
	{
		selfShare = bias * selfShare + selfShare;
		otherShare = otherShare - (1.0f - otherShare) * bias;
	}
	selfShare = g_EntityBounce * selfShare;
	self->vel[0] = (self->vel[0] - n[0]) - n[0] * selfShare;
	self->vel[1] = (self->vel[1] - n[1]) - n[1] * selfShare;
	self->vel[2] = (self->vel[2] - n[2]) - n[2] * selfShare;
	other = self->other;
	otherShare = g_EntityBounce * otherShare;
	other->vel[0] = n[0] * otherShare + other->vel[0];
	other->vel[1] = n[1] * otherShare + other->vel[1];
	other->vel[2] = n[2] * otherShare + other->vel[2];
}

/* the score of a kill (lead: when the dead car leads) */
static void bcb_entity_killed(struct BCarEntity *self, int lead)
{
	struct BCarEntity *killer = self->lastHit;

	if (self->type == BCAR_ENT_PUCK)
	{
		if (killer != NULL)
			BCarEntity_AddScore(killer, g_Score(0x002efa50));   /* PUCK_KILL */
		return;
	}
	if (killer == NULL || BCarEntity_SameTeam(self, killer, NULL, NULL) != 0)
	{
		if (lead)
		{
			BCarEntity_AddScore(self, g_Score(0x002efa84));     /* LEAD_SELF_KILL */
			return;
		}
	}
	else
	{
		if (lead)
		{
			BCarEntity_AddScore(self->lastHit, g_Score(0x002efa88)); /* LEAD_OTHER_KILL */
			BCarEntity_AddScore(self, g_Score(0x002efa84));
			return;
		}
		BCarEntity_AddScore(self->lastHit, g_Score(0x002efa4c));  /* OTHER_KILL */
	}
	BCarEntity_AddScore(self, g_Score(0x002efa48));             /* SELF_KILL */
}

/* restart the grow effect's animation `anim` (looped or not) */
static void bcb_entity_grow_anim(struct BCarEntity *self, struct BAnim *anim, int loop)
{
	if (self->extraInst != NULL)
		baFlushAnimations(self->extraInst, self->extraInst->root, 1, 0);
	if (anim != NULL)
		baQueueAnimation(self->extraInst, self->extraInst->root, anim, 0x400, -1, loop, NULL, NULL);
}

/* 0x0009d100  (file inferred: CBCar_Entity.cpp)
Moves every entity by `dt` frames: animations, the speed-scale puffs, turning to the target
heading, gravity, thrust and friction, the speed limits, up to 10 collision passes, the
flames (the grow effect and its explosion), lining up with the ground or rubbing on the
arena edge, the hit timeout, falling below KILLHEIGHT (scores, death, respawn), the king
spot and the keeper scores, following the carrier, and the actor instances.
`control`: the game is running. */
void BCarEntities_Update(float dt, int control)
{
	struct BCarEntity *self;
	struct BCarEntity *carrier;
	struct BActorInstance *inst;
	float *pos;
	float diff;
	float old;
	float speed;
	float limit;
	float x;
	float z;
	float flame;
	float friction;
	float strength;
	float fwd[4];
	float axis[4];
	float n[4];
	float puff[4];
	float m[16];
	float distance;
	float frames;
	float factor;
	float offset;
	unsigned int passes;
	int hitWorld;
	int index;
	int lead;
	int r5;
	float a;

	for (self = g_Entities; self != NULL; self = self->next)
	{
		baUpdateAnimations(self->inst, self->inst->root, dt, NULL);
		index = bcb_player_of(self);
		self->oldVel[0] = self->vel[0];
		self->oldVel[1] = self->vel[1];
		self->oldVel[2] = self->vel[2];
		self->oldVel[3] = 1.0f;
		pos = self->pos;
		self->oldPos[0] = self->pos[0];
		self->oldPos[1] = self->pos[1];
		self->oldPos[2] = self->pos[2];
		self->oldPos[3] = 1.0f;

		/* a powerup speed scale: random puffs while it fades back to 1 */
		if (self->speedScale != 1.0f)
		{
			if (self->speedScale < 1.0f)
			{
				int r1 = bcb_rand();
				int r2 = bcb_rand();
				int r3 = bcb_rand();
				int r4 = bcb_rand();

				r5 = bcb_rand();
				factor = (float)r1 * 3.05185e-06f + 0.5f;              /* 0x00265744, 0x002536d0 */
				puff[0] = (float)r2 * 0.000305185f + factor * self->vel[0]; /* 0x00265808 */
				puff[1] = (float)r3 * 0.000915555f + factor * self->vel[1]; /* 0x00265804 */
				puff[2] = (float)r4 * 0.000305185f + factor * self->vel[2];
				puff[3] = 1.0f;
				if (r5 % 100 < BCB_FTOL((1.0f - self->speedScale) * 250.0f))   /* 0x00262d34 */
					BCarEffects_Burst(BCB_EFFECTS, 4, pos, puff, 0, -1);
			}
			self->speedScale = self->speedScale - (self->speedScale - 1.0f) * 0.016666668f; /* 0x00262b7c */
		}

		/* turn to the target heading, at most ENTITYROT */
		if (self->heading != self->targetHeading && control != 0)
		{
			diff = self->targetHeading - self->heading;
			if (diff < 0.0f)
				diff = diff + 6.2831855f;                          /* 0x00255930 */
			if (6.2831855f < diff)
				diff = diff - 6.2831855f;
			if (3.1415925f < diff)                                 /* 0x00256750 */
				diff = diff - 6.2831855f;
			if (g_EntityRot < diff)
				diff = g_EntityRot;
			if (diff < -g_EntityRot)
				diff = -g_EntityRot;
			old = self->heading;
			self->heading = diff + old;
			self->heading = CLMath_WrapAngle(diff + old);
		}

		/* forces */
		self->accel[0] = 0.0f;
		self->accel[1] = 0.0f;
		self->accel[2] = 0.0f;
		self->accel[3] = 1.0f;
		self->accel[0] = g_WorldGravity[0] * self->mass + self->accel[0];
		self->accel[1] = g_WorldGravity[1] * self->mass + self->accel[1];
		self->accel[2] = g_WorldGravity[2] * self->mass + self->accel[2];
		self->accel[0] = self->accel[0] + self->force[0];
		self->accel[1] = self->accel[1] + self->force[1];
		self->accel[2] = self->accel[2] + self->force[2];
		self->accel[0] = dt * self->accel[0];
		self->accel[1] = dt * self->accel[1];
		self->accel[2] = dt * self->accel[2];
		x = self->invMass * self->accel[0] + self->vel[0];
		self->vel[0] = x;
		self->vel[1] = self->invMass * self->accel[1] + self->vel[1];
		z = self->invMass * self->accel[2] + self->vel[2];
		self->vel[2] = z;
		if (self->onGround != 0)
		{
			if (self->active == 0)
			{
				self->vel[0] = 0.0f;
				self->vel[1] = 0.0f;
				self->vel[2] = 0.0f;
				self->vel[3] = 1.0f;
			}
			else
			{
				self->vel[0] = x - x * g_WorldFriction;
				self->vel[2] = z - z * g_WorldFriction;
			}
		}
		if (self->boosting != 0)
		{
			BCarTimer_Update(&self->boostTimer);
			if (BCarTimer_GetRemaining(&self->boostTimer) == 0.0f)
				self->boosting = 0;
		}
		if (0.0f < self->spinTime)
		{
			self->spinTime = self->spinTime - dt * 0.016666668f;
			if (self->spinTime < 0.0f)
				self->spinTime = 0.0f;
		}
		if (0.0f < self->respawnTime)
		{
			self->respawnTime = self->respawnTime - dt * 0.016666668f;
			if (self->respawnTime < 0.0f)
				self->respawnTime = 0.0f;
		}

		/* speed limits */
		speed = sqrtf(self->vel[2] * self->vel[2] + self->vel[1] * self->vel[1] +
			self->vel[0] * self->vel[0]);
		if (self->boosting == 0)
		{
			if (BCarWorld_GetMaxSpeed() < speed)
			{
				limit = BCarWorld_GetMaxSpeed() / speed;
				self->vel[0] = limit * self->vel[0];
				self->vel[1] = limit * self->vel[1];
				self->vel[2] = limit * self->vel[2];
			}
		}
		else if (BCarWorld_GetBoostValue() < speed)
		{
			limit = BCarWorld_GetBoostValue() / speed;
			self->vel[0] = limit * self->vel[0];
			self->vel[1] = limit * self->vel[1];
			self->vel[2] = limit * self->vel[2];
		}
		if (self->onGround == 0 && g_MaxVertVelocity < self->vel[1])
			self->vel[1] = g_MaxVertVelocity;
		if (g_WorldFrozen != 0 || self->frozen != 0)
		{
			self->vel[0] = 0.0f;
			self->vel[1] = 0.0f;
			self->vel[2] = 0.0f;
			self->vel[3] = 1.0f;
		}
		self->lastVel[0] = self->vel[0];
		self->lastVel[1] = self->vel[1];
		self->lastVel[2] = self->vel[2];
		self->lastVel[3] = 1.0f;
		self->lastPos[0] = self->pos[0];
		self->lastPos[1] = self->pos[1];
		self->lastPos[2] = self->pos[2];
		self->lastPos[3] = 1.0f;
		if (0 < self->onGround)
			self->onGround--;

		/* collisions, then the move */
		passes = 0;
		hitWorld = 0;
		do
		{
			passes++;
			self->collisionFlags = 0;
			if (BCarEntity_CollideWorld(self) != 0)
			{
				self->collisionFlags |= BCAR_HIT_WORLD;
				hitWorld = 1;
			}
			if (BCarEntity_CollideEntities(self) != 0)
				self->collisionFlags |= BCAR_HIT_ENTITY;
			if (self->collisionFlags == 0)
				break;
			BCarEntity_Bounce(self, control);
			if (self->collisionFlags == 0)
				break;
		} while (passes < 10);
		if (self->collisionFlags == 0 && self->active != 0)
		{
			pos[0] = pos[0] + self->vel[0];
			self->pos[1] = self->pos[1] + self->vel[1];
			self->pos[2] = self->pos[2] + self->vel[2];
		}
		self->force[0] = 0.0f;
		self->force[1] = 0.0f;
		self->force[2] = 0.0f;
		self->force[3] = 1.0f;

		/* the flames: start (or keep) the grow effect */
		if (self->active != 0 && (flame = BCarFlame_Hit(BCB_FLAME, pos, self->radius), 0.0f < flame))
		{
			self->stageTime[1] = flame;
			self->stageTime[0] = self->animTime[0];
			self->stageTime[2] = self->animTime[2];
			if (self->growing == 0)
			{
				self->growing = 1;
				self->stage = 0;
				if (self->extraInst != NULL)
					baFlushAnimations(self->extraInst, self->extraInst->root, 1, 0);
				if (self->animGrow != NULL)
					baQueueAnimation(self->extraInst, self->extraInst->root, self->animGrow, 0x400,
						-1, 0, NULL, NULL);
			}
			else if (self->stage == 2)
			{
				bcb_entity_grow_anim(self, self->animIdle, 1);
				self->stage = 1;
			}
			BCarTimer_SetDuration(&self->growTimer, self->stageTime[self->stage]);
			BCarTimer_Start(&self->growTimer);
		}
		if (self->growing != 0)
		{
			if (self->extraInst != NULL)
				baUpdateAnimations(self->extraInst, self->extraInst->root, dt, NULL);
			BCarTimer_Update(&self->growTimer);
			if (BCarTimer_GetRemaining(&self->growTimer) == 0.0f)
			{
				self->stage++;
				if (self->stage == 3)
				{
					self->growing = 0;
					if (self->extraInst != NULL)
						baFlushAnimations(self->extraInst, self->extraInst->root, 1, 0);
					BCB_CALL(FUN_0008ce30, void (*)(void *, float *, float, float, float, int, ...))(
						BCB_EFFECTS, pos, 26.0f, 26.0f, 10.0f, 6, 4, 5, 6, 7, 8);
					BCarSound_Play(BCB_SOUND, 0x17, 0xff, 0, -1);
				}
				else
				{
					BCarTimer_SetDuration(&self->growTimer, self->stageTime[self->stage]);
					BCarTimer_Start(&self->growTimer);
					if (self->stage == 1)
						bcb_entity_grow_anim(self, self->animIdle, 1);
					else if (self->stage == 2)
						bcb_entity_grow_anim(self, self->animRumble, 0);
				}
			}
		}

		/* line up with the ground, or rub on the arena's edge */
		if (hitWorld)
		{
			axis[0] = 0.0f;
			axis[1] = 0.0f;
			axis[2] = 1.0f;
			axis[3] = 1.0f;
			bmVectorRotateXYZ(fwd, axis, self->rot[0], self->heading + 3.1415925f, self->rot[2]);
			bmVectorNormalise(fwd, fwd);
			if ((self->groundNormal[2] + self->groundNormal[0]) * 0.0f + self->groundNormal[1] <= 0.5f)
			{
				friction = fabsf(fwd[0] * self->groundNormal[0] + fwd[2] * self->groundNormal[2] +
					fwd[1] * self->groundNormal[1]) * g_WorldEdgeFriction;
				self->force[0] = self->force[0] - friction * self->vel[0];
				self->force[2] = self->force[2] - friction * self->vel[2];
				speed = sqrtf(self->vel[2] * self->vel[2] + self->vel[1] * self->vel[1] +
					self->vel[0] * self->vel[0]);
				if (self->type == BCAR_ENT_CAR)
					strength = speed / BCarPlayer_GetMaxSpeed(self->player);
				else
					strength = speed / BCarWorld_GetMaxSpeed();
				if (!(strength <= 1.0f))
					strength = 1.0f;
				if (index != -1)
					BCarGame_Rumble(index, 0, strength);
				BCarEffects_Spark(BCB_EFFECTS, 2, self->groundPoint, friction * 20.0f,
					friction * 30.0f, friction * 5.0f, BCB_FTOL(friction * 20.0f)); /* 0x00255530, 0x002553a8, 0x002536c4 */
			}
			else
			{
				self->groundNormal[3] = 1.0f;
				n[0] = self->groundNormal[0];
				n[1] = self->groundNormal[1];
				n[2] = self->groundNormal[2];
				self->rot[0] = BCarMath_TurnTo(self->rot[0],
					CLMath_WrapAngle(BCarMath_Atan2(n[2], n[1])), 0.06283186f); /* 0x3d80adfc */
				self->rot[2] = BCarMath_TurnTo(self->rot[2],
					CLMath_WrapAngle(-BCarMath_Atan2(n[0], n[1])), 0.06283186f);
				self->onGround = 5;
				self->falling = 0;
			}
		}
		else if (self->falling != 0 && control != 0)
		{
			self->rot[0] = BCarMath_TurnTo(self->rot[0], self->rotVel[0] + self->rot[0], 0.06283186f);
			self->rot[2] = BCarMath_TurnTo(self->rot[2], self->rotVel[2] + self->rot[2], 0.06283186f);
		}

		/* the last hit is forgotten after TIMEOUT */
		if (self->lastHit != NULL)
		{
			self->lastHitFrames++;
			frames = (float)self->lastHitFrames;
			if (self->lastHitFrames < 0)
				frames = frames + 4.2949673e+09f;                  /* 0x00253c50 */
			if (g_HitTimeout <= frames / BCarTimer_GetFrameTime(&g_BCarTimer.timer))
				self->lastHit = NULL;
		}
		if (self->respawnBlink != 0 && self->respawnBlinkEnd <= BCB_NOW())
			self->respawnBlink = 0;
		lead = g_LeadPlayer != NULL && g_LeadPlayer->entity == self;

		/* fell off the arena */
		if (self->active != 0 && self->pos[1] <= g_KillHeight)
		{
			bcb_entity_killed(self, lead);
			if (g_WorldSpawns == NULL)
			{
				BCarSound_Play(BCB_SOUND, (bcb_rand() & 1) + 0xf, 0xff, 0, -1);
				self->active = 0;
				self->dead = 1;
				self->falling = 0;
				self->deathTime = BCarTimer_GetUpdates(&g_BCarTimer.timer);
				self->rot[0] = 0.0f;
				self->rot[1] = 0.0f;
				self->rot[2] = 0.0f;
				self->rot[3] = 1.0f;
				BCarEntity_DropCarried(self);
			}
			else
			{
				if (g_Lives != -1)
				{
					self->lives--;
					if (self->lives == 0)
					{
						BCarSound_Play(BCB_SOUND, 0xe, 0xff, 0, -1);
						BCarEntity_Kill(self);
						goto scored;
					}
				}
				BCarSound_Play(BCB_SOUND, (bcb_rand() & 1) + 0xf, 0xff, 0, -1);
				BCarEntity_Respawn(self);
			}
		}
scored:
		/* the king spot and the keeper */
		if (self->type == BCAR_ENT_CAR)
		{
			distance = bmVectorDistance(pos, g_KingSpot);
			if (distance < g_GainRadius && control != 0)
			{
				self->kingCounter++;
				if (BCarTimer_GetFrameTime(&g_BCarTimer.timer) <= (float)self->kingCounter)
				{
					self->kingCounter = 0;
					BCarEntity_AddScore(self, lead ? g_Score(0x002efa74) : g_Score(0x002efa38)); /* KING_GAIN */
				}
			}
			if (g_LoseRadius < distance && control != 0)
			{
				self->kingCounter--;
				if (self->kingCounter < 0)
				{
					self->kingCounter = BCB_FTOL(BCarTimer_GetFrameTime(&g_BCarTimer.timer));
					BCarEntity_AddScore(self, lead ? g_Score(0x002efa78) : g_Score(0x002efa3c)); /* KING_LOSE */
				}
			}
			if (self->carried != NULL && control != 0)
			{
				self->carryCounter++;
				if (BCarTimer_GetFrameTime(&g_BCarTimer.timer) <= (float)self->carryCounter)
				{
					self->carryCounter = 0;
					BCarEntity_AddScore(self, lead ? g_Score(0x002efa7c) : g_Score(0x002efa40)); /* KEEPER_GAIN */
				}
			}
		}

		/* follow the carrier */
		carrier = self->carrier;
		if (carrier != NULL)
		{
			pos[0] = carrier->pos[0];
			self->pos[1] = carrier->pos[1];
			self->pos[2] = carrier->pos[2];
			self->pos[3] = carrier->pos[3];
			self->rot[0] = self->carrier->rot[0];
			self->rot[2] = self->carrier->rot[2];
		}

		/* the actor instances */
		inst = self->inst;
		inst->pos[0] = pos[0];
		inst->pos[1] = self->pos[1];
		inst->pos[2] = self->pos[2];
		inst->pos[3] = self->pos[3];
		if (self->pickup == 0)
		{
			carrier = self->carrier;
			if (carrier != NULL)
				self->inst->pos[1] = carrier->drawOffset + carrier->radius + self->inst->pos[1];
			offset = self->drawOffset;
		}
		else
			offset = self->carriedOffset;
		self->inst->pos[1] = offset + self->inst->pos[1];
		bmMatrixRotationXZY(m, self->rot[0], self->heading + 3.1415925f, self->rot[2]);
		bmMatrixToQuaternion(self->inst->rot, m);
		if (self->extraInst != NULL)
		{
			self->extraInst->pos[0] = self->inst->pos[0];
			self->extraInst->pos[1] = self->inst->pos[1];
			self->extraInst->pos[2] = self->inst->pos[2];
			self->extraInst->pos[3] = self->inst->pos[3];
			self->extraInst->rot[0] = self->inst->rot[0];
			self->extraInst->rot[1] = self->inst->rot[1];
			self->extraInst->rot[2] = self->inst->rot[2];
			self->extraInst->rot[3] = self->inst->rot[3];
		}
		inst = self->inst;
		self->instPos[0] = inst->pos[0];
		self->instPos[1] = inst->pos[1];
		self->instPos[2] = inst->pos[2];
		self->instPos[3] = inst->pos[3];
		self->instPos[1] = self->radius * 0.25f + self->instPos[1]; /* 0x00257b88 */
		self->instRot[0] = inst->rot[0];
		self->instRot[1] = inst->rot[1];
		self->instRot[2] = inst->rot[2];
		self->instRot[3] = inst->rot[3];
		if (self->falling == 0)
			self->alpha = 1.0f;
		else
		{
			a = 1.0f - -self->inst->pos[1] / -g_KillHeight;
			self->alpha = a;
			if (1.0f < a)
				self->alpha = 1.0f;
			if (self->alpha < 0.0f)
				self->alpha = 0.0f;
		}
	}
	BCB_CALL(FUN_00082b20, void (*)(float *, float *, float *, float *))(
		g_BCarPlayers[0].entity->pos, g_BCarPlayers[1].entity->pos,
		g_BCarPlayers[2].entity->pos, g_BCarPlayers[3].entity->pos);
}
