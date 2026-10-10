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
001a57b0 void FUN_001a57b0(float phase, float b, float amplitude, float speed, float e);
001a5750 void FUN_001a5750(void *self);
001abc70 int FUN_001abc70(void *self, void *package, void *genPackage, int index, int start, int handicap);
001af960 int FUN_001af960(void *self);
001add20 void *FUN_001add20(void *self);
001add40 int FUN_001add40(void *self, int variant);
001b12a0 void *FUN_001b12a0(void *self);
001af380 void *FUN_001af380(void *self);
001af3d0 int FUN_001af3d0(void *self);
001a1ac0 void CRowingGame_Begin(void *self);
001a1d60 void CRowingGame_EmitSteam(void *self);
001a1f20 void CRowingGame_UpdateCamera(void *self);
001a2130 void CRowingGame_UpdateCloseups(void *self);
001a2330 void CRowingGame_Abort(void *self);
001a23e0 void CRowingGame_Shutdown(void *self);
001a4b10 float FUN_001a4b10(void *water, float x, float z);
001b0f70 void FUN_001b0f70(void *self);
001a8940 void FUN_001a8940(void *self, int mode);
001adea0 void FUN_001adea0(void *self, float dt);
001aeb90 void FUN_001aeb90(void *self, float dt);
001ae2e0 void FUN_001ae2e0(void *self, float fov);
001a76e0 int FUN_001a76e0(void *self);
001abb70 void FUN_001abb70(void *self, int simple, int unused);
001adbf0 void FUN_001adbf0(void *self);
001af9a0 void FUN_001af9a0(void *self);
001a5970 void FUN_001a5970(void);
001af450 void FUN_001af450(void *self);
001a289c void FUN_001a289c(void);
001af460 void FUN_001af460(void *self);
001af540 void FUN_001af540(void *self, int section);
001a57f0 void FUN_001a57f0(void);
001ac8f0 void FUN_001ac8f0(void *self, float dt);
001b10a0 void FUN_001b10a0(void *self, float dt);
001ade80 void FUN_001ade80(void *self, int mode);
001b0e50 void FUN_001b0e50(void *self);
001a5610 void FUN_001a5610(void *water, float t);
001ae420 int FUN_001ae420(void *self);
001a3b00 int FUN_001a3b00(void *self);
001a3ee0 void FUN_001a3ee0(void *self, float x, float z, int *col, int *row);
001a4170 int FUN_001a4170(void *self, struct CWaterParams *params);
001a41b0 void FUN_001a41b0(void *self);
001a4290 int FUN_001a4290(void *self, struct CWaterVertex *v, float u0, float v0, float vScale, float x0, float y0, float z0, float x1, float y1, float z1, unsigned int c0, unsigned int c1);
001a43b0 int FUN_001a43b0(void *self, float height, int blend);
001a4460 int FUN_001a4460(void *self);
001a4700 int FUN_001a4700(void *self, float height, int blend);
001a49b0 int FUN_001a49b0(void *self, float x, float z, float depth, float inner, float outer);
001a4b70 float FUN_001a4b70(void *self, float x, float z, float *normal);
001a4c70 void FUN_001a4c70(void *self, unsigned char **cursor, float h, float z, float angle, float u, float *vpos, unsigned int colour);
001a4d80 void FUN_001a4d80(void *self, unsigned char **cursor, unsigned int colour, float h, float u, float vpos, int continued);
001a4ed0 void FUN_001a4ed0(void *self, unsigned char **cursor, unsigned int colour, float h, float u, float vpos, int continued);
001a5010 int FUN_001a5010(void *self, int blend);

# Rowing/CRowing_Water.c
001a5400 void FUN_001a5400(void *self);
001a56f0 int FUN_001a56f0(void *self, struct CWaterParams *params);
