/*
RollerBall/unplaced_camera.c  (module rollerball_b)

The RollerBall game camera (game + 0x1368, vtable 0x00274884), 0x00192c40-0x00194970.
The original file is not known: the code has no assert or heap tag. Its .rdata (vtable
0x00274884 and the constant 602.0 at 0x00274888) lies between CRollerball_Entity.cpp's and
CRollerball_World.cpp's, so it is probably a file of its own (a "CRollerball_Camera.cpp").

Camera modes (+0x48): 1 the intro fly-by (classlib_ui CLIntroCam), 2 the count down,
3 play, 4 the end (follow the winner). In play the camera frames every ball (and in some
variations the goals, platforms or cutters): 0x001935f0 fits a view of the given pitch and
yaw around a set of points. 0x00193120, 0x00193260 and 0x00193360 are missing from the
function table (only reached by the jump table of 0x00194800).

Layout: +0x0c eye (float[4]), +0x1c target (float[4]), +0x30 pitch, +0x34 yaw, +0x40 field
of view, +0x44 roll, +0x48 mode, +0x4c fade in, +0x50 darken out, +0x54 darken in,
+0x58/+0x5c/+0x60 requests for these three, +0x64 yaw speed, +0x68/+0x6c nearest/farthest
distance, +0x70 margin, +0x74/+0x78 eye/target smoothing, +0x80 frame timer, +0xb8 the
player followed at the end, +0xbc the time of the end shot.
*/

#include "rollerball_b.h"

#define RBC_EYE 0x0c
#define RBC_TARGET 0x1c
#define RBC_PITCH 0x30
#define RBC_YAW 0x34
#define RBC_FOV 0x40
#define RBC_ROLL 0x44
#define RBC_MODE 0x48
#define RBC_FADE 0x4c
#define RBC_DARKEN_OUT 0x50
#define RBC_DARKEN_IN 0x54
#define RBC_REQ_FADE 0x58
#define RBC_REQ_DARKEN_OUT 0x5c
#define RBC_REQ_DARKEN_IN 0x60
#define RBC_YAW_SPEED 0x64
#define RBC_NEAR 0x68
#define RBC_FAR 0x6c
#define RBC_MARGIN 0x70
#define RBC_EYE_LERP 0x74
#define RBC_TARGET_LERP 0x78
#define RBC_TIMER 0x80
#define RBC_FOLLOWED 0xb8
#define RBC_END_TIME 0xbc

/* 0x00274884  camera vtable */
const void *const rbb_vtable_00274884[1] = {
	(const void *)FUN_00192f20, /* +0x00 scalar deleting destructor */
};

/* classlib_ui: the HUD object */
extern int DAT_002ecfac;
/* core: no human plays (attract mode) */
extern int DAT_002baeb8;

/* 0x00192c40  (camera file unknown)
Constructor. */
void *CRollerballCamera_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274884;
	rbb_TimerCtor(RBB_ADDR(self, RBC_TIMER));
	return self;
}

/* 0x00192c60  (camera file unknown)
Destructor. */
void CRollerballCamera_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274884;
	rbb_TimerDtor(RBB_ADDR(self, RBC_TIMER));
}

/* 0x00192c80  (camera file unknown) */
void CRollerballCamera_SetMode(void *self, int mode)
{
	RBB_I(self, RBC_MODE) = mode;
	if (mode == 1)
		RBB_I(self, RBC_DARKEN_IN) = 1;
}

/* 0x00192ca0  (camera file unknown)
Set the view (field of view, look-at). */
void CRollerballCamera_Apply(void *self)
{
	bdSetFieldOfView(RBB_F(self, RBC_FOV), RBB_F(self, RBC_FOV) * 3.0f * 0.25f);
	bdSetCameraLookAt(RBB_V(self, RBC_EYE), RBB_V(self, RBC_TARGET), RBB_F(self, RBC_ROLL));
	bdUpdateView();
}

/* 0x00192ce0  (camera file unknown)
The player to show in the end shot: the winners (+0x17) take turns, each for an equal
part of the end time. */
void *CRollerballCamera_PickWinner(void *self)
{
	char *first = rbb_FirstPlayer;
	char *e;
	int count = 0;
	int pick;
	int i;
	float slice;

	for (e = first; e != NULL; e = RBB_P(e, RBE_NEXT))
		if (RBB_C(e, RBE_WINNER) != 0)
			count++;
	if (count == 0)
		return NULL;
	slice = RBB_F(self, RBC_END_TIME) / (float)count;
	pick = RBB_FTOL((RBB_F(self, RBC_END_TIME) - rbb_TimerRemaining(RBB_ADDR(self, RBC_TIMER))) / slice);
	if (pick >= count)
		pick = count - 1;
	i = 0;
	for (e = first; e != NULL; e = RBB_P(e, RBE_NEXT)) {
		if (RBB_C(e, RBE_WINNER) != 0) {
			if (i == pick)
				return e;
			i++;
		}
	}
	return NULL;
}

/* 0x00192d70  (camera file unknown)
Draw the screen fades: the fade in after a cut, and the darkening out (+0x50) or in
(+0x54) over the camera timer. */
void CRollerballCamera_DrawOverlays(void *self)
{
	float t;

	if (RBB_I(self, RBC_FADE) != 0 && CLFade_Draw(RBB_F(rbb_Game, 0x268c)) != 0)
		RBB_I(self, RBC_FADE) = 0;
	if (RBB_I(self, RBC_DARKEN_OUT) != 0) {
		t = rbb_TimerProgress(RBB_ADDR(self, RBC_TIMER));
		if (1.0f <= t)
			RBB_I(self, RBC_DARKEN_OUT) = 0;
		CLScreen_DrawDarken(RBB_FTOL(t * 255.0f), 1);
	}
	if (RBB_I(self, RBC_DARKEN_IN) != 0) {
		t = rbb_TimerProgress(RBB_ADDR(self, RBC_TIMER));
		if (1.0f <= t)
			RBB_I(self, RBC_DARKEN_IN) = 0;
		CLScreen_DrawDarken(RBB_FTOL((1.0f - t) * 255.0f), 1);
	}
}

/* 0x00192e20  (camera file unknown)
Reset for the variation: distances and pitch of the play camera, the intro path start. */
void CRollerballCamera_Reset(void *self)
{
	RBB_I(self, RBC_MODE) = 0;
	RBB_F(self, RBC_TARGET + 0) = 0.0f;
	RBB_F(self, RBC_TARGET + 4) = 0.0f;
	RBB_F(self, RBC_TARGET + 8) = 0.0f;
	RBB_F(self, RBC_EYE + 0) = 0.0f;
	RBB_F(self, RBC_EYE + 4) = 0.0f;
	RBB_F(self, RBC_EYE + 8) = 0.0f;
	switch (rbb_Variation) {
	case 0:
		RBB_F(self, RBC_NEAR) = 500.0f;
		RBB_F(self, RBC_FAR) = 3250.0f;
		goto common;
	case 1:
		RBB_F(self, RBC_NEAR) = 500.0f;
		RBB_F(self, RBC_FAR) = 2800.0f;
		goto common;
	case 2:
	case 3:
		RBB_F(self, RBC_NEAR) = 2500.0f;
		RBB_F(self, RBC_FAR) = 10000.0f;
		RBB_F(self, RBC_PITCH) = -0.785398185f;
		RBB_F(self, RBC_MARGIN) = 1100.0f;
		goto speeds;
	case 4:
		RBB_F(self, RBC_NEAR) = 300.0f;
		RBB_F(self, RBC_FAR) = 2500.0f;
		goto common;
	default:
		goto done;
	}
common:
	RBB_F(self, RBC_PITCH) = -0.69813168f;
	RBB_F(self, RBC_MARGIN) = 800.0f;
speeds:
	RBB_U(self, RBC_YAW_SPEED) = 0x3a64c388u;   /* 0.000872665 (0.05 degrees) */
	RBB_U(self, RBC_EYE_LERP) = 0x3ccccccdu;    /* 0.025 */
	RBB_U(self, RBC_TARGET_LERP) = 0x3ccccccdu;
done:
	rbb_TimerReset(RBB_ADDR(self, RBC_TIMER));
	RBB_F(self, RBC_ROLL) = 0.0f;
	RBB_F(self, RBC_FOV) = 1.22173047f;
	CLIntroCam_Restart();
	CLIntroCam_GetPosition(RBB_V(self, RBC_EYE), 0.0f);
	CLIntroCam_GetTarget(RBB_V(self, RBC_TARGET), 0.0f);
	RBB_I(self, RBC_FADE) = 0;
	RBB_I(self, RBC_DARKEN_OUT) = 0;
	RBB_I(self, RBC_DARKEN_IN) = 0;
	RBB_I(self, RBC_REQ_FADE) = 0;
	RBB_I(self, RBC_REQ_DARKEN_OUT) = 0;
	RBB_I(self, RBC_REQ_DARKEN_IN) = 0;
	RBB_P(self, RBC_FOLLOWED) = NULL;
}

/* 0x00192f20  (camera file unknown)
Scalar deleting destructor. */
void *CRollerballCamera_DeletingDtor(void *self, unsigned int flags)
{
	CRollerballCamera_Dtor(self);
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x00192f40  (camera file unknown) */
int CRollerballCamera_Init(void *self)
{
	rbb_TimerInit(RBB_ADDR(self, RBC_TIMER));
	CRollerballCamera_Reset(self);
	return 1;
}

/* 0x00192f60  (camera file unknown)
Mode 1: the intro fly-by. When it ends (or a player with a pad presses a button, or in
attract mode) go to the count down (mode 2), at the yaw of the variation. */
void CRollerballCamera_UpdateIntro(void *self, float dt)
{
	int ended = 0;
	int skipped = 0;
	int status;
	int i;
	float fov;

	(void)dt;
	status = CLIntroCam_Update(1.0f, 1);
	if (status == 0) {
		float pos[4];
		float target[4];
		float t = (float)RBB_FTOL(CLIntroCam_GetTime());
		CLIntroCam_GetPosition(pos, t);
		CLIntroCam_GetTarget(target, t);
		CLIntroCam_GetRoll(RBB_V(self, RBC_ROLL), t);
		CLIntroCam_GetFov(&fov, t);
		memcpy(RBB_V(self, RBC_EYE), pos, 16);
		memcpy(RBB_V(self, RBC_TARGET), target, 16);
	} else {
		if (status == -1)
			bkPrintf("***CAMERA ERROR***\n");
		ended = 1;
	}
	/* game + 0x25d8: the input object of each player slot; +0x14 a button was pressed */
	for (i = 0; i < 4; i++) {
		if (rbb_PlayerHuman(i) != 0 && RBB_I(RBB_P(rbb_Game, 0x25d8 + i * 4), 0x14) != 0) {
			skipped = 1;
			break;
		}
	}
	if (!skipped) {
		if (DAT_002baeb8 != 0)
			skipped = 1;
		else if (!ended)
			return;
	}
	RBB_I(self, RBC_REQ_FADE) = 1;
	fov = 1.22173047f;   /* stored and not used (the original passes it to an empty function) */
	(void)fov;
	RBB_F(self, RBC_ROLL) = 0.0f;
	if (skipped) {
		float last = (float)(CLIntroCam_GetFrameCount() - 1);
		CLIntroCam_GetPosition(RBB_V(self, RBC_EYE), last);
		CLIntroCam_GetTarget(RBB_V(self, RBC_TARGET), last);
	}
	RBB_I(self, RBC_MODE) = 2;
	FUN_001d5950();
	switch (rbb_Variation) {
	case 0:
		RBB_F(self, RBC_YAW) = 3.92699075f;
		break;
	case 1:
	case 4:
		RBB_F(self, RBC_YAW) = 0.0f;
		break;
	case 2:
		RBB_F(self, RBC_YAW) = 4.71238899f;
		break;
	case 3:
		RBB_F(self, RBC_YAW) = 2.3561945f;
		break;
	}
	CLHud_StartBars((float *)(uintptr_t)DAT_002ecfac, 280.0f);
}

/* the shared tail of the end shots: look at `pos` from 300 units behind and above (pitch
-15 degrees, the camera's yaw); the eye's w is `w` */
static void rbb_CameraLookAtFromBehind(void *self, const float *pos, float w)
{
	float back[3];
	float off[4];

	RBB_U(self, RBC_PITCH) = 0xbe860a92u;   /* -0.261799 (-15 degrees) */
	back[0] = 0.0f;
	back[1] = 0.0f;
	back[2] = 300.0f;
	off[3] = w;
	bmVectorRotateXY(off, back, rbb_bitsf(0xbe860a92u), RBB_F(self, RBC_YAW));
	RBB_F(self, RBC_EYE + 12) = off[3];
	RBB_F(self, RBC_EYE + 8) = off[2] + pos[2];
	RBB_F(self, RBC_EYE + 0) = off[0] + pos[0];
	RBB_F(self, RBC_EYE + 4) = off[1] + pos[1];
	RBB_F(self, RBC_TARGET + 0) = pos[0];
	RBB_F(self, RBC_TARGET + 4) = pos[1];
	RBB_F(self, RBC_TARGET + 8) = pos[2];
	RBB_F(self, RBC_TARGET + 12) = pos[3];
}

/* a new followed player: turn its ball to the camera's yaw */
static void rbb_CameraTurnBall(void *self, char *e)
{
	float axis_angle[4];

	axis_angle[3] = -RBB_F(self, RBC_YAW);
	axis_angle[0] = 0.0f;
	axis_angle[1] = 1.0f;
	axis_angle[2] = 0.0f;
	bmAxisAngleToQuaternion(RBB_V(e, RBE_ROTATION), axis_angle);
}

/* the followed player changed: remember it and fade in (returns whether it changed) */
static int rbb_CameraFollow(void *self, char *e)
{
	if (e == RBB_P(self, RBC_FOLLOWED))
		return 0;
	RBB_P(self, RBC_FOLLOWED) = e;
	RBB_I(self, RBC_REQ_FADE) = 1;
	return 1;
}

/* 0x00193120  (camera file unknown; missing from the function table)
End shot of "base": behind the winner, facing the arena centre. The eye's w is not set
in the original (stack garbage); 0 here. */
void FUN_00193120(void *self, float dt)
{
	char *e = CRollerballCamera_PickWinner(self);
	float pos[4];
	float heading;
	int changed;

	(void)dt;
	if (e == NULL)
		return;
	changed = rbb_CameraFollow(self, e);
	memcpy(pos, RBB_V(e, RBE_POS), 16);
	heading = (float)(atan2((double)pos[0], (double)pos[2]) + (double)3.14159274f);
	RBB_F(self, RBC_YAW) = heading;
	RBB_F(e, RBE_HEADING) = heading;
	RBB_F(e, RBE_HEADING2) = heading;
	if (changed)
		rbb_CameraTurnBall(self, e);
	rbb_CameraLookAtFromBehind(self, pos, 0.0f);
}

/* 0x00193260  (camera file unknown; missing from the function table)
End shot of "race2": like 0x00193120, without turning the ball. */
void FUN_00193260(void *self, float dt)
{
	char *e = CRollerballCamera_PickWinner(self);
	float pos[4];

	(void)dt;
	if (e == NULL)
		return;
	rbb_CameraFollow(self, e);
	memcpy(pos, RBB_V(e, RBE_POS), 16);
	RBB_F(self, RBC_YAW) = (float)(atan2((double)pos[0], (double)pos[2]) + (double)3.14159274f);
	rbb_CameraLookAtFromBehind(self, pos, 0.0f);
}

/* 0x00193360  (camera file unknown; missing from the function table)
End shot of "race": behind the winner at a fixed yaw of -45 degrees. */
void FUN_00193360(void *self, float dt)
{
	char *e = CRollerballCamera_PickWinner(self);
	float pos[4];
	int changed;

	(void)dt;
	if (e == NULL)
		return;
	changed = rbb_CameraFollow(self, e);
	memcpy(pos, RBB_V(e, RBE_POS), 16);
	RBB_F(self, RBC_YAW) = -0.785398185f;
	RBB_F(e, RBE_HEADING) = -0.785398185f;
	RBB_F(e, RBE_HEADING2) = -0.785398185f;
	if (changed)
		rbb_CameraTurnBall(self, e);
	rbb_CameraLookAtFromBehind(self, pos, 0.0f);
}

/* 0x00193490  (camera file unknown)
End shot of "sumo" and "drop": behind the winner, facing away from the centre, and never
lower than 602. The eye's w gets the turn angle (garbage in the original when the
followed player did not change; 0 here). */
void CRollerballCamera_EndShotArena(void *self, float dt)
{
	char *e = CRollerballCamera_PickWinner(self);
	float pos[4];
	float back[3];
	float off[3];
	float axis_angle[4];
	float heading;
	int changed;

	(void)dt;
	if (e == NULL)
		return;
	changed = rbb_CameraFollow(self, e);
	memcpy(pos, RBB_V(e, RBE_POS), 16);
	heading = (float)atan2((double)pos[0], (double)pos[2]);
	RBB_F(self, RBC_YAW) = heading;
	RBB_F(e, RBE_HEADING) = heading;
	RBB_F(e, RBE_HEADING2) = heading;
	axis_angle[3] = 0.0f;
	if (changed) {
		axis_angle[3] = -RBB_F(self, RBC_YAW);
		axis_angle[0] = 0.0f;
		axis_angle[1] = 1.0f;
		axis_angle[2] = 0.0f;
		bmAxisAngleToQuaternion(RBB_V(e, RBE_ROTATION), axis_angle);
	}
	RBB_U(self, RBC_PITCH) = 0xbe860a92u;
	back[0] = 0.0f;
	back[1] = 0.0f;
	back[2] = 300.0f;
	bmVectorRotateXY(off, back, rbb_bitsf(0xbe860a92u), RBB_F(self, RBC_YAW));
	off[1] = off[1] + pos[1];
	if (off[1] < 602.0f)
		off[1] = 602.0f;
	RBB_F(self, RBC_EYE + 4) = off[1];
	RBB_F(self, RBC_EYE + 0) = off[0] + pos[0];
	RBB_F(self, RBC_EYE + 8) = off[2] + pos[2];
	RBB_F(self, RBC_EYE + 12) = axis_angle[3];
	RBB_F(self, RBC_TARGET + 0) = pos[0];
	RBB_F(self, RBC_TARGET + 4) = pos[1];
	RBB_F(self, RBC_TARGET + 8) = pos[2];
	RBB_F(self, RBC_TARGET + 12) = pos[3];
}

/* 0x001935f0  (camera file unknown)
Fit a view of the given pitch and yaw around `count` points (float[4] each; their w is set
to 1): in camera space, points below the mean height drop by 200, the field of view
(+0x40; 0.75 of it vertically) bounds the eye's distance, which is then kept between the
nearest (+0x68) and farthest (+0x6c) distance less the margin (+0x70). Writes the eye and
the target (the mean of the points) in world space. */
void CRollerballCamera_FitPoints(void *self, float *eye_out, float *target_out, float *points,
	int count, float pitch, float yaw)
{
	float m[16];
	float local[20][4];
	float sum_y = 0.0f;
	float sum_z = 0.0f;
	float mean_y;
	float mean_z;
	float s;
	float k;
	float hi;
	float lo;
	float v;
	float centre_x;
	float centre_y;
	float dist_y;
	float dist;
	float eye[4];
	float target[4];
	int i;

	bmMatrixRotationZXY(m, pitch, yaw, 0.0f);
	for (i = 0; i < count; i++) {
		points[i * 4 + 3] = 1.0f;
		bmTransformVector4(local[i], m, points + i * 4);
		sum_y = sum_y + local[i][1];
		sum_z = sum_z + local[i][2];
	}
	mean_y = sum_y * (1.0f / (float)count);
	mean_z = (1.0f / (float)count) * sum_z;
	for (i = 0; i < count; i++)
		if (local[i][1] < mean_y)
			local[i][1] = local[i][1] - 200.0f;

	/* vertical: the eye height that sees every point */
	s = (float)sin((double)RBB_F(self, RBC_FOV) * 0.5);
	k = s * 0.75f;
	hi = local[0][1] - local[0][2] * k;
	lo = local[0][2] * k + local[0][1];
	for (i = 0; i < count; i++) {
		v = local[i][1] - k * local[i][2];
		if (hi < v)
			hi = v;
		v = k * local[i][2] + local[i][1];
		if (v < lo)
			lo = v;
	}
	centre_y = (hi + lo) * 0.5f;
	dist_y = (-1.0f / k) * (hi - lo) * 0.5f;

	/* horizontal */
	hi = local[0][0] - local[0][2] * s;
	lo = local[0][2] * s + local[0][0];
	for (i = 0; i < count; i++) {
		v = local[i][0] - s * local[i][2];
		if (hi < v)
			hi = v;
		v = s * local[i][2] + local[i][0];
		if (v < lo)
			lo = v;
	}
	centre_x = (hi + lo) * 0.5f;
	dist = (-1.0f / s) * (hi - lo) * 0.5f;
	if (dist < dist_y)
		dist_y = dist;

	dist_y = dist_y - RBB_F(self, RBC_MARGIN);
	if (mean_z - dist_y < RBB_F(self, RBC_NEAR))
		dist_y = mean_z - RBB_F(self, RBC_NEAR);
	eye[2] = dist_y;
	if (RBB_F(self, RBC_FAR) < mean_z - eye[2])
		eye[2] = mean_z - RBB_F(self, RBC_FAR);
	eye[0] = centre_x;
	eye[1] = centre_y;
	eye[3] = 1.0f;
	target[0] = centre_x;
	target[1] = centre_y;
	target[2] = mean_z;
	target[3] = 1.0f;
	bmMatrixRotationYXZ(m, -pitch, -yaw, 0.0f);
	bmTransformVector4(eye_out, m, eye);
	bmTransformVector4(target_out, m, target);
}

/* the debug "sole player" view (game + 0x44 set, player game + 0x48): from the ball's
centre along its heading, 10 degrees down */
static int rbb_CameraFirstPerson(void *self)
{
	char *e;
	float ahead[3];
	float off[3];

	if (RBB_I(rbb_Game, 0x44) == 0)
		return 0;
	e = RBB_P(rbb_Game, 0x48);
	memcpy(RBB_V(self, RBC_EYE), RBB_V(e, RBE_POS), 16);
	ahead[0] = 0.0f;
	ahead[1] = 0.0f;
	RBB_F(self, RBC_EYE + 4) = RBB_F(e, RBE_RADIUS) * 0.5f + RBB_F(self, RBC_EYE + 4);
	ahead[2] = RBB_F(e, RBE_RADIUS);
	bmVectorRotateXY(off, ahead, 0.17453292f, RBB_F(e, RBE_HEADING));
	RBB_F(self, RBC_TARGET + 0) = off[0] + RBB_F(self, RBC_EYE + 0);
	RBB_F(self, RBC_TARGET + 4) = off[1] + RBB_F(self, RBC_EYE + 4);
	RBB_F(self, RBC_TARGET + 8) = off[2] + RBB_F(self, RBC_EYE + 8);
	return 1;
}

/* the pitch of the count down (mode 2) and of play (mode 3); the yaw turns except in the
count down and the intro */
static void rbb_CameraTurn(void *self, float dt)
{
	int mode = RBB_I(self, RBC_MODE);

	if (mode == 2)
		RBB_F(self, RBC_PITCH) = -0.575958669f;
	else if (mode == 3)
		RBB_F(self, RBC_PITCH) = -0.69813168f;
	if (mode < 1 || 2 < mode)
		RBB_F(self, RBC_YAW) = dt * RBB_F(self, RBC_YAW_SPEED) + RBB_F(self, RBC_YAW);
}

/* the balls in play (+0x17) as points */
static int rbb_CameraAddBalls(float (*points)[4], int n)
{
	char *e;

	for (e = (char *)rbb_FirstPlayer; e != NULL; e = RBB_P(e, RBE_NEXT)) {
		if (RBB_C(e, RBE_WINNER) != 0) {
			memcpy(points[n], RBB_V(e, RBE_POS), 16);
			n++;
		}
	}
	return n;
}

/* move the camera to the fitted view: at once (flags & 2) or smoothly */
static void rbb_CameraMoveTo(void *self, float dt, unsigned char flags, const float *eye,
	const float *target)
{
	if ((flags & 2) == 0) {
		bmVectorLerp(RBB_V(self, RBC_EYE), RBB_V(self, RBC_EYE), eye, dt * RBB_F(self, RBC_EYE_LERP));
		bmVectorLerp(RBB_V(self, RBC_TARGET), RBB_V(self, RBC_TARGET), target,
			dt * RBB_F(self, RBC_TARGET_LERP));
		return;
	}
	memcpy(RBB_V(self, RBC_EYE), eye, 16);
	memcpy(RBB_V(self, RBC_TARGET), target, 16);
}

/* 0x00193c10  (camera file unknown)
Play view of "base": every ball and every lit goal. */
void CRollerballCamera_FrameBase(void *self, float dt, unsigned char flags)
{
	float points[20][4];
	float eye[4];
	float target[4];
	char *goal;
	int n;

	if (rbb_CameraFirstPerson(self))
		return;
	rbb_CameraTurn(self, dt);
	n = rbb_CameraAddBalls(points, 0);
	for (goal = RBB_P(rbb_GoalSystem, RBGS_GOAL_LIST); goal != NULL; goal = RBB_P(goal, RBG_NEXT)) {
		if (RBB_S(goal, RBG_STATE) != RBG_STATE_IDLE) {
			memcpy(points[n], RBB_V(goal, RBG_POS), 16);
			n++;
		}
	}
	CRollerballCamera_FitPoints(self, eye, target, points[0], n, RBB_F(self, RBC_PITCH),
		RBB_F(self, RBC_YAW));
	rbb_CameraMoveTo(self, dt, flags, eye, target);
}

/* 0x00193e40  (camera file unknown)
Play view of "sumo": every ball and the visible ones of the variation's five platforms
(variation object + 0x5c, stride 0x18; platform +0x64 position, +0x78 visible). */
void CRollerballCamera_FrameSumo(void *self, float dt, unsigned char flags)
{
	float points[20][4];
	float eye[4];
	float target[4];
	char *var;
	int n;
	int i;

	if (rbb_CameraFirstPerson(self))
		return;
	rbb_CameraTurn(self, dt);
	n = rbb_CameraAddBalls(points, 0);
	var = RBB_P(rbb_Game, 0x26f0);
	for (i = 0; i < 5; i++) {
		char *obj = RBB_P(var, 0x5c + i * 0x18);
		if (RBB_C(obj, 0x78) != 0) {
			memcpy(points[n], RBB_V(obj, 0x64), 16);
			n++;
		}
	}
	CRollerballCamera_FitPoints(self, eye, target, points[0], n, RBB_F(self, RBC_PITCH),
		RBB_F(self, RBC_YAW));
	rbb_CameraMoveTo(self, dt, flags, eye, target);
}

/* the balls in play, each with a second point 0-3000 units ahead along its horizontal
velocity (full at a speed of 80) */
static int rbb_CameraAddBallsAhead(float (*points)[4])
{
	char *e;
	int n = 0;

	for (e = (char *)rbb_FirstPlayer; e != NULL; e = RBB_P(e, RBE_NEXT)) {
		float vz;
		float vx;
		float speed;
		if (RBB_C(e, RBE_WINNER) == 0)
			continue;
		memcpy(points[n], RBB_V(e, RBE_POS), 16);
		vz = RBB_F(e, RBE_VEL + 8);
		vx = RBB_F(e, RBE_VEL + 0);
		n++;
		speed = sqrtf(vx * vx + vz * vz);
		if (0.0f < speed) {
			float flat[4];
			float ahead[3];
			float k;
			speed = speed * 0.0125000002f;
			memcpy(points[n], RBB_V(e, RBE_POS), 16);
			k = 1.0f;
			if (speed <= 1.0f) {
				k = speed;
				if (speed < 0.0f)
					k = 0.0f;
			}
			flat[1] = 0.0f;
			flat[0] = vx;
			flat[2] = vz;
			bmVectorSetLength(ahead, flat, k * 3000.0f);
			points[n][0] = ahead[0] + points[n][0];
			points[n][1] = ahead[1] + points[n][1];
			points[n][2] = ahead[2] + points[n][2];
			n++;
		}
	}
	return n;
}

/* 0x00194120  (camera file unknown)
Play view of "race2": the balls and where they are heading. The yaw does not turn. */
void CRollerballCamera_FrameRace2(void *self, float dt, unsigned char flags)
{
	float points[20][4];
	float eye[4];
	float target[4];
	int n;

	n = rbb_CameraAddBallsAhead(points);
	CRollerballCamera_FitPoints(self, eye, target, points[0], n, RBB_F(self, RBC_PITCH),
		RBB_F(self, RBC_YAW));
	rbb_CameraMoveTo(self, dt, flags, eye, target);
}

/* 0x00194330  (camera file unknown)
Play view of "race": as "race2", with the sole player view. */
void CRollerballCamera_FrameRace(void *self, float dt, unsigned char flags)
{
	float points[20][4];
	float eye[4];
	float target[4];
	int n;

	if (rbb_CameraFirstPerson(self))
		return;
	n = rbb_CameraAddBallsAhead(points);
	CRollerballCamera_FitPoints(self, eye, target, points[0], n, RBB_F(self, RBC_PITCH),
		RBB_F(self, RBC_YAW));
	rbb_CameraMoveTo(self, dt, flags, eye, target);
}

/* 0x001945e0  (camera file unknown)
Play view of "drop": every ball and the visible ones of five of the variation's
platforms (variation object + 0x58, stride 0x58). No sole player view, no pitch change. */
void CRollerballCamera_FrameDrop(void *self, float dt, unsigned char flags)
{
	float points[20][4];
	float eye[4];
	float target[4];
	char *var;
	int n;
	int i;

	if (RBB_I(self, RBC_MODE) < 1 || 2 < RBB_I(self, RBC_MODE))
		RBB_F(self, RBC_YAW) = dt * RBB_F(self, RBC_YAW_SPEED) + RBB_F(self, RBC_YAW);
	n = rbb_CameraAddBalls(points, 0);
	var = RBB_P(rbb_Game, 0x26f0);
	for (i = 0; i < 5; i++) {
		char *obj = RBB_P(var, 0x58 + i * 0x58);
		if (RBB_C(obj, 0x78) != 0) {
			memcpy(points[n], RBB_V(obj, 0x64), 16);
			n++;
		}
	}
	CRollerballCamera_FitPoints(self, eye, target, points[0], n, RBB_F(self, RBC_PITCH),
		RBB_F(self, RBC_YAW));
	rbb_CameraMoveTo(self, dt, flags, eye, target);
}

/* 0x00194800  (camera file unknown)
Mode 4: the end shot of the variation. */
void CRollerballCamera_UpdateEnd(void *self, float dt)
{
	switch (rbb_Variation) {
	case 0:
		FUN_00193120(self, dt);
		return;
	case 1:
		CRollerballCamera_EndShotArena(self, dt);
		return;
	case 2:
		FUN_00193260(self, dt);
		return;
	case 3:
		FUN_00193360(self, dt);
		return;
	case 4:
		CRollerballCamera_EndShotArena(self, dt);
		return;
	}
}

/* the play view of the variation */
static void rbb_CameraFrame(void *self, float dt, unsigned char flags)
{
	switch (rbb_Variation) {
	case 0:
		CRollerballCamera_FrameBase(self, dt, flags);
		break;
	case 1:
		CRollerballCamera_FrameSumo(self, dt, flags);
		break;
	case 2:
		CRollerballCamera_FrameRace2(self, dt, flags);
		break;
	case 3:
		CRollerballCamera_FrameRace(self, dt, flags);
		break;
	case 4:
		CRollerballCamera_FrameDrop(self, dt, flags);
		break;
	}
}

/* 0x00194850  (camera file unknown)
Mode 2: the count down. Cut to the play view; play starts (mode 3) when the HUD's count
down has ended. */
void CRollerballCamera_UpdateCountdown(void *self, float dt)
{
	rbb_CameraFrame(self, dt, 2);
	if (CLHud_UpdateCountdown(DAT_002ecfac) != 0)
		RBB_I(self, RBC_MODE) = 3;
}

/* 0x001948f0  (camera file unknown)
Mode 3: play (the view moves smoothly). */
void CRollerballCamera_UpdatePlay(void *self, float dt)
{
	rbb_CameraFrame(self, dt, 1);
}

/* 0x00194970  (camera file unknown)
Per-frame update: the mode, the requested fades, then the view. */
void CRollerballCamera_Update(void *self, float dt)
{
	rbb_TimerUpdate(RBB_ADDR(self, RBC_TIMER));
	switch (RBB_I(self, RBC_MODE)) {
	case 1:
		CRollerballCamera_UpdateIntro(self, dt);
		break;
	case 2:
		CRollerballCamera_UpdateCountdown(self, dt);
		break;
	case 3:
		CRollerballCamera_UpdatePlay(self, dt);
		break;
	case 4:
		CRollerballCamera_UpdateEnd(self, dt);
		break;
	}
	if (RBB_I(self, RBC_REQ_FADE) != 0) {
		RBB_I(self, RBC_FADE) = 1;
		RBB_I(self, RBC_REQ_FADE) = 0;
	}
	if (RBB_I(self, RBC_REQ_DARKEN_OUT) != 0) {
		RBB_I(self, RBC_DARKEN_OUT) = 1;
		RBB_I(self, RBC_REQ_DARKEN_OUT) = 0;
	}
	if (RBB_I(self, RBC_REQ_DARKEN_IN) != 0) {
		RBB_I(self, RBC_DARKEN_IN) = 1;
		RBB_I(self, RBC_REQ_DARKEN_IN) = 0;
	}
	bdSetFieldOfView(RBB_F(self, RBC_FOV), RBB_F(self, RBC_FOV) * 3.0f * 0.25f);
	bdSetCameraLookAt(RBB_V(self, RBC_EYE), RBB_V(self, RBC_TARGET), RBB_F(self, RBC_ROLL));
	bdUpdateView();
}
