/*
Meta/CMeta_Rain.c  <-  C:\BlitzParty\Game\Src\Meta\CMeta_Rain.cpp

Rain on the Meta board: rain drops are items of a BACTOR extra system ("rain"; the three
callbacks 0x001642f0, 0x00164460 and 0x00163df0 were missed by Ghidra's first analysis)
that fall to the ground and make splashes; splash sprites live in a block of 1000
(0x003090f4, list 0x00309100); lightning flashes a directional light and thunder follows.

0x001645f0 is tagged with CMeta_Rain.cpp; the functions in front of it work on the same
splashes and drops (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x002b0cac  splash sprites in the block */
int DAT_002b0cac = 1000;
/* 0x002b0cb0  the lightning's blue */
int DAT_002b0cb0 = 0x7f;

/* 0x003090e4  splash2.bmp; 0x003090e8 lightening.bmp */
void *DAT_003090e4;
void *DAT_003090e8;
/* 0x003090f0  splashes alive */
int DAT_003090f0;
/* 0x003090f4  the splash block */
struct MbSplash *DAT_003090f4;
/* 0x003090f8  lightening.wav */
void *DAT_003090f8;
/* 0x003090fc  the lightning light */
struct BLight *DAT_003090fc;
/* 0x00309100  the live splashes (node at +0x38) */
struct MbList DAT_00309100;
/* 0x00309110  a rain drop's line (2 vertices) */
static struct MbVertex DAT_00309110[2];
/* 0x00309158  a splash quad (4 vertices) */
static struct MbVertex DAT_00309158[4];
/* 0x003091e8  thunder is due; 0x003091ec, 0x003091f0 the lightning's red and green;
   0x003091f4 the lightning is on; 0x003091f8 frames to the thunder; 0x003091fc the
   function-static guard of the thunder timer */
int DAT_003091e8;
int DAT_003091ec;
int DAT_003091f0;
int DAT_003091f4;
float DAT_003091f8;
unsigned int DAT_003091fc;

/* 0x002b236c  the "SPLISH" particle type (the particle system's data) */
extern char DAT_002b236c[];

#define SPLASH_FROM_NODE(node) ((struct MbSplash *)((char *)(node) - 0x38))

/* the grey (127) colour channels of the bdisplay tables */
#define GREY127 (DAT_002bce18[127] | DAT_002bc490[127] | DAT_002bd640[127])

/* 0x00163cd0  Meta (file inferred: CMeta_Rain.cpp)
Per frame: a splash grows and fades; at the end of its life it leaves the list. Returns
the list predecessor when it ended, else NULL. */
void *MbSplash_Update(struct MbSplash *self)
{
	void *prev;

	self->life = self->life - DAT_0028fda8;
	if (self->life <= 0.0f)
	{
		prev = MB_LIST_REMOVE(&DAT_00309100, &self->next);
		self->active = 0;
		DAT_003090f0--;
		return prev;
	}
	self->alpha = self->alpha - (self->alpha0 / self->life0) * DAT_0028fda8;
	if (self->alpha < 0.0f)
		self->alpha = 0.0f;
	self->size = DAT_0028fda8 + DAT_0028fda8 + self->size;
	return NULL;
}

/* 0x00163d40  Meta (file inferred: CMeta_Rain.cpp)
Updates every splash. */
void MbSplash_UpdateAll(void)
{
	void *node;
	void *prev;

	for (node = MB_LIST_NEXT(&DAT_00309100, NULL); node != NULL;
		node = MB_LIST_NEXT(&DAT_00309100, node))
	{
		prev = MbSplash_Update(SPLASH_FROM_NODE(node));
		if (prev != NULL)
			node = prev;
	}
}

/* 0x00163d80  Meta (file inferred: CMeta_Rain.cpp)
Ends every splash and frees the block. */
void MbSplash_DeleteAll(void)
{
	void *node;
	void *next;

	node = MB_LIST_NEXT(&DAT_00309100, NULL);
	while (node != NULL)
	{
		next = MB_LIST_REMOVE(&DAT_00309100, node);
		SPLASH_FROM_NODE(node)->active = 0;
		DAT_003090f0--;
		node = MB_LIST_NEXT(&DAT_00309100, next);
	}
	if (DAT_003090f4 != NULL)
	{
		MB_DELETE(DAT_003090f4);
		DAT_003090f4 = NULL;
	}
}

/* 0x00163df0  Meta (file inferred: CMeta_Rain.cpp; missing from the function table)
The rain system's draw callback: each drop is a short line along its fall. */
int MetaRain_DrawDrops(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *p;
	int i;

	MB_SET_RS(6, 1);
	((void (*)(int, int, int))FUN_00046cb0)(7, 3, 0);
	((void (*)(int, int, int))FUN_00046cb0)(10, 0, 0);
	((void (*)(int, int, int))FUN_00046cb0)(3, 0, 0);
	FUN_000438b0();
	bdSetTexture(0, NULL);
	for (i = 0; i < count; i++)
	{
		p = (float *)items[i];
		DAT_00309110[0].x = p[0] + p[0x1e];
		DAT_00309110[0].y = p[0x1f] + p[1];
		DAT_00309110[0].z = p[2] + p[0x20];
		DAT_00309110[0].colour = DAT_002bc898[MB_U8(p, 0x5c)] | GREY127;
		DAT_00309110[1].x = (p[0] + p[0x1e]) - p[0x1a] * 0.5f;
		DAT_00309110[1].y = (p[0x1f] + p[1]) - p[0x1b] * 0.5f;
		DAT_00309110[1].z = (p[2] + p[0x20]) - p[0x1c] * 0.5f;
		DAT_00309110[1].colour = DAT_002bc898[MB_U8(p, 0x5c) >> 1] | GREY127;
		bdDrawPrimitive(2, DAT_00309110, 2);
	}
	FUN_00046d00(10);
	MB_SET_RS(6, 0);
	return 0;
}

/* 0x00163f30  Meta (file inferred: CMeta_Rain.cpp)
Deletes the lightning light. */
void MetaRain_DeleteLight(void)
{
	if (DAT_003090fc != NULL)
	{
		baDeleteLight(DAT_003090fc);
		DAT_003090fc = NULL;
	}
}

/* 0x00163f50  Meta (file inferred: CMeta_Rain.cpp)
Builds a splash quad of half size `size` at `pos` lying on the plane with `normal` (the
5th argument is not used), grey by `alpha` with alpha 50. Like the scorch quad, without
turning and normalising. */
void MbSplash_BuildQuad(struct MbVertex *verts, float *pos, float *normal, float size,
	int unused, float alpha)
{
	float ax;
	float ay;
	float az;
	float ref[3];
	float u[3];
	float v[3];
	float nx;
	float ny;
	float nz;
	float dx;
	float dy;
	float dz;
	unsigned int colour;
	int a;
	char axis = 0;

	ax = normal[0];
	if (normal[0] < 0.0f)
		ax = -ax;
	if (0.0f <= normal[1])
		ay = normal[1];
	else
		ay = -normal[1];
	az = normal[2];
	if (normal[2] < 0.0f)
		az = -az;
	if (!(ay < ax) && !(ay < az))
		axis = 1;
	if (!(az < ay) && !(az < ax))
		axis = 2;
	if (axis == 1)
	{
		ref[0] = 1.0f;
		ref[1] = 0.0f;
		ref[2] = 0.0f;
	}
	else if (axis == 2)
	{
		ref[0] = 0.0f;
		ref[1] = 1.0f;
		ref[2] = 0.0f;
	}
	else
	{
		ref[0] = 0.0f;
		ref[1] = 0.0f;
		ref[2] = 1.0f;
	}
	nx = normal[0];
	ny = normal[1];
	nz = normal[2];
	u[0] = ny * ref[2] - ref[1] * nz;
	u[1] = ref[0] * nz - ref[2] * nx;
	u[2] = ref[1] * nx - ref[0] * ny;
	v[0] = u[1] * nz - u[2] * ny;
	v[1] = u[2] * nx - u[0] * nz;
	v[2] = u[0] * ny - u[1] * nx;
	a = (int)alpha;
	colour = DAT_002bce18[a] | DAT_002bc490[a] | DAT_002bc898[50] | DAT_002bd640[a];
	dx = u[0] * size + v[0] * size;
	dy = u[1] * size + v[1] * size;
	dz = u[2] * size + v[2] * size;
	verts[0].x = pos[0] - dx;
	verts[0].y = pos[1] - dy;
	verts[0].z = pos[2] - dz;
	verts[0].colour = colour;
	verts[0].u = 0.0f;
	verts[0].v = 1.0f;
	verts[3].x = dx + pos[0];
	verts[3].y = dy + pos[1];
	verts[3].z = dz + pos[2];
	verts[3].colour = colour;
	verts[3].u = 1.0f;
	verts[3].v = 0.0f;
	dx = u[0] * size - v[0] * size;
	dy = u[1] * size - v[1] * size;
	dz = u[2] * size - v[2] * size;
	verts[2].x = dx + pos[0];
	verts[2].y = dy + pos[1];
	verts[2].z = dz + pos[2];
	verts[2].colour = colour;
	verts[2].u = 1.0f;
	verts[2].v = 1.0f;
	verts[1].x = pos[0] - dx;
	verts[1].y = pos[1] - dy;
	verts[1].z = pos[2] - dz;
	verts[1].colour = colour;
	verts[1].u = 0.0f;
	verts[1].v = 0.0f;
}

/* 0x00164220  Meta (file inferred: CMeta_Rain.cpp)
Draws a splash on surfaces that face up enough. */
void MbSplash_Draw(struct MbSplash *self)
{
	MbSplash_BuildQuad(DAT_00309158, self->pos, self->normal, self->size, 0, self->alpha);
	if ((double)self->normal[1] > 0.4)                      /* 0x00270218 (a double) */
	{
		bdSetTexture(0, (struct bdTexture *)DAT_003090e4);
		MB_SET_RS(6, 1);
		MB_SET_RS(10, 1);
		MB_SET_RS(3, 0);
		MB_SET_RS(5, 0);
		MB_SET_RS(7, 3);
		bdDrawPrimitive(4, DAT_00309158, 4);
		MB_SET_RS(3, MB_U32(DAT_003055d0, 0x48));
	}
}

/* 0x001642c0  Meta (file inferred: CMeta_Rain.cpp)
Draws every splash. */
void MbSplash_DrawAll(void)
{
	void *node;

	for (node = MB_LIST_NEXT(&DAT_00309100, NULL); node != NULL;
		node = MB_LIST_NEXT(&DAT_00309100, node))
		MbSplash_Draw(SPLASH_FROM_NODE(node));
}

/* 0x001642f0  Meta (file inferred: CMeta_Rain.cpp; missing from the function table)
The rain system's item callback: a new drop. `data` points at 13 words (height, ?, ?,
x range, z range, velocity, ...): the drop starts at a random x, z in the ranges and
traces 1.4 times 8 frames of its fall to find where it lands. */
int MetaRain_InitDrop(struct BExtraSystem *system, void *item, void *data)
{
	float *p = (float *)item;
	int range;

	if (data == NULL)
		return 0;
	memcpy(&p[0x15], data, 13 * sizeof(float));
	range = MB_I32(p, 0x60);
	p[1] = p[0x15];
	p[0] = (float)(mb_rand() % (range * 2)) - (float)range;
	range = MB_I32(p, 0x64);
	p[3] = 1.0f;
	p[2] = (float)(mb_rand() % (range * 2)) - (float)range;
	p[8] = p[0];
	p[9] = p[1];
	p[10] = p[2];
	p[0xb] = p[3];
	p[8] = p[0x1a] * 8.0f + p[8];                          /* 0x0025552c */
	p[9] = p[0x1b] * 8.0f + p[9];
	p[10] = p[0x1c] * 8.0f + p[10];
	p[8] = (p[8] - p[0]) * 1.4f;                          /* 0x00263098 */
	p[9] = (p[9] - p[1]) * 1.4f;
	p[10] = (p[10] - p[2]) * 1.4f;
	/* the original leaves an uninitialised stack word in the fourth component */
	p[0xb] = 0.0f;
	((float (*)(float *, float *, float *, float *))FUN_000b4bd0)(p, &p[8], &p[0x10], &p[0xc]);
	MB_I32(p, 0x50) = 0;
	return 0;
}

/* 0x00164460  Meta (file inferred: CMeta_Rain.cpp; missing from the function table)
The rain system's update callback: drops fall; one that reaches the ground splashes (two
"SPLISH" particles) and goes, so does one that falls below its floor. */
int MetaRain_UpdateDrops(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *p;
	float vel[3];
	int i = 0;
	int n;

	while (i < count)
	{
		p = (float *)items[i];
		if (p[0x16] <= p[1])
		{
			if (p[0x11] <= p[1])
			{
				i++;
				p[0] = p[0x1a] * 0.25f * DAT_0028fda8 + p[0];   /* 0x00257b88 */
				p[1] = p[0x1b] * 0.25f * DAT_0028fda8 + p[1];
				MB_I32(p, 0x50) = MB_I32(p, 0x50) + 1;
				p[2] = p[0x1c] * 0.25f * DAT_0028fda8 + p[2];
				continue;
			}
			for (n = 2; n != 0; n--)
			{
				vel[0] = (float)mb_rand() * DAT_0028fda8 * MB_RAND_SCALE;
				vel[1] = ((float)mb_rand() * MB_RAND_SCALE + 1.5f) * DAT_0028fda8; /* 0x002538a0 */
				vel[2] = (float)mb_rand() * DAT_0028fda8 * MB_RAND_SCALE;
				FUN_00080d60((int)DAT_002b236c, (undefined4)&p[0x10], (undefined4)vel, 0,
					0x3c3c3c);
			}
		}
		baDeleteExtraItem(system, i);
		count--;
	}
	return 0;
}

/* 0x001645f0  Meta/CMeta_Rain.cpp
Starts the rain (`count` drops at most): the splash block, textures, the rain system,
the lightning light and the thunder sound. */
void CMetaRain_Init(struct CMetaRain *self, int count)
{
	int i;

	DAT_003090f4 = (struct MbSplash *)bkHeapAlloc(DAT_002b0cac << 6,
		"C:\\BlitzParty\\Game\\Src\\Meta\\CMeta_Rain.cpp", 0x235, 0x2001);
	if (DAT_003090f4 == NULL)
	{
		bkPrintf("CMetaRain::initRain : Could not malloc enough memory for rain splash block.\n");
		DAT_003090f4 = NULL;
	}
	else
	{
		for (i = 0; i < DAT_002b0cac; i++)
			DAT_003090f4[i].active = 0;
	}
	DAT_003090e4 = bLoadResource(NULL, "splash2.bmp", 0, 0xdefa);
	DAT_003090e8 = bLoadResource(NULL, "lightening.bmp", 0, 0xdefa);
	self->system = baCreateExtraSystem("rain", (int (*)())MetaRain_InitDrop,
		(int (*)())MetaRain_UpdateDrops, (int (*)())FUN_000a6fa0, (int (*)())MetaRain_DrawDrops,
		(int (*)())FUN_001d5950, 0x88, count, 1, 0);
	DAT_003090fc = baCreateLight(NULL, 0);
	self->flashTimer = 240.0f;
	self->flashTime = 40.0f;
	baEnableLight(DAT_003090fc, 0);
	baSetLightColour(DAT_003090fc, 0x7f, 0x7f, 0x7f);
	DAT_003090f8 = bLoadResource(NULL, "lightening.wav", 2, 0xdefa);
}

/* 0x00164730  Meta (file inferred: CMeta_Rain.cpp)
Per frame: lightning every 240 frames flickers the light for 30 frames; thunder follows
50..99 frames later. */
void CMetaRain_UpdateLightning(struct CMetaRain *self)
{
	float dir[3];
	int r;

	if ((DAT_003091fc & 1) == 0)
	{
		DAT_003091fc = DAT_003091fc | 1;
		DAT_003091f8 = (float)(mb_rand() % 0x32 + 0x32);
	}
	if (DAT_003090fc == NULL)
		return;
	if (DAT_003091f4 == 0)
		self->flashTimer = self->flashTimer - DAT_0028fda8;
	if (self->flashTimer <= 0.0f)
	{
		DAT_003091f4 = 1;
		DAT_002b0cb0 = 0x7f;
		DAT_003091f0 = 0x7f;
		DAT_003091ec = 0x7f;
		baSetLightColour(DAT_003090fc, 0x7f, 0x7f, 0x7f);
		baEnableLight(DAT_003090fc, 1);
		dir[0] = -1.0f;
		dir[1] = 0.0f;
		dir[2] = 0.0f;
		baSetDirectionalLight(DAT_003090fc, dir);
		self->flashTimer = 240.0f;
		self->flashTime = 30.0f;
	}
	r = mb_rand();
	if (DAT_003091f4 == 0 || r % 10 < 5)
	{
		baEnableLight(DAT_003090fc, 0);
	}
	else
	{
		baSetLightColour(DAT_003090fc, DAT_003091ec, DAT_003091f0, DAT_002b0cb0);
		baEnableLight(DAT_003090fc, 1);
	}
	if (DAT_003091f4 == 1)
		self->flashTime = self->flashTime - DAT_0028fda8;
	if (DAT_003091e8 != 0)
		DAT_003091f8 = DAT_003091f8 - DAT_0028fda8;
	if (DAT_003091f8 <= 0.0f)
	{
		if (DAT_003090f8 != NULL)
			CLSamples_Play((int)DAT_003090f8, 0xff, 0, 0, NULL);
		DAT_003091e8 = 0;
		DAT_003091f8 = (float)(mb_rand() % 0x32 + 0x32);
	}
	if (self->flashTime <= 0.0f)
	{
		self->flashTime = 40.0f;
		DAT_003091f4 = 0;
		baEnableLight(DAT_003090fc, 0);
		DAT_003091e8 = 1;
	}
}
