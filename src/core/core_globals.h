/*
CORE_GLOBALS.H

openfrenzy core module. Classes and globals of Core\*.cpp (BlitzParty game core:
core.cpp, the game-mode stack, cheats, the CGameMode_* screens).

Core-owned globals live in the blocks of core_data.h and are named here by their
original address (CORE_VAR). Globals of other modules are declared at the end: an
uninitialised one gets a tentative definition in crt/core_foreign.c (merged with the
owner's by -fcommon), an initialised one a weak definition with its XBE value.
Layouts are the Xbox ones (ILP32): do not change offsets.
*/

#ifndef OPENFRENZY_CORE_GLOBALS_H
#define OPENFRENZY_CORE_GLOBALS_H

#include <stdint.h>
#include "core_data.h"

/* ---------- CGameMode (base class in core_modes_a) and the mode stack */

struct CGameMode;

/* the CGameMode virtual functions that the mode stack calls */
struct CGameModeVtbl {
	void *(*destroy)(struct CGameMode *self, int flags);  /* +0x00 scalar deleting destructor */
	void (*suspend)(struct CGameMode *self);               /* +0x04 another mode goes on top */
	void (*resume)(struct CGameMode *self);                /* +0x08 back on top / start */
	void (*exit)(struct CGameMode *self);                  /* +0x0c popped */
	void (*enter)(struct CGameMode *self);                 /* +0x10 pushed */
	void (*update)(struct CGameMode *self, int top);       /* +0x14 */
	void (*process)(struct CGameMode *self, int top);      /* +0x18 */
	void (*draw)(struct CGameMode *self, int top);         /* +0x1c from the first opaque mode up */
	void (*post_draw)(struct CGameMode *self, int top);    /* +0x20 */
};

struct CGameMode {
	const struct CGameModeVtbl *vtbl; /* +0x00 */
	uint32_t field_04;                /* +0x04 */
	const char *name;                 /* +0x08 "Root", "BGeffects", "PressStart", ... */
	uint32_t field_0c;                /* +0x0c */
	uint8_t flags;                    /* +0x10 bit 0: opaque (modes below are not drawn) */
};

#define CGAMEMODESTACK_MAX 20

/* 0x002ba750 (CGameModeStack, core.cpp) */
struct CGameModeStack {
	struct CGameMode *modes[CGAMEMODESTACK_MAX]; /* +0x00 */
	int count;                                   /* +0x50 */
	int first_drawn;                             /* +0x54 the top opaque mode */
};

/* ---------- the 13 games (CGame objects made in core.cpp) */

struct CGame;
struct CGameVtbl {
	void *(*destroy)(struct CGame *self, int flags); /* +0x00 */
	int (*init)(struct CGame *self);                 /* +0x04 */
	void (*shutdown)(struct CGame *self);            /* +0x08 */
	int (*start)(struct CGame *self, int variation); /* +0x0c */
};
struct CGame {
	const struct CGameVtbl *vtbl; /* +0x00 */
	const char *name;             /* +0x04 */
};
#define CORE_GAME_COUNT 13

/* ---------- cheats (0x00283d30, 7 entries of 0x4c bytes, in .data) */

struct CoreCheat {
	const char *name;     /* +0x00 "Welsh Mode", "Real Controls", ... */
	int sequence[16];     /* +0x04 button codes 0xc5-0xc8, 0 ends */
	int state;            /* +0x44 */
	int states;           /* +0x48 2: on/off, else the number of states */
};
#define CORE_CHEAT_COUNT 7
#define CORE_CHEAT_DEBUG 6

/* ---------- core globals (original addresses) */

#define g_ModeStack CORE_VAR(struct CGameModeStack, 0x002ba750)
#define g_PausedLast CORE_VAR(int, 0x002ba748)
#define g_CheatMessage ((uint16_t *)CORE_ADDR(0x002ba7a8))     /* UTF-16, 0x80 characters */
#define g_CheatButtons ((void **)CORE_ADDR(0x002ba8a8))        /* 4 input buttons */
#define g_CheatInput ((int *)CORE_ADDR(0x002ba8b8))            /* 17 entries */
#define g_CheatInputMap CORE_VAR(void *, 0x002ba8fc)
#define g_CheatInputLen CORE_VAR(int, 0x002ba900)
#define g_CheatMessageTime CORE_VAR(float, 0x002ba904)
#define g_FirstMode CORE_VAR(struct CGameMode *, 0x002ba908)
#define g_SoundChannel CORE_VAR(void *, 0x002ba90c)
#define g_DisplayWidth CORE_VAR(int, 0x002ba918)
#define g_DisplayHeight CORE_VAR(int, 0x002ba91c)
#define g_DisplayBpp CORE_VAR(int, 0x002ba920)
#define g_DisplayZDepth CORE_VAR(int, 0x002ba924)
#define g_DisplayActualWidth CORE_VAR(int, 0x002ba928)
#define g_DisplayActualHeight CORE_VAR(int, 0x002ba92c)
#define g_DisplayActualBpp CORE_VAR(int, 0x002ba930)
#define g_DisplayActualZDepth CORE_VAR(int, 0x002ba934)
#define g_DisplayFullscreen CORE_VAR(int, 0x002ba938)
#define g_Display60Hz CORE_VAR(int, 0x002ba93c)
#define g_ClearRed CORE_VAR(int, 0x002ba940)
#define g_ClearGreen CORE_VAR(int, 0x002ba944)
#define g_ClearBlue CORE_VAR(int, 0x002ba948)
#define g_ClearAlpha CORE_VAR(int, 0x002ba94c)
#define g_Quit CORE_VAR(int, 0x002ba950)
#define g_Games ((struct CGame **)CORE_ADDR(0x002ba954))       /* CORE_GAME_COUNT */
#define g_RefreshRate CORE_VAR(float, 0x002ba98c)
#define g_Fps CORE_VAR(float, 0x002ba990)
#define g_FrameSeconds CORE_VAR(float, 0x002ba994)
#define g_FrameCount CORE_VAR(int, 0x002ba99c)
#define g_Settings CORE_VAR(void *, 0x002ba9a0)
#define g_DemoTimerOff CORE_VAR(int, 0x002ba9b0)
#define g_DebugInputMap CORE_VAR(void *, 0x002ba9b4)
#define g_ScreenshotButton CORE_VAR(void *, 0x002ba9b8)
#define g_DebugPosButton CORE_VAR(void *, 0x002ba9bc)
#define g_VideoStandard CORE_VAR(int, 0x002ba9c0)
#define g_DemoTimer CORE_VAR(float, 0x002ba9c8)
#define g_VideoFlags CORE_VAR(unsigned int, 0x002ba9cc)
#define g_LastClock CORE_VAR(uint64_t, 0x002ba9d0)
#define g_FlagBAAD8 CORE_VAR(int, 0x002baad8)
#define g_LaunchedFromDemos CORE_VAR(int, 0x002baadc)
#define g_Log2 CORE_VAR(void *, 0x002baae0)
#define g_DebugLog CORE_VAR(void *, 0x002baae4)
#define g_PadsChanged CORE_VAR(uint32_t, 0x002baae8)
#define g_LastVBlank CORE_VAR(int, 0x002baaec)
#define g_ShowFps CORE_VAR(int, 0x002baaf0)
#define g_FpsText ((uint16_t *)CORE_ADDR(0x002baaf8))
#define g_NoHeapCheck CORE_VAR(int, 0x002bae84)
#define g_GameType CORE_VAR(int, 0x002bae8c)
#define g_StartState CORE_VAR(int, 0x002bae98)
#define g_FlagBAE9C CORE_VAR(int, 0x002bae9c)
#define g_MusicOn CORE_VAR(int, 0x002baeb8)

/* players: 4 entries of 0x28 bytes from 0x002baed4 */
#define CORE_PLAYER_STRIDE 0x28
#define g_PlayerPresent(i) CORE_VAR(int, 0x002baed8 + (i) * CORE_PLAYER_STRIDE)
#define g_PlayerPort(i) CORE_VAR(int, 0x002baee0 + (i) * CORE_PLAYER_STRIDE)

/* .data */
#define g_RootModeName ((char *)CORE_ADDR(0x00283b18))        /* "BGeffects" */
#define g_Cheats ((struct CoreCheat *)CORE_ADDR(0x00283d30))
#define g_CheatSequences ((int *)CORE_ADDR(0x00283d34))         /* g_Cheats[k].sequence, 0x13-int stride */
#define g_TurboCheat CORE_VAR(int, 0x00283e58)                  /* g_Cheats[3].state ("Turbo Mode") */
#define g_BoneColours ((float *)CORE_ADDR(0x00283f48))          /* 7 x 17 x 4 floats */
#define g_BoneColour ((float *)CORE_ADDR(0x00284278))           /* 16 floats: a matrix */
#define g_PreloadPackages ((const char **)CORE_ADDR(0x002842c0))
#define g_PadsPrimary CORE_VAR(int, 0x00284408)
#define g_PadChangedPort CORE_VAR(int, 0x0028440c)
#define g_FrameScale CORE_VAR(float, 0x00284410)
#define g_PressStartName CORE_VAR(const char *, 0x00284418)
#define g_PadLastPort ((int *)CORE_ADDR(0x0028441c))           /* 4 */
#define g_DefaultFirstMode CORE_VAR(struct CGameMode *, 0x002ba564)
#define g_LanguageFolder CORE_VAR(const char *, 0x002ba544)
#define g_GameRegion CORE_VAR(int, 0x002ba9c4)
#define g_Language CORE_VAR(int, 0x002bae78)
#define g_LanguageFolders ((const char **)CORE_ADDR(0x002839c0))   /* 12 */
#define g_MenuInputs ((void **)CORE_ADDR(0x002bb010))              /* 4 input maps */
#define g_PadsChannel CORE_VAR(void *, 0x002ba54c)      /* a debug channel: any message = pads changed */

/* ---------- globals of other modules (crt/core_foreign.c) */

extern float DAT_0028fda8;              /* frame time factor, 1.0 = 1/60 s */
extern float DAT_00292c84;              /* 4.0 */
extern int DAT_002ecdec;                /* pause count: +20 while a pad is missing */
extern int DAT_002cb550;                /* paused (bdisplay/engine_misc) */
extern void *DAT_002ecc74;              /* the text table object; +0x20: entries of 0x10 bytes */
extern int DAT_002db824;
extern void *DAT_002db814[2];           /* engine_misc: two blocks checked every frame */
extern unsigned char DAT_002d08a8[0xc00];  /* LAUNCH_DATA */
extern int DAT_002d0750;                /* launch data type */
extern int DAT_002d02ac;
extern uint64_t DAT_002cd7a0;           /* clock frequency */
extern uint32_t DAT_002be330[16];       /* copy of 0x002855f0 */
extern uint32_t DAT_002855f0[16];
extern int DAT_002be57c;                /* current render mode */
extern float DAT_002be588, DAT_002be58c;
extern float DAT_002ecde8;
extern float DAT_002909b0;
extern int DAT_0028198c;
extern unsigned char DAT_002ecd90[];    /* classlib_ui object (pause screen) */

#endif
