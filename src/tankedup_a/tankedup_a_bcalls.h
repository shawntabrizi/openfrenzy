/*
TANKEDUP_A_BCALLS.H

Calls from tankedup_a into tankedup_b (0x001cee60-0x001de950) and into other modules whose
table prototype hides `this`, a float or a result. Each macro casts the function to its real
shape, read from the binary (ECX `this`, stack arguments, `ret N`, float use), so a call
stays right whatever prototype the owner gives the function in its .decls file.

The names describe what the function does for TankedUp; the owner may name it differently.
Owner: tankedup_a.
*/

#ifndef OPENFRENZY_TANKEDUP_A_BCALLS_H
#define OPENFRENZY_TANKEDUP_A_BCALLS_H

/* ---------- tankedup_b: the game mode and the text reader */

/* 0x001d7b00  CGameMode_TankedUp constructor (0x2c bytes) */
#define TUB_GameMode_Ctor ((void *(*)(void *))FUN_001d7b00)
/* 0x001d7c70 / 0x001d7c80  the TankedUp text reader's constructor and destructor (vtable 0x0027bca8) */
#define TUB_Reader_Ctor ((void (*)(void *))FUN_001d7c70)
#define TUB_Reader_Dtor ((void (*)(void *))FUN_001d7c80)
/* 0x001d7c90  reads the next line into `line`; returns it without leading white space */
#define TUB_Reader_ReadLine ((char *(*)(void *, char *))FUN_001d7c90)

/* ---------- tankedup_b: the debug object (0x003150b0) */

#define TUB_Debug_Initialise ((int (*)(void *))FUN_001d6d90)
#define TUB_Debug_Free ((void (*)(void *))FUN_001d79d0)
#define TUB_Debug_ReleaseInput ((void (*)(void *))FUN_001d6d40)
/* 0x001d6820 / 0x001d6860  render states for the floor decals (no `this`) */
#define TUB_Decal_BeginStates ((void (*)(void))FUN_001d6820)
#define TUB_Decal_StencilOn ((void (*)(void))FUN_001d6860)
/* 0x001d6d80  the debug draw switches: 1 hides the world (shows the object meshes), 2 shows
   the boundaries and checkpoints */
#define TUB_Debug_GetDrawFlags ((int (*)(void *))FUN_001d6d80)
/* 0x001d6680  (no `this`) draws a collision mesh in a colour (debug) */
#define TUB_Debug_DrawMesh ((void (*)(void *, unsigned int, int))FUN_001d6680)
/* 0x001d6750  (no `this`) sets an animated texture's time without looping */
#define TUB_Texture_SetTimeOnce ((void (*)(void *, int))FUN_001d6750)
/* 0x001d67c0  (no `this`) 1 when the debug "single player" option is on in a one-minigame
   game (0x00283ef0, g_GameType 1); *player (may be NULL) gets the first present player */
#define TUB_Debug_SinglePlayer ((int (*)(int *))FUN_001d67c0)
/* 0x001d6960  draws a light's debug cone: (pos, target, angle, r, g, b) */
#define TUB_Debug_DrawCone ((void (*)(float *, float *, float, int, int, int))FUN_001d6960)

/* ---------- tankedup_b: the sound system (CTank_Game +0xa34) */

#define TUB_Sound_Ctor ((void *(*)(void *))FUN_001d5880)
#define TUB_Sound_Initialise ((int (*)(void *))FUN_001d5960)
#define TUB_Sound_Free ((void (*)(void *))FUN_001d5a10)
/* 0x001d5a50  plays sound `kind` at `pos`; returns the channel */
#define TUB_Sound_Play ((int (*)(void *, int, float *, int, int, int))FUN_001d5a50)

/* ---------- tankedup_b: the entity system (CTank_Game +0xb20) */

/* (package, entity file, world, route, spawn, debris, lights, sound, boom) */
#define TUB_Entities_Initialise \
	((int (*)(void *, void *, char *, void *, void *, void *, void *, void *, void *, void *))FUN_001d41d0)
#define TUB_Entities_Free ((void (*)(void *))FUN_001d4730)
#define TUB_Entities_Update ((int (*)(void *, float, int))FUN_001d5790)
#define TUB_Entities_StateChange ((void (*)(void *, int, int))FUN_001d3e90)
#define TUB_Entities_Draw ((void (*)(void *))FUN_001d0920)
#define TUB_Entities_DrawLate ((void (*)(void *))FUN_001d14e0)
/* 0x001d1d10  the position of entity `id` */
#define TUB_Entities_GetPosition ((void (*)(void *, int, float *))FUN_001d1d10)
/* 0x001d1c30  entity `id`: position (raised by 60 over its clearance), velocity, rotation,
   its clearance and another word; returns 0 when there is no such entity */
#define TUB_Entities_GetInfo \
	((int (*)(void *, int, float *, float *, float *, void *, void *))FUN_001d1c30)
/* 0x001d1e80 / 0x001d20e0 / 0x001d2340  drive tank `id` (three control schemes) */
#define TUB_Entities_DriveTracks ((void (*)(void *, int, float, float))FUN_001d1e80)
#define TUB_Entities_DriveStick ((void (*)(void *, int, float, float))FUN_001d20e0)
#define TUB_Entities_DriveCamera ((void (*)(void *, int, float, float))FUN_001d2340)
/* 0x001d4cc0  tank `id` fires (primary button); 0x001d2c80  tank `id` reverses (secondary) */
#define TUB_Entities_FirePrimary ((int (*)(void *, int))FUN_001d4cc0)
#define TUB_Entities_Reverse ((void (*)(void *, int))FUN_001d2c80)
/* 0x001d62d0  (no `this`) ranks four scores: out[i] = the place (1-4) of in[i] */
#define TUB_RankScores ((void (*)(int *, int *))FUN_001d62d0)
/* 0x001d1770  the centre of the active tanks (and two more points the camera ignores) */
#define TUB_Entities_GetCentre ((void (*)(void *, float *, float *, float *))FUN_001d1770)

/* ---------- tankedup_b: the character system (CTank_Game +0xfd0) */

#define TUB_Chars_Ctor ((void *(*)(void *))FUN_001d5af0)
#define TUB_Chars_Initialise ((int (*)(void *, void *, void *))FUN_001d5b80)
#define TUB_Chars_Free ((void (*)(void *))FUN_001d5da0)
#define TUB_Chars_Update ((int (*)(void *, float, int))FUN_001d5ee0)
#define TUB_Chars_StateChange ((void (*)(void *, int, int))FUN_001d5de0)
#define TUB_Chars_Draw ((void (*)(void *))FUN_001d6090)

/* ---------- other modules: functions whose table prototype hides `this` or a result */

/* 0x000b0770 (charlib)  an empty __thiscall (this, a, b) method; TankedUp's systems that
   ignore a state change point at it */
#define TU_EmptyStateChange(obj, from, to) FUN_000b0770((void *)(obj), (from), (to))
/* 0x00096600 (bumpercars_b)  folded getter: returns the float at +0x3c (TuTimer.elapsed) */
#define TU_Timer_GetElapsed(t) ((float (*)(void *))FUN_00096600)((void *)(t))
/* 0x000772e0 (classlib_ui)  the pause menu's update (__thiscall on 0x002ecd90) */
#define TU_PauseMenu_Update() ((int (*)(void *))FUN_000772e0)((void *)DAT_002ecd90)

#endif
