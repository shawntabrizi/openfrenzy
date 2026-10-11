/*
Demolition/unplaced.c  <-  C:\BlitzParty\Game\Src\Demolition\ (file not known)

Gap functions of the demolition_b module (between the tagged files of the Demolition
directory). Each says which file it probably belongs to.
*/

#include "demolition_b.h"
#include "demolition_b_internal.h"

/* ====================================================================================
   Explosions (probably CDemolition_Explosion.cpp: these follow 0x000f0b20, the last
   function tagged with that file).

   CDemolition_Explosion (demolition_a; started by 0x000eff40):
     +0x10 next (in the manager's list)   +0x14 position (x, y, z, w)
     +0x24 age   +0x28 active   +0x29 visible
     +0x2c, +0xa4, +0x104, +0x170  effect parts (vtable: +4 Update(dt), +8 Draw())
     +0x70, +0xe0, +0x140, +0x1ac  "part running" flags
     +0x740 blast timer (rollerball_a timer)   +0x778 power
     +0x77c owner (the player who placed the bomb)
     +0x780 frag counter object (the first explosion of a chain; +0x784 frags so far)

   The explosion manager (`this` below):
     +0x08 first explosion   +0x10 two flash lights of 0x60 bytes:
       +0x08 light (CDemolition_Light)   +0x10 timer   +0x48 active   +0x4c colour   +0x5c age

   The player (CDemolition_Player, this file's Player functions):
     +0x1130 message receiver   +0x1140 next player   +0x1144 position   +0x1168 height
     +0x116c alive   +0x116d (remote bombs) armed   +0x1178/0x117c/0x1180 colour (r, g, b)
     +0x1184 index   +0x118c statistics (vtable +0x18 Fragged(victim), +0x1c FraggedBy(killer);
     +0x3c int[] frags by player)   +0x1194 score   +0x1198 hit cooldown   +0x11a0 remote bomb
     +0x1234 team mate
   ==================================================================================== */

/* 0x000ebe70 (demolition_a)  damages the world objects within `radius` of `pos` */
static void db_damage_world(float *pos, float radius)
{
	DB_DUAL(FUN_000ebe70, void (*)(int, undefined4, undefined4), void (*)(void *, float *, float),
		((int)(DAT_002ff85c + 0x79a4), (undefined4)pos, db_fbits(radius)),
		(DAT_002ff85c + 0x79a4, pos, radius));
}

/* the hit message an explosion sends to a player it hit */
static void db_explosion_hit_message(char *explosion, char *player)
{
	struct DbMessage message;

	FUN_0010cec0((undefined4 *)&message);
	message.pos[0] = DB_F32(explosion, 0x14);
	message.pos[1] = DB_F32(explosion, 0x18);
	message.pos[2] = DB_F32(explosion, 0x1c);
	message.type = 0;
	message.mode = 2;
	message.kind = 1;
	message.pos[3] = DB_F32(explosion, 0x20);
	message.value = DB_F32(explosion, 0x778);
	db_message_send(player + 0x1130, &message);
	FUN_0010ced0((undefined4 *)&message);
}

/* Scores a player caught in a frag/remote explosion (the code is the same in
   DamageObjectsFrag and DamageObjectsRemote; only the error text differs). Hitting yourself
   or your team mate costs the owner a point; the k-th frag of one chain gives 3 * k. */
static void db_explosion_frag(char *self, char *victim, const char *error)
{
	char *owner = (char *)DB_PTR(self, 0x77c);
	char *frags = (char *)DB_PTR(self, 0x780);
	float pos[4];
	const char *text;
	int bonus;

	DB_F32(victim, 0x1198) = 100.0f;
	pos[0] = DB_F32(victim, 0x1144);
	pos[1] = DB_F32(victim, 0x1148);
	pos[2] = DB_F32(victim, 0x114c);
	pos[3] = DB_F32(victim, 0x1150);
	pos[1] = DB_F32(victim, 0x1168) + pos[1];
	if (victim == owner || victim == (char *)DB_PTR(owner, 0x1234))
	{
		DB_I32(owner, 0x1194)--;
		owner = (char *)DB_PTR(self, 0x77c);
		db_popup(pos, DB_I32(owner, 0x1178), DB_I32(owner, 0x117c), DB_I32(owner, 0x1180), "-1");
	}
	else
	{
		void *ownerStats = DB_PTR(owner, 0x118c);
		void *victimStats = DB_PTR(victim, 0x118c);

		DB_I32(victimStats, 0x3c + DB_I32(owner, 0x1184) * 4)++;
		if (DB_I32(frags, 0x784) == 0)
			DB_VFN(ownerStats, 0x18, void (*)(void *, void *))(ownerStats, victim);
		DB_VFN(victimStats, 0x1c, void (*)(void *, void *))(victimStats, DB_PTR(self, 0x77c));
		frags = (char *)DB_PTR(self, 0x780);
		DB_I32(frags, 0x784)++;
		switch (DB_I32(DB_PTR(self, 0x780), 0x784))
		{
		case 1: bonus = 3; text = "3"; break;
		case 2: bonus = 6; text = "6"; break;
		case 3: bonus = 9; text = "9"; break;
		default:
			bkPrintf(error);
			text = NULL;
			break;
		}
		if (text != NULL)
		{
			owner = (char *)DB_PTR(self, 0x77c);
			DB_I32(owner, 0x1194) += bonus;
			owner = (char *)DB_PTR(self, 0x77c);
			db_popup(pos, DB_I32(owner, 0x1178), DB_I32(owner, 0x117c), DB_I32(owner, 0x1180), text);
		}
	}
	db_explosion_hit_message(self, victim);
}

/* 0x000f0030  CDemolition_Explosion::DamageObjectsFrag (variation 0)
   Not in the function table: Ghidra folded it into 0x000f0d10, which jumps to it.
   Sets off the bombs within `radius`, damages the world, and frags the players. */
static void CDemolition_Explosion_DamageObjectsFrag(char *self, float radius)
{
	char *bomb;
	char *player;

	for (bomb = (char *)DB_PTR(DB_PTR(DAT_002ff85c, 0x13e8), 4); bomb != NULL;
		bomb = (char *)DB_PTR(bomb, 0x10))
	{
		if (DB_U8(bomb, 0x15) != 0 && DB_I32(bomb, 0x4454) != 1 &&
			db_dist3(DB_FP(self, 0x14), DB_FP(bomb, 0x43a4)) < radius)
			CDemolition_FragBomb_Detonate((struct CDemolition_FragBomb *)bomb, DB_PTR(self, 0x780),
				DB_PTR(self, 0x77c));
	}
	db_damage_world(DB_FP(self, 0x14), radius);
	player = DB_GAME_PLAYERS();
	if (DB_PTR(self, 0x77c) == NULL)
		return;
	for (; player != NULL; player = DB_PLAYER_NEXT(player))
	{
		if (DB_F32(player, 0x1198) <= 0.0f &&
			db_dist3(DB_FP(self, 0x14), DB_FP(player, 0x1144)) < radius)
			db_explosion_frag(self, player,
				"CDemolition_Explosion::DamageObjectsFrag: Apparently we fragged more that three "
				"players then!!!");
	}
}

/* 0x000f0380  CDemolition_Explosion::DamageObjectsRemote (variation 1)
   Not in the function table (folded into 0x000f0d10 by Ghidra).
   Lets the world react (game +0x79a0, virtual +0x2c), sets off the armed remote bombs of
   the players within `radius`, then frags the players. */
static void CDemolition_Explosion_DamageObjectsRemote(char *self, float radius)
{
	void *world = DB_PTR(DAT_002ff85c, 0x79a0);
	char *first;
	char *player;

	DB_VFN(world, 0x2c, void (*)(void *, float *, float))(world, DB_FP(self, 0x14), radius);
	first = DB_GAME_PLAYERS();
	if (first == NULL)
		return;
	for (player = first; player != NULL; player = DB_PLAYER_NEXT(player))
	{
		char *remote = (char *)DB_PTR(player, 0x11a0);

		if (DB_U8(remote, 0x116d) != 0 &&
			db_dist3(DB_FP(self, 0x14), DB_FP(remote, 0x1144)) < radius)
			CDemolition_RemoteBomb_Detonate((struct CDemolition_RemoteBomb *)remote,
				DB_PTR(self, 0x780), DB_PTR(self, 0x77c));
	}
	for (player = first; player != NULL; player = DB_PLAYER_NEXT(player))
	{
		if (DB_F32(player, 0x1198) <= 0.0f &&
			db_dist3(DB_FP(self, 0x14), DB_FP(player, 0x1144)) < radius)
			db_explosion_frag(self, player,
				"CDemolition_Explosion::DamageObjectsRemote: Apparently we fragged more that three "
				"players then!!!");
	}
}

/* 0x000f06b0  CDemolition_Explosion::DamageObjects (variation 2: score loss)
   Not in the function table (folded into 0x000f0d10 by Ghidra).
   Sets off the 8 bombs of the bomb system within `radius`; a player hit loses a quarter of
   his score (shown as "-n"). */
static void CDemolition_Explosion_DamageObjectsLoss(char *self, float radius)
{
	char *bombs = (char *)DB_PTR(DAT_002ff85c, 0x13e8) + 0x10;
	char *player;
	int i;

	for (i = 0; i < 8; i++)
	{
		char *bomb = bombs + i * 0x2b0;

		if (DB_U8(bomb, 0x1c0) != 0 && db_dist3(DB_FP(self, 0x14), DB_FP(bomb, 0x180)) < radius)
			CDemolition_TimedBomb_Explode((struct CDemolition_TimedBomb *)bomb);
	}
	for (player = DB_GAME_PLAYERS(); player != NULL; player = DB_PLAYER_NEXT(player))
	{
		float pos[4];
		int score;
		int lost;

		if (!(DB_F32(player, 0x1198) <= 0.0f) ||
			!(db_dist3(DB_FP(self, 0x14), DB_FP(player, 0x1144)) < radius))
			continue;
		DB_F32(player, 0x1198) = 100.0f;
		pos[0] = DB_F32(player, 0x1144);
		pos[1] = DB_F32(player, 0x1148);
		pos[2] = DB_F32(player, 0x114c);
		pos[3] = DB_F32(player, 0x1150);
		pos[1] = DB_F32(player, 0x1168) + pos[1];
		score = DB_I32(player, 0x1194);
		lost = score - (int)((float)score * 0.75f);
		score -= lost;
		DB_I32(player, 0x1194) = score;
		if (score < 0)
			DB_I32(player, 0x1194) = 0;
		if (lost > 0)
			db_popup_int(pos, DB_I32(player, 0x1178), DB_I32(player, 0x117c), DB_I32(player, 0x1180),
				"-%d", lost);
		db_explosion_hit_message(self, player);
	}
}

/* 0x000f08e0  CDemolition_Explosion::DamageObjects (variation 3: last man standing)
   Not in the function table (folded into 0x000f0d10 by Ghidra).
   Sets off the bombs within `radius`; a player hit loses one life. Stops as soon as at most
   one player has lives left. */
static void CDemolition_Explosion_DamageObjectsLives(char *self, float radius)
{
	char *bomb;
	char *first;
	char *player;
	int alive;

	for (bomb = (char *)DB_PTR(DB_PTR(DAT_002ff85c, 0x13e8), 4); bomb != NULL;
		bomb = (char *)DB_PTR(bomb, 0x10))
	{
		if (DB_U8(bomb, 0x15) != 0 && DB_I32(bomb, 0x4454) != 1 &&
			db_dist3(DB_FP(self, 0x14), DB_FP(bomb, 0x43a4)) < radius)
			CDemolition_CrossBomb_Detonate((struct CDemolition_CrossBomb *)bomb, DB_PTR(self, 0x780),
				DB_PTR(self, 0x77c));
	}
	first = DB_GAME_PLAYERS();
	alive = 0;
	if (first == NULL)
		return;
	for (player = first; player != NULL; player = DB_PLAYER_NEXT(player))
	{
		if (DB_I32(player, 0x1194) > 0)
			alive++;
	}
	for (player = first; player != NULL; player = DB_PLAYER_NEXT(player))
	{
		int lives;

		if (DB_U8(player, 0x116c) == 0 || !(DB_F32(player, 0x1198) <= 0.0f))
			continue;
		lives = DB_I32(player, 0x1194);
		if (lives <= 0 || !(db_dist3(DB_FP(self, 0x14), DB_FP(player, 0x1144)) < radius))
			continue;
		DB_F32(player, 0x1198) = 100.0f;
		DB_I32(player, 0x1194) = lives - 1;
		if (lives - 1 <= 0)
			alive--;
		if (alive <= 1)
			return;
		if (DB_PTR(self, 0x77c) != NULL)
			DB_I32(DB_PTR(player, 0x118c), 0x3c + DB_I32(DB_PTR(self, 0x77c), 0x1184) * 4)++;
		db_explosion_hit_message(self, player);
	}
}

/* 0x000f0bf0  Demolition (probably CDemolition_Explosion.cpp)
   Starts an explosion: takes a free explosion of the list (else the oldest), a free flash
   light (else the oldest) lit with `pos`, then starts the explosion
   (CDemolition_Explosion::Start, 0x000eff40). */
void CDemolition_ExplosionMgr_Start(struct CDemolition_ExplosionMgr *self, void *source,
	void *owner, float *pos, float time, float power)
{
	char *explosion;
	char *free = NULL;
	char *oldest = (char *)pos;   /* the original leaves it at this stack value; never used
	                                 while the list has an explosion */
	char *light = NULL;
	float best;
	int i;

	best = 0.0f;
	for (explosion = (char *)DB_PTR(self, 8); explosion != NULL;
		explosion = (char *)DB_PTR(explosion, 0x10))
	{
		if (DB_U8(explosion, 0x28) == 0)
		{
			free = explosion;
			break;
		}
		if (best < DB_F32(explosion, 0x24))
		{
			oldest = explosion;
			best = DB_F32(explosion, 0x24);
		}
	}
	best = 0.0f;
	for (i = 0; i < 2; i++)
	{
		char *slot = (char *)self + 0x10 + i * 0x60;

		if (DB_U8(slot, 0x48) == 0)
		{
			light = slot;
			break;
		}
		if (best <= DB_F32(slot, 0x5c))
		{
			light = slot;
			best = DB_F32(slot, 0x5c);
		}
	}
	DB_U8(light, 0x48) = 1;
	DB_F32(light, 0x5c) = 0.0f;
	DB_F32(light, 0x4c) = pos[0];
	DB_F32(light, 0x50) = pos[1];
	DB_F32(light, 0x54) = pos[2];
	DB_F32(light, 0x58) = pos[3];
	FUN_000edbc0((int)DB_PTR(light, 8), (undefined4 *)pos);
	DB_DUAL(FUN_0018fa30, void (*)(int, undefined4), void (*)(void *, float),
		((int)DB_PTR(light, 8), 0x3903126f), (DB_PTR(light, 8), db_bitsf(0x3903126f)));
	FUN_0018fa10((void *)DB_PTR(light, 8), 0x7f, 0, 0);
	DB_VFN(DB_PTR(light, 8), 0xc, void (*)(void *, int))(DB_PTR(light, 8), 1);
	db_timer_start(light + 0x10, 0.3f);
	if (free == NULL)
		free = oldest;
	DB_DUAL(FUN_000eff40,
		void (*)(int, int, undefined4, undefined4 *, undefined4, undefined4),
		void (*)(void *, void *, void *, float *, float, float),
		((int)free, (int)source, (undefined4)owner, (undefined4 *)pos, db_fbits(time), db_fbits(power)),
		(free, source, owner, pos, time, power));
}

/* 0x000f0d10  CDemolition_Explosion::DamageObjects (probably CDemolition_Explosion.cpp)
   Dispatches on the game variation (game +0x28) to the four functions above. */
void CDemolition_Explosion_DamageObjects(struct CDemolition_Explosion *self, float radius)
{
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
		CDemolition_Explosion_DamageObjectsFrag((char *)self, radius);
		break;
	case 1:
		CDemolition_Explosion_DamageObjectsRemote((char *)self, radius);
		break;
	case 2:
		CDemolition_Explosion_DamageObjectsLoss((char *)self, radius);
		break;
	case 3:
		CDemolition_Explosion_DamageObjectsLives((char *)self, radius);
		break;
	}
}

/* 0x000f0d50  Demolition (probably CDemolition_Explosion.cpp)
   Draws the visible explosions (their four parts, and the blast while it grows). */
void CDemolition_ExplosionMgr_Draw(struct CDemolition_ExplosionMgr *self)
{
	char *e;

	for (e = (char *)DB_PTR(self, 8); e != NULL; e = (char *)DB_PTR(e, 0x10))
	{
		if (DB_U8(e, 0x29) == 0)
			continue;
		DB_VFN(e + 0x2c, 8, void (*)(void *))(e + 0x2c);
		DB_VFN(e + 0xa4, 8, void (*)(void *))(e + 0xa4);
		DB_VFN(e + 0x104, 8, void (*)(void *))(e + 0x104);
		DB_VFN(e + 0x170, 8, void (*)(void *))(e + 0x170);
		if (FUN_0018ea50((void *)(e + 0x740)) > 0.0f)
			FUN_000efd50((int)e);
	}
}

/* 0x000f0dc0  CDemolition_Explosion::Update (probably CDemolition_Explosion.cpp)
   While the blast grows it damages everything within (blast fraction * power). The
   explosion ends when none of its four parts runs any more. */
void CDemolition_Explosion_Update(struct CDemolition_Explosion *self, float dt)
{
	char *e = (char *)self;
	int running;

	if (DB_U8(e, 0x28) == 0)
		return;
	FUN_0018e8e0((void *)(e + 0x740));
	if (FUN_0018ea50((void *)(e + 0x740)) > 0.0f && DB_U8(DAT_002ff85c, 0x25) == 0)
		CDemolition_Explosion_DamageObjects(self,
			BCarTimer_GetRemaining((struct BCarTimer *)(e + 0x740)) * DB_F32(e, 0x778));
	DB_F32(e, 0x24) = dt + DB_F32(e, 0x24);
	DB_VFN(e + 0x2c, 4, void (*)(void *, float))(e + 0x2c, dt);
	DB_VFN(e + 0xa4, 4, void (*)(void *, float))(e + 0xa4, dt);
	DB_VFN(e + 0x104, 4, void (*)(void *, float))(e + 0x104, dt);
	DB_VFN(e + 0x170, 4, void (*)(void *, float))(e + 0x170, dt);
	running = DB_U8(e, 0x70) != 0;
	if (DB_U8(e, 0xe0) != 0)
		running++;
	if (DB_U8(e, 0x140) != 0)
		running++;
	if (DB_U8(e, 0x1ac) != 0)
		running++;
	if (running == 0)
	{
		DB_U8(e, 0x28) = 0;
		DB_U8(e, 0x29) = 0;
	}
}

/* 0x000f0ea0  Demolition (probably CDemolition_Explosion.cpp)
   Updates the explosions and the two flash lights. */
void CDemolition_ExplosionMgr_Update(struct CDemolition_ExplosionMgr *self, float dt)
{
	char *e;
	int i;

	for (e = (char *)DB_PTR(self, 8); e != NULL; e = (char *)DB_PTR(e, 0x10))
		CDemolition_Explosion_Update((struct CDemolition_Explosion *)e, dt);
	for (i = 0; i < 2; i++)
		FUN_000ef940((int)((char *)self + 0x10 + i * 0x60), dt);
}

/* 0x000f0ee0  Demolition (the bomb effects of CDemolition_Bomb.cpp; a gap function)
   Destructor of the effects block of the frag, timed and cross bombs: the sparks (+0x50)
   and the fuse timer (+0x18). */
void CDemolition_BombFx_Destruct(struct CDemolition_BombFx *self)
{
	char *s = (char *)self;

	CSparkPointSystem_Destruct((struct CSparkPointSystem *)(s + 0x50));
	CDemoCountdown_Dtor((unsigned int *)(s + 0x18));
}

/* ====================================================================================
   CDemolition_Object (vtable 0x0026b0d4): the physics object the bombs derive from
   (probably CDemolition_Object.cpp; these follow the tagged CDemolition_Bomb.cpp).

   It extends demolition_d's base (0x0010bd30, 0x43a0 bytes) with a CDemoBody at +0x43a0
   (demolition_a: CDemoSimulation steps the bodies):
     +0x438c spawn position   +0x43a4 position   +0x43c4 velocity   +0x43d4 acceleration
     +0x43e4 contact normal   +0x4404 mass   +0x4414 gravity   +0x4424 applied force
     +0x4434 drag   +0x4438 sliding friction   +0x443c rolling friction
   ==================================================================================== */

/* 0x000f52c0  constructor */
struct CDemolition_Object *CDemolition_Object_Construct(struct CDemolition_Object *self)
{
	char *s = (char *)self;

	FUN_0010bd30((undefined4 *)s);
	DB_PTR(s, 0) = (void *)vtable_0026b0d4;
	CDemoBody_Ctor(s + 0x43a0);
	return self;
}

/* 0x000f5310  destructor */
void CDemolition_Object_Destruct(struct CDemolition_Object *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026b0d4;
	CDemoBody_Dtor(s + 0x43a0);
	FUN_0010bd50((undefined4 *)s);
}

/* 0x000f5370  (called by CDemoSimulation_Step, demolition_a)
   Integrates the acceleration: velocity += acceleration * dt * mass. */
void CDemolition_Object_Integrate(struct CDemolition_Object *self, float dt)
{
	char *s = (char *)self;
	float k = dt * DB_F32(s, 0x4404);

	DB_F32(s, 0x43c4) = k * DB_F32(s, 0x43d4) + DB_F32(s, 0x43c4);
	DB_F32(s, 0x43c8) = k * DB_F32(s, 0x43d8) + DB_F32(s, 0x43c8);
	DB_F32(s, 0x43cc) = k * DB_F32(s, 0x43dc) + DB_F32(s, 0x43cc);
}

/* 0x000f53c0  (called by CDemoSimulation_Step after the step)
   Clears the applied force. */
void CDemolition_Object_Stop(struct CDemolition_Object *self)
{
	char *s = (char *)self;

	DB_F32(s, 0x4424) = 0.0f;
	DB_F32(s, 0x4428) = 0.0f;
	DB_F32(s, 0x442c) = 0.0f;
}

/* ====================================================================================
   The object list of the game (game +0x1398, vtable 0x0026b0e8): a list of objects
   (linked at +0x10) that are updated (virtual +8) and drawn (virtual +0xc).
   ==================================================================================== */

/* 0x000f53e0  constructor */
void CDemolition_ObjectList_Construct(struct CDemolition_ObjectList *self)
{
	DB_PTR(self, 0) = (void *)vtable_0026b0e8;
}

/* 0x000f53f0  destructor */
void CDemolition_ObjectList_Destruct(struct CDemolition_ObjectList *self)
{
	DB_PTR(self, 0) = (void *)vtable_0026b0e8;
}

/* 0x000f5400  Init(package): an empty list. Always succeeds. */
int CDemolition_ObjectList_Init(struct CDemolition_ObjectList *self, void *package)
{
	(void)package;
	DB_PTR(self, 4) = NULL;
	return 1;
}

/* 0x000f5410  updates every object (virtual +8, Update(dt)) */
void CDemolition_ObjectList_Update(struct CDemolition_ObjectList *self, float dt)
{
	char *object;

	for (object = (char *)DB_PTR(self, 4); object != NULL; object = (char *)DB_PTR(object, 0x10))
		DB_VFN(object, 8, void (*)(void *, float))(object, dt);
}

/* 0x000f5440  draws every object (virtual +0xc) */
void CDemolition_ObjectList_Draw(struct CDemolition_ObjectList *self)
{
	char *object;

	for (object = (char *)DB_PTR(self, 4); object != NULL; object = (char *)DB_PTR(object, 0x10))
		DB_VFN(object, 0xc, void (*)(void *))(object);
}

/* 0x000f5460  CDemolition_Object: deleting destructor (vtable 0x0026b0d4 slot 0) */
void *CDemolition_Object_DeletingDestruct(struct CDemolition_Object *self, unsigned int flags)
{
	CDemolition_Object_Destruct(self);
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000f5480  sets the position of the body and the spawn position */
void CDemolition_Object_SetPosition(struct CDemolition_Object *self, float *pos)
{
	char *s = (char *)self;

	CDemoBody_SetPosition(s + 0x43a0, pos);
	DB_F32(s, 0x438c) = pos[0];
	DB_F32(s, 0x4390) = pos[1];
	DB_F32(s, 0x4394) = pos[2];
	DB_F32(s, 0x4398) = pos[3];
}

/* 0x000f54c0  (called by CDemoSimulation_Step before the step; dt is not used)
   The acceleration of the body: drag, gravity / mass, then the friction along the contact
   plane (the sliding friction above a speed of 0.03, else a rolling friction that grows
   with the speed), then the applied force. */
void CDemolition_Object_ComputeForces(struct CDemolition_Object *self, float dt)
{
	char *s = (char *)self;
	float invMass;
	float gravityY;
	float along;
	float slide[3];
	float speed;
	float k;

	(void)dt;
	DB_F32(s, 0x43d4) = 0.0f;
	DB_F32(s, 0x43d8) = 0.0f;
	DB_F32(s, 0x43dc) = 0.0f;
	DB_F32(s, 0x43d4) = -(DB_F32(s, 0x4434) * DB_F32(s, 0x43c4));
	DB_F32(s, 0x43d8) = -(DB_F32(s, 0x43c8) * DB_F32(s, 0x4434));
	DB_F32(s, 0x43dc) = -(DB_F32(s, 0x43cc) * DB_F32(s, 0x4434));
	invMass = 1.0f / DB_F32(s, 0x4404);
	DB_F32(s, 0x43d4) = invMass * DB_F32(s, 0x4414) + DB_F32(s, 0x43d4);
	DB_F32(s, 0x43d8) = invMass * DB_F32(s, 0x4418) + DB_F32(s, 0x43d8);
	DB_F32(s, 0x43dc) = invMass * DB_F32(s, 0x441c) + DB_F32(s, 0x43dc);
	gravityY = invMass * DB_F32(s, 0x4418);
	/* the velocity without its part along the contact normal */
	along = DB_F32(s, 0x43ec) * DB_F32(s, 0x43cc) + DB_F32(s, 0x43e8) * DB_F32(s, 0x43c8) +
		DB_F32(s, 0x43e4) * DB_F32(s, 0x43c4);
	slide[0] = DB_F32(s, 0x43c4) - along * DB_F32(s, 0x43e4);
	slide[1] = DB_F32(s, 0x43c8) - along * DB_F32(s, 0x43e8);
	slide[2] = DB_F32(s, 0x43cc) - along * DB_F32(s, 0x43ec);
	speed = slide[2] * slide[2] + slide[1] * slide[1] + slide[0] * slide[0];
	if (speed > 0.03f)
	{
		bmVectorNormalise(slide, slide);
		k = gravityY * DB_F32(s, 0x4438);
	}
	else
	{
		if (!(speed > 0.0f))
			goto applied;
		bmVectorNormalise(slide, slide);
		k = speed * 33.333336f * DB_F32(s, 0x443c) * gravityY;
	}
	DB_F32(s, 0x43d4) = slide[0] * k + DB_F32(s, 0x43d4);
	DB_F32(s, 0x43d8) = slide[1] * k + DB_F32(s, 0x43d8);
	DB_F32(s, 0x43dc) = slide[2] * k + DB_F32(s, 0x43dc);
applied:
	DB_F32(s, 0x43d4) = DB_F32(s, 0x43d4) + DB_F32(s, 0x4424);
	DB_F32(s, 0x43d8) = DB_F32(s, 0x43d8) + DB_F32(s, 0x4428);
	DB_F32(s, 0x43dc) = DB_F32(s, 0x43dc) + DB_F32(s, 0x442c);
}

/* 0x000f56e0  sets the applied force (4 floats) */
void CDemolition_Object_SetVelocity(struct CDemolition_Object *self, float *velocity)
{
	char *s = (char *)self;

	DB_F32(s, 0x4424) = velocity[0];
	DB_F32(s, 0x4428) = velocity[1];
	DB_F32(s, 0x442c) = velocity[2];
	DB_F32(s, 0x4430) = velocity[3];
}

/* 0x000f5710  CDemolition_ObjectList: deleting destructor (vtable 0x0026b0e8 slot 0) */
void *CDemolition_ObjectList_DeletingDestruct(struct CDemolition_ObjectList *self,
	unsigned int flags)
{
	DB_PTR(self, 0) = (void *)vtable_0026b0e8;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}
