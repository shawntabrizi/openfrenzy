/*
TankedUp/tu_helpers.c  (module tankedup_a; not an original source file)

Helpers that stand in for C library code the original calls.
*/

#include "tankedup_a.h"

/* the C library's per-thread rand() seed (the MSVC tiddata holdrand starts at 1) */
static unsigned int tu_holdrand = 1;

/* MSVC rand() (libc _rand at 0x001e133e) on TankedUp's own seed. musl rand() gives another
sequence, so the MSVC formula is kept. */
int tu_rand(void)
{
	tu_holdrand = tu_holdrand * 0x343fdu + 0x269ec3u;
	return (int)((tu_holdrand >> 16) & 0x7fff);
}
