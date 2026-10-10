/*
MOUSEWHACK.H

Module mousewhack: the "Bug Bash" minigame (internal name MouseWhack; game list name
"MouseWhack", variations "Brawl", "Tremors", "Burgers", "Pied Pipers"),
0x0016cce0-0x001872f0. Original files (C:\BlitzParty\Game\Src\MouseWhack\):

  CModule_MouseWhack.cpp   0x0016cce0-0x0016cea0  the CGame entry of core's game list
  CGameMode_MouseWhack.cpp 0x0016ceb0-0x00173f70  the game mode: load, init, frame, draw
                                                  (gap functions; the file name is in the
                                                  heap allocation tags of 0x0016d780)
  CBbash_Game.cpp          0x00173f80-0x0017d170  rules, spawning, scores, camera
  CBbash_Player.cpp        0x0017d180-0x00180710  the player (a CharLib AI character)
  CBbash_Bullets.cpp       0x00180720-0x00180f50  paint balls / spit balls
  CBbash_Bugs.cpp          0x00180f60-0x00183c90  the bugs (worms, roaches, flies, tremors)
  CBBash_Splat.cpp         0x00183ca0-0x00185e80  paint splats on the floor
  (unplaced)               0x00185e90-0x001872f0  the player AI

Owner: mousewhack. Other modules may include this header; only mousewhack edits it.

Conventions (docs/ARCHITECTURE.md):
- The game keeps almost all of its state in globals: .bss 0x0030a0a8-0x0030ef04 and the
  initialised block 0x002b26b8-0x002b4b8c. Both keep their original layout: the code
  indexes across them (tables of actor instances, vertex arrays, per-player arrays), so
  they are byte blocks reached by original address with MW_I/MW_F/... below.
  Six objects inside the .bss range are constructed by core's static constructors and are
  defined there as separate symbols (DAT_0030a0e4, DAT_0030a0f0, DAT_0030a1d0,
  DAT_0030ea20, DAT_0030ed00, DAT_0030ee30); MW_ADDR maps their addresses to them.
  DAT_0030a1d0 is the arena object (CBbash_Arena, 0x4850 bytes up to 0x0030ea20; core
  sizes it 0x90, -fcommon keeps the larger size).
- Object fields not understood yet are reached with MW_AT(p, off, type) and the original
  offset; virtual calls go through MW_VFN(obj, byte offset, function type) with the object
  as first argument (the x86 code passed it in ECX).
- Functions keep their FUN_<addr> names unless renamed in mousewhack.decls.
*/

#ifndef OPENFRENZY_MOUSEWHACK_H
#define OPENFRENZY_MOUSEWHACK_H

#include "xtl.h"
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================== the data blocks */

#define MW_DATA_LO 0x002b26b8u
#define MW_DATA_HI 0x002b4b8cu

extern uint32_t mw_data[(MW_DATA_HI - MW_DATA_LO) / 4];      /* 0x002b26b8 (mw_data.c) */
extern unsigned char mw_bss0[0x0030a0e4 - 0x0030a0a8];        /* 0x0030a0a8 (mw_bss.c) */
extern unsigned char DAT_0030a0e4[0xc];                       /* core static init: a list */
extern unsigned char DAT_0030a0f0[0x48];                      /* core static init: 0x00025920 object */
extern unsigned char mw_bss1[0x0030a1d0 - 0x0030a138];        /* 0x0030a138 */
extern unsigned char DAT_0030a1d0[0x0030ea20 - 0x0030a1d0];   /* the arena (CBbash_Arena), core static init */
extern unsigned char DAT_0030ea20[0x10];                      /* core static init: a list */
extern unsigned char mw_bss3[0x0030ed00 - 0x0030ea30];        /* 0x0030ea30 */
extern unsigned char DAT_0030ed00[0xc];                       /* core static init: the bullet list */
extern unsigned char mw_bss4[0x0030ee30 - 0x0030ed0c];        /* 0x0030ed0c */
extern unsigned char DAT_0030ee30[0x10];                      /* core static init: the bug list */
extern unsigned char mw_bss5[0x0030ef04 - 0x0030ee40];        /* 0x0030ee40 */

/* the port address of original data address `a` (a constant: the ternaries fold) */
#define MW_ADDR(a) ((void *)( \
	(a) < 0x0030a0a8u ? (char *)mw_data + ((a) - MW_DATA_LO) : \
	(a) < 0x0030a0e4u ? (char *)mw_bss0 + ((a) - 0x0030a0a8u) : \
	(a) < 0x0030a0f0u ? (char *)DAT_0030a0e4 + ((a) - 0x0030a0e4u) : \
	(a) < 0x0030a138u ? (char *)DAT_0030a0f0 + ((a) - 0x0030a0f0u) : \
	(a) < 0x0030a1d0u ? (char *)mw_bss1 + ((a) - 0x0030a138u) : \
	(a) < 0x0030ea20u ? (char *)DAT_0030a1d0 + ((a) - 0x0030a1d0u) : \
	(a) < 0x0030ea30u ? (char *)DAT_0030ea20 + ((a) - 0x0030ea20u) : \
	(a) < 0x0030ed00u ? (char *)mw_bss3 + ((a) - 0x0030ea30u) : \
	(a) < 0x0030ed0cu ? (char *)DAT_0030ed00 + ((a) - 0x0030ed00u) : \
	(a) < 0x0030ee30u ? (char *)mw_bss4 + ((a) - 0x0030ed0cu) : \
	(a) < 0x0030ee40u ? (char *)DAT_0030ee30 + ((a) - 0x0030ee30u) : \
	(char *)mw_bss5 + ((a) - 0x0030ee40u)))

#define MW_G(a, type) (*(type *)MW_ADDR(a))
#define MW_A(a) ((char *)MW_ADDR(a))          /* address as char * */
#define MW_I(a) MW_G(a, int)
#define MW_U(a) MW_G(a, unsigned int)
#define MW_F(a) MW_G(a, float)
#define MW_S(a) MW_G(a, short)
#define MW_US(a) MW_G(a, unsigned short)
#define MW_B(a) MW_G(a, unsigned char)
#define MW_P(a) MW_G(a, void *)
#define MW_FV(a) ((float *)MW_ADDR(a))        /* a float vector */
#define MW_IV(a) ((int *)MW_ADDR(a))
#define MW_SV(a) ((short *)MW_ADDR(a))

/* ======================================================================== named globals */

#define mw_Bullets MW_P(0x0030a0a8)            /* the bullet manager (0x10 bytes, 0x00180720) */
#define mw_TremorMenu MW_G(0x0030a0b0, struct CTweakMenu *)
#define mw_RouteTables MW_P(0x0030a0b4)        /* new CLRouteTable[4] (CModule start) */
#define mw_LevelPackage MW_P(0x0030a0b8)       /* Packages\BugBash */
#define mw_Camera MW_P(0x0030a0bc)             /* the charlib camera (0x574 bytes) */
#define mw_WallSet MW_P(0x0030a0c0)            /* the arena's wall set ("miniset") */
#define mw_ScoreMenu MW_G(0x0030a0c4, struct CTweakMenu *)
#define mw_VariationPackage MW_P(0x0030a0c8)   /* Packages\Bug-V<n> */
#define mw_DebugMenu MW_G(0x0030a0cc, struct CTweakMenu *)
#define mw_ArenaSet MW_P(0x0030a0d8)           /* the arena set (BSet) */
#define mw_ArenaActor MW_P(0x0030a0dc)         /* the arena actor */
#define mw_ImpactMenu MW_G(0x0030a0e0, struct CTweakMenu *)
#define mw_Arena ((void *)DAT_0030a1d0)        /* the arena object (CBbash_Arena) */
#define mw_Variation MW_S(0x0030a1d0)          /* 0 Brawl, 1 Tremors, 2 Burgers, 3 Pied Pipers */
#define mw_Timer ((void *)MW_A(0x0030a1d8))    /* arena +8: the frame timer (vtable 0x00272d64) */
#define mw_DbgMessages MW_I(0x0030a27c)        /* tweak "debug messages" */
#define mw_DbgOverlays MW_I(0x0030a280)        /* tweak "overlay updates" */
#define mw_DbgAIState MW_I(0x0030a284)         /* tweak "AI state" */
#define mw_DbgSpeedLock MW_I(0x0030a290)       /* tweak "Game Speed lock" */
#define mw_DbgShadows MW_I(0x0030a294)         /* tweak "Shadows" */
#define mw_DbgFrameCounters MW_I(0x0030a298)   /* tweak "Frame Counters" */
#define mw_DbgBugsFade MW_I(0x0030a29c)        /* tweak "Bugs fade out" */
#define mw_DbgFirstPerson MW_I(0x0030a2a0)     /* tweak "First person" */
#define mw_DbgHoldOutro MW_I(0x0030a2a4)       /* tweak "hold outro" */
#define mw_FontBold MW_P(0x0030a318)           /* fonts\handgotb.ttf */
#define mw_ShadowTexture MW_P(0x0030a328)      /* textures\shadow.bmp */
#define mw_TrainModel MW_P(0x0030a338)         /* models\train.obe */
#define mw_Game ((void *)MW_A(0x0030eb60))     /* the game object (CBbash_Game) */
#define mw_GameState MW_S(0x0030eb92)          /* 1 intro, 2 playing, ... */
#define mw_Quit MW_S(0x0030ebe8)               /* nonzero: stop processing and drawing */
#define mw_Light2 MW_P(0x0030ebf0)
#define mw_Light MW_P(0x0030ebf4)
#define mw_Players ((void **)MW_ADDR(0x0030ec60))  /* the 4 players (CBbash_Player, 0x1410 bytes) */
#define mw_Helpers ((void **)MW_ADDR(0x0030ed0c))  /* 10 objects of 0x1c bytes (0x00181730) */
#define mw_ParticleTexture MW_P(0x0030ee40)    /* textures\particletwo00.bmp ("deathTex") */
#define mw_SprayTexture MW_P(0x0030ee44)       /* textures\spray.bmp */
#define mw_BlastTexture MW_P(0x0030eec4)       /* textures\blastring.bmp */
#define mw_SwipeTexture MW_P(0x0030eec8)       /* textures\swipe.bmp */
#define mw_SplatTextures ((void **)MW_ADDR(0x0030eecc))  /* 10 splat textures */

/* ======================================================================== object helpers */

#define MW_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define MW_AI(p, off) MW_AT(p, off, int)
#define MW_AU(p, off) MW_AT(p, off, unsigned int)
#define MW_AF(p, off) MW_AT(p, off, float)
#define MW_AS(p, off) MW_AT(p, off, short)
#define MW_AP(p, off) MW_AT(p, off, void *)
#define MW_AB(p, off) MW_AT(p, off, unsigned char)
#define MW_AV(p, off) ((float *)((char *)(p) + (off)))   /* a float vector field */

/* virtual function at byte offset `off` of the object's vtable */
#define MW_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* call function `fn` with an explicit shape (prototype in the table has the wrong shape) */
#define MW_FN(fn, ret, args) ((ret(*) args)(fn))

/* bKernel heap: operator new and delete */
#define MW_NEW(size, line) bkHeapAlloc((uint)(size), mw_GameModeFile, (line), 0x1001)
#define MW_DELETE(p) bkHeapFree((void *)(p))

/* x87 _ftol (0x001de998): truncating conversion */
#define MW_FTOL(x) ((int)(longlong)(x))

/* MSVC rand(), inlined into the game: holdrand is the C library's seed at 0x00285630 */
extern unsigned int DAT_00285630;
static inline int mw_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* a float's bit pattern, for an undefined4 parameter that carries a float */
static inline unsigned int mw_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

static inline float mw_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

extern const char mw_GameModeFile[];   /* "C:\BlitzParty\Game\Src\MouseWhack\CGameMode_MouseWhack.cpp" */

/* ======================================================================== other modules' globals */

extern float DAT_002855f0[16];        /* bkernel/bdisplay: the identity matrix */
extern float DAT_0028fda8;            /* core: frame time scale (1.0 at the nominal rate) */
extern float DAT_0028ffa8;            /* bdisplay: near plane used by the minigames (4000) */
extern float DAT_0028ffac;            /* bdisplay: far plane (10000) */
extern unsigned char DAT_00290718[];  /* classlib_ui: player colours, 0x60 bytes per player */
extern int DAT_00292c78;              /* charlib: 0x240 */
extern int DAT_002bae6c;              /* core: sound effect volume 0-255 */
extern int DAT_002bae70;              /* core: handicap / difficulty 0-2 */
extern int DAT_002bae94;              /* core: number of players */
extern int DAT_002baeb8;              /* core: this game mode has no RefBot / skips the intro */
extern int DAT_002baed4[];            /* core: player records, 10 ints each */
extern void *DAT_002db808;            /* engine_misc: the DirectSound object */
extern float DAT_002ea478[4];         /* classlib_ui: a zero vector */
extern int DAT_002ecdec;              /* classlib_ui: the game is paused */
extern void *DAT_002ecfac;            /* classlib_ui: the HUD object */
extern void *DAT_002f289c[4];         /* charlib: the AI characters (one per player) */
extern float DAT_00293920;            /* charlib: the global animation speed factor */
extern float DAT_002ba990;            /* core: frames per second */
extern unsigned char *DAT_002ea488;   /* classlib_ui: the actor list holder */

/* ======================================================================== functions missing from the table
(found through calls and vtables; exported with scripts/ExportAt.java; reported to the architect) */

void FUN_00170ec0(void *self);           /* CBbash_Arena::Initialise (CBbash_Arena.cpp) */
void FUN_00172730(void *self);           /* CBbash_Arena: reset for a new round */
void FUN_0017d1f0(void *self);           /* CBbash_Player::~CBbash_Player */
void FUN_00181550(void *self, float *pos, float *rot, const char *model, const char *smashed,
	const char *anim, const char *alpha, int flag);  /* CBbash_Bugs: a smashable prop */
void FUN_00182c90(void);                 /* frees the splat vertex buffers */

/* other modules' functions whose table prototype lacks a float / return value */
extern unsigned char DAT_002ed324[];     /* classlib_ui: the rumble object */

#endif
