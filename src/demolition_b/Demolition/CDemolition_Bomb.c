/*
Demolition/CDemolition_Bomb.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Bomb.cpp

The bombs of the four Demolition variations and the bomb systems that own them. The game
(DAT_002ff85c) keeps the bomb system of the current variation at +0x48 (and +0x13e8 for the
explosions); the explosion manager is at game +0x13f0.

  variation 0  CDemolition_BombSystemVariationFrag    (vtable 0x0026af48): a list of frag
               bombs (CDemolition_FragBomb, vtable 0x0026ae18) the players pick up and throw
  variation 1  ...VariationRemote (0x0026af6c): one remote bomb per player
               (CDemolition_RemoteBomb, a CharLib character, vtable 0x0026ae30)
  variation 2  ...VariationTimed (0x0026af90): 8 timed bombs of 0x2b0 bytes
               (CDemolition_TimedBomb, vtable 0x0026af2c) inside the system
  variation 3  ...VariationCross (0x0026afb4): a list of cross bombs
               (CDemolition_CrossBomb, vtable 0x0026af30)

The bomb objects derive from CDemolition_Object (vtable 0x0026b0d4, Demolition/unplaced.c),
a demolition_d physics object (0x0010bd30) of 0x43a0 bytes plus a frame at +0x43a0
(position +0x43a4, velocity +0x43c4, orientation +0x43f4).

Bomb object fields (frag and cross bombs):
  +0x10 next   +0x15 active   +0x16 visible   +0x17 frozen   +0x1c moving
  +0x439c, +0x4410 radius (40.5)   +0x4450 type   +0x4454 state (0 idle, 1 held, 2 thrown)
  +0x4460 the bomb model (BActorInstance)   +0x45d0 the player holding it
  frag bomb: +0x4610 effects (+0x10 "bombstate" node, +0x18 fuse timer, +0x50 sparks)
  cross bomb: +0x45d8 effects (+0x18 timer, +0x50 sparks)
  effects: +0x00 position (x, y, z, w), +0xd6/+0xd8/+0xda glow colour (shorts)

The remote bomb is a CharLib AI character (charlib_ai.h): two AI states (0 idle: 0x000f18b0,
1 moving: 0x000f2ea0) and its own fields from +0x1140 like the players (position +0x1144,
+0x116c..+0x116e flags, colour +0x1178, index +0x1184, phase +0x1190 (0 armed, 1 growing,
2 shrinking), cooldown +0x1198, holder +0x11d8, "bombstate" node +0x11dc, two glow sprites
+0x11e0 (0x14 bytes each), node position +0x1208, timer +0x1220, scale +0x1258).
*/

#include "demolition_b.h"
#include "demolition_b_internal.h"

/* ---------- calls with a float that the table prototype hides */

/* 0x0018e450 (rollerball_a)  plays sound `id` of the game's sound bank (game +0x79d0) */
static void db_sound(int id, int volume)
{
	FUN_0018e450((int)(DAT_002ff85c + 0x79d0), id, volume, 0, 0, 0);
}

/* 0x000ebac0 (demolition_a)  rumbles pad `player` with `strength` (0..1) (game +0x7a38) */
static void db_rumble(int player, float strength)
{
	DB_DUAL(FUN_000ebac0, void (*)(int, int), void (*)(void *, int, float),
		((int)(DAT_002ff85c + 0x7a38), player),
		(DAT_002ff85c + 0x7a38, player, strength));
}

/* 0x00102120 (demolition_c)  the world's free spot test: moves `pos` to free ground within
   `radius`; nonzero when it found one */
static int db_world_find_spot(void *world, float *pos, float radius)
{
	return DB_DUAL(FUN_00102120, int (*)(int, undefined4 *, undefined4, float *),
		int (*)(void *, float *, float, float *),
		((int)world, (undefined4 *)pos, db_fbits(radius), NULL), (world, pos, radius, NULL));
}

/* 0x000ed620 (demolition_a)  update of the object at game +0x77d4 (its table prototype has no
   parameters; the original passes `this` and dt) */
static void db_game_77d4_update(float dt)
{
	DB_DUAL(FUN_000ed620, void (*)(void), void (*)(void *, float), (),
		(DAT_002ff85c + 0x77d4, dt));
}

/* the explosion manager (game +0x13f0) starts an explosion: 0.3 s, power 500 */
static void db_explode_at(void *source, void *owner, float *pos)
{
	CDemolition_ExplosionMgr_Start((struct CDemolition_ExplosionMgr *)(DAT_002ff85c + 0x13f0),
		source, owner, pos, 0.3f, 500.0f);
}

/* ====================================================================================
   CDemolition_FragBomb (vtable 0x0026ae18)
   ==================================================================================== */

/* 0x000f0f30  Demolition (file inferred: the destructor of CDemolition_FragBomb, whose
   constructor 0x000f2200 is in CDemolition_Bomb.cpp) */
void CDemolition_FragBomb_Destruct(struct CDemolition_FragBomb *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026ae18;
	CDemolition_BombFx_Destruct((struct CDemolition_BombFx *)(s + 0x4610));
	FUN_000ec300((undefined4 *)(s + 0x45d8));
	CDemolition_Object_Destruct((struct CDemolition_Object *)s);
}

/* 0x000f0fa0  Demolition/CDemolition_Bomb.cpp  (vtable 0x0026ae18 slot 1)
   Frees the bomb model and stops the sparks. */
void CDemolition_FragBomb_Release(struct CDemolition_FragBomb *self)
{
	char *s = (char *)self;

	baDeleteActorInstance((struct BActorInstance *)(s + 0x4460));
	DB_VFN(s + 0x4660, 0, void (*)(void *))(s + 0x4660);
}

/* 0x000f0fd0  Demolition/CDemolition_Bomb.cpp  (vtable 0x0026ae18 slot 4)
   Resets the bomb: inactive, at (0, -2, 0), physics constants, then the base reset. */
void CDemolition_FragBomb_Reset(struct CDemolition_FragBomb *self)
{
	char *s = (char *)self;

	DB_U8(s, 0x14) = 0;
	DB_U8(s, 0x15) = 0;
	DB_U8(s, 0x16) = 0;
	DB_U8(s, 0x17) = 0;
	DB_U8(s, 0x1c) = 1;
	DB_U32(s, 0x4414) = 0;
	DB_U32(s, 0x4418) = 0xc0000000;   /* -2.0f */
	DB_U32(s, 0x441c) = 0;
	DB_F32(s, 0x4434) = db_bitsf(0x3ca3d70a);   /* 0.02 */
	DB_F32(s, 0x4438) = db_bitsf(0x3dcccccd);   /* 0.1 */
	DB_F32(s, 0x443c) = db_bitsf(0x3e4ccccd);   /* 0.2 */
	DB_F32(s, 0x4440) = db_bitsf(0x3e4ccccd);   /* 0.2 */
	DB_F32(s, 0x4444) = db_bitsf(0x3f4ccccd);   /* 0.8 */
	DB_F32(s, 0x4448) = 0.5f;
	DB_U32(s, 0x4454) = 0;
	DB_U32(s, 0x444c) = 0;
	CDemolition_Object_Stop((struct CDemolition_Object *)s);
}

/* 0x000f1050  Demolition/CDemolition_Bomb.cpp
   Throws the bomb with `velocity`: lights the 3 second fuse. */
void CDemolition_FragBomb_Throw(struct CDemolition_FragBomb *self, float *velocity)
{
	char *s = (char *)self;

	DB_U8(s, 0x15) = 1;
	DB_U8(s, 0x16) = 1;
	DB_U8(s, 0x1c) = 1;
	DB_I32(s, 0x4454) = 2;
	DB_F32(s, 0x43c4) = 0.0f;
	DB_F32(s, 0x43c8) = 0.0f;
	DB_F32(s, 0x43cc) = 0.0f;
	CDemolition_Object_SetVelocity((struct CDemolition_Object *)s, velocity);
	db_timer_start(s + 0x4628, 3.0f);
	db_sound(0x14, 0x7f);
}

/* 0x000f10d0  Demolition/CDemolition_Bomb.cpp
   The holder (+0x45d0) picks the bomb up. */
void CDemolition_FragBomb_PickUp(struct CDemolition_FragBomb *self)
{
	char *s = (char *)self;

	DB_I32(s, 0x4454) = 1;
	DB_U8(s, 0x15) = 1;
	DB_U8(s, 0x16) = 1;
	DB_U8(s, 0x1c) = 0;
	DB_F32(s, 0x43c4) = 0.0f;
	DB_F32(s, 0x43c8) = 0.0f;
	DB_F32(s, 0x43cc) = 0.0f;
	DB_VFN(s + 0x4660, 0xc, void (*)(void *))(s + 0x4660);
	DB_PTR(DB_PTR(s, 0x45d0), 0x11a8) = s;
}

/* ====================================================================================
   CDemolition_RemoteBomb (vtable 0x0026ae30, secondary 0x0026ae2c at +0x1130)
   ==================================================================================== */

/* 0x000f1120  Demolition/CDemolition_Bomb.cpp  constructor */
struct CDemolition_RemoteBomb *CDemolition_RemoteBomb_Construct(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;

	CDemolition_CharLib_Construct((struct CDemolition_CharLib *)s);
	DB_PTR(s, 0) = (void *)vtable_0026ae30;
	DB_PTR(s, 0x1130) = (void *)vtable_0026ae2c;
	FUN_000ec2f0((undefined4 *)(s + 0x11a0));
	FUN_000ec2f0((undefined4 *)(s + 0x1220));
	return self;
}

/* 0x000f1190  Demolition/CDemolition_Bomb.cpp  destructor */
void CDemolition_RemoteBomb_Destruct(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026ae30;
	DB_PTR(s, 0x1130) = (void *)vtable_0026ae2c;
	FUN_000ec300((undefined4 *)(s + 0x1220));
	FUN_000ec300((undefined4 *)(s + 0x11a0));
	CDemolition_CharLib_Destruct((struct CDemolition_CharLib *)s);
}

/* 0x002978d8  the remote bomb's animations ({ name[0x40], speed }); the actor's table
   (+0x324). CDemolition_RemoteBomb_CreateActor looks up 47 animations like every CharLib
   character; the original reads past these two entries into the following .data, where
   no name matches a sub-object. The port pads the table with empty names (no match). */
struct DbAnimName {
	char name[0x40];
	float speed;
};
struct DbAnimName DAT_002978d8[47] = {
	{ "idle", 0.05f },
	{ "brun", 0.05f },
};

/* 0x00297960  AI priorities (2 states x 2 conditions), 0x00297964 the conditions (virtual
   slots 0x4c and 0x54 through demolition_d's thunks), 0x0029796c the states (virtual slots
   0x94 and 0x9c) */
signed char DAT_00297960[4] = { 0, 1, 1, 0 };
void *DAT_00297964[2] = { (void *)FUN_0010d220, (void *)FUN_0010d240 };
void *DAT_0029796c[2] = { (void *)FUN_0010d340, (void *)FUN_0010d380 };

/* 0x000f1200  Demolition/CDemolition_Bomb.cpp
   Creates the bomb's CharLib actor (the bomb model game +0x13a8) for `player`. */
int CDemolition_RemoteBomb_CreateActor(struct CDemolition_RemoteBomb *self, int player)
{
	char *s = (char *)self;
	float pos[3];
	char *actor;
	int i;

	pos[0] = 0.0f;
	pos[1] = 0.0f;
	pos[2] = 0.0f;
	actor = (char *)Actor_Create(DB_PTR(DAT_002ff85c, 0x13a8), pos, NULL, 1, 0, 1);
	DB_PTR(s, 0x30) = actor;
	DB_I32(actor, 0x220) = 0x19;
	DB_U16(DB_PTR(s, 0x30), 0x230) = (unsigned short)player;
	DB_U16(s, 0x98) = 1;
	DB_U16(s, 0x34) = 2;
	DB_U16(s, 0x36) = 0;
	DB_I32(s, 4) = 0;
	DB_I32(s, 8) = 0;
	DB_PTR(s, 0x1094) = DAT_0029796c;
	DB_PTR(s, 0x1098) = DAT_00297964;
	DB_PTR(s, 0x109c) = DAT_00297960;
	Actor_InitCharData(DB_PTR(s, 0x30));
	DB_PTR(DB_PTR(s, 0x30), 0x324) = DAT_002978d8;
	for (i = 0; i < 47; i++)
	{
		actor = (char *)DB_PTR(s, 0x30);
		DB_PTR(DB_PTR(s, 0x30), 0x264 + i * 4) = baFindSubObject(
			(struct BActorInstance *)(actor + 0x10), (const char *)DB_PTR(actor, 0x324) + i * 0x44);
	}
	/* think interval: the character's base value (0x00290750, 0x60 bytes per character)
	   divided by the difficulty + 1 */
	DB_I32(s, 0x10b4) = *(int *)((char *)&DAT_00290750 + DAT_002baed4[player * 10] * 0x60) /
		(DAT_002bae70 + 1);
	return 1;
}

/* 0x000f1320  Demolition/CDemolition_Bomb.cpp
   Creates the bomb's input map (stick and buttons) and binds the stick to `player`'s pad. */
int CDemolition_RemoteBomb_CreateInput(struct CDemolition_RemoteBomb *self, int player)
{
	char *s = (char *)self;

	DB_PTR(s, 0x5c) = biCreateMap(4);
	DB_PTR(s, 0x64) = biCreateChannel(DB_PTR(s, 0x5c), "left/right", 3, 0, 0, 0, 0);
	DB_PTR(s, 0x60) = biCreateChannel(DB_PTR(s, 0x5c), "up/down", 3, 0, 0, 0, 0);
	DB_PTR(s, 0x68) = biCreateChannel(DB_PTR(s, 0x5c), "action1", 1, 0, 0, 0, 0);
	DB_PTR(s, 0x6c) = biCreateChannel(DB_PTR(s, 0x5c), "action2", 1, 1, 0, 0, 0);
	biBindInput(DB_PTR(s, 0x60), DAT_002baed4[player * 10 + 3] | 0xbc, 0);
	biBindInput(DB_PTR(s, 0x64), DAT_002baed4[player * 10 + 3] | 0xbb, 0);
	return 1;
}

/* 0x000f13e0  Demolition/CDemolition_Bomb.cpp  (not in the function table; called by
   0x000f2750)
   Without stick input the bomb keeps rolling the way it heads. */
void CDemolition_RemoteBomb_Steer(struct CDemolition_RemoteBomb *self, float dt)
{
	char *s = (char *)self;
	float forward[3];
	float dir[4];

	(void)dt;
	if (DB_U8(s, 0x1171) != 0 || DB_I32(DB_PTR(s, 0x60), 0x14) != 0 ||
		DB_I32(DB_PTR(s, 0x64), 0x14) != 0)
		return;
	forward[0] = 0.0f;
	forward[1] = 0.0f;
	forward[2] = 1.0f;
	bmVectorRotateY(dir, forward, DB_F32(DB_PTR(s, 0x30), 0x1d8));
	AI_SetStick(s, dir, 1.0f);
}

/* 0x000f1450  Demolition/CDemolition_Bomb.cpp
   Phase 0 (armed): counts the hit cooldown down to 0. */
void CDemolition_RemoteBomb_UpdateArmed(struct CDemolition_RemoteBomb *self, float dt)
{
	char *s = (char *)self;

	DB_F32(s, 0x1198) = DB_F32(s, 0x1198) - dt;
	if (DB_F32(s, 0x1198) < 0.0f)
		DB_F32(s, 0x1198) = 0.0f;
}

/* 0x000f1480  Demolition/CDemolition_Bomb.cpp
   Phase 1: the placed bomb grows out of the ground; when the timer ends it rolls (AI
   state 1) with a 30 frame cooldown. */
void CDemolition_RemoteBomb_UpdateGrow(struct CDemolition_RemoteBomb *self, float dt)
{
	char *s = (char *)self;
	float t;

	(void)dt;
	FUN_0018e8e0((int)(s + 0x1220));
	t = BCarTimer_GetRemaining((struct BCarTimer *)(s + 0x1220));
	DB_F32(DB_PTR(s, 0x30), 0xe0) = t;
	DB_F32(DB_PTR(s, 0x30), 0xe4) = (1.0f - t) * 5.0f + 1.0f;
	DB_F32(DB_PTR(s, 0x30), 0xe8) = t;
	DB_F32(s, 0x1258) = t;
	if (!(t < 1.0f))
	{
		AI_SetState(s, 1);
		DB_I32(s, 0x1190) = 0;
		DB_F32(s, 0x1198) = 30.0f;
	}
}

/* 0x000f1500  Demolition/CDemolition_Bomb.cpp
   Phase 2: the recalled bomb shrinks into the ground; then it is gone. */
void CDemolition_RemoteBomb_UpdateShrink(struct CDemolition_RemoteBomb *self, float dt)
{
	char *s = (char *)self;
	float t;
	float left;

	(void)dt;
	FUN_0018e8e0((int)(s + 0x1220));
	t = BCarTimer_GetRemaining((struct BCarTimer *)(s + 0x1220));
	left = 1.0f - t;
	DB_F32(DB_PTR(s, 0x30), 0xe0) = left;
	DB_F32(DB_PTR(s, 0x30), 0xe4) = t * 5.0f + 1.0f;
	DB_F32(DB_PTR(s, 0x30), 0xe8) = left;
	DB_F32(s, 0x1258) = left;
	if (!(t < 1.0f))
	{
		DB_I32(s, 0x1190) = 0;
		DB_U8(s, 0x116d) = 0;
		DB_U8(s, 0x116e) = 0;
		DB_I32(DB_PTR(s, 0x30), 0x218) = 0;
		DB_U32(DB_PTR(s, 0x30), 0x220) &= ~8u;
	}
}

/* 0x000f1590  Demolition/CDemolition_Bomb.cpp  (vtable 0x0026ae30 +0xe8) Draw */
void CDemolition_RemoteBomb_Draw(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;

	if (DB_U8(s, 0x116e) != 0)
		baDrawActorInstance((struct BActorInstance *)((char *)DB_PTR(s, 0x30) + 0x10), 7);
}

/* 0x002ff938  the marker quad of 0x000f15b0: 4 vertices (FVF 0x152: position, normal,
   colour, uv; 0x24 bytes each) */
float DAT_002ff938[4 * 9];

/* 0x002bce18, 0x002bc490, 0x002bd640: colour channel tables (red, green, blue: value ->
   ARGB bits), 0x002bc998: the alpha bits (bdisplay) */
extern unsigned int DAT_002bce18[];
extern unsigned int DAT_002bc490[];
extern unsigned int DAT_002bd640[];
extern unsigned int DAT_002bc998;

/* 0x000f15b0  Demolition/CDemolition_Bomb.cpp
   Draws the coloured ground marker under the bomb (300 units * scale, the player's colour
   faded by the scale). */
void CDemolition_RemoteBomb_DrawMarker(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;
	unsigned int colour;
	float pos[3];
	float m[16];
	float size;
	int r;
	int g;
	int b;

	r = (int)((double)(unsigned int)DB_I32(s, 0x1178) * DB_F32(s, 0x1258));
	g = (int)((double)(unsigned int)DB_I32(s, 0x117c) * DB_F32(s, 0x1258));
	b = (int)((double)(unsigned int)DB_I32(s, 0x1180) * DB_F32(s, 0x1258));
	pos[0] = DB_F32(s, 0x1144);
	pos[1] = DB_F32(s, 0x1148);
	pos[2] = DB_F32(s, 0x114c);
	bdSetTexture(0, DB_PTR(DAT_002ff85c, 0x6c78));
	bdPushRenderState(7, 2, 1);
	size = DB_F32(s, 0x1258) * 300.0f;
	colour = DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bc998;
	DAT_002ff938[0 * 9 + 6] = db_bitsf(colour);
	DAT_002ff938[1 * 9 + 6] = db_bitsf(colour);
	DAT_002ff938[0 * 9 + 0] = -size;
	DAT_002ff938[0 * 9 + 2] = -size;
	DAT_002ff938[1 * 9 + 0] = -size;
	DAT_002ff938[1 * 9 + 2] = size;
	DAT_002ff938[2 * 9 + 0] = size;
	DAT_002ff938[3 * 9 + 6] = db_bitsf(colour);
	DAT_002ff938[2 * 9 + 2] = -size;
	DAT_002ff938[3 * 9 + 0] = size;
	DAT_002ff938[3 * 9 + 2] = size;
	DAT_002ff938[0 * 9 + 1] = 0.0f;
	DAT_002ff938[1 * 9 + 1] = 0.0f;
	DAT_002ff938[2 * 9 + 1] = 0.0f;
	DAT_002ff938[3 * 9 + 1] = 0.0f;
	DAT_002ff938[2 * 9 + 6] = db_bitsf(colour);
	DAT_002ff938[0 * 9 + 7] = 0.0f;
	DAT_002ff938[0 * 9 + 8] = 0.0f;
	DAT_002ff938[1 * 9 + 7] = 0.0f;
	DAT_002ff938[1 * 9 + 8] = 1.0f;
	DAT_002ff938[2 * 9 + 7] = 1.0f;
	DAT_002ff938[2 * 9 + 8] = 0.0f;
	DAT_002ff938[3 * 9 + 7] = 1.0f;
	DAT_002ff938[3 * 9 + 8] = 1.0f;
	bmMatrixTranslation(m, pos[0], pos[1], pos[2]);
	bdSetWorldMatrix(m);
	bdSetDepthOffset(db_bitsf(0xbd4ccccd));   /* -0.05 */
	bdDrawPrimitiveFVF(4, DAT_002ff938, 4, 0x152);
	bdPopRenderState(7);
	bdSetDepthOffset(0.0f);
}

/* 0x000f17d0  Demolition/CDemolition_Bomb.cpp  (vtable 0x0026ae30 +0xf0) Reset */
void CDemolition_RemoteBomb_Reset(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;

	DB_U8(s, 0x116c) = 0;
	DB_U8(s, 0x116d) = 0;
	DB_U8(s, 0x116e) = 0;
	DB_U8(s, 0x1174) = 1;
	DB_U8(s, 0x1171) = 0;
	DB_U32(DB_PTR(s, 0x30), 0x220) &= ~8u;
	DB_I32(DB_PTR(s, 0x30), 0x218) = 0;
	DB_U8(s, 0x1218) = 1;
	DB_U8(s, 0x1219) = 1;
	DB_I32(s, 0x1190) = 0;
	DB_F32(s, 0x1198) = 30.0f;
	DB_F32(s, 0x1258) = 0.0f;
}

/* 0x000f1830  Demolition/CDemolition_Bomb.cpp
   Recalls the bomb: it stops and shrinks away (phase 2, 0.15 s). */
void CDemolition_RemoteBomb_Recall(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;

	AI_SetState(s, 0);
	DB_I32(s, 0x1190) = 2;
	db_timer_start(s + 0x1220, db_bitsf(0x3e19999a));   /* 0.15 */
	db_sound(0xc, 0x7f);
}

/* 0x000f1880  Demolition/CDemolition_Bomb.cpp  (vtable 0x0026ae30 +0x54)
   AI condition 1: the bomb is controllable and armed (phase 0) or not growing/shrinking. */
int CDemolition_RemoteBomb_CanRoll(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;
	int phase;

	if (DB_U8(s, 0x1174) == 0)
		return 0;
	phase = DB_I32(s, 0x1190);
	if (phase > 0 && phase <= 2)
		return 0;
	return 1;
}

/* 0x000f18b0  Demolition/CDemolition_Bomb.cpp  (vtable 0x0026ae30 +0x94)
   AI state 0 (idle): plays "idle" and stops pushing. */
int CDemolition_RemoteBomb_StateIdle(struct CDemolition_RemoteBomb *self, int entered)
{
	char *s = (char *)self;

	if (entered != 0)
		Actor_PlayAnim(DB_PTR(s, 0x30), 0, 1, 0, 1.0f, 0.1f, 1);
	DB_F32(s, 0x84) = 0.0f;
	DB_F32(s, 0x80) = 0.0f;
	DB_F32(s, 0x7c) = 0.0f;
	return 0;
}

/* ====================================================================================
   CDemolition_TimedBomb (vtable 0x0026af2c, 0x2b0 bytes; 8 inside the timed system)
   ==================================================================================== */

/* 0x000f1900  Demolition/CDemolition_Bomb.cpp  destructor */
void CDemolition_TimedBomb_Destruct(struct CDemolition_TimedBomb *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026af2c;
	CDemolition_BombFx_Destruct((struct CDemolition_BombFx *)(s + 0x1c8));
}

/* the glow sprite of a bomb's effects (+0x00 position, +0xd6 colour), 250 x 250 */
static void db_bombfx_draw_glow(char *fx, int alpha, float zbias)
{
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	DB_F32(fx, 0xc) = 1.0f;
	bdDrawSprite3D((float *)fx, 250.0f, 250.0f, 0.0f, DB_PTR(DAT_002ff85c, 0x6c68), 0, 0,
		DB_I16(fx, 0xd6), DB_I16(fx, 0xd8), DB_I16(fx, 0xda), alpha, NULL, zbias);
	bdPopRenderState(5);
	bdPopRenderState(6);
}

/* 0x000f1920  Demolition/CDemolition_Bomb.cpp
   Draws the timed bomb's sparks and glow. */
void CDemolition_TimedBomb_DrawFx(struct CDemolition_TimedBomb *self)
{
	char *s = (char *)self;

	DB_VFN(s + 0x218, 8, void (*)(void *))(s + 0x218);
	db_bombfx_draw_glow(s + 0x1c8, 0x69, db_bitsf(0xbc23d70a));   /* -0.01 */
}

/* ====================================================================================
   CDemolition_CrossBomb (vtable 0x0026af30)
   ==================================================================================== */

/* 0x000f19c0  Demolition/CDemolition_Bomb.cpp  destructor */
void CDemolition_CrossBomb_Destruct(struct CDemolition_CrossBomb *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026af30;
	CDemolition_BombFx_Destruct((struct CDemolition_BombFx *)(s + 0x45d8));
	CDemolition_Object_Destruct((struct CDemolition_Object *)s);
}

/* 0x000f1a20  Demolition/CDemolition_Bomb.cpp  (vtable 0x0026af30 slot 1)
   Frees the bomb model and stops the sparks. */
void CDemolition_CrossBomb_Release(struct CDemolition_CrossBomb *self)
{
	char *s = (char *)self;

	baDeleteActorInstance((struct BActorInstance *)(s + 0x4460));
	DB_VFN(s + 0x4628, 0, void (*)(void *))(s + 0x4628);
}

/* 0x000f1a50  Demolition/CDemolition_Bomb.cpp  (slot 3 of the frag and cross bombs)
   Draws the bomb model; a held bomb is hidden while the game is paused. */
void CDemolition_Bomb_Draw(struct CDemolition_FragBomb *self)
{
	char *s = (char *)self;

	if (DB_U8(s, 0x16) == 0)
		return;
	if (DB_I32(s, 0x4454) == 1 && DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	baDrawActorInstance((struct BActorInstance *)(s + 0x4460), 7);
}

/* 0x000f1a80  Demolition/CDemolition_Bomb.cpp
   Draws the cross bomb's sparks and glow. */
void CDemolition_CrossBomb_DrawFx(struct CDemolition_CrossBomb *self)
{
	char *s = (char *)self;

	if (DB_U8(s, 0x16) == 0)
		return;
	if (DB_I32(s, 0x4454) == 1 && DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	DB_VFN(s + 0x4628, 8, void (*)(void *))(s + 0x4628);
	db_bombfx_draw_glow(s + 0x45d8, 0x5f, -0.5f);
}

/* 0x000f1b40  Demolition/CDemolition_Bomb.cpp
   `player` takes the cross bomb (it becomes his bomb at +0x11a0). */
void CDemolition_CrossBomb_Attach(struct CDemolition_CrossBomb *self, struct CDemolition_Player *player)
{
	char *s = (char *)self;
	char *holder;

	DB_I32(s, 0x4454) = 1;
	DB_U8(s, 0x15) = 1;
	DB_U8(s, 0x16) = 1;
	DB_PTR(s, 0x45d0) = player;
	DB_U8(s, 0x1c) = 0;
	DB_F32(s, 0x43c4) = 0.0f;
	DB_F32(s, 0x43c8) = 0.0f;
	DB_F32(s, 0x43cc) = 0.0f;
	DB_PTR(player, 0x11a0) = s;
	holder = (char *)DB_PTR(s, 0x45d0);
	CDemolition_Player_SetCarry((struct CDemolition_Player *)holder,
		DB_PTR(DB_PTR(holder, 0x30), 0x300), DB_PTR(holder, 0x1238));
}

/* ====================================================================================
   The bomb systems (game +0x48)
   ==================================================================================== */

/* 0x000f1ba0  Demolition/CDemolition_Bomb.cpp  CDemolition_BombSystem constructor (inline) */
struct CDemolition_BombSystem *CDemolition_BombSystem_Construct(struct CDemolition_BombSystem *self)
{
	DB_PTR(self, 0) = (void *)vtable_0026af44;
	return self;
}

/* 0x000f1bb0  Demolition/CDemolition_Bomb.cpp  CDemolition_BombSystem destructor (inline) */
void CDemolition_BombSystem_Destruct(struct CDemolition_BombSystem *self)
{
	DB_PTR(self, 0) = (void *)vtable_0026af44;
}

/* 0x000f1bc0  Demolition/CDemolition_Bomb.cpp  the game's bomb system: Update(dt) (slot 2) */
void CDemolition_Game_UpdateBombs(void *game, float dt)
{
	void *bombs = DB_PTR(game, 0x48);

	DB_VFN(bombs, 8, void (*)(void *, float))(bombs, dt);
}

/* 0x000f1bd0  Demolition/CDemolition_Bomb.cpp  the game's bomb system: Draw (slot 3) */
void CDemolition_Game_DrawBombs(void *game)
{
	void *bombs = DB_PTR(game, 0x48);

	DB_VFN(bombs, 0xc, void (*)(void *))(bombs);
}

/* 0x000f1be0  Demolition/CDemolition_Bomb.cpp  the game's bomb system: DrawFx (slot 4) */
void CDemolition_Game_DrawBombFx(void *game)
{
	void *bombs = DB_PTR(game, 0x48);

	DB_VFN(bombs, 0x10, void (*)(void *))(bombs);
}

/* 0x000f1bf0  Demolition/CDemolition_Bomb.cpp  the game's bomb system: Start (slot 6) */
void CDemolition_Game_StartBombs(void *game)
{
	void *bombs = DB_PTR(game, 0x48);

	DB_VFN(bombs, 0x18, void (*)(void *))(bombs);
}

/* 0x000f1c00  Demolition/CDemolition_Bomb.cpp  the game's bomb system: Stop (slot 7) */
void CDemolition_Game_StopBombs(void *game)
{
	void *bombs = DB_PTR(game, 0x48);

	DB_VFN(bombs, 0x1c, void (*)(void *))(bombs);
}

/* ---------- variation 0: frag bombs (vtable 0x0026af48; +0x04 the first bomb) */

/* 0x000f1c10  Demolition/CDemolition_Bomb.cpp  (0x0026af48 slot 2) Update(dt) */
void CDemolition_BombSystemFrag_Update(struct CDemolition_BombSystem *self, float dt)
{
	char *b;

	for (b = (char *)DB_PTR(self, 4); b != NULL; b = (char *)DB_PTR(b, 0x10))
	{
		if (DB_U8(b, 0x15) != 0)
			DB_VFN(b, 8, void (*)(void *, float))(b, dt);
	}
	FUN_0018ec60((int)(DAT_002ff85c + 0x77d4), (undefined4)DB_PTR(self, 4));
	db_game_77d4_update(dt);
}

/* 0x000f1c70  Demolition/CDemolition_Bomb.cpp  (0x0026af48 slot 3) Draw */
void CDemolition_BombSystemFrag_Draw(struct CDemolition_BombSystem *self)
{
	char *b;

	if (DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	for (b = (char *)DB_PTR(self, 4); b != NULL; b = (char *)DB_PTR(b, 0x10))
	{
		if (DB_U8(b, 0x16) != 0)
			DB_VFN(b, 0xc, void (*)(void *))(b);
	}
}

/* 0x000f1ca0  Demolition/CDemolition_Bomb.cpp  (0x0026af48 slot 4) DrawFx: the sparks */
void CDemolition_BombSystemFrag_DrawFx(struct CDemolition_BombSystem *self)
{
	char *b;

	if (DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	for (b = (char *)DB_PTR(self, 4); b != NULL; b = (char *)DB_PTR(b, 0x10))
	{
		if (DB_U8(b, 0x16) != 0)
			DB_VFN(b + 0x4660, 8, void (*)(void *))(b + 0x4660);
	}
}

/* 0x000f1ce0  Demolition/CDemolition_Bomb.cpp  (0x0026af48 slot 5) Reset
   Resets the bombs and gives two to each player (game +0x70: the players by index) in his
   colour. */
void CDemolition_BombSystemFrag_Reset(struct CDemolition_BombSystem *self)
{
	char *b;
	char *player;
	int slot = 0;
	int index = 0;

	for (b = (char *)DB_PTR(self, 4); b != NULL; b = (char *)DB_PTR(b, 0x10))
	{
		DB_VFN(b, 0x10, void (*)(void *))(b);
		player = (char *)DB_PTR(DAT_002ff85c, 0x70 + index * 4);
		DB_PTR(player, 0x11a0 + slot * 4) = b;
		DB_PTR(b, 0x45d0) = player;
		DB_I16(b, 0x4700) = (short)DB_I32(player, 0x1178);
		DB_I16(b, 0x4702) = (short)DB_I32(player, 0x117c);
		DB_I16(b, 0x4704) = (short)DB_I32(player, 0x1180);
		slot++;
		if (slot >= 2)
		{
			index++;
			slot = 0;
		}
	}
}

/* 0x000f1d50  Demolition/CDemolition_Bomb.cpp  (0x0026af48 slot 7) Stop: freezes the bombs */
void CDemolition_BombSystemFrag_Stop(struct CDemolition_BombSystem *self)
{
	char *b;

	for (b = (char *)DB_PTR(self, 4); b != NULL; b = (char *)DB_PTR(b, 0x10))
		DB_U8(b, 0x17) = 1;
}

/* ---------- variation 1: remote bombs (vtable 0x0026af6c; +0x04 the 4 bombs) */

/* 0x000f1d70  Demolition/CDemolition_Bomb.cpp  (0x0026af6c slot 1) Shutdown */
void CDemolition_BombSystemRemote_Shutdown(struct CDemolition_BombSystem *self)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		void *b = DB_PTR(self, 4 + i * 4);

		if (b == NULL)
			continue;
		DB_VFN(b, 0xe0, void (*)(void *))(b);
		b = DB_PTR(self, 4 + i * 4);
		if (b != NULL)
			DB_VFN(b, 0x48, void (*)(void *, int))(b, 1);
	}
}

/* 0x000f1db0  Demolition/CDemolition_Bomb.cpp  (0x0026af6c slot 2) Update(dt) */
void CDemolition_BombSystemRemote_Update(struct CDemolition_BombSystem *self, float dt)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		void *b = DB_PTR(self, 4 + i * 4);

		if (DB_U8(b, 0x116d) != 0)
			DB_VFN(b, 0xe4, void (*)(void *, float))(b, dt);
	}
}

/* 0x000f1df0  Demolition/CDemolition_Bomb.cpp  (0x0026af6c slot 3) Draw */
void CDemolition_BombSystemRemote_Draw(struct CDemolition_BombSystem *self)
{
	int i;

	if (DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	for (i = 0; i < 4; i++)
	{
		void *b = DB_PTR(self, 4 + i * 4);

		if (DB_U8(b, 0x116d) != 0)
			DB_VFN(b, 0xe8, void (*)(void *))(b);
	}
}

/* 0x000f1e30  Demolition/CDemolition_Bomb.cpp  (0x0026af6c slot 4) DrawFx */
void CDemolition_BombSystemRemote_DrawFx(struct CDemolition_BombSystem *self)
{
	int i;

	if (DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	for (i = 0; i < 4; i++)
	{
		void *b = DB_PTR(self, 4 + i * 4);

		if (DB_U8(b, 0x116d) != 0)
			DB_VFN(b, 0xec, void (*)(void *))(b);
	}
}

/* 0x000f1e70  Demolition/CDemolition_Bomb.cpp  (0x0026af6c slot 5) Reset
   Resets the 4 bombs and pairs bomb i with player i (game +0x70). */
void CDemolition_BombSystemRemote_Reset(struct CDemolition_BombSystem *self)
{
	char *player;
	int i;

	for (i = 0; i < 4; i++)
	{
		void *b = DB_PTR(self, 4 + i * 4);

		DB_VFN(b, 0xf0, void (*)(void *))(b);
	}
	for (i = 0; i < 4; i++)
	{
		player = (char *)DB_PTR(DAT_002ff85c, 0x70 + i * 4);
		DB_PTR(player, 0x11a0) = DB_PTR(self, 4 + i * 4);
		DB_PTR(DB_PTR(self, 4 + i * 4), 0x11d8) = player;
	}
}

/* 0x000f1f00  Demolition/CDemolition_Bomb.cpp  (0x0026af6c slot 7) Stop: no more control */
void CDemolition_BombSystemRemote_Stop(struct CDemolition_BombSystem *self)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		DB_U8(DB_PTR(self, 4 + i * 4), 0x1174) = 0;
		AI_SetState(DB_PTR(self, 4 + i * 4), 0);
	}
}

/* ---------- variation 2: timed bombs (vtable 0x0026af90; +0x10 8 bombs of 0x2b0 bytes,
   +0x1590 a timer, +0x15cc visible, +0x15cd frozen) */

/* 0x000f1f30  Demolition/CDemolition_Bomb.cpp  destructor */
void CDemolition_BombSystemTimed_Destruct(struct CDemolition_BombSystem *self)
{
	char *s = (char *)self;
	int i;

	DB_PTR(s, 0) = (void *)vtable_0026af90;
	FUN_000ec300((undefined4 *)(s + 0x1590));
	/* MSVC's `eh vector destructor iterator` (0x001def24): last to first */
	for (i = 8 - 1; i >= 0; i--)
		CDemolition_TimedBomb_Destruct((struct CDemolition_TimedBomb *)(s + 0x10 + i * 0x2b0));
}

/* 0x000f1fa0  Demolition/CDemolition_Bomb.cpp  (0x0026af90 slot 1) Shutdown */
void CDemolition_BombSystemTimed_Shutdown(struct CDemolition_BombSystem *self)
{
	char *s = (char *)self;
	int i;

	for (i = 0; i < 8; i++)
	{
		char *b = s + 0x10 + i * 0x2b0;

		baDeleteActorInstance((struct BActorInstance *)(b + 0x10));
		DB_VFN(b + 0x218, 0, void (*)(void *))(b + 0x218);
	}
}

/* 0x000f1fe0  Demolition/CDemolition_Bomb.cpp  (0x0026af90 slot 3) Draw */
void CDemolition_BombSystemTimed_Draw(struct CDemolition_BombSystem *self)
{
	char *s = (char *)self;
	int i;

	if (DB_U8(DAT_002ff85c, 0x25) != 0 || DB_U8(s, 0x15cc) == 0)
		return;
	for (i = 0; i < 8; i++)
	{
		char *b = s + 0x10 + i * 0x2b0;

		if (DB_U8(b, 0x1c1) != 0)
			baDrawActorInstance((struct BActorInstance *)(b + 0x10), 7);
	}
}

/* 0x000f2030  Demolition/CDemolition_Bomb.cpp  (0x0026af90 slot 4) DrawFx */
void CDemolition_BombSystemTimed_DrawFx(struct CDemolition_BombSystem *self)
{
	char *s = (char *)self;
	int i;

	if (DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	for (i = 0; i < 8; i++)
	{
		char *b = s + 0x10 + i * 0x2b0;

		if (DB_U8(b, 0x1c1) != 0)
			CDemolition_TimedBomb_DrawFx((struct CDemolition_TimedBomb *)b);
	}
}

/* 0x000f2070  Demolition/CDemolition_Bomb.cpp  (0x0026af90 slot 5) Reset */
void CDemolition_BombSystemTimed_Reset(struct CDemolition_BombSystem *self)
{
	char *s = (char *)self;
	int i;

	DB_U8(s, 0x15cc) = 0;
	DB_U8(s, 0x15cd) = 0;
	for (i = 0; i < 8; i++)
	{
		char *b = s + 0x10 + i * 0x2b0;

		DB_U8(b, 0x1c0) = 0;
		DB_U8(b, 0x1c1) = 0;
		DB_U8(b, 0x1c2) = 0;
	}
}

/* 0x000f2110  Demolition/CDemolition_Bomb.cpp  (0x0026af90 slot 7) Stop */
void CDemolition_BombSystemTimed_Stop(struct CDemolition_BombSystem *self)
{
	char *s = (char *)self;
	int i;

	DB_U8(s, 0x15cd) = 1;
	for (i = 0; i < 8; i++)
		DB_U8(s + 0x10 + i * 0x2b0, 0x1c2) = 1;
}

/* ---------- variation 3: cross bombs (vtable 0x0026afb4; +0x04 the first bomb,
   +0x10 started, +0x11 stopped) */

/* 0x000f2130  Demolition/CDemolition_Bomb.cpp  (slot 1 of the frag and cross systems)
   Shutdown: releases and deletes the bombs of the list. */
void CDemolition_BombSystemList_Shutdown(struct CDemolition_BombSystem *self)
{
	char *b = (char *)DB_PTR(self, 4);
	char *next;

	while (b != NULL)
	{
		next = (char *)DB_PTR(b, 0x10);
		DB_VFN(b, 4, void (*)(void *))(b);
		DB_VFN(b, 0, void *(*)(void *, int))(b, 1);
		b = next;
	}
}

/* 0x000f2160  Demolition/CDemolition_Bomb.cpp  (0x0026afb4 slot 3) Draw */
void CDemolition_BombSystemCross_Draw(struct CDemolition_BombSystem *self)
{
	char *b;

	if (DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	for (b = (char *)DB_PTR(self, 4); b != NULL; b = (char *)DB_PTR(b, 0x10))
		DB_VFN(b, 0xc, void (*)(void *))(b);
}

/* 0x000f2190  Demolition/CDemolition_Bomb.cpp  (0x0026afb4 slot 4) DrawFx */
void CDemolition_BombSystemCross_DrawFx(struct CDemolition_BombSystem *self)
{
	char *b;

	if (DB_U8(DAT_002ff85c, 0x25) != 0)
		return;
	for (b = (char *)DB_PTR(self, 4); b != NULL; b = (char *)DB_PTR(b, 0x10))
		CDemolition_CrossBomb_DrawFx((struct CDemolition_CrossBomb *)b);
}

/* 0x000f21c0  Demolition/CDemolition_Bomb.cpp  (0x0026afb4 slot 6) Start */
void CDemolition_BombSystemCross_Start(struct CDemolition_BombSystem *self)
{
	DB_U8(self, 0x10) = 1;
}

/* 0x000f21d0  Demolition/CDemolition_Bomb.cpp  (0x0026afb4 slot 7) Stop */
void CDemolition_BombSystemCross_Stop(struct CDemolition_BombSystem *self)
{
	char *b;

	DB_U8(self, 0x11) = 1;
	for (b = (char *)DB_PTR(self, 4); b != NULL; b = (char *)DB_PTR(b, 0x10))
		DB_U8(b, 0x17) = 1;
}

/* 0x000f21f0  Demolition/CDemolition_Bomb.cpp  (vtable 0x0026ae2c, the remote bomb's
   message receiver at +0x1130) the deleting destructor, adjusted to the object */
void *CDemolition_RemoteBomb_Receiver_DeletingDestruct(void *receiver, unsigned int flags)
{
	return CDemolition_RemoteBomb_DeletingDestruct(
		(struct CDemolition_RemoteBomb *)((char *)receiver - 0x1130), flags);
}

/* ====================================================================================
   CDemolition_FragBomb, continued
   ==================================================================================== */

/* 0x000f2200  Demolition/CDemolition_Bomb.cpp  constructor */
struct CDemolition_FragBomb *CDemolition_FragBomb_Construct(struct CDemolition_FragBomb *self)
{
	char *s = (char *)self;

	CDemolition_Object_Construct((struct CDemolition_Object *)s);
	DB_PTR(s, 0) = (void *)vtable_0026ae18;
	FUN_000ec2f0((undefined4 *)(s + 0x45d8));
	FUN_000ec2f0((undefined4 *)(s + 0x4610 + 0x18));
	CSparkPointSystem_Construct((struct CSparkPointSystem *)(s + 0x4610 + 0x50));
	return self;
}

/* 0x000f2280  Demolition/CDemolition_Bomb.cpp  (0x0026ae18 slot 0) deleting destructor */
void *CDemolition_FragBomb_DeletingDestruct(struct CDemolition_FragBomb *self, unsigned int flags)
{
	CDemolition_FragBomb_Destruct(self);
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000f22a0  Demolition/CDemolition_Bomb.cpp
   Sets the bomb up: the bomb model (game +0x13a4), a random heading, the physics frame
   (radius 40.5, mass 2), the "bombstate" node, the sparks; then resets it. */
int CDemolition_FragBomb_Init(struct CDemolition_FragBomb *self, int type)
{
	char *s = (char *)self;
	float axis[4];
	float origin[3];
	float quat[4];
	float up[3];
	int degrees;

	DB_I32(s, 0x10) = 0;
	DB_I32(s, 0x4450) = type;
	DB_F32(s, 0x4410) = 40.5f;
	DB_F32(s, 0x439c) = 40.5f;
	baInitActorInstance((struct BActorInstance *)(s + 0x4460), DB_PTR(DAT_002ff85c, 0x13a4), 1,
		"C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Bomb.cpp", 0x77);
	degrees = db_rand() % 360;
	axis[0] = 0.0f;
	axis[1] = 1.0f;
	axis[2] = 0.0f;
	axis[3] = (float)degrees * 0.0174532924f;
	bmAxisAngleToQuaternion(quat, axis);
	origin[0] = 0.0f;
	origin[1] = 0.0f;
	origin[2] = 0.0f;
	FUN_000ec490((undefined4 *)(s + 0x43a0), (undefined4 *)origin, (undefined4 *)quat, 2.0f);
	DB_PTR(s, 0x4610 + 0x10) = baFindNode((struct BActorNode *)DB_PTR(s, 0x4594), "bombstate");
	FUN_000ec310((int)(s + 0x4628));
	CSparkPointSystem_Init((struct CSparkPointSystem *)(s + 0x4660), NULL, 0x14);
	DB_U8(s, 0x46b9) = 0;
	DB_F32(s, 0x46c4) = db_bitsf(0xbe99999a);   /* -0.3 */
	DB_I32(s, 0x46d0) = 0x10;
	DB_I32(s, 0x46d4) = 0x10;
	DB_I32(s, 0x46d8) = 4;
	DB_I32(s, 0x46dc) = 6;
	DB_F32(s, 0x46bc) = 20.0f;
	DB_F32(s, 0x46cc) = 1.0f;
	DB_F32(s, 0x46ec) = 120.0f;
	CParticleSystem_SetTexture((struct CPSHeader *)(s + 0x4660), DB_I32(DAT_002ff85c, 0x6c54));
	CParticleSystem_SetTexture2((struct CParticleSystem *)(s + 0x4660), DB_I32(DAT_002ff85c, 0x6c78));
	up[0] = 0.0f;
	up[1] = 1.0f;
	up[2] = 0.0f;
	FUN_000ab4c0((int)(s + 0x4660), (undefined4 *)up);
	FUN_0010bd80((int)s, 0x10);
	DB_VFN(s, 0x10, void (*)(void *))(s);
	return 1;
}

/* 0x002978b4  height of a held bomb above the hands, by character (+0x1188 of the holder) */
float DAT_002978b4[6] = { 20.0f, 20.0f, 20.0f, 20.0f, 20.0f, 20.0f };

/* 0x000f2480  Demolition/CDemolition_Bomb.cpp
   A held bomb follows its holder: midway between the hands (+0x123c, +0x1240), raised by
   the character's height, turned with him. */
void CDemolition_FragBomb_FollowHolder(struct CDemolition_FragBomb *self, float dt)
{
	char *s = (char *)self;
	char *holder = (char *)DB_PTR(s, 0x45d0);
	float left[4];
	float right[4];
	float pos[4];
	float axis[4];

	(void)dt;
	baGetNodePosition((struct BActorInstance *)((char *)DB_PTR(holder, 0x30) + 0x10),
		(struct BActorNode *)DB_PTR(holder, 0x123c), left);
	holder = (char *)DB_PTR(s, 0x45d0);
	baGetNodePosition((struct BActorInstance *)((char *)DB_PTR(holder, 0x30) + 0x10),
		(struct BActorNode *)DB_PTR(holder, 0x1240), right);
	holder = (char *)DB_PTR(s, 0x45d0);
	pos[0] = (right[0] + left[0]) * 0.5f;
	pos[1] = (right[1] + left[1]) * 0.5f;
	pos[2] = (right[2] + left[2]) * 0.5f;
	pos[1] = pos[1] + DAT_002978b4[DB_I32(holder, 0x1188)];
	axis[0] = 0.0f;
	axis[1] = 1.0f;
	axis[2] = 0.0f;
	axis[3] = -DB_F32(DB_PTR(holder, 0x30), 0x1e8);
	bmAxisAngleToQuaternion(DB_FP(s, 0x43f4), axis);
	CDemolition_Object_SetPosition((struct CDemolition_Object *)s, pos);
}

/* 0x000f2570  Demolition/CDemolition_Bomb.cpp
   The bomb goes off: an explosion at its position, credited to `owner` (frag chain
   `source`, NULL for a new chain). */
void CDemolition_FragBomb_Detonate(struct CDemolition_FragBomb *self, void *source, void *owner)
{
	char *s = (char *)self;

	DB_U8(s, 0x15) = 0;
	DB_U8(s, 0x16) = 0;
	DB_I32(s, 0x4454) = 0;
	db_sound(db_rand() % 4, 0xff);
	db_explode_at(source, owner, DB_FP(s, 0x43a4));
}

/* ====================================================================================
   CDemolition_RemoteBomb, continued
   ==================================================================================== */

/* 0x000f2600  Demolition/CDemolition_Bomb.cpp  (0x0026ae30 +0x48) deleting destructor */
void *CDemolition_RemoteBomb_DeletingDestruct(struct CDemolition_RemoteBomb *self, unsigned int flags)
{
	CDemolition_RemoteBomb_Destruct(self);
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000f2620  Demolition/CDemolition_Bomb.cpp  (0x0026ae30 +0xdc) Init(player)
   The bomb of `player`: input, actor, "bombstate" node, glow textures, the holder's colour,
   the start position (game +0x7c0 + 0x20 * player), then a reset. */
int CDemolition_RemoteBomb_Init(struct CDemolition_RemoteBomb *self, int player)
{
	char *s = (char *)self;
	char *holder;

	DB_I32(s, 0x1184) = player;
	DB_U8(s, 0x116c) = 0;
	DB_U8(s, 0x116d) = 0;
	DB_U8(s, 0x116e) = 0;
	DB_U8(s, 0x1174) = 1;
	DB_U8(s, 0x1171) = 0;
	CDemolition_RemoteBomb_CreateInput(self, player);
	CDemolition_RemoteBomb_CreateActor(self, player);
	DB_PTR(s, 0x11dc) = baFindNode((struct BActorNode *)DB_PTR(DB_PTR(s, 0x30), 0x144), "bombstate");
	DB_PTR(s, 0x11e8) = DB_PTR(DAT_002ff85c, 0x6c64);
	DB_PTR(s, 0x11fc) = DB_PTR(DAT_002ff85c, 0x6c68);
	holder = (char *)DB_PTR(s, 0x11d8);
	DB_I32(s, 0x1178) = DB_I32(holder, 0x1178);
	DB_I32(s, 0x117c) = DB_I32(holder, 0x117c);
	DB_I32(s, 0x1180) = DB_I32(holder, 0x1180);
	CDemolition_CharLib_SetPosition((struct CDemolition_CharLib *)s,
		(float *)(DAT_002ff85c + (DB_I32(s, 0x1184) + 0x3e) * 0x20));
	FUN_000ec310((int)(s + 0x1220));
	DB_VFN(s, 0xf0, void (*)(void *))(s);
	return 1;
}

/* 0x000f2710  Demolition/CDemolition_Bomb.cpp  (0x0026ae30 +0xe0) Shutdown: the input */
void CDemolition_RemoteBomb_Shutdown(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;

	biDeleteChannel(DB_PTR(s, 0x64));
	biDeleteChannel(DB_PTR(s, 0x60));
	biDeleteChannel(DB_PTR(s, 0x68));
	biDeleteChannel(DB_PTR(s, 0x6c));
	biDeleteMap(DB_PTR(s, 0x5c));
}

/* 0x000f2750  Demolition/CDemolition_Bomb.cpp  (0x0026ae30 +0xe4) Update(dt)
   (The table's size of this function is too small: it ends at 0x000f2983.) */
void CDemolition_RemoteBomb_Update(struct CDemolition_RemoteBomb *self, float dt)
{
	char *s = (char *)self;
	char *actor;
	float axis[4];
	int i;

	if (DB_U8(s, 0x116d) == 0)
		return;
	actor = (char *)DB_PTR(s, 0x30);
	DB_F32(s, 0x1144) = DB_F32(actor, 0xd0);
	DB_F32(s, 0x1148) = DB_F32(actor, 0xd4);
	DB_F32(s, 0x114c) = DB_F32(actor, 0xd8);
	DB_F32(s, 0x1150) = DB_F32(actor, 0xdc);
	/* the stick is relative to the camera (game +0x12a0) */
	DB_F32(s, 0x1118) = CLMath_WrapAngle((float)atan2((double)DB_F32(s, 0x1144) - DB_F32(DAT_002ff85c, 0x12a0),
		(double)DB_F32(s, 0x114c) - DB_F32(DAT_002ff85c, 0x12a8)));
	if (DB_F32(s, 0x1148) <= -100.0f)
		DB_F32(s, 0x1148) = 5.0f;
	if (DB_U8(s, 0x1174) == 0)
		biClearMap(DB_PTR(s, 0x5c));
	else
		CDemolition_RemoteBomb_Steer(self, dt);
	switch (DB_I32(s, 0x1190))
	{
	case 0:
		CDemolition_RemoteBomb_UpdateArmed(self, dt);
		break;
	case 1:
		CDemolition_RemoteBomb_UpdateGrow(self, dt);
		break;
	case 2:
		CDemolition_RemoteBomb_UpdateShrink(self, dt);
		break;
	}
	AI_Tick(s);
	Actor_Update(DB_PTR(s, 0x30));
	actor = (char *)DB_PTR(s, 0x30);
	baUpdateAnimations((struct BActorInstance *)(actor + 0x10), (struct BActorNode *)DB_PTR(actor, 0x144),
		DAT_0028fda8 + DAT_0028fda8, NULL);
	actor = (char *)DB_PTR(s, 0x30);
	axis[0] = 0.0f;
	axis[1] = 1.0f;
	axis[2] = 0.0f;
	axis[3] = -(DB_F32(actor, 0x1e8) + 3.14159274f);
	bmAxisAngleToQuaternion(DB_FP(actor, 0xf0), axis);
	baPrepActorInstance((struct BActorInstance *)((char *)DB_PTR(s, 0x30) + 0x10), 1);
	baGetNodePosition((struct BActorInstance *)((char *)DB_PTR(s, 0x30) + 0x10),
		(struct BActorNode *)DB_PTR(s, 0x11dc), DB_FP(s, 0x1208));
	DB_F32(s, 0x1214) = 1.0f;
	for (i = 0; i < 2; i++)
	{
		char *glow = s + 0x11e0 + i * 0x14;

		if (i == 0)
		{
			DB_I16(s, 0x11ec) = 0x7f;
			DB_I16(s, 0x11ee) = 0x7f;
			DB_I16(s, 0x11f0) = 0x7f;
			DB_I16(s, 0x11f2) = 0x40;
			DB_F32(s, 0x11e0) = 60.0f;
		}
		else
		{
			DB_I16(glow, 0xc) = (short)DB_I32(s, 0x1178);
			DB_I16(glow, 0xe) = (short)DB_I32(s, 0x117c);
			DB_I16(glow, 0x10) = (short)DB_I32(s, 0x1180);
			DB_I16(glow, 0x12) = 0x7f;
			DB_F32(glow, 0) = 150.0f;
		}
		DB_F32(glow, 4) = 0.0f;
	}
	DB_F32(s, 0x1154) = DB_F32(s, 0x1144);
	DB_F32(s, 0x115c) = DB_F32(s, 0x114c);
}

/* 0x000f2990  Demolition/CDemolition_Bomb.cpp  (0x0026ae30 +0xec) DrawFx
   The two glow sprites at the "bombstate" node and the ground marker. */
void CDemolition_RemoteBomb_DrawFx(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;
	int i;

	if (DB_U8(s, 0x116e) == 0)
		return;
	bdSetWorldIdentity();
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	for (i = 0; i < 2; i++)
	{
		char *glow = s + 0x11e0 + i * 0x14;

		DB_F32(s, 0x1214) = 1.0f;
		bdDrawSprite3D(DB_FP(s, 0x1208), DB_F32(glow, 0), DB_F32(glow, 0), DB_F32(glow, 4),
			DB_PTR(glow, 8), 0, 0, DB_I16(glow, 0xc), DB_I16(glow, 0xe), DB_I16(glow, 0x10),
			DB_I16(glow, 0x12), NULL, 0.0f);
	}
	CDemolition_RemoteBomb_DrawMarker(self);
	bdPopRenderState(5);
	bdPopRenderState(6);
}

/* 0x000f2a40  Demolition/CDemolition_Bomb.cpp
   Places the bomb 150 units in front of its holder (on free ground if the world finds
   some within 180), facing away from him; it grows for 0.15 s. */
void CDemolition_RemoteBomb_Place(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;
	char *holder;
	float forward[4];
	float pos[4];
	float euler[4];
	void *world;
	float angle;

	/* w of pos and euler: uninitialised stack in the original (the position setters copy 4
	   floats); the port uses 1 and 0 */
	pos[3] = 1.0f;
	euler[3] = 0.0f;

	DB_U32(DB_PTR(s, 0x30), 0x220) |= 8;
	DB_U8(s, 0x116d) = 1;
	DB_U8(s, 0x116e) = 1;
	DB_I32(DB_PTR(s, 0x30), 0x218) = 0;
	DB_U8(s, 0x1218) = 1;
	DB_U8(s, 0x1219) = 1;
	AI_SetState(s, 0);
	forward[0] = 0.0f;
	forward[1] = 0.0f;
	forward[2] = 150.0f;
	forward[3] = 1.0f;
	bmVectorRotateY(pos, forward, DB_F32(DB_PTR(DB_PTR(s, 0x11d8), 0x30), 0x1d8));
	holder = (char *)DB_PTR(s, 0x11d8);
	pos[0] = pos[0] + DB_F32(holder, 0x1144);
	pos[1] = pos[1] + DB_F32(holder, 0x1148);
	pos[2] = pos[2] + DB_F32(holder, 0x114c);
	world = DB_PTR(DAT_002ff85c, 0x79a0);
	world = DB_VFN(world, 0x34, void *(*)(void *))(world);
	if (db_world_find_spot(world, pos, 180.0f) != 0)
	{
		angle = (float)(atan2(pos[0], pos[2]) + 3.14159274f);
		bmVectorRotateY(pos, forward, angle);
		holder = (char *)DB_PTR(s, 0x11d8);
		pos[0] = pos[0] + DB_F32(holder, 0x1144);
		pos[1] = pos[1] + DB_F32(holder, 0x1148);
		pos[2] = pos[2] + DB_F32(holder, 0x114c);
		CDemolition_CharLib_SetPosition((struct CDemolition_CharLib *)s, pos);
		euler[0] = 0.0f;
		euler[1] = angle;
	}
	else
	{
		CDemolition_CharLib_SetPosition((struct CDemolition_CharLib *)s, pos);
		euler[0] = 0.0f;
		euler[1] = DB_F32(DB_PTR(DB_PTR(s, 0x11d8), 0x30), 0x1e8);
	}
	euler[2] = 0.0f;
	CDemolition_CharLib_SetRotation((struct CDemolition_CharLib *)s, euler);
	DB_I32(s, 0x1190) = 1;
	db_timer_start(s + 0x1220, db_bitsf(0x3e19999a));   /* 0.15 */
	db_sound(0xb, 0x7f);
}

/* 0x000f2c20  Demolition/CDemolition_Bomb.cpp
   The remote bomb goes off (40.5 above its position); its holder's pad rumbles by how close
   he is (full at 0, none from 3000). */
void CDemolition_RemoteBomb_Detonate(struct CDemolition_RemoteBomb *self, void *source, void *owner)
{
	char *s = (char *)self;
	char *holder;
	float pos[4];
	float dx;
	float dz;
	float strength;

	DB_U8(s, 0x116d) = 0;
	DB_U8(s, 0x116e) = 0;
	DB_I32(DB_PTR(s, 0x30), 0x218) = 0;
	DB_U32(DB_PTR(s, 0x30), 0x220) &= 0xfffffff7u;
	pos[0] = DB_F32(s, 0x1144);
	pos[1] = DB_F32(s, 0x1148) + 40.5f;
	pos[2] = DB_F32(s, 0x114c);
	pos[3] = 1.0f;   /* uninitialised stack in the original */
	DB_I32(s, 0x1190) = 0;
	db_sound(db_rand() % 4, 0xff);
	holder = (char *)DB_PTR(s, 0x11d8);
	dx = DB_F32(s, 0x1144) - DB_F32(holder, 0x1144);
	dz = DB_F32(s, 0x114c) - DB_F32(holder, 0x114c);
	strength = 1.0f - sqrtf(dx * dx + dz * dz) * 0.00033333333f;
	if (strength > 1.0f)
		strength = 1.0f;
	else if (strength < 0.0f)
		strength = 0.0f;
	db_rumble(DB_I32(holder, 0x1184), strength);
	db_explode_at(source, owner, pos);
}

/* 0x000f2d80  Demolition/CDemolition_Bomb.cpp
   Rolling into another player (or his armed remote bomb) within 130 sets the bomb off.
   Returns 1 if it went off. */
int CDemolition_RemoteBomb_CheckContact(struct CDemolition_RemoteBomb *self)
{
	char *s = (char *)self;
	char *first = DB_GAME_PLAYERS();
	char *p;

	if (DB_U8(DAT_002ff85c, 0x25) != 0)
		return 0;
	for (p = first; p != NULL; p = DB_PLAYER_NEXT(p))
	{
		if (p == (char *)DB_PTR(s, 0x11d8))
			continue;
		if (db_dist3(DB_FP(s, 0x1144), DB_FP(p, 0x1144)) < 130.0f)
		{
			CDemolition_RemoteBomb_Detonate(self, NULL, DB_PTR(s, 0x11d8));
			return 1;
		}
	}
	for (p = first; p != NULL; p = DB_PLAYER_NEXT(p))
	{
		char *other;

		if (p == (char *)DB_PTR(s, 0x11d8))
			continue;
		other = (char *)DB_PTR(p, 0x11a0);
		if (DB_U8(other, 0x116d) == 0)
			continue;
		if (db_dist3(DB_FP(s, 0x1144), DB_FP(other, 0x1144)) < 130.0f)
		{
			CDemolition_RemoteBomb_Detonate(self, NULL, DB_PTR(s, 0x11d8));
			return 1;
		}
	}
	return 0;
}

/* 0x000f2ea0  Demolition/CDemolition_Bomb.cpp  (0x0026ae30 +0x9c)
   AI state 1 (rolling): plays "brun", pushes at 0.7 of the top speed along the stick,
   plays the rolling sounds at frames 44 and 59 of the animation, and goes off on contact
   or when the actor touches something (actor +0x218). */
int CDemolition_RemoteBomb_StateRoll(struct CDemolition_RemoteBomb *self, int entered)
{
	char *s = (char *)self;
	char *actor;
	float push;
	int frame;

	if (entered != 0)
		Actor_PlayAnim(DB_PTR(s, 0x30), 1, 1, 0, 0.0f, 0.1f, 1);
	actor = (char *)DB_PTR(s, 0x30);
	DB_F32(s, 0x54) = 0.7f;
	DB_F32(actor, 0x23c) = DB_F32(DB_PTR(actor, 0x324), 0x84) * DB_F32(actor, 0x254) / DAT_0028fda8;
	push = DB_F32(s, 0x10b0) * DB_F32(s, 0x54);
	DB_F32(s, 0x7c) = push * DB_F32(s, 0x44);
	DB_F32(s, 0x80) = push * DB_F32(s, 0x48);
	DB_F32(s, 0x84) = push * DB_F32(s, 0x4c);
	DB_F32(DB_PTR(s, 0x30), 0x1d8) = CLMath_HeadingXZ(DB_FP(s, 0x7c));
	frame = DB_I32(DB_PTR(DB_PTR(s, 0x30), 0x144), 0x74) / 1024;
	if (frame >= 0x2c && frame < 0x3b)
	{
		if (DB_U8(s, 0x1218) != 0)
		{
			db_sound(7, 0x7f);
			DB_U8(s, 0x1218) = 0;
			DB_U8(s, 0x1219) = 1;
		}
	}
	else if (frame >= 0x3b && frame < 0x46)
	{
		if (DB_U8(s, 0x1219) != 0)
		{
			db_sound(6, 0x7f);
			DB_U8(s, 0x1219) = 0;
			DB_U8(s, 0x1218) = 1;
		}
	}
	if (DB_U8(DAT_002ff85c, 0x25) == 0 && DB_F32(s, 0x1198) <= 0.0f &&
		CDemolition_RemoteBomb_CheckContact(self) == 0 && DB_I32(DB_PTR(s, 0x30), 0x218) != 0)
		CDemolition_RemoteBomb_Detonate(self, NULL, DB_PTR(s, 0x11d8));
	return 0;
}
