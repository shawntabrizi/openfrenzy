/*
FoxHunt/fh_bss.c  (module foxhunt)

The Fox Hunt .bss, 0x00302138-0x00302340, in its original layout (see foxhunt.h, FH_ADDR).
The objects that core's static constructors build (core/crt/static_init.c) and the two words
classlib_fx shares (0x00302210 the players, 0x003022c8 the speech object) are tentative
definitions here too; -fcommon merges them with the other modules' definitions.
*/

#include "foxhunt.h"

unsigned char fh_bss0[0x003021ac - 0x00302138] __attribute__((aligned(16)));
unsigned char DAT_003021ac[0xc] __attribute__((aligned(4)));
unsigned char DAT_003021b8[0x48] __attribute__((aligned(4)));
unsigned char fh_bss1[0x00302210 - 0x00302200] __attribute__((aligned(16)));
unsigned char DAT_00302210[0x10] __attribute__((aligned(16)));
unsigned char fh_bss2[0x003022bc - 0x00302220] __attribute__((aligned(16)));
unsigned char DAT_003022bc[0xc] __attribute__((aligned(4)));
unsigned char DAT_003022c8[0x4] __attribute__((aligned(4)));
/* 0x003022cc-0x00302300 is one block: the effects object at 0x003022cc is {sparks, lightning}
(0x0013b910 writes +4 = 0x003022d0), so fh_bss3 is DAT_003022cc + 4 (foxhunt.h). Core's
tentative 4-byte definition merges into this one (integration round 2: Tag shark list fault). */
unsigned char DAT_003022cc[0x00302300 - 0x003022cc] __attribute__((aligned(4)));
unsigned char DAT_00302300[0xc] __attribute__((aligned(4)));
unsigned char DAT_0030230c[0xc] __attribute__((aligned(4)));
unsigned char DAT_00302318[0xc] __attribute__((aligned(4)));
unsigned char DAT_00302324[0xc] __attribute__((aligned(4)));
/* 0x00302330 is the gate list that core's static constructor initialises by this name
(integration round 2: Tag variation 2 gate fault); fh_bss4 is this object (foxhunt.h). */
unsigned char DAT_00302330[0x00302340 - 0x00302330] __attribute__((aligned(16)));
