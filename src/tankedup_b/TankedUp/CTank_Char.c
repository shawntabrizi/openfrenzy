/*
TankedUp/CTank_Char.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Char.cpp

The characters of TankedUp (CTank_Game +0xfd0, 0x1184 bytes): each player's character
riding its tank (full and low detail) and its shield. 0x001d5b80 is tagged with
CTank_Char.cpp; the other methods around it work on the same object (file inferred).

Layout (BActorInstance = 0x170 bytes, root node at +0x134, position at +0xc0, scale at
+0xd0, rotation quaternion at +0xe0):
  +0x0000 the 4 character models      +0x0010 + i * 0x170 the 4 character instances
  +0x05d0 the 4 low-detail models     +0x05e0 + i * 0x170 the 4 low-detail instances
  +0x0ba0 the 4 shield models         +0x0bb0 + i * 0x170 the 4 shield instances
  +0x1170 the players (tankedup_a)    +0x1174 the lights (tankedup_a)
  +0x1178 the game state              +0x117c, +0x1180 the shield texture scroll (0-1)
*/

#include "tankedup_b.h"
#include "tankedup_b_data.h"
#include "tankedup_b_ext.h"

#define CH_MODEL(i) (0x0000 + (i) * 4)
#define CH_INST(i) (0x0010 + (i) * 0x170)
#define CH_LOW_MODEL(i) (0x05d0 + (i) * 4)
#define CH_LOW_INST(i) (0x05e0 + (i) * 0x170)
#define CH_SHIELD_MODEL(i) (0x0ba0 + (i) * 4)
#define CH_SHIELD_INST(i) (0x0bb0 + (i) * 0x170)
#define CH_PLAYERS 0x1170
#define CH_LIGHTS 0x1174
#define CH_STATE 0x1178
#define CH_SCROLL_U 0x117c
#define CH_SCROLL_V 0x1180

#define CH_ROOT(inst) ((struct BActorNode *)TU_PTR(inst, 0x134))

/* the players' methods (CTank_Players, tankedup_a; thiscall) */
#define TA_PlayerIsFirst(players, i) TU_CALL(FUN_001c83d0, int (*)(void *, int))(players, i)
#define TA_PlayerIsAlive(players, i) TU_CALL(FUN_001c83b0, int (*)(void *, int))(players, i)
#define TA_PlayerCanAct(players, i) TU_CALL(FUN_001c8490, int (*)(void *, int))(players, i)
#define TA_PlayerTankPose(players, i, pos, rot) \
	TU_CALL(FUN_001c8370, void (*)(void *, int, float *, float *))(players, i, pos, rot)

/* 0x001d5af0  TankedUp (gap; CTank_Char.cpp inferred)
CTank_Char constructor. */
void *TankChar_Ctor(void *self)
{
	TU_I32(self, CH_STATE) = 1;
	return self;
}

/* 0x001d5b00  TankedUp (gap; CTank_Char.cpp inferred)
CTank_Char::PlayAnim: loops animation `name` on player `player`'s character (full and
low detail). */
void TankChar_PlayAnim(void *self, int player, const char *name)
{
	struct BActorInstance *inst;
	void *anim;

	inst = (struct BActorInstance *)TU_ADDR(self, CH_INST(player));
	anim = baFindSubObject(inst, name);
	if (anim != NULL)
		baQueueAnimation(inst, CH_ROOT(inst), (struct BAnim *)anim, 0x400, -1, 1, NULL, NULL);
	inst = (struct BActorInstance *)TU_ADDR(self, CH_LOW_INST(player));
	anim = baFindSubObject(inst, name);
	if (anim != NULL)
		baQueueAnimation(inst, CH_ROOT(inst), (struct BAnim *)anim, 0x400, -1, 1, NULL, NULL);
}

/* load a model whose name has "<playerN>" colour tags */
static void *tu_load_char_model(char *name)
{
	TankChar_SubstituteColours(name);
	return bLoadResource(NULL, name, 1, 0xdefa);
}

/* 0x001d5b80  TankedUp/CTank_Char.cpp
CTank_Char::Initialise: the shield, character and low-detail character of each player
(the characters at 0.7 scale, idle). */
int TankChar_Initialise(void *self, void *players, void *lights)
{
	char name[64];
	struct BActorInstance *inst;
	void *anim;
	int i;

	TU_PTR(self, CH_PLAYERS) = players;
	TU_I32(self, CH_STATE) = 1;
	TU_PTR(self, CH_LIGHTS) = lights;
	TU_F32(self, CH_SCROLL_V) = 0.0f;
	TU_F32(self, CH_SCROLL_U) = 0.0f;
	for (i = 0; i < 4; i++) {
		sprintf(name, "entities\\shield_b.obe", i + 1);
		TU_PTR(self, CH_SHIELD_MODEL(i)) = tu_load_char_model(name);
		inst = (struct BActorInstance *)TU_ADDR(self, CH_SHIELD_INST(i));
		baInitActorInstance(inst, (struct BActor *)TU_PTR(self, CH_SHIELD_MODEL(i)), 2, TU_FILE_CHAR, 0x37);
		baPrepActorInstance(inst, 1);
		baSetNodeTime(inst, CH_ROOT(inst), 0, 1);
		anim = baFindSubObject(inst, "rotate");
		if (anim != NULL)
			baQueueAnimation(inst, CH_ROOT(inst), (struct BAnim *)anim, 0x400, -1, 1, NULL, NULL);

		sprintf(name, "chars\\player-<player%d>.obe", i + 1);
		TU_PTR(self, CH_MODEL(i)) = tu_load_char_model(name);
		inst = (struct BActorInstance *)TU_ADDR(self, CH_INST(i));
		baInitActorInstance(inst, (struct BActor *)TU_PTR(self, CH_MODEL(i)), 8, TU_FILE_CHAR, 0x41);
		TU_F32(inst, 0xd8) = 0.7f;
		TU_F32(inst, 0xd4) = 0.7f;
		TU_F32(inst, 0xd0) = 0.7f;
		baPrepActorInstance(inst, 1);
		baSetNodeTime(inst, CH_ROOT(inst), 0, 1);

		sprintf(name, "chars\\low\\low-player-<player%d>.obe", i + 1);
		TU_PTR(self, CH_LOW_MODEL(i)) = tu_load_char_model(name);
		inst = (struct BActorInstance *)TU_ADDR(self, CH_LOW_INST(i));
		baInitActorInstance(inst, (struct BActor *)TU_PTR(self, CH_LOW_MODEL(i)), 8, TU_FILE_CHAR, 0x49);
		/* the low-detail character uses the full one's animations */
		baSetInstanceAnimations((struct BActor *)TU_PTR(self, CH_MODEL(i)), inst);
		TU_F32(inst, 0xd8) = 0.7f;
		TU_F32(inst, 0xd4) = 0.7f;
		TU_F32(inst, 0xd0) = 0.7f;
		baPrepActorInstance(inst, 1);
		baSetNodeTime(inst, CH_ROOT(inst), 0, 1);
		TankChar_PlayAnim(self, i, "idle"); /* 0x00263714 */
		baUpdateAnimations(inst, CH_ROOT(inst), 1.0f, NULL);
	}
	return 1;
}

/* 0x001d5da0  TankedUp (gap; CTank_Char.cpp inferred)
CTank_Char::Free: the character, low-detail and shield instances. */
void TankChar_Free(void *self)
{
	int i;

	for (i = 0; i < 4; i++) {
		baDeleteActorInstance((struct BActorInstance *)TU_ADDR(self, CH_INST(i)));
		baDeleteActorInstance((struct BActorInstance *)TU_ADDR(self, CH_LOW_INST(i)));
		baDeleteActorInstance((struct BActorInstance *)TU_ADDR(self, CH_SHIELD_INST(i)));
	}
}

/* plays a start animation once, then loops another */
static void tu_play_end(struct BActorInstance *inst, const char *start, const char *loop)
{
	void *anim;

	baFlushAnimations(inst, CH_ROOT(inst), 1, 0);
	anim = baFindSubObject(inst, start);
	if (anim != NULL)
		baQueueAnimation(inst, CH_ROOT(inst), (struct BAnim *)anim, 0x400, -1, 0, NULL, NULL);
	anim = baFindSubObject(inst, loop);
	if (anim != NULL)
		baQueueAnimation(inst, CH_ROOT(inst), (struct BAnim *)anim, 0x400, -1, 1, NULL, NULL);
}

/* 0x001d5de0  TankedUp (gap; CTank_Char.cpp inferred)
CTank_Char::StateChange: at the end (state 5) the winners celebrate and the others sulk. */
void TankChar_StateChange(void *self, int from, int to)
{
	struct BActorInstance *inst;
	int i;

	(void)from;
	if (to != 5)
		return;
	for (i = 0; i < 4; i++) {
		inst = (struct BActorInstance *)TU_ADDR(self, CH_INST(i));
		if (TA_PlayerIsFirst(TU_PTR(self, CH_PLAYERS), i) == 0)
			tu_play_end(inst, "losestart", "loseloop");
		else
			tu_play_end(inst, "victory_start", "victory_loop");
	}
}

/* 0x001d5ee0  TankedUp (gap; CTank_Char.cpp inferred)
CTank_Char::Update: scrolls the shield texture, puts every character and shield on its
player's tank and animates the characters (always after the game, else only in
single-human mode). Returns `state`. */
int TankChar_Update(void *self, float dt, int state)
{
	char *inst;
	float pos[4];
	float rot[4];
	float m[16];
	float f;
	int player;
	int i;

	TU_I32(self, CH_STATE) = state;
	f = DAT_0028fda8 * 0.00400000019f + TU_F32(self, CH_SCROLL_U); /* 0x00255074 */
	TU_F32(self, CH_SCROLL_U) = f;
	if (1.0f <= f)
		TU_F32(self, CH_SCROLL_U) = f - 1.0f;
	f = DAT_0028fda8 * 0.00999999978f + TU_F32(self, CH_SCROLL_V); /* 0x0025372c */
	TU_F32(self, CH_SCROLL_V) = f;
	if (1.0f <= f)
		TU_F32(self, CH_SCROLL_V) = f - 1.0f;
	for (i = 0; i < 4; i++) {
		TA_PlayerTankPose(TU_PTR(self, CH_PLAYERS), i, pos, rot);
		f = rot[1] + 3.1415925f; /* 0x00256750 */
		inst = (char *)TU_ADDR(self, CH_INST(i));
		memcpy(inst + 0xc0, pos, 16);
		memcpy((char *)TU_ADDR(self, CH_LOW_INST(i)) + 0xc0, pos, 16);
		memcpy((char *)TU_ADDR(self, CH_SHIELD_INST(i)) + 0xc0, pos, 16);
		bmMatrixRotationYXZ(m, rot[0], f, rot[2]);
		bmMatrixToQuaternion(TU_FP(TU_ADDR(self, CH_INST(i)), 0xe0), m);
		bmMatrixToQuaternion(TU_FP(TU_ADDR(self, CH_LOW_INST(i)), 0xe0), m);
		bmMatrixToQuaternion(TU_FP(TU_ADDR(self, CH_SHIELD_INST(i)), 0xe0), m);
		if (state > 4 || TankChar_SingleHumanMode(&player) != 0)
			baUpdateAnimations((struct BActorInstance *)inst, CH_ROOT(inst), dt, NULL);
	}
	return state;
}

/* 0x001d6090  TankedUp (gap; CTank_Char.cpp inferred)
CTank_Char::Draw: the characters of the living players (low detail while playing, except
the debug switch +0xf4 of CTank_Debug and single-human mode, where the human's own
character is hidden), then the shields of the players that can act, with the scrolling
texture matrix. */
void TankChar_Draw(void *self)
{
	struct BActorInstance *inst;
	float m[16];
	int human;
	int i;

	bdSetRenderState(3, 1, 0);
	bdSetRenderState(4, 0, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	baSetBrightness(0x100);
	bdSetWorldMatrix(DAT_002855f0);
	for (i = 0; i < 4; i++) {
		if (TA_PlayerIsAlive(TU_PTR(self, CH_PLAYERS), i) == 0)
			continue;
		inst = (struct BActorInstance *)TU_ADDR(self, CH_INST(i));
		TA_LightsEnableTank(TU_PTR(self, CH_LIGHTS), 0);
		human = -1;
		if (TU_I32(self, CH_STATE) < 5 && DAT_003151a4 == 0 && TankChar_SingleHumanMode(&human) == 0) {
			inst = (struct BActorInstance *)TU_ADDR(self, CH_LOW_INST(i));
			baPrepActorInstance(inst, 1);
			baDrawActorInstance(inst, 7);
		} else {
			baPrepActorInstance(inst, 1);
			if (human != i)
				baDrawActorInstance(inst, 7);
		}
		TA_LightsEnableTank(TU_PTR(self, CH_LIGHTS), 1);
	}
	memcpy(m, DAT_002855f0, sizeof m);
	m[8] = TU_F32(self, CH_SCROLL_U) * 0.5f;
	m[9] = TU_F32(self, CH_SCROLL_V) * 0.5f;
	m[0] = 0.5f;
	m[4] = 0.0f;
	m[1] = 0.0f;
	m[5] = 0.5f;
	m[2] = 0.0f;
	m[6] = 0.0f;
	m[10] = 1.0f;
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(2, 3, 0);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)9, 2);
	D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)2, (const D3DMATRIX *)m);
	for (i = 0; i < 4; i++) {
		if (TA_PlayerCanAct(TU_PTR(self, CH_PLAYERS), i) == 0)
			continue;
		inst = (struct BActorInstance *)TU_ADDR(self, CH_SHIELD_INST(i));
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 1, 0);
		baPrepActorInstance(inst, 1);
		baDrawActorInstance(inst, 7);
		bdSetRenderState(6, 0, 0);
		bdSetRenderState(10, 0, 0);
	}
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)9, 0);
	bdSetRenderState(3, 1, 0);
}
