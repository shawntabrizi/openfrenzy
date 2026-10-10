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
