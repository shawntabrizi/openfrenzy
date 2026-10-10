/*
CharLib/lights.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\lights.cpp (tags at 0x000b72d0, 0x000b7380,
0x000b7490 and 0x000b7570, source lines 695-787).

CharLib lights: a ClassLib list (world object + 0x3c) of lights that wrap a bactor light
(at +0x08) and animate it: directional, spot, point, rotating, fading, and point lights
attached to a node of a platform. At most 5 bactor lights exist at a time.

The light methods at 0x000b6910-0x000b7270 come before the tagged factory functions and
the update and delete functions after them; all are placed here (inferred).
*/

#include "charlib.h"

/* ---------- the light (0xe8 bytes) */

#define LT_BLIGHT 0x08             /* the bactor light (BLight) */
#define LT_COLOUR 0x1c             /* int r, g, b: the colour in the bactor light */
#define LT_TYPE 0x78               /* 0 directional, 1 spot, 2 point, 3 fading point,
                                      4 attached point, 5 turning directional,
                                      6 turning fading directional */
#define LT_POS 0x7c                /* float[4] */
#define LT_ATTACH 0x8c             /* platform the light follows (its use count at +0x45c) */
#define LT_NODE 0x90               /* node of the platform (NULL: its position) */
#define LT_DIR 0x94                /* float[4] */
#define LT_ANGLE 0xa4              /* float: turn so far */
#define LT_TURN_SPEED 0xa8         /* float: turn per frame */
#define LT_CREATED 0xac            /* int: the bactor light exists */
#define LT_FROM 0xb0               /* int[3]: colour at the start of a fade */
#define LT_TO 0xbc                 /* int[3]: colour at the end of a fade */
#define LT_RANGE 0xc8              /* float: range at the start of a fade */
#define LT_RANGE_TO 0xcc           /* float: range at the end */
#define LT_DURATION 0xd0           /* float: frames of the fade */
#define LT_TIME 0xd4               /* float: frames faded so far */
#define LT_DELAY 0xd8              /* float: frames before the fade starts */
#define LT_ON 0xdc                 /* int: the bactor light is switched on */
#define LT_LIMITED 0xe0            /* int: counts against the limit of Lights_Limit */
#define LT_VISIBLE 0xe4            /* int */

/* 0x002f6b5c  only one limited light may be on (1) */
#define lights_limit_mode CHL_VAR(int, 0x002f6b5c)
/* 0x002f6b60  bactor lights created */
#define lights_count CHL_VAR(int, 0x002f6b60)
/* 0x002f6b64  lights switched on (only ever decremented here) */
#define lights_on_count CHL_VAR(int, 0x002f6b64)
/* 0x002f6b68 / 6c / 70  the saved ambient colour (b, g, r) */
#define lights_saved_b CHL_VAR(int, 0x002f6b68)
#define lights_saved_g CHL_VAR(int, 0x002f6b6c)
#define lights_saved_r CHL_VAR(int, 0x002f6b70)

/* 0x00284b10  the ambient colour r, g, b (bactor .data) */
extern int DAT_00284b10;
extern int DAT_00284b14;
extern int DAT_00284b18;

/* the light list: the CharLib world object (chl_actor_list) + 0x3c */
#define lights_list ((void *)((char *)chl_actor_list + 0x3c))

/* bactor light functions (bactor.h): baCreateLight(light, flags), baEnableLight(light, on),
baSetAmbientLight(r, g, b), baSetLightColour(light, r, g, b), baDeleteLight(light),
baSetDirectionalLight(light, dir), baSetPointLight(light, pos, range),
baSetSpotLight(light, pos, target, range, angle) */
#define lt_Create ((void *(*)(void *, unsigned int))FUN_000382f0)
#define lt_Enable ((void (*)(void *, int))FUN_00038370)
#define lt_SetAmbient ((void (*)(int, int, int))FUN_00038440)
#define lt_SetColour ((void (*)(void *, int, int, int))FUN_00038470)
#define lt_Delete ((void (*)(void *))FUN_000384d0)
#define lt_SetDirectional ((void (*)(void *, float *))FUN_00038550)
#define lt_SetPoint ((void (*)(void *, float *, float))FUN_000385a0)
#define lt_SetSpot ((void (*)(void *, float *, float *, float, float))FUN_000385e0)
/* core_modes_b 0x0001e9d0: out = a + b (vectors) */
#define lt_VecAdd ((void (*)(float *, float *, float *))FUN_0001e9d0)

/* the common start of the constructors: the bactor light, if fewer than 5 exist */
static void light_create(void *light)
{
	CHL_I32(light, LT_CREATED) = 0;
	if (lights_count < 5 && lt_Create(CHL_FIELD(void, light, LT_BLIGHT), 0) != NULL) {
		lights_count = lights_count + 1;
		CHL_I32(light, LT_CREATED) = 1;
	}
}

/* 0x000b6910  CharLib (lights.cpp, inferred)
A directional light. */
void *Light_InitDirectional(void *light, unsigned char r, unsigned char g, unsigned char b, float *dir)
{
	CHL_I32(light, LT_VISIBLE) = 1;
	light_create(light);
	CHL_I32(light, LT_TYPE) = 0;
	CHL_PTR(light, LT_ATTACH) = NULL;
	CHL_F32(light, LT_DELAY) = 0.0f;
	dir[3] = 1.0f;
	CHL_U32(light, LT_COLOUR + 0x0) = r;
	CHL_U32(light, LT_COLOUR + 0x8) = b;
	CHL_U32(light, LT_COLOUR + 0x4) = g;
	if (CHL_I32(light, LT_CREATED) != 0)
		lt_SetDirectional(CHL_FIELD(void, light, LT_BLIGHT), dir);
	chl_ListInsert(lights_list, light, 0);
	CHL_I32(light, LT_LIMITED) = 0;
	CHL_I32(light, LT_ON) = 1;
	return light;
}

/* 0x000b69d0  CharLib (lights.cpp, inferred)
Switches the lights on or off (with restore_ambient, the limited lights are left alone,
and the ambient light goes to full grey while they are off, then back). */
void Lights_EnableAll(int enable, int restore_ambient)
{
	void *light;

	if (restore_ambient != 0 && enable == 0) {
		lights_saved_r = DAT_00284b10;
		lights_saved_g = DAT_00284b14;
		lights_saved_b = DAT_00284b18;
		lt_SetAmbient(0x7f, 0x7f, 0x7f);
	}
	for (light = chl_ListNext(lights_list, NULL); light != NULL; light = chl_ListNext(lights_list, light)) {
		if (CHL_I32(light, LT_ON) != 0 && (restore_ambient == 0 || CHL_I32(light, LT_LIMITED) == 0))
			lt_Enable(CHL_FIELD(void, light, LT_BLIGHT), enable);
	}
	if (restore_ambient != 0 && enable != 0)
		lt_SetAmbient(lights_saved_r, lights_saved_g, lights_saved_b);
}

/* 0x000b6aa0  CharLib (lights.cpp, inferred) */
void Lights_SetLimitMode(int mode)
{
	lights_limit_mode = mode;
}

/* 0x000b6ab0  CharLib (lights.cpp, inferred)
In limit mode 1, when a limited light comes on (`on`, passed in AL in the original) and
more than one limited light is on, the first of them is switched off. Returns 1. */
int Lights_Limit(void *light, int on)
{
	void *other;
	int lit = 0;

	if ((char)on == 0 || CHL_I32(light, LT_LIMITED) == 0 || lights_limit_mode != 1)
		return 1;
	other = chl_ListNext(lights_list, NULL);
	if (other == NULL)
		return 1;
	do {
		if (CHL_I32(other, LT_LIMITED) != 0 && CHL_I32(other, LT_ON) != 0)
			lit++;
		other = chl_ListNext(lights_list, other);
	} while (other != NULL);
	if (1 < lit && (other = chl_ListNext(lights_list, NULL)) != NULL) {
		while (CHL_I32(other, LT_LIMITED) == 0 || CHL_I32(other, LT_ON) == 0) {
			other = chl_ListNext(lights_list, other);
			if (other == NULL)
				return 1;
		}
		lt_Enable(CHL_FIELD(void, other, LT_BLIGHT), 0);
		CHL_I32(other, LT_ON) = 0;
	}
	return 1;
}

/* 0x000b6b80  CharLib (lights.cpp, inferred)
A spot light at pos pointing at target. */
void *Light_InitSpot(void *light, unsigned char r, unsigned char g, unsigned char b, float *pos, float *target,
	float range, float angle)
{
	CHL_I32(light, LT_VISIBLE) = 1;
	CHL_I32(light, LT_ON) = 0;
	light_create(light);
	CHL_PTR(light, LT_ATTACH) = NULL;
	CHL_I32(light, LT_TYPE) = 1;
	CHL_F32(light, LT_DELAY) = 0.0f;
	memcpy(CHL_FIELD(char, light, LT_POS), pos, 0x10);
	CHL_U32(light, LT_COLOUR + 0x0) = r;
	CHL_U32(light, LT_COLOUR + 0x4) = g;
	CHL_U32(light, LT_COLOUR + 0x8) = b;
	pos[3] = 1.0f;
	target[3] = 1.0f;
	if (CHL_I32(light, LT_CREATED) != 0)
		lt_SetSpot(CHL_FIELD(void, light, LT_BLIGHT), pos, target, range, angle);
	chl_ListInsert(lights_list, light, 0);
	CHL_I32(light, LT_LIMITED) = 0;
	CHL_I32(light, LT_ON) = 1;
	return light;
}

/* 0x000b6c70  CharLib (lights.cpp, inferred)
A directional light that turns about y by `turn` per frame (limited). */
void *Light_InitTurning(void *light, unsigned char r, unsigned char g, unsigned char b, float turn, float *dir)
{
	CHL_I32(light, LT_VISIBLE) = 1;
	CHL_I32(light, LT_ON) = 0;
	light_create(light);
	CHL_U32(light, LT_COLOUR + 0x0) = r;
	CHL_U32(light, LT_COLOUR + 0x8) = b;
	CHL_I32(light, LT_TYPE) = 5;
	CHL_PTR(light, LT_ATTACH) = NULL;
	CHL_F32(light, LT_DELAY) = 0.0f;
	CHL_U32(light, LT_COLOUR + 0x4) = g;
	memcpy(CHL_FIELD(char, light, LT_DIR), dir, 0x10);
	CHL_F32(light, LT_DIR + 0xc) = 1.0f;
	if (CHL_I32(light, LT_CREATED) != 0)
		lt_SetDirectional(CHL_FIELD(void, light, LT_BLIGHT), CHL_FIELD(float, light, LT_DIR));
	chl_ListInsert(lights_list, light, 0);
	CHL_F32(light, LT_ANGLE) = 0.0f;
	CHL_F32(light, LT_TURN_SPEED) = turn;
	CHL_I32(light, LT_LIMITED) = 1;
	return light;
}

/* 0x000b6d60  CharLib (lights.cpp, inferred)
A turning directional light that fades out (to black) over `duration` frames after
`delay` frames (limited). */
void *Light_InitTurningFade(void *light, int *colour, float turn, float *dir, float duration, float delay)
{
	CHL_I32(light, LT_VISIBLE) = 1;
	CHL_I32(light, LT_ON) = 0;
	light_create(light);
	CHL_F32(light, LT_DURATION) = duration;
	CHL_F32(light, LT_DELAY) = delay;
	CHL_F32(light, LT_TIME) = 0.0f;
	CHL_PTR(light, LT_ATTACH) = NULL;
	CHL_I32(light, LT_TYPE) = 6;
	CHL_I32(light, LT_FROM + 0x0) = colour[0];
	CHL_I32(light, LT_FROM + 0x4) = colour[1];
	CHL_I32(light, LT_FROM + 0x8) = colour[2];
	CHL_I32(light, LT_COLOUR + 0x8) = CHL_I32(light, LT_FROM + 0x8);
	CHL_I32(light, LT_COLOUR + 0x4) = CHL_I32(light, LT_FROM + 0x4);
	CHL_I32(light, LT_TO + 0x0) = 0;
	CHL_I32(light, LT_TO + 0x4) = 0;
	CHL_I32(light, LT_TO + 0x8) = 0;
	CHL_I32(light, LT_COLOUR + 0x0) = CHL_I32(light, LT_FROM + 0x0);
	memcpy(CHL_FIELD(char, light, LT_DIR), dir, 0x10);
	CHL_F32(light, LT_DIR + 0xc) = 1.0f;
	if (CHL_I32(light, LT_CREATED) != 0)
		lt_SetDirectional(CHL_FIELD(void, light, LT_BLIGHT), CHL_FIELD(float, light, LT_DIR));
	CHL_F32(light, LT_ANGLE) = 0.0f;
	CHL_F32(light, LT_TURN_SPEED) = turn;
	chl_ListInsert(lights_list, light, 0);
	CHL_I32(light, LT_LIMITED) = 1;
	if (CHL_I32(light, LT_CREATED) != 0 && CHL_F32(light, LT_DELAY) == 0.0f)
		Lights_Limit(light, 1);
	return light;
}

/* 0x000b6eb0  CharLib (lights.cpp, inferred)
A point light. */
void *Light_InitPoint(void *light, unsigned char r, unsigned char g, unsigned char b, float *pos, float range)
{
	CHL_I32(light, LT_VISIBLE) = 1;
	CHL_I32(light, LT_ON) = 0;
	light_create(light);
	CHL_PTR(light, LT_ATTACH) = NULL;
	CHL_F32(light, LT_DELAY) = 0.0f;
	CHL_I32(light, LT_TYPE) = 2;
	memcpy(CHL_FIELD(char, light, LT_POS), pos, 0x10);
	CHL_F32(light, LT_RANGE) = range;
	CHL_U32(light, LT_COLOUR + 0x0) = r;
	CHL_U32(light, LT_COLOUR + 0x4) = g;
	CHL_U32(light, LT_COLOUR + 0x8) = b;
	pos[3] = 1.0f;
	if (CHL_I32(light, LT_CREATED) != 0)
		lt_SetPoint(CHL_FIELD(void, light, LT_BLIGHT), pos, range);
	chl_ListInsert(lights_list, light, 0);
	CHL_I32(light, LT_LIMITED) = 0;
	CHL_I32(light, LT_ON) = 1;
	return light;
}

/* 0x000b6fa0  CharLib (lights.cpp, inferred)
A point light that fades from colour `from` and range `range` to `to` and `range_to` over
`duration` frames after `delay` frames (limited); it deletes itself at the end. */
void *Light_InitFadingPoint(void *light, int *from, int *to, float *pos, float range, float range_to, float duration,
	float delay)
{
	CHL_I32(light, LT_VISIBLE) = 1;
	CHL_I32(light, LT_ON) = 0;
	light_create(light);
	CHL_F32(light, LT_DELAY) = delay;
	CHL_PTR(light, LT_ATTACH) = NULL;
	CHL_I32(light, LT_TYPE) = 3;
	CHL_I32(light, LT_FROM + 0x0) = from[0];
	CHL_I32(light, LT_FROM + 0x4) = from[1];
	CHL_I32(light, LT_FROM + 0x8) = from[2];
	CHL_I32(light, LT_TO + 0x0) = to[0];
	CHL_I32(light, LT_TO + 0x4) = to[1];
	CHL_I32(light, LT_TO + 0x8) = to[2];
	memcpy(CHL_FIELD(char, light, LT_POS), pos, 0x10);
	CHL_F32(light, LT_RANGE) = range;
	CHL_I32(light, LT_COLOUR + 0x0) = CHL_I32(light, LT_FROM + 0x0);
	CHL_F32(light, LT_RANGE_TO) = range_to;
	CHL_I32(light, LT_COLOUR + 0x8) = CHL_I32(light, LT_FROM + 0x8);
	CHL_F32(light, LT_DURATION) = duration;
	CHL_F32(light, LT_TIME) = 0.0f;
	CHL_I32(light, LT_COLOUR + 0x4) = CHL_I32(light, LT_FROM + 0x4);
	if (CHL_I32(light, LT_CREATED) != 0)
		lt_SetPoint(CHL_FIELD(void, light, LT_BLIGHT), pos, range);
	chl_ListInsert(lights_list, light, 0);
	CHL_I32(light, LT_LIMITED) = 1;
	if (CHL_I32(light, LT_CREATED) != 0 && CHL_F32(light, LT_DELAY) == 0.0f)
		Lights_Limit(light, 1);
	return light;
}

/* 0x000b7100  CharLib (lights.cpp, inferred)
A point light that follows a platform (or a node of it), at offset pos (limited). */
void *Light_InitAttached(void *light, void *attach, void *node, float *pos, int *colour, float range, int visible)
{
	CHL_I32(light, LT_VISIBLE) = visible;
	CHL_I32(light, LT_ON) = 0;
	light_create(light);
	CHL_PTR(light, LT_NODE) = node;
	CHL_PTR(light, LT_ATTACH) = attach;
	CHL_I32(attach, 0x45c) = CHL_I32(attach, 0x45c) + 1;
	CHL_I32(light, LT_TYPE) = 4;
	CHL_I32(light, LT_FROM + 0x0) = colour[0];
	CHL_I32(light, LT_FROM + 0x4) = colour[1];
	CHL_I32(light, LT_FROM + 0x8) = colour[2];
	CHL_I32(light, LT_TO + 0x0) = colour[0];
	CHL_I32(light, LT_TO + 0x4) = colour[1];
	CHL_I32(light, LT_TO + 0x8) = colour[2];
	CHL_F32(light, LT_RANGE) = range;
	CHL_F32(light, LT_DURATION) = 0.0f;
	CHL_F32(light, LT_DELAY) = 0.0f;
	CHL_F32(light, LT_TIME) = 0.0f;
	CHL_F32(light, LT_RANGE_TO) = range;
	memcpy(CHL_FIELD(char, light, LT_POS), pos, 0x10);
	if (CHL_I32(light, LT_CREATED) != 0) {
		lt_SetPoint(CHL_FIELD(void, light, LT_BLIGHT), CHL_FIELD(float, CHL_PTR(light, LT_ATTACH), 0xe0),
			CHL_F32(light, LT_RANGE));
		if (CHL_I32(light, LT_CREATED) != 0)
			lt_SetColour(CHL_FIELD(void, light, LT_BLIGHT), CHL_I32(light, LT_FROM + 0x0),
				CHL_I32(light, LT_FROM + 0x4), CHL_I32(light, LT_FROM + 0x8));
	}
	chl_ListInsert(lights_list, light, 0);
	CHL_I32(light, LT_LIMITED) = 1;
	if (CHL_I32(light, LT_CREATED) != 0)
		Lights_Limit(light, 1);
	return light;
}

/* 0x000b7270  CharLib (lights.cpp, inferred)
Moves a point light. */
void Light_SetPosition(void *light, float *pos)
{
	if (CHL_I32(light, LT_CREATED) != 0 && CHL_I32(light, LT_TYPE) == 2) {
		lt_SetPoint(CHL_FIELD(void, light, LT_BLIGHT), pos, CHL_F32(light, LT_RANGE));
		memcpy(CHL_FIELD(char, light, LT_POS), pos, 0x10);
	}
}

/* 0x000b72d0  CharLib/lights.cpp
New turning fading directional light (NULL when 5 lights exist). */
void *Light_NewTurningFade(int r, int g, int b, float turn, float *dir, float duration, float delay)
{
	int colour[3];
	void *light;

	if (lights_count < 5) {
		colour[0] = r;
		colour[1] = g;
		colour[2] = b;
		light = chl_Alloc(0xe8, "C:\\BlitzParty\\Game\\Src\\CharLib\\lights.cpp", 0x2b7, 0x1001);
		if (light != NULL)
			return Light_InitTurningFade(light, colour, turn, dir, duration, delay);
	}
	return NULL;
}

/* 0x000b7380  CharLib/lights.cpp
New flash: a point light of the colour that fades to black over `duration` frames,
range 1 / strength growing to 10 / strength. */
void *Light_NewFlash(float *pos, float strength, int duration, int r, int g, int b, float delay)
{
	int black[3];
	int colour[3];
	void *light;

	if (5 <= lights_count)
		return NULL;
	if (strength == 0.0f) {
		chl_Printf("Light of no strength equals no light at all.\n SORT IT OUT!\n");
		return NULL;
	}
	black[0] = 0;
	black[1] = 0;
	black[2] = 0;
	colour[0] = r;
	colour[1] = g;
	colour[2] = b;
	light = chl_Alloc(0xe8, "C:\\BlitzParty\\Game\\Src\\CharLib\\lights.cpp", 0x2d9, 0x1001);
	if (light == NULL)
		return NULL;
	return Light_InitFadingPoint(light, colour, black, pos, 1.0f / strength, (1.0f / strength) * 10.0f,
		(float)duration, delay);
}

/* 0x000b7490  CharLib/lights.cpp
New point light following a platform (range 1 / strength). */
void *Light_NewAttached(void *attach, void *node, float *pos, float strength, int r, int g, int b, int visible)
{
	int colour[3];
	void *light;

	if (5 <= lights_count)
		return NULL;
	if (strength == 0.0f) {
		chl_Printf("Light of no strength equals no light at all.\n SORT IT OUT!\n");
		return NULL;
	}
	colour[0] = r;
	colour[1] = g;
	colour[2] = b;
	light = chl_Alloc(0xe8, "C:\\BlitzParty\\Game\\Src\\CharLib\\lights.cpp", 0x2fa, 0x1001);
	if (light == NULL)
		return NULL;
	return Light_InitAttached(light, attach, node, pos, colour, 1.0f / strength, visible);
}

/* 0x000b7570  CharLib/lights.cpp
A short flash at pos (fades out over 10 frames). */
void Light_Flash(float *pos, int r, int g, int b)
{
	int colour[3];
	int black[3];
	void *light;

	colour[0] = r;
	colour[1] = g;
	colour[2] = b;
	black[0] = 0;
	black[1] = 0;
	black[2] = 0;
	if (lights_count < 5) {
		light = chl_Alloc(0xe8, "C:\\BlitzParty\\Game\\Src\\CharLib\\lights.cpp", 0x313, 0x1001);
		if (light != NULL)
			Light_InitFadingPoint(light, colour, black, pos, 0.0004f, 0.004f, 10.0f, 0.0f);
	}
}

/* 0x000b7620  CharLib (lights.cpp, inferred)
Shows or hides the attached lights. */
void Lights_SetAttachedVisible(int visible)
{
	void *light;

	for (light = chl_ListNext(lights_list, NULL); light != NULL; light = chl_ListNext(lights_list, light)) {
		if (CHL_I32(light, LT_TYPE) == 4) {
			CHL_I32(light, LT_VISIBLE) = visible;
			Lights_Limit(light, visible != 0);
		}
	}
}

/* 0x000b7680  CharLib (lights.cpp, inferred)
Deletes a light; returns the list node before it. */
void *Light_Delete(void *light)
{
	void *prev = chl_ListRemove(lights_list, light);

	if (CHL_I32(light, LT_CREATED) != 0) {
		if (CHL_I32(light, LT_ON) != 0)
			lights_on_count = lights_on_count - 1;
		lt_Delete(CHL_FIELD(void, light, LT_BLIGHT));
		lights_count = lights_count - 1;
	}
	if (CHL_PTR(light, LT_ATTACH) != NULL)
		CHL_I32(CHL_PTR(light, LT_ATTACH), 0x45c) = CHL_I32(CHL_PTR(light, LT_ATTACH), 0x45c) - 1;
	chl_Free(light);
	return prev;
}

/* the position of an attached light: the platform's position plus the offset (turned by
the node's matrix when there is a node) */
static void light_attached_pos(void *light, float *out)
{
	void *attach = CHL_PTR(light, LT_ATTACH);

	if (CHL_PTR(light, LT_NODE) != NULL) {
		chl_PrepInstance((char *)attach + 0x20, 1);
		FUN_000505c0(out, CHL_FIELD(float, CHL_PTR(light, LT_NODE), 0x80), CHL_FIELD(float, light, LT_POS));
		lt_VecAdd(out, CHL_FIELD(float, CHL_PTR(light, LT_ATTACH), 0xe0), out);
		return;
	}
	lt_VecAdd(out, CHL_FIELD(float, light, LT_POS), CHL_FIELD(float, attach, 0xe0));
}

/* the colour of a fade at t / 1024 */
static void light_fade_colour(void *light, int t)
{
	int u = 0x400 - t;
	int bb = CHL_I32(light, LT_FROM + 0x8) * u + CHL_I32(light, LT_TO + 0x8) * t;
	int gg = CHL_I32(light, LT_FROM + 0x4) * u + CHL_I32(light, LT_TO + 0x4) * t;
	int rr = CHL_I32(light, LT_FROM + 0x0) * u + CHL_I32(light, LT_TO + 0x0) * t;

	lt_SetColour(CHL_FIELD(void, light, LT_BLIGHT), rr / 1024, gg / 1024, bb / 1024);
}

/* 0x000b76f0  CharLib (lights.cpp, inferred)
Per frame: turns, fades (deleting the light at the end of its fade) and moves the lights
with their platforms. A light without a bactor light is deleted. */
void Light_Update(void *light)
{
	float pos[4];
	float range;
	int t;

	if (CHL_I32(light, LT_CREATED) == 0) {
		Light_Delete(light);
		return;
	}
	switch (CHL_I32(light, LT_TYPE)) {
	case 3:
		if (!(CHL_F32(light, LT_DELAY) <= 0.0f))
			break;
		CHL_F32(light, LT_TIME) = chl_time_scale + CHL_F32(light, LT_TIME);
		t = (CHL_FTOL(CHL_F32(light, LT_TIME)) << 10) / CHL_FTOL(CHL_F32(light, LT_DURATION));
		if (0x400 < t) {
			Light_Delete(light);
			return;
		}
		light_fade_colour(light, t);
		range = ((float)(0x400 - t) * CHL_F32(light, LT_RANGE) + (float)t * CHL_F32(light, LT_RANGE_TO)) *
			0.0009765625f;
		if (CHL_PTR(light, LT_ATTACH) == NULL) {
			lt_SetPoint(CHL_FIELD(void, light, LT_BLIGHT), CHL_FIELD(float, light, LT_POS), range);
			return;
		}
		light_attached_pos(light, pos);
		lt_SetPoint(CHL_FIELD(void, light, LT_BLIGHT), pos, range);
		return;
	case 4:
		if (CHL_I32(light, LT_VISIBLE) == 0)
			return;
		if (CHL_PTR(light, LT_ATTACH) == NULL) {
			lt_SetPoint(CHL_FIELD(void, light, LT_BLIGHT), CHL_FIELD(float, light, LT_POS), CHL_F32(light, LT_RANGE));
			return;
		}
		light_attached_pos(light, pos);
		lt_SetPoint(CHL_FIELD(void, light, LT_BLIGHT), pos, CHL_F32(light, LT_RANGE));
		return;
	case 5:
	case 6:
		CHL_F32(light, LT_ANGLE) = chl_time_scale * CHL_F32(light, LT_TURN_SPEED) + CHL_F32(light, LT_ANGLE);
		CHL_F32(light, LT_ANGLE) = FUN_000703a0(CHL_F32(light, LT_ANGLE));
		FUN_00051170(pos, CHL_FIELD(float, light, LT_DIR), CHL_F32(light, LT_ANGLE));
		lt_SetDirectional(CHL_FIELD(void, light, LT_BLIGHT), pos);
		if (CHL_I32(light, LT_TYPE) == 5)
			return;
		if (!(CHL_F32(light, LT_DELAY) <= 0.0f))
			break;
		CHL_F32(light, LT_TIME) = chl_time_scale + CHL_F32(light, LT_TIME);
		t = (CHL_FTOL(CHL_F32(light, LT_TIME)) << 10) / CHL_FTOL(CHL_F32(light, LT_DURATION));
		if (0x400 < t) {
			Light_Delete(light);
			return;
		}
		light_fade_colour(light, t);
		return;
	default:
		return;
	}
	/* the delay before a fade */
	CHL_F32(light, LT_DELAY) = CHL_F32(light, LT_DELAY) - chl_time_scale;
	if (CHL_F32(light, LT_DELAY) <= 0.0f) {
		Lights_Limit(light, 1);
		CHL_F32(light, LT_DELAY) = 0.0f;
	}
}
