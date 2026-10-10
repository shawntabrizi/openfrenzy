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
/* 0x001c3790, 0x001c3570, 0x001c36e0, 0x001c3700: the physics part at ENT_PHYS (thiscall) */
#define TA_PhysCtor(phys) TU_CALL(FUN_001c3790, void (*)(void *))(phys)
#define TA_PhysInit(phys) TU_CALL(FUN_001c3570, int (*)(void *))(phys)
#define TA_PhysSetLife(phys, life) TU_CALL(FUN_001c36e0, void (*)(void *, float))(phys, life)
#define TA_PhysStart(phys) TU_CALL(FUN_001c3700, void (*)(void *))(phys)
/* the world (thiscall): gravity, laps of the race (0: no race), ground height under a
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
/* 0x001cdd50 (cdecl) */
#define TA_DrawTokenShadow(pos, tokens, shadow) \
	TU_CALL(FUN_001cdd50, void (*)(float *, void *, void *))(pos, tokens, shadow)
/* the entities' message handlers (thiscall on CTank_Entities) */
#define TA_HandleMessage(ents, msg) TU_CALL(FUN_001ce130, void (*)(void *, void *))(ents, msg)
#define TA_SendMessageNow(ents, ent, flag, msg) \
	TU_CALL(FUN_001cde40, void (*)(void *, void *, int, void *))(ents, ent, flag, msg)
#define TA_TurnEntity(ents, id, turn) TU_CALL(FUN_001ce870, void (*)(void *, int, float))(ents, id, turn)
#define TA_EntityCleanup(ent) TU_CALL(FUN_001ce280, void (*)(void *))(ent)
#define TA_EntsSetup(ents) TU_CALL(FUN_001ce290, void (*)(void *))(ents)
/* the camera (thiscall) */
#define TA_CameraSetSpeed(camera, speed, id) \
	TU_CALL(FUN_001c84b0, void (*)(void *, float, int))(camera, speed, id)
#define TA_IsFriendly(camera, a, b) TU_CALL(FUN_001c82e0, int (*)(void *, int, int))(camera, a, b)
#define TA_CameraShake(camera, id, kind) TU_CALL(FUN_001c8e80, void (*)(void *, int, int))(camera, id, kind)
/* the players (thiscall) */
#define TA_CameraHeading(players) TU_CALL(FUN_001c66f0, float (*)(void *))(players)
/* 0x001ce940: an angle wrapped to 0..2 pi (returned on the x87 stack) */
#define TA_WrapAngle(angle) TU_CALL(FUN_001ce940, float (*)(float))(angle)
/* the explosions (thiscall) */
#define TA_Boom(boom, kind, pos, vel, flags, colour) \
	TU_CALL(FUN_001c3540, void (*)(void *, int, float *, float *, int, unsigned int))(boom, kind, pos, \
		vel, flags, colour)
/* the spawn points (thiscall): a named point of a player (pos[4], rot[4]); a random point of
   a list */
#define TA_SpawnFind(spawn, name, player) \
	TU_CALL(FUN_001c4110, float *(*)(void *, const char *, int))(spawn, name, player)
#define TA_SpawnRandom(spawn, name, out) \
	TU_CALL(FUN_001c4420, void (*)(void *, const char *, float *))(spawn, name, out)

/* ---------- engine_misc: the original returns the inner test's result (the table and
   engine_misc's definitions say void; reported for integration) */
#define TU_SegmentSphere(seg, sphere, hit) \
	TU_CALL(FUN_00052ea0, int (*)(float *, float *, void *))(seg, sphere, hit)
#define TU_SweepSphereSphere(a, b, vec, hit) \
	TU_CALL(FUN_00053af0, int (*)(float *, float *, float *, void *))(a, b, vec, hit)

/* |a - b|^2: 0x00051540 returns it on the x87 stack (bkernel's definition is empty) */
static inline float tu_dist_sq(const float *a, const float *b)
{
	float x = a[0] - b[0];
	float y = a[1] - b[1];
	float z = a[2] - b[2];

	return z * z + x * x + y * y;
}

#endif
