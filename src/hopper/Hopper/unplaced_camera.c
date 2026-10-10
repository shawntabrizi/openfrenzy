/*
Hopper/unplaced_camera.c  <-  gap functions 0x00141920-0x00142740 (no source path in the XBE)
(module hopper)

The camera object (0x002aebd8, vtable 0x0026edb8, 0x34 bytes):
  +0x04 float[4]  eye                 +0x14 float[4]  target
  +0x24 float     the camera's angle around the arena (degrees)
  +0x28 byte      the state of play (hop_Play): 0 intro camera path, 1 intro done,
                  2 countdown, 5 playing
  +0x2c int (0x40)                    +0x30 float  variation 4: the single player view's drop
The intro runs the .lom camera path (classlib_ui's CLIntroCam); then the camera eases
(1/16 a frame) to the variation's view and the HUD counts down; while playing it circles
the arena (variation 4 follows a single human player along the arm). Each AI player's
stick directions are relative to the camera (AI +0x1118).
*/

#include "hopper.h"

#define CAM_EYE(self) HOP_AV(self, 0x04)
#define CAM_TARGET(self) HOP_AV(self, 0x14)
#define CAM_ANGLE(self) HOP_AF(self, 0x24)
#define CAM_PLAY(self) HOP_AB(self, 0x28)
#define CAM_DROP(self) HOP_AF(self, 0x30)

extern float DAT_002f7a2c;             /* charlib (CCamera.cpp): the camera heading */
extern void *DAT_002f26c4;             /* bumpercars_b: the debug free camera */
extern int DAT_002baeb8;               /* core: skip the intro */

#define hop_SpeedBoost HOP_F(0x002aed74)       /* variation 4: the arm's turn speed (degrees a frame) */
#define hop_SinkSpeed HOP_F(0x002af160)        /* variation 4: how fast the arena sinks */

/* 0x00141920  (gap): the camera's start */
void HopCamera_Start(void *self)
{
	HOP_AF(self, 0x14) = 0.0f;
	HOP_AF(self, 0x18) = 0.0f;
	HOP_AF(self, 0x1c) = 0.0f;
	HOP_AF(self, 0x04) = 0.0f;
	HOP_AF(self, 0x08) = 0.0f;
	HOP_AF(self, 0x0c) = 0.0f;
	CAM_ANGLE(self) = 0.0f;
	CAM_DROP(self) = 0.0f;
	HOP_AI(self, 0x2c) = 0x40;
	CAM_PLAY(self) = 0;
	if (hop_Variation == 4)
		CAM_ANGLE(self) = 60.0f;
}

/* 0x00141960  (gap): sets the view. During the intro the eye and target follow the camera
path (the view itself is set only while paused; the path sets it otherwise); with the
debug free camera nothing is done. */
void HopCamera_Apply(void *self)
{
	float pos[4];
	float target[4];
	float roll;
	float t;

	if (DAT_002f26c4 != NULL)
		return;
	if (CAM_PLAY(self) != 0) {
		bdSetCameraLookAt(CAM_EYE(self), CAM_TARGET(self), 0.0f);
		bdUpdateView();
		return;
	}
	t = CLIntroCam_GetTime();
	CLIntroCam_GetPosition(pos, t);
	CLIntroCam_GetTarget(target, t);
	CLIntroCam_GetRoll(&roll, t);
	HOP_AF(self, 0x04) = pos[0];
	HOP_AF(self, 0x08) = pos[1];
	HOP_AF(self, 0x0c) = pos[2];
	HOP_AF(self, 0x14) = target[0];
	HOP_AF(self, 0x18) = target[1];
	HOP_AF(self, 0x1c) = target[2];
	if (DAT_002ecdec != 0) {
		bdSetCameraLookAt(pos, target, roll);
		bdUpdateView();
	}
}

/* 0x00141a20  (gap): scalar deleting destructor */
void *HopCamera_Destroy(void *self, byte flags)
{
	*(const void *const **)self = vtable_0026edb8;
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026edb8  the camera's vtable */
const void *const vtable_0026edb8[1] = {
	(const void *)FUN_00141a20, /* [0] scalar deleting destructor */
};

/* 0x00141a40  (gap): variation 1 while playing: circles the arena (-0.1 degrees a frame),
1500 out, 800 up, looking at y -500 */
void HopCamera_Play1(void *self)
{
	float a;

	a = CAM_ANGLE(self) - 0.1f;
	CAM_ANGLE(self) = a;
	if (a < 1.0f)
		CAM_ANGLE(self) = a + 360.0f;
	HOP_F(0x002aeac4) = hop_CamAngle * HOP_DEG;
	DAT_002f7a2c = CAM_ANGLE(self) * HOP_DEG;
	HOP_AF(self, 0x18) = -500.0f;
	a = CAM_ANGLE(self) * HOP_DEG;
	HOP_AF(self, 0x08) = 800.0f;
	HOP_AF(self, 0x04) = sinf(a) * -1500.0f;
	HOP_AF(self, 0x0c) = cosf(a) * -1500.0f;
}

/* 0x00141ac0  (gap): variation 2 while playing: circles (-0.2 degrees a frame), 2200 out */
void HopCamera_Play2(void *self)
{
	float a;

	a = CAM_ANGLE(self) - 0.2f;
	CAM_ANGLE(self) = a;
	if (a < 1.0f)
		CAM_ANGLE(self) = a + 360.0f;
	DAT_002f7a2c = CAM_ANGLE(self) * HOP_DEG;
	HOP_F(0x002aeac4) = hop_CamAngle * HOP_DEG;
	HOP_AF(self, 0x18) = -500.0f;
	a = CAM_ANGLE(self) * HOP_DEG;
	HOP_AF(self, 0x08) = 800.0f;
	HOP_AF(self, 0x04) = sinf(a) * -2200.0f;
	HOP_AF(self, 0x0c) = cosf(a) * -2200.0f;
}

/* 0x00141b40  (gap): variation 3 while playing: sways +-10 degrees (frame counter
0x003027dc), 3000 out, bobbing 800 +- 200, looking at y -400 */
void HopCamera_Play3(void *self)
{
	float t, s, a;

	HOP_I(0x003027dc) = HOP_I(0x003027dc) + 1;
	t = (float)HOP_I(0x003027dc);
	s = sinf(HOP_DEG * t);
	CAM_ANGLE(self) = s * 10.0f;
	DAT_002f7a2c = s * 10.0f * HOP_DEG;
	HOP_AF(self, 0x18) = -400.0f;
	a = CAM_ANGLE(self) * HOP_DEG;
	HOP_AF(self, 0x04) = sinf(a) * -3000.0f;
	HOP_AF(self, 0x08) = sinf(t * 0.8f * HOP_DEG) * 200.0f + 800.0f;
	HOP_AF(self, 0x0c) = cosf(a) * -3000.0f;
}

/* 0x00141bc0  (gap): variation 4 while playing: the camera turns with the arm; the arena
sinks (wrapping by 9463 with the players); with one human player (0x003027c8) the camera
follows him along the arm until he is 60 along it. */
void HopCamera_Play4(void *self)
{
	float rot[4];
	float v[4];
	float a, f, r;
	void *player;
	void *actor;
	int i, single;

	a = CAM_ANGLE(self) - hop_SpeedBoost;
	CAM_ANGLE(self) = a;
	if (a < 0.0f)
		CAM_ANGLE(self) = a + 360.0f;
	if (360.0f < CAM_ANGLE(self))
		CAM_ANGLE(self) = CAM_ANGLE(self) - 360.0f;
	DAT_002f7a2c = (CAM_ANGLE(self) - 90.0f) * HOP_DEG;
	if (hop_SpeedBoost == 0.0f)
		hop_SinkSpeed = 0.0f;
	else
		hop_SinkSpeed = 1182.9f / (360.0f / hop_SpeedBoost);
	if (DAT_002ecdec == 0)
		hop_Sink = hop_Sink - hop_SinkSpeed;
	if (hop_Sink <= -9463.0f) {
		hop_Sink = hop_Sink + 9463.0f;
		for (i = 0; i < DAT_002bae94; i++) {
			actor = hop_PlayerActor(i);
			HOP_AF(actor, 0xd4) = HOP_AF(actor, 0xd4) - 9463.0f;
			HOP_FV(0x002ae8bc)[i] = HOP_AF(hop_PlayerActor(i), 0xd4);
			hop_ActorSnap(hop_PlayerActor(i));
		}
	}
	f = 100.0f;
	if (HOP_I(0x003027c8) != 0) {
		single = HOP_I(0x003027cc);
		player = hop_Players[single];
		if (HOP_AF(player, 0x1180) <= 60.0f) {
			a = (CAM_ANGLE(self) - ((125.0f - HOP_AF(player, 0x1180)) + 2.0f)) * HOP_DEG;
			if (HOP_AS(player, 0x36) == 0xe)
				f = 100.0f - CAM_DROP(self);
			else
				f = -CAM_DROP(self);
			CAM_DROP(self) = f * 0.25f + CAM_DROP(self);
			r = (float)single * 140.0f + 800.0f;
			HOP_AF(self, 0x04) = -(r * sinf(a));
			HOP_AF(self, 0x08) = (HOP_AF(hop_PlayerActor(single), 0xd4) + 250.0f) - CAM_DROP(self);
			HOP_AF(self, 0x0c) = -(r * cosf(a));
			v[0] = -1000.0f;
			v[1] = 0.0f;
			v[2] = 0.0f;
			v[3] = 1.0f;
			bmVectorRotateY(rot, v, (HOP_AF(hop_Players[single], 0x1180) + 125.0f) * HOP_DEG + DAT_002f7a2c);
			HOP_AF(self, 0x14) = rot[0];
			HOP_AF(self, 0x18) = HOP_AF(self, 0x08);
			HOP_AF(self, 0x1c) = rot[2];
			return;
		}
		HOP_I(0x003027c8) = 0;
	}
	HOP_AF(self, 0x14) = 0.0f;
	HOP_AF(self, 0x1c) = 0.0f;
	HOP_AF(self, 0x18) = f - hop_Sink;
	a = (CAM_ANGLE(self) - 90.0f) * HOP_DEG;
	HOP_AF(self, 0x04) = sinf(a) * -2600.0f;
	HOP_AF(self, 0x08) = HOP_AF(self, 0x18) + 700.0f;
	HOP_AF(self, 0x0c) = cosf(a) * -2600.0f;
}

/* v -= (v - goal) / 16 */
static inline float hop_Ease(float v, float goal)
{
	return v - (v - goal) * 0.0625f;
}

/* 0x00141f00  (gap): after the intro: eases the eye and target to the variation's view */
void HopCamera_EaseIn(void *self)
{
	float ty, s, a, r;

	if (DAT_002ecdec != 0)
		return;
	switch (hop_Variation) {
	case 1:
	case 2:
		/* variation 1: 1500 out, variation 2: 2200 out */
		r = hop_Variation == 1 ? -1500.0f : -2200.0f;
		HOP_AF(self, 0x14) = HOP_AF(self, 0x14) - HOP_AF(self, 0x14) * 0.0625f;
		ty = hop_Ease(HOP_AF(self, 0x18), -500.0f);
		HOP_AF(self, 0x18) = ty;
		HOP_AF(self, 0x1c) = HOP_AF(self, 0x1c) - HOP_AF(self, 0x1c) * 0.0625f;
		HOP_AF(self, 0x04) = hop_Ease(HOP_AF(self, 0x04), sinf(CAM_ANGLE(self) * HOP_DEG) * r);
		HOP_AF(self, 0x08) = hop_Ease(HOP_AF(self, 0x08), ty + 1300.0f);
		HOP_AF(self, 0x0c) = hop_Ease(HOP_AF(self, 0x0c), cosf(CAM_ANGLE(self) * HOP_DEG) * r);
		return;
	case 3:
		s = (float)sin(0.0);
		CAM_ANGLE(self) = 10.0f * s;
		HOP_AF(self, 0x14) = HOP_AF(self, 0x14) - HOP_AF(self, 0x14) * 0.0625f;
		ty = hop_Ease(HOP_AF(self, 0x18), -400.0f);
		HOP_AF(self, 0x18) = ty;
		HOP_AF(self, 0x1c) = HOP_AF(self, 0x1c) - HOP_AF(self, 0x1c) * 0.0625f;
		a = 10.0f * s * HOP_DEG;
		HOP_AF(self, 0x04) = hop_Ease(HOP_AF(self, 0x04), sinf(a) * -3000.0f);
		HOP_AF(self, 0x08) = hop_Ease(HOP_AF(self, 0x08), s * 200.0f + ty + 1200.0f);
		HOP_AF(self, 0x0c) = hop_Ease(HOP_AF(self, 0x0c), cosf(a) * -3000.0f);
		return;
	case 4:
		a = CAM_ANGLE(self) - hop_SpeedBoost;
		CAM_ANGLE(self) = a;
		if (a < 0.0f)
			CAM_ANGLE(self) = a + 360.0f;
		if (360.0f < CAM_ANGLE(self))
			CAM_ANGLE(self) = CAM_ANGLE(self) - 360.0f;
		HOP_AF(self, 0x14) = HOP_AF(self, 0x14) - HOP_AF(self, 0x14) * 0.0625f;
		HOP_AF(self, 0x18) = hop_Ease(HOP_AF(self, 0x18), 100.0f - hop_Sink);
		HOP_AF(self, 0x1c) = HOP_AF(self, 0x1c) - HOP_AF(self, 0x1c) * 0.0625f;
		a = (CAM_ANGLE(self) - 90.0f) * HOP_DEG;
		HOP_AF(self, 0x04) = hop_Ease(HOP_AF(self, 0x04), sinf(a) * -2600.0f);
		HOP_AF(self, 0x08) = hop_Ease(HOP_AF(self, 0x08), HOP_AF(self, 0x18) + 700.0f);
		HOP_AF(self, 0x0c) = hop_Ease(HOP_AF(self, 0x0c), cosf(a) * -2600.0f);
		return;
	}
}

/* variation 4 before the play: the players stand on the arm at height 210 (angle: the
arm's, less 125 and, when `along`, plus how far along each player is) */
static void hop_PlacePlayersOnArm(int along)
{
	float a, r;
	int i;

	for (i = 0; i < 4; i++) {
		HOP_AF(hop_PlayerActor(i), 0xd4) = 210.0f;
		if (along)
			a = (hop_CamAngle - (125.0f - HOP_AF(hop_Players[i], 0x1180))) * HOP_DEG;
		else
			a = (hop_CamAngle - 125.0f) * HOP_DEG;
		r = (float)HOP_IV(0x002ae8cc)[i] * 140.0f + 800.0f;
		HOP_AF(hop_PlayerActor(i), 0xd0) = -(r * sinf(a));
		HOP_AF(hop_PlayerActor(i), 0xd8) = -(r * cosf(a));
		hop_ActorSnap(hop_PlayerActor(i));
	}
}

/* a human player pressed one of the two buttons */
static int hop_HumanPressed(int i)
{
	int pressed = 0;

	if (DAT_002baed4[i][1] != 0) {
		if (HOP_AI(HOP_AP(hop_Players[i], 0x68), 0x14) != 0)
			pressed = 1;
		if (HOP_AI(HOP_AP(hop_Players[i], 0x6c), 0x14) != 0)
			pressed = 1;
	}
	return pressed;
}

/* 0x00142200  (gap): the camera per frame: the intro camera path (a human button skips it),
then the countdown (the camera eases in; at the end the players are released and the
play starts, state 5), then the variation's camera. Afterwards every AI player's stick is
made relative to the camera. */
void HopCamera_Update(void *self)
{
	float pos[4];
	float target[4];
	void *actor;
	int done, frames, i;

	if (hop_RoundOver != 0)
		return;
	switch (CAM_PLAY(self)) {
	case 0:
		done = CLIntroCam_Update(1.0f, 0);
		Actor_UpdateAnimations();
		if (hop_HumanPressed(0))
			done = 1;
		if (hop_HumanPressed(1))
			done = 1;
		if (hop_HumanPressed(2))
			done = 1;
		if (hop_HumanPressed(3))
			done = 1;
		if (DAT_002baeb8 == 0) {
			if (done == 0) {
				if (hop_Variation != 4)
					return;
				hop_PlacePlayersOnArm(1);
				return;
			}
			if (done == -1) {
				bkPrintf("***CAMERA ERROR***\n");
				goto playing;
			}
		}
		frames = CLIntroCam_GetFrameCount();
		CLIntroCam_GetPosition(pos, (float)frames);
		CLIntroCam_GetTarget(target, (float)frames);
		HOP_AF(self, 0x04) = pos[0];
		HOP_AF(self, 0x08) = pos[1];
		HOP_AF(self, 0x0c) = pos[2];
		HOP_AF(self, 0x14) = target[0];
		HOP_AF(self, 0x18) = target[1];
		HOP_AF(self, 0x1c) = target[2];
		CLIntroCam_Stop();
		CAM_PLAY(self) = 1;
		bdSetFieldOfView(1.2217305f, 0.9162979f);
		baAddExtraItem(HOP_G(0x002afff8, struct BExtraSystem *), 24.0);
		memset(HOP_ADDR(0x002af11c), 0, 5 * 4);
		HopCamera_EaseIn(self);
		return;
	case 1:
		if (hop_Variation == 4)
			hop_PlacePlayersOnArm(0);
		CLHud_StartBars((float *)(uintptr_t)DAT_002ecfac, 280.0f);
		/* fall through */
	case 2:
		CAM_PLAY(self) = 2;
		HopCamera_EaseIn(self);
		if (CLHud_UpdateCountdown(DAT_002ecfac) == 0) {
			if (DAT_002ecdec == 0)
				Actor_UpdateAnimations();
			return;
		}
		CAM_PLAY(self) = 5;
		for (i = 0; i < 4; i++)
			HOP_VFN(hop_Players[i], 0x40, void (*)(void *, int, int))(hop_Players[i], 1, 0);
		if (hop_Variation < 3)
			CLHud_StopCountdown(DAT_002ecfac);
		break;
	default:
		break;
	}
playing:
	if (DAT_002f26c4 != NULL) {
		BCarFreeCamera_Update((struct BCarFreeCamera *)DAT_002f26c4, 1.0f);
		return;
	}
	switch (hop_Variation) {
	case 1:
		HopCamera_Play1(self);
		break;
	case 2:
		HopCamera_Play2(self);
		break;
	case 3:
		HopCamera_Play3(self);
		break;
	case 4:
		HopCamera_Play4(self);
		break;
	}
	for (i = 0; i < DAT_002bae94; i++) {
		actor = HOP_AP(DAT_002f289c[i], 0x30);
		HOP_AF(DAT_002f289c[i], 0x1118) = CLMath_WrapAngle(atan2f(HOP_AF(actor, 0xd0) - HOP_AF(self, 0x04),
			HOP_AF(actor, 0xd8) - HOP_AF(self, 0x0c)));
	}
}
