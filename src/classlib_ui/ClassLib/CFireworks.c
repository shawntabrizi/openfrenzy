/*
ClassLib/CFireworks.c  (classlib_ui)
Original: C:\BlitzParty\Game\Src\ClassLib\CFireworks.cpp (tags 0x0007f490-0x0007ffa0).

Fireworks: rockets that leave smoke trails and burst into coloured sparks. Particles
come from fixed pools (free list / active list) and are drawn as coloured points
through a vertex batch (classlib_fx 0x00084f30). The point writer 0x0007f3d0 is placed
here too (inferred: only the fireworks call it).

Point batch: 0x00 points per vertex buffer, 0x04.. vertex buffers (0-terminated),
  0x84 write position, 0x88 points left, 0x8c buffer index, 0x94 primitive type.
Spark pool (0x14): 0x00 count, 0x04 sparks, 0x08 active, 0x0c free, 0x10 batch.
Spark (0x50): 0x00 next, 0x04 position (x, y, z, w), 0x14 velocity, 0x24 gravity,
  0x28 alpha, 0x2c colour r, g, b, 0x38 colour speed r, g, b, 0x44 life, 0x48 fade
  start, 0x4c fading.
Trail pool (0x18): 0x00 count, 0x04 size, 0x08 trails, 0x0c active, 0x10 free,
  0x14 batch.
Trail (0x3c): 0x00 next, 0x04 position, 0x14 velocity, 0x24 alpha, 0x28 alpha speed,
  0x2c colour r, g, b, 0x38 life.
Fireworks (0x1c): 0x00 trail pool, 0x04 spark pool, 0x08 rocket count (50), 0x0c burst
  callback, 0x10 rockets, 0x14 active, 0x18 free.
Rocket (0x40): 0x00 next, 0x04 position, 0x14 velocity, 0x24 colour r, g, b, 0x30 life,
  0x34 spark life, 0x38 spark speed, 0x3c spark count.
*/

#include "classlib_ui.h"

#define OV_F(o, off) (*(float *)((o) + (off)))
#define OV_I(o, off) (*(int *)((o) + (off)))
#define OV_U(o, off) (*(unsigned int *)((o) + (off)))

extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bc898[256];
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bd640[256];
extern float DAT_002855f0[16];

/* rand() / 32767 as the game computes it (FILD * 3.051851e-05) */
#define CL_RAND01() ((double)CLU_Rand() * (double)3.051851e-05f)

/* 0x0007f3d0  ClassLib/CFireworks.cpp (inferred; thiscall)
Writes one coloured point into the batch; a full vertex buffer is drawn and the next
one locked (nothing is written when there is none). */
void CLPoints_Add(int *self, float *pos, unsigned int colour)
{
	int index;
	int point;

	if ((unsigned int)self[0x22] < 1) {
		FUN_0003f790((struct bdVertexBuffer *)self[self[0x23] + 1]);
		/* +0x94 is the point size (float): bdDrawPointSprites takes it in a float register */
		((void (*)(int, int, int, float))FUN_000406d0)(self[self[0x23] + 1], 0, self[0],
			*(float *)&self[0x25]);
		index = self[0x23];
		if (self[index + 2] == 0)
			return;
		self[0x23] = index + 1;
		self[0x22] = self[0];
		self[0x21] = ((int (*)(int, unsigned int))FUN_0003f710)(self[index + 2], 0);
	}
	point = self[0x21];
	OV_U(point, 0xc) = colour;
	OV_F(point, 0) = pos[0];
	OV_F(point, 4) = pos[1];
	OV_F(point, 8) = pos[2];
	self[0x21] = self[0x21] + 0x10;
	self[0x22] = self[0x22] + -1;
}

/* builds a pool's free list over `count` particles of `size` bytes */
static void CLFireworks_LinkFree(int buffer, int count, int size)
{
	int i;

	for (i = 0; i < count - 1; i++)
		*(int *)(buffer + i * size) = buffer + (i + 1) * size;
	*(int *)(buffer + (count - 1) * size) = 0;
}

/* ---------- sparks */

/* 0x0007f490  ClassLib/CFireworks.cpp (thiscall)
Spark pool constructor. */
int *CLSparks_Init(int *self, int count, uint batch_a, uint batch_b)
{
	int batch;

	self[0] = 0;
	self[1] = 0;
	self[2] = 0;
	self[3] = 0;
	self[4] = 0;
	self[0] = count;
	self[1] = (int)CLU_Alloc(count * 0x50, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CFireworks.cpp", 0x1c, 0x2001);
	if (self[1] == 0) {
		self[0] = 0;
		return self;
	}
	memset((void *)self[1], 0, (count * 5 & 0xfffffff) << 4);
	self[3] = self[1];
	self[2] = 0;
	CLFireworks_LinkFree(self[1], count, 0x50);
	batch = (int)CLU_Alloc(0x9c, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CFireworks.cpp", 0x2d, 0x1001);
	if (batch == 0)
		batch = 0;
	else
		batch = (int)FUN_00084f30((struct CPointSprites *)batch, 2, batch_a, count, batch_b);
	self[4] = batch;
	return self;
}

/* 0x0007f590  ClassLib/CFireworks.cpp (thiscall; the argument is not used)
Moves the sparks and fades their colour in, then out; dead sparks go to the free
list. */
void CLSparks_Update(int self, undefined4 unused)
{
	int *link;
	int spark;
	int next;
	int dead;
	float life, r, g, b;
	double inv;

	(void)unused;
	link = (int *)(self + 8);
	spark = *link;
	while (spark != 0) {
		life = OV_F(spark, 0x44) - 1.0f;
		OV_F(spark, 0x44) = life;
		dead = life <= 0.0f;
		OV_F(spark, 0x18) = OV_F(spark, 0x24) + OV_F(spark, 0x18);
		r = OV_F(spark, 0x2c) + OV_F(spark, 0x38);
		OV_F(spark, 0x2c) = r;
		g = OV_F(spark, 0x3c) + OV_F(spark, 0x30);
		OV_F(spark, 0x30) = g;
		b = OV_F(spark, 0x34) + OV_F(spark, 0x40);
		OV_F(spark, 0x34) = b;
		if (OV_I(spark, 0x4c) == 0 && life < OV_F(spark, 0x48)) {
			OV_I(spark, 0x4c) = 1;
			inv = 1.0 / (double)life;
			OV_F(spark, 0x38) = (float)-((double)r * inv);
			OV_F(spark, 0x3c) = (float)-((double)g * inv);
			OV_F(spark, 0x40) = (float)-(inv * (double)b);
		}
		OV_F(spark, 4) = OV_F(spark, 0x14) + OV_F(spark, 4);
		OV_F(spark, 8) = OV_F(spark, 8) + OV_F(spark, 0x18);
		OV_F(spark, 0xc) = OV_F(spark, 0xc) + OV_F(spark, 0x1c);
		next = OV_I(spark, 0);
		if (dead) {
			*link = next;
			OV_I(spark, 0) = OV_I(self, 0xc);
			OV_I(self, 0xc) = spark;
		} else {
			link = (int *)spark;
		}
		spark = next;
	}
}

/* 0x0007f6a0  ClassLib/CFireworks.cpp (thiscall)
Draws the sparks. */
void CLSparks_Draw(int self)
{
	int spark;
	unsigned int colour;

	if (OV_I(self, 8) == 0)
		return;
	FUN_00043930((float *)DAT_002855f0);
	FUN_00085060((struct CPointSprites *)OV_I(self, 0x10));
	for (spark = OV_I(self, 8); spark != 0; spark = OV_I(spark, 0)) {
		colour = DAT_002bd640[(int)OV_F(spark, 0x34)] | DAT_002bc490[(int)OV_F(spark, 0x30)];
		colour = colour | DAT_002bce18[(int)OV_F(spark, 0x2c)];
		colour = colour | DAT_002bc898[(int)OV_F(spark, 0x28)];
		CLPoints_Add((int *)OV_I(self, 0x10), (float *)(spark + 4), colour);
	}
	FUN_000d31f0((struct CFire_PointSprites *)OV_I(self, 0x10));
}

/* 0x0007f730  ClassLib/CFireworks.cpp (thiscall)
Starts a spark (NULL when the pool is empty): its colour rises from black to
(r, g, b) and fades out over the last `fade` part of its life (life scaled by a random
1-2). */
int CLSparks_Spawn(int self, undefined4 *pos, undefined4 *velocity, undefined4 gravity, float r, float g, float b,
		   float life, float fade)
{
	int spark;

	spark = OV_I(self, 0xc);
	if (spark == 0)
		return 0;
	OV_I(self, 0xc) = OV_I(spark, 0);
	OV_I(spark, 0) = OV_I(self, 8);
	OV_I(self, 8) = spark;
	OV_U(spark, 4) = pos[0];
	OV_U(spark, 8) = pos[1];
	OV_U(spark, 0xc) = pos[2];
	OV_U(spark, 0x10) = pos[3];
	OV_F(spark, 0x10) = 1.0f;
	OV_U(spark, 0x14) = velocity[0];
	OV_U(spark, 0x18) = velocity[1];
	OV_U(spark, 0x1c) = velocity[2];
	OV_U(spark, 0x24) = gravity;
	OV_I(spark, 0x20) = 0;
	OV_F(spark, 0x2c) = 0.0f;
	OV_F(spark, 0x30) = 0.0f;
	OV_F(spark, 0x34) = 0.0f;
	life = (float)(CL_RAND01() * (double)life + (double)life);
	OV_I(spark, 0x4c) = 0;
	OV_F(spark, 0x28) = 127.0f;
	OV_F(spark, 0x44) = life;
	OV_F(spark, 0x48) = (float)(((double)1.0f - (double)fade) * (double)life);
	OV_F(spark, 0x38) = (float)((double)r / ((double)OV_F(spark, 0x44) - (double)OV_F(spark, 0x48)));
	OV_F(spark, 0x3c) = (float)((double)g / ((double)OV_F(spark, 0x44) - (double)OV_F(spark, 0x48)));
	OV_F(spark, 0x40) = (float)((double)b / ((double)OV_F(spark, 0x44) - (double)OV_F(spark, 0x48)));
	return spark;
}

/* ---------- trails */

/* 0x0007f820  ClassLib/CFireworks.cpp (thiscall)
Trail pool constructor (`size` scales the smoke speed). */
int *CLTrails_Init(int *self, int count, uint batch_a, int size)
{
	int batch;

	memset(self, 0, 6 * 4);
	self[0] = count;
	self[1] = size;
	self[2] = (int)CLU_Alloc(count * 0x3c, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CFireworks.cpp", 0xcd, 0x2001);
	if (self[2] == 0) {
		self[0] = 0;
		return self;
	}
	memset((void *)self[2], 0, ((unsigned int)(count * 0x3c) >> 2) * 4);
	self[4] = self[2];
	self[3] = 0;
	CLFireworks_LinkFree(self[2], count, 0x3c);
	batch = (int)CLU_Alloc(0x9c, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CFireworks.cpp", 0xdf, 0x1001);
	if (batch == 0)
		batch = 0;
	else
		batch = (int)FUN_00084f30((struct CPointSprites *)batch, 1, batch_a, count, size);
	self[5] = batch;
	return self;
}

/* 0x0007f920  ClassLib/CFireworks.cpp (thiscall; the argument is not used)
Moves the trail puffs and fades them; dead or invisible puffs go to the free list. */
void CLTrails_Update(int self, undefined4 unused)
{
	int *link;
	int trail;
	int next;
	int dead;

	(void)unused;
	link = (int *)(self + 0xc);
	trail = *link;
	while (trail != 0) {
		OV_F(trail, 0x38) = OV_F(trail, 0x38) - 1.0f;
		dead = OV_F(trail, 0x38) <= 0.0f;
		OV_F(trail, 0x24) = OV_F(trail, 0x28) + OV_F(trail, 0x24);
		if (OV_F(trail, 0x24) < 0.0f) {
			OV_F(trail, 0x24) = 0.0f;
			dead = 1;
		}
		if (OV_F(trail, 0x24) > 127.0f)
			OV_F(trail, 0x24) = 127.0f;
		OV_F(trail, 4) = OV_F(trail, 0x14) + OV_F(trail, 4);
		OV_F(trail, 8) = OV_F(trail, 8) + OV_F(trail, 0x18);
		OV_F(trail, 0xc) = OV_F(trail, 0xc) + OV_F(trail, 0x1c);
		next = OV_I(trail, 0);
		if (dead) {
			*link = next;
			OV_I(trail, 0) = OV_I(self, 0x10);
			OV_I(self, 0x10) = trail;
		} else {
			link = (int *)trail;
		}
		trail = next;
	}
}

/* 0x0007f9d0  ClassLib/CFireworks.cpp (thiscall)
Draws the trail puffs. */
void CLTrails_Draw(int self)
{
	int trail;
	unsigned int colour;

	FUN_00043930((float *)DAT_002855f0);
	FUN_00085060((struct CPointSprites *)OV_I(self, 0x14));
	for (trail = OV_I(self, 0xc); trail != 0; trail = OV_I(trail, 0)) {
		colour = DAT_002bd640[(int)OV_F(trail, 0x34)] | DAT_002bc490[(int)OV_F(trail, 0x30)];
		colour = colour | DAT_002bce18[(int)OV_F(trail, 0x2c)];
		colour = colour | DAT_002bc898[(int)OV_F(trail, 0x24)];
		CLPoints_Add((int *)OV_I(self, 0x14), (float *)(trail + 4), colour);
	}
	FUN_000d31f0((struct CFire_PointSprites *)OV_I(self, 0x14));
}

/* 0x0007fa50  ClassLib/CFireworks.cpp (thiscall)
Starts a grey smoke puff at `pos` with a small random velocity (plus `rise` upwards). */
void CLTrails_Spawn(int self, undefined4 *pos, float rise)
{
	int trail;
	double size, spread, half, grey;
	int r1, r2;

	trail = OV_I(self, 0x10);
	if (trail == 0)
		return;
	OV_I(self, 0x10) = OV_I(trail, 0);
	OV_I(trail, 0) = OV_I(self, 0xc);
	OV_I(self, 0xc) = trail;
	OV_U(trail, 4) = pos[0];
	OV_U(trail, 8) = pos[1];
	OV_U(trail, 0xc) = pos[2];
	OV_U(trail, 0x10) = pos[3];
	OV_F(trail, 0x10) = 1.0f;
	r1 = CLU_Rand();
	r2 = CLU_Rand();
	size = (double)r1 * (double)3.051851e-05f * ((double)OV_F(self, 4) * (double)0.25f) +
	       (double)OV_F(self, 4) * (double)0.75f;
	spread = (double)0.01f * size;
	half = size * (double)0.005f;
	OV_F(trail, 0x14) = (float)((double)r2 * (double)3.051851e-05f * spread - half);
	OV_F(trail, 0x18) = (float)(CL_RAND01() * spread - half + (double)rise);
	OV_F(trail, 0x20) = 0.0f;
	OV_F(trail, 0x1c) = (float)(CL_RAND01() * spread - half);
	grey = CL_RAND01() * (double)64.0f + (double)32.0f;
	OV_F(trail, 0x2c) = (float)grey;
	OV_F(trail, 0x30) = (float)grey;
	OV_F(trail, 0x34) = (float)grey;
	OV_F(trail, 0x24) = 127.0f;
	OV_F(trail, 0x38) = (float)(CL_RAND01() * (double)20.0f + (double)20.0f);
	OV_F(trail, 0x28) = (float)((double)-127.0f / (double)OV_F(trail, 0x38));
}

/* ---------- rockets */

/* 0x0007fc30  ClassLib/CFireworks.cpp (thiscall)
Bursts a rocket: slows it to a quarter and throws its sparks in random directions
(scaled by FUN_00051570 with the rocket's spark speed) around its velocity. */
void CLFireworks_Burst(int self, int rocket)
{
	float dir[4];
	float velocity[4];
	int i;

	OV_F(rocket, 0x14) = OV_F(rocket, 0x14) * 0.25f;
	OV_F(rocket, 0x18) = OV_F(rocket, 0x18) * 0.25f;
	OV_F(rocket, 0x1c) = OV_F(rocket, 0x1c) * 0.25f;
	for (i = 0; i < OV_I(rocket, 0x3c); i++) {
		dir[0] = (float)(CL_RAND01() * (double)10.0f - (double)5.0f);
		dir[1] = (float)(CL_RAND01() * (double)10.0f - (double)5.0f);
		dir[2] = (float)(CL_RAND01() * (double)10.0f - (double)5.0f);
		FUN_00051570(velocity, dir, OV_F(rocket, 0x38));
		velocity[0] = velocity[0] + OV_F(rocket, 0x14);
		velocity[1] = velocity[1] + OV_F(rocket, 0x18);
		velocity[2] = velocity[2] + OV_F(rocket, 0x1c);
		CLSparks_Spawn(OV_I(self, 4), (undefined4 *)(rocket + 4), (undefined4 *)velocity, 0xbe19999a /* -0.15f */,
			       OV_F(rocket, 0x24), OV_F(rocket, 0x28), OV_F(rocket, 0x2c), OV_F(rocket, 0x34), 0.5f);
	}
}

/* 0x0007fdb0  ClassLib/CFireworks.cpp (inferred); missing from of_functions.tsv
Default burst callback of the fireworks (cdecl wrapper of CLFireworks_Burst). */
void FUN_0007fdb0(int fireworks, int rocket)
{
	CLFireworks_Burst(fireworks, rocket);
}

/* 0x0007fdc0  ClassLib/CFireworks.cpp (thiscall)
Per frame: trails, sparks and rockets (each rocket leaves a puff; at the end of its
life the burst callback runs). */
void CLFireworks_Update(int self, undefined4 arg)
{
	int *link;
	int rocket;
	int next;
	int dead;

	CLTrails_Update(OV_I(self, 0), arg);
	CLSparks_Update(OV_I(self, 4), arg);
	link = (int *)(self + 0x14);
	rocket = *link;
	while (rocket != 0) {
		dead = 0;
		OV_F(rocket, 0x30) = OV_F(rocket, 0x30) - 1.0f;
		if (OV_F(rocket, 0x30) <= 0.0f) {
			dead = 1;
			if (OV_I(self, 0xc) != 0)
				((void (*)(int, int))OV_U(self, 0xc))(self, rocket);
		}
		OV_F(rocket, 4) = OV_F(rocket, 0x14) + OV_F(rocket, 4);
		OV_F(rocket, 8) = OV_F(rocket, 8) + OV_F(rocket, 0x18);
		OV_F(rocket, 0xc) = OV_F(rocket, 0xc) + OV_F(rocket, 0x1c);
		CLTrails_Spawn(OV_I(self, 0), (undefined4 *)(rocket + 4), 0.0f);
		next = OV_I(rocket, 0);
		if (dead) {
			*link = next;
			OV_I(rocket, 0) = OV_I(self, 0x18);
			OV_I(self, 0x18) = rocket;
		} else {
			link = (int *)rocket;
		}
		rocket = next;
	}
}

/* 0x0007fe60  ClassLib/CFireworks.cpp (thiscall)
Draws the trails and the sparks. */
void CLFireworks_Draw(int self)
{
	CLTrails_Draw(OV_I(self, 0));
	CLSparks_Draw(OV_I(self, 4));
}

/* 0x0007fe80  ClassLib/CFireworks.cpp (thiscall)
Launches a rocket (NULL when none is free); r, g, b are clamped to 0-255. */
int CLFireworks_Launch(int self, undefined4 *pos, undefined4 *velocity, undefined4 life, float r, float g, float b,
		       undefined4 spark_count, undefined4 spark_speed, undefined4 spark_life)
{
	int rocket;

	rocket = OV_I(self, 0x18);
	if (rocket == 0)
		return 0;
	OV_I(self, 0x18) = OV_I(rocket, 0);
	OV_I(rocket, 0) = OV_I(self, 0x14);
	OV_I(self, 0x14) = rocket;
	OV_U(rocket, 4) = pos[0];
	OV_U(rocket, 8) = pos[1];
	OV_U(rocket, 0xc) = pos[2];
	OV_U(rocket, 0x10) = pos[3];
	OV_F(rocket, 0x10) = 1.0f;
	OV_U(rocket, 0x30) = life;
	OV_U(rocket, 0x14) = velocity[0];
	OV_U(rocket, 0x18) = velocity[1];
	OV_U(rocket, 0x1c) = velocity[2];
	OV_U(rocket, 0x20) = velocity[3];
	if (255.0f < r)
		r = 255.0f;
	if (255.0f < g)
		g = 255.0f;
	if (255.0f < b)
		b = 255.0f;
	if (r < 0.0f)
		r = 0.0f;
	if (g < 0.0f)
		g = 0.0f;
	if (b < 0.0f)
		b = 0.0f;
	OV_F(rocket, 0x2c) = b;
	OV_F(rocket, 0x24) = r;
	OV_F(rocket, 0x28) = g;
	OV_U(rocket, 0x3c) = spark_count;
	OV_U(rocket, 0x38) = spark_speed;
	OV_U(rocket, 0x34) = spark_life;
	return rocket;
}

/* 0x0007ffa0  ClassLib/CFireworks.cpp (thiscall)
Fireworks constructor: trail pool (trail_count, trail_a, trail_size), spark pool
(spark_count, spark_a, spark_b) and 50 rockets. */
int *CLFireworks_Init(int *self, int trail_count, uint trail_a, int trail_size, int spark_count, uint spark_a,
		      uint spark_b)
{
	int pool;

	memset(self, 0, 7 * 4);
	self[3] = (int)FUN_0007fdb0;
	pool = (int)CLU_Alloc(0x14, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CFireworks.cpp", 0x17b, 0x1001);
	if (pool == 0)
		pool = 0;
	else
		pool = (int)CLSparks_Init((int *)pool, spark_count, spark_a, spark_b);
	self[1] = pool;
	pool = (int)CLU_Alloc(0x18, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CFireworks.cpp", 0x17c, 0x1001);
	if (pool == 0)
		pool = 0;
	else
		pool = (int)CLTrails_Init((int *)pool, trail_count, trail_a, trail_size);
	self[0] = pool;
	self[2] = 0x32;
	self[4] = (int)CLU_Alloc(0xc80, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CFireworks.cpp", 0x181, 0x2001);
	memset((void *)self[4], 0, (self[2] & 0x3ffffff) << 6);
	self[5] = 0;
	self[6] = self[4];
	if (self[2] != 1)
		CLFireworks_LinkFree(self[4], self[2], 0x40);
	else
		*(int *)(self[2] * 0x40 + -0x40 + self[4]) = 0;
	return self;
}
