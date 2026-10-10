/*
FoxHunt/unplaced_level.c  (module foxhunt; gap functions 0x00136ef0-0x00138790 of
C:\BlitzParty\Game\Src\FoxHunt\, file unknown)

The level objects of Fox Hunt (the flag update, breakable glass, escalators and the gates
of the energy ball variation, kept in the lists of the .bss), the text reader of the level scripts (foxhunt.cfg: the same
reader as bumpercars_b's, vtable 0x0026e048), and the CWaterField constructor.
*/

#include "foxhunt.h"

extern const void *const vtable_0026e05c[];   /* classlib_fx: CWaterField */

extern char *DAT_002ba96c;            /* core: the game (+0x18 stage) */
extern float DAT_00292c84;            /* core: gravity */

#define STAGE() FH_AI(DAT_002ba96c, 0x18)
#define FLAGS() FH_LIST(0x0030230c)
#define GLASS() FH_LIST(0x00302318)
#define ESCALATORS() FH_LIST(0x00302324)
#define GATES() FH_LIST(0x00302330)

/* the player `i` through the original address arithmetic (i may be -1: the original then
reads the .bss word before the player table) */
#define PLAYER_AT(i) FH_P(0x00302210u + (unsigned int)(i) * 4u)

/* rand() / 32767 as the x87 code compares it with a double constant */
static inline double fh_randd(void)
{
	return (double)fh_rand() * (double)3.05185094e-05f;
}

/* the colour of the player record `rec` (0xRRGGBB) */
static unsigned int player_colour(int rec)
{
	unsigned char *c = &DAT_00290718[DAT_002baed4[rec * 10] * 0x60];

	return ((unsigned int)c[0] << 16) | ((unsigned int)c[1] << 8) | c[2];
}

/* copies the 4 words of the actor position into `to` */
static void copy_pos(float *to, const float *from)
{
	to[0] = from[0];
	to[1] = from[1];
	to[2] = from[2];
	to[3] = from[3];
}

/* ======================================================================== the flags */

/* 0x00136ef0  (gap; follows CFHunt_Flag.cpp): CFHunt_Flag::Update: `target` is the ray
target. Follows the carrier's right palm, handles a drop (state 2) and a score (state 3)
of the carrier, colours the debris pieces, falls and lands when free, respawns below
y -3000 or above 4010, is picked up by a near player (state 0) and, in Victim (stage 4),
smokes (`smoke` 0) or sparks (`smoke` 1). Returns the list node (the flag). */
char *CFHunt_Flag_Update(void *self, float *target, int smoke)
{
	char *player;
	char *a;
	float v[4];
	float step;
	float fall;
	float probe;
	float best;
	float d;
	float dx;
	float dz;
	unsigned int colour;
	int count;
	int i;
	int slot;
	int pick;
	int kind;

	FH_AF(self, 0x44) = target[0];
	FH_AF(self, 0x48) = target[1];
	FH_AF(self, 0x4c) = target[2];
	FH_AF(self, 0x50) = target[3];
	step = 0.0f;
	if (FH_AI(self, 0x14) == 1) {
		/* carried: no other player may carry this flag */
		count = DAT_002bae94;
		for (i = 0; i < count; i++) {
			player = fh_Players[i];
			if (FH_AI(player, 0x11c4) == 1 && FH_AI(player, 0x11c8) == FH_AI(self, 0xc) &&
			    FH_AI(self, 0x18) != i) {
				FH_AI(player, 0x11c4) = 0;
				FH_AI(fh_Players[i], 0x11c8) = -1;
				count = DAT_002bae94;
			}
		}
		for (i = 0; i < DAT_002bae94; i++) {
			player = fh_Players[i];
			if (FH_AI(player, 0x11c4) == 1 && FH_AI(player, 0x11c8) == FH_AI(self, 0xc)) {
				struct BActorNode *node = baFindNode((struct BActorNode *)FH_AP(FH_AP(player, 0x30), 0x144), "rpalm");

				baGetNodePosition((struct BActorInstance *)(FH_AP(fh_Players[i], 0x30) + 0x10), node, v);
				copy_pos(FH_AV(FH_AP(self, 0x8), 0xd0), v);
				if (FH_AS(fh_Players[i], 0x36) == 0xd)
					FH_AF(FH_AP(self, 0x8), 0xd4) = FH_AF(FH_AP(self, 0x8), 0xd4) + 100.0f;
			} else if (FH_AI(player, 0x11c4) == 2 && FH_AI(player, 0x11c8) == FH_AI(self, 0xc)) {
				/* dropped */
				FH_AI(player, 0x11c4) = 0;
				FH_AI(fh_Players[i], 0x11c8) = -1;
				FH_AI(self, 0x14) = 2;
				FH_AI(self, 0x18) = -1;
				if (STAGE() == 2) {
					FH_AI(fh_Players[i], 0x11dc) = 0;
					FH_AI(fh_Players[i], 0x11e0) = 0;
					FH_AI(fh_Players[i], 0x11e4) = 0;
					FH_AI(fh_Players[i], 0x11e8) = 0;
					FoxHunt_OpenGates();
				}
			} else if (FH_AI(player, 0x11c4) == 3 && FH_AI(player, 0x11c8) == FH_AI(self, 0xc)) {
				/* scored */
				FH_AI(player, 0x11c4) = 0;
				FH_AI(fh_Players[i], 0x11c8) = -1;
				if (STAGE() == 2) {
					for (slot = 0x11dc; slot < 0x11ec; slot += 4) {
						if (FH_AI(fh_Players[i], slot) == 1) {
							FH_AI(fh_Players[i], 0x11d0) += 0x14;
							FH_AI(fh_Players[i], slot) = 0;
						}
						FoxHunt_OpenGates();
					}
					FH_AI(fh_Players[i], 0x11d0) += 10;
				}
				FH_AI(self, 0x14) = 3;
				FH_AI(self, 0x18) = -1;
			}
		}
		if (STAGE() != 4) {
			/* the original reads the colour of record -1 when the carrier just dropped
			or scored; the pieces lose that colour again below, so the port skips it */
			colour = FH_AI(self, 0x18) >= 0 ? player_colour(FH_AI(self, 0x18)) : 0xffffffffu;
			FH_AU(FH_AP(self, 0x24), 0x98) = colour;
			FH_AU(FH_AP(self, 0x28), 0x98) = colour;
			FH_AU(FH_AP(self, 0x2c), 0x98) = colour;
			copy_pos(FH_AV(FH_AP(self, 0x24), 0x4), FH_AV(FH_AP(self, 0x8), 0xd0));
			copy_pos(FH_AV(FH_AP(self, 0x28), 0x4), FH_AV(FH_AP(self, 0x8), 0xd0));
			copy_pos(FH_AV(self, 0x30), FH_AV(FH_AP(self, 0x8), 0xd0));
		}
		copy_pos(FH_AV(FH_AP(self, 0x2c), 0x4), FH_AV(FH_AP(self, 0x8), 0xd0));
		FH_AF(FH_AP(self, 0x2c), 0x8) = FH_AF(FH_AP(self, 0x2c), 0x8) - 60.0f;
		if (STAGE() == 5) {
			player = PLAYER_AT(FH_AI(self, 0x18));
			FH_AF(FH_AP(FH_AP(self, 0x24), 0x74), 0x24) = (float)FH_AI(player, 0x11d0) * 3.0f;
			FH_AF(FH_AP(FH_AP(self, 0x28), 0x74), 0x24) = (float)FH_AI(player, 0x11d0) + 15.0f;
		}
	} else {
		FH_AI(self, 0x18) = -1;
		if (STAGE() != 4) {
			FH_AI(FH_AP(self, 0x24), 0x98) = -1;
			copy_pos(FH_AV(FH_AP(self, 0x24), 0x4), FH_AV(FH_AP(self, 0x8), 0xd0));
			FH_AF(FH_AP(self, 0x24), 0x8) = FH_AF(FH_AP(self, 0x24), 0x8) + 120.0f;
			FH_AI(FH_AP(self, 0x28), 0x98) = -1;
			copy_pos(FH_AV(FH_AP(self, 0x28), 0x4), FH_AV(FH_AP(self, 0x8), 0xd0));
			FH_AF(FH_AP(self, 0x28), 0x8) = FH_AF(FH_AP(self, 0x28), 0x8) + 120.0f;
			copy_pos(FH_AV(self, 0x30), FH_AV(FH_AP(self, 0x8), 0xd0));
			FH_AF(self, 0x34) = FH_AF(self, 0x34) + 120.0f;
		}
		copy_pos(FH_AV(FH_AP(self, 0x2c), 0x4), FH_AV(FH_AP(self, 0x8), 0xd0));
		FH_AF(FH_AP(self, 0x2c), 0x8) = FH_AF(FH_AP(self, 0x2c), 0x8) + 60.0f;
		step = DAT_00292c84 * 0.05f;
		if ((FH_AB(FH_AP(self, 0x8), 0x220) & 1) != 0) {
			/* falling: lands when the fall of this frame reaches the ground */
			probe = CLColl_GroundProbe(FH_AV(FH_AP(self, 0x8), 0xd0), FH_AF(self, 0x20), NULL, NULL, 0);
			a = FH_AP(self, 0x8);
			fall = DAT_0028fda8 * FH_AF(a, 0x198) - DAT_0028fda8 * DAT_0028fda8 * step * 0.5f;
			if (probe <= -fall) {
				FH_AF(a, 0x198) = FH_AF(a, 0x198) - DAT_0028fda8 * step;
				FH_AF(FH_AP(self, 0x8), 0xd4) = FH_AF(FH_AP(self, 0x8), 0xd4) - probe;
				FH_AU(FH_AP(self, 0x8), 0x220) &= 0xfffffffeu;
			} else {
				FH_AF(a, 0x198) = FH_AF(a, 0x198) - DAT_0028fda8 * step;
				FH_AF(FH_AP(self, 0x8), 0xd4) = fall + FH_AF(FH_AP(self, 0x8), 0xd4);
			}
		}
		if ((double)FH_AF(FH_AP(self, 0x8), 0xd4) <= -3000.0 || 4010.0f <= FH_AF(FH_AP(self, 0x8), 0xd4))
			CFHunt_Flag_Respawn(self);
		if (FH_AI(self, 0x14) == 0) {
			for (i = 0; i < DAT_002bae94; i++) {
				player = fh_Players[i];
				if (FH_AI(player, 0x11c4) == 0 && FH_AI(player, 0x11ac) == 1 && FH_AS(player, 0x36) != 0xd &&
				    fh_DistanceSquared(FH_AV(FH_AP(self, 0x8), 0xd0), FH_AV(FH_AP(player, 0x30), 0xd0)) < 9000.0f) {
					FH_AI(fh_Players[i], 0x11c4) = 1;
					FH_AI(fh_Players[i], 0x11c8) = FH_AI(self, 0xc);
					FH_AI(fh_Players[i], 0x11cc) = FH_AI(fh_Players[i], 0x11d4);
					FH_AI(self, 0x14) = 1;
					FH_AI(self, 0x18) = i;
					FHunt_Sound_Play(fh_Speech, 4, 0xff, 0);
					break;
				}
			}
		}
		if (STAGE() == 4)
			FH_AI(self, 0x14) = 4;
	}
	if (FH_AI(self, 0x14) == 2) {
		FH_AI(FH_AP(self, 0x24), 0x98) = -1;
		FH_AI(FH_AP(self, 0x28), 0x98) = -1;
		FH_AI(FH_AP(self, 0x2c), 0x98) = -1;
		FH_AI(self, 0x14) = 0;
		FH_AI(self, 0x18) = -1;
		FH_AI(FH_AP(self, 0x8), 0x220) = 1;
		FH_AF(FH_AP(self, 0x8), 0xd4) = FH_AF(FH_AP(self, 0x8), 0xd4) + 100.0f;
	}
	if (FH_AI(self, 0x14) == 3) {
		FH_AI(FH_AP(self, 0x24), 0x98) = -1;
		FH_AI(FH_AP(self, 0x28), 0x98) = -1;
		FH_AI(FH_AP(self, 0x2c), 0x98) = -1;
		FH_AI(self, 0x14) = 0;
		FH_AI(self, 0x18) = -1;
		FH_AI(FH_AP(self, 0x8), 0x220) = 1;
		CFHunt_Flag_Respawn(self);
	} else if (FH_AI(self, 0x14) == 4) {
		/* Victim: the nearest free player (on the ground plane) takes the flag. Without
		one the original keeps the bits of `step` as the index; the port uses 0. */
		(void)step;
		pick = 0;
		best = 1000000.0f;
		for (i = 0; i < DAT_002bae94; i++) {
			if (FH_AI(fh_Players[i], 0x11ac) != 1)
				continue;
			a = FH_AP(DAT_002f289c[i], 0x30);
			dx = FH_AF(a, 0xd0) - FH_AF(FH_AP(self, 0x8), 0xd0);
			dz = FH_AF(a, 0xd8) - FH_AF(FH_AP(self, 0x8), 0xd8);
			d = sqrtf(dx * dx + dz * dz);
			if (d < best && FH_AI(fh_Players[i], 0x11c4) != 1) {
				pick = i;
				best = d;
			}
		}
		FH_AI(fh_Players[pick], 0x11c4) = 1;
		FH_AI(fh_Players[pick], 0x11c8) = FH_AI(self, 0xc);
		FH_AI(self, 0x14) = 1;
		FH_AI(self, 0x18) = pick;
	}
	if (STAGE() != 4)
		return (char *)self;
	/* Victim: smoke or sparks from the flag */
	v[0] = 0.0f;
	v[2] = 0.0f;
	v[3] = 0.0f;   /* a stack word the original never sets */
	v[1] = ((float)fh_rand() * 6.10370189e-05f + FH_F(0x00302144)) * DAT_0028fda8;
	if (smoke == 0) {
		if (fh_randd() <= 0.8)
			return (char *)self;
		kind = 0x29;
	} else {
		if (smoke != 1)
			return (char *)self;
		if (fh_randd() > 0.6)
			FoxHunt_AddDebris(0x2c, FH_AV(FH_AP(self, 0x8), 0xd0), v, 0, 0xffffffffu);
		v[0] = ((float)fh_rand() * 3.05185094e-05f + 1.8f) * DAT_0028fda8;
		v[1] = ((float)fh_rand() * 3.05185094e-05f + 1.9f) * DAT_0028fda8;
		v[2] = ((float)fh_rand() * 3.05185094e-05f + 1.8f) * DAT_0028fda8;
		if (fh_randd() <= 0.7)
			return (char *)self;
		kind = 0x2a;
	}
	FoxHunt_AddDebris(kind, FH_AV(FH_AP(self, 0x8), 0xd0), v, 0, 0xffffffffu);
	return (char *)self;
}

/* 0x00137940  (gap): updates every flag (`target` is the ray target) */
void FoxHunt_UpdateFlags(float *target, int smoke)
{
	char *node = fh_ListNext(FLAGS(), NULL);

	while (node != NULL)
		node = fh_ListNext(FLAGS(), CFHunt_Flag_Update(node, target, smoke));
}

/* ======================================================================== the glass

The breakable glass (0x18 bytes, in the list 0x00302318 with its node at +0xc):
  +0x00 type (1 or 2)   +0x04 broken   +0x08 hit points (30)   +0x14 the CharLib platform */

/* 0x00137980  (gap): the glass of `type` (model table 0x002ae1f8) at `pos` */
void *FoxHunt_Glass_Ctor(void *self, float *pos, int type)
{
	FH_AP(self, 0x14) = (char *)Platform_Create(FH_PV(0x002ae1f8)[type * 2], pos, 0);
	CLList_Insert((int *)GLASS(), (int *)((char *)self + 0xc), 0);
	FH_AU(FH_AP(self, 0x14), 0x230) |= 0x200;
	FH_AI(self, 0) = type;
	FH_AI(self, 0x4) = 0;
	FH_AI(self, 0x8) = 0x1e;
	return self;
}

/* 0x001379e0  (gap): walks the glass list (does nothing with it) */
void FoxHunt_UpdateGlass(void)
{
	char *node;

	for (node = fh_ListNext(GLASS(), NULL); node != NULL; node = fh_ListNext(GLASS(), node))
		;
}

/* 0x00137a00  (gap): a hit on the glass by something moving by `vel` at `pos`; the third
hit breaks it into shards (the piece table of its type) that fly away from `pos`, opens
the routes over it and smashes the platform. Returns the list node. */
char *FoxHunt_Glass_Hit(void *self, float *vel, float *pos)
{
	float out[4];
	float at[4];
	float d;
	float f;
	char *piece;
	char *end;
	int kind_a;
	int kind_b;
	int r;

	out[0] = vel[0];
	out[2] = vel[2];
	out[3] = vel[3];
	out[1] = vel[1];
	FH_AI(self, 0x8) = FH_AI(self, 0x8) - 10;
	if (FH_AI(self, 0x8) < 0 && FH_AI(self, 0x4) == 0) {
		if (FH_AI(self, 0) == 1) {
			piece = FH_A(0x002ae210);
			end = FH_A(0x002ae300);
			kind_a = 0x26;
			kind_b = 0x24;
		} else if (FH_AI(self, 0) == 2) {
			piece = FH_A(0x002ae300);
			end = FH_A(0x002ae3b4);
			kind_a = 0x27;
			kind_b = 0x25;
		} else {
			piece = end = NULL;
			kind_a = kind_b = 0;
		}
		for (; piece < end; piece += 0xc) {
			at[0] = FH_AF(piece, 0);
			at[1] = FH_AF(piece, 4);
			at[2] = FH_AF(piece, 8);
			at[3] = 0.0f;   /* a stack word the original never sets */
			d = bmVectorDistance(at, pos);
			if (d == 0.0f)
				d = 0.01f;
			r = fh_rand();
			f = 1.0f / (d * 0.02f);
			out[0] = f * vel[0] + f * vel[0];
			out[1] = f * vel[1];
			out[2] = f * vel[2] + f * vel[2];
			FoxHunt_AddDebris(0.5f <= (float)r * 3.05185094e-05f ? kind_a : kind_b, at, out, 0, 0xffffffffu);
		}
		FH_AI(self, 0x4) = 1;
		FoxHunt_SelectRouteHeights(FH_AI(self, 0));
		Platform_Smash(FH_AP(self, 0x14), 1);
		FHunt_Sound_Play(fh_Speech, 0xd, 0xff, 0);
	}
	return (char *)self + 0xc;
}

/* 0x00137c60  (not in the function table; called by the game mode's exit): frees every
glass */
void FUN_00137c60(void)
{
	char *node = fh_ListNext(GLASS(), NULL);
	char *next;

	while (node != NULL) {
		next = fh_ListRemove(GLASS(), node);
		if (node - 0xc != NULL)
			bkHeapFree(node - 0xc);
		node = fh_ListNext(GLASS(), next);
	}
}

/* 0x00137cb0  (gap): a hit on every glass of type `type` (see FoxHunt_Glass_Hit) */
void FoxHunt_HitGlass(int type, float *vel, float *pos)
{
	char *node;

	for (node = fh_ListNext(GLASS(), NULL); node != NULL; node = fh_ListNext(GLASS(), node)) {
		if (FH_AI(node, -0xc) == type)
			node = FoxHunt_Glass_Hit(node - 0xc, vel, pos);
	}
}

/* ======================================================================== the escalators

The escalator (0x20 bytes, in the list 0x00302324 with its node at +0):
  +0x08 the actor   +0x0c direction (0 or 1)   +0x10 1   +0x18 id (1 or 2)
  +0x1c the arrow texture */

/* 0x00137d00  (gap): the escalator `id` (model table 0x002ae43c) at `pos`, running in
`direction` (0 or 1); `user` goes to the actor's +0x1e8 */
void *FoxHunt_Escalator_Ctor(void *self, float *pos, int direction, int user, int id)
{
	char *actor;
	const char *arrow;
	int i;

	actor = (char *)Actor_Create(FH_PV(0x002ae43c)[id * 2], pos, NULL, 4, 0, 1);
	FH_AP(self, 0x8) = actor;
	FH_AI(actor, 0x220) = 0x100;
	CLList_Insert((int *)ESCALATORS(), (int *)self, 0);
	Actor_InitCharData(FH_AP(self, 0x8));
	FH_AP(FH_AP(self, 0x8), 0x324) = FH_A(0x002ae3b8);
	baFlushAnimations(FH_AP(self, 0x8) + 0x10, (struct BActorNode *)FH_AP(FH_AP(self, 0x8), 0x144), 1, 0);
	for (i = 0; i < 2; i++)
		FH_AP(FH_AP(self, 0x8), 0x264 + i * 4) = (char *)baFindSubObject(
			(struct BActorInstance *)(FH_AP(self, 0x8) + 0x10), FH_AP(FH_AP(self, 0x8), 0x324) + i * 0x44);
	FH_AI(FH_AP(self, 0x8), 0x1e8) = user;
	FH_AI(self, 0xc) = direction;
	FH_AI(self, 0x10) = 1;
	FH_AI(self, 0x18) = id;
	if (direction == 0) {
		Actor_PlayAnim(FH_AP(self, 0x8), 0, 1, 0, 2.0f, 0.1f, 1);
		arrow = "textures\\arrow1.gif";
	} else {
		if (direction != 1)
			return self;
		Actor_PlayAnim(FH_AP(self, 0x8), 1, 1, 0, 2.0f, 0.1f, 1);
		arrow = "textures\\arrow2.gif";
	}
	FH_AP(self, 0x1c) = (char *)bLoadResource(NULL, arrow, 0, 0xdefa);
	bdTextureSetFrame((struct bdTexture *)FH_AP(self, 0x1c), 0);
	return self;
}

/* 0x00137e40  (gap): loads the two escalator models */
void FoxHunt_LoadEscalatorModels(void)
{
	char **slot;

	for (slot = FH_PV(0x002ae444); slot < FH_PV(0x002ae454); slot += 2)
		*slot = (char *)bLoadResource(NULL, slot[-1], 1, 8);
}

/* 0x00137e70  (gap): reverses the escalator (animation and arrow frame; escalator 2
shows the other arrow frame). Returns the list node. */
char *FoxHunt_Escalator_Toggle(void *self)
{
	baFlushAnimations(FH_AP(self, 0x8) + 0x10, (struct BActorNode *)FH_AP(FH_AP(self, 0x8), 0x144), 1, 0);
	if (FH_AI(self, 0x18) == 1) {
		if (FH_AI(self, 0xc) == 1) {
			Actor_PlayAnim(FH_AP(self, 0x8), 0, 1, 0, 2.0f, 0.1f, 1);
			bdTextureSetFrame((struct bdTexture *)FH_AP(self, 0x1c), 0);
			FH_AI(self, 0xc) = 0;
			return (char *)self;
		}
		if (FH_AI(self, 0xc) == 0) {
			Actor_PlayAnim(FH_AP(self, 0x8), 1, 1, 0, 2.0f, 0.1f, 1);
			bdTextureSetFrame((struct bdTexture *)FH_AP(self, 0x1c), 1);
			FH_AI(self, 0xc) = 1;
			return (char *)self;
		}
	} else if (FH_AI(self, 0x18) == 2) {
		if (FH_AI(self, 0xc) == 1) {
			Actor_PlayAnim(FH_AP(self, 0x8), 0, 1, 0, 2.0f, 0.1f, 1);
			bdTextureSetFrame((struct bdTexture *)FH_AP(self, 0x1c), 1);
			FH_AI(self, 0xc) = 0;
			return (char *)self;
		}
		if (FH_AI(self, 0xc) == 0) {
			Actor_PlayAnim(FH_AP(self, 0x8), 1, 1, 0, 2.0f, 0.1f, 1);
			bdTextureSetFrame((struct bdTexture *)FH_AP(self, 0x1c), 0);
			FH_AI(self, 0xc) = 1;
		}
	}
	return (char *)self;
}

/* 0x00137f80  (gap): walks the escalator list (does nothing with it) */
void FoxHunt_UpdateEscalators(void)
{
	char *node;

	for (node = fh_ListNext(ESCALATORS(), NULL); node != NULL; node = fh_ListNext(ESCALATORS(), node))
		;
}

/* 0x00137fa0  (gap): the direction of the escalator `id` (0 without one) */
int FoxHunt_EscalatorDirection(int id)
{
	char *node;

	for (node = fh_ListNext(ESCALATORS(), NULL); node != NULL; node = fh_ListNext(ESCALATORS(), node)) {
		if (FH_AI(node, 0x18) == id)
			return FH_AI(node, 0xc);
	}
	return 0;
}

/* 0x00137fe0  (gap): reverses every escalator (the button); the players forget their
escalator plans */
void FoxHunt_ToggleEscalators(void)
{
	char *node;

	FH_AI(fh_Players[0], 0x1148) = 0;
	FH_AI(fh_Players[1], 0x1148) = 0;
	FH_AI(fh_Players[2], 0x1148) = 0;
	FH_AI(fh_Players[3], 0x1148) = 0;
	node = fh_ListNext(ESCALATORS(), NULL);
	while (node != NULL)
		node = fh_ListNext(ESCALATORS(), FoxHunt_Escalator_Toggle(node));
}

/* 0x00138040  (not in the function table; called by the game mode's exit): frees every
escalator */
void FUN_00138040(void)
{
	char *node = fh_ListNext(ESCALATORS(), NULL);
	char *next;

	while (node != NULL) {
		next = fh_ListRemove(ESCALATORS(), node);
		if (node != NULL)
			bkHeapFree(node);
		node = fh_ListNext(ESCALATORS(), next);
	}
}

/* ======================================================================== the gates

The gate of the energy ball variation (0x44 bytes, in the list 0x00302330 with its node at
+0x18):
  +0x00 id (1..3)   +0x04 35.0   +0x08 position   +0x20 the actor   +0x24 90
  +0x28, +0x2c, +0x30 user values   +0x34, +0x38 10   +0x3c open (1)   +0x40 0 */

/* 0x00138090  (gap): loads the three gate models (the original loads the first name for
each of them) */
void FoxHunt_LoadGateModels(void)
{
	char **slot;

	for (slot = FH_PV(0x002ae454); slot < FH_PV(0x002ae46c); slot += 2) {
		*slot = (char *)bLoadResource(NULL, FH_P(0x002ae450), 1, 0);
		if (*slot != NULL)
			bkPrintf("Gate Model Loaded\n");
	}
}

/* 0x001380d0  (gap): the id of the nearest open gate to the player `player` (-1 without
one within 100000) */
int FoxHunt_NearestGate(int player)
{
	char *node;
	float best = 100000.0f;
	float d;
	int id = -1;

	for (node = fh_ListNext(GATES(), NULL); node != NULL; node = fh_ListNext(GATES(), node)) {
		if (FH_AI(node, 0x24) == 1) {
			d = bmVectorDistance(FH_AV(FH_AP(node, 0x8), 0xd0), FH_AV(FH_AP(fh_Players[player], 0x30), 0xd0));
			if (d < best) {
				id = FH_AI(node, -0x18);
				best = d;
			}
		}
	}
	return id;
}

/* 0x00138150  (gap): the gate `id` (model table 0x002ae44c) at `pos` (moved down by 30:
the caller's vector changes too); `unused` is not read */
void *FoxHunt_Gate_Ctor(void *self, float *pos, float unused, int a, int b, int c, int id)
{
	char *actor;
	int i;

	(void)unused;
	pos[1] = pos[1] - 30.0f;
	actor = (char *)Actor_Create(FH_PV(0x002ae44c)[id * 2], pos, NULL, 5, 0, 1);
	FH_AP(self, 0x20) = actor;
	copy_pos(FH_AV(actor, 0xd0), pos);
	CLList_Insert((int *)GATES(), (int *)((char *)self + 0x18), 0);
	FH_AF(FH_AP(self, 0x20), 0xe0) = 0.5f;
	FH_AF(FH_AP(self, 0x20), 0xe4) = 0.5f;
	FH_AF(FH_AP(self, 0x20), 0xe8) = 0.5f;
	FH_AP(FH_AP(self, 0x20), 0x324) = FH_A(0x002ae468);   /* "1-idle", ... */
	for (i = 0; i < 4; i++)
		FH_AP(FH_AP(self, 0x20), 0x264 + i * 4) = (char *)baFindSubObject(
			(struct BActorInstance *)(FH_AP(self, 0x20) + 0x10), FH_AP(FH_AP(self, 0x20), 0x324) + i * 0x44);
	FH_AI(self, 0) = id;
	FH_AF(self, 0x4) = 35.0f;
	copy_pos(FH_AV(self, 0x8), pos);
	FH_AI(self, 0x28) = a;
	FH_AI(self, 0x2c) = c;
	FH_AI(self, 0x24) = 0x5a;
	FH_AI(self, 0x30) = b;
	FH_AI(self, 0x34) = 10;
	FH_AI(self, 0x38) = 10;
	FH_AI(self, 0x3c) = 1;
	FH_AI(self, 0x40) = 0;
	Actor_PlayAnim(FH_AP(self, 0x20), 0, 1, 0, 2.0f, 0.1f, 1);
	return self;
}

/* 0x001382a0  (gap): opens every closed gate (animation 3 and the sub-animation 0) */
void FoxHunt_OpenGates(void)
{
	char *node;
	char *actor;

	for (node = fh_ListNext(GATES(), NULL); node != NULL; node = fh_ListNext(GATES(), node)) {
		if (FH_AI(node, 0x24) == 0) {
			FH_AI(node, 0x24) = 1;
			Actor_PlayAnim(FH_AP(node, 0x8), 3, 1, 0, 2.0f, 0.1f, 1);
			actor = FH_AP(node, 0x8);
			baQueueAnimationBlendIn((struct BActorInstance *)(actor + 0x10), (struct BActorNode *)FH_AP(actor, 0x144),
				(struct BAnim *)FH_AP(actor, 0x264), 0x400, -1, 0.1f, 1, NULL, NULL);
		}
	}
}

/* 0x00138320  (gap): an open gate closes when a player with the flag reaches it for the
first time (a burst in the player's colour, a sound, and a marker that counts the gates
already passed); every carrier near it is marked as through it. A closed gate smokes.
Returns the list node. */
char *FoxHunt_Gate_Update(void *self)
{
	char *player;
	float at[4];
	float v[4];
	unsigned int colour;
	int passed;
	int kind;
	int i;
	int r;

	if (FH_AI(self, 0x3c) == 1) {
		for (i = 0; i < DAT_002bae94; i++) {
			if (FH_AI(fh_Players[i], 0x11c4) != 1)
				continue;
			if (!(fh_DistanceSquared(FH_AV(self, 0x8), FH_AV(FH_AP(fh_Players[i], 0x30), 0xd0)) < 14000.0f))
				continue;
			player = fh_Players[i];
			if (FH_AI(player, 0x11d8 + FH_AI(self, 0) * 4) == 0) {
				v[0] = 0.0f;
				v[1] = 5.0f;
				v[2] = 0.0f;
				v[3] = 0.0f;   /* a stack word the original never sets */
				passed = FH_AI(player, 0x11dc) == 1;
				if (FH_AI(player, 0x11e0) == 1)
					passed++;
				if (FH_AI(player, 0x11e4) == 1)
					passed++;
				at[2] = FH_AF(self, 0x10);
				at[0] = FH_AF(self, 0x8);
				at[1] = FH_AF(self, 0xc) + 100.0f;
				at[3] = FH_AF(self, 0x14);
				colour = player_colour(i);
				FoxHunt_AddDebris(3, at, DAT_002ea478, 0, colour);
				FHunt_Sound_Play(fh_Speech, 6, 0xff, 0);
				Actor_PlayAnim(FH_AP(self, 0x20), 1, 1, 0, 2.0f, 0.1f, 1);
				baQueueAnimationBlendIn((struct BActorInstance *)(FH_AP(self, 0x20) + 0x10),
					(struct BActorNode *)FH_AP(FH_AP(self, 0x20), 0x144), (struct BAnim *)FH_AP(FH_AP(self, 0x20), 0x26c),
					0x400, -1, 0.1f, 1, NULL, NULL);
				kind = passed == 0 ? 0x1c : passed == 1 ? 0x1d : passed == 2 ? 0x1e : 0;
				if (kind != 0)
					FoxHunt_AddDebris(kind, at, v, 0, colour);
				FH_AI(self, 0x3c) = 0;
			}
			FH_AI(fh_Players[i], 0x11d8 + FH_AI(self, 0) * 4) = 1;
		}
	} else if (FH_AI(self, 0x3c) == 0) {
		r = fh_rand();
		v[0] = (float)r * 3.05185094e-05f - 0.5f;
		v[1] = DAT_0028fda8 * 10.125f;
		v[2] = (float)fh_rand() * 3.05185094e-05f - 0.5f;
		v[3] = 0.0f;   /* a stack word the original never sets */
		FoxHunt_AddDebris(0x10, FH_AV(FH_AP(self, 0x20), 0xd0), v, 0, 0x141464);
	}
	return (char *)self + 0x18;
}

/* 0x001385d0  (gap): copies the position of the gate `id` into `out` (the last one with
that id) */
void FoxHunt_GatePosition(float *out, int id)
{
	char *node;

	for (node = fh_ListNext(GATES(), NULL); node != NULL; node = fh_ListNext(GATES(), node)) {
		if (FH_AI(node, -0x18) == id)
			copy_pos(out, FH_AV(node, -0x10));
	}
}

/* 0x00138620  (not in the function table; called by the game mode's exit): frees every
gate */
void FUN_00138620(void)
{
	char *node = fh_ListNext(GATES(), NULL);
	char *next;

	while (node != NULL) {
		next = fh_ListRemove(GATES(), node);
		if (node - 0x18 != NULL)
			bkHeapFree(node - 0x18);
		node = fh_ListNext(GATES(), next);
	}
}

/* 0x00138670  (gap): updates every gate */
void FoxHunt_UpdateGates(void)
{
	char *node = fh_ListNext(GATES(), NULL);

	while (node != NULL)
		node = fh_ListNext(GATES(), FoxHunt_Gate_Update(node - 0x18));
}

/* ======================================================================== the text reader */

/* 0x0026e048  vtable of the text reader */
const void *const vtable_0026e048[1] = {
	(const void *)FUN_00138770, /* [0] scalar deleting destructor */
};

/* 0x001386a0  (gap): the text reader's constructor (layout: struct BCarTextReader of
bumpercars_b: +0x4 buffer, +0x8 cursor, +0xc end, +0x10 name, +0x210 line) */
void FoxHunt_TextReader_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026e048;
}

/* 0x001386b0  (gap): the text reader's destructor */
void FoxHunt_TextReader_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026e048;
}

/* 0x001386c0  (gap): copies the line at the cursor into `buffer` (without the line end)
and steps over one character; returns the line without its leading blanks */
char *FoxHunt_TextReader_ReadLine(void *self, char *buffer)
{
	char *out = buffer;
	char c;

	if (FH_AP(self, 0x8) < FH_AP(self, 0xc)) {
		do {
			c = *FH_AP(self, 0x8);
			if (c == '\n' || c == '\r')
				break;
			*out++ = c;
			FH_AP(self, 0x8)++;
		} while (FH_AP(self, 0x8) < FH_AP(self, 0xc));
	}
	*out = 0;
	FH_AP(self, 0x8)++;
	while (*buffer != 0 && (*buffer == ' ' || *buffer == '\r' || *buffer == '\n' || *buffer == '\t'))
		buffer++;
	return buffer;
}

/* 0x00138720  (gap): moves the cursor to the start of the next line and counts it */
void FoxHunt_TextReader_NextLine(void *self)
{
	char *end = FH_AP(self, 0xc);
	char c;

	if (FH_AP(self, 0x8) < end) {
		do {
			c = *FH_AP(self, 0x8);
			if (c == '\n' || c == '\r')
				break;
			FH_AP(self, 0x8)++;
		} while (FH_AP(self, 0x8) < FH_AP(self, 0xc));
		if (FH_AP(self, 0x8) < end) {
			do {
				c = *FH_AP(self, 0x8);
				if (c != '\n' && c != '\r')
					break;
				FH_AP(self, 0x8)++;
			} while (FH_AP(self, 0x8) < FH_AP(self, 0xc));
		}
	}
	FH_AI(self, 0x210)++;
}

/* 0x00138770  (gap): the text reader's scalar deleting destructor */
void *FoxHunt_TextReader_Destroy(void *self, unsigned char flags)
{
	*(const void *const **)self = vtable_0026e048;
	if ((flags & 1) != 0)
		FH_DELETE(self);
	return self;
}

/* ======================================================================== CWaterField */

/* 0x00138790  (gap; the class is classlib_fx's CWaterField, vtable 0x0026e05c):
CWaterField::CWaterField: clears the buffers and the 0x2400 words of edge and cell data */
void *FoxHunt_CWaterField_Ctor(void *self)
{
	unsigned int *w = (unsigned int *)self;
	int i;

	w[0x241e] = 0;
	w[0x2422] = 0;
	w[0x2421] = 0;
	w[0x11] = 0;
	w[0x2423] = 0;
	*(const void *const **)self = vtable_0026e05c;
	for (i = 0; i < 0x2400; i++)
		w[0x12 + i] = 0;
	return self;
}
