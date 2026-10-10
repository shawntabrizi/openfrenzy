# mousewhack prototype overrides (docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;   Only of_types.h / C types and "struct Name *".

# ---- MouseWhack/CModule_MouseWhack.c
0016cce0 unsigned int *CModule_MouseWhack_Ctor(unsigned int *self);
0016cd10 void CModule_MouseWhack_Dtor(unsigned int *self);
0016cd20 unsigned int *CModule_MouseWhack_Destroy(unsigned int *self, unsigned char flags);
0016cd40 int CModule_MouseWhack_Start(unsigned int *self, int variation);

# ---- MouseWhack/CGameMode_MouseWhack.c
0016ceb0 void *CGameMode_MouseWhack_Ctor(void *self);
0016ced0 void CGameMode_MouseWhack_Dtor(void *self);
0016cee0 void CGameMode_MouseWhack_Enter(void *self);
0016cf30 void CGameMode_MouseWhack_Process(void *self, int top);
0016d070 void MouseWhack_DrawActorsFirstPass(void);
0016d150 void MouseWhack_DrawActors(void);
0016d1d0 void CGameMode_MouseWhack_Start(void *self);
0016d2b0 void *CGameMode_MouseWhack_Destroy(void *self, unsigned char flags);
0016d2d0 void MouseWhack_DrawBugShadows(void);
0016d780 void CGameMode_MouseWhack_Init(void *self);
0016f920 void CGameMode_MouseWhack_Exit(void *self);
0016fe50 void CGameMode_MouseWhack_Draw(void *self, int top);

# ---- MouseWhack/CBbash_Arena.c
001708b0 int CBbash_Arena_LoadLOMFile(void *self, void *package, const char *name);
00170940 void CBbash_Arena_FindClearLeader(void *self);
00170a00 void CBbash_Arena_FindLeaders(void *self);
00170a80 void CBbash_Arena_FindLeadingTeam(void *self);
00170b80 void CBbash_Arena_UpdateBugNumbers(void *self);
00170c00 void CBbash_Arena_DrawDebug(void *self);
00170ea0 void *CBbash_Arena_Ctor(void *self);
00170eb0 void CBbash_Arena_Dtor(void *self);
001725d0 short CBbash_Arena_ChoosePlayerToAttack(void *self);
00172660 int CBbash_Arena_DecideBugType(void *self);

# ---- MouseWhack/CBbash_Game.c
001729a0 void CBbash_Game_FollowCamera(void *self);
00172b80 void CBbash_Game_LoadPackages(void *self, int unused);
00172c90 void CBbash_Game_FollowCameraSmooth(void *self);
00172fd0 void CBbash_Game_LookAtWinner(void *self);
001730e0 void CBbash_Game_HidePlayers(void *self);
00173190 void CBbash_Game_RestorePlayers(void *self);
001731d0 void CBbash_Game_StartOutro(void *self);
00173470 void CBbash_Game_Outro(void *self);
00173970 void CBbash_Game_TrainPush(void *self, float *out, int unused);
00173f80 void CBbash_Game_Intro(void *self);
00174610 void CBbash_Game_Play(void *self);
00175b70 void CBbash_Game_Update(void *self);

# ---- MouseWhack/CBbash_Player.c
0017d180 void CBbash_Player_UpdateAction(void *self);
0017d200 void CBbash_Player_CheckChasedBug(void *self);
0017d2f0 void CBbash_Player_UpdateKnockCount(void *self);
0017d330 void CBbash_Player_CreateInputMaps(void);
0017d510 float MW_RandSigned(void);
0017d550 void CBbash_Player_SetModel(void *self, int lowDetail);
0017d6d0 void *CBbash_Player_Ctor(void *self, int player, float *pos);
0017df00 void CBbash_Player_UpdatePlatform(void *self);
0017e050 void CBbash_Player_ChooseEnemy(void *self);
0017e1d0 void CBbash_Player_ChooseThreat(void *self);
0017e2d0 void CBbash_Player_ChooseBug(void *self);
0017e640 int CBbash_Player_Tag(void *self);
0017eb80 void CBbash_Player_LookAround(void *self);
0017ed40 void CBbash_Player_Hit(void *self, float *from, int knockedDown);
0017f000 int CBbash_Player_Steer(void *self);
0017f8a0 void CBbash_Player_Strike(void *self);

# ---- MouseWhack/CBbash_Bullets.c
00180720 void *CBbash_Bullets_Ctor(void *self);
001807c0 void *CBbash_Bullet_Ctor(void *self, float *pos, float *vel, short type);
001808d0 void CBbash_Bullet_Update(void *self);
00180c50 void CBbash_Bullets_Dtor(void *self);
00180ce0 void CBbash_Bullets_Fire(void *self, float *from, float *dir, short player);
00180de0 void CBbash_Bullets_Spit(void *self, float *from, float *to);
00180f00 void CBbash_Bullets_Update(void *self);

# ---- MouseWhack/CBBash_Destructable.c
00180f60 void CBBash_Destructable_Update(void *self);
00181040 void CBBash_Destructable_Draw(void *self);
00181520 void CBBash_Destructable_Destroy(void *self);

# ---- MouseWhack/CBbash_Bugs.c
00181730 void *MW_Ring_Ctor(void *self);
00181740 void MW_Ring_Update(void *self);
00181770 void MW_LoadBugModels(void);
001817c0 void *CBbash_Bug_Ctor(void *self);
001817d0 void CBbash_Bug_Dtor(void *self);
00181830 void CBbash_Bug_Update(void *self);
00181d50 void Bug_PaintTag(void *self, short player);
00181fc0 void CBbash_Bug_Spawn(void *self, short type);

# ---- MouseWhack/unplaced.c
00176100 void *MW_Timer_Ctor(void *self);
00176140 void MW_Timer_Dtor(void *self);
00176150 int MW_Timer_Reset(void *self);
001761a0 void MW_Timer_Update(void *self);
00176230 void *MW_Timer_Destroy(void *self, unsigned char flags);
00176250 void Bug_Fly_Die(void *self);
001763d0 void Bug_Rubble_Update(void *self);
00176540 void Bug_Tremor_InHole(void *self);
00176670 int Bug_ReachedPoint(void *self);
00176770 void Bug_Roach_Dead(void *self);
001767d0 void Bug_Worm_Dead(void *self);
00176830 unsigned int Bug_Spore_Start(void *self);
00176ac0 void Bug_Spore_Fly(void *self);
00177050 void Bug_Hit(void *self, int scored);
00177520 void Bug_Train_Run(void *self);
001776e0 unsigned int Bug_Train_Start(void *self);
001778f0 void Bug_Fly_Attack(void *self);
00177fd0 unsigned int Bug_Fly_Start(void *self);
001786e0 void Bug_Fly_Wander(void *self);
00178d30 void Bug_Fly_Knocked(void *self);
00178e80 void Bug_Tremor_Aim(void *self, float *target);
00179540 void Bug_Tremor_Die(void *self);
001796f0 void Bug_Tremor_Attack(void *self);
00179ba0 void Bug_Tremor_Watch(void *self);
00179e10 void Bug_Tremor_Rise(void *self);
0017ac20 void Bug_Roach_ChoosePoint(void *self);
0017b870 void Bug_SpawnBrawl(void *self);
0017b9c0 void Bug_SpawnBurgers(void *self);
0017bb40 int Bug_SpawnPiedPipers(void *self);
0017c490 void Bug_Roach_Start(void *self);
0017c7b0 void Bug_Roach_Patrol(void *self);
0017a530 void Bug_Roach_GoHome(void *self);
0017a830 void Bug_Roach_AttackStand(void *self);
0017cd90 void Bug_Roach_Update(void *self);
0017adb0 void Bug_Worm_DropIn(void *self);
0017afb0 void Bug_Worm_GoHome(void *self);
0017b2d0 void Bug_Worm_Wander(void *self);
0017cbd0 void Bug_Worm_Start(void *self);
0017ce20 void Bug_Worm_Update(void *self);
0017c360 void Bug_Train_Update(void *self);
0017c3a0 void Bug_Fly_Update(void *self);
0017c410 void Bug_Tremor_Update(void *self);
0017bd90 void Bug_Spore_Burst(void *self);
0017cd40 void Bug_Spore_Update(void *self);
0017ce90 void MW_LoadSounds(struct bkPackage *package);
0017cf10 int MW_PlaySound(unsigned int index, int freqChange, int volume);
0017cfc0 int MW_PlaySoundAt(unsigned int index, int freqChange, float *pos);
0017d090 void MW_StopSound(int channel);
0017d0b0 void MW_Player_PlayVoice(void *player, int kind, int volume, float cooldown, unsigned int flags, int choice, int allowRepeat);
0017d100 void MW_Player_SmashHitProp(void *player);

# ---- provisional shapes (functions not written yet)
001839b0 void FUN_001839b0(void);
00182c10 void FUN_00182c10(void);
001836b0 void FUN_001836b0(short index);
00183300 void FUN_00183300(void);
00186b00 void FUN_00186b00(void);
00182f50 void FUN_00182f50(void);
00182ea0 void FUN_00182ea0(void);
001853e0 void FUN_001853e0(void);
00185110 void FUN_00185110(void);
00184930 void FUN_00184930(float *pos, void *player, float angle);
00184bb0 void *FUN_00184bb0(void *bug);
001842b0 void FUN_001842b0(void *bug, int arg);
00183a70 void FUN_00183a70(float *pos, short player);
00186280 void FUN_00186280(void);
00184760 void Splat_AddCrumbs(float *pos, int unused);
