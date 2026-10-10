/*
ClassLib/CParticleSystem.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\CParticleSystem.cpp

Particle systems (see the class list in classlib_fx.h), the list that links them, and
CParticleEffect, a set of emitters that each fire an array of CParticleSystem.

0x000a7e50 is a gap function just before the first CParticleSystem.cpp function and works
on CParticleEffect, so it is placed here (file inferred).

The emitters draw into vertex buffers of bdisplay: each beam or sprite is a short strip
whose first and last vertices are doubled, so all strips can be drawn as one.
*/

#include "classlib_fx.h"

static const char s_CPSFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\CParticleSystem.cpp";

#define PS_VERTEX 0x1c /* bytes per strip vertex (FVF 0x144) */

/* bdisplay (assumed prototypes) */
#define PS_CreateVB ((int (*)(int *, int, int, int, int))FUN_0003f500)
#define PS_LockVB ((byte *(*)(int, int))FUN_0003f710)
#define PS_DrawVB ((void (*)(int, int, int, int))FUN_0003f4a0)
#define PS_SetState ((void (*)(int, int, int))FUN_00046cb0)
/* a collision query (engine_misc): (a1, pos, dir, info) */
#define PS_Collide ((int (*)(int, void *, float *, void *))FUN_00053af0)

/* virtual calls */
#define PS_VT(obj) ((void *const *)((CPSHeader *)(obj))->vtable)
#define PS_Release(obj) (((void (*)(void *))PS_VT(obj)[0])(obj))
#define PS_Update(obj, dt) (((void (*)(void *, float))PS_VT(obj)[1])((obj), (dt)))
#define PS_Draw(obj) (((void (*)(void *))PS_VT(obj)[2])(obj))
#define PS_Reset(obj) (((void (*)(void *))PS_VT(obj)[3])(obj))
#define PS_Emit(obj, p) (((void (*)(void *, CParticle *))PS_VT(obj)[4])((obj), (p)))

static float PS_RandUnit(void) /* rand() * (1 / 32767) */
{
	return (float)CLF_Rand() * 3.05185094e-05f;
}

/* ======================================================================== CParticleEffect */

/* 0x000a7e50  ClassLib (gap; file inferred: CParticleSystem.cpp)
Fires the next system of `emitter` as a burst of `kind` (0-5: more particles and faster
turning) in colour (r, g, b); `strength` (seconds) sets how long it emits. */
void CParticleEffect_Burst(CParticleEffect *self, CParticleEmitter *emitter, short r, short g,
                           short b, int kind, float strength)
{
	CParticleSystem *sys;
	int idx;

	if (emitter == NULL)
		return;
	if (emitter->systems == NULL)
		return;
	idx = emitter->next;
	emitter->next = (emitter->next + 1) % self->numSystems;
	sys = &emitter->systems[idx];
	CParticleSystem_SetPosition(&sys->h, emitter->pos);
	sys->p.riseSpeed = emitter->scale * 8.0f;
	sys->p.radiusSpeed = emitter->scale * 1.11111116f;
	sys->p.radius = emitter->scale * 111.111115f;
	sys->p.emitTime = strength * 60.0f;
	switch (kind) {
	case 0:
		sys->p.maxEmit = 4;
		sys->p.angleSpeed = 0.0610865243f;
		sys->p.riseSpeed = sys->p.riseSpeed * 1.60000002f;
		break;
	case 1:
		sys->p.maxEmit = 6;
		sys->p.angleSpeed = 0.0872664601f;
		sys->p.riseSpeed = sys->p.riseSpeed * 1.39999998f;
		break;
	case 2:
		sys->p.maxEmit = 10;
		sys->p.angleSpeed = 0.157079637f;
		break;
	case 3:
		sys->p.maxEmit = 0xf;
		sys->p.angleSpeed = 0.314159274f;
		sys->p.riseSpeed = sys->p.riseSpeed * 0.699999988f;
		break;
	case 4:
		sys->p.maxEmit = 0x14;
		sys->p.angleSpeed = 0.383972436f;
		sys->p.riseSpeed = sys->p.riseSpeed * 0.400000006f;
		break;
	case 5:
		sys->p.maxEmit = 8;
		sys->p.angleSpeed = 0.314159274f;
		sys->p.riseSpeed = sys->p.riseSpeed + sys->p.riseSpeed;
		sys->p.radiusSpeed = sys->p.radiusSpeed * 1.5f;
		break;
	default:
		break;
	}
	CParticleSystem_Start(sys, r, g, b);
	sys->p.active = 1;
}

/* 0x000a8580  ClassLib/CParticleSystem.cpp
Switches every emitter with id `id` on or off (its actor animation, see AnimSwitch_Set). */
void CParticleEffect_SetOn(CParticleEffect *self, int id, int on)
{
	CParticleEmitter *e;

	for (e = self->emitters; e != NULL; e = e->link) {
		if (e->id == id)
			AnimSwitch_Set((int *)e, on);
	}
}

/* 0x000a85c0  ClassLib/CParticleSystem.cpp
Moves an emitter. */
void CParticleEmitter_SetPosition(CParticleEmitter *self, float *pos)
{
	if (self != NULL) {
		self->pos[0] = pos[0];
		self->pos[1] = pos[1];
		self->pos[2] = pos[2];
		self->pos[3] = pos[3];
	}
}

/* 0x000a85f0  ClassLib/CParticleSystem.cpp
Updates the emitters: their time and actor animation while on, their systems, and the
drift added to every particle's velocity. */
void CParticleEffect_Update(CParticleEffect *self, float dt)
{
	CParticleEmitter *e;
	CParticleSystem *sys;
	int i;
	int j;

	for (e = self->emitters; e != NULL; e = e->link) {
		if (e->on == 1) {
			e->time = dt * 0.0166666675f + e->time;
			if (e->actor != 0)
				CLF_UpdateAnim(e->actor, *(int *)(e->actor + 0x134), dt, 0);
		} else {
			e->time = 0.0f;
		}
		if (e->systems != NULL) {
			for (i = 0; i < self->numSystems; i++) {
				sys = &e->systems[i];
				PS_Update(sys, dt);
				if (e->drift != NULL) {
					for (j = 0; j < sys->h.maxParticles; j++) {
						sys->h.particles[j].vel[0] = sys->h.particles[j].vel[0] + e->drift[0];
						sys->h.particles[j].vel[1] = sys->h.particles[j].vel[1] + e->drift[1];
						sys->h.particles[j].vel[2] = sys->h.particles[j].vel[2] + e->drift[2];
					}
				}
			}
		}
	}
}

/* places an actor at the emitter, scaled by the emitter's scale */
static void CParticleEffect_PlaceActor(int actor, CParticleEmitter *e)
{
	*(float *)(actor + 0xc0) = e->pos[0];
	*(float *)(actor + 0xc4) = e->pos[1];
	*(float *)(actor + 0xc8) = e->pos[2];
	*(float *)(actor + 0xcc) = e->pos[3];
	*(float *)(actor + 0xd8) = e->scale;
	*(float *)(actor + 0xd4) = *(float *)(actor + 0xd8);
	*(float *)(actor + 0xd0) = *(float *)(actor + 0xd4);
	CLF_UpdateActor(actor, 1);
}

/* 0x000a86e0  ClassLib/CParticleSystem.cpp
Draws the effect. With `drawActors`: animates the textures, and draws the effect's actor at
every emitter (mode 0) or each emitter's own actor while on, tinted by its colour. Then,
in modes other than 0, draws every system. */
void CParticleEffect_Draw(CParticleEffect *self, int mode, uint drawActors)
{
	void (*set_ambient)(int, int, int) = (void (*)(int, int, int))FUN_00038440;
	CParticleEmitter *e;
	int ambient[3];
	uint colour;
	int r;
	int g;
	int b;
	int i;

	if (self->emitters == NULL)
		return;
	if (drawActors != 0) {
		PS_SetState(3, 1, 0);
		PS_SetState(4, 0, 0);
		PS_SetState(7, 3, 0);
		PS_SetState(0, 0, 0);
		ambient[0] = DAT_00284b10;
		ambient[1] = DAT_00284b14;
		ambient[2] = DAT_00284b18;
		for (e = self->emitters; e != NULL; e = e->link) {
			FUN_000438b0();
			if (e->texture != 0) {
				if (e->on == 1)
					Texture_SetFrameAtTime(e->texture, CLF_FTOL(e->time * 1000.0f), 1);
				else
					FUN_00041580(e->texture, 0);
			}
			if (self->texture1 != 0)
				FUN_00041600(self->texture1, CLF_FTOL(e->time * 1000.0f));
			if (self->texture2 != 0)
				FUN_00041600(self->texture2, CLF_FTOL(e->time * 1000.0f));
			if (mode == 0) {
				CParticleEffect_PlaceActor(self->actor, e);
				set_ambient(0x5f, 0x5f, 0x5f);
				PS_SetState(6, 0, 0);
				FUN_00034d50(self->actor, 1);
				FUN_00046d00(6);
			} else if (e->on == 1 && e->actor != 0) {
				colour = e->colour;
				if (colour == 0xffffffff) {
					g = 0x7f;
					r = 0;
					b = 0x7f;
				} else {
					g = colour >> 8 & 0xff;
					b = colour & 0xff;
					r = colour >> 16 & 0xff;
				}
				PS_SetState(6, 1, 0);
				PS_SetState(10, 1, 0);
				PS_SetState(5, 0, 0);
				CParticleEffect_PlaceActor(e->actor, e);
				set_ambient(r, g, b);
				FUN_00034d50(e->actor, 7);
				set_ambient(ambient[0], ambient[1], ambient[2]);
				FUN_00046d00(6);
				FUN_00046d00(10);
				FUN_00046d00(5);
			}
		}
		FUN_00046d00(0);
		FUN_00046d00(7);
		FUN_00046d00(4);
		FUN_00046d00(3);
	}
	PS_SetState(3, 0, 0);
	for (e = self->emitters; e != NULL; e = e->link) {
		if (e->systems != NULL && mode != 0) {
			for (i = 0; i < self->numSystems; i++)
				PS_Draw(&e->systems[i]);
		}
	}
	FUN_00046d00(3);
}

/* 0x000a8a00  ClassLib/CParticleSystem.cpp
Collision query along `dir` from an emitter that is on (sphere of the emitter's scale).
Returns the query's result != 0; false for no emitter, an emitter that is off or a zero
direction. */
int CParticleEffect_Collide(CParticleEffect *self, CParticleEmitter *e, int a2, float *dir)
{
	float sphere[5];
	int info[13];

	if (e == NULL)
		return 0;
	if (dir[2] * dir[2] + dir[1] * dir[1] + dir[0] * dir[0] != 0.0f && e->on != 0) {
		((int *)sphere)[0] = ((int *)e->pos)[0];
		sphere[4] = e->scale * self->scale;
		((int *)sphere)[1] = ((int *)e->pos)[1];
		((int *)sphere)[2] = ((int *)e->pos)[2];
		sphere[3] = 1.0f;
		info[0] = 1;
		return PS_Collide(a2, sphere, dir, info) != 0;
	}
	return 0;
}

/* ======================================================================== shared setters and the list */

/* 0x000a8ab0  ClassLib/CParticleSystem.cpp
Sets the particle texture of any system. */
void CParticleSystem_SetTexture(CPSHeader *self, int texture)
{
	self->texture = texture;
}

/* 0x000a8ac0  ClassLib/CParticleSystem.cpp
CParticleSystemList constructor (vtable only). */
void CParticleSystemList_Construct(CParticleSystemList *self)
{
	self->vtable = vtable_00266cd4;
}

/* 0x000a8ad0  ClassLib/CParticleSystem.cpp
CParticleSystemList destructor (vtable only). */
void CParticleSystemList_Destruct(CParticleSystemList *self)
{
	self->vtable = vtable_00266cd4;
}

/* 0x000a8ae0  ClassLib/CParticleSystem.cpp
Draws every system of the list. */
void CParticleSystemList_DrawAll(CParticleSystemList *self)
{
	CPSHeader *s;

	for (s = self->first; s != NULL; s = s->next)
		PS_Draw(s);
}

/* ======================================================================== CParticleSystem (vtable 0x00266cd8) */

/* 0x000a8520  ClassLib/CParticleSystem.cpp
Deleting destructor (vtable slot 5): flag 2 destroys an array (its count is the word in
front of it), flag 1 frees the memory. */
CParticleSystem *CParticleSystem_DeletingDtor(CParticleSystem *self, byte flags)
{
	int count;
	CParticleSystem *p;

	if ((flags & 2) != 0) {
		/* eh vector destructor iterator: last element first */
		count = ((int *)self)[-1];
		p = self + count;
		while (count-- > 0) {
			p = p - 1;
			CParticleSystem_Destruct(p);
		}
		if ((flags & 1) != 0)
			CLF_Free((int *)self - 1);
		return (CParticleSystem *)((int *)self - 1);
	}
	CParticleSystem_Destruct(self);
	if ((flags & 1) != 0)
		CLF_Free(self);
	return self;
}

/* 0x000a8b00  ClassLib/CParticleSystem.cpp
Destructor (vtable only). */
void CParticleSystem_Destruct(CParticleSystem *self)
{
	self->h.vtable = vtable_00266cd8;
}

/* 0x000a8b10  ClassLib/CParticleSystem.cpp
Default parameters. */
void CParticleSystem_DefaultParams(CPSParams *p)
{
	p->active = 0;
	p->unknown51 = 0;
	p->angleSpeed = 0.157079637f;
	p->gravity = -0.00999999978f;
	p->speed = 5.0f;
	p->drag = 0.99000001f;
	p->radiusSpeed = 2.0f;
	p->riseAccel = 0.300000012f;
	p->riseSpeed = 20.0f;
	p->radius = 200.0f;
	p->numRings = 6;
	p->ringLength = 0x1e;
	p->maxEmit = 10;
	p->lifeMin = 0x1e;
	p->lifeRange = 0x1e;
	p->pointSize = 80.0f;
	p->emitTime = 500.0f;
}

/* 0x000a8b80  ClassLib/CParticleSystem.cpp
Reset (vtable slot 3): inactive, the emit time restarts. */
void CParticleSystem_Reset(CParticleSystem *self)
{
	self->p.active = 0;
	self->emitTimer = self->p.emitTime;
}

/* 0x000a9510 belongs to CSparkPointSystem; see below. */

/* 0x000a9540  ClassLib/CParticleSystem.cpp
Sets the second texture (ring strips / point sprites). */
void CParticleSystem_SetTexture2(CParticleSystem *self, int texture)
{
	self->texture2 = texture;
}

/* 0x000a9550  ClassLib/CParticleSystem.cpp
Moves the emitter of any system. */
void CParticleSystem_SetPosition(CPSHeader *self, float *pos)
{
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
}

/* 0x000a9570  ClassLib/CParticleSystem.cpp
CParticleSystemList deleting destructor (its only virtual function). */
CParticleSystemList *CParticleSystemList_DeletingDtor(CParticleSystemList *self, byte flags)
{
	self->vtable = vtable_00266cd4;
	if ((flags & 1) != 0)
		CLF_Free(self);
	return self;
}

/* 0x000a9590  ClassLib/CParticleSystem.cpp
Releases and frees every system of the list. */
void CParticleSystemList_DeleteAll(CParticleSystemList *self)
{
	CPSHeader *s;
	CPSHeader *next;

	s = self->first;
	while (s != NULL) {
		next = s->next;
		PS_Release(s);
		CLF_Free(s);
		s = next;
	}
}

/* 0x000a95c0  ClassLib/CParticleSystem.cpp
Constructor (vtable only). */
void CParticleSystem_Construct(CParticleSystem *self)
{
	self->h.vtable = vtable_00266cd8;
}

/* 0x000a95d0  ClassLib/CParticleSystem.cpp
Init: parameters (or the defaults), `maxParticles` particles, the ring arrays and the two
vertex buffers. Returns 0 when a vertex buffer cannot be created. */
int CParticleSystem_Init(CParticleSystem *self, CPSParams *params, int maxParticles)
{
	int i;
	int len;

	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	if (params == NULL)
		CParticleSystem_DefaultParams(&self->p);
	else
		memcpy(&self->p, params, sizeof(CPSParams));
	self->h.maxParticles = maxParticles;
	self->emitTimer = self->p.emitTime;
	self->h.particles = CLF_Alloc(maxParticles * 0x68, s_CPSFile, 0xf8, 0x1001);
	self->rings = CLF_Alloc(self->p.numRings * 0x2c, s_CPSFile, 0xfc, 0x1001);
	len = self->p.ringLength;
	self->ringPoints = CLF_Alloc(len * self->p.numRings * 0x10, s_CPSFile, 0x100, 0x1001);
	for (i = 0; i < self->p.numRings; i++)
		self->rings[i].points = (float (*)[4])((byte *)self->ringPoints + self->p.ringLength * i * 0x10);
	self->ringWidths = CLF_Alloc(self->p.ringLength * self->p.numRings * 4, s_CPSFile, 0x107, 0x1001);
	self->ringExtra = CLF_Alloc(self->p.ringLength * self->p.numRings * 4, s_CPSFile, 0x108, 0x1001);
	for (i = 0; i < self->p.numRings; i++) {
		self->rings[i].widths = (float *)((byte *)self->ringWidths + self->p.ringLength * i * 4);
		self->rings[i].extra = (float *)((byte *)self->ringExtra + self->p.ringLength * i * 4);
	}
	self->ringColours = CLF_Alloc(self->p.ringLength * self->p.numRings * 0x10, s_CPSFile, 0x112, 0x1001);
	for (i = 0; i < self->p.numRings; i++)
		self->rings[i].colours = (int (*)[4])((byte *)self->ringColours + self->p.ringLength * i * 0x10);
	if (self->h.maxParticles == 0) {
		self->pointVB = 0;
	} else if (PS_CreateVB(&self->pointVB, self->h.maxParticles, 0x42, 0, 1) == 0) {
		return 0;
	}
	if (PS_CreateVB(&self->vb, (self->p.ringLength + 3) * self->p.numRings * 2, 0x144, 0, 1) == 0)
		return 0;
	PS_Reset(self);
	return 1;
}

/* 0x000a9800  ClassLib/CParticleSystem.cpp
Emit (vtable slot 4): a particle at the head of a random ring, fading out from the
system colour, with a random velocity. */
void CParticleSystem_Emit(CParticleSystem *self, CParticle *p)
{
	short life;
	int ring;
	float r3;
	float r4;
	float r5;
	float r6;

	p->alive = 1;
	p->age = 0;
	p->life = (short)((longlong)CLF_Rand() % (longlong)self->p.lifeRange) + (short)self->p.lifeMin;
	life = p->life;
	p->colour[0] = (short)(self->colour[0] << 7);
	p->colour[1] = (short)(self->colour[1] << 7);
	p->colour[2] = (short)(self->colour[2] << 7);
	p->colour[3] = 0x3f80;
	p->angle = 0.0f;
	p->dColour[0] = (short)-(p->colour[0] / life);
	p->dColour[1] = (short)-(p->colour[1] / life);
	p->dColour[2] = (short)-(p->colour[2] / life);
	ring = (int)((longlong)CLF_Rand() % (longlong)self->p.numRings);
	p->pos[0] = self->rings[ring].points[0][0];
	p->pos[1] = self->rings[ring].points[0][1];
	p->pos[2] = self->rings[ring].points[0][2];
	p->pos[3] = 1.0f;
	r3 = (float)CLF_Rand();
	r4 = (float)CLF_Rand();
	r5 = (float)CLF_Rand();
	r6 = (float)CLF_Rand();
	p->vel[0] = r6 * 3.05185094e-05f * self->p.speed * (r3 * 3.05185094e-05f + 0.5f);
	p->vel[1] = PS_RandUnit() * self->p.speed * (r4 * 1.52592547e-05f);
	p->vel[2] = PS_RandUnit() * self->p.speed * (r5 * 3.05185094e-05f + 0.5f);
}

/* 0x000a9a30  ClassLib/CParticleSystem.cpp
Release (vtable slot 0): the vertex buffers and arrays. */
void CParticleSystem_Release(CParticleSystem *self)
{
	if (self->vb != 0)
		FUN_0003f650(self->vb);
	if (self->pointVB != 0)
		FUN_0003f650(self->pointVB);
	if (self->h.particles != NULL) {
		CLF_Free(self->h.particles);
		self->h.particles = NULL;
	}
	if (self->ringColours != NULL) {
		CLF_Free(self->ringColours);
		self->ringColours = NULL;
	}
	if (self->ringWidths != NULL) {
		CLF_Free(self->ringWidths);
		self->ringWidths = NULL;
	}
	if (self->ringExtra != NULL) {
		CLF_Free(self->ringExtra);
		self->ringExtra = NULL;
	}
	if (self->ringPoints != NULL) {
		CLF_Free(self->ringPoints);
		self->ringPoints = NULL;
	}
	if (self->rings != NULL) {
		CLF_Free(self->rings);
		self->rings = NULL;
	}
	self->h.maxParticles = 0;
	self->h.numAlive = 0;
}

/* 0x000a9af0  ClassLib/CParticleSystem.cpp
Update (vtable slot 1): the rings spiral up (a new head point every tick), then the
particles move; up to maxEmit dead (or expired) particles are emitted per tick while the
emit time lasts. Expired particles that are not re-emitted stay "alive" but uncounted. */
void CParticleSystem_Update(CParticleSystem *self, float dt)
{
	CPSRing *ring;
	CParticle *p;
	short age;
	int emitted;
	int i;
	int k;

	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	self->emitTimer = self->emitTimer - dt;
	if (self->emitTimer <= 0.0f)
		self->emitTimer = 0.0f;
	for (i = 0; i < self->p.numRings; i++) {
		ring = &self->rings[i];
		ring->angle = ring->angle + self->p.angleSpeed;
		if (ring->count < self->p.ringLength)
			ring->count = ring->count + 1;
		for (k = ring->count - 1; k > 0; k--) {
			ring->points[k][0] = ring->points[k - 1][0];
			ring->points[k][1] = ring->points[k - 1][1];
			ring->points[k][2] = ring->points[k - 1][2];
			ring->points[k][3] = ring->points[k - 1][3];
		}
		ring->points[0][0] = (float)(sin((double)ring->angle) * (double)ring->radius + (double)self->h.pos[0]);
		ring->points[0][2] = (float)(cos((double)ring->angle) * (double)ring->radius + (double)self->h.pos[2]);
		ring->points[0][1] = dt * ring->vel[1] + ring->points[0][1];
		ring->vel[0] = self->p.drag * ring->vel[0];
		ring->vel[1] = self->p.drag * ring->vel[1];
		ring->vel[2] = self->p.drag * ring->vel[2];
		ring->vel[1] = dt * self->p.riseAccel + ring->vel[1];
		ring->radius = dt * self->p.radiusSpeed + ring->radius;
	}
	emitted = 0;
	for (i = 0; i < self->h.maxParticles; i++) {
		p = &self->h.particles[i];
		if (p->alive == 0) {
			if (0.0f < self->emitTimer && emitted < self->p.maxEmit) {
				PS_Emit(self, p);
				emitted = emitted + 1;
				self->h.numAlive = self->h.numAlive + 1;
			}
		} else {
			age = p->age;
			p->age = age + 1;
			if (age < p->life) {
				p->pos[0] = dt * p->vel[0] + p->pos[0];
				p->pos[1] = dt * p->vel[1] + p->pos[1];
				p->pos[2] = dt * p->vel[2] + p->pos[2];
				p->vel[0] = self->p.drag * p->vel[0];
				p->vel[1] = self->p.drag * p->vel[1];
				p->vel[2] = self->p.drag * p->vel[2];
				p->colour[0] = p->colour[0] + p->dColour[0];
				p->colour[1] = p->colour[1] + p->dColour[1];
				p->colour[2] = p->colour[2] + p->dColour[2];
				p->vel[1] = dt * self->p.gravity + p->vel[1];
				self->h.numAlive = self->h.numAlive + 1;
			} else if (0.0f < self->emitTimer && emitted < self->p.maxEmit) {
				PS_Emit(self, p);
				emitted = emitted + 1;
				self->h.numAlive = self->h.numAlive + 1;
			}
		}
	}
	if (self->h.numAlive < 1 && self->emitTimer == 0.0f)
		self->p.active = 0;
}

/* writes the alive particles as point sprites (x, y, z, colour) */
static int PS_WritePoints(CPSHeader *h, uint *out, int col2)
{
	CParticle *p;
	uint *w;
	int i;

	w = out;
	for (i = 0; i < h->maxParticles; i++) {
		p = &h->particles[i];
		if (p->alive != 0) {
			((float *)w)[0] = p->pos[0];
			((float *)w)[1] = p->pos[1];
			((float *)w)[2] = p->pos[2];
			if (col2 == 0)
				w[3] = DAT_002bc898[p->colour[3] >> 7] | DAT_002bd640[p->colour[2] >> 7] |
				       DAT_002bc490[p->colour[1] >> 7] | DAT_002bce18[p->colour[0] >> 7];
			else
				w[3] = DAT_002bd640[p->colour2[2] >> 9] | DAT_002bc490[p->colour2[1] >> 9] |
				       DAT_002bce18[p->colour2[0] >> 9] | DAT_002bc898[127];
			w = w + 4;
		}
	}
	return (int)((byte *)w - (byte *)out) >> 4;
}

/* the point sprite texture stage set-up (texture modulated by the vertex colour) */
static void PS_BeginPoints(int texture)
{
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)2, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)3, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)4, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)6, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 5);
	FUN_000416c0(0, 0);
	FUN_000416c0(1, 0);
	FUN_000416c0(2, 0);
	FUN_000416c0(3, texture);
}

/* 0x000a9d20  ClassLib/CParticleSystem.cpp
Draw (vtable slot 2): the ring trails as strips, the particles as point sprites. */
void CParticleSystem_Draw(CParticleSystem *self)
{
	CPSRing *ring;
	byte *base;
	byte *w;
	byte *next;
	uint *points;
	int nverts;
	int npoints;
	int n;
	int i;

	nverts = 0;
	npoints = 0;
	if (self->p.active == 0 || (self->p.numRings == 0 && self->h.maxParticles == 0))
		return;
	FUN_000438b0();
	if (self->p.numRings != 0) {
		base = PS_LockVB(self->vb, 4);
		if (base == NULL)
			return;
		w = base;
		for (i = 0; i < self->p.numRings; i++) {
			ring = &self->rings[i];
			next = w;
			if (2 < ring->count &&
			    CLF_DrawPolyline(ring->count, ring->points, ring->widths, ring->extra,
			                     ring->colours, w + PS_VERTEX, 0.0f) != 0) {
				n = ring->count;
				memcpy(w, w + PS_VERTEX, PS_VERTEX);
				next = w + (n * 2 + 6) * PS_VERTEX;
				memcpy(next - PS_VERTEX, w + (n * 2 + 4) * PS_VERTEX, PS_VERTEX);
			}
			w = next;
		}
		FUN_0003f790(self->vb);
		nverts = (int)(w - base) / PS_VERTEX;
	}
	if (self->h.maxParticles != 0) {
		points = (uint *)PS_LockVB(self->pointVB, 4);
		if (points == NULL)
			return;
		npoints = PS_WritePoints(&self->h, points, 0);
		FUN_0003f790(self->pointVB);
	}
	PS_SetState(6, 1, 0);
	PS_SetState(5, 0, 0);
	FUN_00044b00(10, 1, 0);
	FUN_00043b70(1);
	if (nverts != 0) {
		FUN_000416c0(0, self->texture2);
		PS_DrawVB(4, self->vb, 0, nverts);
	}
	if (npoints != 0) {
		PS_BeginPoints(self->h.texture);
		CLF_DrawPointSprites(self->pointVB, 0, npoints, self->p.pointSize);
		D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 1);
		FUN_000416c0(3, 0);
	}
	FUN_00043b70(1);
	FUN_00046d00(6);
	FUN_00046d00(5);
}

/* 0x000aa020  ClassLib/CParticleSystem.cpp
Start: restarts the emit time and lays out the rings around the emitter at a random
start angle, each a column of points fading from colour (r, g, b) to black. */
void CParticleSystem_Start(CParticleSystem *self, short r, short g, short b)
{
	CPSRing *ring;
	float step;
	double angle;
	double s;
	double c;
	float radius;
	int R;
	int G;
	int B;
	int dR;
	int dG;
	int dB;
	int len;
	int i;
	int j;

	self->emitTimer = self->p.emitTime;
	step = 6.28318548f / (float)self->p.numRings;
	self->h.numAlive = 0;
	angle = (double)((float)CLF_Rand() * 0.000191753454f);
	self->colour[0] = r;
	self->colour[1] = g;
	self->colour[2] = b;
	for (i = 0; i < self->p.numRings; i++) {
		ring = &self->rings[i];
		ring->count = 0;
		B = (int)self->colour[2] << 7;
		len = self->p.ringLength;
		R = (int)self->colour[0] << 7;
		ring->angle = (float)angle;
		radius = self->p.radius;
		ring->radius = self->p.radius;
		dR = R / len;
		s = sin(angle);
		G = (int)self->colour[1] << 7;
		dG = G / len;
		c = cos(angle);
		dB = B / len;
		ring->vel[0] = 0.0f;
		ring->vel[2] = 0.0f;
		ring->vel[1] = ((float)CLF_Rand() * 1.52592547e-05f + 0.5f) * self->p.riseSpeed;
		for (j = 0; j < self->p.ringLength; j++) {
			ring->points[j][0] = (float)(s * (double)radius + (double)self->h.pos[0]);
			ring->points[j][1] = self->h.pos[1];
			ring->points[j][2] = (float)(c * (double)radius + (double)self->h.pos[2]);
			ring->points[j][3] = 1.0f;
			ring->widths[j] = 20.0f;
			ring->colours[j][0] = R >> 7;
			ring->colours[j][1] = G >> 7;
			ring->colours[j][2] = B >> 7;
			ring->colours[j][3] = 0x7f;
			R = R - dR;
			G = G - dG;
			B = B - dB;
		}
		angle = angle + (double)step;
	}
	for (i = 0; i < self->h.maxParticles; i++)
		self->h.particles[i].alive = 0;
}

/* ======================================================================== beams (CSparkSystem, CSparkPointSystem) */

/* writes every alive particle as a beam from its previous position (width0 there, its
size at its position); returns the vertices written */
static int PS_WriteBeams(CPSHeader *h, byte *base, int fixedWidth)
{
	CParticle *p;
	int c0[4];
	int c1[4];
	byte *w;
	float w0;
	int i;

	w = base;
	for (i = 0; i < h->maxParticles; i++) {
		p = &h->particles[i];
		if (p->alive != 0) {
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
			if (CLF_DrawBeam(p->prevPos, w0, w0, c0, p->pos, p->size, p->size, c1, 0,
			                 (float *)(w + PS_VERTEX), 0.0f) != 0) {
				memcpy(w, w + PS_VERTEX, PS_VERTEX);
				memcpy(w + 9 * PS_VERTEX, w + 8 * PS_VERTEX, PS_VERTEX);
				w = w + 10 * PS_VERTEX;
			}
		}
	}
	return (int)(w - base) / PS_VERTEX;
}

/* writes every alive particle as a rotated sprite; returns the vertices written */
static int PS_WriteSprites(CPSHeader *h, byte *base)
{
	CParticle *p;
	byte *w;
	int i;

	w = base;
	for (i = 0; i < h->maxParticles; i++) {
		p = &h->particles[i];
		if (p->alive != 0 &&
		    CLF_DrawSprite(p->pos, p->size, p->size, p->angle, 0, 0, 0, p->colour[0] >> 7,
		                   p->colour[1] >> 7, p->colour[2] >> 7, p->colour[3] >> 7, w + PS_VERTEX,
		                   0.0f) != 0) {
			memcpy(w, w + PS_VERTEX, PS_VERTEX);
			memcpy(w + 5 * PS_VERTEX, w + 4 * PS_VERTEX, PS_VERTEX);
			w = w + 6 * PS_VERTEX;
		}
	}
	return (int)(w - base) / PS_VERTEX;
}

/* draws `n` strip vertices of `vb` with blend mode `blend` (bdisplay state 10) */
static void PS_DrawStrips(int vb, int n, int blend)
{
	FUN_00043b70(1);
	PS_SetState(6, 1, 0);
	PS_SetState(5, 0, 0);
	FUN_00044b00(10, blend, 0);
	if (n != 0)
		PS_DrawVB(4, vb, 0, n);
	FUN_00043b70(1);
	FUN_00046d00(6);
	FUN_00046d00(5);
}

/* ======================================================================== CSparkSystem (vtable 0x00266cf0) */

/* 0x000a8b90  ClassLib/CParticleSystem.cpp
Constructor (vtable only). */
void CSparkSystem_Construct(CSparkSystem *self)
{
	self->h.vtable = vtable_00266cf0;
}

/* 0x000a8ba0  ClassLib/CParticleSystem.cpp
Destructor (vtable only). */
void CSparkSystem_Destruct(CSparkSystem *self)
{
	self->h.vtable = vtable_00266cf0;
}

/* 0x000a8bb0  ClassLib/CParticleSystem.cpp
Default parameters. */
void CSparkSystem_DefaultParams(CSparkParams *p)
{
	p->active = 0;
	p->bounce = 1;
	p->gravity = -0.200000003f;
	p->speed = 45.0f;
	p->drag = 0.970000029f;
	p->lifeMin = 100;
	p->lifeRange = 100;
	p->colour[0] = 0x10;
	p->colour[1] = 5;
	p->colour[2] = 1;
	p->colour2[0] = 0x80;
	p->colour2[1] = 100;
	p->colour2[2] = 0x28;
	p->sizeMin = 3;
	p->sizeRange = 6;
}

/* 0x000a8c10  ClassLib/CParticleSystem.cpp
Draw (vtable slot 2): beams from the previous positions. */
void CSparkSystem_Draw(CSparkSystem *self)
{
	byte *base;
	int n;

	if (self->p.active != 0 && 0 < self->h.numAlive) {
		FUN_000438b0();
		FUN_000416c0(0, self->h.texture);
		base = PS_LockVB(self->vb, 4);
		if (base != NULL) {
			n = PS_WriteBeams(&self->h, base, 0);
			FUN_0003f790(self->vb);
			PS_DrawStrips(self->vb, n, 1);
		}
	}
}

/* 0x000a8de0  ClassLib/CParticleSystem.cpp
Reset (vtable slot 3): inactive. */
void CSparkSystem_Reset(CSparkSystem *self)
{
	self->p.active = 0;
}

/* 0x000aa250  ClassLib/CParticleSystem.cpp
Deleting destructor (vtable slot 5). */
CSparkSystem *CSparkSystem_DeletingDtor(CSparkSystem *self, byte flags)
{
	self->h.vtable = vtable_00266cf0;
	if ((flags & 1) != 0)
		CLF_Free(self);
	return self;
}

/* 0x000aa270  ClassLib/CParticleSystem.cpp
Init: parameters (or the defaults), particles and the vertex buffer (10 per particle).
Returns 0 when the buffer cannot be created. */
int CSparkSystem_Init(CSparkSystem *self, CSparkParams *params, int maxParticles)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	if (params == NULL)
		CSparkSystem_DefaultParams(&self->p);
	else
		memcpy(&self->p, params, sizeof(CSparkParams));
	self->h.maxParticles = maxParticles;
	self->h.particles = CLF_Alloc(maxParticles * 0x68, s_CPSFile, 0x651, 0x1001);
	if (PS_CreateVB(&self->vb, self->h.maxParticles * 10, 0x144, 0, 1) != 0) {
		PS_Reset(self);
		return 1;
	}
	return 0;
}

/* 0x000aa310  ClassLib/CParticleSystem.cpp
Emit (vtable slot 4): a spark at the emitter, both colours fading to black, random size and
velocity. */
void CSparkSystem_Emit(CSparkSystem *self, CParticle *p)
{
	short life;
	float r3;
	float r4;
	float r5;
	float r6;

	p->alive = 1;
	p->age = 0;
	p->life = (short)((longlong)CLF_Rand() % (longlong)self->p.lifeRange) + (short)self->p.lifeMin;
	life = p->life;
	p->colour[0] = (short)(self->p.colour[0] << 7);
	p->colour[1] = (short)(self->p.colour[1] << 7);
	p->colour[2] = (short)(self->p.colour[2] << 7);
	p->dColour[0] = (short)-(p->colour[0] / life);
	p->dColour[1] = (short)-(p->colour[1] / life);
	p->dColour[2] = (short)-(p->colour[2] / life);
	p->colour2[0] = (short)(self->p.colour2[0] << 7);
	p->colour2[1] = (short)(self->p.colour2[1] << 7);
	p->colour2[2] = (short)(self->p.colour2[2] << 7);
	p->dColour2[0] = (short)-(p->colour2[0] / life);
	p->dColour2[1] = (short)-(p->colour2[1] / life);
	p->dColour2[2] = (short)-(p->colour2[2] / life);
	p->sizeSpeed = 0.0f;
	p->size = (float)((int)((longlong)CLF_Rand() % (longlong)self->p.sizeRange) + self->p.sizeMin);
	p->pos[0] = self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = self->h.pos[2];
	p->pos[3] = 1.0f;
	p->prevPos[0] = self->h.pos[0];
	p->prevPos[1] = self->h.pos[1];
	p->prevPos[2] = self->h.pos[2];
	p->prevPos[3] = 1.0f;
	r3 = (float)CLF_Rand();
	r4 = (float)CLF_Rand();
	r5 = (float)CLF_Rand();
	r6 = (float)CLF_Rand();
	p->vel[0] = r6 * 3.05185094e-05f * self->p.speed * (r3 * 3.05185094e-05f - 0.5f);
	p->vel[1] = PS_RandUnit() * self->p.speed * (r4 * 1.52592547e-05f);
	p->vel[2] = PS_RandUnit() * self->p.speed * (r5 * 3.05185094e-05f - 0.5f);
}

/* 0x000aa590  ClassLib/CParticleSystem.cpp
Release (vtable slot 0). */
void CSparkSystem_Release(CSparkSystem *self)
{
	if (self->vb != 0)
		FUN_0003f650(self->vb);
	if (self->h.particles != NULL) {
		CLF_Free(self->h.particles);
		self->h.particles = NULL;
	}
	self->h.maxParticles = 0;
	self->h.numAlive = 0;
}

/* 0x000aa5d0  ClassLib/CParticleSystem.cpp
Update (vtable slot 1): moves the sparks (they bounce at the emitter height when enabled);
dead sparks are not re-emitted. */
void CSparkSystem_Update(CSparkSystem *self, float dt)
{
	CParticle *p;
	short age;
	int i;

	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	for (i = 0; i < self->h.maxParticles; i++) {
		p = &self->h.particles[i];
		if (p->alive == 0)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age < p->life) {
			if ((DAT_002ba99c & 1) != 0) {
				p->prevPos[0] = p->pos[0];
				p->prevPos[1] = p->pos[1];
				p->prevPos[2] = p->pos[2];
				p->prevPos[3] = p->pos[3];
			}
			p->pos[0] = dt * p->vel[0] + p->pos[0];
			p->pos[1] = dt * p->vel[1] + p->pos[1];
			p->pos[2] = dt * p->vel[2] + p->pos[2];
			p->vel[0] = self->p.drag * p->vel[0];
			p->vel[1] = self->p.drag * p->vel[1];
			p->vel[2] = self->p.drag * p->vel[2];
			p->colour[0] = p->colour[0] + p->dColour[0];
			p->colour[1] = p->colour[1] + p->dColour[1];
			p->colour[2] = p->colour[2] + p->dColour[2];
			p->colour2[0] = p->colour2[0] + p->dColour2[0];
			p->vel[1] = dt * self->p.gravity + p->vel[1];
			p->colour2[1] = p->colour2[1] + p->dColour2[1];
			p->colour2[2] = p->colour2[2] + p->dColour2[2];
			self->h.numAlive = self->h.numAlive + 1;
			if (self->p.bounce != 0 && p->pos[1] < self->h.pos[1]) {
				p->vel[0] = p->vel[0] * 0.5f;
				p->vel[1] = p->vel[1] * 0.5f;
				p->vel[2] = p->vel[2] * 0.5f;
				p->vel[1] = 0.0f;
				p->pos[1] = self->h.pos[1];
			}
		} else {
			p->alive = 0;
		}
	}
	if (self->h.numAlive < 1)
		self->p.active = 0;
}

/* ======================================================================== CSmokeSystem (vtable 0x00266d08) */

/* 0x000a8df0  ClassLib/CParticleSystem.cpp
Constructor (vtable only). */
void CSmokeSystem_Construct(CSmokeSystem *self)
{
	self->h.vtable = vtable_00266d08;
}

/* 0x000a8e00  ClassLib/CParticleSystem.cpp
Destructor (vtable only). */
void CSmokeSystem_Destruct(CSmokeSystem *self)
{
	self->h.vtable = vtable_00266d08;
}

/* 0x000a8e10  ClassLib/CParticleSystem.cpp
Draw (vtable slot 2): rotated sprites, alpha blended. */
void CSmokeSystem_Draw(CSmokeSystem *self)
{
	byte *base;
	int n;

	if (self->p.active != 0 && 0 < self->h.numAlive) {
		FUN_000438b0();
		FUN_000416c0(0, self->h.texture);
		base = PS_LockVB(self->vb, 4);
		if (base != NULL) {
			n = PS_WriteSprites(&self->h, base);
			FUN_0003f790(self->vb);
			PS_DrawStrips(self->vb, n, 1);
		}
	}
}

/* 0x000a8f90  ClassLib/CParticleSystem.cpp
Reset (vtable slot 3, CSmokeSystem and CGlowSystem): inactive. */
void CSmokeSystem_Reset(CSmokeSystem *self)
{
	self->p.active = 0;
}

/* 0x000aa730  ClassLib/CParticleSystem.cpp
Deleting destructor (vtable slot 5). */
CSmokeSystem *CSmokeSystem_DeletingDtor(CSmokeSystem *self, byte flags)
{
	self->h.vtable = vtable_00266d08;
	if ((flags & 1) != 0)
		CLF_Free(self);
	return self;
}

/* 0x000aa750  ClassLib/CParticleSystem.cpp
Init: parameters (or the defaults), particles and the vertex buffer (6 per particle).
Returns 0 when the buffer cannot be created. */
int CSmokeSystem_Init(CSmokeSystem *self, CSmokeParams *params, int maxParticles)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	if (params == NULL) {
		self->p.active = 0;
		self->p.gravity = 0.0f;
		self->p.speed = 20.0f;
		self->p.drag = 0.949999988f;
		self->p.radius = 200.0f;
		self->p.lifeMin = 0x14;
		self->p.lifeRange = 0x1e;
		self->p.spin = 0.00999999978f;
	} else {
		memcpy(&self->p, params, sizeof(CSmokeParams));
	}
	self->h.maxParticles = maxParticles;
	self->h.particles = CLF_Alloc(maxParticles * 0x68, s_CPSFile, 0x7bc, 0x1001);
	if (PS_CreateVB(&self->vb, self->h.maxParticles * 6, 0x144, 0, 1) != 0) {
		PS_Reset(self);
		return 1;
	}
	return 0;
}

/* 0x000aa810  ClassLib/CParticleSystem.cpp
Emit (vtable slot 4): a smoke puff at the emitter, at a random distance-like value that
sets its brightness and size, random angle, spin and velocity. */
void CSmokeSystem_Emit(CSmokeSystem *self, CParticle *p)
{
	short life;
	float dist;
	float r5;
	float r6;
	float r7;
	float r8;
	float vz;
	int grey;

	p->alive = 1;
	p->age = 0;
	p->life = (short)((longlong)CLF_Rand() % (longlong)self->p.lifeRange) + (short)self->p.lifeMin;
	p->angle = (float)(CLF_Rand() % 0x168) * 0.0174532924f;
	dist = (float)CLF_Rand() * self->p.radius * 3.05185094e-05f;
	p->pos[0] = self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = self->h.pos[2];
	p->prevPos[0] = self->h.pos[0];
	p->prevPos[1] = p->pos[1];
	p->prevPos[2] = p->pos[2];
	p->pos[3] = 1.0f;
	p->prevPos[3] = 1.0f;
	p->spin = (float)CLF_Rand() * self->p.spin * 3.05185094e-05f;
	r5 = (float)CLF_Rand();
	r6 = (float)CLF_Rand();
	r7 = (float)CLF_Rand();
	r8 = (float)CLF_Rand();
	p->vel[0] = r8 * 3.05185094e-05f * self->p.speed * (r5 * 3.05185094e-05f - 0.5f);
	p->vel[1] = PS_RandUnit() * self->p.speed * (r6 * 1.52592547e-05f);
	vz = PS_RandUnit();
	p->colour[0] = 0x4000;
	p->vel[2] = vz * self->p.speed * (r7 * 3.05185094e-05f - 0.5f);
	grey = CLF_FTOL((dist * 64.0f) / self->p.radius);
	life = p->life;
	p->colour[2] = 0;
	p->colour[3] = 0x4000;
	p->colour[1] = (short)(grey << 7);
	p->dColour[3] = 0;
	p->dColour[0] = (short)(-0x4000 / (longlong)(int)life);
	p->dColour[1] = (short)-(p->colour[1] / life);
	p->dColour[2] = (short)(0 / (longlong)(int)life);
	dist = (dist * 4.0f) / self->p.radius;
	dist = dist + dist;
	p->size = dist;
	p->sizeSpeed = dist;
}

/* 0x000aaad0  ClassLib/CParticleSystem.cpp
Release (vtable slot 0). */
void CSmokeSystem_Release(CSmokeSystem *self)
{
	if (self->vb != 0)
		FUN_0003f650(self->vb);
	if (self->h.particles != NULL) {
		CLF_Free(self->h.particles);
		self->h.particles = NULL;
	}
	self->h.maxParticles = 0;
	self->h.numAlive = 0;
}

/* 0x000aaf40  ClassLib/CParticleSystem.cpp
Update (vtable slot 1, CSmokeSystem and CGlowSystem; both have drag at +0x44 and gravity at
+0x48): the sprites move, grow and turn; dead ones are not re-emitted. */
void CSmokeSystem_Update(CSmokeSystem *self, float dt)
{
	CParticle *p;
	short age;
	int i;

	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	for (i = 0; i < self->h.maxParticles; i++) {
		p = &self->h.particles[i];
		if (p->alive == 0)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age < p->life) {
			p->prevPos[0] = p->pos[0];
			p->prevPos[1] = p->pos[1];
			p->prevPos[2] = p->pos[2];
			p->prevPos[3] = p->pos[3];
			p->pos[0] = dt * p->vel[0] + p->pos[0];
			p->pos[1] = dt * p->vel[1] + p->pos[1];
			p->pos[2] = dt * p->vel[2] + p->pos[2];
			p->vel[0] = self->p.drag * p->vel[0];
			p->vel[1] = self->p.drag * p->vel[1];
			p->vel[2] = self->p.drag * p->vel[2];
			p->colour[0] = p->colour[0] + p->dColour[0];
			p->colour[1] = p->colour[1] + p->dColour[1];
			p->colour[2] = p->colour[2] + p->dColour[2];
			p->vel[1] = dt * self->p.gravity + p->vel[1];
			p->size = p->size + p->sizeSpeed;
			p->sizeSpeed = p->sizeSpeed * self->p.drag;
			p->angle = p->spin + p->angle;
			self->h.numAlive = self->h.numAlive + 1;
		} else {
			p->alive = 0;
		}
	}
	if (self->h.numAlive < 1)
		self->p.active = 0;
}

/* ======================================================================== CGlowSystem (vtable 0x00266d20) */

/* 0x000a8fa0  ClassLib/CParticleSystem.cpp
Constructor (vtable only). */
void CGlowSystem_Construct(CGlowSystem *self)
{
	self->h.vtable = vtable_00266d20;
}

/* 0x000a8fb0  ClassLib/CParticleSystem.cpp
Destructor (vtable only). */
void CGlowSystem_Destruct(CGlowSystem *self)
{
	self->h.vtable = vtable_00266d20;
}

/* 0x000a8fc0  ClassLib/CParticleSystem.cpp
Draw (vtable slot 2): rotated sprites, additive. */
void CGlowSystem_Draw(CGlowSystem *self)
{
	byte *base;
	int n;

	if (self->p.active != 0 && 0 < self->h.numAlive) {
		FUN_000438b0();
		FUN_000416c0(0, self->h.texture);
		base = PS_LockVB(self->vb, 4);
		if (base != NULL) {
			n = PS_WriteSprites(&self->h, base);
			FUN_0003f790(self->vb);
			PS_DrawStrips(self->vb, n, 3);
		}
	}
}

/* 0x000aab10  ClassLib/CParticleSystem.cpp
Deleting destructor (vtable slot 5). */
CGlowSystem *CGlowSystem_DeletingDtor(CGlowSystem *self, byte flags)
{
	self->h.vtable = vtable_00266d20;
	if ((flags & 1) != 0)
		CLF_Free(self);
	return self;
}

/* 0x000aab30  ClassLib/CParticleSystem.cpp
Init: parameters (or the defaults), particles and the vertex buffer (6 per particle).
Returns 0 when the buffer cannot be created. */
int CGlowSystem_Init(CGlowSystem *self, CGlowParams *params, int maxParticles)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	if (params == NULL) {
		self->p.active = 0;
		self->p.gravity = 0.0500000007f;
		self->p.speed = 20.0f;
		self->p.drag = 0.949999988f;
		self->p.unknown4c = 200.0f;
		self->p.sizeMin = 1;
		self->p.sizeRange = 4;
		self->p.lifeMin = 0x1e;
		self->p.lifeRange = 0x28;
		self->p.spin = 0.00999999978f;
	} else {
		memcpy(&self->p, params, sizeof(CGlowParams));
	}
	self->h.maxParticles = maxParticles;
	self->h.particles = CLF_Alloc(maxParticles * 0x68, s_CPSFile, 0x908, 0x1001);
	if (PS_CreateVB(&self->vb, self->h.maxParticles * 6, 0x144, 0, 1) != 0) {
		PS_Reset(self);
		return 1;
	}
	return 0;
}

/* 0x000aac00  ClassLib/CParticleSystem.cpp
Emit (vtable slot 4): a grey glow at the emitter with random angle, spin, velocity and
size. */
void CGlowSystem_Emit(CGlowSystem *self, CParticle *p)
{
	short life;
	short grey;
	float r4;
	float r5;
	float r6;
	float r7;
	float size;

	p->alive = 1;
	p->age = 0;
	p->life = (short)((longlong)CLF_Rand() % (longlong)self->p.lifeRange) + (short)self->p.lifeMin;
	p->angle = (float)(CLF_Rand() % 0x168) * 0.0174532924f;
	p->pos[0] = self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = self->h.pos[2];
	p->prevPos[0] = p->pos[0];
	p->prevPos[1] = p->pos[1];
	p->prevPos[2] = p->pos[2];
	p->pos[3] = 1.0f;
	p->prevPos[3] = 1.0f;
	CLF_Rand(); /* drawn and not used */
	p->spin = (float)CLF_Rand() * self->p.spin * 3.05185094e-05f;
	r4 = (float)CLF_Rand();
	r5 = (float)CLF_Rand();
	r6 = (float)CLF_Rand();
	r7 = (float)CLF_Rand();
	p->vel[0] = r7 * 3.05185094e-05f * self->p.speed * (r4 * 3.05185094e-05f - 0.5f);
	p->vel[1] = PS_RandUnit() * self->p.speed * (r5 * 1.52592547e-05f);
	p->vel[2] = PS_RandUnit() * self->p.speed * (r6 * 3.05185094e-05f - 0.5f);
	grey = (short)(CLF_FTOL((float)(CLF_Rand() & 0xf) * 0.0174532924f + 16.0f) << 7);
	life = p->life;
	p->colour[3] = 0x4000;
	p->dColour[3] = 0;
	p->colour[0] = grey;
	p->colour[1] = grey;
	p->colour[2] = grey;
	p->dColour[0] = (short)-(grey / life);
	p->dColour[1] = (short)-(p->colour[1] / life);
	p->dColour[2] = (short)-(p->colour[2] / life);
	size = (float)CLF_Rand() * (float)self->p.sizeRange * 3.05185094e-05f + (float)self->p.sizeMin;
	size = size + size;
	p->size = size;
	p->sizeSpeed = size;
}

/* 0x000aaf00  ClassLib/CParticleSystem.cpp
Release (vtable slot 0). */
void CGlowSystem_Release(CGlowSystem *self)
{
	if (self->vb != 0)
		FUN_0003f650(self->vb);
	if (self->h.particles != NULL) {
		CLF_Free(self->h.particles);
		self->h.particles = NULL;
	}
	self->h.maxParticles = 0;
	self->h.numAlive = 0;
}

/* ======================================================================== CSparkPointSystem (vtable 0x00266d38) */

/* 0x000a9140  ClassLib/CParticleSystem.cpp
Constructor (vtable only). */
void CSparkPointSystem_Construct(CSparkPointSystem *self)
{
	self->h.vtable = vtable_00266d38;
}

/* 0x000a9150  ClassLib/CParticleSystem.cpp
Destructor (vtable only). */
void CSparkPointSystem_Destruct(CSparkPointSystem *self)
{
	self->h.vtable = vtable_00266d38;
}

/* 0x000a9160  ClassLib/CParticleSystem.cpp
Default parameters. */
void CSparkPointSystem_DefaultParams(CSparkPointParams *p)
{
	p->active = 0;
	p->bounce = 1;
	p->gravity = -0.5f;
	p->speed = 10.0f;
	p->drag = 0.970000029f;
	p->unknown68 = 0.00999999978f;
	p->unknown6c = 1.0f;
	p->lifeMin = 0x10;
	p->lifeRange = 0x20;
	p->colour[0] = 8;
	p->colour[1] = 4;
	p->colour[2] = 0;
	p->colour2[0] = 0x60;
	p->colour2[1] = 0x40;
	p->colour2[2] = 0x20;
	p->sizeMin = 3;
	p->sizeRange = 2;
	p->pointSize = 80.0f;
}

/* 0x000a91e0  ClassLib/CParticleSystem.cpp
Draw (vtable slot 2): beams (1 wide at the previous position) and point sprites in the
second colour. */
void CSparkPointSystem_Draw(CSparkPointSystem *self)
{
	byte *base;
	uint *points;
	int nverts;
	int npoints;

	if (self->p.active == 0 || self->h.numAlive < 1)
		return;
	FUN_000438b0();
	base = PS_LockVB(self->vb, 4);
	if (base == NULL)
		return;
	nverts = PS_WriteBeams(&self->h, base, 1);
	FUN_0003f790(self->vb);
	points = (uint *)PS_LockVB(self->pointVB, 4);
	if (points == NULL)
		return;
	npoints = PS_WritePoints(&self->h, points, 1);
	FUN_0003f790(self->pointVB);
	FUN_00043b70(1);
	PS_SetState(6, 1, 0);
	PS_SetState(5, 0, 0);
	FUN_00044b00(10, 1, 0);
	FUN_000416c0(0, self->h.texture);
	if (nverts != 0)
		PS_DrawVB(4, self->vb, 0, nverts);
	PS_BeginPoints(self->texture2);
	if (npoints != 0)
		CLF_DrawPointSprites(self->pointVB, 0, npoints, self->p.pointSize);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 1);
	FUN_00043b70(1);
	FUN_00046d00(6);
	FUN_00046d00(5);
}

/* 0x000a9510  ClassLib/CParticleSystem.cpp
Reset (vtable slot 3): inactive, every particle dead. */
void CSparkPointSystem_Reset(CSparkPointSystem *self)
{
	int i;

	self->p.active = 0;
	for (i = 0; i < self->h.maxParticles; i++)
		self->h.particles[i].alive = 0;
}

/* 0x000ab050  ClassLib/CParticleSystem.cpp
Deleting destructor (vtable slot 5). */
CSparkPointSystem *CSparkPointSystem_DeletingDtor(CSparkPointSystem *self, byte flags)
{
	self->h.vtable = vtable_00266d38;
	if ((flags & 1) != 0)
		CLF_Free(self);
	return self;
}

/* 0x000ab070  ClassLib/CParticleSystem.cpp
Init: parameters (or the defaults), particles, the beam buffer (10 per particle) and the
point buffer. Returns 0 when a buffer cannot be created. */
int CSparkPointSystem_Init(CSparkPointSystem *self, CSparkPointParams *params, int maxParticles)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	self->h.texture = 0;
	self->texture2 = 0;
	if (params == NULL)
		CSparkPointSystem_DefaultParams(&self->p);
	else
		memcpy(&self->p, params, sizeof(CSparkPointParams));
	self->h.maxParticles = maxParticles;
	self->h.particles = CLF_Alloc(maxParticles * 0x68, s_CPSFile, 0xa53, 0x1001);
	if (PS_CreateVB(&self->vb, self->h.maxParticles * 10, 0x144, 0, 1) != 0 &&
	    PS_CreateVB(&self->pointVB, self->h.maxParticles, 0x42, 0, 1) != 0) {
		PS_Reset(self);
		return 1;
	}
	return 0;
}

/* 0x000ab120  ClassLib/CParticleSystem.cpp
Emit (vtable slot 4): a spark at the emitter along the emission direction with a little
random spread. */
void CSparkPointSystem_Emit(CSparkPointSystem *self, CParticle *p)
{
	short life;
	float dirY;
	float r3;
	float r4;
	float s;

	p->alive = 1;
	p->age = 0;
	p->life = (short)((longlong)CLF_Rand() % (longlong)self->p.lifeRange) + (short)self->p.lifeMin;
	life = p->life;
	p->colour[0] = (short)(self->p.colour[0] << 7);
	p->colour[1] = (short)(self->p.colour[1] << 7);
	p->colour[2] = (short)(self->p.colour[2] << 7);
	p->dColour[0] = (short)-(p->colour[0] / life);
	p->dColour[1] = (short)-(p->colour[1] / life);
	p->dColour[2] = (short)-(p->colour[2] / life);
	p->colour2[0] = (short)(self->p.colour2[0] << 7);
	p->colour2[1] = (short)(self->p.colour2[1] << 7);
	p->colour2[2] = (short)(self->p.colour2[2] << 7);
	p->dColour2[0] = (short)-(p->colour2[0] / life);
	p->dColour2[1] = (short)-(p->colour2[1] / life);
	p->dColour2[2] = (short)-(p->colour2[2] / life);
	p->sizeSpeed = 0.0f;
	p->size = (float)((int)((longlong)CLF_Rand() % (longlong)self->p.sizeRange) + self->p.sizeMin);
	p->pos[0] = self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = self->h.pos[2];
	p->pos[3] = 1.0f;
	p->prevPos[0] = self->h.pos[0];
	p->prevPos[1] = self->h.pos[1];
	p->prevPos[3] = 1.0f;
	p->prevPos[2] = self->h.pos[2];
	dirY = self->dir[1];
	r3 = (float)CLF_Rand();
	r4 = (float)CLF_Rand();
	s = PS_RandUnit() * self->p.speed;
	p->vel[0] = s * ((r3 * 6.10370216e-06f + self->dir[0]) - 0.100000001f);
	p->vel[1] = s * dirY;
	p->vel[2] = s * ((r4 * 6.10370216e-06f + self->dir[2]) - 0.100000001f);
}

/* 0x000ab340  ClassLib/CParticleSystem.cpp
Release (vtable slot 0). */
void CSparkPointSystem_Release(CSparkPointSystem *self)
{
	if (self->vb != 0)
		FUN_0003f650(self->vb);
	if (self->pointVB != 0)
		FUN_0003f650(self->pointVB);
	if (self->h.particles != NULL) {
		CLF_Free(self->h.particles);
		self->h.particles = NULL;
	}
	self->h.maxParticles = 0;
	self->h.numAlive = 0;
}

/* 0x000ab390  ClassLib/CParticleSystem.cpp
Update (vtable slot 1): the sparks move (stopping at the emitter height when enabled);
dead ones are not re-emitted. */
void CSparkPointSystem_Update(CSparkPointSystem *self, float dt)
{
	CParticle *p;
	short age;
	int i;

	self->h.numAlive = 0;
	if (self->p.active == 0)
		return;
	for (i = 0; i < self->h.maxParticles; i++) {
		p = &self->h.particles[i];
		if (p->alive == 0)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age < p->life) {
			p->prevPos[0] = p->pos[0];
			p->prevPos[1] = p->pos[1];
			p->prevPos[2] = p->pos[2];
			p->prevPos[3] = p->pos[3];
			p->pos[0] = dt * p->vel[0] + p->pos[0];
			p->pos[1] = dt * p->vel[1] + p->pos[1];
			p->pos[2] = dt * p->vel[2] + p->pos[2];
			p->vel[0] = self->p.drag * p->vel[0];
			p->vel[1] = self->p.drag * p->vel[1];
			p->vel[2] = self->p.drag * p->vel[2];
			p->colour[0] = p->colour[0] + p->dColour[0];
			p->colour[1] = p->colour[1] + p->dColour[1];
			p->colour[2] = p->colour[2] + p->dColour[2];
			p->colour2[0] = p->colour2[0] + p->dColour2[0];
			p->vel[1] = dt * self->p.gravity + p->vel[1];
			p->colour2[1] = p->colour2[1] + p->dColour2[1];
			p->colour2[2] = p->colour2[2] + p->dColour2[2];
			self->h.numAlive = self->h.numAlive + 1;
			if (self->p.bounce != 0 && p->pos[1] < self->h.pos[1])
				p->pos[1] = self->h.pos[1];
		} else {
			p->alive = 0;
		}
	}
	if (self->h.numAlive < 1)
		self->p.active = 0;
}

/* ======================================================================== vtables (.rdata 0x00266cd4-0x00266d50) */

/* 0x00266cd4 */
const void *const vtable_00266cd4[1] = { (const void *)CParticleSystemList_DeletingDtor };
/* 0x00266cd8 */
const void *const vtable_00266cd8[6] = {
	(const void *)CParticleSystem_Release, (const void *)CParticleSystem_Update,
	(const void *)CParticleSystem_Draw, (const void *)CParticleSystem_Reset,
	(const void *)CParticleSystem_Emit, (const void *)CParticleSystem_DeletingDtor,
};
/* 0x00266cf0 */
const void *const vtable_00266cf0[6] = {
	(const void *)CSparkSystem_Release, (const void *)CSparkSystem_Update,
	(const void *)CSparkSystem_Draw, (const void *)CSparkSystem_Reset,
	(const void *)CSparkSystem_Emit, (const void *)CSparkSystem_DeletingDtor,
};
/* 0x00266d08 */
const void *const vtable_00266d08[6] = {
	(const void *)CSmokeSystem_Release, (const void *)CSmokeSystem_Update,
	(const void *)CSmokeSystem_Draw, (const void *)CSmokeSystem_Reset,
	(const void *)CSmokeSystem_Emit, (const void *)CSmokeSystem_DeletingDtor,
};
/* 0x00266d20  (slots 1 and 3 are CSmokeSystem's) */
const void *const vtable_00266d20[6] = {
	(const void *)CGlowSystem_Release, (const void *)CSmokeSystem_Update,
	(const void *)CGlowSystem_Draw, (const void *)CSmokeSystem_Reset,
	(const void *)CGlowSystem_Emit, (const void *)CGlowSystem_DeletingDtor,
};
/* 0x00266d38 */
const void *const vtable_00266d38[6] = {
	(const void *)CSparkPointSystem_Release, (const void *)CSparkPointSystem_Update,
	(const void *)CSparkPointSystem_Draw, (const void *)CSparkPointSystem_Reset,
	(const void *)CSparkPointSystem_Emit, (const void *)CSparkPointSystem_DeletingDtor,
};
