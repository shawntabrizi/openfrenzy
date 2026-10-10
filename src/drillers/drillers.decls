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
0011c7e0 int FUN_0011c7e0(void *self);
0011e140 void FUN_0011e140(void *self);
0011e1e0 void FUN_0011e1e0(void *self);
0011f770 void FUN_0011f770(void *self);
0011fd50 void FUN_0011fd50(void *self);
00120260 void FUN_00120260(void *self);
001202c0 void FUN_001202c0(void *self);
00120a70 void FUN_00120a70(void *self);
00121a80 void FUN_00121a80(void *self);

# Drillers/CDrillers_Particle.cpp (the three particle systems; DrParticleSystem in drillers.h)
00123540 void DrSparks_Dtor(struct DrParticleSystem *self);
00123550 void DrSparks_Draw(struct DrParticleSystem *self);
00123730 void DrPs_Burst(struct DrParticleSystem *self, int colour);
00123770 void DrDust_Ctor(struct DrParticleSystem *self);
00123780 void DrDust_Dtor(struct DrParticleSystem *self);
00123790 void DrDust_Draw(struct DrParticleSystem *self);
00123930 void DrSmoke_Ctor(struct DrParticleSystem *self);
00123940 void DrSmoke_Dtor(struct DrParticleSystem *self);
00123950 void DrSmoke_Draw(struct DrParticleSystem *self);
00123af0 void DrPs_Stop(struct DrParticleSystem *self);
00123b00 void DrSparks_Ctor(struct DrParticleSystem *self);
00123b10 struct DrParticleSystem *DrSparks_DeletingDtor(struct DrParticleSystem *self, unsigned int flags);
00123b30 int DrSparks_Init(struct DrParticleSystem *self, const struct DrPsParams *params, int count);
00123be0 void DrSparks_InitParticle(struct DrParticleSystem *self, struct CParticle *p, int colour);
00123fb0 void DrSparks_Update(struct DrParticleSystem *self, float dt);
00124110 struct DrParticleSystem *DrDust_DeletingDtor(struct DrParticleSystem *self, unsigned int flags);
00124130 int DrDust_Init(struct DrParticleSystem *self, const struct DrPsParams *params, int count);
001241e0 void DrDust_InitParticle(struct DrParticleSystem *self, struct CParticle *p, int colour);
001244c0 void DrPs_UpdateSprites(struct DrParticleSystem *self, float dt);
001245d0 struct DrParticleSystem *DrSmoke_DeletingDtor(struct DrParticleSystem *self, unsigned int flags);
001245f0 void DrSmoke_InitParticle(struct DrParticleSystem *self, struct CParticle *p, int colour);
00124910 void DrPs_Release(struct DrParticleSystem *self);
00124bc0 void DrRain_Create(struct BExtraSystem **slot, int count);

# Drillers/CDrillers_Effects.cpp (the effects object 0x00300c78; DrEffects in drillers.h)
001115f0 struct DrFxGroup *DrFxGroup_Ctor(struct DrFxGroup *self);
00111650 void DrFxGroup_Dtor(struct DrFxGroup *self);
001116b0 void DrEffects_Dtor(struct DrEffects *self);
00111720 void DrEffects_InitSparks(struct DrEffects *self);
00111750 void DrEffects_ReleaseSparks(struct DrEffects *self);
001117d0 void DrEffects_SparkBurst(struct DrEffects *self, float *pos);
00111830 void DrFxGroup_Init(struct DrFxGroup *self);
00111850 void DrFxGroup_Release(struct DrFxGroup *self);
00111870 void DrFxGroup_Update(struct DrFxGroup *self, float dt);
001118d0 void DrFxGroup_Draw(struct DrFxGroup *self);
001118f0 void DrFxGroup_Stop(struct DrFxGroup *self);
00112940 void DrEffects_CreateGen(struct DrEffects *self);
00112980 void DrFx_AddScoreFloaty(float x, float z, char type, char player);
00112a40 void DrEffects_UpdateMarkers(struct DrEffects *self);
00112c20 void DrEffects_CreateScreen(struct DrEffects *self);
00112c90 void DrEffects_DeleteScreen(struct DrEffects *self);
00112cb0 void DrFx_Rumble(int player, int kind);
00112d40 struct DrEffects *DrEffects_Ctor(struct DrEffects *self);
00112dc0 struct DrEffects *DrEffects_DeletingDtor(struct DrEffects *self, unsigned int flags);
00112de0 void DrFxGroup_Start(struct DrFxGroup *self, float *pos, int player);
001136a0 int FUN_001136a0(struct BExtraSystem *system, void *item, void *args);
00113810 int FUN_00113810(struct BExtraSystem *system, int count, void **items, void *arg);
001139a0 void DrEffects_Init(struct DrEffects *self);
00113fd0 void DrEffects_AddTokenAt(struct DrEffects *self, float x, float z);
00114230 void DrEffects_AddRandomToken(struct DrEffects *self);
001145d0 void DrEffects_UpdateTokens(struct DrEffects *self);

# Drillers/unplaced_map.c (the map 0x00298da0; DrMap in drillers.h)
001148f0 void DrMap_FillCell(struct DrMap *self, char x, char z);
00114990 void DrMap_Build(struct DrMap *self);
00115410 char DrMap_CheckHeld(struct DrMap *self, char x, char z);
00115710 void DrMap_Free(struct DrMap *self);
00115740 struct DrMap *DrMap_DeletingDtor(struct DrMap *self, unsigned int flags);

# Drillers/unplaced_players.c (the players object 0x00298e70)
00116970 void DrPlayers_Reset(void *self, char player);
00116be0 void DrPlayers_StartLoop(void *self, int count, int player);
00116cf0 void DrPlayers_UpdateLoops(void *self);
00118f00 void DrPlayers_DrawWinner(void *self, char player);
00117440 void *DrPlayers_DeletingDtor(void *self, unsigned int flags);

# Drillers/unplaced_ai.c (the AI object 0x002aadf8)
00121920 float DrAi_AngleDiff(void *self, float a, float b);
00121990 char DrAi_InPolygon(void *self, float px, float pz, int n, const float *poly);
