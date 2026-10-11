/*
TankedUp/unplaced_mode.c  <-  gap functions of C:\BlitzParty\Game\Src\TankedUp\
(0x001d7b00-0x001d7d60; their file is not known)

CGameMode_TankedUp (vtable 0x0027bc74, 0x2c bytes): the game mode of the minigame. Each
method hands over to the one CTank_Game object (0x00312f50, tankedup_a); Pushed, Resume
and Start jump into CTank_Game code at 0x001c1a90, 0x001c1c10 and 0x001c2080 (missing from
the function table; tankedup_a defines them in TankedUp/CTank_Game.c).
Field +0x28: pop the mode when CTank_Game's update returns 0.

Also the TankedUp text reader (vtable 0x0027bca8; bumpercars_b's 0x000a5c00 initialises
it, the layout is tankedup_b.h RD_*) and the entities' animation callback 0x001d7d60.
*/

#include "tankedup_b.h"
#include "tankedup_b_data.h"
#include "tankedup_b_ext.h"

/* tankedup_a: the CTank_Game object (0x00312f50, 0x2160 bytes) */
extern unsigned char DAT_00312f50[];
#define TU_GAME_OBJ ((void *)DAT_00312f50)

/* core: the game mode stack */
extern unsigned char DAT_002ba750[];

/* the CTank_Game methods that are missing from the table (tankedup_a, CTank_Game.c) */
void FUN_001c1a90(void *game);
void FUN_001c1c10(void *game);
void FUN_001c2080(void *game);

void *TankMode_DeletingDtor(void *self, unsigned int flags);
void TankMode_Suspend(void *self);
void TankMode_Resume(void *self);
void TankMode_Popped(void *self);
void TankMode_Pushed(void *self);
void TankMode_Input(void *self, int active);
void TankMode_Update(void *self, int active);
void TankMode_Render(void *self, int active);
void TankMode_LoadingStarted(void *self);
void TankMode_Start(void *self);

/* 0x0027bc74  vtable of CGameMode_TankedUp (rebuilt from the XBE) */
const void *const vtable_0027bc74[13] = {
	(const void *)TankMode_DeletingDtor,     /* +0x00 0x001d7bc0 */
	(const void *)TankMode_Suspend,          /* +0x04 0x001d7b70 */
	(const void *)TankMode_Resume,           /* +0x08 0x001d7b60 */
	(const void *)TankMode_Popped,           /* +0x0c 0x001d7be0 */
	(const void *)TankMode_Pushed,           /* +0x10 0x001d7b30 */
	(const void *)TankMode_Input,            /* +0x14 0x001d7b80 */
	(const void *)TankMode_Update,           /* +0x18 0x001d7b90 */
	(const void *)TankMode_Render,           /* +0x1c 0x001d7c20 */
	(const void *)FUN_001ec260,              /* +0x20 */
	(const void *)CGameMode_UpdateLoading,   /* +0x24 0x00012930 */
	(const void *)CGameMode_FinishedLoading, /* +0x28 0x000128e0 */
	(const void *)TankMode_LoadingStarted,   /* +0x2c 0x001d7c30 */
	(const void *)TankMode_Start,            /* +0x30 0x001d7c40 */
};

void *TankReader_DeletingDtor(void *self, unsigned int flags);

/* 0x0027bca8  vtable of the TankedUp text reader */
const void *const vtable_0027bca8[1] = {
	(const void *)TankReader_DeletingDtor,   /* +0x00 0x001d7d40 */
};

/* 0x001d7b00  TankedUp (gap)
CGameMode_TankedUp constructor. */
void *TankMode_Ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, (char *)"TankedUp");
	*(const void *const **)self = vtable_0027bc74;
	TU_I32(self, 0x28) = 0;
	return self;
}

/* 0x001d7b20  TankedUp (gap)
CGameMode_TankedUp destructor. */
void TankMode_Dtor(void *self)
{
	*(const void *const **)self = vtable_0027bc74;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x001d7b30  TankedUp (gap)
Pushed (+0x10): the character voices, then CTank_Game::Pushed (packages). */
void TankMode_Pushed(void *self)
{
	Core_LoadCharacterVoices(1);
	TU_I32(self, 0x28) = 0;
	DAT_00315988 = NULL;
	FUN_001c1a90(TU_GAME_OBJ);
}

/* 0x001d7b60  TankedUp (gap)
Resume (+0x08): CTank_Game's resume (the near and far planes and the input). */
void TankMode_Resume(void *self)
{
	(void)self;
	FUN_001c1c10(TU_GAME_OBJ);
}

/* 0x001d7b70  TankedUp (gap)
Suspend (+0x04): CTank_Game releases the input. */
void TankMode_Suspend(void *self)
{
	(void)self;
	CTankGame_ReleaseInput((struct CTank_Game *)TU_GAME_OBJ);
}

/* 0x001d7b80  TankedUp (gap)
Input (+0x14). */
void TankMode_Input(void *self, int active)
{
	(void)self;
	CTankGame_Input((struct CTank_Game *)TU_GAME_OBJ, active);
}

/* 0x001d7b90  TankedUp (gap)
Update (+0x18): one frame of CTank_Game; when it ends and +0x28 is set, the mode is
popped. */
void TankMode_Update(void *self, int active)
{
	if (CTankGame_Update((struct CTank_Game *)TU_GAME_OBJ, active) == 0 && TU_I32(self, 0x28) != 0)
		CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, 1);
}

/* 0x001d7bc0  TankedUp (gap)
The deleting destructor (+0x00). */
void *TankMode_DeletingDtor(void *self, unsigned int flags)
{
	TankMode_Dtor(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x001d7be0  TankedUp (gap)
Popped (+0x0c): frees CTank_Game and prints the free 3D sound buffers. */
void TankMode_Popped(void *self)
{
	DSCAPS caps;

	(void)self;
	CTankGame_Free((struct CTank_Game *)TU_GAME_OBJ);
	TU_DEBUG("this house is clean...\n");
	IDirectSound_GetCaps((struct IDirectSound *)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", caps.dwFree3DBuffers);
}

/* 0x001d7c20  TankedUp (gap)
Render (+0x1c). */
void TankMode_Render(void *self, int active)
{
	(void)self;
	CTankGame_Render((struct CTank_Game *)TU_GAME_OBJ, active);
}

/* 0x001d7c30  TankedUp (gap)
LoadingStarted (+0x2c): CTank_Game's one-time set-up. */
void TankMode_LoadingStarted(void *self)
{
	(void)self;
	CTankGame_InitGameOneTime((struct CTank_Game *)TU_GAME_OBJ);
}

/* 0x001d7c40  TankedUp (gap)
Start (+0x30): sets bit 0 of +0x10, the planes 40-80000, then CTank_Game's start. */
void TankMode_Start(void *self)
{
	TU_U32(self, 0x10) = TU_U32(self, 0x10) | 1;
	bdSetNearFar(40.0f, 80000.0f);
	FUN_001c2080(TU_GAME_OBJ);
}

/* 0x001d7c70  TankedUp (gap)
The text reader's constructor. */
void TankReader_Ctor(void *self)
{
	*(const void *const **)self = vtable_0027bca8;
}

/* 0x001d7c80  TankedUp (gap)
The text reader's destructor. */
void TankReader_Dtor(void *self)
{
	*(const void *const **)self = vtable_0027bca8;
}

/* 0x001d7c90  TankedUp (gap)
Reads the next line into `buffer` and returns it without its leading white space. The
reader then skips the rest of the following line up to its line end (so the files need
CR LF line ends; this is the original behaviour) and counts one line. */
char *TankReader_GetLine(void *self, char *buffer)
{
	char *out = buffer;
	char *cur;

	while (TU_U32(self, RD_CUR) < TU_U32(self, RD_END)) {
		cur = TU_AT(self, RD_CUR, char *);
		if (*cur == '\n' || *cur == '\r')
			break;
		*out++ = *cur;
		TU_AT(self, RD_CUR, char *) = cur + 1;
	}
	*out = 0;
	cur = TU_AT(self, RD_CUR, char *) + 1;
	TU_AT(self, RD_CUR, char *) = cur;
	while (*buffer != 0 && (*buffer == ' ' || *buffer == '\r' || *buffer == '\n' || *buffer == '\t'))
		buffer++;
	if (cur < TU_AT(self, RD_END, char *)) {
		while (cur < TU_AT(self, RD_END, char *) && *cur != '\n' && *cur != '\r') {
			cur++;
			TU_AT(self, RD_CUR, char *) = cur;
		}
		while (cur < TU_AT(self, RD_END, char *) && (*cur == '\n' || *cur == '\r')) {
			cur++;
			TU_AT(self, RD_CUR, char *) = cur;
		}
	}
	TU_I32(self, RD_LINE) = TU_I32(self, RD_LINE) + 1;
	return buffer;
}

/* 0x001d7d40  TankedUp (gap)
The text reader's deleting destructor. */
void *TankReader_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0027bca8;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x001d7d60  TankedUp (gap; not in the function table)
The animation callback of entities with an ANIMATION: when the animation ends (reason 0)
the entity (context) gets +0x964 = 1. */
int FUN_001d7d60(int reason, void *instance, void *node, void *anim, void *context)
{
	(void)instance;
	(void)node;
	(void)anim;
	if (reason == 0)
		TU_I32(context, 0x964) = 1;
	return 1;
}
