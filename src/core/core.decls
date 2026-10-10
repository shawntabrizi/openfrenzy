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
