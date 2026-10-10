/*
CharLib/CCamera.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\CCamera.cpp (tag at 0x000d62c0, source line 1372;
error text of 0x000d6260 names CCamera.cpp).

The CharLib game camera. Modes: 1 follows one actor, 2 plays the queued intro camera
(LOM) files through ClassLib's CIntroCam, 3 is a fixed camera, 5 (the default) zooms and
turns automatically to keep every target on the screen. A camera shake and a camera sway
add to every mode. Targets are 0x48-byte records in a ClassLib list.
*/

#include "charlib.h"
#include "charlib_ai.h"

/* ---------- the camera */

#define CAM_LOM_NAMES 0x000        /* char[10][0x40]: queued intro camera (LOM) files */
#define CAM_LOM_PARAMS 0x280       /* int per queued file (passed to the AI hold slot) */
#define CAM_LOM_COUNT 0x2a8        /* int */
#define CAM_LOM_PACKAGE 0x2ac      /* package of the LOM files */
#define CAM_TARGETS 0x2b4          /* ClassLib list of the targets */
#define CAM_PITCH 0x2c0            /* float: pitch of the mode 5 camera */
#define CAM_BORDER_OUT_X 0x2c4     /* float: screen border (zoom out), x */
#define CAM_BORDER_IN_X 0x2c8      /* float: screen border (zoom in), x */
#define CAM_BORDER_OUT_Y 0x2cc     /* float */
#define CAM_BORDER_IN_Y 0x2d0      /* float */
#define CAM_ZOOM_POW 0x2d4         /* int: apply CAM_ZOOM_EXP */
#define CAM_ZOOM_EXP 0x2d8         /* float */
#define CAM_ZOOM_DEPTH 0x2dc       /* float */
#define CAM_SHAKE_AMP 0x2e0        /* int: shake amplitude */
#define CAM_SHAKE_FRAMES 0x2e4     /* int: shake length */
#define CAM_SHAKE_TIME 0x2e8       /* float: shake time left */
#define CAM_SHAKE_TIMER 0x2ec      /* float: time to the next shake offset */
#define CAM_SHAKE_INTERVAL 0x2f0   /* float */
#define CAM_SWAY 0x2f4             /* float: > 0 sways the camera (roll) */
#define CAM_HEADING 0x304          /* float: heading from the eye to the target */
#define CAM_DISTANCE 0x308         /* float: mode 5 distance from the target */
#define CAM_BOUND_MIN 0x310        /* float[4]: target positions, minimum */
#define CAM_BOUND_MAX 0x320        /* float[4]: target positions, maximum */
#define CAM_SCREEN_MIN 0x330       /* float[4]: projected targets, minimum */
#define CAM_SCREEN_MAX 0x340       /* float[4]: projected targets, maximum */
#define CAM_SCREEN_QUAD 0x3d0      /* 4 x float[4]: corners of the projected box (x, y) */
#define CAM_VIEW_MIN 0x410         /* float[4]: targets in view space, minimum */
#define CAM_VIEW_MAX 0x420         /* float[4]: targets in view space, maximum */
#define CAM_PIVOT 0x4b0            /* float[4]: mode 5 pivot (moves towards CAM_PIVOT_GOAL) */
#define CAM_PIVOT_GOAL 0x4c0       /* float[4] */
#define CAM_PIVOT_SPEED 0x4d0      /* float */
#define CAM_EYE_WANTED 0x4d4       /* float[4] */
#define CAM_EYE 0x4e4              /* float[4] */
#define CAM_TARGET 0x504           /* float[4] */
#define CAM_SAVED_EYE 0x514        /* float[4]: eye and target before the intro */
#define CAM_SAVED_TARGET 0x524     /* float[4] */
#define CAM_FINAL_EYE 0x534        /* float[4]: eye and target with the shake */
#define CAM_FINAL_TARGET 0x544     /* float[4] */
#define CAM_SHAKE_OFFSET 0x554     /* float[4] */
#define CAM_MODE 0x568             /* int */
#define CAM_PREV_MODE 0x56c        /* int: mode before the intro (-1: none) */
#define CAM_FOLLOW 0x570           /* the actor of mode 1 */

/* ---------- a target (0x48 bytes) */

#define TGT_LINK 0x00              /* ClassLib list link (next, prev) */
#define TGT_POS 0x08               /* float *: position */
#define TGT_VEL 0x0c               /* float *: velocity */
#define TGT_PREDICT 0x10           /* float[4]: predicted position */
#define TGT_SCREEN 0x20            /* float[4]: projected position */
#define TGT_VIEW 0x30              /* float[4]: position in view space */
#define TGT_PARAM 0x40             /* int */
#define TGT_LIST 0x44              /* the list that holds the target */

/* ---------- data (charlib_data.c) */

#define cam_auto_target_rate CHL_VAR(float, 0x00294b4c)   /* 0.05 */
#define cam_auto_eye_rate CHL_VAR(float, 0x00294b50)      /* 0.1 */
#define cam_auto_max_distance CHL_VAR(float, 0x00294b54)  /* 2400 */
#define cam_auto_zoom_out_rate CHL_VAR(float, 0x00294b58) /* 0.7 */
#define cam_auto_zoom_in_rate CHL_VAR(float, 0x00294b5c)  /* 0.7 */
#define cam_auto_target_lift CHL_VAR(float, 0x00294b60)   /* -120 */
#define cam_auto_zoom_scale CHL_VAR(float, 0x00294b68)
#define cam_auto_zoom_in_step CHL_VAR(float, 0x00294b6c)
#define cam_auto_zoom_out_step CHL_VAR(float, 0x00294b70)
#define cam_auto_zoom_min CHL_VAR(float, 0x00294b74)
#define cam_auto_zoom_max CHL_VAR(float, 0x00294b78)
#define cam_shake_interval CHL_VAR(float, 0x00294b94)     /* 2.0 */
#define cam_auto_eye_start CHL_ARR(float, 0x00294b98)     /* float[4] */
#define cam_auto_pivot_start CHL_ARR(float, 0x00294ba8)   /* float[4] */
#define cam_auto_pitch CHL_VAR(float, 0x00294bb8)         /* -0.5 */
#define cam_auto_zoom_method CHL_VAR(int, 0x00294bbc)     /* 3: 0, 1 and 2 are older methods */
#define cam_auto_eye_step CHL_VAR(float, 0x00294bc0)      /* 1000 */
#define cam_sway_min CHL_VAR(float, 0x00294bc4)           /* -pi/4 */
#define cam_sway_max CHL_VAR(float, 0x00294bc8)           /* pi/4 */
#define cam_sway_speed CHL_VAR(float, 0x00294bcc)
#define cam_auto_zoom CHL_VAR(float, 0x002f7a28)          /* zoom speed of mode 5 */
#define cam_heading CHL_VAR(float, 0x002f7a2c)
#define cam_predict_time CHL_VAR(float, 0x002f7a38)
#define cam_sway_angle CHL_VAR(float, 0x002f7a3c)
#define cam_last_cheer CHL_VAR(int, 0x002f7a40)

/* foreign data (tentative definitions in charlib_extern.c) */
extern float DAT_002be430[16]; /* bdisplay: world * view matrix */
extern int DAT_002ecdec;       /* classlib_ui/core: the game is paused */
extern int DAT_003055e0;       /* bdisplay/core: skip request */
extern int DAT_002baed4[4 * 10]; /* core: player table, 0x28 bytes per player */

/* 0x00253fdc */
static const char cam_cheer_format[] = "gencheer%02d.wav";

/* 0x0028fdd8 (bkernel data): the z axis (0, 0, 1, 1) */
static float cam_axis_z[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

/* 0x001d5950: the ClassLib list destructor (thiscall list; the generated prototype has no
argument) */
#define cam_ListDtor ((void (*)(void *))FUN_001d5950)
/* classlib_ui / bkernel calls whose generated prototypes are incomplete (owners' names) */
#define cam_LoadFile ((void *(*)(void *, const char *, void *, unsigned int *, unsigned int *, int))FUN_0004c6b0)
#define cam_ParseMapLoad ((int *(*)(void *, int, unsigned int))FUN_0007f2f0)
#define cam_IntroCamLoad ((int (*)(int *, void *))FUN_0007e680)

static void cam_copy4(void *cam, int to, int from)
{
	CHL_F32(cam, to + 0x0) = CHL_F32(cam, from + 0x0);
	CHL_F32(cam, to + 0x4) = CHL_F32(cam, from + 0x4);
	CHL_F32(cam, to + 0x8) = CHL_F32(cam, from + 0x8);
	CHL_F32(cam, to + 0xc) = CHL_F32(cam, from + 0xc);
}

/* 0x000d58d0  CharLib/CCamera.cpp
Resets the zoom parameters, the sway and the shake. */
void CCamera_Reset(void *cam)
{
	CHL_I32(cam, 0x2b0) = 0;
	CHL_I32(cam, 0x30c) = 0;
	CHL_I32(cam, CAM_ZOOM_POW) = 1;
	CHL_F32(cam, CAM_ZOOM_EXP) = 1.6f;
	CHL_F32(cam, CAM_ZOOM_DEPTH) = 2200.0f;
	CHL_U32(cam, CAM_BORDER_OUT_X) = 0x43130000; /* 147 */
	CHL_U32(cam, CAM_BORDER_IN_X) = 0x437bffff;  /* 252 */
	CHL_U32(cam, CAM_BORDER_OUT_Y) = 0x42d1ffff; /* 105 */
	CHL_U32(cam, CAM_BORDER_IN_Y) = 0x433cffff;  /* 189 */
	CHL_I32(cam, CAM_SWAY) = 0;
	CHL_I32(cam, 0x2fc) = 0;
	CHL_I32(cam, 0x300) = 0;
	CHL_F32(cam, CAM_SHAKE_OFFSET + 0x0) = 0.0f;
	CHL_F32(cam, CAM_SHAKE_OFFSET + 0x4) = 0.0f;
	CHL_F32(cam, CAM_SHAKE_OFFSET + 0x8) = 0.0f;
	CHL_F32(cam, CAM_SHAKE_OFFSET + 0xc) = 1.0f;
	CHL_I32(cam, CAM_SHAKE_FRAMES) = 0;
	CHL_I32(cam, CAM_SHAKE_AMP) = 0;
	CHL_F32(cam, CAM_SHAKE_TIME) = 0.0f;
}

/* 0x000d5970  CharLib/CCamera.cpp
Starts a camera shake of amplitude amp for frames frames. */
void CCamera_Shake(void *cam, int amp, int frames)
{
	CHL_I32(cam, CAM_SHAKE_AMP) = amp;
	CHL_F32(cam, CAM_SHAKE_TIME) = (float)frames;
	CHL_I32(cam, CAM_SHAKE_FRAMES) = frames;
	CHL_F32(cam, CAM_SHAKE_INTERVAL) = cam_shake_interval;
	CHL_F32(cam, CAM_SHAKE_TIMER) = 0.0f;
}

/* 0x000d59b0  CharLib/CCamera.cpp
Sways the camera: the roll swings between cam_sway_min and cam_sway_max (the speed turns
round 15 degrees before each end). */
void CCamera_Sway(void *cam)
{
	cam_sway_angle = cam_sway_speed + cam_sway_angle;
	if (cam_sway_angle > cam_sway_max - 0.2617993950843811f)
		cam_sway_speed = cam_sway_speed - 0.0013962633674964309f;
	else if (cam_sway_angle < cam_sway_min + 0.2617993950843811f)
		cam_sway_speed = cam_sway_speed + 0.0013962633674964309f;
	FUN_000435f0(CHL_FIELD(float, cam, CAM_EYE), CHL_FIELD(float, cam, CAM_TARGET), cam_sway_angle);
}

/* 0x000d5a40  CharLib/CCamera.cpp
Box around the target positions (CAM_BOUND_MIN, CAM_BOUND_MAX; x, y, z). */
void CCamera_TargetBounds(void *cam)
{
	void *list = CHL_FIELD(void, cam, CAM_TARGETS);
	int first = 1;
	void *t;

	for (t = chl_ListNext(list, NULL); t != NULL; t = chl_ListNext(list, t)) {
		float *p = (float *)CHL_PTR(t, TGT_POS);

		if (first) {
			first = 0;
			CHL_F32(cam, CAM_BOUND_MIN + 0x0) = CHL_F32(cam, CAM_BOUND_MAX + 0x0) = p[0];
			CHL_F32(cam, CAM_BOUND_MIN + 0x4) = CHL_F32(cam, CAM_BOUND_MAX + 0x4) = p[1];
			CHL_F32(cam, CAM_BOUND_MIN + 0x8) = CHL_F32(cam, CAM_BOUND_MAX + 0x8) = p[2];
		} else {
			if (CHL_F32(cam, CAM_BOUND_MAX + 0x0) < p[0])
				CHL_F32(cam, CAM_BOUND_MAX + 0x0) = p[0];
			if (p[0] < CHL_F32(cam, CAM_BOUND_MIN + 0x0))
				CHL_F32(cam, CAM_BOUND_MIN + 0x0) = p[0];
			if (CHL_F32(cam, CAM_BOUND_MAX + 0x4) < p[1])
				CHL_F32(cam, CAM_BOUND_MAX + 0x4) = p[1];
			if (p[1] < CHL_F32(cam, CAM_BOUND_MIN + 0x4))
				CHL_F32(cam, CAM_BOUND_MIN + 0x4) = p[1];
			if (CHL_F32(cam, CAM_BOUND_MAX + 0x8) < p[2])
				CHL_F32(cam, CAM_BOUND_MAX + 0x8) = p[2];
			if (p[2] < CHL_F32(cam, CAM_BOUND_MIN + 0x8))
				CHL_F32(cam, CAM_BOUND_MIN + 0x8) = p[2];
		}
	}
}

/* 0x000d5b60  CharLib/CCamera.cpp
Fixed camera (mode 3) at CAM_FINAL_EYE looking at CAM_FINAL_TARGET. */
void CCamera_SetStatic(void *cam)
{
	CHL_I32(cam, CAM_MODE) = 3;
	FUN_000435f0(CHL_FIELD(float, cam, CAM_FINAL_EYE), CHL_FIELD(float, cam, CAM_FINAL_TARGET), 0.0f);
	FUN_00043790();
}

/* 0x000d5bb0  CharLib/CCamera.cpp
Removes a target from its list. */
void CameraTarget_Remove(void *target)
{
	if (CHL_PTR(target, TGT_LIST) != NULL)
		chl_ListRemove(CHL_PTR(target, TGT_LIST), target);
}

/* 0x000d5bc0  CharLib/CCamera.cpp
Camera shake: every CAM_SHAKE_INTERVAL a new random offset of up to the current amplitude
(which falls to 0 over the shake); the offset is cleared when the shake ends. */
void CCamera_UpdateShake(void *cam)
{
	int amp = CHL_FTOL((double)CHL_I32(cam, CAM_SHAKE_AMP) * (double)CHL_F32(cam, CAM_SHAKE_TIME) /
		(double)CHL_I32(cam, CAM_SHAKE_FRAMES));
	int range;

	CHL_F32(cam, CAM_SHAKE_TIMER) = CHL_F32(cam, CAM_SHAKE_TIMER) - chl_time_scale;
	if (CHL_F32(cam, CAM_SHAKE_TIMER) <= 0.0f) {
		range = amp * 2 + 1;
		CHL_F32(cam, CAM_SHAKE_OFFSET + 0x0) = (float)(CHL_Rand() % range) - (float)amp;
		CHL_F32(cam, CAM_SHAKE_OFFSET + 0x4) = (float)(CHL_Rand() % range) - (float)amp;
		CHL_F32(cam, CAM_SHAKE_OFFSET + 0x8) = (float)(CHL_Rand() % range) - (float)amp;
		CHL_F32(cam, CAM_SHAKE_TIMER) = CHL_F32(cam, CAM_SHAKE_INTERVAL);
	}
	CHL_F32(cam, CAM_SHAKE_TIME) = CHL_F32(cam, CAM_SHAKE_TIME) - chl_time_scale;
	if (CHL_F32(cam, CAM_SHAKE_TIME) <= 0.0f) {
		CHL_F32(cam, CAM_SHAKE_TIME) = 0.0f;
		CHL_F32(cam, CAM_SHAKE_OFFSET + 0x0) = 0.0f;
		CHL_F32(cam, CAM_SHAKE_OFFSET + 0x4) = 0.0f;
		CHL_F32(cam, CAM_SHAKE_OFFSET + 0x8) = 0.0f;
		CHL_F32(cam, CAM_SHAKE_OFFSET + 0xc) = 1.0f;
	}
}

/* 0x000d5d00  CharLib/CCamera.cpp
Box around the projected targets (CAM_SCREEN_MIN, CAM_SCREEN_MAX; 4 components) and its
corners in CAM_SCREEN_QUAD. */
void CCamera_ScreenBounds(void *cam)
{
	void *list = CHL_FIELD(void, cam, CAM_TARGETS);
	int first = 1;
	int i;
	void *t;

	for (t = chl_ListNext(list, NULL); t != NULL; t = chl_ListNext(list, t)) {
		if (first) {
			for (i = 0; i < 0x10; i += 4) {
				CHL_F32(cam, CAM_SCREEN_MIN + i) = CHL_F32(t, TGT_SCREEN + i);
			}
			for (i = 0; i < 0x10; i += 4) {
				CHL_F32(cam, CAM_SCREEN_MAX + i) = CHL_F32(t, TGT_SCREEN + i);
			}
			first = 0;
		} else {
			for (i = 0; i < 0x10; i += 4) {
				if (CHL_F32(cam, CAM_SCREEN_MAX + i) < CHL_F32(t, TGT_SCREEN + i))
					CHL_F32(cam, CAM_SCREEN_MAX + i) = CHL_F32(t, TGT_SCREEN + i);
				if (CHL_F32(t, TGT_SCREEN + i) < CHL_F32(cam, CAM_SCREEN_MIN + i))
					CHL_F32(cam, CAM_SCREEN_MIN + i) = CHL_F32(t, TGT_SCREEN + i);
			}
		}
	}
	CHL_F32(cam, CAM_SCREEN_QUAD + 0x00) = CHL_F32(cam, CAM_SCREEN_MIN + 0x0);
	CHL_F32(cam, CAM_SCREEN_QUAD + 0x04) = CHL_F32(cam, CAM_SCREEN_MIN + 0x4);
	CHL_F32(cam, CAM_SCREEN_QUAD + 0x10) = CHL_F32(cam, CAM_SCREEN_MAX + 0x0);
	CHL_F32(cam, CAM_SCREEN_QUAD + 0x14) = CHL_F32(cam, CAM_SCREEN_MIN + 0x4);
	CHL_F32(cam, CAM_SCREEN_QUAD + 0x20) = CHL_F32(cam, CAM_SCREEN_MAX + 0x0);
	CHL_F32(cam, CAM_SCREEN_QUAD + 0x24) = CHL_F32(cam, CAM_SCREEN_MAX + 0x4);
	CHL_F32(cam, CAM_SCREEN_QUAD + 0x30) = CHL_F32(cam, CAM_SCREEN_MIN + 0x0);
	CHL_F32(cam, CAM_SCREEN_QUAD + 0x34) = CHL_F32(cam, CAM_SCREEN_MAX + 0x4);
}

/* 0x000d5eb0  CharLib/CCamera.cpp
Box around the targets in view space (CAM_VIEW_MIN, CAM_VIEW_MAX; the first target sets
4 components, the others widen x, y, z). */
void CCamera_ViewBounds(void *cam)
{
	void *list = CHL_FIELD(void, cam, CAM_TARGETS);
	int first = 1;
	int i;
	void *t;

	for (t = chl_ListNext(list, NULL); t != NULL; t = chl_ListNext(list, t)) {
		if (first) {
			for (i = 0; i < 0x10; i += 4) {
				CHL_F32(cam, CAM_VIEW_MIN + i) = CHL_F32(t, TGT_VIEW + i);
			}
			for (i = 0; i < 0x10; i += 4) {
				CHL_F32(cam, CAM_VIEW_MAX + i) = CHL_F32(t, TGT_VIEW + i);
			}
			first = 0;
		} else {
			for (i = 0; i < 0xc; i += 4) {
				if (CHL_F32(cam, CAM_VIEW_MAX + i) < CHL_F32(t, TGT_VIEW + i))
					CHL_F32(cam, CAM_VIEW_MAX + i) = CHL_F32(t, TGT_VIEW + i);
				if (CHL_F32(t, TGT_VIEW + i) < CHL_F32(cam, CAM_VIEW_MIN + i))
					CHL_F32(cam, CAM_VIEW_MIN + i) = CHL_F32(t, TGT_VIEW + i);
			}
		}
	}
}

/* 0x000d5fe0  CharLib/CCamera.cpp
Mode 1: the eye moves 20% of the way to 400 behind and 400 above the followed actor (turned
by its heading); the camera looks at the actor, 160 above its feet. */
void CCamera_UpdateFollow(void *cam)
{
	float off[4];
	float want[3];
	void *actor;

	off[0] = 0.0f;
	off[1] = 400.0f;
	off[2] = 400.0f;
	off[3] = 1.0f;
	FUN_00051170(off, off, CHL_F32(CHL_PTR(cam, CAM_FOLLOW), 0x1e8));
	actor = CHL_PTR(cam, CAM_FOLLOW);
	want[0] = off[0] + CHL_F32(actor, 0xd0);
	want[1] = off[1] + CHL_F32(actor, 0xd4);
	want[2] = off[2] + CHL_F32(actor, 0xd8);
	FUN_00050ff0(CHL_FIELD(float, cam, CAM_EYE), CHL_FIELD(float, cam, CAM_EYE), 0.8f, want, 0.2f);
	actor = CHL_PTR(cam, CAM_FOLLOW);
	CHL_F32(cam, CAM_TARGET + 0x0) = CHL_F32(actor, 0xd0);
	CHL_F32(cam, CAM_TARGET + 0x4) = CHL_F32(actor, 0xd4);
	CHL_F32(cam, CAM_TARGET + 0x8) = CHL_F32(actor, 0xd8);
	CHL_F32(cam, CAM_TARGET + 0xc) = CHL_F32(actor, 0xdc);
	CHL_F32(cam, CAM_TARGET + 0x4) = CHL_F32(cam, CAM_TARGET + 0x4) + 160.0f;
	FUN_000435f0(CHL_FIELD(float, cam, CAM_EYE), CHL_FIELD(float, cam, CAM_TARGET), 0.0f);
	FUN_00043790();
}

/* 0x000d60d0  CharLib/CCamera.cpp
Resets the automatic camera (mode 5): rates, start eye and pivot, target at the origin. */
void CCamera_ResetAuto(void *cam)
{
	int i;

	cam_auto_target_rate = 0.05f;
	cam_auto_eye_rate = 0.1f;
	cam_auto_zoom_out_rate = 0.7f;
	cam_auto_zoom_in_rate = 0.7f;
	for (i = 0; i < 4; i++) {
		CHL_F32(cam, CAM_EYE + i * 4) = cam_auto_eye_start[i];
	}
	cam_copy4(cam, CAM_FINAL_EYE, CAM_EYE);
	cam_auto_max_distance = 2400.0f;
	cam_auto_zoom = 0.0f;
	for (i = 0; i < 4; i++) {
		CHL_F32(cam, CAM_PIVOT + i * 4) = cam_auto_pivot_start[i];
	}
	cam_copy4(cam, CAM_PIVOT_GOAL, CAM_PIVOT);
	CHL_F32(cam, CAM_PIVOT_SPEED) = 0.0f;
	CHL_F32(cam, CAM_FINAL_TARGET + 0x0) = 0.0f;
	CHL_F32(cam, CAM_FINAL_TARGET + 0x4) = 0.0f;
	CHL_F32(cam, CAM_FINAL_TARGET + 0x8) = 0.0f;
	CHL_F32(cam, CAM_FINAL_TARGET + 0xc) = 1.0f;
	CHL_F32(cam, CAM_EYE_WANTED + 0x0) = 0.0f;
	CHL_F32(cam, CAM_EYE_WANTED + 0x4) = 0.0f;
	CHL_F32(cam, CAM_EYE_WANTED + 0x8) = 0.0f;
	CHL_F32(cam, CAM_EYE_WANTED + 0xc) = 1.0f;
	CHL_F32(cam, CAM_TARGET + 0x0) = 0.0f;
	CHL_F32(cam, CAM_TARGET + 0x4) = 0.0f;
	CHL_F32(cam, CAM_TARGET + 0x8) = 0.0f;
	CHL_F32(cam, CAM_TARGET + 0xc) = 1.0f;
	FUN_000435f0(CHL_FIELD(float, cam, CAM_EYE), CHL_FIELD(float, cam, CAM_TARGET), 0.0f);
	FUN_00043790();
	CHL_F32(cam, CAM_PITCH) = cam_auto_pitch;
	CHL_F32(cam, CAM_DISTANCE) =
		FUN_00051500(CHL_FIELD(float, cam, CAM_TARGET), CHL_FIELD(float, cam, CAM_EYE));
}

/* 0x000d6260  CharLib/CCamera.cpp
ResetCameraMode(): sets the mode; mode 5 resets the automatic camera; a bad mode becomes 5
(without the reset). */
void CCamera_SetMode(void *cam, int mode)
{
	CHL_I32(cam, CAM_MODE) = mode;
	switch (mode) {
	case 1:
	case 2:
	case 3:
	case 4:
		break;
	case 5:
		CCamera_ResetAuto(cam);
		return;
	default:
		chl_Printf("ERROR CCamera.cpp, ResetCameraMode(): bad mode %d\n", mode);
		CHL_I32(cam, CAM_MODE) = 5;
	}
}

/* 0x000d62c0  CharLib/CCamera.cpp
Adds a target that follows pos (with velocity vel for the prediction). As in the original,
the list insert and the field writes after it run even when the allocation failed. */
void *CCamera_AddTarget(void *cam, float *pos, int param, float *vel)
{
	void *t = chl_Alloc(0x48, "C:\\BlitzParty\\Game\\Src\\CharLib\\CCamera.cpp", 0x55c, 0x1001);

	if (t == NULL) {
		t = NULL;
	} else {
		CHL_PTR(t, TGT_POS) = pos;
		CHL_PTR(t, TGT_VEL) = vel;
		CHL_PTR(t, TGT_LIST) = NULL;
	}
	chl_ListInsert(CHL_FIELD(void, cam, CAM_TARGETS), t, 0);
	CHL_PTR(t, TGT_LIST) = CHL_FIELD(void, cam, CAM_TARGETS);
	CHL_I32(t, TGT_PARAM) = param;
	return t;
}

/* 0x000d6320  CharLib/CCamera.cpp
Predicted target position: x and z move by cam_predict_time * velocity, but only away from
0 (a coordinate at exactly 0 keeps its old prediction; z at 0 returns early); y is the
position. */
void CameraTarget_Predict(void *target)
{
	float *pos = (float *)CHL_PTR(target, TGT_POS);
	float *vel = (float *)CHL_PTR(target, TGT_VEL);
	float x = cam_predict_time * vel[0] + pos[0];
	float z = cam_predict_time * vel[2] + pos[2];

	if (pos[0] > 0.0f) {
		if (x <= pos[0])
			x = pos[0];
		CHL_F32(target, TGT_PREDICT + 0x0) = x;
	} else if (pos[0] < 0.0f) {
		if (pos[0] <= x)
			x = pos[0];
		CHL_F32(target, TGT_PREDICT + 0x0) = x;
	}
	CHL_F32(target, TGT_PREDICT + 0x4) = pos[1];
	if (pos[2] > 0.0f) {
		if (z <= pos[2]) {
			CHL_F32(target, TGT_PREDICT + 0x8) = pos[2];
			return;
		}
	} else {
		if (pos[2] >= 0.0f)
			return;
		if (pos[2] <= z)
			z = pos[2];
	}
	CHL_F32(target, TGT_PREDICT + 0x8) = z;
}

/* 0x000d63f0  CharLib/CCamera.cpp
Constructor. */
void *CCamera_Ctor(void *cam, int mode)
{
	FUN_00071050(CHL_FIELD(int, cam, CAM_TARGETS));
	CHL_I32(cam, CAM_LOM_COUNT) = 0;
	CHL_I32(cam, CAM_PREV_MODE) = -1;
	CCamera_Reset(cam);
	CHL_I32(cam, CAM_MODE) = mode;
	switch (mode) {
	case 3:
		CCamera_SetStatic(cam);
		return cam;
	case 5:
		CCamera_ResetAuto(cam);
	}
	return cam;
}

/* 0x000d64a0  CharLib/CCamera.cpp
Destructor: frees the targets and stops an intro camera. */
void CCamera_Dtor(void *cam)
{
	void *list = CHL_FIELD(void, cam, CAM_TARGETS);
	void *t = chl_ListNext(list, NULL);

	while (t != NULL) {
		void *prev = CHL_PTR(t, TGT_LINK + 4);

		if (CHL_PTR(t, TGT_LIST) != NULL)
			chl_ListRemove(CHL_PTR(t, TGT_LIST), t);
		chl_Free(t);
		t = chl_ListNext(list, prev);
	}
	if (CHL_I32(cam, CAM_MODE) == 2) {
		FUN_0007cc90();
		FUN_0007d590();
	}
	cam_ListDtor(list);
}

/* 0x000d6540  CharLib/CCamera.cpp
Mode 5: projects the targets, moves the target point and the eye towards the targets'
centre and zooms so that every target stays between the two screen borders. The jumps
follow the original's control flow. */
void CCamera_UpdateAuto(void *cam)
{
	void *list = CHL_FIELD(void, cam, CAM_TARGETS);
	float *eye = CHL_FIELD(float, cam, CAM_EYE);
	float *pivot;
	float near_out = 1e10f;  /* smallest distance of a target inside the outer border */
	float near_in = 1000.0f; /* smallest distance of a target inside the inner border */
	float p[4], centre[4], look[4], dir[4], unused[4], world[4], axis[4], inv[16];
	float ox_lo, ox_hi, oy_lo, oy_hi, ix_lo, ix_hi, iy_lo, iy_hi;
	float f, step;
	int outside, inside;
	void *t;

	FUN_00043b70(1);
	FUN_000435f0(eye, CHL_FIELD(float, cam, CAM_TARGET), 0.0f);
	FUN_00043790();
	CCamera_TargetBounds(cam);
	FUN_000438b0();
	for (t = chl_ListNext(list, NULL); t != NULL; t = chl_ListNext(list, t)) {
		CameraTarget_Predict(t);
		CHL_F32(t, TGT_PREDICT + 0xc) = 1.0f;
		p[0] = CHL_F32(t, TGT_PREDICT + 0x0);
		p[1] = cam_auto_target_lift + CHL_F32(t, TGT_PREDICT + 0x4);
		p[2] = CHL_F32(t, TGT_PREDICT + 0x8);
		p[3] = CHL_F32(t, TGT_PREDICT + 0xc);
		FUN_000505c0(CHL_FIELD(float, t, TGT_VIEW), DAT_002be430, p);
		f = 1.0f / CHL_F32(t, TGT_VIEW + 0x8);
		CHL_F32(t, TGT_VIEW + 0x0) = f * CHL_F32(t, TGT_VIEW + 0x0);
		CHL_F32(t, TGT_VIEW + 0x4) = f * CHL_F32(t, TGT_VIEW + 0x4);
		FUN_00043c10(CHL_FIELD(float, t, TGT_SCREEN), p, 1);
	}
	CCamera_ViewBounds(cam);
	CCamera_ScreenBounds(cam);

	/* centre of the targets in view space, back to world space */
	FUN_00050ff0(centre, CHL_FIELD(float, cam, CAM_VIEW_MIN), 0.5f, CHL_FIELD(float, cam, CAM_VIEW_MAX), 0.5f);
	centre[0] = centre[0] * centre[2];
	centre[3] = 1.0f;
	centre[1] = centre[1] * centre[2];
	cam_auto_zoom_scale = CHL_F32(cam, CAM_ZOOM_DEPTH) / centre[2];
	if (cam_auto_zoom_scale > 0.0f || isnan(cam_auto_zoom_scale)) {
		if (CHL_I32(cam, CAM_ZOOM_POW) != 0)
			cam_auto_zoom_scale = (float)pow(cam_auto_zoom_scale, CHL_F32(cam, CAM_ZOOM_EXP));
	} else {
		cam_auto_zoom_scale = 0.0f;
	}
	FUN_000502f0(inv, DAT_002be430);
	FUN_000505c0(world, inv, centre);
	FUN_000505c0(unused, DAT_002be430, world);
	dir[0] = world[0] - eye[0];
	dir[1] = world[1] - eye[1];
	dir[2] = world[2] - eye[2];
	FUN_000514a0(dir, dir);
	f = CHL_F32(cam, CAM_BOUND_MAX + 0x4) - eye[1];
	if (dir[1] != 0.0f)
		f = f / dir[1];
	FUN_00050ff0(look, dir, f, eye, 1.0f);
	look[3] = 1.0f;
	FUN_000505c0(unused, DAT_002be430, look);
	if (cam_auto_zoom_method == 0)
		FUN_00050ff0(look, CHL_FIELD(float, cam, CAM_BOUND_MIN), 0.5f, CHL_FIELD(float, cam, CAM_BOUND_MAX), 0.5f);

	/* target point towards the look point */
	p[0] = look[0] - CHL_F32(cam, CAM_TARGET + 0x0);
	p[1] = look[1] - CHL_F32(cam, CAM_TARGET + 0x4);
	p[2] = look[2] - CHL_F32(cam, CAM_TARGET + 0x8);
	FUN_00050ff0(CHL_FIELD(float, cam, CAM_TARGET), CHL_FIELD(float, cam, CAM_TARGET), 1.0f, p,
		cam_auto_target_rate);

	/* wanted eye: CAM_DISTANCE from the look point, pitched, on the line from the pivot */
	pivot = CHL_FIELD(float, cam, CAM_PIVOT);
	p[0] = look[0] - pivot[0];
	p[1] = 0.0f;
	p[2] = look[2] - pivot[2];
	FUN_00051220(axis, cam_axis_z, CHL_F32(cam, CAM_PITCH), (float)CHL_FPATAN(p[0], p[2]));
	FUN_00051570(p, axis, CHL_F32(cam, CAM_DISTANCE));
	CHL_F32(cam, CAM_EYE_WANTED + 0x0) = look[0] + p[0];
	CHL_F32(cam, CAM_EYE_WANTED + 0x4) = look[1] + p[1];
	CHL_F32(cam, CAM_EYE_WANTED + 0x8) = look[2] + p[2];
	p[0] = CHL_F32(cam, CAM_EYE_WANTED + 0x0) - eye[0];
	p[1] = CHL_F32(cam, CAM_EYE_WANTED + 0x4) - eye[1];
	p[2] = CHL_F32(cam, CAM_EYE_WANTED + 0x8) - eye[2];
	step = cam_auto_eye_step / cam_auto_eye_rate;
	if (step < sqrt((double)p[1] * p[1] + (double)p[2] * p[2] + (double)p[0] * p[0]))
		FUN_00051570(p, p, step);
	FUN_00050ff0(eye, eye, 1.0f, p, cam_auto_eye_rate);
	CHL_F32(cam, CAM_DISTANCE) = FUN_00051500(eye, look);

	/* zoom */
	outside = 0;
	inside = 1;
	if (cam_auto_zoom_method == 3) {
		ox_lo = cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_OUT_X) - 320.0f;
		ox_hi = 320.0f - cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_OUT_X);
		oy_lo = cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_OUT_Y) - 240.0f;
		oy_hi = 240.0f - cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_OUT_Y);
		ix_lo = cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_IN_X) - 320.0f;
		ix_hi = 320.0f - cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_IN_X);
		iy_lo = cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_IN_Y) - 240.0f;
		iy_hi = 240.0f - cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_IN_Y);
		t = chl_ListNext(list, NULL);
		if (t != NULL) {
			do {
				float sx = CHL_F32(t, TGT_SCREEN + 0x0);
				float sy = CHL_F32(t, TGT_SCREEN + 0x4);

				if (sx - ox_lo < near_out)
					near_out = sx - ox_lo;
				if (sx - ix_lo < near_in)
					near_in = sx - ix_lo;
				if (ox_hi - sx < near_out)
					near_out = ox_hi - sx;
				if (ix_hi - sx < near_in)
					near_in = ix_hi - sx;
				if (sy - oy_lo < near_out)
					near_out = sy - oy_lo;
				if (sy - iy_lo < near_in)
					near_in = sy - iy_lo;
				if (oy_hi - sy < near_out)
					near_out = oy_hi - sy;
				if (iy_hi - sy < near_in)
					near_in = iy_hi - sy;
				t = chl_ListNext(list, t);
			} while (t != NULL);
			outside = near_out < 0.0f;
			if (near_in <= 0.0f)
				goto not_inside;
			goto decide;
		}
		goto zoom_in;
	}
	for (t = chl_ListNext(list, NULL); t != NULL; t = chl_ListNext(list, t)) {
		f = cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_OUT_X);
		if (CHL_F32(t, TGT_SCREEN + 0x0) < f - 320.0f || 320.0f - f < CHL_F32(t, TGT_SCREEN + 0x0)) {
			outside = 1;
			break;
		}
		f = cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_OUT_Y);
		if (CHL_F32(t, TGT_SCREEN + 0x4) < f - 240.0f || 240.0f - f < CHL_F32(t, TGT_SCREEN + 0x4)) {
			outside = 1;
			break;
		}
	}
	for (t = chl_ListNext(list, NULL); t != NULL; t = chl_ListNext(list, t)) {
		f = cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_IN_X);
		if (CHL_F32(t, TGT_SCREEN + 0x0) <= f - 320.0f || 320.0f - f <= CHL_F32(t, TGT_SCREEN + 0x0))
			goto not_inside;
		f = cam_auto_zoom_scale * CHL_F32(cam, CAM_BORDER_IN_Y);
		if (CHL_F32(t, TGT_SCREEN + 0x4) <= f - 240.0f || 240.0f - f <= CHL_F32(t, TGT_SCREEN + 0x4))
			goto not_inside;
	}
	goto decide;
not_inside:
	inside = 0;
decide:
	if (outside) {
		/* a target is outside the outer border: zoom out */
		if (cam_auto_zoom_method != 1) {
			if (cam_auto_zoom_method == 2) {
				cam_auto_zoom = cam_auto_zoom + cam_auto_zoom_out_step;
				if (cam_auto_zoom_max < cam_auto_zoom)
					cam_auto_zoom = cam_auto_zoom_max;
			} else {
				if (cam_auto_zoom_method != 3)
					goto after;
				if (cam_auto_zoom < 0.0f)
					cam_auto_zoom = cam_auto_zoom * 0.5f;
				cam_auto_zoom = cam_auto_zoom - cam_auto_zoom_out_rate * near_out;
			}
			goto add;
		}
		CHL_F32(cam, CAM_DISTANCE) = cam_auto_zoom_out_step + CHL_F32(cam, CAM_DISTANCE);
	} else {
		if (inside)
			goto zoom_in;
		cam_auto_zoom = cam_auto_zoom * 0.5f;
	}
after:
	if (cam_auto_zoom_method != 2 && cam_auto_zoom_method != 3)
		goto clamp;
	goto add;
zoom_in:
	/* every target is inside the inner border (or there are none): zoom in */
	if (cam_auto_zoom_method == 1) {
		CHL_F32(cam, CAM_DISTANCE) = CHL_F32(cam, CAM_DISTANCE) - cam_auto_zoom_in_step;
		goto after;
	}
	if (cam_auto_zoom_method == 2) {
		cam_auto_zoom = cam_auto_zoom - cam_auto_zoom_in_step;
		if (cam_auto_zoom < cam_auto_zoom_min)
			cam_auto_zoom = cam_auto_zoom_min;
	} else {
		if (cam_auto_zoom_method != 3)
			goto after;
		if (0.0f < cam_auto_zoom)
			cam_auto_zoom = cam_auto_zoom * 0.5f;
		cam_auto_zoom = cam_auto_zoom - cam_auto_zoom_in_rate * near_in;
	}
add:
	CHL_F32(cam, CAM_DISTANCE) = cam_auto_zoom + CHL_F32(cam, CAM_DISTANCE);
clamp:
	if (CHL_F32(cam, CAM_DISTANCE) > cam_auto_max_distance)
		CHL_F32(cam, CAM_DISTANCE) = cam_auto_max_distance;
	if (cam_auto_max_distance < cam_auto_zoom)
		cam_auto_zoom = cam_auto_max_distance;
	/* as in the original: a zoom speed at or above -max changes its sign each frame */
	if (cam_auto_zoom < -cam_auto_max_distance)
		cam_auto_zoom = -cam_auto_max_distance;
	else
		cam_auto_zoom = -cam_auto_zoom;
	FUN_000435f0(CHL_FIELD(float, cam, CAM_FINAL_EYE), CHL_FIELD(float, cam, CAM_FINAL_TARGET), 0.0f);
	FUN_00070ed0(pivot, CHL_FIELD(float, cam, CAM_PIVOT_GOAL), CHL_F32(cam, CAM_PIVOT_SPEED), 0.5f);
}

/* 0x000d6e80  CharLib/CCamera.cpp
Per-frame update: runs the mode, the sway and the shake, stores the heading of the camera
and of each AI player as seen from the eye, and sets the final view. While the game is
paused the last view is kept. */
void CCamera_Update(void *cam)
{
	float *eye = CHL_FIELD(float, cam, CAM_EYE);
	int i;

	FUN_000438b0();
	if (DAT_002ecdec != 0) {
		FUN_000435f0(CHL_FIELD(float, cam, CAM_FINAL_EYE), CHL_FIELD(float, cam, CAM_FINAL_TARGET), 0.0f);
		FUN_00043790();
		return;
	}
	switch (CHL_I32(cam, CAM_MODE)) {
	case 1:
		CCamera_UpdateFollow(cam);
		break;
	case 2:
		CCamera_UpdateIntro(cam);
		break;
	case 3:
		FUN_000435f0(CHL_FIELD(float, cam, CAM_FINAL_EYE), CHL_FIELD(float, cam, CAM_FINAL_TARGET), 0.0f);
		FUN_00043790();
		return;
	case 5:
		CCamera_UpdateAuto(cam);
	}
	if (0.0f < CHL_F32(cam, CAM_SWAY))
		CCamera_Sway(cam);
	if (0.0f < CHL_F32(cam, CAM_SHAKE_TIME))
		CCamera_UpdateShake(cam);
	CHL_F32(cam, CAM_HEADING) = FUN_000703a0((float)CHL_FPATAN(
		(double)CHL_F32(cam, CAM_TARGET + 0x0) - (double)eye[0],
		(double)CHL_F32(cam, CAM_TARGET + 0x8) - (double)eye[2]));
	cam_heading = CHL_F32(cam, CAM_HEADING);
	for (i = 0; i < chl_player_count; i++) {
		void *actor = CHL_PTR(chl_ai_players[i], AI_ACTOR);

		CHL_F32(chl_ai_players[i], 0x1118) = FUN_000703a0((float)CHL_FPATAN(
			(double)CHL_F32(actor, 0xd0) - (double)eye[0], (double)CHL_F32(actor, 0xd8) - (double)eye[2]));
	}
	CHL_F32(cam, CAM_FINAL_EYE + 0x0) = CHL_F32(cam, CAM_SHAKE_OFFSET + 0x0) + eye[0];
	CHL_F32(cam, CAM_FINAL_EYE + 0x4) = eye[1] + CHL_F32(cam, CAM_SHAKE_OFFSET + 0x4);
	CHL_F32(cam, CAM_FINAL_EYE + 0x8) = eye[2] + CHL_F32(cam, CAM_SHAKE_OFFSET + 0x8);
	CHL_F32(cam, CAM_FINAL_TARGET + 0x0) = CHL_F32(cam, CAM_TARGET + 0x0) + CHL_F32(cam, CAM_SHAKE_OFFSET + 0x0);
	CHL_F32(cam, CAM_FINAL_TARGET + 0x4) = CHL_F32(cam, CAM_TARGET + 0x4) + CHL_F32(cam, CAM_SHAKE_OFFSET + 0x4);
	CHL_F32(cam, CAM_FINAL_TARGET + 0x8) = CHL_F32(cam, CAM_TARGET + 0x8) + CHL_F32(cam, CAM_SHAKE_OFFSET + 0x8);
	FUN_00043790();
}

/* 0x000d7030  CharLib/CCamera.cpp
Mode 2: plays the intro cameras. When the current one ends (or a player skips it) the next
queued LOM file is loaded; for each file the matching player cheers. With no file left
(or on a skip) the camera goes back to the mode it had before. */
void CCamera_UpdateIntro(void *cam)
{
	char name[32];
	unsigned int size;
	int skip = 0;
	int i, cheer;
	int *map;

	if (FUN_0007cc70() > 30.0f) {
		/* the intro can be skipped after 30 frames */
		if (DAT_003055e0 != 0)
			skip = 1;
		for (i = 0; i < chl_player_count; i++) {
			void *pad;

			if (DAT_002baed4[i * 10 + 1] == 0)
				continue;
			pad = CHL_PTR(chl_ai_players[i], AI_BUTTON);
			if (CHL_I32(pad, 0x14) != 0 && CHL_I32(pad, 0x20) == 1)
				skip = 1;
		}
		if (skip == 1)
			goto next;
	}
	if (FUN_0007e270(chl_time_scale, 0) == 0)
		goto play;
next:
	FUN_00071db0(-1.0f);
	if (skip != 0 || CHL_I32(cam, CAM_LOM_COUNT) == 0) {
		FUN_0007cc90();
		FUN_0007d590();
		if (CHL_I32(cam, CAM_PREV_MODE) > 0) {
			CCamera_SetMode(cam, CHL_I32(cam, CAM_PREV_MODE));
			CHL_I32(cam, CAM_PREV_MODE) = -1;
			if (skip != 0) {
				cam_copy4(cam, CAM_FINAL_EYE, CAM_SAVED_EYE);
				cam_copy4(cam, CAM_FINAL_TARGET, CAM_SAVED_TARGET);
				cam_copy4(cam, CAM_EYE, CAM_FINAL_EYE);
				cam_copy4(cam, CAM_TARGET, CAM_FINAL_TARGET);
			}
			CCamera_Update(cam);
		}
		return;
	}
	FUN_0007cc90();
	FUN_0007d590();
	for (i = 0; i < 4; i++) {
		void *ai = chl_ai_players[i];

		if (ai == NULL)
			continue;
		if ((unsigned int)CHL_U16(CHL_PTR(ai, AI_ACTOR), 0x230) == 4u - CHL_I32(cam, CAM_LOM_COUNT)) {
			AI_VCALL(ai, AI_VSLOT_HOLD_END, void (*)(void *, int, int))(ai, CHL_I32(cam, CAM_LOM_PARAMS), 0x22);
			FUN_000719a0(6, DAT_002baed4[i * 10], 0xff, 0, 0, NULL);
			do {
				cheer = CHL_Rand() % 3;
			} while (cheer == cam_last_cheer);
			cam_last_cheer = cheer;
			sprintf(name, cam_cheer_format, cheer);
			FUN_000713a0((int)chl_FindObject(NULL, name, 2, 0xdefa), 0xff, 0, 0, NULL);
		} else {
			AI_VCALL(ai, AI_VSLOT_HOLD_END, void (*)(void *, int, int))(ai, CHL_I32(cam, CAM_LOM_PARAMS), 0);
		}
	}
	map = cam_ParseMapLoad(cam_LoadFile(CHL_PTR(cam, CAM_LOM_PACKAGE), (const char *)cam, NULL, &size, NULL, 0),
		2, size);
	if (map == NULL) {
		chl_Printf("InitialiseCamera - Error parsing camera LOM file [%s]\n",
			(char *)cam + CHL_I32(cam, CAM_LOM_COUNT) * 0x40);
		return;
	}
	cam_IntroCamLoad(map, cam);
	FUN_0007f3a0(map);
	for (i = 1; i < CHL_I32(cam, CAM_LOM_COUNT); i++) {
		strcpy((char *)cam + CAM_LOM_NAMES + (i - 1) * 0x40, (char *)cam + CAM_LOM_NAMES + i * 0x40);
		CHL_I32(cam, CAM_LOM_PARAMS + (i - 1) * 4) = CHL_I32(cam, CAM_LOM_PARAMS + i * 4);
	}
	CHL_I32(cam, CAM_LOM_COUNT) = CHL_I32(cam, CAM_LOM_COUNT) - 1;
	CCamera_Update(cam);
play:
	FUN_0007dbd0(CHL_FIELD(float, cam, CAM_FINAL_EYE), FUN_0007cc70());
	FUN_0007df20(CHL_FIELD(float, cam, CAM_FINAL_TARGET), FUN_0007cc70());
	cam_copy4(cam, CAM_EYE, CAM_FINAL_EYE);
	cam_copy4(cam, CAM_TARGET, CAM_FINAL_TARGET);
}

/* 0x000d7420  CharLib/CCamera.cpp
Changes the mode: 1 follows the actor data, 2 starts the intro cameras (the view is saved),
3 is a fixed camera at data (eye, target: 8 floats), 5 the automatic camera. */
void CCamera_SwitchMode(void *cam, void *data, int mode)
{
	float off[4];
	void *actor;
	int i;

	CHL_I32(cam, CAM_PREV_MODE) = CHL_I32(cam, CAM_MODE);
	if (CHL_I32(cam, CAM_MODE) == 2) {
		FUN_0007cc90();
		FUN_0007d590();
	}
	switch (mode) {
	case 1:
		CHL_PTR(cam, CAM_FOLLOW) = data;
		off[0] = 0.0f;
		off[1] = 400.0f;
		off[2] = 400.0f;
		off[3] = 1.0f;
		FUN_00051170(off, off, CHL_F32(data, 0x1e8));
		actor = CHL_PTR(cam, CAM_FOLLOW);
		CHL_F32(cam, CAM_EYE + 0x0) = off[0] + CHL_F32(actor, 0xd0);
		CHL_F32(cam, CAM_EYE + 0x4) = off[1] + CHL_F32(actor, 0xd4);
		CHL_F32(cam, CAM_EYE + 0x8) = off[2] + CHL_F32(actor, 0xd8);
		actor = CHL_PTR(cam, CAM_FOLLOW);
		CHL_F32(cam, CAM_TARGET + 0x0) = CHL_F32(actor, 0xd0);
		CHL_F32(cam, CAM_TARGET + 0x4) = CHL_F32(actor, 0xd4);
		CHL_F32(cam, CAM_TARGET + 0x8) = CHL_F32(actor, 0xd8);
		CHL_F32(cam, CAM_TARGET + 0xc) = CHL_F32(actor, 0xdc);
		CHL_I32(cam, CAM_SWAY) = 0;
		CHL_F32(cam, CAM_TARGET + 0x4) = CHL_F32(cam, CAM_TARGET + 0x4) + 160.0f;
		break;
	case 2:
		cam_copy4(cam, CAM_SAVED_EYE, CAM_FINAL_EYE);
		cam_copy4(cam, CAM_SAVED_TARGET, CAM_FINAL_TARGET);
		CHL_I32(cam, CAM_MODE) = 2;
		CCamera_UpdateIntro(cam);
		return;
	case 3:
		for (i = 0; i < 8; i++) {
			CHL_F32(cam, CAM_FINAL_EYE + i * 4) = ((float *)data)[i];
		}
		for (i = 0; i < 4; i++) {
			CHL_F32(cam, CAM_EYE + i * 4) = ((float *)data)[i];
		}
		for (i = 0; i < 4; i++) {
			CHL_F32(cam, CAM_TARGET + i * 4) = ((float *)data)[4 + i];
		}
		CCamera_SetStatic(cam);
		return;
	case 5:
		CHL_I32(cam, CAM_MODE) = mode;
		CCamera_ResetAuto(cam);
		return;
	}
	CHL_I32(cam, CAM_MODE) = mode;
}
