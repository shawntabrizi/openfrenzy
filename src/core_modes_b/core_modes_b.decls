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
