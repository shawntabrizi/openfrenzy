/*
ScratchMatch/CScratchmatch_Camera.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CScratchmatch_Camera.cpp
(module scratchmatch)

The camera of Scratch Match: the intro fly-through (a classlib intro camera loaded from
cams\cam<variation>.lom), then during the game a camera that wanders between random offsets
and now and then closes up on the leading player and circles them; in the outro it shows the
winners one after another. Every mode eases the eye, the target and the field of view towards
a goal (SMCamera_MoveTowards).

A camera goal is a float[26]: eye (0-2), target (4-6), field-of-view scale (25).
*/

#include "scratchmatch.h"

/* 0x001bb630  (gap; before the camera file's first tagged function): sets the field of view
and the view matrix from the camera */
void SMCamera_SetView(void)
{
	bdSetFieldOfView(sm_CamFov * 1.06814158f, sm_CamFov * 0.816814065f);
	bdSetCameraLookAt(sm_CamPos, sm_CamTarget, 0.0f);
	bdUpdateView();
}

/* 0x001bb670  ScratchMatch/CScratchmatch_Camera.cpp: loads the intro camera path of the
variation (cams\cam<n>.lom) from `package` */
void SMCamera_LoadIntro(struct bkPackage *package)
{
	uint size;
	char *file;
	int *map;

	file = (char *)bkLoadFile(package, sm_CameraFiles[sm_GameVariation], NULL, &size, NULL, 0);
	if (file == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Camera.cpp", 0x14c, "POINTER ASSERTION FAILURE: pucFile");
	map = ParseMap_Load(file, 2, (int)size);
	if (map == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Camera.cpp", 0x14f, "POINTER ASSERTION FAILURE: pLevelInstance");
	CLIntroCam_Load((int)(uintptr_t)map, 0);
	ParseMap_Free(map);
}

/* 0x001bb700  ScratchMatch/CScratchmatch_Camera.cpp: one frame of the intro camera; returns
1 when it has finished (or was skipped), else 0 */
int SMCamera_UpdateIntro(void)
{
	int result;

	result = CLIntroCam_Update((float)(SMTimer_FrameTime() * 60.0f), 0);
	CLIntroCam_GetPosition(sm_CamPos, CLIntroCam_GetTime());
	CLIntroCam_GetTarget(sm_CamTarget, CLIntroCam_GetTime());
	if (result == 0) {
		if (sm_IntroSkipped == 0)
			return 0;
	} else if (result == -1) {
		SM_ASSERT_FAIL("CScratchmatch_Camera.cpp", 0x174, "ASSERTION FAILURE: FALSE");
	}
	CLIntroCam_Stop();
	return 1;
}

/* 0x001bb790  (gap): a thunk of classlib_ui's CLIntroCam_Destroy (0x0007d590); other
minigames call it too */
void SMCamera_DestroyIntro(void)
{
	CLIntroCam_Destroy();
}

/* 0x001bb7a0  (gap): a thunk of classlib_ui's CLIntroCam_Restart (0x0007cc30) */
int SMCamera_RestartIntro(void)
{
	return CLIntroCam_Restart();
}

/* 0x001bb7b0  (gap; the camera's initialiser): the start position and the default tweak
values, and a random 20-30 s delay before the first close-up. The original returns a
leftover of the random number (rand / 1000), which nobody uses. */
unsigned int SMCamera_Init(void)
{
	unsigned int random;

	sm_CamCloseUp = 0;
	sm_CamPos[0] = 0.0f;
	sm_CamTarget[0] = 0.0f;
	sm_CamTarget[2] = 0.0f;
	sm_CamLeaderHeading = 0.0f;
	sm_CamOffset[0] = 0.0f;
	sm_CamOffset[1] = 0.0f;
	sm_CamOffset[2] = 0.0f;
	sm_CamOffsetFrom[0] = 0.0f;
	sm_CamOffsetFrom[1] = 0.0f;
	sm_CamOffsetFrom[2] = 0.0f;
	SM_F(0x0030ff60) = 0.0f;
	sm_CamTargetZ = 0.0f;
	SM_F(0x0030ff8c) = 0.0f;
	SM_F(0x0030ff90) = 0.0f;
	random = sm_rand();
	sm_CamPos[1] = 880.0f;
	sm_CamPos[2] = -1800.0f;
	sm_CamTarget[1] = -200.0f;
	sm_CamPlayerY = 800.0f;
	sm_CamPlayerZ = -1800.0f;
	sm_CamTargetY = 800.0f;
	SM_U(0x0030ff78) = 0x3e2db7afu;   /* 0.16964 */
	SM_U(0x0030ff7c) = 0xbd80adfdu;   /* -0.062832 */
	sm_CamFov = 1.0f;
	sm_CamCloseUpDelay = (float)(random % 1000) * 0.01f + 20.0f;
	return random / 1000;
}

/* 0x001bb8a0  (gap): eases the camera towards `goal` by `rate` (at most 1), then moves the eye
back by `pullBack` along the goal's view direction */
void SMCamera_MoveTowards(float *goal, float rate, float pullBack)
{
	float dir[3];
	float dy, dz, tx, ty, tz;

	if (rate > 1.0f)
		rate = 1.0f;
	dy = goal[1] - sm_CamPos[1];
	tx = goal[4] - sm_CamTarget[0];
	ty = goal[5] - sm_CamTarget[1];
	sm_CamFov = sm_CamFov + (goal[25] - sm_CamFov) * rate;
	dz = (goal[2] - sm_CamPos[2]) * rate;
	sm_CamPos[0] = sm_CamPos[0] + (goal[0] - sm_CamPos[0]) * rate;
	sm_CamPos[1] = sm_CamPos[1] + dy * rate;
	sm_CamPos[2] = sm_CamPos[2] + dz;
	tz = (goal[6] - sm_CamTarget[2]) * rate;
	sm_CamTarget[0] = sm_CamTarget[0] + tx * rate;
	sm_CamTarget[1] = sm_CamTarget[1] + ty * rate;
	sm_CamTarget[2] = sm_CamTarget[2] + tz;
	dir[0] = goal[4] - goal[0];
	dir[1] = goal[5] - goal[1];
	dir[2] = goal[6] - goal[2];
	bmVectorNormalise(dir, dir);
	pullBack = -pullBack;
	sm_CamPos[0] = sm_CamPos[0] + dir[0] * pullBack;
	sm_CamPos[1] = sm_CamPos[1] + dir[1] * pullBack;
	sm_CamPos[2] = sm_CamPos[2] + dir[2] * pullBack;
}

/* 0x001bba70  (gap): the outro camera: every sm_LeaderPeriod seconds (after 2 s) the next of
the players sharing first place, seen from behind and above (a fade-in on each change) */
void SMCamera_FollowLeader(void)
{
	float goal[26];
	float offset[3];
	struct SMPlayer *player;
	unsigned int rank;

	rank = (unsigned int)SM_FTOL((SMTimer_Elapsed(SMT_GAME) - 2.0f) / sm_LeaderPeriod);
	if (rank >= 0x7fffffffu)
		rank = 0;
	if (rank >= sm_Leaders)
		rank = sm_Leaders - 1;
	if (sm_Leader != rank && SMTimer_Elapsed(SMT_GAME) < sm_ufloat(sm_Leaders) * sm_LeaderPeriod) {
		sm_Leader = rank;
		sm_FadeIn = 1;
	}
	player = &sm_Players[sm_Rank[rank]];
	sm_CamLeaderHeading = player->heading;
	sm_CamOffset[0] = 0.0f;
	sm_CamOffset[1] = 200.0f;
	sm_CamOffset[2] = -300.0f;
	bmVectorRotateY(offset, sm_CamOffset, player->heading);
	goal[0] = offset[0] + player->pod.pos[0];
	goal[4] = player->pod.pos[0];
	goal[25] = 1.25f;
	goal[1] = offset[1] + player->pod.pos[1];
	goal[2] = offset[2] + player->pod.pos[2];
	goal[6] = player->pod.pos[2];
	goal[5] = player->pod.pos[1] + 120.0f;
	SMCamera_MoveTowards(goal, (float)(SMTimer_FrameTime() * 25.0f), 0.0f);
}

/* 0x001bbbd0  (gap): the game camera. Normally it eases between random offsets around a
fixed spot (a new offset every 4 s); every 12-22 s it closes up on the leading player for 6 s,
circling them. The goal is built as base + offset, and the close-up's base already contains
the offset (so the circle offset counts twice), as in the original. */
void SMCamera_Wander(void)
{
	float goal[26];
	float offset[3];
	double k;
	float angle;
	struct SMPlayer *player;
	unsigned int r1, r2, r3;

	if (SMTimer_Elapsed(SMT_CLOSEUP) > sm_CamCloseUpDelay) {
		sm_CamCloseUp = 1;
		SMTimer_Reset(SMT_CLOSEUP);
		r1 = sm_rand();
		sm_Wander = 0;
		sm_CamCloseUpPlayer = sm_Rank[0];
		sm_CamCloseUpDelay = (float)(int)(r1 % 1000) * 0.01f + 12.0f;
	}
	if (sm_CamCloseUp == 1 && SMTimer_Elapsed(SMT_CLOSEUP) <= 6.0) {
		/* close-up: circle the player */
		player = &sm_Players[sm_CamCloseUpPlayer];
		goal[0] = player->pos[0];
		goal[1] = player->pos[1];
		goal[2] = player->pos[2];
		goal[3] = player->pos[3];
		goal[4] = player->pos[0];
		goal[5] = player->pos[1] + 150.0f;
		goal[6] = player->pos[2];
		goal[7] = player->pos[3];
		angle = (float)(player->unk45c - (SMTimer_Elapsed(SMT_CLOSEUP) * 0.166666672f - 0.5f) * 6.28318548f);
		goal[25] = 1.25f;
		offset[0] = (float)(sin(angle) * 150.0f);
		offset[1] = 100.0f;
		offset[2] = (float)(cos(angle) * 150.0f);
		goal[0] = goal[0] + offset[0];
		goal[1] = goal[1] + offset[1];
		goal[2] = goal[2] + offset[2];
	} else {
		if (sm_CamCloseUp == 1) {
			sm_CamCloseUp = 0;
			sm_Wander = 1;
		}
		/* wander: between the previous and the next offset */
		offset[0] = sm_CamOffset[0] - sm_CamOffsetFrom[0];
		goal[1] = sm_CamPlayerY;
		offset[1] = sm_CamOffset[1] - sm_CamOffsetFrom[1];
		goal[0] = 0.0f;
		goal[2] = sm_CamPlayerZ;
		goal[4] = 0.0f;
		goal[5] = sm_CamTargetY;
		goal[6] = sm_CamTargetZ;
		offset[2] = sm_CamOffset[2] - sm_CamOffsetFrom[2];
		k = SMTimer_Elapsed(SMT_CAMERA) * 0.25f;
		goal[25] = 1.0f;
		offset[0] = (float)(offset[0] * k);
		offset[1] = (float)(offset[1] * k);
		offset[2] = (float)(k * offset[2]);
		offset[0] = sm_CamOffsetFrom[0] + offset[0];
		offset[1] = sm_CamOffsetFrom[1] + offset[1];
		offset[2] = sm_CamOffsetFrom[2] + offset[2];
	}
	goal[0] = goal[0] + offset[0];
	goal[1] = goal[1] + offset[1];
	goal[2] = goal[2] + offset[2];
	if (SMTimer_Elapsed(SMT_CAMERA) > 4.0 && sm_CamCloseUp == 0) {
		SMTimer_Reset(SMT_CAMERA);
		sm_CamOffsetFrom[2] = sm_CamOffset[2];
		sm_CamOffsetFrom[1] = sm_CamOffset[1];
		sm_CamOffsetFrom[0] = sm_CamOffset[0];
		SM_U(0x0030ff5c) = SM_U(0x0030ff4c);
		r1 = sm_rand();
		r2 = sm_rand();
		r3 = sm_rand();
		sm_CamOffset[0] = ((float)(int)(r1 % 1000) - 500.0f) * 0.5f;
		sm_CamOffset[1] = ((float)(int)(r2 % 2000) - 1000.0f) * 0.5f;
		sm_CamOffset[2] = ((float)(int)(r3 % 2000) - 1000.0f) * 0.5f;
	}
	SMCamera_MoveTowards(goal, (float)(SMTimer_FrameTime() * 1.14999998f), 0.0f);
}
