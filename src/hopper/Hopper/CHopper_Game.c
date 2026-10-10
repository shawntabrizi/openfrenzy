/*
Hopper/CHopper_Game.c  <-  C:\BlitzParty\Game\Src\Hopper\CHopper_Game.cpp
(module hopper)

The game object (0x00302798, vtable 0x0026e448; core's static constructors construct it):
the game mode CGameMode "Hopper" forwards its virtuals to it, and it drives the sub-objects:
  0x002ae878 antics (CHopper_Antics.c)   0x002ae948 resources (CHopper_Resources.c)
  0x002aebd8 the camera                   0x002aec10 the arena (blades, arm, disc)
  0x002afe80 the rules (counters)        0x002affa0 the effects
  0x002b0038 the play (clock, HUD, end)  0x002b0378 the "anypressed" input
Only 0x0013f3a0 carries the file name; 0x0013eae0-0x0013f380 are placed here by address
(file inferred).

The object (0x44 bytes):
  +0x04 the game mode         +0x08 variation (hop_Variation)   +0x0c a list
  +0x18 byte: pause state (0 none, 1 may pause, 2 paused)
  +0x1c frame counter         +0x20 1: the game is over (scores handed to core)
  +0x24 the round is over (hop_RoundOver)   +0x28 a bdisplay statistic of the last frame
  +0x2c the character colour set the players have   +0x30 1: one human player (+0x34)
  +0x38, +0x3c the near and far planes before the game   +0x40
*/

#include "hopper.h"

#define GAME_I(self, off) HOP_AI(self, off)

extern int DAT_00283ea4;               /* core: the character colour set */
extern int DAT_00283ef0;               /* core: follow a single human player */
extern float DAT_0028ffa8;             /* bdisplay: the minigames' near plane */
extern float DAT_0028ffac;             /* bdisplay: the far plane */
extern int DAT_002bc48c;               /* bdisplay */
extern void *DAT_002f26c4;             /* bumpercars_b: the debug free camera */
extern unsigned char DAT_002ecd90[];   /* classlib_ui: the pause object */

/* classlib_ui 0x00071f90 returns the package it opened (its table prototype says void) */
#define hop_AddPackage(name, cutscene) ((void *(*)(const char *, int))FUN_00071f90)((name), (cutscene))

/* 0x0013eae0  (gap): the game object's constructor (core's static constructor) */
void *HopGame_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026e448;
	CLList_Init((int *)((char *)self + 0xc));
	return self;
}

/* 0x0013eb00  (gap): the game object's destructor (core's exit function) */
void HopGame_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026e448;
	FUN_001d5950();    /* the list's destructor (empty) */
}

/* 0x0013eb10  (gap): enter: starts loading packages\hop-v<variation> and sets the near and
far planes of the minigames (4000, 10000), keeping the old ones */
void HopGame_Enter(void *self, int variation)
{
	char name[64];

	GAME_I(self, 0x8) = variation;
	sprintf(name, "packages\\hop-v%d", variation);
	HOP_P(0x002ae98c) = hop_AddPackage(name, 0);
	if (HOP_P(0x002ae98c) == NULL)
		bkPrintf("ERROR hopper package failed to load\n");
	GAME_I(self, 0x2c) = DAT_00283ea4;
	HOP_AF(self, 0x38) = DAT_0028ffa8;
	HOP_AF(self, 0x3c) = DAT_0028ffac;
	DAT_0028ffa8 = 4000.0f;
	DAT_0028ffac = 10000.0f;
}

/* 0x0013eb90  (gap): start (the packages are loaded): starts the sub-objects, the HUD
(score boxes, scores, a 60 second clock for variations 1 and 2, bars), and the ambient
sounds of the variation */
void HopGame_Start(void *self)
{
	HopAntics_Start(hop_Antics);
	HopPlay_Start(HOP_ADDR(0x002b0038));
	HopCamera_Start(HOP_ADDR(0x002aebd8));
	HopArena_Start(HOP_ADDR(0x002aec10));
	HopRules_Start(HOP_ADDR(0x002afe80));
	HopFx_Start(HOP_ADDR(0x002affa0));
	FUN_0019d5c0();
	HOP_I(0x002b011c) = 0;
	HOP_I(0x002b0120) = 0;
	HOP_AU(HOP_AP(self, 0x4), 0x10) |= 1;
	FUN_00072720((undefined4)(uintptr_t)((char *)self + 0xc));
	CLHud_CreateScoreBoxes(DAT_002ecfac);
	CLHud_CreateScoreTexts(DAT_002ecfac, HOP_IV(0x002aff24));
	CLHud_CreateClock(DAT_002ecfac, GAME_I(self, 0x8) < 3 ? 60.0f : -1.0f);
	CLHud_CreateBars(DAT_002ecfac);
	HopRes_SetRenderStates(hop_Res);
	bdSaveRenderStates((unsigned int *)HOP_ADDR(0x002aea48));
	FUN_001d5950();
	bdRestoreRenderStates((unsigned int *)HOP_ADDR(0x002aea48));
	switch (GAME_I(self, 0x8)) {
	case 1:
		bsPlaySample(HOP_G(0x002ae9b0, struct BSample *), 0x96, 0, 0, 0);
		return;
	case 2:
		bsPlaySample(HOP_G(0x002ae9b4, struct BSample *), 0x96, 0, 0, 0);
		return;
	case 3:
	case 4:
		bsPlaySample(HOP_G(0x002ae9a8, struct BSample *), 200, 0, 0, 0);
		bsPlaySample(HOP_G(0x002ae9ac, struct BSample *), 200, 0, 0, 0);
		bsPlaySample(HOP_G(0x002ae9b0, struct BSample *), 100, 0, 0, 0);
		return;
	}
}

/* 0x0013ed10  (gap): exit: restores the near and far planes and shuts the sub-objects down */
void HopGame_Exit(void *self)
{
	DAT_0028ffa8 = HOP_AF(self, 0x38);
	DAT_0028ffac = HOP_AF(self, 0x3c);
	FUN_0019d810();
	HopFx_Shutdown(HOP_ADDR(0x002affa0));
	FUN_001d5950();    /* the rules (empty) */
	HopArena_Shutdown(HOP_ADDR(0x002aec10));
	FUN_001d5950();    /* the camera (empty) */
	FUN_001d5950();    /* the play (empty) */
	HopRes_Free(hop_Res);
	HopAntics_Shutdown(hop_Antics);
	HopRes_SetRenderStates(hop_Res);
}

/* 0x0013ed80  (gap; not in the function table): resume */
void HopGame_Resume(void *self)
{
	(void)self;
	HopInput_Resume(HOP_ADDR(0x002b0378));
	HopRes_SetRenderStates(hop_Res);
}

/* 0x0013eda0  (gap; not in the function table): suspend: deletes the "anypressed" input's
map (rowing_a 0x001af950, thiscall) */
void HopGame_Suspend(void *self)
{
	(void)self;
	FUN_001af950((int)(uintptr_t)HOP_ADDR(0x002b0378));
}

/* 0x0013edb0  (gap; not in the function table): update: while the mode loads its packages,
polls the loading (the mode's virtual 0x24) */
void HopGame_Update(void *self, int top)
{
	void *mode;

	if (top == 0) {
		mode = HOP_AP(self, 0x4);
		if (HOP_AI(mode, 0x14) != 0)
			HOP_VFN(mode, 0x24, void (*)(void *))(mode);
		return;
	}
	FUN_001d5950();
}

/* 0x0013ede0  (gap): gives the players the current character colour set when it changed,
and finds whether exactly one human plays (then the camera follows him) */
void HopGame_CheckPlayers(void *self)
{
	int humans, last, i;

	if (DAT_00283ea4 != GAME_I(self, 0x2c)) {
		GAME_I(self, 0x2c) = DAT_00283ea4;
		for (i = 0; i < DAT_002bae94; i++)
			Core_SetCharacterColours((char *)hop_PlayerActor(i) + 0x10, DAT_00283ea4);
	}
	if (DAT_00283ef0 != 0 && hop_Play == 5) {
		humans = 0;
		last = -1;
		if (DAT_002bae94 > 0) {
			for (i = 0; i < DAT_002bae94; i++) {
				if (DAT_002baed4[i][1] != 0) {
					humans++;
					last = i;
				}
			}
			if (humans == 1) {
				GAME_I(self, 0x34) = last;
				GAME_I(self, 0x30) = 1;
				return;
			}
		}
	}
	GAME_I(self, 0x30) = 0;
}

/* 0x0013ee80  (gap; not in the function table): process: one frame of the game. The pause
menu may end the game (the scores go to core: variation 3 hands over the finishing order);
otherwise the camera, the rules of the variation (or the idle players before the play), the
arena, the rules, the effects and the tokens update. */
void HopGame_Process(void *self, int top)
{
	unsigned char *pause = (unsigned char *)self + 0x18;

	if (top == 0)
		return;
	HopPlay_ProfileBegin(HOP_ADDR(0x002b0038), 0, "all");
	if (GAME_I(self, 0x20) == 1)
		return;
	HopGame_CheckPlayers(self);
	FUN_0019d870();
	DebrisColours_SetAll(HOP_AV(hop_PlayerActor(0), 0xd0), HOP_AV(hop_PlayerActor(1), 0xd0),
		HOP_AV(hop_PlayerActor(2), 0xd0), HOP_AV(hop_PlayerActor(3), 0xd0));
	if (*pause == 1) {
		if (DAT_002ecdec != 0) {
			*pause = 2;
			goto check_paused;
		}
		goto check_menu;
	}
check_paused:
	if (DAT_002ecdec == 0) {
		if (*pause == 2)
			*pause = 1;
		goto check_menu;
	}
	HOP_I(0x002b0120) = 0;
	goto pause_menu;
check_menu:
	if (HOP_I(0x002b0120) != 0)
		goto play;
pause_menu:
	if (HOP_I(0x002b036c) < 1 && CLPause_Update((undefined4 *)DAT_002ecd90) != 0) {
		if (hop_Variation == 3) {
			DAT_002baed4[0][5] = HOP_IV(0x002aff34)[0];
			GAME_I(self, 0x20) = 1;
			DAT_002baed4[1][5] = HOP_IV(0x002aff34)[1];
			DAT_002baed4[2][5] = HOP_IV(0x002aff34)[2];
			DAT_002baed4[3][5] = HOP_IV(0x002aff34)[3];
			return;
		}
		DAT_002baed4[0][5] = HOP_IV(0x002aff24)[0];
		GAME_I(self, 0x20) = 1;
		DAT_002baed4[1][5] = HOP_IV(0x002aff24)[1];
		DAT_002baed4[2][5] = HOP_IV(0x002aff24)[2];
		DAT_002baed4[3][5] = HOP_IV(0x002aff24)[3];
		return;
	}
play:
	if (GAME_I(self, 0x20) == 1)
		return;
	HopPlay_UpdateHud(HOP_ADDR(0x002b0038));
	if (DAT_002ecdec == 0) {
		HopCamera_Update(HOP_ADDR(0x002aebd8));
		if (*pause != 0 && hop_Play == 5) {
			HopAntics_Update(hop_Antics);
		} else if (hop_Play == 0) {
			AI_Tick(hop_Players[0]);
			AI_Tick(hop_Players[1]);
			AI_Tick(hop_Players[2]);
			AI_Tick(hop_Players[3]);
			Actor_UpdateAll();
			Actor_UpdateAll();
			Actor_UpdateAnimations();
		}
		HopArena_Update(HOP_ADDR(0x002aec10), 0);
		HopRules_Update(HOP_ADDR(0x002afe80));
		HopFx_Update(HOP_ADDR(0x002affa0));
		CToken_UpdateAll();
	}
	Shadows_Aim();
	GAME_I(self, 0x1c)++;
}

/* 0x0013f0a0  (gap; not in the function table): draw: fog (variation 4 doubles the fog end
before the play), the near plane (variation 1: 750 while playing, else 100; others 5), the
camera, the arena, the players, the effects, the tokens, the variation's extras and the
overlays; each part is timed by the play object's profile */
void HopGame_Draw(void *self, int top)
{
	int v;

	if (top == 0)
		return;
	HopPlay_ProfileBegin(HOP_ADDR(0x002b0038), 1, "render");
	if (GAME_I(self, 0x20) == 1)
		return;
	v = hop_Variation;
	if (GAME_I(self, 0x8) == 4 && hop_Play == 0)
		bdSetFogRange(HOP_FV(0x002aebb0)[v], HOP_FV(0x002aebc0)[v] + HOP_FV(0x002aebc0)[v]);
	else
		bdSetFogRange(HOP_FV(0x002aebb0)[v], HOP_FV(0x002aebc0)[v]);
	v = hop_Variation;
	bdSetFogColour((unsigned char)HOP_FTOL(HOP_FV(0x002aeb80)[v]), (unsigned char)HOP_FTOL(HOP_FV(0x002aeb90)[v]),
		(unsigned char)HOP_FTOL(HOP_FV(0x002aeba0)[v]));
	if (hop_Variation == 1) {
		if (hop_Play == 5 && hop_RoundOver == 0)
			bdSetNearFar(750.0f, 50000.0f);
		else
			bdSetNearFar(100.0f, 50000.0f);
	} else {
		bdSetNearFar(5.0f, 50000.0f);
	}
	HopCamera_Apply(HOP_ADDR(0x002aebd8));
	HopPlay_ProfileBegin(HOP_ADDR(0x002b0038), 2, "arena");
	HopArena_Draw(HOP_ADDR(0x002aec10), 0);
	HopPlay_ProfileEnd(HOP_ADDR(0x002b0038), 2);
	FUN_001d5950();    /* the rules. draw (empty) */
	HopPlay_ProfileBegin(HOP_ADDR(0x002b0038), 3, "antics");
	HopAntics_Draw(hop_Antics);
	HopPlay_ProfileEnd(HOP_ADDR(0x002b0038), 3);
	HopPlay_ProfileBegin(HOP_ADDR(0x002b0038), 4, "pretty");
	HopFx_Draw(HOP_ADDR(0x002affa0));
	CToken_DrawAll();
	if (hop_Variation == 2)
		HopArena_DrawExtras2(HOP_ADDR(0x002aec10));
	if (hop_Variation == 3)
		HopArena_DrawExtras3(HOP_ADDR(0x002aec10));
	if (hop_Variation == 4)
		HopArena_DrawExtras4(HOP_ADDR(0x002aec10));
	bdSetRenderState(4, 0, 0);
	HopPlay_DrawOverlay(HOP_ADDR(0x002b0038));
	HopPlay_ProfileEnd(HOP_ADDR(0x002b0038), 4);
	GAME_I(self, 0x28) = (int)FUN_0003f010();
	DAT_002bc48c = 0;
	HopPlay_ProfileEnd(HOP_ADDR(0x002b0038), 1);
	HopPlay_ProfileEnd(HOP_ADDR(0x002b0038), 0);
	HopPlay_DrawProfile(HOP_ADDR(0x002b0038));
}

/* 0x0013f2c0  (gap): restart (the debug menu's "restart"): puts everything back for a new
round */
void HopGame_Restart(void *self)
{
	void *token;

	(void)self;
	HopRes_SetRenderStates(hop_Res);
	Debris_Init(NULL, 1000);
	HopAntics_Reset(hop_Antics);
	HopPlay_Start(HOP_ADDR(0x002b0038));
	HopCamera_Start(HOP_ADDR(0x002aebd8));
	HopArena_Start(HOP_ADDR(0x002aec10));
	HopRules_Start(HOP_ADDR(0x002afe80));
	for (token = CToken_Next(NULL); token != NULL; token = CToken_Next(NULL))
		CToken_Delete((struct CToken *)token);
	HopFx_Shutdown(HOP_ADDR(0x002affa0));
	HopFx_Start(HOP_ADDR(0x002affa0));
	HOP_B(0x003027b0) = 1;
	hop_RoundOver = 0;
	CLHud_ResetCountdown(DAT_002ecfac);
	HOP_I(0x002b011c) = 0;
	HOP_I(0x002b0120) = 0;
}

/* 0x0013f380  (gap): scalar deleting destructor of the game object */
void *HopGame_Destroy(void *self, byte flags)
{
	HopGame_Dtor(self);
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026e448  the game object's vtable */
const void *const vtable_0026e448[1] = {
	(const void *)FUN_0013f380, /* [0] scalar deleting destructor */
};

/* 0x0013f3a0  Hopper/CHopper_Game.cpp: init (the packages are loaded): the fog of the
variation, the tokens' assets, the resources, debris and the RefBot, and with the debug
free camera on (0x002ba56c) a free camera with the 5 points at antics +0x80 */
void HopGame_Init(void *self)
{
	float *r = HOP_FV(0x002aeb80);    /* fog red, green, blue, start, end: [variation] */
	float *g = HOP_FV(0x002aeb90);
	float *b = HOP_FV(0x002aeba0);
	float *start = HOP_FV(0x002aebb0);
	float *end = HOP_FV(0x002aebc0);
	int v = hop_Variation;
	void *camera;
	char *point;

	GAME_I(self, 0x20) = 0;
	switch (v) {
	case 1:
		r[v] = 67.0f;
		g[v] = 60.0f;
		b[v] = 54.0f;
		start[v] = 1000.0f;
		end[v] = 65000.0f;
		break;
	case 2:
		r[v] = 0.0f;
		g[v] = 125.0f;
		b[v] = 135.0f;
		start[v] = 4300.0f;
		end[v] = 7400.0f;
		break;
	case 3:
		r[v] = 0.0f;
		g[v] = 130.0f;
		b[v] = 165.0f;
		start[v] = 3000.0f;
		end[v] = 12000.0f;
		break;
	case 4:
		r[v] = 13.0f;
		g[v] = 0.0f;
		b[v] = 48.0f;
		start[v] = 4200.0f;
		end[v] = 15300.0f;
		break;
	}
	CToken_LoadAssets(1, NULL);
	HopRes_Load(hop_Res);
	bdSaveRenderStates((unsigned int *)HOP_ADDR(0x002aea48));
	Debris_Init(NULL, 1000);
	RefBot_Create();
	bdRestoreRenderStates((unsigned int *)HOP_ADDR(0x002aea48));
	if (DAT_002ba56c != 0) {
		camera = HOP_NEW(0x26c, "C:\\BlitzParty\\Game\\Src\\Hopper\\CHopper_Game.cpp", 0xdc);
		if (camera != NULL)
			BCarFreeCamera_Construct((struct BCarFreeCamera *)camera, 0, 20.0f, 1);
		for (point = HOP_A(0x002ae8f8); point < HOP_A(0x002ae948); point += 0x10)
			BCarFreeCamera_AddPoint((struct BCarFreeCamera *)DAT_002f26c4, (float *)point);
	}
	HOP_I(0x002b036c) = 0x100;
}
