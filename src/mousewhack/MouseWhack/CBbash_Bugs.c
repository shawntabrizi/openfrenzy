/*
MouseWhack/CBbash_Bugs.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CBbash_Bugs.cpp
(module mousewhack)

The bug object: its construction, spawning, frame and painting. 0x00181d50 and
0x00181fc0 are tagged with this file; 0x00181770-0x00181830 are gap functions of the same
class (the bug's constructor/destructor, the model table). The behaviour of each bug
type is in unplaced.c (0x00176250-0x0017ce20). 0x00181730/0x00181740 are the blast rings
(between CBBash_Destructable.cpp and this file; file not known, kept here).

Bug layout (byte offsets; see also unplaced.c): +0x00 its actor, +0x04 its platform
(the train), +0x08 its list node (list 0x0030ee30), +0x10 its route (CLRoute),
+0x1004 short frames to the next wander, +0x1006 short, +0x1008 BActorNode *[5],
+0x101c float[4], +0x102c float[5], +0x1040 float[5], +0x1054..+0x1060 floats,
+0x1064 the spore's cloud model, +0x1070 its instance, +0x11a4 its root node,
+0x11e0/+0x11e4/+0x11e8 the cloud's animations, +0x11ec short type, +0x11ee short state,
+0x11f0 float state time, +0x11f4 short hit points, +0x11f6 short its sound channel,
+0x11f8 short, +0x11fa short alive, +0x11fc short splat when it dies, +0x11fe short its
value, +0x1200/+0x1202 short moving / frames moving, +0x1204 short target player,
+0x1208 float, +0x120c short the player who hit (owns) it, +0x120e short, +0x1210 float
(255), +0x1214 short animation, +0x1216 short, +0x1218 float[4] target, ...,
+0x127c float[4] velocity asked for, +0x128c float[4] velocity after the collision.
Types: 0 worm, 1 roach, 2 tremor, 3 fly, 4 spore, 5 bad worm, 6 bad roach, 7 tremor
rubble, 8 train, 10 worm (enters at the far end).

A blast ring (0x1c bytes, 10 at mw_Helpers): +0 float[4] position, +0x10 short kind (0
free, 1 smash, 2 swipe), +0x12 short frames, +0x14 float radius, +0x18 float angle.
*/

#include "mousewhack.h"

#define BG_S(off) MW_AS(self, off)
#define BG_I(off) MW_AI(self, off)
#define BG_F(off) MW_AF(self, off)
#define BG_V(off) MW_AV(self, off)
#define BG_ACTOR ((unsigned char *)MW_AP(self, 0x0))
#define BA_F(off) MW_AF(BG_ACTOR, off)
#define BA_V(off) MW_AV(BG_ACTOR, off)

static const char mw_BugsFile[] = "C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBbash_Bugs.cpp";

/* 0x00181730  (gap): a blast ring's constructor */
void *MW_Ring_Ctor(void *self)
{
	MW_AS(self, 0x10) = 0;
	MW_AS(self, 0x12) = 0;
	return self;
}

/* 0x00181740  (gap): a blast ring's frame: it lasts 20 frames */
void MW_Ring_Update(void *self)
{
	short kind = MW_AS(self, 0x10);

	if (kind != 0 && (kind == 1 || kind == 2)) {
		MW_AS(self, 0x12) = (short)(MW_AS(self, 0x12) + 1);
		if (0x14 < MW_AS(self, 0x12)) {
			MW_AS(self, 0x12) = 0;
			MW_AS(self, 0x10) = 0;
		}
	}
}

/* 0x00181770  (gap): loads the 9 bug models (table 0x002b4218 of {name, model}) */
void MW_LoadBugModels(void)
{
	int i;

	for (i = 0; i < 9; i++) {
		MW_P(0x002b421c + i * 8) = bLoadResource(NULL, (const char *)MW_P(0x002b4218 + i * 8), 1, 0);
		bkPrintf(MW_P(0x002b421c + i * 8) == NULL ? "Failed to load actor %s\n" : "Loaded actor %s\n",
			(const char *)MW_P(0x002b4218 + i * 8));
	}
}

/* 0x001817c0  (gap): the bug's constructor (its route) */
void *CBbash_Bug_Ctor(void *self)
{
	CLRoute_Ctor((struct CLRoute *)((char *)self + 0x10));
	return self;
}

/* 0x001817d0  (gap): the bug's destructor: the cloud instance, the route */
void CBbash_Bug_Dtor(void *self)
{
	if (BG_I(0x1064) != 0)
		baDeleteActorInstance((struct BActorInstance *)((char *)self + 0x1070));
	CLRoute_Dtor((struct CLRoute *)((char *)self + 0x10));
}

/* pushes the actor out of a circle of radius `r` around (cx, cz) */
static int bg_PushOutOf(void *self, float cx, float cz, float r)
{
	float d[4], dist;

	d[0] = cx - BA_F(0xd0);
	d[2] = cz - BA_F(0xd8);
	d[1] = 0.0f;
	dist = sqrtf(d[0] * d[0] + d[2] * d[2]);
	if (!(dist < r))
		return 0;
	bmVectorSetLength(d, d, r - dist);
	BA_F(0xd0) = BA_F(0xd0) - d[0];
	BA_F(0xd4) = BA_F(0xd4) - d[1];
	BA_F(0xd8) = BA_F(0xd8) - d[2];
	return 1;
}

/* 0x00181830  (gap): a bug's frame: the behaviour of its type, the move (the train's
platform takes its velocity), out of the burger stands ("Burgers") and the players'
bases ("Pied Pipers"), its height above the ground, the collision, the "moving" count,
and death when it leaves the level */
void CBbash_Bug_Update(void *self)
{
	unsigned char *actor;
	int k;

	switch (BG_S(0x11ec)) {
	case 0:
	case 5:
	case 10:
		Bug_Worm_Update(self);
		break;
	case 1:
	case 6:
		Bug_Roach_Update(self);
		break;
	case 2:
		Bug_Tremor_Update(self);
		break;
	case 3:
		Bug_Fly_Update(self);
		break;
	case 4:
		Bug_Spore_Update(self);
		break;
	case 7:
		Bug_Rubble_Update(self);
		break;
	case 8:
		Bug_Train_Update(self);
		break;
	default:
		bkPrintf("Unhandled bug type \n");
	}
	if (BG_S(0x11ec) == 8) {
		memcpy(MW_AV(MW_AP(self, 4), 0x1a4), BG_V(0x127c), 0x10);
	} else {
		actor = BG_ACTOR;
		memcpy(MW_AV(actor, 0x1a4), MW_AV(actor, 0xd0), 0x10);
		memcpy(BG_V(0x127c), MW_AV(actor, 0x194), 0x10);
		MW_AF(actor, 0xd0) = MW_AF(actor, 0x194) + MW_AF(actor, 0xd0);
		MW_AF(actor, 0xd8) = MW_AF(actor, 0x19c) + MW_AF(actor, 0xd8);
		MW_AF(actor, 0xd4) = MW_AF(actor, 0x198) + MW_AF(actor, 0xd4);
		MW_AF(actor, 0xdc) = 1.0f;
	}
	if (mw_Variation == 2) {
		for (k = 0; k < 4; k++) {
			float r = MW_F(0x0030ea00) * MW_F(0x0030e9f0 + k * 4) + MW_AF(MW_AP(BG_ACTOR, 0x18c), 0x9c);

			if (bg_PushOutOf(self, MW_F(0x002b2718 + k * 0x10), MW_F(0x002b2720 + k * 0x10), r))
				BA_F(0xdc) = 1.0f;
		}
	}
	if (BG_S(0x11ec) != 3 && BG_S(0x11ec) != 8)
		BG_F(0x1230) = (float)((double)BA_F(0xd4) - (double)CLColl_GroundProbe(BA_V(0xd0), 30.0f, NULL, NULL, 0));
	if (mw_Variation == 3 &&
	    ((BG_S(0x11ec) == 0 && BG_S(0x11ee) != 4) || (BG_S(0x11ec) == 1 && BG_S(0x11ee) != 5))) {
		for (k = 0; k < 4; k++)
			bg_PushOutOf(self, MW_F(0x002b2d08 + k * 0x10), MW_F(0x002b2d10 + k * 0x10),
				MW_AF(MW_AP(BG_ACTOR, 0x18c), 0x9c) + 150.0f);
	}
	if (BG_S(0x11ec) != 8) {
		if (BG_S(0x11ee) != 0)
			CRColl_MoveActor(BG_ACTOR);
		if (BG_F(0x127c) == BA_F(0x194) && BG_F(0x1284) == BA_F(0x19c)) {
			BG_S(0x1200) = 0;
			BG_S(0x1202) = 0;
		} else {
			BG_S(0x1202) = (short)(BG_S(0x1202) + 1);
			BG_S(0x1200) = 1;
		}
	}
	if (BG_S(0x11ec) < 7) {
		actor = BG_ACTOR;
		if (MW_AF(actor, 0xd0) < -2400.0f || 2400.0f < MW_AF(actor, 0xd0) || MW_AF(actor, 0xd8) < -2400.0f ||
		    2400.0f < MW_AF(actor, 0xd8)) {
			BG_S(0x11fa) = 0;
			if (mw_DbgMessages != 0)
				bkPrintf("bug off level \n");
			BG_S(0x11fc) = 0;
		}
		if (BA_F(0xd4) <= -300.0f) {
			BG_S(0x11fa) = 0;
			if (mw_DbgMessages != 0)
				bkPrintf("bug off level \n");
			BG_S(0x11fc) = 0;
		}
	}
	if (BG_S(0x11ec) != 8)
		memcpy(BG_V(0x128c), BA_V(0x194), 0x10);
}

/* 0x00181d50  CBbash_Bugs.cpp, "Pied Pipers": a paint ball of `player` hits the bug. His
own bug pops (-1 point); another's worm or roach becomes his (his colour's model) and
runs to his base (states 4/5). */
void Bug_PaintTag(void *self, short player)
{
	float keep[4];
	unsigned char *actor;
	int character, time, colour;

	if (MW_AS(self, 0x120c) == player) {
		colour = DAT_002baed4[player * 10] * 0x60;
		baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318), (double)BA_F(0xd0), 340.0,
			(double)BA_F(0xd8), (int)DAT_00290718[colour], (int)DAT_00290718[colour + 1], (int)DAT_00290718[colour + 2],
			"-1", 1.0);
		if (0 < MW_AI(mw_Players[player], 0x1400))
			MW_AI(mw_Players[player], 0x1400) = MW_AI(mw_Players[player], 0x1400) - 1;
		BG_S(0x11fa) = 0;
		BG_S(0x11fc) = 1;
		MW_PlaySound(mw_rand() % 6 + 9, 0, 0xff);
		return;
	}
	BG_S(0x120c) = player;
	character = (short)DAT_002baed4[player * 10];
	actor = BG_ACTOR;
	time = MW_AI(MW_AP(actor, 0x144), 0x74);
	memcpy(keep, MW_AV(actor, 0xd0), 0x10);
	if (BG_S(0x11ec) == 0) {
		baDeleteActorInstance((struct BActorInstance *)(actor + 0x10));
		baInitActorInstance((struct BActorInstance *)(BG_ACTOR + 0x10), (struct BActor *)MW_P(0x0030a634 + character * 4), 8,
			mw_BugsFile, 0x1ca);
		memcpy(BA_V(0xd0), keep, 0x10);
		BG_S(0x11ee) = 4;
	} else {
		if (BG_S(0x11ec) != 1)
			return;
		baDeleteActorInstance((struct BActorInstance *)(actor + 0x10));
		baInitActorInstance((struct BActorInstance *)(BG_ACTOR + 0x10), (struct BActor *)MW_P(0x0030aef0 + character * 4), 8,
			mw_BugsFile, 0x1d6);
		memcpy(BA_V(0xd0), keep, 0x10);
		BG_S(0x11ee) = 5;
	}
	BG_I(0x11f0) = 0;
	Actor_PlayAnim(BG_ACTOR, 0, 1, 0, 1.0f, 0.1f, 1);
	baSetNodeTime((struct BActorInstance *)(BG_ACTOR + 0x10), NULL, time, 1);
}

/* the actor's animations from the name table `names` (0x44 bytes per name), `count` */
static void bg_FindAnims(void *self, unsigned int names, int count)
{
	int i;

	MW_AP(BG_ACTOR, 0x324) = MW_A(names);
	for (i = 0; i < count; i++)
		MW_AP(BG_ACTOR, 0x264 + i * 4) = baFindSubObject((struct BActorInstance *)(BG_ACTOR + 0x10),
			(const char *)MW_AP(BG_ACTOR, 0x324) + i * 0x44);
}

/* the collision of a bug's actor */
static void bg_SetCollision(void *self, float a, float b, float c, float weight)
{
	unsigned char *coll = (unsigned char *)MW_AP(BG_ACTOR, 0x18c);

	MW_AF(coll, 0x9c) = a;
	MW_AI(coll, 0x90) = 0;
	MW_AF(coll, 0x80) = b;
	MW_AF(coll, 0xa0) = c;
	MW_AI(coll, 0x1ac) = 0;
	*(float *)MW_AP(BG_ACTOR, 0x188) = weight;
}

/* 0x00181fc0  CBbash_Bugs.cpp: (re)spawns the bug as `type`: far away (the behaviour
places it), its model, collision, animations, value (+0x11fe) and hit points (+0x11f4)
from the tweaks; the roaches' scuttle sound; the tremor's 5 body nodes; the spore's
cloud. In "Pied Pipers" every bug is worth 10. */
void CBbash_Bug_Spawn(void *self, short type)
{
	float pos[4];
	unsigned char *platform;
	int r1, r2;
	static const char *const tremorNodes[5] = {"base", "neck", "neckb", "head", "headtop"};
	int i;

	r1 = mw_rand();
	r2 = mw_rand();
	pos[1] = -500.0f;
	pos[0] = (float)(r1 % 28000 - 14000);
	pos[2] = (float)(r2 % 28000 - 14000);
	if (mw_DbgMessages != 0)
		bkPrintf("spanwing bug of type %d\n", (int)type);
	BG_S(0x11fa) = 1;
	BG_S(0x120e) = 0;
	BG_S(0x120c) = -1;
	BG_S(0x11fc) = 0;
	BG_I(0) = 0;
	BG_I(4) = 0;
	BG_S(0x11ee) = 0;
	for (i = 0; i < 5; i++)
		BG_I(0x1008 + i * 4) = 0;
	for (i = 0; i < 10; i++)
		BG_I(0x102c + i * 4) = 0;
	BG_I(0x105c) = 0;
	BG_I(0x1060) = 0;
	BG_I(0x1228) = 0;
	BG_I(0x11e0) = 0;
	BG_I(0x11e4) = 0;
	BG_I(0x11e8) = 0;
	BG_I(0x127c) = 0;
	BG_I(0x1280) = 0;
	BG_I(0x1284) = 0;
	BG_F(0x1288) = 1.0f;
	BG_I(0x128c) = 0;
	BG_I(0x1290) = 0;
	BG_I(0x1294) = 0;
	BG_F(0x1298) = 1.0f;
	BG_S(0x1006) = 0;
	BG_I(0x101c) = 0;
	BG_I(0x1020) = 0;
	BG_I(0x1024) = 0;
	BG_F(0x1028) = 1.0f;
	BG_I(0x1054) = 0;
	BG_S(0x11f8) = 0;
	BG_S(0x1204) = (short)(mw_rand() & 3);
	BG_I(0x1208) = 0;
	BG_I(0x1218) = 0;
	BG_I(0x121c) = 0;
	BG_I(0x1220) = 0;
	BG_F(0x1224) = 1.0f;
	BG_I(0x1230) = 0;
	BG_S(0x123a) = 0;
	BG_I(0x1248) = 0;
	BG_I(0x1250) = 0;
	BG_I(0x1274) = 0;
	BG_I(0x1234) = 0;
	BG_F(0x1210) = 255.0f;
	BG_I(0x1254) = 0;
	BG_I(0x1258) = 0;
	BG_I(0x125c) = 0;
	BG_I(0x1260) = 0;
	BG_I(0x1064) = 0;
	pos[3] = 1.0f;
	if (type == 8) {
		pos[2] = 0.0f;
		pos[0] = 0.0f;
		pos[1] = 100.0f;
		MW_AP(self, 4) = Platform_Create("models\\train.obe", pos, 0);
		platform = (unsigned char *)MW_AP(self, 4);
		MW_AS(platform, 0x3fa) = 0;
		MW_AS(MW_AP(self, 4), 0x3da) = 0;
		MW_AS(MW_AP(self, 4), 0x3f8) = 1;
		MW_AS(MW_AP(self, 4), 0x3d8) = 1;
		MW_AU(MW_AP(self, 4), 0x230) = MW_AU(MW_AP(self, 4), 0x230) | 0x400000;
	} else {
		if (type < 10) {
			MW_AP(self, 0) = Actor_Create(MW_P(0x002b421c + type * 8), pos, NULL, 1, 0, 1);
		} else {
			MW_AP(self, 0) = Actor_Create(MW_P(0x002b41cc + type * 8), pos, NULL, 1, 0, 1);
			baDeleteActorInstance((struct BActorInstance *)(BG_ACTOR + 0x10));
			baInitActorInstance((struct BActorInstance *)(BG_ACTOR + 0x10),
				(struct BActor *)bLoadResource(NULL, "models\\badworm.obe", 1, 0xdefa), 8, mw_BugsFile, 0x223);
			memcpy(BA_V(0xd0), pos, 0x10);
		}
		BA_F(0x254) = 2.0f;
		MW_AI(BG_ACTOR, 0x220) = 0x59;
		Actor_InitCharData(BG_ACTOR);
		if (type == 2) {
			MW_AI(BG_ACTOR, 0x220) = 0x49;
			bg_SetCollision(self, 354.0f, 129.8f, 120.0f, 10000.0f);
			MW_AB(MW_AP(BG_ACTOR, 0x18c), 0x15d) = 1;
			MW_AF(MW_AP(BG_ACTOR, 0x18c), 0x9c) = 11.8f;
			MW_AI(MW_AP(BG_ACTOR, 0x18c), 0x90) = 0;
			MW_AF(MW_AP(BG_ACTOR, 0x18c), 0x80) = 11.8f;
			MW_AF(MW_AP(BG_ACTOR, 0x18c), 0xa0) = 12.0f;
			BG_I(0x1278) = 0;
		} else if (type < 7) {
			MW_AI(BG_ACTOR, 0x220) = 0x59;
			bg_SetCollision(self, 118.0f, 118.0f, 60.0f, 1.0f);
		}
	}
	BG_S(0x11f6) = 0;
	switch (type) {
	case 0:
		bg_FindAnims(self, 0x002b4260, 2);
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = MW_S(0x0030ee00);
		BG_S(0x11f4) = MW_S(0x0030ce3c);
		break;
	case 1:
		bg_FindAnims(self, 0x002b43b8, 2);
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = MW_S(0x0030ee04);
		BG_S(0x11f4) = MW_S(0x0030ce3e);
		BG_S(0x11f6) = (short)MW_PlaySound(mw_rand() & 1, 0, 200);
		break;
	case 2:
		bg_FindAnims(self, 0x002b4440, 10);
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = MW_S(0x0030ee0c);
		BG_S(0x11f4) = MW_S(0x0030ce40);
		MW_PlaySound(4, 0, 0xff);
		for (i = 0; i < 5; i++)
			MW_AP(self, 0x1008 + i * 4) = baFindNode((struct BActorNode *)MW_AP(BG_ACTOR, 0x144), tremorNodes[i]);
		for (i = 0; i < 5; i++)
			if (MW_AP(self, 0x1008 + i * 4) == NULL)
				bkPrintf("could not find node %d\n", i);
		for (i = 0; i < 10; i++)
			BG_I(0x102c + i * 4) = 0;
		BG_I(0x105c) = 0;
		BG_I(0x1060) = 0;
		break;
	case 3:
		bg_FindAnims(self, 0x002b4770, 7);
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = MW_S(0x0030ee08);
		BG_I(0x1228) = 0;
		BG_S(0x11f4) = MW_S(0x0030ce42);
		break;
	case 4:
		MW_AI(BG_ACTOR, 0x220) = 0x59;
		bg_SetCollision(self, 80.0f, 0.0f, 80.0f, 1.0f);
		BG_I(0x1228) = 0;
		bg_FindAnims(self, 0x002b42e8, 3);
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = 10;
		MW_AP(self, 0x1064) = bLoadResource(NULL, "models\\sporwngs.obe", 1, 0xdefa);
		baInitActorInstance((struct BActorInstance *)((char *)self + 0x1070), (struct BActor *)MW_AP(self, 0x1064), 3,
			mw_BugsFile, 0x2f7);
		MW_AP(self, 0x11e0) = baFindSubObject((struct BActorInstance *)((char *)self + 0x1070), "buzz");
		MW_AP(self, 0x11e4) = baFindSubObject((struct BActorInstance *)((char *)self + 0x1070), "deathf");
		MW_AP(self, 0x11e8) = baFindSubObject((struct BActorInstance *)((char *)self + 0x1070), "deathb");
		MW_AP(self, 0x1008) = baFindNode((struct BActorNode *)MW_AP(BG_ACTOR, 0x144), "body");
		MW_AP(self, 0x100c) = baFindNode((struct BActorNode *)MW_AP(BG_ACTOR, 0x144), "belly");
		break;
	case 5:
		bg_FindAnims(self, 0x002b4260, 2);
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = -5;
		BG_S(0x11f4) = MW_S(0x0030ce3c);
		break;
	case 6:
		bg_FindAnims(self, 0x002b43b8, 2);
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = -10;
		BG_S(0x11f4) = MW_S(0x0030ce3e);
		BG_S(0x11f6) = (short)MW_PlaySound(mw_rand() & 1, 0, 200);
		break;
	case 7:
		bg_FindAnims(self, 0x002b46e8, 2);
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = 0;
		MW_AI(BG_ACTOR, 0x220) = 0;
		break;
	case 8:
		BG_S(0x11ee) = 0;
		BG_S(0x11fe) = 0;
		break;
	case 10:
		bg_FindAnims(self, 0x002b4260, 2);
		BG_S(0x11ee) = 5;
		BG_S(0x11fe) = MW_S(0x0030ee00);
		BG_S(0x11f4) = MW_S(0x0030ce3c);
		break;
	}
	if (mw_Variation == 3)
		BG_S(0x11fe) = 10;
	BG_S(0x1c) = 1;
	if (type != 8)
		MW_AI(BG_ACTOR, 0x21c) = 3;
	BG_S(0x1244) = 0;
	BG_I(0x127c) = 0;
	BG_I(0x1280) = 0;
	BG_I(0x1284) = 0;
	BG_S(0x123c) = 0;
	BG_S(0x123e) = 0;
	BG_S(0x1238) = 0;
	BG_S(0x122c) = 0;
	BG_S(0x1216) = 0;
	BG_I(0x11f0) = 0;
	BG_I(0x1264) = 0;
	BG_S(0x124e) = 0;
	BG_S(0x124c) = 0;
	BG_I(0x126c) = 0;
	BG_I(0x1270) = 0;
	BG_S(0x1200) = 0;
	BG_S(0x1202) = 0;
	BG_I(0x1278) = 0;
	BG_I(0x1058) = 0;
	BG_S(0x1240) = -1;
	BG_S(0x1242) = -1;
	BG_S(0x1004) = 10;
	BG_S(0x1214) = -1;
	BG_S(0x11ec) = type;
	BG_S(0x1268) = 1;
}
