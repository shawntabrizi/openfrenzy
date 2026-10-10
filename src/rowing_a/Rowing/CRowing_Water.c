/*
Rowing/CRowing_Water.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_Water.cpp

The whirlpool water of variant 3 (" Eddy, Steady, Go!"): a CWaterField subclass
(vtable 0x00274f60, its other methods are in Rowing/CRowing_Game.c) whose grid is laid out
as a spiral around the origin. 0x001a5400 is tagged with this file; the gap functions
0x001a5610 and 0x001a56f0 after it use the same fields and are inferred to be in it.

Fields after CWaterField (see Rowing/CRowing_Game.c): +0x9090 the cell positions
(x, y, z), +0x9094/+0x9098 the cos/sin of each cell's spiral angle, +0x909c 1.0, +0x90a0 the
whirlpool radius R, +0x90a4 its depth D, then H[1..nz] the height of each ring.
*/

#include "rowing_a.h"

#define WATER_F32(off) RA_F32(self, off)

/* the whirlpool's height at distance D from the centre (R +0x90a0, depth +0x90a4) */
static float ra_RingHeight(void *self, float d)
{
	float s;

	if (d < WATER_F32(0x90a0)) {
		s = sinf(d / WATER_F32(0x90a0) * 1.57079625f) - 1.0f;
		return s * WATER_F32(0x90a4) * s + WATER_F32(0x8);
	}
	return WATER_F32(0x8);
}

/* 0x001a5400  Rowing/CRowing_Water.cpp
Lays the grid out as a spiral: column i starts at angle 2 pi i / nx; going inwards ring by
ring the angle turns faster and faster (the turn rate grows inside 7/9 of the size); the
ring radius is (1 - u^2) * sizeX with u the distance from the rim. Stores each cell's
position (with the whirlpool height) and the cos/sin of its angle. */
void FUN_001a5400(void *self)
{
	CWaterField *w = (CWaterField *)self;
	float *pos, *cosA, *sinA;
	float angle, turn, turnRate, k, u, r, x, z, d, y, c, s;
	int col, row, i;

	RA_PTRF(self, 0x9090) = RA_NEW(w->numCells * 0xc, RA_FILE_WATER, 0x204);
	RA_PTRF(self, 0x9094) = RA_NEW(w->numCells << 2, RA_FILE_WATER, 0x20e);
	RA_PTRF(self, 0x9098) = RA_NEW(w->numCells << 2, RA_FILE_WATER, 0x20f);
	for (col = 0; col < w->p.nx; col++) {
		turn = 0.0f;
		turnRate = 0.0f;
		angle = ((float)col / (float)w->p.nx) * 6.28318501f;
		for (row = w->p.nz - 1; row >= 0; row--) {
			k = 128.0f / (float)w->p.nz;
			i = w->p.nx * row + col;
			angle = k * turn + angle;
			turn = k * turnRate + turn;
			s = sinf(angle);
			c = cosf(angle);
			u = 1.0f - (float)row / (float)w->p.nz;
			r = (1.0f - u * u) * w->p.sizeX;
			if (r < w->p.sizeX * 0.777777791f)
				turnRate = k * 5.99999985e-05f + turnRate;
			x = c * r;
			z = r * s;
			d = bmFastSqrt(x * x + z * z);
			y = ra_RingHeight(self, d);
			pos = (float *)RA_PTRF(self, 0x9090) + i * 3;
			pos[0] = x;
			pos[1] = y;
			pos[2] = z;
			cosA = (float *)RA_PTRF(self, 0x9094);
			sinA = (float *)RA_PTRF(self, 0x9098);
			cosA[i] = c;
			sinA[i] = s;
		}
	}
}

/* 0x001a5610  Rowing (gap; CRowing_Water.cpp inferred)
Sets the whirlpool's strength T (0..1): radius 800 + 1200 T, depth -600 - 500 T, and the
height of every ring from its first cell's distance. */
void FUN_001a5610(void *water, float t)
{
	void *self = water;
	CWaterField *w = (CWaterField *)water;
	float *p, d;
	int i;

	WATER_F32(0x90a0) = t * 1200.0f + 800.0f;
	WATER_F32(0x90a4) = -600.0f - t * 500.0f;
	for (i = 0; i < w->p.nz; i++) {
		p = (float *)RA_PTRF(self, 0x9090) + w->p.nx * i * 3;
		d = bmFastSqrt(p[2] * p[2] + p[0] * p[0]);
		d = bmFastSqrt(d * d);
		WATER_F32(0x90a4 + (w->p.nz - i) * 4) = ra_RingHeight(self, d);
	}
}

/* 0x001a56f0  Rowing (gap; CRowing_Water.cpp inferred) (vtable 0x00274f60 slot 1)
The whirlpool's Init: CWaterField's Init, radius 800, depth -600, the spiral, the ring
heights for strength 0 and the vertices (BuildVertices, slot 5). */
int FUN_001a56f0(void *self, CWaterParams *params)
{
	if (CWaterField_Init((CWaterField *)self, params) == 0)
		return 0;
	WATER_F32(0x909c) = 1.0f;
	WATER_F32(0x90a0) = 800.0f;
	WATER_F32(0x90a4) = -600.0f;
	FUN_001a5400(self);
	FUN_001a5610(self, 0.0f);
	RA_VFN(self, 0x14, int (*)(void *))(self);
	return 1;
}
