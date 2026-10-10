/*
ScratchMatch/CScratchmatch_Render.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CScratchmatch_Render.cpp
(module scratchmatch)

Drawing Scratch Match: the arena and the set (models\arena.obe, set.obe, backlights.obe), the
three animated floor textures (animtex\flrlit<a-c>.gif), the players on their platforms, the
lights and effects, then the 2D HUD and the fades.
*/

#include "scratchmatch.h"

/* an animated texture (0x1c bytes from 0x002b5da0: name, texture, frames, frame) */
#define sm_AnimTexName(i) (SM_A(0x002b5da0) + 0x1c * (i))
#define sm_AnimTexTexture(i) SM_G(0x002b5db0 + 0x1c * (i), struct bdTexture *)
#define sm_AnimTexFrames(i) SM_I(0x002b5db4 + 0x1c * (i))
#define sm_AnimTexFrame(i) SM_I(0x002b5db8 + 0x1c * (i))

/* the full model of a player when the camera is close (close-up or outro), else the low one */
static struct BActorInstance *sm_PlayerModel(int index)
{
	if ((sm_Wander == 0 && sm_CamCloseUpPlayer == index) || sm_State == 3)
		return &sm_Players[index].inst;
	return sm_Players[index].lowInst;
}

/* 0x001bd480  (gap; placed with the drawing code it belongs to): the speakers, the players
(full or low model) under the static lights, the light pods, the hovering lights, the
effects, the platforms, the light beams and the smoke */
void SMRender_DrawPlayers(void)
{
	struct BActorInstance *model;
	int i;

	bdSetWorldIdentity();
	bdSetProjectionMode(1);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(5, 1, 0);
	baSetAmbientLight(0x46, 0x46, 0x28);
	baSetBrightness(0x100);
	SMScenic_Draw(sm_Speakers);
	SMLights_DisableSpotLights();
	SMLights_EnableStaticLights();
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		if (sm_Players[i].visible == 1) {
			bdSetWorldIdentity();
			if (sm_DrawActors == 1) {
				model = sm_PlayerModel(i);
				baDrawActorInstance(model, 1);
				baDrawActorInstance(model, 4);
			}
		}
	}
	SMLights_DisableStaticLights();
	SMLights_CheckSpotLights();
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	SMLights_DrawPods(1);
	SMGenActors_Draw();
	SMEffects_Draw();
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		if (sm_DrawActors == 1) {
			baDrawActorInstance(&sm_Players[i].pod, 1);
			baDrawActorInstance(&sm_Players[i].pod, 4);
		}
	}
	bdSetRenderState(10, 1, 0);
	SMLights_DrawPods(0);
	SMEffects_DrawSmoke();
}

/* 0x001bd5f0  (gap; placed with the drawing code): the arena set: the opaque pass darkened
through texture stage 1, then the other passes */
void SMRender_DrawArena(void)
{
	bdSetWorldMatrix(DAT_002855f0);
	bdSetProjectionMode(1);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(0, 0, 0);
	bdSetRenderState(4, 1, 0);
	bdSetRenderState(5, 1, 0);
	baSetAmbientLight(sm_Ambient, sm_Ambient, sm_Ambient);
	baSetBrightness(0x100);
	bdSetRenderState(5, 1, 0);
	SMEffects_SetDarkenStage(0x80);
	bdSetRenderState(5, 1, 0);
	baDrawSet(sm_Arena, 1);
	SMEffects_ResetDarkenStage();
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 0, 0);
	SMEffects_SetDarkenStage(0x80);
	baDrawSet(sm_Arena, 2);
	baDrawSet(sm_Arena, 4);
	SMEffects_ResetDarkenStage();
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* 0x001bd700  (gap; placed with the drawing code): the render states of the 2D pass */
void SMRender_Begin2D(void)
{
	bdSetProjectionMode(0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(5, 0, 0);
	baSetBrightness(0x100);
}

/* 0x001bd750  ScratchMatch/CScratchmatch_Render.cpp: loads the three animated floor textures */
void SMRender_LoadAnimTextures(void)
{
	char name[32];
	int i;

	for (i = 0; i < 3; i++) {
		wsprintfA(name, "animtex\\%s.gif", sm_AnimTexName(i));
		sm_AnimTexTexture(i) = (struct bdTexture *)bLoadResource(NULL, name, 0, 0xdefa);
		if (sm_AnimTexTexture(i) == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Render.cpp", 0x299,
				"POINTER ASSERTION FAILURE: maxAnimTextures[ un32Texture ].pTexture");
	}
}

/* 0x001bd7c0  ScratchMatch/CScratchmatch_Render.cpp: frees the animated textures */
void SMRender_FreeAnimTextures(void)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (sm_AnimTexTexture(i) == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Render.cpp", 0x2a4,
				"ASSERTION FAILURE: maxAnimTextures[ un32Texture ].pTexture != NULL");
		bkDeleteResource((struct bkResource *)sm_AnimTexTexture(i));
		sm_AnimTexTexture(i) = NULL;
	}
}

/* 0x001bd810  ScratchMatch/CScratchmatch_Render.cpp (bracketed): the next frame of each
animated texture every 0.1 s */
void SMRender_UpdateAnimTextures(void)
{
	int i;

	if (SMTimer_Elapsed(SMT_ANIMTEX) > 0.1f) {
		for (i = 0; i < 3; i++) {
			sm_AnimTexFrame(i) = sm_AnimTexFrame(i) + 1;
			if (sm_AnimTexFrame(i) == sm_AnimTexFrames(i))
				sm_AnimTexFrame(i) = 0;
			bdTextureSetFrame(sm_AnimTexTexture(i), sm_AnimTexFrame(i));
		}
		SMTimer_Reset(SMT_ANIMTEX);
	}
}

/* 0x001bd880  ScratchMatch/CScratchmatch_Render.cpp: the render variables and the sets */
void SMRender_Init(void)
{
	memset(SM_A(0x003101e0), 0, 9 * 4);
	SM_I(0x00310200) = 1;
	sm_StageHeight = 350.0f;
	sm_FadeState = 0;
	sm_FadeIn = 0;
	sm_Ambient = 0x7f;
	sm_FadeAlpha = 0.0f;
	bkPrintf("Loading sets...\n");
	sm_Arena = (struct BSet *)bLoadResource(sm_VarPackage, "models\\arena.obe", 6, 0xdefa);
	if (sm_Arena == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Render.cpp", 0x59, "POINTER ASSERTION FAILURE: mpArena");
	sm_Set = (struct BSet *)bLoadResource(sm_Package, "models\\set.obe", 6, 0xdefa);
	if (sm_Set == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Render.cpp", 0x5c, "POINTER ASSERTION FAILURE: mpSet");
	sm_SetLights = (struct BSet *)bLoadResource(sm_Package, "models\\backlights.obe", 6, 0xdefa);
	if (sm_SetLights == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Render.cpp", 0x5f, "POINTER ASSERTION FAILURE: mpSetLights");
	bkPrintf("Loaded sets\n");
	SMRender_LoadAnimTextures();
}

/* 0x001bd980  (gap; frees what SMRender_Init loaded) */
void SMRender_Free(void)
{
	SMRender_FreeAnimTextures();
	if (sm_SetLights != NULL) {
		bkDeleteResource((struct bkResource *)sm_SetLights);
		sm_SetLights = NULL;
	}
	if (sm_Set != NULL) {
		bkDeleteResource((struct bkResource *)sm_Set);
		sm_Set = NULL;
	}
	if (sm_Arena != NULL) {
		bkDeleteResource((struct bkResource *)sm_Arena);
		sm_Arena = NULL;
	}
}

/* wraps an angle into [0, 2 pi] by whole turns; exactly 2 pi becomes 0 */
static float sm_WrapAngle(float angle)
{
	if (angle == 6.28318548f)
		return 0.0f;
	while (angle < 0.0f)
		angle = angle + 6.28318548f;
	while (6.28318548f < angle)
		angle = angle - 6.28318548f;
	return angle;
}

/* 0x001bd9e0  (gap; placed with its callers): sets the rotation of `instance` from Euler
angles (x, then y, then z) */
void SMRender_SetRotation(struct BActorInstance *instance, float x, float y, float z)
{
	float axisAngle[4];
	float qx[4], qy[4], qz[4], qxy[4], q[4];

	x = sm_WrapAngle(x);
	y = sm_WrapAngle(y);
	z = sm_WrapAngle(z);
	axisAngle[0] = 1.0f;
	axisAngle[1] = 0.0f;
	axisAngle[2] = 0.0f;
	axisAngle[3] = x;
	bmAxisAngleToQuaternion(qx, axisAngle);
	axisAngle[0] = 0.0f;
	axisAngle[1] = 1.0f;
	axisAngle[2] = 0.0f;
	axisAngle[3] = y;
	bmAxisAngleToQuaternion(qy, axisAngle);
	axisAngle[0] = 0.0f;
	axisAngle[1] = 0.0f;
	axisAngle[2] = 1.0f;
	axisAngle[3] = z;
	bmAxisAngleToQuaternion(qz, axisAngle);
	bmQuaternionMultiply(qxy, qx, qy);
	bmQuaternionMultiply(q, qxy, qz);
	instance->rot[0] = q[0];
	instance->rot[1] = q[1];
	instance->rot[2] = q[2];
	instance->rot[3] = q[3];
}

/* 0x001bdc30  (gap; placed with the drawing code): moves the platforms and the player models
to the players' positions, turned to face the centre (plus their spin) */
void SMRender_PlacePlayers(void)
{
	struct SMPlayer *player;
	struct BActorInstance *model;
	float angle;
	int i;

	bdSetWorldMatrix(DAT_002855f0);
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		player->pod.pos[0] = player->pos[0];
		angle = 6.28318548f - (player->spin + player->heading);
		player->pod.pos[1] = player->pos[1];
		player->pod.pos[2] = player->pos[2];
		player->pod.pos[3] = player->pos[3];
		SMRender_SetRotation(&player->pod, 0.0f, angle, 0.0f);
		bdSetWorldIdentity();
		baPrepActorInstance(&player->pod, 1);
		model = sm_PlayerModel(i);
		model->pos[0] = player->pos[0];
		model->pos[1] = player->pos[1];
		model->pos[2] = player->pos[2];
		model->pos[3] = player->pos[3];
		SMRender_SetRotation(model, 0.0f, angle, 0.0f);
		baPrepActorInstance(model, 1);
	}
}

/* 0x001bdda0  (gap; placed with the drawing code): the frame: fog, near/far planes, arena,
players, then the 2D pass (frame rate, the HUD while playing) and the fade-in */
void SMRender_Draw(void)
{
	bdSetRenderState(8, 1, 0);
	bdSetRenderState(7, 2, 0);
	bdSetFogColour(0x51, 0x34, 0x5a);
	bdSetFogRange(5000.0f, 15000.0f);
	bdSetNearFar(1.0f, 25000.0f);
	SMRender_DrawArena();
	SMRender_PlacePlayers();
	SMRender_DrawPlayers();
	bdSetWorldMatrix(DAT_002855f0);
	SMRender_Begin2D();
	SMSprites_DrawFramerate();
	if (sm_State == 2)
		SMSprites_Draw();
	if (sm_FadeIn == 1) {
		bdSetWorldMatrix(DAT_002855f0);
		if (CLFade_Draw((float)(SMTimer_FrameTime() * 100.0f)) == 1)
			sm_FadeIn = 0;
	}
}
