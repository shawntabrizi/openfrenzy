/*
ROLLERBALL_B.H

Module rollerball_b: the "RollerBall" minigame (sumo balls on floating platforms; level
names "base", "sumo", "race2", "race", "drop"), part 2 of 2: 0x001915c0-0x0019d150.
Original files (C:\BlitzParty\Game\Src\RollerBall\), in address order:

  CRollerball_Goal.cpp (rest)   0x001915c0-0x00192160  the goal system (rollerball_a has
                                                       the start of the file; inferred)
  CRollerball_Entity.cpp        0x00192170-0x00192c20  the player-ball entities and their
                                                       system (tag 0x001927e0)
  (camera; file not known)      0x00192c40-0x001953e0  the game camera (vtable 0x00274884)
  CRollerball_World.cpp         0x00195400-0x001966a0  the level: LOM file, platforms
                                                       (file name from the heap tag at
                                                       0x002748cc; 2 functions missing
                                                       from the function table)
  CRollerball_Shadow.cpp        0x00196710-0x00196aa0  ball shadows (tag 0x001967f0)
  CRollerBall_EntityEvent.cpp   0x00196ad0-0x00196fa0  entity event queues (tag 0x00196ad0)
  CRollerBall_Water.cpp         0x00196fc0-0x00197580  the goo water plane (tag 0x00196fc0)
  CRollerBall_Cutter.cpp        0x001975b0-0x00198370  the cutters (tag 0x001975b0)
  (game mode; file not known)   0x00198370-0x00198710  CGameMode_RollerBall
  CRollerball_ASEImport.cpp     0x00198730-0x001989e0  ASE shape import (tags 0x00198730..)
  CRollerBall_Player.cpp        0x00198a00-0x0019a4e0  the player (tag 0x00199ba0)
  (AI and shaders; inferred)    0x0019a4e0-0x0019d130  player controllers (AI states)
  CRollerBall_Particle.cpp      0x0019d150             particles (tag)

Owner: rollerball_b. Other modules may include this header; only rollerball_b edits it.

Conventions (docs/ARCHITECTURE.md):
- Objects keep their original (ILP32) layout. Fields are reached with RBB_AT(p, off, type)
  and the original byte offset; well understood fields have named accessor macros below.
- Virtual calls go through RBB_VFN(obj, byte offset, function type), with the object as the
  first argument (the x86 code passed it in ECX).
- Functions of rollerball_a are called through RBB_FN(FUN_<addr>, ret, (args)): their
  prototypes are still Ghidra's guesses and may change; the cast fixes the call shape here.
- The game object (CRollerball_Game, rollerball_a) is reached through the pointer
  DAT_0030ef04 (rbb_Game).
- The initialised RollerBall data block (0x002b4b8c-0x002b5474, rollerball_a's of_rb_data)
  is RBD_ADDR(addr).
*/

#ifndef OPENFRENZY_ROLLERBALL_B_H
#define OPENFRENZY_ROLLERBALL_B_H

#include "xtl.h"
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================== object helpers */

#define RBB_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define RBB_I(p, off) RBB_AT(p, off, int)
#define RBB_U(p, off) RBB_AT(p, off, unsigned int)
#define RBB_F(p, off) RBB_AT(p, off, float)
#define RBB_S(p, off) RBB_AT(p, off, short)
#define RBB_US(p, off) RBB_AT(p, off, unsigned short)
#define RBB_B(p, off) RBB_AT(p, off, unsigned char)
#define RBB_C(p, off) RBB_AT(p, off, char)
#define RBB_P(p, off) RBB_AT(p, off, void *)
#define RBB_V(p, off) ((float *)((char *)(p) + (off)))     /* a float vector field */
#define RBB_ADDR(p, off) ((void *)((char *)(p) + (off)))   /* address of a field */

/* virtual function at byte offset `off` of the object's vtable */
#define RBB_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* call function `fn` with an explicit shape */
#define RBB_FN(fn, ret, args) ((ret(*) args)(fn))

/* bKernel heap: operator new and delete */
#define RBB_NEW(size, file, line) bkHeapAlloc((uint)(size), (file), (line), 0x1001)
#define RBB_DELETE(p) bkHeapFree((void *)(p))

/* x87 _ftol (0x001de998): truncating conversion */
#define RBB_FTOL(x) ((int)(longlong)(x))

/* MSVC rand(), inlined into the game: holdrand is the C library's seed at 0x00285630 */
extern unsigned int DAT_00285630;
static inline int rbb_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* rand() / 32767 (the constant 3.05185094e-05 at 0x00253740) */
#define RBB_RANDF() ((float)rbb_rand() * 3.05185094e-05f)

/* a float's bit pattern, for an undefined4 parameter that carries a float */
static inline unsigned int rbb_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

static inline float rbb_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* ======================================================================== data */

/* the RollerBall .data block 0x002b4b8c-0x002b5474: owned by rollerball_a (of_rb_data in
src/rollerball_a/RollerBall/rbb_data.c, original layout); declared here so this module does
not depend on rollerball_a's header */
#define RBD_DATA_LO 0x002b4b8cu
#define RBD_DATA_HI 0x002b5474u
extern uint32_t of_rb_data[];
#define RBD_ADDR(a) ((void *)((char *)of_rb_data + ((a) - RBD_DATA_LO)))
#define RBD_I(a) (*(int *)RBD_ADDR(a))
#define RBD_U(a) (*(unsigned int *)RBD_ADDR(a))
#define RBD_F(a) (*(float *)RBD_ADDR(a))
#define RBD_FV(a) ((float *)RBD_ADDR(a))
#define RBD_IV(a) ((int *)RBD_ADDR(a))
#define RBD_SV(a) ((const char *const *)RBD_ADDR(a))   /* a table of string pointers */

/* the game object (CRollerBall_Game, 0x2ae0 bytes; rollerball_a defines the pointer in
CModule_RollerBall.c) */
extern unsigned char *DAT_0030ef04;
#define rbb_Game ((char *)DAT_0030ef04)
#define rbb_Variation RBB_I(rbb_Game, 0x1c)           /* 0 base, 1 sumo, 2 race2, 3 race, 4 drop */
#define rbb_FirstPlayer RBB_P(rbb_Game, 0x1464)       /* list of player entities (next: +0x10) */
#define rbb_GoalSystem ((char *)rbb_Game + 0x1470)   /* the goal system object */

/* core: 4 player records of 10 ints at 0x002baed4: [0] character, [1] present (human),
[8] team */
extern int DAT_002baed4[];
#define rbb_PlayerCharacter(i) DAT_002baed4[(i) * 10]
#define rbb_PlayerHuman(i) DAT_002baed4[(i) * 10 + 1]
#define rbb_PlayerTeam(i) DAT_002baed4[(i) * 10 + 8]

extern int DAT_00283ea4;      /* core: character colour set */
extern int DAT_00293928;      /* classlib_ui: sample volume */

/* character tables of the data block (index: character 0-5) */
#define rbb_CharacterModel(c) RBD_SV(0x002b5098)[c]     /* "dub", "fly", ... */
#define rbb_CharacterOptModel(c) RBD_SV(0x002b50b0)[c]  /* "dubopt", ... */
#define rbb_CharacterBall(c) RBD_SV(0x002b50c8)[c]      /* "sphere_green", ... */
#define rbb_CharacterColour(c) RBD_IV(0x002b50e8 + (c) * 12)  /* r, g, b (0-255) */

/* ======================================================================== shared engine helpers */

/* The frame timer class (vtable 0x002745e8; COMDAT code shared by several games, so its
functions sit in other modules). Layout: +0x08 running, +0x09 paused, +0x0a stopped,
+0x28 duration (s), +0x30 remaining time, +0x34 progress 0..1. Names as in rollerball_a
(CRollerBallTimer_*). */
#define rbb_TimerInit(t) RBB_FN(FUN_000ec310, int, (void *))(t)
#define rbb_TimerCtor(t) RBB_FN(FUN_0018e8c0, void, (void *))(t)
#define rbb_TimerDtor(t) RBB_FN(FUN_0018e8d0, void, (void *))(t)
#define rbb_TimerUpdate(t) RBB_FN(FUN_0018e8e0, void, (void *))(t)
#define rbb_TimerSetDuration(t, s) RBB_FN(FUN_0018ea10, void, (void *, float))((t), (s))
#define rbb_TimerStart(t) RBB_FN(FUN_0018ea30, void, (void *))(t)
#define rbb_TimerRemaining(t) RBB_FN(FUN_0018ea50, float, (void *))(t)
#define rbb_TimerReset(t) RBB_FN(FUN_0018ea70, void, (void *))(t)
#define rbb_TimerProgress(t) RBB_FN(FUN_000965f0, float, (void *))(t)

/* the game's sound effects (game + 0x26fc; rollerball_a 0x0018e450 CRollerBallSfx_Play:
bsPlaySample(sample, volume, a, flags, pos)); ids follow the name table at 0x002b4fe0 */
#define rbb_PlaySound(id, vol, a, flags, pos) \
	RBB_FN(FUN_0018e450, int, (void *, int, int, int, unsigned int, float *))( \
		(char *)rbb_Game + 0x26fc, (id), (vol), (a), (flags), (pos))
enum {
	RBB_SND_BALL2BALL1 = 0, RBB_SND_BALL2BALL2, RBB_SND_BALL2FLOOR1, RBB_SND_BALL2FLOOR2,
	RBB_SND_GOALACTIVATE, RBB_SND_DEACTIVATE, RBB_SND_GOALSCORE, RBB_SND_CROWDLOOP,
	RBB_SND_CROWDCHEER, RBB_SND_HAZARD, RBB_SND_PLATFORMDROP, RBB_SND_BALLROLL, RBB_SND_GOOSPLASH,
	RBB_SND_STREETAMB
};

/* ======================================================================== goal system */

/* goal system (game + 0x1470) */
#define RBGS_MODELS 0x08          /* 2 resources: goalsign.obe, goalsfx.obe */
#define RBGS_GOAL_LIST 0x10       /* first goal (next: +0x04) */
#define RBGS_GOALS 0x14           /* 9 goal pointers */
#define RBGS_GOAL_COUNT 0x38
#define RBGS_ACTIVE 0x3c          /* goals light up (cleared when the clock runs out) */
#define RBGS_PATTERN 0x40         /* 0-2: pattern + 1 goals are lit */
#define RBGS_TIMER 0x48           /* pattern timer (frame timer) */
#define RBGS_FLARES 0x80          /* 3 flares (rollerball_a 0x00191060/0x00191110/0x00190bb0) */
#define RBGS_FLARE_SIZE 0x68
#define RBGS_BURSTS 0x1b8         /* 8 particle bursts: +0x04 pos, +0x14 on, +0x18 age, +0x1c system */
#define RBGS_BURST_SIZE 0xc4
#define RBGS_ACTIVATE_WAIT 0x7d8
#define RBGS_DEACTIVATE_WAIT 0x7dc

/* goal */
#define RBG_NEXT 0x04
#define RBG_PLAYER 0x08           /* the player entity whose ball touched it last */
#define RBG_POS 0x0c              /* float[4] */
#define RBG_RADIUS 0x2c
#define RBG_ENABLED 0x30          /* short */
#define RBG_STATE 0x32            /* short, RBG_STATE_* */
#define RBG_ANIM0_ON 0x40         /* byte: actor 0 animates (2 actors, stride 0x18) */
#define RBG_ANIM0_INST 0x44
#define RBG_ANIM1_ON 0x58
#define RBG_ANIM1_INST 0x5c
#define RBG_SCORE_ANIM 0x60
#define RBG_COLOUR 0x64           /* int r, g, b */
#define RBG_STATE_TIME 0x70
#define RBG_FADE 0x74             /* 0-0x7fff */
enum { RBG_STATE_IDLE, RBG_STATE_ACTIVATING, RBG_STATE_DEACTIVATING, RBG_STATE_LIT, RBG_STATE_SCORED };

/* ======================================================================== entities */

/* entity system (CRollerball_EntitySystem, in the game object) */
#define RBES_CHARACTERS 0x04      /* 4 resources: characters\<name>.obe of players 0-3 */
#define RBES_OPT_CHARACTERS 0x14  /* 4 resources: characters\<name>opt.obe */
#define RBES_BALLS 0x24           /* 4 resources: models\sphere_<colour>.obe */
#define RBES_MODELS 0x34          /* hoverpod.obe, shadowball.obe */
#define RBES_LIST 0x3c            /* first entity */
#define RBES_COLOUR_SET 0x40

/* player entity (CRollerball_Entity / CRollerBall_Player; 0x427c bytes, made by 0x00198a00) */
#define RBE_EVENTS 0x04           /* event queue (CRollerBall_EntityEventSystem base) */
#define RBE_NEXT 0x10
#define RBE_ACTIVE 0x14           /* byte: still in the game */
#define RBE_WINNER 0x17           /* byte: in play / a winner (the camera frames these) */
#define RBE_COMPUTER 0x19         /* byte: no human plays it */
#define RBE_START_POS 0x40b8      /* float[4] */
#define RBE_BODY 0x40cc           /* physics body (rollerball_a, vtable 0x002745f0) */
#define RBE_POS 0x40d0            /* float[4] (body + 0x04) */
#define RBE_VEL 0x40f0            /* float[3] */
#define RBE_ACCEL 0x4100          /* float[3] */
#define RBE_NORMAL 0x4110         /* float[3] ground normal */
#define RBE_ROTATION 0x4120       /* quaternion */
#define RBE_TIME_SCALE 0x4130
#define RBE_RADIUS 0x413c
#define RBE_FORCE 0x4140          /* float[3]: +4 is gravity */
#define RBE_PUSH 0x4150           /* float[4] */
#define RBE_DRAG 0x4160
#define RBE_KINETIC_FRICTION 0x4164
#define RBE_STATIC_FRICTION 0x4168
#define RBE_PLAYER 0x419c         /* player index 0-3 */
#define RBE_CHARACTER 0x41a0      /* character 0-5 (read as a char for the sample set) */
#define RBE_HEADING2 0x41a4       /* float: heading (copy) */
#define RBE_COLOUR 0x41c0         /* int r, g, b (half of the table colour) */
#define RBE_SCORE 0x41e4
#define RBE_CONTROLLER 0x4204     /* the controller (AI state object) */
#define RBE_HEADING 0x420c        /* float: heading */

#include "rollerball_b_missing.h"

#endif
