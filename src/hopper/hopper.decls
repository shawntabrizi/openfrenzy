# hopper prototype overrides (docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;   Only of_types.h / C types and "struct Name *".

# ---- Hopper/unplaced_sparks.c (FoxHunt's sparks and lightning)
0013b140 void HopSparks_AddSpark(struct HopSparks *self, float *pos, float *vel, float ground, int red, int green, int blue, int alpha, float length);
0013b460 void HopSparks_Delete(struct HopSparks *self);
0013b480 void HopSparks_Update(struct HopSparks *self, float dt);
0013b4b0 void HopSparks_Draw(struct HopSparks *self);
0013b910 int HopSparks_Init(struct HopSparks *self, void *collide);

# ---- Hopper/CModule_Hopper.c
0013b990 unsigned int *CModule_Hopper_Ctor(unsigned int *self);
0013b9c0 void CModule_Hopper_Dtor(unsigned int *self);
0013b9d0 unsigned int *CModule_Hopper_Destroy(unsigned int *self, byte flags);
0013b9f0 int CModule_Hopper_Start(unsigned int *self, int variation);

# ---- Hopper/CHopper_Antics.c (the antics object 0x002ae878; self = that object)
0013bb70 void HopAntics_DrawSkeleton(struct BActorInstance *inst, struct BActorNode *node, int alpha, int red, int green, int blue);
0013bc90 void HopAntics_DrawRing1(int player, float *pos);
0013be10 void *HopAntics_Destroy(void *self, byte flags);
0013be30 void HopAntics_PlaceActor(void *self, void *actor);
0013bfc0 void HopAntics_Reset(void *self);
0013c360 void HopAntics_Start(void *self);
0013c8e0 void HopAntics_Update1(void *self);
0013ce90 void HopAntics_Update2(void *self);
0013d500 void HopAntics_Update3(void *self);
0013d9f0 void HopAntics_Update4(void *self);
0013de00 void HopAntics_Update(void *self);
0013de40 void HopAntics_DrawRing4(int player, float *pos);
0013e1a0 void HopAntics_DrawRing3(int player, float *pos);
0013e360 void HopAntics_DrawSpiral(void *self);
0013e510 void HopAntics_Draw(void *self);
0013ea80 void HopAntics_Shutdown(void *self);

# ---- Hopper/CHopper_Game.c (the game object 0x00302798)
0013eae0 void *HopGame_Ctor(void *self);
0013eb00 void HopGame_Dtor(void *self);
0013eb10 void HopGame_Enter(void *self, int variation);
0013eb90 void HopGame_Start(void *self);
0013ed10 void HopGame_Exit(void *self);
0013ede0 void HopGame_CheckPlayers(void *self);
0013f2c0 void HopGame_Restart(void *self);
0013f380 void *HopGame_Destroy(void *self, byte flags);
0013f3a0 void HopGame_Init(void *self);

# ---- Hopper/CHopper_Resources.c (the game mode, the resources 0x002ae948)
0013f590 void *CGameMode_Hopper_Ctor(void *self);
0013f5b0 void CGameMode_Hopper_Dtor(void *self);
0013f5c0 void CGameMode_Hopper_Enter(void *self);
0013f5f0 void CGameMode_Hopper_Resume(void *self);
0013f600 void CGameMode_Hopper_Suspend(void *self);
0013f610 void CGameMode_Hopper_Update(void *self, int top);
0013f620 void CGameMode_Hopper_Process(void *self, int top);
0013f630 void CGameMode_Hopper_Draw(void *self, int top);
0013f640 void CGameMode_Hopper_Init(void *self);
0013f650 void CGameMode_Hopper_Start(void *self);
0013f660 void *CGameMode_Hopper_Destroy(void *self, byte flags);
0013f680 void CGameMode_Hopper_Exit(void *self);
0013f710 void HopRes_SetRenderStates(void *self);
0013f7e0 void HopRes_Load1(void *self);
0013fd50 void HopRes_Load2(void *self);
00140180 void HopRes_Load3(void *self);
00140680 int HopRes_LoadLOM(void *self, struct bkPackage *package, const char *name);
00140710 void *HopRes_Destroy(void *self, byte flags);
00140730 void HopRes_Load4(void *self);
00140c70 void HopRes_Load(void *self);
001414a0 void HopRes_Free(void *self);

# ---- Hopper/unplaced_camera.c (the camera 0x002aebd8)
00141920 void HopCamera_Start(void *self);
00141960 void HopCamera_Apply(void *self);
00141a20 void *HopCamera_Destroy(void *self, byte flags);
00141a40 void HopCamera_Play1(void *self);
00141ac0 void HopCamera_Play2(void *self);
00141b40 void HopCamera_Play3(void *self);
00141bc0 void HopCamera_Play4(void *self);
00141f00 void HopCamera_EaseIn(void *self);
00142200 void HopCamera_Update(void *self);

# ---- Hopper/unplaced_arena.c (the arena 0x002aec10)
00142760 void HopArena_Start(void *self);
001428e0 void HopArena_Update1(void *self, int fast);
00142cd0 void HopArena_DropNext(void *self);
00142d50 void HopArena_Update4(void *self, int fast);
001430a0 void HopArena_DrawExtras4(void *self);
001431e0 void HopArena_Shutdown(void *self);
00143220 void *HopArena_Destroy(void *self, byte flags);
00143240 void HopArena_Update2(void *self, int fast);
001436e0 void HopArena_Wobble3(void *self);
001438c0 void HopArena_Update3(void *self, int fast);
00143c60 void HopArena_Update(void *self, int fast);
00143ca0 char HopArena_WhichBlade1(void *self, float x, float z);
00143da0 int HopArena_InBlades(void *self, float x, float z, short state);
00143ed0 void HopArena_DrawFloor1(void *self);
00143fa0 void HopArena_Draw1(void *self, int fast);
001441c0 void HopArena_Draw2(void *self, int fast);
001445d0 void HopArena_DrawExtras2(void *self);
001447e0 int HopArena_WhichBlade3(void *self, float x, float z);
00144b40 int HopArena_InBlades3(void *self, float x, float z, short state);
00144d90 void HopArena_Draw3(void *self, int fast);
00144fd0 void HopArena_DrawExtras3(void *self);
00145920 void HopArena_Draw4(void *self, int fast);
00145c30 void HopArena_Draw(void *self, int fast);

# ---- provisional shapes (written later in this module)
0014c970 void HopShader_SetConstant(float r, float g, float b, float a);
001472d0 void HopBubbles_Start(void *self);
00147380 void HopBubbles_Update(void *self);
0014a940 void HopFx_Start(void *self);
00148c80 void HopFx_Shutdown(void *self);
0014b090 void HopFx_Update(void *self);
00148c40 void HopFx_Draw(void *self);
0014b100 void HopTimers_Start(void *self);
0014b340 void HopTimers_Begin(void *self, int index, const char *name);
0014b380 void HopTimers_End(void *self, int index);
0014b6e0 void HopTimers_Update(void *self);
0014c800 void HopTimers_DrawOverlay(void *self);
0014b3a0 void HopTimers_Draw(void *self);
0014c880 void HopOverlay_Resume(void *self);
0014c8f0 void *HopShaderColour_Ctor(void *self);
0014c950 void *HopShaderLit_Ctor(void *self);
00145cb0 void *HopPlayer_Ctor(void *self, int index, float *pos);
00145eb0 void HopPlayer_Dtor(void *self);
00145fb0 void HopPlayers_Start(void);
00148c90 void HopFx_PlayerEffect(void *self, int player, int kind);
