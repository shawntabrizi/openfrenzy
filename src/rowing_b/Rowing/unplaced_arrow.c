/*
Rowing/unplaced_arrow.c  <-  C:\BlitzParty\Game\Src\Rowing\ (file not known)

The direction arrows of the Rowing layout 1 (one per "arrow" point of the level, 0xe0
bytes each): an arrow outline of 12 points drawn as a strip of quads that fade from grey to
transparent outwards, wobbling with three sine waves per corner.

0x001b5b10-0x001b5ea0 lie after CRowing_Tokens.cpp (gap functions; their file is not
known).
*/

#include "rowing_b.h"

/* an arrow (0xe0 bytes) */
struct RowArrow {
	struct bdVertexBuffer *vb;  /* +0x00 28 vertices */
	void *texture;              /* +0x04 */
	float pos[4];               /* +0x08 */
	float yaw;                  /* +0x18 */
	float phaseA[7];            /* +0x1c */
	float speedA[7];            /* +0x38 */
	float phaseB[7];            /* +0x54 */
	float speedB[7];            /* +0x70 */
	float phaseC[7];            /* +0x8c */
	float speedC[7];            /* +0xa8 */
	float amp[7];               /* +0xc4 */
};

/* 0x00276000, 0x002760c0  the arrow's inner and outer outline */
static const float rb_arrow_inner[12][4] = {
	{ -60.0f, 0.0f, -80.0f, 1.0f },
	{ -60.0f, 0.0f, 0.0f, 1.0f },
	{ -60.0f, 0.0f, 80.0f, 1.0f },
	{ -120.0f, 0.0f, 80.0f, 1.0f },
	{ -60.0f, 0.0f, 120.0f, 1.0f },
	{ 0.0f, 0.0f, 160.0f, 1.0f },
	{ 60.0f, 0.0f, 120.0f, 1.0f },
	{ 120.0f, 0.0f, 80.0f, 1.0f },
	{ 60.0f, 0.0f, 80.0f, 1.0f },
	{ 60.0f, 0.0f, 0.0f, 1.0f },
	{ 60.0f, 0.0f, -80.0f, 1.0f },
	{ 0.0f, 0.0f, -80.0f, 1.0f },
};
static const float rb_arrow_outer[12][4] = {
	{ -90.0f, 0.0f, -110.0f, 1.0f },
	{ -90.0f, 0.0f, -30.0f, 1.0f },
	{ -90.0f, 0.0f, 50.0f, 1.0f },
	{ -160.0f, 0.0f, 50.0f, 1.0f },
	{ -80.0f, 0.0f, 125.0f, 1.0f },
	{ 0.0f, 0.0f, 200.0f, 1.0f },
	{ 80.0f, 0.0f, 125.0f, 1.0f },
	{ 160.0f, 0.0f, 50.0f, 1.0f },
	{ 90.0f, 0.0f, 50.0f, 1.0f },
	{ 90.0f, 0.0f, -30.0f, 1.0f },
	{ 90.0f, 0.0f, -110.0f, 1.0f },
	{ 0.0f, 0.0f, -110.0f, 1.0f },
};

#define RB_ARROW_PHASE 0.000191753439f     /* 0x00263520 */

/* 0x001b5b10  Rowing (gap; file not known) */
void RowArrow_Free(struct RowArrow *self)
{
	if (self->vb != NULL) {
		bdVertexBufferDestroy(self->vb);
		self->vb = NULL;
	}
}

/* 0x001b5b30  Rowing (gap; file not known)
An arrow at pos turned by yaw, its waves at random phases. */
struct RowArrow *RowArrow_ctor(struct RowArrow *self, float *pos, float yaw)
{
	int i;

	memset(self, 0, 0x38 * 4);
	memcpy(self->pos, pos, 16);
	self->yaw = yaw;
	if (bdVertexBufferCreate(&self->vb, 0x1c, 0x152, 0, 1) == 0)
		return self;
	for (i = 0; i < 7; i++) {
		self->speedA[i] = 0.300000012f;
		self->speedB[i] = 0.300000012f;
		self->phaseA[i] = (float)rb_rand() * RB_ARROW_PHASE;
		self->speedC[i] = 0.300000012f;
		self->phaseB[i] = (float)rb_rand() * RB_ARROW_PHASE;
		self->amp[i] = 0.5f;
		self->phaseC[i] = (float)rb_rand() * RB_ARROW_PHASE;
	}
	return self;
}

/* a wave speed drifts by -0.05 to +0.05, kept in [0.0001, 0.5] */
static float rb_arrow_speed(float speed)
{
	speed = ((float)rb_rand() * 3.05185108e-06f - 0.0500000007f) + speed;  /* 0x00265744, 0x00258b54 */
	if (0.5f < speed)
		speed = 0.5f;
	if (speed < 9.99999975e-05f)                                    /* 0x00266ff4 */
		speed = 9.99999975e-05f;
	return speed;
}

/* 0x001b5c50  Rowing (gap; file not known)
Moves the waves on (their speeds drift at random) and lets the wobble size drift in
[0.3, 1]. */
void RowArrow_Update(struct RowArrow *self, float unused)
{
	float v;
	int i;

	(void)unused;
	for (i = 0; i < 7; i++) {
		self->speedA[i] = rb_arrow_speed(self->speedA[i]);
		self->phaseA[i] = self->phaseA[i] + self->speedA[i];
		if (6.28318501f < self->phaseA[i]) {
			v = self->phaseA[i];
			do
				v = v - 6.28318501f;
			while (6.28318501f < v);
			self->phaseA[i] = v;
		}
		self->speedB[i] = rb_arrow_speed(self->speedB[i]);
		self->phaseB[i] = self->speedB[i] + self->phaseB[i];
		if (6.28318501f <= self->phaseB[i]) {
			v = self->phaseB[i];
			do
				v = v - 6.28318501f;
			while (6.28318501f <= v);
			self->phaseB[i] = v;
		}
		self->speedC[i] = rb_arrow_speed(self->speedC[i]);
		self->phaseC[i] = self->speedC[i] + self->phaseC[i];
		if (6.28318501f <= self->phaseC[i]) {
			v = self->phaseC[i];
			do
				v = v - 6.28318501f;
			while (6.28318501f <= v);
			self->phaseC[i] = v;
		}
		self->amp[i] = ((float)rb_rand() * 3.05185097e-07f - 0.00499999989f) + self->amp[i];   /* 0x002757f0 */
		if (1.0f < self->amp[i])
			self->amp[i] = 1.0f;
		if (self->amp[i] < 0.300000012f)                            /* 0x00265580 */
			self->amp[i] = 0.300000012f;
	}
}

/* 0x001b5ea0  Rowing (gap; file not known)
Draws the arrow: 13 pairs of vertices (the outline closed), the inner one grey and wobbling
sideways, the outer one transparent, `t` of the way out and up the wave. */
void RowArrow_Draw(struct RowArrow *self, float t)
{
	float m[16];
	float *v;
	float lift, sx, sz;
	float x0, z0;
	const float *a;
	const float *b;
	unsigned int i;
	int count = 0;
	int k;

	v = bdVertexBufferLock(self->vb, 4);
	if (v == NULL)
		return;
	for (i = 0; i < 0xd; i++) {
		k = (int)((i % 12) % 7);
		lift = sinf(self->phaseA[k]) * self->amp[k] * 50.0f + 100.0f;   /* 0x002559b8, 0x00255544 */
		sx = sinf(self->phaseB[k]) * self->amp[k];
		sz = sinf(self->phaseC[k]) * self->amp[k];
		a = rb_arrow_inner[i == 12 ? 0 : i];
		b = rb_arrow_outer[i == 12 ? 0 : i];
		x0 = sx * 5.0f + a[0];
		z0 = a[2] + sz * 5.0f;
		v[0] = x0;
		v[1] = a[1];
		v[2] = z0;
		/* alpha, blue, green, red 127 (0x002bca94, 0x002bd83c, 0x002bc68c, 0x002bd014) */
		RB_U32(v, 0x18) = DAT_002bc898[127] | DAT_002bd640[127] | DAT_002bc490[127] | DAT_002bce18[127];
		RB_U32(v, 0x1c) = 0;
		RB_U32(v, 0x20) = 0;
		v[9] = ((sx * 8.0f + b[0]) - x0) * t + x0;                  /* 0x0025552c */
		v[10] = ((lift + b[1]) - a[1]) * t + a[1];
		v[11] = ((b[2] + sz * 8.0f) - z0) * t + z0;
		RB_U32(v, 0x3c) = DAT_002bc898[0] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
		RB_U32(v, 0x40) = 0;
		RB_U32(v, 0x44) = 0;
		count += 2;
		v += 0x12;
	}
	bdVertexBufferUnlock(self->vb);
	bmMatrixRotationY(m, -self->yaw);
	m[12] = self->pos[0];
	m[13] = self->pos[1];
	m[14] = self->pos[2];
	bdSetWorldMatrix(m);
	bdSetTexture(0, (struct bdTexture *)self->texture);
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(1);
	bdSetRenderState(5, 0, 0);
	bdDrawVertexBuffer(4, self->vb, 0, count);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
}
