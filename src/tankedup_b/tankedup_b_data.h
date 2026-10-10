/*
TANKEDUP_B_DATA.H

The TankedUp globals that tankedup_b uses, with their Ghidra names. All of them are in the
zero-initialised part of .data (.bss, above 0x002b9478) except the colour letters
0x002b74a4.
- 0x00312f50-0x003153a0 is tankedup_a's block (CTank_Game, and the CTank_Debug object of this
  module at 0x003150b0): tankedup_a defines it and exports the DAT_ names used here as
  aliases (src/tankedup_a/TankedUp/tu_data.c).
- Everything else is defined by this module in TankedUp/tu_data.c.
Owner: tankedup_b.
*/

#ifndef TANKEDUP_B_DATA_H
#define TANKEDUP_B_DATA_H

/* a vertex of the decal and shadow quads (fvf 0x152: xyz, normal, diffuse, tex1) */
struct TankVertex {
	float pos[3];
	float normal[3];
	unsigned int colour;
	float uv[2];
};

/* 0x002b74a4: the colour letter of each character for "<playerN>" in model names */
extern char s_grbyop_002b74a4[];

/* tweakables and debug switches of the entity system */
extern int DAT_003150c0;            /* debug: collision spheres */
extern float DAT_003150b4;          /* the analog turn band, degrees */
extern float DAT_00315158;          /* the speed-up per frame of a tank */
extern float DAT_00315178;          /* the camera look-ahead distance (shared with tankedup_a) */
extern int DAT_0031519c;            /* draw the track decals */
extern int DAT_003151a4;            /* debug: always the full-detail models */
extern void *DAT_003151b4;          /* the TankedUp tweak menu (parent of "tokens") */
extern float DAT_003150b8;          /* debris speed */
extern float DAT_0031516c;          /* the triple shot's spread, degrees */
extern float DAT_00315170;          /* the cannon's length, in tank radii */
/* the AI tweakables (CTank_Debug) */
extern float DAT_003150ec;          /* "AutoFireAng" degrees */
extern float DAT_003150f0;          /* "AutoFireDist" */
extern float DAT_003150f4;          /* "target Rand Dist" */
extern float DAT_003150f8;          /* "Vindictiveness" */
extern float DAT_003150fc;          /* "Move on target" (wander) degrees */
extern float DAT_00315160;          /* AI "speed" */
extern float DAT_00315164;          /* "token Get Dist" */
extern float DAT_00315168;          /* "rotCost" */
extern float DAT_003151bc;          /* "Local RF" MoveAng degrees */
extern float DAT_003151c0;          /* "Local RF" LookAng degrees */
extern float DAT_003151c4;          /* "Local RF" SideSpace */

/* vertex buffers: 0x003153a0 the ground shadow quad, 0x00315430 the world mesh debug
   triangle, 0x003154c0 the light cone fan, 0x00315998 / 0x00315a28 the track decal quad and
   joint triangle */
extern struct TankVertex DAT_003153a0[4];
extern struct TankVertex DAT_00315430[4];
extern struct TankVertex DAT_003154c0[34];
extern struct TankVertex DAT_00315998[4];
extern struct TankVertex DAT_00315a28[3];

/* the Token Master AI's spawn list: a function-local static of 0x001d9d30 (guard bit 0 of
   0x00315990; the exit funclet 0x001f2860 clears it) */
extern void *DAT_00315988;
extern int DAT_0031598c;
extern unsigned int DAT_00315990;

/* zero velocity of other modules (classlib_fx owns it) */
extern float DAT_002ea478[4];

#endif
