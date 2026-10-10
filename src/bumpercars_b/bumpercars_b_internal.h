/*
BUMPERCARS_B_INTERNAL.H

Module bumpercars_b: calls into other modules with the shapes that the original code uses.

Most callees in bumpercars_a are not decompiled yet; their table prototypes are Ghidra's
guesses (a lost ECX `this`, floats typed undefined4, lost results). The wrappers below call
them through BCB_CALL with the shape read from the original call sites (ECX = `this`, first
parameter). When bumpercars_a defines them, it must use these shapes (see the list in the
final report of bumpercars_b), or the wrappers here must follow.

Objects (thiscall `this`) inside the BumperCars block:
  0x002ed470  debug switches (CBCar_Debug)      0x002ed4b4  debris effects
  0x002efa98  the camera (CBCar_Camera)         0x002f0778  sounds (CBCar_Sound)
  0x002f0a5c  lights (CBCar_Light)              0x002f1380  the flames (CBCar_Flame)
*/

#ifndef OPENFRENZY_BUMPERCARS_B_INTERNAL_H
#define OPENFRENZY_BUMPERCARS_B_INTERNAL_H

#include "bumpercars_b.h"

#define BCB_DEBUG BCB_ADDR(0x002ed470)
#define BCB_EFFECTS BCB_ADDR(0x002ed4b4)
#define BCB_CAMERA BCB_ADDR(0x002efa98)
#define BCB_SOUND BCB_ADDR(0x002f0778)
#define BCB_LIGHTS BCB_ADDR(0x002f0a5c)
#define BCB_FLAME BCB_ADDR(0x002f1380)

/* 0x002f138c  the floaty score texts (CBCar_FloatyScore) */
#define g_FloatyScore BCB_VAR(struct BCarFloatyScore *, 0x002f138c)

/* the engine sound range (set by bumpercars_a from its configuration) */
#define g_EnginePitchMin BCB_VAR(int, 0x002f2048)
#define g_EngineVolumeMax BCB_VAR(int, 0x002f204c)
#define g_EngineVolumeMin BCB_VAR(int, 0x002f2050)
#define g_EnginePitchMax BCB_VAR(int, 0x002f2054)

/* ---------- bumpercars_a: sounds (this = 0x002f0778) */

/* 0x00092120: plays `sample` at `volume` on `channel` (-1: a free one); returns the channel */
static inline int BCarSound_Play(void *sound, int sample, int volume, int arg, int channel)
{
	return BCB_CALL(FUN_00092120, int (*)(void *, int, int, int, int))(sound, sample, volume,
		arg, channel);
}

/* 0x00092290: nonzero when `channel` has stopped playing (0 for -1) */
static inline int BCarSound_IsFree(void *sound, int channel)
{
	return BCB_CALL(FUN_00092290, int (*)(void *, int))(sound, channel);
}

/* 0x000922d0: sets a parameter (0xc: the pitch) of a channel */
static inline void BCarSound_SetParam(void *sound, int channel, int param, int value)
{
	BCB_CALL(FUN_000922d0, void (*)(void *, int, int, int))(sound, channel, param, value);
}

/* 0x00092310: plays a character's voice sample */
static inline void BCarSound_PlayVoice(void *sound, int sample, int character, int volume)
{
	BCB_CALL(FUN_00092310, void (*)(void *, int, int, int))(sound, sample, character, volume);
}

/* 0x00092450: a speech of player `player` (a random sample of group `speech`) */
static inline void BCarSound_Speak(void *sound, int player, int speech, int volume, float *pos,
	float arg)
{
	BCB_CALL(FUN_00092450, void (*)(void *, int, int, int, float *, float))(sound, player,
		speech, volume, pos, arg);
}

/* ---------- bumpercars_a: lights (this = 0x002f0a5c) */

/* 0x00092500: attaches the light named `name` to an entity */
static inline int BCarLight_Attach(void *lights, const char *name, struct BCarEntity *entity)
{
	return BCB_CALL(FUN_00092500, int (*)(void *, const char *, struct BCarEntity *))(lights,
		name, entity);
}

/* 0x00092560 */
static inline void BCarLight_Enable(void *lights, int on)
{
	BCB_CALL(FUN_00092560, void (*)(void *, int))(lights, on);
}

/* 0x00092610 */
static inline void BCarLight_Restore(void *lights)
{
	BCB_CALL(FUN_00092610, void (*)(void *))(lights);
}

/* 0x00092a10: colours the light of an entity */
static inline void BCarLight_SetColour(void *lights, int light, int r, int g, int b)
{
	BCB_CALL(FUN_00092a10, void (*)(void *, int, int, int, int))(lights, light, r, g, b);
}

/* ---------- bumpercars_a: debug switches (this = 0x002ed470) */

#define BCB_DEBUG_GET(fn) BCB_CALL(fn, int (*)(void *))(BCB_DEBUG)

/* ---------- bumpercars_a: debris effects (this = 0x002ed4b4) */

/* 0x0008cba0 */
static inline void BCarEffects_Burst(void *flames, int type, float *pos, float *vel, int arg,
	int colour)
{
	BCB_CALL(FUN_0008cba0, void (*)(void *, int, float *, float *, int, int))(flames, type, pos,
		vel, arg, colour);
}

/* 0x0008cbd0 */
static inline void BCarEffects_Spark(void *flames, int type, float *pos, float a, float b,
	float c, int count)
{
	BCB_CALL(FUN_0008cbd0, void (*)(void *, int, float *, float, float, float, int))(flames,
		type, pos, a, b, c, count);
}

/* 0x0008cb80 */
static inline void BCarEffects_Update(void *flames, float dt)
{
	BCB_CALL(FUN_0008cb80, void (*)(void *, float))(flames, dt);
}

/* ---------- bumpercars_a: camera (this = 0x002efa98) */

/* 0x0008e8d1: the camera's heading */
static inline float BCarCamera_GetAngle(void *camera)
{
	return BCB_CALL(FUN_0008e8d1, float (*)(void *))(camera);
}

/* 0x0008fe4b: the rank of a player */
static inline int BCarCamera_GetRank(void *camera, int player)
{
	return BCB_CALL(FUN_0008fe4b, int (*)(void *, int))(camera, player);
}

/* 0x0009194d: nonzero in the single-view mode; *player is the player viewed */
static inline int BCarCamera_GetViewPlayer(void *camera, int *player)
{
	return BCB_CALL(FUN_0009194d, int (*)(void *, int *))(camera, player);
}

/* ---------- bumpercars_a: the flames of the arena (CBCar_Flame.cpp, this = 0x002f1380) */

static inline void BCarFlame_Draw(void *flame, int reflection, int pass)
{
	BCB_CALL(FUN_000946d0, void (*)(void *, int, int))(flame, reflection, pass);
}

static inline float BCarFlame_Hit(void *flame, float *pos, float radius)
{
	return BCB_CALL(FUN_00094880, float (*)(void *, float *, float))(flame, pos, radius);
}

static inline void BCarFlame_Pause(void *flame)
{
	BCB_CALL(FUN_000948f0, void (*)(void *))(flame);
}

static inline void BCarFlame_Resume(void *flame)
{
	BCB_CALL(FUN_00094920, void (*)(void *))(flame);
}

static inline void BCarFlame_Delete(void *flame)
{
	BCB_CALL(FUN_00094a00, void (*)(void *))(flame);
}

static inline void BCarFlame_Update(void *flame, float dt, int control)
{
	BCB_CALL(FUN_00094af0, void (*)(void *, float, int))(flame, dt, control);
}

static inline int BCarFlame_Inside(void *flame, float *pos, float radius, float *out)
{
	return BCB_CALL(FUN_00095150, int (*)(void *, float *, float, float *))(flame, pos, radius,
		out);
}

/* ---------- bumpercars_a: the AI driver of a player (this = &player->ai) */

/* ---------- other modules */

/* FoxHunt 0x001386c0: copies the next line of the reader into `buffer`; returns the line
without its leading blanks. 0x00138720 moves the reader to the next line. */
static inline char *BCarText_ReadLine(struct BCarTextReader *reader, char *buffer)
{
	return BCB_CALL(FUN_001386c0, char *(*)(struct BCarTextReader *, char *))(reader, buffer);
}

static inline void BCarText_NextLine(struct BCarTextReader *reader)
{
	BCB_CALL(FUN_00138720, void (*)(struct BCarTextReader *))(reader);
}

/* TankedUp 0x001c36e0: sets the duration (seconds) of a timer; 0x001c3770 pauses it;
0x001d5950 is its destructor */
static inline void BCarTimer_SetDuration(struct BCarTimer *timer, float seconds)
{
	BCB_CALL(FUN_001c36e0, void (*)(struct BCarTimer *, float))(timer, seconds);
}

static inline void BCarTimer_Pause(struct BCarTimer *timer)
{
	BCB_CALL(FUN_001c3770, void (*)(struct BCarTimer *))(timer);
}

static inline void BCarTimer_Destruct(struct BCarTimer *timer)
{
	BCB_CALL(FUN_001d5950, void (*)(struct BCarTimer *))(timer);
}

/* Demolition 0x00101f50: an angle moved from `from` toward `to` by at most `step` */
static inline float BCarMath_TurnTo(float from, float to, float step)
{
	return BCB_CALL(FUN_00101f50, float (*)(float, float, float))(from, to, step);
}

/* the debug option table (0x00082870, this = *(0x002ba9a0)): the value of an option */
extern void *DAT_002ba9a0;
static inline char *BCarOptions_Get(const char *name)
{
	return BCB_CALL(FUN_00082870, char *(*)(void *, const char *))(DAT_002ba9a0, name);
}

#endif
