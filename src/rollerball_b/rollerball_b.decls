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
00198a20 void CRollerballPlayer_Dtor(void *self);
00198a30 int CRollerballPlayer_InitInput(void *self, int pad);
00198e30 void CRollerballPlayer_FreeInput(void *self);
00198e80 void CRollerballPlayer_FindTeamMate(void *self);
00198f10 void CRollerballPlayer_Draw(void *self);
00199040 void CRollerballPlayer_DrawReflection(void *self, int rider);
001990e0 void CRollerballPlayer_SetTargetHeading(void *self, float heading);
001990f0 void *CRollerballPlayer_DeletingDtor(void *self, unsigned int flags);
00199110 void CRollerballPlayer_Shutdown(void *self);
00199170 void CRollerballPlayer_Control(void *self, float dt);
001993b0 void CRollerballPlayer_Turn(void *self, float dt);
00199460 void CRollerballPlayer_UpdateFloor(void *self);
00199530 void CRollerballPlayer_ProcessEvents(void *self, float dt);
001999e0 void CRollerballPlayer_SetAnimMode(void *self, int mode);
00199ba0 int CRollerballPlayer_Init(void *self, int player, float *pos, float *rot, float mass);
00199f30 void CRollerballPlayer_Update(void *self, float dt);

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

# the race path record (file unknown: RollerBall/unplaced_path.c)
001980f0 void *CRollerBallPath_Ctor(void *self);
00198130 void CRollerBallPath_SetPoints(void *self, float *points, int count);
001981e0 unsigned int CRollerBallPath_Contains(void *self, float *pos);
00198340 float CRollerBallPath_SideOfLine(float *a, float *line);
00198370 void *CRollerBallPath_DeletingDtor(void *self, unsigned int flags);
001983a0 int CRollerBallPath_Collide(void *self, float *pos, float radius, float *out);

# CGameMode_RollerBall (file unknown: RollerBall/unplaced_gamemode.c)
00198600 void *CGameModeRollerBall_Ctor(void *self);
00198620 void CGameModeRollerBall_Dtor(void *self);
00198630 void CGameModeRollerBall_Enter(void *self);
00198650 void CGameModeRollerBall_Exit(void *self);
00198660 void CGameModeRollerBall_Initialise(void *self);
00198670 void CGameModeRollerBall_Start(void *self);
00198680 void CGameModeRollerBall_Nothing(void *self);
00198690 void CGameModeRollerBall_Update(void *self, int top);
001986a0 void CGameModeRollerBall_Process(void *self, int top);
001986b0 void CGameModeRollerBall_Draw(void *self, int top);
001986c0 void *CGameModeRollerBall_DeletingDtor(void *self, unsigned int flags);

# RollerBall/CRollerball_ASEImport.cpp (0x001989c0 keeps its table prototype: other modules call it)
001986e0 void CRollerBallAseShape_Ctor(void *self);
00198700 void CRollerBallAseShape_Dtor(void *self);
00198710 void *CRollerBallAseShape_DeletingDtor(void *self, unsigned int flags);
00198730 void CRollerBallAseShape_Read(void *self, void *reader);
00198840 void CRollerBallAseShape_ReadNamed(void *self, void *reader, const char *name);
001989a0 void CRollerBallAseReader_Ctor(void *self);
001989b0 void CRollerBallAseReader_Dtor(void *self);
001989e0 void *CRollerBallAseReader_DeletingDtor(void *self, unsigned int flags);

# shader fragment and player controllers (file unknown: RollerBall/unplaced_ai.c;
# the five constructors are listed above under "player controllers")
0019a4e0 void *CRollerBallShaderFragment_Ctor(void *self);
0019a520 void CRollerBallShaderFragment_Begin(void);
0019a560 void CRollerBallShaderFragment_End(void);
0019a590 void CRollerBallShaderFragment_SetConstants(void *self, int *ctx);
0019a620 int CRollerBallControl_Init(void *self, void *entity);
0019a660 void CRollerBallControlRace2_Reset(void *self);
0019a730 void *CRollerBallControl_PickRandomTarget(void *self);
0019a7c0 void *CRollerBallControl_Nearest(void *self, float *pos);
0019a840 void *CRollerBallControl_Farthest(void *self, float *pos);
0019a8c0 void *CRollerBallControl_InSights(void *self, float heading);
0019a9c0 void *CRollerBallControlBase_DeletingDtor(void *self, unsigned int flags);
0019a9e0 void CRollerBallControlBase_Think(void *self, float dt);
0019acc0 void CRollerBallControlBase_Chase(void *self);
0019ade0 void CRollerBallControlBase_Approach(void *self);
0019aee0 void CRollerBallControlBase_Reset(void *self);
0019b0e0 void CRollerBallControlBase_RollTimers(void *self);
0019b1f0 void *CRollerBallControlSumo_DeletingDtor(void *self, unsigned int flags);
0019b210 void CRollerBallControlArena_Attack(void *self);
0019b3a0 void CRollerBallControlArena_Recover(void *self);
0019b570 void CRollerBallControlSumo_Dodge(void *self, float dt);
0019b7a0 void CRollerBallControlArena_Hit(void *self, void *other);
0019b830 void CRollerBallControlSumo_Reset(void *self);
0019baa0 void CRollerBallControlArena_RollTimers(void *self);
0019bc40 void *CRollerBallControlRace2_DeletingDtor(void *self, unsigned int flags);
0019bc60 void CRollerBallControlRace_Seek(void *self);
0019bf90 void *CRollerBallControlRace_DeletingDtor(void *self, unsigned int flags);
0019bfb0 void CRollerBallControlRace_PickLane(void *self);
0019c000 void *CRollerBallControlDrop_DeletingDtor(void *self, unsigned int flags);
0019c020 void CRollerBallControlDrop_Dodge(void *self, float dt);
0019c250 void CRollerBallControlArena_ChooseTarget(void *self);
0019c420 void CRollerBallControlDrop_Reset(void *self);
0019c690 void CRollerBallControlSumo_Think(void *self, float dt);
0019c900 void CRollerBallControlRace_Think(void *self, float dt);
0019c960 void CRollerBallControlRace_Reset(void *self);
0019ca60 void CRollerBallControlDrop_Think(void *self, float dt);
0019ccd0 void CRollerBallControlBase_Update(void *self, float dt);
0019cd20 void CRollerBallControlSumo_Update(void *self, float dt);
0019cd90 void CRollerBallControlRace2_Update(void *self, float dt);
0019cdb0 void CRollerBallControlRace_Update(void *self, float dt);
0019cde0 void CRollerBallControlDrop_Update(void *self, float dt);

# RollerBall/CRollerBall_Particle.cpp (0x0019d0c0 and 0x0019d110 keep their table prototypes)
0019ce50 void CRollerBallParticles_Ctor(void *self);
0019ce60 void CRollerBallParticles_Draw(void *self);
0019d120 void CRollerBallParticles_Dtor(void *self);
0019d130 void *CRollerBallParticles_DeletingDtor(void *self, unsigned int flags);
0019d150 int CRollerBallParticles_Init(void *self, int *params, int count);
