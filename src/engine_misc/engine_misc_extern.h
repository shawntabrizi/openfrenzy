/*
ENGINE_MISC_EXTERN.H

Functions of other modules that engine_misc calls, with the prototype this module
believes is the true one. The generated of_decls.h has Ghidra's guesses for them
(for example a printf-like function with one parameter, or a float result shown as
void). Each macro casts the original FUN_<addr> to the believed type, so the call
compiles and passes float arguments in the right registers. When the owning module
fixes the prototype in its .decls file, these casts become no-ops.

Owner: engine_misc. Other modules: do not rely on these names.
*/

#ifndef OPENFRENZY_ENGINE_MISC_EXTERN_H
#define OPENFRENZY_ENGINE_MISC_EXTERN_H

#include <stdarg.h>

/* ---------- bKernel (module bkernel) */

/* 0x0004b230: debug printf (Babel bkPrintf). Variadic. */
#define em_bkPrintf ((void (*)(const char *, ...))FUN_0004b230)
/* 0x0004b380: debug vprintf (format, va_list) */
#define em_bkVPrintf ((void (*)(const char *, va_list))FUN_0004b380)
/* 0x0004bf10: named allocation (size, file, line, flags, name, 0) */
#define em_bkAllocNamed ((void *(*)(int, const char *, int, int, const char *, int))FUN_0004bf10)
/* 0x0004c480: allocation (size, file, line, flags): the game's operator new */
#define em_bkAlloc ((void *(*)(int, const char *, int, int))FUN_0004c480)
/* 0x0004c4b0: zeroed allocation (size, fill byte 0, file, line, flags) */
#define em_bkAllocZero ((void *(*)(int, int, const char *, int, int))FUN_0004c4b0)
/* 0x0004c110: free */
#define em_bkFree ((void (*)(void *))FUN_0004c110)
/* 0x0004c280: returns the name of an allocation group (used as a heap block name) */
#define em_bkGroupName ((const char *(*)(int))FUN_0004c280)
/* 0x0004c5f0: 64-bit tick counter */
#define em_bkGetTicks ((long long (*)(void))FUN_0004c5f0)
/* 0x0004d4f0: creates a named event/signal */
#define em_bkCreateEvent ((int (*)(const char *))FUN_0004d4f0)
/* 0x0004d590, 0x0004d620: event handle helpers of the background loader */
#define em_bkEventCallback ((int (*)(const char *, void *, void *))FUN_0004d590)
#define em_bkEventWaitHandle ((int (*)(const char *, int, int))FUN_0004d620)
/* 0x0004d6d0: tests an event (handle, 0) */
#define em_bkEventTest ((int (*)(int, int))FUN_0004d6d0)
/* 0x0004d7f0, 0x0004da50: event reset/create for a background load channel */
#define em_bkEventReset ((void (*)(const char *))FUN_0004d7f0)
#define em_bkEventSetup ((void (*)(const char *))FUN_0004da50)
/* 0x0004d860: deletes an event */
#define em_bkEventDelete ((void (*)(int))FUN_0004d860)
/* 0x0004cff0: background (asynchronous) file read */
#define em_bkBackgroundRead \
	((int (*)(int, void *, int, int, int, int, int, const char *, int))FUN_0004cff0)
/* 0x0004e390: finds a resource in the loaded packages */
#define em_bkFindResource ((void *(*)(int, int, int, int, int, int))FUN_0004e390)
/* 0x0004e520, 0x0004e5a0: creates / kills a worker thread (function, argument, name) */
#define em_bkCreateThread ((int (*)(void *, void *, const char *))FUN_0004e520)
#define em_bkKillThread ((void (*)(int))FUN_0004e5a0)
/* 0x0004e8f0: no arguments */
#define em_bkThreadYield ((void (*)(void))FUN_0004e8f0)
/* 0x0004b130: wide-string copy (dst, src) */
#define em_bkWcsCopy ((void (*)(unsigned short *, const unsigned short *))FUN_0004b130)

/* BINPUT (input.cpp, module bkernel) */
#define em_biCreateChannel \
	((struct BInputChannel *(*)(struct BInputMap *, const char *, int, int, int, int, int))FUN_00048a10)
#define em_biDeleteMap ((void (*)(struct BInputMap *))FUN_00048970)
#define em_biBind ((int (*)(struct BInputChannel *, unsigned int, unsigned int))FUN_00048b20)
#define em_biResetChannel ((void (*)(struct BInputChannel *))FUN_00048d40)
#define em_biUpdate ((int (*)(void))FUN_00048d60)
#define em_biInitPads ((void (*)(int))FUN_00049d40)

/* BMATHS vector/matrix helpers (module bkernel) */
/* 0x000502f0: dst = inverse of src (returns non-zero when invertible) */
#define em_bmMatInvert ((int (*)(void *, const void *))FUN_000502f0)
#define em_bmMatMul ((void (*)(void *, const void *, const void *))FUN_00050150)
#define em_bmMatMulUnaligned ((void (*)(void *, const void *, const void *))FUN_0004ff30)
#define em_bmMatTranslation ((void (*)(void *, float, float, float))FUN_00050250)
#define em_bmMatScale ((void (*)(void *, float, float, float))FUN_000502a0)
/* 0x000505c0: out = mat * in (out, mat, in) */
#define em_bmMatTransformVec ((void (*)(float *, const void *, const float *))FUN_000505c0)
/* 0x00050530: vec = vec * mat, in place (4 components) */
#define em_bmMatTransformInPlace ((void (*)(const void *, float *))FUN_00050530)
/* 0x00050650: dst = transpose of src */
#define em_bmMatTranspose ((void (*)(void *, const void *))FUN_00050650)
#define em_bmMatRotXY ((void (*)(void *, float, float))FUN_00050840)
#define em_bmMatRotXYZ ((void (*)(void *, float, float, float))FUN_00050d20)
#define em_bmSqrt ((float (*)(float))FUN_00050f80)
/* 0x00051220: out = v rotated by two angles (out, v, a, b) */
#define em_bmVecRotate ((void (*)(float *, const float *, float, float))FUN_00051220)
#define em_bmVecNormalise ((void (*)(float *, const float *))FUN_000514a0)
/* 0x00051540: squared distance */
#define em_bmVecDistSq ((float (*)(const float *, const float *))FUN_00051540)
#define em_bmMatFromQuat ((void (*)(void *, const void *))FUN_000515c0)

/* ---------- bActor (module bactor) */

#define em_baFindInstanceByCrc ((void *(*)(void *, int))FUN_00035b40)
/* BACTOR camera.cpp: 0x0003c000 creates a camera (camera, flags, then two tag groups read
by 0x0003bf20: tag 0/3 four doubles, 1 a word and a double, 2 two words and four doubles,
4 three doubles and a word, 5 two words); 0x0003c0b0 sets one group; 0x0003c330 deletes;
0x0003bf10 makes a camera current and returns the previous one (NULL: none). The groups
are read from the stack like variadic arguments (floats pushed as doubles), so the calls
here are variadic; module bactor must define them so. */
#define em_baCreateCamera ((void *(*)(void *, int, ...))FUN_0003c000)
#define em_baSetCamera ((void (*)(void *, ...))FUN_0003c0b0)
#define em_baDeleteCamera ((void (*)(void *))FUN_0003c330)
#define em_baSelectCamera ((void *(*)(void *))FUN_0003bf10)
/* 0x00035cc0, 0x00034d50: actor render state and drawing */
#define em_baSetRenderPass ((void (*)(void *, int, int, void *))FUN_00035cc0)
#define em_baDrawActor ((int (*)(void *, int))FUN_00034d50)
/* 0x00051f80, 0x00051fe0: count / fix up the shapes of a collision resource (bkernel) */
#define em_bmCountShapes ((int (*)(void *))FUN_00051f80)
#define em_bmFixupShapes ((void (*)(void *, void *))FUN_00051fe0)

/* ---------- other modules' vector helpers */

#define em_bmVecSub ((void (*)(float *, const float *, const float *))FUN_00019230)
#define em_bmVecAdd ((void (*)(float *, const float *, const float *))FUN_0001e9d0)
#define em_bmVecScale ((void (*)(float *, const float *, float))FUN_0001ea00)
#define em_bmVecCross ((void (*)(float *, const float *, const float *))FUN_00036ac0)
#define em_bmVecDivide ((void (*)(float *, const float *, float))FUN_000384a0)
#define em_bmVecCopy4 ((void (*)(void *, const void *))FUN_000127e0)

/* ---------- bDisplay (module bdisplay) */

/* 0x00047010: text of a DirectX error code */
#define em_bdErrorString ((const char *(*)(int))FUN_00047010)
/* 0x00047280: shades a colour (r, g, b, &r, &g, &b, delta) */
#define em_bdShadeColour ((void (*)(int, int, int, int *, int *, int *, int))FUN_00047280)
/* 0x000425a0: locks a surface (surface, 0, 0, 0, 0, &pitch) and returns its bits */
#define em_bdLockSurface ((void *(*)(int, int, int, int, int, int *))FUN_000425a0)
/* 0x00043cf0: projects a world point to the screen (out, in, flags) */
#define em_bdProjectPoint ((void (*)(float *, const float *, int))FUN_00043cf0)
/* camera and render-target helpers (module bdisplay); float arguments as the original
pushes floats */
#define em_bdSetWorldMatrix ((void (*)(const void *))FUN_00043930)
#define em_bdSetVertexMode ((void (*)(int, int, int))FUN_00046cb0)
#define em_bdResetVertexMode ((void (*)(int))FUN_00046d00)
#define em_bdDrawPrimitive ((void (*)(int, const void *, int))FUN_0003f2d0)
#define em_bdSetRenderTarget ((int (*)(int, int, int, int, int, float, int))FUN_00045330)
#define em_bdLookAt ((void (*)(float *, float *, float))FUN_000435f0)
#define em_bdSetCameraPosition ((void (*)(float, float, float))FUN_00043490)
#define em_bdSetFov ((void (*)(float, float))FUN_00044a00)
#define em_bdSetClipPlanes ((void (*)(float, float))FUN_00044810)
#define em_bdApplyCamera ((void (*)(void))FUN_00043790)
#define em_bdCreateRenderTexture ((int (*)(int, int, int, int, unsigned int))FUN_00044e90)
#define em_bdReleaseTexture ((void (*)(int))FUN_000458a0)
#define em_bdSetTexture ((void (*)(int, int))FUN_000455a0)
#define em_bdPushState ((int (*)(int))FUN_00044ac0)
#define em_bdPopState ((int (*)(int))FUN_00044ae0)
#define em_bdSetTextureStage ((void (*)(int, unsigned int, int))FUN_00044b00)
#define em_bdSetStageTexture ((int (*)(int, int))FUN_000416c0)

#endif
