/*
ClassLib/classlib_ui_extern.c  (classlib_ui)

Tentative definitions of the globals of other modules that classlib_ui code uses
(docs/ARCHITECTURE.md section 12: -fcommon merges them with the owner's definition,
or with the owner's alias into its data block). The owners are named in
classlib_ui.h. When an owner has not defined a global yet, it stays zero here.
*/

#include "classlib_ui.h"

unsigned int DAT_00285630;       /* the engine's rand() seed, initial value 1 (owner: engine) */
char *DAT_002ba544;              /* core */
unsigned char DAT_002ba750[0x58]; /* core */
int DAT_00283d74;                /* core */
int DAT_00283e0c;                /* core */
int DAT_002ba928;                 /* core: screen width */
int DAT_002ba92c;                 /* core: screen height */
int DAT_002ba9ac;                 /* core */
int DAT_002ba9b0;                 /* core */
int DAT_002bae6c;                 /* core */
int DAT_002bae80;                 /* core */
int DAT_002bae84;                 /* core */
int DAT_003055e0;
unsigned int DAT_002bc490[256];   /* bdisplay */
unsigned int DAT_002bc898[256];   /* bdisplay */
unsigned int DAT_002bcc94[256];   /* bdisplay */
unsigned int DAT_002bce18[256];   /* bdisplay */
unsigned int DAT_002bd640[256];   /* bdisplay */
float DAT_002855f0[16];           /* engine: identity matrix (initialised data of its owner) */
float DAT_002be580;               /* bdisplay: near clip */
float DAT_002be584;               /* bdisplay: far clip */
float DAT_002be588;               /* bdisplay: field of view */
float DAT_002be58c;               /* bdisplay: aspect */
int DAT_002844cc;                 /* core: model shadow colour */
int DAT_002844d0;
int DAT_002844d4;
undefined4 DAT_00284b10;          /* bactor: actor tint */
undefined4 DAT_00284b14;
undefined4 DAT_00284b18;
int DAT_002bae7c;                 /* core */
int DAT_002bae94;                 /* core: player count */
int DAT_002baea8;                 /* core */
int DAT_002baed4[40];             /* core: player table (10 ints per player) */
int DAT_002baed8[40];             /* core: player table +4 */
int DAT_002baee0[40];             /* core: player table +0xc */
int DAT_002ba8b8[16];             /* core */
int DAT_002ba900;                 /* core */
float DAT_002ba904;               /* core */
unsigned short DAT_002ba7a8[0x88]; /* core */
int DAT_002ba99c;                 /* core */
int DAT_00284408;                 /* core */
int DAT_002db810;                 /* engine_misc */
int DAT_002bae68;                 /* core: effects volume */
int DAT_002baed0;                 /* core: game over */
int DAT_002baeb8;                 /* core: results shown */
int DAT_002b9c48[4];              /* core */
float DAT_002ed350[16];           /* per-player world positions (bumpercars / classlib_fx data) */
int DAT_002baebc;                 /* core: team game */
int DAT_002ba6a4;                 /* core */
int DAT_002b9c68;                 /* core */
int DAT_002bae7c;                 /* core: rumble enabled (options) */
int DAT_002ed340;                 /* classlib_fx: debris system */
int DAT_002ed344;
int DAT_002ed348;
int DAT_002ed34c;
int DAT_002ed44c;
unsigned char DAT_00290ea8[0x40]; /* classlib_fx: debris effect object */
