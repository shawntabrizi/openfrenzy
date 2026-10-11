/*
Hopper/unplaced_sparks.c  <-  gap functions 0x0013b0c0-0x0013b910 (no source path in the XBE)
(module hopper)

Two bactor "extra" systems that only FoxHunt uses (foxhunt 0x00134360 creates them, 0x00133fd0
updates and 0x00133ff0 draws them): falling sparks that bounce off the ground (a collision
callback given at creation, foxhunt's ray cast 0x00134040) and burst into debris, and fading
lightning quads. The module table puts these functions in Hopper (they follow FoxHunt's code
and precede CModule_Hopper.cpp); 0x0013b0c0 lies in the gap before the table's first Hopper
function and is not in the table.

The owner object (foxhunt's, passed in ECX) is two system pointers: struct HopSparks.

Spark item (0x3c bytes, 0x200 items):  [0..2] position, [3] 1, [4..6] velocity (per second),
  [7] 1, [8] ground height without a collision callback, [9] trail length,
  [10..13] red, green, blue, alpha (ints).
Lightning item (0x3c bytes, 0x200 items): [0..2] position, [10..13] red, green, blue, alpha
  (ints, fading by 10 a frame), [14] texture.
*/

#include "hopper.h"

/* foxhunt's object: the two systems */
struct HopSparks {
	struct BExtraSystem *sparks;     /* +0x0 "rain" */
	struct BExtraSystem *lightning;  /* +0x4 "lightening" */
};

/* the collision callback: (position, step, hit out, normal out) -> fraction of the step to the
hit, 0 when nothing is hit */
typedef float (*HopCollideFn)(float *pos, float *step, float *hit, float *normal);

#define hop_SparkTexture HOP_G(0x003023d4, struct bdTexture *)   /* the lightning texture */
#define hop_SparkCollide HOP_G(0x003023d8, HopCollideFn)
#define hop_LightningQuad ((float *)HOP_ADDR(0x003023e0))          /* 4 vertices (0x24 bytes each) */
#define hop_SparkLine ((float *)HOP_ADDR(0x00302470))              /* 2 vertices */

/* foxhunt 0x00134000 (thiscall on its object 0x003022cc): adds debris of a type at a position */
extern unsigned char DAT_003022cc[];

/* 0x0013b0c0  (gap; not in the function table): spark creation. args: x, y, z, vx, vy, vz,
ground height (doubles), red, green, blue, alpha (ints), trail length (double). */
int FUN_0013b0c0(struct BExtraSystem *system, float *item, void *args)
{
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	item[0] = (float)va_arg(ap, double);
	item[1] = (float)va_arg(ap, double);
	item[2] = (float)va_arg(ap, double);
	item[3] = 1.0f;
	item[4] = (float)va_arg(ap, double);
	item[5] = (float)va_arg(ap, double);
	item[6] = (float)va_arg(ap, double);
	item[7] = 1.0f;
	item[8] = (float)va_arg(ap, double);
	HOP_AI(item, 0x28) = va_arg(ap, int);
	HOP_AI(item, 0x2c) = va_arg(ap, int);
	HOP_AI(item, 0x30) = va_arg(ap, int);
	HOP_AI(item, 0x34) = va_arg(ap, int);
	item[9] = (float)va_arg(ap, double);
	return 0;
}

/* 0x0013b140  (gap): adds a spark (thiscall on the owner) */
void HopSparks_AddSpark(struct HopSparks *self, float *pos, float *vel, float ground, int red, int green,
	int blue, int alpha, float length)
{
	baAddExtraItem(self->sparks, (double)pos[0], (double)pos[1], (double)pos[2], (double)vel[0],
		(double)vel[1], (double)vel[2], (double)ground, red, green, blue, alpha, (double)length);
}

/* 0x0013b1b0  (gap; not in the function table): lightning creation. args: x, y, z (doubles),
red, green, blue, alpha (ints). */
int FUN_0013b1b0(struct BExtraSystem *system, float *item, void *args)
{
	va_list ap;

	(void)system;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	item[0] = (float)va_arg(ap, double);
	item[1] = (float)va_arg(ap, double);
	item[2] = (float)va_arg(ap, double);
	HOP_AI(item, 0x28) = va_arg(ap, int);
	HOP_AI(item, 0x2c) = va_arg(ap, int);
	HOP_AI(item, 0x30) = va_arg(ap, int);
	HOP_AI(item, 0x34) = va_arg(ap, int);
	HOP_AP(item, 0x38) = hop_SparkTexture;
	return 0;
}

/* 0x0013b200  (gap; not in the function table): lightning update: fades every quad by 10 and
deletes it when its alpha is below 11 */
int FUN_0013b200(struct BExtraSystem *system, int count, void **items, void *arg)
{
	int *item;
	int i = 0;

	(void)arg;
	while (i < count) {
		item = (int *)items[i];
		if (item[0xd] < 0xb) {
			baDeleteExtraItem(system, i);
			count--;
		} else {
			item[0xd] -= 10;
			item[0xa] -= 10;
			item[0xb] -= 10;
			item[0xc] -= 10;
			i++;
		}
	}
	return 0;
}

/* 0x0013b260  (gap; not in the function table): lightning draw: a textured quad 3500 high and
500 deep from each position (the first vertex is a fixed grey (50, 50, 70)) */
int FUN_0013b260(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *quad = hop_LightningQuad;
	float *item;
	unsigned int colour;
	int i;

	(void)system;
	(void)arg;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetWorldIdentity();
	for (i = 0; i < count; i++) {
		item = (float *)items[i];
		bdSetTexture(0, hop_SparkTexture);
		/* vertex 0: the position */
		quad[0] = item[0];
		quad[1] = item[1];
		quad[2] = item[2];
		quad[7] = 1.0f;
		quad[8] = 1.0f;
		/* vertex 1: 3500 up */
		quad[9] = item[0];
		quad[10] = item[1] + 3500.0f;
		quad[11] = item[2];
		quad[0x10] = 1.0f;
		quad[0x11] = 0.0f;
		/* vertex 2: 500 back */
		quad[0x12] = item[0];
		quad[0x13] = item[1];
		quad[0x14] = item[2] - 500.0f;
		quad[0x19] = 0.0f;
		quad[0x1a] = 1.0f;
		/* vertex 3 */
		quad[0x1b] = item[0];
		quad[0x1c] = item[1] + 3500.0f;
		HOP_AU(quad, 0x18) = HOP_ARGB(255, 50, 50, 70);
		quad[0x1d] = item[2] - 500.0f;
		quad[0x22] = 0.0f;
		quad[0x23] = 0.0f;
		colour = HOP_ARGB(HOP_AI(item, 0x34), HOP_AI(item, 0x28), HOP_AI(item, 0x2c), HOP_AI(item, 0x30));
		HOP_AU(quad, 0x3c) = colour;
		HOP_AU(quad, 0x60) = colour;
		HOP_AU(quad, 0x84) = colour;
		bdDrawPrimitiveFVF(4, quad, 4, 0x152);
	}
	bdSetRenderState(6, 0, 0);
	return 0;
}

/* 0x0013b460  (gap): deletes both systems */
void HopSparks_Delete(struct HopSparks *self)
{
	baDeleteExtraSystem(self->sparks);
	baDeleteExtraSystem(self->lightning);
}

/* 0x0013b480  (gap): updates both systems (dt: the frame time) */
void HopSparks_Update(struct HopSparks *self, float dt)
{
	baUpdateExtras(self->sparks, (void *)(uintptr_t)hop_fbits(dt));
	baUpdateExtras(self->lightning, (void *)(uintptr_t)hop_fbits(dt));
}

/* 0x0013b4b0  (gap): draws both systems */
void HopSparks_Draw(struct HopSparks *self)
{
	baDrawExtras(self->sparks, NULL);
	baDrawExtras(self->lightning, NULL);
}

/* two debris bursts (type 0x2b) where a spark landed */
static void hop_SparkBurst(float *hit)
{
	float vel[4];
	int i;
	unsigned int r1, r2, r3;

	for (i = 2; i != 0; i--) {
		r1 = (unsigned int)hop_rand();
		r2 = (unsigned int)hop_rand();
		r3 = (unsigned int)hop_rand();
		vel[0] = (float)r1 * DAT_0028fda8 * 3.0518509e-05f;
		vel[1] = ((float)r2 * 3.0518509e-05f + 1.5f) * DAT_0028fda8;
		vel[2] = (float)r3 * DAT_0028fda8 * 3.0518509e-05f;
		vel[3] = 1.0f;
		FUN_00134000(0x2b, (float *)(uintptr_t)hit, (float *)(uintptr_t)vel, 0, 0xffffffff);
	}
}

/* 0x0013b4d0  (gap; not in the function table): spark update. arg: the frame time (a float's
bits). A spark moves by velocity * 60 * dt; when the step hits something (the collision
callback, or the ground height without one) it bursts into debris and is deleted. */
int FUN_0013b4d0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float scale = hop_bitsf((unsigned int)(uintptr_t)arg) * 60.0f;
	float step[4];
	float next[4];
	float hit[4];
	float normal[4];
	float *item;
	int i = 0;

	/* the original passes the (unwritten) hit buffer to the burst when there is no
	   callback; it is zeroed here */
	memset(hit, 0, sizeof hit);
	while (i < count) {
		item = (float *)items[i];
		/* the original copies an unset stack word into w; w keeps its value here */
		next[3] = item[3];
		step[0] = scale * item[4];
		step[1] = scale * item[5];
		step[2] = scale * item[6];
		next[0] = step[0] + item[0];
		next[1] = step[1] + item[1];
		next[2] = step[2] + item[2];
		if (hop_SparkCollide == NULL) {
			/* fcomp; test ah, 0x41; jp: moves only when above the ground (or NaN) */
			if (!(next[1] <= item[8])) {
				memcpy(item, next, 16);
				i++;
				continue;
			}
			bmVectorSetLength(step, step, item[8] - next[1]);
			step[0] += item[0];
			step[1] += item[1];
			step[2] += item[2];
		} else if (hop_SparkCollide(item, step, hit, normal) <= 0.0f) {
			memcpy(item, next, 16);
			i++;
			continue;
		}
		hop_SparkBurst(hit);
		baDeleteExtraItem(system, i);
		count--;
	}
	return 0;
}

/* 0x0013b7d0  (gap; not in the function table): spark draw: a line from each spark back along
its velocity, trail length long, fading to alpha 0 */
int FUN_0013b7d0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *line = hop_SparkLine;
	float dir[4];
	float *item;
	int i;

	(void)system;
	(void)arg;
	bdSetRenderState(6, 1, 0);
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
	for (i = 0; i < count; i++) {
		item = (float *)items[i];
		bmVectorNormalise(dir, item + 4);
		dir[0] = dir[0] * item[9];
		dir[1] = dir[1] * item[9];
		dir[2] = item[9] * dir[2];
		line[0] = item[0];
		line[1] = item[1];
		line[2] = item[2];
		HOP_AU(line, 0x18) = HOP_ARGB(HOP_AI(item, 0x34), HOP_AI(item, 0x28), HOP_AI(item, 0x2c),
			HOP_AI(item, 0x30));
		line[9] = item[0] - dir[0];
		line[10] = item[1] - dir[1];
		line[11] = item[2] - dir[2];
		HOP_AU(line, 0x3c) = HOP_ARGB(0, HOP_AI(item, 0x28), HOP_AI(item, 0x2c), HOP_AI(item, 0x30));
		bdDrawPrimitiveFVF(2, line, 2, 0x152);
	}
	bdSetRenderState(6, 0, 0);
	return 0;
}

/* 0x0013b910  (gap): creates the systems: "rain" (sparks) and "lightening"; collide is the
sparks' collision callback (NULL: the ground height of each spark). Returns 1. */
int HopSparks_Init(struct HopSparks *self, void *collide)
{
	hop_SparkCollide = (HopCollideFn)collide;
	self->sparks = baCreateExtraSystem("rain", (int (*)())FUN_0013b0c0, (int (*)())FUN_0013b4d0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_0013b7d0, NULL, 0x3c, 0x200, 1, 0);
	self->lightning = baCreateExtraSystem("lightening", (int (*)())FUN_0013b1b0, (int (*)())FUN_0013b200,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_0013b260, NULL, 0x3c, 0x200, 1, 0);
	return 1;
}
