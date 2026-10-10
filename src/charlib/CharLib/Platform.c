/*
CharLib/Platform.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\Platform.cpp (tag at 0x000b7fb0, source line 581).

Platforms: moving, turning, falling, seesaw and smashable scenery that actors stand on.
A platform (0x470 bytes) is a list node, a CharLib actor at +0x10 (type 2) and the
platform data after it. The CharLib world object (chl_actor_list) keeps the active
platforms at +0x18 and the inactive ones at +0x24.

The functions between the lights.cpp tags and the Collect.cpp tag that work on platforms
(0x000b7c70-0x000b99f0) are placed here (inferred). 0x000b8460 is an animation callback
that Ghidra did not find (not in src/include/of_functions.tsv; reported).
*/

#include "charlib.h"
#include "charlib_actor.h"
#include "charlib_coll.h"

/* ---------- the platform (0x470 bytes) */

#define PL_ACTOR(p) ((void *)((char *)(p) + 0x10))  /* the embedded actor */
#define PL_INST 0x20               /* the actor's instance (its matrix first) */
#define PL_INST_POS 0x50           /* translation row of the instance matrix */
#define PL_POS 0xe0                /* the actor's position (float[4]) */
#define PL_ORIENT 0x100            /* the actor's orientation quaternion */
#define PL_ROOT_NODE 0x154         /* the actor's root node */
#define PL_COLLMESH 0x1a0          /* the actor's collision mesh */
#define PL_VEL 0x1a4               /* the actor's velocity (float[4]) */
#define PL_PREV_POS 0x1b4          /* the actor's position before this update */
#define PL_SPIN 0x1d4              /* float[4]: angular velocity per frame (x, y, z) */
#define PL_EULER 0x1f4             /* the actor's rotation (x, y, z, w) */
#define PL_START_QUAT 0x214        /* quaternion of the start rotation */
#define PL_ACTOR_FLAGS 0x230       /* the actor's flags (0x1 falling, 0x2 orientation set,
                                      0x20 no collision) */
#define PL_BOB_LEVEL_SINK 0x24c    /* float: sinking (the actor's 0x23c) */
#define PL_PREV_EULER 0x370        /* float[4]: rotation before this update */
#define PL_SAVED_EULER 0x380       /* float[4]: rotation of the last mesh refresh */
#define PL_DELTA_MATRIX 0x390      /* float[16]: movement of this update (new * old^-1) */
#define PL_GROUP 0x3d0             /* int: group id (negative: the smashed copy) */
#define PL_MOVED 0x3d4             /* char: the collision mesh must be refreshed */
#define PL_ACTIVE 0x3d5            /* char */
#define PL_POS_KEYS 0x3d8          /* key track of the position (count at 0x3da) */
#define PL_ROT_KEYS 0x3f8          /* key track of the rotation (count at 0x3fa) */
#define PL_ANIM 0x418              /* struct PlatformAnim */
#define PL_ANIM_SPEED 0x41c
#define PL_FALL 0x43c              /* falling-platform data (shorts and floats) */
#define PL_SMASH 0x440             /* smash data: model name, ... */
#define PL_SEESAW 0x444            /* seesaw data: axis byte, limits, spring */
#define PL_MOVER 0x448             /* constant-velocity data */
#define PL_TRAILS 0x44c            /* trail data: trails[8], nodes[8], count, enabled */
#define PL_FLAGS 0x450             /* 0x1 loop position, 0x2 loop rotation, 0x8 / 0x10
                                      wait for movement, 0x40 remove, 0x80 removed, 0x2000
                                      smoke on landing, 0x4000 float on the water, 0x8000
                                      no collision */
#define PL_BUFFER_454 0x454
#define PL_DIP_NEAR_HOLDERS 0x458  /* int: dip while a player holds something nearby */
#define PL_LIGHTS 0x45c            /* int: lights attached to it */
#define PL_PARENT_NODE 0x460       /* node of the parent actor it is attached to */
#define PL_PARENT 0x464            /* the parent actor */

/* the animation clock of a platform's key tracks */
struct PlatformAnim {
	float t;                   /* 0x00 */
	float speed;               /* 0x04 frames per frame (0: stopped) */
	int length;                /* 0x08 */
	int mode;                  /* 0x0c 0 stop at the end, 1 loop, 2 ping-pong */
	unsigned int flags;        /* 0x10 1: mode 1 stops at the start */
};

/* the world object's platform lists */
#define pl_active_list ((void *)((char *)chl_actor_list + 0x18))
#define pl_inactive_list ((void *)((char *)chl_actor_list + 0x24))
#define pl_light_list ((void *)((char *)chl_actor_list + 0x3c))

/* 0x002f6b74  the water level of floating platforms */
#define pl_water_level CHL_VAR(float, 0x002f6b74)
/* 0x00293958  fall acceleration of falling platforms (0.2) */
#define pl_fall_accel CHL_VAR(float, 0x00293958)
/* 0x002939ac / b0 / b4  floating: damping (0.95), spring (0.05), dead band (3) */
#define pl_float_damping CHL_VAR(float, 0x002939ac)
#define pl_float_spring CHL_VAR(float, 0x002939b0)
#define pl_float_band CHL_VAR(float, 0x002939b4)
/* 0x002939b8  gravity factor of a dropping platform (0.5) */
#define pl_drop_gravity CHL_VAR(float, 0x002939b8)

/* 0x00291760 / 0x002917b8  the smoke particle effects (bumpercars .data) */
extern unsigned char DAT_00291760[0x2c];
extern unsigned char DAT_002917b8[0x2c];
#define pl_StartEffect ((int (*)(void *, float *, float *, int, int))FUN_00080d60)
/* bactor: baFindSubObject(inst, name), baGetNodePosition(inst, node, out),
bGetKey3(out, track, time) */
#define pl_FindSubObject ((void *(*)(void *, const char *))FUN_00035c70)
#define pl_GetNodePosition ((void (*)(void *, void *, float *))FUN_00035ce0)
#define pl_GetKey3 ((void (*)(float *, void *, int))FUN_0003cae0)
/* classlib_fx: CProjTrail_AddPoint(trail, point) */
#define pl_TrailAddPoint ((void (*)(void *, float *))FUN_00083580)

/* 0x000b7c70  CharLib (Platform.cpp, inferred)
Deletes a platform: its actor, the references of the actors' character data to it (the
standing platform and the touched list), the lights attached to it lose it, its data
blocks. Returns the active-list node before it. */
void *Platform_Delete(void *platform)
{
	void *prev = chl_ListRemove(pl_active_list, platform);
	void *actor;
	void *light;
	int i;
	int j;
	int k;

	Actor_Delete(PL_ACTOR(platform));
	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL; actor = chl_ListNext(chl_actor_list, actor)) {
		if (ACT_CD(actor) == NULL)
			continue;
		if (CHL_PTR(ACT_CD(actor), CD_PLATFORM) == platform)
			CHL_PTR(ACT_CD(actor), CD_PLATFORM) = NULL;
		for (i = 0; i < 5; i++) {
			if (CHL_PTR(ACT_CD(actor), CD_TOUCHED + i * 4) != platform)
				continue;
			/* remove it, moving the later entries down (the entry that moves into
			slot i is not looked at again, as in the original) */
			k = i;
			if (i < 4) {
				k = 4;
				for (j = i; j < 4; j++)
					CHL_PTR(ACT_CD(actor), CD_TOUCHED + j * 4) = CHL_PTR(ACT_CD(actor), CD_TOUCHED + j * 4 + 4);
			}
			CHL_PTR(ACT_CD(actor), CD_TOUCHED + k * 4) = NULL;
			CHL_I32(ACT_CD(actor), CD_TOUCHED_COUNT) = CHL_I32(ACT_CD(actor), CD_TOUCHED_COUNT) - 1;
		}
	}
	if (0 < CHL_I32(platform, PL_LIGHTS)) {
		for (light = chl_ListNext(pl_light_list, NULL); light != NULL; light = chl_ListNext(pl_light_list, light)) {
			if (CHL_PTR(light, 0x8c) == platform)
				CHL_PTR(light, 0x8c) = NULL;
		}
	}
	chl_Free(CHL_PTR(platform, PL_BUFFER_454));
	CHL_PTR(platform, PL_BUFFER_454) = NULL;
	chl_Free(CHL_PTR(platform, 0x3dc));
	CHL_PTR(platform, 0x3dc) = NULL;
	chl_Free(CHL_PTR(platform, 0x3fc));
	CHL_PTR(platform, 0x3fc) = NULL;
	chl_Free(CHL_PTR(platform, PL_FALL));
	CHL_PTR(platform, PL_FALL) = NULL;
	chl_Free(CHL_PTR(platform, PL_SMASH));
	CHL_PTR(platform, PL_SMASH) = NULL;
	chl_Free(CHL_PTR(platform, PL_SEESAW));
	CHL_PTR(platform, PL_SEESAW) = NULL;
	chl_Free(CHL_PTR(platform, PL_MOVER));
	CHL_PTR(platform, PL_MOVER) = NULL;
	chl_Free(CHL_PTR(platform, PL_TRAILS));
	CHL_PTR(platform, PL_TRAILS) = NULL;
	chl_Free(platform);
	return prev;
}

/* 0x000b7e40  CharLib (Platform.cpp, inferred)
Activates the inactive platforms of a group (into the active list, their actors into the
actor list). */
void Platforms_ActivateGroup(int group)
{
	void *platform = chl_ListNext(pl_inactive_list, NULL);
	void *next;

	while (platform != NULL) {
		next = platform;
		if (CHL_I32(platform, PL_GROUP) == group) {
			next = chl_ListRemove(pl_inactive_list, platform);
			chl_ListInsert(pl_active_list, platform, 0);
			chl_ListInsert(chl_actor_list, PL_ACTOR(platform), 0);
			CHL_U8(platform, PL_ACTIVE) = 1;
			CHL_U32(platform, PL_FLAGS) = CHL_U32(platform, PL_FLAGS) & 0xffffff7f;
		}
		platform = chl_ListNext(pl_inactive_list, next);
	}
}

/* 0x000b7ee0  CharLib (Platform.cpp, inferred)
Deactivates the active platforms of a group (and their smashed copies, group -id). */
void Platforms_DeactivateGroup(int group)
{
	void *platform = chl_ListNext(pl_active_list, NULL);
	void *next;

	while (platform != NULL) {
		next = platform;
		if (CHL_I32(platform, PL_GROUP) == group || CHL_I32(platform, PL_GROUP) == -group) {
			chl_ListRemove(chl_actor_list, PL_ACTOR(platform));
			next = chl_ListRemove(pl_active_list, platform);
			chl_ListInsert(pl_inactive_list, platform, 0);
			CHL_U8(platform, PL_ACTIVE) = 0;
		}
		platform = chl_ListNext(pl_active_list, next);
	}
}

/* 0x000b7f70  CharLib (Platform.cpp, inferred) */
void Platform_Deactivate(void *platform)
{
	chl_ListRemove(chl_actor_list, PL_ACTOR(platform));
	chl_ListRemove(pl_active_list, platform);
	chl_ListInsert(pl_inactive_list, platform, 0);
}

/* 0x000b7fb0  CharLib/Platform.cpp
Creates an active platform from a model (its actor is a static type 2 actor). */
void *Platform_Create(const char *name, float *pos, int no_collision)
{
	void *platform = chl_Calloc(0x470, 0, "C:\\BlitzParty\\Game\\Src\\CharLib\\Platform.cpp", 0x245, 0x2001);

	if (platform != NULL) {
		CHL_PTR(platform, PL_PARENT_NODE) = NULL;
		CHL_U8(platform, PL_ACTIVE) = 1;
		Actor_LoadAny(name, pos, 2, PL_ACTOR(platform), no_collision);
		FUN_00070c10(CHL_FIELD(float, platform, PL_START_QUAT), CHL_FIELD(float, platform, PL_EULER), 0.0f);
		CHL_U8(platform, PL_MOVED) = 0;
		chl_ListInsert(pl_active_list, platform, 0);
	}
	return platform;
}

/* 0x000b8040  CharLib (Platform.cpp, inferred)
A seesaw platform: springs back towards level unless the tilt is within the dead band,
loses spin (pow(damping, time)) and stays within its tilt limits. */
void Platform_UpdateSeesaw(void *platform)
{
	unsigned char *seesaw = CHL_PTR(platform, PL_SEESAW);
	unsigned int axis = seesaw[0];
	float *angle = CHL_FIELD(float, platform, PL_EULER + axis * 4);
	float *spin = CHL_FIELD(float, platform, PL_SPIN + axis * 4);
	float tilt = *angle;

	if (3.1415925f < *angle)
		tilt = tilt - 6.283185f;
	if (tilt < -3.1415925f)
		tilt = tilt + 6.283185f;
	if (CHL_F32(seesaw, 0x1c) == 0.0f || fabsf(tilt) <= CHL_F32(seesaw, 0x1c)) {
		if (*angle <= 3.1415925f)
			*spin = *spin - CHL_F32(seesaw, 0x10) * *angle;
		else
			*spin = (6.283185f - *angle) * CHL_F32(seesaw, 0x10) + *spin;
		*spin = (float)(pow((double)CHL_F32(CHL_PTR(platform, PL_SEESAW), 0x14), (double)chl_time_scale) *
			(double)*spin);
	} else {
		*spin = *spin * 1.08f;
	}
	seesaw = CHL_PTR(platform, PL_SEESAW);
	if (CHL_F32(seesaw, 0xc) < *spin)
		*spin = CHL_F32(seesaw, 0xc);
	else if (*spin < -CHL_F32(seesaw, 0xc))
		*spin = -CHL_F32(seesaw, 0xc);
	seesaw = CHL_PTR(platform, PL_SEESAW);
	if (CHL_F32(seesaw, 0x4) < tilt) {
		*angle = CHL_F32(seesaw, 0x4);
		*spin = 0.0f;
		return;
	}
	if (tilt < CHL_F32(seesaw, 0x8)) {
		*angle = CHL_F32(seesaw, 0x8);
		*spin = 0.0f;
	}
}

/* 0x000b81c0  CharLib (Platform.cpp, inferred)
Scrolls the texture offsets of a platform with a constant-velocity block (wrapped to the
fractional part). */
void Platform_ScrollTexture(void *platform)
{
	void *mover = CHL_PTR(platform, PL_MOVER);
	float dv = chl_time_scale * CHL_F32(mover, 0x18) * CHL_F32(mover, 0x10);
	int iv;
	int iu;

	CHL_F32(mover, 0x24) = chl_time_scale * CHL_F32(mover, 0x14) * CHL_F32(mover, 0x10) + CHL_F32(mover, 0x24);
	mover = CHL_PTR(platform, PL_MOVER);
	CHL_F32(mover, 0x28) = dv + CHL_F32(mover, 0x28);
	mover = CHL_PTR(platform, PL_MOVER);
	iv = CHL_FTOL(CHL_F32(mover, 0x28));
	iu = CHL_FTOL(CHL_F32(mover, 0x24));
	CHL_F32(mover, 0x24) = CHL_F32(mover, 0x24) - (float)iu;
	mover = CHL_PTR(platform, PL_MOVER);
	CHL_F32(mover, 0x28) = CHL_F32(mover, 0x28) - (float)iv;
}

/* 0x000b8240  CharLib (Platform.cpp, inferred)
Advances an animation clock and handles its end: stop (mode 0), loop (mode 1; with flag 1
it stops at the start when running backwards, and at 0 at the end), ping-pong (mode 2). */
void PlatformAnim_Advance(struct PlatformAnim *anim)
{
	float length;
	float t;

	if (anim->speed == 0.0f)
		return;
	t = chl_time_scale * anim->speed + anim->t;
	length = (float)anim->length;
	anim->t = t;
	if (t <= length) {
		if (0.0f <= t)
			return;
		if (anim->mode != 0) {
			if (anim->mode == 1) {
				if ((anim->flags & 1) != 0) {
					anim->t = length;
					anim->speed = 0.0f;
					return;
				}
				if (anim->length != 0) {
					do
						t = t + length;
					while (t < 0.0f);
					anim->t = t;
					return;
				}
			} else {
				if (anim->mode != 2)
					return;
				if (anim->length != 0) {
					anim->t = -t;
					anim->speed = -anim->speed;
					if (anim->t <= length)
						return;
					do
						anim->t = anim->t - length;
					while ((float)anim->length < anim->t);
					return;
				}
			}
			anim->t = 0.0f;
			return;
		}
	} else {
		if (anim->mode == 0) {
			anim->t = length;
			anim->speed = 0.0f;
			return;
		}
		if (anim->mode != 1) {
			if (anim->mode != 2)
				return;
			if (anim->length != 0) {
				anim->t = (float)(anim->length * 2) - t;
				anim->speed = -anim->speed;
				if (0.0f <= anim->t)
					return;
				t = anim->t;
				do
					t = t + length;
				while (t < 0.0f);
				anim->t = t;
				return;
			}
			anim->t = 0.0f;
			return;
		}
		if ((anim->flags & 1) == 0) {
			if (anim->length != 0) {
				do
					t = t - length;
				while (length < t);
				anim->t = t;
				return;
			}
			anim->t = 0.0f;
			return;
		}
	}
	anim->speed = 0.0f;
	anim->t = 0.0f;
}

/* 0x000b83f0  CharLib (Platform.cpp, inferred)
The platform of a group (active first, then inactive); NULL when there is none. */
void *Platform_Find(int group)
{
	void *platform;

	for (platform = chl_ListNext(pl_active_list, NULL); platform != NULL;
	     platform = chl_ListNext(pl_active_list, platform)) {
		if (CHL_I32(platform, PL_GROUP) == group)
			return platform;
	}
	for (platform = chl_ListNext(pl_inactive_list, NULL); platform != NULL;
	     platform = chl_ListNext(pl_inactive_list, platform)) {
		if (CHL_I32(platform, PL_GROUP) == group)
			return platform;
	}
	return NULL;
}

/* 0x000b8460  CharLib (Platform.cpp, inferred; not in the function table)
Animation callback of a smashed platform's "smash" animation: deactivates the copy (on any
event, as in the original). */
int Platform_SmashAnimCallback(int reason, void *inst, void *node, void *anim, void *platform)
{
	(void)reason;
	(void)inst;
	(void)node;
	(void)anim;
	chl_ListRemove(chl_actor_list, PL_ACTOR(platform));
	chl_ListRemove(pl_active_list, platform);
	chl_ListInsert(pl_inactive_list, platform, 0);
	return 1;
}

/* 0x000b84a0  CharLib (Platform.cpp, inferred)
Deletes every platform (the inactive ones are moved to the active list first). */
void Platforms_DeleteAll(void)
{
	void *platform = chl_ListNext(pl_inactive_list, NULL);
	void *prev;

	while (platform != NULL) {
		prev = chl_ListRemove(pl_inactive_list, platform);
		chl_ListInsert(pl_active_list, platform, 0);
		platform = chl_ListNext(pl_inactive_list, prev);
	}
	platform = chl_ListNext(pl_active_list, NULL);
	while (platform != NULL)
		platform = chl_ListNext(pl_active_list, Platform_Delete(platform));
}

/* 0x000b8520  CharLib (Platform.cpp, inferred)
Turns an actor that stands on a turning platform node with it: about the node's position,
by the change of the node's rotation since the last frame; the actor's heading turns too. */
void Platform_RotateActor(void *actor)
{
	unsigned char *cd = ACT_CD(actor);
	void *node = CHL_PTR(CHL_PTR(cd, CD_PLATFORM), PL_ROOT_NODE);
	float quat[4];
	float old_heading;
	float new_heading;
	float pivot[3];
	float d[4];
	float m_old[16];
	float m_inv[16];
	float m_new[16];
	void *platform;

	memcpy(quat, CHL_FIELD(char, node, 0x30), 0x10);
	old_heading = FUN_00070610(CHL_FIELD(float, ACT_CD(actor), 0x1f4));
	new_heading = FUN_00070610(quat);
	CHL_F32(actor, ACT_HEADING_WANTED) = CHL_F32(actor, ACT_HEADING_WANTED) - FUN_000703d0(old_heading, new_heading);
	FUN_000515c0(m_old, CHL_FIELD(float, ACT_CD(actor), 0x1f4));
	FUN_000502f0(m_inv, m_old);
	FUN_000515c0(m_new, quat);
	cd = ACT_CD(actor);
	platform = CHL_PTR(cd, CD_PLATFORM);
	pivot[0] = CHL_F32(platform, PL_PREV_POS + 0x0) + CHL_F32(cd, 0x1d4);
	pivot[1] = CHL_F32(platform, PL_PREV_POS + 0x4) + CHL_F32(cd, 0x1d8);
	pivot[2] = CHL_F32(platform, PL_PREV_POS + 0x8) + CHL_F32(cd, 0x1dc);
	d[3] = 1.0f;
	d[0] = CHL_F32(actor, ACT_POS + 0x0) - pivot[0];
	d[1] = CHL_F32(actor, ACT_POS + 0x4) - pivot[1];
	d[2] = CHL_F32(actor, ACT_POS + 0x8) - pivot[2];
	chl_MatTransform(m_inv, d);
	chl_MatTransform(m_new, d);
	CHL_F32(actor, ACT_POS + 0x0) = d[0] + pivot[0];
	CHL_F32(actor, ACT_POS + 0x4) = d[1] + pivot[1];
	CHL_F32(actor, ACT_POS + 0x8) = d[2] + pivot[2];
	platform = CHL_PTR(ACT_CD(actor), CD_PLATFORM);
	node = CHL_PTR(platform, PL_ROOT_NODE);
	CHL_F32(actor, ACT_POS + 0x0) = ((CHL_F32(node, 0x10) + CHL_F32(platform, PL_POS + 0x0)) - pivot[0]) +
		CHL_F32(actor, ACT_POS + 0x0);
	CHL_F32(actor, ACT_POS + 0x4) = ((CHL_F32(node, 0x14) + CHL_F32(platform, PL_POS + 0x4)) - pivot[1]) +
		CHL_F32(actor, ACT_POS + 0x4);
	CHL_F32(actor, ACT_POS + 0x8) = ((CHL_F32(node, 0x18) + CHL_F32(platform, PL_POS + 0x8)) - pivot[2]) +
		CHL_F32(actor, ACT_POS + 0x8);
}

/* 0x000b8720  CharLib (Platform.cpp, inferred)
A falling platform (data at 0x43c: short timer, short reset time, short shake time, short
flags, float fall speed, float shake, float[4] home): shakes, then falls (and tumbles with
flags 2, 4, 8); after the reset time it comes back home (flag 1) or is removed (0x40). */
void Platform_UpdateFalling(void *platform)
{
	short *fall = CHL_PTR(platform, PL_FALL);

	if (fall[0] == 0)
		return;
	fall[0] = (short)(fall[0] + 1);
	fall = CHL_PTR(platform, PL_FALL);
	if (fall[2] < fall[0]) {
		CHL_F32(platform, PL_VEL + 0x4) = CHL_F32(platform, PL_VEL + 0x4) - pl_fall_accel * CHL_F32(fall, 0x8);
		if ((CHL_U8(fall, 0x6) & 2) != 0)
			CHL_F32(platform, PL_SPIN + 0x0) = CHL_F32(platform, PL_SPIN + 0x0) + 0.002f;
		if ((CHL_U8(fall, 0x6) & 4) != 0)
			CHL_F32(platform, PL_SPIN + 0x4) = CHL_F32(platform, PL_SPIN + 0x4) + 0.001f;
		if ((CHL_U8(fall, 0x6) & 8) != 0)
			CHL_F32(platform, PL_SPIN + 0x8) = CHL_F32(platform, PL_SPIN + 0x8) + 0.002f;
	} else {
		int r = CHL_Rand();

		fall = CHL_PTR(platform, PL_FALL);
		CHL_F32(platform, PL_POS + 0x0) = (float)r * CHL_F32(fall, 0xc) * (float)(int)fall[0] * 3.05185094e-05f +
			CHL_F32(fall, 0x10);
		r = CHL_Rand();
		fall = CHL_PTR(platform, PL_FALL);
		CHL_F32(platform, PL_POS + 0x8) = (float)r * CHL_F32(fall, 0xc) * (float)(int)fall[0] * 3.05185094e-05f +
			CHL_F32(fall, 0x18);
	}
	if (fall[1] < fall[0]) {
		if ((CHL_U8(fall, 0x6) & 1) != 0) {
			CHL_F32(platform, PL_VEL + 0x0) = 0.0f;
			CHL_F32(platform, PL_VEL + 0x4) = 0.0f;
			CHL_F32(platform, PL_VEL + 0x8) = 0.0f;
			CHL_F32(platform, PL_EULER + 0x0) = 0.0f;
			CHL_F32(platform, PL_EULER + 0x4) = 0.0f;
			CHL_F32(platform, PL_EULER + 0x8) = 0.0f;
			CHL_F32(platform, PL_SPIN + 0x0) = 0.0f;
			CHL_F32(platform, PL_SPIN + 0x4) = 0.0f;
			CHL_F32(platform, PL_SPIN + 0x8) = 0.0f;
			CHL_F32(platform, PL_VEL + 0xc) = 1.0f;
			CHL_F32(platform, PL_EULER + 0xc) = 1.0f;
			CHL_F32(platform, PL_SPIN + 0xc) = 1.0f;
			memcpy(CHL_FIELD(char, platform, PL_POS), CHL_FIELD(char, fall, 0x10), 0x10);
			*(short *)CHL_PTR(platform, PL_FALL) = 0;
			CHL_F32(platform, PL_START_QUAT + 0x0) = 0.0f;
			CHL_F32(platform, PL_START_QUAT + 0x4) = 0.0f;
			CHL_F32(platform, PL_START_QUAT + 0x8) = 0.0f;
			CHL_F32(platform, PL_START_QUAT + 0xc) = 1.0f;
			CRColl_PrepareMesh(PL_ACTOR(platform), 0, 1);
			return;
		}
		CHL_U32(platform, PL_FLAGS) = CHL_U32(platform, PL_FLAGS) | 0x40;
	}
}

/* 0x000b8960  CharLib (Platform.cpp, inferred)
The constant velocity of a platform in the world (turned by its rotation matrix at
0x110): absolute != 0 writes it, else adds it times the frame time. */
void Platform_GetVelocity(float *out, void *platform, int absolute)
{
	float *v = CHL_PTR(platform, PL_MOVER);
	float local[4];
	float speed;

	local[0] = v[0];
	local[1] = v[1];
	local[2] = v[2];
	local[3] = v[3];
	chl_MatTransform(CHL_FIELD(float, platform, 0x110), local);
	speed = CHL_F32(CHL_PTR(platform, PL_MOVER), 0x10);
	if (absolute != 0) {
		out[3] = local[3];
		out[0] = local[0] * speed;
		out[1] = local[1] * speed;
		out[2] = local[2] * speed;
		return;
	}
	out[0] = chl_time_scale * local[0] * speed + out[0];
	out[1] = chl_time_scale * local[1] * speed + out[1];
	out[2] = chl_time_scale * local[2] * speed + out[2];
}

/* 0x000b8a40  CharLib (Platform.cpp, inferred)
Adds a platform's velocity to an actor's: against it, half the actor's velocity plus the
platform's; with it, each axis takes the platform's speed when that is faster. */
void Platform_AddVelocity(float *vel, float *add)
{
	if (vel[0] * add[0] + add[2] * vel[2] + vel[1] * add[1] < 0.0f) {
		vel[0] = vel[0] * 0.5f + add[0];
		vel[1] = vel[1] * 0.5f + add[1];
		vel[2] = vel[2] * 0.5f + add[2];
		return;
	}
	if ((0.0f < add[0] && vel[0] < add[0]) || (add[0] < 0.0f && add[0] < vel[0]))
		vel[0] = add[0];
	if ((0.0f < add[1] && vel[1] < add[1]) || (add[1] < 0.0f && add[1] < vel[1]))
		vel[1] = add[1];
	if ((0.0f < add[2] && vel[2] < add[2]) || (add[2] < 0.0f && add[2] < vel[2]))
		vel[2] = add[2];
}

/* 0x000b8b60  CharLib (Platform.cpp, inferred)
Platforms attached to a node of another actor follow it: their matrix becomes the node's
matrix times the parent's; their collision mesh is rebuilt at that place. */
void Platforms_UpdateAttached(void)
{
	void *platform;
	float m[16];
	float pos[4];
	float quat[4];

	for (platform = chl_ListNext(pl_active_list, NULL); platform != NULL;
	     platform = chl_ListNext(pl_active_list, platform)) {
		float *inst;

		if (CHL_PTR(platform, PL_PARENT_NODE) == NULL)
			continue;
		inst = CHL_FIELD(float, platform, PL_INST);
		chl_PrepInstance(inst, 1);
		chl_PrepInstance((char *)CHL_PTR(platform, PL_PARENT) + 0x10, 1);
		chl_MatMul(m, CHL_FIELD(float, CHL_PTR(platform, PL_PARENT_NODE), 0x80), inst);
		chl_MatMul(inst, (float *)((char *)CHL_PTR(platform, PL_PARENT) + 0x10), m);
		if (CHL_PTR(platform, PL_COLLMESH) != NULL) {
			memcpy(pos, CHL_FIELD(char, platform, PL_POS), 0x10);
			memcpy(quat, CHL_FIELD(char, platform, PL_ORIENT), 0x10);
			memcpy(CHL_FIELD(char, platform, PL_POS), CHL_FIELD(char, platform, PL_INST_POS), 0x10);
			FUN_00051700(CHL_FIELD(float, platform, PL_ORIENT), inst);
			CRColl_RefreshMesh(platform, NULL);
			memcpy(CHL_FIELD(char, platform, PL_POS), pos, 0x10);
			memcpy(CHL_FIELD(char, platform, PL_ORIENT), quat, 0x10);
		}
		FUN_000beb70(PL_ACTOR(platform));
	}
}

/* 0x000b8d20  CharLib (Platform.cpp, inferred)
Smoke around the foot of a platform: puffs along the four sides of its bounding box
(0.15 per unit of length), pushed outwards, with random colour and offset, two of every
three alternately with and without gravity. */
void Platform_Smoke(void *platform)
{
	void *mesh = CHL_PTR(platform, PL_COLLMESH);
	float corners[5][4];
	float puff[4];
	float dir[4];
	float y = CHL_F32(platform, PL_POS + 0x4) + 16.0f;
	float steps_f;
	float t;
	float s;
	int which = 0;
	int steps;
	int side;
	int i;
	int colour;
	int r1;
	int r2;
	int r3;

	corners[0][0] = CHL_F32(mesh, MESH_MIN + 0x0) - 16.0f;
	corners[0][1] = y;
	corners[0][2] = CHL_F32(mesh, MESH_MIN + 0x8) - 16.0f;
	corners[0][3] = 1.0f;
	corners[1][0] = CHL_F32(mesh, MESH_MAX + 0x0) + 16.0f;
	corners[1][1] = y;
	corners[1][2] = CHL_F32(mesh, MESH_MIN + 0x8) - 16.0f;
	corners[1][3] = 1.0f;
	corners[2][0] = CHL_F32(mesh, MESH_MAX + 0x0) + 16.0f;
	corners[2][1] = y;
	corners[2][2] = CHL_F32(mesh, MESH_MAX + 0x8) + 16.0f;
	corners[2][3] = 1.0f;
	corners[3][0] = CHL_F32(mesh, MESH_MIN + 0x0) - 16.0f;
	corners[3][1] = y;
	corners[3][2] = CHL_F32(mesh, MESH_MAX + 0x8) + 16.0f;
	corners[3][3] = 1.0f;
	corners[4][0] = CHL_F32(mesh, MESH_MIN + 0x0) - 16.0f;
	corners[4][1] = y;
	corners[4][2] = CHL_F32(mesh, MESH_MIN + 0x8) - 16.0f;
	corners[4][3] = 1.0f;
	for (side = 0; side < 4; side++) {
		steps = CHL_FTOL(FUN_00051500(corners[side], corners[side + 1]) * 0.15f);
		if (steps == 0 || steps <= 0)
			continue;
		steps_f = (float)steps;
		for (i = 0; i < steps; i++) {
			t = (float)i / steps_f;
			FUN_00050ff0(puff, corners[side], t, corners[side + 1], 1.0f - t);
			dir[0] = puff[0] - CHL_F32(platform, PL_POS + 0x0);
			dir[1] = puff[1] - CHL_F32(platform, PL_POS + 0x4);
			dir[2] = puff[2] - CHL_F32(platform, PL_POS + 0x8);
			FUN_000514a0(dir, dir);
			s = (float)CHL_Rand() * 3.05185094e-05f + 1.0f;
			dir[0] = dir[0] * s;
			dir[1] = 0.0f;
			dir[2] = dir[2] * s;
			r1 = CHL_Rand() % 32;
			r2 = CHL_Rand() % 32;
			r3 = CHL_Rand() % 32;
			colour = ((((r1 + 0x5a) & 0xff) << 8) + ((r2 + 0x46) & 0xff)) << 8;
			colour = colour + ((r3 + 0x32) & 0xff);
			puff[0] = ((float)CHL_Rand() * 0.000122074038f - 2.0f) + puff[0];
			puff[1] = ((float)CHL_Rand() * 0.000122074038f - 2.0f) + puff[1];
			puff[2] = ((float)CHL_Rand() * 0.000122074038f - 2.0f) + puff[2];
			if (which == 0) {
				pl_StartEffect(DAT_00291760, puff, dir, 0, colour);
				which = 1;
			} else if (which == 1) {
				pl_StartEffect(DAT_002917b8, puff, dir, 0, colour);
				which = 2;
			} else {
				which = 0;
			}
		}
	}
}

/* 0x000b90f0  CharLib (Platform.cpp, inferred)
The per-frame update of an active platform: removal, falling or texture scrolling,
dipping near players that hold something, floating, its animation clock, movement and
rotation from key tracks or velocities, the seesaw, its matrices and collision mesh,
dropping to the ground (actor flag 1) and the trails at its nodes. */
void Platform_Update(void *platform)
{
	unsigned int flags = CHL_U32(platform, PL_FLAGS);
	float old_matrix[16];
	float inverse[16];
	float *pos = CHL_FIELD(float, platform, PL_POS);
	float *euler = CHL_FIELD(float, platform, PL_EULER);
	float probe[4];
	void *tri;
	void *under;
	float f;
	float h;
	float g;
	float dv;
	float vy;
	int moved;
	int i;

	if ((flags & 0x80) != 0)
		return;
	if ((flags & 0x40) != 0) {
		CHL_U32(platform, PL_FLAGS) = (flags & 0xffffffbf) | 0x80;
		CHL_U32(platform, PL_ACTOR_FLAGS) = CHL_U32(platform, PL_ACTOR_FLAGS) | 0x20;
		chl_ListRemove(chl_actor_list, PL_ACTOR(platform));
		return;
	}
	if (CHL_PTR(platform, PL_FALL) != NULL)
		Platform_UpdateFalling(platform);
	else if (CHL_PTR(platform, PL_MOVER) != NULL && CHL_F32(CHL_PTR(platform, PL_MOVER), 0x10) != 0.0f)
		Platform_ScrollTexture(platform);
	if (CHL_I32(platform, PL_DIP_NEAR_HOLDERS) != 0) {
		f = 0.460751891f - AI_NearestHolderDistSq(pos) * 3.00751879e-07f;
		if (f < 0.16f)
			f = 0.16f;
		else if (0.46f < f)
			f = 0.46f;
		CHL_F32(platform, PL_BOB_LEVEL_SINK) = CHL_F32(platform, PL_BOB_LEVEL_SINK) * 0.9f + f * 0.1f;
		CHL_F32(platform, 0x344) = f * 5.0f * 0.1f + CHL_F32(platform, 0x344) * 0.9f;
	}
	if ((CHL_U32(platform, PL_FLAGS) & 0x4000) != 0) {
		vy = (pl_water_level - pos[1]) * pl_float_spring + CHL_F32(platform, PL_VEL + 0x4);
		CHL_F32(platform, PL_VEL + 0x4) = vy;
		if (pl_water_level < pos[1] - pl_float_band || pl_float_band + pos[1] < pl_water_level)
			CHL_F32(platform, PL_VEL + 0x4) = (float)(pow((double)pl_float_damping, (double)chl_time_scale) * (double)vy);
	}
	if (CHL_F32(platform, PL_BOB_LEVEL_SINK) != 0.0f)
		CHL_U8(platform, PL_MOVED) = 1;
	PlatformAnim_Advance(CHL_FIELD(struct PlatformAnim, platform, PL_ANIM));
	memcpy(old_matrix, CHL_FIELD(char, platform, PL_INST), 0x40);
	if (1 < CHL_U16(platform, 0x3fa) || CHL_F32(platform, PL_SPIN + 0x4) != 0.0f)
		memcpy(CHL_FIELD(char, platform, PL_PREV_EULER), euler, 0x10);
	FUN_000502f0(inverse, old_matrix);
	memcpy(CHL_FIELD(char, platform, PL_PREV_POS), pos, 0x10);

	/* position */
	if (CHL_U16(platform, 0x3da) < 2) {
		FUN_00070550(pos, CHL_FIELD(float, platform, PL_VEL), chl_time_scale);
	} else {
		pl_GetKey3(pos, CHL_FIELD(void, platform, PL_POS_KEYS), CHL_FTOL(CHL_F32(platform, PL_ANIM)));
		CHL_F32(platform, PL_VEL + 0x0) = pos[0] - CHL_F32(platform, PL_PREV_POS + 0x0);
		CHL_F32(platform, PL_VEL + 0x4) = pos[1] - CHL_F32(platform, PL_PREV_POS + 0x4);
		CHL_F32(platform, PL_VEL + 0x8) = pos[2] - CHL_F32(platform, PL_PREV_POS + 0x8);
		flags = CHL_U32(platform, PL_FLAGS);
		if ((flags & 1) != 0) {
			moved = CHL_F32(platform, PL_VEL + 0x0) != 0.0f;
			if ((flags & 8) == 0) {
				if (!moved && CHL_F32(platform, PL_VEL + 0x4) == 0.0f && CHL_F32(platform, PL_VEL + 0x8) == 0.0f)
					CHL_F32(platform, PL_ANIM_SPEED) = 0.0f;
			} else if (moved || CHL_F32(platform, PL_VEL + 0x4) != 0.0f || CHL_F32(platform, PL_VEL + 0x8) != 0.0f) {
				CHL_U32(platform, PL_FLAGS) = flags & 0xfffffff7;
			}
		}
	}

	/* rotation */
	if (CHL_U16(platform, 0x3fa) < 2) {
		FUN_00070550(euler, CHL_FIELD(float, platform, PL_SPIN), chl_time_scale);
		for (i = 0; i < 3; i++)
			euler[i] = FUN_000703a0(euler[i]);
	} else {
		pl_GetKey3(euler, CHL_FIELD(void, platform, PL_ROT_KEYS), CHL_FTOL(CHL_F32(platform, PL_ANIM)));
		flags = CHL_U32(platform, PL_FLAGS);
		if ((flags & 2) != 0) {
			moved = euler[0] != CHL_F32(platform, PL_PREV_EULER + 0x0);
			if ((flags & 0x10) == 0) {
				if (!moved && euler[1] == CHL_F32(platform, PL_PREV_EULER + 0x4) &&
				    euler[2] == CHL_F32(platform, PL_PREV_EULER + 0x8))
					CHL_F32(platform, PL_ANIM_SPEED) = 0.0f;
			} else if (moved || euler[1] != CHL_F32(platform, PL_PREV_EULER + 0x4) ||
				   euler[2] != CHL_F32(platform, PL_PREV_EULER + 0x8)) {
				CHL_U32(platform, PL_FLAGS) = flags & 0xffffffef;
			}
		}
	}
	if (CHL_PTR(platform, PL_SEESAW) != NULL)
		Platform_UpdateSeesaw(platform);
	if ((CHL_U8(platform, PL_ACTOR_FLAGS) & 2) == 0)
		FUN_00070c10(CHL_FIELD(float, platform, PL_ORIENT), euler, 0.0f);
	if (CHL_I32(platform, 0x330) == 0)
		chl_PrepInstance(CHL_FIELD(void, platform, PL_INST), 0);
	chl_MatMul(CHL_FIELD(float, platform, PL_DELTA_MATRIX), CHL_FIELD(float, platform, PL_INST), inverse);

	/* collision mesh and shadow */
	if (euler[0] == CHL_F32(platform, PL_PREV_EULER + 0x0) && euler[1] == CHL_F32(platform, PL_PREV_EULER + 0x4) &&
	    euler[2] == CHL_F32(platform, PL_PREV_EULER + 0x8)) {
		if (CHL_F32(platform, PL_VEL + 0x0) == 0.0f && CHL_F32(platform, PL_VEL + 0x4) == 0.0f &&
		    CHL_F32(platform, PL_VEL + 0x8) == 0.0f)
			goto unchanged;
		if (CHL_PTR(platform, PL_COLLMESH) != NULL)
			CRColl_MovePlatformBox(platform);
	} else if (CHL_PTR(platform, PL_COLLMESH) != NULL) {
		CRColl_RefreshMesh(platform, NULL);
	}
	FUN_000beb70(PL_ACTOR(platform));
unchanged:
	if (CHL_U16(platform, 0x3fa) < 2 && CHL_F32(platform, PL_SPIN + 0x4) == 0.0f)
		memcpy(CHL_FIELD(char, platform, PL_PREV_EULER), euler, 0x10);

	/* dropping to the ground */
	if ((CHL_U8(platform, PL_ACTOR_FLAGS) & 1) != 0) {
		CRColl_RefreshMesh(platform, NULL);
		probe[0] = pos[0];
		probe[2] = pos[2];
		probe[3] = pos[3];
		probe[1] = CHL_F32(CHL_PTR(platform, PL_COLLMESH), MESH_MIN + 0x4) - 1.0f;
		CHL_U32(platform, PL_ACTOR_FLAGS) = CHL_U32(platform, PL_ACTOR_FLAGS) | 0x20;
		h = CLColl_GroundProbe(probe, 1.0f, &tri, &under, 0);
		g = pl_drop_gravity * CHL_VAR(float, 0x00292c84);
		dv = chl_time_scale * g;
		vy = CHL_F32(platform, PL_VEL + 0x4) - dv;
		if (!(h < (0.5f * dv - vy) * chl_time_scale)) {
			CHL_F32(platform, PL_VEL + 0x4) = vy;
			pos[1] = pos[1] - chl_time_scale * chl_time_scale * g * 0.5f;
		} else {
			CHL_U32(platform, PL_ACTOR_FLAGS) = CHL_U32(platform, PL_ACTOR_FLAGS) & 0xfffffffe;
			f = CHL_F32(CHL_PTR(platform, PL_COLLMESH), MESH_MIN + 0x4);
			CHL_F32(platform, PL_VEL + 0x4) = 0.0f;
			pos[1] = f - h;
			if ((CHL_U32(platform, PL_FLAGS) & 0x2000) != 0)
				Platform_Smoke(platform);
		}
		CHL_U32(platform, PL_ACTOR_FLAGS) = CHL_U32(platform, PL_ACTOR_FLAGS) & 0xffffffdf;
		CRColl_RefreshMesh(platform, NULL);
		FUN_000beb70(PL_ACTOR(platform));
	}

	/* trails at its nodes */
	{
		void *trails = CHL_PTR(platform, PL_TRAILS);

		if (trails != NULL && CHL_I32(trails, 0x44) != 0) {
			for (i = 0; i < CHL_I32(CHL_PTR(platform, PL_TRAILS), 0x40); i++) {
				pl_GetNodePosition(CHL_FIELD(void, platform, PL_INST),
					CHL_PTR(CHL_PTR(platform, PL_TRAILS), 0x20 + i * 4), probe);
				pl_TrailAddPoint(CHL_PTR(CHL_PTR(platform, PL_TRAILS), i * 4), probe);
			}
		}
	}
}

/* 0x000b9880  CharLib (Platform.cpp, inferred)
Smashes a platform: smoke (with `smoke`), a copy from the smash model (data 0x440: name,
..., +0x18 sink, +0x30 no collision) that plays its "smash" animation and removes itself
at the end, and the platform is removed. Returns the copy. */
void *Platform_Smash(void *platform, int smoke)
{
	void *copy = NULL;
	char *smash = CHL_PTR(platform, PL_SMASH);
	void *anim;

	if (smash != NULL) {
		if (smoke != 0)
			Platform_Smoke(platform);
		smash = CHL_PTR(platform, PL_SMASH);
		if (*smash != '\0') {
			copy = Platform_Create(smash, CHL_FIELD(float, platform, PL_POS), CHL_I32(smash, 0x30));
			if (copy != NULL) {
				void *inst = CHL_FIELD(void, copy, PL_INST);

				anim = pl_FindSubObject(inst, "smash");
				if (anim != NULL) {
					chl_FlushAnimations(inst, CHL_PTR(copy, PL_ROOT_NODE), 1, 0);
					chl_QueueAnimation(inst, CHL_PTR(copy, PL_ROOT_NODE), anim, 0x400, -1, 0,
						(CHLAnimCallback)Platform_SmashAnimCallback, copy);
					CHL_U32(copy, PL_BOB_LEVEL_SINK) = CHL_U32(CHL_PTR(platform, PL_SMASH), 0x18);
				}
				memcpy(CHL_FIELD(char, copy, PL_PREV_EULER), CHL_FIELD(char, platform, PL_EULER), 0x10);
				memcpy(CHL_FIELD(char, copy, PL_EULER), CHL_FIELD(char, platform, PL_EULER), 0x10);
				CHL_I32(copy, PL_GROUP) = -CHL_I32(platform, PL_GROUP);
			}
		}
	}
	CHL_U32(platform, PL_FLAGS) = CHL_U32(platform, PL_FLAGS) | 0x80;
	chl_ListRemove(chl_actor_list, PL_ACTOR(platform));
	chl_ListRemove(pl_active_list, platform);
	chl_ListInsert(pl_inactive_list, platform, 0);
	return copy;
}

/* 0x000b99f0  CharLib (Platform.cpp, inferred) */
void Platforms_UpdateAll(void)
{
	void *platform;

	for (platform = chl_ListNext(pl_active_list, NULL); platform != NULL;
	     platform = chl_ListNext(pl_active_list, platform))
		Platform_Update(platform);
}
