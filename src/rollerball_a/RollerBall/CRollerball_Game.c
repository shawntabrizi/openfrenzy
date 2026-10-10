/*
RollerBall/CRollerball_Game.c  <-  C:\BlitzParty\Game\Src\RollerBall\CRollerball_Game.cpp

The game object CRollerBall_Game (0x2ae0 bytes, DAT_0030ef04, vtable 0x00273a4c): its
sub-systems, packages, start, per-frame update and render, and the game states
(1 intro, 2 play, 3 results, 4 leave).

The functions at 0x001878b0-0x00187f50 are gap functions of this directory; the
constructor stores this file's vtable, the others use the game object, so they are
placed here. 0x00187c70, 0x00188fa0, 0x001890e0 and 0x001892e0 are missing from
of_functions.tsv (reached only through rollerball_b's game mode thunks 0x00198660..).
*/

#include "rollerball_a_ext.h"

/* 0x00273a4c  vtable of CRollerBall_Game (one slot: the deleting destructor) */
const void *const vtable_00273a4c[1] = {
	(const void *)CRollerBallGame_DeletingDtor,
};

/* 0x001878b0  RollerBall (gap; CRollerball_Game.cpp inferred)
The constructor: the sub-objects in member order. */
unsigned char *CRollerBallGame_Ctor(void *self)
{
	unsigned char *g = self;

	*(const void *const **)g = vtable_00273a4c;
	rbb_WorldCtor(g + RB_GO_WORLD);
	rbb_CameraCtor(g + RB_GO_CAMERA);
	rbb_EntitiesCtor(g + RB_GO_ENTITIES);
	CRollerBallGoals_Ctor(g + RB_GO_GOALS);
	CRollerBallRespawn_Ctor(g + RB_GO_RESPAWN);
	CRollerBallTextures_Ctor(g + RB_GO_TEXTURES);
	CRollerBallHud_Ctor(g + RB_GO_HUD);
	CRollerBallSpotFX_Ctor(g + RB_GO_SPOTFX);
	CRollerBallLights_Ctor(g + RB_GO_LIGHTS);
	CRollerBallInput_Ctor(g + RB_GO_INPUT);
	CRollerBallInfo_Ctor(g + RB_GO_INFO);
	CRollerBallSim_Ctor(g + RB_GO_SIM);
	CRollerBallTimer_Ctor(g + RB_GO_COUNTDOWN);
	CRollerBallDebug_Ctor(g + RB_GO_DEBUG);
	CRollerBallDirect3D_Ctor(g + RB_GO_DIRECT3D);
	CRollerBallSfx_Ctor(g + RB_GO_SFX);
	CRollerBallVideo_Ctor(g + RB_GO_VIDEO);
	CRollerBall2740_Ctor(g + RB_GO_2740);
	CRollerBallShaders_Ctor(g + RB_GO_SHADERS);
	CRollerBall2850_Ctor(g + RB_GO_2850);
	CRollerBallRumble_Ctor(g + RB_GO_RUMBLE);
	CParticleSystemList_Construct((struct CParticleSystemList *)(g + RB_GO_PARTICLES));
	RB_PTR(g, 0x18) = NULL;
	return g;
}

/* 0x00187a50  RollerBall (gap; CRollerball_Game.cpp inferred)
The destructor: the sub-objects in reverse order. */
void CRollerBallGame_Dtor(void *self)
{
	unsigned char *g = self;

	*(const void *const **)g = vtable_00273a4c;
	CParticleSystemList_Destruct((struct CParticleSystemList *)(g + RB_GO_PARTICLES));
	CRollerBallRumble_Dtor(g + RB_GO_RUMBLE);
	CRollerBall2850_Dtor(g + RB_GO_2850);
	CRollerBallShaders_Dtor(g + RB_GO_SHADERS);
	CRollerBall2740_Dtor(g + RB_GO_2740);
	CRollerBallVideo_Dtor(g + RB_GO_VIDEO);
	CRollerBallSfx_Dtor(g + RB_GO_SFX);
	CRollerBallDirect3D_Dtor(g + RB_GO_DIRECT3D);
	CRollerBallDebug_Dtor(g + RB_GO_DEBUG);
	CRollerBallTimer_Dtor(g + RB_GO_COUNTDOWN);
	CRollerBallSim_Dtor(g + RB_GO_SIM);
	CRollerBallInfo_Dtor(g + RB_GO_INFO);
	CRollerBallInput_Dtor(g + RB_GO_INPUT);
	CRollerBallLights_Dtor(g + RB_GO_LIGHTS);
	CRollerBallSpotFX_Dtor(g + RB_GO_SPOTFX);
	CRollerBallHud_Dtor(g + RB_GO_HUD);
	CRollerBallTextures_Dtor(g + RB_GO_TEXTURES);
	CRollerBallRespawn_Dtor(g + RB_GO_RESPAWN);
	CRollerBallGoals_Dtor(g + RB_GO_GOALS);
	rbb_EntitiesDtor(g + RB_GO_ENTITIES);
	rbb_CameraDtor(g + RB_GO_CAMERA);
	rbb_WorldDtor(g + RB_GO_WORLD);
}

/* 0x00187bf0  RollerBall (gap; CRollerball_Game.cpp inferred)
Loads the character voices and the packages packages\roll-v<n> (variation + 1) and
packages\roll-gen, and keeps their ids. */
void CRollerBallGame_LoadPackages(unsigned char *self)
{
	char name[256];
	void *package;

	Core_LoadCharacterVoices(1);
	sprintf(name, "packages\\roll-v%d", RB_I32(self, RB_GO_VARIATION) + 1);
	package = RB_CALL(CLLoading_AddPackage, void *(*)(const char *, int))(name, 0);
	RB_PTR(self, RB_GO_PACKAGE) = package;
	if (package != NULL) {
		RB_U32(self, RB_GO_PACKAGEID) = bkGetPackageId((struct bkPackage *)package);
		sprintf(name, "packages\\roll-gen");
		package = RB_CALL(CLLoading_AddPackage, void *(*)(const char *, int))(name, 0);
		RB_PTR(self, RB_GO_GENPACKAGE) = package;
		if (package != NULL)
			RB_U32(self, RB_GO_GENPACKAGEID) = bkGetPackageId((struct bkPackage *)package);
	}
}

/* 0x00187c70  RollerBall (missing from the table; reached by rollerball_b's mode thunk
0x00198696): when ARG is 0 and the game mode's +0x14 is set, calls its virtual +0x24. */
void FUN_00187c70(void *self, int arg)
{
	unsigned char *mode;

	if (arg == 0) {
		mode = RB_PTR(self, RB_GO_MODE);
		if (RB_I32(mode, 0x14) != 0)
			RB_VFN(mode, 0x24, void (*)(void *))(mode);
	}
}

/* the sub-system updates shared by the intro and play states */
static void rb_UpdateSystems(unsigned char *self, float dt, int goals)
{
	unsigned char *var;

	rbb_EntitiesUpdate(self + RB_GO_ENTITIES, dt);
	if (goals)
		rbb_GoalsUpdate(self + RB_GO_GOALS, dt);
	CRollerBallInput_Update(self + RB_GO_INPUT, dt);
	var = RB_PTR(self, RB_GO_VAROBJ);
	RB_VFN(var, 0x0c, void (*)(void *, float))(var, dt);
	CRollerBallRespawn_Update(self + RB_GO_RESPAWN, dt);
	CRollerBallTextures_Update(self + RB_GO_TEXTURES, dt);
	rbb_CameraUpdate(self + RB_GO_CAMERA, dt);
	CRollerBallHud_Update(self + RB_GO_HUD, dt);
	CRollerBallSpotFX_Update(self + RB_GO_SPOTFX, dt);
	CRollerBallLights_Update(self + RB_GO_LIGHTS, dt);
	CDemoObjectList_Update(self + RB_GO_PARTICLES, dt);
	FUN_001ec260(); /* empty (ECX = the sfx system, dt) */
	rbb_WorldUpdate(self + RB_GO_WORLD, dt);
	CRollerBallVideo_Update(self + RB_GO_VIDEO, dt);
	CRollerBall2850_Update(self + RB_GO_2850, dt);
	FUN_001ec260(); /* empty (ECX = the rumble system, dt) */
	rbb_ShadowsUpdate(self + RB_GO_SHADOWS, dt);
}

/* 0x00187c90  RollerBall (gap; CRollerball_Game.cpp inferred)
Game state 1 (intro): while +0x13b0 is 1..3 the world runs; at 3 the game switches to
state 2 (play), starts the countdown and calls the variation's virtual +8 (start). */
void CRollerBallGame_UpdateIntro(unsigned char *self, float dt)
{
	unsigned char *var;
	int step = RB_I32(self, 0x13b0);

	if (step <= 0)
		return;
	if (step > 2) {
		if (step != 3)
			return;
		RB_I16(self, RB_GO_STATE) = 2;
		CRollerBallTimer_Start(self + RB_GO_COUNTDOWN);
		var = RB_PTR(self, RB_GO_VAROBJ);
		RB_VFN(var, 0x08, void (*)(void *))(var);
	}
	rb_UpdateSystems(self, dt, 0);
}

/* 0x00187da0  RollerBall (gap; CRollerball_Game.cpp inferred)
Game states 2..4: the per-frame update of every sub-system, goals included. */
void CRollerBallGame_UpdatePlay(unsigned char *self, float dt)
{
	rb_UpdateSystems(self, dt, 1);
}

/* 0x00187e80  RollerBall (gap; CRollerball_Game.cpp inferred)
Fog and ambient light of the current variation; sets core's DAT_002ba940..948. */
void CRollerBallGame_SetFog(void)
{
	int v = RB_I32(RB_GAME, RB_GO_VARIATION);

	if (RB_VS(int, v, 0x002b4bb8u) == 0) {
		bdSetRenderState(8, 0, 0);
	} else {
		bdSetRenderState(8, 1, 0);
		v = RB_I32(RB_GAME, RB_GO_VARIATION);
		bdSetFogColour(RB_VS(unsigned int, v, 0x002b4bbcu), RB_VS(unsigned int, v, 0x002b4bc0u),
			RB_VS(unsigned int, v, 0x002b4bc4u));
		v = RB_I32(RB_GAME, RB_GO_VARIATION);
		bdSetFogRange(RB_VS(float, v, 0x002b4bc8u), RB_VS(float, v, 0x002b4bccu));
	}
	v = RB_I32(RB_GAME, RB_GO_VARIATION);
	baSetAmbientLight(RB_VS(int, v, 0x002b4bd0u), RB_VS(int, v, 0x002b4bd4u),
		RB_VS(int, v, 0x002b4bd8u));
	DAT_002ba940 = RB_VS(int, RB_I32(RB_GAME, RB_GO_VARIATION), 0x002b4bdcu);
	DAT_002ba944 = RB_VS(int, RB_I32(RB_GAME, RB_GO_VARIATION), 0x002b4be0u);
	DAT_002ba948 = RB_VS(int, RB_I32(RB_GAME, RB_GO_VARIATION), 0x002b4be4u);
}

/* ======================================================================== the variations */

/* Five variation classes share one 18-slot interface (vtables 0x00273a78, 0x00273ac0,
0x00273b08, 0x00273b50, 0x00273b98):
  +0x00 Initialise   +0x04 Shutdown   +0x08 Start        +0x0c Update(dt)
  +0x10 Draw         +0x14 Reset      +0x18 SetScores    +0x1c (int)
  +0x20 (float)      +0x24..+0x2c, +0x38..+0x40 (float getters, 0 by default)
  +0x30, +0x34 (int, 0 by default)    +0x44 deleting destructor */

/* 0x00187f60  RollerBall/CRollerball_Game.cpp: the default float getter (0) */
float CRollerBallVariation_Zero(void *self)
{
	(void)self;
	return 0.0f;
}

/* 0x00187f70  RollerBall/CRollerball_Game.cpp
Start (base, race): every player gets the flags +0x1a, +0x1b, +0x1c; the players are
released. */
void CRollerBallVariation_Start(void *self)
{
	unsigned char *p;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1A) = 1;
		RB_U8(p, RB_PL_1B) = 1;
		RB_U8(p, RB_PL_1C) = 1;
	}
	rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 0);
}

/* 0x00187fb0  RollerBall/CRollerball_Game.cpp
Update of the "last ball standing" variation: players that fell (y < -400) respawn; when
the countdown ends the winners (best score; in a team game the team with more points) get
the winner message (3) and the others the loser message (6), the game goes to state 3,
the result time is 5 s per winner (7 s for one human winner), every player is put back
on its start point, and the HUD shows the winner text. */
void CRollerBallBase_UpdatePlay(void *self)
{
	struct RB_Msg msg;
	unsigned char *p, *first, *q, *other;
	int best, a, b, winners;

	rbb_MsgCtor(&msg);
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) != 0 && RB_F32(p, RB_PL_POS + 4) < -400.0f)
			CRollerBallRespawn_Respawn(RB_GAME + RB_GO_RESPAWN, p, RB_FP(p, RB_PL_POS));
	}
	if (CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN) <= 0.0f) {
		rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 1);
		first = RB_FIRST_PLAYER();
		if (DAT_002baebc == 0) {
			best = RB_I32(first, RB_PL_SCORE);
			for (p = first; p != NULL; p = RB_NEXT(p)) {
				RB_U8(p, RB_PL_1A) = 0;
				RB_U8(p, RB_PL_ACTIVE) = 0;
				if (best <= RB_I32(p, RB_PL_SCORE))
					best = RB_I32(p, RB_PL_SCORE);
			}
			for (p = first; p != NULL; p = RB_NEXT(p))
				RB_U8(p, RB_PL_WINNER) = best <= RB_I32(p, RB_PL_SCORE);
		} else {
			for (p = first; p != NULL; p = RB_NEXT(p)) {
				RB_U8(p, RB_PL_1A) = 0;
				RB_U8(p, RB_PL_ACTIVE) = 0;
				RB_U8(p, RB_PL_1C) = 0;
			}
			/* the first player of the other team. The original reads an uninitialised
			local when there is none (a team game always has two teams); the port then
			uses the first player. */
			other = first;
			for (q = first; q != NULL; q = RB_NEXT(q)) {
				if (q != first && q != RB_AT(first, RB_PL_TEAMMATE, unsigned char *)) {
					other = q;
					break;
				}
			}
			a = RB_I32(first, RB_PL_SCORE) +
				RB_I32(RB_AT(first, RB_PL_TEAMMATE, unsigned char *), RB_PL_SCORE);
			b = RB_I32(RB_AT(other, RB_PL_TEAMMATE, unsigned char *), RB_PL_SCORE) +
				RB_I32(other, RB_PL_SCORE);
			for (p = first; p != NULL; p = RB_NEXT(p))
				RB_U8(p, RB_PL_WINNER) = 1;
			if (b < a) {
				RB_U8(other, RB_PL_WINNER) = 0;
				RB_U8(RB_AT(other, RB_PL_TEAMMATE, unsigned char *), RB_PL_WINNER) = 0;
			} else if (a < b) {
				RB_U8(first, RB_PL_WINNER) = 0;
				RB_U8(RB_AT(first, RB_PL_TEAMMATE, unsigned char *), RB_PL_WINNER) = 0;
			}
		}
		winners = 0;
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			msg.value = 0.5f;
			msg.b9 = 1;
			msg.b8 = 1;
			msg.unique = 0;
			msg.prio = 2;
			msg.id = 3;
			if (RB_U8(p, RB_PL_WINNER) == 0) {
				msg.arg = 6;
				rbb_PostMsg(p, &msg);
			} else {
				msg.arg = 3;
				rbb_PostMsg(p, &msg);
				winners++;
			}
		}
		RB_I16(RB_GAME, RB_GO_STATE) = 3;
		CRollerBallTimer_Stop(RB_GAME + RB_GO_COUNTDOWN);
		rbb_CameraSetMode(RB_GAME + RB_GO_CAMERA, 4);
		if (DAT_002bae98 == 1 && winners == 1)
			RB_F32(RB_GAME, RB_GO_RESULTTIME) = 7.0f;
		else
			RB_F32(RB_GAME, RB_GO_RESULTTIME) = (float)winners * 5.0f;
		CRollerBallTimer_Set(RB_GAME + RB_GO_TIMER, RB_F32(RB_GAME, RB_GO_RESULTTIME));
		CRollerBallTimer_Start(RB_GAME + RB_GO_TIMER);
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			RB_F32(p, RB_PL_VEL) = 0.0f;
			RB_F32(p, RB_PL_VEL + 4) = 0.0f;
			RB_F32(p, RB_PL_VEL + 8) = 0.0f;
			rbb_PlayerReset(p);
			rbb_PlayerPlace(p, (const float *)(RB_GAME + 0x730 + RB_I32(p, RB_PL_SLOT) * 0x20));
			RB_U8(p, RB_PL_1B) = 0;
		}
		RB_VFN(self, 0x18, void (*)(void *))(self);
		{
			int scores[4];

			scores[1] = DAT_002baf10;
			scores[0] = DAT_002baee8;
			scores[2] = DAT_002baf38;
			scores[3] = DAT_002baf60;
			CLHud_CreateWinnerText(DAT_002ecfac, scores, 0);
		}
	}
	rbb_MsgDtor(&msg);
}

/* the end of the results state, shared by the variations: when the result time is almost
over (2 s passed) and a winning human player (or, without one, any human player) presses a
button (pad +0x14), or when the timer ran out and the commentary is idle, the game goes to
state 4 (leave) with a 0.7 s timer. */
static void rb_UpdateResults(void)
{
	unsigned char *first, *p;
	unsigned char **pad;
	int *rec;
	int leave = 0;
	float t;

	t = CRollerBallTimer_Remaining(RB_GAME + RB_GO_TIMER);
	if (t < RB_F32(RB_GAME, RB_GO_RESULTTIME) - 2.0f) {
		first = RB_FIRST_PLAYER();
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (RB_U8(p, RB_PL_WINNER) != 0 && RB_U8(p, RB_PL_AI) == 0)
				break;
		}
		rec = &DAT_002baed8;
		pad = (unsigned char **)(RB_GAME + RB_GO_PADS);
		if (p != NULL) {
			/* a human winner exists: only the winners' buttons count */
			for (p = first; p != NULL; p = RB_NEXT(p), rec += 10, pad++) {
				if (RB_U8(p, RB_PL_WINNER) != 0 && *rec != 0 && RB_I32(*pad, 0x14) != 0) {
					leave = 1;
					break;
				}
			}
		} else {
			for (; rec < &DAT_002baed8 + 40; rec += 10, pad++) {
				if (*rec != 0 && RB_I32(*pad, 0x14) != 0) {
					leave = 1;
					break;
				}
			}
		}
	}
	if ((t <= 0.0f && CLSamples_CommentaryIdle() != 0) || leave) {
		RB_I16(RB_GAME, RB_GO_STATE) = 4;
		CRollerBallTimer_Set(RB_GAME + RB_GO_TIMER, 0.7f);
		CRollerBallTimer_Start(RB_GAME + RB_GO_TIMER);
		RB_I32(RB_GAME, 0x13c4) = 1;
	}
}

/* 0x001882f0  RollerBall/CRollerball_Game.cpp
Results state of the variations (see rb_UpdateResults). */
void CRollerBallVariation_UpdateResults(void)
{
	rb_UpdateResults();
}

/* 0x00188410  RollerBall/CRollerball_Game.cpp
Reset (base, race): players lose the flags +0x1a, +0x1b; state 1 (intro); camera mode 1. */
void CRollerBallVariation_Reset(void *self)
{
	unsigned char *p;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1A) = 0;
		RB_U8(p, RB_PL_1B) = 0;
	}
	RB_I16(RB_GAME, RB_GO_STATE) = 1;
	rbb_CameraSetMode(RB_GAME + RB_GO_CAMERA, 1);
}

/* 0x00188450  RollerBall/CRollerball_Game.cpp
SetScores (base): core's player scores (records of 0x28 bytes) = the players' points. */
void CRollerBallVariation_SetScores(void *self)
{
	unsigned char *p;
	int *score = &DAT_002baee8;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), score += 10)
		*score = RB_I32(p, RB_PL_SCORE);
}

/* ======================================================================== CRollerBall_Sumo */

/* Variation "sumo" (vtable 0x00273ac0, 0x2a0 bytes): +0x08 a timer, +0x40 the ring radius,
+0x44 five platforms of 0x18 bytes (+0x48 state, +0x4c, +0x50 time, +0x54, +0x58 the
world object, +0x5c the platform object), +0xc0 the ring, +0x260 a flag, +0x264 four arms
(world object, "retract" sub-object), +0x284 the ramp platform, +0x288 four random
times. */

/* 0x00188480  RollerBall/CRollerball_Game.cpp: constructor */
void *CRollerBallSumo_Ctor(void *self)
{
	*(const void *const **)self = vtable_00273ac0;
	CRollerBallTimer_Ctor((unsigned char *)self + 0x08);
	rbb_PlatformsCtor((unsigned char *)self + 0xc0);
	return self;
}

/* 0x001884d0  RollerBall/CRollerball_Game.cpp: +4 (vtable +0x1c of sumo and drop) */
int CRollerBallVariation_Get4(void *self)
{
	return RB_I32(self, 4);
}

/* 0x001884e0  RollerBall/CRollerball_Game.cpp: destructor */
void CRollerBallSumo_Dtor(void *self)
{
	*(const void *const **)self = vtable_00273ac0;
	rbb_PlatformsDtor((unsigned char *)self + 0xc0);
	CRollerBallTimer_Dtor((unsigned char *)self + 0x08);
}

/* 0x00188540  RollerBall/CRollerball_Game.cpp
Shutdown: the ring, then the five platform objects (virtual deleting destructor). */
void CRollerBallSumo_Shutdown(void *self)
{
	void **obj;
	int i;

	FUN_001d5950(); /* empty (ECX = the timer) */
	rbb_PlatformsShutdown((unsigned char *)self + 0xc0);
	obj = (void **)((unsigned char *)self + 0x5c);
	for (i = 5; i != 0; i--, obj += 6) {
		if (*obj != NULL) {
			rbb_PlatformDelete(*obj);
			if (*obj != NULL)
				RB_VFN(*obj, 0x00, void *(*)(void *, unsigned int))(*obj, 1);
			*obj = NULL;
		}
	}
}

/* 0x00188590  RollerBall/CRollerball_Game.cpp
Start (sumo): every player gets the flags +0x1b, +0x1c; the players are released. */
void CRollerBallSumo_Start(void *self)
{
	unsigned char *p;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1B) = 1;
		RB_U8(p, RB_PL_1C) = 1;
	}
	rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 0);
}

/* 0x001885d0  RollerBall/CRollerball_Game.cpp
Results state of sumo: when no platform is in state 2 its timer stops; then as
0x001882f0. */
void CRollerBallSumo_UpdateResults(void *self)
{
	int i, moving = 0;

	for (i = 0; i < 5; i++) {
		if (RB_I32(self, 0x48 + i * 0x18) == 2)
			moving = 1;
	}
	if (!moving)
		CRollerBallTimer_Stop((unsigned char *)self + 0x08);
	rb_UpdateResults();
}

/* 0x00188720  RollerBall/CRollerball_Game.cpp
Draw (sumo): the ring, then the five platforms. */
void CRollerBallSumo_Draw(void *self)
{
	void **obj;
	int i;

	rbb_Platforms_196e80((unsigned char *)self + 0xc0);
	obj = (void **)((unsigned char *)self + 0x5c);
	for (i = 5; i != 0; i--, obj += 6)
		rbb_Platform_1973f0(*obj);
}

/* the points of the ranks 1..4 at the field OFF of the players: the players of the best
rank get 10, the next 5, then 3, then 1 (ranks without players are skipped) */
static void rb_ScoresByRank(int off)
{
	unsigned char *first = RB_FIRST_PLAYER(), *p;
	int rank, place = 0, count, points = 0;
	int *score;

	for (rank = 1; rank < 5; rank++) {
		if (first == NULL)
			continue;
		count = 0;
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (RB_I32(p, off) == rank)
				count++;
		}
		if (count == 0)
			continue;
		score = &DAT_002baee8;
		for (p = first; p != NULL; p = RB_NEXT(p), score += 10) {
			if (RB_I32(p, off) == rank) {
				switch (place) {
				case 0:
					points = 10;
					break;
				case 1:
					points = 5;
					break;
				case 2:
					points = 3;
					break;
				case 3:
					points = 1;
					break;
				}
				*score = points;
			}
		}
		place++;
	}
}

/* 0x00188750  RollerBall/CRollerball_Game.cpp
SetScores (sumo): by rank (+0x41f8). */
void CRollerBallSumo_SetScores(void *self)
{
	(void)self;
	rb_ScoresByRank(RB_PL_RANK);
}

/* the field at +0x0c (A) or +0x08 (B) of the first platform whose state is not 2 or 3
(sumo: platforms of 0x18 bytes from +0x48) */
static float rb_PlatformValue(void *self, int stride, int state0, int value0, int fallback)
{
	int i = 0;

	while (RB_I32(self, state0 + i * stride) > 1 && RB_I32(self, state0 + i * stride) < 4) {
		i++;
		if (i > 4)
			return RB_F32(self, fallback);
	}
	return RB_F32(self, value0 + i * stride);
}

/* 0x001887f0  RollerBall/CRollerball_Game.cpp: vtable +0x24 (sumo) */
float CRollerBallSumo_GetValueA(void *self)
{
	return rb_PlatformValue(self, 0x18, 0x48, 0x54, 0xb0);
}

/* 0x00188830  RollerBall/CRollerball_Game.cpp: vtable +0x28 (sumo) */
float CRollerBallSumo_GetValueB(void *self)
{
	return rb_PlatformValue(self, 0x18, 0x48, 0x50, 0xb0);
}

/* 0x00188870  RollerBall/CRollerball_Game.cpp: vtable +0x2c (sumo)
The variation timer while a platform object is active (+0x78), else 60. */
float CRollerBallSumo_GetTime(void *self)
{
	int i;

	for (i = 0; i < 5; i++) {
		if (RB_U8(RB_PTR(self, 0x5c + i * 0x18), 0x78) != 0)
			return CRollerBallTimer_Remaining((unsigned char *)self + 0x08);
	}
	return 60.0f;
}

/* 0x001888b0  RollerBall/CRollerball_Game.cpp: +0x28 (race2 vtable +0x30) */
int CRollerBallRace2_Get28(void *self)
{
	return RB_I32(self, 0x28);
}

/* 0x001888c0  RollerBall/CRollerball_Game.cpp: +0x2c (race2 vtable +0x34) */
int CRollerBallRace2_Get2c(void *self)
{
	return RB_I32(self, 0x2c);
}

/* 0x001888d0  RollerBall/CRollerball_Game.cpp
Array constructor callback of the 0x30-byte path records. */
void *CRollerBallPath_Construct(void *self)
{
	rbb_PathCtor(self);
	return self;
}

/* 0x001888e0  RollerBall (missing from the table)
Array destructor callback of the 0x30-byte path records (jmp to rollerball_b 0x00198110). */
void FUN_001888e0(void *self)
{
	rbb_PathDtor(self);
}

/* 0x001888f0  RollerBall/CRollerball_Game.cpp
Start (race2): every player gets the flags +0x1a, +0x1b; the players are released. */
void CRollerBallRace2_Start(void *self)
{
	unsigned char *p;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1A) = 1;
		RB_U8(p, RB_PL_1B) = 1;
	}
	rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 0);
}

/* the points of a place: 5, 3, 1, then 0 */
static int rb_PlacePoints(int place)
{
	if (place == 0)
		return 5;
	if (place == 1)
		return 3;
	if (place == 2)
		return 1;
	return 0;
}

/* 0x00188930  RollerBall/CRollerball_Game.cpp
SetScores (race2): players that finished a lap (+0x41f0 > 0) score by their place
(+0x41f8): 5, 3, 1; the others 0. */
void CRollerBallRace2_SetScores(void *self)
{
	unsigned char *p;
	int *score = &DAT_002baee8;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), score += 10) {
		*score = 0;
		if (RB_I32(p, RB_PL_41F0) > 0)
			*score = rb_PlacePoints(RB_I32(p, RB_PL_RANK));
	}
}

/* 0x00188990  RollerBall/CRollerball_Game.cpp: +0x34 (race vtable +0x30) */
int CRollerBallRace_Get34(void *self)
{
	return RB_I32(self, 0x34);
}

/* 0x001889a0  RollerBall/CRollerball_Game.cpp: +0x38 (race vtable +0x34) */
int CRollerBallRace_Get38(void *self)
{
	return RB_I32(self, 0x38);
}

/* 0x001889b0  RollerBall/CRollerball_Game.cpp
SetScores (race): a player that finished (+0x41f0 >= 3 laps) has its place (+0x41f8);
the others are placed by how many players finished or are further (+0x41ec). Places score
5, 3, 1, then 0. */
void CRollerBallRace_SetScores(void *self)
{
	unsigned char *p, *q;
	int *score = &DAT_002baee8;
	int place;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), score += 10) {
		if (RB_I32(p, RB_PL_41F0) < 3) {
			place = 0;
			for (q = RB_FIRST_PLAYER(); q != NULL; q = RB_NEXT(q)) {
				if (p != q && (RB_I32(q, RB_PL_41F0) > 2 ||
						RB_F32(p, 0x41ec) < RB_F32(q, 0x41ec)))
					place++;
			}
		} else {
			place = RB_I32(p, RB_PL_RANK);
		}
		*score = rb_PlacePoints(place);
	}
}

/* ======================================================================== CRollerBall_Drop */

/* Variation "drop" (vtable 0x00273b98): +0x08 a timer, +0x44 five platforms of 0x58 bytes
(+0x44 state, +0x4c, +0x50, +0x58 the platform object). */

/* 0x00188a50  RollerBall/CRollerball_Game.cpp: constructor */
void *CRollerBallDrop_Ctor(void *self)
{
	*(const void *const **)self = vtable_00273b98;
	CRollerBallTimer_Ctor((unsigned char *)self + 0x08);
	return self;
}

/* 0x00188a70  RollerBall/CRollerball_Game.cpp: vtable +0x20 (sumo, drop)
jmp to 0x000965f0 with ECX = the timer (+8): a folded function that returns the float
at +0x34 (the timer's elapsed fraction). */
float CRollerBallVariation_TimerFraction(void *self)
{
	return RB_F32((unsigned char *)self + 0x08, 0x34);
}

/* 0x00188a80  RollerBall/CRollerball_Game.cpp: destructor */
void CRollerBallDrop_Dtor(void *self)
{
	*(const void *const **)self = vtable_00273b98;
	CRollerBallTimer_Dtor((unsigned char *)self + 0x08);
}

/* 0x00188a90  RollerBall/CRollerball_Game.cpp
Shutdown (drop): the five platform objects (virtual deleting destructor). */
void CRollerBallDrop_Shutdown(void *self)
{
	void **obj;
	int i;

	FUN_001d5950(); /* empty (ECX = the timer) */
	obj = (void **)self;
	for (i = 5; i != 0; i--) {
		obj += 0x16;
		if (*obj != NULL) {
			rbb_PlatformDelete(*obj);
			if (*obj != NULL)
				RB_VFN(*obj, 0x00, void *(*)(void *, unsigned int))(*obj, 1);
			*obj = NULL;
		}
	}
}

/* 0x00188ad0  RollerBall/CRollerball_Game.cpp
Start (drop): active players (+0x15) get the flags +0x1a, +0x1b, +0x1c; the players are
released. */
void CRollerBallDrop_Start(void *self)
{
	unsigned char *p;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_15) != 0) {
			RB_U8(p, RB_PL_1B) = 1;
			RB_U8(p, RB_PL_1A) = 1;
			RB_U8(p, RB_PL_1C) = 1;
		}
	}
	rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 0);
}

/* 0x00188b10  RollerBall/CRollerball_Game.cpp
Draw (drop): the five platforms. */
void CRollerBallDrop_Draw(void *self)
{
	void **obj;
	int i;

	obj = (void **)((unsigned char *)self + 0x58);
	for (i = 5; i != 0; i--, obj += 0x16)
		rbb_Platform_1973f0(*obj);
}

/* 0x00188b30  RollerBall/CRollerball_Game.cpp
SetScores (drop): by the field +0x41f0. */
void CRollerBallDrop_SetScores(void *self)
{
	(void)self;
	rb_ScoresByRank(RB_PL_41F0);
}

/* 0x00188bd0  RollerBall/CRollerball_Game.cpp: vtable +0x38 (drop) */
float CRollerBallDrop_GetValueA(void *self)
{
	return rb_PlatformValue(self, 0x58, 0x44, 0x50, 0x1ac);
}

/* 0x00188c10  RollerBall/CRollerball_Game.cpp: vtable +0x3c (drop) */
float CRollerBallDrop_GetValueB(void *self)
{
	return rb_PlatformValue(self, 0x58, 0x44, 0x4c, 0x1ac);
}

/* 0x00188c50  RollerBall/CRollerball_Game.cpp: vtable +0x40 (drop)
The variation timer while a platform object is active (+0x78), else 60. */
float CRollerBallDrop_GetTime(void *self)
{
	int i;

	for (i = 0; i < 5; i++) {
		if (RB_U8(RB_PTR(self, 0x58 + i * 0x58), 0x78) != 0)
			return CRollerBallTimer_Remaining((unsigned char *)self + 0x08);
	}
	return 60.0f;
}

/* 0x00188c90  RollerBall/CRollerball_Game.cpp
The game object's deleting destructor (flags bit 0: free the object). */
void *CRollerBallGame_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallGame_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== game: start .. render */

/* 0x00188cb0  RollerBall/CRollerball_Game.cpp
Start of variation VARIATION (1-4; the setting "variation" overrides it with a 0-based
value): pushes a new RollerBall game mode (0x28 bytes, rollerball_b), then the minigame
intro (game 0x4c + variation, picture "roll<n>.bmp"). Returns 0 when a mode could not be
pushed. (The original guards the allocations with a C++ exception frame.) */
int CRollerBallGame_Start(unsigned char *self, int variation)
{
	char picture[256];
	void *mode;
	const char *setting;

	mode = RB_NEW(0x28, RB_FILE_GAME, 0xe1);
	if (mode != NULL)
		mode = rbb_GameModeCtor(mode);
	RB_PTR(self, RB_GO_MODE) = mode;
	setting = Settings_Get((struct Settings *)DAT_002ba9a0, "variation");
	if (setting == NULL)
		RB_I32(self, RB_GO_VARIATION) = variation - 1;
	else
		RB_I32(self, RB_GO_VARIATION) = (int)atol(setting);
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode,
			1) == 0) {
		RB_PTR(self, RB_GO_MODE) = NULL;
		return 0;
	}
	sprintf(picture, "roll%d.bmp", RB_I32(self, RB_GO_VARIATION) + 1);
	mode = RB_NEW(0x300, RB_FILE_GAME, 0xf5);
	if (mode != NULL)
		mode = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)mode,
			RB_I32(self, RB_GO_VARIATION) + 0x4c, picture);
	return CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode,
		1) != 0;
}

/* 0x00188df0  RollerBall/CRollerball_Game.cpp
Shutdown: when the game mode never initialised (+4) only the packages are closed. Else
every sub-system shuts down, the variation object is shut down and freed, the package
resources are deleted, the packages closed, and core's fog values and the sound fall-off
distances are restored; fog off. */
void CRollerBallGame_Shutdown(unsigned char *self)
{
	unsigned char *var;

	if (RB_I32(RB_PTR(self, RB_GO_MODE), 4) == 0) {
		bkClosePackageThunk(RB_PTR(self, RB_GO_PACKAGE));
		bkClosePackageThunk(RB_PTR(self, RB_GO_GENPACKAGE));
		return;
	}
	FUN_001d5950(); /* empty (ECX = the debug system) */
	CRollerBallSpotFX_Shutdown(self + RB_GO_SPOTFX);
	CDemoOverlay_Shutdown(self + RB_GO_HUD);
	CRollerBallLights_Shutdown(self + RB_GO_LIGHTS);
	FUN_001d5950(); /* empty (ECX = the texture animations) */
	FUN_001d5950(); /* empty (ECX = the simulation) */
	FUN_001d5950(); /* empty (ECX = the countdown) */
	FUN_001d5950(); /* empty (ECX = the input) */
	rbb_EntitiesShutdown(self + RB_GO_ENTITIES);
	rbb_WorldShutdown(self + RB_GO_WORLD);
	FUN_001bb790(); /* the intro camera (thunk to CLIntroCam_Destroy) */
	CRollerBallRespawn_Shutdown(self + RB_GO_RESPAWN);
	CRollerBallGoals_Shutdown(self + RB_GO_GOALS);
	FUN_001d5950(); /* empty (ECX = the Direct3D system) */
	CParticleSystemList_DeleteAll((struct CParticleSystemList *)(self + RB_GO_PARTICLES));
	CRollerBallSfx_Shutdown(self + RB_GO_SFX);
	CRollerBallInfo_Shutdown(self + RB_GO_INFO);
	CRollerBallVideo_Shutdown(self + RB_GO_VIDEO);
	CRollerBallShaders_Shutdown(self + RB_GO_SHADERS);
	CRollerBall2850_Shutdown(self + RB_GO_2850);
	CDemoRumble_Shutdown(self + RB_GO_RUMBLE);
	rbb_ShadowsShutdown(self + RB_GO_SHADOWS);
	var = RB_PTR(self, RB_GO_VAROBJ);
	if (var != NULL) {
		RB_VFN(var, 0x04, void (*)(void *))(var);
		RB_DELETE(RB_PTR(self, RB_GO_VAROBJ));
		RB_PTR(self, RB_GO_VAROBJ) = NULL;
	}
	bkDeletePackageResources(RB_U32(self, RB_GO_PACKAGEID), 1);
	bkDeletePackageResources(RB_U32(self, RB_GO_GENPACKAGEID), 1);
	bkClosePackageThunk(RB_PTR(self, RB_GO_PACKAGE));
	bkClosePackageThunk(RB_PTR(self, RB_GO_GENPACKAGE));
	DAT_002ba940 = RB_I32(self, 0x28);
	RB_U8(self, RB_GO_QUIT) = 0;
	DAT_002ba944 = RB_I32(self, 0x2c);
	DAT_002ba948 = RB_I32(self, 0x30);
	DAT_002ba94c = RB_I32(self, 0x34);
	DAT_0028ffa8 = RB_F32(self, 0x38);
	DAT_0028ffac = RB_F32(self, 0x3c);
	bdSetRenderState(8, 0, 0);
}

/* 0x00188fa0  RollerBall (missing from the table; reached by rollerball_b's mode thunk
0x0019867a)
Begin: saves core's fog values and the sound fall-off distances (then 4000 / 15000, or
20000 in variation 3), sets the fog, the far plane (70000, or 140000 in variation 4), the
HUD, the start sound (sample 7 at volume 255 in variations 0 and 1, sample 13 at 127 in
2..4), resets the simulation clock, and starts a 0.5 s timer. */
void FUN_00188fa0(void *self)
{
	unsigned char *g = self;
	int sample, volume;

	RB_I32(g, 0x28) = DAT_002ba940;
	RB_I32(g, 0x2c) = DAT_002ba944;
	RB_I32(g, 0x30) = DAT_002ba948;
	RB_I32(g, 0x34) = DAT_002ba94c;
	DAT_002ba940 = 0;
	DAT_002ba944 = 0;
	DAT_002ba948 = 0;
	DAT_002ba94c = 0;
	RB_F32(g, 0x38) = DAT_0028ffa8;
	RB_F32(g, 0x3c) = DAT_0028ffac;
	DAT_0028ffa8 = 4000.0f;
	DAT_0028ffac = 15000.0f;
	if (RB_I32(g, RB_GO_VARIATION) == 3)
		DAT_0028ffac = 20000.0f;
	CRollerBallGame_SetFog();
	bdSetNearFar(1.0f, RB_I32(g, RB_GO_VARIATION) == 4 ? 140000.0f : 70000.0f);
	RB_U8(g, RB_GO_PAUSED) = 0;
	RB_U8(g, RB_GO_QUIT) = 0;
	CRollerBallHud_Begin(g + RB_GO_HUD);
	switch (RB_I32(g, RB_GO_VARIATION)) {
	case 0:
	case 1:
		volume = 0xff;
		sample = 7;
		break;
	case 2:
	case 3:
	case 4:
		volume = 0x7f;
		sample = 0xd;
		break;
	default:
		goto no_sound;
	}
	CRollerBallSfx_Play(g + RB_GO_SFX, sample, volume, 0, 0, 0);
no_sound:
	CRollerBallSim_Reset(g + RB_GO_SIM);
	CRollerBallTimer_Set(RB_GAME + RB_GO_TIMER, 0.5f);
	CRollerBallTimer_Start(RB_GAME + RB_GO_TIMER);
	RB_I32(RB_GAME, 0x13c8) = 1;
}

/* 0x001890e0  RollerBall (missing from the table; reached by rollerball_b's mode thunk
0x001986a6)
The per-frame update (RUN 0: nothing). Recolours the characters when the colour set
(DAT_00283ea4) changed; notes when exactly one human player is left (debug option
DAT_00283ef0); unless the pause menu holds the game: computer players, the clocks, then
the state update with the frame time (at most 2; x1.5 with DAT_00283e58). A pause sets
the paused flag; QUIT leaves: the variation's scores, then the mode is popped. */
void FUN_001890e0(void *self, int run)
{
	unsigned char *g = self;
	unsigned char *p, *sole;
	unsigned char *var;
	int count;
	float dt;

	if (run == 0)
		return;
	if (DAT_00283ea4 != RB_I32(g, 0x1468)) {
		p = RB_AT(g, RB_GO_PLAYERS, unsigned char *);
		RB_I32(g, 0x1468) = DAT_00283ea4;
		for (; p != NULL; p = RB_NEXT(p)) {
			Core_SetCharacterColours(RB_PTR(p, RB_PL_ACTOR), DAT_00283ea4);
			baPrepActorInstance(RB_PTR(p, RB_PL_ACTOR), 1);
		}
	}
	if (DAT_00283ef0 == 0) {
		RB_I32(RB_GAME, RB_GO_SOLE) = 0;
	} else {
		count = 0;
		sole = (unsigned char *)(uintptr_t)(unsigned int)run;
		for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
			if (RB_U8(p, RB_PL_AI) == 0) {
				count++;
				sole = p;
			}
		}
		if (count == 1) {
			RB_I32(RB_GAME, RB_GO_SOLE) = 1;
			RB_PTR(RB_GAME, RB_GO_SOLEPLAYER) = sole;
		} else {
			RB_I32(RB_GAME, RB_GO_SOLE) = 0;
		}
	}
	if (RB_I32(g, 0x13bc) == 0 && RB_I32(g, 0x13b8) == 0 &&
			CLPause_Update((undefined4 *)DAT_002ecd90) != 0)
		return;
	rbb_EntitiesSetAI(g + RB_GO_ENTITIES);
	if (RB_U8(g, RB_GO_PAUSED) != 0) {
		rbb_CameraApply(g + RB_GO_CAMERA, 1.0f);
		if (DAT_002ecdec == 0)
			RB_U8(g, RB_GO_PAUSED) = 0;
		return;
	}
	CRollerBallInfo_Update(g + RB_GO_INFO);
	CRollerBallSim_Update(g + RB_GO_SIM);
	dt = RB_F32(g, 0x268c);
	if (dt > 2.0f)
		dt = 2.0f;
	CRollerBallTimer_Update(g + RB_GO_COUNTDOWN);
	if (DAT_00283e58 != 0)
		dt = dt * 1.5f;
	switch (RB_I16(g, RB_GO_STATE)) {
	case 1:
		CRollerBallGame_UpdateIntro(g, dt);
		break;
	case 2:
		CRollerBallGame_UpdatePlay(g, dt);
		break;
	case 3:
	case 4:
		CRollerBallGame_UpdatePlay(g, dt);
		break;
	default:
		bkPrintf("*** ERROR : CRollerBall_Game::PollPreRender: Unhandled game state!");
	}
	if (DAT_002ecdec != 0)
		RB_U8(g, RB_GO_PAUSED) = 1;
	if (RB_U8(g, RB_GO_QUIT) != 0) {
		var = RB_PTR(g, RB_GO_VAROBJ);
		RB_VFN(var, 0x18, void (*)(void *))(var);
		CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, 1);
	}
}

/* 0x001892e0  RollerBall (missing from the table; reached by rollerball_b's mode thunk
0x001986b6)
The render (DRAW 0: nothing): entities, the variation scenery, shadows (render state 5
pushed, z bias 2), the variation's draw, the spot effects, the HUD, the goals, the
particles, the shader effects, the hud scores, the video screens and the camera; fog is
off while the overlays draw. */
void FUN_001892e0(void *self, int draw)
{
	unsigned char *g = self;
	unsigned char *var;
	void *shadows;

	if (draw == 0)
		return;
	rbb_EntitiesDraw(g + RB_GO_ENTITIES);
	shadows = g + RB_GO_SHADOWS;
	rbb_DrawScenery1(g + RB_GO_WORLD);
	bdPushRenderState(5, 0, 0);
	rbb_ShadowsDraw1(shadows);
	D3DDevice_SetRenderState_ZBias(2);
	rbb_DrawScenery2(g + RB_GO_WORLD);
	D3DDevice_SetRenderState_ZBias(0);
	rbb_ShadowsDraw2(shadows);
	bdPopRenderState(5);
	var = RB_PTR(g, RB_GO_VAROBJ);
	RB_VFN(var, 0x10, void (*)(void *))(var);
	CRollerBallSpotFX_Draw(g + RB_GO_SPOTFX);
	CDemoInfo_Draw(g + RB_GO_RESPAWN);
	if (RB_VS(int, RB_I32(RB_GAME, RB_GO_VARIATION), 0x002b4bb8u) != 0)
		bdSetRenderState(8, 0, 0);
	CRollerBallGoals_Draw(g + RB_GO_GOALS);
	CParticleSystemList_DrawAll((struct CParticleSystemList *)(g + RB_GO_PARTICLES));
	CRollerBall2850_Draw(g + RB_GO_2850);
	CRollerBallHud_Draw(g + RB_GO_HUD);
	CRollerBallVideo_Draw(g + RB_GO_VIDEO);
	rbb_CameraDraw(g + RB_GO_CAMERA);
	if (RB_VS(int, RB_I32(RB_GAME, RB_GO_VARIATION), 0x002b4bb8u) != 0)
		bdSetRenderState(8, 1, 0);
}
