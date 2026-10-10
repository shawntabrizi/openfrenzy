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
