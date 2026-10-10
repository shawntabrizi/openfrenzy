/*
MouseWhack/CBbash_Bullets.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CBbash_Bullets.cpp
(module mousewhack)

The paint balls of "Pied Pipers" and the tremors' spit balls. 0x00180ce0 and 0x00180de0
are tagged with this file (heap allocation tags); 0x00180720-0x00180c50 and 0x00180f00
are methods of the same classes (gap functions; file inferred from the class).

The manager (mw_Bullets, 0x10 bytes, heap): +0 short, +4 the list of bullets (CLList).
A bullet (0x40 bytes, its own list node): +8 its actor, +0xc float[4] position,
+0x1c float[4] velocity, +0x2c float its gravity, +0x30 short players hit, +0x32
short[] who, +0x3a short flags (1 heavy fall 1.1 a frame, 2 gravity +0x2c, 8 hits
things, 0x10 faces its flight; paint 0x1e, spit 0x1d), +0x3c short gone (1 hit a player,
2 hit a bug, 3 hit the ground), +0x3e short the player who fired it (-1: a tremor).
The 7 bullet models are a table at 0x002b41dc of {name, model}: a paint ball per
character and the spit ball (6).
*/

#include "mousewhack.h"

#define BU_S(off) MW_AS(self, off)
#define BU_I(off) MW_AI(self, off)
#define BU_F(off) MW_AF(self, off)
#define BU_V(off) MW_AV(self, off)
#define BU_ACTOR ((unsigned char *)MW_AP(self, 0x8))

#define BU_LIST(mgr) ((int *)((char *)(mgr) + 4))

static const char mw_BulletsFile[] = "C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBbash_Bullets.cpp";

/* 0x00180720  (gap): the manager's constructor: an empty list, the 7 bullet models */
void *CBbash_Bullets_Ctor(void *self)
{
	int i;

	CLList_Init(BU_LIST(self));
	CLList_Init2(BU_LIST(self));
	BU_S(0) = 0;
	for (i = 0; i < 7; i++) {
		const char *name = (const char *)MW_P(0x002b41dc + i * 8);

		MW_P(0x002b41e0 + i * 8) = bLoadResource(NULL, name, 1, 0);
		bkPrintf(MW_P(0x002b41e0 + i * 8) == NULL ? "Failed to load bullet %s\n" : "Loaded bullet %s\n",
			(const char *)MW_P(0x002b41dc + i * 8));
	}
	return self;
}

/* 0x001807c0  (gap): a bullet's constructor: model `type` at `pos` with `vel` (both get
w = 1), a 50-radius collision */
void *CBbash_Bullet_Ctor(void *self, float *pos, float *vel, short type)
{
	unsigned char *coll;

	memset(self, 0, 0x40);
	MW_AP(self, 8) = Actor_Create(MW_P(0x002b41e0 + type * 8), pos, NULL, 1, 0, 1);
	MW_AF(BU_ACTOR, 0x254) = 2.0f;
	MW_AI(BU_ACTOR, 0x220) = 0x59;
	Actor_InitCharData(BU_ACTOR);
	BU_S(0x3c) = 0;
	BU_S(0x30) = 0;
	pos[3] = 1.0f;
	vel[3] = 1.0f;
	memcpy(BU_V(0xc), pos, 0x10);
	memcpy(BU_V(0x1c), vel, 0x10);
	coll = (unsigned char *)MW_AP(BU_ACTOR, 0x18c);
	MW_AF(coll, 0x9c) = 50.0f;
	MW_AI(coll, 0x90) = 0;
	MW_AF(coll, 0x80) = 80.0f;
	MW_AF(coll, 0xa0) = 50.0f;
	MW_AI(coll, 0x1ac) = 0;
	*(float *)MW_AP(BU_ACTOR, 0x188) = 1.0f;
	return self;
}

/* 0x001808d0  (gap): a bullet's frame (not while paused): gravity; on the ground it is
gone (a paint splat and a sound in "Pied Pipers", a spit splat for a tremor's);
it faces its flight; hitting another player stuns him, hitting a live bug (not a tremor)
paints it for the shooter ("Pied Pipers"); then it moves */
void CBbash_Bullet_Update(void *self)
{
	unsigned short flags;
	float heading, pitch, lean[4];
	unsigned char *coll, *node;
	short k, i;
	int slot;

	if (DAT_002ecdec != 0)
		return;
	flags = (unsigned short)BU_S(0x3a);
	if ((flags & 1) != 0)
		BU_F(0x20) = BU_F(0x20) - 1.1f;
	if ((flags & 2) != 0)
		BU_F(0x20) = BU_F(0x20) - DAT_0028fda8 * BU_F(0x2c);
	if ((flags & 3) != 0 && MW_AS(MW_AP(BU_ACTOR, 0x18c), 100) != 0) {
		/* it touches the ground */
		BU_S(0x3c) = 3;
		if (mw_Variation == 3) {
			BU_F(0x10) = MW_AF(MW_AP(MW_AP(BU_ACTOR, 0x18c), 0), 0x4c) + 1.0f;
			FUN_00184f40(BU_V(0xc), BU_S(0x3e));
			MW_PlaySound((mw_rand() & 3) + 0x27, 0, 0xff);
		}
		if (BU_S(0x3e) == -1)
			FUN_00184150(BU_V(0xc));
	}
	if ((BU_S(0x3a) & 0x10) != 0) {
		heading = mw_Heading(BU_F(0x1c), BU_F(0x24));
		pitch = mw_Heading(sqrtf(BU_F(0x1c) * BU_F(0x1c) + BU_F(0x24) * BU_F(0x24)), BU_F(0x20));
		MW_AF(BU_ACTOR, 0x1e8) = heading;
		lean[0] = (float)((double)1.5707963f - (double)pitch);
		lean[1] = 0.0f;
		lean[2] = 0.0f;
		lean[3] = 0.0f;
		bmVectorRotateY(lean, lean, heading);
		MW_AF(BU_ACTOR, 0x1ec) = lean[2];
		MW_AF(BU_ACTOR, 0x1e4) = lean[0];
	}
	coll = (unsigned char *)MW_AP(BU_ACTOR, 0x18c);
	if ((BU_S(0x3a) & 8) != 0 && BU_S(0x3c) == 0 && MW_AI(coll, 0x160) != 0) {
		/* the actors it touches (up to 7, from +0x160): players */
		for (k = 0; k < 4; k++) {
			for (i = 0; i < 7; i++) {
				void *touched = MW_AP(MW_AP(BU_ACTOR, 0x18c), 0x160 + i * 4);

				if (touched == NULL)
					break;
				if (touched == MW_AP(mw_Players[k], 0x30) && k != BU_S(0x3e)) {
					BU_S(0x32 + BU_S(0x30) * 2) = k;
					BU_S(0x30) = (short)(BU_S(0x30) + 1);
					BU_S(0x3c) = 1;
					CBbash_Player_Hit(mw_Players[k], BU_V(0xc), 0);
					if (BU_S(0x3e) == -1)
						FUN_00184150(BU_V(0xc));
				}
			}
		}
		if (BU_S(0x3c) == 0) {
			/* bugs */
			for (slot = 0x160; slot < 0x160 + 7 * 4; slot += 4) {
				for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
				     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
					unsigned char *bug = node - 8;

					if (MW_AS(bug, 0x11fa) == 0 || MW_AS(bug, 0x11ec) == 2)
						continue;
					if (MW_AP(bug, 0) != MW_AP(MW_AP(BU_ACTOR, 0x18c), slot))
						continue;
					BU_S(0x3c) = 2;
					if (mw_Variation == 3)
						FUN_00181d50(bug, (short)(unsigned short)BU_S(0x3e));
				}
			}
		}
	}
	BU_F(0xc) = DAT_0028fda8 * BU_F(0x1c) + BU_F(0xc);
	BU_F(0x10) = DAT_0028fda8 * BU_F(0x20) + BU_F(0x10);
	BU_F(0x14) = DAT_0028fda8 * BU_F(0x24) + BU_F(0x14);
	memcpy(MW_AV(BU_ACTOR, 0xd0), BU_V(0xc), 0x10);
	memcpy(MW_AV(BU_ACTOR, 0x194), BU_V(0x1c), 0x10);
}

/* 0x00180c50  (gap): the manager's destructor: deletes every bullet and its actor */
void CBbash_Bullets_Dtor(void *self)
{
	int *node, *next;

	node = (int *)(uintptr_t)CLList_Next((unsigned int *)BU_LIST(self), NULL);
	while (node != NULL) {
		next = (int *)(uintptr_t)CLList_Remove(BU_LIST(self), node);
		if (node != NULL) {
			Actor_Delete(MW_AP(node, 8));
			MW_DELETE(node);
		}
		node = (int *)(uintptr_t)CLList_Next((unsigned int *)BU_LIST(self), (unsigned int *)next);
	}
	FUN_001d5950();   /* the list's (empty) destructor */
}

/* 0x00180ce0  CBbash_Bullets.cpp: player `player` fires a paint ball of his colour from
`from` along `dir`; its gravity makes it land about where it was aimed */
void CBbash_Bullets_Fire(void *self, float *from, float *dir, short player)
{
	float height, frames;
	short character;
	void *bullet;

	height = from[1] - MW_AF(MW_AP(mw_Players[player], 0x30), 0xd4);
	character = (short)DAT_002baed4[player * 10];
	dir[3] = 1.0f;
	from[3] = 1.0f;
	frames = 2000.0f / sqrtf(dir[2] * dir[2] + dir[1] * dir[1] + dir[0] * dir[0]);
	bullet = bkHeapAlloc(0x40, mw_BulletsFile, 0x77, 0x1001);
	if (bullet == NULL)
		bullet = NULL;
	else
		bullet = CBbash_Bullet_Ctor(bullet, from, dir, character);
	MW_AF(bullet, 0x2c) = (height * 0.5f) / frames;
	CLList_Insert(BU_LIST(self), (int *)bullet, 0);
	MW_AS(bullet, 0x3e) = player;
	MW_AS(bullet, 0x3a) = 0x1e;
}

/* 0x00180de0  CBbash_Bullets.cpp: a tremor spits a spit ball (model 6) from `from` at
`to`: 30 a frame, lifted to arc over the distance */
void CBbash_Bullets_Spit(void *self, float *from, float *to)
{
	float vel[4];
	float lift;
	void *bullet;

	lift = sqrtf((from[1] - to[1]) * (from[1] - to[1]) + (from[0] - to[0]) * (from[0] - to[0]) +
		(from[2] - to[2]) * (from[2] - to[2])) * 0.033333335f * 0.5f * 1.1f;
	vel[0] = to[0] - from[0];
	vel[1] = to[1] - from[1];
	vel[2] = to[2] - from[2];
	bmVectorSetLength(vel, vel, 30.0f);
	vel[1] = vel[1] + lift;
	vel[3] = 1.0f;
	bullet = bkHeapAlloc(0x40, mw_BulletsFile, 0xa0, 0x1001);
	if (bullet == NULL)
		bullet = NULL;
	else
		bullet = CBbash_Bullet_Ctor(bullet, from, vel, 6);
	CLList_Insert(BU_LIST(self), (int *)bullet, 0);
	MW_AS(bullet, 0x3a) = 0x1d;
	MW_AS(bullet, 0x3e) = -1;
}

/* 0x00180f00  (gap): the bullets' frame: each one moves; the ones that are gone are
deleted */
void CBbash_Bullets_Update(void *self)
{
	int *node, *next;

	node = (int *)(uintptr_t)CLList_Next((unsigned int *)BU_LIST(self), NULL);
	while (node != NULL) {
		CBbash_Bullet_Update(node);
		next = node;
		if (MW_AS(node, 0x3c) != 0) {
			next = (int *)(uintptr_t)CLList_Remove(BU_LIST(self), node);
			Actor_Delete(MW_AP(node, 8));
			MW_DELETE(node);
		}
		node = (int *)(uintptr_t)CLList_Next((unsigned int *)BU_LIST(self), (unsigned int *)next);
	}
}
