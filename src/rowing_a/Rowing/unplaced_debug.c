/*
Rowing/unplaced_debug.c  <-  Rowing gap 0x001afa50-0x001aff20 (no file tag)

The debug text and lines object (game +0x9c, vtable 0x00275bc4) and the debug drawing
helpers (rings, lines and rectangles in screen or world space). The drawing helpers are
thiscall in the XBE with `this` (the debug object) not used; the port has no `this`.

The object (fields at original offsets): +0x00 the vtable, +0x04 the font
("handgotb.ttf"), +0x08 the queued texts, +0x0c the queued lines, +0x10 the texts (500 bytes
each), +0x2720 the lines (0x24 bytes: from, to, r, g, b).
The vertices are .bss: 0x0030f770 (21 ring points), 0x0030fa68 (2 line points),
0x0030fab0 (4 rectangle corners), 0x24 bytes each (x, y, z at +0, colour at +0x18).
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"

extern unsigned int DAT_002bca94; /* bdisplay: the opaque alpha part of a colour */


/* .rdata 0x00275bc4: the debug object's vtable (rebuilt) */
const void *const vtable_00275bc4[1] = {
	(const void *)FUN_001aff20, /* +0x00 deleting destructor */
};

/* an opaque display colour from R, G, B */
static unsigned int ra_Colour(int r, int g, int b)
{
	return DAT_002bce18[(unsigned char)r] | DAT_002bc490[(unsigned char)g] | DAT_002bd640[(unsigned char)b] |
		DAT_002bca94;
}

/* 0x001afa50  Rowing (gap)
The debug object's constructor. */
void *FUN_001afa50(void *self)
{
	RA_AT(self, 0, const void *) = vtable_00275bc4;
	return self;
}

/* 0x001afa60  Rowing (gap)
Loads the debug font; nothing queued. PACKAGE is not read (the font comes from the global
package). Returns 1 when the font loaded. */
int FUN_001afa60(void *self, void *package)
{
	(void)package;
	RA_I32(self, 0x8) = 0;
	RA_I32(self, 0xc) = 0;
	RA_PTRF(self, 0x4) = bLoadResource(NULL, "handgotb.ttf", 3, 0xdefa);
	return RA_PTRF(self, 0x4) != NULL;
}

/* 0x001afaa0  Rowing (gap)
Prints FORMAT at (X, Y) in colour (R, G, B, A) with the debug font, in screen space. */
void FUN_001afaa0(void *self, float x, float y, int r, int g, int b, int a, const char *format, ...)
{
	char text[0x400];
	unsigned short wide[0x400];
	va_list args;

	va_start(args, format);
	vsprintf(text, format, args);
	va_end(args);
	bdSetWorldMatrix((float *)DAT_002855f0);
	bdSetProjectionMode(0);
	bdSetRenderState(1, 1, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bkStringToWide(wide, text);
	bdPrintFontLite((struct bdFont *)RA_PTRF(self, 0x4), wide, (int)strlen(text), x, y, r, g, b, a, NULL, NULL);
	bdSetProjectionMode(1);
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(6, 0, 0);
}

/* 0x001afba0  Rowing (gap)
Draws a ring of RADIUS round POS (20 segments, in the x-z plane) in (R, G, B). */
void FUN_001afba0(float *pos, float radius, int r, int g, int b)
{
	float *v = (float *)RW_ADDR(0x0030f770);
	unsigned int colour;
	float a;
	int i;

	bdSetTexture(0, NULL);
	bdSetWorldMatrix((float *)DAT_002855f0);
	colour = ra_Colour(r, g, b);
	for (i = 0; i < 20; i++) {
		a = (float)i * 0.0500000007f * 6.28318548f;
		v[i * 9] = sinf(a) * radius + pos[0];
		v[i * 9 + 1] = pos[1];
		v[i * 9 + 2] = cosf(a) * radius + pos[2];
		RA_U32(v, i * 0x24 + 0x18) = colour;
	}
	memcpy(v + 20 * 9, v, 0x24);
	bdDrawPrimitiveFVF(2, v, 0x15, 0x152);
}

/* 0x001afc80  Rowing (gap)
Draws a line from FROM to TO in (R, G, B). */
void FUN_001afc80(float *from, float *to, int r, int g, int b)
{
	float *v = (float *)RW_ADDR(0x0030fa68);
	unsigned int colour;

	bdSetTexture(0, NULL);
	bdSetWorldMatrix((float *)DAT_002855f0);
	v[0] = from[0];
	v[1] = from[1];
	v[2] = from[2];
	colour = ra_Colour(r, g, b);
	RA_U32(v, 0x18) = colour;
	v[9] = to[0];
	v[10] = to[1];
	v[11] = to[2];
	RA_U32(v, 0x3c) = colour;
	bdDrawPrimitiveFVF(1, v, 2, 0x152);
}

/* 0x001afd20  Rowing (gap)
Draws the queued texts (grey, from y 240 down in steps of 20) and lines, and empties the
queues. */
void FUN_001afd20(void *self)
{
	float y = 240.0f;
	char *line;
	int i;

	for (i = 0; i < RA_I32(self, 0x8); i++) {
		/* the original passes the text itself as the format */
		FUN_001afaa0(self, 0.0f, y, 0x7f, 0x7f, 0x7f, 0x40, (const char *)self + 0x10 + i * 500);
		y -= 20.0f;
	}
	for (i = 0; i < RA_I32(self, 0xc); i++) {
		line = (char *)self + 0x2720 + i * 0x24;
		FUN_001afc80((float *)line, (float *)(line + 0x10), (unsigned char)line[0x20], (unsigned char)line[0x21],
			(unsigned char)line[0x22]);
	}
	RA_I32(self, 0x8) = 0;
	RA_I32(self, 0xc) = 0;
}

/* 0x001afdc0  Rowing (gap)
Fills the screen rectangle (X, Y, W, H) with COLOUR (0xAARRGGBB; blended unless opaque). */
void FUN_001afdc0(float x, float y, float w, float h, unsigned int colour)
{
	float *v = (float *)RW_ADDR(0x0030fab0);
	unsigned int a = colour >> 24;

	bdSetProjectionMode(0);
	bdSetTexture(0, NULL);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(5, 0, 0);
	if (a == 0xff) {
		bdSetRenderState(6, 0, 0);
	} else {
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 0, 0);
	}
	bdSetWorldMatrix((float *)DAT_002855f0);
	v[0] = x;
	v[1] = y;
	v[9] = x + w;
	v[10] = y;
	v[18] = x;
	v[19] = y + h;
	v[27] = x + w;
	v[28] = y + h;
	v[2] = 0.0f;
	v[11] = 0.0f;
	v[20] = 0.0f;
	v[29] = 0.0f;
	colour = DAT_002bd640[colour & 0xff] | DAT_002bc490[(colour >> 8) & 0xff] | DAT_002bce18[(colour >> 16) & 0xff] |
		DAT_002bc898[a];
	RA_U32(v, 0x18) = colour;
	RA_U32(v, 0x3c) = colour;
	RA_U32(v, 0x60) = colour;
	RA_U32(v, 0x84) = colour;
	bdDrawPrimitiveFVF(4, v, 4, 0x152);
	bdSetRenderState(6, 0, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(5, 1, 0);
}

/* 0x001aff20  Rowing (gap) (vtable 0x00275bc4 slot 0)
The debug object's deleting destructor. */
void *FUN_001aff20(void *self, unsigned int flags)
{
	RA_AT(self, 0, const void *) = vtable_00275bc4;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}
