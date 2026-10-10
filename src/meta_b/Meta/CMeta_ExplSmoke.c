/*
Meta/CMeta_ExplSmoke.c  <-  C:\BlitzParty\Game\Src\Meta\CMeta_ExplSmoke.cpp

Explosion smoke: a set of puffs on a sphere that stay where they are and a set that
fly outwards; each puff is a rotating sprite that fades out after its life. All smoke
objects are in the list at 0x00308e6c (node at +0x38).

Only the constructor 0x0015e740 is tagged; the update, draw and list functions in
front of it work on the same objects (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x00308e6c  the smoke objects */
struct MbList DAT_00308e6c;

#define SMOKE_FROM_NODE(node) ((struct CMetaExplSmoke *)((char *)(node) - 0x38))

/* 0x0015e3d0  Meta/CMeta_ExplSmoke.cpp (file inferred)
Per frame: still puffs turn; moving puffs fly and slow down; a puff whose life is over
fades out one alpha step per frame. */
void CMetaExplSmoke_Update(struct CMetaExplSmoke *self)
{
	struct MbSmokePuff *p;
	int i;

	for (i = 0; i < self->numStill; i++)
	{
		p = &self->still[i];
		p->life = p->life - DAT_0028fda8;
		p->angle = p->angle + p->spin;
		if (p->life <= 0.0f && p->alpha > 0)
			p->alpha = p->alpha - 1;
	}
	for (i = 0; i < self->numMoving; i++)
	{
		p = &self->moving[i];
		p->life = p->life - DAT_0028fda8;
		FUN_00050ff0(p->pos, p->pos, 1.0f, p->dir, DAT_0028fda8 * p->speed);
		p->angle = DAT_0028fda8 * p->spin + p->angle;
		p->speed = p->speed - DAT_0028fda8 * p->decel;
		if (p->life <= 0.0f && p->alpha > 0)
			p->alpha = p->alpha - 1;
	}
}

/* writes one puff as 6 strip vertices (a sprite of 4 and two repeated ends that join
   the sprites); returns the number of vertices written */
static int smoke_write_puff(struct CMetaExplSmoke *self, struct MbSmokePuff *p)
{
	if (p->alpha <= 0)
		return 0;
	bdDrawSprite3D(p->pos, 256.0f, 256.0f, p->angle, (struct bdTexture *)self->texture, 0, 0,
		p->r, p->g, p->b, p->alpha, (float *)(self->write + 0x1c), 0.0f);
	memcpy(self->write, self->write + 0x1c, 0x1c);
	memcpy(self->write + 0x8c, self->write + 0x70, 0x1c);
	self->write = self->write + 0xa8;
	return 6;
}

/* 0x0015e4e0  Meta/CMeta_ExplSmoke.cpp (file inferred)
Draws every visible puff from one vertex buffer. */
void CMetaExplSmoke_Draw(struct CMetaExplSmoke *self)
{
	int count = 0;
	int i;
	char *base;

	FUN_00043930((undefined4 *)DAT_002855f0);
	MB_SET_RS(6, 1);
	MB_SET_RS(10, 0);
	MB_SET_RS(5, 0);
	base = (char *)bdVertexBufferLock(self->vb, 4);
	self->base = base;
	self->write = base;
	if (base == NULL)
		return;
	for (i = 0; i < self->numStill; i++)
		count += smoke_write_puff(self, &self->still[i]);
	for (i = 0; i < self->numMoving; i++)
		count += smoke_write_puff(self, &self->moving[i]);
	bdVertexBufferUnlock(self->vb);
	FUN_00043b70(1);
	if (count > 5)
	{
		bdSetTexture(0, (struct bdTexture *)self->texture);
		bdDrawVertexBuffer(4, self->vb, 0, count);
	}
	MB_SET_RS(6, 0);
	MB_SET_RS(5, 1);
	FUN_00043b70(1);
}

/* 0x0015e6e0  Meta/CMeta_ExplSmoke.cpp (file inferred)
Updates every smoke object. */
void MetaExplSmoke_UpdateAll(void)
{
	void *node;

	for (node = MB_LIST_NEXT(&DAT_00308e6c, NULL); node != NULL;
		node = MB_LIST_NEXT(&DAT_00308e6c, node))
		CMetaExplSmoke_Update(SMOKE_FROM_NODE(node));
}

/* 0x0015e710  Meta/CMeta_ExplSmoke.cpp (file inferred)
Draws every smoke object. */
void MetaExplSmoke_DrawAll(void)
{
	void *node;

	for (node = MB_LIST_NEXT(&DAT_00308e6c, NULL); node != NULL;
		node = MB_LIST_NEXT(&DAT_00308e6c, node))
		CMetaExplSmoke_Draw(SMOKE_FROM_NODE(node));
}

/* rand() % 5 - rand() % 10 of one rand() (after one rand() that is thrown away) plus the
   colour channel `base` */
static int smoke_colour(int base)
{
	int r;

	mb_rand();
	r = mb_rand();
	return (r % 5 - r % 10) + base;
}

/* 0x0015e740  Meta/CMeta_ExplSmoke.cpp
Creates explosion smoke at `pos`: `numStill` puffs on a sphere of `radiusStill` and
`numMoving` puffs on a sphere of `radiusMoving` that fly outwards; colour is 0x00BBGGRR
(each channel halved). */
struct CMetaExplSmoke *CMetaExplSmoke_Create(float *pos, int numStill, float radiusStill,
	int numMoving, float radiusMoving, unsigned int colour)
{
	float v[4];
	float a;
	float b;
	int i;
	int red = (int)((colour & 0xff) >> 1);
	int green = (int)(((colour >> 8) & 0xff) >> 1);
	int blue = (int)(((colour >> 16) & 0xff) >> 1);
	struct CMetaExplSmoke *self;
	struct MbSmokePuff *p;

	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 1.0f;
	v[3] = 1.0f;
	self = (struct CMetaExplSmoke *)MB_NEW(0x40,
		"C:\\BlitzParty\\Game\\Src\\Meta\\CMeta_ExplSmoke.cpp", 0xfd);
	if (self == NULL)
		return NULL;
	self->texture = bLoadResource(NULL, "myaismoke00.tga", 0, 0xdefa);
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->numStill = numStill;
	self->numMoving = numMoving;
	self->radiusStill = radiusStill;
	self->radiusMoving = radiusMoving;
	self->still = (struct MbSmokePuff *)bkHeapAlloc(numStill * 0x44,
		"C:\\BlitzParty\\Game\\Src\\Meta\\CMeta_ExplSmoke.cpp", 0x10f, 0x2001);
	self->moving = (struct MbSmokePuff *)bkHeapAlloc(numMoving * 0x44,
		"C:\\BlitzParty\\Game\\Src\\Meta\\CMeta_ExplSmoke.cpp", 0x110, 0x2001);
	for (i = 0; i < numStill; i++)
	{
		p = &self->still[i];
		mb_rand();
		mb_rand();
		a = (float)mb_rand() * MB_RAND_ANGLE;
		b = a - 0.31415924f;                            /* 0x00253878 */
		/* the direction keeps turning from puff to puff */
		FUN_00051310(v, v, b, a, b);
		p->pos[0] = v[0] * radiusStill + pos[0];
		p->pos[1] = v[1] * radiusStill + pos[1];
		p->pos[2] = v[2] * radiusStill + pos[2];
		p->pos[3] = 1.0f;
		p->angle = (float)(mb_rand() % 360) * 0.017453292f;    /* 0x002640f8 */
		if ((mb_rand() & 1) != 0)
			p->spin = ((float)mb_rand() * MB_RAND_SCALE + 0.1f) * 0.017453292f;
		else
			p->spin = -(((float)mb_rand() * MB_RAND_SCALE + 1.0f) * 0.017453292f);
		p->life = 160.0f;
		p->r = smoke_colour(red);
		p->g = smoke_colour(green);
		p->b = smoke_colour(blue);
		p->alpha = 0x14;
	}
	for (i = 0; i < numMoving; i++)
	{
		p = &self->moving[i];
		mb_rand();
		mb_rand();
		a = (float)mb_rand() * 9.587672e-05f;           /* 0x00263514: pi / 32767 */
		b = a - 0.31415924f;
		p->dir[3] = 1.0f;
		p->dir[0] = b;
		p->dir[1] = a;
		p->dir[2] = b;
		FUN_00051310(v, v, b, a, b);
		p->pos[0] = v[0] * radiusMoving + pos[0];
		p->pos[1] = v[1] * radiusMoving + pos[1];
		p->pos[2] = v[2] * radiusMoving + pos[2];
		p->pos[3] = 1.0f;
		p->angle = (float)(mb_rand() % 360) * 0.017453292f;
		if ((mb_rand() & 1) != 0)
			p->spin = ((float)mb_rand() * 6.103702e-05f + 0.1f) * 0.017453292f; /* 0x00268b28 */
		else
			p->spin = -(((float)mb_rand() * 6.103702e-05f + 1.0f) * 0.017453292f);
		p->speed = (float)mb_rand() * 6.103702e-05f;
		p->decel = 0.0f;
		p->life = 80.0f;
		p->r = smoke_colour(red);
		p->g = smoke_colour(green);
		p->b = smoke_colour(blue);
		p->alpha = 0x14;
	}
	MB_LIST_INSERT(&DAT_00308e6c, &self->next, 0);
	bdVertexBufferCreate(&self->vb, (numStill + numMoving) * 6, 0x144, 0, 1);
	return self;
}

/* 0x0015eeb0  Meta/CMeta_ExplSmoke.cpp (file inferred)
Deletes every smoke object. */
void MetaExplSmoke_DeleteAll(void)
{
	void *node;
	void *next;
	struct CMetaExplSmoke *smoke;

	node = MB_LIST_NEXT(&DAT_00308e6c, NULL);
	while (node != NULL)
	{
		next = MB_LIST_REMOVE(&DAT_00308e6c, node);
		smoke = SMOKE_FROM_NODE(node);
		smoke->texture = NULL;
		MB_DELETE(smoke->still);
		smoke->still = NULL;
		MB_DELETE(smoke->moving);
		smoke->moving = NULL;
		bdVertexBufferDestroy(smoke->vb);
		MB_DELETE(smoke);
		node = MB_LIST_NEXT(&DAT_00308e6c, next);
	}
}
