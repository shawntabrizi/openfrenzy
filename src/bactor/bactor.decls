# bactor: true prototypes of the BACTOR functions (see docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;

# ---------- BACTOR/actor.c
00034730 int baInitialise(void);
00034780 void baShutdown(void);
000347a0 int bCountNodeDefs(struct BActorNodeDef *first);
000347e0 struct BActorNode *bInitNodes(struct BActorNodeDef *first, struct BActorNode *node, struct BActorNode *parent, int numSlots);
00034930 void baDeleteActorInstance(struct BActorInstance *inst);
00034990 void bFixupNodeDefs(char *base, struct BActorNodeDef *first, int depth, struct BActor *actor);
00034b50 struct BActor *baLoadActor(struct BActor *item, void *package);
00034bf0 void baPrepActorInstance(struct BActorInstance *inst, int flags);
00034d50 int baDrawActorInstance(struct BActorInstance *inst, int passes);
00034dd0 void bReleaseNodeDefs(struct BActorNodeDef *first);
00034e30 void baFreeActor(struct BActor *actor);
00034e70 void baSetNodeTime(struct BActorInstance *inst, struct BActorNode *node, int time, int recurse);
00034f60 void baSetNodeTimeBlend(struct BActorInstance *inst, struct BActorNode *node, int time, int time2, float t, int recurse);
00035110 void bSendAnimEvents(int inclusive, int to, struct BActorInstance *inst, struct BActorNodeDef *def, int from, int dir, struct BActorNode *node, struct BAnim *anim);
000354e0 void bQueueAnim(struct BActorInstance *inst, struct BActorNode *node, int mode, struct BAnim *anim, struct BAnim *anim2, int speed, int speed2, int time, int time2, float blendSpeed, uint32_t flags, int (*callback)(), void *context);
00035770 void baQueueAnimation(struct BActorInstance *inst, struct BActorNode *node, struct BAnim *anim, int speed, int time, uint32_t flags, int (*callback)(), void *context);
00035810 void baQueueBlendedAnimation(struct BActorInstance *inst, struct BActorNode *node, struct BAnim *anim, struct BAnim *anim2, int speed, int speed2, int time, int time2, float blendSpeed, uint32_t flags, int (*callback)(), void *context);
000358d0 void baQueueAnimationBlendIn(struct BActorInstance *inst, struct BActorNode *node, struct BAnim *anim, int speed, int time, float blendSpeed, uint32_t flags, int (*callback)(), void *context);
00035970 void baQueueCopyAnimation(struct BActorInstance *inst, struct BActorNode *node, struct BActorNode *source, float blendSpeed, uint32_t flags, int (*callback)(), void *context);
00035aa0 void bRebindNodes(struct BActorNodeDef *first, struct BActorNode *node);
00035b10 void baSetInstanceAnimations(struct BActor *actor, struct BActorInstance *inst);
00035b40 struct BActorNode *baFindNodeByHash(struct BActorNode *first, uint32_t hash);
00035b90 struct BActorNode *baFindNode(struct BActorNode *first, const char *name);
00035bb0 void baHideNode(struct BActorNode *node, int recurse);
00035bf0 void baShowNode(struct BActorNode *node, int recurse);
00035c30 struct BActorSubObject *baFindSubObjectByHash(struct BActorInstance *inst, uint32_t hash);
00035c70 struct BActorSubObject *baFindSubObject(struct BActorInstance *inst, const char *name);
00035cc0 void baSetRenderPass(struct BActorInstance *inst, int pass, int mode, int param);
00035ce0 void baGetNodePosition(struct BActorInstance *inst, struct BActorNode *node, float *out);
00035d50 void baSetBrightness(int brightness);
00035d70 void baChangeAnimation(struct BActorNode *node, int recurse, int speed, int speed2, float blendSpeed, float blend, uint32_t mask);
00035e60 struct BVertexStreams *bCountVertices(struct BActorInstance *inst, struct BActorNode *node, int stream, int *numVerts, int *numIndices);
00035f00 char *bExtractVertices(struct BActorInstance *inst, struct BActorNode *node, int stream, char *out, int *base, uint32_t flags);
000366f0 void *baGetActorVertices(struct BActorInstance *inst, struct BActorNode *node, int stream, int *numVerts, int *numIndices, unsigned int *size, void *buffer, uint32_t flags);
00036870 int baIsAnimating(struct BActorInstance *inst, struct BActorNode *node, int recurse, struct BAnim **anim);
00036940 void baCopyAnimationState(struct BActorInstance *dstInst, struct BActorNode *dst, struct BActorInstance *srcInst, struct BActorNode *src, int recurse);
00036a20 void baSetNodeRotation(struct BActorNode *node, const float *quat);
00036ac0 void bVec3Cross(float *out, const float *a, const float *b);
00036b00 float bVec3Dot(const float *a, const float *b);
00036b20 struct BActorInstance *baInitActorInstance(struct BActorInstance *inst, struct BActor *actor, int numSlots, const char *file, int line);
00036cb0 void bBlendNodeToTime(struct BActorInstance *inst, struct BActorNode *node, int time, float t, int recurse);
00036e70 int bUpdateNodes(struct BActorInstance *inst, struct BActorNode *first, float dt, float *rootMotion, struct BAnimEvent *events, int numEvents, int *numActive);
000380f0 int baUpdateAnimations(struct BActorInstance *inst, struct BActorNode *node, float dt, float *rootMotion);
00038190 void baFlushAnimations(void *inst, struct BActorNode *node, int recurse, int keep);

# ---------- BACTOR/lights.c
000382c0 int baInitLights(void);
000382f0 struct BLight *baCreateLight(struct BLight *light, uint32_t flags);
00038370 void baEnableLight(struct BLight *light, int enable);
00038440 void baSetAmbientLight(int r, int g, int b);
00038470 void baSetLightColour(struct BLight *light, int r, int g, int b);
000384a0 void bVec3Divide(float *out, const float *v, float d);
000384d0 void baDeleteLight(struct BLight *light);
00038550 void baSetDirectionalLight(struct BLight *light, const float *dir);
000385a0 void baSetPointLight(struct BLight *light, const float *pos, float range);
000385e0 void baSetSpotLight(struct BLight *light, const float *pos, const float *target, float range, float angle);

# ---------- BACTOR/pcxbActor.c
00038670 void bFixupSoftSkin(struct BActor *actor);
00038830 void bMatMulTranspose(float *out, const float *a, const float *b, int rows);
00038a60 void bDrawGroupsUntextured(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag);
00038ad0 void bDrawGroupsTex0(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag);
00038bd0 void bDrawGroupsTex01(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag);
00038ce0 void bSetNodeTransformConstants(struct BActorInstance *inst, struct BActorNode *node);
00038d90 void bFindSilhouettes(struct BActorNode *first, struct BActorInstance *inst, struct BShadowBuild *build);
00039000 void bBuildShadowQuads(struct BActorNode *first, struct BActorInstance *inst, struct BShadowBuild *build);
000393b0 void baDeleteShadowVolume(struct BShadowVolume *shadow);
000393e0 void bUpdateNodeMatrices(struct BActorNode *first, const float *parentMatrix, const float *parentScale, struct BActorInstance *inst);
000395b0 void bReleaseSoftSkin(struct BActor *actor);
000395d0 void bDrawGroupsTex0Wrap(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag);
000398e0 void bDrawGroupsTex01Wrap(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag);
00039ac0 void bDrawMesh(struct BMesh *mesh, struct BActorInstance *inst, uint32_t passes, struct BActorNode *node);
00039e20 struct BShadowVolume *baCreateShadowVolume(struct BActorInstance *inst, struct BShadowVolume *shadow, const float *light, float extrude, uint32_t flags);
00039fa0 void baDrawShadowVolume(struct BShadowVolume *shadow);
0003a0e0 void baDrawShadowFill(uint32_t r, uint32_t g, uint32_t b, uint32_t a);
0003a220 void bDrawNodes(struct BActorNode *first, struct BActorInstance *inst, int passes);

# ---------- BACTOR/pcxbShaderSelector.c
0003ace0 void bShaderEndPass(struct BActorInstance *inst, int pass);
0003bd60 int bShaderSelect(struct BActorInstance *inst, int pass, struct BMesh *mesh);

# ---------- BACTOR/camera.c
0003bef0 int baInitCameras(void);
0003c390 void baDeleteAllCameras(void);

# ---------- BACTOR/extras.c
0003c3c0 int baInitExtras(void);
0003ca10 void baDeleteAllExtras(void);

# ---------- BACTOR/unplaced.c
0003ca40 int baInitSets(void);
0003cae0 void bGetKey3(float *out, struct BKeyTrack3 *track, int time);
0003cca0 void bGetKey4(float *out, struct BKeyTrack4 *track, int time);
0003cfa0 void bDrawSoftSkin(struct BActorInstance *inst, int passes);
0003d0d0 void bReleaseMesh(struct BActorNodeDef *def);
0003d8f0 void bFixupMesh(char *base, struct BActorNodeDef *def, struct BActor *actor);
0003da90 void bLightAssignIndex(struct BLight *light);
0003db10 void bLightUpdateD3D(struct BLight *light);
0003dca0 void bLightEnableD3D(struct BLight *light);
0003dcc0 void bLightColourChanged(struct BLight *light);
0003dcf0 void bSetAmbientD3D(void);
