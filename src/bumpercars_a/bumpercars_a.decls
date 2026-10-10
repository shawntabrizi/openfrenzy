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

# ---- BumperCars/unplaced_ai.c (the computer driver, class CBCarAI at player slot + 0x10)
00088f80 struct DebrisType *Debris_FindType(const char *name, struct DebrisType *types, int count);
00089020 struct CBCarAI *CBCarAI_Ctor(struct CBCarAI *self);
00089070 void CBCarAI_Dtor(struct CBCarAI *self);
000890d0 void CBCarAI_DestroyTimers(struct CBCarAI *self);
000890f0 void CBCarAI_Reset(struct CBCarAI *self);
00089130 unsigned char *CBCarAI_FindNearest(struct CBCarAI *self, int kind);
00089280 unsigned char *CBCarAI_FindNearestFlashing(struct CBCarAI *self);
000893c0 unsigned char *CBCarAI_FindFurthest(struct CBCarAI *self);
00089510 unsigned char *CBCarAI_FindNearestToGoal(struct CBCarAI *self);
00089670 unsigned char *CBCarAI_FindNearestPuck(struct CBCarAI *self);
00089710 void CBCarAI_Pause(struct CBCarAI *self);
00089730 void CBCarAI_Resume(struct CBCarAI *self);
00089750 struct CBCarAI *CBCarAI_Destroy(struct CBCarAI *self, byte flags);
00089770 int CBCarAI_Init(struct CBCarAI *self, unsigned char *player, unsigned int *behaviours);
00089910 unsigned char *CBCarAI_FindBlocker(struct CBCarAI *self);
00089b30 void CBCarAI_Chase(struct CBCarAI *self);
00089ca0 void CBCarAI_Circle(struct CBCarAI *self);
00089e40 int CBCarAI_Ram(struct CBCarAI *self);
0008a070 void CBCarAI_Flee(struct CBCarAI *self);
0008a1e0 void CBCarAI_GotoWaypoint(struct CBCarAI *self);
0008a270 void CBCarAI_FollowWaypoints(struct CBCarAI *self);
0008a740 void CBCarAI_AvoidFlames(struct CBCarAI *self);
0008a8b0 void CBCarAI_AvoidHoles(struct CBCarAI *self);
0008ab20 void CBCarAI_AvoidWalls(struct CBCarAI *self);
0008ad90 void CBCarAI_AvoidGoal(struct CBCarAI *self);
0008ae80 void CBCarAI_FindTarget(struct CBCarAI *self);
0008b0a0 void CBCarAI_RethinkTarget(struct CBCarAI *self);
0008b1e0 void CBCarAI_StateChase(struct CBCarAI *self);
0008b480 void CBCarAI_StateFlee(struct CBCarAI *self);
0008b600 void CBCarAI_StatePuck(struct CBCarAI *self);
0008b800 void CBCarAI_Sense(struct CBCarAI *self);
0008bb30 void CBCarAI_StateWander(struct CBCarAI *self);
0008bdd0 void CBCarAI_Update(struct CBCarAI *self, int param);

# ---- not written yet, called by the AI (this-first shapes)

# ---- BumperCars/CBCar_Debug.c
0008bf50 void *CBCar_Debug_Ctor(struct CBCar_Debug *self);
0008bf90 void CBCar_Debug_Dtor(struct CBCar_Debug *self);
0008bfa0 void CBCar_Debug_Update(struct CBCar_Debug *self, int playing);
0008c000 int CBCar_Debug_NoLighting(struct CBCar_Debug *self);
0008c020 int CBCar_Debug_Get40(struct CBCar_Debug *self);
0008c030 int CBCar_Debug_EnvMap(struct CBCar_Debug *self);
0008c040 int CBCar_Debug_Reflections(struct CBCar_Debug *self);
0008c050 int CBCar_Debug_Overlays(struct CBCar_Debug *self);
0008c060 int CBCar_Debug_Fog(struct CBCar_Debug *self);
0008c070 int CBCar_Debug_CollisionMesh(struct CBCar_Debug *self);
0008c080 int CBCar_Debug_Characters(struct CBCar_Debug *self);
0008c090 int CBCar_Debug_Headlights(struct CBCar_Debug *self);
0008c0a0 struct CBCar_Debug *CBCar_Debug_Destroy(struct CBCar_Debug *self, byte flags);
0008c0c0 int CBCar_Debug_Initialise(struct CBCar_Debug *self);
0008c9f0 void CBCar_Debug_CreateOverlay(struct CBCar_Debug *self);
0008cae0 void CBCar_Debug_Shutdown(struct CBCar_Debug *self);

# ---- BumperCars/unplaced_fx.c (the debris effects, CBCar_Game + 0x5c)
0008cb60 void CBCar_Effects_Shutdown(void *self);
0008cb80 void CBCar_Effects_Update(void *self, float dt);
0008cb90 void CBCar_Effects_Draw(void *self, int view);
0008cba0 void CBCar_Effects_Add(void *self, int type, float *pos, float *vel, int lifeFrames, unsigned int colour);
0008cbd0 void CBCar_Effects_AddRing(void *self, int type, float *pos, float outSpeed, float upSpeed, float jitter, int count);
0008cdc0 int CBCar_Effects_Init(void *self);
0008ce30 void CBCar_Effects_Burst(void *self, float *pos, float outSpeed, float upSpeed, float jitter, int count, int ntypes, ...);
0008d030 void CBCar_Effects_Circle(int type, float *pos, float speedUp, float heightRandom, float angleRandom, float radius, int count, int lifeFrames);

# ---- BumperCars/CBCar_Camera.c (the camera, CBCar_Game + 0x2640)
0008d1e0 void *CBCar_Camera_Ctor(struct CBCar_Camera *self);
0008d1ee void CBCar_Camera_Dtor(struct CBCar_Camera *self);
0008d1f9 int CBCar_Camera_Initialise(struct CBCar_Camera *self, struct bkPackage *package, char *file);
0008d7ee void CBCar_Camera_HideWinner(struct CBCar_Camera *self);
0008d81d int CBCar_Camera_ShowWinner(struct CBCar_Camera *self);
0008d91d void CBCar_Camera_UpdateScores(struct CBCar_Camera *self);
0008dc4a void CBCar_Camera_LoadInformation(struct CBCar_Camera *self, struct bkPackage *package, struct BCarTextReader *reader);
0008e7f0 void CBCar_Camera_Shutdown(struct CBCar_Camera *self);
0008e8d1 float CBCar_Camera_GetYaw(struct CBCar_Camera *self);
0008e8e2 float CBCar_Camera_GetPitch(struct CBCar_Camera *self);
0008e8f3 float CBCar_Camera_StaticHeading(struct CBCar_Camera *self);
0008e974 void CBCar_Camera_GetDirection(struct CBCar_Camera *self, float *out);
0008ea74 void CBCar_Camera_SkipIntro(struct CBCar_Camera *self);
0008eceb void CBCar_Camera_Update(struct CBCar_Camera *self, int playing, float dt);
0008fc2e void CBCar_Camera_Nothing(void);
0008fc39 void CBCar_Camera_DrawFades(struct CBCar_Camera *self);
0008fe4b int CBCar_Camera_RacePlace(struct CBCar_Camera *self, int player);
00090243 int CBCar_Camera_InsideBorder(struct CBCar_Camera *self, float *p);
0009032e int CBCar_Camera_OutsideBorder(struct CBCar_Camera *self, float *p);
00090419 void CBCar_Camera_Static(struct CBCar_Camera *self);
000905bb void CBCar_Camera_Zoom(struct CBCar_Camera *self);
000909ab void CBCar_Camera_FirstPerson(struct CBCar_Camera *self);
00090b89 void CBCar_Camera_Winners(struct CBCar_Camera *self);
00090fba void CBCar_Camera_Rotate(struct CBCar_Camera *self, float dt);
00091287 void CBCar_Camera_Apply(struct CBCar_Camera *self);
0009130b void CBCar_Camera_SetLookAt(struct CBCar_Camera *self, float *eye, float *target, float roll);
0009139b void CBCar_Camera_SetAngles(struct CBCar_Camera *self, float *position, float *angles);
00091422 void CBCar_Camera_CarView(struct CBCar_Camera *self, int view, float back, float up);
000915fd void CBCar_Camera_LookAlong(struct CBCar_Camera *self, float *pos, float *dir, float roll);
00091650 void CBCar_Camera_ApplyMirrored(struct CBCar_Camera *self);
0009175a void CBCar_Camera_CarViewMirrored(struct CBCar_Camera *self, int view, float back, float up);
0009194d int CBCar_Camera_ViewsOn(struct CBCar_Camera *self, int *view);

# ---- BumperCars/CBCar_Goal.c (CBCar_Game + 0x3310)
000919e0 void *CBCar_Goal_Ctor(struct CBCar_Goal *self);
000919f0 void CBCar_Goal_Dtor(struct CBCar_Goal *self);
00091a00 struct BCarGoalArea *CBCar_Goal_Find(struct CBCar_Goal *self, float *pos);
00091aa0 float CBCar_Goal_Distance(struct CBCar_Goal *self, float *pos);
00091b30 void CBCar_Goal_Pause(struct CBCar_Goal *self);
00091b40 struct CBCar_Goal *CBCar_Goal_Destroy(struct CBCar_Goal *self, byte flags);
00091b60 void CBCar_Goal_LoadGoals(struct CBCar_Goal *self, struct BCarTextReader *reader);
00091d70 void CBCar_Goal_Shutdown(struct CBCar_Goal *self);
00091db0 void CBCar_Goal_Update(struct CBCar_Goal *self, int playing);
00091e30 int CBCar_Goal_Initialise(struct CBCar_Goal *self, struct bkPackage *package, char *file);

# ---- BumperCars/CBCar_Sound.c (CBCar_Game + 0x3320)
00091ef0 void *CBCar_Sound_Ctor(struct CBCar_Sound *self);
00091fd0 int CBCar_Sound_Initialise(struct CBCar_Sound *self);
000920b0 void CBCar_Sound_Shutdown(struct CBCar_Sound *self);
00092120 int CBCar_Sound_Play(struct CBCar_Sound *self, int index, int volume, int pitch, int channel);
00092290 int CBCar_Sound_ChannelDone(int channel);
000922b0 void CBCar_Sound_PauseAll(void);
000922c0 void CBCar_Sound_ResumeAll(void);
000922d0 void CBCar_Sound_SetPitch(struct CBCar_Sound *self, int channel, int index, int pitch);
00092310 void CBCar_Sound_Commentary(int kind, int choice, int volume);
00092340 void CBCar_Sound_Update(struct CBCar_Sound *self, float dt);
000923d0 void CBCar_Sound_StopAll(struct CBCar_Sound *self);
00092450 void CBCar_Sound_QueueVoice(struct CBCar_Sound *self, int player, int kind, int volume, float *pos, float delay);

# ---- BumperCars/CBCar_Light.c (CBCar_Game + 0x3604)
000924e0 void *CBCar_Light_Ctor(struct CBCar_Light *self);
000924f0 void CBCar_Light_Dtor(struct CBCar_Light *self);
00092500 struct BCarLight *CBCar_Light_Follow(struct CBCar_Light *self, const char *name, unsigned char *entity);
00092560 void CBCar_Light_SetOff(struct CBCar_Light *self, int off);
00092610 void CBCar_Light_ApplyAmbient(struct CBCar_Light *self);
00092680 void CBCar_Light_Draw(struct CBCar_Light *self, byte view);
00092740 void CBCar_Light_CreateSpot(struct CBCar_Light *self, float *target);
000927a0 void CBCar_Light_AimSpot(struct CBCar_Light *self, float *target);
000927d0 struct CBCar_Light *CBCar_Light_Destroy(struct CBCar_Light *self, byte flags);
000927f0 void CBCar_Light_Shutdown(struct CBCar_Light *self);
00092860 void CBCar_Light_SetD3DLight(struct BCarLight *l, int exact);
00092a10 void CBCar_Light_SetColour(struct BCarLight *l, int r, int g, int b);
00092a70 void CBCar_Light_Update(struct CBCar_Light *self, int playing, float dt);
00092dc0 void CBCar_Light_LoadLights(struct CBCar_Light *self, struct BCarTextReader *reader);
00094240 int CBCar_Light_Initialise(struct CBCar_Light *self, struct bkPackage *package, char *file);

# ---- BumperCars/CBCar_Flame.c (CBCar_Game + 0x3f28)
00094360 void *CBCar_Flames_Ctor(struct CBCar_Flames *self);
00094370 struct BCarFlame *CBCar_Flame_Ctor(struct BCarFlame *self);
000943c0 void CBCar_Flame_Dtor(struct BCarFlame *self);
00094420 void CBCar_Flame_UpdateArm(struct BCarFlame *self, float dt);
000946d0 void CBCar_Flames_Draw(struct CBCar_Flames *self, int view, int reflection);
00094880 float CBCar_Flames_HitTime(struct CBCar_Flames *self, float *pos, float radius);
000948f0 void CBCar_Flames_Pause(struct CBCar_Flames *self);
00094920 void CBCar_Flames_Resume(struct CBCar_Flames *self);
00094950 void CBCar_Flames_SetActive(struct CBCar_Flames *self, int active);
00094a00 void CBCar_Flames_Shutdown(struct CBCar_Flames *self);
00094af0 void CBCar_Flames_Update(struct CBCar_Flames *self, float dt, int playing);
00095150 int CBCar_Flames_Near(struct CBCar_Flames *self, float *pos, float radius, float *heading);
000951f0 int CBCar_Flames_Sweep(struct CBCar_Flames *self, float *pos, float radius, float *vel);
000952e0 void CBCar_Flames_LoadFlameInfo(struct CBCar_Flames *self, struct BCarTextReader *reader);
