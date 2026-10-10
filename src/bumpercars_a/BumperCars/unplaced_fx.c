/*
BumperCars/unplaced_fx.c  (module bumpercars_a)

The BumperCars debris effects (gap functions 0x0008cb60-0x0008d030; source file unknown):
the object at CBCar_Game + 0x5c (its methods ignore `this`), the BumperCars debris types
(0x00291e88, 0x2c bytes each: "STEAM", ...) with their sprites (0x00291810), the ground test
the debris system uses (0x0008cc10, missing from the table) and bursts of debris.
*/

#include "bumpercars_a.h"

/* 0x00291810  the five BumperCars debris sprite definitions; 0x00291e88 the debris types
(BumperCars/bca_data.c) */
extern unsigned char DAT_00291810[];
extern struct DebrisType DAT_00291e88[];

struct CSpriteDef;
struct CSpriteVB;
void Sprite_LoadDefs(struct CSpriteDef *defs, int count, int package);
void Sprite_FreeDefs(struct CSpriteDef *defs, int count);
void SpriteVB_ForceBufferSize(struct CSpriteVB *self, int size);

/* 0x0008cb60  (gap): shuts the effects down (the sprites and the debris systems) */
void CBCar_Effects_Shutdown(void *self)
{
	(void)self;
	Sprite_FreeDefs((struct CSpriteDef *)DAT_00291810, 5);
	CLDebris_Start(); /* classlib_ui's name for 0x00080bb0: deletes the debris system */
}

/* 0x0008cb80  (gap): updates the debris */
void CBCar_Effects_Update(void *self, float dt)
{
	(void)self;
	Debris_UpdateSystems(dt);
}

/* 0x0008cb90  (gap): draws the debris (view 0 draws nothing) */
void CBCar_Effects_Draw(void *self, int view)
{
	(void)self;
	if (view != 0)
		CLDebris_Stop(); /* classlib_ui's name for 0x00080cc0: draws the debris */
}

/* 0x0008cba0  (gap): adds one piece of BumperCars debris type `type` */
void CBCar_Effects_Add(void *self, int type, float *pos, float *vel, int lifeFrames, unsigned int colour)
{
	(void)self;
	Debris_Add(&DAT_00291e88[type], pos, vel, lifeFrames, colour);
}

/* 0x0008cbd0  (gap): a ring of `count` pieces of type `type` */
void CBCar_Effects_AddRing(void *self, int type, float *pos, float outSpeed, float upSpeed, float jitter, int count)
{
	(void)self;
	if (count != 0)
		Debris_AddRing(&DAT_00291e88[type], count, pos, outSpeed, upSpeed, jitter, 0, 0xffffffffu);
}

/* 0x0008cc10  (missing from the table): the debris ground test: the nearest of up to 30 hits
of the move against the world's collision mesh (game + 0x78). Writes the hit point and the
normal (w = 1); returns the hit fraction (0.001 for a hit at 0), or 0. */
float FUN_0008cc10(float *pos, float *delta, float *point, float *normal)
{
	struct BRay ray;
	struct BCollHit hits[30];
	int count, i;
	float t;

	if (sqrtf(delta[2] * delta[2] + delta[1] * delta[1] + delta[0] * delta[0]) == 0.0f)
		return 0.0f;
	DAT_00285644 = 0.01f;
	ray.origin[0] = pos[0];
	ray.origin[1] = pos[1];
	ray.origin[2] = pos[2];
	ray.dir[1] = delta[1];
	ray.dir[0] = delta[0];
	ray.dir[2] = delta[2];
	ray.length = sqrtf(delta[2] * delta[2] + delta[1] * delta[1] + delta[0] * delta[0]);
	count = 30;
	hits[0].flags = BCOLL_T | BCOLL_NORMAL | BCOLL_POINT;
	ray.origin[3] = 1.0f;
	ray.dir[3] = 1.0f;
	if (bmRayMesh(&ray, BCG(0x78, struct BCollMesh *), &count, hits) == 0 || count == 0)
		return 0.0f;
	t = hits[0].t;
	memcpy(normal, hits[0].normal, 16);
	memcpy(point, hits[0].point, 16);
	for (i = 1; i < count; i++) {
		if (hits[i].t < t) {
			t = hits[i].t;
			memcpy(normal, hits[i].normal, 16);
			memcpy(point, hits[i].point, 16);
		}
	}
	normal[3] = 1.0f;
	if (t == 0.0f)
		t = 0.001f;
	return t;
}

/* 0x0008cdc0  (gap): starts the effects: the sprites, the debris system with 3000 pieces
on the ground test, a 12000-vertex buffer and the wind */
int CBCar_Effects_Init(void *self)
{
	float wind[4];

	(void)self;
	Sprite_LoadDefs((struct CSpriteDef *)DAT_00291810, 5, 0);
	Debris_Init((void *)FUN_0008cc10, 3000);
	SpriteVB_ForceBufferSize((struct CSpriteVB *)(void *)DAT_00290ea8, 12000);
	wind[0] = -12.0f;
	wind[1] = -10.0f;
	wind[2] = 5.0f;
	wind[3] = 1.0f;
	DebrisSys_SetWindDirection((struct DebrisSysData *)DAT_002ed34c->userData, wind);
	return 1;
}

/* 0x0008ce30  (gap): a ring of `count` pieces with a random type out of the `ntypes` type
indices that follow (at most 10). A variadic member function: `self` is on the stack. */
void CBCar_Effects_Burst(void *self, float *pos, float outSpeed, float upSpeed, float jitter, int count, int ntypes, ...)
{
	int types[10];
	float local[4], vel[4], step, angle, half;
	va_list ap;
	int i, n;

	(void)self;
	if (count == 0 || ntypes == 0)
		return;
	if (ntypes > 10)
		ntypes = 10;
	va_start(ap, ntypes);
	for (i = 0; i < ntypes; i++)
		types[i] = va_arg(ap, int);
	va_end(ap);
	step = 0x1.921fb6p+2f / (float)count;
	local[1] = upSpeed;
	local[0] = 0.0f;
	local[2] = outSpeed;
	local[3] = 1.0f;
	angle = (float)bca_rand() * 0x1.9222d8p-14f;
	for (n = count; n > 0; n--) {
		bmVectorRotateY(vel, local, angle);
		if (jitter > 0.0f) {
			half = jitter * 0.5f;
			vel[0] = ((float)bca_rand() * jitter * (1.0f / 32767.0f) - half) + vel[0];
			vel[1] = ((float)bca_rand() * jitter * (1.0f / 32767.0f) - half) + vel[1];
			vel[2] = ((float)bca_rand() * jitter * (1.0f / 32767.0f) - half) + vel[2];
		}
		Debris_Add(&DAT_00291e88[types[bca_rand() % ntypes]], pos, vel, 0, 0xffffffffu);
		angle = CLMath_WrapAngle(angle + step);
	}
}

/* 0x0008d030  (gap): `count` pieces of type `type` on a circle of `radius` around pos (at a
random height up to heightRandom), each moving up at speedUp (the circle starts at a random
angle; each angle gets a random jitter that cancels out) */
void CBCar_Effects_Circle(int type, float *pos, float speedUp, float heightRandom, float angleRandom, float radius,
	int count, int lifeFrames)
{
	float step, angle, p[4], vel[4], v;
	double a, j;
	int n;

	if (count == 0)
		return;
	step = 0x1.921fb6p+2f / (float)count;
	angle = (float)bca_rand() * step * (1.0f / 32767.0f);
	if (count <= 0)
		return;
	for (n = count; n > 0; n--) {
		bca_rand();
		j = (double)bca_rand() * (double)angleRandom * (double)(1.0f / 32767.0f);
		a = ((double)angle + j) - j;
		vel[3] = 1.0f;
		vel[0] = 0.0f;
		vel[2] = 0.0f;
		p[3] = 1.0f;
		p[0] = (float)(sin(a) * (double)radius + (double)pos[0]);
		p[1] = (float)bca_rand() * (1.0f / 32767.0f) * heightRandom + pos[1];
		p[2] = (float)(cos(a) * (double)radius + (double)pos[2]);
		bca_rand();
		v = (float)bca_rand() * heightRandom * (1.0f / 32767.0f);
		vel[1] = (speedUp + v) - v;
		Debris_Add(&DAT_00291e88[type], p, vel, lifeFrames, 0xffffffffu);
		angle = angle + step;
	}
}
