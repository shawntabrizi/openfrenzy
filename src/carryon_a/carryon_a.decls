# src/carryon_a/carryon_a.decls - prototype overrides of the carryon_a module
# (docs/ARCHITECTURE.md, section 11). <addr> <prototype>;
# x86 `this` (ECX) is the first parameter.

# CarryOn/unplaced.c
000ba6d0 void FUN_000ba6d0(void);

# CarryOn/CFire_Game.cpp
000ba700 void CFire_Game_LoadRampTextures(struct CFireGame *self);
000ba760 void CFire_Game_UpdateRampTextures(struct CFireGame *self, float step);
000ba7f0 void CFire_Game_CreatePanels(struct CFireGame *self);
000ba910 void CFire_Game_UpdatePanels(struct CFireGame *self, float step);
000babe0 void CFire_Game_SetVariantIndex(struct CFireGame *self, int variant);
000bac00 int CFire_Game_AddAnimTexture(struct CFireGame *self, const char *name, float speed);
000bac90 void CFire_Game_UpdateAnimTextures(struct CFireGame *self);
000bad10 void CFire_Game_BeginLoad(struct CFireGame *self);
000bae70 void CFire_Game_InitOnce(struct CFireGame *self);
000bb480 void CFire_Game_ShuffleOrder(struct CFireGame *self);
000bba60 void CFire_Game_InitObjects(struct CFireGame *self);
000bc3d0 void CFire_Game_Start(struct CFireGame *self);
000bc890 void CFire_Game_ClosePackages(void);
000bc920 void CFire_Game_BackgroundLoad(struct CFireGame *self, int skip);
000bc9f0 void CFire_Game_DrawDebug(void);
000bccb0 void CFire_Game_Draw(struct CFireGame *self, int draw);
000bd590 void CFire_Game_UpdateNearPlayers(struct CFireGame *self);
000bd800 struct CFireGame *CFire_Game_DeletingDtor(struct CFireGame *self, unsigned int flags);
000bd820 int CFire_Game_SetVariant(struct CFireGame *self, int variant);
000bd8e0 void CFire_Game_Shutdown(struct CFireGame *self);
000bdd50 void CFire_Game_Update(struct CFireGame *self, int update);

# CharLib/CharLibShadow.cpp (gap functions; file inferred from their data)
000be3f0 void Shadows_SetFov(float fovX, float fovY);
000be430 void Shadows_SetDistance(float distance);
000be440 void Shadows_SetPitchLimits(float lowest, float highest);
000be460 void Shadows_DeleteAll(void);
000be4a0 void Shadows_RenderMaps(void);
000be550 int Shadows_ApplyStages(void);
000be5a0 void Shadows_RestoreStages(void);
000be5d0 void Shadows_SetInUse(int player, int on);
000be610 void Shadows_RenderMapsFor(void **held);

# CarryOn/CCarryOn_Player.cpp
000bf760 void CCarryOn_Player_PlanRoute(void *route, float *from, float *target, float unused, float *step);
000bf850 void CCarryOn_Player_SetThinkInterval(struct CCarryOnPlayer *self, float scale);
000bf920 int CCarryOn_Player_FindRicher(struct CCarryOnPlayer *self);
000bf9c0 int CCarryOn_Player_CollectPads(struct CCarryOnPlayer *self);
000bfc00 void CCarryOn_Player_ChooseWanted(struct CCarryOnPlayer *self);
000bfcd0 int CCarryOn_Player_FindWantedCarrier(struct CCarryOnPlayer *self);
000bfd70 void CCarryOn_Player_DecideParts(struct CCarryOnPlayer *self);
000bfe50 int CCarryOn_Player_CollectParts(struct CCarryOnPlayer *self);
000c00d0 int CCarryOn_Player_DeliverParts(struct CCarryOnPlayer *self);
000c01b0 int CCarryOn_Player_FindRicherGlitter(struct CCarryOnPlayer *self);
000c0250 int CCarryOn_Player_CollectGlitter(struct CCarryOnPlayer *self);
000c0510 int CCarryOn_Player_EmptyGlitter(struct CCarryOnPlayer *self);
000c0600 int CCarryOn_Player_WorkConveyor(struct CCarryOnPlayer *self);
000c0810 int CCarryOn_Player_CanHit(struct CCarryOnPlayer *self, int other);
000c0900 void CCarryOn_Player_CreateControls(void);
000c0af0 int CCarryOn_Player_GiveFireworks(struct CCarryOnPlayer *self, int how, int type, int count);
000c0c40 void CCarryOn_Player_AddScore(struct CCarryOnPlayer *self, int points);
000c0c60 void CCarryOn_Player_ShowScore(struct CCarryOnPlayer *self, int points, int player);
000c0d30 void CCarryOn_Player_LoseFireworks(struct CCarryOnPlayer *self, int how, int count);
000c0fd0 void CCarryOn_Player_DrawEffects(struct CCarryOnPlayer *self);
000c10d0 int CCarryOn_Player_PickRandomOpponent(struct CCarryOnPlayer *self, int awake);
000c11e0 int CCarryOn_Player_PickWorstEnemy(struct CCarryOnPlayer *self, int awake);
000c12a0 void CCarryOn_Player_Hit(struct CCarryOnPlayer *self, void *from, float damage, int mode);
000c1440 void CCarryOn_Player_UpdateBurden(struct CCarryOnPlayer *self);
000c1650 void CCarryOn_Player_UpdatePost(struct CCarryOnPlayer *self);
000c1760 void CCarryOn_Player_Update(struct CCarryOnPlayer *self);
000c1a60 void CCarryOn_Player_DrawAll(void);
000c1b30 void CCarryOn_Player_UpdateControls(struct CFireResource *res);
000c1c10 void CCarryOn_Player_SetAltModel(struct CCarryOnPlayer *self, int alt);
000c1d50 int CCarryOn_Player_IsLeading(struct CCarryOnPlayer *self, int *count);
000c1ea0 struct CCarryOnPlayer *CCarryOn_Player_Ctor(struct CCarryOnPlayer *self, int player, float *pos);
000c2180 void CCarryOn_Player_Dtor(struct CCarryOnPlayer *self);
000c2200 void CCarryOn_Player_DecideLaunch(struct CCarryOnPlayer *self);
000c2390 int CCarryOn_Player_GoLaunch(struct CCarryOnPlayer *self);
000c24f0 void CCarryOn_Player_DecideGlitter(struct CCarryOnPlayer *self);
000c2660 void CCarryOn_Player_Punch(struct CCarryOnPlayer *self, struct CCarryOnPlayer *other);
000c2880 int CCarryOn_Player_TryPunch(struct CCarryOnPlayer *self);
000c2a60 void CCarryOn_Player_RespawnFarthest(struct CCarryOnPlayer *self);
000c2a80 struct CCarryOnPlayer *CCarryOn_Player_GiveToAttacker(struct CCarryOnPlayer *self, int how);
000c2e00 int CCarryOn_Player_PickTarget(struct CCarryOnPlayer *self, int awake);
000c2fb0 void CCarryOn_Player_SetMode(struct CCarryOnPlayer *self);
000c3150 int CCarryOn_Player_RunMode(struct CCarryOnPlayer *self);

# the text reader (vtable 0x00267bb8) and CarryOn/CFire_ASEImport.cpp
000c3a20 struct CoTextReader *CFire_TextReader_Ctor(struct CoTextReader *self);
000c3a30 void CFire_TextReader_Dtor(struct CoTextReader *self);
000c3a40 void CFire_TextReader_Rewind(struct CoTextReader *self);
000c3a50 void CFire_TextReader_SkipLine(struct CoTextReader *self);
000c3aa0 struct CoTextReader *CFire_TextReader_DeletingDtor(struct CoTextReader *self, unsigned int flags);
000c3ac0 struct CFireASEImport *CFire_ASEImport_Ctor(struct CFireASEImport *self);
000c3ad0 void CFire_ASEImport_Dtor(struct CFireASEImport *self);
000c3ae0 void CFire_ASEImport_ReadPaths(struct CFireASEImport *self, struct CoTextReader *reader, void **paths, int *count, const char *name);
000c3eb0 void CFire_ASEImport_FreePaths(struct CFireASEImport *self, void *paths, int count);
000c3f00 struct CFireASEImport *CFire_ASEImport_DeletingDtor(struct CFireASEImport *self, unsigned int flags);

# CarryOn/CFire_Carry.cpp
000c3f20 struct CoCarried *CFire_Carried_Ctor(struct CoCarried *self, int player, int type, void *model);
000c4110 void CFire_Carried_UpdateHands(struct CoCarried *self);
000c41b0 void *CFire_Carried_Place(struct CoCarried *self);
000c43f0 void CFire_Carried_Update(struct CoCarried *self, float step);
000c4830 void CFire_Carried_Draw(struct CoCarried *self, unsigned int passes);
000c4d60 struct CFireCarry *CFire_Carry_Ctor(struct CFireCarry *self, void *levelPackage, void *genericPackage, int variant);
000c52b0 void CFire_Carry_Draw(struct CFireCarry *self, unsigned int passes);
000c5310 void CFire_Carry_UpdateHeld(struct CFireCarry *self, int player);
000c5340 void *CFire_Carry_GetHeld(struct CFireCarry *self, int player);
000c5390 void CFire_Carry_Dtor(struct CFireCarry *self);
000c5490 void CFire_Carry_Update(struct CFireCarry *self, float step);

# CarryOn/CFire_World.cpp
000c6640 void CFire_World_Unload(void *self);
000c67a0 void CFire_World_Update(void *self, float step);
000c6890 void CFire_World_DrawSky(void *self);
000c69c0 void CFire_World_DrawFar(void *self);
000c6a20 void CFire_World_DrawNear(void *self);
000c6a80 void CFire_World_DrawObjects(void *self);
000c6af0 void CFire_World_DrawAlpha(void *self);
000c6c50 int CFire_World_LoadLOM(void *self, void *package, const char *name);
000c6d00 int CFire_World_Load(void *self, void *package, int variant, const char *name);

# CarryOn/CFire_Resource.cpp
000c6f30 struct CFireResource *CFire_Resource_Ctor(struct CFireResource *self, void *levelPackage, void *genericPackage, int variant, int numPlayers);
000c75a0 void CFire_Resource_Dtor(struct CFireResource *self);
000c7700 void CFire_Resource_UpdateDetail(struct CFireResource *self);
000c77c0 int CFire_Resource_FindSample(struct CFireResource *self, int kind, int index, int volume);
000c7840 void CFire_Resource_ReserveChannel(struct CFireResource *self, int channel);
000c7880 void CFire_Resource_Rumble(struct CFireResource *self, int player, int event);
000c7950 void CFire_Resource_ApplyFog(struct CFireResource *self);
000c7a20 void CFire_Resource_DrawLines(float *points, int count, int flags, unsigned int colour);
000c7be0 void CFire_Resource_DrawCircleFlags(float *pos, float radius, float height, unsigned int flags, unsigned int colour);
000c7da0 void CFire_Resource_DrawBones(void *instance, void *node, int a, int r, int g, int b);
000c7ed0 void CFire_Resource_DrawSkeleton(void *instance, unsigned int colour);
000c7f30 void CFire_Resource_Printf(struct CFireResource *self, float x, float y, unsigned int colour, int flags, const char *format, ...);
000c80b0 int CFire_Resource_MoveTowards(float *pos, float *target, float step);
000c8170 void CFire_Resource_BeginStage(int factor);
000c8200 void CFire_Resource_EndStage(void);
000c8230 void CFire_Resource_DrawCircle(float *pos, float radius, float height, unsigned int colour);

# CarryOn/CFire_Camera.cpp
000c8250 void CFire_Camera_StartGame(struct CFireCamera *self);
000c8340 int CFire_Camera_IsReady(struct CFireCamera *self);
000c8370 void CFire_Camera_DrawFade(struct CFireCamera *self, float step);
000c8420 struct CCarryOnPlayer *CFire_Camera_PickWinner(struct CFireCamera *self);
000c8550 void CFire_Camera_StartEnd(struct CFireCamera *self);
000c85f0 float CFire_Camera_GetYaw(struct CFireCamera *self);
000c8610 void CFire_Camera_GetEye(struct CFireCamera *self, float *out);
000c8640 void CFire_Camera_GetTarget(struct CFireCamera *self, float *out);
000c8670 void CFire_Camera_Lerp(struct CFireCamera *self, float step);
000c8730 void CFire_Camera_Apply(struct CFireCamera *self);
000c8760 void CFire_Camera_Reset(struct CFireCamera *self);
000c87b0 void CFire_Camera_UpdateHeadings(struct CFireCamera *self);
000c8840 void CFire_Camera_UpdateBounds(struct CFireCamera *self);
000c89b0 struct CFireCamera *CFire_Camera_Ctor(struct CFireCamera *self, int variant);
000c8b10 void CFire_Camera_Dtor(struct CFireCamera *self);
000c8b50 void CFire_Camera_UpdateIntro(struct CFireCamera *self, float step);
000c8cb0 void CFire_Camera_UpdateEnd(struct CFireCamera *self, float step);
000c8ed0 void CFire_Camera_UpdateFireworks(struct CFireCamera *self, float step);
000c9040 void CFire_Camera_UpdatePlayers(struct CFireCamera *self, float step);
000c91a0 void CFire_Camera_Update(struct CFireCamera *self, float step);

# CarryOn: the conveyor belts (variant 4; file not known). carryon_b names 0x000ca230 CFire_Conveyor.
000c9490 void *CFire_Belt_Ctor(void *self);
000c94b0 void CFire_Belt_Init(void *self, void *fireworks, void *path, float speed, int points, int count, int window);
000c9520 void CFire_Belt_Update(void *self, float step);
000c96a0 int CFire_Belt_HasItemAt(void *self, float at, float width, int type);
000c97a0 void CFire_Belt_Draw(void *self);
000c9950 float CFire_Belt_GetT(void *self, float x);
000c9970 int CFire_Belt_Launch(struct CFireBelt *self, int slot, float *pos, int player);
000c9bd0 int CFire_Belt_Pass(struct CFireBelt *self, int slot, float *pos, int player, void *sparks);
000c9cd0 int CFire_Belt_Hit(struct CFireBelt *self, float *pos, int player, int pass, void *sparks);
000c9e60 void CFire_ConveyorItem_Start(struct CoConveyorItem *self, int player, int paint);
000c9ee0 void CFire_ConveyorItem_Update(struct CoConveyorItem *self, float step, void *smoke, void *sparks, struct CFireBelt **belts, int numBelts);
000ca060 void CFire_ConveyorItem_Draw(struct CoConveyorItem *self);
