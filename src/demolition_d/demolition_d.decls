# demolition_d prototype overrides (docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;   Only of_types.h / C types and "struct Name *".

# ---- Demolition/unplaced_ai_common.c (the brains: shared methods)
00105a10 int DemoAI_ThreatNear(struct DemoAI *self);
00105a70 int DemoAI_Init(struct DemoAI *self, char *owner);
00105aa0 void DemoAI2_OnPlayerGone(struct DemoAI *self, char *player);
00105b00 int DemoAI_ThreatUrgent(struct DemoAI *self);
00105b50 void DemoAI_GoToTarget(struct DemoAI *self);
00105b90 void DemoAI_GoRandom(struct DemoAI *self, float clearance);
00105d60 void DemoAI_GoNear(struct DemoAI *self, float *pos, float offset);
00105e10 void DemoAI_GoAroundTarget(struct DemoAI *self, float radius);
00105ef0 int DemoAI_FacingTarget(struct DemoAI *self, float maxAngle);
00105f70 float DemoAI_TargetDistXZ(struct DemoAI *self, float *pos);
00105fb0 float DemoAI_TargetDist(struct DemoAI *self, float *pos);
00106000 int DemoAI_Steer(struct DemoAI *self, float dt, float turnRate);
001060e0 char *DemoAI_PickRandom(struct DemoAI *self);
00106180 char *DemoAI_PickLeader(struct DemoAI *self);
00106210 char *DemoAI_PickNearest(struct DemoAI *self, float *pos);
001062a0 char *DemoAI_PickNemesis(struct DemoAI *self);

# ---- Demolition/unplaced_ai_v0.c (variation 0 brain, vtable 0x0026bb58)
001059e0 void DemoAI0_StateWait(struct DemoAI *self, float dt);
00106340 struct DemoAI *DemoAI0_Ctor(struct DemoAI *self);
00106350 struct DemoAI *DemoAI0_Delete(struct DemoAI *self, unsigned int flags);
00106370 void DemoAI0_Scan(struct DemoAI *self, float dt);
001065c0 void DemoAI0_StateAttack(struct DemoAI *self, float dt);
00106830 void DemoAI0_StateWander(struct DemoAI *self, float dt);
001068c0 void DemoAI0_OnHit(struct DemoAI *self, char *attacker);
001069d0 void DemoAI0_Flee(struct DemoAI *self, float dt);
00106bb0 void DemoAI0_Chase(struct DemoAI *self, float dt);
00106e80 void DemoAI0_Reset(struct DemoAI *self);
001071f0 void DemoAI0_Restart(struct DemoAI *self);
00109ec0 void DemoAI0_StateHunt(struct DemoAI *self, float dt);
00109f30 void DemoAI0_StateFlee(struct DemoAI *self, float dt);
00109f70 void DemoAI0_ModeRandom(struct DemoAI *self, float dt);
0010a0a0 void DemoAI0_ModeNearest(struct DemoAI *self, float dt);
0010a1d0 void DemoAI0_ModeRevenge(struct DemoAI *self, float dt);
0010a300 void DemoAI0_ModeNemesis(struct DemoAI *self, float dt);
0010a430 void DemoAI0_ModeLeader(struct DemoAI *self, float dt);
0010b8b0 void DemoAI0_Think(struct DemoAI *self, float dt);
0010bbd0 void DemoAI0_Process(struct DemoAI *self, float dt);

# ---- Demolition/unplaced_ai_v1.c (variation 1 brain, vtable 0x0026bb7c)
00105a60 struct DemoAI *DemoAI1_Ctor(struct DemoAI *self);
00107370 struct DemoAI *DemoAI1_Delete(struct DemoAI *self, unsigned int flags);
00107390 void DemoAI1_Scan(struct DemoAI *self, float dt);
00107500 void DemoAI1_StateStart(struct DemoAI *self, float dt);
00107550 void DemoAI1_SteerBomb(struct DemoAI *self, float dt);
00107600 void DemoAI1_SetMode(struct DemoAI *self, int mode);
00107810 void DemoAI1_OnHit(struct DemoAI *self, char *attacker);
001078c0 void DemoAI1_Evade(struct DemoAI *self, float dt);
00107aa0 void DemoAI1_Wander(struct DemoAI *self, float dt);
00107be0 void DemoAI1_Reset(struct DemoAI *self);
00107f70 void DemoAI1_Restart(struct DemoAI *self);
0010a560 void DemoAI1_StateRoam(struct DemoAI *self, float dt);
0010a6f0 void DemoAI1_StateEvade(struct DemoAI *self, float dt);
0010a8b0 void DemoAI1_StateHold(struct DemoAI *self, float dt);
0010a930 void DemoAI1_ModeRandom(struct DemoAI *self, float dt);
0010aab0 void DemoAI1_ModeNearest(struct DemoAI *self, float dt);
0010abd0 void DemoAI1_ModeRevenge(struct DemoAI *self, float dt);
0010ad40 void DemoAI1_ModeNemesis(struct DemoAI *self, float dt);
0010aec0 void DemoAI1_ModeLeader(struct DemoAI *self, float dt);
0010ba40 void DemoAI1_Think(struct DemoAI *self, float dt);
0010bc20 void DemoAI1_Process(struct DemoAI *self, float dt);

# ---- Demolition/unplaced_ai_v2.c (variation 2 brain, vtable 0x0026bba0)
00105a90 struct DemoAI *DemoAI2_Ctor(struct DemoAI *self);
00108110 struct DemoAI *DemoAI2_Delete(struct DemoAI *self, unsigned int flags);
00108130 void DemoAI2_Scan(struct DemoAI *self, float dt);
001083d0 void DemoAI2_StateAttack(struct DemoAI *self, float dt);
00108480 void DemoAI2_OnHit(struct DemoAI *self, char *attacker);
001085b0 void DemoAI2_Reset(struct DemoAI *self);
00108900 void DemoAI2_Restart(struct DemoAI *self);
00108b60 void DemoAI2_Flee(struct DemoAI *self, float dt);
00108c60 char *DemoAI2_PickToken(struct DemoAI *self);
0010afe0 void DemoAI2_StateCollect(struct DemoAI *self, float dt);
0010b210 void DemoAI2_StateFlee(struct DemoAI *self, float dt);
0010bc50 void DemoAI2_Process(struct DemoAI *self, float dt);

# ---- Demolition/unplaced_ai_v3.c (variation 3 brain, vtable 0x0026bbc4)
00105ac0 struct DemoAI *DemoAI3_Ctor(struct DemoAI *self);
00105ad0 void DemoAI3_StateStart(struct DemoAI *self, float dt);
00108ec0 struct DemoAI *DemoAI3_Delete(struct DemoAI *self, unsigned int flags);
00108ee0 void DemoAI3_Scan(struct DemoAI *self, float dt);
001091e0 void DemoAI3_StateFetch(struct DemoAI *self, float dt);
00109290 void DemoAI3_StateCarry(struct DemoAI *self, float dt);
00109640 void DemoAI3_StateEvade(struct DemoAI *self, float dt);
001097d0 void DemoAI3_StateThrow(struct DemoAI *self, float dt);
001098d0 void DemoAI3_OnHit(struct DemoAI *self, char *attacker);
00109980 void DemoAI3_Reset(struct DemoAI *self);
00109d10 void DemoAI3_Restart(struct DemoAI *self);
0010b250 void DemoAI3_ModeRandom(struct DemoAI *self, float dt);
0010b3a0 void DemoAI3_ModeNearest(struct DemoAI *self, float dt);
0010b4e0 void DemoAI3_ModeRevenge(struct DemoAI *self, float dt);
0010b620 void DemoAI3_ModeNemesis(struct DemoAI *self, float dt);
0010b770 void DemoAI3_ModeLeader(struct DemoAI *self, float dt);
0010bb00 void DemoAI3_Think(struct DemoAI *self, float dt);
0010bce0 void DemoAI3_Process(struct DemoAI *self, float dt);

# ---- ClassLib/unplaced_token.c (token helpers; ClassLib/CToken.cpp by content)
0010c200 void *CToken_Next(void *token);

# ---- Demolition/unplaced_charlibai.c (CharLib AI base, vtable 0x0026bdc0, and table thunks)
0010d220 int DemoCLAI_Cond00(void *ai);
0010d230 int DemoCLAI_Cond01(void *ai);
0010d240 int DemoCLAI_Cond02(void *ai);
0010d250 int DemoCLAI_Cond03(void *ai);
0010d260 int DemoCLAI_Cond04(void *ai);
0010d270 int DemoCLAI_Cond05(void *ai);
0010d280 int DemoCLAI_Cond06(void *ai);
0010d290 int DemoCLAI_Cond07(void *ai);
0010d2a0 int DemoCLAI_Cond08(void *ai);
0010d2b0 int DemoCLAI_Cond09(void *ai);
0010d2c0 int DemoCLAI_Cond10(void *ai);
0010d2d0 int DemoCLAI_Cond11(void *ai);
0010d2e0 int DemoCLAI_Cond12(void *ai);
0010d2f0 int DemoCLAI_Cond13(void *ai);
0010d300 int DemoCLAI_Cond14(void *ai);
0010d310 int DemoCLAI_Cond15(void *ai);
0010d320 int DemoCLAI_Cond16(void *ai);
0010d330 int DemoCLAI_Cond17(void *ai);
0010d340 void DemoCLAI_State00(void *ai, int entered);
0010d360 void DemoCLAI_State01(void *ai, int entered);
0010d380 void DemoCLAI_State02(void *ai, int entered);
0010d3a0 void DemoCLAI_State03(void *ai, int entered);
0010d3c0 void DemoCLAI_State04(void *ai, int entered);
0010d3e0 void DemoCLAI_State05(void *ai, int entered);
0010d400 void DemoCLAI_State06(void *ai, int entered);
0010d420 void DemoCLAI_State07(void *ai, int entered);
0010d440 void DemoCLAI_State08(void *ai, int entered);
0010d460 void DemoCLAI_State09(void *ai, int entered);
0010d480 void DemoCLAI_State10(void *ai, int entered);
0010d4a0 void DemoCLAI_State11(void *ai, int entered);
0010d4c0 void DemoCLAI_State12(void *ai, int entered);
0010d4e0 void DemoCLAI_State13(void *ai, int entered);
0010d500 void DemoCLAI_State14(void *ai, int entered);
0010d520 void DemoCLAI_State15(void *ai, int entered);
0010d540 void DemoCLAI_State16(void *ai, int entered);
0010d560 void DemoCLAI_State17(void *ai, int entered);
0010d580 void *DemoCLAI_Ctor(void *self);
0010d5a0 void DemoCLAI_Dtor(void *self);

# ---- Demolition/CDemolition_Crate.c (crates, vtable 0x0026be9c)
0010d5b0 void *DemoCLAI_DeletingDtor(void *self, unsigned int flags);
0010d5d0 void *DemoCrate_Ctor(void *self);
0010d5f0 void DemoCrate_Dtor(void *self);
0010d600 void DemoCrate_Shutdown(void *self);
0010d620 void DemoCrate_CheckGround(void *self, float dt);
0010d680 void DemoCrate_Draw(void *self);
0010d6a0 void DemoCrate_Reset(void *self);
0010d6d0 void *DemoCrate_DeletingDtor(void *self, unsigned int flags);
0010d6f0 void DemoCrate_Init(void *self);
0010d830 void DemoCrate_Fall(void *self, float dt);
0010d960 void DemoCrate_Damage(void *self, float *pos, float radius);
0010db40 void DemoCrate_Update(void *self, float dt);

# ---- ClassLib/unplaced_token.c (token assets, glow pass, list walk)
0010c080 void CToken_DrawGlowSprites(void);
0010c190 void CToken_LoadAssets(int shadowMode, void *shadowTexture);
0010ce90 void CToken_UpdateAll(void);

# ---- Demolition/CDemolition_EntityEvent.c (entity event queue, vtables 0x0026bc00 / 0x0026bc04)
0010bd10 void *DemoEntityEvent_Ctor(void *event);
0010bd20 void DemoEntityEvent_Dtor(void *event);
0010bd30 void *DemoEntityEventSystem_Ctor(void *system);
0010bd50 void DemoEntityEventSystem_Dtor(void *system);
0010bd80 void DemoEntityEventSystem_InitQueue(int system, unsigned int size);
0010be40 void *DemoEntityEvent_Delete(void *event, unsigned int flags);
0010bea0 void DemoEntityEventSystem_Push(int system, int event);
0010c030 void *DemoEntityEventSystem_Delete(void *system, unsigned int flags);

# ---- Demolition/CDemolition_Event.c (CharLib event queue, vtables 0x0026bd20 / 0x0026bd24)
0010cec0 void *DemoCharEvent_Ctor(void *event);
0010ced0 void DemoCharEvent_Dtor(void *event);
0010cee0 void *DemoCharEventSystem_Ctor(void *system);
0010cf00 void DemoCharEventSystem_Dtor(void *system);
0010cf30 void DemoCharEventSystem_InitQueue(int system, unsigned int size);
0010cff0 void *DemoCharEvent_Delete(void *event, unsigned int flags);
0010d050 void DemoCharEventSystem_Push(int system, int event);
0010d1d0 void *DemoCharEventSystem_Delete(void *system, unsigned int flags);

# ---- Demolition/CDemolition_Wall.c (walls, vtable 0x0026bef0; 0x0010db80/0x0010db90 are not in the table)
0010dba0 void DemoWall_SetY(int self, float y);
0010dbc0 void DemoWall_SetAngle(int self, float angle);
0010dbe0 void DemoWall_SetTexture(int self, int texture);
0010dbf0 void DemoWall_DrawFlying(struct DemoWall *self);
0010dc60 void DemoWall_SetColours(struct DemoWall *self);
0010dce0 void DemoWall_ResetBricks(struct DemoWall *self);
0010ddb0 void DemoWall_UpdateFaces(struct DemoWall *self);
0010dff0 struct DemoWall *DemoWall_Delete(struct DemoWall *self, unsigned int flags);
0010e010 void DemoWall_Shutdown(int self);
0010e050 void DemoWall_SetPosition(int self, float *pos);
0010e080 void DemoWall_Draw(int self);
0010e120 void DemoWall_Build(struct DemoWall *self, unsigned int flags);
0010efd0 void DemoWall_Blast(int self, float *pos, float radius);
0010f370 void DemoWall_UpdateFlying(struct DemoWall *self, float dt);
0010f4c0 void DemoWall_Update(int self, float dt);
0010f5b0 void DemoWall_Reset(int self);
0010f5e0 int DemoWall_Init(int self, float width, float height, float depth, int rows, int cols);
