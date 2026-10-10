/*
ClassLib/unplaced.c  (classlib_fx)

ClassLib functions of block 2 whose original source file is not known. Gap functions
whose file could be inferred from the data they use are in that file instead
(CProjector.c, RefBot.c, ...), with a "file inferred" note.
*/

#include "classlib_fx.h"

/* ======================================================================== animated textures
0x00082be0-0x00082f60 (between the BumperCars block and CProjector.cpp) */

/* 0x00082be0  ClassLib (gap)
Advances the frame by speed * scale (frame-rate scaled); with `apply`, shows that frame. */
void AnimTex_Advance(CAnimTex *self, float scale, int apply)
{
	self->frame = DAT_0028fda8 * self->speed * scale + self->frame;
	if (apply != 0)
		FUN_00041600(self->texture, CLF_FTOL(self->frame));
}

/* 0x00082c20  ClassLib (gap)
AnimTex_Advance for every texture of a list (inlined). */
void AnimTex_AdvanceList(CLFList *list, float scale, int apply)
{
	CAnimTex *t;

	for (t = CLF_ListNext(list, NULL); t != NULL; t = CLF_ListNext(list, t)) {
		t->frame = DAT_0028fda8 * t->speed * scale + t->frame;
		if (apply != 0)
			FUN_00041600(t->texture, CLF_FTOL(t->frame));
	}
}

/* 0x00082c80  ClassLib (gap)
Shows the current frame of every texture of a list. */
void AnimTex_ApplyList(CLFList *list)
{
	CAnimTex *t;

	for (t = CLF_ListNext(list, NULL); t != NULL; t = CLF_ListNext(list, t))
		FUN_00041600(t->texture, CLF_FTOL(t->frame));
}

/* 0x00082cc0  ClassLib (gap)
Sets and shows frame `frame` on every texture of a list. */
void AnimTex_SetFrameList(CLFList *list, int frame)
{
	CAnimTex *t;

	t = CLF_ListNext(list, NULL);
	if (t != NULL) {
		do {
			t->frame = (float)frame;
			FUN_00041600(t->texture, frame);
			t = CLF_ListNext(list, t);
		} while (t != NULL);
	}
}

/* 0x00082d10  ClassLib (gap)
Writes a grey version of texture `src` into texture `dst` (they may be the same): the
average of R, G and B, alpha kept. Formats 1/2 are 32-bit ARGB, 3/4 16-bit ARGB1555;
other formats are left unchanged. Rows use the destination width. */
void Texture_CopyGreyscale(int dst, int src)
{
	uint *d32;
	uint *s32;
	ushort *d16;
	ushort *s16;
	uint px;
	uint grey;
	int width;
	int height;
	int pitch;
	int format;
	int srcWidth;
	int srcHeight;
	int srcPitch;
	int srcFormat;
	void *dpix;
	void *spix;
	int x;

	dpix = CLF_LockTexture(dst, 0, &width, &height, &pitch, &format, NULL);
	if (dst == src) {
		srcWidth = width;
		srcHeight = height;
		srcPitch = pitch;
		spix = dpix;
	} else {
		spix = CLF_LockTexture(src, 0, &srcWidth, &srcHeight, &srcPitch, &srcFormat, NULL);
	}
	(void)srcWidth;
	(void)srcHeight;
	if (format == 1 || format == 2) {
		d32 = dpix;
		s32 = spix;
		while (height != 0) {
			height = height - 1;
			for (x = width; x != 0; x = x - 1) {
				px = *s32;
				grey = ((px >> 16 & 0xff) + (px >> 8 & 0xff) + (px & 0xff)) / 3;
				*d32 = (grey << 16) | (grey << 8) | (px & 0xff000000) | grey;
				s32 = s32 + 1;
				d32 = d32 + 1;
			}
			d32 = d32 + (pitch / 4 - width);
			s32 = s32 + (srcPitch / 4 - width);
		}
	} else if (format == 3 || format == 4) {
		d16 = dpix;
		s16 = spix;
		while (height != 0) {
			for (x = width; x != 0; x = x - 1) {
				px = *s16;
				grey = ((px >> 10 & 0x1f) + (px >> 5 & 0x1f) + (px & 0x1f)) / 3;
				*d16 = (ushort)(((grey << 5 | grey) << 5) | (px & 0x8000) | grey);
				s16 = s16 + 1;
				d16 = d16 + 1;
			}
			d16 = d16 + (pitch / 2 - width);
			s16 = s16 + (srcPitch / 2 - width);
			height = height - 1;
		}
	}
	CLF_UnlockTexture(dst);
	if (dst != src)
		CLF_UnlockTexture(src);
}

/* 0x00082f10  ClassLib (gap)
Deletes every texture of a list. */
void AnimTex_DeleteList(CLFList *list)
{
	CAnimTex *t;
	void *prev;

	t = CLF_ListNext(list, NULL);
	while (t != NULL) {
		if (t->list == NULL)
			prev = NULL;
		else
			prev = CLF_ListRemove(t->list, t);
		CLF_Free(t);
		t = CLF_ListNext(list, prev);
	}
}

/* 0x00082f60  ClassLib (gap)
Constructor: finds texture `name` (frees itself and returns when missing) and links
into `list` when given. */
CAnimTex *AnimTex_Init(CAnimTex *self, char *name, float speed, CLFList *list)
{
	self->texture = CLF_FindTexture(0, name, 0, 0xdefa);
	if (self->texture == 0) {
		CLF_Printf("***WARNING*** failed to find texture %s\n", name);
		CLF_Free(self);
		return self;
	}
	self->speed = speed;
	self->frame = 0.0f;
	if (list != NULL) {
		self->list = list;
		CLF_ListInsert(list, self, 0);
	}
	return self;
}

/* ======================================================================== point sprites
0x00084f30-0x00085060 (between RefBot.cpp and SpriteVB.cpp) */

/* bdisplay vertex buffers (assumed prototypes) */
#define PS_CreateVB ((int (*)(int *, int, int, int, int))FUN_0003f500)
#define PS_LockVB ((void *(*)(int, int))FUN_0003f710)

/* 0x00084f30  ClassLib (gap)
Constructor: enough vertex buffers of at most 16000 vertices for `numVerts` vertices
(at most 28 buffers). */
CPointSprites *PointSprites_Init(CPointSprites *self, int blendMode, int texture, uint numVerts,
                                 int param94)
{
	uint buffers;
	uint i;

	memset(self, 0, 0x27 * 4);
	self->param94 = param94;
	self->vertsPerBuffer = numVerts;
	buffers = 1;
	if (16000 < numVerts) {
		buffers = numVerts / 16000 + 1;
		self->vertsPerBuffer = 16000;
		if (0x1b < buffers)
			buffers = 0x1c;
	}
	i = 0;
	if (buffers != 0) {
		do {
			if (PS_CreateVB(&self->buffers[i], self->vertsPerBuffer, 0x42, 0, 1) == 0) {
				CLF_Printf("======== VERTEX BUFFER CREATE FAILED IN THE POINTSPRITES ========\n");
				DbgBreakPoint();
				buffers = i;
			}
			i = i + 1;
		} while ((int)i < (int)buffers);
	}
	if ((int)buffers < 0x1e)
		memset(&self->buffers[buffers], 0, (0x1e - buffers) * 4);
	self->texture = texture;
	self->buffer = 0;
	self->write = NULL;
	self->blendMode = blendMode;
	self->vertsLeft = self->vertsPerBuffer;
	return self;
}

/* 0x00085000  ClassLib (gap)
Sets blend mode `mode` of the table at 0x00290e38 (D3DDevice_SetRenderState inlined:
blend op, source blend, destination blend). */
void Blend_SetMode(int mode)
{
	uint v;

	v = DAT_00290e38[mode][2];
	D3DDevice_SetRenderState_Simple(0x40350, v);
	D3D__RenderState[D3DRS_BLENDOP] = v;
	v = DAT_00290e38[mode][0];
	D3DDevice_SetRenderState_Simple(0x40344, v);
	D3D__RenderState[D3DRS_SRCBLEND] = v;
	v = DAT_00290e38[mode][1];
	D3DDevice_SetRenderState_Simple(0x40348, v);
	D3D__RenderState[D3DRS_DESTBLEND] = v;
}

/* 0x00085060  ClassLib (gap)
Locks the first buffer and sets the render states for drawing (texture on stage 3,
modulated by the vertex colour). */
void PointSprites_Begin(CPointSprites *self)
{
	int mode;

	self->write = PS_LockVB(self->buffers[0], 4);
	self->vertsLeft = self->vertsPerBuffer;
	self->buffer = 0;
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)2, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)3, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)4, 4);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)6, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)7, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 4);
	FUN_000416c0(0, 0);
	FUN_000416c0(1, 0);
	FUN_000416c0(2, 0);
	if (self->texture != 0)
		FUN_000416c0(3, self->texture);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(4, 0, 0);
	switch (self->blendMode) {
	case 0:
		FUN_00044b00(6, 0, 0);
		FUN_00043930((undefined4 *)&DAT_002855f0);
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
		FUN_00043930((undefined4 *)&DAT_002855f0);
		return;
	}
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(6, 1, 0);
	Blend_SetMode(mode);
	FUN_00043930((undefined4 *)&DAT_002855f0);
}

/* ======================================================================== weighted texture frames, actor animation switch
0x000a7d40-0x000a7e30 (between CRay.cpp and CParticleSystem.cpp) */

/* 0x000a7d40  ClassLib (gap)
Shows the frame of animated texture `tex` that is current at `time`, counting from frame
`first`: each frame lasts its weight (frame records of 0x18 bytes at tex + 0x24, weight at
+0x14; frame count at tex + 0x37). Negative times count back from the end. */
void Texture_SetFrameAtTime(int tex, int time, int first)
{
	int *weight;
	int *w;
	int total;
	int count;
	int n;
	int sum;

	count = *(byte *)(tex + 0x37);
	total = 0;
	if (first < count) {
		weight = (int *)(*(int *)(tex + 0x24) + 0x14 + first * 0x18);
		n = count - first;
		w = weight;
		do {
			total = total + *w;
			w = w + 6;
			n = n + -1;
		} while (n != 0);
		if (total != 0) {
			if (time < 0)
				time = total - -time % total;
			sum = 0;
			while (sum = sum + *weight, sum <= time % total) {
				first = first + 1;
				weight = weight + 6;
				if (count <= first)
					return;
			}
			FUN_00041580(tex, first);
		}
	}
}

/* 0x000a7dc0  ClassLib (gap)
An actor animation switch { on, ..., actor at [6], animation at [7] }: switching on plays
the animation (looping), switching off stops the actor's animations. */
void AnimSwitch_Set(int *self, int on)
{
	if (self != NULL) {
		if (on == 0) {
			if (*self == 1) {
				FUN_00038190(self[6], *(int *)(self[6] + 0x134), 1, 0);
				*self = 0;
			}
		} else if (*self == 0) {
			if (self[7] != 0)
				CLF_PlayAnim(self[6], *(int *)(self[6] + 0x134), self[7], 0x400, -1, 1, NULL, 0);
			*self = 1;
		}
	}
}

/* 0x000a7e30  ClassLib (gap)
Sets the word at self + 8 of an animation switch. */
void AnimSwitch_SetValue(int self, int value)
{
	if (self != 0)
		*(int *)(self + 8) = value;
}
