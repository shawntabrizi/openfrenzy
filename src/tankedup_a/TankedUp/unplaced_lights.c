/*
TankedUp/unplaced_lights.c  (gap functions of C:\BlitzParty\Game\Src\TankedUp\, file unknown)

The light system (CTank_Game +0xe34, "lightSystem"): four bActor lights (struct TuLight,
tankedup_a.h). Two of them become the sun lights when the game starts; the others are
flashes of explosions and shots (point lights that fade out), tanks' spot lights, and
tracking spots. The ambient and tracking spot colours come from the debug object
(TU_DBG_*). The methods are __thiscall on the light array; some ignore `this`.
*/

#include "tankedup_a.h"

#define TU_LIGHT_COUNT 4

/* a free light (state 0) is made ready: no age, no life, no cone, not the sun */
static void lights_clear(struct TuLight *light)
{
	light->time = 0.0f;
	light->duration = 0.0f;
	light->drawCone = 0;
	light->sun = 0;
}

/* 0x001c3820  TankedUp (gap; "lightSystem.Initialise")
Marks the four lights as "no light" until Start creates them. Returns 1. */
int CTankLights_Initialise(struct TuLight *self)
{
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++)
		self[i].state = 1;
	return 1;
}

/* 0x001c3850  TankedUp (gap)
Sets the ambient light to the debug object's ambient colour. */
void CTankLights_ApplyAmbient(struct TuLight *self)
{
	(void)self;
	baSetAmbientLight(TU_DBG_AMBIENT_R, TU_DBG_AMBIENT_G, TU_DBG_AMBIENT_B);
}

/* 0x001c3870  TankedUp (gap)
Reads the debug object's ambient colour. */
void CTankLights_GetAmbient(struct TuLight *self, int *r, int *g, int *b)
{
	(void)self;
	*r = TU_DBG_AMBIENT_R;
	*g = TU_DBG_AMBIENT_G;
	*b = TU_DBG_AMBIENT_B;
}

/* 0x001c38a0  TankedUp (gap)
Sets the ambient light. (A __thiscall method that does not use `this`; written without it,
as tankedup_b calls it.) */
void CTankLights_SetAmbient(int r, int g, int b)
{
	baSetAmbientLight(r, g, b);
}

/* 0x001c38c0  TankedUp (gap)
Switches the sun lights on or off. The main sun stays on while "off", in grey, with a
grey ambient light (the paused look); "on" gives back its colour and the ambient light. */
void CTankLights_EnableSun(struct TuLight *self, int on)
{
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		struct TuLight *light = &self[i];

		if (light->state != 2)
			continue;
		baEnableLight((struct BLight *)light->light, on);
		if (light->sun == 1) {
			if (on == 0) {
				baEnableLight((struct BLight *)light->light, 1);
				baSetLightColour((struct BLight *)light->light, 0x7f, 0x7f, 0x7f);
				baSetAmbientLight(0x60, 0x60, 0x60);
			} else {
				baSetLightColour((struct BLight *)light->light, 200, 0xaa, 0x50);
				baSetAmbientLight(TU_DBG_AMBIENT_R, TU_DBG_AMBIENT_G, TU_DBG_AMBIENT_B);
			}
		}
	}
}

/* 0x001c3960  TankedUp (gap)
Switches every light in use (state > 1) on or off. */
void CTankLights_EnableAll(struct TuLight *self, int on)
{
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++)
		if (self[i].state > 1)
			baEnableLight((struct BLight *)self[i].light, on);
}

/* 0x001c3990  TankedUp (gap)
Frees light `index` (-1: none). */
void CTankLights_Remove(struct TuLight *self, int index)
{
	if (index != -1) {
		struct TuLight *light = &self[index];

		light->state = 0;
		light->drawCone = 0;
		baEnableLight((struct BLight *)light->light, 0);
		TU_DPRINTF("Removeing light %d\n", index);
	}
}

/* 0x001c39d0  TankedUp (gap)
Deletes the bActor lights. */
void CTankLights_Free(struct TuLight *self)
{
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		if (self[i].light != NULL) {
			baEnableLight((struct BLight *)self[i].light, 0);
			baDeleteLight((struct BLight *)self[i].light);
			self[i].light = NULL;
		}
	}
}

/* 0x001c3a10  TankedUp (gap)
Ages the lights by `step` frames. A flash (state 6) dims as it ages; a light whose life is
over is freed. Returns `state` (no state change). */
int CTankLights_Update(struct TuLight *self, float step, int state)
{
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		struct TuLight *light = &self[i];
		int kind = light->state;
		float time = step + light->time;

		light->time = time;
		if (kind == 0 || kind == 1 || light->duration == 0.0f || light->duration > time) {
			if (kind == 6) {
				light->pos[3] = 1.0f;
				light->target[3] = 1.0f;
				baSetPointLight((struct BLight *)light->light, light->pos,
					1.0f / ((1.0f - time / light->duration) * light->range));
			}
		} else {
			light->state = 0;
			light->drawCone = 0;
			baEnableLight((struct BLight *)light->light, 0);
			TU_DPRINTF("Removeing light %d\n", i);
		}
	}
	return state;
}

/* 0x001c3ae0  TankedUp (gap)
Creates the four bActor lights (switched off), sets the ambient light, and makes the two
sun lights: a warm main sun (the one dimmed while paused) and a cool fill light. */
void CTankLights_Start(struct TuLight *self)
{
	float dir[4];
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		struct TuLight *light = &self[i];

		light->light = baCreateLight(NULL, 0);
		if (light->light == NULL) {
			light->state = 1;
			TU_DPRINTF("ERROR: can't create light (%d)\n", i);
		} else {
			TU_DPRINTF("starting light %d\n", i);
			light->state = 0;
		}
		baEnableLight((struct BLight *)light->light, 0);
		light->drawCone = 0;
	}
	baSetAmbientLight(TU_DBG_AMBIENT_R, TU_DBG_AMBIENT_G, TU_DBG_AMBIENT_B);
	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		struct TuLight *light;

		if (self[i].state != 0)
			continue;
		light = &self[i];
		lights_clear(light);
		dir[0] = 0.03f;
		dir[1] = 0.72f;
		dir[2] = 0.7f;
		dir[3] = 1.0f;
		light->state = 2;
		baEnableLight((struct BLight *)light->light, 1);
		baSetDirectionalLight((struct BLight *)light->light, dir);
		baSetLightColour((struct BLight *)light->light, 200, 0xaa, 0x50);
		light->sun = 1;
		for (i = 0; self[i].state != 0; ) {
			i++;
			if (i > 3)
				return;
		}
		light = &self[i];
		lights_clear(light);
		baEnableLight((struct BLight *)light->light, 1);
		light->state = 2;
		dir[0] = 0.69f;
		dir[1] = 0.72f;
		dir[2] = 0.05f;
		dir[3] = 1.0f;
		baSetDirectionalLight((struct BLight *)light->light, dir);
		baSetLightColour((struct BLight *)light->light, 0x90, 199, 0xcc);
		return;
	}
}

/* 0x001c3c90  TankedUp (gap)
Points the tracking spots of `owner` at `target`, in the debug object's spot colour and
range. */
void CTankLights_TrackSpot(struct TuLight *self, int owner, float *target)
{
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		struct TuLight *light = &self[i];

		if (light->state == 5 && light->owner == owner) {
			light->target[0] = target[0];
			light->target[1] = target[1];
			light->target[2] = target[2];
			light->target[3] = target[3];
			light->target[3] = 1.0f;
			baSetSpotLight((struct BLight *)light->light, light->pos, light->target,
				1.0f / TU_DBG_SPOT_RANGE, 0.349065840f);
			light->r = TU_DBG_SPOT_R;
			light->g = TU_DBG_SPOT_G;
			light->b = TU_DBG_SPOT_B;
			baSetLightColour((struct BLight *)light->light, light->r, light->g, light->b);
		}
	}
}

/* 0x001c3d30  TankedUp (gap)
Adds an explosion flash at `pos`: a point light that fades over 180 frames, in (r, g, b)
or grey when r is -1. Nothing happens when every light is in use. */
void CTankLights_AddFlash(struct TuLight *self, float *pos, int r, int g, int b)
{
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		struct TuLight *light = &self[i];

		if (light->state != 0)
			continue;
		lights_clear(light);
		light->state = 6;
		baEnableLight((struct BLight *)light->light, 1);
		light->pos[0] = pos[0];
		light->pos[1] = pos[1];
		light->pos[2] = pos[2];
		light->pos[3] = pos[3];
		light->pos[3] = 1.0f;
		baSetPointLight((struct BLight *)light->light, light->pos, 0.0005f);
		light->range = 2000.0f;
		light->duration = 180.0f;
		if (r != -1) {
			baSetLightColour((struct BLight *)light->light, r, g, b);
			return;
		}
		baSetLightColour((struct BLight *)light->light, 0x7f, 0x7f, 0x7f);
		return;
	}
}

/* 0x001c3e00  TankedUp (gap)
Adds a short orange flash at `pos` (a shot): 60 frames. */
void CTankLights_AddFireFlash(struct TuLight *self, float *pos)
{
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		struct TuLight *light = &self[i];

		if (light->state != 0)
			continue;
		lights_clear(light);
		light->state = 6;
		baEnableLight((struct BLight *)light->light, 1);
		light->pos[0] = pos[0];
		light->pos[1] = pos[1];
		light->pos[2] = pos[2];
		light->pos[3] = pos[3];
		light->pos[3] = 1.0f;
		baSetPointLight((struct BLight *)light->light, light->pos, 0.00025f);
		light->range = 4000.0f;
		light->duration = 60.0f;
		baSetLightColour((struct BLight *)light->light, 0x7f, 0x40, 0);
		return;
	}
}

/* 0x001c3eb0  TankedUp (gap)
A spot light that points straight down from `pos` (a tank's light). *index is the light;
-1 takes a free light (and stays -1 when none is free). (r, g, b) sets the colour unless
r is -1. */
void CTankLights_SetSpot(struct TuLight *self, int *index, float *pos, int r, int g, int b)
{
	float target[4];

	if (*index == -1) {
		int i;

		for (i = 0; i < TU_LIGHT_COUNT; i++) {
			if (self[i].state == 0) {
				lights_clear(&self[i]);
				break;
			}
		}
		if (i == TU_LIGHT_COUNT)
			i = -1;
		*index = i;
		if (i == -1)
			return;
		self[i].state = 4;
		baEnableLight((struct BLight *)self[*index].light, 1);
	}
	target[0] = pos[0];
	target[2] = pos[2];
	target[1] = pos[1] - 1.0f;
	target[3] = 1.0f;
	baSetSpotLight((struct BLight *)self[*index].light, pos, target, 0.000666666660f, 3.14159250f);
	if (r != -1)
		baSetLightColour((struct BLight *)self[*index].light, r, g, b);
}

/* 0x001c3fa0  TankedUp (gap)
Per frame before drawing: the tracking spots take the debug colour and range and the
debug switch TU_DBG_SPOTS, the suns the switch TU_DBG_SUN; sets the ambient light; draws
the debug cones. */
void CTankLights_Draw(struct TuLight *self)
{
	struct TuLight copy;
	int i;

	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		struct TuLight *light = &self[i];

		if (light->state == 5) {
			light->r = TU_DBG_SPOT_R;
			light->g = TU_DBG_SPOT_G;
			light->b = TU_DBG_SPOT_B;
			baSetLightColour((struct BLight *)light->light, light->r, light->g, light->b);
			baSetSpotLight((struct BLight *)light->light, light->pos, light->target,
				1.0f / TU_DBG_SPOT_RANGE, 0.349065840f);
			baEnableLight((struct BLight *)light->light, TU_DBG_SPOTS);
		}
	}
	for (i = 0; i < TU_LIGHT_COUNT; i++)
		if (self[i].state == 2)
			baEnableLight((struct BLight *)self[i].light, TU_DBG_SUN);
	baSetAmbientLight(TU_DBG_AMBIENT_R, TU_DBG_AMBIENT_G, TU_DBG_AMBIENT_B);
	for (i = 0; i < TU_LIGHT_COUNT; i++) {
		if (self[i].drawCone != 0) {
			copy = self[i];
			TUB_Debug_DrawCone(copy.pos, copy.target, copy.cone, copy.r, copy.g, copy.b);
		}
	}
}
