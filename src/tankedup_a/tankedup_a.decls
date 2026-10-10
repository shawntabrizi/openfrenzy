# src/tankedup_a/tankedup_a.decls - prototype overrides of the tankedup_a module
# (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.
# Subsystem methods take their sub-object of CTank_Game (tankedup_a.h) as `void *self`.

# TankedUp/CModule_TankedUp.cpp
001c1760 void *CModuleTankedUp_Ctor(void *self);
001c1790 void CModuleTankedUp_Dtor(void *self);
001c17a0 int CModuleTankedUp_Init(void *self);
001c17b0 void CModuleTankedUp_Slot2(void *self);
001c17c0 void *CModuleTankedUp_DeletingDtor(void *self, unsigned int flags);
001c17e0 int CModuleTankedUp_Start(void *self, int variation);

# TankedUp/CTank_Game.cpp (0x001c1fb0 tagged; the rest inferred)
001c1880 struct CTank_Game *CTankGame_Ctor(struct CTank_Game *self);
001c1990 void CTankGame_Dtor(struct CTank_Game *self);
001c1b20 void CTankGame_Free(struct CTank_Game *self);
001c1c60 void CTankGame_ReleaseInput(struct CTank_Game *self);
001c1ca0 void CTankGame_Input(struct CTank_Game *self, int active);
001c1d00 void CTankGame_Render(struct CTank_Game *self, int active);
001c1eb0 void CTankGame_Pause(struct CTank_Game *self);
001c1f30 void CTankGame_Resume(struct CTank_Game *self);
001c1fb0 int CTankGame_Start(struct CTank_Game *self, int variation);
001c2240 int CTankGame_Update(struct CTank_Game *self, int active);
001c2660 int CTankGame_LoadGameSettings(struct CTank_Game *self, struct TuTextReader *reader);
001c2940 int CTankGame_LoadGameDetails(struct CTank_Game *self);
001c2cf0 void CTankGame_InitGameOneTime(struct CTank_Game *self);

# TankedUp/CTank_Boom.cpp (0x001c3180 tagged; the rest inferred)
001c2e00 int CTankBoom_Initialise(void *self);
001c2e70 void CTankBoom_DrawScorch(void *self, struct TuBoom *node);
001c30b0 void CTankBoom_Free(void *self);
001c30d0 int CTankBoom_Update(void *self, float step, int state);
001c3140 void CTankBoom_DrawScorches(void *self);
001c3180 void CTankBoom_Add(void *self, float *pos, float height);
001c3250 void CTankBoom_DrawShockwave(void *self, struct TuBoom *node);
001c3450 void CTankBoom_DrawShockwaves(void *self);

# TankedUp/unplaced_debris.c (debris system; 0x001c3490 is not in the table)
001c34b0 int TankDebris_Initialise(void *self, void *package, void *world);
001c34f0 void TankDebris_Free(void *self);
001c3510 int TankDebris_Update(void *self, float step, int state);
001c3530 void TankDebris_Draw(void *self);
001c3540 void TankDebris_Add(void *self, int type, float *pos, float *vel, int life, unsigned int colour);

# TankedUp/unplaced_timer.c (struct TuTimer)
001c3570 int CTankTimer_Reset(struct TuTimer *self);
001c35b0 void CTankTimer_Tick(struct TuTimer *self);
001c36e0 void CTankTimer_SetLength(struct TuTimer *self, float seconds);
001c3700 void CTankTimer_StartOrResume(struct TuTimer *self);
001c3770 void CTankTimer_Pause(struct TuTimer *self);
001c3790 void CTankTimer_Ctor(struct TuTimer *self);
001c37b0 int CTankTimer_Initialise(struct TuTimer *self, int seconds);
001c3800 float CTankTimer_GetFrame(struct TuTimer *self);
001c3810 void CTankTimer_Resume(struct TuTimer *self);

# TankedUp/unplaced_lights.c (struct TuLight[4])
001c3820 int CTankLights_Initialise(struct TuLight *self);
001c3850 void CTankLights_ApplyAmbient(struct TuLight *self);
001c3870 void CTankLights_GetAmbient(struct TuLight *self, int *r, int *g, int *b);
001c38a0 void CTankLights_SetAmbient(int r, int g, int b);
001c38c0 void CTankLights_EnableSun(struct TuLight *self, int on);
001c3960 void CTankLights_EnableAll(struct TuLight *self, int on);
001c3990 void CTankLights_Remove(struct TuLight *self, int index);
001c39d0 void CTankLights_Free(struct TuLight *self);
001c3a10 int CTankLights_Update(struct TuLight *self, float step, int state);
001c3ae0 void CTankLights_Start(struct TuLight *self);
001c3c90 void CTankLights_TrackSpot(struct TuLight *self, int owner, float *target);
001c3d30 void CTankLights_AddFlash(struct TuLight *self, float *pos, int r, int g, int b);
001c3e00 void CTankLights_AddFireFlash(struct TuLight *self, float *pos);
001c3eb0 void CTankLights_SetSpot(struct TuLight *self, int *index, float *pos, int r, int g, int b);
001c3fa0 void CTankLights_Draw(struct TuLight *self);

# TankedUp/CTank_Spawn.cpp (0x001c41a0, 0x001c44b0 tagged; the rest inferred)
001c40d0 void *CTankSpawn_GetPoints(void *self, const char *name);
001c4110 void *CTankSpawn_GetPoint(void *self, const char *name, int index);
001c4160 int CTankSpawn_GetCount(void *self, const char *name);
001c41a0 int CTankSpawn_LoadSpawnPoint(void *self, struct TuTextReader *reader, struct TuSpawnPoint **list);
001c43c0 void CTankSpawn_Free(void *self);
001c4420 void CTankSpawn_GetRandomPoint(void *self, const char *name, float *out);
001c44b0 void CTankSpawn_LoadSpawnInfo(void *self, struct TuTextReader *reader);
001c4770 int CTankSpawn_Initialise(void *self, void *package, char *file);

# TankedUp/CTank_Camera.cpp (0x001c5030, 0x001c6a70 tagged; the rest inferred)
001c4850 void *CTankCamera_Ctor(void *self);
001c4880 void CTankCamera_Dtor(void *self);
001c4890 void CTankCamera_CreateInput(void *self);
001c4a10 void CTankCamera_ReleaseInput(void *self);
001c4ad0 void CTankCamera_Start(void *self);
001c4af0 int CTankCamera_LoadLom(void *self);
001c4b90 float CTankCamera_GetShotTime(void *self);
001c4ba0 float CTankCamera_GetShotTimeLeft(void *self);
001c4bc0 void CTankCamera_Draw(void *self);
001c4d60 void CTankCamera_Pause(void *self);
001c4da0 void CTankCamera_Resume(void *self);
001c4db0 int CTankCamera_Input(void *self);
001c5030 void CTankCamera_NewShot(void *self, struct TuShot **out);
001c5180 void CTankCamera_LoadCameraInfo(void *self, struct TuTextReader *reader);
001c56e0 void CTankCamera_UpdateFrom(void *self, struct TuShot *shot, float step);
001c5950 void CTankCamera_UpdateTo(void *self, struct TuShot *shot, float step);
001c5c00 void CTankCamera_UpdateZoom(void *self, struct TuShot *shot, float step);
001c60c0 int CTankCamera_Update(void *self, float step, int state);
001c66f0 float CTankCamera_GetHeading(void *self);
001c6750 int CTankCamera_SetShot(void *self, const char *name);
001c68c0 void CTankCamera_SetView(void *self);
001c6a70 int CTankCamera_Initialise(void *self, void *package, char *file, void *entities);
001c70d0 void CTankCamera_Free(void *self);
001c71c0 void CTankCamera_StateChange(void *self, int from, int to);

# TankedUp/CTank_Route.cpp (0x001c74c0, 0x001c75f0, 0x001c7a70, 0x001c7ac0 tagged; the rest inferred)
001c7280 void *CTankRoute_Ctor(void *self);
001c7290 int CTankSystem_KeepState(void *self, float step, int state);
001c72a0 void CTankRoute_Draw(void *self);
001c74c0 void CTankRoute_NewNode(void *self, struct TuRouteNode **out, float x, float z, float radius);
001c7580 void CTankRoute_AddNode(void *self, float x, float z, float radius);
001c75f0 void CTankRoute_LinkNodes(void *self);
001c76f0 int CTankRoute_Initialise(void *self, void *package, char *file, void *world);
001c79f0 void CTankRoute_Free(void *self);
001c7a50 void CTankRoute_FreeList(struct TuRouteLink **self);
001c7a70 void CTankRoute_Push(struct TuRouteLink **self, struct TuRouteNode *node);
001c7ac0 void CTankRoute_SetTarget(void *self, float *pos, int index);
001c7cd0 float CTankRoute_GetWaypoint(void *self, float *pos, float *out, int index);
001c7e20 void CTankRoute_LinkEntities(void *self, void *entities);

# TankedUp/CTank_Players.cpp (0x001c7f40 tagged; the rest inferred)
001c7f20 void *CTankPlayers_Ctor(void *self);
001c7f30 void CTankPlayers_Dtor(void *self);
001c7f40 int CTankPlayers_Initialise(void *self, void *entities, struct TuTimer *timer, void *world, void *camera, void *sound, void *lights);
001c81b0 void CTankPlayers_Rumble(void *self, int player, int kind);
001c82e0 int CTankPlayers_SameTeam(void *self, int a, int b);
001c8370 void CTankPlayers_GetTankPose(void *self, int player, float *pos, float *rot);
001c83b0 int CTankPlayers_IsAlive(void *self, int player);
001c83d0 int CTankPlayers_IsFirst(void *self, int player);
001c83f0 int CTankPlayers_IsWinner(void *self, int id);
001c8460 int CTankPlayers_IsHuman(void *self, int id);
001c8490 int CTankPlayers_CanAct(void *self, int player);
001c84b0 void CTankPlayers_SetEngineVolume(void *self, float value, int id);
001c8510 void CTankPlayers_Input(void *self);
001c8650 void CTankPlayers_Draw(void *self);
001c86d0 void CTankPlayers_CreatePlayerInput(void *self, int player);
001c8a20 void CTankPlayers_ReleasePlayerInput(void *self, int player);
001c8af0 void CTankPlayers_CreateInput(void *self);
001c8b30 void CTankPlayers_ReleaseInput(void *self);
001c8b70 void CTankPlayers_BeginFrame(void *self);
001c8ca0 int CTankPlayers_GetRank(void *self, int id);
001c8d70 void CTankPlayers_PlayVoice(int player, int kind, int unused, int volume);
001c8db0 void *CTankPlayers_DeletingDtor(void *self, unsigned int flags);
001c8dd0 void CTankPlayers_Free(void *self);
001c8e80 void CTankPlayers_RumbleId(void *self, int id, int kind);
001c8ec0 void CTankPlayers_StateChange(void *self, int from, int to);
001c9120 void CTankPlayers_CreateHud(void *self);
001c9600 void CTankPlayers_UpdateHud(void *self, int *state);
001c9c90 void CTankPlayers_Start(void *self);
001c9d30 int CTankPlayers_Update(void *self, float step, int state);

# TankedUp/CTank_World.cpp (0x001cbcd0, 0x001cd920 tagged; the rest inferred;
# 0x001c9f50 is not in the table)
001c9fb0 void *CTankWorld_Ctor(void *self);
001c9fd0 void CTankWorldObject_Dtor(void *obj);
001c9fe0 void CTankWorld_SwitchGroup(void *self, int group);
001ca060 struct TuWorldObject *CTankWorld_FindObject(void *self, const char *name, struct TuWorldObject *after);
001ca0b0 void CTankWorld_DrawSky(void *self, float time);
001ca470 void CTankWorld_Draw(void *self);
001ca9a0 void CTankWorld_Pause(void *self);
001ca9d0 void CTankWorld_Resume(void *self);
001caa00 void CTankWorld_ObjectMatrices(float *pos, float *rot, float *toLocal, float *toWorld, float *rotate);
001caa70 float CTankWorld_GetGravity(void *self);
001caa80 int CTankWorld_GetLaps(void *self);
001caa90 void CTankWorld_ClearObject(void *self, struct TuWorldObject *obj);
001cab50 void CTankWorld_StartObject(void *self, struct TuWorldObject *obj);
001cac90 void CTankWorld_RunStartChains(void *self);
001cad70 int CTankWorld_Update(void *self, float step, int state);
001cb1a0 float CTankWorld_GroundDistance(void *self, float *pos, float radius, float *normal);
001cb3f0 int CTankWorld_SphereHits(void *self, float *from, float *to, float radius, int startedOnly);
001cb560 float CTankWorld_SweepGround(void *self, float *pos, float *delta, float *hit, float *normal);
001cb7f0 int CTankWorld_CanSee(void *self, float *from, float *to);
001cb860 int CTankWorld_InsideBoundary(struct TuWorldObject *obj, float *point);
001cb970 int CTankWorld_CrossCheckpoint(void *self, float *pos, float *delta, unsigned int *flags);
001cbc80 void CTankWorld_Start(void *self);
001cbcd0 void CTankWorld_LoadWorldObjects(void *self, struct TuTextReader *reader);
001cd320 void CTankWorld_Free(void *self);
001cd3d0 int CTankWorld_Collide(void *self, float *pos, float *vel, float *rot, float radius, unsigned int *flags, float bounce, int *onGround, float *friction, float *normal, int *bounced);
001cd8c0 float CTankWorld_FreeFraction(void *self, float *from, float *to);
001cd920 int CTankWorld_Initialise(void *self, void *package, char *file, void *sound, void *lights);

# TankedUp/unplaced_entities.c (gap; probably the start of CTank_Entities.cpp, which
# tankedup_b continues). 0x001cdc30, 0x001cebd0, 0x001cdd50, 0x001ce270 and 0x001ce940 take no
# `this` (the first two are __thiscall methods that do not use it; tankedup_b calls 0x001cebd0
# that way).
001cdb20 void *CTankEntities_Ctor(void *self);
001cdb40 void CTankEntities_SetLinks(void *self, void *camera, void *players);
001cdb60 void CTankEntities_Start(void *self);
001cdc30 void CTankEntities_Revive(void *ent);
001cdca0 void CTankEntities_Pause(void *self);
001cdcf0 void CTankEntities_Resume(void *self);
001cdd50 void CTankEntities_DrawGlow(float *pos, float angle, void *texture);
001cde00 int CTankEntities_FindId(void *self, const char *name);
001cde40 int CTankEntities_SendMessageNow(void *self, void *ent, int state, void *message);
001ce100 void *CTankEntities_Get(void *self, int id);
001ce130 void CTankEntities_RouteMessage(void *self, void *message);
001ce1f0 void CTankEntities_ChangeState(void *self, void *ent);
001ce270 void CTankEntities_SkipId(void);
001ce280 void CTankEntities_EntityDtor(void *ent);
001ce290 void CTankEntities_Setup(void *self);
001ce300 void CTankEntities_Shutdown(void *self);
001ce350 int CTankEntities_IsAlive(void *self, int id);
001ce390 void CTankEntities_GetStats(void *self, int id, int *kills, int *health, int *mines, int *laps, int *finished);
001ce410 int CTankEntities_CanAct(void *self, int id);
001ce460 void CTankEntities_SetLead(void *self, int id, float lead);
001ce4d0 int CTankEntities_GetProgress(void *self, int id);
001ce520 int CTankEntities_HasShell(void *self, int id);
001ce630 float CTankEntities_GetScoreBar(void *self, int id);
001ce700 float CTankEntities_GetSpeedTime(void *self, int id);
001ce7d0 void CTankEntities_FindNearestTank(void *self, float *pos, int selfId, int *out);
001ce870 void CTankEntities_Turn(void *self, int id, float turn);
001ce940 float CTankEntities_WrapAngle(float angle);
001ce990 int CTankEntities_CountNear(void *self, float *pos, float radius);
001cea10 void CTankEntities_Think(void *self);
001ceb30 void CTankEntities_UpdateStates(void *self);
001ceba0 void CTankEntities_ChangeStates(void *self);
001cebd0 void CTankEntities_InitEntity(void *ent);
