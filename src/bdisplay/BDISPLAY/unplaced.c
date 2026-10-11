/*
UNPLACED.C  <-  C:\Babel\Xbox\Src\BDISPLAY\ (file not known)

bdisplay functions whose original source file is not known: the text
layout (bPrintFontVerts), the render-state stack, the error names, the
2D blit of a linear texture, and five functions at the end of the module
that work on the debug GUI's data (0x00047040-0x00047300; probably the
start of C:\Babel\Xbox\Src\BGUI\gui.cpp, but the function table puts
them in bdisplay).
*/

#include "bdisplay_internal.h"

/* 0x000459b0  BDISPLAY/(unplaced): lays out length characters of text in
the box format and writes four bdVertex per character to vertices. The
characters go to DAT_002bef00 (at most 0x400), the lines to lines.
Escapes: "@@" and "\\\\" print '@' and '\\'; "\\n" starts a new line;
"@[name]" draws the texture resource name inline; "@(text)" passes text
to the callback (reason 1). Returns the number of lines. */
int bPrintFontVerts(float *vertices, bdPrintLine *lines, bdFont *font, const unsigned short *text, int length,
	bdPrintFormat *format, int r, int g, int b, int a, void *callback, void *context)
{
	bdPrintCallback print = (bdPrintCallback)callback;
	bdPrintCallbackInfo info;
	char name[64];
	char *p;
	bdPrintChar *record, *ch;
	bdPrintLine *line;
	const bdGlyph *glyph;
	bdTexture *picture;
	unsigned short c, next;
	float scaleU, scaleV, halfU, halfV, f;
	float xOffset = 0.0f, yOffset = 0.0f, total, textHeight, x, y, yy, add;
	int i, j, k, numChars, numLines, spaces;
	float *v;
	DWORD colour;

	if (0x400 < length)
	{
		length = 0x400;
		bdPrintf("bdPrintFont: *** WARNING : String length truncated to %d chars ***\n", 0x400);
	}
	scaleU = 1.0f / (float)font->texture->width;
	record = DAT_002bef00;
	info.chars = DAT_002bef00;
	info.context = context;
	scaleV = 1.0f / (float)font->texture->height;
	halfU = scaleU * 0.5f;
	halfV = scaleV * 0.5f;

	/* the characters */
	for (i = 0; i < length; i++)
	{
		c = text[i];
		if (c == '@' && i < length - 1 && text[i + 1] != '@')
		{
			next = text[i + 1];
			i++;
			if (next == '[')
			{
				p = name;
				for (i++; i < length && text[i] != ']'; i++)
					*p++ = (char)text[i];
				*p = 0;
				record->code = BD_PRINT_CODE_PICTURE;
				picture = (bdTexture *)FUN_0004bb30(0, name, 0, 0xdefa);
				record->texture = picture;
				if (picture != NULL)
				{
					record->advance = (float)picture->width;
					record->height = (float)picture->height;
				}
				else
				{
					record->advance = (float)font->glyphs['W' - BDFONT_FIRST_CHAR].width;
					record->height = font->height;
				}
				record->u[3] = 1.0f;
				record->u[2] = 1.0f;
				record->v[2] = 1.0f;
				record->v[1] = 1.0f;
				record->u[1] = 0.0f;
				record->u[0] = 0.0f;
				record->v[3] = 0.0f;
				record->v[0] = 0.0f;
				if (print != NULL)
				{
					info.record = record;
					info.text = name;
					if (print(2, &info) == 0)
						record--;
				}
				record++;
			}
			else if (next == '(')
			{
				p = name;
				for (i++; i < length && text[i] != ')'; i++)
					*p++ = (char)text[i];
				*p = 0;
				record->code = BD_PRINT_CODE_PICTURE;
				if (print != NULL)
				{
					info.record = record;
					info.text = name;
					if (print(1, &info) != 0)
						record++;
				}
			}
			else
			{
				bdPrintf("Unknown escape sequence '%c'\n", next);
			}
		}
		else if (c == '\\' && i < length - 1 && text[i + 1] != '\\')
		{
			if (text[i + 1] == 'n')
			{
				record->code = BD_PRINT_CODE_NEWLINE;
				record->advance = 0.0f;
				record->height = 0.0f;
				record++;
			}
			i++;
		}
		else
		{
			if (c == '@' || c == '\\')
				i++;
			record->code = text[i] - BDFONT_FIRST_CHAR;
			record->texture = font->texture;
			glyph = &font->glyphs[record->code];
			if (glyph->width == 0)
			{
				bdPrintf("bPrintFontVerts: glyph 0x%04x '%c' not present in font [at = %d]\n", text[i], text[i], i);
				glyph = &font->glyphs[BDFONT_MISSING_GLYPH];
			}
			record->advance = (float)glyph->width;
			record->height = font->height;
			f = (float)glyph->u * scaleU + halfU;
			record->u[1] = f;
			record->u[0] = f;
			f = (float)glyph->width * scaleU + f;
			record->u[3] = f;
			record->u[2] = f;
			f = (float)glyph->v * scaleV + halfV;
			record->v[3] = f;
			record->v[0] = f;
			f = (float)glyph->height * scaleV + f;
			record->v[2] = f;
			record->v[1] = f;
			record++;
		}
	}
	numChars = (int)(record - DAT_002bef00);
	if (numChars == 0)
		return 0;

	/* the lines */
	if (format->wrap != 0)
	{
		line = lines;
		line->chars = DAT_002bef00;
		line->numChars = 0;
		line->width = 0.0f;
		numLines = 1;
		i = 0;
		if (0 < numChars)
		{
			do
			{
				while (i < numChars)
				{
					if (DAT_002bef00[i].code == BD_PRINT_CODE_NEWLINE)
						break;
					if (!(line->width <= format->width))
						break;
					line->numChars++;
					line->width = line->width + DAT_002bef00[i].advance;
					i++;
				}
				if (line->width > format->width)
				{
					/* too wide: break at the line's last space */
					for (j = line->numChars - 1; 0 <= j; j--)
					{
						if (line->chars[j].code == 0)
						{
							line->numChars = j;
							i = (int)(line->chars - DAT_002bef00) + j + 1;
							break;
						}
					}
				}
				else
				{
					if (numChars <= i)
						break;
					if (DAT_002bef00[i].code != BD_PRINT_CODE_NEWLINE)
						continue;
					i++;
				}
				numLines++;
				line++;
				line->chars = &DAT_002bef00[i];
				line->numChars = 0;
				line->width = 0.0f;
			} while (i < numChars);
		}
	}
	else
	{
		line = lines;
		lines[0].chars = DAT_002bef00;
		lines[0].numChars = 0;
		numLines = 0;
		for (k = 0; k < numChars; k++)
		{
			if (lines[0].chars[k].code == BD_PRINT_CODE_NEWLINE)
			{
				numLines++;
				line++;
				line->chars = &DAT_002bef00[k + 1];
				line->numChars = 0;
			}
			else
			{
				line->numChars++;
			}
		}
		numLines++;
	}

	/* line sizes and the vertical placement */
	total = 0.0f;
	for (i = 0; i < numLines; i++)
	{
		line = &lines[i];
		line->width = 0.0f;
		line->height = font->height;
		for (k = 0; k < line->numChars; k++)
		{
			if (line->height < line->chars[k].height)
				line->height = line->chars[k].height;
			line->width = line->chars[k].advance + line->width;
		}
		total = total + line->height;
	}
	textHeight = (float)(numLines - 1) * format->lineSpacing + total;
	switch (format->vAlign)
	{
	case 1:
		yOffset = 0.0f;
		break;
	case 2:
		yOffset = (format->height - textHeight) * 0.5f;
		break;
	case 4:
		yOffset = format->height - textHeight;
		break;
	default:
		break;
	}
	y = format->y - yOffset;
	info.vertices = vertices;
	v = vertices;

	/* the vertices */
	for (i = 0; i < numLines; i++)
	{
		line = &lines[i];
		switch (format->hAlign)
		{
		case 0:
			xOffset = 0.0f;
			break;
		case 2:
			xOffset = (format->width - line->width) * 0.5f;
			break;
		case 3:
			xOffset = format->width - line->width;
			break;
		case 5:
			/* justified: the free width goes to the spaces */
			xOffset = 0.0f;
			if (0 < line->numChars)
			{
				spaces = 0;
				for (k = 0; k < line->numChars; k++)
					spaces += line->chars[k].code == 0;
				if (1 < spaces)
				{
					add = (format->width - line->width) / (float)spaces;
					for (k = 0; k < line->numChars; k++)
					{
						if (line->chars[k].code == 0)
							line->chars[k].advance = add + line->chars[k].advance;
					}
				}
			}
			break;
		default:
			/* 1, 4: the previous line's offset */
			break;
		}
		info.line = i;
		x = xOffset + format->x;
		for (k = 0; k < line->numChars; k++)
		{
			ch = &line->chars[k];
			if (ch->code != BD_PRINT_CODE_NEWLINE)
			{
				yy = y - (line->height - ch->height) * 0.5f;
				colour = DAT_002bc898[a] | DAT_002bd640[b] | DAT_002bce18[r] | DAT_002bc490[g];
				v[0] = x;
				v[1] = yy - ch->height;
				v[2] = 0.0f;
				v[7] = ch->u[1];
				v[8] = ch->v[1];
				((DWORD *)v)[6] = colour;
				v[10] = yy;
				v[9] = x;
				v[11] = 0.0f;
				v[16] = ch->u[0];
				v[17] = ch->v[0];
				((DWORD *)v)[15] = colour;
				v[18] = x + ch->advance;
				v[19] = yy - ch->height;
				v[20] = 0.0f;
				v[25] = ch->u[2];
				v[26] = ch->v[2];
				((DWORD *)v)[24] = colour;
				v[27] = x + ch->advance;
				v[28] = yy;
				v[29] = 0.0f;
				v[34] = ch->u[3];
				v[35] = ch->v[3];
				((DWORD *)v)[33] = colour;
				v += 36;
				if (print != NULL)
				{
					info.current = v - 36;
					info.index = k;
					print(0, &info);
				}
			}
			x = x + ch->advance;
		}
		y = y - (line->height + format->lineSpacing);
	}
	return numLines;
}

/* 0x00046320  BDISPLAY/(unplaced): width and height of a string printed
on one line (top aligned, no wrap) */
void bdGetStringSize(bdFont *font, const unsigned short *text, int length, float *width, float *height)
{
	bdPrintFormat format;
	float rect[4];

	format.hAlign = 0;
	format.width = 0.0f;
	format.height = 0.0f;
	format.x = 0.0f;
	format.y = 0.0f;
	format.wrap = 0;
	format.lineSpacing = 0.0f;
	format.vAlign = 1;
	bdGetTextExtent(rect, font, text, length, &format, NULL, NULL);
	*width = rect[2] - rect[0];
	*height = rect[3] - rect[1];
}

/* 0x00046c90  BDISPLAY/(unplaced) */
unsigned int bdGetRenderState(int state, unsigned int *extra)
{
	if (extra != NULL)
		*extra = DAT_002be600[state * 2 + 1];
	return DAT_002be600[state * 2];
}

/* 0x00046cb0  BDISPLAY/(unplaced): saves the state's value on its stack
(16 deep; deeper pushes are counted but not saved), then sets it */
void bdPushRenderState(int state, unsigned int value, unsigned int extra)
{
	int depth = DAT_002bedf8[state];

	if (-1 < depth && depth < 0xf)
	{
		depth = state * BD_STATE_STACK + depth;
		DAT_002be678[depth * 2] = DAT_002be600[state * 2];
		DAT_002be678[depth * 2 + 1] = DAT_002be600[state * 2 + 1];
	}
	DAT_002bedf8[state]++;
	bdSetRenderState(state, value, extra);
}

/* 0x00046d00  BDISPLAY/(unplaced) */
void bdPopRenderState(int state)
{
	int depth = DAT_002bedf8[state] - 1;

	DAT_002bedf8[state] = depth;
	if (-1 < depth && depth < 0xf)
	{
		depth = (state * BD_STATE_STACK + depth) * 2;
		bdSetRenderState(state, DAT_002be678[depth], DAT_002be678[depth + 1]);
	}
}

/* 0x00046d40  BDISPLAY/(unplaced): copies the 15 states (value, extra) */
void bdSaveRenderStates(unsigned int *saved)
{
	int i;

	for (i = 0; i < BD_NUM_STATES * 2; i++)
		saved[i] = DAT_002be600[i];
}

/* 0x00046d60  BDISPLAY/(unplaced): sets the states that differ */
void bdRestoreRenderStates(unsigned int *saved)
{
	int state = 0;

	do
	{
		if (saved[0] != DAT_002be600[state * 2] || saved[1] != DAT_002be600[state * 2 + 1])
			bdSetRenderState(state, saved[0], saved[1]);
		state++;
		saved += 2;
	} while (state < 0xf);
}

/* 0x00046da0  BDISPLAY/(unplaced) */
undefined4 FUN_00046da0(undefined4 value)
{
	return value;
}

/* 0x00046db0  BDISPLAY/(unplaced): a jump to bdLoadTexture */
struct bdTexture *bdLoadTexture2(void *package, unsigned int crc)
{
	return bdLoadTexture(package, crc);
}

/* 0x00046dc0  BDISPLAY/(unplaced): a jump to bdTextureDelete (bKernel's
destructor of resource type 5) */
void bdTextureDelete2(struct bdTexture *texture)
{
	bdTextureDelete(texture);
}

/* 0x00046dd0  BDISPLAY/(unplaced): draws a linear 640 x 448 texture as a
screen rectangle of its size, at x, y (pixels from the top left) or
centred (x -1). Begins and ends a scene if none is open. The vertices'
normals are not written by the original (zero here). */
void bdDrawLinearTexture(bdTexture *texture, int x, int y)
{
	bdVertex quad[4];
	float left, top, right, bottom;
	int began, xx, yy;
	DWORD colour;

	if (DAT_002bc468 == 0)
	{
		if (bdBeginScene() == 0)
			return;
		began = 1;
	}
	else
	{
		began = 0;
	}
	if (x == -1)
	{
		xx = -(int)((unsigned int)texture->width >> 1);
		yy = (int)((unsigned int)texture->height >> 1);
	}
	else
	{
		xx = x - DAT_002bc414;
		yy = DAT_002bc418 - y;
	}
	left = (float)xx;
	top = (float)yy;
	right = (float)texture->width + left;
	bottom = top - (float)texture->height;
	bdPushRenderState(2, 0xc, 0);
	bdSetProjectionMode(0);
	bdSetWorldMatrix(DAT_002855f0);
	memset(quad, 0, sizeof(quad));
	colour = DAT_002bc898[127] | DAT_002bd640[127] | DAT_002bc490[127] | DAT_002bce18[127];
	quad[0].x = left;
	quad[0].y = top;
	quad[0].z = 1.0f;
	quad[0].color = colour;
	quad[0].u = 0.0f;
	quad[0].v = 0.0f;
	quad[1].x = left;
	quad[1].y = bottom;
	quad[1].z = 1.0f;
	quad[1].color = colour;
	quad[1].u = 0.0f;
	quad[1].v = 448.0f;
	quad[2].x = right;
	quad[2].y = top;
	quad[2].z = 1.0f;
	quad[2].color = colour;
	quad[2].u = 640.0f;
	quad[2].v = 0.0f;
	quad[3].x = right;
	quad[3].y = bottom;
	quad[3].z = 1.0f;
	quad[3].color = colour;
	quad[3].u = 640.0f;
	quad[3].v = 448.0f;
	bdPushRenderState(7, 1, 0);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(4, 0, 0);
	bdSetTexture(0, texture);
	bdDrawPrimitive(BDPRIMTYPE_TRISTRIP, quad, 4);
	bdSetTexture(0, NULL);
	bdSetProjectionMode(1);
	bdPopRenderState(7);
	bdPopRenderState(3);
	bdPopRenderState(4);
	bdPopRenderState(2);
	if (began)
		bdEndScene();
}

/* 0x00047010  BDISPLAY/(unplaced): the name of a D3D or DirectSound error */
const char *bdGetErrorString(int result)
{
	int i = 0;

	if (0 < DAT_00284e20)
	{
		do
		{
			if (result == DAT_00284d48[i].code)
				return DAT_00284d48[i].name;
			i++;
		} while (i < DAT_00284e20);
	}
	return "Unknown Error";
}

/* calls the first virtual function of a C++ object (thiscall, no arguments) */
static void call_first_virtual(void **object)
{
	((void (*)(void *))((void **)object[0])[0])(object);
}

/* 0x00047040  BDISPLAY/(unplaced): bdEndScene's callback: the first
virtual function of each of the three objects at 0x002caf08 */
void FUN_00047040(void)
{
	if (DAT_002caf08 != NULL)
		call_first_virtual(DAT_002caf08);
	if (DAT_002caf0c != NULL)
		call_first_virtual(DAT_002caf0c);
	if (DAT_002caf10 != NULL)
		call_first_virtual(DAT_002caf10);
}

/* 0x00047070  BDISPLAY/(unplaced), probably BGUI/gui.cpp: initialises the
debug GUI block (owner engine_misc): colours, the input map with its
channels, the empty menu list. The original clears 0x002caf18-0x002cb55c
in one loop; engine_misc keeps those symbols in one block. */
int FUN_00047070(void)
{
	BYTE *block = (BYTE *)&DAT_002caf18;

	/* the block is larger than DAT_002caf18's declared type: hide that from the optimiser */
	__asm__("" : "+r"(block));
	memset(block, 0, 0x191 * 4);
	DAT_002caf20 = 0xff;
	DAT_002caf1c = 0xff;
	DAT_002caf18 = 0xff;
	DAT_002caf2c = 0;
	DAT_002caf28 = 0;
	DAT_002caf24 = 0;
	DAT_002caf30 = 0;
	DAT_002caf34 = 0;
	DAT_002caf38 = 0;
	DAT_002caf3c = 0;
	DAT_002caf40 = 0xb4;
	DAT_002caf44 = 0x8c;
	DAT_002caf48 = 100;
	DAT_002caf4c = 0;
	DAT_002caf50 = 0;
	DAT_002caf54 = 0;
	DAT_002caf58 = 0x73;
	DAT_002caf5c = 0xbd;
	DAT_002caf60 = 0xb7;
	DAT_002caf64 = 0xff;
	DAT_002caf68 = 0xff;
	DAT_002caf6c = 0xff;
	DAT_002caf70 = 0;
	DAT_002caf74 = 0;
	DAT_002caf78 = 0x7f;
	DAT_002caf7c = (struct BInputMap *)FUN_000488c0(9);
	DAT_002caf80 = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Left", 1, 4, 0, 0, 0);
	DAT_002caf84 = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Right", 1, 4, 0, 0, 0);
	DAT_002caf88 = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Up", 1, 4, 0, 0, 0);
	DAT_002caf8c = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Down", 1, 4, 0, 0, 0);
	DAT_002caf90 = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Select", 1, 4, 0, 0, 0);
	DAT_002caf94 = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Deselect", 1, 1, 0, 0, 0);
	DAT_002caf9c = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Prior", 1, 4, 0, 0, 0);
	DAT_002caf98 = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Next", 1, 4, 0, 0, 0);
	DAT_002cafa0 = (struct BInputChannel *)FUN_00048a10((struct biMap *)DAT_002caf7c, "Exit", 1, 1, 0, 0, 0);
	DAT_002cb540 = &DAT_002cafa4;
	DAT_002cb53c = &DAT_002cafa4;
	DAT_002cb548 = NULL;
	DAT_002cb54c = NULL;
	FUN_00047e30();
	DAT_002cb550 = 0;
	DAT_002cb554 = 0;
	return 1;
}

/* 0x00047280  BDISPLAY/(unplaced): adds add to r, g, b, clamped to 0-255
(b is only clamped above when it is not negative, as in the original) */
void FUN_00047280(int r, int g, int b, int *outR, int *outG, int *outB, int add)
{
	r = r + add;
	g = g + add;
	b = b + add;
	if (r < 0)
		r = 0;
	else if (0xff < r)
		r = 0xff;
	if (g < 0)
		g = 0;
	else if (0xff < g)
		g = 0xff;
	if (b < 0)
	{
		*outR = r;
		*outG = g;
		*outB = 0;
		return;
	}
	if (0xff < b)
		b = 0xff;
	*outR = r;
	*outG = g;
	*outB = b;
}

/* 0x00047300  BDISPLAY/(unplaced), probably BGUI/gui.cpp: activates the
debug GUI (if it has menus and is not active) with callback; the GUI takes
over the input */
void FUN_00047300(void *callback)
{
	BYTE *menu;

	if (DAT_002cb53c != &DAT_002cafa4 && DAT_002cb550 == 0)
	{
		DAT_002cb558 = (int (*)(struct BGuiEvent *))callback;
		DAT_002cc1b4 = DAT_002caf7c;
		DAT_002cb550 = 1;
		FUN_00048d40((struct biChannel *)DAT_002cafa0);
		if (DAT_002cb548 != NULL)
		{
			DAT_002cb54c = DAT_002cb548;
			menu = (BYTE *)DAT_002cb548;
			*(unsigned int *)(menu + 4) |= 4;
			if (*(int *)(menu + 0xc) != 0)
				*(int *)(menu + 0x594) = 0;
		}
	}
}
