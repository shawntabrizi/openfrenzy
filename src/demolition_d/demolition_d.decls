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
