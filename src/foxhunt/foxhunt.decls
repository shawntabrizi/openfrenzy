# foxhunt prototype overrides (docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;   Only of_types.h / C types and "struct Name *".

# ---- FoxHunt/CModule_FoxHunt.c
00125070 unsigned int *FUN_00125070(unsigned int *self, unsigned char flags);
00125090 void FUN_00125090(int player);
00125360 unsigned int *CModule_FoxHunt_Ctor(unsigned int *self);
00125390 void CModule_FoxHunt_Dtor(unsigned int *self);
001253a0 unsigned int *CModule_FoxHunt_Destroy(unsigned int *self, unsigned char flags);
001253c0 int CModule_FoxHunt_Start(unsigned int *self, int variation);

# ---- FoxHunt/CGameMode_FoxHunt.c
00125560 void *CGameMode_FoxHunt_Ctor(void *self);
00125580 void CGameMode_FoxHunt_Dtor(void *self);
00125590 void CGameMode_FoxHunt_Enter(void *self);
00125640 void CGameMode_FoxHunt_DrawFrameRate(void *self);
00125750 int FoxHunt_ReadQuoted(const char *text, char *out, int max);
001257a0 void CGameMode_FoxHunt_DrawActorPass6(void *self, char *actor);
00125950 void CGameMode_FoxHunt_DrawActor(void *self, char *actor);
00125c20 void FoxHunt_CameraAtPlayer1(void);
00125c90 void FoxHunt_UpdateColourSet(void);
00125cf0 void FoxHunt_SelectRouteHeights(int index);
00125d60 void *CGameMode_FoxHunt_Destroy(void *self, unsigned char flags);
00125d80 void CGameMode_FoxHunt_Init(void *self);
00126220 void CGameMode_FoxHunt_Update(void *self);
00128900 void CGameMode_FoxHunt_UpdateState(void *self);
00129170 int CGameMode_FoxHunt_Results(void *self);
00129860 void CGameMode_FoxHunt_DrawActors(void *self);
00129910 void FoxHunt_CreateIcons(void);
00129ab0 void CGameMode_FoxHunt_Exit(void *self);
00129e50 void CGameMode_FoxHunt_Process(void *self, int top);
0012a190 void CGameMode_FoxHunt_Draw(void *self, int top);
0012a7c0 void CGameMode_FoxHunt_InitCooperation(void *self, void *package);
0012b760 void CGameMode_FoxHunt_InitEnergyBall(void *self, void *package);
0012c790 void CGameMode_FoxHunt_InitOddOneOut(void *self, void *package);
0012d450 void CGameMode_FoxHunt_InitVictim(void *self, void *package);
0012e020 void CGameMode_FoxHunt_Start(void *self);

# ---- FoxHunt/CFHunt_GenericGoal.c
00133530 void *CFHunt_GenericGoal_Ctor(void *self);
00133690 void CFHunt_GenericGoal_SetActive(void *self, char *goal, int on);
00133700 void CFHunt_GenericGoal_SetColour(void *self, char *goal, int colour);
00133720 void CFHunt_GenericGoal_Update(void *self, float dt);
00133780 void CFHunt_GenericGoal_StartEffect(void *self, char *goal, int r, int g, int b, unsigned int level);
001338c0 char *CFHunt_GenericGoal_AddGoal(void *self, float *pos, float size, int user);
00133b00 void CFHunt_GenericGoal_Draw(void *self, int pass);

# ---- FoxHunt/unplaced_sound.c
00133db0 void *FHunt_Sound_Ctor(void *self);
00133e50 int FHunt_Sound_Initialise(void *self, void *package);
00133ee0 int FHunt_Sound_Play(void *self, unsigned int id, int volume, int pitch);
00133f80 void *FoxHunt_Effects_Ctor(void *self);
00133fd0 void FoxHunt_Effects_Update(void *self, float dt);
00133ff0 void FoxHunt_Effects_Draw(void *self);
00134000 char *FoxHunt_AddDebris(int type, float *pos, float *vel, int life, unsigned int colour);
00134360 int FoxHunt_Effects_Init(void *self, int unused, int variation);
00134400 void FoxHunt_LoadPropActors(void);

# ---- FoxHunt/unplaced_player.c
0012e9a0 void FoxHunt_Player_Dtor(void *self);
0012e9b0 void FoxHunt_Player_RouteTo(void *self, float *to);
0012e9e0 void FoxHunt_Player_CheckGoal(void *self);
0012ea60 void FoxHunt_Player_UseEscalator(void *self, float *target);
0012edb0 void FoxHunt_CreatePlayerInputs(void);
0012ef90 void FoxHunt_Player_Rumble(void *self, int kind);
0012f0a0 void *FoxHunt_Player_Ctor(void *self, int index, float *pos);
0012f410 void FoxHunt_Player_FollowRoute(void *self);
0012fb60 void FoxHunt_Player_Think(void *self);
0012fe30 void FoxHunt_Player_Act(void *self);
0012ff00 void FoxHunt_Player_ChooseCooperation(void *self);
00130310 void FoxHunt_Player_ChooseBonus(void *self);
00130400 int FoxHunt_Player_NextPatrolPoint(void *self);
001306b0 int FoxHunt_Player_Retaliate(void *self);
00130780 int FoxHunt_Player_RandomOther(void *self, int skip_down);
00130830 void FoxHunt_Player_AvoidNearest(void *self);
00130d10 void FoxHunt_Player_Hit(void *self, float *from, float power, int side);
00130f30 void FoxHunt_Player_Update(void *self);
00131240 void FoxHunt_UpdatePlayers(void);
00131270 void FoxHunt_Player_RespawnAt(void *self, char *point);
00131530 void FoxHunt_Player_DrawRoute(void *self);
001316e0 void FoxHunt_Player_SetModel(void *self, int normal);
00131810 void FoxHunt_Player_ChooseEnergyBall(void *self);
00131a20 void FoxHunt_Player_ChooseOddOneOut(void *self);
00131af0 void FoxHunt_Player_ChooseVictim(void *self);
00131d30 int FoxHunt_Player_Plan(void *self);
00132a40 int FoxHunt_Player_Attack(void *self);
001331f0 void FoxHunt_Player_Respawn(void *self);
00133310 void FoxHunt_Player_SetMode(void *self, int mode);
00133370 void FUN_00133370(int player);
00133490 void FoxHunt_SharkView(int id, float *pos, float *heading);
001334f0 float FUN_001334f0(float x, float z);

# ---- FoxHunt/CFHunt_Throwable.c
00134460 void *CFHunt_Throwable_Ctor(void *self, int type, float *pos, float size);
00134620 int FoxHunt_Throwable_FindTarget(void *self);
00134770 void FoxHunt_Throwable_Update(void *self);
00135800 void FoxHunt_Buttons_Init(void);
00135840 void *FoxHunt_Button_Ctor(void *self, int id, float a, float b, int c, int d, float *pos, float heading);
00135cd0 int FoxHunt_Button_IsNear(void *self, char *actor);
00135d80 void *FoxHunt_Shark_Ctor(void *self, float *pos, int index);
001359d0 int FoxHunt_Button_Press(void *self);
00135a20 int FoxHunt_Button_Release(void *self);
00135a60 int FoxHunt_Button_Update(void *self);
00135d50 void FoxHunt_UpdateButtons(void);
00135f70 void FoxHunt_LoadSharkModel(void);
00135fb0 char *FoxHunt_Shark_Reset(void *self);
00136040 char *FoxHunt_Shark_Update(void *self);
00136390 void FoxHunt_UpdateSharks(void);
001363c0 void FoxHunt_ResetSharks(void);
00136440 void FoxHunt_LoadFlagModels(void);
00136470 void FoxHunt_DropFlag(int id);
001364c0 void FoxHunt_ResetFlags(void);
00136530 void FoxHunt_ResetFlag(int id);

# ---- FoxHunt/CFHunt_Flag.c
00136570 void *CFHunt_Flag_Ctor(void *self, float *pos, int a, int b);
00136890 int CFHunt_Flag_NearOther(void *self);
00136be0 void CFHunt_Flag_Respawn(void *self);

# ---- FoxHunt/unplaced_level.c
00136ef0 char *CFHunt_Flag_Update(void *self, float *target, int smoke);
00137940 void FoxHunt_UpdateFlags(float *target, int smoke);
00137980 void *FoxHunt_Glass_Ctor(void *self, float *pos, int type);
001379e0 void FoxHunt_UpdateGlass(void);
00137a00 char *FoxHunt_Glass_Hit(void *self, float *vel, float *pos);
00137cb0 void FoxHunt_HitGlass(int type, float *vel, float *pos);
00137d00 void *FoxHunt_Escalator_Ctor(void *self, float *pos, int direction, int user, int id);
00137e40 void FoxHunt_LoadEscalatorModels(void);
00137e70 char *FoxHunt_Escalator_Toggle(void *self);
00137f80 void FoxHunt_UpdateEscalators(void);
00137fa0 int FoxHunt_EscalatorDirection(int id);
00137fe0 void FoxHunt_ToggleEscalators(void);
00138090 void FoxHunt_LoadGateModels(void);
001380d0 int FoxHunt_NearestGate(int player);
00138150 void *FoxHunt_Gate_Ctor(void *self, float *pos, float unused, int a, int b, int c, int id);
001382a0 void FoxHunt_OpenGates(void);
00138320 char *FoxHunt_Gate_Update(void *self);
001385d0 void FoxHunt_GatePosition(float *out, int id);
00138670 void FoxHunt_UpdateGates(void);
001386a0 void FoxHunt_TextReader_Ctor(void *self);
001386b0 void FoxHunt_TextReader_Dtor(void *self);
001386c0 char *FoxHunt_TextReader_ReadLine(void *self, char *buffer);
00138720 void FoxHunt_TextReader_NextLine(void *self);
00138770 void *FoxHunt_TextReader_Destroy(void *self, unsigned char flags);
00138790 void *FoxHunt_CWaterField_Ctor(void *self);
