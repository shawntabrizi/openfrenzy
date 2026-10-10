/*
CARRYON_A_EXT.H

What carryon_a uses from other modules: their globals (CarryOn/carryon_a_foreign.c holds
weak fallbacks with the XBE values, an owner's definition replaces them) and call aliases
with the real shape for functions whose table prototype is still Ghidra's guess.
`this` (ECX) is the first parameter of every alias.
*/

#ifndef CARRYON_A_EXT_H
#define CARRYON_A_EXT_H

/* ---------- globals of other modules */

/* core */
extern int DAT_002bae70;                /* difficulty of the CPU players */
extern int DAT_002bae94;                /* number of players */
extern int DAT_002bae98;                /* game type (2: tournament) */
extern int DAT_002baeb8;                /* demo mode: CPU players only */
extern int DAT_002bae7c;                /* rumble on */
extern int DAT_002bae6c;                /* the sound effect volume */
extern int DAT_002baebc;                /* team game */
extern int DAT_002baed4[4][10];         /* players: [0] character, [1] human, [8] team */
extern int DAT_002baee8[];              /* = &DAT_002baed4[0][5]: score, stride 10 */
extern int DAT_002baef4[];              /* = &DAT_002baed4[0][8]: team, stride 10 */
extern int DAT_002baf10, DAT_002baf38, DAT_002baf60;  /* scores of players 2..4 */
extern int DAT_002ecdec;                /* the game is paused */
extern unsigned char DAT_002ba750[];    /* the game mode stack */
extern void **DAT_002ba95c;             /* the tournament (vtable +0xc: next game) */
extern unsigned char DAT_002ecd90[];    /* the pause menu (classlib_ui 0x000772e0) */
extern unsigned int DAT_00285630;       /* rand() seed */
extern float DAT_002855f0[16];          /* the identity matrix */
extern float DAT_00285634[4];           /* the identity quaternion */
extern unsigned int DAT_002bc490[256], DAT_002bd640[256], DAT_002bc898[256], DAT_002bce18[256];
                                        /* bdisplay colour channel tables */
extern unsigned int DAT_002bca94;       /* bdisplay: opaque alpha */
extern float DAT_0028fda8;              /* frame time scale (1.0) */
extern float DAT_0028ffa8;              /* the display's near plane */
extern float DAT_0028ffac;              /* the display's far plane */
extern int DAT_00283e58;                /* slow motion debug switch */
extern int DAT_00283ea4;                /* character colour set */
extern long long DAT_002cd7a0;          /* timer ticks per second */
extern char s_BGeffects_00283b18[];     /* "BGeffects": the mode to return to */

/* core_modes_b: the score table and its ranking */
extern unsigned char DAT_002b9968[];    /* the score table (CScoreTable) */
extern int DAT_002b9c58[4];             /* players by rank */
extern int DAT_002b9c68[4];             /* number of players at each rank */

/* charlib (route grid and AI players) */
extern int DAT_00292c4c;                /* route grid columns */
extern int DAT_00292c50;                /* route grid rows */
extern float DAT_00292c60;              /* route grid x */
extern float DAT_00292c64;              /* route grid z */
extern float DAT_00292c68;              /* route grid width */
extern float DAT_00292c6c;              /* route grid depth */
extern int DAT_00292c78;                /* route grid cells (columns x rows) */
extern unsigned char DAT_00292ca0[];    /* the actor animation names (0x44 bytes each) */
extern int DAT_00293928;                /* AI voice volume */
extern float DAT_00293a38;              /* punch reach scale while holding (1.5) */
extern float DAT_002ea478[4];           /* a zero vector */
extern void *DAT_002f289c[4];           /* the CharLib AI players */
extern void *DAT_002f2878[6];
extern void *DAT_002f27d8;              /* the route tables in use */
extern void *DAT_002ea488;              /* the route context in use */
extern void *DAT_002f26c4;              /* bumpercars_b object (the demo camera?) */
extern void *DAT_002f27c4;              /* bumpercars_b object (0x000a74d0) */
extern float DAT_002f7a2c;              /* CCamera.cpp: the camera heading */
extern float DAT_00294b64;              /* CCamera.cpp: camera height */
extern float DAT_00294b98[4];           /* CCamera.cpp: eye start */
extern float DAT_00294ba8[4];           /* CCamera.cpp: pivot start */
extern float DAT_00294bb8;              /* CCamera.cpp: camera pitch */

/* classlib_ui */
extern void *DAT_002ecfac;              /* the score HUD */
extern unsigned char DAT_002ed324[];    /* the rumble manager */
extern int DAT_002ba56c;                /* bumpercars_b switch: the demo camera exists */
extern unsigned int DAT_002bc48c;       /* bdisplay: line colour index */

/* the player colours (classlib_fx/core table, 0x60 bytes per character: r, g, b first) */
extern unsigned char DAT_00290718[];

/* ---------- call aliases (real shapes) */

/* classlib_ui: CLLoading_AddPackage returns the opened package */
#define co_LoadPackage ((void *(*)(const char *, int))FUN_00071f90)

/* charlib */
#define co_SetShadowLight ((void (*)(int, float, float, float))FUN_000be6d0)
#define co_CharLibInitShadows ((void (*)(void))FUN_000b9bf0)

/* rollerball_b: CoTextReader set-up on a buffer */
#define co_TextReader_Init ((void (*)(struct CoTextReader *, void *, unsigned int))FUN_001989c0)

/* rowing_a: the intro camera of the minigames */
#define co_IntroCam_Init ((int (*)(void))FUN_0019d5c0)
#define co_IntroCam_Active ((int (*)(void))FUN_0019d630)
#define co_IntroCam_GetPos ((void (*)(float *))FUN_0019d7d0)
#define co_IntroCam_Delete ((void (*)(void))FUN_0019d810)
#define co_IntroCam_Update ((void (*)(void))FUN_0019d870)
#define co_Profiler ((void (*)(int, int))FUN_001af540)

/* bumpercars_b */
#define co_DemoCam_Ctor ((void *(*)(void *, int, float, int))FUN_000a5ca0)
#define co_DemoCam_Dtor ((void (*)(void *))FUN_000a5c80)
#define co_DemoCam_Update ((void (*)(void *, float))FUN_000a5fa0)

/* bumpercars_a */
#define co_Debris_SetPlayers ((void (*)(float *, float *, float *, float *))FUN_00082b20)

/* empty functions (`ret`): a no-op virtual method, a no-op destructor */
#define co_Nothing ((void (*)(void))FUN_001d5950)

/* carryon_b */
#define co_Mode_Ctor ((void *(*)(void *))FUN_000d2290)
#define co_Fireworks_Init ((void (*)(void *, int))FUN_000cf200)
#define co_Fireworks_Delete ((void (*)(void *))FUN_000d0360)
#define co_Fireworks_Update ((void (*)(void *))FUN_000d0f40)
#define co_Spawner_Ctor ((void *(*)(void *))FUN_000ce110)
#define co_Spawner_Dtor ((void (*)(void *))FUN_000ce130)
#define co_Spawner_Init ((void (*)(void *, int, void *))FUN_000ce140)
#define co_Spawner_AddPoint ((void (*)(void *, void *, int))FUN_000ce3a0)
#define co_Spawner_AddPad ((void (*)(void *, void *, int))FUN_000ce420)
#define co_Spawner_Delete ((void (*)(void *))FUN_000ceea0)
#define co_Spawner_Update ((void (*)(void *, float, int))FUN_000ce7a0)
#define co_Launcher_Ctor ((void (*)(void *))FUN_000ca730)
#define co_Launcher_Init ((int (*)(void *, void *, float *, float, float, int))FUN_000ca740)
#define co_Launcher_Delete ((void (*)(void *))FUN_000ca830)
#define co_Launcher_Update ((void (*)(void *, float, void *))FUN_000cb150)
#define co_Launcher_Draw ((void (*)(void *))FUN_000cacf0)
#define co_Glitter_Ctor ((void *(*)(void *))FUN_000cbe30)
#define co_Glitter_Start ((void (*)(void *))FUN_000ccb50)
#define co_Glitter_Update ((void (*)(void *, float))FUN_000cce70)
#define co_Glitter_Delete ((void (*)(void *))FUN_000cd4a0)
#define co_Paint_Init ((void (*)(void *, float *, float, void *, int, void *, int))FUN_000ca230)
#define co_Paint_Update ((void (*)(void *, float, void *, void *, void *, int))FUN_000ca3f0)
#define co_Paint_Delete ((void (*)(void *))FUN_000ca690)
#define co_Sky_Ctor ((void *(*)(void *))FUN_000d18a0)
#define co_Sky_Delete ((void (*)(void *))FUN_000d0fb0)
#define co_Sky_Update ((void (*)(void *, float))FUN_000d15c0)
#define co_Sky_CreateLight ((void *(*)(void *, int, void *, int, int, int, int))FUN_000d10d0)
#define co_Obj2c_Delete ((void (*)(void *))FUN_000d1dd0)
#define co_Obj2c_Update ((void (*)(void *, float))FUN_000d19f0)
#define co_Timer_Start ((void (*)(void *, unsigned int))FUN_000d1fc0)
#define co_Timer_Stop ((void (*)(void *, int))FUN_000d1f50)
#define co_Timer_Draw ((void (*)(void *))FUN_000d21a0)
#define co_Sky_SetLight ((int (*)(void *, void *, unsigned int, unsigned int))FUN_000d1410)
#define co_Sky_EnableLights ((void (*)(void *, int, int))FUN_000d19b0)
#define co_Obj2c_Draw ((void (*)(void *))FUN_000d1a80)
#define co_Glitter_Draw ((void (*)(void *, int))FUN_000cd370)
#define co_Glitter_DrawEffects ((void (*)(void *))FUN_000cbe50)
#define co_Glitter_DrawHud ((void (*)(void *))FUN_000cbf20)
#define co_Spawner_Draw ((void (*)(void *, unsigned int))FUN_000ce490)
#define co_Spawner_DrawEffects ((void (*)(void *))FUN_000cef40)
#define co_Paint_Draw ((void (*)(void *))FUN_000ca470)
#define co_Fireworks_Draw ((void (*)(void *))FUN_000d0450)
#define co_Spawner_FindPad ((void *(*)(void *, float *, int))FUN_000cece0)
#define co_Spawner_HasLoad ((int (*)(void *, int))FUN_000cec70)
#define co_Spawner_FindType ((void *(*)(void *, int))FUN_000cee20)
#define co_Spawner_FindAny ((void *(*)(void *))FUN_000cedb0)
#define co_Pad_Use ((int (*)(void *, int))FUN_000cbb80)
#define co_Station_Deliver ((void (*)(void *))FUN_000cd720)
#define co_Glitter_FindPickups ((int (*)(void *, float *, void *, void **))FUN_000cc520)
#define co_Glitter_AtJar ((int (*)(void *, float *))FUN_000cc510)
#define co_Glitter_Drop ((void (*)(void *, int, float *, int))FUN_000cc090)
#define co_Rack_NearestPoint ((void (*)(void *, float *, float *))FUN_000cab20)
#define co_Rack_AddRocket ((int (*)(void *, int))FUN_000cb050)
#define co_Rack_HasRoom ((int (*)(void *, int))FUN_000cb0c0)
#define co_Conveyor_FindNearest ((void *(*)(void *, float *, int))FUN_000ca4c0)
#define co_Sparks_Stream ((void (*)(void *, float *, float *, int, int, int))FUN_000d4600)

/* rowing_a: the frame timer's mark (the timer object is the resource's +0x18) */
#define co_Timer_Mark ((void (*)(void *, int))FUN_001af540)

/* bumpercars_a: the settings (command line / ini) */
extern void *DAT_002ba9a0;
/* carryon_b: the frame timer (at +0x18 of the resource object) */
#define co_Timer_Ctor ((void (*)(void *))FUN_000d1e10)
#define co_Timer_Dtor ((void (*)(void *))FUN_000d1e50)
#define co_Timer_Init ((void (*)(void *))FUN_000d1e60)
#define co_Timer_Frame ((void (*)(void *))FUN_000d1eb0)

/* classlib_ui */
#define co_Rumble_SetMotors ((int (*)(void *, int, int, int, int))FUN_00080630)
#define co_Hud_SetCountdown ((void (*)(void *, float))FUN_00078930)
#define co_Hud_CountdownDone ((int (*)(void *))FUN_0007a900)
#define co_Hud_ShowWinner ((int (*)(void *, int *, int))FUN_0007ad50)
#define co_Hud_Draw ((void (*)(void *))FUN_0007bd80)
#define co_Hud_DrawPlayerMarker ((void (*)(void *, int, float *, float *))FUN_0007b970)
#define co_Overlays_SetList ((void (*)(void *))FUN_00072720)
#define co_PauseMenu_Update ((int (*)(void *))FUN_000772e0)

/* bactor: baUpdateExtras takes the frame time */
#define co_UpdateExtras ((void (*)(void *, float))FUN_0003c620)

/* bumpercars_b: an extras system holder (0x90 bytes, [0] the extras, +4 a yaw) */
#define co_ExtraFx_Init ((void (*)(void *, int))FUN_000a74d0)

/* charlib: CharLibShadow and other per-frame jobs */
#define co_Shadows_Render ((void (*)(void **))FUN_000bef00)
#define co_Shadows_End ((void (*)(void))FUN_000beed0)
#define co_CharLib_UpdateShadows ((void (*)(void))FUN_000be9a0)
#define co_CharLib_UpdateFx ((void (*)(void))FUN_000b9ae0)
#define co_CharLib_DeleteFx ((void (*)(void))FUN_000ba430)
#define co_CharLib_DeleteShadows ((void (*)(void))FUN_000b9b80)
#define co_RouteItem_Update ((void *(*)(void *))FUN_000ba470)

#endif
