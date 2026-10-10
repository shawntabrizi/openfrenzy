# charlib prototypes (see docs/ARCHITECTURE.md section 11): <addr> <prototype>;
# Route.cpp
000ab4e0 void CLRoute_ClearKnown(void);
000ab570 void CLRoute_SampleHeights(void);
000ab9f0 int CLRoute_Search(int start, struct CLRoute *r);
000ad4b0 int CLRoute_CellAt(float *pos);
000ad520 void CLRoute_CellCentre(float *out, int cell);
000ad5c0 void CLRoute_SetHeights(struct CLRouteHeights *heights);
000ad620 void CLRoute_DrawDebug(void);
000ad7d0 struct CLRouteHeights *CLRouteHeights_Ctor(struct CLRouteHeights *self);
000ad810 void CLRouteHeights_Dtor(struct CLRouteHeights *self);
000ad880 void CLRouteHeights_Free2(struct CLRouteHeights *self);
000ad8b0 struct CLRoute *CLRoute_Ctor(struct CLRoute *self);
000ad930 void CLRoute_Dtor(struct CLRoute *self);
000ad980 void CLRoute_Rebuild(void);
000ad990 int CLRoute_Update(struct CLRoute *r);
000adb70 int CLRoute_Start(struct CLRoute *r, float *from, float *to, int arg4);
000adc30 void CLRoute_RandomPoint(float *out, int need_ground);
# CRCollision.cpp area
000b3ff0 float CLColl_GroundProbe(float *pos, float radius, void **tri_out, void **platform_out, int mode);
# AI.cpp area
000b6860 int CLRoute_NextCell(float *from, float *to, int arg3, struct CLRoute *r);
# Actor.cpp
000adef0 void Actor_InitCharData(void *actor);
000adfc0 void *Actor_Delete(void *actor);
000ae1b0 void Actor_DrawShadowMeshes(void);
000ae210 void Actor_DrawOpaque(void *actor);
000ae2f0 void Actor_DrawFaded(void *actor);
000ae4d0 void Actor_PrepAll(void);
000ae550 void Actor_CollMeshName(char *out, const char *name);
000ae5d0 void Actor_ShadowMeshName(char *out, const char *name);
000ae650 float Actor_AnimProgress(void *inst);
000ae820 void Actor_SetFade(void *actor, float target, float frames);
000ae870 void *Actor_Update(void *actor);
000af1a0 void *Actor_Create(void *model, float *pos, void *mem, int type, int at_tail, int link);
000af350 void Actor_DeleteAll(void);
000af380 void Actor_UpdateAnimations(void);
000af580 void Actor_SwitchAnim(void *actor, int index, float blend);
000af690 void Actor_PlayAnim(void *actor, int index, int loop, int queue, float speed, float blend, int sync);
000af8f0 void Actor_SetConstantAlphaStage(int alpha);
000af970 void Actor_FacePoint(void *actor, float *point);
000af9e0 void Actor_UpdateAll(void);
000afa10 void *Actor_Load(const char *name, void *package, float *pos, int type, void *mem, int no_collision);
# gap after Actor.cpp
000add50 void CLMath_TurnTowards(float *angle, float target, float rate, float smoothing);
000b1d20 void Actor_UpdateKnockback(void *actor);
000b3d50 void CRColl_BuildMesh(void *actor);
000b3dc0 void CRColl_BuildMeshFrom(void *actor, void *inst);
000b6270 void CRColl_MoveActor(void *actor);
000b69d0 void Lights_EnableAll(int enable, int restore_ambient);
000b7270 void Light_SetPosition(void *light, float *pos);
000b8520 void Platform_RotateActor(void *actor);
000b8960 void Platform_GetVelocity(float *out, void *platform, int absolute);
000b8a40 void Platform_AddVelocity(float *vel, float *add);
