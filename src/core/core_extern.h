/*
CORE_EXTERN.H

openfrenzy core module. What core code calls in other modules and in the platform layer,
with the prototypes that the call sites in orig/default.xbe show.

Why: the generated of_decls.h has Ghidra's prototype guesses, which often miss the
`this` (ECX) argument, stack arguments or the return value. Each ext_* macro below casts
the function to the prototype seen in the x86 code (thiscall/fastcall: `this` and EDX
become the first parameters). The owner of the function decides its real prototype in its
.decls file; when it differs from the one here, one of the two is wrong: check the
disassembly and fix this file. Names in comments are guesses from strings and use.

Do not use weak references to test whether a function exists: ld.lld resolves an
undefined weak symbol in a PC-relative (ADRP) reference to the current page, not to 0.
*/

#ifndef OPENFRENZY_CORE_EXTERN_H
#define OPENFRENZY_CORE_EXTERN_H

#include <stdint.h>

struct CGameMode;

/* ---------- bkernel */

/* 0x0004b230 debug printf to the log channels (printf format) */
#define ext_DebugPrintf ((void (*)(const char *, ...))FUN_0004b230)
/* 0x0004b160 open a log channel (0, path or 0, 0xe) -> handle */
#define ext_LogOpen ((void *(*)(int, const char *, int))FUN_0004b160)
/* 0x0004b210 select the log channel */
#define ext_LogSelect ((void (*)(void *))FUN_0004b210)
/* 0x0004b540 close a log channel */
#define ext_LogClose ((void (*)(void *))FUN_0004b540)
/* 0x0004b620 (-1) */
#define ext_FUN_0004b620 ((void (*)(int))FUN_0004b620)
/* 0x0004c480 operator new (size, source file, line, flags 0x1001) */
#define ext_New ((void *(*)(unsigned int, const char *, int, int))FUN_0004c480)
/* 0x0004c110 operator delete */
#define ext_Delete ((void (*)(void *))FUN_0004c110)
/* 0x0004c230 heap statistics: returns total free, *largest = largest free block */
#define ext_HeapFree ((int (*)(int *))FUN_0004c230)
/* 0x0004c2e0 physical memory in use */
#define ext_PhysMem ((int (*)(void))FUN_0004c2e0)
/* 0x0004c300 heap dump/check */
#define ext_FUN_0004c300 ((void (*)(void))FUN_0004c300)
/* 0x0004f7a0 try to reserve memory (0, size, 1) -> nonzero on success */
#define ext_TryReserve ((int (*)(int, int, int))FUN_0004f7a0)
/* 0x0004e690 start the game thread: (function, 0) */
#define ext_RunMain ((void (*)(void (*)(void), int))FUN_0004e690)
/* 0x0004eed0 (1) */
#define ext_FUN_0004eed0 ((void (*)(int))FUN_0004eed0)
/* 0x0004f710 preload a package: (path, first, 0xdefa, 0) */
#define ext_PreloadPackage ((int (*)(const char *, int, int, int))FUN_0004f710)
/* 0x0004f860 */
#define ext_FUN_0004f860 ((void (*)(void))FUN_0004f860)
/* 0x0004f890 timer init (timer, frames); 0x0004f8b0 timer tick; 0x0004f8c0 timer due */
#define ext_TimerInit ((void (*)(void *, int))FUN_0004f890)
#define ext_TimerTick ((void (*)(void *))FUN_0004f8b0)
#define ext_TimerDue ((int (*)(void *))FUN_0004f8c0)
/* 0x0004c5f0 read the 64-bit clock */
#define ext_ClockRead ((uint64_t (*)(void))FUN_0004c5f0)
/* 0x0004c670 clock difference (start, end) */
#define ext_ClockDiff ((uint64_t (*)(uint64_t, uint64_t))FUN_0004c670)
/* 0x0004c620 clock ticks to seconds (x87 result) */
#define ext_ClockToSeconds ((float (*)(uint64_t))FUN_0004c620)
/* 0x0004d4f0 does a debug channel exist (name) */
#define ext_ChannelExists ((int (*)(const char *))FUN_0004d4f0)
/* 0x0004d620 open a debug channel (name, 100, 0) */
#define ext_ChannelOpen ((void *(*)(const char *, int, int))FUN_0004d620)
/* 0x0004d6d0 receive from a channel (channel, buffer or 0) -> nonzero when a message came */
#define ext_ChannelReceive ((int (*)(void *, char *))FUN_0004d6d0)
/* 0x0004bb30 find a resource (package or 0, name, type, 0xdefa) */
#define ext_FindResource ((void *(*)(int, const char *, int, int))FUN_0004bb30)
/* 0x0004ae80 swprintf for 16-bit (UTF-16) strings */
#define ext_WSprintf ((int (*)(uint16_t *, const uint16_t *, ...))FUN_0004ae80)
/* 0x0004b110 text layout (string, x, y, r, g, b, a, 0, 0) */
#define ext_TextLayout ((int (*)(const uint16_t *, float, float, int, int, int, int, int, int))FUN_0004b110)
/* 0x0004b7a0 heap check (9, ptr, size, line) */
#define ext_HeapCheck ((int (*)(int, uint32_t, uint32_t, uint32_t))FUN_0004b7a0)
/* 0x00048d60 controller state changed (any) */
#define ext_PadsChanged ((uint32_t (*)(void))FUN_00048d60)
/* 0x0004a0d0 port of controller N, -1 when absent */
#define ext_PadPort ((int (*)(int))FUN_0004a0d0)
/* 0x000488c0 create an input map (count) */
#define ext_InputMapCreate ((void *(*)(int))FUN_000488c0)
/* 0x00048a10 add a button to an input map (map, name, 1, b, 0, 0, 0) */
#define ext_InputMapAdd ((void *(*)(void *, const char *, int, int, int, int, int))FUN_00048a10)
/* 0x00048b20 bind a button (button, code, 0) */
#define ext_InputBind ((void (*)(void *, uint32_t, int))FUN_00048b20)
/* 0x00048970 free an input map */
#define ext_InputMapFree ((void (*)(void *))FUN_00048970)

/* ---------- bdisplay */

#define ext_DisplayPreInit ((void (*)(void))FUN_00042be0)
/* 0x00042c70 open the display (width, height, bpp, zdepth, flags) */
#define ext_DisplayOpen ((int (*)(int, int, int, int, int))FUN_00042c70)
/* 0x00041e90 get the display size (&w, &h, &bpp, &zdepth) */
#define ext_DisplayGet ((void (*)(int *, int *, int *, int *))FUN_00041e90)
/* 0x00044810 near and far planes */
#define ext_SetClipPlanes ((void (*)(float, float))FUN_00044810)
#define ext_GuiSetup ((int (*)(void))FUN_00047070)
#define ext_DisplayClose ((void (*)(void))FUN_00042530)
#define ext_BeginScene ((int (*)(void))FUN_00042570)
#define ext_EndScene ((void (*)(void))FUN_00042590)
/* 0x00041ee0 clear colour (r, g, b, a) */
#define ext_SetClearColour ((void (*)(int, int, int, int))FUN_00041ee0)
/* 0x00043b70 select a render state set; 0x000438b0 */
#define ext_SetRenderMode ((void (*)(int))FUN_00043b70)
#define ext_FUN_000438b0 ((void (*)(void))FUN_000438b0)
/* 0x00043440 draw text (font, string, layout) */
#define ext_DrawText ((void (*)(void *, const uint16_t *, int))FUN_00043440)
/* 0x0003efa0 (0x152, 0) */
#define ext_FUN_0003efa0 ((void (*)(int, int))FUN_0003efa0)

/* ---------- engine_misc */

/* 0x000559a0 (0.01f from 0x00284414) */
#define ext_FUN_000559a0 ((void (*)(float))FUN_000559a0)
#define ext_FUN_00055240 ((void (*)(int))FUN_00055240)
#define ext_FUN_00047cb0 ((void (*)(void))FUN_00047cb0)
#define ext_FUN_00047de0 ((void (*)(void))FUN_00047de0)
#define ext_FUN_00054730 ((void (*)(void))FUN_00054730)

/* ---------- classlib_ui */

#define ext_FUN_00071b10 ((void (*)(void))FUN_00071b10)
#define ext_FUN_00071690 ((void (*)(void))FUN_00071690)
/* 0x00071720 play a sound (5, 0x7f, 0, 0, 0) */
#define ext_PlaySfx ((void (*)(int, int, int, int, int))FUN_00071720)
/* 0x000713a0 play a sample (sample, a, b, 0, position or 0) */
#define ext_PlaySample ((int (*)(void *, int, int, int, void *))FUN_000713a0)
/* thiscall on the object at 0x002ecd90 */
#define ext_FUN_000762e0 ((void (*)(void *))FUN_000762e0)
#define ext_PauseResume ((void (*)(void *))FUN_000752c0)
#define ext_PauseStart ((void (*)(void *))FUN_00075320)
/* 0x00071050 list head init (this): next = prev = this, count = 0 */
#define ext_ListInit ((void (*)(void *))FUN_00071050)

/* ---------- objects made by core_init (thiscall constructors return this) */

/* 0x00082860 the command line / settings object (0x5804 bytes); 0x00082870 get (this, key) */
#define ext_SettingsCtor ((void *(*)(void *))FUN_00082860)
#define ext_SettingsGet ((char *(*)(void *, const char *))FUN_00082870)
/* 0x00082a00 */
#define ext_FUN_00082a00 ((void (*)(void))FUN_00082a00)
/* 0x001d5950 the empty function (folded trivial destructors) */
#define ext_Nothing ((void (*)(void *))FUN_001d5950)

/* game modes (core_modes_a, core_modes_b): constructors (this) -> this */
#define ext_CGameMode_Root_Ctor ((struct CGameMode *(*)(void *))FUN_00026c60)
#define ext_CGameMode_13a10_Ctor ((struct CGameMode *(*)(void *))FUN_00013a10)
#define ext_CGameMode_18350_Ctor ((struct CGameMode *(*)(void *))FUN_00018350)
/* 0x000128b0 / 0x000128c0: CGameMode hooks on push (mode) and pop (mode) */
#define ext_CGameMode_OnPush ((void (*)(struct CGameMode *))FUN_000128b0)
#define ext_CGameMode_OnPop ((int (*)(struct CGameMode *))FUN_000128c0)

/* the 13 games (constructors (this) -> this) */
#define ext_Game0_Ctor ((void *(*)(void *))FUN_00151040)
#define ext_Game1_Ctor ((void *(*)(void *))FUN_000828d0)
#define ext_Game2_Ctor ((void *(*)(void *))FUN_000b1a60)
#define ext_Game3_Ctor ((void *(*)(void *))FUN_000d9390)
#define ext_Game4_Ctor ((void *(*)(void *))FUN_000e7af0)
#define ext_Game5_Ctor ((void *(*)(void *))FUN_0010f830)
#define ext_Game6_Ctor ((void *(*)(void *))FUN_00125360)
#define ext_Game7_Ctor ((void *(*)(void *))FUN_0013b990)
#define ext_Game8_Ctor ((void *(*)(void *))FUN_0016cce0)
#define ext_Game9_Ctor ((void *(*)(void *))FUN_001877b0)
#define ext_Game10_Ctor ((void *(*)(void *))FUN_0019d530)
#define ext_Game11_Ctor ((void *(*)(void *))FUN_001ba560)
#define ext_Game12_Ctor ((void *(*)(void *))FUN_001c1760)

/* ---------- platform (XDK) */

/* D3D: implemented in src/d3d8 (fastcall in the XDK: stage, type, value) */
void D3DDevice_SetTextureState_Deferred(unsigned long stage, int type, unsigned long value);
/* src/d3d8/d3d8_extra.h */
struct core_field_status { int field; unsigned int vblank_count; };  /* D3DFIELD_STATUS */
void D3D_KickOffAndWaitForIdle(void);
void D3DDevice_GetDisplayFieldStatus(struct core_field_status *status);

/* XAPI / kernel (src/xkernel, src/vfs) */
struct core_memory_status {  /* MEMORYSTATUS, 32 bytes */
	uint32_t dwLength, dwMemoryLoad, dwTotalPhys, dwAvailPhys;
	uint32_t dwTotalPageFile, dwAvailPageFile, dwTotalVirtual, dwAvailVirtual;
};
void GlobalMemoryStatus(struct core_memory_status *status);
int XMountUtilityDrive(int format_clean);  /* src/vfs/vfs_files.h */
void DbgBreakPoint(void);
unsigned long XGetVideoStandard(void);  /* 0x000309f9 */
unsigned long XGetVideoFlags(void);     /* 0x00030a22 */

#endif
