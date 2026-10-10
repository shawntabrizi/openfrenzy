/*
Demolition/CDemolition_Particle.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Particle.cpp

The particle systems of Demolition (classes and layouts: demolition_c_particle.h). They use the
ClassLib particle header and the bdisplay beam, sprite, polyline and point sprite helpers in the
same way as ClassLib/CParticleSystem.cpp (classlib_fx).

Random numbers come from the game's inlined MSVC rand() (dc_rand); a "random unit" is
rand() * (1 / 32767) (0x00253740). The x87 code computes in extended precision; this port
computes in float.
*/

#include "demolition_c_internal.h"
#include "demolition_c_particle.h"

#define DC_VERTEX 0x1c          /* bytes per strip vertex (FVF 0x144) */
#define DC_RAND_UNIT 3.05185094e-05f /* 0x00253740 */

/* 0x002ba99c  the frame counter (core) */
extern int DAT_002ba99c;
/* 0x00285644  the collision speed epsilon (0.001; bumpercars changes it) */
extern float DAT_00285644;

/* the vtables (rebuilt with symbols) */
extern const void *const vtable_0026b9ec[6];
extern const void *const vtable_0026ba04[6];
extern const void *const vtable_0026ba1c[6];
extern const void *const vtable_0026ba34[6];
extern const void *const vtable_0026ba4c[6];
extern const void *const vtable_0026ba64[6];
extern const void *const vtable_0026ba7c[6];

/* virtual calls of the ClassLib layout */
#define DC_PS_VT(obj) (*(void *const *const *)(obj))
#define DC_PS_Reset(obj) (((void (*)(void *))DC_PS_VT(obj)[3])(obj))
#define DC_PS_Emit(obj, p) (((void (*)(void *, CParticle *))DC_PS_VT(obj)[4])((obj), (p)))

static float dc_unit(void)
{
	return (float)dc_rand() * DC_RAND_UNIT;
}

/* frees the vertex buffer and the particles (the Release of most classes) */
static void dc_release(CPSHeader *h, int vb)
{
	if (vb != 0)
		bdVertexBufferDestroy((struct bdVertexBuffer *)vb);
	if (h->particles != NULL)
	{
		bkHeapFree(h->particles);
		h->particles = NULL;
	}
	h->maxParticles = 0;
	h->numAlive = 0;
}

/* kills every particle */
static void dc_kill_all(CPSHeader *h)
{
	int i;

	for (i = 0; i < h->maxParticles; i++)
		h->particles[i].alive = 0;
}

/* writes each alive particle as a beam from its previous position (10 vertices each: the
   strip's first and last vertices doubled); returns the end of the vertices written */
static byte *dc_write_beams(CPSHeader *h, byte *base, int fixedWidth)
{
	CParticle *p;
	int c0[4];
	int c1[4];
	byte *w;
	float w0;
	int i;

	w = base;
	for (i = 0; i < h->maxParticles; i++)
	{
		p = &h->particles[i];
		if (p->alive == 0)
			continue;
		c0[0] = p->colour[0] >> 7;
		c0[1] = p->colour[1] >> 7;
		c0[2] = p->colour[2] >> 7;
		c0[3] = 0x80;
		c1[0] = p->colour2[0] >> 7;
		c1[1] = p->colour2[1] >> 7;
		c1[2] = p->colour2[2] >> 7;
		c1[3] = 0x80;
		p->pos[3] = 1.0f;
		p->prevPos[3] = 1.0f;
		w0 = fixedWidth ? 1.0f : p->size;
		if (bdDrawBeam3D(p->prevPos, w0, w0, c0, p->pos, p->size, p->size, c1, NULL,
			(float *)(w + DC_VERTEX), 0.0f) != 0)
		{
			memcpy(w, w + DC_VERTEX, DC_VERTEX);
			memcpy(w + 9 * DC_VERTEX, w + 8 * DC_VERTEX, DC_VERTEX);
			w = w + 10 * DC_VERTEX;
		}
	}
	return w;
}

/* additive strips: the render state around a strip draw */
static void dc_begin_strips(void)
{
	bdSetProjectionMode(1);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
}

static void dc_end_strips(void)
{
	bdSetProjectionMode(1);
	bdPopRenderState(6);
	bdPopRenderState(5);
}

/* moves a beam particle and fades it (the Update of DcSparkSystem and DcFlashSparkSystem);
   the previous position follows every other frame */
static void dc_move_spark(CParticle *p, float dt, float drag, float gravity)
{
	if ((DAT_002ba99c & 1) != 0)
	{
		p->prevPos[0] = p->pos[0];
		p->prevPos[1] = p->pos[1];
		p->prevPos[2] = p->pos[2];
		p->prevPos[3] = p->pos[3];
	}
	p->pos[0] = dt * p->vel[0] + p->pos[0];
	p->pos[1] = dt * p->vel[1] + p->pos[1];
	p->pos[2] = dt * p->vel[2] + p->pos[2];
	p->vel[0] = drag * p->vel[0];
	p->vel[1] = drag * p->vel[1];
	p->vel[2] = drag * p->vel[2];
	p->colour[0] = p->colour[0] + p->dColour[0];
	p->colour[1] = p->colour[1] + p->dColour[1];
	p->colour[2] = p->colour[2] + p->dColour[2];
	p->colour2[0] = p->colour2[0] + p->dColour2[0];
	p->vel[1] = dt * gravity + p->vel[1];
	p->colour2[1] = p->colour2[1] + p->dColour2[1];
	p->colour2[2] = p->colour2[2] + p->dColour2[2];
}

/* ======================================================================== DcSparkSystem */

/* 0x001025e0  Demolition/CDemolition_Particle.cpp
Destructor (vtable only). */
void DcSparkSystem_Destruct(DcSparkSystem *self)
{
	self->h.vtable = vtable_0026b9ec;
}

/* 0x001025f0  Demolition/CDemolition_Particle.cpp
Default parameters: orange sparks. */
void DcSparkSystem_DefaultParams(DcSparkParams *p)
{
	p->active = 0;
	p->bounce = 1;
	p->gravity = -0.200000003f;
	p->speed = 45.0f;
	p->drag = 0.970000029f;
	p->lifeMin = 0x19;
	p->lifeRange = 0x32;
	p->colour[0] = 0x10;
	p->colour[1] = 5;
	p->colour[2] = 1;
	p->colour2[0] = 0x80;
	p->colour2[1] = 100;
	p->colour2[2] = 0x28;
	p->sizeMin = 3;
	p->sizeRange = 6;
	p->unknown70 = 0.600000024f;
}

/* 0x00102660  Demolition/CDemolition_Particle.cpp
Draw (vtable slot 2): the sparks as beams, additive. */
void DcSparkSystem_Draw(DcSparkSystem *self)
{
	byte *base;
	byte *w;
	int n;

	if (self->p.active == 0 || self->h.numAlive <= 0)
		return;
	bdSetWorldIdentity();
	bdSetTexture(0, (struct bdTexture *)self->h.texture);
	base = bdVertexBufferLock((struct bdVertexBuffer *)self->vb, 4);
	if (base == NULL)
		return;
	w = dc_write_beams(&self->h, base, 0);
	bdVertexBufferUnlock((struct bdVertexBuffer *)self->vb);
	dc_begin_strips();
	n = (int)(w - base) / DC_VERTEX;
	if (n != 0)
		bdDrawVertexBuffer(4, (struct bdVertexBuffer *)self->vb, 0, n);
	dc_end_strips();
}

/* 0x00102830  Demolition/CDemolition_Particle.cpp
Reset (vtable slot 3): inactive, every particle dead. */
void DcSparkSystem_Reset(DcSparkSystem *self)
{
	self->p.active = 0;
	self->h.numAlive = 0;
	dc_kill_all(&self->h);
}

/* 0x00102870  Demolition/CDemolition_Particle.cpp
Emits every particle of a system at once (through its Emit slot). Also used by other
games (meta, rowing). */
void DcParticles_EmitAll(CPSHeader *self)
{
	int i;

	self->numAlive = 0;
	for (i = 0; i < self->maxParticles; i++)
		DC_PS_Emit(self, &self->particles[i]);
}

/* 0x00103360  Demolition/CDemolition_Particle.cpp
Constructor (vtable only). */
DcSparkSystem *DcSparkSystem_Construct(DcSparkSystem *self)
{
	self->h.vtable = vtable_0026b9ec;
	return self;
}

/* 0x00103370  Demolition/CDemolition_Particle.cpp
Deleting destructor (vtable slot 5). */
DcSparkSystem *DcSparkSystem_DeletingDtor(DcSparkSystem *self, unsigned char flags)
{
	self->h.vtable = vtable_0026b9ec;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00103390  Demolition/CDemolition_Particle.cpp
Init: parameters (or the defaults), particles and the vertex buffer (10 per particle).
Returns 0 when the buffer cannot be created. */
int DcSparkSystem_Init(DcSparkSystem *self, DcSparkParams *params, int maxParticles)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	if (params == NULL)
		DcSparkSystem_DefaultParams(&self->p);
	else
		memcpy(&self->p, params, sizeof(DcSparkParams));
	self->h.maxParticles = maxParticles;
	self->h.particles = bkHeapAlloc(maxParticles * 0x68, s_DcParticleFile, 0x4a, 0x1001);
	if (bdVertexBufferCreate((struct bdVertexBuffer **)&self->vb, self->h.maxParticles * 10, 0x144,
		0, 1) != 0)
	{
		DC_PS_Reset(self);
		return 1;
	}
	return 0;
}

/* 0x00103430  Demolition/CDemolition_Particle.cpp
Release (vtable slot 0). */
void DcSparkSystem_Release(DcSparkSystem *self)
{
	dc_release(&self->h, self->vb);
}

/* 0x00103470  Demolition/CDemolition_Particle.cpp
Update (vtable slot 1): moves and fades the sparks; inactive when none is left. */
void DcSparkSystem_Update(DcSparkSystem *self, float dt)
{
	CParticle *p;
	short age;
	int i;

	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	for (i = 0; i < self->h.maxParticles; i++)
	{
		p = &self->h.particles[i];
		if (p->alive == 0)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age < p->life)
		{
			dc_move_spark(p, dt, self->p.drag, self->p.gravity);
			self->h.numAlive = self->h.numAlive + 1;
		}
		else
		{
			p->alive = 0;
		}
	}
	if (self->h.numAlive < 1)
		self->p.active = 0;
}

/* ======================================================================== chunks */

/* draws the chunks: plain while flying, blended with their fade brightness while fading */
static void dc_draw_chunks(DcChunk *c, int n)
{
	int b;
	int i;

	for (i = 0; i < n; i++)
	{
		if (c[i].alive == 0)
			continue;
		if (c[i].phase == 0)
		{
			baDrawActorInstance((struct BActorInstance *)c[i].actor, 7);
		}
		else if (c[i].phase == 1)
		{
			bdPushRenderState(6, 1, 0);
			bdSetRenderState(10, 0, 0);
			b = c[i].fade >> 7;
			if (b < 0)
				b = 0;
			baSetBrightness(b);
			baDrawActorInstance((struct BActorInstance *)c[i].actor, 7);
			baSetBrightness(0x100);
			bdPopRenderState(6);
		}
	}
}

/* counts a chunk's life: flying for `life` frames, then fading for `fadeTime`. Returns 0 when
   the chunk has just died (the original then skips it) */
static int dc_chunk_age(DcChunk *c)
{
	short age;

	if (c->phase == 0)
	{
		age = c->age;
		c->age = age + 1;
		if (c->life < age)
		{
			c->phase = 1;
			c->age = 0;
		}
	}
	else if (c->phase == 1)
	{
		age = c->age;
		c->fade = c->fade + c->fadeStep;
		c->age = age + 1;
		if (c->fadeTime < (int)age)
		{
			c->alive = 0;
			return 0;
		}
	}
	return 1;
}

/* moves a chunk and bounces it off the world's collision mesh (game + 0x2a4) */
static void dc_chunk_move(DcChunk *c, DcChunkParams *prm, float dt)
{
	float hit[4];
	float n[4];
	float d;
	float e;
	float rx;
	float rz;

	hit[3] = c->pos[3]; /* not written by the ray test (the original keeps a stack word) */
	c->pos[0] = dt * c->vel[0] + c->pos[0];
	c->pos[1] = dt * c->vel[1] + c->pos[1];
	c->pos[2] = dt * c->vel[2] + c->pos[2];
	d = prm->drag;
	c->vel[0] = d * c->vel[0];
	c->vel[1] = d * c->vel[1];
	c->vel[2] = d * c->vel[2];
	c->vel[1] = dt * prm->gravity + c->vel[1];
	if (prm->bounce != 0 &&
	    (DAT_00285644 < fabsf(c->vel[0]) || DAT_00285644 < fabsf(c->vel[1]) ||
	     DAT_00285644 < fabsf(c->vel[2])) &&
	    DcCollide_Ray(DC_PTR(DC_GAME, 0x2a4), c->pos, c->vel, hit, n) != 0)
	{
		c->pos[0] = hit[0];
		c->pos[1] = hit[1];
		c->pos[2] = hit[2];
		c->pos[3] = hit[3];
		e = prm->restitution;
		d = n[0] * c->vel[0] + n[2] * c->vel[2] + n[1] * c->vel[1];
		rx = c->vel[0] - n[0] * d;
		rz = c->vel[2] - n[2] * d;
		c->vel[0] = rx - n[0] * d * e;
		c->vel[1] = (c->vel[1] - n[1] * d) - n[1] * d * e;
		c->vel[2] = rz - n[2] * d * e;
	}
}

/* rolls a chunk by its horizontal speed; returns 0 when it does not move */
static int dc_chunk_roll(DcChunk *c, int positiveOnly)
{
	float aa[4];
	float q[4];
	float vx;
	float vz;
	float len;

	vz = c->vel[2];
	vx = c->vel[0];
	len = sqrtf(vx * vx + vz * vz);
	if (positiveOnly ? !(0.0f < len) : len == 0.0f)
		return 0;
	aa[0] = -(vz * (1.0f / len));
	aa[1] = 0.0f;
	aa[2] = (1.0f / len) * vx;
	aa[3] = ((len * 1.57079625f) / (c->radius + c->radius)) * c->spin;
	bmAxisAngleToQuaternion(q, aa);
	bmQuaternionMultiply(c->quat, c->quat, q);
	FUN_00198590(c->quat);
	return 1;
}

/* sets the actor's rotation from the chunk's when it is not all zero */
static void dc_chunk_set_rotation(DcChunk *c)
{
	if (c->quat[0] != 0.0f || c->quat[1] != 0.0f || c->quat[2] != 0.0f || c->quat[3] != 0.0f)
	{
		DC_F32(c->actor, 0xe0) = c->quat[0];
		DC_F32(c->actor, 0xe4) = c->quat[1];
		DC_F32(c->actor, 0xe8) = c->quat[2];
		DC_F32(c->actor, 0xec) = c->quat[3];
		baPrepActorInstance((struct BActorInstance *)c->actor, 1);
	}
}

/* starts a chunk at the emitter, unturned (the common part of 0x00103850 and 0x00104000) */
static void dc_chunk_place(DcChunk *c)
{
	c->quat[0] = 0.0f;
	c->quat[1] = 0.0f;
	c->quat[2] = 0.0f;
	c->quat[3] = 1.0f;
}

static void dc_chunk_set_actor(DcChunk *c)
{
	DC_F32(c->actor, 0xe0) = c->quat[0];
	DC_F32(c->actor, 0xe4) = c->quat[1];
	DC_F32(c->actor, 0xe8) = c->quat[2];
	DC_F32(c->actor, 0xec) = c->quat[3];
	DC_F32(c->actor, 0xc0) = c->pos[0];
	DC_F32(c->actor, 0xc4) = c->pos[1];
	DC_F32(c->actor, 0xc8) = c->pos[2];
	DC_F32(c->actor, 0xcc) = c->pos[3];
	baPrepActorInstance((struct BActorInstance *)c->actor, 1);
}

/* ======================================================================== DcDebrisSystem */

/* 0x001028a0  Demolition/CDemolition_Particle.cpp
Constructor (vtable only). */
DcDebrisSystem *DcDebrisSystem_Construct(DcDebrisSystem *self)
{
	self->h.vtable = vtable_0026ba04;
	return self;
}

/* 0x001028b0  Demolition/CDemolition_Particle.cpp
Destructor (vtable only). */
void DcDebrisSystem_Destruct(DcDebrisSystem *self)
{
	self->h.vtable = vtable_0026ba04;
}

/* 0x001028c0  Demolition/CDemolition_Particle.cpp
Release (vtable slot 0): the chunks' actor instances. */
void DcDebrisSystem_Release(DcDebrisSystem *self)
{
	int i;

	for (i = 0; i < 3; i++)
		baDeleteActorInstance((struct BActorInstance *)self->chunks[i].actor);
}

/* 0x001028f0  Demolition/CDemolition_Particle.cpp
Draw (vtable slot 2). */
void DcDebrisSystem_Draw(DcDebrisSystem *self)
{
	if (self->p.active != 0 && self->numChunks != 0)
		dc_draw_chunks(self->chunks, 3);
}

/* 0x001029a0  Demolition/CDemolition_Particle.cpp
Reset (vtable slot 3). */
void DcDebrisSystem_Reset(DcDebrisSystem *self)
{
	int i;

	self->p.active = 0;
	for (i = 0; i < 3; i++)
		self->chunks[i].alive = 0;
}

/* 0x00103580  Demolition/CDemolition_Particle.cpp
Deleting destructor (vtable slot 5). */
DcDebrisSystem *DcDebrisSystem_DeletingDtor(DcDebrisSystem *self, unsigned char flags)
{
	self->h.vtable = vtable_0026ba04;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* the chunk models of the game (game + 0x13a4 + 4 * index), per variation (only the first
   3 of each list are used) */
static const int s_DcDebrisModels[3][6] = {
	{ 3, 4, 5, 6, 9, 10 },          /* variation 1 */
	{ 0xb, 0xc, 0xd, 0xe, 0xf, 0x10 }, /* variation 3 */
	{ 3, 4, 5, 6, 7, 8 },           /* the others */
};

/* 0x001035a0  Demolition/CDemolition_Particle.cpp
Init: parameters (or the defaults: speed 90 in variation 3, else 70) and the 3 chunk actor
instances on the variation's debris models. `unused` is not read. Returns 1. */
int DcDebrisSystem_Init(DcDebrisSystem *self, DcChunkParams *params, int unused)
{
	const int *models;
	char *model;
	int line;
	int i;

	(void)unused;
	self->h.particles = NULL;
	self->h.numAlive = 0;
	if (params == NULL)
	{
		self->p.active = 0;
		self->p.gravity = -1.0f;
		self->p.drag = 0.99000001f;
		self->p.bounce = 1;
		self->p.restitution = 0.600000024f;
		if (DC_I32(DC_GAME, 0x28) == 3)
			self->p.speed = 90.0f;
		else
			self->p.speed = 70.0f;
	}
	else
	{
		memcpy(&self->p, params, sizeof(DcChunkParams));
	}
	if (DC_I32(DC_GAME, 0x28) == 1)
	{
		models = s_DcDebrisModels[0];
		line = 0x1e8;
	}
	else if (DC_I32(DC_GAME, 0x28) == 3)
	{
		models = s_DcDebrisModels[1];
		line = 0x1fe;
	}
	else
	{
		models = s_DcDebrisModels[2];
		line = 0x214;
	}
	for (i = 0; i < 3; i++)
	{
		model = DC_PTR(DC_GAME, 0x13a4 + models[i] * 4);
		baInitActorInstance((struct BActorInstance *)self->chunks[i].actor, (struct BActor *)model,
			1, s_DcParticleFile, line);
		self->chunks[i].radius = DC_F32(model, 0x5c);
	}
	DC_PS_Reset(self);
	return 1;
}

/* 0x00103850  Demolition/CDemolition_Particle.cpp
Throws one chunk from the emitter: 60 frames flying, then 30 fading; a random velocity
(up, mostly) and spin. */
void DcDebrisSystem_EmitChunk(DcDebrisSystem *self, DcChunk *c)
{
	float a;
	float b;
	float cz;

	c->alive = 1;
	c->life = 0x3c;
	c->fade = 0x8000;
	c->fadeTime = 0x1e;
	c->fadeStep = -0x444;
	c->age = 0;
	c->phase = 0;
	c->pos[0] = self->h.pos[0];
	c->pos[1] = self->h.pos[1];
	c->pos[2] = self->h.pos[2];
	c->pos[3] = 1.0f;
	a = dc_unit() - 0.5f;
	b = dc_unit() * 0.5f + 0.5f;
	cz = dc_unit() - 0.5f;
	c->vel[0] = dc_unit() * self->p.speed * a;
	c->vel[1] = dc_unit() * self->p.speed * b;
	c->vel[2] = dc_unit() * self->p.speed * cz;
	dc_chunk_place(c);
	c->spin = dc_unit() * 1.5f + 2.5f;
	dc_chunk_set_actor(c);
}

/* 0x00103ae0  Demolition/CDemolition_Particle.cpp
Update (vtable slot 1): moves, bounces and rolls the chunks; a chunk below -500 dies. */
void DcDebrisSystem_Update(DcDebrisSystem *self, float dt)
{
	DcChunk *c;
	int i;

	if (self->p.active == 0)
		return;
	self->numChunks = 0;
	for (i = 0; i < 3; i++)
	{
		c = &self->chunks[i];
		if (c->alive == 0)
			continue;
		if (!(-500.0f <= c->pos[1])) /* 0x00268ce4 */
		{
			c->alive = 0;
			continue;
		}
		if (!dc_chunk_age(c))
			continue;
		dc_chunk_move(c, &self->p, dt);
		if (dc_chunk_roll(c, 1))
			dc_chunk_set_rotation(c);
		DC_F32(c->actor, 0xc8) = c->pos[2];
		DC_F32(c->actor, 0xc0) = c->pos[0];
		DC_F32(c->actor, 0xc4) = c->radius + c->pos[1];
		self->numChunks = self->numChunks + 1;
	}
	if (self->numChunks < 1)
		self->p.active = 0;
}

/* 0x00103f10  Demolition/CDemolition_Particle.cpp
Throws all 3 chunks. */
void DcDebrisSystem_Explode(DcDebrisSystem *self)
{
	int i;

	for (i = 0; i < 3; i++)
		DcDebrisSystem_EmitChunk(self, &self->chunks[i]);
}

/* ======================================================================== DcRubbleSystem */

/* 0x001029c0  Demolition/CDemolition_Particle.cpp
Constructor (vtable only). */
DcRubbleSystem *DcRubbleSystem_Construct(DcRubbleSystem *self)
{
	self->h.vtable = vtable_0026ba1c;
	return self;
}

/* 0x001029d0  Demolition/CDemolition_Particle.cpp
Destructor (vtable only). */
void DcRubbleSystem_Destruct(DcRubbleSystem *self)
{
	self->h.vtable = vtable_0026ba1c;
}

/* 0x001029e0  Demolition/CDemolition_Particle.cpp
Release (vtable slot 0): the chunks' actor instances. */
void DcRubbleSystem_Release(DcRubbleSystem *self)
{
	int i;

	for (i = 0; i < 4; i++)
		baDeleteActorInstance((struct BActorInstance *)self->chunks[i].actor);
}

/* 0x00102a10  Demolition/CDemolition_Particle.cpp
Draw (vtable slot 2). */
void DcRubbleSystem_Draw(DcRubbleSystem *self)
{
	if (self->p.active == 0)
		return;
	bdPushRenderState(3, 1, 0);
	if (self->numChunks != 0)
		dc_draw_chunks(self->chunks, 4);
	bdPopRenderState(3);
}

/* 0x00102ae0  Demolition/CDemolition_Particle.cpp
Reset (vtable slot 3). */
void DcRubbleSystem_Reset(DcRubbleSystem *self)
{
	int i;

	self->p.active = 0;
	for (i = 0; i < 4; i++)
		self->chunks[i].alive = 0;
}

/* 0x00103f40  Demolition/CDemolition_Particle.cpp
Deleting destructor (vtable slot 5). */
DcRubbleSystem *DcRubbleSystem_DeletingDtor(DcRubbleSystem *self, unsigned char flags)
{
	self->h.vtable = vtable_0026ba1c;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00103f60  Demolition/CDemolition_Particle.cpp
Init: parameters (or the defaults) and the 4 chunk actor instances on the game's rubble
models (game + 0x79b4). `unused` is not read. Returns 1. */
int DcRubbleSystem_Init(DcRubbleSystem *self, DcChunkParams *params, int unused)
{
	char *model;
	int i;

	(void)unused;
	self->h.particles = NULL;
	self->h.numAlive = 0;
	if (params == NULL)
	{
		self->p.active = 0;
		self->p.gravity = -1.0f;
		self->p.speed = 60.0f;
		self->p.drag = 0.959999979f;
		self->p.bounce = 1;
		self->p.restitution = 0.600000024f;
	}
	else
	{
		memcpy(&self->p, params, sizeof(DcChunkParams));
	}
	for (i = 0; i < 4; i++)
	{
		model = DC_PTR(DC_GAME, 0x79b4 + i * 4);
		baInitActorInstance((struct BActorInstance *)self->chunks[i].actor, (struct BActor *)model,
			1, s_DcParticleFile, 0x3fe);
		self->chunks[i].radius = DC_F32(model, 0x5c);
	}
	DC_PS_Reset(self);
	return 1;
}

/* 0x00104000  Demolition/CDemolition_Particle.cpp
Throws one chunk from the emitter: 30-49 frames flying, then 60 fading; a random velocity
biased by the throw direction, and spin. */
void DcRubbleSystem_EmitChunk(DcRubbleSystem *self, DcChunk *c)
{
	float a;
	float b;
	float cz;

	c->alive = 1;
	c->age = 0;
	c->fade = 0x8000;
	c->fadeTime = 0x3c;
	c->fadeStep = -0x222;
	c->phase = 0;
	c->life = (short)(dc_rand() % 0x14) + 0x1e;
	c->pos[0] = self->h.pos[0];
	c->pos[1] = self->h.pos[1];
	c->pos[2] = self->h.pos[2];
	c->pos[3] = 1.0f;
	a = (dc_unit() * 0.200000003f + self->dir[0]) - 0.100000001f;
	b = dc_unit() * 0.25f + self->dir[1] + 0.25f;
	cz = (dc_unit() * 0.200000003f + self->dir[2]) - 0.100000001f;
	c->vel[0] = dc_unit() * self->p.speed * a;
	c->vel[1] = dc_unit() * self->p.speed * b;
	c->vel[2] = dc_unit() * self->p.speed * cz;
	dc_chunk_place(c);
	c->spin = dc_unit() * 1.5f + 1.5f;
	dc_chunk_set_actor(c);
}

/* 0x001042c0  Demolition/CDemolition_Particle.cpp
Update (vtable slot 1): moves, bounces and rolls the chunks; a chunk below -500 dies. */
void DcRubbleSystem_Update(DcRubbleSystem *self, float dt)
{
	DcChunk *c;
	int i;

	if (self->p.active == 0)
		return;
	self->numChunks = 0;
	for (i = 0; i < 4; i++)
	{
		c = &self->chunks[i];
		if (c->alive == 0)
			continue;
		if (!(-500.0f <= c->pos[1])) /* 0x00268ce4 */
		{
			c->alive = 0;
			continue;
		}
		if (!dc_chunk_age(c))
			continue;
		dc_chunk_move(c, &self->p, dt);
		dc_chunk_roll(c, 0);
		DC_F32(c->actor, 0xc0) = c->pos[0];
		DC_F32(c->actor, 0xc8) = c->pos[2];
		DC_F32(c->actor, 0xc4) = c->radius + c->pos[1];
		dc_chunk_set_rotation(c);
		self->numChunks = self->numChunks + 1;
	}
	if (self->numChunks < 1)
		self->p.active = 0;
}

/* 0x00104700  Demolition/CDemolition_Particle.cpp
Throws all 4 chunks. */
void DcRubbleSystem_Explode(DcRubbleSystem *self)
{
	int i;

	for (i = 0; i < 4; i++)
		DcRubbleSystem_EmitChunk(self, &self->chunks[i]);
}

/* 0x00104730  Demolition/CDemolition_Particle.cpp
Sets the throw direction. */
void DcRubbleSystem_SetDirection(DcRubbleSystem *self, float *dir)
{
	self->dir[0] = dir[0];
	self->dir[1] = dir[1];
	self->dir[2] = dir[2];
	self->dir[3] = dir[3];
}

/* ======================================================================== DcFlashSparkSystem */

/* 0x00102b00  Demolition/CDemolition_Particle.cpp
Constructor (vtable only). */
DcFlashSparkSystem *DcFlashSparkSystem_Construct(DcFlashSparkSystem *self)
{
	self->h.vtable = vtable_0026ba34;
	return self;
}

/* 0x00102b10  Demolition/CDemolition_Particle.cpp
Draw (vtable slot 2): the sparks as thin beams, then a blue glow sprite on each. */
void DcFlashSparkSystem_Draw(DcFlashSparkSystem *self)
{
	CParticle *p;
	byte *base;
	byte *w;
	int n;
	int i;

	if (self->p.active == 0 || self->h.numAlive <= 0)
		return;
	bdSetWorldIdentity();
	bdSetTexture(0, (struct bdTexture *)self->h.texture);
	base = bdVertexBufferLock((struct bdVertexBuffer *)self->vb, 4);
	if (base == NULL)
		return;
	w = dc_write_beams(&self->h, base, 1);
	bdVertexBufferUnlock((struct bdVertexBuffer *)self->vb);
	bdSetProjectionMode(1);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	n = (int)(w - base) / DC_VERTEX;
	if (n != 0)
		bdDrawVertexBuffer(4, (struct bdVertexBuffer *)self->vb, 0, n);
	bdSetProjectionMode(1);
	for (i = 0; i < self->h.maxParticles; i++)
	{
		p = &self->h.particles[i];
		if (p->alive == 0)
			continue;
		p->pos[3] = 1.0f;
		bdDrawSprite3D(p->pos, 40.0f, 40.0f, 0.0f, (struct bdTexture *)self->glowTexture, 0, 0,
			1, 0xf, 0x20, 0x7f, NULL, 0.0f);
	}
	bdPopRenderState(6);
	bdPopRenderState(5);
}

/* 0x00102d60  Demolition/CDemolition_Particle.cpp
Reset (vtable slot 3). */
void DcFlashSparkSystem_Reset(DcFlashSparkSystem *self)
{
	self->p.active = 0;
	dc_kill_all(&self->h);
}

/* 0x00104750  Demolition/CDemolition_Particle.cpp
Deleting destructor (vtable slot 5). */
DcFlashSparkSystem *DcFlashSparkSystem_DeletingDtor(DcFlashSparkSystem *self, unsigned char flags)
{
	self->h.vtable = vtable_0026ba34;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00104770  Demolition/CDemolition_Particle.cpp
Init: parameters (or the defaults), particles and the vertex buffer (10 per particle).
Returns 0 when the buffer cannot be created. */
int DcFlashSparkSystem_Init(DcFlashSparkSystem *self, DcFlashSparkParams *params, int maxParticles)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	self->glowTexture = 0;
	if (params == NULL)
	{
		self->p.active = 0;
		self->p.bounce = 1;
		self->p.gravity = -0.5f;
		self->p.speed = 15.0f;
		self->p.drag = 0.970000029f;
		self->p.unknown60 = 0.5f;
		self->p.unknown64 = 1.0f;
	}
	else
	{
		memcpy(&self->p, params, sizeof(DcFlashSparkParams));
	}
	self->h.maxParticles = maxParticles;
	self->h.particles = bkHeapAlloc(maxParticles * 0x68, s_DcParticleFile, 0x5a4, 0x1001);
	if (bdVertexBufferCreate((struct bdVertexBuffer **)&self->vb, self->h.maxParticles * 10, 0x144,
		0, 1) != 0)
	{
		DC_PS_Reset(self);
		return 1;
	}
	return 0;
}

/* 0x00104820  Demolition/CDemolition_Particle.cpp
Emit (vtable slot 4): a spark at the emitter, 15-44 frames, colours from dark blue to
light blue fading out, size 4-7, a random velocity. */
void DcFlashSparkSystem_Emit(DcFlashSparkSystem *self, CParticle *p)
{
	float a;
	float b;
	float c;
	float s;
	short life;

	p->alive = 1;
	p->age = 0;
	life = (short)(dc_rand() % 0x1e) + 0xf;
	p->colour[0] = 0;
	p->colour[1] = 0x200;
	p->colour[2] = 0x400;
	p->colour2[0] = 0x1000;
	p->colour2[1] = 0x2000;
	p->colour2[2] = 0x3000;
	p->life = life;
	p->dColour[0] = 0;
	p->dColour[1] = (short)(-0x200 / (int)life);
	p->dColour[2] = (short)(-0x400 / (int)life);
	p->dColour2[0] = (short)(-0x1000 / (int)life);
	p->dColour2[1] = (short)(-0x2000 / (int)life);
	p->dColour2[2] = (short)(-0x3000 / (int)life);
	p->sizeSpeed = 0.0f;
	p->size = (float)((dc_rand() & 3) + 4);
	p->pos[0] = self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = self->h.pos[2];
	p->pos[3] = 1.0f;
	p->prevPos[0] = self->h.pos[0];
	p->prevPos[1] = self->h.pos[1];
	p->prevPos[2] = self->h.pos[2];
	p->prevPos[3] = 1.0f;
	a = dc_unit() - 0.5f;
	b = dc_unit() - 0.5f;
	c = dc_unit() - 0.5f;
	s = dc_unit() * self->p.speed + self->p.speed;
	p->vel[0] = s * a;
	p->vel[1] = s * b;
	p->vel[2] = s * c;
}

/* 0x00104a30  Demolition/CDemolition_Particle.cpp
Release (vtable slot 0). */
void DcFlashSparkSystem_Release(DcFlashSparkSystem *self)
{
	dc_release(&self->h, self->vb);
}

/* 0x00104a70  Demolition/CDemolition_Particle.cpp
Update (vtable slot 1): moves and fades the sparks; inactive when none is left. */
void DcFlashSparkSystem_Update(DcFlashSparkSystem *self, float dt)
{
	CParticle *p;
	short age;
	int i;

	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	for (i = 0; i < self->h.maxParticles; i++)
	{
		p = &self->h.particles[i];
		if (p->alive == 0)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age < p->life)
		{
			dc_move_spark(p, dt, self->p.drag, self->p.gravity);
			self->h.numAlive = self->h.numAlive + 1;
		}
		else
		{
			p->alive = 0;
		}
	}
	if (self->h.numAlive < 1)
		self->p.active = 0;
}

/* ======================================================================== DcGasSystem */

/* 0x00102d90  Demolition/CDemolition_Particle.cpp
Constructor (vtable only). */
DcGasSystem *DcGasSystem_Construct(DcGasSystem *self)
{
	self->h.vtable = vtable_0026ba4c;
	return self;
}

/* 0x00102da0  Demolition/CDemolition_Particle.cpp
Destructor (vtable only). */
void DcGasSystem_Destruct(DcGasSystem *self)
{
	self->h.vtable = vtable_0026ba4c;
}

/* 0x00102db0  Demolition/CDemolition_Particle.cpp
Draw (vtable slot 2): the particles as additive point sprites. */
void DcGasSystem_Draw(DcGasSystem *self)
{
	CParticle *p;
	uint *base;
	uint *w;
	int n;
	int i;

	if (self->p.active == 0 || self->h.numAlive <= 0)
		return;
	bdSetWorldIdentity();
	base = bdVertexBufferLock((struct bdVertexBuffer *)self->vb, 4);
	if (base == NULL)
		return;
	w = base;
	for (i = 0; i < self->h.maxParticles; i++)
	{
		p = &self->h.particles[i];
		if (p->alive == 0)
			continue;
		((float *)w)[0] = p->pos[0];
		((float *)w)[1] = p->pos[1];
		((float *)w)[2] = p->pos[2];
		w[3] = DAT_002bc898[p->colour[3] >> 7] | DAT_002bd640[p->colour[2] >> 7] |
		       DAT_002bc490[p->colour[1] >> 7] | DAT_002bce18[p->colour[0] >> 7];
		w = w + 4;
	}
	bdVertexBufferUnlock((struct bdVertexBuffer *)self->vb);
	bdSetProjectionMode(1);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)2, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)3, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)4, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)6, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 5);
	bdSetTexture(0, NULL);
	bdSetTexture(1, NULL);
	bdSetTexture(2, NULL);
	bdSetTexture(3, (struct bdTexture *)self->h.texture);
	n = (int)((byte *)w - (byte *)base) >> 4;
	if (n != 0)
		bdDrawPointSprites((struct bdVertexBuffer *)self->vb, 0, n, self->p.pointSize);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 1);
	bdSetProjectionMode(1);
	bdPopRenderState(6);
	bdPopRenderState(5);
}

/* 0x00102f60  Demolition/CDemolition_Particle.cpp
Reset (vtable slot 3): inactive, not emitting, every particle dead. */
void DcGasSystem_Reset(DcGasSystem *self)
{
	self->p.active = 0;
	self->p.emitting = 0;
	dc_kill_all(&self->h);
}

/* 0x00104b80  Demolition/CDemolition_Particle.cpp
Deleting destructor (vtable slot 5). */
DcGasSystem *DcGasSystem_DeletingDtor(DcGasSystem *self, unsigned char flags)
{
	self->h.vtable = vtable_0026ba4c;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00104ba0  Demolition/CDemolition_Particle.cpp
Init: parameters (or the defaults), particles and the point sprite buffer (1 per particle).
Returns 0 when the buffer cannot be created. */
int DcGasSystem_Init(DcGasSystem *self, DcGasParams *params, int maxParticles)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	if (params == NULL)
	{
		self->p.active = 0;
		self->p.emitting = 0;
		self->p.unknown48 = 0.0f;
		self->p.speed = 60.0f;
		self->p.drag = 0.949999988f;
		self->p.radius = 75.0f;
		self->p.lifeMin = 6;
		self->p.lifeRange = 10;
		self->p.maxEmit = 2;
		self->p.pointSize = 300.0f;
	}
	else
	{
		memcpy(&self->p, params, sizeof(DcGasParams));
	}
	self->h.maxParticles = maxParticles;
	self->h.particles = bkHeapAlloc(maxParticles * 0x68, s_DcParticleFile, 0x73d, 0x1001);
	if (bdVertexBufferCreate((struct bdVertexBuffer **)&self->vb, self->h.maxParticles, 0x42, 0, 1)
		!= 0)
	{
		DC_PS_Reset(self);
		return 1;
	}
	return 0;
}

/* 0x00104c60  Demolition/CDemolition_Particle.cpp
Emit (vtable slot 4): a puff at a random point of a disc around the emitter (radius / 3),
rising, fading from (8, 16, 127) and alpha 64. */
void DcGasSystem_Emit(DcGasSystem *self, CParticle *p)
{
	float angle;
	float r;
	int life;

	p->alive = 1;
	p->age = 0;
	p->life = (short)(dc_rand() % self->p.lifeRange) + (short)(self->p.lifeMin / 2);
	angle = dc_unit() * 6.28318548f; /* 0x00255930 */
	r = self->p.radius * 0.333333343f /* 0x0025df94 */ * (float)dc_rand() * DC_RAND_UNIT;
	life = p->life;
	p->colour[0] = 0x400;
	p->colour[1] = 0x800;
	p->colour[2] = 0x3f80;
	p->colour[3] = 0x2000;
	p->dColour[3] = 0;
	p->dColour[0] = (short)(-0x400 / life);
	p->dColour[1] = (short)(-0x800 / life);
	p->dColour[2] = (short)(-0x3f80 / life);
	p->pos[0] = cosf(angle) * r + self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = sinf(angle) * r + self->h.pos[2];
	p->vel[0] = 0.0f;
	p->pos[3] = 1.0f;
	p->vel[2] = 0.0f;
	p->vel[1] = dc_unit() * self->p.speed;
}

/* 0x00104dc0  Demolition/CDemolition_Particle.cpp
Update (vtable slot 1): emits up to maxEmit puffs while emitting (into dead or expired
particles), moves (one step a frame; dt is not used) and fades the others. Inactive when it
has stopped emitting and none is left. */
void DcGasSystem_Update(DcGasSystem *self, float dt)
{
	CParticle *p;
	short age;
	int emitted;
	int i;

	(void)dt;
	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	emitted = 0;
	for (i = 0; i < self->h.maxParticles; i++)
	{
		p = &self->h.particles[i];
		if (p->alive == 0)
		{
			if (emitted < self->p.maxEmit && self->p.emitting != 0)
			{
				DC_PS_Emit(self, p);
				emitted = emitted + 1;
				self->h.numAlive = self->h.numAlive + 1;
			}
			continue;
		}
		age = p->age;
		p->age = age + 1;
		if (age < p->life)
		{
			p->pos[0] = p->vel[0] + p->pos[0];
			p->pos[1] = p->vel[1] + p->pos[1];
			p->pos[2] = p->vel[2] + p->pos[2];
			p->vel[0] = self->p.drag * p->vel[0];
			p->vel[1] = self->p.drag * p->vel[1];
			p->vel[2] = self->p.drag * p->vel[2];
			p->colour[0] = p->colour[0] + p->dColour[0];
			p->colour[1] = p->colour[1] + p->dColour[1];
			p->colour[2] = p->colour[2] + p->dColour[2];
			p->colour[3] = p->colour[3] + p->dColour[3];
		}
		else
		{
			if (self->p.maxEmit <= emitted || self->p.emitting == 0)
			{
				p->alive = 0;
				continue;
			}
			DC_PS_Emit(self, p);
			emitted = emitted + 1;
		}
		self->h.numAlive = self->h.numAlive + 1;
	}
	if (self->p.emitting == 0 && self->h.numAlive < 1)
		self->p.active = 0;
}

/* ======================================================================== DcTrailSystem */

/* 0x00102f90  Demolition/CDemolition_Particle.cpp
Constructor (vtable only). */
DcTrailSystem *DcTrailSystem_Construct(DcTrailSystem *self)
{
	self->h.vtable = vtable_0026ba64;
	return self;
}

/* 0x00102fa0  Demolition/CDemolition_Particle.cpp
Destructor (vtable only). */
void DcTrailSystem_Destruct(DcTrailSystem *self)
{
	self->h.vtable = vtable_0026ba64;
}

/* 0x00102fb0  Demolition/CDemolition_Particle.cpp
Draw (vtable slot 2): the ribbon from tail to head as one strip, additive. The strip's end
is placed from `head` (not head - tail), as in the original. */
void DcTrailSystem_Draw(DcTrailSystem *self)
{
	byte *base;
	byte *end;
	int tail;
	int n;

	if (self->p.active == 0)
		return;
	bdSetWorldMatrix(DAT_002855f0);
	base = bdVertexBufferLock((struct bdVertexBuffer *)self->vb, 4);
	if (base == NULL)
		return;
	tail = self->tail;
	n = self->head - tail;
	end = base;
	if (2 < n &&
	    bdDrawPolyline3D(n, self->points[tail], &self->widths[tail], &self->extra[tail],
	                     self->colours[tail], (float *)(base + DC_VERTEX), 0.0f) != 0)
	{
		n = self->head * 2;
		memcpy(base, base + DC_VERTEX, DC_VERTEX);
		end = base + (n + 6) * DC_VERTEX;
		memcpy(end - DC_VERTEX, base + (n + 4) * DC_VERTEX, DC_VERTEX);
	}
	bdVertexBufferUnlock((struct bdVertexBuffer *)self->vb);
	n = (int)(end - base) / DC_VERTEX;
	bdSetProjectionMode(1);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	bdSetTexture(0, (struct bdTexture *)self->texture);
	if (n != 0)
		bdDrawVertexBuffer(4, (struct bdVertexBuffer *)self->vb, 0, n);
	dc_end_strips();
}

/* 0x00103100  Demolition/CDemolition_Particle.cpp
Reset (vtable slot 3): inactive and empty. */
void DcTrailSystem_Reset(DcTrailSystem *self)
{
	self->p.active = 0;
	self->p.growing = 0;
	self->head = 0;
	self->tail = 0;
}

/* 0x00103110  Demolition/CDemolition_Particle.cpp
Sets the ribbon's widths (20) and colours: grey from 32 at the head down to 0 at the end,
alpha 127. */
void DcTrailSystem_SetColours(DcTrailSystem *self)
{
	int step;
	int r;
	int g;
	int b;
	int i;

	step = -0x1000 / self->p.length;
	self->h.numAlive = 0;
	r = 0x1000;
	g = 0x1000;
	b = 0x1000;
	for (i = 0; i < self->p.length; i++)
	{
		self->points[i][3] = 1.0f;
		self->widths[i] = 20.0f;
		self->colours[i][0] = r >> 7;
		self->colours[i][1] = g >> 7;
		self->colours[i][2] = b >> 7;
		self->colours[i][3] = 0x7f;
		g = g + step;
		b = b + step;
		r = r + step;
	}
}

/* 0x001031b0  Demolition/CDemolition_Particle.cpp
Sets the ribbon's texture. */
void DcTrailSystem_SetTexture(DcTrailSystem *self, int texture)
{
	self->texture = texture;
}

/* 0x00104ed0  Demolition/CDemolition_Particle.cpp
Deleting destructor (vtable slot 5). */
DcTrailSystem *DcTrailSystem_DeletingDtor(DcTrailSystem *self, unsigned char flags)
{
	self->h.vtable = vtable_0026ba64;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00104ef0  Demolition/CDemolition_Particle.cpp
Init: parameters (or the defaults: 15 points), a random wave, the point arrays and the
vertex buffer (2 per point + 6). `unused` is not read. Returns 0 when the buffer cannot be
created. */
int DcTrailSystem_Init(DcTrailSystem *self, DcTrailParams *params, int unused)
{
	(void)unused;
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	self->emitVel[0] = 0.0f;
	self->emitVel[1] = 0.0f;
	self->emitVel[2] = 0.0f;
	if (params == NULL)
	{
		self->p.active = 0;
		self->p.unknown65 = 0;
		self->p.drag = 0.980000019f;
		self->p.length = 0xf;
		self->p.gravity = 0.5f;
	}
	else
	{
		memcpy(&self->p, params, sizeof(DcTrailParams));
	}
	self->angle = dc_unit() * 6.28318548f;
	self->angleSpeed = (float)(dc_rand() % 5) * 0.0174532924f /* 0x002640f8 */ +
		0.0349065848f /* 0x0026bad0 */;
	self->amplitude = dc_unit() * 1.5f + 0.5f;
	self->timer = dc_unit() * 20.0f;
	self->pointBuf = bkHeapAlloc(self->p.length << 5, s_DcParticleFile, 0x8ca, 0x1001);
	self->widthBuf = bkHeapAlloc(self->p.length << 2, s_DcParticleFile, 0x8ce, 0x1001);
	self->extraBuf = bkHeapAlloc(self->p.length << 2, s_DcParticleFile, 0x8cf, 0x1001);
	self->colourBuf = bkHeapAlloc(self->p.length << 4, s_DcParticleFile, 0x8d3, 0x1001);
	if (bdVertexBufferCreate((struct bdVertexBuffer **)&self->vb, self->p.length * 2 + 6, 0x144, 0,
		1) == 0)
		return 0;
	self->vels = (float (*)[4])((char *)self->pointBuf + self->p.length * 0x10);
	self->points = (float (*)[4])self->pointBuf;
	self->widths = (float *)self->widthBuf;
	self->colours = (int (*)[4])self->colourBuf;
	self->extra = (float *)self->extraBuf;
	DC_PS_Reset(self);
	return 1;
}

/* 0x00104ef9  Demolition/CDemolition_Particle.cpp
Not a function of its own: Ghidra's entry in the middle of 0x00104ef0 (after its compare of
`params` with 0, with EBX = 0). Nothing calls it. Kept as the NULL-parameter path of the
Init so the table entry has a definition. */
int DcTrailSystem_InitTail(DcTrailSystem *self)
{
	return DcTrailSystem_Init(self, NULL, 0);
}

/* 0x00105130  Demolition/CDemolition_Particle.cpp
Release (vtable slot 0): the vertex buffer and the point arrays. */
void DcTrailSystem_Release(DcTrailSystem *self)
{
	if (self->vb != 0)
		bdVertexBufferDestroy((struct bdVertexBuffer *)self->vb);
	if (self->colourBuf != NULL)
	{
		bkHeapFree(self->colourBuf);
		self->colourBuf = NULL;
	}
	if (self->widthBuf != NULL)
	{
		bkHeapFree(self->widthBuf);
		self->widthBuf = NULL;
	}
	if (self->extraBuf != NULL)
	{
		bkHeapFree(self->extraBuf);
		self->extraBuf = NULL;
	}
	if (self->pointBuf != NULL)
	{
		bkHeapFree(self->pointBuf);
		self->pointBuf = NULL;
	}
	self->h.maxParticles = 0;
	self->h.numAlive = 0;
}

/* 0x001051c0  Demolition/CDemolition_Particle.cpp
Update (vtable slot 1): grows the ribbon (or shrinks it from the tail once it stops; then
inactive when empty), shifts the points back, adds the emitter as the first point with a
sideways waving velocity, and moves the points. The wave changes speed at random times. */
void DcTrailSystem_Update(DcTrailSystem *self, float dt)
{
	float s;
	float a;
	int i;

	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	if (self->p.growing == 0)
	{
		self->tail = self->tail + 1;
		if (self->head <= self->tail)
		{
			self->p.active = 0;
			self->p.growing = 0;
			return;
		}
	}
	else if (self->head < self->p.length)
	{
		self->head = self->head + 1;
	}
	for (i = self->head - 1; self->tail < i; i--)
	{
		memcpy(self->points[i], self->points[i - 1], sizeof self->points[i]);
		memcpy(self->vels[i], self->vels[i - 1], sizeof self->vels[i]);
	}
	self->points[0][0] = self->h.pos[0];
	self->points[0][1] = self->h.pos[1];
	self->points[0][2] = self->h.pos[2];
	self->vels[0][0] = self->emitVel[0];
	self->vels[0][1] = self->emitVel[1];
	self->vels[0][2] = self->emitVel[2];
	for (i = self->tail; i < self->head; i++)
	{
		self->points[i][0] = dt * self->vels[i][0] + self->points[i][0];
		self->points[i][1] = dt * self->vels[i][1] + self->points[i][1];
		self->points[i][2] = dt * self->vels[i][2] + self->points[i][2];
		self->vels[i][0] = self->p.drag * self->vels[i][0];
		self->vels[i][1] = self->p.drag * self->vels[i][1];
		self->vels[i][2] = self->p.drag * self->vels[i][2];
		self->vels[i][1] = dt * self->p.gravity + self->vels[i][1];
	}
	s = sinf(self->angle);
	self->emitVel[0] = s * self->amplitude;
	self->emitVel[2] = -s * self->amplitude;
	a = self->angle + self->angleSpeed;
	self->angle = a;
	if (6.28318548f < a) /* 0x00255930 */
	{
		a = self->angle;
		do
			a = a - 6.28318548f;
		while (6.28318548f < a);
		self->angle = a;
	}
	self->timer = self->timer - dt;
	if (self->timer <= 0.0f)
	{
		self->angleSpeed = (float)(dc_rand() % 5) * 0.0174532924f + 0.0349065848f;
		self->timer = dc_unit() * 10.0f; /* 0x002536d8 */
	}
}

/* ======================================================================== DcDustSystem */

/* 0x001031c0  Demolition/CDemolition_Particle.cpp
Constructor (vtable only). */
DcDustSystem *DcDustSystem_Construct(DcDustSystem *self)
{
	self->h.vtable = vtable_0026ba7c;
	return self;
}

/* 0x001031d0  Demolition/CDemolition_Particle.cpp
Draw (vtable slot 2): the particles as rotated sprites (6 vertices each), additive. */
void DcDustSystem_Draw(DcDustSystem *self)
{
	CParticle *p;
	byte *base;
	byte *w;
	int n;
	int i;

	if (self->p.active == 0 || self->h.numAlive <= 0)
		return;
	bdSetWorldIdentity();
	bdSetTexture(0, (struct bdTexture *)self->h.texture);
	base = bdVertexBufferLock((struct bdVertexBuffer *)self->vb, 4);
	if (base == NULL)
		return;
	w = base;
	for (i = 0; i < self->h.maxParticles; i++)
	{
		p = &self->h.particles[i];
		if (p->alive != 0 &&
		    bdDrawSprite3D(p->pos, p->size, p->size, p->angle, NULL, 0, 0, p->colour[0] >> 7,
		                   p->colour[1] >> 7, p->colour[2] >> 7, p->colour[3] >> 7,
		                   (float *)(w + DC_VERTEX), 0.0f) != 0)
		{
			memcpy(w, w + DC_VERTEX, DC_VERTEX);
			memcpy(w + 5 * DC_VERTEX, w + 4 * DC_VERTEX, DC_VERTEX);
			w = w + 6 * DC_VERTEX;
		}
	}
	bdVertexBufferUnlock((struct bdVertexBuffer *)self->vb);
	dc_begin_strips();
	n = (int)(w - base) / DC_VERTEX;
	if (n != 0)
		bdDrawVertexBuffer(4, (struct bdVertexBuffer *)self->vb, 0, n);
	dc_end_strips();
}

/* 0x00103350  Demolition/CDemolition_Particle.cpp
Sets the shape the dust rises in. */
void DcDustSystem_SetShape(DcDustSystem *self, struct DcShape *shape)
{
	self->shape = shape;
}

/* 0x00105420  Demolition/CDemolition_Particle.cpp
Deleting destructor (vtable slot 5). */
DcDustSystem *DcDustSystem_DeletingDtor(DcDustSystem *self, unsigned char flags)
{
	self->h.vtable = vtable_0026ba7c;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00105440  Demolition/CDemolition_Particle.cpp
Init: parameters (or the defaults), particles and the vertex buffer (6 per particle).
Returns 0 when the buffer cannot be created. */
int DcDustSystem_Init(DcDustSystem *self, DcDustParams *params, int maxParticles)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	self->shape = NULL;
	if (params == NULL)
	{
		self->p.active = 0;
		self->p.emitting = 0;
		self->p.rise = 5.0f;
		self->p.speed = 70.0f;
		self->p.drag = 0.949999988f;
		self->p.radius = 75.0f;
		self->p.lifeMin = 200;
		self->p.lifeRange = 200;
		self->p.interval = 40.0f;
	}
	else
	{
		memcpy(&self->p, params, sizeof(DcDustParams));
	}
	self->h.maxParticles = maxParticles;
	self->h.particles = bkHeapAlloc(maxParticles * 0x68, s_DcParticleFile, 0xa55, 0x1001);
	if (bdVertexBufferCreate((struct bdVertexBuffer **)&self->vb, self->h.maxParticles * 6, 0x144, 0,
		1) != 0)
	{
		DC_PS_Reset(self);
		return 1;
	}
	return 0;
}

/* 0x00105500  Demolition/CDemolition_Particle.cpp
Emit (vtable slot 4): a dust cloud 500 above a random point of the shape (12 tries, then
its centre), slowly turning, rising and growing, white fading out. Without a shape the
original uses an unset position; this port uses 0. */
void DcDustSystem_Emit(DcDustSystem *self, CParticle *p)
{
	struct DcShape *s;
	float pos[3];
	float r;
	float u;
	int tries;
	int found;
	short d;

	p->alive = 1;
	p->age = 0;
	pos[0] = 0.0f;
	pos[1] = 0.0f;
	pos[2] = 0.0f;
	s = self->shape;
	if (s != NULL)
	{
		found = 0;
		pos[1] = s->centre[1] + 500.0f; /* 0x002686fc */
		tries = 0;
		r = s->radius;
		do
		{
			float rx = dc_unit();
			float rz = dc_unit();

			pos[0] = (rx * (r + r) - r) + self->shape->centre[0];
			pos[2] = (rz * (r + r) - r) + self->shape->centre[2];
			if (DcShape_Contains(self->shape, pos) != 0)
				found = 1;
			if (10 < tries)
			{
				pos[0] = self->shape->centre[0];
				pos[2] = self->shape->centre[2];
				break;
			}
			tries = tries + 1;
		} while (!found);
	}
	p->pos[2] = pos[2];
	p->pos[0] = pos[0];
	p->life = (short)(dc_rand() % self->p.lifeRange) + (short)(self->p.lifeMin / 2);
	p->pos[1] = pos[1];
	p->pos[3] = 1.0f;
	p->angle = dc_unit() * 6.28318548f;
	p->vel[0] = 0.0f;
	p->spin = dc_unit() * 0.0199999996f - 0.00999999978f;
	u = dc_unit();
	p->vel[2] = 0.0f;
	p->vel[1] = u + u + self->p.rise;
	p->sizeSpeed = 1.0f;
	p->size = dc_unit() * 300.0f + 300.0f; /* 0x00254f14 */
	p->colour[3] = (short)(((dc_rand() & 0x3f) + 0x30) * 0x80);
	p->colour[0] = 0x3f80;
	p->colour[1] = 0x3f80;
	p->colour[2] = 0x3f80;
	p->dColour[3] = 0;
	d = (short)(-0x3f80 / (int)p->life);
	p->dColour[0] = d;
	p->dColour[1] = d;
	p->dColour[2] = d;
}

/* 0x001057d0  Demolition/CDemolition_Particle.cpp
Release (vtable slot 0). */
void DcDustSystem_Release(DcDustSystem *self)
{
	dc_release(&self->h, self->vb);
}

/* 0x00105810  Demolition/CDemolition_Particle.cpp
Update (vtable slot 1): while emitting, one cloud at random intervals (0.5-1.5 x interval);
moves, turns, grows and fades the clouds. Inactive when it has stopped emitting and none is
left. */
void DcDustSystem_Update(DcDustSystem *self, float dt)
{
	CParticle *p;
	short age;
	float t;
	int i;

	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	if (self->p.emitting != 0)
	{
		t = self->timer - dt;
		self->timer = t;
		if (t < 0.0f)
		{
			self->timer = (dc_unit() + 0.5f) * self->p.interval;
			for (i = 0; i < self->h.maxParticles; i++)
			{
				if (self->h.particles[i].alive == 0)
				{
					DC_PS_Emit(self, &self->h.particles[i]);
					break;
				}
			}
		}
	}
	for (i = 0; i < self->h.maxParticles; i++)
	{
		p = &self->h.particles[i];
		if (p->alive == 0)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age < p->life)
		{
			p->colour[0] = p->colour[0] + p->dColour[0];
			p->colour[1] = p->colour[1] + p->dColour[1];
			p->colour[2] = p->colour[2] + p->dColour[2];
			p->pos[0] = dt * p->vel[0] + p->pos[0];
			p->colour[3] = p->colour[3] + p->dColour[3];
			p->pos[1] = dt * p->vel[1] + p->pos[1];
			p->pos[2] = dt * p->vel[2] + p->pos[2];
			p->angle = p->spin + p->angle;
			p->size = p->sizeSpeed + p->size;
			self->h.numAlive = self->h.numAlive + 1;
		}
		else
		{
			p->alive = 0;
		}
	}
	if (self->p.emitting == 0 && self->h.numAlive < 1)
		self->p.active = 0;
}

/* 0x00105970  Demolition/CDemolition_Particle.cpp
Reset (vtable slot 3): inactive, not emitting, a new random timer, every particle dead. */
void DcDustSystem_Reset(DcDustSystem *self)
{
	self->p.active = 0;
	self->p.emitting = 0;
	self->timer = (dc_unit() + 0.5f) * self->p.interval;
	dc_kill_all(&self->h);
}

/* ======================================================================== vtables */

/* 0x0026b9ec */
const void *const vtable_0026b9ec[6] = {
	(const void *)DcSparkSystem_Release, (const void *)DcSparkSystem_Update,
	(const void *)DcSparkSystem_Draw, (const void *)DcSparkSystem_Reset,
	(const void *)CSparkSystem_Emit, (const void *)DcSparkSystem_DeletingDtor,
};
/* 0x0026ba04  (slot 4: an empty function, 0x001ec260) */
const void *const vtable_0026ba04[6] = {
	(const void *)DcDebrisSystem_Release, (const void *)DcDebrisSystem_Update,
	(const void *)DcDebrisSystem_Draw, (const void *)DcDebrisSystem_Reset,
	(const void *)FUN_001ec260, (const void *)DcDebrisSystem_DeletingDtor,
};
/* 0x0026ba1c */
const void *const vtable_0026ba1c[6] = {
	(const void *)DcRubbleSystem_Release, (const void *)DcRubbleSystem_Update,
	(const void *)DcRubbleSystem_Draw, (const void *)DcRubbleSystem_Reset,
	(const void *)FUN_001ec260, (const void *)DcRubbleSystem_DeletingDtor,
};
/* 0x0026ba34 */
const void *const vtable_0026ba34[6] = {
	(const void *)DcFlashSparkSystem_Release, (const void *)DcFlashSparkSystem_Update,
	(const void *)DcFlashSparkSystem_Draw, (const void *)DcFlashSparkSystem_Reset,
	(const void *)DcFlashSparkSystem_Emit, (const void *)DcFlashSparkSystem_DeletingDtor,
};
/* 0x0026ba4c  (slot 0 is CGlowSystem's Release, classlib_fx) */
const void *const vtable_0026ba4c[6] = {
	(const void *)CGlowSystem_Release, (const void *)DcGasSystem_Update,
	(const void *)DcGasSystem_Draw, (const void *)DcGasSystem_Reset,
	(const void *)DcGasSystem_Emit, (const void *)DcGasSystem_DeletingDtor,
};
/* 0x0026ba64 */
const void *const vtable_0026ba64[6] = {
	(const void *)DcTrailSystem_Release, (const void *)DcTrailSystem_Update,
	(const void *)DcTrailSystem_Draw, (const void *)DcTrailSystem_Reset,
	(const void *)FUN_001ec260, (const void *)DcTrailSystem_DeletingDtor,
};
/* 0x0026ba7c */
const void *const vtable_0026ba7c[6] = {
	(const void *)DcDustSystem_Release, (const void *)DcDustSystem_Update,
	(const void *)DcDustSystem_Draw, (const void *)DcDustSystem_Reset,
	(const void *)DcDustSystem_Emit, (const void *)DcDustSystem_DeletingDtor,
};
