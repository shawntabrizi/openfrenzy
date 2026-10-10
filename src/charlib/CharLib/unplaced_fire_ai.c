/*
CharLib/unplaced_fire_ai.c  (charlib)

The conditions and state handlers of the AI players of the "Fireworks" minigame (CarryOn,
CFire_*): their tables are the vtables at 0x00294a84 (conditions) and 0x00294ac0 (state
handlers), which the CarryOn player constructor installs. They lie in the CharLib block
in front of CCamera.cpp, so the module table gives them to charlib; by what they use
(the CarryOn game object 0x002f6d5c, its players 0x002f6d28, its state 0x00293b44) they
belong with the carryon modules (reported to the architect).

Conditions: int (*)(void *ai). State handlers: int (*)(void *ai, int entered).
*/

#include "charlib.h"
#include "charlib_actor.h"
#include "charlib_ai.h"

/* CarryOn globals (carryon .data / .bss) */
extern int DAT_00293b44;          /* the game state (3: ..., 4, 5: ...) */
extern void *DAT_002f6d28[4];     /* the CarryOn players */
extern void *DAT_002f6d5c;        /* the CarryOn game; +0x250 its phase */
extern void *DAT_00293afc;        /* the conveyor (carryon_b CFire_Conveyor) */
extern int DAT_00293f24;
extern int DAT_00293f28;
extern int DAT_00293f30;
extern int DAT_00293f34;
extern float DAT_00293f20;        /* attack cooldown (6.0) */
extern float DAT_00294a7c;        /* throw animation speed (5.0) */
extern float DAT_00294a80;        /* catch animation speed (1.5) */
/* core: 0x002baea4 paused, 0x002baebc team game, 0x002baef4 team of each player (0x28
bytes per player) */
extern int DAT_002baebc;
extern int DAT_002baef4[];

#define fire_game_phase CHL_I32(DAT_002f6d5c, 0x250)
/* 0x00292c98  jump speed (50) */
#define fire_jump_speed CHL_VAR(float, 0x00292c98)
/* 0x00293928  voice volume (200) */
#define fire_voice_volume CHL_VAR(int, 0x00293928)
/* 0x002f7a20  blend the walk animation in from the jump landing */
#define fire_blend_walk CHL_VAR(int, 0x002f7a20)
/* 0x002f7a24  attack lunge start time */
#define fire_lunge_start CHL_VAR(float, 0x002f7a24)
/* 0x002f2890 / 94  the highest height of player 0 and its base (Actor.c) */
#define fire_max_height CHL_VAR(float, 0x002f2890)
#define fire_height_base CHL_VAR(float, 0x002f2894)

/* 0x00294afc..  the walk animation blend (speeds 0x400 and 0x200, blend 0.02 / -0.02,
20 frames), run / walk speed thresholds (10, 8), lunge speed (10) and end (3), attack
blends and speeds (3 each), throw times (30, 20) and speed (5) */
#define fire_walk_speed1 CHL_VAR(int, 0x00294afc)
#define fire_walk_speed2 CHL_VAR(int, 0x00294b00)
#define fire_walk_blend_in CHL_VAR(float, 0x00294b04)
#define fire_walk_blend_out CHL_VAR(float, 0x00294b08)
#define fire_walk_blend_frames CHL_VAR(int, 0x00294b0c)
#define fire_run_speed CHL_VAR(float, 0x00294b10)
#define fire_walk_speed CHL_VAR(float, 0x00294b14)
#define fire_lunge_speed CHL_VAR(float, 0x00294b18)
#define fire_lunge_end CHL_VAR(float, 0x00294b1c)
#define fire_attack_blends CHL_ARR(float, 0x00294b20)
#define fire_attack_speeds CHL_ARR(float, 0x00294b30)
#define fire_throw_anim_time CHL_VAR(float, 0x00294b40)
#define fire_throw_move_time CHL_VAR(float, 0x00294b44)
#define fire_throw_speed CHL_VAR(float, 0x00294b48)

/* CarryOn methods (carryon_a / carryon_b) */
#define fire_GameSetPlayerState ((void (*)(void *, int, int))FUN_000c7880)
#define fire_PickUp ((int (*)(void *, int))FUN_000c2a80)
#define fire_Drop ((void (*)(void *, int, int))FUN_000c0d30)
#define fire_HoldState ((int (*)(void *, int))FUN_000c1d50)
#define fire_ConveyorFindNearest ((void *(*)(void *, float *, int))FUN_000ca4c0)
#define fire_ConveyorTake ((void (*)(void *, int, int))FUN_000c9e60)

/* bactor: baIsAnimating(inst, node, recurse, &anim), baQueueBlendedAnimation(inst, node,
anim, anim2, speed, speed2, time, time2, blend, flags, callback, context),
baChangeAnimation(node, recurse, speed, speed2, blend_speed, blend, mask) */
#define fire_IsAnimating ((int (*)(void *, void *, int, void **))FUN_00036870)
#define fire_QueueBlended \
	((void (*)(void *, void *, void *, void *, int, int, int, int, float, unsigned int, void *, void *))FUN_00035810)
#define fire_ChangeAnimation ((void (*)(void *, int, int, int, float, float, unsigned int))FUN_00035d70)

/* classlib_ui: CLSamples_PlayGeneric(index, volume, arg3, flags, pos); 0x0028fdd8 the
forward vector (0, 0, 1) */
#define fire_PlayGeneric ((int (*)(int, int, int, unsigned int, float *))FUN_00071720)
extern float DAT_0028fdd8[4];

#define FIRE_ACTOR(ai) CHL_PTR(ai, AI_ACTOR)

static void fire_stop(void *ai)
{
	CHL_F32(ai, AI_PUSH + 0x8) = 0.0f;
	CHL_F32(ai, AI_PUSH + 0x4) = 0.0f;
	CHL_F32(ai, AI_PUSH + 0x0) = 0.0f;
}

/* 0x000d4890  CharLib (file unknown; CarryOn AI) */
int FireAI_GameRunning(void *ai)
{
	(void)ai;
	return DAT_00293b44 < 4;
}

/* 0x000d48a0  CharLib (file unknown; CarryOn AI)
State: idle. Entering plays the idle-start animation; while idle and the actor's
animation is the base one, now and then (5 in 256) a fidget plays. */
int FireAI_StateIdle(void *ai, int entered)
{
	void *actor = FIRE_ACTOR(ai);
	void *anim = NULL;

	if (entered != 0) {
		Actor_PlayAnim(actor, 0x22, 0, 0, 1.0f, 0.2f, 1);
		Actor_PlayAnim(FIRE_ACTOR(ai), 0, 1, 1, 1.0f, 0.03f, 1);
	} else if ((fire_IsAnimating(ACT_INST(actor), CHL_PTR(actor, ACT_NODE), 0, &anim) == 0 ||
		    anim == CHL_PTR(FIRE_ACTOR(ai), ACT_ANIMS)) &&
		   (CHL_Rand() & 0xff) < 5 && DAT_00293b44 != 3) {
		Actor_PlayAnim(FIRE_ACTOR(ai), 0x21, 0, 0, 1.0f, 0.1f, 1);
		Actor_PlayAnim(FIRE_ACTOR(ai), 0, 1, 1, 1.0f, 0.03f, 1);
	}
	fire_stop(ai);
	return 0;
}

/* 0x000d4990  CharLib (file unknown; CarryOn AI) */
int FireAI_GameOver(void *ai)
{
	(void)ai;
	return DAT_00293b44 == 5;
}

/* 0x000d49a0  CharLib (file unknown; CarryOn AI)
State: game over. Entering plays the win or lose animation (by the hold state, and in
team games by the result), then its loop; the route is finished. */
int FireAI_StateGameOver(void *ai, int entered)
{
	int result;
	int loop;

	if (entered != 0) {
		result = fire_HoldState(ai, 0);
		if (DAT_002baebc == 0) {
			if (result == -1) {
				Actor_PlayAnim(FIRE_ACTOR(ai), 0x1c, 0, 0, 1.0f, 0.2f, 1);
				loop = 0x24;
			} else {
				Actor_PlayAnim(FIRE_ACTOR(ai), 0x1b, 0, 0, 1.0f, 0.2f, 1);
				loop = 0x23;
			}
		} else if (result == -1) {
			Actor_PlayAnim(FIRE_ACTOR(ai), 0x1c, 0, 0, 1.0f, 0.2f, 1);
			loop = 0x24;
		} else if (result != 1) {
			Actor_PlayAnim(FIRE_ACTOR(ai), 0, 1, 0, 1.0f, 0.1f, 1);
			goto done;
		} else {
			Actor_PlayAnim(FIRE_ACTOR(ai), 0x1b, 0, 0, 1.0f, 0.2f, 1);
			loop = 0x23;
		}
		Actor_PlayAnim(FIRE_ACTOR(ai), loop, 1, 1, 1.0f, 0.03f, 1);
	}
done:
	CHL_I16(ai, AI_ROUTE + 0xc) = 1;
	fire_stop(ai);
	return 0;
}

/* 0x000d4ae0  CharLib (file unknown; CarryOn AI)
Condition: the current action is over (state 1: past the attack cooldown; 9: the AI has
finished the hold (0x1120 < 0); 14: game state 4; others: the animation ended). */
int FireAI_ActionOver(void *ai)
{
	switch (CHL_I16(ai, AI_STATE)) {
	case 1:
		if (CHL_F32(ai, AI_STICK_AMOUNT) < CHL_F32(ai, 0x10e8))
			return 1;
		break;
	case 9:
		if (CHL_I16(FIRE_ACTOR(ai), ACT_ANIM_DONE) != 0 && CHL_I32(ai, 0x1120) < 0)
			return 1;
		break;
	case 0xe:
		if (DAT_00293b44 == 4)
			return 1;
		break;
	default:
		if (CHL_I16(FIRE_ACTOR(ai), ACT_ANIM_DONE) != 0)
			return 1;
	}
	return 0;
}

/* 0x000d4b50  CharLib (file unknown; CarryOn AI)
Condition: wants to walk (stick pushed past 0x10e8; in states other than 0, 3 and 8 only
when the animation is over); never in phase 3. */
int FireAI_WantsWalk(void *ai)
{
	short state;

	if (fire_game_phase == 3)
		return 0;
	state = CHL_I16(ai, AI_STATE);
	if (state == 0 || state == 3 || state == 8) {
		if (CHL_F32(ai, AI_STICK_AMOUNT) < CHL_F32(ai, 0x10e8))
			return 0;
	} else {
		if (CHL_F32(ai, AI_STICK_AMOUNT) < CHL_F32(ai, 0x10e8))
			return 0;
		if (CHL_I16(FIRE_ACTOR(ai), ACT_ANIM_DONE) == 0)
			return 0;
	}
	return 1;
}

/* 0x000d4bb0  CharLib (file unknown; CarryOn AI)
Condition: jump (button pressed, on the ground), not in phase 3. */
int FireAI_WantsJump(void *ai)
{
	if (fire_game_phase != 3 && CHL_I32(CHL_PTR(ai, AI_BUTTON), 0x14) != 0 &&
	    CHL_I32(FIRE_ACTOR(ai), 0x214) != 0)
		return 1;
	return 0;
}

/* 0x000d4bf0  CharLib (file unknown; CarryOn AI)
Condition: attack. With the attack button (0x6c), outside phase 3 (and in phase 2 only
with 0x1130), looks for another CarryOn player (not of the own team in team games, not in
state 9) within the height range 0x10f8, the angle 0x10f0 and the distance 0x10f4 (scaled
by the angle); the target goes to AI_ENEMY (-1: none) and the wanted heading turns to it.
Returns 1 whenever the button is pressed. */
int FireAI_WantsAttack(void *ai)
{
	void *actor;
	void *other;
	float heading;
	float angle;
	float dist;
	unsigned int i;

	if ((fire_game_phase == 2 && CHL_I32(ai, 0x1130) != 0) || fire_game_phase == 3 ||
	    CHL_I32(CHL_PTR(ai, 0x6c), 0x14) == 0)
		return 0;
	for (i = 0; (int)i < chl_player_count; i++) {
		actor = FIRE_ACTOR(ai);
		if (i == CHL_U16(actor, ACT_PLAYER))
			continue;
		if (DAT_002baebc != 0 && DAT_002baef4[i * 10] == DAT_002baef4[CHL_U16(actor, ACT_PLAYER) * 10])
			continue;
		if (CHL_I16(DAT_002f6d28[i], 0x36) == 9)
			continue;
		other = CHL_PTR(DAT_002f6d28[i], 0x30);
		if (!(fabsf(CHL_F32(actor, ACT_POS + 0x4) - CHL_F32(other, ACT_POS + 0x4)) < CHL_F32(ai, 0x10f8)))
			continue;
		heading = FUN_00070de0(CHL_FIELD(float, actor, ACT_POS), CHL_FIELD(float, other, ACT_POS));
		angle = FUN_00070670(heading, CHL_F32(FIRE_ACTOR(ai), ACT_HEADING));
		if (angle < CHL_F32(ai, 0x10f0)) {
			dist = FUN_00051500(CHL_FIELD(float, FIRE_ACTOR(ai), ACT_POS),
				CHL_FIELD(float, CHL_PTR(DAT_002f6d28[i], 0x30), ACT_POS));
			if ((angle + 1.0f) * dist < CHL_F32(ai, 0x10f4)) {
				CHL_I16(ai, AI_ENEMY) = (short)i;
				CHL_F32(FIRE_ACTOR(ai), ACT_HEADING_WANTED) = heading;
				return 1;
			}
		}
	}
	CHL_I16(ai, AI_ENEMY) = -1;
	return 1;
}

/* 0x000d4d70  CharLib (file unknown; CarryOn AI)
Condition: falling (in the air and falling faster than 0x10ec). */
int FireAI_Falling(void *ai)
{
	if (CHL_I32(FIRE_ACTOR(ai), 0x214) == 0 && CHL_F32(FIRE_ACTOR(ai), ACT_VEL + 0x4) <= CHL_F32(ai, 0x10ec))
		return 1;
	return 0;
}

/* 0x000d4da0  CharLib (file unknown; CarryOn AI)
Condition: in phase 3 with the button pressed and nothing on the conveyor near the
player. */
int FireAI_WantsTakeFromConveyor(void *ai)
{
	if (fire_game_phase == 3 && CHL_I32(CHL_PTR(ai, AI_BUTTON), 0x14) != 0)
		return *(int *)fire_ConveyorFindNearest(DAT_00293afc, CHL_FIELD(float, FIRE_ACTOR(ai), ACT_POS), 0) == 0;
	return 0;
}

/* 0x000d4df0  CharLib (file unknown; CarryOn AI)
Condition: in phase 3 holding something (0x1130) with the attack button pressed. */
int FireAI_WantsLoad(void *ai)
{
	if (fire_game_phase == 3 && CHL_I32(ai, 0x1130) != 0)
		return CHL_I32(CHL_PTR(ai, 0x6c), 0x14) != 0;
	return 0;
}

/* 0x000d4e20  CharLib (file unknown; CarryOn AI)
Condition that never holds. */
int FireAI_Never(void *ai)
{
	(void)ai;
	return 0;
}

/* 0x000d4e30  CharLib (file unknown; CarryOn AI)
State: stand. */
int FireAI_StateStand(void *ai, int entered)
{
	if (entered != 0)
		Actor_PlayAnim(FIRE_ACTOR(ai), 0, 1, 0, 1.0f, 0.1f, 1);
	fire_stop(ai);
	return 0;
}

/* 0x000d4e80  CharLib (file unknown; CarryOn AI)
State: walk / run. Entering starts the walk animation (blended in from a landing when
fire_blend_walk is set); the run and walk animations switch at the speed thresholds; the
animation speed follows the actor's speed; the push follows the stick. */
int FireAI_StateWalk(void *ai, int entered)
{
	void *actor;
	float f;

	if (entered != 0) {
		CHL_I32(ai, 0x10e4) = 1;
		if (fire_blend_walk == 0 || CHL_I16(ai, AI_PREV_STATE) != 3) {
			Actor_PlayAnim(FIRE_ACTOR(ai), 1, 1, 0, 0.0f, chl_time_scale * 0.1f, 1);
		} else {
			actor = FIRE_ACTOR(ai);
			chl_FlushAnimations(ACT_INST(actor), CHL_PTR(actor, ACT_NODE), 1, 0);
			CHL_U16(FIRE_ACTOR(ai), ACT_ANIM_QUEUED) = 0;
			actor = FIRE_ACTOR(ai);
			fire_QueueBlended(ACT_INST(actor), CHL_PTR(actor, ACT_NODE), CHL_PTR(actor, ACT_ANIMS + 0x18 * 4),
				CHL_PTR(actor, ACT_ANIMS + 5 * 4), fire_walk_speed1, fire_walk_speed2, -1, -1, fire_walk_blend_in, 3,
				NULL, NULL);
		}
	}
	if (fire_blend_walk != 0 && CHL_I16(ai, AI_PREV_STATE) == 3 && CHL_I32(ai, AI_STATE_FRAMES) == fire_walk_blend_frames)
		fire_ChangeAnimation(CHL_PTR(FIRE_ACTOR(ai), ACT_NODE), 1, fire_walk_speed1, fire_walk_speed2,
			fire_walk_blend_out, 0.0f, 7);
	if (CHL_I32(ai, 0x10e4) == 0 || !(fire_run_speed < CHL_F32(FIRE_ACTOR(ai), 0x254))) {
		if (CHL_I32(ai, 0x10e4) == 0 && CHL_F32(FIRE_ACTOR(ai), 0x254) < fire_walk_speed) {
			Actor_SwitchAnim(FIRE_ACTOR(ai), 1, 0.05f);
			CHL_I32(ai, 0x10e4) = 1;
		}
	} else {
		Actor_SwitchAnim(FIRE_ACTOR(ai), 0x18, 0.05f);
		CHL_I32(ai, 0x10e4) = 0;
	}
	actor = FIRE_ACTOR(ai);
	if (CHL_I32(ai, 0x10e4) == 0)
		f = CHL_F32(CHL_PTR(actor, ACT_ANIM_INFO), 0x6a0);
	else
		f = CHL_F32(CHL_PTR(actor, ACT_ANIM_INFO), 0x84);
	CHL_F32(actor, ACT_ANIM_RATE) = (f * CHL_F32(actor, 0x254)) / chl_time_scale;
	f = CHL_F32(ai, AI_MAX_SPEED_SCALE) * CHL_F32(ai, AI_STICK_AMOUNT);
	CHL_F32(ai, AI_PUSH + 0x0) = f * CHL_F32(ai, AI_MOVE_DIR + 0x0);
	CHL_F32(ai, AI_PUSH + 0x4) = f * CHL_F32(ai, AI_MOVE_DIR + 0x4);
	CHL_F32(ai, AI_PUSH + 0x8) = f * CHL_F32(ai, AI_MOVE_DIR + 0x8);
	CHL_F32(FIRE_ACTOR(ai), ACT_HEADING_WANTED) = FUN_000706f0(CHL_FIELD(float, ai, AI_PUSH));
	return 0;
}

/* 0x000d5060  CharLib (file unknown; CarryOn AI)
State: jump. Entering sets the jump speed (plus the platform's), plays the jump animation
and voice and resets the height record of player 0; the push follows the stick. */
int FireAI_StateJump(void *ai, int entered)
{
	void *actor;
	void *platform;
	float f;

	if (entered != 0) {
		CHL_F32(FIRE_ACTOR(ai), ACT_VEL + 0x4) = fire_jump_speed;
		actor = FIRE_ACTOR(ai);
		platform = CHL_PTR(ACT_CD(actor), CD_PLATFORM);
		if (platform != NULL) {
			CHL_F32(actor, ACT_VEL + 0x0) = CHL_F32(platform, 0x1a4) + CHL_F32(actor, ACT_VEL + 0x0);
			actor = FIRE_ACTOR(ai);
			CHL_F32(actor, ACT_VEL + 0x4) = CHL_F32(CHL_PTR(ACT_CD(actor), CD_PLATFORM), 0x1a8) +
				CHL_F32(actor, ACT_VEL + 0x4);
			actor = FIRE_ACTOR(ai);
			CHL_F32(actor, ACT_VEL + 0x8) = CHL_F32(CHL_PTR(ACT_CD(actor), CD_PLATFORM), 0x1ac) +
				CHL_F32(actor, ACT_VEL + 0x8);
		}
		Actor_PlayAnim(FIRE_ACTOR(ai), 2, 0, 0, 1.0f, 0.3f, 1);
		if (CHL_F32(ai, 0x10e8) <= CHL_F32(ai, AI_STICK_AMOUNT))
			CHL_F32(FIRE_ACTOR(ai), ACT_HEADING_WANTED) = FUN_000706f0(CHL_FIELD(float, ai, AI_PUSH));
		if (CHL_I16(FIRE_ACTOR(ai), ACT_PLAYER) == 0) {
			fire_height_base = CHL_F32(FIRE_ACTOR(ai), ACT_POS + 0x4);
			fire_max_height = 0.0f;
		}
		AI_PlayVoice(ai, 0, fire_voice_volume, 0.0f, 0, -1, 0);
	}
	f = CHL_F32(ai, AI_MAX_SPEED_SCALE) * CHL_F32(ai, AI_STICK_AMOUNT);
	CHL_F32(ai, AI_PUSH + 0x0) = f * CHL_F32(ai, AI_MOVE_DIR + 0x0);
	CHL_F32(ai, AI_PUSH + 0x4) = f * CHL_F32(ai, AI_MOVE_DIR + 0x4);
	CHL_F32(ai, AI_PUSH + 0x8) = f * CHL_F32(ai, AI_MOVE_DIR + 0x8);
	return 0;
}

/* 0x000d5190  CharLib (file unknown; CarryOn AI)
State: fall. */
int FireAI_StateFall(void *ai, int entered)
{
	if (entered != 0) {
		Actor_PlayAnim(FIRE_ACTOR(ai), 5, 0, 0, 1.0f, 0.3f, 1);
		AI_PlayVoice(ai, 1, fire_voice_volume, 0.0f, 0, -1, 0);
	}
	fire_stop(ai);
	return 0;
}

/* 0x000d5200  CharLib (file unknown; CarryOn AI)
State: attack. Entering plays one of three attacks (sound, animation and its speed and
blend) and starts the cooldown; between fire_lunge_start and fire_lunge_end the player
lunges forward; it faces its target; between 0x1104 and 0x1108 frames (or past 0x1104
before the hit flag 0x1092) virtual slot 4 does the hit. */
int FireAI_StateAttack(void *ai, int entered)
{
	unsigned int kind;
	int r;
	float *push;

	if (entered != 0) {
		kind = (unsigned int)(CHL_Rand() % 3);
		r = CHL_Rand();
		fire_PlayGeneric(kind == 0 ? (r & 1) + 0xf : (r & 1) + 0x11, 0x7f, 0, 0, NULL);
		Actor_PlayAnim(FIRE_ACTOR(ai), kind + 0xd, 0, 0, fire_attack_speeds[kind], fire_attack_blends[kind], 1);
		CHL_I16(ai, 0x1092) = 0;
		if (CHL_F32(ai, 0x1138) < DAT_00293f20)
			CHL_F32(ai, 0x1138) = DAT_00293f20;
	}
	if (!(CHL_F32(ai, AI_STATE_TIME) <= fire_lunge_end) || CHL_F32(ai, AI_STATE_TIME) < fire_lunge_start) {
		fire_stop(ai);
	} else {
		push = CHL_FIELD(float, ai, AI_PUSH);
		FUN_00051170(push, DAT_0028fdd8, CHL_F32(FIRE_ACTOR(ai), ACT_HEADING_WANTED));
		push[0] = fire_lunge_speed * push[0];
		push[1] = fire_lunge_speed * push[1];
		push[2] = fire_lunge_speed * push[2];
	}
	if (CHL_I16(ai, AI_ENEMY) != -1)
		CHL_F32(FIRE_ACTOR(ai), ACT_HEADING_WANTED) = FUN_00070de0(CHL_FIELD(float, FIRE_ACTOR(ai), ACT_POS),
			CHL_FIELD(float, CHL_PTR(DAT_002f6d28[CHL_I16(ai, AI_ENEMY)], 0x30), ACT_POS));
	if ((float)CHL_I32(ai, 0x1104) <= CHL_F32(ai, AI_STATE_TIME) &&
	    (CHL_F32(ai, AI_STATE_TIME) <= (float)CHL_I32(ai, 0x1108) || CHL_I16(ai, 0x1092) == 0))
		AI_VCALL(ai, 4, void (*)(void *))(ai);
	return 0;
}

/* 0x000d53d0  CharLib (file unknown; CarryOn AI)
State: second attack (animation 8): moves with the stick; between 0x110c and 0x1110
frames (or past 0x110c before the hit flag) virtual slot 4 does the hit. */
int FireAI_StateAttack2(void *ai, int entered)
{
	float f;

	if (entered != 0) {
		Actor_PlayAnim(FIRE_ACTOR(ai), 8, 0, 0, 1.0f, 0.1f, 1);
		CHL_I16(ai, 0x1092) = 0;
	}
	f = CHL_F32(ai, AI_MAX_SPEED_SCALE) * CHL_F32(ai, AI_STICK_AMOUNT);
	CHL_F32(ai, AI_PUSH + 0x0) = f * CHL_F32(ai, AI_MOVE_DIR + 0x0);
	CHL_F32(ai, AI_PUSH + 0x4) = f * CHL_F32(ai, AI_MOVE_DIR + 0x4);
	CHL_F32(ai, AI_PUSH + 0x8) = f * CHL_F32(ai, AI_MOVE_DIR + 0x8);
	if ((float)CHL_I32(ai, 0x110c) <= CHL_F32(ai, AI_STATE_TIME) &&
	    (CHL_F32(ai, AI_STATE_TIME) <= (float)CHL_I32(ai, 0x1110) || CHL_I16(ai, 0x1092) == 0))
		AI_VCALL(ai, 4, void (*)(void *))(ai);
	return 0;
}

/* 0x000d5460  CharLib (file unknown; CarryOn AI)
State: throw. Entering tells the game (state 3), picks up or drops the held firework by
the game phase, plays the voice and the throw animation and resets the mode. */
int FireAI_StateThrow(void *ai, int entered)
{
	int phase;
	int flag;

	if (entered == 0)
		goto done;
	fire_GameSetPlayerState(DAT_002f6d5c, CHL_U16(FIRE_ACTOR(ai), ACT_PLAYER), 3);
	phase = fire_game_phase;
	if (phase == 0) {
		flag = DAT_00293f34;
		if (DAT_00293f30 != 0)
			goto pick_up;
		goto check;
	}
	if (phase != 1) {
		if (phase != 2)
			goto drop;
		flag = DAT_00293f28;
		if (DAT_00293f24 == 0)
			goto check;
	}
pick_up:
	fire_PickUp(ai, 0);
	goto voice;
check:
	if (flag != 0)
		goto voice;
drop:
	fire_Drop(ai, 0, -1);
voice:
	AI_PlayVoice(ai, 2, fire_voice_volume, 0.0f, 0, -1, 0);
	Actor_PlayAnim(FIRE_ACTOR(ai), 0x11, 0, 0, DAT_00294a7c, 0.1f, 1);
	AI_VCALL(ai, AI_VSLOT_SET_MODE, void (*)(void *, int))(ai, -1);
done:
	fire_stop(ai);
	return 0;
}

/* 0x000d5540  CharLib (file unknown; CarryOn AI)
State: catch. Entering tells the game (state 4), picks up in phases 0 / 2 when allowed,
plays the voice and the catch animation (0x1120 = 1: still holding). Until
fire_throw_move_time the player is pushed backwards (forwards once 0x1120 is 0); at
fire_throw_anim_time the end animation (0x15 / 0x16) plays once (0x1120 = -1). */
int FireAI_StateCatch(void *ai, int entered)
{
	float *push;
	float s;

	if (entered != 0) {
		fire_GameSetPlayerState(DAT_002f6d5c, CHL_U16(FIRE_ACTOR(ai), ACT_PLAYER), 4);
		if ((fire_game_phase == 2 && DAT_00293f28 != 0) || (fire_game_phase == 0 && DAT_00293f34 != 0))
			fire_PickUp(ai, 1);
		if (fire_game_phase == 2)
			(void)(CHL_Rand() % 100); /* chooses between two equal calls in the original */
		AI_PlayVoice(ai, 3, fire_voice_volume, 0.0f, 0, -1, 0);
		Actor_PlayAnim(FIRE_ACTOR(ai), 0x13, 0, 0, DAT_00294a80, 0.1f, 1);
		CHL_I32(ai, 0x1120) = 1;
		AI_VCALL(ai, AI_VSLOT_SET_MODE, void (*)(void *, int))(ai, -1);
	}
	if (fire_throw_move_time <= CHL_F32(ai, AI_STATE_TIME)) {
		fire_stop(ai);
	} else {
		push = CHL_FIELD(float, ai, AI_PUSH);
		FUN_00051170(push, DAT_0028fdd8, CHL_F32(FIRE_ACTOR(ai), ACT_HEADING_WANTED));
		s = CHL_I32(ai, 0x1120) == 0 ? fire_throw_speed : -fire_throw_speed;
		push[0] = s * push[0];
		push[1] = s * push[1];
		push[2] = s * push[2];
	}
	if (-1 < CHL_I32(ai, 0x1120) && fire_throw_anim_time <= CHL_F32(ai, AI_STATE_TIME)) {
		Actor_PlayAnim(FIRE_ACTOR(ai), CHL_I32(ai, 0x1120) == 0 ? 0x16 : 0x15, 0, 0, 1.0f, 0.1f, 1);
		CHL_I32(ai, 0x1120) = -1;
	}
	return 0;
}

/* 0x000d5720  CharLib (file unknown; CarryOn AI)
State: hit (animation 7). */
int FireAI_StateHit(void *ai, int entered)
{
	if (entered != 0)
		Actor_PlayAnim(FIRE_ACTOR(ai), 7, 0, 0, 1.0f, 0.1f, 1);
	fire_stop(ai);
	return 0;
}

/* 0x000d5770  CharLib (file unknown; CarryOn AI)
State: take a firework from the conveyor (row 0). */
int FireAI_StateTake(void *ai, int entered)
{
	if (entered != 0) {
		Actor_PlayAnim(FIRE_ACTOR(ai), 6, 0, 0, 1.0f, 0.1f, 1);
		CHL_F32(FIRE_ACTOR(ai), ACT_HEADING_WANTED) = 0.0f;
		fire_ConveyorTake(fire_ConveyorFindNearest(DAT_00293afc, CHL_FIELD(float, FIRE_ACTOR(ai), ACT_POS), 0),
			CHL_U16(FIRE_ACTOR(ai), ACT_PLAYER), 0);
	}
	fire_stop(ai);
	return 0;
}

/* 0x000d57f0  CharLib (file unknown; CarryOn AI)
State: load a firework onto the launcher (row 1); one fewer held (0x1130). */
int FireAI_StateLoad(void *ai, int entered)
{
	if (entered != 0) {
		Actor_PlayAnim(FIRE_ACTOR(ai), 6, 0, 0, 1.0f, 0.1f, 1);
		CHL_F32(FIRE_ACTOR(ai), ACT_HEADING_WANTED) = 0.0f;
		fire_ConveyorTake(fire_ConveyorFindNearest(DAT_00293afc, CHL_FIELD(float, FIRE_ACTOR(ai), ACT_POS), 1),
			CHL_U16(FIRE_ACTOR(ai), ACT_PLAYER), 1);
		CHL_I32(ai, 0x1130) = CHL_I32(ai, 0x1130) - 1;
	}
	fire_stop(ai);
	return 0;
}

/* 0x000d5870  CharLib (file unknown; CarryOn AI)
State: drop (animation 6, the held firework is dropped with 3). */
int FireAI_StateDrop(void *ai, int entered)
{
	if (entered != 0) {
		Actor_PlayAnim(FIRE_ACTOR(ai), 6, 0, 0, 1.0f, 0.1f, 1);
		fire_Drop(ai, 3, -1);
	}
	fire_stop(ai);
	return 0;
}
