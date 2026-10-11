/*
ClassLib/CIntroCam.c  (classlib_ui)
Original: C:\BlitzParty\Game\Src\ClassLib\CIntroCam.cpp (tags at 0x0007d540, 0x0007d590,
0x0007d960).

The minigame intro camera: an animation loaded from a parse map ("Camera" block with
MAX_FRAMES, RadianScale and the start keys; "AnimStream" blocks with per-frame keys for
the position, the target, the roll and the field of view) and played back frame by frame
(linear or Bezier interpolation). Functions 0x0007cc30-0x0007e680 are placed here
(inferred from their use of the camera object).

Camera object (0x20 bytes, pointer at 0x002ed218):
  0x00  position keys (circular list, 0x3c-byte nodes)
  0x04  target keys (circular list, 0x3c-byte nodes)
  0x08  roll keys (circular list, 0x10-byte nodes)
  0x0c  field of view keys (circular list, 0x10-byte nodes)
  0x10  frame count (MAX_FRAMES)
  0x14  flags: 1/2/4/8 the first position/target/roll/FOV key is not frame 0,
        0x10 position Bezier, 0x20 target Bezier, 0x40 finished
  0x18  time (frames)
  0x1c  the sound or effect started with the animation (bactor 0x0003bf10)

Vector key (0x3c bytes): frame, position x/y/z/w, tangent in x/y/z/w (0x14), tangent out
x/y/z/w (0x24), -, previous (0x34), next (0x38).
Scalar key (0x10 bytes): frame, value, previous (0x08), next (0x0c).
*/

#include "classlib_ui.h"

#define OV_F(o, off) (*(float *)((o) + (off)))
#define OV_I(o, off) (*(int *)((o) + (off)))
#define OV_U(o, off) (*(unsigned int *)((o) + (off)))

/* ---------- globals (classlib_ui_data.h) */

/* 0x00290cb0  angle scale of the roll and FOV keys (1 / RadianScale) */
#define clu_radian_scale CLU_VAR(float, 0x00290cb0)
/* 0x002ed218  the intro camera */
#define clu_intro_cam CLU_VAR(int, 0x002ed218)

/* vector key fields */
#define KEY_FRAME(k) (*(unsigned int *)(k))
#define KEY_PREV(k) (*(int *)((k) + 0x34))
#define KEY_NEXT(k) (*(int *)((k) + 0x38))
/* scalar key fields */
#define SKEY_VALUE(k) (*(float *)((k) + 4))
#define SKEY_PREV(k) (*(int *)((k) + 8))
#define SKEY_NEXT(k) (*(int *)((k) + 0xc))

/* unsigned 32-bit to float, as the x87 code does it (FILD + 2^32 when negative) */
static double CLIntroCam_UnsignedToDouble(unsigned int value)
{
	double d = (double)(int)value;

	if ((int)value < 0)
		d = d + (double)4.2949673e+09f;
	return d;
}

/* ---------- state */

/* 0x0007cc30  ClassLib/CIntroCam.cpp (inferred)
Restarts the intro camera animation. */
int CLIntroCam_Restart(void)
{
	if (clu_intro_cam == 0)
		return 0;
	OV_F(clu_intro_cam, 0x18) = 0.0f;
	OV_U(clu_intro_cam, 0x14) = OV_U(clu_intro_cam, 0x14) & 0xffffffbf;
	OV_U(clu_intro_cam, 0x1c) = (unsigned int)FUN_0003bf10(0);
	return 1;
}

/* 0x0007cc70  ClassLib/CIntroCam.cpp (inferred)
Current time of the animation (frames). */
float CLIntroCam_GetTime(void)
{
	if (clu_intro_cam == 0)
		return 0.0f;
	return OV_F(clu_intro_cam, 0x18);
}

/* 0x0007cc90  ClassLib/CIntroCam.cpp (inferred)
Stops the animation. */
void CLIntroCam_Stop(void)
{
	if (clu_intro_cam != 0) {
		OV_U(clu_intro_cam, 0x14) = OV_U(clu_intro_cam, 0x14) | 0x40;
		if (OV_I(clu_intro_cam, 0x1c) != 0)
			FUN_0003bf10((struct BCamera *)OV_U(clu_intro_cam, 0x1c));
	}
}

/* ---------- parse map callbacks: one property of an "AnimStream" key per call

Each callback gets a property (name at +8, value at +0x2c) and a key counter; when the
name matches, the value goes to key number *counter (one further when the track's first
key was added at frame 0 by the "Camera" block) and the counter goes up. */

/* walks `count` steps along a key list */
static int CLIntroCam_KeyAt(int head, int count, int next_offset)
{
	int key = head;

	for (; count != 0; count = count + -1)
		key = *(int *)(key + next_offset);
	return key;
}

/* stores a vector key property (track 0 position / 1 target) */
static int CLIntroCam_SetVectorKey(int property, int *counter, const char *name, int track, int field, int set_w)
{
	int count;
	int key;

	if (_stricmp((char *)(property + 8), name) != 0 || OV_I(clu_intro_cam, 0x10) < *counter)
		return 0;
	count = *counter;
	if ((*(unsigned char *)(clu_intro_cam + 0x14) & (track == 0 ? 1 : 2)) != 0)
		count = count + 1;
	key = CLIntroCam_KeyAt(OV_I(clu_intro_cam, track * 4), count, 0x38);
	OV_U(key, field) = OV_U(property, 0x2c);
	if (set_w)
		OV_F(key, field + 4) = 1.0f;
	*counter = *counter + 1;
	return 0;
}

/* 0x0007ccc0  ClassLib/CIntroCam.cpp (inferred) "X" of a position key */
int CLIntroCam_PosX(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "X", 0, 4, 0);
}

/* 0x0007cd10  ClassLib/CIntroCam.cpp (inferred) "Y" of a position key */
int CLIntroCam_PosY(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "Y", 0, 8, 0);
}

/* 0x0007cd60  ClassLib/CIntroCam.cpp (inferred) "Z" of a position key */
int CLIntroCam_PosZ(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "Z", 0, 0xc, 0);
}

/* 0x0007cdb0  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_PosTanInX(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanInX", 0, 0x14, 0);
}

/* 0x0007ce00  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_PosTanInY(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanInY", 0, 0x18, 0);
}

/* 0x0007ce50  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_PosTanInZ(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanInZ", 0, 0x1c, 1);
}

/* 0x0007cea0  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_PosTanOutX(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanOutX", 0, 0x24, 0);
}

/* 0x0007cef0  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_PosTanOutY(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanOutY", 0, 0x28, 0);
}

/* 0x0007cf40  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_PosTanOutZ(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanOutZ", 0, 0x2c, 1);
}

/* 0x0007cf90  ClassLib/CIntroCam.cpp (inferred) "X" of a target key */
int CLIntroCam_TarX(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "X", 1, 4, 0);
}

/* 0x0007cfe0  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_TarY(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "Y", 1, 8, 0);
}

/* 0x0007d030  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_TarZ(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "Z", 1, 0xc, 1);
}

/* 0x0007d090  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_TarTanInX(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanInX", 1, 0x14, 0);
}

/* 0x0007d0e0  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_TarTanInY(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanInY", 1, 0x18, 0);
}

/* 0x0007d130  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_TarTanInZ(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanInZ", 1, 0x1c, 1);
}

/* 0x0007d190  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_TarTanOutX(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanOutX", 1, 0x24, 0);
}

/* 0x0007d1e0  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_TarTanOutY(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanOutY", 1, 0x28, 0);
}

/* 0x0007d230  ClassLib/CIntroCam.cpp (inferred) */
int CLIntroCam_TarTanOutZ(int property, int *counter)
{
	return CLIntroCam_SetVectorKey(property, counter, "TanOutZ", 1, 0x2c, 1);
}

/* stores a scalar key property (track 2 roll / 3 FOV), scaled by the radian scale */
static int CLIntroCam_SetScalarKey(int property, int *counter, const char *name, int track)
{
	int count;
	int key;

	if (_stricmp((char *)(property + 8), name) != 0 || OV_I(clu_intro_cam, 0x10) < *counter)
		return 0;
	count = *counter;
	if ((*(unsigned char *)(clu_intro_cam + 0x14) & (track == 2 ? 4 : 8)) != 0)
		count = count + 1;
	key = CLIntroCam_KeyAt(OV_I(clu_intro_cam, track * 4), count, 0xc);
	SKEY_VALUE(key) = clu_radian_scale * OV_F(property, 0x2c);
	*counter = *counter + 1;
	return 0;
}

/* 0x0007d290  ClassLib/CIntroCam.cpp (inferred) "Roll" of a roll key */
int CLIntroCam_Roll(int property, int *counter)
{
	return CLIntroCam_SetScalarKey(property, counter, "Roll", 2);
}

/* 0x0007d2f0  ClassLib/CIntroCam.cpp (inferred) "FOV" of a field of view key */
int CLIntroCam_Fov(int property, int *counter)
{
	return CLIntroCam_SetScalarKey(property, counter, "FOV", 3);
}

/* 0x0007d350  ClassLib/CIntroCam.cpp (inferred)
Frame count of the animation (0 without a camera). */
int CLIntroCam_GetFrameCount(void)
{
	if (clu_intro_cam == 0)
		return 0;
	return OV_I(clu_intro_cam, 0x10);
}

/* interpolates a scalar key track at time t (linear) */
static int CLIntroCam_EvalScalar(float *out, float t, int track, unsigned int default_bits)
{
	unsigned int frame;
	int key;
	int prev;

	if (clu_intro_cam == 0 || t < 0.0f)
		return 0;
	frame = (unsigned int)(long long)t;
	if (OV_U(clu_intro_cam, 0x10) < frame)
		return 0;
	key = OV_I(clu_intro_cam, track * 4);
	if (key == 0) {
		*(unsigned int *)out = default_bits;
		return 1;
	}
	if (KEY_FRAME(SKEY_PREV(key)) < frame) {
		*out = SKEY_VALUE(SKEY_PREV(key));
		return 1;
	}
	while (KEY_FRAME(key) < frame)
		key = SKEY_NEXT(key);
	if (frame == 0) {
		*out = SKEY_VALUE(key);
		return 1;
	}
	prev = SKEY_PREV(key);
	*out = (float)((((double)t - CLIntroCam_UnsignedToDouble(KEY_FRAME(prev))) *
			((double)SKEY_VALUE(key) - (double)SKEY_VALUE(prev))) /
			       CLIntroCam_UnsignedToDouble(KEY_FRAME(key) - KEY_FRAME(prev)) +
		       (double)SKEY_VALUE(prev));
	return 1;
}

/* 0x0007d360  ClassLib/CIntroCam.cpp (inferred)
Roll at time t (0 without roll keys). Returns 0 when t is outside the animation. */
int CLIntroCam_GetRoll(float *out, float t)
{
	return CLIntroCam_EvalScalar(out, t, 2, 0);
}

/* 0x0007d440  ClassLib/CIntroCam.cpp (inferred)
Field of view at time t (pi/4 without FOV keys). */
int CLIntroCam_GetFov(float *out, float t)
{
	return CLIntroCam_EvalScalar(out, t, 3, 0x3f490fd8); /* 0.7853981f */
}

/* 0x0007d520  ClassLib/CIntroCam.cpp (inferred)
Field of view at the current time. */
void CLIntroCam_GetCurrentFov(float *out)
{
	CLIntroCam_GetFov(out, OV_F(clu_intro_cam, 0x18));
}

/* ---------- creation */

/* 0x0007d540  ClassLib/CIntroCam.cpp
Allocates the intro camera (stopped). */
void CLIntroCam_Create(void)
{
	if (clu_intro_cam != 0) {
		CLU_DebugPrintf("Warning: Intro Cam already has a valid memory address\n");
		return;
	}
	clu_intro_cam = (int)CLU_Alloc(0x20, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CIntroCam.cpp", 0x51, 0x1001);
	memset((void *)clu_intro_cam, 0, 0x20);
	OV_U(clu_intro_cam, 0x14) = OV_U(clu_intro_cam, 0x14) | 0x40;
}

/* frees a circular key list */
static void CLIntroCam_FreeList(int track, int next_offset)
{
	int key;
	int next;

	key = OV_I(clu_intro_cam, track * 4);
	if (key == 0)
		return;
	next = *(int *)(key + next_offset);
	if (next != key) {
		do {
			key = *(int *)(next + next_offset);
			FUN_0004c110((void *)next);
			next = key;
		} while (key != OV_I(clu_intro_cam, track * 4));
	}
	FUN_0004c110((void *)OV_I(clu_intro_cam, track * 4));
	OV_I(clu_intro_cam, track * 4) = 0;
}

/* 0x0007d590  ClassLib/CIntroCam.cpp
Frees the intro camera and its keys. */
void CLIntroCam_Destroy(void)
{
	if (clu_intro_cam == 0)
		return;
	CLIntroCam_FreeList(0, 0x38);
	CLIntroCam_FreeList(1, 0x38);
	CLIntroCam_FreeList(2, 0xc);
	CLIntroCam_FreeList(3, 0xc);
	FUN_0004c110((void *)clu_intro_cam);
	clu_intro_cam = 0;
}

/* allocates a key of `size` bytes as a one-element circular list */
static int CLIntroCam_NewKey(int size, int line, int prev_offset, int next_offset)
{
	int key = (int)CLU_Alloc(size, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CIntroCam.cpp", line, 0x1001);

	*(int *)(key + next_offset) = key;
	*(int *)(key + prev_offset) = key;
	return key;
}

/* 0x0007d6c0  ClassLib/CIntroCam.cpp (inferred); missing from of_functions.tsv
Parse map callback for the "Camera" block: frame count, radian scale and the frame 0
keys of the four tracks. */
int FUN_0007d6c0(int node, undefined4 unused)
{
	int key;

	(void)unused;
	ParseMap_GetInt(node, "MAX_FRAMES", (undefined4 *)(clu_intro_cam + 0x10));
	if (ParseMap_GetFloat(node, "RadianScale", (undefined4 *)CLU_ADDR(0x00290cb0)) == 0)
		clu_radian_scale = 1.0f;
	else
		clu_radian_scale = 1.0f / clu_radian_scale;
	key = CLIntroCam_NewKey(0x3c, 299, 0x34, 0x38);
	OV_I(clu_intro_cam, 0) = key;
	KEY_FRAME(key) = 0;
	ParseMap_GetFloat(node, "PosX", (undefined4 *)(key + 4));
	ParseMap_GetFloat(node, "PosY", (undefined4 *)(key + 8));
	ParseMap_GetFloat(node, "PosZ", (undefined4 *)(key + 0xc));
	OV_F(key, 0x10) = 1.0f;
	key = CLIntroCam_NewKey(0x3c, 0x139, 0x34, 0x38);
	OV_I(clu_intro_cam, 4) = key;
	KEY_FRAME(key) = 0;
	ParseMap_GetFloat(node, "TarPosX", (undefined4 *)(key + 4));
	ParseMap_GetFloat(node, "TarPosY", (undefined4 *)(key + 8));
	ParseMap_GetFloat(node, "TarPosZ", (undefined4 *)(key + 0xc));
	OV_F(key, 0x10) = 1.0f;
	key = CLIntroCam_NewKey(0x10, 0x147, 8, 0xc);
	OV_I(clu_intro_cam, 8) = key;
	KEY_FRAME(key) = 0;
	ParseMap_GetFloat(node, "Roll", (undefined4 *)(key + 4));
	SKEY_VALUE(key) = clu_radian_scale * SKEY_VALUE(key);
	key = CLIntroCam_NewKey(0x10, 0x151, 8, 0xc);
	OV_I(clu_intro_cam, 0xc) = key;
	KEY_FRAME(key) = 0;
	ParseMap_GetFloat(node, "FOV", (undefined4 *)(key + 4));
	SKEY_VALUE(key) = clu_radian_scale * SKEY_VALUE(key);
	if (OV_I(clu_intro_cam, 0x10) == 0)
		CLU_DebugPrintf("ERROR: No MAX_FRAMES property found in camera data.\n");
	return 2;
}

/* adds a key at the frame of a "FrNum" property to a track (flag: the track's first
key is not frame 0) */
static int CLIntroCam_AddKey(int property, int *counter, int track, unsigned int flag, int size, int line,
			     int prev_offset, int next_offset)
{
	int key;
	int head;

	if (_stricmp((char *)(property + 8), "FrNum") != 0 || OV_I(clu_intro_cam, 0x10) < *counter)
		return 0;
	if (*counter == 0) {
		if (OV_I(property, 0x2c) == 0) {
			*counter = 1;
			return 0;
		}
		OV_U(clu_intro_cam, 0x14) = OV_U(clu_intro_cam, 0x14) | flag;
	}
	key = (int)CLU_Alloc(size, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CIntroCam.cpp", line, 0x1001);
	head = OV_I(clu_intro_cam, track * 4);
	if (head == 0) {
		*(int *)(key + next_offset) = key;
		*(int *)(key + prev_offset) = key;
		OV_I(clu_intro_cam, track * 4) = key;
		KEY_FRAME(key) = OV_U(property, 0x2c);
		*counter = *counter + 1;
		return 0;
	}
	*(int *)(key + prev_offset) = *(int *)(head + prev_offset);
	*(int *)(key + next_offset) = OV_I(clu_intro_cam, track * 4);
	*(int *)(*(int *)(OV_I(clu_intro_cam, track * 4) + prev_offset) + next_offset) = key;
	*(int *)(OV_I(clu_intro_cam, track * 4) + prev_offset) = key;
	KEY_FRAME(key) = OV_U(property, 0x2c);
	*counter = *counter + 1;
	return 0;
}

/* 0x0007d890  ClassLib/CIntroCam.cpp (inferred); missing from of_functions.tsv
"FrNum": adds a position key. */
int FUN_0007d890(int property, int *counter)
{
	return CLIntroCam_AddKey(property, counter, 0, 1, 0x3c, 0x1da, 0x34, 0x38);
}

/* 0x0007d960  ClassLib/CIntroCam.cpp
"FrNum": adds a target key. */
int CLIntroCam_AddTargetKey(int property, int *counter)
{
	return CLIntroCam_AddKey(property, counter, 1, 2, 0x3c, 0x2fb, 0x34, 0x38);
}

/* 0x0007da30  ClassLib/CIntroCam.cpp (inferred); missing from of_functions.tsv
"FrNum": adds a roll key. */
int FUN_0007da30(int property, int *counter)
{
	return CLIntroCam_AddKey(property, counter, 2, 4, 0x10, 0x41d, 8, 0xc);
}

/* 0x0007db00  ClassLib/CIntroCam.cpp (inferred); missing from of_functions.tsv
"FrNum": adds a field of view key. */
int FUN_0007db00(int property, int *counter)
{
	return CLIntroCam_AddKey(property, counter, 3, 8, 0x10, 0x464, 8, 0xc);
}

/* ---------- playback */

/* interpolates a vector key track at time t (linear, or Bezier with the tangents) */
static int CLIntroCam_EvalVector(float *out, float t, int track, unsigned int bezier_flag)
{
	float a[4], c1[4], c2[4], r[4], n[4];
	unsigned int frame;
	int key;
	int prev;
	int span;
	float third, u, v;
	double inv, dt;

	if (clu_intro_cam == 0 || t < 0.0f)
		return 0;
	frame = (unsigned int)(long long)t;
	if (OV_U(clu_intro_cam, 0x10) < frame)
		return 0;
	key = OV_I(clu_intro_cam, track * 4);
	prev = KEY_PREV(key);
	if (KEY_FRAME(prev) < frame) {
		out[0] = OV_F(prev, 4);
		out[1] = OV_F(prev, 8);
		out[2] = OV_F(prev, 0xc);
		out[3] = OV_F(prev, 0x10);
		return 1;
	}
	while (KEY_FRAME(key) < frame)
		key = KEY_NEXT(key);
	if (frame == 0) {
		FUN_000127e0((float *)out, (float *)(key + 4));
		return 1;
	}
	prev = KEY_PREV(key);
	if ((OV_U(clu_intro_cam, 0x14) & bezier_flag) != 0) {
		memcpy(a, (void *)(prev + 4), sizeof(a));
		memcpy(n, (void *)(key + 4), sizeof(n));
		span = (int)(KEY_FRAME(key) - KEY_FRAME(prev));
		third = (float)(CLIntroCam_UnsignedToDouble((unsigned int)span) * (double)0.33333334f);
		FUN_0001ea00(c1, (float *)(prev + 0x24), third);
		FUN_0001ea00(c2, (float *)(key + 0x14), third);
		c1[0] = c1[0] + a[0];
		c1[1] = c1[1] + a[1];
		c1[2] = c1[2] + a[2];
		c2[0] = c2[0] + n[0];
		c2[1] = c2[1] + n[1];
		c2[2] = c2[2] + n[2];
		if (span == 0)
			u = 0.0f;
		else
			u = (float)(((double)t - CLIntroCam_UnsignedToDouble(KEY_FRAME(prev))) /
				    (double)(float)CLIntroCam_UnsignedToDouble((unsigned int)span));
		v = 1.0f - u;
		FUN_0001ea00(r, a, v);
		FUN_0001ea00(a, c1, (float)((double)u * (double)3.0f));
		r[0] = a[0] + r[0];
		r[1] = a[1] + r[1];
		r[2] = a[2] + r[2];
		FUN_0001ea00(r, r, v);
		FUN_0001ea00(a, c2, (float)((double)(float)((double)u * (double)u) * (double)3.0f));
		r[0] = a[0] + r[0];
		r[1] = a[1] + r[1];
		r[2] = a[2] + r[2];
		FUN_0001ea00(r, r, v);
		FUN_0001ea00(a, n, (float)((double)(float)((double)u * (double)u) * (double)u));
		FUN_0001e9d0(out, r, a);
		out[3] = 1.0f;
		return 1;
	}
	dt = (double)t - CLIntroCam_UnsignedToDouble(KEY_FRAME(prev));
	inv = 1.0 / CLIntroCam_UnsignedToDouble(KEY_FRAME(key) - KEY_FRAME(prev));
	out[0] = (float)(((double)OV_F(key, 4) - (double)OV_F(prev, 4)) * dt * inv + (double)OV_F(prev, 4));
	out[1] = (float)(((double)OV_F(key, 8) - (double)OV_F(prev, 8)) * dt * inv + (double)OV_F(prev, 8));
	out[3] = 1.0f;
	out[2] = (float)(((double)OV_F(key, 0xc) - (double)OV_F(prev, 0xc)) * dt * inv + (double)OV_F(prev, 0xc));
	return 1;
}

/* 0x0007dbd0  ClassLib/CIntroCam.cpp (inferred)
Camera position at time t. */
int CLIntroCam_GetPosition(float *out, float t)
{
	return CLIntroCam_EvalVector(out, t, 0, 0x10);
}

/* 0x0007df20  ClassLib/CIntroCam.cpp (inferred)
Camera target at time t. */
int CLIntroCam_GetTarget(float *out, float t)
{
	return CLIntroCam_EvalVector(out, t, 1, 0x20);
}

/* 0x0007e270  ClassLib/CIntroCam.cpp (inferred)
Advances the animation by `step` frames and sets the camera (unless flags & 1).
Returns 1 when the animation has ended (or there is none). */
int CLIntroCam_Update(float step, unsigned char flags)
{
	float position[4];
	float target[4];
	float roll;
	float fov;
	float t;

	if (clu_intro_cam == 0 || (OV_U(clu_intro_cam, 0x14) & 0x40) != 0)
		return 1;
	OV_F(clu_intro_cam, 0x18) = step + OV_F(clu_intro_cam, 0x18);
	if (OV_U(clu_intro_cam, 0x10) <= (unsigned int)(long long)((double)OV_F(clu_intro_cam, 0x18) + (double)0.5f)) {
		OV_U(clu_intro_cam, 0x14) = OV_U(clu_intro_cam, 0x14) | 0x40;
		if (OV_I(clu_intro_cam, 0x1c) != 0)
			FUN_0003bf10((struct BCamera *)OV_U(clu_intro_cam, 0x1c));
		return 1;
	}
	if ((flags & 1) == 0) {
		t = OV_F(clu_intro_cam, 0x18);
		roll = t;
		CLIntroCam_GetPosition(position, t);
		CLIntroCam_GetTarget(target, t);
		CLIntroCam_GetRoll(&roll, t);
		CLIntroCam_GetFov(&fov, t);
		FUN_000435f0(position, target, roll);
		FUN_00044a00(fov, 0.0f);
		FUN_00043790();
	}
	return 0;
}

/* 0x0007e340  ClassLib/CIntroCam.cpp (inferred); missing from of_functions.tsv
Parse map callback for an "AnimStream" block: reads the keys of the stream named by
its "Name" property ("position"/"Pos", "TarPos", "Roll" or "FOV"). */
int FUN_0007e340(int node, undefined4 unused)
{
	char *name;
	char *type;
	int bezier;
	int counter;

	(void)unused;
	name = ParseMap_GetString(node, "Name");
	if (_stricmp(name, "position") == 0 || _stricmp(name, "Pos") == 0) {
		type = ParseMap_GetString(node, "Type");
		bezier = type != NULL && _stricmp(type, "Bezier") == 0;
		OV_U(clu_intro_cam, 0x14) = OV_U(clu_intro_cam, 0x14) ^
					    ((bezier << 4 ^ OV_U(clu_intro_cam, 0x14)) & 0x10);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)FUN_0007d890, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_PosX, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_PosY, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_PosZ, (undefined4)&counter);
		if ((*(unsigned char *)(clu_intro_cam + 0x14) & 0x10) != 0) {
			counter = 0;
			ParseMap_ForEachProperty(node, (int)CLIntroCam_PosTanInX, (undefined4)&counter);
			counter = 0;
			ParseMap_ForEachProperty(node, (int)CLIntroCam_PosTanInY, (undefined4)&counter);
			counter = 0;
			ParseMap_ForEachProperty(node, (int)CLIntroCam_PosTanInZ, (undefined4)&counter);
			counter = 0;
			ParseMap_ForEachProperty(node, (int)CLIntroCam_PosTanOutX, (undefined4)&counter);
			counter = 0;
			ParseMap_ForEachProperty(node, (int)CLIntroCam_PosTanOutY, (undefined4)&counter);
			counter = 0;
			ParseMap_ForEachProperty(node, (int)CLIntroCam_PosTanOutZ, (undefined4)&counter);
		}
		return 0;
	}
	if (_stricmp(name, "TarPos") == 0) {
		type = ParseMap_GetString(node, "Type");
		bezier = type != NULL && _stricmp(type, "Bezier") == 0;
		OV_U(clu_intro_cam, 0x14) = OV_U(clu_intro_cam, 0x14) ^
					    ((bezier << 5 ^ OV_U(clu_intro_cam, 0x14)) & 0x20);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_AddTargetKey, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarX, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarY, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarZ, (undefined4)&counter);
		if ((*(unsigned char *)(clu_intro_cam + 0x14) & 0x20) == 0)
			return 0;
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarTanInX, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarTanInY, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarTanInZ, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarTanOutX, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarTanOutY, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_TarTanOutZ, (undefined4)&counter);
		return 0;
	}
	if (_stricmp(name, "Roll") == 0) {
		counter = 0;
		ParseMap_ForEachProperty(node, (int)FUN_0007da30, (undefined4)&counter);
		counter = 0;
		ParseMap_ForEachProperty(node, (int)CLIntroCam_Roll, (undefined4)&counter);
		return 0;
	}
	if (_stricmp(name, "FOV") != 0)
		return 0;
	counter = 0;
	ParseMap_ForEachProperty(node, (int)FUN_0007db00, (undefined4)&counter);
	counter = 0;
	ParseMap_ForEachProperty(node, (int)CLIntroCam_Fov, (undefined4)&counter);
	return 0;
}

/* 0x0007e680  ClassLib/CIntroCam.cpp (inferred)
Loads the intro camera from a parse map and restarts it. */
int CLIntroCam_Load(int map, undefined4 arg)
{
	if (clu_intro_cam == 0)
		CLIntroCam_Create();
	ParseMap_WalkMap(map, (int)FUN_0007d6c0, arg, (int)"Camera");
	ParseMap_WalkMap(map, (int)FUN_0007e340, arg, (int)"AnimStream");
	OV_F(clu_intro_cam, 0x18) = 0.0f;
	OV_U(clu_intro_cam, 0x14) = OV_U(clu_intro_cam, 0x14) & 0xffffffbf;
	OV_U(clu_intro_cam, 0x1c) = (unsigned int)FUN_0003bf10(0);
	return 1;
}
