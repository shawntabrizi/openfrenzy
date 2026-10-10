/*
CORE_MODES_A_MODES.H

Game modes of module core_modes_a: the CGameMode base class (0x00012800)
and the front-end modes of CGameMode_AskAgain.cpp, CGameMode_CharacterSelect.cpp,
CGameMode_Credits.cpp, CGameMode_DemoSplash.cpp, CGameMode_FullGame.cpp,
CGameMode_GameSizeMenu.cpp and CGameMode_GameWinner.cpp.

Owner: core_modes_a. Layouts keep the original offsets.

CGameMode vtable (13 slots, 14 for menu modes), as the mode stack (core,
CGameModeStack 0x002ba750) calls it:
  +0x00 scalar deleting destructor (self, flags)
  +0x04 Suspend(self)          another mode was pushed on top, or this one popped
  +0x08 Resume(self)           became the top again
  +0x0c Popped(self)
  +0x10 Pushed(self)
  +0x14 Input(self, int active)
  +0x18 Update(self, int active)
  +0x1c Render(self, int active)
  +0x20 (self, int)
  +0x24 UpdateLoading(self)
  +0x28 FinishedLoading(self)
  +0x2c LoadingStarted(self)
  +0x30 (self)
  +0x34 HandleButton(self, int button, int player)   (menu modes)
*/

#ifndef CORE_MODES_A_MODES_H
#define CORE_MODES_A_MODES_H

#include "core_modes_a.h"

/* CGameMode (vtable 0x002538a4; ctor 0x00012800, dtor 0x00012860) */
struct CGameMode {
	const void *const *vtbl; /* +0x00 */
	int unk04;               /* +0x04 */
	const char *name;        /* +0x08 */
	int refCount;            /* +0x0c */
	unsigned int flags;      /* +0x10 1: draws over the modes below, 2: menu input, 4: ... */
	int loadState;           /* +0x14 2: background package loading */
	int unk18;               /* +0x18 */
	struct CMA_List list;    /* +0x1c */
};                           /* 0x28 */

/* the menu mode of module core (vtable 0x002577d0; ctor 0x00030090, dtor 0x00030190):
   button auto-repeat per player */
struct CGameModeMenu {
	struct CGameMode base;          /* +0x00 */
	int unk28;                      /* +0x28 */
	unsigned char held[4][9];       /* +0x2c */
	int repeatDelay[9];             /* +0x50 */
	int repeatRate[9];              /* +0x74 */
};                                  /* 0x98 */

/* CGameMode_AskAgain (vtable 0x00253928): "play again?" with two options */
struct CGameMode_AskAgain {
	struct CGameModeMenu menu;      /* +0x00 */
	struct CBGTypeText *options[2]; /* +0x98 */
	struct CBGTypeTextBox *question;/* +0xa0 */
	struct CMA_List overlays;       /* +0xa4 */
	int unkb0;                      /* +0xb0 */
	int unkb4;                      /* +0xb4 */
	int font;                       /* +0xb8 */
	int selection;                  /* +0xbc */
};                                  /* 0xc0 */

/* CGameMode_AttractMode (vtable 0x002539c0) */
struct CGameMode_AttractMode {
	struct CGameMode base;          /* +0x00 */
	int playVideo;                  /* +0x28 */
};

/* CBGEffectGroup (vtable 0x00253bac): a CBGEffect that owns a list of effects */
struct CBGEffectGroup {
	struct CBGEffect base;          /* +0x00 */
	struct CMA_List children;       /* +0x10 */
	unsigned char deleteFinished;   /* +0x1c */
};                                  /* 0x20 */

/* CGameMode_BGEffects (vtable 0x00253bc4): the 3D background of the menus */
struct CGameMode_BGEffects {
	struct CGameMode base;          /* +0x00 */
	float farPlane;                 /* +0x28 250 */
	float fogStart;                 /* +0x2c 100 */
	float fov;                      /* +0x30 */
	float eye[4];                   /* +0x34 */
	float target[4];                /* +0x44 */
	int package;                    /* +0x54 */
	unsigned char active;           /* +0x58 */
	int font1;                      /* +0x5c handgotb.ttf */
	int font2;                      /* +0x60 ocraext.ttf */
	struct CBGEffectGroup group;    /* +0x64 */
};                                  /* 0x84 */

/* a player panel of the character select screen (0x44 bytes; ctor 0x00013bd0) */
struct CMA_CharSelPanel {
	void *overlay;                    /* +0x00 COverlay of the character model */
	void *shown;                      /* +0x04 the overlay that slides out */
	struct CBGTypeText *joinText;     /* +0x08 text 0x105 */
	struct CBGTypeText *readyText;    /* +0x0c text 0x106 */
	struct CBGTypeText *nameText;     /* +0x10 text 0x187 + player */
	struct CBGTypeText *charText;     /* +0x14 the character's name */
	struct CBGTypeTextBox *infoBox;   /* +0x18 */
	struct CMA_List overlays;         /* +0x1c */
	int unk28;                        /* +0x28 */
	struct CBGEffectGroup *group;     /* +0x2c */
	float x;                          /* +0x30 viewport position */
	float y;                          /* +0x34 */
	int unk38;                        /* +0x38 */
	int unk3c;                        /* +0x3c */
	float animDelay;                  /* +0x40 */
};

/* CGameMode_CharacterSelect (vtable 0x00253dc4) */
struct CGameMode_CharacterSelect {
	struct CGameModeMenu menu;        /* +0x000 */
	void *tweakMenu;                  /* +0x098 CTweakMenu "CharSelect" (core) */
	int camStage;                     /* +0x09c celebration camera: player 0-3, 4 group, 5 done */
	int cheerStarted;                 /* +0x0a0 */
	float cheerTimer;                 /* +0x0a4 */
	float cheerVolume[5];             /* +0x0a8 */
	int cheerVoice[5];                /* +0x0bc */
	int lastCheer;                    /* +0x0d0 */
	int actors[4];                    /* +0x0d4 the players' characters (celebration) */
	int unke4;                        /* +0x0e4 */
	void *charOverlay[6];             /* +0x0e8 character portraits */
	void *charHighlight[6];           /* +0x100 */
	float pulsePhase[6];              /* +0x118 */
	void *marker[6][4];               /* +0x130 player markers on each portrait */
	void *playerOverlay[4];           /* +0x190 */
	struct CMA_List overlays;         /* +0x1a0 */
	struct CBGTypeTextBox *menuText[4]; /* +0x1ac */
	int inGame[4];                    /* +0x1bc */
	int selection[4];                 /* +0x1cc */
	int owner[6];                     /* +0x1dc the player that took each character, -1 */
	int font1;                        /* +0x1f4 handgotb.ttf */
	int font2;                        /* +0x1f8 handgotl.ttf */
	struct CBGTypeTextBox *prompt;    /* +0x1fc */
	int celebrate[4];                 /* +0x200 */
	int arrowCount[4];                /* +0x210 */
	int created;                      /* +0x220 */
	int menuState;                    /* +0x224 */
	int confirmed;                    /* +0x228 */
	int quit;                         /* +0x22c */
	struct CBGEffectGroup *group;     /* +0x230 */
	unsigned char unk234[0x60];       /* +0x234 */
	struct CMA_CharSelPanel panels[4];/* +0x294 */
	unsigned char unk3a4[4];          /* +0x3a4 */
	signed char stage;                /* +0x3a8 0 join, 1 choose, 2 markers, 3 celebrate, 4 menu, 5 */
	unsigned char unk3a9[3];
	int ready;                        /* +0x3ac */
};

/* CGameMode_GameStats (vtable 0x0025513c, name "GameStats"): the table of the round scores */
struct CGameMode_GameStats {
	struct CGameModeMenu menu;      /* +0x000 */
	struct CMA_List overlays;       /* +0x098 */
	float timer;                    /* +0x0a4 buttons work when it is 0 */
	int unka8;                      /* +0x0a8 */
	void *portrait[4];              /* +0x0ac per player */
	void *totalText[4];             /* +0x0bc */
	void *roundText[4];             /* +0x0cc */
	int place[4];                   /* +0x0dc */
	int order[4];                   /* +0x0ec */
	int total[4];                   /* +0x0fc counted up */
	int target[4];                  /* +0x10c this round's score */
	int shown[4];                   /* +0x11c */
	unsigned char counted[4];       /* +0x12c */
	int tickSound;                  /* +0x130 kickimpact2.wav */
	int swipeSound;                 /* +0x134 punchswipe1.wav */
	unsigned char closing;          /* +0x138 */
	unsigned char standalone;       /* +0x139 not over another mode: draws the menu */
	unsigned char unk13a[2];
};                                  /* 0x13c */

extern const void *const vtable_002538a4[];
extern const void *const vtable_00253928[];
extern const void *const vtable_002539c0[];
extern const void *const vtable_00253bac[];
extern const void *const vtable_00253bc4[];
extern const void *const vtable_00253c04[];
extern const void *const vtable_00253dc4[];
extern const void *const vtable_0025513c[];

/* ---------- globals (core_modes_a_data.c) */

extern struct CGameMode_BGEffects *DAT_002b95d0; /* the BGEffects mode */

/* the mode stack of module core (CGameModeStack at 0x002ba750: 20 modes, count, first drawn).
   Core defines its .bss as one block and aliases every Ghidra name into it (src/core/core_data.h);
   0x002ba750 has no Ghidra name, so it is reached from DAT_002ba74c, the name before it. */
extern unsigned char DAT_002ba74c[];
#define CMA_MODE_STACK ((void *)(DAT_002ba74c + 4))
#define CMA_MODE_AT(i) (((struct CGameMode **)CMA_MODE_STACK)[(i)])
#define CMA_MODE_COUNT (*(int *)(DAT_002ba74c + 4 + 0x50))
/* FUN_0002b5b0 (thiscall): the mode on the stack with this name (case-insensitive), or 0 */
#define CMA_ModeStack_Find(name) \
	((struct CGameMode *(*)(void *, const char *))FUN_0002b5b0)(CMA_MODE_STACK, (name))
/* FUN_0002b640 (thiscall): push MODE; RESUME calls its Resume */
#define CMA_ModeStack_Push(mode, resume) \
	((int (*)(void *, void *, int))FUN_0002b640)(CMA_MODE_STACK, (mode), (resume))
/* FUN_0002b6f0 (thiscall): pop the top mode; RESUME resumes the one below */
#define CMA_ModeStack_Pop(resume) ((int (*)(void *, int))FUN_0002b6f0)(CMA_MODE_STACK, (resume))

/* classlib_ui COverlay creation from a 0x3b0-byte parameter block: FUN_00074500 a new overlay
   (cdecl), FUN_00073ae0 a child of PARENT (thiscall) */
#define CMA_Overlay_Create(params) ((void *(*)(void *))FUN_00074500)(params)
#define CMA_Overlay_CreateChild(parent, params) \
	((void *(*)(void *, void *))FUN_00073ae0)((parent), (params))

/* core FUN_00030000: the menu modes' update (thiscall; the generated prototype has no "this"
   until core declares it) */
#define CMA_MenuMode_Process(self, active) \
	__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(&FUN_00030000), void (*)(int)), \
		((void (*)(int))FUN_00030000)(active), \
		((void (*)(void *, int))FUN_00030000)((void *)(self), (active)))

/* players: 4 entries of 0x28 bytes at 0x002baed0 (module core's .bss block) */
struct CMA_Player {
	int unk00;          /* +0x00 */
	int character;      /* +0x04 0..5, -1 none */
	int joined;         /* +0x08 */
	int unk0c[7];       /* +0x0c */
};
extern struct CMA_Player DAT_002baed0[4];
extern int DAT_002bae94; /* number of players */

/* the minigame modules (module core): vtable +0x0c Start(variant) */
extern void *DAT_002ba954[13];

#endif
