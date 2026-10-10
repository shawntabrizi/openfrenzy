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

# the game mode and the text reader (gap)
001d7c70 void TankReader_Ctor(void *self);
001d7c80 void TankReader_Dtor(void *self);
001d7c90 char *TankReader_GetLine(void *self, char *buffer);

# TankedUp/Decal.cpp
001de090 void *TankDecal_Ctor(void *self, int maxPoints, float width, float life, void *texture, void *texture2);
001de140 void TankDecal_AddPoint(void *self, float *pos, float *normal, float *dir, float alpha);
001de300 void TankDecal_Draw(void *self);

# TankedUp/CTank_MoveQue.cpp
001de940 void TankMoveQue_Free(void *self);
