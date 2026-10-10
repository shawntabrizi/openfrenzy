/*
Drillers/CDrillers_Game.c  <-  C:\BlitzParty\Game\Src\Drillers\CDrillers_Game.cpp

The Drillers game object (CDrillers_Game, one static object at 0x002ff9f0, vtable
0x0026bfdc) and the game mode that CModule_Drillers pushes (CGameMode_Drillers, vtable
0x0026c0d8). Every game-mode slot is a thunk that runs the game object's method.
0x00110960 is tagged with this file; the functions from 0x0010f9c0 to 0x00111580 (gap)
are inferred to be in it.

The game object, fields at original offsets (0x002ff9f0 + offset):
  +0x08 the CGameMode_Drillers game mode (0x002ff9f8)
  +0x0c variant 1-4 (0x002ff9fc)
  +0x10 game state (0x002ffa00): 1 intro camera, 3, 5 playing, 6 game over
  +0x14 state frame counter
  +0x18 (byte) a core flag (0x002bae8c)
  +0x1c, +0x20 frame counters
  +0x24 (byte) 1 after Initialise
  +0x28 timer value at the start (64-bit), +0x30 copy, +0x38 time left (64-bit)
  +0x40 the four scores (0x002ffa30)
  +0x60, +0x64
  +0x68 round (variants 1 and 3: rounds 1-3)
  +0x70 timer value (64-bit)
  +0x80 players alive at the last check
  +0x84 (byte) paused/skip flag
  +0x88
  +0x90 (passed to classlib_ui 0x00072720)
  +0x9c.. +0xa0[3] per-round values (0, 90, 180)
  +0xac colour set, +0xb0/+0xb4 single human player
  +0xb8 game-over countdown
  +0xbc, +0xc0 saved sound fall-off distances
*/

#include "drillers.h"

/* game state values (+0x10) */
#define DR_STATE_INTRO 1
#define DR_STATE_PLAY 5
#define DR_STATE_OVER 6

#define GAME_I32(off) DR_I32(self, off)

/* the map object (0x00298da0), the players object (0x00298e70), the other static objects */
#define DR_MAP DR_PTR(void, 0x00298da0u)
#define DR_PLAYERS DR_PTR(void, 0x00298e70u)
#define DR_RES DR_PTR(void, 0x002aa900u)
#define DR_CAMERA DR_PTR(void, 0x002aa830u)
#define DR_PADS DR_PTR(void, 0x002aaaf8u)
#define DR_HUD2 DR_PTR(void, 0x002aab08u)
#define DR_AI DR_PTR(void, 0x002aadf8u)
#define DR_FX DR_PTR(void, DR_FX_ADDR)
/* the four per-player emitter groups of the effects object (0x10c bytes each) */
#define DR_FXP(i) DR_PTR(void, DR_FX_ADDR + 0x900u + (i) * 0x10cu)
/* the saved render states */
#define DR_RSTATES DR_PTR(unsigned int, 0x002aaa70u)

/* the sound samples and channels (resources object fields) */
#define DR_SAMPLE(addr) DR_VAR(struct BSample *, addr)
#define DR_CHANNEL(addr) DR_VAR(int, addr)

/* classlib_ui 0x00071f90 returns the package (the table says void) */
#define dr_AddPackage ((int (*)(const char *, int))FUN_00071f90)

extern void *DAT_002ba9a0;            /* the settings (core) */
extern int DAT_002ba56c;              /* bumpercars_b switch: the demo camera exists */
extern void *DAT_002f26c4;            /* bumpercars_b object (the demo camera?) */
extern float DAT_00285644;            /* collision sweep epsilon (engine_misc) */
extern int DAT_00283ea4;              /* character colour set */
extern int DAT_00283ef0;
extern unsigned char DAT_002ecd90[];  /* the pause menu (classlib_ui) */
extern int DAT_002baf1c, DAT_002baf44, DAT_002baf6c;  /* team of players 1-3 */
extern int DAT_002baf10, DAT_002baf38, DAT_002baf60;  /* minigame score of players 1-3 */

/* 0x0026bfdc  vtable of CDrillers_Game (rebuilt from the XBE) */
const void *const vtable_0026bfdc[1] = {
	(const void *)CDrillersGame_DeletingDtor, /* +0x00 */
};

/* 0x0026c0d8  vtable of CGameMode_Drillers (rebuilt from the XBE) */
const void *const vtable_0026c0d8[13] = {
	(const void *)CGameModeDrillers_DeletingDtor,  /* +0x00 */
	(const void *)CGameModeDrillers_Suspend,       /* +0x04 */
	(const void *)CGameModeDrillers_Suspend,       /* +0x08 resume: the same thunk */
	(const void *)CGameModeDrillers_Popped,        /* +0x0c */
	(const void *)CGameModeDrillers_Pushed,        /* +0x10 */
	(const void *)CGameModeDrillers_Input,         /* +0x14 */
	(const void *)CGameModeDrillers_Update,        /* +0x18 */
	(const void *)CGameModeDrillers_Render,        /* +0x1c */
	(const void *)FUN_001ec260,                    /* +0x20 */
	(const void *)CGameMode_UpdateLoading,         /* +0x24 */
	(const void *)CGameMode_FinishedLoading,       /* +0x28 */
	(const void *)CGameModeDrillers_LoadingStarted, /* +0x2c */
	(const void *)CGameModeDrillers_Start,         /* +0x30 */
};

/* 0x0010f9c0  Drillers (gap; CDrillers_Game.cpp inferred)
The constructor of the static game object (core's static constructor #19). */
void *CDrillersGame_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026bfdc;
	CLList_Init((int *)DR_P(self, 0x90)); /* lea ecx,[esi+0x90] */
	return self;
}

/* 0x0010f9e0  Drillers (gap; CDrillers_Game.cpp inferred) */
void CDrillersGame_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026bfdc;
	FUN_001d5950();
}

/* 0x0010fa00  Drillers (gap; CDrillers_Game.cpp inferred)
Resets a round: the scores, the state, the timers, the round number (variants 1 and 3
count rounds 1-3), the players' scores, the extras and the emitter groups. */
void CDrillersGame_Reset(void *self)
{
	unsigned long long now;
	int i;

	GAME_I32(0xb8) = 0;
	GAME_I32(0x60) = 0;
	GAME_I32(0x64) = 3;
	GAME_I32(0x40) = 1;
	GAME_I32(0x44) = 1;
	GAME_I32(0x48) = 1;
	GAME_I32(0x4c) = 1;
	if (GAME_I32(0x68) == 0) {
		GAME_I32(0x10) = DR_STATE_INTRO;
	} else if (GAME_I32(0xc) == 1) {
		DR_VAR(float, 0x002aa878) = 1600.0f;
		GAME_I32(0x10) = 7;
	} else if (GAME_I32(0x68) != 0 && GAME_I32(0xc) == 3) {
		CLHud_StartBars((float *)(uintptr_t)DAT_002ecfac, 280.0f);
		GAME_I32(0x10) = 3;
	} else {
		GAME_I32(0x10) = DR_STATE_INTRO;
	}
	GAME_I32(0x14) = 0;
	now = bkTimerRead();
	DR_AT(self, 0x28, unsigned long long) = now;
	GAME_I32(0x30) = (int)now;
	GAME_I32(0x74) = (int)(now >> 32);
	GAME_I32(0x34) = (int)(now >> 32);
	GAME_I32(0x70) = (int)now;
	GAME_I32(0x3c) = 0;
	if (GAME_I32(0xc) == 2)
		GAME_I32(0x38) = 9000;
	else
		GAME_I32(0x38) = 6000;
	CLHud_StopCountdown(DAT_002ecfac);
	i = GAME_I32(0xc);
	if (i == 1 || i == 3)
		GAME_I32(0x68) = (GAME_I32(0x68) + 1) & 3;
	if (i == 1 || i == 3)
		DR_U32(DAT_002ecfac, 0x8c) = (unsigned int)(2 < GAME_I32(0x68));
	if (GAME_I32(0xc) == 1) {
		DR_VAR(float, 0x002a55e4) = DR_VAR(float, 0x002a55e0);
		if (1 < GAME_I32(0x68))
			DR_VAR(float, 0x002a55e0) = (float)GAME_I32(0x9c + GAME_I32(0x68) * 4);
		else
			DR_VAR(float, 0x002a55e0) = 0.0f;
	} else {
		DR_VAR(float, 0x002a55e0) = 0.0f;
	}
	if (GAME_I32(0x68) == 0)
		GAME_I32(0x68) = 1;
	for (i = 0; i < 4; i++) {
		DR_PL(int, i, 0x00299570u) = 0;
		DR_PL(int, i, 0x00298ed0u) = 0;
	}
	DR_VAR(int, 0x00298e4c) = 0;
	DR_VAR(int, 0x00298e50) = 0;
	DR_VAR(int, 0x00298e54) = 0;
	DR_VAR(int, 0x00298e58) = 0;
	if (1 < GAME_I32(0x68)) {
		if (DR_VAR(struct BExtraSystem *, 0x003014b0) != NULL)
			baClearExtraSystem(DR_VAR(struct BExtraSystem *, 0x003014b0));
		if (DR_VAR(struct BExtraSystem *, 0x003014b4) != NULL)
			baClearExtraSystem(DR_VAR(struct BExtraSystem *, 0x003014b4));
		if (DR_VAR(struct BExtraSystem *, 0x0030149c) != NULL)
			baClearExtraSystem(DR_VAR(struct BExtraSystem *, 0x0030149c));
		if (DR_VAR(struct BExtraSystem *, 0x003014a0) != NULL)
			baClearExtraSystem(DR_VAR(struct BExtraSystem *, 0x003014a0));
		if (DR_VAR(struct BExtraSystem *, 0x003014b8) != NULL)
			baClearExtraSystem(DR_VAR(struct BExtraSystem *, 0x003014b8));
		if (DR_VAR(struct BExtraSystem *, 0x003014bc) != NULL)
			baClearExtraSystem(DR_VAR(struct BExtraSystem *, 0x003014bc));
	}
	FUN_001118f0(DR_FXP(0));
	FUN_001118f0(DR_FXP(1));
	FUN_001118f0(DR_FXP(2));
	FUN_001118f0(DR_FXP(3));
	DR_VAR(int, 0x002aa888) = 0;
	DR_VAR(int, 0x002aa88c) = 0;
	DR_VAR(int, 0x002aa89c) = 0;
}

/* a free sound channel: the first channel whose status is 0, else `current` */
static int dr_free_channel(int current)
{
	int channel;

	for (channel = 0; channel < bsGetNumChannels(); channel++) {
		if (bsGetChannelStatus(channel) == 0)
			return channel;
	}
	return current;
}

/* 0x0010fc00  Drillers (gap; CDrillers_Game.cpp inferred)
Start (game-mode slot +0x30): the mode becomes opaque; the HUD score boxes, texts, clock
(60 s for Erodeo and Quantum Snare, none otherwise) and bars; the start sound and, for
variants 1 and 2, the looping engine sounds on reserved channels. */
void CDrillersGame_Start(void *self)
{
	FUN_0011e140(DR_RES);
	DR_U32(DR_PTRF(self, 8), 0x10) |= 1;
	FUN_00072720((undefined4)(uintptr_t)DR_P(self, 0x90));
	CLHud_CreateScoreBoxes(DAT_002ecfac);
	CLHud_CreateScoreTexts(DAT_002ecfac, DR_PTR(int, 0x002ffa30u));
	if (GAME_I32(0xc) == 2 || GAME_I32(0xc) == 4) {
		CLHud_CreateClock(DAT_002ecfac, 60.0f);
		CLHud_SetClock(DAT_002ecfac, 60.0f, 1);
	} else {
		CLHud_CreateClock(DAT_002ecfac, -1.0f);
		CLHud_SetClock(DAT_002ecfac, -1.0f, 1);
	}
	CLHud_StopCountdown(DAT_002ecfac);
	bdSaveRenderStates(DR_RSTATES);
	FUN_001d5950();
	bdRestoreRenderStates(DR_RSTATES);
	CLHud_CreateBars(DAT_002ecfac);
	if (GAME_I32(0xc) < 3)
		FUN_00112c20(DR_FX);
	bsPlaySample(DR_SAMPLE(0x002aa9d4), 100, 0, 0, 0);
	if (g_DrVariant < 3) {
		DR_CHANNEL(0x002aaa44) = dr_free_channel(DR_CHANNEL(0x002aaa44));
		bsPlaySampleOnChannel(DR_CHANNEL(0x002aaa44), DR_SAMPLE(0x002aa9f4), 0xff, 0, 0, 0x20);
		bsReserveChannel(DR_CHANNEL(0x002aaa44), 1);
		if (g_DrVariant == 1) {
			DR_CHANNEL(0x002aaa40) = dr_free_channel(DR_CHANNEL(0x002aaa40));
			bsPlaySampleOnChannel(DR_CHANNEL(0x002aaa40), DR_SAMPLE(0x002aa9f0), 0xff, 0, 0,
				0x20);
			bsReserveChannel(DR_CHANNEL(0x002aaa40), 1);
		}
	}
}

/* 0x0010fdc0  Drillers (gap; CDrillers_Game.cpp inferred)
Starts a round: builds the map (variants 1 and 2), resets the round, the effects and the
four players. */
void CDrillersGame_NewRound(void *self)
{
	if (g_DrVariant < 3)
		FUN_00114990(DR_MAP);
	CDrillersGame_Reset(self);
	FUN_001139a0(DR_FX);
	FUN_00116970(DR_PLAYERS, 0);
	FUN_00116970(DR_PLAYERS, 1);
	FUN_00116970(DR_PLAYERS, 2);
	FUN_00116970(DR_PLAYERS, 3);
	GAME_I32(0x80) = 4;
	GAME_I32(0x88) = (int)0xc4000000;
	DR_VAR(int, 0x002ffa7c) = (GAME_I32(0xc) == 4) - 1 & 0x5b;
}

/* 0x0010fe50  Drillers (gap; CDrillers_Game.cpp inferred)
Pushed: the variant (the "variation" setting overrides it), the variant package
"packages\drill-v<n>", and the sound fall-off distances (saved, then 4000 / 10000). */
void CDrillersGame_Load(void *self, int variant)
{
	char name[64];
	char *value;

	value = Settings_Get((struct Settings *)DAT_002ba9a0, "variation");
	if (value != NULL)
		variant = atol(value);
	GAME_I32(0xc) = variant;
	sprintf(name, "packages\\drill-v%d", g_DrVariant);
	DR_VAR(int, 0x002aa908) = dr_AddPackage(name, 0);
	if (DR_VAR(int, 0x002aa908) == 0)
		bkPrintf("ERROR driller package failed to load\n");
	DR_F32(self, 0xc0) = DAT_0028ffa8;
	DR_F32(self, 0xbc) = DAT_0028ffac;
	DAT_0028ffa8 = 4000.0f;
	DAT_0028ffac = 10000.0f;
}

/* 0x0010fef0  Drillers (gap; CDrillers_Game.cpp inferred)
Popped: restores the sound distances, clears the textures and frees the game's objects. */
void CDrillersGame_Unload(void *self)
{
	FUN_0019d810();
	DAT_0028ffa8 = DR_F32(self, 0xc0);
	DAT_0028ffac = DR_F32(self, 0xbc);
	bdSetTexture(0, NULL);
	bdSetTexture(1, NULL);
	FUN_00120260(DR_PADS);
	if (g_DrVariant < 3)
		FUN_00115710(DR_MAP);
	FUN_0011f770(DR_RES);
	if (GAME_I32(0xc) < 3)
		FUN_00112c90(DR_FX);
	COverlay_DeleteAll();
}

/* 0x0010ff70  Drillers (not in of_functions.tsv; body of the game-mode slot +0x14)
Input: while the mode is not on top it only updates the loading screen. */
void FUN_0010ff70(void *self, int top)
{
	void *mode;

	if (top == 0) {
		mode = DR_PTRF(self, 8);
		if (DR_I32(mode, 0x14) != 0)
			DR_VFN(mode, 0x24, void (*)(void *))(mode);
		return;
	}
	FUN_001202c0(DR_PADS);
}

/* the volume of the engine loop: 0xff minus the distance of the camera key time from
   0x2fd or 0xb13 (whichever half of the camera path), divided by 3; at most 0xe6 */
static int dr_engine_volume(int actor)
{
	int t;
	int base;
	int v;
	void *node = DR_PTRF(actor, 0x134);

	t = DR_I32(DR_I32(node, 0xc), DR_U8(node, 0x78) * 0x40 + 0x18);
	t = (t + ((t >> 31) & 0x3ff)) >> 10;
	base = 0x2fd;
	if (0x707 < t)
		base = 0xb13;
	v = abs(abs(base - t)) / 3;
	if (v < 0x100) {
		if (v < 0)
			v = 0;
	} else {
		v = 0xff;
	}
	v = 0xff - v;
	if (v < 0xe7) {
		if (v < 0)
			v = 0;
	} else {
		v = 0xe6;
	}
	return v;
}

/* 0x0010ffa0  Drillers (not in of_functions.tsv; 0x00110000 is its second chunk)
Update of Rubble Alliance: the volumes of the two engine loops follow the animation of
two actors; then, while playing, drops the cells that no longer hold on (map 0x001154b0). */
void FUN_0010ffa0(void *self)
{
	int t;
	int v;
	void *node;

	bsSetChannelVolume(DR_CHANNEL(0x002aaa44), dr_engine_volume(DR_VAR(int, 0x002a748c)));
	node = DR_PTRF(DR_VAR(int, 0x002a74c4), 0x134);
	t = DR_I32(DR_I32(node, 0xc), DR_U8(node, 0x78) * 0x40 + 0x18);
	v = abs(((t + ((t >> 31) & 0x3ff)) >> 10) - 0xe10) / 0xe;
	if (v < 0x100) {
		if (v < 0)
			v = 0;
	} else {
		v = 0xff;
	}
	v = 0xe6 - v;
	if (v < 0xe7) {
		if (v < 0)
			v = 0;
		else if (0 < v)
			v = v / 2;
	} else {
		v = 0xe6 / 2;
	}
	bsSetChannelVolume(DR_CHANNEL(0x002aaa40), v);
	if (GAME_I32(0x10) != DR_STATE_PLAY)
		return;
	FUN_001154b0(DR_MAP);
}

/* 0x001100e0  Drillers (not in of_functions.tsv)
Update of Erodeo: the engine loop volume, the effects, then, while playing, the
effects of the play and the loose cells. */
void FUN_001100e0(void *self)
{
	bsSetChannelVolume(DR_CHANNEL(0x002aaa44), dr_engine_volume(DR_VAR(int, 0x002a74c4)));
	FUN_001145d0(DR_FX);
	FUN_00112a40(DR_FX);
	if (GAME_I32(0x10) != DR_STATE_PLAY)
		return;
	FUN_00114230(DR_FX);
	FUN_001154b0(DR_MAP);
}

/* 0x001101b0  Drillers (gap; CDrillers_Game.cpp inferred)
Follows the character colour set; while playing with the "single human" option
(0x00283ef0) finds out whether exactly one player is human (+0xb0, the player in +0xb4). */
void CDrillersGame_CheckColours(void *self)
{
	int i;
	int humans;
	int last;

	if (DAT_00283ea4 != GAME_I32(0xac)) {
		GAME_I32(0xac) = DAT_00283ea4;
		for (i = 0; i < DAT_002bae94; i++)
			Core_SetCharacterColours(DR_PL(void *, i, 0x00298ec0u), DAT_00283ea4);
	}
	if (DAT_00283ef0 != 0 && GAME_I32(0x10) == DR_STATE_PLAY) {
		humans = 0;
		last = -1;
		for (i = 0; i < DAT_002bae94; i++) {
			if (DAT_002baed8[i * 10] != 0) {
				humans++;
				last = i;
			}
		}
		if (0 < DAT_002bae94 && humans == 1) {
			GAME_I32(0xb4) = last;
			GAME_I32(0xb0) = 1;
			return;
		}
	}
	GAME_I32(0xb0) = 0;
}

/* 0x00110270  Drillers (not in of_functions.tsv; body of the game-mode slot +0x1c;
0x00110840 is its last chunk)
Render: the idle/win/still animations of the players, the fog of the variant, the
camera (intro camera, or the game camera with the variant's height), the map, the
players, the effects, the HUD and the overlays. */
void FUN_00110270(void *self, int top)
{
	int i;
	int actor;
	const char *anim;
	struct BActorSubObject *seg;
	int v4;
	float t;
	float eye[3];
	float target[3];
	float roll;
	int score0;
	int score1;
	int count;
	int last;
	int best;
	int winner;

	if (top == 0)
		return;
	for (i = 0; i < 4; i++) {
		if (g_DrState == DR_STATE_OVER) {
			DR_PL(int, i, 0x00298eb0u) = DR_PL(int, i, 0x00298eb4u);
			actor = DR_PL(int, i, 0x00298ec4u);
			anim = "winloop";
		} else if (g_DrState < DR_STATE_PLAY) {
			DR_PL(int, i, 0x00298eb0u) = DR_PL(int, i, 0x00298eb4u);
			actor = DR_PL(int, i, 0x00298ec4u);
			anim = "idle";
		} else {
			DR_PL(int, i, 0x00298eb0u) = DR_PL(int, i, 0x00298eb8u);
			actor = DR_PL(int, i, 0x00298ec8u);
			anim = "still";
		}
		DR_PL(int, i, 0x00298ec0u) = actor;
		seg = baFindSubObject((struct BActorInstance *)(uintptr_t)actor, anim);
		if (seg == NULL)
			bkPrintf("IDLE SEG NOT FOUND\n");
		else
			baQueueAnimation((struct BActorInstance *)(uintptr_t)actor,
				DR_AT(actor, 0x134, struct BActorNode *), (struct BAnim *)seg, 0x400, -1, 1,
				NULL, NULL);
	}
	v4 = GAME_I32(0xc) * 4;
	bdSetFogRange(DR_VAR(float, 0x00301c58u + v4), DR_VAR(float, 0x00301c68u + v4));
	bdSetFogColour((unsigned char)DR_FTOL(DR_VAR(float, 0x00301c28u + v4)),
		(unsigned char)DR_FTOL(DR_VAR(float, 0x00301c38u + v4)),
		(unsigned char)DR_FTOL(DR_VAR(float, 0x00301c48u + v4)));
	bdSetRenderState(8, 1, 0);
	if (FUN_0011c7e0(DR_CAMERA) != 0)
		return;
	if (DAT_002f26c4 == NULL || g_DrState != DR_STATE_PLAY) {
		if (g_DrState == DR_STATE_INTRO) {
			t = CLIntroCam_GetTime();
			CLIntroCam_GetPosition(eye, t);
			CLIntroCam_GetTarget(target, t);
			CLIntroCam_GetRoll(&roll, t);
			eye[0] -= DR_VAR(float, 0x002a55e8);
			eye[1] -= DR_VAR(float, 0x002a55ec);
			eye[2] -= DR_VAR(float, 0x002a55f0);
			DR_VAR(float, 0x002aa834) = eye[0];
			DR_VAR(float, 0x002aa838) = eye[1];
			DR_VAR(float, 0x002aa83c) = eye[2];
			target[0] -= DR_VAR(float, 0x002a55e8);
			target[1] -= DR_VAR(float, 0x002a55ec);
			target[2] -= DR_VAR(float, 0x002a55f0);
			DR_VAR(float, 0x002aa840) = target[0];
			DR_VAR(float, 0x002aa844) = target[1];
			DR_VAR(float, 0x002aa848) = target[2];
			bdSetNearFar(1.0f, 140000.0f);
			bdSetCameraLookAt(eye, target, roll);
			bdUpdateView();
		} else {
			if (GAME_I32(0xc) == 1) {
				DR_VAR(float, 0x002aa838) -= DR_VAR(float, 0x002aa890);
				DR_VAR(float, 0x002aa844) -= DR_VAR(float, 0x002aa894);
			}
			DR_VAR(float, 0x002aa838) = DR_VAR(float, 0x002aa878) + DR_VAR(float, 0x002aa838);
			DR_VAR(float, 0x002aa844) = DR_VAR(float, 0x002aa878) + DR_VAR(float, 0x002aa844);
			bdSetCameraLookAt(DR_PTR(float, 0x002aa834u), DR_PTR(float, 0x002aa840u), 0.0f);
			bdUpdateView();
			DR_VAR(float, 0x002aa838) -= DR_VAR(float, 0x002aa878);
			DR_VAR(float, 0x002aa844) -= DR_VAR(float, 0x002aa878);
			if (GAME_I32(0xc) == 1) {
				DR_VAR(float, 0x002aa838) = DR_VAR(float, 0x002aa890) + DR_VAR(float, 0x002aa838);
				DR_VAR(float, 0x002aa844) = DR_VAR(float, 0x002aa894) + DR_VAR(float, 0x002aa844);
				DR_VAR(float, 0x002aa890) = (400.0f - DR_VAR(float, 0x002aa890)) * 0.0625f +
					DR_VAR(float, 0x002aa890);
				DR_VAR(float, 0x002aa894) = (400.0f - DR_VAR(float, 0x002aa894)) * 0.0625f +
					DR_VAR(float, 0x002aa894);
			}
		}
	}
	if (GAME_I32(0xc) < 3)
		FUN_001160f0(DR_MAP);
	FUN_001164d0(DR_MAP);
	if (GAME_I32(0xc) < 3)
		FUN_001151f0(DR_MAP);
	if (DAT_002ecdec == 0)
		DR_VAR(int, 0x002a54c8) += 2;
	if (0xd2 < DR_VAR(int, 0x002a54c8))
		DR_VAR(int, 0x002a54c8) = 0x5a;
	for (i = 0; i < 4; i++) {
		if ((DR_PL(signed char, i, 0x00298e90u) == 0 || 0 < DR_PL(int, i, 0x00299564u)) &&
				2 < GAME_I32(0xc) && DR_PL(float, i, 0x00299568u) != 0.0f) {
			FUN_001195c0(DR_PLAYERS, i);
			DR_PL(float, i, 0x00299568u) = DR_PL(float, i, 0x00299568u) - 16.0f;
			if (DR_PL(float, i, 0x00299568u) < 0.0f)
				DR_PL(float, i, 0x00299568u) = 0.0f;
		} else if (GAME_I32(0x10) == DR_STATE_OVER &&
				((GAME_I32(0x68) == 3 && 0x37 < DR_VAR(int, 0x002aa89c)) ||
				GAME_I32(0xc) == 2 || GAME_I32(0xc) == 4)) {
			FUN_00118f00(DR_PLAYERS, i);
		} else if (DR_PL(signed char, i, 0x00298e90u) < 2) {
			FUN_001195c0(DR_PLAYERS, i);
		} else {
			FUN_00117200(DR_PLAYERS, i);
		}
	}
	if (GAME_I32(0xc) == 2 || GAME_I32(0xc) == 4) {
		FUN_001d5950();
		FUN_00113c60(DR_FX);
	}
	if (g_DrVariant == 2 && g_DrState == DR_STATE_PLAY) {
		if (DAT_002baebc == 0) {
			best = 0;
			count = 0;
			last = 4;
			for (i = 0; i < 4; i++) {
				if (DR_PL(char, i, 0x00298e90u) == 1) {
					count++;
					last = i;
				}
				if (best < DR_PL(int, i, 0x00298ed0u))
					best = DR_PL(int, i, 0x00298ed0u);
			}
			if (count == 1 && best != DR_PL(int, last, 0x00298ed0u))
				FUN_00120870(DR_HUD2, last, best - DR_PL(int, last, 0x00298ed0u));
		} else {
			score0 = 0;
			score1 = 0;
			if (DAT_002baef4[0] == 0)
				score0 = DR_PL(int, 0, 0x00298ed0u);
			if (DAT_002baef4[0] == 1)
				score1 = DR_PL(int, 0, 0x00298ed0u);
			if (DAT_002baf1c == 0)
				score0 += DR_PL(int, 1, 0x00298ed0u);
			if (DAT_002baf1c == 1)
				score1 += DR_PL(int, 1, 0x00298ed0u);
			if (DAT_002baf44 == 0)
				score0 += DR_PL(int, 2, 0x00298ed0u);
			if (DAT_002baf44 == 1)
				score1 += DR_PL(int, 2, 0x00298ed0u);
			if (DAT_002baf6c == 0)
				score0 += DR_PL(int, 3, 0x00298ed0u);
			if (DAT_002baf6c == 1)
				score1 += DR_PL(int, 3, 0x00298ed0u);
			winner = score0 <= score1;
			count = 0;
			last = winner;
			for (i = 0; i < 4; i++) {
				if (DR_PL(char, i, 0x00298e90u) == 1) {
					count++;
					last = i;
				}
			}
			if (count == 1 && winner != DAT_002baef4[last * 10]) {
				if (winner == 0)
					FUN_00120870(DR_HUD2, last, score1 - score0);
				else
					FUN_00120870(DR_HUD2, last, score0 - score1);
			}
		}
	}
	FUN_00117460(DR_PLAYERS);
	bdSetWorldIdentity();
	bdSaveRenderStates(DR_RSTATES);
	baDrawExtras(NULL, NULL);
	CToken_DrawAll();
	FUN_00120a70(DR_HUD2);
	FUN_001118d0(DR_FXP(0));
	FUN_001118d0(DR_FXP(1));
	FUN_001118d0(DR_FXP(2));
	FUN_001118d0(DR_FXP(3));
	FUN_001117a0(DR_FX);
	if (GAME_I32(0x10) == 3)
		FUN_001d5950();
	if (DAT_002ba56c == 0)
		COverlay_DrawAll(1);
	if (DR_VAR(int, 0x00301498) != 0)
		FUN_00112a70(DR_FX);
	if (g_DrState == DR_STATE_OVER && DR_VAR(int, 0x002aa88c) != 0) {
		DR_VAR(int, 0x002aa88c) += 0x20;
		if (0x108 < DR_VAR(int, 0x002aa88c))
			DR_VAR(int, 0x002aa88c) = 0x109;
		CLScreen_DrawDarken(DR_VAR(int, 0x002aa88c), 1);
	}
	if (GAME_I32(0x10) == DR_STATE_OVER) {
		if ((g_DrVariant == 1 || g_DrVariant == 3) && DR_U8(self, 0x84) == 0)
			FUN_00120d00(DR_HUD2);
	} else {
		DR_VAR(int, 0x002aaba4) = 0;
	}
	bdRestoreRenderStates(DR_RSTATES);
}

/* 0x00110940  Drillers (gap; CDrillers_Game.cpp inferred) */
void *CDrillersGame_DeletingDtor(void *self, unsigned int flags)
{
	CDrillersGame_Dtor(self);
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x00110960  Drillers/CDrillers_Game.cpp
Initialise (game-mode slot +0x2c, LoadingStarted): the pads, the AI, the map size of the
variant (20 x 20 cells of 80 units, Erodeo 10 x 10 cells of 160), the effects, the
resources, the emitters, the humans, the camera range, the light, the per-round values,
the first round, the debris, the projectors and the referee robot; with the demo camera
(0x002ba56c), the camera object of bumpercars_b and its five points. */
void CDrillersGame_Initialise(void *self)
{
	void *camera;
	int i;

	FUN_0011fd50(DR_PADS);
	FUN_00121a80(DR_AI);
	if (GAME_I32(0xc) == 2) {
		DR_VAR(int, 0x002ff9e4) = 10;
		DR_VAR(int, 0x002ff9e0) = 0xa0;
	} else {
		DR_VAR(int, 0x002ff9e4) = 0x14;
		DR_VAR(int, 0x002ff9e0) = 0x50;
	}
	DR_VAR(int, 0x002ff9e8) = DR_VAR(int, 0x002ff9e4);
	FUN_00112940(DR_FX);
	FUN_0010c190(1, 0);
	FUN_0011e1e0(DR_RES);
	FUN_00111830(DR_FXP(0));
	FUN_00111830(DR_FXP(1));
	FUN_00111830(DR_FXP(2));
	FUN_00111830(DR_FXP(3));
	FUN_00111720(DR_FX);
	DR_U8(self, 0x18) = (unsigned char)DAT_002bae8c;
	DR_PL(unsigned char, 0, 0x00299574u) = (unsigned char)DAT_002baed8[0];
	DR_PL(unsigned char, 1, 0x00299574u) = (unsigned char)DAT_002baf00;
	DR_PL(unsigned char, 2, 0x00299574u) = (unsigned char)DAT_002baf28;
	DR_PL(unsigned char, 3, 0x00299574u) = (unsigned char)DAT_002baf50;
	DR_U8(self, 0x24) = 1;
	GAME_I32(0x68) = 0;
	bdSetNearFar(300.0f, 75000.0f);
	DAT_00285644 = 0.01f;
	baSetAmbientLight(0x40, 0x40, 0x40);
	DR_U8(self, 0x84) = 0;
	DR_VAR(float, 0x002aa890) = 0.0f;
	DR_VAR(float, 0x002aa894) = 0.0f;
	for (i = 0; i < 3; i++) {
		if (i == 0)
			GAME_I32(0xa0) = 0;
		else
			GAME_I32(0xa0 + i * 4) = i * 0x5a;
	}
	for (i = 0; i < 22; i++)
		DR_VAR(unsigned char, 0x00300e81u + i * 0x30) = 0;
	CDrillersGame_NewRound(self);
	DR_VAR(int, 0x003014d0) = 0;
	FUN_0019d5c0();
	Debris_Init(NULL, 1000);
	CProjector_Reset();
	bdSaveRenderStates(DR_RSTATES);
	RefBot_Create();
	bdRestoreRenderStates(DR_RSTATES);
	if (DAT_002ba56c != 0) {
		camera = DR_NEW(0x26c, DR_FILE_GAME, 0x168, 0x1001);
		if (camera != NULL)
			FUN_000a5ca0((undefined4 *)camera, 0, dr_fbits(20.0f), 1);
		for (i = 0; i < 5; i++)
			FUN_000a5f60((int)(uintptr_t)DAT_002f26c4,
				DR_PTR(undefined4, 0x00298d50u + i * 0x10));
	}
	DR_VAR(int, 0x002aadf0) = 0x100;
}

/* 0x00110b90  Drillers (gap; CDrillers_Game.cpp inferred)
Update of Quantum Snare: the effects, the players' snares; while playing, one frame in
24 (by rand) a new effect. */
void CDrillersGame_UpdateSnare(void *self)
{
	FUN_001145d0(DR_FX);
	FUN_00112a40(DR_FX);
	FUN_00116cf0(DR_PLAYERS);
	if (GAME_I32(0x10) == DR_STATE_PLAY && dr_rand() % 0x18 == 0)
		FUN_00114230(DR_FX);
}

/* 0x00110bf0  Drillers (not in of_functions.tsv; body of the game-mode slot +0x18)
Update: the pause menu (it ends the game with the current scores), the HUD scores and
clock, the variant's update, the players' moves and AI, and the end of the game: every
player out, one player or team left (Erodeo), the quit button, or the time up. */
void FUN_00110bf0(void *self, int top)
{
	float colours[4][4];
	int i;
	int v;
	int alive;
	int last;
	short best;
	short lastScore;
	short sum0;
	short sum1;
	short count0;
	short count1;
	int over;
	int dead;
	float clock;

	if (top == 0)
		return;
	alive = 0;
	DR_VAR(unsigned char, 0x002ffa74) = DAT_002ecdec != 0;
	FUN_0019d870();
	CDrillersGame_CheckColours(self);
	for (i = 0; i < 4; i++) {
		int actor = DR_PL(int, i, 0x00298ebcu);

		colours[i][0] = DR_F32(actor, 0xc0);
		colours[i][1] = DR_F32(actor, 0xc4);
		colours[i][2] = DR_F32(actor, 0xc8);
		colours[i][3] = 1.0f;
	}
	DebrisColours_SetAll(colours[0], colours[1], colours[2], colours[3]);
	if (DAT_002ecdec != 0)
		DR_VAR(int, 0x002aa88c) = 0;
	if ((DAT_002ecdec != 0 || DR_VAR(int, 0x002aa88c) == 0) && DR_VAR(int, 0x002aadf0) < 1 &&
			CLPause_Update((undefined4 *)DAT_002ecd90) != 0) {
		DAT_002baee8[0] = DR_VAR(int, 0x002ffa30);
		DAT_002baf10 = DR_VAR(int, 0x002ffa34);
		DAT_002baf38 = DR_VAR(int, 0x002ffa38);
		DAT_002baf60 = DR_VAR(int, 0x002ffa3c);
		return;
	}
	v = GAME_I32(0xc);
	if (v == 1 || v == 3) {
		GAME_I32(0x40) = DR_VAR(int, 0x002aab2c);
		GAME_I32(0x44) = DR_VAR(int, 0x002aab3c);
		GAME_I32(0x48) = DR_VAR(int, 0x002aab4c);
		GAME_I32(0x4c) = DR_VAR(int, 0x002aab5c);
	} else {
		GAME_I32(0x40) = DR_PL(int, 0, 0x00298ed0u);
		GAME_I32(0x44) = DR_PL(int, 1, 0x00298ed0u);
		GAME_I32(0x48) = DR_PL(int, 2, 0x00298ed0u);
		GAME_I32(0x4c) = DR_PL(int, 3, 0x00298ed0u);
	}
	if (v == 2 || v == 4 || g_DrState <= DR_STATE_PLAY)
		CLHud_SetScores(DAT_002ecfac, DR_PTR(int, 0x002ffa30u));
	if (DAT_002ecdec == 0) {
		v = GAME_I32(0xc);
		if (v == 2 || v == 4) {
			if (g_DrState < DR_STATE_PLAY)
				clock = v == 2 ? 90.0f : 60.0f;
			else
				clock = (float)DR_AT(self, 0x38, long long) * 0.01f;
		} else {
			clock = -1.0f;
		}
		CLHud_SetClock(DAT_002ecfac, clock, 1);
	}
	if (DR_AT(self, 0x38, long long) < 1000)
		FUN_001d5950();
	if (g_DrVariant == 2 || g_DrVariant == 4) {
		best = 0;
		for (i = 0; i < 4; i++) {
			if ((int)best < GAME_I32(0x40 + i * 4))
				best = (short)GAME_I32(0x40 + i * 4);
		}
		CLHud_SetScoresInt(DAT_002ecfac, (int *)DR_P(self, 0x40));
		DR_I32(DAT_002ecfac, 0x3c) = 0;
		DR_F32(DAT_002ecfac, 0x40) = (float)(int)best;
		if (DR_U8(self, 0x84) == 0)
			CLHud_UpdateBars(DAT_002ecfac);
	}
	if (DAT_002ecdec == 0) {
		CProjector_UpdateAll();
		FUN_0010ce90();
	}
	COverlay_UpdateAll();
	FUN_0011e140(DR_RES);
	RefBot_Update();
	bdRestoreRenderStates(DR_RSTATES);
	if (DR_U8(self, 0x84) != 0)
		return;
	GAME_I32(0x20) = GAME_I32(0x20) + 1;
	GAME_I32(0x1c) = GAME_I32(0x1c) + 1;
	DR_VAR(int, 0x002a54c4) += 1;
	FUN_00111870(DR_FXP(0), 1.0f);
	FUN_00111870(DR_FXP(1), 1.0f);
	FUN_00111870(DR_FXP(2), 1.0f);
	FUN_00111870(DR_FXP(3), 1.0f);
	FUN_00111770(DR_FX);
	baUpdateExtras(NULL, (void *)(uintptr_t)dr_fbits(DAT_0028fda8 * (1.0f / 60.0f)));
	if (g_DrVariant == 4)
		DR_VAR(float, 0x003014ec) = ((DR_VAR(float, 0x002aa84c) + 180.0f) - 25.0f) *
			0.017453292f;
	else
		DR_VAR(float, 0x003014ec) = 3.1415927f;
	switch (GAME_I32(0xc)) {
	case 1:
		FUN_0010ffa0(self);
		break;
	case 2:
		FUN_001100e0(self);
		break;
	case 4:
		CDrillersGame_UpdateSnare(self);
		break;
	}
	if (GAME_I32(0xb8) != 0)
		GAME_I32(0xb8) = GAME_I32(0xb8) - 1;
	if (GAME_I32(0x10) != DR_STATE_PLAY)
		return;
	if (0 < DR_VAR(int, 0x002ffa7c))
		DR_VAR(int, 0x002ffa7c) -= 1;

	/* the players' moves; `alive` counts the players in play, `last` is the last one */
	last = 0;
	lastScore = 0;
	for (i = 0; i < 4; i++) {
		if (DR_PL(char, i, 0x00298e90u) != 1)
			continue;
		if (g_DrVariant < 3 && DR_PL(int, i, 0x00299594u) != 0)
			continue;
		alive++;
		last = (short)i;
		lastScore = DR_PL(short, i, 0x00298ed0u);
		FUN_00119cd0(DR_PLAYERS, i);
		if (DR_PL(char, i, 0x00299574u) != 0)
			continue;
		switch (GAME_I32(0xc)) {
		case 1:
			if (DR_PL(int, i, 0x00299594u) == 0)
				FUN_00121c80(DR_AI, i);
			break;
		case 2:
			if (DR_PL(int, i, 0x00299594u) == 0)
				FUN_00122530(DR_AI, i);
			break;
		case 3:
			FUN_00122c10(DR_AI, i);
			break;
		case 4:
			FUN_001230b0(DR_AI, i);
			break;
		}
	}

	/* is the game over? */
	over = 0;
	v = GAME_I32(0xc);
	if (v == 1 || v == 3) {
		if ((signed char)alive != GAME_I32(0x80)) {
			dead = 0;
			for (i = 0; i < 4; i++) {
				if (DR_PL(int, i, 0x00299594u) != 0)
					dead++;
			}
			bkPrintf("%d dead\n", dead);
			for (i = 0; i < 4; i++) {
				if (DR_PL(char, i, 0x00298e90u) != 1)
					continue;
				switch ((signed char)alive) {
				case 1:
					DR_PL(int, i, 0x00299570u) = 5;
					break;
				case 2:
					DR_PL(int, i, 0x00299570u) = 3;
					break;
				case 3:
					DR_PL(int, i, 0x00299570u) = 1;
					break;
				case 4:
					DR_PL(int, i, 0x00299570u) = 0;
					break;
				}
			}
			GAME_I32(0x80) = (signed char)alive;
		}
		if ((signed char)alive == 1 || (signed char)alive == 0)
			over = 1;
	} else if (v == 2 && ((signed char)alive == 1 ||
			((signed char)alive == 2 && DAT_002baebc != 0))) {
		/* one player left, or two of one team: the game ends if they lead */
		int ends = 1;

		if (DAT_002baebc == 0) {
			for (i = 0; i < 4; i++) {
				if ((int)lastScore <= DR_PL(int, i, 0x00298ed0u) && (short)i != (short)last)
					ends = 0;
			}
		} else {
			int team;

			if ((signed char)alive == 2) {
				count0 = 0;
				count1 = 0;
				for (i = 0; i < 4; i++) {
					if (DR_PL(char, i, 0x00298e90u) == 1) {
						if (DAT_002baef4[i * 10] == 0)
							count0++;
						if (DAT_002baef4[i * 10] == 1)
							count1++;
					}
				}
				if (count0 != 2 && count1 != 2)
					ends = 0;
			}
			sum0 = 0;
			sum1 = 0;
			for (i = 0; i < 4; i++) {
				if (DAT_002baef4[i * 10] == 0)
					sum0 += DR_PL(short, i, 0x00298ed0u);
				if (DAT_002baef4[i * 10] == 1)
					sum1 += DR_PL(short, i, 0x00298ed0u);
			}
			team = DAT_002baef4[(short)last * 10];
			if (team == 0 && sum0 <= sum1)
				ends = 0;
			if (team == 1 && sum1 <= sum0)
				ends = -1;
		}
		/* ends 0 or -1: on to the button and time checks */
		if (ends == 1)
			over = 1;
	} else if ((signed char)alive == 0) {
		over = 1;
	}
	if (over) {
		bkPrintf("GAME OVER because all men dead\n");
	} else {
		if (DR_I32(DR_VAR(void *, 0x002aab04), 0x14) == 0) {
			if (DR_AT(self, 0x38, long long) != 0)
				return;
			if (GAME_I32(0xc) != 2 && GAME_I32(0xc) != 4)
				return;
		}
	}
	if (DR_I32(DR_VAR(void *, 0x002aab04), 0x14) != 0)
		bkPrintf("GAME OVER because button pressed\n");
	if (DR_AT(self, 0x38, long long) == 0 && (GAME_I32(0xc) == 2 || GAME_I32(0xc) == 4))
		bkPrintf("GAME OVER because time out\n");

	/* game over */
	GAME_I32(0xb8) = 0x1e;
	GAME_I32(0x14) = 0;
	DR_VAR(float, 0x002aa850) = 2600.0f;
	DR_VAR(float, 0x002aa84c) = 0.0f;
	GAME_I32(0x10) = DR_STATE_OVER;
	for (i = 0; i < 4; i++) {
		int actor = DR_PL(int, i, 0x00298ec0u);

		baFlushAnimations((void *)(uintptr_t)actor, DR_AT(actor, 0x134, struct BActorNode *), 1,
			0);
	}
	DR_VAR(int, 0x002aaba0) = 0;
	if (g_DrVariant == 3)
		bsStopSample(DR_SAMPLE(0x002aa9cc));
	bsStopChannel(DR_CHANNEL(0x002aaa04));
	bsStopChannel(DR_CHANNEL(0x002aaa08));
	bsStopChannel(DR_CHANNEL(0x002aaa0c));
	bsStopChannel(DR_CHANNEL(0x002aaa10));
	if (g_DrVariant < 3) {
		for (i = 0; i < 8; i++)
			baAddExtraItem(DR_VAR(struct BExtraSystem *, 0x003014bc), 1);
	}
	if ((GAME_I32(0xc) == 1 || GAME_I32(0xc) == 3) && DR_VAR(int, 0x002ffa58) == 3) {
		for (i = 0; i < 4; i++) {
			int total;
			unsigned int rec = 0x002aab24u + i * 0x10;

			DR_VAR(int, rec + 4) = DR_PL(int, i, 0x00299570u);
			total = DR_VAR(int, rec) + DR_VAR(int, rec - 4) + DR_PL(int, i, 0x00299570u);
			DR_VAR(int, rec + 8) = total;
			GAME_I32(0x40 + i * 4) = total;
			DAT_002baee8[i * 10] = total;
		}
	}
	if (GAME_I32(0xc) == 2 || GAME_I32(0xc) == 4) {
		GAME_I32(0x40) = DR_PL(int, 0, 0x00298ed0u);
		GAME_I32(0x44) = DR_PL(int, 1, 0x00298ed0u);
		GAME_I32(0x48) = DR_PL(int, 2, 0x00298ed0u);
		GAME_I32(0x4c) = DR_PL(int, 3, 0x00298ed0u);
		CLHud_CreateWinnerText(DAT_002ecfac, (int *)DR_P(self, 0x40), 0);
	}
}

/* ---------- CGameMode_Drillers (vtable 0x0026c0d8); every slot runs on the game object */

/* 0x001114a0  Drillers (gap; CDrillers_Game.cpp inferred) */
void *CGameModeDrillers_Ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, "Drillers");
	*(const void *const **)self = vtable_0026c0d8;
	return self;
}

/* 0x001114c0  Drillers (gap; CDrillers_Game.cpp inferred) */
void CGameModeDrillers_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026c0d8;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x001114d0  Drillers (gap; CDrillers_Game.cpp inferred)
Pushed (+0x10): the character voices, then the game object loads the variant kept at
+0x28 by CModule_Drillers. */
void CGameModeDrillers_Pushed(void *self)
{
	Core_LoadCharacterVoices(1);
	DR_VAR(int, 0x002ffab8) = 0;
	CDrillersGame_Load(g_DrGame, DR_I32(self, 0x28));
	DR_I32(self, 0x14) = 1;
}

/* 0x00111500  Drillers (gap; CDrillers_Game.cpp inferred)
Suspend and resume (+0x04, +0x08): the empty function on the game object. */
void CGameModeDrillers_Suspend(void *self)
{
	(void)self;
	FUN_001d5950();
}

/* 0x00111510  Drillers (gap; thunk to 0x0010ff70) */
void CGameModeDrillers_Input(void *self, int top)
{
	(void)self;
	FUN_0010ff70(g_DrGame, top);
}

/* 0x00111520  Drillers (gap; thunk to 0x00110bf0) */
void CGameModeDrillers_Update(void *self, int top)
{
	(void)self;
	FUN_00110bf0(g_DrGame, top);
}

/* 0x00111530  Drillers (gap; thunk to 0x00110270) */
void CGameModeDrillers_Render(void *self, int top)
{
	(void)self;
	FUN_00110270(g_DrGame, top);
}

/* 0x00111540  Drillers (gap; thunk to 0x00110960) */
void CGameModeDrillers_LoadingStarted(void *self)
{
	(void)self;
	CDrillersGame_Initialise(g_DrGame);
}

/* 0x00111550  Drillers (gap; thunk to 0x0010fc00) */
void CGameModeDrillers_Start(void *self)
{
	(void)self;
	CDrillersGame_Start(g_DrGame);
}

/* 0x00111560  Drillers (gap; CDrillers_Game.cpp inferred) */
void *CGameModeDrillers_DeletingDtor(void *self, unsigned int flags)
{
	CGameModeDrillers_Dtor(self);
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x00111580  Drillers (gap; CDrillers_Game.cpp inferred)
Popped (+0x0c): unloads the game (if it was loaded: mode +0x04), or else only frees the
variant package; prints the number of free 3D sound buffers. */
void CGameModeDrillers_Popped(void *self)
{
	DSCAPS caps;

	if (DR_I32(self, 4) == 0) {
		bkClosePackageThunk((struct bkPackage *)(uintptr_t)DR_VAR(int, 0x002aa908));
		bkPrintf("this house is clean...\n");
		return;
	}
	CDrillersGame_Unload(g_DrGame);
	bkPrintf("this house is clean...\n");
	IDirectSound_GetCaps((struct IDirectSound *)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", (int)caps.dwFree3DBuffers);
}
