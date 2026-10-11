/*
ClassLib/COverlay.c  (classlib_ui)
Original: C:\BlitzParty\Game\Src\ClassLib\COverlay.cpp

2D overlays (images, text and small 3D models drawn over the game), kept in a tree:
every overlay is a list head of its children and a node in its parent's list.

COverlay object (0x40c bytes, offsets from the object start; fields 0x5c-0x40b are a
copy of the creation descriptor, so "desc + N" is "object + 0x5c + N"):

  0x000  list head of the children (CLList_*)          0x00c  parent list
  0x010  node in the parent's list (next, prev)         0x018  update handler count
  0x01c  update handlers (array of void (*)(COverlay*))  0x020  draw function
  0x050  set when a colour fade ends                    0x054  hidden (short)
  0x058  -                                              0x05c  extra object (glow text)
  0x060  texture (image) / 0x074 actor (model)          0x078-0x094 model light setup
  0x098, 0x09c  text box size                           0x0a0  font (text)
  0x0a4  inline text (UTF-16)                           0x2a4  text pointer (or 0: inline)
  0x2a8  horizontal alignment (short: 0 left, 1 centre, 2 right)
  0x2aa  vertical alignment (short: 0 top, 1 centre, 3 bottom)
  0x2ac, 0x2b0, 0x2b4  colour r, g, b (0-255 floats)    0x2b8  alpha (0-256)
  0x2bc  colour fade rate (int)                         0x2c0, 0x2c4  alpha max / min
  0x2c8-0x2d0  colour max r, g, b (ints)                0x2d4-0x2dc  colour min r, g, b
  0x2e0  move speed                                     0x2e8  update handler bits
  0x2ec  flags (0x80 colour fade, 0x100 render target, 0x200 glow object, 0x400 tinted
         model, 0x800 additive, 0x1000 draw last, 0x2000 mirrored model, 0x4000 model
         shadow, 0x8000 drop shadow, 0x10000 glow text also drawn plainly)
  0x2f0  move flags (bit 0: moving)                     0x2f4  colour fade flags
  0x2f8  scale animation flags                          0x300  position x, y, z, w
  0x310  position relative to the parent                0x364, 0x368  width, height
  0x36c  texture width                                  0x374, 0x37c, 0x384  scale x, y, rotation
  0x378, 0x380, 0x388  their speeds                     0x38c-0x3a0  their min / max
  0x3a4  glow amount                                    0x3a8  glow target, 0x3ac glow speed
  0x3b0-0x3bc  render target size and format            0x3c0-0x3cc  render target info
  0x3d0  render target                                  0x3e0  model shadow size
  0x3e8-0x404  texture coordinates u0..u3, v0..v3

The update handlers come from the table at 0x00290938 (one per bit of 0x2e8, in bit
order): 0 colour fade (0x00072870), 1 scale (0x00072ab0), 2 move (0x00073e00), 3 glow
(0x00072800). The draw functions are the next three entries: image (0x00072bf0), text
(0x000730f0), model (0x00073eb0).
*/

#include "classlib_ui.h"

/* ---------- field access */

#define OV_F(o, off) (*(float *)((o) + (off)))
#define OV_I(o, off) (*(int *)((o) + (off)))
#define OV_U(o, off) (*(unsigned int *)((o) + (off)))
#define OV_S(o, off) (*(short *)((o) + (off)))

/* ---------- globals (classlib_ui_data.h) */

#define clu_time_scale CLU_VAR(float, 0x0028fda8)
/* 0x00290938  update handlers and draw functions, indexed by bit (see above) */
#define clu_overlay_functions CLU_ARR(uint32_t, 0x00290938)
/* 0x00290954  overlays are drawn */
#define clu_overlays_visible CLU_VAR(int, 0x00290954)
/* 0x00290958  overlay pass enabled */
#define clu_overlay_pass CLU_VAR(int, 0x00290958)
/* 0x0029095c  models are drawn (-1: set up only) */
#define clu_overlay_models CLU_VAR(int, 0x0029095c)
/* 0x002eca30 / 0x002eca38  text scale of the text drawer */
#define clu_text_scale_x CLU_VAR(float, 0x002eca30)
#define clu_text_scale_y CLU_VAR(float, 0x002eca38)
/* 0x002eca34  model render state set up this frame */
#define clu_models_set_up CLU_VAR(int, 0x002eca34)
/* 0x002eca3c  root overlay list */
#define clu_overlay_root CLU_VAR(int, 0x002eca3c)
/* 0x002eca40  overlay drawn last */
#define clu_overlay_current CLU_VAR(int, 0x002eca40)
/* 0x002eca48  quad of the image drawer */
#define clu_image_quad CLU_ARR(struct CLQuadVertex, 0x002eca48)
/* 0x002ecad8  shadow quad of the model drawer */
#define clu_shadow_quad CLU_ARR(struct CLQuadVertex, 0x002ecad8)
/* 0x002ece70  quad of the volume bar drawer (FUN_00074c10) */
#define clu_bar_quad CLU_ARR(struct CLQuadVertex, 0x002ece70)
/* 0x002ecfac  HUD object (Hud.cpp) */
#define clu_hud CLU_VAR(int, 0x002ecfac)

/* port (src/d3d8/d3d8_gl.c): display.anti_aliasing's pass (FXAA) over the screen, once a
frame; the original has no such call */
void halo_screen_anti_alias(short x0, short y0, short x1, short y1);

/* globals of other modules (tentative definitions in classlib_ui_extern.c) */
extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bc898[256];
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bd640[256];
extern float DAT_002be580; /* projection near / far / field of view / aspect (bdisplay) */
extern float DAT_002be584;
extern float DAT_002be588;
extern float DAT_002be58c;
extern int DAT_002844cc;   /* model shadow colour r, g, b (core) */
extern int DAT_002844d0;
extern int DAT_002844d4;
extern undefined4 DAT_00284b10; /* actor tint r, g, b (bactor) */
extern undefined4 DAT_00284b14;
extern undefined4 DAT_00284b18;

/* bdisplay functions whose table prototypes do not match the calls (see the report) */
#define CL_SetRenderState3(a, b, c) ((void (*)(int, int, int))FUN_00046cb0)((a), (b), (c))
#define CL_SetClipPlanes(n, f) ((void (*)(float, float))FUN_00044810)((n), (f))
#define CL_SetRenderTargetColour(t, r, g, b, a, s, m) \
	((int (*)(int, int, int, int, int, float, int))FUN_00045330)((t), (r), (g), (b), (a), (s), (m))
#define CL_SetActorTint(r, g, b) ((void (*)(int, int, int))FUN_00038440)((r), (g), (b))

/* ---------- update handlers */

/* 0x00072730  ClassLib/COverlay.cpp
Creates the render target of an overlay (flag 0x100) and takes its size. */
int COverlay_CreateRenderTarget(int self)
{
	int target;
	float width;

	target = (int)FUN_00044e90(OV_I(self, 0x3b0), OV_I(self, 0x3b4), OV_I(self, 0x3b8), OV_I(self, 0x3bc), 0);
	OV_I(self, 0x3d0) = target;
	if (target == 0)
		return 0;
	FUN_00044a70((struct bdRenderTarget *)target, (undefined4 *)(self + 0x3c0), (undefined4 *)(self + 0x3c4), (undefined4 *)(self + 0x3c8),
		     (undefined4 *)(self + 0x3cc));
	width = (float)OV_I(self, 0x3c0);
	OV_F(self, 0x36c) = width;
	OV_F(self, 0x364) = width;
	OV_F(self, 0x368) = (float)OV_I(self, 0x3c4);
	return 1;
}

/* 0x000727b0  ClassLib/COverlay.cpp
Initialises a text overlay from its descriptor (the original passes self in EBX and
desc in EAX). */
void COverlay_InitText(int self, undefined4 *desc)
{
	memcpy((void *)(self + 0x5c), desc, 0xec * 4);
	if (desc[0x11] == 0)
		OV_I(self, 0xa0) = (int)FUN_0004bb30(0, (char *)(desc + 0xb1), 3, 0xdefa);
	OV_U(self, 0x20) = clu_overlay_functions[5];
}

/* 0x00072800  ClassLib/COverlay.cpp
Update handler 3: moves the glow amount towards its target. */
void COverlay_UpdateGlow(int self)
{
	float step = clu_time_scale * OV_F(self, 0x3ac);

	if ((double)step + (double)OV_F(self, 0x3a8) < (double)OV_F(self, 0x3a4)) {
		OV_F(self, 0x3a4) = OV_F(self, 0x3a4) - step;
		return;
	}
	if ((double)OV_F(self, 0x3a4) < (double)OV_F(self, 0x3a8) - (double)step) {
		OV_F(self, 0x3a4) = step + OV_F(self, 0x3a4);
		return;
	}
	OV_F(self, 0x3a4) = OV_F(self, 0x3a8);
}

/* 0x00072870  ClassLib/COverlay.cpp
Update handler 0: fades the colour (flag 0x80 or fade flag 8) or the alpha between
their limits; clears fade flag 1 (and sets 0x50) at a limit. */
void COverlay_UpdateFade(int self)
{
	unsigned int fade = OV_U(self, 0x2f4);
	float rate, max_r, max_g, max_b, min_r, min_g, min_b, limit;

	if ((fade & 1) == 0)
		return;
	if (*(signed char *)(self + 0x2ec) < 0 || (fade & 8) != 0) {
		rate = (float)OV_I(self, 0x2bc);
		OV_F(self, 0x2ac) = (float)((double)clu_time_scale * rate + OV_F(self, 0x2ac));
		OV_F(self, 0x2b0) = (float)((double)clu_time_scale * rate + OV_F(self, 0x2b0));
		OV_F(self, 0x2b4) = (float)((double)rate * clu_time_scale + OV_F(self, 0x2b4));
		max_r = (float)OV_I(self, 0x2c8);
		if (max_r < OV_F(self, 0x2ac))
			OV_F(self, 0x2ac) = max_r;
		max_g = (float)OV_I(self, 0x2cc);
		if (max_g < OV_F(self, 0x2b0))
			OV_F(self, 0x2b0) = max_g;
		max_b = (float)OV_I(self, 0x2d0);
		if (max_b < OV_F(self, 0x2b4))
			OV_F(self, 0x2b4) = max_b;
		min_r = (float)OV_I(self, 0x2d4);
		if (OV_F(self, 0x2ac) < min_r)
			OV_F(self, 0x2ac) = min_r;
		min_g = (float)OV_I(self, 0x2d8);
		if (OV_F(self, 0x2b0) < min_g)
			OV_F(self, 0x2b0) = min_g;
		min_b = (float)OV_I(self, 0x2dc);
		if (OV_F(self, 0x2b4) < min_b)
			OV_F(self, 0x2b4) = min_b;
		if (OV_F(self, 0x2ac) == max_r && OV_F(self, 0x2b0) == max_g && OV_F(self, 0x2b4) == max_b) {
			OV_I(self, 0x50) = 1;
			OV_U(self, 0x2f4) = fade & 0xfffffffe;
		}
		if (min_r == OV_F(self, 0x2ac) && OV_F(self, 0x2b0) == min_g && OV_F(self, 0x2b4) == min_b) {
			OV_I(self, 0x50) = 1;
			OV_U(self, 0x2f4) = OV_U(self, 0x2f4) & 0xfffffffe;
		}
		return;
	}
	OV_F(self, 0x2b8) = (float)((double)OV_I(self, 0x2bc) * clu_time_scale + OV_F(self, 0x2b8));
	limit = (float)OV_I(self, 0x2c0);
	if (limit < OV_F(self, 0x2b8)) {
		OV_F(self, 0x2b8) = limit;
		OV_U(self, 0x2f4) = fade & 0xfffffffe;
	}
	limit = (float)OV_I(self, 0x2c4);
	if (OV_F(self, 0x2b8) < limit) {
		OV_F(self, 0x2b8) = limit;
		OV_U(self, 0x2f4) = OV_U(self, 0x2f4) & 0xfffffffe;
	}
}

/* 0x00072ab0  ClassLib/COverlay.cpp
Update handler 1: animates scale x, y and rotation between their limits; stops when
all three are clamped. */
void COverlay_UpdateScale(int self)
{
	int clamped;
	float value;

	if ((OV_U(self, 0x2f8) & 1) == 0)
		return;
	clamped = 1;
	OV_F(self, 0x374) = clu_time_scale * OV_F(self, 0x378) + OV_F(self, 0x374);
	OV_F(self, 0x37c) = clu_time_scale * OV_F(self, 0x380) + OV_F(self, 0x37c);
	OV_F(self, 0x384) = clu_time_scale * OV_F(self, 0x388) + OV_F(self, 0x384);
	if (OV_F(self, 0x374) < OV_F(self, 0x38c))
		OV_F(self, 0x374) = OV_F(self, 0x38c);
	else if (OV_F(self, 0x374) <= OV_F(self, 0x390))
		clamped = 0;
	else
		OV_F(self, 0x374) = OV_F(self, 0x390);
	if (OV_F(self, 0x37c) < OV_F(self, 0x394))
		OV_F(self, 0x37c) = OV_F(self, 0x394);
	else if (OV_F(self, 0x37c) <= OV_F(self, 0x398))
		clamped = 0;
	else
		OV_F(self, 0x37c) = OV_F(self, 0x398);
	if (OV_F(self, 0x384) < OV_F(self, 0x39c)) {
		value = OV_F(self, 0x39c);
	} else {
		if (OV_F(self, 0x384) <= OV_F(self, 0x3a0))
			return;
		value = OV_F(self, 0x3a0);
	}
	OV_F(self, 0x384) = value;
	if (clamped)
		OV_U(self, 0x2f8) = OV_U(self, 0x2f8) & 0xfffffffe;
}

/* ---------- draw functions */

/* 0x00072bf0  ClassLib/COverlay.cpp
Draw function of image overlays. With `shadow` set it draws the black drop shadow
(one pixel right and down); flag 0x8000 draws that shadow first. */
void COverlay_DrawImage(int self, int shadow)
{
	double x;
	float y;
	int r, g, b;
	unsigned int cr, cg, cb;

	if (shadow == 0 && (*(unsigned int *)(self + 0x2ec) & 0x8000) != 0)
		COverlay_DrawImage(self, 1);
	CL_SetRenderState3(3, 0, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00035d50(0x100);
	if (shadow == 0 && *(signed char *)(self + 0x2ec) < 0)
		FUN_00044b00(10, 1, 0);
	else if (shadow == 0 && (OV_U(self, 0x2ec) & 0x800) != 0)
		FUN_00044b00(10, 3, 0);
	else
		FUN_00044b00(10, 0, 0);
	if ((OV_U(self, 0x2ec) & 0x100) == 0) {
		FUN_000416c0(0, (struct bdTexture *)OV_I(self, 0x60));
	} else {
		r = (int)OV_F(self, 0x2ac);
		g = (int)OV_F(self, 0x2b0);
		b = (int)OV_F(self, 0x2b4);
		CL_SetRenderTargetColour(0, r, g, b, (int)OV_F(self, 0x2b8), 1.0f, 2);
		FUN_000455a0((struct bdRenderTarget *)OV_I(self, 0x3d0), 0);
	}
	x = (double)OV_F(self, 0x300);
	y = OV_F(self, 0x304);
	if (OV_S(self, 0x2a8) == 1)
		x = x - (double)OV_F(self, 0x364) * (double)0.5f;
	else if (OV_S(self, 0x2a8) == 2)
		x = x - (double)OV_F(self, 0x364);
	if (OV_S(self, 0x2aa) == 1)
		y = (float)((double)y - (double)OV_F(self, 0x368) * (double)0.5f);
	else if (OV_S(self, 0x2aa) == 3)
		y = (float)((double)y - (double)OV_F(self, 0x368));
	if (shadow != 0) {
		x = x + (double)1.0f;
		y = (float)((double)y - (double)1.0f);
		r = 0;
		g = 0;
		b = 0;
	} else {
		r = (int)OV_F(self, 0x2ac);
		g = (int)OV_F(self, 0x2b0);
		b = (int)OV_F(self, 0x2b4);
	}
	cr = DAT_002bce18[r];
	cg = DAT_002bc490[g];
	cb = DAT_002bd640[b];
	if ((*(unsigned char *)(self + 0x2e8) & 2) != 0) {
		/* scaled about the centre */
		double hw = (double)OV_F(self, 0x364) * (double)0.5f * (double)OV_F(self, 0x374);
		double hh = (double)OV_F(self, 0x368) * (double)0.5f * (double)OV_F(self, 0x37c);

		clu_image_quad[0].x = (float)(x - hw);
		clu_image_quad[0].z = 0.0f;
		clu_image_quad[0].y = (float)((double)y - hh);
		clu_image_quad[0].colour = DAT_002bc898[(int)OV_F(self, 0x2b8)] | cr | cg | cb;
		clu_image_quad[0].u = 0.0f;
		clu_image_quad[0].v = 1.0f;
		clu_image_quad[1].x = (float)(hw + x);
		clu_image_quad[1].z = 0.0f;
		clu_image_quad[1].y = (float)((double)y - hh);
		clu_image_quad[1].colour = DAT_002bc898[(int)OV_F(self, 0x2b8)] | cr | cg | cb;
		clu_image_quad[1].u = 1.0f;
		clu_image_quad[1].v = 1.0f;
		clu_image_quad[2].x = (float)(hw + x);
		clu_image_quad[2].z = 0.0f;
		clu_image_quad[2].y = (float)(hh + (double)y);
		clu_image_quad[2].colour = DAT_002bc898[(int)OV_F(self, 0x2b8)] | cr | cg | cb;
		clu_image_quad[2].u = 1.0f;
		clu_image_quad[2].v = 0.0f;
		clu_image_quad[3].x = (float)(x - hw);
		clu_image_quad[3].z = 0.0f;
		clu_image_quad[3].y = (float)(hh + (double)y);
		clu_image_quad[3].colour = DAT_002bc898[(int)OV_F(self, 0x2b8)] | cr | cg | cb;
		clu_image_quad[3].u = 0.0f;
		clu_image_quad[3].v = 0.0f;
	} else {
		clu_image_quad[0].x = (float)x;
		clu_image_quad[0].y = y;
		clu_image_quad[0].z = 0.0f;
		clu_image_quad[0].colour = DAT_002bc898[(int)OV_F(self, 0x2b8)] | cr | cg | cb;
		clu_image_quad[0].u = OV_F(self, 0x3e8);
		clu_image_quad[0].v = OV_F(self, 0x3f8);
		clu_image_quad[1].x = (float)(x + (double)OV_F(self, 0x364));
		clu_image_quad[1].y = y;
		clu_image_quad[1].z = 0.0f;
		clu_image_quad[1].colour = DAT_002bc898[(int)OV_F(self, 0x2b8)] | cr | cg | cb;
		clu_image_quad[1].u = OV_F(self, 0x3ec);
		clu_image_quad[1].v = OV_F(self, 0x3fc);
		clu_image_quad[2].x = (float)(x + (double)OV_F(self, 0x364));
		clu_image_quad[2].y = (float)((double)y - (double)OV_F(self, 0x368));
		clu_image_quad[2].z = 0.0f;
		clu_image_quad[2].colour = DAT_002bc898[(int)OV_F(self, 0x2b8)] | cr | cg | cb;
		clu_image_quad[2].u = OV_F(self, 0x3f0);
		clu_image_quad[2].v = OV_F(self, 0x400);
		clu_image_quad[3].x = (float)x;
		clu_image_quad[3].y = (float)((double)y - (double)OV_F(self, 0x368));
		clu_image_quad[3].z = 0.0f;
		clu_image_quad[3].colour = DAT_002bc898[(int)OV_F(self, 0x2b8)] | cr | cg | cb;
		clu_image_quad[3].u = OV_F(self, 0x3f4);
		clu_image_quad[3].v = OV_F(self, 0x404);
	}
	FUN_0003f2d0(5, (void *)clu_image_quad, 4);
	FUN_000416c0(0, 0);
	FUN_00046d00(3);
}

/* text layout passed to the text drawer (FUN_00043400 / CLSprite_DrawBlurred) */
struct CLTextBox {
	float x, y;          /* 0x00 */
	float width, height; /* 0x08 */
	int halign, valign;  /* 0x10 0 left, 2 centre, 3 right / 1 top, 2 centre, 4 bottom */
	undefined4 box_w;    /* 0x18 */
	undefined4 box_h;    /* 0x1c */
};

/* the text drawer: font, text, length, box, r, g, b, a, callback, context */
#define CL_DrawText(font, text, len, box, r, g, b, a) \
	FUN_00043400((struct bdFont *)(font), (const unsigned short *)(text), (int)(len), (struct bdPrintFormat *)(box), (r), (g), (b), (a), 0, 0)

/* 0x000730f0  ClassLib/COverlay.cpp
Draw function of text overlays: plain text, or glowing text (glow amount 0x3a4) drawn
blurred, with an optional drop shadow (flag 0x8000). */
void COverlay_DrawText(int self, int shadow)
{
	struct CLTextBox box;
	float scale_rotate[16];
	float translate[16];
	float world[16];
	unsigned short *text;
	int len;
	int a;
	float spread;

	if (shadow == 0 && (OV_U(self, 0x2ec) & 0x8000) != 0)
		COverlay_DrawText(self, 1);
	CL_SetRenderState3(3, 0, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 0, 0);
	if (shadow != 0) {
		box.x = OV_F(self, 0x300) + 1.0f;
		box.y = OV_F(self, 0x304) - 1.0f;
	} else {
		box.x = OV_F(self, 0x300);
		box.y = OV_F(self, 0x304);
	}
	box.height = OV_F(self, 0x368);
	box.width = OV_F(self, 0x364);
	box.box_h = OV_U(self, 0x9c);
	box.box_w = OV_U(self, 0x98);
	if (OV_S(self, 0x2a8) == 0)
		box.halign = 0;
	else if (OV_S(self, 0x2a8) == 1)
		box.halign = 2;
	else if (OV_S(self, 0x2a8) == 2)
		box.halign = 3;
	if (OV_S(self, 0x2aa) == 0)
		box.valign = 1;
	else if (OV_S(self, 0x2aa) == 1)
		box.valign = 2;
	else if (OV_S(self, 0x2aa) == 3)
		box.valign = 4;
	text = (unsigned short *)OV_I(self, 0x2a4);
	if (text == NULL)
		text = (unsigned short *)(self + 0xa4);

	if (OV_F(self, 0x3a4) == 0.0f) {
		/* plain text */
		clu_text_scale_x = OV_F(self, 0x374);
		clu_text_scale_y = OV_F(self, 0x37c);
		if ((*(unsigned char *)(self + 0x2e8) & 2) != 0) {
			FUN_000502a0((undefined4 *)scale_rotate, OV_F(self, 0x374), OV_F(self, 0x37c), OV_F(self, 0x384));
			FUN_00050250((undefined4 *)translate, OV_F(self, 0x300), OV_F(self, 0x304), OV_F(self, 0x308));
			/* the original picks 0x0004ff30 for unaligned matrices; both multiply */
			FUN_00050150(world, translate, scale_rotate);
			FUN_00043930((undefined4 *)world);
			box.y = 0.0f;
			box.x = 0.0f;
		}
		if (shadow != 0) {
			box.x = box.x + 1.0f;
			box.y = box.y - 1.0f;
			a = (int)OV_F(self, 0x2b8);
			len = FUN_0004b110((short *)text);
			CL_DrawText(OV_I(self, 0xa0), text, len, &box, 0, 0, 0, a);
		} else {
			a = (int)OV_F(self, 0x2b8);
			len = FUN_0004b110((short *)text);
			CL_DrawText(OV_I(self, 0xa0), text, len, &box, (int)OV_F(self, 0x2ac), (int)OV_F(self, 0x2b0),
				    (int)OV_F(self, 0x2b4), a);
		}
		FUN_000438b0();
	} else if ((*(unsigned char *)(self + 0x2e8) & 2) != 0) {
		/* glowing, scaled */
		spread = OV_F(self, 0x3a4) * 3.0f;
		a = (int)OV_F(self, 0x2b8);
		len = FUN_0004b110((short *)text);
		if (shadow != 0)
			CLSprite_DrawBlurred(OV_U(self, 0xa0), (undefined4)text, len, (float *)&box, 0, 0, 0, a, spread,
					     OV_F(self, 0x37c), 0);
		else
			CLSprite_DrawBlurred(OV_U(self, 0xa0), (undefined4)text, len, (float *)&box,
					     (int)OV_F(self, 0x2ac), (int)OV_F(self, 0x2b0), (int)OV_F(self, 0x2b4), a,
					     spread, OV_F(self, 0x37c), 0);
	} else {
		/* glowing */
		if (shadow != 0) {
			spread = OV_F(self, 0x3a4) * 3.0f;
			a = (int)((double)OV_F(self, 0x2b8) * (double)OV_F(self, 0x3a4));
			len = FUN_0004b110((short *)text);
			CLSprite_DrawBlurred(OV_U(self, 0xa0), (undefined4)text, len, (float *)&box, 0, 0, 0, a, spread,
					     1.0f, 0);
		}
		if ((OV_U(self, 0x2ec) & 0x10000) != 0) {
			a = (int)OV_F(self, 0x2b8);
			len = FUN_0004b110((short *)text);
			CL_DrawText(OV_I(self, 0xa0), text, len, &box, (int)OV_F(self, 0x2ac), (int)OV_F(self, 0x2b0),
				    (int)OV_F(self, 0x2b4), a);
		}
		spread = OV_F(self, 0x3a4) * 3.0f;
		a = (int)((double)OV_F(self, 0x2b8) * (double)OV_F(self, 0x3a4));
		len = FUN_0004b110((short *)text);
		CLSprite_DrawBlurred(OV_U(self, 0xa0), (undefined4)text, len, (float *)&box, (int)OV_F(self, 0x2ac),
				     (int)OV_F(self, 0x2b0), (int)OV_F(self, 0x2b4), a, spread, 1.0f, 0);
	}
	FUN_00044b00(6, 0, 0);
	FUN_00046d00(3);
}

/* ---------- creation */

/* 0x00073510  ClassLib/COverlay.cpp
Initialises an image overlay from its descriptor (the original passes self in EBX). */
void COverlay_InitImage(int self, undefined4 *desc)
{
	int texture;
	int glow;

	memcpy((void *)(self + 0x5c), desc, 0xec * 4);
	OV_F(self, 0x3e8) = 0.0f;
	OV_F(self, 0x3ec) = 1.0f;
	OV_F(self, 0x3f0) = 1.0f;
	OV_F(self, 0x3f4) = 0.0f;
	OV_F(self, 0x3f8) = 0.0f;
	OV_F(self, 0x3fc) = 0.0f;
	OV_F(self, 0x400) = 1.0f;
	OV_F(self, 0x404) = 1.0f;
	texture = desc[1];
	if (texture == 0)
		texture = (int)FUN_0004bb30(0, (char *)(desc + 0xb1), 0, 0xdefa);
	OV_I(self, 0x60) = texture;
	if ((OV_U(self, 0x2ec) & 0x200) != 0) {
		glow = (int)CLU_Alloc(0x18, "C:\\BlitzParty\\Game\\Src\\ClassLib\\COverlay.cpp", 0x134, 0x1001);
		if (glow == 0)
			glow = 0;
		else
			glow = (int)FUN_00082f60((struct CAnimTex *)glow, (char *)(self + 800), *(float *)&desc[0xa1], 0); /* the speed is a float */
		OV_I(self, 0x5c) = glow;
	}
	if (OV_F(self, 0x364) == 0.0f || OV_F(self, 0x368) == 0.0f) {
		texture = OV_I(self, 0x60);
		if (texture != 0) {
			OV_F(self, 0x36c) = (float)*(unsigned short *)(texture + 0x2e);
			OV_F(self, 0x364) = OV_F(self, 0x36c);
			OV_F(self, 0x368) = (float)*(unsigned short *)(texture + 0x30);
		}
	}
	OV_U(self, 0x20) = clu_overlay_functions[4];
}

/* 0x00073690  ClassLib/COverlay.cpp
Initialises a model overlay from its descriptor (the original passes self in EBX). */
void COverlay_InitModel(int self, undefined4 *desc)
{
	int resource;
	int actor;

	memcpy((void *)(self + 0x5c), desc, 0xec * 4);
	if (desc[6] != 0) {
		OV_U(self, 0x74) = desc[6];
	} else {
		resource = (int)FUN_0004bb30(0, (char *)(desc + 0xb1), 1, 0);
		if (resource == 0)
			return;
		actor = (int)FUN_00036b20(0, (struct BActor *)resource, 8, (const char *)"C:\\BlitzParty\\Game\\Src\\ClassLib\\COverlay.cpp", 0x173);
		OV_I(self, 0x74) = actor;
		FUN_00035cc0((struct BActorInstance *)actor, 0, 1, 0);
		FUN_00035cc0((struct BActorInstance *)OV_I(self, 0x74), 1, 0, 0);
		FUN_00034e70((struct BActorInstance *)OV_I(self, 0x74), (undefined4 *)OV_I(OV_I(self, 0x74), 0x134), 0x18, 1);
		FUN_00038190((void *)OV_I(self, 0x74), (struct BActorNode *)OV_I(OV_I(self, 0x74), 0x134), 1, 0);
	}
	OV_U(self, 0x78) = desc[7];
	OV_U(self, 0x7c) = desc[8];
	OV_U(self, 0x80) = desc[9];
	OV_U(self, 0x84) = desc[10];
	OV_U(self, 0x88) = desc[0xb];
	OV_U(self, 0x8c) = desc[0xc];
	OV_U(self, 0x90) = desc[0xd];
	OV_U(self, 0x94) = desc[0xe];
	actor = OV_I(self, 0x74);
	OV_U(actor, 0xc0) = desc[0xa9];
	OV_U(actor, 0xc4) = desc[0xaa];
	OV_U(actor, 0xc8) = desc[0xab];
	OV_U(actor, 0xcc) = desc[0xac];
	OV_U(OV_I(self, 0x74), 0xd0) = OV_U(self, 0x374);
	OV_U(OV_I(self, 0x74), 0xd4) = OV_U(self, 0x37c);
	OV_U(OV_I(self, 0x74), 0xd8) = OV_U(self, 0x384);
	FUN_00034bf0((struct BActorInstance *)OV_U(self, 0x74), 1);
	OV_U(self, 0x20) = clu_overlay_functions[6];
}

/* 0x000737f0  ClassLib/COverlay.cpp (thiscall)
Draws an overlay and, below it, its children (their positions follow the parent).
A render-target overlay (0x100) draws only when its target can be selected. */
void COverlay_Draw(int self)
{
	int node;
	int child;

	if (OV_S(self, 0x54) != 0)
		return;
	clu_overlay_current = self;
	if ((OV_U(self, 0x2ec) & 0x100) != 0) {
		if (CL_SetRenderTargetColour(OV_I(self, 0x3d0), 0, 0, 0, 0, 1.0f, 3) == 0)
			return;
		((void (*)(int, int))OV_U(self, 0x20))(self, 0);
		for (node = (int)CLList_Next((unsigned int *)self, NULL); node != 0;
		     node = (int)CLList_Next((unsigned int *)self, (unsigned int *)node)) {
			child = node - 0x10;
			OV_F(child, 0x300) = OV_F(self, 0x300) + OV_F(child, 0x310);
			OV_F(child, 0x304) = OV_F(child, 0x314) + OV_F(self, 0x304);
			COverlay_Draw(child);
		}
		return;
	}
	((void (*)(int, int))OV_U(self, 0x20))(self, 0);
	for (node = (int)CLList_Next((unsigned int *)self, NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)self, (unsigned int *)node)) {
		child = node - 0x10;
		OV_F(child, 0x300) = OV_F(child, 0x310) + OV_F(self, 0x300);
		OV_F(child, 0x304) = OV_F(child, 0x314) + OV_F(self, 0x304);
		COverlay_Draw(child);
	}
}

/* 0x00073900  ClassLib/COverlay.cpp
Draws the HUD and every root overlay (the one flagged 0x1000 last) with the overlay
camera (45 degrees, near 45, far 50000), then restores the camera. */
void COverlay_DrawAll(int no_z)
{
	int saved_state[34];
	float saved_near, saved_far, saved_fov, saved_aspect;
	int node;
	int child;
	int last;

	last = 0;
	if (clu_overlay_pass == 0)
		return;
	/* port: the 3D scene ends here and the HUD starts, so FXAA keeps the HUD sharp */
	halo_screen_anti_alias(0, 0, 640, 480);
	FUN_00046d40((undefined4 *)saved_state);
	if (clu_hud != 0)
		FUN_00078fb0(clu_hud);
	if (clu_overlays_visible == 0 || clu_overlay_root == 0)
		return;
	if (no_z != 0)
		FUN_00044b00(3, 0, 0);
	FUN_00043490(0, 0, 0);
	FUN_000434b0(0.0f, 0.0f, 0.0f, 0);
	saved_fov = DAT_002be588;
	saved_aspect = DAT_002be58c;
	FUN_00044b00(8, 0, 0);
	saved_near = DAT_002be580;
	saved_far = DAT_002be584;
	CL_SetClipPlanes(45.0f, 50000.0f);
	FUN_00044a00(1.2217305f, 0.91630563f);
	FUN_00043790();
	FUN_00083370();
	FUN_00083130();
	FUN_00044b00(8, 0, 0);
	FUN_00043b70(0);
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(4, 0, 0);
	FUN_00044b00(1, 2, 0);
	clu_models_set_up = 0;
	FUN_00044a00(1.2217305f, 0.91630563f);
	FUN_00043790();
	FUN_000438b0();
	FUN_00044b00(2, 0xc, 1);
	for (node = (int)CLList_Next((unsigned int *)clu_overlay_root, NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)clu_overlay_root, (unsigned int *)node)) {
		child = node - 0x10;
		if ((OV_U(child, 0x2ec) & 0x1000) != 0)
			last = child;
		else
			COverlay_Draw(child);
	}
	if (last != 0 && (OV_U(last, 0x2ec) & 0x1000) != 0)
		COverlay_Draw(last);
	FUN_00044a00(saved_fov, saved_aspect);
	CL_SetClipPlanes(saved_near, saved_far);
	FUN_00043790();
	FUN_00046d60(saved_state);
	FUN_00043b70(1);
}

/* 0x00073ae0  ClassLib/COverlay.cpp (thiscall: parent in ECX)
Creates a child overlay of `parent` from a descriptor (type at desc+0x288: 0 image,
1 model, 2 text). The descriptor's y is negated and it is cleared afterwards. */
int COverlay_CreateChild(int parent, undefined4 *desc)
{
	int self;

	self = (int)CLU_Alloc(0x40c, "C:\\BlitzParty\\Game\\Src\\ClassLib\\COverlay.cpp", 0x32d, 0x1001);
	if (self != 0) {
		CLList_Init((int *)self);
		memset((void *)(self + 0x5c), 0, 0xec * 4);
		OV_I(self, 0x10) = 0;
		OV_S(self, 0x54) = 0;
	} else {
		self = 0;
	}
	if (self == 0)
		return 0;
	OV_I(self, 0x10) = 0;
	memset((void *)(self + 0x5c), 0, 0xec * 4);
	if (desc[0xa2] == 0)
		COverlay_InitImage(self, desc);
	if (desc[0xa2] == 1)
		COverlay_InitModel(self, desc);
	if (desc[0xa2] == 2)
		COverlay_InitText(self, desc);
	*(float *)&desc[0xae] = -*(float *)&desc[0xae];
	OV_U(self, 0x310) = desc[0xad];
	OV_U(self, 0x314) = desc[0xae];
	OV_U(self, 0x318) = desc[0xaf];
	OV_U(self, 0x31c) = desc[0xb0];
	{
		/* the first count does not reset 0x58/0x54 (only the root version does) */
		short count = 0;
		int bit;

		for (bit = 0; bit < 32; bit++)
			if ((OV_U(self, 0x2e8) & (1u << bit)) != 0)
				count = count + 1;
		OV_I(self, 0x18) = count;
		OV_I(self, 0x1c) = 0;
		if (count != 0) {
			OV_I(self, 0x1c) = (int)FUN_0004c4b0(count * 4, 0,
							     (const char *)"C:\\BlitzParty\\Game\\Src\\ClassLib\\COverlay.cpp",
							     0x355, 0x2001);
			count = 0;
			OV_I(self, 0x58) = 0;
			OV_S(self, 0x54) = 0;
			for (bit = 0; bit < 32; bit++) {
				if ((OV_U(self, 0x2e8) & (1u << bit)) != 0) {
					((uint32_t *)OV_I(self, 0x1c))[count] = clu_overlay_functions[bit];
					count = count + 1;
				}
			}
		}
	}
	CLList_Insert((int *)parent, (int *)(self + 0x10), 1);
	OV_I(self, 0xc) = parent;
	memset(desc, 0, 0xec * 4);
	return self;
}

/* 0x00073e00  ClassLib/COverlay.cpp
Update handler 2: moves the overlay towards its target (0x64) at speed 0x2e0. */
void COverlay_UpdateMove(int self)
{
	float *pos = (float *)(self + 0x300);
	float delta[3];
	float dist;
	float step;

	delta[0] = OV_F(self, 0x64) - pos[0];
	delta[1] = OV_F(self, 0x68) - OV_F(self, 0x304);
	delta[2] = OV_F(self, 0x6c) - OV_F(self, 0x308);
	dist = sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
	step = clu_time_scale * OV_F(self, 0x2e0);
	if (dist > step) {
		FUN_00050ff0(pos, pos, 1.0f, delta, step / dist);
		return;
	}
	pos[0] = OV_F(self, 0x64);
	OV_U(self, 0x304) = OV_U(self, 0x68);
	OV_U(self, 0x308) = OV_U(self, 0x6c);
	OV_U(self, 0x30c) = OV_U(self, 0x70);
	if ((OV_U(self, 0x2f0) & 1) != 0)
		OV_U(self, 0x2f0) = OV_U(self, 0x2f0) & 0xfffffffe;
}

/* draws the model of a model overlay with its alpha (or tint, flag 0x400) */
static void COverlay_DrawModelPass(int self)
{
	undefined4 tint_r, tint_g, tint_b;

	if ((OV_U(self, 0x2ec) & 0x400) != 0) {
		FUN_00035d50(0x100);
		FUN_00044b00(3, 0, 0);
		FUN_00044b00(6, 1, 0);
		FUN_00044b00(10, 1, 0);
		tint_r = DAT_00284b10;
		tint_g = DAT_00284b14;
		tint_b = DAT_00284b18;
		CL_SetActorTint((int)((double)OV_F(self, 0x2ac) * (double)OV_F(self, 0x2b8) * (double)0.00390625f),
				(int)((double)OV_F(self, 0x2b0) * (double)OV_F(self, 0x2b8) * (double)0.00390625f),
				(int)((double)OV_F(self, 0x2b4) * (double)OV_F(self, 0x2b8) * (double)0.00390625f));
		FUN_00034d50((struct BActorInstance *)OV_I(self, 0x74), 7);
		FUN_00044b00(6, 0, 0);
		FUN_00038440(tint_r, tint_g, tint_b);
	} else if (OV_F(self, 0x2b8) > 0.0f && OV_F(self, 0x2b8) < 256.0f) {
		FUN_00044b00(6, 1, 0);
		FUN_00035d50((int)OV_F(self, 0x2b8));
		FUN_00044b00(10, 0, 0);
		FUN_00034d50((struct BActorInstance *)OV_I(self, 0x74), 7);
		FUN_00044b00(6, 0, 0);
		FUN_00035d50(0x100);
	} else if (OV_U(self, 0x2b8) == 0x43800000) { /* exactly 256.0f */
		FUN_00035d50(0x100);
		FUN_00044b00(6, 1, 0);
		FUN_00044b00(7, 3, 0);
		FUN_00044b00(5, 1, 0);
		FUN_00034d50((struct BActorInstance *)OV_I(self, 0x74), 7);
	}
}

/* 0x00073eb0  ClassLib/COverlay.cpp
Draw function of model overlays: the model (mirrored first with flag 0x2000, then its
floor shadow with 0x4000) with the overlay camera clip planes. */
void COverlay_DrawModel(int self)
{
	float saved_near, saved_far;
	int actor;
	unsigned int colour;
	float s;

	if (clu_overlay_models == 0)
		return;
	if (clu_models_set_up == 0) {
		FUN_00043b70(1);
		FUN_00044b00(5, 1, 0);
		clu_models_set_up = 1;
	}
	saved_near = DAT_002be580;
	saved_far = DAT_002be584;
	CL_SetClipPlanes(45.0f, 50000.0f);
	FUN_00043490(OV_F(self, 0x78), OV_F(self, 0x7c), OV_F(self, 0x80));
	FUN_000434b0(OV_F(self, 0x88), OV_F(self, 0x8c), OV_F(self, 0x90), 0);
	FUN_00043790();
	actor = OV_I(self, 0x74);
	OV_U(actor, 0xc0) = OV_U(self, 0x300);
	OV_U(actor, 0xc4) = OV_U(self, 0x304);
	OV_U(actor, 0xc8) = OV_U(self, 0x308);
	OV_U(actor, 0xcc) = OV_U(self, 0x30c);
	OV_U(OV_I(self, 0x74), 0xd0) = OV_U(self, 0x374);
	OV_U(OV_I(self, 0x74), 0xd8) = OV_U(self, 0x384);
	if ((OV_U(self, 0x2ec) & 0x2000) != 0) {
		OV_F(OV_I(self, 0x74), 0xd4) = -OV_F(self, 0x37c);
		FUN_00034bf0((struct BActorInstance *)OV_U(self, 0x74), 1);
		if (clu_overlay_models == -1)
			return;
		COverlay_DrawModelPass(self);
		if ((OV_U(self, 0x2ec) & 0x4000) != 0) {
			actor = OV_I(self, 0x74);
			s = OV_F(self, 0x3e0);
			memset(clu_shadow_quad, 0, 0x24 * 4);
			clu_shadow_quad[0].x = OV_F(actor, 0xc0) - s * 200.0f;
			clu_shadow_quad[0].y = OV_F(OV_I(self, 0x74), 0xc4);
			clu_shadow_quad[0].z = OV_F(OV_I(self, 0x74), 0xc8) - OV_F(self, 0x3e0) * 250.0f;
			colour = DAT_002bd640[DAT_002844d4] | DAT_002bc490[DAT_002844d0] | DAT_002bce18[DAT_002844cc];
			clu_shadow_quad[0].colour = colour | DAT_002bc898[150];
			clu_shadow_quad[1].x = OV_F(self, 0x3e0) * 200.0f + OV_F(OV_I(self, 0x74), 0xc0);
			clu_shadow_quad[1].y = OV_F(OV_I(self, 0x74), 0xc4);
			clu_shadow_quad[1].z = OV_F(OV_I(self, 0x74), 0xc8) - OV_F(self, 0x3e0) * 250.0f;
			clu_shadow_quad[1].colour = colour | DAT_002bc898[150];
			clu_shadow_quad[2].x = OV_F(self, 0x3e0) * 200.0f + OV_F(OV_I(self, 0x74), 0xc0);
			clu_shadow_quad[2].y = OV_F(OV_I(self, 0x74), 0xc4);
			clu_shadow_quad[2].z = OV_F(self, 0x3e0) * 100.0f + OV_F(OV_I(self, 0x74), 0xc8);
			clu_shadow_quad[2].colour = colour | DAT_002bc898[60];
			clu_shadow_quad[3].x = OV_F(OV_I(self, 0x74), 0xc0) - OV_F(self, 0x3e0) * 200.0f;
			clu_shadow_quad[3].y = OV_F(OV_I(self, 0x74), 0xc4);
			clu_shadow_quad[3].z = OV_F(self, 0x3e0) * 100.0f + OV_F(OV_I(self, 0x74), 0xc8);
			clu_shadow_quad[3].colour = colour | DAT_002bc898[60];
			CL_SetRenderState3(5, 0, 0);
			CL_SetRenderState3(3, 0, 0);
			CL_SetRenderState3(6, 1, 0);
			CL_SetRenderState3(10, 0, 0);
			FUN_000416c0(0, 0);
			FUN_000438b0();
			FUN_0003f2d0(5, (void *)clu_shadow_quad, 4);
			FUN_00046d00(3);
			FUN_00046d00(6);
			FUN_00046d00(10);
			FUN_00046d00(5);
		}
	}
	OV_U(OV_I(self, 0x74), 0xd4) = OV_U(self, 0x37c);
	FUN_00034bf0((struct BActorInstance *)OV_U(self, 0x74), 1);
	if (clu_overlay_models == -1)
		return;
	COverlay_DrawModelPass(self);
	CL_SetClipPlanes(saved_near, saved_far);
}

/* 0x00074000  ClassLib/COverlay.cpp
Not a function: Ghidra's table entry points into the middle of COverlay_DrawModel
(0x00073eb0, bytes 0x00073ffd-0x00074008 decoded from a wrong offset). Nothing calls
it; it does nothing here. */
void FUN_00074000(undefined4 unused1, undefined4 *unused2)
{
	(void)unused1;
	(void)unused2;
}

/* 0x00074500  ClassLib/COverlay.cpp
Creates a root overlay (in the list 0x002eca3c; models at the tail, others at the
head) from a descriptor. Render-target overlays (0x100) return 0 when the target
cannot be created (the object is not freed, as in the original). */
int COverlay_Create(int *desc)
{
	int self;
	short count;
	int bit;

	self = (int)CLU_Alloc(0x40c, "C:\\BlitzParty\\Game\\Src\\ClassLib\\COverlay.cpp", 0x5c, 0x1001);
	if (self != 0) {
		CLList_Init((int *)self);
		memset((void *)(self + 0x5c), 0, 0xec * 4);
		OV_I(self, 0x10) = 0;
		OV_S(self, 0x54) = 0;
	} else {
		self = 0;
	}
	if (self == 0)
		return 0;
	memset((void *)(self + 0x5c), 0, 0xec * 4);
	if (desc[0xa2] == 0)
		COverlay_InitImage(self, (undefined4 *)desc);
	if (desc[0xa2] == 1)
		COverlay_InitModel(self, (undefined4 *)desc);
	if (desc[0xa2] == 2)
		COverlay_InitText(self, (undefined4 *)desc);
	OV_I(self, 0x300) = desc[0xa9];
	OV_I(self, 0x304) = desc[0xaa];
	OV_I(self, 0x308) = desc[0xab];
	OV_I(self, 0x30c) = desc[0xac];
	OV_I(self, 0x64) = desc[2];
	OV_I(self, 0x68) = desc[3];
	OV_I(self, 0x6c) = desc[4];
	OV_I(self, 0x70) = desc[5];
	count = 0;
	for (bit = 0; bit < 32; bit++)
		if ((OV_U(self, 0x2e8) & (1u << bit)) != 0)
			count = count + 1;
	OV_I(self, 0x18) = count;
	OV_I(self, 0x1c) = 0;
	if (count != 0) {
		OV_I(self, 0x1c) = (int)FUN_0004c4b0(count * 4, 0,
						     (const char *)"C:\\BlitzParty\\Game\\Src\\ClassLib\\COverlay.cpp", 0x8a,
						     0x2001);
		count = 0;
		for (bit = 0; bit < 32; bit++) {
			if ((OV_U(self, 0x2e8) & (1u << bit)) != 0) {
				((uint32_t *)OV_I(self, 0x1c))[count] = clu_overlay_functions[bit];
				count = count + 1;
			}
		}
	}
	OV_I(self, 0x3b0) = desc[0xd5];
	OV_I(self, 0x3b4) = desc[0xd6];
	OV_I(self, 0x3b8) = desc[0xd7];
	OV_I(self, 0x3bc) = desc[0xd8];
	OV_I(self, 0x3e0) = desc[0xe1];
	if ((OV_U(self, 0x2ec) & 0x100) != 0 && (char)COverlay_CreateRenderTarget(self) == '\0')
		return 0;
	OV_I(self, 0x58) = 0;
	OV_S(self, 0x54) = 0;
	CLList_Insert((int *)clu_overlay_root, (int *)(self + 0x10), desc[0xa2] == 1);
	OV_I(self, 0xc) = clu_overlay_root;
	return self;
}

/* ---------- update and deletion (0x000748c0-0x00074be0, inferred COverlay.cpp) */

/* 0x002ecfc0  per-player HUD overlays cleared with the HUD's (Hud.cpp) */
#define clu_hud_player_overlays CLU_ARR(int, 0x002ecfc0)

/* 0x000748c0  ClassLib/COverlay.cpp (inferred; thiscall)
Deletes an overlay and its children; clears the HUD's references to it. Returns the
previous node of its parent list (so a caller's loop can go on). */
int COverlay_Delete(int self)
{
	int previous;
	int i;
	int hud;

	previous = CLList_Remove((int *)OV_I(self, 0xc), (int *)(self + 0x10));
	hud = clu_hud;
	for (i = 0x170; i < 0x180; i += 4) {
		if (hud == 0)
			continue;
		if (self == *(int *)(hud + i + 0x10)) {
			*(int *)(hud + i + 0x10) = 0;
			hud = clu_hud;
		} else if (self == *(int *)(hud + i)) {
			*(int *)(hud + i) = 0;
			clu_hud_player_overlays[(i - 0x170) / 4] = 0;
			hud = clu_hud;
		}
	}
	COverlay_Destroy(self);
	FUN_0004c110((void *)self);
	return previous;
}

/* 0x00074930  ClassLib/COverlay.cpp (inferred; thiscall)
Per-frame update of an overlay and its children: model animation, life time (0x3dc;
deleted at 0), sound delay (0x3e4; plays sample 0x408), width easing (towards 0x36c at
0x370), glow object, update handlers. Returns the node to go on from. */
int COverlay_Update(int self)
{
	int node;
	int i;

	if (OV_I(self, 0x2e4) == 1)
		((void (*)(int, int, float, int))FUN_000380f0)(OV_I(self, 0x74), OV_I(OV_I(self, 0x74), 0x134),
							      clu_time_scale, 0);
	if (OV_F(self, 0x3dc) > 0.0f) {
		OV_F(self, 0x3dc) = OV_F(self, 0x3dc) - clu_time_scale;
		if (OV_F(self, 0x3dc) <= 0.0f)
			return COverlay_Delete(self);
	}
	if (OV_F(self, 0x3e4) >= 0.0f) {
		OV_F(self, 0x3e4) = OV_F(self, 0x3e4) - clu_time_scale;
		if (OV_F(self, 0x3e4) < 0.0f && OV_I(self, 0x408) != 0) {
			CLSamples_Play(OV_I(self, 0x408), 0xff, 0, 0, NULL);
			return self + 0x10;
		}
		return self + 0x10;
	}
	if (OV_F(self, 0x364) != OV_F(self, 0x36c)) {
		double s = (double)clu_time_scale * (double)OV_F(self, 0x370);
		double w;

		if (OV_F(self, 0x36c) <= OV_F(self, 0x364)) {
			w = (double)OV_F(self, 0x364) - s;
			if (w < (double)OV_F(self, 0x36c))
				w = (double)OV_F(self, 0x36c);
		} else {
			w = s + (double)OV_F(self, 0x364);
			if (w > (double)OV_F(self, 0x36c))
				w = (double)OV_F(self, 0x36c);
		}
		OV_F(self, 0x364) = (float)w;
	}
	if (OV_I(self, 0x5c) != 0) {
		FUN_00082be0((struct CAnimTex *)OV_I(self, 0x5c), 1.0f, 1);
		OV_I(self, 0x60) = OV_I(OV_I(self, 0x5c), 8);
	}
	for (i = 0; i < OV_I(self, 0x18); i++)
		((void (*)(int))((uint32_t *)OV_I(self, 0x1c))[i])(self);
	for (node = (int)CLList_Next((unsigned int *)self, NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)self, (unsigned int *)COverlay_Update(node - 0x10)))
		;
	return self + 0x10;
}

/* 0x00074ac0  ClassLib/COverlay.cpp (inferred; thiscall)
Destructor body: deletes the children and frees the overlay's resources. */
void COverlay_Destroy(int self)
{
	int node;
	int glow;

	for (node = (int)CLList_Next((unsigned int *)self, NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)self, (unsigned int *)COverlay_Delete(node - 0x10)))
		;
	if (OV_I(self, 0x74) != 0 && *(char *)(self + 800) != '\0')
		FUN_00034930((struct BActorInstance *)OV_I(self, 0x74));
	if (OV_I(self, 0xa0) != 0)
		OV_I(self, 0xa0) = 0;
	if (OV_I(self, 0x1c) != 0) {
		FUN_0004c110((void *)OV_I(self, 0x1c));
		OV_I(self, 0x1c) = 0;
	}
	if (OV_I(self, 0x3d0) != 0) {
		FUN_000458a0((struct bdRenderTarget *)OV_I(self, 0x3d0));
		OV_I(self, 0x3d0) = 0;
	}
	glow = OV_I(self, 0x5c);
	if (glow != 0) {
		FUN_001d5950(); /* the glow object's destructor (empty) */
		FUN_0004c110((void *)glow);
		OV_I(self, 0x5c) = 0;
	}
	FUN_001d5950(); /* the list member's destructor (empty) */
}

/* 0x00074ba0  ClassLib/COverlay.cpp (inferred)
Deletes every root overlay that is not kept (0x58 == 0). */
void COverlay_DeleteAll(void)
{
	int node;

	if (clu_overlay_root == 0)
		return;
	for (node = (int)CLList_Next((unsigned int *)clu_overlay_root, NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)clu_overlay_root, (unsigned int *)node)) {
		if (*(int *)(node + 0x48) == 0)
			node = COverlay_Delete(node - 0x10);
	}
}

/* 0x00074be0  ClassLib/COverlay.cpp (inferred)
Updates every root overlay (and so the whole tree). */
void COverlay_UpdateAll(void)
{
	int node;

	if (clu_overlay_root == 0)
		return;
	for (node = (int)CLList_Next((unsigned int *)clu_overlay_root, NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)clu_overlay_root, (unsigned int *)COverlay_Update(node - 0x10)))
		;
}

/* 0x00074c10  ClassLib/COverlay.cpp (inferred); missing from of_functions.tsv
Draw function of the pause menu's volume bars: ten segments of growing height; the
segment 0x3d4 is drawn with 0.4 of the alpha. */
void FUN_00074c10(int self, int shadow)
{
	int r, g, b, a;
	unsigned int colour;
	double x;
	float base, seg_width, inc, height, next_x;
	int i;

	(void)shadow;
	CL_SetRenderState3(3, 0, 0);
	FUN_000438b0();
	FUN_00043b70(0);
	FUN_00044b00(1, 2, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 0, 0);
	FUN_00044b00(5, 0, 0);
	r = (int)OV_F(self, 0x2ac);
	g = (int)OV_F(self, 0x2b0);
	b = (int)OV_F(self, 0x2b4);
	a = (int)OV_F(self, 0x2b8);
	colour = DAT_002bc898[a] | DAT_002bd640[b] | DAT_002bce18[r] | DAT_002bc490[g];
	clu_bar_quad[0].colour = colour;
	clu_bar_quad[1].colour = colour;
	clu_bar_quad[2].colour = colour;
	clu_bar_quad[3].colour = colour;
	x = (double)OV_F(self, 0x300);
	base = (float)((double)OV_F(self, 0x304) - (double)OV_F(self, 0x368));
	seg_width = (float)((double)OV_F(self, 0x364) * (double)0.1f);
	inc = (float)((double)OV_F(self, 0x368) * (double)0.1f);
	height = inc;
	for (i = 0; i < 10; i++) {
		if (i == OV_I(self, 0x3d4)) {
			a = (int)((double)a * (double)0.4f);
			colour = DAT_002bc898[a] | DAT_002bd640[b] | DAT_002bce18[r] | DAT_002bc490[g];
			clu_bar_quad[0].colour = colour;
			clu_bar_quad[1].colour = colour;
			clu_bar_quad[2].colour = colour;
			clu_bar_quad[3].colour = colour;
		}
		clu_bar_quad[0].x = (float)x;
		clu_bar_quad[0].z = 0.0f;
		clu_bar_quad[0].y = (float)((double)height + (double)base);
		clu_bar_quad[1].z = 0.0f;
		clu_bar_quad[2].y = base;
		clu_bar_quad[2].z = 0.0f;
		clu_bar_quad[3].y = base;
		clu_bar_quad[3].z = 0.0f;
		next_x = (float)((double)seg_width + x);
		clu_bar_quad[1].x = (float)((double)seg_width + x - (double)1.0f);
		clu_bar_quad[1].y = (float)((double)height + (double)base);
		clu_bar_quad[2].x = (float)((double)seg_width + x - (double)1.0f);
		clu_bar_quad[3].x = (float)x;
		FUN_000416c0(0, 0);
		FUN_0003f2d0(5, (void *)clu_bar_quad, 4);
		x = (double)next_x;
		height = height + inc;
	}
	FUN_00046d00(3);
}
