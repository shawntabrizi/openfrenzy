# src/bumpercars_b/bumpercars_b.decls - prototype overrides of the bumpercars_b module
# (docs/ARCHITECTURE.md, section 11).  <addr> <prototype>;
# thiscall/fastcall `this` is the first parameter.

# BumperCars/bcb_flame.c (gap)
00096240 int BCarFlame_Initialise(void *self, struct bkPackage *package, const char *name);

# BumperCars/bcb_timer.c (gap): the timer class
00096320 void BCarTimer_Construct(struct BCarTimer *self);
00096350 int BCarTimer_Reset(struct BCarTimer *self);
000963a0 void BCarTimer_Update(struct BCarTimer *self);
000964d0 void BCarTimer_Start(struct BCarTimer *self);
00096550 int BCarGameTimer_Init(struct BCarGameTimer *self, int seconds);
000965b0 float BCarTimer_GetFrames(struct BCarTimer *self);
000965d0 void BCarTimer_Start2(struct BCarTimer *self);
000965e0 float BCarTimer_GetFraction(struct BCarTimer *self);
000965f0 float BCarTimer_GetRemaining(struct BCarTimer *self);
00096600 float BCarTimer_GetElapsed(struct BCarTimer *self);
00096610 float BCarTimer_GetFrameTime(struct BCarTimer *self);
00096620 float BCarTimer_GetAverageFrame(struct BCarTimer *self);
00096630 void BCarTimer_Restart(struct BCarTimer *self);
00096670 unsigned long long BCarTimer_GetUpdates(struct BCarTimer *self);

# BumperCars/CBCar_Player.c
00096680 void BCarPlayers_StartEngineSounds(void);
000966b0 void BCarPlayer_Clear(struct BCarPlayer *self);
000967b0 void BCarPlayer_CreateInput(int index);
00096b00 void BCarPlayer_DeleteInput(int index);
00096bd0 void BCarPlayers_CreateInputs(void);
00096c30 float BCarPlayer_GetMaxSpeed(struct BCarPlayer *self);
00096cb0 void BCarPlayers_ShutdownInput(void);
00096d00 void BCarPlayers_ResetInput(void);
00096d90 void BCarPlayers_LoadInformation(struct BCarTextReader *reader);
00097b20 void BCarPlayer_StopBrake(struct BCarPlayer *self);
00097c10 void BCarPlayer_Brake(struct BCarPlayer *self);
00098030 void BCarPlayer_Boost(struct BCarPlayer *self);
000981c0 void BCarPlayer_DriveToGoal(struct BCarPlayer *self);
000982f0 void BCarPlayers_Update(float dt, int control);
000991f0 void BCarPlayers_UpdateLead(int speak);
000994b0 void BCarPlayers_DrawLead(int shadow);
00099710 void BCarPlayers_Draw(int shadow);
00099fc0 void BCarPlayers_CreateOverlays(void);
0009a170 int BCarPlayers_Init(struct bkPackage *package, const char *name);
0009abb0 void BCarPlayers_Shutdown(void);

# BumperCars/CBCar_Entity.c
0009acb0 struct BCarEntity *BCarEntity_Construct(struct BCarEntity *self);
0009ad00 void BCarEntity_Destruct(struct BCarEntity *self);
0009ad60 void BCarEntity_Turn(struct BCarEntity *self, float delta);
0009adc0 void BCarEntity_SetHeading(struct BCarEntity *self, float heading);
0009ae00 void BCarEntities_ClampScores(void);
0009ae30 void BCarEntity_SetType(struct BCarEntity *self, int type);
0009ae40 void BCarEntities_PauseTimers(void);
0009ae90 void BCarEntities_ResumeTimers(void);
0009aee0 void BCarEntity_AddScore(struct BCarEntity *self, int points);
0009af70 struct BCarEntity *BCarEntity_Create(struct BActor *actor, struct BActor *extraActor, float scale, struct BCarEntityParams *params);
0009b4c0 void BCarEntity_Thrust(struct BCarEntity *self, float amount, int force);
0009b580 void BCarEntity_Kill(struct BCarEntity *self);
0009b5e0 int BCarEntity_CollideEntities(struct BCarEntity *self);
0009b850 int BCarEntity_CollideWorld(struct BCarEntity *self);
0009b930 void BCarEntity_Respawn(struct BCarEntity *self);
0009bc50 void BCarEntities_Draw(int shadow, int ghost);
0009c170 int BCarEntities_GetCentre(float *out);
0009c210 void BCarEntity_Spin(struct BCarEntity *self, float time);
0009c2d0 void BCarEntities_DeleteAll(void);
0009c360 void BCarEntity_Bounce(struct BCarEntity *self, int control);
0009d100 void BCarEntities_Update(float dt, int control);

# BumperCars/CBCar_World.c
0009e3b0 void BCarWorld_Reset(void);
0009e5a0 float BCarWorld_GetMaxSpeed(void);
0009e5c0 float BCarWorld_GetBoostValue(void);
0009e5e0 float BCarWorld_GetEntitySpeed(void);
0009e600 float BCarWorld_GetRestitution(void);
0009e610 void BCarWorld_ResetScores(void);
0009e690 void BCarWorld_LoadScoreInformation(struct BCarTextReader *reader);
0009ed60 void BCarWorld_Pause(void);
0009ed80 void BCarWorld_Resume(void);
0009eda0 void BCarWorld_Shutdown(void);
0009ee20 void BCarWorld_LoadSpawnInfo(struct BCarTextReader *reader);
0009f030 struct BCarSpawn *BCarWorld_FindSpawn(struct BCarEntity *entity);
0009f180 int BCarWorld_LoadScoreFile(struct bkPackage *package, const char *name);
0009f250 void BCarWorld_Update(float dt, int control);
0009f2e0 void BCarWorld_DrawSets(int reflection, int mirrored);
0009f6d0 float BCarWorld_GetGroundHeight(float *pos, float radius, float *normal);
0009f860 int BCarWorld_LoadSpawnFile(struct bkPackage *package, const char *name);
0009f930 void BCarWorld_Draw(int reflection, int pass);
0009f9b0 void BCarWorld_LoadInformation(struct bkPackage *package, struct BCarTextReader *reader);
000a0620 int BCarWorld_Init(struct bkPackage *package, const char *name);

# BumperCars/CBCar_FloatyScore.c
000a0b70 void BCarFloaty_Update(struct BCarFloaty *self, float dt, float angle);
000a0ba0 void BCarFloaty_Draw(struct BCarFloaty *self);
000a0bc0 void BCarFloaty_Add(struct BCarFloaty *self, float *pos, int points, int r, int g, int b);
000a0c30 struct BCarFloaty *BCarFloaty_Construct(struct BCarFloaty *self, int maxItems, float scale);
000a0c90 void BCarFloaty_Destruct(struct BCarFloaty *self);

# BumperCars/CBCar_Puck.c, CBCar_Powerup.c, CBCar_Keepsake.c
000a0cc0 int BCarPucks_Init(void);
000a0db0 void BCarPuck_Load(struct BCarTextReader *reader);
000a16c0 void BCarPowerup_Load(struct BCarTextReader *reader);
000a1f40 int BCarPowerups_Init(void);
000a24f0 void BCarKeepsake_Load(struct BCarTextReader *reader);
000a2af0 int BCarKeepsakes_Init(void);

# BumperCars/bcb_misc.c (gap)
000a4e00 int BCarText_GetName(char *line, char *out, int size);
000a50e0 void BCarRender_BeginReflection(int mode);
000a5150 void BCarRender_EndReflection(void);
000a5180 int BCarEntity_SameTeam(struct BCarEntity *a, struct BCarEntity *b, int *indexA, int *indexB);
000a5230 void BCarGame_Rumble(int player, int type, float strength);
000a5540 float BCarMath_Atan2(float y, float x);
000a5be0 void BCarTextReader_Construct(struct BCarTextReader *self);
000a5bf0 void BCarTextReader_Destruct(struct BCarTextReader *self);
000a5c00 void BCarTextReader_Init(struct BCarTextReader *self, char *buffer, int size, const char *name);
000a5af0 void BCarMath_TiltToNormal(float *out, float *quat, float *normal);

# BumperCars/CBCar_Track.c
000a6260 struct BCarTrack *BCarTrack_Construct(struct BCarTrack *self, int maxPoints, float width, float spacing, struct bdTexture *texture, struct bdTexture *texture2);
000a6310 void BCarTrack_Destruct(struct BCarTrack *self);
000a6330 void BCarTrack_Update(struct BCarTrack *self, float dt);
000a6350 void BCarTrack_End(struct BCarTrack *self);
000a6360 void BCarTrack_AddPoint(struct BCarTrack *self, float *pos, float *normal, float *dir, float alpha);
000a64c0 void BCarTrack_Draw(struct BCarTrack *self);

# BumperCars/CBCar_Headlight.c
000a6930 void BCarHeadlight_Destruct(struct BCarHeadlight *self);
000a6950 void BCarHeadlight_Draw(struct BCarHeadlight *self, float alpha);
000a6b50 void BCarHeadlight_SetFade(struct BCarHeadlight *self, float fade);
000a6bc0 struct BCarHeadlight *BCarHeadlight_Construct(struct BCarHeadlight *self, float *pos, float *rot, struct bdTexture *texture, float range, float width, int segments);
000a6d60 void BCarHeadlight_Update(struct BCarHeadlight *self);

# ---- agent B (0x000a0cc0-0x000a8250); A's lines above for this range are kept
# BumperCars/CBCar_Puck.c
000a11b0 void BCarPucks_Update(int playing);
000a1390 void BCarPucks_Delete(void);
000a13d0 void BCarPucks_Draw(int glow, int glowPass);
# BumperCars/CBCar_Powerup.c
000a1a70 void BCarPowerups_Delete(void);
000a1ac0 void BCarPowerup_Respawn(struct BCarEntity *entity);
000a1b60 void BCarPowerup_Hit(struct BCarEntity *car, struct BCarEntity *item);
000a1d30 void BCarPowerups_Draw(int draw);
000a2020 void BCarPowerups_Update(int playing, float dt);
000a24c0 void BCarEntity_DropCarried(struct BCarEntity *self);
# BumperCars/CBCar_Keepsake.c
000a2950 float BCarKeepsake_RayTest(float *origin, float *dir, float *point, float *normal);
000a2d40 void BCarKeepsakes_Update(void);
000a3080 void BCarKeepsakes_Draw(int glow, int glowPass);
000a3450 void BCarKeepsakes_Delete(void);
# BumperCars/CBCar_Explosion.c
000a34b0 struct BCarExplosions *BCarExplosions_Construct(struct BCarExplosions *self);
000a34f0 struct BCarExplosion *BCarExplosion_Construct(struct BCarExplosion *self, float *pos, float radius, float duration, struct bdTexture *texture);
000a3740 int BCarExplosion_Update(struct BCarExplosion *self, float dt);
000a3880 void BCarExplosion_Draw(struct BCarExplosion *self, int view);
000a3aa0 void BCarExplosions_Update(struct BCarExplosions *self, float dt);
000a3af0 void BCarExplosions_Draw(struct BCarExplosions *self, int view);
000a3b20 int BCarExplosions_Add(struct BCarExplosions *self, float *pos, float radius);
000a3bf0 void BCarExplosions_Delete(struct BCarExplosions *self);
