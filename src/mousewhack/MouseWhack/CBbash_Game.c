/*
MouseWhack/CBbash_Game.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CBbash_Game.cpp
(module mousewhack)

CBbash_Game: the rules of Bug Bash (one static object at 0x0030eb60, no constructor): the
intro camera, the game frame (spawning bugs, hits, scores, the round timer), the follow
camera and the outro that shows the winners. 0x00173f80 and 0x00174610 are tagged with
this file; the functions from 0x001729a0 on are methods of the same object (gap functions;
file inferred from the class).

Game layout (byte offsets from 0x0030eb60):
  +0x08 float[4] camera target, +0x18 float[4] camera eye
  +0x28 float    outro: time on the current winner
  +0x2c float    outro: time since the outro started
  +0x30 short    outro: the winner shown (index into the arena's leaders)
  +0x32 short    state (mw_GameState): 0 outro shows a winner, 1 intro / turning to a
                 winner, 2 fading out, ...
  +0x34 float    the fade (0..255)
  +0x38 short    the winner's celebration started
  +0x48 float[4][4] the players' positions saved while the arena is drawn
*/

#include "mousewhack.h"

#define GM_S(off) MW_AS(self, off)
#define GM_I(off) MW_AI(self, off)
#define GM_F(off) MW_AF(self, off)
#define GM_V(off) MW_AV(self, off)

/* core: the mode stack, the player records (+4: a human plays this slot), tournament mode */
extern unsigned char DAT_002ba750[];
extern int DAT_002baed8[];
extern int DAT_002bae98;

/* foxhunt 0x001334f0: the heading of a direction (dx, -dz) */
#define gm_Heading(dx, dz) ((float (*)(float, float))FUN_001334f0)((dx), (dz))

/* 0x001729a0  (gap): the follow camera of "Brawl"/"Burgers": above the middle of the
active players, far enough to see them all; the players' stick directions follow it */
void CBbash_Game_FollowCamera(void *self)
{
	float maxX, minX, maxZ, minZ, width, cx, cz, heading;
	unsigned char *actor;
	short i;
	int k;

	(void)self;
	actor = (unsigned char *)MW_AP(mw_Players[0], 0x30);
	maxX = MW_AF(actor, 0xd0);
	minX = maxX;
	maxZ = MW_AF(actor, 0xd8);
	minZ = maxZ;
	for (k = 0; k < 4; k++) {
		if (MW_AS(mw_Players[k], 0x10b8) != 0)
			continue;
		actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
		if (maxX < MW_AF(actor, 0xd0))
			maxX = MW_AF(actor, 0xd0);
		if (maxZ < MW_AF(actor, 0xd8))
			maxZ = MW_AF(actor, 0xd8);
		if (MW_AF(actor, 0xd0) < minX)
			minX = MW_AF(actor, 0xd0);
		if (MW_AF(actor, 0xd8) < minZ)
			minZ = MW_AF(actor, 0xd8);
	}
	width = (maxX - minX) * 0.8f;
	if (width < 1100.0f)
		width = 1100.0f;
	cx = (maxX + minX) * 0.5f;
	cz = (minZ + maxZ) * 0.5f;
	MW_AF(mw_Camera, 0x534) = cx * 0.125f;
	MW_AF(mw_Camera, 0x538) = MW_F(0x002b29bc);
	MW_AF(mw_Camera, 0x53c) = -(width + 450.0f);
	MW_AF(mw_Camera, 0x544) = cx * 0.5f;
	MW_AF(mw_Camera, 0x548) = 0.0f;
	MW_AF(mw_Camera, 0x54c) = cz * 0.5f;
	heading = gm_Heading(MW_F(0x002b29b8) - MW_F(0x0030eb40), -(MW_F(0x002b29c0) - MW_F(0x0030eb48)));
	for (i = 0; (int)i < DAT_002bae94; i++)
		MW_AF(mw_Players[i], 0x1118) = heading;
	bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
}

/* 0x00172b80  (gap): the mode was pushed: character voices and the packages (the level
package Packages\BugBash and the variation's Packages\Bug-V<n>; "Pied Pipers" uses V1) */
void CBbash_Game_LoadPackages(void *self, int unused)
{
	(void)self;
	(void)unused;
	Core_LoadCharacterVoices(1);
	bkPrintf("bug bash pushed variation %d\n", (int)mw_Variation);
	mw_LevelPackage = ((void *(*)(const char *, int))FUN_00071f90)("Packages\\BugBash", 0);
	if (mw_LevelPackage == NULL)
		bkPrintf("MouseWhack pak file error ***\n");
	switch (mw_Variation) {
	case 0:
		mw_VariationPackage = ((void *(*)(const char *, int))FUN_00071f90)("Packages\\Bug-V1", 0);
		if (mw_VariationPackage == NULL)
			bkPrintf("BugBash variation1 pak file error ***\n");
		break;
	case 1:
		mw_VariationPackage = ((void *(*)(const char *, int))FUN_00071f90)("Packages\\Bug-V2", 0);
		if (mw_VariationPackage == NULL)
			bkPrintf("BugBash variation2 pak file error ***\n");
		break;
	case 2:
		mw_VariationPackage = ((void *(*)(const char *, int))FUN_00071f90)("Packages\\Bug-V3", 0);
		if (mw_VariationPackage == NULL)
			bkPrintf("BugBash variation3 pak file error ***\n");
		break;
	case 3:
		mw_VariationPackage = ((void *(*)(const char *, int))FUN_00071f90)("Packages\\Bug-V1", 0);
		if (mw_VariationPackage == NULL)
			bkPrintf("BugBash variation4 pak file error ***\n");
		break;
	}
}

/* 0x00172c90  (gap): the follow camera of "Tremors"/"Pied Pipers": the smoothed player
positions (0x0030ea30, at most 40 units a frame once the game runs) framed like
CBbash_Game_FollowCamera */
void CBbash_Game_FollowCameraSmooth(void *self)
{
	float delta[4];
	float maxX, minX, maxZ, minZ, width, cx, cz, heading;
	float *smooth;
	unsigned char *actor;
	short i;
	int k;

	(void)self;
	if (MW_S(0x0030ce00) == 0) {
		for (k = 0; k < 4; k++)
			memcpy(MW_FV(0x0030ea30 + k * 0x10), MW_AV(MW_AP(mw_Players[k], 0x30), 0xd0), 0x10);
	} else {
		for (k = 0; k < 4; k++) {
			smooth = MW_FV(0x0030ea30 + k * 0x10);
			actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
			delta[0] = smooth[0] - MW_AF(actor, 0xd0);
			delta[1] = smooth[1] - MW_AF(actor, 0xd4);
			delta[2] = smooth[2] - MW_AF(actor, 0xd8);
			if (sqrtf(delta[2] * delta[2] + delta[1] * delta[1] + delta[0] * delta[0]) <= 40.0f) {
				memcpy(smooth, MW_AV(actor, 0xd0), 0x10);
			} else {
				bmVectorSetLength(delta, delta, 40.0f);
				smooth[0] = smooth[0] - delta[0];
				smooth[1] = smooth[1] - delta[1];
				smooth[2] = smooth[2] - delta[2];
			}
		}
	}
	maxX = MW_F(0x0030ea30);
	minX = maxX;
	maxZ = MW_F(0x0030ea38);
	minZ = maxZ;
	for (k = 0; k < 4; k++) {
		smooth = MW_FV(0x0030ea30 + k * 0x10);
		if (MW_AS(mw_Players[k], 0x12d8) != 0)
			continue;
		if (maxX < smooth[0])
			maxX = smooth[0];
		if (maxZ < smooth[2])
			maxZ = smooth[2];
		if (smooth[0] < minX)
			minX = smooth[0];
		if (smooth[2] < minZ)
			minZ = smooth[2];
	}
	width = (maxX - minX) * 1.1f;
	if (width < 1800.0f)
		width = 1800.0f;
	cx = (maxX + minX) * 0.5f;
	cz = (maxZ + minZ) * 0.5f;
	MW_AF(mw_Camera, 0x534) = cx * 0.125f;
	MW_AF(mw_Camera, 0x538) = MW_F(0x002b29cc);
	MW_AF(mw_Camera, 0x53c) = -width - 700.0f;
	if (MW_AF(mw_Camera, 0x53c) < -3150.0f)
		MW_AF(mw_Camera, 0x53c) = -3150.0f;
	MW_AF(mw_Camera, 0x544) = cx * 0.5f;
	MW_AF(mw_Camera, 0x548) = 0.0f * 0.5f;
	MW_AF(mw_Camera, 0x54c) = cz * 0.5f;
	heading = gm_Heading(MW_F(0x002b29c8) - MW_F(0x0030eb50), -(MW_F(0x002b29d0) - MW_F(0x0030eb58)));
	for (i = 0; (int)i < DAT_002bae94; i++)
		MW_AF(mw_Players[i], 0x1118) = heading;
	bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
}

/* 0x00172fd0  (gap): the outro camera looks at the winner shown (+0x30): 500 up and 500
in front of him */
void CBbash_Game_LookAtWinner(void *self)
{
	float eye[4];
	unsigned char *actor;
	short winner;

	GM_F(0x28) = 0.0f;
	winner = MW_AS(mw_Arena, 0xd8 + GM_S(0x30) * 2);
	eye[3] = 0.0f;
	eye[0] = 0.0f;
	eye[1] = 500.0f;
	eye[2] = 500.0f;
	bmVectorRotateY(eye, eye, MW_AF(MW_AP(mw_Players[winner], 0x30), 0x1e8));
	actor = (unsigned char *)MW_AP(mw_Players[winner], 0x30);
	eye[0] = eye[0] + MW_AF(actor, 0xd0);
	eye[1] = eye[1] + MW_AF(actor, 0xd4);
	eye[2] = eye[2] + MW_AF(actor, 0xd8);
	memcpy(GM_V(0x8), MW_AV(actor, 0xd0), 0x10);
	GM_F(0xc) = GM_F(0xc) + 150.0f;
	GM_F(0x20) = eye[2];
	GM_F(0x24) = eye[3];
	GM_F(0x18) = eye[0];
	GM_F(0x1c) = eye[1];
	bkPrintf("cam looking at player %d\n", (int)winner);
	bdSetCameraLookAt(GM_V(0x18), GM_V(0x8), 0.0f);
}

/* 0x001730e0  (gap): before the arena is drawn: saves the players' positions (+0x48) and
squashes them out of the way (y x 0.1; in "Pied Pipers" also x x 0.5 and z + 200 if z > 0) */
void CBbash_Game_HidePlayers(void *self)
{
	unsigned char *actor;
	int k;

	for (k = 0; k < 4; k++) {
		actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
		memcpy(GM_V(0x48 + k * 0x10), MW_AV(actor, 0xd0), 0x10);
		MW_AF(actor, 0xd4) = MW_AF(actor, 0xd4) * 0.1f;
		if (mw_Variation == 3) {
			if (0.0f < MW_AF(actor, 0xd8))
				MW_AF(actor, 0xd8) = MW_AF(actor, 0xd8) + 200.0f;
			MW_AF(actor, 0xd0) = MW_AF(actor, 0xd0) * 0.5f;
		}
	}
}

/* 0x00173190  (gap): restores the positions saved by CBbash_Game_HidePlayers */
void CBbash_Game_RestorePlayers(void *self)
{
	int k;

	for (k = 0; k < 4; k++)
		memcpy(MW_AV(MW_AP(mw_Players[k], 0x30), 0xd0), GM_V(0x48 + k * 0x10), 0x10);
}

/* 0x001731d0  (gap): starts the outro: finds the winners, puts the players back on their
start points facing the middle, kills the bugs' animations and turns to the first winner */
void CBbash_Game_StartOutro(void *self)
{
	unsigned char *node, *actor;
	float *start;
	float heading;
	int k;

	if (MW_S(0x0030a278) == 0)
		CBbash_Arena_FindLeaders(mw_Arena);
	else
		CBbash_Arena_FindLeadingTeam(mw_Arena);
	GM_S(0x30) = 0;
	GM_I(0x28) = 0;
	for (k = 0; k < 4; k++) {
		start = MW_FV(0x002b2718 + k * 0x10);
		actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
		MW_AF(actor, 0xd0) = start[0] <= 0.0f ? start[0] + 160.0f : start[0] - 160.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xd4) = 0.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xd8) = start[2] <= 0.0f ? start[2] + 160.0f : start[2] - 160.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe0) = 1.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe4) = 1.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe8) = 1.0f;
	}
	AI_TickAll();
	Actor_UpdateAnimations();
	for (k = 0; k < 4; k++) {
		actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
		baFlushAnimations(actor + 0x10, (struct BActorNode *)MW_AP(actor, 0x144), 1, 0);
		Actor_PlayAnim(MW_AP(mw_Players[k], 0x30), 0, 0, 0, 1.0f, 0.0f, 1);
	}
	Actor_UpdateAnimations();
	for (k = 0; k < 4; k++) {
		MW_VFN(mw_Players[k], 0x44, void (*)(void *, int))(mw_Players[k], 0);
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe0) = 1.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe4) = 1.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe8) = 1.0f;
	}
	Actor_UpdateAnimations();
	FUN_00182c10();
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
		if (MW_AS(node, 0x11e4) != 8 && MW_AS(node, 0x11e4) != 7) {
			actor = (unsigned char *)MW_AP(node - 8, 0x0);
			baChangeAnimation((struct BActorNode *)MW_AP(actor, 0x144), 1, 1, 1, 0.0f, 0.0f, 7);
			MW_AF(MW_AP(node - 8, 0x0), 0xd4) = MW_AF(MW_AP(node - 8, 0x0), 0xd4) - 2000.0f;
			MW_AF(MW_AP(node - 8, 0x0), 0xd0) = MW_AF(MW_AP(node - 8, 0x0), 0xd0) - 2000.0f;
		}
	}
	for (k = 0; k < 4; k++) {
		actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
		heading = gm_Heading(MW_AF(actor, 0xd0), -MW_AF(actor, 0xd8));
		heading = (3.1415925f - heading) + 3.1415925f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0x1e8) = heading;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0x1d8) = heading;
		actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
		CLQuat_FromEuler(MW_AV(actor, 0xf0), MW_AV(actor, 0x1e4), 3.1415925f);
		baPrepActorInstance((struct BActorInstance *)((char *)MW_AP(mw_Players[k], 0x30) + 0x10), 1);
	}
	GM_S(0x38) = 0;
	GM_S(0x32) = 1;
	CBbash_Game_LookAtWinner(self);
}

/* a player pressed A or B (pad objects +0x68, +0x6c): skip to the fade out */
static void gm_SkipIfPressed(void *self, void *player)
{
	if (MW_AI(MW_AP(player, 0x68), 0x14) != 0 || MW_AI(MW_AP(player, 0x6c), 0x14) != 0) {
		GM_S(0x32) = 2;
		MW_AI(MW_AP(player, 0x68), 0x14) = 0;
		MW_AI(MW_AP(player, 0x6c), 0x14) = 0;
		GM_I(0x34) = 0;
	}
}

/* 0x00173470  (gap): the outro: shows each winner in turn (4 s, 8 s for a single winner in
a tournament), lets a player skip (any player if all winners are computer players, else
the winners), fades out and leaves the game (or starts a new round in tournament mode 2) */
void CBbash_Game_Outro(void *self)
{
	float limit;
	short winners;
	int allComputer, k;

	if (DAT_002ecdec == 0 && mw_DbgHoldOutro == 0)
		GM_F(0x28) = MW_F(0x0030a270) + GM_F(0x28);
	winners = 0;
	for (k = 0; k < 4; k++) {
		if (MW_AS(mw_Arena, 0xd8 + k * 2) != -1)
			winners++;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe0) = 1.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe4) = 1.0f;
		MW_AF(MW_AP(mw_Players[k], 0x30), 0xe8) = 1.0f;
	}
	allComputer = 1;
	limit = 4000.0f;
	if (DAT_002bae98 == 1 && winners == 1)
		limit = 8000.0f;
	GM_F(0x2c) = DAT_0028fda8 + GM_F(0x2c);
	if (GM_F(0x2c) > 120.0f && GM_S(0x32) != 2 && DAT_002ecdec == 0) {
		for (k = 0; k < 4; k++) {
			short winner = MW_AS(mw_Arena, 0xd8 + k * 2);

			if (winner == -1)
				break;
			if (DAT_002baed8[winner * 10] != 0)
				allComputer = 0;
		}
		if (allComputer != 0) {
			for (k = 0; k < 4; k++)
				gm_SkipIfPressed(self, mw_Players[k]);
		} else {
			for (k = 0; k < 4; k++) {
				short winner = MW_AS(mw_Arena, 0xd8 + k * 2);

				if (winner == -1)
					break;
				gm_SkipIfPressed(self, mw_Players[winner]);
			}
		}
	}
	switch (GM_S(0x32)) {
	case 0:
		bdSetCameraLookAt(GM_V(0x18), GM_V(0x8), 0.0f);
		if (limit <= GM_F(0x28)) {
			MW_S(0x0030eb90) = MW_S(0x0030eb90) + 1;
			GM_I(0x28) = 0;
			if (MW_S(0x0030eb90) == winners) {
				GM_S(0x32) = 2;
				GM_I(0x34) = 0;
			} else {
				GM_S(0x32) = 1;
				GM_S(0x38) = 0;
				CBbash_Game_LookAtWinner(self);
			}
		}
		break;
	case 1:
		bdSetCameraLookAt(GM_V(0x18), GM_V(0x8), 0.0f);
		if (GM_S(0x38) == 1) {
			void *actor = MW_AP(mw_Players[MW_AS(mw_Arena, 0xd8 + MW_S(0x0030eb90) * 2)], 0x30);

			Actor_PlayAnim(actor, 0x1b, 0, 0, 1.0f, 0.3f, 1);
			Actor_PlayAnim(MW_AP(mw_Players[MW_AS(mw_Arena, 0xd8 + MW_S(0x0030eb90) * 2)], 0x30), 0x23, 1, 1, 1.0f,
				0.5f, 1);
			GM_S(0x32) = 0;
		}
		break;
	case 2:
		bdSetCameraLookAt(GM_V(0x18), GM_V(0x8), 0.0f);
		if (DAT_002ecdec == 0)
			GM_F(0x34) = DAT_0028fda8 * 8.5f + GM_F(0x34);
		if (!(GM_F(0x34) < 255.0f)) {
			MW_F(0x0030cde8) = 5000.0f;
			MW_S(0x0030ce00) = 0;
			CLHud_DeleteWinnerText((int)(uintptr_t)DAT_002ecfac);
			if (DAT_002bae98 != 2) {
				CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, 1);
				mw_Quit = 1;
				return;
			}
			FUN_00172730(mw_Arena);
			return;
		}
		break;
	}
	for (k = 0; k < 4; k++) {
		MW_AI(MW_AP(mw_Players[k], 0x68), 0x14) = 0;
		MW_AI(MW_AP(mw_Players[k], 0x6c), 0x14) = 0;
		MW_AI(MW_AP(mw_Players[k], 0x60), 0x14) = 0;
		MW_AI(MW_AP(mw_Players[k], 0x64), 0x14) = 0;
	}
	AI_TickAll();
	Actor_UpdateAll();
	if (DAT_002ecdec != 0)
		return;
	Actor_UpdateAnimations();
}

/* the train hit a player: he respawns and loses the train damage, shown as a floating
"-<n>" in his colour */
static void gm_TrainHit(void *player)
{
	char text[36];
	unsigned char *actor;

	MW_AI(player, 0x1124) = 1;
	MW_VFN(player, 0x24, void (*)(void *))(player);
	MW_AI(player, 0x1400) = MW_AI(player, 0x1400) - MW_I(0x0030ee20);
	if (MW_AI(player, 0x1400) < 0)
		MW_AI(player, 0x1400) = 0;
	sprintf(text, "%d", -MW_I(0x0030ee20));
	actor = (unsigned char *)MW_AP(player, 0x30);
	baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318), (double)MW_AF(actor, 0xd0), 340.0,
		(double)MW_AF(actor, 0xd8), (int)MW_AS(player, 0x136e), (int)MW_AS(player, 0x1370), (int)MW_AS(player, 0x1372),
		text, 1.0);
}

/* 0x00173970  (gap; thiscall, `self` unused): "Tremors": the train (the active bug of type
8) pushes the players along; a player pushed past the end of the platform is hit. `out`
receives the train position. */
void CBbash_Game_TrainPush(void *self, float *out, int unused)
{
	float frontMax, frontMin, top, zMax, zMin, radius, x, y, z, w, nz;
	unsigned char *node, *train, *platform, *actor;
	short direction;
	int k;

	(void)self;
	(void)unused;
	train = NULL;
	node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL);
	if (node == NULL)
		return;
	do {
		if (MW_AS(node, 0x11f2) != 0 && MW_AS(node - 8, 0x11ec) == 8)
			train = node - 8;
		node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node);
	} while (node != NULL);
	if (train == NULL)
		return;
	platform = (unsigned char *)MW_AP(train, 0x4);
	out[0] = MW_AF(platform, 0xe0);
	frontMax = out[0] + 2240.0f;
	out[1] = MW_AF(platform, 0xe4);
	out[2] = MW_AF(platform, 0xe8);
	frontMin = out[0] - 2240.0f;
	out[3] = MW_AF(platform, 0xec);
	direction = 1;
	top = out[1] + 180.0f;
	zMax = out[2] + 280.0f;
	zMin = out[2] - 280.0f;
	if (MW_AF(MW_AP(train, 0x4), 0x1a4) <= 0.0f)
		direction = -1;
	for (k = 0; k < 4; k++) {
		actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
		y = MW_AF(actor, 0xd4);
		z = MW_AF(actor, 0xd8);
		w = MW_AF(actor, 0xdc);
		radius = MW_AF(MW_AP(actor, 0x18c), 0xa0);
		if (direction == 1) {
			x = MW_AF(actor, 0xd0) - radius;
			if (!(x < frontMax && frontMax - 200.0f < x && y - radius < top))
				continue;
			if (zMax <= z || z <= zMin) {
				if (z - radius < zMax && zMin < z + radius) {
					nz = zMax <= z ? z + 4.0f : z - 4.0f;
					MW_AF(actor, 0xd0) = radius + frontMax;
					MW_AF(actor, 0xd4) = y;
					MW_AF(actor, 0xd8) = nz;
					MW_AF(actor, 0xdc) = w;
					if (1700.0f <= radius + frontMax)
						gm_TrainHit(mw_Players[k]);
				}
			} else {
				MW_AF(actor, 0xd4) = y;
				MW_AF(actor, 0xd0) = radius + frontMax;
				MW_AF(actor, 0xdc) = w;
				MW_AF(actor, 0xd8) = z;
				if (1700.0f <= radius + frontMax)
					gm_TrainHit(mw_Players[k]);
			}
		} else {
			x = MW_AF(actor, 0xd0) + radius;
			if (!(frontMin < x && x < frontMin + 200.0f && y - radius < top))
				continue;
			if (zMax <= z || z <= zMin) {
				if (z - radius < zMax && zMin < z + radius) {
					x = frontMin - radius;
					nz = zMax <= z ? z + 4.0f : z - 4.0f;
					MW_AF(actor, 0xd0) = x;
					MW_AF(actor, 0xd4) = y;
					MW_AF(actor, 0xd8) = nz;
					MW_AF(actor, 0xdc) = w;
					if (x <= -1440.0f)
						gm_TrainHit(mw_Players[k]);
				}
			} else {
				x = frontMin - radius;
				MW_AF(actor, 0xd4) = y;
				MW_AF(actor, 0xd0) = x;
				MW_AF(actor, 0xdc) = w;
				MW_AF(actor, 0xd8) = z;
				if (x <= -1440.0f)
					gm_TrainHit(mw_Players[k]);
			}
		}
	}
}

static const char s_GameFile[] = "C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBbash_Game.cpp";

/* new bug (0x12a0 bytes, 0x001817c0) put in the bug list and spawned as `type` */
static void *gm_NewBug(int line, short type)
{
	void *bug = bkHeapAlloc(0x12a0, s_GameFile, line, 0x1001);

	if (bug != NULL)
		bug = FUN_001817c0(bug);
	CLList_Insert((int *)DAT_0030ee30, (int *)((char *)bug + 8), 0);
	FUN_00181fc0(bug, type);
	return bug;
}

/* a new bug of type 2 (a fly?) gets a partner of type 7 at the same slot (+0x1242) if
the slot is free (0x0030ebf8) */
static void gm_SpawnPartner(void *bug, int line)
{
	void *partner;
	short slot;

	FUN_00181830(bug);
	slot = MW_AS(bug, 0x1242);
	if (MW_S(0x0030ebf8 + slot * 2) == 0) {
		partner = gm_NewBug(line, 7);
		MW_AS(partner, 0x1242) = slot;
	}
}

/* a random new bug of the arena's mix */
static void gm_SpawnRandomBug(int line, int partnerLine)
{
	void *bug;
	int type, index;

	type = CBbash_Arena_DecideBugType(mw_Arena);
	index = type;
	if (type == 5)
		index = 0;
	else if (type == 6)
		index = 1;
	MW_S(0x0030ce30 + index * 2) = MW_S(0x0030ce30 + index * 2) + 1;
	bug = gm_NewBug(line, (short)type);
	MW_S(0x0030ce60) = MW_S(0x0030ce60) + 1;
	if (type == 2)
		gm_SpawnPartner(bug, partnerLine);
}

/* a player was hit: he respawns and loses `damage` points, shown as a floating number in
his colour */
static void gm_HitPlayer(void *player, int damage)
{
	char text[32];
	unsigned char *actor;

	MW_AI(player, 0x1124) = 1;
	MW_VFN(player, 0x24, void (*)(void *))(player);
	MW_AI(player, 0x1400) = MW_AI(player, 0x1400) + damage;
	if (MW_AI(player, 0x1400) < 0)
		MW_AI(player, 0x1400) = 0;
	sprintf(text, "%d", damage);
	actor = (unsigned char *)MW_AP(player, 0x30);
	baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318), (double)MW_AF(actor, 0xd0), 340.0,
		(double)MW_AF(actor, 0xd8), (int)MW_AS(player, 0x136e), (int)MW_AS(player, 0x1370), (int)MW_AS(player, 0x1372),
		text, 1.0);
}

/* the camera height of "Brawl" (+300) and "Pied Pipers" (+200) while looking */
static void gm_LookWithLift(void)
{
	float lift;

	if (mw_Variation == 3)
		lift = 200.0f;
	else if (mw_Variation == 0)
		lift = 300.0f;
	else
		return;
	MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) + lift;
	bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
	MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) - lift;
}

/* 0x00173f80  MouseWhack/CBbash_Game.cpp: the intro (phase 0): the intro camera flight
(skipped by a human player's A/B after 10 frames), then the countdown; the camera blends
from the intro camera to the game camera */
void CBbash_Game_Intro(void *self)
{
	unsigned char *node;
	float lift;
	short status;
	int k;

	DAT_00293920 = 1.0f;
	if (MW_S(0x0030ce02) == 0) {
		if (GM_F(0x34) <= 0.0f)
			GM_I(0x34) = 0;
		else
			GM_F(0x34) = GM_F(0x34) - DAT_0028fda8 * 8.5f;
		gm_LookWithLift();
		if (mw_Variation == 2)
			CBbash_Game_FollowCamera(self);
		if (mw_Variation == 1) {
			CBbash_Game_FollowCameraSmooth(self);
			if (MW_S(0x0030ce76) == 0) {
				for (k = 0; k < 5; k++)
					gm_NewBug(0x11f, 10);
				MW_S(0x0030ce76) = 1;
			}
			for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
			     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
				if (MW_AS(node, 0x11f2) != 0)
					FUN_00181830(node - 8);
			}
		}
		status = (short)CLIntroCam_Update(DAT_0028fda8, 0);
		if (status != 0) {
			CLIntroCam_GetPosition(MW_FV(0x0030a2f0), MW_F(0x0030a314));
			CLIntroCam_GetTarget(MW_FV(0x0030a300), MW_F(0x0030a314));
		}
		MW_F(0x0030a314) = MW_F(0x0030a314) + DAT_0028fda8;
		if (status == -1)
			bkPrintf("Error with intro cam!\n");
		if (mw_Variation == 0 || mw_Variation == 3)
			MW_F(0x0030a400) = MW_F(0x0030a400) + 24.0f;
		for (k = 0; k < 4; k++) {
			if (DAT_002baed8[k * 10] != 0 && 10.0f <= MW_F(0x0030a314)) {
				if (MW_AI(MW_AP(mw_Players[k], 0x68), 0x14) != 0) {
					status = 1;
					MW_F(0x0030a310) = 1.0f;
				}
				if (MW_AI(MW_AP(mw_Players[k], 0x6c), 0x14) != 0) {
					status = 1;
					MW_F(0x0030a310) = 1.0f;
				}
			}
		}
		if (DAT_002baeb8 != 0)
			MW_F(0x0030a310) = 1.0f;
		if ((DAT_002baeb8 != 0 || status != 0) && MW_S(0x0030ce02) == 0) {
			/* the intro camera ended: game camera, RefBot, HUD bars */
			FUN_000d6e80((int)(uintptr_t)mw_Camera);
			if (mw_Variation == 3 || mw_Variation == 0) {
				MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) + 200.0f;
				bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
				MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) - 200.0f;
			}
			if (mw_Variation == 2)
				CBbash_Game_FollowCamera(self);
			if (mw_Variation == 1)
				CBbash_Game_FollowCameraSmooth(self);
			CLIntroCam_Stop();
			bdSetNearFar(50.0f, 10000.0f);
			RefBot_Create();
			FUN_001d5950(); /* empty; was given the vector (0, 50, 0) */
			CLHud_StartBars((float *)DAT_002ecfac, 280.0f);
			MW_S(0x0030ce02) = 1;
			GM_I(0x34) = 0;
			bsSetSfxVolume((DAT_002bae6c * 0xff) / 0xff);
			FUN_000d6e80((int)(uintptr_t)mw_Camera);
			if (mw_Variation == 3 || mw_Variation == 0) {
				MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) + 200.0f;
				bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
				MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) - 200.0f;
			}
			if (mw_Variation == 2) {
				CBbash_Game_FollowCamera(self);
			} else if (mw_Variation == 1) {
				if (mw_WallSet != NULL)
					bkDeleteResource((struct bkResource *)mw_WallSet);
				mw_WallSet = NULL;
				CBbash_Game_FollowCameraSmooth(self);
				bkPrintf("miniSet deleted\n");
			} else if (mw_Variation == 3) {
				if (mw_WallSet != NULL)
					bkDeleteResource((struct bkResource *)mw_WallSet);
				mw_WallSet = NULL;
				bkPrintf("miniSet deleted \n");
			}
			bkPrintf("intro cam ended\n");
		}
	}
	RespawnPoints_UpdateAll();
	FUN_000ba6d0();
	if (MW_S(0x0030ce02) == 1) {
		if (MW_F(0x0030a310) < 1.0f) {
			MW_F(0x0030a310) = DAT_0028fda8 * 0.01f + MW_F(0x0030a310);
			if (!(MW_F(0x0030a310) < 1.0f)) {
				for (k = 0; k < 4; k++)
					Actor_PlayAnim(MW_AP(mw_Players[k], 0x30), 0, 0, 0, 1.0f, 0.2f, 1);
			}
		}
		lift = 0.0f;
		if (mw_Variation == 3)
			lift = 200.0f;
		else if (mw_Variation == 0)
			lift = 300.0f;
		if (lift != 0.0f)
			MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) + lift;
		bmVectorLerp(MW_AV(mw_Camera, 0x534), MW_FV(0x0030a2f0), MW_AV(mw_Camera, 0x534), MW_F(0x0030a310));
		bmVectorLerp(MW_AV(mw_Camera, 0x544), MW_FV(0x0030a300), MW_AV(mw_Camera, 0x544), MW_F(0x0030a310));
		bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
		if (mw_Variation == 3 || mw_Variation == 0)
			MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) - 200.0f;
		if (CLHud_UpdateCountdown((int)(uintptr_t)DAT_002ecfac) != 0) {
			/* the round starts: 60 s */
			MW_F(0x0030cde8) = 60000.0f;
			MW_S(0x0030ce00) = 1;
			MW_F(0x0030cdec) = 60000.0f;
			for (k = 0; k < 4; k++)
				MW_VFN(mw_Players[k], 0x44, void (*)(void *, int))(mw_Players[k], 1);
			Actor_UpdateAnimations();
		}
		GM_F(0x0) = 0.0f;
	}
	Actor_UpdateAll();
	Actor_UpdateAnimations();
}

/* swaps player `player`'s burger stand (by its actor's +0x230 index) to `model` */
static void gm_SwapBurgerStand(void *player, void *model, int line)
{
	unsigned int stand;

	stand = MW_AU(MW_AP(player, 0x30), 0x230) & 0xffff;
	baDeleteActorInstance((struct BActorInstance *)(MW_A(0x0030e430) + stand * 0x170));
	stand = MW_AU(MW_AP(player, 0x30), 0x230) & 0xffff;
	baInitActorInstance((struct BActorInstance *)(MW_A(0x0030e430) + stand * 0x170), (struct BActor *)model, 8, s_GameFile,
		line);
	stand = MW_AU(MW_AP(player, 0x30), 0x230) & 0xffff;
	MW_S(0x0030e420 + stand * 2) = 1;
	stand = MW_AU(MW_AP(player, 0x30), 0x230) & 0xffff;
	memcpy(MW_A(0x0030e4f0) + stand * 0x170, MW_FV(0x002b2718 + stand * 0x10), 0x10);
}

/* the scores of the 4 players */
static void gm_GetScores(int *scores)
{
	int k;

	for (k = 0; k < 4; k++)
		scores[k] = MW_AI(mw_Players[k], 0x1400);
}

/* 0x00174610  MouseWhack/CBbash_Game.cpp: the game (phase 1): AI skill, HUD scores,
spawning, the bugs and bullets, the burger stands, the players, the train, the end of
the round */
void CBbash_Game_Play(void *self)
{
	float delta[4], pos[4], stand[4], node[4];
	int scores[4], scores2[4];
	unsigned char *bugNode, *next, *bug, *actor;
	float health, radius, length, dist;
	void *player;
	int k, j, best, worst, score;
	short type, index;

	if (MW_S(0x0030ebe8) != 0)
		return;
	/* the AI players copy player 0's AI tweaks; their skill follows the handicap */
	for (k = 1; k < 4; k++) {
		player = mw_Players[k];
		MW_AF(player, 0x133c) = MW_AF(mw_Players[0], 0x133c);
		MW_AF(player, 0x1340) = MW_AF(mw_Players[0], 0x1340);
		MW_AF(player, 0x1344) = MW_AF(mw_Players[0], 0x1344);
		if (DAT_002baed8[k * 10] == MW_I(0x0030ec50 + k * 4)) {
			if (DAT_002baed8[k * 10] == 0) {
				MW_AF(player, 0x10e0) = MW_F(0x0030ec3c);
				MW_I(0x0030ec50 + k * 4) = 1;
			} else {
				MW_AI(player, 0x10e0) = 0;
				MW_I(0x0030ec50 + k * 4) = 0;
			}
		}
	}
	gm_GetScores(scores);
	CLHud_SetScoresInt((int)(uintptr_t)DAT_002ecfac, scores);
	best = MW_AI(mw_Players[0], 0x1400);
	worst = best;
	for (k = 0; k < 4; k++) {
		score = MW_AI(mw_Players[k], 0x1400);
		if (best < score)
			best = score;
		if (score < worst)
			worst = score;
	}
	MW_AF(DAT_002ecfac, 0x40) = (float)best;
	MW_AF(DAT_002ecfac, 0x3c) = (float)worst;
	if (mw_Variation == 1) {
		/* the train */
		if (MW_S(0x0030ce6a) == 0) {
			MW_S(0x0030ce6a) = 1;
			gm_NewBug(0x4aa, 8);
		}
	} else if (mw_Variation == 2) {
		for (k = 0; k < 4; k++)
			memcpy(MW_A(0x0030e4f0) + k * 0x170, MW_FV(0x002b2718 + k * 0x10), 0x10);
	} else if (mw_Variation == 3) {
		/* the players are kept off the 4 traps; the spray of a player */
		for (k = 0; k < 4; k++) {
			for (j = 0; j < 4; j++) {
				float *trap = MW_FV(0x002b2d08 + j * 0x10);

				actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
				radius = MW_AF(MW_AP(actor, 0x18c), 0x9c) + 150.0f;
				delta[0] = trap[0] - MW_AF(actor, 0xd0);
				delta[1] = trap[1] - MW_AF(actor, 0xd4);
				delta[2] = trap[2] - MW_AF(actor, 0xd8);
				delta[1] = 0.0f;
				length = sqrtf(delta[2] * delta[2] + delta[0] * delta[0]);
				if (length < radius) {
					bmVectorSetLength(delta, delta, radius - length);
					actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
					MW_AF(actor, 0xd0) = MW_AF(actor, 0xd0) - delta[0];
					MW_AF(actor, 0xd4) = MW_AF(actor, 0xd4) - delta[1];
					MW_AF(actor, 0xd8) = MW_AF(actor, 0xd8) - delta[2];
				}
			}
			player = mw_Players[k];
			if (MW_AF(player, 0x12bc) != 0.0f) {
				baGetNodePosition((struct BActorInstance *)((char *)MW_AP(player, 0x30) + 0x10),
					(struct BActorNode *)MW_AP(player, 0x12b4), node);
				FUN_00184930(node, mw_Players[k], MW_AF(MW_AP(mw_Players[k], 0x30), 0x1e8));
				MW_AF(mw_Players[k], 0x12bc) = MW_AF(mw_Players[k], 0x12bc) - DAT_0028fda8;
				if (MW_AF(mw_Players[k], 0x12bc) < 0.0f)
					MW_AI(mw_Players[k], 0x12bc) = 0;
			}
		}
	}
	/* the first 3 bugs */
	if (GM_S(0x4) == 0) {
		GM_S(0x4) = 1;
		for (k = 0; k < 3; k++)
			gm_SpawnRandomBug(0x4e7, 0x4f7);
	}
	/* a new bug every <+0x2c92> frames while bugs remain */
	if ((float)(int)MW_S(0x0030ce62) < GM_F(0x0) && MW_S(0x0030ce60) < MW_S(0x0030ce5e)) {
		GM_F(0x0) = 0.0f;
		gm_SpawnRandomBug(0x50a, 0x51a);
	}
	if ((float)(int)MW_S(0x0030ce62) < GM_F(0x0)) {
		int flies = 0;

		if (mw_Variation == 0) {
			if (MW_S(0x0030ce48) == 0) {
				/* a tremor, at random */
				if (100.0f < (float)mw_rand() * 0.0001f + MW_F(0x0030ce4c)) {
					gm_NewBug(0x52a, 4);
					MW_S(0x0030ce48) = MW_S(0x0030ce48) + 1;
					MW_F(0x0030ce4c) = 0.0f;
				} else {
					MW_F(0x0030ce4c) = MW_F(0x0030ce4c) + 1.5f;
				}
			}
			flies = 1;
		} else if (mw_Variation == 1) {
			flies = 1;
		}
		if (flies != 0 && MW_S(0x0030ce54) == 0) {
			if (300.0f < (float)mw_rand() * 0.0001f + MW_F(0x0030ce58)) {
				bug = (unsigned char *)gm_NewBug(0x53e, 2);
				MW_S(0x0030ce54) = MW_S(0x0030ce54) + 1;
				MW_F(0x0030ce58) = 0.0f;
				gm_SpawnPartner(bug, 0x54c);
			} else {
				MW_F(0x0030ce58) = MW_F(0x0030ec1c) + MW_F(0x0030ce58);
			}
		}
	}
	CBbash_Arena_UpdateBugNumbers(mw_Arena);
	CBbash_Arena_FindClearLeader(mw_Arena);
	/* the players */
	for (k = 0; k < DAT_002bae94; k++) {
		if (MW_AF(MW_AP(mw_Players[k], 0x30), 0xd4) <= -290.0f) {
			/* fell off: respawn */
			MW_AI(mw_Players[k], 0x1124) = 0;
			MW_VFN(mw_Players[k], 0x24, void (*)(void *))(mw_Players[k]);
		}
		FUN_0017d2f0(mw_Players[k]);
		if (DAT_002baed8[k * 10] == 0)
			MW_VFN(mw_Players[k], 0xc, void (*)(void *))(mw_Players[k]);
		MW_VFN(mw_Players[k], 0x4c, void (*)(void *))(mw_Players[k]);
	}
	/* the bugs */
	bugNode = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL);
	while (bugNode != NULL) {
		bug = bugNode - 8;
		if (MW_AS(bug, 0x11fc) == 1) {
			FUN_00184bb0(bug);
			if (mw_Variation == 3)
				FUN_001842b0(bug, 6);
			MW_AS(bug, 0x11fc) = 0;
		}
		next = bugNode;
		if (MW_AS(bug, 0x11fa) != 0) {
			FUN_00181830(bug);
		} else {
			/* dead: fades out, then goes */
			for (k = 0; k < 4; k++) {
				if (MW_AP(mw_Players[k], 0x1364) == bug)
					MW_AP(mw_Players[k], 0x1364) = NULL;
			}
			if (MW_AS(bug, 0x11ec) == 8 || mw_DbgBugsFade == 0) {
				MW_AI(bug, 0x1210) = 0;
			} else {
				MW_AI(MW_AP(bug, 0x0), 0x220) = 0x200;
				MW_AI(MW_AP(bug, 0x0), 0x224) = MW_FTOL(MW_AF(bug, 0x1210));
				MW_AF(bug, 0x1210) = MW_AF(bug, 0x1210) - DAT_0028fda8 * 10.0f;
			}
			if (MW_AF(bug, 0x1210) <= 0.0f) {
				next = (unsigned char *)(uintptr_t)CLList_Remove((int *)DAT_0030ee30, (int *)bugNode);
				type = MW_AS(bug, 0x11ec);
				if (type == 5 || type == 6 || (type < 7 && type != 4 && type != 2)) {
					index = type == 5 ? 0 : type == 6 ? 1 : type;
					MW_S(0x0030ce30 + index * 2) = MW_S(0x0030ce30 + index * 2) - 1;
					MW_S(0x0030ce60) = MW_S(0x0030ce60) - 1;
				} else if (type == 4) {
					MW_S(0x0030ce48) = MW_S(0x0030ce48) - 1;
				} else if (type == 2) {
					MW_S(0x0030ce54) = MW_S(0x0030ce54) - 1;
				}
				if (MW_AS(bug, 0x11ec) == 8)
					Platform_Delete(MW_AP(bug, 0x4));
				else
					Actor_Delete(MW_AP(bug, 0x0));
				FUN_001817d0(bug);
				MW_DELETE(bug);
			}
		}
		bugNode = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)next);
	}
	/* the bullets (node at +0x1b8; state +0x1c4 == 2: finished) */
	bugNode = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, NULL);
	while (bugNode != NULL) {
		unsigned char *bullet = bugNode - 0x1b8;

		FUN_00180f60(bullet);
		next = bugNode;
		if (MW_AS(bullet, 0x1c4) == 2) {
			next = (unsigned char *)(uintptr_t)CLList_Remove((int *)DAT_0030ed00, (int *)bugNode);
			if (MW_AP(bullet, 0x1c) != NULL)
				Platform_Delete(MW_AP(bullet, 0x1c));
			FUN_00181520(bullet);
			MW_DELETE(bullet);
		}
		bugNode = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, (unsigned int *)next);
	}
	for (k = 0; k < 10; k++)
		FUN_00181740(mw_Helpers[k]);
	if (mw_Variation == 2) {
		/* the burger stands: their health (0x0030e9f0) */
		MW_S(0x0030ce74) = 0;
		for (k = 0; k < DAT_002bae94; k++) {
			if (MW_F(0x0030e9f0 + k * 4) > 0.0f)
				MW_S(0x0030ce74) = MW_S(0x0030ce74) + 1;
		}
		for (k = 0; k < DAT_002bae94; k++) {
			if ((double)MW_F(0x0030e9f0 + k * 4) < 0.15 && MW_S(0x0030ea08 + k * 2) == 0) {
				FUN_0017cf10(0x2d, 0, 0xff);
				MW_S(0x0030ea08 + k * 2) = 1;
			}
			if (MW_F(0x0030e9f0 + k * 4) <= 0.0f) {
				if (MW_S(0x0030ce6c + k * 2) == 1) {
					FUN_00183a70(MW_AV(MW_AP(mw_Players[k], 0x30), 0xd0), (short)k);
					if (1 < MW_S(0x0030ce74))
						CLSamples_PlayCommentary(0x37, (int)MW_AS(mw_Players[k], 0x1374), 0xff, 0, 0, NULL);
				}
				MW_AI(MW_AP(mw_Players[k], 0x30), 0x220) = 0;
				MW_AU(MW_AP(mw_Players[k], 0x30), 0x220) |= 0x100;
				MW_S(0x0030ce6c + k * 2) = 0;
				AI_SetState(mw_Players[k], 6);
			}
			/* the player is pushed out of every stand (radius by its health) */
			for (j = 0; j < 4; j++) {
				float *at = MW_FV(0x002b2718 + j * 0x10);

				actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
				radius = MW_F(0x0030ea00) * MW_F(0x0030e9f0 + j * 4) + 30.0f;
				memcpy(pos, MW_AV(actor, 0xd0), 0x10);
				stand[0] = at[0];
				stand[1] = at[1];
				stand[2] = at[2];
				stand[3] = at[3];
				dist = bmVectorDistance(pos, stand);
				if (dist < radius) {
					pos[0] = pos[0] - stand[0];
					pos[1] = pos[1] - stand[1];
					pos[2] = pos[2] - stand[2];
					bmVectorSetLength(pos, pos, radius);
					actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
					MW_AF(actor, 0xd0) = pos[0] + at[0];
					MW_AF(actor, 0xd4) = pos[1] + at[1];
					MW_AF(actor, 0xd8) = pos[2] + at[2];
				}
			}
			/* the stand's model and size follow its health */
			health = MW_F(0x0030e9f0 + k * 4);
			if (health < 0.0f || health < 0.8f) {
				if (health < 0.0f)
					health = 0.0f;
				if (MW_S(0x0030e420 + (MW_AU(MW_AP(mw_Players[k], 0x30), 0x230) & 0xffff) * 2) == 0)
					gm_SwapBurgerStand(mw_Players[k], MW_P(0x0030e414), 0x61b);
			}
			if (health < 0.6f && MW_S(0x0030e420 + (MW_AU(MW_AP(mw_Players[k], 0x30), 0x230) & 0xffff) * 2) == 1)
				gm_SwapBurgerStand(mw_Players[k], MW_P(0x0030e418), 0x622);
			if (health < 0.4f && MW_S(0x0030e420 + (MW_AU(MW_AP(mw_Players[k], 0x30), 0x230) & 0xffff) * 2) == 2)
				gm_SwapBurgerStand(mw_Players[k], MW_P(0x0030e41c), 0x629);
			if (health == 0.0f) {
				MW_F(0x0030e500 + k * 0x170 + 8) = 0.0f;
				MW_F(0x0030e500 + k * 0x170) = 0.0f;
			} else {
				health = health + 0.1f;
				MW_F(0x0030e500 + k * 0x170 + 8) = health;
				MW_F(0x0030e500 + k * 0x170) = health;
			}
		}
		if (MW_S(0x0030ce74) < 2) {
			/* one stand left: the round ends */
			for (k = 0; k < 4; k++) {
				scores2[k] = MW_AI(mw_Players[k], 0x1400);
				if (MW_S(0x0030ce6c + k * 2) == 0) {
					MW_S(0x0030ce6c + k * 2) = 1;
					MW_AI(mw_Players[k], 0x1124) = 0;
					MW_VFN(mw_Players[k], 0x24, void (*)(void *))(mw_Players[k]);
				}
			}
			CLHud_CreateWinnerText((int)(uintptr_t)DAT_002ecfac, scores2, 0);
			MW_F(0x0030cde8) = 16000.0f;
			MW_S(0x0030ce00) = 2;
			CBbash_Arena_FindLeaders(mw_Arena);
			CBbash_Game_StartOutro(self);
		}
	}
	Platforms_UpdateAll();
	AI_TickAll();
	for (k = 0; k < DAT_002bae94; k++) {
		FUN_0017df00(mw_Players[k]);
		memcpy((char *)mw_Players[k] + 0x13cc, MW_AV(MW_AP(mw_Players[k], 0x30), 0x194), 0x10);
	}
	Actor_UpdateAll();
	/* each player's weapon object follows his right hand side */
	for (k = 0; k < 4; k++) {
		void *weapon;

		pos[0] = 100.0f;
		pos[1] = -50.0f;
		pos[2] = 0.0f;
		pos[3] = 0.0f;
		bmVectorRotateY(pos, pos, MW_AF(MW_AP(mw_Players[k], 0x30), 0x1e8) - 1.5707963f);
		actor = (unsigned char *)MW_AP(mw_Players[k], 0x30);
		pos[0] = pos[0] + MW_AF(actor, 0xd0);
		pos[1] = pos[1] + MW_AF(actor, 0xd4);
		pos[2] = pos[2] + MW_AF(actor, 0xd8);
		pos[3] = 1.0f;
		weapon = MW_AP(mw_Players[k], 0x1138);
		MW_AF(weapon, 0xd0) = pos[0];
		MW_AF(weapon, 0xd4) = pos[1];
		MW_AF(weapon, 0xd8) = pos[2];
		MW_AF(weapon, 0xdc) = 1.0f;
	}
	for (k = 0; k < DAT_002bae94; k++)
		memcpy((char *)mw_Players[k] + 0x13dc, MW_AV(MW_AP(mw_Players[k], 0x30), 0x194), 0x10);
	RespawnPoints_UpdateAll();
	FUN_000ba6d0();
	Actor_UpdateAnimations();
	if (mw_Variation == 1) {
		/* the train; players beyond the platform ends lose 10 */
		CBbash_Game_TrainPush(self, (float *)scores, 0);
		for (k = 0; k < 4; k++) {
			if (1800.0f <= MW_AF(MW_AP(mw_Players[k], 0x30), 0xd0))
				gm_HitPlayer(mw_Players[k], -10);
			if (MW_AF(MW_AP(mw_Players[k], 0x30), 0xd0) <= -1540.0f)
				gm_HitPlayer(mw_Players[k], -10);
		}
	}
	if (MW_F(0x0030cde8) <= 0.0f) {
		/* time up */
		MW_F(0x0030cde8) = 16000.0f;
		MW_S(0x0030ce00) = 2;
		for (k = 0; k < 4; k++)
			bkPrintf("player %d score %d\n", k + 1, MW_AI(mw_Players[k], 0x1400));
		CBbash_Arena_FindLeaders(mw_Arena);
		gm_GetScores(scores2);
		CLHud_CreateWinnerText((int)(uintptr_t)DAT_002ecfac, scores2, 0);
		if (mw_Variation == 2) {
			for (k = 0; k < 4; k++) {
				if (MW_S(0x0030ce6c + k * 2) == 0) {
					MW_S(0x0030ce6c + k * 2) = 1;
					MW_AI(mw_Players[k], 0x1124) = 0;
					MW_VFN(mw_Players[k], 0x24, void (*)(void *))(mw_Players[k]);
				}
			}
		}
		CBbash_Game_StartOutro(self);
	}
	if (MW_S(0x0030cdfe) != 0) {
		memset(scores, 0, sizeof scores);
		for (k = 0; k < DAT_002bae94; k++)
			scores[k] = MW_AI(mw_Players[k], 0x1400);
		CLHud_SetScores((int)(uintptr_t)DAT_002ecfac, scores);
	}
}

/* 0x00175b70  (gap): a frame of the game: pause handling, the cameras, the phase
(0 intro, 1 game, 2 outro, 3 paused), then the HUD, floating scores, debris, clock,
shadows, the RefBot, the splats and the bullets; +0 counts the frames */
void CBbash_Game_Update(void *self)
{
	float eye[4];
	float clock, heading;
	unsigned char *actor;

	MW_I(0x0030ef00) = 0;
	if (mw_Quit != 0) {
		MW_I(0x0030ef00) = 0;
		return;
	}
	for (actor = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_002ea488, NULL); actor != NULL;
	     actor = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_002ea488, (unsigned int *)actor)) {
		if (MW_AP(actor, 0x18c) != NULL) {
			MW_AB(MW_AP(actor, 0x18c), 0x199) = 0;
			MW_AB(MW_AP(actor, 0x18c), 0x198) = 0;
		}
	}
	if (MW_P(0x0030a324) != NULL) {
		MW_AB(MW_P(0x0030a324), 0x3d4) = 1;
		if (MW_AF(MW_P(0x0030a324), 0xe4) < 0.0f) {
			MW_AF(MW_P(0x0030a324), 0xe4) = MW_AF(MW_P(0x0030a324), 0xe4) + 10.0f;
			if (!(MW_AF(MW_P(0x0030a324), 0xe4) < 0.0f)) {
				MW_AI(MW_P(0x0030a324), 0xe4) = 0;
				MW_AB(MW_P(0x0030a324), 0x3d4) = 0;
				MW_P(0x0030a324) = NULL;
			}
		}
	}
	if (mw_DbgOverlays != 0)
		COverlay_UpdateAll();
	if (DAT_002ecdec != 0)
		MW_F(0x0030a270) = 0.0f;
	if (MW_S(0x0030cdfe) != 0)
		MW_F(0x0030cde8) = MW_F(0x0030cde8) - MW_F(0x0030a270);
	if (DAT_002ecdec == 0) {
		if (MW_S(0x0030ce06) != 0) {
			MW_S(0x0030ce06) = 0;
			if (MW_S(0x0030ce04) == 0)
				CLIntroCam_Update(0.0f, 0);
			MW_S(0x0030ce00) = MW_S(0x0030ce04);
		}
	} else if (MW_S(0x0030ce06) == 0) {
		MW_S(0x0030ce06) = 1;
		MW_S(0x0030ce04) = MW_S(0x0030ce00);
		MW_S(0x0030ce00) = 3;
	}
	if (MW_S(0x0030ce02) != 0 && MW_S(0x0030ce00) != 2) {
		CBbash_Game_HidePlayers(self);
		if (mw_Variation == 3)
			MW_AF(mw_Camera, 0x308) = 3100.0f;
		if (mw_Variation == 0)
			MW_AF(mw_Camera, 0x308) = 1600.0f;
		FUN_000d6e80((int)(uintptr_t)mw_Camera);
		if (mw_Variation == 3) {
			MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) + 200.0f;
			bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
			MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) - 200.0f;
		} else if (mw_Variation == 0) {
			MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) + 300.0f;
			bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
			MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) - 300.0f;
		}
		if (mw_Variation == 2)
			CBbash_Game_FollowCamera(self);
		if (mw_Variation == 1)
			CBbash_Game_FollowCameraSmooth(self);
		CBbash_Game_RestorePlayers(self);
		if (mw_DbgFirstPerson != 0) {
			/* "First person": behind player 0 */
			eye[0] = 0.0f;
			eye[1] = 200.0f;
			eye[2] = -600.0f;
			eye[3] = 0.0f;
			bmVectorRotateY(eye, eye, MW_AF(MW_AP(mw_Players[0], 0x30), 0x1e8));
			actor = (unsigned char *)MW_AP(mw_Players[0], 0x30);
			memcpy(MW_AV(mw_Camera, 0x534), MW_AV(actor, 0xd0), 0x10);
			MW_AF(mw_Camera, 0x538) = MW_AF(mw_Camera, 0x538) + 200.0f;
			actor = (unsigned char *)MW_AP(mw_Players[0], 0x30);
			eye[0] = eye[0] + MW_AF(actor, 0xd0);
			eye[1] = eye[1] + MW_AF(actor, 0xd4);
			eye[2] = eye[2] + MW_AF(actor, 0xd8);
			bdSetCameraLookAt(eye, MW_AV(mw_Camera, 0x534), 0.0f);
			MW_AI(mw_Players[0], 0x1118) = 0;
		}
	}
	switch (MW_S(0x0030ce00)) {
	case 0:
		CBbash_Game_Intro(self);
		break;
	case 1:
		CBbash_Game_Play(self);
		break;
	case 3:
		if (MW_S(0x0030ce04) != 2)
			break;
		/* fall through */
	case 2:
		CBbash_Game_Outro(self);
		break;
	}
	if (mw_Quit != 0)
		return;
	DebrisColours_SetAll(MW_AV(MW_AP(mw_Players[0], 0x30), 0xd0), MW_AV(MW_AP(mw_Players[1], 0x30), 0xd0),
		MW_AV(MW_AP(mw_Players[2], 0x30), 0xd0), MW_AV(MW_AP(mw_Players[3], 0x30), 0xd0));
	CLHud_UpdateBars((int)(uintptr_t)DAT_002ecfac);
	heading = gm_Heading(MW_AF(mw_Camera, 0x534) - MW_AF(mw_Camera, 0x544), MW_AF(mw_Camera, 0x538) - MW_AF(mw_Camera, 0x548));
	MW_F(0x0030eaf8) = MW_F(0x0030a0d0);
	MW_F(0x0030ea74) = (3.1415925f - heading) + 3.1415925f;
	MW_F(0x0030eaf0) = MW_F(0x0030a0d4);
	if (DAT_002ecdec == 0) {
		baUpdateExtras((struct BExtraSystem *)MW_P(0x0030ea70), (void *)(uintptr_t)mw_fbits(DAT_0028fda8));
		Debris_UpdateSystems(DAT_0028fda8 * 0.016666668f);
	}
	bdUpdateView();
	if (MW_S(0x0030ce00) == 1 || (MW_S(0x0030ce04) == 1 && DAT_002ecdec != 0))
		clock = MW_F(0x0030cde8) * 0.001f;
	else if (MW_S(0x0030ce00) == 0 || (MW_S(0x0030ce04) == 0 && DAT_002ecdec != 0))
		clock = 60.0f;
	else
		clock = 0.0f;
	CLHud_SetClock((int)(uintptr_t)DAT_002ecfac, clock, 1);
	if (mw_Variation == 2)
		CLHud_AddSeconds((int)(uintptr_t)DAT_002ecfac, DAT_0028fda8);
	if (DAT_002f289c[0] == NULL)
		bkPrintf("no player/s \n");
	if (mw_DbgShadows != 0)
		Shadows_Aim();
	if (DAT_002ecdec == 0)
		RefBot_Update();
	FUN_00186280();
	FUN_00180f00(mw_Bullets);
	GM_F(0x0) = DAT_0028fda8 + GM_F(0x0);
}
