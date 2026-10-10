# src/carryon_b/carryon_b.decls - prototype overrides of the carryon_b module (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# CarryOn/CFire_Conveyor.c
000ca230 void CFire_Conveyor_Init(struct CFire_Conveyor *self, float *pos, float arg, struct CFire_Path *paths, int count, int unused, int count2);
000ca3f0 void CFire_Conveyor_Update(struct CFire_Conveyor *self, float dt, int arg2, int arg3, int arg4, int steps);
000ca470 void CFire_Conveyor_Draw(struct CFire_Conveyor *self);
000ca4c0 struct CFire_ConveyorItem *CFire_Conveyor_FindNearest(struct CFire_Conveyor *self, float *pos, int row2);
000ca690 void CFire_Conveyor_Free(struct CFire_Conveyor *self);

# CarryOn/CFire_LaunchRack.c
000ca730 struct CFire_LaunchRack *CFire_LaunchRack_ctor(struct CFire_LaunchRack *self);
000ca740 int CFire_LaunchRack_Init(struct CFire_LaunchRack *self, struct CFire_Fireworks *fireworks, float *pos, float width, float depth, float angle);
000ca830 void CFire_LaunchRack_Free(struct CFire_LaunchRack *self);
000ca880 void CFire_LaunchRack_CheckPlayers(struct CFire_LaunchRack *self);
000ca970 int CFire_LaunchRack_StartLaunch(struct CFire_LaunchRack *self);
000ca9e0 void CFire_LaunchRack_SlotLaunch(struct CFire_LaunchRack *self, int slot, float *pos, float *vel);
000cab20 void CFire_LaunchRack_NearestPoint(struct CFire_LaunchRack *self, float *out, float *pos);
000cacf0 void CFire_LaunchRack_Draw(struct CFire_LaunchRack *self);
000cb050 int CFire_LaunchRack_AddRocket(struct CFire_LaunchRack *self, int team);
000cb0c0 int CFire_LaunchRack_HasRoom(struct CFire_LaunchRack *self, int count);
000cb130 struct CFire_LaunchRack *CFire_LaunchRack_dtor(struct CFire_LaunchRack *self, unsigned int flags);
000cb150 void CFire_LaunchRack_Update(struct CFire_LaunchRack *self, float dt, struct CFire_Sparks *sparks);

# CarryOn/CFire_Lights.c
000d1010 struct CFire_Light *CFire_Lights_AddPoint(struct CFire_Lights *self, int group, float *pos, int r, int g, int b, float range, unsigned char flags);
000d10a0 struct CFire_Light *CFire_Lights_AddPointDefault(struct CFire_Lights *self, int group, int r, int g, int b, float range, unsigned char flags);
000d10d0 struct CFire_Light *CFire_Lights_AddDirectional(struct CFire_Lights *self, int group, float *dir, int r, int g, int b, unsigned char flags);
000d1160 struct CFire_Light *CFire_Lights_AddSpot(struct CFire_Lights *self, int group, float *pos, float *target, float angle, int r, int g, int b, float range, unsigned char flags);
000d1210 struct CFire_Light *CFire_Lights_AddSpotDefault(struct CFire_Lights *self, int group, float angle, int r, int g, int b, float range, unsigned char flags);
000d1250 void CFire_Lights_Attach(struct CFire_Lights *self, struct CFire_Light *light);
000d1390 void CFire_Lights_Enable(struct CFire_Lights *self, struct CFire_Light *light, int on);
000d1410 unsigned int CFire_Lights_SetFlags(struct CFire_Lights *self, struct CFire_Light *light, unsigned int set, unsigned int toggle);
000d1470 void CFire_Lights_SetColour(struct CFire_Lights *self, struct CFire_Light *light, int r, int g, int b);
000d14a0 void CFire_Lights_SetPos(struct CFire_Lights *self, struct CFire_Light *light, float *pos);
000d1510 void CFire_Lights_SetSpot(struct CFire_Lights *self, struct CFire_Light *light, float *pos, float *target);
000d1580 void CFire_Lights_SetAngle(struct CFire_Lights *self, struct CFire_Light *light, float angle);
000d15c0 void CFire_Lights_Update(struct CFire_Lights *self, float dt);
000d16f0 void CFire_Lights_Flash(struct CFire_Lights *self, float *pos, int r, int g, int b);
000d18a0 struct CFire_Lights *CFire_Lights_ctor(struct CFire_Lights *self);
000d19b0 void CFire_Lights_EnableGroup(struct CFire_Lights *self, int group, int on);


# CarryOn/CFire_Lights.c (continued)
000d0fb0 void CFire_Lights_Free(struct CFire_Lights *self);

# CarryOn/unplaced.c: rockets in the arena
000cb5d0 struct CFire_Rocket *CFire_Rocket_ctor(struct CFire_Rocket *self, int unused);
000cb5f0 float CFire_Rocket_RandomSpot(float *out);
000cb840 void CFire_Rocket_Drop(struct CFire_Rocket *self, struct BActorInstance *actor);
000cb890 void CFire_Rocket_Update(struct CFire_Rocket *self, float dt);
000cbb80 int CFire_Rocket_CheckCatch(struct CFire_Rocket *self, int player);
000cbc40 void CFire_Rocket_Throw(struct CFire_Rocket *self, float *pos, float angle, float speed, float vy, int type, struct BActorInstance *actor);
000cbcc0 void CFire_Rocket_Draw(struct CFire_Rocket *self, unsigned int passes);

# CarryOn/CFire_V2_Glitter.c
000cbe30 struct CFire_V2Glitter *CFire_V2Glitter_ctor(struct CFire_V2Glitter *self);
000cbe50 void CFire_V2Glitter_Draw(struct CFire_V2Glitter *self);
000cbf20 void CFire_V2Glitter_ShowScores(void);
000cbf90 void CFire_V2Glitter_InitLights(struct CFire_V2Glitter *self);
000cc090 void CFire_V2Glitter_Beam(struct CFire_V2Glitter *self, int unused, float *pos, int count);
000cc150 void CFire_V2Glitter_UpdateLights(struct CFire_V2Glitter *self, float dt);
000cc470 void CFire_V2Glitter_AddGlitter(struct CFire_V2Glitter *self, int player, int amount);
000cc510 struct CFire_Roman *CFire_V2Glitter_NearestRoman(struct CFire_V2Glitter *self, float *pos);
000cc520 int CFire_V2Glitter_PickRomans(struct CFire_V2Glitter *self, int unused, struct CFire_Roman *prefer, struct CFire_Roman **out);
000cc530 int CFire_V2Glitter_LaunchRoman(struct CFire_V2Glitter *self);
000cc870 struct CFire_GlowPot *CFire_GlowPot_ctor(struct CFire_GlowPot *self, float *pos, int player, float *modelPos);
000cc940 void CFire_GlowPot_Update(struct CFire_GlowPot *self, float unused);
000cca90 void CFire_GlowPot_Draw(struct CFire_GlowPot *self, int passes);
000ccb50 void CFire_V2Glitter_Init(struct CFire_V2Glitter *self);
000cce70 void CFire_V2Glitter_Update(struct CFire_V2Glitter *self, float dt);
000cd370 void CFire_V2Glitter_DrawDish(struct CFire_V2Glitter *self, int passes);
000cd4a0 void CFire_V2Glitter_Free(struct CFire_V2Glitter *self);

# CarryOn/CFire_Arena.c
000cd5c0 struct CFire_Pad *CFire_Pad_ctor(struct CFire_Pad *self, float *pos, int player, struct CFire_Fireworks *fireworks);
000cd720 void CFire_Pad_AddRocket(struct CFire_Pad *self);
000cd770 void CFire_Pad_Launch(struct CFire_Pad *self);
000cd940 void CFire_Pad_Update(struct CFire_Pad *self, float dt);
000cda30 void CFire_Pad_Draw(struct CFire_Pad *self, unsigned int passes);
000cde00 void CFire_Spawner_Update(struct CFire_Spawner *self, struct CFire_Rocket *rockets, int *active);
000ce110 struct CFire_Arena *CFire_Arena_ctor(struct CFire_Arena *self);
000ce130 void CFire_Arena_dtor(struct CFire_Arena *self);
000ce140 void CFire_Arena_Init(struct CFire_Arena *self, int variant, struct CFire_Fireworks *fireworks);
000ce3a0 void CFire_Arena_AddPad(struct CFire_Arena *self, float *pos, int player);
000ce420 void CFire_Arena_AddSpawner(struct CFire_Arena *self, float *pos, int dir);
000ce490 void CFire_Arena_Draw(struct CFire_Arena *self, unsigned int passes);
000ce5c0 void CFire_Arena_DrawShadows(struct CFire_Arena *self);
000ce7a0 void CFire_Arena_Update(struct CFire_Arena *self, float dt, int phase);
000cec70 int CFire_Arena_AnyFreeRocket(struct CFire_Arena *self, int player);
000cece0 struct CFire_Rocket *CFire_Arena_NearestFreeRocket(struct CFire_Arena *self, float *pos, int player);
000cedb0 struct CFire_Rocket *CFire_Arena_RandomLyingRocket(struct CFire_Arena *self);
000cee20 struct CFire_Rocket *CFire_Arena_RandomLyingRocketOfType(struct CFire_Arena *self, int type);
000ceea0 void CFire_Arena_Free(struct CFire_Arena *self);
000cef40 void CFire_Arena_DrawShadowPass(struct CFire_Arena *self);

# CarryOn/CFire_Fireworks.c
000cf200 void CFire_Fireworks_Init(struct CFire_Fireworks *self, int variant);
000cf6d0 void CFire_Fireworks_BurstStars(struct CFire_Fireworks *self, float *pos, float *vel, float r, float g, float b);
000cf810 void CFire_Fireworks_BurstRing(struct CFire_Fireworks *self, float *pos, float *vel, int count, float size, float speed, float r, float g, float b);
000cf9a0 int CFire_Fireworks_Launch(struct CFire_Fireworks *self, float *pos, float *vel, float r, float g, float b);
000cf9e0 int CFire_Fireworks_LaunchRandom(struct CFire_Fireworks *self, float *pos, float *vel);
000cfa40 void CFire_Fireworks_MaxHeight(struct CFire_Fireworks *self, float *height, float scale);
000cfaa0 void CFire_Fireworks_Burst(struct CFire_Fireworks *self, struct CFire_Shell *shell);
000cff30 void CFire_Fireworks_DrawShells(struct CFire_Fireworks *self);
000d0000 struct CFire_Shell *CFire_Fireworks_AddShell(struct CFire_Fireworks *self, float *pos, float *vel, int t0, int t1, int t2, int t3, int t4, int player);
000d02e0 void CFire_Fireworks_AddShellOfType(struct CFire_Fireworks *self, float *pos, float *vel, int type, int player);
000d0360 void CFire_Fireworks_Free(struct CFire_Fireworks *self);
000d0450 void CFire_Fireworks_Draw(struct CFire_Fireworks *self);
000d04c0 void CFire_Fireworks_UpdateRockets(struct CFire_Fireworks *self);
000d0610 void CFire_Fireworks_Split(struct CFire_Fireworks *self, struct CFire_Shell *shell, struct CFire_Shell ***link);
000d0790 void CFire_Fireworks_UpdateShells(struct CFire_Fireworks *self);
000d0f40 void CFire_Fireworks_Update(struct CFire_Fireworks *self);

# CarryOn/unplaced.c: smoke, frame timer, game mode
000d19f0 void CFire_Smoke_Update(struct CFire_Smoke *self, float unused);
000d1a80 void CFire_Smoke_Draw(struct CFire_Smoke *self);
000d1b10 void CFire_Smoke_Add(struct CFire_Smoke *self, float *pos, float size, float life, float spread);
000d1dd0 void CFire_Smoke_Free(struct CFire_Smoke *self);
000d1e10 void CFire_Timer_ctor(struct CFire_Timer *self);
000d1e50 void CFire_Timer_dtor(struct CFire_Timer *self);
000d1e60 int CFire_Timer_Init(struct CFire_Timer *self);
000d1eb0 void CFire_Timer_Frame(struct CFire_Timer *self);
000d1f50 void CFire_Timer_End(struct CFire_Timer *self, int section);
000d1fc0 void CFire_Timer_Mark(struct CFire_Timer *self, unsigned int colour);
000d2040 void CFire_DrawRect(float x, float y, float w, float h, unsigned int colour);
000d21a0 void CFire_Timer_Draw(struct CFire_Timer *self);
000d2270 struct CFire_Timer *CFire_Timer_dtor_del(struct CFire_Timer *self, unsigned int flags);
000d2290 struct CGameMode_CarryOn *CGameMode_CarryOn_ctor(struct CGameMode_CarryOn *self);
000d22b0 void CGameMode_CarryOn_dtor(struct CGameMode_CarryOn *self);
000d22c0 void CGameMode_CarryOn_Pushed(struct CGameMode_CarryOn *self);
000d22e0 void CGameMode_CarryOn_Popped(struct CGameMode_CarryOn *self);
000d2360 void CGameMode_CarryOn_Input(struct CGameMode_CarryOn *self, int active);
000d2370 void CGameMode_CarryOn_Update(struct CGameMode_CarryOn *self, int active);
000d23b0 void CGameMode_CarryOn_Render(struct CGameMode_CarryOn *self, int active);
000d2400 void CGameMode_CarryOn_LoadingStarted(struct CGameMode_CarryOn *self);
000d2430 void CGameMode_CarryOn_Start(struct CGameMode_CarryOn *self);
000d24a0 struct CGameMode_CarryOn *CGameMode_CarryOn_dtor_del(struct CGameMode_CarryOn *self, unsigned int flags);

# CarryOn/CFire_Sprites.c
000cef60 void CFire_QuadSprites_Add(struct CFire_QuadSprites *self, float *pos, float size, unsigned int colour);
000d24c0 struct CFire_QuadSprites *CFire_QuadSprites_ctor(struct CFire_QuadSprites *self);
000d24e0 int CFire_QuadSprites_Init(struct CFire_QuadSprites *self, int blendMode, struct bkResource *texture, int count);
000d2650 void CFire_QuadSprites_Begin(struct CFire_QuadSprites *self);
000d2750 struct CFire_PointSprites *CFire_PointSprites_ctor(struct CFire_PointSprites *self);
000d2770 void CFire_PointSprites_Free(struct CFire_PointSprites *self);
000d27a0 int CFire_PointSprites_Init(struct CFire_PointSprites *self, int blendMode, struct bkResource *texture, unsigned int count, float pointSize);
000d2870 struct CFire_Sparks *CFire_Sparks_ctor(struct CFire_Sparks *self);
000d2880 void CFire_Sparks_Draw(struct CFire_Sparks *self);
000d2b00 int CFire_Trails_Init(struct CFire_Trails *self, struct bkResource *texture, int count);
000d2ba0 void CFire_Trails_Free(struct CFire_Trails *self);
000d2be0 void CFire_Trails_Draw(struct CFire_Trails *self);
000d2ec0 void CFire_QuadSprites_Flush(struct CFire_QuadSprites *self);
000d3060 void CFire_PointSprites_Begin(struct CFire_PointSprites *self);
000d31f0 void CFire_PointSprites_Flush(struct CFire_PointSprites *self);
000d3290 struct CFire_Sparks *CFire_Sparks_dtor(struct CFire_Sparks *self, unsigned int flags);
000d32b0 void CFire_Sparks_Start(struct CFire_Sparks *self, struct CFire_Spark *spark, float *pos, int r, int g, int b, float fadeIn);
000d35c0 void CFire_Sparks_StartSmall(struct CFire_Sparks *self, struct CFire_Spark *spark, float *pos, int r, int g, int b);
000d38e0 void CFire_Sparks_StartMoving(struct CFire_Sparks *self, struct CFire_Spark *spark, float *pos, float *vel, int r, int g, int b);
000d3c00 void CFire_Sparks_StartMoving2(struct CFire_Sparks *self, struct CFire_Spark *spark, float *pos, float *vel, int r, int g, int b);
000d3f20 int CFire_Sparks_Init(struct CFire_Sparks *self, struct bkResource *texture, int count);
000d4030 void CFire_Sparks_Burst(struct CFire_Sparks *self, float *pos, int count, float spread, int r, int g, int b, float fadeIn);
000d4160 void CFire_Sparks_Emit(struct CFire_Sparks *self, float *pos, int count, int r, int g, int b);
000d4210 void CFire_Sparks_EmitRandom(struct CFire_Sparks *self, float *pos, float *vel, int count);
000d42c0 void CFire_Sparks_EmitMoving(struct CFire_Sparks *self, float *pos, float *vel, int count, int r, int g, int b);
000d4370 void CFire_Sparks_Update(struct CFire_Sparks *self, float dt);
000d4600 void CFire_Trails_Add(struct CFire_Trails *self, float *pos, float *origin, short r, short g, short b);
000d4730 void CFire_Trails_Update(struct CFire_Trails *self, float unused);
000d7670 void CFire_PointSprites_Add(struct CFire_PointSprites *self, float *pos, float size, unsigned int colour);

# CarryOn/FWorkTypes.c
000d7720 void CFire_RandomColour(float *r, float *g, float *b);
000d77e0 int CFire_FWorkStars_Init(struct CFire_FWorkStars *self, int count, float pointSize);
000d7900 void CFire_FWorkStars_Update(struct CFire_FWorkStars *self);
000d7a10 void CFire_FWorkStars_Draw(struct CFire_FWorkStars *self);
000d7b50 struct CFire_Star *CFire_FWorkStars_Add(struct CFire_FWorkStars *self, float *pos, float *vel, float gravity, float size, float r, float g, float b, float fadeAt);
000d7c90 int CFire_FWorkSmoke_Init(struct CFire_FWorkSmoke *self, int count);
000d7da0 void CFire_FWorkSmoke_Update(struct CFire_FWorkSmoke *self);
000d7e50 void CFire_FWorkSmoke_Draw(struct CFire_FWorkSmoke *self);
000d7f00 void CFire_FWorkSmoke_Add(struct CFire_FWorkSmoke *self, float *pos, float vy, float speed);

# CarryOn/CFire_Glitter.c
000d80f0 int CFire_Glitter_Init(struct CFire_Glitter *self, int count, float pointSize);
000d8270 void CFire_Glitter_Update(struct CFire_Glitter *self);
000d8330 void CFire_Glitter_Draw(struct CFire_Glitter *self);
000d83b0 struct CFire_GlitterBit *CFire_Glitter_Emit(struct CFire_Glitter *self, float *pos, float unused, float size, float speed);
000d84c0 struct CFire_GlitterBit *CFire_Glitter_EmitSphere(struct CFire_Glitter *self, float *pos, float unused, float size, float speed);
000d8600 int CFire_Glitter_Collect(struct CFire_Glitter *self, float *pos, float radius, float height, int *count);
000d8720 void CFire_Glitter_Free(struct CFire_Glitter *self);

# CarryOn/CFire_Romans.c
000d8760 int CFire_Romans_Init(struct CFire_Romans *self, int count);
000d88d0 void CFire_Romans_UpdateLight(struct CFire_Romans *self, struct CFire_Roman *roman);
000d8a70 void CFire_Romans_Update(struct CFire_Romans *self, float dt, struct CFire_Glitter *glitter, struct CFire_Sparks *sparks);
000d8ec0 void CFire_Romans_Draw(struct CFire_Romans *self);
000d8fd0 struct CFire_Roman *CFire_Romans_Launch(struct CFire_Romans *self, float *pos, float *vel, float gravity, float size, float r, float g, float b, float fuse);
000d90f0 struct CFire_Roman *CFire_Romans_Nearest(struct CFire_Romans *self, float *pos);
000d9150 int CFire_Romans_Count(struct CFire_Romans *self);
000d9170 int CFire_Romans_Pick(struct CFire_Romans *self, int unused, struct CFire_Roman *prefer, struct CFire_Roman **out);
000d9360 void CFire_Pool_Free(struct CFire_Pool *self);
