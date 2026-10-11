/*
Demolition/CDemolition_LiftShaft.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_LiftShaft.cpp

Variation 3 of Demolition: the arena falls down a lift shaft. CDemolition_LiftShaft (vtable
0x0026b880, 0x1690 bytes, at +0x40 of the variation object) owns:
  - the shaft: a ring of 10 sections scrolling up past the arena (and a shake),
  - 4 platforms (actors with brakes that throw sparks); a platform can fall away,
  - rings of glowing lights (barriers) that rise past, in 4 random patterns, with 3 dynamic
    lights following the lowest ones,
  - the fall itself: waits, speeds up to 80, brakes (sparks), slows, and so on; and the
    camera sway and the ambient light that fades in as the fall starts.

0x000fff50 is tagged with CDemolition_LiftShaft.cpp; the gap functions 0x000ff630-0x00101900
use the same object (file inferred). Four functions of this file are missing from the function
table (see demolition_c.h): 0x000ffe90, 0x000ffec0, 0x00100ef0 and 0x00100f60.
*/

#include "demolition_c_internal.h"
#include "demolition_c_particle.h"

/* ---------- data (.data, owned here) */

/* 0x00298b38  the shaft section models */
static const char *const DAT_00298b38[4] = {
	"cross\\liftshaft\\walls top start.obe",
	"cross\\liftshaft\\walls.obe",
	"cross\\liftshaft\\walldoor.obe",
	"cross\\liftshaft\\glass walls.obe",
};
/* 0x00298b48  the section heights */
static const float DAT_00298b48[4] = { 1632.0f, 1632.0f, 816.0f, 1632.06006f };
/* 0x00298b58  the platform model */
static const char *const DAT_00298b58[1] = { "cross\\objects\\lift.obe" };
/* 0x00298b5c  the spark nodes of a platform, then its brake animations */
static const char *const DAT_00298b5c[4] = { "sparks03", "sparks04", "sparks05", "sparks06" };
static const char *const DAT_00298b6c[2] = { "brake_in", "brake_out" };
/* 0x00298b80-0x00298bac  barrier colours of the patterns (r, g, b as ints) */
static const int DAT_00298b80[3] = { 0x13, 0x4a, 0x7f };
static const int DAT_00298b8c[3] = { 0x7b, 0x55, 0x22 };
static const int DAT_00298b98[3] = { 0x20, 0x78, 0x30 };
static const int DAT_00298ba4[3] = { 0x7f, 0, 0 };

/* 0x00297520  the variation settings (demolition_a, 0x3c bytes each): +0 the shaft is drawn,
   +0x24 ambient red, green, blue */
extern int DAT_00297520[60];

extern const void *const vtable_0026b880[1];

/* ---------- helpers */

#define DC_RAND_UNIT 3.05185094e-05f /* 0x00253740 */

static float dc_unit(void)
{
	return (float)dc_rand() * DC_RAND_UNIT;
}

/* CDemolition_Light (demolition_a) */
#define DC_LIGHT(self, i) ((void *)(self)->lights[i])
#define DC_LIGHT_INIT(l) (((void (*)(void *))(*(void *const *const *)(l))[1])(l))
#define DC_LIGHT_RELEASE(l) (((void (*)(void *))(*(void *const *const *)(l))[2])(l))
#define DC_LIGHT_SET_ON(l, on) (((void (*)(void *, int))(*(void *const *const *)(l))[3])((l), (on)))
#define DC_LIGHT_UPDATE(l, dt) (((void (*)(void *, float))(*(void *const *const *)(l))[4])((l), (dt)))

/* the ClassLib particle system calls (vtable slots) */
#define DC_PS_CALL(obj, slot) (((void (*)(void *))(*(void *const *const *)(obj))[slot])(obj))
#define DC_PS_UPDATE(obj, dt) (((void (*)(void *, float))(*(void *const *const *)(obj))[1])((obj), (dt)))

/* 0x0018fa30 (rollerball_a)  a light's range (float in an undefined4) */
static inline void dc_light_set_range(void *light, float range)
{
	DC_DUAL(FUN_0018fa30, void (*)(int, undefined4), void (*)(int, float),
		((int)light, dc_fbits(range)), ((int)light, range));
}

/* adds a section on top of the ring (inlined in 0x001005c0 and 0x00100fd0) */
static void dc_ring_push(struct CDemolition_LiftShaft *self, struct DcLiftSegment *seg, int flag)
{
	int idx = (self->ringHead + self->ringCount) % 10;

	self->ring[idx].seg = seg;
	self->ring[idx].opaqueOnly = (unsigned char)flag;
	if (self->ringCount < 10)
		self->ringCount = self->ringCount + 1;
	self->sectionCounter = self->sectionCounter + 1;
}

/* the platform's brake animation (inlined) */
static void dc_platform_brake(struct DcPlatform *p, void *anim)
{
	baFlushAnimations(p->actor, (struct BActorNode *)DC_PTR(p->actor, 0x134), 1, 0);
	baQueueAnimation((struct BActorInstance *)p->actor, (struct BActorNode *)DC_PTR(p->actor, 0x134),
		(struct BAnim *)anim, 0x400, -1, 0, 0, 0);
}

/* the end of a barrier pattern: picks another one (inlined in the 4 pattern functions) */
static void dc_next_pattern(struct CDemolition_LiftShaft *self)
{
	uint r;

	do
		r = (uint)dc_rand() & 3;
	while (r == (uint)self->pattern);
	self->pattern = (int)r;
	self->barrierCounter = 0;
}

/* ---------- functions */

/* 0x000ff630  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Releases the platforms' actors and spark systems. */
void CDemolition_LiftShaft_ReleasePlatforms(struct CDemolition_LiftShaft *self)
{
	int i;
	int k;

	for (i = 0; i < 4; i++)
	{
		baDeleteActorInstance((struct BActorInstance *)self->platforms[i].actor);
		for (k = 0; k < 4; k++)
			DC_PS_CALL(self->platforms[i].sparks[k], 0);
	}
}

/* 0x000ff680  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Reset of the fall: not started, slowing down (state 1), brakes off. */
void CDemolition_LiftShaft_ResetFall(struct CDemolition_LiftShaft *self)
{
	int i;

	self->started = 0;
	self->state = 1;
	for (i = 0; i < 4; i++)
		self->platforms[i].braking = 0;
}

/* 0x000ff6b0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
State 3: the fall speeds up (to 80); when its time is up, the brakes go on (state 4). */
void CDemolition_LiftShaft_Fall(struct CDemolition_LiftShaft *self, float dt)
{
	struct DcPlatform *p;
	float t;
	int i;

	self->speed = dt + self->speed;
	if (80.0f < self->speed) /* 0x0026556c */
		self->speed = 80.0f;
	t = self->timer - dt;
	self->timer = t;
	if (!(t < 0.0f))
		return;
	self->state = 4;
	for (i = 0; i < 4; i++)
	{
		p = &self->platforms[i];
		if (p->falling == 0 && p->braking == 0)
		{
			p->braking = 1;
			dc_platform_brake(p, p->anims[0]);
		}
	}
}

/* 0x000ff760  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Draws the shaft sections, from the arena down (the glass/door sections opaque only). */
void CDemolition_LiftShaft_DrawShaft(struct CDemolition_LiftShaft *self)
{
	struct DcLiftSegment *seg;
	float m[16];
	float t[16];
	int idx;
	int i;

	bmMatrixTranslation(m, 0.0f, self->drop + self->scroll + self->shake, 0.0f);
	idx = self->ringHead;
	for (i = 0; i < self->ringCount; i++)
	{
		seg = self->ring[idx].seg;
		bmMatrixTranslation(t, 0.0f, -seg->height, 0.0f);
		bmMatrixMultiplyCopy(m, m, t);
		bdSetWorldMatrix(m);
		if (self->ring[idx].opaqueOnly == 0)
		{
			baDrawSet((struct BSet *)seg->model, 1);
			baDrawSet((struct BSet *)seg->model, 2);
		}
		else
		{
			baDrawSet((struct BSet *)seg->model, 1);
		}
		idx = (idx + 1) % 10;
	}
}

/* 0x000ff860  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Draws the visible platforms. */
void CDemolition_LiftShaft_DrawPlatforms(struct CDemolition_LiftShaft *self)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (self->platforms[i].visible != 0)
			baDrawActorInstance((struct BActorInstance *)self->platforms[i].actor, 7);
	}
}

/* 0x000ff890  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Moves the barriers up with the fall (the ones above 8000 leave the ring); each grows and fades
as it rises out of the floor; the 3 lights follow the lowest barriers above -8000. */
void CDemolition_LiftShaft_UpdateBarriers(struct CDemolition_LiftShaft *self, float dt)
{
	struct DcBarrier *b;
	float y;
	float v;
	int count;
	int idx;
	int used;
	int i;

	count = self->barrierCount;
	if (0 < count)
	{
		idx = self->barrierHead;
		for (i = 0; i < count; i++)
		{
			if (8000.0f < self->barriers[idx].centre[1]) /* 0x0026b87c */
				self->barrierCount = self->barrierCount - 1;
			idx = (idx + 1) % 10;
		}
		idx = self->barrierHead;
		for (i = 0; i < self->barrierCount; i++)
		{
			b = &self->barriers[idx];
			y = dt * self->speed + b->centre[1];
			b->centre[1] = y;
			if (0.0f <= y)
			{
				v = 0.0f;
			}
			else
			{
				v = y * -6.66666674e-05f; /* 0x0026b878 */
				if (1.0f < v)
					v = 1.0f;
				else if (v < 0.0f)
					v = 0.0f;
			}
			b->size = 2000.0f * v + 200.0f;
			b->colour[0] = (short)(int)((float)b->baseColour[0] * (1.0f - v));
			b->colour[1] = (short)(int)((float)b->baseColour[1] * (1.0f - v));
			b->colour[2] = (short)(int)((float)b->baseColour[2] * (1.0f - v));
			b->zBias = v * -0.000250000012f; /* 0x0026b874 */
			idx = (idx + 1) % 10;
		}
	}
	count = self->barrierCount;
	if (count <= 0)
		return;
	used = 0;
	idx = (self->barrierHead - 1 + count) % 10;
	for (i = 0; i < self->barrierCount; i++)
	{
		b = &self->barriers[idx];
		if (b->centre[1] < -8000.0f) /* 0x0026b870 */
			break;
		if (used == 3)
			goto update;
		FUN_0018fa10((void *)DC_LIGHT(self, used), b->colour[0] / 2, b->colour[1] / 2,
			b->colour[2] / 2);
		dc_light_set_range(DC_LIGHT(self, used), 0.000333333359f);
		FUN_000edbc0((int)DC_LIGHT(self, used), (undefined4 *)b->centre);
		DC_LIGHT_SET_ON(DC_LIGHT(self, used), 1);
		used = used + 1;
		idx = idx - 1;
		if (idx < 0)
			idx = 9;
	}
	for (; used < 3; used++)
		DC_LIGHT_SET_ON(DC_LIGHT(self, used), 0);
update:
	for (i = 0; i < 3; i++)
		DC_LIGHT_UPDATE(DC_LIGHT(self, i), dt);
}

/* 0x000ffc00  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Draws the barriers: at each point a coloured glow and a grey core, additive. */
void CDemolition_LiftShaft_DrawBarriers(struct CDemolition_LiftShaft *self)
{
	struct DcBarrier *b;
	float pos[4];
	int glow;
	int core;
	int idx;
	int i;
	int j;

	bdSetWorldIdentity();
	bdSetProjectionMode(1);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	glow = DC_I32(DC_GAME, 0x6c68);
	core = DC_I32(DC_GAME, 0x6c64);
	idx = self->barrierHead;
	pos[3] = 1.0f;
	for (i = 0; i < self->barrierCount; i++)
	{
		b = &self->barriers[idx];
		pos[1] = b->centre[1] + self->shake;
		for (j = 0; j < b->count; j++)
		{
			pos[0] = b->points[j][0];
			pos[2] = b->points[j][2];
			bdDrawSprite3D(pos, b->size, b->size, 0.0f, (struct bdTexture *)glow, 0, 0, b->colour[0],
				b->colour[1], b->colour[2], 0x7f, NULL, b->zBias);
			bdDrawSprite3D(pos, 100.0f, 100.0f, 0.0f, (struct bdTexture *)core, 0, 0, 0x60, 0x60,
				0x60, 0x7f, NULL, b->zBias);
		}
		idx = (idx + 1) % 10;
	}
	bdSetProjectionMode(1);
	bdPopRenderState(6);
	bdPopRenderState(5);
}

/* 0x000ffda0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Adds a barrier at height y in the current shape and colour (at most 10). */
void CDemolition_LiftShaft_AddBarrier(struct CDemolition_LiftShaft *self, float y)
{
	struct DcBarrier *b;

	if (10 <= self->barrierCount)
		return;
	self->barrierHead = self->barrierHead - 1;
	if (self->barrierHead < 0)
		self->barrierHead = 9;
	self->barrierCount = self->barrierCount + 1;
	b = &self->barriers[self->barrierHead];
	b->centre[0] = 0.0f;
	b->centre[1] = y;
	b->centre[2] = 0.0f;
	b->baseColour[0] = self->colour[0];
	b->baseColour[1] = self->colour[1];
	b->baseColour[2] = self->colour[2];
	if (self->shape == 0)
	{
		b->count = 2;
		b->points[0][0] = -1200.0f;
		b->points[1][0] = 1200.0f;
		b->points[1][2] = 0.0f;
		b->points[0][2] = 0.0f;
	}
	else if (self->shape == 1)
	{
		b->count = 2;
		b->points[0][0] = 0.0f;
		b->points[0][2] = 1200.0f;
		b->points[1][0] = 0.0f;
		b->points[1][2] = -1200.0f;
	}
	else if (self->shape == 2)
	{
		b->points[0][0] = -1200.0f;
		b->points[3][2] = -1200.0f;
		b->count = 4;
		b->points[1][0] = 0.0f;
		b->points[1][2] = 1200.0f;
		b->points[2][0] = 1200.0f;
		b->points[2][2] = 0.0f;
		b->points[3][0] = 0.0f;
		b->points[0][2] = 0.0f;
	}
}

/* 0x000ffe90  Demolition (file inferred: CDemolition_LiftShaft.cpp; missing from the table)
Constructor of a platform: its 4 spark systems. */
void *FUN_000ffe90(void *platform)
{
	struct DcPlatform *p = platform;
	int k;

	for (k = 0; k < 4; k++)
		CSparkPointSystem_Construct((struct CSparkPointSystem *)p->sparks[k]);
	return platform;
}

/* 0x000ffec0  Demolition (file inferred: CDemolition_LiftShaft.cpp; missing from the table)
Destructor of a platform: its spark systems, last first. */
void FUN_000ffec0(void *platform)
{
	struct DcPlatform *p = platform;
	int k;

	for (k = 3; k >= 0; k--)
		CSparkPointSystem_Destruct((struct CSparkPointSystem *)p->sparks[k]);
}

/* 0x000ffee0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Destructor: the lights, then the platforms (last first). */
void CDemolition_LiftShaft_Destruct(struct CDemolition_LiftShaft *self)
{
	int i;

	self->vtable = vtable_0026b880;
	for (i = 2; i >= 0; i--)
		FUN_000ed760((void *)DC_LIGHT(self, i));
	for (i = 3; i >= 0; i--)
		FUN_000ffec0(&self->platforms[i]);
}

/* 0x000fff50  Demolition/CDemolition_LiftShaft.cpp
Sets the 4 platforms up: actor instances turned to face the centre, the spark systems at the
brake nodes, the world object "platform0N", and the brakes open. Returns 1. */
int CDemolition_LiftShaft_InitPlatforms(struct CDemolition_LiftShaft *self)
{
	static const float s_up[3] = { 0.0f, 1.0f, 0.0f };
	struct DcPlatform *p;
	char name[256];
	float aa[4];
	float pos[4];
	char *sys;
	uint crc;
	int i;
	int j;
	int k;

	for (i = 0; i < 4; i++)
	{
		p = &self->platforms[i];
		baInitActorInstance((struct BActorInstance *)p->actor, (struct BActor *)self->platformModel, 1,
			s_DcLiftShaftFile, 0x102);
		aa[0] = 0.0f;
		aa[3] = (float)i * -1.57079637f /* 0x0026b8d4 */ + 3.14159274f /* 0x0026b01c */;
		aa[1] = 1.0f;
		aa[2] = 0.0f;
		bmAxisAngleToQuaternion(p->quat, aa);
		DC_F32(p->actor, 0xe0) = p->quat[0];
		DC_F32(p->actor, 0xe4) = p->quat[1];
		DC_F32(p->actor, 0xe8) = p->quat[2];
		DC_F32(p->actor, 0xec) = p->quat[3];
		baPrepActorInstance((struct BActorInstance *)p->actor, 1);
		for (k = 0; k < 4; k++)
		{
			p->sparkNodes[k] = baFindNode((struct BActorNode *)DC_PTR(p->actor, 0x134),
				DAT_00298b5c[k]);
			if (p->sparkNodes[k] == NULL)
				bkPrintf("ERROR: Failed to find node %s!\n", DAT_00298b5c[k]);
		}
		p->pos[0] = 0.0f;
		p->pos[1] = 0.0f;
		p->pos[2] = 0.0f;
		p->fallSpeed = 0.0f;
		p->enabled = 1;
		p->visible = 1;
		p->soundStarted = 0;
		p->falling = 0;
		p->braking = 0;
		for (k = 0; k < 4; k++)
		{
			sys = p->sparks[k];
			CSparkPointSystem_Init((struct CSparkPointSystem *)sys, NULL, 0x1e);
			baGetNodePosition((struct BActorInstance *)p->actor, (struct BActorNode *)p->sparkNodes[k],
				pos);
			CParticleSystem_SetPosition((struct CPSHeader *)sys, pos);
			DC_U8(sys, 0x59) = 0;        /* bounce */
			DC_F32(sys, 0x64) = 0.0f;    /* gravity */
			DC_F32(sys, 0x5c) = 110.0f;  /* speed */
			DC_I32(sys, 0x70) = 0x10;    /* life */
			DC_I32(sys, 0x74) = 0x20;
			DC_I32(sys, 0x78) = 8;       /* size */
			DC_I32(sys, 0x7c) = 0xc;
			CParticleSystem_SetTexture((struct CPSHeader *)sys, DC_I32(DC_GAME, 0x6c54));
			CParticleSystem_SetTexture2((struct CParticleSystem *)sys, DC_I32(DC_GAME, 0x6c78));
			FUN_000ab4c0((void *)sys, (undefined4 *)s_up);
		}
		sprintf(name, "platform0%d", i);
		crc = bkCRCString(name);
		for (j = 0; j < DC_I32(DC_GAME, 0x7bc); j++)
		{
			if (crc == DC_U32(DC_GAME, 0x2a8 + j * 0x90))
			{
				p->worldObject = DC_GAME + 0x21c + j * 0x90;
				break;
			}
		}
		for (k = 0; k < 2; k++)
		{
			p->anims[k] = baFindSubObject((struct BActorInstance *)p->actor, DAT_00298b6c[k]);
			if (p->anims[k] == NULL)
				bkPrintf("ERROR: Failed to find anim seg %s!\n", DAT_00298b6c[k]);
		}
		dc_platform_brake(p, p->anims[1]);
	}
	return 1;
}

/* 0x00100240  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Shutdown: models, lights and platforms. */
void CDemolition_LiftShaft_Shutdown(struct CDemolition_LiftShaft *self)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (self->shaftModels[i] != NULL)
		{
			bkDeleteResource((struct bkResource *)self->shaftModels[i]);
			self->shaftModels[i] = NULL;
		}
	}
	if (self->platformModel != NULL)
	{
		bkDeleteResource((struct bkResource *)self->platformModel);
		self->platformModel = NULL;
	}
	for (i = 0; i < 3; i++)
		DC_LIGHT_RELEASE(DC_LIGHT(self, i));
	CDemolition_LiftShaft_ReleasePlatforms(self);
}

/* 0x001002b0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Starts the fall (state 3, for 300-900 frames) and its sound. */
void CDemolition_LiftShaft_Start(struct CDemolition_LiftShaft *self)
{
	self->started = 1;
	self->state = 3;
	self->timer = (dc_unit() + 0.5f) * 600.0f; /* 0x00255444 */
	self->sound = FUN_0018e450((void *)(DC_GAME + 0x79d0), 0x11, 0xff, 0, 0, 0);
	self->soundOn = 1;
}

/* 0x00100330  Demolition (file inferred: CDemolition_LiftShaft.cpp)
State 0: waits (once started), then state 2 for 75-125 frames. */
void CDemolition_LiftShaft_Wait(struct CDemolition_LiftShaft *self, float dt)
{
	if (self->started != 0)
		self->timer = self->timer - dt;
	if (self->timer < 0.0f)
	{
		self->state = 2;
		self->timer = (dc_unit() + 0.5f) * 50.0f + 50.0f; /* 0x002559b8 */
	}
}

/* 0x001003b0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
State 1: slows down; stopped, waits 50-150 frames (state 0). */
void CDemolition_LiftShaft_SlowDown(struct CDemolition_LiftShaft *self, float dt)
{
	float v;

	v = self->speed - dt * 0.5f;
	self->speed = v;
	if (v < 0.0f)
	{
		self->speed = 0.0f;
		self->state = 0;
		self->timer = (dc_unit() + 0.5f) * 100.0f; /* 0x00255544 */
	}
}

/* 0x00100420  Demolition (file inferred: CDemolition_LiftShaft.cpp)
State 2: waits, then falls again (state 3, for 300-900 frames). */
void CDemolition_LiftShaft_WaitFall(struct CDemolition_LiftShaft *self, float dt)
{
	float t;

	t = self->timer - dt;
	self->timer = t;
	if (t < 0.0f)
	{
		self->state = 3;
		self->timer = (dc_unit() + 0.5f) * 600.0f;
	}
}

/* 0x00100490  Demolition (file inferred: CDemolition_LiftShaft.cpp)
State 4: braking. Above speed 30 the brakes' sparks fly faster with the speed; below, the
brakes open. The speed falls by 1% a frame; under 1 the lift stops and waits 50-150 frames. */
void CDemolition_LiftShaft_Brake(struct CDemolition_LiftShaft *self)
{
	struct DcPlatform *p;
	float s;
	float v;
	int i;
	int k;

	if (30.0f < self->speed) /* 0x002553a8 */
	{
		s = self->speed * 0.0125f; /* 0x00268684 */
		for (i = 0; i < 4; i++)
		{
			for (k = 0; k < 4; k++)
			{
				if (self->platforms[i].falling == 0)
					DC_F32(self->platforms[i].sparks[k], 0x6c) = s;
			}
		}
	}
	else
	{
		for (i = 0; i < 4; i++)
		{
			p = &self->platforms[i];
			if (p->falling == 0 && p->braking != 0)
			{
				p->braking = 0;
				dc_platform_brake(p, p->anims[1]);
			}
		}
	}
	v = self->speed * 0.99000001f; /* 0x002687dc */
	self->speed = v;
	if (v <= 1.0f)
	{
		self->speed = 0.0f;
		self->state = 0;
		self->timer = (dc_unit() + 0.5f) * 100.0f;
	}
}

/* 0x001005c0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Scrolls the shaft with the fall speed (first the drop to 2000) and the shake; when the lowest
section has passed, a wall section is added on top. */
void CDemolition_LiftShaft_UpdateShaft(struct CDemolition_LiftShaft *self, float dt)
{
	float s;
	float d;
	float v;
	float h;

	s = self->shake + self->shakeVel;
	self->shake = s;
	self->shakeVel = self->shakeVel * 0.920000017f - s * 0.800000012f; /* 0x0026b914, 0x00257c78 */
	d = dt * self->speed;
	if (2000.0f <= self->drop) /* 0x00257c68 */
	{
		self->scroll = d + self->scroll;
	}
	else
	{
		v = d + self->drop;
		self->drop = v;
		if (2000.0f < v)
		{
			self->drop = 2000.0f;
			self->scroll = -(2000.0f - v);
		}
	}
	self->distance = d + self->distance;
	h = self->ring[self->ringHead].seg->height;
	if (!(h < self->scroll))
		return;
	v = self->scroll;
	do
		v = v - h;
	while (h < v);
	self->scroll = v;
	self->ringHead = (self->ringHead + 1) % 10;
	self->ringCount = self->ringCount - 1;
	dc_ring_push(self, &self->sections[1], 1);
}

/* 0x001006e0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Barrier pattern 0: a blue ring of 4 every 8000; after 4-9 rings another pattern. Returns
what the original leaves in EAX (not used). */
uint CDemolition_LiftShaft_Pattern0(struct CDemolition_LiftShaft *self, float dt)
{
	uint r;

	self->barrierDist = dt * self->speed + self->barrierDist;
	if (8000.0f < self->barrierDist)
	{
		self->barrierDist = 0.0f;
		if (self->barrierCount < 10)
		{
			self->kind = 1;
			self->colour[0] = (short)DAT_00298b80[0];
			self->colour[1] = (short)DAT_00298b80[1];
			self->colour[2] = (short)DAT_00298b80[2];
			self->shape = 2;
			CDemolition_LiftShaft_AddBarrier(self, -15000.0f);
			self->barrierCounter = self->barrierCounter + 1;
		}
	}
	r = (uint)self->barrierCounter;
	if (self->barrierEvery <= self->barrierCounter)
	{
		dc_next_pattern(self);
		r = (uint)dc_rand();
		self->barrierEvery = (int)(r % 6) + 4;
		r = r / 6;
	}
	return r;
}

/* 0x00100800  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Barrier pattern 1: orange pairs every 2000, alternately across x and z; after 6-9 another. */
void CDemolition_LiftShaft_Pattern1(struct CDemolition_LiftShaft *self, float dt)
{
	self->barrierDist = dt * self->speed + self->barrierDist;
	if (2000.0f < self->barrierDist)
	{
		self->barrierDist = 0.0f;
		if (self->barrierCount < 10)
		{
			self->kind = 2;
			self->shape = self->shape == 0;
			self->colour[0] = (short)DAT_00298b8c[0];
			self->colour[1] = (short)DAT_00298b8c[1];
			self->colour[2] = (short)DAT_00298b8c[2];
			CDemolition_LiftShaft_AddBarrier(self, -15000.0f);
			self->barrierCounter = self->barrierCounter + 1;
		}
	}
	if (self->barrierEvery <= self->barrierCounter)
	{
		dc_next_pattern(self);
		self->barrierEvery = (dc_rand() & 3) + 6;
	}
}

/* 0x00100920  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Barrier pattern 2: a red ring of 4 every 8000; after 6-9 another. */
void CDemolition_LiftShaft_Pattern2(struct CDemolition_LiftShaft *self, float dt)
{
	self->barrierDist = dt * self->speed + self->barrierDist;
	if (8000.0f < self->barrierDist)
	{
		self->barrierDist = 0.0f;
		if (self->barrierCount < 10)
		{
			self->shape = 2;
			self->kind = 4;
			self->colour[0] = (short)DAT_00298ba4[0];
			self->colour[1] = (short)DAT_00298ba4[1];
			self->colour[2] = (short)DAT_00298ba4[2];
			CDemolition_LiftShaft_AddBarrier(self, -15000.0f);
			self->barrierCounter = self->barrierCounter + 1;
		}
	}
	if (self->barrierEvery <= self->barrierCounter)
	{
		dc_next_pattern(self);
		self->barrierEvery = (dc_rand() & 3) + 6;
	}
}

/* 0x00100a40  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Barrier pattern 3: green pairs every 4000, alternately across x and z; after 6-9 another. */
void CDemolition_LiftShaft_Pattern3(struct CDemolition_LiftShaft *self, float dt)
{
	self->barrierDist = dt * self->speed + self->barrierDist;
	if (4000.0f < self->barrierDist) /* 0x0026b918 */
	{
		self->barrierDist = 0.0f;
		if (self->barrierCount < 10)
		{
			self->kind = 3;
			self->shape = self->shape == 0;
			self->colour[0] = (short)DAT_00298b98[0];
			self->colour[1] = (short)DAT_00298b98[1];
			self->colour[2] = (short)DAT_00298b98[2];
			CDemolition_LiftShaft_AddBarrier(self, -15000.0f);
			self->barrierCounter = self->barrierCounter + 1;
		}
	}
	if (self->barrierEvery <= self->barrierCounter)
	{
		dc_next_pattern(self);
		self->barrierEvery = (dc_rand() & 3) + 6;
	}
}

/* 0x00100b60  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Updates the platforms: a falling one speeds up (to 50.5) and drops away with its brakes on
and a falling sound (panned left for the first two, right for the others) that fades with
the distance; every platform's actor follows its position, and its spark systems emit while
the brakes are on. */
void CDemolition_LiftShaft_UpdatePlatforms(struct CDemolition_LiftShaft *self, float dt)
{
	struct DcPlatform *p;
	float v;
	int i;
	int k;

	for (i = 0; i < 4; i++)
	{
		p = &self->platforms[i];
		if (p->falling == 1)
		{
			p->fallSpeed = dt * 5.0f + p->fallSpeed; /* 0x002536c4 */
			if (50.5f < p->fallSpeed) /* 0x0026b920 */
				p->fallSpeed = 50.5f;
			p->pos[1] = p->pos[1] - dt * p->fallSpeed;
			p->pos[1] = dt * self->speed + p->pos[1];
			if (p->braking == 0)
			{
				p->braking = 1;
				dc_platform_brake(p, p->anims[0]);
			}
			for (k = 0; k < 4; k++)
				DC_F32(p->sparks[k], 0x20) = p->pos[1];
			if (p->soundStarted == 0)
			{
				switch (i)
				{
				case 0:
				case 1:
					p->sound = FUN_0018e450((void *)(DC_GAME + 0x79d0), 0x13, 0xff, 0xff, 0, 0);
					p->soundStarted = 1;
					break;
				case 2:
				case 3:
					p->sound = FUN_0018e450((void *)(DC_GAME + 0x79d0), 0x13, 0xff, -0xff, 0, 0);
					p->soundStarted = 1;
					break;
				default:
					p->soundStarted = 1;
					break;
				}
			}
			else
			{
				v = fabsf(p->pos[1]) * 6.66666674e-05f; /* 0x0026b91c */
				if (1.0f < v)
					v = 1.0f;
				else if (v < 0.0f)
					v = 0.0f;
				FUN_0018e4b0(p->sound, (int)((1.0f - v) * 255.0f)); /* 0x002537d0 */
			}
		}
		DC_F32(p->actor, 0xe0) = p->quat[0];
		DC_F32(p->actor, 0xe4) = p->quat[1];
		DC_F32(p->actor, 0xe8) = p->quat[2];
		DC_F32(p->actor, 0xec) = p->quat[3];
		DC_F32(p->actor, 0xc0) = p->pos[0];
		DC_F32(p->actor, 0xc4) = p->pos[1];
		DC_F32(p->actor, 0xc8) = p->pos[2];
		DC_F32(p->actor, 0xcc) = p->pos[3];
		baUpdateAnimations((struct BActorInstance *)p->actor, (struct BActorNode *)DC_PTR(p->actor, 0x134),
			dt, NULL);
		baPrepActorInstance((struct BActorInstance *)p->actor, 1);
		for (k = 0; k < 4; k++)
		{
			if (p->braking != 0)
			{
				FUN_0019d0c0((int *)p->sparks[k], 1);
				DC_U8(p->sparks[k], 0x58) = 1; /* active */
			}
			DC_PS_UPDATE(p->sparks[k], dt);
		}
	}
}

/* 0x00100ef0  Demolition (file inferred: CDemolition_LiftShaft.cpp; missing from the table)
Draw: the shaft (when the variation draws it) and the platforms. */
void FUN_00100ef0(struct CDemolition_LiftShaft *self)
{
	FUN_0018e540(0x20);
	bdPushRenderState(3, 1, 0);
	if (DAT_00297520[DC_I32(DC_GAME, 0x28) * 15] != 0)
		CDemolition_LiftShaft_DrawShaft(self);
	CDemolition_LiftShaft_DrawPlatforms(self);
	bdPopRenderState(3);
	FUN_001bea60();
}

/* 0x00100f60  Demolition (file inferred: CDemolition_LiftShaft.cpp; missing from the table)
DrawEffects: the sparks of the visible platforms and the barriers. */
void FUN_00100f60(struct CDemolition_LiftShaft *self)
{
	int i;
	int k;

	bdPushRenderState(8, 0, 0);
	for (i = 0; i < 4; i++)
	{
		if (self->platforms[i].visible != 0)
		{
			for (k = 0; k < 4; k++)
				DC_PS_CALL(self->platforms[i].sparks[k], 2);
		}
	}
	CDemolition_LiftShaft_DrawBarriers(self);
	bdPopRenderState(8);
}

/* 0x00100fd0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Restarts the shaft ring: the top section, then 8 wall sections (opaque only). */
void CDemolition_LiftShaft_ResetShaft(struct CDemolition_LiftShaft *self)
{
	int i;

	self->ringCount = 0;
	self->ringHead = 0;
	self->sectionCounter = 0;
	self->ring[0].seg = &self->sections[0];
	self->ring[0].opaqueOnly = 0;
	if (self->ringCount < 10)
		self->ringCount = self->ringCount + 1;
	self->sectionCounter = self->sectionCounter + 1;
	for (i = 0; i < 8; i++)
		dc_ring_push(self, &self->sections[1], 1);
}

/* 0x001011c0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Restarts the barriers: 3 blue rings at -3000, -6000 and -9000. */
void CDemolition_LiftShaft_ResetBarriers(struct CDemolition_LiftShaft *self)
{
	float y;
	int i;

	self->barrierCount = 0;
	self->barrierHead = 0;
	self->kind = 1;
	self->colour[0] = (short)DAT_00298b80[0];
	self->colour[1] = (short)DAT_00298b80[1];
	y = -3000.0f;
	self->colour[2] = (short)DAT_00298b80[2];
	self->shape = 2;
	for (i = 0; i < 3; i++)
	{
		CDemolition_LiftShaft_AddBarrier(self, y);
		y = y - 3000.0f; /* 0x0026b924 */
	}
	self->barrierDist = 0.0f;
}

/* 0x00101250  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Sways the camera (game + 0x1370, + 0x1374): towards a falling platform's side while one is
in view (200-10000), else to random places at random times. */
void CDemolition_LiftShaft_SwayCamera(struct CDemolition_LiftShaft *self, float dt)
{
	struct DcPlatform *p;
	float t;
	int left;
	int right;
	int any;
	int i;

	if (self->started == 0)
		return;
	left = 0;
	right = 0;
	any = 0;
	for (i = 0; i < 4; i++)
	{
		p = &self->platforms[i];
		if (p->falling == 1 && 200.0f < p->pos[1] && p->pos[1] < 10000.0f) /* 0x002657a0 */
		{
			switch (i)
			{
			case 0:
			case 1:
				left = 1;
				break;
			case 2:
			case 3:
				right = 1;
				break;
			default:
				break;
			}
			any = 1;
		}
	}
	if (any)
	{
		if (right)
			DC_F32(DC_GAME, 0x1370) = left ? 0.0f : 200.0f;
		else
			DC_F32(DC_GAME, 0x1370) = -200.0f;
	}
	else
	{
		t = self->camTimerX - dt;
		self->camTimerX = t;
		if (t <= 0.0f)
		{
			self->camTimerX = (dc_unit() + 0.5f) * 120.0f; /* 0x00255b50 */
			DC_F32(DC_GAME, 0x1370) = dc_unit() * 400.0f - 200.0f; /* 0x00255914 */
		}
	}
	t = self->camTimerY - dt;
	self->camTimerY = t;
	if (t <= 0.0f)
	{
		self->camTimerY = (dc_unit() + 0.5f) * 80.0f;
		DC_F32(DC_GAME, 0x1374) = dc_unit() * 100.0f - 50.0f;
	}
}

/* 0x00101480  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Constructor: the platforms and the 3 lights. */
struct CDemolition_LiftShaft *CDemolition_LiftShaft_Construct(struct CDemolition_LiftShaft *self)
{
	int i;

	self->vtable = vtable_0026b880;
	for (i = 0; i < 4; i++)
		FUN_000ffe90(&self->platforms[i]);
	for (i = 0; i < 3; i++)
		FUN_000ed640((undefined4 *)DC_LIGHT(self, i));
	return self;
}

/* 0x00101500  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Deleting destructor (vtable slot 0). */
struct CDemolition_LiftShaft *CDemolition_LiftShaft_DeletingDtor(struct CDemolition_LiftShaft *self,
                                                                 unsigned char flags)
{
	CDemolition_LiftShaft_Destruct(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00101520  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Fades the ambient light in from the variation's colour over the first 2000 of the fall, runs
the barrier pattern and moves the barriers. */
void CDemolition_LiftShaft_UpdateLights(struct CDemolition_LiftShaft *self, float dt)
{
	const int *s;
	float v;
	float t;

	if (self->fadeIn != 0)
	{
		v = self->distance * 0.000500000024f; /* 0x00257c64 */
		if (1.0f < v)
			v = 1.0f;
		else if (v < 0.0f)
			v = 0.0f;
		s = &DAT_00297520[DC_I32(DC_GAME, 0x28) * 15];
		t = 1.0f - v;
		baSetAmbientLight(s[9] + (int)((float)(0x7f - s[9]) * t),
			s[10] + (int)((float)(0x7f - s[10]) * t), s[11] + (int)((float)(0x40 - s[11]) * t));
		if (1.0f <= v)
			self->fadeIn = 0;
	}
	switch (self->pattern)
	{
	case 0:
		CDemolition_LiftShaft_Pattern0(self, dt);
		break;
	case 1:
		CDemolition_LiftShaft_Pattern1(self, dt);
		break;
	case 2:
		CDemolition_LiftShaft_Pattern2(self, dt);
		break;
	case 3:
		CDemolition_LiftShaft_Pattern3(self, dt);
		break;
	default:
		break;
	}
	CDemolition_LiftShaft_UpdateBarriers(self, dt);
}

/* 0x001016a0  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Initialise (variation 3): loads the shaft sections and the platform model, and sets
everything up. Returns 0 if a model is missing. */
int CDemolition_LiftShaft_Initialise(struct CDemolition_LiftShaft *self, void *package)
{
	void *res;
	int i;

	for (i = 0; i < 4; i++)
	{
		res = bLoadResource((struct bkPackage *)package, DAT_00298b38[i], 6, 0xdefa);
		self->shaftModels[i] = res;
		if (res == NULL)
		{
			bkPrintf("*** ERROR *** Could not find lift shaft object [%s]\n", DAT_00298b38[i]);
			return 0;
		}
	}
	self->platformModel = bLoadResource((struct bkPackage *)package, DAT_00298b58[0], 1, 0xdefa);
	bkPrintf("Loading lift :%s\n", DAT_00298b58[0]);
	if (self->platformModel == NULL)
	{
		bkPrintf("ERROR: Failed to load liftshaft model %s\n", DAT_00298b58[0]);
		return 0;
	}
	for (i = 0; i < 4; i++)
	{
		self->sections[i].model = self->shaftModels[i];
		self->sections[i].height = DAT_00298b48[i];
	}
	self->speed = 0.0f;
	self->drop = 410.0f;
	self->scroll = 0.0f;
	self->state = 0;
	self->started = 0;
	self->distance = 0.0f;
	self->shake = 0.0f;
	self->shakeVel = 0.0f;
	self->soundOn = 0;
	self->timer = (dc_unit() + 0.5f) * 100.0f;
	CDemolition_LiftShaft_ResetShaft(self);
	self->pattern = 0;
	self->barrierCounter = 0;
	self->barrierEvery = 0;
	self->fadeIn = 1;
	self->barrierDist = 0.0f;
	CDemolition_LiftShaft_ResetBarriers(self);
	for (i = 0; i < 3; i++)
		DC_LIGHT_INIT(DC_LIGHT(self, i));
	CDemolition_LiftShaft_InitPlatforms(self);
	self->camTimerX = (dc_unit() + 0.5f) * 120.0f;
	self->camTimerY = (dc_unit() + 0.5f) * 80.0f;
	return 1;
}

/* 0x00101900  Demolition (file inferred: CDemolition_LiftShaft.cpp)
Update (through the variation): the fall state, the shaft, lights and barriers, platforms,
camera, and the pitch of the fall sound (with the speed, at least 188). */
void CDemolition_LiftShaft_Update(struct CDemolition_LiftShaft *self, float dt)
{
	int pitch;

	switch (self->state)
	{
	case 0:
		CDemolition_LiftShaft_Wait(self, dt);
		break;
	case 1:
		CDemolition_LiftShaft_SlowDown(self, dt);
		break;
	case 2:
		CDemolition_LiftShaft_WaitFall(self, dt);
		break;
	case 3:
		CDemolition_LiftShaft_Fall(self, dt);
		break;
	case 4:
		CDemolition_LiftShaft_Brake(self);
		break;
	default:
		break;
	}
	CDemolition_LiftShaft_UpdateShaft(self, dt);
	CDemolition_LiftShaft_UpdateLights(self, dt);
	CDemolition_LiftShaft_UpdatePlatforms(self, dt);
	CDemolition_LiftShaft_SwayCamera(self, dt);
	if (self->soundOn != 0)
	{
		pitch = (int)(self->speed * 0.0125f * 22050.0f); /* 0x00268684, 0x0026b968 */
		if (pitch < 0xbc)
			pitch = 0xbc;
		FUN_0018e490(self->sound, pitch);
	}
}

/* 0x0026b880 */
const void *const vtable_0026b880[1] = { (const void *)CDemolition_LiftShaft_DeletingDtor };
