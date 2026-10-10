/*
Rowing/CRowing_Lights.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_Lights.cpp

The Rowing light manager (one object of 0x48 bytes, g_RowLights at 0x0030fc04): a list of
point and directional lights in groups, of which at most 8 own an engine light (BLight) at
a time; a light may draw a glow sprite. The D3D light of a BLight is set again with the
Rowing attenuation (the range, or its square root) after each change.

0x001b52e0 and 0x001b5370 are tagged with CRowing_Lights.cpp; 0x001b4f90-0x001b56a0 work on
the same objects (file inferred).
*/

#include "rowing_b.h"

/* 0x001b4f90  Rowing (gap; CRowing_Lights.cpp inferred) */
struct RowLights *RowLights_ctor(struct RowLights *self)
{
	memset(self, 0, 0x48);
	g_RowLights = self;
	self->rawRange = 0;
	return self;
}

/* 0x001b4fb0  Rowing (gap; CRowing_Lights.cpp inferred)
Frees every light (switching off and deleting its BLight). */
void RowLights_Free(struct RowLights *self)
{
	struct RowLight *next;

	while (self->list != NULL) {
		next = self->list->next;
		if (self->list->blight != NULL) {
			baEnableLight(self->list->blight, 0);
			baDeleteLight(self->list->blight);
		}
		self->list->blight = NULL;
		RB_DELETE(self->list);
		self->list = next;
	}
	g_RowLights = NULL;
}

/* 0x001b5010  Rowing (gap; CRowing_Lights.cpp inferred)
Sets the glow sprite of a light (only its size when `sprite` is NULL). */
void RowLight_SetSprite(struct RowLight *light, void *sprite, float size)
{
	if (sprite != NULL) {
		light->sprite = sprite;
		light->spriteSize = size;
		return;
	}
	light->spriteSize = size;
}

/* 0x001b5040  Rowing (gap; CRowing_Lights.cpp inferred)
Adds the glow sprites of the lights that are on to sprite list 2. */
void RowLights_DrawSprites(struct RowLights *self)
{
	struct RowLight *light;
	int colour[4];

	for (light = self->list; light != NULL; light = light->next) {
		if ((light->flags & 1) == 0 || light->sprite == NULL)
			continue;
		bdSetWorldMatrix((float *)DAT_002855f0);
		colour[0] = light->r;
		colour[1] = light->g;
		colour[2] = light->b;
		colour[3] = 0xff;
		/* (the original passes the sprite manager at 0x0030f138 as an unused this) */
		RowSprites_Add(2, light->pos, light->spriteSize, light->spriteSize, 0.0f, light->sprite, colour,
			-0.0500000007f);
	}
}

/* 0x001b50d0  Rowing (gap; CRowing_Lights.cpp inferred)
Sets the D3D light of the light's BLight again: its colour (and specular colour with
BLight flag 4) and, for point and spot lights, range 1000000, falloff 1 and the Rowing
attenuation (attenuation 2 = the range when rawRange, else attenuation 1 = its root). */
void RowLight_SetD3DLight(struct RowLight *light, int rawRange)
{
	unsigned char *bl = (unsigned char *)light->blight;
	D3DLIGHT8 d3d;

	if (bl == NULL)
		return;
	memset(&d3d, 0, sizeof d3d);
	d3d.Diffuse.r = DAT_002bd220[RB_I32(bl, 0x14)];
	d3d.Diffuse.g = DAT_002bd220[RB_I32(bl, 0x18)];
	d3d.Diffuse.b = DAT_002bd220[RB_I32(bl, 0x1c)];
	d3d.Diffuse.a = 1.0f;
	if ((RB_U8(bl, 0x10) & 4) == 0) {
		d3d.Specular.r = 0.0f;
		d3d.Specular.g = 0.0f;
		d3d.Specular.b = 0.0f;
		d3d.Specular.a = 0.0f;
	} else {
		d3d.Specular.r = DAT_002bd220[RB_I32(bl, 0x14)];
		d3d.Specular.g = DAT_002bd220[RB_I32(bl, 0x18)];
		d3d.Specular.b = DAT_002bd220[RB_I32(bl, 0x1c)];
		d3d.Specular.a = 1.0f;
	}
	switch (RB_I32(bl, 0x00)) {
	case 1:     /* directional */
		d3d.Type = D3DLIGHT_DIRECTIONAL;
		d3d.Direction.x = -RB_F32(bl, 0x30);
		d3d.Direction.y = -RB_F32(bl, 0x34);
		d3d.Direction.z = -RB_F32(bl, 0x38);
		break;
	case 2:     /* point */
		d3d.Type = D3DLIGHT_POINT;
		d3d.Position.x = RB_F32(bl, 0x30);
		d3d.Position.y = RB_F32(bl, 0x34);
		d3d.Position.z = RB_F32(bl, 0x38);
		d3d.Range = 1000000.0f;                                 /* 0x49742400 */
		d3d.Falloff = 1.0f;
		if (rawRange != 0)
			d3d.Attenuation2 = light->range;
		else
			d3d.Attenuation1 = bmFastSqrt(light->range);
		break;
	case 3:     /* spot */
		d3d.Type = D3DLIGHT_SPOT;
		d3d.Position.x = RB_F32(bl, 0x30);
		d3d.Position.y = RB_F32(bl, 0x34);
		d3d.Position.z = RB_F32(bl, 0x38);
		d3d.Direction.x = RB_F32(bl, 0x40);
		d3d.Direction.y = RB_F32(bl, 0x44);
		d3d.Direction.z = RB_F32(bl, 0x48);
		d3d.Range = 1000000.0f;
		d3d.Falloff = 1.0f;
		if (rawRange != 0)
			d3d.Attenuation2 = light->range;
		else
			d3d.Attenuation1 = bmFastSqrt(light->range);
		d3d.Phi = RB_F32(bl, 0x64);
		break;
	default:
		return;
	}
	D3DDevice_SetLight((DWORD)RB_I32(bl, 0x20), &d3d);
}

/* a new light at the head of the manager's list */
static struct RowLight *rb_light_new(struct RowLights *self, int line, unsigned char flags)
{
	struct RowLight *light = RB_NEW(0x3c, RB_FILE_LIGHTS, line, 0x2001);

	memset(light, 0, 0x3c);
	if (flags & 1)
		light->flags |= 4;
	(void)self;
	return light;
}

/* 0x001b52e0  Rowing/CRowing_Lights.cpp
Adds a point light of group `group`. */
struct RowLight *RowLights_AddPoint(struct RowLights *self, int group, float *pos, int r, int g, int b,
	float range, unsigned char flags)
{
	struct RowLight *light = rb_light_new(self, 0x80, flags);

	light->group = group;
	light->r = r;
	light->range = range;
	light->b = b;
	light->type = 0;
	light->g = g;
	memcpy(light->pos, pos, 16);
	light->next = self->list;
	self->list = light;
	return light;
}

/* 0x001b5370  Rowing/CRowing_Lights.cpp
Adds a directional light of group `group`. */
struct RowLight *RowLights_AddDirectional(struct RowLights *self, int group, float *dir, int r, int g,
	int b, unsigned char flags)
{
	struct RowLight *light = rb_light_new(self, 0x97, flags);

	light->r = r;
	light->g = g;
	light->b = b;
	light->type = 1;
	light->group = group;
	memcpy(light->pos, dir, 16);
	light->next = self->list;
	self->list = light;
	return light;
}

/* 0x001b5400  Rowing (gap; CRowing_Lights.cpp inferred)
Gives the light a BLight: in a free slot of the 8, else in the slot of the first light
that is not on (deleting its BLight). Nothing when all 8 are on. */
void RowLights_Attach(struct RowLights *self, struct RowLight *light)
{
	int i;

	for (i = 0; i < 8; i++)
		if (self->slots[i] == NULL)
			break;
	if (i == 8) {
		for (i = 0; i < 8; i++) {
			if ((self->slots[i]->flags & 1) == 0) {
				baDeleteLight(self->slots[i]->blight);
				self->slots[i]->blight = NULL;
				break;
			}
		}
		if (i == 8)
			return;
	}
	light->blight = baCreateLight(NULL, (light->flags & 4) ? 4 : 0);
	self->slots[i] = light;
	self->blights[i] = light->blight;
	if (light->type != 1) {
		baSetPointLight(light->blight, light->pos, bmFastSqrt(light->range));
		baSetLightColour(light->blight, light->r, light->g, light->b);
		RowLight_SetD3DLight(light, self->rawRange);
		baEnableLight(light->blight, 1);
		return;
	}
	baSetDirectionalLight(light->blight, light->pos);
	baSetLightColour(light->blight, light->r, light->g, light->b);
	RowLight_SetD3DLight(light, self->rawRange);
	baEnableLight(light->blight, 1);
}

/* 0x001b5510  Rowing (gap; CRowing_Lights.cpp inferred)
Switches the light on (when the game has not switched it off with flag 2; it gets a
BLight when it has none) or off. */
void RowLights_Enable(struct RowLights *self, struct RowLight *light, int on)
{
	unsigned int flags = light->flags;

	if ((flags & 2) == 0 && on != 0) {
		if ((flags & 1) != 0)
			return;
		if (light->blight == NULL) {
			RowLights_Attach(self, light);
			if (light->blight == NULL) {
				bkPrintf("Rowing Light Enable couldn't find a spare\n");
				return;
			}
		}
		light->flags = light->flags | 1;
		baEnableLight(light->blight, 1);
		return;
	}
	if ((flags & 1) != 0) {
		light->flags = flags & ~1u;
		if (light->blight != NULL)
			baEnableLight(light->blight, 0);
	}
}

/* 0x001b5590  Rowing (gap; CRowing_Lights.cpp inferred)
Sets the light's colour. */
void RowLights_SetColour(struct RowLights *self, struct RowLight *light, int r, int g, int b)
{
	light->r = r;
	light->g = g;
	light->b = b;
	if (light->blight != NULL) {
		baSetLightColour(light->blight, r, g, b);
		RowLight_SetD3DLight(light, self->rawRange);
	}
}

/* 0x001b55d0  Rowing (gap; CRowing_Lights.cpp inferred)
Moves a point light. */
void RowLights_SetPos(struct RowLights *self, struct RowLight *light, float *pos)
{
	if (light->type != 0)
		return;
	memcpy(light->pos, pos, 16);
	if (light->blight != NULL) {
		baSetPointLight(light->blight, pos, bmFastSqrt(light->range));
		RowLight_SetD3DLight(light, self->rawRange);
	}
}

/* 0x001b5630  Rowing (gap; CRowing_Lights.cpp inferred)
Sets the attenuation mode and resets the D3D lights of every light with a BLight. */
void RowLights_SetRawRange(struct RowLights *self, int rawRange)
{
	struct RowLight *light;

	self->rawRange = rawRange;
	for (light = self->list; light != NULL; light = light->next)
		if (light->blight != NULL)
			RowLight_SetD3DLight(light, self->rawRange);
}

/* 0x001b5660  Rowing (gap; CRowing_Lights.cpp inferred)
Switches the lights of a group on or off. */
void RowLights_EnableGroup(struct RowLights *self, int group, int on)
{
	struct RowLight *light;

	for (light = self->list; light != NULL; light = light->next)
		if (light->group == group)
			RowLights_Enable(self, light, on);
}

/* 0x001b56a0  Rowing (gap; CRowing_Lights.cpp inferred)
Changes the flags of a group's lights: (flags | set) ^ toggle. A light that becomes
switched off (flag 2) goes off; a change of flag 1 is applied. Returns the last light's
new flags (-1 when the group is empty). */
int RowLights_SetGroupFlags(struct RowLights *self, int group, unsigned int set, unsigned int toggle)
{
	struct RowLight *light;
	unsigned int old;
	unsigned int flags;
	int result = -1;

	for (light = self->list; light != NULL; light = light->next) {
		if (light->group != group)
			continue;
		old = light->flags;
		flags = (old | set) ^ toggle;
		light->flags = flags;
		if ((flags & 2) != 0) {
			if ((flags & 1) != 0) {
				light->flags = flags & ~1u;
				if (light->blight != NULL)
					baEnableLight(light->blight, 0);
			}
			old = 0;
		}
		if ((old & 1) != (light->flags & 1))
			RowLights_Enable(self, light, (int)(light->flags & 1));
		result = (int)light->flags;
	}
	return result;
}
