/*
CHARLIB_EXTERN.H

charlib module. What charlib code calls in other modules, with the prototypes that the
call sites in orig/default.xbe show (thiscall/fastcall: `this` and EDX become the first
parameters). Each macro casts the generated FUN_<addr> declaration, whose Ghidra prototype
is often wrong (missing `this`, float arguments shown as integers, missing returns). The
owner of the function decides its real prototype in its .decls file; where an owner has
already declared one, the cast here matches it. Names follow the owners' names.

Included by charlib.h.
*/

#ifndef OPENFRENZY_CHARLIB_EXTERN_H
#define OPENFRENZY_CHARLIB_EXTERN_H

/* an animation callback of bactor (reason 0: the animation finished) */
typedef int (*CHLAnimCallback)(int reason, void *instance, void *node, void *anim, void *context);

/* ---------- bkernel */

/* 0x0004b230 bkPrintf: the debug log (printf format) */
#define chl_Printf ((void (*)(const char *, ...))FUN_0004b230)
/* 0x0004c480 bkHeapAlloc(size, file, line, flags): operator new */
#define chl_Alloc ((void *(*)(unsigned int, const char *, int, unsigned int))FUN_0004c480)
/* 0x0004c4b0 bkHeapCalloc(size, fill, file, line, flags) */
#define chl_Calloc ((void *(*)(unsigned int, unsigned char, const char *, int, unsigned int))FUN_0004c4b0)
/* 0x0004c110 bkHeapFree(pointer) */
#define chl_Free ((void (*)(void *))FUN_0004c110)
/* 0x0004c5f0 bkTimerRead() / 0x0004c670 bkTimerDelta(start, end): profiling */
#define chl_TimerRead ((unsigned long long (*)(void))FUN_0004c5f0)
#define chl_TimerDelta ((unsigned long long (*)(unsigned long long, unsigned long long))FUN_0004c670)
/* 0x0004bb30 find a loaded object by name: (package, name, type, context) */
#define chl_FindObject ((void *(*)(void *, const char *, int, int))FUN_0004bb30)

/* bkernel matrix helpers: out = a * b (0x00050150 needs 16-byte aligned matrices,
0x0004ff30 does not; the game picks one by testing the addresses) */
#define chl_MatMulAligned ((void (*)(float *, float *, float *))FUN_00050150)
#define chl_MatMulUnaligned ((void (*)(float *, float *, float *))FUN_0004ff30)
static inline void chl_MatMul(float *out, float *a, float *b)
{
	if ((((unsigned int)out | (unsigned int)a | (unsigned int)b) & 0xf) == 0)
		chl_MatMulAligned(out, a, b);
	else
		chl_MatMulUnaligned(out, a, b);
}
/* 0x00050af0 matrix from Euler angles (out, x, y, z) */
#define chl_MatFromEuler ((void (*)(float *, float, float, float))FUN_00050af0)
/* 0x00050530 transform a vector by a matrix in place (matrix, vector) */
#define chl_MatTransform ((void (*)(float *, float *))FUN_00050530)

/* ---------- bdisplay */

/* 0x000416c0 bdSetTexture(stage, texture) */
#define chl_SetTexture ((int (*)(int, void *))FUN_000416c0)
/* 0x00044b00 set a render state (state, value, extra) */
#define chl_SetRenderState ((void (*)(int, unsigned int, int))FUN_00044b00)
/* 0x0003f2d0 bdDrawPrimitive(type, vertices, count) */
#define chl_DrawPrimitive ((void (*)(int, void *, int))FUN_0003f2d0)
/* 0x002bc490.. colour channel tables of bdisplay (256 entries each) */
extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bc898[256];
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bd640[256];

/* ---------- classlib_ui (math and lists) */

#define chl_ListInsert ((void (*)(void *, void *, int))FUN_00071070)
#define chl_ListRemove ((void *(*)(void *, void *))FUN_000710b0)
#define chl_ListNext ((void *(*)(void *, void *))FUN_00071110)
#define chl_ListInit2 ((void (*)(void *))FUN_00071060)

/* ---------- bactor (prototypes as declared in src/bactor/bactor.h) */

/* 0x00036b20 baInitActorInstance(inst or NULL, actor, slots, file, line) -> inst */
#define chl_InitInstance ((void *(*)(void *, void *, int, const char *, int))FUN_00036b20)
/* 0x00034930 baDeleteActorInstance(inst) */
#define chl_DeleteInstance ((void (*)(void *))FUN_00034930)
/* 0x00034bf0 baPrepActorInstance(inst, flags): matrices from position and orientation */
#define chl_PrepInstance ((void (*)(void *, int))FUN_00034bf0)
/* 0x00034d50 baDrawActorInstance(inst, passes) */
#define chl_DrawInstance ((int (*)(void *, int))FUN_00034d50)
/* 0x00034e70 baSetNodeTime(inst, node, time, recurse) */
#define chl_SetNodeTime ((void (*)(void *, void *, int, int))FUN_00034e70)
/* 0x00035cc0 baSetRenderPass(inst, pass, mode, param) */
#define chl_SetRenderPass ((void (*)(void *, int, int, int))FUN_00035cc0)
/* 0x00035d50 baSetBrightness(brightness): 0x100 full */
#define chl_SetBrightness ((void (*)(int))FUN_00035d50)
/* 0x00035770 baQueueAnimation(inst, node, anim, speed, time, flags, callback, context) */
#define chl_QueueAnimation \
	((void (*)(void *, void *, void *, int, int, unsigned int, CHLAnimCallback, void *))FUN_00035770)
/* 0x000358d0 baQueueAnimationBlendIn(inst, node, anim, speed, time, blend, flags, callback, context) */
#define chl_QueueAnimationBlendIn \
	((void (*)(void *, void *, void *, int, int, float, unsigned int, CHLAnimCallback, void *))FUN_000358d0)
/* 0x00038190 baFlushAnimations(inst, node, recurse, keep) */
#define chl_FlushAnimations ((void (*)(void *, void *, int, int))FUN_00038190)
/* 0x000380f0 baUpdateAnimations(inst, node, dt, root_motion) */
#define chl_UpdateAnimations ((int (*)(void *, void *, float, float *))FUN_000380f0)

/* ---------- bdisplay (render state helpers; names are guesses from use) */

/* 0x00046cb0 push a draw state (3, 0, 0) / 0x00046d00 pop it */
#define chl_PushDrawState ((void (*)(int, int, int))FUN_00046cb0)
#define chl_PopDrawState ((void (*)(void))FUN_00046d00)
/* 0x00044dd0 fog start and end / 0x00044e20 fog colour (r, g, b) */
#define chl_SetFogRange ((void (*)(float, float))FUN_00044dd0)
#define chl_SetFogColour ((void (*)(int, int, int))FUN_00044e20)

/* ---------- classlib_fx */

/* animated texture lists (AnimTex_*): 0x00082c20 advance(list, level, apply),
0x00082c80 apply(list), 0x00082f10 delete(list) */
#define chl_AnimTexAdvanceList ((void (*)(void *, float, int))FUN_00082c20)
#define chl_AnimTexApplyList ((void (*)(void *))FUN_00082c80)
#define chl_AnimTexDeleteList ((void (*)(void *))FUN_00082f10)

#endif
