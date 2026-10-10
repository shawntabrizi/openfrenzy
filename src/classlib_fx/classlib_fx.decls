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
