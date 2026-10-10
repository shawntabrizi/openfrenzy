/*
Core/BG_Effects.c  <-  C:\BlitzParty\Game\Src\Core\BG_Effects.cpp
(and BG_DrawRect at 0x00011000, the function before it)

Background effects of the front end: a scrolling grid of random characters
(CBGMatrixText), lines of text (CBGText, CBGTextBuf), text that types
itself (CBGTypeText, CBGTypeTextBox) and the scrolling tiled backdrop
(CBGTilingBackdrop). The effects live in the list of the BGEffects game
mode (Core/CGameMode_AskAgain.c). Also the C library rand() copies the
game uses (Core_Rand, Core_RandUnit).
*/

#include "core_modes_a.h"
#include "core_modes_a_modes.h"

OF_ASSERT_SIZE(struct CBGEffect, 0x10);
OF_ASSERT_SIZE(struct CBGMatrixText, 0x48);
OF_ASSERT_SIZE(struct CBGText, 0x3c);
OF_ASSERT_SIZE(struct CBGTextBuf, 0x44);
OF_ASSERT_SIZE(struct CBGTypeText, 0x90);       /* new(0x90) */
OF_ASSERT_SIZE(struct CBGTypeTextBox, 0xb0);    /* new(0xb0) */
OF_ASSERT_SIZE(struct CBGTilingBackdrop, 0x24);
OF_ASSERT_SIZE(struct CMA_TextLayout, 0x20);
OF_ASSERT_SIZE(struct CMA_Vertex, 0x24);

/* ---------- data */

/* 0x002b9480: the four vertices of BG_DrawRect */
static struct CMA_Vertex DAT_002b9480[4];
/* 0x002b9510: the four vertices of the tiling backdrop */
static struct CMA_Vertex DAT_002b9510[4];

/* 0x002818e0: characters of the glitching matrix text */
static const wchar16 DAT_002818e0[83] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
	'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '}', '[', ']', '_', '=',
	'+', '*', '(', ')', '!', ',', '<', '>', '|', '?', ';', ':', '\'', '/', '#',
};

/* 0x0028450c: default colour of new text effects (r, g, b, a) */
int DAT_0028450c[4] = { 0xff, 0xff, 0xff, 0x28 };
/* 0x0028451c: colour of grey text (CBGTypeText_SetGreyStyle) */
int DAT_0028451c[4] = { 0x7f, 0x7f, 0x7f, 0xff };

static const char s_BG_Effects_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\BG_Effects.cpp";

/* ---------- vtables */

/* 0x00253744 CBGEffect */
const void *const vtable_00253744[] = {
	(const void *)CBGEffect_Delete,
	(const void *)FUN_001d5950, /* Init: nothing */
	(const void *)FUN_000a6fa0, /* Update: return 1 */
	(const void *)FUN_001d5950, /* Render: nothing */
	(const void *)FUN_000135c0, /* Show */
	(const void *)FUN_000135d0, /* Hide */
};

/* 0x00253788 CBGMatrixText */
const void *const vtable_00253788[] = {
	(const void *)CBGMatrixText_Delete,
	(const void *)CBGMatrixText_Init,
	(const void *)CBGMatrixText_Update,
	(const void *)CBGMatrixText_Render,
	(const void *)FUN_000135c0,
	(const void *)FUN_000135d0,
	(const void *)CBGMatrixText_FillLine,
};

/* 0x002537d8 CBGText */
const void *const vtable_002537d8[] = {
	(const void *)CBGText_Delete,
	(const void *)FUN_001d5950,
	(const void *)FUN_000a6fa0,
	(const void *)CBGText_Render,
	(const void *)FUN_000135c0,
	(const void *)FUN_000135d0,
	(const void *)CBGText_SetText,
};

/* 0x002537f4 CBGTextBuf */
const void *const vtable_002537f4[] = {
	(const void *)CBGTextBuf_Delete,
	(const void *)FUN_001d5950,
	(const void *)CBGTextBuf_Update,
	(const void *)CBGText_Render,
	(const void *)FUN_000135c0,
	(const void *)FUN_000135d0,
	(const void *)CBGTextBuf_SetText,
};

/* 0x00253810 CBGTypeText */
const void *const vtable_00253810[] = {
	(const void *)CBGTypeText_Delete,
	(const void *)FUN_001d5950,
	(const void *)CBGTypeText_Update,
	(const void *)CBGTypeText_Render,
	(const void *)FUN_000135c0,
	(const void *)CBGTypeText_Hide,
	(const void *)CBGTextBuf_SetText,
	(const void *)CBGTypeText_Start,
};

/* 0x00253858 CBGTypeTextBox */
const void *const vtable_00253858[] = {
	(const void *)CBGTypeTextBox_Delete,
	(const void *)FUN_001d5950,
	(const void *)CBGTypeText_Update,
	(const void *)CBGTypeTextBox_Render,
	(const void *)FUN_000135c0,
	(const void *)CBGTypeText_Hide,
	(const void *)CBGTextBuf_SetText,
	(const void *)CBGTypeText_Start,
};

/* 0x00253880 CBGTilingBackdrop */
const void *const vtable_00253880[] = {
	(const void *)CBGText_Delete, /* 0x00012760: only resets the base vtable */
	(const void *)CBGTilingBackdrop_Init,
	(const void *)FUN_000a6fa0,
	(const void *)CBGTilingBackdrop_Render,
	(const void *)FUN_000135c0,
	(const void *)FUN_000135d0,
};

/* the render states the text effects set (FUN_00044b00: state, value, stage) */
static void BG_SetTextRenderStates(void)
{
	FUN_00044b00(4, 0, 0);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 0, 0);
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(7, 1, 0);
	FUN_00044b00(8, 1, 0);
}

/* the pulse of selected text: |sin(2 t / f + phase)| (x87 computes it in extended precision) */
static double BG_Pulse(float phase)
{
	long long t = CMA_GetTicks();
	return fabs(sin(((double)t + (double)t) / (double)DAT_002cd7a0 + (double)phase));
}

/* ---------- functions */

/* 0x00011000  (leading function, before Core/BG_Effects.cpp)
   A flat coloured rectangle from (x, y) to (x + w, y - h). The colour came in EAX. */
void BG_DrawRect(float x, float y, float w, float h, int *rgba)
{
	struct CMA_Vertex *v = DAT_002b9480;

	v[0].x = x;
	v[0].y = y;
	v[0].z = 0.0f;
	v[0].color = CMA_RGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
	v[1].x = x + w;
	v[1].y = y;
	v[1].z = 0.0f;
	v[1].color = CMA_RGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
	v[2].x = v[1].x;
	v[2].y = y - h;
	v[2].z = 0.0f;
	v[2].color = CMA_RGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
	v[3].x = x;
	v[3].y = v[2].y;
	v[3].z = 0.0f;
	v[3].color = CMA_RGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
	FUN_000416c0(0, 0);
	FUN_0003f2d0(5, (int)v, 4);
}

/* 0x00011160  Core/BG_Effects.cpp */
void CBGMatrixText_Init(struct CBGMatrixText *self)
{
	self->timer = 0.0f;
}

/* 0x00011170  Core/BG_Effects.cpp
   Every interval: move the grid up by `scroll` characters and fill the new characters. */
int CBGMatrixText_Update(struct CBGMatrixText *self)
{
	double t = (double)DAT_0028fda8 + (double)self->timer;
	wchar16 *dst;
	wchar16 *src;
	int n;

	self->timer = (float)t;
	if (!(t > (double)self->interval))
		return 1;
	dst = self->buffer;
	src = dst + self->scroll;
	for (n = self->cols * self->rows - self->scroll; n != 0; n--)
		*dst++ = *src++;
	if ((self->base.flags & 8) == 0) {
		CMA_VFN(self, 0x18, void (*)(struct CBGMatrixText *, wchar16 *, int))(self, dst, self->scroll);
		self->cursor = self->cols * self->rows - 1;
	} else {
		for (n = self->scroll; n != 0; n--)
			*dst++ = ' ';
		self->cursor -= self->scroll;
		if (self->cursor < 0 || self->scroll == 0) {
			self->base.flags = 0x10;
			return 0;
		}
	}
	self->timer = self->timer - self->interval;
	return 1;
}

/* 0x00011210  Core/BG_Effects.cpp
   One row per line, 20 units apart, in the BGEffects mode's first font. The red component is the
   row number in the original (kept). */
void CBGMatrixText_Render(struct CBGMatrixText *self)
{
	float m[16];
	const wchar16 *row;
	int i;

	FUN_00044b00(7, 1, 0);
	FUN_00043b70(0);
	memset(m, 0, sizeof(m));
	m[0] = self->scale;
	m[5] = self->scale;
	m[10] = 1.0f;
	m[12] = self->x;
	m[13] = self->y;
	m[15] = 1.0f;
	FUN_00043930((undefined4 *)m);
	row = self->buffer;
	for (i = 0; i < self->rows; i++) {
		CMA_PrintText(DAT_002b95d0->font1, row, self->cols, 0.0f, (float)i * -20.0f, i,
			self->color[1], self->color[2], self->color[3], 0, 0);
		row += self->cols;
	}
}

/* 0x000112d0  Core/BG_Effects.cpp */
void CBGText_SetText(struct CBGText *self, unsigned short *text)
{
	self->text = text;
	self->source = text;
	self->length = CMA_WStrLen(text);
}

/* 0x000112f0  Core/BG_Effects.cpp */
void CBGText_Render(struct CBGText *self)
{
	float m[16];

	memcpy(m, DAT_002855f0, sizeof(m));
	m[0] = self->scale;
	m[5] = self->scale;
	m[10] = self->scale;
	m[12] = self->x;
	m[13] = self->y;
	FUN_00043930((undefined4 *)m);
	CMA_PrintText(self->font, self->text, self->length, 0.0f, 0.0f, self->color[0], self->color[1],
		self->color[2], self->color[3], 0, 0);
}

/* 0x00011370  Core/BG_Effects.cpp */
int CBGTextBuf_Update(struct CBGTextBuf *self)
{
	self->text.length = CMA_WStrLen(self->buffer);
	return 1;
}

/* 0x00011390  Core/BG_Effects.cpp: copy TEXT into the buffer (truncated to the capacity) */
void CBGTextBuf_SetText(struct CBGTextBuf *self, unsigned short *text)
{
	self->text.source = text;
	self->text.length = CMA_WStrLen(text);
	if (self->text.length >= self->capacity)
		self->text.length = self->capacity - 1;
	memcpy(self->buffer, text, (size_t)self->text.length * 2);
	self->buffer[self->text.length] = 0;
}

/* 0x000113e0  Core/BG_Effects.cpp
   Characters shown for TEXT: "@...]" markup counts as one, and its closing ']' once more (as in
   the original). Reads one character past an empty string (as in the original). */
int BG_CountGlyphs(unsigned short *text)
{
	int count = 0;
	int i = 0;

	for (;;) {
		if (text[i] == '@') {
			unsigned short c;
			do {
				c = text[i + 1];
				i++;
			} while (c != 0 && c != ']');
		} else {
			i++;
		}
		count++;
		if (text[i] == 0)
			return count;
	}
}

/* 0x00011420  Core/BG_Effects.cpp: start typing (vtable slot 7) */
void CBGTypeText_Start(struct CBGTypeText *self, float charDelay, float startDelay, int sound)
{
	float w;
	float h;

	self->beepVolume = 0;
	if (self->style & 0x10) {
		FUN_00046320(self->buf.text.font, (int)self->buf.buffer, self->buf.capacity - 1, &w, &h);
		self->buf.text.x = self->buf.text.x - w;
		self->style &= ~0x10u;
	}
	self->typed = 0;
	self->charTimer = 0.0f;
	self->lines = 0;
	self->revealed = 0;
	self->startDelay = startDelay;
	self->charDelay = charDelay;
	self->typeSound = sound;
	self->voice = -1;
}

/* 0x000114b0  Core/BG_Effects.cpp: highlight (pulsing, full alpha) */
void CBGTypeText_Select(struct CBGTypeText *self, float pulsePhase)
{
	self->glowTarget = 0.0f;
	self->glow = 0.0f;
	self->style |= 1;
	self->buf.text.color[3] = 0x7f;
	self->alpha = 127.0f;
	self->pulsePhase = pulsePhase;
}

/* 0x000114e0  Core/BG_Effects.cpp */
void CBGTypeText_Reset(struct CBGTypeText *self)
{
	self->buf.text.color[3] = 0x7f;
	self->alpha = 127.0f;
	self->glowTarget = 0.0f;
	self->glow = 0.0f;
}

/* 0x00011500  Core/BG_Effects.cpp: not highlighted (dim, glowing) */
void CBGTypeText_Deselect(struct CBGTypeText *self)
{
	self->buf.text.color[3] = 0x30;
	self->glowTarget = 2.0f;
	self->style &= ~1u;
}

/* 0x00011520  Core/BG_Effects.cpp */
void CBGTilingBackdrop_Init(struct CBGTilingBackdrop *self)
{
	self->tileU = 5.0f;
	self->tileV = 3.75f;
	self->scrollU = 0.0f;
	self->scrollV = 0.0f;
	self->texture = FUN_0004bb30(0, "ff_tile.bmp", 0, 0);
	if (self->texture == 0)
		CMA_DebugPrintf("CTilingBackDrop: Couldn't load ff_tile.bmp\n");
}

/* 0x00011570  Core/BG_Effects.cpp: a 640x480 quad, the texture scrolls diagonally */
void CBGTilingBackdrop_Render(struct CBGTilingBackdrop *self)
{
	struct CMA_Vertex *v = DAT_002b9510;
	uint32_t grey;

	FUN_00043b70(0);
	FUN_000416c0(0, self->texture);
	FUN_000438b0();
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(4, 0, 0);
	FUN_00044b00(1, 2, 0);
	FUN_00044b00(2, 3, 0);
	grey = CMA_RGBA(127, 127, 127, 127);
	v[0].x = -320.0f;
	v[0].y = -240.0f;
	v[0].z = 0.5f;
	v[0].color = grey;
	v[0].u = self->scrollU;
	v[0].v = self->tileV + self->scrollV;
	v[1].x = 320.0f;
	v[1].y = -240.0f;
	v[1].z = 0.5f;
	v[1].color = grey;
	v[1].u = self->scrollU + self->tileU;
	v[1].v = self->tileV + self->scrollV;
	v[2].x = 320.0f;
	v[2].y = 240.0f;
	v[2].z = 0.5f;
	v[2].color = grey;
	v[2].u = self->scrollU + self->tileU;
	v[2].v = self->scrollV;
	v[3].x = -320.0f;
	v[3].y = 240.0f;
	v[3].z = 0.5f;
	v[3].color = grey;
	v[3].u = self->scrollU;
	v[3].v = self->scrollV;
	FUN_0003f2d0(5, (int)v, 4);
	self->scrollU = (float)((double)self->scrollU - (double)DAT_0028fda8 * 0.01f);
	self->scrollV = (float)((double)self->scrollV - (double)DAT_0028fda8 * 0.01f);
}

/* 0x00011700  Core/BG_Effects.cpp: operator new of the statically linked XDK libraries */
void *ExternalObject_New(unsigned int size)
{
	return CMA_New(size, "External Object", 0, 0x1000);
}

/* 0x00011720  Core/BG_Effects.cpp: operator delete of the statically linked XDK libraries
   (a jump to FUN_0004c110) */
void ExternalObject_Delete(void *block)
{
	CMA_Delete(block);
}

/* 0x00011730  Core/BG_Effects.cpp: rand() (MSVC linear congruential generator) */
unsigned int Core_Rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
	return (DAT_00285630 >> 16) & 0x7fff;
}

/* 0x00011750  Core/BG_Effects.cpp: rand() / 32767.0 */
float Core_RandUnit(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
	return (float)(int)((DAT_00285630 >> 16) & 0x7fff) * 3.0518509e-05f;
}

/* 0x00011790  Core/BG_Effects.cpp: CBGEffect scalar deleting destructor */
struct CBGEffect *CBGEffect_Delete(struct CBGEffect *self, unsigned int flags)
{
	self->vtbl = vtable_00253744;
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x000117b0  Core/BG_Effects.cpp: grid of spaces (cols x rows) at (x, y) */
struct CBGMatrixText *CBGMatrixText_ctor(struct CBGMatrixText *self, float x, float y, int cols,
	int rows, int scroll, float interval, int *color)
{
	unsigned int n = (unsigned int)(cols * rows);
	unsigned int i;

	self->base.flags = 3;
	self->x = x;
	self->cols = cols;
	self->rows = rows;
	self->base.vtbl = vtable_00253788;
	self->y = y;
	self->scroll = scroll;
	self->buffer = CMA_New(n * 2, s_BG_Effects_cpp, 0xa8, 0x1001);
	self->color[0] = color[0];
	self->color[1] = color[1];
	self->color[2] = color[2];
	self->color[3] = color[3];
	self->interval = interval;
	self->scale = 1.0f;
	self->cursor = self->cols * self->rows - 1;
	for (i = 0; i < n; i++)
		self->buffer[i] = ' ';
	return self;
}

/* 0x00011890  Core/BG_Effects.cpp */
void CBGMatrixText_dtor(struct CBGMatrixText *self)
{
	self->base.vtbl = vtable_00253788;
	CMA_Delete(self->buffer);
	self->base.vtbl = vtable_00253744;
}

/* 0x000118e0  Core/BG_Effects.cpp: random '0' and '1' characters (bit 0 of the third seed byte) */
void CBGMatrixText_FillLine(struct CBGMatrixText *self, unsigned short *dst, int count)
{
	(void)self;
	for (; count != 0; count--) {
		DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
		*dst++ = (unsigned short)(((DAT_00285630 >> 16) & 1) + '0');
	}
}

/* 0x00011930  Core/BG_Effects.cpp: FillLine of the glitching variant (vtable 0x00255448,
   core_modes_b): random characters while the int at +0x48 is set, else spaces; one call in 100
   toggles it. Returns what the original left in EAX. */
unsigned int CBGMatrixTextGlitch_FillLine(struct CBGMatrixText *self, unsigned short *dst,
	unsigned int count)
{
	int *glitch = &CMA_FIELD(int, self, 0x48);
	unsigned int r;
	unsigned int result;

	if (*glitch == 0) {
		for (; count != 0; count--)
			*dst++ = ' ';
	} else {
		for (; count != 0; count--) {
			DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
			*dst++ = DAT_002818e0[((DAT_00285630 >> 16) & 0x7fff) % 0x53];
		}
	}
	DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
	r = (DAT_00285630 >> 16) & 0x7fff;
	result = r / 100;
	if (r % 100 == 0) {
		result = (unsigned int)*glitch;
		*glitch = (*glitch == 0);
	}
	return result;
}

/* 0x000119e0  Core/BG_Effects.cpp: the colour is stored halved */
struct CBGText *CBGText_ctor(struct CBGText *self, int x, int y, unsigned short *text, int font,
	int *color)
{
	self->base.flags = 3;
	self->font = font;
	self->base.vtbl = vtable_002537d8;
	self->scale = 1.0f;
	self->color[0] = color[0] / 2;
	self->color[1] = color[1] / 2;
	self->color[2] = color[2] / 2;
	self->color[3] = color[3] / 2;
	self->x = (float)x;
	self->text = text;
	self->source = text;
	self->y = (float)y;
	self->length = text == 0 ? 0 : CMA_WStrLen(text);
	return self;
}

/* 0x00011a90  Core/BG_Effects.cpp */
void CBGText_dtor(struct CBGText *self)
{
	self->base.vtbl = vtable_00253744;
}

/* 0x00011aa0  Core/BG_Effects.cpp: CAPACITY 0 means the length of TEXT plus one */
struct CBGTextBuf *CBGTextBuf_ctor(struct CBGTextBuf *self, int x, int y, unsigned short *text,
	int font, int *color, int capacity)
{
	wchar16 *buffer;

	CBGText_ctor(&self->text, x, y, text, font, color);
	self->text.base.vtbl = vtable_002537f4;
	if (capacity == 0)
		capacity = self->text.length + 1;
	self->capacity = capacity;
	buffer = CMA_New((unsigned int)capacity * 2, s_BG_Effects_cpp, 0x1a5, 0x1001);
	self->text.text = buffer;
	self->buffer = buffer;
	if (text == 0) {
		buffer[0] = 0;
	} else {
		if (capacity < self->text.length)
			self->text.length = capacity - 1;
		memcpy(buffer, self->text.source, (size_t)self->text.length * 2);
		self->buffer[self->text.length] = 0;
	}
	return self;
}

/* 0x00011b70  Core/BG_Effects.cpp */
void CBGTextBuf_dtor(struct CBGTextBuf *self)
{
	self->text.base.vtbl = vtable_002537f4;
	CMA_Delete(self->buffer);
	self->text.base.vtbl = vtable_00253744;
}

/* 0x00011bc0  Core/BG_Effects.cpp */
void CBGTypeText_dtor(struct CBGTypeText *self)
{
	self->buf.text.base.vtbl = vtable_00253810;
	if (self->typeSound != 0 && self->voice >= 0)
		FUN_000546e0(self->voice);
	CBGTextBuf_dtor(&self->buf);
}

/* 0x00011c30  Core/BG_Effects.cpp */
struct CBGTypeText *CBGTypeText_Delete(struct CBGTypeText *self, unsigned int flags)
{
	CBGTypeText_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* the fields both CBGTypeText constructors set (0x00011c50, 0x00011cd0) */
static void CBGTypeText_InitFields(struct CBGTypeText *self)
{
	self->typeSound = 0;
	self->buf.text.base.node.next = 0;
	self->glowTarget = 0.0f;
	self->alpha = 0.0f;
	self->charTimer = 0.0f;
	self->charDelay = 0.0f;
	self->typed = 0;
	self->lines = 0;
	self->style = 0;
	self->wrap = 0;
	self->buf.text.base.vtbl = vtable_00253810;
	self->glow = 5.0f;
	self->glowRate = 0.15f;
	self->alphaRate = 3.0f;
	self->boxWidth = 640.0f;
	self->revealed = -1;
}

/* 0x00011c50  Core/BG_Effects.cpp */
struct CBGTypeText *CBGTypeText_ctor(struct CBGTypeText *self, int x, int y, unsigned short *text,
	int font)
{
	CBGTextBuf_ctor(&self->buf, x, y, text, font, DAT_0028450c, 0);
	CBGTypeText_InitFields(self);
	return self;
}

/* 0x00011cd0  Core/BG_Effects.cpp: empty text with room for CAPACITY characters */
struct CBGTypeText *CBGTypeText_ctorEmpty(struct CBGTypeText *self, int x, int y, int capacity,
	int font)
{
	CBGTextBuf_ctor(&self->buf, x, y, 0, font, DAT_0028450c, capacity);
	CBGTypeText_InitFields(self);
	return self;
}

/* 0x00011d50  Core/BG_Effects.cpp: typing, the "computertext" beeps, glow and alpha.
   Returns 0 when the text has faded out. */
int CBGTypeText_Update(struct CBGTypeText *self)
{
	char name[32];
	double t;
	float alphaTarget;

	self->buf.text.length = CMA_WStrLen(self->buf.buffer);
	if (self->charDelay != 0.0f) {
		if (self->startDelay <= 0.0f) {
			int voice = self->voice;

			if (voice == -1) {
				self->voice = CMA_PlaySample(self->typeSound, 0xff, 0, 0x20, 0);
			} else if (self->typeSound != 0 && voice != -2 &&
				self->typed >= BG_CountGlyphs(self->buf.buffer)) {
				FUN_000546e0(voice);
				self->voice = -2;
			}
			t = (double)DAT_0028fda8 + (double)self->charTimer;
			self->charTimer = (float)t;
			if (!(t < (double)self->charDelay)) {
				do {
					int typed = self->typed;
					const wchar16 *c;

					if (typed >= BG_CountGlyphs(self->buf.buffer))
						break;
					if (self->beepVolume != 0 && typed % 5 == 0) {
						int sample;

						sprintf(name, "computertext%d.wav", (int)(Core_Rand() & 3) + 1);
						sample = FUN_0004bb30(0, name, 2, 0xdefa);
						CMA_PlaySample(sample, self->beepVolume, 0, 0, 0);
					}
					typed = self->typed + 1;
					self->charTimer = self->charTimer - self->charDelay;
					self->typed = typed;
					c = self->buf.text.text + typed;
					if (c[0] == '\\' && c[1] == 'n') {
						self->typed = typed + 2;
						self->lines = self->lines + 1;
					}
				} while (!(self->charTimer < self->charDelay));
			}
		} else {
			self->startDelay = self->startDelay - DAT_0028fda8;
		}
		self->revealed = self->typed;
	}

	/* glow moves towards its target (the step is rounded to a float, the sums are not) */
	if (self->glow != self->glowTarget) {
		float step = DAT_0028fda8 * self->glowRate;
		double towards = (double)self->glowTarget - (double)step;

		if (self->glow >= self->glowTarget) {
			if (towards >= (double)self->glow)
				self->glow = self->glowTarget;
			else
				self->glow = (float)((double)self->glow - (double)step);
		} else if (towards <= (double)self->glow) {
			self->glow = self->glowTarget;
		} else {
			self->glow = (float)((double)step + (double)self->glow);
		}
	}

	/* alpha moves towards the colour's alpha (the step stays unrounded, as on the x87 stack) */
	alphaTarget = (float)self->buf.text.color[3];
	if (self->alpha != alphaTarget) {
		double step = (double)DAT_0028fda8 * (double)self->alphaRate;

		if ((double)alphaTarget - step <= (double)self->alpha) {
			if ((double)alphaTarget + step >= (double)self->alpha)
				self->alpha = alphaTarget;
			else
				self->alpha = (float)((double)self->alpha - step);
		} else {
			self->alpha = (float)(step + (double)self->alpha);
		}
	}
	return self->alpha > 0.0f;
}

/* 0x00011fd0  Core/BG_Effects.cpp: shadow, text, pulse glow and underline bar */
void CBGTypeText_Render(struct CBGTypeText *self)
{
	struct CBGText *text = &self->buf.text;
	struct CMA_TextLayout layout;
	unsigned int style;
	float m[16];
	void *cb;
	void *ctx;

	if (text->length == 0)
		return;
	self->glyphCount = 0;
	BG_SetTextRenderStates();
	FUN_00043b70(0);
	layout.width = self->boxWidth / text->scale;
	style = self->style;
	layout.wrap = self->wrap;
	layout.height = 0.0f;
	layout.unk1c = 0;
	if (style & 8)
		layout.hAlign = 2;
	else
		layout.hAlign = (style & 0x10) ? 3 : 0;
	if (style & 0x40)
		layout.vAlign = 2;
	else
		layout.vAlign = (style & 0x20) ? 4 : 1;
	memcpy(m, DAT_002855f0, sizeof(m));
	ctx = self->charDelay != 0.0f ? (void *)self : 0;
	cb = self->charDelay != 0.0f ? (void *)FUN_00072060 : 0;

	if (self->glow == 0.0f) {
		double pulse;

		m[0] = text->scale;
		m[5] = text->scale;
		m[10] = text->scale;
		m[12] = text->x;
		m[13] = text->y;
		layout.x = 0.0f;
		layout.y = 0.0f;
		if (style & 4) {
			m[12] = m[12] + 1.0f;
			m[13] = m[13] - 1.0f;
			FUN_00043930((undefined4 *)m);
			CMA_PrintTextBox(text->font, text->text, text->length, &layout, 0, 0, 0,
				(int)self->alpha, cb, ctx);
			m[12] = m[12] - 1.0f;
			m[13] = m[13] + 1.0f;
		}
		FUN_00043930((undefined4 *)m);
		self->glyphCount = 0;
		CMA_PrintTextBox(text->font, text->text, text->length, &layout, text->color[0],
			text->color[1], text->color[2], (int)self->alpha, cb, ctx);
		if ((self->style & 1) == 0 || text->color[3] == 0)
			goto underline;
		pulse = BG_Pulse(self->pulsePhase);
		layout.x = text->x;
		layout.y = text->y;
		CMA_PrintTextBoxFx(text->font, text->text, text->length, &layout, text->color[0],
			text->color[1], text->color[2], (int)(pulse * (double)self->alpha),
			(float)(3.0 * pulse), text->scale, ctx);
	} else {
		layout.x = text->x;
		layout.y = text->y;
		CMA_PrintTextBoxFx(text->font, text->text, text->length, &layout, text->color[0],
			text->color[1], text->color[2], (int)self->alpha, self->glow, text->scale, ctx);
	}

underline:
	if (self->style & 2) {
		int rgba[4];
		float lineHeight = CMA_FIELD(float, text->font, 0x28);

		FUN_000438b0();
		rgba[0] = text->color[0] << 1;
		rgba[1] = text->color[1] << 1;
		rgba[2] = text->color[2] << 1;
		rgba[3] = (int)self->alpha << 1;
		BG_DrawRect(-320.0f,
			(float)((double)text->y - (double)lineHeight * (double)text->scale * 0.75),
			(float)((double)self->alpha * (double)5.0393701f), 2.0f, rgba);
	}
}

/* 0x00012390  Core/BG_Effects.cpp: fade out over TIME frames (0: 30); -1 removes the effect
   from the BGEffects list and deletes it now */
void CBGTypeText_Hide(struct CBGTypeText *self, float time)
{
	double rate;

	if (self->typeSound != 0 && self->voice >= 0) {
		FUN_000546e0(self->voice);
		self->voice = -1;
	}
	if (time == -1.0f) {
		CMA_List_Remove(&DAT_002b95d0->group.children, &self->buf.text.base.node);
		CMA_VFN(self, 0, void *(*)(void *, unsigned int))(self, 1);
		return;
	}
	self->buf.text.color[3] = 0;
	self->glowTarget = 8.0f;
	if (time == 0.0f)
		time = 30.0f;
	rate = 1.0 / (double)time;
	self->alphaRate = (float)(rate * (double)self->alpha);
	self->glowRate = (float)(rate * (double)self->glow);
}

/* 0x00012430  Core/BG_Effects.cpp */
void CBGTypeText_SetGreyStyle(struct CBGTypeText *self)
{
	self->style = 0x20;
	self->wrap = 1;
	self->buf.text.color[0] = DAT_0028451c[0] >> 1;
	self->buf.text.color[1] = DAT_0028451c[1] >> 1;
	self->buf.text.color[2] = DAT_0028451c[2] >> 1;
	self->buf.text.color[3] = DAT_0028451c[3] >> 1;
	self->glowTarget = 0.0f;
}

/* the fields both CBGTypeTextBox constructors set */
static void CBGTypeTextBox_InitLayout(struct CBGTypeTextBox *self, float width)
{
	self->layout.x = 0.0f;
	self->layout.y = 0.0f;
	self->layout.unk1c = 0;
	self->layout.hAlign = 0;
	self->type.buf.text.base.vtbl = vtable_00253858;
	self->layout.width = width;
	self->layout.height = 480.0f;
	self->layout.wrap = 1;
	self->layout.vAlign = 1;
}

/* 0x00012480  Core/BG_Effects.cpp: a 540 x 480 box */
struct CBGTypeTextBox *CBGTypeTextBox_ctor(struct CBGTypeTextBox *self, int x, int y,
	unsigned short *text, int font)
{
	CBGTypeText_ctor(&self->type, x, y, text, font);
	CBGTypeTextBox_InitLayout(self, 540.0f);
	return self;
}

/* 0x000124f0  Core/BG_Effects.cpp: a 640 x 480 box */
struct CBGTypeTextBox *CBGTypeTextBox_ctorEmpty(struct CBGTypeTextBox *self, int x, int y,
	int capacity, int font)
{
	CBGTypeText_ctorEmpty(&self->type, x, y, capacity, font);
	CBGTypeTextBox_InitLayout(self, 640.0f);
	return self;
}

/* 0x00012560  Core/BG_Effects.cpp */
void CBGTypeTextBox_Render(struct CBGTypeTextBox *self)
{
	struct CBGTypeText *type = &self->type;
	struct CBGText *text = &type->buf.text;
	struct CMA_TextLayout *layout = &self->layout;
	void *ctx;
	float m[16];

	layout->x = text->x;
	layout->y = text->y;
	if (type->glow != 0.0f) {
		ctx = type->charDelay != 0.0f ? (void *)type : 0;
		CMA_PrintTextBoxFx(text->font, text->text, text->length, layout, text->color[0],
			text->color[1], text->color[2], (int)type->alpha, type->glow, text->scale, ctx);
		return;
	}
	memcpy(m, DAT_002855f0, sizeof(m));
	m[0] = text->scale;
	m[5] = text->scale;
	m[10] = text->scale;
	FUN_00043930((undefined4 *)m);
	if (type->style & 4) {
		layout->x = layout->x + 1.0f;
		layout->y = layout->y - 1.0f;
		CMA_PrintTextBox(text->font, text->text, text->length, layout, 0, 0, 0, (int)type->alpha,
			0, 0);
		layout->x = layout->x - 1.0f;
		layout->y = layout->y + 1.0f;
	}
	CMA_PrintTextBox(text->font, text->text, text->length, layout, text->color[0], text->color[1],
		text->color[2], (int)type->alpha, 0, 0);
	if ((type->style & 1) && text->color[3] != 0) {
		long long t = CMA_GetTicks();
		double pulse = fabs(sin(((double)t + (double)t) / (double)DAT_002cd7a0));

		ctx = type->charDelay != 0.0f ? (void *)type : 0;
		CMA_PrintTextBoxFx(text->font, text->text, text->length, layout, text->color[0],
			text->color[1], text->color[2], (int)(pulse * (double)type->alpha),
			(float)(3.0 * pulse), text->scale, ctx);
	}
}

/* 0x00012730  Core/BG_Effects.cpp */
struct CBGTilingBackdrop *CBGTilingBackdrop_ctor(struct CBGTilingBackdrop *self)
{
	self->base.flags = 3;
	self->base.vtbl = vtable_00253880;
	return self;
}

/* 0x00012740  Core/BG_Effects.cpp */
struct CBGMatrixText *CBGMatrixText_Delete(struct CBGMatrixText *self, unsigned int flags)
{
	CBGMatrixText_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}
