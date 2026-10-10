/*
Drillers/CDrillers_Particle.c  <-  C:\BlitzParty\Game\Src\Drillers\CDrillers_Particle.cpp

Three small particle systems (DrParticleSystem, drillers.h), each 0x50 bytes:
  sparks  vtable 0x0026cbbc  beams from the previous position, gravity, floor bounce, the
                             colour of a player (or orange)
  dust    vtable 0x0026cbd4  rotating sprites, red to transparent
  smoke   vtable 0x0026cbec  rotating grey sprites (additive), growing
The particles are classlib_fx's CParticle (0x68 bytes, 8.7 fixed-point colours).

0x00123b30 and 0x00124130 name this file in their allocations; the functions from
0x00123540 to 0x00124910 are tagged with it or lie between them.
*/

#include "drillers.h"

/* the vertex write pointers of the three Draw functions (start and current) */
#define DR_SPARK_VTX0 DR_VAR(unsigned int *, 0x003020d4u)
#define DR_SPARK_VTX DR_VAR(unsigned int *, 0x003020d8u)
#define DR_DUST_VTX0 DR_VAR(unsigned int *, 0x003020dcu)
#define DR_DUST_VTX DR_VAR(unsigned int *, 0x003020e0u)
#define DR_SMOKE_VTX0 DR_VAR(unsigned int *, 0x003020e4u)
#define DR_SMOKE_VTX DR_VAR(unsigned int *, 0x003020e8u)

/* the textures of the resources object (0x002aa978 sparks, 0x002aa97c dust) */
#define DR_TEX_SPARK DR_VAR(int, 0x002aa978u)
#define DR_TEX_DUST DR_VAR(int, 0x002aa97cu)

/* the colours of the four players (red, green, blue bytes) */
#define DR_PLAYER_R(i) DR_VAR(unsigned char, 0x002aa90cu + (i))
#define DR_PLAYER_G(i) DR_VAR(unsigned char, 0x002aa910u + (i))
#define DR_PLAYER_B(i) DR_VAR(unsigned char, 0x002aa914u + (i))

/* the fog red of variant 0 (+4 per variant); used as an angle offset */
#define DR_FOG_R0 DR_VAR(float, 0x00301c28u)

/* 1 / 32767 */
#define DR_RAND_SCALE 3.0518509447574615e-05f
#define DR_DEG 0.017453292f

/* the vertices of one particle are 7 words (0x1c bytes) each */
#define DR_VTX_WORDS 7

/* 0x0026cbbc  vtable of the sparks (rebuilt from the XBE) */
const void *const vtable_0026cbbc[6] = {
	(const void *)DrPs_Release,          /* +0x00 */
	(const void *)DrSparks_Update,       /* +0x04 */
	(const void *)DrSparks_Draw,         /* +0x08 */
	(const void *)DrPs_Stop,             /* +0x0c */
	(const void *)DrSparks_InitParticle, /* +0x10 */
	(const void *)DrSparks_DeletingDtor, /* +0x14 */
};

/* 0x0026cbd4  vtable of the dust (rebuilt from the XBE) */
const void *const vtable_0026cbd4[6] = {
	(const void *)DrPs_Release,          /* +0x00 */
	(const void *)DrPs_UpdateSprites,    /* +0x04 */
	(const void *)DrDust_Draw,           /* +0x08 */
	(const void *)DrPs_Stop,             /* +0x0c */
	(const void *)DrDust_InitParticle,   /* +0x10 */
	(const void *)DrDust_DeletingDtor,   /* +0x14 */
};

/* 0x0026cbec  vtable of the smoke (rebuilt from the XBE) */
const void *const vtable_0026cbec[6] = {
	(const void *)DrPs_Release,          /* +0x00 */
	(const void *)DrPs_UpdateSprites,    /* +0x04 */
	(const void *)DrSmoke_Draw,          /* +0x08 */
	(const void *)DrPs_Stop,             /* +0x0c */
	(const void *)DrSmoke_InitParticle,  /* +0x10 */
	(const void *)DrSmoke_DeletingDtor,  /* +0x14 */
};

/* a random value in [0, 1] */
static float dr_frand(void)
{
	return (float)dr_rand() * DR_RAND_SCALE;
}

/* 0x00123540  Drillers (gap; CDrillers_Particle.cpp inferred)
Destructor of the sparks. */
void DrSparks_Dtor(DrParticleSystem *self)
{
	self->h.vtable = vtable_0026cbbc;
}

/* the end of a Draw: the render states and the strip of the written vertices */
static void dr_ps_draw_end(DrParticleSystem *self, unsigned int *start, unsigned int *end,
	int projection, unsigned int blend)
{
	int count;

	bdVertexBufferUnlock(self->vb);
	bdSetProjectionMode(projection);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, blend, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(8, 0, 0);
	count = (int)(end - start) / DR_VTX_WORDS;
	if (count != 0)
		bdDrawVertexBuffer(4, self->vb, 0, count);
	bdSetRenderState(8, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetProjectionMode(1);
}

/* 0x00123550  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbbc +0x08)
Draw: every live particle is a beam from its previous position to its position (two
colours); the eight vertices are joined to the strip by a copy of the first and the last. */
void DrSparks_Draw(DrParticleSystem *self)
{
	unsigned int *v;
	CParticle *p;
	int c0[4];
	int c1[4];
	int i;

	if ((self->p.flags & 1) == 0 || self->h.numAlive <= 0)
		return;
	bdSetWorldIdentity();
	bdSetTexture(0, (struct bdTexture *)(uintptr_t)self->h.texture);
	v = (unsigned int *)bdVertexBufferLock(self->vb, 4);
	DR_SPARK_VTX0 = v;
	DR_SPARK_VTX = v;
	if (v == NULL)
		return;
	for (i = 0; i < self->h.maxParticles; i++) {
		p = &self->h.particles[i];
		if ((p->alive & 1) == 0)
			continue;
		c0[0] = p->colour[0] >> 7;
		c0[1] = p->colour[1] >> 7;
		c0[2] = p->colour[2] >> 7;
		c0[3] = 0xff;
		c1[0] = p->colour2[0] >> 7;
		c1[1] = p->colour2[1] >> 7;
		c1[2] = p->colour2[2] >> 7;
		c1[3] = 0xff;
		if (bdDrawBeam3D(p->prevPos, p->size, p->size, c0, p->pos, p->size, p->size, c1, NULL,
				(float *)(v + DR_VTX_WORDS), 0.0f) != 0) {
			v = DR_SPARK_VTX;
			memcpy(v, v + DR_VTX_WORDS, DR_VTX_WORDS * 4);
			memcpy(v + 9 * DR_VTX_WORDS, v + 8 * DR_VTX_WORDS, DR_VTX_WORDS * 4);
			DR_SPARK_VTX = v + 10 * DR_VTX_WORDS;
		}
		v = DR_SPARK_VTX;
	}
	dr_ps_draw_end(self, DR_SPARK_VTX0, DR_SPARK_VTX, 0, 1);
}

/* 0x00123730  Drillers/CDrillers_Particle.cpp
Burst: every particle starts again from the emitter (InitParticle with `colour`). */
void DrPs_Burst(DrParticleSystem *self, int colour)
{
	int i;

	self->h.numAlive = 0;
	for (i = 0; i < self->h.maxParticles; i++)
		DR_VFN(self, 0x10, void (*)(DrParticleSystem *, CParticle *, int))(self,
			&self->h.particles[i], colour);
}

/* 0x00123770  Drillers/CDrillers_Particle.cpp
Constructor of the dust. */
void DrDust_Ctor(DrParticleSystem *self)
{
	self->h.vtable = vtable_0026cbd4;
}

/* 0x00123780  Drillers/CDrillers_Particle.cpp
Destructor of the dust. */
void DrDust_Dtor(DrParticleSystem *self)
{
	self->h.vtable = vtable_0026cbd4;
}

/* Draw of the dust and the smoke: one rotating sprite per live particle (four vertices,
   joined to the strip by a copy of the first and the last) */
static void dr_ps_draw_sprites(DrParticleSystem *self, unsigned int **start,
	unsigned int **cur, unsigned int blend)
{
	unsigned int *v;
	CParticle *p;
	int i;

	if ((self->p.flags & 1) == 0 || self->h.numAlive <= 0)
		return;
	bdSetWorldIdentity();
	bdSetTexture(0, (struct bdTexture *)(uintptr_t)self->h.texture);
	v = (unsigned int *)bdVertexBufferLock(self->vb, 4);
	*start = v;
	*cur = v;
	if (v == NULL)
		return;
	for (i = 0; i < self->h.maxParticles; i++) {
		p = &self->h.particles[i];
		if ((p->alive & 1) != 0 &&
				bdDrawSprite3D(p->pos, p->size, p->size, p->angle, NULL, 0, 0,
					p->colour[0] >> 9, p->colour[1] >> 9, p->colour[2] >> 9,
					p->colour[3] >> 7, (float *)(v + DR_VTX_WORDS), 0.0f) != 0) {
			v = *cur;
			memcpy(v, v + DR_VTX_WORDS, DR_VTX_WORDS * 4);
			memcpy(v + 5 * DR_VTX_WORDS, v + 4 * DR_VTX_WORDS, DR_VTX_WORDS * 4);
			*cur = v + 6 * DR_VTX_WORDS;
		}
		v = *cur;
	}
	dr_ps_draw_end(self, *start, *cur, 1, blend);
}

/* 0x00123790  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbd4 +0x08)
Draw of the dust (blend mode 1). */
void DrDust_Draw(DrParticleSystem *self)
{
	dr_ps_draw_sprites(self, &DR_DUST_VTX0, &DR_DUST_VTX, 1);
}

/* 0x00123930  Drillers/CDrillers_Particle.cpp
Constructor of the smoke. */
void DrSmoke_Ctor(DrParticleSystem *self)
{
	self->h.vtable = vtable_0026cbec;
}

/* 0x00123940  Drillers/CDrillers_Particle.cpp
Destructor of the smoke. */
void DrSmoke_Dtor(DrParticleSystem *self)
{
	self->h.vtable = vtable_0026cbec;
}

/* 0x00123950  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbec +0x08)
Draw of the smoke (blend mode 3, additive). */
void DrSmoke_Draw(DrParticleSystem *self)
{
	dr_ps_draw_sprites(self, &DR_SMOKE_VTX0, &DR_SMOKE_VTX, 3);
}

/* 0x00123af0  Drillers/CDrillers_Particle.cpp (vtable +0x0c)
Stop: the system is no longer active. */
void DrPs_Stop(DrParticleSystem *self)
{
	self->p.flags &= ~1u;
}

/* 0x00123b00  Drillers/CDrillers_Particle.cpp
Constructor of the sparks. */
void DrSparks_Ctor(DrParticleSystem *self)
{
	self->h.vtable = vtable_0026cbbc;
}

/* 0x00123b10  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbbc +0x14) */
DrParticleSystem *DrSparks_DeletingDtor(DrParticleSystem *self, unsigned int flags)
{
	self->h.vtable = vtable_0026cbbc;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x00123b30  Drillers/CDrillers_Particle.cpp
Init of the sparks: the parameters (default: inactive, floor bounce, speed 45, drag 0.97,
gravity -0.2), `count` particles and a vertex buffer of 10 vertices each. Returns 0 when
the vertex buffer fails. */
int DrSparks_Init(DrParticleSystem *self, const DrPsParams *params, int count)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	if (params == NULL) {
		self->p.flags = (self->p.flags & ~1u) | 2;
		self->p.gravity = -0.2f;
		self->p.speed = 45.0f;
		self->p.drag = 0.97f;
	} else {
		self->p = *params;
	}
	self->h.maxParticles = count;
	self->h.particles = (CParticle *)DR_NEW(count * 0x68, DR_FILE_PARTICLE, 0xf6, 0x1001);
	if (bdVertexBufferCreate(&self->vb, self->h.maxParticles * 10, 0x144, 0, 1) == 0)
		return 0;
	self->h.texture = DR_TEX_SPARK;
	return 1;
}

/* 0x00123be0  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbbc +0x10)
A new spark at the emitter: 60 ticks, white-hot to the colour of player `colour` (orange
for -1), size 3-8 x 4, a random velocity (for -1 in a cone around the fog angle). */
void DrSparks_InitParticle(DrParticleSystem *self, CParticle *p, int colour)
{
	float a;
	float b;
	float c;
	float angle;
	int r1;
	int r2;
	int r3;

	p->alive |= 1;
	p->age = 0;
	p->life = 0x3c;
	p->colour[0] = 0x800;
	p->colour[1] = 0x280;
	p->colour[2] = 0x80;
	p->dColour[0] = -0x22;
	p->dColour[1] = -10;
	p->dColour[2] = -2;
	if (colour == -1) {
		p->colour2[0] = 0x4000;
		p->colour2[1] = 0x3200;
		p->colour2[2] = 0x1400;
	} else {
		p->colour2[0] = (short)(DR_PLAYER_R(colour) << 7);
		p->colour2[1] = (short)(DR_PLAYER_G(colour) << 7);
		p->colour2[2] = (short)(DR_PLAYER_B(colour) << 7);
	}
	p->dColour2[0] = (short)-(p->colour2[0] / 0x3c);
	p->dColour2[1] = (short)-(p->colour2[1] / 0x3c);
	p->dColour2[2] = (short)-(p->colour2[2] / 0x3c);
	p->sizeSpeed = 0.0f;
	p->size = (float)(dr_rand() % 6 + 3);
	p->pos[0] = self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = self->h.pos[2];
	p->pos[3] = 1.0f;
	p->prevPos[0] = self->h.pos[0];
	p->prevPos[1] = self->h.pos[1];
	p->prevPos[2] = self->h.pos[2];
	p->prevPos[3] = 1.0f;
	if (colour == -1) {
		angle = ((float)(dr_rand() % 200) - 100.0f + DR_FOG_R0) * DR_DEG;
		r1 = dr_rand();
		r2 = dr_rand();
		r3 = dr_rand();
		a = sinf(angle) * ((float)r1 * DR_RAND_SCALE * 0.5f);
		b = -0.5f - (float)r2 * DR_RAND_SCALE * 0.5f;
		c = cosf(angle) * ((float)r3 * DR_RAND_SCALE * 0.5f);
	} else {
		a = dr_frand() - 0.5f;
		b = dr_frand() * 0.5f;
		c = dr_frand() - 0.5f;
	}
	p->vel[0] = dr_frand() * self->p.speed * a;
	p->vel[1] = dr_frand() * self->p.speed * b;
	p->vel[2] = dr_frand() * self->p.speed * c;
	p->size *= 4.0f;
	p->sizeSpeed *= 4.0f;
	p->vel[0] *= 4.0f;
	p->vel[1] *= 4.0f;
	p->vel[2] *= 4.0f;
}

/* 0x00123fb0  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbbc +0x04)
Update: the sparks move, slow down, fall and change colour; with floor bounce one that
falls below the emitter's height bounces at half speed. A spark dies at the end of its
life; the system stops when none is alive. */
void DrSparks_Update(DrParticleSystem *self, float dt)
{
	CParticle *p;
	short age;
	int i;

	self->h.numAlive = 0;
	if ((self->p.flags & 1) == 0)
		return;
	for (i = 0; i < self->h.maxParticles; i++) {
		p = &self->h.particles[i];
		if ((p->alive & 1) == 0)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age >= p->life) {
			p->alive &= 0xfe;
			continue;
		}
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
		p->colour[0] += p->dColour[0];
		p->colour[1] += p->dColour[1];
		p->colour[2] += p->dColour[2];
		p->colour2[0] += p->dColour2[0];
		p->vel[1] = dt * self->p.gravity + p->vel[1];
		p->colour2[1] += p->dColour2[1];
		p->colour2[2] += p->dColour2[2];
		self->h.numAlive++;
		if ((self->p.flags & 2) != 0 && p->pos[1] < self->h.pos[1]) {
			p->vel[0] *= 0.5f;
			p->vel[1] *= 0.5f;
			p->vel[2] *= 0.5f;
			p->vel[1] = 0.0f;
			p->pos[1] = self->h.pos[1];
		}
	}
	if (self->h.numAlive < 1)
		self->p.flags &= ~1u;
}

/* 0x00124110  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbd4 +0x14) */
DrParticleSystem *DrDust_DeletingDtor(DrParticleSystem *self, unsigned int flags)
{
	self->h.vtable = vtable_0026cbd4;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x00124130  Drillers/CDrillers_Particle.cpp
Init of the dust and the smoke: the parameters (default: inactive, speed 100, drag 0.95,
no gravity), `count` particles and a vertex buffer of 6 vertices each. Returns 0 when the
vertex buffer fails. */
int DrDust_Init(DrParticleSystem *self, const DrPsParams *params, int count)
{
	self->h.particles = NULL;
	self->h.numAlive = 0;
	if (params == NULL) {
		self->p.flags &= ~1u;
		self->p.gravity = 0.0f;
		self->p.speed = 100.0f;
		self->p.drag = 0.95f;
	} else {
		self->p = *params;
	}
	self->h.maxParticles = count;
	self->h.particles = (CParticle *)DR_NEW(count * 0x68, DR_FILE_PARTICLE, 0x275, 0x1001);
	if (bdVertexBufferCreate(&self->vb, self->h.maxParticles * 6, 0x144, 0, 1) == 0)
		return 0;
	self->h.texture = DR_TEX_DUST;
	return 1;
}

/* 0x001241e0  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbd4 +0x10)
A new dust particle at the emitter: 15-24 ticks, a random angle and spin, a random
velocity, red with a green part that grows with the size, fading out. */
void DrDust_InitParticle(DrParticleSystem *self, CParticle *p, int colour)
{
	float base;
	float a;
	float b;
	float c;
	short green;
	short life;

	(void)colour;
	p->alive |= 1;
	p->age = 0;
	p->life = (short)((dr_rand() % 0x14 + 0x1e) / 2);
	p->angle = (float)(dr_rand() % 0x168) * DR_DEG;
	base = dr_frand() * 200.0f;
	p->pos[0] = self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = self->h.pos[2];
	p->prevPos[0] = p->pos[0];
	p->prevPos[1] = p->pos[1];
	p->prevPos[2] = p->pos[2];
	p->pos[3] = 1.0f;
	p->prevPos[3] = 1.0f;
	p->spin = dr_frand() * 0.01f;
	a = dr_frand() - 0.5f;
	b = dr_frand() * 0.5f;
	c = dr_frand() - 0.5f;
	p->vel[0] = dr_frand() * self->p.speed * a;
	p->vel[1] = dr_frand() * self->p.speed * b;
	p->vel[2] = dr_frand() * self->p.speed * c;
	p->colour[0] = 0x4000;
	green = (short)DR_FTOL(base * 64.0f * 0.005f);
	life = p->life;
	p->colour[2] = 0;
	p->colour[3] = 0x7f80;
	p->colour[1] = (short)(green << 7);
	p->dColour[3] = 0;
	p->size = base * 4.0f * 0.005f * 8.0f;
	p->sizeSpeed = p->size;
	p->dColour[0] = (short)(-0x4000 / life);
	p->dColour[1] = (short)-((short)(green << 7) / life);
	p->dColour[2] = (short)(0 / life);
}

/* 0x001244c0  Drillers/CDrillers_Particle.cpp (vtables 0x0026cbd4 and 0x0026cbec +0x04)
Update of the dust and the smoke: the particles move, slow down, fall, fade, grow (by a
slowing growth) and turn. A particle dies at the end of its life; the system stops when
none is alive. */
void DrPs_UpdateSprites(DrParticleSystem *self, float dt)
{
	CParticle *p;
	short age;
	int i;

	self->h.numAlive = 0;
	if ((self->p.flags & 1) == 0)
		return;
	for (i = 0; i < self->h.maxParticles; i++) {
		p = &self->h.particles[i];
		if ((p->alive & 1) == 0)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age >= p->life) {
			p->alive &= 0xfe;
			continue;
		}
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
		p->colour[0] += p->dColour[0];
		p->colour[1] += p->dColour[1];
		p->colour[2] += p->dColour[2];
		p->vel[1] = dt * self->p.gravity + p->vel[1];
		p->size = p->size + p->sizeSpeed;
		p->sizeSpeed = self->p.drag * p->sizeSpeed;
		p->angle = p->spin + p->angle;
		self->h.numAlive++;
	}
	if (self->h.numAlive < 1)
		self->p.flags &= ~1u;
}

/* 0x001245d0  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbec +0x14) */
DrParticleSystem *DrSmoke_DeletingDtor(DrParticleSystem *self, unsigned int flags)
{
	self->h.vtable = vtable_0026cbec;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x001245f0  Drillers/CDrillers_Particle.cpp (vtable 0x0026cbec +0x10; the table entry
0x001246e0 is its second chunk)
A new smoke particle at the emitter: 60-119 ticks, a random angle and spin, a random
velocity, dark grey fading out, size 2-10 x 4. */
void DrSmoke_InitParticle(DrParticleSystem *self, CParticle *p, int colour)
{
	float a;
	float b;
	float c;
	float size;
	short grey;
	short life;

	(void)colour;
	p->alive |= 1;
	p->age = 0;
	p->life = (short)(dr_rand() % 0x3c + 0x3c);
	p->angle = (float)(dr_rand() % 0x168) * DR_DEG;
	(void)dr_rand(); /* drawn and not used */
	p->pos[0] = self->h.pos[0];
	p->pos[1] = self->h.pos[1];
	p->pos[2] = self->h.pos[2];
	p->prevPos[0] = p->pos[0];
	p->prevPos[1] = p->pos[1];
	p->prevPos[2] = p->pos[2];
	p->pos[3] = 1.0f;
	p->prevPos[3] = 1.0f;
	p->spin = dr_frand() * 0.01f;
	a = dr_frand() - 0.5f;
	b = dr_frand() * 0.5f;
	c = dr_frand() - 0.5f;
	p->vel[0] = dr_frand() * self->p.speed * a;
	p->vel[1] = dr_frand() * self->p.speed * b;
	p->vel[2] = dr_frand() * self->p.speed * c;
	grey = (short)DR_FTOL((float)(dr_rand() % 64) * DR_DEG + 32.0f);
	life = p->life;
	p->colour[3] = 0x7f80;
	p->dColour[3] = 0;
	grey = (short)(grey << 7);
	p->colour[0] = grey;
	p->colour[1] = grey;
	p->colour[2] = grey;
	p->dColour[0] = (short)-(grey / life);
	p->dColour[1] = (short)-(p->colour[1] / life);
	p->dColour[2] = (short)-(p->colour[2] / life);
	size = dr_frand() * 4.0f + 1.0f;
	size = size + size;
	p->size = size;
	p->sizeSpeed = size;
	p->size *= 4.0f;
	p->sizeSpeed *= 4.0f;
}

/* 0x00124910  Drillers/CDrillers_Particle.cpp (vtable +0x00)
Release: frees the vertex buffer and the particles. */
void DrPs_Release(DrParticleSystem *self)
{
	if (self->vb != NULL)
		bdVertexBufferDestroy(self->vb);
	if (self->h.particles != NULL) {
		DR_DELETE(self->h.particles);
		self->h.particles = NULL;
	}
	self->h.maxParticles = 0;
	self->h.numAlive = 0;
}

/* ---------- the "rain" extra system (gap functions after the particle file; the three
   callbacks 0x00124950, 0x001249c0 and 0x00124b00 are not in of_functions.tsv) */

/* 0x003020f0  the two vertices of a drop's line */
#define DR_RAIN_VTX DR_PTR(bdVertex, 0x003020f0u)

/* 0x00124950  Drillers (not in of_functions.tsv; update callback of "rain")
The drops fall (a quarter of their velocity per tick); one below its floor goes. */
int FUN_00124950(struct BExtraSystem *system, int count, void **items, void *arg)
{
	DrRainDrop *d;
	int i = 0;

	(void)arg;
	while (i < count) {
		d = (DrRainDrop *)items[i];
		if (d->pos[1] >= d->a.floor) {
			d->age++;
			i++;
			d->pos[0] = d->a.vel[0] * 0.25f + d->pos[0];
			d->pos[1] = d->a.vel[1] * 0.25f + d->pos[1];
			d->pos[2] = d->a.vel[2] * 0.25f + d->pos[2];
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x001249c0  Drillers (not in of_functions.tsv; draw callback of "rain")
Every drop is a line from pos + tail (its alpha) to pos + tail - vel / 2 (grey 127). */
int FUN_001249c0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	unsigned int saved[30];
	bdVertex *v = DR_RAIN_VTX;
	DrRainDrop *d;
	int i;

	(void)system;
	(void)arg;
	bdSaveRenderStates(saved);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(3, 0, 0);
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
	for (i = 0; i < count; i++) {
		d = (DrRainDrop *)items[i];
		v[0].x = d->pos[0] + d->a.tail[0];
		v[0].y = d->a.tail[1] + d->pos[1];
		v[0].z = d->pos[2] + d->a.tail[2];
		v[0].color = DAT_002bc898[(unsigned char)d->a.alpha] | DAT_002bd83c | DAT_002bc68c |
			DAT_002bd014;
		v[1].x = (d->pos[0] + d->a.tail[0]) - d->a.vel[0] * 0.5f;
		v[1].y = (d->a.tail[1] + d->pos[1]) - d->a.vel[1] * 0.5f;
		v[1].color = DAT_002bc898[0] | DAT_002bd83c | DAT_002bc68c | DAT_002bd014;
		v[1].z = (d->pos[2] + d->a.tail[2]) - d->a.vel[2] * 0.5f;
		bdDrawPrimitive(2, v, 2);
	}
	bdRestoreRenderStates(saved);
	return 0;
}

/* 0x00124b00  Drillers (not in of_functions.tsv; item callback of "rain")
A new drop from a DrRainArgs (passed by pointer, see drillers.h): at the start height,
x and z random in the ranges. */
int FUN_00124b00(struct BExtraSystem *system, DrRainDrop *d, void *args)
{
	va_list ap;
	const DrRainArgs *a;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	a = va_arg(ap, const DrRainArgs *);
	d->a = *a;
	d->pos[1] = d->a.y;
	d->pos[0] = (float)(dr_rand() % (d->a.rangeX * 2)) - (float)d->a.rangeX;
	d->pos[3] = 1.0f;
	d->age = 0;
	d->pos[2] = (float)(dr_rand() % (d->a.rangeZ * 2)) - (float)d->a.rangeZ;
	return 0;
}

/* 0x00124bc0  Drillers (gap; after CDrillers_Particle.cpp)
Creates the "rain" extra system (0x58-byte drops, `count` of them) in *slot. */
void DrRain_Create(struct BExtraSystem **slot, int count)
{
	*slot = baCreateExtraSystem("rain", (int (*)())FUN_00124b00,
		(int (*)())FUN_00124950, (int (*)())FUN_000a6fa0,
		(int (*)())FUN_001249c0, (int (*)())FUN_001d5950, 0x58, count, 1, 0);
}
