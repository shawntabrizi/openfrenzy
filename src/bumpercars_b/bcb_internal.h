/*
BCB_INTERNAL.H

Private to the bumpercars_b module: calls into other modules with the shape the x86 code
uses (ECX `this` first, floats as floats, the real argument count). The table prototypes
of most of these functions are still Ghidra's guesses, so every call goes through a cast.
The shapes were read from the call sites (tools/calls.py) and the callees' `ret N`
(tools/abi.py); the owners must agree when they write the functions.
*/

#ifndef OPENFRENZY_BCB_INTERNAL_H
#define OPENFRENZY_BCB_INTERNAL_H

#include "bumpercars_b.h"

#define BCB_FN(fn, ret, params) ((ret(*) params)(fn))

/* ---------- sub-objects of CBCar_Game (bumpercars_a), by their original address */
#define BCB_DEBUG BCB_ADDR(0x002ed470)    /* CBCar_Debug */
#define BCB_EFFECTS BCB_ADDR(0x002ed4b4)  /* the debris effects */
#define BCB_CAMERA BCB_ADDR(0x002efa98)   /* CBCar_Camera */
#define BCB_LIGHT BCB_ADDR(0x002f0a5c)    /* CBCar_Light */
#define BCB_SOUND BCB_ADDR(0x002f0778)    /* CBCar_Sound */

/* ---------- core */
#define bcb_Rand() ((int)BCB_FN(FUN_00011730, unsigned int, (void))())
#define bcb_RandUnit() BCB_FN(FUN_00011750, float, (void))()
#define bcb_Vec3Sub(out, a, b) BCB_FN(FUN_00019230, void, (float *, float *, float *))((out), (a), (b))
#define bcb_Vec3Length(v) BCB_FN(FUN_0001ea30, float, (float *))(v)
/* the score table's ranking (core_modes_b object 0x002b9968) */
extern unsigned char DAT_002b9968[];
#define bcb_RankScores() BCB_FN(FUN_0001bd70, int, (void *))(DAT_002b9968)
/* core's game settings (a Settings object, bumpercars_a 0x00082870 reads a key) */
extern void *DAT_002ba9a0;
#define bcb_GetSetting(key) BCB_FN(FUN_00082870, char *, (void *, const char *))(DAT_002ba9a0, (key))
/* recolours an actor with the colour set (core 0x0002bb00) */
#define bcb_SetCharacterColours(inst, set) BCB_FN(FUN_0002bb00, void, (void *, int))((inst), (set))

/* ---------- classlib */
#define bcb_WrapAngle(a) BCB_FN(FUN_000703a0, float, (float))(a)
#define bcb_CreateOverlay(desc) BCB_FN(FUN_00073ae0, void *, (void *))(desc)
#define bcb_HudSetFraction(player, value, max) \
	BCB_FN(FUN_000789c0, void, (void *, int, int, int))((void *)DAT_002ecfac, (player), (value), (max))
/* the angle a is moved towards b by at most step (demolition_c 0x00101f50) */
#define bcb_ApproachAngle(a, b, step) BCB_FN(FUN_00101f50, float, (float, float, float))((a), (b), (step))

/* ---------- FoxHunt's text reader methods */
#define bcb_ReadLine(reader, buf) \
	BCB_FN(FUN_001386c0, char *, (struct BCarTextReader *, char *))((reader), (buf))
#define bcb_NextLine(reader) BCB_FN(FUN_00138720, void, (struct BCarTextReader *))(reader)

/* ---------- TankedUp: timer duration (0x001c36e0), pause (0x001c3770), destructor */
#define bcb_TimerSetDuration(t, seconds) \
	BCB_FN(FUN_001c36e0, void, (struct BCarTimer *, float))((t), (seconds))
#define bcb_TimerPause(t) BCB_FN(FUN_001c3770, void, (struct BCarTimer *))(t)
#define bcb_TimerDtor(t) BCB_FN(FUN_001d5950, void, (struct BCarTimer *))(t)
/* texture stage helpers of other minigames, used before/after reflections */
#define bcb_StageReset1() BCB_FN(FUN_001d6820, void, (void))()
#define bcb_StageReset2() BCB_FN(FUN_001d6860, void, (void))()
#define bcb_StageRestore() BCB_FN(FUN_0018e640, void, (void))()
#define bcb_DrawWater() BCB_FN(FUN_001bea60, void, (void))()
/* engine tail (engine_misc 0x001ec260): the player's shadow / marker */
#define bcb_DrawPlayerMarker(ai, reflect) BCB_FN(FUN_001ec260, void, (void *, int))((ai), (reflect))

/* ---------- bumpercars_a: the AI driver of a player (player + 0x10) */
#define bcb_AIInit(ai, player, levels) \
	BCB_FN(FUN_00089770, int, (void *, struct BCarPlayer *, int *))((ai), (player), (levels))
#define bcb_AIShutdown(ai) BCB_FN(FUN_000890d0, void, (void *))(ai)
#define bcb_AIStart(ai) BCB_FN(FUN_000890f0, void, (void *))(ai)
#define bcb_AIPause(ai) BCB_FN(FUN_00089710, void, (void *))(ai)
#define bcb_AIResume(ai) BCB_FN(FUN_00089730, void, (void *))(ai)
#define bcb_AIUpdate(ai, dt) BCB_FN(FUN_0008bdd0, void, (void *, float))((ai), (dt))

/* ---------- bumpercars_a: CBCar_Debug switches (0x002ed470) */
#define bcb_DebugNoLights() BCB_FN(FUN_0008c000, int, (void *))(BCB_DEBUG)
#define bcb_DebugWater() BCB_FN(FUN_0008c030, int, (void *))(BCB_DEBUG)
#define bcb_DebugReflections() BCB_FN(FUN_0008c040, int, (void *))(BCB_DEBUG)
#define bcb_DebugZState(which) BCB_FN(FUN_0008c060, unsigned int, (void *, int))(BCB_DEBUG, (which))
#define bcb_DebugCollision() BCB_FN(FUN_0008c070, int, (void *))(BCB_DEBUG)
#define bcb_DebugDriverReflections() BCB_FN(FUN_0008c080, int, (void *))(BCB_DEBUG)
#define bcb_DebugHeadlights() BCB_FN(FUN_0008c090, int, (void *))(BCB_DEBUG)

/* ---------- bumpercars_a: the debris effects (0x002ed4b4) */
#define bcb_EffectAdd(type, pos, vel, a4, a5) \
	BCB_FN(FUN_0008cba0, void, (void *, int, float *, float *, int, int))(BCB_EFFECTS, (type), (pos), (vel), (a4), (a5))
#define bcb_EffectAddCone(type, pos, a, b, c, colour) \
	BCB_FN(FUN_0008cbd0, void, (void *, int, float *, float, float, float, int))(BCB_EFFECTS, (type), (pos), (a), (b), (c), (colour))
#define bcb_EffectsUpdate(dt) BCB_FN(FUN_0008cb80, void, (void *, float))(BCB_EFFECTS, (dt))

/* ---------- bumpercars_a: CBCar_Camera (0x002efa98) */
#define bcb_CameraAngle() BCB_FN(FUN_0008e8d1, float, (void *))(BCB_CAMERA)
#define bcb_CameraRank(player) BCB_FN(FUN_0008fe4b, int, (void *, int))(BCB_CAMERA, (player))
#define bcb_CameraIsSingle(view) BCB_FN(FUN_0009194d, int, (void *, int *))(BCB_CAMERA, (view))

/* ---------- bumpercars_a: CBCar_Sound (0x002f0778) */
#define bcb_SoundPlay(sample, volume, a4, channel) \
	BCB_FN(FUN_00092120, int, (void *, int, int, int, int))(BCB_SOUND, (sample), (volume), (a4), (channel))
#define bcb_SoundFinished(channel) BCB_FN(FUN_00092290, int, (void *, int))(BCB_SOUND, (channel))
#define bcb_SoundSet(channel, what, value) \
	BCB_FN(FUN_000922d0, void, (void *, int, int, int))(BCB_SOUND, (channel), (what), (value))
#define bcb_SoundCharacter(sample, character, volume) \
	BCB_FN(FUN_00092310, void, (void *, int, int, int))(BCB_SOUND, (sample), (character), (volume))
#define bcb_SoundSpeak(player, sample, volume, pos, a6) \
	BCB_FN(FUN_00092450, void, (void *, int, int, int, float *, float))(BCB_SOUND, (player), (sample), (volume), (pos), (a6))

/* ---------- bumpercars_a: CBCar_Light (0x002f0a5c) */
#define bcb_LightAddSpot(name, entity) \
	BCB_FN(FUN_00092500, int, (void *, char *, struct BCarEntity *))(BCB_LIGHT, (name), (entity))
#define bcb_LightSetColour(spot, r, g, b) \
	BCB_FN(FUN_00092a10, void, (void *, int, int, int, int))(BCB_LIGHT, (spot), (r), (g), (b))
#define bcb_LightSelect(on) BCB_FN(FUN_00092560, void, (void *, int))(BCB_LIGHT, (on))
#define bcb_LightRestore() BCB_FN(FUN_00092610, void, (void *))(BCB_LIGHT)

/* ---------- bumpercars_a: the arena's debris colours and flames */
#define bcb_SetDebrisColours(a, b, c, d) \
	BCB_FN(FUN_00082b20, void, (float *, float *, float *, float *))((a), (b), (c), (d))

/* the colour bytes of a character (classlib_ui table, 0x60 bytes per character) */
#define BCB_CHAR_COLOUR(character, i) (DAT_00290718[(character) * 0x60 + (i)])

#endif
