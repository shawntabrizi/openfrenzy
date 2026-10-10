/*
BACTOR_SHARED.C

Tentative (common) definitions of the other modules' globals that BACTOR
uses (see bactor_extern.h). They merge with the owners' definitions
(-fcommon); until an owner defines one, this copy keeps the image linking.
They hold no BACTOR data.
*/

#include "bactor.h"
#include "bactor_extern.h"

/* bdisplay (src/bdisplay/bdisplay_globals.h) */
struct bdVertexBuffer *DAT_00284c10;
float *PTR_DAT_00284c70;
float DAT_002be330[16];
float DAT_002be370[16];
float DAT_002be3b0[16];
float DAT_002be430[16];
float DAT_002be470[16];
float DAT_002be570[3];
float DAT_002be580;
float DAT_002be584;
float DAT_002be588;
float DAT_002be58c;
float DAT_002be590;
float DAT_002be594;
int DAT_002be5c8;
float DAT_002be5e8;
float DAT_002be5ec;
uint32_t DAT_002be600[15 * 2];
int DAT_002bee34[4];
int DAT_002bee44[15];
int DAT_002bee80;
int DAT_002bee84;
int DAT_002bc3ec;
uint32_t DAT_002bc400;
int DAT_002bc424;
int DAT_002bc890;
unsigned int DAT_002bc490[256];
unsigned int DAT_002bc898[256];
unsigned int DAT_002bce18[256];
unsigned int DAT_002bd640[256];
float DAT_002bd220[256];

/* bkernel (src/bkernel/bkernel_core.h) */
int DAT_002cc4f0;
