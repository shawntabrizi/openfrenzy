/*
TANKEDUP_B_DATA.H

The TankedUp globals that tankedup_b uses, with their Ghidra names. All of them are in the
zero-initialised part of .data (.bss, above 0x002b9478) except the colour letters
0x002b74a4. They are defined in TankedUp/tu_data.c as tentative (common) definitions, so a
second definition of the same name in tankedup_a (same type and size) merges with them
(-fcommon, docs/ARCHITECTURE.md section 12). Owner: tankedup_b.
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

/* 0x003153a0: the ground shadow quad */
extern struct TankVertex DAT_003153a0[4];

/* zero velocity of other modules (classlib_fx owns it) */
extern float DAT_002ea478[4];

#endif
