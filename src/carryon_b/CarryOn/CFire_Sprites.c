/*
CarryOn/CFire_Sprites.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Sprites.cpp

The sprite helpers of the fireworks:

- CFire_PointSprites: point sprites in a set of vertex buffers (XYZ + colour), the CarryOn
  copy of classlib_fx's CPointSprites with the same layout.
- CFire_QuadSprites: camera-facing quads (6 vertices each, a triangle strip with
  degenerate joins) in a set of vertex buffers.
- CFire_Sparks (vtable 0x00268bd8): sparks that leave a short trail, drawn as beams.
- CFire_Trails: short trails of positions relative to a moving origin, drawn as beams.

0x000d2b00 and 0x000d3f20 are tagged with CFire_Sprites.cpp and 0x000d2ba0-0x000d3c00 lie
between them. The others work on the same classes (file inferred): 0x000d24c0-0x000d2880
just before, 0x000d4030-0x000d4730 just after; the two Add functions 0x000cef60 and
0x000d7670 lie in other files' code (inline functions emitted there).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_Sprites.cpp"

/* 0x00268bd8  vtable of CFire_Sparks */
const void *const vtable_00268bd8[1] = {
	(const void *)CFire_Sparks_dtor,
};

/* the blend mode of a sprite set: 0 opaque, 1-4 alpha blending with the table entry
   0, 1, 3, 2 of Blend_SetMode */
static void cb_sprites_blend(int blendMode)
{
	int mode;

	switch (blendMode)
	{
	case 0:
		bdSetRenderState(6, 0, 0);
		return;
	case 1:
		mode = 0;
		break;
	case 2:
		mode = 1;
		break;
	case 3:
		mode = 3;
		break;
	case 4:
		mode = 2;
		break;
	default:
		return;
	}
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(mode);
}

/* the beam colours of a spark or trail: (colour * age / fadeIn while it fades in) >> 7,
   alpha 0x80 */
static void cb_beam_colour(int *out, const short *colour, short age, short fadeIn)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (age < fadeIn)
			out[i] = ((int)colour[i] * (int)age) / (int)fadeIn;
		else
			out[i] = colour[i];
		out[i] = out[i] >> 7;
	}
	out[3] = 0x80;
}

/* appends a beam's 8 vertices (written at out + 7 dwords by bdDrawBeam3D) as a strip
   piece: the first and the last vertex are doubled for the degenerate joins */
static unsigned int *cb_beam_join(unsigned int *out)
{
	memcpy(out, out + 7, 7 * 4);
	memcpy(out + 0x3f, out + 0x38, 7 * 4);
	return out + 0x46;
}

/* draws the strip pieces written to the locked buffer (additive, no z writes) */
static void cb_beam_draw(struct bdVertexBuffer *vb, unsigned int *start, unsigned int *end)
{
	int n;

	bdVertexBufferUnlock(vb);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(1);
	bdSetRenderState(5, 0, 0);
	n = (int)((char *)end - (char *)start) / 0x1c;
	if (n != 0)
		bdDrawVertexBuffer(4, vb, 0, n);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetProjectionMode(1);
}

/* the vertex buffers of a sprite set: one when they fit in 5688 vertices, else as many
   as needed (at most 28); a failed one ends the set */
static int cb_sprites_create(int *vertsPerBuffer, struct bdVertexBuffer **buffers, unsigned int verts,
	int fvf)
{
	unsigned int n = 1;
	unsigned int i;

	*vertsPerBuffer = (int)verts;
	if (0x1638 < verts)
	{
		n = verts / 0x1638 + 1;
		*vertsPerBuffer = 0x1638;
		if (0x1b < n)
			n = 0x1c;
	}
	for (i = 0; (int)i < (int)n; i++)
	{
		if (bdVertexBufferCreate(&buffers[i], *vertsPerBuffer, fvf, 0, 1) == 0)
		{
			bkPrintf("======== MALLOC FAILED IN THE FIREWORKS ========\n");
			DbgBreakPoint();
			n = i;
		}
	}
	if ((int)n < 0x1e)
		memset(&buffers[n], 0, (0x1e - n) * 4);
	return (int)n;
}

/* 0x000cef60  CarryOn/CFire_Sprites.cpp (file inferred; the code lies before
CFire_Fireworks.cpp)
Adds a quad of `size` at pos; the joins of the strip get colour 0. A full buffer moves on
to the next one (nothing when there is none). */
void CFire_QuadSprites_Add(struct CFire_QuadSprites *self, float *pos, float size, unsigned int colour)
{
	float *v;
	int i;

	if ((unsigned int)self->vertsLeft < 6)
	{
		bdVertexBufferUnlock(self->buffers[self->buffer]);
		i = self->buffer;
		if (self->buffers[i + 1] == NULL)
			return;
		self->buffer = i + 1;
		self->vertsLeft = self->vertsPerBuffer;
		self->write = bdVertexBufferLock(self->buffers[i + 1], 0);
	}
	v = self->write;
	/* 6 vertices of 9 floats: xyz, normal, colour, uv */
	CB_U32(v, 0xcc) = 0;
	CB_U32(v, 0x18) = 0;
	CB_U32(v, 0xa8) = colour;
	CB_U32(v, 0x84) = colour;
	CB_U32(v, 0x60) = colour;
	CB_U32(v, 0x3c) = colour;
	CB_F32(v, 0x24) = size * self->corners[0] + pos[0];
	CB_F32(v, 0x00) = CB_F32(v, 0x24);
	CB_F32(v, 0x48) = size * self->corners[1] + pos[0];
	CB_F32(v, 0x6c) = size * self->corners[2] + pos[0];
	CB_F32(v, 0x90) = size * self->corners[3] + pos[0];
	CB_F32(v, 0xb4) = CB_F32(v, 0x90);
	CB_F32(v, 0x28) = size * self->corners[4] + pos[1];
	CB_F32(v, 0x04) = CB_F32(v, 0x28);
	CB_F32(v, 0x4c) = size * self->corners[5] + pos[1];
	CB_F32(v, 0x70) = size * self->corners[6] + pos[1];
	CB_F32(v, 0x94) = size * self->corners[7] + pos[1];
	CB_F32(v, 0xb8) = CB_F32(v, 0x94);
	CB_F32(v, 0x2c) = size * self->corners[8] + pos[2];
	CB_F32(v, 0x08) = CB_F32(v, 0x2c);
	CB_F32(v, 0x50) = size * self->corners[9] + pos[2];
	CB_F32(v, 0x74) = size * self->corners[10] + pos[2];
	CB_F32(v, 0x98) = size * self->corners[11] + pos[2];
	CB_F32(v, 0xbc) = CB_F32(v, 0x98);
	self->write = (float *)((char *)v + 0xd8);
	self->vertsLeft = self->vertsLeft - 6;
}

/* 0x000d24c0  CarryOn/CFire_Sprites.cpp (file inferred) */
struct CFire_QuadSprites *CFire_QuadSprites_ctor(struct CFire_QuadSprites *self)
{
	memset(self, 0, 0xc8);
	return self;
}

/* 0x000d24e0  CarryOn/CFire_Sprites.cpp (file inferred)
Makes the vertex buffers for `count` quads and fills in their texture coordinates. */
int CFire_QuadSprites_Init(struct CFire_QuadSprites *self, int blendMode, struct bkResource *texture,
	int count)
{
	float *v;
	int n;
	int b;
	unsigned int k;

	n = cb_sprites_create(&self->vertsPerBuffer, self->buffers, (unsigned int)count * 6, 0x152);
	self->blendMode = blendMode;
	self->texture = texture;
	for (b = 0; b < n; b++)
	{
		v = bdVertexBufferLock(self->buffers[b], 0);
		for (k = 0; k < (unsigned int)self->vertsPerBuffer; k += 6)
		{
			CB_F32(v, 0x1c) = 1.0f;
			CB_F32(v, 0x20) = 0.0f;
			CB_F32(v, 0x40) = 1.0f;
			CB_F32(v, 0x44) = 0.0f;
			CB_F32(v, 0x64) = 1.0f;
			CB_F32(v, 0x68) = 1.0f;
			CB_F32(v, 0x88) = 0.0f;
			CB_F32(v, 0x8c) = 0.0f;
			CB_F32(v, 0xac) = 0.0f;
			CB_F32(v, 0xb0) = 1.0f;
			CB_F32(v, 0xd0) = 0.0f;
			CB_F32(v, 0xd4) = 1.0f;
			v = (float *)((char *)v + 0xd8);
		}
		bdVertexBufferUnlock(self->buffers[b]);
	}
	self->vertsLeft = self->vertsPerBuffer;
	self->buffer = 0;
	self->write = NULL;
	return 1;
}

/* 0x000d2650  CarryOn/CFire_Sprites.cpp (file inferred)
Takes the quad corners from the camera's right and up vectors and locks the first
buffer. */
void CFire_QuadSprites_Begin(struct CFire_QuadSprites *self)
{
	float c = DAT_002be330[0];
	float d = DAT_002be330[1];
	float e = DAT_002be330[4];
	float f = DAT_002be330[5];
	float g = DAT_002be330[8];
	float h = DAT_002be330[9];

	self->corners[0] = -c - d;
	self->corners[1] = d - c;
	self->corners[2] = c - d;
	self->corners[3] = c + d;
	self->corners[4] = -e - f;
	self->corners[5] = f - e;
	self->corners[6] = e - f;
	self->corners[7] = e + f;
	self->corners[8] = -g - h;
	self->corners[9] = h - g;
	self->corners[10] = g - h;
	self->corners[11] = g + h;
	self->write = bdVertexBufferLock(self->buffers[0], 0);
	self->vertsLeft = self->vertsPerBuffer;
	self->buffer = 0;
}

/* 0x000d2750  CarryOn/CFire_Sprites.cpp (file inferred) */
struct CFire_PointSprites *CFire_PointSprites_ctor(struct CFire_PointSprites *self)
{
	memset(self, 0, 0x9c);
	return self;
}

/* 0x000d2770  CarryOn/CFire_Sprites.cpp (file inferred)
Destroys the vertex buffers (of point or quad sprites). */
void CFire_PointSprites_Free(struct CFire_PointSprites *self)
{
	int i;

	for (i = 0; i < 0x20; i++)
	{
		if (self->buffers[i] != NULL)
		{
			bdVertexBufferDestroy(self->buffers[i]);
			self->buffers[i] = NULL;
		}
	}
}

/* 0x000d27a0  CarryOn/CFire_Sprites.cpp (file inferred)
Makes the vertex buffers for `count` point sprites. */
int CFire_PointSprites_Init(struct CFire_PointSprites *self, int blendMode, struct bkResource *texture,
	unsigned int count, float pointSize)
{
	self->pointSize = pointSize;
	cb_sprites_create(&self->vertsPerBuffer, self->buffers, count, 0x42);
	self->texture = texture;
	self->buffer = 0;
	self->write = NULL;
	self->blendMode = blendMode;
	self->vertsLeft = self->vertsPerBuffer;
	return 1;
}

/* 0x000d2870  CarryOn/CFire_Sprites.cpp (file inferred) */
struct CFire_Sparks *CFire_Sparks_ctor(struct CFire_Sparks *self)
{
	self->vtbl = vtable_00268bd8;
	return self;
}

/* 0x000d2880  CarryOn/CFire_Sprites.cpp (file inferred)
Draws every spark as a beam from its tail to its head. */
void CFire_Sparks_Draw(struct CFire_Sparks *self)
{
	struct CFire_Spark *spark;
	unsigned int *start;
	unsigned int *out;
	int tail[4];
	int head[4];
	int i;

	bdSetWorldMatrix((float *)DAT_002855f0);
	bdSetTexture(0, (struct bdTexture *)self->texture);
	start = bdVertexBufferLock(self->vb, 4);
	if (start == NULL)
		return;
	out = start;
	for (i = 0; i < self->count; i++)
	{
		spark = &self->sparks[i];
		if (spark->active == 0)
			continue;
		cb_beam_colour(tail, spark->colour, spark->age, spark->fadeIn);
		cb_beam_colour(head, spark->colour2, spark->age, spark->fadeIn);
		if (bdDrawBeam3D(spark->tail, spark->width, spark->width, tail, spark->trail[0],
			spark->width, spark->width, head, NULL, (float *)(out + 7), 0.0f) != 0)
			out = cb_beam_join(out);
	}
	cb_beam_draw(self->vb, start, out);
}

/* 0x000d2b00  CarryOn/CFire_Sprites.cpp
Makes `count` trails (none active) and their vertex buffer. Returns 0 when the buffer
cannot be made. */
int CFire_Trails_Init(struct CFire_Trails *self, struct bkResource *texture, int count)
{
	int i;

	self->count = count;
	self->trails = NULL;
	self->trails = bkHeapAlloc(count * 0xac, CB_FILE, 0x500, 0x2001);
	if (bdVertexBufferCreate(&self->vb, self->count * 10, 0x144, 0, 1) == 0)
		return 0;
	self->texture = texture;
	for (i = 0; i < self->count; i++)
		self->trails[i].active = 0;
	return 1;
}

/* 0x000d2ba0  CarryOn/CFire_Sprites.cpp (bracketed) */
void CFire_Trails_Free(struct CFire_Trails *self)
{
	if (self->vb != NULL)
		bdVertexBufferDestroy(self->vb);
	if (self->trails != NULL)
	{
		bkHeapFree(self->trails);
		self->trails = NULL;
	}
	self->count = 0;
}

/* 0x000d2be0  CarryOn/CFire_Sprites.cpp (bracketed)
Draws every trail with 2 or more positions as a beam from its oldest to its newest
position (relative to its origin). */
void CFire_Trails_Draw(struct CFire_Trails *self)
{
	struct CFire_Trail *trail;
	unsigned int *start;
	unsigned int *out;
	int tail[4];
	int head[4];
	float from[4];
	float to[4];
	int i;

	bdSetWorldMatrix((float *)DAT_002855f0);
	bdSetTexture(0, (struct bdTexture *)self->texture);
	start = bdVertexBufferLock(self->vb, 4);
	if (start == NULL)
		return;
	out = start;
	for (i = 0; i < self->count; i++)
	{
		trail = &self->trails[i];
		if (trail->active == 0)
			continue;
		cb_beam_colour(tail, trail->colour, trail->age, trail->fadeIn);
		cb_beam_colour(head, trail->colour2, trail->age, trail->fadeIn);
		if (1 < trail->length)
		{
			from[0] = trail->trail[trail->length - 1][0] + trail->origin[0];
			from[1] = trail->trail[trail->length - 1][1] + trail->origin[1];
			from[2] = trail->trail[trail->length - 1][2] + trail->origin[2];
			from[3] = 1.0f;
			to[0] = trail->trail[0][0] + trail->origin[0];
			to[1] = trail->trail[0][1] + trail->origin[1];
			to[2] = trail->trail[0][2] + trail->origin[2];
			to[3] = 1.0f;
			if (bdDrawBeam3D(from, trail->width, trail->width, tail, to, trail->width, trail->width,
				head, NULL, (float *)(out + 7), 0.0f) != 0)
				out = cb_beam_join(out);
		}
	}
	cb_beam_draw(self->vb, start, out);
}

/* 0x000d2ec0  CarryOn/CFire_Sprites.cpp (bracketed)
Draws the quads added since Begin: every full buffer, then the current one. */
void CFire_QuadSprites_Flush(struct CFire_QuadSprites *self)
{
	int used = self->vertsPerBuffer - self->vertsLeft;
	int i;

	bdVertexBufferUnlock(self->buffers[self->buffer]);
	if (used == 0 && self->buffer == 0)
		return;
	bdSetRenderState(3, 0, 0);
	cb_sprites_blend(self->blendMode);
	if (self->texture != NULL)
		bdSetTexture(0, (struct bdTexture *)self->texture);
	bdSetWorldMatrix((float *)DAT_002855f0);
	for (i = 0; i <= self->buffer; i++)
		bdDrawVertexBuffer(4, self->buffers[i], 0, i == self->buffer ? used : self->vertsPerBuffer);
	self->write = NULL;
	self->vertsLeft = self->vertsPerBuffer;
	self->buffer = 0;
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	D3DDevice_SetStreamSource(0, NULL, 0x24);
	D3DDevice_InsertFence();
}

/* 0x000d3060  CarryOn/CFire_Sprites.cpp (bracketed)
Locks the first buffer and sets the point sprite texture stage 3 up. */
void CFire_PointSprites_Begin(struct CFire_PointSprites *self)
{
	self->write = bdVertexBufferLock(self->buffers[0], 0);
	self->vertsLeft = self->vertsPerBuffer;
	self->buffer = 0;
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)2, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)3, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)4, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)6, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 4);
	bdSetTexture(0, NULL);
	bdSetTexture(1, NULL);
	bdSetTexture(2, NULL);
	if (self->texture != NULL)
		bdSetTexture(3, (struct bdTexture *)self->texture);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(4, 0, 0);
	cb_sprites_blend(self->blendMode);
	bdSetWorldMatrix((float *)DAT_002855f0);
}

/* 0x000d31f0  CarryOn/CFire_Sprites.cpp (bracketed)
Draws the point sprites of the current buffer (only that one) and resets the set. */
void CFire_PointSprites_Flush(struct CFire_PointSprites *self)
{
	int used = self->vertsPerBuffer - self->vertsLeft;

	bdVertexBufferUnlock(self->buffers[self->buffer]);
	if (used == 0)
		return;
	bdDrawPointSprites(self->buffers[self->buffer], 0, used, self->pointSize);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 1);
	self->write = NULL;
	self->vertsLeft = self->vertsPerBuffer;
	self->buffer = 0;
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	D3DDevice_SetStreamSource(0, NULL, 0x24);
	D3DDevice_InsertFence();
}

/* 0x000d3290  CarryOn/CFire_Sprites.cpp (bracketed)
Scalar deleting destructor (vtable slot 0). */
struct CFire_Sparks *CFire_Sparks_dtor(struct CFire_Sparks *self, unsigned int flags)
{
	self->vtbl = vtable_00268bd8;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* the colours of a new spark: the tail a quarter of the head, both 8.7 fixed point,
   fading to 0 over `life` updates */
static void cb_spark_colours(struct CFire_Spark *spark, int r, int g, int b, int life)
{
	spark->colour[0] = (short)((r / 4) << 7);
	spark->fade[0] = (short)(spark->colour[0] / -life);
	spark->colour[1] = (short)((g / 4) << 7);
	spark->fade[1] = (short)(spark->colour[1] / -life);
	spark->colour[2] = (short)((b / 4) << 7);
	spark->fade[2] = (short)(spark->colour[2] / -life);
	spark->colour2[0] = (short)(r << 7);
	spark->fade2[0] = (short)(spark->colour2[0] / -life);
	spark->colour2[1] = (short)(g << 7);
	spark->fade2[1] = (short)(spark->colour2[1] / -life);
	spark->colour2[2] = (short)(b << 7);
	spark->fade2[2] = (short)(spark->colour2[2] / -life);
}

/* a new spark's trail head and tail at pos */
static void cb_spark_place(struct CFire_Spark *spark, float *pos)
{
	spark->trail[0][0] = pos[0];
	spark->trail[0][1] = pos[1];
	spark->trail[0][2] = pos[2];
	spark->trail[0][3] = 1.0f;
	spark->tail[0] = pos[0];
	spark->tail[1] = pos[1];
	spark->tail[2] = pos[2];
	spark->tail[3] = 1.0f;
}

#define CB_RAND01 3.0518509e-05f    /* 1/32767 (0x00253740) */

/* 0x000d32b0  CarryOn/CFire_Sprites.cpp (bracketed)
A spark at pos living 150 updates, fading in over fadeIn x 150, flying in a random
direction (the sparks' spread). */
void CFire_Sparks_Start(struct CFire_Sparks *self, struct CFire_Spark *spark, float *pos, int r, int g,
	int b, float fadeIn)
{
	int ra, rb, rc, rd, re, rf;

	spark->active = 1;
	spark->age = 0;
	spark->life = 0x96;
	spark->fadeIn = (short)(int)(fadeIn * 150.0f);     /* 0x00262df8 */
	cb_spark_colours(spark, r, g, b, 150);
	spark->width = (float)cb_rand() * CB_RAND01 * 15.0f + 5.0f;
	cb_spark_place(spark, pos);
	ra = cb_rand();
	rb = cb_rand();
	rc = cb_rand();
	rd = cb_rand();
	spark->vel[0] = (float)rd * CB_RAND01 * self->spread * ((float)ra * CB_RAND01 - 0.5f);
	re = cb_rand();
	spark->vel[1] = (float)re * CB_RAND01 * self->spread * ((float)rb * CB_RAND01 - 0.5f);
	rf = cb_rand();
	spark->length = 0;
	spark->maxLength = 8;
	spark->vel[2] = (float)rf * CB_RAND01 * self->spread * ((float)rc * CB_RAND01 - 0.5f);
}

/* 0x000d35c0  CarryOn/CFire_Sprites.cpp (bracketed)
A short spark (35 updates) flying up and out at up to 30. */
void CFire_Sparks_StartSmall(struct CFire_Sparks *self, struct CFire_Spark *spark, float *pos, int r,
	int g, int b)
{
	int ra, rb, rc, rd, re, rf;

	(void)self;
	cb_spark_colours(spark, r, g, b, 35);
	spark->active = 1;
	spark->age = 0;
	spark->life = 0x23;
	spark->fadeIn = 1;
	spark->width = (float)cb_rand() * CB_RAND01 * 5.0f + 2.0f;
	cb_spark_place(spark, pos);
	ra = cb_rand();
	rb = cb_rand();
	rc = cb_rand();
	rd = cb_rand();
	spark->vel[0] = (float)rd * CB_RAND01 * 30.0f * ((float)ra * CB_RAND01 * 0.4f - 0.2f);
	re = cb_rand();
	spark->vel[1] = (float)re * CB_RAND01 * 30.0f * ((float)rb * CB_RAND01 * 0.5f);
	rf = cb_rand();
	spark->length = 0;
	spark->maxLength = 4;
	spark->vel[2] = (float)rf * CB_RAND01 * 30.0f * ((float)rc * CB_RAND01 * 0.4f - 0.2f);
}

/* 0x000d38e0  CarryOn/CFire_Sprites.cpp (bracketed)
A spark (50 updates) with `vel` plus a random velocity of up to 30. */
void CFire_Sparks_StartMoving(struct CFire_Sparks *self, struct CFire_Spark *spark, float *pos,
	float *vel, int r, int g, int b)
{
	int ra, rb, rc, rd, re, rf;

	(void)self;
	cb_spark_colours(spark, r, g, b, 50);
	spark->active = 1;
	spark->age = 0;
	spark->life = 0x32;
	spark->fadeIn = 1;
	spark->width = (float)cb_rand() * CB_RAND01 * 5.0f + 5.0f;
	cb_spark_place(spark, pos);
	ra = cb_rand();
	rb = cb_rand();
	rc = cb_rand();
	rd = cb_rand();
	spark->vel[0] = (float)rd * CB_RAND01 * 30.0f * ((float)ra * CB_RAND01 * 0.4f - 0.2f) + vel[0];
	re = cb_rand();
	spark->vel[1] = (float)re * CB_RAND01 * 30.0f * ((float)rb * CB_RAND01 * 0.4f - 0.2f) + vel[1];
	rf = cb_rand();
	spark->length = 0;
	spark->maxLength = 4;
	spark->vel[2] = (float)rf * CB_RAND01 * 30.0f * ((float)rc * CB_RAND01 * 0.4f - 0.2f) + vel[2];
}

/* 0x000d3c00  CarryOn/CFire_Sprites.cpp (bracketed)
Like CFire_Sparks_StartMoving, wider (10-20) and flying more upwards. */
void CFire_Sparks_StartMoving2(struct CFire_Sparks *self, struct CFire_Spark *spark, float *pos,
	float *vel, int r, int g, int b)
{
	int ra, rb, rc, rd, re, rf;

	(void)self;
	cb_spark_colours(spark, r, g, b, 50);
	spark->active = 1;
	spark->age = 0;
	spark->life = 0x32;
	spark->fadeIn = 1;
	spark->width = (float)cb_rand() * CB_RAND01 * 10.0f + 10.0f;
	cb_spark_place(spark, pos);
	ra = cb_rand();
	rb = cb_rand();
	rc = cb_rand();
	rd = cb_rand();
	spark->vel[0] = (float)rd * CB_RAND01 * 30.0f * ((float)ra * CB_RAND01 * 0.4f - 0.2f) + vel[0];
	re = cb_rand();
	spark->vel[1] = (float)re * CB_RAND01 * 30.0f * ((float)rb * CB_RAND01 * 0.5f + 0.1f) + vel[1];
	rf = cb_rand();
	spark->length = 0;
	spark->maxLength = 4;
	spark->vel[2] = (float)rf * CB_RAND01 * 30.0f * ((float)rc * CB_RAND01 * 0.4f - 0.2f) + vel[2];
}

/* 0x000d3f20  CarryOn/CFire_Sprites.cpp
Makes `count` sparks (none active) and their vertex buffer. Returns 0 when the buffer
cannot be made. */
int CFire_Sparks_Init(struct CFire_Sparks *self, struct bkResource *texture, int count)
{
	float origin[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	int i;

	self->count = count;
	self->sparks = NULL;
	self->gravity = -0.2f;
	self->spread = 45.0f;
	self->drag = 0.97f;
	self->sparks = bkHeapAlloc(count * 0xe8, CB_FILE, 0x39b, 0x2001);
	memset(self->sparks, 0, (size_t)self->count * 0xe8);
	if (bdVertexBufferCreate(&self->vb, self->count * 10, 0x144, 0, 1) == 0)
		return 0;
	self->texture = texture;
	for (i = 0; i < self->count; i++)
	{
		CFire_Sparks_Start(self, &self->sparks[i], origin, 0x80, 100, 0x28, 0.01f);
		self->sparks[i].active = 0;
	}
	return 1;
}

/* 0x000d4030  CarryOn/CFire_Sprites.cpp (file inferred)
Starts `count` free sparks at pos with this spread, in the colour (r -1: a random bright
colour at a third) fading in over fadeIn. */
void CFire_Sparks_Burst(struct CFire_Sparks *self, float *pos, int count, float spread, int r, int g,
	int b, float fadeIn)
{
	float fr, fg, fb;
	int n = 0;
	int i;

	self->spread = spread;
	for (i = 0; i < self->count && n < count; i++)
	{
		if (self->sparks[i].active != 0)
			continue;
		if (r == -1)
		{
			/* (the original keeps the random colour in its argument slots) */
			CFire_RandomColour(&fr, &fg, &fb);
			CFire_Sparks_Start(self, &self->sparks[i], pos, (int)(fr * 0.333333343f),
				(int)(fg * 0.333333343f), (int)(fb * 0.333333343f), fadeIn);
		}
		else
		{
			CFire_Sparks_Start(self, &self->sparks[i], pos, r, g, b, fadeIn);
		}
		n++;
	}
}

/* 0x000d4160  CarryOn/CFire_Sprites.cpp (file inferred)
Starts `count` short sparks at pos in the colour scaled to a brightest channel of 127. */
void CFire_Sparks_Emit(struct CFire_Sparks *self, float *pos, int count, int r, int g, int b)
{
	int most;
	int n = 0;
	int i;

	most = r;
	if (r < g)
		most = g;
	if (most < b)
		most = b;
	for (i = 0; i < self->count; i++)
	{
		if (count <= n)
			return;
		if (self->sparks[i].active == 0)
		{
			CFire_Sparks_StartSmall(self, &self->sparks[i], pos, (r * 0x7f) / most, (g * 0x7f) / most,
				(b * 0x7f) / most);
			n++;
		}
	}
}

/* 0x000d4210  CarryOn/CFire_Sprites.cpp (file inferred)
Starts `count` sparks moving with vel, each in a random bright colour at a third. */
void CFire_Sparks_EmitRandom(struct CFire_Sparks *self, float *pos, float *vel, int count)
{
	float r, g, b;
	int n = 0;
	int i;

	for (i = 0; i < self->count; i++)
	{
		if (count <= n)
			return;
		if (self->sparks[i].active == 0)
		{
			CFire_RandomColour(&r, &g, &b);
			CFire_Sparks_StartMoving(self, &self->sparks[i], pos, vel, (int)(r * 0.333333343f),
				(int)(g * 0.333333343f), (int)(b * 0.333333343f));
			n++;
		}
	}
}

/* 0x000d42c0  CarryOn/CFire_Sprites.cpp (file inferred)
Starts `count` wide sparks moving with vel in the colour scaled to a brightest channel
of 127. */
void CFire_Sparks_EmitMoving(struct CFire_Sparks *self, float *pos, float *vel, int count, int r,
	int g, int b)
{
	int most;
	int n = 0;
	int i;

	most = r;
	if (r < g)
		most = g;
	if (most < b)
		most = b;
	for (i = 0; i < self->count; i++)
	{
		if (count <= n)
			return;
		if (self->sparks[i].active == 0)
		{
			CFire_Sparks_StartMoving2(self, &self->sparks[i], pos, vel, (r * 0x7f) / most,
				(g * 0x7f) / most, (b * 0x7f) / most);
			n++;
		}
	}
}

/* fades a colour channel by its step, not below 0 */
static void cb_fade(short *colour, short step)
{
	*colour = (short)(*colour + step);
	if (*colour < 0)
		*colour = 0;
}

/* 0x000d4370  CarryOn/CFire_Sprites.cpp (file inferred)
Moves every spark (drag, gravity), lengthens its trail and fades its colours; it goes out
when its life is over. */
void CFire_Sparks_Update(struct CFire_Sparks *self, float dt)
{
	struct CFire_Spark *spark;
	short age;
	int i;
	int k;

	for (i = 0; i < self->count; i++)
	{
		spark = &self->sparks[i];
		if (spark->active == 0)
			continue;
		age = spark->age;
		spark->age = (short)(age + 1);
		if (!(age < spark->life))
		{
			spark->active = 0;
			continue;
		}
		if (spark->length < (int)spark->maxLength)
			spark->length++;
		for (k = 8; k > 0; k--)
		{
			spark->trail[k][0] = spark->trail[k - 1][0];
			spark->trail[k][1] = spark->trail[k - 1][1];
			spark->trail[k][2] = spark->trail[k - 1][2];
			spark->trail[k][3] = spark->trail[k - 1][3];
		}
		spark->tail[0] = spark->trail[spark->length][0];
		spark->tail[1] = spark->trail[spark->length][1];
		spark->tail[2] = spark->trail[spark->length][2];
		spark->tail[3] = spark->trail[spark->length][3];
		spark->trail[0][0] = dt * spark->vel[0] + spark->trail[0][0];
		spark->trail[0][1] = dt * spark->vel[1] + spark->trail[0][1];
		spark->trail[0][2] = dt * spark->vel[2] + spark->trail[0][2];
		spark->vel[0] = self->drag * spark->vel[0];
		spark->vel[1] = self->drag * spark->vel[1];
		spark->vel[2] = self->drag * spark->vel[2];
		cb_fade(&spark->colour[0], spark->fade[0]);
		spark->vel[1] = dt * self->gravity + spark->vel[1];
		cb_fade(&spark->colour[1], spark->fade[1]);
		cb_fade(&spark->colour[2], spark->fade[2]);
		cb_fade(&spark->colour2[0], spark->fade2[0]);
		cb_fade(&spark->colour2[1], spark->fade2[1]);
		cb_fade(&spark->colour2[2], spark->fade2[2]);
	}
}

/* 0x000d4600  CarryOn/CFire_Sprites.cpp (file inferred)
Starts one free trail at pos (relative to origin) in the colour (each channel << 7). */
void CFire_Trails_Add(struct CFire_Trails *self, float *pos, float *origin, short r, short g, short b)
{
	struct CFire_Trail *trail;
	int n = 0;
	int i;

	for (i = 0; i < self->count; i++)
	{
		if (0 < n)
			return;
		trail = &self->trails[i];
		if (trail->active != 0)
			continue;
		trail->origin = origin;
		trail->colour2[0] = (short)(r << 7);
		trail->colour2[1] = (short)(g << 7);
		trail->active = 1;
		trail->age = 0;
		trail->life = 10;
		trail->fadeIn = 3;
		trail->colour[0] = 0;
		trail->colour[1] = 0;
		trail->colour[2] = 0;
		trail->colour2[2] = (short)(b << 7);
		n++;
		trail->width = (float)cb_rand() * CB_RAND01 * 5.0f + 5.0f;
		trail->trail[0][0] = pos[0];
		trail->trail[0][1] = pos[1];
		trail->trail[0][2] = pos[2];
		trail->trail[0][3] = 1.0f;
		trail->length = 0;
		trail->maxLength = 4;
	}
}

/* 0x000d4730  CarryOn/CFire_Sprites.cpp (file inferred)
Lengthens every trail and halves its newest position (towards the origin); it goes out
when its life is over. */
void CFire_Trails_Update(struct CFire_Trails *self, float unused)
{
	struct CFire_Trail *trail;
	short age;
	int i;
	int k;

	(void)unused;
	for (i = 0; i < self->count; i++)
	{
		trail = &self->trails[i];
		if (trail->active == 0)
			continue;
		age = trail->age;
		trail->age = (short)(age + 1);
		if (!(age < trail->life))
		{
			trail->active = 0;
			continue;
		}
		if (trail->length < (int)trail->maxLength)
			trail->length++;
		for (k = 7; k > 0; k--)
		{
			trail->trail[k][0] = trail->trail[k - 1][0];
			trail->trail[k][1] = trail->trail[k - 1][1];
			trail->trail[k][2] = trail->trail[k - 1][2];
			trail->trail[k][3] = trail->trail[k - 1][3];
		}
		trail->trail[0][0] = (float)((double)trail->trail[0][0] * 0.5);   /* 0x00257310 (double) */
		trail->trail[0][1] = (float)((double)trail->trail[0][1] * 0.5);
		trail->trail[0][2] = (float)((double)trail->trail[0][2] * 0.5);
	}
}

/* 0x000d7670  CarryOn/CFire_Sprites.cpp (file inferred; the code lies in FWorkTypes.cpp)
Adds a point sprite (the size is not used: the set has one point size). A full buffer
moves on to the next one (nothing when there is none). */
void CFire_PointSprites_Add(struct CFire_PointSprites *self, float *pos, float size,
	unsigned int colour)
{
	float *v;
	int i;

	(void)size;
	if (self->vertsLeft == 0)
	{
		bdVertexBufferUnlock(self->buffers[self->buffer]);
		i = self->buffer;
		if (self->buffers[i + 1] == NULL)
			return;
		self->buffer = i + 1;
		self->vertsLeft = self->vertsPerBuffer;
		self->write = bdVertexBufferLock(self->buffers[i + 1], 0);
	}
	v = self->write;
	CB_U32(v, 0xc) = colour;
	v[0] = pos[0];
	v[1] = pos[1];
	v[2] = pos[2];
	self->write = v + 4;
	self->vertsLeft = self->vertsLeft - 1;
}
