# src/drillers/drillers.decls - prototype overrides of the drillers module
# (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# Drillers/CModule_Drillers.cpp
0010f830 void *CModuleDrillers_Ctor(void *self);
0010f860 void CModuleDrillers_Dtor(void *self);
0010f870 void *CModuleDrillers_DeletingDtor(void *self, unsigned int flags);
0010f890 int CModuleDrillers_Start(void *self, int variant);

# Drillers/CDrillers_Game.cpp (CGameMode_Drillers; inferred)
001114a0 void *CGameModeDrillers_Ctor(void *self);
001114c0 void CGameModeDrillers_Dtor(void *self);
001114d0 void CGameModeDrillers_Pushed(void *self);
00111500 void CGameModeDrillers_Suspend(void *self);
00111510 void CGameModeDrillers_Input(void *self, int top);
00111520 void CGameModeDrillers_Update(void *self, int top);
00111530 void CGameModeDrillers_Render(void *self, int top);
00111540 void CGameModeDrillers_LoadingStarted(void *self);
00111550 void CGameModeDrillers_Start(void *self);
00111560 void *CGameModeDrillers_DeletingDtor(void *self, unsigned int flags);
00111580 void CGameModeDrillers_Popped(void *self);

# Drillers/CDrillers_Game.cpp (CDrillers_Game, the static object 0x002ff9f0; inferred)
0010f9c0 void *CDrillersGame_Ctor(void *self);
0010f9e0 void CDrillersGame_Dtor(void *self);
0010fa00 void CDrillersGame_Reset(void *self);
0010fc00 void CDrillersGame_Start(void *self);
0010fdc0 void CDrillersGame_NewRound(void *self);
0010fe50 void CDrillersGame_Load(void *self, int variant);
0010fef0 void CDrillersGame_Unload(void *self);
001101b0 void CDrillersGame_CheckColours(void *self);
00110940 void *CDrillersGame_DeletingDtor(void *self, unsigned int flags);
00110960 void CDrillersGame_Initialise(void *self);
00110b90 void CDrillersGame_UpdateSnare(void *self);

# callees (prototypes from the disassembly; renamed when their file is written)
00111720 void FUN_00111720(void *self);
00111830 void FUN_00111830(void *self);
00111870 void FUN_00111870(void *self, float dt);
001118d0 void FUN_001118d0(void *self);
001118f0 void FUN_001118f0(void *self);
00112940 void FUN_00112940(void *self);
00112a40 void FUN_00112a40(void *self);
00112c20 void FUN_00112c20(void *self);
00112c90 void FUN_00112c90(void *self);
001139a0 void FUN_001139a0(void *self);
00114230 void FUN_00114230(void *self);
001145d0 void FUN_001145d0(void *self);
00114990 void FUN_00114990(void *self);
00115410 char FUN_00115410(void *self, char x, char y);
00115710 void FUN_00115710(void *self);
00116970 void FUN_00116970(void *self, int player);
00116cf0 void FUN_00116cf0(void *self);
00118f00 void FUN_00118f00(void *self, int player);
0011c7e0 int FUN_0011c7e0(void *self);
0011e140 void FUN_0011e140(void *self);
0011e1e0 void FUN_0011e1e0(void *self);
0011f770 void FUN_0011f770(void *self);
0011fd50 void FUN_0011fd50(void *self);
00120260 void FUN_00120260(void *self);
001202c0 void FUN_001202c0(void *self);
00120a70 void FUN_00120a70(void *self);
00121a80 void FUN_00121a80(void *self);
