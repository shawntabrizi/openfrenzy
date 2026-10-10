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
000be460 void CharLibShadow_DeleteAll(void);

# CarryOn/CCarryOn_Player.cpp
000c0900 void CCarryOn_Player_CreateControls(void);
000c0fd0 void CCarryOn_Player_DrawEffects(struct CCarryOnPlayer *self);
000c1760 void CCarryOn_Player_Update(struct CCarryOnPlayer *self);
000c1a60 void CCarryOn_Player_DrawAll(void);
000c1b30 void CCarryOn_Player_UpdateControls(struct CFireResource *res);
000c1ea0 struct CCarryOnPlayer *CCarryOn_Player_Ctor(struct CCarryOnPlayer *self, int player, float *pos);
000c2180 void CCarryOn_Player_Dtor(struct CCarryOnPlayer *self);

# the text reader (vtable 0x00267bb8) and CarryOn/CFire_ASEImport.cpp
000c3a20 struct CoTextReader *CFire_TextReader_Ctor(struct CoTextReader *self);
000c3a30 void CFire_TextReader_Dtor(struct CoTextReader *self);
000c3ac0 struct CFireASEImport *CFire_ASEImport_Ctor(struct CFireASEImport *self);
000c3ad0 void CFire_ASEImport_Dtor(struct CFireASEImport *self);
000c3ae0 void CFire_ASEImport_ReadPaths(struct CFireASEImport *self, struct CoTextReader *reader, void **paths, int *count, const char *name);
000c3eb0 void CFire_ASEImport_FreePaths(struct CFireASEImport *self, void *paths, int count);

# CarryOn/CFire_Carry.cpp
000c4d60 void *CFire_Carry_Ctor(void *self, void *levelPackage, void *genericPackage, int variant);
000c52b0 void CFire_Carry_Draw(void *self, unsigned int passes);
000c5340 void *CFire_Carry_GetHeld(void *self, int player);
000c5390 void CFire_Carry_Dtor(void *self);
000c5490 void CFire_Carry_Update(void *self, float step);

# CarryOn/CFire_World.cpp
000c6640 void CFire_World_Unload(void *self);
000c67a0 void CFire_World_Update(void *self, float step);
000c6890 void CFire_World_DrawSky(void *self);
000c69c0 void CFire_World_DrawFar(void *self);
000c6a20 void CFire_World_DrawNear(void *self);
000c6a80 void CFire_World_DrawObjects(void *self);
000c6af0 void CFire_World_DrawAlpha(void *self);
000c6d00 int CFire_World_Load(void *self, void *package, int variant, const char *name);

# CarryOn/CFire_Resource.cpp
000c6f30 struct CFireResource *CFire_Resource_Ctor(struct CFireResource *self, void *levelPackage, void *genericPackage, int variant, int numPlayers);
000c75a0 void CFire_Resource_Dtor(struct CFireResource *self);
000c7700 void CFire_Resource_UpdateDetail(struct CFireResource *self);
000c77c0 int CFire_Resource_FindSample(struct CFireResource *self, int kind, int variant, int volume);
000c7840 void CFire_Resource_SetSample(struct CFireResource *self, int sample);
000c7950 void CFire_Resource_DrawBackground(struct CFireResource *self);
000c7a20 void CFire_Resource_DrawLines(float *points, int count, int closed, unsigned int colour);
000c7f30 void CFire_Resource_Printf(struct CFireResource *self, float x, float y, unsigned int colour, int align, const char *format, ...);

# CarryOn/CFire_Camera.cpp
000c8340 int CFire_Camera_IsFading(struct CFireCamera *self);
000c8370 void CFire_Camera_DrawFade(struct CFireCamera *self, float step);
000c8550 void CFire_Camera_StartEnd(struct CFireCamera *self);
000c85f0 float CFire_Camera_GetYaw(struct CFireCamera *self);
000c8610 void CFire_Camera_GetEye(struct CFireCamera *self, float *out);
000c8640 void CFire_Camera_GetTarget(struct CFireCamera *self, float *out);
000c8730 void CFire_Camera_Apply(struct CFireCamera *self);
000c8760 void CFire_Camera_Reset(struct CFireCamera *self);
000c89b0 struct CFireCamera *CFire_Camera_Ctor(struct CFireCamera *self, int variant);
000c8b10 void CFire_Camera_Dtor(struct CFireCamera *self);
000c91a0 void CFire_Camera_Update(struct CFireCamera *self, float step);

# CarryOn: the conveyor belts (variant 4; file not known). carryon_b names 0x000ca230 CFire_Conveyor.
000c9490 void *CFire_Belt_Ctor(void *self);
000c94b0 void CFire_Belt_Init(void *self, void *fireworks, void *path, float speed, int a, int b, int c);
000c9520 void CFire_Belt_Update(void *self, float step);
000c97a0 void CFire_Belt_Draw(void *self);
