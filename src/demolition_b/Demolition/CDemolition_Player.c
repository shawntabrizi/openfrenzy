/*
Demolition/CDemolition_Player.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Player.cpp

CDemolition_Player (0x13e0 bytes, vtable 0x0026b448, message receiver vtable 0x0026b440 at
+0x1130): a Demolition player, a CharLib AI character (CDemolition_CharLib, charlib_ai.h).
The functions from 0x000f9740 are tagged with this file; 0x000f92b0-0x000f94c0 are the gap
functions before them (constructor, destructor, actor and input), "file inferred".

Its CharLib AI tables (0x00298820 states, 0x002987d8 conditions, 0x00298690 priorities) go
through demolition_d's thunks to the virtual slots: the conditions 0..17 are slots
0x4c..0x90, the states 0..17 slots 0x94..0xd8 (`int state(self, entered)`).

  condition 0 (0x4c) 0x000f9e40 Stop        state 0  (0x94) 0x000faec0 Idle
  condition 2 (0x54) 0x000f9ee0 Move        state 1  (0x98) 0x000fafe0 Walk
  condition 3 (0x58) 0x000f9f70 Jump        state 2  (0x9c) 0x000fb080 Run
  condition 4 (0x5c) 0x000f9fb0 Rise        state 3  (0xa0) 0x000fb270 Jump
  condition 5 (0x60) 0x000fac10 Fall        state 4  (0xa4) 0x000fa070 Land
  condition 6 (0x64) 0x000f9fe0 Action      state 5  (0xa8) 0x000fb3e0 Fall
  condition 9 (0x70) 0x000fa010 Landed      state 6  (0xac) 0x000fb630 Air
                                            state 9  (0xb8) 0x000fb820 Throw
                                            state 10 (0xbc) 0x000fa0e0 Hurt
                                            state 11 (0xc0) 0x000fa160 Attack
                                            state 13 (0xc8) 0x000fb890 Knocked out
                                            state 14 (0xcc) 0x000fb9c0 Down
                                            state 15 (0xd0) 0x000fa1e0 Get up
                                            state 16 (0xd4) 0x000fca70 Celebrate
                                            state 17 (0xd8) 0x000fa290 Victory
  (The state names follow the animations they play; 7, 8 and 12 are 0x000f8ab0.)
  other slots: +0x10 0x000fbc40, +0x44 0x000fc0b0 SetActive, +0x48 deleting destructor,
  +0xdc 0x000fa530 Init(index), +0xe0 0x000fa6a0 Shutdown, +0xe4 0x000fc7e0 Update(dt),
  +0xe8 0x000fa720 Draw, +0xec 0x000fc5d0 DrawFx, +0xf0 0x000f9740 Reset

Player fields (besides charlib_ai.h's):
  +0x1140 next player   +0x1144 position   +0x1154 previous position   +0x1164 radius (50)
  +0x1168 height (177)  +0x116c in play   +0x116d (remote bomb) armed   +0x116e visible
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
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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

/* 0x000f9e40  AI condition 0 (slot 0x4c): stop. In states 2 and 13..17 (and 3..12, 1): see
   the cases; true when the stick is released (+0x54 < +0x10e8), or for variations 0 and 3
   while charging a throw */
int CDemolition_Player_CondStop(struct CDemolition_Player *self)
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
		/* knocked out: when the actor landed (+0x234) and the knockout is not from behind */
		if (DB_U16(DB_PTR(s, 0x30), 0x234) != 0 && DB_I32(s, 0x1120) < 0)
			return 1;
		return 0;
	case 14:
	case 15:
	case 16:
	case 17:
		return 0;
	default:
		/* 0, 1, 3..12 (and others): when the actor stands (+0x234) */
		return DB_U16(DB_PTR(s, 0x30), 0x234) != 0;
	}
}

/* 0x000f9ee0  AI condition 2 (slot 0x54): move (stick past the walk threshold) */
int CDemolition_Player_CondMove(struct CDemolition_Player *self)
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

/* 0x000f9f70  AI condition 3 (slot 0x58): jump (action1, standing; not in variations 1 and
   3, not knocked out) */
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
int CDemolition_Player_CondGrounded(struct CDemolition_Player *self)
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
int CDemolition_Player_CondFalling(struct CDemolition_Player *self)
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

/* 0x000fa160  AI state 11 (slot 0xc0): attack (voice 11 with a 180 frame cooldown,
   "attack_1") */
int CDemolition_Player_StateAttack(struct CDemolition_Player *self, int entered)
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

/* 0x000fa1e0  AI state 15 (slot 0xd0): get up ("front_knockout_getup" or
   "back_knockout_getup" by +0x11b0); when the actor stands again: idle, and in variation 1
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
