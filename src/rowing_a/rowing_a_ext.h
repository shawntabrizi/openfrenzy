/*
ROWING_A_EXT.H

Calls from rowing_a into other modules whose prototype in of_decls.h (Ghidra's guess) does
not have the real shape: thiscall `this` (ECX) missing, float parameters shown as integers,
results that are pointers. Each macro casts the FUN_<addr> symbol to the shape found in the
disassembly at the call sites (docs/ARCHITECTURE.md: "go through a cast to the real shape").

The rowing_b functions (0x001b12c0-0x001ba160) are listed here with the shapes rowing_a
uses; rowing_b should declare the same shapes in rowing_b.decls (see COORDINATION.txt).
A `float` below is a parameter whose value is a float constant at every call site.

Owner: rowing_a.
*/

#ifndef OPENFRENZY_ROWING_A_EXT_H
#define OPENFRENZY_ROWING_A_EXT_H

/* ---------- rowing_b: CRowing_Lights.cpp (the lights object, game +0xa4) */
#define rwb_Lights_Ctor(self) RA_CALL(FUN_001b4f90, void *(*)(void *))(self)
/* a point light: type, position, colour 0-127, falloff, flag (bit 0: on) */
#define rwb_Lights_AddPoint(lights, type, pos, r, g, b, falloff, flag) \
	RA_CALL(FUN_001b52e0, void *(*)(void *, int, float *, int, int, int, float, int)) \
	((lights), (type), (pos), (r), (g), (b), (falloff), (flag))
/* a directional light */
#define rwb_Lights_AddDirectional(lights, type, dir, r, g, b, flag) \
	RA_CALL(FUN_001b5370, void *(*)(void *, int, float *, int, int, int, int)) \
	((lights), (type), (dir), (r), (g), (b), (flag))
/* light texture and size (cdecl, no `this`) */
#define rwb_Light_SetTexture(light, texture, size) \
	RA_CALL(FUN_001b5010, void (*)(void *, void *, float))((light), (texture), (size))
#define rwb_Lights_SetFlags(lights, type, set, toggle) \
	RA_CALL(FUN_001b56a0, int (*)(void *, int, unsigned int, unsigned int))((lights), (type), (set), (toggle))
#define rwb_Lights_SetColour(lights, light, r, g, b) \
	RA_CALL(FUN_001b5590, void (*)(void *, void *, int, int, int))((lights), (light), (r), (g), (b))
#define rwb_Lights_SetMode(lights, mode) RA_CALL(FUN_001b5630, void (*)(void *, int))((lights), (mode))
#define rwb_Lights_Enable(lights, type, on) RA_CALL(FUN_001b5660, void (*)(void *, int, int))((lights), (type), (on))
#define rwb_Lights_Draw(lights) RA_CALL(FUN_001b5040, void (*)(void *))(lights)

/* ---------- rowing_b: the world (game +0x7c, 0x37c bytes) */
#define rwb_World_Ctor(self) RA_CALL(FUN_001b2c10, void *(*)(void *))(self)
#define rwb_World_Init(world, package, name) \
	RA_CALL(FUN_001b43c0, int (*)(void *, void *, const char *))((world), (package), (name))
#define rwb_World_Update(world) RA_CALL(FUN_001b31d0, void (*)(void *))(world)
#define rwb_World_DrawA(world, detail) RA_CALL(FUN_001b4720, void (*)(void *, int))((world), (detail))
#define rwb_World_DrawB(world, detail) RA_CALL(FUN_001b3230, void (*)(void *, int))((world), (detail))
#define rwb_World_DrawDetail(world) RA_CALL(FUN_001b3280, void (*)(void *))(world)
#define rwb_World_AddObject(objects, package, name, pos, rot) \
	RA_CALL(FUN_001b6a90, void *(*)(void *, void *, const char *, float *, float *)) \
	((objects), (package), (name), (pos), (rot))
/* the sound banks (cdecl) */
#define rwb_Sound_Init(package, genPackage) RA_CALL(FUN_001b6da0, void (*)(void *, void *))((package), (genPackage))

/* ---------- rowing_b: other objects */
#define rwb_GameMode_Ctor(self) RA_CALL(FUN_001b60c0, void *(*)(void *))(self)
#define rwb_Tokens_Init(tokens, package) RA_CALL(FUN_001b5730, void (*)(void *, void *))((tokens), (package))
#define rwb_Tokens_Draw(tokens) RA_CALL(FUN_001b5790, void (*)(void *))(tokens)
#define rwb_Tokens_DrawBlended(tokens) RA_CALL(FUN_001b57b0, void (*)(void *))(tokens)
#define rwb_Buoy_Draw(buoy, alpha) RA_CALL(FUN_001b5ea0, void (*)(void *, float))((buoy), (alpha))
#define rwb_Obj270_Ctor(self) RA_CALL(FUN_001b47b0, void *(*)(void *))(self)
#define rwb_Obj270_Init(obj, texture, count) RA_CALL(FUN_001b4c60, int (*)(void *, void *, int))((obj), (texture), (count))
#define rwb_Obj270_Draw(obj) RA_CALL(FUN_001b4800, void (*)(void *))(obj)
#define rwb_Sprites_DrawOpaque(sprites) RA_CALL(FUN_001b1430, void (*)(void *))(sprites)
#define rwb_Sprites_DrawBlended(sprites) RA_CALL(FUN_001b1490, void (*)(void *))(sprites)
#define rwb_Particles_Init(vents, sprites, steam, blobs) \
	RA_CALL(FUN_001b1c60, int (*)(void *, void *, int, int))((vents), (sprites), (steam), (blobs))
#define rwb_Particles_Draw(vents) RA_CALL(FUN_001b2b10, void (*)(void *))(vents)

/* ---------- other modules */

/* classlib_ui 0x00071f90 returns the package (the table says void) */
#define ra_AddPackage RA_CALL(FUN_00071f90, void *(*)(const char *, int))
/* foxhunt 0x00138790: the CWaterField constructor (`this` in ECX) */
#define ra_WaterField_Ctor(self) RA_CALL(FUN_00138790, void *(*)(void *))(self)
/* a CWaterField virtual function at byte offset OFF */
#define RA_WATER_VFN(w, off, type) RA_VFN(w, off, type)

#endif
