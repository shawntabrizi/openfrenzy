/*
Demolition/CDemolition_Player.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Player.cpp

CDemolition_Player (0x13e0 bytes, vtable 0x0026b448, message receiver vtable 0x0026b440 at
+0x1130): a Demolition player, a CharLib AI character (CDemolition_CharLib, charlib_ai.h).
The functions from 0x000f9740 are tagged with this file; 0x000f92b0-0x000f94c0 are the gap
functions before them (constructor, destructor, actor and input), "file inferred".

Its CharLib AI tables (0x00298820 states, 0x002987d8 conditions, 0x00298690 priorities) go
through demolition_d's thunks to the virtual slots: the conditions 0..17 are slots
0x4c..0x90, the states 0..17 slots 0x94..0xd8 (`int state(self, entered)`).

  condition 0 (0x4c) 0x000f9e40 Stand       state 0  (0x94) 0x000faec0 Stand
  condition 2 (0x54) 0x000f9ee0 Walk        state 1  (0x98) 0x000fafe0 Idle
  condition 3 (0x58) 0x000f9f70 Jump        state 2  (0x9c) 0x000fb080 Walk
  condition 4 (0x5c) 0x000f9fb0 Land        state 3  (0xa0) 0x000fb270 Jump
  condition 5 (0x60) 0x000fac10 Attack      state 4  (0xa4) 0x000fa070 Land
  condition 6 (0x64) 0x000f9fe0 Action      state 5  (0xa8) 0x000fb3e0 Attack
  condition 9 (0x70) 0x000fa010 Fall        state 6  (0xac) 0x000fb630 Jump attack
                                            state 9  (0xb8) 0x000fb820 Fall
                                            state 10 (0xbc) 0x000fa0e0 Hurt
                                            state 11 (0xc0) 0x000fa160 Taunt
                                            state 13 (0xc8) 0x000fb890 Knockout
                                            state 14 (0xcc) 0x000fb9c0 Blown away
                                            state 15 (0xd0) 0x000fa1e0 Get up
                                            state 16 (0xd4) 0x000fca70 Throw
                                            state 17 (0xd8) 0x000fa290 Victory
  (The names follow meta_a's CharLib player (Meta/unplaced_player.c) and the animations
  played; states 7, 8 and 12 are 0x000f8ab0. Actor +0x234: the animation ended.)
  other slots: +0x10 0x000fbc40, +0x44 0x000fc0b0 SetActive, +0x48 deleting destructor,
  +0xdc 0x000fa530 Init(index), +0xe0 0x000fa6a0 Shutdown, +0xe4 0x000fc7e0 Update(dt),
  +0xe8 0x000fa720 Draw, +0xec 0x000fc5d0 DrawFx, +0xf0 0x000f9740 Reset

Player fields (besides charlib_ai.h's):
  +0x1140 next player   +0x1144 position   +0x1154 previous position   +0x1164 radius (50)
  +0x1168 height (177)  +0x116c in play   +0x116d active (the AI runs)   +0x116e visible
  +0x116f shown by the results camera   +0x1171 CPU player   +0x1174 under control
  +0x1178, +0x117c, +0x1180 colour   +0x1184 index   +0x1188 character
  +0x118c the variation's AI brain (demolition_d DemoAI0..3)   +0x1190 phase
  +0x1194 score   +0x1198 hit cooldown   +0x11a0, +0x11a4 the player's bombs
  +0x11a8 the frag bomb held   +0x11ac charging a throw   +0x11c8 throw power
  +0x11d0 the throw timer (CDemoCountdown)   +0x120c throw now   +0x1210 the charge held
  +0x1214 start position   +0x1224 respawn point position   +0x1234 team mate
  +0x1238 nodes "guts", "lpalm", "rpalm"
  +0x1244 holding (animation +0x1248 on node +0x124c)
  +0x1250 the controller is shown, +0x1251 its animation runs (animation +0x1258 on node
  +0x125c), +0x1254 the controller instance, +0x1260 the node it is held at
  +0x1270 the controller (BActorInstance)
*/

#include "demolition_b.h"
#include "demolition_b_internal.h"

#include <stdint.h>

#define DB_PLAYER_FILE "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Player.cpp"

/* 0x002f289c  the players' CharLib AI objects */
extern int DAT_002f289c[];
/* 0x002baebc  team mode (core) */
extern int DAT_002baebc;
/* 0x00293928  the AI voice volume (charlib) */
extern int DAT_00293928;
/* 0x002b05a4, 0x002b05a8  the hurt and the knockout animation speeds (charlib) */
extern float DAT_002b05a4;
extern float DAT_002b05a8;
/* 0x00290738  the characters (classlib_ui, 0x60 bytes each): +0x00 the damage that knocks
   out, +0x18 the AI think interval (0x00290750) */
extern char DAT_00290738[];
/* 0x002ed324  the rumble manager (classlib_ui) */
extern unsigned char DAT_002ed324[];
/* 0x00297520  the variation settings (demolition_a, 15 ints each); +0x04: models drawn */
extern int DAT_00297520[];

/* 0x00297b20  the player's animations ({ name[0x40], speed }); the actor's table (+0x324).
   CDemolition_Player_CreateActor looks up 47 like every CharLib character; the original
   reads past these 43 entries into the following .data, where no name matches a
   sub-object. The port pads the table with empty names (no match), as for the remote
   bomb (CDemolition_Bomb.c). */
struct DbAnimName DAT_00297b20[47] = {
	{ "idle_ready", 0.3f },
	{ "walk", 0.16f },
	{ "jump", 1.0f },
	{ "attack_weapon_ready", 1.0f },
	{ "attack_weapon", 1.0f },
	{ "land", 1.8f },
	{ "land", 1.0f },
	{ "attack_1", 0.0f },
	{ "attack_kick", 0.0f },
	{ "hurt", 0.0f },
	{ "fall", 1.0f },
	{ "idle", 0.0f },
	{ "score_goal", 1.0f },
	{ "attack1", 1.0f },
	{ "attack2", 1.0f },
	{ "attack_knockout", 1.0f },
	{ "attack_flying_kick", 1.0f },
	{ "front_hurt", 1.0f },
	{ "back_hurt", 1.0f },
	{ "front_knockout", 1.0f },
	{ "back_knockout", 1.0f },
	{ "front_knockout_getup", 1.0f },
	{ "back_knockout_getup", 1.0f },
	{ "camera_pose", 1.0f },
	{ "run", 0.1f },
	{ "idle_additional1", 0.0f },
	{ "idle_additional2", 0.0f },
	{ "victory", 1.0f },
	{ "defeat", 1.0f },
	{ "select", 1.0f },
	{ "deselect", 1.0f },
	{ "drown", 1.0f },
	{ "idle_charselect", 1.0f },
	{ "psyched", 1.0f },
	{ "intro", 1.0f },
	{ "victory_idle", 1.0f },
	{ "defeat_idle", 1.0f },
	{ "head_idle_1", 1.0f },
	{ "head_idle_2", 1.0f },
	{ "idle_breathe_hold", 1.0f },
	{ "throw", 1.6f },
	{ "throw_release", 2.0f },
	{ "controller", 1.0f },
};

/* 0x00297ad8  the characters' colours (r, g, b; halved for the player) */
const unsigned int DAT_00297ad8[6][3] = {
	{ 0x2e, 0xa7, 0x24 }, { 0xd2, 0x00, 0x00 }, { 0x20, 0x71, 0xcd },
	{ 0xf8, 0xce, 0x00 }, { 0xf2, 0x82, 0x1e }, { 0xec, 0x6b, 0xae },
};

/* 0x00298690  the AI priorities [state][condition] (18 x 18) */
signed char DAT_00298690[18 * 18] = {
	0, 0, 1, 2, 0, 5, 0, 6, 6, 8, 0, 0, 9, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 0, 5, 0, 6, 6, 8, 0, 0, 9, 0, 0, 0, 0, 0,
	1, 0, 0, 2, 0, 5, 0, 6, 6, 8, 0, 0, 9, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 9, 0, 8, 0, 0, 7, 0, 0, 9, 0, 0, 0, 0, 0,
	1, 0, 2, 3, 0, 6, 0, 7, 7, 8, 0, 0, 9, 0, 0, 0, 0, 0,
	1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 2, 3, 0, 4, 0, 5, 5, 9, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* 0x002987d8  the AI conditions: demolition_d's thunks to the virtual slots 0x4c..0x90 */
void *const DAT_002987d8[18] = {
	(void *)FUN_0010d220, (void *)FUN_0010d230, (void *)FUN_0010d240, (void *)FUN_0010d250,
	(void *)FUN_0010d260, (void *)FUN_0010d270, (void *)FUN_0010d280, (void *)FUN_0010d290,
	(void *)FUN_0010d2a0, (void *)FUN_0010d2b0, (void *)FUN_0010d2c0, (void *)FUN_0010d2d0,
	(void *)FUN_0010d2e0, (void *)FUN_0010d2f0, (void *)FUN_0010d300, (void *)FUN_0010d310,
	(void *)FUN_0010d320, (void *)FUN_0010d330,
};

/* 0x00298820  the AI states: demolition_d's thunks to the virtual slots 0x94..0xd8 */
void *const DAT_00298820[18] = {
	(void *)FUN_0010d340, (void *)FUN_0010d360, (void *)FUN_0010d380, (void *)FUN_0010d3a0,
	(void *)FUN_0010d3c0, (void *)FUN_0010d3e0, (void *)FUN_0010d400, (void *)FUN_0010d420,
	(void *)FUN_0010d440, (void *)FUN_0010d460, (void *)FUN_0010d480, (void *)FUN_0010d4a0,
	(void *)FUN_0010d4c0, (void *)FUN_0010d4e0, (void *)FUN_0010d500, (void *)FUN_0010d520,
	(void *)FUN_0010d540, (void *)FUN_0010d560,
};

/* 0x00298868  the nodes the player holds things at */
const char *const DAT_00298868[3] = { "guts", "lpalm", "rpalm" };

/* 0x002988d4  (copied to +0x10d8 when hit) */
float DAT_002988d4 = 50.0f;

/* the end of most AI states: no push velocity, stay */
static int db_state_end(char *s)
{
	DB_F32(s, 0x84) = 0.0f;
	DB_F32(s, 0x80) = 0.0f;
	DB_F32(s, 0x7c) = 0.0f;
	return 0;
}

/* the start of most AI states: the actor's flag 0x1000 off */
static void db_state_begin(char *s)
{
	DB_U32(DB_PTR(s, 0x30), 0x220) &= 0xffffefff;
}

/* blends the animation of `node` back to the actor's root animation (actor +0x144) */
static void db_release_node(char *s, void *node)
{
	char *actor = (char *)DB_PTR(s, 0x30);

	baQueueCopyAnimation((struct BActorInstance *)(actor + 0x10), (struct BActorNode *)node,
		(struct BActorNode *)DB_PTR(actor, 0x144), 0.1f, 0, NULL, NULL);
}

/* 0x000f92b0  (file inferred) constructor */
struct CDemolition_Player *CDemolition_Player_Construct(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	CDemolition_CharLib_Construct((struct CDemolition_CharLib *)s);
	DB_PTR(s, 0) = (void *)vtable_0026b448;
	DB_PTR(s, 0x1130) = (void *)vtable_0026b440;
	CDemoCountdown_Ctor((unsigned int *)(s + 0x11d0));
	return self;
}

/* 0x000f9310  (file inferred) destructor */
void CDemolition_Player_Destruct(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026b448;
	DB_PTR(s, 0x1130) = (void *)vtable_0026b440;
	CDemoCountdown_Dtor((unsigned int *)(s + 0x11d0));
	CDemolition_CharLib_Destruct((struct CDemolition_CharLib *)s);
}

/* 0x000f9370  (file inferred) creates the player's CharLib actor (the character model
   game +0x80 + 4 * player), its AI tables and animations, and its think interval */
int CDemolition_Player_CreateActor(struct CDemolition_Player *self, int player)
{
	char *s = (char *)self;
	float pos[4];
	char *actor;
	int i;

	DB_I32(s, 0x119c) = 0;
	pos[0] = 0.0f;
	pos[1] = 0.0f;
	pos[2] = 0.0f;
	pos[3] = 1.0f;
	actor = (char *)Actor_Create(DB_PTR(DAT_002ff85c, 0x80 + player * 4), pos, NULL, 1, 0, 1);
	DB_PTR(s, 0x30) = actor;
	DB_I32(actor, 0x220) = 0x59;
	DB_U16(DB_PTR(s, 0x30), 0x230) = (unsigned short)player;
	DB_U16(s, 0x98) = 1;
	DB_U16(s, 0x34) = 0x12;
	DB_U16(s, 0x36) = 0;
	DB_I32(s, 4) = 0;
	DB_I32(s, 8) = 0;
	DB_PTR(s, 0x1094) = (void *)DAT_00298820;
	DB_PTR(s, 0x1098) = (void *)DAT_002987d8;
	DB_PTR(s, 0x109c) = DAT_00298690;
	Actor_InitCharData(DB_PTR(s, 0x30));
	DB_PTR(DB_PTR(s, 0x30), 0x324) = DAT_00297b20;
	for (i = 0; i < 47; i++)
	{
		actor = (char *)DB_PTR(s, 0x30);
		DB_PTR(actor, 0x264 + i * 4) = baFindSubObject((struct BActorInstance *)(actor + 0x10),
			(const char *)DB_PTR(actor, 0x324) + i * 0x44);
		if (DB_PTR(DB_PTR(s, 0x30), 0x264 + i * 4) == NULL)
			bkPrintf("ERROR: Failed to find anim segment :%s\n",
				(const char *)DB_PTR(DB_PTR(s, 0x30), 0x324) + i * 0x44);
	}
	/* think interval: the character's base value divided by the difficulty + 1 */
	DB_I32(s, 0x10b4) = *(int *)((char *)&DAT_00290750 + DAT_002baed4[player * 10] * 0x60) /
		(DAT_002bae70 + 1);
	return 1;
}

/* 0x000f94c0  (file inferred) creates the input map (stick, action1, action2) and binds it
   to `player`'s pad (record +0xc: the port bits); action2 is a held button in variations 1
   and 2 */
int CDemolition_Player_CreateInput(struct CDemolition_Player *self, int player)
{
	char *s = (char *)self;
	unsigned int port = (unsigned int)DAT_002baed4[player * 10 + 3];
	int v;

	DB_PTR(s, 0x5c) = biCreateMap(4);
	DB_PTR(s, 0x64) = biCreateChannel(DB_PTR(s, 0x5c), "left/right", 3, 0, 0, 0, 0);
	DB_PTR(s, 0x60) = biCreateChannel(DB_PTR(s, 0x5c), "up/down", 3, 0, 0, 0, 0);
	DB_PTR(s, 0x68) = biCreateChannel(DB_PTR(s, 0x5c), "action1", 1, 1, 0, 0, 0);
	v = DB_I32(DAT_002ff85c, 0x28);
	DB_PTR(s, 0x6c) = biCreateChannel(DB_PTR(s, 0x5c), "action2", 1, (v != 0 && v != 3) ? 1 : 0,
		0, 0, 0);
	biBindInput(DB_PTR(s, 0x60), port | 0xbe, port | 0xbd);
	biBindInput(DB_PTR(s, 0x64), port | 0xc0, port | 0xbf);
	biBindInput(DB_PTR(s, 0x60), port | 0xba, 0);
	biBindInput(DB_PTR(s, 0x64), port | 0xb9, 0);
	biBindInput(DB_PTR(s, 0x68), port | 0xc5, 0);
	v = DB_I32(DAT_002ff85c, 0x28);
	if (v == 1)
	{
		biBindInput(DB_PTR(s, 0x6c), port | 0xcc, 0);
		biBindInput(DB_PTR(s, 0x6c), port | 0xc5, 0);
		biBindInput(DB_PTR(s, 0x6c), port | 0xc6, 0);
		biBindInput(DB_PTR(s, 0x6c), port | 0xc7, 0);
	}
	else if (v == 3)
	{
		biBindInput(DB_PTR(s, 0x6c), port | 0xc5, 0);
		biBindInput(DB_PTR(s, 0x6c), port | 0xc6, 0);
		biBindInput(DB_PTR(s, 0x6c), port | 0xc7, 0);
	}
	else
	{
		/* variation 0 and the others bind the same two */
		biBindInput(DB_PTR(s, 0x6c), port | 0xc7, 0);
		biBindInput(DB_PTR(s, 0x6c), port | 0xc6, 0);
	}
	return 1;
}

/* 0x000f9740  Reset (virtual +0xf0): in play, visible, nothing held, no score */
void CDemolition_Player_Reset(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	DB_U8(s, 0x116c) = 1;
	DB_U8(s, 0x116e) = 1;
	DB_U8(s, 0x116f) = 1;
	DB_U8(s, 0x116d) = 0;
	DB_U8(s, 0x1171) = 0;
	DB_U8(s, 0x1173) = 0;
	DB_U8(s, 0x1172) = 0;
	DB_PTR(s, 0x11a0) = NULL;
	DB_PTR(s, 0x11a4) = NULL;
	DB_PTR(s, 0x11a8) = NULL;
	DB_I32(s, 0x1190) = 0;
	DB_I32(s, 0x1208) = 0;
	DB_I32(s, 0x1194) = 0;
	DB_I32(s, 0x1198) = 0;
	DB_U8(s, 0x11ac) = 0;
	DB_U8(s, 0x120c) = 0;
	DB_I32(s, 0x1210) = 0;
	DB_PTR(s, 0x1234) = NULL;
	DB_U8(s, 0x1244) = 0;
	DB_PTR(s, 0x124c) = NULL;
	DB_PTR(s, 0x1248) = NULL;
	DB_U8(s, 0x1250) = 0;
	DB_U8(s, 0x1251) = 0;
	DB_I32(s, 0x1258) = 0;
	DB_PTR(s, 0x1254) = NULL;
	DB_PTR(s, 0x125c) = NULL;
	DB_PTR(s, 0x1260) = NULL;
}

/* 0x000f97f0  in team mode: the team mate is the first other player of the same team
   (record +0x20) */
void CDemolition_Player_FindTeamMate(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	char *p;
	int me;
	int i;

	if (DAT_002baebc == 0)
	{
		DB_PTR(s, 0x1234) = NULL;
		return;
	}
	me = DB_I32(s, 0x1184);
	i = 0;
	for (p = DB_GAME_PLAYERS(); p != NULL; p = DB_PLAYER_NEXT(p), i++)
	{
		if (DB_I32(p, 0x1184) != me && DAT_002baed4[me * 10 + 8] == DAT_002baed4[i * 10 + 8])
		{
			DB_PTR(s, 0x1234) = p;
			bkPrintf("TEAMMODE: FoundTeamMate %d for player %d\n", i, me);
			return;
		}
	}
}

/* 0x000f9870  (re)starts the holding animations (`dt` is not used) */
void CDemolition_Player_PlayHoldAnims(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;
	char *actor;

	(void)dt;
	if (DB_U8(s, 0x1244) != 0)
	{
		actor = (char *)DB_PTR(s, 0x30);
		baFlushAnimations(actor + 0x10, (struct BActorNode *)DB_PTR(s, 0x124c), 1, 0);
		actor = (char *)DB_PTR(s, 0x30);
		baQueueAnimation((struct BActorInstance *)(actor + 0x10), (struct BActorNode *)DB_PTR(s, 0x124c),
			(struct BAnim *)DB_PTR(s, 0x1248), 0x400, -1, 0, NULL, NULL);
	}
	if (DB_U8(s, 0x1250) != 0 && DB_U8(s, 0x1251) != 0)
	{
		actor = (char *)DB_PTR(s, 0x30);
		baFlushAnimations(actor + 0x10, (struct BActorNode *)DB_PTR(s, 0x125c), 1, 0);
		actor = (char *)DB_PTR(s, 0x30);
		baQueueAnimation((struct BActorInstance *)(actor + 0x10), (struct BActorNode *)DB_PTR(s, 0x125c),
			(struct BAnim *)DB_PTR(s, 0x1258), 0x400, -1, 0, NULL, NULL);
	}
}

/* 0x000f9920  places the controller instance (+0x1254) in the hand: its matrix =
   hand node matrix (+0x1260 node +0x80) x instance, then x the actor's matrix (`dt` is not
   used). The original picks the aligned or unaligned multiply by the addresses. */
void CDemolition_Player_PlaceController(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;
	float m[16];
	char *inst;
	char *node;
	char *actor;

	(void)dt;
	if (DB_U8(s, 0x1250) == 0 || DB_PTR(s, 0x1254) == NULL)
		return;
	baPrepActorInstance((struct BActorInstance *)DB_PTR(s, 0x1254), 1);
	node = (char *)DB_PTR(s, 0x1260);
	inst = (char *)DB_PTR(s, 0x1254);
	if ((((uintptr_t)node | (uintptr_t)inst | (uintptr_t)m) & 0xf) == 0)
		bmMatrixMultiplyCopy(m, (float *)(node + 0x80), (float *)inst);
	else
		bmMatrixMultiply(m, (float *)(node + 0x80), (float *)inst);
	inst = (char *)DB_PTR(s, 0x1254);
	actor = (char *)DB_PTR(s, 0x30);
	if ((((uintptr_t)actor | (uintptr_t)inst | (uintptr_t)m) & 0xf) == 0)
		bmMatrixMultiplyCopy((float *)inst, (float *)(actor + 0x10), m);
	else
		bmMatrixMultiply((float *)inst, (float *)(actor + 0x10), m);
}

/* 0x000f99e0  holds something: the animation `anim` on `node` */
void CDemolition_Player_SetCarry(struct CDemolition_Player *self, void *anim, void *node)
{
	char *s = (char *)self;

	DB_PTR(s, 0x124c) = node;
	DB_PTR(s, 0x1248) = anim;
	DB_U8(s, 0x1244) = 1;
}

/* 0x000f9a00  lets go of what it holds */
void CDemolition_Player_DropCarry(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	if (DB_U8(s, 0x1244) != 0)
	{
		db_release_node(s, DB_PTR(s, 0x124c));
		DB_U8(s, 0x1244) = 0;
	}
}

/* 0x000f9a40  shows the controller: its animation `anim` on `node` (when given) and the
   instance `inst` held at `handNode` (when given) */
void CDemolition_Player_SetController(struct CDemolition_Player *self, void *anim, void *node,
	void *inst, void *handNode)
{
	char *s = (char *)self;

	DB_U8(s, 0x1250) = 1;
	if (anim != NULL)
	{
		DB_U8(s, 0x1251) = 1;
		DB_PTR(s, 0x125c) = node;
		DB_PTR(s, 0x1258) = anim;
	}
	if (inst != NULL)
	{
		DB_PTR(s, 0x1254) = inst;
		DB_PTR(s, 0x1260) = handNode;
	}
}

/* 0x000f9a90  stops the controller's animation */
void CDemolition_Player_StopControllerAnim(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	if (DB_U8(s, 0x1251) != 0)
	{
		db_release_node(s, DB_PTR(s, 0x125c));
		DB_U8(s, 0x1251) = 0;
	}
}

/* 0x000f9ad0  variation 0, the throw button (action2): pressing it starts charging (timer
   0.4 s); releasing both buttons keeps the charge and throws (state 0x10) while the player
   holds a bomb. Knocked out (states 13..17) cancels it. */
void CDemolition_Player_UpdateThrowFrag(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;
	int state = DB_U16(s, 0x36);

	(void)dt;
	if (state >= 0xd && state <= 0x11)
	{
		DB_I32(DB_PTR(s, 0x6c), 0x14) = 0;
		DB_U8(s, 0x11ac) = 0;
	}
	else if (DB_U8(s, 0x1174) != 0)
	{
		if (DB_U8(s, 0x11ac) == 0)
		{
			if (DB_I32(DB_PTR(s, 0x6c), 0x14) != 0)
			{
				DB_U8(s, 0x11ac) = 1;
				FUN_0018ea10((int)(uintptr_t)(s + 0x11d0), 0.4f);
				FUN_0018ea30((int)(uintptr_t)(s + 0x11d0));
			}
		}
		else if (DB_I32(DB_PTR(s, 0x6c), 0x14) == 0 && DB_I32(DB_PTR(s, 0x68), 0x14) == 0)
		{
			DB_U8(s, 0x11ac) = 0;
			DB_F32(s, 0x1210) = BCarTimer_GetRemaining((struct BCarTimer *)(s + 0x11d0));
			DB_U8(s, 0x120c) = 1;
		}
	}
	if (DB_U8(s, 0x1244) == 0)
		DB_U8(s, 0x11ac) = 0;
	if (DB_U8(s, 0x1174) != 0 && DB_U8(s, 0x120c) != 0)
	{
		DB_U8(s, 0x120c) = 0;
		if (DB_U8(s, 0x1244) != 0)
		{
			DB_F32(s, 0x11c8) = DB_F32(s, 0x1210);
			AI_SetState(s, 0x10);
		}
	}
	FUN_0018e8e0((int)(uintptr_t)(s + 0x11d0));
}

/* 0x000f9bb0  variation 0: sets off the frag bomb it holds and lets go of it */
void CDemolition_Player_DetonateFrag(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	DB_U8(s, 0x11ac) = 0;
	if (DB_U8(s, 0x1244) == 0)
		return;
	CDemolition_FragBomb_Detonate((struct CDemolition_FragBomb *)DB_PTR(s, 0x11a8), NULL, NULL);
	if (DB_U8(s, 0x1244) != 0)
	{
		db_release_node(s, DB_PTR(s, 0x124c));
		DB_U8(s, 0x1244) = 0;
	}
}

/* 0x000f9c10  variation 1, the remote bomb: an unarmed bomb follows the player; under
   control the button (action2) places the bomb, or recalls it when armed (not while it
   grows or shrinks, phases 1 and 2) */
void CDemolition_Player_UpdateRemote(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;
	char *bomb = (char *)DB_PTR(s, 0x11a0);
	int phase;

	(void)dt;
	if (bomb == NULL)
		return;
	if (DB_U8(bomb, 0x116d) == 0)
		CDemolition_CharLib_SetPosition((struct CDemolition_CharLib *)bomb, DB_FP(s, 0x1144));
	if (DB_U8(s, 0x1174) == 0 || DB_I32(DB_PTR(s, 0x6c), 0x14) == 0)
		return;
	bomb = (char *)DB_PTR(s, 0x11a0);
	phase = DB_I32(bomb, 0x1190);
	if (phase > 0 && phase <= 2)
		return;
	if (DB_U8(bomb, 0x116d) != 0)
		CDemolition_RemoteBomb_Recall((struct CDemolition_RemoteBomb *)bomb);
	else
		CDemolition_RemoteBomb_Place((struct CDemolition_RemoteBomb *)bomb);
}

/* 0x000f9c80  stops the controller's animation (the same as 0x000f9a90, but only while the
   controller is shown) */
void CDemolition_Player_HideControllerAnim(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	if (DB_U8(s, 0x1250) != 0 && DB_U8(s, 0x1251) != 0)
	{
		db_release_node(s, DB_PTR(s, 0x125c));
		DB_U8(s, 0x1251) = 0;
	}
}

/* 0x000f9cd0  fell off the level (height <= -1990): respawn at the farthest point
   (virtual +0x24); `dt` is not used */
void CDemolition_Player_CheckFall(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;

	(void)dt;
	if (DB_F32(s, 0x1148) <= -1990.0f)
		DB_VFN(s, 0x24, void (*)(void *))(s);
}

/* 0x000f9cf0  variation 3, the throw button (action2): like 0x000f9ad0 with a 0.6 s timer,
   only while holding a bomb; releasing action2 throws */
void CDemolition_Player_UpdateThrowCross(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;
	int state = DB_U16(s, 0x36);

	(void)dt;
	if (state >= 0xd && state <= 0x11)
	{
		DB_I32(DB_PTR(s, 0x6c), 0x14) = 0;
		DB_U8(s, 0x11ac) = 0;
	}
	else
	{
		if (DB_U8(s, 0x1174) == 0)
			goto timer;
		if (DB_U8(s, 0x1244) != 0)
		{
			if (DB_U8(s, 0x11ac) == 0)
			{
				if (DB_I32(DB_PTR(s, 0x6c), 0x14) != 0)
				{
					DB_U8(s, 0x11ac) = 1;
					FUN_0018ea10((int)(uintptr_t)(s + 0x11d0), 0.6f);
					FUN_0018ea30((int)(uintptr_t)(s + 0x11d0));
				}
			}
			else if (DB_I32(DB_PTR(s, 0x6c), 0x14) == 0)
			{
				DB_U8(s, 0x11ac) = 0;
				DB_U8(s, 0x120c) = 1;
				DB_F32(s, 0x1210) = BCarTimer_GetRemaining((struct BCarTimer *)(s + 0x11d0));
			}
		}
	}
	if (DB_U8(s, 0x1174) != 0 && DB_U8(s, 0x120c) != 0)
	{
		DB_U8(s, 0x120c) = 0;
		if (DB_U8(s, 0x1244) != 0)
		{
			DB_F32(s, 0x11c8) = DB_F32(s, 0x1210);
			AI_SetState(s, 0x10);
		}
	}
timer:
	FUN_0018e8e0((int)(uintptr_t)(s + 0x11d0));
}

/* 0x000f9de0  variation 3: sets off the cross bomb it holds and lets go of it */
void CDemolition_Player_DetonateCross(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	DB_U8(s, 0x11ac) = 0;
	if (DB_U8(s, 0x1244) == 0)
		return;
	CDemolition_CrossBomb_Detonate((struct CDemolition_CrossBomb *)DB_PTR(s, 0x11a0), NULL, NULL);
	if (DB_U8(s, 0x1244) != 0)
	{
		db_release_node(s, DB_PTR(s, 0x124c));
		DB_U8(s, 0x1244) = 0;
	}
}

/* 0x000f9e40  AI condition 0 (slot 0x4c): back to STAND: walking (2) with the stick released
   (+0x54 < +0x10e8) or (variations 0 and 3) charging a throw; knocked out (13) when its
   animation ended and +0x1120 < 0; never in 14..17; else when the animation ended */
int CDemolition_Player_CondStand(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	int state = DB_U16(s, 0x36);

	switch (state)
	{
	case 2:
		/* stick below the walk threshold */
		if (DB_F32(s, 0x54) < DB_F32(s, 0x10e8))
			return 1;
		{
			int v = DB_I32(DAT_002ff85c, 0x28);

			if ((v == 0 || v == 3) && DB_U8(s, 0x11ac) != 0)
				return 1;
		}
		return 0;
	case 13:
		/* knocked out: when the animation ended and the get-up started (+0x1120 < 0) */
		if (DB_U16(DB_PTR(s, 0x30), 0x234) != 0 && DB_I32(s, 0x1120) < 0)
			return 1;
		return 0;
	case 14:
	case 15:
	case 16:
	case 17:
		return 0;
	default:
		/* 0, 1, 3..12 (and others): when the animation ended */
		return DB_U16(DB_PTR(s, 0x30), 0x234) != 0;
	}
}

/* 0x000f9ee0  AI condition 2 (slot 0x54): WALK (the stick past +0x10e8); in states other
   than 0, 2, 4 and 11 also only when the animation ended; never in 13..17 */
int CDemolition_Player_CondWalk(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	int state = DB_U16(s, 0x36);
	int v;

	if (state > 0x11)
		goto standing;
	switch (state)
	{
	case 0:
		v = DB_I32(DAT_002ff85c, 0x28);
		if ((v == 0 || v == 3) && DB_U8(s, 0x11ac) != 0)
			return 0;
		/* fall through */
	case 2:
	case 4:
	case 11:
		if (DB_F32(s, 0x54) < DB_F32(s, 0x10e8))
			return 0;
		return 1;
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
		return 0;
	default:
		break;
	}
standing:
	if (DB_F32(s, 0x54) < DB_F32(s, 0x10e8))
		return 0;
	return DB_U16(DB_PTR(s, 0x30), 0x234) != 0;
}

/* 0x000f9f70  AI condition 3 (slot 0x58): JUMP (action1, on the ground; not in variations 1
   and 3, not knocked out) */
int CDemolition_Player_CondJump(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	int state = DB_U16(s, 0x36);
	int v;

	if (state >= 0xd && state <= 0x11)
		return 0;
	v = DB_I32(DAT_002ff85c, 0x28);
	if (v == 1 || v == 3)
		return 0;
	if (DB_I32(DB_PTR(s, 0x68), 0x14) == 0)
		return 0;
	return DB_PTR(DB_PTR(s, 0x30), 0x214) != NULL;
}

/* 0x000f9fb0  AI condition 4 (slot 0x5c): on the ground (+0x214) and not moving up (the
   actor's vertical speed +0x198 <= 0) */
int CDemolition_Player_CondLand(struct CDemolition_Player *self)
{
	char *actor = (char *)DB_PTR(self, 0x30);

	if (DB_PTR(actor, 0x214) == NULL)
		return 0;
	return DB_F32(actor, 0x198) <= 0.0f;
}

/* 0x000f9fe0  AI condition 6 (slot 0x64): action2 pressed, in variation 2 only, not in
   states 14 and 15 */
int CDemolition_Player_CondAction(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	int state;

	if (DB_I32(DAT_002ff85c, 0x28) != 2)
		return 0;
	state = DB_U16(s, 0x36);
	if (state >= 0xe && state <= 0xf)
		return 0;
	return DB_I32(DB_PTR(s, 0x6c), 0x14) != 0;
}

/* 0x000fa010  AI condition 9 (slot 0x70): falling (in the air, +0x214 == 0, and the
   vertical speed at most +0x10ec), or in state 3 any downward speed */
int CDemolition_Player_CondFall(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	unsigned short state = DB_U16(s, 0x36);
	char *actor;

	if (state >= 0xd && state <= 0x11)
		return 0;
	actor = (char *)DB_PTR(s, 0x30);
	if (DB_PTR(actor, 0x214) != NULL)
		return 0;
	if (DB_F32(actor, 0x198) <= DB_F32(s, 0x10ec))
		return 1;
	if (state != 3)
		return 0;
	return DB_F32(actor, 0x198) < 0.0f;
}

/* 0x000fa070  AI state 4 (slot 0xa4): land ("land", voice 1) */
int CDemolition_Player_StateLand(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;

	db_state_begin(s);
	if (entered)
	{
		Actor_PlayAnim(DB_PTR(s, 0x30), 5, 0, 0, 1.0f, 0.1f, 1);
		AI_PlayVoice(s, 1, DAT_00293928, 0.0f, 0, -1, 0);
	}
	return db_state_end(s);
}

/* 0x000fa0e0  AI state 10 (slot 0xbc): hurt (voice 2, "front_hurt" at the hurt speed, mode
   -1) */
int CDemolition_Player_StateHurt(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;

	db_state_begin(s);
	if (entered)
	{
		AI_PlayVoice(s, 2, DAT_00293928, 0.0f, 0, -1, 0);
		Actor_PlayAnim(DB_PTR(s, 0x30), 0x11, 0, 0, DAT_002b05a4, 0.1f, 1);
		DB_VFN(s, 0x18, void (*)(void *, int))(s, -1);
	}
	return db_state_end(s);
}

/* 0x000fa160  AI state 11 (slot 0xc0): TAUNT (voice 11 with a 180 frame cooldown,
   "attack_1") */
int CDemolition_Player_StateTaunt(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;

	db_state_begin(s);
	if (entered)
	{
		AI_PlayVoice(s, 0xb, DAT_00293928, 180.0f, 0, -1, 0);
		Actor_PlayAnim(DB_PTR(s, 0x30), 7, 0, 0, 1.0f, 0.1f, 1);
	}
	return db_state_end(s);
}

/* 0x000fa1e0  AI state 15 (slot 0xd0): GET UP ("front_knockout_getup" or
   "back_knockout_getup" by +0x11b0); when the animation ended: STAND, and in variation 1
   the controller comes back to the hand */
int CDemolition_Player_StateGetUp(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;

	db_state_begin(s);
	if (entered)
		Actor_PlayAnim(DB_PTR(s, 0x30), DB_U8(s, 0x11b0) != 0 ? 0x15 : 0x16, 0, 0, 1.0f, 0.1f, 1);
	if (DB_U16(DB_PTR(s, 0x30), 0x234) != 0)
	{
		AI_SetState(s, 0);
		if (DB_I32(DAT_002ff85c, 0x28) == 1)
			CDemolition_Player_SetController(self, DB_PTR(DB_PTR(s, 0x30), 0x30c), DB_PTR(s, 0x1238),
				s + 0x1270, DB_PTR(s, 0x1240));
	}
	return db_state_end(s);
}

/* 0x000fa290  AI state 17 (slot 0xd8): victory ("victory", then "victory_idle" queued),
   facing the results camera (game +0x12c8) */
int CDemolition_Player_StateVictory(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	char *actor;

	db_state_begin(s);
	if (entered)
	{
		Actor_PlayAnim(DB_PTR(s, 0x30), 0x1b, 0, 0, 1.0f, 0.1f, 1);
		Actor_PlayAnim(DB_PTR(s, 0x30), 0x23, 1, 1, 1.0f, 0.05f, 1);
	}
	actor = (char *)DB_PTR(s, 0x30);
	db_state_end(s);
	DB_F32(actor, 0x1d8) = DB_F32(DAT_002ff85c, 0x12c8);
	actor = (char *)DB_PTR(s, 0x30);
	DB_F32(actor, 0x1e8) = DB_F32(actor, 0x1d8);
	return 0;
}

/* 0x000fa330  hit by `damage` from `from` (a position when `how` is 0): the damage adds up
   (+0x111c); past the character's limit it knocks the player out (rumble, state 13,
   "front_knockout" when hit from the front, else "back_knockout"), else it hurts (state 10,
   "front_hurt" / "back_hurt"). With `how` 0 the player turns towards the hit. `how` 1
   means from behind. */
void CDemolition_Player_Hit(struct CDemolition_Player *self, float *from, float damage, int how)
{
	char *s = (char *)self;
	char *actor;
	float heading = 0.0f;
	float angle;
	int character;

	DB_F32(s, 0x10d8) = DAT_002988d4;
	if (how == 0)
	{
		heading = CLMath_HeadingTo(DB_FP(DB_PTR(s, 0x30), 0xd0), from);
		angle = CLMath_AngleDistance(heading, DB_F32(DB_PTR(s, 0x30), 0x1e8));
	}
	else
	{
		/* the original compares the pointer's bits as a float (only `how` 1 reaches the
		   comparisons below, and there the result does not matter) */
		uint32_t bits = (uint32_t)(uintptr_t)from;

		memcpy(&angle, &bits, sizeof angle);
	}
	actor = (char *)DB_PTR(s, 0x30);
	DB_F32(s, 0x111c) = damage + DB_F32(s, 0x111c);
	character = DAT_002baed4[DB_U16(actor, 0x230) * 10];
	if (DB_F32(s, 0x111c) > *(float *)(DAT_00290738 + character * 0x60))
	{
		CLRumble_PlayNamed((int)(uintptr_t)DAT_002ed324, DB_U16(actor, 0x230),
			(char *)"Generic Knock Down");
		AI_SetState(s, 0xd);
		DB_F32(s, 0x111c) = 0.0f;
		if (!(angle > db_bitsf(0x3fc90fda)) && how != 1)   /* 1.57079625 */
		{
			DB_I32(s, 0x1120) = 1;
			Actor_PlayAnim(DB_PTR(s, 0x30), 0x13, 0, 0, DAT_002b05a8, 0.1f, 1);
			if (how == 0)
				DB_F32(DB_PTR(s, 0x30), 0x1d8) = heading;
		}
		else
		{
			DB_I32(s, 0x1120) = 0;
			Actor_PlayAnim(DB_PTR(s, 0x30), 0x14, 0, 0, DAT_002b05a8, 0.1f, 1);
			if (how == 0)
				DB_F32(DB_PTR(s, 0x30), 0x1d8) = CLMath_WrapAngle(heading + db_bitsf(0x40490fda));
		}
	}
	else
	{
		AI_SetState(s, 0xa);
		if (!(angle > db_bitsf(0x3fc90fda)) && how != 1)
			Actor_PlayAnim(DB_PTR(s, 0x30), 0x11, 0, 0, DAT_002b05a4, 0.1f, 1);
		else
			Actor_PlayAnim(DB_PTR(s, 0x30), 0x12, 0, 0, DAT_002b05a4, 0.1f, 1);
	}
}

/* 0x000fa500  deleting destructor through the message receiver (vtable 0x0026b440) */
void *CDemolition_Player_Receiver_DeletingDestruct(void *receiver, unsigned int flags)
{
	return CDemolition_Player_DeletingDestruct(
		(struct CDemolition_Player *)((char *)receiver - 0x1130), flags);
}

/* 0x000fa510  deleting destructor (vtable 0x0026b448 slot 18) */
void *CDemolition_Player_DeletingDestruct(struct CDemolition_Player *self, unsigned int flags)
{
	CDemolition_Player_Destruct(self);
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000fa530  Init(index) (virtual +0xdc): the input, the actor, the event queue, size,
   colour (the character's, halved), the throw timer, the hand nodes, the idle animation,
   the controller instance (model game +0xa0 + 4 * index), then Reset (virtual +0xf0) */
int CDemolition_Player_Init(struct CDemolition_Player *self, int index)
{
	char *s = (char *)self;
	int character;
	int i;

	DB_I32(s, 0x1184) = index;
	DB_U8(s, 0x116c) = 1;
	DB_U8(s, 0x116d) = 0;
	DB_U8(s, 0x116e) = 1;
	DB_U8(s, 0x116f) = 1;
	DB_U8(s, 0x1171) = 0;
	DB_U8(s, 0x1173) = 0;
	DB_U8(s, 0x1172) = 0;
	CDemolition_Player_CreateInput(self, index);
	CDemolition_Player_CreateActor(self, index);
	DemoCharEventSystem_InitQueue((int)(uintptr_t)(s + 0x1130), 0x10);
	DB_F32(s, 0x1168) = 177.0f;
	DB_F32(s, 0x1164) = 50.0f;
	character = DAT_002baed4[index * 10];
	DB_I32(s, 0x1188) = character;
	DB_U32(s, 0x1178) = DAT_00297ad8[character][0] >> 1;
	DB_U32(s, 0x117c) = DAT_00297ad8[DB_I32(s, 0x1188)][1] >> 1;
	DB_U32(s, 0x1180) = DAT_00297ad8[DB_I32(s, 0x1188)][2] >> 1;
	CDemoCountdown_Init((int)(uintptr_t)(s + 0x11d0));
	for (i = 0; i < 3; i++)
		DB_PTR(s, 0x1238 + i * 4) = baFindNode(
			(struct BActorNode *)DB_PTR(DB_PTR(s, 0x30), 0x144), DAT_00298868[i]);
	Actor_PlayAnim(DB_PTR(s, 0x30), 0, 1, 0, 1.0f, 0.1f, 1);
	for (i = 0; i < 1; i++)
		baInitActorInstance((struct BActorInstance *)(s + 0x1270),
			(struct BActor *)DB_PTR(DAT_002ff85c, 0xa0 + DB_I32(s, 0x1184) * 4), 1, DB_PLAYER_FILE,
			0x108);
	DB_VFN(s, 0xf0, void (*)(void *))(s);
	return 1;
}

/* 0x000fa6a0  Shutdown (virtual +0xe0): the input, the brain (its virtual +0xc, then
   freed), the controller instance, and the player's AI slot */
void CDemolition_Player_Shutdown(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	void *brain;

	biDeleteChannel(DB_PTR(s, 0x64));
	biDeleteChannel(DB_PTR(s, 0x60));
	biDeleteChannel(DB_PTR(s, 0x68));
	biDeleteChannel(DB_PTR(s, 0x6c));
	biDeleteMap(DB_PTR(s, 0x5c));
	brain = DB_PTR(s, 0x118c);
	if (brain != NULL)
	{
		DB_VFN(brain, 0xc, void (*)(void *))(brain);
		bkHeapFree(DB_PTR(s, 0x118c));
		DB_PTR(s, 0x118c) = NULL;
	}
	baDeleteActorInstance((struct BActorInstance *)(s + 0x1270));
	DAT_002f289c[DB_I32(s, 0x1184)] = 0;
}

/* 0x000fa720  Draw (virtual +0xe8): the brain's debug draw for CPU players (virtual +8),
   the actor (or its skeleton in the player's colour when the variation hides models,
   settings +0x04), and the controller while the round runs */
void CDemolition_Player_Draw(struct CDemolition_Player *self)
{
	char *s = (char *)self;

	if (DB_U8(s, 0x116e) == 0)
		return;
	bdSetRenderState(6, 0, 0);
	if (DB_U8(s, 0x1171) != 0)
		DB_VFN(DB_PTR(s, 0x118c), 8, void (*)(void *))(DB_PTR(s, 0x118c));
	if (DAT_00297520[DB_I32(DAT_002ff85c, 0x28) * 15 + 1] != 0)
		baDrawActorInstance((struct BActorInstance *)((char *)DB_PTR(s, 0x30) + 0x10), 7);
	else
		CDemoDebug_DrawSkeleton(DAT_002ff85c + 0x78f8, (char *)DB_PTR(s, 0x30) + 0x10,
			DB_I32(s, 0x1178), DB_I32(s, 0x117c), DB_I32(s, 0x1180), 0xff);
	if (DB_U8(DAT_002ff85c, 0x25) == 0 && DB_U8(s, 0x1250) != 0 && DB_PTR(s, 0x1254) != NULL)
		baDrawActorInstance((struct BActorInstance *)DB_PTR(s, 0x1254), 7);
}

/* 0x000fa7e0  sets the start position: a respawn point there (charlib, registers itself),
   the position, and in the arena (variation 3) the player faces away from the centre */
void CDemolition_Player_SetStart(struct CDemolition_Player *self, float *pos)
{
	char *s = (char *)self;
	void *point;
	float euler[4];

	memcpy(s + 0x1214, pos, 16);
	memcpy(s + 0x1224, s + 0x1214, 16);
	point = bkHeapAlloc(0x20, DB_PLAYER_FILE, 0x3cd, 0x1001);
	if (point != NULL)
		RespawnPoint_Init(point, DB_FP(s, 0x1224), 0);
	CDemolition_CharLib_SetPosition((struct CDemolition_CharLib *)s, DB_FP(s, 0x1214));
	if (DB_I32(DAT_002ff85c, 0x28) == 3)
	{
		euler[0] = 0.0f;
		euler[1] = (float)(atan2((double)DB_F32(s, 0x1214), (double)DB_F32(s, 0x121c)) +
			(double)3.1415927f);
		euler[2] = 0.0f;
		euler[3] = 0.0f;   /* uninitialised in the original */
		CDemolition_CharLib_SetRotation((struct CDemolition_CharLib *)s, euler);
	}
}

/* 0x000fa8d0  variation 0 update: falling off the level sets off a held bomb and respawns;
   the throw button; under control with empty hands a free frag bomb of the player's two
   (+0x11a0, +0x11a4) appears in the hand; the game's held-model slot (game +0xfc); knocked
   out (13..15, 17) drops held bombs (state 1 -> 0) */
void CDemolition_Player_UpdateFrag(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;
	int state;
	int i;
	char *bomb;

	if (DB_F32(s, 0x1148) <= -100.0f)
	{
		if (DB_U8(s, 0x1244) != 0)
		{
			CDemolition_FragBomb_Detonate((struct CDemolition_FragBomb *)DB_PTR(s, 0x11a8), NULL, NULL);
			CDemolition_Player_DropCarry(self);
		}
		DB_VFN(s, 0x24, void (*)(void *))(s);
	}
	if (DB_I32(s, 0x1190) == 0)
		CDemolition_Player_UpdateThrowFrag(self, dt);
	state = DB_U16(s, 0x36);
	if (!(state >= 0xd && state <= 0x11))
	{
		if (DB_U8(s, 0x1244) == 0 && DB_U8(s, 0x1174) != 0 && DB_U8(DAT_002ff85c, 0x25) == 0)
		{
			for (i = 0; i < 2; i++)
				if (DB_U8(DB_PTR(s, 0x11a0 + i * 4), 0x15) == 0)
					break;
			if (i < 2)
			{
				bomb = (char *)DB_PTR(s, 0x11a0 + i * 4);
				if (bomb != NULL)
				{
					DB_PTR(s, 0x1248) = DB_PTR(DB_PTR(s, 0x30), 0x300);
					DB_PTR(s, 0x124c) = DB_PTR(s, 0x1238);
					DB_U8(s, 0x1244) = 1;
					CDemolition_FragBomb_PickUp((struct CDemolition_FragBomb *)bomb);
				}
			}
		}
	}
	if (DB_U8(s, 0x1244) != 0)
		DB_PTR(DAT_002ff85c, 0xfc + DB_I32(s, 0x1184) * 4) = (char *)DB_PTR(s, 0x11a8) + 0x4460;
	else
		DB_PTR(DAT_002ff85c, 0xfc + DB_I32(s, 0x1184) * 4) = NULL;
	state = DB_U16(s, 0x36);
	if ((state >= 0xd && state <= 0xf) || state == 0x11)
	{
		for (i = 0; i < 2; i++)
		{
			bomb = (char *)DB_PTR(s, 0x11a0 + i * 4);
			if (DB_I32(bomb, 0x4454) == 1)
			{
				DB_I32(bomb, 0x4454) = 0;
				DB_U8(bomb, 0x15) = 0;
				DB_U8(bomb, 0x16) = 0;
				DB_U8(bomb, 0x17) = 0;
				DB_U8(bomb, 0x1c) = 0;
			}
		}
	}
}

/* 0x000faa40  variation 1 update: falling off respawns; the remote bomb */
void CDemolition_Player_UpdateRemoteVariation(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;

	if (DB_F32(s, 0x1148) <= -1990.0f)
		DB_VFN(s, 0x24, void (*)(void *))(s);
	if (DB_I32(s, 0x1190) == 0)
		CDemolition_Player_UpdateRemote(self, dt);
}

/* 0x000faa80  variation 3 update: falling off respawns (while in play); the throw button;
   the game's held-model slot (game +0xfc) */
void CDemolition_Player_UpdateCross(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;

	if (DB_U8(s, 0x116c) != 0 && DB_F32(s, 0x1148) <= -1990.0f)
		DB_VFN(s, 0x24, void (*)(void *))(s);
	if (DB_I32(s, 0x1190) == 0)
		CDemolition_Player_UpdateThrowCross(self, dt);
	if (DB_U8(s, 0x1244) != 0)
		DB_PTR(DAT_002ff85c, 0xfc + DB_I32(s, 0x1184) * 4) = (char *)DB_PTR(s, 0x11a0) + 0x4460;
	else
		DB_PTR(DAT_002ff85c, 0xfc + DB_I32(s, 0x1184) * 4) = NULL;
}

/* 0x000fab10  variation 3: throws the cross bomb (+0x11a0) the way the player faces, 30
   degrees up, at 35 + 90 * the charge (+0x11c8) */
void CDemolition_Player_ThrowCross(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	char *bomb = (char *)DB_PTR(s, 0x11a0);
	float speed;
	float v[4];
	float dir[4];
	float velocity[4];

	if (bomb == NULL)
		return;
	if (DB_U8(s, 0x1244) != 0)
	{
		db_release_node(s, DB_PTR(s, 0x124c));
		DB_U8(s, 0x1244) = 0;
	}
	speed = DB_F32(s, 0x11c8) * 90.0f + 35.0f;
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 100.0f;
	v[3] = 1.0f;
	bmVectorRotateXY(dir, v, db_bitsf(0xbf060a92), DB_F32(DB_PTR(s, 0x30), 0x1e8));
	bmVectorNormalise(velocity, dir);
	velocity[0] = velocity[0] * speed;
	velocity[1] = velocity[1] * speed;
	velocity[2] = velocity[2] * speed;
	CDemolition_CrossBomb_Throw((struct CDemolition_CrossBomb *)bomb, s, velocity);
}

/* ---------- the CharLib movement globals (charlib/meta_a .data; see meta_a_globals.h) */
extern float DAT_002b074c;      /* attack angle (degrees) */
extern float DAT_002b0750;      /* velocity kept when stopping */
extern float DAT_002b0768;      /* stand time before idling */
extern float DAT_002b076c;      /* idle time before standing */
extern int DAT_002b0770, DAT_002b0774;
extern float DAT_002b0778, DAT_002b077c;
extern int DAT_002b0780;
extern float DAT_002b0784;      /* attack step speed */
extern float DAT_002b0788;      /* attack step time */
extern float DAT_002b078c[4];   /* attack blend speeds */
extern float DAT_002b079c[4];   /* attack animation speeds */
extern float DAT_002b07ac;      /* jump attack speed */
extern float DAT_002b07b0;      /* jump attack range */
extern float DAT_002b07bc;      /* knockout slide time */
extern float DAT_002b07c0;      /* knockout slide speed */
extern float DAT_002b059c, DAT_002b05a0;   /* walk -> run, run -> walk speeds */
extern float DAT_00292c98;      /* jump speed */
extern float DAT_002f2890, DAT_002f2894;
extern int DAT_00305650;
extern float DAT_00305654;
extern float DAT_00293920;      /* the global animation speed factor */
extern float DAT_00293a38;      /* reach scale with the power-up */
extern float DAT_0028fdd8[4];   /* (0, 0, 1, 1) */
extern int DAT_002ba56c;        /* the debug free camera is on: full models */
extern int DAT_00283ea4;        /* the colour set (core) */
/* 0x00290728  the characters (classlib_ui, 0x60 bytes each): +0x00 attack reach,
   +0x04 knockback, +0x10 the damage that knocks out (0x00290738) */
extern char DAT_00290728[];
#define DB_CHARACTER_F(c, off) (*(float *)(DAT_00290728 + (c) * 0x60 + (off)))

/* the push velocity from the stick: direction (+0x44) * stick amount * speed scale */
static void db_push_from_stick(char *s)
{
	float k = DB_F32(s, 0x10b0) * DB_F32(s, 0x54);

	DB_F32(s, 0x7c) = k * DB_F32(s, 0x44);
	DB_F32(s, 0x80) = k * DB_F32(s, 0x48);
	DB_F32(s, 0x84) = k * DB_F32(s, 0x4c);
}

/* the actor's velocity (+0x194) scaled by `k` */
static void db_scale_velocity(char *actor, float k)
{
	DB_F32(actor, 0x194) = k * DB_F32(actor, 0x194);
	DB_F32(actor, 0x198) = k * DB_F32(actor, 0x198);
	DB_F32(actor, 0x19c) = k * DB_F32(actor, 0x19c);
}

/* the character index of the player's actor (+0x230) */
static int db_character(char *s)
{
	return DAT_002baed4[DB_U16(DB_PTR(s, 0x30), 0x230) * 10];
}

/* 0x000fac10  AI condition 5 (slot 0x60), variation 2 only: ATTACK on action2: a player in
   front within the reach (1), else something hittable in front (1), else 2 */
int CDemolition_Player_CondAttack(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	char *actor;
	char *other;
	char *platform = NULL;
	int i;
	float heading;
	float turn;
	float distance;
	double reach;
	float from[4];
	float ray[4];
	float hit[4];
	float k;
	double t;

	if (DB_I32(DAT_002ff85c, 0x28) != 2 || DB_I32(DB_PTR(s, 0x6c), 0x14) == 0)
		return 0;
	for (i = 0; i < DAT_002bae94; i++)
	{
		actor = (char *)DB_PTR(s, 0x30);
		if (i == DB_U16(actor, 0x230))
			continue;
		other = (char *)(uintptr_t)DAT_002f289c[i];
		if (DB_U16(other, 0x36) == 0xd)
			continue;
		if (!(DB_F32(s, 0x10f8) > fabsf(DB_F32(actor, 0xd4) - DB_F32(DB_PTR(other, 0x30), 0xd4))))
			continue;
		heading = CLMath_HeadingTo(DB_FP(actor, 0xd0), DB_FP(DB_PTR(other, 0x30), 0xd0));
		turn = CLMath_AngleDistance(heading, DB_F32(DB_PTR(s, 0x30), 0x1e8));
		if (!((double)turn < (double)DAT_002b074c * (double)0.017453292f))
			continue;
		distance = bmVectorDistance(DB_FP(DB_PTR(s, 0x30), 0xd0),
			DB_FP(DB_PTR((char *)(uintptr_t)DAT_002f289c[i], 0x30), 0xd0));
		reach = DB_CHARACTER_F(db_character(s), 0);
		if (DB_F32(s, 0x10a4) != 0.0f)
			reach = reach * (double)DAT_00293a38;
		if (reach * (double)0.8f > ((double)turn + 1.0) * (double)distance)
		{
			DB_U16((char *)(uintptr_t)DAT_002f289c[i], 0x1088) = DB_U16(DB_PTR(s, 0x30), 0x230);
			DB_I16(s, 0x1084) = (short)i;
			DB_F32(DB_PTR(s, 0x30), 0x1d8) = heading;
			FUN_001d5950();   /* a debug call (100.0, 8) compiled out */
			return 1;
		}
	}
	/* a ray in front of the player, at chest height, as long as the reach */
	actor = (char *)DB_PTR(s, 0x30);
	memcpy(from, DB_FP(actor, 0xd0), 16);
	from[1] = from[1] + 80.0f;
	bmVectorRotateY(ray, DAT_0028fdd8, DB_F32(actor, 0x1e8));
	k = DB_CHARACTER_F(db_character(s), 0);
	ray[0] = ray[0] * k;
	ray[1] = ray[1] * k;
	ray[2] = ray[2] * k;
	t = (double)CRColl_RayCast(from, ray, hit, NULL, (void **)&platform);
	if (platform != NULL && DB_I32(platform, 0x440) != 0 && t > 0.0)
	{
		DB_I16(s, 0x1084) = -1;
		return 1;
	}
	DB_I16(s, 0x1084) = -1;
	return 2;
}

/* 0x000faec0  AI state 0 (slot 0x94): STAND ("idle_ready"); variations 0 and 3 turn to the
   stick while charging a throw; after a while IDLE, 1 in 30 frames */
int CDemolition_Player_StateStand(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	int v;

	db_state_begin(s);
	if (entered)
	{
		Actor_PlayAnim(DB_PTR(s, 0x30), 0, 1, 0, 1.0f, 0.1f, 1);
		db_scale_velocity((char *)DB_PTR(s, 0x30), DAT_002b0750);
	}
	db_state_end(s);
	v = DB_I32(DAT_002ff85c, 0x28);
	if ((v == 0 || v == 3) && DB_U8(s, 0x11ac) != 0 && !(DB_F32(s, 0x54) < DB_F32(s, 0x10e8)))
		DB_F32(DB_PTR(s, 0x30), 0x1d8) = CLMath_HeadingXZ(DB_FP(s, 0x44));
	if (!(DB_F32(s, 0x40) < DAT_002b0768) && db_rand() % 0x1e == 0)
		AI_SetState(s, 1);
	return 0;
}

/* 0x000fafe0  AI state 1 (slot 0x98): IDLE ("idle"); back to STAND after a while, 1 in 30
   frames */
int CDemolition_Player_StateIdle(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;

	db_state_begin(s);
	if (entered)
		Actor_PlayAnim(DB_PTR(s, 0x30), 0xb, 1, 0, 1.0f, 0.1f, 1);
	db_state_end(s);
	if (DB_F32(s, 0x40) > DAT_002b076c && db_rand() % 0x1e == 0)
		AI_SetState(s, 0);
	return 0;
}

/* 0x000fb080  AI state 2 (slot 0x9c): WALK, switching between "walk" and "run" by the
   actor's speed (+0x254), pushed by the stick */
int CDemolition_Player_StateWalk(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	char *actor;
	float rate;

	db_state_begin(s);
	if (entered)
	{
		DB_I32(s, 0x10e4) = 1;
		if (DAT_00305650 == 0 || DB_I16(s, 0x38) != 4)
			Actor_PlayAnim(DB_PTR(s, 0x30), 1, 1, 0, 0.0f, DAT_0028fda8 * 0.1f, 1);
		else
		{
			actor = (char *)DB_PTR(s, 0x30);
			baFlushAnimations(actor + 0x10, (struct BActorNode *)DB_PTR(actor, 0x144), 1, 0);
			DB_I16(DB_PTR(s, 0x30), 0x236) = 0;
			actor = (char *)DB_PTR(s, 0x30);
			baQueueBlendedAnimation((struct BActorInstance *)(actor + 0x10),
				(struct BActorNode *)DB_PTR(actor, 0x144), (struct BAnim *)DB_PTR(actor, 0x2c4),
				(struct BAnim *)DB_PTR(actor, 0x278), DAT_002b0770, DAT_002b0774, -1, -1,
				DAT_002b0778, 3, NULL, NULL);
		}
	}
	if (DAT_00305650 != 0 && DB_I16(s, 0x38) == 4 && DB_I32(s, 0x3c) == DAT_002b0780)
		baChangeAnimation((struct BActorNode *)DB_PTR(DB_PTR(s, 0x30), 0x144), 1, DAT_002b0770,
			DAT_002b0774, DAT_002b077c, 0.0f, 7);
	if (DB_I32(s, 0x10e4) != 0)
	{
		if (DB_F32(DB_PTR(s, 0x30), 0x254) > DAT_002b059c)
		{
			Actor_SwitchAnim(DB_PTR(s, 0x30), 0x18, 0.05f);
			DB_I32(s, 0x10e4) = 0;
			goto speed;
		}
		goto speed;
	}
	if (DB_F32(DB_PTR(s, 0x30), 0x254) < DAT_002b05a0)
	{
		Actor_SwitchAnim(DB_PTR(s, 0x30), 1, 0.05f);
		DB_I32(s, 0x10e4) = 1;
	}
speed:
	actor = (char *)DB_PTR(s, 0x30);
	if (DB_I32(s, 0x10e4) != 0)
		rate = DB_F32(DB_PTR(actor, 0x324), 0x84);    /* "walk" */
	else
		rate = DB_F32(DB_PTR(actor, 0x324), 0x6a0);   /* "run" */
	DB_F32(actor, 0x23c) = rate * DB_F32(actor, 0x254) / DAT_0028fda8;
	db_push_from_stick(s);
	DB_F32(DB_PTR(s, 0x30), 0x1d8) = CLMath_HeadingXZ(DB_FP(s, 0x7c));
	return 0;
}

/* 0x000fb270  AI state 3 (slot 0xa0): JUMP (with the platform's velocity), turning to the
   stick */
int CDemolition_Player_StateJump(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	char *actor;
	char *platform;

	db_state_begin(s);
	if (entered)
	{
		DB_F32(DB_PTR(s, 0x30), 0x198) = DAT_00292c98;
		actor = (char *)DB_PTR(s, 0x30);
		platform = (char *)DB_PTR(DB_PTR(actor, 0x18c), 0x1b0);
		if (platform != NULL)
		{
			DB_F32(actor, 0x194) = DB_F32(platform, 0x1a4) + DB_F32(actor, 0x194);
			actor = (char *)DB_PTR(s, 0x30);
			DB_F32(actor, 0x198) = DB_F32(DB_PTR(DB_PTR(actor, 0x18c), 0x1b0), 0x1a8) +
				DB_F32(actor, 0x198);
			actor = (char *)DB_PTR(s, 0x30);
			DB_F32(actor, 0x19c) = DB_F32(DB_PTR(DB_PTR(actor, 0x18c), 0x1b0), 0x1ac) +
				DB_F32(actor, 0x19c);
		}
		Actor_PlayAnim(DB_PTR(s, 0x30), 2, 0, 0, 1.0f, 0.3f, 1);
		if (!(DB_F32(s, 0x54) < DB_F32(s, 0x10e8)))
			DB_F32(DB_PTR(s, 0x30), 0x1d8) = CLMath_HeadingXZ(DB_FP(s, 0x7c));
		if (DB_U16(DB_PTR(s, 0x30), 0x230) == 0)
		{
			DAT_002f2894 = DB_F32(DB_PTR(s, 0x30), 0xd4);
			DAT_002f2890 = 0.0f;
		}
		AI_PlayVoice(s, 0, DAT_00293928, 0.0f, 0, -1, 0);
		DB_F32(DB_PTR(s, 0x30), 0x238) = -1.0f;
	}
	if (!(DB_F32(s, 0x54) < DB_F32(s, 0x10e8)))
		DB_F32(DB_PTR(s, 0x30), 0x1d8) = CLMath_HeadingXZ(DB_FP(s, 0x44));
	db_push_from_stick(s);
	return 0;
}

/* 0x000fb3e0  AI state 5 (slot 0xa8): ATTACK: one of three attacks ("attack1", "attack2",
   "attack_knockout" by rand, with a sound), or with the power-up (+0x10a4) the super
   attack; steps forward during the attack time, faces the enemy (+0x1084), and lands the
   hit (virtual +0x10) in its window (+0x1104..+0x1108, frames) */
int CDemolition_Player_StateAttack(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	int kind;
	int r;
	float heading;
	float k;

	db_state_begin(s);
	if (entered)
	{
		if (DB_F32(s, 0x10a4) == 0.0f)
		{
			kind = db_rand() % 3;
			r = db_rand();
			CLSamples_PlayGeneric(kind == 0 ? (r % 2) + 0xf : (r % 2) + 0x11, 0x7f, 0, 0, NULL);
			Actor_PlayAnim(DB_PTR(s, 0x30), kind + 0xd, 0, 0, DAT_002b079c[kind],
				DAT_002b078c[kind], 1);
		}
		else
		{
			CLSamples_PlayGeneric((db_rand() % 2) + 0x11, 0x7f, 0, 0, NULL);
			Actor_PlayAnim(DB_PTR(s, 0x30), DB_PTR(DB_PTR(s, 0x30), 0x274) != NULL ? 4 : 0xe, 0, 0,
				DAT_002b079c[3], DAT_002b078c[3], 0);
		}
		DB_I16(s, 0x1092) = 0;
	}
	if (DB_F32(s, 0x40) <= DAT_002b0788 && !(DB_F32(s, 0x40) < DAT_00305654))
	{
		bmVectorRotateY(DB_FP(s, 0x7c), DAT_0028fdd8, DB_F32(DB_PTR(s, 0x30), 0x1d8));
		k = DAT_002b0784;
		DB_F32(s, 0x7c) = k * DB_F32(s, 0x7c);
		DB_F32(s, 0x80) = k * DB_F32(s, 0x80);
		DB_F32(s, 0x84) = k * DB_F32(s, 0x84);
	}
	else
	{
		DB_F32(s, 0x84) = 0.0f;
		DB_F32(s, 0x80) = 0.0f;
		DB_F32(s, 0x7c) = 0.0f;
	}
	if (DB_I16(s, 0x1084) != -1)
	{
		heading = CLMath_HeadingTo(DB_FP(DB_PTR(s, 0x30), 0xd0),
			DB_FP(DB_PTR((char *)(uintptr_t)DAT_002f289c[DB_I16(s, 0x1084)], 0x30), 0xd0));
		DB_F32(DB_PTR(s, 0x30), 0x1d8) = heading;
		if (entered)
			DB_F32(DB_PTR(s, 0x30), 0x1e8) = heading;
	}
	if ((float)DB_I32(s, 0x1104) <= DB_F32(s, 0x40) &&
		(!((float)DB_I32(s, 0x1108) < DB_F32(s, 0x40)) || DB_I16(s, 0x1092) == 0))
		DB_VFN(s, 0x10, int (*)(void *))(s);
	return 0;
}

/* 0x000fb630  AI state 6 (slot 0xac): JUMP ATTACK ("attack_flying_kick"): aims at the
   nearest-in-angle player in range (+0x1084, -1 none) and flies at it; once the hit lands
   (virtual +0x10, window +0x110c..+0x1110 frames) +0x1084 becomes -2 and it stops */
int CDemolition_Player_StateJumpAttack(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	int best = -1;
	float bestTurn = db_bitsf(0x3fc90fda);
	int i;
	char *other;
	float heading;
	float turn;

	if (entered)
	{
		Actor_PlayAnim(DB_PTR(s, 0x30), 0x10, 0, 0, 1.0f, 0.1f, 1);
		DB_I16(s, 0x1092) = 0;
		for (i = 0; i < 4; i++)
		{
			if (i == DB_U16(DB_PTR(s, 0x30), 0x230))
				continue;
			other = (char *)(uintptr_t)DAT_002f289c[i];
			if (DB_I16(other, 0x36) == 0xd)
				continue;
			if (!(DAT_002b07b0 * DAT_002b07b0 > CLVec3_DistanceSqXZ(DB_FP(DB_PTR(s, 0x30), 0xd0),
				DB_FP(DB_PTR(other, 0x30), 0xd0))))
				continue;
			heading = CLMath_HeadingTo(DB_FP(DB_PTR(s, 0x30), 0xd0),
				DB_FP(DB_PTR((char *)(uintptr_t)DAT_002f289c[i], 0x30), 0xd0));
			turn = CLMath_AngleDistance(heading, DB_F32(DB_PTR(s, 0x30), 0x1e8));
			if (bestTurn == -1.0f || turn < bestTurn)
			{
				bestTurn = turn;
				best = i;
			}
		}
		if (best != -1)
		{
			DB_F32(DB_PTR(s, 0x30), 0x1d8) = CLMath_HeadingTo(DB_FP(DB_PTR(s, 0x30), 0xd0),
				DB_FP(DB_PTR((char *)(uintptr_t)DAT_002f289c[best], 0x30), 0xd0));
			DB_I16(s, 0x1084) = (short)best;
		}
		else
			DB_I16(s, 0x1084) = -1;
	}
	DB_F32(s, 0x80) = 0.0f;
	if (DB_I16(s, 0x1084) == -2)
	{
		DB_F32(s, 0x7c) = 0.0f;
		DB_F32(s, 0x84) = 0.0f;
		DB_F32(s, 0x88) = 1.0f;
	}
	else
	{
		DB_F32(s, 0x7c) = 0.0f;
		DB_F32(s, 0x84) = DAT_002b07ac;
		bmVectorRotateY(DB_FP(s, 0x7c), DB_FP(s, 0x7c), DB_F32(DB_PTR(s, 0x30), 0x1d8));
	}
	if (DB_I16(s, 0x1084) != -2 && (float)DB_I32(s, 0x110c) <= DB_F32(s, 0x40) &&
		(!((float)DB_I32(s, 0x1110) < DB_F32(s, 0x40)) || DB_I16(s, 0x1092) == 0) &&
		DB_VFN(s, 0x10, int (*)(void *))(s) != 0)
		DB_I16(s, 0x1084) = -2;
	return 0;
}

/* 0x000fb820  AI state 9 (slot 0xb8): FALL ("fall"), pushed by the stick */
int CDemolition_Player_StateFall(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;

	db_state_begin(s);
	if (entered)
		Actor_PlayAnim(DB_PTR(s, 0x30), 0xa, 1, 0, 1.0f, 0.3f, 1);
	db_push_from_stick(s);
	return 0;
}

/* 0x000fb890  AI state 13 (slot 0xc8): KNOCKOUT ("front_knockout", voice 3): slides back
   for a while, then gets up ("front_/back_knockout_getup") after +0x10d8 frames */
int CDemolition_Player_StateKnockout(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	float k;

	db_state_begin(s);
	if (entered)
	{
		AI_PlayVoice(s, 3, DAT_00293928, 0.0f, 0, -1, 0);
		Actor_PlayAnim(DB_PTR(s, 0x30), 0x13, 0, 0, DAT_002b05a8, 0.1f, 1);
		DB_I32(s, 0x1120) = 1;
		DB_VFN(s, 0x18, void (*)(void *, int))(s, -1);
	}
	if (DB_F32(s, 0x40) < DAT_002b07bc)
	{
		bmVectorRotateY(DB_FP(s, 0x7c), DAT_0028fdd8, DB_F32(DB_PTR(s, 0x30), 0x1d8));
		k = DAT_002b07c0;
		if (DB_I32(s, 0x1120) != 0)
			k = -k;
		DB_F32(s, 0x7c) = k * DB_F32(s, 0x7c);
		DB_F32(s, 0x80) = k * DB_F32(s, 0x80);
		DB_F32(s, 0x84) = k * DB_F32(s, 0x84);
	}
	else
	{
		DB_F32(s, 0x84) = 0.0f;
		DB_F32(s, 0x80) = 0.0f;
		DB_F32(s, 0x7c) = 0.0f;
	}
	if (DB_I32(s, 0x1120) >= 0 && !(DB_F32(s, 0x40) < DB_F32(s, 0x10d8)))
	{
		Actor_PlayAnim(DB_PTR(s, 0x30), DB_I32(s, 0x1120) != 0 ? 0x15 : 0x16, 0, 0, 1.0f, 0.1f, 1);
		DB_I32(s, 0x1120) = -1;
	}
	return 0;
}

/* 0x000fb9c0  AI state 14 (slot 0xcc): BLOWN AWAY by an explosion: flies with the knock
   velocity (+0x11b4) in "front_knockout" or "back_knockout" (+0x11b0); when the animation
   ended and it is on the ground (or not rising), it gets up (state 15) while in play */
int CDemolition_Player_StateBlownAway(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	char *actor;

	if (entered)
	{
		DB_U32(DB_PTR(s, 0x30), 0x220) |= 0x1000;
		memcpy(DB_ADDR(DB_PTR(s, 0x30), 0x194), s + 0x11b4, 16);
		Actor_PlayAnim(DB_PTR(s, 0x30), DB_U8(s, 0x11b0) != 0 ? 0x13 : 0x14, 0, 0, 1.0f, 0.1f, 1);
	}
	actor = (char *)DB_PTR(s, 0x30);
	if (DB_U16(actor, 0x234) != 0 &&
		(DB_PTR(actor, 0x214) != NULL || DB_F32(actor, 0x198) <= 0.0f))
	{
		if (DB_U8(s, 0x116c) != 0)
			AI_SetState(s, 0xf);
		else
			DB_U32(actor, 0x220) &= 0xffffefff;
	}
	return db_state_end(s);
}

/* 0x000fbaa0  this player hits `victim`: knock back (the character's knockback, double
   with the power-up), its voice, the hit counts, the damage (10000, so it knocks out) and
   the frag statistics of both brains (virtual +0x18 Fragged, +0x1c FraggedBy) */
void CDemolition_Player_HitPlayer(struct CDemolition_Player *self, struct CDemolition_Player *victim)
{
	char *s = (char *)self;
	char *v = (char *)victim;
	char *me = (char *)DB_PTR(s, 0x30);
	char *them = (char *)DB_PTR(v, 0x30);
	float push[4];
	float k;
	void *myBrain;
	void *theirBrain;
	int myIndex;

	push[0] = DB_F32(them, 0xd0) - DB_F32(me, 0xd0);
	push[1] = DB_F32(them, 0xd4) - DB_F32(me, 0xd4);
	push[2] = DB_F32(them, 0xd8) - DB_F32(me, 0xd8);
	push[3] = 0.0f;   /* not set in the original */
	k = DB_CHARACTER_F(db_character(s), 4);
	if (DB_F32(s, 0x10a4) != 0.0f)
		k = k + k;
	bmVectorSetLength(push, push, k);
	them = (char *)DB_PTR(v, 0x30);
	DB_F32(them, 0x194) = push[0] + DB_F32(them, 0x194);
	DB_F32(them, 0x198) = push[1] + DB_F32(them, 0x198);
	DB_F32(them, 0x19c) = push[2] + DB_F32(them, 0x19c);
	DB_U16(s, 0x1086) = DB_U16(DB_PTR(v, 0x30), 0x230);
	AI_PlayVoice(v, 0xc, DAT_00293928, 180.0f, 0, -1, 0);
	myIndex = DB_U16(DB_PTR(s, 0x30), 0x230);
	DB_U16(v, 0x1088) = (unsigned short)myIndex;
	DB_I16(v, 0x108a + myIndex * 2)++;
	CDemolition_Player_Hit(victim, DB_FP(DB_PTR(s, 0x30), 0xd0), 10000.0f, 0);
	DB_I16(v, 0x108a + DB_U16(DB_PTR(s, 0x30), 0x230) * 2)++;
	theirBrain = DB_PTR(v, 0x118c);
	myBrain = DB_PTR(s, 0x118c);
	DB_I32(theirBrain, 0x3c + DB_I32(s, 0x1184) * 4)++;
	DB_VFN(myBrain, 0x18, void (*)(void *, void *))(myBrain, v);
	DB_VFN(theirBrain, 0x1c, void (*)(void *, void *))(theirBrain, s);
}

/* 0x000fbc40  virtual +0x10: the attack lands: on a player standing on this one's head
   (jump attack, state 6), or on a player in front within half the reach. Returns 1 when it
   hit a player. */
int CDemolition_Player_AttackLands(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	char *ground;
	char *other;
	char *target;
	char *actor;
	int i;
	int count;
	int player;
	float heading;
	float turn;
	float distance;
	double reach;
	float forward[4];

	DB_I16(s, 0x1092) = 1;
	if (DB_U16(s, 0x36) == 6)
	{
		count = DB_U8(DB_PTR(DB_PTR(s, 0x30), 0x18c), 0x198);
		for (i = 0; i < count; i++)
		{
			ground = (char *)DB_PTR(DB_PTR(s, 0x30), 0x18c);
			other = (char *)DB_PTR(ground, 0x17c + i * 4);
			if (DB_I32(other, 0x21c) == 1)
			{
				player = DB_U16(other, 0x230);
				target = (char *)(uintptr_t)DAT_002f289c[player];
				if (DB_I16(target, 0x36) != 0xd)
				{
					CLSamples_PlayGeneric((db_rand() % 2) + 0xd, 0x7f, 0, 0, NULL);
					target = (char *)(uintptr_t)DAT_002f289c[player];
					if (DB_I16(target, 0x36) != 0xa && DB_F32(target, 0x10a8) == 0.0f)
					{
						CDemolition_Player_HitPlayer(self, (struct CDemolition_Player *)target);
						return 1;
					}
				}
			}
			count = DB_U8(DB_PTR(DB_PTR(s, 0x30), 0x18c), 0x198);
		}
	}
	for (i = 0; i < DAT_002bae94; i++)
	{
		actor = (char *)DB_PTR(s, 0x30);
		if (i == DB_U16(actor, 0x230))
			continue;
		other = (char *)DB_PTR((char *)(uintptr_t)DAT_002f289c[i], 0x30);
		if (!(DB_F32(s, 0x1100) > fabsf(DB_F32(actor, 0xd4) - DB_F32(other, 0xd4))))
			continue;
		heading = CLMath_HeadingTo(DB_FP(actor, 0xd0), DB_FP(other, 0xd0));
		turn = CLMath_AngleDistance(heading, DB_F32(DB_PTR(s, 0x30), 0x1e8));
		if (!(turn < DB_F32(s, 0x10fc)))
			continue;
		distance = bmVectorDistance(DB_FP(DB_PTR(s, 0x30), 0xd0),
			DB_FP(DB_PTR((char *)(uintptr_t)DAT_002f289c[i], 0x30), 0xd0));
		reach = DB_CHARACTER_F(db_character(s), 0);
		if (DB_F32(s, 0x10a4) != 0.0f)
			reach = reach * (double)DAT_00293a38;
		if (!(reach * 0.5 > ((double)turn + 1.0) * (double)distance))
			continue;
		target = (char *)(uintptr_t)DAT_002f289c[i];
		if (DB_I16(target, 0x36) == 0xd)
			continue;
		CLSamples_PlayGeneric((db_rand() % 4) + 0xb, 0x7f, 0, 0, NULL);
		target = (char *)(uintptr_t)DAT_002f289c[i];
		if (DB_F32(target, 0x10a8) == 0.0f)
		{
			CDemolition_Player_HitPlayer(self, (struct CDemolition_Player *)target);
			return 1;
		}
	}
	/* the original also turns (0, 0, 1) by the yaw into a local it does not use */
	bmVectorRotateY(forward, DAT_0028fdd8, DB_F32(DB_PTR(s, 0x30), 0x1e8));
	return 0;
}

/* 0x000fbf10  auto aim: the nearest other player (not the team mate, not knocked out,
   13..15 and 17) within `maxYaw` of the yaw and `maxPitch` of the pitch; returns its pitch
   and yaw (else the actor's own, +0x1e4 / +0x1e8) */
void CDemolition_Player_AutoAim(struct CDemolition_Player *self, float *pitch, float *yaw,
	float maxPitch, float maxYaw)
{
	char *s = (char *)self;
	char *p;
	int state;
	float bestDist = 99999.0f;   /* 0x47c34f80 */
	float bestYaw = DB_F32(DB_PTR(s, 0x30), 0x1e8);
	float bestPitch = DB_F32(DB_PTR(s, 0x30), 0x1e4);
	float d[3];
	float yawTo;
	float pitchTo;
	float dist;

	for (p = DB_GAME_PLAYERS(); p != NULL; p = DB_PLAYER_NEXT(p))
	{
		if (p == s)
			continue;
		if (DB_PTR(s, 0x1234) != NULL && p == (char *)DB_PTR(s, 0x1234))
			continue;
		state = DB_U16(p, 0x36);
		if ((state >= 0xd && state <= 0xf) || state == 0x11)
			continue;
		d[0] = DB_F32(p, 0x1144) - DB_F32(s, 0x1144);
		d[1] = DB_F32(p, 0x1148) - DB_F32(s, 0x1148);
		d[2] = DB_F32(p, 0x114c) - DB_F32(s, 0x114c);
		yawTo = (float)atan2((double)d[0], (double)d[2]);
		if (!(fabsf(CLMath_AngleDelta(yawTo, DB_F32(DB_PTR(s, 0x30), 0x1e8))) < maxYaw))
			continue;
		dist = sqrtf(d[1] * d[1] + d[0] * d[0] + d[2] * d[2]);
		if (dist > bestDist)
			continue;
		pitchTo = (float)atan2((double)d[1], (double)dist);
		if (!(fabsf(CLMath_AngleDelta(pitchTo, DB_F32(DB_PTR(s, 0x30), 0x1e4))) < maxPitch))
			continue;
		bestDist = dist;
		bestYaw = yawTo;
		bestPitch = pitchTo;
	}
	*pitch = bestPitch;
	*yaw = bestYaw;
}

/* 0x000fc0b0  virtual +0x44: switches between the full model (game +0x80 + 4 * index) and
   the "opt" model (+0x90; not with the debug free camera): recreates the actor in place,
   rebinds the hand nodes and what the player holds, and the colours */
void CDemolition_Player_SetOptModel(struct CDemolition_Player *self, int opt)
{
	char *s = (char *)self;
	char *game;
	void *model;
	float pos[4];
	char *actor;
	int i;

	if (opt == DB_I32(s, 0x119c))
		return;
	DB_I32(s, 0x119c) = opt;
	game = DAT_002ff85c;
	if (opt != 0)
		model = DB_PTR(game, 0x90 + DB_I32(s, 0x1184) * 4);
	else
		model = DB_PTR(game, 0x80 + DB_I32(s, 0x1184) * 4);
	if (DAT_002ba56c != 0)
		model = DB_PTR(game, 0x80 + DB_I32(s, 0x1184) * 4);
	if (model != NULL)
	{
		actor = (char *)DB_PTR(s, 0x30);
		memcpy(pos, actor + 0xd0, 16);
		baDeleteActorInstance((struct BActorInstance *)(actor + 0x10));
		Actor_Create(model, pos, DB_PTR(s, 0x30), 1, 0, 0);
		Actor_PlayAnim(DB_PTR(s, 0x30), 0, 1, 0, 1.0f, 0.0f, 0);
		actor = (char *)DB_PTR(s, 0x30);
		baUpdateAnimations((struct BActorInstance *)(actor + 0x10),
			(struct BActorNode *)DB_PTR(actor, 0x144), 0.0f, NULL);
		baPrepActorInstance((struct BActorInstance *)((char *)DB_PTR(s, 0x30) + 0x10), 1);
		game = DAT_002ff85c;
	}
	if (opt != 0)
		baSetInstanceAnimations((struct BActor *)DB_PTR(game, 0x80 + DB_I32(s, 0x1184) * 4),
			(struct BActorInstance *)((char *)DB_PTR(s, 0x30) + 0x10));
	for (i = 0; i < 3; i++)
		DB_PTR(s, 0x1238 + i * 4) = baFindNode(
			(struct BActorNode *)DB_PTR(DB_PTR(s, 0x30), 0x144), DAT_00298868[i]);
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
		if (DB_U8(s, 0x1244) != 0)
		{
			DB_PTR(s, 0x124c) = DB_PTR(s, 0x1238);
			DB_PTR(s, 0x1248) = DB_PTR(DB_PTR(s, 0x30), 0x300);
			DB_U8(s, 0x1244) = 1;
			CDemolition_FragBomb_PickUp((struct CDemolition_FragBomb *)DB_PTR(s, 0x11a0));
		}
		break;
	case 1:
		/* CDemolition_Player_SetController, inlined */
		DB_U8(s, 0x1250) = 1;
		if (DB_PTR(DB_PTR(s, 0x30), 0x30c) != NULL)
		{
			DB_U8(s, 0x1251) = 1;
			DB_PTR(s, 0x125c) = DB_PTR(s, 0x1238);
			DB_PTR(s, 0x1258) = DB_PTR(DB_PTR(s, 0x30), 0x30c);
		}
		DB_PTR(s, 0x1254) = s + 0x1270;
		DB_PTR(s, 0x1260) = DB_PTR(s, 0x1240);
		break;
	case 3:
		if (DB_U8(s, 0x1244) != 0)
		{
			DB_PTR(s, 0x124c) = DB_PTR(s, 0x1238);
			DB_PTR(s, 0x1248) = DB_PTR(DB_PTR(s, 0x30), 0x300);
			DB_U8(s, 0x1244) = 1;
		}
		break;
	}
	Core_SetCharacterColours((char *)DB_PTR(s, 0x30) + 0x10, DAT_00283ea4);
}

/* 0x000fc2e0  handles one message of the event queue (+0x1130: +0x1134 entries of 0x24
   bytes, +0x1138 capacity, +0x113a head, +0x113c count):
     type 0, an explosion hit (DbMessage: +0x10 position, +0x20 power): blown away (state
       14) from the explosion with a knock velocity of (40, 120) * (1 - distance / power),
       facing it (+0x11b0: from behind), drops/detonates what it holds, a voice and a
       rumble of distance / power
     type 1, an animation (+0x11 loop, +0x12 queue, +0x14 index, +0x18 blend)
   (`dt` is not used) */
void CDemolition_Player_HandleMessage(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;
	char *entry;
	float d[4];
	float n[4];
	float len;
	float inv;
	float a;
	float b;
	float yawAway;
	unsigned short head;

	(void)dt;
	if (DB_U16(s, 0x113c) == 0)
		return;
	head = DB_U16(s, 0x113a);
	entry = (char *)DB_PTR(s, 0x1134) + head * 0x24;
	switch (DB_U16(entry, 4))
	{
	case 0:
		entry = (char *)DB_PTR(s, 0x1134) + DB_U16(s, 0x113a) * 0x24;
		d[0] = DB_F32(s, 0x1144) - DB_F32(entry, 0x10);
		d[1] = DB_F32(s, 0x1148) - DB_F32(entry, 0x14);
		d[2] = DB_F32(s, 0x114c) - DB_F32(entry, 0x18);
		d[3] = 0.0f;   /* not set in the original */
		bmVectorNormalise(n, d);
		entry = (char *)DB_PTR(s, 0x1134) + DB_U16(s, 0x113a) * 0x24;
		len = sqrtf(d[1] * d[1] + d[0] * d[0] + d[2] * d[2]);
		inv = 1.0f / DB_F32(entry, 0x20);
		a = 40.0f - len * 40.0f * inv;
		b = 120.0f - len * 120.0f * inv;
		DB_F32(s, 0x11b4) = n[0] * a;
		DB_F32(s, 0x11b8) = b;
		DB_F32(s, 0x11bc) = n[2] * a;
		yawAway = (float)atan2((double)d[0], (double)d[2]);
		if (fabsf(CLMath_AngleDelta(yawAway, DB_F32(DB_PTR(s, 0x30), 0x1e8))) < 1.5707964f)
		{
			DB_U8(s, 0x11b0) = 0;
			DB_F32(DB_PTR(s, 0x30), 0x1d8) = yawAway;
		}
		else
		{
			DB_U8(s, 0x11b0) = 1;
			DB_F32(DB_PTR(s, 0x30), 0x1d8) = (float)((double)yawAway + (double)3.1415927f);
		}
		switch (DB_I32(DAT_002ff85c, 0x28))
		{
		case 0:
			CDemolition_Player_DetonateFrag(self);
			break;
		case 1:
			CDemolition_Player_HideControllerAnim(self);
			break;
		case 3:
			CDemolition_Player_DetonateCross(self);
			break;
		}
		CLSamples_PlayFromSet((char)DB_U8(s, 0x1188), 2, DB_FP(s, 0x1144), DAT_00293928, 0, -1, 0);
		AI_SetState(s, 0xe);
		CDemoRumble_Shake(DAT_002ff85c + 0x7a38, DB_I32(s, 0x1184),
			sqrtf(d[0] * d[0] + d[2] * d[2]) * inv);
		break;
	case 1:
		Actor_PlayAnim(DB_PTR(s, 0x30), DB_I32(entry, 0x14), DB_U8(entry, 0x11), DB_U8(entry, 0x12),
			1.0f, DB_F32(entry, 0x18), 1);
		break;
	default:
		DB_U16(s, 0x113a) = (unsigned short)((head + 1) & (DB_U16(s, 0x1138) - 1));
		DB_U16(s, 0x113c)--;
		return;
	}
	if (DB_U16(s, 0x113c) != 0)
	{
		DB_U16(s, 0x113a) = (unsigned short)((DB_U16(s, 0x113a) + 1) & (DB_U16(s, 0x1138) - 1));
		DB_U16(s, 0x113c)--;
	}
}

/* 0x000fc5d0  DrawFx (virtual +0xec): the HUD ring under the player (classlib_ui), with an
   arrow 100 units ahead: in variation 0 under control towards the auto-aim target, in
   variation 3 (and variation 0 without control) the way it faces; none in 1 and 2 */
void CDemolition_Player_DrawFx(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	float pitch;
	float yaw;
	float v[4];
	float out[4];
	float target[4];
	int variation;

	if (DB_U8(s, 0x116c) == 0)
		return;
	variation = DB_I32(DAT_002ff85c, 0x28);
	if (variation != 0 && variation != 3)
	{
		CLHud_DrawAIRing(DB_I32(s, 0x1184), DB_FP(s, 0x1144), NULL);
		return;
	}
	if (variation == 0 && DB_U8(s, 0x1174) != 0)
		CDemolition_Player_AutoAim(self, &pitch, &yaw, db_bitsf(0x3f860a92), db_bitsf(0x3f060a92));
	else
		yaw = DB_F32(DB_PTR(s, 0x30), 0x1e8);
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 100.0f;
	v[3] = 1.0f;
	bmVectorRotateY(out, v, yaw);
	target[0] = out[0] + DB_F32(s, 0x1144);
	target[1] = out[1] + DB_F32(s, 0x1148);
	target[2] = out[2] + DB_F32(s, 0x114c);
	target[3] = out[3];
	CLHud_DrawAIRing(DB_I32(s, 0x1184), DB_FP(s, 0x1144), target);
}

/* 0x000fc6d0  variation 0: throws the frag bomb it holds (+0x11a8) at the auto-aim target
   (30 degrees above its pitch) at 5 + 100 * the charge (+0x11c8) */
void CDemolition_Player_ThrowFrag(struct CDemolition_Player *self)
{
	char *s = (char *)self;
	char *bomb = (char *)DB_PTR(s, 0x11a8);
	float speed;
	float pitch;
	float yaw;
	float v[4];
	float dir[4];
	float velocity[4];

	if (bomb == NULL)
		return;
	if (DB_U8(s, 0x1244) != 0)
	{
		db_release_node(s, DB_PTR(s, 0x124c));
		DB_U8(s, 0x1244) = 0;
	}
	speed = DB_F32(s, 0x11c8) * 100.0f + 5.0f;
	CDemolition_Player_AutoAim(self, &pitch, &yaw, db_bitsf(0x3f860a92), db_bitsf(0x3f060a92));
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 100.0f;
	v[3] = 1.0f;
	bmVectorRotateXY(dir, v, -pitch - 0.52359879f, yaw);
	bmVectorNormalise(velocity, dir);
	velocity[0] = velocity[0] * speed;
	velocity[1] = velocity[1] * speed;
	velocity[2] = velocity[2] * speed;
	CDemolition_FragBomb_Throw((struct CDemolition_FragBomb *)bomb, velocity);
}

/* 0x000fc7e0  Update (virtual +0xe4): position; while active (+0x116d) the hit cooldown,
   the input (cleared without control; CPU players are driven by their brain, virtual
   +4), the variation's update and the CharLib AI; then the messages, the actor and its
   animations (turned to the yaw), the controller, the previous position (the axes the
   variation tracks) and the debris colour of the player */
void CDemolition_Player_Update(struct CDemolition_Player *self, float dt)
{
	char *s = (char *)self;
	char *actor;
	float axisAngle[4];

	memcpy(s + 0x1144, DB_ADDR(DB_PTR(s, 0x30), 0xd0), 16);
	if (DB_U8(s, 0x116d) != 0)
	{
		DB_F32(s, 0x1198) = DB_F32(s, 0x1198) - dt;
		if (DB_F32(s, 0x1198) < 0.0f)
			DB_F32(s, 0x1198) = 0.0f;
		if (DB_U8(s, 0x1174) == 0)
			biClearMap(DB_PTR(s, 0x5c));
		if (DB_U8(s, 0x1171) != 0)
		{
			biClearMap(DB_PTR(s, 0x5c));
			if (DB_U8(s, 0x1171) != 0 && DB_U8(s, 0x1173) != 0)
				DB_VFN(DB_PTR(s, 0x118c), 4, void (*)(void *, float))(DB_PTR(s, 0x118c), dt);
		}
		switch (DB_I32(DAT_002ff85c, 0x28))
		{
		case 0:
			CDemolition_Player_UpdateFrag(self, dt);
			break;
		case 1:
			CDemolition_Player_UpdateRemoteVariation(self, dt);
			break;
		case 2:
			CDemolition_Player_CheckFall(self, dt);
			break;
		case 3:
			CDemolition_Player_UpdateCross(self, dt);
			break;
		}
		AI_Tick(s);
	}
	CDemolition_Player_HandleMessage(self, dt);
	Actor_Update(DB_PTR(s, 0x30));
	CDemolition_Player_PlayHoldAnims(self, dt);
	actor = (char *)DB_PTR(s, 0x30);
	baUpdateAnimations((struct BActorInstance *)(actor + 0x10),
		(struct BActorNode *)DB_PTR(actor, 0x144),
		DAT_0028fda8 * DB_F32(actor, 0x23c) * DAT_00293920, NULL);
	actor = (char *)DB_PTR(s, 0x30);
	axisAngle[0] = 0.0f;
	axisAngle[1] = 1.0f;
	axisAngle[2] = 0.0f;
	axisAngle[3] = -(DB_F32(actor, 0x1e8) + 3.1415927f);
	bmAxisAngleToQuaternion(DB_FP(actor, 0xf0), axisAngle);
	baPrepActorInstance((struct BActorInstance *)((char *)DB_PTR(s, 0x30) + 0x10), 1);
	CDemolition_Player_PlaceController(self, dt);
	memcpy(s + 0x1144, DB_ADDR(DB_PTR(s, 0x30), 0xd0), 16);
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
	case 2:
	case 3:
		DB_F32(s, 0x1158) = DB_F32(s, 0x1148);
		DB_F32(s, 0x115c) = DB_F32(s, 0x114c);
		DB_F32(s, 0x1154) = DB_F32(s, 0x1144);
		break;
	case 1:
		DB_F32(s, 0x115c) = DB_F32(s, 0x114c);
		DB_F32(s, 0x1154) = DB_F32(s, 0x1144);
		break;
	}
	switch (DB_I32(s, 0x1184))
	{
	case 0:
		DebrisColours_Set0(DB_FP(s, 0x1144));
		break;
	case 1:
		DebrisColours_Set1(DB_FP(s, 0x1144));
		break;
	case 2:
		DebrisColours_Set2(DB_FP(s, 0x1144));
		break;
	case 3:
		DebrisColours_Set3(DB_FP(s, 0x1144));
		break;
	}
}

/* 0x000fca70  AI state 16 (slot 0xd4): THROW ("throw", then "throw_release" when it
   ends, which throws the frag (variation 0) or cross bomb (variation 3)); then STAND */
int CDemolition_Player_StateThrow(struct CDemolition_Player *self, int entered)
{
	char *s = (char *)self;
	int variation;

	db_state_begin(s);
	if (entered)
	{
		DB_U8(s, 0x11c4) = 1;
		Actor_PlayAnim(DB_PTR(s, 0x30), 0x28, 0, 0, 1.0f, 0.1f, 1);
	}
	if (DB_U16(DB_PTR(s, 0x30), 0x234) != 0)
	{
		if (DB_U8(s, 0x11c4) != 0)
		{
			DB_U8(s, 0x11c4) = 0;
			Actor_PlayAnim(DB_PTR(s, 0x30), 0x29, 0, 0, 1.0f, 0.1f, 1);
			variation = DB_I32(DAT_002ff85c, 0x28);
			if (variation == 0)
				CDemolition_Player_ThrowFrag(self);
			else if (variation == 3)
				CDemolition_Player_ThrowCross(self);
		}
		else
			AI_SetState(s, 0);
	}
	return db_state_end(s);
}
