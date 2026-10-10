# src/rowing_a/rowing_a.decls - prototype overrides of the rowing_a module
# (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# Rowing/unplaced_particles.c (a RollerBall particle system class; vtable 0x00274e3c)
0019d200 void RbParticle_Emit(void *self, struct CParticle *p);
0019d400 void RbParticle_Update(void *self, float dt);
0019d510 void RbParticle_SetDirection(void *self, const float *dir);

# Rowing/CRowing_Game.c: game 10 (CGame, vtable 0x00274e74)
0019d530 void *CGameRowing_Ctor(void *self);
0019d560 void CGameRowing_Dtor(void *self);
0019d570 int CGameRowing_Init(void *self);
0019d580 void CGameRowing_Shutdown(void *self);
0019d590 int CGameRowing_Start(void *self, int variant);
0019d5a0 void *CGameRowing_DeletingDtor(void *self, unsigned int flags);

# Rowing/debugCam.c
0019d5c0 int DebugCam_Init(void);
0019d610 int DebugCam_InitWatch(int *toggle);
0019d630 int DebugCam_IsActive(void);
0019d640 void **DebugCam_Ctor(void **self);
0019d6e0 int DebugCam_Toggle(void);
0019d7d0 void DebugCam_GetPosition(float *out);
0019d810 void DebugCam_Shutdown(void);
0019d870 void DebugCam_Update(void);

# Rowing/CRowing_Game.c (the CRowing_Game object 0x0030f0b0)
0019d8d0 void FUN_0019d8d0(void *self);
0019d9b0 void FUN_0019d9b0(void *self, int player, int flag);
0019dad0 int FUN_0019dad0(void *self);

# callees whose bodies come later (shapes from the disassembly of their callers)
001af680 void FUN_001af680(void *self, unsigned int colour);
001a74a0 float *FUN_001a74a0(void *self);
001afaa0 void FUN_001afaa0(void *self, float x, float y, int r, int g, int b, int a, const char *format, ...);
001ae290 void FUN_001ae290(void *self);
001ae2f0 void FUN_001ae2f0(void *self);
001ae050 void FUN_001ae050(void *self, int flag);
001afd20 void FUN_001afd20(void *self);
001af570 void FUN_001af570(void *self);
001ada40 void FUN_001ada40(void *self);
001aab80 void FUN_001aab80(void *self, int detail);
001a5770 void FUN_001a5770(void *self);
0019eaa0 void FUN_0019eaa0(struct CWaterField *water, void *world, float x, float z);
0019ec50 void FUN_0019ec50(void *self, void *package);
0019f0f0 void FUN_0019f0f0(void *self, void *package);
0019f590 void FUN_0019f590(void *self, void *package);
0019fba0 void *FUN_0019fba0(void *self, unsigned int flags);
0019fbc0 void FUN_0019fbc0(void *self, void *package);
001a0260 void *FUN_001a0260(void *self, unsigned int flags);
001a0280 void FUN_001a0280(void *self);
001a43c0 void FUN_001a43c0(void *self);
001a0290 void CRowingGame_Load(void *self);
001a0dd0 void CRowingGame_Initialise(void *self);
001aff40 void *FUN_001aff40(void *self, int variant);
001afa50 void *FUN_001afa50(void *self);
001afa60 int FUN_001afa60(void *self, void *package);
001a5950 void FUN_001a5950(void);
001a57b0 void FUN_001a57b0(int a, int b, float c, float d, int e);
001a5750 void FUN_001a5750(void *self);
001abc70 int FUN_001abc70(void *self, void *package, void *genPackage, int index, int start, int handicap);
001af960 int FUN_001af960(void *self);
001add20 void *FUN_001add20(void *self);
001add40 int FUN_001add40(void *self, int variant);
001b12a0 void *FUN_001b12a0(void *self);
001af380 void *FUN_001af380(void *self);
001af3d0 int FUN_001af3d0(void *self);
