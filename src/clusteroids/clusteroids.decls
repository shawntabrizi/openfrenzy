# src/clusteroids/clusteroids.decls - prototype overrides of the clusteroids module
# (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# Clusteroids/CModule_Clusteroids.cpp
000d9390 void *CModuleClusteroids_Ctor(void *self);
000d93c0 void CModuleClusteroids_Dtor(void *self);
000d93d0 int CModuleClusteroids_Init(void *self);
000d93f0 void *CModuleClusteroids_DeletingDtor(void *self, unsigned int flags);
000d9410 int CModuleClusteroids_Start(void *self, int variant);

# Clusteroids/CGameMode_Clusteroids.cpp
000d9520 void *CGameModeClusteroids_Ctor(void *self);
000d9540 void CGameModeClusteroids_Dtor(void *self);
000d9560 void CGameModeClusteroids_Pushed(void *self);
000d9620 void CGameModeClusteroids_LoadingStarted(void *self);
000d96f0 void CGameModeClusteroids_Resume(void *self);
000d9710 void CGameModeClusteroids_Suspend(void *self);
000d9730 void CGameModeClusteroids_Input(int *self, int active);
000d9750 void CGameModeClusteroids_Render(void *self, int active);
000d9760 void *CGameModeClusteroids_DeletingDtor(void *self, unsigned int flags);
000d9780 void CGameModeClusteroids_Start(void *self);
000d9f30 void CGameModeClusteroids_Update(void *self, int active);
000da1f0 void CGameModeClusteroids_Popped(void *self);

# Clusteroids/CClust_Arena.cpp (0x000da370 tagged; the rest inferred)
000da370 void ClustArena_LoadModels(void);
000daad0 void ClustArena_Free(void);
000dabe0 void ClustArena_InitSpawnPoints(void);
000dacf0 void ClustArena_InitLights(void);
000dadb0 void ClustArena_UpdateLights(void);
000dae10 void ClustArena_DrawLights(void);
000daf60 void ClustArena_SparkRing(float *pos);
000daff0 void ClustWalker_NextTarget(float *self);
000db0f0 void ClustWalkers_Init(void);
000db2a0 void ClustWalkers_Update(void);
000db3d0 void ClustGame_Init(void);
000dbd60 void ClustGame_Reset(void);

# (gap functions, file unknown; see the comments in the .c files)
000dc9a0 void ClustTrails_Init(void);
000dcab0 int ClustGame_GetHighestScore(void);
000dcaf0 int ClustGame_GetLowestScore(void);
000dd0d0 void ClustGame_Update(void);
000de210 int ClustCamera_Init(void *self, int package);
000de770 void ClustCountdown_Update(void);
000de8e0 float ClustCountdown_GetRemaining(void);
000de8f0 int ClustTimer_Init(void);
000de950 void ClustTimer_UpdateFrame(void);
000dec00 int ClustSound_Load(int package);
000ded00 void ClustSound_Free(void);

# Remaining functions (names from the definitions; file in brackets)
# Clusteroids/CClust_Player.c
000dee00 void ClustPlayer_Init(int player);
000df7f0 void ClustPlayers_Celebrate(void);
000dfb30 void ClustPlayers_LineUp(void);
000dfbe0 void ClustPlayer_Rumble(int player, int effect);
000dfc90 void ClustPlayer_Update(int player);
# Clusteroids/CClust_Pods.c
000e3150 int ClustPods_GetLeader(void);
000e3260 void ClustPods_AddScenic(char *pod, short model);
000e3350 void ClustPods_Release(int *pod, int *prev, short player);
000e3440 float ClustPods_NearestFreePod(char *pod);
000e34e0 float ClustPods_NearestPlayer(char *pod);
000e3570 void ClustPods_InitCollector(void);
000e37a0 void ClustPods_Reset(void);
000e39d0 int ClustPods_SpawnTailsPods(void);
000e3c20 float ClustPods_Heading(float x, float z);
000e3d00 void ClustPods_Attach(short player, char *pod);
000e3e20 void ClustPods_UpdateScenic(void);
000e3fb0 void ClustPods_UpdateHeld(void);
000e4990 void ClustPods_Respawn(char *pod);
000e4b50 void ClustPods_Init(void);
000e4bd0 void ClustPods_UpdateCarried(void);
000e4ea0 void ClustPods_UpdateFree(void);
000e51b0 void ClustGates_UpdateDeliveries(void);
000e52f0 char *ClustPods_FirstHolder(void);
000e5320 int ClustPods_CountLongest(int unused, int *best);
000e53d0 int ClustPods_CountLoose(int player);
000e5410 int ClustPods_CountLooseOf(char *player);
000e5440 float ClustVec3_Distance(float *a, float *b);
000e5480 char *ClustPods_ChooseVictim(int player);
000e55b0 char *ClustPods_NearestShip(int player, float *pos, float *distance);
000e5640 char *ClustPods_NearestFree(int player);
000e5700 char *ClustPods_FindRobbable(int player);
000e57d0 char *ClustPods_FindRobbableHuman(int player);
# Clusteroids/CClust_Tokens.c
000e2ae0 void ClustTokens_Reset(void);
000e2b40 int ClustTokens_Add(float x, float z, int kind);
000e2bd0 void ClustTokens_Collect(void);
000e2d10 void ClustTokens_Init(void);
000e2d40 void ClustTokens_Update(void);
000e2e90 void ClustTokens_Draw(void);
000e3030 void ClustTokens_DrawGlow(void);
# Clusteroids/CClust_Trails.c
000dc080 void ClustTrails_Reset(void);
000dc0d0 void ClustTrails_Free(void);
000dc0f0 void *ClustTrails_Add(char *player, short id);
000dc760 void ClustTrails_Update(void);
000dc930 void ClustTrails_Draw(void);
# Clusteroids/unplaced.c
000dc9d0 int ClustGame_GetWinner(void);
000dcb30 void ClustGame_UpdateScores(void);
000dcfa0 void ClustGame_UpdatePlaying(void);
000dd610 void ClustGame_DrawScreenBar(void);
000dd7a0 void ClustCamera_UpdateTarget(void);
000dda20 int ClustIntro_SpinnerDone(int reason);
000dda80 int ClustIntro_SpinnerIntroDone(int reason);
000ddad0 int ClustIntro_DoorsOpened(int reason);
000ddb30 int ClustIntro_DoorsIntroDone(int reason);
000ddb80 unsigned int ClustGame_CollectWinners(void);
000ddc10 int ClustGame_MayContinue(int player);
000ddcb0 int ClustGame_SkipPressed(void);
000ddd50 void ClustGame_UpdateEnd(void);
000de2f0 int ClustCamera_Update(void *self, int unused, float step);
000de4f0 void ClustGame_UpdateIntro(void);
000de710 void *ClustCamera_DeletingDtor(void *self, unsigned int flags);
000de730 int ClustCountdown_Init(void);
000de890 void ClustCountdown_SetDuration(float seconds);
000de8c0 void ClustCountdown_Start(void);
000decb0 int ClustSound_Play(int index, int volume);
000ded70 int ClustSound_PlayVoice(int player, int kind);
# Clusteroids/unplaced_ai.c
000e58b0 void ClustAI_Collector(int player, float *x, float *z);
000e5cb0 void ClustAI_PickRoamTarget(int player, int avoid);
000e5de0 void ClustAI_Tails(int player, float *x, float *z);
000e61f0 void ClustAI_Mines(int player, float *x, float *z);
000e6650 void ClustAI_PodHog(int player, float *x, float *z);
# Clusteroids/unplaced_bullets.c
000e47f0 void ClustBullets_Move(void);
000e6af0 void ClustBullets_Fire(short player);
000e6d50 void ClustBullets_Init(void);
000e6d70 void ClustArena_PushFromSpawnPoints(void);
000e6f70 short ClustPods_HitPlayer(char *pod);
000e7120 void ClustPods_Collide(int *pod);
000e7210 void ClustPlayers_Collide(void);
000e73f0 void ClustBullets_HitTest(void);
000e7a40 float ClustArena_GetHeight(float x, float z);
# Clusteroids/unplaced_draw.c
000deac0 void ClustProfile_Reset(void);
000deaf0 void ClustProfile_Begin(int index, const char *name);
000deb40 void ClustProfile_End(int index);
000e04c0 void ClustDraw_PodConnector(int player, int red, int green, int blue, int alpha);
000e0720 void ClustDraw_Clouds(void);
000e0ce0 void ClustDraw_Fps(void);
000e0e00 void ClustDraw_ArenaActors(int trans);
000e0f60 void ClustDraw_Gates(void);
000e1060 void ClustDraw_PodConnectors(void);
000e10d0 void ClustDraw_ScenicPods(int trans);
000e1200 void ClustDraw_FreePods(int trans);
000e12e0 void ClustDraw_FreeMines(int trans);
000e1440 void ClustDraw_HeldPods(int trans);
000e1620 void ClustDraw_HeldPodTiles(int trans);
000e16f0 void ClustDraw_Bullets(void);
000e17f0 void ClustDraw_Line(float *from, float *to, unsigned int colour, int alpha);
000e18d0 void ClustDraw_Circle(float x, float y, float z, float radius, unsigned int colour, int alpha);
000e19c0 void ClustDraw_DebugRadii(void);
000e1b60 void ClustPlayer_SetConnector(int player, float *from, float *to);
000e1df0 void ClustDraw_Players(void);
000e2210 void ClustPlayers_UpdateConnectors(void);
000e22b0 void ClustGame_Draw(void);
000e28b0 void ClustGame_Render(void);
