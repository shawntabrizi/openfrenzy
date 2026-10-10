# src/rowing_b/rowing_b.decls - prototype overrides of the rowing_b module
# (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# Rowing/CRowing_Sprites.cpp (0x001b12c0, 0x001b1870, 0x001b1c60 tagged; the gap
# functions 0x001b1e00-0x001b2b10 inferred)
001b12c0 int RowSprites_Init(void);
001b1330 void RowSprites_Free(void);
001b1360 int RowSprites_Add(int type, float *pos, float width, float height, float angle, void *texture, int *colour, float zBias);
001b1430 void RowSprites_DrawOpaque(void);
001b1490 void RowSprites_DrawBlended(void);
001b1600 void RowQuadSprites_Free(struct RowQuadSprites *self);
001b1630 int RowQuadSprites_Init(struct RowQuadSprites *self, int blendMode, void *texture, int count);
001b17a0 void RowQuadSprites_Begin(struct RowQuadSprites *self);
001b1870 struct RowHelix *RowHelix_ctor(struct RowHelix *self, int r, int g, int b, int arg58);
001b1a10 void RowHelix_Free(struct RowHelix *self);
001b1ab0 void RowHelix_Flash(struct RowHelix *self);
001b1ac0 void RowHelix_Draw(struct RowHelix *self);
001b1c60 int RowParticles_Init(struct RowParticles *self, int unused, int steamCount, int blobCount);
001b1e00 void RowParticles_UpdateBlobs(struct RowParticles *self);
001b1e90 void RowParticles_UpdateSteam(struct RowParticles *self);
001b1f70 void RowParticles_Update(struct RowParticles *self);
001b1f80 struct RowSteam *RowParticles_AddSteam(struct RowParticles *self, float *pos, float size, float life);
001b2150 struct RowSteam *RowParticles_AddSmoke(struct RowParticles *self, float *pos, float size);
001b2300 struct RowBlob *RowParticles_AddBlob(struct RowParticles *self, float *pos, float size);
001b2600 void RowQuadSprites_Flush(struct RowQuadSprites *self);
001b27e0 void RowHelix_Update(struct RowHelix *self, float unused);
001b2990 void RowHelix_Push(struct RowHelix *self, float *pos, float *vel, int alpha);
001b2a50 void RowHelix_Add(struct RowHelix *self, float dt, float *pos, float *vel, int alpha);
001b2ab0 void RowParticles_Free(struct RowParticles *self);
001b2b10 void RowParticles_Draw(struct RowParticles *self);

# Rowing/CRowing_World.cpp (0x001b43c0 tagged; the gap functions 0x001b2c10-0x001b4720 inferred)
001b2c10 void *RowWorld_ctor(void *self);
001b2c30 void RowWorld_LoadPaths(void *self);
001b3170 void RowWorld_Update(void *self, float dt);
001b31d0 void RowWorld_SetFog(void);
001b3230 void RowWorld_DrawObjects(void *self, int draw);
001b3280 void RowWorld_DrawBlended(void *self);
001b3310 void RowPath_Draw(struct RowPath *self, float y, unsigned char r, unsigned char g, unsigned char b);
001b3530 void RowPath_GetSegment(struct RowPath *self, float *out);
001b4070 void RowWorld_DrawCollision(void *self);
001b40f0 void RowWorld_DrawRaceLine(void *self);
001b4310 int RowWorld_LoadLOM(void *world, void *package, const char *name);
001b43c0 int RowWorld_Load(void *self, void *package, const char *name);
001b44f0 void RowPaths_Free(struct RowPath *paths, int count);
001b4530 void RowWorld_Free(void *self);
001b4720 void RowWorld_Draw(void *self, int draw);

# Rowing/CRowing_Spark.cpp (0x001b4c60 tagged; 0x001b47b0-0x001b4d40 inferred)
001b47b0 struct RowSparks *RowSparks_ctor(struct RowSparks *self);
001b47c0 void RowSparks_Free(void *self);
001b4800 void RowSparks_Draw(struct RowSparks *self);
001b49d0 struct RowSparks *RowSparks_DeletingDtor(struct RowSparks *self, unsigned int flags);
001b49f0 void RowSpark_Start(struct RowSpark *spark, float *pos, float scale);
001b4c60 int RowSparks_Init(struct RowSparks *self, void *texture, int count);
001b4d40 void RowSparks_Update(struct RowSparks *self, float dt);

# Rowing/CRowing_Lights.cpp (0x001b52e0, 0x001b5370 tagged; 0x001b4f90-0x001b56a0 inferred)
001b4f90 struct RowLights *RowLights_ctor(struct RowLights *self);
001b4fb0 void RowLights_Free(struct RowLights *self);
001b5010 void RowLight_SetSprite(struct RowLight *light, void *sprite, float size);
001b5040 void RowLights_DrawSprites(struct RowLights *self);
001b50d0 void RowLight_SetD3DLight(struct RowLight *light, int rawRange);
001b52e0 struct RowLight *RowLights_AddPoint(struct RowLights *self, int group, float *pos, int r, int g, int b, float range, unsigned char flags);
001b5370 struct RowLight *RowLights_AddDirectional(struct RowLights *self, int group, float *dir, int r, int g, int b, unsigned char flags);
001b5400 void RowLights_Attach(struct RowLights *self, struct RowLight *light);
001b5510 void RowLights_Enable(struct RowLights *self, struct RowLight *light, int on);
001b5590 void RowLights_SetColour(struct RowLights *self, struct RowLight *light, int r, int g, int b);
001b55d0 void RowLights_SetPos(struct RowLights *self, struct RowLight *light, float *pos);
001b5630 void RowLights_SetRawRange(struct RowLights *self, int rawRange);
001b5660 void RowLights_EnableGroup(struct RowLights *self, int group, int on);
001b56a0 int RowLights_SetGroupFlags(struct RowLights *self, int group, unsigned int set, unsigned int toggle);

# Rowing/CRowing_ASEImport.cpp (0x001b9ea0, 0x001ba160 tagged; 0x001b9de0-0x001b9e80 inferred)
001b9de0 struct RowAseFile *RowAseFile_ctor(struct RowAseFile *self);
001b9df0 void RowAseFile_dtor(struct RowAseFile *self);
001b9e00 void RowAseText_ReadLine(int self, char *line);
001b9e40 struct RowAseFile *RowAseFile_DeletingDtor(struct RowAseFile *self, unsigned int flags);
001b9e60 struct RowAseImport *RowAseImport_ctor(struct RowAseImport *self);
001b9e70 void RowAseImport_dtor(struct RowAseImport *self);
001b9e80 struct RowAseImport *RowAseImport_DeletingDtor(struct RowAseImport *self, unsigned int flags);
001b9ea0 void RowAseImport_ReadAllShapes(struct RowAseImport *self, struct RowAseFile *file, struct RowPath **paths, int *count);
001ba160 void RowAseImport_ReadShapes(struct RowAseImport *self, struct RowAseFile *file, struct RowPath **paths, int *count, const char *name, const char *name2);

# Rowing/CRowing_objects.cpp (0x001b6330, 0x001b6730, 0x001b6a90 tagged; the rest inferred)
001b6270 unsigned int RowNode_GetPasses(void *node);
001b62f0 struct RowObject *RowObject_ctor(struct RowObject *self);
001b6300 void RowObject_dtor(struct RowObject *self);
001b6330 int RowObject_Init(struct RowObject *self, void *actor);
001b6420 int RowObject_Update(struct RowObject *self, float dt);
001b6470 void RowObject_Draw(struct RowObject *self, unsigned int pass);
001b6490 void RowModel_Free(struct RowModel *self);
001b64e0 int RowModel_Load(struct RowModel *self, void *package, const char *name);
001b6580 void *RowFloaty_ctor(void *self);
001b65d0 void RowModel_AddInstance(struct RowModel *self, struct RowObject *obj);
001b6610 int RowObjects_Update(void *list, float dt);
001b6650 void RowObjects_Draw(void *list, unsigned int pass);
001b6690 struct RowObject *RowObject_DeletingDtor(struct RowObject *self, unsigned int flags);
001b66d0 void RowObject_SetPos(struct RowObject *self, float *pos);
001b6700 void RowObject_SetRot(struct RowObject *self, float *rot);
001b6730 struct RowObject *RowModel_CreateInstance(struct RowModel *self, float *pos, float *rot);
001b6a70 void *RowFloaty_DeletingDtor(void *self, unsigned int flags);
001b6a90 void *RowObjects_Add(void *list, void *package, const char *name, float *pos, float *rot);
001b6b50 void RowObjects_Free(void *list);
001b6fe0 struct RowObject *RowBuoy_ctor(void *self);
001b70d0 void RowBuoy_DrawLight(struct RowObject *self);
001b7370 struct RowObject *RowRaceBuoy_ctor(void *self);
001b7390 int RowRaceBuoy_UpdateLights(struct RowObject *self);
001b73e0 void RowRaceBuoy_DrawLights(struct RowObject *self);
001b7710 struct RowObject *RowCrane_ctor(void *self);
001b7770 void RowCrane_dtor(struct RowObject *self);
001b77c0 void RowCrane_AnimDone(struct RowObject *self);
001b77d0 int RowCrane_NextAnim(struct RowObject *self);
001b78d0 void RowCrane_DrawLight(struct RowObject *self);
001b7a00 void RowFloaty_Sink(void *self);
001b7a30 void RowFloaty_dtor(void *self);
001b7aa0 void RowFloaty_SetHit(void *self);
001b7ab0 void *RowBuoy_DeletingDtor(void *self, unsigned int flags);
001b7ad0 void RowObject_dtor_thunk(struct RowObject *self);
001b7ae0 int RowBuoy_Init(struct RowObject *self, void *actor, float *pos, float *rot);
001b7c00 int RowBuoy_Update(struct RowObject *self, float dt);
001b7e60 void RowBuoy_Draw(struct RowObject *self, unsigned int pass);
001b7e90 int RowRaceBuoy_Update(struct RowObject *self, float dt);
001b80d0 void RowRaceBuoy_Draw(struct RowObject *self, unsigned int pass);
001b8100 void RowRaceBuoy_FindGate(struct RowObject *self);
001b82c0 void *RowCrane_DeletingDtor(void *self, unsigned int flags);
001b82e0 int RowCrane_Init(struct RowObject *self, void *actor, int kind);
001b8440 int RowCrane_Update(struct RowObject *self, float dt);
001b8500 void RowCrane_Draw(struct RowObject *self, unsigned int pass);
001b8540 int RowFloaty_OnHit(void *self, void *boat, void *a, void *b, float *out);
001b86a0 int RowFloaty_Init(void *self, void *actor, int type, float *pos, float *rot);
001b8760 void RowFloaty_Spawn(void *self, float atStart, float speed, int target);
001b8950 int RowFloaty_Update(void *self, float dt);
001b8c10 void RowFloaty_Draw(void *self, unsigned int pass);
001b8df0 void RowFloaty_AddPull(float *out, float *pos, float *other, int type);
001b8fe0 float RowFloaty_SumPull(float *pos);
001b9030 int RowFloaty_CheckTouch(float *pos);
001b9140 int RowRaceBuoy_Init(struct RowObject *self, void *actor, float *pos, float *rot);

# Rowing/Rowing_Sound.cpp (0x001b6da0 tagged; 0x001b6ba0-0x001b6f50 inferred)
001b6ba0 int RowSound_Play(int index, int volume);
001b6be0 int RowSound_PlayMoving(int index, float *pos, int volume);
001b6c30 void RowSound_Queue(int index, int volume);
001b6c80 void RowSound_SetFrequency(int channel, int index, float scale);
001b6cc0 void RowSound_Update(void *queue);
001b6d60 void RowSound_Reserve(void *queue, int channel);
001b6da0 void RowSound_Init(void *package, void *languagePackage);
001b6ee0 void RowSound_Queue3D(int index, float *pos, int volume);
001b6f50 void RowSound_Free(void);
