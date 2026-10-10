/*
FoxHunt/CGameMode_FoxHunt.c  <-  C:\BlitzParty\Game\Src\FoxHunt\CGameMode_FoxHunt.cpp
(module foxhunt)

The Tag game mode (CGameMode_FoxHunt, 0x280 bytes, vtable 0x0026cdec): loads the level
and the variation package, makes the players, the throwables and the goals, runs the rules
of the four variations each frame and draws the scene.

The mode object (fields after the CGameMode base, 0x28 bytes; see GM_* below):
  +0x28  the 4 camera targets (actor instances)    +0x38  4 player positions (vec4)
  +0x9c  frame counter                             +0xa0  round timer (frames)
  +0xa4  state: 0 intro, 1 countdown, 2 playing, 4/5 results, 6 exit
  +0xb0  best score, +0xc0 the leader              +0xc4  the 4 scores shown on the HUD
  +0x180 variation 1-4                             +0x1e4 the near/far planes it replaced
  +0x20c the debug font                            +0x218, +0x21c two lights
  +0x27c the goal markers (CFHunt_GenericGoal)
*/

#include "foxhunt.h"

const char fh_GameModeFile[] = "C:\\BlitzParty\\Game\\Src\\FoxHunt\\CGameMode_FoxHunt.cpp";

/* fields of the mode object */
#define GM_STATE(m) FH_AI(m, 0xa4)
#define GM_VARIATION(m) FH_AI(m, 0x180)
#define GM_LIGHT(m) FH_AP(m, 0x218)
#define GM_LIGHT2(m) FH_AP(m, 0x21c)
#define GM_AMBIENT(m) FH_AI(m, 0x1bc)
#define GM_AMBIENT_LIT(m) FH_AI(m, 0x1b8)

/* fields of a CharLib actor (the player's model) */
#define ACTOR_FLAGS(a) FH_AU(a, 0x220)
#define ACTOR_ALPHA(a) FH_AI(a, 0x224)
#define ACTOR_LIGHTING(a) FH_AI(a, 0x21c)

/* 0x0026cdec  CGameMode_FoxHunt vtable */
const void *const vtable_0026cdec[13] = {
	(const void *)FUN_00125d60, /* [0] +0x00 scalar deleting destructor */
	(const void *)FUN_001d5950, /* [1] +0x04 suspend (empty) */
	(const void *)FUN_001d5950, /* [2] +0x08 resume (empty) */
	(const void *)FUN_00129ab0, /* [3] +0x0c exit */
	(const void *)FUN_00125590, /* [4] +0x10 enter */
	(const void *)FUN_000d9730, /* [5] +0x14 update (polls the loading) */
	(const void *)FUN_00129e50, /* [6] +0x18 process */
	(const void *)FUN_0012a190, /* [7] +0x1c draw */
	(const void *)FUN_001ec260, /* [8] +0x20 post draw (empty) */
	(const void *)FUN_00012930, /* [9] +0x24 poll the loading */
	(const void *)FUN_000128e0, /* [10] +0x28 loading finished */
	(const void *)FUN_00125d80, /* [11] +0x2c init (the packages are loaded) */
	(const void *)FUN_0012e020, /* [12] +0x30 start */
};

/* 0x00125560  (gap, before CGameMode_FoxHunt.cpp): CGameMode_FoxHunt::CGameMode_FoxHunt */
void *CGameMode_FoxHunt_Ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, "Tag");
	*(const void *const **)self = vtable_0026cdec;
	return self;
}

/* 0x00125580  (gap): CGameMode_FoxHunt::~CGameMode_FoxHunt */
void CGameMode_FoxHunt_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026cdec;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x00125590  CGameMode_FoxHunt.cpp: enter (the mode was pushed): starts loading the level
package and the variation package (Tag1 for variations 1-2, Tag3 for 3-4) and sets the
near/far planes to 4000/10000, saving the old ones */
void CGameMode_FoxHunt_Enter(void *self)
{
	int variation;

	FH_AI(self, 0x14) = 1;
	Core_LoadCharacterVoices(1);
	FH_I(0x00302138) = 0;
	fh_LevelPackage = fh_AddPackage("Packages\\FoxLevel");
	if (fh_LevelPackage == NULL)
		return;
	variation = GM_VARIATION(self);
	if (variation == 1 || variation == 2) {
		fh_VariationPackage = fh_AddPackage("Packages\\Tag1");
		if (fh_VariationPackage == NULL)
			return;
	} else if (variation == 3 || variation == 4) {
		fh_VariationPackage = fh_AddPackage("Packages\\Tag3");
		if (fh_VariationPackage == NULL)
			return;
	}
	FH_AF(self, 0x1e4) = DAT_0028ffa8;
	FH_AF(self, 0x1e8) = DAT_0028ffac;
	DAT_0028ffa8 = 4000.0f;
	DAT_0028ffac = 10000.0f;
}

/* 0x00125640  CGameMode_FoxHunt.cpp: draws two frame-rate lines (the global and the mode's
+0x208) with the debug font */
void CGameMode_FoxHunt_DrawFrameRate(void *self)
{
	char text[256];
	unsigned short wide[256];

	bdSetProjectionMode(0);
	bdSetRenderState(3, 0, 0);
	bdSetWorldIdentity();
	sprintf(text, "Frame/Sec %f", (double)DAT_002ba990);
	bkStringToWide(wide, text);
	bdPrintFontLite((struct bdFont *)FH_AP(self, 0x20c), wide, (int)strlen(text), -300.0f, -20.0f,
		0x7f, 0x7f, 0x7f, 100, NULL, NULL);
	sprintf(text, "Frame/Sec %f", (double)FH_AF(self, 0x208));
	bkStringToWide(wide, text);
	bdPrintFontLite((struct bdFont *)FH_AP(self, 0x20c), wide, (int)strlen(text), -300.0f, -60.0f,
		0x7f, 0x5a, 0x5a, 100, NULL, NULL);
	bdSetProjectionMode(1);
}

/* 0x00125750  CGameMode_FoxHunt.cpp: copies the first "quoted" string of `text` into `out`
(at most `max` characters). Returns 0 (and an empty string) when `text` is NULL. */
int FoxHunt_ReadQuoted(const char *text, char *out, int max)
{
	int i;

	for (;;) {
		if (text == NULL) {
			*out = 0;
			return 0;
		}
		if (*text == '"')
			break;
		text++;
	}
	for (i = 0; (text = text + 1, text != NULL && *text != '"') && i < max; i++)
		out[i] = *text;
	out[i] = 0;
	return 1;
}

/* 0x001257a0  CGameMode_FoxHunt.cpp: draws one actor's second pass (pass 6), lit by the
mode's light when the actor asks for it (lighting 1 or 5). Faded actors (flags 0x600) that
are not opaque are skipped. */
void CGameMode_FoxHunt_DrawActorPass6(void *self, char *actor)
{
	if ((ACTOR_FLAGS(actor) & 0x600) != 0 && ACTOR_ALPHA(actor) < 0x100)
		return;
	if (ACTOR_LIGHTING(actor) == 5) {
		bdPushRenderState(3, 1, 0);
		baSetAmbientLight(GM_AMBIENT(self), GM_AMBIENT(self), GM_AMBIENT(self));
		baEnableLight((struct BLight *)GM_LIGHT(self), 1);
	} else if (ACTOR_LIGHTING(actor) == 1) {
		bdPushRenderState(3, 1, 0);
		baSetAmbientLight(GM_AMBIENT_LIT(self), GM_AMBIENT_LIT(self), GM_AMBIENT_LIT(self));
		baEnableLight((struct BLight *)GM_LIGHT(self), 1);
	} else {
		baEnableLight((struct BLight *)GM_LIGHT(self), 0);
		baSetAmbientLight(GM_AMBIENT(self), GM_AMBIENT(self), GM_AMBIENT(self));
	}
	baSetBrightness(0x100);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, (ACTOR_FLAGS(actor) & 0x400) != 0, 0);
	bdSetRenderState(5, (ACTOR_FLAGS(actor) & 0x800) == 0, 0);
	if ((ACTOR_FLAGS(actor) & 0x80000) != 0) {
		bdPushRenderState(3, 0, 0);
		baDrawActorInstance((struct BActorInstance *)(actor + 0x10), 6);
		bdPopRenderState(3);
	} else {
		if ((ACTOR_FLAGS(actor) & 0x200000) != 0)
			Lights_EnableAll(0, 1);
		baDrawActorInstance((struct BActorInstance *)(actor + 0x10), 6);
		if ((ACTOR_FLAGS(actor) & 0x200000) != 0)
			Lights_EnableAll(1, 1);
	}
	if (ACTOR_LIGHTING(actor) == 5) {
		bdPopRenderState(3);
		baEnableLight((struct BLight *)GM_LIGHT(self), 0);
	} else if (ACTOR_LIGHTING(actor) == 1) {
		bdPopRenderState(3);
		baEnableLight((struct BLight *)GM_LIGHT(self), 0);
	}
	baSetAmbientLight(GM_AMBIENT(self), GM_AMBIENT(self), GM_AMBIENT(self));
}

/* 0x00125950  CGameMode_FoxHunt.cpp: draws one actor's first pass (pass 1) with its fog,
animated textures and lighting; actors with flag 0x10000 get a scrolled texture matrix.
Actors with lighting 0 are skipped in the mode's state +0x1d8 == 2. */
void CGameMode_FoxHunt_DrawActor(void *self, char *actor)
{
	float m[16];
	int i;

	if ((ACTOR_FLAGS(actor) & 0x600) != 0 && ACTOR_ALPHA(actor) <= 0xff)
		return;
	if (ACTOR_LIGHTING(actor) != 0 || FH_AI(self, 0x1d8) != 2) {
		if (FH_AI(actor, 0x340) != 0) {
			bdSetRenderState(8, FH_AI(self, 0x1e0) == 1, 0);
			bdSetFogRange(FH_AF(actor, 0x344), FH_AF(actor, 0x348));
			bdSetFogColour(FH_AU(actor, 0x34c), FH_AU(actor, 0x350), FH_AU(actor, 0x354));
		}
		bdSetRenderState(8, FH_AI(self, 0x1e0) == 1, 0);
		AnimTex_ApplyList((struct CLFList *)(actor + 0x328));
		bdSetRenderState(7, 3, 0);
		baSetAmbientLight(GM_AMBIENT(self), GM_AMBIENT(self), GM_AMBIENT(self));
		if (ACTOR_LIGHTING(actor) == 1) {
			bdPushRenderState(3, 1, 0);
			baSetAmbientLight(GM_AMBIENT_LIT(self), GM_AMBIENT_LIT(self), GM_AMBIENT_LIT(self));
			baEnableLight((struct BLight *)GM_LIGHT(self), 1);
		} else if (ACTOR_LIGHTING(actor) == 5) {
			baSetAmbientLight(GM_AMBIENT(self), GM_AMBIENT(self), GM_AMBIENT(self));
			baEnableLight((struct BLight *)GM_LIGHT(self), 1);
		}
		if ((ACTOR_FLAGS(actor) & 0x10000) != 0) {
			baSetBrightness(0x100);
			Actor_SetConstantAlphaStage(0x50);
			for (i = 0; i < 16; i++)
				m[i] = 0.0f;
			m[0] = 1.0f;
			m[5] = 1.0f;
			m[10] = 1.0f;
			m[15] = 1.0f;
			m[8] = 0.1f;
			m[9] = 0.1f;
			D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)3, (const D3DMATRIX *)m);
		}
		bdSetRenderState(5, 1, 0);
		if ((ACTOR_FLAGS(actor) & 0x600) == 0) {
			bdSetRenderState(6, 0, 0);
		} else {
			bdSetRenderState(6, 1, 0);
			bdSetRenderState(10, (ACTOR_FLAGS(actor) & 0x400) != 0 ? 1 : 0, 0);
		}
		if ((ACTOR_FLAGS(actor) & 0x80000) != 0) {
			bdPushRenderState(3, 0, 0);
			baDrawActorInstance((struct BActorInstance *)(actor + 0x10), 1);
			bdPopRenderState(3);
		} else {
			if ((ACTOR_FLAGS(actor) & 0x200000) != 0)
				Lights_EnableAll(0, 1);
			baDrawActorInstance((struct BActorInstance *)(actor + 0x10), 1);
			if ((ACTOR_FLAGS(actor) & 0x200000) != 0)
				Lights_EnableAll(1, 1);
		}
		bdSetRenderState(1, 2, 0);
		bdSetRenderState(6, 1, 0);
		if (ACTOR_LIGHTING(actor) == 1) {
			bdPopRenderState(3);
			baEnableLight((struct BLight *)GM_LIGHT(self), 0);
		} else if (ACTOR_LIGHTING(actor) == 5) {
			baEnableLight((struct BLight *)GM_LIGHT(self), 0);
		}
	}
	baSetAmbientLight(GM_AMBIENT(self), GM_AMBIENT(self), GM_AMBIENT(self));
}

/* 0x00125c20  CGameMode_FoxHunt.cpp: puts the camera 150 units above player 1, turned to
its heading */
void FoxHunt_CameraAtPlayer1(void)
{
	char *actor = FH_AP(fh_Players[0], 0x30);
	float y = FH_AF(actor, 0xd4) + 150.0f;

	bdSetCameraPosition(FH_AF(actor, 0xd0), y, FH_AF(actor, 0xd8));
	bdSetCameraAngles(0.0f, FH_AF(FH_AP(fh_Players[0], 0x30), 0x1e8), 0.0f, 1);
	FH_AI(fh_Players[0], 0x1118) = 0;
}

/* 0x00125c90  CGameMode_FoxHunt.cpp: when the character colour set changed, recolours the
players' models */
void FoxHunt_UpdateColourSet(void)
{
	int i;

	if (DAT_00283ea4 == FH_I(0x003021a0))
		return;
	FH_I(0x003021a0) = DAT_00283ea4;
	for (i = 0; i < DAT_002bae94; i++)
		Core_SetCharacterColours(FH_AP(DAT_002f289c[i], 0x30) + 0x10, DAT_00283ea4);
}

/* 0x00125cf0  CGameMode_FoxHunt.cpp: clears word `index` of 0x00302188 and selects the
route height table for the current pair of switches 0x0030218c/0x00302190 (0/0: 0x00302154,
0/1: 0x00302188, 1/0: 0x00302158) */
void FoxHunt_SelectRouteHeights(int index)
{
	FH_IV(0x00302188)[index] = 0;
	if (FH_I(0x0030218c) == 0) {
		if (FH_I(0x00302190) == 0) {
			CLRoute_SetHeights((struct CLRouteHeights *)FH_P(0x00302154));
			return;
		}
		if (FH_I(0x00302190) == 1) {
			CLRoute_SetHeights((struct CLRouteHeights *)FH_P(0x00302188));
			return;
		}
	} else if (FH_I(0x0030218c) == 1 && FH_I(0x00302190) == 0) {
		CLRoute_SetHeights((struct CLRouteHeights *)FH_P(0x00302158));
	}
}

/* 0x00125d60  CGameMode_FoxHunt.cpp: scalar deleting destructor */
void *CGameMode_FoxHunt_Destroy(void *self, unsigned char flags)
{
	CGameMode_FoxHunt_Dtor(self);
	if ((flags & 1) != 0)
		FH_DELETE(self);
	return self;
}

/* 0x00125d80  CGameMode_FoxHunt.cpp: init (the packages are loaded): loads their resources,
makes the speech object and the lights, and sets the start values of the round. Variations
1-2 (the town) and 3-4 (the beach) have their own camera, light and goal set-up. */
void CGameMode_FoxHunt_Init(void *self)
{
	char *speech;
	char *goals;
	struct BLight *light;
	float *dir;

	bLoadPackageResources((struct bkPackage *)fh_LevelPackage, 0xffffffffu, 0xdefa, 0);
	FoxHunt_Effects_Init(DAT_003022cc, 0, GM_VARIATION(self));
	bLoadPackageResources((struct bkPackage *)fh_VariationPackage, 0xffffffffu, 0xdefa, 0);
	CProjector_Reset();
	Pickups_LoadModels();
	CRayControl_ResetList();
	speech = (char *)FH_NEW(0xa8, fh_GameModeFile, 0xb8);
	if (speech == NULL)
		fh_Speech = NULL;
	else
		fh_Speech = (char *)FHunt_Sound_Ctor(speech);
	FHunt_Sound_Initialise(fh_Speech, NULL);
	GM_LIGHT2(self) = (char *)baCreateLight(NULL, 0);
	FH_AI(self, 0xa8) = 0;
	FH_AF(self, 0x170) = 3.14f;
	FH_AF(self, 0x174) = 1500.0f;
	FH_AF(self, 0x220) = 15.0f;
	FH_AI(self, 0x188) = 0;
	FH_AI(self, 0x18c) = 0;
	FH_AF(self, 0x190) = 0.25f;
	FH_AI(self, 0x194) = 3;
	if (GM_VARIATION(self) < 3) {
		FH_AF(self, 0x234) = -925.0f;
		FH_AF(self, 0x238) = 0.5f;
		FH_AF(self, 0x23c) = -1100.0f;
		FH_AF(self, 0x244) = 910.0f;
		FH_AF(self, 0x248) = 0.0f;
		FH_AF(self, 0x24c) = 1100.0f;
		FH_AU(self, 0x10c) = 0x44a9f3a6u;   /* 1359.61 */
		FH_AU(self, 0x110) = 0x44a679f4u;   /* 1331.81 */
		FH_AU(self, 0x114) = 0xc4b32b44u;   /* -1433.35 */
		FH_AF(self, 0x118) = 1.0f;
		goals = (char *)FH_NEW(0x24, fh_GameModeFile, 0xd9);
		if (goals == NULL)
			goals = NULL;
		else
			goals = (char *)CFHunt_GenericGoal_Ctor(goals);
		FH_AP(self, 0x27c) = goals;
		light = baCreateLight(NULL, 0);
		dir = FH_AV(self, 0x224);
		GM_LIGHT(self) = (char *)light;
		dir[0] = 0.03f;
		FH_AF(self, 0x228) = 0.72f;
		FH_AF(self, 0x22c) = -0.7f;
		baSetDirectionalLight(light, dir);
		baSetLightColour((struct BLight *)GM_LIGHT(self), 0xb4, 0xb4, 0xa0);
		baEnableLight((struct BLight *)GM_LIGHT(self), 0);
		GM_AMBIENT_LIT(self) = 0x8c;
		GM_AMBIENT(self) = 0x7f;
		dir[0] = 600.0f;
		FH_AF(self, 0x228) = 0.0f;
		FH_AF(self, 0x22c) = -600.0f;
		FH_AF(self, 0x8c) = 3395.0f;
		FH_AF(self, 0x90) = -220.0f;
		FH_AF(self, 0x94) = -2220.0f;
	}
	if (GM_VARIATION(self) > 2) {
		FH_AF(self, 0x234) = 1295.0f;
		FH_AF(self, 0x238) = 50.0f;
		FH_AF(self, 0x23c) = -800.0f;
		FH_AF(self, 0x244) = -1300.0f;
		FH_AF(self, 0x248) = 45.0f;
		FH_AF(self, 0x24c) = -795.0f;
		FH_AF(self, 0x10c) = 0.0f;
		FH_AU(self, 0x110) = 0x449da47bu;   /* 1261.14 */
		FH_AU(self, 0x114) = 0xc4f16410u;   /* -1931.13 */
		light = baCreateLight(NULL, 0);
		dir = FH_AV(self, 0x224);
		GM_LIGHT(self) = (char *)light;
		dir[0] = 0.03f;
		FH_AF(self, 0x228) = 0.72f;
		FH_AF(self, 0x22c) = -0.7f;
		baSetDirectionalLight(light, dir);
		baSetLightColour((struct BLight *)GM_LIGHT(self), 0x96, 0x96, 0x96);
		dir[0] = 100.0f;
		FH_AF(self, 0x228) = 500.0f;
		FH_AF(self, 0x22c) = 1300.0f;
		FH_AI(self, 0x1c4) = 0;
		FH_AI(self, 0x1c8) = 1;
		GM_AMBIENT_LIT(self) = 0x8c;
		GM_AMBIENT(self) = 0x7f;
		FH_AI(self, 0x214) = 0;
		FH_AI(self, 0x16c) = 0;
	}
	FH_AI(self, 0x1a4) = 1;
	FH_AI(self, 0x1a8) = 1;
	FH_AI(self, 0x1ac) = 0;
	FH_AI(self, 0x1b4) = 0;
	FH_AI(self, 0x1c0) = 0;
	FH_AF(self, 0x178) = 960.0f;
	FH_AI(self, 0x17c) = 0;
	FH_AI(self, 0xd8) = -2;
	FH_AI(self, 0xdc) = 0;
	FH_AI(self, 0xf4) = 0;
	FH_AI(self, 0xf8) = 0xff;
	FH_AI(self, 0x164) = 0;
	FH_AI(self, 0x1cc) = 0;
	FH_AI(self, 0x1d0) = 1;
	FH_AI(self, 0x1d4) = 1;
	FH_AI(self, 0x1d8) = 1;
	FH_AI(self, 0x1dc) = 1;
	FH_AI(self, 0x1f0) = 0;
	FH_AI(self, 0x1f4) = 0;
	FH_AI(self, 0x1f8) = 0;
	FH_AI(self, 0x1fc) = 0;
	FH_AI(self, 0x200) = 0;
	FH_AI(self, 0x204) = 0;
	FH_AI(self, 0x88) = 0;
	FH_AI(self, 0x1e0) = 1;
	FH_AU(self, 0x1b0) = DAT_002ba56c != 0 ? 4u : 0u;
	FH_I(0x00302160) = 0;
	FH_AI(self, 0x274) = 0;
	FH_AI(self, 0x278) = 0;
	FH_AI(self, 0x1a0) = 0;
	FH_I(0x00302150) = 0;
	FH_F(0x00302144) = 1.2f;
	FH_I(0x0030218c) = 1;
	FH_I(0x00302190) = 1;
	bdSetWorldIdentity();
	bdSetProjectionMode(1);
	bdSetNearFar(40.0f, 11000.0f);
	FH_AP(self, 0x20c) = (char *)bLoadResource(NULL, "handgotb.ttf", 3, 0);
	FH_AI(self, 0x28) = 0;
	FH_AI(self, 0x2c) = 0;
	FH_AI(self, 0x30) = 0;
	FH_AI(self, 0x34) = 0;
}

/* ======================================================================== the rules */

#define PL(i) fh_Players[i]
#define PL_ACTOR(p) FH_AP(p, 0x30)
#define WATER() FH_P(0x0030214c)                 /* the sea / harbour (CWaterField) */
#define GOAL_A() FH_P(0x00302194)                /* the two goals of variations 1-2 */
#define GOAL_B() FH_P(0x0030219c)

/* a random number in [0, 1] (rand() / 32767 with the original float 0x38000100) */
static inline float fh_randf(void)
{
	return (float)fh_rand() * 3.05185094e-05f;
}

/* the colour bytes of the character of player record `rec` */
#define CHAR_R(rec) DAT_00290718[DAT_002baed4[(rec) * 10] * 0x60 + 0]
#define CHAR_G(rec) DAT_00290718[DAT_002baed4[(rec) * 10] * 0x60 + 1]
#define CHAR_B(rec) DAT_00290718[DAT_002baed4[(rec) * 10] * 0x60 + 2]

static void water_splash(float x, float z, float depth, float inner, float outer)
{
	FH_VFN(WATER(), 0x28, int (*)(void *, float, float, float, float, float))(WATER(), x, z, depth, inner, outer);
}

/* drops what the player holds (variations 3-4: a throwable) back to the ground state */
static void drop_held_throwable(char *player)
{
	FH_AI(player, 0x11bc) = 0;
	FH_AI(fh_Throwables[FH_AI(player, 0x11c0)], 0x8) = 0;
	FH_AI(fh_Throwables[FH_AI(player, 0x11c0)], 0xc) = 4;
	FH_AI(FH_AP(fh_Throwables[FH_AI(player, 0x11c0)], 0x14), 0x220) = 1;
	FH_AI(fh_Throwables[FH_AI(player, 0x11c0)], 0x48) = -1;
	FH_AI(player, 0x11c0) = -1;
}

/* variation 3/4: the 40 coloured sparks of an eliminated player, at its position raised by
100 (variation 4 adds 40 white ones) */
static void elimination_sparks(int i, int rec, int victim)
{
	unsigned char r = CHAR_R(rec), g = CHAR_G(rec), b = CHAR_B(rec);
	char *actor = PL_ACTOR(PL(i));
	float pos[4];
	float vel[4];
	int n;

	pos[0] = FH_AF(actor, 0xd0);
	pos[2] = FH_AF(actor, 0xd8);
	pos[3] = FH_AF(actor, 0xdc);
	pos[1] = FH_AF(actor, 0xd4) + 100.0f;
	Light_Flash(pos, r, g, b);
	for (n = 0x28; n != 0; n--) {
		vel[0] = (float)fh_rand() * 0.00152592547f - 25.0f;
		vel[1] = (float)fh_rand() * 0.00183111057f;
		vel[2] = (float)fh_rand() * 0.00152592547f - 25.0f;
		FoxHunt_AddDebris(0x10, pos, vel, 0, ((unsigned int)r << 16) | ((unsigned int)g << 8) | b);
		if (victim) {
			vel[0] = (float)fh_rand() * 0.00183111057f;
			vel[1] = (float)fh_rand() * 0.00183111057f;
			vel[2] = (float)fh_rand() * 0.00183111057f;
			FoxHunt_AddDebris(0x2a, pos, vel, 0, 0xffffffffu);
		}
	}
}

static int count_active(void)
{
	int n = 0;

	if (FH_AI(PL(0), 0x11ac) == 1)
		n++;
	if (FH_AI(PL(1), 0x11ac) == 1)
		n++;
	if (FH_AI(PL(2), 0x11ac) == 1)
		n++;
	if (FH_AI(PL(3), 0x11ac) == 1)
		n++;
	return n;
}

/* variation 1-2: rain drops over the town (white while playing, grey in the intro) */
static void rain(void *self, int count)
{
	float pos[4];
	float vel[4];
	float x, z;
	int i;

	vel[3] = 1.0f;
	vel[0] = 0.0f;
	vel[1] = -80.0f;
	vel[2] = 0.0f;
	for (i = 0; i < count; i++) {
		x = fh_randf();
		z = fh_randf();
		pos[3] = 1.0f;
		pos[1] = 3000.0f;
		if (GM_STATE(self) == 0) {
			pos[0] = x * 14000.0f - 7000.0f;
			pos[2] = z * 14000.0f - 7000.0f;
			FUN_00134030((undefined4 *)DAT_003022cc, pos, vel, -302.0f, 0x5a, 0x5a, 0x5a, 200, 200.0f);
		} else {
			pos[0] = x * 6000.0f - 3000.0f;
			pos[2] = z * 8000.0f - 4000.0f;
			FUN_00134030((undefined4 *)DAT_003022cc, pos, vel, -302.0f, 0x7f, 0x7f, 0x7f, 200, 200.0f);
		}
	}
}

/* variation 1-2: lightning: one frame in 100 the flash restarts (the sky actor +0x80 shows,
"lightening" plays); the flash light is on for frames 0-10 of the flash except 3, 4 and 5 */
static void lightning(void *self)
{
	char *sky = FH_AP(self, 0x80);
	unsigned int flash;

	if (fh_randf() > 0.99f) {
		/* a strike position is drawn but not used */
		(void)fh_rand();
		(void)fh_rand();
		FH_AF(sky, 0xd4) = 0.0f;
		FHunt_Sound_Play(fh_Speech, 8, 0xff, 0);
		FH_AF(self, 0x220) = 0.0f;
	}
	flash = FH_AU(self, 0x220);
	if (FH_AF(self, 0x220) > 10.0f || flash == 0x40400000u || flash == 0x40800000u || flash == 0x40a00000u) {
		baEnableLight((struct BLight *)GM_LIGHT2(self), 0);
		FH_AF(FH_AP(self, 0x80), 0xd4) = -20000.0f;
	} else {
		baEnableLight((struct BLight *)GM_LIGHT2(self), 1);
		FH_AF(FH_AP(self, 0x80), 0xd4) = 0.0f;
	}
}

/* variation 3-4: bubbles and debris from two vents and splashes around the beach */
static void beach_bubbles(void *self, double chance)
{
	float pos[4];
	float vel[4];
	float x, z;
	int i;

	FUN_00137f80();
	FUN_00135d50();
	vel[3] = 1.0f;
	pos[0] = -1372.0f;
	pos[1] = 821.0f;
	pos[2] = 2042.0f;
	vel[0] = (fh_randf() - 0.5f) * 0.0833333358f;
	vel[1] = DAT_0028fda8 * FH_AF(self, 0x190);
	vel[2] = (fh_randf() - 0.5f) * 0.0833333358f;
	if ((double)fh_randf() > chance)
		FoxHunt_AddDebris(7, pos, vel, 0, 0xffffffffu);
	pos[0] = 1115.0f;
	pos[1] = 250.0f;
	pos[2] = 1065.0f;
	if ((double)fh_randf() > chance)
		FoxHunt_AddDebris(7, pos, vel, 0, 0xffffffffu);
	for (i = 2; i != 0; i--) {
		vel[0] = (fh_randf() - 0.5f) * 17.5f;
		vel[1] = DAT_0028fda8 * 7.5f;
		pos[0] = 0.0f;
		pos[1] = 150.0f;
		pos[2] = -500.0f;
		vel[2] = (fh_randf() - 0.5f) * 17.5f;
		FoxHunt_AddDebris(0x21, pos, vel, 0, 0xffffffffu);
		x = (float)fh_rand() * 0.00610370189f - 100.0f;
		z = ((float)fh_rand() * 0.00610370189f - 100.0f) - 500.0f;
		water_splash(x, z, -5.0f, 5.0f, 10.0f);
	}
}

/* 0x00126220  CGameMode_FoxHunt.cpp: the rules of one frame: the players' bonus points and
falls, the throwables, the goals of variations 1-2 (scoring with the ball or the flag), the
eliminations of variations 3-4, the ranks, the HUD, the weather and the water, then every
subsystem's update */
void CGameMode_FoxHunt_Update(void *self)
{
	char *p;
	char *actor;
	char **slot;
	int i, n, score, count, rec;

	if (GM_STATE(self) == 2) {
		for (i = 0; i < DAT_002bae94; i++) {
			FH_AI(self, 0x28 + i * 4) = 0;
			FH_AI(PL(i), 0x11f0) = 0;
			if (FH_AI(PL(i), 0x11c4) != 0)
				FH_AI(PL(i), 0x11f0) += 3;
			p = PL(i);
			if (FH_AI(p, 0x11d8) == 1)
				FH_AI(p, 0x11f0) = FH_AI(p, 0x11f0) + 3;
			else if (FH_AI(p, 0x11d8) == 2)
				FH_AI(p, 0x11f0) = FH_AI(p, 0x11f0) + 2;
			else if (FH_AI(p, 0x11d8) == 3)
				FH_AI(p, 0x11f0) = FH_AI(p, 0x11f0) + 1;
			if (FH_AI(PL(i), 0x11ac) == 0)
				FH_AI(PL(i), 0x11f0) = 0;
			FH_AI(PL(i), 0x11d4) = FH_AI(self, 0xa8);
			p = PL(i);
			if (FH_AF(PL_ACTOR(p), 0xd4) <= -690.0f || FH_AF(PL_ACTOR(p), 0xd4) >= 4000.0f) {
				/* fell out of the level */
				if (FH_AI(p, 0x11bc) == 1)
					drop_held_throwable(PL(i));
				FUN_00133370((int)(uintptr_t)PL(i));
				if (FH_AI(PL(i), 0x11c4) == 1 && GM_VARIATION(self) != 4) {
					FH_AI(PL(i), 0x11c4) = 0;
					FUN_00136530(FH_AI(PL(i), 0x11c8));
					FH_AI(PL(i), 0x11c8) = -1;
				}
				if (GM_VARIATION(self) == 2) {
					FH_AI(PL(i), 0x11dc) = 0;
					FH_AI(PL(i), 0x11e0) = 0;
					FH_AI(PL(i), 0x11e4) = 0;
					FH_AI(PL(i), 0x11e8) = 0;
					FUN_001382a0();
				}
			}
			if (DAT_002baed8[i * 10] == 0 && FH_AI(PL(i), 0x11ac) == 1)
				FUN_0012fb60(PL(i));
		}
		FUN_00131240();
		RespawnPoints_UpdateAll();
		FUN_000ba6d0();
		if (FH_AI(self, 0x1dc) == 1) {
			for (slot = (char **)FH_ADDR(0x003022d4); slot < (char **)FH_ADDR(0x003022f4); slot++) {
				FUN_00134770(*slot);
				if (WATER() != NULL && GM_VARIATION(self) < 3) {
					actor = FH_AP(*slot, 0x14);
					if (FH_AF(actor, 0xd4) < -250.0f && FH_AI(*slot, 0x4c) == 0) {
						water_splash(FH_AF(actor, 0xd0), FH_AF(actor, 0xd8), -6.0f, 10.0f, 30.0f);
						FHunt_Sound_Play(fh_Speech, 2, 0xff, 0);
						FH_AI(*slot, 0x4c) = 1;
					}
				}
				if (FH_AI(*slot, 0x48) > -1 && FH_AI(*slot, 0xc) == 1)
					FH_AP(self, 0x28 + FH_AI(*slot, 0x48) * 4) = FH_AP(*slot, 0x14) + 0x10;
			}
		}
		if (GM_STATE(self) == 2) {   /* (tested again in the original) */
			if (GM_VARIATION(self) == 1) {
				/* Cooperation: two players at the two goals at once score 10 each */
				CFHunt_GenericGoal_SetColour(FH_AP(self, 0x27c), GOAL_A(), 0x7f7f);
				CFHunt_GenericGoal_SetColour(FH_AP(self, 0x27c), GOAL_B(), 0x7f7f);
				FH_AI(GOAL_A(), 0x34) = -1;
				FH_AI(GOAL_B(), 0x34) = -1;
				for (i = 0; i < DAT_002bae94; i++) {
					FH_AI(PL(i), 0x11b8) = 0;
					if (FH_AI(PL(i), 0x11c4) == 1) {
						char *goal;

						if (fh_DistanceSquared(FH_AV(GOAL_A(), 0x14), FH_AV(PL_ACTOR(PL(i)), 0xd0)) < 17000.0f) {
							FH_AI(GOAL_A(), 0x34) = i;
							goal = GOAL_A();
						} else if (fh_DistanceSquared(FH_AV(GOAL_B(), 0x14), FH_AV(PL_ACTOR(PL(i)), 0xd0)) < 17000.0f) {
							FH_AI(GOAL_B(), 0x34) = i;
							goal = GOAL_B();
						} else {
							continue;
						}
						FH_AI(PL(i), 0x11b8) = 1;
						CFHunt_GenericGoal_SetColour(FH_AP(self, 0x27c), goal,
							(int)(((unsigned int)CHAR_R(i) << 16) | ((unsigned int)CHAR_G(i) << 8) | CHAR_B(i)));
					}
				}
				n = FH_AU(GOAL_A(), 0x34) < 0x80000000u;
				if (FH_AI(GOAL_B(), 0x34) > -1)
					n++;
				if (n == 2) {
					char *a = PL(FH_AI(GOAL_A(), 0x34));
					char *b = PL(FH_AI(GOAL_B(), 0x34));

					FH_AI(a, 0x11d0) += 10;
					FH_AI(PL(FH_AI(GOAL_A(), 0x34)), 0x11c4) = 3;
					FUN_0012ef90(a, 3);
					CLSamples_PlayFromSet((char)DAT_002baed4[FH_AUS(PL_ACTOR(PL(FH_AI(GOAL_A(), 0x34))), 0x230) * 10],
						4, NULL, DAT_00293928, 0, -1, 0);
					FH_AI(b, 0x11d0) += 10;
					FH_AI(PL(FH_AI(GOAL_B(), 0x34)), 0x11c4) = 3;
					FUN_0012ef90(b, 3);
					CLSamples_PlayFromSet((char)DAT_002baed4[FH_AUS(PL_ACTOR(PL(FH_AI(GOAL_B(), 0x34))), 0x230) * 10],
						4, NULL, DAT_00293928, 0, -1, 0);
					rec = FH_AI(GOAL_A(), 0x34);
					CFHunt_GenericGoal_StartEffect(FH_AP(self, 0x27c), GOAL_A(), CHAR_R(rec), CHAR_G(rec), CHAR_B(rec), 2);
					rec = FH_AI(GOAL_A(), 0x34);
					Light_Flash(FH_AV(GOAL_A(), 0x14), CHAR_R(rec), CHAR_G(rec), CHAR_B(rec));
					rec = FH_AI(GOAL_B(), 0x34);
					CFHunt_GenericGoal_StartEffect(FH_AP(self, 0x27c), GOAL_B(), CHAR_R(rec), CHAR_G(rec), CHAR_B(rec), 2);
					rec = FH_AI(GOAL_A(), 0x34);   /* (the original uses goal A's player here too) */
					Light_Flash(FH_AV(GOAL_B(), 0x14), CHAR_R(rec), CHAR_G(rec), CHAR_B(rec));
					FH_AI(GOAL_A(), 0x34) = -1;
					FH_AI(GOAL_B(), 0x34) = -1;
					FHunt_Sound_Play(fh_Speech, 1, 0xff, 0);
				}
			} else if (GM_VARIATION(self) == 2) {
				/* Energy Ball: a ball carried into a goal scores; the effect grows with the
				number of bonuses the carrier collected */
				CFHunt_GenericGoal_SetColour(FH_AP(self, 0x27c), GOAL_A(), 0x7f7f);
				CFHunt_GenericGoal_SetColour(FH_AP(self, 0x27c), GOAL_B(), 0x7f7f);
				for (i = 0; i < DAT_002bae94; i++) {
					char *goal;
					int level;

					FH_AI(PL(i), 0x11b8) = 0;
					p = PL(i);
					level = FH_AI(p, 0x11dc) == 1;
					if (FH_AI(p, 0x11e0) == 1)
						level++;
					if (FH_AI(p, 0x11e4) == 1)
						level++;
					if (FH_AI(p, 0x11c4) != 1)
						continue;
					if (fh_DistanceSquared(FH_AV(GOAL_A(), 0x14), FH_AV(PL_ACTOR(p), 0xd0)) < 17000.0f) {
						goal = GOAL_A();
						FH_AI(goal, 0x34) = i;
						FH_AI(PL(FH_AI(goal, 0x34)), 0x11c4) = 3;
						FUN_0012ef90(PL(FH_AI(goal, 0x34)), 3);
						FH_AI(goal, 0x4) = FH_AI(goal, 0x14);
						FH_AI(goal, 0x8) = FH_AI(goal, 0x18);
						FH_AI(goal, 0xc) = FH_AI(goal, 0x1c);
						FH_AI(goal, 0x10) = FH_AI(goal, 0x20);
						FH_AI(GOAL_A(), 0x28) = 2;
						CLSamples_PlayFromSet((char)DAT_002baed4[FH_AUS(PL_ACTOR(PL(FH_AI(GOAL_A(), 0x34))), 0x230) * 10],
							4, NULL, DAT_00293928, 0, -1, 0);
						rec = FH_AI(GOAL_A(), 0x34);
						Light_Flash(FH_AV(GOAL_A(), 0x14), CHAR_R(rec), CHAR_G(rec), CHAR_B(rec));
						goal = GOAL_A();
					} else if (fh_DistanceSquared(FH_AV(GOAL_B(), 0x14), FH_AV(PL_ACTOR(PL(i)), 0xd0)) < 17000.0f) {
						goal = GOAL_B();
						FH_AI(goal, 0x34) = i;
						FH_AI(PL(FH_AI(goal, 0x34)), 0x11c4) = 3;
						FH_AI(goal, 0x4) = FH_AI(goal, 0x14);
						FH_AI(goal, 0x8) = FH_AI(goal, 0x18);
						FH_AI(goal, 0xc) = FH_AI(goal, 0x1c);
						FH_AI(goal, 0x10) = FH_AI(goal, 0x20);
						FH_AI(GOAL_B(), 0x28) = 2;
						rec = FH_AI(GOAL_B(), 0x34);
						Light_Flash(FH_AV(GOAL_B(), 0x14), CHAR_R(rec), CHAR_G(rec), CHAR_B(rec));
						goal = GOAL_B();
					} else {
						continue;
					}
					if (level >= 0 && level <= 3) {
						rec = FH_AI(goal, 0x34);
						CFHunt_GenericGoal_StartEffect(FH_AP(self, 0x27c), goal, CHAR_R(rec), CHAR_G(rec), CHAR_B(rec),
							(unsigned int)level);
					}
					FHunt_Sound_Play(fh_Speech, 1, 0xff, 0);
				}
			} else if (GM_VARIATION(self) == 3) {
				/* Odd One Out: when the timer runs out, every active player without the flag
				is out */
				for (i = 0; i < DAT_002bae94; i++) {
					if (FH_AI(PL(i), 0x11c4) == 1)
						FH_AI(PL(i), 0x11d0) = 1;
					FH_AI(self, 0xc4 + i * 4) = FH_AI(PL(i), 0x11d0) / 10;
				}
				if (FH_AF(self, 0xa0) < 360.0f) {
					if (FH_AF(self, 0x18c) > 59.0f) {
						FHunt_Sound_Play(fh_Speech, 0xf, 0xff, 0);
						FH_AF(self, 0x18c) = 0.0f;
					}
					FH_AF(self, 0x18c) = DAT_0028fda8 + FH_AF(self, 0x18c);
				}
				if (FH_AF(self, 0xa0) <= 0.0f) {
					FH_AI(DAT_002ecfac, 0xcc) = 1;
					FH_AF(self, 0xa0) = 1800.0f;
					/* the lowest score of the active players (not used) */
					score = 100000;
					for (i = 0; i < DAT_002bae94; i++) {
						if (FH_AI(PL(i), 0x11d0) < score && FH_AI(PL(i), 0x11ac) == 1)
							score = FH_AI(PL(i), 0x11d0);
					}
					(void)score;
					for (i = 0; i < DAT_002bae94; i++) {
						p = PL(i);
						if (FH_AI(p, 0x11c4) != 0 || FH_AI(p, 0x11ac) != 1)
							continue;
						FH_AI(p, 0x11ac) = 0;
						FH_AI(PL_ACTOR(PL(i)), 0x220) = 0;
						FH_AI(PL(i), 0x11d8) = FH_AI(PL(i), 0x11b0);
						FHunt_Sound_Play(fh_Speech, 9, 0xff, 0);
						count = count_active();
						if (count > 1)
							CLSamples_PlayCommentary(0x37, DAT_002baed4[i * 10], 0xff, 0, 0, 0);
						FUN_0012ef90(PL(i), 2);
						elimination_sparks(i, i, 0);
						FH_AF(PL_ACTOR(PL(i)), 0xd0) = (float)i * 100.0f;
						FH_AF(PL_ACTOR(PL(i)), 0xd4) = 0.0f;
						FH_AF(PL_ACTOR(PL(i)), 0xd8) = 1500.0f;
						FH_AI(PL(i), 0x11c4) = 0;
						FH_AI(PL(i), 0x11c8) = -1;
						FH_AI(PL(i), 0x11d8) = FH_AI(self, 0xd4);
						FH_AI(PL(i), 0x11d0) = 0;
						bkPrintf("NumberOfActive Players %d\n", count);
						FUN_00136e60(count);
					}
					FH_AI(self, 0xd4) = 0;
					for (i = 0; i < 4; i++) {
						if (FH_AI(PL(i), 0x11ac) == 1) {
							FH_AI(PL(i), 0x11d0) = 1;
							FH_AI(PL(i), 0x11d8) = 1;
							FH_AI(self, 0xd4)++;
						}
					}
					FUN_001364c0();
				}
				for (i = 0; i < DAT_002bae94; i++) {
					if (FH_AI(PL(i), 0x11ac) == 0) {
						actor = PL_ACTOR(PL(i));
						FH_AU(actor, 0xe0) = 0x3a83126fu;   /* 0.001 */
						FH_AU(PL_ACTOR(PL(i)), 0xe4) = 0x3a83126fu;
						FH_AU(PL_ACTOR(PL(i)), 0xe8) = 0x3a83126fu;
						FH_AF(PL_ACTOR(PL(i)), 0xd0) = 100.0f;
						FH_AF(PL_ACTOR(PL(i)), 0xd4) = 600.0f;
						FH_AF(PL_ACTOR(PL(i)), 0xd8) = 1500.0f;
					}
				}
			} else if (GM_VARIATION(self) == 4) {
				/* Victim: when the timer runs out, the player with the flag is out */
				FH_AI(self, 0x164) = 0;
				for (i = 0; i < DAT_002bae94; i++) {
					p = PL(i);
					if (FH_AI(p, 0x11ac) == 0) {
						FH_AI(p, 0x11d0) = 0;
						FH_AF(PL_ACTOR(PL(i)), 0xd0) = (float)i * 100.0f;
						FH_AF(PL_ACTOR(PL(i)), 0xd4) = 0.0f;
						FH_AF(PL_ACTOR(PL(i)), 0xd8) = 1300.0f;
					} else if (FH_AI(p, 0x11ac) == 1) {
						FH_AI(p, 0x11d0) = 1;
						FH_AI(PL(i), 0x11d8) = 1;
						FH_AI(self, 0xc0) = i;
					}
				}
				if (FH_AF(self, 0xa0) < 360.0f) {
					FH_AI(self, 0x164) = 1;
					if (FH_AF(self, 0x18c) > 59.0f) {
						FHunt_Sound_Play(fh_Speech, 0xf, 0xff, 0);
						FH_AF(self, 0x18c) = 0.0f;
					}
					FH_AF(self, 0x18c) = DAT_0028fda8 + FH_AF(self, 0x18c);
				}
				if (FH_AF(self, 0xa0) <= 0.0f) {
					count = 0;
					for (i = 0; i < DAT_002bae94; i++) {
						p = PL(i);
						if (FH_AI(p, 0x11c4) != 1 || FH_AI(p, 0x11ac) != 1)
							continue;
						FH_AI(p, 0x11ac) = 0;
						FH_AI(PL_ACTOR(PL(i)), 0x220) = 0;
						FH_AI(PL(i), 0x11d8) = FH_AI(PL(i), 0x11b0);
						FH_AI(PL(i), 0x11d0) = 0;
						elimination_sparks(i, i, 1);
						FH_AF(PL_ACTOR(PL(i)), 0xd0) = (float)i * 100.0f;
						FH_AF(PL_ACTOR(PL(i)), 0xd4) = 0.0f;
						FH_AF(PL_ACTOR(PL(i)), 0xd8) = 1300.0f;
						FHunt_Sound_Play(fh_Speech, 9, 0xff, 0);
						count += count_active();
						if (count > 1)
							CLSamples_PlayCommentary(0x37, DAT_002baed4[i * 10], 0xff, 0, 0, 0);
						FUN_0012ef90(PL(i), 2);
						if (FH_AI(PL(i), 0x11c4) == 1) {
							FH_AI(PL(i), 0x11c4) = 0;
							FUN_00136470(FH_AI(PL(i), 0x11c8));
							FH_AI(PL(i), 0x11c8) = -1;
						}
					}
					FUN_001364c0();
					FH_AF(self, 0xa0) = 1800.0f;
				}
			}
		}
		if (GM_VARIATION(self) < 3) {
			/* the ranks by score */
			for (i = 0; i < DAT_002bae94; i++) {
				score = FH_AI(PL(i), 0x11d0);
				if (score < FH_AI(self, 0xb0)) {
					if (score < FH_AI(self, 0xb4)) {
						if (score < FH_AI(self, 0xb8)) {
							if (FH_AI(self, 0xbc) <= score) {
								FH_AI(self, 0xbc) = score;
								FH_AI(PL(i), 0x11d8) = 4;
							}
						} else {
							FH_AI(self, 0xb8) = score;
							FH_AI(PL(i), 0x11d8) = 3;
						}
					} else {
						FH_AI(self, 0xb4) = score;
						FH_AI(PL(i), 0x11d8) = 2;
					}
				} else {
					FH_AI(self, 0xc0) = i;
					FH_AI(self, 0xb0) = FH_AI(PL(i), 0x11d0);
					FH_AI(PL(i), 0x11d8) = 1;
				}
				if (GM_VARIATION(self) == 4 && FH_AI(self, 0xd4) != 0 && FH_AI(PL(i), 0x11ac) == 1) {
					FH_AI(self, 0xc0) = i;
					FH_AI(PL(i), 0x11d8) = 1;
				}
			}
		}
	}
	if (GM_STATE(self) != 6) {
		/* the HUD: scores (variation 3: in or out), the flag/ball icons, the best score */
		for (i = 0; i < DAT_002bae94; i++) {
			if (GM_VARIATION(self) == 3)
				FH_AI(self, 0xc4 + i * 4) = FH_AI(PL(i), 0x11ac);
			else
				FH_AI(self, 0xc4 + i * 4) = FH_AI(PL(i), 0x11d0);
			if (DAT_002baed8[i * 10] == 1 && GM_STATE(self) == 2) {
				if (FH_AI(PL(i), 0x11c4) == 1)
					FH_AS(FH_P(0x00302164 + i * 8), 0x54) = 0;
				else if (FH_AI(PL(i), 0x11c4) == 0)
					FH_AS(FH_P(0x00302164 + i * 8), 0x54) = 1;
				if (FH_AI(PL(i), 0x11ac) == 0)
					FH_AS(FH_P(0x00302168 + i * 8), 0x54) = 1;
				if (FH_AI(PL(i), 0x11ac) == 1)
					FH_AS(FH_P(0x00302168 + i * 8), 0x54) = 1;
			} else {
				FH_AS(FH_P(0x00302168 + i * 8), 0x54) = 1;
				FH_AS(FH_P(0x00302164 + i * 8), 0x54) = 1;
			}
			if (GM_VARIATION(self) < 3) {
				score = FH_AI(self, 0xb0);
				FH_AI(DAT_002ecfac, 0x3c) = 0;
				if (score < 1)
					FH_AF(DAT_002ecfac, 0x40) = 10.0f;
				else
					FH_AF(DAT_002ecfac, 0x40) = (float)score;
			} else if (GM_VARIATION(self) == 3) {
				FH_AF(DAT_002ecfac, 0x40) = 1.0f;
				FH_AI(DAT_002ecfac, 0x3c) = 0;
			} else {
				score = FH_AI(self, 0xb0);
				FH_AI(DAT_002ecfac, 0x3c) = 0;
				FH_AF(DAT_002ecfac, 0x40) = (float)score;
			}
		}
		CLHud_SetScoresInt((int)(uintptr_t)DAT_002ecfac, (int *)((char *)self + 0xc4));
		CLHud_SetScores((int)(uintptr_t)DAT_002ecfac, (int *)((char *)self + 0xc4));
		CLHud_UpdateBars((int)(uintptr_t)DAT_002ecfac);
		if (GM_STATE(self) == 2 || GM_STATE(self) == 0 || GM_STATE(self) == 1) {
			if (GM_VARIATION(self) < 3 || GM_VARIATION(self) == 3 ||
				(GM_VARIATION(self) == 4 && FH_AI(self, 0x1c4) == 0))
				CLHud_SetClock((int)(uintptr_t)DAT_002ecfac, FH_AF(self, 0x9c) * (1.0f / 60.0f), 1);
		}
		DebrisColours_SetAll(FH_AV(PL_ACTOR(PL(0)), 0xd0), FH_AV(PL_ACTOR(PL(1)), 0xd0),
			FH_AV(PL_ACTOR(PL(2)), 0xd0), FH_AV(PL_ACTOR(PL(3)), 0xd0));
	}
	/* the weather */
	if (GM_VARIATION(self) == 2) {
		FUN_00138670();
		rain(self, FH_AI(self, 0x194));
		lightning(self);
	} else if (GM_VARIATION(self) == 1) {
		rain(self, 6);
		lightning(self);
	} else if (GM_VARIATION(self) == 3) {
		beach_bubbles(self, 0.95);
	} else if (GM_VARIATION(self) == 4) {
		beach_bubbles(self, 0.93);
	}
	/* falls into the water */
	if (GM_VARIATION(self) < 3) {
		FUN_001379e0();
		for (i = 0; i < DAT_002bae94; i++) {
			actor = PL_ACTOR(PL(i));
			if (FH_AF(actor, 0xd4) <= -300.0f && FH_AI(PL(i), 0x11e8) == 0) {
				water_splash(FH_AF(actor, 0xd0), FH_AF(actor, 0xd8), -12.0f, 20.0f, 50.0f);
				FHunt_Sound_Play(fh_Speech, 2, 0xff, 0);
				FH_AI(PL(i), 0x11e8) = 1;
				FH_AF(PL_ACTOR(PL(i)), 0xd4) = -300.0f;
				if (FH_AI(PL(i), 0x11bc) == 1)
					drop_held_throwable(PL(i));
				if (FH_AI(PL(i), 0x11c4) == 1) {
					FUN_00136530(FH_AI(PL(i), 0x11c8));
					FH_AI(PL(i), 0x11c8) = -1;
					FH_AI(PL(i), 0x11c4) = 0;
					FH_AI(PL(i), 0x11dc) = 0;
					FH_AI(PL(i), 0x11e0) = 0;
					FH_AI(PL(i), 0x11e4) = 0;
					if (GM_VARIATION(self) == 2)
						FUN_001382a0();
				}
				AI_SetState(PL(i), 0x10);
				FH_AI(PL_ACTOR(PL(i)), 0x220) = 0;
			}
		}
		for (n = 0; n < FH_AI(self, 0x194) / 2; n++)
			FH_VFN(WATER(), 0x24, int (*)(void *, float))(WATER(), 0.3f);
	} else if (GM_VARIATION(self) > 2) {
		/* the animated sea texture, and splashes of players in the sea */
		if (FH_AI(self, 0x214) < 6) {
			if (fh_randf() > 0.6f)
				FH_AI(self, 0x214)++;
		} else {
			FH_AI(self, 0x214) = 0;
		}
		bdTextureSetFrame((struct bdTexture *)FH_AP(self, 0x210), FH_AI(self, 0x214));
		for (i = 0; i < DAT_002bae94; i++) {
			FH_AI(PL(i), 0x11e8)++;
			p = PL(i);
			actor = PL_ACTOR(p);
			if ((FH_AF(actor, 0xd4) <= -5.0f && FH_AF(actor, 0x19c) > 1.0f) ||
				(FH_AF(actor, 0xd4) <= -5.0f && FH_AS(p, 0x36) == 4)) {
				if (FH_AI(p, 0x11e8) > 5) {
					FHunt_Sound_Play(fh_Speech, 2, 0xff, 0);
					FH_AI(PL(i), 0x11e8) = 0;
				}
				water_splash(FH_AF(PL_ACTOR(PL(i)), 0xd0), FH_AF(PL_ACTOR(PL(i)), 0xd8), -12.0f, 20.0f, 50.0f);
			}
		}
	}
	if (FH_AI(self, 0x1a8) == 1)
		FH_VFN(WATER(), 0xc, void (*)(void *))(WATER());
	Lights_UpdateAll();
	Actor_UpdateAll();
	if (GM_VARIATION(self) < 3)
		CFHunt_GenericGoal_Update(FH_AP(self, 0x27c), DAT_0028fda8);
	if (FH_AI(self, 0x1d0) == 1)
		Shadows_Aim();
	RefBot_Update();
	Actor_UpdateAnimations();
	FUN_00137940(FH_P(0x00302198) + 0x534, FH_AI(self, 0x164));
	FUN_00136390();
	FoxHunt_Effects_Update(DAT_003022cc, DAT_0028fda8 * (1.0f / 60.0f));
	CRayControl_UpdateAll();
	CLHud_AddSeconds((int)(uintptr_t)DAT_002ecfac, DAT_0028fda8);
	COverlay_UpdateAll();
	CProjector_UpdateAll();
	FH_AF(self, 0xa8) = DAT_0028fda8 + FH_AF(self, 0xa8);
	FH_AF(self, 0x220) = FH_AF(self, 0x220) + 1.0f;
}

/* the scores shown on the results screen: variations 1-2 the points, 3-4 the place */
static void results_scores(void *self)
{
	int i;

	for (i = 0; i < DAT_002bae94; i++) {
		if (GM_VARIATION(self) < 3)
			FH_AI(self, 0xc4 + i * 4) = FH_AI(PL(i), 0x11d0);
		else
			FH_AI(self, 0xc4 + i * 4) = 5 - FH_AI(PL(i), 0x11d8);
	}
}

/* 0x00128900  CGameMode_FoxHunt.cpp: the round state machine: frame timing; intro (the
fly-over camera, until it ends or a button is pressed: then the countdown starts at the
camera path's last point); countdown (then play); play (counts the active players, ends
the round when fewer than two are left or the time is up, and sets the final places) */
void CGameMode_FoxHunt_UpdateState(void *self)
{
	char **slot;
	char *cam;
	float tmp[4];
	int i, n, done;
	unsigned long long now, delta;

	FH_AI(self, 0x1f8) = FH_AI(self, 0x1f0);
	FH_AI(self, 0x1fc) = FH_AI(self, 0x1f4);
	now = bkTimerRead();
	FH_AU(self, 0x1f4) = (unsigned int)(now >> 32);
	FH_AU(self, 0x1f0) = (unsigned int)now;
	delta = bkTimerDelta(((unsigned long long)FH_AU(self, 0x1fc) << 32) | FH_AU(self, 0x1f8), now);
	FH_AU(self, 0x200) = (unsigned int)delta;
	FH_AU(self, 0x204) = (unsigned int)(delta >> 32);
	FH_AF(self, 0x208) = bkTimerRate((long long)delta);

	if (GM_STATE(self) == 2 || GM_STATE(self) == 4 || GM_STATE(self) == 5) {
		n = 0;
		for (i = 0; i < DAT_002bae94; i++) {
			if (FH_AI(PL(i), 0x11ac) == 1)
				n++;
		}
		FH_AI(self, 0xd4) = n;
		for (i = 0; i < DAT_002bae94; i++)
			FH_AI(PL(i), 0x11b0) = n;
		if (FH_AI(self, 0xd4) < 2 && GM_STATE(self) == 2) {
			if (GM_VARIATION(self) < 3) {
				GM_STATE(self) = 4;
				FH_AI(self, 0x9c) = 0;
				FH_AI(DAT_002ecfac, 0xcc) = 1;
				FUN_00136de0();
				results_scores(self);
			} else {
				if (FH_AI(self, 0x16c) == 0)
					FUN_00136de0();
				FH_AI(DAT_002ecfac, 0xcc) = 1;
				FH_AI(self, 0x16c)++;
				if (FH_AI(self, 0x16c) < 0x78)
					goto timers;
				GM_STATE(self) = 4;
				FH_AI(self, 0x9c) = 0;
				results_scores(self);
			}
			FH_AI(self, 0x168) = CLHud_CreateWinnerText((int)(uintptr_t)DAT_002ecfac, (int *)((char *)self + 0xc4), 0);
		}
timers:
		if (GM_STATE(self) == 2)
			FH_AF(self, 0xa0) = FH_AF(self, 0xa0) - DAT_0028fda8;
		if (FH_AF(self, 0x9c) > 0.0f)
			FH_AF(self, 0x9c) = FH_AF(self, 0x9c) - DAT_0028fda8;
		if (FH_AF(self, 0x9c) > 0.0f)
			return;
		if (GM_VARIATION(self) > 2)
			return;
		if (GM_STATE(self) != 2)
			return;
		GM_STATE(self) = 4;
		FUN_00136de0();
		if (GM_VARIATION(self) == 3) {
			if (FH_AI(self, 0xd4) == 2) {
				for (i = 0; i < DAT_002bae94; i++) {
					char *p = PL(i);

					if (FH_AI(p, 0x11ac) == 1) {
						if (FH_AI(p, 0x11c4) == 1) {
							FH_AI(p, 0x11d8) = 1;
							FH_AI(self, 0xc0) = i;
						} else if (FH_AI(p, 0x11c4) == 0) {
							FH_AI(p, 0x11d8) = 2;
						}
					}
				}
			}
		} else if (GM_VARIATION(self) == 4 && FH_AI(self, 0xd4) == 2) {
			for (i = 0; i < DAT_002bae94; i++) {
				char *p = PL(i);

				if (FH_AI(p, 0x11ac) == 1) {
					if (FH_AI(p, 0x11c4) == 1) {
						FH_AI(p, 0x11d8) = 2;
					} else if (FH_AI(p, 0x11c4) == 0) {
						FH_AI(p, 0x11d8) = 1;
						FH_AI(self, 0xc0) = i;
					}
				}
			}
		}
		results_scores(self);
		FH_AI(self, 0x168) = CLHud_CreateWinnerText((int)(uintptr_t)DAT_002ecfac, (int *)((char *)self + 0xc4), 0);
		return;
	}
	if (GM_STATE(self) != 0) {
		if (GM_STATE(self) != 1)
			return;
		/* the countdown */
		if (CLHud_UpdateCountdown((int)(uintptr_t)DAT_002ecfac) == 0)
			return;
		GM_STATE(self) = 2;
		FH_AI(self, 0xf8) = 0;
		FH_AF(self, 0x160) = 1.0f;
		for (slot = fh_Players; slot < fh_Players + 4; slot++)
			FH_VFN(*slot, 0x44, void (*)(void *, int))(*slot, 1);
		Actor_UpdateAnimations();
		return;
	}
	/* the intro */
	for (slot = (char **)FH_ADDR(0x003022d4); slot < (char **)FH_ADDR(0x003022f4); slot++)
		FUN_00134770(*slot);
	done = CLIntroCam_Update(DAT_0028fda8, 0);
	if (GM_VARIATION(self) == 1) {
		FH_AF(FH_AP(self, 0x84), 0xd8) = DAT_0028fda8 * 12.0f + FH_AF(FH_AP(self, 0x84), 0xd8);
		if ((double)fh_randf() < 0.3)
			water_splash(FH_AF(FH_AP(self, 0x84), 0xd0), FH_AF(FH_AP(self, 0x84), 0xd8), -50.0f, 15.0f, 30.0f);
	} else if (GM_VARIATION(self) == 2) {
		FH_AF(FH_AP(self, 0x84), 0xd8) = DAT_0028fda8 * 6.0f + FH_AF(FH_AP(self, 0x84), 0xd8);
		FH_AF(FH_AP(self, 0x84), 0xd0) = DAT_0028fda8 * 6.0f + FH_AF(FH_AP(self, 0x84), 0xd0);
		if (Core_RandUnit() < 0.3f)
			water_splash(FH_AF(FH_AP(self, 0x84), 0xd0), FH_AF(FH_AP(self, 0x84), 0xd8), -50.0f, 15.0f, 30.0f);
	}
	for (i = 0; i < DAT_002bae94; i++) {
		if (FH_AI(FH_AP(PL(i), 0x6c), 0x14) != 0 || FH_AI(FH_AP(PL(i), 0x68), 0x14) != 0)
			done = 1;
	}
	if (done == 0)
		return;
	if (GM_VARIATION(self) < 3) {
		Actor_Delete(FH_AP(self, 0x84));
		FH_AP(self, 0x84) = NULL;
	}
	/* the camera starts at the last point of the intro path */
	n = CLIntroCam_GetFrameCount();
	CLIntroCam_GetPosition(tmp, (float)n - 1.0f);
	FH_AF(self, 0x11c) = tmp[0];
	FH_AF(self, 0x120) = tmp[1];
	FH_AF(self, 0x124) = tmp[2];
	FH_AF(self, 0x128) = 1.0f;
	DAT_00294b98[0] = tmp[0];
	DAT_00294b98[1] = tmp[1];
	DAT_00294b98[2] = tmp[2];
	DAT_00294b98[3] = 1.0f;
	n = CLIntroCam_GetFrameCount();
	CLIntroCam_GetTarget(tmp, (float)n - 1.0f);
	DAT_00294b98[6] = tmp[2];
	DAT_00294b98[4] = tmp[0];
	DAT_00294b98[5] = tmp[1];
	DAT_00294b98[7] = 1.0f;
	FH_AF(self, 0x134) = tmp[2];
	FH_AF(self, 0x12c) = tmp[0];
	FH_AF(self, 0x130) = tmp[1];
	FH_AF(self, 0x138) = 1.0f;
	cam = FH_P(0x00302198);
	FH_AI(cam, 0x534) = FH_AI(self, 0x11c);
	FH_AI(cam, 0x538) = FH_AI(self, 0x120);
	FH_AI(cam, 0x53c) = FH_AI(self, 0x124);
	FH_AI(cam, 0x540) = FH_AI(self, 0x128);
	cam = FH_P(0x00302198);
	FH_AI(cam, 0x544) = FH_AI(self, 0x12c);
	FH_AI(cam, 0x548) = FH_AI(self, 0x130);
	FH_AI(cam, 0x54c) = FH_AI(self, 0x134);
	FH_AI(cam, 0x550) = FH_AI(self, 0x138);
	cam = FH_P(0x00302198);
	FH_AI(cam, 0x4e4) = FH_AI(self, 0x11c);
	FH_AI(cam, 0x4e8) = FH_AI(self, 0x120);
	FH_AI(cam, 0x4ec) = FH_AI(self, 0x124);
	FH_AI(cam, 0x4f0) = FH_AI(self, 0x128);
	cam = FH_P(0x00302198);
	FH_AI(cam, 0x504) = FH_AI(self, 0x12c);
	FH_AI(cam, 0x508) = FH_AI(self, 0x130);
	FH_AI(cam, 0x50c) = FH_AI(self, 0x134);
	FH_AI(cam, 0x510) = FH_AI(self, 0x138);
	bkPrintf("tCas final X %f\n", (double)FH_AF(FH_P(0x00302198), 0x534));
	bkPrintf("tCas final Y %f\n", (double)FH_AF(FH_P(0x00302198), 0x538));
	bkPrintf("tCas final Z %f\n", (double)FH_AF(FH_P(0x00302198), 0x53c));
	CLIntroCam_Destroy();
	FH_AI(self, 0x160) = 0;
	FH_AI(self, 0xf8) = 0;
	GM_STATE(self) = 1;
	bsSetSfxVolume((DAT_002bae6c * 0xff) / 0xff);
	for (slot = fh_Players; slot < fh_Players + 4; slot++)
		AI_SetState(*slot, 0);
	if (GM_VARIATION(self) > 2) {
		cam = FH_P(0x00302198);
		FH_AI(cam, 0x4b0) = FH_AI(self, 0xfc);
		FH_AI(cam, 0x4b4) = FH_AI(self, 0x100);
		FH_AI(cam, 0x4b8) = FH_AI(self, 0x104);
		FH_AI(cam, 0x4bc) = FH_AI(self, 0x108);
	}
	CLHud_StartBars((float *)DAT_002ecfac, 280.0f);
}

/* a results-screen skip test: player `p`'s buttons (+0x6c, +0x68) after the minimum time */
static void results_skip(void *self, int human, char *p)
{
	float t;

	if (human != 0)
		return;
	if (FH_AI(FH_AP(p, 0x6c), 0x14) == 0 && FH_AI(FH_AP(p, 0x68), 0x14) == 0)
		return;
	t = FH_AF(self, 0x178);
	if (FH_AI(self, 0x198) == 1 ? t < 240.0f : t < 120.0f) {
		GM_STATE(self) = 5;
		FH_AI(self, 0x1a0) = 1;
	}
}

/* 0x00129170  CGameMode_FoxHunt.cpp: the results: picks the winners (team mode: the
winning team; else the best place), then shows them one at a time on the podium (town:
(800, 0, -700); beach: (-1534, 800, 1647)) with the camera in front, until the time runs
out or a button is pressed. Returns 1. */
int CGameMode_FoxHunt_Results(void *self)
{
	char **slot;
	int best = 5;
	int i, w;
	char *actor;
	float eye[4];
	float target[4];
	float lift;
	char *winner;

	if (FH_AI(self, 0xd8) == -2) {
		GM_AMBIENT_LIT(self) = 0x6e;
		if (DAT_002baebc == 1) {
			bkPrintf("TeamMode is on according to this\n");
			for (i = 0; i < 4; i++) {
				FH_AI(self, 0xe0 + i * 4) = 0;
				if (FH_AI(self, 0x168) == -1 || DAT_002baef4[i * 10] == FH_AI(self, 0x168)) {
					FH_VFN(PL(i), 0x44, void (*)(void *, int))(PL(i), 0);
					FH_AI(self, 0xe0 + i * 4) = 1;
					FH_AI(self, 0xdc)++;
				}
			}
			FH_AI(self, 0x198) = 2;
		} else {
			for (slot = fh_Players, i = 0; slot < fh_Players + 4; slot++, i++) {
				if (FH_AI(*slot, 0x11d8) <= best)
					best = FH_AI(*slot, 0x11d8);
				FH_AI(self, 0xe0 + i * 4) = 0;
			}
			for (i = 0; i < 4; i++) {
				if (FH_AI(PL(i), 0x11d8) == best) {
					FH_VFN(PL(i), 0x44, void (*)(void *, int))(PL(i), 0);
					FH_AI(self, 0xe0 + i * 4) = 1;
					FH_AI(self, 0xdc)++;
					FH_AI(self, 0xc0) = i;
				}
				FH_AI(self, 0x198) = FH_AI(self, 0xdc);
			}
		}
		FH_AI(self, 0xd8) = FH_AI(self, 0xdc);
		if (FH_AI(self, 0x198) == 1)
			FH_AF(self, 0xf0) = 360.0f;
		else
			FH_AF(self, 0xf0) = 240.0f;
		FH_AF(self, 0x178) = FH_AF(self, 0xf0);
		FH_AI(self, 0xf4) = 1;
		FH_AI(self, 0x17c) = 0;
		if (GM_VARIATION(self) == 3) {
			for (i = 0; i < DAT_002bae94; i++)
				FH_AI(self, 0xc4 + i * 4) = FH_AI(PL(i), 0x11d0);
		}
		CLHud_SetScoresInt((int)(uintptr_t)DAT_002ecfac, (int *)((char *)self + 0xc4));
		CLHud_SetScores((int)(uintptr_t)DAT_002ecfac, (int *)((char *)self + 0xc4));
	}
	/* the first winner still to show */
	w = -1;
	for (i = 3; i >= 0; i--) {
		if (FH_AI(self, 0xe0 + i * 4) == 1) {
			FH_AI(self, 0x19c) = i;
			w = i;
		}
	}
	if (w < 0 || w > 3) {
		w = FH_AI(self, 0x19c);
		GM_STATE(self) = 5;
	}
	actor = PL_ACTOR(PL(w));
	if (GM_VARIATION(self) < 3) {
		FH_AF(actor, 0xd0) = 800.0f;
		FH_AF(PL_ACTOR(PL(w)), 0xd4) = 0.0f;
		FH_AF(PL_ACTOR(PL(w)), 0xd8) = -700.0f;
		FH_AU(PL_ACTOR(PL(w)), 0x1e8) = 0x40ce16a6u;   /* 6.44 */
	} else if (GM_VARIATION(self) > 2) {
		FH_AF(actor, 0xd0) = -1534.0f;
		FH_AF(PL_ACTOR(PL(w)), 0xd4) = 800.0f;
		FH_AF(PL_ACTOR(PL(w)), 0xd8) = 1647.0f;
		FH_AU(PL_ACTOR(PL(w)), 0x1e8) = 0x40714639u;   /* 3.77 */
		if (GM_VARIATION(self) == 4)
			FUN_001363c0();
	}
	FH_AF(PL_ACTOR(PL(w)), 0xe0) = 1.0f;
	FH_AF(PL_ACTOR(PL(w)), 0xe4) = 1.0f;
	FH_AF(PL_ACTOR(PL(w)), 0xe8) = 1.0f;
	winner = PL(w);
	actor = PL_ACTOR(winner);
	target[0] = FH_AF(actor, 0xd0);
	target[1] = FH_AF(actor, 0xd4);
	target[2] = FH_AF(actor, 0xd8);
	target[3] = FH_AF(actor, 0xdc);
	eye[0] = FH_AF(actor, 0xd0);
	eye[1] = FH_AF(actor, 0xd4);
	eye[2] = FH_AF(actor, 0xd8);
	eye[3] = FH_AF(actor, 0xdc);
	if (FH_AI(self, 0x17c) == 0) {
		if (GM_STATE(self) != 5) {
			/* (the original flushes player 2's animations with the winner's node) */
			baFlushAnimations(PL_ACTOR(PL(1)) + 0x10, (struct BActorNode *)FH_AP(PL_ACTOR(winner), 0x144), 1, 0);
			Actor_PlayAnim(PL_ACTOR(PL(w)), 0x1b, 0, 0, 1.0f, 0.1f, 1);
			Actor_PlayAnim(PL_ACTOR(PL(w)), 0, 1, 1, 1.0f, 0.05f, 1);
		}
		FH_AI(self, 0x17c) = 1;
	}
	for (i = 0; i < 4; i++) {
		results_skip(self, DAT_002baed8[w * 10], PL(i));
		if (w != i)
			FH_AF(PL_ACTOR(PL(i)), 0xd8) = 0.0f;
	}
	if (DAT_002baed8[w * 10] == 1)
		results_skip(self, 0, PL(w));
	if (FH_AF(self, 0x178) <= 0.0f) {
		FH_AI(self, 0x17c) = 1;
		if (GM_STATE(self) != 5) {
			FH_AF(self, 0xf0) = 240.0f;
			FH_AF(self, 0x178) = 240.0f;
			FH_AI(self, 0xf4) = 1;
			FH_AI(self, 0x17c) = 0;
			FH_AI(self, 0xd8)--;
			FH_AI(self, 0xe0 + w * 4) = 0;
			bkPrintf("player %d Done moving to next\n", w);
		}
		if (FH_AI(self, 0xd8) < 1) {
			GM_STATE(self) = 5;
			bkPrintf("STATE = FINISHED <---------------------\n");
		}
	}
	if (DAT_002ecdec == 0)
		FH_AF(self, 0x178) = FH_AF(self, 0x178) - DAT_0028fda8;
	target[1] = target[1] + 150.0f;
	if (GM_VARIATION(self) < 3) {
		eye[2] = eye[2] + 200.0f;
		lift = 50.0f;
	} else {
		eye[0] = eye[0] - 100.0f;
		eye[2] = eye[2] - 200.0f;
		lift = 200.0f;
	}
	eye[1] = eye[1] + lift;
	bdSetCameraLookAt(eye, target, 0.0f);
	return 1;
}

/* 0x00129860  CGameMode_FoxHunt.cpp: draws the actors: the first pass of all, then the
faded shadows (flag 0x100000 off), then the second pass of the actors not lit by the
mode's light */
void CGameMode_FoxHunt_DrawActors(void *self)
{
	char *a;

	for (a = fh_ListNext((unsigned int *)DAT_002ea488, NULL); a != NULL; a = fh_ListNext((unsigned int *)DAT_002ea488, a))
		CGameMode_FoxHunt_DrawActor(self, a);
	for (a = fh_ListNext((unsigned int *)DAT_002ea488, NULL); a != NULL; a = fh_ListNext((unsigned int *)DAT_002ea488, a)) {
		if ((ACTOR_FLAGS(a) & 0x100000) == 0)
			Actor_DrawFaded(a);
	}
	for (a = fh_ListNext((unsigned int *)DAT_002ea488, NULL); a != NULL; a = fh_ListNext((unsigned int *)DAT_002ea488, a)) {
		if (ACTOR_LIGHTING(a) != 1)
			CGameMode_FoxHunt_DrawActorPass6(self, a);
	}
}

/* the two HUD icons of a player (overlay descriptor of classlib_ui, 0x3b0 bytes) */
static int make_icon(const char *texture)
{
	unsigned int desc[0xec];

	memset(desc, 0, sizeof desc);
	desc[0x94] = 0x42b40000u;   /* 90 */
	desc[0x95] = 0x42b40000u;
	desc[0x96] = 0x42b40000u;
	desc[0x97] = 0x437f0000u;   /* 255 */
	desc[0xa2] = 0;
	desc[0xad] = 0x41a00000u;   /* 20 */
	desc[0xae] = 0x41a80000u;   /* 21 */
	desc[0xaf] = 0;
	desc[0xc2] = 0x41c80000u;   /* 25 */
	desc[0xc3] = 0x41c80000u;
	desc[1] = FH_WORD(bLoadResource(NULL, texture, 0, 0xdefa));
	desc[0xa4] = 0x80;
	return COverlay_CreateChild(FH_AI(DAT_002ecfac, 0xf0), (undefined4 *)desc);   /* the HUD's overlay root */
}

/* 0x00129910  CGameMode_FoxHunt.cpp: makes each player's two HUD icons (texturesl\icon.bmp,
texturesl\icon2.bmp) */
void FoxHunt_CreateIcons(void)
{
	int i;

	for (i = 0; i < DAT_002bae94; i++) {
		FH_I(0x00302164 + i * 8) = make_icon("texturesl\\icon.bmp");
		FH_I(0x00302168 + i * 8) = make_icon("texturesl\\icon2.bmp");
	}
}

/* frees an object whose destructor is empty (0x001d5950) */
static void delete_plain(void *object)
{
	FUN_001d5950();
	bkHeapFree(object);
}

/* 0x00129ab0  CGameMode_FoxHunt.cpp: exit: frees the route tables, restores the near/far
planes and, when the game was loaded (+0x04 == 1), frees everything the round made; then
closes the two packages */
void CGameMode_FoxHunt_Exit(void *self)
{
	char *p;
	char **slot;
	char *object;
	DSCAPS caps;
	int i;

	bkPrintf("POPPED\n");
	if (fh_RouteTables != NULL) {
		int *block = (int *)fh_RouteTables - 1;

		/* delete[] (eh vector destructor iterator; destructor 0x000ad880) */
		for (i = *block - 1; i >= 0; i--)
			CLRouteHeights_Free2((struct CLRouteHeights *)(fh_RouteTables + i * 0x18));
		bkHeapFree(block);
	}
	object = FH_P(0x00302198);
	DAT_0028ffa8 = FH_AF(self, 0x1e4);
	DAT_0028ffac = FH_AF(self, 0x1e8);
	if (FH_AI(self, 0x4) == 1) {
		if (FH_P(0x00302198) != NULL) {
			CCamera_Dtor(FH_P(0x00302198));
			bkHeapFree(object);
		}
		for (i = 0; i < DAT_002bae94; i++) {
			if (FH_AP(PL(i), 0x5c) != NULL) {
				biDeleteMap((struct biMap *)FH_AP(PL(i), 0x5c));
				p = PL(i);
				if (p != NULL) {
					FUN_0012e9a0(p);
					bkHeapFree(p);
				}
			}
			DAT_002f289c[i] = NULL;
		}
		for (slot = (char **)FH_ADDR(0x003022d4); slot < (char **)FH_ADDR(0x003022f4); slot++) {
			if (*slot != NULL)
				delete_plain(*slot);
		}
		if (GM_VARIATION(self) > 2) {
			if (FH_P(0x003022fc) != NULL)
				delete_plain(FH_P(0x003022fc));
			FUN_00138040();
		}
		if (GM_VARIATION(self) < 3) {
			FUN_00137c60();
			object = FH_AP(self, 0x27c);
			if (object != NULL) {
				FUN_00133620(object);
				bkHeapFree(object);
			}
		}
		if (GM_VARIATION(self) == 2)
			FUN_00138620();
		if (GM_STATE(self) == 0)
			CLIntroCam_Destroy();
		object = (char *)DAT_002f26c4;
		if (DAT_002f26c4 != NULL) {
			BCarFreeCamera_Destruct((struct BCarFreeCamera *)DAT_002f26c4);
			bkHeapFree(object);
		}
		if (WATER() != NULL)
			FH_VFN(WATER(), 0x0, void *(*)(void *, int))(WATER(), 1);
		Pickups_DeleteAll();
		RespawnPoints_DeleteAll();
		Platforms_DeleteAll();
		Actor_DeleteAll();
		FUN_001363f0();
		FUN_00136de0();
		RefBot_Delete();
		FUN_00133f90(DAT_003022cc);
		bsReserveChannel(2, 0);
		bsReserveChannel(3, 0);
		bsStopAllThunk();
		if (fh_Speech != NULL)
			delete_plain(fh_Speech);
		baDeleteLight((struct BLight *)GM_LIGHT2(self));
		baDeleteLight((struct BLight *)GM_LIGHT(self));
		Lights_DeleteAll();
		Shadows_DeleteAll();
		if (FH_P(0x00302140) != NULL) {
			object = FH_P(0x00302140);
			CTweakMenu_Dtor((struct CTweakMenu *)object);
			bkHeapFree(object);
		}
		if (FH_P(0x00302148) != NULL) {
			object = FH_P(0x00302148);
			CTweakMenu_Dtor((struct CTweakMenu *)object);
			bkHeapFree(object);
		}
		if (FH_P(0x00302184) != NULL) {
			object = FH_P(0x00302184);
			CTweakMenu_Dtor((struct CTweakMenu *)object);
			bkHeapFree(object);
		}
		CLRumble_Shutdown((int *)DAT_002ed324);
		COverlay_DeleteAll();
		CProjector_DeleteAll();
		if (FH_P(0x0030215c) != NULL) {
			object = FH_P(0x0030215c);
			CLRouteHeights_Dtor((struct CLRouteHeights *)object);
			bkHeapFree(object);
		}
		if (FH_P(0x00302188) != NULL) {
			object = FH_P(0x00302188);
			CLRouteHeights_Dtor((struct CLRouteHeights *)object);
			bkHeapFree(object);
		}
		if (FH_P(0x00302158) != NULL) {
			object = FH_P(0x00302158);
			CLRouteHeights_Dtor((struct CLRouteHeights *)object);
			bkHeapFree(object);
		}
		if (FH_P(0x00302154) != NULL) {
			object = FH_P(0x00302154);
			CLRouteHeights_Dtor((struct CLRouteHeights *)object);
			bkHeapFree(object);
		}
		if (fh_LevelPackage != NULL)
			bkDeletePackageResources(bkGetPackageId((struct bkPackage *)fh_LevelPackage), 2);
		if (fh_VariationPackage != NULL)
			bkDeletePackageResources(bkGetPackageId((struct bkPackage *)fh_VariationPackage), 2);
	}
	if (fh_LevelPackage != NULL) {
		bkGetPackageId((struct bkPackage *)fh_LevelPackage);
		bkClosePackageThunk((struct bkPackage *)fh_LevelPackage);
		fh_LevelPackage = NULL;
	}
	if (fh_VariationPackage != NULL) {
		bkGetPackageId((struct bkPackage *)fh_VariationPackage);
		bkClosePackageThunk((struct bkPackage *)fh_VariationPackage);
		fh_VariationPackage = NULL;
	}
	IDirectSound_GetCaps((struct IDirectSound *)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", caps.dwFree3DBuffers);
	bkPrintf("this house is clean...\n");
	FUN_001d5950();   /* called with (0xffff, 0): an empty function (a compiled-out check) */
}

/* 0x00129e50  CGameMode_FoxHunt.cpp: process (top: this mode is on top of the stack): runs
the rules and the state machine unless paused, the camera of the countdown and play, the
places for the result table, the pause menu, and pops the mode when the round is over */
void CGameMode_FoxHunt_Process(void *self, int top)
{
	char *cam;
	int i;

	if (top == 0)
		return;
	if (DAT_002ecdec == 0) {
		CGameMode_FoxHunt_Update(self);
		CGameMode_FoxHunt_UpdateState(self);
	} else if (GM_STATE(self) == 0) {
		CLIntroCam_Update(0.0f, 0);
	}
	FoxHunt_UpdateColourSet();
	if (FH_AI(self, 0x1b0) == 0) {
		if (GM_STATE(self) == 4 || GM_STATE(self) == 5) {
			CGameMode_FoxHunt_Results(self);
		} else {
			if (GM_STATE(self) == 2) {
				if (FH_AF(self, 0x160) < 1.0f && DAT_002baeb8 == 0) {
					Core_Vec4Copy((float *)(FH_P(0x00302198) + 0x534), FH_AV(self, 0x254));
					Core_Vec4Copy((float *)(FH_P(0x00302198) + 0x544), FH_AV(self, 0x264));
				}
			} else if (GM_STATE(self) != 1 || FH_P(0x00302198) == NULL) {
				goto camera_done;
			}
			if (GM_VARIATION(self) > -1)
				Core_Vec4Copy((float *)(FH_P(0x00302198) + 0x4b0), FH_AV(self, 0xfc));
			FH_AI(FH_P(0x00302198), 0x308) = FH_AI(self, 0x15c);
			for (i = 0; i < 4; i++) {
				char *actor = PL_ACTOR(PL(i));

				FH_AI(self, 0x38 + i * 0x10) = FH_AI(actor, 0xd0);
				FH_AI(self, 0x3c + i * 0x10) = FH_AI(actor, 0xd4);
				FH_AI(self, 0x40 + i * 0x10) = FH_AI(actor, 0xd8);
				FH_AI(self, 0x44 + i * 0x10) = FH_AI(actor, 0xdc);
				FH_AF(self, 0x3c + i * 0x10) = FH_AF(self, 0x3c + i * 0x10) * 0.666666687f;
			}
			CCamera_Update(FH_P(0x00302198));
			if (DAT_002baeb8 != 1 && FH_AF(self, 0x160) < 1.0f && DAT_002ecdec == 0) {
				FH_AF(self, 0x160) = FH_AF(self, 0x160) + 0.02f;
				FH_AS(PL(0), 0x10b8) = 0;
				FH_AS(PL(1), 0x10b8) = 0;
				FH_AS(PL(2), 0x10b8) = 0;
				FH_AS(PL(3), 0x10b8) = 0;
			}
		}
	}
camera_done:
	if (GM_STATE(self) > 3)
		bdUpdateView();
	for (i = 0; i < DAT_002bae94; i++)
		DAT_002baed4[i * 10 + 5] = 5 - FH_AI(PL(i), 0x11d8);
	FH_AI(self, 0x274) = FH_AI(self, 0x278);
	if (GM_STATE(self) == 5 || GM_STATE(self) == 6) {
		if (DAT_002ecdec == 0)
			goto finished;
	} else {
		FH_AU(self, 0x278) = DAT_002ecdec != 0;
		if (FH_AI(self, 0xf8) != 0)
			goto finished;
	}
	if (CLPause_Update((undefined4 *)DAT_002ecd90) != 0)
		return;
finished:
	if (GM_STATE(self) == 6) {
		CLHud_DeleteWinnerText((int)(uintptr_t)DAT_002ecfac);
		if (DAT_002bae98 != 2)
			CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, top);
	}
}

/* 0x0012a190  CGameMode_FoxHunt.cpp: draw (top: this mode is on top): the view and fog of
the variation, the level sets, the actors, the AI debug, the sea, the goals, the shadows,
the effects, the HUD, the fades of the results, and the camera for the next frame */
void CGameMode_FoxHunt_Draw(void *self, int top)
{
	char *p;
	int i, fade;
	float pos[4];
	float heading;

	if (top == 0)
		return;
	bdSetRenderState(4, 0, 0);
	bdSetWorldIdentity();
	bdSetProjectionMode(1);
	bdSetNearFar(40.0f, 50000.0f);
	bdSetFieldOfView(fh_bitsf(0x3f9c61aau), fh_bitsf(0x3f6a927fu));   /* 1.2217 (70 degrees), 0.9163 */
	if (GM_VARIATION(self) < 3) {
		bdSetFogColour(0x50, 0x50, 0x6e);
		bdSetFogRange(1000.0f, 30000.0f);
	} else {
		if (GM_VARIATION(self) == 3)
			bdSetFogColour(0, 0, 0xf);
		else
			bdSetFogColour(10, 0x1e, 0x2d);
		bdSetFogRange(3000.0f, 18000.0f);
	}
	bdSetRenderState(8, FH_AI(self, 0x1e0) == 1, 0);
	bdSetRenderState(3, 1, 0);
	if (GM_VARIATION(self) < 3 && FH_AI(self, 0x1d8) == 1)
		baDrawSet((struct BSet *)FH_AP(self, 0x78), 2);
	Actor_PrepAll();
	CGameMode_FoxHunt_DrawActors(self);
	bdSetRenderState(3, 0, 0);
	for (i = 0; i < DAT_002bae94; i++) {
		if (DAT_002baed8[i * 10] == 0 && FH_AI(PL(i), 0x11ac) == 1 && FH_AI(self, 0x1ac) == 1)
			FUN_00131530(PL(i));
	}
	bdSetWorldIdentity();
	if (GM_VARIATION(self) < 3 && FH_AI(self, 0x1d8) == 1)
		baDrawSet((struct BSet *)FH_AP(self, 0x78), 1);
	if (GM_VARIATION(self) > 2) {
		if (FH_AI(self, 0x1d8) == 1)
			baDrawSet((struct BSet *)FH_AP(self, 0x78), 1);
		if (GM_VARIATION(self) > 2) {
			bdSetRenderState(6, 1, 0);
			bdSetRenderState(10, 1, 0);
			baDrawSet((struct BSet *)FH_AP(self, 0x7c), 1);
			baDrawSet((struct BSet *)FH_AP(self, 0x7c), 4);
			bdSetRenderState(6, 0, 0);
			bdSetRenderState(10, 0, 0);
		}
	}
	bdSetRenderState(8, 0, 0);
	for (i = 0; i < 4; i++) {
		if (FH_AI(PL(i), 0x11ac) == 1)
			CLHud_DrawAIRing(i, FH_AV(PL_ACTOR(PL(i)), 0xd0), NULL);
	}
	if (GM_VARIATION(self) < 5 && FH_AI(self, 0x1a8) == 1) {
		/* the sea */
		bdSetRenderState(8, FH_AI(self, 0x1e0) == 1, 0);
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(7, 3, 0);
		bdSetRenderState(10, GM_VARIATION(self) > 2, 0);
		bdSetRenderState(3, 1, 0);
		bdSetWorldIdentity();
		if (FH_VFN(WATER(), 0x14, int (*)(void *))(WATER()) != 0)
			FH_VFN(WATER(), 0x1c, int (*)(void *, int))(WATER(), 0);
		if (GM_VARIATION(self) < 3)
			FH_VFN(WATER(), 0x20, int (*)(void *, float, int))(WATER(), 6000.0f, 0);
		bdSetRenderState(6, 0, 0);
		bdSetRenderState(10, 0, 0);
	}
	bdSetRenderState(8, 0, 0);
	if (FH_AI(self, 0x1b4) != 0)
		CLRoute_DrawDebug();
	if (GM_VARIATION(self) < 3 && FH_AI(self, 0x1d4) == 1) {
		CFHunt_GenericGoal_Draw(FH_AP(self, 0x27c), 0);
		CFHunt_GenericGoal_Draw(FH_AP(self, 0x27c), 1);
	}
	if (FH_AI(self, 0x1d0) == 1) {
		Shadows_DrawFor((char *)self + 0x28);
		bdSetRenderState(4, 0, 0);
	}
	if (FH_AI(self, 0x1d4) == 1)
		FoxHunt_Effects_Draw(DAT_003022cc);
	CRayControl_DrawAll();
	bdSetRenderState(3, 0, 0);
	if (FH_AI(self, 0x1cc) == 1)
		CGameMode_FoxHunt_DrawFrameRate(self);
	if (FH_AI(self, 0x1a4) == 1) {
		COverlay_DrawAll(1);
		CLHud_DrawScoreBars((int)(uintptr_t)DAT_002ecfac, (float)FH_AI(PL(0), 0x11ac), (float)FH_AI(PL(1), 0x11ac),
			(float)FH_AI(PL(2), 0x11ac), (float)FH_AI(PL(3), 0x11ac), 1.1f);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	/* the fades of the results */
	if (FH_AI(self, 0xf4) == 1 && GM_STATE(self) == 4) {
		if (CLFade_Draw(DAT_0028fda8) == 1)
			FH_AI(self, 0xf4) = 0;
	} else if (GM_STATE(self) == 5) {
		int go;

		if (FH_AI(self, 0x198) == 1 && DAT_002bae98 == 1) {
			if (DAT_002ecdec != 0)
				goto fade_in;
			if (CLSamples_CommentaryIdle() != 0) {
				go = 1;
			} else {
				if (DAT_002ecdec != 0)
					goto fade_in;
				go = FH_AI(self, 0x1a0) == 1;
			}
		} else {
			go = DAT_002ecdec == 0;
		}
		if (go) {
			fade = FH_AI(self, 0xf8) + FH_FTOL(DAT_0028fda8 * 8.5f);
			FH_AI(self, 0xf8) = fade;
			if (fade < 0xff) {
				CLScreen_DrawDarken(fade, 1);
			} else {
				FH_AI(self, 0xf8) = 0xff;
				CLScreen_DrawDarken(0xff, 1);
				GM_STATE(self) = 6;
			}
		}
	}
fade_in:
	if (GM_STATE(self) == 0 && DAT_002ecdec == 0) {
		fade = FH_AI(self, 0xf8) + FH_FTOL(DAT_0028fda8 * -8.5f);
		FH_AI(self, 0xf8) = fade;
		if (fade < 1)
			FH_AI(self, 0xf8) = 0;
		else
			CLScreen_DrawDarken(fade, 1);
	}
	/* the camera */
	if (GM_STATE(self) == 1 || GM_STATE(self) == 2) {
		switch (FH_AI(self, 0x1b0)) {
		case 0:
			bdSetCameraLookAt((float *)(FH_P(0x00302198) + 0x534), (float *)(FH_P(0x00302198) + 0x544), 0.0f);
			bdUpdateView();
			return;
		case 1:
			FoxHunt_CameraAtPlayer1();
			bdUpdateView();
			return;
		case 2:
			p = FH_AP(self, 0x1c8);
			FUN_00133490(p, pos, &heading);
			pos[1] = pos[1] + 15.0f;
			bdSetCameraPosition(pos[0], pos[1], pos[2]);
			bdSetCameraAngles(0.0f, heading, 0.0f, 1);
			bdUpdateView();
			return;
		default:
			if (FH_AI(self, 0x1b0) == 4 && DAT_002f26c4 != NULL)
				BCarFreeCamera_Update((struct BCarFreeCamera *)DAT_002f26c4, DAT_0028fda8);
			bdUpdateView();
			return;
		}
	}
}

/* ======================================================================== level set-up */

#include "drillers_data.h"

/* reads the next line of a level script (the reader is a local struct BCarTextReader) */
static char *read_line(void *reader, char *buffer)
{
	char *line = FoxHunt_TextReader_ReadLine(reader, buffer);

	FoxHunt_TextReader_NextLine(reader);
	return line;
}

/* opens the level script `name` of `package` ("cfg" name in the reader for messages) */
static void open_script(void *reader, void *package, const char *name, const char *title)
{
	unsigned int size;
	char *data;

	data = (char *)bkLoadFile((struct bkPackage *)package, name, NULL, &size, NULL, 0);
	BCarTextReader_Init((struct BCarTextReader *)reader, data, (int)size, title);
}

/* a route height table (CLRouteHeights, 8 bytes) filled from the height map file `name` */
static char *load_heights(void *package, const char *name, int line)
{
	char *heights = (char *)FH_NEW(8, fh_GameModeFile, line);
	unsigned int size;
	char *data;

	if (heights != NULL)
		heights = (char *)CLRouteHeights_Ctor((struct CLRouteHeights *)heights);
	data = (char *)bkLoadFile((struct bkPackage *)package, name, NULL, &size, NULL, 0);
	memcpy(FH_AP(heights, 0), data, size);
	return heights;
}

/* the players' models: pairs of (name, model) at 0x002ab268-0x002ab2d0, the first name
word is the last word of the Drillers data block */
static char **model_slot(unsigned int a)
{
	return a < 0x002ab26cu ? (char **)DR_ADDR(a) : (char **)FH_ADDR(a);
}

static void load_player_models(void)
{
	unsigned int a;

	for (a = 0x002ab270u; a < 0x002ab2d0u; a += 0x10) {
		*model_slot(a - 4) = (char *)bLoadResource(NULL, *model_slot(a - 8), 1, 0);
		*model_slot(a + 4) = (char *)bLoadResource(NULL, *model_slot(a), 1, 0);
		if (*model_slot(a - 4) == NULL)
			bkPrintf("EEEeeeeeeeeekkkkk!!!!!\n");
	}
}

/* the CWaterField of the level: `params` are the 16 words of its CWaterParams */
static void make_water(const unsigned int *params, int line_new, int line_params)
{
	char *water;
	unsigned int *p;
	int i;

	water = (char *)FH_NEW(0x9090, fh_GameModeFile, line_new);
	if (water == NULL)
		FH_P(0x0030214c) = NULL;
	else
		FH_P(0x0030214c) = (char *)FoxHunt_CWaterField_Ctor(water);
	p = (unsigned int *)FH_NEW(0x40, fh_GameModeFile, line_params);
	for (i = 0; i < 16; i++)
		p[i] = params[i];
	p[0xe] = FH_WORD("textures\\waterref.bmp");
	FH_VFN(WATER(), 0x4, int (*)(void *, void *))(WATER(), p);
	bkHeapFree(p);
}

/* the HUD of the round: score boxes and texts, the clock, the bars, the player icons */
static void make_hud(void *self)
{
	CLHud_CreateScoreBoxes((int)(uintptr_t)DAT_002ecfac);
	CLHud_CreateScoreTexts((int)(uintptr_t)DAT_002ecfac, (int *)((char *)self + 0xc4));
	CLHud_CreateClock((int)(uintptr_t)DAT_002ecfac, FH_AF(self, 0x9c) * (1.0f / 60.0f));
	CLHud_CreateBars((int)(uintptr_t)DAT_002ecfac);
	FoxHunt_CreateIcons();
}

/* the intro camera path from the level's .lom file */
static void load_intro(void *self, void *package, const char *name)
{
	unsigned int size;
	char *data;
	int *map;

	data = (char *)bkLoadFile((struct bkPackage *)package, name, NULL, &size, NULL, 0);
	map = ParseMap_Load(data, 2, (int)size);
	if (map == NULL)
		bkPrintf("level instance failed \n");
	CLIntroCam_Load((int)(uintptr_t)map, (undefined4)(uintptr_t)self);
	ParseMap_Free(map);
}

/* reserves sound channels 2 and 3 for the looping sounds `a` and `b` (FHunt_Sound) */
static void start_loops(int a, int b)
{
	bsStopChannel(2);
	bsReserveChannel(2, 1);
	bsPlaySampleOnChannel(2, (struct BSample *)FH_AP(fh_Speech, 0x54 + a * 4), 0xff, 0, 0, 0);
	bsStopChannel(3);
	bsReserveChannel(3, 1);
	bsPlaySampleOnChannel(3, (struct BSample *)FH_AP(fh_Speech, 0x54 + b * 4), 0xff, 0, 0, 0);
}

/* the intro or (without RefBot intro) straight to play */
static void start_state(void *self)
{
	if (DAT_002baeb8 == 0) {
		GM_STATE(self) = 0;
	} else {
		GM_STATE(self) = 2;
		FH_AI(self, 0xf8) = 0;
		CLIntroCam_Destroy();
	}
}

/* places player `rec`'s actor */
static void place_player(int rec, float x, float y, float z, float w)
{
	char *actor = PL_ACTOR(PL(rec));

	FH_AF(actor, 0xd0) = x;
	FH_AF(actor, 0xd4) = y;
	FH_AF(actor, 0xd8) = z;
	FH_AF(actor, 0xdc) = w;
}

/* 0x0012a7c0  CGameMode_FoxHunt.cpp: sets up variation 1 (Cooperation, the town): reads
foxhunt.cfg (respawn points, game time, camera, goals, chairs), loads the town, its four
route height maps, two flags, the players, two glass panes, the harbour water, the HUD, the
intro, the rain and harbour sounds, the lightning sky, the boat and a shark */
void CGameMode_FoxHunt_InitCooperation(void *self, void *package)
{
	unsigned char reader[0x214] __attribute__((aligned(4)));
	char buffer[0x200];
	char mapfile[0x200];
	char *line;
	char *actor;
	char *object;
	float pos[4];
	float gpos[4];
	float size;
	int count, i, n, type, ints[3];

	FoxHunt_TextReader_Ctor(reader);
	open_script(reader, package, "foxhunt.cfg", "FoxHunt.cfg");
	while (FH_AP(reader, 0x8) < FH_AP(reader, 0xc)) {
		line = read_line(reader, buffer);
		if (strncmp(line, "//", 2) == 0)
			continue;
		if (_strnicmp(line, "MAPFILE ", 8) == 0)
			FoxHunt_ReadQuoted(line, mapfile, 0x1ff);
		if (_strnicmp(line, "NOOFRESPAWNPOINTS ", 0x12) == 0 && sscanf(line, "%*s%d", &count) == 1) {
			pos[0] = 0.0f;
			pos[1] = 0.0f;
			pos[2] = 0.0f;
			pos[3] = 0.0f;
			for (i = 0; i < count; i++) {
				line = read_line(reader, buffer);
				if (sscanf(line, "%*s%f%f%f", &pos[0], &pos[1], &pos[2]) == 3) {
					object = (char *)FH_NEW(0x20, fh_GameModeFile, 0x9d9);
					if (object != NULL)
						RespawnPoint_Init(object, pos, 0);
				}
			}
		}
		if (_strnicmp(line, "GAMETIME ", 9) == 0 && sscanf(line, "%*s%f", FH_AV(self, 0xac)) == 1) {
			FH_AF(self, 0xac) = FH_AF(self, 0xac) * 60.0f;
			FH_AF(self, 0x9c) = FH_AF(self, 0xac);
		}
		if (_strnicmp(line, "CAMERASETUP ", 0xc) == 0)
			sscanf(line, "%*s%f%f%f", FH_AV(self, 0xfc), FH_AV(self, 0x100), FH_AV(self, 0x104));
		if (_strnicmp(line, "CAMERADISTANCE ", 0xf) == 0)
			sscanf(line, "%*s%f", FH_AV(self, 0x15c));
		if (_strnicmp(line, "NOOFGOALS ", 10) == 0) {
			bkPrintf("Found No of Goals\n");
			if (sscanf(line, "%*s%d", &n) == 1) {
				for (i = 0; i < n; i++) {
					float ignored[3];
					int id, a, b;

					line = read_line(reader, buffer);
					sscanf(line, "%*s%f%f%f", &ignored[0], &ignored[1], &ignored[2]);
					line = read_line(reader, buffer);
					sscanf(line, "%*s%f%f%f", &gpos[0], &gpos[1], &gpos[2]);
					line = read_line(reader, buffer);
					sscanf(line, "%*s%d%d%d", &id, &a, &b);
					/* the positions in the script are replaced by fixed ones (w is a stack
					word the original never sets; 1 here) */
					gpos[3] = 1.0f;
					if (i == 0) {
						gpos[0] = -700.0f;
						gpos[2] = -700.0f;
						gpos[1] = 250.0f;
						FH_P(0x00302194) = CFHunt_GenericGoal_AddGoal(FH_AP(self, 0x27c), gpos, 0.7f, 1);
						CFHunt_GenericGoal_SetActive(FH_AP(self, 0x27c), GOAL_A(), 1);
						CFHunt_GenericGoal_SetColour(FH_AP(self, 0x27c), GOAL_A(), 0x7f7f7f);
					} else if (i == 1) {
						gpos[0] = 700.0f;
						gpos[2] = 700.0f;
						gpos[1] = 250.0f;
						FH_P(0x0030219c) = CFHunt_GenericGoal_AddGoal(FH_AP(self, 0x27c), gpos, 0.7f, 2);
						CFHunt_GenericGoal_SetActive(FH_AP(self, 0x27c), GOAL_B(), 1);
						CFHunt_GenericGoal_SetColour(FH_AP(self, 0x27c), GOAL_B(), 0x7f7f7f);
					}
				}
			}
		}
		if (_strnicmp(line, "NOOFCHAIRS ", 0xb) == 0) {
			pos[0] = 0.0f;
			pos[1] = 0.0f;
			pos[2] = 0.0f;
			pos[3] = 0.0f;
			if (sscanf(line, "%*s%d", &n) == 1) {
				FoxHunt_LoadPropActors();
				for (i = 0; i < n; i++) {
					char *l;

					l = read_line(reader, buffer);
					sscanf(l, "%*s%f%f%f", &pos[0], &pos[1], &pos[2]);
					l = read_line(reader, buffer);
					sscanf(l, "%*s%d", &type);
					l = read_line(reader, buffer);
					sscanf(l, "%*s%f", &size);
					object = (char *)FH_NEW(0x50, fh_GameModeFile, 0xa67);
					if (object != NULL)
						object = (char *)CFHunt_Throwable_Ctor(object, type, pos, size);
					FH_PV(0x003022d4)[i] = object;
				}
			}
		}
	}
	(void)ints;
	pos[0] = 0.0f;
	pos[1] = 0.0f;
	pos[2] = 0.0f;
	pos[3] = 0.0f;
	actor = (char *)Actor_LoadAny("obe\\tag1a4.obe", pos, 0, NULL, 0);
	ACTOR_FLAGS(actor) |= 0x410000;
	FH_AP(self, 0x78) = (char *)bLoadResource(NULL, "obe\\tag1_arenabg5.obe", 6, 0xdefa);
	if (FH_AP(self, 0x78) == NULL)
		bkPrintf("No set loaded \n");
	FH_P(0x0030215c) = load_heights(package, "tag1a.hmp", 0xa86);
	FH_P(0x00302188) = load_heights(package, "tag1b.hmp", 0xa89);
	FH_P(0x00302158) = load_heights(package, "tag1c.hmp", 0xa8c);
	FH_P(0x00302154) = load_heights(package, "tag1d.hmp", 0xa8f);
	if (FH_P(0x0030215c) == NULL)
		bkPrintf("NO HEIGHT MAP ---------------------------------------\n");
	CLRoute_SetHeights((struct CLRouteHeights *)FH_P(0x0030215c));
	/* two flags at random places */
	pos[0] = (float)(fh_rand() % 0x708 - 900);
	pos[2] = (float)(fh_rand() % 0x708 - 900);
	pos[1] = 3000.0f;
	pos[3] = 0.0f;
	FUN_00136440();
	object = (char *)FH_NEW(0x54, fh_GameModeFile, 0xaa6);
	if (object != NULL)
		CFHunt_Flag_Ctor(object, pos, 1, 1);
	pos[0] = (float)(fh_rand() % 0x708 - 900);
	pos[2] = (float)(fh_rand() % 0x708 - 900);
	object = (char *)FH_NEW(0x54, fh_GameModeFile, 0xaab);
	if (object != NULL)
		CFHunt_Flag_Ctor(object, pos, 2, 2);
	pos[1] = 2000.0f;
	load_player_models();
	for (i = 0; i < DAT_002bae94; i++) {
		pos[0] = (float)(fh_rand() % 0xaf0 - 0x578);
		pos[2] = (float)(fh_rand() % 0xaf0 - 0x578);
		object = (char *)FH_NEW(0x11fc, fh_GameModeFile, 0xac0);
		if (object != NULL)
			object = (char *)FoxHunt_Player_Ctor(object, i, pos);
		PL(i) = object;
		DAT_002f289c[i] = object;
	}
	place_player(DAT_002baec0[0], 40.0f, 10.0f, -913.0f, pos[3]);
	place_player(DAT_002baec0[1], 200.0f, 10.0f, -720.0f, pos[3]);
	place_player(DAT_002baec0[2], 796.0f, 10.0f, -235.0f, pos[3]);
	place_player(DAT_002baec0[3], 1029.0f, 10.0f, -173.0f, pos[3]);
	/* two glass panes (glassaa.obe, glassbb.obe) */
	pos[0] = -940.0f;
	pos[1] = 820.0f;
	pos[2] = -515.0f;
	object = (char *)FH_NEW(0x18, fh_GameModeFile, 0xadf);
	if (object != NULL)
		FoxHunt_Glass_Ctor(object, pos, 1);
	pos[0] = 265.0f;
	pos[1] = 790.0f;
	pos[2] = 941.0f;
	object = (char *)FH_NEW(0x18, fh_GameModeFile, 0xae4);
	if (object != NULL)
		FoxHunt_Glass_Ctor(object, pos, 2);
	{
		static const unsigned int water[16] = {
			0xc4fa0000u, 0xc3960000u, 0xc55ac000u,   /* corner (-2000, -300, -3500) */
			0x45cb2000u, 0x45cb2000u,                /* size 6500 x 6500 */
			0x5a, 0x5a,                              /* 90 x 90 cells */
			0,                                       /* no wrap */
			0x3f733333u, 0x3e4ccccdu,                /* damping 0.95, tension 0.2 */
			0x40800000u, 0x40800000u, 0x3dcccccdu,   /* height 4, normal 4, slope uv 0.1 */
			0x4f016448u,                             /* colour */
			0,                                       /* texture name (set by make_water) */
			0x3f800000u,                             /* uv scale 1 */
		};

		make_water(water, 0xae8, 0xaeb);
	}
	FH_AI(PL(0), 0x11d0) = 0;
	FH_AI(PL(1), 0x11d0) = 0;
	FH_AI(PL(2), 0x11d0) = 0;
	FH_AI(PL(3), 0x11d0) = 0;
	make_hud(self);
	FH_AI(self, 0xbc) = -1;
	FH_AI(self, 0xb8) = -1;
	FH_AI(self, 0xb4) = -1;
	FH_AI(self, 0xb0) = -1;
	load_intro(self, package, "tag3.lom");
	start_loops(0, 14);
	/* the lightning sky and the boat */
	pos[0] = 0.0f;
	pos[1] = -20000.0f;
	pos[2] = 0.0f;
	*(char **)FH_ADDR(0x002ab2cc) = (char *)bLoadResource(NULL, FH_P(0x002ab2c8), 1, 0);
	FH_AP(self, 0x80) = (char *)Actor_Create(FH_P(0x002ab2cc), pos, NULL, 4, 0, 1);
	FH_AP(self, 0x84) = NULL;
	pos[0] = 3395.0f;
	pos[1] = -330.0f;
	pos[2] = -2220.0f;
	*(char **)FH_ADDR(0x002ab2d4) = (char *)bLoadResource(NULL, FH_P(0x002ab2d0), 1, 0);
	FH_AP(self, 0x84) = (char *)Actor_Create(FH_P(0x002ab2d4), pos, NULL, 4, 0, 1);
	start_state(self);
	/* a shark */
	pos[0] = 1800.0f;
	pos[1] = -390.0f;
	pos[2] = 5200.0f;
	FUN_00135f70();
	object = (char *)FH_NEW(0x60, fh_GameModeFile, 0xb6d);
	if (object != NULL)
		FoxHunt_Shark_Ctor(object, pos, 1);
	FoxHunt_TextReader_Dtor(reader);
}
