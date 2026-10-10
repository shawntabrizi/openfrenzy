/*
XBFONT.C  <-  C:\Babel\Xbox\Src\BDISPLAY\xbFont.cpp

Bitmap fonts: the vertex ring the printers write into, loading and
deleting fonts, and the printers that draw laid-out text (bdPrintFont)
or one plain line (PrintFontLite). 0x00042e10-0x00043480. Only bdFontInit
carries the file's assert tag; the rest follows it and works on fonts
(inferred). The layout itself, bPrintFontVerts (0x000459b0), is in
unplaced.c.

Strings are 16-bit characters. Vertices are bdVertex (FVF 0x152, drawn
with the 2D projection); a character is four of them, drawn as a strip.
*/

#include "bdisplay_internal.h"

#define FONT_FILE "C:\\Babel\\Xbox\\Src\\BDISPLAY\\xbFont.cpp"

/* the ring of font vertices: 0x1000 bdVertex */
#define FONT_RING_VERTICES 0x1000

/* 0x00042e10  BDISPLAY/xbFont.cpp */
void bdFontInit(void)
{
	DAT_002be264 = bdAlloc(0x24e10, FONT_FILE, 0x2b, 0x2001, "Font Print Vertices", 0);
}

/* 0x00042e40  BDISPLAY/xbFont.cpp (inferred) */
void bdFontShutdown(void)
{
	if (DAT_002be264 != NULL)
	{
		bdFree(DAT_002be264);
		DAT_002be264 = NULL;
	}
}

/* 0x00042e60  BDISPLAY/xbFont.cpp (inferred): loads a font from a package;
its texture is embedded (0x780) and marked as not freeable */
bdFont *bdLoadFont(void *package, unsigned int crc)
{
	bdFont *font = (bdFont *)bdFindResource(package, crc);
	DWORD *resource;

	if (font == NULL)
		return NULL;
	font->texture = &font->fontTexture;
	bdTextureSetup(font->texture);
	resource = (DWORD *)font->texture->resource;
	resource[5] = 0;
	resource[4] = 0;
	resource[0] |= 0x80000000u;
	return font;
}

/* 0x00042ec0  BDISPLAY/xbFont.cpp (inferred): the font resource's
destructor (bKernel's resource type 3) */
void bdFontDelete(bdFont *font)
{
	bdTextureDelete(font->texture);
	if (-1 < *(int *)font)
		bdFree(font);
}

/* takes count characters' worth of vertices from the ring */
static bdVertex *font_vertices(int numChars)
{
	bdVertex *vertices;

	if (FONT_RING_VERTICES - DAT_002be268 < numChars * 4)
		DAT_002be268 = 0;
	vertices = (bdVertex *)DAT_002be264 + DAT_002be268;
	DAT_002be268 += numChars * 4;
	return vertices;
}

/* 0x00042ef0  BDISPLAY/xbFont.cpp (inferred): the bounding rectangle
(min x, min y, max x, max y) of the laid-out string; 0 if nothing is
drawn. Ghidra lists six arguments; the seventh (context) is used. */
int bdGetTextExtent(float *rect, bdFont *font, const unsigned short *text, int length,
	bdPrintFormat *format, void *callback, void *context)
{
	bdPrintLine lines[64];
	bdVertex *vertices, *v;
	int numLines, line, count;

	vertices = font_vertices(length);
	numLines = bPrintFontVerts((float *)vertices, lines, font, text, length, format, 0, 0, 0, 0, callback,
		context);
	rect[3] = -3.4028235e+38f;
	rect[2] = -3.4028235e+38f;
	rect[1] = 3.4028235e+38f;
	rect[0] = 3.4028235e+38f;
	v = vertices;
	for (line = 0; line < numLines; line++)
	{
		for (count = lines[line].numChars << 2; count != 0; count--)
		{
			if (v->x < rect[0])
				rect[0] = v->x;
			if (rect[2] < v->x)
				rect[2] = v->x;
			if (v->y < rect[1])
				rect[1] = v->y;
			if (rect[3] < v->y)
				rect[3] = v->y;
			v++;
		}
	}
	return v != vertices;
}

/* 0x00043010  BDISPLAY/xbFont.cpp (inferred): lays out and draws a string
in a box (the original takes the length in EDX) */
void bdPrintFontBox(bdFont *font, const unsigned short *text, int length, bdPrintFormat *format, int r,
	int g, int b, int a, void *callback, void *context)
{
	bdPrintLine lines[64];
	bdVertex *vertices;
	bdTexture *selected = (bdTexture *)1;
	bdPrintChar *ch;
	int numLines, line, i, vertex;

	vertices = font_vertices(length);
	numLines = bPrintFontVerts((float *)vertices, lines, font, text, length, format, r, g, b, a, callback,
		context);
	vertex = 0;
	for (line = 0; line < numLines; line++)
	{
		for (i = 0; i < lines[line].numChars; i++)
		{
			ch = &lines[line].chars[i];
			if (ch->code != BD_PRINT_CODE_NEWLINE)
			{
				if ((bdTexture *)ch->texture != selected)
				{
					bdSetTexture(0, (bdTexture *)ch->texture);
					selected = (bdTexture *)ch->texture;
				}
				bdDrawPrimitive(BDPRIMTYPE_TRISTRIP, vertices + vertex, 4);
			}
			vertex += 4;
		}
	}
}

/* 0x00043140  BDISPLAY/xbFont.cpp (inferred): one line of text at x, y
(the original takes the font in EBX); no escapes, no layout */
void PrintFontLite(bdFont *font, const unsigned short *text, int length, float x, float y, int r, int g,
	int b, int a, void *callback, void *context)
{
	bdPrintCallbackInfo info;
	bdVertex *vertices, *v;
	const bdGlyph *glyph;
	float scaleU, scaleV, u0, u1, v0, v1;
	unsigned int c;
	int i;
	DWORD colour;

	scaleU = 1.0f / (float)font->texture->width;
	scaleV = 1.0f / (float)font->texture->height;
	vertices = font_vertices(length);
	info.context = context;
	info.line = 0;
	info.vertices = (float *)vertices;
	v = vertices;
	for (i = 0; i < length; i++)
	{
		c = text[i];
		glyph = &font->glyphs[c - BDFONT_FIRST_CHAR];
		if (glyph->width == 0)
		{
			bdPrintf("PrintFontLite: glyph 0x%04x '%c' not present in font [at = %d]\n", c, c, i);
			glyph = &font->glyphs[BDFONT_MISSING_GLYPH];
		}
		colour = DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bc898[a] | DAT_002bd640[b];
		u0 = (float)glyph->u * scaleU;
		u1 = (float)glyph->width * scaleU + u0;
		v0 = (float)glyph->v * scaleV;
		v1 = (float)glyph->height * scaleV + v0;
		v[0].x = x;
		v[0].z = 0.0f;
		v[0].y = y - font->height;
		v[0].u = u0;
		v[0].v = v1;
		v[0].color = colour;
		v[1].u = u0;
		v[1].x = x;
		v[1].v = v0;
		v[1].y = y;
		v[1].z = 0.0f;
		v[1].color = colour;
		v[2].x = (float)glyph->width + x;
		v[2].z = 0.0f;
		v[2].y = y - font->height;
		v[2].u = u1;
		v[2].v = v1;
		v[2].color = colour;
		v[3].y = y;
		v[3].z = 0.0f;
		v[3].x = (float)glyph->width + x;
		v[3].u = u1;
		v[3].v = v0;
		v[3].color = colour;
		if (callback != NULL)
		{
			info.index = i;
			info.current = (float *)v;
			((bdPrintCallback)callback)(0, &info);
		}
		x = (float)glyph->width + x;
		v += 4;
	}
	bdSetTexture(0, font->texture);
	bdDrawPrimitive(BDPRIMTYPE_QUADLIST, vertices, length * 4);
}

/* 0x00043400  BDISPLAY/xbFont.cpp (inferred) */
void bdPrintFont(bdFont *font, const unsigned short *text, int length, bdPrintFormat *format, int r, int g,
	int b, int a, void *callback, void *context)
{
	bdPrintFontBox(font, text, length, format, r, g, b, a, callback, context);
}

/* 0x00043440  BDISPLAY/xbFont.cpp (inferred) */
void bdPrintFontLite(bdFont *font, const unsigned short *text, int length, float x, float y, int r, int g,
	int b, int a, void *callback, void *context)
{
	PrintFontLite(font, text, length, x, y, r, g, b, a, callback, context);
}
