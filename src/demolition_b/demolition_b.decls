# src/demolition_b/demolition_b.decls - prototype overrides of the demolition_b module
# (docs/ARCHITECTURE.md, section 11). <addr> <prototype>;  thiscall `this` is the first parameter.

# Demolition/unplaced.c: explosions (probably CDemolition_Explosion.cpp)
000f0bf0 void CDemolition_ExplosionMgr_Start(struct CDemolition_ExplosionMgr *self, void *source, void *owner, float *pos, float time, float power);
000f0d10 void CDemolition_Explosion_DamageObjects(struct CDemolition_Explosion *self, float radius);
000f0d50 void CDemolition_ExplosionMgr_Draw(struct CDemolition_ExplosionMgr *self);
000f0dc0 void CDemolition_Explosion_Update(struct CDemolition_Explosion *self, float dt);
000f0ea0 void CDemolition_ExplosionMgr_Update(struct CDemolition_ExplosionMgr *self, float dt);
000f0ee0 void CDemolition_BombFx_Destruct(struct CDemolition_BombFx *self);

# Demolition/CDemolition_Bomb.c
000f0f30 void CDemolition_FragBomb_Destruct(struct CDemolition_FragBomb *self);
000f0fa0 void CDemolition_FragBomb_Release(struct CDemolition_FragBomb *self);
000f0fd0 void CDemolition_FragBomb_Reset(struct CDemolition_FragBomb *self);
000f1050 void CDemolition_FragBomb_Throw(struct CDemolition_FragBomb *self, float *velocity);
000f10d0 void CDemolition_FragBomb_PickUp(struct CDemolition_FragBomb *self);
000f1120 struct CDemolition_RemoteBomb *CDemolition_RemoteBomb_Construct(struct CDemolition_RemoteBomb *self);
000f1190 void CDemolition_RemoteBomb_Destruct(struct CDemolition_RemoteBomb *self);
000f1200 int CDemolition_RemoteBomb_CreateActor(struct CDemolition_RemoteBomb *self, int player);
000f1320 int CDemolition_RemoteBomb_CreateInput(struct CDemolition_RemoteBomb *self, int player);
000f1450 void CDemolition_RemoteBomb_UpdateArmed(struct CDemolition_RemoteBomb *self, float dt);
000f1480 void CDemolition_RemoteBomb_UpdateGrow(struct CDemolition_RemoteBomb *self, float dt);
000f1500 void CDemolition_RemoteBomb_UpdateShrink(struct CDemolition_RemoteBomb *self, float dt);
000f1590 void CDemolition_RemoteBomb_Draw(struct CDemolition_RemoteBomb *self);
000f15b0 void CDemolition_RemoteBomb_DrawMarker(struct CDemolition_RemoteBomb *self);
000f17d0 void CDemolition_RemoteBomb_Reset(struct CDemolition_RemoteBomb *self);
000f1830 void CDemolition_RemoteBomb_Recall(struct CDemolition_RemoteBomb *self);
000f1880 int CDemolition_RemoteBomb_CanRoll(struct CDemolition_RemoteBomb *self);
000f18b0 int CDemolition_RemoteBomb_StateIdle(struct CDemolition_RemoteBomb *self, int entered);
000f1900 void CDemolition_TimedBomb_Destruct(struct CDemolition_TimedBomb *self);
000f1920 void CDemolition_TimedBomb_DrawFx(struct CDemolition_TimedBomb *self);
000f19c0 void CDemolition_CrossBomb_Destruct(struct CDemolition_CrossBomb *self);
000f1a20 void CDemolition_CrossBomb_Release(struct CDemolition_CrossBomb *self);
000f1a50 void CDemolition_Bomb_Draw(struct CDemolition_FragBomb *self);
000f1a80 void CDemolition_CrossBomb_DrawFx(struct CDemolition_CrossBomb *self);
000f1b40 void CDemolition_CrossBomb_Attach(struct CDemolition_CrossBomb *self, struct CDemolition_Player *player);
000f1ba0 struct CDemolition_BombSystem *CDemolition_BombSystem_Construct(struct CDemolition_BombSystem *self);
000f1bb0 void CDemolition_BombSystem_Destruct(struct CDemolition_BombSystem *self);
000f1bc0 void CDemolition_Game_UpdateBombs(void *game, float dt);
000f1bd0 void CDemolition_Game_DrawBombs(void *game);
000f1be0 void CDemolition_Game_DrawBombFx(void *game);
000f1bf0 void CDemolition_Game_StartBombs(void *game);
000f1c00 void CDemolition_Game_StopBombs(void *game);
000f1c10 void CDemolition_BombSystemFrag_Update(struct CDemolition_BombSystem *self, float dt);
000f1c70 void CDemolition_BombSystemFrag_Draw(struct CDemolition_BombSystem *self);
000f1ca0 void CDemolition_BombSystemFrag_DrawFx(struct CDemolition_BombSystem *self);
000f1ce0 void CDemolition_BombSystemFrag_Reset(struct CDemolition_BombSystem *self);
000f1d50 void CDemolition_BombSystemFrag_Stop(struct CDemolition_BombSystem *self);
000f1d70 void CDemolition_BombSystemRemote_Shutdown(struct CDemolition_BombSystem *self);
000f1db0 void CDemolition_BombSystemRemote_Update(struct CDemolition_BombSystem *self, float dt);
000f1df0 void CDemolition_BombSystemRemote_Draw(struct CDemolition_BombSystem *self);
000f1e30 void CDemolition_BombSystemRemote_DrawFx(struct CDemolition_BombSystem *self);
000f1e70 void CDemolition_BombSystemRemote_Reset(struct CDemolition_BombSystem *self);
000f1f00 void CDemolition_BombSystemRemote_Stop(struct CDemolition_BombSystem *self);
000f1f30 void CDemolition_BombSystemTimed_Destruct(struct CDemolition_BombSystem *self);
000f1fa0 void CDemolition_BombSystemTimed_Shutdown(struct CDemolition_BombSystem *self);
000f1fe0 void CDemolition_BombSystemTimed_Draw(struct CDemolition_BombSystem *self);
000f2030 void CDemolition_BombSystemTimed_DrawFx(struct CDemolition_BombSystem *self);
000f2070 void CDemolition_BombSystemTimed_Reset(struct CDemolition_BombSystem *self);
000f2110 void CDemolition_BombSystemTimed_Stop(struct CDemolition_BombSystem *self);
000f2130 void CDemolition_BombSystemList_Shutdown(struct CDemolition_BombSystem *self);
000f2160 void CDemolition_BombSystemCross_Draw(struct CDemolition_BombSystem *self);
000f2190 void CDemolition_BombSystemCross_DrawFx(struct CDemolition_BombSystem *self);
000f21c0 void CDemolition_BombSystemCross_Start(struct CDemolition_BombSystem *self);
000f21d0 void CDemolition_BombSystemCross_Stop(struct CDemolition_BombSystem *self);
000f21f0 void *CDemolition_RemoteBomb_Receiver_DeletingDestruct(void *receiver, unsigned int flags);
000f2200 struct CDemolition_FragBomb *CDemolition_FragBomb_Construct(struct CDemolition_FragBomb *self);
000f2280 void *CDemolition_FragBomb_DeletingDestruct(struct CDemolition_FragBomb *self, unsigned int flags);
000f22a0 int CDemolition_FragBomb_Init(struct CDemolition_FragBomb *self, int type);
000f2480 void CDemolition_FragBomb_FollowHolder(struct CDemolition_FragBomb *self, float dt);
000f2570 void CDemolition_FragBomb_Detonate(struct CDemolition_FragBomb *self, void *source, void *owner);
000f2600 void *CDemolition_RemoteBomb_DeletingDestruct(struct CDemolition_RemoteBomb *self, unsigned int flags);
000f2620 int CDemolition_RemoteBomb_Init(struct CDemolition_RemoteBomb *self, int player);
000f2710 void CDemolition_RemoteBomb_Shutdown(struct CDemolition_RemoteBomb *self);
000f2750 void CDemolition_RemoteBomb_Update(struct CDemolition_RemoteBomb *self, float dt);
000f2990 void CDemolition_RemoteBomb_DrawFx(struct CDemolition_RemoteBomb *self);
000f2a40 void CDemolition_RemoteBomb_Place(struct CDemolition_RemoteBomb *self);
000f2c20 void CDemolition_RemoteBomb_Detonate(struct CDemolition_RemoteBomb *self, void *source, void *owner);
000f2d80 int CDemolition_RemoteBomb_CheckContact(struct CDemolition_RemoteBomb *self);
000f2ea0 int CDemolition_RemoteBomb_StateRoll(struct CDemolition_RemoteBomb *self, int entered);
000f31b0 void CDemolition_TimedBomb_Explode(struct CDemolition_TimedBomb *self);
000f39a0 void CDemolition_CrossBomb_Detonate(struct CDemolition_CrossBomb *self, void *source, void *owner);

# Demolition/unplaced.c: CDemolition_Object (vtable 0x0026b0d4)
000f52c0 struct CDemolition_Object *CDemolition_Object_Construct(struct CDemolition_Object *self);
000f5310 void CDemolition_Object_Destruct(struct CDemolition_Object *self);
000f53c0 void CDemolition_Object_Stop(struct CDemolition_Object *self);
000f5480 void CDemolition_Object_SetPosition(struct CDemolition_Object *self, float *pos);
000f56e0 void CDemolition_Object_SetVelocity(struct CDemolition_Object *self, float *velocity);

# Demolition/unplaced.c: CDemolition_CharLib (vtable 0x0026b2c0)
000f8a50 struct CDemolition_CharLib *CDemolition_CharLib_Construct(struct CDemolition_CharLib *self);
000f8ac0 void CDemolition_CharLib_Destruct(struct CDemolition_CharLib *self);
000f8cc0 void CDemolition_CharLib_SetPosition(struct CDemolition_CharLib *self, float *pos);
000f8d30 void CDemolition_CharLib_SetRotation(struct CDemolition_CharLib *self, float *euler);

# Demolition/CDemolition_Player.c
000f99e0 void CDemolition_Player_SetCarry(struct CDemolition_Player *self, void *node, void *object);
