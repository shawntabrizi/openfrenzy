/*
UNPLACED_LIGHTNING.C  <-  gap functions 0x0015c8c0-0x0015cc80 of C:\BlitzParty\Game\Src\Meta\
(no source path in the XBE; after Effects.cpp, before meta_b's CLightning.cpp at
0x0015cef0). Except 0x0015c910 they are methods of the lightning manager and its bolts,
so their file is probably CLightning.cpp; the table gives them to meta_a.

Layouts: meta_b's struct CLightningBolt and struct CLightningMgr (src/meta_b/
meta_b_internal.h). Offsets used here:

  bolt (0x48): +0x08 float pos[4] where it strikes, +0x18 float scatter radius,
               +0x1c float start[4], +0x2c float end[4], +0x3c brightness, +0x44 life
  manager (DAT_00308460): +0x00 short number of groups, +0x04 8 CLLists (0x0c each),
               +0x64 short group ids, +0x74 strike time, +0x78 accumulator,
               +0x7c strike interval, +0x80 strike group
*/

#include "meta_a.h"

/* 0x002b1e44  the smoke debris type (classlib_fx DebrisType, in the Meta .data block) */
#define MA_DEBRIS_SMOKE ((struct DebrisType *)MA_META_ADDR(0x002b1e44u))
/* 0x002b2290 / 0x002b22e8  the CORONABLUE / CORONAWHITE debris types */
#define MA_DEBRIS_CORONABLUE ((struct DebrisType *)MA_META_ADDR(0x002b2290u))
#define MA_DEBRIS_CORONAWHITE ((struct DebrisType *)MA_META_ADDR(0x002b22e8u))

/* ---------- data (.bss) */

/* 0x00308458  number of live lightning bolts (a word; meta_b reads it as short) */
short DAT_00308458;
/* 0x003084e8  the vertices of the bolt being drawn: up to 13 of 0x24 bytes (position at
   +0x00, colour at +0x18) */
unsigned int DAT_003084e8[13 * 9];

/* 0x0015c8c0  Meta (file inferred: CLightning.cpp): the lightning manager's destructor
   (fastcall; static destructor of DAT_00308460): destroy the 8 group lists (last first;
   the list destructor 0x001d5950 is empty) */
void FUN_0015c8c0(int self)
{
	int i;

	(void)self;
	for (i = 8; i > 0; i--)
		FUN_001d5950();
}

/* 0x0015c8e0  Meta (file inferred: CLightning.cpp): the manager (thiscall): strike
   `group` automatically every `interval` for `time` (puzzle action; the floats are
   passed as raw dwords) */
void FUN_0015c8e0(int self, undefined4 group, undefined4 interval, undefined4 time)
{
	MA_U32(self, 0x80) = group;
	MA_U32(self, 0x74) = time;
	MA_U32(self, 0x7c) = interval;
	MA_U32(self, 0x78) = 0;
}

/* 0x0015c910  Meta: a burst at `pos` (a float *; a player's position when it drops or
   loses its orbs): 9 rings of corona debris (CORONABLUE, CORONAWHITE with colour
   0x000affc8, CORONAWHITE) and a light flash */
void FUN_0015c910(undefined4 pos)
{
	int i;

	for (i = 0; i < 9; i++) {
		switch (i % 3) {
		case 0:
			Debris_AddRing(MA_DEBRIS_CORONABLUE, i + 7, (float *)pos,
				(float)i * 0.6f + 0.5f, 8.0f, 28.0f, i, 0xffffffffu); /* 0x00265738, 0x002536d0 */
			break;
		case 1:
			Debris_AddRing(MA_DEBRIS_CORONAWHITE, i + 7, (float *)pos,
				(float)i * 0.6f + 0.5f, 8.0f, 28.0f, i, 0xaffc8u);
			break;
		case 2:
			Debris_AddRing(MA_DEBRIS_CORONAWHITE, i + 7, (float *)pos,
				(float)i * 0.6f + 0.5f, 8.0f, 28.0f, i, 0xffffffffu);
			break;
		}
	}
	Light_NewFlash((float *)pos, 5000.0f, 0x50, 100, 100, 0x7f, 0.0f);
}

/* 0x0015ca00  Meta (file inferred: CLightning.cpp): the lightning manager's constructor
   (fastcall; static constructor of DAT_00308460) */
undefined2 *FUN_0015ca00(undefined2 *self)
{
	int i;

	for (i = 0; i < 8; i++)
		CLList_Init((int *)((char *)self + 4 + i * 0xc));
	MA_I16(self, 0) = 0;
	MA_I16(self, 0x64) = 0;
	DAT_00308458 = 0;
	MA_I32(self, 0x74) = 0;
	MA_I32(self, 0x7c) = 0;
	MA_I32(self, 0x78) = 0;
	return self;
}

/* 0x0015ca40  Meta (file inferred: CLightning.cpp): strike a bolt (fastcall): it lives
   22 frames; it starts at its position (or at a random point `scatter` away from it)
   and ends 3000 units above; two puffs of smoke */
void FUN_0015ca40(int bolt)
{
	float radius[4];
	float up[4];
	float *start;

	if (MA_F32(bolt, 0x44) == 0.0f)
		DAT_00308458 = DAT_00308458 + 1;
	MA_F32(bolt, 0x44) = 22.0f;
	start = MA_FP(bolt, 0x1c);
	if (MA_F32(bolt, 0x18) == 0.0f) {
		memcpy(start, MA_FP(bolt, 8), 4 * sizeof(float));
	} else {
		radius[0] = MA_F32(bolt, 0x18);
		radius[1] = 0.0f;
		radius[2] = 0.0f;
		radius[3] = 1.0f;
		bmVectorRotateY(start, radius, (float)ma_rand() * 0.000191656247f); /* 0x0026fd58 */
		start[0] = MA_F32(bolt, 8) + start[0];
		start[1] = MA_F32(bolt, 0xc) + start[1];
		start[2] = MA_F32(bolt, 0x10) + start[2];
	}
	memcpy(MA_FP(bolt, 0x2c), start, 4 * sizeof(float));
	MA_F32(bolt, 0x30) = MA_F32(bolt, 0x30) + 3000.0f; /* 0x0026b924 */
	up[0] = 0.0f;
	up[1] = 10.0f;
	up[2] = 0.0f;
	up[3] = 1.0f;
	Debris_Add(MA_DEBRIS_SMOKE, start, (float *)DAT_002ea478, 0, 0xffffffffu);
	Debris_Add(MA_DEBRIS_SMOKE, start, up, 0, 0xffffffffu);
}

/* 0x0015cb80  Meta (file inferred: CLightning.cpp): a bolt hits the players (fastcall):
   a player within 200 units of where it strikes, and not already in state 10 or 13, goes
   to state 10 (hit) and is pushed away at speed 4 */
void FUN_0015cb80(int bolt)
{
	int i;
	int character;
	float d[3];
	float dist2;
	float dist;
	float k;
	short state;

	for (i = 0; i < 4; i++) {
		character = MA_I32(DAT_003075bc[i], 0x30);
		d[0] = MA_F32(character, 0xd0) - MA_F32(bolt, 8);
		d[1] = MA_F32(character, 0xd4) - MA_F32(bolt, 0xc);
		d[2] = MA_F32(character, 0xd8) - MA_F32(bolt, 0x10);
		dist2 = d[2] * d[2] + d[1] * d[1] + d[0] * d[0];
		if (!(dist2 < 40000.0f)) /* 0x0026fd74 */
			continue;
		state = MA_I16(DAT_003075bc[i], 0x36);
		if (state == 10 || state == 0xd)
			continue;
		bkPrintf("LIGHTNING *** OUCH!\n"); /* 0x0026fd5c */
		AI_SetState((void *)DAT_003075bc[i], 10);
		dist = sqrtf(dist2);
		if (dist > 0.0f) {
			k = 4.0f / dist; /* 0x00257c40 */
			d[0] = d[0] * k;
			d[1] = d[1] * k;
			d[2] = k * d[2];
			Platform_AddVelocity(MA_FP(MA_I32(DAT_003075bc[i], 0x30), 0x194), d);
		}
	}
}

/* 0x0015cc80  Meta (file inferred: CLightning.cpp): draw a bolt from `from` to `to` as a
   primitive (bdDrawPrimitive type 2) of `segments` + 1 vertices; the inner ones are scattered up to 75 units
   sideways. Brightness 0..1 picks the colour. As in the original, vertex 0's colour is
   never written (the first colour store is vertex 1's) */
void FUN_0015cc80(float *from, float *to, int segments, float brightness)
{
	unsigned short level;
	unsigned short level2;
	unsigned int colour;
	float step;
	float delta[3];
	float cur[3];
	float jitter[4];
	float off[4];
	float *v;
	int i;
	int r1, r2;

	level = (unsigned short)(int)(brightness * 255.0f); /* 0x002537d0; _ftol */
	level2 = (unsigned short)(level + 100);
	if (level2 > 0xff)
		level2 = 0xff;
	v = (float *)DAT_003084e8;
	v[0] = from[0];
	v[1] = from[1];
	v[2] = from[2];
	colour = DAT_002bce18[level] | DAT_002bc898[level] | DAT_002bd640[level2] | DAT_002bc490[level];
	DAT_003084e8[9 + 6] = colour; /* 0x00308524: vertex 1, +0x18 */
	step = 1.0f / (float)segments;
	delta[0] = step * (to[0] - from[0]);
	delta[1] = step * (to[1] - from[1]);
	delta[2] = step * (to[2] - from[2]);
	cur[0] = delta[0] + from[0];
	cur[1] = delta[1] + from[1];
	cur[2] = delta[2] + from[2];
	for (i = 1; i < segments; i++) {
		r1 = ma_rand();
		r2 = ma_rand();
		jitter[0] = (float)r1 * 0.00228888821f; /* 0x0026fd78: 75 / 32767 */
		jitter[1] = 0.0f;
		jitter[2] = 0.0f;
		jitter[3] = 1.0f;
		bmVectorRotateY(off, jitter, (float)r2 * 0.000191656247f); /* 0x0026fd58 */
		v = (float *)&DAT_003084e8[i * 9];
		v[0] = off[0] + cur[0];
		v[1] = off[1] + cur[1];
		v[2] = off[2] + cur[2];
		DAT_003084e8[i * 9 + 6] =
			DAT_002bce18[level] | DAT_002bc898[level] | DAT_002bd640[level2] | DAT_002bc490[level];
		cur[0] = cur[0] + delta[0];
		cur[1] = cur[1] + delta[1];
		cur[2] = cur[2] + delta[2];
	}
	v = (float *)&DAT_003084e8[segments * 9];
	v[0] = to[0];
	v[1] = to[1];
	v[2] = to[2];
	DAT_003084e8[segments * 9 + 6] =
		DAT_002bce18[level] | DAT_002bc898[level] | DAT_002bd640[level2] | DAT_002bc490[level];
	bdDrawPrimitive(2, DAT_003084e8, segments + 1);
}
