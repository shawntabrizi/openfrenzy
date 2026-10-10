/*
CharLib/unplaced_ai.c  (charlib)

The CharLib AI player base class (0x000b01d0-0x000b18a0): construction, the state
machine, the steering along the route grid, spawning and respawning, holding objects.
The source file is not known (the gap between the last Actor.cpp tag and the
CRCollision.cpp tags; AI.cpp is tagged later, at 0x000b6710). Field offsets:
charlib_ai.h.
*/

#include "charlib.h"
#include "charlib_actor.h"
#include "charlib_ai.h"

/* ---------- globals */

/* 0x002f2898  how fast AI_COOLDOWN runs down (1.0) */
#define ai_cooldown_rate CHL_VAR(float, 0x002f2898)
/* 0x002f28ac  AI players jump now and then while riding a moving platform */
#define ai_platform_hops CHL_VAR(int, 0x002f28ac)
/* 0x002f27e4  default CLRoute.patience while the actor is in the air or moving cells */
#define ai_route_patience CHL_VAR(int, 0x002f27e4)
/* 0x002f28b0  the debug arrow quad of AI_DrawRouteTarget (4 vertices of 0x24 bytes) */
#define ai_debug_quad CHL_ARR(unsigned char, 0x002f28b0)
/* 0x00293928  volume of the respawn voice (200) */
#define ai_respawn_volume CHL_VAR(int, 0x00293928)
/* 0x0029392c  height above which a cell needs a jump (80) */
#define ai_jump_height CHL_VAR(int, 0x0029392c)
/* 0x00293930  wall frames (x 5) before jumping (4) */
#define ai_wall_jump_frames CHL_VAR(int, 0x00293930)
/* 0x00293934  climb frames before jumping (3) */
#define ai_climb_frames CHL_VAR(int, 0x00293934)
/* 0x00293938  facing a wall: dot product limit (-0.3) */
#define ai_wall_dot CHL_VAR(float, 0x00293938)
/* 0x0029393c  cell height range that counts as a step (100) */
#define ai_step_height CHL_VAR(int, 0x0029393c)
/* 0x00293940  frames stuck before jittering the target (5) */
#define ai_stuck_frames CHL_VAR(float, 0x00293940)
/* 0x00293944  jitter of the target when stuck: +-60 */
#define ai_stuck_jitter CHL_VAR(int, 0x00293944)
/* 0x00293948  AI_MODE_TIME when avoiding a stack (20) */
#define ai_unstack_time CHL_VAR(int, 0x00293948)
/* 0x0029394c  HUD value of a respawned player (180) */
#define ai_respawn_hud_time CHL_VAR(float, 0x0029394c)
/* 0x002939bc  respawn point timer after a spawn */
#define ai_spawn_point_time CHL_VAR(float, 0x002939bc)
/* 0x00293a28  speed factor while AI_TIMERS[0] runs (1.5) */
#define ai_boost_factor CHL_VAR(float, 0x00293a28)

/* the character table of classlib_ui (0x002906f8, 0x60 bytes per character): +0x24 base
acceleration, +0x28 top speed, +0x38 acceleration skill factor */
extern unsigned char DAT_002906f8[6 * 0x60];
#define ai_char_accel(c) CHL_F32(DAT_002906f8, (c) * 0x60 + 0x24)
#define ai_char_top_speed(c) CHL_F32(DAT_002906f8, (c) * 0x60 + 0x28)
#define ai_char_skill_factor(c) CHL_F32(DAT_002906f8, (c) * 0x60 + 0x38)
/* 0x002baed4  the player table of core (0x28 bytes per player); +0x00 the character */
extern int DAT_002baed4[];
#define ai_player_character(p) DAT_002baed4[(p) * 10]
/* 0x002baea4  the game is paused (core) */
extern int DAT_002baea4;
/* 0x002ecfac  the HUD object (classlib_ui): +0xb8 float per player, +0x180 text per player */
extern void *DAT_002ecfac;
/* 0x002ecc74  ClassLib font/colour object: +0x20 table of 16-byte entries */
extern void *DAT_002ecc74;
/* 0x00290e8c  character of the next respawn effect (bumpercars .data, 0) */
extern int DAT_00290e8c;
/* 0x00291734  the "RESPAWNCHAR" particle effect definition (bumpercars .data) */
extern unsigned char DAT_00291734[0x2c];
/* 0x002ea478  effect parameter block (ClassLib .bss) */
extern unsigned char DAT_002ea478[0x10];
/* the character data colour tables of bdisplay */
extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bc898[256];
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bd640[256];

/* _purecall: the slots of the abstract methods */
static void ai_purecall(void)
{
	chl_Printf("pure virtual function call\n");
	abort();
}

/* 0x00266fa8  the vtable of the AI base class (rebuilt from the XBE) */
static const void *const vtable_00266fa8[] = {
	(const void *)FUN_001d5950,    /* [0] +0x00 (empty) */
	(const void *)FUN_001d5950,    /* [1] +0x04 (empty) */
	(const void *)FUN_001d5950,    /* [2] +0x08 (empty) */
	(const void *)AI_Think,        /* [3] +0x0c */
	(const void *)FUN_00151650,    /* [4] +0x10 */
	(const void *)ai_purecall,     /* [5] +0x14 */
	(const void *)ai_purecall,     /* [6] +0x18 */
	(const void *)FUN_00151650,    /* [7] +0x1c */
	(const void *)AI_RespawnAt,    /* [8] +0x20 */
	(const void *)AI_RespawnFarthest, /* [9] +0x24 */
	(const void *)AI_Spawn,        /* [10] +0x28 */
	(const void *)FUN_001d5950,    /* [11] +0x2c (empty) */
	(const void *)AI_CanMove,      /* [12] +0x30 */
	(const void *)FUN_000a6fa0,    /* [13] +0x34 */
	(const void *)ai_purecall,     /* [14] +0x38 */
	(const void *)ai_purecall,     /* [15] +0x3c */
	(const void *)AI_HoldEnd,      /* [16] +0x40 */
	(const void *)FUN_001ec260,    /* [17] +0x44 */
};

/* ---------- bkernel vector helpers */

/* 0x00051570 out = in scaled to length len */
#define ai_Vec3SetLength ((void (*)(float *, float *, float))FUN_00051570)
/* 0x00051170 out = in rotated about y by angle */
#define ai_Vec3RotateY ((void (*)(float *, float *, float))FUN_00051170)
/* 0x000514a0 out = in normalised */
#define ai_Vec3Normalize ((void (*)(float *, float *))FUN_000514a0)
/* 0x00050ff0 out = a * sa + b * sb */
#define ai_Vec3Combine ((void (*)(float *, float *, float, float *, float))FUN_00050ff0)

/* ---------- other modules */

/* 0x00071720 CLSamples_PlayGeneric(index, volume, arg3, flags, pos) */
#define ai_PlayGeneric ((int (*)(int, int, int, unsigned int, float *))FUN_00071720)
/* 0x00071770 CLSamples_PlayFromSet(set, kind, pos, volume, flags, choice, allow_repeat) */
#define ai_PlayFromSet ((int (*)(char, int, float *, int, unsigned int, int, int))FUN_00071770)
/* 0x000719e0 CLSamples_PlayVoice(character, kind, pos, volume, cooldown, flags, choice,
allow_repeat) */
#define ai_PlayVoice ((int (*)(char, int, float *, int, float, unsigned int, int, int))FUN_000719e0)
/* 0x00080d60 start a particle effect (definition, position, parameters, 0, -1) */
#define ai_StartEffect ((int (*)(void *, float *, void *, int, int))FUN_00080d60)
/* 0x00082b20 (bumpercars) the four player positions */
#define ai_PlayerPositions ((void (*)(float *, float *, float *, float *))FUN_00082b20)

/* the list of respawn points: the CharLib world object (chl_actor_list) + 0x30;
a point has its position at +0x08, the player using it at +0x18 (short, -1 free), its
id at +0x1a (short) and a timer at +0x1c */
#define ai_respawn_list ((void *)((char *)chl_actor_list + 0x30))

/* 0x000b01d0  CharLib (file unknown)
The AI base constructor: the route, then every field cleared (the vtable too, as in the
original: the derived constructor sets its own) and the defaults. */
void *AI_Ctor(void *ai)
{
	void *cost;
	void *waypoints;
	void *parent;

	*(const void *const **)ai = vtable_00266fa8;
	CLRoute_Ctor(CHL_FIELD(struct CLRoute, ai, AI_ROUTE));
	ai_cooldown_rate = 1.0f;
	cost = CHL_PTR(ai, AI_ROUTE + 0xfb0);
	waypoints = CHL_PTR(ai, AI_ROUTE + 0xfc0);
	parent = CHL_PTR(ai, AI_ROUTE + 0xfc8);
	memset(ai, 0, 0x44c * 4);
	CHL_I16(ai, AI_SHORT_1088) = -1;
	CHL_I16(ai, AI_SHORT_1086) = -1;
	CHL_F32(ai, 0x10f8) = 200.0f;
	CHL_F32(ai, 0x1100) = 200.0f;
	CHL_F32(ai, AI_SPEED) = 200.0f;
	CHL_PTR(ai, AI_ROUTE + 0xfb0) = cost;
	CHL_F32(ai, AI_SKILL) = 0.0f;
	CHL_I32(ai, AI_RESPAWNS) = 0;
	CHL_PTR(ai, AI_ROUTE + 0xfc0) = waypoints;
	CHL_PTR(ai, AI_ROUTE + 0xfc8) = parent;
	CHL_I16(ai, AI_ROUTE + 0xc) = 1; /* route finished */
	CHL_F32(ai, 0x10e8) = 0.1f;
	CHL_F32(ai, 0x10ec) = -30.0f;
	CHL_U32(ai, 0x10f0) = 0x3f860a91; /* 1.0472 (pi / 3) */
	CHL_F32(ai, 0x10f4) = 300.0f;
	CHL_U32(ai, 0x10fc) = 0x3f490fda; /* pi / 4 */
	CHL_I32(ai, 0x1104) = 6;
	CHL_I32(ai, 0x1108) = 6;
	CHL_I32(ai, 0x110c) = 5;
	CHL_I32(ai, 0x1110) = 100;
	CHL_I32(ai, AI_FIRST_SPAWN_DONE) = 1;
	return ai;
}

/* 0x000b02d0  CharLib (file unknown) */
void AI_Dtor(void *ai)
{
	*(const void *const **)ai = vtable_00266fa8;
	CLRoute_Dtor(CHL_FIELD(struct CLRoute, ai, AI_ROUTE));
}

/* 0x000b02f0  CharLib (file unknown)
Switches the state machine to `state` and calls its handler with entered = 1. */
void AI_SetState(void *ai, int state)
{
	CHL_U16(ai, AI_PREV_STATE) = CHL_U16(ai, AI_STATE);
	CHL_F32(ai, AI_STATE_TIME) = 0.0f;
	CHL_I32(ai, AI_STATE_FRAMES) = 0;
	CHL_U16(ai, AI_STATE) = (unsigned short)state;
	((void (**)(void *, int))CHL_PTR(ai, AI_STATE_FUNCS))[(unsigned short)state](ai, 1);
}

/* 0x000b0320  CharLib (file unknown)
Plays a voice of the player's character at the actor. */
void AI_PlayVoice(void *ai, int kind, int volume, float cooldown, unsigned int flags, int choice, int allow_repeat)
{
	void *actor = CHL_PTR(ai, AI_ACTOR);

	ai_PlayVoice((char)ai_player_character(CHL_U16(actor, ACT_PLAYER)), kind, CHL_FIELD(float, actor, ACT_POS),
		volume, cooldown, flags, choice, allow_repeat);
}

/* the sounds and HUD update of a respawn (all but the first spawn) */
static void ai_respawn_feedback(void *ai)
{
	void *actor = CHL_PTR(ai, AI_ACTOR);
	unsigned int player = CHL_U16(actor, ACT_PLAYER);
	void *text;

	if (CHL_I32(ai, AI_RESPAWNS) == 0)
		return;
	ai_PlayGeneric(5, 0x7f, 0, 1, NULL);
	ai_PlayFromSet((char)ai_player_character(CHL_U16(CHL_PTR(ai, AI_ACTOR), ACT_PLAYER)), 0xf,
		CHL_FIELD(float, CHL_PTR(ai, AI_ACTOR), ACT_POS), ai_respawn_volume, 0, -1, 0);
	player = CHL_U16(CHL_PTR(ai, AI_ACTOR), ACT_PLAYER);
	CHL_F32(DAT_002ecfac, 0xb8 + player * 4) = ai_respawn_hud_time;
	player = CHL_U16(CHL_PTR(ai, AI_ACTOR), ACT_PLAYER);
	text = CHL_PTR(DAT_002ecfac, 0x180 + player * 4);
	CHL_U32(text, 0x2a4) = CHL_U32(CHL_PTR(DAT_002ecc74, 0x20), (player + 400) * 0x10);
	player = CHL_U16(CHL_PTR(ai, AI_ACTOR), ACT_PLAYER);
	CHL_PTR(CHL_PTR(DAT_002ecfac, 0x180 + player * 4), 0xa0) =
		chl_FindObject(NULL, "handgotb.ttf", 3, 0xdefa);
}

/* 0x000b0370  CharLib (file unknown)
vtable slot 9: respawns at the respawn point farthest from the other players (the one
whose nearest other player is farthest away); without respawn points, AI_Spawn(NULL). */
void AI_RespawnFarthest(void *ai)
{
	void *point;
	void *best = NULL;
	float best_dist = 0.0f;
	float nearest;
	float d;
	unsigned int i;

	ai_respawn_feedback(ai);
	CHL_I32(ai, AI_RESPAWNS) = CHL_I32(ai, AI_RESPAWNS) + 1;
	CHL_U32(CHL_PTR(ai, AI_ACTOR), ACT_FLAGS) = CHL_U32(CHL_PTR(ai, AI_ACTOR), ACT_FLAGS) & 0xfffffeff;
	if (CHL_I32(chl_actor_list, 0x38) == 0) {
		AI_VCALL(ai, AI_VSLOT_SPAWN, void (*)(void *, void *))(ai, NULL);
		return;
	}
	for (point = chl_ListNext(ai_respawn_list, NULL); point != NULL; point = chl_ListNext(ai_respawn_list, point)) {
		nearest = 1e16f;
		for (i = 0; (int)i < chl_player_count; i++) {
			if (i != CHL_U16(CHL_PTR(ai, AI_ACTOR), ACT_PLAYER)) {
				d = FUN_00070db0(CHL_FIELD(float, CHL_PTR(chl_ai_players[i], AI_ACTOR), ACT_POS),
					CHL_FIELD(float, point, 0x8));
				if (d < nearest)
					nearest = d;
			}
		}
		if (best_dist < nearest) {
			best_dist = nearest;
			best = point;
		}
	}
	if (CHL_I16(best, 0x18) != -1) {
		CHL_F32(best, 0x1c) = 60.0f;
		RespawnPoint_Update(best);
	}
	AI_VCALL(ai, AI_VSLOT_SPAWN, void (*)(void *, void *))(ai, best);
}

/* 0x000b0560  CharLib (file unknown)
vtable slot 8: respawns at the respawn point with id `id`, or AI_Spawn(NULL). */
void AI_RespawnAt(void *ai, int id)
{
	void *point;

	ai_respawn_feedback(ai);
	CHL_I32(ai, AI_RESPAWNS) = CHL_I32(ai, AI_RESPAWNS) + 1;
	CHL_U32(CHL_PTR(ai, AI_ACTOR), ACT_FLAGS) = CHL_U32(CHL_PTR(ai, AI_ACTOR), ACT_FLAGS) & 0xfffffeff;
	for (point = chl_ListNext(ai_respawn_list, NULL); point != NULL; point = chl_ListNext(ai_respawn_list, point)) {
		if (CHL_I16(point, 0x1a) == id) {
			AI_VCALL(ai, AI_VSLOT_SPAWN, void (*)(void *, void *))(ai, point);
			CHL_F32(point, 0x1c) = 60.0f;
			RespawnPoint_Update(point);
			return;
		}
	}
	AI_VCALL(ai, AI_VSLOT_SPAWN, void (*)(void *, void *))(ai, NULL);
}

/* 0x000b06c0  CharLib (file unknown)
vtable slot 12: may the AI move in this direction (the base class always agrees). */
int AI_CanMove(void *ai, float *dir)
{
	(void)ai;
	(void)dir;
	return 1;
}

/* 0x000b06d0  CharLib (file unknown)
The player picks up `actor` into hold slot `slot`: the actor is no longer prepared on its
own (ACT_NO_PREP) and moves to the end of the actor list (drawn after the player). */
void AI_Hold(void *ai, void *actor, int slot)
{
	CHL_I32(actor, ACT_NO_PREP) = 1;
	CHL_PTR(ai, AI_HELD + slot * 4) = actor;
	chl_ListRemove(chl_actor_list, actor);
	chl_ListInsert(chl_actor_list, actor, 1);
}

/* 0x000b0710  CharLib (file unknown)
The squared x/z distance from pos to the nearest player that holds something. */
float AI_NearestHolderDistSq(float *pos)
{
	float nearest = 1e9f;
	float d;
	int i;

	for (i = 0; i < 4; i++) {
		if (CHL_PTR(chl_ai_players[i], AI_HELD) != NULL) {
			d = FUN_00070db0(pos, CHL_FIELD(float, CHL_PTR(chl_ai_players[i], AI_ACTOR), ACT_POS));
			if (d < nearest)
				nearest = d;
		}
	}
	return nearest;
}

/* 0x000b0770  CharLib (file unknown)
vtable slot 16 (empty in the base class). */
void AI_HoldEnd(void *ai, int a, int b)
{
	(void)ai;
	(void)a;
	(void)b;
}

/* 0x000b0780  CharLib (file unknown)
Sets the fake stick from a world direction: scaled to amount * 32760, turned into the
camera's frame; x goes to the x axis, -z to the y axis. Directions shorter than 1e-4
leave the stick alone. */
void AI_SetStick(void *ai, float *dir, float amount)
{
	float local[4];

	if (sqrtf(dir[2] * dir[2] + dir[1] * dir[1] + dir[0] * dir[0]) < 1e-4f)
		return;
	ai_Vec3SetLength(dir, dir, amount * 32760.0f);
	ai_Vec3RotateY(local, dir, -CHL_F32(ai, AI_CAMERA_ANGLE));
	CHL_I32(CHL_PTR(ai, AI_STICK_Y), 0x14) = -CHL_FTOL(local[2]);
	CHL_I32(CHL_PTR(ai, AI_STICK_X), 0x14) = CHL_FTOL(local[0]);
}

/* 0x000b0820  CharLib (file unknown)
The per-frame AI update of an active player: reads the stick back into a world
direction, runs the state machine (conditions, then the handler of the state), applies
the push velocity and caps the actor's x/z speed to the character's top speed. */
void AI_Update(void *ai)
{
	void *actor = CHL_PTR(ai, AI_ACTOR);
	int c = ai_player_character(CHL_U16(actor, ACT_PLAYER));
	signed char best = 0;
	signed char priority;
	int chosen = 0;
	float top;
	float f;
	float stick[4];
	int i;

	if (CHL_I16(ai, AI_DISABLED) != 0)
		return;
	if (ai_char_top_speed(c) <= CHL_F32(ai, AI_SPEED))
		top = ai_char_top_speed(c);
	else
		top = CHL_F32(ai, AI_SPEED);
	f = (1.0f - (1.0f - ai_char_skill_factor(c)) * CHL_F32(ai, AI_SKILL)) * ai_char_accel(c);
	CHL_F32(ai, AI_MAX_SPEED_SCALE) = f;
	if (CHL_F32(ai, AI_TIMERS) != 0.0f) {
		top = ai_boost_factor * top;
		CHL_F32(ai, AI_MAX_SPEED_SCALE) = f * ai_boost_factor;
	}
	stick[1] = 0.0f;
	stick[0] = (float)CHL_I32(CHL_PTR(ai, AI_STICK_X), 0x14) * 3.05250323e-05f;
	stick[2] = (float)CHL_I32(CHL_PTR(ai, AI_STICK_Y), 0x14) * -3.05250323e-05f;
	f = sqrtf(stick[0] * stick[0] + stick[2] * stick[2]);
	if (1.0f < f)
		f = 1.0f;
	CHL_F32(ai, AI_STICK_AMOUNT) = f;
	ai_Vec3RotateY(CHL_FIELD(float, ai, AI_MOVE_DIR), stick, CHL_F32(ai, AI_CAMERA_ANGLE));
	ai_Vec3Normalize(CHL_FIELD(float, ai, AI_MOVE_DIR), CHL_FIELD(float, ai, AI_MOVE_DIR));
	CHL_F32(ai, AI_MOVE_HEADING) = FUN_000703a0((float)CHL_FPATAN(CHL_F32(ai, AI_MOVE_DIR + 0x0),
		CHL_F32(ai, AI_MOVE_DIR + 0x8)));

	/* the conditions of the current state */
	for (i = 0; i < (int)CHL_U16(ai, AI_COND_COUNT); i++) {
		priority = CHL_I8(CHL_PTR(ai, AI_PRIORITY),
			CHL_U16(ai, AI_STATE) * CHL_U16(ai, AI_COND_COUNT) + i);
		if (best < priority && ((int (**)(void *))CHL_PTR(ai, AI_COND_FUNCS))[i](ai) != 0) {
			best = priority;
			chosen = i;
		}
	}
	CHL_I32(ai, AI_STATE_FRAMES) = CHL_I32(ai, AI_STATE_FRAMES) + 1;
	CHL_F32(ai, AI_STATE_TIME) = chl_time_scale + CHL_F32(ai, AI_STATE_TIME);
	if (best != 0) {
		CHL_U16(ai, AI_PREV_STATE) = CHL_U16(ai, AI_STATE);
		CHL_U16(ai, AI_STATE) = (unsigned short)chosen;
		CHL_F32(ai, AI_STATE_TIME) = 0.0f;
		CHL_I32(ai, AI_STATE_FRAMES) = 0;
	}
	((void (**)(void *, int))CHL_PTR(ai, AI_STATE_FUNCS))[CHL_U16(ai, AI_STATE)](ai, best != 0);

	if (CHL_F32(ACT_CD(CHL_PTR(ai, AI_ACTOR)), CD_STUN_TIME) == 0.0f) {
		float *vel = CHL_FIELD(float, CHL_PTR(ai, AI_ACTOR), ACT_VEL);

		ai_Vec3Combine(vel, vel, 1.0f, CHL_FIELD(float, ai, AI_PUSH), chl_time_scale);
	}
	if (CHL_F32(ai, AI_PUSH + 0x8) * CHL_F32(ai, AI_PUSH + 0x8) +
	    CHL_F32(ai, AI_PUSH + 0x4) * CHL_F32(ai, AI_PUSH + 0x4) +
	    CHL_F32(ai, AI_PUSH + 0x0) * CHL_F32(ai, AI_PUSH + 0x0) != 0.0f) {
		actor = CHL_PTR(ai, AI_ACTOR);
		f = sqrtf(CHL_F32(actor, ACT_VEL + 0x8) * CHL_F32(actor, ACT_VEL + 0x8) +
			CHL_F32(actor, ACT_VEL + 0x0) * CHL_F32(actor, ACT_VEL + 0x0));
		if (top < f) {
			top = top / f;
			CHL_F32(actor, ACT_VEL + 0x0) = top * CHL_F32(actor, ACT_VEL + 0x0);
			CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_VEL + 0x8) = top * CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_VEL + 0x8);
		}
	}
	actor = CHL_PTR(ai, AI_ACTOR);
	CHL_F32(actor, 0x254) = sqrtf(CHL_F32(actor, ACT_VEL + 0x8) * CHL_F32(actor, ACT_VEL + 0x8) +
		CHL_F32(actor, ACT_VEL + 0x4) * CHL_F32(actor, ACT_VEL + 0x4) +
		CHL_F32(actor, ACT_VEL + 0x0) * CHL_F32(actor, ACT_VEL + 0x0));
}

/* 0x000b0b00  CharLib (file unknown)
Debug view: a flat arrow quad from the actor to the route's target cell centre. */
void AI_DrawRouteTarget(void *ai)
{
	unsigned char *v = ai_debug_quad;
	void *actor = CHL_PTR(ai, AI_ACTOR);
	float d[3];
	float side;
	unsigned int colour;

	d[0] = CHL_F32(ai, AI_ROUTE + 0xfcc) - CHL_F32(actor, ACT_POS + 0x0);
	d[1] = CHL_F32(ai, AI_ROUTE + 0xfd0) - CHL_F32(actor, ACT_POS + 0x4);
	d[2] = CHL_F32(ai, AI_ROUTE + 0xfd4) - CHL_F32(actor, ACT_POS + 0x8);
	ai_Vec3SetLength(d, d, 15.0f);
	side = -d[0];
	colour = DAT_002bc898[0x60] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0x7f];
	CHL_U32(v, 0x18) = colour;
	CHL_F32(v, 0x00) = CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x0) - d[2];
	CHL_F32(v, 0x04) = CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x4);
	CHL_F32(v, 0x08) = CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x8) - side;
	CHL_F32(v, 0x24) = CHL_F32(ai, AI_ROUTE + 0xfcc) - d[2];
	CHL_F32(v, 0x28) = CHL_F32(ai, AI_ROUTE + 0xfd0);
	CHL_F32(v, 0x2c) = CHL_F32(ai, AI_ROUTE + 0xfd4) - side;
	CHL_F32(v, 0x48) = d[2] + CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x0);
	CHL_F32(v, 0x4c) = CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x4);
	CHL_F32(v, 0x50) = side + CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x8);
	CHL_F32(v, 0x6c) = d[2] + CHL_F32(ai, AI_ROUTE + 0xfcc);
	CHL_F32(v, 0x70) = CHL_F32(ai, AI_ROUTE + 0xfd0);
	CHL_F32(v, 0x74) = side + CHL_F32(ai, AI_ROUTE + 0xfd4);
	CHL_U32(v, 0x3c) = colour;
	CHL_U32(v, 0x60) = colour;
	CHL_U32(v, 0x84) = colour;
	chl_SetTexture(0, NULL);
	chl_SetRenderState(7, 1, 0);
	chl_SetRenderState(6, 1, 0);
	chl_SetRenderState(4, 0, 0);
	chl_SetRenderState(10, 0, 0);
	chl_DrawPrimitive(4, v, 4);
	chl_SetRenderState(6, 0, 0);
	chl_SetRenderState(4, 1, 0);
	chl_SetRenderState(7, 3, 0);
}

/* 0x000b0cb0  CharLib (file unknown)
vtable slot 10: (re)spawns the player at a respawn point (or parks it at (0, 8000, 0)
without one): random offset of +-0.5 units, facing pi, on the ground, standing still,
state 0, pushing away other players who overlap. Spawns after the first start the
"RESPAWNCHAR" effect and a sound; the actor grows in (scale y from 0). */
void AI_Spawn(void *ai, void *point)
{
	void *actor;
	void *other;
	float d[4];
	float dist;
	float h;
	unsigned int i;

	if (point == NULL) {
		actor = CHL_PTR(ai, AI_ACTOR);
		CHL_F32(actor, ACT_POS + 0x8) = 0.0f;
		CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x0) = 0.0f;
		CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x4) = 8000.0f;
		return;
	}
	actor = CHL_PTR(ai, AI_ACTOR);
	memcpy(CHL_FIELD(char, actor, ACT_POS), CHL_FIELD(char, point, 0x8), 0x10);
	CHL_U16(point, 0x18) = CHL_U16(CHL_PTR(ai, AI_ACTOR), ACT_PLAYER);
	CHL_F32(point, 0x1c) = ai_spawn_point_time;
	actor = CHL_PTR(ai, AI_ACTOR);
	CHL_F32(actor, ACT_POS + 0x0) = (float)(CHL_Rand() % 0x65 - 0x32) * 0.01f + CHL_F32(actor, ACT_POS + 0x0);
	actor = CHL_PTR(ai, AI_ACTOR);
	CHL_F32(actor, ACT_POS + 0x8) = (float)(CHL_Rand() % 0x65 - 0x32) * 0.01f + CHL_F32(actor, ACT_POS + 0x8);
	CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_HEADING) = 3.1415925f;
	CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_HEADING_WANTED) = 3.1415925f;
	actor = CHL_PTR(ai, AI_ACTOR);
	h = CLColl_GroundProbe(CHL_FIELD(float, point, 0x8), 0.0f, NULL, NULL, 0);
	CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_POS + 0x4) = CHL_F32(actor, ACT_POS + 0x4) - h;
	actor = CHL_PTR(ai, AI_ACTOR);
	memcpy(CHL_FIELD(char, actor, ACT_POS_PREV), CHL_FIELD(char, actor, ACT_POS), 0x10);
	memcpy(CHL_FIELD(char, actor, ACT_POS_PREMOVE), CHL_FIELD(char, actor, ACT_POS), 0x10);
	CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_VEL + 0x8) = 0.0f;
	CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_VEL + 0x4) = 0.0f;
	CHL_F32(CHL_PTR(ai, AI_ACTOR), ACT_VEL + 0x0) = 0.0f;
	CHL_I16(ai, AI_DISABLED) = 1;
	CHL_U16(ai, AI_PREV_STATE) = CHL_U16(ai, AI_STATE);
	CHL_U16(ai, AI_STATE) = 0;
	CHL_F32(ai, AI_STATE_TIME) = 0.0f;
	CHL_I32(ai, AI_STATE_FRAMES) = 0;
	((void (**)(void *, int))CHL_PTR(ai, AI_STATE_FUNCS))[0](ai, 1);

	/* push overlapping players away */
	for (i = 0; (int)i < chl_player_count; i++) {
		actor = CHL_PTR(ai, AI_ACTOR);
		if (i == CHL_U16(actor, ACT_PLAYER))
			continue;
		other = CHL_PTR(chl_ai_players[i], AI_ACTOR);
		d[0] = CHL_F32(other, ACT_POS + 0x0) - CHL_F32(actor, ACT_POS + 0x0);
		d[2] = CHL_F32(other, ACT_POS + 0x8) - CHL_F32(actor, ACT_POS + 0x8);
		d[1] = 0.0f;
		other = CHL_PTR(chl_ai_players[i], AI_ACTOR);
		dist = sqrtf(d[0] * d[0] + d[2] * d[2]);
		if (dist < CHL_F32(ACT_CD(other), 0x9c) + CHL_F32(ACT_CD(actor), 0x9c)) {
			if (dist == 0.0f)
				CHL_F32(other, ACT_VEL + 0x0) = CHL_F32(other, ACT_VEL + 0x0) + 20.0f;
			else
				ai_Vec3Combine(CHL_FIELD(float, other, ACT_VEL), CHL_FIELD(float, other, ACT_VEL), 1.0f, d,
					20.0f / dist);
		}
	}
	if (CHL_I32(ai, AI_FIRST_SPAWN_DONE) == 0) {
		CHL_I32(ai, AI_FIRST_SPAWN_DONE) = 1;
		return;
	}
	actor = CHL_PTR(ai, AI_ACTOR);
	memcpy(d, CHL_FIELD(char, actor, ACT_POS), 0x10);
	CHL_F32(actor, ACT_SCALE + 0x4) = 0.0f;
	DAT_00290e8c = ai_player_character(CHL_U16(CHL_PTR(ai, AI_ACTOR), ACT_PLAYER));
	ai_StartEffect(DAT_00291734, d, DAT_002ea478, 0, -1);
	ai_PlayGeneric(5, 0x7f, 0, 1, NULL);
}

/* 0x000b1030  CharLib (file unknown)
Per frame: cooldown, the paused hook, AI_Update, the four timers (slot 14 when one runs
out) and the timer at 0x10d0. */
void AI_Tick(void *ai)
{
	float f;
	int i;

	f = CHL_F32(ai, AI_COOLDOWN) - ai_cooldown_rate * chl_time_scale;
	CHL_F32(ai, AI_COOLDOWN) = f;
	if (f < 0.0f)
		CHL_F32(ai, AI_COOLDOWN) = 0.0f;
	if (DAT_002baea4 != 0)
		AI_VCALL(ai, AI_VSLOT_PAUSED, void (*)(void *))(ai);
	AI_Update(ai);
	for (i = 0; i < 4; i++) {
		float *timer = CHL_FIELD(float, ai, AI_TIMERS + i * 4);

		if (*timer != 0.0f) {
			f = *timer - chl_time_scale;
			*timer = f;
			if (f <= 0.0f) {
				*timer = 0.0f;
				AI_VCALL(ai, AI_VSLOT_TIMER, void (*)(void *, int))(ai, i);
			}
		}
	}
	if (0.0f < CHL_F32(ai, AI_TIMER_10D0)) {
		f = CHL_F32(ai, AI_TIMER_10D0) - chl_time_scale;
		CHL_F32(ai, AI_TIMER_10D0) = f;
		if (f < 0.0f)
			CHL_F32(ai, AI_TIMER_10D0) = 0.0f;
	}
}

/* 0x000b1100  CharLib (file unknown)
Runs every AI player for one frame (slot 0, AI_Tick, slot 1), then hands the four
player positions to FUN_00082b20 (bumpercars). */
void AI_TickAll(void)
{
	int i;

	for (i = 0; i < chl_player_count; i++) {
		AI_VCALL(chl_ai_players[i], AI_VSLOT_UPDATE_PRE, void (*)(void *))(chl_ai_players[i]);
		AI_Tick(chl_ai_players[i]);
		AI_VCALL(chl_ai_players[i], AI_VSLOT_UPDATE_POST, void (*)(void *))(chl_ai_players[i]);
	}
	ai_PlayerPositions(CHL_FIELD(float, CHL_PTR(chl_ai_players[0], AI_ACTOR), ACT_POS),
		CHL_FIELD(float, CHL_PTR(chl_ai_players[1], AI_ACTOR), ACT_POS),
		CHL_FIELD(float, CHL_PTR(chl_ai_players[2], AI_ACTOR), ACT_POS),
		CHL_FIELD(float, CHL_PTR(chl_ai_players[3], AI_ACTOR), ACT_POS));
}

/* 0x000b1190  CharLib (file unknown)
Steers the player along its route: replans when it changed cells (cycles make it more
patient), picks the next cell (or the destination itself when close), jitters the target
when stuck, sets the stick, and presses jump to climb into higher cells or when it keeps
walking into a wall. */
void AI_FollowRoute(void *ai)
{
	struct CLRoute *r = CHL_FIELD(struct CLRoute, ai, AI_ROUTE);
	void *actor = CHL_PTR(ai, AI_ACTOR);
	struct CLRouteCell *grid;
	float y = CHL_F32(actor, ACT_POS + 0x4);
	float d[4];
	float step[4];
	float target[4];
	float dist;
	float len;
	float f;
	int cell;
	int next;
	int ahead;
	int mode;
	int i;
	void **planes;

	if (r->path[0] != 0)
		return;
	cell = CLRoute_CellAt(CHL_FIELD(float, actor, ACT_POS));
	if (cell == r->next_cell && CHL_I16(ACT_CD(CHL_PTR(ai, AI_ACTOR)), 100) != 0)
		r->stuck_time = chl_time_scale + r->stuck_time;
	else
		r->stuck_time = 0.0f;
	mode = CHL_I32(CHL_PTR(ai, AI_ACTOR), 0x214);
	if (mode == 1 || mode == 2 || (cell != r->cur_cell && mode != 0)) {
		r->patience = ai_route_patience;
		if (cell != r->cur_cell && (int)r->waypoint_count < route_cells) {
			int n = r->waypoint_count;

			for (i = 0; i < r->waypoint_count; i++) {
				if (cell == r->waypoints[i]) {
					r->patience = route_patience;
					break;
				}
			}
			r->waypoints[n] = (short)cell;
			r->waypoint_count = (short)(r->waypoint_count + 1);
		}
		if (cell != r->cur_cell)
			r->state = 0;
		r->next_cell = (short)CLRoute_NextCell(CHL_FIELD(float, CHL_PTR(ai, AI_ACTOR), ACT_POS),
			r->dst_centre, y, r);
		r->cur_cell = (short)cell;
		CHL_I16(ai, AI_CLIMB_COUNT) = 0;
	}
	if (CHL_I32(CHL_PTR(ai, AI_ACTOR), 0x214) != 0 && (r->cur_cell == r->next_cell || r->next_cell == -1)) {
		r->path[0] = 1;
		goto done;
	}
	next = r->next_cell;
	if (next < 0) {
		if (r->cost[r->src_cell] == 32000 || r->state <= r->patience)
			goto done;
		next = CLRoute_PathStep(r, cell, r->dst_cell, y);
		if (next == -1)
			next = r->parent[cell];
	}
	CLRoute_CellCentre(target, next);
	FUN_00070580(d, target, CHL_FIELD(float, CHL_PTR(ai, AI_ACTOR), ACT_POS));
	if (ai_stuck_frames < r->stuck_time) {
		int range = ai_stuck_jitter * 2 + 1;
		int jitter = ai_stuck_jitter;
		int rx = CHL_Rand();
		int rz = CHL_Rand();

		r->stuck_time = 0.0f;
		d[0] = (float)(rx % range - jitter) + d[0];
		d[2] = (float)(rz % range - jitter) + d[2];
	}
	d[1] = 0.0f;
	d[3] = 0.0f; /* not set in the original (stack garbage); only copied */
	dist = sqrtf(d[0] * d[0] + d[2] * d[2]);
	if (dist < r->arrive_radius) {
		FUN_00070580(d, r->dst_pos, CHL_FIELD(float, CHL_PTR(ai, AI_ACTOR), ACT_POS));
		dist = sqrtf(d[0] * d[0] + d[2] * d[2]);
	}
	d[1] = 0.0f;
	if (dist == 0.0f)
		goto done;
	actor = CHL_PTR(ai, AI_ACTOR);
	f = dist;
	if (CHL_F32(actor, 0x254) <= dist)
		f = CHL_F32(actor, 0x254);
	if (1.0f <= f) {
		len = dist;
		if (CHL_F32(actor, 0x254) <= dist)
			len = CHL_F32(actor, 0x254);
		if (len == 0.0f)
			goto done;
	} else {
		len = 1.0f;
	}
	ai_Vec3SetLength(d, d, len);
	memcpy(step, d, 0x10);
	if (AI_VCALL(ai, AI_VSLOT_CAN_MOVE, int (*)(void *, float *))(ai, step) == 0)
		return;
	if (CHL_F32(ai, AI_SLOW_RADIUS) <= dist)
		dist = 1.0f;
	else
		dist = dist / CHL_F32(ai, AI_SLOW_RADIUS);
	AI_SetStick(ai, step, dist);

	/* one cell ahead */
	actor = CHL_PTR(ai, AI_ACTOR);
	f = route_size_x / ((float)route_cols * len);
	step[0] = d[0] * f;
	step[1] = d[1] * f;
	step[2] = f * d[2];
	d[0] = step[0] + CHL_F32(actor, ACT_POS + 0x0);
	d[1] = step[1] + CHL_F32(actor, ACT_POS + 0x4);
	d[2] = step[2] + CHL_F32(actor, ACT_POS + 0x8);
	ahead = CLRoute_CellAt(d);
	actor = CHL_PTR(ai, AI_ACTOR);
	if (CHL_I32(actor, 0x214) == 0 || ahead == r->cur_cell) {
		CHL_I16(ai, AI_CLIMB_COUNT) = 0;
		goto done;
	}
	grid = route_grid;
	{
		struct CLRouteCell *here = &grid[r->cur_cell];
		struct CLRouteCell *there = &grid[ahead];

		if ((here->bottom != here->top && here->top - here->bottom <= (float)ai_step_height) ||
		    (there->bottom != there->top && there->top - there->bottom <= (float)ai_step_height &&
		     there->bottom <= here->bottom)) {
			/* uneven cells: jump when climbing and the step is high */
			if (CHL_F32(actor, ACT_POS_PREMOVE + 0x4) <= CHL_F32(actor, ACT_POS + 0x4)) {
				if (there->bottom <= (float)ai_jump_height + y)
					goto facing;
				CHL_I16(ai, AI_CLIMB_COUNT) = (short)(CHL_I16(ai, AI_CLIMB_COUNT) + 1);
				if (CHL_I16(ai, AI_CLIMB_COUNT) <= ai_climb_frames)
					goto facing;
				CHL_I32(CHL_PTR(ai, AI_BUTTON), 0x14) = 1;
			}
			CHL_I16(ai, AI_CLIMB_COUNT) = 0;
		} else {
			CHL_I16(ai, AI_CLIMB_COUNT) = 0;
			if ((float)ai_jump_height + y < there->top && there->top <= route_max_climb + y)
				CHL_I32(CHL_PTR(ai, AI_BUTTON), 0x14) = 1;
		}
	}
facing:
	/* walking into a wall: jump after a while */
	d[0] = step[0];
	d[2] = step[2];
	d[3] = step[3];
	d[1] = 0.0f;
	ai_Vec3Normalize(d, d);
	planes = (void **)ACT_CD(CHL_PTR(ai, AI_ACTOR));
	for (i = 0; i < (int)CHL_U16(ACT_CD(CHL_PTR(ai, AI_ACTOR)), 100); i++) {
		void *plane = planes[i];

		if (CHL_F32(plane, 0x34) < 0.7f &&
		    CHL_F32(plane, 0x30) * d[0] + CHL_F32(plane, 0x38) * d[2] < ai_wall_dot) {
			CHL_I16(ai, AI_WALL_COUNT) = (short)(CHL_I16(ai, AI_WALL_COUNT) + 5);
			if (ai_wall_jump_frames * 5 < (int)CHL_I16(ai, AI_WALL_COUNT)) {
				CHL_I32(CHL_PTR(ai, AI_BUTTON), 0x14) = 1;
				CHL_I16(ai, AI_WALL_COUNT) = 0;
			}
			goto done;
		}
	}
	if (CHL_I16(ai, AI_WALL_COUNT) != 0)
		CHL_I16(ai, AI_WALL_COUNT) = (short)(CHL_I16(ai, AI_WALL_COUNT) - 1);
done:
	if (r->next_cell == -2)
		r->next_cell = (short)CLRoute_NextCell(CHL_FIELD(float, CHL_PTR(ai, AI_ACTOR), ACT_POS),
			r->dst_centre, y, r);
}

/* 0x000b18a0  CharLib (file unknown)
The second half of AI_Think (Ghidra split the function here; the original reaches it
only from 0x000b1860): replans every AI_THINK_INTERVAL frames, hops on moving platforms,
avoids standing on or under other players (mode 6), then runs slots 11 and 5. */
void AI_ThinkTail(void *ai)
{
	void *actor;
	void *other;
	void *platform;
	float dx;
	float dy;
	float dz;
	float dxz2;
	float t;
	unsigned int i;
	int n;

	t = chl_time_scale + CHL_F32(ai, AI_THINK_TIME);
	CHL_F32(ai, AI_THINK_TIME) = t;
	if (CHL_F32(ai, AI_THINK_INTERVAL) <= t || CHL_I32(ai, AI_FORCE_THINK) != 0) {
		if (AI_VCALL(ai, AI_VSLOT_WANTS_MODE, int (*)(void *))(ai) != 0)
			AI_VCALL(ai, AI_VSLOT_SET_MODE, void (*)(void *, int))(ai, -1);
		AI_FollowRoute(ai);
		if (CHL_I16(ai, AI_ROUTE + 0xfb6) == -1)
			AI_VCALL(ai, AI_VSLOT_SET_MODE, void (*)(void *, int))(ai, -1);
	}
	if (ai_platform_hops != 0 &&
	    (platform = CHL_PTR(ACT_CD(CHL_PTR(ai, AI_ACTOR)), CD_PLATFORM)) != NULL &&
	    CHL_PTR(platform, 0x448) != NULL && CHL_Rand() % 100 < 5)
		CHL_I32(CHL_PTR(ai, AI_BUTTON), 0x14) = 1;
	if (CHL_I32(ai, AI_STACK_COUNT) != 0)
		CHL_I32(ai, AI_STACK_COUNT) = CHL_I32(ai, AI_STACK_COUNT) - 1;
	for (i = 0; (int)i < chl_player_count; i++) {
		actor = CHL_PTR(ai, AI_ACTOR);
		if (i == CHL_U16(actor, ACT_PLAYER))
			continue;
		other = CHL_PTR(chl_ai_players[i], AI_ACTOR);
		dx = CHL_F32(actor, ACT_POS + 0x0) - CHL_F32(other, ACT_POS + 0x0);
		dy = CHL_F32(actor, ACT_POS + 0x4) - CHL_F32(other, ACT_POS + 0x4);
		dz = CHL_F32(actor, ACT_POS + 0x8) - CHL_F32(other, ACT_POS + 0x8);
		dxz2 = dx * dx + dz * dz;
		if (sqrtf(dy * dy + dxz2) <= CHL_F32(ACT_CD(other), 0x9c) + CHL_F32(ACT_CD(actor), 0x9c) + 10.0f &&
		    sqrtf(dxz2) < fabsf(dy)) {
			n = CHL_I32(ai, AI_STACK_COUNT) + 10;
			CHL_I32(ai, AI_STACK_COUNT) = n;
			if (0x32 < n) {
				if (CHL_I32(ai, AI_MODE) != 6) {
					AI_VCALL(ai, AI_VSLOT_SET_MODE, void (*)(void *, int))(ai, 6);
					CHL_I32(ai, AI_MODE_TIME) = ai_unstack_time;
					CHL_I32(ai, AI_FORCE_THINK) = 1;
				}
				CHL_PTR(ai, AI_TARGET) = CHL_FIELD(float, CHL_PTR(chl_ai_players[i], AI_ACTOR), ACT_POS);
			}
			break;
		}
	}
	AI_VCALL(ai, AI_VSLOT_PRE_ACT, void (*)(void *))(ai);
	AI_VCALL(ai, AI_VSLOT_ACT, void (*)(void *))(ai);
}

/* 0x000b1860  CharLib (file unknown)
vtable slot 3: clears the sensor hits (+0x14 of each 0xf0-byte entry), then AI_ThinkTail. */
void AI_Think(void *ai)
{
	void *sensors = CHL_PTR(ai, AI_SENSORS);
	int i;

	for (i = 0; i < CHL_I32(CHL_PTR(ai, AI_SENSORS), 0x8); i++) {
		sensors = CHL_PTR(ai, AI_SENSORS);
		CHL_I32(*(void **)sensors, i * 0xf0 + 0x14) = 0;
	}
	AI_ThinkTail(ai);
}
