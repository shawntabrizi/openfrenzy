/*
Rowing/CRowing_Spark.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_Spark.cpp

The sparks of the Rowing game (vtable 0x00275f34): sparks that leave a short trail and fade
from orange to dark, drawn as beams. The Rowing copy of CarryOn's CFire_Sparks (without the
fade-in; the sparks start at the given position with a random velocity).

0x001b4c60 is tagged with CRowing_Spark.cpp; 0x001b47b0-0x001b4d40 work on the same object
(file inferred). carryon_b also calls RowSparks_Free (0x001b47c0).
*/

#include "rowing_b.h"

/* 0x00275f34  vtable of the sparks */
const void *const vtable_00275f34[1] = {
	(const void *)RowSparks_DeletingDtor,
};

/* 0x001b47b0  Rowing (gap; CRowing_Spark.cpp inferred) */
struct RowSparks *RowSparks_ctor(struct RowSparks *self)
{
	self->vtable = vtable_00275f34;
	return self;
}

/* 0x001b47c0  Rowing (gap; CRowing_Spark.cpp inferred)
Destroys the vertex buffer and frees the sparks. */
void RowSparks_Free(void *self)
{
	struct RowSparks *sparks = self;

	if (sparks->vb != NULL)
		bdVertexBufferDestroy(sparks->vb);
	if (sparks->sparks != NULL) {
		RB_DELETE(sparks->sparks);
		sparks->sparks = NULL;
	}
	sparks->count = 0;
}

/* the beam colour of a spark: colour >> 7, alpha 0x80 */
static void rb_spark_colour(int *out, const short *colour)
{
	out[0] = (int)colour[0] >> 7;
	out[1] = (int)colour[1] >> 7;
	out[2] = (int)colour[2] >> 7;
	out[3] = 0x80;
}

/* 0x001b4800  Rowing (gap; CRowing_Spark.cpp inferred)
Draws every spark as a beam from its tail to its head (additive, no z writes). */
void RowSparks_Draw(struct RowSparks *self)
{
	struct RowSpark *spark;
	unsigned int *start;
	unsigned int *out;
	int tail[4];
	int head[4];
	int i;
	int n;

	bdSetWorldMatrix((float *)DAT_002855f0);
	bdSetTexture(0, (struct bdTexture *)self->texture);
	start = bdVertexBufferLock(self->vb, 4);
	if (start == NULL)
		return;
	out = start;
	for (i = 0; i < self->count; i++) {
		spark = &self->sparks[i];
		if (spark->active == 0)
			continue;
		rb_spark_colour(tail, spark->colour);
		rb_spark_colour(head, spark->colour2);
		if (bdDrawBeam3D(spark->tail, spark->width, spark->width, tail, spark->trail[0], spark->width,
			spark->width, head, NULL, (float *)(out + 7), 0.0f) != 0) {
			/* the strip joins: the first and the last vertex doubled */
			memcpy(out, out + 7, 7 * 4);
			memcpy(out + 0x3f, out + 0x38, 7 * 4);
			out = out + 0x46;
		}
	}
	bdVertexBufferUnlock(self->vb);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(1);
	bdSetRenderState(5, 0, 0);
	n = (int)((char *)out - (char *)start) / 0x1c;
	if (n != 0)
		bdDrawVertexBuffer(4, self->vb, 0, n);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetProjectionMode(1);
}

/* 0x001b49d0  Rowing (gap; CRowing_Spark.cpp inferred) scalar deleting destructor */
struct RowSparks *RowSparks_DeletingDtor(struct RowSparks *self, unsigned int flags)
{
	self->vtable = vtable_00275f34;
	if (flags & 1)
		RB_DELETE(self);
	return self;
}

/* 0x001b49f0  Rowing (gap; CRowing_Spark.cpp inferred)
Starts a spark at pos living 2 x scale updates, orange at the tail and yellow at the head
fading to black, 0.05-0.1 x scale wide, flying at up to scale in a random direction. */
void RowSpark_Start(struct RowSpark *spark, float *pos, float scale)
{
	short life;
	float dx, dy, dz;

	spark->active = 1;
	spark->age = 0;
	life = (short)(int)(scale + scale);
	spark->life = life;
	spark->colour[0] = 0x3f80;
	spark->colour[1] = 0x1400;
	spark->colour[2] = 0x500;
	spark->colour2[0] = 0x3f80;
	spark->colour2[1] = 0x3200;
	spark->colour2[2] = 0x1400;
	spark->fade[0] = (short)(-0x3f80 / (int)life);
	spark->fade2[0] = spark->fade[0];
	spark->fade[1] = (short)(-0x1400 / (int)life);
	spark->fade2[2] = spark->fade[1];
	spark->fade[2] = (short)(-0x500 / (int)life);
	spark->fade2[1] = (short)(-0x3200 / (int)life);
	/* 0x00266454, 0x00267a84 */
	spark->width = ((float)rb_rand() * 0.000152592547f + 2.0f) * scale * 0.0250000004f;
	spark->trail[0][0] = pos[0];
	spark->trail[0][1] = pos[1];
	spark->trail[0][2] = pos[2];
	spark->trail[0][3] = 1.0f;
	spark->tail[0] = pos[0];
	spark->tail[1] = pos[1];
	spark->tail[2] = pos[2];
	spark->tail[3] = 1.0f;
	/* a direction of -0.2 to 0.2 on each axis (0x00275f38, 0x00257c7c) */
	dx = (float)rb_rand() * 1.22074043e-05f - 0.200000003f;
	dy = (float)rb_rand() * 1.22074043e-05f - 0.200000003f;
	dz = (float)rb_rand() * 1.22074043e-05f - 0.200000003f;
	spark->vel[0] = rb_randf() * scale * dx;
	spark->vel[1] = rb_randf() * scale * dy;
	spark->length = 0;
	spark->maxLength = 4;
	spark->vel[2] = rb_randf() * scale * dz;
}

/* 0x001b4c60  Rowing/CRowing_Spark.cpp
Makes `count` sparks (none active) and their vertex buffer. Returns 0 when the buffer
cannot be made. */
int RowSparks_Init(struct RowSparks *self, void *texture, int count)
{
	float origin[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	int i;

	self->count = count;
	self->sparks = NULL;
	self->gravity = -0.200000003f;
	self->spread = 45.0f;
	self->drag = 0.970000029f;
	self->sparks = RB_NEW(count * 0xe4, RB_FILE_SPARK, 0x5f, 0x2001);
	if (bdVertexBufferCreate(&self->vb, self->count * 10, 0x144, 0, 1) == 0)
		return 0;
	self->texture = texture;
	for (i = 0; i < self->count; i++) {
		RowSpark_Start(&self->sparks[i], origin, 10.0f);
		self->sparks[i].active = 0;
	}
	return 1;
}

/* 0x001b4d40  Rowing (gap; CRowing_Spark.cpp inferred)
Moves every spark (drag, gravity), lengthens its trail and fades its colours (no clamp);
it goes out when its life is over. */
void RowSparks_Update(struct RowSparks *self, float dt)
{
	struct RowSpark *spark;
	short age;
	int i;
	int k;

	for (i = 0; i < self->count; i++) {
		spark = &self->sparks[i];
		if (spark->active == 0)
			continue;
		age = spark->age;
		spark->age = (short)(age + 1);
		if (!(age < spark->life)) {
			spark->active = 0;
			continue;
		}
		if (spark->length < (int)spark->maxLength)
			spark->length++;
		for (k = 8; k > 0; k--)
			memcpy(spark->trail[k], spark->trail[k - 1], 16);
		memcpy(spark->tail, spark->trail[spark->length], 16);
		spark->trail[0][0] = dt * spark->vel[0] + spark->trail[0][0];
		spark->trail[0][1] = dt * spark->vel[1] + spark->trail[0][1];
		spark->trail[0][2] = dt * spark->vel[2] + spark->trail[0][2];
		spark->vel[0] = self->drag * spark->vel[0];
		spark->vel[1] = self->drag * spark->vel[1];
		spark->vel[2] = self->drag * spark->vel[2];
		spark->colour[0] = (short)(spark->colour[0] + spark->fade[0]);
		spark->colour[1] = (short)(spark->colour[1] + spark->fade[1]);
		spark->colour[2] = (short)(spark->colour[2] + spark->fade[2]);
		spark->colour2[0] = (short)(spark->colour2[0] + spark->fade2[0]);
		spark->vel[1] = dt * self->gravity + spark->vel[1];
		spark->colour2[1] = (short)(spark->colour2[1] + spark->fade2[1]);
		spark->colour2[2] = (short)(spark->colour2[2] + spark->fade2[2]);
	}
}
