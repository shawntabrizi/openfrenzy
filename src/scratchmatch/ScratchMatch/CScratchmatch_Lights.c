/*
ScratchMatch/CScratchmatch_Lights.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CScratchmatch_Lights.cpp
(module scratchmatch)

The lights of Scratch Match: two static lights for the players (mapStaticLights, from the
definitions at 0x002b5e04), two spot lights that sweep round the stage in cycling colours
(maxSpotLights), and the light pod with its beams over the stage (mxRotationLights: the
models actors\lightpod.obe and actors\lightbeams.obe, both playing their "nice" animation).

A spot light is 0xe8 bytes from 0x00310560: the light, ..., position (+0xb8), target
(+0xc8), range (+0xd8), cone angle (+0xdc), colour bytes red, green, blue (+0xe4).
*/

#include "scratchmatch.h"

static const char s_LightsFile[] = "C:\\BlitzParty\\Game\\Src\\ScratchMatch\\CScratchmatch_Lights.cpp";

#define sm_Spot(i) (sm_SpotLights + 0xe8 * (i))
#define SMS_LIGHT(s) SM_AT(s, 0x00, struct BLight *)
#define SMS_POS(s) (&SM_AT(s, 0xb8, float))
#define SMS_TARGET(s) (&SM_AT(s, 0xc8, float))
#define SMS_RANGE(s) SM_AT(s, 0xd8, float)
#define SMS_ANGLE(s) SM_AT(s, 0xdc, float)
#define SMS_RGB(s) (&SM_AT(s, 0xe4, unsigned char))

/* a static light definition (0x18 bytes from 0x002b5e04): vector, type (0 directional,
1 point), range, colour bytes */
#define sm_StaticDef(i) (SM_A(0x002b5e04) + 0x18 * (i))

/* 0x001bde50  (gap; frees what SMLights_Init made) */
void SMLights_Free(void)
{
	int i;

	for (i = 0; i < 2; i++) {
		baEnableLight(sm_StaticLights[i], 0);
		baDeleteLight(sm_StaticLights[i]);
	}
	for (i = 0; i < 2; i++) {
		baEnableLight(SMS_LIGHT(sm_Spot(i)), 0);
		baDeleteLight(SMS_LIGHT(sm_Spot(i)));
	}
	FUN_001d5950();
	baDeleteActorInstance(sm_Pod);
	baDeleteActorInstance(sm_Beams);
	bkDeleteResource((struct bkResource *)sm_PodActor);
	sm_PodActor = NULL;
	bkDeleteResource((struct bkResource *)sm_BeamsActor);
	sm_BeamsActor = NULL;
}

/* 0x001bdf00  ScratchMatch/CScratchmatch_Lights.cpp: checks that the spot lights exist */
void SMLights_CheckSpotLights(void)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (SMS_LIGHT(sm_Spot(i)) == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x142,
				"POINTER ASSERTION FAILURE: maxSpotLights[ un32SpotLightIndex ].pLight");
	}
}

/* 0x001bdf40  ScratchMatch/CScratchmatch_Lights.cpp: switches the spot lights off */
void SMLights_DisableSpotLights(void)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (SMS_LIGHT(sm_Spot(i)) == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x14d,
				"POINTER ASSERTION FAILURE: maxSpotLights[ un32SpotLightIndex ].pLight");
		baEnableLight(SMS_LIGHT(sm_Spot(i)), 0);
	}
}

/* 0x001bdf90  ScratchMatch/CScratchmatch_Lights.cpp (bracketed): every 0.5 s rotates the 24-bit
colour word of the spot lights left by 2 bits */
void SMLights_CycleColours(void)
{
	unsigned int value;

	if (SMTimer_Elapsed(SMT_LIGHTS) > 0.5f) {
		value = sm_LightColours;
		sm_LightColours = (((value << 2) | (value >> 30)) & 0xffffff) | ((value & 0xffffff) >> 22);
		SMTimer_Reset(SMT_LIGHTS);
	}
}

/* 0x001bdfe0  ScratchMatch/CScratchmatch_Lights.cpp: switches the static lights on */
void SMLights_EnableStaticLights(void)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (sm_StaticLights[i] == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x16e,
				"POINTER ASSERTION FAILURE: mapStaticLights[ un32LightIndex ]");
		baEnableLight(sm_StaticLights[i], 1);
	}
}

/* 0x001be020  ScratchMatch/CScratchmatch_Lights.cpp: switches the static lights off */
void SMLights_DisableStaticLights(void)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (sm_StaticLights[i] == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x17a,
				"POINTER ASSERTION FAILURE: mapStaticLights[ un32LightIndex ]");
		baEnableLight(sm_StaticLights[i], 0);
	}
}

/* 0x001be060  ScratchMatch/CScratchmatch_Lights.cpp (bracketed): draws the light pod and its
beams (tweak "Draw Lightbeams"): pass 1 the first pass, else the last */
void SMLights_DrawPods(int pass)
{
	if (sm_DrawLightbeams == 1) {
		if (pass == 1) {
			baDrawActorInstance(sm_Pod, 1);
			baDrawActorInstance(sm_Beams, 1);
			return;
		}
		baSetBrightness(0x100);
		baDrawActorInstance(sm_Pod, 4);
		baDrawActorInstance(sm_Beams, 4);
	}
}

/* 0x001be0c0  ScratchMatch/CScratchmatch_Lights.cpp (bracketed): the spot lights circle the
stage (half a turn a second, half a turn apart) in the current colours */
void SMLights_UpdateSpotLights(void)
{
	float offset[3];
	unsigned int colour;
	char *spot;
	float height, targetY, targetZ;
	unsigned int i;

	for (i = 0; i < 2; i++) {
		spot = sm_Spot(i);
		targetZ = sm_SpotTargetZ;
		targetY = sm_SpotTargetY;
		height = sm_SpotHeight;
		SMS_POS(spot)[0] = 0.0f;
		SMS_POS(spot)[1] = height;
		SMS_POS(spot)[2] = 0.0f;
		SMS_TARGET(spot)[0] = 0.0f;
		SMS_TARGET(spot)[1] = targetY;
		SMS_TARGET(spot)[2] = 0.0f;
		offset[0] = 0.0f;
		offset[1] = 0.0f;
		offset[2] = targetZ;
		bmVectorRotateY(offset, offset,
			(float)((sm_ufloat(i) + SMTimer_Elapsed(SMT_BEAT)) * 0.5f * 6.28318548f));
		colour = sm_LightColours;
		SMS_TARGET(spot)[0] = offset[0] + SMS_TARGET(spot)[0];
		SMS_TARGET(spot)[1] = offset[1] + SMS_TARGET(spot)[1];
		SMS_TARGET(spot)[2] = offset[2] + SMS_TARGET(spot)[2];
		SMS_RGB(spot)[1] = (unsigned char)(colour >> 8);
		SMS_RGB(spot)[2] = (unsigned char)colour;
		SMS_RGB(spot)[0] = (unsigned char)(colour >> 16);
		SMS_ANGLE(spot) = 0.392699093f;
		SMS_RANGE(spot) = 2.5e-06f;
		baSetLightColour(SMS_LIGHT(spot), SMS_RGB(spot)[0], SMS_RGB(spot)[1], colour & 0xff);
		baSetSpotLight(SMS_LIGHT(spot), SMS_POS(spot), SMS_TARGET(spot), SMS_RANGE(spot), SMS_ANGLE(spot));
	}
}

/* 0x001be1f0  ScratchMatch/CScratchmatch_Lights.cpp (bracketed): places the pod and the beams */
void SMLights_PlacePods(void)
{
	sm_Pod->pos[0] = sm_PodPos[0];
	sm_Pod->pos[1] = sm_PodPos[1];
	sm_Pod->pos[2] = sm_PodPos[2];
	sm_Pod->pos[3] = sm_PodPos[3];
	SMRender_SetRotation(sm_Pod, 0.0f, 0.0f, 0.0f);
	baPrepActorInstance(sm_Pod, 1);
	sm_Beams->pos[0] = sm_PodPos[0];
	sm_Beams->pos[1] = sm_PodPos[1];
	sm_Beams->pos[2] = sm_PodPos[2];
	sm_Beams->pos[3] = sm_PodPos[3];
	SMRender_SetRotation(sm_Beams, 0.0f, 0.0f, 0.0f);
	baPrepActorInstance(sm_Beams, 1);
}

/* 0x001be290  ScratchMatch/CScratchmatch_Lights.cpp: loads the pod and the beams and starts
their animation, makes the static and the spot lights */
void SMLights_Init(void)
{
	struct BLight *light;
	const char *def;
	int i;

	sm_PodActor = (struct BActor *)bLoadResource(sm_VarPackage, (const char *)SM_P(0x002b5e34), 1, 0xdefa);
	if (sm_PodActor == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x4b, "POINTER ASSERTION FAILURE: mxRotationLights.pLightPod");
	sm_BeamsActor = (struct BActor *)bLoadResource(sm_VarPackage, (const char *)SM_P(0x002b5e38), 1, 0xdefa);
	if (sm_BeamsActor == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x4e, "POINTER ASSERTION FAILURE: mxRotationLights.pLightBeams");
	bkPrintf("Loaded light pod models\n");
	baInitActorInstance(sm_Pod, sm_PodActor, 4, s_LightsFile, 0x54);
	sm_PodAnim = baFindSubObject(sm_Pod, "nice");
	if (sm_PodAnim == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x56,
			"POINTER ASSERTION FAILURE: mxRotationLights.pLightPodAnimSegment");
	baFlushAnimations(sm_Pod, sm_Pod->root, 1, 0);
	baQueueAnimation(sm_Pod, sm_Pod->root, (struct BAnim *)sm_PodAnim, 0x400, -1, 1, NULL, NULL);
	baInitActorInstance(sm_Beams, sm_BeamsActor, 4, s_LightsFile, 0x5e);
	sm_BeamsAnim = baFindSubObject(sm_Beams, "nice");
	if (sm_BeamsAnim == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x60,
			"POINTER ASSERTION FAILURE: mxRotationLights.pLightBeamsAnimSegment");
	baFlushAnimations(sm_Beams, sm_Beams->root, 1, 0);
	baQueueAnimation(sm_Beams, sm_Beams->root, (struct BAnim *)sm_BeamsAnim, 0x400, -1, 1, NULL, NULL);
	FUN_001d5950();   /* an empty function; the original passes sm_Beams */
	sm_PodPos[0] = 0.0f;
	sm_PodPos[1] = 0.0f;
	sm_PodPos[2] = -200.0f;
	for (i = 0; i < 2; i++) {
		light = baCreateLight(NULL, 0);
		def = sm_StaticDef(i);
		sm_StaticLights[i] = light;
		if (SM_AT(def, 0xc, int) == 0)
			baSetDirectionalLight(light, (const float *)def);
		else if (SM_AT(def, 0xc, int) == 1)
			baSetPointLight(light, (const float *)def, SM_AT(def, 0x10, float));
		else
			SM_ASSERT_FAIL("CScratchmatch_Lights.cpp", 0x80, "ASSERTION FAILURE: FALSE");
		baSetLightColour(sm_StaticLights[i], SM_AT(def, 0x14, unsigned char), SM_AT(def, 0x15, unsigned char),
			SM_AT(def, 0x16, unsigned char));
	}
	for (i = 0; i < 2; i++)
		SMS_LIGHT(sm_Spot(i)) = baCreateLight(NULL, 0);
	sm_SpotHeight = 2000.0f;
	sm_SpotTargetY = -400.0f;
	sm_SpotTargetZ = 750.0f;
	SM_F(0x00310238) = -100.0f;
	SM_I(0x0031023c) = 0;
	sm_LightColours = 0x51;
	SMLights_PlacePods();
}

/* 0x001be570  (gap; after the lights file's tagged functions): the pod and beam animations,
the spot lights, the pod positions */
void SMLights_Update(void)
{
	baUpdateAnimations(sm_Pod, sm_Pod->root, (float)(SMTimer_FrameTime() * 50.0f), NULL);
	baUpdateAnimations(sm_Beams, sm_Beams->root, (float)(SMTimer_FrameTime() * 50.0f), NULL);
	SMLights_UpdateSpotLights();
	SMLights_PlacePods();
}
