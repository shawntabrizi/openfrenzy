/*
ClassLib/CWaterField.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\CWaterField.cpp

A rippling water surface: a grid of cells (height and vertical speed) relaxed towards its
neighbours every tick, drawn as one indexed triangle strip with normals from a slope
table, plus four edge fans. Virtual functions (vtable 0x0026e05c):
  [0] deleting destructor  [1] Init  [2] Rebuild  [3] Ripple  [4] Scroll  [5] BuildVertices
  [6] UpdateVertices  [7] Draw  [8] DrawEdges  [9] RandomDrop  [10] Splash

0x001387d0 and 0x001389f0 are gap functions in front of the first CWaterField.cpp function
and only serve it (file inferred).
*/

#include "classlib_fx.h"
#include "d3d8_extra.h"

static const char s_CWaterFieldFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\CWaterField.cpp";

/* bdisplay (owners' prototypes) */
#define WF_CreateVB ((int (*)(int *, int, int, int, int))FUN_0003f500)
#define WF_LockVB ((byte *(*)(int, int))FUN_0003f710)
#define WF_CreateIB ((int (*)(int *, int))FUN_0003f1f0)
#define WF_IBData ((ushort *(*)(int))FUN_0003f290)
#define WF_DrawPrimitive ((void (*)(int, void *, int))FUN_0003f2d0)

/* 0x00302340 / 0x00302388  the two materials (D3DMATERIAL8) */
extern D3DMATERIAL8 DAT_00302340;
extern D3DMATERIAL8 DAT_00302388;

#define WF_CELL(self, i) ((self)->cells[(i)])

/* 0x001387d0  ClassLib (gap; file inferred: CWaterField.cpp)
Builds the index buffer: one strip that winds over the grid two rows at a time
(the original's spiral walk, kept literally). Indices past the last vertex become 0. */
int CWaterField_BuildIndices(CWaterField *self)
{
	ushort *base;
	ushort *w;
	ushort *w2;
	int cols;
	int rows;
	int rowsLeft;
	int col;
	int k;
	int left;
	short row;
	short s;

	WF_CreateIB(&self->ib, self->numIndices);
	base = WF_IBData(self->ib);
	cols = self->p.nx;
	col = 0;
	row = 0;
	if (self->p.wrap != 0)
		cols = cols + 1;
	rows = self->p.nz;
	rowsLeft = rows;
	w = base;
	for (; 0 < cols && (left = rowsLeft - 2, 0 < rows); rows = rows - 2) {
		if (0 < cols) {
			k = cols;
			do {
				if (col == self->p.nx) {
					w[0] = (short)self->p.nx * row;
					w[1] = (short)self->p.nx * (row + 1);
				} else {
					w[0] = (short)self->p.nx * row + (short)col;
					w[1] = (short)self->p.nx * (row + 1) + (short)col;
				}
				w = w + 2;
				col = col + 1;
				k = k - 1;
			} while (k != 0);
		}
		row = row + 1;
		if (1 < rows) {
			rowsLeft = rowsLeft - 1;
			do {
				if (col - 1 == self->p.nx) {
					w[0] = (short)self->p.nx * row;
					w[1] = (short)self->p.nx * row - 1;
				} else {
					w[0] = (short)self->p.nx * row + (short)(col - 1);
					w[1] = (short)col - 2 + (short)self->p.nx * row;
				}
				w = w + 2;
				row = row + 1;
				rowsLeft = rowsLeft - 1;
			} while (rowsLeft != 0);
		}
		col = col - 2;
		if (1 < cols) {
			rowsLeft = cols - 1;
			do {
				if (col == self->p.nx) {
					w[0] = (short)self->p.nx * (row - 1);
					w[1] = (short)self->p.nx * (row - 2);
				} else {
					w[0] = (short)self->p.nx * (row - 1) + (short)col;
					w[1] = (short)self->p.nx * (row - 2) + (short)col;
				}
				w = w + 2;
				col = col - 1;
				rowsLeft = rowsLeft - 1;
			} while (rowsLeft != 0);
		}
		row = row - 2;
		w2 = w;
		rowsLeft = left;
		if (2 < rows) {
			do {
				w = w2 + 2;
				w2[0] = (short)self->p.nx * row + (short)col + 1;
				s = (short)self->p.nx * row;
				row = row - 1;
				rowsLeft = rowsLeft - 1;
				w2[1] = (short)col + 2 + s;
				w2 = w;
			} while (rowsLeft != 0);
		}
		row = row + 1;
		cols = cols - 2;
		col = col + 2;
		rowsLeft = left;
	}
	for (w = base; w < base + self->numIndices; w++) {
		if ((uint)self->numVerts <= (uint)*w)
			*w = 0;
	}
	FUN_001d5950(); /* an empty debug function (the index buffer) */
	return 1;
}

/* 0x001389f0  ClassLib (gap; file inferred: CWaterField.cpp)
A local copy of the vector constructor iterator: constructs `count` elements of `size`
bytes (the constructor is a thiscall taking the element). */
void CWaterField_ConstructArray(void *mem, int size, int count, void *ctor)
{
	byte *p;

	p = mem;
	for (; 0 < count; count--) {
		((void *(*)(void *))ctor)(p);
		p = p + size;
	}
}

/* the vector constructor the normal table uses (0x00206980, folded into the DSOUND
section: "mov eax, ecx; ret") */
static void *CWaterField_Vec3Ctor(void *self)
{
	return self;
}

/* 0x00138a20  ClassLib/CWaterField.cpp
Ripple (vtable slot 3): each water cell's speed relaxes towards the weighted mean of its 8
neighbours (edges wrap for flags 2 and 3; fixed cells are held at 0), then heights move. */
void CWaterField_Ripple(CWaterField *self)
{
	float *c;
	float diag;
	float adj;
	byte *flag;
	int nx;
	int i;

	c = &self->cells[0][0];
	flag = self->flags;
	for (i = self->numCells; i > 0; i--) {
		nx = self->p.nx;
		switch (*flag) {
		case 0:
			diag = (c[nx * 2 - 2] + c[nx * 2 + 2] + c[-nx * 2 - 2] + c[-nx * 2 + 2]) * 0.0666666701f;
			adj = (c[-2] + c[2] + c[nx * 2] + c[-nx * 2]) * 0.13333334f;
			c[1] = self->p.damping * c[1] + ((adj + diag) - c[0]) * self->p.tension;
			break;
		case 1:
			c[0] = 0.0f;
			c[1] = 0.0f;
			break;
		case 2:
			diag = (c[nx * 4 - 2] + c[-2] + c[nx * 2 + 2] + c[-nx * 2 + 2]) * 0.0833333358f;
			adj = (c[nx * 2 - 2] + c[2] + c[nx * 2] + c[-nx * 2]) * 0.166666672f;
			c[1] = self->p.damping * c[1] + ((adj + diag) - c[0]) * self->p.tension;
			break;
		case 3:
			diag = (c[-nx * 4 + 2] + c[nx * 2 - 2] + c[-nx * 2 - 2] + c[2]) * 0.0833333358f;
			adj = (c[-2] + c[-nx * 2 + 2] + c[nx * 2] + c[-nx * 2]) * 0.166666672f;
			c[1] = self->p.damping * c[1] + ((adj + diag) - c[0]) * self->p.tension;
			break;
		default:
			break;
		}
		flag = flag + 1;
		c = c + 2;
	}
	c = &self->cells[0][0];
	for (i = self->numCells; i > 0; i--) {
		c[0] = c[1] + c[0];
		c = c + 2;
	}
}

/* 0x00138b80  ClassLib/CWaterField.cpp
Scroll (vtable slot 4): moves the waves `dist` towards -z (at most 6 cells), mixing the
two rows the distance falls between. */
void CWaterField_Scroll(CWaterField *self, float dist)
{
	float t;
	float frac;
	float inv;
	int whole;
	int off0;
	int off1;
	int i;
	int count;
	float *c;
	byte *flag;

	t = dist / self->cellZ;
	c = &self->cells[0][0];
	flag = self->flags;
	if (6.0f < t)
		t = 6.0f;
	whole = CLF_FTOL(t);
	off0 = self->p.nx * whole;
	frac = t - (float)whole;
	inv = 1.0f - frac;
	off1 = (whole + 1) * self->p.nx;
	count = self->numCells - off1;
	for (i = 0; i < count; i++) {
		if (*flag == 0) {
			c[1] = inv * c[off0 * 2 + 1] + frac * c[off1 * 2 + 1];
			c[0] = inv * c[off0 * 2] + frac * c[off1 * 2];
		}
		flag = flag + 1;
		c = c + 2;
	}
}

/* 0x00138c40  ClassLib/CWaterField.cpp
Rebuild (vtable slot 2): calls BuildVertices (slot 5). */
void CWaterField_Rebuild(CWaterField *self)
{
	((int (*)(CWaterField *))self->vtable[5])(self);
}

/* 0x00138c50  ClassLib/CWaterField.cpp
Scrolls the texture, keeping both offsets in [0, 1). */
void CWaterField_ScrollUV(CWaterField *self, float du, float dv)
{
	float u;
	float v;

	u = du + self->uOffset;
	self->uOffset = u;
	v = dv + self->vOffset;
	self->vOffset = v;
	if (1.0f <= v) {
		v = self->vOffset;
		do {
			v = v - 1.0f;
		} while (1.0f <= v);
		self->vOffset = v;
	}
	if (self->vOffset < 0.0f) {
		v = self->vOffset;
		do {
			v = v + 1.0f;
		} while (v < 0.0f);
		self->vOffset = v;
	}
	if (1.0f <= u) {
		u = self->uOffset;
		do {
			u = u - 1.0f;
		} while (1.0f <= u);
		self->uOffset = u;
	}
	if (self->uOffset < 0.0f) {
		u = self->uOffset;
		do {
			u = u + 1.0f;
		} while (u < 0.0f);
		self->uOffset = u;
	}
}

/* 0x00138d40  ClassLib/CWaterField.cpp
Draws a flat fan of `count` points (xs, zs relative to the corner) at the surface height. */
void CWaterField_DrawFan(CWaterField *self, float *xs, float *zs, int count, CWaterVertex *v)
{
	int i;

	for (i = 0; i < count; i++) {
		v[i].pos[0] = self->p.x0 + xs[i];
		v[i].pos[2] = zs[i] + self->p.z0;
		v[i].pos[1] = self->p.y;
		v[i].colour = self->p.colour;
		v[i].normal[0] = 0.0f;
		v[i].normal[1] = 1.0f;
		v[i].normal[2] = 0.0f;
		v[i].uv[0] = self->uvPerUnit * xs[i] + self->uOffset;
		v[i].uv[1] = zs[i] * self->uvPerUnit + self->vOffset;
	}
	WF_DrawPrimitive(4, v, count);
}

/* 0x00138de0  ClassLib/CWaterField.cpp
Splash (vtable slot 10): pushes the surface down to `depth` inside `inner` radius of
(x, z), easing to nothing at `outer` (only where it is deeper than the water now).
Returns 0 when the field has no cells. */
int CWaterField_Splash(CWaterField *self, float x, float z, float depth, float inner, float outer)
{
	float inv;
	float cx;
	float cz;
	float rIn;
	float rOut;
	float d2;
	float f;
	float *cell;
	int x0;
	int x1;
	int z0;
	int z1;
	int ix;
	int iz;

	if (self->cells == NULL)
		return 0;
	inv = 1.0f / self->p.sizeX;
	cx = (x - self->p.x0) * (float)self->p.nx * inv;
	cz = ((z - self->p.z0) / self->p.sizeZ) * (float)self->p.nz;
	rIn = (float)self->p.nx * inv * inner;
	rOut = (float)self->p.nx * inv * outer;
	x0 = CLF_FTOL(cx - rOut);
	x1 = CLF_FTOL(rOut + cx + 1.0f);
	z0 = CLF_FTOL(cz - rOut);
	z1 = CLF_FTOL(rOut + cz + 1.0f);
	if (x0 < 1)
		x0 = 1;
	if (z0 < 1)
		z0 = 1;
	if (self->p.nx - 2 < x1)
		x1 = self->p.nx - 2;
	if (self->p.nz - 2 < z1)
		z1 = self->p.nz - 2;
	for (ix = x0; ix <= x1; ix++) {
		for (iz = z0; iz <= z1; iz++) {
			d2 = ((float)iz - cz) * ((float)iz - cz) + ((float)ix - cx) * ((float)ix - cx);
			if (d2 < rOut * rOut) {
				if (rIn * rIn <= d2)
					f = (1.0f - (FUN_00050f80(d2) - rIn) / (rOut - rIn)) * depth;
				else
					f = depth;
				cell = self->cells[self->p.nx * iz + ix];
				if (f < cell[0]) {
					cell[0] = f;
					cell[1] = 0.0f;
				}
			}
		}
	}
	return 1;
}

/* 0x00138ff0  ClassLib/CWaterField.cpp
Destructor: frees the cells, buffers, flags and normal table. */
void CWaterField_Destruct(CWaterField *self)
{
	self->vtable = vtable_0026e05c;
	if (self->cells != NULL) {
		CLF_Free(self->cells);
		self->cells = NULL;
	}
	if (self->ib != 0) {
		FUN_0003f270((void *)self->ib);
		self->ib = 0;
	}
	if (self->vb != 0) {
		FUN_0003f650((void *)self->vb);
		self->vb = 0;
	}
	if (self->flags != NULL) {
		CLF_Free(self->flags);
		self->flags = NULL;
	}
	if (self->normals != NULL) {
		CLF_Free(self->normals);
		self->normals = NULL;
	}
}

/* 0x00139080  ClassLib/CWaterField.cpp
Builds the 32 x 32 table of normals by slope (once): x and z from -16 to 15 steps of
0.04375, y = sqrt(1 - x^2 - z^2). */
void CWaterField_BuildNormals(CWaterField *self)
{
	float (*table)[3];
	float x;
	float r;
	float z;
	int i;
	int j;

	if (self->normals != NULL)
		return;
	table = CLF_Alloc(0x3000, s_CWaterFieldFile, 0x17c, 0x1001);
	if (table != NULL)
		CWaterField_ConstructArray(table, 0xc, 0x400, (void *)CWaterField_Vec3Ctor);
	self->normals = table;
	for (i = 0; i < 0x20; i++) {
		x = (float)(i - 0x10) * 0.0437499993f;
		r = 1.0f - x * x;
		for (j = 0; j < 0x20; j++) {
			self->normals[j * 0x20 + i][0] = x;
			z = (float)(j - 0x10) * 0.0437499993f;
			self->normals[j * 0x20 + i][2] = z;
			self->normals[j * 0x20 + i][1] = sqrtf(r - z * z);
		}
	}
}

/* 0x00139170  ClassLib/CWaterField.cpp
Sets the water material: diffuse and specular from ARGB colours, white ambient, the
specular power, and specular lighting on or off (render state 0x5d). */
void CWaterField_SetMaterial(uint diffuse, uint specular, float power, int specularOn)
{
	memset(&DAT_00302340, 0, 0x11 * 4);
	DAT_00302340.Ambient.r = 1.0f;
	DAT_00302340.Ambient.g = 1.0f;
	DAT_00302340.Ambient.b = 1.0f;
	DAT_00302340.Ambient.a = 1.0f;
	DAT_00302340.Specular.a = (float)(specular >> 0x18) * 0.00392156886f;
	DAT_00302340.Specular.r = (float)(specular >> 0x10 & 0xff) * 0.00392156886f;
	DAT_00302340.Specular.g = (float)(specular >> 8 & 0xff) * 0.00392156886f;
	DAT_00302340.Specular.b = (float)(specular & 0xff) * 0.00392156886f;
	DAT_00302340.Diffuse.a = (float)(diffuse >> 0x18) * 0.00392156886f;
	DAT_00302340.Diffuse.r = (float)(diffuse >> 0x10 & 0xff) * 0.00392156886f;
	DAT_00302340.Diffuse.g = (float)(diffuse >> 8 & 0xff) * 0.00392156886f;
	DAT_00302340.Diffuse.b = (float)(diffuse & 0xff) * 0.00392156886f;
	DAT_00302340.Emissive.r = 0.0f;
	DAT_00302340.Emissive.g = 0.0f;
	DAT_00302340.Emissive.b = 0.0f;
	DAT_00302340.Emissive.a = 1.0f;
	DAT_00302340.Power = power;
	D3DDevice_SetMaterial(&DAT_00302340);
	if (specularOn != 0) {
		D3DDevice_SetRenderState_Deferred((D3DRENDERSTATETYPE)0x5d, 1);
		return;
	}
	D3DDevice_SetRenderState_Deferred((D3DRENDERSTATETYPE)0x5d, 0);
}

/* 0x00139340  ClassLib/CWaterField.cpp
Sets a plain white material (power 8) without specular lighting. */
void CWaterField_ResetMaterial(void)
{
	memset(&DAT_00302388, 0, 0x11 * 4);
	DAT_00302388.Ambient.r = 1.0f;
	DAT_00302388.Ambient.g = 1.0f;
	DAT_00302388.Ambient.b = 1.0f;
	DAT_00302388.Ambient.a = 1.0f;
	DAT_00302388.Diffuse.r = 1.0f;
	DAT_00302388.Diffuse.g = 1.0f;
	DAT_00302388.Diffuse.b = 1.0f;
	DAT_00302388.Diffuse.a = 1.0f;
	DAT_00302388.Specular.r = 1.0f;
	DAT_00302388.Specular.g = 1.0f;
	DAT_00302388.Specular.b = 1.0f;
	DAT_00302388.Specular.a = 1.0f;
	DAT_00302388.Emissive.r = 0.0f;
	DAT_00302388.Emissive.g = 0.0f;
	DAT_00302388.Emissive.b = 0.0f;
	DAT_00302388.Emissive.a = 0.0f;
	DAT_00302388.Power = 8.0f;
	D3DDevice_SetMaterial(&DAT_00302388);
	D3DDevice_SetRenderState_Deferred((D3DRENDERSTATETYPE)0x5d, 0);
}

/* the slope table index (0..31) of a height difference */
static int CWaterField_SlopeIndex(float slope, float scale)
{
	int i;

	i = CLF_FTOL(slope * scale) + 0x10;
	if (i < 0)
		i = 0;
	else if (0x1f < i)
		i = 0x1f;
	return i;
}

/* 0x00139400  ClassLib/CWaterField.cpp
UpdateVertices (vtable slot 6): heights, normals and texture offsets of the water cells
(the slope index uses slopeUV here, as in the original). */
int CWaterField_UpdateVertices(CWaterField *self)
{
	CWaterVertex *v;
	float *c;
	byte *flag;
	float scale;
	float u;
	float vv;
	float dx;
	float dz;
	int nx;
	int ix;
	int iz;
	int row;
	int col;

	scale = self->p.slopeUV;
	nx = self->p.nx;
	flag = self->flags;
	v = (CWaterVertex *)WF_LockVB(self->vb, 0);
	vv = self->vOffset;
	c = &self->cells[0][0];
	for (row = self->p.nz; row != 0; row--) {
		u = self->uOffset;
		for (col = nx; col != 0; col--) {
			if (*flag == 0) {
				v->pos[1] = self->p.heightScale * c[0] + self->p.y;
				dx = c[2] - c[-2];
				ix = CWaterField_SlopeIndex(dx, scale);
				v->uv[0] = dx * self->p.slopeUV + u;
				dz = (c[nx * 2] - c[-nx * 2]) * scale;
				iz = CLF_FTOL(dz) + 0x10;
				if (iz < 0)
					iz = 0;
				else if (0x1f < iz)
					iz = 0x1f;
				ix = iz * 0x20 + ix;
				v->uv[1] = dz + vv;
				v->normal[0] = self->normals[ix][0];
				v->normal[1] = self->normals[ix][1];
				v->normal[2] = self->normals[ix][2];
			}
			u = u + self->uvStep;
			v = v + 1;
			c = c + 2;
			flag = flag + 1;
		}
		vv = vv + self->uvStep;
	}
	FUN_0003f790((void *)self->vb);
	return 1;
}

/* 0x00139590  ClassLib/CWaterField.cpp
BuildVertices (vtable slot 5): every vertex: position, colour, normal and texture
coordinates (edge and fixed cells get an upward normal and plain coordinates). */
int CWaterField_BuildVertices(CWaterField *self)
{
	CWaterVertex *v;
	float *c;
	byte *flag;
	float stepX;
	float stepZ;
	float x;
	float z;
	float u;
	float vv;
	float d;
	int ix;
	int iz;
	int row;
	int col;

	flag = self->flags;
	v = (CWaterVertex *)WF_LockVB(self->vb, 4);
	stepX = self->cellX;
	z = self->p.z0;
	c = &self->cells[0][0];
	stepZ = self->cellZ;
	vv = self->vOffset;
	for (row = self->p.nz; row != 0; row--) {
		u = self->uOffset;
		x = self->p.x0;
		for (col = self->p.nx; col != 0; col--) {
			v->pos[0] = x;
			v->pos[2] = z;
			v->pos[1] = self->p.heightScale * c[0] + self->p.y;
			if (*flag == 0) {
				v->colour = self->p.colour;
				d = c[2] - c[-2];
				ix = CWaterField_SlopeIndex(d, self->p.normalScale);
				v->uv[0] = d * self->p.slopeUV + u;
				d = c[self->p.nx * 2] - c[self->p.nx * -2];
				iz = CWaterField_SlopeIndex(d, self->p.normalScale);
				ix = (iz * 0x20 + ix);
				v->uv[1] = d * self->p.slopeUV + vv;
				v->normal[0] = self->normals[ix][0];
				v->normal[1] = self->normals[ix][1];
				v->normal[2] = self->normals[ix][2];
			} else {
				v->colour = self->p.colour;
				v->normal[0] = 0.0f;
				v->normal[2] = 0.0f;
				v->normal[1] = 1.0f;
				v->uv[0] = u;
				v->uv[1] = vv;
			}
			x = x + stepX;
			v = v + 1;
			c = c + 2;
			flag = flag + 1;
			u = u + self->uvStep;
		}
		vv = vv + self->uvStep;
		z = z + stepZ;
	}
	FUN_0003f790((void *)self->vb);
	return 1;
}

/* 0x00139790  ClassLib/CWaterField.cpp
Draw (vtable slot 7): the strip, optionally blended (`blend`: bdisplay state 2 = 3 and
render state 0x58 = 0x1010 while drawing). */
int CWaterField_Draw(CWaterField *self, int blend)
{
	FUN_000416c0(0, (void *)self->texture);
	if (blend != 0) {
		FUN_00044b00(2, 3, 0);
		D3DDevice_SetRenderState_Deferred((D3DRENDERSTATETYPE)0x58, 0x1010);
	}
	FUN_0003efa0(0x152, (void *)self->vb);
	FUN_0003f2a0((void *)self->ib, 0);
	D3DDevice_DrawIndexedVertices(D3DPT_TRIANGLESTRIP, self->numIndices, D3D__IndexData);
	DAT_002bc48c = DAT_002bc48c + self->p.nz * self->p.nx * 2;
	if (blend != 0)
		D3DDevice_SetRenderState_Deferred((D3DRENDERSTATETYPE)0x58, 0);
	D3DDevice_SetStreamSource(0, NULL, 0x24);
	D3DDevice_SetIndices(NULL, 0);
	D3DDevice_InsertFence();
	return 1;
}

/* 0x00139850  ClassLib/CWaterField.cpp
DrawEdges (vtable slot 8): four fans that extend the surface `height` beyond each side. */
int CWaterField_DrawEdges(CWaterField *self, float height, int blend)
{
	float xs[5];
	float zs[5];
	float a;
	float b;
	float sx;
	float sz;

	if (blend != 0) {
		FUN_00044b00(2, 3, 0);
		D3DDevice_SetRenderState_Deferred((D3DRENDERSTATETYPE)0x58, 0x1010);
	}
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)10, 1);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
	sx = self->p.sizeX;
	sz = self->p.sizeZ;
	a = height * -0.707000017f;
	b = height * 0.707000017f;
	xs[0] = 0.0f;
	xs[1] = -height;
	xs[2] = 0.0f;
	xs[3] = a;
	xs[4] = sx * 0.5f;
	zs[0] = 0.0f;
	zs[1] = sz * 0.5f;
	zs[2] = sz;
	zs[3] = b + sz;
	zs[4] = height + sz;
	CWaterField_DrawFan(self, xs, zs, 5, self->edges[0]);
	xs[0] = 0.0f;
	xs[1] = sx * 0.5f;
	xs[2] = sx;
	xs[3] = b + sx;
	xs[4] = height + sx;
	zs[0] = sz;
	zs[1] = height + sz;
	zs[2] = sz;
	zs[3] = b + sz;
	zs[4] = sz * 0.5f;
	CWaterField_DrawFan(self, xs, zs, 5, self->edges[1]);
	xs[0] = sx;
	xs[1] = height + sx;
	xs[2] = sx;
	xs[3] = b + sx;
	xs[4] = sx * 0.5f;
	zs[0] = sz;
	zs[1] = sz * 0.5f;
	zs[2] = 0.0f;
	zs[3] = a;
	zs[4] = -height;
	CWaterField_DrawFan(self, xs, zs, 5, self->edges[2]);
	xs[0] = sx;
	xs[1] = sx * 0.5f;
	xs[2] = 0.0f;
	xs[3] = a;
	xs[4] = -height;
	zs[0] = 0.0f;
	zs[1] = -height;
	zs[2] = 0.0f;
	zs[3] = a;
	zs[4] = sz * 0.5f;
	CWaterField_DrawFan(self, xs, zs, 5, self->edges[3]);
	if (blend != 0)
		D3DDevice_SetRenderState_Deferred((D3DRENDERSTATETYPE)0x58, 0);
	return 1;
}

/* 0x00139ab0  ClassLib/CWaterField.cpp
RandomDrop (vtable slot 9): lowers the speed of a random inner water cell by `depth`.
Returns 0 when the cell is not water. */
int CWaterField_RandomDrop(CWaterField *self, float depth)
{
	int r1;
	int r2;
	int i;

	r1 = CLF_Rand();
	r2 = CLF_Rand();
	i = ((int)((longlong)r2 % (longlong)(self->p.nz - 2)) + 1) * self->p.nx +
	    (int)((longlong)r1 % (longlong)(self->p.nx - 2)) + 1;
	if (self->flags[i] != 0)
		return 0;
	self->cells[i][1] = self->cells[i][1] - depth;
	return 1;
}

/* 0x00139b40  ClassLib/CWaterField.cpp
Deleting destructor (vtable slot 0). */
CWaterField *CWaterField_DeletingDtor(CWaterField *self, byte flags)
{
	CWaterField_Destruct(self);
	if ((flags & 1) != 0)
		CLF_Free(self);
	return self;
}

/* 0x00139b60  ClassLib/CWaterField.cpp
Init (vtable slot 1): parameters (or the defaults at 0x002ae578), even cell counts, the
cell and flag arrays (the border is fixed, or wraps left/right), texture, indices, vertex
buffer and normal table. Returns 0 when out of memory. */
int CWaterField_Init(CWaterField *self, CWaterParams *params)
{
	int nx;
	int nz;
	int n;
	int i;
	float cellX;
	float sizeZ;
	float step;
	byte *mem;

	if (params == NULL)
		params = &DAT_002ae578;
	memcpy(&self->p, params, sizeof(CWaterParams));
	nx = self->p.nx + 1U & 0xfffffffe;
	nz = self->p.nz + 1U & 0xfffffffe;
	self->p.nx = nx;
	self->p.nz = nz;
	self->uOffset = 0.0f;
	self->vOffset = 0.0f;
	self->unknown9064 = 0;
	sizeZ = ((float)nz * self->p.sizeX) / (float)nx;
	self->unknown9068 = 0;
	self->p.sizeZ = sizeZ;
	n = nz * nx;
	cellX = self->p.sizeX / (float)(nx - 1);
	self->numCells = n;
	self->numVerts = n;
	self->cellX = cellX;
	self->cellZ = sizeZ / (float)(nz - 1);
	step = self->p.uvScale / (float)nx;
	self->uvStep = step;
	self->unknown9060 = step;
	self->unknown906c = step;
	self->uvPerUnit = step / cellX;
	if (self->p.wrap == 0)
		self->numIndices = n * 2;
	else
		self->numIndices = (nz + n) * 2;
	self->cells = CLF_Alloc(n * 8, s_CWaterFieldFile, 0x87, 0x1001);
	if (self->cells == NULL)
		return 0;
	memset(self->cells, 0, (size_t)self->numCells * 8);
	mem = CLF_Alloc(self->numCells, s_CWaterFieldFile, 0x8c, 0x1001);
	self->flags = mem;
	if (mem == NULL)
		return 0;
	memset(mem, 0, (size_t)self->numCells);
	for (i = 0; i < self->p.nx; i++) {
		self->flags[i] = 1;
		self->flags[(i - self->p.nx) + self->numCells] = 1;
	}
	if (self->p.wrap == 0) {
		for (i = 0; i < self->p.nz; i++) {
			self->flags[self->p.nx * i] = 1;
			self->flags[self->p.nx * (i + 1) - 1] = 1;
		}
	} else {
		for (i = 1; i < self->p.nz - 1; i++) {
			self->flags[self->p.nx * i] = 2;
			self->flags[self->p.nx * (i + 1) - 1] = 3;
		}
	}
	self->texture = CLF_FindTexture(0, self->p.textureName, 0, 0xdefa);
	CWaterField_BuildIndices(self);
	if (self->vb == 0) {
		if (WF_CreateVB(&self->vb, self->numVerts, 0x152, 0, 1) == 0)
			return 0;
		mem = WF_LockVB(self->vb, 4);
		memset(mem, 0, (size_t)(self->numVerts * 9 & 0x3fffffff) * 4);
		FUN_0003f790((void *)self->vb);
	}
	CWaterField_BuildNormals(self);
	return 1;
}

/* 0x0026e05c  the CWaterField vtable */
const void *const vtable_0026e05c[11] = {
	(const void *)CWaterField_DeletingDtor, (const void *)CWaterField_Init,
	(const void *)CWaterField_Rebuild,      (const void *)CWaterField_Ripple,
	(const void *)CWaterField_Scroll,       (const void *)CWaterField_BuildVertices,
	(const void *)CWaterField_UpdateVertices, (const void *)CWaterField_Draw,
	(const void *)CWaterField_DrawEdges,    (const void *)CWaterField_RandomDrop,
	(const void *)CWaterField_Splash,
};
