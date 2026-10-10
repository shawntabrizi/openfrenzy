/*
ClassLib/RefBot.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\RefBot.cpp

The referee robot ("refrobot2.obe"): an actor with a laser bone that projector rays
(CProjector.c) start from, animated screen textures, and its sounds.

Only 0x00084a30 is tagged with RefBot.cpp. The gap functions 0x000848c0-0x00084990 and
0x00084d20-0x00084ee0 all work on the RefBot instance (0x002ed448), so they are placed
here (file inferred). 0x00084880 and 0x00084970 are animation callbacks that Ghidra did
not make functions (they are not in the function table).
*/

#include "classlib_fx.h"

static const char s_RefBotFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\RefBot.cpp";

/* data: the RefBot pointer (0x002ed448) and its tables (0x00290df8-0x00290e34) are in
the data blocks of classlib_fx_data.c */

/* 0x00084880  ClassLib (not in the function table; file inferred: RefBot.cpp)
Animation callback: when the animation of the actor's own model ends (event 0), plays
it again with this callback. Returns 1. */
int FUN_00084880(int event, int actor, int model, int anim, int user)
{
	if (event == 0 && model == *(int *)(actor + 0x134))
		CLF_PlayAnim(actor, model, anim, 0x400, -1, 0, (void *)FUN_00084880, user);
	return 1;
}

/* 0x000848c0  ClassLib (gap; file inferred: RefBot.cpp)
Plays the "eye" animation on the laser bone. */
void RefBot_PlayEye(void)
{
	if (DAT_002ed448 != NULL) {
		FUN_00038190(DAT_002ed448->actor, DAT_002ed448->laserNode, 1, 0);
		CLF_PlayAnim(DAT_002ed448->actor, DAT_002ed448->laserNode, DAT_002ed448->anims[3], 0x400,
		             -1, 0, NULL, 0);
	}
}

/* 0x00084900  ClassLib (gap; file inferred: RefBot.cpp)
Starts the beam sound. */
void RefBot_PlayBeamSound(void)
{
	if (DAT_002ed448 != NULL)
		DAT_002ed448->sample = CLF_PlaySample(0x19, 0x7f, 0, 0x21, NULL);
}

/* 0x00084930  ClassLib (gap; file inferred: RefBot.cpp)
Plays the beam end sounds and stops the looping sample. */
void RefBot_StopBeamSound(void)
{
	if (DAT_002ed448 != NULL) {
		CLF_PlaySample(0x15, 0x7f, 0, 0, NULL);
		CLF_PlaySample(0x16, 0x7f, 0, 0, NULL);
		FUN_000547a0(DAT_002ea4f4);
	}
}

/* 0x00084970  ClassLib (not in the function table; file inferred: RefBot.cpp)
Animation callback: when the "out" animation ends (event 0), the RefBot is inactive. */
int FUN_00084970(int event, int actor, int model, int anim, int user)
{
	if (event == 0)
		DAT_002ed448->active = 0;
	return 1;
}

/* 0x00084990  ClassLib (gap; file inferred: RefBot.cpp)
Draws the RefBot (when active) with its render states. */
void RefBot_Draw(void)
{
	void (*set_state)(int, int, int) = (void (*)(int, int, int))FUN_00046cb0;

	if (DAT_002ed448 != NULL && DAT_002ed448->active != 0) {
		set_state(5, 1, 0);
		set_state(3, 1, 0);
		set_state(6, 0, 0);
		set_state(7, 1, 0);
		set_state(4, 1, 0);
		CLF_UpdateActor(DAT_002ed448->actor, 1);
		FUN_00034d50(DAT_002ed448->actor, 7);
		FUN_00046d00(5);
		FUN_00046d00(3);
		FUN_00046d00(6);
		FUN_00046d00(7);
		FUN_00046d00(4);
	}
}

/* 0x00084a30  ClassLib/RefBot.cpp
Creates the RefBot (unless the game mode has none): its actor, laser bone, animations
and four animated screen textures. */
void RefBot_Create(void)
{
	CRefBot *bot;
	CAnimTex *tex;
	int model;
	int actor;
	int i;

	if (DAT_002baeb8 != 0)
		return;
	if (DAT_002ed448 != NULL)
		CLF_Printf("ERROR already have a refbot. Can't make another\n");
	bot = CLF_Alloc(sizeof(CRefBot), s_RefBotFile, 0x41, 0x1001);
	if (bot != NULL) {
		CLF_ListInit(&bot->textures);
		bot->actor = 0;
		bot->laserNode = 0;
		bot->texSpeed = 1.0f;
	}
	DAT_002ed448 = bot;
	model = CLF_FindTexture(0, DAT_00290df8, 1, 0);
	DAT_002ed448->actor = FUN_00036b20(0, model, 2, (undefined4)s_RefBotFile, 0x47);
	CLF_UpdateActor(DAT_002ed448->actor, 1);
	DAT_002ed448->laserNode = CLF_FindNode(*(int *)(DAT_002ed448->actor + 0x134), "refrobo_hole");
	if (DAT_002ed448->laserNode == 0)
		CLF_Printf("**ERROR** ref bot doesn't have a laser_bone!\n");
	FUN_00035cc0(DAT_002ed448->actor, 0, 1, 0);
	FUN_00035cc0(DAT_002ed448->actor, 1, 0, 0);
	actor = DAT_002ed448->actor;
	*(float *)(actor + 0xdc) = 1.0f;
	*(float *)(actor + 0xd0) = 0.5f;
	*(float *)(actor + 0xd4) = 0.5f;
	*(float *)(actor + 0xd8) = 0.5f;
	DAT_002ed448->active = 0;
	for (i = 0; i < 4; i++)
		DAT_002ed448->anims[i] = (int)CLF_FindAnim(DAT_002ed448->actor, DAT_00290e18[i]);
	actor = DAT_002ed448->actor;
	*(float *)(actor + 0xc0) = DAT_00290e28[0];
	*(float *)(actor + 0xc4) = DAT_00290e28[1];
	*(float *)(actor + 0xc8) = DAT_00290e28[2];
	*(float *)(actor + 0xcc) = DAT_00290e28[3];
	tex = CLF_Alloc(sizeof(CAnimTex), s_RefBotFile, 0x68, 0x1001);
	if (tex != NULL)
		AnimTex_Init(tex, "flash.gif", 10.0f, &DAT_002ed448->textures);
	tex = CLF_Alloc(sizeof(CAnimTex), s_RefBotFile, 0x69, 0x1001);
	if (tex != NULL)
		AnimTex_Init(tex, "kitkar.gif", 10.0f, &DAT_002ed448->textures);
	tex = CLF_Alloc(sizeof(CAnimTex), s_RefBotFile, 0x6a, 0x1001);
	if (tex != NULL)
		AnimTex_Init(tex, "lifeline.gif", 10.0f, &DAT_002ed448->textures);
	tex = CLF_Alloc(sizeof(CAnimTex), s_RefBotFile, 0x6b, 0x1001);
	if (tex != NULL)
		AnimTex_Init(tex, "screen.gif", 10.0f, &DAT_002ed448->textures);
}

/* 0x00084d20  ClassLib (gap; file inferred: RefBot.cpp)
Moves the source of every RefBot projector to the laser bone. */
void RefBot_UpdateBeams(void)
{
	CProjector *p;
	float pos[4];

	if (DAT_002ed448 != NULL) {
		CLF_NodePosition(DAT_002ed448->actor, DAT_002ed448->laserNode, pos);
		for (p = CLF_ListNext(&DAT_002ed43c, NULL); p != NULL; p = CLF_ListNext(&DAT_002ed43c, p)) {
			p->source[0] = pos[0];
			p->source[1] = pos[1];
			p->source[2] = pos[2];
			p->source[3] = pos[3];
		}
	}
}

/* 0x00084d80  ClassLib (gap; file inferred: RefBot.cpp)
Brings the RefBot in: "in", then "idle" looping. */
void RefBot_Enter(void)
{
	int actor;

	if (DAT_002ed448 != NULL) {
		DAT_002ed448->active = 1;
		CLF_PlaySample(0x17, 0x7f, 0, 0, NULL);
		actor = DAT_002ed448->actor;
		FUN_00038190(actor, *(int *)(actor + 0x134), 1, 0);
		actor = DAT_002ed448->actor;
		CLF_PlayAnim(actor, *(int *)(actor + 0x134), DAT_002ed448->anims[0], 0x400, -1, 0, NULL, 0);
		actor = DAT_002ed448->actor;
		CLF_PlayAnim(actor, *(int *)(actor + 0x134), DAT_002ed448->anims[1], 0x400, -1, 0,
		             (void *)FUN_00084880, 0);
	}
}

/* 0x00084e20  ClassLib (gap; file inferred: RefBot.cpp)
Sends the RefBot out ("out"; inactive when it ends). */
void RefBot_Leave(void)
{
	int actor;

	if (DAT_002ed448 != NULL) {
		CLF_PlaySample(0x18, 0x7f, 0, 0, NULL);
		actor = DAT_002ed448->actor;
		FUN_00038190(actor, *(int *)(actor + 0x134), 1, 0);
		actor = DAT_002ed448->actor;
		CLF_PlayAnimBlend(actor, *(int *)(actor + 0x134), DAT_002ed448->anims[2], 0x400, -1, 0.025f,
		                  0, (void *)FUN_00084970, 0);
	}
}

/* 0x00084e90  ClassLib (gap; file inferred: RefBot.cpp)
Per-frame update while active and not paused. */
void RefBot_Update(void)
{
	int actor;

	if (DAT_002ed448 != NULL && DAT_002ed448->active != 0 && DAT_002ecdec == 0) {
		actor = DAT_002ed448->actor;
		CLF_UpdateAnim(actor, *(int *)(actor + 0x134), DAT_0028fda8, 0);
		AnimTex_AdvanceList(&DAT_002ed448->textures, DAT_002ed448->texSpeed, 1);
		RefBot_UpdateBeams();
	}
}

/* 0x00084ee0  ClassLib (gap; file inferred: RefBot.cpp)
Deletes the RefBot. */
void RefBot_Delete(void)
{
	CRefBot *bot;

	if (DAT_002ed448 != NULL) {
		FUN_00034930(DAT_002ed448->actor);
		AnimTex_DeleteList(&DAT_002ed448->textures);
		bot = DAT_002ed448;
		if (bot != NULL) {
			FUN_001c4880(bot); /* thiscall (ECX = bot): destroys the member at +0x20 */
			CLF_Free(bot);
		}
		DAT_002ed448 = NULL;
	}
}
