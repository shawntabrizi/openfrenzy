/*
Meta/CMeta_Explosion.c  <-  C:\BlitzParty\Game\Src\Meta\CMeta_Explosion.cpp

Explosions on the Meta board (bombs): three ClassLib particle effects (sparks, flames,
debris; their classes and vtables belong to classlib_fx) and a shock wave ring that
grows over a timer. All explosions are in the list at 0x00308f08 (node at +0x1a0).

0x0015f530 is tagged with CMeta_Explosion.cpp; the constructor, destructor and the
other methods around it work on the same objects (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x00308f08  the explosions */
struct MbList DAT_00308f08;

/* 0x00308e78  the shock wave quad (4 vertices) */
static struct MbVertex DAT_00308e78[4];

#define EXPLOSION_FROM_NODE(node) ((struct CMetaExplosion *)((char *)(node) - 0x1a0))

/* the colour table entries 127 of red, green and blue (bdisplay tables) */
#define GREY127 (DAT_002bce18[127] | DAT_002bc490[127] | DAT_002bd640[127])

/* the effect's virtual methods: +0 release, +4 update(frame time), +8 draw */
#define FX_RELEASE(fx) MB_VFN(fx, 0, void (*)(void *))(fx)
#define FX_UPDATE(fx, dt) MB_VFN(fx, 4, void (*)(void *, float))(fx, dt)
#define FX_DRAW(fx) MB_VFN(fx, 8, void (*)(void *))(fx)

/* 0x0015ef20  Meta/CMeta_Explosion.cpp (file inferred)
Constructor. */
struct CMetaExplosion *CMetaExplosion_Construct(struct CMetaExplosion *self)
{
	((void (*)(void *))FUN_000a8b90)(self->sparks);
	((void (*)(void *))FUN_000a8df0)(self->flames);
	((void (*)(void *))FUN_000a8fa0)(self->debris);
	/* the timer's constructor is the shared "return this" function */
	Meta_ReturnThis(&self->timer);
	self->alphaMax = 127.0f;
	self->alpha = 127.0f;
	self->unk04 = 0;
	self->texExplosion = NULL;
	self->texExplosion2 = NULL;
	self->texShockwave = NULL;
	self->texSparks = NULL;
	self->flags = self->flags & 0xfffffff8u;
	self->unk08 = 100.0f;
	return self;
}

/* 0x0015efc0  Meta/CMeta_Explosion.cpp (file inferred)
Destructor. */
void CMetaExplosion_Destruct(struct CMetaExplosion *self)
{
	/* the timer's destructor (0x001d5950) is empty */
	FUN_001d5950();
	((void (*)(void *))FUN_000a8fb0)(self->debris);
	((void (*)(void *))FUN_000a8e00)(self->flames);
	((void (*)(void *))FUN_000a8ba0)(self->sparks);
}

/* the parameter blocks of the flame and debris effects (classlib_fx) */
struct MbFlameParams
{
	unsigned char flag;         /* +0x00 */
	unsigned char pad01[3];
	float f04;                  /* +0x04 */
	float f08;                  /* +0x08 */
	float f0c;                  /* +0x0c */
	float f10;                  /* +0x10 */
	int i14;                    /* +0x14 */
	int i18;                    /* +0x18 */
	float f1c;                  /* +0x1c */
	int i20;                    /* +0x20 */
	int i24;                    /* +0x24 */
};

struct MbDebrisParams
{
	unsigned char flag;         /* +0x00 */
	unsigned char pad01[3];
	float f04;                  /* +0x04 */
	float f08;                  /* +0x08 */
	float f0c;                  /* +0x0c */
	float f10;                  /* +0x10 */
	float f14;                  /* +0x14 */
	int i18;                    /* +0x18 */
	int i1c;                    /* +0x1c */
	int i20;                    /* +0x20 */
	int i24;                    /* +0x24 */
};

/* 0x0015f030  Meta/CMeta_Explosion.cpp (file inferred)
Loads the textures (once) and sets up the effects: `numSparks` sparks, `numFlames`
flames, `numDebris` debris; then resets the shock wave timer. Returns 1. */
int CMetaExplosion_Setup(struct CMetaExplosion *self, int numSparks, int numFlames,
	int numDebris)
{
	struct MbFlameParams flame;
	struct MbDebrisParams debris;

	if (self->texExplosion == NULL)
		self->texExplosion = bLoadResource(NULL, "explosion.bmp", 0, 0xdefa);
	if (self->texExplosion2 == NULL)
		self->texExplosion2 = bLoadResource(NULL, "explosion2.bmp", 0, 0xdefa);
	if (self->texShockwave == NULL)
		self->texShockwave = bLoadResource(NULL, "shockwave.bmp", 0, 0xdefa);
	if (self->texSparks == NULL)
		self->texSparks = bLoadResource(NULL, "sparkburst01.bmp", 0, 0xdefa);
	if (numSparks > 0)
	{
		((void (*)(void *, void *, int))FUN_000aa270)(self->sparks, NULL, numSparks);
		((void (*)(void *, void *))FUN_000a8ab0)(self->sparks, self->texSparks);
	}
	if (numFlames > 0)
	{
		flame.flag = 0;
		flame.f0c = 0.0f;
		flame.f04 = 14.0f;
		flame.f08 = 0.9f;
		flame.f10 = 40.0f;
		flame.i14 = 0x14;
		flame.i18 = 0x14;
		flame.f1c = 0.01f;
		((void (*)(void *, void *, int))FUN_000aa750)(self->flames, &flame, numFlames);
		((void (*)(void *, void *))FUN_000a8ab0)(self->flames, self->texExplosion);
	}
	if (numDebris > 0)
	{
		debris.flag = 0;
		debris.f0c = 0.005f;
		debris.f04 = 20.0f;
		debris.f08 = 0.95f;
		debris.f10 = 100.0f;
		debris.i18 = 4;
		debris.i1c = 2;
		debris.i20 = 0x1e;
		debris.i24 = 0x14;
		debris.f14 = 0.01f;
		((void (*)(void *, void *, int))FUN_000aab30)(self->debris, &debris, numDebris);
		((void (*)(void *, void *))FUN_000a8ab0)(self->debris, self->texExplosion);
	}
	MbTimer_Reset(&self->timer);
	return 1;
}

/* 0x0015f1d0  Meta/CMeta_Explosion.cpp (file inferred)
Updates every active explosion: the shock wave fades, the effects move on. */
void MetaExplosions_UpdateAll(void)
{
	void *node;
	struct CMetaExplosion *e;

	for (node = MB_LIST_NEXT(&DAT_00308f08, NULL); node != NULL;
		node = MB_LIST_NEXT(&DAT_00308f08, node))
	{
		e = EXPLOSION_FROM_NODE(node);
		if ((e->flags & 4) != 0)
		{
			e->alpha = e->alpha - 28.0f;               /* 0x0026fe9c */
			MbTimer_Update(&e->timer);
			FX_UPDATE(e->sparks, DAT_0028fda8);
			FX_UPDATE(e->flames, DAT_0028fda8);
			FX_UPDATE(e->debris, DAT_0028fda8);
		}
	}
}

/* 0x0015f260  Meta/CMeta_Explosion.cpp (file inferred)
Sets the explosion off at `pos` (float[4]): starts the three effects there and a shock
wave of 0.3 seconds. */
void CMetaExplosion_Start(struct CMetaExplosion *self, float *pos)
{
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	((void (*)(void *, float *))FUN_000a9550)(self->sparks, pos);
	((void (*)(void *))FUN_00102870)(self->sparks);
	self->sparks[0x44] = 1;
	((void (*)(void *, float *))FUN_000a9550)(self->flames, pos);
	((void (*)(void *))FUN_00102870)(self->flames);
	self->flames[0x3c] = 1;
	((void (*)(void *, float *))FUN_000a9550)(self->debris, pos);
	((void (*)(void *))FUN_00102870)(self->debris);
	self->debris[0x3c] = 1;
	MbTimer_SetDuration(&self->timer, 0.3f);
	MbTimer_Start(&self->timer);
	self->flags = self->flags | 4;
}

/* 0x0015f310  Meta/CMeta_Explosion.cpp (file inferred)
Draws the shock wave: four textured quads around the centre, turned by 90 degrees,
grey, alpha from the fading alpha, size from the timer's progress. */
void CMetaExplosion_DrawShockwave(struct CMetaExplosion *self)
{
	float size = MbTimer_GetProgress(&self->timer) * 500.0f;   /* 0x002686fc */
	float ratio = self->alpha / self->alphaMax;
	float matrix[16];
	float angle;
	float s;
	float c;
	unsigned int colour;
	int alpha;
	int i;

	if (ratio < 0.0f)
		ratio = 0.0f;
	alpha = (int)(ratio * 127.0f);                             /* 0x00255348 */
	bdSetTexture(0, (struct bdTexture *)self->texShockwave);
	MB_SET_RS(3, 0);
	MB_SET_RS(6, 1);
	MB_SET_RS(10, 1);
	MB_SET_RS(5, 0);
	MB_SET_RS(7, 2);
	for (i = 0; i < 4; i++)
	{
		angle = (float)i * 90.0f * 0.017453292f;               /* 0x00255180, 0x002640f8 */
		colour = DAT_002bc898[alpha] | GREY127;
		DAT_00308e78[0].x = 0.0f;
		DAT_00308e78[0].y = 0.0f;
		DAT_00308e78[0].z = 0.0f;
		DAT_00308e78[1].y = 0.0f;
		DAT_00308e78[3].y = 0.0f;
		DAT_00308e78[2].y = 0.0f;
		DAT_00308e78[0].u = 1.0f;
		DAT_00308e78[0].v = 1.0f;
		DAT_00308e78[1].u = 1.0f;
		DAT_00308e78[1].v = 0.0f;
		DAT_00308e78[2].u = 0.0f;
		DAT_00308e78[2].v = 1.0f;
		DAT_00308e78[3].u = 0.0f;
		DAT_00308e78[3].v = 0.0f;
		s = sinf(angle) * size;
		c = cosf(angle) * size;
		DAT_00308e78[1].x = s;
		DAT_00308e78[0].colour = colour;
		DAT_00308e78[1].colour = colour;
		DAT_00308e78[2].colour = colour;
		DAT_00308e78[3].colour = colour;
		DAT_00308e78[1].z = c;
		DAT_00308e78[3].x = s + c;
		DAT_00308e78[3].z = c - s;
		DAT_00308e78[2].x = c;
		DAT_00308e78[2].z = -s;
		MB_DUAL(FUN_00050250, void (*)(undefined4 *, undefined4, undefined4, undefined4),
			void (*)(float *, float, float, float),
			((undefined4 *)matrix, mb_fbits(self->pos[0]), mb_fbits(self->pos[1]),
				mb_fbits(self->pos[2])),
			(matrix, self->pos[0], self->pos[1], self->pos[2]));
		FUN_00043930((undefined4 *)matrix);
		bdDrawPrimitive(4, DAT_00308e78, 4);
	}
	MB_SET_RS(6, 0);
	MB_SET_RS(5, 1);
}

/* 0x0015f530  Meta/CMeta_Explosion.cpp
Creates an explosion (0x1a8 bytes) and adds it to the list. */
struct CMetaExplosion *CMetaExplosion_Create(void)
{
	struct CMetaExplosion *self;

	self = (struct CMetaExplosion *)MB_NEW(0x1a8,
		"C:\\BlitzParty\\Game\\Src\\Meta\\CMeta_Explosion.cpp", 0x1c7);
	if (self != NULL)
		self = CMetaExplosion_Construct(self);
	/* as in the original, a failed allocation inserts the node of a NULL object */
	MB_LIST_INSERT(&DAT_00308f08, (char *)self + 0x1a0, 0);
	return self;
}

/* 0x0015f5b0  Meta/CMeta_Explosion.cpp (file inferred)
Draws an active explosion (with a stencil-like state 4 around it). Returns the list
node of the explosion. */
void *CMetaExplosion_Draw(struct CMetaExplosion *self)
{
	if ((self->flags & 4) != 0)
	{
		((void (*)(int, int, int))FUN_00046cb0)(4, 0, 0);
		FX_DRAW(self->sparks);
		FX_DRAW(self->flames);
		FX_DRAW(self->debris);
		if (MbTimer_GetRemaining(&self->timer) > 0.0f)
			CMetaExplosion_DrawShockwave(self);
		FUN_00046d00(4);
	}
	return &self->next;
}

/* 0x0015f620  Meta/CMeta_Explosion.cpp (file inferred)
Draws every explosion (not called by the game). */
void MetaExplosions_DrawAll(void)
{
	void *node;

	node = MB_LIST_NEXT(&DAT_00308f08, NULL);
	while (node != NULL)
	{
		node = CMetaExplosion_Draw(EXPLOSION_FROM_NODE(node));
		node = MB_LIST_NEXT(&DAT_00308f08, node);
	}
}

/* 0x0015f650  Meta/CMeta_Explosion.cpp (file inferred)
Deletes every explosion. */
void MetaExplosions_DeleteAll(void)
{
	void *node;
	void *next;
	struct CMetaExplosion *e;

	node = MB_LIST_NEXT(&DAT_00308f08, NULL);
	while (node != NULL)
	{
		e = EXPLOSION_FROM_NODE(node);
		next = MB_LIST_REMOVE(&DAT_00308f08, node);
		FX_RELEASE(e->sparks);
		FX_RELEASE(e->flames);
		FX_RELEASE(e->debris);
		e->texExplosion = NULL;
		e->texShockwave = NULL;
		e->texSparks = NULL;
		CMetaExplosion_Destruct(e);
		MB_DELETE(e);
		node = MB_LIST_NEXT(&DAT_00308f08, next);
	}
}
