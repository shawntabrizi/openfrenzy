/*
Meta/CWater.c  <-  C:\BlitzParty\Game\Src\Meta\CWater.cpp

Water on the Meta board: up to 6 water areas (0x003090b8), each a ClassLib CWaterField
(classlib_fx, a 0x9090-byte rippling surface; its methods are called through its vtable)
over a rectangle or a circle, with a level that can rise or fall, drips that splash at
random times and random drops. Players splash when they move in it.

0x00162fc0 is tagged with CWater.cpp; the other functions here use the same water array
(file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x003090b4  number of water areas */
int DAT_003090b4;
/* 0x003090b8  the water areas */
struct CMetaWater *DAT_003090b8[6];

/* 0x002b0c90-0x002b0ca4  water tweakables (meta_a's copy of the Meta .data block) */
#define DAT_002b0c90 MB_META(0x002b0c90, float)          /* -0.5: splash depth per speed */
#define DAT_002b0c94 MB_META(0x002b0c94, float)          /* 0.8: inner radius per speed */
#define DAT_002b0c98 MB_META(0x002b0c98, float)          /* 2.4: outer radius per speed */
#define DAT_002b0c9c MB_META(0x002b0c9c, unsigned int)   /* 0x50060606: the surface colour */
#define DAT_002b0ca0 MB_META(0x002b0ca0, unsigned int)   /* 0xffffffff: material diffuse */
#define DAT_002b0ca4 MB_META(0x002b0ca4, unsigned int)   /* 0xff333333: material specular */

/* 0x003055dc  the display blends the water (bdisplay) */
extern int DAT_003055dc;
/* 0x002f6b74  the water level other code reads (charlib) */
extern float DAT_002f6b74;
/* 0x002ae578  CWaterField's default parameters (classlib_fx) */
extern char DAT_002ae578[0x40];

/* CWaterField's virtual methods (classlib_fx CWaterField.c) */
#define WF_DELETE(wf, flags) MB_VFN(wf, 0x00, void *(*)(void *, int))(wf, flags)
#define WF_INIT(wf, params) MB_VFN(wf, 0x04, int (*)(void *, void *))(wf, params)
#define WF_RIPPLE(wf) MB_VFN(wf, 0x0c, void (*)(void *))(wf)
#define WF_BUILD(wf) MB_VFN(wf, 0x14, int (*)(void *))(wf)
#define WF_DRAW(wf, blend) MB_VFN(wf, 0x1c, int (*)(void *, int))(wf, blend)
#define WF_DROP(wf, depth) MB_VFN(wf, 0x24, int (*)(void *, float))(wf, depth)
#define WF_SPLASH(wf, x, z, d, i, o) \
	MB_VFN(wf, 0x28, int (*)(void *, float, float, float, float, float))(wf, x, z, d, i, o)

/* 0x00162c40  Meta (file inferred: CWater.cpp)
Clears the water array. */
void MetaWater_Init(void)
{
	int i;

	for (i = 0; i < 6; i++)
		DAT_003090b8[i] = NULL;
	DAT_003090b4 = 0;
}

/* 0x00162c60  Meta (file inferred: CWater.cpp)
`value`, but at most `limit` (either sign) away from zero. */
float MetaWater_Clamp(float value, float limit)
{
	float a;
	float r;

	if (limit <= 0.0f)
		limit = -limit;
	a = value;
	if (value <= 0.0f)
		a = -value;
	r = value;
	if (limit < a)
	{
		r = limit;
		if (value <= 0.0f)
			r = -r;
	}
	return r;
}

/* 0x00162cb0  Meta (file inferred: CWater.cpp)
Per frame: random drops at their interval, then the surface's ripples. */
void CMetaWater_UpdateField(struct CMetaWater *self)
{
	if (self->field == NULL)
		return;
	if (self->dropInterval > 0.0f)
	{
		self->dropAccum = DAT_0028fda8 + self->dropAccum;
		while (self->dropInterval < self->dropAccum)
		{
			self->dropAccum = self->dropAccum - self->dropInterval;
			WF_DROP(self->field, self->dropDepth);
		}
	}
	WF_RIPPLE(self->field);
}

/* 0x00162d30  Meta (file inferred: CWater.cpp)
Draws the water: fog colour, vertices, specular material, blended when the display
wants it. */
void CMetaWater_Draw(struct CMetaWater *self)
{
	if (self->field == NULL)
		return;
	FUN_000438b0();
	MB_I32(self->field, 0x38) =
		((DAT_002b058c * 0x100 + DAT_002b0580) * 0x100 + DAT_002b0584) * 0x100 + DAT_002b0588;
	WF_BUILD(self->field);
	MB_SET_RS(3, 1);
	CWaterField_SetMaterial(DAT_002b0ca0, DAT_002b0ca4, 50.0f, 1);
	MB_SET_RS(6, 1);
	MB_SET_RS(10, DAT_003055dc != 0);
	WF_DRAW(self->field, 0);
	MB_SET_RS(6, 0);
	MB_SET_RS(3, 0);
	CWaterField_ResetMaterial();
	MB_SET_RS(3, MB_U32(DAT_003055d0, 0x48));
}

/* 0x00162e10  Meta (file inferred: CWater.cpp)
A splash at `pos` (x, z) of `strength`. */
void CMetaWater_Splash(struct CMetaWater *self, float *pos, float strength)
{
	WF_SPLASH(self->field, pos[0], pos[2], DAT_002b0c90 * strength, DAT_002b0c94 * strength,
		DAT_002b0c98 * strength);
}

/* 0x00162e60  Meta (file inferred: CWater.cpp)
Random drops of `depth` every `interval` frames (fine surfaces only). */
void CMetaWater_SetDrops(struct CMetaWater *self, float interval, float depth)
{
	if (MB_I32(self->field, 0x18) > 2)
	{
		self->dropDepth = depth;
		self->dropAccum = 0.0f;
		self->dropInterval = interval;
	}
}

/* 0x00162e90  Meta (file inferred: CWater.cpp)
A splash at `pos` on the first water area. */
void MetaWater_Splash(float *pos, float strength)
{
	if (DAT_003090b8[0] != NULL)
		WF_SPLASH(DAT_003090b8[0]->field, pos[0], pos[2], DAT_002b0c90 * strength,
			DAT_002b0c94 * strength, DAT_002b0c98 * strength);
}

/* 0x00162ee0  Meta (file inferred: CWater.cpp)
The water area with `tag`, or NULL. */
struct CMetaWater *MetaWater_Find(int tag)
{
	int i;

	for (i = 0; i < DAT_003090b4; i++)
	{
		if (DAT_003090b8[i]->tag == tag)
			return DAT_003090b8[i];
	}
	return NULL;
}

/* 0x00162f30  Meta (file inferred: CWater.cpp)
Destructor: deletes the surface (the drips have nothing to destroy). */
void CMetaWater_Destruct(struct CMetaWater *self)
{
	int i;

	if (self->field != NULL)
		WF_DELETE(self->field, 1);
	for (i = 0; i < 10; i++)
		FUN_001d5950();
}

/* 0x00162f90  Meta (file inferred: CWater.cpp)
Draws every water area. */
void MetaWater_DrawAll(void)
{
	int i;

	for (i = 0; i < DAT_003090b4; i++)
		CMetaWater_Draw(DAT_003090b8[i]);
}

/* 0x00162fc0  Meta/CWater.cpp
Creates the surface: the default parameters with the corner `pos`, size, cell counts
(`nx`, `nz`), water.bmp and the board's colour. Without a surface when it fails. */
void CMetaWater_CreateField(struct CMetaWater *self, float *pos, int nx, int nz,
	float sizeX, float sizeZ)
{
	char params[0x40];
	void *field;

	memcpy(params, DAT_002ae578, sizeof params);
	MB_I32(params, 0x14) = nx;
	MB_F32(params, 0x00) = pos[0];
	MB_I32(params, 0x18) = nz;
	MB_F32(params, 0x04) = pos[1];
	MB_F32(params, 0x08) = pos[2];
	MB_F32(params, 0x0c) = sizeX;
	MB_F32(params, 0x10) = sizeZ;
	MB_PTR(params, 0x38) = (void *)"water.bmp";
	MB_F32(params, 0x28) = 1.0f;
	MB_U32(params, 0x34) = DAT_002b0c9c;
	MB_F32(params, 0x24) = 0.1f;
	field = MB_NEW(0x9090, "C:\\BlitzParty\\Game\\Src\\Meta\\CWater.cpp", 0x12d);
	if (field == NULL)
		field = NULL;
	else
		field = (void *)((undefined4 *(*)(void *))FUN_00138790)(field);
	self->field = field;
	/* as in the original, a failed allocation calls through NULL */
	if (WF_INIT(field, params) == 0)
	{
		if (self->field != NULL)
			WF_DELETE(self->field, 1);
		self->field = NULL;
	}
}

/* 0x001630c0  Meta (file inferred: CWater.cpp)
Makes the water area with `tag` move to `level` at `speed` per frame. */
void MetaWater_SetLevel(int tag, float level, float speed)
{
	struct CMetaWater *w;
	int i;

	for (i = 0; i < DAT_003090b4; i++)
	{
		if (DAT_003090b8[i]->tag == tag)
		{
			w = DAT_003090b8[i];
			if (w != NULL)
			{
				w->targetLevel = level;
				w->speed = speed;
				return;
			}
			break;
		}
	}
	bkPrintf("Couldn't find water\n");
}

/* 0x00163120  Meta (file inferred: CWater.cpp)
Splashes the water a player is in (its feet `depth` below the surface at least); the
splash grows with the player's speed (at least `minSpeed`). Returns 1 when it splashed. */
int MetaWater_PlayerSplash(void *meta, float depth, float minSpeed)
{
	struct CMetaWater *w;
	void *c;
	float dx;
	float dz;
	float speed;
	int inside;
	int i;

	for (i = 0; i < DAT_003090b4; i++)
	{
		c = MB_PTR(meta, 0x30);
		w = DAT_003090b8[i];
		if (!(depth + MB_F32(c, 0xd4) < w->level))
			continue;
		if (w->round == 0)
			inside = w->minX <= MB_F32(c, 0xd0) && w->minZ <= MB_F32(c, 0xd8) &&
				MB_F32(c, 0xd0) <= w->maxX && MB_F32(c, 0xd8) <= w->maxZ;
		else
		{
			dx = MB_F32(c, 0xd0) - w->minX;
			dz = MB_F32(c, 0xd8) - w->minZ;
			inside = dx * dx + dz * dz <= w->maxX;
		}
		if (!inside)
			continue;
		speed = sqrtf(MB_F32(c, 0x19c) * MB_F32(c, 0x19c) + MB_F32(c, 0x198) * MB_F32(c, 0x198) +
			MB_F32(c, 0x194) * MB_F32(c, 0x194));
		if (speed < minSpeed)
			speed = minSpeed;
		WF_SPLASH(DAT_003090b8[i]->field, MB_F32(c, 0xd0), MB_F32(c, 0xd8),
			speed * DAT_002b0c90, DAT_002b0c94 * speed, DAT_002b0c98 * speed);
		return 1;
	}
	return 0;
}

/* 0x00163260  Meta (file inferred: CWater.cpp)
Counts a drip's timer down; at its end it starts again (random time) and returns 1. */
int MbDrip_Update(struct MbDrip *self)
{
	if (self->timer > 0.0f)
	{
		self->timer = self->timer - DAT_0028fda8;
		if (self->timer <= 0.0f)
		{
			self->timer = (float)mb_rand() * self->timeRange * MB_RAND_SCALE + self->timeMin;
			return 1;
		}
	}
	return 0;
}

/* 0x001632d0  Meta (file inferred: CWater.cpp)
A drip at (x, z) every timeMin..timeMin+timeRange frames, splashing (depth, inner,
outer). */
void MbDrip_Init(struct MbDrip *self, float x, float z, float timeMin, float timeRange,
	float depth, float inner, float outer)
{
	self->x = x;
	self->depth = depth;
	self->z = z;
	self->outer = outer;
	self->inner = inner;
	self->timeRange = timeRange;
	self->timeMin = timeMin;
	self->timer = (self->timeRange * 0.5f + self->timeMin) * (float)mb_rand() * MB_RAND_SCALE +
		1.0f;
}

/* 0x00163350  Meta (file inferred: CWater.cpp)
Per frame: the level moves towards its target, the surface, the drips. */
void CMetaWater_Update(struct CMetaWater *self)
{
	float step;
	float level;
	int i;

	if (self->level != self->targetLevel)
	{
		step = MetaWater_Clamp(self->targetLevel - self->level, DAT_0028fda8 * self->speed);
		level = self->level;
		self->level = step + level;
		if (self->field != NULL)
			MB_F32(self->field, 8) = step + level;
		DAT_002f6b74 = self->level;
	}
	CMetaWater_UpdateField(self);
	if (self->numDrips > 0)
	{
		for (i = 0; i < self->numDrips; i++)
		{
			if (MbDrip_Update(&self->drips[i]) != 0)
				WF_SPLASH(self->field, self->drips[i].x, self->drips[i].z,
					self->drips[i].depth, self->drips[i].inner, self->drips[i].outer);
		}
	}
}

/* 0x001633f0  Meta (file inferred: CWater.cpp)
Constructor: a water area at `pos` of `sizeX` x `sizeZ` (cells `nx` x `nz`) with `tag`; it
sets the board's water level. The 7th area is refused (and deleted). */
struct CMetaWater *CMetaWater_Construct(struct CMetaWater *self, float *pos, int nx, int nz,
	float sizeX, float sizeZ, short tag)
{
	int i;

	for (i = 0; i < 10; i++)
		Meta_ReturnThis(&self->drips[i]);
	if (DAT_003090b4 < 6)
	{
		self->numDrips = 0;
		self->round = 0;
		self->minX = pos[0];
		self->minZ = pos[2];
		self->maxX = sizeX + pos[0];
		self->maxZ = sizeZ + pos[2];
		DAT_002f6b74 = pos[1];
		self->targetLevel = DAT_002f6b74;
		self->level = DAT_002f6b74;
		self->tag = tag;
		self->speed = 0.0f;
		self->dropInterval = 0.0f;
		DAT_003090b8[DAT_003090b4] = self;
		CMetaWater_CreateField(self, pos, nx, nz, sizeX, sizeZ);
		DAT_003090b4++;
		DAT_002b058c = 0x48;
	}
	else
	{
		bkPrintf("TOO MUCH WATER please see me. DJH\n");
		if (self != NULL)
		{
			CMetaWater_Destruct(self);
			MB_DELETE(self);
		}
	}
	return self;
}

/* 0x00163500  Meta (file inferred: CWater.cpp)
Adds a drip (10 at most). */
void CMetaWater_AddDrip(struct CMetaWater *self, float x, float z, float timeMin,
	float timeRange, float strength, float size)
{
	if (self->numDrips < 10)
	{
		MbDrip_Init(&self->drips[self->numDrips], x, z, timeMin, timeRange,
			DAT_002b0c90 * strength, DAT_002b0c94 * size, DAT_002b0c98 * size);
		self->numDrips++;
	}
}

/* 0x00163560  Meta (file inferred: CWater.cpp)
Deletes every water area. */
void MetaWater_DeleteAll(void)
{
	struct CMetaWater *w;
	int i;

	for (i = 0; i < DAT_003090b4; i++)
	{
		w = DAT_003090b8[i];
		if (w != NULL)
		{
			CMetaWater_Destruct(w);
			MB_DELETE(w);
		}
	}
	for (i = 0; i < 6; i++)
		DAT_003090b8[i] = NULL;
	DAT_003090b4 = 0;
}

/* 0x001635b0  Meta (file inferred: CWater.cpp)
Updates every water area. */
void MetaWater_UpdateAll(void)
{
	int i;

	for (i = 0; i < DAT_003090b4; i++)
		CMetaWater_Update(DAT_003090b8[i]);
}
