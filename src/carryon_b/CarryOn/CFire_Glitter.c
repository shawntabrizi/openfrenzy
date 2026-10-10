/*
CarryOn/CFire_Glitter.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Glitter.cpp

Glitter: a pool of point sprites that fall slowly, slow down sideways and twinkle through
64 random colours. The players collect the glitter of the glitter world (variant 2).

0x000d80f0 is tagged with CFire_Glitter.cpp; the other functions work on the same object
(file inferred).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_Glitter.cpp"

/* 0x000d80f0  CarryOn/CFire_Glitter.cpp
Makes `count` bits (all free), their point sprites and the 64 twinkle colours. */
int CFire_Glitter_Init(struct CFire_Glitter *self, int count, float pointSize)
{
	struct bkResource *texture;
	struct CFire_PointSprites *points;
	int i;

	texture = bLoadResource(NULL, "alphatex\\firework_glitter.bmp", 0, 0xdefa);
	self->count = count;
	self->bits = bkHeapAlloc(count * 0x24, CB_FILE, 0x20, 0x2001);
	if (self->bits == NULL)
	{
		self->count = 0;
		return 0;
	}
	memset(self->bits, 0, (size_t)count * 0x24);
	self->free = self->bits;
	self->used = NULL;
	for (i = 0; i < count - 1; i++)
		self->bits[i].next = &self->bits[i + 1];
	self->bits[count - 1].next = NULL;
	points = bkHeapAlloc(0x9c, CB_FILE, 0x32, 0x1001);
	if (points != NULL)
		points = CFire_PointSprites_ctor(points);
	self->points = points;
	CFire_PointSprites_Init(points, 2, texture, count, pointSize);
	for (i = 0; i < 0x40; i++)
		self->colours[i] = DAT_002bd640[cb_rand() & 0x3f] | DAT_002bc898[127] |
			DAT_002bc490[64] | DAT_002bce18[64];   /* DAT_002bca94, DAT_002bc590, DAT_002bcf18 */
	self->colour = 0;
	return 1;
}

/* 0x000d8270  CarryOn/CFire_Glitter.cpp (file inferred)
The bits fall (gravity 0.03, sideways 0.999 a step); they go when their life is over or
below -500. */
void CFire_Glitter_Update(struct CFire_Glitter *self)
{
	struct CFire_GlitterBit **link = &self->used;
	struct CFire_GlitterBit *bit = self->used;
	float life, vy, vx, vz;

	while (bit != NULL)
	{
		life = bit->life - 1.0f;
		bit->life = life;
		vy = bit->vel[1] - 0.03f;                       /* 0x00268ce8 */
		bit->vel[1] = vy;
		vx = bit->vel[0] * 0.999f;                      /* 0x00257b84 */
		bit->vel[0] = vx;
		vz = bit->vel[2] * 0.999f;
		bit->vel[2] = vz;
		bit->pos[0] = vx + bit->pos[0];
		vy = vy + bit->pos[1];
		bit->pos[1] = vy;
		bit->pos[2] = vz + bit->pos[2];
		if (life < 0.0f || vy < -500.0f)                /* 0x00268ce4 */
		{
			*link = bit->next;
			bit->next = self->free;
			self->free = bit;
			bit = *link;
		}
		else
		{
			link = &bit->next;
			bit = bit->next;
		}
	}
}

/* 0x000d8330  CarryOn/CFire_Glitter.cpp (file inferred)
Draws the bits (when the "Fireworks" tweak is on), each in the next twinkle colour. */
void CFire_Glitter_Draw(struct CFire_Glitter *self)
{
	struct CFire_GlitterBit *bit;
	unsigned int colour;

	if (self->used == NULL || DAT_00294114 == 0)
		return;
	bdSetWorldMatrix((float *)DAT_002855f0);
	CFire_PointSprites_Begin(self->points);
	for (bit = self->used; bit != NULL; bit = bit->next)
	{
		colour = self->colours[self->colour];
		self->colour = (self->colour + 1) & 0x3f;
		CB_CALL(FUN_0007f3d0, void (*)(void *, float *, unsigned int))(self->points, bit->pos,
			colour);
	}
	CFire_PointSprites_Flush(self->points);
}

/* 0x000d83b0  CarryOn/CFire_Glitter.cpp (file inferred)
A bit at pos with a random velocity in a cube of `speed`; NULL when the pool is used up.
`unused` is not read. */
struct CFire_GlitterBit *CFire_Glitter_Emit(struct CFire_Glitter *self, float *pos, float unused,
	float size, float speed)
{
	struct CFire_GlitterBit *bit = self->free;
	float half;

	(void)unused;
	if (bit == NULL)
		return NULL;
	self->free = bit->next;
	half = speed * 0.5f;
	bit->next = self->used;
	self->used = bit;
	bit->pos[0] = pos[0];
	bit->pos[1] = pos[1];
	bit->size = size;
	bit->pos[2] = pos[2];
	bit->vel[0] = (float)cb_rand() * speed * 3.0518509e-05f - half;   /* 0x00253740 */
	bit->vel[1] = (float)cb_rand() * speed * 3.0518509e-05f - half;
	bit->life = 1000.0f;
	bit->vel[2] = (float)cb_rand() * speed * 3.0518509e-05f - half;
	return bit;
}

/* 0x000d84c0  CarryOn/CFire_Glitter.cpp (file inferred)
Like CFire_Glitter_Emit with the velocity inside a sphere of diameter `speed`. */
struct CFire_GlitterBit *CFire_Glitter_EmitSphere(struct CFire_Glitter *self, float *pos, float unused,
	float size, float speed)
{
	struct CFire_GlitterBit *bit = self->free;
	float half;
	float limit;
	float vz;

	(void)unused;
	if (bit == NULL)
		return NULL;
	half = speed * 0.5f;
	self->free = bit->next;
	bit->next = self->used;
	self->used = bit;
	limit = speed * speed * 0.25f;                      /* 0x00257b88 */
	bit->pos[0] = pos[0];
	bit->pos[1] = pos[1];
	bit->pos[2] = pos[2];
	bit->size = size;
	do
	{
		bit->vel[0] = (float)cb_rand() * speed * 3.0518509e-05f - half;
		bit->vel[1] = (float)cb_rand() * speed * 3.0518509e-05f - half;
		vz = (float)cb_rand() * speed * 3.0518509e-05f - half;
		bit->vel[2] = vz;
	} while (limit < vz * vz + bit->vel[1] * bit->vel[1] + bit->vel[0] * bit->vel[0]);
	bit->life = 1000.0f;
	return bit;
}

/* 0x000d8600  CarryOn/CFire_Glitter.cpp (file inferred)
Collects the bits in the cylinder (radius, height above pos): each adds 1 to *count, at
most up to 100. Returns *count. */
int CFire_Glitter_Collect(struct CFire_Glitter *self, float *pos, float radius, float height,
	int *count)
{
	struct CFire_GlitterBit **link = &self->used;
	struct CFire_GlitterBit *bit = self->used;
	struct CFire_GlitterBit *next;
	float dx, dz, dy;

	if (bit == NULL)
		return *count;
	do
	{
		next = bit->next;
		dx = bit->pos[0] - pos[0];
		dz = bit->pos[2] - pos[2];
		dy = bit->pos[1] - pos[1];
		if (dz < radius && -radius < dz && dx < radius && -radius < dx && 0.0f < dy &&
			dy < height && dx * dx + dz * dz < radius * radius)
		{
			if (99 < *count)
				break;
			*count = *count + 1;
			*link = bit->next;
			bit->next = self->free;
			self->free = bit;
		}
		else
		{
			link = &bit->next;
		}
		bit = next;
	} while (next != NULL);
	return *count;
}

/* 0x000d8720  CarryOn/CFire_Glitter.cpp (file inferred) */
void CFire_Glitter_Free(struct CFire_Glitter *self)
{
	struct CFire_PointSprites *points;

	bkHeapFree(self->bits);
	self->bits = NULL;
	points = self->points;
	if (points != NULL)
	{
		CFire_PointSprites_Free(points);
		bkHeapFree(points);
	}
}
