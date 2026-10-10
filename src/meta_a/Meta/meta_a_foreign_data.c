/*
META_A_FOREIGN_DATA.C

Fallback data that needs a hand-written initialiser (pointers). This file does not
include meta_a_globals.h, whose declarations use other types.
*/

/* 0x00290ea8  (classlib_fx data) a CSpriteVB whose batch list is empty: next and prev
   point at the object itself. Weak fallback until classlib_fx defines it. */
__attribute__((weak, aligned(4))) unsigned int DAT_00290ea8[15] = {
	(unsigned int)DAT_00290ea8, (unsigned int)DAT_00290ea8,
};
