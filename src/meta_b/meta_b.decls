# src/meta_b/meta_b.decls - prototype overrides of the meta_b module (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# Meta/CLightning.cpp
0015cef0 void CLightning_Strike(float *from, float *to);
0015d010 void CLightningMgr_Reset(struct CLightningMgr *self);
0015d090 void CLightningMgr_AddBolt(struct CLightningMgr *self, float *pos, int arg, short tag, unsigned short groupId);
0015d160 void CLightningMgr_StrikeGroup(struct CLightningMgr *self, short tag, unsigned short groupId);
0015d260 struct CLightningBolt *CLightningBolt_Update(struct CLightningBolt *self);
0015d300 void CLightningMgr_UpdateTimer(struct CLightningMgr *self);
0015d360 void CLightningMgr_Draw(struct CLightningMgr *self);
0015d440 void CLightningMgr_Update(struct CLightningMgr *self);

# Meta/unplaced.c: vents
0015d4c0 struct CMetaVent *CMetaVent_Init(struct CMetaVent *self, void *type, int colour, short tag, short active, float rate);
0015d590 int CMetaVent_AddCycle(struct CMetaVent *self, short offTime, short onTime);
0015d5d0 void CMetaVent_AddType(struct CMetaVent *self, void *type, int colour, float rate);
0015d610 void MetaVent_DrawCylinder(float *pos, float *dir, float radius, float height, short r, short g, short b, short a);
0015d8f0 void CMetaVent_SetVelocity(struct CMetaVent *self, float *vel, float spreadXZ, float spreadY);
0015d930 void CMetaVent_SetPosition(struct CMetaVent *self, float *pos, float spreadXZ, float spreadY);
0015d970 void CMetaVent_Emit(struct CMetaVent *self, int index);
0015dc40 void CMetaVent_SetActive(struct CMetaVent *self, int mode);
0015dcc0 int CMetaVent_Push(struct CMetaVent *self, int player, float *pos, int test);
0015e020 void CMetaVent_SetPush(struct CMetaVent *self, float *dir, float radius, float length, float strength, uint flags);
0015e080 void MetaVents_DeleteAll(void);
0015e0c0 int MetaVents_SetActive(int tag, int mode);
0015e110 void CMetaVent_DrawDebug(struct CMetaVent *self);
0015e180 void CMetaVent_Update(struct CMetaVent *self);
0015e370 void MetaVents_Update(void);
0015e3a0 void MetaVents_DrawDebug(void);

# Meta/CMeta_ExplSmoke.cpp
0015e3d0 void CMetaExplSmoke_Update(struct CMetaExplSmoke *self);
0015e4e0 void CMetaExplSmoke_Draw(struct CMetaExplSmoke *self);
0015e6e0 void MetaExplSmoke_UpdateAll(void);
0015e710 void MetaExplSmoke_DrawAll(void);
0015e740 struct CMetaExplSmoke *CMetaExplSmoke_Create(float *pos, int numStill, float radiusStill, int numMoving, float radiusMoving, uint colour);
0015eeb0 void MetaExplSmoke_DeleteAll(void);

# Meta/CMeta_Explosion.cpp
0015ef20 struct CMetaExplosion *CMetaExplosion_Construct(struct CMetaExplosion *self);
0015efc0 void CMetaExplosion_Destruct(struct CMetaExplosion *self);
0015f030 int CMetaExplosion_Setup(struct CMetaExplosion *self, int numSparks, int numFlames, int numDebris);
0015f1d0 void MetaExplosions_UpdateAll(void);
0015f260 void CMetaExplosion_Start(struct CMetaExplosion *self, float *pos);
0015f310 void CMetaExplosion_DrawShockwave(struct CMetaExplosion *self);
0015f530 struct CMetaExplosion *CMetaExplosion_Create(void);
0015f5b0 void *CMetaExplosion_Draw(struct CMetaExplosion *self);
0015f620 void MetaExplosions_DrawAll(void);
0015f650 void MetaExplosions_DeleteAll(void);

# Meta/unplaced.c: shared helpers
00162f20 void *Meta_ReturnThis(void *self);

# Meta/unplaced.c: the pause-aware timer
0016bab0 int MbTimer_Reset(struct MbTimer *self);
0016bae0 void MbTimer_Update(struct MbTimer *self);
0016bc10 void MbTimer_SetDuration(struct MbTimer *self, float seconds);
0016bc30 void MbTimer_Start(struct MbTimer *self);
0016bc50 float MbTimer_GetRemaining(struct MbTimer *self);
0016bc60 float MbTimer_GetProgress(struct MbTimer *self);

# Meta/CMLaser.cpp
0015f6e0 void CMLaser_Draw(struct CMLaser *self);
0015f750 void CMLaser_Init(void);
0015f790 void CMLaser_DrawAll(void);
0015f820 void CMLaser_Trace(struct CMLaser *self);
0015fad0 struct CMLaser *CMLaser_Construct(struct CMLaser *self, float *origin, float *angles, float *spin, float maxLength, float width, int *colour, int *endColour, float life);
0015fbb0 struct CMLaser *CMLaser_Update(struct CMLaser *self);
0015fcd0 void CMLaser_DeleteAll(void);
0015fd10 void CMLaser_UpdateAll(void);
0015fd40 void CMLaser_Fire(float *origin, int count, int r, int g, int b, int a, float life);
0015ff00 void CMLaser_Show(float *origin, int count, float life);

# Meta/CScorchMarks.cpp
0015ff80 void CScorchMarks_Init(void);
0015ffc0 void CScorchMark_Draw(struct CScorchMark *self);
00160060 void CScorchMarks_DrawAll(void);
00160120 void CScorchMark_BuildQuad(struct MbVertex *verts, float *pos, float *normal, float size, float yaw);
001603b0 void CScorchMarks_DeleteAll(void);
00160400 struct CScorchMark *CScorchMark_Construct(struct CScorchMark *self, float *pos, float *normal, void *parent, int type);
00160560 void CScorchMarks_UpdateAll(void);
001605e0 struct CScorchMark *CScorchMark_Create(float *pos, float *normal, void *parent, int type);

# Meta/CBomb.cpp
00160680 void CBomb_SetupPhysics(void *actor);
00160720 void CBomb_Init(void);
00160760 void CBomb_LightFuses(int tag);
001607e0 struct CBomb *CBomb_Find(int tag);
00160810 struct CAirRaid *CAirRaid_Construct(struct CAirRaid *self, float life, float interval, float *from, float *to, float margin, float *vel);
001608b0 struct MbBombGroup *MbBombGroup_Construct(struct MbBombGroup *self);
001608d0 void Meta_Nothing(void);
001608e0 struct CBomb *CBomb_Construct(struct CBomb *self, int kind, float *pos, int tag);
00160bd0 void CBomb_DebugAll(void);
00160bf0 void MbBombGroup_AddPosition(float *pos, short tag, uint groupId);
00160cb0 struct CBomb *MbBombPos_Drop(struct MbBombPos *self, int kind);
00160d50 int MbBombGroup_Start(int kind, uint groupId, int count, float interval);
00160db0 struct CBomb *MbBombGroup_DropOne(struct MbBombGroup *self);
00160e70 void *CBomb_Delete(struct CBomb *self);
00160ee0 void CBomb_DeleteAll(void);
00160f10 void *CBomb_Explode(struct CBomb *self);
00161250 struct CAirRaid *CAirRaid_Update(struct CAirRaid *self);
00161460 void MbBombGroups_DeleteAll(void);
001614c0 void MbBombGroup_Update(struct MbBombGroup *self);
00161510 void CAirRaid_DeleteAll(void);
00161560 void *CBomb_Update(struct CBomb *self);
00161b80 void CBomb_UpdateAll(void);
