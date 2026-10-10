/*
ScratchMatch/ScratchMatch_Effects.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\ScratchMatch_Effects.cpp
(module scratchmatch)

The effects of Scratch Match: two smoke planes (models\smoke01.obe, smoke02.obe, drawn with a
scrolling texture matrix, "maxSmokePlanes"), eight respawn effect slots (the model
models\respawn.obe, "maxSCEffectInfo"; no code in this game starts one, so the slots stay
free), the smoke sprite and the debris system.

A smoke plane is 0x38 bytes from 0x00310730: actor, instance, position (+0x08), scroll speed
(+0x18), texture offsets (+0x28, +0x2c). An effect slot is 0x1c bytes from 0x003107a0: type
(0xffff free), instance, ..., position (+0x0c).
*/

#include "scratchmatch.h"

static const char s_EffectsFile[] = "C:\\BlitzParty\\Game\\Src\\ScratchMatch\\ScratchMatch_Effects.cpp";

#define sm_Smoke(i) (sm_SmokePlanes + 0x38 * (i))
#define SMK_ACTOR(p) SM_AT(p, 0x00, struct BActor *)
#define SMK_INSTANCE(p) SM_AT(p, 0x04, struct BActorInstance *)
#define SMK_POS(p) (&SM_AT(p, 0x08, float))
#define SMK_SPEED(p) SM_AT(p, 0x18, float)
#define SMK_SCROLL(p) SM_AT(p, 0x28, float)
#define SMK_SCROLL2(p) SM_AT(p, 0x2c, float)

#define sm_Effect(i) (sm_Effects + 0x1c * (i))
#define SME_TYPE(e) SM_AT(e, 0x00, int)
#define SME_INSTANCE(e) SM_AT(e, 0x04, struct BActorInstance *)
#define SME_POS(e) (&SM_AT(e, 0x0c, float))

/* 0x001be5d0  (gap; before the effects file's tagged functions): draws the active respawn
effects, then calls classlib's debris glue 0x00080cc0 */
void SMEffects_Draw(void)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (SME_TYPE(sm_Effect(i)) != 0xffff) {
			baDrawActorInstance(SME_INSTANCE(sm_Effect(i)), 1);
			baDrawActorInstance(SME_INSTANCE(sm_Effect(i)), 4);
		}
	}
	CLDebris_Stop();
}

/* 0x001be610  ScratchMatch/ScratchMatch_Effects.cpp: loads the two smoke planes */
void SMEffects_LoadSmoke(void)
{
	char name[64];
	char *plane;
	float index;
	int i;

	for (i = 0; i < 2; i++) {
		plane = sm_Smoke(i);
		wsprintfA(name, "models\\smoke%02ld.obe", i + 1);
		SMK_ACTOR(plane) = (struct BActor *)bLoadResource(sm_Package, name, 1, 0xdefa);
		if (SMK_ACTOR(plane) == NULL)
			SM_ASSERT_FAIL("ScratchMatch_Effects.cpp", 0xf1,
				"POINTER ASSERTION FAILURE: maxSmokePlanes[ un32SmokePlaneNum ].pActor");
		SMK_INSTANCE(plane) = baInitActorInstance(NULL, SMK_ACTOR(plane), 2, s_EffectsFile, 0xf3);
		if (SMK_INSTANCE(plane) == NULL)
			SM_ASSERT_FAIL("ScratchMatch_Effects.cpp", 0xf4,
				"POINTER ASSERTION FAILURE: maxSmokePlanes[ un32SmokePlaneNum ].pActorInstance");
		index = sm_ufloat((unsigned int)i);
		SMK_POS(plane)[0] = 0.0f;
		SMK_POS(plane)[1] = 0.0f;
		SM_AT(plane, 0x1c, float) = 0.0f;
		SMK_SCROLL(plane) = 0.0f;
		SMK_SCROLL2(plane) = 0.0f;
		SMK_POS(plane)[2] = -50.0f * index;
		SMK_SPEED(plane) = (index + 1.0f) * 0.04f;
	}
}

/* 0x001be720  ScratchMatch/ScratchMatch_Effects.cpp: frees the smoke planes */
void SMEffects_FreeSmoke(void)
{
	char *plane;
	int i;

	for (i = 0; i < 2; i++) {
		plane = sm_Smoke(i);
		if (SMK_INSTANCE(plane) == NULL)
			SM_ASSERT_FAIL("ScratchMatch_Effects.cpp", 0x109,
				"POINTER ASSERTION FAILURE: maxSmokePlanes[ un32SmokePlaneNum ].pActorInstance");
		baDeleteActorInstance(SMK_INSTANCE(plane));
		SMK_INSTANCE(plane) = NULL;
		if (SMK_ACTOR(plane) == NULL)
			SM_ASSERT_FAIL("ScratchMatch_Effects.cpp", 0x10d,
				"POINTER ASSERTION FAILURE: maxSmokePlanes[ un32SmokePlaneNum ].pActor");
		bkDeleteResource((struct bkResource *)SMK_ACTOR(plane));
		SMK_ACTOR(plane) = NULL;
	}
}

/* 0x001be7a0  ScratchMatch/ScratchMatch_Effects.cpp: the respawn model, the free effect
slots, the smoke planes, the smoke sprite and the debris system */
void SMEffects_Init(void)
{
	char name[64];
	int i;

	wsprintfA(name, "models\\%s.obe", SM_A(0x002b5f44));
	sm_RespawnActor = bLoadResource(sm_Package, name, 1, 0xdefa);
	if (sm_RespawnActor == NULL)
		SM_ASSERT_FAIL("ScratchMatch_Effects.cpp", 0x57,
			"POINTER ASSERTION FAILURE: maxSCEffectInfo[ un32ModelIndex ].pActor");
	for (i = 0; i < 8; i++)
		SME_TYPE(sm_Effect(i)) = 0xffff;
	SMEffects_LoadSmoke();
	Sprite_LoadDefs(sm_SmokeSprite, 1, 0);
	Debris_Init(NULL, 1000);
}

/* 0x001be840  ScratchMatch/ScratchMatch_Effects.cpp: frees the effects (and calls classlib's
debris glue 0x00080bb0) */
void SMEffects_Free(void)
{
	Sprite_FreeDefs(sm_SmokeSprite, 1);
	CLDebris_Start();
	SMEffects_FreeSmoke();
	if (sm_RespawnActor == NULL)
		SM_ASSERT_FAIL("ScratchMatch_Effects.cpp", 0x71,
			"ASSERTION FAILURE: maxSCEffectInfo[ un32ModelIndex ].pActor != NULL");
	bkDeleteResource((struct bkResource *)sm_RespawnActor);
	sm_RespawnActor = NULL;
}

/* 0x001be890  ScratchMatch/ScratchMatch_Effects.cpp: scrolls and places the smoke planes. The
original steps the scroll value by 0x3c bytes but the planes by 0x38, so the second plane
scrolls its +0x2c offset with its +0x1c speed (0): kept as it was. */
void SMEffects_UpdateSmoke(void)
{
	struct BActorInstance *instance;
	char *plane;
	float *scroll;
	int i;

	for (i = 0; i < 2; i++) {
		plane = sm_Smoke(i);
		scroll = SM_FV(0x00310758 + 0x3c * i);
		if (SMK_INSTANCE(plane) == NULL)
			SM_ASSERT_FAIL("ScratchMatch_Effects.cpp", 0x11a,
				"POINTER ASSERTION FAILURE: maxSmokePlanes[ un32SmokePlaneNum ].pActorInstance");
		*scroll = (float)(SMTimer_FrameTime() * scroll[-4] + *scroll);
		instance = SMK_INSTANCE(plane);
		instance->pos[0] = SMK_POS(plane)[0];
		instance->pos[1] = SMK_POS(plane)[1];
		instance->pos[2] = SMK_POS(plane)[2];
		instance->pos[3] = SMK_POS(plane)[3];
		baPrepActorInstance(SMK_INSTANCE(plane), 1);
	}
}

/* 0x001be910  ScratchMatch/ScratchMatch_Effects.cpp: draws the smoke planes with their texture
offsets in the texture matrix of stage 0 */
void SMEffects_DrawSmoke(void)
{
	float matrix[16];
	char *plane;
	float scroll;
	int i;

	bdSetRenderState(2, 3, 0);
	D3DDevice_SetTextureState_Deferred(0, 9 /* D3DTSS_TEXTURETRANSFORMFLAGS */, 2);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	for (i = 0; i < 2; i++) {
		plane = sm_Smoke(i);
		if (SMK_INSTANCE(plane) == NULL)
			SM_ASSERT_FAIL("ScratchMatch_Effects.cpp", 0x131,
				"POINTER ASSERTION FAILURE: maxSmokePlanes[ un32SmokePlaneNum ].pActorInstance");
		bdSetWorldMatrix(DAT_002855f0);
		scroll = SMK_SCROLL(plane);
		memcpy(matrix, DAT_002855f0, sizeof matrix);
		matrix[8] = SMK_SCROLL2(plane);
		matrix[9] = scroll;
		D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)2 /* D3DTS_TEXTURE0 */, (const D3DMATRIX *)matrix);
		baDrawActorInstance(SMK_INSTANCE(plane), 4);
	}
	D3DDevice_SetTextureState_Deferred(0, 9 /* D3DTSS_TEXTURETRANSFORMFLAGS */, 0);
}

/* 0x001be9e0  (gap; a texture stage helper shared with other minigames' drawing): stage 1
modulates by the texture factor `level` (alpha) */
void SMEffects_SetDarkenStage(int level)
{
	D3DDevice_SetRenderState_TextureFactor((DWORD)level << 24);
	D3DDevice_SetTextureState_Deferred(1, 0 /* COLOROP */, 0xe);
	D3DDevice_SetTextureState_Deferred(1, 2 /* COLORARG1 */, 2);
	D3DDevice_SetTextureState_Deferred(1, 3 /* COLORARG2 */, 1);
	D3DDevice_SetTextureState_Deferred(1, 10 /* ADDRESSU */, 1);
	D3DDevice_SetTextureState_Deferred(1, 11 /* ADDRESSV */, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x10000);
	D3DDevice_SetTextureState_Deferred(1, 9 /* TEXTURETRANSFORMFLAGS */, 2);
}

/* 0x001bea60  (gap; called by several minigames): stage 1 back to its default */
void SMEffects_ResetDarkenStage(void)
{
	bdSetTexture(1, NULL);
	D3DDevice_SetTextureState_Deferred(1, 0 /* COLOROP */, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
}

/* 0x001bea90  (gap; the effects' frame): respawn effects, smoke, debris */
void SMEffects_Update(void)
{
	struct BActorInstance *instance;
	char *effect;
	int i;

	for (i = 0; i < 8; i++) {
		effect = sm_Effect(i);
		if (SME_TYPE(effect) != 0xffff) {
			bdSetWorldMatrix(DAT_002855f0);
			instance = SME_INSTANCE(effect);
			instance->pos[0] = SME_POS(effect)[0];
			instance->pos[1] = SME_POS(effect)[1];
			instance->pos[2] = SME_POS(effect)[2];
			instance->pos[3] = SME_POS(effect)[3];
			instance = SME_INSTANCE(effect);
			baUpdateAnimations(instance, instance->root, (float)(SMTimer_FrameTime() * 20000.0f), NULL);
			if (SME_INSTANCE(effect) != NULL)
				baPrepActorInstance(SME_INSTANCE(effect), 1);
		}
	}
	SMEffects_UpdateSmoke();
	if (SMTimer_Elapsed(SMT_EFFECTS) > 0.1f)
		SMTimer_Reset(SMT_EFFECTS);
	Debris_UpdateSystems((float)SMTimer_FrameTime());
}
