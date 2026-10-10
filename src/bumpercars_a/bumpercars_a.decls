# bumpercars_a prototype overrides (docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;   Only of_types.h / C types and "struct Name *".

# ---- ClassLib/Debris.c
00080d00 void Debris_UpdateSystems(float dt);
00080d30 void Debris_StopPiece(struct DebrisPiece *piece);
00080d60 void *Debris_Add(struct DebrisType *type, float *pos, float *vel, int lifeFrames, unsigned int colour);
00081210 void Debris_SpawnDeath(struct DebrisPiece *piece);
000815f0 int Debris_Collide(struct DebrisPiece *piece, float *delta);
00081880 void Debris_AddRing(struct DebrisType *type, int count, float *pos, float outSpeed, float upSpeed, float jitter, int lifeFrames, unsigned int colour);
00081a30 void Debris_AddCone(struct DebrisType *type, int count, float *pos, float *dir, float radius, float speed, float outSpeed, float speedRandom, float outRandom, int lifeFrames, unsigned int colour);
00081cb0 void Debris_AddSphere(struct DebrisType *type, int count, float step, float *pos, float speed, float speedRandom, int lifeFrames);
00081ed0 void Debris_AddFountain(struct DebrisType *type, float *pos, int count, float speed, float speedRandom, unsigned int colour);
000827f0 void Debris_Init(void *ground, int maxItems);

# ---- ClassLib/DebrisSys.c
000868f0 void DebrisSys_UpdateWind(struct DebrisSysData *data);
000869d0 void DebrisSys_SetWindDirection(struct DebrisSysData *data, float *dir);
00086a00 int DebrisSys_SwirlForce(float *out, struct DebrisSysData *data, float *pos);
00086b70 int DebrisSys_InitSystemWind(struct BExtraSystem *system);

# ---- BumperCars/CBCar_Game.c
00086b80 void *CBCar_Game_PlayerSlotCtor(void *slot);
00086ba0 void CBCar_Game_Dtor(struct CBCar_Game *self);
00086d10 void CBCar_Game_Start(struct CBCar_Game *self);
00086f00 void CBCar_Game_Pause(struct CBCar_Game *self);
00086f40 void CBCar_Game_Unpause(struct CBCar_Game *self);
00086fa0 void CBCar_Game_DrawScene(struct CBCar_Game *self, int view, int pass);
00087140 void CBCar_Game_ClearSettings(struct CBCar_Game *self);
00087200 void CBCar_Game_RankPlayers(struct CBCar_Game *self);
000872b0 struct CBCar_Game *CBCar_Game_Ctor(struct CBCar_Game *self);
000873e0 struct CBCar_Game *CBCar_Game_Destroy(struct CBCar_Game *self, byte flags);
00087400 int CBCar_Game_PushMode(struct CBCar_Game *self, int variation);
00087800 int CBCar_Game_LoadSettings(struct CBCar_Game *self, struct BCarTextReader *reader);
00087b60 void CBCar_Game_EndRound(struct CBCar_Game *self);
00087e30 void CBCar_Game_RankCars(struct CBCar_Game *self, int active);
00088330 void CBCar_Game_Initialise(struct CBCar_Game *self);
00088a10 void CBCar_Game_Shutdown(struct CBCar_Game *self);
00088b70 int CBCar_Game_Process(struct CBCar_Game *self, int top);

# ---- prototypes of functions not written yet that CBCar_Game calls (the shape the x86
# code uses: ECX `this` first); each is checked when its function is written
00082860 struct Settings *Settings_Init(struct Settings *self);
00082870 char *Settings_Get(struct Settings *self, const char *key);
00089020 void *FUN_00089020(void *self);
00089070 void FUN_00089070(void *self);
000890f0 void FUN_000890f0(void *self);
0008bf50 void *FUN_0008bf50(void *self);
0008bf90 void FUN_0008bf90(void *self);
0008bfa0 void FUN_0008bfa0(void *self, int playing);
0008c000 int FUN_0008c000(void *self);
0008c050 int FUN_0008c050(void *self);
0008c060 unsigned int FUN_0008c060(void *self, int which);
0008c0c0 int FUN_0008c0c0(void *self);
0008c9f0 void FUN_0008c9f0(void *self);
0008cae0 void FUN_0008cae0(void *self);
0008cb60 void FUN_0008cb60(void *self);
0008cb80 void FUN_0008cb80(void *self, float dt);
0008cb90 void FUN_0008cb90(void *self, int view);
0008cdc0 int FUN_0008cdc0(void *self);
0008d1e0 void *FUN_0008d1e0(void *self);
0008d1ee void FUN_0008d1ee(void *self);
0008d1f9 int FUN_0008d1f9(void *self, struct bkPackage *package, char *file);
0008d81d int FUN_0008d81d(void *self);
0008d91d void FUN_0008d91d(void *self);
0008e7f0 void FUN_0008e7f0(void *self);
0008e8d1 float FUN_0008e8d1(void *self);
0008ea74 void FUN_0008ea74(void *self);
0008eceb void FUN_0008eceb(void *self, int playing, float dt);
0008fc39 void FUN_0008fc39(void *self);
0008fe4b int FUN_0008fe4b(void *self, int player);
00091287 void FUN_00091287(void *self);
00091422 void FUN_00091422(void *self, int view, int a3, float a4);
0009194d int FUN_0009194d(void *self, int *view);
000919e0 void *FUN_000919e0(void *self);
000919f0 void FUN_000919f0(void *self);
00091b30 void FUN_00091b30(void *self);
00091d70 void FUN_00091d70(void *self);
00091db0 void FUN_00091db0(void *self, int playing);
00091e30 int FUN_00091e30(void *self, struct bkPackage *package, char *file);
00091ef0 void *FUN_00091ef0(void *self);
00091fd0 int FUN_00091fd0(void *self);
000920b0 void FUN_000920b0(void *self);
00092120 int FUN_00092120(void *self, int sample, int volume, int a4, int a5);
00092340 void FUN_00092340(void *self, float dt);
000923d0 void FUN_000923d0(void *self);
000924e0 void *FUN_000924e0(void *self);
000924f0 void FUN_000924f0(void *self);
00092680 void FUN_00092680(void *self, byte view);
00092740 void FUN_00092740(void *self, float *pos);
000927f0 void FUN_000927f0(void *self);
00092a70 void FUN_00092a70(void *self, int playing, float dt);
00094240 int FUN_00094240(void *self, struct bkPackage *package, char *file);
00094360 void *FUN_00094360(void *self);
00094950 void FUN_00094950(void *self, int a2);

# ---- BumperCars/CModule_BumperCars.c
000828d0 unsigned int *CModule_BumperCars_Ctor(unsigned int *self);
00082900 void CModule_BumperCars_Dtor(unsigned int *self);
00082910 int CModule_BumperCars_Init(unsigned int *self);
00082920 void CModule_BumperCars_Shutdown(unsigned int *self);
00082930 unsigned int *CModule_BumperCars_Destroy(unsigned int *self, byte flags);
00082950 int CModule_BumperCars_Start(unsigned int *self, int variation);

# ---- BumperCars/unplaced.c
00082a00 void DebrisColours_Clear(void);
00082a20 void DebrisColours_Set0(float *colour);
00082a60 void DebrisColours_Set1(float *colour);
00082aa0 void DebrisColours_Set2(float *colour);
00082ae0 void DebrisColours_Set3(float *colour);
00082b20 void DebrisColours_SetAll(float *colour0, float *colour1, float *colour2, float *colour3);
