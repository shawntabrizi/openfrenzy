/*
TANKEDUP_B_EXT.H

Calls from tankedup_b into other modules whose table prototype (of_decls.h) does not have the
shape of the x86 code, or is still Ghidra's guess (tankedup_a is written at the same time).
Each macro calls the function through a cast to the shape read from the call sites and the
callee's `ret N` (thiscall `this` first). Owner: tankedup_b.
*/

#ifndef TANKEDUP_B_EXT_H
#define TANKEDUP_B_EXT_H

#include "tankedup_b.h"

/* ---------- tankedup_a */

/* 0x001cebd0 (entity init; the ECX it gets is not used) */
#define TA_InitEntity(ent) TU_CALL(FUN_001cebd0, void (*)(void *))(ent)
/* 0x001c3790, 0x001c3570, 0x001c36e0, 0x001c3700: the life timer at ENT_TIMER
   (CTankTimer_Ctor, _Reset, _SetLength, _StartOrResume; thiscall) */
#define TA_TimerCtor(timer) TU_CALL(FUN_001c3790, void (*)(void *))(timer)
#define TA_TimerReset(timer) TU_CALL(FUN_001c3570, int (*)(void *))(timer)
#define TA_TimerSetLength(timer, seconds) TU_CALL(FUN_001c36e0, void (*)(void *, float))(timer, seconds)
#define TA_TimerStart(timer) TU_CALL(FUN_001c3700, void (*)(void *))(timer)
/* the world (thiscall): gravity, the mode (CTankWorld_GetMode; the race's lap count, 0: no
   race), ground height under a
   sphere (and the ground normal), sphere sweep, ray length, the lap line test and the
   collision move */
#define TA_WorldGravity(world) TU_CALL(FUN_001caa70, float (*)(void *))(world)
#define TA_WorldLaps(world) TU_CALL(FUN_001caa80, int (*)(void *))(world)
#define TA_WorldGroundHeight(world, pos, radius, out) \
	TU_CALL(FUN_001cb1a0, float (*)(void *, float *, float, float *))(world, pos, radius, out)
#define TA_WorldHitSphere(world, from, to, radius, flag) \
	TU_CALL(FUN_001cb3f0, int (*)(void *, float *, float *, float, int))(world, from, to, radius, flag)
#define TA_WorldRayLength(world, from, to) \
	TU_CALL(FUN_001cd8c0, float (*)(void *, float *, float *))(world, from, to)
#define TA_WorldCheckLine(world, pos, vel, flags) \
	TU_CALL(FUN_001cb970, int (*)(void *, float *, float *, unsigned int *))(world, pos, vel, flags)
#define TA_WorldMove(world, pos, vel, rot, radius, flags, restitution, onGround, friction, out, noKill) \
	TU_CALL(FUN_001cd3d0, int (*)(void *, float *, float *, float *, float, unsigned int *, float, \
		int *, float *, float *, int *))(world, pos, vel, rot, radius, flags, restitution, onGround, \
		friction, out, noKill)
/* the lights (thiscall) */
#define TA_LightsEnable(lights, on) TU_CALL(FUN_001c3960, void (*)(void *, int))(lights, on)
#define TA_LightsEnableTank(lights, on) TU_CALL(FUN_001c38c0, void (*)(void *, int))(lights, on)
#define TA_LightsRestore(lights) TU_CALL(FUN_001c3850, void (*)(void *))(lights)
#define TA_LightsRemove(lights, light) TU_CALL(FUN_001c3990, void (*)(void *, int))(lights, light)
#define TA_SetAmbient(r, g, b) TU_CALL(FUN_001c38a0, void (*)(int, int, int))(r, g, b)
/* 0x001cdd50 (cdecl): a glow sprite at pos, turned by angle (bdDrawSprite3D) */
#define TA_DrawGlow(pos, angle, texture) \
	TU_CALL(FUN_001cdd50, void (*)(float *, float, void *))(pos, angle, texture)
/* the entities' per-frame steps of tankedup_a (thiscall on CTank_Entities) */
#define TA_EntsFree(ents) TU_CALL(FUN_001ce300, void (*)(void *))(ents)
#define TA_EntsThink(ents) TU_CALL(FUN_001cea10, void (*)(void *))(ents)
#define TA_EntsStep2(ents) TU_CALL(FUN_001ceba0, void (*)(void *))(ents)
#define TA_EntsStep3(ents) TU_CALL(FUN_001ceb30, void (*)(void *))(ents)
/* the entities' message handlers (thiscall on CTank_Entities) */
#define TA_HandleMessage(ents, msg) TU_CALL(FUN_001ce130, void (*)(void *, void *))(ents, msg)
#define TA_SendMessageNow(ents, ent, flag, msg) \
	TU_CALL(FUN_001cde40, void (*)(void *, void *, int, void *))(ents, ent, flag, msg)
#define TA_TurnEntity(ents, id, turn) TU_CALL(FUN_001ce870, void (*)(void *, int, float))(ents, id, turn)
#define TA_EntityCleanup(ent) TU_CALL(FUN_001ce280, void (*)(void *))(ent)
#define TA_EntsSetup(ents) TU_CALL(FUN_001ce290, void (*)(void *))(ents)
/* the players (CTank_Players, thiscall): a tank's speed, whether two owners are friends,
   a player event, a player's rank */
#define TA_PlayersSetSpeed(players, speed, id) \
	TU_CALL(FUN_001c84b0, void (*)(void *, float, int))(players, speed, id)
#define TA_IsFriendly(players, a, b) TU_CALL(FUN_001c82e0, int (*)(void *, int, int))(players, a, b)
#define TA_PlayersShake(players, id, kind) TU_CALL(FUN_001c8e80, void (*)(void *, int, int))(players, id, kind)
#define TA_PlayersRank(players, id) TU_CALL(FUN_001c8ca0, int (*)(void *, int))(players, id)
/* the camera (CTank_Camera, thiscall): its heading */
#define TA_CameraHeading(camera) TU_CALL(FUN_001c66f0, float (*)(void *))(camera)
/* 0x001ce940: an angle wrapped to 0..2 pi (returned on the x87 stack) */
#define TA_WrapAngle(angle) TU_CALL(FUN_001ce940, float (*)(float))(angle)
/* the debris (TankDebris_Add, thiscall) and the explosions (CTankBoom_Add, thiscall) */
#define TA_DebrisAdd(debris, kind, pos, vel, life, colour) \
	TU_CALL(FUN_001c3540, void (*)(void *, int, float *, float *, int, unsigned int))(debris, kind, pos, \
		vel, life, colour)
#define TA_BoomAdd(boom, pos, height) TU_CALL(FUN_001c3180, void (*)(void *, float *, float))(boom, pos, height)
/* the lights: a firing flash (CTankLights_AddFireFlash, thiscall) */
#define TA_LightsFireFlash(lights, pos) TU_CALL(FUN_001c3e00, void (*)(void *, float *))(lights, pos)
/* the spawn points (thiscall): a named point of a player (pos[4], rot[4]); a random point of
   a list */
#define TA_SpawnFind(spawn, name, player) \
	TU_CALL(FUN_001c4110, float *(*)(void *, const char *, int))(spawn, name, player)
#define TA_SpawnRandom(spawn, name, out) \
	TU_CALL(FUN_001c4420, void (*)(void *, const char *, float *))(spawn, name, out)
/* the point list of a name (pos[4], rot[4], +0x20 weight, +0x24 next) and its length */
#define TA_SpawnGetPoints(spawn, name) TU_CALL(FUN_001c40d0, void *(*)(void *, const char *))(spawn, name)
#define TA_SpawnGetCount(spawn, name) TU_CALL(FUN_001c4160, int (*)(void *, const char *))(spawn, name)

/* ---------- engine_misc: 0x00052ea0 and 0x00053af0 return the inner test's result in the
   original (eax of 0x00052770 / 0x00052da0), but engine_misc defines them void. Calling a
   void function through an int cast reads a garbage w0 on arm64, so these wrappers repeat
   the two small wrapper bodies and call the int-returning inner tests directly (reported
   for integration). */
#include "engine_misc_maths.h"

/* 0x00052ea0: a segment {origin[4], delta[4]} against a sphere */
static inline int TU_SegmentSphere(float *seg, void *sphere, void *hit)
{
	struct BRay ray;
	float inv;

	ray.origin[0] = seg[0];
	ray.origin[1] = seg[1];
	ray.origin[2] = seg[2];
	ray.origin[3] = seg[3];
	ray.length = sqrtf(seg[6] * seg[6] + seg[5] * seg[5] + seg[4] * seg[4]);
	inv = 1.0f / ray.length;
	ray.dir[0] = inv * seg[4];
	ray.dir[1] = inv * seg[5];
	ray.dir[2] = inv * seg[6];
	ray.dir[3] = 0.0f; /* not set by the original */
	return bmRaySphere(&ray, (struct BSphere *)sphere, (struct BCollHit *)hit);
}

/* 0x00053af0: sphere a moving by vec against sphere b */
static inline int TU_SweepSphereSphere(void *a, void *b, float *vec, void *hit)
{
	float len, inv, dir[4];

	len = sqrtf(vec[2] * vec[2] + vec[1] * vec[1] + vec[0] * vec[0]);
	inv = 1.0f / len;
	dir[0] = inv * vec[0];
	dir[1] = inv * vec[1];
	dir[2] = inv * vec[2];
	dir[3] = 0.0f; /* not set by the original */
	return bmSweepSphereSphere((struct BSphere *)a, (struct BSphere *)b, dir, len,
		(struct BCollHit *)hit);
}

/* |a - b|^2: 0x00051540 returns it on the x87 stack (bkernel's definition is empty) */
static inline float tu_dist_sq(const float *a, const float *b)
{
	float x = a[0] - b[0];
	float y = a[1] - b[1];
	float z = a[2] - b[2];

	return z * z + x * x + y * y;
}

#endif
