/*
Rowing/unplaced_gamemode.c  <-  C:\BlitzParty\Game\Src\Rowing\ (file not known)

CGameMode_Rowing (vtable 0x00276180, the core game mode "Rowing"; 0x38 bytes): a thin
wrapper whose methods run the static CRowing_Game object at 0x0030f0b0 (rowing_a). It keeps
core's light colour while the game runs.

0x001b60c0-0x001b6200 lie between CRowing_Tokens.cpp and CRowing_objects.cpp (gap
functions; their file is not known). The Resume, Update, Process and Draw methods are
thunks "mov ecx, 0x0030f0b0; jmp <rowing_a>" into four rowing_a entry points that are
missing from the function table (0x0019d8e0, 0x0019d900, 0x001a2890, 0x0019dc80); rowing_b
calls them by their FUN_ names, with weak stand-ins below until rowing_a defines them.
*/

#include "rowing_b.h"

/* core's light colour (red, green, blue, alpha) */
extern unsigned int DAT_002ba940, DAT_002ba944, DAT_002ba948, DAT_002ba94c;
/* the DirectSound object (engine_misc) */
extern void *DAT_002db808;

/* 0x00276180  vtable of CGameMode_Rowing */
const void *const vtable_00276180[13] = {
	(const void *)CGameModeRowing_DeletingDtor,     /* +0x00 */
	(const void *)FUN_0019d580,                     /* +0x04 suspend (rowing_a) */
	(const void *)CGameModeRowing_Resume,           /* +0x08 */
	(const void *)CGameModeRowing_Popped,           /* +0x0c */
	(const void *)CGameModeRowing_Pushed,           /* +0x10 */
	(const void *)CGameModeRowing_Update,           /* +0x14 */
	(const void *)CGameModeRowing_Process,          /* +0x18 */
	(const void *)CGameModeRowing_Draw,             /* +0x1c */
	(const void *)FUN_001ec260,                     /* +0x20 */
	(const void *)CGameMode_UpdateLoading,          /* +0x24 */
	(const void *)CGameMode_FinishedLoading,        /* +0x28 */
	(const void *)CGameModeRowing_LoadingStarted,   /* +0x2c */
	(const void *)CGameModeRowing_Start,            /* +0x30 */
};

/* 0x001b60c0  Rowing (gap; file not known) */
void *CGameModeRowing_ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, "Rowing");
	RB_PTR(self, 0) = (void *)vtable_00276180;
	return self;
}

/* 0x001b60e0  Rowing (gap; file not known) */
void CGameModeRowing_dtor(void *self)
{
	RB_PTR(self, 0) = (void *)vtable_00276180;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x001b60f0  Rowing (gap; file not known)
Loads the characters' voices and the game's packages (background loading). */
void CGameModeRowing_Pushed(void *self)
{
	Core_LoadCharacterVoices(1);
	RB_CALL(FUN_001a0290, void (*)(void *))(RW_GAME);
	RB_I32(self, 0x14) = 1;
}

/* 0x001b6110  Rowing (gap; file not known) thunk into rowing_a */
void CGameModeRowing_Resume(void *self)
{
	(void)self;
	FUN_0019d8e0(RW_GAME);
}

/* 0x001b6120  Rowing (gap; file not known) thunk into rowing_a */
void CGameModeRowing_Update(void *self, int top)
{
	(void)self;
	FUN_0019d900(RW_GAME, top);
}

/* 0x001b6130  Rowing (gap; file not known)
Runs the game's frame once its packages are loaded. */
void CGameModeRowing_Process(void *self, int top)
{
	if (RB_I32(self, 0x14) != 0)
		return;
	FUN_001a2890(RW_GAME, top);
}

/* 0x001b6150  Rowing (gap; file not known)
Draws the game once its packages are loaded. */
void CGameModeRowing_Draw(void *self, int top)
{
	if (RB_I32(self, 0x14) != 0)
		return;
	FUN_0019dc80(RW_GAME, top);
}

/* 0x001b6170  Rowing (gap; file not known) thunk into rowing_a */
void CGameModeRowing_LoadingStarted(void *self)
{
	(void)self;
	RB_CALL(FUN_001a0dd0, void (*)(void *))(RW_GAME);
}

/* 0x001b6180  Rowing (gap; file not known)
Starts the game, draws over the modes below, keeps core's light colour and sets it to
black (render state 8 off). */
void CGameModeRowing_Start(void *self)
{
	RB_CALL(FUN_001a1ac0, void (*)(void *))(RW_GAME);
	RB_U32(self, 0x10) = RB_U32(self, 0x10) | 1;
	RB_U32(self, 0x28) = DAT_002ba940;
	RB_U32(self, 0x2c) = DAT_002ba944;
	RB_U32(self, 0x30) = DAT_002ba948;
	RB_U32(self, 0x34) = DAT_002ba94c;
	DAT_002ba948 = 0;
	DAT_002ba944 = 0;
	DAT_002ba940 = 0;
	DAT_002ba94c = 0xff;
	bdSetRenderState(8, 0, 0);
}

/* 0x001b61e0  Rowing (gap; file not known) scalar deleting destructor */
void *CGameModeRowing_DeletingDtor(void *self, unsigned int flags)
{
	CGameModeRowing_dtor(self);
	if (flags & 1)
		RB_DELETE(self);
	return self;
}

/* 0x001b6200  Rowing (gap; file not known)
Ends the game (frees it; after it ran, restores core's light colour) and logs the free 3D
sound buffers. */
void CGameModeRowing_Popped(void *self)
{
	DSCAPS caps;

	if (RB_I32(self, 0x04) == 0) {
		RB_CALL(FUN_001a2330, void (*)(void *))(RW_GAME);
	} else {
		RB_CALL(FUN_001a23e0, void (*)(void *))(RW_GAME);
		DAT_002ba940 = RB_U32(self, 0x28);
		DAT_002ba944 = RB_U32(self, 0x2c);
		DAT_002ba948 = RB_U32(self, 0x30);
		DAT_002ba94c = RB_U32(self, 0x34);
	}
	IDirectSound_GetCaps((struct IDirectSound *)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", (int)caps.dwFree3DBuffers);
}

/* ---------- weak stand-ins for the rowing_a entry points missing from the table
   (rowing_a's definitions replace them at link time) */

__attribute__((weak)) void FUN_0019d8e0(void *game)
{
	(void)game;
	of_unimplemented(0x0019d8e0, "FUN_0019d8e0 (rowing_a, CRowing_Game resume)");
}

__attribute__((weak)) void FUN_0019d900(void *game, int top)
{
	(void)game;
	(void)top;
	of_unimplemented(0x0019d900, "FUN_0019d900 (rowing_a, CRowing_Game update)");
}

__attribute__((weak)) void FUN_001a2890(void *game, int top)
{
	(void)game;
	(void)top;
	of_unimplemented(0x001a2890, "FUN_001a2890 (rowing_a, CRowing_Game process)");
}

__attribute__((weak)) void FUN_0019dc80(void *game, int top)
{
	(void)game;
	(void)top;
	of_unimplemented(0x0019dc80, "FUN_0019dc80 (rowing_a, CRowing_Game draw)");
}
