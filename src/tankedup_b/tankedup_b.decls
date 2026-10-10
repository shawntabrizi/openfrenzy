# src/tankedup_b/tankedup_b.decls - prototype overrides of the tankedup_b module
# (docs/ARCHITECTURE.md, section 11). <addr> <prototype>;
# x86 `this` (ECX) is the first parameter of methods. Objects are `void *` (their layouts are
# offset tables in tankedup_b.h), so tankedup_a can call them without this module's types.
# Float arguments and results are real floats (read from the x87 code).

# TankedUp/CTank_Entities.cpp
001cee60 int TankEnts_SpawnEntity(void *self, void **outNode);
001cef60 void TankEnts_LoadEntities(void *self, void *reader);
001d0310 void TankEnts_EntityLoadTemplate(void *self, void *entity);
001d0920 void TankEnts_Draw(void *self);
001d14e0 void TankEnts_DrawSpecial(void *self);
001d1770 void TankEnts_GetCameraBounds(void *self, float *centre, float *maxs, float *mins);
001d1a60 float TankEnts_AngleToPoint(void *self, void *entity, float *point);
001d1ac0 void TankEnts_StoreMessage(void *self, void *message);
001d1b40 void TankEnts_RemoveMessages(void *self, int id, int type);
001d1b90 void TankEnts_ProcessMessages(void *self);
001d1bf0 void TankEnts_FlushMessages(void *self);
001d1c30 int TankEnts_GetEntityInfo(void *self, int id, float *pos, float *vel, float *rot, float *radius, float *turret);
001d1d10 void TankEnts_GetEntityPos(void *self, int id, float *pos);
001d1d60 void TankEnts_Push(void *self, int id, float amount, int raw);
001d1e80 void TankEnts_DriveTracks(void *self, int id, float left, float right);
001d1f90 void TankEnts_DriveAnalog(void *self, int id, float angle, float speed);
001d20e0 void TankEnts_DriveStick(void *self, int id, float x, float y);
001d2200 void TankEnts_DriveAnalogLinear(void *self, int id, float angle, float speed);
001d2340 void TankEnts_DriveStickLinear(void *self, int id, float x, float y);
001d2460 float TankEnts_RayFraction(void *self, float *from, float *to, int ignore);
001d2590 void TankEnts_UpdateTracks(void *self);
001d26e0 void TankEnts_KillBuriedTanks(void *self);
001d2770 void TankEnts_LinkEntityTemplates(void *self);
001d2880 void TankEnts_SendDelayedMessage(void *self, int type, unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5, float delay);
001d2900 void TankEnts_DestroyEntity(void *self, void **node);
001d2a00 int TankEnts_CreateEntity(void *self, int owner, const char *templateName, float *pos, float *rot, float push);
001d2c80 void TankEnts_Reverse(void *self, int id);
001d2d00 void TankEnts_EntityHit(void *self, void *ent, void *other);
001d2dc0 int TankEnts_CollideEntity(void *self, void **node);
001d35e0 void TankEnts_CheckWaypoints(void *self, void *entity);
001d3850 void TankEnts_UpdatePhysics(void *self, int variant);
001d3e90 void TankEnts_SetVariant(void *self, int unused, int variant);
001d40a0 void TankEnts_ScatterRocks(void *self, int count);
001d41d0 int TankEnts_Initialise(void *self, void *package, const char *name, void *world, void *route, void *spawn, void *boom, void *lights, void *sound, void *players);

# TankedUp gap functions after CTank_Entities.cpp (sound, characters, drawing helpers)
001d5960 int TankSound_Initialise(void *self);
001d5a50 int TankSound_Play(void *self, unsigned int sound, void *pos, int volume, int unused, int pitch);
001d6600 void TankChar_SubstituteTag(char *name, const char *tag, int letter);
001d67c0 int TankChar_SingleHumanMode(int *player);
001d6870 void TankEnts_DirToAngles(float *dir, float *pitch, float *heading);
001d68f0 void TankChar_SubstituteColours(char *name);
001d6b40 void TankDraw_SetAlphaFactor(int alpha);

# the game mode CGameMode_TankedUp and the text reader (gap; TankedUp/unplaced_mode.c)
001d7b00 void *TankMode_Ctor(void *self);
001d7b20 void TankMode_Dtor(void *self);
001d7b30 void TankMode_Pushed(void *self);
001d7b60 void TankMode_Resume(void *self);
001d7b70 void TankMode_Suspend(void *self);
001d7b80 void TankMode_Input(void *self, int active);
001d7b90 void TankMode_Update(void *self, int active);
001d7bc0 void *TankMode_DeletingDtor(void *self, unsigned int flags);
001d7be0 void TankMode_Popped(void *self);
001d7c20 void TankMode_Render(void *self, int active);
001d7c30 void TankMode_LoadingStarted(void *self);
001d7c40 void TankMode_Start(void *self);
001d7c70 void TankReader_Ctor(void *self);
001d7c80 void TankReader_Dtor(void *self);
001d7c90 char *TankReader_GetLine(void *self, char *buffer);
001d7d40 void *TankReader_DeletingDtor(void *self, unsigned int flags);

# TankedUp/Decal.cpp
001de090 void *TankDecal_Ctor(void *self, int maxPoints, float width, float life, void *texture, void *texture2);
001de140 void TankDecal_AddPoint(void *self, float *pos, float *normal, float *dir, float alpha);
001de300 void TankDecal_Draw(void *self);

# TankedUp/CTank_MoveQue.cpp (0x001de780 tagged; the rest trailing, inferred)
001de780 void *TankMoveQue_Ctor(void *self, int count);
001de7c0 void TankMoveQue_Get(void *self, float *a, float *b, float *c, float *d);
001de890 void TankMoveQue_Put(void *self, float a, float b, float c, float d);
001de940 void TankMoveQue_Free(void *self);

# gap 0x001d4730-0x001d5790: CTank_Entities methods (file inferred)
001d4730 void TankEnts_Free(void *self);
001d4830 void TankEnts_Fire(void *self, void *entity, float offset);
001d4cc0 int TankEnts_FireCommand(void *self, int id);
001d4e10 void TankEnts_Explode(void *self, void *entity);
001d55a0 void TankEnts_ProcessDead(void *self);
001d5790 int TankEnts_Update(void *self, float dt, int state);

# gap 0x001d79d0-0x001d9b50 (the game mode, the text reader and AI helpers)
001d86d0 void TankEnts_GetRespawnPos(void *self, void *entity);

# TankedUp/CTank_Debug.cpp (0x001d6d90, 0x001d6f10 tagged; the rest inferred)
001d6cb0 void *TankDebug_Ctor(void *self);
001d6cd0 void TankDebug_Dtor(void *self);
001d6ce0 void TankDebug_CreateInput(void *self);
001d6d40 void TankDebug_ReleaseInput(void *self);
001d6d80 int TankDebug_GetDetail(void *self);
001d6d90 int TankDebug_Initialise(void *self);
001d6f10 void TankDebug_Setup(void *self);
001d79d0 void TankDebug_Free(void *self);

# CTank_Sound (gap 0x001d5880-0x001d5a50; the class name is in its debug text)
001d5880 void *TankSound_Ctor(void *self);
001d5a10 void TankSound_Free(void *self);

# TankedUp/CTank_Char.cpp (0x001d5b80 tagged; the rest inferred)
001d5af0 void *TankChar_Ctor(void *self);
001d5b00 void TankChar_PlayAnim(void *self, int player, const char *name);
001d5b80 int TankChar_Initialise(void *self, void *players, void *lights);
001d5da0 void TankChar_Free(void *self);
001d5de0 void TankChar_StateChange(void *self, int from, int to);
001d5ee0 int TankChar_Update(void *self, float dt, int state);
001d6090 void TankChar_Draw(void *self);

# TankedUp gap 0x001d62d0-0x001d6c40 (TankedUp/unplaced.c)
001d62d0 void TankRanks(int *scores, int *ranks);
001d6330 void TankMesh_DrawPart(void *mesh, int part, unsigned int colour, int shaded);
001d6680 void TankMesh_Draw(void *mesh, unsigned int colour, int shaded);
001d6750 void TankTexture_SetTime(void *texture, int time);
001d6820 void TankDraw_DecalStates(void);
001d6860 void TankDraw_StencilOn(void);
001d6960 void TankDebug_DrawCone(float *pos, float *target, float angle, int r, int g, int b);
001d6bd0 void TankPoint_RotateAround(float *out, float *point, float angle, float *centre);
001d6c40 void TankPoint_SetDistance(float *point, float distance, float *centre);

# TankedUp/CTank_AI.cpp (0x001d9bb0 and later tagged; 0x001d7d80-0x001d9b50 gap, inferred)
001d7d80 int TankAI_PositionClear(void *self, float *pos, void *ignore);
001d7e10 int TankAI_CommandPoint(void *self, void *entity, int event, int *data);
001d8440 void TankAI_MineExplode(void *self, void *entity);
001d84e0 int TankAI_Mine(void *self, void *entity, int event, int *data);
001d8820 int TankAI_Think(void *self, void *entity, int event, int *data);
001d9b50 void TankEnts_GetTanks(void *self, float *positions, int *scores);
001d9bb0 int TankAI_GetTokenSpawn(void *self, int *unused, float *pos, float *rot);
001d9d30 int TankAI_TokenMaster(void *self, void *entity, int event, int *data);
001da1c0 int TankAI_Token(void *self, void *entity, int event, int *data);
001dab40 int TankAI_DropBox(void *self, void *entity, int event, int *data);
001db5a0 int TankAI_NoFloor(void *self, void *entity, int event, int *data);
001dba50 int TankAI_DropZone(void *self, void *entity, int event, int *data);
