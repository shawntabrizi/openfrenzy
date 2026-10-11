# src/core/core.decls - prototype overrides of the core module (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# Core/core.cpp
0002c8e0 int Core_CreateGames(void);
0002cc90 void Core_UpdateFrameTime(void);
0002cd80 void Core_Init(void);

# Core/unplaced.c: start-up helpers near core.cpp
0002d540 void Core_Main(void);
0002c020 void Core_UpdatePads(void);
0002c270 void Core_CreateDebugInput(void);
0002c2e0 void Core_ResetToStart(void);
0002c3f0 void Core_ResetToStart_Thunk(void);
0002c400 void Core_ReadVideoSettings(void);
0002c460 void Core_PollSoundChannel(void);
0002c520 void Core_ReadLaunchInfo(void);
0002c5a0 void Core_HeapStats(void);
0002c5b0 void Core_PrintMemoryStatus(struct core_memory_status *status);
0002c680 int Core_OpenDisplay(void);

# Core/unplaced.c: CGameModeStack (the stack at 0x002ba750)
0002b470 void CGameModeStack_Update(struct CGameModeStack *self);
0002b4b0 void CGameModeStack_Process(struct CGameModeStack *self);
0002b520 void CGameModeStack_PostDraw(struct CGameModeStack *self);
0002b560 void CGameModeStack_Show(struct CGameModeStack *self);
0002b5b0 struct CGameMode *CGameModeStack_Find(struct CGameModeStack *self, const char *name);
0002b5f0 void CGameModeStack_ShowTop(void);
0002b640 int CGameModeStack_Push(struct CGameModeStack *self, struct CGameMode *mode, int resume);
0002b6f0 int CGameModeStack_Pop(struct CGameModeStack *self, int resume);
0002b790 int CGameModeStack_PopTo(struct CGameModeStack *self, const char *name, int resume);
0002b870 void CGameModeStack_PopAll(struct CGameModeStack *self);
0002b8a0 void CGameModeStack_Draw(struct CGameModeStack *self);

# Core/unplaced.c: cheats and character colours
0002b8f0 void Core_CheatInputCreate(int player);
0002b9d0 void Core_CheatInputFree(void);
0002b9e0 void Core_CheatMessageState(int cheat, int state);
0002ba70 void Core_SetBoneColour(void *actor, const char *name, int index, int set);
0002bb00 void Core_SetCharacterColours(void *actor, int set);
0002bc90 void Core_CheatOn(int cheat);
0002bd20 void Core_CheatOff(int cheat);
0002bd90 void Core_CheatToggle(int cheat);
0002be10 void Core_CheatUpdate(void);

# Core/unplaced.c: the CGame base class (vtable 0x00256b18) and the gain curve
0002bf10 void CGame_Ctor(unsigned int *self, const char *name);
0002bf30 void CGame_Ctor6(unsigned int *self, const char *name, unsigned int a, unsigned int b, unsigned int c, unsigned int d);
0002bf60 void CGame_Dtor(unsigned int *self);
0002bf70 unsigned int *CGame_Destroy(unsigned int *self, int flags);
0002bf90 float Core_Gain(float gain, float t);

# Core/CTweakMenu.c
0002e290 void CTweakMenu_SkipWords(char **cursor, int count);
0002e2f0 void CTweakMenu_SaveAll(void);
0002e510 struct CTweakMenu *CTweakMenu_Ctor(struct CTweakMenu *self, const char *name);
0002e610 struct CTweakMenu *CTweakMenu_CtorSub(struct CTweakMenu *self, struct CTweakMenu *parent, const char *name);
0002e710 void CTweakMenu_LoadValue(struct CTweakMenu *self, const char *name, int *size, float *fvar, int *ivar, int *bvar);
0002e910 struct CTweakItem *CTweakItem_CtorFloat(struct CTweakItem *self, struct CTweakMenu *menu, const char *name, float *var, float step, float min, float max);
0002eaa0 struct CTweakItem *CTweakItem_CtorInt(struct CTweakItem *self, struct CTweakMenu *menu, const char *name, int *var, int step, int min, int max, int nlabels, const char **labels);
0002ece0 struct CTweakItem *CTweakItem_CtorBool(struct CTweakItem *self, struct CTweakMenu *menu, const char *name, int *var);
0002ed90 int CTweakMenu_HandleSelect(int id, int selected);
0002f1a0 struct CTweakItem *CTweakMenu_AddFloat(struct CTweakMenu *self, const char *name, float *var, float step, float min, float max);
0002f250 void CTweakMenu_AddInt(struct CTweakMenu *self, const char *name, int *var, int step, int min, int max, int nlabels, ...);
0002f340 struct CTweakItem *CTweakMenu_AddBool(struct CTweakMenu *self, const char *name, int *var);
0002f3e0 void CTweakMenu_Dtor(struct CTweakMenu *self);

# Core/CGameMode_Root.c (vtable 0x00256010 is built by core_modes_b with the constructor)
00027540 void CGameMode_Root_CreateDebugMenu(void);
00027db0 struct CGameMode_Root *CGameMode_Root_Destroy(struct CGameMode_Root *self, int flags);
00027dd0 void CGameMode_Root_Update(struct CGameMode_Root *self, int top);
00027f10 void CGameMode_Root_Pushed(struct CGameMode_Root *self);
000283a0 void CGameMode_Root_Popped(struct CGameMode_Root *self);

# Core/CGameMode_Scores.c (vtable 0x002565d0)
000284e0 struct CGameMode_Scores *CGameMode_Scores_Ctor(struct CGameMode_Scores *self, int screen);
00028580 void CGameMode_Scores_Dtor(struct CGameMode_Scores *self);
000285f0 void CGameMode_Scores_Update(struct CGameMode_Scores *self, int top);
00028650 int CGameMode_Scores_Button(struct CGameMode_Scores *self, int button, int player);
000286b0 void CGameMode_Scores_DrawBackground(struct CGameMode_Scores *self);
00028950 struct CGameMode_Scores *CGameMode_Scores_Destroy(struct CGameMode_Scores *self, int flags);
00028970 void CGameMode_Scores_Popped(struct CGameMode_Scores *self);
00028ab0 void CGameMode_Scores_ComputePoints(int *winner, int *points, int final);
00028df0 void CGameMode_Scores_CreateTexts(struct CGameMode_Scores *self);
00029190 void CGameMode_Scores_CreateWinners(struct CGameMode_Scores *self);
00029440 void CGameMode_Scores_Draw(struct CGameMode_Scores *self, int top);
00029640 void CGameMode_Scores_Loaded(struct CGameMode_Scores *self);
00029800 struct ScoreOrb *CGameMode_Scores_AddOrb(struct CGameMode_Scores *self, int player, float x, float y);
00029900 void CGameMode_Scores_Pushed(struct CGameMode_Scores *self);
00029c10 void CGameMode_Scores_Process(struct CGameMode_Scores *self, int top);

# Core/menus_common.c (CMenuMode, vtable 0x002577d0)
0002fe60 void CMenuMode_Update(struct CMenuMode *self, int top);
00030090 struct CMenuMode *CMenuMode_Ctor(struct CMenuMode *self, const char *name);
00030190 void CMenuMode_Dtor(struct CMenuMode *self);

# Core/CGameMode_Team.c (vtable 0x0025667c)
0002a5c0 struct CGameMode_Team *CGameMode_Team_Ctor(struct CGameMode_Team *self);
0002fe6c void FUN_0002fe6c(int *self);
0002ff40 void CMenuMode_ActorOut(struct CMenuMode *self, const char *anim);
0002ffd0 void CMenuMode_FreeActors(void);
00030000 void CMenuMode_Process(int top);
00030210 void CMenuMode_SetActor(struct CMenuMode *self, void *model, const char *anim);
00030310 void CMenuMode_Draw(struct CMenuMode *self, int top);
00030460 struct CMenuMode *CMenuMode_Destroy(struct CMenuMode *self, int flags);
00030480 int Core_WriteLaunchData(const char *path, const char *device, int type, uint flags, const uint *data);

# Core/CVectorShape.c
0002f4a0 void CVectorShape_Draw(struct CVectorShape *self);
0002f500 void CVectorShape_DrawPart(struct CVectorShape *self, unsigned int count);
0002f570 struct CVectorShape *CVectorShape_Init(struct CVectorShape *self);
0002f580 void CVectorShape_Free(struct CVectorShape *self);
0002f5a0 void CVectorShape_Set(struct CVectorShape *self, const int *data, const int *colour);
0002f710 void CVectorShape_BuildFan(struct CVectorShape *self, const float *centre, int *vb, const int *colour, const int *glow);

# Core/unplaced.c: game state
0002f930 void Core_SelectLanguage(void);
0002fa20 void Core_InitGameState(void);
0002fb50 void Core_ChooseStages(int count);
0002fbf0 void Core_CreatePlayerInput(int player);
0002a650 void CGameMode_Team_Popped(struct CGameMode_Team *self);
0002a690 void CGameMode_Team_Draw(struct CGameMode_Team *self, int top);
0002a720 int CGameMode_Team_Button(struct CGameMode_Team *self, int button, int player);
0002a790 void CGameMode_Team_PlayAnim(struct CGameMode_Team *self, int player, int anim, int loop, int keep, float frame, float blend);
0002a8b0 struct CGameMode_Team *CGameMode_Team_Destroy(struct CGameMode_Team *self, int flags);
0002a8d0 void CGameMode_Team_Dtor(struct CGameMode_Team *self);
0002a930 void CGameMode_Team_Pushed(struct CGameMode_Team *self);
0002ad30 void CGameMode_Team_Loaded(struct CGameMode_Team *self);
0002aeb0 void CGameMode_Team_Shuffle(struct CGameMode_Team *self);
0002b0a0 void CGameMode_Team_Process(struct CGameMode_Team *self, int top);

# Core/CSpinCube.c (CSpinCube 0x00257604, CMapCube 0x0025760c, CSelectCube 0x00257680)
0002d560 void CSpinCube_BuildFaces(struct CSpinCube *self);
0002d830 void CSpinCube_Draw(struct CSpinCube *self);
0002da70 void CSelectCube_Set(struct CSpinCube *self, int unused1, int unused2, void *picture);
0002da90 void CSelectCube_Turn(struct CSpinCube *self, int unused1, int unused2, void *picture, char forward);
0002db40 void CSelectCube_Update(struct CSpinCube *self);
0002dc10 struct CSpinCube *CSpinCube_Destroy(struct CSpinCube *self, int flags);
0002dc30 struct CSpinCube *CSpinCube_Ctor(struct CSpinCube *self);
0002dcf0 void CMapCube_Dtor(struct CSpinCube *self);
0002dd50 void CMapCube_FindValidGames(struct CSpinCube *self, int zone, int depth);
0002dff0 void CMapCube_Update(struct CSpinCube *self);
0002e0d0 struct CSpinCube *CSelectCube_Ctor(struct CSpinCube *self);
0002e160 void CSelectCube_Dtor(struct CSpinCube *self);
0002e1b0 struct CSpinCube *CMapCube_Ctor(struct CSpinCube *self, int zone);
0002e250 struct CSpinCube *CMapCube_Destroy(struct CSpinCube *self, int flags);
0002e270 struct CSpinCube *CSelectCube_Destroy(struct CSpinCube *self, int flags);
