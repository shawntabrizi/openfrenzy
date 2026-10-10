/*
Demolition/CDemolition_Explosion.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Explosion.cpp

The explosion system of the Demolition game object (+0x13f0, vtable
0x0026ab94): a pool of 12 explosions (each four particle systems, a countdown
and a flash quad) and two light flashes (0x000ef910-0x000f0b20). 0x000efbc0
and 0x000f0b20 are tagged with this file (operator new lines 60 and 1138);
the functions between them are bracketed, the flash functions before them
are placed by their vtable (0x0026ab84, next to the explosion vtables).
The rest of CDemolition_Explosion.cpp (start, update, damage) is demolition_b's
(0x000f0bf0-).
*/

#include "demolition_a.h"

#define DE_FILE "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Explosion.cpp"

/* demolition_c's particle systems (names from demolition_c.decls) */
#define de_SparksCtor(ps) ((void *(*)(void *))FUN_00103360)(ps)            /* DcSparkSystem_Construct */
#define de_SparksDtor(ps) ((void (*)(void *))FUN_001025e0)(ps)             /* DcSparkSystem_Destruct */
#define de_SparksInit(ps, params, n) ((int (*)(void *, void *, int))FUN_00103390)((ps), (params), (n))
#define de_DebrisCtor(ps) ((void *(*)(void *))FUN_001028a0)(ps)            /* DcDebrisSystem_Construct */
#define de_DebrisDtor(ps) ((void (*)(void *))FUN_001028b0)(ps)             /* DcDebrisSystem_Destruct */
#define de_DebrisInit(ps, params, n) ((int (*)(void *, void *, int))FUN_001035a0)((ps), (params), (n))
#define de_EmitAll(ps) ((void (*)(void *))FUN_00102870)(ps)                /* DcParticles_EmitAll */
#define de_DebrisExplode(ps) ((void (*)(void *))FUN_00103f10)(ps)          /* DcDebrisSystem_Explode */

/* a particle system's virtual methods used here (vtable slot 0 and slot 3) */
#define de_PSSlot(ps, off) DA_VFN((ps), (off), void (*)(void *))(ps)

/* the textures of the game (game +0x6c50: vtable, then 20 textures) */
#define DE_TEXTURE(i) (((struct CDemolition_TextureSystem *)(DA_GAME + DG_TEXTURES))->textures[(i)])

OF_ASSERT_SIZE(struct CDemolition_Countdown, 0x38);
OF_ASSERT_SIZE(struct CDemolition_Flash, 0x60);
OF_ASSERT_OFFSET(struct CDemolition_Flash, active, 0x48);
OF_ASSERT_OFFSET(struct CDemolition_Flash, age, 0x5c);
OF_ASSERT_SIZE(struct CDemolition_Explosion, 0x790);
OF_ASSERT_OFFSET(struct CDemolition_Explosion, sparks, 0x2c);
OF_ASSERT_OFFSET(struct CDemolition_Explosion, debris, 0x170);
OF_ASSERT_OFFSET(struct CDemolition_Explosion, timer, 0x740);
OF_ASSERT_OFFSET(struct CDemolition_Explosion, source, 0x780);
OF_ASSERT_OFFSET(struct CDemolition_ExplosionSystem, flashes, 0x10);
OF_ASSERT_SIZE(struct DA_LineVertex, 0x24);

/* 0x0026ab84: a flash */
static const void *const vtable_0026ab84[] = {
	(const void *)CDemoFlash_DeletingDtor, /* [0] */
};

/* 0x0026ab90: an explosion */
static const void *const vtable_0026ab90[] = {
	(const void *)CDemoExplosion_DeletingDtor, /* [0] */
};

/* 0x0026ab94: the explosion system */
static const void *const vtable_0026ab94[] = {
	(const void *)CDemoExplosionSystem_DeletingDtor, /* [0] */
};

/* 0x002ff8a8: the flash quad (four vertices, a triangle list of type 4 with 4 vertices) */
static struct DA_LineVertex DAT_002ff8a8[4];

/* ---------- the flashes */

/* 0x000ef910  (file inferred from the vtable) */
void *CDemoFlash_Ctor(void *self)
{
	struct CDemolition_Flash *f = (struct CDemolition_Flash *)self;

	f->vtbl = vtable_0026ab84;
	CDemoCountdown_Ctor((unsigned int *)&f->timer);
	return self;
}

/* 0x000ef930  (file inferred from the vtable) */
void CDemoFlash_Dtor(void *self)
{
	struct CDemolition_Flash *f = (struct CDemolition_Flash *)self;

	f->vtbl = vtable_0026ab84;
	CDemoCountdown_Dtor((unsigned int *)&f->timer);
}

/* 0x000ef940  (file inferred from the vtable)
Update: while active, the age grows, the countdown runs, the light's range goes from 0.00025
to 0.000125 and its colour from (127, 32, 0) to black (T = countdown fraction); at the end the
flash stops and switches its light off. SELF is an int (as demolition_b calls it). */
void CDemoFlash_Update(int self, float dt)
{
	struct CDemolition_Flash *f = (struct CDemolition_Flash *)(uintptr_t)(unsigned int)self;
	float t;
	int g, r;

	if (!f->active)
		return;
	f->age = dt + f->age;
	DA_TimerUpdate(&f->timer);
	t = DA_TimerFraction(&f->timer);
	DA_LightSetRange(f->light, t * -0.000125f + 0.00025f);
	g = (int)(32.0f * (1.0f - t));
	r = (int)((1.0f - t) * 127.0f);
	DA_LightSetColour(f->light, r, g, 0);
	if (t >= 1.0f)
	{
		f->active = 0;
		f->light->vtbl->enable(f->light, 0);
	}
}

/* ---------- the explosions */

/* 0x000ef9e0  (file inferred: between the flash and the explosion functions) */
void *CDemoExplosion_Ctor(void *self)
{
	struct CDemolition_Explosion *e = (struct CDemolition_Explosion *)self;

	e->vtbl = vtable_0026ab90;
	de_SparksCtor(e->sparks);
	DA_SmokeCtor(e->smoke);
	DA_GlowCtor(e->glow);
	de_DebrisCtor(e->debris);
	CDemoCountdown_Ctor((unsigned int *)&e->timer);
	return self;
}

/* 0x000efa60  (file inferred) */
void CDemoExplosion_Dtor(void *self)
{
	struct CDemolition_Explosion *e = (struct CDemolition_Explosion *)self;

	e->vtbl = vtable_0026ab90;
	CDemoCountdown_Dtor((unsigned int *)&e->timer);
	de_DebrisDtor(e->debris);
	DA_GlowDtor(e->glow);
	DA_SmokeDtor(e->smoke);
	de_SparksDtor(e->sparks);
}

/* ---------- the explosion system */

/* 0x000efaf0  (file inferred)
Constructs the two flashes (the original uses the C++ vector constructor iterator). */
void *CDemoExplosionSystem_Ctor(void *self)
{
	struct CDemolition_ExplosionSystem *es = (struct CDemolition_ExplosionSystem *)self;
	int i;

	es->vtbl = vtable_0026ab94;
	for (i = 0; i < 2; i++)
		CDemoFlash_Ctor(&es->flashes[i]);
	return self;
}

/* 0x000efb20  (file inferred)
Destroys the two flashes, the last first (the vector destructor iterator 0x001def24). */
void CDemoExplosionSystem_Dtor(void *self)
{
	struct CDemolition_ExplosionSystem *es = (struct CDemolition_ExplosionSystem *)self;
	int i;

	es->vtbl = vtable_0026ab94;
	for (i = 2; i-- > 0;)
		CDemoFlash_Dtor(&es->flashes[i]);
}

/* 0x000efb40  (file inferred)
Shutdown: shuts down the four particle systems of each explosion (slot 0 of each) and deletes
the explosion (its virtual deleting destructor). The list head is not cleared. */
void CDemoExplosionSystem_Shutdown(void *self)
{
	struct CDemolition_ExplosionSystem *es = (struct CDemolition_ExplosionSystem *)self;
	struct CDemolition_Explosion *e, *next;

	for (e = es->first; e != NULL; e = next)
	{
		next = e->next;
		de_PSSlot(e->sparks, 0x0);
		de_PSSlot(e->smoke, 0x0);
		de_PSSlot(e->glow, 0x0);
		de_PSSlot(e->debris, 0x0);
		DA_VFN(e, 0x0, void *(*)(void *, unsigned int))(e, 1);
	}
}

/* 0x000efba0  (file inferred from the vtable) */
void *CDemoFlash_DeletingDtor(void *self, unsigned int flags)
{
	CDemoFlash_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000efbc0  Demolition/CDemolition_Explosion.cpp
Init: creates the flash's point light (line 60), initialises it and adds it to the light
system; inactive, age 0. Always succeeds (a failed allocation is not checked: the original
calls the light's Init through a NULL pointer). */
int CDemoFlash_Init(void *self)
{
	struct CDemolition_Flash *f = (struct CDemolition_Flash *)self;
	void *mem;

	mem = DA_New(0x34, DE_FILE, 0x3c);
	f->light = mem != NULL ? (struct CDemolition_Light *)CDemoPointLight_Ctor(mem) : NULL;
	f->light->vtbl->init(f->light);
	CDemoLightSystem_Add(DA_GAME + DG_LIGHTS, f->light);
	CDemoCountdown_Init((int)(uintptr_t)&f->timer);
	f->active = 0;
	f->age = 0.0f;
	return 1;
}

/* 0x000efc60  Demolition/CDemolition_Explosion.cpp (bracketed) */
void *CDemoExplosion_DeletingDtor(void *self, unsigned int flags)
{
	CDemoExplosion_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000efc80  Demolition/CDemolition_Explosion.cpp (bracketed)
Init: sparks (50, texture 1), smoke and glow (25 each, texture 2), debris (1), the countdown;
inactive; then slot 3 (+0xc, a reset) of each particle system. Always succeeds. */
int CDemoExplosion_Init(void *self)
{
	struct CDemolition_Explosion *e = (struct CDemolition_Explosion *)self;

	e->next = NULL;
	de_SparksInit(e->sparks, NULL, 0x32);
	DA_PSSetTexture(e->sparks, DE_TEXTURE(1));
	DA_SmokeInit(e->smoke, NULL, 0x19);
	DA_PSSetTexture(e->smoke, DE_TEXTURE(2));
	DA_GlowInit(e->glow, NULL, 0x19);
	DA_PSSetTexture(e->glow, DE_TEXTURE(2));
	de_DebrisInit(e->debris, NULL, 1);
	CDemoCountdown_Init((int)(uintptr_t)&e->timer);
	e->active = 0;
	e->visible = 0;
	e->age = 0.0f;
	de_PSSlot(e->sparks, 0xc);
	de_PSSlot(e->smoke, 0xc);
	de_PSSlot(e->glow, 0xc);
	de_PSSlot(e->debris, 0xc);
	return 1;
}

/* 0x000efd50  Demolition/CDemolition_Explosion.cpp (bracketed)
Draws the flash: four quads at the explosion, turned by 0, 90, 180 and 270 degrees about the
y axis, of size T * power (T = countdown fraction), with texture 6 and the alpha
(1 - T) * 128 in every colour channel. Render states 6 and 5 are pushed (1, 0) and restored;
state 10 is set to 1. SELF is an int (as demolition_b calls it). */
void CDemoExplosion_DrawFlash(int self)
{
	struct CDemolition_Explosion *e = (struct CDemolition_Explosion *)(uintptr_t)(unsigned int)self;
	struct DA_LineVertex *v = DAT_002ff8a8;
	float m[16];
	float t, size, angle, s, c;
	uint32_t colour;
	int a, i;

	t = DA_TimerFraction(&e->timer);
	size = t * e->power;
	a = (int)((1.0f - t) * 128.0f);
	DA_SetTexture(0, DE_TEXTURE(6));
	DA_PushRenderState(6, 1, 0);
	DA_PushRenderState(5, 0, 0);
	DA_SetRenderState(10, 1, 0);
	for (i = 0; i < 4; i++)
	{
		angle = (float)i * 90.0f * 0.0174532924f;
		v[0].x = 0.0f;
		v[0].y = 0.0f;
		v[0].z = 0.0f;
		v[1].y = 0.0f;
		v[3].y = 0.0f;
		v[2].y = 0.0f;
		v[0].u = 1.0f;
		v[0].v = 1.0f;
		v[1].u = 1.0f;
		v[1].v = 0.0f;
		v[2].u = 0.0f;
		v[2].v = 1.0f;
		v[3].u = 0.0f;
		v[3].v = 0.0f;
		s = sinf(angle) * size;
		v[1].x = s;
		colour = DA_RGBA(a, a, a, a);
		v[0].colour = colour;
		v[1].colour = colour;
		v[2].colour = colour;
		v[3].colour = colour;
		c = cosf(angle) * size;
		v[1].z = c;
		v[3].x = s + c;
		v[3].z = c - s;
		v[2].x = c;
		v[2].z = -s;
		DA_MatrixTranslation(m, e->pos.x, e->pos.y, e->pos.z);
		DA_SetWorldMatrix(m);
		DA_DrawPrimitive(4, DAT_002ff8a8, 4, 0x152);
	}
	DA_PopRenderState(6);
	DA_PopRenderState(5);
}

/* 0x000eff40  Demolition/CDemolition_Explosion.cpp (bracketed)
Starts the explosion at POS: SOURCE (itself when NULL, then +0x784 is cleared) and OWNER are
kept, the four particle systems move to POS and emit (the debris explodes), all are active,
the flash size is POWER and the countdown runs for TIME seconds. Called by demolition_b. */
void CDemoExplosion_Start(void *self, void *source, void *owner, float *pos, float time, float power)
{
	struct CDemolition_Explosion *e = (struct CDemolition_Explosion *)self;

	e->source = source;
	if (source == NULL)
	{
		e->unk784 = 0;
		e->source = e;
	}
	e->owner = owner;
	e->pos.x = pos[0];
	e->pos.y = pos[1];
	e->pos.z = pos[2];
	e->pos.w = pos[3];
	DA_PSSetPosition(e->sparks, pos);
	DA_PSSetPosition(e->smoke, pos);
	DA_PSSetPosition(e->glow, pos);
	DA_PSSetPosition(e->debris, pos);
	de_EmitAll(e->sparks);
	de_EmitAll(e->smoke);
	de_EmitAll(e->glow);
	de_DebrisExplode(e->debris);
	DA_FIELD(char, e, 0x70) = 1;  /* the sparks' active flag (+0x44) */
	DA_FIELD(char, e, 0xe0) = 1;  /* the smoke's (+0x3c) */
	DA_FIELD(char, e, 0x140) = 1; /* the glow's (+0x3c) */
	DA_FIELD(char, e, 0x1ac) = 1; /* the debris' (+0x3c) */
	e->active = 1;
	e->visible = 1;
	e->age = 0.0f;
	e->power = power;
	DA_TimerSet(&e->timer, time);
	DA_TimerStart(&e->timer);
}

/* 0x000f0b00  Demolition/CDemolition_Explosion.cpp (bracketed) */
void *CDemoExplosionSystem_DeletingDtor(void *self, unsigned int flags)
{
	CDemoExplosionSystem_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000f0b20  Demolition/CDemolition_Explosion.cpp
Init: creates 12 explosions (line 1138), each initialised and pushed at the head of the list
(a failed allocation is not checked), then initialises the two flashes. PACKAGE is not used.
Always succeeds. */
int CDemoExplosionSystem_Init(void *self, void *package)
{
	struct CDemolition_ExplosionSystem *es = (struct CDemolition_ExplosionSystem *)self;
	struct CDemolition_Explosion *e;
	void *mem;
	int i;

	(void)package;
	es->first = NULL;
	for (i = 0xc; i != 0; i--)
	{
		mem = DA_New(0x790, DE_FILE, 0x472);
		e = mem != NULL ? (struct CDemolition_Explosion *)CDemoExplosion_Ctor(mem) : NULL;
		CDemoExplosion_Init(e);
		e->next = es->first != NULL ? es->first : NULL;
		es->first = e;
	}
	for (i = 0; i < 2; i++)
		CDemoFlash_Init(&es->flashes[i]);
	return 1;
}
