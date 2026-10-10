/*
CarryOn/CCarryOn_Player.c  <-  C:\BlitzParty\Game\Src\CarryOn\CCarryOn_Player.cpp

The Fireworks player: a CharLib AI player (src/charlib/charlib_ai.h) that carries
fireworks (variants 1, 3, 4) or glitter (variant 2). The AI modes (CP_MODE, +0x04) are
run by CCarryOn_Player_Think (vtable slot 11):

  0 idle, 1 go and use the variant's goal (launch rack, glitter jar, station, conveyor),
  2 chase a player, 4 hit back, 5 wait, 6 go to AI_TARGET, 7 attack a player,
  9 collect (launch pads, glitter, parts)

The constructor 0x000c1ea0 is tagged with CCarryOn_Player.cpp; the vtable (0x00267b70)
and the call graph place the gap functions 0x000bf760-0x000c3150 here (file inferred).
*/

#include "carryon_a.h"

#define CP_ACTOR_OF(self) CO_PTR(self, CP_ACTOR)
#define CP_POS(self) CO_FP(CP_ACTOR_OF(self), CA_POS)
#define CP_PLAYER(self) CO_U16(CP_ACTOR_OF(self), CA_PLAYER)
#define CP_OTHER_POS(i) CO_FP(CO_PTR(g_coPlayers[i], CP_ACTOR), CA_POS)
/* the per-character table (classlib_fx/core, 0x60 bytes per character) */
#define CP_CHAR(player) (DAT_002baed4[player][0])
#define CP_CHAR_F32(player, off) (*(float *)&DAT_00290718[CP_CHAR(player) * 0x60 + (off)])
#define CP_CHAR_I32(player, off) (*(int *)&DAT_00290718[CP_CHAR(player) * 0x60 + (off)])

/* 0x000bf760  CarryOn/CCarryOn_Player.cpp (file inferred)
Plans a route to `target` (kept inside the route grid); while no route is found it moves
the target by `step` and tries again, 30 times at most. `unused` is not read. */
void CCarryOn_Player_PlanRoute(void *route, float *from, float *target, float unused, float *step)
{
	float limit;
	int i;

	for (i = 0; i < 30; i++)
	{
		if (DAT_00292c60 + 0.1f > target[0])            /* 0x002536cc */
			target[0] = DAT_00292c60 + 0.1f;
		if (DAT_00292c64 + 0.1f > target[2])
			target[2] = DAT_00292c64 + 0.1f;
		limit = DAT_00292c68 + DAT_00292c60 - 0.1f;
		if (limit < target[0])
			target[0] = limit;
		limit = DAT_00292c6c + DAT_00292c64 - 0.1f;
		if (limit < target[2])
			target[2] = limit;
		CLRoute_Start(route, from, target, from[1]);
		if (CO_I16(route, 0xfb6) != -1)
			return;
		target[0] = target[0] + step[0];
		target[1] = step[1] + target[1];
		target[2] = step[2] + target[2];
	}
}

/* 0x000bf850  CarryOn/CCarryOn_Player.cpp (file inferred)
Sets the frames between AI plans (AI_THINK_INTERVAL): the character's base time divided
by the difficulty, scaled by `scale` (and a random factor 0.5..1). Hard CPU players
(difficulty 2) plan every frame; in variant 2 better players plan twice as often. */
void CCarryOn_Player_SetThinkInterval(struct CCarryOnPlayer *self, float scale)
{
	if (CO_I32(g_fireRes, 0x250) == 1 && DAT_002bae70 > 0)
		scale = scale * 0.5f;
	if (DAT_002bae70 == 2)
	{
		CO_I32(self, 0x10b4) = 0;
		return;
	}
	if (scale > 0.0f)
		scale = scale * ((float)co_rand() * 3.0518509e-05f + 1.0f) * 0.5f;  /* 0x00253740 */
	CO_I32(self, 0x10b4) = (int)((float)CP_CHAR_I32(CP_PLAYER(self), 0x38)
		/ (float)(DAT_002bae70 * 5 + 5) * scale);
}

/* 0x000bf920  CarryOn/CCarryOn_Player.cpp (file inferred)
The last opponent (in player order) that carries more fireworks than this player (at
least 1). Returns -1 when there is none. (The distance it computes is not used.) */
int CCarryOn_Player_FindRicher(struct CCarryOnPlayer *self)
{
	int least;
	int found = -1;
	int i;

	least = self->fireworks > 1 ? self->fireworks : 1;
	for (i = 0; i < DAT_002bae94; i++)
	{
		if (i == CP_PLAYER(self))
			continue;
		if (DAT_002baebc != 0 && DAT_002baed4[i][8] == DAT_002baed4[CP_PLAYER(self)][8])
			continue;
		if (least < g_coPlayers[i]->fireworks)
		{
			bmVectorDistance(CP_POS(self), CP_OTHER_POS(i));
			found = i;
		}
	}
	return found;
}

/* 0x000bf9c0  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 1, mode 9: walks to a launch pad with a firework and takes it; with 4 fireworks
(or 3, sometimes) it goes to launch them instead; on the way it may switch to chasing a
richer player who is much nearer than the pad. Returns nonzero when the mode is done. */
int CCarryOn_Player_CollectPads(struct CCarryOnPlayer *self)
{
	void *item;
	float target[4];
	int state;
	int other;
	float toOther, toPad;

	if (self->fireworks > 3 || (self->fireworks > 2 && (co_rand() & 0xff) > 200))
	{
		self->goal = NULL;
		self->wantLaunch = 1;
		return 1;
	}
	if (self->goal != NULL)
	{
		if (self->fireworks > 3)
		{
			self->goal = NULL;
			self->wantLaunch = 1;
			return 1;
		}
		state = CO_I32(self->goal, 0x30);
		if (state > 2 && state < 7)
		{
			if (CO_F32(self, CP_THINK_TIME) >= 20.0f)     /* 0x00255530 */
			{
				other = CCarryOn_Player_FindRicher(self);
				if (other != -1)
				{
					target[0] = CO_F32(self->goal, 0x8);
					target[1] = CO_F32(self->goal, 0xc);
					target[2] = CO_F32(self->goal, 0x10);
					target[3] = CO_F32(self->goal, 0x14);
					toOther = bmVectorDistance(CP_POS(self), CP_OTHER_POS(other));
					toPad = bmVectorDistance(CP_POS(self), target);
					if (toOther < toPad * 0.25f)              /* 0x00257b88 */
					{
						self->chase = other;
						self->goal = NULL;
						return 1;
					}
				}
			}
			if (CO_I16(self, CP_ARRIVED) == 0)
				goto search;
			co_Pad_Use(self->goal, CP_PLAYER(self));
		}
		self->goal = NULL;
		return 1;
	}
search:
	for (;;)
	{
		item = co_Spawner_FindPad(g_fireGame.spawner, CP_POS(self), CP_PLAYER(self));
		if (item == NULL)
			return 1;
		if (item == self->goal)
			return CO_I16(self, CP_ARRIVED) != 0;
		self->goal = item;
		target[0] = CO_F32(item, 0x8);
		target[1] = CO_F32(item, 0xc);
		target[2] = CO_F32(item, 0x10);
		target[3] = CO_F32(item, 0x14);
		CCarryOn_Player_PlanRoute(CO_ADDR(self, CP_ROUTE), CP_POS(self), target,
			CO_F32(CP_ACTOR_OF(self), CA_POS + 4), &DAT_00293fc0[0]);
		self->hasRoute = CO_I16(self, CP_ROUTE_NODE) != -1;
		if (CO_I16(self, CP_ROUTE_NODE) != -1 || CO_I32(item, 0x30) != 6)
			return CO_I16(self, CP_ARRIVED) != 0;
		CO_I32(item, 0x30) = 7;                         /* unreachable */
	}
}

/* 0x000bfc00  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 3: the two part types the player still needs for its station's firework. */
void CCarryOn_Player_ChooseWanted(struct CCarryOnPlayer *self)
{
	int *station = self->station;
	int r;

	switch (station[1])
	{
	case 0:
		r = co_rand() % 3;
		self->wantType[0] = DAT_00293fd0[r][0];
		self->wantType[1] = DAT_00293fd0[r][1];
		break;
	case 1:
		self->wantType[0] = station[2];
		self->wantType[1] = station[2] == 0;
		break;
	case 2:
		if (station[2] == station[3])
		{
			self->wantType[0] = station[2];
			self->wantType[1] = station[2] == 0;
		}
		else if (station[2] != 0 && station[3] != 0)
		{
			self->wantType[0] = 1;
			self->wantType[1] = 3;
		}
		else
		{
			self->wantType[0] = 0;
			self->wantType[1] = station[2] == 0 ? station[3] : station[2];
		}
		break;
	}
}

/* 0x000bfcd0  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 3: the last opponent that carries a part of a type this player wants (-1). */
int CCarryOn_Player_FindWantedCarrier(struct CCarryOnPlayer *self)
{
	struct CCarryOnPlayer *other;
	int found = -1;
	int i;

	CCarryOn_Player_ChooseWanted(self);
	for (i = 0; i < DAT_002bae94; i++)
	{
		if (i == CP_PLAYER(self))
			continue;
		if (DAT_002baebc != 0 && DAT_002baed4[i][8] == DAT_002baed4[CP_PLAYER(self)][8])
			continue;
		other = g_coPlayers[i];
		if (other->fireworks == 0)
			continue;
		if (other->partTypes[0] == self->wantType[0] || other->partTypes[0] == self->wantType[1])
		{
			bmVectorDistance(CP_POS(self), CP_OTHER_POS(i));
			found = i;
		}
	}
	return found;
}

/* 0x000bfd70  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 3: chooses the next mode: deliver the parts (1), collect (9), chase a carrier of
a wanted part (2) or attack someone (7). */
void CCarryOn_Player_DecideParts(struct CCarryOnPlayer *self)
{
	int other;

	self->hasTarget = 0;
	self->goal = NULL;
	CCarryOn_Player_ChooseWanted(self);
	if (self->fireworks > 0)
	{
		self->chase = -1;
		CCarryOn_Player_SetThinkInterval(self, 0.0f);
		self->hasTarget = 0;
		CO_I32(self, CP_MODE) = 1;
	}
	else if (self->chase == -1)
	{
		if (co_Spawner_HasLoad(g_fireGame.spawner, CP_PLAYER(self)) != 0)
		{
			self->chase = -1;
			self->goal = NULL;
			CO_I32(self, CP_MODE) = 9;
		}
		else if (CO_I32(self, 0x14) != 0 && (other = CCarryOn_Player_FindWantedCarrier(self)) != -1)
		{
			self->chase = other;
			self->routeAge = 100;
			CO_I32(self, CP_MODE) = 2;
		}
		else
		{
			self->chase = -1;
			CCarryOn_Player_SetThinkInterval(self, 0.0f);
			CO_I32(self, 0x18) = -1;
			CO_I32(self, CP_MODE) = 7;
		}
	}
	else
	{
		CCarryOn_Player_SetThinkInterval(self, 0.0f);
		self->routeAge = 100;
		CO_I32(self, CP_MODE) = 2;
	}
	if (CO_I32(self, CP_MODE) != 2)
		self->chase = -1;
}

/* 0x000bfe50  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 3, mode 9: collects a part from the spawner (preferring the wanted types when
aggressive), or switches to chasing a nearer carrier of a wanted part. */
int CCarryOn_Player_CollectParts(struct CCarryOnPlayer *self)
{
	struct CCarryOnPlayer *other;
	void *item;
	float target[4];
	float step[4];
	float scale;
	int found;

	if (self->fireworks > 0)
	{
		self->goal = NULL;
		return 1;
	}
	if (self->goal != NULL)
	{
		found = CCarryOn_Player_FindWantedCarrier(self);
		if (found != -1)
		{
			other = g_coPlayers[found];
			target[0] = CO_F32(self->goal, 0x8);
			target[1] = CO_F32(self->goal, 0xc);
			target[2] = CO_F32(self->goal, 0x10);
			target[3] = CO_F32(self->goal, 0x14);
			if (other->partTypes[0] == self->wantType[0])
				scale = CO_I32(self->goal, 0x34) == self->wantType[0] ? 0.75f : 1.0f;
			else
				scale = CO_I32(self->goal, 0x34) != self->wantType[0] ? 0.75f : 0.5f;
			if (bmVectorDistance(CP_POS(self), CP_OTHER_POS(found))
					< bmVectorDistance(CP_POS(self), target) * scale)
			{
				self->chase = found;
				self->goal = NULL;
				return 1;
			}
		}
		if (CO_I32(self->goal, 0x30) != 6 || self->fireworks >= 1)
		{
			self->goal = NULL;
			return 1;
		}
		if (CO_I16(self, CP_ARRIVED) != 0)
		{
			co_Pad_Use(self->goal, CP_PLAYER(self));
			self->goal = NULL;
			return 1;
		}
	}
	for (;;)
	{
		item = self->goal;
		if (item == NULL)
		{
			if (CO_I32(self, 0x14) != 0)
			{
				CCarryOn_Player_ChooseWanted(self);
				item = co_Spawner_FindType(g_fireGame.spawner, self->wantType[0]);
				if (item == NULL)
					item = co_Spawner_FindType(g_fireGame.spawner, self->wantType[1]);
			}
			if (item == NULL)
			{
				item = co_Spawner_FindAny(g_fireGame.spawner);
				if (item == NULL)
					return 1;
			}
		}
		if (item == self->goal)
			return CO_I16(self, CP_ARRIVED) != 0;
		self->goal = item;
		target[0] = CO_F32(item, 0x8);
		target[1] = CO_F32(item, 0xc);
		target[2] = CO_F32(item, 0x10);
		target[3] = CO_F32(item, 0x14);
		step[0] = CP_POS(self)[0] - target[0];
		step[1] = CP_POS(self)[1] - target[1];
		step[2] = CP_POS(self)[2] - target[2];
		bmVectorSetLength(step, step, 2.0f);
		CCarryOn_Player_PlanRoute(CO_ADDR(self, CP_ROUTE), CP_POS(self), target,
			CO_F32(CP_ACTOR_OF(self), CA_POS + 4), step);
		self->hasRoute = CO_I16(self, CP_ROUTE_NODE) != -1;
		if (CO_I16(self, CP_ROUTE_NODE) != -1)
			return CO_I16(self, CP_ARRIVED) != 0;
		if (CO_I32(item, 0x30) != 6)
			return 1;
		CO_I32(item, 0x30) = 7;
	}
}

/* 0x000c00d0  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 3, mode 1: takes the parts to the player's station (its position at +0x20) and
hands them over there. Returns nonzero when done. */
int CCarryOn_Player_DeliverParts(struct CCarryOnPlayer *self)
{
	float target[4];

	self->goal = NULL;
	if (self->fireworks == 0)
		return 1;
	if (self->hasTarget == 0)
	{
		target[0] = CO_F32(self->station, 0x20);
		target[1] = CO_F32(self->station, 0x24);
		target[2] = CO_F32(self->station, 0x28);
		target[3] = CO_F32(self->station, 0x2c);
		self->hasTarget = 1;
		self->target[0] = target[0];
		self->target[1] = target[1];
		self->target[2] = target[2];
		self->target[3] = target[3];
		CCarryOn_Player_PlanRoute(CO_ADDR(self, CP_ROUTE), CP_POS(self), target,
			CO_F32(CP_ACTOR_OF(self), CA_POS + 4), &DAT_00293fe8[0]);
		self->hasRoute = CO_I16(self, CP_ROUTE_NODE) != -1;
	}
	else if (CO_I16(self, CP_ARRIVED) != 0)
	{
		self->hasTarget = 0;
		co_Station_Deliver(self->station);
		return 1;
	}
	return 0;
}

/* 0x000c01b0  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 2: the last opponent with more glitter than this player (at least 1), or -1. */
int CCarryOn_Player_FindRicherGlitter(struct CCarryOnPlayer *self)
{
	int least;
	int found = -1;
	int i;

	least = self->glitter > 1 ? self->glitter : 1;
	for (i = 0; i < DAT_002bae94; i++)
	{
		if (i == CP_PLAYER(self))
			continue;
		if (DAT_002baebc != 0 && DAT_002baed4[i][8] == DAT_002baed4[CP_PLAYER(self)][8])
			continue;
		if (least < g_coPlayers[i]->glitter)
		{
			bmVectorDistance(CP_POS(self), CP_OTHER_POS(i));
			found = i;
		}
	}
	return found;
}

/* 0x000c0250  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 2, mode 9: walks to a glitter pick-up that no other CPU player goes for (any
reachable one otherwise) and jumps for it when near. With more than 75 glitter it wants
to empty the jar. Returns nonzero when done (glitterFull: nothing to collect). */
int CCarryOn_Player_CollectGlitter(struct CCarryOnPlayer *self)
{
	void *items[8];
	void *found = NULL;
	float limit;
	float dx, dz;
	unsigned int chance;
	int count;
	int k, i;

	if (self->glitter > 75)
	{
		self->wantLaunch = 1;
		return 1;
	}
	if (DAT_002bae70 == 1)
		limit = 4.0f;                                       /* 0x00257c40 */
	else if (DAT_002bae70 == 2)
		limit = 2.0f;                                       /* 0x00257318 */
	else
		limit = 8.0f;                                       /* 0x0025552c */
	if (self->hasTarget != 0 && limit >= self->glitterHeld)
		goto near;
	self->glitterFull = 0;
	if (self->hasTarget == 0)
		self->pickup = NULL;
	else
		self->glitterHeld = 0.0f;
	count = co_Glitter_FindPickups(g_fireGame.glitter, CP_POS(self), self->pickup, items);
	if (count == 0)
	{
		self->glitterFull = 1;
		return 1;
	}
	/* a pick-up that no other CPU player is after */
	for (k = 0; k < count; k++)
	{
		for (i = 0; i < 4; i++)
			if (i != CP_PLAYER(self) && DAT_002baed4[i][1] == 0 && g_coPlayers[i]->pickup == items[k])
				break;
		if (i != 4)
			continue;
		self->target[0] = CO_F32(items[k], 0x4);
		self->target[1] = CO_F32(items[k], 0x8);
		self->target[2] = CO_F32(items[k], 0xc);
		self->target[3] = CO_F32(items[k], 0x10);
		CCarryOn_Player_PlanRoute(CO_ADDR(self, CP_ROUTE), CP_POS(self), self->target,
			CO_F32(CP_ACTOR_OF(self), CA_POS + 4), &DAT_00294008[0]);
		if (CO_I16(self, CP_ROUTE_NODE) != -1)
		{
			found = items[k];
			self->hasRoute = 1;
			if (found != NULL)
				goto chosen;
			break;
		}
	}
	/* any reachable one */
	for (k = 0; k < count; k++)
	{
		self->target[0] = CO_F32(items[k], 0x4);
		self->target[1] = CO_F32(items[k], 0x8);
		self->target[2] = CO_F32(items[k], 0xc);
		self->target[3] = CO_F32(items[k], 0x10);
		CCarryOn_Player_PlanRoute(CO_ADDR(self, CP_ROUTE), CP_POS(self), self->target,
			CO_F32(CP_ACTOR_OF(self), CA_POS + 4), &DAT_00293ff8[0]);
		if (CO_I16(self, CP_ROUTE_NODE) != -1)
		{
			found = items[k];
			self->hasRoute = 1;
			break;
		}
	}
chosen:
	if (k == count)
	{
		self->glitterFull = 1;
		return 1;
	}
	self->hasTarget = 1;
	self->pickup = found;
near:
	if (self->pickup != NULL)
	{
		dx = CP_POS(self)[0] - CO_F32(self->pickup, 0x4);
		dz = CP_POS(self)[2] - CO_F32(self->pickup, 0xc);
		if (dx * dx + dz * dz < 8100.0f)                    /* 0x00267ad4 */
		{
			CO_I16(self, CP_ARRIVED) = 1;
			if (DAT_002bae70 == 1)
				chance = 8;
			else if (DAT_002bae70 == 2)
				chance = 0x10;
			else
				chance = 0;
			if ((unsigned int)(co_rand() & 0xff) < chance)
				CO_I32(CO_PTR(self, 0x68), 0x14) = 1;           /* press jump */
		}
	}
	return 0;
}

/* 0x000c0510  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 2, mode 1: takes the glitter to the jar (the belt item at +0x11a0) and empties
it there. Returns nonzero when done. */
int CCarryOn_Player_EmptyGlitter(struct CCarryOnPlayer *self)
{
	float target[4];

	if (self->glitter == 0)
		return 1;
	if (self->hasTarget == 0)
	{
		target[0] = CO_F32(self->belt, 0xc);
		target[1] = CO_F32(self->belt, 0x10);
		target[2] = CO_F32(self->belt, 0x14);
		target[3] = CO_F32(self->belt, 0x18);
		self->hasTarget = 1;
		self->target[0] = target[0];
		self->target[1] = target[1];
		self->target[2] = target[2];
		self->target[3] = target[3];
		CCarryOn_Player_PlanRoute(CO_ADDR(self, CP_ROUTE), CP_POS(self), target,
			CO_F32(CP_ACTOR_OF(self), CA_POS + 4), &DAT_00294018[0]);
		self->hasRoute = CO_I16(self, CP_ROUTE_NODE) != -1;
	}
	else if (CO_I16(self, CP_ARRIVED) != 0)
	{
		self->hasTarget = 0;
		return 1;
	}
	return 0;
}

/* 0x000c0600  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 4, mode 1 and 9: stands by the conveyor and jumps when one of its own fireworks
(or, failing that, any firework on the first belt with one of its own on the second)
comes by; with fireworks in hand it sometimes uses "action" when any firework passes.
Easy players (difficulty 0) just press buttons at random. Always returns 0. */
int CCarryOn_Player_WorkConveyor(struct CCarryOnPlayer *self)
{
	void *item;
	float t, offset, at;

	if (DAT_002bae70 == 0)
	{
		if ((co_rand() & 0xff) < 5)
			CO_I32(CO_PTR(self, 0x68), 0x14) = 1;           /* jump */
		if ((co_rand() & 0xff) != 0xff)
			return 0;
		CO_I32(CO_PTR(self, 0x6c), 0x14) = 1;               /* action */
		return 0;
	}
	item = co_Conveyor_FindNearest(g_fireGame.paint, CP_POS(self), 0);
	t = CFire_Belt_GetT(g_fireGame.conveyor[0], **(float **)CO_PTR(item, 0x3c));
	if (DAT_002bae70 == 2)
		offset = 0.0f;
	else
		offset = (float)co_rand() * 2.746666e-06f - 0.06f;  /* 0x00267ae4, 0x00267ae0 */
	at = offset + t + 0.075f;                               /* 0x00267adc */
	if (CFire_Belt_HasItemAt(g_fireGame.conveyor[0], at, 0.1f, CP_PLAYER(self) + 1) == 0)
	{
		if (CFire_Belt_HasItemAt(g_fireGame.conveyor[0], at, 1.0f, 0) == 0)
			goto done;
		t = CFire_Belt_GetT(g_fireGame.conveyor[1], **(float **)CO_PTR(item, 0x3c));
		if (CFire_Belt_HasItemAt(g_fireGame.conveyor[1], t + offset + 0.14285715f, 0.1f,
				CP_PLAYER(self) + 1) == 0)                  /* 0x00267ad8 */
			goto done;
	}
	CO_I32(CO_PTR(self, 0x68), 0x14) = 1;                   /* jump */
done:
	if (CO_I32(CO_PTR(self, 0x68), 0x14) == 0 && (co_rand() & 0xff) < self->fireworks * 5)
	{
		item = co_Conveyor_FindNearest(g_fireGame.paint, CP_POS(self), 1);
		CFire_Belt_GetT(g_fireGame.conveyor[0], **(float **)CO_PTR(item, 0x3c));
		if (CFire_Belt_HasItemAt(g_fireGame.conveyor[0], at, 0.1f, -1) != 0)
			CO_I32(CO_PTR(self, 0x6c), 0x14) = 1;           /* action */
	}
	return 0;
}

/* 0x000c0810  CarryOn/CCarryOn_Player.cpp (file inferred)
Whether `other` is in front of this player and inside its punch range (the character's
reach, larger while it holds something). */
int CCarryOn_Player_CanHit(struct CCarryOnPlayer *self, int other)
{
	float angle;
	float dist;
	float reach;

	angle = CLMath_AngleDistance(CLMath_HeadingTo(CP_POS(self), CP_OTHER_POS(other)),
		CO_F32(CP_ACTOR_OF(self), CA_YAW));
	if (angle < CO_F32(self, 0x10fc))
	{
		dist = bmVectorDistance(CP_POS(self), CP_OTHER_POS(other));
		reach = CP_CHAR_F32(CP_PLAYER(self), 0x10);
		if (CO_F32(self, 0x10a4) != 0.0f)
			reach = reach * DAT_00293a38;
		if ((angle + 1.0f) * dist < reach * 0.5f)
			return 1;
	}
	return 0;
}

/* 0x000c0900  CarryOn/CCarryOn_Player.cpp (file inferred)
Creates every player's input map: "Forward", "leftRight", "jump", "action" on the pad,
and "jumpcheat" in a map of its own. */
void CCarryOn_Player_CreateControls(void)
{
	struct CCarryOnPlayer *p;
	unsigned int pad;
	int i;

	for (i = 0; i < DAT_002bae94; i++)
	{
		p = g_coPlayers[i];
		pad = (unsigned int)DAT_002baed4[i][3];
		CO_PTR(p, 0x5c) = biCreateMap(4);
		CO_PTR(g_coPlayers[i], 0x60) = biCreateChannel(CO_PTR(g_coPlayers[i], 0x5c), "Forward", 3, 0, 0, 0, 0);
		CO_PTR(g_coPlayers[i], 0x64) = biCreateChannel(CO_PTR(g_coPlayers[i], 0x5c), "leftRight", 3, 0, 0, 0, 0);
		CO_PTR(g_coPlayers[i], 0x68) = biCreateChannel(CO_PTR(g_coPlayers[i], 0x5c), "jump", 1, 1, 0, 0, 0);
		CO_PTR(g_coPlayers[i], 0x6c) = biCreateChannel(CO_PTR(g_coPlayers[i], 0x5c), "action", 1, 1, 0, 0, 0);
		CO_PTR(g_coPlayers[i], 0x74) = biCreateMap(1);
		CO_PTR(g_coPlayers[i], 0x78) = biCreateChannel(CO_PTR(g_coPlayers[i], 0x74), "jumpcheat", 1, 0, 0, 0, 0);
		biBindInput(CO_PTR(g_coPlayers[i], 0x60), pad | 0xbe, pad | 0xbd);
		biBindInput(CO_PTR(g_coPlayers[i], 0x64), pad | 0xc0, pad | 0xbf);
		biBindInput(CO_PTR(g_coPlayers[i], 0x60), pad | 0xba, 0);
		biBindInput(CO_PTR(g_coPlayers[i], 0x64), pad | 0xb9, 0);
		biBindInput(CO_PTR(g_coPlayers[i], 0x68), pad | 0xc5, 0);
		biBindInput(CO_PTR(g_coPlayers[i], 0x6c), pad | 199, 0);
		biBindInput(CO_PTR(g_coPlayers[i], 0x6c), pad | 0xc6, 0);
	}
}

/* 0x000c0af0  CarryOn/CCarryOn_Player.cpp (file inferred)
Gives the player `count` fireworks of part type `type` (5 at most in variant 1, else 1).
`how` 0: picked up (plays the pick-up sound, sometimes a voice), 1: taken from another
player. In variant 3 a voice cheers when the station asks for two parts of this type.
Returns 0 when the player cannot carry more. */
int CCarryOn_Player_GiveFireworks(struct CCarryOnPlayer *self, int how, int type, int count)
{
	int most;
	int voice = -1;

	most = (CO_I32(g_fireRes, 0x250) == 0) * 4 + 1;
	if (self->fireworks >= most)
		return 0;
	if (how == 0)
	{
		CFire_Resource_FindSample(g_fireRes, 0xe, -1, 0x80);
		if ((unsigned int)(co_rand() & 0xff)
				< (unsigned int)((CO_I32(g_fireRes, 0x250) == 2 ? 0x12 : 0) + 2))
			voice = 5;
		self->collected = self->collected + count;
	}
	else if (how == 1)
		self->taken = self->taken + count;
	if (CO_I32(g_fireRes, 0x250) == 2 && self->station[1] == 2 && self->station[2] == type
			&& self->station[3] == type)
		voice = 0xb;
	else if (voice == -1)
		goto give;
	AI_PlayVoice(self, voice, DAT_00293928, 180.0f, 0, -1, 0);
give:
	while (--count >= 0 && self->fireworks < most)
	{
		self->partTypes[self->fireworks] = type;
		self->fireworks = self->fireworks + 1;
	}
	return 1;
}

/* 0x000c0c40  CarryOn/CCarryOn_Player.cpp (file inferred)
Adds to the score while the game is being played. */
void CCarryOn_Player_AddScore(struct CCarryOnPlayer *self, int points)
{
	if (g_fireGame.state == CO_STATE_PLAY)
		self->score = self->score + points;
}

/* 0x000c0c60  CarryOn/CCarryOn_Player.cpp (file inferred)
While playing: shows "<points>" rising from the player, in the colour of `player` (this
player when -1). */
void CCarryOn_Player_ShowScore(struct CCarryOnPlayer *self, int points, int player)
{
	char text[12];
	unsigned char *rgb;
	float *pos;

	if (g_fireGame.state != CO_STATE_PLAY)
		return;
	if (player == -1)
		player = CP_PLAYER(self);
	sprintf(text, "%d", points);
	rgb = &DAT_00290718[DAT_002baed4[player][0] * 0x60];
	pos = CP_POS(self);
	baAddExtraItem(*(struct BExtraSystem **)DAT_002f27c4, CO_PTR(g_fireRes, 0x10),
		(double)pos[0], (double)pos[1], (double)pos[2],
		(unsigned int)rgb[0] * 2 / 2, (unsigned int)rgb[1] * 2 / 2, (unsigned int)rgb[2] * 2 / 2,
		text);
}

/* 0x000c0d30  CarryOn/CCarryOn_Player.cpp (file inferred)
Takes fireworks away (`count` of them, all when -1). `how` 0: hit (in variant 2 it also
drops part of the glitter), 1: knocked out (all glitter too), 2/3: everything, 4: they
are delivered (variant 1: rockets into the launch rack for points; variant 3: the
station). */
void CCarryOn_Player_LoseFireworks(struct CCarryOnPlayer *self, int how, int count)
{
	static const int launchSample[5] = { 0, 1, 2, 3, 3 };  /* 0x00267b10 */
	float pos[4];
	void *node;
	int drop;
	int n;

	switch (how)
	{
	case 0:
		if (CO_I32(g_fireRes, 0x250) == 1 && self->glitter > 0)
		{
			drop = (int)((float)self->glitter * DAT_00293f18);
			node = baFindNode(CO_PTR(CP_ACTOR_OF(self), 0x144),
				*(const char **)&DAT_00290718[DAT_002baed4[CP_PLAYER(self)][0] * 0x60 - 0x10]);
			baGetNodePosition(CO_ADDR(CP_ACTOR_OF(self), 0x10), node, pos);
			co_Glitter_Drop(g_fireGame.glitter, CP_PLAYER(self), pos, drop);
			self->glitter = self->glitter - drop;
			if (self->glitterTimer < DAT_00293f1c)
				self->glitterTimer = DAT_00293f1c;
			CFire_Resource_FindSample(g_fireRes, 8, 2, 0x80);
		}
		if (count != -1)
		{
			self->fireworks = self->fireworks - count;
			if (self->fireworks >= 0)
				return;
		}
		self->fireworks = 0;
		return;
	case 1:
		if (count == -1)
			self->fireworks = 0;
		else
		{
			self->fireworks = self->fireworks - count;
			if (self->fireworks < 0)
				self->fireworks = 0;
		}
		self->glitter = 0;
		if (DAT_00293f14 != 0)
			self->int1144 = 0;
		break;
	case 2:
	case 3:
		self->fireworks = 0;
		self->glitter = 0;
		return;
	case 4:
		if (CO_I32(g_fireRes, 0x250) == 0)
		{
			if (self->fireworks == 0 || co_Rack_HasRoom(g_fireGame.launcher, self->fireworks) == 0)
				return;
			CCarryOn_Player_AddScore(self, DAT_00294028[self->fireworks]);
			CCarryOn_Player_ShowScore(self, DAT_00294028[self->fireworks], -1);
			if (self->fireworks == 5)
				AI_PlayVoice(self, 4, DAT_00293928, 0.0f, 0, -1, 0);
			CFire_Resource_FindSample(g_fireRes, 0xc, -1, 0x80);
			for (n = self->fireworks; n > 0; n--)
				if (co_Rack_AddRocket(g_fireGame.launcher, CP_PLAYER(self) + 1) != 0)
					self->fireworks = self->fireworks - 1;
		}
		else if (CO_I32(g_fireRes, 0x250) == 2)
		{
			if (self->fireworks != 0)
				CFire_Resource_FindSample(g_fireRes, 0xc, launchSample[self->partTypes[0]], 0x80);
			for (n = self->fireworks - 1; n >= 0; n--)
				self->fireworks = self->fireworks - 1;
		}
		break;
	}
}

/* 0x000c0fd0  CarryOn/CCarryOn_Player.cpp (file inferred)
"Debug Lines" for CPU players: a line to the next route point (white with a route, red
without) and a circle in the colour of the AI mode; then the HUD marker over the
player. */
void CCarryOn_Player_DrawEffects(struct CCarryOnPlayer *self)
{
	float line[2][4];
	void *actor = CP_ACTOR_OF(self);

	if (DAT_002f6d60 != 0 && DAT_002baed4[CO_U16(actor, CA_PLAYER)][1] == 0)
	{
		line[0][0] = CO_F32(actor, 0xd0);
		line[0][1] = CO_F32(actor, 0xd4);
		line[0][2] = CO_F32(actor, 0xd8);
		line[0][3] = CO_F32(actor, 0xdc);
		line[1][0] = CO_F32(self, 0x1058);
		line[1][1] = CO_F32(self, 0x105c);
		line[1][2] = CO_F32(self, 0x1060);
		line[1][3] = CO_F32(self, 0x1064);
		CFire_Resource_DrawLines(&line[0][0], 2, 0,
			((-(unsigned int)(self->hasRoute != 0)) & 0xffff) - 0x10000);
		CFire_Resource_DrawCircle(CO_FP(actor, CA_POS), 15.0f, 0,
			DAT_00294040[CO_I32(self, CP_MODE)]);
	}
	bdSetWorldMatrix(DAT_002855f0);
	co_Hud_DrawPlayerMarker(CO_U16(CP_ACTOR_OF(self), CA_PLAYER),
		CO_FP(CP_ACTOR_OF(self), CA_POS), NULL); /* ECX = DAT_002ecfac, unused */
}

/* 0x000c10d0  CarryOn/CCarryOn_Player.cpp (file inferred)
A random opponent (not on this player's team); with `awake` set, not one that is hurt
or knocked out (-1 when every opponent is). */
int CCarryOn_Player_PickRandomOpponent(struct CCarryOnPlayer *self, int awake)
{
	unsigned int me;
	int i;
	int ok;

	if (awake != 0)
	{
		me = CP_PLAYER(self);
		for (i = 0; i < DAT_002bae94; i++)
		{
			if (i != (int)me && (DAT_002baebc == 0 || DAT_002baed4[i][8] != DAT_002baed4[me][8])
					&& CO_I16(g_coPlayers[i], CP_STATE) != 7 && CO_I16(g_coPlayers[i], CP_STATE) != 9)
				break;
		}
		if (i == DAT_002bae94)
			return -1;
	}
	do
	{
		i = co_rand() % DAT_002bae94;
		me = CP_PLAYER(self);
		ok = i != (int)me;
		if (awake != 0 && (CO_I16(g_coPlayers[i], CP_STATE) == 7 || CO_I16(g_coPlayers[i], CP_STATE) == 9))
			ok = 0;
	} while ((DAT_002baebc != 0 && DAT_002baed4[i][8] == DAT_002baed4[me][8]) || !ok);
	return i;
}

/* 0x000c11e0  CarryOn/CCarryOn_Player.cpp (file inferred)
The opponent that hit this player most often (awake ones only when `awake`), else a
random one. */
int CCarryOn_Player_PickWorstEnemy(struct CCarryOnPlayer *self, int awake)
{
	short most = 0;
	int found = -1;
	unsigned int me;
	int i;

	if (DAT_002bae94 > 0)
	{
		me = CP_PLAYER(self);
		for (i = 0; i < DAT_002bae94; i++)
		{
			if (i != (int)me
					&& (awake == 0 || (CO_I16(g_coPlayers[i], CP_STATE) != 7
						&& CO_I16(g_coPlayers[i], CP_STATE) != 9))
					&& (DAT_002baebc == 0 || DAT_002baed4[i][8] != DAT_002baed4[me][8])
					&& most < CO_I16(self, CP_HITS + i * 2))
			{
				most = CO_I16(self, CP_HITS + i * 2);
				found = i;
			}
		}
		if (found != -1)
			return found;
	}
	return CCarryOn_Player_PickRandomOpponent(self, awake);
}

/* 0x000c12a0  CarryOn/CCarryOn_Player.cpp (file inferred)
A hit of strength `damage`. Mode 0: `from` is the attacker's position (the player turns
to face it); modes 1 and 2: `from` holds the hit's angle relative to the facing as a
float bit pattern (callers pass any pointer: the angle is then tiny and counts as from
the front), mode 1 knocks backwards. Enough damage knocks the player out (AI state 9),
otherwise it is hurt (state 7). */
void CCarryOn_Player_Hit(struct CCarryOnPlayer *self, void *from, float damage, int mode)
{
	unsigned int bits = (unsigned int)from;
	float angle;
	float heading = 0.0f;

	memcpy(&angle, &bits, sizeof angle);
	if (mode == 0)
	{
		heading = CLMath_HeadingTo(CP_POS(self), (float *)from);
		angle = CLMath_AngleDistance(heading, CO_F32(CP_ACTOR_OF(self), CA_YAW));
	}
	CO_F32(self, 0x111c) = damage + CO_F32(self, 0x111c);
	if (CO_F32(self, 0x111c) <= CP_CHAR_F32(CP_PLAYER(self), 0x20))
	{
		AI_SetState(self, 7);
		if (angle <= 1.5707963f && mode != 1)               /* 0x00253898 */
		{
			Actor_PlayAnim(CP_ACTOR_OF(self), 0x11, 0, 0, 1.0f, 0.1f, 1);
			return;
		}
		Actor_PlayAnim(CP_ACTOR_OF(self), 0x12, 0, 0, 1.0f, 0.1f, 1);
		return;
	}
	AI_SetState(self, 9);
	CO_F32(self, 0x111c) = 0.0f;
	if (angle > 1.5707963f || mode == 1)
	{
		CO_I32(self, 0x1120) = 0;
		Actor_PlayAnim(CP_ACTOR_OF(self), 0x14, 0, 0, 1.0f, 0.1f, 1);
		if (mode == 0)
			CO_F32(CP_ACTOR_OF(self), 0x1d8) = CLMath_WrapAngle(heading + 3.1415925f);
	}
	else
	{
		CO_I32(self, 0x1120) = 1;
		Actor_PlayAnim(CP_ACTOR_OF(self), 0x13, 0, 0, 1.0f, 0.1f, 1);
		if (mode == 0)
			CO_F32(CP_ACTOR_OF(self), 0x1d8) = heading;
	}
}

/* 0x000c1440  CarryOn/CCarryOn_Player.cpp (vtable slot 0, before the AI update)
The burden (AI_SKILL, +0x10e0) of what the player carries slows it down: per firework
in variant 1, per glitter in variant 2 (CPU players by difficulty), a part in variant 3.
Keeps each weight tweak so that it plus "Basic Burden" stays at most 1. */
void CCarryOn_Player_UpdateBurden(struct CCarryOnPlayer *self)
{
	float basic = DAT_002f6d3c;
	float weight = DAT_00293f3c;

	if (CO_I32(g_fireRes, 0x250) == 1 && DAT_002baed4[CP_PLAYER(self)][1] == 0)
	{
		basic = DAT_0029409c[DAT_002bae70];
		weight = DAT_00294090[DAT_002bae70];
	}
	if (DAT_00293f38 + DAT_002f6d3c > 1.0f)
		DAT_00293f38 = 1.0f - DAT_002f6d3c;
	if (weight + basic > 1.0f)
		weight = 1.0f - basic;
	if (DAT_00293f40 + DAT_002f6d3c > 1.0f)
		DAT_00293f40 = 1.0f - DAT_002f6d3c;
	if (DAT_00293f44 + DAT_002f6d3c > 1.0f)
		DAT_00293f44 = 1.0f - DAT_002f6d3c;
	CO_F32(self, 0x10e0) = 0.0f;
	switch (CO_I32(g_fireRes, 0x250))
	{
	case 0:
		switch (self->fireworks)
		{
		case 1:
			CO_F32(self, 0x10e0) = DAT_00293f38 * 0.2f;
			return;
		case 2:
			CO_F32(self, 0x10e0) = DAT_00293f38 * 0.4f;
			return;
		case 3:
			CO_F32(self, 0x10e0) = DAT_00293f38 * 0.6f;
			return;
		case 4:
			CO_F32(self, 0x10e0) = DAT_00293f38 * 0.8f;
			return;
		case 5:
			CO_F32(self, 0x10e0) = DAT_00293f38;
			return;
		}
		break;
	case 1:
		CO_F32(self, 0x10e0) = (float)self->glitter * weight * 0.01f + basic;
		return;
	case 2:
		if (self->fireworks != 0)
			CO_F32(self, 0x10e0) = DAT_00293f40;
		break;
	case 3:
		CO_F32(self, 0x10e0) = DAT_00293f44;
		break;
	}
}

/* 0x000c1650  CarryOn/CCarryOn_Player.cpp (vtable slot 1, after the AI update)
A fall (actor flag 0x8000) knocks the player down (random direction, huge damage); runs
the two cool-down timers; times the bully's target (DAT_00293f48). */
void CCarryOn_Player_UpdatePost(struct CCarryOnPlayer *self)
{
	if (CP_PLAYER(self) == (unsigned int)DAT_00293f48)
		DAT_002f6d40 = DAT_002f6d40 + CO_F32(g_fireRes, 0xb4);
	if ((signed char)(CO_U32(CP_ACTOR_OF(self), 0x220) >> 8) < 0
			&& CO_I16(self, CP_STATE) != 7 && CO_I16(self, CP_STATE) != 9)
		CCarryOn_Player_Hit(self, DAT_002ea478, 10000.0f, (co_rand() & 1) + 1);
	if (self->glitterTimer != 0.0f)
	{
		self->glitterTimer = self->glitterTimer - CO_F32(g_fireRes, 0xb4);
		if (self->glitterTimer < 0.0f)
			self->glitterTimer = 0.0f;
	}
	if (self->timer113c != 0.0f)
	{
		self->timer113c = self->timer113c - CO_F32(g_fireRes, 0xb4);
		if (self->timer113c < 0.0f)
			self->timer113c = 0.0f;
	}
}

/* 0x000c1760  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 2, every frame: the glitter jar follows the carried jar; new glitter makes it
sparkle and its sound louder (it fades again otherwise), the rumble follows the sound;
the jar animation runs while the player walks. */
void CCarryOn_Player_Update(struct CCarryOnPlayer *self)
{
	float vel[4];
	unsigned char *rgb;
	void *held;
	unsigned int player;

	if (CO_I32(g_fireRes, 0x250) != 1)
		return;
	held = CFire_Carry_GetHeld(DAT_002f6d4c, CP_PLAYER(self));
	if (held != NULL)
	{
		self->jarPos[0] = CO_F32(held, 0xc0);
		self->jarPos[1] = CO_F32(held, 0xc4);
		self->jarPos[2] = CO_F32(held, 0xc8);
		self->jarPos[3] = CO_F32(held, 0xcc);
	}
	if (self->lastGlitter < self->glitter)
	{
		self->glitterHeld = 0.0f;
		self->volume = self->volume + 0x1e;
		if (self->volume > 0xff)
			self->volume = 0xff;
		vel[0] = (float)co_rand() * 0.12207404f - 2000.0f;     /* 0x00267b28, 0x00257c68 */
		vel[1] = (float)co_rand() * 0.09155553f - 1000.0f;     /* 0x00267b24, 0x00255898 */
		vel[2] = (float)co_rand() * 0.12207404f - 2000.0f;
		vel[3] = 1.0f;
		if (held != NULL)
		{
			rgb = &DAT_00290718[DAT_002baed4[CP_PLAYER(self)][0] * 0x60];
			co_Sparks_Stream(CO_PTR(DAT_002f7968, 0xc), vel, self->jarPos,
				(int)rgb[0] * 2 / 2, (int)rgb[1] * 2 / 2, (int)rgb[2] * 2 / 2);
		}
	}
	else
	{
		self->glitterHeld = CO_F32(g_fireRes, 0xb4) + self->glitterHeld;
		if (self->volume != 0)
		{
			self->volume = self->volume - 10;
			if (self->volume < 0)
				self->volume = 0;
		}
	}
	self->lastGlitter = self->glitter;
	bsSetChannelVolume(self->sample, (unsigned short)self->volume);
	player = CP_PLAYER(self);
	if (DAT_002baed4[player][1] != 0 && DAT_002bae7c != 0)
		co_Rumble_SetMotors(DAT_002ed324, player, 3, self->volume * 0xffff / 0xff, 0);
	switch (CO_U16(self, CP_STATE))
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 6:
	case 0xe:
		baFlushAnimations(CO_ADDR(CP_ACTOR_OF(self), 0x10), self->jarNode, 1, 0);
		baQueueAnimation(CO_ADDR(CP_ACTOR_OF(self), 0x10), self->jarNode, self->jarObject,
			0x400, -1, 0, NULL, NULL);
		self->jarAnim = 1;
		return;
	default:
		self->jarAnim = 0;
		return;
	}
}

/* 0x000c1a60  CarryOn/CCarryOn_Player.cpp (file inferred)
Draws every actor: in full ("PlayerDetail") or as a flat silhouette in its player's
colour. */
void CCarryOn_Player_DrawAll(void)
{
	unsigned int saved[30];
	unsigned char *rgb;
	unsigned int node;

	bdSaveRenderStates(saved);
	Actor_PrepAll();
	bdSetWorldMatrix(DAT_002855f0);
	co_Nothing();
	for (node = CLList_Next(DAT_002ea488, NULL); node != 0;
			node = CLList_Next(DAT_002ea488, (unsigned int *)node))
	{
		if (CO_I32(node, 0x21c) == 0)
			continue;
		if (DAT_0029411c == 0)
		{
			rgb = &DAT_00290718[DAT_002baed4[CO_U16(node, CA_PLAYER)][0] * 0x60];
			CFire_Resource_DrawSkeleton(CO_ADDR(node, 0x10),
				(unsigned int)rgb[1] << 9 | (unsigned int)rgb[2] << 1 | (unsigned int)rgb[0] << 17
					| 0xff000000);
		}
		else
			Actor_DrawFirstPass((void *)node);
	}
	bdRestoreRenderStates(saved);
}

/* 0x000c1b30  CarryOn/CCarryOn_Player.cpp (file inferred)
Runs the AI of the CPU players (all players at the end): players that fell off the
world respawn; finds the "bully" (variants 1 and 3: the player that took the most
fireworks from others rather than picking them up); then CharLib's AI tick. */
void CCarryOn_Player_UpdateControls(struct CFireResource *res)
{
	struct CCarryOnPlayer *p;
	int bully = -1;
	int most = 0;
	int i;

	(void)res;
	for (i = 0; i < DAT_002bae94; i++)
	{
		p = g_coPlayers[i];
		if (CO_F32(CO_PTR(p, CP_ACTOR), 0xd4) <= -1990.0f)    /* 0x00267b2c */
			CO_VFN(g_coPlayers[i], 0x24, void (*)(void *))(g_coPlayers[i]);
		if (DAT_002baed4[i][1] == 0 || g_fireGame.state == CO_STATE_END)
			CO_VFN(g_coPlayers[i], 0xc, void (*)(void *))(g_coPlayers[i]);
		if (CO_I32(g_fireRes, 0x250) == 0)
		{
			if (g_coPlayers[i]->collected + g_coPlayers[i]->taken >= 8
					&& most < g_coPlayers[i]->taken - g_coPlayers[i]->collected)
			{
				bully = i;
				most = g_coPlayers[i]->taken - g_coPlayers[i]->collected;
			}
		}
		else if (CO_I32(g_fireRes, 0x250) == 2)
		{
			if (g_coPlayers[i]->collected + g_coPlayers[i]->taken >= 3
					&& most < g_coPlayers[i]->taken - g_coPlayers[i]->collected)
			{
				bully = i;
				most = g_coPlayers[i]->taken - g_coPlayers[i]->collected;
			}
		}
	}
	DAT_00293f4c = bully;
	AI_TickAll();
}

/* 0x000c1c10  CarryOn/CCarryOn_Player.cpp (vtable slot 17)
Switches the actor between its two model instances (the alternative one is used when
`alt` is set and there is no demo camera), carrying the animation and position over. */
void CCarryOn_Player_SetAltModel(struct CCarryOnPlayer *self, int alt)
{
	unsigned int *from;
	unsigned int *to;

	if (DAT_002f26c4 != NULL)
		alt = 0;
	if (alt == self->altModel)
		return;
	self->altModel = alt;
	if (alt == 0)
	{
		from = self->instance[0];
		to = self->instance[1];
	}
	else
	{
		from = self->instance[1];
		to = self->instance[0];
	}
	memcpy(from, CO_ADDR(CP_ACTOR_OF(self), 0x10), 0x5c * 4);
	baCopyAnimationState((struct BActorInstance *)to, (struct BActorNode *)to[0x4d],
		(struct BActorInstance *)from, (struct BActorNode *)from[0x4d], 1);
	Core_SetCharacterColours(to, DAT_00283ea4);
	to[0x30] = from[0x30];
	to[0x31] = from[0x31];
	to[0x32] = from[0x32];
	to[0x33] = from[0x33];
	to[0x38] = from[0x38];
	to[0x39] = from[0x39];
	to[0x3a] = from[0x3a];
	to[0x3b] = from[0x3b];
	baPrepActorInstance((struct BActorInstance *)to, 1);
	memcpy(CO_ADDR(CP_ACTOR_OF(self), 0x10), to, 0x5c * 4);
	CFire_Carry_UpdateHeld(DAT_002f6d4c, CP_PLAYER(self));
	self->jarNode = baFindNode(CO_PTR(CP_ACTOR_OF(self), 0x144), "guts");
}

/* 0x000c1d50  CarryOn/CCarryOn_Player.cpp (file inferred)
Whether this player (or its team) leads: 1 alone in the lead, 0 sharing it (or a team
draw), -1 behind. `count` (may be NULL) receives how many share the lead (2 / 4 for
teams). */
int CCarryOn_Player_IsLeading(struct CCarryOnPlayer *self, int *count)
{
	int team[2];
	int best;
	int n;
	int i;

	if (DAT_002baebc == 0)
	{
		best = g_coPlayers[0]->score;
		for (i = 1; i < DAT_002bae94; i++)
			if (best < g_coPlayers[i]->score)
				best = g_coPlayers[i]->score;
		n = 0;
		for (i = 0; i < DAT_002bae94; i++)
			if (g_coPlayers[i]->score == best)
				n++;
		if (count != NULL)
			*count = n;
		if (self->score != best)
			return -1;
		return n == 1;
	}
	team[0] = 0;
	team[1] = 0;
	if (DAT_002bae94 > 0)
	{
		for (i = 0; i < DAT_002bae94; i++)
			team[DAT_002baed4[i][8]] += g_coPlayers[i]->score;
		if (team[0] != team[1])
		{
			if ((unsigned int)DAT_002baed4[CP_PLAYER(self)][8] == (unsigned int)(team[0] < team[1]))
			{
				if (count != NULL)
					*count = 2;
				return 1;
			}
			if (count != NULL)
				*count = 2;
			return -1;
		}
	}
	if (count != NULL)
		*count = 4;
	return 0;
}

/* 0x00267b70  the vtable of CCarryOn_Player (slots as src/charlib/charlib_ai.h names
them) */
const void *const vtable_00267b70[18] = {
	(const void *)CCarryOn_Player_UpdateBurden,     /* [0] +0x00 before the AI update */
	(const void *)CCarryOn_Player_UpdatePost,       /* [1] +0x04 after the AI update */
	(const void *)FUN_001d5950,                     /* [2] +0x08 paused (empty) */
	(const void *)AI_Think,                         /* [3] +0x0c */
	(const void *)CCarryOn_Player_TryPunch,         /* [4] +0x10 */
	(const void *)FUN_001d5950,                     /* [5] +0x14 act (empty) */
	(const void *)CCarryOn_Player_SetMode,          /* [6] +0x18 */
	(const void *)CCarryOn_Player_RunMode,          /* [7] +0x1c */
	(const void *)AI_RespawnAt,                     /* [8] +0x20 */
	(const void *)CCarryOn_Player_RespawnFarthest,  /* [9] +0x24 */
	(const void *)AI_Spawn,                         /* [10] +0x28 */
	(const void *)FUN_001d5950,                     /* [11] +0x2c (empty) */
	(const void *)AI_CanMove,                       /* [12] +0x30 */
	(const void *)FUN_000a6fa0,                     /* [13] +0x34 */
	(const void *)FUN_001ec260,                     /* [14] +0x38 timer (empty) */
	(const void *)FUN_001ec260,                     /* [15] +0x3c (empty) */
	(const void *)AI_HoldEnd,                       /* [16] +0x40 */
	(const void *)CCarryOn_Player_SetAltModel,      /* [17] +0x44 */
};

/* 0x000c1ea0  CarryOn/CCarryOn_Player.cpp
Creates player `player` at `pos`: the CharLib AI player, the actor of its character, an
alternative model instance, the AI tables of carryon_b (0x00294998, 0x00294a84,
0x00294ac0) and the Fireworks state; in variant 2 the jar's looping sound. */
struct CCarryOnPlayer *CCarryOn_Player_Ctor(struct CCarryOnPlayer *self, int player, float *pos)
{
	struct CoCharModel *model = &DAT_00293a68[DAT_002baed4[player][0]];
	void *actor;
	void *copy;
	void *inst;
	int i;

	AI_Ctor(self);
	*(const void *const **)self = vtable_00267b70;
	actor = Actor_Create(model->actor, pos, NULL, 1, 0, 1);
	CO_PTR(self, CP_ACTOR) = actor;
	copy = bkHeapAlloc(0x170, CO_FILE_PLAYER, 0x130, 0x2001);
	self->instance[1] = copy;
	memcpy(copy, CO_ADDR(actor, 0x10), 0x5c * 4);
	inst = baInitActorInstance(NULL, model->anim, 6, CO_FILE_PLAYER, 0x132);
	self->instance[0] = inst;
	baSetInstanceAnimations(model->actor, inst);
	if (DAT_00283ea4 != 0)
		Core_SetCharacterColours(CO_ADDR(CP_ACTOR_OF(self), 0x10), DAT_00283ea4);
	self->altModel = 0;
	CO_I32(CP_ACTOR_OF(self), 0x220) = 0x59;
	CO_U16(CP_ACTOR_OF(self), CA_PLAYER) = (unsigned short)player;
	CO_I16(self, CP_ARRIVED) = 1;
	CO_I16(self, 0x34) = 0xf;
	CO_I16(self, CP_STATE) = 0;
	CO_I32(self, CP_MODE) = 0;
	CO_I32(self, 0x08) = 0;
	CO_PTR(self, 0x1094) = DAT_00294ac0;                /* AI state functions */
	CO_PTR(self, 0x1098) = DAT_00294a84;                /* AI conditions */
	CO_PTR(self, 0x109c) = DAT_00294998;                /* AI priorities */
	Actor_InitCharData(CP_ACTOR_OF(self));
	CO_PTR(CP_ACTOR_OF(self), 0x324) = DAT_00292ca0;    /* the animation names */
	for (i = 0; i < 0x2f; i++)
		CO_PTR(CP_ACTOR_OF(self), 0x264 + i * 4) = baFindSubObject(
			CO_ADDR(CP_ACTOR_OF(self), 0x10), (char *)CO_PTR(CP_ACTOR_OF(self), 0x324) + i * 0x44);
	self->jarNode = baFindNode(CO_PTR(CP_ACTOR_OF(self), 0x144), "guts");
	self->jarObject = baFindSubObject(CO_ADDR(CP_ACTOR_OF(self), 0x10), "glitter");
	CCarryOn_Player_SetThinkInterval(self, 1.0f);
	self->collected = 0;
	self->taken = 0;
	self->glitter = 0;
	self->glitterTimer = 0.0f;
	self->timer113c = 0.0f;
	self->fireworks = CO_I32(g_fireRes, 0x250) == 3 ? 2 : 0;
	self->score = 0;
	self->scoreText = 0;
	self->station = NULL;
	self->hasTarget = 0;
	self->goal = NULL;
	self->int1144 = 0;
	self->attacker = -1;
	self->chase = -1;
	self->wantLaunch = 0;
	self->pickup = NULL;
	self->glitterFull = 0;
	CO_I32(self, 0x1124) = 0;
	if (CO_I32(g_fireRes, 0x250) == 1)
	{
		self->sample = CFire_Resource_FindSample(g_fireRes, 8, 1, 2);
		self->volume = 0;
		self->lastGlitter = 0;
		self->glitterHeld = 0.0f;
		CFire_Resource_ReserveChannel(g_fireRes, self->sample);
		bsSetChannelVolume(self->sample, 0);
	}
	DAT_00293f48 = -1;
	DAT_00293f4c = -1;
	DAT_002f6d40 = 0.0f;
	return self;
}

/* 0x000c2180  CarryOn/CCarryOn_Player.cpp (file inferred)
The destructor: back to the normal model, frees the alternative instance and the copy. */
void CCarryOn_Player_Dtor(struct CCarryOnPlayer *self)
{
	*(const void *const **)self = vtable_00267b70;
	CCarryOn_Player_SetAltModel(self, 0);
	baDeleteActorInstance(self->instance[0]);
	bkHeapFree(self->instance[1]);
	self->instance[1] = NULL;
	AI_Dtor(self);
}

/* 0x000c2200  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 1: chooses the next mode: launch the fireworks (1) when carrying enough and the
rack has room, collect (9), chase a richer player (2) or attack someone (7). */
void CCarryOn_Player_DecideLaunch(struct CCarryOnPlayer *self)
{
	int other;
	int i;

	self->hasTarget = 0;
	self->goal = NULL;
	if (self->fireworks > 3)
		self->wantLaunch = 1;
	if (self->fireworks > 2 && (co_rand() & 0xff) > 200)
		self->wantLaunch = 1;
	if (self->fireworks == 0)
		self->wantLaunch = 0;
	if (self->wantLaunch != 0)
	{
		for (i = 0; i < DAT_00294390; i++)
		{
			if (CO_I32(g_fireGame.launcher, 0xc + i * 4) == 0)
			{
				self->chase = -1;
				goto launch;
			}
		}
	}
	if (self->chase == -1)
	{
		if (self->fireworks < 4 && co_Spawner_HasLoad(g_fireGame.spawner, CP_PLAYER(self)) != 0)
		{
			self->goal = NULL;
			CO_I32(self, CP_MODE) = 9;
		}
		else if (self->fireworks < 2)
		{
			if (CO_I32(self, 0x14) != 0 && (other = CCarryOn_Player_FindRicher(self)) != -1)
			{
				self->chaseFireworks = self->fireworks;
				self->chase = other;
				self->routeAge = 100;
				CO_I32(self, CP_MODE) = 2;
			}
			else
			{
				CCarryOn_Player_SetThinkInterval(self, 0.0f);
				CO_I32(self, 0x18) = -1;
				CO_I32(self, CP_MODE) = 7;
			}
		}
		else
		{
launch:
			CCarryOn_Player_SetThinkInterval(self, 0.0f);
			self->hasTarget = 0;
			CO_I32(self, CP_MODE) = 1;
			self->wantLaunch = 0;
		}
	}
	else
	{
		CCarryOn_Player_SetThinkInterval(self, 0.0f);
		self->chaseFireworks = self->fireworks;
		CO_I32(self, CP_MODE) = 2;
		self->routeAge = 100;
	}
	if (CO_I32(self, CP_MODE) != 2)
		self->chase = -1;
}

/* 0x000c2390  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 1, mode 1: walks to the launch rack (its nearest point; 20 further when no
route) and loads the fireworks there while it has a free slot. Returns nonzero when
done. */
int CCarryOn_Player_GoLaunch(struct CCarryOnPlayer *self)
{
	float target[4];
	int i;

	self->goal = NULL;
	if (self->fireworks == 0)
		return 1;
	for (i = 0; i < DAT_00294390; i++)
	{
		if (CO_I32(g_fireGame.launcher, 0xc + i * 4) != 0)
			continue;
		if (self->hasTarget == 0)
		{
			co_Rack_NearestPoint(g_fireGame.launcher, target, CP_POS(self));
			self->hasTarget = 1;
			self->target[0] = target[0];
			self->target[3] = target[3];
			self->target[2] = target[2];
			self->target[1] = target[1];
			self->hasRoute = 0;
			CCarryOn_Player_PlanRoute(CO_ADDR(self, CP_ROUTE), CP_POS(self), target,
				CO_F32(CP_ACTOR_OF(self), CA_POS + 4), &DAT_002940a8[0]);
			if (CO_I16(self, CP_ROUTE_NODE) == -1)
			{
				target[2] = target[2] + 20.0f;              /* 0x00255530 */
				self->target[3] = target[3];
				self->target[2] = target[2];
				self->target[0] = target[0];
				self->target[1] = target[1];
				self->hasRoute = 0;
				return 0;
			}
			self->hasRoute = 1;
		}
		else if (CO_I16(self, CP_ARRIVED) != 0)
		{
			self->hasTarget = 0;
			CCarryOn_Player_LoseFireworks(self, 4, -1);
			return 1;
		}
		return 0;
	}
	return 1;
}

/* 0x000c24f0  CarryOn/CCarryOn_Player.cpp (file inferred)
Variant 2: chooses the next mode: empty the jar (1) when it is full enough, collect (9)
near the jar, chase a richer player (2) or attack someone (7). The amount that is
"enough" falls with the difficulty (50, 20, 10). */
void CCarryOn_Player_DecideGlitter(struct CCarryOnPlayer *self)
{
	int enough;
	int full;
	int other;

	if (DAT_002bae70 == 0)
		enough = 0x32;
	else if (DAT_002bae70 == 1)
		enough = 0x14;
	else if (DAT_002bae70 == 2)
		enough = 10;
	else
		enough = (int)self;                             /* (never: the register held `this`) */
	full = self->glitterFull;
	self->glitterFull = 0;
	if (self->wantLaunch != 0)
	{
		CCarryOn_Player_SetThinkInterval(self, 0.2f);
		self->hasTarget = 0;
		CO_I32(self, CP_MODE) = 1;
		self->wantLaunch = 0;
	}
	else if (self->glitter >= 0x4b)
	{
		CCarryOn_Player_SetThinkInterval(self, 0.2f);
		self->hasTarget = 0;
		CO_I32(self, CP_MODE) = 1;
	}
	else if (co_Glitter_AtJar(g_fireGame.glitter, CP_POS(self)) != 0 && full == 0)
	{
		self->hasTarget = 0;
		CO_I32(self, CP_MODE) = 9;
	}
	else if (self->glitter >= enough)
	{
		CCarryOn_Player_SetThinkInterval(self, 0.3f);
		self->hasTarget = 0;
		CO_I32(self, CP_MODE) = 1;
	}
	else if ((self->chase == -1 && CO_I32(self, 0x14) == 0)
			|| (other = CCarryOn_Player_FindRicherGlitter(self)) == -1)
	{
		CCarryOn_Player_SetThinkInterval(self, 0.0f);
		CO_I32(self, 0x18) = -1;
		CO_I32(self, CP_MODE) = 7;
	}
	else
	{
		if (self->chase != -1)
			CCarryOn_Player_SetThinkInterval(self, 0.0f);
		self->chase = other;
		self->chaseGlitter = self->glitter;
		self->chaseTargetGlitter = g_coPlayers[other]->glitter;
		self->routeAge = 100;
		CO_I32(self, CP_MODE) = 2;
	}
	if (CO_I32(self, CP_MODE) != 2)
		self->chase = -1;
}

/* 0x000c2660  CarryOn/CCarryOn_Player.cpp (file inferred)
Punches `other`: the punch sound, a push away from this player (twice as strong while
holding something), a rare taunt from the victim, then the hit (instant knock-out with
the "Instant KO" tweaks of variants 1 and 3). Counts the hit twice in the victim's
CP_HITS. (The original returns the address of that counter; nobody uses it.) */
void CCarryOn_Player_Punch(struct CCarryOnPlayer *self, struct CCarryOnPlayer *other)
{
	float push[4];
	float strength;
	float damage;
	void *actor;

	CFire_Resource_Rumble(g_fireRes, CP_PLAYER(self), 2);
	push[0] = CP_POS(other)[0] - CP_POS(self)[0];
	push[1] = CP_POS(other)[1] - CP_POS(self)[1];
	push[2] = CP_POS(other)[2] - CP_POS(self)[2];
	if (CO_F32(self, 0x10a4) == 0.0f)
		strength = CP_CHAR_F32(CP_PLAYER(self), 0x14);
	else
		strength = CP_CHAR_F32(CP_PLAYER(self), 0x14) + CP_CHAR_F32(CP_PLAYER(self), 0x14);
	bmVectorSetLength(push, push, strength);
	actor = CP_ACTOR_OF(other);
	CO_F32(actor, 0x194) = push[0] + CO_F32(actor, 0x194);
	CO_F32(actor, 0x198) = push[1] + CO_F32(actor, 0x198);
	CO_F32(actor, 0x19c) = push[2] + CO_F32(actor, 0x19c);
	CO_I16(self, CP_TARGET_PLAYER) = (short)CP_PLAYER(other);
	if (co_rand() % 100 < 2)
		AI_PlayVoice(other, 0xc, DAT_00293928, 180.0f, 0, -1, 0);
	CO_I16(other, CP_HIT_BY) = (short)CP_PLAYER(self);
	other->attacker = CP_PLAYER(self);
	CO_I16(other, CP_HITS + CP_PLAYER(self) * 2) += 1;
	if ((CO_I32(g_fireRes, 0x250) == 2 && DAT_00293f2c != 0)
			|| (CO_I32(g_fireRes, 0x250) == 0 && DAT_002f6d38 != 0))
		damage = 10000.0f;
	else
		damage = CP_CHAR_F32(CP_PLAYER(self), 0x1c);
	CCarryOn_Player_Hit(other, CP_POS(self), damage, 0);
	CO_I16(other, CP_HITS + CP_PLAYER(self) * 2) += 1;
}

/* 0x000c2880  CarryOn/CCarryOn_Player.cpp (vtable slot 4)
Punches the first opponent in reach (in front, about the same height, not knocked
out), with a punch sound; one that is already being thrown (+0x10a8) is not hit.
Returns 1 when it punched. */
int CCarryOn_Player_TryPunch(struct CCarryOnPlayer *self)
{
	float angle;
	float dist;
	float reach;
	int i;

	CO_I16(self, 0x1092) = 1;
	for (i = 0; i < DAT_002bae94; i++)
	{
		if (i == (int)CP_PLAYER(self))
			continue;
		if (DAT_002baebc != 0 && DAT_002baed4[i][8] == DAT_002baed4[CP_PLAYER(self)][8])
			continue;
		if (!(fabsf(CP_POS(self)[1] - CP_OTHER_POS(i)[1]) < CO_F32(self, 0x1100)))
			continue;
		angle = CLMath_AngleDistance(CLMath_HeadingTo(CP_POS(self), CP_OTHER_POS(i)),
			CO_F32(CP_ACTOR_OF(self), CA_YAW));
		if (!(angle < CO_F32(self, 0x10fc)))
			continue;
		dist = bmVectorDistance(CP_POS(self), CP_OTHER_POS(i));
		reach = CP_CHAR_F32(CP_PLAYER(self), 0x10);
		if (CO_F32(self, 0x10a4) != 0.0f)
			reach = reach * DAT_00293a38;
		if ((angle + 1.0f) * dist < reach * 0.5f && CO_I16(g_coPlayers[i], CP_STATE) != 9)
		{
			CLSamples_PlayGeneric((co_rand() & 3) + 0xb, 0x7f, 0, 0, NULL);
			if (CO_F32(g_coPlayers[i], 0x10a8) == 0.0f)
			{
				CCarryOn_Player_Punch(self, g_coPlayers[i]);
				return 1;
			}
		}
	}
	return 0;
}

/* 0x000c2a60  CarryOn/CCarryOn_Player.cpp (vtable slot 9)
Respawns at the spawn point farthest from the others, empty-handed. */
void CCarryOn_Player_RespawnFarthest(struct CCarryOnPlayer *self)
{
	self->int1144 = 0;
	self->fireworks = 0;
	self->glitter = 0;
	AI_RespawnFarthest(self);
}

/* 0x000c2a80  CarryOn/CCarryOn_Player.cpp (file inferred)
The player loses what it carries (`how` as CCarryOn_Player_LoseFireworks); the last
player who hit it gets it: variant 1 one firework (`how` 0) or all but one, variant 2
half the glitter (up to 100), variant 3 the part. Plays a gloat or moan sometimes.
Returns the attacker, or NULL. */
struct CCarryOnPlayer *CCarryOn_Player_GiveToAttacker(struct CCarryOnPlayer *self, int how)
{
	struct CCarryOnPlayer *attacker;
	int fireworks = self->fireworks;
	int amount;
	int count;
	int type;
	int n;
	int r;
	int character;
	void *actor;
	int kind;

	if (fireworks == 0 && self->glitter == 0)
		return NULL;
	if (self->attacker < 0 || self->attacker > 3)
	{
		CCarryOn_Player_LoseFireworks(self, how, -1);
		return NULL;
	}
	attacker = g_coPlayers[self->attacker];
	switch (CO_I32(g_fireRes, 0x250))
	{
	case 0:
		if (how == 0)
		{
			if (attacker->fireworks < 5)
			{
				attacker->taken = attacker->taken + 1;
				if (CO_I32(g_fireRes, 0x250) == 2 && attacker->station[1] == 2
						&& attacker->station[2] == 5 && attacker->station[3] == 5)
					AI_PlayVoice(attacker, 0xb, DAT_00293928, 180.0f, 0, -1, 0);
				n = 0;
				do
				{
					if (attacker->fireworks > 4)
						break;
					attacker->partTypes[attacker->fireworks] = 5;
					n--;
					attacker->fireworks = attacker->fireworks + 1;
				} while (n >= 0);
			}
			count = 1;
			how = 0;
		}
		else
		{
			if (attacker->fireworks < 5)
			{
				attacker->taken = attacker->taken + fireworks - 1;
				if (CO_I32(g_fireRes, 0x250) == 2 && attacker->station[1] == 2
						&& attacker->station[2] == 5 && attacker->station[3] == 5)
					AI_PlayVoice(attacker, 0xb, DAT_00293928, 180.0f, 0, -1, 0);
				for (n = fireworks - 2; n >= 0 && attacker->fireworks < 5; n--)
				{
					attacker->partTypes[attacker->fireworks] = 5;
					attacker->fireworks = attacker->fireworks + 1;
				}
			}
			count = -1;
		}
		CCarryOn_Player_LoseFireworks(self, how, count);
		r = Core_Rand();
		if (r % 100 < 4)
		{
			CLSamples_PlayFromSet((char)DAT_002baed4[CP_PLAYER(self)][0], 7, CP_POS(self),
				DAT_00293928, 0, -1, 0);
			return attacker;
		}
		if (r % 100 > 1)
			return attacker;
		CLSamples_PlayFromSet((char)DAT_002baed4[self->attacker][0], 6, CP_POS(attacker),
			DAT_00293928, 0, -1, 0);
		return attacker;
	case 1:
		amount = self->glitter / 2;
		if (amount + attacker->glitter > 100)
			amount = 100 - attacker->glitter;
		attacker->glitter = attacker->glitter + amount;
		self->glitter = self->glitter - amount;
		r = Core_Rand() % 100;
		if (r > 0x13)
			goto attackerSays;
		goto victimSays;
	case 2:
		type = self->partTypes[0];
		if (attacker->fireworks < 1)
		{
			attacker->taken = attacker->taken + 1;
			if (CO_I32(g_fireRes, 0x250) == 2 && attacker->station[1] == 2
					&& attacker->station[2] == type && attacker->station[3] == type)
				AI_PlayVoice(attacker, 0xb, DAT_00293928, 180.0f, 0, -1, 0);
			n = 0;
			do
			{
				if (attacker->fireworks > 0)
					break;
				attacker->partTypes[attacker->fireworks] = type;
				n--;
				attacker->fireworks = attacker->fireworks + 1;
			} while (n >= 0);
		}
		r = Core_Rand() % 100;
		if (r < 0x14)
			goto victimSays;
attackerSays:
		if (r > 0x27)
			goto done;
		actor = CP_ACTOR_OF(attacker);
		character = DAT_002baed4[self->attacker][0];
		kind = 6;
		goto say;
victimSays:
		actor = CP_ACTOR_OF(self);
		character = DAT_002baed4[CP_PLAYER(self)][0];
		kind = 7;
say:
		CLSamples_PlayFromSet((char)character, kind, CO_FP(actor, CA_POS), DAT_00293928, 0, -1, 0);
done:
		CCarryOn_Player_LoseFireworks(self, how, -1);
		return attacker;
	default:
		CCarryOn_Player_LoseFireworks(self, how, -1);
		return NULL;
	}
}

/* 0x000c2e00  CarryOn/CCarryOn_Player.cpp (file inferred)
An opponent to attack (awake ones only when `awake`): one carrying fireworks, else one
with points, else one by the player records' field 4, else a random one. (The first two
passes compare the opponent's count with the record field 4 of the last one chosen, as
the original does.) */
int CCarryOn_Player_PickTarget(struct CCarryOnPlayer *self, int awake)
{
	unsigned int me;
	int best;
	int found = -1;
	int pass;
	int value;
	int i;

	for (pass = 0; pass < 3; pass++)
	{
		best = 0;
		for (i = 0; i < DAT_002bae94; i++)
		{
			me = CP_PLAYER(self);
			if (i == (int)me)
				continue;
			if (awake != 0 && (CO_I16(g_coPlayers[i], CP_STATE) == 7
					|| CO_I16(g_coPlayers[i], CP_STATE) == 9))
				continue;
			if (DAT_002baebc != 0 && DAT_002baed4[i][8] == DAT_002baed4[me][8])
				continue;
			if (pass == 0)
				value = g_coPlayers[i]->fireworks;
			else if (pass == 1)
				value = g_coPlayers[i]->score;
			else
				value = DAT_002baed4[i][4];
			if (best < value)
			{
				best = DAT_002baed4[i][4];
				found = i;
			}
		}
		if (found != -1)
			return found;
	}
	return CCarryOn_Player_PickRandomOpponent(self, awake);
}

/* 0x000c2fb0  CarryOn/CCarryOn_Player.cpp (vtable slot 6)
Chooses a new AI mode: aggressive or not by difficulty; at the end of the game idle;
the bully's chosen victim hits back at the bully; others may hit back at whoever hit
them (by the character's grudge chance) or decide by variant. */
void CCarryOn_Player_SetMode(struct CCarryOnPlayer *self)
{
	int calm;
	int variant;

	CCarryOn_Player_SetThinkInterval(self, 1.0f);
	CO_I32(self, 0x14) = co_rand() % 100 < DAT_002bae70 * 0x28;
	if (g_fireGame.state == CO_STATE_END)
	{
		CO_I32(self, CP_MODE) = 0;
		return;
	}
	CO_F32(self, CP_THINK_TIME) = 0.0f;
	CO_I32(self, 0x24) = -1;
	CO_I32(self, 0x28) = 0;
	CO_I16(self, CP_ARRIVED) = 1;
	variant = CO_I32(g_fireRes, 0x250);
	calm = 1;
	if (CO_I32(self, 0x14) != 0)
	{
		if (variant == 2 && self->fireworks != 0)
			calm = 0;
		if (variant == 0 && self->fireworks > 1)
			calm = 0;
		if (variant == 1 && self->glitter > 0x31)
			calm = 0;
	}
	if (CP_PLAYER(self) == (unsigned int)DAT_00293f48 && DAT_00293f4c != -1)
	{
		CO_I16(self, CP_HIT_BY) = (short)DAT_00293f4c;
		self->attacker = DAT_00293f4c;
	}
	else if (DAT_002bae70 != 0 || !calm || CO_I16(self, CP_HIT_BY) == -1
			|| CP_CHAR_I32(CP_PLAYER(self), 0x28) <= Core_Rand() % 100)
	{
		switch (variant)
		{
		case 0:
			CCarryOn_Player_DecideLaunch(self);
			return;
		case 1:
			CCarryOn_Player_DecideGlitter(self);
			return;
		case 2:
			CCarryOn_Player_DecideParts(self);
			return;
		case 3:
			CO_I32(self, CP_MODE) = 1;
			return;
		}
		return;
	}
	CO_I32(self, CP_MODE) = 4;
	CCarryOn_Player_SetThinkInterval(self, 0.0f);
}

/* replans the route to player `other` when it is old (more than 20 frames) or arrived;
part of CCarryOn_Player_RunMode (inlined in the original) */
static void co_RouteToPlayer(struct CCarryOnPlayer *self, int other, int force)
{
	float step[4];

	if (force || self->routeAge > 0x14 || CO_I16(self, CP_ARRIVED) != 0)
	{
		Core_Vec3Sub(step, CP_POS(self), CP_OTHER_POS(other));
		bmVectorSetLength(step, step, 1.0f);
		CCarryOn_Player_PlanRoute(CO_ADDR(self, CP_ROUTE), CP_POS(self), CP_OTHER_POS(other),
			CO_F32(CP_ACTOR_OF(self), CA_POS + 4), step);
		self->routeAge = 0;
	}
}

/* 0x000c3150  CarryOn/CCarryOn_Player.cpp (vtable slot 7)
Runs the current AI mode (see the top of this file). Returns nonzero when the mode is
over (AI_Think then calls SetMode). */
int CCarryOn_Player_RunMode(struct CCarryOnPlayer *self)
{
	float target[4];
	float dir[4];
	void *item;
	float scale;
	int other;
	int fresh;
	short hitBy;

	if ((g_fireGame.state != CO_STATE_PLAY && CO_I32(self, CP_MODE) != 0)
			|| (CO_I32(self, CP_MODE) != 4 && CO_I16(self, CP_STATE) == 7
				&& CO_I32(self, CP_STATE_FRAMES) == 1))
		return 1;
	switch (CO_I32(self, CP_MODE))
	{
	case 0:
		return 1;
	case 1:
		switch (CO_I32(g_fireRes, 0x250))
		{
		case 0:
			return CCarryOn_Player_GoLaunch(self);
		case 1:
			return CCarryOn_Player_EmptyGlitter(self);
		case 2:
			return CCarryOn_Player_DeliverParts(self);
		case 3:
			return CCarryOn_Player_WorkConveyor(self);
		}
		break;

	case 2:                                                 /* chase */
		other = self->chase;
		if (other == -1)
			return 1;
		if (CO_F32(self, CP_THINK_TIME) >= 400.0f)          /* 0x00255914 */
			goto stopChase;
		switch (CO_I32(g_fireRes, 0x250))
		{
		case 0:
			if (self->fireworks != self->chaseFireworks || g_coPlayers[other]->fireworks == 0)
				goto stopChase;
			if (CO_F32(self, CP_THINK_TIME) >= 20.0f
					&& (item = co_Spawner_FindPad(g_fireGame.spawner, CP_POS(self), CP_PLAYER(self))) != NULL)
			{
				target[0] = CO_F32(item, 0x8);
				target[1] = CO_F32(item, 0xc);
				target[2] = CO_F32(item, 0x10);
				target[3] = CO_F32(item, 0x14);
				if (bmVectorDistance(CP_POS(self), CP_OTHER_POS(self->chase))
						< bmVectorDistance(CP_POS(self), target))
				{
					self->chase = -1;
					return 1;
				}
			}
			break;
		case 1:
			if (self->chaseGlitter + 10 < self->glitter
					|| g_coPlayers[other]->glitter < self->chaseTargetGlitter)
				goto stopChase;
			break;
		case 2:
			if (self->fireworks > 0 || g_coPlayers[other]->fireworks == 0)
				goto stopChase;
			if (CO_F32(self, CP_THINK_TIME) >= 20.0f)
			{
				item = co_Spawner_FindType(g_fireGame.spawner, self->wantType[0]);
				if (item != NULL)
				{
					scale = 0.75f;
					if (g_coPlayers[self->chase]->partTypes[0] != self->wantType[0])
						scale = 1.0f;
					target[0] = CO_F32(item, 0x8);
					target[1] = CO_F32(item, 0xc);
					target[2] = CO_F32(item, 0x10);
					target[3] = CO_F32(item, 0x14);
					if (bmVectorDistance(CP_POS(self), CP_OTHER_POS(self->chase))
							< bmVectorDistance(CP_POS(self), target) * scale)
						goto stopChase;
				}
				if (g_coPlayers[self->chase]->partTypes[0] != self->wantType[0]
						&& (item = co_Spawner_FindType(g_fireGame.spawner, self->wantType[1])) != NULL)
				{
					target[0] = CO_F32(item, 0x8);
					target[1] = CO_F32(item, 0xc);
					target[2] = CO_F32(item, 0x10);
					target[3] = CO_F32(item, 0x14);
					if (bmVectorDistance(CP_POS(self), CP_OTHER_POS(self->chase))
							< bmVectorDistance(CP_POS(self), target))
						goto stopChase;
				}
			}
			break;
		case 3:
			return 1;
		}
		if (Core_Rand() % 100 < 0x28 && CCarryOn_Player_CanHit(self, self->chase) != 0)
			CO_I32(CO_PTR(self, 0x6c), 0x14) = 1;               /* action: punch */
		co_RouteToPlayer(self, self->chase, 0);
		self->routeAge = self->routeAge + 1;
		if (CO_I16(self, CP_ROUTE_NODE) == -1)
		{
			self->chase = -1;
			self->hasRoute = 0;
			return 1;
		}
		goto routed;
stopChase:
		self->chase = -1;
		return 1;

	case 4:                                                 /* hit back */
		hitBy = CO_I16(self, CP_HIT_BY);
		if (hitBy == -1)
			return 1;
		if (CO_I16(self, CP_TARGET_PLAYER) == hitBy)
		{
			CO_I16(self, CP_HIT_BY) = -1;
			CO_I16(self, CP_TARGET_PLAYER) = -1;
			return 1;
		}
		if (CO_F32(self, CP_THINK_TIME) >= 400.0f)
		{
			CO_I16(self, CP_HIT_BY) = -1;
			return 1;
		}
		if (CO_I16(g_coPlayers[hitBy], CP_STATE) == 7 || CO_I16(g_coPlayers[hitBy], CP_STATE) == 9)
			return 1;
		if (CO_I32(g_fireRes, 0x250) == 2 && self->fireworks > 0)
			return 1;
		if (Core_Rand() % 100 < 0x28 && CCarryOn_Player_CanHit(self, CO_I16(self, CP_HIT_BY)) != 0)
			CO_I32(CO_PTR(self, 0x6c), 0x14) = 1;
		co_RouteToPlayer(self, CO_I16(self, CP_HIT_BY), 0);
		goto counted;

	case 5:                                                 /* wait */
		if (CO_I32(self, 0x10) == 0)
			return 1;
		break;

	case 6:                                                 /* go to AI_TARGET */
		Core_Vec3Sub(dir, CP_POS(self), CO_PTR(self, 0x1c));
		AI_SetStick(self, dir, 1.0f);
		break;

	case 7:                                                 /* attack */
		fresh = 0;
		if ((unsigned char)Core_Rand() > 0xfa)
			return 1;
		if (CO_I32(g_fireRes, 0x250) == 2 && self->fireworks > 0)
			return 1;
		other = CO_I32(self, 0x18);
		if (other != -1)
		{
			if (CO_I16(g_coPlayers[other], CP_STATE) == 7)
				return 1;
			if (Core_Rand() % 100 < 0x28 && CCarryOn_Player_CanHit(self, CO_I32(self, 0x18)) != 0)
				CO_I32(CO_PTR(self, 0x6c), 0x14) = 1;
		}
		if (CO_I32(self, 0x18) == -1 || CO_I16(g_coPlayers[CO_I32(self, 0x18)], CP_STATE) == 9)
		{
			fresh = 1;
			CO_I16(self, CP_TARGET_PLAYER) = -1;
			if (Core_Rand() % 100 < 0x32)
				CO_I32(self, 0x18) = CCarryOn_Player_PickTarget(self, 1);
			else
				CO_I32(self, 0x18) = CCarryOn_Player_PickWorstEnemy(self, 1);
		}
		if (CO_I16(self, CP_TARGET_PLAYER) == CO_I32(self, 0x18))
			goto lost;
		co_RouteToPlayer(self, CO_I32(self, 0x18), fresh);
counted:
		self->routeAge = self->routeAge + 1;
		if (CO_I16(self, CP_ROUTE_NODE) == -1)
		{
			self->hasRoute = 0;
lost:
			CO_I32(self, 0x18) = -1;
			CO_I16(self, CP_TARGET_PLAYER) = -1;
			return 1;
		}
routed:
		self->hasRoute = 1;
		return 0;

	case 9:                                                 /* collect */
		switch (CO_I32(g_fireRes, 0x250))
		{
		case 0:
			return CCarryOn_Player_CollectPads(self);
		case 1:
			return CCarryOn_Player_CollectGlitter(self);
		case 2:
			return CCarryOn_Player_CollectParts(self);
		case 3:
			return CCarryOn_Player_WorkConveyor(self);
		}
		break;
	}
	return 0;
}
