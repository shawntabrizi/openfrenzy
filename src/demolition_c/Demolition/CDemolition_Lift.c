/*
Demolition/CDemolition_Lift.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Lift.cpp

Variation 1 of Demolition: the arena is a lift going down a shaft. CDemolition_Lift (vtable
0x0026b66c, 0x2330 bytes, at +0x10 of the variation object) owns:
  - 6 cogs and 4 worm shafts (bactor instances) that spin with the lift speed,
  - the shaft: a ring of 3 sections (floor, then girders / girders extra) scrolling past,
  - 5 pairs of CDemolition_Wall girder walls (demolition_d) scrolling past, re-textured at
    random as they wrap,
  - the "control box01" object of the world, which swings and throws sparks.

0x000fda10, 0x000fdbf0 and 0x000fdd50 are tagged with CDemolition_Lift.cpp. The gap functions
0x000fcb70-0x000fd710 and 0x000fde10-0x000fe180 use the same object (file inferred).
*/

#include "demolition_c_internal.h"

#define DC_WALL_SIZE 0x1c4

/* 0x00297520  demolition_a's per variation settings (struct DA_VariationSettings, 0x3c bytes
   each; demolition_a.h cannot be included here, its DAT_002ff85c type differs). Only the
   "light" int at +0x00 is read. */
extern char DAT_00297520[];
#define DC_VARIATION_LIGHT(v) (*(int *)(DAT_00297520 + (v) * 0x3c))

/* ---------- data (.data, owned here) */

/* 0x002988d8  the part models */
static const char *const DAT_002988d8[2] = {
	"remote\\objects\\cog.obe",
	"remote\\objects\\worm shaft.obe",
};
/* 0x002988e0  cog positions */
static const float DAT_002988e0[6][4] = {
	{ -2139.37012f, -585.130005f, -1415.32996f, 0.0f },
	{ -1838.63f, -541.289978f, 1839.67004f, 0.0f },
	{ 1411.03003f, -585.130005f, 2097.34009f, 0.0f },
	{ -1811.81995f, 442.570007f, -1003.84003f, 0.0f },
	{ -1815.68005f, 443.279999f, 1838.20996f, 0.0f },
	{ 1004.34998f, 442.570007f, 1824.79004f, 0.0f },
};
/* 0x00298940  cog start angles */
static const float DAT_00298940[6] = { 0.0f, 0.785398185f, -4.71238899f, 1.57079637f,
	0.785398185f, 0.0f };
/* 0x00298958  worm shaft positions */
static const float DAT_00298958[4][4] = {
	{ -1884.54004f, -606.070007f, -885.580017f, 0.0f },
	{ 845.780029f, -606.070007f, 1878.01001f, 0.0f },
	{ -1832.76001f, -87.1699982f, -749.01001f, 0.0f },
	{ 749.52002f, -87.1699982f, 1845.72998f, 0.0f },
};
/* 0x00298998  worm shaft start angles */
static const float DAT_00298998[4] = { 0.0f, -4.71238899f, 4.71238899f, 4.71238899f };
/* 0x002989a8  the shaft section models */
static const char *const DAT_002989a8[3] = {
	"remote\\lift\\floor.obe",
	"remote\\lift\\girders.obe",
	"remote\\lift\\girders extra.obe",
};
/* 0x002989b4  heights of the shaft sections */
static const float DAT_002989b4[3] = { 3150.0f, 3200.0f, 3200.0f };

/* CDemolition_Lift's vtable 0x0026b66c: the deleting destructor only */
const void *const vtable_0026b66c[1] = { (const void *)FUN_000fd200 };

/* the walls of girder pair i, wall j */
static void *dc_wall(struct CDemolition_Lift *self, int i, int j)
{
	return self->girders[i] + j * DC_WALL_SIZE;
}

/* adds a section on top of the shaft ring (inlined in the original) */
static void dc_shaft_push(struct CDemolition_Lift *self, struct DcLiftSegment *seg)
{
	self->ring[(self->ringHead + self->ringCount) % 3] = seg;
	if (self->ringCount < 3)
		self->ringCount = self->ringCount + 1;
	self->segCounter = self->segCounter + 1;
}

/* girders, then girders extra, alternately */
static struct DcLiftSegment *dc_shaft_next(struct CDemolition_Lift *self)
{
	return &self->segs[(self->segCounter % 2 == 0) ? 1 : 2];
}

/* adds a pair to the girder ring (inlined in the original) */
static void dc_girder_push(struct CDemolition_Lift *self, int pair)
{
	self->girderRing[(self->girderHead + self->girderCount) % 5].strips = self->girders[pair];
	if (self->girderCount < 5)
		self->girderCount = self->girderCount + 1;
}

/* resets one wall of the ring head and re-textures it, a random texture every 2-3 walls
   (inlined twice in 0x000fd710) */
static void dc_girder_recycle(struct CDemolition_Lift *self, int offset)
{
	void *wall = self->girderRing[self->girderHead].strips + offset;

	FUN_0010f5b0((int)wall);
	FUN_0010dbe0((int)wall, DC_I32(DC_GAME, 0x6c84));
	self->texCounter = self->texCounter + 1;
	if (self->texEvery <= self->texCounter)
	{
		int r = dc_rand() % 6;

		FUN_0010dbe0((int)(self->girderRing[self->girderHead].strips + offset),
			DC_I32(DC_GAME, 0x6c88 + r * 4));
		self->texEvery = dc_rand() % 2 + 2;
		self->texCounter = 0;
	}
}

/* 0x000fcb30  Demolition (file inferred: CDemolition_Lift.cpp; missing from the table)
Constructs a girder pair: 2 CDemolition_Wall. */
void *FUN_000fcb30(void *self)
{
	int i;

	for (i = 0; i < 2; i++)
		FUN_0010db80((char *)self + i * DC_WALL_SIZE);
	return self;
}

/* 0x000fcb50  Demolition (file inferred: CDemolition_Lift.cpp; missing from the table)
Destroys a girder pair, last wall first. */
void FUN_000fcb50(void *self)
{
	int i;

	for (i = 1; i >= 0; i--)
		FUN_0010db90((char *)self + i * DC_WALL_SIZE);
}

/* 0x000fcb70  Demolition (file inferred: CDemolition_Lift.cpp)
Destructor: destroys the 5 girder pairs, last first. */
void CDemolition_Lift_Destruct(struct CDemolition_Lift *self)
{
	int i;

	self->vtable = vtable_0026b66c;
	for (i = 4; i >= 0; i--)
		FUN_000fcb50(self->girders[i]);
}

/* 0x000fcb90  Demolition (file inferred: CDemolition_Lift.cpp; added to the table in
integration round 2)
Starts the lift (state 1, speeding up) with sound 0xf at full volume. Variation 1 slot 3
(Start) calls it. */
void CDemolition_Lift_Start(struct CDemolition_Lift *self)
{
	self->state = 1;
	FUN_0018e450((void *)(DC_GAME + 0x79d0), 0xf, 0xff, 0, 0, 0);
}

/* 0x000fcbc0  Demolition (file inferred: CDemolition_Lift.cpp)
Starts stopping the lift. */
void CDemolition_Lift_Stop(struct CDemolition_Lift *self)
{
	self->state = 2;
}

/* 0x000fcbd0  Demolition (file inferred: CDemolition_Lift.cpp)
Deletes the actor instances of the cogs and worm shafts. */
void CDemolition_Lift_ReleaseParts(struct CDemolition_Lift *self)
{
	int i;

	for (i = 0; i < 6; i++)
		baDeleteActorInstance((struct BActorInstance *)self->cogs[i].actor);
	for (i = 0; i < 4; i++)
		baDeleteActorInstance((struct BActorInstance *)self->shafts[i].actor);
}

/* 0x000fcc20  Demolition (file inferred: CDemolition_Lift.cpp)
State 1: speeds the lift up (down to -4) and starts its sound once. dt is not used. */
void CDemolition_Lift_SpeedUp(struct CDemolition_Lift *self, float dt)
{
	float v;

	(void)dt;
	v = self->speed - 0.00999999978f; /* 0x0025372c */
	self->speed = v;
	if (v < -4.0f) /* 0x0026b670 */
	{
		self->speed = -4.0f;
		if (self->soundOn == 0)
		{
			self->sound = FUN_0018e450((void *)(DC_GAME + 0x79d0), 0x10, 0xc0, 0, 0, 0);
			self->soundOn = 1;
		}
	}
}

/* 0x000fcc80  Demolition (file inferred: CDemolition_Lift.cpp)
State 2: slows the lift down to a stop, then stops its sound. dt is not used. */
void CDemolition_Lift_SlowDown(struct CDemolition_Lift *self, float dt)
{
	float v;

	(void)dt;
	v = self->speed + 0.0199999996f; /* 0x0025588c */
	self->speed = v;
	if (0.0f < v)
	{
		self->speed = 0.0f;
		self->state = 0;
	}
	if (self->soundOn != 0)
	{
		FUN_000ebc20(self->sound);
		self->soundOn = 0;
	}
}

/* 0x000fcce0  Demolition (file inferred: CDemolition_Lift.cpp; added to the table in
integration round 2)
Draws the actor instances of the 6 cogs and the 4 worm shafts (7 = every pass). Only
0x000fd540 reaches it (a tail jump). */
void CDemolition_Lift_DrawParts(struct CDemolition_Lift *self)
{
	int i;

	for (i = 0; i < 6; i++)
		baDrawActorInstance((struct BActorInstance *)self->cogs[i].actor, 7);
	for (i = 0; i < 4; i++)
		baDrawActorInstance((struct BActorInstance *)self->shafts[i].actor, 7);
}

/* 0x000fcd30  Demolition (file inferred: CDemolition_Lift.cpp)
Deletes the shaft section models. */
void CDemolition_Lift_ReleaseShaft(struct CDemolition_Lift *self)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (self->segModels[i] != NULL)
		{
			bkDeleteResource((struct bkResource *)self->segModels[i]);
			self->segModels[i] = NULL;
		}
	}
}

/* 0x000fcd60  Demolition (file inferred: CDemolition_Lift.cpp)
Scrolls the shaft with the lift: first the drop down to -2500, then the scroll. When the
bottom section has passed, it leaves the ring and a new one is added on top. */
void CDemolition_Lift_UpdateShaft(struct CDemolition_Lift *self, float dt)
{
	float d;
	float height;

	d = dt * self->speed;
	if (self->drop <= -2500.0f) /* 0x0026b674 */
	{
		self->scroll = d + self->scroll;
	}
	else
	{
		d = d + self->drop;
		self->drop = d;
		if (d < -2500.0f)
		{
			self->drop = -2500.0f;
			self->scroll = -(-2500.0f - d);
		}
	}
	height = self->ring[self->ringHead]->height;
	if (-height <= self->scroll)
		return;
	self->scroll = height + self->scroll;
	self->ringHead = (self->ringHead + 1) % 3;
	self->ringCount = self->ringCount - 1;
	dc_shaft_push(self, dc_shaft_next(self));
}

/* 0x000fce70  Demolition (file inferred: CDemolition_Lift.cpp)
Draws the shaft sections from the bottom up (no caller in the code: called through the
variation). */
void CDemolition_Lift_DrawShaft(struct CDemolition_Lift *self)
{
	float m[16];
	float t[16];
	struct DcLiftSegment *seg;
	int idx;
	int i;

	bdPushRenderState(3, 1, 0);
	bmMatrixTranslation(m, 0.0f, self->scroll + self->drop, 0.0f);
	idx = self->ringHead;
	for (i = 0; i < self->ringCount; i++)
	{
		seg = self->ring[idx];
		bdSetWorldMatrix(m);
		baDrawSet((struct BSet *)seg->model, 1);
		baDrawSet((struct BSet *)seg->model, 2);
		bmMatrixTranslation(t, 0.0f, seg->height, 0.0f);
		bmMatrixMultiplyCopy(m, m, t);
		idx = (idx + 1) % 3;
	}
	bdPopRenderState(3);
}

/* 0x000fcf70  Demolition (file inferred: CDemolition_Lift.cpp)
Releases the 10 girder walls. */
void CDemolition_Lift_ReleaseGirders(struct CDemolition_Lift *self)
{
	int i;
	int j;

	for (i = 0; i < 5; i++)
		for (j = 0; j < 2; j++)
			FUN_0010e010((int)dc_wall(self, i, j));
}

/* 0x000fcfa0  Demolition (file inferred: CDemolition_Lift.cpp)
Places the girder walls of the ring, 1600 apart from the scroll position + 1500 down, and
updates them. */
void CDemolition_Lift_PlaceGirders(struct CDemolition_Lift *self, float dt)
{
	float y;
	int idx;
	int i;
	int j;

	y = self->girderScroll + 1500.0f; /* 0x0026562c */
	idx = self->girderHead;
	for (i = 0; i < self->girderCount; i++)
	{
		for (j = 0; j < 2; j++)
		{
			void *wall = self->girderRing[idx].strips + j * DC_WALL_SIZE;

			dc_wall_set_height(wall, y);
			dc_wall_update(wall, dt);
		}
		y = y - 1600.0f; /* 0x00256678 */
		idx = (idx + 1) % 5;
	}
}

/* 0x000fd040  Demolition (file inferred: CDemolition_Lift.cpp)
Draws the girder walls of the ring. */
void CDemolition_Lift_DrawGirders(struct CDemolition_Lift *self)
{
	int idx;
	int i;
	int j;

	bdPushRenderState(3, 1, 0);
	FUN_000ed970((int)(DC_GAME + 0x6ca4), 0, 4);
	idx = self->girderHead;
	for (i = 0; i < self->girderCount; i++)
	{
		for (j = 0; j < 2; j++)
			FUN_0010e080((int)(self->girderRing[idx].strips + j * DC_WALL_SIZE));
		idx = (idx + 1) % 5;
	}
	bdPopRenderState(3);
	FUN_000ed970((int)(DC_GAME + 0x6ca4), 1, 4);
}

/* 0x000fd100  Demolition (file inferred: CDemolition_Lift.cpp)
Moves the world's objects (their matrices at game + 0x9f8, 0x74 apart) with the lift. */
void CDemolition_Lift_MoveWorld(struct CDemolition_Lift *self, float dt)
{
	float m[16];
	float v;
	int i;

	v = dt * self->speed + self->shaftScroll;
	self->shaftScroll = v;
	bmMatrixTranslation(m, 0.0f, v, 0.0f);
	for (i = 0; i < DC_I32(DC_GAME, 0xa3c); i++)
		memcpy(DC_ADDR(DC_GAME, 0x9f8 + i * 0x74), m, sizeof m);
}

/* 0x000fd180  Demolition (file inferred: CDemolition_Lift.cpp; added to the table in
integration round 2)
An explosion at POS with RADIUS damages each of the 10 girder walls (DemoWall_Blast,
demolition_d). Variation 1 slot 11 (Damage) calls it. */
void CDemolition_Lift_Blast(struct CDemolition_Lift *self, float *pos, float radius)
{
	int i;
	int j;

	for (i = 0; i < 5; i++)
		for (j = 0; j < 2; j++)
			DemoWall_Blast((int)dc_wall(self, i, j), pos, radius);
}

/* 0x000fd1d0  Demolition (file inferred: CDemolition_Lift.cpp)
Constructor: the 5 girder pairs. */
struct CDemolition_Lift *CDemolition_Lift_Construct(struct CDemolition_Lift *self)
{
	int i;

	self->vtable = vtable_0026b66c;
	for (i = 0; i < 5; i++)
		FUN_000fcb30(self->girders[i]);
	return self;
}

/* 0x000fd200  Demolition (file inferred: CDemolition_Lift.cpp)
Deleting destructor (vtable slot 0). */
struct CDemolition_Lift *CDemolition_Lift_DeletingDestructor(struct CDemolition_Lift *self,
                                                             unsigned char flags)
{
	CDemolition_Lift_Destruct(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* the reset of the cogs or worm shafts (one loop of 0x000fd220) */
static void dc_reset_parts(struct DcLiftPart *part, const float *angles, int n)
{
	int i;

	for (i = 0; i < n; i++)
	{
		part[i].quat[0] = 0.0f;
		part[i].quat[1] = 0.0f;
		part[i].quat[2] = 0.0f;
		part[i].quat[3] = 1.0f;
		DC_F32(part[i].actor, 0xc0) = part[i].pos[0];
		DC_F32(part[i].actor, 0xc4) = part[i].pos[1];
		DC_F32(part[i].actor, 0xc8) = part[i].pos[2];
		DC_F32(part[i].actor, 0xcc) = part[i].pos[3];
		part[i].angle = angles[i];
	}
}

/* 0x000fd220  Demolition (file inferred: CDemolition_Lift.cpp)
Puts the cogs and worm shafts back at their places, unturned, with their start angles. */
void CDemolition_Lift_ResetParts(struct CDemolition_Lift *self)
{
	dc_reset_parts(self->cogs, DAT_00298940, 6);
	dc_reset_parts(self->shafts, DAT_00298998, 4);
}

/* 0x000fd300  Demolition (file inferred: CDemolition_Lift.cpp)
Shutdown: parts, shaft models and girder walls. */
void CDemolition_Lift_Shutdown(struct CDemolition_Lift *self)
{
	CDemolition_Lift_ReleaseParts(self);
	CDemolition_Lift_ReleaseShaft(self);
	CDemolition_Lift_ReleaseGirders(self);
}

/* turns one part by its spin and sets its actor's rotation (inlined in 0x000fd320) */
static void dc_spin_part(struct DcLiftPart *part, float v, float rate, float ax, float ay,
                         float az, const float *axis)
{
	float aa[4];
	float q[4];
	float r[4];

	if (v != 0.0f)
	{
		aa[0] = ax;
		aa[1] = ay;
		aa[2] = az;
		aa[3] = v * 1.57079625f * rate; /* 0x00253898 */
		bmAxisAngleToQuaternion(q, aa);
		bmQuaternionMultiply(part->quat, part->quat, q);
		FUN_00198590(part->quat);
	}
	aa[0] = axis[0];
	aa[1] = axis[1];
	aa[2] = axis[2];
	aa[3] = part->angle;
	bmAxisAngleToQuaternion(q, aa);
	bmQuaternionMultiply(r, part->quat, q);
	DC_F32(part->actor, 0xe4) = r[1];
	DC_F32(part->actor, 0xe0) = r[0];
	DC_F32(part->actor, 0xe8) = r[2];
	DC_F32(part->actor, 0xec) = r[3];
	baPrepActorInstance((struct BActorInstance *)part->actor, 1);
}

/* 0x000fd320  Demolition (file inferred: CDemolition_Lift.cpp)
Spins the cogs (about x) and worm shafts (about z) with the lift speed, then turns each by its
start angle (cogs and the first two shafts about y, the other shafts about x). */
void CDemolition_Lift_UpdateParts(struct CDemolition_Lift *self, float dt)
{
	static const float yAxis[3] = { 0.0f, 1.0f, 0.0f };
	static const float xAxis[3] = { 1.0f, 0.0f, 0.0f };
	int i;

	for (i = 0; i < 6; i++)
		dc_spin_part(&self->cogs[i], dt * self->speed, 0.00291545200f /* 0x0026b678 */,
			1.0f, 0.0f, 0.0f, yAxis);
	for (i = 0; i < 4; i++)
		dc_spin_part(&self->shafts[i], dt * self->speed, 0.00999999978f /* 0x0025372c */,
			0.0f, 0.0f, 1.0f, (i < 2) ? yAxis : xAxis);
}

/* 0x000fd540  Demolition (file inferred: CDemolition_Lift.cpp; added to the table in
integration round 2)
Draws the lift world: the shaft, the girder walls and the cogs and worm shafts. Nothing is
drawn when the variation's "light" setting (demolition_a table 0x00297520, +0x00) is 0.
Variation 1 slot 7 (Draw) calls it. */
void CDemolition_Lift_Draw(struct CDemolition_Lift *self)
{
	if (DC_VARIATION_LIGHT(DC_I32(DC_GAME, 0x28)) == 0)
		return;
	CDemolition_Lift_DrawShaft(self);
	CDemolition_Lift_DrawGirders(self);
	CDemolition_Lift_DrawParts(self);
}

/* 0x000fd570  Demolition (file inferred: CDemolition_Lift.cpp)
Restarts the shaft ring: the floor, then two girder sections. */
void CDemolition_Lift_ResetShaft(struct CDemolition_Lift *self)
{
	self->ringCount = 0;
	self->ringHead = 0;
	dc_shaft_push(self, &self->segs[0]);
	dc_shaft_push(self, dc_shaft_next(self));
	dc_shaft_push(self, dc_shaft_next(self));
}

/* 0x000fd660  Demolition (file inferred: CDemolition_Lift.cpp)
Restarts the girder ring with the 5 pairs in order. */
void CDemolition_Lift_ResetGirderRing(struct CDemolition_Lift *self)
{
	int i;

	self->girderCount = 0;
	self->girderHead = 0;
	for (i = 0; i < 5; i++)
		dc_girder_push(self, i);
}

/* 0x000fd710  Demolition (file inferred: CDemolition_Lift.cpp)
Scrolls the girders with the lift. When the scroll passes 1600 (either way) it wraps, the
ring turns back by one (the head moves back, the count stays) and the new head pair is reset
and re-textured. */
void CDemolition_Lift_ScrollGirders(struct CDemolition_Lift *self, float dt)
{
	float v;
	int offset;

	v = dt * self->speed + self->girderScroll;
	self->girderScroll = v;
	if (0.0f < v)
	{
		if (v <= 1600.0f) /* 0x00256678 */
			return;
		v = self->girderScroll;
		do
			v = v - 1600.0f;
		while (1600.0f < v);
		self->girderScroll = v;
	}
	else
	{
		if (!(v < -1600.0f)) /* 0x0026b67c */
			return;
		v = self->girderScroll;
		do
			v = v + 1600.0f;
		while (v < -1600.0f);
		self->girderScroll = v;
	}
	self->girderHead = self->girderHead - 1;
	if (self->girderHead < 0)
		self->girderHead = 4;
	self->girderCount = self->girderCount - 1;
	if (self->girderCount < 5)
		self->girderCount = self->girderCount + 1;
	for (offset = 0; offset < 2 * DC_WALL_SIZE; offset += DC_WALL_SIZE)
		dc_girder_recycle(self, offset);
}

/* 0x000fda10  Demolition/CDemolition_Lift.cpp
Finds the control box in the world (by the CRC of "control box01"), starts its swing
animation, and creates its sparks (a particle system of 200, at most, in the game's list).
The spark timer starts at a random 0-59 frames. Returns 1. */
int CDemolition_Lift_InitControlBox(struct CDemolition_Lift *self)
{
	char *actor;
	void *sparks;
	uint crc;
	int i;

	crc = bkCRCString("control box01");
	self->controlBox = NULL;
	for (i = 0; i < DC_I32(DC_GAME, 0xe44); i++)
	{
		if (crc == DC_U32(DC_GAME, 0xabc + i * 0x40))
		{
			self->controlBox = DC_GAME + 0xa84 + i * 0x40;
			break;
		}
	}
	if (self->controlBox != NULL)
		self->swingAnim = baFindSubObject((struct BActorInstance *)DC_PTR(self->controlBox, 0x30),
			"swing");
	/* the original reads the box without checking it */
	actor = DC_PTR(self->controlBox, 0x30);
	self->sparksNode = baFindNode((struct BActorNode *)DC_PTR(actor, 0x134), "sparks");
	actor = DC_PTR(self->controlBox, 0x30);
	baFlushAnimations(actor, (struct BActorNode *)DC_PTR(actor, 0x134), 1, 0);
	actor = DC_PTR(self->controlBox, 0x30);
	baQueueAnimation((struct BActorInstance *)actor, (struct BActorNode *)DC_PTR(actor, 0x134),
		(struct BAnim *)self->swingAnim, 0x400, -1, 1, 0, 0);
	sparks = bkHeapAlloc(0x6c, s_DcLiftFile, 0x50d, 0x1001);
	if (sparks != NULL)
		sparks = FUN_00102b00(sparks);
	self->sparks = sparks;
	FUN_00104770(self->sparks, NULL, 200);
	CParticleSystem_SetTexture((struct CPSHeader *)self->sparks, DC_I32(DC_GAME, 0x6c54));
	FUN_0019d110((int)self->sparks, DC_I32(DC_GAME, 0x6c78));
	FUN_0018e010((void *)(DC_GAME + 0x7bd0), (void *)self->sparks);
	self->sparking = 0;
	self->sparkTimer = dc_rand() % 0x3c;
	return 1;
}

/* 0x000fdbf0  Demolition/CDemolition_Lift.cpp
Animates the control box and moves the sparks to its "sparks" node. The box sparks on and off
at random (off for up to 59 frames, on for up to 9); while on, it emits 0-9 sparks a frame. */
void CDemolition_Lift_UpdateControlBox(struct CDemolition_Lift *self, float dt)
{
	char *actor;
	char *sparks;
	int t;

	actor = DC_PTR(self->controlBox, 0x30);
	baUpdateAnimations((struct BActorInstance *)actor, (struct BActorNode *)DC_PTR(actor, 0x134),
		dt, NULL);
	baPrepActorInstance((struct BActorInstance *)DC_PTR(self->controlBox, 0x30), 1);
	baGetNodePosition((struct BActorInstance *)DC_PTR(self->controlBox, 0x30),
		(struct BActorNode *)self->sparksNode, self->sparkPos);
	sparks = self->sparks;
	self->sparkPos[3] = 1.0f;
	DC_F32(sparks, 0x1c) = self->sparkPos[0];
	DC_F32(sparks, 0x20) = self->sparkPos[1];
	DC_F32(sparks, 0x24) = self->sparkPos[2];
	DC_F32(sparks, 0x28) = self->sparkPos[3];
	t = self->sparkTimer;
	self->sparkTimer = t - 1;
	if (t < 1)
	{
		self->sparking = self->sparking == 0;
		self->sparkTimer = dc_rand() % (self->sparking ? 10 : 0x3c);
	}
	if (self->sparking != 0)
	{
		int n = (int)((float)dc_rand() * 3.05185094e-05f /* 0x00253740 */ * 10.0f /* 0x002536d8 */);

		FUN_0019d0c0((int *)self->sparks, n);
		DC_U8(self->sparks, 0x50) = 1;
	}
}

/* the set-up of the cogs or worm shafts (one loop of 0x000fdd50) */
static void dc_init_parts(struct DcLiftPart *part, void *model, const float (*pos)[4], int n,
                          int line)
{
	int i;

	for (i = 0; i < n; i++)
	{
		baInitActorInstance((struct BActorInstance *)part[i].actor, (struct BActor *)model, 1,
			s_DcLiftFile, line);
		part[i].pos[0] = pos[i][0];
		part[i].pos[1] = pos[i][1];
		part[i].pos[2] = pos[i][2];
		part[i].pos[3] = pos[i][3];
	}
}

/* 0x000fdd50  Demolition/CDemolition_Lift.cpp
Creates the actor instances of the cogs and worm shafts at their places. `package` is not
used. Returns 1. */
int CDemolition_Lift_InitParts(struct CDemolition_Lift *self, void *package)
{
	(void)package;
	dc_init_parts(self->cogs, self->partModels[0], DAT_002988e0, 6, 0xbf);
	dc_init_parts(self->shafts, self->partModels[1], DAT_00298958, 4, 200);
	CDemolition_Lift_ResetParts(self);
	return 1;
}

/* 0x000fde10  Demolition (file inferred: CDemolition_Lift.cpp)
Per frame (called through the variation). */
void CDemolition_Lift_Update(struct CDemolition_Lift *self, float dt)
{
	if (self->state != 0)
	{
		if (self->state == 1)
			CDemolition_Lift_SpeedUp(self, dt);
		else if (self->state == 2)
			CDemolition_Lift_SlowDown(self, dt);
	}
	CDemolition_Lift_UpdateParts(self, dt);
	CDemolition_Lift_UpdateShaft(self, dt);
	CDemolition_Lift_ScrollGirders(self, dt);
	CDemolition_Lift_PlaceGirders(self, dt);
	CDemolition_Lift_UpdateControlBox(self, dt);
	CDemolition_Lift_MoveWorld(self, dt);
}

/* 0x000fde70  Demolition (file inferred: CDemolition_Lift.cpp)
Sets the 10 girder walls up: in each pair one wall at z 2500 turned by pi, one at x -2500
turned by pi/2, all reset with the default texture and a random one every 2-3 walls. Then
restarts the ring. */
void CDemolition_Lift_InitGirders(struct CDemolition_Lift *self)
{
	float pos[4];
	float angle;
	int i;
	int j;
	char *wall;

	self->texCounter = 0;
	self->texEvery = dc_rand() % 2 + 2;
	self->girderScroll = 0.0f;
	pos[1] = 0.0f;
	pos[3] = 0.0f; /* the original passes an unset stack word here */
	wall = self->girders[0];
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 2; j++)
		{
			if ((j & 1) == 0)
			{
				pos[0] = 0.0f;
				pos[2] = 2500.0f;
				angle = 3.14159274f;
			}
			else
			{
				pos[2] = 0.0f;
				pos[0] = -2500.0f;
				angle = 1.57079637f;
			}
			FUN_0010e050((int)wall, (undefined4 *)pos);
			dc_wall_set_angle(wall, angle);
			FUN_0010f5b0((int)wall);
			FUN_0010dbe0((int)wall, DC_I32(DC_GAME, 0x6c84));
			self->texCounter = self->texCounter + 1;
			if (self->texEvery <= self->texCounter)
			{
				FUN_0010dbe0((int)wall, DC_I32(DC_GAME, 0x6c88 + (dc_rand() % 6) * 4));
				self->texEvery = dc_rand() % 2 + 2;
				self->texCounter = 0;
			}
			wall += DC_WALL_SIZE;
		}
	}
	CDemolition_Lift_ResetGirderRing(self);
}

/* 0x000fe010  Demolition (file inferred: CDemolition_Lift.cpp)
Reset for a new round (called through the variation). */
void CDemolition_Lift_Reset(struct CDemolition_Lift *self)
{
	self->shaftScroll = 0.0f;
	self->speed = 0.0f;
	self->state = 0;
	self->soundOn = 0;
	CDemolition_Lift_ResetParts(self);
	self->segCounter = 0;
	self->drop = 0.0f;
	self->scroll = 0.0f;
	CDemolition_Lift_ResetShaft(self);
	CDemolition_Lift_InitGirders(self);
}

/* 0x000fe050  Demolition (file inferred: CDemolition_Lift.cpp)
Loads the 3 shaft section models (heights 3150, 3200, 3200) and restarts the shaft.
Returns 0 if a model is missing. */
int CDemolition_Lift_InitShaft(struct CDemolition_Lift *self, void *package)
{
	void *res;
	int i;

	for (i = 0; i < 3; i++)
	{
		res = bLoadResource((struct bkPackage *)package, DAT_002989a8[i], 6, 0xdefa);
		self->segModels[i] = res;
		if (res == NULL)
		{
			bkPrintf("*** ERROR *** Could not find lift shaft object [%s]\n", DAT_002989a8[i]);
			return 0;
		}
	}
	self->segs[0].model = self->segModels[0];
	self->segs[0].height = DAT_002989b4[0];
	self->segs[1].model = self->segModels[1];
	self->segs[2].model = self->segModels[2];
	self->segs[1].height = DAT_002989b4[1];
	self->segs[2].height = DAT_002989b4[2];
	self->segCounter = 0;
	self->drop = 0.0f;
	self->scroll = 0.0f;
	CDemolition_Lift_ResetShaft(self);
	return 1;
}

/* 0x000fe120  Demolition (file inferred: CDemolition_Lift.cpp)
Creates the 10 girder walls (4500 x 1600 x 200, 14 x 12 quads) and sets them up. `package` is
not used. Returns 1. */
int CDemolition_Lift_CreateGirders(struct CDemolition_Lift *self, void *package)
{
	int i;
	int j;

	(void)package;
	self->girderScroll = 0.0f;
	for (i = 0; i < 5; i++)
		for (j = 0; j < 2; j++)
			FUN_0010f5e0((int)dc_wall(self, i, j), 4500.0f, 1600.0f, 200.0f, 0xe, 0xc);
	CDemolition_Lift_InitGirders(self);
	return 1;
}

/* 0x000fe180  Demolition (file inferred: CDemolition_Lift.cpp)
Initialise (called by the variation's Initialise 0x000e9f70): loads the cog and worm shaft
models, then sets everything up. Returns 0 if a model is missing. */
int CDemolition_Lift_Initialise(struct CDemolition_Lift *self, void *package)
{
	void *res;
	int i;

	for (i = 0; i < 2; i++)
	{
		res = bLoadResource((struct bkPackage *)package, DAT_002988d8[i], 1, 0xdefa);
		self->partModels[i] = res;
		if (res == NULL)
		{
			bkPrintf("ERROR: Failed to load lift model %s\n", DAT_002988d8[i]);
			return 0;
		}
	}
	CDemolition_Lift_InitParts(self, package);
	CDemolition_Lift_InitShaft(self, package);
	CDemolition_Lift_CreateGirders(self, package);
	CDemolition_Lift_InitControlBox(self);
	self->shaftScroll = 0.0f;
	self->speed = 0.0f;
	self->state = 0;
	self->soundOn = 0;
	CDemolition_Lift_ResetParts(self);
	self->segCounter = 0;
	self->drop = 0.0f;
	self->scroll = 0.0f;
	CDemolition_Lift_ResetShaft(self);
	CDemolition_Lift_InitGirders(self);
	return 1;
}
