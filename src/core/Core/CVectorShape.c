/*
CVECTORSHAPE.C  <-  C:\BlitzParty\Game\Src\Core\CVectorShape.cpp

openfrenzy core module: CVectorShape, a shape drawn as coloured lines (vertices and line
index pairs from a data block). Decompiled from orig/default.xbe (functional, not
byte-matching). Only 0x0002f5a0 is tagged with this file; its neighbours 0x0002f4a0-
0x0002f710 use the same object, so they are placed here.
*/

#include <string.h>

#include "core_globals.h"
#include "core_extern.h"

/* the shape data: nverts, nlines, then nverts vertices (x, y, z, w) and nlines index pairs */
struct CVectorShape {
	int colour[4];               /* +0x00 red, green, blue, alpha (0-255) */
	int vertex_count;            /* +0x10 */
	int line_count;              /* +0x14 */
	const float (*verts)[4];     /* +0x18 */
	const int *lines;            /* +0x1c */
	const int *data;             /* +0x20 */
	unsigned char *vertices;     /* +0x24 2 vertices of 0x24 bytes per line */
	int count;                   /* +0x28 vertices in .vertices */
};
_Static_assert(sizeof(struct CVectorShape) == 0x2c, "CVectorShape size");

static uint32_t CVectorShape_Colour(const int *rgba)
{
	return DAT_002bc490[rgba[1]] | DAT_002bd640[rgba[2]] | DAT_002bce18[rgba[0]] | DAT_002bc898[rgba[3]];
}

/* 0x0002f4a0  (Core, CVectorShape.cpp) draw all lines in the shape's colour */
void CVectorShape_Draw(struct CVectorShape *self)
{
	int i;

	for (i = 0; i < self->count; i++)
		*(uint32_t *)(self->vertices + i * 0x24 + 0x18) = CVectorShape_Colour(self->colour);
	ext_DrawPrimitives(1, self->vertices, self->count);
}

/* 0x0002f500  (Core, CVectorShape.cpp) draw the first COUNT vertices (whole lines) */
void CVectorShape_DrawPart(struct CVectorShape *self, unsigned int count)
{
	unsigned int n = count & 0xfffffffe;
	unsigned int i;

	if (self->count < (int)(count & 0xfffffffe))
		n = (unsigned int)self->count;
	if (n == 0)
		return;
	for (i = 0; i < n; i++)
		*(uint32_t *)(self->vertices + i * 0x24 + 0x18) = CVectorShape_Colour(self->colour);
	ext_DrawPrimitives(1, self->vertices, (int)n);
}

/* 0x0002f570  (Core, CVectorShape.cpp) CVectorShape::CVectorShape (returns this, eax at the
original's ret) */
struct CVectorShape *CVectorShape_Init(struct CVectorShape *self)
{
	self->vertices = NULL;
	self->data = NULL;
	return self;
}

/* 0x0002f580  (Core, CVectorShape.cpp) CVectorShape::~CVectorShape */
void CVectorShape_Free(struct CVectorShape *self)
{
	ext_Delete(self->vertices);
	self->vertices = NULL;
}

/* 0x0002f5a0  Core/CVectorShape.cpp  set the shape DATA and COLOUR; build the line vertices */
void CVectorShape_Set(struct CVectorShape *self, const int *data, const int *colour)
{
	unsigned char *vertex;
	const int *line;
	int i;

	self->colour[0] = colour[0];
	self->colour[1] = colour[1];
	self->colour[2] = colour[2];
	self->colour[3] = colour[3];
	self->data = data;
	self->vertex_count = data[0];
	self->verts = (const float (*)[4])(data + 2);
	self->line_count = data[1];
	self->lines = data + 2 + self->vertex_count * 4;
	self->count = self->line_count * 2;
	if (self->vertices != NULL)
	{
		ext_Delete(self->vertices);
		self->vertices = NULL;
	}
	vertex = ext_New((unsigned int)self->count * 0x24, "C:\\BlitzParty\\Game\\Src\\Core\\CVectorShape.cpp",
		0x7a, 0x2001);
	self->vertices = vertex;
	line = self->lines;
	for (i = self->line_count; i != 0; i--)
	{
		uint32_t *v = (uint32_t *)vertex;

		memcpy(&v[0], self->verts[line[0]], 3 * sizeof(float));
		v[6] = CVectorShape_Colour(self->colour);
		v[7] = 0;
		v[8] = 0;
		memcpy(&v[9], self->verts[line[1]], 3 * sizeof(float));
		v[0xf] = CVectorShape_Colour(self->colour);
		v[0x10] = 0x3f800000;
		v[0x11] = 0x3f800000;
		vertex += 0x48;
		line += 2;
	}
}

/* 0x0002f710  (Core, CVectorShape.cpp) a triangle per line, to the point CENTRE, into the
vertex buffer *VB (made or regrown here): line ends in COLOUR, the centre in GLOW (or the
first table entries when NULL) */
void CVectorShape_BuildFan(struct CVectorShape *self, const float *centre, int *vb,
	const int *colour, const int *glow)
{
	int buffer = *vb;
	int n = self->line_count;
	uint32_t *v;
	const int *line;

	if (buffer != 0)
	{
		if (((int (*)(int))FUN_0003ef90)(buffer) < n * 3)
		{
			/* the original frees the buffer but keeps using the old handle */
			((void (*)(int))FUN_0003f650)(buffer);
			*vb = 0;
		}
	}
	if (buffer == 0)
		((int (*)(int *, int, int, int, int))FUN_0003f500)(&buffer, n * 3, 0x152, 8, 1);
	v = ((uint32_t *(*)(int, int))FUN_0003f710)(buffer, 4);
	if (v == NULL)
	{
		*vb = buffer;
		return;
	}
	line = self->lines;
	for (; n != 0; n--)
	{
		memcpy(&v[0], self->verts[line[0]], 3 * sizeof(float));
		v[6] = DAT_002bd640[colour[2]] | DAT_002bc898[colour[3]] | DAT_002bce18[colour[0]] | DAT_002bc490[colour[1]];
		v[7] = 0;
		v[8] = 0;
		memcpy(&v[9], self->verts[line[1]], 3 * sizeof(float));
		v[0xf] = DAT_002bd640[colour[2]] | DAT_002bc898[colour[3]] | DAT_002bce18[colour[0]] | DAT_002bc490[colour[1]];
		v[0x10] = 0;
		v[0x11] = 0x3f800000;
		memcpy(&v[0x12], centre, 3 * sizeof(float));
		line += 2;
		if (glow == NULL)
			v[0x18] = DAT_002bc898[0] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
		else
			v[0x18] = DAT_002bc898[glow[3]] | DAT_002bd640[glow[2]] | DAT_002bc490[glow[1]] | DAT_002bce18[glow[0]];
		v[0x19] = 0x3f800000;
		v[0x1a] = 0x3f000000;
		v += 0x1b;
	}
	((void (*)(int))FUN_0003f790)(buffer);
	*vb = buffer;
}
