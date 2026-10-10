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

# ---- MouseWhack/unplaced.c
00176100 void *MW_Timer_Ctor(void *self);
00176140 void MW_Timer_Dtor(void *self);
00176150 int MW_Timer_Reset(void *self);
001761a0 void MW_Timer_Update(void *self);
00176230 void *MW_Timer_Destroy(void *self, unsigned char flags);

# ---- provisional shapes (functions not written yet)
00172b80 void FUN_00172b80(void *self, int unused);
00175b70 void FUN_00175b70(void *self);
0017cf10 int FUN_0017cf10(unsigned int sound, int arg, int volume);
0017ce90 void FUN_0017ce90(int arg);
001839b0 void FUN_001839b0(void);
00181770 void FUN_00181770(void);
0017d6d0 void *FUN_0017d6d0(void *self, int index, float *pos);
00181730 void *FUN_00181730(void *self);
0017d330 void FUN_0017d330(void);
00180720 void *FUN_00180720(void *self);
00180c50 void FUN_00180c50(void *self);
00182c10 void FUN_00182c10(void);
00181520 void FUN_00181520(void *self);
001817d0 void FUN_001817d0(void *self);
00181040 void FUN_00181040(void *self);
001836b0 void FUN_001836b0(short index);
00183300 void FUN_00183300(void);
00186b00 void FUN_00186b00(void);
00182f50 void FUN_00182f50(void);
00182ea0 void FUN_00182ea0(void);
001853e0 void FUN_001853e0(void);
00185110 void FUN_00185110(void);
