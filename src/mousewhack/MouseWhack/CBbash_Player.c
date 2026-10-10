/*
MouseWhack/CBbash_Player.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CBbash_Player.cpp
(module mousewhack)

CBbash_Player: a Bug Bash player, derived from the CharLib AI player (charlib_ai.h, 0x1130
bytes) and 0x1410 bytes in all; vtable 0x00272f68. Humans drive it through their input
map, the computer through the AI state machine of 0x002b4b60 (states), 0x002b4b3c
(conditions) and 0x002b4ae8 (priorities) in the module's data block. The player swings a
mallet (a paint gun in "Pied Pipers") at bugs and other players.

Fields after the AI base (byte offsets):
  +0x1138 the hammer's hit actor (models\debug.obe, a 75-radius collision)
  +0x113c the mallet/gun model, +0x1140 its instance (BActorInstance)
  +0x12b0 the "hammer" node of the mallet, +0x12b4 the palm node, +0x12b8 the shoulder node
  +0x12c0 short team, +0x12c4 float[4] the velocity of the platform he stands on
  +0x12d4 float time since the last knock-down, +0x12e0 short frames the action is held,
  +0x12e2 short, +0x12e4 short (20), +0x12f2 short the stun animation, +0x12f4 short 1:
  smash (ground bugs ahead), 0: swipe, +0x12f6 short riding the train (said so),
  +0x12f8 float the gun's cool-down, +0x12fc float stun time, +0x1300 float[4] stun
  tweaks, +0x1310 float the heading he was hit from
  computer players: +0x1314 float aim wobble range, +0x1318 float aim wobble,
  +0x131c short frames to the next wobble, +0x1320 float[4] a push to follow (Tremors),
  +0x1330 short follow it, +0x1332 short side (+-1), +0x1334 short frames the side holds,
  +0x1338 float the side's x line, +0x133c float chance to defend his stand, +0x1340
  float chance to go for a bug, +0x1344/+0x1348 float the steering bend, +0x134c short
  the bend direction, +0x134e short aggression, +0x1350 short, +0x1352 short random
  aggression, +0x1354 float swing distance, +0x1358 short frames to re-check threats,
  +0x135a short defending, +0x135c short the stand he attacks, +0x1360 float time at
  it, +0x1364 the bug he chases, +0x1368 short frames to re-check players,
  +0x136a short attacking a player, +0x136c short that player
  +0x136e/+0x1370/+0x1372 short his colour, +0x1374 short his character
  +0x13ec short[4] grudges against each player, +0x13f4 short[4] hits on each player
  +0x13fc short knock-downs in a row, +0x1400 int score
*/

#include "mousewhack.h"

#define PL_S(off) MW_AS(self, off)
#define PL_I(off) MW_AI(self, off)
#define PL_F(off) MW_AF(self, off)
#define PL_V(off) MW_AV(self, off)
#define PL_ACTOR ((unsigned char *)MW_AP(self, 0x30))
#define PA_F(off) MW_AF(PL_ACTOR, off)
#define PA_V(off) MW_AV(PL_ACTOR, off)
#define PL_INDEX ((int)MW_AT(PL_ACTOR, 0x230, unsigned short))   /* his player slot */

/* core: the team of each player record (+0x20) */
extern int DAT_002baef4[];
extern int DAT_002baed8[];
extern int DAT_002baee0[];
/* charlib: the voice volume */
extern int DAT_00293928;

/* the actor of player `i` */
#define PL_ACTOR_OF(i) ((unsigned char *)MW_AP(mw_Players[i], 0x30))

/* "%d" as a floating number in the player's colour above `pos` */
static void pl_ShowNumber(void *self, float x, float z, int value)
{
	char text[32];

	sprintf(text, "%d", value);
	baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318), (double)x, 340.0, (double)z,
		(int)PL_S(0x136e), (int)PL_S(0x1370), (int)PL_S(0x1372), text, 1.0);
}

/* 0x00272f68  the CBbash_Player vtable (base: the CharLib AI, 0x00266fa8) */
const void *const vtable_00272f68[20] = {
	(const void *)FUN_001d5950, /* [0] +0x00 (empty) */
	(const void *)FUN_001d5950, /* [1] +0x04 (empty) */
	(const void *)FUN_001d5950, /* [2] +0x08 (empty) */
	(const void *)FUN_000b1860, /* [3] +0x0c AI_Think */
	(const void *)FUN_0017f8a0, /* [4] +0x10 CBbash_Player_Strike */
	(const void *)FUN_001d5950, /* [5] +0x14 (empty) */
	(const void *)FUN_001ec260, /* [6] +0x18 (pure) */
	(const void *)FUN_0017f000, /* [7] +0x1c CBbash_Player_Steer */
	(const void *)FUN_000b0560, /* [8] +0x20 */
	(const void *)FUN_000b0370, /* [9] +0x24 */
	(const void *)FUN_000b0cb0, /* [10] +0x28 */
	(const void *)FUN_001d5950, /* [11] +0x2c (empty) */
	(const void *)FUN_000b06c0, /* [12] +0x30 */
	(const void *)FUN_000a6fa0, /* [13] +0x34 */
	(const void *)FUN_001ec260, /* [14] +0x38 (pure) */
	(const void *)FUN_001ec260, /* [15] +0x3c (pure) */
	(const void *)FUN_000b0770, /* [16] +0x40 */
	(const void *)FUN_0017d550, /* [17] +0x44 CBbash_Player_SetModel */
	(const void *)FUN_0017e640, /* [18] +0x48 CBbash_Player_Tag */
	(const void *)FUN_0017d180, /* [19] +0x4c CBbash_Player_UpdateAction */
};

/* 0x0017d180  CBbash_Player.cpp (vtable +0x4c): counts the frames the action button is
held; in "Pied Pipers" the gun lets go of it while it cools down */
void CBbash_Player_UpdateAction(void *self)
{
	if (MW_AI(MW_AP(self, 0x6c), 0x14) != 0) {
		PL_S(0x12e0) = (short)(PL_S(0x12e0) + 1);
		if (mw_Variation == 3 && 0.0f < PL_F(0x12f8)) {
			MW_AI(MW_AP(self, 0x6c), 0x14) = 0;
			PL_S(0x12e0) = 0;
		}
	}
	if (0.0f < PL_F(0x12f8))
		PL_F(0x12f8) = PL_F(0x12f8) - DAT_0028fda8;
}

/* 0x0017d1f0  CBbash_Player.cpp: destructor (missing from the function table) */
void FUN_0017d1f0(void *self)
{
	*(const void *const **)self = vtable_00272f68;
	AI_Dtor(self);
}

/* 0x0017d200  CBbash_Player.cpp: forgets the bug he chases (+0x1364) when it is gone,
dead, his own ("Pied Pipers"), or too far from his stand ("Burgers") */
void CBbash_Player_CheckChasedBug(void *self)
{
	unsigned char *bug = (unsigned char *)MW_AP(self, 0x1364);
	short state;

	if ((mw_Variation == 3 && (int)MW_AS(bug, 0x120c) == PL_INDEX) || MW_AS(bug, 0x11fa) == 0 ||
	    (mw_Variation == 2 && 800.0f < bmVectorDistance(MW_AV(MW_AP(bug, 0), 0xd0), MW_FV(0x002b2718 + PL_INDEX * 0x10)))) {
		PL_I(0x1364) = 0;
		return;
	}
	if (MW_AI(bug, 0) != 0 || MW_AI(bug, 4) != 0) {
		state = MW_AS(bug, 0x11ee);
		switch (MW_AS(bug, 0x11ec)) {
		case 0:
		case 1:
			if (state != 3)
				return;
			break;
		case 2:
			if (state != 6 && state != 4)
				return;
			break;
		case 3:
			if (state != 4)
				return;
			break;
		default:
			break;
		}
	}
	PL_S(0x135a) = 0;
	PL_I(0x1364) = 0;
}

/* 0x0017d2f0  CBbash_Player.cpp: the knock-down count (+0x13fc) is forgotten after 300
frames */
void CBbash_Player_UpdateKnockCount(void *self)
{
	if (PL_S(0x13fc) != 0) {
		PL_F(0x12d4) = DAT_0028fda8 + PL_F(0x12d4);
		if (PL_F(0x12d4) <= 300.0f)
			return;
		PL_S(0x13fc) = 0;
	}
	PL_I(0x12d4) = 0;
}

/* 0x0017d330  CBbash_Player.cpp: the input maps of the players: "Forward", "leftRight"
(stick and d-pad), "jump", "action" (two buttons) and "action2" */
void CBbash_Player_CreateInputMaps(void)
{
	void *player;
	unsigned int pad;
	int i;

	for (i = 0; i < DAT_002bae94; i++) {
		pad = (unsigned int)DAT_002baee0[i * 10];
		player = mw_Players[i];
		MW_AP(player, 0x5c) = biCreateMap(10);
		player = mw_Players[i];
		MW_AP(player, 0x60) = biCreateChannel((struct biMap *)MW_AP(player, 0x5c), "Forward", 3, 0, 0, 0, 0);
		MW_AP(player, 0x64) = biCreateChannel((struct biMap *)MW_AP(player, 0x5c), "leftRight", 3, 0, 0, 0, 0);
		MW_AP(player, 0x68) = biCreateChannel((struct biMap *)MW_AP(player, 0x5c), "jump", 1, 1, 0, 0, 0);
		MW_AP(player, 0x6c) = biCreateChannel((struct biMap *)MW_AP(player, 0x5c), "action", 1, 0, 0, 0, 0);
		MW_AP(player, 0x70) = biCreateChannel((struct biMap *)MW_AP(player, 0x5c), "action2", 1, 0, 0, 0, 0);
		biBindInput((struct biChannel *)MW_AP(player, 0x60), pad | 0xba, 0);
		biBindInput((struct biChannel *)MW_AP(player, 0x64), pad | 0xb9, 0);
		biBindInput((struct biChannel *)MW_AP(player, 0x60), pad | 0xbe, pad | 0xbd);
		biBindInput((struct biChannel *)MW_AP(player, 0x64), pad | 0xc0, pad | 0xbf);
		biBindInput((struct biChannel *)MW_AP(player, 0x68), pad | 0xc5, 0);
		biBindInput((struct biChannel *)MW_AP(player, 0x6c), pad | 0xc6, 0);
		biBindInput((struct biChannel *)MW_AP(player, 0x6c), pad | 199, 0);
	}
}

/* 0x0017d510  CBbash_Player.cpp: a random number in -1..1 */
float MW_RandSigned(void)
{
	return (float)((double)mw_rand() * (double)6.1037019e-05f - (double)1.0f);
}

/* 0x0017d550  CBbash_Player.cpp (vtable +0x44): rebuilds the player's actor from the
high (0) or low (1) detail model of his character, keeping the position; the low model
borrows the high model's animations */
void CBbash_Player_SetModel(void *self, int lowDetail)
{
	unsigned char *actor = PL_ACTOR;
	int character = DAT_002baed4[MW_AT(actor, 0x230, unsigned short) * 10];
	void *model;
	float pos[4];

	model = lowDetail == 0 ? MW_P(0x002b26bc + character * 0x10) : MW_P(0x002b26c4 + character * 0x10);
	if (model != NULL) {
		pos[0] = MW_AF(actor, 0xd0);
		pos[1] = MW_AF(actor, 0xd4);
		pos[2] = MW_AF(actor, 0xd8);
		pos[3] = MW_AF(actor, 0xdc);
		baDeleteActorInstance((struct BActorInstance *)(actor + 0x10));
		Actor_Create(model, pos, MW_AP(self, 0x30), 1, 0, 0);
		if (PL_S(0x1374) == 2) {
			MW_AP(self, 0x12b4) = baFindNode((struct BActorNode *)MW_AP(PL_ACTOR, 0x144), "lpalm");
			MW_AP(self, 0x12b8) = baFindNode((struct BActorNode *)MW_AP(PL_ACTOR, 0x144), "lshoulder");
		} else {
			MW_AP(self, 0x12b4) = baFindNode((struct BActorNode *)MW_AP(PL_ACTOR, 0x144), "rpalm");
			MW_AP(self, 0x12b8) = baFindNode((struct BActorNode *)MW_AP(PL_ACTOR, 0x144), "rshoulder");
		}
		Actor_PlayAnim(PL_ACTOR, 0, 0, 0, 1.0f, 0.0f, 0);
		baUpdateAnimations((struct BActorInstance *)(PL_ACTOR + 0x10), (struct BActorNode *)MW_AP(PL_ACTOR, 0x144), 0.0f,
			NULL);
		baPrepActorInstance((struct BActorInstance *)(PL_ACTOR + 0x10), 1);
	}
	if (lowDetail != 0 && model != NULL) {
		character = DAT_002baed4[MW_AT(PL_ACTOR, 0x230, unsigned short) * 10];
		if (MW_P(0x002b26bc + character * 0x10) != NULL)
			baSetInstanceAnimations((struct BActor *)MW_P(0x002b26bc + character * 0x10),
				(struct BActorInstance *)(PL_ACTOR + 0x10));
	}
}

/* the mallet (gun in "Pied Pipers") model of each character */
static const char *const pl_WeaponModels[6][2] = {
	{"models\\malletdub.obe", "models\\pntgundub.obe"},
	{"models\\malletfly.obe", "models\\pntgunfly.obe"},
	{"models\\malletgeena.obe", "models\\pntgungeena.obe"},
	{"models\\malletzak.obe", "models\\pntgunzak.obe"},
	{"models\\malletjade.obe", "models\\pntgunjade.obe"},
	{"models\\malletjet.obe", "models\\pntgunjet.obe"},
};

static const char *const pl_WeaponErrors[6] = {
	"Failed to find dubs hammer/gun expect a crash at some point\n",
	"Failed to find flys hammer/gun expect a crash at some point\n",
	"Failed to find geenas hammer/gun expect a crash at some point\n",
	"Failed to find zaks hammer/gun expect a crash at some point\n",
	"Failed to find jades hammer expect a crash at some point\n",
	"Failed to find jets hammer expect a crash at some point\n",
};

/* 0x0017d6d0  CBbash_Player.cpp: constructor: player slot `player` at `pos` */
void *CBbash_Player_Ctor(void *self, int player, float *pos)
{
	float zero[4];
	unsigned char *hit;
	int character, i;
	unsigned int r;

	AI_Ctor(self);
	*(const void *const **)self = vtable_00272f68;
	zero[0] = 0.0f;
	zero[1] = 0.0f;
	zero[2] = 0.0f;
	zero[3] = 1.0f;
	MW_AP(self, 0x30) = Actor_Create(MW_P(0x002b26bc + DAT_002baed4[player * 10] * 0x10), pos, NULL, 1, 0, 1);
	MW_AI(PL_ACTOR, 0x220) = 0x59;
	MW_AS(PL_ACTOR, 0x230) = (short)player;
	PL_S(0x98) = 1;
	PL_S(0x34) = 9;
	PL_S(0x36) = 0;
	PL_I(4) = 0;
	PL_I(8) = 0;
	PL_S(0x1350) = 0;
	bkPrintf("player num %d\n", player);
	MW_AP(self, 0x1094) = MW_A(0x002b4b60);
	MW_AP(self, 0x1098) = MW_A(0x002b4b3c);
	MW_AP(self, 0x109c) = MW_A(0x002b4ae8);
	for (i = 0; i < 4; i++) {
		PL_S(0x13ec + i * 2) = 0;
		PL_S(0x13f4 + i * 2) = 0;
	}
	PL_S(0x12f6) = 0;
	PL_I(0x12d4) = 0;
	character = DAT_002baed4[player * 10];
	PL_S(0x136e) = (short)DAT_00290718[character * 0x60];
	PL_S(0x1370) = (short)DAT_00290718[character * 0x60 + 1];
	PL_S(0x1372) = (short)DAT_00290718[character * 0x60 + 2];
	PL_S(0x1374) = (short)DAT_002baed4[player * 10];
	PL_S(0x12c0) = (short)DAT_002baef4[player * 10];
	PL_S(0x12e0) = 0;
	PL_S(0x12e2) = 0;
	PL_S(0x12e4) = 0x14;
	PL_I(0x1400) = 0;
	PL_S(0x13fc) = 0;
	Actor_InitCharData(PL_ACTOR);
	PL_I(0x12c4) = 0;
	PL_I(0x12c8) = 0;
	PL_I(0x12cc) = 0;
	PL_F(0x12d0) = 1.0f;
	PL_I(0x113c) = 0;
	PL_S(0x12f0) = 0;
	PL_S(0x12ec) = 0;
	PL_S(0x12d8) = 0;
	PL_I(0x12dc) = 0;
	PL_S(0x135c) = -1;
	PL_F(0x12fc) = 70.0f;
	PL_I(0x1124) = 0;
	if ((unsigned short)PL_S(0x1374) < 6) {
		MW_AP(self, 0x113c) = bLoadResource(NULL, pl_WeaponModels[PL_S(0x1374)][mw_Variation == 3], 1, 0xdefa);
		MW_AP(PL_ACTOR, 0x324) = MW_A(0x002b34f0);   /* the animation name table ("idle_breathe", ...) */
		if (MW_AP(self, 0x113c) == NULL)
			bkPrintf(pl_WeaponErrors[PL_S(0x1374)]);
	}
	MW_AF(DAT_002ecfac, 0xb8 + player * 4) = 180.0f;
	MW_AF(DAT_002ecfac, 0xa8 + player * 4) = 275.0f;
	MW_AP(self, 0x1138) = Actor_Create(bLoadResource(NULL, "models\\debug.obe", 1, 0xdefa), zero, NULL, 1, 0, 1);
	MW_AI(MW_AP(self, 0x1138), 0x220) = 0x100010;
	Actor_InitCharData(MW_AP(self, 0x1138));
	baInitActorInstance((struct BActorInstance *)((char *)self + 0x1140), (struct BActor *)MW_AP(self, 0x113c), 2,
		"C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBbash_Player.cpp", 0x11b);
	hit = (unsigned char *)MW_AP(MW_AP(self, 0x1138), 0x18c);
	MW_AF(hit, 0x9c) = 75.0f;
	MW_AI(hit, 0x90) = 0;
	MW_AF(hit, 0x80) = 75.0f;
	MW_AF(hit, 0xa0) = 150.0f;
	MW_AI(hit, 0x1ac) = 0;
	*(float *)MW_AP(MW_AP(self, 0x1138), 0x188) = 1.0f;
	if (MW_AP(self, 0x113c) == NULL)
		bkPrintf("failed to find hammer\n");
	PL_I(0x12b0) = 0;
	PL_I(0x12bc) = 0;
	MW_AP(self, 0x12b0) = baFindNode((struct BActorNode *)MW_AP(self, 0x1274), "hammer");
	bkPrintf(MW_AP(self, 0x12b0) == NULL ? "Could not find object to attach\n" : "Found object to attach\n");
	PL_I(0x12b4) = 0;
	if (PL_S(0x1374) == 2) {
		MW_AP(self, 0x12b4) = baFindNode((struct BActorNode *)MW_AP(PL_ACTOR, 0x144), "lpalm");
		MW_AP(self, 0x12b8) = baFindNode((struct BActorNode *)MW_AP(PL_ACTOR, 0x144), "lshoulder");
	} else {
		MW_AP(self, 0x12b4) = baFindNode((struct BActorNode *)MW_AP(PL_ACTOR, 0x144), "rpalm");
		MW_AP(self, 0x12b8) = baFindNode((struct BActorNode *)MW_AP(PL_ACTOR, 0x144), "rshoulder");
	}
	bkPrintf(MW_AP(self, 0x12b4) == NULL ? "Could not find object to attach to\n" : "Found object to attach to\n");
	for (i = 0; i < (800 - 0x264) / 4; i++)
		MW_AP(PL_ACTOR, 0x264 + i * 4) = baFindSubObject((struct BActorInstance *)(PL_ACTOR + 0x10),
			(const char *)MW_AP(PL_ACTOR, 0x324) + i * 0x44);
	PL_I(0x12f8) = 0;
	/* (an int into the AI's think interval, as the original) */
	PL_I(0x10b4) = MW_AT(DAT_00290718, DAT_002baed4[player * 10] * 0x60 + 0x38, int) / (DAT_002bae70 + 1);
	MW_I(0x0030ec50 + player * 4) = DAT_002baed8[player * 10] == 0 ? 1 : 0;
	PL_I(0x133c) = MW_I(0x0030ec34);
	PL_I(0x10e0) = MW_I(0x0030ec50 + player * 4) == 0 ? 0 : MW_I(0x0030ec3c);
	PL_I(0x1344) = MW_I(0x0030ec40);
	PL_I(0x1348) = MW_I(0x0030ec40);
	PL_I(0x1314) = MW_I(0x0030ec44);
	PL_I(0x1340) = MW_I(0x0030ec38);
	PL_I(0x1354) = MW_I(0x0030ec48);
	PL_S(0x134e) = (short)MW_FTOL(MW_F(0x0030ec4c));
	PL_S(0x131c) = 1;
	PL_S(0x134c) = -1;
	r = (unsigned int)mw_rand();
	PL_I(0x12e8) = 0;
	PL_S(0x12ee) = 0;
	PL_S(0x1352) = (short)((short)(r % 0x32) - 0x19);
	r = (unsigned int)mw_rand();
	PL_I(0x1360) = 0;
	PL_F(0x1318) = ((float)r * 6.1037019e-05f - 1.0f) * PL_F(0x1314) * 0.5f;
	PL_I(0x1320) = 0;
	PL_I(0x1324) = 0;
	PL_I(0x1328) = 0;
	PL_F(0x132c) = 1.0f;
	PL_I(0x1338) = 0;
	PL_S(0x1368) = 0;
	PL_S(0x136a) = 0;
	PL_S(0x1358) = 0;
	PL_S(0x135a) = 0;
	PL_I(0x1364) = 0;
	PL_S(0x1334) = 0;
	PL_S(0x1330) = 0;
	PL_S(0x12f2) = 0;
	PL_I(0x1300) = 0;
	PL_I(0x1304) = 0;
	PL_I(0x1308) = 0;
	PL_I(0x130c) = 0;
	PL_I(0x1310) = 0;
	PL_S(0x1332) = 0;
	PL_S(0x136c) = -1;
	PL_S(0x12f4) = 1;
	return self;
}

/* 0x0017df00  CBbash_Player.cpp: platforms: takes the velocity of the platform he
stands on, keeps drifting with it (slowing down) after he leaves it; on the train of
"Tremors" he says something once */
void CBbash_Player_UpdatePlatform(void *self)
{
	unsigned char *platform = (unsigned char *)MW_AP(MW_AP(PL_ACTOR, 0x18c), 0x1b0);
	unsigned char *actor;

	if (platform != NULL) {
		PL_I(0x12c4) = MW_AI(platform, 0x1a4);
		PL_I(0x12c8) = MW_AI(platform, 0x1a8);
		PL_I(0x12cc) = MW_AI(platform, 0x1ac);
		PL_I(0x12d0) = MW_AI(platform, 0x1b0);
	}
	actor = PL_ACTOR;
	if (MW_AP(MW_AP(actor, 0x18c), 0x1b0) == NULL) {
		MW_AF(actor, 0xd0) = MW_AF(actor, 0xd0) + PL_F(0x12c4);
		MW_AF(actor, 0xd4) = MW_AF(actor, 0xd4) + PL_F(0x12c8);
		MW_AF(actor, 0xd8) = MW_AF(actor, 0xd8) + PL_F(0x12cc);
		PL_S(0x12f6) = 0;
	} else if (mw_Variation == 1 && PL_S(0x12f6) == 0) {
		if (mw_rand() % 3 == 0)
			MW_Player_PlayVoice(self, 2, DAT_00293928, 100.0f, 0, -1, 0);
		PL_S(0x12f6) = 1;
	}
	MW_AI(MW_AP(PL_ACTOR, 0x18c), 0x1bc) = 0;
	PL_F(0x12c4) = PL_F(0x12c4) * 0.8f;
	PL_F(0x12c8) = PL_F(0x12c8) * 0.8f;
	PL_F(0x12cc) = PL_F(0x12cc) * 0.8f;
}

/* the player's side rule (+0x1334 frames left, +0x1332 side, +0x1338 x line): may he go
for something at x? */
static int pl_SideAllows(void *self, float x)
{
	if (PL_S(0x1334) == 0)
		return 1;
	return (x <= PL_F(0x1338) || -1 < PL_S(0x1332)) && (PL_F(0x1338) <= x || PL_S(0x1332) < 1);
}

/* 0x0017e050  CBbash_Player.cpp: a computer player picks another player to attack (not
in "Pied Pipers"): the first one (of another team) whose grudge less hits plus his
aggression is above 2 */
void CBbash_Player_ChooseEnemy(void *self)
{
	short i;

	if (mw_Variation == 3)
		return;
	if (mw_DbgMessages != 0)
		bkPrintf("checking for player targets\n");
	for (i = 0;; i++) {
		if ((MW_I(0x0030a278) == 0 || PL_S(0x12c0) != MW_AS(mw_Players[i], 0x12c0)) &&
		    2 < (short)(PL_S(0x13ec + i * 2) - PL_S(0x13f4 + i * 2) + PL_S(0x134e)))
			break;
		if (3 < (short)(i + 1))
			return;
	}
	if (!pl_SideAllows(self, MW_AF(PL_ACTOR_OF(i), 0xd0)))
		return;
	if (mw_DbgMessages != 0)
		bkPrintf("player %d attacking player %d\n", (int)MW_AS(PL_ACTOR, 0x230), (int)i);
	if (mw_rand() % 3 == 0)
		CLSamples_PlayVoice((char)DAT_002baed4[PL_INDEX * 10], 0xc, NULL, DAT_00293928, 0.0f, 0, -1, 0);
	PL_S(0x136a) = 1;
	PL_S(0x136c) = i;
}

/* 0x0017e1d0  CBbash_Player.cpp: a computer player looks for a bug that goes for him
or his stand (a roach attacking a stand in "Burgers", or an attacking fly) and defends
with the chance +0x133c */
void CBbash_Player_ChooseThreat(void *self)
{
	unsigned char *node;
	int threat;

	if (PL_S(0x135a) != 0)
		return;
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
		unsigned char *bug = node - 8;

		threat = 0;
		if (mw_Variation == 2) {
			if (MW_AS(bug, 0x11ec) == 1 && MW_AS(bug, 0x11ee) == 4)
				threat = 1;
			if (MW_AS(bug, 0x11ec) == 3 && MW_AS(bug, 0x11ee) == 5)
				threat = 1;
		} else if (MW_AS(bug, 0x11ec) == 3 && MW_AS(bug, 0x11ee) == 5) {
			threat = 1;
		}
		if (threat && (int)MW_AS(bug, 0x1204) == PL_INDEX) {
			if (PL_F(0x133c) <= (float)mw_rand() * 3.0518509e-05f)
				return;
			MW_AP(self, 0x1364) = bug;
			PL_S(0x135a) = 1;
			return;
		}
	}
}

/* 0x0017e2d0  CBbash_Player.cpp: a computer player picks the nearest live bug to chase
(+0x1364) that no team mate chases and that nobody else reaches first; with the chance
+0x1340 he takes the first suitable one. In "Burgers" only bugs within 800 of his
stand; in "Pied Pipers" maybe not his own. */
void CBbash_Player_ChooseBug(void *self)
{
	unsigned char *node, *bug, *chosen = NULL;
	float best = 1e9f, dist, chance;
	int me, k, taken, mateTaken;

	PL_S(0x135a) = 0;
	PL_I(0x1364) = 0;
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
		bug = node - 8;
		if (MW_AS(bug, 0x11ec) == 7 || MW_AS(bug, 0x11ec) == 8)
			continue;
		dist = bmVectorDistance(PA_V(0xd0), MW_AV(MW_AP(bug, 0), 0xd0));
		if (mw_Variation == 2 && !(bmVectorDistance(MW_FV(0x002b2718 + PL_INDEX * 0x10), MW_AV(MW_AP(bug, 0), 0xd0)) <= 800.0f))
			continue;
		chance = (float)mw_rand() * 3.0518509e-05f;
		me = PL_INDEX;
		if (!(((int)MW_AS(bug, 0x120c) != me || chance <= PL_F(0x1340) || mw_Variation != 3) &&
		      pl_SideAllows(self, MW_AF(MW_AP(bug, 0), 0xd0))))
			continue;
		switch (MW_AS(bug, 0x11ec)) {
		case 0: case 1: case 2: case 3: case 4:
			if (MW_AS(bug, 0x11ee) == 0)
				continue;
			break;
		}
		taken = 0;
		for (k = 0; k < 4; k++)
			if (k != me && bug == (unsigned char *)MW_AP(mw_Players[k], 0x1364))
				taken = 1;
		if (taken)
			continue;
		if (MW_I(0x0030a278) != 0) {
			mateTaken = 0;
			for (k = 0; k < 4; k++)
				if (k != me && PL_S(0x12c0) == MW_AS(mw_Players[k], 0x12c0) &&
				    bug == (unsigned char *)MW_AP(mw_Players[k], 0x1364))
					mateTaken = 1;
			if (mateTaken)
				continue;
		}
		chance = (float)mw_rand() * 3.0518509e-05f;
		if (MW_AS(bug, 0x11fa) != 0 && PL_F(0x1340) < chance) {
			if (MW_AP(self, 0x1364) == NULL)
				MW_AP(self, 0x1364) = bug;
			return;
		}
		for (k = 0; k < 4; k++) {
			unsigned char *theirs;

			if (k == PL_INDEX)
				continue;
			theirs = (unsigned char *)MW_AP(mw_Players[k], 0x1364);
			if (bug == theirs && bmVectorDistance(MW_AV(PL_ACTOR_OF(k), 0xd0), MW_AV(MW_AP(theirs, 0), 0xd0)) < dist)
				dist = 1e9f;
		}
		if (dist < best && MW_AS(bug, 0x11fa) != 0) {
			best = dist;
			MW_AP(self, 0x1364) = bug;
			chosen = bug;
		}
	}
	MW_AP(self, 0x1364) = chosen;
}

/* 0x0017e640  CBbash_Player.cpp (vtable +0x48), "Pied Pipers": the gun's tag: two blast
rings ahead (250 and 100); worms and roaches within them that are not his yet become his
(his colour's model, 0x0030a634 worms / 0x0030aef0 roaches) and run home (states 4/5);
he scores 0x0030ee10 per worm and 0x0030ee14 per roach. Returns -1. */
int CBbash_Player_Tag(void *self)
{
	float far_[4], near_[4], keep[4];
	unsigned char *node, *bug, *actor;
	void *ring;
	int k, time, value;

	far_[0] = 250.0f;
	far_[1] = 0.0f;
	far_[2] = 0.0f;
	bmVectorRotateY(far_, far_, PA_F(0x1e8) - 1.5707963f);
	far_[0] = far_[0] + PA_F(0xd0);
	far_[1] = far_[1] + PA_F(0xd4);
	far_[2] = far_[2] + PA_F(0xd8);
	near_[0] = 100.0f;
	near_[1] = 0.0f;
	near_[2] = 0.0f;
	bmVectorRotateY(near_, near_, PA_F(0x1e8) - 1.5707963f);
	near_[0] = near_[0] + PA_F(0xd0);
	near_[1] = near_[1] + PA_F(0xd4);
	near_[2] = near_[2] + PA_F(0xd8);
	PL_S(0x1092) = 1;
	PL_S(0x12e0) = 0;
	PL_S(0x12e2) = 0;
	for (k = 0; k < 10; k++) {
		ring = mw_Helpers[k];
		if (MW_AS(ring, 0x10) == 0) {
			MW_AS(ring, 0x10) = 1;
			MW_AF(ring, 0x18) = PA_F(0x1e8) - 1.5707963f;
			memcpy(ring, far_, 0x10);
			MW_AF(ring, 0x14) = 150.0f;
			MW_AS(ring, 0x12) = 0;
			break;
		}
	}
	for (k = 0; k < 10; k++) {
		ring = mw_Helpers[k];
		if (MW_AS(ring, 0x10) == 0) {
			MW_AS(ring, 0x10) = 1;
			MW_AF(ring, 0x18) = PA_F(0x1e8) - 1.5707963f;
			memcpy(ring, near_, 0x10);
			MW_AF(ring, 0x14) = 100.0f;
			MW_AS(ring, 0x12) = 0;
			break;
		}
	}
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
		bug = node - 8;
		if (!(bmVectorDistance(far_, MW_AV(MW_AP(bug, 0), 0xd0)) < 120.0f ||
		      bmVectorDistance(near_, MW_AV(MW_AP(bug, 0), 0xd0)) < 100.0f))
			continue;
		if (MW_AS(bug, 0x11ec) == 0) {
			if ((int)MW_AS(bug, 0x120c) == PL_INDEX)
				continue;
			PL_I(0x1400) = PL_I(0x1400) + MW_I(0x0030ee10);
			actor = (unsigned char *)MW_AP(bug, 0);
			MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
			time = MW_AI(MW_AP(actor, 0x144), 0x74);
			memcpy(keep, MW_AV(actor, 0xd0), 0x10);
			baDeleteActorInstance((struct BActorInstance *)(actor + 0x10));
			baInitActorInstance((struct BActorInstance *)((unsigned char *)MW_AP(bug, 0) + 0x10),
				(struct BActor *)MW_P(0x0030a634 + PL_S(0x1374) * 4), 8,
				"C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBbash_Player.cpp", 0x55e);
			memcpy(MW_AV(MW_AP(bug, 0), 0xd0), keep, 0x10);
			MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
			MW_AS(bug, 0x11ee) = 4;
			MW_AI(bug, 0x11f0) = 0;
			Actor_PlayAnim(MW_AP(bug, 0), 0, 1, 0, 1.0f, 0.1f, 1);
			baSetNodeTime((struct BActorInstance *)((unsigned char *)MW_AP(bug, 0) + 0x10), NULL, time, 1);
			value = MW_I(0x0030ee10);
		} else if (MW_AS(bug, 0x11ec) == 1 && (int)MW_AS(bug, 0x120c) != PL_INDEX) {
			PL_I(0x1400) = PL_I(0x1400) + MW_I(0x0030ee14);
			actor = (unsigned char *)MW_AP(bug, 0);
			MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
			memcpy(keep, MW_AV(actor, 0xd0), 0x10);
			time = MW_AI(MW_AP(actor, 0x144), 0x74);
			baDeleteActorInstance((struct BActorInstance *)(actor + 0x10));
			baInitActorInstance((struct BActorInstance *)((unsigned char *)MW_AP(bug, 0) + 0x10),
				(struct BActor *)MW_P(0x0030aef0 + PL_S(0x1374) * 4), 8,
				"C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBbash_Player.cpp", 0x580);
			memcpy(MW_AV(MW_AP(bug, 0), 0xd0), keep, 0x10);
			MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
			MW_AS(bug, 0x11ee) = 5;
			MW_AI(bug, 0x11f0) = 0;
			Actor_PlayAnim(MW_AP(bug, 0), 0, 1, 0, 1.0f, 0.1f, 1);
			baSetNodeTime((struct BActorInstance *)((unsigned char *)MW_AP(bug, 0) + 0x10), NULL, time, 1);
			value = MW_I(0x0030ee14);
		} else {
			continue;
		}
		{
			char text[32];
			void *owner = mw_Players[MW_AS(bug, 0x120c)];

			sprintf(text, "%d", value);
			baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318),
				(double)MW_AF(MW_AP(owner, 0x30), 0xd0), 340.0, (double)MW_AF(MW_AP(owner, 0x30), 0xd8),
				(int)MW_AS(owner, 0x136e), (int)MW_AS(owner, 0x1370), (int)MW_AS(owner, 0x1372), text, 1.0);
		}
	}
	return -1;
}

/* 0x0017eb80  CBbash_Player.cpp: looks at the bugs within 420: turns towards the nearest
one (when within 45 degrees of where he wants to face) and chooses a smash (+0x12f4 = 1:
at least as many ground bugs ahead as flying ones) or a swipe (0) */
void CBbash_Player_LookAround(void *self)
{
	unsigned char *node, *bug, *nearest = NULL;
	float best = 1e7f, dist, dx, dz, heading;
	short ground = 0, air = 0;

	node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL);
	if (node != NULL) {
		do {
			bug = node - 8;
			if (MW_AS(bug, 0x11ec) != 8) {
				dx = PA_F(0xd0) - MW_AF(MW_AP(bug, 0), 0xd0);
				dz = PA_F(0xd8) - MW_AF(MW_AP(bug, 0), 0xd8);
				dist = sqrtf(dx * dx + dz * dz);
				if (dist < 420.0f) {
					if (dist < best && MW_AS(bug, 0x11ec) < 5) {
						best = dist;
						nearest = bug;
					}
					heading = CLMath_HeadingTo(PA_V(0xd0), MW_AV(MW_AP(bug, 0), 0xd0));
					if (CLMath_AngleDistance(heading, PA_F(0x1e8)) < 1.0005071f) {
						switch (MW_AS(bug, 0x11ec)) {
						case 0: case 1: case 2:
							ground = (short)(ground + 1);
							break;
						case 3: case 4:
							air = (short)(air + 1);
							break;
						}
					}
				}
			}
			node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node);
		} while (node != NULL);
		if (nearest != NULL) {
			heading = CLMath_HeadingTo(PA_V(0xd0), MW_AV(MW_AP(nearest, 0), 0xd0));
			if (CLMath_AngleDistance(heading, PA_F(0x1d8)) < 0.7853981f)
				PA_F(0x1d8) = heading;
		}
		if (ground < air) {
			PL_S(0x12f4) = 0;
			return;
		}
	}
	PL_S(0x12f4) = 1;
}

/* 0x0017ed40  CBbash_Player.cpp: the player is hit from `from`: stunned (state 6, a
voice, a rumble, falls forwards or backwards: anims 0x11/0x12) or, with `knockedDown`,
knocked down (state 7, anims 0x13/0x14 at double speed, two rumbles) */
void CBbash_Player_Hit(void *self, float *from, int knockedDown)
{
	float heading, away;

	heading = CLMath_HeadingTo(PA_V(0xd0), from);
	away = CLMath_AngleDistance(heading, PA_F(0x1e8));
	PL_I(0x1300) = MW_I(0x0030ec24);
	PL_I(0x1308) = MW_I(0x0030ec2c);
	PL_I(0x1304) = MW_I(0x0030ec28);
	PL_I(0x130c) = MW_I(0x0030ec30);
	PL_F(0x1310) = heading;
	if ((short)knockedDown == 0) {
		PA_F(0x1d8) = heading;
		AI_SetState(self, 6);
		if (mw_rand() % 3 == 0)
			CLSamples_PlayVoice((char)DAT_002baed4[PL_INDEX * 10], 2, NULL, DAT_00293928, 0.0f, 0, -1, 0);
		CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, MW_AS(PL_ACTOR, 0x230), 0, 0x7fff, 500, 10, 0x46, 0, 3);
		if (1.5707963f < away) {
			Actor_PlayAnim(PL_ACTOR, 0x12, 0, 0, 1.0f, 0.1f, 1);
			PA_F(0x1d8) = CLMath_WrapAngle(heading + 3.1415925f);
			return;
		}
		Actor_PlayAnim(PL_ACTOR, 0x11, 0, 0, 1.0f, 0.1f, 1);
		PA_F(0x1d8) = heading;
		return;
	}
	PL_S(0x12f2) = away <= 1.5707963f ? 0x13 : 0x14;
	AI_SetState(self, 7);
	CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, MW_AS(PL_ACTOR, 0x230), 0, 0x7fff, 600, 10, 0x32, 0, 3);
	CLRumble_PlayMotor((int)(uintptr_t)DAT_002ed324, MW_AS(PL_ACTOR, 0x230), 1, 0x7fff, 500, 0x1e, 0x32, 0, 4);
	if (mw_rand() % 3 == 0)
		CLSamples_PlayVoice((char)DAT_002baed4[PL_INDEX * 10], 3, NULL, DAT_00293928, 0.0f, 0, -1, 0);
	PL_I(0x12fc) = MW_I(0x0030ec20);
	if (1.5707963f < away) {
		Actor_PlayAnim(PL_ACTOR, 0x14, 0, 0, 2.0f, 0.1f, 1);
		return;
	}
	Actor_PlayAnim(PL_ACTOR, 0x13, 0, 0, 2.0f, 0.1f, 1);
}

/* walking into a steep wall (a contact with a normal under 0.7 up, against the
direction): after 4 such frames the computer player jumps */
static void pl_JumpWalls(void *self, const float *dir)
{
	unsigned char *coll = (unsigned char *)MW_AP(PL_ACTOR, 0x18c);
	unsigned short count = MW_AT(coll, 0x64, unsigned short);
	unsigned char *tri;
	int i;

	for (i = 0; i < (int)count; i++) {
		tri = (unsigned char *)MW_AP(coll, i * 4);
		if (MW_AF(tri, 0x34) < 0.7f && MW_AF(tri, 0x30) * dir[0] + MW_AF(tri, 0x38) * dir[2] < -0.3f) {
			PL_S(0x1080) = (short)(PL_S(0x1080) + 5);
			if (PL_S(0x1080) < 0x15)
				return;
			MW_AI(MW_AP(self, 0x68), 0x14) = 1;
			PL_S(0x1080) = 0;
			return;
		}
	}
	if (PL_S(0x1080) != 0)
		PL_S(0x1080) = (short)(PL_S(0x1080) - 1);
}

/* 0x0017f000  CBbash_Player.cpp (vtable +0x1c): a computer player's steering (only with
the tweak "AI state" on): checks and picks the bug to chase, threats and enemies on
timers, then walks to the target (bug, enemy, enemy stand, or home to his own stand in
"Burgers"), bending his path (+0x1344 x +0x134c), swings when in reach (+0x1354 plus a
wobble), and jumps out of walls. "Tremors": follows a push (+0x1320) when told. Always
returns 0. */
int CBbash_Player_Steer(void *self)
{
	float dir[4], to[4];
	float dist, bend, reach, length;
	int me;

	if (mw_DbgAIState == 0)
		return 0;
	if ((MW_AP(self, 0x1364) == NULL || (CBbash_Player_CheckChasedBug(self), MW_AP(self, 0x1364) == NULL)) &&
	    (CBbash_Player_ChooseBug(self), mw_Variation == 3))
		PL_I(0x1344) = PL_I(0x1348);
	if (0xaa < (int)PL_S(0x1352) + (int)PL_S(0x1350))
		CBbash_Player_ChooseBug(self);
	if (mw_Variation == 1 && PL_S(0x1330) != 0) {
		PL_S(0x1330) = 0;
		dir[3] = 0.0f;
		bmVectorSetLength(dir, PL_V(0x1320), 0.5f);
		AI_SetStick(self, dir, 1.0f);
		to[0] = dir[0];
		to[1] = dir[1];
		to[2] = dir[2];
		to[3] = dir[3];
		bmVectorNormalise(to, to);
		pl_JumpWalls(self, to);
		return 0;
	}
	PL_S(0x1358) = (short)(PL_S(0x1358) - 1);
	PL_S(0x1368) = (short)(PL_S(0x1368) - 1);
	if (PL_S(0x1334) != 0)
		PL_S(0x1334) = (short)(PL_S(0x1334) - 1);
	if (PL_S(0x1358) < 1) {
		PL_S(0x1358) = 0x3c;
		CBbash_Player_ChooseThreat(self);
		PL_S(0x1352) = (short)((short)(mw_rand() % 0x32) - 0x19);
	}
	if (PL_S(0x1368) < 1) {
		if (PL_S(0x135a) == 0) {
			PL_S(0x1368) = 0x79;
			PL_I(0x1360) = 0;
			PL_S(0x135c) = -1;
			CBbash_Player_ChooseEnemy(self);
		} else {
			PL_S(0x1368) = 0x1e;
		}
	}
	me = PL_INDEX;
	if (MW_AP(self, 0x1364) == NULL && PL_S(0x136a) == 0) {
		/* nothing to do: in "Burgers" back to the front of his stand */
		float *stand;

		if (mw_Variation != 2)
			return 0;
		stand = MW_FV(0x002b2718 + me * 0x10);
		dir[3] = stand[3];
		dir[0] = (stand[0] + 160.0f) - PA_F(0xd0);
		dir[1] = stand[1] - PA_F(0xd4);
		dir[2] = (stand[2] + 160.0f) - PA_F(0xd8);
		length = (cmb_vec3_length(dir) - 50.0f) * 0.01f;
		if (length <= 0.0f)
			return 0;
		if (0.5f < length)
			length = 0.5f;
		bmVectorSetLength(dir, dir, length);
		AI_SetStick(self, dir, 1.0f);
		return 0;
	}
	if (PL_S(0x136a) == 0) {
		unsigned char *bugActor = (unsigned char *)MW_AP(MW_AP(self, 0x1364), 0);

		to[0] = MW_AF(bugActor, 0xd0);
		to[2] = MW_AF(bugActor, 0xd8);
		to[3] = MW_AF(bugActor, 0xdc);
	} else {
		if (mw_Variation == 2 && MW_F(0x0030e9f0 + PL_S(0x136a) * 4) <= 0.0f) {
			/* (the original tests the stand of +0x136a, the "attacking" flag, here) */
			PL_S(0x135c) = -1;
			PL_S(0x136a) = 0;
			MW_AS(mw_Players[me], 0x13ec + PL_S(0x136c) * 2) = 0;
		}
		to[0] = MW_AF(PL_ACTOR_OF(PL_S(0x136c)), 0xd0);
		to[2] = MW_AF(PL_ACTOR_OF(PL_S(0x136c)), 0xd8);
		to[3] = MW_AF(PL_ACTOR_OF(PL_S(0x136c)), 0xdc);
		if (mw_Variation == 2)
			PL_F(0x1360) = DAT_0028fda8 + PL_F(0x1360);
		if (100.0f <= PL_F(0x1360))
			PL_S(0x135c) = PL_S(0x136c);
		if (-1 < PL_S(0x135c)) {
			/* he goes for the enemy's burger stand */
			if (MW_F(0x0030e9f0 + PL_S(0x135c) * 4) <= 0.0f) {
				PL_S(0x135c) = -1;
				PL_S(0x136a) = 0;
				MW_AS(mw_Players[me], 0x13ec + PL_S(0x136c) * 2) = 0;
			}
			to[0] = MW_F(0x002b2718 + PL_S(0x136c) * 0x10);
			to[2] = MW_F(0x002b2720 + PL_S(0x136c) * 0x10);
			to[3] = MW_F(0x002b2724 + PL_S(0x136c) * 0x10);
			PL_S(0x1368) = 0x79;
		}
	}
	to[1] = PA_F(0xd4);
	dist = bmVectorDistance(PA_V(0xd0), to);
	if (dist <= 200.0f) {
		bend = 0.0f;
	} else {
		bend = dist - 200.0f;
		if (100.0f < dist - 200.0f)
			bend = 100.0f;
		bend = bend * 0.01f * 0.7853981f * PL_F(0x1344) * (float)(int)PL_S(0x134c);
	}
	if (mw_Variation == 3)
		PL_F(0x1344) = PL_F(0x1344) * 0.98f;
	dir[0] = to[0] - PA_F(0xd0);
	dir[1] = to[1] - PA_F(0xd4);
	dir[2] = to[2] - PA_F(0xd8);
	dir[3] = 1.0f;
	bmVectorNormalise(dir, dir);
	if (sqrtf(dir[0] * dir[0] + dir[2] * dir[2] + dir[1] * dir[1]) != 0.0f) {
		if (mw_Variation == 3) {
			bmVectorSetLength(dir, dir, 0.5f);
			AI_SetStick(self, dir, 300.0f <= dist ? 1.0f : 0.2f);
		} else {
			bmVectorRotateY(dir, dir, bend);
			dir[3] = 1.0f;
			bmVectorSetLength(dir, dir, 0.5f);
			AI_SetStick(self, dir, 1.0f);
		}
	}
	to[0] = dir[0];
	to[1] = dir[1];
	to[2] = dir[2];
	to[3] = 1.0f;
	bmVectorNormalise(to, to);
	PL_S(0x131c) = (short)(PL_S(0x131c) - 1);
	if (PL_S(0x131c) < 1) {
		PL_F(0x1318) = MW_RandSigned() * PL_F(0x1314) * 0.5f;
		PL_S(0x131c) = 0x32;
	}
	MW_AI(MW_AP(self, 0x6c), 0x14) = 0;
	PL_S(0x1350) = 0;
	reach = PL_F(0x1354) + PL_F(0x1318);
	if (mw_Variation == 3) {
		if (dist < reach * 5.0f) {
			float heading = CLMath_HeadingTo(PA_V(0xd0), MW_AV(MW_AP(MW_AP(self, 0x1364), 0), 0xd0));

			if (CLMath_AngleDistance(heading, PA_F(0x1e8)) < 0.3f)
				MW_AI(MW_AP(self, 0x6c), 0x14) = 1;
		}
	} else if (dist < reach) {
		MW_AI(MW_AP(self, 0x6c), 0x14) = 1;
	}
	pl_JumpWalls(self, to);
	return 0;
}

/* a hit bug: the player scores its value (+0x11fe), shown at the bug */
static void pl_ScoreBug(void *self, unsigned char *bug)
{
	PL_I(0x1400) = PL_I(0x1400) + (int)MW_AS(bug, 0x11fe);
}

/* 0x0017f8a0  CBbash_Player.cpp (vtable +0x10): the swing lands. A smash (+0x12f4 = 1,
always in "Pied Pipers") hits within 120 of the point 150 ahead and leaves a hammer
mark; a swipe hits within 320 in a 57-degree cone and leaves a swipe mark. A blast ring
shows it. Bugs: worms and roaches die (a splat), tremors and flies lose a hit point,
spores start to burst; the player scores. "Burgers": the enemy stands in reach lose 0.1 x
the tweak 0x0030ce50 and their owners bear a grudge. Players in reach are stunned or,
after 2 stuns in a row, knocked down; their grudges and hit counts go up. */
void CBbash_Player_Strike(void *self)
{
	float p[4], mark[4], at[4], push[4];
	float d, dx, dy, dz;
	unsigned char *node, *bug;
	void *ring, *victim;
	short smash, range = 0x78, s;
	short hit = 0;
	int k, me;

	PL_S(0x1092) = 1;
	p[0] = 150.0f;
	p[1] = 0.0f;
	p[2] = 0.0f;
	p[3] = 1.0f;
	/* (the original reads `at` uninitialised when "Pied Pipers" turns a swipe into a
	smash; the port zeroes it) */
	memset(at, 0, sizeof at);
	bmVectorRotateY(p, p, PA_F(0x1e8) - 1.5707963f);
	p[0] = p[0] + PA_F(0xd0);
	smash = PL_S(0x12f4);
	p[1] = p[1] + PA_F(0xd4);
	p[2] = p[2] + PA_F(0xd8);
	if (smash == 0) {
		memcpy(p, PA_V(0xd0), 0x10);
		memcpy(mark, p, 0x10);
		FUN_00183ca0(mark, MW_AS(PL_ACTOR, 0x230), PA_F(0x1e8));
	} else {
		memcpy(mark, p, 0x10);
		memcpy(at, p, 0x10);
		FUN_00183e00(mark, (int)MW_AS(PL_ACTOR, 0x230));
	}
	for (k = 0; k < 10; k++) {
		ring = mw_Helpers[k];
		if (MW_AS(ring, 0x10) != 0)
			continue;
		MW_AS(ring, 0x10) = smash == 1 ? 1 : 2;
		MW_AF(ring, 0x18) = PA_F(0x1e8) - 1.5707963f;
		memcpy(ring, p, 0x10);
		MW_AF(ring, 0x14) = smash == 1 ? 100.0f : 300.0f;
		break;
	}
	if (mw_Variation == 3)
		smash = 1;
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
		bug = node - 8;
		if (MW_AS(bug, 0x11ec) == 8)
			continue;
		if (smash == 1) {
			range = 0x78;
			d = bmVectorDistance(p, MW_AV(MW_AP(bug, 0), 0xd0));
			if (MW_AS(bug, 0x11ec) == 2)
				d = d - 50.0f;
			hit = d < 120.0f;
		} else {
			if (smash == 0) {
				unsigned char *ba = (unsigned char *)MW_AP(bug, 0);

				dx = PA_F(0xd0) - MW_AF(ba, 0xd0);
				dy = PA_F(0xd4) - MW_AF(ba, 0xd4);
				dz = PA_F(0xd8) - MW_AF(ba, 0xd8);
				range = 0x140;
				d = CLMath_AngleDistance(CLMath_HeadingTo(PA_V(0xd0), MW_AV(ba, 0xd0)), PA_F(0x1e8));
				hit = !(1.0005071f < d || 320.0f <= sqrtf(dx * dx + dy * dy + dz * dz));
				if (MW_AI(PL_ACTOR, 0x214) == 0 && -1 < MW_AS(bug, 0x11ec) && MW_AS(bug, 0x11ec) < 2) {
					hit = 0;
					continue;
				}
			}
		}
		if (!hit || mw_Variation == 3)
			continue;
		PL_S(0x135a) = 0;
		switch (MW_AS(bug, 0x11ec)) {
		case 0:
		case 5:
			if (MW_AS(bug, 0x11ee) == 3)
				break;
			Actor_PlayAnim(MW_AP(bug, 0), 1, 0, 0, 1.0f, 0.1f, 1);
			pl_ScoreBug(self, bug);
			MW_AS(bug, 0x11ee) = 3;
			MW_AI(bug, 0x11f0) = 0;
			MW_PlaySound(mw_rand() % 6 + 9, 0, 0xff);
			PL_S(0x135a) = 0;
			MW_AS(bug, 0x11fc) = 1;
			MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
			pl_ShowNumber(self, MW_AF(MW_AP(bug, 0), 0xd0), MW_AF(MW_AP(bug, 0), 0xd8), (int)MW_AS(bug, 0x11fe));
			FUN_001842b0(bug, 2);
			break;
		case 1:
		case 6:
			if (MW_AS(bug, 0x11ee) == 3)
				break;
			Actor_PlayAnim(MW_AP(bug, 0), 1, 0, 0, 1.0f, 0.1f, 1);
			pl_ScoreBug(self, bug);
			MW_AS(bug, 0x11ee) = 3;
			MW_AI(bug, 0x11f0) = 0;
			MW_PlaySound(mw_rand() % 6 + 9, 0, 0xff);
			PL_S(0x135a) = 0;
			FUN_001842b0(bug, 4);
			MW_AS(bug, 0x11fc) = 1;
			MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
			pl_ShowNumber(self, MW_AF(MW_AP(bug, 0), 0xd0), MW_AF(MW_AP(bug, 0), 0xd8), (int)MW_AS(bug, 0x11fe));
			MW_StopSound((int)MW_AS(bug, 0x11f6));
			break;
		case 2:
			if (MW_AS(bug, 0x11ee) == 4 || MW_AS(bug, 0x11ee) == 6)
				break;
			MW_AS(bug, 0x11f4) = (short)(MW_AS(bug, 0x11f4) - 1);
			MW_AI(bug, 0x11f0) = 0;
			if (MW_AS(bug, 0x11f4) == 0) {
				MW_AS(bug, 0x11ee) = 4;
				pl_ScoreBug(self, bug);
				MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
				pl_ShowNumber(self, MW_AF(MW_AP(bug, 0), 0xd0), MW_AF(MW_AP(bug, 0), 0xd8), (int)MW_AS(bug, 0x11fe));
				FUN_00184500(bug, 8);
			} else {
				MW_AS(bug, 0x11ee) = 6;
			}
			PL_S(0x135a) = 0;
			if (mw_Variation == 0)
				MW_F(0x002b2d54 + MW_AS(bug, 0x1242) * 0x10) = 1.0f;
			else if (mw_Variation == 1)
				MW_F(0x002b2db4 + MW_AS(bug, 0x1242) * 0x10) = 1.0f;
			break;
		case 3:
			if (MW_AS(bug, 0x11ee) == 4 || MW_AS(bug, 0x11ee) == 3)
				break;
			if (MW_AS(bug, 0x11f4) != 0)
				MW_AS(bug, 0x11f4) = (short)(MW_AS(bug, 0x11f4) - 1);
			MW_AI(bug, 0x11f0) = 0;
			if (MW_AS(bug, 0x11f4) < 1) {
				MW_AS(bug, 0x11ee) = 4;
				pl_ScoreBug(self, bug);
				/* the fly is knocked away; (the original reuses the strike point for
				this vector, so later bugs are tested against it) */
				p[0] = MW_AF(MW_AP(bug, 0), 0xd0) - PA_F(0xd0);
				p[2] = MW_AF(MW_AP(bug, 0), 0xd8) - PA_F(0xd8);
				p[1] = 0.0f;
				MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
				bmVectorSetLength(p, p, 10.0f);
				memcpy(MW_AV(MW_AP(bug, 0), 0x194), p, 0x10);
				pl_ShowNumber(self, MW_AF(MW_AP(bug, 0), 0xd0), MW_AF(MW_AP(bug, 0), 0xd8), (int)MW_AS(bug, 0x11fe));
				FUN_001842b0(bug, 4);
			} else {
				MW_AS(bug, 0x11ee) = 3;
			}
			PL_S(0x135a) = 0;
			MW_PlaySound(mw_rand() % 3 + 0xf, 0, 0xff);
			MW_StopSound((int)MW_AS(bug, 0x11f6));
			break;
		case 4:
			if (MW_AS(bug, 0x11ee) != 2) {
				MW_AS(bug, 0x11ee) = 2;
				MW_AI(bug, 0x11f0) = 0;
				MW_AS(bug, 0x120c) = MW_AS(PL_ACTOR, 0x230);
			}
			break;
		}
	}
	if (PL_I(0x1400) < 0)
		PL_I(0x1400) = 0;
	if (mw_Variation == 2) {
		for (k = 0; k < 4; k++) {
			float *size = MW_FV(0x0030e9f0 + k * 4);

			me = PL_INDEX;
			if (k == me)
				continue;
			if (MW_I(0x0030a278) != 0 && MW_AS(mw_Players[me], 0x12c0) == MW_AS(mw_Players[k], 0x12c0))
				continue;
			if (!(bmVectorDistance(p, MW_FV(0x002b2718 + k * 0x10)) < (float)(int)range))
				continue;
			*size = *size - MW_F(0x0030ce50) * 0.1f;
			if (*size <= 0.0f)
				*size = 0.0f;
			MW_AS(mw_Players[k], 0x13ec + PL_INDEX * 2) = (short)(MW_AS(mw_Players[k], 0x13ec + PL_INDEX * 2) + 3);
			if (-1 < PL_S(0x135c)) {
				void *mine = mw_Players[PL_INDEX];

				MW_AS(mine, 0x13ec + k * 2) = (short)(MW_AS(mine, 0x13ec + k * 2) - 2);
				if (MW_AS(mine, 0x13ec + k * 2) < 1) {
					PL_S(0x135c) = -1;
					PL_S(0x136c) = 0;
					PL_S(0x136a) = 0;
				}
			}
			if (0.0f < *size)
				Splat_AddCrumbs(MW_FV(0x002b2718 + k * 0x10), 0);
		}
	}
	for (k = 0; k < DAT_002bae94; k++) {
		if (MW_I(0x0030a278) != 0 && PL_S(0x12c0) == MW_AS(mw_Players[k], 0x12c0))
			continue;
		if (k == PL_INDEX)
			continue;
		if (smash == 0) {
			unsigned char *va = PL_ACTOR_OF(k);

			dx = PA_F(0xd0) - MW_AF(va, 0xd0);
			dy = PA_F(0xd4) - MW_AF(va, 0xd4);
			dz = PA_F(0xd8) - MW_AF(va, 0xd8);
			s = 0x140;
			if (CLMath_AngleDistance(CLMath_HeadingTo(PA_V(0xd0), MW_AV(va, 0xd0)), PA_F(0x1e8)) <= 1.0005071f)
				d = sqrtf(dx * dx + dy * dy + dz * dz);
			else
				d = 999999.0f;
		} else {
			s = 0x78;
			d = bmVectorDistance(at, MW_AV(PL_ACTOR_OF(k), 0xd0));
		}
		if (!(d < (float)(int)s))
			continue;
		victim = mw_Players[k];
		if (MW_AS(victim, 0x36) == 6 || MW_AS(victim, 0x36) == 7)
			continue;
		push[0] = MW_AF(PL_ACTOR_OF(k), 0xd0) - PA_F(0xd0);
		push[1] = MW_AF(PL_ACTOR_OF(k), 0xd4) - PA_F(0xd4);
		push[2] = MW_AF(PL_ACTOR_OF(k), 0xd8) - PA_F(0xd8);
		bmVectorSetLength(push, push, 25.0f);
		push[1] = 25.0f;
		MW_AF(PL_ACTOR_OF(k), 0x194) = push[0] + MW_AF(PL_ACTOR_OF(k), 0x194);
		MW_AF(PL_ACTOR_OF(k), 0x198) = MW_AF(PL_ACTOR_OF(k), 0x198) + 25.0f;
		MW_AF(PL_ACTOR_OF(k), 0x19c) = push[2] + MW_AF(PL_ACTOR_OF(k), 0x19c);
		me = PL_INDEX;
		if (MW_AS(mw_Players[k], 0x13fc) < 2) {
			MW_AS(mw_Players[k], 0x13fc) = (short)(MW_AS(mw_Players[k], 0x13fc) + 1);
			CBbash_Player_Hit(mw_Players[k], MW_AV(PL_ACTOR_OF(me), 0xd0), 0);
		} else {
			CBbash_Player_Hit(mw_Players[k], MW_AV(PL_ACTOR_OF(me), 0xd0), 1);
			MW_AS(mw_Players[k], 0x13fc) = 0;
		}
		me = PL_INDEX;
		PL_S(0x1086) = (short)k;
		MW_AS(mw_Players[k], 0x1088) = MW_AS(PL_ACTOR, 0x230);
		MW_AS(mw_Players[k], 0x108a + me * 2) = (short)(MW_AS(mw_Players[k], 0x108a + me * 2) + 1);
		MW_AS(mw_Players[k], 0x13ec + me * 2) = (short)(MW_AS(mw_Players[k], 0x13ec + me * 2) + 1);
		MW_AS(mw_Players[me], 0x13f4 + k * 2) = (short)(MW_AS(mw_Players[me], 0x13f4 + k * 2) + 1);
		MW_AS(mw_Players[me], 0x13ec + k * 2) = MW_AS(mw_Players[me], 0x13f4 + k * 2);
		if (mw_rand() % 3 == 0)
			CLSamples_PlayVoice((char)DAT_002baed4[MW_AT(PL_ACTOR_OF(me), 0x230, unsigned short) * 10], 0xb, NULL,
				DAT_00293928, 100.0f, 0, -1, 0);
		s = PL_S(0x135c);
		if (s < 0) {
			if (PL_S(0x136a) == 0 || k != (int)PL_S(0x136c))
				continue;
			s = 0;
			PL_S(0x136a) = 0;
			PL_I(0x1360) = 0;
		} else {
			PL_I(0x1360) = 0;
		}
		PL_S(0x136c) = s;
		PL_S(0x135c) = -1;
		PL_S(0x1368) = 0x79;
	}
	PL_S(0x12e0) = 0;
	PL_S(0x12e2) = 0;
}
