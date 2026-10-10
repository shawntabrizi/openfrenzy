/*
MENUS_COMMON.C  <-  C:\BlitzParty\Game\Src\Core\menus_common.cpp

openfrenzy core module: CMenuMode, the base of the front-end screens (a CGameMode with
button repeat and an animated menu character), vtable 0x002577d0. Decompiled from
orig/default.xbe (functional, not byte-matching).

Only 0x00030210 is tagged with this file in the source map; 0x0002fe60-0x00030460 use the
same class layout and globals (0x002bb008-0x002bb024), so they are placed here.
0x00030480 is XAPI code (the launch-data writer of XLaunchNewImage), see its comment.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"
#include "core_modes.h"

static const char menus_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\menus_common.cpp";

extern const void *const vtable_002577d0[14];

/* the button-repeat loop of 0x0002fe60 for players FIRST.. */
static void CMenuMode_RepeatButtons(struct CMenuMode *self, int first)
{
	int player, button;

	for (player = first; player < g_PlayerCount; player++)
	{
		if ((self->flags & 2) != 0 && g_PlayerPresent(player) == 0)
			continue;
		for (button = 0; button < 9; button++)
		{
			int frames = *(int *)(g_PlayerButton(player, button) + 0x20);
			int fire = 0;

			if (frames == 0)
			{
				self->held[player][button] = 0;
				if (frames > self->repeat_delay[button] && frames % self->repeat_rate[button] == 0)
					fire = 1;
			}
			else if (self->held[player][button] == 0)
			{
				/* a held button repeats only if it was released since the screen started */
				if (frames == 1)
					fire = 1;
				else if (frames > self->repeat_delay[button] && frames % self->repeat_rate[button] == 0)
					fire = 1;
			}
			if (fire)
			{
				if (((int (*)(struct CMenuMode *, int, int))self->vtbl[13])(self, button, player) != 0)
					return;
				break;
			}
		}
	}
}

/* 0x0002fe60  (Core, menus_common.cpp) CMenuMode update (vtable +0x14): press and repeat of
the 9 buttons of each player -> vtable +0x34 (button, player); a nonzero result stops */
void CMenuMode_Update(struct CMenuMode *self, int top)
{
	if (top == 0)
		return;
	CMenuMode_RepeatButtons(self, 0);
}

/* 0x0002fe6c  (Core) not a function: Ghidra's split of 0x0002fe60 at the loop head (the
player in EBX, the flags from the compare). Nothing calls it. STUB: it logs and returns. */
void FUN_0002fe6c(int *self)
{
	(void)self;
	of_unimplemented(0x0002fe6c, "FUN_0002fe6c (interior of 0x0002fe60)");
}

/* 0x0002ff40  (Core, menus_common.cpp) send the menu character out with animation ANIM
("out" when NULL); it keeps playing as the old character */
void CMenuMode_ActorOut(struct CMenuMode *self, const char *anim)
{
	void *found;

	if (g_MenuActor == NULL)
		return;
	if (g_MenuActorOld != NULL)
		ext_ActorFree(g_MenuActorOld);
	if (anim == NULL)
		anim = "out";
	found = ext_ActorFindAnim(g_MenuActor, anim);
	if (found != NULL)
		ext_ActorPlay(g_MenuActor, *(void **)(g_MenuActor + 0x134), found, 0, -1, 0, 0, 0);
	g_MenuActorOld = g_MenuActor;
	g_MenuActor = NULL;
	self->actor = NULL;
}

/* 0x0002ffd0  (Core, menus_common.cpp) free both menu characters */
void CMenuMode_FreeActors(void)
{
	if (g_MenuActor != NULL)
		ext_ActorFree(g_MenuActor);
	g_MenuActor = NULL;
	if (g_MenuActorOld != NULL)
		ext_ActorFree(g_MenuActorOld);
	g_MenuActorOld = NULL;
}

/* 0x00030000  (Core, menus_common.cpp) process (vtable +0x18): animate the menu characters;
the old one is freed when its animation ends. The original is thiscall, but `this` is
not used: the port's prototype has only TOP (other modules call it so); the vtable slot
uses CMenuMode_ProcessV. */
void CMenuMode_Process(int top)
{
	if (top == 0)
		return;
	if (g_MenuActorOld != NULL
		&& !ext_ActorAnimPlaying(g_MenuActorOld, *(void **)(g_MenuActorOld + 0x134), 0, 0))
	{
		ext_ActorFree(g_MenuActorOld);
		g_MenuActorOld = NULL;
	}
	if (g_MenuActor != NULL)
	{
		ext_ActorUpdate(g_MenuActor, *(void **)(g_MenuActor + 0x134), DAT_0028fda8 * 60.0f, 0);
		ext_ActorSetVisible(g_MenuActor, 1);
	}
	if (g_MenuActorOld != NULL)
	{
		ext_ActorUpdate(g_MenuActorOld, *(void **)(g_MenuActorOld + 0x134), DAT_0028fda8 * 60.0f, 0);
		ext_ActorSetVisible(g_MenuActorOld, 1);
	}
}

/* the vtable form of 0x00030000 (self, top) */
static void CMenuMode_ProcessV(struct CMenuMode *self, int top)
{
	(void)self;
	CMenuMode_Process(top);
}

/* 0x00030090  (Core, menus_common.cpp) CMenuMode::CMenuMode(name) */
struct CMenuMode *CMenuMode_Ctor(struct CMenuMode *self, const char *name)
{
	int player, button;

	((void *(*)(void *, const char *))FUN_00012800)(self, name);
	self->vtbl = vtable_002577d0;
	self->actor = NULL;
	if (g_MenuModeCount == 0)
	{
		for (player = 0; player < g_PlayerCount; player++)
			Core_CreatePlayerInput(player);
	}
	/* buttons already down do not count as presses on this screen */
	for (button = 0; button < 9; button++)
	{
		for (player = 0; player < g_PlayerCount; player++)
		{
			self->held[player][button] = *(int *)(g_PlayerButton(player, button) + 0x20) != 0;
			((void (*)(void *))FUN_00048d40)(g_PlayerButton(player, button));
		}
	}
	g_MenuModeCount++;
	for (button = 0; button < 9; button++)
	{
		self->repeat_delay[button] = 0x1e;
		self->repeat_rate[button] = 10;
	}
	return self;
}

/* 0x00030190  (Core, menus_common.cpp) CMenuMode::~CMenuMode */
void CMenuMode_Dtor(struct CMenuMode *self)
{
	int i;

	self->vtbl = vtable_002577d0;
	g_MenuModeCount--;
	if (g_MenuModeCount == 0)
	{
		CMenuMode_FreeActors();
		for (i = 0; i < 4; i++)
		{
			if (g_MenuInputMaps[i] != NULL)
				ext_InputMapFree(g_MenuInputMaps[i]);
		}
	}
	((void (*)(void *))FUN_00012860)(self);
}

/* 0x00030210  Core/menus_common.cpp  a new menu character from MODEL, entering with
animation ANIM ("in" when NULL); the current one goes out */
void CMenuMode_SetActor(struct CMenuMode *self, void *model, const char *anim)
{
	void *found;

	if (self->actor != NULL)
		CMenuMode_ActorOut(self, NULL);
	if (model == NULL)
		return;
	g_MenuActor = ext_ActorCreate(NULL, model, 2, menus_cpp, 0x10a);
	self->actor = g_MenuActor;
	*(uint32_t *)(g_MenuActor + 0xcc) = 0;
	*(uint32_t *)(g_MenuActor + 0xdc) = 0;
	ext_ActorSetVisible(g_MenuActor, 1);
	ext_ActorSetMatrixMode(g_MenuActor, *(void **)(g_MenuActor + 0x134), 0, 1);
	ext_ActorSetFlags(g_MenuActor, *(void **)(g_MenuActor + 0x134), 1, 0);
	if (anim == NULL)
		anim = "in";
	found = ext_ActorFindAnim(self->actor, anim);
	if (found != NULL)
	{
		unsigned char *actor = self->actor;

		ext_ActorPlay(actor, *(void **)(actor + 0x134), found, 0, -1, 0, 0, 0);
		ext_ActorUpdate(actor, *(void **)(actor + 0x134), 0.0f, 0);
		ext_ActorSetVisible(actor, 1);
	}
}

/* 0x00030310  (Core, menus_common.cpp) draw (vtable +0x1c): the menu characters */
void CMenuMode_Draw(struct CMenuMode *self, int top)
{
	static const float eye[4] = { 0.0f, 0.0f, -320.0f, 1.0f };
	static const float at[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	unsigned char state[0x78];

	if (top == 0 || (g_MenuActor == NULL && g_MenuActorOld == NULL))
		return;
	ext_SaveState(state);
	ext_SetRenderMode(1);
	ext_SetRenderState(5, 1, 0);
	ext_SetRenderState(8, 0, 0);
	ext_SetRenderState(3, g_MenuStencil, 0);
	ext_SetRenderState(6, 1, 0);
	ext_SetRenderState(4, 1, 0);
	ext_SetRenderState(10, 0, 0);
	ext_SetCamera(eye, at, 0);
	ext_SetClipPlanes(1.0f, 8000.0f);
	ext_SetFov(1.5707964f, 0);
	ext_ApplyCamera();
	if (g_MenuActor != NULL)
	{
		ext_SetRenderState(7, (self->flags & 4) ? 1 : 3, 0);
		ext_ActorDraw(g_MenuActor, 7);
	}
	if (g_MenuActorOld != NULL)
	{
		ext_SetRenderState(7, (self->flags & 4) ? 1 : 3, 0);
		ext_ActorDraw(g_MenuActorOld, 7);
	}
	ext_RestoreState(state);
}

/* 0x00030460  (Core, menus_common.cpp) scalar deleting destructor (vtable [0]) */
struct CMenuMode *CMenuMode_Destroy(struct CMenuMode *self, int flags)
{
	CMenuMode_Dtor(self);
	if (flags & 1)
		ext_Delete(self);
	return self;
}

/* the kernel's LaunchDataPage (a page that survives a reboot). The port never reboots:
one page for the process. */
static uint32_t *core_launch_data_page;

/* 0x00030480  (XAPI, after menus_common.cpp in the image) write the launch data page for
XLaunchNewImage (its only caller, 0x000305be, is XAPI code the port replaces): TYPE, FLAGS,
the 3072-byte DATA (unless TYPE is -1) and "DEVICE;PATH" (device "\Device\Cdrom0" when
NULL). Returns 0 or STATUS_NO_MEMORY. MmPersistContiguousMemory is not done (no reboot). */
int Core_WriteLaunchData(const char *path, const char *device, int type, unsigned int flags,
	const unsigned int *data)
{
	char *text;
	int length;

	if (core_launch_data_page == NULL)
		core_launch_data_page = MmAllocateContiguousMemory(0x1000);
	if (core_launch_data_page == NULL)
		return (int)0xc0000017;
	if (device == NULL)
		device = "\\Device\\Cdrom0";
	memset(core_launch_data_page, 0, 0x1000);
	core_launch_data_page[0] = (uint32_t)type;
	core_launch_data_page[1] = flags;
	if (type != -1)
		memcpy(core_launch_data_page + 0x100, data, 0x300 * 4);
	if (path != NULL)
	{
		text = (char *)(core_launch_data_page + 2);
		lstrcpynA(text, device, 0x207);
		length = (int)strlen(text);
		text[length] = ';';
		length++;
		lstrcpynA(text + length, path, 0x208 - length);
	}
	return 0;
}

/* R6025: a pure virtual function was called (the CRT's _purecall, 0x001e0323) */
static void CMenuMode_PureCall(void)
{
	fprintf(stderr, "R6025\n- pure virtual function call\n");
	abort();
}

/* 0x002577d0: the CMenuMode vtable */
const void *const vtable_002577d0[14] = {
	(const void *)CMenuMode_Destroy,   /* [0] */
	(const void *)FUN_001d5950,        /* [1] */
	(const void *)FUN_001d5950,        /* [2] */
	(const void *)FUN_001d5950,        /* [3] */
	(const void *)FUN_001d5950,        /* [4] */
	(const void *)CMenuMode_Update,    /* [5] */
	(const void *)CMenuMode_ProcessV,  /* [6] */
	(const void *)CMenuMode_Draw,      /* [7] */
	(const void *)FUN_001ec260,        /* [8] */
	(const void *)FUN_00012930,        /* [9] */
	(const void *)FUN_000128e0,        /* [10] */
	(const void *)FUN_001d5950,        /* [11] */
	(const void *)FUN_001d5950,        /* [12] */
	(const void *)CMenuMode_PureCall,  /* [13] button: pure virtual */
};
