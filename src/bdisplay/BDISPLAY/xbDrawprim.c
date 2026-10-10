/*
XBDRAWPRIM.C  <-  C:\Babel\Xbox\Src\BDISPLAY\xbDrawprim.cpp

Vertex and index buffers, the vertex shader / stream cache, and the
immediate drawing helpers (sprites, beams, polylines) of the Babel display
layer. 0x0003ecc0-0x00041520.

The functions before the first assert tag of xbDrawprim.cpp (0x0003ecc0-
0x0003f020) are placed here because they use the same vertex shader and
stream cache (0x002bc400, 0x00284c10); their file is inferred.

Floating point: the original computes in x87 extended precision and rounds
where it stores; this port computes in float. Results can differ in the
last bits.
*/

#include "bdisplay_internal.h"

#define DRAWPRIM_FILE "C:\\Babel\\Xbox\\Src\\BDISPLAY\\xbDrawprim.cpp"

/* 0x0003ecc0  BDISPLAY/xbDrawprim.cpp (inferred): vertex size of an FVF code, 0 if unknown */
int bdFVFStride(int fvf)
{
	switch (fvf)
	{
	case 0x042: return 0x10;
	case 0x144: return 0x1c;
	case 0x152: return 0x24;
	case 0x15c: return 0x38;
	case 0x1c4: return 0x20;
	case 0x1d2: return 0x28;
	case 0x244: return 0x24;
	case 0x252: return 0x2c;
	case 0x25c: return 0x40;
	case 0x2c4: return 0x28;
	case 0x2d2: return 0x30;
	case 0x344: return 0x2c;
	case 0x352: return 0x34;
	case 0x35c: return 0x48;
	case 0x3c4: return 0x30;
	case 0x3d2: return 0x38;
	case 0x444: return 0x34;
	case 0x452: return 0x3c;
	case 0x45c: return 0x50;
	case 0x4c4: return 0x38;
	case 0x4d2: return 0x40;
	default: return 0;
	}
}

/* 0x0003ef90  BDISPLAY/xbDrawprim.cpp (inferred) */
int bdVertexBufferGetSize(bdVertexBuffer *vb)
{
	return vb->numVertices;
}

/* 0x0003efa0  BDISPLAY/xbDrawprim.cpp (inferred): set the vertex shader and
stream 0, skipping what is already set; vb NULL: no stream */
void bdSetVertexShaderAndStream(int fvf, bdVertexBuffer *vb)
{
	if ((uint32_t)fvf != DAT_002bc400)
	{
		D3DDevice_SetVertexShader(fvf);
		DAT_002bc400 = fvf;
	}
	if (vb != DAT_00284c10)
	{
		if (vb != NULL)
		{
			D3DDevice_SetStreamSource(0, vb->buffer[vb->drawBuffer], vb->stride);
			DAT_00284c10 = vb;
			return;
		}
		D3DDevice_SetStreamSource(0, NULL, bdFVFStride(fvf));
		DAT_00284c10 = vb;
	}
}

/* 0x0003f010  BDISPLAY/xbDrawprim.cpp (inferred) */
undefined4 FUN_0003f010(void)
{
	return DAT_002bc48c;
}

/* 0x0003f020  BDISPLAY/xbDrawprim.cpp (inferred): clips the segment a-b
against the plane z = 3 in clip space. a and b (originally in ESI and EDI)
are 13 floats: position x, y, z, w (0-3), clip-space position (4-7, written
here), then five values interpolated with the position (8-12: colour r, g,
b, a and width). Returns 0 if both ends are behind the plane, 1 if a was
moved onto it, 2 if b was, 3 if both are in front. */
int bdClipSegmentNear(float *a, float *b)
{
	static const int fields[9] = { 0, 1, 2, 3, 8, 9, 10, 11, 12 };
	float t;
	int i;

	FUN_000505c0(a + 4, DAT_002be470, a);
	FUN_000505c0(b + 4, DAT_002be470, b);
	if (a[6] <= 3.0f && b[6] <= 3.0f)
		return 0;
	if (a[6] <= 3.0f && 3.0f < b[6])
	{
		t = (b[6] - 3.0f) / ((b[6] - 3.0f) - (a[6] - 3.0f));
		for (i = 0; i < 9; i++)
			a[fields[i]] = (a[fields[i]] - b[fields[i]]) * t + b[fields[i]];
		return 1;
	}
	if (3.0f < a[6] && b[6] <= 3.0f)
	{
		t = (a[6] - 3.0f) / ((a[6] - 3.0f) - (b[6] - 3.0f));
		for (i = 0; i < 9; i++)
			b[fields[i]] = (b[fields[i]] - a[fields[i]]) * t + a[fields[i]];
		return 2;
	}
	return 3;
}

/* 0x0003f1f0  BDISPLAY/xbDrawprim.cpp */
int bdIndexBufferCreate(bdIndexBuffer **out, int numIndices)
{
	bdIndexBuffer *ib;

	if (numIndices == 0)
		numIndices = 1;
	ib = (bdIndexBuffer *)bdAlloc(numIndices * 2 + 0x20, DRAWPRIM_FILE, 0x5b9, 0x2001, "Index Buffer", 0);
	ib->numIndices = numIndices;
	ib->usage = 0x10;
	ib->format = 0x65;
	ib->d3d.Common = 0x10001;
	ib->d3d.Lock = 0;
	ib->d3d.Data = (DWORD)(uintptr_t)ib->indices;
	ib->next = DAT_002bcda8.next;
	ib->prev = &DAT_002bcda8;
	DAT_002bcda8.next->prev = ib;
	DAT_002bcda8.next = ib;
	*out = ib;
	return 1;
}

/* 0x0003f270  BDISPLAY/xbDrawprim.cpp */
void bdIndexBufferDestroy(bdIndexBuffer *ib)
{
	ib->prev->next = ib->next;
	ib->next->prev = ib->prev;
	bdFree(ib);
}

/* 0x0003f290  BDISPLAY/xbDrawprim.cpp */
WORD *bdIndexBufferGetData(bdIndexBuffer *ib)
{
	return ib->indices;
}

/* 0x0003f2a0  BDISPLAY/xbDrawprim.cpp */
void bdSetIndexBuffer(bdIndexBuffer *ib, int baseVertex)
{
	if (ib != NULL)
	{
		D3DDevice_SetIndices(&ib->d3d, baseVertex);
		return;
	}
	D3DDevice_SetIndices(NULL, 0);
}

/* the start of the immediate draws: vertex shader fvf, no stream */
static void set_immediate_shader(int fvf)
{
	if (DAT_002bc400 != (uint32_t)fvf)
	{
		D3DDevice_SetVertexShader(fvf);
		DAT_002bc400 = fvf;
	}
	if (DAT_00284c10 != NULL)
	{
		D3DDevice_SetStreamSource(0, NULL, bdFVFStride(fvf));
		DAT_00284c10 = NULL;
	}
}

/* 0x0003f2d0  BDISPLAY/xbDrawprim.cpp: draws bdVertex (FVF 0x152) from memory */
void bdDrawPrimitive(int type, void *vertices, int count)
{
	BYTE *v = (BYTE *)vertices;

	set_immediate_shader(BD_FVF_VERTEX);
	if (type == BDPRIMTYPE_QUADLIST)
	{
		for (; count != 0; count -= 4)
		{
			D3DDevice_DrawVerticesUP(D3DPT_TRIANGLESTRIP, 4, v, sizeof(bdVertex));
			v += 4 * sizeof(bdVertex);
		}
		return;
	}
	D3DDevice_DrawVerticesUP((D3DPRIMITIVETYPE)DAT_00284c14[type], count, v, sizeof(bdVertex));
}

/* 0x0003f360  BDISPLAY/xbDrawprim.cpp: draws bdTLVertex (FVF 0x144) from memory */
void bdDrawPrimitiveTL(int type, void *vertices, int count)
{
	BYTE *v = (BYTE *)vertices;

	set_immediate_shader(BD_FVF_TLVERTEX);
	if (type == BDPRIMTYPE_QUADLIST)
	{
		for (; count != 0; count -= 4)
		{
			D3DDevice_DrawVerticesUP(D3DPT_TRIANGLESTRIP, 4, v, sizeof(bdTLVertex));
			v += 4 * sizeof(bdTLVertex);
		}
		return;
	}
	D3DDevice_DrawVerticesUP((D3DPRIMITIVETYPE)DAT_00284c14[type], count, v, sizeof(bdTLVertex));
}

/* 0x0003f3f0  BDISPLAY/xbDrawprim.cpp: draws vertices of any FVF from memory */
void bdDrawPrimitiveFVF(int type, void *vertices, int count, int fvf)
{
	BYTE *v = (BYTE *)vertices;
	int stride;
	unsigned int quads;

	if ((uint32_t)fvf != DAT_002bc400)
	{
		D3DDevice_SetVertexShader(fvf);
		DAT_002bc400 = fvf;
	}
	if (DAT_00284c10 != NULL)
	{
		D3DDevice_SetStreamSource(0, NULL, bdFVFStride(fvf));
		DAT_00284c10 = NULL;
	}
	if (type == BDPRIMTYPE_QUADLIST)
	{
		stride = bdFVFStride(fvf);
		if (0 < count)
		{
			quads = ((unsigned int)(count - 1) >> 2) + 1;
			do
			{
				D3DDevice_DrawVerticesUP(D3DPT_TRIANGLESTRIP, 4, v, stride);
				v += stride * 4;
				quads--;
			} while (quads != 0);
		}
		return;
	}
	D3DDevice_DrawVerticesUP((D3DPRIMITIVETYPE)DAT_00284c14[type], count, v, bdFVFStride(fvf));
}

/* 0x0003f4a0  BDISPLAY/xbDrawprim.cpp: draws from a vertex buffer */
void bdDrawVertexBuffer(int type, bdVertexBuffer *vb, int start, int count)
{
	int i;

	bdSetVertexShaderAndStream(vb->fvf, vb);
	if (type == BDPRIMTYPE_QUADLIST)
	{
		i = 0;
		if (0 < count)
		{
			do
			{
				D3DDevice_DrawVertices(D3DPT_TRIANGLESTRIP, i, 4);
				i += 4;
			} while (i < count);
		}
		return;
	}
	D3DDevice_DrawVertices((D3DPRIMITIVETYPE)DAT_00284c14[type], start, count);
}

/* 0x0003f500  BDISPLAY/xbDrawprim.cpp: a vertex buffer of numBuffers copies
(rotated by bdVertexBufferLock); returns 0 when out of memory */
int bdVertexBufferCreate(bdVertexBuffer **out, int numVertices, int fvf, int flags, int numBuffers)
{
	bdVertexBuffer *vb;
	D3DVertexBuffer *header;
	unsigned int bufferSize;
	int totalSize;
	BYTE *data;
	unsigned int offset;
	int i = 0;

	if (numVertices == 0)
		numVertices = 1;
	vb = (bdVertexBuffer *)bdAlloc(numBuffers * 0xc + 0x4c, DRAWPRIM_FILE, 0x101, 0x2001,
		"Vertex Buffer Header", 0);
	vb->numVertices = numVertices;
	vb->flags = flags;
	vb->fvf = fvf;
	vb->stride = bdFVFStride(fvf);
	bufferSize = (unsigned int)(vb->stride * numVertices) + 0xfffu & 0xfffff000u;
	totalSize = (int)(bufferSize * numBuffers);
	vb->numBuffers = numBuffers;
	vb->current = 0;
	vb->drawBuffer = 0;
	data = (BYTE *)bdAlloc(totalSize, DRAWPRIM_FILE, 0x10d, 0x2003, "Vertex Buffer Data", 0x1000);
	if (data == NULL)
	{
		bdPrintf("bdVertexBufferCreate FAILED (Out of memory)\n");
		DbgBreakPoint();
		bdFree(vb);
		return 0;
	}
	vb->data = data;
	bdPhysicalProtect(data, totalSize, 0x404);
	if (0 < numBuffers)
	{
		offset = 0;
		header = vb->headers;
		do
		{
			vb->buffer[i] = header;
			header->Common = 1;
			header->Data = offset;
			header->Lock = 0;
			D3DResource_Register((D3DResource *)header, data);
			i++;
			offset += bufferSize;
			header++;
		} while (i < numBuffers);
	}
	vb->next = DAT_002bcdc8.next;
	vb->prev = (bdVertexBuffer *)&DAT_002bcdc8;
	DAT_002bcdc8.next->prev = vb;
	DAT_002bcdc8.next = vb;
	*out = vb;
	return 1;
}

/* 0x0003f650  BDISPLAY/xbDrawprim.cpp */
void bdVertexBufferDestroy(bdVertexBuffer *vb)
{
	int i;

	if (vb == DAT_00284c10)
		set_immediate_shader(BD_FVF_VERTEX);
	i = 0;
	if (0 < vb->numBuffers)
	{
		do
		{
			D3DResource_BlockUntilNotBusy((D3DResource *)vb->buffer[i]);
			i++;
		} while (i < vb->numBuffers);
	}
	vb->prev->next = vb->next;
	vb->next->prev = vb->prev;
	bdPhysicalProtect(vb->data, vb->stride * vb->numVertices * vb->numBuffers, 4);
	bdFree(vb->data);
	vb->data = NULL;
	bdFree(vb);
}

/* 0x0003f710  BDISPLAY/xbDrawprim.cpp: locks the next copy that the GPU
is not using; returns the data (also kept in vb->locked). flags 2 and 8
become D3DLOCK flags 0x80 and 0x20. */
void *bdVertexBufferLock(bdVertexBuffer *vb, unsigned int flags)
{
	BYTE *data;
	int tries = vb->numBuffers;

	if (1 < tries)
	{
		while (--tries != 0 && D3DResource_IsBusy((D3DResource *)vb->buffer[vb->current]) != 0)
			vb->current = (vb->current + 1) % vb->numBuffers;
		if (tries == 0)
			bdPrintf("bdVertexBufferLock: WARNING: vb 0x%08x has all buffers busy!\n", vb);
	}
	D3DVertexBuffer_Lock(vb->buffer[vb->current], 0, 0, &data, ((flags & 2) << 4 | flags & 8) << 2);
	vb->locked = data;
	return data;
}

/* 0x0003f790  BDISPLAY/xbDrawprim.cpp: the locked copy becomes the one to
draw and the next lock moves on; returns 1 when the rotation wrapped */
int bdVertexBufferUnlock(bdVertexBuffer *vb)
{
	int next;

	vb->drawBuffer = vb->current;
	next = vb->current + 1;
	vb->current = next % vb->numBuffers;
	return next / vb->numBuffers;
}

/* 0x0003f7b0  BDISPLAY/xbDrawprim.cpp: a screen-aligned quad of half size
width x height (in pixels) around the projection of pos, rotated by angle.
Writes 4 bdTLVertex to vertices; with vertices NULL it draws them with
texture. Returns 0 if pos is outside the depth range. */
int bdDrawSprite(float *pos, float width, float height, float angle, bdTexture *texture, int flipU,
	int flipV, int r, int g, int b, int a, float *vertices, float zBias)
{
	float p[4];
	float local[28];
	float c, s, nh, nw;
	float left, top;
	DWORD colour;

	if (vertices == NULL)
		vertices = local;
	bdProjectPoints(p, pos, 1);
	if (p[2] < 0.05f || p[2] > 1.0f)
		return 0;
	p[0] = DAT_002be5a0 + p[0];
	p[1] = DAT_002be5a4 - p[1];
	p[2] = p[2] + zBias;
	colour = BD_RGBA(r, g, b, a);
	((DWORD *)vertices)[4] = colour;
	vertices[3] = p[3];
	vertices[5] = flipU != 0 ? 0.0f : 1.0f;
	vertices[6] = flipV != 0 ? 1.0f : 0.0f;
	((DWORD *)vertices)[0xb] = colour;
	vertices[10] = p[3];
	vertices[0xc] = flipU != 0 ? 0.0f : 1.0f;
	vertices[0xd] = flipV != 0 ? 0.0f : 1.0f;
	vertices[0x11] = p[3];
	((DWORD *)vertices)[0x12] = colour;
	vertices[0x13] = flipU != 0 ? 1.0f : 0.0f;
	vertices[0x14] = flipV != 0 ? 1.0f : 0.0f;
	((DWORD *)vertices)[0x19] = colour;
	vertices[0x18] = p[3];
	vertices[0x1a] = flipU != 0 ? 1.0f : 0.0f;
	vertices[0x1b] = flipV != 0 ? 0.0f : 1.0f;
	if (angle == 0.0f)
	{
		vertices[0] = p[0] + width;
		vertices[1] = p[1] - height;
		vertices[2] = p[2];
		vertices[7] = p[0] + width;
		top = p[1] + height;
		vertices[8] = top;
		vertices[9] = p[2];
		left = p[0] - width;
		vertices[0xe] = left;
		vertices[0xf] = p[1] - height;
		vertices[0x10] = p[2];
		vertices[0x15] = left;
		vertices[0x16] = top;
	}
	else
	{
		c = cosf(angle);
		s = sinf(angle);
		nh = -height;
		vertices[0] = nh * s + c * width + p[0];
		vertices[1] = (nh * c - s * width) + p[1];
		vertices[2] = p[2];
		vertices[7] = s * height + c * width + p[0];
		vertices[8] = (c * height - s * width) + p[1];
		vertices[9] = p[2];
		nw = -width;
		vertices[0xe] = nw * c + nh * s + p[0];
		vertices[0xf] = (nh * c - nw * s) + p[1];
		vertices[0x10] = p[2];
		vertices[0x15] = nw * c + s * height + p[0];
		vertices[0x16] = (c * height - nw * s) + p[1];
	}
	vertices[0x17] = p[2];
	if (vertices == local)
	{
		bdSetTexture(0, texture);
		bdDrawPrimitiveTL(BDPRIMTYPE_TRISTRIP, local, 4);
	}
	return 1;
}

/* 0x0003faf0  BDISPLAY/xbDrawprim.cpp: a textured beam from the projection
of pos1 to that of pos2 (8 bdTLVertex, two quads); width is the half width
at each end, length how far the beam extends past it. Returns 0 if an end
is outside the depth range or the beam is off screen. */
int bdDrawBeam(float *pos1, float width1, float length1, int *colour1, float *pos2, float width2,
	float length2, int *colour2, bdTexture *texture, float *vertices, float zBias)
{
	float a[4], b[4];
	float local[56];
	float cy, extent, dx, dy, len, ux, uy;
	float wx, wy, lx, vs1, vs2, x1, y1, x2, y2, bx, by;

	bdProjectPoints(a, pos1, 1);
	cy = DAT_002be5a4;
	if (a[2] < 0.1f || a[2] > 1.0f)
		return 0;
	a[0] = DAT_002be5a0 + a[0];
	pos2[3] = 1.0f;
	a[1] = cy - a[1];
	bdProjectPoints(b, pos2, 1);
	if (b[2] < 0.1f || b[2] > 1.0f)
		return 0;
	b[0] = DAT_002be5a0 + b[0];
	b[1] = DAT_002be5a4 - b[1];
	extent = length1;
	if (length1 < width1)
		extent = width1;
	if (extent < width2)
		extent = width2;
	if (extent < length2)
		extent = length2;
	if (!(((0.0f <= a[0] + extent) || (0.0f <= b[0] + extent)) &&
		((a[0] - extent <= DAT_002be590) || (b[0] - extent <= DAT_002be590)) &&
		((0.0f <= a[1] + extent) || (0.0f <= b[1] + extent))))
		return 0;
	if (DAT_002be594 < a[1] - extent && DAT_002be594 < b[1] - extent)
		return 0;
	if (vertices == NULL)
		vertices = local;
	dx = b[0] - a[0];
	dy = b[1] - a[1];
	len = bd_sqrt_approx(dx * dx + dy * dy);
	((DWORD *)vertices)[4] = BD_RGBA(colour1[0], colour1[1], colour1[2], colour1[3]);
	((DWORD *)vertices)[0x19] = ((DWORD *)vertices)[4];
	((DWORD *)vertices)[0x12] = ((DWORD *)vertices)[4];
	((DWORD *)vertices)[0xb] = ((DWORD *)vertices)[4];
	((DWORD *)vertices)[0x20] = BD_RGBA(colour2[0], colour2[1], colour2[2], colour2[3]);
	((DWORD *)vertices)[0x35] = ((DWORD *)vertices)[0x20];
	((DWORD *)vertices)[0x2e] = ((DWORD *)vertices)[0x20];
	((DWORD *)vertices)[0x27] = ((DWORD *)vertices)[0x20];
	vertices[0x11] = a[3];
	ux = dx * (1.0f / len);
	vertices[0x18] = a[3];
	vertices[10] = a[3];
	vertices[3] = a[3];
	vertices[0x34] = b[3];
	vertices[0x2d] = b[3];
	uy = (1.0f / len) * dy;
	vertices[0x26] = b[3];
	vertices[0x1f] = b[3];
	wx = uy * width2;
	wy = -(width2 * ux);
	lx = uy * length2;
	if (width1 <= width2)
	{
		vs1 = (width1 / width2) * 0.5f;
		vs2 = 0.5f;
	}
	else
	{
		vs1 = 0.5f;
		vs2 = (width2 / width1) * 0.5f;
	}
	x1 = a[0] - uy * width1;
	vertices[6] = 0.0f;
	vertices[0xd] = 0.0f;
	vertices[0x14] = 0.5f;
	vertices[0x1b] = 0.5f;
	vertices[0x22] = 0.5f;
	vertices[0x29] = 0.5f;
	vertices[0] = x1 - length1 * ux;
	y1 = a[1] - (-(width1 * ux));
	vertices[1] = y1 - uy * length1;
	a[2] = a[2] + zBias;
	vertices[2] = a[2];
	vertices[0x10] = a[2];
	vertices[5] = 0.5f - vs1;
	vertices[0x13] = 0.5f - vs1;
	x2 = uy * width1 + a[0];
	vertices[0x15] = x2;
	vertices[7] = x2 - length1 * ux;
	vertices[0x17] = a[2];
	y2 = -(width1 * ux) + a[1];
	vertices[0x16] = y2;
	vertices[8] = y2 - uy * length1;
	vertices[9] = a[2];
	vertices[0xc] = vs1 + 0.5f;
	vertices[0x1a] = vs1 + 0.5f;
	vertices[0xe] = x1;
	vertices[0xf] = y1;
	bx = b[0] - wx;
	vertices[0x1c] = bx;
	vertices[0x1d] = b[1] - wy;
	b[2] = b[2] + zBias;
	vertices[0x1e] = b[2];
	by = 0.5f - vs2;
	vertices[0x21] = by;
	vertices[0x23] = wx + b[0];
	vertices[0x24] = wy + b[1];
	vertices[0x25] = b[2];
	vertices[0x28] = vs2 + 0.5f;
	vertices[0x2c] = b[2];
	vertices[0x2a] = bx + ux * length2;
	vertices[0x2f] = by;
	vertices[0x30] = 1.0f;
	vertices[0x33] = b[2];
	vertices[0x2b] = (b[1] - wy) + lx;
	vertices[0x37] = 1.0f;
	vertices[0x31] = ux * length2 + wx + b[0];
	vertices[0x32] = lx + wy + b[1];
	vertices[0x36] = vs2 + 0.5f;
	if (vertices == local)
	{
		bdSetTexture(0, texture);
		bdDrawPrimitiveTL(BDPRIMTYPE_TRISTRIP, local, 8);
	}
	return 1;
}

/* 0x00040070  BDISPLAY/xbDrawprim.cpp: a strip of quads along numPoints
projected points, widths in pixels (one per point). Writes bdTLVertex to
vertices: 4 for the first point, 2 for each inner point, 4 for the last;
returns how many, or 0 if a point is outside the depth range.

As in the original, with exactly two points the end of the strip uses
values left over from the first segment (dir = P0.y - ux * w0, and
ux * w0 instead of ux) instead of the segment's direction. */
int bdDrawPolylineStrip(int numPoints, float *points, float *widths, int *colours, float *vertices,
	float zBias)
{
	float p0[4], p1[4];
	float dx, dy, inv, ux, uyOld, ux0w, uyw, x0, y0, z, xR, yL, dir;
	float uxNew, ox, oy, capX, capY;
	float *v;
	int *colour;
	int i;
	DWORD rgba;

	bdProjectPoints(p0, points, 1);
	if (p0[2] < 0.05f || p0[2] > 1.0f)
		return 0;
	p0[0] = DAT_002be5a0 + p0[0];
	p0[1] = DAT_002be5a4 - p0[1];
	bdProjectPoints(p1, points + 4, 1);
	if (p1[2] < 0.05f || p1[2] > 1.0f)
		return 0;
	p1[0] = DAT_002be5a0 + p1[0];
	p1[1] = DAT_002be5a4 - p1[1];
	dx = p1[0] - p0[0];
	dy = p1[1] - p0[1];
	inv = 1.0f / bd_sqrt_approx(dx * dx + dy * dy);
	ux = dx * inv;
	uyOld = inv * dy;
	uyw = uyOld * widths[0];
	ux0w = ux * widths[0];

	/* the first point: two vertices for the start cap, two across */
	v = vertices;
	colour = colours;
	rgba = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
	v[5] = 0.0f;
	v[6] = 0.0f;
	x0 = p0[0] - uyw;
	v[0] = x0 - ux0w;
	y0 = p0[1] - (-ux0w);
	v[1] = y0 - uyw;
	z = p0[2] + zBias;
	v[2] = z;
	xR = uyw + p0[0];
	((DWORD *)v)[4] = rgba;
	v[3] = 1.0f;
	v[7] = xR - ux0w;
	v[0xc] = 1.0f;
	yL = -ux0w + p0[1];
	v[8] = yL - uyw;
	v[9] = z;
	v[0xd] = 0.0f;
	v[10] = 1.0f;
	((DWORD *)v)[0xb] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
	v[0xe] = x0;
	v[0x10] = z;
	v[0xf] = y0;
	v[0x13] = 0.0f;
	v[0x14] = 0.5f;
	((DWORD *)v)[0x12] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
	v[0x11] = 1.0f;
	v[0x15] = xR;
	v[0x16] = yL;
	v[0x17] = z;
	v[0x1a] = 1.0f;
	v[0x1b] = 0.5f;
	v[0x18] = 1.0f;
	((DWORD *)v)[0x19] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
	v += 0x1c;
	colour += 4;
	i = 1;
	/* what the end cap uses (see above) */
	dir = yL;
	uxNew = ux0w;
	if (numPoints - 1 >= 2)
	{
		/* the inner points: two vertices each, mitred by the mean direction */
		do
		{
			p0[0] = p1[0];
			p0[1] = p1[1];
			p0[2] = p1[2];
			p0[3] = p1[3];
			bdProjectPoints(p1, points + 4 * (i + 1), 1);
			if (p1[2] < 0.05f)
				return 0;
			if (p1[2] > 1.0f)
				return 0;
			p1[0] = DAT_002be5a0 + p1[0];
			p1[1] = DAT_002be5a4 - p1[1];
			dx = p1[0] - p0[0];
			dy = p1[1] - p0[1];
			inv = 1.0f / bd_sqrt_approx(dx * dx + dy * dy);
			uxNew = dx * inv;
			dir = inv * dy;
			ox = (uyOld + dir) * 0.5f * widths[i];
			oy = -((uxNew + ux) * 0.5f * widths[i]);
			v[5] = 0.0f;
			v[6] = 0.5f;
			v[0] = p0[0] - ox;
			v[1] = p0[1] - oy;
			v[2] = p0[2] + zBias;
			v[3] = 1.0f;
			((DWORD *)v)[4] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
			v[7] = ox + p0[0];
			v[0xc] = 1.0f;
			v[0xd] = 0.5f;
			v[8] = oy + p0[1];
			v[9] = p0[2] + zBias;
			((DWORD *)v)[0xb] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
			v[10] = 1.0f;
			v += 0xe;
			colour += 4;
			i++;
			ux = uxNew;
			uyOld = dir;
		} while (i < numPoints - 1);
	}

	/* the last point: two vertices across, two for the end cap */
	dir = dir * widths[i];
	uxNew = uxNew * widths[i];
	v[5] = 0.0f;
	v[6] = 0.5f;
	capX = p1[0] - dir;
	v[0] = capX;
	capY = p1[1] - (-uxNew);
	v[1] = capY;
	z = p1[2] + zBias;
	v[2] = z;
	((DWORD *)v)[4] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
	v[3] = 1.0f;
	v[7] = dir + p1[0];
	v[0xd] = 0.5f;
	v[0xc] = 1.0f;
	v[8] = -uxNew + p1[1];
	v[9] = z;
	v[10] = 1.0f;
	((DWORD *)v)[0xb] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
	v[0xe] = capX + uxNew;
	v[0x13] = 0.0f;
	v[0x14] = 1.0f;
	v[0xf] = capY + dir;
	v[0x10] = z;
	((DWORD *)v)[0x12] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
	v[0x11] = 1.0f;
	v[0x1a] = 1.0f;
	v[0x1b] = 1.0f;
	v[0x15] = uxNew + dir + p1[0];
	v[0x16] = dir + -uxNew + p1[1];
	v[0x17] = z;
	((DWORD *)v)[0x19] = BD_RGBA(colour[0], colour[1], colour[2], colour[3]);
	v[0x18] = 1.0f;
	return (int)(((BYTE *)(v + 0x15) - (BYTE *)vertices) + 0x1c) / 0x1c;
}

/* 0x000406d0  BDISPLAY/xbDrawprim.cpp: draws count vertices of vb from
start as point sprites of size pointSize (Ghidra lists three arguments:
the size, the fourth, is read from the stack) */
void bdDrawPointSprites(bdVertexBuffer *vb, int start, int count, float pointSize)
{
	D3DDevice_SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
	D3DDevice_SetRenderState(D3DRS_POINTSCALEENABLE, TRUE);
	D3DDevice_SetRenderState(D3DRS_POINTSIZE, bd_float_bits(pointSize));
	bdSetVertexShaderAndStream(0x42, vb);
	bdSetVertexShaderAndStream(vb->fvf, vb);
	D3DDevice_DrawVertices((D3DPRIMITIVETYPE)DAT_00284c14[BDPRIMTYPE_POINTLIST], start, count);
	D3DDevice_SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
	D3DDevice_SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
}

/* 0x00040750  BDISPLAY/xbDrawprim.cpp: bdDrawSprite with the size in world units */
int bdDrawSprite3D(float *pos, float width, float height, float angle, bdTexture *texture, int flipU,
	int flipV, int r, int g, int b, int a, float *vertices, float zBias)
{
	if (bdProjectSize(pos, width, height, &width, &height) == 0.0f)
		return 0;
	return bdDrawSprite(pos, width, height, angle, texture, flipU, flipV, r, g, b, a, vertices, zBias);
}

/* 0x000407d0  BDISPLAY/xbDrawprim.cpp: bdDrawBeam with the sizes in world units */
int bdDrawBeam3D(float *pos1, float width1, float length1, int *colour1, float *pos2, float width2,
	float length2, int *colour2, bdTexture *texture, float *vertices, float zBias)
{
	if (bdProjectSize(pos1, width1, length1, &width1, &length1) == 0.0f)
		return 0;
	if (bdProjectSize(pos2, width2, length2, &width2, &length2) == 0.0f)
		return 0;
	return bdDrawBeam(pos1, width1, length1, colour1, pos2, width2, length2, colour2, texture, vertices,
		zBias);
}

/* 0x00040880  BDISPLAY/xbDrawprim.cpp: bdDrawPolylineStrip with the widths
in world units (screenWidths receives the pixel widths) */
int bdDrawPolyline3D(int numPoints, float *points, float *widths, float *screenWidths, int *colours,
	float *vertices, float zBias)
{
	int i = 0;

	if (0 < numPoints)
	{
		do
		{
			if (bdProjectWidth(points + 4 * i, widths[i], &screenWidths[i]) == 0.0f)
				return 0;
			i++;
		} while (i < numPoints);
	}
	return bdDrawPolylineStrip(numPoints, points, screenWidths, colours, vertices, zBias);
}

/* 0x00040900  BDISPLAY/xbDrawprim.cpp: bdDrawBeam3D after clipping the beam
against the near plane (z = 3 in clip space); size and colour of the moved
end are interpolated */
int bdDrawBeamClipped(float *pos1, float width1, float length1, int *colour1, float *pos2,
	float width2, float length2, int *colour2, bdTexture *texture, float *vertices, float zBias)
{
	float clip1[4], clip2[4];
	float point[4];
	int colour[4];
	float t;
	int i;

	FUN_000505c0(clip1, DAT_002be470, pos1);
	FUN_000505c0(clip2, DAT_002be470, pos2);
	if (clip1[2] <= 3.0f && clip2[2] <= 3.0f)
		return 0;
	if (3.0f < clip1[2] && clip2[2] <= 3.0f)
	{
		t = (clip1[2] - 3.0f) / ((clip1[2] - 3.0f) - (clip2[2] - 3.0f));
		for (i = 0; i < 4; i++)
			point[i] = (pos2[i] - pos1[i]) * t + pos1[i];
		for (i = 0; i < 4; i++)
			colour[i] = (int)((float)(colour2[i] - colour1[i]) * t + (float)colour1[i]);
		return bdDrawBeam3D(pos1, width1, length1, colour1, point, (width2 - width1) * t + width1,
			(length2 - length1) * t + length1, colour, texture, vertices, zBias);
	}
	if (clip1[2] <= 3.0f && 3.0f < clip2[2])
	{
		t = (clip2[2] - 3.0f) / ((clip2[2] - 3.0f) - (clip1[2] - 3.0f));
		for (i = 0; i < 4; i++)
			point[i] = (pos1[i] - pos2[i]) * t + pos2[i];
		for (i = 0; i < 4; i++)
			colour[i] = (int)((float)(colour1[i] - colour2[i]) * t + (float)colour2[i]);
		return bdDrawBeam3D(point, (width1 - width2) * t + width2, (length1 - length2) * t + length2,
			colour, pos2, width2, length2, colour2, texture, vertices, zBias);
	}
	return bdDrawBeam3D(pos1, width1, length1, colour1, pos2, width2, length2, colour2, texture,
		vertices, zBias);
}

/* 0x00040cd0  BDISPLAY/xbDrawprim.cpp: a polyline in world space, clipped
against the near plane: the visible runs are drawn by bdDrawPolyline3D
into vertices, joined by repeated vertices (degenerate triangles). points
are 4 floats, colours 4 ints (r, g, b, a) per point. screenWidths is
scratch for bdDrawPolyline3D. Returns the number of vertices written. The
DbgBreakPoint calls are the original's buffer checks. */
int bdDrawPolylineClipped(int numPoints, float *points, float *widths, float *screenWidths,
	int *colours, float *vertices, float zBias)
{
	float *runPoints, *runWidths;
	int *runColours;
	float *outPoint;
	float *outWidth;
	int *outColour;
	float *v;
	float a[13], b[13];
	int remaining, started, total, clip, count, written, seg;
	int last;

	runPoints = (float *)bdNew(numPoints << 5, DRAWPRIM_FILE, 0x4a4, 0x2003);
	runWidths = (float *)bdNew(numPoints * 8, DRAWPRIM_FILE, 0x4a5, 0x2003);
	runColours = (int *)bdNew(numPoints << 5, DRAWPRIM_FILE, 0x4a6, 0x2003);
	remaining = numPoints - 1;
	started = 0;
	total = 0;
	if (remaining != 0)
	{
		v = vertices;
		outPoint = runPoints;
		outWidth = runWidths;
		outColour = runColours;
		seg = 0;
		do
		{
			/* a: point seg, b: point seg + 1 */
			a[0] = points[seg * 4 + 0];
			a[1] = points[seg * 4 + 1];
			a[2] = points[seg * 4 + 2];
			a[3] = points[seg * 4 + 3];
			a[8] = (float)colours[seg * 4 + 0];
			a[9] = (float)colours[seg * 4 + 1];
			a[10] = (float)colours[seg * 4 + 2];
			a[11] = (float)colours[seg * 4 + 3];
			a[12] = widths[seg];
			b[0] = points[seg * 4 + 4];
			b[1] = points[seg * 4 + 5];
			b[2] = points[seg * 4 + 6];
			b[3] = points[seg * 4 + 7];
			b[8] = (float)colours[seg * 4 + 4];
			b[9] = (float)colours[seg * 4 + 5];
			b[10] = (float)colours[seg * 4 + 6];
			b[11] = (float)colours[seg * 4 + 7];
			b[12] = widths[seg + 1];
			remaining--;
			clip = bdClipSegmentNear(a, b);
			if ((clip == 2 || clip == 3) && started == 0)
			{
				/* a run starts at a */
				if (numPoints * 8 - 4 < (int)(outPoint - runPoints))
					DbgBreakPoint();
				outPoint[0] = a[0];
				outPoint[1] = a[1];
				outPoint[2] = a[2];
				outPoint[3] = a[3];
				outPoint += 4;
				if (numPoints * 2 - 1 < (int)(outWidth - runWidths))
					DbgBreakPoint();
				*outWidth++ = a[12];
				if (numPoints * 8 - 4 < (int)(outColour - runColours))
					DbgBreakPoint();
				outColour[0] = (int)a[8];
				outColour[1] = (int)a[9];
				outColour[2] = (int)a[10];
				outColour[3] = (int)a[11];
				outColour += 4;
				started = 1;
			}
			if (clip == 1)
			{
				/* a was behind: a new run from the clipped a to b */
				if (numPoints * 8 - 8 < (int)(outPoint - runPoints))
					DbgBreakPoint();
				outPoint[0] = a[0];
				outPoint[1] = a[1];
				outPoint[2] = a[2];
				outPoint[3] = a[3];
				outPoint[4] = b[0];
				outPoint[5] = b[1];
				outPoint[6] = b[2];
				outPoint[7] = b[3];
				outPoint += 8;
				if (numPoints * 2 - 2 < (int)(outWidth - runWidths))
					DbgBreakPoint();
				outWidth[0] = a[12];
				outWidth[1] = b[12];
				outWidth += 2;
				if (numPoints * 8 - 8 < (int)(outColour - runColours))
					DbgBreakPoint();
				outColour[0] = (int)a[8];
				outColour[1] = (int)a[9];
				outColour[2] = (int)a[10];
				outColour[3] = (int)a[11];
				outColour[4] = (int)b[8];
				outColour[5] = (int)b[9];
				outColour[6] = (int)b[10];
				outColour[7] = (int)b[11];
				outColour += 8;
				count = started + 2;
			}
			else if (clip == 2)
			{
				/* b was behind: the run ends at the clipped b */
				if (numPoints * 8 - 4 < (int)(outPoint - runPoints))
					DbgBreakPoint();
				outPoint[0] = b[0];
				outPoint[1] = b[1];
				outPoint[2] = b[2];
				outPoint[3] = b[3];
				if (numPoints * 2 - 1 < (int)(outWidth - runWidths))
					DbgBreakPoint();
				*outWidth = b[12];
				if (numPoints * 8 - 4 < (int)(outColour - runColours))
					DbgBreakPoint();
				outColour[0] = (int)b[8];
				outColour[1] = (int)b[9];
				outColour[2] = (int)b[10];
				outColour[3] = (int)b[11];
				count = started + 1;
				outPoint += 4;
				outWidth += 1;
				outColour += 4;
				if (0 < remaining)
				{
					if (numPoints * 2 < count)
						DbgBreakPoint();
					if (numPoints < count)
						DbgBreakPoint();
					if ((numPoints * 4 - (int)(((BYTE *)v - (BYTE *)vertices) + 0x1c) / 0x1c) + 4 <
						count * 2 + 4)
						DbgBreakPoint();
					written = bdDrawPolyline3D(count, runPoints, runWidths, screenWidths, runColours,
						v + 7, zBias);
					last = numPoints * 4 + 4;
					started = 0;
					if (last < (int)((BYTE *)v - (BYTE *)vertices) / 0x1c)
						DbgBreakPoint();
					/* repeat the first vertex before the run */
					v[0] = v[7];
					v[2] = v[9];
					v[1] = v[8];
					v[3] = v[10];
					if (last < (int)((written * 0x1c - (int)(uintptr_t)vertices) + 0x1c +
						(int)(uintptr_t)v) / 0x1c)
						DbgBreakPoint();
					/* and the last vertex after it */
					v[written * 7 + 7] = v[written * 7];
					v[written * 7 + 8] = v[written * 7 + 1];
					v[written * 7 + 9] = v[written * 7 + 2];
					v[written * 7 + 10] = v[written * 7 + 3];
					total = total + 2 + written;
					v = v + (written + 2) * 7;
					outPoint = runPoints;
					count = started;
					outWidth = runWidths;
					outColour = runColours;
				}
			}
			else
			{
				count = started;
				if (clip == 3)
				{
					/* both in front: the run goes on to b */
					if (numPoints * 8 - 4 < (int)(outPoint - runPoints))
						DbgBreakPoint();
					outPoint[0] = b[0];
					outPoint[1] = b[1];
					outPoint[2] = b[2];
					outPoint[3] = b[3];
					outPoint += 4;
					if (numPoints * 2 - 1 < (int)(outWidth - runWidths))
						DbgBreakPoint();
					*outWidth++ = b[12];
					if (numPoints * 8 - 4 < (int)(outColour - runColours))
						DbgBreakPoint();
					outColour[0] = (int)b[8];
					outColour[1] = (int)b[9];
					outColour[2] = (int)b[10];
					outColour[3] = (int)b[11];
					outColour += 4;
					count = started + 1;
				}
			}
			started = count;
			seg++;
		} while (remaining != 0);
		if (started != 0)
		{
			if (numPoints * 2 < started)
				DbgBreakPoint();
			if (numPoints < started)
				DbgBreakPoint();
			if ((numPoints * 4 - (int)(((BYTE *)v - (BYTE *)vertices) + 0x1c) / 0x1c) + 4 < started * 2 + 4)
				DbgBreakPoint();
			written = bdDrawPolyline3D(started, runPoints, runWidths, screenWidths, runColours, v + 7, zBias);
			last = numPoints * 4 + 4;
			if (last < (int)((BYTE *)v - (BYTE *)vertices) / 0x1c)
				DbgBreakPoint();
			v[1] = v[8];
			v[2] = v[9];
			v[0] = v[7];
			v[3] = v[10];
			if (last < (int)((written * 0x1c - (int)(uintptr_t)vertices) + 0x1c + (int)(uintptr_t)v) / 0x1c)
				DbgBreakPoint();
			v[written * 7 + 7] = v[written * 7];
			v[written * 7 + 8] = v[written * 7 + 1];
			v[written * 7 + 9] = v[written * 7 + 2];
			v[written * 7 + 10] = v[written * 7 + 3];
			total = total + 2 + written;
		}
	}
	bdFree(runColours);
	bdFree(runWidths);
	bdFree(runPoints);
	return total;
}
