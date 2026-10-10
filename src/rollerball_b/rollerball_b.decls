# src/rollerball_b/rollerball_b.decls - prototype overrides of the rollerball_b module
# (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.
# Objects are `void *` (their layouts are offset macros in rollerball_b.h).

# RollerBall/CRollerball_Goal.cpp (rest; inferred)
001915c0 void CRollerballGoalSystem_ResetGoals(void *self);
00191690 void CRollerballGoalSystem_StartFlare(void *self, float *pos, int r, int g, int b);
001916e0 void CRollerballGoalSystem_StartBurst(void *self, float *pos, short r, short g, short b);
00191780 void CRollerballGoal_UpdateActivating(void *self);
001917a0 void CRollerballGoal_UpdateDeactivating(void *self);
001917c0 void CRollerballGoal_Score(void *self);
00191a50 int CRollerballGoalSystem_Init(void *self, struct bkPackage *package);
00191a7b int FUN_00191a7b(void);
00191b30 void CRollerballGoal_Update(void *self, float dt);
00191bd0 void CRollerballGoalSystem_Update(void *self, float dt);

# RollerBall/CRollerball_Entity.cpp
00192170 void *CRollerballEntity_Ctor(void *self);
001921c0 void CRollerballEntity_Dtor(void *self);
00192220 void CRollerballEntity_Integrate(void *self, float dt);
00192270 void CRollerballEntity_ClearPush(void *self);
00192290 void CRollerballEntitySystem_Ctor(void *self);
001922a0 void CRollerballEntitySystem_Dtor(void *self);
001922b0 void CRollerballEntitySystem_Shutdown(void *self);
00192350 void *CRollerballEntitySystem_GetFirst(void *self);
00192360 void CRollerballEntitySystem_Update(void *self, float dt);
00192390 void CRollerballEntitySystem_Draw(void *self);
00192400 void CRollerballEntitySystem_UpdateComputerFlags(void);
00192430 void CRollerballEntitySystem_SetAnimMode(int mode);
00192460 void *CRollerballEntity_DeletingDtor(void *self, unsigned int flags);
00192480 void CRollerballEntity_SetPosition(void *self, float *pos);
001924c0 void CRollerballEntity_ComputeForces(void *self);
001926e0 void CRollerballEntity_UpdateRoll(void *self);
00192790 void CRollerballEntity_SetPush(void *self, float *push);
001927c0 void *CRollerballEntitySystem_DeletingDtor(void *self, unsigned int flags);
001927e0 int CRollerballEntitySystem_Init(void *self, struct bkPackage *package);

# RollerBall/CRollerBall_EntityEvent.cpp
00196a80 void CRollerBallEntityEventSystem_Ctor(void *self);
00196aa0 void CRollerBallEntityEventSystem_Dtor(void *self);

# RollerBall/CRollerBall_Player.cpp
00198a00 void *CRollerballPlayer_Ctor(void *self);
00198e80 void CRollerballPlayer_FindTeamMate(void *self);
001999e0 void CRollerballPlayer_SetAnimMode(void *self, int mode);
00199ba0 int CRollerballPlayer_Init(void *self, int player, float *pos, float *rot, float mass);

# player controllers (AI states; inferred)
0019a640 void *CRollerballControlSumo_Ctor(void *self);
0019a650 void *CRollerballControlRace2_Ctor(void *self);
0019a710 void *CRollerballControlRace_Ctor(void *self);
0019a720 void *CRollerballControlDrop_Ctor(void *self);
0019a9b0 void *CRollerballControlBase_Ctor(void *self);

# the camera (file unknown: RollerBall/unplaced_camera.c)
00192c40 void *CRollerballCamera_Ctor(void *self);
00192c60 void CRollerballCamera_Dtor(void *self);
00192c80 void CRollerballCamera_SetMode(void *self, int mode);
00192ca0 void CRollerballCamera_Apply(void *self);
00192ce0 void *CRollerballCamera_PickWinner(void *self);
00192d70 void CRollerballCamera_DrawOverlays(void *self);
00192e20 void CRollerballCamera_Reset(void *self);
00192f20 void *CRollerballCamera_DeletingDtor(void *self, unsigned int flags);
00192f40 int CRollerballCamera_Init(void *self);
00192f60 void CRollerballCamera_UpdateIntro(void *self, float dt);
00193490 void CRollerballCamera_EndShotArena(void *self, float dt);
001935f0 void CRollerballCamera_FitPoints(void *self, float *eye_out, float *target_out, float *points, int count, float pitch, float yaw);
00193c10 void CRollerballCamera_FrameBase(void *self, float dt, unsigned char flags);
00193e40 void CRollerballCamera_FrameSumo(void *self, float dt, unsigned char flags);
00194120 void CRollerballCamera_FrameRace2(void *self, float dt, unsigned char flags);
00194330 void CRollerballCamera_FrameRace(void *self, float dt, unsigned char flags);
001945e0 void CRollerballCamera_FrameDrop(void *self, float dt, unsigned char flags);
00194800 void CRollerballCamera_UpdateEnd(void *self, float dt);
00194850 void CRollerballCamera_UpdateCountdown(void *self, float dt);
001948f0 void CRollerballCamera_UpdatePlay(void *self, float dt);
00194970 void CRollerballCamera_Update(void *self, float dt);

# RollerBall/CRollerball_World.cpp
00194a40 void CRollerballWorld_Ctor(void *self);
00194a50 void CRollerballWorld_Dtor(void *self);
00194a60 void CRollerballWorld_Shutdown(void *self);
00194bb0 void CRollerballWorld_Update(void *self, float dt);
00194c70 void CRollerballWorld_DrawBase(void *self);
00194e10 void CRollerballWorld_DrawShadowsBase(void *self);
00194e70 void CRollerballWorld_DrawSumo(void *self);
00195010 void CRollerballWorld_DrawRace(void *self);
00195170 void CRollerballWorld_DrawDrop(void *self);
001952f0 void CRollerballWorld_DrawShadows(void *self);
00195340 void CRollerballWorld_DrawPlatforms(void *self);
001953e0 void *CRollerballWorld_DeletingDtor(void *self, unsigned int flags);
00196500 void CRollerballWorld_Draw(void *self);
00196560 void CRollerballWorld_DrawShadowPass(void *self);
001965f0 int CRollerballWorld_LoadLOM(void *self, struct bkPackage *package, const char *path);
001966a0 int CRollerballWorld_Load(void *self, struct bkPackage *package, const char *level);

# RollerBall/CRollerball_Shadow.cpp
00196710 void CRollerballShadow_Render(void *self);
00196760 void CRollerballShadow_AddCaster(void *self, unsigned char **caster);
001967a0 void CRollerballShadowSystem_Add(void *self, void *shadow);
001967c0 void CRollerballShadowSystem_Update(void *self);
001967f0 int CRollerballShadow_Init(void *self, int type, int width, int height, void *player);
00196900 int CRollerballShadowSystem_Init(void *self);
00196910 void CRollerballShadowSystem_Shutdown(void *self);
00196940 void CRollerballShadowSystem_Begin(void *self);
00196a00 void CRollerballShadowSystem_End(void *self);

# RollerBall/CRollerBall_EntityEvent.cpp (0x00196a80/0x00196aa0 above)
00196a60 void CRollerBallEvent_Ctor(void *self);
00196a70 void CRollerBallEvent_Dtor(void *self);
00196ad0 void CRollerBallEntityEventSystem_InitQueue(void *self, unsigned int size);
00196b90 void *CRollerBallEvent_DeletingDtor(void *self, unsigned int flags);
00196bf0 void CRollerBallEntityEventSystem_Post(void *self, void *event);
00196d90 void *CRollerBallEntityEventSystem_DeletingDtor(void *self, unsigned int flags);

# RollerBall/CRollerBall_Water.cpp
00196de0 void CRollerBallWater_Ctor(void *self);
00196df0 void CRollerBallWater_Dtor(void *self);
00196e00 void CRollerBallWater_Shutdown(void *self);
00196e30 void CRollerBallWater_Update(void *self);
00196e80 void CRollerBallWater_Draw(void *self);
00196f70 void CRollerBallWater_Splash(void *self, float *pos, float depth, float inner, float outer);
00196fa0 void *CRollerBallWater_DeletingDtor(void *self, unsigned int flags);
00196fc0 int CRollerBallWater_Init(void *self, float *pos, float size, int nx, int nz);
00197150 void CRollerBallWater_SetRing(void *self, float inner, float outer);

# RollerBall/CRollerBall_Cutter.cpp
001973c0 void *CRollerBallCutter_Ctor(void *self);
001973e0 void CRollerBallCutter_Dtor(void *self);
001973f0 void CRollerBallCutter_Draw(void *self);
001974f0 void CRollerBallCutter_Reset(void *self);
00197540 void CRollerBallCutter_SetRadius(void *self, float radius);
00197550 void CRollerBallCutter_SetDuration(void *self, float seconds);
00197560 void CRollerBallCutter_SetReverse(void *self, int reverse);
00197570 void CRollerBallCutter_SetStartAngle(void *self, float angle);
00197580 void CRollerBallCutter_Pause(void *self);
00197590 void *CRollerBallCutter_DeletingDtor(void *self, unsigned int flags);
001975b0 int CRollerBallCutter_Init(void *self, float *centre, int segments);
001977d0 void CRollerBallCutter_Shutdown(void *self);
00197850 void CRollerBallCutter_BuildBeam(void *self, float dt);
00197bd0 void CRollerBallCutter_BuildScorch(void *self, float dt);
00197f40 void CRollerBallCutter_Start(void *self);
00197fa0 int CRollerBallCutter_Update(void *self, float dt);
