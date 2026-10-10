/*
TankedUp/CTank_Boom.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Boom.cpp

CTank_Boom (CTank_Game +0xfc4): explosions ("booms"). Each boom leaves a scorch decal on
the ground (10 s) and a flat shock wave ring (1.2 s). 0x001c3180 (Add) is tagged with this
file; the other functions are placed here because of the string "CTank_Boom::Initialise" or
because they walk the same boom list.
*/

#include "tankedup_a.h"

/* a boom (0x34 bytes, from the heap) */
struct TuBoom {
	int active;                  /* +0x00 */
	float pos[4];                /* +0x04 */
	float height;                /* +0x14 height of the boom above the ground */
	float time;                  /* +0x18 seconds since the boom */
	float scorchLife;            /* +0x1c 10 s */
	float unknown20;             /* +0x20 */
	float shockLife;             /* +0x24 1.2 s */
	float scorchRadius;          /* +0x28 570 */
	float angle;                 /* +0x2c the scorch's random rotation (radians) */
	struct TuBoom *next;         /* +0x30 */
};
OF_ASSERT_SIZE(struct TuBoom, 0x34);

/* the boom system (CTank_Game +0xfc4) */
struct TuBoomSystem {
	struct TuBoom *first;        /* +0x00 */
	struct bdTexture *shockwave; /* +0x04 "boom\shockwave.bmp" */
	struct bdTexture *scorch;    /* +0x08 "decal\scorch.bmp" */
};

/* a vertex of FVF 0x152 (position, normal, colour, one texture coordinate) */
struct TuVertex {
	float x, y, z;               /* +0x00 */
	float nx, ny, nz;            /* +0x0c */
	unsigned int colour;         /* +0x18 */
	float u, v;                  /* +0x1c */
};
OF_ASSERT_SIZE(struct TuVertex, 0x24);

/* the scorch quad (0x00315208) and the shock wave quad (0x00315298) */
#define TU_SCORCH_QUAD (&TU_VAR(struct TuVertex, 0x00315208))
#define TU_SHOCK_QUAD (&TU_VAR(struct TuVertex, 0x00315298))

/* a grey colour with alpha: every channel = `level` (0-127) */
static unsigned int boom_colour(int level)
{
	return DAT_002bc898[level] | DAT_002bd640[level] | DAT_002bc490[level] | DAT_002bce18[level];
}

/* 0x001c2e00  TankedUp/CTank_Boom.cpp (inferred: "CTank_Boom::Initialise")
Loads the two textures. Always returns 1. */
int CTankBoom_Initialise(void *self)
{
	struct TuBoomSystem *boom = (struct TuBoomSystem *)self;

	boom->first = NULL;
	boom->shockwave = (struct bdTexture *)bLoadResource(NULL, "boom\\shockwave.bmp", 0, 0xdefa);
	boom->scorch = (struct bdTexture *)bLoadResource(NULL, "decal\\scorch.bmp", 0, 0xdefa);
	if (boom->shockwave == NULL)
		TU_DPRINTF("CTank_Boom::Initialise: Could not load texture 'boom\\shockwave.bmp' \n");
	if (boom->scorch == NULL)
		TU_DPRINTF("CTank_Boom::Initialise: Could not load texture 'decal\\scorch.bmp' \n");
	return 1;
}

/* 0x001c2e70  TankedUp/CTank_Boom.cpp (inferred)
Draws a boom's scorch: a ground quad of radius `scorchRadius`, turned by `angle`, under the
boom, fading out over its life (alpha 127 -> 0). */
void CTankBoom_DrawScorch(void *self, struct TuBoom *node)
{
	struct TuBoomSystem *boom = (struct TuBoomSystem *)self;
	struct TuVertex *quad = TU_SCORCH_QUAD;
	int level = TU_FTOL((1.0f - node->time / node->scorchLife) * 127.0f);
	float y = node->pos[1] - node->height;
	float z = node->pos[2];
	float x = node->pos[0];
	double a = node->angle;
	double r = node->scorchRadius;
	unsigned int colour;

	bdSetTexture(0, boom->scorch);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 3, 0);
	bdSetDepthOffset(-5.0f);
	TUB_Decal_BeginStates();
	TUB_Decal_StencilOn();
	quad[0].x = (float)(cos(a) * r + x);
	quad[0].z = (float)(sin(a) * r + z);
	quad[1].x = (float)(cos(a + 1.57079625f) * r + x);
	quad[1].z = (float)(sin(a + 1.57079625f) * r + z);
	quad[2].x = (float)(cos(a + 4.71238899f) * r + x);
	quad[2].z = (float)(sin(a + 4.71238899f) * r + z);
	quad[3].x = (float)(cos(a + 3.14159250f) * r + x);
	colour = boom_colour(level);
	quad[0].colour = colour;
	quad[3].z = (float)(sin(a + 3.14159250f) * r + z);
	quad[0].u = 1.0f;
	quad[0].v = 1.0f;
	quad[1].u = 1.0f;
	quad[1].v = 0.0f;
	quad[2].u = 0.0f;
	quad[2].v = 1.0f;
	quad[3].u = 0.0f;
	quad[3].v = 0.0f;
	quad[0].y = y;
	quad[1].y = y;
	quad[1].colour = colour;
	quad[2].y = y;
	quad[2].colour = colour;
	quad[3].y = y;
	quad[3].colour = colour;
	bdSetWorldMatrix(DAT_002855f0);
	bdDrawPrimitiveFVF(4, quad, 4, 0x152);
	FUN_0018e640();
	bdPopRenderState(5);
	bdPopRenderState(3);
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdSetDepthOffset(0.0f);
}

/* 0x001c30b0  TankedUp/CTank_Boom.cpp (inferred)
Frees every boom. */
void CTankBoom_Free(void *self)
{
	struct TuBoomSystem *boom = (struct TuBoomSystem *)self;

	while (boom->first != NULL) {
		struct TuBoom *node = boom->first;

		boom->first = node->next;
		TU_DELETE(node);
	}
}

/* 0x001c30d0  TankedUp/CTank_Boom.cpp (inferred)
Ages the booms by `step` frames (1/60 s each) and frees those whose shock wave and scorch
are both over, or that were switched off. Returns `state` (no state change). */
int CTankBoom_Update(void *self, float step, int state)
{
	struct TuBoom **link = &((struct TuBoomSystem *)self)->first;

	if (*link == NULL)
		return state;
	step = step * 0.0166666675f;
	do {
		struct TuBoom *node = *link;
		float time = step + node->time;

		node->time = time;
		if (time <= node->shockLife || time <= node->scorchLife) {
			if (node->active != 0) {
				link = &node->next;
				continue;
			}
		} else {
			node->active = 0;
		}
		*link = node->next;
		TU_DELETE(node);
	} while (*link != NULL);
	return state;
}

/* 0x001c3140  TankedUp/CTank_Boom.cpp (inferred)
Draws the scorches of the active booms. */
void CTankBoom_DrawScorches(void *self)
{
	struct TuBoom *node;

	for (node = ((struct TuBoomSystem *)self)->first; node != NULL; node = node->next)
		if (node->active != 0 && node->time < node->scorchLife)
			CTankBoom_DrawScorch(self, node);
}

/* 0x001c3180  TankedUp/CTank_Boom.cpp
Adds a boom at `pos`, `height` above the ground. The scorch's rotation is a whole degree
from the game's inlined rand() (bKernel's seed 0x00285630). */
void CTankBoom_Add(void *self, float *pos, float height)
{
	struct TuBoomSystem *boom = (struct TuBoomSystem *)self;
	struct TuBoom *node;

	TU_DPRINTF("add new 'boom' pos[1] = %.2f, hight = %.02f\n", (double)pos[1], (double)height);
	node = (struct TuBoom *)TU_NEW(0x34, TU_FILE_BOOM, 0xd3);
	node->height = height;
	node->active = 1;
	node->time = 0.0f;
	node->shockLife = 1.2f;
	node->scorchLife = 10.0f;
	node->scorchRadius = 570.0f;
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	node->angle = (float)(int)(((DAT_00285630 >> 16) & 0x7fff) % 360) * 0.0174532924f;
	node->next = boom->first;
	node->pos[0] = pos[0];
	node->pos[1] = pos[1];
	node->pos[2] = pos[2];
	node->pos[3] = pos[3];
	boom->first = node;
}

/* 0x001c3250  TankedUp/CTank_Boom.cpp (inferred)
Draws a boom's shock wave: four quads (one per quarter turn) of a ring that grows to 650
units and fades out over the shock wave's life. */
void CTankBoom_DrawShockwave(void *self, struct TuBoom *node)
{
	struct TuBoomSystem *boom = (struct TuBoomSystem *)self;
	struct TuVertex *quad = TU_SHOCK_QUAD;
	float matrix[16];
	float radius = 650.0f * (node->time / node->shockLife);
	int level = TU_FTOL((1.0f - node->time / node->shockLife) * 127.0f);
	int i;

	bdSetTexture(0, boom->shockwave);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(3, 0, 0);
	for (i = 0; i < 4; i++) {
		double angle = (double)i * 90.0f * 0.0174532924f;
		double s, c;

		quad[0].x = 0.0f;
		quad[0].y = 0.0f;
		quad[0].z = 0.0f;
		quad[1].y = 0.0f;
		quad[3].y = 0.0f;
		quad[2].y = 0.0f;
		quad[0].u = 1.0f;
		s = sin(angle);
		quad[0].v = 1.0f;
		quad[1].u = 1.0f;
		quad[1].v = 0.0f;
		quad[2].u = 0.0f;
		quad[2].v = 1.0f;
		quad[3].u = 0.0f;
		quad[3].v = 0.0f;
		quad[1].x = (float)(s * radius);
		c = cos(angle);
		quad[0].colour = boom_colour(level);
		c = c * radius;
		quad[1].z = (float)c;
		quad[3].x = (float)((double)quad[1].x + c);
		quad[3].z = (float)(c - (double)quad[1].x);
		quad[2].x = (float)c;
		quad[2].z = -quad[1].x;
		quad[1].colour = quad[0].colour;
		quad[2].colour = quad[0].colour;
		quad[3].colour = quad[0].colour;
		bmMatrixTranslation(matrix, node->pos[0], node->pos[1], node->pos[2]);
		bdSetWorldMatrix(matrix);
		bdDrawPrimitiveFVF(4, quad, 4, 0x152);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(3, 1, 0);
}

/* 0x001c3450  TankedUp/CTank_Boom.cpp (inferred)
Draws the shock waves of the active booms. */
void CTankBoom_DrawShockwaves(void *self)
{
	struct TuBoom *node;

	for (node = ((struct TuBoomSystem *)self)->first; node != NULL; node = node->next)
		if (node->active != 0 && node->time < node->shockLife)
			CTankBoom_DrawShockwave(self, node);
}
