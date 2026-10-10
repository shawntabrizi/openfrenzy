/*
RollerBall/unplaced_gamemode.c  (module rollerball_b)

CGameMode_RollerBall (0x28 bytes, vtable 0x00274b18, name "RollerBall"), 0x00198600-
0x001986c0: the game mode that rollerball_a's CRollerBall_Game::Start pushes on core's
mode stack. Its methods only forward to the game object (DAT_0030ef04) of rollerball_a
(most are "mov ecx, game; jmp" thunks). The original file is not known.

CGameMode virtual functions (core_globals.h): [0] destroy, [1] suspend, [2] resume,
[3] exit, [4] enter, [5] update, [6] process, [7] draw, [8] post draw, then core's
loading functions and two RollerBall ones (+0x2c, +0x30).
*/

#include "rollerball_b.h"

/* rollerball_a functions missing from the function table (defined in
src/rollerball_a/RollerBall/CRollerball_Game.c; prototypes as rollerball_a.h has them) */
void FUN_00187c70(void *self, int arg);
void FUN_00188fa0(void *self);
void FUN_001890e0(void *self, int run);
void FUN_001892e0(void *self, int draw);

/* 0x00274b18  CGameMode_RollerBall vtable */
const void *const rbb_vtable_00274b18[13] = {
	(const void *)FUN_001986c0, /* +0x00 scalar deleting destructor */
	(const void *)FUN_00198680, /* +0x04 suspend */
	(const void *)FUN_00198680, /* +0x08 resume */
	(const void *)FUN_00198650, /* +0x0c exit */
	(const void *)FUN_00198630, /* +0x10 enter */
	(const void *)FUN_00198690, /* +0x14 update */
	(const void *)FUN_001986a0, /* +0x18 process */
	(const void *)FUN_001986b0, /* +0x1c draw */
	(const void *)FUN_001ec260, /* +0x20 post draw (empty) */
	(const void *)FUN_00012930, /* +0x24 core: CGameMode_UpdateLoading */
	(const void *)FUN_000128e0, /* +0x28 core: CGameMode_FinishedLoading */
	(const void *)FUN_00198660, /* +0x2c */
	(const void *)FUN_00198670, /* +0x30 */
};

/* 0x00198600  (file unknown)
Constructor. */
void *CGameModeRollerBall_Ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, "RollerBall");
	*(const void *const **)self = rbb_vtable_00274b18;
	return self;
}

/* 0x00198620  (file unknown)
Destructor. */
void CGameModeRollerBall_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274b18;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x00198630  (file unknown)
Pushed: load the game's packages, then the mode's +0x14 flag is set. */
void CGameModeRollerBall_Enter(void *self)
{
	CRollerBallGame_LoadPackages(DAT_0030ef04);
	RBB_I(self, 0x14) = 1;
}

/* 0x00198650  (file unknown)
Popped: shut the game down. */
void CGameModeRollerBall_Exit(void *self)
{
	(void)self;
	CRollerBallGame_Shutdown(DAT_0030ef04);
}

/* 0x00198660  (file unknown)
Loading started: CRollerBall_Game::Initialise. */
void CGameModeRollerBall_Initialise(void *self)
{
	(void)self;
	CRollerBallGame_Initialise(DAT_0030ef04);
}

/* 0x00198670  (file unknown)
Loading finished: mark the mode (flags +0x10 bit 0) and start the game (rollerball_a
0x00188fa0). */
void CGameModeRollerBall_Start(void *self)
{
	RBB_U(self, 0x10) |= 1;
	FUN_00188fa0(DAT_0030ef04);
}

/* 0x00198680  (file unknown)
Suspend and resume: nothing (the game's empty function 0x001d5950). */
void CGameModeRollerBall_Nothing(void *self)
{
	(void)self;
	FUN_001d5950();
}

/* 0x00198690  (file unknown)
Update: rollerball_a 0x00187c70. */
void CGameModeRollerBall_Update(void *self, int top)
{
	(void)self;
	FUN_00187c70(DAT_0030ef04, top);
}

/* 0x001986a0  (file unknown)
Process: the game's frame (rollerball_a 0x001890e0, "CRollerBall_Game::PollPreRender"). */
void CGameModeRollerBall_Process(void *self, int top)
{
	(void)self;
	FUN_001890e0(DAT_0030ef04, top);
}

/* 0x001986b0  (file unknown)
Draw: the game's render (rollerball_a 0x001892e0). */
void CGameModeRollerBall_Draw(void *self, int top)
{
	(void)self;
	FUN_001892e0(DAT_0030ef04, top);
}

/* 0x001986c0  (file unknown)
Scalar deleting destructor. */
void *CGameModeRollerBall_DeletingDtor(void *self, unsigned int flags)
{
	CGameModeRollerBall_Dtor(self);
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}
