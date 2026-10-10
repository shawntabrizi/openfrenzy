/*
BACTOR/lights.c  <-  C:\Babel\Xbox\Src\BACTOR\lights.cpp

Lights: two lists (disabled and enabled lights) and the ambient colour.
The D3D side (bLightUpdateD3D and friends) is in unplaced.c.

0x000382f0 is tagged with lights.cpp; the others follow it in the XBE (file
inferred).
*/

#include <math.h>

#include "bactor.h"
#include "bactor_extern.h"

/* 0x00284b0c: global brightness (0x100: full) */
int DAT_00284b0c = 0x100;
/* 0x00284b10: ambient colour (0..255 per channel) */
int DAT_00284b10 = 0x1e;
int DAT_00284b14 = 0x1e;
int DAT_00284b18 = 0x1e;

/* 0x002bb138: head of the disabled lights; 0x002bb1a8: head of the enabled lights */
struct BLight DAT_002bb138;
struct BLight DAT_002bb1a8;
/* 0x002bb218 */
int DAT_002bb218;

/* 0x000382c0  BACTOR/lights.cpp (file inferred) */
int baInitLights(void)
{
	/* FUN_000a6fa0 is a shared "return 1" function (identical-code folding) */
	if (FUN_000a6fa0() == 0)
		return 0;
	g_lightsOn.prev = &g_lightsOn;
	g_lightsOn.next = &g_lightsOn;
	g_lightsOff.prev = &g_lightsOff;
	g_lightsOff.next = &g_lightsOff;
	return 1;
}

/* 0x000382f0  BACTOR/lights.cpp
creates a light (allocated when light is NULL), disabled */
struct BLight *baCreateLight(struct BLight *light, uint32_t flags)
{
	if (light == NULL)
	{
		light = ba_bkAlloc(sizeof(struct BLight), "C:\\Babel\\Xbox\\Src\\BACTOR\\lights.cpp", 0x56, 0x2002);
		if (light == NULL)
			return NULL;
		light->flags = flags | BLIGHTF_ALLOCATED;
	}
	else
	{
		light->flags = flags;
	}
	light->type = 0;
	light->color[2] = 0x7f;
	light->color[1] = 0x7f;
	light->color[0] = 0x7f;
	light->prev = g_lightsOff.prev;
	light->next = &g_lightsOff;
	g_lightsOff.prev->next = light;
	g_lightsOff.prev = light;
	bLightAssignIndex(light);
	return light;
}

/* 0x00038370  BACTOR/lights.cpp (file inferred) */
void baEnableLight(struct BLight *light, int enable)
{
	uint32_t on = light->flags & BLIGHTF_ENABLED;

	if ((on == 0 || enable == 0) && (on != 0 || enable != 0))
	{
		light->next->prev = light->prev;
		light->prev->next = light->next;
		if (enable != 0)
		{
			if (DAT_002bb218 == DAT_002bc3ec)
			{
				/* the light stays unlinked (as in the original) */
				ba_bkPrintf("baEnableLight: ERROR - max of %d lights already active\n", DAT_002bc3ec);
				return;
			}
			light->prev = g_lightsOn.prev;
			light->next = &g_lightsOn;
			g_lightsOn.prev->next = light;
			g_lightsOn.prev = light;
			light->flags |= BLIGHTF_ENABLED;
			DAT_002bb218++;
			bLightEnableD3D(light);
			return;
		}
		DAT_002bb218--;
		light->flags &= ~BLIGHTF_ENABLED;
		light->prev = g_lightsOff.prev;
		light->next = &g_lightsOff;
		g_lightsOff.prev->next = light;
		g_lightsOff.prev = light;
		bLightEnableD3D(light);
	}
}

/* 0x00038440  BACTOR/lights.cpp (file inferred) */
void baSetAmbientLight(int r, int g, int b)
{
	DAT_00284b10 = r;
	DAT_00284b14 = g;
	DAT_00284b18 = b;
	bSetAmbientD3D();
}

/* 0x00038470  BACTOR/lights.cpp (file inferred) */
void baSetLightColour(struct BLight *light, int r, int g, int b)
{
	light->color[0] = r;
	light->color[1] = g;
	light->color[2] = b;
	bLightColourChanged(light);
}

/* 0x000384a0  BACTOR/lights.cpp (file inferred)
out = v / d (three components) */
void bVec3Divide(float *out, const float *v, float d)
{
	float scale = 1.0f / d;

	out[0] = scale * v[0];
	out[1] = scale * v[1];
	out[2] = scale * v[2];
}

/* 0x000384d0  BACTOR/lights.cpp (file inferred) */
void baDeleteLight(struct BLight *light)
{
	if (light->flags & BLIGHTF_ENABLED)
	{
		light->next->prev = light->prev;
		light->prev->next = light->next;
		DAT_002bb218--;
		light->flags &= ~BLIGHTF_ENABLED;
		light->prev = g_lightsOff.prev;
		light->next = &g_lightsOff;
		g_lightsOff.prev->next = light;
		g_lightsOff.prev = light;
		bLightEnableD3D(light);
	}
	light->next->prev = light->prev;
	light->prev->next = light->next;
	if (light->flags & BLIGHTF_ALLOCATED)
		ba_bkFree(light);
}

/* 0x00038550  BACTOR/lights.cpp (file inferred) */
void baSetDirectionalLight(struct BLight *light, const float *dir)
{
	light->type = BLIGHT_DIRECTIONAL;
	light->pos[0] = dir[0];
	light->pos[1] = dir[1];
	light->pos[2] = dir[2];
	light->pos[3] = dir[3];
	ba_Vec3Normalize(light->dir, dir);
	light->pos[3] = 1.0f;
	light->dir[3] = 1.0f;
	bLightUpdateD3D(light);
}

/* 0x000385a0  BACTOR/lights.cpp (file inferred) */
void baSetPointLight(struct BLight *light, const float *pos, float range)
{
	light->type = BLIGHT_POINT;
	light->pos[0] = pos[0];
	light->pos[1] = pos[1];
	light->pos[2] = pos[2];
	light->pos[3] = pos[3];
	light->dir[0] = range;
	bLightUpdateD3D(light);
}

/* 0x000385e0  BACTOR/lights.cpp (file inferred) */
void baSetSpotLight(struct BLight *light, const float *pos, const float *target, float range, float angle)
{
	light->type = BLIGHT_SPOT;
	light->pos[0] = pos[0];
	light->pos[1] = pos[1];
	light->pos[2] = pos[2];
	light->pos[3] = pos[3];
	light->dir[0] = target[0] - pos[0];
	light->dir[1] = target[1] - pos[1];
	light->dir[2] = target[2] - pos[2];
	light->dir[3] = 0.0f;
	ba_Vec3Normalize(light->dirNorm, light->dir);
	light->dirNorm[3] = 0.0f;
	light->range = range;
	light->angle = angle;
	light->cosHalfAngle = (float)cos((double)angle * 0.5);
	bLightUpdateD3D(light);
}
