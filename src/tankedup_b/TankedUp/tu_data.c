/*
TankedUp/tu_data.c - the TankedUp globals of tankedup_b (see tankedup_b_data.h).
The .bss ones are tentative definitions (no initialiser) so that tankedup_a can define the
same names (merged by -fcommon).
*/

#include "tankedup_b.h"
#include "tankedup_b_data.h"

/* 0x002b74a4  "grbyop" (.data) */
char s_grbyop_002b74a4[8] = "grbyop";

int DAT_003150c0;                   /* 0x003150c0 */
float DAT_003150b4;                 /* 0x003150b4 */
float DAT_00315158;                 /* 0x00315158 */
float DAT_00315178;                 /* 0x00315178 */
int DAT_0031519c;                   /* 0x0031519c */
int DAT_003151a4;                   /* 0x003151a4 */
void *DAT_003151b4;                 /* 0x003151b4 */
struct TankVertex DAT_003153a0[4];  /* 0x003153a0 */
