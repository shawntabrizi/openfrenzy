/*
CLUSTEROIDS_GAME.H

Internal names of the clusteroids module: the globals of the game (by original address,
inside the blocks of clusteroids_data.h) and the layout of the player records.
The prototypes of the module's functions are in clusteroids.decls (of_decls.h).

Owner: clusteroids.
*/

#ifndef OPENFRENZY_CLUSTEROIDS_GAME_H
#define OPENFRENZY_CLUSTEROIDS_GAME_H

#include "clusteroids.h"

/* ---------- game state */

#define g_ClVariant CL_VAR(short, 0x002f7f6e)          /* 1 Tails, 2 Pod Hog, 3 Mines, 4 Collector */
#define g_ClPackage CL_VAR(int, 0x002f8f68)            /* "Packages\Cluster" */
#define g_ClVariantPackage CL_VAR(int, 0x002f8f6c)     /* "packages\clust-v4" (Collector) */
#define g_ClPhase CL_VAR(short, 0x002fa278)            /* 1 intro, 2 playing, 3 finished */
#define g_ClGameOver CL_VAR(short, 0x002f9a70)         /* the mode pops on the next update */
#define g_ClTweaks CL_VAR(struct CTweakMenu *, 0x002f7a48)
#define g_ClColours CL_VAR(struct CTweakMenu *, 0x002f7a44)
#define g_ClFrameTime CL_VAR(float, 0x002fae9c)        /* frame time, 1.0 = 1/60 s */
#define g_ClCamera CL_ADDR(0x00296ee4)                 /* the camera object (vtable 0x00269648) */

/* ---------- functions missing from of_functions.tsv (tail-jump targets Ghidra did not make
   functions; reported to the architect) */

/* 0x000debb0: prints the profile pass times (the tail of ClustGame_Draw jumps here) */
void FUN_000debb0(void);
/* 0x000df730 (not defined separately): frees the debug input channels/map and the players'
   input maps; CGameModeClusteroids_Suspend tail-jumps here and holds its code inline */

/* ---------- the four players: records of 0x850 bytes from 0x002fcb20 */

#define CL_PLAYER_BASE 0x002fcb20u
#define CL_PLAYER_SIZE 0x850
#define CL_PLAYER(i) ((char *)CL_ADDR(CL_PLAYER_BASE) + (i) * CL_PLAYER_SIZE)

#define PL_OUT 0x000          /* short: out of the game */
#define PL_SCORE 0x002        /* short */
#define PL_X 0x018            /* float position x */
#define PL_Z 0x01c            /* float position z */
#define PL_WIND 0x020         /* float[4]: the debris wind source (x, 0, z, 0) */
#define PL_SHIP 0x070         /* BActorInstance (0x170): the ship (intro and end) */
#define PL_SHIP2 0x1e0        /* BActorInstance (0x170): the ship while playing */
#define PL_CHAR 0x350         /* BActorInstance (0x170): the character */
#define PL_CHAR2 0x4c0        /* BActorInstance (0x170): the character's second actor */
#define PL_CONTROL 0x630      /* short: 1 human, 2 computer after a pause-menu change */
#define PL_PODS 0x674         /* the first pod of the player's chain (linked by pod +0x00) */
#define PL_TARGET_POD 0x678   /* the pod the AI goes for */
#define PL_ANIMS 0x688        /* struct BAnim *[19]: the character animations */
#define PL_IN_MAP 0x7f4       /* struct biMap * */
#define PL_IN_STICK_Y 0x7f8   /* struct biChannel * "p1 joy y" */
#define PL_IN_STICK_X 0x7fc   /* "p1 joy x" */
#define PL_IN_FIRE 0x800      /* "Fire" */
#define PL_IN_BOOST 0x804     /* "Boost" */

#endif
