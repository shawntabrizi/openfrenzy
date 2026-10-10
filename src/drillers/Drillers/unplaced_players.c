/*
Drillers/unplaced_players.c  <-  C:\BlitzParty\Game\Src\Drillers\ (file not known)

The players object (one static object at 0x00298e70, vtable 0x0026c2ec; layout in
drillers.h): the four drillers, their trails (the snare line of TailBlazer and Quantum
Snare) and the snare loops a closed trail leaves for a moment.

These gap functions lie between CDrillers_Effects.cpp (0x00114230) and CDrillers_Lom.cpp
(0x0011b470); no assert names their file. 0x00116f80, 0x00117200, 0x00117460, 0x00117d00,
0x00118400, 0x001195c0, 0x00119cd0 and 0x0011add0 are not in of_functions.tsv.
*/

#include "drillers.h"

#define OBJ ((char *)DR_ADDR(DR_PLAYERS_ADDR))

/* the map size: cell size, half the number of cells across z and x */
#define DR_CELL DR_VAR(int, 0x002ff9e0u)
#define DR_HALF_Z DR_VAR(int, 0x002ff9e4u)
#define DR_HALF_X DR_VAR(int, 0x002ff9e8u)
/* game +0x14 (frames in the state), +0x1c (frame counter), +0x40 (scores) */
#define DR_STATE_FRAMES DR_VAR(int, 0x002ffa04u)
#define DR_FRAME DR_VAR(int, 0x002ffa0cu)
#define DR_SCORE(i) DR_VAR(int, 0x002ffa30u + (i) * 4)
/* game +0x84: frozen; +0x8c: Quantum Snare start countdown */
#define DR_FROZEN DR_VAR(signed char, 0x002ffa74u)
#define DR_COUNTDOWN DR_VAR(int, 0x002ffa7cu)
/* the four player lights (resources), the effect light, the camera light */
#define DR_LIGHT(i) DR_VAR(struct BLight *, 0x002aa994u + (i) * 4)
#define DR_FX_LIGHT DR_VAR(struct BLight *, 0x002aa9acu)
#define DR_CAM_LIGHT DR_VAR(struct BLight *, 0x002aa9b0u)
/* samples (resources) */
#define DR_SAMPLE(addr) DR_VAR(struct BSample *, addr)
/* the electric texture of the snare line */
#define DR_TEX_ZAP DR_VAR(int, 0x002aa970u)
/* the frame of the electric texture (players +0xc654) */
#define DR_ZAP_FRAME DR_AT(OBJ, 0xc654, int)
/* the round totals of the HUD extras object (0x002aab2c + i * 0x10) */
#define DR_ROUND_TOTAL(i) DR_VAR(int, 0x002aab2cu + (i) * 0x10)
/* the winners shown (0x002aa898) */
#define DR_WINNERS DR_VAR(int, 0x002aa898u)
/* tuning values (float; 0 switches the drawing off) */
#define DR_TUNE_DRILLER DR_VAR(float, 0x002aae28u)
#define DR_TUNE_DRIVER DR_VAR(float, 0x002aae2cu)
/* the camera eye */
#define DR_CAM_EYE DR_PTR(float, 0x002aa834u)
/* the snare polygon of the AI object (pairs of x, z) */
#define DR_POLY DR_PTR(float, 0x002aae64u)
/* the actor of player i */
#define DR_ACTOR_BODY(i) DR_PF(i, 0x4c, char *)
#define DR_ACTOR_DRILL(i) DR_PF(i, 0x50, char *)
/* the animation counter of the drivers (0x00302018) */
#define DR_DRIVE_ANIM DR_VAR(int, 0x00302018u)

/* the effects object, the AI object */
#define DR_FX g_DrFx
#define DR_AI DR_PTR(void, 0x002aadf8u)

#define DR_DEG 0.017453292f
#define DR_BLACK (DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0] | DAT_002bcc94)

/* the colour bytes of the character of player i */
#define DR_CHAR_COLOUR(i, c) (DAT_00290718[DAT_002baed4[(i) * 10] * 0x60 + (c)])

/* 0x0026c2ec  vtable of the players object (rebuilt from the XBE) */
const void *const vtable_0026c2ec[1] = {
	(const void *)DrPlayers_DeletingDtor, /* +0x00 */
};

/* 0x00116970  Drillers (gap; file not known)
Puts player `player` (0-3) on its start cell next to the centre, facing outwards, with a
full trail at the start, 250 on the HUD bar, and the trail length of the variant
(32, 16 in TailBlazer, 4 in Quantum Snare); clears the snare loops and switches the lights
on (the other three only in variants 3 and 4). */
void DrPlayers_Reset(void *self, char player)
{
	float fx;
	float fz;
	float cell;
	float hx;
	float hz;
	int cx;
	int cz;
	int i = player;
	int k;

	(void)self;
	switch (i) {
	case 0:
		cx = DR_HALF_X + 1;
		cz = DR_HALF_Z - 1;
		DR_PF(0, 0xc, float) = 90.0f;
		break;
	case 1:
		cx = DR_HALF_X - 1;
		cz = DR_HALF_Z - 1;
		DR_PF(1, 0xc, float) = 180.0f;
		break;
	case 2:
		cx = DR_HALF_X - 1;
		cz = DR_HALF_Z + 1;
		DR_PF(2, 0xc, float) = 270.0f;
		break;
	case 3:
		cx = DR_HALF_X + 1;
		cz = DR_HALF_Z + 1;
		DR_PF(3, 0xc, float) = 0.0f;
		break;
	default:
		/* the original leaves the player number and the object pointer here */
		cx = i;
		cz = 0;
		break;
	}
	DR_F32(DAT_002ecfac, 0xa8 + i * 4) = 250.0f;
	fx = (float)cx * 1024.0f;
	DR_AT(OBJ, 0xc644 + i * 4, int) = 0;
	fx = fx + 512.0f;
	DR_PF(i, 0x724, int) = 0;
	DR_PF(i, 0x14, float) = DR_PF(i, 0xc, float);
	DR_PF(i, 0x18, float) = 128.0f;
	DR_PF(i, 0x1e, unsigned char) = 0xff;
	DR_PF(i, 0x4, float) = fx;
	DR_PF(i, 0x1f, unsigned char) = 0xff;
	DR_PF(i, 0x730, unsigned char) = 0;
	DR_PF(i, 0x731, unsigned char) = 0;
	DR_PF(i, 0x20, signed char) = 1;
	fz = (float)cz * 1024.0f;
	DR_PF(i, 0x5c, int) = 0;
	DR_PF(i, 0x6f0, int) = 10;
	fz = fz + 512.0f;
	DR_PF(i, 0x670, int) = 0;
	DR_PF(i, 0x6f4, int) = 0;
	DR_PF(i, 0x6f8, float) = 255.0f;
	DR_PF(i, 0x8, float) = fz;
	DR_PF(i, 0x6fc, int) = 0;
	DR_PF(i, 0x708, int) = 0;
	DR_PF(i, 0x70c, int) = 0;
	DR_PF(i, 0x710, int) = 0;
	DR_PF(i, 0x71c, int) = 0;
	DR_PF(i, 0x720, int) = 0;
	DR_PF(i, 0x72c, int) = 0;
	DR_PF(i, 0x60, int) = 0;
	DR_PF(i, 0x674, int) = 0;
	switch (g_DrVariant) {
	case 1:
	case 2:
		DR_PF(i, 0x64, short) = 0x20;
		break;
	case 3:
		DR_PF(i, 0x64, short) = 0x10;
		break;
	case 4:
		DR_PF(i, 0x64, short) = 4;
		break;
	}
	DR_PF(i, 0x66c, int) = 0;
	cell = (float)DR_CELL;
	fx = fx * 0.0009765625f;
	hx = (float)DR_HALF_X;
	fz = fz * 0.0009765625f;
	hz = (float)DR_HALF_Z;
	for (k = 0; k < 0x80; k++) {
		DR_TRAIL_Z(i, k) = (fz - hz) * cell;
		DR_TRAIL_X(i, k) = (fx - hx) * cell;
		DR_TRAIL_W(i, k) = 0;
	}
	for (k = 0; k < 0x10; k++)
		DR_LOOP(k, 0x6acc, int) = 0;
	baEnableLight(DR_LIGHT(0), 1);
	baEnableLight(DR_LIGHT(1), 2 < g_DrVariant);
	baEnableLight(DR_LIGHT(2), 2 < g_DrVariant);
	baEnableLight(DR_LIGHT(3), 2 < g_DrVariant);
}

/* 0x00116be0  Drillers (gap; file not known)
A closed trail of more than 7 points: a sound, and a snare loop (the first free one of 16)
of the trail's `count` points, closed at the first point, for 32 frames. */
void DrPlayers_StartLoop(void *self, int count, int player)
{
	int k;
	int j;

	(void)self;
	if (count <= 7)
		return;
	DR_PF(player, 0x674, int) = 0;
	bsPlaySample(DR_SAMPLE(0x002aa9c4u), 200, 0, 0, 0);
	for (k = 0; DR_LOOP(k, 0x6acc, int) != 0; ) {
		k++;
		if (15 < k)
			return;
	}
	for (j = 0; j < count; j++) {
		DR_LOOP_X(k, j) = DR_TRAIL_X(player, j);
		DR_LOOP_Z(k, j) = DR_TRAIL_Z(player, j);
	}
	DR_LOOP_X(k, count) = DR_TRAIL_X(player, 0);
	DR_LOOP_Z(k, count) = DR_TRAIL_Z(player, 0);
	DR_LOOP(k, 0x6acc, int) = 1;
	DR_LOOP(k, 0x6ad0, int) = 0x20;
	DR_LOOP(k, 0x6ad4, int) = count;
	DR_LOOP(k, 0x6ad8, int) = player;
}

/* 0x00116cf0  Drillers (gap; file not known)
The snare loops count down; while a loop is fresh (more than 8 frames left) every other
player (not of the loop's team) whose trail head is inside it is snared: unless already
hit, a "5" for the snarer, five tokens fly from the victim, a burst, a scorch mark and a
sound; the hit timer is set (64). */
void DrPlayers_UpdateLoops(void *self)
{
	float pos[4];
	float fx;
	float fz;
	int *timer;
	int owner;
	int n;
	int k;
	int j;
	int t;

	(void)self;
	for (k = 0; k < 0x10; k++) {
		if (DR_LOOP(k, 0x6acc, int) == 0)
			continue;
		t = DR_LOOP(k, 0x6ad0, int) - 1;
		DR_LOOP(k, 0x6ad0, int) = t;
		if (t < 1) {
			DR_LOOP(k, 0x6acc, int) = 0;
			continue;
		}
		if (t <= 8)
			continue;
		n = DR_LOOP(k, 0x6ad4, int);
		for (j = 0; j < n; j++) {
			DR_POLY[j * 2] = DR_LOOP_X(k, j);
			DR_POLY[j * 2 + 1] = DR_LOOP_Z(k, j);
		}
		DR_POLY[n * 2] = DR_LOOP_X(k, 0);
		DR_POLY[n * 2 + 1] = DR_LOOP_Z(k, 0);
		owner = DR_LOOP(k, 0x6ad8, int);
		for (j = 0; j < 4; j++) {
			if (j == owner)
				continue;
			if (DAT_002baebc != 0 && DAT_002baef4[owner * 10] == DAT_002baef4[j * 10])
				continue;
			if (DrAi_InPolygon(DR_AI, DR_TRAIL_X(j, 0), DR_TRAIL_Z(j, 0),
					DR_LOOP(k, 0x6ad4, int) + 1, DR_POLY) == 0)
				continue;
			timer = &DR_PF(j, 0x6f4, int);
			if (0 < *timer) {
				*timer = 0x40;
				continue;
			}
			if (*timer < 0) {
				*timer = -0x40;
				continue;
			}
			*timer = 0x40;
			DrFx_AddScoreFloaty(DR_TRAIL_X(j, 0), DR_TRAIL_Z(j, 0), 2, (char)owner);
			DR_PF(j, 0x6f8, float) = 255.0f;
			fx = DR_PF(j, 0x4, float) * 0.0009765625f - (float)DR_HALF_X;
			fz = DR_PF(j, 0x8, float) * 0.0009765625f - (float)DR_HALF_Z;
			for (t = 0; t < 5; t++)
				DrEffects_AddTokenAt(DR_FX, fx, fz);
			pos[0] = fx * (float)DR_CELL;
			pos[1] = (float)DR_CELL;
			pos[2] = (float)DR_CELL * fz;
			/* the original leaves a stack word (the object pointer) in w */
			pos[3] = 1.0f;
			DrFxGroup_Start(&DR_FX->groups[j], pos, owner);
			baAddExtraItem(DR_FX->scorch, (double)pos[0], 15.0, (double)pos[2]);
			bsPlaySample(DR_SAMPLE(0x002aa9c8u), 200, 0, 0, 0);
		}
	}
}

/* 0x00116f80  Drillers (not in of_functions.tsv)
A trail marker at x, z: an arrow head (a fan of 5 vertices) at height y + 10 in the
colour r, g, b (the wings a quarter as bright), wobbling by angles derived from x and
z. */
void FUN_00116f80(int player, float x, float z, int r, int g, int b, int y)
{
	bdVertex *v = DR_PTR(bdVertex, 0x00301d20u);
	float trans[16];
	float rot[16];
	float m[16];
	float a1;
	float a2;
	float a3;
	unsigned int wing;

	(void)player;
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
	bmMatrixTranslation(trans, x, 0.0f, z);
	a1 = ((float)(DR_FTOL(x * 234.5f) % 0x5a) - 45.0f) * DR_DEG;
	a2 = ((float)(DR_FTOL(x * z) % 0x5a) - 45.0f) * DR_DEG;
	a3 = ((float)(DR_FTOL(x * 123.4f) % 0x5a) - 45.0f) * DR_DEG;
	bmMatrixRotationXYZ(rot, a3, a2, a1);
	bmMatrixMultiplyCopy(m, trans, rot);
	bdSetWorldMatrix(m);
	v[0].y = (float)y + 10.0f;
	v[0].color = DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bcc94;
	wing = DAT_002bce18[r / 4] | DAT_002bc490[g / 4] | DAT_002bd640[b / 4] | DAT_002bcc94;
	v[0].x = 0.0f;
	v[0].z = 0.0f;
	v[1].x = 50.0f;
	v[1].y = 0.0f;
	v[1].z = 20.0f;
	v[2].x = -50.0f;
	v[2].y = 0.0f;
	v[2].z = 20.0f;
	v[3].x = 0.0f;
	v[3].y = 0.0f;
	v[3].z = -60.0f;
	v[4].x = 50.0f;
	v[4].y = 0.0f;
	v[4].z = 20.0f;
	v[1].color = wing;
	v[2].color = wing;
	v[3].color = wing;
	v[4].color = wing;
	bdDrawPrimitiveFVF(5, v, 5, 0x152);
	bdSetWorldIdentity();
}

/* 0x00117200  Drillers (not in of_functions.tsv)
A player out of the game: driver and driller tumble down (turning faster, sinking 48 per
step of the counter +0x20, which wraps at 64), the light follows (variants 3, 4). */
void FUN_00117200(void *self, char player)
{
	float m[16];
	float x;
	float z;
	char *body;
	char *drill;
	signed char c;
	int i = player;

	(void)self;
	x = (DR_PF(i, 0x4, float) * 0.0009765625f - (float)DR_HALF_X) * (float)DR_CELL;
	z = (DR_PF(i, 0x8, float) * 0.0009765625f - (float)DR_HALF_Z) * (float)DR_CELL;
	DR_PF(i, 0xc, float) = DR_PF(i, 0xc, float) + 12.0f;
	bdSetWorldIdentity();
	c = DR_PF(i, 0x20, signed char);
	bmMatrixRotationXZY(m, (float)(c * 8) * DR_DEG, (DR_PF(i, 0xc, float) + 180.0f) * DR_DEG,
		(float)((int)c << 4) * DR_DEG);
	body = DR_ACTOR_BODY(i);
	drill = DR_ACTOR_DRILL(i);
	bmMatrixToQuaternion((float *)(body + 0xe0), m);
	bmMatrixToQuaternion((float *)(drill + 0xe0), m);
	DR_F32(body, 0xc0) = x;
	DR_F32(body, 0xc4) = (float)(c * -0x30);
	DR_F32(body, 0xc8) = z;
	baSetNodeTime((struct BActorInstance *)body, DR_AT(body, 0x134, struct BActorNode *), 0, 1);
	baPrepActorInstance((struct BActorInstance *)body, 1);
	baDrawActorInstance((struct BActorInstance *)body, 7);
	DR_F32(drill, 0xc0) = x;
	DR_F32(drill, 0xc4) = (float)(c * -0x30);
	DR_F32(drill, 0xc8) = z;
	if (2 < g_DrVariant) {
		DR_F32(drill, 0xc4) = DR_F32(drill, 0xc4) + 22.0f;
		if (2 < g_DrVariant)
			baSetPointLight(DR_LIGHT(i), DR_FP(drill, 0xc0), 0.0019f);
	}
	DR_F32(drill, 0xd0) = 1.0f;
	DR_F32(drill, 0xd4) = 1.0f;
	DR_F32(drill, 0xd8) = 1.0f;
	if (DR_FROZEN == 0)
		baUpdateAnimations((struct BActorInstance *)drill, DR_AT(drill, 0x134, struct BActorNode *),
			1.0f, NULL);
	baPrepActorInstance((struct BActorInstance *)drill, 1);
	baDrawActorInstance((struct BActorInstance *)drill, 7);
	bdSetWorldIdentity();
	bdSetTexture(1, NULL);
	c = (signed char)(DR_PF(i, 0x20, signed char) + 1);
	DR_PF(i, 0x20, signed char) = c;
	if ('?' < c)
		DR_PF(i, 0x20, signed char) = 0;
}

/* 0x00117440  Drillers (gap; file not known; vtable 0x0026c2ec +0x00) */
void *DrPlayers_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026c2ec;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* the offset 16 to the side of segment (x0, z0) -> (x1, z1) */
static void dr_side(float *out, float x0, float z0, float x1, float z1)
{
	float m[16];

	out[0] = 0.0f;
	out[1] = 0.0f;
	out[2] = 16.0f;
	out[3] = 1.0f;
	bmMatrixRotationY(m, atan2f(x1 - x0, z1 - z0) + 1.5707964f);
	bmTransformVector4InPlace(m, out);
}

/* a fence segment: a lit rail (6 vertices) and an electric band (4 vertices, the zap
   texture) from point 0 to point 1; `bright` the rail's colour, `band0`, `band1` the band's
   colours at the two ends (the band's top at fy1 + top) */
static void dr_fence_segment(float x0, float z0, float x1, float z1, float x2, float z2,
	float fy1, float fy0, float top, unsigned int rail0, unsigned int rail1, unsigned int band0,
	unsigned int band1, unsigned int band1low)
{
	bdVertex *v = DR_PLAYERS_VTX;
	float a[4];
	float b[4];

	dr_side(a, x0, z0, x1, z1);
	dr_side(b, x1, z1, x2, z2);
	v[0].y = fy1;
	v[0].x = x0 - a[0];
	v[0].z = z0 - a[2];
	v[0].color = rail0;
	v[1].y = fy1;
	v[1].x = x1 - b[0];
	v[1].z = z1 - b[2];
	v[1].color = rail1;
	v[2].x = x0;
	v[2].y = fy0;
	v[2].z = z0;
	v[2].color = rail0;
	v[3].y = fy0;
	v[3].x = x1;
	v[3].z = z1;
	v[3].color = rail1;
	v[4].y = fy1;
	v[4].x = a[0] + x0;
	v[4].z = a[2] + z0;
	v[4].color = rail0;
	v[5].y = fy1;
	v[5].x = b[0] + x1;
	v[5].z = b[2] + z1;
	v[5].color = rail1;
	bdSetTexture(0, NULL);
	bdDrawPrimitiveFVF(4, v, 6, 0x152);
	v[0].x = x0;
	v[0].y = top + fy1;
	v[0].z = z0;
	v[0].color = band0;
	v[0].u = 0.0f;
	v[0].v = 0.0f;
	v[1].y = top + fy1;
	v[1].x = x1;
	v[1].z = z1;
	v[1].color = band1;
	v[1].v = 0.0f;
	v[1].u = 1.0f;
	v[2].x = x0;
	v[2].y = fy0;
	v[2].z = z0;
	v[2].color = band0;
	v[2].u = 0.0f;
	v[2].v = 1.0f;
	v[3].x = x1;
	v[3].y = fy0;
	v[3].z = z1;
	v[3].color = band1low;
	v[3].u = 1.0f;
	v[3].v = 1.0f;
	bdSetTexture(0, (struct bdTexture *)(uintptr_t)DR_TEX_ZAP);
	bdTextureSetFrame((struct bdTexture *)(uintptr_t)DR_TEX_ZAP, DR_ZAP_FRAME / 2);
	bdDrawPrimitiveFVF(4, v, 4, 0x152);
}

/* a glow strip on the ground along points 0..n of a line (two passes, one per side, 150
   wide, from `colour` at the line to black); `xs`, `zs` the line, `first` the index of the
   first point, which gets the first pair of the strip */
static void dr_glow_strip(const float *xs, const float *zs, int first, int last,
	unsigned int colour)
{
	bdVertex *v;
	float angle;
	float s;
	float c;
	int count;
	int pass;
	int j;

	for (pass = 0; pass < 2; pass++) {
		v = DR_PLAYERS_VTX;
		count = 0;
		for (j = first; j < last; j++) {
			angle = ((float)pass * 180.0f + 90.0f) * DR_DEG +
				atan2f(xs[j + 1] - xs[j], zs[j + 1] - zs[j]);
			s = sinf(angle);
			c = cosf(angle);
			if (j == 0) {
				v[0].x = xs[0];
				v[0].y = 30.0f;
				v[0].z = zs[0];
				v[1].y = 30.0f;
				v[1].x = s * 150.0f + xs[0];
				v[1].z = c * 150.0f + zs[0];
				v[0].color = colour;
				v[1].color = DR_BLACK;
				count += 2;
				v += 2;
			}
			v[0].x = xs[j + 1];
			v[0].y = 30.0f;
			v[0].z = zs[j + 1];
			v[1].y = 30.0f;
			v[1].x = s * 150.0f + xs[j + 1];
			v[1].z = c * 150.0f + zs[j + 1];
			v[0].color = colour;
			v[1].color = DR_BLACK;
			count += 2;
			v += 2;
		}
		bdSetTexture(0, NULL);
		bdDrawPrimitiveFVF(4, DR_PLAYERS_VTX, count - 2, 0x152);
	}
}

/* 0x00117460  Drillers (not in of_functions.tsv)
Draws the snare loops: an electric fence along each loop in the snarer's colour (fading in
the last 8 frames) and a glow on the ground on both sides. */
void FUN_00117460(void *self)
{
	unsigned int rail;
	unsigned int band;
	float fy1;
	float fy0;
	int owner;
	int r;
	int g;
	int b;
	int d;
	int k;
	int j;
	int n;

	(void)self;
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	for (k = 0; k < 0x10; k++) {
		if (DR_LOOP(k, 0x6acc, int) == 0)
			continue;
		owner = DR_LOOP(k, 0x6ad8, int);
		r = DR_CHAR_COLOUR(owner, 0) * 2;
		g = DR_CHAR_COLOUR(owner, 1) * 2;
		b = DR_CHAR_COLOUR(owner, 2) * 2;
		if (DR_LOOP(k, 0x6ad0, int) < 8) {
			d = (DR_LOOP(k, 0x6ad0, int) - 8) * 0x20;
			r += d;
			g += d;
			b += d;
			if (r < 0)
				r = 0;
			if (g < 0)
				g = 0;
			if (b < 0)
				b = 0;
		}
		bdSetTexture(0, NULL);
		bdSetWorldIdentity();
		n = DR_LOOP(k, 0x6ad4, int);
		if (0 < n) {
			fy0 = (float)owner * 3.0f;
			fy1 = 32.0f + fy0;
			fy0 = fy0 + 44.0f;
			rail = DAT_002bd640[b / 3] | DAT_002bc490[g / 3] | DAT_002bce18[r / 3] | DAT_002bcc94;
			band = DAT_002bd640[b / 2] | DAT_002bc490[g / 2] | DAT_002bce18[r / 2] | DAT_002bcc94;
			for (j = 0; j < n; j++) {
				float *xs = &DR_LOOP_X(k, 0);
				float *zs = &DR_LOOP_Z(k, 0);

				dr_fence_segment(xs[j], zs[j], xs[j + 1], zs[j + 1], xs[j + 2], zs[j + 2], fy1,
					fy0, 172.0f, rail, rail, band, band, band);
			}
		}
		dr_glow_strip(&DR_LOOP_X(k, 0), &DR_LOOP_Z(k, 0), 0, n,
			DAT_002bd640[b / 2] | DAT_002bc490[g / 2] | DAT_002bce18[r / 2] | DAT_002bcc94);
	}
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* a half disc of 7 rim vertices (radius 150, 30 high) around trail point `at`, facing away
   from point `from`, from angle `base`; the fan is drawn when `draw` */
static void dr_trail_cap(bdVertex *v, float x, float z, float xFrom, float zFrom, float base,
	unsigned int centre, int draw)
{
	float heading = atan2f(x - xFrom, z - zFrom);
	float a;
	int k;

	v[0].color = centre;
	v[0].x = x;
	v[0].y = 30.0f;
	v[0].z = z;
	for (k = 0; k < 7; k++) {
		a = ((float)k * 30.0f + base) * DR_DEG + heading;
		v[k + 1].y = 30.0f;
		v[k + 1].x = sinf(a) * 150.0f + x;
		v[k + 1].color = DR_BLACK;
		v[k + 1].z = cosf(a) * 150.0f + z;
	}
	if (draw)
		bdDrawPrimitiveFVF(5, v, 8, 0x152);
}

/* 0x00117d00  Drillers (not in of_functions.tsv)
TailBlazer: the trail of player `player` while it shows (fade 16 or more): a flickering
colour, a glowing cap at both ends, the markers fading in along the first 5/6 and out
along the rest (4 per segment), and the ground glow on both sides. */
void FUN_00117d00(void *self, int player)
{
	bdVertex *cap = DR_PTR(bdVertex, 0x00301dd8u);
	float *xs = &DR_TRAIL_X(player, 0);
	float *zs = &DR_TRAIL_Z(player, 0);
	unsigned int colour;
	float dx;
	float dz;
	int flicker;
	int len;
	int r;
	int g;
	int b;
	int j;
	int s;
	int pass;
	int count;
	bdVertex *v;
	float angle;
	float sn;
	float cs;

	(void)self;
	if (DR_PF(player, 0x6f8, float) < 16.0f)
		return;
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
	flicker = DR_FTOL(DR_PF(player, 0x6f8, float));
	r = flicker - 0xe7 + DR_CHAR_COLOUR(player, 0) * 2;
	b = flicker - 0xe7 + DR_CHAR_COLOUR(player, 2) * 2;
	g = flicker - 0xe7 + DR_CHAR_COLOUR(player, 1) * 2;
	if (r < 0)
		r = 0;
	if (g < 0)
		g = 0;
	if (b < 0)
		b = 0;
	if (0xff < r)
		r = 0xff;
	if (0xff < g)
		g = 0xff;
	if (0xff < b)
		b = 0xff;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	len = DR_PF(player, 0x64, short);
	colour = DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bcc94 | DAT_002bce18[r];
	dr_trail_cap(cap, xs[len - 3], zs[len - 3], xs[len - 4], zs[len - 4], 270.0f, colour, 1);
	dr_trail_cap(cap, xs[2], zs[2], xs[1], zs[1], 90.0f, colour, 1);
	for (pass = 0; pass < 2; pass++) {
		len = DR_PF(player, 0x64, short);
		count = 0;
		v = DR_PLAYERS_VTX;
		for (j = 1; j < len - 2; j++) {
			if (j < (len * 5) / 6) {
				if (DR_TRAIL_W(player, j) < 0x78)
					DR_TRAIL_W(player, j) += 8;
			} else if (0 < DR_TRAIL_W(player, j)) {
				DR_TRAIL_W(player, j) -= 8;
			}
			if (pass == 0 && j != DR_PF(player, 0x64, short) - 3) {
				dx = (xs[j + 1] - xs[j]) * 0.25f;
				dz = (zs[j + 1] - zs[j]) * 0.25f;
				for (s = 0; s < 4; s++)
					FUN_00116f80(player, (float)s * dx + xs[j], dz * (float)s + zs[j], r, g, b,
						DR_TRAIL_W(player, j));
			}
			angle = atan2f(xs[j + 1] - xs[j], zs[j + 1] - zs[j]) +
				((float)pass * 180.0f + 90.0f) * DR_DEG;
			sn = sinf(angle);
			cs = cosf(angle);
			/* (the original also has a first pair for j == 0, which never runs) */
			v[0].x = xs[j + 1];
			v[0].y = 30.0f;
			v[0].z = zs[j + 1];
			v[1].y = 30.0f;
			v[1].x = sn * 150.0f + xs[j + 1];
			v[1].z = cs * 150.0f + zs[j + 1];
			v[0].color = DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bcc94 | DAT_002bce18[r];
			v[1].color = DR_BLACK;
			count += 2;
			v += 2;
			len = DR_PF(player, 0x64, short);
		}
		bdDrawPrimitiveFVF(4, DR_PLAYERS_VTX, count - 2, 0x152);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
}

/* 0x00118400  Drillers (not in of_functions.tsv)
Quantum Snare: the snare line of player `player`: an electric fence along the trail (the
end segment fading to black; white while the player is hit and blinking), a glowing cap
at the head (a cap at the tail is built and not drawn), and the ground glow. The colour
bytes are the character colours doubled in 8 bits (they wrap as in the original). */
void FUN_00118400(void *self, int player)
{
	bdVertex *cap = DR_PTR(bdVertex, 0x00301ef8u);
	float *xs = &DR_TRAIL_X(player, 0);
	float *zs = &DR_TRAIL_Z(player, 0);
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned int rail;
	unsigned int band;
	unsigned int black;
	unsigned int centre;
	float fy1;
	float fy0;
	float top;
	int hit;
	int len;
	int j;
	int end;

	(void)self;
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	r = (unsigned char)(DR_CHAR_COLOUR(player, 0) << 1);
	g = (unsigned char)(DR_CHAR_COLOUR(player, 1) << 1);
	b = (unsigned char)(DR_CHAR_COLOUR(player, 2) << 1);
	bdSetWorldIdentity();
	hit = DR_PF(player, 0x6f4, int);
	if (hit < 0 && ((unsigned int)hit & 2) != 0 && g_DrState == 5) {
		b = 200;
		g = 200;
		r = 200;
	}
	len = DR_PF(player, 0x64, short);
	black = DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
	if (len != 3 && -1 < len - 3) {
		fy1 = 32.0f + (float)player * 3.0f;
		fy0 = (float)player * 3.0f + 44.0f;
		rail = DAT_002bce18[r / 3] | DAT_002bd640[b / 3] | DAT_002bc490[g / 3];
		band = DAT_002bd640[b >> 1] | DAT_002bc490[g >> 1] | DAT_002bce18[r >> 1];
		for (j = 0; j < DR_PF(player, 0x64, short) - 3; j++) {
			end = j == DR_PF(player, 0x64, short) - 4;
			top = 172.0f;
			if (j == DR_PF(player, 0x64, short) - 3)
				top = 32.0f;
			dr_fence_segment(xs[j], zs[j], xs[j + 1], zs[j + 1], xs[j + 2], zs[j + 2], fy1, fy0,
				top, rail | DAT_002bcc94, (end ? black : rail) | DAT_002bcc94,
				band | DAT_002bcc94, (end ? black : band) | DAT_002bcc94,
				end ? (black | DAT_002bcc94) : (band | DAT_002bca94));
		}
	}
	bdSetTexture(0, NULL);
	len = DR_PF(player, 0x64, short);
	centre = DAT_002bd640[b >> 1] | DAT_002bc490[g >> 1] | DAT_002bce18[r >> 1] | DAT_002bcc94;
	dr_trail_cap(cap, xs[len - 3], zs[len - 3], xs[len - 4], zs[len - 4], 270.0f, centre, 1);
	dr_trail_cap(cap, xs[1], zs[1], xs[0], zs[0], 90.0f, centre, 0);
	dr_glow_strip(xs, zs, 0, len - 2, centre);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* 0x00118f00  Drillers (gap; file not known)
Game over: player `player` celebrates if it won (the best round total in variants 1 and
3, the best score or the winning team otherwise; the winners are counted in 0x002aa898):
it bobs and spins (in variants 1 and 2 with a green ring at the bottom of each bob and 8
worms around it), lit brighter. */
void DrPlayers_DrawWinner(void *self, char player)
{
	float m1[16];
	float m2[16];
	float lightPos[3];
	unsigned int team[4];
	float x;
	float z;
	float bob;
	float off;
	char *body;
	char *drill;
	int variant = g_DrVariant;
	int best = 0;
	int mine;
	int winner;
	int i;
	int k;

	(void)self;
	team[0] = 0;
	team[1] = 0;
	if (player == 0)
		DR_WINNERS = 0;
	i = player;
	if (g_DrVariant == 1 || g_DrVariant == 3) {
		if (DR_PF(i, 0x20, signed char) == 1) {
			body = DR_ACTOR_BODY(i);
			x = DR_F32(body, 0xc0);
			z = DR_F32(body, 0xc8);
		} else {
			z = 0.0f;
			x = (float)((i * 5 - 10) * 0x40) - 80.0f;
		}
		if (0 < DR_ROUND_TOTAL(0))
			best = DR_ROUND_TOTAL(0);
		if (best < DR_ROUND_TOTAL(1))
			best = DR_ROUND_TOTAL(1);
		if (best < DR_ROUND_TOTAL(2))
			best = DR_ROUND_TOTAL(2);
		if (best < DR_ROUND_TOTAL(3))
			best = DR_ROUND_TOTAL(3);
		mine = DR_ROUND_TOTAL(i);
	} else {
		if (DR_PF(i, 0x20, signed char) == 1) {
			body = DR_ACTOR_BODY(i);
			x = DR_F32(body, 0xc0);
			z = DR_F32(body, 0xc8);
		} else {
			z = 0.0f;
			x = (float)((2 - i) * 0x140) + 80.0f;
		}
		if (DAT_002baebc != 0) {
			team[DAT_002baef4[0]] += DR_SCORE(0);
			team[DAT_002baf1c] += DR_SCORE(1);
			team[DAT_002baf44] += DR_SCORE(2);
			team[DAT_002baf6c] += DR_SCORE(3);
			if ((int)team[1] < (int)team[0])
				winner = 0;
			else
				winner = ((int)team[1] <= (int)team[0]) + 1;
			if (DAT_002baef4[i * 10] != winner && winner != 2)
				return;
			goto celebrate;
		}
		if (0 < DR_SCORE(0))
			best = DR_SCORE(0);
		if (best < DR_SCORE(1))
			best = DR_SCORE(1);
		if (best < DR_SCORE(2))
			best = DR_SCORE(2);
		if (best < DR_SCORE(3))
			best = DR_SCORE(3);
		mine = DR_SCORE(i);
	}
	if (mine != best)
		return;
	DR_WINNERS++;
celebrate:
	DR_PF(i, 0x72c, int) = 1;
	bob = fabsf(sinf((float)(DR_STATE_FRAMES * 8) * DR_DEG) * 150.0f);
	DR_PF(i, 0x5c, float) = bob;
	if (variant < 3) {
		if (bob <= 11.0f)
			baAddExtraItem(DR_FX->ring, (double)x, 0.0, (double)z, 0x10, 0x7f, 0x10, 0.0);
		if (g_DrVariant < 3 && DR_PF(i, 0x5c, float) < 16.0f) {
			for (k = 0; k < 8; k++) {
				(void)dr_rand();
				off = ((float)(dr_rand() & 3) - 2.0f) * (float)DR_CELL;
				baAddExtraItem(DR_FX->worm, 2, (double)(off + x), 0.0, (double)(z + off));
			}
		}
	}
	DR_PF(i, 0xc, float) = (float)(DR_STATE_FRAMES * 6);
	bdSetWorldIdentity();
	if (g_DrVariant < 3) {
		bmMatrixRotationY(m1, (DR_PF(i, 0xc, float) + 180.0f) * DR_DEG);
		bmMatrixRotationY(m2, (DR_PF(i, 0xc, float) + 180.0f) * DR_DEG);
	} else {
		DR_PF(i, 0x710, float) = 0.0f;
		bmMatrixRotationYZX(m1, 0.0f, DR_PF(i, 0xc, float) * DR_DEG, -0.0f);
		bmMatrixRotationYZX(m2, 0.0f, (DR_PF(i, 0xc, float) + 180.0f) * DR_DEG,
			DR_PF(i, 0x710, float) * -1.5f * DR_DEG);
	}
	body = DR_ACTOR_BODY(i);
	drill = DR_ACTOR_DRILL(i);
	bmMatrixToQuaternion((float *)(body + 0xe0), m1);
	bmMatrixToQuaternion((float *)(drill + 0xe0), m2);
	DR_F32(body, 0xc0) = x;
	DR_F32(body, 0xc4) = DR_PF(i, 0x5c, float);
	DR_F32(body, 0xc8) = z;
	baSetNodeTime((struct BActorInstance *)body, DR_AT(body, 0x134, struct BActorNode *), 0, 1);
	baPrepActorInstance((struct BActorInstance *)body, 1);
	DR_F32(drill, 0xd0) = 0.8f;
	DR_F32(drill, 0xd4) = 0.8f;
	DR_F32(drill, 0xd8) = 0.8f;
	DR_F32(drill, 0xc0) = DR_F32(body, 0xc0);
	DR_F32(drill, 0xc4) = DR_F32(body, 0xc4);
	DR_F32(drill, 0xc8) = DR_F32(body, 0xc8);
	DR_F32(drill, 0xcc) = DR_F32(body, 0xcc);
	if (2 < g_DrVariant)
		DR_F32(drill, 0xc4) = DR_F32(drill, 0xc4) + 22.0f;
	if (DR_FROZEN == 0)
		baUpdateAnimations((struct BActorInstance *)drill, DR_AT(drill, 0x134, struct BActorNode *),
			1.0f, NULL);
	baPrepActorInstance((struct BActorInstance *)drill, 1);
	DR_F32(drill, 0xc0) = DR_F32(body, 0xc0);
	DR_F32(drill, 0xc4) = DR_F32(body, 0xc4);
	DR_F32(drill, 0xc8) = DR_F32(body, 0xc8);
	DR_F32(drill, 0xcc) = DR_F32(body, 0xcc);
	lightPos[0] = DR_F32(drill, 0xc0);
	lightPos[1] = (float)DR_CELL + DR_F32(drill, 0xc4) + 300.0f;
	lightPos[2] = DR_F32(drill, 0xc8);
	if (2 < g_DrVariant)
		baSetPointLight(DR_LIGHT(i), lightPos,
			(g_DrState == 3 || g_DrState == 6) ? 0.0025f : 0.0019f);
	bdSetRenderState(3, 1, 0);
	if (g_DrVariant == 1)
		baSetAmbientLight(0x40, 0x40, 0x40);
	else
		baSetAmbientLight(0x7f, 0x7f, 0x7f);
	if (DR_TUNE_DRILLER != 0.0f) {
		if (g_DrVariant == 1)
			baSetLightColour(DR_LIGHT(0), 0x7f, 0x7f, 0x7f);
		baDrawActorInstance((struct BActorInstance *)drill, 7);
		if (g_DrVariant == 1)
			baSetLightColour(DR_LIGHT(0), 0x40, 0x40, 0x40);
	}
	if (DR_TUNE_DRIVER != 0.0f)
		baDrawActorInstance((struct BActorInstance *)body, 7);
	bdSetRenderState(3, 0, 0);
	baSetAmbientLight(0x40, 0x40, 0x40);
	bdSetTexture(1, NULL);
	bdSetWorldIdentity();
}

/* 0x001195c0  Drillers (not in of_functions.tsv)
Draws player `player` in play: the driver (its drive animation from the shared counter,
which player 0 advances, 0-60) and the driller turned to the heading (variants 1, 2: the
driller's own heading turns towards it by 30 a frame while playing; variants 3, 4: tilted
by +0x710), the lights, the countdown flash of Quantum Snare, a shadow, and the trail of
TailBlazer or Quantum Snare. */
void FUN_001195c0(void *self, char player)
{
	float m1[16];
	float m2[16];
	float v[4];
	float x;
	float z;
	float d;
	char *body;
	char *drill;
	int i = player;

	x = (DR_PF(i, 0x4, float) * 0.0009765625f - (float)DR_HALF_X) * (float)DR_CELL;
	z = (DR_PF(i, 0x8, float) * 0.0009765625f - (float)DR_HALF_Z) * (float)DR_CELL;
	if (player == 0 && DAT_002ecdec == 0) {
		DR_DRIVE_ANIM++;
		if (0x3c < DR_DRIVE_ANIM)
			DR_DRIVE_ANIM = 0;
	}
	D3DDevice_SetRenderState_TextureFactor(0x40000000);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLOROP, 0xe);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLORARG1, 2);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLORARG2, 1);
	D3DDevice_SetTextureStageState(1, D3DTSS_ADDRESSU, 1);
	D3DDevice_SetTextureStageState(1, D3DTSS_ADDRESSV, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 2);
	bdSetWorldIdentity();
	if (2 < g_DrVariant) {
		if (g_DrState != 5)
			DR_PF(i, 0x710, float) = 0.0f;
		bmMatrixRotationYZX(m1, 0.0f, DR_PF(i, 0xc, float) * DR_DEG,
			DR_PF(i, 0x710, float) * -1.5f * DR_DEG);
		bmMatrixRotationYZX(m2, 0.0f, (DR_PF(i, 0xc, float) + 180.0f) * DR_DEG,
			DR_PF(i, 0x710, float) * -1.5f * DR_DEG);
	} else {
		if (g_DrState == 5 && DR_PF(i, 0x20, signed char) != 0) {
			d = DrAi_AngleDiff(DR_AI, DR_PF(i, 0x14, float), DR_PF(i, 0xc, float));
			if (0.0f < d * 0.25f)
				DR_PF(i, 0x14, float) = DR_PF(i, 0x14, float) - 30.0f;
			else if (d * 0.25f < 0.0f)
				DR_PF(i, 0x14, float) = DR_PF(i, 0x14, float) + 30.0f;
			bmMatrixRotationY(m1, (DR_PF(i, 0x14, float) + 180.0f) * DR_DEG);
		} else {
			bmMatrixRotationY(m1, (DR_PF(i, 0xc, float) + 180.0f) * DR_DEG);
		}
		memcpy(m2, m1, sizeof m2);
	}
	body = DR_ACTOR_BODY(i);
	drill = DR_ACTOR_DRILL(i);
	bmMatrixToQuaternion((float *)(body + 0xe0), m1);
	bmMatrixToQuaternion((float *)(drill + 0xe0), m2);
	DR_F32(body, 0xc0) = x;
	DR_F32(body, 0xc4) = DR_PF(i, 0x5c, float);
	DR_F32(body, 0xc8) = z;
	baSetNodeTime((struct BActorInstance *)body, DR_AT(body, 0x134, struct BActorNode *),
		DR_DRIVE_ANIM << 10, 1);
	baPrepActorInstance((struct BActorInstance *)body, 1);
	DR_F32(drill, 0xd0) = 1.0f;
	DR_F32(drill, 0xd4) = 1.0f;
	DR_F32(drill, 0xd8) = 1.0f;
	if (g_DrState == 5) {
		DR_F32(drill, 0xc0) = DR_F32(body, 0xc0);
		DR_F32(drill, 0xc4) = DR_F32(body, 0xc4);
		DR_F32(drill, 0xc8) = DR_F32(body, 0xc8);
		DR_F32(drill, 0xcc) = DR_F32(body, 0xcc);
		if (2 < g_DrVariant)
			DR_F32(drill, 0xc4) = DR_F32(drill, 0xc4) + 22.0f;
	}
	if (DR_TUNE_DRILLER != 0.0f) {
		if (DAT_002ecdec == 0)
			baUpdateAnimations((struct BActorInstance *)drill,
				DR_AT(drill, 0x134, struct BActorNode *), 1.0f, NULL);
		baPrepActorInstance((struct BActorInstance *)drill, 1);
	}
	if (g_DrState != 5) {
		DR_F32(drill, 0xc0) = DR_F32(body, 0xc0);
		DR_F32(drill, 0xc4) = DR_F32(body, 0xc4);
		DR_F32(drill, 0xc8) = DR_F32(body, 0xc8);
		DR_F32(drill, 0xcc) = DR_F32(body, 0xcc);
		if (2 < g_DrVariant)
			DR_F32(drill, 0xc4) = DR_F32(drill, 0xc4) + 22.0f;
	}
	v[0] = DR_F32(drill, 0xc0);
	v[1] = (float)DR_CELL + DR_F32(drill, 0xc4) + 300.0f;
	v[2] = DR_F32(drill, 0xc8);
	if (2 < g_DrVariant)
		baSetPointLight(DR_LIGHT(i), v, 0.0019f);
	if (DR_COUNTDOWN != 0 && g_DrState == 5) {
		bdSetRenderState(6, 1, 0);
		baSetBrightness(0x80 - DR_FTOL(sinf((float)DR_COUNTDOWN * 32.0f * DR_DEG) * -64.0f));
	}
	if (DR_PF(i, 0x20, signed char) != 0 && DR_PF(i, 0x6f4, int) < 1) {
		bdSetRenderState(3, 1, 0);
		if (g_DrVariant < 3)
			baEnableLight(DR_FX_LIGHT, 0);
		if (DR_TUNE_DRILLER != 0.0f) {
			v[3] = 1.0f;
			if (g_DrVariant < 3) {
				v[0] = 0.0f;
				v[1] = 0.0f;
				v[2] = 1.0f;
				baSetDirectionalLight(DR_LIGHT(0), v);
				baSetLightColour(DR_LIGHT(0), 0x7f, 0x7f, 0x7f);
			} else {
				v[0] = DR_CAM_EYE[0];
				v[1] = DR_CAM_EYE[1];
				v[2] = DR_CAM_EYE[2];
				baSetDirectionalLight(DR_CAM_LIGHT, v);
				baSetLightColour(DR_CAM_LIGHT, 0x7f, 0x7f, 0x7f);
				baEnableLight(DR_CAM_LIGHT, 1);
			}
			baSetAmbientLight(0x7f, 0x7f, 0x7f);
			baDrawActorInstance((struct BActorInstance *)drill, 7);
			if (g_DrVariant < 3) {
				v[0] = -1.0f;
				v[1] = -0.5f;
				v[2] = 0.0f;
				v[3] = 1.0f;
				baSetDirectionalLight(DR_LIGHT(0), v);
				baSetLightColour(DR_LIGHT(0), 0x40, 0x40, 0x40);
			} else {
				baEnableLight(DR_CAM_LIGHT, 1);
				baSetLightColour(DR_CAM_LIGHT, 0, 0, 0);
			}
		}
		if (g_DrVariant < 3)
			baSetAmbientLight(0x7f, 0x7f, 0x7f);
		else
			baSetAmbientLight(0x40, 0x40, 0x40);
		bdSetRenderState(3, 0, 0);
		if (DR_TUNE_DRIVER != 0.0f) {
			baDrawActorInstance((struct BActorInstance *)body, 7);
			FUN_001147d0(DR_F32(drill, 0xc0), (float)i, DR_F32(drill, 0xc8));
		}
		baSetAmbientLight(0x40, 0x40, 0x40);
		if (g_DrVariant < 3)
			baEnableLight(DR_FX_LIGHT, 1);
	}
	bdSetWorldIdentity();
	baSetBrightness(0x100);
	bdSetRenderState(6, 0, 0);
	bdSetTexture(1, NULL);
	if (3 < g_DrState && ((DR_COUNTDOWN & 1) == 0 || g_DrState != 5)) {
		if (g_DrVariant == 3)
			FUN_00117d00(self, i);
		if (g_DrVariant == 4)
			FUN_00118400(self, i);
	}
}

/* the debris type "RESPAWNCHAR" (bumpercars_a data) */
extern unsigned char DAT_00291734[];

/* the sound channels of the four drillers' engines */
#define DR_ENGINE_CHANNEL(i) DR_VAR(int, 0x002aaa04u + (i) * 4)

/* a player is out: the commentary when two or more players are still in play (the second
   test of the original can never pass once the first failed) */
static void dr_out_commentary(int character)
{
	int n = 0;

	if (DR_PF(0, 0x20, signed char) == 1)
		n = 1;
	if (DR_PF(1, 0x20, signed char) == 1)
		n++;
	if (DR_PF(2, 0x20, signed char) == 1)
		n++;
	if (DR_PF(3, 0x20, signed char) == 1)
		n++;
	if (1 < n || (2 < n && DR_VAR(int, 0x002ffa58u) != 3 && (g_DrVariant & 1) != 0))
		CLSamples_PlayCommentary(0x37, character, 0xff, 0, 0, NULL);
}

/* 0x00119cd0  Drillers (not in of_functions.tsv)
The move of player `player` for one frame.
- A hit player (timer > 0) fades out, then respawns in a random corner block (keeping 3/4
  of its points) with a puff of debris; 32 frames later it is back (fade 255, timer -64).
- Else it drives along its heading; Quantum Snare's trail follows the head.
- Variants 1 and 2: it keeps to the middle of its lane; the cell under it is drilled
  (Rubble Alliance at once, Erodeo after three passes, with dust and the cell's look);
  Erodeo's tokens are collected; two players on one cell crash (both out), a player that
  leaves the map or drops into a hole is out.
- Variants 3 and 4: the trail records the path; at the edge of the arena (radius 36.1
  cells) the player bounces off with a ring; then the trail tests (foxhunt's 0x00125090),
  the trail grows, and in Quantum Snare the hit timer counts back to 0. */
void FUN_00119cd0(void *self, unsigned int player)
{
	DrMap *map = g_DrMap;
	DrCell *cell;
	float pos[4];
	float fx = 0.0f;
	float fz = 0.0f;
	float ax;
	float az;
	float xs;
	float zs;
	float d;
	float h;
	float a;
	float r;
	signed char *alive;
	int *timer;
	float *fade;
	float *count;
	int i = (signed char)player;
	int type;
	int found;
	int k;
	int j;
	int cx;
	int cz;
	int t;

	(void)self;
	alive = &DR_PF(i, 0x20, signed char);
	found = 0;
	if (*alive == 1 && DR_I32(DR_PF(i, 0x2c, char *), 0x14) != 0)
		bsPlaySample(DR_VAR(struct BSample *, 0x002aa9d8u + DAT_002baed4[i * 10] * 4), 100, 0, 0,
			0);
	timer = &DR_PF(i, 0x6f4, int);
	if (0 < *timer) {
		fade = &DR_PF(i, 0x6f8, float);
		if (*fade > 0.0f) {
			*timer = 0x40;
			return;
		}
		count = &DR_PF(i, 0x71c, float);
		if (*count == 0.0f) {
			cx = dr_rand() & 7;
			cz = dr_rand() & 7;
			if ((dr_rand() & 1) != 0)
				cx += 0x18;
			if ((dr_rand() & 1) != 0)
				cz += 0x18;
			DR_PF(i, 0x4, float) = (float)cx * 1024.0f;
			DR_PF(i, 0x8, float) = (float)cz * 1024.0f;
			xs = (DR_PF(i, 0x4, float) * 0.0009765625f - (float)DR_HALF_X) * (float)DR_CELL;
			zs = (DR_PF(i, 0x8, float) * 0.0009765625f - (float)DR_HALF_Z) * (float)DR_CELL;
			DR_PF(i, 0x60, int) = DR_FTOL((float)DR_PF(i, 0x60, int) * 0.75f);
			DR_F32(DAT_002ecfac, 0xb8 + i * 4) = 90.0f;
			for (k = 0; k < 0x80; k++) {
				DR_TRAIL_X(i, k) = xs;
				DR_TRAIL_Z(i, k) = zs;
			}
			pos[0] = xs;
			pos[2] = zs;
			(void)dr_rand();
			(void)dr_rand();
			(void)dr_rand();
			DAT_00290e8c = DAT_002baed4[i * 10];
			pos[1] = 0.0f;
			pos[3] = 1.0f;
			Debris_Add((struct DebrisType *)DAT_00291734, pos, DAT_002ea478, 0, 0xffffffffu);
		}
		*fade = 0.0f;
		*timer = 0x40;
		*count = *count + 1.0f;
		if (32.0f < *count) {
			*count = 0.0f;
			*fade = 255.0f;
			*timer = -0x40;
		}
		return;
	}

	/* drive */
	if (g_DrVariant == 4 && (DR_PF(i, 0x64, short) < 0x10 || (DR_FRAME & 1) != 0)) {
		for (k = DR_PF(i, 0x64, short) - 1; 0 < k; k--) {
			DR_TRAIL_X(i, k) = DR_TRAIL_X(i, k - 1);
			DR_TRAIL_Z(i, k) = DR_TRAIL_Z(i, k - 1);
		}
	}
	if (DR_PF(i, 0xc, float) < 0.0f)
		DR_PF(i, 0xc, float) = DR_PF(i, 0xc, float) + 360.0f;
	if (359.0f < DR_PF(i, 0xc, float))
		DR_PF(i, 0xc, float) = DR_PF(i, 0xc, float) - 360.0f;
	a = DR_PF(i, 0xc, float) * DR_DEG;
	DR_PF(i, 0x4, float) = sinf(a) * DR_PF(i, 0x18, float) *
		(DR_VAR(float, 0x002aae10u) * DAT_0028fda8) + DR_PF(i, 0x4, float);
	DR_PF(i, 0x8, float) = cosf(a) * DR_PF(i, 0x18, float) *
		(DR_VAR(float, 0x002aae10u) * DAT_0028fda8) + DR_PF(i, 0x8, float);
	xs = DR_PF(i, 0x4, float) * 0.0009765625f;
	DR_PF(i, 0x1c, signed char) = (signed char)DR_FTOL(xs);
	zs = DR_PF(i, 0x8, float) * 0.0009765625f;
	DR_PF(i, 0x1d, signed char) = (signed char)DR_FTOL(zs);
	ax = DR_F32(DR_ACTOR_BODY(i), 0xc0);
	az = DR_F32(DR_ACTOR_BODY(i), 0xc8);
	if (ax < (float)-(DR_CELL * DR_HALF_X)) {
		DR_PF(i, 0x724, int) = 1;
		DR_PF(i, 0x1c, signed char) = 0;
	}
	if ((float)(DR_CELL * DR_HALF_X) - (float)DR_CELL * 0.5f < ax) {
		DR_PF(i, 0x724, int) = 1;
		DR_PF(i, 0x1c, signed char) = (signed char)(DR_HALF_X * 2 - 1);
	}
	if (az < (float)-(DR_CELL * DR_HALF_Z)) {
		DR_PF(i, 0x724, int) = 1;
		DR_PF(i, 0x1d, signed char) = 0;
	}
	if (DR_HALF_Z * 2 <= DR_PF(i, 0x1d, signed char)) {
		DR_PF(i, 0x1d, signed char) = (signed char)(DR_HALF_Z * 2 - 1);
		DR_PF(i, 0x724, int) = 1;
	}

	if (2 < g_DrVariant) {
		fz = az;
		if (g_DrVariant == 3) {
			fx = (xs - (float)DR_HALF_X) * (float)DR_CELL;
			DR_TRAIL_W(i, 0) = 0;
			fz = (zs - (float)DR_HALF_Z) * (float)DR_CELL;
			DR_TRAIL_X(i, 0) = fx;
			DR_TRAIL_Z(i, 0) = fz;
			if (50.0f < sqrtf((fx - DR_TRAIL_X(i, 1)) * (fx - DR_TRAIL_X(i, 1)) +
					(fz - DR_TRAIL_Z(i, 1)) * (fz - DR_TRAIL_Z(i, 1)))) {
				for (k = DR_PF(i, 0x64, short) - 1; 1 <= k; k--) {
					if (DR_TRAIL_X(i, k) != DR_TRAIL_X(i, k - 1) ||
							DR_TRAIL_Z(i, k) != DR_TRAIL_Z(i, k - 1)) {
						DR_TRAIL_X(i, k) = DR_TRAIL_X(i, k - 1);
						DR_TRAIL_Z(i, k) = DR_TRAIL_Z(i, k - 1);
						DR_TRAIL_W(i, k) = DR_TRAIL_W(i, k - 1);
					}
				}
			}
		}
		if (g_DrVariant == 4) {
			fx = (DR_PF(i, 0x4, float) * 0.0009765625f - (float)DR_HALF_X) * (float)DR_CELL;
			fz = (DR_PF(i, 0x8, float) * 0.0009765625f - (float)DR_HALF_Z) * (float)DR_CELL;
			DR_TRAIL_X(i, 0) = fx;
			DR_TRAIL_Z(i, 0) = fz;
		}
		r = (float)DR_CELL * 36.1f;
		if (r < sqrtf(fz * fz + fx * fx)) {
			a = atan2f(fx, fz);
			xs = sinf(a) * r;
			zs = cosf(a) * r;
			baAddExtraItem(g_DrFx->ring, (double)xs, 0.0, (double)zs,
				(int)DR_CHAR_COLOUR(i, 0) << 1, (int)DR_CHAR_COLOUR(i, 1) << 1,
				(int)DR_CHAR_COLOUR(i, 2) << 1, (double)a);
			d = 1.0f / (float)DR_CELL;
			DR_PF(i, 0x4, float) = (d * xs + (float)DR_HALF_X) * 1024.0f;
			DR_PF(i, 0x8, float) = (zs * d + (float)DR_HALF_Z) * 1024.0f;
			h = a * 57.29578f - DR_PF(i, 0xc, float);
			DR_PF(i, 0xc, float) = h + h + DR_PF(i, 0xc, float) + 180.0f;
		}
		if (g_DrVariant == 3) {
			FUN_00125090((int)player);
			if (DR_PF(i, 0x64, short) < 0x7f && (DR_FRAME & 7) == 0)
				DR_PF(i, 0x64, short) = (short)(DR_PF(i, 0x64, short) + 1);
		} else if (g_DrVariant == 4) {
			if (DR_PF(i, 0x64, short) < 0x30) {
				DR_PF(i, 0x68, float) = DR_PF(i, 0x68, float) + 0.2f;
				if (1.0f < DR_PF(i, 0x68, float)) {
					DR_PF(i, 0x68, float) = 0.0f;
					DR_PF(i, 0x64, short) = (short)(DR_PF(i, 0x64, short) + 1);
				}
			}
			t = *timer;
			if (0 < t) {
				*timer = t - 1;
			} else if (t < 0) {
				*timer = t + 1;
				if (DR_VAR(float, 0x002aae40u) != 0.0f && -0x20 < t + 1)
					*timer = -0x40;
			} else {
				*timer = 0;
			}
			FUN_00125090((int)player);
		}
		return;
	}

	/* variants 1 and 2: keep to the lane */
	if (DR_PF(i, 0xc, float) == 0.0f || dr_fbits(DR_PF(i, 0xc, float)) == 0x43340000u) {
		k = DR_PF(i, 0x1c, signed char) * 0x400 + 0x200;
		DR_PF(i, 0x4, float) = ((float)k - DR_PF(i, 0x4, float)) * 0.125f + DR_PF(i, 0x4, float);
	} else {
		k = DR_PF(i, 0x1d, signed char) * 0x400 + 0x200;
		DR_PF(i, 0x8, float) = ((float)k - DR_PF(i, 0x8, float)) * 0.125f + DR_PF(i, 0x8, float);
	}
	cell = &map->columns[DR_PF(i, 0x1c, signed char)][DR_PF(i, 0x1d, signed char)];
	fx = (DR_PF(i, 0x4, float) * 0.0009765625f - (float)DR_HALF_X) * (float)DR_CELL;
	fz = (DR_PF(i, 0x8, float) * 0.0009765625f - (float)DR_HALF_Z) * (float)DR_CELL;
	if (cell->height == 0x400)
		type = 3;
	else
		type = (cell->height == 0x401) + 1;
	if (g_DrVariant == 2) {
		baAddExtraItem(g_DrFx->dust, (double)fx, 0.0, (double)fz, type);
		baAddExtraItem(g_DrFx->dust, (double)fx, 0.0, (double)fz, type);
	}
	cx = DR_PF(i, 0x1c, signed char);
	cz = DR_PF(i, 0x1d, signed char);
	cell = &map->columns[cx][cz];
	if (0x3ff < cell->height) {
		if (g_DrVariant == 2) {
			if (DR_PF(i, 0x1e, signed char) != cx || DR_PF(i, 0x1f, signed char) != cz)
				cell->height = (short)(cell->height + 1);
			if (DR_PF(i, 0x704, signed char) != 0 &&
					(DR_PF(i, 0x1e, signed char) != DR_PF(i, 0x1c, signed char) ||
					DR_PF(i, 0x1f, signed char) != DR_PF(i, 0x1d, signed char))) {
				DR_PF(i, 0x1e, signed char) = DR_PF(i, 0x1c, signed char);
				DR_PF(i, 0x1f, signed char) = DR_PF(i, 0x1d, signed char);
			}
			cell = &map->columns[DR_PF(i, 0x1c, signed char)][DR_PF(i, 0x1d, signed char)];
			if (cell->height == 0x401)
				cell->look = 1;
			if (cell->height == 0x402)
				cell->look = 2;
			if (0x402 < cell->height) {
				cell->height = 0x3ff;
				if (bsCountPlaying() < bsGetNumChannels() / 2)
					bsPlaySample(DR_SAMPLE(0x002aa9bcu), 0x96, 0, 0, 0);
				if (DR_PF(i, 0xc, float) == 0.0f ||
						dr_fbits(DR_PF(i, 0xc, float)) == 0x43340000u)
					cell->look = 3;
				else
					cell->look = 6;
			}
			for (k = 0; k < 22; k++) {
				DrTokenSlot *ts = &g_DrFx->tokens[k];
				float tx;
				float tz;

				if (ts->active == 0 || ts->bounces <= 2.0f)
					continue;
				if ((float)DR_PF(i, 0x1c, signed char) != (float)DR_HALF_X + ts->x)
					continue;
				if ((float)DR_PF(i, 0x1d, signed char) != (float)DR_HALF_Z + ts->z)
					continue;
				tx = (float)DR_CELL * ts->x + (float)(DR_CELL / 2);
				tz = (float)DR_CELL * ts->z + (float)(DR_CELL / 2);
				switch (ts->type) {
				case 0:
					DR_PF(i, 0x60, int) += 1;
					DrFx_AddScoreFloaty(tx, tz, 0, (char)player);
					break;
				case 1:
					DR_PF(i, 0x60, int) += 3;
					DrFx_AddScoreFloaty(tx, tz, 1, (char)player);
					break;
				case 2:
					DR_PF(i, 0x60, int) += 5;
					DrFx_AddScoreFloaty(tx, tz, 2, (char)player);
					break;
				}
				found = 1;
				ts->active = 0;
				CToken_Collect(ts->token);
			}
			if (found)
				bsPlaySample(DR_SAMPLE(0x002aa9b4u), 0xff, 0, 0, 0);
		} else if (g_DrVariant == 1) {
			cell->height = 0x3ff;
			if (bsCountPlaying() < bsGetNumChannels() / 2)
				bsPlaySample(DR_SAMPLE(0x002aa9bcu), 0x96, 0, 0, 0);
			if (DR_PF(i, 0xc, float) == 0.0f || dr_fbits(DR_PF(i, 0xc, float)) == 0x43340000u)
				cell->look = 1;
			else
				cell->look = 4;
		}
	}

	/* two players on one cell (both low enough) */
	for (j = 0; j < 4; j++) {
		float half;

		if (i == j || DR_PF(j, 0x20, signed char) != 1)
			continue;
		half = (float)(DR_CELL / 2);
		if (!(DR_PF(i, 0x70c, float) > half || DR_PF(j, 0x70c, float) < half))
			continue;
		if (!(DR_PF(j, 0x70c, float) > half || DR_PF(i, 0x70c, float) < half))
			continue;
		if (DR_PF(i, 0x1c, signed char) != DR_PF(j, 0x1c, signed char) ||
				DR_PF(i, 0x1d, signed char) != DR_PF(j, 0x1d, signed char))
			continue;
		if (DR_VAR(float, 0x002aae3cu) == 0.0f) {
			if (DR_COUNTDOWN != 0)
				continue;
			*alive = 2;
			DR_PF(j, 0x20, signed char) = 2;
			pos[0] = fx;
			pos[1] = (float)DR_CELL;
			pos[2] = fz;
			pos[3] = 1.0f;
			DrFxGroup_Start(&g_DrFx->groups[i], pos, i);
			DrFxGroup_Start(&g_DrFx->groups[j], pos, j);
			bsPlaySample(DR_SAMPLE(0x002aa9c8u), 200, 0, 0, 0);
			dr_out_commentary(DAT_002baed4[j * 10]);
			bsSetChannelFrequency(DR_ENGINE_CHANNEL(j), 22000);
			bsSetChannelFrequency(DR_ENGINE_CHANNEL(i), 22000);
			bsStopChannel(DR_ENGINE_CHANNEL(j));
			bsStopChannel(DR_ENGINE_CHANNEL(i));
		} else {
			DR_PF(i, 0x60, int) -= 10;
			if (DR_PF(i, 0x60, int) < 0)
				DR_PF(i, 0x60, int) = 0;
			DR_PF(j, 0x60, int) -= 10;
			if (DR_PF(j, 0x60, int) < 0)
				DR_PF(j, 0x60, int) = 0;
			DR_PF(i, 0xc, float) = DR_PF(i, 0xc, float) + 180.0f;
			DR_PF(j, 0xc, float) = DR_PF(j, 0xc, float) + 180.0f;
		}
	}

	/* off the map */
	if (DR_PF(i, 0x724, int) == 1) {
		DR_PF(i, 0x724, int) = 2;
		pos[1] = (float)DR_CELL;
		pos[0] = fx;
		pos[2] = fz;
		pos[3] = 1.0f;
		DrFxGroup_Start(&g_DrFx->groups[i], pos, i);
		*alive = 2;
		bsPlaySample(DR_SAMPLE(0x002aa9c8u), 100, 0, 0, 0);
		bsStopChannel(DR_ENGINE_CHANNEL(i));
		dr_out_commentary(DAT_002baed4[i * 10]);
	}

	/* a hole */
	cell = &map->columns[DR_PF(i, 0x1c, signed char)][DR_PF(i, 0x1d, signed char)];
	if (cell->height < (g_DrVariant != 1 ? 0x320 : 0x366) &&
			DR_PF(i, 0x70c, float) <= (float)(DR_CELL / 2) && *alive == 1) {
		CLSamples_PlayFromSet((char)DAT_002baed4[i * 10], 0xd, NULL, 200, 0, -1, 0);
		bsSetChannelFrequency(DR_ENGINE_CHANNEL(i), 22000);
		bsStopChannel(DR_ENGINE_CHANNEL(i));
		*alive = 2;
		dr_out_commentary(DAT_002baed4[i * 10]);
	}
}
