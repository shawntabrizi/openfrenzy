/*
Rowing/CRowing_Sprites.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_Sprites.cpp

The sprite helpers of the Rowing game (an adapted copy of CarryOn's CFire_Sprites.cpp):

- the five sprite lists (0x0030fb40): bdDrawSprite3D quads collected per type and drawn
  in one call each;
- RowQuadSprites: camera-facing quads in a set of vertex buffers (CFire_QuadSprites with
  8 buffers of 28872 vertices);
- RowHelix: the twisting trail behind a boat, a polyline of up to 18 drifting points;
- RowParticles: steam puffs and spray blobs, drawn as quads.

0x001b12c0, 0x001b1870 and 0x001b1c60 are tagged with CRowing_Sprites.cpp; 0x001b1330-
0x001b1ac0 lie between them. The gap functions 0x001b1e00-0x001b2b10 that follow work on
the same classes (file inferred). RowQuadSprites_Add is 0x001b10f0 in rowing_a.
*/

#include "rowing_b.h"

/* the quads of a particle set (rowing_a 0x001b10f0, thiscall) */
#define RowQuadSprites_Add(self, pos, size, colour) \
	RB_CALL(FUN_001b10f0, void (*)(struct RowQuadSprites *, float *, float, unsigned int)) \
		((self), (pos), (size), (colour))

/* ---------- the sprite lists */

/* 0x001b12c0  Rowing/CRowing_Sprites.cpp
Allocates the vertex arrays of the five sprite lists (0x1800 vertices each). */
int RowSprites_Init(void)
{
	struct RowSpriteList *list;
	int i;

	for (i = 0; i < 5; i++) {
		list = &g_RowSpriteLists[i];
		list->verts = RB_NEW(0x2a000, RB_FILE_SPRITES, 0x3c, 0x2001);
		if (list->verts == NULL) {
			bkPrintf("ERROR: Could not allcoate memory for sprite list [%d].\n", i);
			return 0;
		}
		list->count = 0;
		list->texture = NULL;
	}
	return 1;
}

/* 0x001b1330  Rowing/CRowing_Sprites.cpp (bracketed) */
void RowSprites_Free(void)
{
	int i;

	for (i = 0; i < 5; i++) {
		if (g_RowSpriteLists[i].verts != NULL) {
			RB_DELETE(g_RowSpriteLists[i].verts);
			g_RowSpriteLists[i].verts = NULL;
		}
	}
}

/* 0x001b1360  Rowing/CRowing_Sprites.cpp (bracketed)
Adds a sprite to list `type` (6 vertices: the 4 of bdDrawSprite3D with the first and the
last doubled for the strip joins). Returns 0 when the list is full or not visible. */
int RowSprites_Add(int type, float *pos, float width, float height, float angle, void *texture,
	int *colour, float zBias)
{
	struct RowSpriteList *list = &g_RowSpriteLists[type];
	unsigned int *v;

	if (list->verts == NULL)
		return 0;
	if (list->count + 6 < 0x1800) {
		v = (unsigned int *)list->verts + list->count * 7;
		if (bdDrawSprite3D(pos, width, height, angle, (struct bdTexture *)texture, 0, 0, colour[0],
			colour[1], colour[2], colour[3], (float *)(v + 7), zBias) == 1) {
			memcpy(v, v + 7, 7 * 4);
			memcpy(v + 0x23, v + 0x1c, 7 * 4);
			list->count = list->count + 6;
			list->texture = texture;
			return 1;
		}
	} else {
		bkPrintf("ERROR: Run out of sprites of type [%d]\n", type);
	}
	return 0;
}

/* 0x001b1430  Rowing/CRowing_Sprites.cpp (bracketed)
Draws sprite list 0 (opaque). */
void RowSprites_DrawOpaque(void)
{
	struct RowSpriteList *list = &g_RowSpriteLists[0];

	if (list->verts == NULL)
		return;
	bdSetRenderState(6, 0, 0);
	if (list->count != 0) {
		if (list->texture != NULL)
			bdSetTexture(0, (struct bdTexture *)list->texture);
		bdDrawPrimitiveFVF(4, list->verts, list->count, 0x144);
		list->count = 0;
	}
}

/* draws one alpha-blended sprite list */
static void rb_sprites_draw_list(struct RowSpriteList *list)
{
	if (list->texture != NULL)
		bdSetTexture(0, (struct bdTexture *)list->texture);
	bdDrawPrimitiveFVF(4, list->verts, list->count, 0x144);
}

/* 0x001b1490  Rowing/CRowing_Sprites.cpp (bracketed)
Draws sprite lists 1-4 with alpha blending (Blend_SetMode 0, 3, 1, 2). The original does
not reset the count of list 1. */
void RowSprites_DrawBlended(void)
{
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(0);
	if (g_RowSpriteLists[1].verts == NULL)
		return;
	if (g_RowSpriteLists[1].count != 0)
		rb_sprites_draw_list(&g_RowSpriteLists[1]);
	Blend_SetMode(3);
	if (g_RowSpriteLists[3].count != 0) {
		rb_sprites_draw_list(&g_RowSpriteLists[3]);
		g_RowSpriteLists[3].count = 0;
	}
	Blend_SetMode(1);
	if (g_RowSpriteLists[2].count != 0) {
		rb_sprites_draw_list(&g_RowSpriteLists[2]);
		g_RowSpriteLists[2].count = 0;
	}
	Blend_SetMode(2);
	if (g_RowSpriteLists[4].count != 0) {
		rb_sprites_draw_list(&g_RowSpriteLists[4]);
		g_RowSpriteLists[4].count = 0;
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* ---------- RowQuadSprites */

/* 0x001b1600  Rowing/CRowing_Sprites.cpp (bracketed)
Destroys the vertex buffers. */
void RowQuadSprites_Free(struct RowQuadSprites *self)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (self->buffers[i] != NULL) {
			bdVertexBufferDestroy(self->buffers[i]);
			self->buffers[i] = NULL;
		}
	}
}

/* 0x001b1630  Rowing/CRowing_Sprites.cpp (bracketed)
Makes the vertex buffers for `count` quads (one buffer when they fit in 28872 vertices,
else as many as needed, at most 7; a failed one ends the set) and fills in their texture
coordinates. */
int RowQuadSprites_Init(struct RowQuadSprites *self, int blendMode, void *texture, int count)
{
	unsigned int verts = (unsigned int)count * 6;
	unsigned int k;
	float *v;
	int n = 1;
	int i;

	self->vertsPerBuffer = (int)verts;
	if (0x70c8 < verts) {
		n = (int)verts / 0x70c8 + 1;
		self->vertsPerBuffer = 0x70c8;
		if (6 < n)
			n = 7;
	}
	for (i = 0; i < n; i++) {
		if (bdVertexBufferCreate(&self->buffers[i], self->vertsPerBuffer, 0x152, 0, 1) == 0) {
			bkPrintf("======== MALLOC FAILED IN THE ROWING ========\n");
			DbgBreakPoint();
			n = i;
		}
	}
	if (n < 8)
		memset(&self->buffers[n], 0, (size_t)(8 - n) * 4);
	self->blendMode = blendMode;
	self->texture = texture;
	for (i = 0; i < n; i++) {
		v = bdVertexBufferLock(self->buffers[i], 0);
		for (k = 0; k < (unsigned int)self->vertsPerBuffer; k += 6) {
			/* 6 vertices of 9 floats: xyz, normal, colour, uv */
			RB_F32(v, 0x1c) = 1.0f;
			RB_F32(v, 0x20) = 0.0f;
			RB_F32(v, 0x40) = 1.0f;
			RB_F32(v, 0x44) = 0.0f;
			RB_F32(v, 0x64) = 1.0f;
			RB_F32(v, 0x68) = 1.0f;
			RB_F32(v, 0x88) = 0.0f;
			RB_F32(v, 0x8c) = 0.0f;
			RB_F32(v, 0xac) = 0.0f;
			RB_F32(v, 0xb0) = 1.0f;
			RB_F32(v, 0xd0) = 0.0f;
			RB_F32(v, 0xd4) = 1.0f;
			v = (float *)((char *)v + 0xd8);
		}
		bdVertexBufferUnlock(self->buffers[i]);
	}
	self->write = NULL;
	self->buffer = 0;
	self->vertsLeft = self->vertsPerBuffer;
	return 1;
}

/* 0x001b17a0  Rowing/CRowing_Sprites.cpp (bracketed)
Takes the quad corners from the camera's right and up vectors and locks the first
buffer. */
void RowQuadSprites_Begin(struct RowQuadSprites *self)
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

/* ---------- RowHelix */

/* 0x001b1870  Rowing/CRowing_Sprites.cpp
Makes a helix trail in colour (r, g, b); the first one also makes the shared polyline
arrays (widths 10, 20, 30, then 40; colours 64). */
struct RowHelix *RowHelix_ctor(struct RowHelix *self, int r, int g, int b, int arg58)
{
	int i;

	memset(self, 0, 0x68);
	if (g_RowHelixWidths == NULL) {
		g_RowHelixWidths = RB_NEW(0x48, RB_FILE_SPRITES, 0x4a9, 0x2001);
		g_RowHelixScreenWidths = RB_NEW(0x48, RB_FILE_SPRITES, 0x4aa, 0x2001);
		g_RowHelixColours = RB_NEW(0x120, RB_FILE_SPRITES, 0x4ab, 0x2001);
		for (i = 0; i < 0x12; i++) {
			if (i < 3)
				g_RowHelixWidths[i] = (float)(i + 1) * 10.0f;   /* 0x002536d8 */
			else
				g_RowHelixWidths[i] = 40.0f;
			g_RowHelixColours[i * 4 + 0] = 0x40;
			g_RowHelixColours[i * 4 + 1] = 0x40;
			g_RowHelixColours[i * 4 + 2] = 0x40;
			g_RowHelixColours[i * 4 + 3] = 0x40;
		}
	}
	self->colour[0] = r;
	self->colour[1] = g;
	self->colour[2] = b;
	self->arg58 = arg58;
	self->texture = bLoadResource(NULL, "tex_special\\helixtwist.bmp", 0, 0xdefa);
	self->points = RB_NEW(0x120, RB_FILE_SPRITES, 0x4c2, 0x2001);
	self->vels = RB_NEW(0x120, RB_FILE_SPRITES, 0x4c3, 0x2001);
	self->alphas = RB_NEW(0x48, RB_FILE_SPRITES, 0x4c4, 0x2001);
	self->count = 0;
	memset(self->points, 0, 0x120);
	memset(self->vels, 0, 0x120);
	memset(self->alphas, 0, 0x48);
	self->axis[0] = 0.0f;
	self->axis[1] = 1.0f;
	self->axis[2] = 0.0f;
	self->axis[3] = 1.0f;
	bdVertexBufferCreate(&self->vb, 0x28, 0x144, 0, 1);
	return self;
}

/* 0x001b1a10  Rowing/CRowing_Sprites.cpp (bracketed)
Frees the shared polyline arrays and the trail's own arrays and buffer. */
void RowHelix_Free(struct RowHelix *self)
{
	if (g_RowHelixWidths != NULL) {
		RB_DELETE(g_RowHelixWidths);
		g_RowHelixWidths = NULL;
	}
	if (g_RowHelixScreenWidths != NULL) {
		RB_DELETE(g_RowHelixScreenWidths);
		g_RowHelixScreenWidths = NULL;
	}
	if (g_RowHelixColours != NULL) {
		RB_DELETE(g_RowHelixColours);
		g_RowHelixColours = NULL;
	}
	if (self->points != NULL) {
		RB_DELETE(self->points);
		self->points = NULL;
	}
	if (self->vels != NULL) {
		RB_DELETE(self->vels);
		self->vels = NULL;
	}
	if (self->alphas != NULL) {
		RB_DELETE(self->alphas);
		self->alphas = NULL;
	}
	if (self->vb != NULL) {
		bdVertexBufferDestroy(self->vb);
		self->vb = NULL;
	}
}

/* 0x001b1ab0  Rowing/CRowing_Sprites.cpp (bracketed)
Starts the flash to white. */
void RowHelix_Flash(struct RowHelix *self)
{
	self->flash = 1.0f;
}

/* 0x001b1ac0  Rowing/CRowing_Sprites.cpp (bracketed)
Draws a trail of 3 or more points as an additive polyline; the alpha falls off along the
trail. */
void RowHelix_Draw(struct RowHelix *self)
{
	float *verts;
	int r, g, b;
	int i;
	int n;

	if (self->count < 3)
		return;
	r = self->colour[0];
	g = self->colour[1];
	b = self->colour[2];
	if (self->flash > 0.0f) {
		r = r + (int)((float)(0xff - r) * self->flash);
		g = g + (int)((float)(0xff - g) * self->flash);
		b = b + (int)((float)(0xff - b) * self->flash);
	}
	verts = bdVertexBufferLock(self->vb, 4);
	if (verts == NULL)
		return;
	for (i = 0; i < self->count; i++) {
		g_RowHelixColours[i * 4 + 0] = r;
		g_RowHelixColours[i * 4 + 1] = g;
		g_RowHelixColours[i * 4 + 2] = b;
		g_RowHelixColours[i * 4 + 3] = (self->alphas[i] * (self->count - i)) / self->count;
	}
	bdSetWorldMatrix((float *)DAT_002855f0);
	n = bdDrawPolyline3D(self->count, self->points[0].pos, g_RowHelixWidths,
		g_RowHelixScreenWidths, g_RowHelixColours, verts, 0.0f);
	bdVertexBufferUnlock(self->vb);
	if (n == 0)
		return;
	bdSetTexture(0, (struct bdTexture *)self->texture);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 0, 0);
	bdDrawVertexBuffer(4, self->vb, 0, self->count * 2 + 4);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* ---------- RowParticles */

/* 0x001b1c60  Rowing/CRowing_Sprites.cpp
Loads the steam and blob textures and makes `steamCount` steam puffs and `blobCount`
blobs (free lists) with their quad sets. Returns 0 when a pool cannot be allocated. */
int RowParticles_Init(struct RowParticles *self, int unused, int steamCount, int blobCount)
{
	struct RowQuadSprites *quads;
	unsigned int size;
	int i;

	(void)unused;
	self->steamTexture = bLoadResource(NULL, "tex_special\\steam_01.bmp", 0, 0xdefa);
	self->blobTexture = bLoadResource(NULL, "tex_special\\blob.bmp", 0, 0xdefa);
	size = (unsigned int)steamCount * 0x54;
	self->steamPool = RB_NEW(size, RB_FILE_SPRITES, 0x164, 0x2001);
	self->steam = NULL;
	self->steamFree = self->steamPool;
	if (self->steamPool == NULL)
		return 0;
	memset(self->steamPool, 0, size);
	for (i = 0; i < steamCount - 1; i++)
		self->steamPool[i].next = &self->steamPool[i + 1];
	self->steamPool[steamCount - 1].next = NULL;
	quads = RB_NEW(0x68, RB_FILE_SPRITES, 0x177, 0x1001);
	if (quads != NULL)
		memset(quads, 0, 0x68);
	self->steamQuads = quads;
	RowQuadSprites_Init(quads, 1, self->steamTexture, steamCount);

	size = (unsigned int)blobCount * 0x4c;
	self->blobPool = RB_NEW(size, RB_FILE_SPRITES, 0x17e, 0x2001);
	self->blobs = NULL;
	self->blobFree = self->blobPool;
	if (self->blobPool == NULL)
		return 0;
	memset(self->blobPool, 0, size);
	for (i = 0; i < blobCount - 1; i++)
		self->blobPool[i].next = &self->blobPool[i + 1];
	self->blobPool[blobCount - 1].next = NULL;
	quads = RB_NEW(0x68, RB_FILE_SPRITES, 0x191, 0x1001);
	if (quads != NULL)
		memset(quads, 0, 0x68);
	self->blobQuads = quads;
	RowQuadSprites_Init(quads, 2, self->blobTexture, blobCount);
	return 1;
}

/* 0x001b1e00  Rowing (gap; CRowing_Sprites.cpp inferred)
Moves the blobs (size, colour); a blob goes back to the free list when its life is
over. */
void RowParticles_UpdateBlobs(struct RowParticles *self)
{
	struct RowBlob **link = &self->blobs;
	struct RowBlob *blob;

	while ((blob = *link) != NULL) {
		blob->life = blob->life - 1.0f;
		blob->colour[1] = blob->fade[0] + blob->colour[1];
		blob->colour[2] = blob->fade[1] + blob->colour[2];
		blob->colour[3] = blob->fade[2] + blob->colour[3];
		blob->size = blob->grow + blob->size;
		blob->pos[0] = blob->pos[0] + blob->vel[0];
		blob->pos[1] = blob->pos[1] + blob->vel[1];
		blob->pos[2] = blob->pos[2] + blob->vel[2];
		if (blob->life <= 0.0f) {
			*link = blob->next;
			blob->next = self->blobFree;
			self->blobFree = blob;
		} else {
			link = &blob->next;
		}
	}
}

/* 0x001b1e90  Rowing (gap; CRowing_Sprites.cpp inferred)
Moves the steam puffs: they grow, rise, spin and fade in to 64 then out at a third of the
speed; a puff goes back to the free list when it is transparent or its life is over. */
void RowParticles_UpdateSteam(struct RowParticles *self)
{
	struct RowSteam **link = &self->steam;
	struct RowSteam *puff;
	int expired;
	int clear;

	while ((puff = *link) != NULL) {
		puff->life = puff->life - 1.0f;
		expired = puff->life <= 0.0f;
		puff->size = puff->grow + puff->size;
		puff->vel[1] = puff->rise + puff->vel[1];
		puff->angle = puff->spin + puff->angle;
		puff->alpha = puff->alpha + puff->fade;
		clear = puff->alpha < 0.0f;
		if (clear)
			puff->alpha = 0.0f;
		if (0.0f < puff->fade && 64.0f < puff->alpha) {    /* 0x00253f38 */
			puff->alpha = 64.0f;
			puff->fade = puff->fade * -0.333333343f;        /* 0x00275cf0 */
		}
		puff->pos[0] = puff->pos[0] + puff->vel[0];
		puff->pos[1] = puff->pos[1] + puff->vel[1];
		puff->pos[2] = puff->pos[2] + puff->vel[2];
		if (clear || expired) {
			*link = puff->next;
			puff->next = self->steamFree;
			self->steamFree = puff;
		} else {
			link = &puff->next;
		}
	}
}

/* 0x001b1f70  Rowing (gap; CRowing_Sprites.cpp inferred) */
void RowParticles_Update(struct RowParticles *self)
{
	RowParticles_UpdateSteam(self);
	RowParticles_UpdateBlobs(self);
}

/* takes a puff from the free list into the active list at pos */
static struct RowSteam *rb_steam_take(struct RowParticles *self, const float *pos)
{
	struct RowSteam *puff = self->steamFree;

	if (puff == NULL)
		return NULL;
	self->steamFree = puff->next;
	puff->next = self->steam;
	self->steam = puff;
	puff->pos[0] = pos[0];
	puff->pos[1] = pos[1];
	puff->pos[2] = pos[2];
	puff->pos[3] = pos[3];
	return puff;
}

/* 0x001b1f80  Rowing (gap; CRowing_Sprites.cpp inferred)
A grey steam puff at pos rising at 1.5-4.5, of `size`, growing by 5-10 % a step, fading
out over `life` updates (a random 20-70 when life < 1). */
struct RowSteam *RowParticles_AddSteam(struct RowParticles *self, float *pos, float size, float life)
{
	struct RowSteam *puff = rb_steam_take(self, pos);
	float step;

	if (puff == NULL)
		return NULL;
	puff->vel[0] = 0.0f;
	puff->size = size;
	puff->pos[3] = 1.0f;
	puff->vel[2] = 0.0f;
	puff->pad20 = 0.0f;
	puff->rise = 0.0f;
	puff->vel[1] = rb_randf() * 3.0f + 1.5f;                            /* 0x0025384c, 0x002538a0 */
	puff->angle = (float)(rb_rand() % 0x168) * 0.0174532924f;           /* 0x002640f8 */
	puff->spin = (float)((rb_rand() & 2) - 1) * 0.0174532924f;
	puff->grow = rb_randf() * size * 0.0500000007f + size * 0.0500000007f;  /* 0x00258b54 */
	if (1.0f <= life)
		puff->life = life;
	else
		puff->life = rb_randf() * 50.0f + 20.0f;                        /* 0x002559b8, 0x00255530 */
	step = 48.0f / puff->life;                                          /* 0x002657e4 */
	puff->colour[0] = 0x7f;
	puff->colour[1] = 0x7f;
	puff->colour[2] = 0x7f;
	puff->alpha = 48.0f;
	puff->fade = -step;
	return puff;
}

/* 0x001b2150  Rowing (gap; CRowing_Sprites.cpp inferred)
A dark steam puff at pos rising at 3-8 that fades in to 64, then out, over 30-60
updates. */
struct RowSteam *RowParticles_AddSmoke(struct RowParticles *self, float *pos, float size)
{
	struct RowSteam *puff = rb_steam_take(self, pos);

	if (puff == NULL)
		return NULL;
	puff->size = size;
	puff->pos[3] = 1.0f;
	puff->vel[0] = 0.0f;
	puff->vel[2] = 0.0f;
	puff->pad20 = 0.0f;
	puff->rise = 0.0f;
	puff->vel[1] = rb_randf() * 5.0f + 3.0f;                            /* 0x002536c4, 0x0025384c */
	puff->angle = (float)(rb_rand() % 0x168) * 0.0174532924f;
	puff->spin = (float)((rb_rand() & 2) - 1) * 0.0174532924f;
	puff->grow = rb_randf() * size * 0.0500000007f + size * 0.0500000007f;
	puff->alpha = 0.0f;
	puff->colour[0] = 0x28;
	puff->colour[1] = 0x28;
	puff->colour[2] = 0x28;
	puff->life = rb_randf() * 30.0f + 30.0f;                            /* 0x002553a8 */
	puff->fade = 255.0f / puff->life;                                   /* 0x002537d0 */
	return puff;
}

/* 0x001b2300  Rowing (gap; CRowing_Sprites.cpp inferred)
A spray blob near pos (within a quarter of its size) of 0.75-1 x `size`, thrown up at
4-8, in a random bright colour that fades to black over its life (8-16 updates). */
struct RowBlob *RowParticles_AddBlob(struct RowParticles *self, float *pos, float size)
{
	struct RowBlob *blob = self->blobFree;
	float f;
	float c;
	float step;

	if (blob == NULL)
		return NULL;
	self->blobFree = blob->next;
	blob->next = self->blobs;
	self->blobs = blob;
	blob->pos[0] = pos[0];
	blob->pos[1] = pos[1];
	blob->pos[2] = pos[2];
	blob->pos[3] = pos[3];
	blob->pos[3] = 1.0f;
	blob->size = size * 0.75f + size * 0.25f * rb_randf();              /* 0x00253844, 0x00257b88 */
	blob->pos[0] = (blob->size * 0.5f * rb_randf() - blob->size * 0.25f) + blob->pos[0];
	blob->pos[1] = (blob->size * 0.5f * rb_randf() - blob->size * 0.25f) + blob->pos[1];
	blob->vel[0] = 0.0f;
	blob->pos[2] = (blob->size * 0.5f * rb_randf() - blob->size * 0.25f) + blob->pos[2];
	blob->vel[2] = 0.0f;
	blob->pad20 = 0.0f;
	blob->vel[1] = rb_randf() * 4.0f + 4.0f;                            /* 0x00257c40 */
	(void)rb_rand();    /* the original draws a number it does not use */
	f = rb_randf();
	blob->colour[0] = 255.0f;
	f = f * f;
	blob->life = ((1.0f - f) * rb_randf() + 1.0f) * 8.0f;               /* 0x0025552c */
	blob->size = blob->size * 0.5f;
	blob->grow = (size / blob->life) * -0.5f;                           /* 0x00257b8c */
	c = rb_randf() * 64.0f + 63.0f;                                     /* 0x00253f38, 0x002630c8 */
	blob->colour[1] = c;
	blob->colour[2] = c * f;
	blob->colour[3] = rb_randf() * blob->colour[2];
	step = 1.0f / blob->life;
	blob->fade[0] = -(step * blob->colour[1]);
	blob->fade[1] = -(step * blob->colour[2]);
	blob->fade[2] = -(step * blob->colour[3]);
	return blob;
}

/* 0x001b2600  Rowing (gap; CRowing_Sprites.cpp inferred)
Draws the quads added since Begin: every full buffer, then the current one. Saves and
restores the render states 3, 5, 6 and 10. */
void RowQuadSprites_Flush(struct RowQuadSprites *self)
{
	int used = self->vertsPerBuffer - self->vertsLeft;
	unsigned int state5, state3, state6, state10;
	int mode;
	int i;

	bdVertexBufferUnlock(self->buffers[self->buffer]);
	if (used == 0 && self->buffer == 0)
		return;
	if (self->texture != NULL)
		bdSetTexture(0, (struct bdTexture *)self->texture);
	state5 = bdGetRenderState(5, NULL);
	state3 = bdGetRenderState(3, NULL);
	state6 = bdGetRenderState(6, NULL);
	state10 = bdGetRenderState(10, NULL);
	bdSetRenderState(3, 0, 0);
	mode = -1;
	switch (self->blendMode) {
	case 0:
		bdSetRenderState(6, 0, 0);
		break;
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
		break;
	}
	if (mode >= 0) {
		bdSetRenderState(5, 0, 0);
		bdSetRenderState(6, 1, 0);
		Blend_SetMode(mode);
	}
	bdSetWorldMatrix((float *)DAT_002855f0);
	for (i = 0; i <= self->buffer; i++)
		bdDrawVertexBuffer(4, self->buffers[i], 0, i == self->buffer ? used : self->vertsPerBuffer);
	self->write = NULL;
	self->vertsLeft = self->vertsPerBuffer;
	self->buffer = 0;
	bdSetRenderState(5, state5, 0);
	bdSetRenderState(6, state6, 0);
	bdSetRenderState(10, state10, 0);
	bdSetRenderState(3, state3, 0);
	D3DDevice_SetStreamSource(0, NULL, 0x24);
	D3DDevice_InsertFence();
}

/* 0x001b27e0  Rowing (gap; CRowing_Sprites.cpp inferred)
Moves the trail: each velocity follows the one before it by a tenth, the first one
jitters (once per point), every point moves by its velocity; two points at the same
height are split by 0.5. */
void RowHelix_Update(struct RowHelix *self, float unused)
{
	int i;

	(void)unused;
	for (i = 1; i < self->count; i++) {
		self->vels[i].pos[0] = (self->vels[i - 1].pos[0] - self->vels[i].pos[0]) * 0.100000001f
			+ self->vels[i].pos[0];                                     /* 0x002536cc */
		self->vels[i].pos[1] = (self->vels[i - 1].pos[1] - self->vels[i].pos[1]) * 0.100000001f
			+ self->vels[i].pos[1];
		self->vels[i].pos[2] = (self->vels[i - 1].pos[2] - self->vels[i].pos[2]) * 0.100000001f
			+ self->vels[i].pos[2];
	}
	for (i = 0; i < self->count; i++) {
		/* 0x00267ce8, 0x00267ce0 */
		self->vels[0].pos[0] = ((float)rb_rand() - 16384.0f) * 6.10351572e-06f + self->vels[0].pos[0];
		self->vels[0].pos[1] = ((float)rb_rand() - 16384.0f) * 6.10351572e-06f + self->vels[0].pos[1];
		self->vels[0].pos[2] = ((float)rb_rand() - 16384.0f) * 6.10351572e-06f + self->vels[0].pos[2];
		self->points[i].pos[0] = self->points[i].pos[0] + self->vels[i].pos[0];
		self->points[i].pos[1] = self->points[i].pos[1] + self->vels[i].pos[1];
		self->points[i].pos[2] = self->points[i].pos[2] + self->vels[i].pos[2];
	}
	if (self->count != 1) {
		for (i = 0; i < self->count - 1; i++) {
			if (self->points[i].pos[1] == self->points[i + 1].pos[1])
				self->points[i + 1].pos[1] = self->points[i + 1].pos[1] + 0.5f;
		}
	}
}

/* 0x001b2990  Rowing (gap; CRowing_Sprites.cpp inferred)
Pushes a new first point (the others move back one place; at most 18). */
void RowHelix_Push(struct RowHelix *self, float *pos, float *vel, int alpha)
{
	int k;

	self->count = self->count + 1;
	if (0x12 < self->count)
		self->count = 0x12;
	for (k = 0x11; k > 0; k--) {
		self->points[k] = self->points[k - 1];
		self->vels[k] = self->vels[k - 1];
		self->alphas[k] = self->alphas[k - 1];
	}
	memcpy(self->points[0].pos, pos, 16);
	memcpy(self->vels[0].pos, vel, 16);
	self->alphas[0] = alpha;
}

/* 0x001b2a50  Rowing (gap; CRowing_Sprites.cpp inferred)
Adds a point and moves the trail; while the flash lasts (it falls by dt/60) the new point
is opaque. */
void RowHelix_Add(struct RowHelix *self, float dt, float *pos, float *vel, int alpha)
{
	if (self->flash > 0.0f) {
		alpha = 0xff;
		self->flash = self->flash - dt * 0.0166666675f;                 /* 0x00262b7c */
		if (self->flash <= 0.0f)
			self->flash = 0.0f;
	}
	RowHelix_Push(self, pos, vel, alpha);
	RowHelix_Update(self, dt);
}

/* 0x001b2ab0  Rowing (gap; CRowing_Sprites.cpp inferred)
Frees the particle pools and their quad sets. */
void RowParticles_Free(struct RowParticles *self)
{
	struct RowQuadSprites *quads;

	RB_DELETE(self->steamPool);
	quads = self->steamQuads;
	self->steamPool = NULL;
	if (quads != NULL) {
		RowQuadSprites_Free(quads);
		RB_DELETE(quads);
	}
	RB_DELETE(self->blobPool);
	self->blobPool = NULL;
	quads = self->blobQuads;
	if (quads != NULL) {
		RowQuadSprites_Free(quads);
		RB_DELETE(quads);
	}
}

/* 0x001b2b10  Rowing (gap; CRowing_Sprites.cpp inferred)
Draws the blobs, then the steam puffs. */
void RowParticles_Draw(struct RowParticles *self)
{
	struct RowBlob *blob;
	struct RowSteam *puff;
	unsigned int colour;

	bdSetWorldMatrix((float *)DAT_002855f0);
	if (self->blobs != NULL) {
		RowQuadSprites_Begin(self->blobQuads);
		for (blob = self->blobs; blob != NULL; blob = blob->next) {
			colour = DAT_002bd640[(int)blob->colour[3]] | DAT_002bc490[(int)blob->colour[2]];
			colour = colour | DAT_002bce18[(int)blob->colour[1]];
			colour = colour | DAT_002bc898[(int)blob->colour[0]];
			RowQuadSprites_Add(self->blobQuads, blob->pos, blob->size, colour);
		}
		RowQuadSprites_Flush(self->blobQuads);
	}
	if (self->steam == NULL)
		return;
	RowQuadSprites_Begin(self->steamQuads);
	for (puff = self->steam; puff != NULL; puff = puff->next) {
		colour = DAT_002bc898[(int)puff->alpha] | DAT_002bd640[puff->colour[2]]
			| DAT_002bc490[puff->colour[1]] | DAT_002bce18[puff->colour[0]];
		RowQuadSprites_Add(self->steamQuads, puff->pos, puff->size, colour);
	}
	RowQuadSprites_Flush(self->steamQuads);
}
