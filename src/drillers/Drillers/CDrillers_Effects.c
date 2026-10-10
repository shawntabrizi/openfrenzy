/*
Drillers/CDrillers_Effects.c  <-  C:\BlitzParty\Game\Src\Drillers\CDrillers_Effects.cpp

The effects object (DrEffects, drillers.h; one static object at 0x00300c78, vtable 0x0026c124):
the four per-player burst groups and five spark systems (CDrillers_Particle.c), the rising
score numbers, the bonus tokens, the screen flash, and ten BACTOR extra systems:
  gen     effect generators: every item fires an effect of another system
  suck    sprites that fly to a target        steam   rising, turning steam sprites
  corona  two turning glows                   hsteam  steam that spreads from a point
  ring    a fading ring (also named "hsteam") dust    a puff of dust
  scorch  a scorch mark on the ground         worm    a glowing trail on the grid
  light   point lights                        rain    (CDrillers_Particle.c)

0x00113fd0 and 0x00114230 name this file in their allocations; the gap functions from
0x001115f0 to 0x001147d0 work on the same object and its extra systems (file inferred).
Twenty-six of them are not in of_functions.tsv (extra-system callbacks reached only by
pointer); they keep their FUN_<addr> names (declared in drillers.h).

The items of the extra systems are reached at their original byte offsets (DR_F32/DR_I32).
Their creation callbacks read the arguments of baAddExtraItem from the va_list: floats come
as doubles (C promotion, as in the original), bytes as ints.
*/

#include "drillers.h"

/* this object */
#define FX g_DrFx

/* the textures and the font of the resources object */
#define DR_TEX_GLOW DR_VAR(int, 0x002aa964u)
#define DR_TEX_CORONA DR_VAR(int, 0x002aa968u)
#define DR_TEX_SCORCH DR_VAR(int, 0x002aa96cu)
#define DR_FONT_MARKER DR_VAR(struct bdFont *, 0x002aa984u)
#define DR_FONT_FLOATY DR_VAR(int, 0x002aa98cu)
/* the light made last */
#define DR_LAST_LIGHT DR_VAR(struct BLight *, 0x002aa9acu)
/* the saved render states of the game */
#define DR_RSTATES DR_PTR(unsigned int, 0x002aaa70u)
/* the per-round rotation of the world (degrees) and the round (game +0x68) */
#define DR_ROUND_ANGLE DR_VAR(float, 0x002a55e0u)
#define DR_ROUND DR_VAR(int, 0x002ffa58u)
/* the camera (eye, target) and its heading */
#define DR_CAM_EYE DR_PTR(float, 0x002aa834u)
#define DR_CAM_TARGET DR_PTR(float, 0x002aa840u)
#define DR_CAM_HEADING DR_VAR(float, 0x002aa84cu)
/* the light rotation of Erodeo (degrees) */
#define DR_LIGHT_ANGLE DR_VAR(float, 0x002a5614u)
/* the map size: cell size, half the number of cells across x and z */
#define DR_CELL DR_VAR(int, 0x002ff9e0u)
#define DR_HALF_Z DR_VAR(int, 0x002ff9e4u)
#define DR_HALF_X DR_VAR(int, 0x002ff9e8u)
/* the game's frame counter (game +0x1c) and state (game +0x10) */
#define DR_FRAME DR_VAR(unsigned int, 0x002ffa0cu)
/* the value set by a type 3 generator (read by the sparks' InitParticle) */
#define DR_GEN_ANGLE DR_VAR(float, 0x00301c28u)
/* the number of tokens allowed (float) */
#define DR_MAX_TOKENS DR_VAR(float, 0x002aae14u)
/* the map columns (0x00298da4: one cell array per x; a cell is 0x2d8 bytes, height at +0x2d0) */
#define DR_MAP_COLUMN(x) DR_VAR(int, 0x00298da4u + (x) * 4)

/* the colours of the four players */
#define DR_PLAYER_R(i) DR_VAR(unsigned char, 0x002aa90cu + (i))
#define DR_PLAYER_G(i) DR_VAR(unsigned char, 0x002aa910u + (i))
#define DR_PLAYER_B(i) DR_VAR(unsigned char, 0x002aa914u + (i))

/* vertex buffers of the draw functions */
#define DR_VTX_SCORCH DR_PTR(bdVertex, 0x002ffac0u)  /* 4 */
#define DR_VTX_WORM DR_PTR(bdVertex, 0x002ffb50u)    /* 32 */
#define DR_VTX_FLASH DR_PTR(bdVertex, 0x00300018u)   /* 4 */
#define DR_VTX_RING DR_PTR(bdVertex, 0x003000a8u)    /* 54 */
#define DR_VTX_CIRCLE DR_PTR(bdVertex, 0x00300ac8u)  /* 12 */

/* a colour of the bdisplay tables */
#define DR_RGBA(r, g, b, a) \
	(DAT_002bce18[(r)] | DAT_002bc490[(g)] | DAT_002bd640[(b)] | DAT_002bc898[(a)])

#define DR_DEG 0.017453292f
/* the height of the current round's layer: (round - 1) * 20 cells of 80 */
#define DR_LAYER_Y() ((float)(DR_ROUND - 1) * 20.0f * 80.0f)

/* 0x0026c124  vtable of the effects object (rebuilt from the XBE) */
const void *const vtable_0026c124[1] = {
	(const void *)DrEffects_DeletingDtor, /* +0x00 */
};

/* ---------- the burst groups */

/* 0x001115f0  Drillers (gap; CDrillers_Effects.cpp inferred)
Constructor of a burst group: its three particle systems. */
DrFxGroup *DrFxGroup_Ctor(DrFxGroup *self)
{
	DrSparks_Ctor(&self->sparks);
	DrDust_Ctor(&self->dust);
	DrSmoke_Ctor(&self->smoke);
	return self;
}

/* 0x00111650  Drillers (gap; CDrillers_Effects.cpp inferred) */
void DrFxGroup_Dtor(DrFxGroup *self)
{
	DrSmoke_Dtor(&self->smoke);
	DrDust_Dtor(&self->dust);
	DrSparks_Dtor(&self->sparks);
}

/* 0x001116b0  Drillers (gap; CDrillers_Effects.cpp inferred)
Destructor of the effects object (the arrays are destroyed last to first). */
void DrEffects_Dtor(DrEffects *self)
{
	int i;

	self->vtable = vtable_0026c124;
	for (i = 7; i >= 0; i--)
		DrSparks_Dtor(&self->sparks[i]);
	for (i = 3; i >= 0; i--)
		DrFxGroup_Dtor(&self->groups[i]);
}

/* 0x00111720  Drillers (gap; CDrillers_Effects.cpp inferred)
The five spark systems: 10 sparks each, inactive. */
void DrEffects_InitSparks(DrEffects *self)
{
	int i;

	for (i = 0; i < 5; i++) {
		DrSparks_Init(&self->sparks[i], NULL, 10);
		self->sparks[i].p.flags &= ~3u;
	}
}

/* 0x00111750  Drillers (gap; CDrillers_Effects.cpp inferred) */
void DrEffects_ReleaseSparks(DrEffects *self)
{
	int i;

	for (i = 0; i < 5; i++)
		DR_VFN(&self->sparks[i], 0x00, void (*)(DrParticleSystem *))(&self->sparks[i]);
}

/* 0x00111770  Drillers (not in of_functions.tsv) */
void FUN_00111770(void *self)
{
	DrEffects *fx = (DrEffects *)self;
	int i;

	for (i = 0; i < 5; i++)
		DrPs_VUpdate(&fx->sparks[i], 1.0f);
}

/* 0x001117a0  Drillers (not in of_functions.tsv) */
void FUN_001117a0(void *self)
{
	DrEffects *fx = (DrEffects *)self;
	int i;

	for (i = 0; i < 5; i++) {
		if ((fx->sparks[i].p.flags & 1) != 0)
			DrPs_VDraw(&fx->sparks[i]);
	}
}

/* 0x001117d0  Drillers (gap; CDrillers_Effects.cpp inferred)
A burst of orange sparks at `pos` from the first idle spark system. */
void DrEffects_SparkBurst(DrEffects *self, float *pos)
{
	int i;

	for (i = 0; i < 5; i++) {
		if ((self->sparks[i].p.flags & 1) == 0) {
			CParticleSystem_SetPosition(&self->sparks[i].h, pos);
			DrPs_Burst(&self->sparks[i], -1);
			self->sparks[i].p.flags |= 1;
			return;
		}
	}
}

/* 0x00111830  Drillers (gap; CDrillers_Effects.cpp inferred)
A burst group: 10 sparks and 80 dust particles. */
void DrFxGroup_Init(DrFxGroup *self)
{
	DrSparks_Init(&self->sparks, NULL, 10);
	DrDust_Init(&self->dust, NULL, 0x50);
}

/* 0x00111850  Drillers (gap; CDrillers_Effects.cpp inferred) */
void DrFxGroup_Release(DrFxGroup *self)
{
	DR_VFN(&self->sparks, 0x00, void (*)(DrParticleSystem *))(&self->sparks);
	DR_VFN(&self->dust, 0x00, void (*)(DrParticleSystem *))(&self->dust);
}

/* 0x00111870  Drillers (gap; CDrillers_Effects.cpp inferred)
While on: the time, the sparks and the dust; the group goes off when both are done. */
void DrFxGroup_Update(DrFxGroup *self, float dt)
{
	char active;

	if ((self->flags & 1) == 0)
		return;
	self->time = dt + self->time;
	DrPs_VUpdate(&self->sparks, dt);
	DrPs_VUpdate(&self->dust, dt);
	active = (self->sparks.p.flags & 1) != 0;
	if ((self->dust.p.flags & 1) != 0)
		active++;
	if (active == 0)
		self->flags &= ~3u;
}

/* 0x001118d0  Drillers (gap; CDrillers_Effects.cpp inferred) */
void DrFxGroup_Draw(DrFxGroup *self)
{
	if ((self->flags & 2) != 0) {
		DrPs_VDraw(&self->sparks);
		DrPs_VDraw(&self->dust);
	}
}

/* 0x001118f0  Drillers (gap; CDrillers_Effects.cpp inferred) */
void DrFxGroup_Stop(DrFxGroup *self)
{
	self->flags &= ~3u;
	self->time = 0.0f;
	DrPs_VStop(&self->sparks);
	DrPs_VStop(&self->dust);
	DrPs_VStop(&self->smoke);
}

/* ---------- extra-system callbacks */

/* the brightness of steam at height y: 8 - y / 300, from 0 to 8 */
static unsigned char dr_steam_light(float y)
{
	float v = 8.0f - y * 0.0033333334f;

	if (v < 0.0f)
		v = 0.0f;
	else if (v > 8.0f)
		v = 8.0f;
	return (unsigned char)DR_FTOL(v);
}

/* the world turned by the round angle, alpha blended sprites */
static void dr_begin_round_sprites(void)
{
	float m[16];

	bmMatrixRotationY(m, DR_ROUND_ANGLE * DR_DEG);
	bdSetWorldMatrix(m);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(8, 0, 0);
}

/* 0x00111930  Drillers (not in of_functions.tsv; draw callback of "steam")
256-unit sprites in the round's layer, every other one turning the other way. */
int FUN_00111930(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char *it;
	unsigned char light;
	float angle;
	unsigned int i;

	(void)system;
	(void)arg;
	dr_begin_round_sprites();
	for (i = 0; (int)i < count; i++) {
		it = (char *)items[i];
		light = dr_steam_light(DR_F32(it, 4));
		DR_F32(it, 4) = DR_LAYER_Y() + DR_F32(it, 4);
		angle = (float)DR_I32(it, 0x10) * DR_DEG;
		if ((i & 1) == 0)
			angle = -angle;
		bdDrawSprite3D((float *)it, 256.0f, 256.0f, angle,
			(struct bdTexture *)(uintptr_t)DR_TEX_GLOW, 0, 0, light, light, light, 0xff, NULL,
			0.0f);
		DR_F32(it, 4) = DR_F32(it, 4) - DR_LAYER_Y();
	}
	bdSetProjectionMode(1);
	bdRestoreRenderStates(DR_RSTATES);
	return 0;
}

/* 0x00111ab0  Drillers (not in of_functions.tsv; system callback of "light")
When the system is deleted: the light of its first item goes. */
int FUN_00111ab0(struct BExtraSystem *system, int deleted)
{
	char *it = DR_AT(system, 0x3c, char *); /* itemData */

	if (deleted != 1)
		return 0;
	if (DR_AT(it, 0x1c, struct BLight *) != NULL) {
		baEnableLight(DR_AT(it, 0x1c, struct BLight *), 0);
		baDeleteLight(DR_AT(it, 0x1c, struct BLight *));
		DR_AT(it, 0x1c, struct BLight *) = NULL;
		DR_LAST_LIGHT = NULL;
		bkPrintf("drillers effects lights shutdown ok\n");
		return 0;
	}
	bkPrintf("make sure lights gone! - if there's an shutdown ok message - we're clear!\n");
	return 0;
}

/* 0x00111b10  Drillers (not in of_functions.tsv; item callback of "light")
Arguments: x, y, z, range, r, g, b. A point light of reach 0.007 / range. */
int FUN_00111b10(struct BExtraSystem *system, void *item, void *args)
{
	char *it = (char *)item;
	va_list ap;
	float pos[4];
	float range;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	DR_F32(it, 0) = (float)va_arg(ap, double);
	DR_F32(it, 4) = (float)va_arg(ap, double);
	DR_F32(it, 0xc) = 1.0f;
	DR_F32(it, 8) = (float)va_arg(ap, double);
	DR_I32(it, 0x14) = va_arg(ap, int);
	range = 0.007f / (float)DR_I32(it, 0x14);
	DR_U8(it, 0x18) = (unsigned char)va_arg(ap, int);
	DR_U8(it, 0x19) = (unsigned char)va_arg(ap, int);
	pos[0] = DR_F32(it, 0);
	pos[1] = DR_F32(it, 4);
	DR_U8(it, 0x1a) = (unsigned char)va_arg(ap, int);
	pos[2] = DR_F32(it, 8);
	DR_I32(it, 0x10) = 0;
	pos[3] = 1.0f;
	DR_LAST_LIGHT = baCreateLight(NULL, 0);
	DR_AT(it, 0x1c, struct BLight *) = DR_LAST_LIGHT;
	baSetPointLight(DR_AT(it, 0x1c, struct BLight *), pos, range);
	baSetLightColour(DR_AT(it, 0x1c, struct BLight *), DR_U8(it, 0x18), DR_U8(it, 0x19),
		DR_U8(it, 0x1a));
	baEnableLight(DR_AT(it, 0x1c, struct BLight *), 1);
	bkPrintf("effect light has been created - should be cleared on shutdown\n");
	return 0;
}

/* 0x00111bf0  Drillers (not in of_functions.tsv; update callback of "light")
Places each light (in Erodeo turned about z with the scene). */
int FUN_00111bf0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char *it;
	float pos[4];
	float copy[4];
	float range;
	int i;

	(void)system;
	(void)arg;
	for (i = 0; i < count; i++) {
		it = (char *)items[i];
		pos[0] = DR_F32(it, 0);
		pos[1] = DR_F32(it, 4);
		range = 0.007f / (float)DR_I32(it, 0x14);
		pos[2] = DR_F32(it, 8);
		pos[3] = 1.0f;
		if (g_DrVariant == 2) {
			copy[0] = DR_F32(it, 0);
			copy[1] = DR_F32(it, 4);
			copy[2] = DR_F32(it, 8);
			copy[3] = 1.0f;
			bmVectorRotateZ(pos, copy, DR_LIGHT_ANGLE * DR_DEG);
		}
		baSetPointLight(DR_AT(it, 0x1c, struct BLight *), pos, range);
		DR_I32(it, 0x10) += 1;
	}
	return 0;
}

/* 0x00111cc0  Drillers (not in of_functions.tsv; draw callback of "hsteam")
128-unit turning sprites in the round's layer. */
int FUN_00111cc0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char *it;
	unsigned char light;
	int i;

	(void)system;
	(void)arg;
	dr_begin_round_sprites();
	for (i = 0; i < count; i++) {
		it = (char *)items[i];
		light = dr_steam_light(DR_F32(it, 4));
		DR_F32(it, 4) = DR_LAYER_Y() + DR_F32(it, 4);
		bdDrawSprite3D((float *)it, 128.0f, 128.0f, (float)DR_I32(it, 0x10) * DR_DEG,
			(struct bdTexture *)(uintptr_t)DR_TEX_GLOW, 0, 0, light, light, light, 0xff, NULL,
			0.0f);
		DR_F32(it, 4) = DR_F32(it, 4) - DR_LAYER_Y();
	}
	bdSetProjectionMode(1);
	bdRestoreRenderStates(DR_RSTATES);
	return 0;
}

/* 0x00111e30  Drillers (not in of_functions.tsv; draw callback of "corona")
Two glows (400 and 300 units) turning against each other, in the item's colour. */
int FUN_00111e30(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char *it;
	int i;

	(void)system;
	(void)arg;
	dr_begin_round_sprites();
	for (i = 0; i < count; i++) {
		it = (char *)items[i];
		DR_F32(it, 4) = DR_LAYER_Y() + DR_F32(it, 4);
		bdDrawSprite3D((float *)it, 400.0f, 400.0f,
			-(((float)DR_I32(it, 0x10) + DR_CAM_HEADING) * DR_DEG),
			(struct bdTexture *)(uintptr_t)DR_TEX_CORONA, 0, 0, DR_U8(it, 0x18), DR_U8(it, 0x19),
			DR_U8(it, 0x1a), 0xff, NULL, 0.0f);
		bdDrawSprite3D((float *)it, 300.0f, 300.0f, (float)DR_I32(it, 0x10) * DR_DEG,
			(struct bdTexture *)(uintptr_t)DR_TEX_CORONA, 0, 0, DR_U8(it, 0x18), DR_U8(it, 0x19),
			DR_U8(it, 0x1a), 0xff, NULL, 0.0f);
		DR_F32(it, 4) = DR_F32(it, 4) - DR_LAYER_Y();
	}
	bdSetProjectionMode(1);
	bdRestoreRenderStates(DR_RSTATES);
	return 0;
}

/* 0x00111fb0  Drillers (not in of_functions.tsv; item callback of "gen")
Arguments: x, y, z, type, then by type: 2 r, g, b; 3 and 4 a float (double); 5 a light
range (int; the original pushed a float's bits), r, g, b. */
int FUN_00111fb0(struct BExtraSystem *system, void *item, void *args)
{
	char *it = (char *)item;
	va_list ap;
	int type;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	DR_F32(it, 0) = (float)va_arg(ap, double);
	DR_F32(it, 4) = (float)va_arg(ap, double);
	DR_F32(it, 8) = (float)va_arg(ap, double);
	DR_F32(it, 0xc) = 1.0f;
	type = va_arg(ap, int);
	DR_I32(it, 0x10) = type;
	if (type == 2) {
		DR_U8(it, 0x18) = (unsigned char)va_arg(ap, int);
		DR_U8(it, 0x19) = (unsigned char)va_arg(ap, int);
		DR_U8(it, 0x1a) = (unsigned char)va_arg(ap, int);
	}
	type = DR_I32(it, 0x10);
	if (type == 3)
		DR_F32(it, 0x20) = (float)va_arg(ap, double);
	if (type == 4)
		DR_F32(it, 0x20) = (float)va_arg(ap, double);
	if (type == 5) {
		DR_I32(it, 0x1c) = va_arg(ap, int);
		DR_U8(it, 0x18) = (unsigned char)va_arg(ap, int);
		DR_U8(it, 0x19) = (unsigned char)va_arg(ap, int);
		DR_U8(it, 0x1a) = (unsigned char)va_arg(ap, int);
	}
	DR_I32(it, 0x14) = 0;
	return 0;
}

/* 0x00112050  Drillers (not in of_functions.tsv; update callback of "gen"; 0x001120b0 is
one of its switch cases, not a function)
Every generator fires its effect: 1 steam, 3 a spark burst every 8th frame of 16 (and the
spark angle), 4 hsteam (each frame); 2 a corona and 5 a light (once). */
int FUN_00112050(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char *it;
	int i;

	(void)system;
	(void)arg;
	for (i = 0; i < count; i++) {
		it = (char *)items[i];
		if (DR_I32(it, 0x14) != 0)
			continue;
		switch (DR_I32(it, 0x10)) {
		case 1:
			baAddExtraItem(FX->steam, (double)DR_F32(it, 0), (double)DR_F32(it, 4),
				(double)DR_F32(it, 8));
			break;
		case 2:
			baAddExtraItem(FX->corona, (double)DR_F32(it, 0), (double)DR_F32(it, 4),
				(double)DR_F32(it, 8), (int)DR_U8(it, 0x18), (int)DR_U8(it, 0x19),
				(int)DR_U8(it, 0x1a));
			DR_I32(it, 0x14) = 1;
			break;
		case 3:
			DR_GEN_ANGLE = DR_F32(it, 0x20);
			if ((DR_FRAME & 0x10) != 0 && (DR_FRAME & 7) == 0)
				DrEffects_SparkBurst(FX, (float *)it);
			break;
		case 4:
			baAddExtraItem(FX->hsteam, (double)DR_F32(it, 0), (double)DR_F32(it, 4),
				(double)DR_F32(it, 8), (double)DR_F32(it, 0x20));
			break;
		case 5:
			baAddExtraItem(FX->light, (double)DR_F32(it, 0), (double)DR_F32(it, 4),
				(double)DR_F32(it, 8), DR_I32(it, 0x1c), (int)DR_U8(it, 0x18),
				(int)DR_U8(it, 0x19), (int)DR_U8(it, 0x1a));
			DR_I32(it, 0x14) = 1;
			break;
		}
	}
	return 0;
}

/* 0x001121a0  Drillers (not in of_functions.tsv; item callback of "suck")
Arguments: x, y, z (doubled), target x, y, z, r, g, b. */
int FUN_001121a0(struct BExtraSystem *system, void *item, void *args)
{
	char *it = (char *)item;
	va_list ap;
	double v;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	v = va_arg(ap, double);
	DR_F32(it, 0) = (float)(v + v);
	v = va_arg(ap, double);
	DR_F32(it, 4) = (float)(v + v);
	DR_F32(it, 0xc) = 1.0f;
	v = va_arg(ap, double);
	DR_F32(it, 8) = (float)(v + v);
	DR_F32(it, 0x10) = (float)va_arg(ap, double);
	DR_F32(it, 0x14) = (float)va_arg(ap, double);
	DR_F32(it, 0x1c) = 1.0f;
	DR_F32(it, 0x18) = (float)va_arg(ap, double);
	DR_U8(it, 0x20) = (unsigned char)va_arg(ap, int);
	DR_U8(it, 0x21) = (unsigned char)va_arg(ap, int);
	DR_U8(it, 0x22) = (unsigned char)va_arg(ap, int);
	return 0;
}

/* 0x00112210  Drillers (not in of_functions.tsv; update callback of "suck")
Each sprite moves an eighth of the way to its target; it goes when within 16. */
int FUN_00112210(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *p;
	int i = 0;

	(void)arg;
	while (i < count) {
		p = (float *)items[i];
		if (16.0 <= fabsf(p[0] - p[4]) || 16.0 <= fabsf(p[1] - p[5]) ||
				16.0 <= fabsf(p[2] - p[6])) {
			i++;
			p[0] = p[0] - (p[0] - p[4]) * 0.125f;
			p[1] = p[1] - (p[1] - p[5]) * 0.125f;
			p[2] = p[2] - (p[2] - p[6]) * 0.125f;
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x001122c0  Drillers (not in of_functions.tsv; draw callback of "suck") */
int FUN_001122c0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char *it;
	int i;

	(void)system;
	(void)arg;
	bdSetWorldIdentity();
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetProjectionMode(1);
	for (i = 0; i < count; i++) {
		it = (char *)items[i];
		bdDrawSprite3D((float *)it, 25.0f, 25.0f, 0.0f,
			(struct bdTexture *)(uintptr_t)DR_TEX_GLOW, 0, 0, DR_U8(it, 0x20), DR_U8(it, 0x21),
			DR_U8(it, 0x22), 0xff, NULL, 0.0f);
	}
	bdSetProjectionMode(1);
	bdRestoreRenderStates(DR_RSTATES);
	return 0;
}

/* 0x00112360  Drillers (not in of_functions.tsv; update callback of "ring")
A ring lives 17 ticks. */
int FUN_00112360(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char *it;
	int i = 0;

	(void)arg;
	while (i < count) {
		it = (char *)items[i];
		if (DR_I32(it, 0x18) < 0x11) {
			DR_I32(it, 0x18) += 1;
			i++;
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x001123a0  Drillers (not in of_functions.tsv; draw callback of "dust")
A growing, turning, fading grey sprite (brightness by type: 1 0x30, 2 0x18, else 0x60). */
int FUN_001123a0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char *it;
	int r;
	int g;
	int b;
	int age;
	float size;
	int i;

	(void)system;
	(void)arg;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetProjectionMode(1);
	bdSetWorldIdentity();
	for (i = 0; i < count; i++) {
		it = (char *)items[i];
		if (DR_I32(it, 0x14) == 1) {
			b = g = r = 0x30;
		} else if (DR_I32(it, 0x14) == 2) {
			b = g = r = 0x18;
		} else {
			b = g = r = 0x60;
		}
		age = DR_I32(it, 0x10);
		r = r + age * -8;
		g = g + age * -8;
		b = b + age * -8;
		if (r < 0)
			r = 0;
		if (g < 0)
			g = 0;
		if (b < 0)
			b = 0;
		size = (float)(age * 4) + 40.0f;
		bdDrawSprite3D((float *)it, size, size, (float)age * 5.0f * DR_DEG,
			(struct bdTexture *)(uintptr_t)DR_TEX_GLOW, 0, 0, r, g, b, 0xff, NULL, 0.0f);
	}
	bdSetProjectionMode(1);
	bdRestoreRenderStates(DR_RSTATES);
	return 0;
}

/* 0x001124c0  Drillers (not in of_functions.tsv; item callback of "scorch")
Arguments: x, y, z. */
int FUN_001124c0(struct BExtraSystem *system, void *item, void *args)
{
	char *it = (char *)item;
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	DR_F32(it, 0) = (float)va_arg(ap, double);
	DR_F32(it, 4) = (float)va_arg(ap, double);
	DR_F32(it, 8) = (float)va_arg(ap, double);
	DR_F32(it, 0xc) = 1.0f;
	DR_I32(it, 0x10) = 0;
	return 0;
}

/* 0x001124f0  Drillers (not in of_functions.tsv; update callback of "corona" and "scorch")
The age of every item. */
int FUN_001124f0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	int i;

	(void)system;
	(void)arg;
	for (i = 0; i < count; i++)
		DR_I32(items[i], 0x10) += 1;
	return 0;
}

/* 0x00112520  Drillers (not in of_functions.tsv; draw callback of "scorch")
A 1000-unit square on the ground, brighter with age (to 80 / 3), additive. */
int FUN_00112520(struct BExtraSystem *system, int count, void **items, void *arg)
{
	bdVertex *v = DR_VTX_SCORCH;
	float *p;
	unsigned int colour;
	int c;
	int i;

	(void)system;
	(void)arg;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 3, 0);
	bdSetRenderState(5, 0, 0);
	bdSetWorldIdentity();
	bdSetTexture(0, (struct bdTexture *)(uintptr_t)DR_TEX_SCORCH);
	for (i = 0; i < count; i++) {
		p = (float *)items[i];
		c = DR_I32(p, 0x10) * 10;
		if (0x50 < c)
			c = 0x50;
		c = c / 3;
		v[0].x = p[0] + 500.0f;
		v[0].y = p[1];
		v[0].z = p[2] - 500.0f;
		v[1].x = p[0] - 500.0f;
		v[1].y = p[1];
		v[1].z = p[2] - 500.0f;
		v[2].x = p[0] + 500.0f;
		v[2].y = p[1];
		v[2].z = p[2] + 500.0f;
		v[3].x = p[0] - 500.0f;
		v[3].y = p[1];
		v[3].z = p[2] + 500.0f;
		colour = DR_RGBA(c, c, c, 255);
		v[0].color = colour;
		v[0].u = 0.0f;
		v[0].v = 0.0f;
		v[1].u = 1.0f;
		v[1].v = 0.0f;
		v[2].u = 0.0f;
		v[2].v = 1.0f;
		v[3].u = 1.0f;
		v[3].v = 1.0f;
		v[1].color = colour;
		v[2].color = colour;
		v[3].color = colour;
		bdDrawPrimitiveFVF(4, v, 4, 0x152);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
	return 0;
}

/* the colour of a worm trail point (two formulas per point, by the worm's type) */
static unsigned int dr_worm_colour(int blue, int green)
{
	int red = blue;

	if (red < 0)
		red = 0;
	if (green < 0)
		green = 0;
	if (blue < 0)
		blue = 0;
	return DAT_002bd640[blue] | DAT_002bc490[green] | DAT_002bce18[red] | DAT_002bcc94;
}

/* 0x00112700  Drillers (not in of_functions.tsv; draw callback of "worm")
Each worm is a strip 10 units high along its 16 trail points, green fading to the tail
(type 1 dimmer; type 2 fades with age); 1600 higher in state 7. Only the first 28 of the
32 vertices are drawn. */
int FUN_00112700(struct BExtraSystem *system, int count, void **items, void *arg)
{
	bdVertex *v;
	char *it;
	float lift;
	int count2;
	int i;
	int k;

	(void)system;
	(void)arg;
	lift = 0.0f;
	if (g_DrState == 7)
		lift = 1600.0f;
	bdSetWorldIdentity();
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdSetTexture(0, NULL);
	for (i = 0; i < count; i++) {
		it = (char *)items[i];
		v = DR_VTX_WORM;
		count2 = 0;
		for (k = 0; k < 15; k++) {
			float *pk = DR_FP(it, (k + 2) * 0x10);
			int age = DR_I32(it, 0x118);
			unsigned int colour;

			if (k == 0) {
				v[0].x = DR_F32(it, 0x10);
				v[0].y = lift + DR_F32(it, 0x14) + 5.0f;
				v[0].z = DR_F32(it, 0x18);
				v[1].x = DR_F32(it, 0x10);
				v[1].y = (DR_F32(it, 0x14) - 5.0f) + lift;
				v[1].z = DR_F32(it, 0x18);
			}
			v[2].x = pk[0];
			v[2].y = lift + pk[1] + 5.0f;
			v[2].z = pk[2];
			v[3].x = pk[0];
			v[3].y = (pk[1] - 5.0f) + lift;
			v[3].z = pk[2];
			if (DR_I32(it, 0x114) == 1)
				colour = dr_worm_colour(k * -4 + 0x20, (k * -3 + 0x30) * 2);
			else
				colour = dr_worm_colour((k * -8 + 0x40) - age, (age + k * 4) * -2 + 0x7f);
			v[0].color = colour;
			v[1].color = colour;
			if (DR_I32(it, 0x114) == 1)
				colour = dr_worm_colour(k * -8 + 0x18, (k * -3 + 0x15) * 4);
			else
				colour = dr_worm_colour(((k * -4 + 0x1c) - age) * 2, (age + k * 2) * -4 + 0x77);
			v[2].color = colour;
			v[3].color = colour;
			v += 2;
			if (k < 14)
				count2 += 2;
		}
		bdDrawPrimitiveFVF(4, DR_VTX_WORM, count2, 0x152);
	}
	bdPopRenderState(6);
	bdPopRenderState(10);
	return 0;
}

/* ---------- the effects object */

/* 0x00112940  Drillers (gap; CDrillers_Effects.cpp inferred)
The "gen" system: 128 generators of 0x24 bytes. */
void DrEffects_CreateGen(DrEffects *self)
{
	self->gen = baCreateExtraSystem("gen", (int (*)())FUN_00111fb0, (int (*)())FUN_00112050,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_001d5950, (int (*)())FUN_001d5950, 0x24, 0x80, 1,
		0);
}

/* 0x00112980  Drillers (gap; CDrillers_Effects.cpp inferred)
A floating score ("1", "3", "5" or "x2" by type) at x, 336, z in the colour of the player's
character. */
void DrFx_AddScoreFloaty(float x, float z, char type, char player)
{
	char text[16];
	int character;

	text[0] = '\0';
	switch (type) {
	case 0:
		strcpy(text, "1");
		break;
	case 1:
		strcpy(text, "3");
		break;
	case 2:
		strcpy(text, "5");
		break;
	case 3:
		strcpy(text, "x2");
		break;
	}
	character = DAT_002baed4[player * 10];
	baAddExtraItem(DR_VAR(struct BExtraSystem *, 0x003014e8u), DR_FONT_FLOATY, (double)x, 336.0,
		(double)z, (int)DAT_00290718[character * 0x60], (int)DAT_00290718[character * 0x60 + 1],
		(int)DAT_00290718[character * 0x60 + 2], text);
}

/* 0x00112a40  Drillers (gap; CDrillers_Effects.cpp inferred)
The score numbers rise 50 per tick and go above 3000. */
void DrEffects_UpdateMarkers(DrEffects *self)
{
	int i;

	for (i = 0; i < 22; i++) {
		if (self->markers[i].active == 1) {
			self->markers[i].pos[1] = self->markers[i].pos[1] + 50.0f;
			if (3000.0f < self->markers[i].pos[1])
				self->markers[i].active = 0;
		}
	}
}

/* 0x00112a70  Drillers (not in of_functions.tsv)
The screen flash: a full-screen quad of grey `flash`, which drops by 8 per frame. */
void FUN_00112a70(void *fx)
{
	DrEffects *self = (DrEffects *)fx;
	bdVertex *v = DR_VTX_FLASH;
	unsigned int colour;

	bdSetProjectionMode(0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(7, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetTexture(0, NULL);
	v[0].x = -320.0f;
	v[0].y = -240.0f;
	v[0].z = 0.0f;
	v[1].x = 320.0f;
	v[1].y = -240.0f;
	v[1].z = 0.0f;
	v[2].x = -320.0f;
	v[2].y = 240.0f;
	v[2].z = 0.0f;
	v[3].x = 320.0f;
	v[3].y = 240.0f;
	v[3].z = 0.0f;
	colour = DR_RGBA(self->flash, self->flash, self->flash, 255);
	v[0].color = colour;
	v[1].color = colour;
	v[2].color = colour;
	v[3].color = colour;
	bdDrawPrimitiveFVF(4, v, 4, 0x152);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(10, 0, 0);
	self->flash -= 8;
	if (self->flash < 0)
		self->flash = 0;
}

/* 0x00112c20  Drillers (gap; CDrillers_Effects.cpp inferred)
A 128 x 128 render target ("vid screen"). */
void DrEffects_CreateScreen(DrEffects *self)
{
	int colourDepth;
	int zDepth;

	self->screen = bdCreateRenderTarget(0x80, 0x80, 0x20, 0x20, 0);
	if (self->screen == NULL)
		bkPrintf("*********ERROR - COULD NOT CREATE VID SCREEN\n");
	bdRenderTargetGetInfo(self->screen, &self->screenWidth, &self->screenHeight, &colourDepth,
		&zDepth);
}

/* 0x00112c90  Drillers (gap; CDrillers_Effects.cpp inferred) */
void DrEffects_DeleteScreen(DrEffects *self)
{
	bdSetTexture(0, NULL);
	bdDeleteRenderTarget(self->screen);
}

/* 0x00112cb0  Drillers (gap; CDrillers_Effects.cpp inferred)
Rumbles the pad of `player`: 0 a short buzz, 1 a long rumble, 2 a medium rumble. */
void DrFx_Rumble(int player, int kind)
{
	if (kind == 0) {
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 0, 0x7fff, 600, 0, 10, 0, 0);
	} else if (kind == 1) {
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 1, 0x7fff, 2000, 10, 0x3c, 0,
			4);
	} else if (kind == 2) {
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 1, 0x7fff, 1000, -1, 0x3c, 0,
			0);
	}
}

/* 0x00112d40  Drillers (gap; CDrillers_Effects.cpp inferred)
Constructor (core's static constructor #20). */
DrEffects *DrEffects_Ctor(DrEffects *self)
{
	int i;

	self->vtable = vtable_0026c124;
	for (i = 0; i < 4; i++)
		DrFxGroup_Ctor(&self->groups[i]);
	for (i = 0; i < 8; i++)
		DrSparks_Ctor(&self->sparks[i]);
	return self;
}

/* 0x00112dc0  Drillers (gap; CDrillers_Effects.cpp inferred) */
DrEffects *DrEffects_DeletingDtor(DrEffects *self, unsigned int flags)
{
	DrEffects_Dtor(self);
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x00112de0  Drillers (gap; CDrillers_Effects.cpp inferred)
Starts a burst group at `pos` in the colour of `player` (sparks and dust; the smoke is only
switched on) and rumbles the player's pad. */
void DrFxGroup_Start(DrFxGroup *self, float *pos, int player)
{
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	CParticleSystem_SetPosition(&self->sparks.h, pos);
	CParticleSystem_SetPosition(&self->dust.h, pos);
	CParticleSystem_SetPosition(&self->smoke.h, pos);
	DrPs_Burst(&self->sparks, player);
	DrPs_Burst(&self->dust, player);
	self->sparks.p.flags |= 1;
	self->smoke.p.flags |= 1;
	self->dust.p.flags |= 1;
	self->flags |= 3;
	self->time = 0.0f;
	CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 1, 0x7fff, 2000, 10, 0x3c, 0, 4);
}

/* 0x00112ea0  Drillers (not in of_functions.tsv; update callback of "steam")
Steam rises 10 per tick on a small circle (2 units per tick) for 201 ticks. */
int FUN_00112ea0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *p;
	int i = 0;

	(void)arg;
	while (i < count) {
		p = (float *)items[i];
		if (DR_I32(p, 0x14) < 0xc9) {
			DR_I32(p, 0x10) += 1;
			if (DR_I32(p, 0x10) == 0x168)
				DR_I32(p, 0x10) = 0;
			DR_I32(p, 0x14) += 1;
			i++;
			p[0] = sinf((float)DR_I32(p, 0x10) * DR_DEG) * 2.0f + p[0];
			p[1] = p[1] + 10.0f;
			p[2] = cosf((float)DR_I32(p, 0x10) * DR_DEG) * 2.0f + p[2];
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x00112f30  Drillers (not in of_functions.tsv; item callback of "hsteam")
Arguments: x, y, z, heading (degrees). The position is jittered by -128..127. */
int FUN_00112f30(struct BExtraSystem *system, void *item, void *args)
{
	float *p = (float *)item;
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	p[0] = (float)va_arg(ap, double);
	p[1] = (float)va_arg(ap, double);
	p[3] = 1.0f;
	p[2] = (float)va_arg(ap, double);
	p[7] = (float)va_arg(ap, double);
	p[0] = (float)(int)(((unsigned int)dr_rand() & 0xff) - 0x80) + p[0];
	p[1] = (float)(int)(((unsigned int)dr_rand() & 0xff) - 0x80) + p[1];
	p[2] = (float)(int)(((unsigned int)dr_rand() & 0xff) - 0x80) + p[2];
	DR_I32(p, 0x10) = dr_rand() & 0xff;
	DR_I32(p, 0x14) = 0x40;
	DR_I32(p, 0x18) = 0;
	return 0;
}

/* 0x00113030  Drillers (not in of_functions.tsv; update callback of "hsteam")
Rises 10 per tick and drifts along its heading, slowing down; 201 ticks. */
int FUN_00113030(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *p;
	int i = 0;

	(void)arg;
	while (i < count) {
		p = (float *)items[i];
		if (DR_I32(p, 0x18) < 0xc9) {
			if (0 < DR_I32(p, 0x14))
				DR_I32(p, 0x14) -= 1;
			DR_I32(p, 0x10) += 1;
			if (DR_I32(p, 0x10) == 0x168)
				DR_I32(p, 0x10) = 0;
			DR_I32(p, 0x18) += 1;
			i++;
			p[0] = sinf(p[7] * DR_DEG) * (float)DR_I32(p, 0x14) * 0.5f + p[0];
			p[1] = p[1] + 10.0f;
			p[2] = cosf(p[7] * DR_DEG) * (float)DR_I32(p, 0x14) * 0.5f + p[2];
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x001130d0  Drillers (not in of_functions.tsv; item callback of "corona")
Arguments: x, y, z, r, g, b. A random start angle. */
int FUN_001130d0(struct BExtraSystem *system, void *item, void *args)
{
	char *it = (char *)item;
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	DR_F32(it, 0) = (float)va_arg(ap, double);
	DR_F32(it, 4) = (float)va_arg(ap, double);
	DR_F32(it, 8) = (float)va_arg(ap, double);
	DR_F32(it, 0xc) = 1.0f;
	DR_U8(it, 0x18) = (unsigned char)va_arg(ap, int);
	DR_U8(it, 0x19) = (unsigned char)va_arg(ap, int);
	DR_U8(it, 0x1a) = (unsigned char)va_arg(ap, int);
	DR_I32(it, 0x14) = 0;
	DR_I32(it, 0x10) = dr_rand() & 0xff;
	return 0;
}

/* 0x00113140  Drillers (not in of_functions.tsv; item callback of "ring")
Arguments: x, y, z, r, g, b, angle (radians). In variants 3 and 4 the ring stands on a
circle of radius 3000 at that angle. */
int FUN_00113140(struct BExtraSystem *system, void *item, void *args)
{
	char *it = (char *)item;
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	DR_F32(it, 0) = (float)va_arg(ap, double);
	DR_F32(it, 4) = (float)va_arg(ap, double);
	DR_F32(it, 8) = (float)va_arg(ap, double);
	DR_F32(it, 0xc) = 1.0f;
	DR_U8(it, 0x10) = (unsigned char)va_arg(ap, int);
	DR_U8(it, 0x11) = (unsigned char)va_arg(ap, int);
	DR_U8(it, 0x12) = (unsigned char)va_arg(ap, int);
	DR_U8(it, 0x13) = 0x40;
	DR_I32(it, 0x18) = 0;
	DR_F32(it, 0x14) = (float)va_arg(ap, double);
	if (2 < g_DrVariant) {
		DR_F32(it, 0) = sinf(DR_F32(it, 0x14)) * 3000.0f;
		DR_F32(it, 8) = cosf(DR_F32(it, 0x14)) * 3000.0f;
	}
	return 0;
}

/* 0x001131c0  Drillers (not in of_functions.tsv; draw callback of "ring")
A ring that grows (200 + 6 per tick) and fades: in variants 1 and 2 a flat fan around the
item, in 3 and 4 an upright turning wheel of 18 triangles facing the angle. */
int FUN_001131c0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	bdVertex *v;
	float *p;
	float m1[16];
	float m2[16];
	float m3[16];
	float radius;
	float turn;
	float a;
	unsigned int inner;
	unsigned int outer;
	int age;
	int r;
	int g;
	int b;
	int i;
	int k;

	(void)system;
	(void)arg;
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(g_DrVariant < 3 ? 5 : 4, g_DrVariant >= 3, 0);
	bdSetWorldIdentity();
	for (i = 0; i < count; i++) {
		p = (float *)items[i];
		age = DR_I32(p, 0x18);
		r = (int)DR_U8(p, 0x10) + age * -0x10;
		g = (int)DR_U8(p, 0x11) + age * -0x10;
		b = (int)DR_U8(p, 0x12) + age * -0x10;
		radius = 6.0f * (float)age + 200.0f;
		turn = (float)age * 10.0f;
		if (r < 0)
			r = 0;
		if (g < 0)
			g = 0;
		if (b < 0)
			b = 0;
		v = DR_VTX_RING;
		if (g_DrVariant < 3) {
			v[0].x = p[0];
			v[0].y = p[1];
			v[0].z = p[2];
			v[0].color = DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bce18[r] | DAT_002bca94;
			outer = DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0] | DAT_002bcc94;
			for (k = 0; k < 19; k++) {
				a = (float)k * 20.0f * DR_DEG;
				v[k + 1].x = sinf(a) * radius + p[0];
				v[k + 1].y = p[1];
				v[k + 1].color = outer;
				v[k + 1].z = cosf(a) * radius + p[2];
			}
			bdDrawPrimitiveFVF(5, DR_VTX_RING, 0x14, 0x152);
		} else {
			bmMatrixTranslation(m2, p[0], p[1], p[2]);
			bmMatrixRotationY(m1, p[5]);
			bmMatrixMultiplyCopy(m3, m2, m1);
			bdSetWorldMatrix(m3);
			inner = DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bce18[r] | DAT_002bcc94;
			outer = DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0] | DAT_002bcc94;
			for (k = 0; k < 0x12; k++) {
				v[0].x = 0.0f;
				v[0].y = 150.0f;
				v[0].z = 0.0f;
				v[1].z = 0.0f;
				v[2].z = 0.0f;
				a = ((float)k * 20.0f + turn) * DR_DEG;
				v[0].color = inner;
				v[1].color = outer;
				v[2].color = outer;
				v[1].x = sinf(a) * radius;
				v[1].y = cosf(a) * radius + 150.0f;
				a = ((float)(k + 1) * 20.0f + turn) * DR_DEG;
				v[2].x = sinf(a) * radius;
				v[2].y = cosf(a) * radius + 150.0f;
				v += 3;
			}
			bdDrawPrimitiveFVF(3, DR_VTX_RING, 0x36, 0x152);
		}
	}
	bdRestoreRenderStates(DR_RSTATES);
	return 0;
}

/* 0x001134f0  Drillers (not in of_functions.tsv; item callback of "dust")
Arguments: x, y, z, type. x and z are jittered by -8..7. */
int FUN_001134f0(struct BExtraSystem *system, void *item, void *args)
{
	float *p = (float *)item;
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	p[0] = (float)va_arg(ap, double);
	p[1] = (float)va_arg(ap, double);
	p[3] = 1.0f;
	p[2] = (float)va_arg(ap, double);
	DR_I32(p, 0x14) = va_arg(ap, int);
	p[0] = ((float)(dr_rand() & 0xf) - 8.0f) + p[0];
	DR_I32(p, 0x10) = 0;
	p[2] = ((float)(dr_rand() & 0xf) - 8.0f) + p[2];
	return 0;
}

/* 0x001135a0  Drillers (not in of_functions.tsv; update callback of "dust")
A puff drifts at random and rises for 17 ticks. */
int FUN_001135a0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *p;
	int i = 0;

	(void)arg;
	while (i < count) {
		p = (float *)items[i];
		if (DR_I32(p, 0x10) < 0x11) {
			p[0] = ((float)(dr_rand() & 0x20) - 16.0f) + p[0];
			p[1] = (float)(dr_rand() & 7) + p[1] + 8.0f;
			DR_I32(p, 0x10) += 1;
			i++;
			p[2] = ((float)(dr_rand() & 0x20) - 16.0f) + p[2];
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x001136a0  Drillers (gap; CDrillers_Effects.cpp inferred; item callback of "worm")
Arguments: type, then for a type other than 1: x, y, z (doubles). Type 1 starts on a random
cell, the others on the cell of x, z. A random direction; the 16 trail points start at the
head. */
int FUN_001136a0(struct BExtraSystem *system, void *item, void *args)
{
	float *p = (float *)item;
	va_list ap;
	float inv;
	int ix;
	int iz;
	int k;

	(void)system;
	ap = (va_list)args;
	DR_I32(p, 0x114) = va_arg(ap, int);
	if (DR_I32(p, 0x114) == 1) {
		ix = DR_HALF_X;
		p[1] = 0.0f;
		p[0] = (float)(dr_rand() % (DR_HALF_X * 2) - ix);
		p[2] = (float)(dr_rand() % (DR_HALF_Z * 2) - DR_HALF_Z);
		p[0] = (float)DR_CELL * p[0];
		p[2] = (float)DR_CELL * p[2];
	} else {
		p[0] = (float)va_arg(ap, double);
		p[1] = (float)va_arg(ap, double);
		p[2] = (float)va_arg(ap, double);
		inv = 1.0f / (float)DR_CELL;
		iz = DR_FTOL(inv * p[2]);
		ix = DR_FTOL(inv * p[0]);
		p[0] = (float)ix * (float)DR_CELL;
		p[2] = (float)(DR_CELL * iz);
	}
	p[3] = 1.0f;
	DR_I32(p, 0x118) = 0;
	DR_I32(p, 0x110) = dr_rand() & 3;
	for (k = 0; k < 16; k++) {
		p[4 + k * 4] = p[0];
		p[5 + k * 4] = p[1];
		p[6 + k * 4] = p[2];
		p[7 + k * 4] = p[3];
	}
	return 0;
}

/* 0x00113810  Drillers (gap; CDrillers_Effects.cpp inferred; update callback of "worm")
A worm moves 48 per tick in its direction, its trail following; it turns back at the map's
edge and turns left or right at random on cell corners. Type 2 lives 33 ticks. */
int FUN_00113810(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *p;
	unsigned int dir;
	int cell;
	int k;
	int i = 0;

	(void)arg;
	while (i < count) {
		p = (float *)items[i];
		if (DR_I32(p, 0x114) == 2 && 0x20 < DR_I32(p, 0x118)) {
			baDeleteExtraItem(system, i);
			count--;
			continue;
		}
		for (k = 14; k >= 0; k--) {
			p[8 + k * 4] = p[4 + k * 4];
			p[9 + k * 4] = p[5 + k * 4];
			p[10 + k * 4] = p[6 + k * 4];
		}
		p[5] = p[1];
		p[4] = p[0];
		p[6] = p[2];
		switch (DR_I32(p, 0x110)) {
		case 0:
			p[0] = p[0] + 48.0f;
			break;
		case 1:
			p[2] = p[2] + 48.0f;
			break;
		case 2:
			p[0] = p[0] - 48.0f;
			break;
		case 3:
			p[2] = p[2] - 48.0f;
			break;
		}
		if ((float)(DR_CELL * DR_HALF_X) < fabsf(p[0]) ||
				(float)(DR_HALF_Z * DR_CELL) < fabsf(p[2]))
			DR_I32(p, 0x110) = (DR_I32(p, 0x110) - 2) & 3;
		cell = DR_CELL;
		if (DR_FTOL(p[0]) % cell == 0 && DR_FTOL(p[2]) % cell == 0) {
			if ((dr_rand() & 1) == 0)
				dir = (unsigned int)(DR_I32(p, 0x110) - 1);
			else
				dir = (unsigned int)(DR_I32(p, 0x110) + 1);
			DR_I32(p, 0x110) = (int)(dir & 3);
		}
		DR_I32(p, 0x118) += 1;
		i++;
	}
	return 0;
}

/* 0x001139a0  Drillers (gap; CDrillers_Effects.cpp inferred)
Initialise: the tokens are collected, the score numbers go, the flash is off; the extra
systems, the rain (512 drops) and the floating scores (64; rise 12, scale 0.3, life 2 s;
Erodeo rise 8, scale 0.2, life 1 s; Quantum Snare life 1 s). */
void DrEffects_Init(DrEffects *self)
{
	int i;

	for (i = 0; i < 22; i++) {
		if (self->tokens[i].active != 0)
			CToken_Collect(self->tokens[i].token);
		self->markers[i].active = 0;
		self->tokens[i].active = 0;
	}
	self->flash = 0;
	self->suck = baCreateExtraSystem("suck", (int (*)())FUN_001121a0, (int (*)())FUN_00112210,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_001122c0, (int (*)())FUN_001d5950, 0x24, 0x200, 1,
		0);
	self->steam = baCreateExtraSystem("steam", (int (*)())FUN_00149650, (int (*)())FUN_00112ea0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00111930, (int (*)())FUN_001d5950, 0x18, 0x400, 1,
		0);
	self->corona = baCreateExtraSystem("corona", (int (*)())FUN_001130d0,
		(int (*)())FUN_001124f0, (int (*)())FUN_000a6fa0, (int (*)())FUN_00111e30,
		(int (*)())FUN_001d5950, 0x1c, 0x40, 1, 0);
	self->hsteam = baCreateExtraSystem("hsteam", (int (*)())FUN_00112f30,
		(int (*)())FUN_00113030, (int (*)())FUN_000a6fa0, (int (*)())FUN_00111cc0,
		(int (*)())FUN_001d5950, 0x20, 0x400, 1, 0);
	self->light = baCreateExtraSystem("light", (int (*)())FUN_00111b10, (int (*)())FUN_00111bf0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_001d5950, (int (*)())FUN_00111ab0, 0x20, 8, 1, 0);
	self->ring = baCreateExtraSystem("hsteam", (int (*)())FUN_00113140, (int (*)())FUN_00112360,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_001131c0, (int (*)())FUN_001d5950, 0x1c, 0x20, 1,
		0);
	self->dust = baCreateExtraSystem("dust", (int (*)())FUN_001134f0, (int (*)())FUN_001135a0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_001123a0, (int (*)())FUN_001d5950, 0x18, 0x80, 1,
		0);
	self->scorch = baCreateExtraSystem("scorch", (int (*)())FUN_001124c0,
		(int (*)())FUN_001124f0, (int (*)())FUN_000a6fa0, (int (*)())FUN_00112520,
		(int (*)())FUN_001d5950, 0x14, 0x20, 1, 0);
	self->worm = baCreateExtraSystem("worm", (int (*)())FUN_001136a0, (int (*)())FUN_00113810,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00112700, (int (*)())FUN_001d5950, 0x11c, 0x100,
		1, 0);
	DrRain_Create(&self->rain, 0x200);
	BCarFloatyScore_Init((struct BCarFloatyScore *)self->floaty, 0x40);
	DR_FX_FLOATY_SCALE(self) = 0.3f;
	DR_FX_FLOATY_RISE(self) = 12.0f;
	if (g_DrVariant == 4) {
		DR_FX_FLOATY_LIFE(self) = 1.0f;
	} else if (g_DrVariant == 2) {
		DR_FX_FLOATY_LIFE(self) = 1.0f;
		DR_FX_FLOATY_SCALE(self) = 0.2f;
		DR_FX_FLOATY_RISE(self) = 8.0f;
	} else {
		DR_FX_FLOATY_LIFE(self) = 2.0f;
	}
	DR_FX_FLOATY_LIFE(self) = DR_FX_FLOATY_LIFE(self) * 60.0f;
	DR_FX_FLOATY_RISE(self) = DR_FX_FLOATY_RISE(self) * 60.0f;
}

/* 0x00113c60  Drillers (not in of_functions.tsv)
Draws the rising score numbers: text in the player's colour (doubled for "x2"), growing and
fading with height, facing the camera. */
void FUN_00113c60(void *self)
{
	DrEffects *fx = (DrEffects *)self;
	DrScoreMarker *mk;
	unsigned short wide[40];
	char text[40];
	float rot[16];
	float scale[16];
	float sr[16];
	float trans[16];
	float world[16];
	float base = 0.0f;
	float t;
	float s;
	float width;
	float height;
	unsigned char alpha;
	int len;
	int r;
	int g;
	int b;
	int i;

	text[0] = '\0';
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(1, 1, 0);
	bdSetWorldIdentity();
	for (i = 0; i < 22; i++) {
		mk = &fx->markers[i];
		if (mk->active != 1)
			continue;
		switch (mk->type) {
		case 0:
			strcpy(text, "1");
			base = 4.0f;
			break;
		case 1:
			strcpy(text, "3");
			base = 4.0f;
			break;
		case 2:
			strcpy(text, "5");
			base = 4.0f;
			break;
		case 3:
			strcpy(text, "x2");
			base = 8.0f;
			break;
		}
		t = mk->pos[1] - 340.0f;
		alpha = (unsigned char)DR_FTOL((250.0f - 0.093984962f * t) * 0.3f);
		s = t * 0.0057142857f + base;
		if (g_DrVariant == 2) {
			bmMatrixRotationZYX(rot,
				atan2f(DR_CAM_EYE[2] - DR_CAM_TARGET[2], DR_CAM_EYE[1] - DR_CAM_TARGET[1]),
				atan2f(DR_CAM_EYE[0] - DR_CAM_TARGET[0], DR_CAM_EYE[1] - DR_CAM_TARGET[1]) +
					3.1415927f,
				0.0f);
		} else {
			bmMatrixRotationXZY(rot, 0.0f, (DR_CAM_HEADING + 180.0f) * DR_DEG, 0.0f);
		}
		bmMatrixScaling(scale, s, s, s);
		bmMatrixMultiplyCopy(sr, scale, rot);
		bmMatrixTranslation(trans, mk->pos[0], mk->pos[1], s + mk->pos[2]);
		bmMatrixMultiplyCopy(world, trans, sr);
		bdSetWorldMatrix(world);
		bkStringToWide(wide, text);
		len = (int)strlen(text);
		bdGetStringSize(DR_FONT_MARKER, wide, len, &width, &height);
		if (mk->type == 3) {
			r = DR_PLAYER_R(mk->player) << 1;
			g = DR_PLAYER_G(mk->player) << 1;
			b = DR_PLAYER_B(mk->player) << 1;
		} else {
			r = DR_PLAYER_R(mk->player);
			g = DR_PLAYER_G(mk->player);
			b = DR_PLAYER_B(mk->player);
		}
		bdPrintFontLite(DR_FONT_MARKER, wide, len, width * -0.5f, height * -0.5f, r, g, b,
			alpha, NULL, NULL);
	}
	bdSetTexture(0, NULL);
	bdSetWorldIdentity();
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(6, 0, 0);
}

/* a new token in slot `t`: a CToken of the slot's type at the centre of its cell (size 1.3
   in Quantum Snare, else 0.8); `line` is the allocation's source line */
static struct CToken *dr_new_token(DrTokenSlot *t, int lineSnare, int lineOther)
{
	float pos[4];
	struct CToken *token;

	pos[1] = 0.0f;
	pos[3] = 1.0f;
	pos[0] = (float)DR_CELL * t->x + (float)(DR_CELL / 2);
	pos[2] = (float)DR_CELL * t->z + (float)(DR_CELL / 2);
	if (g_DrVariant == 4) {
		token = (struct CToken *)DR_NEW(0x290, DR_FILE_EFFECTS, lineSnare, 0x1001);
		if (token != NULL)
			return CToken_Init(token, pos, (int)t->type, 1.3f, NULL);
	} else {
		token = (struct CToken *)DR_NEW(0x290, DR_FILE_EFFECTS, lineOther, 0x1001);
		if (token != NULL)
			return CToken_Init(token, pos, (int)t->type, 0.8f, NULL);
	}
	return NULL;
}

/* 0x00113fd0  Drillers/CDrillers_Effects.cpp
A token of random type at cell x, z (in the first free slot) that flies off: a random
heading and a speed of 16-31. */
void DrEffects_AddTokenAt(DrEffects *self, float x, float z)
{
	DrTokenSlot *t;
	int i;

	for (i = 0; self->tokens[i].active != 0; i++) {
		if (0x15 < i + 1)
			return;
	}
	t = &self->tokens[i];
	t->x = x;
	t->z = z;
	t->bounce = 0.0f;
	t->bounceSpeed = 1.0f;
	t->bounces = 9.0f;
	t->type = (signed char)(dr_rand() % 3);
	t->y = 80.0f;
	t->active = 1;
	t->spin = (float)(dr_rand() & 0x1ff);
	t->speed = (float)((dr_rand() & 0xf) + 0x10);
	t->travelled = 0.0f;
	t->heading = (float)(dr_rand() & 0x7fff);
	t->token = dr_new_token(t, 0x7b3, 0x7b5);
}

/* 0x00114230  Drillers/CDrillers_Effects.cpp
A token of random type on a random cell (variants 3 and 4: within 0.8 of the map) in the
first free slot below the token limit; not on a cell that is drilled out (height below
0x400, variants 1 and 2) or that holds another slot's token. */
void DrEffects_AddRandomToken(DrEffects *self)
{
	DrTokenSlot *t;
	char *column;
	int ix;
	int iz;
	int i;
	int j;

	if (DR_MAX_TOKENS <= 0.0f)
		return;
	for (i = 0; self->tokens[i].active != 0; ) {
		i++;
		if (DR_MAX_TOKENS <= (float)i)
			return;
	}
	t = &self->tokens[i];
	t->type = (signed char)(dr_rand() % 3);
	if (g_DrVariant < 3) {
		t->x = (float)(dr_rand() % (DR_HALF_X * 2 - 1) - DR_HALF_X);
		t->z = (float)(dr_rand() % (DR_HALF_Z * 2 - 1) - DR_HALF_Z);
	} else {
		t->x = (float)(dr_rand() % (DR_HALF_X * 2 - 1) - DR_HALF_X) * 0.8f;
		t->z = (float)(dr_rand() % (DR_HALF_Z * 2 - 1) - DR_HALF_Z) * 0.8f;
	}
	t->y = 80.0f;
	if (g_DrVariant < 3) {
		ix = DR_FTOL(t->x) + DR_HALF_X;
		iz = DR_FTOL(t->z);
		column = (char *)(uintptr_t)DR_MAP_COLUMN(ix);
		if (DR_I16(column, 0x2d0 + (iz + DR_HALF_Z) * 0x2d8) < 0x400)
			return;
	}
	for (j = 0; j < 22; j++) {
		if (j != i && t->x == self->tokens[j].x && t->z == self->tokens[j].z)
			return;
	}
	t->speed = 0.0f;
	t->bounce = 0.0f;
	t->bounceSpeed = 1.0f;
	t->bounces = 9.0f;
	t->active = 1;
	t->spin = (float)(dr_rand() & 0x1ff);
	t->token = dr_new_token(t, 0x7f1, 0x7f3);
}

/* 0x001145d0  Drillers (gap; CDrillers_Effects.cpp inferred)
Deletes the CTokens that no slot holds; then every token bounces (a resting token) or
slides along its heading while inside a radius of 34 cells, slowing down, and the CToken
follows its slot. */
void DrEffects_UpdateTokens(DrEffects *self)
{
	DrTokenSlot *t;
	void *token;
	float v;
	int found;
	int i;

	for (token = CToken_Next(NULL); token != NULL; token = CToken_Next(token)) {
		found = 0;
		for (i = 0; i < 0x16; i++) {
			if (self->tokens[i].active != 0 && self->tokens[i].token == token) {
				found = 1;
				i = 0x16;
			}
		}
		if (!found) {
			CToken_Delete((struct CToken *)token);
			token = NULL;
		}
	}
	for (i = 0; i < 0x16; i++) {
		t = &self->tokens[i];
		if (t->active == 0)
			continue;
		if (t->speed == 0.0f) {
			if (t->bounces < 4.0f) {
				v = t->bounce;
				t->bounce = t->bounceSpeed + v;
				if (t->y < 80.0f) {
					t->bounce = (t->bounceSpeed + v) * -0.6f;
					t->bounceSpeed = t->bounceSpeed + 5.0f;
					t->bounces = t->bounces + 1.0f;
				}
			}
		} else {
			if (4.0f <= t->speed) {
				if (sqrtf(t->x * t->x + t->z * t->z) < 34.0f) {
					t->x = (sinf(t->heading * DR_DEG) * t->speed) / (float)DR_CELL + t->x;
					t->z = (cosf(t->heading * DR_DEG) * t->speed) / (float)DR_CELL + t->z;
				}
				t->travelled = t->travelled + 16.0f;
				t->speed = t->speed - t->speed * 0.015625f;
			}
			if (t->speed < 16.0f)
				t->bounces = 4.0f;
		}
		DR_F32(t->token, 0x180) = (float)DR_CELL * t->x + (float)(DR_CELL / 2);
		DR_F32(t->token, 0x188) = (float)DR_CELL * t->z + (float)(DR_CELL / 2);
		DR_F32(t->token, 0xd0) = DR_F32(t->token, 0x180);
		DR_F32(t->token, 0xd8) = DR_F32(t->token, 0x188);
		t->spin = t->spin + 16.0f;
	}
}

/* 0x001147d0  Drillers (not in of_functions.tsv)
A dark disc of radius 100, 10 above x, y, z (a shadow): a fan of 11 rim vertices. */
void FUN_001147d0(float x, float y, float z)
{
	bdVertex *v = DR_VTX_CIRCLE;
	unsigned int rim;
	float a;
	int k;

	y = y + 10.0f;
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 0, 0);
	bdPushRenderState(5, 0, 0);
	v[0].x = x;
	v[0].z = z;
	v[0].color = DAT_002bcc94 | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
	rim = DAT_002bc898[0] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
	v[0].y = y;
	for (k = 0; k < 11; k++) {
		v[k + 1].y = y;
		v[k + 1].color = rim;
		a = (float)k * 36.0f * DR_DEG;
		v[k + 1].x = sinf(a) * 100.0f + x;
		v[k + 1].z = cosf(a) * 100.0f + z;
	}
	bdDrawPrimitiveFVF(5, DR_VTX_CIRCLE, 0xc, 0x152);
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(5);
}

/* 0x00149650  (hopper's range; not in of_functions.tsv) item callback of "steam", which
this module's DrEffects_Init uses. A weak copy so the image links while the owner of
0x00149650 has not defined it; the owner's definition replaces it.
Arguments: x, y, z; the position is jittered by -128..127, a random start angle. */
__attribute__((weak)) int FUN_00149650(struct BExtraSystem *system, void *item, void *args)
{
	float *p = (float *)item;
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	p[0] = (float)va_arg(ap, double);
	p[1] = (float)va_arg(ap, double);
	p[3] = 1.0f;
	p[2] = (float)va_arg(ap, double);
	p[0] = (float)(int)(((unsigned int)dr_rand() & 0xff) - 0x80) + p[0];
	p[1] = (float)(int)(((unsigned int)dr_rand() & 0xff) - 0x80) + p[1];
	p[2] = (float)(int)(((unsigned int)dr_rand() & 0xff) - 0x80) + p[2];
	DR_I32(p, 0x10) = dr_rand() & 0xff;
	DR_I32(p, 0x14) = 0;
	return 0;
}
