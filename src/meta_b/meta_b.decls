# src/meta_b/meta_b.decls - prototype overrides of the meta_b module (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# Meta/CLightning.cpp
0015cef0 void CLightning_Strike(float *from, float *to);
0015d010 void CLightningMgr_Reset(struct CLightningMgr *self);
0015d090 void CLightningMgr_AddBolt(struct CLightningMgr *self, float *pos, float arg, short tag, unsigned short groupId);
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

# Meta/BigRedButton.cpp
00161c00 void BigRedButton_Init(void);
00161c20 int BigRedButton_FindTag(int tag);
00161c70 struct CBigRedButton *BigRedButton_FindPressable(void);
00161cc0 int CBigRedButton_Release(struct CBigRedButton *self);
00161d00 void BigRedButton_ClearPresses(int tag);
00161d20 void BigRedButton_Enable(int tag, int enabled);
00161d80 struct CBigRedButton *CBigRedButton_Construct(struct CBigRedButton *self, void *model, int mode, float radius, float tolerance, int enabled);
00161ed0 int CBigRedButton_Update(struct CBigRedButton *self);
00161fd0 int CBigRedButton_CanPress(struct CBigRedButton *self, void *character);
00162080 int CBigRedButton_Press(struct CBigRedButton *self);
00162150 int CBigRedButton_Toggle(struct CBigRedButton *self);
00162180 void BigRedButton_DeleteAll(void);
001621d0 struct CBigRedButton *BigRedButton_FindPressableBy(void *character);
00162230 void BigRedButton_UpdateAll(void);

# Meta/Searchlights.cpp
00162260 void Searchlights_DrawAll(void);
00162640 struct CSearchlight *CSearchlight_Construct(struct CSearchlight *self, const char *name, int unused, float *pos, float scale, int arg6, int gunId, int flags, const char *gunModel, const char *gunBone, float *gunPos);
001627a0 void CSearchlight_Update(struct CSearchlight *self);
00162b60 void Searchlights_UpdateAll(void);
00162b90 void *CSearchlight_Delete(struct CSearchlight *self);
00162c10 void Searchlights_DeleteAll(void);

# Meta/BigGun.cpp
00164b50 struct CBigGun *CBigGun_Construct(struct CBigGun *self);
00164bc0 void MbCutterParams_Init(struct MbCutterParams *p);
00164c40 void CBigGunCutter_Draw(struct CBigGunCutter *self);
00164e90 void CBigGunLaser_Draw(struct CBigGunLaser *self, void *unused1, void *unused2);
00165000 void CBigGunCutter_Init(struct CBigGunCutter *self, int count);
001650d0 void CBigGunCutter_StartSpark(struct CBigGunCutter *self, struct MbSpark *s);
00165300 void CBigGunCutter_Emit(struct CBigGunCutter *self, int count, float *pos);
00165380 void CBigGunCutter_Update(struct CBigGunCutter *self, float *pos, int emit);
001654f0 void CBigGun_DrawLaser(struct CBigGun *self, float *target);
00165510 void CBigGunLaser_Init(struct CBigGunLaser *self, void *actor);
00165630 void CBigGunLaser_Update(struct CBigGunLaser *self, void *actor, float *target);
001659e0 void CBigGun_Init(struct CBigGun *self, const char *model, const char *bone, int id);
00165a90 void CBigGun_Aim(struct CBigGun *self, float *target);
00165e20 void CBigGun_Free(struct CBigGun *self);
00165e90 void CBigGun_Destruct(struct CBigGun *self);

# Meta/CWater.cpp
00162c40 void MetaWater_Init(void);
00162c60 float MetaWater_Clamp(float value, float limit);
00162cb0 void CMetaWater_UpdateField(struct CMetaWater *self);
00162d30 void CMetaWater_Draw(struct CMetaWater *self);
00162e10 void CMetaWater_Splash(struct CMetaWater *self, float *pos, float strength);
00162e60 void CMetaWater_SetDrops(struct CMetaWater *self, float interval, float depth);
00162e90 void MetaWater_Splash(float *pos, float strength);
00162ee0 struct CMetaWater *MetaWater_Find(int tag);
00162f30 void CMetaWater_Destruct(struct CMetaWater *self);
00162f90 void MetaWater_DrawAll(void);
00162fc0 void CMetaWater_CreateField(struct CMetaWater *self, float *pos, int nx, int nz, float sizeX, float sizeZ);
001630c0 void MetaWater_SetLevel(int tag, float level, float speed);
00163120 int MetaWater_PlayerSplash(void *meta, float depth, float minSpeed);
00163260 int MbDrip_Update(struct MbDrip *self);
001632d0 void MbDrip_Init(struct MbDrip *self, float x, float z, float timeMin, float timeRange, float depth, float inner, float outer);
00163350 void CMetaWater_Update(struct CMetaWater *self);
001633f0 struct CMetaWater *CMetaWater_Construct(struct CMetaWater *self, float *pos, int nx, int nz, float sizeX, float sizeZ, short tag);
00163500 void CMetaWater_AddDrip(struct CMetaWater *self, float x, float z, float timeMin, float timeRange, float strength, float size);
00163560 void MetaWater_DeleteAll(void);
001635b0 void MetaWater_UpdateAll(void);

# Meta/unplaced.c: particle emitters
001635e0 void CMetaEmitter_SetRate(struct CMetaEmitter *self, float rate);
00163600 struct CMetaEmitter *CMetaEmitter_InitLine(struct CMetaEmitter *self, void *ptype, uint colour, float *from, float *to, float rate, int flags, int arg);
001636d0 struct CMetaEmitter *CMetaEmitter_InitWaterLine(struct CMetaEmitter *self, float strength, float *from, float *to, float rate, int flags, int arg);
001637a0 struct CMetaEmitter *CMetaEmitter_InitCircle(struct CMetaEmitter *self, void *ptype, uint colour, float *centre, float radius, float rate, int flags, int arg);
00163840 void CMetaEmitter_Emit(struct CMetaEmitter *self);
00163ae0 void CMetaEmitter_SetVelocity(struct CMetaEmitter *self, float *vel, float *spread);
00163b20 void MetaEmitters_DeleteAll(void);
00163b60 struct CMetaEmitter *CMetaEmitter_Update(struct CMetaEmitter *self);
00163c60 void MetaEmitters_UpdateAll(void);

# Meta/CMeta_Rain.cpp
00163cd0 void *MbSplash_Update(struct MbSplash *self);
00163d40 void MbSplash_UpdateAll(void);
00163d80 void MbSplash_DeleteAll(void);
00163f30 void MetaRain_DeleteLight(void);
00163f50 void MbSplash_BuildQuad(struct MbVertex *verts, float *pos, float *normal, float size, int unused, float alpha);
00164220 void MbSplash_Draw(struct MbSplash *self);
001642c0 void MbSplash_DrawAll(void);
001645f0 void CMetaRain_Init(struct CMetaRain *self, int count);
00164730 void CMetaRain_UpdateLightning(struct CMetaRain *self);

# Meta/CMeta_Seagulls.cpp
00164990 void MetaSeagulls_Update(void);
001649b0 void MetaSeagulls_Draw(void);
001649d0 void MetaSeagulls_Init(void);
00164b00 void MetaSeagulls_Delete(void);

# Meta/unplaced.c: CSeagulls helpers
0016bc70 int MbGull_Avoid(float *gull, float *box, void *params, float *steer, float *weight);
0016be60 void CSeagulls_Draw(void *flock);

# Meta/LevelLoad.cpp
00166290 int LevelLoad_FindDefine(const char *name, const char **value);
00166320 char *LevelLoad_Preprocess(char *text, int *size);
001666e0 uint LevelLoad_ParseFlags(char *list, const char **names);
001667e0 int LevelLoad_Lookup(const char *name, const char **names);
00166870 void LevelLoad_PackTrack(struct MbKeyTrack *track, float *keys);
00166a60 void *LevelLoad_NearestGoal(float *pos);
00166ad0 void *LevelLoad_BestGoalFor(int player);
00166ba0 void CMetaVent_SetHazard(struct CMetaVent *self, float radius, float strength, uint flags);
00166bd0 void CMetaVent_SetHazardDirection(struct CMetaVent *self, float x, float y, float z);
00166c00 void LevelLoad_LoadHeightMaps(int zone, int part);
00166d40 void LevelLoad_FinishTracks(void *platform, struct CSearchlight *searchlight);
00166db0 int LevelLoad_Load(int zone, int part);

# Meta/unplaced.c: platform dust
00165f10 void MetaPlatform_MakeDust(void *platform);
