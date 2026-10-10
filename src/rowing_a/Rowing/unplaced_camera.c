/*
Rowing/unplaced_camera.c  <-  Rowing gap 0x001add20-0x001af380 (no file tag)

The Rowing camera (game +0x84, 0xc0 bytes). These functions follow the player functions and
use only the camera's fields; no string names their source file.

The camera (fields at original offsets):
  +0x00 the "closeup" setting (0 normal, 1 player 0 only, 2 overhead of player 0)
  +0x04 a skip was asked for (results), +0x08 the fade (0..1), +0x0c the current target,
  +0x1c the wanted target, +0x2c the current eye, +0x3c the wanted eye (4 floats each)
  +0x4c the field of view, +0x50 the mode (bits 0-30; bit 31: the results fade is done)
  +0x54 the last DT, +0x58 the time in the results, +0x5c the skip fade, +0x60 skips
  +0x64 the current view was set, +0x68 the players' minimum, +0x78 maximum (x, y, z)
  +0x88 the view (the variant - 1), +0x8c the players' centre, +0x9c the pitch,
  +0xa0 the turning yaw, +0xa4 the yaw, +0xa8, +0xac, +0xb0 the players' spread,
  +0xb4 the closest distance, +0xb8 the close-up player, +0xbc the fade-in (0..1)

Modes: 0, 1 frame all boats; 2 turn round the boats (variant 3); 3 follow the course
(variant 4); 4 the results close-ups; 5 overhead of player 0; 6 the single human's head.
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"

#define CAM_I32(off) RA_I32(self, off)
#define CAM_U32(off) RA_U32(self, off)
#define CAM_F32(off) RA_F32(self, off)
#define CAM_FP(off) RA_FP(self, off)

#define g_RowGameFlags RW_VAR(unsigned int, 0x0030f0b0)
#define g_GameState RW_VAR(int, 0x0030f0b4)
#define g_RowPlayers RW_VAR(int, 0x0030f0d4)
#define g_RowPlayer(i) RW_VAR(void *, 0x0030f490 + (i) * 4)
#define g_SingleHuman RW_VAR(int, 0x0030f170)      /* game +0xc0 */
#define g_SingleHumanIndex RW_VAR(int, 0x0030f174) /* game +0xc4 */

extern int DAT_002bae6c;  /* core: the sound effects volume */
extern int DAT_002bae98;  /* core: the number of teams */
extern int DAT_002baebc;  /* core: team game */
extern int DAT_00283ef0;  /* core: a single human player gets the camera */
extern int DAT_002ba928;  /* core: the screen width */
extern int DAT_002ba92c;  /* core: the screen height */
extern unsigned int DAT_002bcc94; /* bdisplay: the alpha part of a colour */

/* the mode without bit 31 (sign-extended from 31 bits, as the original reads it) */
#define CAM_MODE() ((int)(CAM_U32(0x50) << 1) >> 1)
#define CAM_SET_MODE(m) (CAM_U32(0x50) = (CAM_U32(0x50) & 0x80000000u) | (unsigned int)(m))

static void ra_Copy4(float *dst, const float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}

/* 0x001add20  Rowing (gap)
The camera's constructor: all zero. */
void *FUN_001add20(void *self)
{
	memset(self, 0, 0xc0);
	return self;
}

/* 0x001add40  Rowing (gap)
Sets the camera up for VIEW (the variant - 1): field of view 70 degrees, pitch -1.083,
the fade, the mode and the closest distance (2000; 1000 and yaw pi/2 for view 2; 400 with
the "closeup" setting), near/far 40/80000. Returns 1. */
int FUN_001add40(void *self, int view)
{
	unsigned int mode;
	const char *setting;

	CAM_F32(0x98) = 1.0f;
	CAM_F32(0x18) = 1.0f;
	CAM_F32(0x28) = 1.0f;
	CAM_F32(0x38) = 1.0f;
	CAM_F32(0x48) = 1.0f;
	mode = CAM_U32(0x50) & 0x7fffffffu;
	CAM_F32(0x4c) = 1.22173047f;
	CAM_F32(0x9c) = -1.08330774f;
	CAM_I32(0xac) = 0;
	CAM_U32(0x50) = mode;
	CAM_I32(0x4) = 0;
	CAM_I32(0x5c) = 0;
	CAM_F32(0x8) = -1.0f;
	CAM_I32(0x60) = 0;
	CAM_I32(0x88) = view;
	CAM_I32(0xbc) = 0;
	switch (view) {
	case 0:
		CAM_F32(0xa0) = 3.1415925f;
		CAM_F32(0xa8) = 3.1415925f;
		CAM_F32(0xa4) = 3.1415925f;
		CAM_F32(0xb4) = 2000.0f;
		CAM_U32(0x50) = (mode & 0x80000000u) | 0;
		break;
	case 1:
		CAM_F32(0xa0) = 3.1415925f;
		CAM_F32(0xa8) = 3.1415925f;
		CAM_F32(0xa4) = 3.1415925f;
		CAM_F32(0xb4) = 2000.0f;
		CAM_U32(0x50) = (mode & 0x80000000u) | 1;
		break;
	case 2:
		CAM_F32(0xb4) = 1000.0f;
		CAM_F32(0xa0) = 1.57079625f;
		CAM_U32(0x50) = (mode & 0x80000000u) | 2;
		break;
	case 3:
		CAM_F32(0xb4) = 2000.0f;
		CAM_U32(0x50) = (mode & 0x80000000u) | 3;
		break;
	}
	bdSetNearFar(40.0f, 80000.0f);
	setting = Settings_Get((struct Settings *)DAT_002ba9a0, "closeup");
	CAM_I32(0x0) = setting != NULL ? (int)atol(setting) : 0;
	if (CAM_I32(0x0) != 0)
		CAM_F32(0xb4) = 400.0f;
	return 1;
}

/* 0x001ade80  Rowing (gap)
Starts the results close-ups (mode 4) on player INDEX. */
void FUN_001ade80(void *self, int index)
{
	CAM_I32(0xb8) = index;
	CAM_U32(0x50) = 4;
	CAM_I32(0x58) = 0;
}

/* 0x001adea0  Rowing (gap)
The fade-in (+0xbc, over 30 frames); at its end the sound effects come back to their
volume. */
void FUN_001adea0(void *self, float dt)
{
	if (CAM_F32(0xbc) < 1.0f) {
		CAM_F32(0xbc) = dt * 0.0333333351f + CAM_F32(0xbc);
		if (CAM_F32(0xbc) >= 1.0f)
			bsSetSfxVolume(DAT_002bae6c);
	}
}

/* 0x001adef0  Rowing (gap)
Draws a black screen-sized quad with alpha ALPHA (clamped 0-255) over the picture (the
vertices are the .bss 0x0030f6d8, 4 x 0x24 bytes). */
void FUN_001adef0(int alpha)
{
	float *v = (float *)RW_ADDR(0x0030f6d8);
	float w, h;
	unsigned int colour;

	if (alpha > 0xff)
		alpha = 0xff;
	else if (alpha < 0)
		alpha = 0;
	bdSetWorldIdentity();
	bdSetProjectionMode(0);
	bdSetRenderState(7, 1, 0);
	bdSetRenderState(5, 0, 0);
	w = (float)(DAT_002ba928 >> 1);
	h = (float)(DAT_002ba92c >> 1);
	colour = DAT_002bd640[alpha] | DAT_002bc490[alpha] | DAT_002bce18[alpha] | DAT_002bcc94;
	/* x, y, z at +0, +4, +8 and the colour at +0x18 of each 0x24-byte vertex */
	v[0] = -w;
	v[1] = -h;
	v[2] = 0.0f;
	RA_U32(v, 0x18) = colour;
	v[9] = -w;
	v[10] = h;
	v[11] = 0.0f;
	RA_U32(v, 0x3c) = colour;
	v[18] = w;
	v[19] = -h;
	v[20] = 0.0f;
	RA_U32(v, 0x60) = colour;
	v[27] = w;
	v[28] = h;
	v[29] = 0.0f;
	RA_U32(v, 0x84) = colour;
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdDrawPrimitive(4, v, 4);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(7, 3, 0);
	bdSetProjectionMode(1);
}

/* 0x001ae050  Rowing (gap)
Draws the camera's fades: the fade-in darkening; in modes 0-3, when FADE is set, the fade
out (+0x08 by DT / 30); in mode 4 the results fade (bit 31 when done) and the skip fade. */
void FUN_001ae050(void *self, int fade)
{
	int alpha, mode;
	float t;

	if (CAM_F32(0xbc) < 1.0f) {
		alpha = 0xff - (int)(CAM_F32(0xbc) * 255.0f);
		if (alpha < 0)
			alpha = 0;
		else if (alpha > 0xff)
			alpha = 0xff;
		CLScreen_DrawDarken(alpha, 1);
	}
	mode = CAM_MODE();
	if (mode < 0)
		return;
	if (mode <= 3) {
		if (CAM_F32(0x8) < 1.0f && fade != 0) {
			if (CAM_F32(0x8) < 0.0f)
				CAM_F32(0x8) = 0.0f;
			CAM_F32(0x8) = CAM_F32(0x54) * 0.0333333351f + CAM_F32(0x8);
			t = CAM_F32(0x8) < 1.0f ? CAM_F32(0x8) : 1.0f;
			FUN_001adef0((int)((1.0f - t) * 255.0f));
		}
	} else if (mode == 4) {
		if ((int)CAM_U32(0x50) >= 0 && CLFade_Draw(CAM_F32(0x54)) != 0)
			CAM_U32(0x50) |= 0x80000000u;
		if (CAM_I32(0x4) != 0) {
			t = CAM_F32(0x5c) < 1.0f ? CAM_F32(0x5c) : 1.0f;
			CLScreen_DrawDarken((int)(t * 255.0f), 1);
		}
	}
}

/* 0x001ae190  Rowing (gap)
The box round the boats in the game (only player 0's with the "closeup" setting): +0x68
minimum, +0x78 maximum. */
void FUN_001ae190(void *self)
{
	float p[4];
	int n = CAM_I32(0x0) != 0 ? 1 : g_RowPlayers, i;

	CAM_F32(0x78) = -99999.0f;
	CAM_F32(0x7c) = -99999.0f;
	CAM_F32(0x80) = -99999.0f;
	CAM_F32(0x68) = 99999.0f;
	CAM_F32(0x6c) = 99999.0f;
	CAM_F32(0x70) = 99999.0f;
	for (i = 0; i < n; i++) {
		if (RA_U8(g_RowPlayer(i), 0x37c) == 0)
			continue;
		FUN_001a7e10(g_RowPlayer(i), p);
		if (CAM_F32(0x78) < p[0])
			CAM_F32(0x78) = p[0];
		if (p[0] < CAM_F32(0x68))
			CAM_F32(0x68) = p[0];
		if (CAM_F32(0x7c) < p[1])
			CAM_F32(0x7c) = p[1];
		if (p[1] < CAM_F32(0x6c))
			CAM_F32(0x6c) = p[1];
		if (CAM_F32(0x80) < p[2])
			CAM_F32(0x80) = p[2];
		if (p[2] < CAM_F32(0x70))
			CAM_F32(0x70) = p[2];
	}
}

/* 0x001ae290  Rowing (gap)
Sets the view straight from the current eye and target (no smoothing). */
void FUN_001ae290(void *self)
{
	bdSetFieldOfView(CAM_F32(0x4c), CAM_F32(0x4c) * 3.0f * 0.25f);
	bdSetNearFar(40.0f, 80000.0f);
	bdSetCameraLookAt(CAM_FP(0x2c), CAM_FP(0xc), 0.0f);
	bdUpdateView();
}

/* 0x001ae2e0  Rowing (gap) */
void FUN_001ae2e0(void *self, float fov)
{
	CAM_F32(0x4c) = fov;
}

/* 0x001ae2f0  Rowing (gap)
Sets the field of view (4:3). */
void FUN_001ae2f0(void *self)
{
	bdSetFieldOfView(CAM_F32(0x4c), CAM_F32(0x4c) * 3.0f * 0.25f);
}

/* the INDEX-th player that has a close-up (0x001a7bb0 not -1), or -1 */
static int ra_NthCloseup(int index)
{
	int i, n = 0;

	for (i = 0; i < DAT_002bae94; i++) {
		if (FUN_001a7bb0(g_RowPlayer(i), NULL) != -1) {
			if (n == index)
				return i;
			n++;
		}
	}
	return -1;
}

/* 0x001ae310  Rowing (gap)
The results close-up player for the time +0x58: each winner in turn for 240 time units
(team game: 1/0.0041667; 360 when one team of one), or -1 when they are all shown. */
int FUN_001ae310(void *self)
{
	int count = 0, slot;
	float span;

	if (DAT_002baebc != 0) {
		FUN_001a7bb0(g_RowPlayer(0), &count);
		slot = (int)(CAM_F32(0x58) * 0.00416666688f);
		if (slot < count)
			return ra_NthCloseup(slot);
		return -1;
	}
	FUN_001a7bb0(g_RowPlayer(0), &count);
	if (count == 0)
		return -1;
	span = 240.0f;
	if (DAT_002bae98 == 1 && count == 1)
		span = 360.0f;
	slot = (int)(CAM_F32(0x58) / span);
	if (slot < count)
		return ra_NthCloseup(slot);
	return -1;
}

/* 0x001ae420  Rowing (gap)
1 when the camera is idle: no skip asked, no fade-out running, the fade-in done. */
int FUN_001ae420(void *self)
{
	if (CAM_I32(0x4) != 0)
		return 0;
	if (!(CAM_F32(0x8) < 0.0f) && CAM_F32(0x8) < 1.0f)
		return 0;
	return CAM_F32(0xbc) >= 1.0f ? 1 : 0;
}

/* 0x001ae470  Rowing (gap)
The boats' centre (+0x8c) and spread (+0xb0: 1.1 x the box diagonal, at least +0xb4). */
void FUN_001ae470(void *self)
{
	float dx, dy, dz;

	CAM_F32(0x8c) = (CAM_F32(0x78) - CAM_F32(0x68)) * 0.5f + CAM_F32(0x68);
	CAM_F32(0x90) = (CAM_F32(0x7c) - CAM_F32(0x6c)) * 0.5f + CAM_F32(0x6c);
	CAM_F32(0x94) = (CAM_F32(0x80) - CAM_F32(0x70)) * 0.5f + CAM_F32(0x70);
	dx = CAM_F32(0x78) - CAM_F32(0x68);
	dy = CAM_F32(0x7c) - CAM_F32(0x6c);
	dz = CAM_F32(0x80) - CAM_F32(0x70);
	CAM_F32(0xb0) = sqrtf(dx * dx + dy * dy + dz * dz) * 1.10000002f;
	if (CAM_F32(0xb0) < CAM_F32(0xb4))
		CAM_F32(0xb0) = CAM_F32(0xb4);
}

/* 0x001ae510  Rowing (gap)
The points to frame (PTS, 4 floats each; *COUNT of them): each boat in the game, plus its
look-ahead point in views 0 and 1, at height 20 (view 2) or 80 (view 3); view 2 adds the
centre at height 20, view 3 a point 250 up and 150 beyond the furthest boat. */
void FUN_001ae510(void *self, float *pts, int *count)
{
	void *p;
	int n, i;

	*count = 0;
	n = CAM_I32(0x0) != 0 ? 1 : g_RowPlayers;
	for (i = 0; i < n; i++) {
		p = g_RowPlayer(i);
		if (RA_U8(p, 0x37c) == 0)
			continue;
		switch (CAM_I32(0x88)) {
		case 0:
		case 1:
			FUN_001a7e10(p, pts + *count * 4);
			(*count)++;
			ra_Copy4(pts + *count * 4, RA_FP(g_RowPlayer(i), 0x35c));
			break;
		case 2:
			FUN_001a7e10(p, pts + *count * 4);
			pts[*count * 4 + 1] = 20.0f;
			break;
		case 3:
			FUN_001a7e10(p, pts + *count * 4);
			pts[*count * 4 + 1] = 80.0f;
			break;
		default:
			FUN_001a7e10(p, pts + *count * 4);
			break;
		}
		(*count)++;
	}
	if (CAM_I32(0x88) == 2) {
		pts[*count * 4] = 0.0f;
		pts[*count * 4 + 1] = 20.0f;
		pts[*count * 4 + 2] = 0.0f;
		pts[*count * 4 + 3] = 1.0f;
		(*count)++;
	} else if (CAM_I32(0x88) == 3) {
		pts[*count * 4] = 0.0f;
		pts[*count * 4 + 1] = 250.0f;
		pts[*count * 4 + 2] = CAM_F32(0x80) + 150.0f;
		pts[*count * 4 + 3] = 1.0f;
		(*count)++;
	}
}

/* 0x001ae6b0  Rowing (gap)
Frames the points of 0x001ae510 from direction (PITCH, YAW): turns them into view space,
pushes each out from their mean by MARGIN (+200 sideways in view 2; +250 vertically), finds
the distance at which the view (half field of view, 4:3) holds them all (at least MIN
closer than the mean), and sets the wanted target (+0x1c, the framed centre at the mean
depth) and eye (+0x3c) back in world space. */
void FUN_001ae6b0(void *self, float pitch, float yaw, float minDist, float margin)
{
	float pts[16 * 4], t[16 * 4], m[16], a[4], b[4];
	float sx = 0.0f, sy = 0.0f, sz, inv, mx, s, ks, lo, hi, x, distY, distX, cx, cy;
	int count, i;

	FUN_001ae510(self, pts, &count);
	bmMatrixRotationZXY(m, pitch, yaw, 0.0f);
	sz = 0.0f;
	for (i = 0; i < count; i++) {
		pts[i * 4 + 3] = 1.0f;
		bmTransformVector4(t + i * 4, m, pts + i * 4);
		sx += t[i * 4];
		sy += t[i * 4 + 1];
		sz += t[i * 4 + 2];
	}
	inv = 1.0f / (float)count;
	sx *= inv;
	sy *= inv;
	sz *= inv;
	for (i = 0; i < count; i++) {
		mx = margin;
		if (CAM_I32(0x88) == 2)
			mx = margin + 200.0f;
		if (t[i * 4] > sx)
			t[i * 4] = mx + t[i * 4];
		else if (t[i * 4] < sx)
			t[i * 4] = t[i * 4] - mx;
		if (t[i * 4 + 1] > sy)
			t[i * 4 + 1] = margin + t[i * 4 + 1];
		else if (t[i * 4 + 1] < sy)
			t[i * 4 + 1] = t[i * 4 + 1] - (margin + 250.0f);
	}

	s = sinf(CAM_F32(0x4c) * 0.5f);
	ks = 0.75f * s;
	lo = t[1] - t[2] * ks;
	hi = t[2] * ks + t[1];
	for (i = 0; i < count; i++) {
		x = t[i * 4 + 1] - ks * t[i * 4 + 2];
		if (x > lo)
			lo = x;
		x = ks * t[i * 4 + 2] + t[i * 4 + 1];
		if (x < hi)
			hi = x;
	}
	cy = (lo + hi) * 0.5f;
	distY = -((lo - hi) * 0.5f / ks);
	lo = t[0] - t[2] * s;
	hi = t[2] * s + t[0];
	for (i = 0; i < count; i++) {
		x = t[i * 4] - s * t[i * 4 + 2];
		if (x > lo)
			lo = x;
		x = s * t[i * 4 + 2] + t[i * 4];
		if (x < hi)
			hi = x;
	}
	cx = (lo + hi) * 0.5f;
	distX = -((lo - hi) * 0.5f / s);
	if (distX < distY)
		distY = distX;
	if (sz - distY < minDist)
		distY = sz - minDist;
	a[0] = cx;
	a[1] = cy;
	a[2] = distY;
	a[3] = 1.0f;
	b[0] = cx;
	b[1] = cy;
	b[2] = sz;
	b[3] = 1.0f;
	bmMatrixRotationYXZ(m, -pitch, -yaw, 0.0f);
	bmTransformVector4(CAM_FP(0x3c), m, a);
	bmTransformVector4(CAM_FP(0x1c), m, b);
}

/* 0x001aea40  Rowing (gap)
A results close-up of player +0xb8: target on the boat, eye 400 away (in front for view 3,
behind otherwise, a quarter to the side in views 0 and 1) and 100 up; the boat's instance
turns to face it. UNUSED (the frame time) is not read. */
void FUN_001aea40(void *self, float unused)
{
	void *p = g_RowPlayer(CAM_I32(0xb8));
	float dx, dy, fz;

	(void)unused;
	ra_Copy4(CAM_FP(0x1c), RA_FP(p, 0x2b4));
	ra_Copy4(CAM_FP(0xc), CAM_FP(0x1c));
	switch (CAM_I32(0x88)) {
	case 0:
	case 1:
		fz = 1.0f;
		dx = 0.25f;
		dy = 0.0f;
		break;
	case 2:
		fz = 1.0f;
		dx = 0.0f;
		dy = 0.0f;
		break;
	case 3:
		fz = -1.0f;
		dx = 0.0f;
		dy = 0.0f;
		break;
	default:
		/* the original reads three uninitialised stack words here (the view is always
		   0-3) */
		fz = 0.0f;
		dx = 0.0f;
		dy = 0.0f;
		break;
	}
	CAM_F32(0xb0) = 400.0f;
	CAM_F32(0x38) = 1.0f;
	CAM_F32(0x2c) = dx * 400.0f + CAM_F32(0xc);
	CAM_F32(0x34) = 400.0f * fz + CAM_F32(0x14);
	CAM_F32(0x30) = dy * 400.0f + CAM_F32(0x10) + 100.0f;
	ra_Copy4(CAM_FP(0x3c), CAM_FP(0x2c));
	bmQuaternionRotationY(RA_FP(RA_PTRF(g_RowPlayer(CAM_I32(0xb8)), 0x358), 0xe0), atan2f(dx, -fz));
}

/* frames the boats again in the loop of mode 2 */
static void ra_Frame(void *self, float pitch, float minDist, float margin)
{
	FUN_001ae6b0(self, pitch, CAM_F32(0xa4) + 3.1415925f, minDist, margin);
}

/* 0x001aeb90  Rowing (gap)
The camera's frame: picks the mode (5 for the overhead setting, 6 for a single human in the
race), moves the wanted eye and target by mode (see the file comment; mode 2 keeps the eye
within 1500 of the whirlpool's centre, mode 4 runs the results close-ups and their skip),
then eases the current eye and target towards them (DT / 20) and sets the view. */
void FUN_001aeb90(void *self, float dt)
{
	void *p, *drv, *node;
	float v[4], out[4], margin, a, t, r, inv, dx, dy, dz, h;
	int i, idx, human;

	CAM_F32(0x54) = dt;
	if (CAM_I32(0x0) == 2 && (CAM_U32(0x50) & 0x7fffffffu) != 4)
		CAM_U32(0x50) = (CAM_U32(0x50) & 0x80000005u) | 5;
	if (g_SingleHuman != 0 &&
		(g_GameState == 5 || g_GameState == 1 || g_GameState == 2 || g_GameState == 3 || g_GameState == 4)) {
		p = g_RowPlayer(g_SingleHumanIndex);
		if (RA_U8(p, 0x34a) == 0 && RA_U8(p, 0x349) == 0)
			CAM_U32(0x50) = (CAM_U32(0x50) & 0x80000006u) | 6;
	}

	switch (CAM_MODE()) {
	case 0:
	case 1:
		FUN_001ae6b0(self, CAM_F32(0x9c), CAM_F32(0xa4) + 3.1415925f, CAM_F32(0xb4), 200.0f);
		break;
	case 2:
		margin = g_GameState < 5 ? 300.0f : 100.0f;
		FUN_001ae190(self);
		CAM_F32(0x6c) = 0.0f;
		CAM_F32(0x7c) = 0.0f;
		if (CAM_I32(0x0) == 0) {
			if (CAM_F32(0x78) < 0.0f)
				CAM_F32(0x78) = 0.0f;
			if (CAM_F32(0x80) < 0.0f)
				CAM_F32(0x80) = 0.0f;
			if (CAM_F32(0x68) > 0.0f)
				CAM_F32(0x68) = 0.0f;
			if (CAM_F32(0x70) > 0.0f)
				CAM_F32(0x70) = 0.0f;
		}
		FUN_001ae470(self);
		ra_Copy4(CAM_FP(0x1c), CAM_FP(0x8c));
		CAM_F32(0x9c) = -0.628318489f;
		if (RA_I32(g_RowPlayer(0), 0x380) != 0)
			CAM_F32(0xa0) = dt * 0.00261799386f + CAM_F32(0xa0);
		if (CAM_F32(0xa0) > 3.1415925f) {
			a = CAM_F32(0xa0);
			do
				a -= 6.28318548f;
			while (a > 3.1415925f);
			CAM_F32(0xa0) = a;
		}
		CAM_F32(0xa4) = CAM_F32(0xa0);
		v[0] = 0.0f;
		v[1] = 0.0f;
		v[2] = CAM_F32(0xb0);
		bmVectorRotateXY(out, v, -0.628318489f, CAM_F32(0xa4));
		CAM_F32(0x3c) = out[0] + CAM_F32(0x1c);
		CAM_F32(0x40) = out[1] + CAM_F32(0x20);
		CAM_F32(0x44) = out[2] + CAM_F32(0x24);
		CAM_F32(0x1c) = (CAM_F32(0x3c) - CAM_F32(0x1c)) * 0.25f + CAM_F32(0x1c);
		CAM_F32(0x24) = (CAM_F32(0x44) - CAM_F32(0x24)) * 0.25f + CAM_F32(0x24);
		ra_Frame(self, CAM_F32(0x9c), 200.0f, margin);
		/* keep the eye within 1500 of the centre, raising it as it is pulled in */
		for (i = 0; i < 10; i++) {
			r = CAM_F32(0x44) * CAM_F32(0x44) + CAM_F32(0x3c) * CAM_F32(0x3c);
			if (r <= 2250000.0f)
				break;
			r = sqrtf(r);
			CAM_F32(0x40) = (r - 1500.0f) * 0.5f + CAM_F32(0x40);
			inv = 1.0f / r;
			CAM_F32(0x3c) = inv * CAM_F32(0x3c) * 1500.0f;
			CAM_F32(0x44) = inv * CAM_F32(0x44) * 1500.0f;
			dz = CAM_F32(0x44) - CAM_F32(0x24);
			dx = CAM_F32(0x3c) - CAM_F32(0x1c);
			dy = CAM_F32(0x40) - CAM_F32(0x20);
			CAM_F32(0x9c) = -atan2f(dy, sqrtf(dx * dx + dz * dz));
			ra_Frame(self, CAM_F32(0x9c), 200.0f, margin);
		}
		break;
	case 3:
		FUN_001ae190(self);
		CAM_F32(0xa0) = dt * 0.0010471975f + CAM_F32(0xa0);
		if (CAM_F32(0xa0) > 3.1415925f) {
			a = CAM_F32(0xa0);
			do
				a -= 6.28318548f;
			while (a > 3.1415925f);
			CAM_F32(0xa0) = a;
		}
		CAM_F32(0xa4) = sinf(CAM_F32(0xa0)) * 0.100000001f + 3.1415925f;
		t = CAM_F32(0x70) * -0.000500000024f;
		if (t < 0.0f)
			t = 0.0f;
		else if (t > 1.0f)
			t = 1.0f;
		CAM_F32(0x9c) = -0.282743335f - t * 0.345575184f;
		FUN_001ae6b0(self, CAM_F32(0x9c), CAM_F32(0xa4) + 3.1415925f, 250.0f, 100.0f);
		break;
	case 4:
		idx = FUN_001ae310(self);
		if (CAM_F32(0x58) >= 120.0f) {
			human = 0;
			for (i = 0; i < 4; i++) {
				if (FUN_001a7bb0(g_RowPlayer(i), NULL) >= 0 && DAT_002baed4[i * 10 + 1] != 0) {
					human = 1;
					if (FUN_001a76e0(g_RowPlayer(i)) != 0)
						CAM_I32(0x4) = 1;
				}
			}
			if (!human && FUN_001af9b0() == 1 && dt > 0.0f)
				CAM_I32(0x4) = 1;
		}
		if (idx == -1 && CLSamples_CommentaryIdle() != 0) {
			CAM_I32(0x4) = 1;
		} else {
			if (idx != CAM_I32(0xb8)) {
				if (CAM_I32(0xb8) != -1)
					FUN_001a8940(g_RowPlayer(CAM_I32(0xb8)), 6);
				if (idx != -1)
					FUN_001a8940(g_RowPlayer(idx), 5);
			}
			if (idx != -1)
				CAM_I32(0xb8) = idx;
		}
		if (CAM_I32(0x4) != 0) {
			CAM_F32(0x5c) = dt * 0.0333333351f + CAM_F32(0x5c);
			if (CAM_F32(0x5c) >= 1.0f) {
				CAM_F32(0x5c) = 1.0f;
				if (++CAM_I32(0x60) > 3)
					g_RowGameFlags |= 2;
			}
		}
		FUN_001aea40(self, dt);
		CAM_F32(0x54) = dt;
		CAM_F32(0x58) = dt + CAM_F32(0x58);
		FUN_001ae290(self);
		return;
	case 5:
		FUN_001a7e10(g_RowPlayer(0), CAM_FP(0x1c));
		FUN_001a7e10(g_RowPlayer(0), CAM_FP(0x3c));
		CAM_F32(0x40) = CAM_F32(0x40) + 600.0f;
		CAM_F32(0x44) = CAM_F32(0x44) - 0.00999999978f;
		break;
	case 6:
		/* the single human's view from the driver's head */
		drv = RA_PTRF(g_RowPlayer(g_SingleHumanIndex), 0xc4);
		node = baFindNode((struct BActorNode *)RA_PTRF(drv, 0x134), "headroot");
		baGetNodePosition((struct BActorInstance *)drv, (struct BActorNode *)node, CAM_FP(0x3c));
		h = RA_F32(g_RowPlayer(g_SingleHumanIndex), 0x2c8);
		ra_Copy4(CAM_FP(0x1c), CAM_FP(0x3c));
		CAM_F32(0x1c) = sinf(h) * 100.0f + CAM_F32(0x1c);
		CAM_F32(0x24) = CAM_F32(0x24) - cosf(h) * 100.0f;
		ra_Copy4(CAM_FP(0xc), CAM_FP(0x1c));
		ra_Copy4(CAM_FP(0x2c), CAM_FP(0x3c));
		FUN_001ae290(self);
		p = g_RowPlayer(g_SingleHumanIndex);
		if (RA_U8(p, 0x34a) == 0 && RA_U8(p, 0x349) == 0 && DAT_00283ef0 != 0)
			return;
		CAM_U32(0x50) = ((g_RowGameFlags >> 2) & 3) | (CAM_U32(0x50) & 0x80000000u);
		if (DAT_00283ef0 != 0)
			return;
		g_SingleHuman = 0;
		return;
	}

	if (CAM_I32(0x64) == 0) {
		ra_Copy4(CAM_FP(0xc), CAM_FP(0x1c));
		ra_Copy4(CAM_FP(0x2c), CAM_FP(0x3c));
		CAM_I32(0x64) = 1;
	}
	t = dt * 0.0500000007f;
	bmVectorLerp(CAM_FP(0xc), CAM_FP(0xc), CAM_FP(0x1c), t);
	bmVectorLerp(CAM_FP(0x2c), CAM_FP(0x2c), CAM_FP(0x3c), t);
	bdSetFieldOfView(CAM_F32(0x4c), CAM_F32(0x4c) * 3.0f * 0.25f);
	bdSetNearFar(40.0f, 80000.0f);
	bdSetCameraLookAt(CAM_FP(0x2c), CAM_FP(0xc), 0.0f);
	bdUpdateView();
}
