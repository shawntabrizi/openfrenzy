# core_modes_b prototype overrides (docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;

# Core/unplaced.c: the score table
0001bd30 void CScoreTable_Clear(struct CScoreTable *self);
0001bd70 int CScoreTable_Rank(struct CScoreTable *self);
0001c0f0 void CScoreTable_AddRound(struct CScoreTable *self, int *scores);
0001c1b0 void CScoreTable_EndRound(struct CScoreTable *self);
0001c200 void cmb_sort_players(int *values, int *place_of, int *order);

# Core/CGameMode_MainMenu.c
0001c2a0 struct CGameMode_MainMenu *CGameMode_MainMenu_ctor(struct CGameMode_MainMenu *self);
0001c2d0 void CGameMode_MainMenu_Load(struct CGameMode_MainMenu *self);
0001c300 void CGameMode_MainMenu_Resume(struct CGameMode_MainMenu *self);
0001c310 void CGameMode_MainMenu_Stop(struct CGameMode_MainMenu *self);
0001c390 void CGameMode_MainMenu_StartParty(struct CGameMode_MainMenu *self);
0001c3d0 void CGameMode_MainMenu_StartTournament(struct CGameMode_MainMenu *self);
0001c400 void CGameMode_MainMenu_Render(struct CGameMode_MainMenu *self, int active);
0001c470 void CGameMode_MainMenu_Update(struct CGameMode_MainMenu *self, int active);
0001c510 void CGameMode_MainMenu_CreateBoxes(struct CGameMode_MainMenu *self);
0001c800 void CGameMode_MainMenu_PrepareLaunch(void);
0001c860 void *CMainMenuBox_DeletingDtor(void *self, unsigned char flags);
0001c880 void CMainMenuBox_Dtor(void *self);
0001c890 void CGameMode_MainMenu_Start(struct CGameMode_MainMenu *self);
0001cb20 int CGameMode_MainMenu_OnButton(struct CGameMode_MainMenu *self, int button, int pad);
0001cd50 struct CGameMode_MainOptions *CGameMode_MainOptions_ctor(struct CGameMode_MainOptions *self);
0001cd90 void CGameMode_MainOptions_Stop(struct CGameMode_MainOptions *self);
0001cdf0 void CGameMode_MainOptions_RefreshTexts(struct CGameMode_MainOptions *self);
0001cf90 void CGameMode_MainOptions_Update(struct CGameMode_MainOptions *self, int active);
0001cfd0 void CGameMode_MainOptions_PlayTrack(int step);
0001d060 void CGameMode_MainOptions_DrawMeter(float x, float y, int level, int unused);
0001d1e0 void *CGameMode_MainMenu_DeletingDtor(void *self, unsigned char flags);

# Core/CGameMode_MainOptions.c
0001d200 void CGameMode_MainOptions_Start(struct CGameMode_MainOptions *self);
0001d790 int CGameMode_MainOptions_OnButton(struct CGameMode_MainOptions *self, int button, int pad);
0001dd30 void CGameMode_MainOptions_Render(struct CGameMode_MainOptions *self, int active);

# Core/unplaced.c
0001ddb0 void *FUN_0001ddb0(void *self);

# Core/CGameMode_Map.c
0001ddd0 void CGameMode_Map_Stop(struct CGameMode_Map *self);
0001de90 void CGameMode_Map_StartZoomOut(struct CGameMode_Map *self);
0001df10 void CGameMode_Map_StartDraw(struct CGameMode_Map *self);
0001dfb0 void CGameMode_Map_SkipDraw(struct CGameMode_Map *self);
0001e060 void CGameMode_Map_Turn(struct CGameMode_Map *self);
0001e0a0 void CGameMode_Map_Choose(struct CGameMode_Map *self);
0001e290 void CGameMode_Map_StartFadeIn(struct CGameMode_Map *self);
0001e300 void CGameMode_Map_FadeIn(struct CGameMode_Map *self);
0001e490 void CGameMode_Map_FadeOut(struct CGameMode_Map *self);
0001e560 void CGameMode_Map_RenderFade(struct CGameMode_Map *self);
0001e940 void CGameMode_Map_Fly(struct CGameMode_Map *self, float t);
0001e9d0 void cmb_vec3_add(float *out, float *a, float *b);
0001ea00 void cmb_vec3_scale(float *out, float *v, float s);
0001ea30 float cmb_vec3_length(float *v);
0001ea60 struct CGameMode_Map *CGameMode_Map_ctor(struct CGameMode_Map *self, int kind);
0001eb10 void *CGameMode_Map_DeletingDtor(struct CGameMode_Map *self, unsigned char flags);
0001eb30 void CGameMode_Map_dtor(struct CGameMode_Map *self);
0001eba0 void CGameMode_Map_CreateOverlays(struct CGameMode_Map *self);
0001f180 void CGameMode_Map_CreatePrompt(struct CGameMode_Map *self);
0001f2e0 void CGameMode_Map_StartFlight(struct CGameMode_Map *self);
0001f460 void CGameMode_Map_StartRound(struct CGameMode_Map *self);
0001f8c0 void CGameMode_Map_DrawOrder(struct CGameMode_Map *self);
0001fac0 void CGameMode_Map_DimOthers(struct CGameMode_Map *self);
0001fcc0 void CGameMode_Map_View(struct CGameMode_Map *self);
0001fd80 void CGameMode_Map_Render3D(struct CGameMode_Map *self);
00020100 void CGameMode_Map_RenderPath(struct CGameMode_Map *self);
000204f0 void CGameMode_Map_StartRoundFade(struct CGameMode_Map *self);
000205d0 void CGameMode_Map_RoundFade(struct CGameMode_Map *self);
000206a0 void CGameMode_Map_Leave(struct CGameMode_Map *self);
000208a0 void CGameMode_Map_Load(struct CGameMode_Map *self);
00020b50 void CGameMode_Map_Loaded(struct CGameMode_Map *self);
00021080 void CGameMode_Map_PromptCountdown(struct CGameMode_Map *self);
00021100 void CGameMode_Map_Update(struct CGameMode_Map *self, int active);
00021630 void CGameMode_Map_Render(struct CGameMode_Map *self, int active);
000216a0 int CGameMode_Map_OnButton(struct CGameMode_Map *self, int button, int pad);

# Core/CGameMode_MetaVote.c (0x00021820-0x000218e0 are unplaced in the table)
00021820 struct CGameMode_MetaVote *CGameMode_MetaVote_ctor(struct CGameMode_MetaVote *self);
00021880 void CGameMode_MetaVote_dtor(struct CGameMode_MetaVote *self);
000218e0 void CGameMode_MetaVote_Finish(struct CGameMode_MetaVote *self);
00021970 void *CGameMode_MetaVote_DeletingDtor(struct CGameMode_MetaVote *self, unsigned char flags);
00021990 void CGameMode_MetaVote_Load(struct CGameMode_MetaVote *self);
00021c40 void CGameMode_MetaVote_Stop(struct CGameMode_MetaVote *self);
00021d50 void CGameMode_MetaVote_MoveTokens(struct CGameMode_MetaVote *self);
00022010 void CGameMode_MetaVote_Render(struct CGameMode_MetaVote *self, int active);
00022280 int CGameMode_MetaVote_OnButton(struct CGameMode_MetaVote *self, int button, int pad);
00022530 struct CMetaToken *CGameMode_MetaVote_MakeToken(struct CGameMode_MetaVote *self, int player, int index);
00022660 void CGameMode_MetaVote_Start(struct CGameMode_MetaVote *self);
00022f40 void CGameMode_MetaVote_Update(struct CGameMode_MetaVote *self, int active);
000233b0 void CGameMode_MetaVote_Loaded(struct CGameMode_MetaVote *self);

# Core/CGameMode_MiniGameIntro.c (all but 0x00023a00 are unplaced in the table)
00023440 struct CGameMode_MiniGameIntro *CGameMode_MiniGameIntro_ctor(struct CGameMode_MiniGameIntro *self, int game, const char *name);
00023530 void CGameMode_MiniGameIntro_dtor(struct CGameMode_MiniGameIntro *self);
00023590 void CGameMode_MiniGameIntro_Start(struct CGameMode_MiniGameIntro *self);
000235d0 void CGameMode_MiniGameIntro_Suspend(struct CGameMode_MiniGameIntro *self);
00023600 int CGameMode_MiniGameIntro_OnButton(struct CGameMode_MiniGameIntro *self, int button, int pad);
000237b0 void CGameMode_MiniGameIntro_Update(struct CGameMode_MiniGameIntro *self, int active);
00023900 void CGameMode_MiniGameIntro_Render(struct CGameMode_MiniGameIntro *self, int active);
000239e0 void *CGameMode_MiniGameIntro_DeletingDtor(struct CGameMode_MiniGameIntro *self, unsigned char flags);
00023a00 void CGameMode_MiniGameIntro_CreateTexts(struct CGameMode_MiniGameIntro *self);
000242d0 void CGameMode_MiniGameIntro_Enter(struct CGameMode_MiniGameIntro *self);
00024510 void CGameMode_MiniGameIntro_Exit(struct CGameMode_MiniGameIntro *self);

# Core/CGameMode_MiniGameSelect.c (0x00024620 and 0x000247b0 are unplaced in the table)
00024620 struct CGameMode_MiniGameSelect *CGameMode_MiniGameSelect_ctor(struct CGameMode_MiniGameSelect *self);
000247b0 void CGameMode_MiniGameSelect_dtor(struct CGameMode_MiniGameSelect *self);
00024820 void CGameMode_MiniGameSelect_Enter(struct CGameMode_MiniGameSelect *self);
00024880 void CGameMode_MiniGameSelect_Suspend(struct CGameMode_MiniGameSelect *self);
000248f0 void CGameMode_MiniGameSelect_Buttons(struct CGameMode_MiniGameSelect *self, int active);
00024940 void CGameMode_MiniGameSelect_Update(struct CGameMode_MiniGameSelect *self, int active);
00024a00 void CGameMode_MiniGameSelect_Render(struct CGameMode_MiniGameSelect *self, int active);
00024cc0 void CGameMode_MiniGameSelect_ShowGame(struct CGameMode_MiniGameSelect *self, int index, int from_below);
00024d40 int CGameMode_MiniGameSelect_OnButton(struct CGameMode_MiniGameSelect *self, int button, int pad);
00024fe0 void *CGameMode_MiniGameSelect_DeletingDtor(struct CGameMode_MiniGameSelect *self, unsigned char flags);
00025000 void CGameMode_MiniGameSelect_Start(struct CGameMode_MiniGameSelect *self);
00025290 void CGameMode_MiniGameSelect_Loaded(struct CGameMode_MiniGameSelect *self);

# Core/CGameMode_ObjectViewer.c (all but 0x00025b40 are unplaced in the table)
00025a20 struct CGameMode_ObjectViewer *CGameMode_ObjectViewer_ctor(struct CGameMode_ObjectViewer *self);
00025a40 void CGameMode_ObjectViewer_dtor(struct CGameMode_ObjectViewer *self);
00025a50 void CGameMode_ObjectViewer_Exit(struct CGameMode_ObjectViewer *self);
00025a90 void CGameMode_ObjectViewer_Start(struct CGameMode_ObjectViewer *self);
00025aa0 void CGameMode_ObjectViewer_Update(struct CGameMode_ObjectViewer *self, int active);
00025ae0 void CGameMode_ObjectViewer_Render(struct CGameMode_ObjectViewer *self, int active);
00025b40 void *CGameMode_ObjectViewer_MakeActor(struct CGameMode_ObjectViewer *self, void *resource);
00025bd0 void *CGameMode_ObjectViewer_DeletingDtor(struct CGameMode_ObjectViewer *self, unsigned char flags);
00025bf0 void CGameMode_ObjectViewer_Input(struct CGameMode_ObjectViewer *self, int active);
00025e70 void CGameMode_ObjectViewer_Enter(struct CGameMode_ObjectViewer *self);

# Core/CGameMode_PressStart.c (0x00026180 is unplaced in the table)
00026180 struct CGameMode_PressStart *CGameMode_PressStart_ctor(struct CGameMode_PressStart *self);
00026200 void CGameMode_PressStart_Suspend(struct CGameMode_PressStart *self);
00026290 int CGameMode_PressStart_OnButton(struct CGameMode_PressStart *self, int button, int pad);
000262d0 void *CGameMode_PressStart_DeletingDtor(struct CGameMode_PressStart *self, unsigned char flags);
000262f0 void CGameMode_PressStart_dtor(struct CGameMode_PressStart *self);
00026350 void CGameMode_PressStart_Start(struct CGameMode_PressStart *self);
00026790 void CGameMode_PressStart_Update(struct CGameMode_PressStart *self, int active);
00026a30 void CGameMode_PressStart_Render(struct CGameMode_PressStart *self, int active);

# Core/unplaced.c
00025920 void *cmb_six_lists_ctor(void *self);
000259a0 void cmb_six_lists_dtor(void *self);
00026bf0 void CGameMode_Root_PollGUI(void);
00026c60 void *CGameMode_Root_ctor(void *self);
00026ce0 void CGameMode_Root_dtor(void *self);
