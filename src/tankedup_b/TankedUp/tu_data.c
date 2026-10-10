/*
TankedUp/tu_data.c - the TankedUp globals of tankedup_b (see tankedup_b_data.h).
Only the globals outside tankedup_a's block (0x00312f50-0x003153a0) are defined here; that
block (CTank_Game and the CTank_Debug object at 0x003150b0) is defined by tankedup_a, which
exports the DAT_ names this module uses as aliases into it.
*/

#include "tankedup_b.h"
#include "tankedup_b_data.h"

/* 0x002b74a4  "grbyop" (.data) */
char s_grbyop_002b74a4[8] = "grbyop";

struct TankVertex DAT_003153a0[4];  /* 0x003153a0 */
struct TankVertex DAT_00315430[4];  /* 0x00315430 world mesh debug triangle */
struct TankVertex DAT_003154c0[34]; /* 0x003154c0 light cone fan */
void *DAT_00315988;                 /* 0x00315988 the AI spawn list (Token Master) */
int DAT_0031598c;                   /* 0x0031598c its length */
unsigned int DAT_00315990;          /* 0x00315990 bit 0: the list is set up (static guard) */
struct TankVertex DAT_00315998[4];  /* 0x00315998 track decal quad */
struct TankVertex DAT_00315a28[3];  /* 0x00315a28 track decal joint triangle */
