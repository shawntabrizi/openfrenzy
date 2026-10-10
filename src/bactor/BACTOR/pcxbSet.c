/*
BACTOR/pcxbSet.c  <-  C:\Babel\Xbox\Src\BACTOR\pcxbSet.cpp

Sets: static geometry split into a grid of cells; a cell is drawn when not
all of its eight corners are outside the same clip plane.

0x0003e0b0 is tagged with pcxbSet.cpp; 0x0003e220 and 0x0003e250 are in
its handler tables.
*/

#include <string.h>

#include "bactor.h"
#include "bactor_extern.h"

/* 0x002bc3f0: percentage of the cells drawn by the last bSetDraw */
static int s_cellsDrawnPercent;

/* 0x0003e0b0  BACTOR/pcxbSet.cpp
fixes up a loaded set: cells, textures, vertex buffer, outcode buffer */
void bSetInit(struct BSet *set)
{
	struct BSetCell *cell;
	struct BSetPrim *prim;
	int numCells = set->cellsY * set->cellsZ * set->cellsX;
	int lastTex0 = 0;
	int lastTex1 = 0;
	void *tex0 = NULL;
	void *tex1 = NULL;
	void *vb;
	void *data;
	int numVerts;
	int i;

	cell = (struct BSetCell *)((char *)set + 0x70);
	prim = (struct BSetPrim *)(cell + numCells);
	set->cells = cell;
	for (; numCells > 0; numCells--, cell++)
	{
		cell->prims = prim;
		for (i = 0; i < cell->numPrims; i++, prim++)
		{
			if ((int)(intptr_t)prim->texture0 != lastTex0)
			{
				tex0 = ba_bkFindTexture(0, (int)(intptr_t)prim->texture0, set->handle, set->package);
				lastTex0 = (int)(intptr_t)prim->texture0;
			}
			prim->texture0 = tex0;
			if ((int)(intptr_t)prim->texture1 != lastTex1)
			{
				tex1 = ba_bkFindTexture(0, (int)(intptr_t)prim->texture1, set->handle, set->package);
				lastTex1 = (int)(intptr_t)prim->texture1;
			}
			prim->texture1 = tex1;
		}
	}
	/* the vertices follow the primitives */
	numVerts = set->numVerts;
	vb = NULL;
	if (ba_bdCreateVertexBuffer(&vb, numVerts, 0x152, 0x10, 1) == 0)
	{
		vb = NULL;
	}
	else if ((data = ba_bdLockVertexBuffer(vb, 0)) != NULL)
	{
		memcpy(data, prim, (size_t)(numVerts * 9 & 0x3fffffff) * 4);
		ba_bdUnlockVertexBuffer(vb);
	}
	set->vertexBuffer = vb;
	set->outcodes = ba_bkAlloc((set->cellsY + 1) * (set->cellsX + 1) * (set->cellsZ + 1) * 4,
		"C:\\Babel\\Xbox\\Src\\BACTOR\\pcxbSet.cpp", 0xb5, 0x2002);
}

/* 0x0003e220  BACTOR/pcxbSet.cpp */
void bSetFree(struct BSet *set)
{
	ba_bdReleaseVertexBuffer(set->vertexBuffer);
	set->vertexBuffer = NULL;
	ba_bkFree(set->outcodes);
	set->outcodes = NULL;
}

/* the clip outcode of a point in clip space: bits 0/1 x outside -w/+w,
2/3 y, 4 z <= 0, 5 z >= w */
static uint32_t bOutcode(const float *p)
{
	uint32_t code = p[0] <= -p[3];

	if (p[3] <= p[0])
		code |= 2;
	if (p[1] <= -p[3])
		code |= 4;
	if (p[3] <= p[1])
		code |= 8;
	if (p[2] <= 0.0f)
		code |= 0x10;
	if (p[3] <= p[2])
		code |= 0x20;
	return code;
}

/* 0x0003e250  BACTOR/pcxbSet.cpp
draws one pass of a set: 1 opaque (computes the cell outcodes first),
2 translucent, 4 additive */
void bSetDraw(struct BSet *set, int pass)
{
	float v[4];
	float c000[4], c001[4], c011[4], c010[4];
	float c100[4], c101[4], c111[4], c110[4];
	float dx000[4], dx010[4], dx001[4], dx011[4];
	float dy0[4], dy1[4], dz[4];
	float p[4], q[4], r[4];
	float sx, sy, sz;
	uint32_t *outcode;
	struct BSetCell *cell;
	struct BSetPrim *prim;
	uint32_t lastU = 0xffffffff;
	uint32_t lastV = 0xffffffff;
	void *lastTex0 = (void *)1;
	void *lastTex1 = (void *)1;
	int vertex = 0;
	int count = 0;
	int drawn = 0;
	int plane;
	int x, y, z, k, n;

	if (pass == 1)
	{
		/* the corners of the bounds in clip space */
		v[0] = set->bounds[0];
		v[1] = set->bounds[2];
		v[2] = set->bounds[4];
		v[3] = 1.0f;
		ba_MatTransform(c000, DAT_002be470, v);
		v[2] = set->bounds[5];
		ba_MatTransform(c001, DAT_002be470, v);
		v[1] = set->bounds[3];
		ba_MatTransform(c011, DAT_002be470, v);
		v[2] = set->bounds[4];
		ba_MatTransform(c010, DAT_002be470, v);
		v[0] = set->bounds[1];
		v[1] = set->bounds[2];
		v[2] = set->bounds[4];
		ba_MatTransform(c100, DAT_002be470, v);
		v[2] = set->bounds[5];
		ba_MatTransform(c101, DAT_002be470, v);
		v[1] = set->bounds[3];
		ba_MatTransform(c111, DAT_002be470, v);
		v[2] = set->bounds[4];
		ba_MatTransform(c110, DAT_002be470, v);
		sx = 1.0f / (float)set->cellsX;
		sy = 1.0f / (float)set->cellsY;
		sz = 1.0f / (float)set->cellsZ;
		for (k = 0; k < 4; k++)
		{
			dx000[k] = (c100[k] - c000[k]) * sx;
			dx010[k] = (c110[k] - c010[k]) * sx;
			dx001[k] = (c101[k] - c001[k]) * sx;
			dx011[k] = (c111[k] - c011[k]) * sx;
		}
		/* the outcodes of the (cellsX + 1) * (cellsY + 1) * (cellsZ + 1) grid points */
		outcode = set->outcodes;
		for (x = 0; x <= set->cellsX; x++)
		{
			for (k = 0; k < 4; k++)
			{
				dy0[k] = (c010[k] - c000[k]) * sy;
				dy1[k] = (c011[k] - c001[k]) * sy;
				p[k] = c000[k];
				q[k] = c001[k];
			}
			for (y = 0; y <= set->cellsY; y++)
			{
				for (k = 0; k < 4; k++)
				{
					dz[k] = (q[k] - p[k]) * sz;
					r[k] = p[k];
				}
				for (z = 0; z <= set->cellsZ; z++)
				{
					*outcode++ = bOutcode(r);
					for (k = 0; k < 4; k++)
						r[k] = dz[k] + r[k];
				}
				for (k = 0; k < 4; k++)
				{
					p[k] = p[k] + dy0[k];
					q[k] = q[k] + dy1[k];
				}
			}
			for (k = 0; k < 4; k++)
			{
				c000[k] = dx000[k] + c000[k];
				c010[k] = dx010[k] + c010[k];
				c001[k] = dx001[k] + c001[k];
				c011[k] = dx011[k] + c011[k];
			}
		}
	}
	D3DDevice_SetTextureState_Deferred(0, D3DTSS_MIPFILTER, ((set->flags & 1) != 0) + 1);
	ba_bdSetVertexShader(0x152, set->vertexBuffer);
	if (DAT_00284b0c != 0x100)
	{
		D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLOROP, 2);
		D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLORARG1, 1);
		D3DDevice_SetTextureState_Deferred(1, D3DTSS_ALPHAOP, 4);
		D3DDevice_SetTextureState_Deferred(1, D3DTSS_ALPHAARG1, 1);
		D3DDevice_SetTextureState_Deferred(1, D3DTSS_ALPHAARG2, 3);
	}
	if (pass == 1)
	{
		if (DAT_002be630 != 0 && DAT_00284b0c == 0x100)
			D3DDevice_SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	}
	else if (pass == 4 && DAT_002be630 == 0)
	{
		D3DDevice_SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	}
	plane = (set->cellsZ + 1) * (set->cellsY + 1);
	outcode = set->outcodes;
	cell = set->cells;
	prim = NULL;
	for (x = 0; x < set->cellsX; x++)
	{
		for (y = 0; y < set->cellsY; y++)
		{
			for (z = 0; z < set->cellsZ; z++, cell++, outcode++)
			{
				/* drawn unless the eight corners share an outside plane */
				if ((outcode[0] & outcode[set->cellsZ + plane + 2] & outcode[set->cellsZ + plane + 1] &
					outcode[set->cellsZ + 2] & outcode[set->cellsZ + 1] & outcode[plane + 1] & outcode[plane] &
					outcode[1]) != 0)
					continue;
				drawn++;
				if (pass == 1)
				{
					count = cell->numPrimsPass[0];
					vertex = cell->firstVertex0;
					prim = cell->prims;
				}
				else if (pass == 2)
				{
					vertex = cell->firstVertex1;
					prim = cell->prims + cell->numPrimsPass[0];
					count = cell->numPrimsPass[1];
				}
				else if (pass == 4)
				{
					count = cell->numPrimsPass[2];
					vertex = cell->firstVertex2;
					prim = cell->prims + cell->numPrimsPass[1] + cell->numPrimsPass[0];
					D3DDevice_SetRenderState(D3DRS_ZWRITEENABLE, 0);
				}
				for (n = count; n > 0; n--, prim++)
				{
					if (prim->texture0 != lastTex0)
					{
						ba_bdSetTexture(0, prim->texture0);
						lastTex0 = prim->texture0;
					}
					if (prim->texture1 != lastTex1)
					{
						ba_bdSetTexture(1, prim->texture1);
						lastTex1 = prim->texture1;
					}
					if (DAT_002bee34[0] != 0)
					{
						if ((uint32_t)(prim->flags & 1) != lastU)
						{
							lastU = prim->flags & 1;
							D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSU, (prim->flags & 1) == 0 ? 3 : 1);
						}
						if ((uint32_t)(prim->flags & 2) != lastV)
						{
							lastV = prim->flags & 2;
							D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSV, (prim->flags & 2) == 0 ? 3 : 1);
						}
					}
					D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex,
						D3DPRIMITIVETOVERTEXCOUNT[prim->type][0] * prim->numPrims +
							D3DPRIMITIVETOVERTEXCOUNT[prim->type][1]);
					vertex += prim->numVerts;
				}
				if (pass == 4)
					D3DDevice_SetRenderState(D3DRS_ZWRITEENABLE, 1);
			}
			outcode++;
		}
		outcode += set->cellsZ + 1;
	}
	if (pass == 1)
	{
		if (DAT_002be630 != 0 && DAT_00284b0c == 0x100)
			D3DDevice_SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	}
	else if (pass == 4 && DAT_002be630 == 0)
	{
		D3DDevice_SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	}
	s_cellsDrawnPercent = drawn * 100 / (set->cellsZ * set->cellsX * set->cellsY);
	if (DAT_00284b0c != 0x100)
	{
		D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLOROP, 1);
		D3DDevice_SetTextureState_Deferred(1, D3DTSS_ALPHAOP, 1);
	}
}
