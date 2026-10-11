/*
Clusteroids/CGameMode_Clusteroids.c  <-  C:\BlitzParty\Game\Src\Clusteroids\CGameMode_Clusteroids.cpp

CGameMode_Clusteroids: the game mode of the minigame (vtable 0x00268ddc, 0x2c bytes).
Pushed opens the packages, LoadingStarted (+0x2c) loads the resources and sets up the
arena, Start (+0x30) builds the HUD and the tweak menus, Update (+0x18) runs a frame,
Render (+0x1c) draws it, Popped frees everything.

Fields: +0x14 loadState (CGameMode), +0x28 the game was set up.
0x000d9520 and 0x000d9540 (gap) are the constructor and destructor (they store this
file's vtable).
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

/* 0x00268ddc  vtable of CGameMode_Clusteroids (rebuilt from the XBE) */
const void *const vtable_00268ddc[13] = {
	(const void *)CGameModeClusteroids_DeletingDtor, /* +0x00 */
	(const void *)CGameModeClusteroids_Suspend,      /* +0x04 */
	(const void *)CGameModeClusteroids_Resume,       /* +0x08 */
	(const void *)CGameModeClusteroids_Popped,       /* +0x0c */
	(const void *)CGameModeClusteroids_Pushed,       /* +0x10 */
	(const void *)CGameModeClusteroids_Input,        /* +0x14 */
	(const void *)CGameModeClusteroids_Update,       /* +0x18 */
	(const void *)CGameModeClusteroids_Render,       /* +0x1c */
	(const void *)FUN_001ec260,                      /* +0x20 */
	(const void *)CGameMode_UpdateLoading,           /* +0x24 */
	(const void *)CGameMode_FinishedLoading,         /* +0x28 */
	(const void *)CGameModeClusteroids_LoadingStarted, /* +0x2c */
	(const void *)CGameModeClusteroids_Start,        /* +0x30 */
};

/* classlib_fx: the sprite definitions "aisteam1" (6) at 0x00290f00 */
extern unsigned char s_aisteam1_00290f00[];
/* core: the list that 0x00072720 resets (static object 0x002f7a5c, core crt) */
extern unsigned char DAT_002f7a5c[];
/* classlib_ui: pause menu update; nonzero while the pause menu runs (thiscall) */
#define cl_PauseMenuUpdate ((int (*)(void *))FUN_000772e0)
/* classlib_ui: HUD helpers still named FUN_ by their owner (thiscall, HUD in ECX) */
#define cl_HudReset ((void (*)(int))FUN_0007bfd0)
#define cl_HudUpdateBars ((void (*)(int))FUN_0007bd80)
/* classlib_ui: CLLoading_AddPackage returns the package */
#define cl_AddPackage ((int (*)(const char *, int))FUN_00071f90)

/* 0x000d9520  Clusteroids/CGameMode_Clusteroids.cpp (inferred: stores this file's vtable) */
void *CGameModeClusteroids_Ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, "Clusteroids");
	*(const void *const **)self = vtable_00268ddc;
	CL_I32(self, 0x28) = 0;
	return self;
}

/* 0x000d9540  Clusteroids/CGameMode_Clusteroids.cpp (inferred) */
void CGameModeClusteroids_Dtor(void *self)
{
	*(const void *const **)self = vtable_00268ddc;
	CL_I32(self, 0x28) = 0;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x000d9560  Clusteroids/CGameMode_Clusteroids.cpp
Pushed: loads the character voices and starts loading "Packages\Cluster" (and, for the
Collector variant, "packages\clust-v4"). */
void CGameModeClusteroids_Pushed(void *self)
{
	char name[64];

	Core_LoadCharacterVoices(1);
	CL_I32(self, 0x14) = 1;
	bkPrintf("CGameMode_Clusteroids::Pushed()\n");
	g_ClPackage = cl_AddPackage("Packages\\Cluster", 0);
	if (g_ClPackage == 0)
		bkPrintf("CGameMode_Clusteroids::GainFocus *** Could not load package 'Packages\\Clusteroids' ***\n");
	else
		bkPrintf("opened Clusteroids package\n");
	if (g_ClVariant == 4) {
		_snprintf(name, 0x3f, "packages\\clust-v%01d", 4);
		g_ClVariantPackage = cl_AddPackage(name, 0);
		if (g_ClVariantPackage == 0)
			bkPrintf("CGameMode_Clusteroids::GainFocus *** Could not load package 'Packages\\Clusteroids' ***\n");
		else
			bkPrintf("opened Clusteroids package\n");
		CL_I32(self, 0x28) = 0;
		return;
	}
	g_ClVariantPackage = 0;
	CL_I32(self, 0x28) = 0;
}

/* 0x000d9620  Clusteroids/CGameMode_Clusteroids.cpp
LoadingStarted (+0x2c): the packages are in. Loads their resources, the sounds, the
timers and the camera, the shared effects, then the arena and the trails. */
void CGameModeClusteroids_LoadingStarted(void *self)
{
	bLoadPackageResources((struct bkPackage *)g_ClPackage, 0xffffffff, 0xdefa, 0);
	if (g_ClVariantPackage != 0)
		bLoadPackageResources((struct bkPackage *)g_ClVariantPackage, 0xffffffff, 0xdefa, 0);
	ClustSound_Load(g_ClPackage);
	ClustTimer_Init();
	if (ClustCamera_Init(g_ClCamera, g_ClPackage) == 0)
		bkPrintf("CClust_Game::Initialise - Camera_Init FAILED!\n");
	CProjector_Reset();
	Sprite_LoadDefs((struct CSpriteDef *)CL_ADDR(0x00295f10), 0xd, 0);
	Sprite_LoadDefs((struct CSpriteDef *)s_aisteam1_00290f00, 6, 0);
	Debris_Init(NULL, 1000);
	g_ClPhase = 1;
	CL_VAR(int, 0x002fa27c) = 0;
	ClustGame_Init();
	ClustTrails_Init();
	g_ClGameOver = 0;
	CL_I32(self, 0x28) = 1;
}

/* 0x000d96f0  Clusteroids/CGameMode_Clusteroids.cpp
Resume (+0x08). */
void CGameModeClusteroids_Resume(void *self)
{
	if (CL_I32(self, 0x28) != 0)
		bkPrintf("CGameMode_Clusteroids::GainFocus()\n");
}

/* 0x000d9710  Clusteroids/CGameMode_Clusteroids.cpp
Suspend (+0x04): deletes the debug input channels and map and every player's input map. */
void CGameModeClusteroids_Suspend(void *self)
{
	int player;
	char *pl;

	if (CL_I32(self, 0x28) == 0)
		return;
	bkPrintf("CGameMode_Clusteroids::LoseFocus()\n");
	if (CL_VAR(int, 0x002fa264) != 0)
		biDeleteChannel((struct biChannel *)CL_VAR(int, 0x002fa264));
	if (CL_VAR(int, 0x002fa268) != 0)
		biDeleteChannel((struct biChannel *)CL_VAR(int, 0x002fa268));
	if (CL_VAR(int, 0x002fa26c) != 0)
		biDeleteChannel((struct biChannel *)CL_VAR(int, 0x002fa26c));
	if (CL_VAR(int, 0x002fa270) != 0)
		biDeleteChannel((struct biChannel *)CL_VAR(int, 0x002fa270));
	biDeleteMap((struct biMap *)CL_VAR(int, 0x002fa260));
	for (player = 0; player < 4; player++) {
		pl = CL_PLAYER(player);
		if (CL_I32(pl, PL_IN_STICK_Y) != 0)
			biDeleteChannel((struct biChannel *)CL_I32(pl, PL_IN_STICK_Y));
		if (CL_I32(pl, PL_IN_STICK_X) != 0)
			biDeleteChannel((struct biChannel *)CL_I32(pl, PL_IN_STICK_X));
		if (CL_I32(pl, PL_IN_FIRE) != 0)
			biDeleteChannel((struct biChannel *)CL_I32(pl, PL_IN_FIRE));
		if (CL_I32(pl, PL_IN_BOOST) != 0)
			biDeleteChannel((struct biChannel *)CL_I32(pl, PL_IN_BOOST));
		biDeleteMap((struct biMap *)CL_I32(pl, PL_IN_MAP));
	}
}

/* 0x000d9730  Clusteroids/CGameMode_Clusteroids.cpp
Input (+0x14): while inactive and loading, polls the background loading. */
void CGameModeClusteroids_Input(int *self, int active)
{
	if (active == 0 && self[5] != 0)
		CL_VFN(self, 0x24, void (*)(void *))(self);
}

/* 0x000d9750  Clusteroids/CGameMode_Clusteroids.cpp
Render (+0x1c). */
void CGameModeClusteroids_Render(void *self, int active)
{
	(void)self;
	if (active != 0)
		FUN_000e28b0();
}

/* 0x000d9760  Clusteroids/CGameMode_Clusteroids.cpp
The deleting destructor. */
void *CGameModeClusteroids_DeletingDtor(void *self, unsigned int flags)
{
	CGameModeClusteroids_Dtor(self);
	if ((flags & 1) != 0)
		CL_DELETE(self);
	return self;
}

/* 0x000d9780  Clusteroids/CGameMode_Clusteroids.cpp
Start (+0x30): the HUD (score boxes, a 60 s clock, score texts, bars), the referee bot,
the rumble menu, the tweak menus "Clust Tweaks" and "Clust Colours", a boost icon per
player, and the fog colour (the old one is restored by Popped). */
void CGameModeClusteroids_Start(void *self)
{
	unsigned int desc[0xec];
	int scores[4];
	int player;
	int overlay;
	int i;
	struct CTweakMenu *menu;

	memset(desc, 0, sizeof desc);
	FUN_00072720((undefined4)DAT_002f7a5c);
	CLHud_CreateScoreBoxes(DAT_002ecfac);
	CLHud_CreateClock(DAT_002ecfac, 60.0f);
	CLHud_StopCountdown(DAT_002ecfac);
	CLHud_ResetCountdown(DAT_002ecfac);
	cl_HudReset(DAT_002ecfac);
	scores[0] = 0;
	scores[1] = 0;
	scores[2] = 0;
	scores[3] = 0;
	CLHud_CreateScoreTexts(DAT_002ecfac, scores);
	CLHud_CreateBars(DAT_002ecfac);
	RefBot_Create();
	CLRumble_CreateMenu((int *)DAT_002ed324);
	for (i = 0xa8; i < 0xb8; i += 4)
		CL_F32(DAT_002ecfac, i) = 75.0f;

	menu = (struct CTweakMenu *)CL_NEW(0xe8, CL_FILE_GAMEMODE, 0xc1, 0x1001);
	g_ClTweaks = menu == NULL ? NULL : CTweakMenu_Ctor(menu, "Clust Tweaks");
	menu = (struct CTweakMenu *)CL_NEW(0xe8, CL_FILE_GAMEMODE, 0xc2, 0x1001);
	g_ClColours = menu == NULL ? NULL : CTweakMenu_Ctor(menu, "Clust Colours");

	menu = g_ClTweaks;
	CTweakMenu_AddFloat(menu, "playerAccel", &CL_VAR(float, 0x002f7d0c), 0.01f, 0.5f, 2.0f);
	CTweakMenu_AddFloat(menu, "playerDrag", &CL_VAR(float, 0x002f7d08), 0.01f, 0.01f, 1.0f);
	CTweakMenu_AddFloat(menu, "playerTurnRate", &CL_VAR(float, 0x002f7d14), 0.05f, 0.10471976f, 0.62831855f);
	CTweakMenu_AddFloat(menu, "playerFireDelay", &CL_VAR(float, 0x002f7d18), 1.0f, 1.0f, 50.0f);
	CTweakMenu_AddFloat(menu, "playerBoostDelay", &CL_VAR(float, 0x002f7d1c), 1.0f, 1.0f, 500.0f);
	CTweakMenu_AddFloat(menu, "tailsPodAttachTime", &CL_VAR(float, 0x002f7d64), 1.0f, 100.0f, 1000.0f);
	CTweakMenu_AddBool(menu, "lastPodPermanent", &CL_VAR(int, 0x002f7d68));
	CTweakMenu_AddFloat(menu, "collectorTargetScore", &CL_VAR(float, 0x002f7d6c), 1.0f, 10.0f, 100.0f);
	CTweakMenu_AddFloat(menu, "aiMaxSpeed", &CL_VAR(float, 0x002f7d78), 0.05f, 0.05f, 1.0f);
	CTweakMenu_AddFloat(menu, "aiAccuracy", &CL_VAR(float, 0x002f7d7c), 0.1f, 0.5f, 10.0f);
	CTweakMenu_AddFloat(menu, "aiTurnRate", &CL_VAR(float, 0x002f7d80), 0.05f, 0.10471976f, 0.62831855f);
	CTweakMenu_AddFloat(menu, "aiFireRate", &CL_VAR(float, 0x002f7d84), 0.01f, 0.1f, 1.0f);
	CTweakMenu_AddFloat(menu, "aiBoostRate", &CL_VAR(float, 0x002f7d88), 0.01f, 0.1f, 1.0f);
	CTweakMenu_AddFloat(menu, "aiShootAheadRate", &CL_VAR(float, 0x002f7d8c), 1.0f, 1.0f, 1000.0f);
	CTweakMenu_AddFloat(menu, "trailX", &CL_VAR(float, 0x002f7d94), 1.0f, -100000.0f, 100000.0f);
	CTweakMenu_AddFloat(menu, "trailY", &CL_VAR(float, 0x002f7d98), 1000.0f, -100000.0f, 100000.0f);
	CTweakMenu_AddFloat(menu, "trailZ", &CL_VAR(float, 0x002f7d9c), 1000.0f, -100000.0f, 100000.0f);
	CTweakMenu_AddBool(menu, "Frame Counter", &CL_VAR(int, 0x002f7da0));
	CTweakMenu_AddBool(menu, "Three Way Firer", &CL_VAR(int, 0x002f7da4));
	CTweakMenu_AddBool(menu, "Render Camera Area", &CL_VAR(int, 0x002f7da8));
	CTweakMenu_AddFloat(menu, "cameraLookAhead", &CL_VAR(float, 0x002f7dac), 0.1f, -10000.0f, 10000.0f);
	CTweakMenu_AddFloat(menu, "bulletSpeed", &CL_VAR(float, 0x002f7db0), 1.0f, -10000.0f, 10000.0f);
	CTweakMenu_AddBool(menu, "highPolyChars", &CL_VAR(int, 0x002f7db4));
	CTweakMenu_AddBool(menu, "boostToggle", &CL_VAR(int, 0x002f7dbc));
	CTweakMenu_AddBool(menu, "radiusToggle", &CL_VAR(int, 0x002f7dc0));
	CTweakMenu_AddBool(menu, "drawTimeToggle", &CL_VAR(int, 0x002f7dc4));

	menu = g_ClColours;
	CTweakMenu_AddInt(menu, "Fog R", &CL_VAR(int, 0x00294bd8), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Fog G", &CL_VAR(int, 0x00294bdc), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Fog B", &CL_VAR(int, 0x00294be0), 1, 0, 0xff, 0);
	CTweakMenu_AddBool(menu, "Snap Colours", &CL_VAR(int, 0x002f7db8));
	CTweakMenu_AddInt(menu, "Top Cloud R", &CL_VAR(int, 0x00294be8), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Top Cloud G", &CL_VAR(int, 0x00294bec), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Top Cloud B", &CL_VAR(int, 0x00294bf0), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Top Cloud A", &CL_VAR(int, 0x00294bf4), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Low Cloud R", &CL_VAR(int, 0x00294bf8), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Low Cloud G", &CL_VAR(int, 0x00294bfc), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Low Cloud B", &CL_VAR(int, 0x00294c00), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(menu, "Low Cloud A", &CL_VAR(int, 0x00294c04), 1, 0, 0xff, 0);

	/* a boost icon overlay in every player's score box, shown for humans */
	for (player = 0; player < DAT_002bae94; player++) {
		memset(desc, 0, sizeof desc);
		desc[0x288 / 4] = 0;                                  /* image */
		((float *)desc)[0x250 / 4] = 90.0f;                  /* colour r, g, b, a */
		((float *)desc)[0x254 / 4] = 90.0f;
		((float *)desc)[0x258 / 4] = 90.0f;
		((float *)desc)[0x25c / 4] = 255.0f;
		((float *)desc)[0x2b4 / 4] = 20.5f;                  /* position */
		((float *)desc)[0x2b8 / 4] = 22.5f;
		((float *)desc)[0x2bc / 4] = 0.0f;
		((float *)desc)[0x308 / 4] = 25.0f;                  /* size */
		((float *)desc)[0x30c / 4] = 25.0f;
		desc[1] = (unsigned int)bLoadResource(NULL, "effects\\textures\\boosticon.bmp", 0, 0xdefa);
		desc[0x290 / 4] = 0x80;
		overlay = COverlay_CreateChild(CL_I32(DAT_002ecfac, 0xf0 + player * 4), (undefined4 *)desc);
		CL_VAR(int, 0x002f7a4c + player * 4) = overlay;
		CL_I16(overlay, 0x54) = (short)(DAT_002baed8[player * 10] == 0);
	}

	/* save the fog colour and set the arena's */
	CL_VAR(unsigned int, 0x002f9b04) = DAT_002ba940;
	CL_VAR(unsigned int, 0x002f9b08) = DAT_002ba944;
	CL_VAR(unsigned int, 0x002f9b0c) = DAT_002ba948;
	CL_VAR(unsigned int, 0x002f9b10) = DAT_002ba94c;
	DAT_002ba940 = CL_VAR(unsigned int, 0x00294bd8);
	DAT_002ba944 = CL_VAR(unsigned int, 0x00294bdc);
	DAT_002ba948 = CL_VAR(unsigned int, 0x00294be0);
	DAT_002ba94c = CL_VAR(unsigned int, 0x00294be4);
	CL_U32(self, 0x10) = CL_U32(self, 0x10) | 1;
	bdSetNearFar(40.0f, 30000.0f);
}

/* 0x000d9f30  Clusteroids/CGameMode_Clusteroids.cpp
Update (+0x18): one frame of the game while it is the top mode: the pause menu, the
timers, the clock, the game, the HUD scores and bars; when the game is over the scores go
to the core player table and the mode pops. */
void CGameModeClusteroids_Update(void *self, int active)
{
	int scores[4];
	int player;
	float value;
	char *pl;

	(void)self;
	if (active == 0)
		return;
	CL_VAR(short, 0x002f7f72) = (short)DAT_002baed8[0];
	CL_VAR(short, 0x002f7f78) = (short)DAT_002baed8[30];
	CL_VAR(short, 0x002f7f74) = (short)DAT_002baed8[10];
	CL_VAR(short, 0x002f7f76) = (short)DAT_002baed8[20];
	if (CL_VAR(int, 0x002f9ad8) == 0 && CL_VAR(int, 0x002f9ae0) == 0 &&
		cl_PauseMenuUpdate(DAT_002ecd90) != 0)
		return;
	ClustCountdown_Update();
	ClustTimer_UpdateFrame();
	if (g_ClPhase == 1) {
		CLHud_SetClock(DAT_002ecfac, 60.0f, 1);
	} else if (g_ClPhase == 2) {
		CLHud_SetClock(DAT_002ecfac, ClustCountdown_GetRemaining(), 1);
	} else if (g_ClPhase == 3) {
		CLHud_SetClock(DAT_002ecfac, 0.0f, 1);
		FUN_000e2210();
		CL_I16(CL_VAR(int, 0x002f7a4c), 0x54) = 1;
		CL_I16(CL_VAR(int, 0x002f7a50), 0x54) = 1;
		CL_I16(CL_VAR(int, 0x002f7a54), 0x54) = 1;
		CL_I16(CL_VAR(int, 0x002f7a58), 0x54) = 1;
	}
	/* a player that changed between human and computer after the pause menu */
	if (CL_VAR(char, 0x002fa361) != 0) {
		if (DAT_002ecdec != 0)
			return;
		CL_VAR(char, 0x002fa361) = 0;
		for (player = 0; player < 4; player++) {
			if ((int)CL_VAR(short, 0x002f7f72 + player * 2) != DAT_002baed8[player * 10])
				CL_I16(CL_PLAYER(player), PL_CONTROL) =
					(short)((CL_VAR(short, 0x002f7f72 + player * 2) == 0) + 1);
		}
	}
	CLHud_AddSeconds(DAT_002ecfac, g_ClFrameTime);
	CL_VAR(float, 0x002fa248) = (60.0f - CL_VAR(float, 0x002fa244)) + CL_VAR(float, 0x002fa248);
	ClustGame_Update();
	for (player = 0; player < 4; player++)
		scores[player] = (int)CL_I16(CL_PLAYER(player), PL_SCORE);
	CLHud_SetScoresInt(DAT_002ecfac, scores);
	value = (float)ClustGame_GetLowestScore();
	player = ClustGame_GetHighestScore();
	CL_F32(DAT_002ecfac, 0x3c) = value;
	CL_F32(DAT_002ecfac, 0x40) = (float)player;
	cl_HudUpdateBars(DAT_002ecfac);
	for (player = 0; player < 4; player++)
		scores[player] = (int)CL_I16(CL_PLAYER(player), PL_SCORE);
	CLHud_SetScores(DAT_002ecfac, scores);
	if (g_ClGameOver != 0) {
		if (CL_VAR(int, 0x002f9abc) != 0) {
			CLHud_DeleteWinnerText(DAT_002ecfac);
			CL_VAR(int, 0x002f9abc) = 0;
		}
		DAT_002baee8[0] = (int)CL_I16(CL_PLAYER(0), PL_SCORE);
		DAT_002baee8[10] = (int)CL_I16(CL_PLAYER(1), PL_SCORE);
		DAT_002baee8[20] = (int)CL_I16(CL_PLAYER(2), PL_SCORE);
		DAT_002baee8[30] = (int)CL_I16(CL_PLAYER(3), PL_SCORE);
		CGameModeStack_Pop(CL_MODESTACK, 1);
	}
	/* the debris wind sources: every player's position (x, z) */
	for (player = 0; player < 4; player++) {
		pl = CL_PLAYER(player);
		CL_U32(pl, PL_WIND) = CL_U32(pl, PL_X);
		CL_U32(pl, PL_WIND + 4) = 0;
		CL_U32(pl, PL_WIND + 8) = CL_U32(pl, PL_Z);
		CL_U32(pl, PL_WIND + 12) = 0;
	}
	FUN_00082b20((float *)CL_P(CL_PLAYER(0), PL_WIND), (float *)CL_P(CL_PLAYER(1), PL_WIND),
		(float *)CL_P(CL_PLAYER(2), PL_WIND), (float *)CL_P(CL_PLAYER(3), PL_WIND));
}

/* 0x000da1f0  Clusteroids/CGameMode_Clusteroids.cpp
Popped (+0x0c): frees everything the game loaded and restores the fog colour. When the
game was never set up (loadState still set) only the packages are closed. */
void CGameModeClusteroids_Popped(void *self)
{
	struct CTweakMenu *menu;

	if (CL_I32(self, 4) == 0) {
		bkClosePackage((struct bkPackage *)g_ClPackage);
		if (g_ClVariantPackage != 0)
			bkClosePackage((struct bkPackage *)g_ClVariantPackage);
		return;
	}
	if (CL_I32(self, 0x28) == 0)
		return;
	if (CL_VAR(int, 0x002f9abc) != 0) {
		CLHud_DeleteWinnerText(DAT_002ecfac);
		CL_VAR(int, 0x002f9abc) = 0;
	}
	Sprite_FreeDefs((struct CSpriteDef *)CL_ADDR(0x00295f10), 0xd);
	Sprite_FreeDefs((struct CSpriteDef *)s_aisteam1_00290f00, 6);
	CLDebris_Start();
	RefBot_Delete();
	CL_DELETE(CL_VAR(void *, 0x002fedf0));
	CL_VAR(void *, 0x002fedf0) = NULL;
	bkPrintf("CGameMode_Clusteroids::Popped()\n");
	if (g_ClPackage != 0) {
		bkPrintf("closing Clusteroids package\n");
		ClustSound_Free();
		bkClosePackage((struct bkPackage *)g_ClPackage);
		if (g_ClVariantPackage != 0)
			bkClosePackage((struct bkPackage *)g_ClVariantPackage);
		g_ClPackage = 0;
		g_ClVariantPackage = 0;
		ClustArena_Free();
	}
	/* 0x001bb790 is a jump to CLIntroCam_Destroy (called with ECX = 0x00296ee4) */
	CLIntroCam_Destroy();
	COverlay_DeleteAll();
	CProjector_DeleteAll();
	menu = g_ClTweaks;
	if (menu != NULL) {
		CTweakMenu_Dtor(menu);
		CL_DELETE(menu);
	}
	menu = g_ClColours;
	if (menu != NULL) {
		CTweakMenu_Dtor(menu);
		CL_DELETE(menu);
	}
	CL_I32(self, 0x28) = 0;
	CLRumble_Shutdown((int *)DAT_002ed324);
	bdSetRenderState(8, 0, 0);
	DAT_002ba940 = CL_VAR(unsigned int, 0x002f9b04);
	DAT_002ba944 = CL_VAR(unsigned int, 0x002f9b08);
	DAT_002ba948 = CL_VAR(unsigned int, 0x002f9b0c);
	DAT_002ba94c = CL_VAR(unsigned int, 0x002f9b10);
}
