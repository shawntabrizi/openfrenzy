/*
BumperCars/unplaced_ai.c  (module bumpercars_a)

The computer driver of a BumperCars car (gap functions 0x00089020-0x0008bdd0; the source file
is not known, the class has the vtable 0x002640e0). One lives in each player slot of the game
(slot + 0x10); its player is the slot, and the slot's car (an entity of bumpercars_b's world
list) is at slot + 0x138.

Every frame (CBCarAI_Update) it looks around (holes, walls, flames, the goal), then runs its
state: find a target, chase or ram it, flee from it, wander, or play the puck. The skill
values come from the difficulty (core DAT_002bae70) and the variation.
*/

#include "bumpercars_a.h"

/* core: the difficulty (0 easy, 1 medium, 2 hard) */
extern int DAT_002bae70;

/* the AI object (0x128 bytes, at player slot + 0x10) */
struct CBCarAI {
	const void *const *vtable;   /* 0x000 0x002640e0 */
	int unknown04;               /* 0x004 */
	float aggression;            /* 0x008 */
	float reaction;              /* 0x00c */
	float patience;              /* 0x010 */
	float accuracy;              /* 0x014 */
	float last[4];               /* 0x018 the four values above when last applied */
	int state;                   /* 0x028 0 find, 1 chase, 2 wander, 3 to the goal, 4 flee */
	unsigned int behaviour;      /* 0x02c AI_* */
	int param;                   /* 0x030 */
	unsigned char *player;       /* 0x034 the player slot */
	unsigned char *target;       /* 0x038 an entity */
	int unknown3c;               /* 0x03c */
	unsigned char targetTimer[0x5c]; /* 0x040 a BCarTimer (0x5c bytes): its seconds at +0x58 (0x098) */
	int unknown9c;               /* 0x09c */
	unsigned char wanderTimer[0x5c]; /* 0x0a0 its seconds at 0x0f8 */
	unsigned char *leader;       /* 0x0fc a player slot */
	int hasPuck;                 /* 0x100 */
	unsigned char *keeper;       /* 0x104 the entity that carries something */
	int circle;                  /* 0x108 */
	float heading;               /* 0x10c */
	float distance;              /* 0x110 */
	float point[4];              /* 0x114 */
	float lookAhead;             /* 0x124 */
};
OF_ASSERT_SIZE(struct CBCarAI, 0x128);

#define AI_AVOID_HOLES 0x0001
#define AI_JITTER      0x0002
#define AI_LEADER      0x0004
#define AI_GOAL        0x0008
#define AI_RAM_NEAR    0x0010
#define AI_AVOID_GOAL  0x0020
#define AI_BLOCK       0x0040
#define AI_BRAKE       0x0080
#define AI_PUCK        0x0100
#define AI_WAYPOINTS   0x0200
#define AI_NO_TARGET   0x0400
#define AI_KEEPER      0x0800
#define AI_CIRCLE      0x1000
#define AI_NO_WALLS    0x2000

/* the timers' seconds (BCarTimer + 0x58) */
#define TARGET_SECONDS(ai) BCA_F32((ai)->targetTimer, 0x58)
#define WANDER_SECONDS(ai) BCA_F32((ai)->wanderTimer, 0x58)

/* the car of the AI's player, and an entity's fields */
#define AI_CAR(ai) (*(unsigned char **)((ai)->player + 0x138))
#define E_F(e, off) BCA_F32(e, off)
#define E_I(e, off) BCA_I32(e, off)
#define E_S(e, off) BCA_AT(e, off, short)
#define E_POS(e) ((float *)((e) + 0x50))
#define E_NEXT(e) BCA_AT(e, 0x8b8, unsigned char *)
#define ENT_FIRST() BCG(0x330c, unsigned char *)

/* bumpercars_b (assumed shapes) */
#define Timer_SetSeconds(t, s) BCA_FN(FUN_001c36e0, void, (void *, float))((t), (s))
#define Timer_Pause(t) BCA_FN(FUN_001c3770, void, (void *))(t)
#define Timer_Begin(t) BCA_FN(FUN_000964d0, void, (void *))(t)
#define Timer_Reset(t) BCA_FN(FUN_00096350, int, (void *))(t)
#define Timer_Tick(t) BCA_FN(FUN_000963a0, void, (void *))(t)
#define Timer_Remaining(t) BCA_FN(FUN_000965f0, float, (void *))(t)
#define Ent_SameSide(a, b) BCA_FN(FUN_000a5180, int, (void *, void *, int *, int *))((a), (b), NULL, NULL)
#define Heading(x, z) BCA_FN(FUN_000a5540, float, (float, float))((x), (z))
#define Car_Steer(car, h) BCA_FN(FUN_0009adc0, void, (void *, float))((car), (h))
#define Car_Throttle(car, s) BCA_FN(FUN_0009b4c0, void, (void *, float))((car), (s))
#define Player_Speed(p) BCA_FN(FUN_00096c30, float, (void *, int))((p), 0)
#define Player_Brake(p) BCA_FN(FUN_00097c10, void, (void *))(p)
#define Player_Boost(p) BCA_FN(FUN_00098030, void, (void *))(p)
#define Ray_Obstacle(from, delta, hit, normal) \
	BCA_FN(FUN_000a2950, float, (float *, float *, float *, float *))((from), (delta), (hit), (normal))
/* this module: the goal (game + 0x3310) and the flames (game + 0x3f28) */
#define GOAL (g_BCarGame->goal)
#define FLAMES (g_BCarGame->unknown3f28)

static float car_speed(unsigned char *car)
{
	return sqrtf(E_F(car, 0x98) * E_F(car, 0x98) + E_F(car, 0x94) * E_F(car, 0x94) + E_F(car, 0x90) * E_F(car, 0x90));
}

/* how many other computer players already aim at entity e (inlined in the original) */
static unsigned int others_targeting(struct CBCarAI *self, unsigned char *e)
{
	unsigned int n = 0;
	int i;

	for (i = 0; i < 4; i++) {
		unsigned char *slot = g_BCarGame->players[i];

		if (self->player != slot && BCA_I32(slot, 0) == 0 && BCA_AT(slot, 0x48, unsigned char *) == e)
			n++;
	}
	return n;
}

/* a point near pos, off by up to `spread` in x and z towards or away from accuracy (the
original draws four numbers and uses the 2nd and the 4th) */
static void aim_jitter(struct CBCarAI *self, float *pos, double spread)
{
	double r;

	bca_rand();
	r = (double)bca_rand();
	pos[0] = (float)((r * (double)self->accuracy * spread * (double)(1.0f / 32767.0f) -
		((double)1.0f - (double)self->accuracy) * r * spread * (double)(1.0f / 32767.0f)) + (double)pos[0]);
	bca_rand();
	r = (double)bca_rand();
	pos[2] = (float)((r * (double)self->accuracy * spread * (double)(1.0f / 32767.0f) -
		((double)1.0f - (double)self->accuracy) * r * spread * (double)(1.0f / 32767.0f)) + (double)pos[2]);
}

static void drive_at_speed(struct CBCarAI *self)
{
	Car_Throttle(AI_CAR(self), Player_Speed(self->player));
}

/* 0x00088f80  (gap): finds a debris type by name in a table of `count` types */
struct DebrisType *Debris_FindType(const char *name, struct DebrisType *types, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (strcmp(name, types[i].name) == 0) {
			BCA_Printf("found %s at[%i]\n", name, i);
			return &types[i];
		}
	}
	BCA_Printf("didn't find %s\n", name);
	return NULL;
}

/* 0x002640e0  CBCarAI vtable */
const void *const vtable_002640e0[1] = {
	(const void *)FUN_00089750, /* [0] scalar deleting destructor */
};

/* 0x00089020  (gap): CBCarAI::CBCarAI */
struct CBCarAI *CBCarAI_Ctor(struct CBCarAI *self)
{
	self->vtable = vtable_002640e0;
	BCA_FN(FUN_00096320, void, (void *))(self->targetTimer);
	BCA_FN(FUN_00096320, void, (void *))(self->wanderTimer);
	return self;
}

/* 0x00089070  (gap): CBCarAI::~CBCarAI */
void CBCarAI_Dtor(struct CBCarAI *self)
{
	self->vtable = vtable_002640e0;
	FUN_001d5950();
	FUN_001d5950();
}

/* 0x000890d0  (gap): the timers' destructors (empty) */
void CBCarAI_DestroyTimers(struct CBCarAI *self)
{
	(void)self;
	FUN_001d5950();
	FUN_001d5950();
}

/* 0x000890f0  (gap): forgets the target and wanders a while */
void CBCarAI_Reset(struct CBCarAI *self)
{
	Timer_SetSeconds(self->wanderTimer, WANDER_SECONDS(self));
	Timer_Begin(self->wanderTimer);
	self->target = NULL;
	self->leader = NULL;
	self->state = 0;
	self->hasPuck = 0;
	self->keeper = NULL;
}

/* 0x00089130  (gap): the nearest live entity (of `kind`, 0: any) that is not on our side
and that at most one other computer player aims at; NULL beyond 999999 */
unsigned char *CBCarAI_FindNearest(struct CBCarAI *self, int kind)
{
	unsigned char *e, *best = NULL, *car;
	float bestDist = 1e7f, d;

	if (ENT_FIRST() == NULL)
		return NULL;
	for (e = ENT_FIRST(); e != NULL; e = E_NEXT(e)) {
		car = AI_CAR(self);
		if (e != car && E_I(e, 0x20) != 0 && E_S(e, 0x8a8) == 0 && (kind == 0 || E_I(e, 0) == kind) &&
		    Ent_SameSide(e, car) == 0 && others_targeting(self, e) < 2) {
			d = bmVectorDistance(E_POS(AI_CAR(self)), E_POS(e));
			if (d < bestDist) {
				bestDist = d;
				best = e;
			}
		}
	}
	if (bestDist < 999999.0f)
		return best;
	return NULL;
}

/* 0x00089280  (gap): the nearest live entity that is flashing (+0x8a4) */
unsigned char *CBCarAI_FindNearestFlashing(struct CBCarAI *self)
{
	unsigned char *e, *best = NULL, *car;
	float bestDist = 1e7f, d;

	if (ENT_FIRST() == NULL)
		return NULL;
	for (e = ENT_FIRST(); e != NULL; e = E_NEXT(e)) {
		car = AI_CAR(self);
		if (e != car && E_I(e, 0x20) != 0 && E_S(e, 0x8a4) != 0 && E_S(e, 0x8a8) == 0 && Ent_SameSide(e, car) == 0 &&
		    others_targeting(self, e) < 2) {
			d = bmVectorDistance(E_POS(AI_CAR(self)), E_POS(e));
			if (d < bestDist) {
				bestDist = d;
				best = e;
			}
		}
	}
	if (bestDist < 0x1.312cecp+23f /* 9999990 */)
		return best;
	return NULL;
}

/* 0x000893c0  (gap): the furthest live entity in play */
unsigned char *CBCarAI_FindFurthest(struct CBCarAI *self)
{
	unsigned char *e, *best, *car;
	float bestDist = 0.0f, d;

	best = ENT_FIRST();
	if (best == AI_CAR(self))
		best = E_NEXT(best);
	if (ENT_FIRST() == NULL)
		return NULL;
	for (e = ENT_FIRST(); e != NULL; e = E_NEXT(e)) {
		car = AI_CAR(self);
		if (e != car && E_I(e, 0x20) != 0 && E_I(e, 0x894) != 0 && E_S(e, 0x8a8) == 0 && Ent_SameSide(e, car) == 0 &&
		    others_targeting(self, e) < 2) {
			d = bmVectorDistance(E_POS(AI_CAR(self)), E_POS(e));
			if (bestDist < d) {
				bestDist = d;
				best = e;
			}
		}
	}
	if (bestDist != 0.0f)
		return best;
	return NULL;
}

/* 0x00089510  (gap): the live entity in play nearest to the goal */
unsigned char *CBCarAI_FindNearestToGoal(struct CBCarAI *self)
{
	unsigned char *e, *best, *car;
	float bestDist = 32767.0f, d;

	best = ENT_FIRST();
	if (best == AI_CAR(self))
		best = E_NEXT(best);
	if (ENT_FIRST() == NULL)
		return NULL;
	for (e = ENT_FIRST(); e != NULL; e = E_NEXT(e)) {
		car = AI_CAR(self);
		if (e != car && E_I(e, 0x20) != 0 && E_I(e, 0x894) != 0 && E_S(e, 0x8a8) == 0 && Ent_SameSide(e, car) == 0 &&
		    others_targeting(self, e) < 2) {
			d = FUN_00091aa0(GOAL, E_POS(e));
			if (d >= 0.0f && d < bestDist) {
				bestDist = d;
				best = e;
			}
		}
	}
	if (bestDist < 32766.0f)
		return best;
	return NULL;
}

/* 0x00089670  (gap): the nearest live puck (kind 3) in play */
unsigned char *CBCarAI_FindNearestPuck(struct CBCarAI *self)
{
	unsigned char *e, *best = NULL, *car;
	float bestDist = 32767.0f, d;

	if (ENT_FIRST() == NULL)
		return NULL;
	for (e = ENT_FIRST(); e != NULL; e = E_NEXT(e)) {
		car = AI_CAR(self);
		if (e != car && E_I(e, 0) == 3 && E_I(e, 0x20) != 0 && E_I(e, 0x894) != 0 && E_S(e, 0x8a8) == 0) {
			d = bmVectorDistance(E_POS(car), E_POS(e));
			if (d < bestDist) {
				bestDist = d;
				best = e;
			}
		}
	}
	if (bestDist < 32766.0f)
		return best;
	return NULL;
}

/* 0x00089710  (gap): pauses the timers */
void CBCarAI_Pause(struct CBCarAI *self)
{
	Timer_Pause(self->targetTimer);
	Timer_Pause(self->wanderTimer);
}

/* 0x00089730  (gap): restarts the timers */
void CBCarAI_Resume(struct CBCarAI *self)
{
	Timer_Begin(self->targetTimer);
	Timer_Begin(self->wanderTimer);
}

/* 0x00089750  (gap): CBCarAI scalar deleting destructor */
struct CBCarAI *CBCarAI_Destroy(struct CBCarAI *self, byte flags)
{
	CBCarAI_Dtor(self);
	if ((flags & 1) != 0)
		BCA_DELETE(self);
	return self;
}

/* 0x00089770  (gap): sets the driver up for a player: the behaviour for the difficulty
(behaviours[3], one per difficulty) and the skill values. Returns 1. */
int CBCarAI_Init(struct CBCarAI *self, unsigned char *player, unsigned int *behaviours)
{
	unsigned char *car;
	float accuracy;
	int variation = g_BCarGame->variation;

	self->player = player;
	self->distance = 32767.0f;
	self->state = 0;
	self->target = NULL;
	self->leader = NULL;
	self->hasPuck = 0;
	car = BCA_AT(player, 0x138, unsigned char *);
	self->point[0] = E_F(car, 0x50);
	self->point[1] = E_F(car, 0x54);
	self->point[2] = E_F(car, 0x58);
	self->point[3] = E_F(car, 0x5c);
	if (DAT_002bae70 == 0) {
		self->behaviour = behaviours[0];
		accuracy = 0.1f;
		self->aggression = 0.1f;
		self->reaction = 0.1f;
		self->patience = variation == 3 ? 0.3f : 0.5f;
		self->accuracy = accuracy;
	} else if (DAT_002bae70 == 1) {
		self->behaviour = behaviours[1];
		accuracy = 0.4f;
		self->aggression = 0.4f;
		self->reaction = 0.4f;
		self->patience = variation == 3 ? 0.4f : 0.7f;
		self->accuracy = variation != 2 ? 0.5f : accuracy;
	} else if (DAT_002bae70 == 2) {
		self->behaviour = behaviours[2];
		self->aggression = 1.0f;
		self->reaction = 1.0f;
		self->patience = 1.0f;
		self->accuracy = variation == 2 ? 0.8f : 1.0f;
	}
	if (variation == 2)
		self->accuracy = self->accuracy * 0.5f;
	self->last[0] = self->aggression;
	self->last[3] = self->accuracy;
	TARGET_SECONDS(self) = self->reaction * 5.0f + 1.0f;
	self->last[1] = self->reaction;
	self->last[2] = self->patience;
	Timer_Reset(self->targetTimer);
	WANDER_SECONDS(self) = (1.0f - self->patience) * 0.5f + 0.2f;
	Timer_Reset(self->wanderTimer);
	Timer_SetSeconds(self->wanderTimer, WANDER_SECONDS(self));
	Timer_Begin(self->wanderTimer);
	return 1;
}

/* 0x00089910  (gap): a live entity in play that stands next to a hole: the point beyond it
(away from us, 1.5 look-aheads) has no ground above the kill height. NULL when none. */
unsigned char *CBCarAI_FindBlocker(struct CBCarAI *self)
{
	unsigned char *e, *car;
	float dir[4], scale;
	struct BRay ray;
	struct BCollHit hit;
	int count;

	for (e = ENT_FIRST(); e != NULL; e = E_NEXT(e)) {
		car = AI_CAR(self);
		if (e == car || E_I(e, 0x20) == 0 || E_I(e, 0x894) == 0 || E_S(e, 0x8a8) != 0 || Ent_SameSide(e, car) != 0)
			continue;
		if (others_targeting(self, e) >= 2)
			continue;
		dir[0] = E_POS(AI_CAR(self))[0] - E_POS(e)[0];
		dir[2] = E_POS(AI_CAR(self))[2] - E_POS(e)[2];
		dir[1] = 0.0f;
		FUN_000514a0(dir, dir);
		scale = self->lookAhead * 1.5f;
		dir[0] = dir[0] * scale;
		dir[1] = dir[1] * scale;
		dir[2] = dir[2] * scale;
		self->point[0] = dir[0] + E_POS(e)[0];
		self->point[1] = dir[1] + E_POS(e)[1];
		hit.flags = BCOLL_T;
		count = 1;
		self->point[2] = dir[2] + E_POS(e)[2];
		DAT_00285644 = 0.01f;
		ray.origin[1] = self->point[1];
		ray.length = E_POS(e)[1] - BCG(0x2588, float); /* KILLHEIGHT */
		ray.origin[0] = self->point[0];
		ray.origin[2] = self->point[2];
		ray.origin[3] = self->point[3];
		ray.dir[1] = -ray.length;
		ray.dir[2] = 0.0f;
		ray.dir[0] = 0.0f;
		ray.dir[3] = 0.0f;
		if (bmRayMesh(&ray, BCG(0x78, struct BCollMesh *), &count, &hit) == 0)
			return e;
	}
	return NULL;
}

/* 0x00089b30  (gap): drives at the target, aiming off by the accuracy */
void CBCarAI_Chase(struct CBCarAI *self)
{
	unsigned char *car;
	float aim[4];
	double spread;

	aim[0] = E_POS(self->target)[0];
	aim[1] = E_POS(self->target)[1];
	aim[2] = E_POS(self->target)[2];
	aim[3] = E_POS(self->target)[3];
	spread = (double)bmVectorDistance(aim, E_POS(AI_CAR(self))) * (double)0.07f;
	aim_jitter(self, aim, spread);
	car = AI_CAR(self);
	Car_Steer(AI_CAR(self), Heading(aim[0] - E_POS(car)[0], aim[2] - E_POS(car)[2]));
	drive_at_speed(self);
}

/* 0x00089ca0  (gap): circles the target (heading 45 degrees off it) */
void CBCarAI_Circle(struct CBCarAI *self)
{
	unsigned char *car;
	float aim[4], heading;
	double spread;

	aim[0] = E_POS(self->target)[0];
	aim[1] = E_POS(self->target)[1];
	aim[2] = E_POS(self->target)[2];
	aim[3] = E_POS(self->target)[3];
	spread = (double)bmVectorDistance(aim, E_POS(AI_CAR(self))) * (double)0.07f;
	aim_jitter(self, aim, spread);
	car = AI_CAR(self);
	heading = Heading(aim[0] - E_POS(car)[0], aim[2] - E_POS(car)[2]);
	if (CLMath_WrapAngle(heading) >= 0x1.921fb4p+1f)
		heading = heading - 0x1.921fb6p-1f;
	else
		heading = heading + 0x1.921fb6p-1f;
	Car_Steer(AI_CAR(self), heading);
	drive_at_speed(self);
}

/* 0x00089e40  (gap): steers at the target; when the car already points that way, boosts
(hard; medium one time in three). Returns 1 when pointing at it. */
int CBCarAI_Ram(struct CBCarAI *self)
{
	unsigned char *car;
	float aim[4], heading, window, carHeading;
	double spread;

	aim[0] = E_POS(self->target)[0];
	aim[1] = E_POS(self->target)[1];
	aim[2] = E_POS(self->target)[2];
	aim[3] = E_POS(self->target)[3];
	spread = (double)bmVectorDistance(aim, E_POS(AI_CAR(self))) * (double)0.07f;
	aim_jitter(self, aim, spread);
	car = AI_CAR(self);
	heading = Heading(aim[0] - E_POS(car)[0], aim[2] - E_POS(car)[2]);
	Car_Steer(AI_CAR(self), heading);
	bca_rand();
	bca_rand();
	car = AI_CAR(self);
	/* the window is (1 - accuracy) * 0 * 15 degrees: always 0 */
	window = (1.0f - self->accuracy) * 0.0f * 15.0f * 0x1.1df46ap-6f;
	carHeading = E_F(car, 0x84);
	if (!(heading - window <= carHeading))
		return 0;
	carHeading = E_F(car, 0x84);
	if (!(carHeading <= heading + window))
		return 0;
	if (DAT_002bae70 == 2) {
		Player_Boost(self->player);
		return 1;
	}
	if (DAT_002bae70 == 1 && bca_rand() % 3 == 0)
		Player_Boost(self->player);
	return 1;
}

/* 0x0008a070  (gap): drives away from the target */
void CBCarAI_Flee(struct CBCarAI *self)
{
	unsigned char *car;
	float aim[4];
	double spread;

	aim[0] = E_POS(self->target)[0];
	aim[1] = E_POS(self->target)[1];
	aim[2] = E_POS(self->target)[2];
	aim[3] = E_POS(self->target)[3];
	spread = (double)bmVectorDistance(aim, E_POS(AI_CAR(self))) * (double)0.07f;
	aim_jitter(self, aim, spread);
	car = AI_CAR(self);
	Car_Steer(AI_CAR(self), Heading(E_POS(car)[0] - aim[0], E_POS(car)[2] - aim[2]));
	drive_at_speed(self);
}

/* the waypoints of the track (game + 0x3618 count, + 0x3620 the current one, records of
0x4c bytes from + 0x3624 with four corners at +0xc, +0x1c, +0x2c, +0x3c) */
#define WP_COUNT BCG(0x3618, int)
#define WP_CURRENT BCG(0x3620, int)
#define WP_CORNER(i, c, axis) BCG(0x3630 + (i) * 0x4c + (c) * 0x10 + (axis) * 4, float)

/* 0x0008a1e0  (gap): drives to the current waypoint's first corner */
void CBCarAI_GotoWaypoint(struct CBCarAI *self)
{
	unsigned char *car = AI_CAR(self);

	Car_Steer(AI_CAR(self), Heading(WP_CORNER(WP_CURRENT, 0, 0) - E_POS(car)[0], WP_CORNER(WP_CURRENT, 0, 2) - E_POS(car)[2]));
	drive_at_speed(self);
}

/* the centre of waypoint i */
static void waypoint_centre(int i, float *out)
{
	out[3] = 1.0f;
	out[0] = (WP_CORNER(i, 0, 0) + WP_CORNER(i, 1, 0) + WP_CORNER(i, 2, 0) + WP_CORNER(i, 3, 0)) * 0.25f;
	out[1] = (WP_CORNER(i, 0, 1) + WP_CORNER(i, 1, 1) + WP_CORNER(i, 2, 1) + WP_CORNER(i, 3, 1)) * 0.25f;
	out[2] = (WP_CORNER(i, 0, 2) + WP_CORNER(i, 1, 2) + WP_CORNER(i, 2, 2) + WP_CORNER(i, 3, 2)) * 0.25f;
}

/* 0x0008a270  (gap): follows the waypoints: drives at the centre of the car's waypoint; if
something is in the way, tries the earlier waypoints in turn */
void CBCarAI_FollowWaypoints(struct CBCarAI *self)
{
	unsigned char *car;
	float aim[4], delta[3], hit[4], normal[4], heading, blocked;
	int i, start, n;
	double spread;

	n = WP_COUNT;
	if (n == 0)
		return;
	car = AI_CAR(self);
	waypoint_centre(E_I(car, 0x48), aim);
	spread = (double)bmVectorDistance(aim, E_POS(car)) * (double)0.07f;
	aim_jitter(self, aim, spread);
	car = AI_CAR(self);
	delta[0] = aim[0] - E_POS(car)[0];
	delta[1] = aim[1] - E_POS(car)[1];
	delta[2] = aim[2] - E_POS(car)[2];
	heading = Heading(delta[0], delta[2]);
	blocked = Ray_Obstacle(E_POS(AI_CAR(self)), delta, normal, hit);
	if (blocked != 0.0f) {
		i = (E_I(AI_CAR(self), 0x48) - 1 + WP_COUNT) % WP_COUNT;
		do {
			waypoint_centre(i, aim);
			spread = (double)bmVectorDistance(aim, E_POS(AI_CAR(self))) * (double)0.07f;
			aim_jitter(self, aim, spread);
			car = AI_CAR(self);
			delta[0] = aim[0] - E_POS(car)[0];
			delta[1] = aim[1] - E_POS(car)[1];
			delta[2] = aim[2] - E_POS(car)[2];
			heading = Heading(delta[0], delta[2]);
			blocked = Ray_Obstacle(E_POS(AI_CAR(self)), delta, normal, hit);
			i = (WP_COUNT - 1 + i) % WP_COUNT;
			start = (E_I(AI_CAR(self), 0x48) - 1 + WP_COUNT) % WP_COUNT;
			if (i == start)
				break;
		} while (blocked != 0.0f);
	}
	Car_Steer(AI_CAR(self), heading);
	drive_at_speed(self);
}

/* the ray of the flame tests: the car's position at its height, and its direction */
static void car_probe(struct CBCarAI *self, float *ray8, float *dir)
{
	unsigned char *car = AI_CAR(self);
	unsigned char *player = self->player;

	ray8[0] = E_F(car, 0x50);
	ray8[2] = E_F(car, 0x58);
	ray8[3] = E_F(car, 0x5c);
	ray8[1] = BCA_I32(BCA_AT(player, 0x138, unsigned char *), 0x1c) == 0 ? E_F(BCA_AT(player, 0x138, unsigned char *), 0xf4)
		: E_F(BCA_AT(player, 0x138, unsigned char *), 0xf8);
	ray8[1] = E_F(car, 0x54) + ray8[1];
	if (dir != NULL) {
		ray8[4] = dir[0];
		ray8[5] = dir[1];
		ray8[6] = dir[2];
		ray8[7] = dir[3];
	}
}

/* 0x0008a740  (gap): flames ahead: turn away and wander a while */
void CBCarAI_AvoidFlames(struct CBCarAI *self)
{
	float ray[8], dir[4], heading;

	FUN_000514a0(dir, (float *)(AI_CAR(self) + 0x90));
	dir[3] = 1.0f;
	dir[0] = dir[0] * self->lookAhead;
	dir[1] = dir[1] * self->lookAhead;
	dir[2] = dir[2] * self->lookAhead;
	car_probe(self, ray, dir);
	if (FUN_00095150(FLAMES, ray, E_F(AI_CAR(self), 0xd0), &heading) != 0) {
		self->distance = self->lookAhead;
		self->state = 2;
		self->heading = heading;
		Timer_SetSeconds(self->wanderTimer, WANDER_SECONDS(self));
		Timer_Begin(self->wanderTimer);
		return;
	}
	if (FUN_00095150(FLAMES, ray, E_F(AI_CAR(self), 0xd0), &heading) != 0) {
		self->state = 2;
		Timer_SetSeconds(self->wanderTimer, WANDER_SECONDS(self));
		Timer_Begin(self->wanderTimer);
		self->distance = 0.0f;
		self->heading = heading;
	}
}

/* the direction of travel in XZ (or the car's heading when still), times the look-ahead */
static void travel_dir(struct CBCarAI *self, float *vel, float *out, int scale_before)
{
	unsigned char *car = AI_CAR(self);
	float v[4];

	vel[0] = E_F(car, 0x90);
	vel[2] = E_F(car, 0x98);
	vel[3] = E_F(car, 0x9c);
	vel[1] = 0.0f;
	if (sqrtf(vel[2] * vel[2] + vel[0] * vel[0]) == 0.0f) {
		v[0] = 0.0f;
		v[1] = 0.0f;
		v[2] = scale_before ? self->lookAhead : 1.0f;
		v[3] = 1.0f;
		bmVectorRotateY(out, v, E_F(AI_CAR(self), 0x84));
	} else {
		FUN_000514a0(out, vel);
		if (scale_before) {
			out[0] = out[0] * self->lookAhead;
			out[1] = out[1] * self->lookAhead;
			out[2] = out[2] * self->lookAhead;
		}
	}
}

/* 0x0008a8b0  (gap): a hole ahead (no ground within 3 look-aheads below the point ahead):
turn away (blending towards the new heading while the wander timer runs) */
void CBCarAI_AvoidHoles(struct CBCarAI *self)
{
	float vel[4], dir[4], heading, remaining;
	struct BRay ray;
	struct BCollHit hit;
	int count;
	unsigned char *car;

	if (E_I(AI_CAR(self), 0x894) == 0)
		return;
	travel_dir(self, vel, dir, 1);
	car = AI_CAR(self);
	self->point[0] = dir[0] + E_POS(car)[0];
	self->point[1] = dir[1] + E_POS(car)[1];
	hit.flags = BCOLL_T;
	count = 1;
	self->point[2] = dir[2] + E_POS(car)[2];
	DAT_00285644 = 0.01f;
	ray.origin[1] = self->point[1];
	ray.origin[0] = self->point[0];
	ray.length = self->lookAhead * 3.0f;
	ray.origin[2] = self->point[2];
	ray.origin[3] = self->point[3];
	ray.dir[1] = -ray.length;
	ray.dir[2] = 0.0f;
	ray.dir[0] = 0.0f;
	if (bmRayMesh(&ray, BCG(0x78, struct BCollMesh *), &count, &hit) != 0)
		return;
	if (Timer_Remaining(self->wanderTimer) == 0.0f) {
		self->state = 2;
		heading = Heading(vel[0], vel[2]);
		self->distance = hit.t;
	} else {
		if (self->distance <= hit.t)
			return;
		heading = Heading(vel[0], vel[2]);
		remaining = Timer_Remaining(self->wanderTimer);
		self->state = 2;
		self->distance = hit.t;
		heading = (float)((double)heading - (((double)heading - (double)self->heading) /
			(double)WANDER_SECONDS(self)) * (double)remaining);
	}
	self->heading = heading;
	Timer_SetSeconds(self->wanderTimer, WANDER_SECONDS(self));
	Timer_Begin(self->wanderTimer);
}

/* 0x0008ab20  (gap): a wall within the look-ahead: turn away (Pinball turns the other way
by 90 degrees) */
void CBCarAI_AvoidWalls(struct CBCarAI *self)
{
	float vel[4], heading, remaining;
	struct BRay ray;
	struct BCollHit hit;
	int count;
	unsigned char *car;
	float length = self->lookAhead;

	travel_dir(self, vel, ray.dir, 0);
	car = AI_CAR(self);
	ray.origin[0] = E_POS(car)[0];
	ray.origin[2] = E_POS(car)[2];
	ray.origin[3] = E_POS(car)[3];
	ray.origin[1] = BCA_I32(car, 0x1c) == 0 ? E_F(car, 0xf4) : E_F(car, 0xf8);
	ray.origin[1] = E_POS(car)[1] + ray.origin[1];
	ray.dir[0] = ray.dir[0] * length;
	hit.flags = BCOLL_T;
	count = 1;
	ray.dir[1] = ray.dir[1] * length;
	DAT_00285644 = 0.01f;
	ray.dir[2] = ray.dir[2] * length;
	ray.length = length;
	if (bmRayMesh(&ray, BCG(0x78, struct BCollMesh *), &count, &hit) == 0)
		return;
	if (Timer_Remaining(self->wanderTimer) == 0.0f) {
		self->state = 2;
		self->heading = Heading(vel[0], vel[2]);
		self->distance = hit.t;
		Timer_SetSeconds(self->wanderTimer, WANDER_SECONDS(self));
		Timer_Begin(self->wanderTimer);
	} else if (hit.t < self->distance) {
		heading = Heading(vel[0], vel[2]);
		remaining = Timer_Remaining(self->wanderTimer);
		self->heading = (float)((double)heading - (((double)heading - (double)self->heading) /
			(double)WANDER_SECONDS(self)) * (double)remaining);
	}
	if (g_BCarGame->variation == 2) {
		self->heading = self->heading + 0x1.921fb4p+1f;
		if (CLMath_WrapAngle(self->heading) < 0x1.921fb4p+1f)
			self->heading = self->heading + 0x1.921fb4p+0f;
		else
			self->heading = self->heading - 0x1.921fb4p+0f;
	}
}

/* 0x0008ad90  (gap): the goal ahead: turn away */
void CBCarAI_AvoidGoal(struct CBCarAI *self)
{
	float dir[4], p[3];
	unsigned char *car;

	FUN_000514a0(dir, (float *)(AI_CAR(self) + 0x90));
	dir[0] = dir[0] * self->lookAhead;
	dir[1] = dir[1] * self->lookAhead;
	dir[2] = dir[2] * self->lookAhead;
	car = AI_CAR(self);
	p[0] = dir[0] + E_POS(car)[0];
	p[1] = dir[1] + E_POS(car)[1];
	p[2] = dir[2] + E_POS(car)[2];
	if (FUN_00091a00(GOAL, p) != NULL) {
		self->state = 2;
		car = AI_CAR(self);
		self->heading = Heading(E_F(car, 0x90), E_F(car, 0x98));
		self->distance = self->lookAhead;
		Timer_SetSeconds(self->wanderTimer, WANDER_SECONDS(self));
		Timer_Begin(self->wanderTimer);
	}
}

/* 0x0008ae80  (gap): chooses what to do next by a weighted draw: the nearest entity, the
furthest one, the leader, the one nearest the goal (then to the goal), or the keeper */
void CBCarAI_FindTarget(struct CBCarAI *self)
{
	int choices[50];
	int n, i, kind;
	unsigned int behaviour = self->behaviour;
	const char *message;

	choices[0] = 0;
	choices[1] = 1;
	n = 2;
	if ((behaviour & AI_LEADER) != 0 && self->leader != NULL) {
		choices[2] = 2;
		choices[3] = 2;
		choices[4] = 2;
		n = 5;
	}
	if ((behaviour & AI_GOAL) != 0) {
		for (i = 0; i < 4; i++)
			choices[n + i] = 3;
		n += 4;
	}
	if ((behaviour & AI_KEEPER) != 0) {
		for (i = 0; i < 12; i++)
			choices[n + i] = 4;
		n += 12;
	}
	i = bca_rand() % n;
	self->state = 2;
	switch (choices[i]) {
	case 0:
		if (g_BCarGame->variation == 1 && (int)Core_Rand() % 3 == 0)
			kind = 2;
		else
			kind = 0;
		self->target = CBCarAI_FindNearest(self, kind);
		if (self->target == NULL) {
			message = "FindNearest failed\n";
			BCA_Printf(message);
		}
		break;
	case 1:
		self->target = CBCarAI_FindFurthest(self);
		if (self->target == NULL)
			BCA_Printf("FindFurthest failed\n");
		break;
	case 2:
		self->target = BCA_AT(self->leader, 0x138, unsigned char *);
		if (self->target == NULL)
			BCA_Printf("leadPl failed\n");
		break;
	case 3:
		self->target = CBCarAI_FindNearestToGoal(self);
		self->state = 3;
		if (self->target == NULL)
			BCA_Printf("FindNearestToGoal failed\n");
		break;
	case 4:
		if (self->keeper == NULL)
			self->target = CBCarAI_FindNearestPuck(self);
		else if (Ent_SameSide(AI_CAR(self), self->keeper) != 0)
			self->target = CBCarAI_FindNearest(self, 1);
		else
			self->target = self->keeper;
		if (self->target == NULL)
			BCA_Printf("KeeperEnt failed\n");
		break;
	}
	if (self->state == 2) {
		if (self->target != NULL)
			self->state = 1;
		if (self->state == 2)
			goto check;
	}
	Timer_SetSeconds(self->targetTimer, TARGET_SECONDS(self));
	Timer_Begin(self->targetTimer);
check:
	if (self->target == NULL)
		BCA_Printf("FindTarget: Could not find a target\n");
}

/* 0x0008b0a0  (gap): a better target: King (variation 3) rams a flashing car in reach;
Bumper Race (1) prefers a kind-2 entity nearer the goal */
void CBCarAI_RethinkTarget(struct CBCarAI *self)
{
	unsigned char *e, *car;
	float ray[4], delta[3];

	if (g_BCarGame->variation == 3) {
		if (E_S(self->target, 0x8a4) != 0)
			return;
		e = CBCarAI_FindNearestFlashing(self);
		if (e != NULL) {
			car = AI_CAR(self);
			delta[0] = E_POS(e)[0] - E_POS(car)[0];
			delta[1] = E_POS(e)[1] - E_POS(car)[1];
			delta[2] = E_POS(e)[2] - E_POS(car)[2];
			car_probe(self, ray, NULL);
			if (FUN_000951f0(FLAMES, ray, E_F(AI_CAR(self), 0xd0), delta) == 0) {
				self->target = e;
				CBCarAI_Ram(self);
				return;
			}
		}
	}
	if (g_BCarGame->variation == 1 && E_I(self->target, 0) != 2) {
		e = CBCarAI_FindNearest(self, 2);
		if (e != NULL && FUN_00091aa0(GOAL, E_POS(e)) < FUN_00091aa0(GOAL, E_POS(self->target)))
			self->target = e;
	}
}

/* brake when much faster than the car's size allows */
static void maybe_brake(struct CBCarAI *self)
{
	unsigned char *car;

	if ((self->behaviour & AI_BRAKE) != 0 || self->hasPuck != 0) {
		car = AI_CAR(self);
		if (E_F(car, 0xd0) + E_F(car, 0xd0) < car_speed(car))
			Player_Brake(self->player);
	}
}

/* 0x0008b1e0  (gap): state 1 / 3: chase (or circle, ram, block) the target */
void CBCarAI_StateChase(struct CBCarAI *self)
{
	unsigned char *saved, *e;
	int odds;

	if ((self->behaviour & AI_NO_TARGET) == 0) {
		if (self->target == NULL) {
			self->state = 0;
			return;
		}
		if (Timer_Remaining(self->targetTimer) == 0.0f) {
			self->target = NULL;
			self->state = 0;
			return;
		}
		CBCarAI_RethinkTarget(self);
		if ((self->behaviour & AI_BLOCK) != 0) {
			e = CBCarAI_FindBlocker(self);
			saved = self->target;
			if (e != NULL) {
				self->target = e;
				CBCarAI_Ram(self);
				self->target = saved;
			}
		}
		if ((self->behaviour & AI_JITTER) != 0) {
			if (self->circle == 0)
				CBCarAI_Chase(self);
			else
				CBCarAI_Circle(self);
		}
		if ((self->behaviour & AI_RAM_NEAR) != 0 &&
		    (double)bmVectorDistance(E_POS(AI_CAR(self)), E_POS(self->target)) <
			    (double)E_F(AI_CAR(self), 0xd0) + (double)E_F(self->target, 0xd0) + 600.0)
			CBCarAI_Ram(self);
		if (self->state == 3 && CBCarAI_Ram(self) != 0)
			self->state = 1;
	}
	maybe_brake(self);
	if ((self->behaviour & AI_WAYPOINTS) != 0) {
		CBCarAI_FollowWaypoints(self);
		if (DAT_002bae70 == 2) {
			if (car_speed(AI_CAR(self)) < 50.0f)
				Player_Boost(self->player);
			odds = 10;
		} else {
			if (DAT_002bae70 != 1)
				goto puck;
			if (car_speed(AI_CAR(self)) < 50.0f && (int)Core_Rand() % 4 == 0)
				Player_Boost(self->player);
			odds = 0x1e;
		}
		if (bca_rand() % odds == 0)
			Player_Boost(self->player);
	}
puck:
	if ((self->behaviour & AI_PUCK) != 0 && self->hasPuck != 0)
		CBCarAI_GotoWaypoint(self);
}

/* 0x0008b480  (gap): state 4: flee from the target */
void CBCarAI_StateFlee(struct CBCarAI *self)
{
	if ((self->behaviour & AI_NO_TARGET) == 0) {
		if (self->target == NULL) {
			self->state = 0;
			return;
		}
		if (Timer_Remaining(self->targetTimer) == 0.0f) {
			self->target = NULL;
			self->state = 0;
			return;
		}
		CBCarAI_Flee(self);
	}
	maybe_brake(self);
	if ((self->behaviour & AI_WAYPOINTS) != 0) {
		CBCarAI_FollowWaypoints(self);
		if (DAT_002bae70 == 2) {
			if (car_speed(AI_CAR(self)) < 50.0f)
				Player_Boost(self->player);
		} else if (DAT_002bae70 == 1 && car_speed(AI_CAR(self)) < 30.0f && (int)Core_Rand() % 3 == 0) {
			Player_Boost(self->player);
		}
	}
	if ((self->behaviour & AI_PUCK) != 0 && self->hasPuck != 0)
		CBCarAI_GotoWaypoint(self);
}

/* 0x0008b600  (gap): carrying the puck: ram whatever is near, drive to the waypoint */
void CBCarAI_StatePuck(struct CBCarAI *self)
{
	unsigned char *e, *saved;
	int odds;

	if ((self->behaviour & AI_RAM_NEAR) != 0 && (e = CBCarAI_FindNearest(self, 0)) != NULL &&
	    (double)bmVectorDistance(E_POS(AI_CAR(self)), E_POS(e)) <
		    (double)E_F(AI_CAR(self), 0xd0) + (double)E_F(e, 0xd0) + 600.0) {
		saved = self->target;
		self->target = e;
		CBCarAI_Ram(self);
		self->target = saved;
	}
	maybe_brake(self);
	CBCarAI_GotoWaypoint(self);
	if (DAT_002bae70 == 2) {
		if (car_speed(AI_CAR(self)) < 50.0f)
			Player_Boost(self->player);
		odds = 10;
	} else {
		if (DAT_002bae70 != 1)
			return;
		if (car_speed(AI_CAR(self)) < 50.0f && bca_rand() % 3 == 0)
			Player_Boost(self->player);
		odds = 0x1e;
	}
	if (bca_rand() % odds == 0)
		Player_Boost(self->player);
}

/* 0x0008b800  (gap): the senses: the look-ahead from the speed (by variation), the leader,
the keeper, the puck, then the avoidance tests; and whether the target aims back at us */
void CBCarAI_Sense(struct CBCarAI *self)
{
	unsigned char *car, *e, *other;
	float speed, base, extra;
	int variation = g_BCarGame->variation;

	car = AI_CAR(self);
	speed = car_speed(car);
	switch (variation) {
	case 1:
		base = E_F(car, 0xd0) * 3.0f + speed + speed;
		self->lookAhead = base;
		extra = (1.0f - self->patience) * car_speed(AI_CAR(self)) * 3.0f;
		self->lookAhead = extra + base;
		break;
	case 2:
		self->lookAhead = E_F(car, 0xd0) * 4.0f + speed * 4.0f * self->patience;
		break;
	case 3:
		base = speed + speed + E_F(car, 0xd0);
		self->lookAhead = base;
		extra = (1.0f - self->patience) * car_speed(AI_CAR(self));
		extra = extra + extra;
		self->lookAhead = extra + base;
		break;
	case 4:
		base = E_F(car, 0xd0) * 3.0f + speed * 3.0f;
		self->lookAhead = base;
		extra = (1.0f - self->patience) * car_speed(AI_CAR(self)) * 5.0f;
		self->lookAhead = extra + base;
		break;
	default:
		break;
	}
	self->leader = BCG(0x32d8, unsigned char *);
	self->keeper = NULL;
	for (e = ENT_FIRST(); e != NULL; e = E_NEXT(e)) {
		if (e != AI_CAR(self) && E_I(e, 0x10) != 0) {
			self->keeper = e;
			break;
		}
	}
	self->hasPuck = E_I(AI_CAR(self), 0x10) != 0;
	if ((self->behaviour & AI_AVOID_HOLES) != 0)
		CBCarAI_AvoidHoles(self);
	if ((self->behaviour & AI_AVOID_GOAL) != 0)
		CBCarAI_AvoidGoal(self);
	CBCarAI_AvoidFlames(self);
	if ((self->behaviour & AI_NO_WALLS) == 0 || (variation == 2 && self->hasPuck == 0))
		CBCarAI_AvoidWalls(self);
	self->circle = 0;
	if (self->target != NULL) {
		other = BCA_AT(self->target, 0x8b4, unsigned char *); /* the target's player slot */
		if (other != NULL && BCA_I32(other, 0) == 0 && BCA_AT(other, 0x48, unsigned char *) == AI_CAR(self)) {
			if ((self->behaviour & AI_CIRCLE) != 0) {
				self->circle = 1;
				return;
			}
			if (BCA_F32(other, 0x18) < self->aggression) {
				BCA_I32(other, 0x38) = 4; /* it flees */
				return;
			}
			self->state = 4;
		}
	}
}

/* 0x0008bb30  (gap): state 2: wander on the chosen heading (boost or brake when it is
roughly right) until the wander timer runs out */
void CBCarAI_StateWander(struct CBCarAI *self)
{
	unsigned char *car;
	float heading, speed, carHeading, diff;
	int difficulty;

	if (Timer_Remaining(self->wanderTimer) == 0.0f) {
		self->target = NULL;
		self->state = 0;
		return;
	}
	if ((self->behaviour & AI_PUCK) != 0 && self->hasPuck != 0) {
		CBCarAI_GotoWaypoint(self);
		return;
	}
	if (Timer_Remaining(self->targetTimer) == 0.0f)
		CBCarAI_FindTarget(self);
	bca_rand();
	bca_rand();
	heading = (1.0f - self->accuracy) * 0.0f * 15.0f * 0x1.1df46ap-6f + self->heading + 0x1.921fb4p+1f;
	Car_Steer(AI_CAR(self), heading);
	drive_at_speed(self);
	car = AI_CAR(self);
	speed = sqrtf(E_F(car, 0x90) * E_F(car, 0x90) + E_F(car, 0x98) * E_F(car, 0x98) + E_F(car, 0x94) * E_F(car, 0x94));
	if (speed == 0.0f)
		carHeading = E_F(car, 0x84);
	else
		carHeading = Heading(E_F(car, 0x90), E_F(car, 0x98));
	diff = CLMath_WrapAngle(CLMath_WrapAngle(carHeading) - CLMath_WrapAngle(heading));
	difficulty = DAT_002bae70;
	if (diff < 0x1.657184p-2f) {
		if (DAT_002bae70 == 2 || (DAT_002bae70 == 1 && (int)Core_Rand() % 3 == 0)) {
			Player_Brake(self->player);
			difficulty = DAT_002bae70;
		}
		car = AI_CAR(self);
		if (E_F(car, 0xd0) * 1.5f < speed || self->distance <= E_F(car, 0xd0)) {
			if (difficulty == 2 || (difficulty == 1 && (int)Core_Rand() % 3 == 0))
				Player_Boost(self->player);
		}
	}
	if (speed < E_F(AI_CAR(self), 0xd0)) {
		diff = CLMath_WrapAngle(CLMath_WrapAngle(carHeading) - CLMath_WrapAngle(heading));
		if (diff < 0x1.657184p-2f) {
			if (DAT_002bae70 != 2) {
				if (DAT_002bae70 != 1)
					return;
				if ((int)Core_Rand() % 3 != 0)
					return;
			}
			Player_Boost(self->player);
		}
	}
}

/* 0x0008bdd0  (gap): one frame of the driver */
void CBCarAI_Update(struct CBCarAI *self, int param)
{
	unsigned char *car;

	if (self->behaviour == 0)
		return;
	self->param = param;
	if (self->aggression != self->last[0])
		self->last[0] = self->aggression;
	if (self->reaction != self->last[1]) {
		self->last[1] = self->reaction;
		TARGET_SECONDS(self) = self->reaction * 5.0f + 1.0f;
	}
	if (self->patience != self->last[2]) {
		self->last[2] = self->patience;
		WANDER_SECONDS(self) = (1.0f - self->patience) * 0.5f + 0.2f;
	}
	if (self->accuracy != self->last[3])
		self->last[3] = self->accuracy;
	if (self->target != NULL && E_I(self->target, 0x20) == 0)
		self->target = NULL;
	if (self->target != NULL && E_I(self->target, 0x894) == 0)
		self->target = NULL;
	car = AI_CAR(self);
	if (E_I(car, 0x20) == 0)
		self->target = NULL;
	if (E_I(AI_CAR(self), 0x894) == 0)
		self->target = NULL;
	if (self->target == NULL && (self->behaviour & AI_NO_TARGET) == 0)
		self->state = 0;
	CBCarAI_Sense(self);
	Timer_Tick(self->targetTimer);
	Timer_Tick(self->wanderTimer);
	if ((self->behaviour & AI_PUCK) != 0 && self->hasPuck != 0) {
		CBCarAI_StatePuck(self);
		return;
	}
	switch (self->state) {
	case 0:
		CBCarAI_FindTarget(self);
		break;
	case 1:
	case 3:
		CBCarAI_StateChase(self);
		break;
	case 2:
		CBCarAI_StateWander(self);
		break;
	case 4:
		CBCarAI_StateFlee(self);
		break;
	}
	if (self->target == NULL) {
		self->state = 2;
		CBCarAI_StateWander(self);
	}
}
