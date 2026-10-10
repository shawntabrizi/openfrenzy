/*
Hopper/CHopper_Antics.c  <-  C:\BlitzParty\Game\Src\Hopper\CHopper_Antics.cpp
(module hopper)

The "antics" object (0x002ae878, vtable 0x0026e330): the players and the rules of each
variation. Only 0x0013c360 carries the file name (a heap tag); the gap functions between
CModule_Hopper.cpp and CHopper_Game.cpp (0x0013bb70-0x0013ea80) are placed here by address
and because they work on this object (file inferred).

Variations (hop_Variation): 1 Prop Rotation (two turning propellers to jump over),
2 Centrifugal Farce (a spinning disc of 27 platforms), 3 Careless Torque (two turning
blades, players on a ring), 4 Twisted System (players ride a turning arm).

The object (0xd0 bytes):
  +0x04 float[4][4]  per player: the ground normal under its ring (x = -1: not known yet)
  +0x54 int[4]       per player: its position on the arm (variation 4: finishing order)
  +0x74              a render pass parameter (flash of a hit player)
  +0x80 float[5][4]  points of the debug free camera
Player AI objects: hop_Players (0x00302b48, also charlib's AI table 0x002f289c).
*/

#include "hopper.h"

static const char s_AnticsFile[] = "C:\\BlitzParty\\Game\\Src\\Hopper\\CHopper_Antics.cpp";

/* per player counters of the rules (0x002aff14..) */
#define hop_HitTimer HOP_IV(0x002aff14)        /* frames of invulnerability after a hit */
#define hop_Score HOP_IV(0x002aff24)           /* score (lives) */
#define hop_Order HOP_IV(0x002aff34)           /* variation 3: finishing place */
#define hop_Fell HOP_IV(0x002aff8c)            /* variation 2: fell off (sound played) */
#define hop_Frozen HOP_IV(0x002afef4)          /* frames the score display flashes */
#define hop_Out HOP_IV(0x002aff54)             /* variation 4: thrown off the arm */
#define hop_Spin HOP_IV(0x002aff04)            /* variation 4: frames spinning off the arm */

/* the light table of the level (7 lights, 0x002aec24-0x002aec40) */
#define hop_Lights ((struct BLight **)HOP_ADDR(0x002aec24))

/* the actor list */
static inline void *hop_NextActor(void *actor)
{
	return (void *)(uintptr_t)CLList_Next(DAT_002ea488, (unsigned int *)actor);
}

/* the score text that floats up from a player who lost points */
static void hop_FloatScore(int player, int lost)
{
	char text[44];
	void *actor = hop_PlayerActor(player);
	unsigned char *colour = DAT_00290718 + DAT_002baed4[player][0] * 0x60;

	sprintf(text, "-%d", lost);
	baAddExtraItem(HOP_G(0x002aeac0, struct BExtraSystem *), HOP_P(0x002ae99c), (double)HOP_AF(actor, 0xd0),
		(double)HOP_AF(actor, 0xd4), (double)HOP_AF(actor, 0xd8), (int)colour[0], (int)colour[1],
		(int)colour[2], text);
}

/* the AI of a computer player thinks (virtual 0x0c) */
static inline void hop_AIThink(void *player)
{
	HOP_VFN(player, 0x0c, void (*)(void *))(player);
}

/* 0x0013bb70  (gap): draws the bones below `node` of an actor instance as lines from the
node to each child (recursively), in colour (alpha, red, green, blue). The line's first
vertex is set once, so after a child's subtree the next sibling starts where the subtree's
last line started (as in the original). */
void HopAntics_DrawSkeleton(struct BActorInstance *inst, struct BActorNode *node, int alpha, int red,
	int green, int blue)
{
	float *line = (float *)HOP_ADDR(0x00302510);
	float pos[4];
	void *child;

	baGetNodePosition(inst, node, pos);
	line[1] = pos[1];
	line[0] = pos[0];
	line[2] = pos[2];
	HOP_AU(line, 0x18) = HOP_ARGB(alpha, red, green, blue);
	child = HOP_AP(node, 0x14c);
	if (child == NULL)
		return;
	do {
		baGetNodePosition(inst, (struct BActorNode *)child, pos);
		line[10] = pos[1];
		line[9] = pos[0];
		line[11] = pos[2];
		HOP_AU(line, 0x3c) = HOP_ARGB(alpha, red, green, blue);
		bdDrawPrimitiveFVF(1, line, 2, 0x152);
		HopAntics_DrawSkeleton(inst, (struct BActorNode *)child, alpha, red, green, blue);
		child = HOP_AP(child, 0x140);
	} while (child != HOP_AP(node, 0x14c));
}

/* 0x0013bc90  (gap): variation 1: the player's ring on the floor (y -134, +5 unless the
variation is 4) in the player's colour */
void HopAntics_DrawRing1(int player, float *pos)
{
	float *normal;
	unsigned char *colour;
	unsigned int argb;

	if (hop_Play == 0)
		return;
	pos[1] = -134.0f;
	normal = (float *)HOP_ADDR(0x002ae87c) + player * 4;
	normal[1] = -1.0f;
	normal[2] = 0.0f;
	normal[3] = 1.0f;
	normal[0] = 0.0f;
	if (hop_Variation != 4)
		pos[1] = pos[1] + 5.0f;
	CLHud_BuildGroundQuad((struct CLQuadVertex *)HOP_ADDR(0x00302558), pos, normal, 96.0f, 0.0f, 80.0f);
	colour = DAT_00290718 + DAT_002baed4[player][0] * 0x60;
	argb = HOP_ARGB(255, colour[0], colour[1], colour[2]);
	HOP_U(0x00302570) = argb;
	HOP_U(0x00302594) = argb;
	HOP_U(0x003025b8) = argb;
	HOP_U(0x003025dc) = argb;
	bdSetTexture(0, HOP_G(0x002aea3c, struct bdTexture *));
	bdSetWorldIdentity();
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(3, 0, 0);
	if (hop_Variation == 4) {
		bdPushRenderState(5, 0, 0);
		bdPushRenderState(4, 0, 0);
	}
	bdDrawPrimitiveFVF(4, HOP_ADDR(0x00302558), 4, 0x152);
	if (hop_Variation == 4) {
		bdPopRenderState(4);
		bdPopRenderState(5);
	}
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(3);
}

/* 0x0013be10  (gap): scalar deleting destructor of the antics object */
void *HopAntics_Destroy(void *self, byte flags)
{
	*(const void *const **)self = vtable_0026e330;
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026e330  the antics object's vtable */
const void *const vtable_0026e330[1] = {
	(const void *)FUN_0013be10, /* [0] scalar deleting destructor */
};

/* 0x0013be30  (gap): puts an actor where it is (no move this frame), stops it, and pushes
every other player that overlaps it (the sum of both radii, char data +0x9c) away from it
by 20 */
void HopAntics_PlaceActor(void *self, void *actor)
{
	float away[4];
	void *other;
	float dist;
	unsigned int i;

	(void)self;
	hop_ActorSnap(actor);
	HOP_AI(actor, 0x19c) = 0;
	HOP_AI(actor, 0x198) = 0;
	HOP_AI(actor, 0x194) = 0;
	for (i = 0; (int)i < DAT_002bae94; i++) {
		if (i == HOP_AUS(actor, 0x230))
			continue;
		other = HOP_AP(DAT_002f289c[i], 0x30);
		away[0] = HOP_AF(other, 0xd0) - HOP_AF(actor, 0xd0);
		away[1] = 0.0f;
		away[2] = HOP_AF(other, 0xd8) - HOP_AF(actor, 0xd8);
		other = HOP_AP(DAT_002f289c[i], 0x30);
		dist = sqrtf(away[0] * away[0] + away[2] * away[2]);
		if (dist < HOP_AF(HOP_AP(other, 0x18c), 0x9c) + HOP_AF(HOP_AP(actor, 0x18c), 0x9c)) {
			if (dist == 0.0f)
				HOP_AF(other, 0x194) = HOP_AF(other, 0x194) + 20.0f;
			else
				bmVectorBlend(HOP_AV(other, 0x194), HOP_AV(other, 0x194), 1.0f, away, 20.0f / dist);
		}
	}
}

/* the 12 character models: the table at 0x002ae818 ({name, model} pairs) gets the models
loaded at 0x002afe84 */
static void hop_SetCharacterModels(void)
{
	int k;

	for (k = 0; k < 12; k++)
		HOP_IV(0x002ae81c)[k * 2] = HOP_IV(0x002afe84)[k];
}

/* 0x0013bfc0  (gap): puts the players back at their start for a new round */
void HopAntics_Reset(void *self)
{
	float turn;
	float x = 0.0f, z = 0.0f;
	float a, r;
	float *normal;
	void *actor;
	int i;

	CProjector_Reset();
	turn = (float)(hop_rand() & 3);
	hop_SetCharacterModels();
	normal = (float *)((char *)self + 4);
	for (i = 0; i < DAT_002bae94; i++, normal += 4) {
		actor = hop_PlayerActor(i);
		switch (hop_Variation) {
		case 1:
			HOP_AU(actor, 0xd4) = 0xc306c831u;    /* -134.78 */
			a = (turn * 90.0f + (float)i * 90.0f + 45.0f) * HOP_DEG;
			x = sinf(a) * 600.0f;
			z = cosf(a) * 600.0f;
			break;
		case 2:
			HOP_AF(actor, 0xd4) = 0.0f;
			a = ((float)i * 90.0f + 45.0f) * HOP_DEG;
			x = sinf(a) * 100.0f;
			z = cosf(a) * 100.0f;
			break;
		case 3:
			HOP_AF(actor, 0xd4) = -269.0f;
			r = (float)i * 100.0f + 1100.0f;
			x = (float)(sin(3.4906585216522217) * r);
			z = (float)(cos(3.4906585216522217) * r);
			break;
		case 4:
			HOP_AF(actor, 0xd4) = 500.0f;
			a = (hop_ArenaAngle - 125.0f) * HOP_DEG;
			r = (float)i * 140.0f + 800.0f;
			x = -(sinf(a) * r);
			z = -(cosf(a) * r);
			FUN_00142d50((int)(uintptr_t)HOP_ADDR(0x002aec10), 0);
			break;
		}
		actor = hop_PlayerActor(i);
		HOP_AF(actor, 0xd0) = x;
		HOP_AF(actor, 0xd8) = z;
		hop_ActorSnap(actor);
		HOP_AI(actor, 0x19c) = 0;
		HOP_AI(actor, 0x198) = 0;
		HOP_AI(actor, 0x194) = 0;
		HOP_AI(hop_Players[i], 0x1180) = 0;
		HOP_AI(hop_Players[i], 0x1184) = 0;
		hop_Score[i] = 0;
		normal[2] = -1.0f;
		normal[1] = -1.0f;
		normal[0] = -1.0f;
		normal[3] = 1.0f;
		AI_SetState(hop_Players[i], 0);
		HopAntics_PlaceActor(self, hop_PlayerActor(i));
	}
	for (i = 0x20; i != 0; i--) {
		Actor_UpdateAll();
		Actor_UpdateAnimations();
	}
}

/* 0x0013c360  Hopper/CHopper_Antics.cpp: makes the four players (Hopper AI players, 0x119c
bytes) and puts them at their start; variation 4 takes the order from the board game's
standings. Then the shadows, the routes (variation 1) and 32 settling frames. */
void HopAntics_Start(void *self)
{
	float pos[4];
	float turn;
	float a, r, base;
	int *order;
	float *normal;
	void *mem;
	void *player;
	void *actor;
	int i;

	turn = (float)(hop_rand() & 3);
	CProjector_Reset();
	hop_SetCharacterModels();
	order = (int *)((char *)self + 0x54);
	if (hop_Variation == 4) {
		bkPrintf("mgsel=%d %d %d %d\n", HOP_AI(DAT_002ba55c, 0x1b0), HOP_AI(DAT_002ba55c, 0x1b4),
			HOP_AI(DAT_002ba55c, 0x1b8), HOP_AI(DAT_002ba55c, 0x1bc));
		bkPrintf("miniscores=%d %d %d %d\n", DAT_002baed4[0][5], DAT_002baed4[1][5], DAT_002baed4[2][5],
			DAT_002baed4[3][5]);
		CScoreTable_Rank((struct CScoreTable *)DAT_002b9968);
		for (i = 0; i < 4; i++) {
			if (i == DAT_002b9c58[0])
				order[i] = 0;
			if (i == DAT_002b9c58[1])
				order[i] = 1;
			if (i == DAT_002b9c58[2])
				order[i] = 2;
			if (i == DAT_002b9c58[3])
				order[i] = 3;
		}
	}
	bkPrintf("player in position=%d %d %d %d\n", DAT_002b9c58[0], DAT_002b9c58[1], DAT_002b9c58[2],
		DAT_002b9c58[3]);
	bkPrintf("order=%d %d %d %d\n", order[0], order[1], order[2], order[3]);
	normal = (float *)((char *)self + 4);
	for (i = 0; i < DAT_002bae94; i++, normal += 4) {
		pos[3] = 0.0f;
		pos[2] = 0.0f;
		pos[1] = 0.0f;
		pos[0] = 0.0f;
		mem = HOP_NEW(0x119c, s_AnticsFile, 0x118);
		if (mem == NULL)
			player = NULL;
		else
			player = HopPlayer_Ctor(mem, i, pos);
		hop_Players[i] = player;
		DAT_002f289c[i] = player;
		HOP_VFN(player, 0x40, void (*)(void *, int, int))(player, 0, 0);
		switch (hop_Variation) {
		case 1:
			a = (turn * 90.0f + (float)i * 90.0f + 45.0f) * HOP_DEG;
			pos[0] = sinf(a) * 600.0f;
			pos[2] = cosf(a) * 600.0f;
			HOP_AU(hop_PlayerActor(i), 0xd4) = 0xc306c831u;    /* -134.78 */
			HOP_AF(hop_PlayerActor(i), 0x1d8) = a;
			break;
		case 2:
			a = ((float)i * 90.0f + 45.0f) * HOP_DEG;
			pos[0] = sinf(a) * 150.0f;
			pos[2] = cosf(a) * 150.0f;
			HOP_AF(hop_PlayerActor(i), 0xd4) = 0.0f;
			HOP_AF(hop_PlayerActor(i), 0x1d8) = a;
			break;
		case 3:
			base = 90.0f;
			if (i < 2)
				base = 210.0f;
			a = ((float)i * 20.0f + base) * HOP_DEG;
			pos[0] = sinf(a) * 1300.0f;
			pos[2] = cosf(a) * 1300.0f;
			HOP_AF(hop_PlayerActor(i), 0xd4) = -269.0f;
			HOP_AU(hop_PlayerActor(i), 0x1d8) = 0x40490fdbu;    /* pi */
			break;
		case 4:
			HOP_AF(hop_PlayerActor(i), 0xd4) = 500.0f;
			a = (hop_ArenaAngle - 125.0f) * HOP_DEG;
			turn = a;
			r = (float)order[i] * 140.0f + 800.0f;
			pos[0] = -(sinf(a) * r);
			pos[2] = -(cosf(a) * r);
			break;
		}
		actor = hop_PlayerActor(i);
		HOP_AF(actor, 0xd0) = pos[0];
		HOP_AF(actor, 0xd8) = pos[2];
		hop_ActorSnap(actor);
		normal[2] = -1.0f;
		normal[1] = -1.0f;
		normal[0] = -1.0f;
		normal[3] = 1.0f;
		HOP_AI(actor, 0x19c) = 0;
		HOP_AI(actor, 0x198) = 0;
		HOP_AI(actor, 0x194) = 0;
		AI_SetState(hop_Players[i], 0);
	}
	HopPlayers_Start();
	if (hop_Variation == 3)
		Shadows_Init(2, 1.1f, 0.0f, 0.0f);
	else if (hop_Variation == 4)
		Shadows_Init(0, 0.0f, 0.0f, 0.0f);
	else
		Shadows_Init(2, 1.0f, 0.0f, 0.0f);
	if (hop_Variation == 1)
		CLRoute_Rebuild();
	for (i = 0x20; i != 0; i--) {
		Actor_UpdateAll();
		Actor_UpdateAnimations();
	}
}

/* the debris burst of a player who fell into the void (the respawn effect in the player's
character colour) */
extern unsigned char DAT_00291734[];   /* bumpercars_a: the RESPAWNCHAR debris type */
extern float DAT_002ea478[4];          /* classlib_ui: a zero vector */

/* 0x0013c8e0  (gap): variation 1 (Prop Rotation) per frame. The two propellers turn (the
platforms' angular speed is set for one platform update), the players are kept between
radius 285 and 822; a propeller that hits a player (not jumping over it) takes a quarter of
his score and makes him dizzy (state 13) for 180 frames. */
void HopAntics_Update1(void *self)
{
	float dist, angle;
	void *player;
	void *actor;
	void *cd;
	int *contact;
	int n, i;
	int onBlade1, onBlade2, hit;
	int old, now;
	void *next;
	unsigned int idx;

	(void)self;
	if (hop_RoundOver != 0)
		return;
	HOP_AF(hop_Platforms[0], 0x1d8) = HOP_F(0x002ae950) * HOP_F(0x002af08c) * HOP_DEG;
	HOP_AF(hop_Platforms[1], 0x1d8) = HOP_F(0x002ae950) * HOP_F(0x002af08c) * HOP_DEG;
	HOP_AF(hop_Platforms[2], 0x1d8) = HOP_F(0x002ae954) * HOP_F(0x002af08c) * HOP_DEG;
	HOP_AF(hop_Platforms[3], 0x1d8) = HOP_F(0x002ae954) * HOP_F(0x002af08c) * HOP_DEG;
	Platforms_UpdateAll();
	HOP_AF(hop_Platforms[0], 0x1d8) = 0.0f;
	HOP_AF(hop_Platforms[1], 0x1d8) = 0.0f;
	HOP_AF(hop_Platforms[2], 0x1d8) = 0.0f;
	HOP_AF(hop_Platforms[3], 0x1d8) = 0.0f;
	for (i = 0; i < DAT_002bae94; i++) {
		onBlade1 = 0;
		if (hop_HitTimer[i] > 0)
			hop_HitTimer[i]--;
		actor = hop_PlayerActor(i);
		dist = sqrtf(HOP_AF(actor, 0xd0) * HOP_AF(actor, 0xd0) + HOP_AF(actor, 0xd8) * HOP_AF(actor, 0xd8));
		if (dist > 822.0f || dist < 285.0f) {
			angle = atan2f(HOP_AF(actor, 0xd0), HOP_AF(actor, 0xd8));
			dist = dist > 822.0f ? 822.0f : 285.0f;
			HOP_AF(actor, 0xd0) = sinf(angle) * dist;
			HOP_AF(hop_PlayerActor(i), 0xd8) = cosf(angle) * dist;
			hop_ActorSnap(hop_PlayerActor(i));
		}
		actor = hop_PlayerActor(i);
		hit = 0;
		if (HopArena_InBlades(HOP_ADDR(0x002aec10), HOP_AF(actor, 0xd0), HOP_AF(actor, 0xd8),
			HOP_AS(hop_Players[i], 0x36)) != 0) {
			player = hop_Players[i];
			cd = HOP_AP(HOP_AP(player, 0x30), 0x18c);
			n = HOP_AI(cd, 0x1d0);
			if (n != 0) {
				onBlade2 = 0;
				contact = (int *)((char *)cd + 0x1bc);
				for (; n > 0; n--, contact++) {
					if ((void *)(uintptr_t)*contact == hop_Platforms[0])
						onBlade1 = 1;
					if ((void *)(uintptr_t)*contact == hop_Platforms[1])
						onBlade1 = 1;
					if ((void *)(uintptr_t)*contact == hop_Platforms[2])
						onBlade2 = 1;
					if ((void *)(uintptr_t)*contact == hop_Platforms[3])
						onBlade2 = 1;
					if (onBlade1 && HOP_AS(player, 0x36) != 3 && HOP_AS(player, 0x36) != 4 &&
						HOP_AF(HOP_AP(player, 0x30), 0xd4) < -134.0f)
						hit = 1;
					if (onBlade2 && HOP_AS(player, 0x36) != 0xe)
						hit = 1;
				}
			}
		}
		if (hop_RoundOver == 0 && hit && HOP_AS(hop_Players[i], 0x36) != 0xd && hop_HitTimer[i] == 0) {
			AI_SetState(hop_Players[i], 0xd);
			hop_HitTimer[i] = 0xb4;
			old = hop_Score[i];
			now = HOP_FTOL((double)old * 0.75);
			hop_Score[i] = now;
			CLSamples_PlayFromSet((char)DAT_002baed4[HOP_AUS(hop_PlayerActor(i), 0x230)][0], 2, NULL, 0xb4, 0,
				-1, 0);
			if (old - now > 0)
				hop_FloatScore(i, old - now);
		}
		if (hop_HitTimer[i] != 0 && HOP_AS(hop_Players[i], 0x36) == 0xd)
			HOP_AF(HOP_AP(hop_PlayerActor(i), 0x18c), 0xa0) = 5.0f;
		else
			HOP_AF(HOP_AP(hop_PlayerActor(i), 0x18c), 0xa0) = 60.0f;
		if (DAT_002baed4[i][1] == 0) {
			HOP_AI(hop_Players[i], 0x28) = 1;
			hop_AIThink(hop_Players[i]);
		}
	}
	AI_TickAll();
	/* each actor updates; a player who was hit (or is falling through, state 14) passes
	   through the blades (platform flag 0x20) */
	for (actor = hop_NextActor(NULL); actor != NULL; actor = hop_NextActor(next)) {
		idx = HOP_AUS(actor, 0x230);
		if (hop_HitTimer[idx] != 0) {
			HOP_AU(hop_Platforms[0], 0x230) |= 0x20;
			HOP_AU(hop_Platforms[1], 0x230) |= 0x20;
		}
		if (hop_HitTimer[idx] != 0 || HOP_AS(hop_Players[idx], 0x36) == 0xe) {
			HOP_AU(hop_Platforms[2], 0x230) |= 0x20;
			HOP_AU(hop_Platforms[3], 0x230) |= 0x20;
			Platforms_UpdateAll();
		}
		next = Actor_Update(actor);
		idx = HOP_AUS(actor, 0x230);
		if (hop_HitTimer[idx] != 0 || HOP_AS(hop_Players[idx], 0x36) == 0xe) {
			HOP_AU(hop_Platforms[0], 0x230) &= ~0x20u;
			HOP_AU(hop_Platforms[1], 0x230) &= ~0x20u;
			HOP_AU(hop_Platforms[2], 0x230) &= ~0x20u;
			HOP_AU(hop_Platforms[3], 0x230) &= ~0x20u;
			Platforms_UpdateAll();
		}
	}
	RespawnPoints_UpdateAll();
	Actor_UpdateAnimations();
}

/* 0x0013ce90  (gap): variation 2 (Centrifugal Farce) per frame. The 27 platforms of the
disc turn (three rings at their own speed); a platform that is not fully there (alpha at
0x002aecb0) drops 4000 down. A player on the platform that is breaking (0x002aed64, 99:
none) gets an effect; a player who falls below -100 screams, below -2000 he comes back at
the centre, loses a quarter of his score, and the score box flashes. */
void HopAntics_Update2(void *self)
{
	void *actor;
	void *cd;
	float pos[4];
	float a;
	int i, k;
	int old, now;

	for (k = 0; k < 27; k++) {
		if (k < 8)
			HOP_AU(hop_Platforms[k], 0x1d8) = 0x3c64c388u;    /* 0.8 degrees a frame */
		else if (k < 0x13)
			HOP_AU(hop_Platforms[k], 0x1d8) = 0xbc482b17u;    /* -0.7 */
		else if (k < 0x1a)
			HOP_AU(hop_Platforms[k], 0x1d8) = 0x3c2b92a6u;    /* 0.6 */
		if (HOP_IV(0x002aecb0)[k] == 0x100)
			HOP_AF(hop_Platforms[k], 0xe4) = 0.0f;
		else
			HOP_AF(hop_Platforms[k], 0xe4) = -4000.0f;
	}
	Platforms_UpdateAll();
	for (i = 0; i < DAT_002bae94; i++) {
		HOP_AI(hop_Players[i], 0x116c) = 0;
		if (-100.0f <= HOP_AF(hop_PlayerActor(i), 0xd4) || hop_Fell[i] != 0) {
			if (HOP_I(0x002aed64) != 99) {
				for (k = 0; k < HOP_AI(HOP_AP(hop_PlayerActor(i), 0x18c), 0x1d0); k++) {
					cd = HOP_AP(hop_PlayerActor(i), 0x18c);
					if (HOP_AP(cd, 0x1bc + k * 4) == hop_Platforms[HOP_I(0x002aed64)]) {
						HopFx_PlayerEffect(HOP_ADDR(0x002affa0), i, 4);
						HOP_AI(hop_Players[i], 0x116c) = 1;
					}
				}
			}
		} else {
			CLSamples_PlayFromSet((char)DAT_002baed4[i][0], 0xd, NULL, (hop_rand() & 0x7f) + 0x7f, 0, 0, 0);
			hop_Fell[i] = 1;
		}
		if (HOP_AF(hop_PlayerActor(i), 0xd4) <= -2000.0f) {
			hop_Fell[i] = 0;
			a = ((float)i * 90.0f + 45.0f) * HOP_DEG;
			HOP_AF(hop_PlayerActor(i), 0xd0) = sinf(a) * 100.0f;
			HOP_AF(hop_PlayerActor(i), 0xd8) = cosf(a) * 100.0f;
			HOP_AF(hop_PlayerActor(i), 0xd4) = 0.0f;
			HopAntics_PlaceActor(self, hop_PlayerActor(i));
			old = hop_Score[i];
			now = HOP_FTOL((double)old * 0.75);
			hop_Score[i] = now;
			if (old - now > 0)
				hop_FloatScore(i, old - now);
			actor = hop_PlayerActor(i);
			pos[0] = HOP_AF(actor, 0xd0);
			pos[1] = HOP_AF(actor, 0xd4);
			pos[2] = HOP_AF(actor, 0xd8);
			pos[3] = 1.0f;
			DAT_00290e8c = DAT_002baed4[i][0];
			Debris_Add((struct DebrisType *)DAT_00291734, pos, DAT_002ea478, 0, 0xffffffffu);
			hop_Frozen[i] = 0x30;
			HOP_AF((void *)(uintptr_t)DAT_002ecfac, 0xb8 + i * 4) = 180.0f;
		}
		if (hop_Frozen[i] > 0)
			hop_Frozen[i]--;
		if (DAT_002baed4[i][1] == 0) {
			HOP_AI(hop_Players[i], 0x28) = 1;
			hop_AIThink(hop_Players[i]);
		}
	}
	AI_TickAll();
	Actor_UpdateAll();
	RespawnPoints_UpdateAll();
	Actor_UpdateAnimations();
}

/* 0x0013d500  (gap): variation 3 (Careless Torque) per frame. Three blades turn; a player
who falls below -2000 respawns on the ring. A player hit by a blade (or past z -65, in
play) loses a life: a burst of sparks, a sound, he is knocked down (state 10) and flashes
for 60 frames. With no lives left he goes down (state 13) and is put aside after 120
frames; the commentator speaks when two or more players are still in. */
void HopAntics_Update3(void *self)
{
	void *player;
	void *actor;
	float a;
	int i, k, t, res, count;
	unsigned int rnd;

	HOP_AF(hop_Platforms[0], 0x1d8) = HOP_F(0x002af138) * HOP_DEG;
	HOP_AF(hop_Platforms[1], 0x1d8) = HOP_F(0x002af140) * HOP_DEG;
	HOP_AF(hop_Platforms[2], 0x1d8) = HOP_F(0x002af130) * HOP_DEG;
	Platforms_UpdateAll();
	HOP_I(0x002aff84) = 0;
	if (DAT_002bae94 <= 0)
		goto done;
	for (i = 0; i < DAT_002bae94; i++) {
		if (HOP_AF(hop_PlayerActor(i), 0xd4) <= -2000.0f) {
			player = hop_Players[i];
			HOP_VFN(player, 0x24, void (*)(void *))(player);
			a = ((float)i * 10.0f + 180.0f) * HOP_DEG;
			HOP_AF(hop_PlayerActor(i), 0xd0) = sinf(a) * 800.0f;
			HOP_AF(hop_PlayerActor(i), 0xd4) = -269.0f;
			HOP_AF(hop_PlayerActor(i), 0xd8) = cosf(a) * 800.0f;
		}
		actor = hop_PlayerActor(i);
		res = HopArena_InBlades3(HOP_ADDR(0x002aec10), HOP_AF(actor, 0xd0), HOP_AF(actor, 0xd8),
			HOP_AS(hop_Players[i], 0x36));
		if ((HOP_AF(hop_PlayerActor(i), 0xd8) > -65.0f || res == 1) && hop_Play == 5 && hop_HitTimer[i] == 0) {
			hop_Score[i]--;
			for (k = 0x10; k != 0; k--) {
				hop_rand();
				hop_rand();
				rnd = (unsigned int)hop_rand();
				actor = hop_PlayerActor(i);
				baAddExtraItem(HOP_G(0x002b0008, struct BExtraSystem *),
					(double)((float)(rnd & 0x3f) + HOP_AF(actor, 0xd0) - 32.0f),
					(double)((float)(rnd & 0x7f) + HOP_AF(actor, 0xd4) + 32.0f),
					(double)((float)(rnd & 0x3f) + HOP_AF(actor, 0xd8) - 32.0f));
			}
			bsPlaySample(HOP_G(0x002ae9bc, struct BSample *), 0xf0, 0, 0, 0);
			if (hop_Score[i] == 0 && hop_Order[i] == 3) {
				hop_Order[i] = HOP_I(0x002aff88);
				HOP_I(0x002aff84) = 1;
			}
			hop_HitTimer[i] = 0x3c;
			AI_SetState(hop_Players[i], 10);
			HopFx_PlayerEffect(HOP_ADDR(0x002affa0), i, 1);
		}
		t = hop_HitTimer[i];
		if (t != 0) {
			player = hop_Players[i];
			if (hop_Score[HOP_AUS(HOP_AP(player, 0x30), 0x230)] < 1) {
				if (t < 0x78) {
					hop_HitTimer[i] = t + 1;
					if (HOP_AS(player, 0x36) != 0xd) {
						AI_SetState(player, 0xd);
						CLSamples_PlayFromSet((char)DAT_002baed4[i][0], 3, NULL, 0xb4, 0, -1, 0);
						count = 0;
						for (k = 0; k < 4; k++) {
							if (hop_Score[HOP_AUS(hop_PlayerActor(k), 0x230)] > 0)
								count++;
						}
						if (count < 2) {
							bkPrintf("didn't player with mancout=%d\n", count);
						} else {
							CLSamples_PlayCommentary(0x37, DAT_002baed4[i][0], 0xff, 0, 0, NULL);
							bkPrintf("played it! for player %d  with mancount=%d\n", i, count);
						}
					}
				} else {
					actor = HOP_AP(player, 0x30);
					HOP_AF(actor, 0xd4) = 200.0f;
					HOP_AF(hop_PlayerActor(i), 0xd0) = 0.0f;
					HOP_AF(hop_PlayerActor(i), 0xd8) = 1000.0f;
					HOP_AF(hop_PlayerActor(i), 0xdc) = 1.0f;
					hop_ActorSnap(hop_PlayerActor(i));
				}
			} else {
				if (t > 0)
					hop_HitTimer[i] = t - 1;
				if (hop_HitTimer[i] > 0x32 && (hop_HitTimer[i] & 2) != 0)
					baSetRenderPass((struct BActorInstance *)((char *)HOP_AP(player, 0x30) + 0x10), 0, 7,
						(int)(uintptr_t)((char *)self + 0x74));
				else
					baSetRenderPass((struct BActorInstance *)((char *)HOP_AP(player, 0x30) + 0x10), 0, 1, 0);
			}
		}
		if (DAT_002baed4[i][1] == 0)
			hop_AIThink(hop_Players[i]);
	}
	if (HOP_I(0x002aff84) == 1) {
		HOP_I(0x002aff88)++;
		HOP_I(0x002aff84) = 0;
	}
done:
	AI_TickAll();
	Actor_UpdateAll();
	Actor_UpdateAnimations();
}

/* 0x0013d9f0  (gap): variation 4 (Twisted System) per frame. The players ride the turning
arm at their place (+0x54) and are steered along it; a player knocked down (state 13)
spins off the arm and sinks. The heights of the players are eased (a quarter a frame)
towards where the update put them. */
void HopAntics_Update4(void *self)
{
	float ys[4];
	float dir[4];
	float rot[4];
	float a, r;
	void *player;
	void *actor;
	int *order = (int *)((char *)self + 0x54);
	int i;

	/* the original reads the heights of all four players; unset ones keep theirs here */
	for (i = 0; i < 4; i++)
		ys[i] = hop_Players[i] != NULL ? HOP_AF(hop_PlayerActor(i), 0xd4) : 0.0f;
	for (i = 0; i < DAT_002bae94; i++) {
		player = hop_Players[i];
		if (HOP_AF(player, 0x1180) < HOP_AF(player, 0x1184))
			HOP_AF(player, 0x1180) = HOP_AF(player, 0x1180) + 2.0f;
		if (hop_Out[i] == 0) {
			if (hop_RoundOver == 0) {
				HOP_AU(hop_PlayerActor(i), 0x220) |= 1;
				a = (hop_ArenaAngle - (125.0f - HOP_AF(hop_Players[i], 0x1180))) * HOP_DEG;
				HOP_AF(hop_PlayerActor(i), 0x1d8) = 1.5707964f + a;
				r = (float)order[i] * 140.0f + 800.0f;
				HOP_AF(hop_PlayerActor(i), 0xd0) = -(r * sinf(a));
				HOP_AF(hop_PlayerActor(i), 0xd8) = -(r * cosf(a));
				dir[0] = 1.0f;
				dir[1] = 0.0f;
				dir[2] = 0.0f;
				bmVectorRotateY(rot, dir, (HOP_AF(hop_Players[i], 0x1180) + 125.0f) * HOP_DEG);
				bmVectorSetLength(dir, rot, 4095.0f);
				HOP_AI(HOP_AP(hop_Players[i], 0x64), 0x14) = -HOP_FTOL(dir[0]);
				HOP_AI(HOP_AP(hop_Players[i], 0x60), 0x14) = HOP_FTOL(dir[2]);
				if (DAT_002baed4[i][1] == 0) {
					HOP_AI(HOP_AP(hop_Players[i], 0x68), 0x14) = 0;
					HOP_AI(HOP_AP(hop_Players[i], 0x6c), 0x14) = 0;
				}
			}
		} else {
			HOP_AU(hop_PlayerActor(i), 0x220) &= ~1u;
			a = ((hop_ArenaAngle - (125.0f - HOP_AF(hop_Players[i], 0x1180))) + (float)(hop_Spin[i] / 2)) * HOP_DEG;
			r = (float)order[i] * 140.0f + (float)(hop_Spin[i] * 3) + 800.0f;
			HOP_AF(hop_PlayerActor(i), 0xd0) = -(r * sinf(a));
			HOP_AF(hop_PlayerActor(i), 0xd4) = -hop_Sink;
			HOP_AF(hop_PlayerActor(i), 0xd8) = -(r * cosf(a));
			if (DAT_002ecdec == 0)
				hop_Spin[i]++;
		}
		hop_ActorSnap(hop_PlayerActor(i));
		if (HOP_AS(hop_Players[i], 0x36) == 0xd)
			hop_Out[i] = 1;
		ys[i] = HOP_AF(hop_PlayerActor(i), 0xd4);
	}
	if (DAT_002ecdec == 0)
		AI_TickAll();
	Actor_UpdateAll();
	if (DAT_002ecdec == 0)
		Actor_UpdateAnimations();
	if (hop_RoundOver == 0 && hop_Play != 0) {
		for (i = 0; i < 4; i++) {
			actor = hop_PlayerActor(i);
			HOP_AF(actor, 0xd4) = (ys[i] - HOP_AF(actor, 0xd4)) * 0.25f + HOP_AF(actor, 0xd4);
		}
	}
}

/* 0x0013de00  (gap): the update of the variation */
void HopAntics_Update(void *self)
{
	switch (hop_Variation) {
	case 1:
		HopAntics_Update1(self);
		return;
	case 2:
		HopAntics_Update2(self);
		return;
	case 3:
		HopAntics_Update3(self);
		return;
	case 4:
		HopAntics_Update4(self);
		return;
	default:
		return;
	}
}

/* the colour of player i's ring: its character colour at `alpha` */
static unsigned int hop_RingColour(int player, int alpha)
{
	unsigned char *colour = DAT_00290718 + DAT_002baed4[player][0] * 0x60;

	return HOP_ARGB(alpha, colour[0], colour[1], colour[2]);
}

/* 0x0013de40  (gap): variation 4: the player's ring on the arm. pos gets the player's place
on the arm (x, z) and the arm's height there; the ring lies on the ground found below,
its normal eased towards the ground's (fixed while the play has not started). */
void HopAntics_DrawRing4(int player, float *pos)
{
	float start[4];
	float delta[4];
	float hit[4];
	float normal[4];
	float a, r;
	float *ring;
	unsigned int argb;

	if (hop_RoundOver != 0)
		return;
	a = (hop_ArenaAngle - (125.0f - HOP_AF(hop_Players[player], 0x1180))) * HOP_DEG;
	r = (float)HOP_IV(0x002ae8cc)[player] * 140.0f + 800.0f;
	pos[0] = -(r * sinf(a));
	pos[2] = -(r * cosf(a));
	if (player >= 0 && player <= 3)
		pos[1] = (220.0f - hop_Sink) - HOP_AF(hop_Players[player], 0x1180) * 3.1f;
	start[2] = pos[2];
	start[0] = pos[0];
	start[3] = pos[3];
	start[1] = pos[1] + 1000.0f;
	delta[0] = 0.0f * 1.4f;
	delta[1] = ((pos[1] - 2000.0f) - start[1]) * 1.4f;
	delta[2] = 1.4f * 0.0f;
	delta[3] = start[3];
	if (hop_RayCastNormal(start, delta, hit, normal) == 0.0f)
		return;
	if (hop_Play != 5) {
		normal[0] = -0.076026f;
		normal[1] = 0.990509f;
		normal[2] = -0.114506f;
		normal[3] = 1.0f;
	}
	ring = (float *)HOP_ADDR(0x002ae87c) + player * 4;
	if (HOP_AU(ring, 0) == 0xbf800000u) {
		ring[0] = normal[0];
		ring[1] = normal[1];
		ring[2] = normal[2];
		ring[3] = normal[3];
	} else {
		ring[0] = (normal[0] - ring[0]) * 0.055555556f + ring[0];
		ring[1] = (normal[1] - ring[1]) * 0.055555556f + ring[1];
		ring[2] = (normal[2] - ring[2]) * 0.055555556f + ring[2];
	}
	CLHud_BuildGroundQuad((struct CLQuadVertex *)HOP_ADDR(0x003025e8), pos, ring, 96.0f,
		HOP_F(0x002af158) * HOP_DEG, 80.0f);
	argb = hop_RingColour(player, 255);
	HOP_U(0x00302600) = argb;
	HOP_U(0x00302624) = argb;
	HOP_U(0x00302648) = argb;
	HOP_U(0x0030266c) = argb;
	bdSetTexture(0, HOP_G(0x002aea3c, struct bdTexture *));
	bdSetWorldIdentity();
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(4, 0, 0);
	bdDrawPrimitiveFVF(4, HOP_ADDR(0x003025e8), 4, 0x152);
	bdPopRenderState(4);
	bdPopRenderState(5);
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(3);
}

/* 0x0013e1a0  (gap): variation 3: the player's ring on the ground below pos (half
transparent) */
void HopAntics_DrawRing3(int player, float *pos)
{
	float start[4];
	float delta[4];
	float hit[4];
	float normal[4];
	unsigned int argb;

	start[2] = pos[2];
	start[0] = pos[0];
	start[3] = pos[3];
	start[1] = pos[1] + 100.0f;
	delta[0] = 0.0f * 1.4f;
	delta[1] = ((pos[1] - 2000.0f) - start[1]) * 1.4f;
	delta[2] = 1.4f * 0.0f;
	delta[3] = start[3];
	if (hop_RayCastNormal(start, delta, hit, normal) == 0.0f)
		return;
	hit[1] = hit[1] + 15.0f;
	CLHud_BuildGroundQuad((struct CLQuadVertex *)HOP_ADDR(0x00302678), hit, normal, 96.0f, 0.0f, 80.0f);
	argb = hop_RingColour(player, 127);
	HOP_U(0x00302690) = argb;
	HOP_U(0x003026b4) = argb;
	HOP_U(0x003026d8) = argb;
	HOP_U(0x003026fc) = argb;
	bdSetTexture(0, HOP_G(0x002aea3c, struct bdTexture *));
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(7, 2, 0);
	bdDrawPrimitive(4, HOP_ADDR(0x00302678), 4);
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(3);
	bdPopRenderState(5);
	bdPopRenderState(7);
}

/* 0x0013e360  (gap): variation 4: the shadow-receiving steps of the arm's spiral: 8 grey
quads from radius 500 to 1400, 16 degrees and 52.5 high apart */
void HopAntics_DrawSpiral(void *self)
{
	float *quad = (float *)HOP_ADDR(0x00302708);
	float angle = hop_ArenaAngle;
	float low = 0.0f;
	float high = 52.5f;
	float s, t, sinS, cosS, sinT, cosT;
	unsigned int grey;
	int i;

	(void)self;
	bdPushRenderState(5, 0, 0);
	Shadows_BeginReceive();
	bdSetTexture(0, NULL);
	for (i = 8; i != 0; i--) {
		s = (angle + 120.0f) * HOP_DEG;
		sinS = sinf(s);
		grey = HOP_ARGB(127, 127, 127, 127);
		HOP_U(0x00302720) = grey;
		quad[0] = 500.0f * sinS;
		quad[1] = low - hop_Sink;
		cosS = cosf(s);
		quad[2] = cosS * 500.0f;
		t = (angle + 104.0f) * HOP_DEG;
		sinT = sinf(t);
		quad[9] = sinT * 500.0f;
		quad[10] = high - hop_Sink;
		cosT = cosf(t);
		quad[11] = cosT * 500.0f;
		quad[0x12] = sinS * 1400.0f;
		quad[0x14] = cosS * 1400.0f;
		quad[0x1b] = sinT * 1400.0f;
		quad[0x1d] = cosT * 1400.0f;
		HOP_U(0x00302744) = grey;
		quad[0x13] = quad[1];
		HOP_U(0x00302768) = grey;
		quad[0x1c] = quad[10];
		HOP_U(0x0030278c) = grey;
		bdDrawPrimitiveFVF(4, quad, 4, 0x152);
		angle = angle - 16.0f;
		low = low + 52.5f;
		high = high + 52.5f;
	}
	Shadows_EndReceive();
	bdPopRenderState(5);
}

/* 0x0013e510  (gap): draws the players. Variation 2 first draws the players standing at
height 0 squashed (scale y -1, a reflection in the stencilled floor, dark) when the floor
reflection (0x002ae964) is on. Each player is drawn (flashing white while hit), or as a
skeleton when 0x002ae97c is 0, then its ring for the variation. Variation 3 hides the
players who are out at the end; variation 4 hides the player 0x003027cc while 0x003027c8
is set. The level's lights are off while the players are drawn. */
void HopAntics_Draw(void *self)
{
	float ring[4];
	float light[4];
	void *actor;
	void *player;
	unsigned int idx;
	int hidden = 0;
	int count;
	int k;

	Actor_PrepAll();
	if (hop_Variation == 4) {
		light[1] = -hop_Sink;
		light[0] = 0.0f;
		light[2] = 0.0f;
		light[3] = 0.0f;
		Shadows_SetLightPos(light);
	}
	for (k = 0; k < 7; k++) {
		if (hop_Lights[k] != NULL)
			baEnableLight(hop_Lights[k], 0);
	}
	baSetAmbientLight(0x7f, 0x7f, 0x7f);
	if (hop_Variation == 2 && HOP_F(0x002ae964) != 0.0f) {
		D3DDevice_SetRenderState(D3DRS_STENCILENABLE, 1);
		D3DDevice_SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		D3DDevice_SetRenderState(D3DRS_STENCILMASK, 1);
		bdSetRenderState(7, 1, 0);
		bdSetRenderState(6, 1, 0);
		baSetBrightness(0x30);
		for (actor = hop_NextActor(NULL); actor != NULL; actor = hop_NextActor(actor)) {
			if (HOP_AI(actor, 0x21c) == 1 && HOP_AF(actor, 0xd4) == 0.0f) {
				bdSetWorldIdentity();
				HOP_AF(actor, 0xe4) = -1.0f;
				baPrepActorInstance((struct BActorInstance *)((char *)actor + 0x10), 1);
				if (HOP_F(0x002ae97c) != 0.0f)
					baDrawActorInstance((struct BActorInstance *)((char *)actor + 0x10), 7);
			}
		}
		baSetBrightness(0x100);
		bdSetRenderState(7, 2, 0);
		D3DDevice_SetRenderState(D3DRS_STENCILENABLE, 0);
		bdSetRenderState(6, 0, 0);
	}
	ring[3] = 1.0f;
	for (actor = hop_NextActor(NULL); actor != NULL; actor = hop_NextActor(actor)) {
		idx = HOP_AUS(actor, 0x230);
		if (HOP_AI(actor, 0x21c) == 1) {
			if (hop_Variation == 3 && hop_RoundOver != 0 && HOP_IV(0x002aff24)[idx] < 1) {
				hidden = 1;
				DAT_00293eb8[idx] = 0;
			} else {
				hidden = 0;
				DAT_00293eb8[idx] = 1;
			}
			count = DAT_00293eb8[0] == 1;
			if (DAT_00293eb8[1] == 1)
				count++;
			if (DAT_00293eb8[2] == 1)
				count++;
			if (DAT_00293eb8[3] == 1)
				count++;
			if (count == 0)
				DAT_00293eb8[0] = 1;
		}
		if (hop_Variation == 4 && HOP_I(0x003027c8) != 0 && (unsigned int)HOP_I(0x003027cc) == idx)
			hidden = 1;
		if (HOP_AI(actor, 0x21c) != 1 || hidden)
			continue;
		if (DAT_002ecdec == 0 && HOP_IV(0x002aff14)[idx] != 0 && hop_RoundOver == 0 &&
			(HOP_B(0x003027b4) & 2) != 0) {
			bdSetRenderState(6, 1, 0);
			baSetBrightness(0x40);
		}
		bdSetWorldIdentity();
		HOP_AF(actor, 0xe4) = 1.0f;
		baPrepActorInstance((struct BActorInstance *)((char *)actor + 0x10), 1);
		bdSetRenderState(8, 0, 0);
		if (HOP_F(0x002ae97c) == 0.0f) {
			bdSetTexture(0, NULL);
			bdSetWorldIdentity();
			HopAntics_DrawSkeleton((struct BActorInstance *)((char *)actor + 0x10),
				(struct BActorNode *)HOP_AP(actor, 0x144), 0xff, 0xff, 0xff, 0xff);
		} else {
			baDrawActorInstance((struct BActorInstance *)((char *)actor + 0x10), 7);
		}
		if (HOP_IV(0x002aff14)[idx] != 0 && (HOP_B(0x003027b4) & 2) != 0) {
			bdSetRenderState(6, 0, 0);
			baSetBrightness(0x100);
		}
		player = hop_Players[idx];
		if (hop_Variation == 4) {
			if (HOP_AI(actor, 0x214) == 0) {
				if (DAT_002ecdec == 0 && hop_RoundOver == 0)
					HOP_AF(player, 0x1170) = HOP_F(0x002af160) + HOP_AF(player, 0x1170);
			} else {
				HOP_AF(player, 0x1170) = HOP_AF(actor, 0xd4);
			}
			if (HOP_IV(0x002aff54)[idx] == 0) {
				ring[0] = HOP_AF(actor, 0xd0);
				ring[1] = HOP_AF(hop_Players[idx], 0x1170) + 10.0f;
				ring[2] = HOP_AF(actor, 0xd8);
			}
		} else {
			ring[0] = HOP_AF(actor, 0xd0);
			ring[1] = HOP_AF(actor, 0xd4);
			ring[2] = HOP_AF(actor, 0xd8);
		}
		if (hop_Variation == 3)
			ring[1] = ring[1] + 10.0f;
		if (HOP_IV(0x002aff54)[idx] == 0 && (hop_Variation != 2 || 0.0f <= ring[1])) {
			bdSetWorldIdentity();
			switch (hop_Variation) {
			case 1:
				HopAntics_DrawRing1(idx, ring);
				break;
			case 2:
				ring[1] = 0.0f;
				ring[3] = 1.0f;
				CLHud_DrawAIRing(idx, ring, NULL);
				break;
			case 3:
				HopAntics_DrawRing3(idx, ring);
				break;
			case 4:
				HopAntics_DrawRing4(idx, ring);
				break;
			}
			bdSetWorldIdentity();
		}
		bdSetRenderState(8, 1, 0);
	}
	if (hop_Variation == 4)
		HopAntics_DrawSpiral(self);
	for (k = 0; k < 7; k++) {
		if (hop_Lights[k] != NULL)
			baEnableLight(hop_Lights[k], 1);
	}
	if (hop_Variation != 4)
		baSetAmbientLight(0x7f, 0x7f, 0x7f);
	else
		baSetAmbientLight(0x40, 0x40, 0x40);
}

/* 0x0013ea80  (gap): deletes the overlays, the players (their input maps, then the AI
objects) and every actor */
void HopAntics_Shutdown(void *self)
{
	void *player;
	int i;

	(void)self;
	COverlay_DeleteAll();
	for (i = 0; i < DAT_002bae94; i++) {
		if (HOP_AP(hop_Players[i], 0x5c) != NULL) {
			biDeleteMap((struct biMap *)HOP_AP(hop_Players[i], 0x5c));
			player = hop_Players[i];
			if (player != NULL) {
				HopPlayer_Dtor(player);
				HOP_DELETE(player);
			}
		}
	}
	RespawnPoints_DeleteAll();
	Actor_DeleteAll();
}
