# classlib_fx prototypes (see docs/ARCHITECTURE.md section 11): true prototypes of the
# functions this module defines. FUN_<addr> becomes a macro for the new name.

# ---- CProjector.cpp (and the draw functions in the gap in front of it)
00082fd0 void CProjTrail_Draw(struct CProjTrail *self);
00083090 void CProjTrail_DrawAll(void);
00083130 void CProjTrail_DrawAll2(void);
000831d0 void CProjector_LoadSpiralTextures(void);
00083220 void CProjector_Reset(void);
000832a0 void CProjector_DrawAll(void);
00083370 void CProjector_DrawRefBotAll(void);
00083440 struct CProjTrail *CProjTrail_Init(struct CProjTrail *self, int texture, int count, int *headColour, int *tailColour, float headWidth, float tailWidth, int second);
00083580 void CProjTrail_AddPoint(struct CProjTrail *self, float *p);
000836b0 void CProjector_SpiralBeam(float *from, float *to, float radius, float rise, int turns, float sparkleDist, struct CProjSpiralType *spiral, float sparkleRate, uint flags, uint colour);
00083b60 void *CProjector_Delete(struct CProjector *self);
00083b90 struct CProjector *CProjector_Init(struct CProjector *self, int actor, int node, struct CProjRayType *type, float *source, float life, int speed, int frameOffset, int refbot, float width, int numPoints);
00083d60 void *CProjector_Update(struct CProjector *self);
00083f40 void *CProjTrail_Delete(struct CProjTrail *self);
00083fa0 void *CProjTrail_Update(struct CProjTrail *self);
00084230 void CProjTrail_UpdateAll(void);
00084280 void CProjTrail_DeleteAll(void);
00084360 void CProjector_SpiralAt(float *pos, int player);
000844a0 void CProjector_SpiralAtEx(float *pos, float radius, float height, int turns, float sparkleDist, int player);
000845d0 void CProjector_UpdateAll(void);
00084620 void CProjector_DeleteAll(void);
000846c0 void CProjector_CreateForActor(int actor, struct CProjRayType *type, float *source);

# ---- RefBot.cpp (and the RefBot gap functions)
000848c0 void RefBot_PlayEye(void);
00084900 void RefBot_PlayBeamSound(void);
00084930 void RefBot_StopBeamSound(void);
00084990 void RefBot_Draw(void);
00084a30 void RefBot_Create(void);
00084d20 void RefBot_UpdateBeams(void);
00084d80 void RefBot_Enter(void);
00084e20 void RefBot_Leave(void);
00084e90 void RefBot_Update(void);
00084ee0 void RefBot_Delete(void);

# ---- unplaced: animated textures
00082be0 void AnimTex_Advance(struct CAnimTex *self, float scale, int apply);
00082c20 void AnimTex_AdvanceList(struct CLFList *list, float scale, int apply);
00082c80 void AnimTex_ApplyList(struct CLFList *list);
00082cc0 void AnimTex_SetFrameList(struct CLFList *list, int frame);
00082d10 void Texture_CopyGreyscale(int dst, int src);
00082f10 void AnimTex_DeleteList(struct CLFList *list);
00082f60 struct CAnimTex *AnimTex_Init(struct CAnimTex *self, char *name, float speed, struct CLFList *list);

# ---- unplaced: point sprites
00084f30 struct CPointSprites *PointSprites_Init(struct CPointSprites *self, int blendMode, int texture, uint numVerts, int param94);
00085000 void Blend_SetMode(int mode);
00085060 void PointSprites_Begin(struct CPointSprites *self);

# ---- SpriteVB.cpp
000861b0 void SpriteVB_Draw(struct CSpriteVB *self);
000862f0 void SpriteVB_ForceBufferSize(struct CSpriteVB *self, int size);
00086350 void SpriteVB_Release(struct CSpriteVB *self);
000863c0 void SpriteVB_Lock(struct CSpriteVB *self);
00086500 struct CSpriteVBBatch *SpriteVB_GetBatch(struct CSpriteVB *self, int key);

# ---- Sprite.cpp
00086580 void Sprite_LoadDefs(struct CSpriteDef *defs, int count, int package);
00086680 void Sprite_FreeDefs(struct CSpriteDef *defs, int count);
000866b0 void Sprite_Animate(struct CSprite *self);
00086750 void Sprite_Update(struct CSprite *self, float dt);
00086770 void Sprite_SetAnim(struct CSprite *self, struct CSpriteAnim *anim);
000867e0 struct CSprite *Sprite_Create(struct CSprite *self, float *pos, struct CSpriteAnim *anim, int user);

# ---- DebrisSys.cpp
00086880 int DebrisSys_InitWind(int self);

# ---- CRay.cpp (and the orb gap functions)
000a7550 void CRayControl_ResetList(void);
000a7560 struct CRayControl *CRayControl_Init(struct CRayControl *self, int texture, float *anchor, int *colour0, int *colour1, float *target, void *test);
000a7650 void CRayControl_Draw(struct CRayControl *self);
000a76c0 struct CRay *CRayControl_AddRay(struct CRayControl *self, float angle, float speed, float length, float width0, float width1);
000a7730 void CRay_Update(struct CRay *self);
000a7800 struct CRayControl *CRay_CreateOrbRayControl(int type, float *anchor, float *target, void *test);
000a7900 void CRayControl_DrawAll(void);
000a7980 void CRayControl_Update(struct CRayControl *self);
000a7af0 void CRayControl_DeleteRays(struct CRayControl *self);
000a7b40 void CRayControl_SetScore(struct CRayControl *self, int score);
000a7c70 void CRayControl_Destroy(struct CRayControl *self);
000a7cd0 void CRayControl_UpdateAll(void);
000a7d00 void CRayControl_DeleteAll(void);

# ---- unplaced: texture frames, animation switch
000a7d40 void Texture_SetFrameAtTime(int tex, int time, int first);
000a7dc0 void AnimSwitch_Set(int *self, int on);
000a7e30 void AnimSwitch_SetValue(int self, int value);

# ---- CParticleSystem.cpp (and 0x000a7e50 in the gap in front of it)
000a7e50 void CParticleEffect_Burst(struct CParticleEffect *self, struct CParticleEmitter *emitter, short r, short g, short b, int kind, float strength);
000a8520 struct CParticleSystem *CParticleSystem_DeletingDtor(struct CParticleSystem *self, byte flags);
000a8580 void CParticleEffect_SetOn(struct CParticleEffect *self, int id, int on);
000a85c0 void CParticleEmitter_SetPosition(struct CParticleEmitter *self, float *pos);
000a85f0 void CParticleEffect_Update(struct CParticleEffect *self, float dt);
000a86e0 void CParticleEffect_Draw(struct CParticleEffect *self, int mode, uint drawActors);
000a8a00 int CParticleEffect_Collide(struct CParticleEffect *self, struct CParticleEmitter *e, int a2, float *dir);
000a8ab0 void CParticleSystem_SetTexture(struct CPSHeader *self, int texture);
000a8ac0 void CParticleSystemList_Construct(struct CParticleSystemList *self);
000a8ad0 void CParticleSystemList_Destruct(struct CParticleSystemList *self);
000a8ae0 void CParticleSystemList_DrawAll(struct CParticleSystemList *self);
000a8b00 void CParticleSystem_Destruct(struct CParticleSystem *self);
000a8b10 void CParticleSystem_DefaultParams(struct CPSParams *p);
000a8b80 void CParticleSystem_Reset(struct CParticleSystem *self);
000a8b90 void CSparkSystem_Construct(struct CSparkSystem *self);
000a8ba0 void CSparkSystem_Destruct(struct CSparkSystem *self);
000a8bb0 void CSparkSystem_DefaultParams(struct CSparkParams *p);
000a8c10 void CSparkSystem_Draw(struct CSparkSystem *self);
000a8de0 void CSparkSystem_Reset(struct CSparkSystem *self);
000a8df0 void CSmokeSystem_Construct(struct CSmokeSystem *self);
000a8e00 void CSmokeSystem_Destruct(struct CSmokeSystem *self);
000a8e10 void CSmokeSystem_Draw(struct CSmokeSystem *self);
000a8f90 void CSmokeSystem_Reset(struct CSmokeSystem *self);
000a8fa0 void CGlowSystem_Construct(struct CGlowSystem *self);
000a8fb0 void CGlowSystem_Destruct(struct CGlowSystem *self);
000a8fc0 void CGlowSystem_Draw(struct CGlowSystem *self);
000a9140 void CSparkPointSystem_Construct(struct CSparkPointSystem *self);
000a9150 void CSparkPointSystem_Destruct(struct CSparkPointSystem *self);
000a9160 void CSparkPointSystem_DefaultParams(struct CSparkPointParams *p);
000a91e0 void CSparkPointSystem_Draw(struct CSparkPointSystem *self);
000a9510 void CSparkPointSystem_Reset(struct CSparkPointSystem *self);
000a9540 void CParticleSystem_SetTexture2(struct CParticleSystem *self, int texture);
000a9550 void CParticleSystem_SetPosition(struct CPSHeader *self, float *pos);
000a9570 struct CParticleSystemList *CParticleSystemList_DeletingDtor(struct CParticleSystemList *self, byte flags);
000a9590 void CParticleSystemList_DeleteAll(struct CParticleSystemList *self);
000a95c0 void CParticleSystem_Construct(struct CParticleSystem *self);
000a95d0 int CParticleSystem_Init(struct CParticleSystem *self, struct CPSParams *params, int maxParticles);
000a9800 void CParticleSystem_Emit(struct CParticleSystem *self, struct CParticle *p);
000a9a30 void CParticleSystem_Release(struct CParticleSystem *self);
000a9af0 void CParticleSystem_Update(struct CParticleSystem *self, float dt);
000a9d20 void CParticleSystem_Draw(struct CParticleSystem *self);
000aa020 void CParticleSystem_Start(struct CParticleSystem *self, short r, short g, short b);
000aa250 struct CSparkSystem *CSparkSystem_DeletingDtor(struct CSparkSystem *self, byte flags);
000aa270 int CSparkSystem_Init(struct CSparkSystem *self, struct CSparkParams *params, int maxParticles);
000aa310 void CSparkSystem_Emit(struct CSparkSystem *self, struct CParticle *p);
000aa590 void CSparkSystem_Release(struct CSparkSystem *self);
000aa5d0 void CSparkSystem_Update(struct CSparkSystem *self, float dt);
000aa730 struct CSmokeSystem *CSmokeSystem_DeletingDtor(struct CSmokeSystem *self, byte flags);
000aa750 int CSmokeSystem_Init(struct CSmokeSystem *self, struct CSmokeParams *params, int maxParticles);
000aa810 void CSmokeSystem_Emit(struct CSmokeSystem *self, struct CParticle *p);
000aaad0 void CSmokeSystem_Release(struct CSmokeSystem *self);
000aab10 struct CGlowSystem *CGlowSystem_DeletingDtor(struct CGlowSystem *self, byte flags);
000aab30 int CGlowSystem_Init(struct CGlowSystem *self, struct CGlowParams *params, int maxParticles);
000aac00 void CGlowSystem_Emit(struct CGlowSystem *self, struct CParticle *p);
000aaf00 void CGlowSystem_Release(struct CGlowSystem *self);
000aaf40 void CSmokeSystem_Update(struct CSmokeSystem *self, float dt);
000ab050 struct CSparkPointSystem *CSparkPointSystem_DeletingDtor(struct CSparkPointSystem *self, byte flags);
000ab070 int CSparkPointSystem_Init(struct CSparkPointSystem *self, struct CSparkPointParams *params, int maxParticles);
000ab120 void CSparkPointSystem_Emit(struct CSparkPointSystem *self, struct CParticle *p);
000ab340 void CSparkPointSystem_Release(struct CSparkPointSystem *self);
000ab390 void CSparkPointSystem_Update(struct CSparkPointSystem *self, float dt);

# ---- CollectableEffects.cpp
000bf080 void CCollectEffect_UpdateRays(struct CCollectEffect *self);
000bf1f0 void CCollectEffect_Init(struct CCollectEffect *self, float *pos, int numRays);
000bf5b0 float CCollectEffect_Update(struct CCollectEffect *self);

# ---- CToken.cpp (and the token gap functions)
0010c230 void CToken_ShadowQuad(struct CTokenVertex *v, float *pos, float *up, float size, float angle);
0010c420 void CToken_DrawGlowAll(void);
0010c520 struct CToken *CToken_FindNearest(float *pos, float maxDist, uint stateMask, int flat);
0010c5c0 int CSpring_Update(struct CSpring *self);
0010c690 struct CToken *CToken_Init(struct CToken *self, float *pos, int type, float size, float *up);
0010cb40 void CToken_DrawShadow(struct CToken *self);
0010cbb0 void CToken_Collect(struct CToken *self);
0010cc00 void *CToken_Delete(struct CToken *self);
0010cc30 void CToken_DrawShadowAll(void);
0010cd00 void CToken_DrawAll(void);
0010cd10 void CToken_DeleteAll(void);
0010cd60 void *CToken_Update(struct CToken *self);

# ---- CWaterField.cpp (and its two gap helpers)
001387d0 int CWaterField_BuildIndices(struct CWaterField *self);
001389f0 void CWaterField_ConstructArray(void *mem, int size, int count, void *ctor);
00138a20 void CWaterField_Ripple(struct CWaterField *self);
00138b80 void CWaterField_Scroll(struct CWaterField *self, float dist);
00138c40 void CWaterField_Rebuild(struct CWaterField *self);
00138c50 void CWaterField_ScrollUV(struct CWaterField *self, float du, float dv);
00138d40 void CWaterField_DrawFan(struct CWaterField *self, float *xs, float *zs, int count, struct CWaterVertex *v);
00138de0 int CWaterField_Splash(struct CWaterField *self, float x, float z, float depth, float inner, float outer);
00138ff0 void CWaterField_Destruct(struct CWaterField *self);
00139080 void CWaterField_BuildNormals(struct CWaterField *self);
00139170 void CWaterField_SetMaterial(uint diffuse, uint specular, float power, int specularOn);
00139340 void CWaterField_ResetMaterial(void);
00139400 int CWaterField_UpdateVertices(struct CWaterField *self);
00139590 int CWaterField_BuildVertices(struct CWaterField *self);
00139790 int CWaterField_Draw(struct CWaterField *self, int blend);
00139850 int CWaterField_DrawEdges(struct CWaterField *self, float height, int blend);
00139ab0 int CWaterField_RandomDrop(struct CWaterField *self, float depth);
00139b40 struct CWaterField *CWaterField_DeletingDtor(struct CWaterField *self, byte flags);
00139b60 int CWaterField_Init(struct CWaterField *self, struct CWaterParams *params);

# ---- unplaced: AI conditions and states (0x00139e00-0x0013af80)
00139e00 int AICond_Walk(void *ai);
00139e70 int AICond_Jump(void *ai);
00139ea0 int AICond_Attack(void *ai);
0013a180 int AICond_Fall(void *ai);
0013a1d0 int AIState_Land(void *ai, int entered);
0013a260 int AIState_Taunt(void *ai, int entered);
0013a2e0 int AIState_Pickup(void *ai, int entered);
0013a360 int AIState_Drown(void *ai, int entered);
0013a420 int AIState_Press(void *ai, int entered);
0013a4c0 int AIState_Idle(void *ai, int entered);
0013a570 int AIState_Stand(void *ai, int entered);
0013a650 int AIState_Walk(void *ai, int entered);
0013a890 int AIState_Jump(void *ai, int entered);
0013a9f0 int AIState_Attack(void *ai, int entered);
0013acf0 int AIState_Throw(void *ai, int entered);
0013ada0 int AIState_JumpAttack(void *ai, int entered);
0013ae50 int AIState_Fall(void *ai, int entered);
0013ae80 int AIState_Hurt(void *ai, int entered);
0013af80 int AIState_Knockout(void *ai, int entered);

# ---- CSeagulls.cpp (and 0x0016cc80 after it)
0016bea0 void CSeagulls_AddObstacle(struct CSeagulls *self, int obstacle);
0016bf20 void CGull_Init(struct CGull *self, struct CFlockParams *p, int onEdge, int model);
0016c1f0 void CSeagulls_Release(struct CSeagulls *self);
0016c260 void CSeagulls_Neighbours(struct CSeagulls *self);
0016c5a0 void CSeagulls_Update(struct CSeagulls *self, float dt);
0016cb30 void CSeagulls_PickTarget(struct CSeagulls *self);
0016cbf0 struct CSeagulls *CSeagulls_Init(struct CSeagulls *self, int count, struct CFlockParams *params, int model);
0016cc80 void CSeagulls_Tick(struct CSeagulls *self, float dt);

# ---- ActorDebris.cpp (file inferred; its callbacks are not in the table)
00085200 void ActorDebris_Kill(void);
000852a0 void ActorDebris_DrawOpaque(struct CActorDebris *item);
00085340 void ActorDebris_DrawBlended(struct CActorDebris *item);
00086160 void ActorDebris_Create(void);
