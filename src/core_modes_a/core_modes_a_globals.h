/*
CORE_MODES_A_GLOBALS.H

Globals module core_modes_a uses, by their Ghidra names (docs/ARCHITECTURE.md
section 12), and helpers to reach them. Definitions (or fallbacks for names
other modules own) are in Core/core_modes_a_data.c.

Owner: core_modes_a.
*/

#ifndef CORE_MODES_A_GLOBALS_H
#define CORE_MODES_A_GLOBALS_H

#include "core_modes_a.h"

/* ---------- module core (.bss block 0x002ba4f0-0x002bb138, .data block 0x002839b8-0x0028450c) */

extern int DAT_002ba544;            /* language folder (char *, "packages\english") */
extern int DAT_002ba554;            /* the CharacterSelect mode */
extern int DAT_002ba558, DAT_002ba55c; /* the modes after character select (party, tournament) */
extern int DAT_002ba564;            /* the first menu mode */
extern int DAT_002ba940, DAT_002ba944, DAT_002ba948, DAT_002ba94c; /* clear colour */
extern int DAT_002ba99c;            /* frame count */
extern int DAT_002ba9a4;            /* package being loaded in the background */
extern int DAT_002ba9a8;            /* second package */
extern int DAT_002ba9ac;
extern int DAT_002ba9b0;            /* packages still loading */
extern float _DAT_002ba98c;         /* refresh rate (50 or 60) */
extern float _DAT_002ba9c8;         /* demo timer (frames) */
extern int DAT_002baadc;            /* launched from the demo disc */
extern int DAT_002baae8;
extern int DAT_002baaec;            /* last vertical blank count */
extern int DAT_002bae6c;
extern int DAT_002bae70;            /* character select menu entry */
extern int DAT_002bae8c;            /* players who joined */
extern int DAT_002bae90;            /* players still free */
extern int DAT_002bae98, DAT_002bae9c;
extern int DAT_002baeac, DAT_002baeb0, DAT_002baeb8, DAT_002baebc;
extern void *DAT_002baf78[36];      /* 4 x 9 objects of the players' button prompts */
extern int DAT_002844cc, DAT_002844d0, DAT_002844d4, DAT_002844d8; /* default clear colour */
extern int DAT_00284408;
extern int PTR_s_packages_english_002839e0[]; /* [game * 4 + variation] (row 0: pointers) */
extern int PTR_s_packages_english_002839e4[];

/* ---------- module classlib_ui (.data 0x0028fda8-0x00290cc0, .bss 0x002ea490-0x002ed33c) */

extern int DAT_00290754;            /* the character table (6 x 0x60): name at -0x5c, voice package */
extern char DAT_002900c8[];         /* 6 x 0x108: model package names */
extern int DAT_002ecdec;            /* pause count */
extern int DAT_002ecde8;
extern void *DAT_002ecc74;          /* the text table */
extern void *DAT_002ecfac;          /* the intro camera */

/* the character table entry I (name at +0, voice package at +0x5c) */
#define CMA_CHAR_NAME(i) (*(const char **)((char *)&DAT_00290754 - 0x5c + (i) * 0x60))
#define CMA_CHAR_VOICE(i) (*(int *)((char *)&DAT_00290754 + (i) * 0x60))
/* the text with index ID in the text table */
#define CMA_TEXT(id) (*(wchar16 **)(CMA_FIELD(int, DAT_002ecc74, 0x20) + (id) * 0x10))

/* ---------- other modules */

extern void *DAT_002db808;          /* engine_misc: the DirectSound object */
extern char DAT_002d0758[];         /* bkernel: the disc root ("d:\") */
extern int DAT_002bc40c, DAT_002bc410; /* bdisplay: the screen width and height */
extern int DAT_0028198c;            /* AskAgain answer: 1 yes, -1 no */
extern int DAT_002838f4, DAT_002838f8; /* the game and variation to play again */
extern float DAT_00283760;          /* attract mode timer */
extern int DAT_00292c4c, DAT_00292c50, DAT_00292c78;
extern int DAT_002d02ac;

/* the animation table (charlib?): 0x44-byte entries, name and speed */
struct CMA_AnimEntry {
	char name[64];
	float speed;
};
extern struct CMA_AnimEntry DAT_00292ca0[40];
#define CMA_ANIM_IDLE_READY 0
#define CMA_ANIM_CAMERA_POSE 23
#define CMA_ANIM_RUN 24
#define CMA_ANIM_SELECT 29
#define CMA_ANIM_DESELECT 30
#define CMA_ANIM_IDLE_CHARSELECT 32
#define CMA_ANIM_PSYCHED 33

/* ---------- this module */

extern int DAT_00281990;            /* the last attract-mode game */
extern int DAT_002b95ac;            /* the CharacterSel package finished loading */
extern int DAT_002b95b0[6];         /* character voice packages to load */
extern char DAT_002b95c8;           /* draw the fade overlay */
extern char DAT_002b95d4;           /* bgeffects package requested */
extern int DAT_002b9670;            /* the CharacterSel package */

/* character select (Core/CGameMode_CharacterSelect.c) */
extern const char *PTR_s_p1_select_bmp_00281998[4];
extern const char *PTR_s_p1_confirm_bmp_002819a8[4];
extern float DAT_002819b8;          /* -4: panel spacing */
extern int DAT_002819bc[4];         /* per player: the next idle animation */
extern float DAT_002819cc[6];       /* per character: cheer delay */
extern float DAT_002819e4[2], DAT_002819ec[2]; /* marker offsets */
extern const char *PTR_s_dub_on_bmp_002819f4[6];
extern const char *PTR_s_dub_off_bmp_00281a0c[6];
extern float DAT_00281a24, DAT_00281a28, DAT_00281a2c; /* portrait row: x0, y, dx */
extern int DAT_00281a30, DAT_00281a34; /* info box position */
extern float DAT_00281a38;          /* info box scroll speed */
extern int DAT_00281a3c;            /* info box line height */
extern float DAT_00281a40[16];      /* 4 player positions (x, y, z, w) */
extern float DAT_00281af0, DAT_00281af4, DAT_00281af8, DAT_00281afc; /* model position */
extern float DAT_00281b00;          /* overlay slide speed */
extern int DAT_00281b04, DAT_00281b08; /* menu text y and spacing */
extern int DAT_00281b0c, DAT_00281b10, DAT_00281b14; /* tweak: ambient light */
extern int DAT_00281b18, DAT_00281b1c, DAT_00281b20; /* tweak: light colour */
extern float DAT_00281b24, DAT_00281b28, DAT_00281b2c; /* tweak: light direction */
extern float DAT_00281b38;          /* portrait pulse speed */
extern int DAT_00281b40;
extern float DAT_00281b74;          /* panel move speed */
extern float DAT_00281bd8[16];      /* celebration: 4 positions */
extern int DAT_00281c30;            /* number of "gencheer" samples */
extern float DAT_00281c34, DAT_00281c38, DAT_00281c3c; /* cheer timing and fade */
extern unsigned char DAT_002b95e8[0x70]; /* the light of character select (bactor light) */
extern float DAT_002b9658[6];       /* per character: cheer countdown */
extern int DAT_002b967c;
extern struct CMA_Vertex DAT_002b9680[4]; /* the floor of the celebration */
extern int DAT_002b9710;

/* ---------- functions of other modules, with the prototypes of the x86 code */

/* platform (XDK) */
long IDirectSound_GetCaps(void *sound, void *caps);
void D3DDevice_GetDisplayFieldStatus(void *status);

/* classlib_ui FUN_00071f90: start loading a package in the background; returns the handle */
#define CMA_LoadPackageAsync(name, flags) ((int (*)(const char *, int))FUN_00071f90)((name), (flags))
/* bdisplay FUN_000463c0: play a Bink video (path, flags) */
#define CMA_PlayBink(path, flags) ((int (*)(const char *, int, void *, int))FUN_000463c0)((path), (flags), 0, 0)

/* bactor FUN_000358d0: play SEQUENCE on ACTOR's TRACK blended over BLEND (a float) */
#define CMA_Actor_PlayBlended(actor, track, seq, speed, loops, blend, loop, a8, a9) \
	CMA_DUAL(FUN_000358d0, \
		void (*)(int, int, undefined4, undefined4, undefined4, undefined4, undefined4, undefined4, undefined4), \
		void (*)(int, int, int, int, int, float, int, int, int), \
		((actor), (track), (seq), (speed), (loops), cma_fbits(blend), (loop), (a8), (a9)), \
		((actor), (track), (seq), (speed), (loops), (blend), (loop), (a8), (a9)))
/* bactor FUN_000380f0: advance ACTOR's TRACK by TIME (a float) */
#define CMA_Actor_Update(actor, track, time, flags) \
	CMA_DUAL(FUN_000380f0, void (*)(int, undefined4, undefined4, undefined4), \
		void (*)(int, int, float, int), \
		((actor), (track), cma_fbits(time), (flags)), ((actor), (track), (time), (flags)))

#endif
