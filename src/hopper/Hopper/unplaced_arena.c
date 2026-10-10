/*
Hopper/unplaced_arena.c  <-  gap functions 0x00142760-0x00145c90 (no source path in the XBE)
(module hopper)

The arena object (0x002aec10, vtable 0x0026edd4): the scenery of each variation, its
animation and its drawing, and the tests whether a point is under a blade.

The object (0x57c bytes):
  +0x004 float   an animation clock (variation 1: 0..29, variation 2: 0..68)
  +0x008, +0x00c variation 1: the blade texture's scroll
  +0x010 the water field (variation 4, 0x002aec20)       +0x014 BLight *[8] (0x002aec24)
  +0x034 int[27] variation 2: how much each platform shakes (0: still, 20: falls)
  +0x0a0 int[27] variation 2: each platform's alpha (0x100 there, falls to 0)
  +0x10c int[18] variation 2: the order the platforms fall in (platform index; 0: done)
  +0x154 int     variation 2: the platform shaking now (99: none)
  +0x160, +0x164 variation 4: the arm's turn (negative) and speed (hop_SpeedBoost)
  +0x168 float   variation 2: the spot lights' sway
  +0x26c BActorInstance *[64] the model instances (0x002aee7c), +0x36c their count
  +0x370 the extra models [32], +0x3f0 their instances [32], +0x470 their count
  +0x474, +0x478 variation 1: the two propellers' angles; +0x47c their speed
  +0x480, +0x484 variation 1: the cable lifts' offsets
  +0x488 the platforms (CharLib platforms, 0x002af098 [27])
  +0x500..+0x540 variation 3: the blades' wobble; +0x50c..+0x51c the five blade angles
  +0x548 variation 4: the arm's angle, +0x54c how far the arena sank (hop_Sink), +0x550
         its sink speed, +0x554 a pulse, +0x55c the texture spin
  +0x560 variation 4: frame counter   +0x564..+0x56c variation 3: electric texture scroll
  +0x574 variation 4: the drill wake instance   +0x578 variation 3: frames before the wobble
*/

#include "hopper.h"

#define AR_F(self, off) HOP_AF(self, off)
#define AR_I(self, off) HOP_AI(self, off)
#define AR_INST(self, i) HOP_AT(self, 0x26c + (i) * 4, struct BActorInstance *)
#define AR_COUNT(self) HOP_AI(self, 0x36c)
#define AR_XINST(self, i) HOP_AT(self, 0x3f0 + (i) * 4, struct BActorInstance *)
#define AR_XCOUNT(self) HOP_AI(self, 0x470)
#define AR_WATER(self) HOP_AP(self, 0x10)

#define hop_Detail HOP_F(0x002ae964)          /* tweak "detail" */
#define hop_Frame HOP_I(0x003027b4)           /* the game's frame counter */
#define hop_PauseState HOP_B(0x003027b0)

/* places a model instance: rotation matrix m, position (x, y, z), node time t, scale 1 */
static void hop_PlaceInstance(struct BActorInstance *inst, float *m, float x, float y, float z, int t)
{
	bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
	HOP_AF(inst, 0xc0) = x;
	HOP_AF(inst, 0xc4) = y;
	HOP_AF(inst, 0xc8) = z;
	baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), t, 1);
	baPrepActorInstance(inst, 1);
}

static void hop_UnitScale(struct BActorInstance *inst)
{
	HOP_AF(inst, 0xd0) = 1.0f;
	HOP_AF(inst, 0xd4) = 1.0f;
	HOP_AF(inst, 0xd8) = 1.0f;
}

/* texture stage 1 modulates by a constant alpha (the texture factor); the same states as
charlib 0x000af8f0, inlined in the original */
static void hop_ConstantAlphaStage(DWORD factor)
{
	D3DDevice_SetRenderState_TextureFactor(factor);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)10, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)11, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
}

/* texture stage 1's matrix: a rotation by `angle` (radians) about the texture's w axis,
offset (du, dv) */
static void hop_TextureSpin(float angle, float du, float dv)
{
	float m[16];
	float s = sinf(angle);
	float c = cosf(angle);

	memset(m, 0, sizeof m);
	m[0] = c;
	m[2] = -s;
	m[5] = 1.0f;
	m[8] = s + du;
	m[9] = dv;
	m[10] = c;
	m[15] = 1.0f;
	D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)3, (const D3DMATRIX *)m);
}

/* is (x, z) inside the polygon of n points (x, z pairs)? (drillers 0x00121990, `self` not
used) */
static int hop_InPolygon(float x, float z, float *poly, int n)
{
	return FUN_00121990(NULL, x, z, n, poly) != 0;
}

/* the quad between angles a1 and a2 (degrees) from radius r1 to r2, closed */
static int hop_InSector(float x, float z, float a1, float a2, float r1, float r2)
{
	float poly[10];

	a1 = a1 * HOP_DEG;
	a2 = a2 * HOP_DEG;
	poly[0] = r1 * sinf(a1);
	poly[1] = cosf(a1) * r1;
	poly[2] = sinf(a1) * r2;
	poly[3] = cosf(a1) * r2;
	poly[4] = r2 * sinf(a2);
	poly[5] = cosf(a2) * r2;
	poly[6] = sinf(a2) * r1;
	poly[7] = cosf(a2) * r1;
	poly[8] = poly[0];
	poly[9] = poly[1];
	return hop_InPolygon(x, z, poly, 5);
}

/* the triangle of a propeller blade (variation 1) from the centre: the points at angles a1
and a2 at radius 900 and a2b at radius rb */
static int hop_InBlade(float x, float z, float a1, float a2, float a3, float r3)
{
	float poly[10];

	a1 = a1 * HOP_DEG;
	a2 = a2 * HOP_DEG;
	a3 = a3 * HOP_DEG;
	poly[0] = 0.0f;
	poly[1] = 0.0f;
	poly[2] = sinf(a1) * 900.0f;
	poly[3] = cosf(a1) * 900.0f;
	poly[4] = sinf(a2) * 900.0f;
	poly[5] = cosf(a2) * 900.0f;
	poly[6] = sinf(a3) * r3;
	poly[7] = cosf(a3) * r3;
	poly[8] = 0.0f;
	poly[9] = 0.0f;
	return hop_InPolygon(x, z, poly, 5);
}

/* 0x00142760  (gap): the arena's start */
void HopArena_Start(void *self)
{
	static const int order[18] = { 1, 4, 8, 0x17, 10, 0x1a, 0xb, 0x19, 0x12, 0x13, 0x15, 2, 0xc, 6, 0xe, 0xf,
		0x10, 0x16 };
	int i;

	AR_F(self, 0x474) = 0.0f;
	AR_F(self, 0x478) = 0.0f;
	AR_F(self, 0x480) = 0.0f;
	AR_F(self, 0x484) = 0.0f;
	AR_F(self, 0x47c) = 0.0f;
	AR_F(self, 0x548) = 0.0f;
	AR_F(self, 0x54c) = 0.0f;
	AR_F(self, 0x550) = 0.0f;
	AR_F(self, 0x004) = 0.0f;
	AR_F(self, 0x508) = 0.7f;
	AR_F(self, 0x160) = -0.1f;
	AR_F(self, 0x164) = 0.0f;
	AR_F(self, 0x55c) = 0.0f;
	AR_F(self, 0x008) = 0.0f;
	AR_F(self, 0x00c) = 0.0f;
	AR_I(self, 0x578) = 0;
	AR_F(self, 0x520) = 0.0f;
	AR_F(self, 0x524) = 0.0f;
	AR_F(self, 0x528) = 0.0f;
	AR_F(self, 0x52c) = 0.0f;
	AR_F(self, 0x530) = 0.0f;
	AR_F(self, 0x50c) = 0.0f;
	AR_F(self, 0x510) = 0.0f;
	AR_F(self, 0x514) = 0.0f;
	AR_F(self, 0x518) = 0.0f;
	AR_F(self, 0x51c) = 0.0f;
	AR_F(self, 0x564) = 0.0f;
	AR_F(self, 0x568) = 0.0f;
	AR_F(self, 0x56c) = 0.0f;
	AR_F(self, 0x500) = 0.0f;
	for (i = 0; i < 0x1b; i++) {
		AR_I(self, 0xa0 + i * 4) = 0x100;
		AR_I(self, 0x34 + i * 4) = 0;
	}
	for (i = 0; i < 0x12; i++)
		AR_I(self, 0x10c + i * 4) = order[i] - 1;
}

/* 0x001428e0  (gap): variation 1 per frame (fast: only the platform instance). The
propellers turn faster while the clock runs and slow down in the last 3 seconds (300
hundredths); the cable lifts run; the instances get their transforms. */
void HopArena_Update1(void *self, int fast)
{
	float m[16];
	float a;
	struct BActorInstance *inst;
	int i;

	a = AR_F(self, 0x004) + 1.0f;
	AR_F(self, 0x004) = a;
	if (29.0f < a)
		AR_F(self, 0x004) = 0.0f;
	a = AR_F(self, 0x47c) * 42.6666f + AR_F(self, 0x480);
	AR_F(self, 0x480) = a;
	if (8965.0f < a)
		AR_F(self, 0x480) = a - 8965.0f;
	a = AR_F(self, 0x47c) * 85.3333f + AR_F(self, 0x484);
	AR_F(self, 0x484) = a;
	if (8965.0f < a)
		AR_F(self, 0x484) = a - 8965.0f;
	for (i = 0; i < AR_COUNT(self); i++) {
		inst = AR_INST(self, i);
		hop_UnitScale(inst);
		bdSetWorldIdentity();
		if (i == 0) {
			if (fast == 0) {
				if (hop_TimeLeft <= 300) {
					if (AR_F(self, 0x47c) <= 0.0f)
						AR_F(self, 0x47c) = 0.0f;
					else
						AR_F(self, 0x47c) = AR_F(self, 0x47c) - 0.005f;
				} else if (AR_F(self, 0x47c) < 0.6f && hop_Play == 5) {
					AR_F(self, 0x47c) = AR_F(self, 0x47c) + 0.001f;
				}
				a = DAT_0028fda8 * AR_F(self, 0x47c) * HOP_F(0x002ae950) + AR_F(self, 0x474);
				AR_F(self, 0x474) = a;
				if (360.0f < a)
					AR_F(self, 0x474) = a - 360.0f;
				a = HOP_F(0x002ae954) * AR_F(self, 0x47c) * DAT_0028fda8 + AR_F(self, 0x478);
				AR_F(self, 0x478) = a;
				if (360.0f < a)
					AR_F(self, 0x478) = a - 360.0f;
			}
			bmMatrixRotationX(m, 0.0f);
		} else if (i == 1 || i == 0xc) {
			bmMatrixRotationY(m, AR_F(self, 0x474) * HOP_DEG);
		} else if (i == 2 || i == 0xd) {
			bmMatrixRotationY(m, AR_F(self, 0x478) * HOP_DEG);
		} else if (i == 10 || i == 0xf) {
			bmMatrixRotationYXZ(m, AR_F(self, 0x474) * -12.0f * HOP_DEG, -AR_F(self, 0x474) * HOP_DEG, 0.0f);
		} else {
			bmMatrixRotationY(m, -AR_F(self, 0x474) * HOP_DEG);
		}
		inst = AR_INST(self, i);
		bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
		HOP_AF(inst, 0xc0) = 0.0f;
		if (i == 0xb || (2 < i && i < 9))
			a = -AR_F(self, 0x484);
		else if (i == 0x10 || (0x10 < i && i < 0x17))
			a = 8965.0f - AR_F(self, 0x484);
		else if (i == 9 || i == 10)
			a = 6000.0f - AR_F(self, 0x480);
		else if (i == 0xe || i == 0xf)
			a = 6000.0f - (AR_F(self, 0x480) + 8965.0f);
		else
			a = 0.0f;
		HOP_AF(inst, 0xc4) = a;
		HOP_AF(inst, 0xc8) = 0.0f;
		if (i < 3 || 8 < i)
			baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), 0, 1);
		else
			baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), HOP_FTOL(AR_F(self, 0x004)), 1);
		baPrepActorInstance(inst, 1);
		HOP_AF(hop_Platforms[0], 0xe0) = 0.0f;
		HOP_AF(hop_Platforms[0], 0xe4) = 0.0f;
		HOP_AF(hop_Platforms[0], 0xe8) = 0.0f;
	}
}

/* 0x00142cd0  (gap): variation 2: while playing, every 2 seconds of the 60 (after the first
5) the next platform of the order starts to shake, with a sound */
void HopArena_DropNext(void *self)
{
	int t, k, idx;

	if (hop_Play != 5)
		return;
	t = HOP_I(0x002b0110) / 100;
	if (4 < 0x3c - t) {
		k = (0x37 - t) / 2;
		if (k < 0x12 && (idx = AR_I(self, 0x10c + k * 4)) != 0) {
			AR_I(self, 0x34 + idx * 4)++;
			AR_I(self, 0x10c + k * 4) = 0;
			bsPlaySample(HOP_G(0x002ae9b0, struct BSample *), 0x96, 0, HOP_AI(HOP_P(0x002ae9b0), 0x20) * 3, 0);
		}
	}
}

/* 0x00142d50  (gap): variation 4 per frame (fast: only the arm). The arm speeds up while
playing (and slows by 0.1 a frame outside the pause state 0), turns; the drill wake follows
the camera angle; the water ripples (every other frame), gets 100 random drops and, while
playing, a splash in the centre every 64 frames. */
void HopArena_Update4(void *self, int fast)
{
	float m[16];
	float a, add;
	struct BActorInstance *inst;
	void *water;
	int n, i;

	n = AR_I(self, 0x560) + 1;
	AR_I(self, 0x560) = n;
	bdTextureSetFrame(HOP_G(0x002aed68, struct bdTexture *), n);
	bdTextureSetFrame(HOP_G(0x002aed6c, struct bdTexture *), AR_I(self, 0x560) + 10);
	bdTextureSetFrame(HOP_G(0x002aea40, struct bdTexture *), AR_I(self, 0x560) / 2);
	if (hop_Play == 5) {
		add = 0.0004960318f;
		if (AR_F(self, 0x164) < 0.90909088f)
			add = 0.05f;
		AR_F(self, 0x164) = AR_F(self, 0x164) + add;
	}
	AR_F(self, 0x160) = -(AR_F(self, 0x164) + 0.1f);
	if (hop_PauseState == 0) {
		a = AR_F(self, 0x164) - 0.1f;
		AR_F(self, 0x164) = a;
		if (a < 0.0f) {
			AR_F(self, 0x164) = 0.0f;
			AR_F(self, 0x160) = 0.0f;
		}
	}
	for (i = 0; i < AR_COUNT(self); i++) {
		if (fast != 0 && i != 0)
			continue;
		inst = AR_INST(self, i);
		hop_UnitScale(inst);
		bdSetWorldIdentity();
		if (i == 0) {
			if (fast == 0) {
				a = AR_F(self, 0x548) + AR_F(self, 0x160);
				AR_F(self, 0x548) = a;
				if (360.0f < a)
					AR_F(self, 0x548) = a - 360.0f;
			}
			a = AR_F(self, 0x548) * HOP_DEG;
		} else if (i == 1 || i == 2) {
			a = 0.0f;
		} else {
			a = AR_F(self, 0x548) * HOP_DEG;
		}
		bmMatrixRotationY(m, a);
		inst = AR_INST(self, i);
		bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
		HOP_AF(inst, 0xc0) = 0.0f;
		if (i == 1)
			HOP_AF(AR_INST(self, 1), 0xc4) = 0.0f;
		else if (i == 2)
			HOP_AF(AR_INST(self, 2), 0xc4) = 9463.0f;
		else
			HOP_AF(inst, 0xc4) = -AR_F(self, 0x54c);
		HOP_AF(inst, 0xc8) = 0.0f;
		baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), 0, 1);
		baPrepActorInstance(inst, 1);
	}
	inst = HOP_AT(self, 0x574, struct BActorInstance *);
	hop_UnitScale(inst);
	bmMatrixRotationY(m, (hop_CamAngle - 89.0f) * HOP_DEG);
	bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
	HOP_AF(inst, 0xc4) = -AR_F(self, 0x54c);
	baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), 0, 1);
	baPrepActorInstance(inst, 1);
	water = AR_WATER(self);
	HOP_AF(water, 8) = -AR_F(self, 0x54c);
	if ((hop_Frame & 1) != 0)
		HOP_VFN(water, 0xc, int (*)(void *))(water);
	for (i = 100; i != 0; i--)
		HOP_VFN(water, 0x24, int (*)(void *, float))(water, 0.05f);
	if (hop_Play == 5 && (hop_Frame & 0x3f) == 0)
		HOP_VFN(water, 0x28, int (*)(void *, float, float, float, float, float))(water, 0.0f, 0.0f, -20.0f, 200.0f,
			500.0f);
}

/* 0x001430a0  (gap): variation 4 after the players: the water (tweak "draw water") turned
with the arm, then the drill wake shaded by the arm's speed */
void HopArena_DrawExtras4(void *self)
{
	float m[16];
	float f;
	void *water;

	if (HOP_F(0x002ae984) == 0.0f)
		return;
	bdSetRenderState(5, 0, 0);
	bmMatrixRotationY(m, AR_F(self, 0x548) * HOP_DEG);
	bdSetWorldMatrix(m);
	water = AR_WATER(self);
	HOP_VFN(water, 0x14, int (*)(void *))(water);
	CWaterField_SetMaterial(0xffffffffu, 0xff333333u, 50.0f, 1);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	water = AR_WATER(self);
	HOP_VFN(water, 0x1c, int (*)(void *, int))(water, 0);
	CWaterField_ResetMaterial();
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(7, 1, 0);
	f = HOP_F(0x002aed74) * 0.25f;
	if (1.0f < f)
		f = 1.0f;
	HopShader_SetConstant(f, f, f, f);
	baSetRenderPass(HOP_AT(self, 0x574, struct BActorInstance *), 0, 7, (int)(uintptr_t)HOP_ADDR(0x002ae8e4));
	bdSetWorldIdentity();
	baDrawActorInstance(HOP_AT(self, 0x574, struct BActorInstance *), 7);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(7, 2, 0);
}

/* 0x001431e0  (gap): deletes the 8 lights and the platforms */
void HopArena_Shutdown(void *self)
{
	struct BLight **light = (struct BLight **)((char *)self + 0x14);
	int i;

	for (i = 8; i != 0; i--, light++) {
		if (*light != NULL) {
			baEnableLight(*light, 0);
			baDeleteLight(*light);
			*light = NULL;
		}
	}
	Platforms_DeleteAll();
}

/* 0x00143220  (gap): scalar deleting destructor */
void *HopArena_Destroy(void *self, byte flags)
{
	*(const void *const **)self = vtable_0026edd4;
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026edd4  the arena's vtable */
const void *const vtable_0026edd4[1] = {
	(const void *)FUN_00143220, /* [0] scalar deleting destructor */
};

/* the 8 spot lights of variation 2: x positions [0..7], z positions [8..15] */
static const uint32_t hop_SpotPos[16] = {
	0xc0466666u, 0x448e8333u, 0x44ce0666u, 0x44a0e000u, 0x4109999au, 0xc4a34000u, 0xc4cf6000u, 0xc48f499au,
	0x44ce8666u, 0x44976000u, 0x00000000u, 0xc485f333u, 0xc4ce8666u, 0xc484e000u, 0xc0f9999au, 0x4498899au,
};

/* 0x00143240  (gap): variation 2 per frame (fast: only instance 0). Instances 8..34 are the
27 platforms, 35..61 their faces: a shaking platform jitters up and down (more as it shakes
longer) and, after 20 steps, fades and sinks 256 down; the faces follow. Instances 1..7 are
animated. With "detail" the 16 spot lights sway. */
void HopArena_Update2(void *self, int fast)
{
	float m[16];
	float a, off, s;
	struct BActorInstance *inst;
	int shaken, k, v, i, t, j;
	unsigned int r;

	a = AR_F(self, 0x168) + 3.0f;
	AR_F(self, 0x168) = a;
	if (360.0f < a)
		AR_F(self, 0x168) = a - 360.0f;
	HopArena_DropNext(self);
	AR_I(self, 0x154) = 99;
	for (i = 0; i < AR_COUNT(self); i++) {
		if (fast != 0 && i != 0)
			continue;
		off = 0.0f;
		shaken = 0;
		inst = AR_INST(self, i);
		if ((0x22 < i && i < 0x3e) || (7 < i && i < 0x23)) {
			k = i - (i < 0x23 ? 8 : 0x23);
			v = AR_I(self, 0x34 + k * 4);
			if (v < 0x14) {
				if (0 < v && (hop_Frame & 7) == 0)
					AR_I(self, 0x34 + k * 4) = v + 1;
				if (0 < AR_I(self, 0x34 + k * 4)) {
					if (i < 0x23) {
						r = (unsigned int)hop_rand();
						v = AR_I(self, 0x34 + k * 4);
						shaken = 1;
						HOP_AF(inst, 0xc4) = (float)((int)r % (v * 2)) - (float)v;
						AR_I(self, 0x154) = i - 8;
					} else {
						HOP_AF(inst, 0xc4) = HOP_AF(AR_INST(self, i - 0x1b), 0xc4);
						shaken = 1;
					}
				}
			} else if (AR_I(self, 0xa0 + k * 4) == 0x100) {
				AR_I(self, 0xa0 + k * 4) = 0xf8;
				bsPlaySample(HOP_G(0x002ae9ac, struct BSample *), 0xfa, 0, 0, 0);
			}
			v = AR_I(self, 0xa0 + k * 4);
			if (v != 0x100) {
				if (0 < v)
					AR_I(self, 0xa0 + k * 4) = v - 4;
				if (AR_I(self, 0xa0 + k * 4) < 0)
					AR_I(self, 0xa0 + k * 4) = 0;
				off = 256.0f - (float)AR_I(self, 0xa0 + k * 4);
			}
		}
		hop_UnitScale(inst);
		bdSetWorldIdentity();
		if (7 < i && i < 0x23)
			a = HOP_AF(hop_Platforms[i - 8], 0x1f8);
		else if (0x22 < i && i < 0x3e)
			a = HOP_AF(hop_Platforms[i - 0x23], 0x1f8);
		else
			a = 0.0f;
		bmMatrixRotationY(m, a);
		bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
		HOP_AF(inst, 0xc0) = 0.0f;
		if (!shaken)
			HOP_AF(inst, 0xc4) = -off;
		HOP_AF(inst, 0xc8) = 0.0f;
		t = 0;
		if (0 < i && i < 8)
			t = HOP_FTOL(AR_F(self, 0x004)) << 9;
		baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), t, 1);
		baPrepActorInstance(inst, 1);
	}
	a = AR_F(self, 0x004) + 1.0f;
	AR_F(self, 0x004) = a;
	if (68.0f < a)
		AR_F(self, 0x004) = 0.0f;
	if (hop_Detail == 0.0f || AR_XCOUNT(self) <= 0)
		return;
	for (i = 0; i < AR_XCOUNT(self); i++) {
		inst = AR_XINST(self, i);
		hop_UnitScale(inst);
		bdSetWorldIdentity();
		s = AR_F(self, 0x168);
		if (((unsigned int)i & 2) == 0)
			s = -s;
		s = sinf(s * HOP_DEG);
		j = i / 2;
		bmMatrixRotationY(m, ((float)j * 45.0f + s * 40.0f) * HOP_DEG);
		bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
		HOP_AU(inst, 0xc0) = hop_SpotPos[j];
		HOP_AF(inst, 0xc4) = -33.0f;
		HOP_AU(inst, 0xc8) = hop_SpotPos[j + 8];
		baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), 0, 1);
		baPrepActorInstance(inst, 1);
	}
}

/* 0x001436e0  (gap): variation 3: the blades' speeds wobble. Two angle clocks (+0x500 at
0.35 a frame, +0x504 at 0.55) give the speeds; every 64 steps of a clock two random +-0.3
values are drawn (and then overwritten, as in the original). The speeds ease (1/16) to
their goals; 0 when not playing. */
void HopArena_Wobble3(void *self)
{
	int k;

	if (hop_Play != 5) {
		AR_F(self, 0x520) = 0.0f;
		AR_F(self, 0x524) = 0.0f;
		AR_F(self, 0x528) = 0.0f;
		AR_F(self, 0x52c) = 0.0f;
		return;
	}
	AR_F(self, 0x500) = AR_F(self, 0x500) + 0.35f;
	AR_F(self, 0x504) = AR_F(self, 0x504) + 0.55f;
	if ((HOP_FTOL(AR_F(self, 0x500)) & 0x3f) == 0) {
		for (k = 0; k < 2; k++)
			AR_F(self, 0x534 + k * 4) = (hop_rand() & 1) != 0 ? -0.3f : 0.3f;
	}
	if ((HOP_FTOL(AR_F(self, 0x504)) & 0x3f) == 0) {
		for (k = 0; k < 2; k++)
			AR_F(self, 0x53c + k * 4) = (hop_rand() & 1) != 0 ? -0.3f : 0.3f;
	}
	AR_F(self, 0x538) = (AR_F(self, 0x500) * 0.0013020834f + 1.3f) *
		((signed char)HOP_FTOL(AR_F(self, 0x500)) < 0 ? -0.5f : 0.5f);
	AR_F(self, 0x540) = (AR_F(self, 0x504) * 0.0013020834f + 1.3f) *
		((signed char)HOP_FTOL(AR_F(self, 0x504)) < 0 ? 0.6f : -0.6f);
	AR_F(self, 0x534) = AR_F(self, 0x538);
	AR_F(self, 0x53c) = AR_F(self, 0x540);
	for (k = 0; k < 4; k++)
		AR_F(self, 0x520 + k * 4) = AR_F(self, 0x520 + k * 4) -
			(AR_F(self, 0x520 + k * 4) - AR_F(self, 0x534 + k * 4)) * 0.0625f;
}

/* the two coloured point lights of variation 3 */
static void hop_Lights3(void)
{
	float pos[3];

	pos[0] = -2000.0f;
	pos[1] = 270.0f;
	pos[2] = -3000.0f;
	baSetPointLight(HOP_G(0x002aec28, struct BLight *), pos, HOP_F(0x002ae970));
	baSetLightColour(HOP_G(0x002aec28, struct BLight *), 0x7f, 0x40, 0);
	baEnableLight(HOP_G(0x002aec28, struct BLight *), 1);
	pos[0] = 0.0f;
	pos[1] = 700.0f;
	pos[2] = -300.0f;
	baSetPointLight(HOP_G(0x002aec2c, struct BLight *), pos, HOP_F(0x002ae974));
	baSetLightColour(HOP_G(0x002aec2c, struct BLight *), 0x10, 0x10, 0x60);
	baEnableLight(HOP_G(0x002aec2c, struct BLight *), 1);
}

/* 0x001438c0  (gap): variation 3 per frame (fast: only instance 0). Sets the view (the
camera's) and the lights; while playing, after 33 frames the blades wobble; the five blade
angles turn by their speeds; the instances get their rotations. */
void HopArena_Update3(void *self, int fast)
{
	float m[16];
	struct BActorInstance *inst;
	int i, k;

	HopCamera_Apply(HOP_ADDR(0x002aebd8));
	hop_Lights3();
	if (hop_PauseState != 0 && hop_Play == 5) {
		if (AR_I(self, 0x578) < 0x21)
			AR_I(self, 0x578)++;
		else
			HopArena_Wobble3(self);
		for (k = 0; k < 5; k++)
			AR_F(self, 0x50c + k * 4) = DAT_0028fda8 * AR_F(self, 0x520 + k * 4) + AR_F(self, 0x50c + k * 4);
	}
	for (i = 0; i < AR_COUNT(self); i++) {
		if (fast != 0 && i != 0)
			continue;
		inst = AR_INST(self, i);
		hop_UnitScale(inst);
		bdSetWorldIdentity();
		switch (i) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 0xe:
		case 0x13:
			bmMatrixRotationY(m, AR_F(self, 0x510) * HOP_DEG);
			break;
		case 4:
		case 5:
		case 6:
		case 7:
		case 0xf:
		case 0x14:
			bmMatrixRotationY(m, AR_F(self, 0x518) * HOP_DEG);
			break;
		case 9:
			bmMatrixRotationX(m, -(AR_F(self, 0x50c) * 5.0f * HOP_DEG));
			break;
		case 10:
			bmMatrixRotationX(m, AR_F(self, 0x50c) * 5.0f * HOP_DEG);
			break;
		case 0xb:
			bmMatrixRotationY(m, AR_F(self, 0x50c) * HOP_DEG);
			break;
		case 0xc:
			bmMatrixRotationY(m, AR_F(self, 0x514) * HOP_DEG);
			break;
		case 0xd:
			bmMatrixRotationY(m, AR_F(self, 0x51c) * HOP_DEG);
			break;
		case 0x10:
			bmMatrixRotationY(m, AR_F(self, 0x50c) * HOP_DEG);
			break;
		default:
			bmMatrixRotationY(m, 0.0f);
			break;
		}
		inst = AR_INST(self, i);
		bmMatrixToQuaternion(HOP_AV(inst, 0xe0), m);
		HOP_AF(inst, 0xc0) = 0.0f;
		HOP_AF(inst, 0xc4) = 0.0f;
		HOP_AF(inst, 0xc8) = 0.0f;
		if (i == 9 || i == 10) {
			HOP_AF(inst, 0xc4) = HOP_AF(inst, 0xc4) + 26.3346f;
			HOP_AF(inst, 0xc8) = HOP_AF(inst, 0xc8) + 211.377f;
		}
		baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), 0, 1);
		baPrepActorInstance(inst, 1);
	}
}

/* 0x00143c60  (gap): the arena per frame (fast: only instance 0) */
void HopArena_Update(void *self, int fast)
{
	switch (hop_Variation) {
	case 1:
		HopArena_Update1(self, fast);
		return;
	case 2:
		HopArena_Update2(self, fast);
		return;
	case 3:
		HopArena_Update3(self, fast);
		return;
	case 4:
		HopArena_Update4(self, fast);
		return;
	}
}

/* 0x00143ca0  (gap): variation 1: which propeller is over (x, z)? 2: the first (blades at
+0x478), 1: the second (+0x474), 0: none (the last of the four blade pairs that matches) */
char HopArena_WhichBlade1(void *self, float x, float z)
{
	float base, f;
	char which = 0;
	int i;

	for (i = 0; i < 4; i++) {
		base = ((unsigned int)i & 1) == 0 ? AR_F(self, 0x478) : AR_F(self, 0x474);
		f = (float)i * 90.0f + base;
		if (hop_InBlade(x, z, f - 20.0f, 14.0f + f, f + 90.0f, 130.0f))
			which = (char)((((unsigned int)i & 1) == 0) + 1);
	}
	return which;
}

/* 0x00143da0  (gap): variation 1: is a player at (x, z) in state `state` hit by a propeller
blade? Players in state 14 pass the first propeller; players in states 3, 4 and 9 (in the
air) pass the second. */
int HopArena_InBlades(void *self, float x, float z, short state)
{
	float base, f;
	unsigned int i;

	for (i = 0; (int)i < 4; i++) {
		base = (i & 1) == 0 ? AR_F(self, 0x478) : AR_F(self, 0x474);
		f = (float)(int)i * 90.0f + base;
		if ((i & 1) == 0) {
			if (state == 0xe)
				continue;
		} else if (state == 3 || state == 4 || state == 9) {
			continue;
		}
		if (hop_InBlade(x, z, f - 12.0f, 10.0f + f, f + 90.0f, 80.0f))
			return 1;
	}
	return 0;
}

/* 0x00143ed0  (gap): variation 1: the dark floor disc (a fan of radius 900 at y -134.78) */
void HopArena_DrawFloor1(void *self)
{
	float *fan = (float *)HOP_ADDR(0x003027e0);
	float *v;
	unsigned int grey;
	float a;
	int i;

	(void)self;
	bdSetTexture(0, NULL);
	bdSetWorldIdentity();
	grey = HOP_ARGB(127, 127, 127, 127);
	fan[0] = 0.0f;
	HOP_AU(fan, 0x4) = 0xc306c831u;
	fan[2] = 0.0f;
	HOP_AU(fan, 0x18) = grey;
	/* 19 ring vertices, 0 to 360 degrees */
	for (i = 0, v = fan + 9; i < 19; i++, v += 9) {
		HOP_AU(v, 0x4) = 0xc306c831u;
		HOP_AU(v, 0x18) = grey;
		a = (float)i * 20.0f * HOP_DEG;
		v[0] = sinf(a) * 900.0f;
		v[2] = cosf(a) * 900.0f;
	}
	bdPushRenderState(5, 0, 0);
	bdDrawPrimitiveFVF(5, fan, 0x14, 0x152);
	bdPopRenderState(5);
}

/* 0x00143fa0  (gap): variation 1: draws the instances (fast: only the ninth, the universe,
and only with "detail"). The propellers (1, 2) scroll a shadow texture; 12 and 13 (the
shadows) are blended; after the platform comes the floor disc. */
void HopArena_Draw1(void *self, int fast)
{
	int i;

	bdSetRenderState(4, 1, 0);
	for (i = 0; i < AR_COUNT(self); i++) {
		if (i == 9 ? hop_Detail == 0.0f : fast != 0)
			continue;
		if (i == 0xc || i == 0xd) {
			bdSetRenderState(5, 0, 0);
			bdSetRenderState(10, 0, 0);
			bdSetRenderState(6, 1, 0);
		}
		if (i == 1 || i == 2) {
			hop_ConstantAlphaStage(0x60000000u);
			AR_F(self, 0x55c) = AR_F(self, 0x474);
			hop_TextureSpin(AR_F(self, 0x474) * HOP_DEG, AR_F(self, 0x008), AR_F(self, 0x00c));
		}
		baDrawActorInstance(AR_INST(self, i), 7);
		if (i == 0) {
			Shadows_BeginReceive();
			HopArena_DrawFloor1(self);
			Shadows_EndReceive();
		} else if (i == 0xc || i == 0xd) {
			bdSetRenderState(5, 1, 0);
			bdSetRenderState(10, 0, 0);
			bdSetRenderState(6, 0, 0);
		}
	}
	bdSetTexture(1, NULL);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	bdSetRenderState(4, 0, 0);
}

/* the light from the camera: direction eye - target */
static void hop_CameraLight(void)
{
	float dir[4];

	dir[0] = HOP_F(0x002aebdc) - HOP_F(0x002aebec);
	dir[3] = 1.0f;
	dir[1] = HOP_F(0x002aebe0) - HOP_F(0x002aebf0);
	dir[2] = HOP_F(0x002aebe4) - HOP_F(0x002aebf4);
	baSetDirectionalLight(HOP_G(0x002aec40, struct BLight *), dir);
}

/* 0x001441c0  (gap): variation 2: draws the instances (fast: only the first, with
"detail"). The platforms fade (brightness = alpha) and show their shake as the edge
texture's frame; a face (35..61) also writes the stencil (the floor reflection of the
antics) and is drawn again in pass 1; the first instance gets a turning texture. */
void HopArena_Draw2(void *self, int fast)
{
	float m[16];
	float s, c;
	int i, k, alpha;

	s = sinf(hop_CamAngle * HOP_DEG);
	memset(m, 0, sizeof m);
	m[5] = 1.0f;
	m[15] = 1.0f;
	c = cosf(hop_CamAngle * HOP_DEG);
	m[0] = c;
	m[2] = -s;
	m[8] = s;
	m[10] = c;
	hop_CameraLight();
	bdSetRenderState(4, 1, 0);
	for (i = 0; i < AR_COUNT(self); i++) {
		if (i == 0 ? hop_Detail == 0.0f : fast != 0)
			continue;
		if ((0x22 < i && i < 0x3e) || (7 < i && i < 0x23)) {
			k = i - (i < 0x23 ? 8 : 0x23);
			alpha = AR_I(self, 0xa0 + k * 4);
			if (alpha == 0)
				continue;
			if (alpha != 0x100) {
				baSetBrightness(alpha);
				bdSetRenderState(6, 1, 0);
				bdSetRenderState(10, 0, 0);
			}
			bdTextureSetFrame(HOP_G(0x002aed68, struct bdTexture *), AR_I(self, 0x34 + k * 4));
		}
		if (i == 0x3e) {
			bdSetRenderState(5, 0, 0);
			bdSetRenderState(10, 1, 0);
			bdSetRenderState(6, 1, 0);
			bdSetRenderState(8, 0, 0);
		} else if (i == 0) {
			hop_ConstantAlphaStage(0x40000000u);
			D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)3, (const D3DMATRIX *)m);
		}
		bdSetTexture(1, NULL);
		if (0x22 < i && i < 0x3e && AR_I(self, 0x14 + i * 4) != 0) {
			D3DDevice_SetRenderState(D3DRS_STENCILENABLE, 1);
			D3DDevice_SetRenderState(D3DRS_STENCILREF, 1);
			D3DDevice_SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
			D3DDevice_SetRenderState(D3DRS_STENCILWRITEMASK, 1);
			D3DDevice_SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
			hop_ConstantAlphaStage((DWORD)(unsigned char)HOP_FTOL((float)AR_I(self, 0x14 + i * 4) * 0.15f) << 24);
			D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)3, (const D3DMATRIX *)m);
		}
		baDrawActorInstance(AR_INST(self, i), 7);
		if (7 < i && i < 0x23 && AR_I(self, 0x80 + i * 4) != 0x100) {
			baSetBrightness(0x100);
			bdSetRenderState(6, 0, 0);
		}
		if (0x22 < i && i < 0x3e) {
			baDrawActorInstance(AR_INST(self, i), 1);
			if (AR_I(self, 0x14 + i * 4) != 0x100) {
				baSetBrightness(0x100);
				bdSetRenderState(6, 0, 0);
			}
			bdSetTexture(1, NULL);
			D3DDevice_SetRenderState(D3DRS_STENCILENABLE, 0);
		}
		if (i == 0) {
			bdSetTexture(1, NULL);
		} else if (i == 0x3e) {
			bdSetRenderState(5, 1, 0);
			bdSetRenderState(10, 0, 0);
			bdSetRenderState(6, 0, 0);
			bdSetRenderState(8, 1, 0);
		}
	}
	bdSetRenderState(4, 0, 0);
}

/* 0x001445d0  (gap): variation 2 after the players (with "detail"): the 8 spot lights;
their beams (odd extra instances) are drawn reflected in the stencilled floor (y scale
-1, dark) and then normally, the beam texture flickering while the clock runs */
void HopArena_DrawExtras2(void *self)
{
	struct BActorInstance *inst;
	int k;

	if (hop_Detail == 0.0f)
		return;
	HOP_I(0x00302ab0) = HOP_I(0x00302ab0) + 1;
	if (2 < HOP_I(0x00302ab0))
		HOP_I(0x00302ab0) = 0;
	for (k = 0; k < AR_XCOUNT(self); k++) {
		if (((unsigned int)k & 1) == 0)
			baDrawActorInstance(AR_XINST(self, k), 1);
	}
	for (k = 0; k < AR_XCOUNT(self); k++) {
		if (0 < hop_TimeLeft)
			bdTextureSetFrame(HOP_G(0x002aed6c, struct bdTexture *), (k / 2 + HOP_I(0x00302ab0)) % 2);
		if (((unsigned int)k & 1) == 0)
			continue;
		inst = AR_XINST(self, k);
		bdSetRenderState(5, 0, 0);
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 1, 0);
		bdSetRenderState(8, 0, 0);
		D3DDevice_SetRenderState(D3DRS_STENCILENABLE, 1);
		D3DDevice_SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		D3DDevice_SetRenderState(D3DRS_STENCILMASK, 1);
		HOP_AF(inst, 0xd4) = -1.0f;
		baSetAmbientLight(0x20, 0x20, 0x20);
		baPrepActorInstance(inst, 1);
		baDrawActorInstance(inst, 4);
		D3DDevice_SetRenderState(D3DRS_STENCILENABLE, 0);
		HOP_AF(inst, 0xd4) = 1.0f;
		baPrepActorInstance(inst, 1);
		baSetAmbientLight(0x30, 0x30, 0x30);
		baDrawActorInstance(inst, 4);
		baSetAmbientLight(0x7f, 0x7f, 0x7f);
		bdSetRenderState(5, 1, 0);
		bdSetRenderState(10, 0, 0);
		bdSetRenderState(6, 0, 0);
		bdSetRenderState(8, 1, 0);
	}
}

/* 0x001447e0  (gap): variation 3: which blade is over (x, z)? For each of the four blades of
the two rotors (+0x510: radius 1000-2000, +0x518: 500-970): 1 inside the narrow sector
(-20..+16 degrees), 2 inside the wide one (+-25), 0 none */
int HopArena_WhichBlade3(void *self, float x, float z)
{
	float f1, f2;
	int i;

	for (i = 0; i < 4; i++) {
		f1 = (float)i * 90.0f + AR_F(self, 0x510);
		if (hop_InSector(x, z, f1 - 20.0f, f1 + 16.0f, 1000.0f, 2000.0f))
			return 1;
		f2 = (float)i * 90.0f + AR_F(self, 0x518);
		if (hop_InSector(x, z, f2 - 20.0f, f2 + 16.0f, 500.0f, 970.0f))
			return 1;
		if (hop_InSector(x, z, f1 - 25.0f, f1 + 25.0f, 1000.0f, 2000.0f))
			return 2;
		if (hop_InSector(x, z, f2 - 25.0f, f2 + 25.0f, 500.0f, 970.0f))
			return 2;
	}
	return 0;
}

/* 0x00144b40  (gap): variation 3: is a player at (x, z) in state `state` hit? 1: by a blade
(players in the air, states 3, 4, 9, pass), or by the core (within 470 of the centre)
while its timer (0x002b001c) is above 64 and the player is not in state 3; 2: within 470
while the core is charged (0x002b0028 == 1); 0: not hit */
int HopArena_InBlades3(void *self, float x, float z, short state)
{
	float f, dist;
	int air = state == 3 || state == 4 || state == 9;
	int i;

	for (i = 0; i < 4; i++) {
		f = (float)i * 90.0f + AR_F(self, 0x510);
		if (!air && hop_InSector(x, z, f - 3.0f, f + 3.0f, 1000.0f, 2000.0f))
			return 1;
		f = (float)i * 90.0f + AR_F(self, 0x518);
		if (!air && hop_InSector(x, z, f - 12.0f, f, 500.0f, 970.0f))
			return 1;
	}
	dist = sqrtf(z * z + x * x);
	if (dist <= 470.0f) {
		if (0x40 < HOP_I(0x002b001c) && state != 3)
			return 1;
		if (dist <= 470.0f && HOP_I(0x002b0028) == 1)
			return 2;
	}
	return 0;
}

/* 0x00144d90  (gap): variation 3: draws the instances (fast: only the eighth, with
"detail"; otherwise the eighth and the seventeenth need "detail"). The glows (17..20) are
blended, 19 and 20 flicker; then the three platforms (shadow receivers). */
void HopArena_Draw3(void *self, int fast)
{
	int i, special;
	unsigned int r;

	bdSetRenderState(4, 1, 0);
	for (i = 0; i < AR_COUNT(self); i++) {
		special = fast != 0 ? i == 8 : (i == 8 || i == 0x11);
		if (special ? hop_Detail == 0.0f : fast != 0)
			continue;
		if (i == 0x12) {
			bdSetRenderState(5, 0, 0);
			bdSetRenderState(10, 1, 0);
			bdSetRenderState(6, 1, 0);
			bdSetRenderState(8, 0, 0);
			baSetAmbientLight(0x20, 0x20, 0x20);
		} else if (i == 0x11) {
			bdSetRenderState(5, 0, 0);
			bdSetRenderState(10, 0, 0);
			bdSetRenderState(6, 1, 0);
			bdSetRenderState(8, 0, 0);
		} else if (i == 0x13 || i == 0x14) {
			bdSetRenderState(5, 0, 0);
			bdSetRenderState(10, 1, 0);
			bdSetRenderState(6, 1, 0);
			bdSetRenderState(8, 0, 0);
			r = (unsigned int)hop_rand() & 0x7f;
			if (DAT_002ecdec != 0)
				r = 0x40;
			baSetAmbientLight((int)r, (int)r, (int)r);
		}
		baDrawActorInstance(AR_INST(self, i), 7);
		if (i == 0x12 || i == 0x11 || i == 0x13 || i == 0x14) {
			bdSetRenderState(5, 1, 0);
			bdSetRenderState(10, 0, 0);
			bdSetRenderState(6, 0, 0);
			bdSetRenderState(8, 1, 0);
			baSetAmbientLight(0x7f, 0x7f, 0x7f);
		}
	}
	Shadows_BeginReceive();
	baDrawActorInstance((struct BActorInstance *)((char *)hop_Platforms[0] + 0x20), 7);
	baDrawActorInstance((struct BActorInstance *)((char *)hop_Platforms[1] + 0x20), 7);
	baDrawActorInstance((struct BActorInstance *)((char *)hop_Platforms[2] + 0x20), 7);
	Shadows_EndReceive();
	bdSetRenderState(8, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(4, 0, 0);
}

/* one electric strip (4 vertices at 0x00302ab8) with each of the three electric textures,
scrolled by the offsets at arena +0x564.. */
static void hop_DrawElectric(void *self)
{
	float *v = (float *)HOP_ADDR(0x00302ab8);
	float *scroll = HOP_AV(self, 0x564);
	int t;

	for (t = 0; t < 3; t++) {
		bdSetTexture(0, HOP_PV(0x002affe0)[t]);
		v[7] = scroll[t];
		v[8] = 0.0f;
		v[0x10] = scroll[t];
		v[0x11] = 1.0f;
		v[0x19] = scroll[t] + 0.5f;
		v[0x1a] = 0.0f;
		v[0x22] = scroll[t] + 0.5f;
		v[0x23] = 1.0f;
		bdDrawPrimitiveFVF(4, v, 4, 0x152);
	}
}

/* the electric strips along the 8 blades of a rotor at `angle` from radius r1 to r2:
the first four stand (y lo..hi), the last four lie (y flat, 35 wide) */
static void hop_DrawBladeSparks(void *self, float angle, float r1, float r2, float lo, float hi, float flat)
{
	float *v = (float *)HOP_ADDR(0x00302ab8);
	float a, s, c, lx = 0.0f, lz = 0.0f;
	unsigned int grey;
	int i;

	for (i = 0; i < 8; i++) {
		a = ((float)i * 90.0f + angle) * HOP_DEG;
		if (3 < i) {
			lx = sinf(a + 1.5707964f) * 35.0f;
			lz = cosf(a + 1.5707964f) * 35.0f;
		}
		s = sinf(a);
		c = cosf(a);
		if (i < 4) {
			v[1] = lo;
			v[10] = hi;
			v[0] = r1 * s;
			v[2] = c * r1;
			v[9] = r1 * s;
			v[11] = c * r1;
			v[0x13] = lo;
			v[0x1c] = hi;
			v[0x12] = s * r2;
			v[0x14] = c * r2;
			v[0x1b] = s * r2;
			v[0x1d] = c * r2;
		} else {
			v[1] = flat;
			v[10] = flat;
			v[0] = r1 * s - lx;
			v[2] = c * r1 - lz;
			v[9] = r1 * s + lx;
			v[11] = c * r1 + lz;
			v[0x13] = flat;
			v[0x1c] = flat;
			v[0x12] = s * r2 - lx;
			v[0x14] = c * r2 - lz;
			v[0x1b] = s * r2 + lx;
			v[0x1d] = c * r2 + lz;
		}
		grey = HOP_ARGB(255, 127, 127, 127);
		HOP_AU(v, 0x18) = grey;
		HOP_AU(v, 0x3c) = grey;
		HOP_AU(v, 0x60) = grey;
		HOP_AU(v, 0x84) = grey;
		hop_DrawElectric(self);
	}
}

/* 0x00144fd0  (gap): variation 3 after the players: electricity. The three textures scroll
(not while paused); 14 panels on the back wall, then strips along the blades of both
rotors. */
void HopArena_DrawExtras3(void *self)
{
	float *v = (float *)HOP_ADDR(0x00302ab8);
	float t, x0, x1;
	unsigned int grey;
	int k;

	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(8, 0, 0);
	bdPushRenderState(4, 0, 0);
	bdSetWorldIdentity();
	if (DAT_002ecdec == 0) {
		t = AR_F(self, 0x564) + 0.003f;
		AR_F(self, 0x564) = t;
		if (0.5f <= t)
			AR_F(self, 0x564) = t - 0.5f;
		t = AR_F(self, 0x568) - 0.007f;
		AR_F(self, 0x568) = t;
		if (t <= 0.0f)
			AR_F(self, 0x568) = t + 0.5f;
		t = AR_F(self, 0x56c) + 0.01f;
		AR_F(self, 0x56c) = t;
		if (0.5f <= t)
			AR_F(self, 0x56c) = t - 0.5f;
	}
	for (k = 0; k < 14; k++) {
		/* columns of panels: x from x0 to x1, 80 apart in y */
		if (k < 3) {
			x0 = -1600.0f;
			x1 = -1000.0f;
			v[1] = -120.0f - (float)k * 80.0f;
			v[10] = -50.0f - (float)k * 80.0f;
		} else if (k < 6) {
			x0 = 1000.0f;
			x1 = 1600.0f;
			t = (float)(k - 3) * 80.0f;
			v[1] = -120.0f - t;
			v[10] = -50.0f - t;
		} else if (k < 8) {
			x0 = -1000.0f;
			x1 = -400.0f;
			v[1] = 150.0f - (float)(k - 6) * 80.0f;
			v[10] = 80.0f - (float)(k - 6) * 80.0f;
		} else if (k < 10) {
			x0 = 400.0f;
			x1 = 1000.0f;
			t = (float)(k - 8) * 80.0f;
			v[1] = 150.0f - t;
			v[10] = 80.0f - t;
		} else if (k < 12) {
			x0 = -400.0f;
			x1 = 0.0f;
			t = (float)(k - 10) * 80.0f;
			v[1] = 370.0f - t;
			v[10] = 300.0f - t;
		} else {
			x0 = 0.0f;
			x1 = 400.0f;
			t = (float)(k - 12) * 80.0f;
			v[1] = 370.0f - t;
			v[10] = 300.0f - t;
		}
		v[0] = x0;
		v[9] = x0;
		v[0x12] = x1;
		v[0x1b] = x1;
		v[2] = 100.0f;
		v[11] = 100.0f;
		v[0x14] = 100.0f;
		grey = HOP_ARGB(255, 64, 64, 64);
		HOP_AU(v, 0x18) = grey;
		v[0x1d] = 100.0f;
		HOP_AU(v, 0x3c) = grey;
		v[0x13] = v[1];
		HOP_AU(v, 0x60) = grey;
		v[0x1c] = v[10];
		HOP_AU(v, 0x84) = grey;
		hop_DrawElectric(self);
	}
	hop_DrawBladeSparks(self, AR_F(self, 0x510), 1080.0f, 1525.0f, -160.0f, -90.0f, -125.0f);
	hop_DrawBladeSparks(self, AR_F(self, 0x518), 540.0f, 900.0f, 100.0f, 170.0f, 135.0f);
	bdSetRenderState(8, 1, 0);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdPopRenderState(4);
}

/* 0x00145920  (gap): variation 4: draws the instances (fast: only the first, with
"detail"); the light colour from the tweaks; the propeller faces (1, 2) unshaded; the
fourth instance (blended) with a texture spinning against the arm */
void HopArena_Draw4(void *self, int fast)
{
	int i;

	bdSetRenderState(4, 1, 0);
	baSetLightColour(HOP_G(0x002aec24, struct BLight *), HOP_FTOL(HOP_F(0x002ae958)), HOP_FTOL(HOP_F(0x002ae95c)),
		HOP_FTOL(HOP_F(0x002ae960)));
	if (DAT_002ecdec == 0)
		AR_F(self, 0x554) = DAT_0028fda8 + DAT_0028fda8 + AR_F(self, 0x554);
	hop_CameraLight();
	for (i = 0; i < AR_COUNT(self); i++) {
		if (i == 0 ? hop_Detail == 0.0f : fast != 0)
			continue;
		if (i == 1 || i == 2)
			hop_ConstantAlphaStage(0);
		if (i == 3) {
			bdSetRenderState(5, 0, 0);
			bdSetRenderState(10, 1, 0);
			bdSetRenderState(6, 1, 0);
			bdSetRenderState(8, 0, 0);
			hop_ConstantAlphaStage(0x60000000u);
			if (hop_PauseState != 0)
				AR_F(self, 0x55c) = (-HOP_F(0x002aed70) - HOP_F(0x002aed74)) + AR_F(self, 0x55c);
			hop_TextureSpin(AR_F(self, 0x55c) * HOP_DEG, 0.0f, 0.0f);
		}
		baDrawActorInstance(AR_INST(self, i), 7);
		if (i == 3) {
			bdSetRenderState(5, 1, 0);
			bdSetRenderState(10, 0, 0);
			bdSetRenderState(6, 0, 0);
			bdSetRenderState(8, 1, 0);
		}
	}
	bdSetRenderState(4, 0, 0);
}

/* 0x00145c30  (gap): draws the arena of the variation (fast: only the special instance) */
void HopArena_Draw(void *self, int fast)
{
	bdSetRenderState(3, 1, 0);
	switch (hop_Variation) {
	case 1:
		HopArena_Draw1(self, fast);
		return;
	case 2:
		HopArena_Draw2(self, fast);
		return;
	case 3:
		HopArena_Draw3(self, fast);
		return;
	case 4:
		HopArena_Draw4(self, fast);
		return;
	}
}
