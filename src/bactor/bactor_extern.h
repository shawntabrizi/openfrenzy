/*
BACTOR_EXTERN.H

What BACTOR uses from other modules and from the platform layer, with the
prototype this module believes is the true one.

- Functions of other modules: each macro casts the original FUN_<addr> to
  the believed type, so a call compiles and passes float arguments in the
  right registers whatever prototype of_decls.h has today. When the owner
  fixes its .decls, the casts become no-ops.
- Globals of other modules (bss of bdisplay and bkernel): declared here and
  defined as common (tentative) definitions in bactor_shared.c, under their
  Ghidra names, so that they merge with the owner's definition.
- XDK functions the shared XDK headers do not declare come from
  src/d3d8/d3d8_extra.h.

Owner: bactor. Other modules: do not rely on these names.
*/

#ifndef OPENFRENZY_BACTOR_EXTERN_H
#define OPENFRENZY_BACTOR_EXTERN_H

#include "xtl.h"
#include "d3d8_extra.h" /* src/d3d8: lights, DrawVerticesUP, XGAssembleShader, XGBuffer */

/* ---------- bKernel (module bkernel) */

/* 0x0004b230: debug printf */
#define ba_bkPrintf ((void (*)(const char *, ...))FUN_0004b230)
/* 0x0004c480: allocation (size, file, line, flags): operator new */
#define ba_bkAlloc ((void *(*)(unsigned int, const char *, int, int))FUN_0004c480)
/* 0x0004c4b0: allocation filled with a byte (size, fill, file, line, flags) */
#define ba_bkAllocFill ((void *(*)(unsigned int, int, const char *, int, int))FUN_0004c4b0)
/* 0x0004c420: named allocation (size, fill, file, line, flags, name) */
#define ba_bkAllocNamedFill ((void *(*)(unsigned int, int, const char *, int, int, const char *))FUN_0004c420)
/* 0x0004bf10: named allocation (size, file, line, flags, name, 0) */
#define ba_bkAllocNamed ((void *(*)(unsigned int, const char *, int, int, const char *, int))FUN_0004bf10)
/* 0x0004c110: free */
#define ba_bkFree ((void (*)(void *))FUN_0004c110)
/* 0x0004c290, 0x0004c2c0: push / pop the name of the current allocation group */
#define ba_bkPushAllocName ((int (*)(const char *))FUN_0004c290)
#define ba_bkPopAllocName ((int (*)(void))FUN_0004c2c0)
/* 0x0004dc30: hash of a name */
#define ba_bkHash ((uint32_t (*)(const char *))FUN_0004dc30)
/* 0x0004d8b0: sends a text message to a target */
#define ba_bkSendMessage ((int (*)(int, const char *))FUN_0004d8b0)
/* 0x0004e390: finds a resource in the loaded packages (item, package, 0, 0, 0, 0) */
#define ba_bkFindResource ((void *(*)(void *, void *, int, int, int, int))FUN_0004e390)
/* 0x0004b7a0: finds a texture (0, name, package handle, package index) */
#define ba_bkFindTexture ((void *(*)(int, int, uint32_t, int))FUN_0004b7a0)
/* 0x0004e8f0: no arguments */
#define ba_bkFlushResources ((void (*)(void))FUN_0004e8f0)

/* bMaths (in bkernel's range) */
/* 0x00050150 (16-byte aligned operands), 0x0004ff30: out = a * b (4x4) */
#define ba_MatMulAligned ((void (*)(float *, const float *, const float *))FUN_00050150)
#define ba_MatMul ((void (*)(float *, const float *, const float *))FUN_0004ff30)
/* 0x00050250: translation matrix */
#define ba_MatTranslation ((void (*)(float *, float, float, float))FUN_00050250)
/* 0x000502a0: scaling matrix */
#define ba_MatScaling ((void (*)(float *, float, float, float))FUN_000502a0)
/* 0x000502f0: out = inverse(m); returns 0 when singular */
#define ba_MatInverse ((int (*)(float *, const float *))FUN_000502f0)
/* 0x00050530: v = v * m (in place, 4 components) */
#define ba_MatTransformInPlace ((void (*)(const float *, float *))FUN_00050530)
/* 0x000505c0: out = v * m (4 components) */
#define ba_MatTransform ((void (*)(float *, const float *, const float *))FUN_000505c0)
/* 0x00050650: out = transpose(m) */
#define ba_MatTranspose ((void (*)(float *, const float *))FUN_00050650)
/* 0x000506c0: out = scale/rotation/translation (tx, ty, tz, rot matrix, sx, sy, sz) */
#define ba_MatCompose ((void (*)(float *, float, float, float, const float *, float, float, float))FUN_000506c0)
/* 0x00050f80: fast approximate square root */
#define ba_FastSqrt ((float (*)(float))FUN_00050f80)
/* 0x000514a0: out = normalize(v) (3 components; unchanged when zero) */
#define ba_Vec3Normalize ((void (*)(float *, const float *))FUN_000514a0)
/* 0x000515c0: rotation matrix of a quaternion */
#define ba_MatFromQuat ((void (*)(float *, const float *))FUN_000515c0)
/* 0x00051860: out = slerp(a, b, t) */
#define ba_QuatSlerp ((void (*)(float *, const float *, const float *, float))FUN_00051860)
/* 0x00051d30: axis and angle of a quaternion */
#define ba_QuatToAxisAngle ((void (*)(float *, const float *))FUN_00051d30)
/* 0x00051df0: quaternion of an axis and angle */
#define ba_QuatFromAxisAngle ((void (*)(float *, const float *))FUN_00051df0)

/* ---------- bDisplay (module bdisplay) */

/* 0x0003ecc0: vertex size of a vertex shader / FVF */
#define ba_bdVertexSize ((int (*)(uint32_t))FUN_0003ecc0)
/* 0x0003efa0: selects a vertex shader / FVF and a vertex buffer */
#define ba_bdSetVertexShader ((void (*)(uint32_t, void *))FUN_0003efa0)
/* 0x0003f500: creates a vertex buffer (out, vertices, FVF, usage, flags) */
#define ba_bdCreateVertexBuffer ((int (*)(void **, int, uint32_t, int, int))FUN_0003f500)
/* 0x0003f650: releases a vertex buffer */
#define ba_bdReleaseVertexBuffer ((void (*)(void *))FUN_0003f650)
/* 0x0003f710: locks a vertex buffer, returns its data */
#define ba_bdLockVertexBuffer ((void *(*)(void *, int))FUN_0003f710)
/* 0x0003f790: unlocks a vertex buffer */
#define ba_bdUnlockVertexBuffer ((int (*)(void *))FUN_0003f790)
/* 0x000416c0: sets the texture of a stage */
#define ba_bdSetTexture ((int (*)(int, void *))FUN_000416c0)
/* 0x00041810: clears the texture of a stage */
#define ba_bdClearTexture ((void (*)(int))FUN_00041810)
/* 0x00043490: sets the camera position */
#define ba_bdSetCameraPos ((void (*)(float, float, float))FUN_00043490)
/* 0x000434b0: sets the camera angles (x, y, z, rotation order) */
#define ba_bdSetCameraAngles ((void (*)(float, float, float, int))FUN_000434b0)
/* 0x000435f0: points the camera from eye to target with a roll */
#define ba_bdSetCameraLookAt ((void (*)(float *, float *, float))FUN_000435f0)
/* 0x00043930: sets the world matrix */
#define ba_bdSetWorldMatrix ((void (*)(const float *))FUN_00043930)
/* 0x00043d20: bounding box visibility (matrix, min xyz, max xyz) */
#define ba_bdBoxVisible \
	((char (*)(const float *, float, float, float, float, float, float))FUN_00043d20)
/* 0x00044810: sets the near and far planes */
#define ba_bdSetNearFar ((void (*)(float, float))FUN_00044810)
/* 0x00044a00: sets the field of view and aspect */
#define ba_bdSetFov ((void (*)(float, float))FUN_00044a00)
/* 0x00044ac0, 0x00044ae0: lock / unlock a display state */
#define ba_bdLockState ((int (*)(int))FUN_00044ac0)
#define ba_bdUnlockState ((int (*)(int))FUN_00044ae0)
/* 0x00044b00: sets a display state (state, value, 0) */
#define ba_bdSetState ((void (*)(int, uint32_t, int))FUN_00044b00)
/* 0x00044e70: applies the global brightness */
#define ba_bdSetBrightness ((void (*)(int))FUN_00044e70)
/* 0x00046cb0, 0x00046d00: push / pop a display state (state, value, 0) */
#define ba_bdPushState ((void (*)(int, uint32_t, int))FUN_00046cb0)
#define ba_bdPopState ((void (*)(int))FUN_00046d00)
/* 0x00047010: text of a D3D error */
#define ba_bdErrorString ((const char *(*)(int))FUN_00047010)

/* bdisplay's globals. These mirror src/bdisplay/bdisplay_globals.h (names,
types, array sizes and the aliases into arrays) so that the symbols are the
same; bactor_shared.c holds tentative definitions that merge with bdisplay's
(-fcommon). When bdisplay_globals.h is committed, include it instead. */
struct bdVertexBuffer;
extern struct bdVertexBuffer *DAT_00284c10; /* stream 0 vertex buffer; 1 = none (initialised data) */
extern float *PTR_DAT_00284c70; /* projection matrix in use (initialised data) */
extern float DAT_002be330[16]; /* camera rotation */
extern float DAT_002be370[16]; /* view */
extern float DAT_002be3b0[16]; /* view * projection, valid when DAT_002be5c8 */
extern float DAT_002be430[16]; /* world * view */
extern float DAT_002be470[16]; /* world * view * projection */
extern float DAT_002be570[3]; /* camera position */
extern float DAT_002be580; /* near plane */
extern float DAT_002be584; /* far plane */
extern float DAT_002be588; /* horizontal field of view */
extern float DAT_002be58c; /* vertical field of view */
extern float DAT_002be590; /* viewport width */
extern float DAT_002be594; /* viewport height */
extern int DAT_002be5c8; /* DAT_002be3b0 is up to date */
extern float DAT_002be5e8; /* fog end */
extern float DAT_002be5ec; /* 1 / (fog end - fog start) */
extern uint32_t DAT_002be600[15 * 2]; /* bdSetRenderState values: [state * 2] */
#define DAT_002be618 (DAT_002be600[6]) /* state 3: lighting */
#define DAT_002be620 (DAT_002be600[8]) /* state 4 */
#define DAT_002be630 (DAT_002be600[12]) /* state 6 */
extern int DAT_002bee34[4]; /* texture stage enabled */
#define DAT_002bee38 (DAT_002bee34[1])
extern int DAT_002bee44[15]; /* state locked */
#define DAT_002bee4c (DAT_002bee44[2])
extern int DAT_002bee80;
extern int DAT_002bee84;
extern int DAT_002bc3ec; /* maximum number of enabled lights */
extern uint32_t DAT_002bc400; /* current vertex shader */
extern int DAT_002bc424; /* stencil bits */
extern int DAT_002bc890; /* frames presented */
extern unsigned int DAT_002bc490[256]; /* colour conversion: green << 8 */
extern unsigned int DAT_002bc898[256]; /* alpha << 24 */
extern unsigned int DAT_002bce18[256]; /* red << 16 */
extern unsigned int DAT_002bd640[256]; /* blue */
extern float DAT_002bd220[256]; /* i * 2 / 255, clamped to 1 */
/* bkernel's (src/bkernel/bkernel_core.h): break into the debugger on shader errors */
extern int DAT_002cc4f0;

#endif
