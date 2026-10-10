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
