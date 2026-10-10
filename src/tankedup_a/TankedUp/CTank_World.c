/*
TankedUp/CTank_World.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_World.cpp

CTank_World (CTank_Game +0xc18, "worldSystem"): the world objects read from the variation's
OBJECTFILE: "OBJECT {" (a set or an actor with a collision mesh, animated textures, an
animation, sounds, health, chains that switch other objects), "BOUNDARY {" (a collision
mesh whose inside sets flags), "CHECKPOINT {" (a race checkpoint) and "SETTINGS {"
(GRAVITY, LAPS; LAPS 0 makes a battle world). It draws the objects, runs their animations
and chains, and answers the collision queries of the tanks, shells and debris.

0x001cbcd0 and 0x001cd920 are tagged with this file; the functions from 0x001c9fb0 to
0x001cd920 are placed here because of their "CTank_World::" strings or because they work
on this layout. 0x001c9f50 (an animation callback) is missing from the function table.
*/

#include "tankedup_a.h"
#include "engine_misc_maths.h"   /* struct BCollHit, BRay, BSphere (engine_misc) */

/* a world object (0x268 bytes, from the heap; BOUNDARY objects use the same record with
   type 3) */
struct TuWorldObject {
	int health;                  /* +0x000 HEALTH: hits it takes */
	int hits;                    /* +0x004 hits left */
	int active;                  /* +0x008 ACTIVE */
	int animCount;               /* +0x00c ANIMCOUNT: 0 loops */
	int animLeft;                /* +0x010 */
	int decal;                   /* +0x014 DECAL: drawn with the decal states */
	int bounce;                  /* +0x018 BOUNCE */
	int frameMax;                /* +0x01c FRAME_MAX (-1 none) */
	int frameMin;                /* +0x020 FRAME_MIN (-1 none) */
	int lockLastFrame;           /* +0x024 LOCKLASTTEXFRAME */
	int started;                 /* +0x028 switched on this frame */
	int sound;                   /* +0x02c SOUND: 1-3 a world sound when it starts */
	int wasActive;               /* +0x030 */
	unsigned int flags;          /* +0x034 FLAGS: 1 solid, 8, 0x10 second pass, 0x20/0x40 group, 0x80 red */
	int warnFlags;               /* +0x038 WARNFLAGS: the group it belongs to */
	float animSpeed;             /* +0x03c ANIMSPEED */
	float life;                  /* +0x040 LIFE: seconds it stays active (0 forever) */
	float restitution;           /* +0x044 RESTITUTION */
	float friction;              /* +0x048 FRICTION */
	float step;                  /* +0x04c the last frame step */
	float pos[4];                /* +0x050 POSITION */
	float rot[4];                /* +0x060 ORIENTATION (radians) */
	char name[0x40];             /* +0x070 NAME */
	char chain[0x40];            /* +0x0b0 CHAIN / TEXTURECHAIN: the objects it switches */
	char animSegment[0x40];      /* +0x0f0 ANIMSEGMENT */
	char animTexture[0x40];      /* +0x130 ANIMTEXTURE */
	char texture1[0x40];         /* +0x170 TANIM1 */
	char texture2[0x40];         /* +0x1b0 TANIM2 */
	int chainKind;               /* +0x1f0 1 CHAIN (when it dies), 2 TEXTURECHAIN (when its texture ends) */
	int unknown1f4;              /* +0x1f4 */
	unsigned char timer[0x40];   /* +0x1f8 its timer (struct TuTimer without the last word) */
	int type;                    /* +0x238 0 none, 1 set, 2 actor, 3 boundary */
	void *set;                   /* +0x23c struct BSet * */
	void *actor;                 /* +0x240 struct BActor * */
	void *instance;              /* +0x244 struct BActorInstance * */
	void *collision;             /* +0x248 struct BCollMesh * */
	void *anim;                  /* +0x24c the animation segment */
	void *texAnim;               /* +0x250 struct bdTexture * */
	void *tex1;                  /* +0x254 */
	void *tex2;                  /* +0x258 */
	void *menu;                  /* +0x25c the tweak menu of a solid object */
	struct TuWorldObject *next;  /* +0x260 */
	int unknown264;              /* +0x264 */
};
OF_ASSERT_OFFSET(struct TuWorldObject, pos, 0x50);
OF_ASSERT_OFFSET(struct TuWorldObject, name, 0x70);
OF_ASSERT_OFFSET(struct TuWorldObject, chainKind, 0x1f0);
OF_ASSERT_OFFSET(struct TuWorldObject, timer, 0x1f8);
OF_ASSERT_OFFSET(struct TuWorldObject, type, 0x238);
OF_ASSERT_OFFSET(struct TuWorldObject, next, 0x260);
OF_ASSERT_SIZE(struct TuWorldObject, 0x268);

/* a checkpoint (0x30 bytes) */
struct TuCheckpoint {
	int oneWay;                  /* +0x00 ONEWAY */
	int boundary;                /* +0x04 BOUNDARY: crossing means being inside */
	float pos[4];                /* +0x08 */
	float rot[4];                /* +0x18 */
	void *collision;             /* +0x28 */
	struct TuCheckpoint *next;   /* +0x2c */
};
OF_ASSERT_SIZE(struct TuCheckpoint, 0x30);

/* the world system (CTank_Game +0xc18, 0x20 bytes) */
struct TuWorld {
	int changed;                 /* +0x00 a chain switched objects this frame */
	int checkpointCount;         /* +0x04 */
	int laps;                    /* +0x08 LAPS (0: a battle world) */
	float gravity;               /* +0x0c GRAVITY */
	struct TuWorldObject *objects; /* +0x10 */
	struct TuCheckpoint *checkpoints; /* +0x14 */
	void *sound;                 /* +0x18 */
	struct TuLight *lights;      /* +0x1c */
};
OF_ASSERT_SIZE(struct TuWorld, 0x20);

/* the debug object's render state 3 value for the world, and the world tweak menu root */
#define TU_DBG_WORLD_RS3 TU_VAR(int, 0x003151a8)
#define TU_DBG_WORLD_MENU TU_VAR(void *, 0x003151b8)

#define TU_HITS 30

static float world_elapsed(struct TuWorldObject *obj)
{
	return TU_Timer_GetElapsed(obj->timer);
}

/* switches `obj` on (and starts it) or off (and stops its animation) */
static void world_toggle(struct TuWorld *world, struct TuWorldObject *obj)
{
	if (obj->active == 0) {
		obj->started = 1;
		obj->active = 1;
		CTankWorld_StartObject(world, obj);
	} else {
		obj->active = 0;
		if (obj->anim != NULL)
			baFlushAnimations(obj->instance, (struct BActorNode *)TU_PTR(obj->instance, 0x134), 1, 0);
	}
}

/* switches every object called `name` */
static void world_toggle_named(struct TuWorld *world, const char *name)
{
	struct TuWorldObject *obj;

	for (obj = CTankWorld_FindObject(world, name, NULL); obj != NULL;
		obj = CTankWorld_FindObject(world, name, obj))
		world_toggle(world, obj);
}

/* 0x001c9f50  TankedUp (not in the function table: an animation callback)
When an object's animation ends, it plays it again until ANIMCOUNT runs out. */
int FUN_001c9f50(int reason, void *instance, void *node, struct BAnim *anim, void *context)
{
	struct TuWorldObject *obj = (struct TuWorldObject *)context;

	(void)instance;
	(void)node;
	(void)anim;
	if (reason == 0) {
		obj->animLeft--;
		if (obj->animLeft > 0)
			baQueueAnimation((struct BActorInstance *)obj->instance,
				(struct BActorNode *)TU_PTR(obj->instance, 0x134), (struct BAnim *)obj->anim,
				TU_FTOL(obj->animSpeed * 1024.0f), -1, 0, (int (*)())FUN_001c9f50, obj);
	}
	return 1;
}

/* 0x001c9fb0  TankedUp (gap) */
void *CTankWorld_Ctor(void *self)
{
	struct TuWorld *world = (struct TuWorld *)self;

	world->objects = NULL;
	world->checkpoints = NULL;
	world->gravity = 0.0f;
	world->checkpointCount = 0;
	world->laps = 0;
	return self;
}

/* 0x001c9fd0  TankedUp (gap)
The world object's destructor: its timer's (empty, 0x001d5950). */
void CTankWorldObject_Dtor(void *obj)
{
	(void)obj;
}

/* 0x001c9fe0  TankedUp (gap)
Group `group` (FLAGS 0x20 -> 1, 0x40 -> 2): its objects (WARNFLAGS) start; objects of
other groups stop. */
void CTankWorld_SwitchGroup(void *self, int group)
{
	struct TuWorldObject *obj;

	for (obj = ((struct TuWorld *)self)->objects; obj != NULL; obj = obj->next) {
		if (obj->warnFlags == 0)
			continue;
		if (obj->warnFlags == group && obj->active == 0) {
			obj->started = 1;
			obj->active = 1;
			CTankWorld_StartObject(self, obj);
		}
		if (obj->warnFlags != group && obj->active != 0) {
			obj->active = 0;
			if (obj->anim != NULL)
				baFlushAnimations(obj->instance, (struct BActorNode *)TU_PTR(obj->instance, 0x134), 1, 0);
		}
	}
}

/* 0x001ca060  TankedUp (gap)
The next object called `name` after `after` (NULL: the first). */
struct TuWorldObject *CTankWorld_FindObject(void *self, const char *name, struct TuWorldObject *after)
{
	struct TuWorldObject *obj = after == NULL ? ((struct TuWorld *)self)->objects : after->next;

	for (; obj != NULL; obj = obj->next)
		if (_stricmp(obj->name, name) == 0)
			return obj;
	return NULL;
}

/* the frame of an object's animated texture by its timer: FRAME_MIN..FRAME_MAX (a loop, or
held at the end with LOCKLASTTEXFRAME), backwards when FRAME_MIN > FRAME_MAX */
static void world_texture_frame(struct TuWorldObject *obj)
{
	int frame;

	if (obj->frameMin == -1 && obj->frameMax == -1) {
		if (obj->lockLastFrame != 0)
			TUB_Texture_SetTimeOnce(obj->texAnim, TU_FTOL(world_elapsed(obj) * 1000.0f));
		else
			bdTextureSetTime((struct bdTexture *)obj->texAnim, TU_FTOL(world_elapsed(obj) * 1000.0f));
		return;
	}
	frame = obj->frameMax;
	if (obj->frameMin < frame) {
		frame = TU_FTOL(world_elapsed(obj) * obj->animSpeed * 6.0f);
		if (obj->lockLastFrame != 0) {
			frame = frame + obj->frameMin;
			if (frame > obj->frameMax)
				frame = obj->frameMax;
		} else {
			frame = frame % (obj->frameMax - obj->frameMin + 1) + obj->frameMin;
		}
	} else if (frame < obj->frameMin) {
		if (obj->lockLastFrame != 0) {
			frame = obj->frameMin - TU_FTOL(world_elapsed(obj) * obj->animSpeed * 6.0f + 1.0f);
			if (frame < obj->frameMax)
				frame = obj->frameMax;
		} else {
			int min = obj->frameMin;

			frame = min - TU_FTOL(world_elapsed(obj) * obj->animSpeed * 6.0f + 1.0f)
				% (min - obj->frameMax);
		}
	}
	bdTextureSetFrame((struct bdTexture *)obj->texAnim, frame);
}

/* the object's world matrix */
static void world_object_matrix(float *out, const float *pos, const float *rot)
{
	float rotation[16];
	float translation[16];

	bmMatrixRotationXYZ(rotation, rot[0], rot[1], rot[2]);
	bmMatrixTranslation(translation, pos[0], pos[1], pos[2]);
	bmMatrixMultiplyCopy(out, translation, rotation);
}

/* 0x001ca0b0  TankedUp (gap)
Draws the world's first pass ("sky": the animated textures run on `time`), unless the
debug object turns it off. */
void CTankWorld_DrawSky(void *self, float time)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuWorldObject *obj;
	float matrix[16];

	bdSetProjectionMode(1);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(3, TU_DBG_WORLD_RS3, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	baSetBrightness(0x100);
	if ((TUB_Debug_GetDrawFlags(TU_ADDR(0x003150b0)) & 1) != 0)
		return;
	for (obj = world->objects; obj != NULL; obj = obj->next) {
		if (obj->active == 0)
			continue;
		if (obj->tex1 != NULL)
			bdTextureSetTime((struct bdTexture *)obj->tex1, TU_FTOL(time * 1000.0f));
		if (obj->tex2 != NULL)
			bdTextureSetTime((struct bdTexture *)obj->tex2, TU_FTOL(time * 1000.0f));
		if (obj->texAnim != NULL)
			world_texture_frame(obj);
		if ((obj->flags & 8) != 0)
			bdSetRenderState(10, 1, 0);
		if ((obj->flags & 0x80) != 0) {
			CTankLights_SetAmbient(0xff, 0, 0);
			bdSetRenderState(3, 1, 0);
		}
		if (obj->type == 1) {
			world_object_matrix(matrix, obj->pos, obj->rot);
			bdSetWorldMatrix(matrix);
			if (obj->decal != 0) {
				BCarRender_SetWaterStates();
				TUB_Decal_StencilOn();
				baDrawSet((struct BSet *)obj->set, 1);
				FUN_0018e640();
			} else {
				baDrawSet((struct BSet *)obj->set, 1);
			}
		} else if (obj->type == 2) {
			bdSetWorldMatrix(DAT_002855f0);
			baDrawActorInstance((struct BActorInstance *)obj->instance, 7);
		}
		if ((obj->flags & 0x80) != 0) {
			CTankLights_ApplyAmbient(world->lights);
			bdSetRenderState(3, TU_DBG_WORLD_RS3, 0);
		}
		if (obj->texAnim != NULL)
			bdTextureSetFrame((struct bdTexture *)obj->texAnim, 0);
		if (obj->tex1 != NULL)
			bdTextureSetFrame((struct bdTexture *)obj->tex1, 0);
		if (obj->tex2 != NULL)
			bdTextureSetFrame((struct bdTexture *)obj->tex2, 0);
		if ((obj->flags & 8) != 0)
			bdSetRenderState(10, 0, 0);
	}
}

/* 0x001ca470  TankedUp (gap)
Draws the world's main passes (4, and 2 for FLAGS 0x10), then the debug collision meshes
(objects with debug flag 1, boundaries and checkpoints with debug flag 2). */
void CTankWorld_Draw(void *self)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuWorldObject *obj;
	struct TuCheckpoint *checkpoint;
	float matrix[16];

	bdSetProjectionMode(1);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(3, TU_DBG_WORLD_RS3, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	baSetBrightness(0x100);
	if ((TUB_Debug_GetDrawFlags(TU_ADDR(0x003150b0)) & 1) == 0) {
		for (obj = world->objects; obj != NULL; obj = obj->next) {
			if (obj->active == 0)
				continue;
			if (obj->texAnim != NULL)
				world_texture_frame(obj);
			if ((obj->flags & 8) != 0)
				bdSetRenderState(10, 1, 0);
			if ((obj->flags & 0x80) != 0) {
				CTankLights_SetAmbient(0xff, 0, 0);
				bdSetRenderState(3, 1, 0);
			}
			if (obj->type == 1) {
				world_object_matrix(matrix, obj->pos, obj->rot);
				bdSetWorldMatrix(matrix);
				baDrawSet((struct BSet *)obj->set, 4);
				if ((obj->flags & 0x10) != 0)
					baDrawSet((struct BSet *)obj->set, 2);
			} else if (obj->type == 2) {
				bdSetWorldMatrix(DAT_002855f0);
				baDrawActorInstance((struct BActorInstance *)obj->instance, 4);
				if ((obj->flags & 0x10) != 0)
					baDrawActorInstance((struct BActorInstance *)obj->instance, 2);
			}
			if ((obj->flags & 0x80) != 0) {
				CTankLights_ApplyAmbient(world->lights);
				bdSetRenderState(3, TU_DBG_WORLD_RS3, 0);
			}
			if (obj->texAnim != NULL)
				bdTextureSetTime((struct bdTexture *)obj->texAnim, 0);
			if ((obj->flags & 8) != 0)
				bdSetRenderState(10, 0, 0);
		}
	}
	for (obj = world->objects; obj != NULL; obj = obj->next) {
		if (obj->collision == NULL)
			continue;
		if (obj->type == 3) {
			if ((TUB_Debug_GetDrawFlags(TU_ADDR(0x003150b0)) & 2) == 0)
				continue;
		} else if ((TUB_Debug_GetDrawFlags(TU_ADDR(0x003150b0)) & 1) == 0) {
			continue;
		}
		if (obj->active != 0) {
			world_object_matrix(matrix, obj->pos, obj->rot);
			bdSetWorldMatrix(matrix);
			TUB_Debug_DrawMesh(obj->collision, 0x40404040, 0);
		}
	}
	if ((TUB_Debug_GetDrawFlags(TU_ADDR(0x003150b0)) & 2) != 0) {
		for (checkpoint = world->checkpoints; checkpoint != NULL; checkpoint = checkpoint->next) {
			world_object_matrix(matrix, checkpoint->pos, checkpoint->rot);
			bdSetWorldMatrix(matrix);
			TUB_Debug_DrawMesh(checkpoint->collision, 0x40404040, 0);
		}
	}
}

/* 0x001ca9a0  TankedUp (gap)
Pauses every object's timer. */
void CTankWorld_Pause(void *self)
{
	struct TuWorldObject *obj;

	for (obj = ((struct TuWorld *)self)->objects; obj != NULL; obj = obj->next)
		CTankTimer_Pause((struct TuTimer *)obj->timer);
}

/* 0x001ca9d0  TankedUp (gap)
Resumes every object's timer. */
void CTankWorld_Resume(void *self)
{
	struct TuWorldObject *obj;

	for (obj = ((struct TuWorld *)self)->objects; obj != NULL; obj = obj->next)
		CTankTimer_StartOrResume((struct TuTimer *)obj->timer);
}

/* 0x001caa00  TankedUp (gap; no `this`)
The matrices into an object's space: `toLocal` translates by -pos, `rotate` turns by -rot,
`toWorld` is the inverse of `rotate`. */
void CTankWorld_ObjectMatrices(float *pos, float *rot, float *toLocal, float *toWorld, float *rotate)
{
	bmMatrixRotationXYZ(rotate, -rot[0], -rot[1], -rot[2]);
	bmMatrixTranslation(toLocal, -pos[0], -pos[1], -pos[2]);
	bmMatrixInverseAffine(toWorld, rotate);
}

/* 0x001caa70  TankedUp (gap)
GRAVITY. */
float CTankWorld_GetGravity(void *self)
{
	return ((struct TuWorld *)self)->gravity;
}

/* 0x001caa80  TankedUp (gap)
LAPS (0: a battle world). */
int CTankWorld_GetLaps(void *self)
{
	return ((struct TuWorld *)self)->laps;
}

/* 0x001caa90  TankedUp (gap)
Clears a new world object. */
void CTankWorld_ClearObject(void *self, struct TuWorldObject *obj)
{
	(void)self;
	obj->hits = 0;
	obj->health = 0;
	obj->active = 1;
	obj->flags = 0;
	obj->warnFlags = 0;
	obj->name[0] = 0;
	obj->chain[0] = 0;
	obj->chainKind = 0;
	obj->animSegment[0] = 0;
	obj->animTexture[0] = 0;
	obj->texture1[0] = 0;
	obj->texture2[0] = 0;
	obj->pos[0] = 0.0f;
	obj->pos[1] = 0.0f;
	obj->pos[2] = 0.0f;
	obj->pos[3] = 1.0f;
	obj->rot[3] = 1.0f;
	obj->rot[0] = 0.0f;
	obj->rot[1] = 0.0f;
	obj->rot[2] = 0.0f;
	obj->animSpeed = 1.0f;
	obj->type = 0;
	obj->set = NULL;
	obj->actor = NULL;
	obj->instance = NULL;
	obj->collision = NULL;
	obj->anim = NULL;
	obj->animCount = 0;
	obj->animLeft = 0;
	obj->life = 0.0f;
	obj->restitution = 0.0f;
	obj->friction = 0.0f;
	obj->decal = 0;
	obj->bounce = 0;
	obj->sound = 0;
	obj->texAnim = NULL;
	obj->tex1 = NULL;
	obj->tex2 = NULL;
	obj->frameMax = -1;
	obj->frameMin = -1;
	obj->lockLastFrame = 0;
}

/* 0x001cab50  TankedUp (gap)
Starts an object: its hits, its life timer, its sound (SOUND 1-3), its animation (looped,
or ANIMCOUNT times), and its group (FLAGS 0x20 / 0x40). */
void CTankWorld_StartObject(void *self, struct TuWorldObject *obj)
{
	struct TuWorld *world = (struct TuWorld *)self;
	float origin[4];
	int kind, volume;

	obj->hits = obj->health;
	CTankTimer_SetLength((struct TuTimer *)obj->timer, obj->life);
	CTankTimer_StartOrResume((struct TuTimer *)obj->timer);
	origin[0] = 0.0f;
	origin[1] = 0.0f;
	origin[2] = 0.0f;
	origin[3] = 1.0f;
	kind = -1;
	volume = 0;
	if (obj->sound == 1) {
		TU_DPRINTF("WORLD SOUND!!!!!!!!!\n");
		volume = 0xff;
		kind = 0x1a;
	} else if (obj->sound == 2) {
		TU_DPRINTF("WORLD SOUND!!!!!!!!!\n");
		volume = 0xff;
		kind = 0x19;
	} else if (obj->sound == 3) {
		TU_DPRINTF("WORLD SOUND!!!!!!!!!\n");
		volume = 0x7d;
		kind = 0x19;
	}
	if (kind != -1)
		TUB_Sound_Play(world->sound, kind, origin, volume, 0, 0);
	obj->animLeft = obj->animCount;
	if (obj->anim != NULL) {
		struct BActorInstance *instance = (struct BActorInstance *)obj->instance;
		int speed = TU_FTOL(obj->animSpeed * 1024.0f);

		if (obj->animCount == 0)
			baQueueAnimation(instance, (struct BActorNode *)TU_PTR(instance, 0x134),
				(struct BAnim *)obj->anim, speed, -1, 1, NULL, NULL);
		else
			baQueueAnimation(instance, (struct BActorNode *)TU_PTR(instance, 0x134),
				(struct BAnim *)obj->anim, speed, -1, 0, (int (*)())FUN_001c9f50, obj);
	}
	if ((obj->flags & 0x20) != 0)
		CTankWorld_SwitchGroup(self, 1);
	if ((obj->flags & 0x40) != 0)
		CTankWorld_SwitchGroup(self, 2);
}

/* 0x001cac90  TankedUp (gap)
Objects that were active at the start and have a chain hand over to their chain: they stop
and switch the objects their chain names. */
void CTankWorld_RunStartChains(void *self)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuWorldObject *obj;

	for (obj = world->objects; obj != NULL; obj = obj->next)
		obj->wasActive = obj->active;
	for (obj = world->objects; obj != NULL; obj = obj->next) {
		if (obj->active != 0 && obj->wasActive != 0 && obj->chainKind != 0) {
			obj->active = 0;
			world_toggle_named(world, obj->chain);
		}
	}
}

/* 0x001cad70  TankedUp (gap; "worldSystem" update)
Runs the objects: an object whose life ran out, or whose hits are used up, dies and fires
its CHAIN; an animated texture that ended fires its TEXTURECHAIN (both only before state
5); actors follow their position and animate. Returns `state`. */
int CTankWorld_Update(void *self, float step, int state)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuWorldObject *obj;
	float matrix[16];

	for (obj = world->objects; obj != NULL; obj = obj->next)
		if (obj->active != 0)
			obj->started = 0;
	world->changed = 0;
	for (obj = world->objects; obj != NULL; obj = obj->next) {
		obj->step = step;
		if (obj->active == 0)
			continue;
		CTankTimer_Tick((struct TuTimer *)obj->timer);
		if (((obj->life != 0.0f && BCarTimer_GetRemaining((struct BCarTimer *)obj->timer) == 0.0f)
				|| (obj->health != 0 && obj->hits == 0))
			&& state <= 4) {
			obj->active = 0;
			if (obj->chainKind == 1) {
				world->changed = 1;
				world_toggle_named(world, obj->chain);
			}
		}
		if (obj->texAnim != NULL && obj->chainKind == 2 && state <= 4) {
			int ended;

			if (obj->frameMin == -1 && obj->frameMax == -1)
				ended = TU_FTOL(world_elapsed(obj) * 1000.0f) > TU_I32(obj->texAnim, 0x38);
			else if (obj->frameMin < obj->frameMax)
				ended = (double)(obj->frameMax - obj->frameMin)
					< (double)world_elapsed(obj) * obj->animSpeed * 6.0f;
			else
				ended = (double)(obj->frameMin - obj->frameMax - 1)
					< (double)world_elapsed(obj) * obj->animSpeed * 6.0f;
			if (ended) {
				world->changed = 1;
				obj->active = 0;
				world_toggle_named(world, obj->chain);
			}
		}
		if (obj->type == 2) {
			struct BActorInstance *instance = (struct BActorInstance *)obj->instance;

			bmMatrixRotationXZY(matrix, obj->rot[0], obj->rot[1], obj->rot[2]);
			bmMatrixToQuaternion(TU_FP(instance, 0xe0), matrix);
			TU_F32(obj->instance, 0xc0) = obj->pos[0];
			TU_F32(obj->instance, 0xc4) = obj->pos[1];
			TU_F32(obj->instance, 0xc8) = obj->pos[2];
			TU_F32(obj->instance, 0xcc) = obj->pos[3];
			if (obj->anim != NULL)
				baUpdateAnimations((struct BActorInstance *)obj->instance,
					(struct BActorNode *)TU_PTR(obj->instance, 0x134), step, NULL);
			baPrepActorInstance((struct BActorInstance *)obj->instance, 1);
		}
	}
	return state;
}

/* 0x001cb1a0  TankedUp (gap)
How far the ground is below `pos` lowered by `radius` (a ray down through the solid
objects' meshes); *normal gets the ground's normal ((0, 1, 0) when there is none, then the
distance is 1000000). */
float CTankWorld_GroundDistance(void *self, float *pos, float radius, float *normal)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuWorldObject *obj;
	struct BCollHit hits[TU_HITS];
	float toLocal[16], toWorld[16], rotate[16];
	struct BRay ray;
	float best = 1e+06f;
	float point[4];
	int count;

	normal[0] = 0.0f;
	normal[1] = 1.0f;
	normal[2] = 0.0f;
	normal[3] = 1.0f;
	for (obj = world->objects; obj != NULL; obj = obj->next) {
		void *mesh;

		if (obj->active == 0 || obj->collision == NULL || (obj->flags & 1) == 0)
			continue;
		CTankWorld_ObjectMatrices(obj->pos, obj->rot, toLocal, toWorld, rotate);
		point[0] = pos[0];
		point[1] = pos[1];
		point[2] = pos[2];
		point[3] = 1.0f;
		bmTransformVector4InPlace(toLocal, point);
		bmTransformVector4InPlace(rotate, point);
		point[1] = point[1] - radius;
		mesh = obj->collision;
		if (TU_F32(mesh, 0x40) <= point[0] + radius && point[0] - radius <= TU_F32(mesh, 0x50)
			&& TU_F32(mesh, 0x48) <= point[2] + radius && point[2] - radius <= TU_F32(mesh, 0x58)) {
			ray.origin[0] = point[0];
			ray.origin[1] = point[1];
			ray.origin[2] = point[2];
			ray.origin[3] = point[3];
			ray.dir[0] = 0.0f;
			ray.dir[1] = -1000000.0f;
			ray.dir[2] = 0.0f;
			ray.dir[3] = 1.0f;
			ray.length = 0.0f;   /* not set in the original */
			count = TU_HITS;
			hits[0].flags = 0xc;
			if (bmRayMesh(&ray, (struct BCollMesh *)obj->collision, &count, hits) != 0) {
				for (; count > 0; count--) {
					float distance = point[1] - hits[count - 1].point[1];

					if (distance < best) {
						normal[0] = hits[count - 1].normal[0];
						normal[1] = hits[count - 1].normal[1];
						normal[2] = hits[count - 1].normal[2];
						normal[3] = 1.0f;
						bmTransformVector4InPlace(toWorld, normal);
						best = distance;
					}
				}
			}
		}
	}
	return best;
}

/* 0x001cb3f0  TankedUp (gap)
Whether a sphere of `radius` moving from `from` to `to` hits an active object (not a
boundary; with `startedOnly`, only objects that started this frame). */
int CTankWorld_SphereHits(void *self, float *from, float *to, float radius, int startedOnly)
{
	struct TuWorldObject *obj;

	for (obj = ((struct TuWorld *)self)->objects; obj != NULL; obj = obj->next) {
		float toLocal[16], toWorld[16], rotate[16];
		struct BCollHit hits[1];
		struct BSphere sphere;
		float delta[4];
		int count;

		if (obj->active == 0 || obj->collision == NULL || obj->type == 3
			|| (startedOnly != 0 && obj->started == 0))
			continue;
		delta[0] = to[0] - from[0];
		delta[1] = to[1] - from[1];
		delta[2] = to[2] - from[2];
		CTankWorld_ObjectMatrices(obj->pos, obj->rot, toLocal, toWorld, rotate);
		sphere.centre[1] = from[1];
		sphere.centre[2] = from[2];
		sphere.centre[0] = from[0];
		sphere.radius = radius;
		sphere.centre[3] = 1.0f;
		bmTransformVector4InPlace(toLocal, sphere.centre);
		bmTransformVector4InPlace(rotate, sphere.centre);
		delta[3] = 1.0f;
		bmTransformVector4InPlace(rotate, delta);
		count = 1;
		hits[0].flags = 0;
		if (bmSweepSphereMesh(&sphere, (struct BCollMesh *)obj->collision, delta, &count, hits) != 0)
			return 1;
	}
	return 0;
}

/* 0x001cb560  TankedUp (gap)
Sweeps a ray from `pos` along `delta` through the solid objects: returns the first hit's
fraction of `delta` (0 when nothing is hit); *hit gets the hit point (in the object's
space, as in the original) and *normal the normal (w = 0.5). */
float CTankWorld_SweepGround(void *self, float *pos, float *delta, float *hit, float *normal)
{
	struct TuWorldObject *obj;
	struct BCollHit hits[TU_HITS];
	float toLocal[16], toWorld[16], rotate[16];
	struct BRay ray;
	float best = 2.0f;
	int count, i;

	if (delta[2] * delta[2] + delta[1] * delta[1] + delta[0] * delta[0] == 0.0f)
		return 0.0f;
	obj = ((struct TuWorld *)self)->objects;
	if (obj == NULL)
		return 0.0f;
	for (; obj != NULL; obj = obj->next) {
		if (obj->active == 0 || obj->collision == NULL || obj->type == 3)
			continue;
		CTankWorld_ObjectMatrices(obj->pos, obj->rot, toLocal, toWorld, rotate);
		ray.origin[0] = pos[0];
		ray.origin[1] = pos[1];
		ray.origin[2] = pos[2];
		ray.dir[0] = delta[0];
		ray.dir[1] = delta[1];
		ray.dir[2] = delta[2];
		ray.origin[3] = 1.0f;
		ray.dir[3] = 1.0f;
		ray.length = 0.0f;   /* not set in the original */
		bmTransformVector4InPlace(toLocal, ray.origin);
		bmTransformVector4InPlace(rotate, ray.origin);
		bmTransformVector4InPlace(rotate, ray.dir);
		count = TU_HITS;
		hits[0].flags = 0xd;
		if ((obj->flags & 1) == 0
			|| bmRayMesh(&ray, (struct BCollMesh *)obj->collision, &count, hits) == 0)
			continue;
		for (i = 0; i < count; i++) {
			if (hits[i].t < best) {
				best = hits[i].t;
				normal[0] = hits[i].normal[0];
				normal[1] = hits[i].normal[1];
				normal[2] = hits[i].normal[2];
				normal[3] = 1.0f;
				bmTransformVector4InPlace(toWorld, normal);
				normal[3] = 0.5f;
				hit[0] = hits[i].point[0];
				hit[1] = hits[i].point[1];
				hit[2] = hits[i].point[2];
				hit[3] = hits[i].point[3];
			}
		}
	}
	if (best != 2.0f)
		return best;
	return 0.0f;
}

/* 0x001cb7f0  TankedUp (gap)
Whether `to` can be seen from `from` (no solid object between them). */
int CTankWorld_CanSee(void *self, float *from, float *to)
{
	float delta[4];
	float hit[4], normal[4];

	delta[0] = to[0] - from[0];
	delta[3] = 1.0f;
	delta[1] = to[1] - from[1];
	delta[2] = to[2] - from[2];
	return CTankWorld_SweepGround(self, from, delta, hit, normal) == 0.0f;
}

/* 0x001cb860  TankedUp (gap; no `this`)
Whether `point` lies inside the boundary object `obj` (a long ray crosses its mesh an odd
number of times). */
int CTankWorld_InsideBoundary(struct TuWorldObject *obj, float *point)
{
	float toLocal[16], toWorld[16], rotate[16];
	struct BCollHit hits[TU_HITS];
	struct BRay ray;
	int count;

	if (obj->collision == NULL || obj->type != 3)
		return 0;
	count = TU_HITS;
	hits[0].flags = 0;
	CTankWorld_ObjectMatrices(obj->pos, obj->rot, toLocal, toWorld, rotate);
	ray.length = (float)sqrt(100000000000106.0);
	ray.origin[0] = point[0];
	ray.origin[1] = point[1];
	ray.origin[2] = point[2];
	ray.dir[0] = 10000000.0f;
	ray.dir[1] = 5.0f;
	ray.dir[2] = 9.0f;
	ray.dir[3] = 1.0f;
	ray.origin[3] = 1.0f;
	bmTransformVector4InPlace(toLocal, ray.origin);
	bmTransformVector4InPlace(rotate, ray.origin);
	if (bmRayMesh(&ray, (struct BCollMesh *)obj->collision, &count, hits) != 0)
		return count & 1;
	return 0;
}

/* 0x001cb970  TankedUp (gap)
The checkpoint that the move from `pos` by `delta` crosses (a boundary checkpoint: that
`pos` is inside), counted from the first; -1 when there is none. *flags: 1 the first
checkpoint, 2 the last, 4 crossed the wrong way (a one-way checkpoint). */
int CTankWorld_CrossCheckpoint(void *self, float *pos, float *delta, unsigned int *flags)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuCheckpoint *checkpoint;
	float rotate[16], toLocal[16], toWorld[16];
	struct BCollHit hits[TU_HITS];
	float localDelta[4];
	float forward[4];
	struct BRay ray;
	int index = 0;
	int count;
	int crossed;

	*flags = 0;
	if (sqrtf(delta[2] * delta[2] + delta[1] * delta[1] + delta[0] * delta[0]) == 0.0f)
		return -1;
	for (checkpoint = world->checkpoints; ; checkpoint = checkpoint->next, index++) {
		if (checkpoint == NULL)
			return -1;
		bmMatrixRotationXYZ(rotate, -checkpoint->rot[0], -checkpoint->rot[1], -checkpoint->rot[2]);
		bmMatrixTranslation(toLocal, -checkpoint->pos[0], -checkpoint->pos[1], -checkpoint->pos[2]);
		bmMatrixInverseAffine(toWorld, rotate);
		localDelta[0] = delta[0];
		localDelta[1] = delta[1];
		localDelta[2] = delta[2];
		localDelta[3] = 1.0f;
		bmTransformVector4InPlace(rotate, localDelta);
		forward[0] = 0.0f;
		forward[1] = 0.0f;
		forward[2] = 1.0f;
		forward[3] = 1.0f;
		bmTransformVector4InPlace(toWorld, forward);
		ray.origin[3] = 1.0f;
		if (checkpoint->boundary == 0) {
			ray.origin[1] = pos[1];
			ray.origin[2] = pos[2];
			ray.origin[0] = pos[0];
			ray.dir[1] = localDelta[1];
			ray.dir[2] = localDelta[2];
			ray.dir[0] = localDelta[0];
			ray.length = sqrtf(localDelta[0] * localDelta[0] + localDelta[1] * localDelta[1]
				+ localDelta[2] * localDelta[2]);
			ray.dir[3] = localDelta[3];
			bmTransformVector4InPlace(toLocal, ray.origin);
			bmTransformVector4InPlace(rotate, ray.origin);
			count = TU_HITS;
			hits[0].flags = 0;
			crossed = bmRayMesh(&ray, (struct BCollMesh *)checkpoint->collision, &count, hits);
		} else {
			ray.origin[0] = pos[0];
			ray.length = (float)sqrt(100000000000081.0);
			ray.origin[1] = pos[1];
			ray.origin[2] = pos[2];
			count = TU_HITS;
			hits[0].flags = 0;
			ray.dir[0] = 1e+07f;
			ray.dir[1] = 0.0f;
			ray.dir[2] = 9.0f;
			ray.dir[3] = 1.0f;
			bmTransformVector4InPlace(toLocal, ray.origin);
			bmTransformVector4InPlace(rotate, ray.origin);
			if (bmRayMesh(&ray, (struct BCollMesh *)checkpoint->collision, &count, hits) == 0)
				count = 0;
			crossed = count & 1;
		}
		if (crossed != 0)
			break;
	}
	if (checkpoint == world->checkpoints)
		*flags |= 1;
	if (checkpoint->next == NULL)
		*flags |= 2;
	if (checkpoint->oneWay != 0
		&& forward[0] * delta[0] + forward[1] * delta[1] + forward[2] * delta[2] <= 0.0f)
		*flags |= 4;
	return index;
}

/* 0x001cbc80  TankedUp (gap)
Starts the active objects; with g_MusicOn (core 0x002baeb8) runs the start chains twice. */
void CTankWorld_Start(void *self)
{
	struct TuWorldObject *obj;

	for (obj = ((struct TuWorld *)self)->objects; obj != NULL; obj = obj->next)
		if (obj->active != 0)
			CTankWorld_StartObject(self, obj);
	if (DAT_002baeb8 != 0) {
		CTankWorld_RunStartChains(self);
		CTankWorld_RunStartChains(self);
	}
}

/* the result of a command: 0 unknown, 1 done, 2 bad use */
static void world_report(struct TuTextReader *reader, char *text, int result)
{
	char command[512];

	if (_stricmp(text, "}") == 0)
		return;
	sscanf(text, "%s", command);
	if (result == 0)
		TU_DPRINTF("CTank_World::LoadWorldObjects(\"%s\", %d): Invalid Command \"%s\"\n",
			reader->name, reader->line, command);
	else if (result == 2)
		TU_DPRINTF("CTank_World::LoadWorldObjects(\"%s\", %d): Bad use of Command \"%s\"\n",
			reader->name, reader->line, command);
	(void)reader;
	(void)command;
}

/* "NAME"-style string command; returns the result (unchanged when it is another command) */
static int world_name(char *text, const char *command, int length, char *out, int size, int result)
{
	if (_strnicmp(text, command, (size_t)length) != 0)
		return result;
	return BCarText_GetName(text, out, size) != 0 ? 1 : 2;
}

/* an int command that must be >= 0 */
static int world_int(char *text, const char *command, int length, int *out, int result)
{
	int value;

	if (_strnicmp(text, command, (size_t)length) != 0)
		return result;
	if (sscanf(text, "%*s%d", &value) == 1 && value >= 0) {
		*out = value;
		return 1;
	}
	return 2;
}

/* ORIENTATION: three angles, each 0-360 degrees, stored in radians */
static int world_orientation(char *text, float *rot, int result)
{
	float angles[3];
	int i;

	if (_strnicmp(text, "ORIENTATION ", 0xc) != 0)
		return result;
	if (sscanf(text, "%*s%f%f%f", &angles[0], &angles[1], &angles[2]) != 3)
		return 2;
	for (i = 0; i < 3; i++)
		if (angles[i] < 0.0f || angles[i] > 360.0f)
			return 2;
	rot[3] = 1.0f;
	rot[0] = angles[0] * 0.0174532924f;
	rot[1] = angles[1] * 0.0174532924f;
	rot[2] = angles[2] * 0.0174532924f;
	return 1;
}

/* POSITION x y z */
static int world_position(char *text, float *pos, int result)
{
	float x, y, z;

	if (_strnicmp(text, "POSITION ", 9) != 0)
		return result;
	if (sscanf(text, "%*s%f%f%f", &x, &y, &z) != 3)
		return 2;
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	pos[3] = 1.0f;
	return 1;
}

/* 0x001cbcd0  TankedUp/CTank_World.cpp
Reads the object file: "OBJECT {", "SETTINGS {", "BOUNDARY {" and "CHECKPOINT {" blocks
(see the file comment). A line is tested against every block kind in turn. Runs out of
memory: stops reading. */
void CTankWorld_LoadWorldObjects(void *self, struct TuTextReader *reader)
{
	struct TuWorld *world = (struct TuWorld *)self;
	char line[512];
	char resource[512];
	char collision[512];
	char *text;
	int result;

	if (reader->end <= reader->cur)
		return;
	do {
		text = TUB_Reader_ReadLine(reader, line);
		if (strncmp(text, "//", 2) == 0)
			continue;
		if (_stricmp(text, "OBJECT {") == 0) {
			struct TuWorldObject *obj =
				(struct TuWorldObject *)TU_NEW(0x268, TU_FILE_WORLD, 0x101);

			if (obj != NULL)
				CTankTimer_Ctor((struct TuTimer *)obj->timer);
			if (obj == NULL) {
				TU_DPRINTF("CTank_World::LoadWorldObjects - Could not get memory for new world "
					"object\n");
				return;
			}
			CTankWorld_ClearObject(self, obj);
			collision[0] = 0;
			resource[0] = 0;
			while (reader->cur < reader->end) {
				float value;

				if (_stricmp(text, "}") == 0)
					break;
				text = TUB_Reader_ReadLine(reader, line);
				if (strncmp(text, "//", 2) == 0)
					continue;
				result = 0;
				result = world_name(text, "NAME ", 5, obj->name, 0x3f, result);
				result = world_name(text, "ANIMSEGMENT ", 0xc, obj->animSegment, 0x3f, result);
				if (_strnicmp(text, "DECAL", 5) == 0) {
					result = 1;
					obj->decal = 1;
					TU_DPRINTF("GOT A DECAL SERFACE!!!!\n");
				}
				result = world_name(text, "ANIMTEXTURE ", 0xc, obj->animTexture, 0x3f, result);
				result = world_name(text, "TANIM1 ", 7, obj->texture1, 0x3f, result);
				result = world_name(text, "TANIM2 ", 7, obj->texture2, 0x3f, result);
				result = world_int(text, "FRAME_MIN ", 10, &obj->frameMin, result);
				result = world_int(text, "FRAME_MAX ", 10, &obj->frameMax, result);
				if (_stricmp(text, "LOCKLASTTEXFRAME") == 0) {
					result = 1;
					obj->lockLastFrame = 1;
					TU_DPRINTF("OBJECT: [%s] IS FRAME LOCKED!!!!\n", obj->name);
				}
				if (_strnicmp(text, "ACTOR ", 6) == 0) {
					result = 2;
					if (BCarText_GetName(text, resource, 0x1ff) != 0) {
						result = 1;
						obj->type = 2;
					}
				}
				if (_strnicmp(text, "SET ", 4) == 0) {
					result = 2;
					if (BCarText_GetName(text, resource, 0x1ff) != 0) {
						result = 1;
						obj->type = 1;
					}
				}
				if (_strnicmp(text, "BOUNCE", 6) == 0) {
					result = 1;
					obj->bounce = 1;
				}
				result = world_name(text, "COLLISION ", 10, collision, 0x1ff, result);
				result = world_int(text, "HEALTH ", 7, &obj->health, result);
				result = world_int(text, "SOUND ", 6, &obj->sound, result);
				if (_strnicmp(text, "LIFE ", 5) == 0) {
					result = 2;
					if (sscanf(text, "%*s%f", &value) == 1 && !(value < 0.0f)) {
						obj->life = value;
						result = 1;
					}
				}
				if (_strnicmp(text, "ACTIVE ", 7) == 0) {
					int on;

					result = 2;
					if (sscanf(text, "%*s%d", &on) == 1) {
						result = 1;
						obj->active = on != 0;
					}
				}
				result = world_int(text, "FLAGS ", 6, (int *)&obj->flags, result);
				result = world_int(text, "WARNFLAGS ", 10, &obj->warnFlags, result);
				if (_strnicmp(text, "CHAIN ", 6) == 0) {
					result = 2;
					if (BCarText_GetName(text, obj->chain, 0x3f) != 0) {
						result = 1;
						obj->chainKind = 1;
					}
				}
				if (_strnicmp(text, "TEXTURECHAIN ", 0xd) == 0) {
					result = 2;
					if (BCarText_GetName(text, obj->chain, 0x3f) != 0) {
						result = 1;
						obj->chainKind = 2;
					}
				}
				result = world_position(text, obj->pos, result);
				result = world_orientation(text, obj->rot, result);
				result = world_int(text, "ANIMCOUNT ", 10, &obj->animCount, result);
				if (_strnicmp(text, "ANIMSPEED ", 10) == 0) {
					result = 2;
					if (sscanf(text, "%*s%f", &value) == 1 && value > 0.0f) {
						obj->animSpeed = value;
						result = 1;
					}
				}
				if (_strnicmp(text, "RESTITUTION ", 0xc) == 0) {
					result = 2;
					if (sscanf(text, "%*s%f", &value) == 1 && !(value < 0.0f)) {
						obj->restitution = value;
						result = 1;
					}
				}
				if (_strnicmp(text, "FRICTION ", 9) == 0) {
					result = 2;
					if (sscanf(text, "%*s%f", &value) == 1 && value <= 1.0f) {
						obj->friction = value;
						result = 1;
					}
				}
				world_report(reader, text, result);
			}
			if (obj->name[0] == 0) {
				TU_DPRINTF("CTank_World::LoadWorldObjects - Object defined with no name, ignoring\n");
				if (obj->texAnim != NULL)
					TU_DELETE(obj->texAnim);
				if (obj->tex1 != NULL)
					TU_DELETE(obj->tex1);
				if (obj->tex2 != NULL)
					TU_DELETE(obj->tex2);
				/* the timer's destructor is empty */
				TU_DELETE(obj);
			} else {
				if (obj->type == 1) {
					obj->set = bLoadResource(NULL, resource, 6, 0xdefa);
					if (obj->set == NULL) {
						TU_DPRINTF("CTank_World::LoadWorldObjects - Could not load set \"%s\". "
							"Object now invisible\n", resource);
						obj->type = 0;
					}
				} else if (obj->type == 2) {
					obj->actor = bLoadResource(NULL, resource, 1, 0xdefa);
					if (obj->actor == NULL) {
						TU_DPRINTF("CTank_World::LoadWorldObjects - Could not load actor \"%s\". "
							"Object now invisible\n", resource);
						obj->type = 0;
					} else {
						obj->instance = baInitActorInstance(NULL, (struct BActor *)obj->actor, 2,
							TU_FILE_WORLD, 0x29d);
						baPrepActorInstance((struct BActorInstance *)obj->instance, 1);
						if (obj->animSegment[0] != 0) {
							obj->anim = baFindSubObject((struct BActorInstance *)obj->instance,
								obj->animSegment);
							if (obj->anim == NULL)
								TU_DPRINTF("CTank_World::LoadWorldObjects - Could not find animation "
									"segment \"%s\". Ignoring\n", obj->animSegment);
						}
					}
				}
				if (collision[0] != 0) {
					obj->collision = bLoadResource(NULL, collision, 7, 0xdefa);
					if (obj->collision == NULL)
						TU_DPRINTF("CTank_World::LoadWorldObjects - Could not load collision mesh "
							"\"%s\"\n", collision);
				}
				if (obj->animTexture[0] != 0) {
					obj->texAnim = bLoadResource(NULL, obj->animTexture, 0, 0xdefa);
					if (obj->texAnim == NULL)
						TU_DPRINTF("CTank_World::LoadWorldObjects - Could not find animated texture "
							"\"%s\"\n", obj->animTexture);
				}
				if (obj->texture1[0] != 0) {
					obj->tex1 = bLoadResource(NULL, obj->texture1, 0, 0xdefa);
					if (obj->tex1 == NULL)
						TU_DPRINTF("CTank_World::LoadWorldObjects - Could not find animated texture "
							"\"%s\"\n", obj->texture1);
				}
				if (obj->texture2[0] != 0) {
					obj->tex2 = bLoadResource(NULL, obj->texture2, 0, 0xdefa);
					if (obj->tex2 == NULL)
						TU_DPRINTF("CTank_World::LoadWorldObjects - Could not find animated texture "
							"\"%s\"\n", obj->texture2);
				}
				CTankTimer_Reset((struct TuTimer *)obj->timer);
				obj->next = world->objects;
				world->objects = obj;
			}
		}
		if (_stricmp(text, "SETTINGS {") == 0) {
			while (reader->cur < reader->end) {
				if (_stricmp(text, "}") == 0)
					break;
				text = TUB_Reader_ReadLine(reader, line);
				if (strncmp(text, "//", 2) == 0)
					continue;
				result = 0;
				if (_strnicmp(text, "GRAVITY ", 8) == 0) {
					result = 2;
					if (sscanf(text, "%*s%f", &world->gravity) == 1)
						result = 1;
				}
				if (_strnicmp(text, "LAPS ", 5) == 0) {
					result = 2;
					if (sscanf(text, "%*s%d", &world->laps) == 1)
						result = 1;
				}
				world_report(reader, text, result);
			}
		}
		if (_stricmp(text, "BOUNDARY {") == 0) {
			struct TuWorldObject *obj =
				(struct TuWorldObject *)TU_NEW(0x268, TU_FILE_WORLD, 0x30f);

			if (obj != NULL)
				CTankTimer_Ctor((struct TuTimer *)obj->timer);
			if (obj == NULL) {
				TU_DPRINTF("CTank_World::LoadWorldObjects - Could not get memory for new boundary "
					"object\n");
				return;
			}
			CTankWorld_ClearObject(self, obj);
			obj->type = 3;
			collision[0] = 0;
			while (reader->cur < reader->end) {
				float value;

				if (_stricmp(text, "}") == 0)
					break;
				text = TUB_Reader_ReadLine(reader, line);
				if (strncmp(text, "//", 2) == 0)
					continue;
				result = 0;
				result = world_name(text, "NAME ", 5, obj->name, 0x3f, result);
				result = world_name(text, "COLLISION ", 10, collision, 0x1ff, result);
				if (_strnicmp(text, "LIFE ", 5) == 0) {
					result = 2;
					if (sscanf(text, "%*s%f", &value) == 1 && !(value < 0.0f)) {
						obj->life = value;
						result = 1;
					}
				}
				if (_strnicmp(text, "ACTIVE ", 7) == 0) {
					int on;

					result = 2;
					if (sscanf(text, "%*s%d", &on) == 1) {
						result = 1;
						obj->active = on != 0;
					}
				}
				result = world_int(text, "FLAGS ", 6, (int *)&obj->flags, result);
				if (_strnicmp(text, "CHAIN ", 6) == 0) {
					result = 2;
					if (BCarText_GetName(text, obj->chain, 0x3f) != 0) {
						result = 1;
						obj->chainKind = 1;
					}
				}
				result = world_position(text, obj->pos, result);
				result = world_orientation(text, obj->rot, result);
				world_report(reader, text, result);
			}
			if (obj->name[0] == 0) {
				TU_DPRINTF("CTank_World::LoadWorldObjects - Object defined with no name, ignoring\n");
				TU_DELETE(obj);
			} else if (collision[0] != 0) {
				obj->collision = bLoadResource(NULL, collision, 7, 0xdefa);
				if (obj->collision == NULL) {
					TU_DPRINTF("CTank_World::LoadWorldObjects - Could not load collision mesh \"%s\". "
						"ignoring\n", collision);
					TU_DELETE(obj);
				} else {
					CTankTimer_Reset((struct TuTimer *)obj->timer);
					obj->next = world->objects;
					world->objects = obj;
				}
			}
			/* (a boundary without COLLISION is dropped without being freed, as in the original) */
		}
		if (_stricmp(text, "CHECKPOINT {") == 0) {
			struct TuCheckpoint *checkpoint =
				(struct TuCheckpoint *)TU_NEW(0x30, TU_FILE_WORLD, 0x3d0);
			int corner;

			if (checkpoint == NULL) {
				TU_DPRINTF("CTank_World::LoadWorldObjects - Could not get memory for new checkpoint\n");
				return;
			}
			checkpoint->pos[0] = 0.0f;
			checkpoint->pos[1] = 0.0f;
			checkpoint->pos[2] = 0.0f;
			checkpoint->pos[3] = 1.0f;
			checkpoint->rot[0] = 0.0f;
			checkpoint->rot[1] = 0.0f;
			checkpoint->rot[2] = 0.0f;
			checkpoint->rot[3] = 1.0f;
			checkpoint->collision = NULL;
			checkpoint->oneWay = 0;
			checkpoint->boundary = 0;
			collision[0] = 0;
			while (reader->cur < reader->end) {
				int on;

				if (_stricmp(text, "}") == 0)
					break;
				text = TUB_Reader_ReadLine(reader, line);
				if (strncmp(text, "//", 2) == 0)
					continue;
				result = 0;
				result = world_name(text, "COLLISION ", 10, collision, 0x1ff, result);
				if (_strnicmp(text, "ONEWAY ", 7) == 0) {
					result = 2;
					if (sscanf(text, "%*s%d", &on) == 1) {
						result = 1;
						checkpoint->oneWay = on != 0;
					}
				}
				if (_strnicmp(text, "BOUNDARY ", 9) == 0) {
					result = 2;
					if (sscanf(text, "%*s%d", &on) == 1) {
						result = 1;
						checkpoint->boundary = on != 0;
					}
				}
				result = world_position(text, checkpoint->pos, result);
				if (_strnicmp(text, "ORIENTATION ", 0xc) == 0) {
					float angles[3];

					result = 2;
					if (sscanf(text, "%*s%f%f%f", &angles[0], &angles[1], &angles[2]) == 3) {
						checkpoint->rot[3] = 1.0f;
						result = 1;
						checkpoint->rot[0] = angles[0] * 0.0174532924f;
						checkpoint->rot[1] = angles[1] * 0.0174532924f;
						checkpoint->rot[2] = angles[2] * 0.0174532924f;
					}
				}
				world_report(reader, text, result);
			}
			/* debug: the corners of the checkpoint gate (printed only) */
			for (corner = 0; corner < 4; corner++) {
				float offset[4], point[4];

				offset[0] = corner < 2 ? -498.0f : -830.0f;
				offset[2] = (corner & 1) == 0 ? 166.0f : 498.0f;
				offset[1] = 0.0f;
				offset[3] = 1.0f;
				bmVectorRotateY(point, offset, checkpoint->rot[1]);
				TU_DPRINTF("%f 200 %f\n", (double)(point[0] + checkpoint->pos[0]),
					(double)(point[2] + checkpoint->pos[2]));
				(void)point;
			}
			text = line;   /* (the original's line register was reused by the loop above) */
			if (collision[0] != 0) {
				checkpoint->collision = bLoadResource(NULL, collision, 7, 0xdefa);
				if (checkpoint->collision == NULL) {
					TU_DPRINTF("CTank_World::LoadWorldObjects - Could not load collision mesh \"%s\". "
						"ignoring\n", collision);
					TU_DELETE(checkpoint);
				} else {
					struct TuCheckpoint *last;

					world->checkpointCount++;
					checkpoint->next = NULL;
					if (world->checkpoints == NULL) {
						world->checkpoints = checkpoint;
					} else {
						for (last = world->checkpoints; last->next != NULL; last = last->next)
							;
						last->next = checkpoint;
					}
				}
			}
		}
	} while (reader->cur < reader->end);
}

/* 0x001cd320  TankedUp/CTank_World.cpp (inferred)
Frees the objects (their tweak menus, actor instances), the checkpoints and the world
tweak menu. */
void CTankWorld_Free(void *self)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuWorldObject *obj = world->objects;
	struct TuCheckpoint *checkpoint;
	void *menu;

	while (obj != NULL) {
		struct TuWorldObject *next;

		if ((obj->flags & 1) != 0 && obj->menu != NULL) {
			menu = obj->menu;
			CTweakMenu_Dtor((struct CTweakMenu *)menu);
			TU_DELETE(menu);
		}
		next = obj->next;
		/* the timer's destructor is empty */
		if (obj->instance != NULL)
			baDeleteActorInstance((struct BActorInstance *)obj->instance);
		CTankWorldObject_Dtor(obj);
		TU_DELETE(obj);
		obj = next;
	}
	world->objects = NULL;
	checkpoint = world->checkpoints;
	while (checkpoint != NULL) {
		struct TuCheckpoint *next = checkpoint->next;

		TU_DELETE(checkpoint);
		checkpoint = next;
	}
	world->checkpoints = NULL;
	menu = TU_DBG_WORLD_MENU;
	if (menu != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)menu);
		TU_DELETE(menu);
	}
	TU_DBG_WORLD_MENU = NULL;
}

/* 0x001cd3d0  TankedUp/CTank_World.cpp (inferred)
Moves a sphere of `radius` at *pos with velocity *vel through the world. Boundaries the
sphere is inside add their FLAGS to *flags; solid objects it hits stop it at the first hit
and bounce it: the velocity loses its part along the mean normal, times (1 + the mean
RESTITUTION * `bounce`) (RESTITUTION 1 with *flags 0x100). An object hit with *flags 2 loses
a hit. *onGround is set when a hit normal points up (y >= 0.7), *friction gets the highest
FRICTION (0 with *flags 0x100 or none), *normal the mean normal, *bounced whether a BOUNCE
object was hit. `rot` (x, y, z) turns an unused up vector. Returns whether anything was hit;
*flags gets the flags of what was touched. */
int CTankWorld_Collide(void *self, float *pos, float *vel, float *rot, float radius,
	unsigned int *flags, float bounce, int *onGround, float *friction, float *normal, int *bounced)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuWorldObject *obj;
	struct BCollHit hits[TU_HITS];
	float toLocal[16], toWorld[16], rotate[16];
	float up[4];
	float sum[4];
	float restitution;
	float nearest;
	unsigned int touched = 0;
	int count = 0;

	if (sqrtf(vel[2] * vel[2] + vel[1] * vel[1] + vel[0] * vel[0]) == 0.0f)
		return 0;
	*onGround = 0;
	*bounced = 0;
	*friction = -1e+06f;
	sum[0] = 0.0f;
	sum[1] = 0.0f;
	sum[2] = 0.0f;
	sum[3] = 1.0f;
	restitution = 0.0f;
	nearest = 10.0f;
	up[0] = 0.0f;
	up[1] = 1.0f;
	up[2] = 0.0f;
	up[3] = 1.0f;
	bmVectorRotateZXY(up, up, rot[2], rot[0], rot[1]);
	for (obj = world->objects; obj != NULL; obj = obj->next) {
		struct BSphere sphere;
		float delta[4];
		int found, i;

		if (obj->active == 0 || obj->collision == NULL)
			continue;
		if (obj->type == 3) {
			if (CTankWorld_InsideBoundary(obj, pos) != 0)
				touched |= obj->flags;
			continue;
		}
		CTankWorld_ObjectMatrices(obj->pos, obj->rot, toLocal, toWorld, rotate);
		sphere.centre[0] = pos[0];
		sphere.centre[1] = pos[1];
		sphere.centre[2] = pos[2];
		delta[1] = vel[1];
		sphere.radius = radius;
		delta[2] = vel[2];
		delta[0] = vel[0];
		delta[3] = vel[3];
		sphere.centre[3] = 1.0f;
		bmTransformVector4InPlace(toLocal, sphere.centre);
		bmTransformVector4InPlace(rotate, sphere.centre);
		delta[3] = 1.0f;
		bmTransformVector4InPlace(rotate, delta);
		found = TU_HITS;
		hits[0].flags = 5;
		if (bmSweepSphereMesh(&sphere, (struct BCollMesh *)obj->collision, delta, &found, hits) == 0)
			continue;
		touched |= obj->flags;
		*bounced = (*bounced != 0 || obj->bounce != 0) ? 1 : 0;
		if (*friction < obj->friction)
			*friction = obj->friction;
		if ((obj->flags & 1) == 0)
			continue;
		if ((*flags & 2) != 0 && obj->health != 0)
			obj->hits--;
		restitution = restitution + obj->restitution;
		for (i = 0; i < found; i++) {
			count++;
			hits[i].normal[3] = 1.0f;
			bmTransformVector4InPlace(toWorld, hits[i].normal);
			sum[0] = sum[0] + hits[i].normal[0];
			sum[1] = sum[1] + hits[i].normal[1];
			sum[2] = sum[2] + hits[i].normal[2];
			if (0.7 <= (double)((hits[i].normal[2] + hits[i].normal[0]) * 0.0f + hits[i].normal[1]))
				*onGround = 1;
			if (hits[i].t < nearest)
				nearest = hits[i].t;
		}
	}
	if (*friction < -999999.0f)
		*friction = 0.0f;
	if ((*flags & 0x100) != 0)
		*friction = 0.0f;
	if (count != 0) {
		float vx = vel[0], vy = vel[1], vz = vel[2], vw = vel[3];
		float dx = vel[0], dy = vel[1], dz = vel[2];
		float speed = sqrtf(dx * dx + dy * dy + dz * dz);
		float push;

		restitution = restitution / (float)count;
		bmVectorNormalise(sum, sum);
		normal[0] = sum[0];
		normal[1] = sum[1];
		normal[2] = sum[2];
		normal[3] = sum[3];
		if ((*flags & 0x100) != 0)
			restitution = 1.0f;
		if (speed > 0.01f) {
			float inverse = 1.0f / speed;

			dx = inverse * dx;
			dy = dy * inverse;
			dz = inverse * dz;
		}
		push = (restitution * bounce + 1.0f) * (dx * sum[0] + dy * sum[1] + dz * sum[2]) * speed;
		pos[1] = nearest * vy + pos[1];
		pos[2] = nearest * vz + pos[2];
		pos[3] = 1.0f;
		pos[0] = nearest * vx + pos[0];
		vel[2] = vz - sum[2] * push;
		vel[0] = vx - sum[0] * push;
		vel[3] = vw;
		vel[1] = vy - sum[1] * push;
	}
	*flags = touched;
	return count != 0;
}

/* 0x001cd8c0  TankedUp/CTank_World.cpp (inferred)
The part of the way from `from` to `to` that is free (1 when nothing is in the way). */
float CTankWorld_FreeFraction(void *self, float *from, float *to)
{
	float delta[4];
	float hit[4], normal[4];
	float fraction;

	delta[0] = to[0] - from[0];
	delta[1] = to[1] - from[1];
	delta[2] = to[2] - from[2];
	fraction = CTankWorld_SweepGround(self, from, delta, hit, normal);
	if (fraction == 0.0f)
		fraction = 1.0f;
	return fraction;
}

/* 0x001cd920  TankedUp/CTank_World.cpp
Initialise: reads the object file `file` (no file: an empty world) and gives every solid
object a tweak menu (restitution, friction, bounce). Returns 0 when the file is missing. */
int CTankWorld_Initialise(void *self, void *package, char *file, void *sound, void *lights)
{
	struct TuWorld *world = (struct TuWorld *)self;
	struct TuTextReader reader;
	struct TuWorldObject *obj;
	char name[512];
	unsigned int size;
	void *buffer;

	TUB_Reader_Ctor(&reader);
	world->sound = sound;
	world->lights = (struct TuLight *)lights;
	world->checkpoints = NULL;
	world->objects = NULL;
	world->checkpointCount = 0;
	world->gravity = 0.0f;
	world->laps = 0;
	if (file[0] != 0) {
		buffer = bkLoadFile((struct bkPackage *)package, file, NULL, &size, NULL, 0);
		if (buffer == NULL) {
			TU_DPRINTF("CTank_World::Initialise - Configuration file [%s] not found\n", file);
			TUB_Reader_Dtor(&reader);
			return 0;
		}
		BCarTextReader_Init((struct BCarTextReader *)&reader, (char *)buffer, (int)size, file);
		CTankWorld_LoadWorldObjects(self, &reader);
		for (obj = world->objects; obj != NULL; obj = obj->next) {
			void *menu;

			if ((obj->flags & 1) == 0)
				continue;
			sprintf(name, obj->bounce == 0 ? "%s" : "%s - bounce", obj->name);
			menu = TU_NEW(0xe8, TU_FILE_WORLD, 0x7a);
			if (menu != NULL)
				menu = CTweakMenu_CtorSub((struct CTweakMenu *)menu,
					(struct CTweakMenu *)TU_DBG_WORLD_MENU, name);
			obj->menu = menu;
			CTweakMenu_AddFloat((struct CTweakMenu *)menu, "restitution", &obj->restitution, 0.01f,
				0.0f, 1.0f);
			CTweakMenu_AddFloat((struct CTweakMenu *)obj->menu, "friction", &obj->friction, 0.01f,
				0.0f, 1.0f);
			CTweakMenu_AddBool((struct CTweakMenu *)obj->menu, "bounce", &obj->bounce);
		}
	}
	TUB_Reader_Dtor(&reader);
	return 1;
}
