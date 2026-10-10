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

# ---- provisional shapes (written later in this module)
0013f590 void *CGameMode_Hopper_Ctor(void *self);
00143da0 int HopArena_InBlades(void *self, float x, float z, short state);
00144b40 int HopArena_InBlades3(void *self, float x, float z, short state);
00145cb0 void *HopPlayer_Ctor(void *self, int index, float *pos);
00145eb0 void HopPlayer_Dtor(void *self);
00145fb0 void HopPlayers_Start(void);
00148c90 void HopFx_PlayerEffect(void *self, int player, int kind);
