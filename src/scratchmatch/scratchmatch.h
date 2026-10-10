/*
SCRATCHMATCH.H

Module scratchmatch: the "Scratch Match" minigame (a rhythm game: button icons scroll up to a
hit line and each player presses the matching buttons on the beat). Game list name
"ScratchMatch", variations "Rhythm", "Pattern", "Scratch" and "Hardcore" (Start receives 1-4;
the code knows variations 1-3: 1 and 2 are the scoring games, 3 the survival game with
lives). 0x001ba560-0x001c1758. Original files (C:\BlitzParty\Game\Src\ScratchMatch\):

  CModule_ScratchMatch.cpp     0x001ba560-0x001ba6c0  the CGame entry of core's game list
  CGameMode_ScratchMatch.cpp   0x001ba6d0-0x001bb330  the game mode: load, init, frame, draw
  CScratchmatch_Sound.cpp      0x001bb340-0x001bb620  samples, the music stream
  CScratchmatch_Camera.cpp     0x001bb630-0x001bbf80  intro camera, follow and wander cameras
  CScratchmatch_Game.cpp       0x001bbf90-0x001bcfd0  rules, the button sequence, game states
  ScratchMatch_GenActors.cpp   0x001bcfe0-0x001bd2d0  the two hovering lights
  (unplaced)                   0x001bd2e0-0x001bd470  the 15 performance-counter timers
  CScratchmatch_Render.cpp     0x001bd480-0x001bde40  arena, players, the draw frame
  CScratchmatch_Lights.cpp     0x001bde50-0x001be5c0  static, spot and pod lights
  ScratchMatch_Effects.cpp     0x001be5d0-0x001beb50  smoke planes, respawn effects, debris
  CScratchmatch_Player.cpp     0x001beb60-0x001bffe0  players: models, input, AI, hits
  CScratchmatch_Sprites.cpp    0x001bfff0-0x001c1550  the HUD: icons, frames, lives, texts
  CScratchmatch_Scenics.cpp    0x001c1560-0x001c1758  the animated speakers (a small class)

Gap functions are placed in the file their content and neighbours point to; each says so.

Owner: scratchmatch. Other modules may include this header; only scratchmatch edits it.

Conventions (docs/ARCHITECTURE.md):
- The game keeps its state in globals: the initialised block 0x002b5b60-0x002b6950 (.data)
  and 0x0030fe90-0x00312f50 (.bss). Both keep their original layout, because the code indexes
  across them (per-player blocks of 0x480 bytes, icon and text lists), so they are byte blocks
  reached by original address with SM_I/SM_F/... below, or through the typed structs.
  No other module uses these addresses.
- Floats that the x87 code kept at extended precision are doubles here where that matters
  (the timers return the unrounded quotient, as ST0 did).
- Functions keep their FUN_<addr> names unless renamed in scratchmatch.decls.
*/

#ifndef OPENFRENZY_SCRATCHMATCH_H
#define OPENFRENZY_SCRATCHMATCH_H

#include "xtl.h"
#include "xkernel.h"
#include "bactor.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================== the data blocks */

#define SM_DATA_LO 0x002b5b60u
#define SM_DATA_HI 0x002b6950u
#define SM_BSS_LO 0x0030fe90u
#define SM_BSS_HI 0x00312f50u

extern uint32_t sm_data[(SM_DATA_HI - SM_DATA_LO) / 4];   /* 0x002b5b60 (sm_data.c) */
extern unsigned char sm_bss[SM_BSS_HI - SM_BSS_LO];        /* 0x0030fe90 (sm_bss.c) */

/* the port address of original data address `a` (a constant: the ternary folds) */
#define SM_ADDR(a) ((void *)((a) < SM_BSS_LO ? (char *)sm_data + ((a) - SM_DATA_LO) \
                                             : (char *)sm_bss + ((a) - SM_BSS_LO)))

#define SM_G(a, type) (*(type *)SM_ADDR(a))
#define SM_A(a) ((char *)SM_ADDR(a))          /* address as char * */
#define SM_I(a) SM_G(a, int)
#define SM_U(a) SM_G(a, unsigned int)
#define SM_F(a) SM_G(a, float)
#define SM_D(a) SM_G(a, double)
#define SM_P(a) SM_G(a, void *)
#define SM_FV(a) ((float *)SM_ADDR(a))        /* a float vector */
#define SM_IV(a) ((int *)SM_ADDR(a))
#define SM_UV(a) ((unsigned int *)SM_ADDR(a))
#define SM_PV(a) ((void **)SM_ADDR(a))

/* object field at byte offset `off` */
#define SM_AT(p, off, type) (*(type *)((char *)(p) + (off)))

/* ======================================================================== typed blocks */

/* a player (0x480 bytes, 4 of them at 0x00310890, CScratchmatch_Player.cpp "scPlayer[i]") */
struct SMPlayer {
	struct BActor *podActor;              /* +0x000 actors\pod_<colour>.obe (variation package) */
	int unk004[3];
	struct BActorInstance pod;            /* +0x010 the platform the player stands on */
	struct BActor *actor;                 /* +0x180 models\scratch_<name>.obe */
	struct BActorSubObject *anims[7];     /* +0x184 scratch_1-4, idle_dance, good_scratch, bad_scratch */
	struct BActorInstance inst;           /* +0x1a0 the full model (the camera's target) */
	struct BActor *lowActor;              /* +0x310 models\<name>opt.obe */
	struct BActorInstance *lowInst;       /* +0x314 the low-polygon model */
	struct biMap *input;                  /* +0x318 */
	struct biChannel *upDown;             /* +0x31c */
	struct biChannel *leftRight;          /* +0x320 */
	struct biChannel *buttons[4];         /* +0x324 A, B, X, Y (button1-4) */
	struct biChannel *dpad[4];            /* +0x334 up, right, down, left */
	float pos[4];                         /* +0x344 */
	int cpu;                              /* +0x354 1: computer player */
	unsigned int history[15];             /* +0x358 pressed-button masks, newest first */
	int unk394[17];
	int unk3d8;                           /* +0x3d8 */
	unsigned int seqPos;                  /* +0x3dc next line of the sequence to show */
	unsigned int hits;                    /* +0x3e0 icons hit (the combo counter) */
	int lives;                            /* +0x3e4 survival game */
	int character;                        /* +0x3e8 */
	int icon;                             /* +0x3ec the icon at the hit line, 0xffff none */
	int prevIcon;                         /* +0x3f0 the icon before it */
	int unk3f4;                           /* +0x3f4 0xffff, never changed */
	int prevButtons;                      /* +0x3f8 buttons of prevIcon */
	unsigned int hitButtons;              /* +0x3fc buttons of the icon just hit */
	int score;                            /* +0x400 */
	unsigned int dance;                   /* +0x404 scratch animation 0-3 */
	int unk408[16];
	int unk448;                           /* +0x448 */
	unsigned int colour;                  /* +0x44c the player colour, 0xRRGGBB */
	unsigned int brightColour;            /* +0x450 the colour doubled, saturated */
	float heading;                        /* +0x454 atan(x / z) */
	float spin;                           /* +0x458 extra turn (intro, losing) */
	float unk45c;                         /* +0x45c */
	float aiTiming;                       /* +0x460 the computer player's timing offset */
	float deathTime;                      /* +0x464 */
	float drop;                           /* +0x468 height above the stage (intro drop) */
	float lifeLostTime;                   /* +0x46c */
	int hit;                              /* +0x470 the current icon was hit */
	int aiWillHit;                        /* +0x474 the computer player hits the next icon */
	int dead;                             /* +0x478 */
	int visible;                          /* +0x47c */
};
OF_ASSERT_SIZE(struct SMPlayer, 0x480);
OF_ASSERT_OFFSET(struct SMPlayer, pod, 0x10);
OF_ASSERT_OFFSET(struct SMPlayer, inst, 0x1a0);
OF_ASSERT_OFFSET(struct SMPlayer, input, 0x318);
OF_ASSERT_OFFSET(struct SMPlayer, pos, 0x344);
OF_ASSERT_OFFSET(struct SMPlayer, history, 0x358);
OF_ASSERT_OFFSET(struct SMPlayer, seqPos, 0x3dc);
OF_ASSERT_OFFSET(struct SMPlayer, score, 0x400);
OF_ASSERT_OFFSET(struct SMPlayer, colour, 0x44c);
OF_ASSERT_OFFSET(struct SMPlayer, visible, 0x47c);

/* a scrolling button icon (0x2c bytes, 100 at 0x00311be0, "maxIconSprites") */
struct SMIcon {
	unsigned int buttons;                 /* +0x00 0: free; bits 0-3 A B X Y, 0x100 any button (bonus) */
	int player;                           /* +0x04 */
	unsigned int seqPos;                  /* +0x08 the sequence line */
	unsigned int colour;                  /* +0x0c alpha in the top byte */
	float x;                              /* +0x10 */
	float y;                              /* +0x14 */
	float scale;                          /* +0x18 */
	float birthTime;                      /* +0x1c */
	float hitTime;                        /* +0x20 when it reaches the hit line */
	int state;                            /* +0x24 2: crossed out (missed) */
	int bonus;                            /* +0x28 */
};
OF_ASSERT_SIZE(struct SMIcon, 0x2c);

/* a floating text (0x24 bytes, 16 at 0x00312d10) */
struct SMText {
	int type;                             /* +0x00 0: free; 1-5 rating, 6 combo */
	unsigned int value;                   /* +0x04 the combo count */
	float x;                              /* +0x08 */
	float y;                              /* +0x0c */
	float duration;                       /* +0x10 */
	float age;                            /* +0x14 */
	float speed;                          /* +0x18 y speed */
	unsigned char rgba[4];                /* +0x1c */
	int dim;                              /* +0x20 dimmed while the icons blink */
};
OF_ASSERT_SIZE(struct SMText, 0x24);

/* a 2D sprite vertex (FVF 0x152: XYZ | NORMAL | DIFFUSE | TEX1), 4 at 0x00311b50 */
struct SMVertex {
	float pos[3];
	float normal[3];
	unsigned int diffuse;
	float uv[2];
};
OF_ASSERT_SIZE(struct SMVertex, 0x24);

/* the speakers (CScratchmatch_Scenics.cpp, 0x10 bytes, new'd in Init) */
struct SMScenic {
	struct BActor *actor;                 /* +0x0 */
	struct BActorInstance *instance;      /* +0x4 */
	struct BActorSubObject *anim;         /* +0x8 m_pAnimationSegment */
	float speed;                          /* +0xc */
};

#define SM_NUM_PLAYERS 4
#define SM_NUM_TIMERS 15
#define SM_NUM_ICONS 100        /* SC_ICONLISTSIZE */
#define SM_NUM_TEXTS 16
#define SM_SEQ_LIMIT 0x200      /* SCBBUF_LIMIT */
#define SM_NO_ICON 0xffff
#define SM_BONUS 0x100          /* an icon that takes any button */

/* ======================================================================== named globals */

/* .data */
#define sm_Variation SM_I(0x002b5b60)                 /* 1-3 (set by the module's Start) */
#define sm_VariationPackages ((const char **)SM_ADDR(0x002b5b60))   /* [variation] packages\scratch<n> */
#define sm_SampleNames ((const char **)SM_ADDR(0x002b5b74))        /* 10 samples\sfx\*.wav */
#define sm_MusicNames ((const char **)SM_ADDR(0x002b5b98))         /* [variation] wmas\scratch_match_v*.wma */
#define sm_Leader SM_U(0x002b5ba8)                    /* the camera's current leader rank */
#define sm_CameraFiles ((const char **)SM_ADDR(0x002b5ba8))        /* [variation] cams\cam<n>.lom */
#define sm_Levels SM_FV(0x002b5ca8)                   /* 4 x {start time, beat period, unused} */
#define sm_GenActorModel SM_A(0x002b5ce8)             /* "models\hovver_light" */
#define sm_GenActorInfo SM_A(0x002b5d28)              /* {BActor *, char anim[...]}, 100 bytes */
#define sm_GenActors SM_A(0x002b5d50)                 /* 2 x 0x20 bytes */
#define sm_AnimTextures SM_A(0x002b5da0)              /* 3 x 0x1c bytes: name, texture, frames, frame */
#define sm_DrawLightbeams SM_I(0x002b5df4)
#define sm_DrawBackground SM_I(0x002b5df8)
#define sm_DrawActors SM_I(0x002b5dfc)
#define sm_DrawExtras SM_I(0x002b5e00)
#define sm_StaticLightDefs SM_A(0x002b5e04)           /* 2 x 0x18 bytes */
#define sm_SmokeSprite ((struct CSpriteDef *)SM_ADDR(0x002b5e40))
#define sm_RespawnActor SM_P(0x002b5f40)
#define sm_AnimNames ((const char **)SM_ADDR(0x002b5f98))          /* 7 animation names */
#define sm_CharacterModels SM_A(0x002b5fb8)           /* 8 x 0x180: pod, model, low model */
#define sm_PlayerSpots SM_FV(0x002b68b8)              /* 4 x {x, z} */
#define sm_SpriteNames ((const char **)SM_ADDR(0x002b68d8))        /* 27 HUD sprites */
#define sm_FirstFreeText SM_U(0x002b6944)
#define sm_FirstFreeIcon SM_U(0x002b6948)

/* .bss */
#define sm_VarPackage SM_G(0x0030fe94, struct bkPackage *)    /* scVarPakIndex: packages\scratch<n> */
#define sm_Package SM_G(0x0030fe98, struct bkPackage *)       /* scPakIndex: packages\scratch */
#define sm_Speakers SM_G(0x0030fe9c, struct SMScenic *)
#define sm_ShowFramerate SM_I(0x0030fea0)
#define sm_SFX ((struct BSample **)SM_ADDR(0x0030feb0))       /* 10 mpSFX */
#define sm_MusicVolume SM_I(0x0030fed8)
#define sm_CrowdVolume SM_I(0x0030fedc)
#define sm_SpeechVolume SM_I(0x0030fee0)
#define sm_ScratchVolume SM_I(0x0030fee4)
#define sm_MusicFrequency SM_I(0x0030fee8)
#define sm_MusicBits SM_I(0x0030feec)
#define sm_MusicRatio SM_F(0x0030fef4)
#define sm_MusicPackage SM_G(0x0030ff14, struct bkPackage *)  /* mpChoonePakIndex */
#define sm_Music SM_G(0x0030ff18, struct BAudioStream *)      /* mpAudioStream */
#define sm_CamPos SM_FV(0x0030ff20)                    /* camera eye */
#define sm_CamTarget SM_FV(0x0030ff30)
#define sm_CamOffset SM_FV(0x0030ff40)                 /* wander: the next offset */
#define sm_CamOffsetFrom SM_FV(0x0030ff50)             /* wander: the previous offset */
#define sm_CamLeaderHeading SM_F(0x0030ff64)
#define sm_CamPlayerY SM_F(0x0030ff68)                 /* tweak "Player Cam Y Pos" */
#define sm_CamPlayerZ SM_F(0x0030ff6c)
#define sm_CamTargetY SM_F(0x0030ff70)
#define sm_CamTargetZ SM_F(0x0030ff74)
#define sm_CamFov SM_F(0x0030ff84)                     /* field-of-view scale */
#define sm_CamCloseUpPlayer SM_I(0x0030ff88)
#define sm_CamCloseUpDelay SM_F(0x0030ff80)            /* seconds until the next close-up */
#define sm_CamCloseUp SM_I(0x0030ff94)
#define sm_IntroSkipped SM_I(0x0030ff9c)
#define sm_HistoryLength SM_I(0x0030ffa0)              /* tweak "Button Biffer Size" (sic) */
#define sm_SequenceLines SM_U(0x0030ffa4)
#define sm_HalfWidth SM_I(0x0030ffa8)                  /* nScreenHalfWidth */
#define sm_HalfHeight SM_I(0x0030ffac)
#define sm_BeatPeriod SM_F(0x0030ffb4)                 /* tweak "Beat Period" */
#define sm_IconLead SM_F(0x0030ffb8)                   /* seconds from birth to the hit line */
#define sm_LeaderPeriod SM_F(0x0030ffec)               /* seconds per leader in the outro */
#define sm_HitRange SM_F(0x0030fff8)                   /* fHitAcceptanceRange */
#define sm_SequenceEndMargin SM_F(0x0030fffc)
#define sm_PracticeStart SM_F(0x00310000)
#define sm_GameLength SM_F(0x0030ffe0)                 /* seconds */
#define sm_GameVariation SM_I(0x00310008)              /* gxGameVariables: 1-3 */
#define sm_State SM_I(0x0031000c)                      /* 0 intro, 1 practice, 2 play, 3 outro */
#define sm_AliveCount SM_I(0x00310018)                 /* survival: players left */
#define sm_Level SM_I(0x0031001c)                      /* survival: speed level 0-3 */
#define sm_StartLives SM_I(0x00310020)
#define sm_Rank SM_IV(0x0031002c)                      /* players by rank (4) */
#define sm_Leaders SM_U(0x0031003c)                    /* players sharing first place */
#define sm_BeatTime SM_F(0x00310044)                   /* time of the last beat */
#define sm_OutroHold SM_I(0x00310048)                  /* a winner pressed a button */
#define sm_Finished SM_I(0x0031004c)                   /* pop the game mode */
#define sm_Paused SM_I(0x00310058)
#define sm_Started SM_I(0x0031005c)
#define sm_LastSurvivor SM_I(0x00310060)               /* survival: one player left */
#define sm_WinnerText SM_I(0x00310064)                 /* the HUD winner text exists */
#define sm_BeatStarted SM_I(0x00310068)
#define sm_Wander SM_I(0x0031006c)                     /* 1: wander camera, 0: close-up */
#define sm_Font SM_G(0x00310090, struct bdFont *)
#define sm_MenuWorld SM_G(0x00310094, struct CTweakMenu *)
#define sm_MenuSprite SM_G(0x00310098, struct CTweakMenu *)
#define sm_MenuCamera SM_G(0x0031009c, struct CTweakMenu *)
#define sm_MenuRender SM_G(0x003100a0, struct CTweakMenu *)
#define sm_MenuSound SM_G(0x003100a4, struct CTweakMenu *)
#define sm_Sequence SM_G(0x003100b0, unsigned int *)   /* mpun32Sequence: 4 entries a line */
#define sm_ClockNow ((LARGE_INTEGER *)SM_ADDR(0x003100b8))
#define sm_ClockFrequency ((LARGE_INTEGER *)SM_ADDR(0x003100c0))
#define sm_TimerElapsed(i) SM_F(0x003100cc + 0x10 * (i))
#define sm_TimerStart(i) ((LARGE_INTEGER *)SM_ADDR(0x003100d0 + 0x10 * (i)))
#define sm_ClockLast ((LARGE_INTEGER *)SM_ADDR(0x003101b8))
#define sm_PauseStart ((LARGE_INTEGER *)SM_ADDR(0x003101c0))
#define sm_FrameTime SM_D(0x003101d0)                  /* seconds since the last frame (double) */
#define sm_PausedTime ((LARGE_INTEGER *)SM_ADDR(0x003101d8))
#define sm_StageHeight SM_F(0x003101ec)
#define sm_BarY SM_F(0x003101f0)                       /* tweak "Bar Y Pos" */
#define sm_FadeState SM_I(0x003101f4)                  /* 1 fading out, 2 faded */
#define sm_Ambient SM_I(0x003101f8)                    /* tweak "Ambient Light Level" */
#define sm_FadeIn SM_I(0x003101fc)                     /* draw the fade-in */
#define sm_Arena SM_G(0x00310204, struct BSet *)       /* mpArena */
#define sm_Set SM_G(0x00310208, struct BSet *)         /* mpSet */
#define sm_SetLights SM_G(0x0031020c, struct BSet *)   /* mpSetLights */
#define sm_FadeAlpha SM_F(0x00310210)
#define sm_LightColours SM_U(0x00310220)               /* three bytes that rotate */
#define sm_StaticLights ((struct BLight **)SM_ADDR(0x00310224))    /* mapStaticLights[2] */
#define sm_SpotHeight SM_F(0x0031022c)
#define sm_SpotTargetY SM_F(0x00310230)
#define sm_SpotTargetZ SM_F(0x00310234)
#define sm_PodActor SM_G(0x00310240, struct BActor *)  /* mxRotationLights.pLightPod */
#define sm_Pod ((struct BActorInstance *)SM_ADDR(0x00310250))
#define sm_PodAnim SM_G(0x003103c0, struct BActorSubObject *)
#define sm_BeamsActor SM_G(0x003103c4, struct BActor *)
#define sm_Beams ((struct BActorInstance *)SM_ADDR(0x003103d0))
#define sm_BeamsAnim SM_G(0x00310540, struct BActorSubObject *)
#define sm_PodPos SM_FV(0x00310544)
#define sm_SpotLights SM_A(0x00310560)                 /* 2 x 0xe8: light, pos, target, ... */
#define sm_SmokePlanes SM_A(0x00310730)                /* 2 x 0x38 */
#define sm_Effects SM_A(0x003107a0)                    /* 8 x 0x1c, type 0xffff free */
#define sm_Players ((struct SMPlayer *)SM_ADDR(0x00310890))
#define sm_Sprites ((struct bdTexture **)SM_ADDR(0x00311a90))      /* mapSprites[27] */
#define sm_IconStartY SM_I(0x00311b08)
#define sm_IconFade SM_F(0x00311b0c)
#define sm_LaneSpacing SM_F(0x00311b10)
#define sm_HitLineY SM_F(0x00311b14)
#define sm_IconEndY SM_F(0x00311b18)
#define sm_IconScale SM_F(0x00311b1c)
#define sm_PulseTime SM_F(0x00311b20)
#define sm_Pulse SM_FV(0x00311b24)                     /* 4 */
#define sm_FlashAlpha SM_F(0x00311b34)
#define sm_Pulsing SM_IV(0x00311b38)                   /* 4 */
#define sm_Blink SM_I(0x00311b48)
#define sm_Flash SM_I(0x00311b4c)
#define sm_Quad ((struct SMVertex *)SM_ADDR(0x00311b50))
#define sm_Icons ((struct SMIcon *)SM_ADDR(0x00311be0))
#define sm_Texts ((struct SMText *)SM_ADDR(0x00312d10))

/* the HUD sprites (sm_SpriteNames order) */
enum {
	SMSPR_FRAMEBACK, SMSPR_TOPFRAME1, SMSPR_TOPFRAME2, SMSPR_TOPFRAME3, SMSPR_TOPFRAME4,
	SMSPR_FRAMEFLASH, SMSPR_CLUSTEREMPTY, SMSPR_CLUSTERALPHA, SMSPR_CLUSTERA, SMSPR_CLUSTERB,
	SMSPR_CLUSTERX, SMSPR_CLUSTERY, SMSPR_SHADA, SMSPR_SHADB, SMSPR_SHADX, SMSPR_SHADY,
	SMSPR_SCOREA, SMSPR_SCOREB, SMSPR_SCOREX, SMSPR_SCOREY, SMSPR_BLANK128, SMSPR_BLANK256,
	SMSPR_QUESTION, SMSPR_LIFEON, SMSPR_LIFEOFF, SMSPR_CROSS, SMSPR_BUTTONFLASH
};

/* the timers (SMTimer_*) */
enum {
	SMT_STATE = 0, SMT_GAME = 1, SMT_BEAT = 2, SMT_DANCE = 3, SMT_LIGHTS = 4,
	SMT_PULSE0 = 5,       /* 5-8: one per player */
	SMT_BLINK = 9, SMT_CAMERA = 10, SMT_FADE = 11, SMT_EFFECTS = 12, SMT_CLOSEUP = 13,
	SMT_ANIMTEX = 14
};

/* ======================================================================== helpers */

/* the game's inlined MSVC rand(): Core_Rand's seed at 0x00285630 */
extern unsigned int DAT_00285630;
static inline unsigned int sm_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (DAT_00285630 >> 16) & 0x7fff;
}

/* the debug assertion of the Babel engine (prints, then breaks into the debugger) */
#define SM_ASSERT_FAIL(file, line, what) \
	(bkPrintf("C:\\BlitzParty\\Game\\Src\\ScratchMatch\\" file "(%d) : " what "\n", (line)), DbgBreakPoint())

/* x87 _ftol (0x001de998): truncating conversion */
#define SM_FTOL(x) ((int)(longlong)(x))

/* bKernel heap: operator delete */
#define SM_DELETE(p) bkHeapFree((void *)(p))

/* an unsigned int converted the way the x87 code did (fild, plus 2^32 when negative) */
static inline float sm_ufloat(unsigned int v)
{
	return (float)v;
}

/* ======================================================================== other modules' globals */

extern float DAT_002855f0[16];        /* bkernel/bdisplay: the identity matrix */
extern float DAT_00285634[4];         /* the identity quaternion */
extern int DAT_002baed4[];            /* core: player records, 10 ints each: +0 character,
                                         +1 controller (1 human), +3 pad port, +5 score */
extern int DAT_002bae6c;              /* core: sound effect volume 0-255 */
extern int DAT_002bae70;              /* core: handicap / difficulty 0-2 */
extern int DAT_002bae98;              /* core: start state (2: no restart) */
extern int DAT_002b9c58;              /* core_modes_b */
extern unsigned char DAT_002b9968[];  /* core_modes_b: the score table object */
extern unsigned char DAT_002ba750[];  /* core: the mode stack */
extern int DAT_00290960;              /* classlib_ui */
extern unsigned char DAT_00290718[];  /* classlib_ui: player colours, 0x60 bytes per character */
extern int DAT_002ecdec;              /* classlib_ui: the game is paused */
extern void *DAT_002ecfac;            /* classlib_ui: the HUD object */
extern void *DAT_002ecc74;            /* classlib_ui: the text table; +0x20 entries of 0x10 bytes */
extern unsigned char DAT_002ecd90[];  /* classlib_ui: the pause object */
extern void *DAT_002db808;            /* engine_misc: the DirectSound object */
extern unsigned int DAT_002bc490[256];  /* bdisplay: green << 8 */
extern unsigned int DAT_002bc898[256];  /* bdisplay: alpha << 24 */
extern unsigned int DAT_002bce18[256];  /* bdisplay: red << 16 */
extern unsigned int DAT_002bd640[256];  /* bdisplay: blue */

#define SM_HUD ((int)(uintptr_t)DAT_002ecfac)
#define sm_PlayerRecord(i) (&DAT_002baed4[(i) * 10])

/* XAPI (src/xkernel) */
BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER *count);
BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER *frequency);

#endif
