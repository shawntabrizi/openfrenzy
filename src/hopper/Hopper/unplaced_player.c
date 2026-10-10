/*
Hopper/unplaced_player.c  <-  gap functions 0x00145cb0-0x001472c5 (no source path in the XBE)
(module hopper)

The Hopper player: a CharLib AI player (0x119c bytes, vtable 0x0026ee50; the base class is
charlib's AI, 0x1130 bytes, see src/charlib/charlib_ai.h). Its state handlers and
transitions are the tables 0x002b04a8 / 0x002b046c (the handlers are in meta_a,
src/meta_a/Meta/unplaced_hopper.c), the priorities 0x002b0388.

Fields added to the AI player:
  +0x1086 short  the last player it hit;  +0x1088 short  the last player that hit it
  +0x108a short[4]  hits taken from each player
  +0x1092 short  1 while it can attack
  +0x10fc float  attack cone (angle distance), +0x1100 float  attack height window
  +0x1130 float[4] variation 3: a jump target   +0x1140 float[4] variation 2: where it runs
  +0x1150, +0x1154 (variation 1 chase state), +0x1158 (index * 8), +0x115c (variation 3:
  frames to step sideways, signed), +0x1160, +0x1164, +0x1168 (variation 2: the nearest
  safe point), +0x116c (variation 2: on the breaking platform), +0x1170 (variation 4: the
  ring's height), +0x1174, +0x1178 (think counter), +0x117c,
  +0x1180, +0x1184 float (variation 4: how far along the arm, and the goal)
  +0x1198 1: the low-detail ("opt") model is in use
*/

#include "hopper.h"

#define PL_ACTOR(self) HOP_AP(self, 0x30)
#define PL_STATE(self) HOP_AS(self, 0x36)
#define PL_INDEX(self) HOP_AUS(PL_ACTOR(self), 0x230)
#define PL_BUTTON(self, off) HOP_AI(HOP_AP(self, off), 0x14)   /* 0x60 forward, 0x64 left/right, 0x68 jump, 0x6c action */

extern int DAT_00283ea4;               /* core: the character colour set */
extern void *DAT_002f26c4;             /* bumpercars_b: the debug free camera */
extern int DAT_002baebc;               /* core: team game */
extern unsigned char DAT_00292ca0[];   /* charlib: the character animation names (0x44 bytes each) */

/* the character table entry of a character: {model name, model, low-detail name, its model} */
#define hop_CharModel(c) HOP_G(0x002ae81c + (c) * 0x10, void *)
#define hop_CharOptModel(c) HOP_G(0x002ae824 + (c) * 0x10, void *)
/* character stats (classlib_ui, 0x60 bytes per character) */
#define hop_CharStat(c, off) HOP_AT(DAT_00290718 + (c) * 0x60, off, float)

/* the safe points of variation 2 (the effects object +0x30: an extra system) */
#define hop_SafePoints HOP_G(0x002affd0, void *)

/* 0x00145cb0  (gap): constructs player `index` (an AI player) at pos: its character's model,
the state tables, the animations (47 named sub-objects), the think interval (the
character's, slower on lower difficulty) */
void *HopPlayer_Ctor(void *self, int index, float *pos)
{
	void *actor;
	int k, off;

	AI_Ctor(self);
	*(const void *const **)self = vtable_0026ee50;
	actor = Actor_Create(hop_CharModel(DAT_002baed4[index][0]), pos, NULL, 1, 0, 1);
	HOP_AP(self, 0x30) = actor;
	HOP_AU(actor, 0x220) = 0x59;
	HOP_AS(PL_ACTOR(self), 0x230) = (short)index;
	if (DAT_00283ea4 != 0)
		Core_SetCharacterColours((char *)PL_ACTOR(self) + 0x10, DAT_00283ea4);
	HOP_AS(self, 0x98) = 1;
	HOP_AI(self, 0x1178) = 0;
	HOP_AI(self, 0x117c) = 0;
	HOP_AS(self, 0x34) = 0xf;
	HOP_AS(self, 0x36) = 0;
	HOP_AI(self, 0x4) = 0;
	HOP_AI(self, 0x8) = 0;
	HOP_AI(self, 0x1174) = 0;
	HOP_AI(self, 0x1154) = 0;
	HOP_AI(self, 0x1158) = index * 8;
	HOP_AI(self, 0x1180) = 0;
	HOP_AI(self, 0x1184) = 0;
	HOP_AI(self, 0x115c) = 0;
	HOP_AI(self, 0x1164) = 0;
	HOP_AI(self, 0x1160) = 0;
	HOP_AI(self, 0x1150) = 0;
	HOP_AP(self, 0x1094) = HOP_ADDR(0x002b04a8);   /* state handlers */
	HOP_AP(self, 0x1098) = HOP_ADDR(0x002b046c);   /* transitions */
	HOP_AP(self, 0x109c) = HOP_ADDR(0x002b0388);   /* priorities */
	Actor_InitCharData(PL_ACTOR(self));
	HOP_AP(PL_ACTOR(self), 0x324) = DAT_00292ca0;
	for (k = 0, off = 0x264; off < 800; k++, off += 4) {
		actor = PL_ACTOR(self);
		HOP_AP(actor, off) = baFindSubObject((struct BActorInstance *)((char *)actor + 0x10),
			(const char *)HOP_AP(actor, 0x324) + k * 0x44);
		bkPrintf("anim seg %d ", k);
		actor = PL_ACTOR(self);
		if (HOP_AP(actor, off) == NULL)
			bkPrintf("FAILED - called %s\n", (const char *)HOP_AP(actor, 0x324) + k * 0x44);
		else
			bkPrintf("OK - called %s\n", (const char *)HOP_AP(actor, 0x324) + k * 0x44);
	}
	HOP_AI(self, 0x10b4) = HOP_AT(DAT_00290718 + DAT_002baed4[index][0] * 0x60, 0x38, int) / (DAT_002bae70 + 1);
	return self;
}

/* 0x00145eb0  (gap): the player's destructor */
void HopPlayer_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026ee50;
	AI_Dtor(self);
}

/* 0x00145ec0  (gap): the ground height under pos (the CharLib probe, truncated), or 1 while
the player stands on the breaking platform (variation 2) */
float HopPlayer_GroundHeight(void *self, float *pos, float radius, int unused3, int unused4)
{
	int h;

	(void)unused3;
	(void)unused4;
	h = HOP_FTOL(CLColl_GroundProbe(pos, radius, NULL, NULL, 0));
	if (HOP_AI(hop_Players[PL_INDEX(self)], 0x116c) != 0)
		h = 1;
	return (float)h;
}

/* 0x00145f10  (gap): a - b in degrees, wrapped to -180..180 */
float HopPlayer_AngleDiff(float a, float b)
{
	float d = a - b;

	if (360.0f < d)
		d = d - 720.0f;
	if (d < -360.0f)
		d = d + 720.0f;
	if (-180.0f < d && d <= 180.0f)
		return d;
	if (180.0f < d)
		return d - 360.0f;
	return d + 360.0f;
}

/* 0x00145f80  (gap): vtable slot 6 (set the mode): clears the plan */
void HopPlayer_ResetMode(void *self)
{
	HOP_AI(self, 0x14) = 0;
	HOP_AI(self, 0x20) = 0;
	HOP_AI(self, 0x24) = -1;
	HOP_AI(self, 0x28) = 1;
	HOP_AUS(self, 0x98) = (unsigned short)(hop_Variation == 3);
}

/* 0x00145fb0  (gap): the players' input: a map of 4 channels each (Forward, leftRight, jump,
action; variation 1 has no repeat on action) bound to the player's pad */
void HopPlayers_Start(void)
{
	void *player;
	unsigned int pad;
	int i;

	for (i = 0; i < DAT_002bae94; i++) {
		pad = (unsigned int)DAT_002baed4[i][3];
		HOP_AP(hop_Players[i], 0x5c) = biCreateMap(4);
		player = hop_Players[i];
		HOP_AP(player, 0x60) = biCreateChannel((struct biMap *)HOP_AP(player, 0x5c), "Forward", 3, 0, 0, 0, 0);
		HOP_AP(hop_Players[i], 0x64) = biCreateChannel((struct biMap *)HOP_AP(hop_Players[i], 0x5c), "leftRight", 3, 0,
			0, 0, 0);
		HOP_AP(hop_Players[i], 0x68) = biCreateChannel((struct biMap *)HOP_AP(hop_Players[i], 0x5c), "jump", 1, 1, 0, 0,
			0);
		HOP_AP(hop_Players[i], 0x6c) = biCreateChannel((struct biMap *)HOP_AP(hop_Players[i], 0x5c), "action", 1,
			hop_Variation != 1, 0, 0, 0);
		biBindInput((struct biChannel *)HOP_AP(hop_Players[i], 0x60), pad | 0xbe, pad | 0xbd);
		biBindInput((struct biChannel *)HOP_AP(hop_Players[i], 0x64), pad | 0xc0, pad | 0xbf);
		biBindInput((struct biChannel *)HOP_AP(hop_Players[i], 0x60), pad | 0xba, 0);
		biBindInput((struct biChannel *)HOP_AP(hop_Players[i], 0x64), pad | 0xb9, 0);
		biBindInput((struct biChannel *)HOP_AP(hop_Players[i], 0x68), pad | 0xc5, 0);
		biBindInput((struct biChannel *)HOP_AP(hop_Players[i], 0x6c), pad | 199, 0);
		biBindInput((struct biChannel *)HOP_AP(hop_Players[i], 0x6c), pad | 0xc6, 0);
	}
}

/* 0x00146180  (gap): vtable slot 16: switches the player between its character's model
(opt 0) and the low-detail model (opt 1), keeping the position, animation (anim, or with
-1 the AI state and its frames) and colours. Not with the debug free camera. */
void HopPlayer_SetDetail(void *self, unsigned int opt, int anim)
{
	float pos[4];
	void *actor;
	void *model;
	void *owner;
	int state = 0, frames = 0;
	int c;

	if (DAT_002f26c4 != NULL || opt == HOP_AU(self, 0x1198))
		return;
	HOP_AU(self, 0x1198) = opt;
	if (anim == -1) {
		owner = hop_Players[PL_INDEX(self)];
		state = HOP_AUS(owner, 0x36);
		frames = HOP_AI(owner, 0x3c);
	}
	actor = PL_ACTOR(self);
	c = DAT_002baed4[HOP_AUS(actor, 0x230)][0];
	model = opt == 0 ? hop_CharModel(c) : hop_CharOptModel(c);
	if (model != NULL) {
		pos[0] = HOP_AF(actor, 0xd0);
		pos[1] = HOP_AF(actor, 0xd4);
		pos[2] = HOP_AF(actor, 0xd8);
		pos[3] = HOP_AF(actor, 0xdc);
		baDeleteActorInstance((struct BActorInstance *)((char *)actor + 0x10));
		Actor_Create(model, pos, PL_ACTOR(self), 1, 0, 0);
		if (opt != 0) {
			c = DAT_002baed4[PL_INDEX(self)][0];
			if (hop_CharModel(c) != NULL)
				baSetInstanceAnimations((struct BActor *)hop_CharModel(c),
					(struct BActorInstance *)((char *)PL_ACTOR(self) + 0x10));
		}
		if (anim != -1)
			Actor_PlayAnim(PL_ACTOR(self), anim, 0, 0, 1.0f, 0.0f, 0);
		actor = PL_ACTOR(self);
		baUpdateAnimations((struct BActorInstance *)((char *)actor + 0x10), (struct BActorNode *)HOP_AP(actor, 0x144),
			0.0f, NULL);
		baPrepActorInstance((struct BActorInstance *)((char *)PL_ACTOR(self) + 0x10), 1);
		Core_SetCharacterColours((char *)PL_ACTOR(self) + 0x10, DAT_00283ea4);
	}
	if (anim == -1) {
		AI_SetState(hop_Players[PL_INDEX(self)], state);
		HOP_AI(hop_Players[PL_INDEX(self)], 0x3c) = frames;
	}
}

/* 0x00146320  (gap): variation 1 (and 2 on the hardest settings): chase the other players.
Within 200 the player turns to the other, presses action and attacks (state 5) unless
the other is down (13, 14); within 1000 it plans a route to him (not in variation 3 or
when `mode` is 2). */
void HopPlayer_Chase(void *self, int mode)
{
	float to[4];
	float dx, dz, dist;
	void *me;
	void *other;
	unsigned int k;

	for (k = 0; k < 4; k++) {
		me = PL_ACTOR(self);
		if (k == HOP_AUS(me, 0x230))
			continue;
		if (DAT_002baebc != 0 && DAT_002baed4[k][8] == DAT_002baed4[HOP_AUS(me, 0x230)][8])
			continue;
		other = hop_PlayerActor(k);
		dx = HOP_AF(me, 0xd0) - HOP_AF(other, 0xd0);
		dz = HOP_AF(me, 0xd8) - HOP_AF(hop_PlayerActor(k), 0xd8);
		dist = sqrtf(dx * dx + dz * dz);
		if (1000.0f < dist)
			HOP_AI(self, 0x1154) = 0;
		if (HOP_AI(self, 0x1154) >= 2)
			continue;
		if (dist < 200.0f) {
			PL_BUTTON(self, 0x6c) = 1;
			HOP_AF(PL_ACTOR(self), 0x1d8) = (atan2f(dx, dz) + 180.0f) * HOP_DEG;
			if (HOP_AS(hop_Players[k], 0x36) != 0xd && HOP_AS(hop_Players[k], 0x36) != 0xe) {
				AI_SetState(self, 5);
				HOP_AI(self, 0x1154) = 2;
			}
		}
		if (HOP_AI(self, 0x1154) == 0 && mode != 2 && dist < 1000.0f && hop_Variation != 3) {
			HOP_AI(self, 0x1154) = 1;
			to[0] = HOP_AF(hop_PlayerActor(k), 0xd0);
			to[1] = 0.0f;
			to[2] = HOP_AF(hop_PlayerActor(k), 0xd8);
			to[3] = 1.0f;
			CLRoute_Start((struct CLRoute *)((char *)self + 0x8c), HOP_AV(PL_ACTOR(self), 0xd0), to,
				HOP_AF(PL_ACTOR(self), 0xd4));
		}
	}
}

/* the nearest of the effects' safe points (variation 2) to (x, z): sets +0x1168, returns
the point */
static float *hop_NearestSafePoint(void *self, float x, float z)
{
	void *points = hop_SafePoints;
	float *p;
	float d, best = 0.0f;
	int k;

	for (k = 0; k < HOP_AI(points, 0x18); k++) {
		p = HOP_AT(HOP_AP(points, 0x38), k * 4, float *);
		d = sqrtf((p[0] - x) * (p[0] - x) + (p[2] - z) * (p[2] - z));
		if (k == 0 || d < best) {
			HOP_AI(self, 0x1168) = k;
			points = hop_SafePoints;
			best = d;
		}
	}
	return HOP_AT(HOP_AP(points, 0x38), HOP_AI(self, 0x1168) * 4, float *);
}

/* 0x001464e0  (gap): variation 1's AI: jump the first propeller when it comes (on the
ground), attack over the second; otherwise walk towards the nearest safe point (pausing
in a rhythm on the easier settings). On difficulty 0..1 some reactions are skipped. */
void HopPlayer_Think1(void *self)
{
	float dir[4];
	float x, z;
	float *p;
	int which, slow = 0, n;

	which = (int)HopArena_WhichBlade1(HOP_ADDR(0x002aec10), HOP_AF(PL_ACTOR(self), 0xd0),
		HOP_AF(PL_ACTOR(self), 0xd8));
	if (-1 < DAT_002bae70 && DAT_002bae70 < 2 &&
		((HOP_AI(self, 0x1178) + (unsigned int)HOP_AB(PL_ACTOR(self), 0x230) * 8) & 0x10) != 0)
		slow = 1;
	if (which == 1) {
		if (PL_BUTTON(self, 0x68) == 0 && HOP_AI(PL_ACTOR(self), 0x214) != 0) {
			if ((hop_rand() & 3) == 0 && slow)
				return;
			PL_BUTTON(self, 0x68) = 1;
			return;
		}
	} else if (which == 2) {
		if ((hop_rand() & 3) == 0 && slow)
			return;
		PL_BUTTON(self, 0x6c) = 1;
		return;
	}
	x = HOP_AF(PL_ACTOR(self), 0xd0);
	z = HOP_AF(PL_ACTOR(self), 0xd8);
	if ((hop_rand() & 0x7f) == 0)
		PL_BUTTON(self, 0x68) = 1;
	p = hop_NearestSafePoint(self, x, z);
	dir[0] = p[0] - HOP_AF(PL_ACTOR(self), 0xd0);
	dir[2] = p[2] - HOP_AF(PL_ACTOR(self), 0xd8);
	if (slow)
		hop_rand();
	n = HOP_AI(self, 0x1178) + 1;
	HOP_AI(self, 0x1178) = n;
	if (hop_TimeLeft > 400) {
		if (DAT_002bae70 == 0 || DAT_002bae70 == 1) {
			if ((signed char)((char)HOP_AS(PL_ACTOR(self), 0x230) * ' ' + (char)n) < 0)
				return;
		} else if (DAT_002bae70 == 2 && (((unsigned int)HOP_AB(PL_ACTOR(self), 0x230) * 0x10 + n) & 0x40) != 0) {
			return;
		}
	}
	dir[1] = 0.0f;
	AI_SetStick(self, dir, 1.0f);
}

/* 0x00146750  (gap): variation 2's AI: chase now and then (difficulty 1: 1 in 8, 2: 1 in 4);
when not over solid ground walk back towards the centre, else run 50 towards the nearest
safe point unless that is over a hole (pausing in a rhythm on the easier settings) */
void HopPlayer_Think2(void *self)
{
	float dir[4];
	float x, z, dx, dz, a;
	float *p;
	void *actor;
	int ground, n, r;

	if (DAT_002bae70 == 1) {
		r = hop_rand() & 7;
		if (r == 0)
			HopPlayer_Chase(self, 2);
	} else if (DAT_002bae70 == 2) {
		r = hop_rand() & 3;
		if (r == 0)
			HopPlayer_Chase(self, 2);
	}
	ground = HOP_FTOL(CLColl_GroundProbe(HOP_AV(PL_ACTOR(self), 0xd0), 0.0f, NULL, NULL, 0));
	actor = PL_ACTOR(self);
	if (HOP_AI(hop_Players[HOP_AUS(actor, 0x230)], 0x116c) != 0)
		ground = 1;
	if ((float)ground == 0.0f) {
		x = HOP_AF(actor, 0xd0);
		z = HOP_AF(actor, 0xd8);
		p = hop_NearestSafePoint(self, x, z);
		HOP_AF(self, 0x1140) = p[0];
		HOP_AF(self, 0x1144) = HOP_AF(actor, 0xd4);
		HOP_AF(self, 0x1148) = p[2];
		HOP_AF(self, 0x114c) = 1.0f;
		dx = HOP_AF(self, 0x1140) - HOP_AF(actor, 0xd0);
		n = HOP_AI(self, 0x1178) + 1;
		dz = HOP_AF(self, 0x1148) - HOP_AF(actor, 0xd8);
		a = atan2f(dx, dz);
		HOP_AF(self, 0x1140) = sinf(a) * 50.0f + HOP_AF(actor, 0xd0);
		HOP_AI(self, 0x1178) = n;
		HOP_AF(self, 0x1148) = cosf(a) * 50.0f + HOP_AF(actor, 0xd8);
		if (DAT_002bae70 == 0) {
			if ((signed char)((char)HOP_AS(actor, 0x230) * ' ' + (char)n) < 0)
				return;
		} else if (DAT_002bae70 == 1 && (((unsigned int)HOP_AB(actor, 0x230) * 0x10 + n) & 0x40) != 0) {
			return;
		}
		if (HopPlayer_GroundHeight(self, HOP_AV(self, 0x1140), 0.0f, 0, 0) == 0.0f) {
			dir[1] = 0.0f;
			dir[0] = dx;
			dir[2] = dz;
			AI_SetStick(self, dir, 1.0f);
		}
	} else {
		x = HOP_AF(actor, 0xd0);
		z = HOP_AF(actor, 0xd8);
		a = atan2f(-x, -z);
		HOP_AF(self, 0x1140) = sinf(a) * 50.0f + HOP_AF(actor, 0xd0);
		HOP_AF(self, 0x1144) = HOP_AF(actor, 0xd4);
		HOP_AF(self, 0x114c) = 1.0f;
		HOP_AF(self, 0x1148) = cosf(a) * 50.0f + HOP_AF(actor, 0xd8);
		if (CLColl_GroundProbe(HOP_AV(self, 0x1140), 0.0f, NULL, NULL, 0) == 0.0f) {
			dir[1] = 0.0f;
			dir[0] = -x;
			dir[2] = -z;
			AI_SetStick(self, dir, 1.0f);
		}
	}
}

/* 0x00146a50  (gap): variation 3's AI. Under a blade (by WhichBlade3; the easier settings
sometimes ignore it): in the inner ring it picks a jump target 3 * (300..427) away; else
(not jumping or knocked down) it may jump (blade 1; humans alive or a random chance by
difficulty) and steps sideways away from the blade's turn for 8 frames. Out of danger it
keeps facing the core: when its angle to it is beyond +-20 degrees it starts stepping
back. Steps steer the stick at a right angle to the centre; with no step it attacks
another player within 150 (not on difficulty 0). */
void HopPlayer_Think3(void *self)
{
	float dir[4];
	float out[4];
	float x, z, dist, angle, d, a;
	void *me;
	void *other;
	int which, zone, ring, humans, k, mask;
	unsigned int r;

	z = HOP_AF(PL_ACTOR(self), 0xd8);
	x = HOP_AF(PL_ACTOR(self), 0xd0);
	dist = sqrtf(z * z + x * x);
	HOP_I(0x00302b58) = HOP_I(0x00302b58) + 1;
	humans = 0;
	angle = atan2f(x, z);
	which = HopArena_WhichBlade3(HOP_ADDR(0x002aec10), x, z);
	if (which != 0 && DAT_002bae70 == 0) {
		r = (unsigned int)hop_rand();
		if ((r & 0xf) == 0)
			which = 0;
	}
	if (1000.0f < HOP_AF(PL_ACTOR(self), 0xd4) && HOP_AU(PL_ACTOR(self), 0xd4) != 0x45fa0000u)
		bkPrintf("man has jumped too high!\n");
	if (DAT_002bae70 == 0) {
		if ((hop_rand() % 5) == 0)
			which = 0;
	} else if (DAT_002bae70 == 1) {
		if ((hop_rand() & 7) == 0)
			which = 0;
	}
	if (dist <= 1000.0f) {
		if (dist <= 470.0f) {
			zone = 3;
			ring = 0;
		} else {
			zone = 2;
			ring = 3;
		}
	} else {
		zone = 1;
		ring = 1;
	}
	for (k = 0; k < 4; k++) {
		if (0 < HOP_IV(0x002aff24)[HOP_AUS(hop_PlayerActor(k), 0x230)] && DAT_002baed4[k][1] != 0)
			humans++;
	}
	if (0 < HOP_AI(self, 0x1160))
		HOP_AI(self, 0x1160)--;
	if (which == 0) {
		if (HOP_AI(self, 0x115c) == 0) {
			d = HopPlayer_AngleDiff(180.0f, angle * 57.29578f);
			if (d < -20.0f) {
				r = (unsigned int)hop_rand();
				HOP_AI(self, 0x115c) = -8 - (int)(r & 0xf);
			} else if (20.0f < d) {
				r = (unsigned int)hop_rand();
				HOP_AI(self, 0x115c) = (int)(r & 0xf) + 8;
			}
		}
	} else {
		if (zone == 3) {
			HOP_AI(self, 0x1164) = 3;
			r = (unsigned int)hop_rand();
			HOP_AF(self, 0x1134) = 0.0f;
			HOP_AF(self, 0x113c) = 1.0f;
			d = (float)HOP_AI(self, 0x1164) * ((float)(r & 0x7f) + 300.0f);
			HOP_AF(self, 0x1130) = sinf(angle) * d;
			HOP_AF(self, 0x1138) = cosf(angle) * d;
			return;
		}
		if (PL_STATE(self) != 3 && PL_STATE(self) != 10) {
			if (DAT_002bae70 == 0)
				mask = 1;
			else if (DAT_002bae70 == 1)
				mask = 7;
			else if (DAT_002bae70 == 2)
				mask = 0xf;
			else
				memcpy(&mask, &dist, sizeof mask);    /* (an unrelated stack word in the original) */
			if (which == 1 && PL_BUTTON(self, 0x68) != 1 && (humans != 0 || (Core_Rand() & (unsigned int)mask) != 0))
				PL_BUTTON(self, 0x68) = 1;
			if (HOP_FV(0x002af130)[ring] <= 0.0f)
				HOP_AI(self, 0x115c) = 8;
			else
				HOP_AI(self, 0x115c) = -8;
		}
	}
	if (HOP_AI(self, 0x115c) != 0 && (humans != 0 || (hop_rand() & 7) != 0)) {
		k = HOP_AI(self, 0x115c);
		if (k < 1) {
			a = angle - 1.5707964f;
			k++;
		} else {
			a = angle + 1.5707964f;
			k--;
		}
		HOP_AI(self, 0x115c) = k;
		dir[1] = 0.0f;
		dir[0] = sinf(a) * 10.0f;
		dir[2] = cosf(a) * 10.0f;
		bmVectorSetLength(out, dir, 32767.0f);
		PL_BUTTON(self, 0x60) = HOP_FTOL(out[2] * -4.0f);
		PL_BUTTON(self, 0x64) = HOP_FTOL(out[0] * 4.0f);
		return;
	}
	if (PL_BUTTON(self, 0x6c) != 0 || DAT_002bae70 == 0)
		return;
	for (k = 0; k < 4; k++) {
		me = PL_ACTOR(self);
		if ((unsigned int)k == HOP_AUS(me, 0x230))
			continue;
		other = hop_PlayerActor(k);
		x = HOP_AF(me, 0xd0) - HOP_AF(other, 0xd0);
		z = HOP_AF(me, 0xd8) - HOP_AF(other, 0xd8);
		if (sqrtf(x * x + z * z) < 150.0f) {
			HOP_AF(me, 0x1d8) = atan2f(x, z) + 3.1415927f;
			PL_BUTTON(self, 0x6c) = 1;
		}
	}
}

/* 0x00146f90  (gap): vtable slot 7: the variation's AI (variation 4 has none). Returns 0. */
int HopPlayer_Think(void *self)
{
	switch (hop_Variation) {
	case 1:
		HopPlayer_Think1(self);
		return 0;
	case 2:
		HopPlayer_Think2(self);
		return 0;
	case 3:
		HopPlayer_Think3(self);
		return 0;
	}
	return 0;
}

/* 0x00146fd0  (gap): vtable slot 4: the attack. Another player (not on the own team, not
knocked down (10), at about the same height (+0x1100)) in front (within +0x10fc) and near
enough (the character's reach, closer for wider angles) is pushed away by the character's
strength, a hit sound plays, and he is knocked out (13; variation 2 only after three hits
by the same player) or down (10, variation 3). Returns 1 when someone was hit. */
int HopPlayer_Attack(void *self)
{
	float push[4];
	void *me;
	void *other;
	float h, d;
	short *hits;
	unsigned int k;
	int check, c;

	HOP_AS(self, 0x1092) = 1;
	if (hop_RoundOver != 0)
		return 0;
	for (k = 0; (int)k < DAT_002bae94; k++) {
		if (DAT_002baebc != 0 && DAT_002baed4[k][8] == DAT_002baed4[PL_INDEX(self)][8])
			continue;
		other = hop_PlayerActor(k);
		if (hop_Variation == 2)
			check = HOP_IV(0x002afef4)[k] < 1;
		else
			check = HOP_AI(other, 0x214) != 0;
		if (!check)
			continue;
		me = PL_ACTOR(self);
		if (k == HOP_AUS(me, 0x230) || HOP_AS(hop_Players[k], 0x36) == 10 ||
			!(fabsf(HOP_AF(me, 0xd4) - HOP_AF(other, 0xd4)) < HOP_AF(self, 0x1100)))
			continue;
		h = CLMath_HeadingTo(HOP_AV(me, 0xd0), HOP_AV(other, 0xd0));
		d = CLMath_AngleDistance(h, HOP_AF(PL_ACTOR(self), 0x1e8));
		if (!(d < HOP_AF(self, 0x10fc)))
			continue;
		c = DAT_002baed4[PL_INDEX(self)][0];
		if (!((d + 1.0f) * bmVectorDistance(HOP_AV(PL_ACTOR(self), 0xd0), HOP_AV(hop_PlayerActor(k), 0xd0)) <
			hop_CharStat(c, 0x10) * 0.5f))
			continue;
		me = PL_ACTOR(self);
		Core_Vec3Sub(push, HOP_AV(hop_PlayerActor(k), 0xd0), HOP_AV(me, 0xd0));
		bmVectorSetLength(push, push, hop_CharStat(DAT_002baed4[HOP_AUS(me, 0x230)][0], 0x14) * 3.0f);
		push[1] = 0.0f;
		cmb_vec3_add(HOP_AV(hop_PlayerActor(k), 0x194), HOP_AV(hop_PlayerActor(k), 0x194), push);
		HOP_AS(self, 0x1086) = (short)k;
		HOP_AS(hop_Players[k], 0x1088) = (short)HOP_AUS(PL_ACTOR(self), 0x230);
		hits = &HOP_AS(hop_Players[k], 0x108a + HOP_AUS(PL_ACTOR(self), 0x230) * 2);
		*hits = (short)(*hits + 1);
		CLSamples_PlayGeneric((int)(Core_Rand() % 4u) + 0xb, 0x7f, 0, 0, NULL);
		HopFx_Rumble(HOP_ADDR(0x002affa0), (int)k, 2);
		if (hop_Variation != 3 &&
			(hop_Variation != 2 || 2 < HOP_AS(hop_Players[k], 0x108a + HOP_AUS(PL_ACTOR(self), 0x230) * 2))) {
			AI_SetState(hop_Players[k], 0xd);
			return 1;
		}
		AI_SetState(hop_Players[k], 10);
		return 1;
	}
	return 0;
}

/* 0x0026ee50  the Hopper player's vtable (charlib's AI virtuals; see charlib_ai.h) */
const void *const vtable_0026ee50[18] = {
	(const void *)FUN_001d5950, /* [0] +0x00 before the AI update (empty) */
	(const void *)FUN_001d5950, /* [1] +0x04 after the AI update (empty) */
	(const void *)FUN_001d5950, /* [2] +0x08 paused (empty) */
	(const void *)FUN_000b1860, /* [3] +0x0c think (AI_Think) */
	(const void *)FUN_00146fd0, /* [4] +0x10 attack */
	(const void *)FUN_001d5950, /* [5] +0x14 act (empty) */
	(const void *)FUN_00145f80, /* [6] +0x18 set the mode */
	(const void *)FUN_00146f90, /* [7] +0x1c the variation's AI */
	(const void *)FUN_000b0560, /* [8] +0x20 AI_RespawnAt */
	(const void *)FUN_000b0370, /* [9] +0x24 AI_RespawnFarthest */
	(const void *)FUN_000b0cb0, /* [10] +0x28 AI_Spawn */
	(const void *)FUN_001d5950, /* [11] +0x2c (empty) */
	(const void *)FUN_000b06c0, /* [12] +0x30 AI_CanMove */
	(const void *)FUN_000a6fa0, /* [13] +0x34 (returns 1) */
	(const void *)FUN_001ec260, /* [14] +0x38 a timer ran out (empty) */
	(const void *)FUN_001ec260, /* [15] +0x3c (empty) */
	(const void *)FUN_00146180, /* [16] +0x40 the model's detail */
	(const void *)FUN_001ec260, /* [17] +0x44 (empty) */
};
