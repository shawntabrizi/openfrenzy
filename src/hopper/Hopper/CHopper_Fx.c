/*
Hopper/CHopper_Fx.c  <-  C:\BlitzParty\Game\Src\Hopper\CHopper_Fx.cpp
(module hopper)

The effects object (0x002affa0, vtable 0x0026eeb8): bactor "extra" systems for bubbles,
tokens, steam, electricity, the spinning poles, steam jets, a flash, foam, the respawn
effect, splashes and sparks, plus the core of variation 3. Only 0x00148ff0 carries the file
name (a heap tag); the gap functions 0x00147420-0x0014b090 around it work on the same
object and are placed here (file inferred). Many of them are extra system callbacks reached
only through baCreateExtraSystem; they are missing from the function table and keep their
FUN_<addr> names.

The object (offsets from 0x002affa0):
  +0x10 the bubble texture   +0x14 "bubble"      +0x18 gold coin model  +0x1c silver
  +0x20 bronze  +0x24..+0x2c their instances      +0x30 "token"  (0x002affd0)
  +0x34 the steam texture     +0x38 "steam"       +0x40..+0x48 three electric textures
  +0x4c "electric"  +0x50 "pole"  +0x54 "steamjet"  +0x58 "flash"  +0x5c "foam"
  +0x60 "respawn"   +0x64 "splash"  +0x68 "hsparks" (0x002b0008)
  +0x6c variation 3: the core's frame; +0x70..+0x78 its texture scrolls
  +0x7c the core beam's brightness (0x002b001c), +0x80 the electricity's (0x002b0020)
  +0x84 the core's cycle frame (0x002b0024), +0x88 its phase (0x002b0028: 1 charging,
  2 firing, 3 cooling)   +0x8c variation 4: foam items made
  +0x164 (0x002b0104) int[2] variation 2: the token value on each ring
Item layouts are given at each creation callback.
*/

#include "hopper.h"

#define FX_SYS(self, off) HOP_AT(self, off, struct BExtraSystem *)
#define hop_Fx ((void *)HOP_ADDR(0x002affa0))
#define hop_SteamTexture HOP_G(0x002affd4, struct bdTexture *)
#define hop_ElectricTextures ((struct bdTexture **)HOP_ADDR(0x002affe0))
#define hop_Water HOP_P(0x002aec20)

extern unsigned char DAT_002ed324[];   /* classlib_ui: the rumble object */

/* va_list access of an extra item creation callback */
#define HOP_ARGS(args) va_list ap = (va_list)(args)

/* ======================================================================== bubbles */

/* 0x00147420  (gap; not in the function table): the bubble system's callback: makes (0) or
frees (1) its vertex buffer (user data +0: 4 vertices per bubble, XYZRHW|DIFFUSE|TEX1) */
int FUN_00147420(struct BExtraSystem *system, int deleted)
{
	struct bdVertexBuffer **vb = (struct bdVertexBuffer **)HOP_AP(system, 0x40);

	if (deleted == 0) {
		bdVertexBufferCreate(vb, HOP_AI(system, 0x10) << 2, 0x144, 0, 1);
		bkPrintf("create bubble vb=%x\n", (unsigned int)(uintptr_t)*vb);
		return 0;
	}
	if (deleted == 1) {
		bkPrintf("destroy bubble vb=%x\n", (unsigned int)(uintptr_t)*vb);
		if (*vb == NULL) {
			bkPrintf("***can't be arsed to free anything (bubble) today");
			return 0;
		}
		bdVertexBufferDestroy(*vb);
	}
	return 0;
}

/* 0x001474a0  (gap; not in the function table): bubble draw: screen sprites into the vertex
buffer, turned with the arm (variation 3: small and green) */
int FUN_001474a0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	struct bdVertexBuffer **vb = (struct bdVertexBuffer **)HOP_AP(system, 0x40);
	float m[16];
	float *item;
	float size;
	int r, g, b;
	int i, n;

	(void)arg;
	bmMatrixRotationY(m, HOP_F(0x002af158) * HOP_DEG);
	bdSetWorldMatrix(m);
	HOP_P(0x00302b5c) = bdVertexBufferLock(*vb, 4);
	HOP_P(0x00302b60) = HOP_P(0x00302b5c);
	if (HOP_P(0x00302b5c) == NULL) {
		bkPrintf("vert buffer failure\n");
		return 0;
	}
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetProjectionMode(1);
	for (i = 0; i < count; i++) {
		item = (float *)items[i];
		item[1] = item[1] - hop_Sink;
		if (hop_Variation == 3) {
			b = 100;
			g = 200;
			r = 0x7f;
			size = 25.0f;
		} else {
			b = 0x40;
			g = 0x7f;
			r = 0x40;
			size = 40.0f;
		}
		if (bdDrawSprite3D(item, size, size, 0.0f, HOP_G(0x002affb0, struct bdTexture *), 0, 0, r, g, b, 0x20,
			(float *)HOP_P(0x00302b60), 0.0f) != 0)
			HOP_P(0x00302b60) = (char *)HOP_P(0x00302b60) + 0x70;
		item[1] = hop_Sink + item[1];
	}
	bdVertexBufferUnlock(*vb);
	bdSetTexture(0, HOP_G(0x002affb0, struct bdTexture *));
	n = (int)(((char *)HOP_P(0x00302b60) - (char *)HOP_P(0x00302b5c)) / 0x1c) >> 2;
	for (i = 0; i < n; i++)
		bdDrawVertexBuffer(4, *vb, i * 4, 4);
	bdSetProjectionMode(1);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
	return 0;
}

/* ======================================================================== steam */

/* 0x00147660  (gap; not in the function table): steam draw: grey sprites fading out as they
rise (8 - y / 300, 0..4; variation 2 twice as bright), turning both ways */
int FUN_00147660(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	float v, a;
	int c;
	unsigned int i;

	(void)system;
	(void)arg;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(8, 0, 0);
	for (i = 0; (int)i < count; i++) {
		item = (float *)items[i];
		v = 8.0f - item[1] * 0.0033333334f;
		if (v < 0.0f)
			v = 0.0f;
		else if (4.0f < v)
			v = 4.0f;
		c = (unsigned char)HOP_FTOL(v);
		if (hop_Variation == 2)
			c = (unsigned char)(c << 1);
		a = (float)HOP_AI(item, 0x10) * HOP_DEG;
		if ((i & 1) == 0)
			a = -a;
		bdDrawSprite3D(item, 128.0f, 128.0f, a, hop_SteamTexture, 0, 0, c, c, c, 0xff, NULL, 0.0f);
	}
	bdSetRenderState(8, 1, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
	return 0;
}

/* ======================================================================== electricity */

/* 0x00147790  (gap; not in the function table): electric arc creation. args: kind (int: 1 a
line, 2 a circle around the core), x1, y1, z1, x2, y2, z2 (doubles). Item (0x38 bytes):
[0..2] start, [4..6] end, [8] kind, [9..11] texture scrolls, [12] circle angle, [13] the
texture length (0.25 / 0.5). */
int FUN_00147790(struct BExtraSystem *system, float *item, void *args)
{
	(void)system;
	if (args == NULL)
		return 0;
	{
		HOP_ARGS(args);
		HOP_AI(item, 0x20) = va_arg(ap, int);
		item[0] = (float)va_arg(ap, double);
		item[1] = (float)va_arg(ap, double);
		item[2] = (float)va_arg(ap, double);
		item[3] = 1.0f;
		item[4] = (float)va_arg(ap, double);
		item[5] = (float)va_arg(ap, double);
		item[6] = (float)va_arg(ap, double);
	}
	item[7] = 1.0f;
	item[9] = 0.0f;
	item[10] = 0.2f;
	item[11] = 0.4f;
	item[12] = 0.0f;
	if (HOP_AI(item, 0x20) == 1)
		item[13] = 0.25f;
	else
		item[13] = 0.5f;
	return 0;
}

/* 0x00147820  (gap; not in the function table): electric update: the three texture scrolls
run (wrapping by 1 - length); circles turn 4 degrees a frame (in 32 steps) */
int FUN_00147820(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	float s0, s1, s2, f, a;
	int i, k;

	(void)system;
	(void)arg;
	for (i = 0; i < count; i++) {
		item = (float *)items[i];
		s0 = item[9] - 0.01f;
		item[9] = s0;
		s1 = item[10] + 0.005f;
		item[10] = s1;
		s2 = item[11] - 0.0075f;
		item[11] = s2;
		if (s0 <= 0.0f)
			item[9] = (1.0f - item[13]) + s0;
		f = 1.0f - item[13];
		if (f <= s1)
			item[10] = s1 - f;
		if (s2 <= 0.0f)
			item[11] = f + s2;
		if (HOP_AI(item, 0x20) != 1) {
			a = item[12];
			for (k = 0; k < 32; k++) {
				a = a - 0.125f;
				if (a < 0.0f)
					a = a + 360.0f;
			}
			item[12] = a;
		}
	}
	return 0;
}

/* ======================================================================== steam jets */

/* 0x001479c0  (gap; not in the function table): steam jet update: 31 frames, pushed out 20 a
frame (sideways by the sign of x), turning */
int FUN_001479c0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	int i = 0, a;

	(void)arg;
	while (i < count) {
		item = (float *)items[i];
		if (HOP_AI(item, 0x14) < 0x1f) {
			a = HOP_AI(item, 0x10) + 1;
			HOP_AI(item, 0x10) = a;
			if (a == 0x168)
				HOP_AI(item, 0x10) = 0;
			if (item[0] <= 0.0f)
				item[0] = item[0] - 20.0f;
			else
				item[0] = item[0] + 20.0f;
			HOP_AI(item, 0x14) = HOP_AI(item, 0x14) + 1;
			i++;
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x00147a40  (gap; not in the function table): steam jet draw: sprites growing and fading
with age, alternately pale blue and grey, turning both ways */
int FUN_00147a40(struct BExtraSystem *system, int count, void **items, void *arg)
{
	void *item;
	float size, a;
	int r, g, b;
	unsigned int i;

	(void)system;
	(void)arg;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 0, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(8, 0, 0);
	for (i = 0; (int)i < count; i++) {
		item = items[i];
		if ((i & 1) == 0) {
			b = 0xa0;
			g = 0x80;
			r = 0x80;
			a = -((float)HOP_AI(item, 0x10) * HOP_DEG);
		} else {
			b = 0x8c;
			g = 100;
			r = 100;
			a = (float)HOP_AI(item, 0x10) * HOP_DEG;
		}
		size = (float)(unsigned char)((unsigned char)(HOP_AB(item, 0x14) + 0x20) << 1);
		bdDrawSprite3D((float *)item, size, size, a, hop_SteamTexture, 0, 0, r, g, b,
			(unsigned char)(0x20 - HOP_AB(item, 0x14)), NULL, 0.0f);
	}
	bdSetRenderState(8, 1, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
	return 0;
}

/* ======================================================================== foam */

/* 0x00147b60  (gap; not in the function table): foam draw (variation 4): sprites on the
water, turned with the camera, brighter with the arm's speed (at most 24) */
int FUN_00147b60(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float m[16];
	float *item;
	unsigned int c;
	int i;

	(void)system;
	(void)arg;
	bmMatrixRotationY(m, hop_CamAngle * HOP_DEG);
	bdSetWorldMatrix(m);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetProjectionMode(1);
	for (i = 0; i < count; i++) {
		item = (float *)items[i];
		item[1] = item[1] - hop_Sink;
		c = (unsigned char)HOP_FTOL(HOP_F(0x002aed74) * 32.0f);
		if (0x18 < c)
			c = 0x18;
		bdDrawSprite3D(item, 32.0f, 32.0f, (float)HOP_AI(item, 0x10) * HOP_DEG, hop_SteamTexture, 0, 0, (int)c,
			(int)c, (int)c, 0xff, NULL, 0.0f);
		item[1] = hop_Sink + item[1];
	}
	bdSetProjectionMode(1);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
	return 0;
}

/* ======================================================================== the flash */

/* 0x00147c80  (gap; not in the function table): flash creation. args: the fade per frame
(double). Item: [0] brightness (255), [1] fade. */
int FUN_00147c80(struct BExtraSystem *system, float *item, void *args)
{
	HOP_ARGS(args);

	(void)system;
	item[1] = (float)va_arg(ap, double);
	item[0] = 255.0f;
	return 0;
}

/* 0x00147ca0  (gap; not in the function table): flash update: fades; deleted at 0 */
int FUN_00147ca0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	int i = 0;

	(void)arg;
	while (i < count) {
		item = (float *)items[i];
		if (0.0f < item[0]) {
			i++;
			item[0] = item[0] - item[1];
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x00147cf0  (gap; not in the function table): flash draw: a white screen quad (camera
space, 320 x 240) at the flash's brightness */
int FUN_00147cf0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *quad = (float *)HOP_ADDR(0x00302b68);
	unsigned int c, argb;
	int i;

	(void)system;
	(void)arg;
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(7, 1, 0);
	bdSetProjectionMode(0);
	for (i = 0; i < count; i++) {
		if (0.0f < *(float *)items[i])
			c = (unsigned int)HOP_FTOL(*(float *)items[i]);
		else
			c = 0;
		c &= 0xff;
		argb = HOP_ARGB(255, c, c, c);
		HOP_U(0x00302b80) = argb;
		quad[0] = -320.0f;
		quad[1] = -240.0f;
		quad[2] = 0.0f;
		quad[9] = 320.0f;
		quad[10] = -240.0f;
		quad[11] = 0.0f;
		quad[0x12] = -320.0f;
		quad[0x13] = 240.0f;
		quad[0x14] = 0.0f;
		quad[0x1b] = 320.0f;
		quad[0x1c] = 240.0f;
		quad[0x1d] = 0.0f;
		HOP_U(0x00302ba4) = argb;
		HOP_U(0x00302bc8) = argb;
		HOP_U(0x00302bec) = argb;
		bdDrawPrimitiveFVF(4, quad, 4, 0x152);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(7, 2, 0);
	bdSetProjectionMode(1);
	return 0;
}

/* ======================================================================== sparks */

/* 0x00147e70  (gap; not in the function table): spark trail update (16 frames): the head
moves (slowing by 1/12, falling 1 a frame), the 7 trail points follow and fade */
int FUN_00147e70(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	int *colour;
	int i = 0, k, v;

	(void)arg;
	while (i < count) {
		item = (float *)items[i];
		if (item[0x24] <= 16.0f) {
			item[0] = item[0] + item[0x20];
			item[1] = item[1] + item[0x21];
			item[2] = item[0x22] + item[2];
			item[0x20] = item[0x20] - item[0x20] * 0.083333336f;
			item[0x21] = item[0x21] - 1.0f;
			item[0x22] = item[0x22] - item[0x22] * 0.083333336f;
			for (k = 7; k > 0; k--) {
				item[k * 4 + 0] = item[(k - 1) * 4 + 0];
				item[k * 4 + 1] = item[(k - 1) * 4 + 1];
				item[k * 4 + 2] = item[(k - 1) * 4 + 2];
				colour = (int *)item + 0x25 + k * 4;
				v = colour[0] - 8;
				colour[0] = v < 0 ? 0 : v;
				v = colour[1] - 8;
				colour[1] = v < 0 ? 0 : v;
				v = colour[2] - 6;
				colour[2] = v < 0 ? 0 : v;
			}
			i++;
			item[0x24] = item[0x24] + 1.0f;
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x00147fb0  (gap; not in the function table): spark trail draw: a polyline of the 8 points,
8 to 1 wide */
int FUN_00147fb0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float widths[8] = { 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
	float screen[8];
	void *item;
	int i, ok;

	(void)system;
	(void)arg;
	/* the original passes an unset stack buffer as the screen widths */
	memset(screen, 0, sizeof screen);
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	for (i = 0; i < count; i++) {
		item = items[i];
		bdSetWorldIdentity();
		bdSetProjectionMode(0);
		bdSetProjectionMode(1);
		ok = bdDrawPolyline3D(8, (float *)item, widths, screen, (int *)((char *)item + 0x94),
			(float *)HOP_ADDR(0x00302bf8), 0.0f);
		bdSetTexture(0, HOP_G(0x002aeb80, struct bdTexture *));
		if (ok != 0)
			bdDrawPrimitiveFVF(4, HOP_ADDR(0x00302bf8), 0x14, 0x144);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetProjectionMode(1);
	return 0;
}

/* ======================================================================== the core (variation 3) */

/* one vertex of the core beam strip (0x00302e28, 0x24 bytes each) */
static void hop_BeamVertex(int n, float x, float y, float z, unsigned int argb, float u, float v)
{
	float *vtx = (float *)HOP_ADDR(0x00302e28) + n * 9;

	vtx[0] = x;
	vtx[1] = y;
	vtx[2] = z;
	HOP_AU(vtx, 0x18) = argb;
	vtx[7] = u;
	vtx[8] = v;
}

/* 0x001480c0  (gap): variation 3: the core's 8 beams (each with the three electric textures,
scrolled; brightness +0x7c fading along the beam), turning both ways */
void HopFx_DrawCore(void *self)
{
	float m[16];
	float a, s;
	unsigned int c1, c2, c3, c4;
	int k, t, c;

	if (DAT_002ecdec == 0) {
		a = HOP_AF(self, 0x70) - 0.0033333332f;
		HOP_AF(self, 0x70) = a;
		if (a < 0.0f)
			HOP_AF(self, 0x70) = a + 0.5f;
		a = HOP_AF(self, 0x74) - 0.0066666664f;
		HOP_AF(self, 0x74) = a;
		if (a < 0.0f)
			HOP_AF(self, 0x74) = a + 0.5f;
		a = HOP_AF(self, 0x78) + 0.0066666664f;
		HOP_AF(self, 0x78) = a;
		if (0.0f < a)
			HOP_AF(self, 0x78) = a - 0.5f;
		HOP_AI(self, 0x6c)++;
	}
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(8, 0, 0);
	for (k = 0; k < 8; k++) {
		if (((unsigned int)k & 1) == 0)
			a = (float)k * 45.0f - (float)(HOP_AI(self, 0x6c) << 1);
		else
			a = (float)HOP_AI(self, 0x6c) + (float)k * 45.0f;
		bmMatrixRotationY(m, a * HOP_DEG);
		for (t = 0; t < 3; t++) {
			bdSetWorldMatrix(m);
			bdSetTexture(0, hop_ElectricTextures[t]);
			s = HOP_AF(self, 0x70 + t * 4);
			c = HOP_AI(self, 0x7c);
			c1 = HOP_ARGB(255, c, c, c);
			c2 = HOP_ARGB(255, (c / 4) * 3, (c / 4) * 3, (c / 4) * 3);
			c3 = HOP_ARGB(255, c / 2, c / 2, c / 2);
			c4 = HOP_ARGB(255, 0, 0, 0);
			hop_BeamVertex(0, -80.0f, 705.0f, -130.0f, c1, s, 0.0f);
			hop_BeamVertex(1, 80.0f, 705.0f, -130.0f, c1, s, 1.0f);
			hop_BeamVertex(2, -100.0f, 405.0f, -130.0f, c1, s + 0.125f, 0.0f);
			hop_BeamVertex(3, 100.0f, 405.0f, -130.0f, c1, s + 0.125f, 1.0f);
			hop_BeamVertex(4, -120.0f, 319.5f, -150.0f, c2, s + 0.25f, 0.0f);
			hop_BeamVertex(5, 120.0f, 319.5f, -150.0f, c2, s + 0.25f, 1.0f);
			hop_BeamVertex(6, -160.0f, 255.0f, -290.0f, c3, s + 0.375f, 0.0f);
			hop_BeamVertex(7, 160.0f, 255.0f, -290.0f, c3, s + 0.375f, 1.0f);
			hop_BeamVertex(8, -200.0f, 255.0f, -480.0f, c4, s + 0.5f, 0.0f);
			hop_BeamVertex(9, 200.0f, 255.0f, -480.0f, c4, s + 0.5f, 1.0f);
			bdDrawPrimitiveFVF(4, HOP_ADDR(0x00302e28), 10, 0x152);
		}
	}
	bdSetRenderState(8, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 1, 0);
}

/* ======================================================================== the respawn effect */

/* 0x001485d0  (gap; not in the function table): respawn creation (variation 2). args: x, y,
z (doubles), the effect instance index (int). Item: [0..2] position, [4] frame, [5] index.
Finds the instance's "spin" node (0x002aeb70[index]). */
int FUN_001485d0(struct BExtraSystem *system, float *item, void *args)
{
	HOP_ARGS(args);
	int idx;

	(void)system;
	item[0] = (float)va_arg(ap, double);
	item[1] = (float)va_arg(ap, double);
	item[2] = (float)va_arg(ap, double);
	idx = va_arg(ap, int);
	HOP_AI(item, 0x14) = idx;
	item[3] = 1.0f;
	HOP_PV(0x002aeb70)[HOP_AI(item, 0x14)] = baFindSubObject((struct BActorInstance *)HOP_PV(0x002aeb60)[idx], "spin");
	if (HOP_PV(0x002aeb70)[HOP_AI(item, 0x14)] == NULL)
		bkPrintf("failed\n");
	item[4] = 1.0f;
	return 0;
}

/* 0x00148650  (gap; not in the function table): respawn update: 74 frames of the effect's
animation at the position */
int FUN_00148650(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float m[16];
	float *item;
	struct BActorInstance *inst;
	int i = 0;

	(void)arg;
	while (i < count) {
		item = (float *)items[i];
		if (item[4] <= 74.0f) {
			inst = (struct BActorInstance *)HOP_PV(0x002aeb60)[HOP_AI(item, 0x14)];
			HOP_AF(inst, 0xd0) = 1.0f;
			HOP_AF(inst, 0xd4) = 1.0f;
			HOP_AF(inst, 0xd8) = 1.0f;
			bmMatrixRotationY(m, 0.0f);
			bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
			HOP_AF(inst, 0xc0) = item[0];
			HOP_AF(inst, 0xc4) = item[1];
			HOP_AF(inst, 0xc8) = item[2];
			baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), HOP_FTOL(item[4] * 3072.0f), 1);
			baPrepActorInstance(inst, 1);
			i++;
			item[4] = item[4] + 1.0f;
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x00148790  (gap; not in the function table): respawn draw */
int FUN_00148790(struct BExtraSystem *system, int count, void **items, void *arg)
{
	int i;

	(void)system;
	(void)arg;
	bdSetWorldIdentity();
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	for (i = 0; i < count; i++)
		baDrawActorInstance((struct BActorInstance *)HOP_PV(0x002aeb60)[HOP_AI(items[i], 0x14)], 7);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
	return 0;
}

/* ======================================================================== splashes */

/* 0x00148820  (gap; not in the function table): splash update: 16 frames, spreading 1.3
times a frame and rising by 16 - age */
int FUN_00148820(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	int i = 0;

	(void)arg;
	while (i < count) {
		item = (float *)items[i];
		if (item[8] <= 16.0f) {
			i++;
			item[4] = item[4] * 1.3f;
			item[5] = (16.0f - item[8]) + item[5];
			item[6] = item[6] * 1.3f;
			item[8] = item[8] + 1.0f;
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* 0x001488a0  (gap; not in the function table): splash draw: a steam sprite at the position
plus the spread, darker with age */
int FUN_001488a0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	int c, i;

	(void)system;
	(void)arg;
	bdSetWorldIdentity();
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(10, 1, 0);
	for (i = 0; i < count; i++) {
		item = (float *)items[i];
		c = 0x10 - HOP_FTOL(item[8]);
		if (c < 0)
			c = 0;
		c = c * 2;
		item[0] = item[0] + item[4];
		item[1] = item[1] + item[5];
		item[2] = item[2] + item[6];
		bdDrawSprite3D(item, 16.0f, 16.0f, item[9], hop_SteamTexture, 0, 0, c, c, c, 0xff, NULL, 0.0f);
		item[0] = item[0] - item[4];
		item[1] = item[1] - item[5];
		item[2] = item[2] - item[6];
	}
	bdPopRenderState(6);
	bdPopRenderState(5);
	bdPopRenderState(10);
	return 0;
}

/* ======================================================================== the effects object */

/* 0x00148980  (gap): variation 1: 10 tokens */
void HopFx_Start1(void *self)
{
	int i;

	for (i = 10; i != 0; i--)
		baAddExtraItem(FX_SYS(self, 0x30));
}

/* 0x001489b0  (gap): variation 2: 6 tokens */
void HopFx_Start2(void *self)
{
	int i;

	for (i = 6; i != 0; i--)
		baAddExtraItem(FX_SYS(self, 0x30));
}

/* 0x001489e0  (gap): variation 3: the core's state and its five electric arcs */
void HopFx_Start3(void *self)
{
	HOP_AF(self, 0x70) = 0.0f;
	HOP_AF(self, 0x74) = 0.0f;
	HOP_AF(self, 0x78) = 0.0f;
	HOP_AI(self, 0x6c) = 0;
	HOP_AI(self, 0x7c) = 0;
	HOP_AI(self, 0x80) = 0x20;
	HOP_AI(self, 0x88) = 0;
	baAddExtraItem(FX_SYS(self, 0x4c), 1, (double)752.1f, (double)931.8f, (double)-307.6f, (double)243.6f,
		(double)756.2f, (double)-160.0f);
	baAddExtraItem(FX_SYS(self, 0x4c), 1, (double)457.6f, (double)1329.7f, (double)-328.7f, (double)225.4f,
		(double)915.5f, (double)-151.2f);
	baAddExtraItem(FX_SYS(self, 0x4c), 1, (double)-212.7f, (double)919.7f, (double)-151.2f, (double)-460.7f,
		(double)1329.7f, (double)-279.2f);
	baAddExtraItem(FX_SYS(self, 0x4c), 1, (double)-703.0f, (double)966.0f, (double)-334.0f, (double)-214.0f,
		(double)769.4f, (double)-160.9f);
	baAddExtraItem(FX_SYS(self, 0x4c), 2, (double)7.729f, (double)1073.2f, (double)-219.5f, (double)7.729f,
		(double)1573.2f, (double)-219.5f);
}

/* 0x00148b50  (gap): variation 4: 32 foam items */
void HopFx_Start4(void *self)
{
	int i;

	(void)self;
	HOP_I(0x002b002c) = 0;
	for (i = 0x20; i != 0; i--)
		baAddExtraItem(HOP_G(0x002afffc, struct BExtraSystem *));
}

/* 0x00148b80  (gap): variation 2: every 8 frames steam rises at one of four places */
void HopFx_Steam2(void *self)
{
	HOP_I(0x00302f90) = (HOP_I(0x00302f90) + 1) & 7;
	if (HOP_I(0x00302f90) == 0)
		baAddExtraItem(FX_SYS(self, 0x38), -1331.0, -2375.0, 1330.0);
	if (HOP_I(0x00302f90) == 1)
		baAddExtraItem(FX_SYS(self, 0x38), 1353.0, -2375.0, 1353.0);
	if (HOP_I(0x00302f90) == 2)
		baAddExtraItem(FX_SYS(self, 0x38), 1331.0, -2375.0, -1330.0);
	if (HOP_I(0x00302f90) == 3)
		baAddExtraItem(FX_SYS(self, 0x38), -1331.0, -2375.0, -1375.0);
}

/* 0x00148c40  (gap): draws every extra system (and variation 3's core) */
void HopFx_Draw(void *self)
{
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
	baDrawExtras(NULL, NULL);
	if (hop_Variation == 3)
		HopFx_DrawCore(self);
}

/* 0x00148c80  (gap): deletes every extra system */
void HopFx_Shutdown(void *self)
{
	(void)self;
	baDeleteExtraSystem(NULL);
}

/* 0x00148c90  (gap): rumbles a player's pad: 0 a long soft buzz, 1 a hit (motor 1), 2 a
short knock, 3 a long knock, 4 a short hit (motor 1) */
void HopFx_Rumble(void *self, int player, int kind)
{
	(void)self;
	switch (kind) {
	case 0:
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 0, 0x5ffd, 1000, 0x14, 0x32, 0, 0);
		return;
	case 1:
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 1, 0x7fff, 1000, 10, 0x3c, 0, 4);
		return;
	case 2:
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 0, 0x7fff, 500, 5, 0x14, 0, 0);
		return;
	case 3:
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 0, 0x7fff, 500, -1, 0x3c, 0, 0);
		return;
	case 4:
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, player, 1, 0x7fff, 500, 10, 10, 0, 4);
		return;
	}
}

/* 0x00148d60  (gap): scalar deleting destructor */
void *HopFx_Destroy(void *self, byte flags)
{
	*(const void *const **)self = vtable_0026eeb8;
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026eeb8  the effects object's vtable */
const void *const vtable_0026eeb8[1] = {
	(const void *)FUN_00148d60, /* [0] scalar deleting destructor */
};

/* 0x00148d80  (gap; not in the function table): bubble creation. args: x, y, z (doubles),
rising flag (int, low byte). Item (0x30 bytes): [0..2] position (jittered by +-96),
[4..6] the jitter, [8] phase (int), [9] age (int), [10] flag (int), [11] rise speed (-20). */
int FUN_00148d80(struct BExtraSystem *system, float *item, void *args)
{
	(void)system;
	if (args == NULL)
		return 0;
	{
		HOP_ARGS(args);
		item[0] = (float)va_arg(ap, double);
		item[1] = (float)va_arg(ap, double);
		item[3] = 1.0f;
		item[2] = (float)va_arg(ap, double);
		HOP_AI(item, 0x28) = va_arg(ap, int) & 0xff;
	}
	item[4] = (float)((hop_rand() & 0x3f) - 0x20) * 3.0f;
	item[5] = (float)((hop_rand() & 0x3f) - 0x20) * 3.0f;
	item[6] = (float)((hop_rand() & 0x3f) - 0x20) * 3.0f;
	item[0] = item[4] + item[0];
	item[1] = item[5] + item[1];
	item[2] = item[6] + item[2];
	HOP_AI(item, 0x20) = hop_rand() & 0xff;
	HOP_AI(item, 0x24) = 0;
	item[0xb] = -20.0f;
	return 0;
}

/* 0x00148eb0  (gap; not in the function table): bubble update: wobbling, rising 10 a frame
(rising bubbles accelerate from -20 by 0.5 a frame and drift back by their jitter at
first) up to y 0 (variation 3: 1040), where they burst (a splash in the water, not in
variation 3) */
int FUN_00148eb0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	float limit = 1040.0f;
	float a, x, z;
	int i = 0, p;
	void *water;

	(void)arg;
	if (hop_Variation != 3)
		limit = 0.0f;
	while (i < count) {
		item = (float *)items[i];
		if (item[1] <= limit) {
			p = HOP_AI(item, 0x20) + 1;
			HOP_AI(item, 0x20) = p;
			if (p == 0x168)
				HOP_AI(item, 0x20) = 0;
			a = (float)HOP_AI(item, 0x20) * HOP_DEG;
			x = sinf(a) * 2.0f + item[0];
			if (HOP_AI(item, 0x28) == 0) {
				item[0] = x;
				item[1] = item[1] + 10.0f;
				item[2] = cosf(a) * 2.0f + item[2];
			} else {
				item[0] = x;
				item[1] = item[0xb] + item[1];
				z = cosf(a) * 2.0f + item[2];
				item[2] = z;
				if (item[0xb] < 5.0f) {
					item[0] = item[4] * 0.125f + x;
					item[2] = item[6] * 0.125f + z;
				}
				if (item[0xb] < 10.0f)
					item[0xb] = item[0xb] + 0.5f;
			}
			HOP_AI(item, 0x24) = HOP_AI(item, 0x24) + 1;
			i++;
		} else {
			if (hop_Variation != 3) {
				water = hop_Water;
				HOP_VFN(water, 0x28, int (*)(void *, float, float, float, float, float))(water, item[0], item[2], -5.0f,
					20.0f, 50.0f);
			}
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* ======================================================================== tokens */

/* 0x00148ff0  Hopper/CHopper_Fx.cpp: token creation (no args). Item (0x34 bytes): [0..2]
position, [4] 12, [5] the ring radius, [6] spin (int), [7] age (int), [8] value (int: 5
gold, 3 silver, 1 bronze), [9] 1, [10] kind (int: 0 gold, 1 silver, 2 bronze), [11]
variation 2: the ring (int), [12] the CToken. The radius: variations 1, 3, 4: 295..806;
variation 2: inner ring 1036..1163 or outer 636..763. Up to 5 tries for a place 150 away
from the other tokens and the players. */
int HopFx_TokenInit(struct BExtraSystem *system, float *item, void *args)
{
	unsigned int r;
	float x, z, a, dx, dz;
	float *other;
	void *mem;
	int tries, overlap, k, n;

	(void)args;
	switch (hop_Variation) {
	case 1:
		item[5] = (float)(hop_rand() & 0x1ff) + 295.0f;
		r = (unsigned int)hop_rand() % 3;
		if (r == 0)
			goto gold;
		if (r == 1) {
			HOP_AI(item, 0x20) = 3;
			HOP_AI(item, 0x28) = 1;
		} else {
			HOP_AI(item, 0x20) = 1;
			HOP_AI(item, 0x28) = 2;
		}
		break;
	case 2:
		HOP_AI(item, 0x2c) = hop_rand() % 2;
		if (HOP_AI(item, 0x2c) == 0)
			item[5] = (float)(int)((Core_Rand() & 0x7f) - 0x40) + 1100.0f;
		else if (HOP_AI(item, 0x2c) == 1)
			item[5] = (float)(int)((Core_Rand() & 0x7f) - 0x40) + 700.0f;
		else if (HOP_AI(item, 0x2c) == 2)
			item[5] = (float)(int)((Core_Rand() & 0x7f) - 0x40) + 350.0f;
		r = (unsigned int)hop_rand() & 3;
		if (r == 0) {
			HOP_AI(item, 0x20) = 5;
			HOP_AI(item, 0x28) = 0;
		} else if (r == 1) {
			HOP_AI(item, 0x20) = 3;
			HOP_AI(item, 0x28) = 1;
		} else {
			HOP_AI(item, 0x20) = 1;
			HOP_AI(item, 0x28) = 2;
		}
		HOP_IV(0x002b0104)[HOP_AI(item, 0x2c)] += HOP_AI(item, 0x20);
		break;
	case 3:
	case 4:
		item[5] = (float)(hop_rand() & 0x1ff) + 295.0f;
	gold:
		HOP_AI(item, 0x20) = 5;
		HOP_AI(item, 0x28) = 0;
		break;
	}
	item[1] = hop_Variation == 1 ? -90.0f : 0.0f;
	item[3] = 1.0f;
	for (tries = 0;;) {
		a = (float)hop_rand() * HOP_DEG;
		overlap = 0;
		x = sinf(a) * item[5];
		item[0] = x;
		z = cosf(a) * item[5];
		item[2] = z;
		n = HOP_AI(system, 0x18);
		for (k = 0; k < n; k++) {
			other = HOP_AT(HOP_AP(system, 0x38), k * 4, float *);
			if (other != item && sqrtf((x - other[0]) * (x - other[0]) + (z - other[2]) * (z - other[2])) < 150.0f) {
				overlap = 1;
				k = n;
			}
		}
		if (!overlap) {
			for (k = 0; k < 4; k++) {
				dx = x - HOP_AF(hop_PlayerActor(k), 0xd0);
				dz = z - HOP_AF(hop_PlayerActor(k), 0xd8);
				if (sqrtf(dx * dx + dz * dz) < 150.0f)
					overlap = 1;
			}
		}
		tries++;
		if (!overlap || 4 < tries)
			break;
	}
	item[4] = 12.0f;
	HOP_AI(item, 0x18) = hop_rand() & 0xff;
	HOP_AI(item, 0x1c) = 0;
	HOP_AI(item, 0x24) = 1;
	mem = HOP_NEW(0x290, "C:\\BlitzParty\\Game\\Src\\Hopper\\CHopper_Fx.cpp", 0x1ea);
	if (mem == NULL)
		HOP_AP(item, 0x30) = NULL;
	else
		HOP_AP(item, 0x30) = CToken_Init((struct CToken *)mem, item, 2 - HOP_AI(item, 0x28), 0.8f, NULL);
	return 0;
}

/* 0x001493f0  (gap): token update. Every player gets the nearest token (+0x1188/+0x118c,
distance +0x1190); a player within 96 collects it (its value floats up, the score grows, a
new token appears while the round runs). Tokens spin. CTokens without an item are deleted
first. (A collected item's memory is still spun, and the next item is skipped, as in the
original.) */
int HopFx_TokenUpdate(struct BExtraSystem *system, int count, void **items, void *arg)
{
	char text[12];
	float *item;
	void *token;
	void *player;
	void *actor;
	unsigned char *colour;
	float dx, dy, dz, d;
	int found, k, i, n, spin;

	(void)arg;
	for (k = 0; k < 4; k++)
		HOP_AF(hop_Players[k], 0x1190) = 99999.0f;
	for (token = CToken_Next(NULL); token != NULL; token = CToken_Next(token)) {
		found = 0;
		for (i = 0; i < count; i++) {
			if (HOP_AP(items[i], 0x30) == token) {
				found = 1;
				i = count;
			}
		}
		if (!found) {
			CToken_Delete((struct CToken *)token);
			token = NULL;
		}
	}
	n = count;
	for (i = 0; i < n; i++) {
		item = (float *)items[i];
		for (k = 0; k < DAT_002bae94; k++) {
			player = hop_Players[k];
			actor = HOP_AP(player, 0x30);
			dx = HOP_AF(actor, 0xd0) - item[0];
			dy = HOP_AF(actor, 0xd4) - item[1];
			dz = HOP_AF(actor, 0xd8) - item[2];
			d = sqrtf(dx * dx + dy * dy + dz * dz);
			if (d < HOP_AF(player, 0x1190)) {
				HOP_AF(player, 0x1190) = d;
				HOP_AF(hop_Players[k], 0x1188) = item[0];
				HOP_AF(hop_Players[k], 0x118c) = item[2];
			}
			if (d < 96.0f) {
				if (hop_Variation == 2)
					HOP_IV(0x002b0104)[HOP_AI(item, 0x2c)] -= HOP_AI(item, 0x20);
				sprintf(text, "%d", HOP_AI(item, 0x20));
				actor = hop_PlayerActor(k);
				colour = DAT_00290718 + DAT_002baed4[k][0] * 0x60;
				baAddExtraItem(HOP_G(0x002aeac0, struct BExtraSystem *), HOP_P(0x002ae99c), (double)HOP_AF(actor, 0xd0),
					(double)HOP_AF(actor, 0xd4), (double)HOP_AF(actor, 0xd8), (int)colour[0], (int)colour[1],
					(int)colour[2], text);
				CToken_Collect((struct CToken *)HOP_AP(item, 0x30));
				baDeleteExtraItem(system, i);
				count--;
				if (hop_RoundOver == 0)
					baAddExtraItem(HOP_G(0x002affd0, struct BExtraSystem *));
				HOP_IV(0x002aff24)[k] += HOP_AI(item, 0x20);
			}
			n = count;
		}
		spin = HOP_AI(item, 0x18);
		HOP_AI(item, 0x18) = spin + 8;
		if (0x168 < spin + 8)
			HOP_AI(item, 0x18) = spin - 0x160;
		HOP_AI(item, 0x1c) = HOP_AI(item, 0x1c) + 1;
	}
	return 0;
}

/* ======================================================================== steam */

/* 0x00149650  (gap; not in the function table): steam creation. args: x, y, z (doubles).
Item (0x18 bytes): [0..2] position (jittered by -128..127), [4] phase (int), [5] age
(int). */
int FUN_00149650(struct BExtraSystem *system, float *item, void *args)
{
	(void)system;
	if (args == NULL)
		return 0;
	{
		HOP_ARGS(args);
		item[0] = (float)va_arg(ap, double);
		item[1] = (float)va_arg(ap, double);
		item[3] = 1.0f;
		item[2] = (float)va_arg(ap, double);
	}
	item[0] = (float)((hop_rand() & 0xff) - 0x80) + item[0];
	item[1] = (float)((hop_rand() & 0xff) - 0x80) + item[1];
	item[2] = (float)((hop_rand() & 0xff) - 0x80) + item[2];
	HOP_AI(item, 0x10) = hop_rand() & 0xff;
	HOP_AI(item, 0x14) = 0;
	return 0;
}

/* 0x00149740  (gap; not in the function table): steam update: rises 10 a frame up to 1000,
wobbling and drifting 2 a frame towards x = 0, z = 0 */
int FUN_00149740(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float *item;
	float x, z;
	int i = 0, p;

	(void)arg;
	while (i < count) {
		item = (float *)items[i];
		if (item[1] <= 1000.0f) {
			p = HOP_AI(item, 0x10) + 1;
			HOP_AI(item, 0x10) = p;
			if (p == 0x168)
				HOP_AI(item, 0x10) = 0;
			x = sinf((float)HOP_AI(item, 0x10) * HOP_DEG) * 2.0f + item[0];
			item[0] = x;
			item[1] = item[1] + 10.0f;
			z = cosf((float)HOP_AI(item, 0x10) * HOP_DEG) * 2.0f + item[2];
			item[2] = z;
			if (0.0f < x)
				item[0] = x - 2.0f;
			if (item[0] < 0.0f)
				item[0] = item[0] + 2.0f;
			if (0.0f < z)
				item[2] = z - 2.0f;
			if (item[2] < 0.0f)
				item[2] = item[2] + 2.0f;
			HOP_AI(item, 0x14) = HOP_AI(item, 0x14) + 1;
			i++;
		} else {
			baDeleteExtraItem(system, i);
			count--;
		}
	}
	return 0;
}

/* ======================================================================== electricity */

/* 0x00149850  (gap; not in the function table): electric draw: each arc is a strip of 8 (a
line) or 32 (a circle of radius 220 around the core) segments, 200 wide, drawn with the
three electric textures (scrolled) in the electricity's brightness (+0x80). In variation 3
the arcs flicker off on odd frames while the core cycle is between 321 and 383. */
int FUN_00149850(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float pts[50][4];
	float *strip = (float *)HOP_ADDR(0x00302f98);
	float *item;
	float *v;
	float segs, dy, du, ang, a;
	unsigned int argb;
	int extra, n, i, t, k;

	(void)system;
	(void)arg;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(8, 0, 0);
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
	if (hop_Variation == 3 && DAT_002ecdec == 0 && 0x140 < HOP_I(0x002b0024) && HOP_I(0x002b0024) < 0x180 &&
		(HOP_B(0x003027b4) & 1) != 0)
		return 0;
	for (k = 0; k < count; k++) {
		item = (float *)items[k];
		extra = HOP_AI(item, 0x20) != 1 ? 0x18 : 0;
		n = extra + 7;                 /* segments - 1 */
		segs = (float)n;
		dy = (item[5] - item[1]) / segs;
		for (i = 0; i < extra + 8; i++) {
			if (HOP_AI(item, 0x20) == 1) {
				pts[i][0] = (item[4] - item[0]) / segs * (float)i + item[0];
				pts[i][1] = dy * (float)i + item[1];
				pts[i][2] = (float)i * ((item[6] - item[2]) / segs) + item[2];
			} else {
				a = (20.0f * (float)i + item[12]) * HOP_DEG;
				pts[i][0] = sinf(a) * 220.0f;
				pts[i][1] = (float)i * dy + item[1];
				pts[i][2] = cosf(a) * 220.0f;
			}
		}
		pts[n + 1][0] = item[4];
		pts[n + 1][1] = item[5];
		pts[n + 1][2] = item[6];
		for (t = 0; t < 3; t++) {
			bdSetTexture(0, hop_ElectricTextures[t]);
			du = 1.0f / segs;
			argb = HOP_ARGB(255, HOP_I(0x002b0020), HOP_I(0x002b0020), HOP_I(0x002b0020));
			for (i = 0; i < extra + 8; i++) {
				if (HOP_AI(item, 0x20) == 2)
					ang = i == 0 ? 0.0f : 1.5707964f;
				else
					ang = atan2f(pts[i + 1][0] - pts[i][0], pts[i + 1][1] - pts[i][1]);
				v = strip + i * 0x12;
				v[2] = pts[i][2];
				HOP_AU(v, 0x18) = argb;
				v[0] = sinf(ang - 1.5707964f) * 100.0f + pts[i][0];
				v[1] = cosf(ang - 1.5707964f) * 100.0f + pts[i][1];
				v[11] = pts[i][2];
				v[7] = du * item[13] * (float)i + item[9 + t];
				v[8] = 0.0f;
				HOP_AU(v, 0x3c) = argb;
				v[9] = sinf(ang + 1.5707964f) * 100.0f + pts[i][0];
				v[10] = cosf(ang + 1.5707964f) * 100.0f + pts[i][1];
				v[0x11] = 1.0f;
				v[0x10] = du * item[13] * (float)i + item[9 + t];
			}
			bdDrawPrimitiveFVF(4, strip, extra + 9 + n, 0x152);
		}
	}
	bdSetRenderState(8, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 1, 0);
	return 0;
}

/* ======================================================================== poles (variation 4) */

/* 0x00149c10  (gap; not in the function table): pole creation. args: an int (overwritten).
Item (0x40 bytes): [0..2] position (x, z 0), [4] 1: low, 0: high (int), [5] the camera
angle when made, [6] 0, [7] side (int, 0 low / 1 high), [8..11] per player: may still score
(ints, 1). The pole starts high above the arena (2700 + 400, +150 when high, less twice
1188 - the sink speed). */
int FUN_00149c10(struct BExtraSystem *system, float *item, void *args)
{
	unsigned int r;
	float y, t;

	(void)system;
	if (args == NULL)
		return 0;
	{
		HOP_ARGS(args);
		HOP_AI(item, 0x10) = va_arg(ap, int);
	}
	item[5] = hop_CamAngle;
	item[1] = 2700.0f;
	item[6] = 0.0f;
	item[0] = sinf(item[5] * HOP_DEG) * 0.0f;
	r = (unsigned int)hop_rand() & 1;
	HOP_AI(item, 0x1c) = (int)r;
	y = (float)HOP_AI(item, 0x1c) * 150.0f + item[1] + 400.0f;
	HOP_AI(item, 0x10) = r == 0;
	item[1] = y;
	t = 1188.0f - HOP_F(0x002af160);
	HOP_AI(item, 0x20) = 1;
	HOP_AI(item, 0x24) = 1;
	HOP_AI(item, 0x28) = 1;
	HOP_AI(item, 0x2c) = 1;
	item[1] = y - (t + t);
	item[2] = cosf(item[5] * HOP_DEG) * item[6];
	return 0;
}

/* 0x00149cd0  (gap; not in the function table): pole update (variation 4). The poles sink
with the arena (deleted below -10000) and turn against the arm. Below y -400 each pole
sweeps the arm: a CPU player sees it coming (97..102 degrees ahead) and, by difficulty,
jumps (low pole) or ducks (high pole); a player inside the pole's sweep (a quad from the
centre out 1365, as wide as the arm's speed * 30, 50..180) who is not dodging correctly is
hit: knocked out when 60 or more along the arm (the commentator speaks when fewer than two
are out), else knocked back 10. Players who let a pole pass score. */
int FUN_00149cd0(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float poly[8];
	float *item;
	void *player;
	void *actor;
	float d, w, s;
	int lowHits = 0, highHits = 0;
	int first = 1;
	int i = 0, k, mod = 0, n;
	int *flags;
	short st;
	unsigned int r;

	(void)arg;
	if (hop_RoundOver != 0 || DAT_002ecdec != 0)
		return 0;
	while (i < count) {
		item = (float *)items[i];
		item[1] = item[1] - HOP_F(0x002af160);
		if (!(-10000.0f <= item[1])) {
			baDeleteExtraItem(system, i);
			count--;
			continue;
		}
		for (k = 0; k < 4; k++) {
			flags = (int *)item + 8 + k;
			if (HOP_IV(0x002aff54)[k] != 0)
				continue;
			if (0 < HOP_IV(0x002aff64)[k])
				HOP_IV(0x002aff64)[k]--;
			if (!(-400.0f <= item[1]))
				continue;
			player = hop_Players[k];
			d = CLMath_AngleDistance((hop_CamAngle - (125.0f - HOP_AF(player, 0x1180))) * HOP_DEG, item[5] * HOP_DEG);
			if (DAT_002baed4[k][1] == 0 && 97.0f < d * 57.29578f && d * 57.29578f < 102.0f &&
				HOP_AI(hop_Players[k], 0x1178) == 0) {
				if (DAT_002bae70 == 0) {
					mod = 6;
				} else if (DAT_002bae70 == 1) {
					if (first) {
						mod = 6;
						first = 0;
					} else {
						mod = 8;
					}
				} else if (DAT_002bae70 == 2) {
					if (first) {
						mod = 0xc;
						first = 0;
					} else {
						mod = 0x14;
					}
				}
				r = (unsigned int)hop_rand();
				HOP_AU(hop_Players[k], 0x117c) = (int)r % mod == 0;
				player = hop_Players[k];
				if (HOP_AS(player, 0x36) != 3 && HOP_AS(player, 0x36) != 0xe) {
					if (((HOP_AI(player, 0x117c) + HOP_AI(item, 0x1c)) & 1) == 0) {
						if (HOP_AI(HOP_AP(player, 0x30), 0x214) == 0)
							goto counted;
						CLSamples_PlayFromSet((char)DAT_002baed4[HOP_AUS(HOP_AP(player, 0x30), 0x230)][0], 0, NULL, 200,
							0, -1, 0);
						AI_SetState(player, 3);
					} else {
						CLSamples_PlayFromSet((char)DAT_002baed4[HOP_AUS(HOP_AP(player, 0x30), 0x230)][0], 0, NULL, 200,
							0, -1, 0);
						AI_SetState(player, 0xe);
					}
					HOP_AI(hop_Players[k], 0x1178) = 0x30;
				}
			}
		counted:
			if (0 < HOP_AI(hop_Players[k], 0x1178))
				HOP_AI(hop_Players[k], 0x1178)--;
			poly[0] = 0.0f;
			poly[1] = 0.0f;
			poly[2] = sinf((item[5] - 90.0f) * HOP_DEG) * 1365.0f;
			poly[3] = cosf((item[5] - 90.0f) * HOP_DEG) * 1365.0f;
			w = HOP_F(0x002aed74) * 30.0f;
			if (50.0f <= w) {
				if (180.0f < w)
					w = 180.0f;
			} else {
				w = 50.0f;
			}
			s = sinf(item[5] * HOP_DEG);
			poly[4] = -(s * w) + poly[2];
			poly[6] = 0.0f;
			poly[7] = 0.0f;
			poly[5] = -(cosf(item[5] * HOP_DEG) * w) + poly[3];
			actor = hop_PlayerActor(k);
			if (FUN_00121990(NULL, HOP_AF(actor, 0xd0), HOP_AF(actor, 0xd8), 4, poly) == 0) {
				if (*flags == 2 && HOP_IV(0x002aff64)[k] == 0) {
					HOP_IV(0x002aff24)[k]++;
					HOP_IV(0x002aff64)[k] = 8;
					*flags = 0;
				}
				if (HOP_IV(0x002afee4)[k] != 0)
					HOP_IV(0x002afee4)[k] = 2;
			} else {
				st = HOP_AS(hop_Players[k], 0x36);
				if (st != 3 && st != 4 && st != 9 && HOP_AI(item, 0x1c) == 0) {
					HOP_IV(0x002afee4)[k] = 1;
					*flags = 0;
				}
				if (HOP_AS(hop_Players[k], 0x36) == 0xe || HOP_AI(item, 0x1c) != 1) {
					if (*flags != 0 && HOP_IV(0x002aff54)[k] == 0)
						*flags = 2;
				} else {
					HOP_IV(0x002afee4)[k] = 1;
					*flags = 0;
				}
			}
			if (HOP_IV(0x002afee4)[k] == 2) {
				HOP_IV(0x002afee4)[k] = 0;
				if (HOP_IV(0x002aff14)[k] == 0) {
					if (HOP_AI(item, 0x1c) == 0)
						lowHits++;
					else
						highHits++;
					player = hop_Players[k];
					HOP_IV(0x002aff14)[k] = 0x14;
					if (60.0f <= HOP_AF(player, 0x1180)) {
						CLSamples_PlayFromSet((char)DAT_002baed4[HOP_AUS(HOP_AP(player, 0x30), 0x230)][0], 3, NULL, 200, 0,
							-1, 0);
						bsPlaySample(HOP_G(0x002ae9bc, struct BSample *), 0xfa, 0, 0, 0);
						n = HOP_AS(hop_Players[0], 0x36) == 0xd;
						if (HOP_AS(hop_Players[1], 0x36) == 0xd)
							n++;
						if (HOP_AS(hop_Players[2], 0x36) == 0xd)
							n++;
						if (HOP_AS(hop_Players[3], 0x36) == 0xd)
							n++;
						if (n < 2) {
							bkPrintf("saying it for %d mancount=%d\n", k, n);
							CLSamples_PlayCommentary(0x37, DAT_002baed4[k][0], 0xff, 0, 0, NULL);
						} else {
							bkPrintf("not saying it, mancount=%d\n", n);
						}
						HOP_AI(hop_Players[k], 0x1194) = HOP_AI(item, 0x1c);
						AI_SetState(hop_Players[k], 0xd);
						player = hop_Players[k];
					} else {
						CLSamples_PlayFromSet((char)DAT_002baed4[HOP_AUS(HOP_AP(player, 0x30), 0x230)][0], 2, NULL, 200, 0,
							-1, 0);
						player = hop_Players[k];
					}
					HOP_AF(player, 0x1184) = HOP_AF(player, 0x1184) + 10.0f;
					CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, k, 0, 0x7fff, 500, -1, 0x3c, 0, 0);
				}
			}
		}
		if (highHits != 0)
			bsPlaySample(HOP_G(0x002ae9b4, struct BSample *), (highHits + 4) * 0x19, 0, 0, 0);
		if (lowHits != 0)
			bsPlaySample(HOP_G(0x002ae9b8, struct BSample *), (lowHits + 4) * 0x19, 0, 0, 0);
		i++;
		item[5] = (-HOP_F(0x002aed70) - HOP_F(0x002aed74)) + item[5];
	}
	return 0;
}

/* 0x0014a360  (gap; not in the function table): pole draw: each pole (extra instance by its
kind) at its height and turn, with the steam (extra instance 4) 8 above it, additive */
int FUN_0014a360(struct BExtraSystem *system, int count, void **items, void *arg)
{
	float m[16];
	float *item;
	struct BActorInstance *inst;
	struct BActorInstance *steam;
	int i;

	(void)system;
	(void)arg;
	if (HOP_B(0x003027b0) == 0)
		return 0;
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
	HopRes_SetRenderStates(hop_Res);
	for (i = 0; i < count; i++) {
		item = (float *)items[i];
		inst = HOP_AT(HOP_ADDR(0x002af000), HOP_AI(item, 0x10) * 4, struct BActorInstance *);
		HOP_AF(inst, 0xd0) = 1.0f;
		HOP_AF(inst, 0xd4) = 1.0f;
		HOP_AF(inst, 0xd8) = 1.0f;
		HOP_AF(inst, 0xc0) = item[0];
		HOP_AF(inst, 0xc4) = item[1] - hop_Sink;
		HOP_AF(inst, 0xc8) = item[2];
		HOP_AF(inst, 0xcc) = 1.0f;
		bdSetWorldIdentity();
		bmMatrixRotationY(m, (HOP_F(0x002aed74) * 0.5f * 0.0f + (item[5] - 3.0f * HOP_F(0x002aed74) * 0.5f)) * HOP_DEG);
		bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
		baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), 0, 1);
		baPrepActorInstance(inst, 1);
		baDrawActorInstance(inst, 7);
		steam = HOP_G(0x002af010, struct BActorInstance *);
		memcpy((char *)steam + 0xd0, (char *)inst + 0xd0, 16);
		memcpy((char *)steam + 0xc0, (char *)inst + 0xc0, 16);
		HOP_AF(steam, 0xc4) = HOP_AF(steam, 0xc4) + 8.0f;
		bmMatrixToQuaternion(HOP_AV(steam, 0xe0), m);
		baSetNodeTime(steam, (struct BActorNode *)HOP_AP(steam, 0x134), 0, 1);
		baPrepActorInstance(steam, 1);
		bdPushRenderState(6, 1, 0);
		bdPushRenderState(10, 3, 0);
		baDrawActorInstance(steam, 7);
		bdPopRenderState(6);
		bdPopRenderState(10);
	}
	return 0;
}

/* ======================================================================== steam jets, foam, sparks, splashes */

/* 0x0014a5e0  (gap; not in the function table): steam jet creation. args: x, y, z
(doubles). Item: [0..2] position (z jittered by -256..255), [4] spin (int), [5] age (int). */
int FUN_0014a5e0(struct BExtraSystem *system, float *item, void *args)
{
	(void)system;
	if (args == NULL)
		return 0;
	{
		HOP_ARGS(args);
		item[0] = (float)va_arg(ap, double);
		item[1] = (float)va_arg(ap, double);
		item[3] = 1.0f;
		item[2] = (float)va_arg(ap, double);
	}
	item[2] = ((float)(hop_rand() & 0x1ff) - 256.0f) + item[2];
	HOP_AI(item, 0x10) = hop_rand() & 0xff;
	HOP_AI(item, 0x14) = 0;
	return 0;
}

/* 0x0014a670  (gap; not in the function table): foam creation (no args): along a line at
118 degrees, 23.75 further out each time from 600 (counter 0x002b002c). Item: [0..2]
position, [4] spin (int), [5] age (int). */
int FUN_0014a670(struct BExtraSystem *system, float *item, void *args)
{
	float r;

	(void)system;
	(void)args;
	r = HOP_F(0x002b002c) * 23.75f + 600.0f;
	HOP_F(0x002b002c) = HOP_F(0x002b002c) + 1.0f;
	item[1] = 10.0f;
	item[3] = 1.0f;
	item[0] = (float)(sin(2.059488534927368) * r);
	item[2] = (float)(cos(2.059488534927368) * r);
	HOP_AI(item, 0x10) = hop_rand() & 0xff;
	HOP_AI(item, 0x14) = 0;
	return 0;
}

/* 0x0014a6f0  (gap): foam update: spins by 0..15 a frame, ages */
int HopFx_FoamUpdate(struct BExtraSystem *system, int count, void **items, void *arg)
{
	void *item;
	int i;

	(void)system;
	(void)arg;
	for (i = 0; i < count; i++) {
		item = items[i];
		HOP_AU(item, 0x10) = (unsigned int)HOP_AI(item, 0x10) + ((unsigned int)hop_rand() & 0xf);
		HOP_AI(item, 0x14) = HOP_AI(item, 0x14) + 1;
	}
	return 0;
}

/* 0x0014a740  (gap; not in the function table): spark trail creation. args: x, y, z
(doubles). Item (0x114 bytes): [0..31] 8 trail points (the rest 2 apart below the head),
[0x20..0x22] velocity (-32..31, 0..15, -32..31), [0x24] age, [0x25..0x44] the points'
colours (r, g, b, a ints: 168..0, 168..0, 224..0, 255). */
int FUN_0014a740(struct BExtraSystem *system, float *item, void *args)
{
	HOP_ARGS(args);
	int *colour;
	int c, d, k;

	(void)system;
	item[0] = (float)va_arg(ap, double);
	item[1] = (float)va_arg(ap, double);
	item[2] = (float)va_arg(ap, double);
	item[3] = 1.0f;
	for (k = 1; k < 8; k++) {
		item[k * 4 + 0] = item[0];
		item[k * 4 + 1] = item[1] - ((float)k + (float)k);
		item[k * 4 + 2] = item[2];
		item[k * 4 + 3] = 1.0f;
	}
	colour = (int *)item + 0x25;
	for (c = 0xa8, d = 0xe0; -0x18 < c; c -= 0x18, d -= 0x20, colour += 4) {
		colour[0] = c;
		colour[1] = c;
		colour[2] = d;
		colour[3] = 0xff;
	}
	item[0x20] = (float)(hop_rand() & 0x3f) - 32.0f;
	item[0x21] = (float)(hop_rand() & 0xf);
	item[0x24] = 0.0f;
	item[0x22] = (float)(hop_rand() & 0x3f) - 32.0f;
	return 0;
}

/* 0x0014a870  (gap; not in the function table): splash creation. args: x, y, z (doubles).
Item (0x28 bytes): [0..2] position, [4..6] spread (-16 or 0), [8] age, [9] the sprite's
angle. */
int FUN_0014a870(struct BExtraSystem *system, float *item, void *args)
{
	HOP_ARGS(args);

	(void)system;
	item[0] = (float)va_arg(ap, double);
	item[1] = (float)va_arg(ap, double);
	item[2] = (float)va_arg(ap, double);
	item[4] = (float)(hop_rand() & 0x10) - 16.0f;
	item[5] = 0.0f;
	item[6] = (float)(hop_rand() & 0x10) - 16.0f;
	item[9] = (float)(hop_rand() & 0x1ff) * HOP_DEG;
	item[3] = 1.0f;
	item[8] = 0.0f;
	return 0;
}

/* ======================================================================== the effects object */

/* 0x0014a940  (gap): creates the extra systems and the variation's first effects */
void HopFx_Start(void *self)
{
	FX_SYS(self, 0x14) = baCreateExtraSystem("bubble", (int (*)())FUN_00148d80, (int (*)())FUN_00148eb0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_001474a0, (int (*)())FUN_00147420, 0x30, 0x400, 1, 4);
	FX_SYS(self, 0x30) = baCreateExtraSystem("token", (int (*)())FUN_00148ff0, (int (*)())FUN_001493f0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_001d5950, (int (*)())FUN_001d5950, 0x34, 0x20, 1, 0);
	FX_SYS(self, 0x38) = baCreateExtraSystem("steam", (int (*)())FUN_00149650, (int (*)())FUN_00149740,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00147660, (int (*)())FUN_001d5950, 0x18, 0x400, 1, 0);
	FX_SYS(self, 0x4c) = baCreateExtraSystem("electric", (int (*)())FUN_00147790, (int (*)())FUN_00147820,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00149850, (int (*)())FUN_001d5950, 0x38, 0x800, 1, 0);
	FX_SYS(self, 0x50) = baCreateExtraSystem("pole", (int (*)())FUN_00149c10, (int (*)())FUN_00149cd0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_0014a360, (int (*)())FUN_001d5950, 0x40, 0x40, 1, 0);
	FX_SYS(self, 0x54) = baCreateExtraSystem("steamjet", (int (*)())FUN_0014a5e0, (int (*)())FUN_001479c0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00147a40, (int (*)())FUN_001d5950, 0x18, 0x400, 1, 0);
	FX_SYS(self, 0x58) = baCreateExtraSystem("flash", (int (*)())FUN_00147c80, (int (*)())FUN_00147ca0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00147cf0, (int (*)())FUN_001d5950, 8, 2, 1, 0);
	FX_SYS(self, 0x5c) = baCreateExtraSystem("foam", (int (*)())FUN_0014a670, (int (*)())FUN_0014a6f0,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00147b60, (int (*)())FUN_001d5950, 0x18, 0x80, 1, 0);
	FX_SYS(self, 0x60) = baCreateExtraSystem("respawn", (int (*)())FUN_001485d0, (int (*)())FUN_00148650,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00148790, (int (*)())FUN_001d5950, 0x18, 4, 1, 0);
	FX_SYS(self, 0x64) = baCreateExtraSystem("splash", (int (*)())FUN_0014a870, (int (*)())FUN_00148820,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_001488a0, (int (*)())FUN_001d5950, 0x28, 0x80, 1, 0);
	FX_SYS(self, 0x68) = baCreateExtraSystem("hsparks", (int (*)())FUN_0014a740, (int (*)())FUN_00147e70,
		(int (*)())FUN_000a6fa0, (int (*)())FUN_00147fb0, (int (*)())FUN_001d5950, 0x114, 0x40, 1, 0);
	switch (hop_Variation) {
	case 1:
		HopFx_Start1(self);
		return;
	case 2:
		HopFx_Start2(self);
		return;
	case 3:
		HopFx_Start3(self);
		return;
	case 4:
		HopFx_Start4(self);
		return;
	}
}

/* 0x0014abb0  (gap): variation 3 per frame: bubbles from six vents (with "detail", 1 in 4
frames); the core's cycle (528 frames): at 144 it charges (+0x80 up to 128), at 288 it
fires (a sound; +0x7c up to 128, +0x80 down to 32), at 480 it cools (+0x7c down to 8,
+0x80 back to 64) */
void HopFx_Update3(void *self)
{
	int t, v;

	HOP_I(0x003044b0) = HOP_I(0x003044b0) + 1;
	if (HOP_F(0x002ae964) != 0.0f && (hop_rand() & 3) == 0) {
		baAddExtraItem(FX_SYS(self, 0x14), -2272.0, -400.0, 630.0, 0);
		baAddExtraItem(FX_SYS(self, 0x14), -1937.0, -400.0, 1360.0, 0);
		baAddExtraItem(FX_SYS(self, 0x14), -866.0, -400.0, 2453.0, 0);
		baAddExtraItem(FX_SYS(self, 0x14), 999.0, -400.0, 2486.0, 0);
		baAddExtraItem(FX_SYS(self, 0x14), 1954.0, -400.0, 1338.0, 0);
		baAddExtraItem(FX_SYS(self, 0x14), 2250.0, -400.0, 614.0, 0);
	}
	t = HOP_AI(self, 0x84) + 1;
	HOP_AI(self, 0x84) = t;
	if (t < 0x1e1) {
		if (t == 0x1e0) {
			HOP_AI(self, 0x88) = 3;
		} else if (t == 0x90) {
			HOP_AI(self, 0x88) = 1;
		} else if (t == 0x120) {
			bsPlaySample(HOP_G(0x002ae9b4, struct BSample *), 200, 0, 0, 0);
			HOP_AI(self, 0x88) = 2;
		}
	} else if (t == 0x210) {
		HOP_AI(self, 0x84) = 0;
		HOP_AI(self, 0x7c) = 0;
		HOP_AI(self, 0x80) = 0x40;
	}
	switch (HOP_AI(self, 0x88)) {
	case 1:
		v = HOP_AI(self, 0x80);
		if (v < 0x80)
			HOP_AI(self, 0x80) = v + 2;
		return;
	case 2:
		if (HOP_AI(self, 0x7c) < 0x80)
			HOP_AI(self, 0x7c) = HOP_AI(self, 0x7c) + 2;
		if (HOP_AI(self, 0x80) < 0x21)
			return;
		HOP_AI(self, 0x80) = HOP_AI(self, 0x80) - 2;
		return;
	case 3:
		if (7 < HOP_AI(self, 0x7c))
			HOP_AI(self, 0x7c) = HOP_AI(self, 0x7c) - 2;
		v = HOP_AI(self, 0x80);
		if (v < 0x40)
			HOP_AI(self, 0x80) = v + 2;
		return;
	}
}

/* 0x0014adb0  (gap): variation 4 per frame: rising bubbles at the four corners (1 in 2
frames) and three loose ones; in the last 55 seconds poles come (more often the faster the
arm turns) */
void HopFx_Update4(void *self)
{
	unsigned int r1, r2, r3;
	int a, b;

	if ((hop_rand() & 1) != 0) {
		baAddExtraItem(FX_SYS(self, 0x14), 800.0, -1317.0, 6300.0, 1);
		baAddExtraItem(FX_SYS(self, 0x14), -720.0, -1317.0, 6430.0, 1);
		baAddExtraItem(FX_SYS(self, 0x14), 710.0, -1317.0, -6200.0, 1);
		baAddExtraItem(FX_SYS(self, 0x14), -800.0, -1317.0, -6200.0, 1);
		r1 = (unsigned int)hop_rand();
		r2 = (unsigned int)hop_rand();
		r3 = (unsigned int)hop_rand();
		baAddExtraItem(FX_SYS(self, 0x14), (double)(((float)(r1 & 0x7fb) - 1024.0f) - 5748.0f),
			(double)(((float)(r2 & 0x7fb) - 1024.0f) - 2328.0f), (double)((float)(r3 & 0x7fb) - 1024.0f), 0);
		r1 = (unsigned int)hop_rand();
		r2 = (unsigned int)hop_rand();
		r3 = (unsigned int)hop_rand();
		baAddExtraItem(FX_SYS(self, 0x14), (double)(((float)(r1 & 0x7fb) - 1024.0f) + 5748.0f),
			(double)(((float)(r2 & 0x7fb) - 1024.0f) - 2328.0f), (double)((float)(r3 & 0x7fb) - 1024.0f), 0);
		r1 = (unsigned int)hop_rand();
		r2 = (unsigned int)hop_rand();
		baAddExtraItem(FX_SYS(self, 0x14), (double)((float)(r1 & 0x1fff) - 4096.0f), -3000.0,
			(double)((float)(r2 & 0x1fff) - 4096.0f), 0);
	}
	HOP_I(0x003044b4) = HOP_I(0x003044b4) + 1;
	if (hop_TimeLeft < 0x157c) {
		a = abs(HOP_FTOL(HOP_F(0x002af158))) % 0x3c;
		b = abs(HOP_FTOL(HOP_F(0x002aed70)));
		if (a < b)
			baAddExtraItem(FX_SYS(self, 0x50), 1);
	}
}

/* 0x0014b090  (gap): the effects per frame: the variation's own, then every extra system
(dt 1/60) */
void HopFx_Update(void *self)
{
	switch (hop_Variation) {
	case 2:
		HopFx_Steam2(self);
		baUpdateExtras(NULL, (void *)(uintptr_t)0x3c888889u);
		return;
	case 3:
		HopFx_Update3(self);
		baUpdateExtras(NULL, (void *)(uintptr_t)0x3c888889u);
		return;
	case 4:
		HopFx_Update4(self);
		break;
	}
	baUpdateExtras(NULL, (void *)(uintptr_t)0x3c888889u);
}
