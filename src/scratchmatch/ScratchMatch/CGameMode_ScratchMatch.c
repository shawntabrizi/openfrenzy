/*
ScratchMatch/CGameMode_ScratchMatch.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CGameMode_ScratchMatch.cpp
(module scratchmatch)

CGameMode_ScratchMatch: the game mode of Scratch Match (vtable 0x00276848, 0x2c bytes). Enter
("pushed") queues the packages for loading; when they are loaded the mode stack calls Init
("Initeonetimed": every subsystem and the tweak menus), then Start ("gamestarted": the HUD and
the intro camera). Process runs a frame of the current game state, Draw draws it, Exit
("popped") frees everything.
*/

#include "scratchmatch.h"

static const char s_ModeFile[] = "C:\\BlitzParty\\Game\\Src\\ScratchMatch\\CGameMode_ScratchMatch.cpp";

/* classlib_ui 0x00071f90 returns the package (its table prototype says void) */
#define sm_AddPackage(name) \
	((struct bkPackage *(*)(const char *, int))CLLoading_AddPackage)((name), 0)

/* the mode object (CGameMode +0x00-0x27) */
#define SMM_FIELD04(m) SM_AT(m, 0x04, int)      /* 0: the mode never finished loading */
#define SMM_FLAGS(m) SM_AT(m, 0x10, unsigned int)
#define SMM_LOADSTATE(m) SM_AT(m, 0x14, int)    /* 1: loading */
#define SMM_INITIALISED(m) SM_AT(m, 0x28, int)  /* Init has run */

/* 0x00276848  CGameMode_ScratchMatch vtable */
const void *const vtable_00276848[13] = {
	(const void *)FUN_001ba970, /* [0] +0x00 scalar deleting destructor */
	(const void *)FUN_001d5950, /* [1] +0x04 suspend (empty) */
	(const void *)FUN_001d5950, /* [2] +0x08 resume (empty) */
	(const void *)FUN_001bb130, /* [3] +0x0c exit */
	(const void *)FUN_001ba710, /* [4] +0x10 enter */
	(const void *)FUN_000d9730, /* [5] +0x14 update (polls the loading) */
	(const void *)FUN_001bae30, /* [6] +0x18 process */
	(const void *)FUN_001ba8a0, /* [7] +0x1c draw */
	(const void *)FUN_001ec260, /* [8] +0x20 post draw (empty) */
	(const void *)FUN_00012930, /* [9] +0x24 poll the loading */
	(const void *)FUN_000128e0, /* [10] +0x28 loading finished */
	(const void *)FUN_001ba990, /* [11] +0x2c init (the packages are loaded) */
	(const void *)FUN_001ba7a0, /* [12] +0x30 start */
};

/* 0x001ba6d0  (gap; the mode's constructor): CGameMode_ScratchMatch::CGameMode_ScratchMatch */
void *CGameMode_ScratchMatch_Ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, "ScratchMatch");
	*(const void *const **)self = vtable_00276848;
	SMM_INITIALISED(self) = 0;
	return self;
}

/* 0x001ba6f0  (gap; the mode's destructor): CGameMode_ScratchMatch::~CGameMode_ScratchMatch */
void CGameMode_ScratchMatch_Dtor(void *self)
{
	*(const void *const **)self = vtable_00276848;
	SMM_INITIALISED(self) = 0;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x001ba710  ScratchMatch/CGameMode_ScratchMatch.cpp: enter (pushed): queues the character
voices, the common and the variation package, and opens the music package */
void CGameMode_ScratchMatch_Enter(void *self)
{
	SMM_LOADSTATE(self) = 1;
	Core_LoadCharacterVoices(1);
	sm_Package = sm_AddPackage("packages\\scratch");
	if (sm_Package == NULL)
		SM_ASSERT_FAIL("CGameMode_ScratchMatch.cpp", 0x75, "POINTER ASSERTION FAILURE: scPakIndex");
	sm_VarPackage = sm_AddPackage(sm_VariationPackages[sm_Variation]);
	if (sm_VarPackage == NULL)
		SM_ASSERT_FAIL("CGameMode_ScratchMatch.cpp", 0x79, "POINTER ASSERTION FAILURE: scVarPakIndex");
	SMSound_OpenMusicPackage();
	bkPrintf("MH: Scratch Match pushed\n");
	SMM_INITIALISED(self) = 0;
}

/* 0x001ba7a0  ScratchMatch/CGameMode_ScratchMatch.cpp: start (after init): timers, the crowd
sample, refbot, the hovering lights, the HUD (score boxes, clock, bars) and the intro camera */
void CGameMode_ScratchMatch_Start(void *self)
{
	int scores[4];

	SMTimer_Reset(SMT_GAME);
	SMTimer_Reset(SMT_CAMERA);
	SMSound_PlaySFX(0);
	FUN_00072720((undefined4)(uintptr_t)SM_A(0x0030fea4));
	CProjector_Reset();
	RefBot_Create();
	SMGenActors_CreateInstances();
	CLHud_CreateScoreBoxes(SM_HUD);
	CLHud_CreateClock(SM_HUD, sm_GameLength);
	/* the original also passes 10 / game length, which 0x00078930 ignores (ret 4) */
	CLHud_StopCountdown(SM_HUD);
	scores[0] = 0;
	scores[1] = 0;
	scores[2] = 0;
	scores[3] = 0;
	CLHud_CreateScoreTexts(SM_HUD, scores);
	CLHud_ResetCountdown(SM_HUD);
	CLHud_CreateBars(SM_HUD);
	SM_AT(DAT_002ecfac, 0x88, int) = 1;
	sm_WinnerText = 0;
	sm_Wander = 0;
	SMCamera_LoadIntro(sm_Package);
	SMM_FLAGS(self) |= 1;
	sm_Started = 1;
	bkPrintf("MH: Scratch Match gamestarted\n");
}

/* 0x001ba8a0  ScratchMatch/CGameMode_ScratchMatch.cpp: draw: the camera, the scene, the
overlays; then the fade-out at the end of the outro and the fade-in of the intro */
void CGameMode_ScratchMatch_Draw(void *self, int top)
{
	double t;
	int alpha;

	(void)self;
	if (top == 0)
		return;
	SMCamera_SetView();
	SMRender_Draw();
	FUN_001d5950();
	COverlay_DrawAll(1);
	if (sm_FadeState == 1) {
		alpha = SM_FTOL(sm_FadeAlpha);
		if (alpha > 0xff)
			alpha = 0xff;
		CLScreen_DrawDarken(alpha, 1);
		if (alpha == 0xff)
			sm_FadeState = 2;
		sm_FadeAlpha = (float)(SMTimer_Elapsed(SMT_FADE) * 127.0f);
	}
	if (sm_State == 0) {
		t = SMTimer_Elapsed(SMT_GAME);
		if (t <= 1.0)
			CLScreen_DrawDarken(0xff - SM_FTOL(t * 255.0f), 1);
		else
			CLScreen_DrawDarken(0, 1);
	}
}

/* 0x001ba970  ScratchMatch/CGameMode_ScratchMatch.cpp: scalar deleting destructor */
void *CGameMode_ScratchMatch_Destroy(void *self, unsigned char flags)
{
	CGameMode_ScratchMatch_Dtor(self);
	if ((flags & 1) != 0)
		SM_DELETE(self);
	return self;
}

/* 0x001ba990  ScratchMatch/CGameMode_ScratchMatch.cpp: init (the packages are loaded):
textures of both packages, the speakers, the font, every subsystem, and the tweak menus
"World", "Sprite", "Camera", "Render" and "Sound" */
void CGameMode_ScratchMatch_Init(void *self)
{
	void *memory;
	struct CTweakMenu *menu;

	bLoadPackageResources(sm_Package, 1, 0xdefa, 0);
	bLoadPackageResources(sm_VarPackage, 1, 0xdefa, 0);
	memory = bkHeapAlloc(0x10, s_ModeFile, 0x92, 0x1001);
	sm_Speakers = memory != NULL ? SMScenic_Ctor((struct SMScenic *)memory) : NULL;
	SMScenic_Load(sm_Speakers, "actors\\speakers.obe");
	sm_Font = (struct bdFont *)bLoadResource(NULL, "handgotl.ttf", 3, 0xdefa);
	if (sm_Font == NULL)
		SM_ASSERT_FAIL("CGameMode_ScratchMatch.cpp", 0x96, "POINTER ASSERTION FAILURE: gpFont");
	SMSprites_Load();
	SMGame_Init();
	SMCamera_Init();
	SMPlayer_InitAll();
	SMSound_Load();
	SMLights_Init();
	SMEffects_Init();
	SMGenActors_Load();

	memory = bkHeapAlloc(0xe8, s_ModeFile, 0xba, 0x1001);
	sm_MenuWorld = memory != NULL ? CTweakMenu_Ctor((struct CTweakMenu *)memory, "World") : NULL;
	CTweakMenu_AddInt(sm_MenuWorld, "Button Biffer Size", &sm_HistoryLength, 1, 1, 0xf, 0);
	CTweakMenu_AddBool(sm_MenuWorld, "Show Framerate", &sm_ShowFramerate);
	CTweakMenu_AddBool(sm_MenuWorld, "Draw Lightbeams", &sm_DrawLightbeams);
	CTweakMenu_AddBool(sm_MenuWorld, "Draw Background", &sm_DrawBackground);
	CTweakMenu_AddBool(sm_MenuWorld, "Draw Actors", &sm_DrawActors);
	CTweakMenu_AddBool(sm_MenuWorld, "Draw Extras", &sm_DrawExtras);

	memory = bkHeapAlloc(0xe8, s_ModeFile, 0xc6, 0x1001);
	sm_MenuSprite = memory != NULL ? CTweakMenu_Ctor((struct CTweakMenu *)memory, "Sprite") : NULL;
	CTweakMenu_AddFloat(sm_MenuSprite, "Bar Y Pos", &sm_BarY, 1.0f, -250.0f, 250.0f);

	memory = bkHeapAlloc(0xe8, s_ModeFile, 0xcb, 0x1001);
	sm_MenuCamera = memory != NULL ? CTweakMenu_Ctor((struct CTweakMenu *)memory, "Camera") : NULL;
	menu = sm_MenuCamera;
	CTweakMenu_AddFloat(menu, "Player Cam Y Pos", &sm_CamPlayerY, 10.0f, -2000.0f, 2000.0f);
	CTweakMenu_AddFloat(sm_MenuCamera, "Player Cam Z Pos", &sm_CamPlayerZ, 10.0f, -2000.0f, 2000.0f);
	CTweakMenu_AddFloat(sm_MenuCamera, "Pl. Cam Y Target", &sm_CamTargetY, 10.0f, -2000.0f, 2000.0f);
	CTweakMenu_AddFloat(sm_MenuCamera, "Pl. Cam Z Target", &sm_CamTargetZ, 10.0f, -2000.0f, 2000.0f);

	memory = bkHeapAlloc(0xe8, s_ModeFile, 0xd1, 0x1001);
	sm_MenuRender = memory != NULL ? CTweakMenu_Ctor((struct CTweakMenu *)memory, "Render") : NULL;
	CTweakMenu_AddInt(sm_MenuRender, "Ambient Light Level", &sm_Ambient, 1, 0, 0x7f, 0);
	CTweakMenu_AddFloat(sm_MenuRender, "Pl.Light offset", &SM_F(0x00310238), 10.0f, -1000.0f, 1000.0f);
	CTweakMenu_AddInt(sm_MenuRender, "Directional Light", &SM_I(0x0031023c), 1, 0, 0x7f, 0);

	memory = bkHeapAlloc(0xe8, s_ModeFile, 0xd8, 0x1001);
	sm_MenuSound = memory != NULL ? CTweakMenu_Ctor((struct CTweakMenu *)memory, "Sound") : NULL;
	CTweakMenu_AddInt(sm_MenuSound, "Music Volume   ", &sm_MusicVolume, 1, 0, 0xff, 0);
	CTweakMenu_AddInt(sm_MenuSound, "Crowd Volume", &sm_CrowdVolume, 1, 0, 0xff, 0);
	CTweakMenu_AddInt(sm_MenuSound, "Scratch Volume", &sm_ScratchVolume, 1, 0, 0xff, 0);
	CTweakMenu_AddInt(sm_MenuSound, "Speech Volume", &sm_SpeechVolume, 1, 0, 0xff, 0);
	CTweakMenu_AddFloat(sm_MenuSound, "Beat Period", &sm_BeatPeriod, 0.0001f, 0.48f, 5.1f);
	bkPrintf("MH: Scratch Match Initeonetimed\n");
	SMM_INITIALISED(self) = 1;
}

/* 0x001bae30  ScratchMatch/CGameMode_ScratchMatch.cpp: process: the pause screen (quits the
game), the pause state, then one frame of the current state, the HUD clock, scores and bars */
void CGameMode_ScratchMatch_Process(void *self, int top)
{
	float scores[4];
	int hudScores[4];
	float last;
	int i;

	(void)self;
	if (top == 0)
		return;
	if (sm_FadeState != 1 && (sm_State != 0 || SMTimer_Elapsed(SMT_GAME) > 1.0) &&
	    CLPause_Update((undefined4 *)DAT_002ecd90) != 0)
		return;
	for (i = 0; i < SM_NUM_PLAYERS; i++)
		sm_Players[i].cpu = sm_PlayerRecord(i)[1] != 1;
	if (DAT_002ecdec != 0 && sm_Paused == 0) {
		sm_Paused = 1;
		bsPauseAll(1);
		SMSound_PauseMusic();
		SMTimer_Pause();
	}
	if (DAT_002ecdec == 0 && sm_Paused == 1) {
		sm_Paused = 0;
		CLMusic_FadeOut((int)0xffffff01);
		bsPauseAll(0);
		SMSound_ResumeMusic();
		SMTimer_Resume();
	}
	SMTimer_UpdateFrame();
	if (sm_Paused == 0) {
		SMRender_UpdateAnimTextures();
		SMLights_CycleColours();
		SMPlayer_UpdateDances();
		SMPlayer_UpdateAnimations();
		SMLights_Update();
		SMPlayer_UpdateIntroDrop();
		SMLights_Update();
		SMScenic_Update(sm_Speakers, (float)SMTimer_FrameTime());
		SMEffects_Update();
		SMGenActors_Update();
		switch (sm_State) {
		case 0:
			SMGame_UpdateIntro();
			break;
		case 1:
			SMGame_UpdatePractice();
			break;
		case 2:
			SMGame_UpdatePlay();
			break;
		case 3:
			SMGame_UpdateOutro();
			break;
		default:
			SM_ASSERT_FAIL("CGameMode_ScratchMatch.cpp", 600, "ASSERTION FAILURE: FALSE");
		}
		COverlay_UpdateAll();
		CProjector_UpdateAll();
		RefBot_Update();
	}
	SMTimer_Elapsed(SMT_BEAT);
	CLHud_SetClock(SM_HUD, -1.0f, 1);
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		if (sm_Players[i].lives < 0)
			sm_Players[i].lives = 0;
	}
	SMGame_RankPlayers();
	for (i = 0; i < SM_NUM_PLAYERS; i++)
		scores[i] = sm_GameVariation != 3 ? (float)sm_Players[i].score : (float)sm_Players[i].lives;
	CLHud_CopyScores(SM_HUD, (undefined4 *)scores);
	last = scores[sm_Rank[3]];
	SM_AT(DAT_002ecfac, 0x40, float) = scores[sm_Rank[0]];
	SM_AT(DAT_002ecfac, 0x3c, float) = last;
	CLHud_UpdateBars(SM_HUD);
	for (i = 0; i < SM_NUM_PLAYERS; i++)
		hudScores[i] = sm_GameVariation == 3 ? sm_Players[i].lives : sm_Players[i].score;
	CLHud_SetScores(SM_HUD, hudScores);
	DebrisColours_SetAll(sm_Players[0].pos, sm_Players[1].pos, sm_Players[2].pos, sm_Players[3].pos);
	if (sm_Finished == 1)
		CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, 1);
}

/* 0x001bb130  ScratchMatch/CGameMode_ScratchMatch.cpp: exit (popped): frees everything Init
made (or only the sounds and packages if loading never finished) and resets the HUD */
void CGameMode_ScratchMatch_Exit(void *self)
{
	DSCAPS caps;
	void *object;
	int offset;

	if (sm_Package == NULL)
		SM_ASSERT_FAIL("CGameMode_ScratchMatch.cpp", 0x126, "POINTER ASSERTION FAILURE: scPakIndex");
	if (sm_VarPackage == NULL)
		SM_ASSERT_FAIL("CGameMode_ScratchMatch.cpp", 0x127, "POINTER ASSERTION FAILURE: scVarPakIndex");
	if (SMM_FIELD04(self) == 0) {
		SMSound_Free();
		bkClosePackage(sm_Package);
		bkClosePackage(sm_VarPackage);
		return;
	}
	if (SMM_INITIALISED(self) == 0)
		return;
	if (sm_State == 0)
		SMCamera_DestroyIntro();
	object = sm_Speakers;
	if (object != NULL) {
		SMScenic_Dtor((struct SMScenic *)object);
		SM_DELETE(object);
	}
	SMGenActors_DeleteInstances();
	SMGenActors_Free();
	SMEffects_Free();
	SMSound_Free();
	SMGame_FreeSequence();
	CProjector_DeleteAll();
	RefBot_Delete();
	SMSprites_Free();
	SMRender_Free();
	SMPlayer_FreeAll();
	SMLights_Free();
	bkClosePackage(sm_Package);
	bkClosePackage(sm_VarPackage);
	object = sm_MenuWorld;
	if (object != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)object);
		SM_DELETE(object);
	}
	object = sm_MenuSprite;
	if (object != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)object);
		SM_DELETE(object);
	}
	object = sm_MenuCamera;
	if (object != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)object);
		SM_DELETE(object);
	}
	object = sm_MenuRender;
	if (object != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)object);
		SM_DELETE(object);
	}
	object = sm_MenuSound;
	if (object != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)object);
		SM_DELETE(object);
	}
	if (sm_WinnerText != 0) {
		CLHud_DeleteWinnerText(SM_HUD);
		sm_WinnerText = 0;
	}
	for (offset = 0xa8; offset < 0xb8; offset += 4)
		SM_AT(DAT_002ecfac, offset, float) = 100.0f;
	SM_AT(DAT_002ecfac, 0x88, int) = 0;
	COverlay_DeleteAll();
	IDirectSound_GetCaps((struct IDirectSound *)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", caps.dwFree3DBuffers);
	bkPrintf("MH: Scratch Match popped\n");
	SMM_INITIALISED(self) = 0;
}
