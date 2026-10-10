# meta_a prototype overrides (docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;

# Meta/CGameMode_Meta.c
0014f4a0 void FUN_0014f4a0(void);

# Meta/unplaced_player.c
00152f30 undefined4 *FUN_00152f30(undefined4 *self);
00151090 int FUN_00151090(void *self, int level);
00153020 void FUN_00153020(int group);

# Meta/puzzles.c
00153080 int FUN_00153080(int puzzle);

# Meta/unplaced_metaplayer.c
00155f70 undefined4 FUN_00155f70(int ai, float *to);
001563e0 uint FUN_001563e0(int ai, int alive_only);
00156c40 void FUN_00156c40(int ai, float *target, float damage, int mode, int drop);
001573d0 uint FUN_001573d0(int ai, int alive_only);
00157bd0 void FUN_00157bd0(int ai);

# Meta/unplaced_tiles.c
00159000 void FUN_00159000(int list);
001592e0 void FUN_001592e0(int board);

# Meta/Tiles_Meta.c
0015a040 void *FUN_0015a040(void *board, float *pos, int group, unsigned int mask);
0015a110 void FUN_0015a110(undefined4 *self, undefined4 *pos, float time);

# Meta/unplaced_board.c
0015b050 void FUN_0015b050(int list);

# Meta/Effects.c
0015be90 void FUN_0015be90(float *pos, int count);
0015c4b0 void FUN_0015c4b0(int sprite);
0015c7a0 void FUN_0015c7a0(char *model, float *pos, float speed_u, float speed_v, int tag);
0015c400 int FUN_0015c400(float *pos, float width, float height, char *texture, int r, int g, int b, int a, float offset);

# Meta/unplaced_lightning.c
0015cc80 void FUN_0015cc80(float *from, float *to, int segments, float brightness);
