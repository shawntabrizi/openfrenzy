# src/carryon_b/carryon_b.decls - prototype overrides of the carryon_b module (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# CarryOn/CFire_Conveyor.c
000ca230 void CFire_Conveyor_Init(struct CFire_Conveyor *self, float *pos, int arg, struct CFire_Path *paths, int count, int unused, int count2);
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

# CarryOn/CFire_Fireworks.c
000cf9a0 int CFire_Fireworks_Launch(struct CFire_Fireworks *self, float *pos, float *vel, float r, float g, float b);
000cf9e0 int CFire_Fireworks_LaunchRandom(struct CFire_Fireworks *self, float *pos, float *vel);

# CarryOn/CFire_Sprites.c
000d4160 void CFire_Sparks_Emit(struct CFire_Sparks *self, float *pos, int count, int r, int g, int b);
