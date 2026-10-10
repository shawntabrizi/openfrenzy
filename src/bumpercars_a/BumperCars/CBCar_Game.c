/*
BumperCars/CBCar_Game.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Game.cpp   (module bumpercars_a)

CBCar_Game: the BumperCars minigame object (one static instance, DAT_002ed458). It loads
the packages (packages\bumper and packages\bump-vNN), reads game.cfg (the section of its
variation), initialises its parts in order (timer, debris effects, sound, light, score,
camera, goal, world, players, debug), runs a frame (Process) and draws it (Draw).

The functions at 0x00087400, 0x00087800 and 0x00088330 are tagged with this file; the others
are methods of the same object between and around them (gap functions; file inferred from
the class). 0x00086c80, 0x00086c90, 0x00086ea0, 0x00086ed0, 0x00086f80 and 0x000874c0 are
missing from the function table: bumpercars_b's CGameMode_BumperCars (vtable 0x0026684c)
and CModule_BumperCars reach them by jumps. They are defined here under their FUN_ names.
*/

#include "bumpercars_a.h"
#include <strings.h>

static const char s_GameFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Game.cpp";

/* classlib_ui globals: the HUD object (Hud.cpp) and the "game over" overlay (0x002ecd90) */
extern void *DAT_002ecfac;
extern unsigned char DAT_002ecd90[];
/* classlib_ui: the game is paused */
extern int DAT_002ecdec;
/* core: per-player state, 0x28 bytes each (+0x00 in game, +0x10 team, +0x1c character) */
extern int DAT_002baed8[];
/* core: 0 = free for all; else the team game (DAT_002baef4 per player: team) */
extern int DAT_002baebc;
extern int DAT_002baeb8;
/* core: the light colour (r, g, b, a ints) and ambient values the minigames override */
extern int DAT_002ba940[4];
/* 0x00290ee4  BumperCars fog/ambient colour (0xbb, 0xcd, 0xfe, 0xff), near and far planes */
extern int DAT_00290ee4[4];
extern float DAT_00290ef4;
extern float DAT_00290ef8;
/* 0x0025633e  an empty string ("" and L"") */
extern const char DAT_0025633e[];

/* 0x002f2068  the start positions: x, y, z, w (1.0 when set), heading, by start slot
(a global after the game object; bumpercars_b fills it) */
extern float DAT_002f2068[][5];

/* 0x00263b80  CBCar_Game vtable */
const void *const vtable_00263b80[1] = {
	(const void *)FUN_000873e0, /* [0] scalar deleting destructor */
};

/* classlib_ui: the rumble menu object (0x002ed324, CRumble.cpp) */
extern unsigned char DAT_002ed324[];
/* core: the mode stack (0x002ba750) and the command line settings (0x002ba9a0) */
extern unsigned char DAT_002ba750[];
extern struct Settings *DAT_002ba9a0;

/* ---------------------------------------------------------------- calls into bumpercars_b
(assumed shapes; see BCA_FN in bumpercars_a.h) */
#define Timer_Init(t, frames) BCA_FN(FUN_00096550, int, (void *, int))((t), (frames))
#define Timer_Reset(t) BCA_FN(FUN_00096630, void, (void *))(t)
#define Timer_Start(t) BCA_FN(FUN_000965d0, void, (void *))(t)
#define Timer_FrameTime(t) BCA_FN(FUN_000965b0, float, (void *))(t)
#define Timer_Elapsed(t) BCA_FN(FUN_000965e0, float, (void *))(t)
#define Timer_TimeLeft(t) BCA_FN(FUN_000965f0, float, (void *))(t)
#define Timer_Time(t) BCA_FN(FUN_00096600, float, (void *))(t)

/* ======================================================================== construction */

/* 0x00086b80  (gap): constructor of a player slot (the array constructor's callback) */
void *CBCar_Game_PlayerSlotCtor(void *slot)
{
	FUN_00089020((char *)slot + 0x10);
	return slot;
}

/* 0x00086b90  (missing from the table): destructor of a player slot (array callback) */
void FUN_00086b90(void *slot)
{
	FUN_00089070((char *)slot + 0x10);
}

/* 0x00086ba0  (gap): CBCar_Game::~CBCar_Game (the parts in reverse order; most of their
destructors are empty) */
void CBCar_Game_Dtor(struct CBCar_Game *self)
{
	int i;

	self->vtable = vtable_00263b80;
	FUN_001d5950(); /* list (0x4bc4) */
	FUN_001d5950(); /* timer (0x4b50) */
	FUN_001d5950(); /* 0x3f28 */
	FUN_000924f0(self->light);
	FUN_001d5950(); /* sound (0x3320) */
	FUN_000919f0(self->goal);
	for (i = 3; i >= 0; i--)
		FUN_00086b90(self->players[i]);
	FUN_0008d1ee(self->camera);
	FUN_001d5950(); /* effects (0x5c) */
	FUN_0008bf90(self->debug);
}

/* 0x00086c80  (missing from the table): CModule_BumperCars::Init (via 0x00082910) */
int FUN_00086c80(struct CBCar_Game *self)
{
	self->started = 0;
	return 1;
}

/* 0x00086c90  (missing from the table): the mode was pushed (CGameMode_BumperCars enter):
loads the character voices and the two packages */
void FUN_00086c90(struct CBCar_Game *self)
{
	char name[64];
	struct bkPackage *package;

	BCA_I32(self->mode, 0x14) = 1;
	self->started = 0;
	self->state = 0;
	Core_LoadCharacterVoices(1);
	package = BCA_FN(FUN_00071f90, struct bkPackage *, (const char *, int))("packages\\bumper", 0);
	self->package = package;
	if (package != NULL) {
		snprintf(name, 0x3f, "packages\\bump-v%02d", self->variation);
		package = BCA_FN(FUN_00071f90, struct bkPackage *, (const char *, int))(name, 0);
		self->variantPackage = package;
		if (package != NULL)
			self->state = 1;
	}
}

/* 0x00086d10  (gap): starts the round: HUD timer and scores, the RefBot, the parts' start,
the crowd, and the BumperCars light colour (the core one is saved) */
void CBCar_Game_Start(struct CBCar_Game *self)
{
	int zero[4];
	int off;

	if (self->state == 0)
		return;
	self->started = 1;
	FUN_00072720((undefined4)(uintptr_t)self->list);
	CProjector_Reset();
	RefBot_Create();
	BCA_FN(FUN_000796c0, void, (void *))(DAT_002ecfac);
	BCA_FN(FUN_00079490, void, (void *, float))(DAT_002ecfac, self->gameTime);
	BCA_FN(FUN_00078930, void, (void *, float))(DAT_002ecfac, 10.0f);
	BCA_FN(FUN_0007bfd0, void, (void *))(DAT_002ecfac);
	zero[0] = 0;
	zero[1] = 0;
	zero[2] = 0;
	zero[3] = 0;
	BCA_FN(FUN_0007a080, void, (void *))(DAT_002ecfac);
	if (self->variation == 4)
		BCA_FN(FUN_0007a360, void, (void *, const void *, int))(DAT_002ecfac, DAT_0025633e, 1);
	else
		BCA_FN(FUN_0007a200, void, (void *, int *))(DAT_002ecfac, zero);
	for (off = 0xa8; off < 0xb8; off += 4)
		BCA_F32(DAT_002ecfac, off) = 50.0f;
	FUN_00099fc0();
	FUN_0008c9f0(self->debug);
	FUN_00096bd0();
	BCA_I32(self, 0x254c) = 1;
	Timer_Start(self->timer);
	FUN_00092120(self->sound, 0xd, 200, 0, -1);
	self->savedLight[1] = DAT_002ba940[1];
	self->savedLight[0] = DAT_002ba940[0];
	self->savedLight[2] = DAT_002ba940[2];
	DAT_002ba940[0] = DAT_00290ee4[0];
	self->savedLight[3] = DAT_002ba940[3];
	DAT_002ba940[3] = DAT_00290ee4[3];
	DAT_002ba940[1] = DAT_00290ee4[1];
	DAT_002ba940[2] = DAT_00290ee4[2];
	if (DAT_002baeb8 != 0)
		FUN_0008ea74(self->camera);
}

/* 0x00086ea0  (missing from the table): the mode is back on top */
void FUN_00086ea0(struct CBCar_Game *self)
{
	if (self->state != 0 && self->started != 0) {
		FUN_0008c9f0(self->debug);
		FUN_00096bd0();
	}
}

/* 0x00086ed0  (missing from the table): another mode goes on top: the players' engine
sounds stop */
void FUN_00086ed0(struct CBCar_Game *self)
{
	int i;

	if (self->state == 0 || self->started == 0)
		return;
	FUN_001d5950(); /* debug (0x18): empty */
	for (i = 0; i < 4; i++) {
		if (BCAR_PLAYER(self, i, 0, int) != 0)
			FUN_00096b00(i);
	}
}

/* 0x00086f00  (gap): pause */
void CBCar_Game_Pause(struct CBCar_Game *self)
{
	BCA_FN(FUN_001c3770, void, (void *))(self->timer);
	FUN_0009ed60();
	FUN_00091b30(self->goal);
	FUN_00096cb0();
	FUN_0009ae40();
	FUN_000922b0();
}

/* 0x00086f40  (gap): resume from the pause */
void CBCar_Game_Unpause(struct CBCar_Game *self)
{
	if (self->started == 0)
		return;
	Timer_Start(self->timer);
	FUN_0009ed80();
	FUN_00091b30(self->goal);
	FUN_00096d00();
	FUN_0009ae90();
	FUN_000922c0();
}

/* 0x00086f80  (missing from the table): the mode update (not on top: the mode's own
virtual at +0x24 runs) */
void FUN_00086f80(struct CBCar_Game *self, int top)
{
	void *mode = self->mode;

	if (top == 0 && BCA_I32(mode, 0x14) != 0)
		BCA_VFN(mode, 0x24, void (*)(void *))(mode);
}

/* 0x00086fa0  (gap): draws the scene for view `view` in pass `pass` (0: everything,
1: without the world details, 2: the reflection) */
void CBCar_Game_DrawScene(struct CBCar_Game *self, int view, int pass)
{
	unsigned int fog;

	if (FUN_0008c000(self->debug) == 0 && BCG(0x3614, int) != 0)
		fog = 1;
	else
		fog = 0;
	bdSetRenderState(3, fog, 0);
	if (pass == 0) {
		FUN_0009f930(view, 0);
		FUN_0009bc50(view, 0);
		bdSetRenderState(3, fog, 0);
		FUN_000994b0(view);
		BCA_FN(FUN_001ec260, void, (void *, int))(self->goal, view);
		bdSetRenderState(3, fog, 0);
		FUN_00099710(view);
		bdSetRenderState(8, 0, 0);
		FUN_000a3080(view, 0);
		FUN_000a1d30(view);
		FUN_000a13d0(view, 0);
		bdSetRenderState(3, fog, 0);
		FUN_0008cb90(self->effects, view);
		FUN_00092680(self->light, (byte)view);
		FUN_000a3c40(view);
		if (self->explosions != NULL)
			BCA_FN(FUN_000a3af0, void, (void *, int))(self->explosions, view);
		if (self->floatyScores == NULL)
			goto done;
	} else if (pass == 1) {
		FUN_0009f930(view, 0);
		FUN_0009bc50(view, 0);
		bdSetRenderState(3, fog, 0);
		FUN_00099710(view);
		bdSetRenderState(8, 0, 0);
		FUN_000a3c40(view);
		if (self->explosions != NULL)
			BCA_FN(FUN_000a3af0, void, (void *, int))(self->explosions, view);
		if (self->floatyScores == NULL)
			goto done;
	} else if (pass == 2) {
		FUN_0009f930(view, 1);
		bdSetRenderState(8, 0, 0);
		FUN_00092680(self->light, (byte)view);
		goto done;
	} else {
		return;
	}
	BCA_FN(FUN_000a0ba0, void, (void *))(self->floatyScores);
done:
	bdSetRenderState(8, FUN_0008c060(self->debug), 0);
}

/* ======================================================================== settings */

/* 0x00087140  (gap): clears the settings before game.cfg is read */
void CBCar_Game_ClearSettings(struct CBCar_Game *self)
{
	self->lightInfo[0] = 0;
	self->worldInfo[0] = 0;
	self->playerInfo[0] = 0;
	self->scoreInfo[0] = 0;
	self->cameraInfo[0] = 0;
	self->goalInfo[0] = 0;
	Timer_Reset(self->timer);
	BCA_I32(self, 0x330c) = 0;
	BCA_I32(self, 0x3318) = 0;
	BCA_I32(self, 0x331c) = 0;
	BCA_I32(self, 0x3308) = 0;
	BCA_I32(self, 0x3614) = 0;
	self->playerGoals = 0;
	self->unknown4bd4 = 0;
	BCA_I32(self, 0x32d0) = 0;
	BCA_I32(self, 0x32e8) = 0;
	BCA_I32(self, 0x32d8) = 0;
	BCA_I32(self, 0x32d4) = 0;
	BCA_I32(self, 0x32dc) = 0;
	BCA_I32(self, 0x32e4) = 0;
	BCA_I32(self, 0x32e0) = 0;
	BCA_I32(self, 0x32f4) = 0;
	BCA_I32(self, 0x3618) = 0;
	BCA_I32(self, 0x3620) = 0;
	BCA_I32(self, 0x3f10) = 0;
	BCA_I32(self, 0x3f24) = 0;
	BCA_I32(self, 0x3f20) = 0;
	BCA_I32(self, 0x3f1c) = 0;
	BCA_I32(self, 0x361c) = 0;
}

/* 0x00087200  (gap): ranks the players by their score (core, 0x28 bytes per player at
0x002baee8, bubble sort) and gives each slot 3 - rank */
void CBCar_Game_RankPlayers(struct CBCar_Game *self)
{
	extern int DAT_002baee8[];
	int order[4];
	int n, i, swap;

	order[0] = 0;
	order[1] = 1;
	order[2] = 2;
	order[3] = 3;
	for (n = 3; n > 0; n--) {
		for (i = 0; i < n; i++) {
			swap = order[i + 1];
			if (DAT_002baee8[swap * 10] < DAT_002baee8[order[i] * 10]) {
				order[i + 1] = order[i];
				order[i] = swap;
			}
		}
	}
	BCA_I32(self, 0x2a20) = 3 - order[0];
	BCA_I32(self, 0x2d00) = 3 - order[1];
	BCA_I32(self, 0x2fe0) = 3 - order[2];
	BCA_I32(self, 0x32c0) = 3 - order[3];
}

/* 0x000872b0  (gap): CBCar_Game::CBCar_Game (the static constructor of DAT_002ed458) */
struct CBCar_Game *CBCar_Game_Ctor(struct CBCar_Game *self)
{
	int i;

	self->vtable = vtable_00263b80;
	FUN_0008bf50(self->debug);
	BCA_FN(FUN_00162f20, void, (void *))(self->effects);
	FUN_0008d1e0(self->camera);
	for (i = 0; i < 4; i++)
		CBCar_Game_PlayerSlotCtor(self->players[i]);
	FUN_000919e0(self->goal);
	FUN_00091ef0(self->sound);
	FUN_000924e0(self->light);
	FUN_00094360(self->unknown3f28);
	BCA_FN(FUN_00096320, void, (void *))(self->timer);
	CLList_Init(self->list);
	self->mode = NULL;
	BCA_I32(self, 0x330c) = 0;
	BCA_I32(self, 0x3318) = 0;
	BCA_I32(self, 0x331c) = 0;
	BCA_I32(self, 0x3308) = 0;
	BCA_I32(self, 0x32d8) = 0;
	self->unknown4bb0[0] = 0;
	BCA_I32(self, 0x3618) = 0;
	BCA_I32(self, 0x3f10) = 0;
	self->started = 0;
	self->unknown4bd4 = 0;
	self->unknown4b4c = -1;
	return self;
}

/* 0x000873e0  (gap): CBCar_Game scalar deleting destructor */
struct CBCar_Game *CBCar_Game_Destroy(struct CBCar_Game *self, byte flags)
{
	CBCar_Game_Dtor(self);
	if ((flags & 1) != 0)
		BCA_DELETE(self);
	return self;
}

/* 0x00087400  BumperCars/CBCar_Game.cpp: makes the game mode and pushes it. The variation
is the command line's "variation" setting if there is one. Returns 1 when pushed. */
int CBCar_Game_PushMode(struct CBCar_Game *self, int variation)
{
	void *mode;
	char *value;

	mode = BCA_NEW(0x2c, s_GameFile, 0x6c, 0x1001);
	if (mode != NULL)
		mode = BCA_FN(FUN_000a4cb0, void *, (void *))(mode);
	value = Settings_Get(DAT_002ba9a0, "variation");
	if (value != NULL)
		variation = atol(value);
	self->variation = variation;
	self->mode = mode;
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode, 1) != 0)
		return 1;
	self->mode = NULL;
	return 0;
}

/* 0x000874c0  (missing from the table): the mode draw: render states, the fog for the
variation, the camera, the scene in two passes, the goal flash, then (with the debug
overlay off) the HUD's player bars over the overlays */
void FUN_000874c0(struct CBCar_Game *self, int top)
{
	float bars[4];
	float fogNear, fogFar;
	int view, i, best;
	unsigned char *player;

	if (top == 0 || self->state == 0)
		return;
	bdSetProjectionMode(1);
	bdSetRenderState(0xd, 0, 1);
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 1, 0);
	bdSetNearFar(DAT_00290ef4, DAT_00290ef8);
	bdSetFieldOfView(0x1.38c354p+0f, 0x1.d524fep-1f);
	baSetBrightness(0x100);
	bdSetFogColour((unsigned int)DAT_00290ee4[0], (unsigned int)DAT_00290ee4[1], (unsigned int)DAT_00290ee4[2]);
	bdSetRenderState(8, FUN_0008c060(self->debug), 0);
	switch (self->variation) {
	case 1:
		fogFar = 22000.0f;
		fogNear = 10000.0f;
		break;
	case 2:
	case 3:
		fogFar = 22000.0f;
		fogNear = 12000.0f;
		break;
	case 4:
		fogFar = 28000.0f;
		fogNear = 19000.0f;
		break;
	default:
		fogFar = 30000.0f;
		fogNear = 12000.0f;
		break;
	}
	bdSetFogRange(fogNear, fogFar);
	view = -1;
	if (FUN_0009194d(self->camera, &view) != 0) {
		FUN_00091422(self->camera, view, 0,
			BCA_F32(*(void **)(self->players[view] + 0x138), 0xf4) * 0.5f);
		bdSetFieldOfView(0x1.eb7c16p+0f, 0x1.709d10p+0f);
	}
	CBCar_Game_DrawScene(self, 0, 0);
	CBCar_Game_DrawScene(self, 1, 0);
	if (view != -1 && BCA_AT(*(void **)(self->players[view] + 0x138), 0x8a4, short) != 0)
		BCA_FN(FUN_000a5390, void, (int, int, int, int))(0, 0, 0xff, 0x60);
	if (FUN_0008c050(self->debug) == 0)
		goto done;
	bdSetNearFar(1.0f, 10000.0f);
	COverlay_DrawAll(1);
	bars[0] = 0.0f;
	bars[1] = 0.0f;
	bars[2] = 0.0f;
	bars[3] = 1.0f;
	best = BCG(0x2580, int);
	for (i = 0; i < 4; i++) {
		player = self->players[i];
		if (BCA_I32(player, 0x270) != 0) {
			bars[i] = 1.0f;
		} else if (best != -1 && BCA_I32(player, 0x164) >= best) {
			bars[i] = 0.0f;
		} else if (BCG(0x2584, float) == 0.0f) {
			bars[i] = 1.0f;
		} else {
			bars[i] = 1.0f - (BCA_F32(player, 0x160) - Timer_Time(self->timer)) / BCG(0x2584, float);
			best = BCG(0x2580, int);
		}
	}
	BCA_FN(FUN_0007c080, void, (void *, float, float, float, float, float))(DAT_002ecfac, bars[0], bars[1],
		bars[2], bars[3], 1.0f);
	bdSetProjectionMode(1);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(7, 3, 0);
	bdSetRenderState(3, 1, 0);
	FUN_00091287(self->camera);
	bdSetFieldOfView(0x1.38c354p+0f, 0x1.d524fep-1f);
	bdSetNearFar(DAT_00290ef4, DAT_00290ef8);
done:
	FUN_0008fc39(self->camera);
}

/* 0x00087800  BumperCars/CBCar_Game.cpp: Game_LoadSettings: reads this variation's section
("variation%d {" ... "}") of game.cfg. Returns 1 when the section was found. */
int CBCar_Game_LoadSettings(struct CBCar_Game *self, struct BCarTextReader *reader)
{
	char word[512];
	char line[512];
	char *text;
	int found = 0;
	int status, number;

#define READ_LINE() BCA_FN(FUN_001386c0, char *, (void *, char *))(reader, line)
#define NEXT_LINE() BCA_FN(FUN_00138720, void, (void *))(reader)
#define GET_STRING(text, dest) BCA_FN(FUN_000a4e00, int, (char *, char *, int))((text), (dest), 0x1ff)
	CBCar_Game_ClearSettings(self);
	while (reader->cursor < reader->end) {
		text = READ_LINE();
		NEXT_LINE();
		if (strncmp(text, "//", 2) == 0)
			continue;
		snprintf(word, 0x1ff, "variation%d {", self->variation);
		if (strcasecmp(line, word) != 0)
			continue;
		found = 1;
		while (reader->cursor < reader->end) {
			if (strcasecmp(line, "}") == 0)
				break;
			text = READ_LINE();
			NEXT_LINE();
			if (strncmp(text, "//", 2) == 0)
				continue;
			status = 0;
			if (strncasecmp(text, "PLAYERINFO ", 0xb) == 0) {
				status = 2;
				if (GET_STRING(text, self->playerInfo) != 0)
					status = 1;
			}
			if (strncasecmp(text, "WORLDINFO ", 10) == 0) {
				status = 2;
				if (GET_STRING(text, self->worldInfo) != 0)
					status = 1;
			}
			if (strncasecmp(text, "SCOREINFO ", 10) == 0) {
				status = 2;
				if (GET_STRING(text, self->scoreInfo) != 0)
					status = 1;
			}
			if (strncasecmp(text, "LIGHTINFO ", 10) == 0) {
				status = 2;
				if (GET_STRING(text, self->lightInfo) != 0)
					status = 1;
			}
			if (strncasecmp(text, "CAMERAINFO ", 0xb) == 0) {
				status = 2;
				if (GET_STRING(text, self->cameraInfo) != 0)
					status = 1;
			}
			if (strncasecmp(text, "GOALINFO ", 9) == 0) {
				status = 2;
				if (GET_STRING(text, self->goalInfo) != 0)
					status = 1;
			}
			/* the original compares 9 characters of "GAMETIMER " */
			if (strncasecmp(text, "GAMETIMER ", 9) == 0) {
				status = 2;
				if (sscanf(text, "%*s%d", &number) == 1) {
					self->gameTime = (float)number;
					status = 1;
				}
			}
			if (strncasecmp(text, "PLAYERGOALS ", 0xc) == 0) {
				status = 2;
				if (sscanf(text, "%*s%d", &self->playerGoals) == 1)
					status = 1;
			}
			if (strcasecmp(text, "}") != 0) {
				sscanf(text, "%s", word);
				if (status == 0)
					BCA_Printf("%s(%d) : Game_LoadSettings(\"%s\", %d): Invalid Command \"%s\"\n", s_GameFile,
						0x54c, reader->name, reader->line, word);
				else if (status == 2)
					BCA_Printf("%s(%d) : Game_LoadSettings(\"%s\", %d): Bad use of Command \"%s\" (%s)\n",
						s_GameFile, 0x54e, reader->name, reader->line, word, text);
			}
		}
	}
	return found;
#undef READ_LINE
#undef NEXT_LINE
#undef GET_STRING
}

/* ======================================================================== the round */

/* 0x00087b60  BumperCars/CBCar_Game.cpp (bracketed): the end of a round: the cars return
to their start positions and play their win or lose animation */
void CBCar_Game_EndRound(struct CBCar_Game *self)
{
	float hit[3], height;
	int winners = 0, i, start;
	unsigned char *player;
	void *car;
	void *actor;

	FUN_000923d0(self->sound);
	self->unknown4bd4 = FUN_0008d81d(self->camera);
	CBCar_Game_RankPlayers(self);
	Timer_Reset(self->timer);
	BCG(0x4b44, int) = 4; /* DAT_002f1f9c: state = 4 (the round is over) */
	BCA_I32(self, 0x254c) = 1;
	FUN_00094950(self->unknown3f28, 0);
	for (i = 0; i < 4; i++) {
		player = self->players[i];
		car = *(void **)(player + 0x138);
		BCA_I32(DAT_002ecfac, 0xb8 + i * 4) = 0;
		if (BCA_I32(player, 0x154) != 0)
			FUN_00048d40(BCA_I32(player, 0x154));
		if (BCA_I32(player, 0x150) != 0)
			FUN_00048d40(BCA_I32(player, 0x150));
		BCA_F32(car, 0x3c) = 1.0f;
		BCA_I32(player, 0x2b4) = 0;
		BCA_AT(car, 0x8a6, short) = 0;
		BCA_F32(car, 0x38) = 1.0f;
		BCA_AT(car, 0x8a4, short) = 0;
		BCA_AT(car, 0x8a8, short) = 0;
		BCA_I32(car, 0x8b0) = 0;
		actor = *(void **)(player + 0x290);
		if (actor != NULL)
			baFlushAnimations(actor, *(struct BActorNode **)((char *)actor + 0x134), 1, 0);
		if ((DAT_002baebc == 0) ? (BCA_I32(car, 0x44) == 1) : (DAT_002baed8[i * 10 + 7] == self->unknown4bd4)) {
			/* a winner */
			winners++;
			if (BCA_I32(player, 0x2a8) != 0) {
				actor = *(void **)(player + 0x290);
				baQueueAnimation((struct BActorInstance *)actor, *(struct BActorNode **)((char *)actor + 0x134),
					*(struct BAnim **)(player + 0x2a8), 0x400, -1, 1, NULL, NULL);
				BCA_I32(player, 0x2b0) = 4;
			}
		} else if (BCA_I32(player, 0x2a0) != 0) {
			actor = *(void **)(player + 0x290);
			baQueueAnimation((struct BActorInstance *)actor, *(struct BActorNode **)((char *)actor + 0x134),
				*(struct BAnim **)(player + 0x2a0), 0x400, -1, 1, NULL, NULL);
			BCA_I32(player, 0x2b0) = 2;
		}
		start = BCA_I32(player, 0x2d0);
		if (DAT_002f2068[start][3] == 1.0f) {
			BCA_F32(car, 0x50) = DAT_002f2068[start][0];
			BCA_F32(car, 0x54) = DAT_002f2068[start][1];
			BCA_F32(car, 0x58) = DAT_002f2068[start][2];
			BCA_F32(car, 0x5c) = DAT_002f2068[start][3];
			BCA_F32(car, 0x84) = DAT_002f2068[BCA_I32(player, 0x2d0)][4];
			BCA_F32(car, 0x80) = BCA_F32(car, 0x84);
			height = BCA_FN(FUN_0009f6d0, float, (float *, float, float *))((float *)((char *)car + 0x50),
				BCA_F32(car, 0xd0), hit);
			BCA_F32(car, 0x54) = BCA_F32(car, 0x54) - (height - 1.0f);
			BCA_F32(car, 0xfc) = CLMath_WrapAngle(BCA_FN(FUN_000a5540, float, (float, float))(hit[2], hit[1]));
			BCA_F32(car, 0x104) = CLMath_WrapAngle(-BCA_FN(FUN_000a5540, float, (float, float))(hit[0], hit[1]));
		}
	}
	if (DAT_002baebc == 0)
		self->unknown4bd4 = winners;
	FUN_00092740(self->light, (char *)*(void **)(self->players[0] + 0x138) + 0x50);
}

/* the scoring entities of the world (bumpercars_b), a list from BCG(0x330c) (next at 0x8b8):
+0x00 kind (1: a car), +0x04 score, +0x08 total, +0x20 out of the game, +0x28 a 64-bit score,
+0x30 goals, +0x44 rank (-1: none yet), +0x4c distance */
#define ENT_NEXT(e) BCA_AT(e, 0x8b8, unsigned char *)
#define ENT_KIND(e) BCA_I32(e, 0x00)
#define ENT_SCORE(e) BCA_I32(e, 0x04)
#define ENT_TOTAL(e) BCA_I32(e, 0x08)
#define ENT_OUT(e) BCA_I32(e, 0x20)
#define ENT_SCORE64(e) BCA_AT(e, 0x28, longlong)
#define ENT_GOALS(e) BCA_U32(e, 0x30)
#define ENT_RANK(e) BCA_I32(e, 0x44)
#define ENT_DISTANCE(e) BCA_U32(e, 0x4c)
#define ENT_FIRST() BCG(0x330c, unsigned char *)       /* DAT_002f0764 */
#define RANK_NEXT BCG(0x2634, int)                     /* DAT_002efa8c: the next rank to give */

/* 0x00087e30  BumperCars/CBCar_Game.cpp (bracketed): ranks the cars when the round ends and
then calls CBCar_Game_EndRound. Knockout (BCG(0x2598) > 0): when all but one car are out,
the survivors rank first and the cars still in, by distance, last. Last car standing: the
one car that is out ranks first, the others by their score. Otherwise, when the time is up
(and the game is timed): variation 4 asks the camera for each car's place; the other
variations add rank, goals and score bonuses into the total and rank by it. */
void CBCar_Game_RankCars(struct CBCar_Game *self, int active)
{
	unsigned char *e, *pick;
	int cars, out, ranked, done, n, i;
	longlong best, limit, v;

	if (active == 0)
		return;
	if (BCG(0x2598, int) > 0) {
		cars = 0;
		out = 0;
		ranked = 0;
		for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
			if (ENT_KIND(e) == 1) {
				cars++;
				if (ENT_OUT(e) != 0) {
					out++;
					if (ENT_RANK(e) != -1)
						ranked++;
				}
			}
		}
		if (out - 1 <= ranked) {
			for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
				if (ENT_KIND(e) == 1 && ENT_RANK(e) == -1 && ENT_OUT(e) != 0) {
					ENT_RANK(e) = RANK_NEXT;
					break;
				}
			}
			if (cars != out) {
				RANK_NEXT = cars;
				for (;;) {
					best = 0;
					pick = NULL;
					done = 1;
					if (ENT_FIRST() == NULL)
						break;
					for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
						if (ENT_KIND(e) == 1 && ENT_OUT(e) == 0 && ENT_RANK(e) == -1) {
							done = 0;
							if ((longlong)ENT_DISTANCE(e) > best) {
								best = (longlong)(int)ENT_DISTANCE(e);
								pick = e;
							}
						}
					}
					if (pick != NULL) {
						ENT_RANK(pick) = RANK_NEXT;
						RANK_NEXT--;
					}
					if (done)
						break;
				}
			}
			CBCar_Game_EndRound(self);
			return;
		}
	}
	cars = 0;
	out = 0;
	for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
		if (ENT_KIND(e) == 1) {
			cars++;
			if (ENT_OUT(e) != 0)
				out++;
		}
	}
	if (ENT_FIRST() != NULL && cars != 1 && out == 1) {
		RANK_NEXT = out;
		for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
			if (ENT_KIND(e) == 1 && ENT_OUT(e) != 0) {
				ENT_RANK(e) = 1;
				RANK_NEXT++;
				break;
			}
		}
		for (;;) {
			best = 0;
			pick = NULL;
			done = 1;
			if (ENT_FIRST() == NULL)
				break;
			for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
				if (ENT_KIND(e) == 1 && ENT_OUT(e) == 0 && ENT_RANK(e) == -1) {
					done = 0;
					if (ENT_SCORE64(e) > best) {
						best = ENT_SCORE64(e);
						pick = e;
					}
				}
			}
			if (pick != NULL) {
				ENT_RANK(pick) = RANK_NEXT;
				RANK_NEXT++;
			}
			if (done)
				break;
		}
		CBCar_Game_EndRound(self);
		return;
	}
	if (self->gameTime == 0.0f)
		return;
	if (Timer_Elapsed(self->timer) < 1.0f)
		return;
	if (self->variation == 4) {
		for (i = 0; i < 4; i++)
			BCA_I32(*(void **)(self->players[i] + 0x138), 0x44) = FUN_0008fe4b(self->camera, i);
		CBCar_Game_EndRound(self);
		return;
	}
	/* the total: 100000 per place above 5th, 10000 when out, +1 for the most goals, plus
	   the score */
	for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
		if (ENT_KIND(e) == 1) {
			if (ENT_RANK(e) == -1)
				ENT_TOTAL(e) = 0;
			else
				ENT_TOTAL(e) = (5 - ENT_RANK(e)) * 100000;
		}
	}
	for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
		if (ENT_KIND(e) == 1 && ENT_OUT(e) != 0)
			ENT_TOTAL(e) += 10000;
	}
	if (BCG(0x259c, int) != -1) {
		best = 0;
		for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
			if (ENT_KIND(e) == 1 && (longlong)ENT_GOALS(e) > best)
				best = (longlong)(int)ENT_GOALS(e);
		}
		for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
			if (ENT_KIND(e) == 1 && (longlong)ENT_GOALS(e) == best)
				ENT_TOTAL(e)++;
		}
	}
	for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
		if (ENT_KIND(e) == 1)
			ENT_TOTAL(e) += ENT_SCORE(e);
	}
	/* the 64-bit scores from the highest down: each group of equal scores adds the next
	   place number (1 for the highest) to the total */
	RANK_NEXT = 1;
	limit = 0x7fffffffffffffffLL;
	for (;;) {
		best = 0;
		for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
			if (ENT_KIND(e) == 1) {
				v = ENT_SCORE64(e);
				if (v > best && v < limit)
					best = v;
			}
		}
		if (best == 0)
			break;
		n = 0;
		for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
			if (ENT_KIND(e) == 1 && ENT_SCORE64(e) == best) {
				ENT_TOTAL(e) += RANK_NEXT;
				n++;
			}
		}
		RANK_NEXT += n;
		limit = best;
	}
	/* the ranks by total, highest first (cars with equal totals share a rank) */
	RANK_NEXT = 1;
	limit = 0x7fffffffffffffffLL;
	for (;;) {
		best = 0;
		for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
			if (ENT_KIND(e) == 1) {
				v = (longlong)ENT_TOTAL(e);
				if (v > best && v < limit)
					best = v;
			}
		}
		if (best == 0)
			break;
		n = 0;
		for (e = ENT_FIRST(); e != NULL; e = ENT_NEXT(e)) {
			if (ENT_KIND(e) == 1 && (longlong)ENT_TOTAL(e) == best) {
				ENT_RANK(e) = RANK_NEXT;
				n++;
			}
		}
		RANK_NEXT += n;
		limit = best;
	}
	CBCar_Game_EndRound(self);
}

/* 0x00088330  BumperCars/CBCar_Game.cpp: CBCar_Game::InitGameOneTime / Initialise: reads
game.cfg and initialises the parts in order; on a failure the parts made so far shut down
and the state goes back to 0 */
void CBCar_Game_Initialise(struct CBCar_Game *self)
{
	struct BCarTextReader reader;
	uint size;
	void *text;
	void *object;
	const char *message;

	BCA_FN(FUN_000a5be0, void, (void *))(&reader);
	if (self->state == 0) {
		BCA_Printf("CBCar_Game::InitGameOneTime - Could not load one or more packages\n");
		goto done;
	}
	if (self->package == NULL || self->variantPackage == NULL) {
		message = "CBCar_Game::InitGameOneTime - Could not load one or more packages\n";
		goto fail_message;
	}
	bLoadPackageResources(self->package, 0xffffffff, 0xdefa, 0);
	bLoadPackageResources(self->variantPackage, 0xffffffff, 0xdefa, 0);
	text = bkLoadFile(self->package, "game.cfg", NULL, &size, NULL, 0);
	if (text == NULL) {
		BCA_Printf("CBCar_Game::Initialise - game.cfg FAILED!\n");
		self->state = 0;
		goto done;
	}
	BCA_FN(FUN_000a5c00, void, (void *, void *, uint, const char *))(&reader, text, size, "game.cfg");
	if (CBCar_Game_LoadSettings(self, &reader) == 0) {
		self->state = 0;
		goto done;
	}
	if (self->worldInfo[0] == 0 || self->playerInfo[0] == 0 || self->cameraInfo[0] == 0) {
		message = "CBCar_Game::Initialise - Initialisation file(s) not specified!\n";
		goto fail_message;
	}
	if (Timer_Init(self->timer, BCA_FTOL(self->gameTime)) == 0) {
		BCA_Printf("CBCar_Game::Initialise - timerSystem.Initialise() FAILED!\n");
		FUN_001d5950();
		goto fail;
	}
	if (FUN_0008cdc0(self->effects) == 0) {
		BCA_Printf("CBCar_Game::Initialise - debris.Initialise FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		goto fail;
	}
	if (FUN_00091fd0(self->sound) == 0) {
		BCA_Printf("CBCar_Sound::Initialise FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		FUN_000920b0(self->sound);
		goto fail;
	}
	if (self->lightInfo[0] != 0 && FUN_00094240(self->light, self->variantPackage, self->lightInfo) == 0) {
		BCA_Printf("CBCar_Light::Initialise FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		FUN_000920b0(self->sound);
		FUN_000927f0(self->light);
		goto fail;
	}
	if (self->scoreInfo[0] != 0 &&
	    BCA_FN(FUN_0009f180, int, (struct bkPackage *, char *))(self->variantPackage, self->scoreInfo) == 0) {
		BCA_Printf("CBCar_Game::Initialise - World_ScoreInit FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		FUN_000920b0(self->sound);
		if (self->lightInfo[0] != 0)
			FUN_000927f0(self->light);
		goto fail;
	}
	if (FUN_0008d1f9(self->camera, self->variantPackage, self->cameraInfo) == 0) {
		BCA_Printf("CBCar_Game::Initialise - Camera_Init FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		FUN_000920b0(self->sound);
		if (self->lightInfo[0] != 0)
			FUN_000927f0(self->light);
		FUN_0008e7f0(self->camera);
		goto fail;
	}
	if (self->goalInfo[0] != 0 && FUN_00091e30(self->goal, self->variantPackage, self->goalInfo) == 0) {
		BCA_Printf("CBCar_Goal::Initialise FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		FUN_000920b0(self->sound);
		if (self->lightInfo[0] != 0)
			FUN_000927f0(self->light);
		FUN_0008e7f0(self->camera);
		FUN_00091d70(self->goal);
		goto fail;
	}
	if (BCA_FN(FUN_000a0620, int, (struct bkPackage *, char *))(self->variantPackage, self->worldInfo) == 0) {
		BCA_Printf("CBCar_Game::Initialise - World_Init FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		FUN_000920b0(self->sound);
		if (self->lightInfo[0] != 0)
			FUN_000927f0(self->light);
		FUN_0008e7f0(self->camera);
		if (self->goalInfo[0] != 0)
			FUN_00091d70(self->goal);
		FUN_0009eda0();
		goto fail;
	}
	if (BCA_FN(FUN_0009a170, int, (struct bkPackage *, char *))(self->variantPackage, self->playerInfo) == 0) {
		BCA_Printf("CBCar_Game::Initialise - Player_Init FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		FUN_000920b0(self->sound);
		if (self->lightInfo[0] != 0)
			FUN_000927f0(self->light);
		FUN_0008e7f0(self->camera);
		if (self->goalInfo[0] != 0)
			FUN_00091d70(self->goal);
		FUN_0009eda0();
		FUN_000a4c20();
		FUN_0009abb0();
		FUN_000a1390();
		FUN_000a3450();
		FUN_000a1a70();
		goto fail;
	}
	if (FUN_0008c0c0(self->debug) == 0) {
		BCA_Printf("CBCar_Game::Initialise - debug.Initialise FAILED!\n");
		FUN_001d5950();
		FUN_0008cb60(self->effects);
		FUN_000920b0(self->sound);
		if (self->lightInfo[0] != 0)
			FUN_000927f0(self->light);
		FUN_0008e7f0(self->camera);
		if (self->goalInfo[0] != 0)
			FUN_00091d70(self->goal);
		FUN_0009eda0();
		FUN_000a4c20();
		FUN_0009c2d0();
		FUN_0009abb0();
		FUN_000a1390();
		FUN_000a3450();
		FUN_000a1a70();
		FUN_0008cae0(self->debug);
		goto fail;
	}
	object = BCA_NEW(8, s_GameFile, 0x1cc, 0x1001);
	if (object != NULL)
		object = BCA_FN(FUN_000a34b0, void *, (void *))(object);
	self->explosions = object;
	if (object == NULL)
		BCA_Printf("CBCar_Game::InitGameOneTime - Could not create Explosion System\n");
	object = BCA_NEW(8, s_GameFile, 0x1cf, 0x1001);
	if (object != NULL)
		object = BCA_FN(FUN_000a0c30, void *, (void *, int, float))(object, 0x10, 2.5f);
	self->floatyScores = object;
	if (object == NULL)
		BCA_Printf("CBCar_Game::InitGameOneTime - Could not create Floaty Score System\n");
	CLRumble_CreateMenu((int *)(void *)DAT_002ed324);
	self->unknown4bb0[0] = 0;
	self->paused = 0;
	goto done;

fail_message:
	BCA_Printf("%s", message);
fail:
	self->state = 0;
done:
	BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
}

/* 0x00088a10  (gap): the mode is popped: closes the packages and shuts the parts down;
the core light colour comes back */
void CBCar_Game_Shutdown(struct CBCar_Game *self)
{
	void *object;

	if (self->package != NULL) {
		bkClosePackageThunk(self->package);
		self->package = NULL;
	}
	if (self->variantPackage != NULL) {
		bkClosePackageThunk(self->variantPackage);
		self->variantPackage = NULL;
	}
	self->started = 0;
	bdSetRenderState(8, 0, 0);
	if (self->state == 0 || BCA_I32(self->mode, 4) == 0)
		return;
	FUN_001d5950();
	FUN_0008cb60(self->effects);
	FUN_000920b0(self->sound);
	FUN_000927f0(self->light);
	FUN_0008e7f0(self->camera);
	FUN_00091d70(self->goal);
	FUN_0009eda0();
	FUN_000a4c20();
	FUN_0009c2d0();
	FUN_0009abb0();
	FUN_000a1390();
	FUN_000a3450();
	FUN_000a1a70();
	FUN_0008cae0(self->debug);
	object = self->explosions;
	if (object != NULL) {
		BCA_FN(FUN_000a3bf0, void, (void *))(object);
		BCA_DELETE(object);
		self->explosions = NULL;
	}
	object = self->floatyScores;
	if (object != NULL) {
		BCA_FN(FUN_000a0c90, void, (void *))(object);
		BCA_DELETE(object);
		self->floatyScores = NULL;
	}
	CLRumble_Shutdown((int *)(void *)DAT_002ed324);
	COverlay_DeleteAll();
	CProjector_DeleteAll();
	RefBot_Delete();
	DAT_002ba940[0] = self->savedLight[0];
	DAT_002ba940[1] = self->savedLight[1];
	DAT_002ba940[2] = self->savedLight[2];
	DAT_002ba940[3] = self->savedLight[3];
	bkHeapDump();
}

/* 0x00088b70  (gap): the mode process: one frame of the game. Returns 1 while the HUD's
"game over" overlay still runs. */
int CBCar_Game_Process(struct CBCar_Game *self, int top)
{
	float dt, left;
	int state, i;

	if (top == 0)
		return 0;
	state = self->state;
	if (state == 0) {
		BCA_I32(self->mode, 0x28) = 1;
		return 0;
	}
	if (state != 5 && (state != 1 || Timer_Time(self->timer) > 1.0f) &&
	    BCA_FN(FUN_000772e0, int, (void *))(DAT_002ecd90) != 0)
		return 1;
	if (DAT_002ecdec != 0) {
		if (self->paused == 0) {
			CBCar_Game_Pause(self);
			self->paused = 1;
		}
		return 0;
	}
	if (self->paused != 0) {
		CBCar_Game_Unpause(self);
		self->paused = 0;
		return 0;
	}
	/* players who joined or left (core per-player "in game" flags) */
	for (i = 0; i < 4; i++) {
		if (BCAR_PLAYER(self, i, 0, int) == 0) {
			if (DAT_002baed8[i * 10] != 0) {
				FUN_000967b0(i);
				BCAR_PLAYER(self, i, 0, int) = 1;
			}
		} else if (DAT_002baed8[i * 10] == 0) {
			FUN_00096b00(i);
			FUN_000890f0(self->players[i] + 0x10);
			BCAR_PLAYER(self, i, 0, int) = 0;
		}
	}
	BCAR_PLAYER(self, 0, 0, int) = DAT_002baed8[0];
	BCAR_PLAYER(self, 3, 0, int) = DAT_002baed8[30];
	BCAR_PLAYER(self, 1, 0, int) = DAT_002baed8[10];
	BCAR_PLAYER(self, 2, 0, int) = DAT_002baed8[20];
	dt = Timer_FrameTime(self->timer);
	if (dt > 8.0f)
		dt = 8.0f;
	if (self->state == 3) {
		left = Timer_TimeLeft(self->timer);
		self->timeLeft = left;
		BCA_FN(FUN_000792f0, void, (void *, float, int))(DAT_002ecfac, left, 1);
		self->crowdTimer = self->crowdTimer - dt;
		if (self->crowdTimer <= 0.0f) {
			self->crowdTimer = (float)bca_rand() * 0x1.2c0258p-7f + 150.0f;
			FUN_00092120(self->sound, (bca_rand() % 2) + 0xf, 0xff, 0, -1);
		}
	} else {
		BCA_FN(FUN_000792f0, void, (void *, float, int))(DAT_002ecfac,
			(unsigned int)self->state < 3 ? self->gameTime : self->timeLeft, 1);
	}
	BCA_FN(FUN_00078c70, void, (void *, float))(DAT_002ecfac, dt);
	COverlay_UpdateAll();
	CProjector_UpdateAll();
	RefBot_Update();
	if (self->explosions != NULL)
		BCA_FN(FUN_000a3aa0, void, (void *, float))(self->explosions, dt);
	if (self->floatyScores != NULL)
		BCA_FN(FUN_000a0b70, void, (void *, float, float))(self->floatyScores, dt, FUN_0008e8d1(self->camera));
	FUN_0008bfa0(self->debug, self->state == 3);
	BCA_FN(FUN_0009f250, void, (float, int))(dt, (unsigned int)self->state >= 3);
	FUN_0008cb80(self->effects, dt * (1.0f / 60.0f));
	BCA_FN(FUN_0009d100, void, (float, int))(dt, self->state == 3);
	FUN_00091db0(self->goal, self->state == 3);
	BCA_FN(FUN_000a41e0, void, (int, float))(self->state == 3, dt);
	BCA_FN(FUN_000982f0, void, (float, int))(dt, self->state == 3);
	BCA_FN(FUN_000a11b0, void, (int))(self->state == 3);
	BCA_FN(FUN_000a2020, void, (int, float))(self->state == 3, dt);
	BCA_FN(FUN_000991f0, void, (int))((unsigned int)self->state >= 3);
	FUN_00092a70(self->light, self->state == 3, dt);
	BCA_FN(FUN_0009ae00, void, (int))(self->state == 3);
	FUN_0008d91d(self->camera);
	FUN_00092340(self->sound, dt);
	CBCar_Game_RankCars(self, (unsigned int)self->state <= 3);
	FUN_0008eceb(self->camera, self->state == 3, dt);
	BCA_FN(FUN_000a2d40, void, (int))(self->state == 3);
	return 0;
}
