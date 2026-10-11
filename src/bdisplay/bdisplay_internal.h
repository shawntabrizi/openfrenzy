/*
BDISPLAY_INTERNAL.H

Helpers for the bdisplay module's own files (not for other modules).

Calls into other modules go through the prototypes in of_decls.h (generated
from src/include/of_functions.tsv and the owners' .decls files). Where the
table's prototype cannot express the call the original makes (a variadic
printf, a result the table calls void, more arguments than the table
lists), the call goes through a cast to the true type below; once the
owner fixes the prototype, the cast does nothing.
*/

#ifndef OPENFRENZY_BDISPLAY_INTERNAL_H
#define OPENFRENZY_BDISPLAY_INTERNAL_H

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bdisplay_types.h"
#include "bdisplay_globals.h"

/* ---------- bKernel (module bkernel) */

/* 0x0004b230 the debug printf */
#define bdPrintf ((void (*)(const char *, ...))FUN_0004b230)
/* 0x0004bf10 allocate(size, file, line, flags, name, alignment) */
#define bdAlloc(size, file, line, flags, name, align) \
	((void *(*)(int, const char *, int, int, const char *, int))FUN_0004bf10)( \
		(size), (file), (line), (flags), (name), (align))
/* 0x0004c480 operator new: allocate(size, file, line, flags) */
#define bdNew(size, file, line, flags) \
	((void *(*)(int, const char *, int, int))FUN_0004c480)((size), (file), (line), (flags))
/* 0x0004c420 allocate(size, fill, file, line, flags, name) */
#define bdAllocFill(size, fill, file, line, flags, name) \
	((void *(*)(int, int, const char *, int, int, const char *))FUN_0004c420)( \
		(size), (fill), (file), (line), (flags), (name))
/* 0x0004c110 free */
#define bdFree(p) ((void (*)(void *))FUN_0004c110)((void *)(p))
/* 0x0004e390 find a resource in the loaded packages (type, name, 0, 0, 0, 0) */
#define bdFindResource(type, name) \
	((void *(*)(int, int, int, int, int, int))FUN_0004e390)((int)(type), (int)(name), 0, 0, 0, 0)

/* 0x0004c670 ticks from start to end; 0x0004c620 the rate of a tick count
(the original passes each 64-bit value as two dwords) */
#define bdTimerDelta(start, end) \
	((unsigned long long (*)(unsigned long long, unsigned long long))FUN_0004c670)((start), (end))
#define bdTimerRate(ticks) ((float (*)(long long))FUN_0004c620)((ticks))

/* ---------- XDK */

/* the kernel's debug break (xkernel: src/xkernel/xbox_kernel.c logs it and
continues); only xkernel's private header declares it */
VOID NTAPI DbgBreakPoint(VOID);

/* the XBE's 0x001e8823 */
#define bdPhysicalProtect(p, size, protect) XPhysicalProtect((void *)(p), (size), (protect))

static inline DWORD bd_float_bits(float f)
{
	DWORD bits;

	memcpy(&bits, &f, sizeof(bits));
	return bits;
}

static inline float bd_bits_float(DWORD bits)
{
	float f;

	memcpy(&f, &bits, sizeof(f));
	return f;
}

/* ---------- copies of three bKernel maths helpers.
Their table prototypes take the float arguments as integers (the original
code moves the bits); calling them with floats would convert the values.
These are the same operations, so this module does not depend on how the
owner settles those prototypes. */

/* 0x00050f80: square root by halving the exponent (approximate) */
static inline float bd_sqrt_approx(float x)
{
	int bits = (int)bd_float_bits(x);

	return bd_bits_float((DWORD)(((bits - 0x3f800000) >> 1) + 0x3f800000));
}

/* 0x00050250: translation matrix */
static inline void bd_matrix_translation(float *m, float x, float y, float z)
{
	m[12] = x;
	m[13] = y;
	m[0] = 1.0f;
	m[5] = 1.0f;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[11] = 0.0f;
	m[10] = 1.0f;
	m[14] = z;
	m[15] = 1.0f;
}

/* 0x000502a0: scale matrix */
static inline void bd_matrix_scale(float *m, float x, float y, float z)
{
	m[0] = x;
	m[5] = y;
	m[10] = z;
	m[15] = 1.0f;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
}

/* out = b x a (4x4, row-major): bKernel's SSE multiply (0x00050150) when
all three are 16-byte aligned, its plain one (0x0004ff30) otherwise; the
two compute the same product. The original tests the alignment the same
way (for its own globals the compiler folded the test). */
static inline void bd_matrix_multiply(float *out, float *a, float *b)
{
	if ((((uintptr_t)out | (uintptr_t)a | (uintptr_t)b) & 0xf) == 0)
		FUN_00050150(out, a, b);
	else
		FUN_0004ff30(out, a, b);
}

/* the colour of channels r, g, b, a (0-255) from the channel tables */
#define BD_RGBA(r, g, b, a) \
	(DAT_002bce18[(r)] | DAT_002bc490[(g)] | DAT_002bd640[(b)] | DAT_002bc898[(a)])

/* port (src/d3d8/d3d8_gl.c): the screen's width in the game's units: 640, or wider with
display.aspect_ratio (bdOpenDisplay; Hor+ in bdSetupProjection) */
long halo_screen_width(void);

#endif
