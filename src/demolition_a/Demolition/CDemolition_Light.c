/*
Demolition/CDemolition_Light.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Light.cpp

The lights of the Demolition game: a light base class (vtable 0x0026a838),
point lights (0x0026a854), directional lights (0x0026a870) and the rotating
hazard lights of the crates (0x0026a88c: a spot light, three actors and two
glow sprites), and the light system of the game object (+0x6ca4, vtable
0x0026a8a8): the variation's main light, a list of lights by group, and up to
100 spark sprites drawn as point sprites.

The tagged functions are 0x000ed650-0x000ee330; the constructor before them
(0x000ed640) and the draw functions after them (0x000ee4a0 and the missed
0x000ee120) are this class's too.
*/

#include "demolition_a.h"
#include "xtl.h"

static const char CDemolition_Light_cpp[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Light.cpp";

/* a pure virtual slot (MSVC's _purecall at 0x001e0323 ends the program) */
static void dl_purecall(void)
{
	DA_Printf("R6025 pure virtual function call (CDemolition_Light)\n");
	abort();
}

/* rollerball_a's light helpers used as slots */
#define dl_PointLightEnable FUN_0018fb30
#define dl_DirLightInit FUN_0018fa40

/* 0x0026a838: the light base class */
static const void *const vtable_0026a838[] = {
	(const void *)CDemoLightBase_DeletingDtor, /* [0] +0x00 */
	(const void *)dl_purecall,                 /* [1] +0x04 init */
	(const void *)dl_purecall,                 /* [2] +0x08 shutdown */
	(const void *)dl_purecall,                 /* [3] +0x0c enable */
	(const void *)dl_purecall,                 /* [4] +0x10 update */
	(const void *)dl_purecall,                 /* [5] +0x14 draw */
	(const void *)FUN_001ec260,                /* [6] +0x18 set group (empty) */
};

/* 0x0026a854: a point light */
static const void *const vtable_0026a854[] = {
	(const void *)CDemoLight_DeletingDtor,     /* [0] +0x00 */
	(const void *)CDemoPointLight_Init,        /* [1] +0x04 */
	(const void *)CDemoLight_Shutdown,         /* [2] +0x08 */
	(const void *)dl_PointLightEnable,         /* [3] +0x0c (rollerball_a) */
	(const void *)CDemoPointLight_Update,      /* [4] +0x10 */
	(const void *)FUN_001d5950,                /* [5] +0x14 */
	(const void *)CDemoPointLight_SetGroup,    /* [6] +0x18 */
};

/* 0x0026a870: a directional light */
static const void *const vtable_0026a870[] = {
	(const void *)CDemoLight_DeletingDtor,     /* [0] +0x00 */
	(const void *)dl_DirLightInit,             /* [1] +0x04 (rollerball_a) */
	(const void *)CDemoLight_Shutdown,         /* [2] +0x08 */
	(const void *)CDemoDirLight_Enable,        /* [3] +0x0c */
	(const void *)FUN_001ec260,                /* [4] +0x10 */
	(const void *)FUN_001d5950,                /* [5] +0x14 */
	(const void *)FUN_001ec260,                /* [6] +0x18 */
};

/* 0x0026a88c: a hazard light */
static const void *const vtable_0026a88c[] = {
	(const void *)CDemoLight_DeletingDtor,     /* [0] +0x00 */
	(const void *)CDemoHazardLight_Init,       /* [1] +0x04 */
	(const void *)CDemoHazardLight_Shutdown,   /* [2] +0x08 */
	(const void *)CDemoHazardLight_Enable,     /* [3] +0x0c */
	(const void *)CDemoHazardLight_Update,     /* [4] +0x10 */
	(const void *)CDemoHazardLight_Draw,       /* [5] +0x14 */
	(const void *)FUN_001ec260,                /* [6] +0x18 */
};

/* 0x0026a8a8: the light system */
static const void *const vtable_0026a8a8[] = {
	(const void *)CDemoLightSystem_DeletingDtor, /* [0] */
};

#define DL_VCALL(light) (*(const struct CDemolition_LightVtbl *const *)(light))

/* ---------- point lights */

/* 0x000ed640  Demolition/CDemolition_Light.cpp (file inferred) */
void *CDemoPointLight_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a854;
	return self;
}

/* 0x000ed650  Demolition/CDemolition_Light.cpp
Slot 1 (Init): group 1, not linked, an engine light, off. */
int CDemoPointLight_Init(void *self)
{
	struct CDemolition_Light *l = (struct CDemolition_Light *)self;

	l->next = NULL;
	l->group = 1;
	l->light = DA_CreateLight(NULL, 0);
	l->on = 0;
	return 1;
}

/* 0x000ed680  Demolition/CDemolition_Light.cpp
Slot 4 (Update): when on and changed, the engine light gets the position, range and colour
and is switched on. */
void CDemoPointLight_Update(void *self, float dt)
{
	struct CDemolition_Light *l = (struct CDemolition_Light *)self;

	(void)dt;
	if (l->on == 0 || l->dirty == 0)
		return;
	DA_SetPointLight(l->light, &l->pos.x, l->range);
	DA_SetLightColour(l->light, l->r, l->g, l->b);
	DA_EnableLight(l->light, 1);
	l->dirty = 0;
}

/* 0x000ed6d0  Demolition/CDemolition_Light.cpp
Slot 6 (SetGroup): switches the engine light when the light is on. */
void CDemoPointLight_SetGroup(void *self, int on)
{
	struct CDemolition_Light *l = (struct CDemolition_Light *)self;

	if (l->on != 0)
		DA_EnableLight(l->light, on);
}

/* 0x000ed6f0  Demolition/CDemolition_Light.cpp
Slot 2 (Shutdown) of point and directional lights: deletes the engine light. */
void CDemoLight_Shutdown(void *self)
{
	DA_DeleteLight(((struct CDemolition_Light *)self)->light);
}

/* ---------- directional lights */

/* 0x000ed700  Demolition/CDemolition_Light.cpp
Slot 3 (Enable) of a directional light: when switched on, the direction (+0x34) and the
colour are set; then the engine light follows ON (only its low byte counts). */
void CDemoDirLight_Enable(void *self, int on)
{
	struct CDemolition_Light *l = (struct CDemolition_Light *)self;

	l->on = (char)on;
	if ((char)on != 0)
	{
		DA_SetDirectionalLight(l->light, (const float *)DA_AT(self, 0x34));
		DA_SetLightColour(l->light, l->r, l->g, l->b);
	}
	DA_EnableLight(l->light, (unsigned char)l->on);
}

/* ---------- hazard lights (0x548 bytes: the spot light inline at +0x34, three actor
   instances from +0xe0, the glow sprite at +0x530) */

/* 0x000ed750  Demolition/CDemolition_Light.cpp
Called by demolition_b's crate setup. */
void *CDemoHazardLight_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a88c;
	return self;
}

/* 0x000ed760  Demolition/CDemolition_Light.cpp
The light destructor (the base class's vtable). */
void CDemoLight_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a838;
}

/* 0x000ed770  Demolition/CDemolition_Light.cpp
Slot 2 (Shutdown): the spot light and the three actors. */
void CDemoHazardLight_Shutdown(void *self)
{
	int i;

	DA_DeleteLight(DA_AT(self, 0x34));
	for (i = 0; i < 3; i++)
		DA_DeleteActorInstance(DA_AT(self, 0xe0 + i * 0x170));
}

/* 0x000ed7b0  Demolition/CDemolition_Light.cpp
Slot 5 (Draw): the base and the lamp, the beam (unlit, half brightness), then the two glow
sprites over the lamp. */
void CDemoHazardLight_Draw(void *self)
{
	if (DA_FIELD(char, self, 0x10) == 0)
		return;
	DA_DrawActorInstance(DA_AT(self, 0xe0), 7);
	DA_DrawActorInstance(DA_AT(self, 0x250), 7);
	DA_PushRenderState(6, 1, 0);
	DA_SetRenderState(10, 0, 0);
	DA_SetBrightness(0x7f);
	DA_DrawActorInstance(DA_AT(self, 0x3c0), 7);
	DA_SetBrightness(0x100);
	DA_SetRenderState(10, 1, 0);
	DA_SetWorldIdentity();
	DA_PushRenderState(5, 0, 0);
	DA_DrawSprite3D((float *)DA_AT(self, 0x530), 250.0f, 150.0f, 0.0f, DA_FIELD(void *, self, 0x540), 0, 0,
		0x7f, 0x7f, 0x7f, 0x7f, NULL, -0.001f);
	DA_DrawSprite3D((float *)DA_AT(self, 0x530), 450.0f, 350.0f, 0.0f, DA_FIELD(void *, self, 0x544), 0, 0,
		0x7f, 0x60, 0, 0x7f, NULL, -0.001f);
	DA_PopRenderState(5);
	DA_PopRenderState(6);
}

/* 0x000ed8b0  Demolition/CDemolition_Light.cpp
Slot 3 (Enable): when switched on, the spot light gets its position, direction and cone; then
it follows ON. */
void CDemoHazardLight_Enable(void *self, int on)
{
	struct CDemolition_Light *l = (struct CDemolition_Light *)self;

	l->on = (char)on;
	l->pos.w = 1.0f;
	if ((char)on != 0)
	{
		DA_FIELD(float, self, 0xb0) = 1.0f;
		DA_SetSpotLight(DA_AT(self, 0x34), &l->pos.x, (const float *)DA_AT(self, 0xa4), DA_FIELD(float, self, 0xb4),
			DA_FIELD(float, self, 0xb8));
		DA_SetLightColour(DA_AT(self, 0x34), l->r, l->g, l->b);
	}
	DA_EnableLight(DA_AT(self, 0x34), (unsigned char)l->on);
}

/* ---------- the light system: lights */

/* 0x000ed920  Demolition/CDemolition_Light.cpp */
void *CDemoLightSystem_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a8a8;
	return self;
}

/* 0x000ed930  Demolition/CDemolition_Light.cpp */
void CDemoLightSystem_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a8a8;
}

/* 0x000ed940  Demolition/CDemolition_Light.cpp
Adds LIGHT at the head of the list. Called by demolition_b. */
void CDemoLightSystem_Add(void *self, void *light)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	struct CDemolition_Light *l = (struct CDemolition_Light *)light;

	if (ls->first == NULL)
	{
		l->next = NULL;
		ls->first = l;
		return;
	}
	l->next = ls->first;
	ls->first = l;
}

/* 0x000ed970  Demolition/CDemolition_Light.cpp
Switches every light of GROUP on or off (slot 3). Called by demolition_b, c and d. */
void CDemoLightSystem_EnableGroup(int self, int on, int group)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)(uintptr_t)self;
	struct CDemolition_Light *l;

	for (l = ls->first; l != NULL; l = l->next)
	{
		if (l->group == group)
			DL_VCALL(l)->enable(l, on);
	}
}

/* 0x000ed9a0  Demolition/CDemolition_Light.cpp
Calls slot 6 with ON for every light of GROUP. Called by demolition_d. */
void CDemoLightSystem_SetGroup(void *self, int on, int group)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	struct CDemolition_Light *l;

	for (l = ls->first; l != NULL; l = l->next)
	{
		if (l->group == group)
			DL_VCALL(l)->setGroup(l, on);
	}
}

/* ---------- the light system: sparks */

/* 0x000ed9d0  Demolition/CDemolition_Light.cpp
The sparks rise with the variation's lift (remote: speed at +0x28 of the variation, cross: at
+0x68); changed positions or colours are written into both vertex buffers (16 bytes per
vertex: position, colour). Cross uses alpha 0x40 for both; the others 0x7f and 0x20. */
void CDemoLightSystem_UpdateSparks(void *self, float dt)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	uint32_t *a, *b;
	uint32_t colour;
	int variation, i;

	if (ls->sparkCount < 1)
		return;
	variation = DA_FIELD(int, DA_GAME, DG_VARIATION);
	if (variation == 1)
		ls->spin = DA_FIELD(float, DA_VARIATION(), 0x28) * dt + ls->spin;
	else if (variation == 3)
		ls->spin = DA_FIELD(float, DA_VARIATION(), 0x68) * dt + ls->spin;
	if (ls->posDirty == 0 && ls->colourDirty == 0)
		return;
	a = (uint32_t *)DA_VBLock(ls->vbA, 0);
	if (a == NULL)
		return;
	b = (uint32_t *)DA_VBLock(ls->vbB, 0);
	if (b == NULL)
		return;
	for (i = 0; i < ls->sparkCount; i++, a += 4, b += 4)
	{
		struct DA_Spark *s = &ls->sparks[i];

		if (ls->posDirty != 0)
		{
			memcpy(&a[0], &s->pos.x, 12);
			memcpy(&b[0], &s->pos.x, 12);
		}
		if (ls->colourDirty != 0)
		{
			colour = DAT_002bd640[s->b] | DAT_002bc490[s->g] | DAT_002bce18[s->r];
			if (DA_FIELD(int, DA_GAME, DG_VARIATION) == 3)
			{
				a[3] = colour | DAT_002bc898[0x40];
				b[3] = colour | DAT_002bc898[0x40];
			}
			else
			{
				a[3] = colour | DAT_002bc898[0x7f];
				b[3] = colour | DAT_002bc898[0x20];
			}
		}
	}
	ls->posDirty = 0;
	ls->colourDirty = 0;
	DA_VBUnlock(ls->vbA);
	DA_VBUnlock(ls->vbB);
}

/* 0x000edba0  Demolition/CDemolition_Light.cpp
The base class's scalar deleting destructor. */
void *CDemoLightBase_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026a838;
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000edbc0  Demolition/CDemolition_Light.cpp
Sets a light's position (w forced to 1) and marks it changed. Called by demolition_b. */
void CDemoLight_SetPosition(int self, unsigned int *pos)
{
	struct CDemolition_Light *l = (struct CDemolition_Light *)(uintptr_t)self;

	memcpy(&l->pos, pos, 16);
	l->pos.w = 1.0f;
	l->dirty = 1;
}

/* 0x000edbf0  Demolition/CDemolition_Light.cpp
The derived lights' scalar deleting destructor. */
void *CDemoLight_DeletingDtor(void *self, unsigned int flags)
{
	CDemoLight_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000edc10  Demolition/CDemolition_Light.cpp
Slot 1 (Init) of a hazard light: group 4, on; the three actors from the light system's
models; orange (0x7f, 0x20, 0); turning at 0.1047 rad per frame, starting at pi when another
light of group 4 exists; the spot light (cone 0.000167 .. 1.047, reach 6000) and the two
corona textures. */
int CDemoHazardLight_Init(void *self)
{
	struct CDemolition_Light *l = (struct CDemolition_Light *)self;
	struct CDemolition_Light *other;
	int found = 0;
	int i;

	l->group = 4;
	l->next = NULL;
	l->on = 1;
	for (i = 0; i < 3; i++)
		DA_InitActorInstance(DA_AT(self, 0xe0 + i * 0x170), DA_FIELD(void *, DA_GAME, DG_LIGHTS + 4 + i * 4), 1,
			CDemolition_Light_cpp, 0x230);
	l->r = 0x7f;
	l->g = 0x20;
	l->b = 0;
	DA_FIELD(float, self, 0xd4) = 0.104719758f;
	other = DA_FIELD(struct CDemolition_Light *, DA_GAME, DG_LIGHTS + 0x10);
	if (other != NULL)
	{
		for (; other != NULL; other = other->next)
		{
			if (other->group == 4)
				found = 1;
		}
	}
	if (found)
	{
		DA_FIELD(float, self, 0xc4) = 3.14159274f;
		DA_Printf("Here1\n");
	}
	else
	{
		DA_FIELD(float, self, 0xc4) = 0.0f;
		DA_Printf("Here2\n");
	}
	DA_CreateLight(DA_AT(self, 0x34), 0);
	DA_FIELD(float, self, 0xb8) = 1.04719758f;
	DA_FIELD(float, self, 0xd0) = 6000.0f;
	DA_FIELD(float, self, 0xb4) = 0.000166666665f;
	DA_FIELD(void *, self, 0x540) = DA_FIELD(void *, DA_GAME, DG_TEXTURES + 4 + 4 * 4);
	DA_FIELD(void *, self, 0x544) = DA_FIELD(void *, DA_GAME, DG_TEXTURES + 4 + 5 * 4);
	return 1;
}

/* 0x000edd30  Demolition/CDemolition_Light.cpp
Slot 4 (Update): the beam turns (angle +0xc4, wrapped), the spot light points at the beam's
end (length +0xd0) and is lit; a moved light moves its actors and glow (40 higher); the lamp
actor turns with the beam. */
void CDemoHazardLight_Update(void *self, float dt)
{
	struct CDemolition_Light *l = (struct CDemolition_Light *)self;
	float *dir = (float *)DA_AT(self, 0xa4);
	float reach[4];
	float axisAngle[4];
	float angle;

	if (l->on == 0)
		return;
	angle = dt * DA_FIELD(float, self, 0xd4) + DA_FIELD(float, self, 0xc4);
	DA_FIELD(float, self, 0xc4) = angle;
	angle = DA_WrapAngle(angle);
	DA_FIELD(float, self, 0xc4) = angle;
	DA_FIELD(float, self, 0xbc) = angle;
	reach[0] = 0.0f;
	reach[1] = 0.0f;
	reach[2] = DA_FIELD(float, self, 0xd0);
	reach[3] = 0.0f; /* not set by the original */
	DA_RotateY(dir, reach, angle);
	dir[0] = l->pos.x + dir[0];
	dir[1] = dir[1] + l->pos.y;
	dir[2] = dir[2] + l->pos.z;
	l->pos.w = 1.0f;
	dir[3] = 1.0f;
	DA_SetSpotLight(DA_AT(self, 0x34), &l->pos.x, dir, DA_FIELD(float, self, 0xb4), DA_FIELD(float, self, 0xb8));
	DA_SetLightColour(DA_AT(self, 0x34), l->r, l->g, l->b);
	DA_EnableLight(DA_AT(self, 0x34), 1);
	if (l->dirty != 0)
	{
		memcpy(DA_AT(self, 0x1a0), &l->pos, 16);
		DA_PrepActorInstance(DA_AT(self, 0xe0), 1);
		memcpy(DA_AT(self, 0x480), &l->pos, 16);
		DA_PrepActorInstance(DA_AT(self, 0x3c0), 1);
		memcpy(DA_AT(self, 0x310), &l->pos, 16);
		memcpy(DA_AT(self, 0x530), &l->pos, 16);
		DA_FIELD(float, self, 0x534) = DA_FIELD(float, self, 0x534) + 40.0f;
	}
	axisAngle[0] = 0.0f;
	axisAngle[1] = 1.0f;
	axisAngle[2] = 0.0f;
	axisAngle[3] = DA_FIELD(float, self, 0xc4);
	DA_QuatFromAxisAngle((float *)DA_AT(self, 0x330), axisAngle);
	memcpy(DA_AT(self, 0x4a0), DA_AT(self, 0x330), 16);
	DA_PrepActorInstance(DA_AT(self, 0x250), 1);
	DA_PrepActorInstance(DA_AT(self, 0x3c0), 1);
}

/* 0x000edf50  Demolition/CDemolition_Light.cpp */
void *CDemoLightSystem_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026a8a8;
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000edf70  Demolition/CDemolition_Light.cpp
Shutdown: every light shuts down and is deleted; the models, the main light and the spark
vertex buffers go. */
void CDemoLightSystem_Shutdown(void *self)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	struct CDemolition_Light *l, *next;
	int i;

	l = ls->first;
	while (l != NULL)
	{
		next = l->next;
		DL_VCALL(l)->shutdown(l);
		DL_VCALL(l)->destroy(l, 1);
		l = next;
	}
	for (i = 0; i < 3; i++)
	{
		if (ls->models[i] != NULL)
		{
			DA_DeleteResource(ls->models[i]);
			ls->models[i] = NULL;
		}
	}
	DL_VCALL(ls->main)->shutdown(ls->main);
	if (ls->main != NULL)
		DL_VCALL(ls->main)->destroy(ls->main, 1);
	DA_VBDestroy(ls->vbA);
	DA_VBDestroy(ls->vbB);
}

/* 0x000edff0  Demolition/CDemolition_Light.cpp
Update: the main light, every light, then the sparks. */
void CDemoLightSystem_Update(void *self, float dt)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	struct CDemolition_Light *l;

	DL_VCALL(ls->main)->update(ls->main, dt);
	for (l = ls->first; l != NULL; l = l->next)
		DL_VCALL(l)->update(l, dt);
	CDemoLightSystem_UpdateSparks(self, dt);
}

/* 0x000ee030  Demolition/CDemolition_Light.cpp
The spark vertex buffers (100 vertices, FVF 0x42: position and colour); no sparks yet, both
buffers to fill; cross starts its sparks 1230 below. */
void CDemoLightSystem_InitSparks(void *self)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;

	ls->sparkCount = 0;
	DA_VBCreate(&ls->vbA, 100, 0x42, 0, 1);
	DA_VBCreate(&ls->vbB, 100, 0x42, 0, 1);
	ls->sparkCount = 0;
	ls->posDirty = 1;
	ls->colourDirty = 1;
	ls->spin = 0.0f;
	if (DA_FIELD(int, DA_GAME, DG_VARIATION) == 3)
		ls->spin = -1230.0f;
}

/* 0x000ee0b0  Demolition/CDemolition_Light.cpp
Adds a spark at POS with colour R, G, B (at most 100). Called by demolition_b. */
void CDemoLightSystem_AddSpark(void *self, float *pos, int r, int g, int b)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	struct DA_Spark *s;

	if (ls->sparkCount >= 100)
		return;
	s = &ls->sparks[ls->sparkCount];
	memcpy(&s->pos, pos, 16);
	ls->sparks[ls->sparkCount].r = r;
	ls->sparks[ls->sparkCount].g = g;
	ls->sparks[ls->sparkCount].b = b;
	ls->sparkCount++;
}

/* 0x000ee120  Demolition/CDemolition_Light.cpp (missing from the function table; tail-called
by 0x000ee4a0)
Draws the sparks as two layers of point sprites (corona01 and corona02 textures, additive
alpha) lifted with the variation (remote: by the spark offset; cross: by the lift height
plus the offset, not at all above 10000; others: in world space). */
void FUN_000ee120(void *self)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	float matrix[16];
	float lift;
	void *corona1, *corona2;
	int variation;

	if (ls->sparkCount < 1)
		return;
	variation = DA_FIELD(int, DA_GAME, DG_VARIATION);
	if (variation == 1)
	{
		lift = ls->spin;
	}
	else if (variation == 3)
	{
		if (!(ls->spin <= 10000.0f))
			return;
		lift = DA_FIELD(float, DA_VARIATION(), 0x16b0) + ls->spin;
	}
	else
	{
		DA_SetWorldIdentity();
		goto draw;
	}
	DA_MatrixTranslation(matrix, 0.0f, lift, 0.0f);
	DA_SetWorldMatrix(matrix);
draw:
	corona2 = DA_FIELD(void *, DA_GAME, DG_TEXTURES + 4 + 5 * 4);
	corona1 = DA_FIELD(void *, DA_GAME, DG_TEXTURES + 4 + 4 * 4);
	DA_SetProjectionMode(1);
	DA_PushRenderState(6, 1, 0);
	DA_PushRenderState(5, 0, 0);
	DA_SetRenderState(10, 1, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)2, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)3, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)4, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)6, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 5);
	DA_SetTexture(0, NULL);
	DA_SetTexture(1, NULL);
	DA_SetTexture(2, NULL);
	DA_SetTexture(3, corona1);
	DA_SetDepthOffset(-0.5f);
	if (DA_FIELD(int, DA_GAME, DG_VARIATION) == 3)
		DA_DrawPointSprites(ls->vbA, 0, ls->sparkCount, 150.0f);
	else
		DA_DrawPointSprites(ls->vbA, 0, ls->sparkCount, 400.0f);
	DA_SetTexture(3, corona2);
	if (DA_FIELD(int, DA_GAME, DG_VARIATION) == 3)
		DA_DrawPointSprites(ls->vbB, 0, ls->sparkCount, 5000.0f);
	else
		DA_DrawPointSprites(ls->vbB, 0, ls->sparkCount, 4000.0f);
	DA_SetDepthOffset(0.0f);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 1);
	DA_SetProjectionMode(1);
	DA_PopRenderState(6);
	DA_PopRenderState(5);
}

/* 0x000ee330  Demolition/CDemolition_Light.cpp
Init: the three hazard base models from PACKAGE (0 when one is missing); the main light of
the variation (frag/remote/mine: a directional light along DAT_002977e0; cross: a point light
at (0, 1000, 0) with range 0.0004), grey (0x60, 0x60, 0x60) and on; the spark buffers. */
int CDemoLightSystem_Init(void *self, void *package)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	struct CDemolition_Light *main;
	void *memory;
	int variation, i;

	ls->first = NULL;
	for (i = 0; i < 3; i++)
	{
		ls->models[i] = DA_LoadResource(package, DAT_002977d0[i], 1, 0xdefa);
		if (ls->models[i] == NULL)
			return 0;
	}
	variation = DA_FIELD(int, DA_GAME, DG_VARIATION);
	if (variation >= 0)
	{
		if (variation < 3)
		{
			memory = DA_New(0x44, CDemolition_Light_cpp, 0x37c);
			if (memory != NULL)
				*(const void *const **)memory = vtable_0026a870;
			main = (struct CDemolition_Light *)memory;
			ls->main = main;
			DL_VCALL(main)->init(main);
			memcpy(DA_AT(main, 0x34), &DAT_002977e0[DA_FIELD(int, DA_GAME, DG_VARIATION)], 16);
			main->dirty = 1;
		}
		else if (variation == 3)
		{
			memory = DA_New(0x34, CDemolition_Light_cpp, 0x386);
			if (memory != NULL)
				*(const void *const **)memory = vtable_0026a854;
			main = (struct CDemolition_Light *)memory;
			ls->main = main;
			DL_VCALL(main)->init(main);
			memcpy(&main->pos, &DAT_002977e0[DA_FIELD(int, DA_GAME, DG_VARIATION)], 16);
			main->pos.w = 1.0f;
			main->range = 0.0004f;
			main->dirty = 1;
		}
	}
	main = ls->main;
	main->r = 0x60;
	main->g = 0x60;
	main->b = 0x60;
	main->dirty = 1;
	DL_VCALL(ls->main)->enable(ls->main, 1);
	CDemoLightSystem_InitSparks(self);
	return 1;
}

/* 0x000ee4a0  Demolition/CDemolition_Light.cpp (file inferred)
Draw: every light (slot 5), then the sparks. */
void CDemoLightSystem_Draw(void *self)
{
	struct CDemolition_LightSystem *ls = (struct CDemolition_LightSystem *)self;
	struct CDemolition_Light *l;

	for (l = ls->first; l != NULL; l = l->next)
		DL_VCALL(l)->draw(l);
	FUN_000ee120(self);
}
