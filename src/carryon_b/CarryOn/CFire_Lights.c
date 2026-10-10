/*
CarryOn/CFire_Lights.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Lights.cpp

The CarryOn light manager (one object, DAT_002f7988): a list of fire lights (point,
directional and spot) of which at most 8 own an engine light (BLight) at a time, and three
flash lights that the firework bursts light up for a moment.

Light flags: 1 on (owns an enabled BLight), 2 off (switched off by the game), 4 the BLight
is created with flag 4.

0x000d1010, 0x000d10d0, 0x000d1160 and 0x000d18a0 are tagged with CFire_Lights.cpp; the
others lie between them (bracketed).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_Lights.cpp"

/* 0x002f7988  the light manager */
struct CFire_Lights *DAT_002f7988;

/* 0x00294968  default position of a point light (0, 100, 0) */
float DAT_00294968[4] = { 0.0f, 100.0f, 0.0f, 1.0f };
/* 0x00294978  default position of a spot light (0, 100, 0) */
float DAT_00294978[4] = { 0.0f, 100.0f, 0.0f, 1.0f };
/* 0x00294988  default target of a spot light (0, 0, 0) */
float DAT_00294988[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

/* switches the engine light of `light` off (flag 1 cleared) */
static void cb_light_off(struct CFire_Light *light)
{
	light->flags &= ~1u;
	if (light->blight != NULL)
		baEnableLight(light->blight, 0);
}

/* sends the light's range (and spot data) to its engine light */
static void cb_light_set_range(struct CFire_Light *light, float range)
{
	light->range = range;
	if (light->blight != NULL)
	{
		if (light->type == 0)
			baSetPointLight(light->blight, light->pos, range);
		else if (light->type == 2)
			baSetSpotLight(light->blight, light->pos, light->target, range, light->angle);
	}
}

/* allocates a light and puts it at the head of the manager's list */
static struct CFire_Light *cb_light_new(struct CFire_Lights *self, int line)
{
	struct CFire_Light *light = bkHeapAlloc(0x50, CB_FILE, line, 0x2001);

	memset(light, 0, 0x50);
	return light;
}

/* 0x000d1010  CarryOn/CFire_Lights.cpp
Adds a point light. */
struct CFire_Light *CFire_Lights_AddPoint(struct CFire_Lights *self, int group, float *pos, int r,
	int g, int b, float range, unsigned char flags)
{
	struct CFire_Light *light = cb_light_new(self, 0x34);

	if (flags & 1)
		light->flags |= 4;
	light->group = group;
	light->r = r;
	light->range = range;
	light->b = b;
	light->type = 0;
	light->g = g;
	light->pos[0] = pos[0];
	light->pos[1] = pos[1];
	light->pos[2] = pos[2];
	light->pos[3] = pos[3];
	light->next = self->list;
	self->list = light;
	return light;
}

/* 0x000d10a0  CarryOn/CFire_Lights.cpp (bracketed)
Adds a point light at the default position (0, 100, 0). */
struct CFire_Light *CFire_Lights_AddPointDefault(struct CFire_Lights *self, int group, int r, int g,
	int b, float range, unsigned char flags)
{
	return CFire_Lights_AddPoint(self, group, DAT_00294968, r, g, b, range, flags);
}

/* 0x000d10d0  CarryOn/CFire_Lights.cpp
Adds a directional light. */
struct CFire_Light *CFire_Lights_AddDirectional(struct CFire_Lights *self, int group, float *dir,
	int r, int g, int b, unsigned char flags)
{
	struct CFire_Light *light = cb_light_new(self, 0x53);

	if (flags & 1)
		light->flags |= 4;
	light->r = r;
	light->g = g;
	light->b = b;
	light->type = 1;
	light->group = group;
	light->pos[0] = dir[0];
	light->pos[1] = dir[1];
	light->pos[2] = dir[2];
	light->pos[3] = dir[3];
	light->next = self->list;
	self->list = light;
	return light;
}

/* 0x000d1160  CarryOn/CFire_Lights.cpp
Adds a spot light from pos to target with the cone angle. */
struct CFire_Light *CFire_Lights_AddSpot(struct CFire_Lights *self, int group, float *pos,
	float *target, float angle, int r, int g, int b, float range, unsigned char flags)
{
	struct CFire_Light *light = cb_light_new(self, 0x69);

	if (flags & 1)
		light->flags |= 4;
	light->r = r;
	light->b = b;
	light->g = g;
	light->group = group;
	light->type = 2;
	light->pos[0] = pos[0];
	light->pos[1] = pos[1];
	light->pos[2] = pos[2];
	light->pos[3] = pos[3];
	light->target[0] = target[0];
	light->target[1] = target[1];
	light->target[2] = target[2];
	light->target[3] = target[3];
	light->angle = angle;
	light->range = range;
	light->next = self->list;
	self->list = light;
	return light;
}

/* 0x000d1210  CarryOn/CFire_Lights.cpp (bracketed)
Adds a spot light from (0, 100, 0) to the origin. */
struct CFire_Light *CFire_Lights_AddSpotDefault(struct CFire_Lights *self, int group, float angle,
	int r, int g, int b, float range, unsigned char flags)
{
	return CFire_Lights_AddSpot(self, group, DAT_00294978, DAT_00294988, angle, r, g, b, range,
		flags);
}

/* 0x000d1250  CarryOn/CFire_Lights.cpp (bracketed)
Gives the light an engine light: a free one of the 8, else one taken from the first light
that is not on. Nothing when all 8 are on. */
void CFire_Lights_Attach(struct CFire_Lights *self, struct CFire_Light *light)
{
	int i;

	for (i = 0; i < 8; i++)
		if (self->slots[i] == NULL)
			break;
	if (i == 8)
	{
		for (i = 0; i < 8; i++)
		{
			if ((self->slots[i]->flags & 1) == 0)
			{
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
	if (light->type == 0)
	{
		baSetPointLight(light->blight, light->pos, light->range);
		baSetLightColour(light->blight, light->r, light->g, light->b);
		baEnableLight(light->blight, 1);
	}
	else if (light->type == 1)
	{
		baSetDirectionalLight(light->blight, light->pos);
		baSetLightColour(light->blight, light->r, light->g, light->b);
		baEnableLight(light->blight, 1);
	}
	else if (light->type == 2)
	{
		baSetSpotLight(light->blight, light->pos, light->target, light->range, light->angle);
		baSetLightColour(light->blight, light->r, light->g, light->b);
		baEnableLight(light->blight, 1);
	}
}

/* 0x000d1390  CarryOn/CFire_Lights.cpp (bracketed)
Switches the light on (when it is not switched off by flag 2) or off. */
void CFire_Lights_Enable(struct CFire_Lights *self, struct CFire_Light *light, int on)
{
	unsigned int flags = light->flags;

	if ((flags & 2) == 0 && on != 0)
	{
		if ((flags & 1) == 0)
		{
			if (light->blight == NULL)
			{
				CFire_Lights_Attach(self, light);
				if (light->blight == NULL)
				{
					bkPrintf("Fire Light Enable couldn't find a spare\n");
					return;
				}
			}
			light->flags |= 1;
			baEnableLight(light->blight, 1);
		}
	}
	else if ((flags & 1) != 0)
	{
		cb_light_off(light);
	}
}

/* 0x000d1410  CarryOn/CFire_Lights.cpp (bracketed)
flags = (flags | set) ^ toggle; flag 2 switches the light off. Returns the new flags. */
unsigned int CFire_Lights_SetFlags(struct CFire_Lights *self, struct CFire_Light *light,
	unsigned int set, unsigned int toggle)
{
	unsigned int old = light->flags;
	unsigned int flags = (old | set) ^ toggle;

	light->flags = flags;
	if (flags & 2)
	{
		if (flags & 1)
			cb_light_off(light);
		old = 0;
	}
	if ((old & 1) != (light->flags & 1))
		CFire_Lights_Enable(self, light, light->flags & 1);
	return light->flags;
}

/* 0x000d1470  CarryOn/CFire_Lights.cpp (bracketed) */
void CFire_Lights_SetColour(struct CFire_Lights *self, struct CFire_Light *light, int r, int g, int b)
{
	(void)self;
	light->r = r;
	light->g = g;
	light->b = b;
	if (light->blight != NULL)
		baSetLightColour(light->blight, r, g, b);
}

/* 0x000d14a0  CarryOn/CFire_Lights.cpp (bracketed)
Moves a point or spot light. */
void CFire_Lights_SetPos(struct CFire_Lights *self, struct CFire_Light *light, float *pos)
{
	(void)self;
	if (light->type == 0 || light->type == 2)
	{
		light->pos[0] = pos[0];
		light->pos[1] = pos[1];
		light->pos[2] = pos[2];
		light->pos[3] = pos[3];
	}
	if (light->blight != NULL)
	{
		if (light->type == 0)
			baSetPointLight(light->blight, light->pos, light->range);
		else if (light->type == 2)
			baSetSpotLight(light->blight, light->pos, light->target, light->range, light->angle);
	}
}

/* 0x000d1510  CarryOn/CFire_Lights.cpp (bracketed)
Moves a spot light and its target. */
void CFire_Lights_SetSpot(struct CFire_Lights *self, struct CFire_Light *light, float *pos,
	float *target)
{
	(void)self;
	if (light->type != 2)
		return;
	light->pos[0] = pos[0];
	light->pos[1] = pos[1];
	light->pos[2] = pos[2];
	light->pos[3] = pos[3];
	light->target[0] = target[0];
	light->target[1] = target[1];
	light->target[2] = target[2];
	light->target[3] = target[3];
	if (light->blight != NULL)
		baSetSpotLight(light->blight, light->pos, light->target, light->range, light->angle);
}

/* 0x000d1580  CarryOn/CFire_Lights.cpp (bracketed)
Sets the cone angle of a spot light. */
void CFire_Lights_SetAngle(struct CFire_Lights *self, struct CFire_Light *light, float angle)
{
	(void)self;
	if (light->type != 2)
		return;
	light->angle = angle;
	if (light->blight != NULL)
		baSetSpotLight(light->blight, light->pos, light->target, light->range, angle);
}

/* 0x000d15c0  CarryOn/CFire_Lights.cpp (bracketed)
Fades the three flash lights: their range grows as their time runs out; at 0 they are
switched off. */
void CFire_Lights_Update(struct CFire_Lights *self, float dt)
{
	struct CFire_Light *light;
	float t;
	int i;

	for (i = 0; i < 3; i++)
	{
		if (self->flashTime[i] == 0.0f)
			continue;
		t = self->flashTime[i] - dt;
		self->flashTime[i] = t;
		if (0.0f <= t)
		{
			cb_light_set_range(self->flash[i], 0.001f - t * 5e-06f); /* 0x0025767c, 0x00268b1c */
		}
		else
		{
			light = self->flash[i];
			self->flashTime[i] = 0.0f;
			if (light->flags & 1)
				cb_light_off(light);
			/* inlined CFire_Lights_SetFlags(self, light, 2, 0) */
			CFire_Lights_SetFlags(self, self->flash[i], 2, 0);
		}
	}
}

/* 0x000d16f0  CarryOn/CFire_Lights.cpp (bracketed)
Lights a flash at pos in the colour (each channel at most 127) for 180 time units. */
void CFire_Lights_Flash(struct CFire_Lights *self, float *pos, int r, int g, int b)
{
	struct CFire_Light *light;
	int i;

	if (0x7f < r)
		r = 0x7f;
	if (0x7f < g)
		g = 0x7f;
	if (0x7f < b)
		b = 0x7f;
	i = self->flashTime[1] < 100000.0f;                 /* 0x00268b20 */
	if (self->flashTime[2] < 100000.0f)
		i = 2;
	self->flashTime[i] = 180.0f;
	/* inlined: switch on again (flag 2 cleared), then on, colour, position, range */
	CFire_Lights_SetFlags(self, self->flash[i], 2, 2);
	CFire_Lights_Enable(self, self->flash[i], 1);
	light = self->flash[i];
	light->r = r;
	light->g = g;
	light->b = b;
	if (light->blight != NULL)
		baSetLightColour(light->blight, r, g, b);
	CFire_Lights_SetPos(self, self->flash[i], pos);
	cb_light_set_range(self->flash[i], 2.5e-05f);       /* 0x38d1b717 */
}

/* 0x000d18a0  CarryOn/CFire_Lights.cpp
Constructor: makes the three flash lights (white point lights, switched off). */
struct CFire_Lights *CFire_Lights_ctor(struct CFire_Lights *self)
{
	struct CFire_Light *light;
	int i;

	memset(self, 0, 0x5c);
	DAT_002f7988 = self;
	for (i = 0; i < 3; i++)
	{
		/* inlined CFire_Lights_AddPointDefault(self, 8, 127, 127, 127, 0.0005f, 0) */
		light = cb_light_new(self, 0x34);
		light->type = 0;
		light->group = 8;
		light->range = 0.0005f;                         /* 0x3a03126f */
		light->r = 0x7f;
		light->g = 0x7f;
		light->b = 0x7f;
		light->pos[0] = DAT_00294968[0];
		light->pos[1] = DAT_00294968[1];
		light->pos[2] = DAT_00294968[2];
		light->pos[3] = DAT_00294968[3];
		light->next = self->list;
		self->list = light;
		self->flash[i] = light;
		CFire_Lights_SetFlags(self, light, 2, 0);
	}
	return self;
}

/* 0x000d19b0  CarryOn/CFire_Lights.cpp (file inferred)
Switches every light of a group on or off. */
void CFire_Lights_EnableGroup(struct CFire_Lights *self, int group, int on)
{
	struct CFire_Light *light;

	for (light = self->list; light != NULL; light = light->next)
		if (light->group == group)
			CFire_Lights_Enable(self, light, on);
}
