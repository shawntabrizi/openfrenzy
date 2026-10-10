/*
MouseWhack/CBbash_Arena.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CBbash_Arena.cpp
(module mousewhack)

CBbash_Arena: the arena of a Bug Bash variation (one static object at 0x0030a1d0, built by a
core static constructor through 0x00170ea0). It holds the variation, the intro camera file,
the frame timer, the bug spawn counts, the props of the arena (gib models, burger stands,
the electric traps of "Pied Pipers") and the team/leader bookkeeping.

0x00170ec0 (Initialise) carries the file name in its allocation tags; the other functions
are methods of the same object around it (gap functions; file inferred from the class).
0x00170ec0 and 0x00172730 are missing from the function table (reached through calls that
Ghidra did not follow); they are defined here under their FUN_ names.

Arena layout (byte offsets; all fields reached with AR_* below):
  +0x0000 short   variation
  +0x0004 void *  the intro camera file (.lom) data
  +0x0008         the frame timer (0xa0 bytes, vtable 0x00272d64)
  +0x00a8 short   team game (1: the variation is played in teams)
  +0x00d8 short[4] the leading players (-1: none)
  +0x0150 font of the debug text
  +0x0460 short   the clear leader (-1: none)
  +0x2c18 float[2] the round length (frames)
  +0x2c3c short[5] bug counts at the start, +0x2c48 short[5] at the end of the round
  +0x2c54 short[7] bugs to spawn of each type, +0x2c60 short[7] spawned, +0x2c6c short[7]
  +0x2c8e short   the bugs to spawn in all
  +0x4820 float[4] attack weights of the players
  +0x4840 short   the screen shot counter
*/

#include "mousewhack.h"

static const char s_ArenaFile[] = "C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBbash_Arena.cpp";

#define AR_S(off) MW_AS(self, off)
#define AR_I(off) MW_AI(self, off)
#define AR_F(off) MW_AF(self, off)
#define AR_P(off) MW_AP(self, off)
#define AR_A(off) ((char *)self + (off))

/* core: 1 when the game is played in teams */
extern int DAT_002baebc;

/* 0x001708b0  (gap): loads the intro camera file `name` (.lom) from `package` and sets up
the intro camera from it. Returns 1, or 0 on error. */
int CBbash_Arena_LoadLOMFile(void *self, void *package, const char *name)
{
	unsigned int size;
	int *map;
	void *data;

	data = bkLoadFile((struct bkPackage *)package, name, NULL, &size, NULL, 0);
	AR_P(0x4) = data;
	if (data == NULL) {
		bkPrintf("LoadLOMFile ERROR: Could not load [%s]\n", name);
		return 0;
	}
	map = ParseMap_Load((char *)data, 2, (int)size);
	if (map == NULL) {
		bkPrintf("LoadLOMFile ERROR: Parsing [%s]\n", name);
		return 0;
	}
	CLIntroCam_Load((int)(uintptr_t)map, (undefined4)(uintptr_t)self);
	ParseMap_Free(map);
	return 1;
}

/* 0x00170940  (gap): the clear leader (+0x460): the player with the highest score if it is
at least 300 and more than 1.2 x the second best score, else -1 */
void CBbash_Arena_FindClearLeader(void *self)
{
	float best, second, score;
	short i, leader;

	second = 0.0f;
	best = 0.0f;
	leader = 0;
	for (i = 0; i < 4; i++) {
		score = (float)MW_AI(mw_Players[i], 0x1400);
		if (best < score) {
			best = score;
			leader = i;
		}
	}
	if (300.0f <= best) {	/* _DAT_00254f14 */
		for (i = 0; i < 4; i++) {
			score = (float)MW_AI(mw_Players[i], 0x1400);
			if (second < score && i != leader)
				second = score;
		}
		if (second * 1.2f < best) {
			AR_S(0x460) = leader;
			return;
		}
	}
	AR_S(0x460) = -1;
}

/* 0x00170a00  (gap): the leading players (+0xd8, up to 4, -1 terminated): every player
with the top score */
void CBbash_Arena_FindLeaders(void *self)
{
	short best, i, count;

	best = -1;
	for (i = 0; i < 4; i++) {
		if ((int)best < MW_AI(mw_Players[i], 0x1400))
			best = (short)MW_AI(mw_Players[i], 0x1400);
		AR_S(0xd8 + i * 2) = -1;
	}
	count = 0;
	for (i = 0; i < 4; i++) {
		if ((int)best == MW_AI(mw_Players[i], 0x1400)) {
			AR_S(0xd8 + count * 2) = i;
			count++;
		}
	}
}

/* 0x00170a80  (gap): the leading team in a team game: the players of the team (+0x12c0 of
the player) with the higher total score, or all four on a draw */
void CBbash_Arena_FindLeadingTeam(void *self)
{
	short team0, team1, i, count;

	team0 = 0;
	team1 = 0;
	count = 0;
	for (i = 0; i < 4; i++) {
		AR_S(0xd8 + i * 2) = -1;
		if (MW_AS(mw_Players[i], 0x12c0) == 0)
			team0 = (short)(team0 + MW_AS(mw_Players[i], 0x1400));
		else
			team1 = (short)(team1 + MW_AS(mw_Players[i], 0x1400));
	}
	if (team1 < team0) {
		for (i = 0; i < 4; i++) {
			if (MW_AS(mw_Players[i], 0x12c0) == 0) {
				AR_S(0xd8 + count * 2) = i;
				count++;
			}
		}
		return;
	}
	if (team0 < team1) {
		for (i = 0; i < 4; i++) {
			if (MW_AS(mw_Players[i], 0x12c0) == 1) {
				AR_S(0xd8 + count * 2) = i;
				count++;
			}
		}
		return;
	}
	if (team0 == team1) {
		for (i = 0; i < 4; i++)
			AR_S(0xd8 + i * 2) = i;
	}
}

/* 0x00170b80  (gap): updateBugNumbers: the number of bugs of each of the 5 types to spawn
grows from the start count to the end count over the round; the growth is added to the
bugs to spawn (+0x2c8e) */
void CBbash_Arena_UpdateBugNumbers(void *self)
{
	short target, start, end;
	int i;

	for (i = 0; i < 5; i++) {
		start = AR_S(0x2c3c + i * 2);
		end = AR_S(0x2c48 + i * 2);
		target = (short)MW_FTOL(((float)(end - start) * (AR_F(0x2c1c) - AR_F(0x2c18))) / AR_F(0x2c1c) + (float)start + 0.5f);
		if (target != AR_S(0x2c54 + i * 2)) {
			if (target < AR_S(0x2c54 + i * 2)) {
				bkPrintf("error in updateBugNumbers\n");
			} else {
				AR_S(0x2c8e) = (short)(AR_S(0x2c8e) + (target - AR_S(0x2c54 + i * 2)));
				AR_S(0x2c54 + i * 2) = target;
			}
		}
	}
}

/* one line of the debug text (bdPrintFontLite at x -250) */
static void ar_DebugLine(void *self, const char *text, float y)
{
	unsigned short wide[256];

	bkStringToWide(wide, text);
	bdPrintFontLite((struct bdFont *)AR_P(0x150), wide, (int)strlen(text), -250.0f, y, 0x7f, 0x7f, 0x7f, 0x7f, NULL, NULL);
}

/* 0x00170c00  (gap): the "Frame Counters" debug text */
void CBbash_Arena_DrawDebug(void *self)
{
	char text[256];

	if (mw_DbgFrameCounters == 0)
		return;
	bdSetProjectionMode(0);
	bdSetRenderState(0, 0, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(1, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetWorldMatrix(DAT_002855f0);
	sprintf(text, "Global anim factor %f", (double)DAT_00293920);
	ar_DebugLine(self, text, 0.0f);
	sprintf(text, "Frame Rate %f", (double)MW_F(0x0030cdf0));
	ar_DebugLine(self, text, -20.0f);
	sprintf(text, "Game Speed %f", (double)DAT_0028fda8);
	ar_DebugLine(self, text, -40.0f);
	sprintf(text, "core Fps %f", (double)DAT_002ba990);
	ar_DebugLine(self, text, -80.0f);
	sprintf(text, "player 0 pos x %f z %f", (double)MW_AF(MW_AP(mw_Players[0], 0x30), 0xd0),
		(double)MW_AF(MW_AP(mw_Players[0], 0x30), 0xd8));
	ar_DebugLine(self, text, -100.0f);
	bdSetProjectionMode(1);
}

/* 0x00170ea0  (gap): CBbash_Arena::CBbash_Arena (static constructor): the frame timer */
void *CBbash_Arena_Ctor(void *self)
{
	MW_Timer_Ctor(AR_A(0x8));
	return self;
}

/* 0x00170eb0  (gap): CBbash_Arena::~CBbash_Arena (atexit) */
void CBbash_Arena_Dtor(void *self)
{
	MW_Timer_Dtor(AR_A(0x8));
}

/* loads a model and makes an actor instance of it at `off` */
static void ar_LoadInstance(void *self, int off, const char *model, int slots, int line)
{
	baInitActorInstance((struct BActorInstance *)AR_A(off),
		(struct BActor *)bLoadResource(NULL, model, 1, 0xdefa), slots, s_ArenaFile, line);
}

/* new smashable prop (0x1d0 bytes, 0x00181550) */
static void ar_NewProp(float *pos, float *rot, const char *model, const char *smashed, const char *alpha, int line)
{
	void *prop = bkHeapAlloc(0x1d0, s_ArenaFile, line, 0x1001);

	if (prop != NULL)
		FUN_00181550(prop, pos, rot, model, smashed, "smash", alpha, 1);
}

/* the bugs to spawn in all: the sum of the 7 type counts */
static void ar_SumBugsToSpawn(void *self)
{
	int i;

	AR_S(0x2c8e) = 0;
	for (i = 0; i < 7; i++)
		AR_S(0x2c8e) = (short)(AR_S(0x2c8e) + AR_S(0x2c54 + i * 2));
}

/* the 5 start and end bug counts (+0x2c3c, +0x2c48) and the counts to spawn (+0x2c54) */
static void ar_SetBugCounts(void *self, const short *start, const short *end)
{
	int i;

	for (i = 0; i < 5; i++) {
		AR_S(0x2c54 + i * 2) = start[i];
		AR_S(0x2c3c + i * 2) = start[i];
		AR_S(0x2c48 + i * 2) = end[i];
	}
}

/* the round length (+0x2c18, +0x2c1c: 5000 frames) after a timer reset */
static void ar_ResetRoundTimer(void *self)
{
	MW_Timer_Reset(AR_A(0x8));
	AR_F(0x2c18) = 5000.0f;
	AR_F(0x2c1c) = 5000.0f;
}

/* 0x00170ec0  CBbash_Arena.cpp (missing from the table): CBbash_Arena::Initialise: the
lights, the gib models, the bug counts and the props of the variation */
void FUN_00170ec0(void *self)
{
	static const short brawlStart[5] = { 3, 3, 0, 2, 0 }, brawlEnd[5] = { 7, 6, 0, 5, 0 };
	static const short tremorsStart[5] = { 0, 0, 0, 6, 0 }, tremorsEnd[5] = { 0, 0, 0, 0xc, 0 };
	static const short burgersStart[5] = { 2, 4, 0, 2, 0 }, burgersEnd[5] = { 6, 9, 0, 5, 0 };
	static const short pipersStart[5] = { 5, 5, 0, 0, 0 }, pipersEnd[5] = { 7, 7, 0, 0, 0 };
	float matrix[16];
	float lightDir[4];
	float pos[4], rot[4], rot2[4];
	const char *name;
	void *model;
	int i;

	MW_I(0x0030eb60) = 0;
	AR_S(0x2c90) = 0;
	AR_I(0x2c9c) = 0x10001; /* shorts +0x2c9c..+0x2ca2 = 1 */
	AR_I(0x2ca0) = 0x10001;
	MW_S(0x0030eb64) = 0;
	AR_I(0x154) = 0;
	for (i = 0; i < 7; i++) {
		AR_S(0x2c54 + i * 2) = 0;
		AR_S(0x2c60 + i * 2) = 0;
		AR_S(0x2c6c + i * 2) = 0;
	}
	AR_I(0x2c24) = 0;
	AR_I(0x2c28) = 0;
	AR_F(0xe0) = -610.0f;
	AR_F(0xe4) = 0.0f;
	AR_F(0xe8) = 1000.0f;
	AR_F(0xf0) = -600.0f;
	AR_F(0xf4) = 0.0f;
	AR_F(0xf8) = -1010.0f;
	MW_I(0x0030eb8c) = 0;
	AR_I(0x140) = 0;
	baSetAmbientLight(0x7f, 0x7f, 0x7f);
	if (DAT_002baebc != 0 && (AR_S(0x0) == 0 || AR_S(0x0) == 2))
		AR_S(0xa8) = 1;
	else
		AR_S(0xa8) = 0;
	AR_S(0x460) = -1;
	AR_I(0x144) = 0;
	mw_GameState = 1;
	lightDir[0] = 0.5f;
	lightDir[1] = 0.2f;
	if (AR_S(0x0) == 2 || AR_S(0x0) == 1)
		lightDir[2] = -0.5f;
	else
		lightDir[2] = 0.5f;
	mw_Light2 = NULL;
	mw_Light = baCreateLight(NULL, 0);
	baSetDirectionalLight((struct BLight *)mw_Light, lightDir);
	baSetLightColour((struct BLight *)mw_Light, 0x7f, 0x7f, 0x46);
	baEnableLight((struct BLight *)mw_Light, 1);
	/* gibs */
	ar_LoadInstance(self, 0x33e0, "models\\effects\\giba.obe", 1, 0xcb);
	ar_LoadInstance(self, 0x3550, "models\\effects\\gibb.obe", 1, 0xcd);
	ar_LoadInstance(self, 0x36c0, "models\\effects\\gibc.obe", 1, 0xcf);
	ar_LoadInstance(self, 0x3830, "models\\effects\\tremgiba.obe", 1, 0xd2);
	ar_LoadInstance(self, 0x39a0, "models\\effects\\tremgibb.obe", 1, 0xd4);
	ar_LoadInstance(self, 0x3b10, "models\\effects\\tremgibc.obe", 1, 0xd6);
	ar_LoadInstance(self, 0x3c80, "models\\effects\\tremgut.obe", 1, 0xd8);
	ar_LoadInstance(self, 0x3df0, "models\\spitgiba.obe", 1, 0xdb);
	ar_LoadInstance(self, 0x3f60, "models\\spitgibb.obe", 1, 0xdd);
	ar_LoadInstance(self, 0x40d0, "models\\spitgibc.obe", 1, 0xdf);
	CLList_Init2((int *)DAT_0030ed00);
	CLList_Init2((int *)DAT_0030ea20);
	AR_S(0x2c78) = 0;
	AR_I(0x2c7c) = 0;
	AR_S(0x2c84) = 0;
	AR_F(0x2c88) = -0.1f;
	AR_S(0x4840) = 0;
	switch (AR_S(0x0)) {
	case 0:
		/* Brawl: train seats and a phone */
		ar_SetBugCounts(self, brawlStart, brawlEnd);
		pos[0] = -200.0f;
		pos[1] = 0.0f;
		pos[2] = -134.0f;
		pos[3] = 1.0f;
		rot[0] = 0.0f;
		rot[1] = 3.1415925f;
		rot[2] = 0.0f;
		rot[3] = 0.0f;
		ar_NewProp(pos, rot, "models\\seatrev.obe", "models\\seatsmsh.obe", "textures\\seatalpha.bmp", 0x116);
		pos[0] = 900.0f;
		ar_NewProp(pos, rot, "models\\seatrev.obe", "models\\seatsmsh.obe", "textures\\seatalpha.bmp", 0x119);
		pos[2] = -265.0f;
		rot[1] = 0.0f;
		pos[0] = -200.0f;
		/* the original passes the position as the rotation from here on */
		ar_NewProp(pos, pos, "models\\seat.obe", "models\\seatsmsh.obe", "textures\\seatalpha.bmp", 0x11f);
		pos[0] = 900.0f;
		ar_NewProp(pos, pos, "models\\seat.obe", "models\\seatsmsh.obe", "textures\\seatalpha.bmp", 0x122);
		pos[0] = 355.0f;
		pos[2] = -180.0f;
		ar_NewProp(pos, pos, "models\\phone.obe", "models\\phonesmsh.obe", "textures\\phonealpha.bmp", 0x127);
		ar_ResetRoundTimer(self);
		AR_S(0x2c30) = 0;
		ar_SumBugsToSpawn(self);
		AR_S(0x2c92) = 1;
		AR_S(0x2c6c) = 1;
		AR_S(0x2c6e) = 1;
		AR_S(0x2c70) = 1;
		AR_S(0x2c72) = 1;
		break;
	case 1:
		/* Tremors */
		ar_SetBugCounts(self, tremorsStart, tremorsEnd);
		ar_ResetRoundTimer(self);
		AR_S(0x2c30) = 0;
		AR_I(0x2c94) = 0;
		AR_S(0x2c98) = 0;
		AR_S(0x2c9a) = 0;
		mw_Light2 = NULL;
		mw_Light2 = baCreateLight(NULL, 0);
		ar_SumBugsToSpawn(self);
		AR_S(0x2c92) = 1;
		AR_S(0x2c6c) = 1;
		AR_S(0x2c6e) = 1;
		AR_S(0x2c70) = 1;
		AR_S(0x2c72) = 1;
		break;
	case 2:
		/* Burgers: seats and tables of the restaurant */
		pos[0] = -1320.0f;
		pos[1] = 0.0f;
		pos[2] = 600.0f;
		pos[3] = 1.0f;
		rot2[0] = 0.0f;
		rot2[1] = 3.1415925f;
		rot2[2] = 0.0f;
		rot2[3] = 0.0f;
		rot[0] = 0.0f;
		rot[1] = 0.0f;
		rot[2] = 0.0f;
		rot[3] = 0.0f;
		ar_NewProp(pos, rot, "models\\bseat.obe", "models\\bseatsmsh.obe", "textures\\seatalpha.bmp", 0x175);
		pos[2] = 410.0f;
		ar_NewProp(pos, rot, "models\\table.obe", "models\\tablesmsh.obe", NULL, 0x178);
		pos[2] = 220.0f;
		ar_NewProp(pos, rot2, "models\\bseatrev.obe", "models\\bseatsmsh.obe", "textures\\seatalpha.bmp", 0x17b);
		pos[2] = 85.0f;
		ar_NewProp(pos, rot, "models\\bseat.obe", "models\\seatsmsh.obe", "textures\\seatalpha.bmp", 0x17e);
		pos[2] = -100.0f;
		ar_NewProp(pos, rot, "models\\table.obe", "models\\tablesmsh.obe", NULL, 0x181);
		pos[2] = -295.0f;
		ar_NewProp(pos, rot2, "models\\bseatrev.obe", "models\\bseatsmsh.obe", "textures\\seatalpha.bmp", 0x184);
		pos[2] = -425.0f;
		ar_NewProp(pos, rot, "models\\bseat.obe", "models\\bseatsmsh.obe", "textures\\seatalpha.bmp", 0x187);
		pos[2] = -620.0f;
		ar_NewProp(pos, rot, "models\\table.obe", "models\\tablesmsh.obe", "textures\\seatalpha.bmp", 0x18a);
		pos[2] = -805.0f;
		ar_NewProp(pos, rot2, "models\\bseatrev.obe", "models\\bseatsmsh.obe", "textures\\seatalpha.bmp", 0x18d);
		/* the squid sign */
		model = bLoadResource(NULL, "effects\\skuid.obe", 1, 0xdefa);
		AR_P(0x21b0) = model;
		baInitActorInstance((struct BActorInstance *)AR_A(0x21c0), (struct BActor *)model, 4, s_ArenaFile, 0x192);
		AR_F(0x2280) = 510.0f;
		AR_F(0x2284) = 600.0f;
		AR_F(0x2288) = 1100.0f;
		AR_F(0x228c) = 1.0f;
		AR_P(0x2330) = baFindSubObject((struct BActorInstance *)AR_A(0x21c0), "spin");
		baFlushAnimations(AR_A(0x21c0), (struct BActorNode *)AR_P(0x22f4), 1, 0);
		baQueueAnimation((struct BActorInstance *)AR_A(0x21c0), (struct BActorNode *)AR_P(0x22f4),
			(struct BAnim *)AR_P(0x2330), 0x400, -1, 1, NULL, NULL);
		/* the neon sign, on and off */
		model = bLoadResource(NULL, "effects\\neon_on.obe", 1, 0xdefa);
		AR_P(0x2334) = model;
		baInitActorInstance((struct BActorInstance *)AR_A(0x2340), (struct BActor *)model, 4, s_ArenaFile, 0x19c);
		model = bLoadResource(NULL, "effects\\neon_off.obe", 1, 0xdefa);
		AR_P(0x2334) = model;
		baInitActorInstance((struct BActorInstance *)AR_A(0x24b0), (struct BActor *)model, 4, s_ArenaFile, 0x19e);
		AR_P(0x2620) = baFindSubObject((struct BActorInstance *)AR_A(0x2340), "active");
		AR_P(0x2624) = baFindSubObject((struct BActorInstance *)AR_A(0x24b0), "active");
		baFlushAnimations(AR_A(0x2340), (struct BActorNode *)AR_P(0x2474), 1, 0);
		baFlushAnimations(AR_A(0x24b0), (struct BActorNode *)AR_P(0x25e4), 1, 0);
		AR_I(0x2570) = 0;
		AR_I(0x2400) = 0;
		AR_I(0x2574) = 0;
		AR_I(0x2404) = 0;
		AR_I(0x2578) = 0;
		AR_I(0x2408) = 0;
		baQueueAnimation((struct BActorInstance *)AR_A(0x2340), (struct BActorNode *)AR_P(0x2474),
			(struct BAnim *)AR_P(0x2620), 0x400, -1, 1, NULL, NULL);
		baQueueAnimation((struct BActorInstance *)AR_A(0x24b0), (struct BActorNode *)AR_P(0x25e4),
			(struct BAnim *)AR_P(0x2624), 0x400, -1, 1, NULL, NULL);
		ar_SetBugCounts(self, burgersStart, burgersEnd);
		/* each player's flag (a model by character, names at 0x002b2778, 0x20 bytes each) */
		for (i = 0; i < 4; i++) {
			char *inst = AR_A(0x2640) + i * 0x170;

			name = MW_A(0x002b2778) + DAT_002baed4[i * 10] * 0x20;
			model = bLoadResource(NULL, name, 1, 0xdefa);
			AR_P(0x2628 + i * 4) = model;
			if (model == NULL) {
				bkPrintf("could not find model %s\n", name);
			} else {
				baInitActorInstance((struct BActorInstance *)inst, (struct BActor *)model, 8, s_ArenaFile, 0x1cd);
				memcpy(inst + 0xc0, MW_FV(0x002b2718 + i * 0x10), 0x10);
				AR_P(0x1bb0 + i * 4) = baFindSubObject((struct BActorInstance *)inst, "flutter");
				bkPrintf("Found model %s\n", MW_A(0x002b2778) + DAT_002baed4[i * 10] * 0x20);
				bmMatrixRotationY(matrix, 3.1415925f);
				bmMatrixToQuaternion((float *)(inst + 0xe0), matrix);
				baFlushAnimations(inst, (struct BActorNode *)MW_AP(inst, 0x134), 1, 0);
				baQueueAnimation((struct BActorInstance *)inst, (struct BActorNode *)MW_AP(inst, 0x134),
					(struct BAnim *)AR_P(0x1bb0 + i * 4), 0x400, -1, 1, NULL, NULL);
			}
		}
		AR_F(0x4830) = 150.0f;
		AR_F(0x4834) = 120.0f;
		AR_S(0x2ca4) = 4;
		ar_ResetRoundTimer(self);
		AR_S(0x2c30) = 0;
		ar_SumBugsToSpawn(self);
		AR_S(0x2c92) = 1;
		AR_S(0x2c6c) = 1;
		AR_S(0x2c6e) = 1;
		AR_S(0x2c70) = 4;
		AR_S(0x2c72) = 1;
		/* the burger stands of the 4 players */
		AR_P(0x4240) = bLoadResource(NULL, "burgers\\burger.obe", 1, 0xdefa);
		AR_P(0x4244) = bLoadResource(NULL, "burgers\\burgerb.obe", 1, 0xdefa);
		AR_P(0x4248) = bLoadResource(NULL, "burgers\\burgerc.obe", 1, 0xdefa);
		AR_P(0x424c) = bLoadResource(NULL, "burgers\\burgerd.obe", 1, 0xdefa);
		for (i = 0; i < 4; i++) {
			char *inst = AR_A(0x4260) + i * 0x170;

			baSetRenderPass((struct BActorInstance *)inst, 0, 1, 0);
			baSetRenderPass((struct BActorInstance *)inst, 1, 0, 0);
			baInitActorInstance((struct BActorInstance *)inst, (struct BActor *)AR_P(0x4240), 8, s_ArenaFile, 0x1ff);
			MW_AF(inst, 0xd8) = 1.1f;
			MW_AF(inst, 0xd4) = 1.1f;
			MW_AF(inst, 0xd0) = 1.1f;
			AR_S(0x4838 + i * 2) = 0;
			AR_S(0x2c9c + i * 2) = 1;
			AR_S(0x4250 + i * 2) = 0;
			AR_S(0x2c10 + i * 2) = 0;
			AR_F(0x4820 + i * 4) = 1.0f;
		}
		/* burger gibs */
		ar_LoadInstance(self, 0x2cb0, "effects\\brgrgiba.obe", 1, 0x209);
		ar_LoadInstance(self, 0x2e20, "effects\\brgrgibb.obe", 1, 0x20b);
		ar_LoadInstance(self, 0x2f90, "effects\\brgrgibc.obe", 1, 0x20d);
		AR_F(0x2d88) = 2.0f;
		AR_F(0x2d84) = 2.0f;
		AR_F(0x2d80) = 2.0f;
		AR_F(0x2ef8) = 2.0f;
		AR_F(0x2ef4) = 2.0f;
		AR_F(0x2ef0) = 2.0f;
		AR_F(0x3068) = 2.0f;
		AR_F(0x3064) = 2.0f;
		AR_F(0x3060) = 2.0f;
		for (i = 0; i < 4; i++)
			memcpy(MW_A(0x0030e4f0) + i * 0x170, MW_FV(0x002b2718 + i * 0x10), 0x10);
		break;
	case 3:
		/* Pied Pipers: the spinning bug traps (names by character at 0x002b2838 and
		0x002b28f8) and their electric fields */
		ar_SetBugCounts(self, pipersStart, pipersEnd);
		ar_ResetRoundTimer(self);
		AR_S(0x2c30) = 0;
		AR_I(0x2c94) = 0;
		AR_S(0x2c98) = 0;
		AR_S(0x2c9a) = 0;
		for (i = 0; i < 4; i++) {
			char *trap = AR_A(0x15f0) + i * 0x170;
			char *field = AR_A(0x1bd0) + i * 0x170;
			float *at = MW_FV(0x002b2d08 + i * 0x10);

			model = bLoadResource(NULL, MW_A(0x002b2838) + DAT_002baed4[i * 10] * 0x20, 1, 0xdefa);
			AR_P(0x15e0) = model;
			baInitActorInstance((struct BActorInstance *)trap, (struct BActor *)model, 4, s_ArenaFile, 0x231);
			memcpy(trap + 0xc0, at, 0x10);
			AR_P(0x1bb0 + i * 4) = baFindSubObject((struct BActorInstance *)trap, "spin");
			baFlushAnimations(trap, (struct BActorNode *)MW_AP(trap, 0x134), 1, 0);
			MW_AI(AR_P(0x1bb0 + i * 4), 0xc) = 0x400;
			baQueueAnimation((struct BActorInstance *)trap, (struct BActorNode *)MW_AP(trap, 0x134),
				(struct BAnim *)AR_P(0x1bb0 + i * 4), 0x400, -1, 1, NULL, NULL);
			model = bLoadResource(NULL, MW_A(0x002b28f8) + DAT_002baed4[i * 10] * 0x20, 1, 0xdefa);
			AR_P(0x1bc0) = model;
			baInitActorInstance((struct BActorInstance *)field, (struct BActor *)model, 4, s_ArenaFile, 0x23b);
			memcpy(field + 0xc0, at, 0x10);
			AR_P(0x2190 + i * 4) = baFindSubObject((struct BActorInstance *)field, "fizz");
			baFlushAnimations(field, (struct BActorNode *)MW_AP(field, 0x134), 1, 0);
			baQueueAnimation((struct BActorInstance *)field, (struct BActorNode *)MW_AP(field, 0x134),
				(struct BAnim *)AR_P(0x2190 + i * 4), 0x400, -1, 1, NULL, NULL);
		}
		model = bLoadResource(NULL, "models\\bugtraps\\textures\\elecfield.gif", 0, 0xdefa);
		AR_P(0x21a0) = model;
		AR_F(0x21a4) = 0.0f;
		AR_F(0x21ac) = 0.3f;
		AR_S(0x21a8) = (short)(MW_AB(model, 0x37) - 1);
		ar_SumBugsToSpawn(self);
		AR_S(0x2c92) = 1;
		AR_S(0x2c6c) = 1;
		AR_S(0x2c6e) = 1;
		AR_S(0x2c70) = 4;
		AR_S(0x2c72) = 1;
		/* the coloured worms and roaches (by player colour) */
		for (i = 0; i < 6; i++) {
			MW_P(0x0030a634 + i * 4) = NULL;
			MW_P(0x0030aef0 + i * 4) = NULL;
		}
		MW_P(0x0030a634) = bLoadResource(NULL, "models\\g-worm.obe", 1, 0xdefa);
		MW_P(0x0030a638) = bLoadResource(NULL, "models\\r-worm.obe", 1, 0xdefa);
		MW_P(0x0030a63c) = bLoadResource(NULL, "models\\b-worm.obe", 1, 0xdefa);
		MW_P(0x0030a640) = bLoadResource(NULL, "models\\y-worm.obe", 1, 0xdefa);
		MW_P(0x0030a644) = bLoadResource(NULL, "models\\o-worm.obe", 1, 0xdefa);
		MW_P(0x0030a648) = bLoadResource(NULL, "models\\p-worm.obe", 1, 0xdefa);
		MW_P(0x0030aef0) = bLoadResource(NULL, "models\\g-roach.obe", 1, 0xdefa);
		MW_P(0x0030aef4) = bLoadResource(NULL, "models\\r-roach.obe", 1, 0xdefa);
		MW_P(0x0030aef8) = bLoadResource(NULL, "models\\b-roach.obe", 1, 0xdefa);
		MW_P(0x0030aefc) = bLoadResource(NULL, "models\\y-roach.obe", 1, 0xdefa);
		MW_P(0x0030af00) = bLoadResource(NULL, "models\\o-roach.obe", 1, 0xdefa);
		MW_P(0x0030af04) = bLoadResource(NULL, "models\\p-roach.obe", 1, 0xdefa);
		for (i = 0; i < 6; i++) {
			if (MW_P(0x0030a634 + i * 4) == NULL)
				bkPrintf("Failed to load coloured worm bug num %d\n", i);
			if (MW_P(0x0030aef0 + i * 4) == NULL)
				bkPrintf("Failed to load coloured roach bug num %d\n", i);
		}
		break;
	}
	AR_S(0x2c34) = 0;
	AR_S(0x2c36) = 0;
	AR_I(0x2c38) = 0;
}

/* 0x001725d0  (gap): choosePlayerToAttack: a player picked at random, weighted by the
attack weights (+0x4820) */
short CBbash_Arena_ChoosePlayerToAttack(void *self)
{
	float total, r;
	short i;

	total = 0.0f;
	for (i = 0; i < 4; i++)
		total = total + AR_F(0x4820 + i * 4);
	r = total * (float)mw_rand() * 3.051851e-05f;
	for (i = 0; i < 4; i++) {
		if (r <= AR_F(0x4820 + i * 4))
			return i;
		r = r - AR_F(0x4820 + i * 4);
	}
	bkPrintf("ERROR in choosePlayerToAttack\n");
	return 3;
}

/* 0x00172660  (gap): decideBugType: one of the 5 bug types picked at random, weighted by
the bugs of each type still to spawn; -1 if none */
int CBbash_Arena_DecideBugType(void *self)
{
	short left[5];
	short total, sum, r;
	short i;

	total = 0;
	for (i = 0; i < 5; i++) {
		left[i] = (short)(AR_S(0x2c54 + i * 2) - AR_S(0x2c60 + i * 2));
		total = (short)(total + left[i]);
	}
	if (total == 0) {
		bkPrintf("Error in decideBugType no bugs to choose\n");
		return -1;
	}
	sum = 0;
	for (i = 0; i < 5; i++) {
		left[i] = (short)(left[i] + sum);
		sum = left[i];
	}
	r = (short)(mw_rand() % (int)total);
	for (i = 0; i < 5; i++) {
		if (left[i] > 0 && r <= left[i])
			return (int)i;
	}
	bkPrintf("error no selection\n");
	return -1;
}

/* 0x00172730  (missing from the table): reset for a new round: deletes the bugs, clears
the spawn counts and gives the variation's counts again */
void FUN_00172730(void *self)
{
	static const short brawl[5] = { 2, 3, 0, 3, 0 };
	static const short tremors[5] = { 5, 0, 1, 5, 0 };
	unsigned char *node, *next;
	int i;

	AR_S(0x460) = -1;
	node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL);
	while (node != NULL) {
		next = (unsigned char *)(uintptr_t)CLList_Remove((int *)DAT_0030ee30, (int *)node);
		Actor_Delete(MW_AP(node - 8, 0x0));
		FUN_001817d0(node - 8);
		MW_DELETE(node - 8);
		node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)next);
	}
	for (i = 0; i < 7; i++) {
		AR_S(0x2c54 + i * 2) = 0;
		AR_S(0x2c60 + i * 2) = 0;
		AR_S(0x2c6c + i * 2) = 0;
	}
	AR_S(0x2c6c) = 1;
	AR_S(0x2c6e) = 1;
	AR_S(0x2c70) = 4;
	AR_S(0x2c72) = 1;
	FUN_00182c10();
	AR_S(0x2c8e) = 0;
	AR_S(0x2c90) = 0;
	for (i = 0; i < 4; i++) {
		MW_AI(mw_Players[i], 0x1364) = 0;
		AR_F(0x4820 + i * 4) = 1.0f;
		AR_S(0x2c9c + i * 2) = 1;
		AR_S(0x2c10 + i * 2) = 0;
	}
	if (AR_S(0x0) == 0) {
		AR_S(0x2c92) = 1;
		ar_SetBugCounts(self, brawl, brawl);
	} else if (AR_S(0x0) == 1) {
		AR_S(0x2c92) = 1;
		ar_SetBugCounts(self, tremors, tremors);
		AR_S(0x2c94) = 0;
		AR_S(0x2c96) = 0;
		AR_S(0x2c98) = 0;
		AR_S(0x2c9a) = 0;
	}
	ar_SumBugsToSpawn(self);
	AR_S(0x2c34) = 0;
	AR_S(0x2c36) = 0;
	AR_S(0x2c38) = 0;
	AR_S(0x2c3a) = 0;
	MW_Timer_Reset(AR_A(0x8));
	AR_S(0x2c30) = 0;
	AR_F(0x2c18) = 5000.0f;
	AR_F(0x2c1c) = 5000.0f;
}
