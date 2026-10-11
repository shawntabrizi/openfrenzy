/*
ClassLib/CToken.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\CToken.cpp

Collectable tokens (bronze, silver, gold): an actor that springs to full size, spins, and
shrinks away when collected, with a glow pass and a shadow quad. Only 0x0010c690 is tagged
with CToken.cpp; the gap functions 0x0010c230-0x0010cd60 work on tokens (file inferred).
*/

#include "classlib_fx.h"

static const char s_CTokenFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\CToken.cpp";

/* bdisplay: bdDrawPrimitive(type, vertices, count) */
#define TOK_DrawPrimitive ((void (*)(int, void *, int))FUN_0003f2d0)
/* bkernel 0x00051540: squared distance of two points (bkernel's decl says void(void)) */
#define TOK_DistanceSq ((float (*)(float *, float *))FUN_00051540)

/* 0x0010c230  ClassLib (gap; file inferred: CToken.cpp)
Lays out the shadow quad: a square of half-size `size` at `pos`, perpendicular to `up`,
turned by `angle`. */
void CToken_ShadowQuad(CTokenVertex *v, float *pos, float *up, float size, float angle)
{
	float ux;
	float uy;
	float uz;
	float d[4];
	float a[4];
	float b[4];
	float sx;
	float sy;
	float sz;

	ux = up[0];
	uy = up[1];
	uz = up[2];
	d[0] = 1.0f;
	d[1] = 0.0f;
	d[2] = 0.0f;
	FUN_00051170(d, d, -angle);
	a[0] = d[2] * uy - d[1] * uz;
	a[1] = d[0] * uz - d[2] * ux;
	a[2] = d[1] * ux - d[0] * uy;
	b[0] = a[1] * uz - a[2] * uy;
	b[1] = a[2] * ux - a[0] * uz;
	b[2] = a[0] * uy - a[1] * ux;
	FUN_000514a0(a, a);
	FUN_000514a0(b, b);
	sx = a[0] * size + b[0] * size;
	v[0].pos[0] = pos[0] - sx;
	sy = a[1] * size + b[1] * size;
	v[0].pos[1] = pos[1] - sy;
	sz = a[2] * size + b[2] * size;
	v[0].uv[0] = 0.0f;
	v[0].uv[1] = 1.0f;
	v[0].pos[2] = pos[2] - sz;
	v[3].pos[0] = sx + pos[0];
	v[3].pos[1] = sy + pos[1];
	v[3].uv[1] = 0.0f;
	v[3].uv[0] = 1.0f;
	v[3].pos[2] = sz + pos[2];
	sx = a[0] * size - b[0] * size;
	v[2].pos[0] = sx + pos[0];
	sy = a[1] * size - b[1] * size;
	v[2].pos[1] = sy + pos[1];
	sz = a[2] * size - b[2] * size;
	v[2].uv[0] = 1.0f;
	v[2].uv[1] = 1.0f;
	v[2].pos[2] = sz + pos[2];
	v[1].pos[0] = pos[0] - sx;
	v[1].pos[1] = pos[1] - sy;
	v[1].uv[0] = 0.0f;
	v[1].uv[1] = 0.0f;
	v[1].pos[2] = pos[2] - sz;
}

/* 0x0010c420  ClassLib (gap; file inferred: CToken.cpp)
The glow pass: draws every glowing token again with an added environment texture
(stage 1, texture factor alpha). */
void CToken_DrawGlowAll(void)
{
	CToken *t;

	if (DAT_002ff9d4.count != 0) {
		D3DDevice_SetRenderState_TextureFactor(DAT_00298d28 << 0x18);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 4);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 1);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 2);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)10, 1);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
		D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
		for (t = CLF_ListNext(&DAT_002ff9d4, NULL); t != NULL; t = CLF_ListNext(&DAT_002ff9d4, t)) {
			if (t->glow != 0) {
				CLF_UpdateActor((int)t->actor, 0);
				FUN_00034d50((void *)t->actor, 7);
			}
		}
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 1);
		D3DDevice_SetTextureState_TexCoordIndex(1, 1);
		FUN_000416c0(1, 0);
	}
}

/* 0x0010c520  ClassLib (gap; file inferred: CToken.cpp)
The nearest token whose state matches `stateMask`, within `maxDist` of `pos` (in the XZ
plane when `flat`), or NULL. */
CToken *CToken_FindNearest(float *pos, float maxDist, uint stateMask, int flat)
{
	CToken *t;
	CToken *best;
	float best2;
	float d;

	best2 = maxDist * maxDist;
	best = NULL;
	for (t = CLF_ListNext(&DAT_002ff9d4, NULL); t != NULL; t = CLF_ListNext(&DAT_002ff9d4, t)) {
		if ((t->state & stateMask) != 0) {
			if (flat == 0) {
				d = TOK_DistanceSq(pos, (float *)(t->actor + 0xc0));
			} else {
				d = (pos[0] - *(float *)(t->actor + 0xc0)) * (pos[0] - *(float *)(t->actor + 0xc0)) +
				    (pos[2] - *(float *)(t->actor + 0xc8)) * (pos[2] - *(float *)(t->actor + 0xc8));
			}
			if (d < best2) {
				best2 = d;
				best = t;
			}
		}
	}
	return best;
}

/* 0x0010c5c0  ClassLib (gap; file inferred: CToken.cpp)
One frame of the spring. A value that has settled (distance and speed within 0.01) snaps
the whole spring to its target. Returns 1 when every value settled this frame. */
int CSpring_Update(CSpring *self)
{
	float d;
	int settled;
	int i;

	settled = 0;
	for (i = 0; i < self->count; i++) {
		d = self->target[i] - self->cur[i];
		self->vel[i] = d * self->stiffness + self->vel[i];
		self->vel[i] = self->vel[i] * self->damping;
		self->cur[i] = self->vel[i] * DAT_0028fda8 + self->cur[i];
		if (-0.00999999978f < d && d < 0.00999999978f && -0.00999999978f < self->vel[i] &&
		    self->vel[i] < 0.00999999978f) {
			self->cur[0] = self->target[0];
			self->cur[1] = self->target[1];
			self->cur[2] = self->target[2];
			self->cur[3] = self->target[3];
			self->vel[0] = 0.0f;
			self->vel[1] = 0.0f;
			self->vel[2] = 0.0f;
			self->vel[3] = 1.0f;
			settled = settled + 1;
		}
	}
	return settled == self->count;
}

/* 0x0010c690  ClassLib/CToken.cpp
Constructor: a token of `type` at `pos` (standing on `up`, default +Y) that springs up
from nearly nothing to `size`. */
CToken *CToken_Init(CToken *self, float *pos, int type, float size, float *up)
{
	uint colour;

	memset(&self->scale, 0, 0xf * 4);
	self->size = size;
	self->type = type;
	self->spinSpeed = 0.0599999987f;
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->pos[3] = 1.0f;
	self->glow = 1;
	if (up == NULL) {
		self->up[0] = 0.0f;
		self->up[1] = 1.0f;
		self->up[2] = 0.0f;
		self->up[3] = 1.0f;
	} else {
		self->up[0] = up[0];
		self->up[1] = up[1];
		self->up[2] = up[2];
		self->up[3] = up[3];
	}
	self->scale.cur[1] = self->size * DAT_00298d24;
	self->scale.cur[2] = self->size * 0.00999999978f;
	self->scale.cur[3] = 0.0f;
	self->scale.cur[0] = self->size * 0.00999999978f;
	self->scale.target[0] = self->size;
	self->scale.target[1] = self->size;
	self->scale.target[2] = self->size;
	self->scale.target[3] = 0.0f;
	self->scale.damping = DAT_00298d1c;
	self->scale.stiffness = DAT_00298d20;
	self->scale.count = 3;
	self->state = 1;
	self->angle = (float)CLF_Rand() * 0.000191753439f;
	CToken_ShadowQuad(self->shadow, self->pos, self->up, DAT_00298d2c * self->scale.cur[0], self->angle);
	FUN_00036b20((void *)self->actor, (void *)DAT_00298d30[self->type].actor, 0, s_CTokenFile, 0x60);
	*(float *)(self->actor + 0xc0) = self->pos[0];
	*(float *)(self->actor + 0xc4) = self->pos[1];
	*(float *)(self->actor + 0xc8) = self->pos[2];
	*(float *)(self->actor + 0xcc) = self->pos[3];
	*(float *)(self->actor + 0xc4) = DAT_00298d48 * self->size + *(float *)(self->actor + 0xc4);
	CLF_UpdateActor((int)self->actor, 1);
	CLF_SetFrame((int)self->actor, *(int *)(self->actor + 0x134), 0, 1);
	if (DAT_002ff9c8 == 1) {
		/* colour tables: red, green, blue, alpha by value */
		if (self->type == 0)
			colour = DAT_002bc898[40] | DAT_002bd640[0] | DAT_002bc490[70] | DAT_002bce18[100];
		else if (self->type == 1)
			colour = DAT_002bd640[100] | DAT_002bc490[90] | DAT_002bce18[0] | DAT_002bc898[40];
		else if (self->type == 2)
			colour = DAT_002bc490[100] | DAT_002bc898[40] | DAT_002bd640[0] | DAT_002bce18[100];
		else
			goto done;
	} else {
		colour = DAT_002bc898[48] | DAT_002bd640[64] | DAT_002bc490[64] | DAT_002bce18[64];
	}
	self->shadow[0].colour = colour;
	self->shadow[1].colour = colour;
	self->shadow[2].colour = colour;
	self->shadow[3].colour = colour;
done:
	CLF_PlaySample(1, 0x7f, 0, 0, NULL);
	CLF_ListInsert(&DAT_002ff9d4, self, 0);
	return self;
}

/* 0x0010cb40  ClassLib (gap; file inferred: CToken.cpp)
Draws the token's shadow (sized by its current scale). */
void CToken_DrawShadow(CToken *self)
{
	if (self->scale.cur[0] != 0.0f) {
		CToken_ShadowQuad(self->shadow, self->pos, self->up, DAT_00298d2c * self->scale.cur[0],
		                  self->angle);
		FUN_000416c0(0, (struct bdTexture *)DAT_002ff9d0);
		TOK_DrawPrimitive(4, self->shadow, 4);
	}
}

/* 0x0010cbb0  ClassLib (gap; file inferred: CToken.cpp)
Collects the token: it spins faster and shrinks away. */
void CToken_Collect(CToken *self)
{
	self->state = 4;
	self->scale.target[0] = 0.0f;
	self->scale.target[1] = 0.0f;
	self->scale.target[2] = 0.0f;
	self->scale.target[3] = 1.0f;
	self->spinSpeed = self->spinSpeed + self->spinSpeed;
	self->scale.stiffness = 0.100000001f;
	CLF_PlaySample(2, 0x7f, 0, 0, NULL);
}

/* 0x0010cc00  ClassLib (gap; file inferred: CToken.cpp)
Deletes a token. Returns the previous list node. */
void *CToken_Delete(CToken *self)
{
	void *prev;

	prev = CLF_ListRemove(&DAT_002ff9d4, self);
	FUN_00034930((void *)self->actor);
	if (self != NULL)
		CLF_Free(self);
	return prev;
}

/* 0x0010cc30  ClassLib (gap; file inferred: CToken.cpp)
Draws every token's shadow (when the shadow mode is set). */
void CToken_DrawShadowAll(void)
{
	CToken *t;

	if (DAT_002ff9d4.count != 0 && DAT_002ff9c8 != 0) {
		FUN_00043930((float *)DAT_002855f0);
		FUN_00044b00(3, 0, 0);
		FUN_00044b00(6, 1, 0);
		FUN_00044b00(7, 2, 0);
		FUN_00044b00(10, DAT_002ff9c8 == 1 ? 1 : 3, 0);
		FUN_00044550(-0.2f);
		for (t = CLF_ListNext(&DAT_002ff9d4, NULL); t != NULL; t = CLF_ListNext(&DAT_002ff9d4, t))
			CToken_DrawShadow(t);
		FUN_00044b00(10, 0, 0);
		FUN_00044b00(7, 3, 0);
		FUN_00044b00(5, 1, 0);
		FUN_00044550(0.0f);
	}
}

/* 0x0010cd00  ClassLib (gap; file inferred: CToken.cpp)
Draws the tokens' extra passes: glow, shadows, then demolition_d's 0x0010c080. */
void CToken_DrawAll(void)
{
	CToken_DrawGlowAll();
	CToken_DrawShadowAll();
	FUN_0010c080();
}

/* 0x0010cd10  ClassLib (gap; file inferred: CToken.cpp)
Deletes every token. */
void CToken_DeleteAll(void)
{
	CToken *t;
	void *prev;

	t = CLF_ListNext(&DAT_002ff9d4, NULL);
	while (t != NULL) {
		prev = CLF_ListRemove(&DAT_002ff9d4, t);
		FUN_00034930((void *)t->actor);
		if (t != NULL)
			CLF_Free(t);
		t = CLF_ListNext(&DAT_002ff9d4, prev);
	}
}

/* 0x0010cd60  ClassLib (gap; file inferred: CToken.cpp)
Per-frame update: the scale spring (appearing or vanishing; a vanished token is deleted)
and the spin. Returns the node to continue the list walk from. */
void *CToken_Update(CToken *self)
{
	float a;

	if (self->state == 1) {
		if (CSpring_Update(&self->scale) != 0)
			self->state = 2;
		*(float *)(self->actor + 0xd0) = self->scale.cur[0];
		*(float *)(self->actor + 0xd4) = self->scale.cur[1];
		*(float *)(self->actor + 0xd8) = self->scale.cur[2];
	} else if (self->state == 4) {
		self->spinSpeed = self->spinSpeed * 1.03999996f;
		CSpring_Update(&self->scale);
		if (*(float *)(self->actor + 0xd0) < 0.00999999978f ||
		    *(float *)(self->actor + 0xd4) < 0.00999999978f)
			return CToken_Delete(self);
		*(float *)(self->actor + 0xd0) = self->scale.cur[0];
		*(float *)(self->actor + 0xd4) = self->scale.cur[1];
		*(float *)(self->actor + 0xd8) = self->scale.cur[2];
	}
	a = DAT_0028fda8 * self->spinSpeed + self->angle;
	self->angle = a;
	a = FUN_000703a0(a);
	self->angle = a;
	FUN_00051e60((float *)(self->actor + 0xe0), a);
	return self;
}
