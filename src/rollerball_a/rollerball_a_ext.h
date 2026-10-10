/*
ROLLERBALL_A_EXT.H

Calls from rollerball_a into rollerball_b (and a few other modules) with the real shapes
read from the x86 code: `this` (ECX) first, floats as floats, pointers as pointers. The
table prototypes of these functions are Ghidra guesses (missing `this`, int for float or
pointer), so every call goes through RB_CALL. The names are ours; the owners may rename
the functions (FUN_<addr> stays valid as a macro).

Owner: rollerball_a. Only rollerball_a edits this file.
*/

#ifndef OPENFRENZY_ROLLERBALL_A_EXT_H
#define OPENFRENZY_ROLLERBALL_A_EXT_H

#include "rollerball_a.h"

/* ---------- rollerball_b: entity system (game +0x1428) and players */

/* 0x00192290 / 0x001922a0  entity system constructor / destructor */
#define rbb_EntitiesCtor(s) RB_CALL(FUN_00192290, void (*)(void *))(s)
#define rbb_EntitiesDtor(s) RB_CALL(FUN_001922a0, void (*)(void *))(s)
/* 0x001922b0  entity system shutdown */
#define rbb_EntitiesShutdown(s) RB_CALL(FUN_001922b0, void (*)(void *))(s)
/* 0x00192360  update every entity (virtual +8 with dt) */
#define rbb_EntitiesUpdate(s, dt) RB_CALL(FUN_00192360, void (*)(void *, float))(s, dt)
/* 0x00192390  draw */
#define rbb_EntitiesDraw(s) RB_CALL(FUN_00192390, void (*)(void *))(s)
/* 0x00192400  marks the computer players from core's player records */
#define rbb_EntitiesSetAI(s) RB_CALL(FUN_00192400, void (*)(void *))(s)
/* 0x00192430  sets a flag on every player (0x001999e0) */
#define rbb_EntitiesFreeze(s, on) RB_CALL(FUN_00192430, void (*)(void *, int))(s, on)
/* 0x00192350 */
#define rbb_Entities_192350(s) RB_CALL(FUN_00192350, int (*)(void *))(s)
/* 0x00192270  player: reset */
#define rbb_PlayerReset(p) RB_CALL(FUN_00192270, void (*)(void *))(p)
/* 0x00192480  player: place at a position (4 floats) */
#define rbb_PlayerPlace(p, pos) RB_CALL(FUN_00192480, void (*)(void *, const float *))(p, pos)
/* 0x001990e0  player: set +0x41a4 */
#define rbb_PlayerSet41a4(p, v) RB_CALL(FUN_001990e0, void (*)(void *, float))(p, v)

/* ---------- rollerball_b: world (game +0x4c) and camera (game +0x1368) */

#define rbb_WorldCtor(s) RB_CALL(FUN_00194a40, void (*)(void *))(s)
#define rbb_WorldDtor(s) RB_CALL(FUN_00194a50, void (*)(void *))(s)
#define rbb_WorldShutdown(s) RB_CALL(FUN_00194a60, void (*)(void *))(s)
#define rbb_WorldUpdate(s, dt) RB_CALL(FUN_00194bb0, void (*)(void *, float))(s, dt)
#define rbb_CameraCtor(s) RB_CALL(FUN_00192c40, void *(*)(void *))(s)
#define rbb_CameraDtor(s) RB_CALL(FUN_00192c60, void (*)(void *))(s)
#define rbb_CameraSetMode(s, m) RB_CALL(FUN_00192c80, void (*)(void *, int))(s, m)
#define rbb_CameraApply(s, f) RB_CALL(FUN_00192ca0, void (*)(void *, float))(s, f)
#define rbb_CameraDraw(s) RB_CALL(FUN_00192d70, void (*)(void *))(s)
#define rbb_CameraUpdate(s, dt) RB_CALL(FUN_00194970, void (*)(void *, float))(s, dt)

/* ---------- rollerball_b: goals (CRollerball_Goal) */

#define rbb_GoalsUpdate(s, dt) RB_CALL(FUN_00191bd0, void (*)(void *, float))(s, dt)

/* ---------- rollerball_b: the message queue (RB_Msg) and the shadow system (+0x29d4) */

#define rbb_MsgCtor(m) RB_CALL(FUN_00196a60, void (*)(struct RB_Msg *))(m)
#define rbb_MsgDtor(m) RB_CALL(FUN_00196a70, void (*)(struct RB_Msg *))(m)
#define rbb_PostMsg(p, m) RB_CALL(FUN_00196bf0, void (*)(void *, struct RB_Msg *))(p, m)
#define rbb_ShadowsShutdown(s) RB_CALL(FUN_00196910, void (*)(void *))(s)
#define rbb_ShadowsDraw1(s) RB_CALL(FUN_00196940, void (*)(void *))(s)
#define rbb_ShadowsDraw2(s) RB_CALL(FUN_00196a00, void (*)(void *))(s)
#define rbb_ShadowsUpdate(s, dt) RB_CALL(FUN_001967c0, void (*)(void *, float))(s, dt)
/* 0x001967f0  shadow constructor (object 0x24 bytes, flag, width, height, player) */
#define rbb_ShadowCtor(o, a, w, h, pl) \
	RB_CALL(FUN_001967f0, void *(*)(void *, int, int, int, void *))(o, a, w, h, pl)
/* 0x00196760  shadow: add a caster (an actor instance pointer field of the player) */
#define rbb_ShadowAddCaster(o, c) RB_CALL(FUN_00196760, void (*)(void *, void *))(o, c)
/* 0x001967a0  shadow system: add a shadow */
#define rbb_ShadowsAdd(s, o) RB_CALL(FUN_001967a0, void (*)(void *, void *))(s, o)
/* 0x00196500 / 0x00196560  variation scenery draw passes (ECX = the render object) */
#define rbb_DrawScenery1(s) RB_CALL(FUN_00196500, void (*)(void *))(s)
#define rbb_DrawScenery2(s) RB_CALL(FUN_00196560, void (*)(void *))(s)

/* ---------- rollerball_b: the sumo arena platforms (0xd0-byte objects) */

#define rbb_PlatformsCtor(s) RB_CALL(FUN_00196de0, void (*)(void *))(s)
#define rbb_PlatformsDtor(s) RB_CALL(FUN_00196df0, void (*)(void *))(s)
#define rbb_PlatformsShutdown(s) RB_CALL(FUN_00196e00, void (*)(void *))(s)
#define rbb_Platforms_196e80(s) RB_CALL(FUN_00196e80, void (*)(void *))(s)
#define rbb_PlatformCtor(o) RB_CALL(FUN_001973c0, void *(*)(void *))(o)
#define rbb_Platform_1973f0(o) RB_CALL(FUN_001973f0, void (*)(void *))(o)
#define rbb_PlatformReset(o) RB_CALL(FUN_001974f0, void (*)(void *))(o)
#define rbb_PlatformSet48(o, v) RB_CALL(FUN_00197540, void (*)(void *, float))(o, v)
#define rbb_PlatformSetTime(o, v) RB_CALL(FUN_00197550, void (*)(void *, float))(o, v)
#define rbb_PlatformSet74(o, v) RB_CALL(FUN_00197560, void (*)(void *, int))(o, v)
#define rbb_PlatformSet50(o, v) RB_CALL(FUN_00197570, void (*)(void *, int))(o, v)
#define rbb_Platform_197580(o) RB_CALL(FUN_00197580, void (*)(void *))(o)
#define rbb_PlatformInit(o, pos, v) RB_CALL(FUN_001975b0, int (*)(void *, const float *, int))(o, pos, v)
#define rbb_PlatformDelete(o) RB_CALL(FUN_001977d0, void (*)(void *))(o)
#define rbb_Platform_197f40(o) RB_CALL(FUN_00197f40, void (*)(void *))(o)
#define rbb_Platform_197fa0(o, v) RB_CALL(FUN_00197fa0, int (*)(void *, float))(o, v)
/* 0x00196f70 / 0x00196fc0 / 0x00197150  sumo ring */
#define rbb_Ring_196f70(o, a, b, c, d) \
	RB_CALL(FUN_00196f70, void (*)(void *, const float *, float, int, int))(o, a, b, c, d)
#define rbb_RingInit(o, pos, r, a, b) \
	RB_CALL(FUN_00196fc0, int (*)(void *, const float *, float, int, int))(o, pos, r, a, b)
#define rbb_RingSet(o, a, b) RB_CALL(FUN_00197150, void (*)(void *, float, float))(o, a, b)

/* ---------- rollerball_b: the game mode (0x28 bytes) */

#define rbb_GameModeCtor(o) RB_CALL(FUN_00198600, void *(*)(void *))(o)

/* ---------- rollerball_b: paths read from ASE files (race variations) */

/* 0x001980f0 / 0x00198110  path record (0x30 bytes) constructor / destructor (0x00198110 is
missing from the table: rollerball_b defines it) */
void FUN_00198110(void *self);
#define rbb_PathCtor(o) RB_CALL(FUN_001980f0, void (*)(void *))(o)
#define rbb_PathDtor(o) RB_CALL(FUN_00198110, void (*)(void *))(o)
#define rbb_PathSetPoints(o, pts, n) RB_CALL(FUN_00198130, void (*)(void *, float *, int))(o, pts, n)
#define rbb_Path_1981e0(o, pos) RB_CALL(FUN_001981e0, unsigned int (*)(void *, const float *))(o, pos)
#define rbb_SideOfLine(a, b) RB_CALL(FUN_00198340, float (*)(const float *, const float *))(a, b)
#define rbb_PathNear(o, pos, r, out) \
	RB_CALL(FUN_001983a0, int (*)(void *, const float *, float, float *))(o, pos, r, out)
/* 0x001986e0 / 0x00198700  ASE shape list constructor / destructor */
#define rbb_AseShapesCtor(o) RB_CALL(FUN_001986e0, void (*)(void *))(o)
#define rbb_AseShapesDtor(o) RB_CALL(FUN_00198700, void (*)(void *))(o)
#define rbb_AseReadShape(o, rd) RB_CALL(FUN_00198730, void (*)(void *, void *))(o, rd)
#define rbb_AseReadNamedShape(o, rd, name) \
	RB_CALL(FUN_00198840, void (*)(void *, void *, const char *))(o, rd, name)
/* 0x001989a0 / 0x001989b0 / 0x001989c0  ASE text reader constructor / destructor / open */
#define rbb_AseReaderCtor(o) RB_CALL(FUN_001989a0, void (*)(void *))(o)
#define rbb_AseReaderDtor(o) RB_CALL(FUN_001989b0, void (*)(void *))(o)
#define rbb_AseReaderOpen(o, data, size) RB_CALL(FUN_001989c0, void (*)(void *, void *, int))(o, data, size)

/* ---------- rollerball_b: particles */

#define rbb_Particle_19d0c0(o, n) RB_CALL(FUN_0019d0c0, void (*)(void *, int))(o, n)
#define rbb_Particle_19d110(o, v) RB_CALL(FUN_0019d110, void (*)(void *, int))(o, v)
#define rbb_Particle_19d120(o) RB_CALL(FUN_0019d120, void (*)(void *))(o)
#define rbb_Particle_19d150(o, a, n) RB_CALL(FUN_0019d150, int (*)(void *, int *, int))(o, a, n)
#define rbb_ParticlesCtor(o) RB_CALL(FUN_0019a4e0, void *(*)(void *))(o)
#define rbb_ParticlesDtor(o) RB_CALL(FUN_0019ce50, void (*)(void *))(o)

#endif
