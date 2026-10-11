/*
bdisplay module tests: one guest image (src/bdisplay/tests/run_tests.sh builds it from the
game build's objects of src/bdisplay and src/bkernel (for the maths), the generated weak
stubs, the guest runtime and the platform layer). No D3D device is created: the platform
layer's state setters only record state, which the tests read back.

The game image globs every .c under src/bdisplay, so this file compiles to nothing unless
OPENFRENZY_BDISPLAY_TEST is defined.

What each test protects (why it matters to the game):
- render states: Babel state numbers must reach the right D3D render and texture states
  (blending, Z, filtering, wrapping); locks must hold a state; push/pop and save/restore
  must give the old state back (the video player and the 2D blit depend on it).
- texture frames: animated textures show the frame of the current time, wrapping.
- texture stages: the select callback can refuse a texture, the old texture's deselect
  callback runs, disabled stages and the "keep" value 1 change nothing.
- text layout (bPrintFontVerts, rebuilt from the disassembly): wrapping breaks at spaces,
  "\\n" starts a line, "@@" prints '@', centring, justification, vertical centring, the
  missing-glyph fallback, and the per-character callback.
- projection: a point on the view axis lands on the screen centre, the edge of the field
  of view on the screen edge, y up; sizes scale with 1/depth; the visibility tests agree.
- near clipping: a segment crossing the near plane is cut there with its colour and width
  interpolated (clipped beams and lines keep their look).
- sprites: four vertices around the projected point with the documented texture flips.
- vertex buffers: the lock/unlock rotation cycles through the copies.
- error names: D3D results map to their names.
*/

#ifdef OPENFRENZY_BDISPLAY_TEST

#include "bdisplay_internal.h"

#include <stdlib.h>

/* ---------- symbols the tested objects need from modules not linked here */

int DAT_002caf18, DAT_002caf1c, DAT_002caf20, DAT_002caf24, DAT_002caf28, DAT_002caf2c;
int DAT_002caf30, DAT_002caf34, DAT_002caf38, DAT_002caf3c;
int DAT_002caf40, DAT_002caf44, DAT_002caf48, DAT_002caf4c, DAT_002caf50, DAT_002caf54;
int DAT_002caf58, DAT_002caf5c, DAT_002caf60, DAT_002caf64, DAT_002caf68, DAT_002caf6c;
int DAT_002caf70, DAT_002caf74, DAT_002caf78;
struct BInputMap *DAT_002caf7c;
struct BInputChannel *DAT_002caf80, *DAT_002caf84, *DAT_002caf88, *DAT_002caf8c;
struct BInputChannel *DAT_002caf90, *DAT_002caf94, *DAT_002caf98, *DAT_002caf9c;
struct BInputChannel *DAT_002cafa0;
struct BGuiMenu
{
	BYTE bytes[0x598];
} DAT_002cafa4;
struct BGuiMenu *DAT_002cb53c, *DAT_002cb540, *DAT_002cb548, *DAT_002cb54c;
int DAT_002cb550, DAT_002cb554;
int (*DAT_002cb558)(struct BGuiEvent *event);
struct BInputMap *DAT_002cc1b4;

static int failures;
static int checks;

#define CHECK(condition, ...) \
	do \
	{ \
		checks++; \
		if (!(condition)) \
		{ \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s: ", __FILE__, __LINE__, #condition); \
			fprintf(stderr, __VA_ARGS__); \
			fprintf(stderr, "\n"); \
		} \
	} while (0)

static int near_equal(float a, float b, float tolerance)
{
	return fabsf(a - b) <= tolerance;
}

/* ---------- vertex formats */

static void test_fvf(void)
{
	CHECK(bdFVFStride(BD_FVF_TLVERTEX) == (int)sizeof(bdTLVertex), "screen vertex stride %d",
		bdFVFStride(BD_FVF_TLVERTEX));
	CHECK(bdFVFStride(BD_FVF_VERTEX) == (int)sizeof(bdVertex), "world vertex stride %d",
		bdFVFStride(BD_FVF_VERTEX));
	CHECK(bdFVFStride(0x12345) == 0, "an unknown FVF has no stride");
}

/* ---------- render states */

static void test_render_states(void)
{
	unsigned int saved[BD_NUM_STATES * 2];
	unsigned int extra;
	int stage;

	bdSetRenderState(5, 1, 0);
	CHECK(D3D__RenderState[D3DRS_ZWRITEENABLE] == 1, "state 5 is Z write");
	bdSetRenderState(6, 1, 0);
	CHECK(D3D__RenderState[D3DRS_ALPHABLENDENABLE] == 1, "state 6 is alpha blending");
	bdSetRenderState(7, 2, 0);
	CHECK(D3D__RenderState[D3DRS_ZFUNC] == 0x203, "state 7 value 2 is LESSEQUAL (0x%lx)",
		(unsigned long)D3D__RenderState[D3DRS_ZFUNC]);
	bdSetRenderState(10, 3, 0);
	CHECK(D3D__RenderState[D3DRS_SRCBLEND] == 0x302 && D3D__RenderState[D3DRS_DESTBLEND] == 1 &&
		D3D__RenderState[D3DRS_BLENDOP] == 0x800b, "blend mode 3 is src alpha, one, reverse subtract");
	bdSetRenderState(4, 2, 0);
	CHECK(D3D__RenderState[D3DRS_CULLMODE] == 0x900, "cull mode 2 is CW");
	bdSetRenderState(1, 2, 0);
	for (stage = 0; stage < 4; stage++)
		CHECK(D3D__TextureState[stage][D3DTSS_MAGFILTER] == 2 && D3D__TextureState[stage][D3DTSS_MINFILTER] == 2 &&
			D3D__TextureState[stage][D3DTSS_MIPFILTER] == 0, "filter 2 is bilinear on stage %d", stage);
	bdSetRenderState(2, 3, 0);
	CHECK(D3D__TextureState[0][D3DTSS_ADDRESSU] == 1 && D3D__TextureState[0][D3DTSS_ADDRESSV] == 1,
		"texture wrap state 3 wraps u and v (the default)");
	bdSetRenderState(2, 1, 0);
	CHECK(D3D__TextureState[0][D3DTSS_ADDRESSU] == 1 && D3D__TextureState[0][D3DTSS_ADDRESSV] == 3,
		"bit 0 wraps u only");
	bdSetRenderState(2, 0xc, 0);
	CHECK(D3D__TextureState[0][D3DTSS_ADDRESSU] == 3 && D3D__TextureState[0][D3DTSS_ADDRESSV] == 3,
		"without bits 0 and 1 both clamp (the video player's 0xc)");
	bdSetRenderState(8, 1, 0);
	CHECK(D3D__RenderState[D3DRS_FOGENABLE] == 1, "state 8 is fog");

	/* locks */
	bdSetRenderState(5, 1, 7);
	CHECK(bdLockRenderState(5) == 1, "first lock");
	bdSetRenderState(5, 0, 0);
	CHECK(D3D__RenderState[D3DRS_ZWRITEENABLE] == 1 && bdGetRenderState(5, &extra) == 1 && extra == 7,
		"a locked state does not change");
	CHECK(bdUnlockRenderState(5) == 0, "unlocked");
	bdSetRenderState(5, 0, 0);
	CHECK(D3D__RenderState[D3DRS_ZWRITEENABLE] == 0, "an unlocked state changes");

	/* push / pop */
	bdSetRenderState(6, 0, 0);
	bdPushRenderState(6, 1, 5);
	CHECK(D3D__RenderState[D3DRS_ALPHABLENDENABLE] == 1, "push sets the new value");
	bdPushRenderState(6, 0, 0);
	bdPopRenderState(6);
	CHECK(D3D__RenderState[D3DRS_ALPHABLENDENABLE] == 1 && bdGetRenderState(6, &extra) == 1 && extra == 5,
		"pop restores the previous value and extra");
	bdPopRenderState(6);
	CHECK(D3D__RenderState[D3DRS_ALPHABLENDENABLE] == 0, "the second pop restores the first value");

	/* save / restore */
	bdSetRenderState(7, 3, 0);
	bdSaveRenderStates(saved);
	bdSetRenderState(7, 1, 0);
	bdSetRenderState(5, 1, 0);
	bdRestoreRenderStates(saved);
	CHECK(D3D__RenderState[D3DRS_ZFUNC] == 0x201 && D3D__RenderState[D3DRS_ZWRITEENABLE] == 0,
		"restore sets the saved states again");
}

/* ---------- textures */

static BYTE texture_memory[3][0x200];

static bdTexture *make_texture(int index, int numFrames, int format)
{
	bdTexture *texture = (bdTexture *)texture_memory[index];

	memset(texture, 0, sizeof(texture_memory[index]));
	texture->frames = (bdTextureFrame *)((BYTE *)texture + 0x80);
	texture->current = &texture->frames[0].d3d;
	texture->numFrames = (BYTE)numFrames;
	texture->format = (WORD)format;
	texture->width = 64;
	texture->height = 32;
	return texture;
}

static int select_calls, deselect_calls, deselect_stage;

static int refuse(bdTexture *texture, int stage, void *context)
{
	(void)texture;
	(void)stage;
	(void)context;
	select_calls++;
	return 0;
}

static int note_deselect(bdTexture *texture, int stage, void *context)
{
	(void)texture;
	(void)context;
	deselect_calls++;
	deselect_stage = stage;
	return 1;
}

static void test_texture_frames(void)
{
	bdTexture *texture = make_texture(0, 3, BDTEXFMT_ARGB8888);

	texture->frames[0].time = 10;
	texture->frames[1].time = 20;
	texture->frames[2].time = 30;
	texture->totalTime = 60;
	bdTextureSetTime(texture, 15);
	CHECK(texture->frame == 1 && texture->current == &texture->frames[1].d3d, "time 15 shows frame 1 (%d)",
		texture->frame);
	bdTextureSetTime(texture, 65);
	CHECK(texture->frame == 0, "time wraps at the total time (%d)", texture->frame);
	bdTextureSetTime(texture, -5);
	CHECK(texture->frame == 2, "negative time counts back from the end (%d)", texture->frame);
	bdTextureSetFrame(texture, -1);
	CHECK(texture->frame == 2 && texture->current == &texture->frames[2].d3d, "frame -1 is the last");
	bdTextureSetFrame(texture, 4);
	CHECK(texture->frame == 1, "frames wrap (%d)", texture->frame);
}

static void test_texture_stages(void)
{
	bdTexture *refused = make_texture(0, 1, BDTEXFMT_ARGB8888);
	bdTexture *palettized = make_texture(1, 1, BDTEXFMT_PAL8);
	bdTexture *plain = make_texture(2, 1, BDTEXFMT_ARGB8888);

	memset(DAT_002be5d4, 0, sizeof(DAT_002be5d4));
	DAT_002bee34[0] = 1;
	DAT_002bee34[1] = 0;
	bdTextureSetCallback(refused, 0, (void *)refuse, NULL);
	select_calls = 0;
	bdSetTexture(0, refused);
	CHECK(select_calls == 1 && DAT_002be5d4[0] == NULL, "a select callback can refuse the texture");
	bdSetTexture(0, palettized);
	CHECK(DAT_002be5d4[0] == palettized, "a texture without callbacks is selected");
	bdTextureSetCallback(palettized, 1, (void *)note_deselect, NULL);
	deselect_calls = 0;
	bdSetTexture(0, plain);
	CHECK(deselect_calls == 1 && deselect_stage == 0 && DAT_002be5d4[0] == plain,
		"the old texture's deselect callback runs with its stage");
	bdSetTexture(0, (bdTexture *)1);
	CHECK(DAT_002be5d4[0] == plain, "1 keeps the texture");
	bdSetTexture(1, plain);
	CHECK(DAT_002be5d4[1] == NULL, "a disabled stage is not changed");
	bdSetTexture(0, NULL);
	CHECK(DAT_002be5d4[0] == NULL, "NULL clears the stage");
	DAT_002bee34[0] = 0;
}

/* ---------- text layout */

static BYTE font_memory[sizeof(bdFont)];

static bdFont *make_font(void)
{
	bdFont *font = (bdFont *)font_memory;
	int c;

	memset(font, 0, sizeof(font_memory));
	font->texture = &font->fontTexture;
	font->texture->width = 256;
	font->texture->height = 128;
	font->height = 16.0f;
	for (c = 'A'; c <= 'Z'; c++)
	{
		font->glyphs[c - BDFONT_FIRST_CHAR].u = (short)((c - 'A') * 10);
		font->glyphs[c - BDFONT_FIRST_CHAR].v = 0;
		font->glyphs[c - BDFONT_FIRST_CHAR].width = 10;
		font->glyphs[c - BDFONT_FIRST_CHAR].height = 16;
	}
	font->glyphs[' ' - BDFONT_FIRST_CHAR].width = 5;
	font->glyphs[' ' - BDFONT_FIRST_CHAR].height = 16;
	font->glyphs['@' - BDFONT_FIRST_CHAR].width = 12;
	font->glyphs['@' - BDFONT_FIRST_CHAR].height = 16;
	font->glyphs[BDFONT_MISSING_GLYPH].width = 7;
	font->glyphs[BDFONT_MISSING_GLYPH].height = 16;
	return font;
}

static int to_wide(unsigned short *out, const char *text)
{
	int n = 0;

	while (text[n] != 0)
	{
		out[n] = (unsigned char)text[n];
		n++;
	}
	return n;
}

static bdPrintFormat make_format(float width, float height, int hAlign, int vAlign, int wrap)
{
	bdPrintFormat format;

	format.x = 100.0f;
	format.y = 50.0f;
	format.width = width;
	format.height = height;
	format.hAlign = hAlign;
	format.vAlign = vAlign;
	format.wrap = wrap;
	format.lineSpacing = 0.0f;
	return format;
}

static bdVertex text_vertices[4 * 64];
static bdPrintLine text_lines[64];

static int layout(bdFont *font, const char *text, bdPrintFormat *format, void *callback, void *context)
{
	unsigned short wide[64];
	int n = to_wide(wide, text);

	memset(text_vertices, 0, sizeof(text_vertices));
	return bPrintFontVerts((float *)text_vertices, text_lines, font, wide, n, format, 255, 255, 255, 255,
		callback, context);
}

static int callback_chars, callback_order_ok;

static int count_chars(int reason, bdPrintCallbackInfo *info)
{
	if (reason == 0)
	{
		if (info->index != callback_chars ||
			info->current != info->vertices + 36 * callback_chars)
			callback_order_ok = 0;
		callback_chars++;
	}
	return 1;
}

static void test_text_layout(void)
{
	bdFont *font = make_font();
	bdPrintFormat format;
	int lines;

	format = make_format(0.0f, 0.0f, 0, 1, 0);
	lines = layout(font, "AB", &format, NULL, NULL);
	CHECK(lines == 1 && text_lines[0].numChars == 2 && text_lines[0].width == 20.0f, "\"AB\" is one line 20 wide");
	CHECK(text_vertices[0].x == 100.0f && text_vertices[0].y == 34.0f && text_vertices[1].y == 50.0f &&
		text_vertices[2].x == 110.0f && text_vertices[4].x == 110.0f && text_vertices[6].x == 120.0f,
		"glyph quads advance by the glyph width, top at y, bottom at y - height (%g %g %g)",
		text_vertices[0].x, text_vertices[0].y, text_vertices[4].x);
	CHECK(near_equal(text_vertices[1].u, 0.5f / 256.0f, 1e-6f) &&
		near_equal(text_vertices[3].u, 10.0f / 256.0f + 0.5f / 256.0f, 1e-6f),
		"texture coordinates are the glyph's texels plus half a texel");
	CHECK(text_vertices[0].color == BD_RGBA(255, 255, 255, 255), "the colour comes from the channel tables");

	format = make_format(25.0f, 0.0f, 0, 1, 1);
	lines = layout(font, "AA AA", &format, NULL, NULL);
	CHECK(lines == 2 && text_lines[0].numChars == 2 && text_lines[1].numChars == 2,
		"wrapping breaks at the space, not inside a word (%d lines, %d + %d)", lines, text_lines[0].numChars,
		text_lines[1].numChars);
	CHECK(text_vertices[2 * 4].y == 34.0f - 16.0f, "the second line is one line height lower (%g)",
		text_vertices[2 * 4].y);

	format = make_format(0.0f, 0.0f, 0, 1, 0);
	lines = layout(font, "A\\nB", &format, NULL, NULL);
	CHECK(lines == 2 && text_lines[0].numChars == 1 && text_lines[1].numChars == 1, "\\n starts a new line");

	lines = layout(font, "@@A", &format, NULL, NULL);
	CHECK(lines == 1 && text_lines[0].numChars == 2 && text_lines[0].width == 22.0f, "@@ prints one '@'");

	lines = layout(font, "a", &format, NULL, NULL);
	CHECK(lines == 1 && text_lines[0].width == 7.0f, "a missing glyph is drawn as the font's last glyph");

	format = make_format(100.0f, 0.0f, 2, 1, 0);
	layout(font, "AA", &format, NULL, NULL);
	CHECK(text_vertices[0].x == 140.0f, "centred text starts at (box - width) / 2 (%g)", text_vertices[0].x);

	format = make_format(60.0f, 0.0f, 5, 1, 0);
	layout(font, "A A A", &format, NULL, NULL);
	CHECK(text_vertices[2 * 4].x == 125.0f && text_vertices[4 * 4].x == 150.0f,
		"justified text spreads the free width over the spaces (%g %g)", text_vertices[2 * 4].x,
		text_vertices[4 * 4].x);

	format = make_format(0.0f, 100.0f, 0, 2, 0);
	layout(font, "A", &format, NULL, NULL);
	CHECK(text_vertices[1].y == 50.0f - 42.0f, "vertical centring moves the line down by (box - height) / 2 (%g)",
		text_vertices[1].y);

	format = make_format(0.0f, 0.0f, 0, 1, 0);
	callback_chars = 0;
	callback_order_ok = 1;
	layout(font, "ABC", &format, (void *)count_chars, NULL);
	CHECK(callback_chars == 3 && callback_order_ok, "the callback sees each character with its vertices");
}

/* ---------- projection */

static void set_up_view(void)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		DAT_002bce18[i] = (unsigned int)i << 16;
		DAT_002bc490[i] = (unsigned int)i << 8;
		DAT_002bd640[i] = (unsigned int)i;
		DAT_002bc898[i] = (unsigned int)i << 24;
	}
	DAT_002bc464 = &DAT_002bcc98;
	memcpy(DAT_002be330, DAT_002855f0, sizeof(DAT_002be330));
	memset(DAT_002be570, 0, sizeof(DAT_002be570));
	DAT_002be5cc = 0;
	DAT_002be580 = 1.0f;
	DAT_002be584 = 1000.0f;
	DAT_002be588 = 1.5707964f;
	DAT_002be58c = 1.0f;
	DAT_002be57c = 0;
	bdSetViewport(0, 0, 640, 480);
	bdSetFieldOfView(1.5707964f, 0.0f);
	bdSetProjectionMode(1);
	bdSetWorldIdentity();
}

static void test_projection(void)
{
	float centre[4] = { 0.0f, 0.0f, 100.0f, 1.0f };
	float right[4] = { 100.0f, 0.0f, 100.0f, 1.0f };
	float top[4] = { 0.0f, 75.0f, 100.0f, 1.0f };
	float behind[4] = { 0.0f, 0.0f, -100.0f, 1.0f };
	float out[4], width, height;
	D3DMATRIX view;

	set_up_view();
	CHECK(DAT_002bcc98.viewport[2] == 640 && DAT_002bcc98.viewport[3] == 480,
		"the viewport is kept in the render target");
	CHECK(near_equal(DAT_002be58c, 2.0f * atanf(0.75f), 1e-5f), "fov y 0 derives from fov x and the aspect");
	bdProjectPoints(out, centre, 1);
	CHECK(near_equal(out[0], 0.0f, 1e-3f) && near_equal(out[1], 0.0f, 1e-3f) && 0.0f < out[2] && out[2] < 1.0f,
		"a point on the view axis projects to the centre (%g %g depth %g)", out[0], out[1], out[2]);
	bdProjectPoints(out, right, 1);
	CHECK(near_equal(out[0], 320.0f, 0.05f), "the edge of the field of view is the screen edge (%g)", out[0]);
	bdProjectPoints(out, top, 1);
	CHECK(near_equal(out[1], 240.0f, 0.05f), "y is up (%g)", out[1]);
	CHECK(bdProjectSize(centre, 10.0f, 10.0f, &width, &height) == 100.0f && near_equal(width, 32.0f, 1e-3f),
		"a size in world units scales with 1 / depth (%g)", width);
	CHECK(bdProjectSize(behind, 10.0f, 10.0f, &width, &height) == 0.0f, "behind the camera there is no size");
	CHECK(bdSphereVisible(centre, 1.0f) == 1 && bdSphereVisible(behind, 1.0f) == 0, "sphere visibility");
	CHECK(bdBoxVisible(DAT_002855f0, -1.0f, 1.0f, -1.0f, 1.0f, 99.0f, 101.0f) == 1, "a box in view is inside");
	CHECK(bdBoxVisible(DAT_002855f0, -1.0f, 1.0f, -1.0f, 1.0f, -101.0f, -99.0f) == 0, "a box behind is outside");
	CHECK(bdBoxVisible(DAT_002855f0, 50.0f, 500.0f, -1.0f, 1.0f, 99.0f, 101.0f) == 2,
		"a box across the edge is partly visible");
	bdSetProjectionMode(0);
	D3DDevice_GetTransform(D3DTS_VIEW, &view);
	CHECK(PTR_DAT_00284c70 == DAT_002be2f0 && memcmp(&view, DAT_00284c30, sizeof(view)) == 0,
		"2D mode uses the 2D projection and an identity view");
	bdSetProjectionMode(1);
	CHECK(PTR_DAT_00284c70 == DAT_002be2b0, "3D mode uses the perspective projection");
}

/* port, display.aspect_ratio 16:9 (Hor+, xbRender.c screen_widening): the game gives its
fields of view for 4:3, so on an 854-column screen what was at the 4:3 edge stays at 320
from the centre (no stretch), the vertical field of view stays, the view reaches further to
the side (to 427), and an object there is not culled. A render target of the game's keeps
its shape. */
static void test_widescreen_projection(void)
{
	float edge43[4] = { 100.0f, 0.0f, 100.0f, 1.0f };
	float edge169[4] = { 133.4375f, 0.0f, 100.0f, 1.0f };
	float out[4];
	bdRenderTarget other;

	DAT_002bc40c = 854;
	set_up_view();
	bdSetViewport(0, 0, 854, 480);
	bdSetFieldOfView(1.5707964f, 0.0f);
	CHECK(near_equal(DAT_002be58c, 2.0f * atanf(0.75f), 1e-5f), "16:9: fov y is 4:3's (%g)", DAT_002be58c);
	bdProjectPoints(out, edge43, 1);
	CHECK(near_equal(out[0], 320.0f, 0.05f), "16:9: the 4:3 edge stays 320 from the centre (%g)", out[0]);
	bdProjectPoints(out, edge169, 1);
	CHECK(near_equal(out[0], 427.0f, 0.05f), "16:9: the view reaches 427 to the side (%g)", out[0]);
	CHECK(bdBoxVisible(DAT_002855f0, 105.0f, 125.0f, -1.0f, 1.0f, 99.0f, 101.0f) == 1,
		"16:9: a box beyond the 4:3 edge is visible, not culled");
	CHECK(bdSphereVisible(edge169, 1.0f) == 1, "16:9: a sphere at the new edge is visible");
	memset(&other, 0, sizeof(other));
	DAT_002bc464 = &other;
	bdSetViewport(0, 0, 640, 480);
	bdSetFieldOfView(1.5707964f, 0.0f);
	bdProjectPoints(out, edge43, 1);
	CHECK(near_equal(out[0], 320.0f, 0.05f) && near_equal(DAT_002be2b0[0], 1.0f, 1e-5f),
		"16:9: a render target keeps the 4:3 projection (%g)", out[0]);
	DAT_002bc464 = &DAT_002bcc98;
	DAT_002bc40c = 0;
	set_up_view();
}

static void test_sprite(void)
{
	float pos[4] = { 0.0f, 0.0f, 100.0f, 1.0f };
	float behind[4] = { 0.0f, 0.0f, -100.0f, 1.0f };
	bdTLVertex v[4];

	set_up_view();
	CHECK(bdDrawSprite(pos, 10.0f, 20.0f, 0.0f, NULL, 0, 0, 1, 2, 3, 4, (float *)v, 0.0f) == 1, "drawn");
	CHECK(v[0].x == 330.0f && v[0].y == 220.0f && v[1].y == 260.0f && v[2].x == 310.0f && v[3].x == 310.0f,
		"the quad surrounds the projected point (%g,%g %g,%g)", v[0].x, v[0].y, v[3].x, v[3].y);
	CHECK(v[0].u == 1.0f && v[0].v == 0.0f && v[3].u == 0.0f && v[3].v == 1.0f, "texture coordinates");
	CHECK(v[0].color == BD_RGBA(1, 2, 3, 4), "colour");
	bdDrawSprite(pos, 10.0f, 20.0f, 0.0f, NULL, 1, 1, 1, 2, 3, 4, (float *)v, 0.0f);
	CHECK(v[0].u == 0.0f && v[0].v == 1.0f && v[3].u == 1.0f && v[3].v == 0.0f, "flips swap the coordinates");
	CHECK(bdDrawSprite(behind, 10.0f, 20.0f, 0.0f, NULL, 0, 0, 1, 2, 3, 4, (float *)v, 0.0f) == 0,
		"nothing behind the camera");
}

/* ---------- near clipping */

static void test_near_clip(void)
{
	float a[13], b[13];

	memcpy(DAT_002be470, DAT_002855f0, sizeof(DAT_002be470));
	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	a[2] = 1.0f;
	a[3] = 1.0f;
	a[8] = 0.0f;
	a[12] = 2.0f;
	b[2] = 5.0f;
	b[3] = 1.0f;
	b[8] = 100.0f;
	b[12] = 6.0f;
	CHECK(bdClipSegmentNear(a, b) == 1, "the end behind the plane is moved");
	CHECK(near_equal(a[2], 3.0f, 1e-5f) && near_equal(a[8], 50.0f, 1e-3f) && near_equal(a[12], 4.0f, 1e-5f),
		"onto the plane, with colour and width interpolated (%g %g %g)", a[2], a[8], a[12]);
	a[2] = 1.0f;
	b[2] = 2.0f;
	CHECK(bdClipSegmentNear(a, b) == 0, "a segment behind the plane is dropped");
	a[2] = 4.0f;
	b[2] = 5.0f;
	CHECK(bdClipSegmentNear(a, b) == 3, "a segment in front is kept");
}

/* ---------- vertex buffer rotation, error names */

static void test_vertex_buffer_rotation(void)
{
	bdVertexBuffer vb;

	memset(&vb, 0, sizeof(vb));
	vb.numBuffers = 3;
	CHECK(bdVertexBufferUnlock(&vb) == 0 && vb.drawBuffer == 0 && vb.current == 1, "first unlock");
	CHECK(bdVertexBufferUnlock(&vb) == 0 && vb.drawBuffer == 1 && vb.current == 2, "second unlock");
	CHECK(bdVertexBufferUnlock(&vb) == 1 && vb.drawBuffer == 2 && vb.current == 0, "the rotation wraps");
}

static void test_error_names(void)
{
	CHECK(strcmp(bdGetErrorString((int)0x8876086c), "D3DERR_INVALIDCALL") == 0, "a D3D error");
	CHECK(strcmp(bdGetErrorString((int)0x8007000e), "DSERR_OUTOFMEMORY") == 0, "a DirectSound error");
	CHECK(strcmp(bdGetErrorString(0x1234), "Unknown Error") == 0, "an unknown code");
}

int main(void)
{
	fprintf(stderr, "test_fvf\n");
	test_fvf();
	fprintf(stderr, "test_render_states\n");
	test_render_states();
	fprintf(stderr, "test_texture_frames\n");
	test_texture_frames();
	fprintf(stderr, "test_texture_stages\n");
	test_texture_stages();
	fprintf(stderr, "test_text_layout\n");
	test_text_layout();
	fprintf(stderr, "test_projection\n");
	test_projection();
	fprintf(stderr, "test_widescreen_projection\n");
	test_widescreen_projection();
	fprintf(stderr, "test_sprite\n");
	test_sprite();
	fprintf(stderr, "test_near_clip\n");
	test_near_clip();
	fprintf(stderr, "test_vertex_buffer_rotation\n");
	test_vertex_buffer_rotation();
	fprintf(stderr, "test_error_names\n");
	test_error_names();
	fprintf(stderr, "bdisplay tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
