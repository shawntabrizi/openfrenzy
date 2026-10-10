/*
BumperCars/CBCar_Headlight.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Headlight.cpp

A headlight beam: a cone of light drawn as a triangle fan from the lamp (the car's actor
position) to a circle `range` ahead, tilted down, fading from the lamp. 0x000a6bc0 is tagged
with CBCar_Headlight.cpp; 0x000a6930-0x000a6b50 and 0x000a6d60 are gap functions of the
same class (file inferred).
*/

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"

/* a headlight (0x40 bytes) */
struct BCarHeadlight {
	int segments;              /* 0x00 of the circle */
	float range;               /* 0x04 */
	float width;               /* 0x08 the cone's angle, radians */
	float angle;               /* 0x0c the roll of the circle (follows the car) */
	float *pos;                /* 0x10 the car's lamp position */
	float *rot;                /* 0x14 the car's rotation (quaternion) */
	struct bdTexture *texture; /* 0x18 */
	void *verts;               /* 0x1c (segments + 2) vertices of 0x24 bytes */
	float tip[4];              /* 0x20 the point the beam looks at */
	float tilt[4];             /* 0x30 quaternion */
};
OF_ASSERT_SIZE(struct BCarHeadlight, 0x40);

/* a vertex of the fan (FVF 0x152: position, normal, diffuse, one texture coordinate) */
struct BCarLightVertex {
	float pos[3];
	float normal[3];
	unsigned int diffuse;
	float u;
	float v;
};

/* bdisplay's colour tables: alpha, red, green and blue in place, by 0..255 */
extern unsigned int DAT_002bc490[];
extern unsigned int DAT_002bc898[];
extern unsigned int DAT_002bce18[];
extern unsigned int DAT_002bd640[];
/* the beam's colour: red 70, green 66, blue 46 of the tables (0x002bcf30, 0x002bc598,
0x002bd6f8) */
#define BCB_BEAM_RGB (DAT_002bce18[70] | DAT_002bc490[66] | DAT_002bd640[46])

/* the tip and the roll of the beam from the car's rotation and the tilt */
static void bcb_headlight_aim(struct BCarHeadlight *self)
{
	float q[4];
	float m[16];
	float v[4];
	float angle;

	bmQuaternionMultiply(q, self->tilt, self->rot);
	bmQuaternionToMatrix(m, q);
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = -1.0f;
	v[3] = 1.0f;
	bmTransformVector4InPlace(m, v);
	self->tip[0] = v[0] + self->pos[0];
	self->tip[1] = v[1] + self->pos[1];
	self->tip[2] = v[2] + self->pos[2];
	v[0] = 0.0f;
	v[1] = 1.0f;
	v[2] = 0.0f;
	v[3] = 1.0f;
	bmTransformVector4InPlace(m, v);
	angle = atan2f(v[0] * 100.0f, v[1] * 100.0f);              /* 0x00255544 */
	if (6.2831855f < angle)                                     /* 0x00255930 */
		angle = angle - 6.2831855f;
	if (angle < 0.0f)
		angle = angle + 6.2831855f;
	self->angle = angle;
}

/* 0x000a6930  (gap; file inferred) */
void BCarHeadlight_Destruct(struct BCarHeadlight *self)
{
	if (self->verts != NULL)
	{
		BCB_DELETE(self->verts);
		self->verts = NULL;
	}
}

/* 0x000a6950  (gap; file inferred)
Draws the beam with the lamp end at `alpha` (0-1) and the far circle transparent. */
void BCarHeadlight_Draw(struct BCarHeadlight *self, float alpha)
{
	struct BCarLightVertex *verts = (struct BCarLightVertex *)self->verts;
	float m[16];
	float step;
	float theta;
	float radius;
	int i;

	verts[0].pos[0] = 0.0f;
	verts[0].pos[1] = 0.0f;
	verts[0].pos[2] = 0.0f;
	verts[0].diffuse = DAT_002bc898[-BCB_FTOL(alpha * -127.0f)] | BCB_BEAM_RGB; /* 0x002632d0 */
	verts[0].u = 0.5f;
	verts[0].v = 0.5f;
	radius = tanf(self->width * 0.5f) * self->range;
	bmMatrixLookAt(m, self->pos, self->tip, self->angle);
	bdSetWorldMatrix(m);
	bdSetTexture(0, self->texture);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(4, 1, 0);
	step = 6.2831855f / (float)self->segments;
	for (i = 1; i <= self->segments + 1; i++)
	{
		theta = (float)(i - 1) * step;
		verts[i].diffuse = BCB_BEAM_RGB | DAT_002bc898[0];
		verts[i].u = sinf(theta) * 0.5f + 0.5f;
		verts[i].v = 0.5f - cosf(theta) * 0.5f;
		verts[i].pos[0] = sinf(theta + self->angle) * radius;
		verts[i].pos[1] = cosf(theta + self->angle) * radius;
		verts[i].pos[2] = self->range;
	}
	bdDrawPrimitiveFVF(5, self->verts, self->segments + 2, 0x152);
	bdSetTexture(0, NULL);
	bdPopRenderState(4);
	bdPopRenderState(3);
	bdPopRenderState(5);
	bdPopRenderState(10);
	bdPopRenderState(6);
}

/* 0x000a6b50  (gap; file inferred)
Tilts the beam down by `degrees`. */
void BCarHeadlight_SetTilt(struct BCarHeadlight *self, float degrees)
{
	float dir[4];

	dir[0] = 0.0f;
	dir[1] = 0.0f;
	dir[2] = -1.0f;
	dir[3] = 1.0f;
	bmVectorRotateX(dir, dir, -(degrees * 0.017453292f));       /* 0x002640f8 */
	dir[3] = 1.0f;
	bmQuaternionFromDirection(self->tilt, dir, 0.0f);
}

/* 0x000a6bc0  BumperCars/CBCar_Headlight.cpp
Constructor: a beam from `pos` on a car rotated by `rot`, `range` long, `width` degrees wide,
a circle of `segments`. */
struct BCarHeadlight *BCarHeadlight_Construct(struct BCarHeadlight *self, float *pos, float *rot,
	struct bdTexture *texture, float range, float width, int segments)
{
	float dir[4];

	self->texture = texture;
	self->pos = pos;
	self->rot = rot;
	self->width = width * 0.017453292f;                         /* 0x002640f8 */
	self->range = range;
	self->segments = segments;
	dir[0] = 0.0f;
	dir[1] = 0.0f;
	dir[2] = -1.0f;
	dir[3] = 1.0f;
	bmQuaternionFromDirection(self->tilt, dir, 0.0f);
	bcb_headlight_aim(self);
	self->verts = bkHeapAlloc((uint)((self->segments + 2) * 0x24),
		"C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Headlight.cpp", 0x3a, 0x2001);
	return self;
}

/* 0x000a6d60  (gap; file inferred)
Follows the car: the tip and the roll of the beam. */
void BCarHeadlight_Update(struct BCarHeadlight *self)
{
	bcb_headlight_aim(self);
}
