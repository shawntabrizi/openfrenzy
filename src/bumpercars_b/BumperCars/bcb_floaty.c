/*
BumperCars/bcb_floaty.c  <-  gap functions 0x000a6e50-0x000a74d0 (file unknown; ClassLib-like
code that CarryOn, Rowing, Hopper, Drillers, RollerBall and Demolition use too)

The floaty text system: an extra system "floatyscore" of short texts that rise and fade
(struct BCarFloatyScore, 0x90 bytes; the last one made is kept in 0x002f27c4). Each text is
drawn twice: once into the stencil, then a brighter, slightly bigger copy outside it (an
outline). 0x000a6e50 and 0x000a6fb0 are missing from the function table (callbacks reached
only through 0x000a74d0); they keep their FUN_ names.
*/

#include <stdarg.h>

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"

/* 0x002f27c4  the floaty text system made last (ClassLib; shared by the minigames) */
struct BCarFloatyScore *DAT_002f27c4;
#define g_FloatySystem DAT_002f27c4

/* 0x000a6e50  (gap; missing from the function table: the item callback of 0x000a74d0)
A new text from the arguments of baAddExtraItem: font, x, y, z (doubles), r, g, b, text. */
int FUN_000a6e50(struct BExtraSystem *system, struct BCarFloatyItem *item, void *args)
{
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	item->font = va_arg(ap, struct bdFont *);
	item->pos[0] = (float)va_arg(ap, double);
	item->pos[1] = (float)va_arg(ap, double);
	item->pos[2] = (float)va_arg(ap, double);
	item->pos[3] = 1.0f;
	item->colour[0] = (unsigned char)va_arg(ap, int);
	item->colour[1] = (unsigned char)va_arg(ap, int);
	item->colour[2] = (unsigned char)va_arg(ap, int);
	item->colour[3] = 0x40;
	sprintf(item->text, "%s", va_arg(ap, char *));            /* 0x002576ec */
	bkStringToWide(item->wtext, item->text);
	bdGetStringSize(item->font, item->wtext, (int)strlen(item->text), &item->width, &item->height);
	item->age = 0;
	item->size = g_FloatySystem->big != 0 ? 3 : 1;
	return 0;
}

/* 0x000a6f20  (gap)
The update callback: every text rises; texts older than their life are deleted. `arg`
carries the frame time's 32 bits. */
int BCarFloaty_UpdateItems(struct BExtraSystem *system, int numItems, struct BCarFloatyItem **items,
	void *arg)
{
	struct BCarFloatyItem *item;
	float dt;
	unsigned int bits = (unsigned int)(uintptr_t)arg;
	int i = 0;

	memcpy(&dt, &bits, sizeof dt);
	if (0 < numItems)
	{
		do
		{
			item = items[i];
			if ((float)item->age <= (float)item->size * g_FloatySystem->life * dt * 64.0f) /* 0x00253f38 */
			{
				item->age++;
				i++;
				item->pos[1] = dt * g_FloatySystem->rise + item->pos[1];
			}
			else
			{
				baDeleteExtraItem(system, i);
				numItems--;
			}
		} while (i < numItems);
	}
	return 0;
}

/* 0x000a6fa0  (gap)
The delete callback: nothing to free. Returns 1. */
int BCarFloaty_DeleteItem(void)
{
	return 1;
}

/* the world matrix of a text: at its position, facing the camera, scaled by `scale` */
static void bcb_floaty_matrix(struct BCarFloatyItem *item, float scale)
{
	float t[16];
	float r[16];
	float s[16];
	float m[16];
	float world[16];

	bmMatrixTranslation(t, item->pos[0], item->pos[1], item->pos[2]);
	bmMatrixRotationY(r, g_FloatySystem->angle);
	bmMatrixScaling(s, scale, scale, scale);
	bmMatrixMultiplyCopy(m, t, r);
	bmMatrixMultiplyCopy(world, m, s);
	bdSetWorldMatrix(world);
}

/* a colour channel faded by `fade`, plus `add`, at least 0 (and at most `max`) */
static int bcb_floaty_channel(unsigned char channel, float fade, int add, int max)
{
	int value = BCB_FTOL((float)channel - fade) + add;

	if (value < 0)
		value = 0;
	if (max < value)
		value = max;
	return value;
}

/* 0x000a6fb0  (gap; missing from the function table: the draw callback of 0x000a74d0)
Draws the texts: first into the stencil (value 1), then a brighter copy 1 unit bigger where
the stencil is not 1. The texts fade with their age. */
int FUN_000a6fb0(struct BExtraSystem *system, int numItems, struct BCarFloatyItem **items, void *arg)
{
	struct BCarFloatyItem *item;
	float grow;
	float small;
	float large;
	float fade;
	int i;
	int r;
	int g;
	int b;

	(void)system;
	(void)arg;
	bdSaveRenderStates(g_FloatySystem->saved);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	D3DDevice_SetRenderState_StencilEnable(1);
	bdSetRenderState(3, 0, 0);
	bdSetProjectionMode(1);
	bdSetTexture(0, NULL);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(8, 0, 0);
	bdSetRenderState(9, 0xff, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(7, 2, 0);
	for (i = 0; i < numItems; i++)
	{
		item = items[i];
		grow = 0.5f * (float)item->age + 2.0f;               /* 0x002536d0, 0x00257318 */
		if (item->size == 2)
			grow = grow + 4.0f;                               /* 0x00257c40 */
		small = grow * g_FloatySystem->scale;
		large = (1.0f + grow) * g_FloatySystem->scale;

		/* the text into the stencil */
		fade = 3.0f / ((float)item->size * g_FloatySystem->life) * (float)item->age; /* 0x0025384c */
		r = bcb_floaty_channel(item->colour[0], fade, 0, 0x7fffffff);
		g = bcb_floaty_channel(item->colour[1], fade, 0, 0x7fffffff);
		b = bcb_floaty_channel(item->colour[2], fade, 0, 0x7fffffff);
		D3DDevice_SetRenderState(D3DRS_STENCILREF, 1);
		D3DDevice_SetRenderState(D3DRS_STENCILPASS, 0x1e01);
		D3DDevice_SetRenderState(D3DRS_STENCILWRITEMASK, 1);
		D3DDevice_SetRenderState(D3DRS_STENCILFUNC, 0x207);
		bcb_floaty_matrix(item, small);
		bdPrintFontLite(item->font, item->wtext, (int)strlen(item->text), item->width * -0.5f,
			item->height * 0.5f, r, g, b, item->colour[3], NULL, NULL); /* 0x00257b8c, 0x002536d0 */

		/* the outline outside it */
		D3DDevice_SetRenderState_StencilEnable(1);
		D3DDevice_SetRenderState(D3DRS_STENCILFUNC, 0x205);
		D3DDevice_SetRenderState(D3DRS_STENCILMASK, 1);
		fade = 3.0f / ((float)item->size * g_FloatySystem->life) * (float)item->age;
		r = bcb_floaty_channel(item->colour[0], fade, 0x10, 0x7f);
		g = bcb_floaty_channel(item->colour[1], fade, 0x10, 0x7f);
		b = bcb_floaty_channel(item->colour[2], fade, 0x10, 0x7f);
		bcb_floaty_matrix(item, large);
		bdPrintFontLite(item->font, item->wtext, (int)strlen(item->text), item->width * -0.5f,
			item->height * 0.5f, r, g, b, item->colour[3], NULL, NULL);
	}
	D3DDevice_SetRenderState_StencilEnable(0);
	bdSetWorldIdentity();
	bdRestoreRenderStates(g_FloatySystem->saved);
	return 0;
}

/* 0x000a74d0  (gap)
Constructor: the extra system "floatyscore" of `maxItems` texts. */
void BCarFloatyScore_Init(struct BCarFloatyScore *self, int maxItems)
{
	self->system = baCreateExtraSystem("floatyscore", (int (*)())FUN_000a6e50,
		(int (*)())BCarFloaty_UpdateItems, (int (*)())BCarFloaty_DeleteItem, (int (*)())FUN_000a6fb0,
		(int (*)())FUN_001d5950, 0x88, maxItems, 1, 0);
	g_FloatySystem = self;
	self->rise = 16.0f;
	g_FloatySystem->scale = 1.0f;
	g_FloatySystem->life = 1.0f;
	g_FloatySystem->big = 0;
}
