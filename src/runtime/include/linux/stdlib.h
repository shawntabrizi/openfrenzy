/*
STDLIB.H

Host <stdlib.h> plus MSVC-named functions and constants.
*/

#ifndef __HALO_LINUX_STDLIB_H
#define __HALO_LINUX_STDLIB_H

#include_next <stdlib.h>

#define _MAX_PATH 260
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256

/* MSVC's rand() (0x001e133e; src/xkernel/xbox_crt.c) returns 0..0x7fff, not musl's 0..0x7fffffff */
#undef RAND_MAX
#define RAND_MAX 0x7fff

char *_itoa(int value, char *string, int radix);
char *_ltoa(long value, char *string, int radix);
char *_ultoa(unsigned long value, char *string, int radix);
void _splitpath(const char *path, char *drive, char *directory, char *name, char *extension);
void _makepath(char *path, const char *drive, const char *directory, const char *name, const char *extension);
char *_fullpath(char *absolute_path, const char *relative_path, size_t maximum_length);

#define itoa _itoa
#define ltoa _ltoa
#define ultoa _ultoa

#endif
