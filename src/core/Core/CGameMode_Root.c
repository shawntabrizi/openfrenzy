/*
CGAMEMODE_ROOT.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_Root.cpp

openfrenzy core module: the root game mode (always at the bottom of the mode stack) and the
debug menu. Its constructor (0x00026c60) and destructor (0x00026ce0) are in core_modes_b;
its vtable is 0x00256010 (owned by core_modes_b, which constructs it).
Decompiled from orig/default.xbe (functional, not byte-matching).
*/

#include <stdio.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"
#include "core_tweak.h"
#include "core_internal.h"

static const char root_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_Root.cpp";

/* CGameMode_Root, 0x7c bytes */
struct CGameMode_Root {
	struct CGameMode base;          /* +0x00 */
	unsigned char unknown_14[0x28]; /* +0x14 */
	unsigned char *pad;             /* +0x3c +0x14: menu button down, +0x20: 1 = debug */
	void *pkg_intro;                /* +0x40 Packages\MiniGameIntro */
	void *pkg_generic;              /* +0x44 Packages\Generic */
	void *pkg_local;                /* +0x48 Packages\Local */
	void *pkg_music;                /* +0x4c Packages\Music */
	unsigned char unknown_50[4];    /* +0x50 */
	void *music[2];                 /* +0x54 music channels: 0 current, 1 next */
	int volume[2];                  /* +0x5c */
	int target[2];                  /* +0x64 */
	int speed[2];                   /* +0x6c fade speed per frame */
	int crossfade;                  /* +0x74 */
	unsigned char unknown_78[4];    /* +0x78 */
};
_Static_assert(sizeof(struct CGameMode_Root) == 0x7c, "CGameMode_Root size");

/* the game modes made by CGameMode_Root::Pushed (core bss, used by core_modes_*) */
#define g_ModeAskAgain CORE_VAR(void *, 0x002ba550)
#define g_Mode554 CORE_VAR(void *, 0x002ba554)
#define g_Mode558 CORE_VAR(void *, 0x002ba558)
#define g_Mode55C CORE_VAR(void *, 0x002ba55c)
#define g_Mode560 CORE_VAR(void *, 0x002ba560)
#define g_Mode568 CORE_VAR(void *, 0x002ba568)
#define g_ChannelMgLoading CORE_VAR(void *, 0x002ba9a4)
#define g_ChannelCsLoading CORE_VAR(void *, 0x002ba9a8)
#define g_DebugMenuOn CORE_VAR(int, 0x002ba6a8)
#define g_Extras CORE_VAR(struct CTweakMenu *, 0x002ba6a0)

/* globals of other modules */
extern void *DAT_002b95cc, *DAT_002b95d0, *DAT_002b9c78;  /* core_modes_a/b mode objects */
extern int DAT_0028ffb8;          /* classlib_ui */
extern uint32_t DAT_00290754[];   /* classlib_ui: 6 records of 0x60 bytes, +0 a package */
extern int DAT_00290958;          /* classlib_ui: draw overlays */
extern void *DAT_002ece10;        /* classlib_ui: the font */
extern int DAT_002b9718;          /* core_modes_a: test map */
extern int DAT_002ed340;          /* classlib_fx: debris off */

#define ext_MusicStop ((void (*)(void *))FUN_000563f0)
/* 0x00056370 bsPlayAudioStream (stream, flags, volume, pan, frequency, callback, ...) */
#define ext_MusicPlay ((int (*)(void *, unsigned int, int, int, int, void *, ...))FUN_00056370)
#define ext_MusicLoop ((void (*)(void *, int))FUN_00056590)
#define ext_MusicVolume ((int (*)(void *, int, int, int))FUN_000564e0)
#define ext_MusicMaster ((void (*)(int))FUN_00056530)

/* 0x00026d40: the debug menu handler (menu event), in core_modes_b's address range and
missing from the function table. Weak: until its owner defines it, it logs like a stub. */
__attribute__((weak)) void FUN_00026d40(void *event)
{
	(void)event;
	of_unimplemented(0x00026d40, "FUN_00026d40");
}

/* 0x00027540  Core/CGameMode_Root.cpp
The debug menu ("Screens", "Games", "Debug") and the "Extras" tweak page. */
void CGameMode_Root_CreateDebugMenu(void)
{
	static const char *const zones[6] = {
		"Building Site", "Night Club", "Stadium", "Military Zone", "Harbour", "Industrial Zone",
	};
	void *route, *screens, *games, *debug;
	void *game_menus[CORE_GAME_COUNT];
	char text[0x20];
	int k, j, id;

	route = ext_MenuCreate(1);
	ext_MenuAdd(route, "Select route", 0x3f2, NULL, 0);
	ext_MenuAdd(route, "Pick minigame", 0x3f3, NULL, 0);
	ext_MenuAdd(route, "Change zone", 0x3f4, NULL, 0);
	ext_MenuAdd(route, "Futon Frenzy", 0x3f5, NULL, 0);
	ext_MenuAdd(route, "Test", 0x3f6, NULL, 0);
	screens = ext_MenuCreate(1);
	ext_MenuAdd(screens, "Main Menu", 0x3eb, NULL, 0);
	ext_MenuAdd(screens, "Character Select", 0x3ec, NULL, 0);
	ext_MenuAdd(screens, "Score Screen", 0x3ed, NULL, 0);
	ext_MenuAdd(screens, "Gamble Screen", 0x3ef, NULL, 0);
	ext_MenuAdd(screens, "Mini Game Select", 0x3f8, NULL, 0);
	ext_MenuAdd(screens, "Game Winner", 0x3f0, NULL, 0);
	ext_MenuAdd(screens, "Game Statistics", 0x3f1, NULL, 0);
	ext_MenuAdd(screens, "Team Screen", 0x3f7, NULL, 0);
	ext_MenuAdd(screens, "Map", 0, route, 0);
	ext_MenuAdd(screens, "Pop", 0x3ff, NULL, 0);
	ext_MenuAdd(screens, "", 0, NULL, 1);
	ext_MenuAdd(screens, "Quit", 0x3e9, NULL, 2);

	/* one page per game: the board zones, or the game's 4 variations (ids from 0 up) */
	id = 0;
	for (k = 0; k < CORE_GAME_COUNT; k++)
	{
		void *menu = ext_MenuCreate(1);

		game_menus[k] = menu;
		if (k == 0)
		{
			int flags = CORE_VAR(int, 0x002839f0) == 0 ? 2 : 0;

			for (j = 0; j < 6; j++)
				ext_MenuAdd(menu, zones[j], id + j, NULL, flags);
			id += 6;
		}
		else if (g_Games[k] == NULL)
			id += 4;
		else
		{
			/* 0x002839e4 + k * 16: nonzero when variation j is playable */
			const int *playable = (const int *)CORE_ADDR(0x002839e4) + k * 4;
			const char *const *variations = (const char *const *)((char *)g_Games[k] + 8);

			for (j = 0; j < 4; j++)
				ext_MenuAdd(menu, variations[j], id++, NULL, playable[j] == 0 ? 2 : 0);
		}
	}
	games = ext_MenuCreate(1);
	for (k = 0; k < CORE_GAME_COUNT; k++)
	{
		if (g_Games[k] == NULL)
			ext_MenuAdd(games, "(---)", 0, game_menus[k], 2);
		else
			ext_MenuAdd(games, g_Games[k]->name, 0, game_menus[k], 0);
	}
	ext_MenuAdd(games, "Tie Break", 999, NULL, 0);
	debug = ext_MenuCreate(1);
	ext_MenuAdd(debug, "Dump Mode Stack", 0x3f9, NULL, 0);
	ext_MenuAdd(debug, "Dump Resources", 0x3fa, NULL, 0);
	ext_MenuAdd(debug, "Dump Heap", 0x3fb, NULL, 0);
	ext_MenuAdd(debug, "Dump Profile - Func Only", 0x3fc, NULL, 0);
	ext_MenuAdd(debug, "Dump Profile - Func + Child", 0x3fd, NULL, 0);
	ext_MenuAdd(debug, "Reset Profile", 0x3fe, NULL, 0);
	ext_MenuAdd(debug, "Model Viewer", 0x3ea, NULL, 0);
	ext_MenuAdd(debug, "Calculate Ranks", 0x400, NULL, 0);
	g_DebugMenu = ext_MenuCreate(0);
	ext_MenuAdd(g_DebugMenu, "Empty", 0, NULL, 2);
	ext_MenuAdd(g_DebugMenu, "Screens", 0, screens, 0);
	ext_MenuAdd(g_DebugMenu, "Games", 0, games, 0);
	ext_MenuAdd(g_DebugMenu, "Debug", 0, debug, 0);
	ext_MenuSetRoot(g_DebugMenu);

	{
		void *memory = ext_New(0xe8, root_cpp, 0x21d, 0x1001);

		g_Extras = memory ? CTweakMenu_Ctor(memory, "Extras") : NULL;
	}
	if (g_Extras == NULL)
		return;
	CTweakMenu_AddBool(g_Extras, "Real FPS", &g_ShowFps);
	CTweakMenu_AddBool(g_Extras, "Record Demo Footage", &CORE_VAR(int, 0x002ba6a4));
	CTweakMenu_AddBool(g_Extras, "draw timerbars", &CORE_VAR(int, 0x002ba910));
	CTweakMenu_AddBool(g_Extras, "Team Mode", &CORE_VAR(int, 0x002baebc));
	CTweakMenu_AddBool(g_Extras, "Test Map", &DAT_002b9718);
	CTweakMenu_AddBool(g_Extras, "Debris Off", &DAT_002ed340);
	CTweakMenu_AddBool(g_Extras, "Draw Overlays", &DAT_00290958);
	CTweakMenu_AddBool(g_Extras, "Play Electric", &CORE_VAR(int, 0x002839b8));
	CTweakMenu_AddBool(g_Extras, "Play Enigma", &CORE_VAR(int, 0x002839bc));
	CTweakMenu_AddBool(g_Extras, "MovieCamera", &CORE_VAR(int, 0x002ba56c));
	CTweakMenu_AddInt(g_Extras, "Background R", &CORE_VAR(int, 0x002844cc), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(g_Extras, "Background G", &CORE_VAR(int, 0x002844d0), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(g_Extras, "Background B", &CORE_VAR(int, 0x002844d4), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(g_Extras, "Music Vol", &CORE_VAR(int, 0x002bae68), 10, 0, 0xff, 0);
	CTweakMenu_AddInt(g_Extras, "Sound Vol", &CORE_VAR(int, 0x002bae6c), 10, 0, 0xff, 0);
	CTweakMenu_AddInt(g_Extras, "Force Team Mode", &CORE_VAR(int, 0x002ba634), 1, -1, 1, 0);
	CTweakMenu_AddBool(g_Extras, "Force Route", &CORE_VAR(int, 0x002ba638));
	/* the six board stages (0x00283b00) and their meta levels */
	for (k = 0; k < 6; k++)
	{
		sprintf(text, "Stage %d", k + 1);
		CTweakMenu_AddInt(g_Extras, text, (int *)CORE_ADDR(0x00283b00) + k, 1, 0, 5, 6,
			*(const char **)CORE_ADDR(0x00283c70), *(const char **)CORE_ADDR(0x00283c90),
			*(const char **)CORE_ADDR(0x00283cb0), *(const char **)CORE_ADDR(0x00283cd0),
			*(const char **)CORE_ADDR(0x00283cf0), *(const char **)CORE_ADDR(0x00283d10));
	}
	/* six zones: forced games (0x002ba658) and variations (0x00283ab8), 3 of each */
	for (k = 0; k < 6; k++)
	{
		struct CTweakMenu *zone;
		void *memory;

		sprintf(text, "Zone %d", k + 1);
		memory = ext_New(0xe8, root_cpp, 0x249, 0x1001);
		zone = memory ? CTweakMenu_CtorSub(memory, g_Extras, text) : NULL;
		CTweakMenu_AddBool(zone, "Force Variations", (int *)CORE_ADDR(0x002ba63c) + k);
		for (j = 0; j < 3; j++)
		{
			sprintf(text, "Game %d", j + 1);
			CTweakMenu_AddInt(zone, text, (int *)CORE_ADDR(0x002ba658) + k * 3 + j, 1, 1, 0xc, 0xd, "",
				g_Games[1]->name, g_Games[2]->name, g_Games[3]->name, g_Games[4]->name,
				g_Games[5]->name, g_Games[6]->name, g_Games[7]->name, g_Games[8]->name,
				g_Games[9]->name, g_Games[10]->name, g_Games[11]->name, g_Games[12]->name);
		}
		for (j = 0; j < 3; j++)
		{
			sprintf(text, "Variation %d", j + 1);
			CTweakMenu_AddInt(zone, text, (int *)CORE_ADDR(0x00283ab8) + k * 3 + j, 1, 0, 3, 0);
		}
	}
}

/* 0x00027db0  Core/CGameMode_Root.cpp  scalar deleting destructor (vtable 0x00256010 [0]) */
struct CGameMode_Root *CGameMode_Root_Destroy(struct CGameMode_Root *self, int flags)
{
	((void (*)(void *))FUN_00026ce0)(self);
	if (flags & 1)
		ext_Delete(self);
	return self;
}

/* 0x00027dd0  Core/CGameMode_Root.cpp  update (vtable +0x14): the debug menu button, and the
fades of the two music channels */
void CGameMode_Root_Update(struct CGameMode_Root *self, int top)
{
	int channel;

	(void)top;
	if (*(int *)(self->pad + 0x14) != 0 && g_DebugMenuOn != 0 && *(int *)(self->pad + 0x20) == 1)
	{
		*(int *)(self->pad + 0x14) = 0;
		ext_MenuSetCallback((void (*)(void))FUN_00026d40);
		CTweakMenu_HandleSelect(0, 0);
		return;
	}
	for (channel = 0; channel < 2; channel++)
	{
		void *music = self->music[channel];

		if (music == NULL)
			continue;
		if (channel != 0 || self->crossfade == 0)
		{
			int speed = self->speed[channel];
			int volume = self->volume[channel] + (int)((double)speed * (double)DAT_0028fda8);

			self->volume[channel] = volume;
			if ((speed > 0 && volume >= self->target[channel])
				|| (speed < 0 && volume <= self->target[channel]))
			{
				self->volume[channel] = self->target[channel];
				if (self->target[channel] == 0)
				{
					ext_MusicStop(music);
					self->music[channel] = NULL;
					if (channel == 0)
						DAT_0028ffb8 = -1;
					else
					{
						self->crossfade = 0;
						if (self->music[0] != NULL)
						{
							((void (*)(void))FUN_000722a0)();
							ext_MusicPlay(self->music[0], 0, self->volume[0], 0, 0, NULL, 0);
							ext_MusicLoop(self->music[0], 1);
							ext_Nothing(NULL);   /* an empty function */
						}
					}
				}
			}
		}
		if (self->music[channel] != NULL)
			ext_MusicVolume(self->music[channel], self->volume[channel], 0, -1);
	}
	ext_MusicMaster(CORE_VAR(int, 0x002bae68));
}

/* 0x00027f10  Core/CGameMode_Root.cpp  CGameMode_Root::Pushed (vtable +0x10): make the
shared game modes, load the common packages, fonts, string table and channels */
void CGameMode_Root_Pushed(struct CGameMode_Root *self)
{
	void *memory;

	memory = ext_New(0xa4, root_cpp, 0x2dc, 0x1001);
	g_Mode558 = memory ? ((void *(*)(void *))FUN_00018800)(memory) : NULL;
	memory = ext_New(0x3b0, root_cpp, 0x2dd, 0x1001);
	g_Mode554 = memory ? ((void *(*)(void *))FUN_000142f0)(memory) : NULL;
	memory = ext_New(0x2c, root_cpp, 0x2de, 0x1001);
	DAT_002b95cc = memory ? ((void *(*)(void *))FUN_00013110)(memory) : NULL;
	memory = ext_New(0x1ac, root_cpp, 0x2df, 0x1001);
	g_ModeAskAgain = memory ? ((void *(*)(void *))FUN_00019260)(memory) : NULL;
	memory = ext_New(0x250, root_cpp, 0x2e0, 0x1001);
	g_DefaultFirstMode = memory ? ((struct CGameMode *(*)(void *))FUN_00026180)(memory) : NULL;
	memory = ext_New(0x198, root_cpp, 0x2e1, 0x1001);
	g_Mode568 = memory ? CGameMode_Team_Ctor(memory) : NULL;
	memory = ext_New(0xd0, root_cpp, 0x2e2, 0x1001);
	DAT_002b9c78 = memory ? ((void *(*)(void *))FUN_0001cd50)(memory) : NULL;
	memory = ext_New(0x84, root_cpp, 0x2e3, 0x1001);
	DAT_002b95d0 = memory ? ((void *(*)(void *))FUN_000138d0)(memory) : NULL;

	self->pkg_generic = ext_LoadPackage(0, "Packages\\Generic", 0);
	if (self->pkg_generic == NULL)
	{
		ext_DebugPrintf("CGameMode_Root::Pushed *** Could not load package 'Packages\\Generic' ***\n");
		return;
	}
	self->pkg_local = ext_LoadPackage(0, "Packages\\Local", 0);
	if (self->pkg_local == NULL)
	{
		ext_DebugPrintf("CGameMode_Root::Pushed *** Could not load package 'Packages\\Local' ***\n");
		return;
	}
	self->pkg_intro = ext_LoadPackage(0, "Packages\\MiniGameIntro", 0);
	if (self->pkg_intro == NULL)
	{
		ext_DebugPrintf("CGameMode_Root::Pushed *** Could not load package 'Packages\\MiniGameIntro' ***\n");
		return;
	}
	self->pkg_music = ext_OpenStreamPackage("Packages\\Music");
	if (self->pkg_music == NULL)
		ext_DebugPrintf("CGameMode_Root::Pushed *** Could not load package 'Packages\\Music' ***\n");
	ext_PackagePrepare(self->pkg_intro, -1, 0xdefa, 0);
	ext_PackagePrepare(self->pkg_generic, -1, 0xdefa, 0);
	ext_PackagePrepare(self->pkg_local, 0x10, 0xdefa, 0);
	DAT_002ece10 = ext_FindResource(0, "handgotl.ttf", 3, 0xdefa);
	DAT_002ecc74 = ext_FindResource(0, "stringtable.txt", 4, 0xdefa);
	memory = ext_New(0x1c4, root_cpp, 0x308, 0x1001);
	g_Mode55C = memory ? ((void *(*)(void *))FUN_00024620)(memory) : NULL;
	memory = ext_New(200, root_cpp, 0x309, 0x1001);
	g_Mode560 = memory ? ((void *(*)(void *))FUN_0001c2a0)(memory) : NULL;
	((void (*)(void))FUN_00078cb0)();
	((void (*)(void))FUN_00071b60)();
	((void (*)(void))FUN_000712b0)();
	((void (*)(int))FUN_00071880)(1);
	((void (*)(void *))FUN_00074e30)(DAT_002ecd90);
	if (!ext_ChannelExists("mgLoading"))
		ext_DebugPrintf("FAILED to create event!\n");
	else
		g_ChannelMgLoading = ext_ChannelOpen("mgLoading", 0x14, 0);
	g_PadsChannel = ext_ChannelOpen("_MemCardChange", 1, 0);
	if (!ext_ChannelExists("csLoading"))
		ext_DebugPrintf("FAILED to create event!\n");
	else
		g_ChannelCsLoading = ext_ChannelOpen("csLoading", 0x14, 0);
	self->music[0] = NULL;
	self->music[1] = NULL;
}

/* 0x000283a0  Core/CGameMode_Root.cpp  CGameMode_Root::Popped (vtable +0x0c): release the
shared modes, the meta level packages, the Extras page and the packages */
void CGameMode_Root_Popped(struct CGameMode_Root *self)
{
	int i;

	if (DAT_002b95d0 != NULL)
		ext_CGameMode_OnPop(DAT_002b95d0);
	if (DAT_002b9c78 != NULL)
		ext_CGameMode_OnPop(DAT_002b9c78);
	if (g_Mode558 != NULL)
		ext_CGameMode_OnPop(g_Mode558);
	if (g_Mode554 != NULL)
		ext_CGameMode_OnPop(g_Mode554);
	if (DAT_002b95cc != NULL)
		ext_CGameMode_OnPop(DAT_002b95cc);
	if (g_ModeAskAgain != NULL)
		ext_CGameMode_OnPop(g_ModeAskAgain);
	if (g_Mode560 != NULL)
		ext_CGameMode_OnPop(g_Mode560);
	if (g_DefaultFirstMode != NULL)
		ext_CGameMode_OnPop(g_DefaultFirstMode);
	if (g_Mode568 != NULL)
		ext_CGameMode_OnPop(g_Mode568);
	if (g_Mode55C != NULL)
		ext_CGameMode_OnPop(g_Mode55C);
	for (i = 0; i < 6; i++)
	{
		void *package = (void *)(uintptr_t)DAT_00290754[i * 0x18];

		if (package != NULL)
			ext_FreePackage(package);
	}
	if (g_Extras != NULL)
	{
		struct CTweakMenu *extras = g_Extras;

		CTweakMenu_Dtor(extras);
		ext_Delete(extras);
	}
	((void (*)(void))FUN_0007cc00)();
	((void (*)(void))FUN_00071400)();
	((void (*)(void))FUN_00071bf0)();
	if (self->pkg_generic != NULL)
	{
		ext_FreePackage(self->pkg_generic);
		self->pkg_generic = NULL;
	}
	if (self->pkg_intro != NULL)
	{
		ext_FreePackage(self->pkg_intro);
		self->pkg_intro = NULL;
	}
	if (self->pkg_local != NULL)
	{
		ext_FreePackage(self->pkg_local);
		self->pkg_local = NULL;
	}
	if (self->pkg_music != NULL)
	{
		ext_FreePackage(self->pkg_music);
		self->pkg_music = NULL;
	}
	((void (*)(int))FUN_0004d7f0)(0);
}
