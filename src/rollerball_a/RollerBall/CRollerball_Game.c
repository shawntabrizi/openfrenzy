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
	CRollerBallPhysics_Ctor(g + RB_GO_PHYSICS);
	CRollerBallInput_Ctor(g + RB_GO_INPUT);
	CRollerBallClock_Ctor(g + RB_GO_CLOCK);
	CRollerBallTimer_Ctor(g + RB_GO_COUNTDOWN);
	CRollerBallDebug_Ctor(g + RB_GO_DEBUG);
	CRollerBallDirect3D_Ctor(g + RB_GO_DIRECT3D);
	CRollerBallSfx_Ctor(g + RB_GO_SFX);
	CRollerBallVideo_Ctor(g + RB_GO_VIDEO);
	CRollerBall2740_Ctor(g + RB_GO_2740);
	CRollerBallShaders_Ctor(g + RB_GO_SHADERS);
	CRollerBallFloaty_Ctor(g + RB_GO_FLOATY);
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
	CRollerBallFloaty_Dtor(g + RB_GO_FLOATY);
	CRollerBallShaders_Dtor(g + RB_GO_SHADERS);
	CRollerBall2740_Dtor(g + RB_GO_2740);
	CRollerBallVideo_Dtor(g + RB_GO_VIDEO);
	CRollerBallSfx_Dtor(g + RB_GO_SFX);
	CRollerBallDirect3D_Dtor(g + RB_GO_DIRECT3D);
	CRollerBallDebug_Dtor(g + RB_GO_DEBUG);
	CRollerBallTimer_Dtor(g + RB_GO_COUNTDOWN);
	CRollerBallClock_Dtor(g + RB_GO_CLOCK);
	CRollerBallInput_Dtor(g + RB_GO_INPUT);
	CRollerBallPhysics_Dtor(g + RB_GO_PHYSICS);
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
	CRollerBallPhysics_Update(self + RB_GO_PHYSICS, dt);
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
	CRollerBallFloaty_Update(self + RB_GO_FLOATY, dt);
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
	CRollerBallInput_Shutdown(self + RB_GO_INPUT);
	CRollerBallVideo_Shutdown(self + RB_GO_VIDEO);
	CRollerBallShaders_Shutdown(self + RB_GO_SHADERS);
	CRollerBallFloaty_Shutdown(self + RB_GO_FLOATY);
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
	CRollerBallClock_Reset(g + RB_GO_CLOCK);
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
	CRollerBallInput_Update(g + RB_GO_INPUT);
	CRollerBallClock_Update(g + RB_GO_CLOCK);
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
	CRollerBallFloaty_Draw(g + RB_GO_FLOATY);
	CRollerBallHud_Draw(g + RB_GO_HUD);
	CRollerBallVideo_Draw(g + RB_GO_VIDEO);
	rbb_CameraDraw(g + RB_GO_CAMERA);
	if (RB_VS(int, RB_I32(RB_GAME, RB_GO_VARIATION), 0x002b4bb8u) != 0)
		bdSetRenderState(8, 1, 0);
}

/* ======================================================================== variation helpers */

/* a texture animation of the texture system (game +0x1ee0: entries of 0x1c bytes from
+0x1f00, count at +0x2018) for every player: the 8 frames of the player's colour from the
animated texture NAMES[i] (one name per player), 4 frames a second */
static void rb_AddPlayerTexAnims(const char *const *names)
{
	unsigned char *p, *e;
	struct bkResource *res;
	short idx;

	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), names++) {
		e = RB_GAME + 0x1f00 + RB_I32(RB_GAME, 0x2018) * 0x1c;
		res = bLoadResource(NULL, *names, 0, 0xdefa);
		RB_PTR(e, 4) = res;
		RB_U16(e, 0xc) = RB_U8(res, 0x37);
		idx = (short)RB_I32(p, RB_PL_INDEX);
		RB_I16(e, 8) = (short)(idx * 8);
		RB_I16(e, 0xa) = (short)(idx * 8 + 7);
		RB_I16(e, 0xe) = (short)(idx * 8);
		RB_I16(e, 0x10) = 1;
		RB_F32(e, 0x18) = 4.0f;
		RB_U8(e, 0x12) = 1;
		RB_F32(e, 0x14) = 0.0f;
		RB_I32(RB_GAME, 0x2018) = RB_I32(RB_GAME, 0x2018) + 1;
	}
}

/* a shadow (0x24 bytes, 256 x 256) for every player, cast by the actor fields +0x41dc and
+0x41d8, added to the shadow system (allocation tag LINE of CRollerball_Game.cpp) */
static void rb_AddPlayerShadows(int line)
{
	unsigned char *p;
	void *shadow;

	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		shadow = RB_NEW(0x24, RB_FILE_GAME, line);
		rbb_ShadowCtor(shadow, 1, 0x100, 0x100, p);
		rbb_ShadowAddCaster(shadow, p + 0x41dc);
		rbb_ShadowAddCaster(shadow, p + 0x41d8);
		rbb_ShadowsAdd(RB_GAME + RB_GO_SHADOWS, shadow);
	}
}

/* the world object (game +0x158: entries of 0x88 bytes, count at +0x6a8, name CRC at
+0x7c) whose name CRC is CRC, or NULL */
static unsigned char *rb_FindWorldObject(unsigned int crc)
{
	int i;

	for (i = 0; i < RB_I32(RB_GAME, 0x6a8); i++) {
		if (RB_U32(RB_GAME, 0x1d4 + i * 0x88) == crc)
			return RB_GAME + 0x158 + i * 0x88;
	}
	return NULL;
}

/* the leave check of every variation update (state 4): the game timer ran out */
static void rb_UpdateLeave(void)
{
	if (CRollerBallTimer_Remaining(RB_GAME + RB_GO_TIMER) <= 0.0f)
		RB_U8(RB_GAME, RB_GO_QUIT) = 1;
}

/* ======================================================================== CRollerBall_Base */

/* Variation "base" (vtable 0x00273a78): the goal game ("goalsign"). */

/* 0x00189400  RollerBall/CRollerball_Game.cpp: deleting destructor (flags bit 0: free) */
void *CRollerBallBase_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00273a78;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x00189420  RollerBall/CRollerball_Game.cpp
Initialise: the players' ramp texture animations (texanim\p<n>ramp_anim.gif, 0x002b4ce8),
the goal sign animation (all frames, 1 frame a second), a shadow per player, then Reset.
Returns 1. */
int CRollerBallBase_Init(void *self, void *package)
{
	unsigned char *e;
	struct bkResource *res;
	unsigned char frames;

	(void)package;
	if (RB_FIRST_PLAYER() != NULL)
		rb_AddPlayerTexAnims((const char *const *)RB_ADDR(0x002b4ce8u));
	e = RB_GAME + 0x1f00 + RB_I32(RB_GAME, 0x2018) * 0x1c;
	res = bLoadResource(NULL, "models\\alphatex\\goalsign_anim.gif", 0, 0xdefa);
	RB_PTR(e, 4) = res;
	frames = RB_U8(res, 0x37);
	RB_U16(e, 0xc) = frames;
	RB_I16(e, 8) = 0;
	RB_U16(e, 0xa) = (unsigned short)(frames - 1);
	RB_I16(e, 0xe) = 0;
	RB_I16(e, 0x10) = 1;
	RB_F32(e, 0x18) = 1.0f;
	RB_U8(e, 0x12) = 1;
	RB_F32(e, 0x14) = 0.0f;
	RB_I32(RB_GAME, 0x2018) = RB_I32(RB_GAME, 0x2018) + 1;
	rb_AddPlayerShadows(0x647);
	RB_VFN(self, 0x14, void (*)(void *))(self);
	return 1;
}

/* 0x001895d0  RollerBall/CRollerball_Game.cpp
Intro state (base): a player that is near the camera (1000) and faces it (within 120
degrees) gets message 3 and the sound 8 once (+0x1e). */
void CRollerBallBase_UpdateIntro(void *self)
{
	struct RB_Msg msg;
	unsigned char *p;
	float angle;

	(void)self;
	p = RB_FIRST_PLAYER();
	angle = atan2f(RB_F32(RB_GAME, 0x1374) - RB_F32(RB_GAME, 0x1384),
		RB_F32(RB_GAME, 0x137c) - RB_F32(RB_GAME, 0x138c));
	for (; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) == 0 || RB_U8(p, RB_PL_1E) != 0)
			continue;
		if (!(bmVectorDistance(RB_FP(p, RB_PL_POS), RB_FP(RB_GAME, 0x1374)) <= 1000.0f))
			continue;
		if (!(fabsf(CLMath_AngleDelta(RB_F32(p, RB_PL_HEADING), angle)) <= 2.09439516f))
			continue;
		rbb_MsgCtor(&msg);
		msg.id = 3;
		msg.prio = 2;
		msg.unique = 0;
		msg.b8 = 1;
		msg.b9 = 0;
		msg.value = 0.01f;
		msg.arg = 3;
		rbb_PostMsg(p, &msg);
		CRollerBallSfx_Play(RB_GAME + RB_GO_SFX, 8, 0x7f, 0, 0, NULL);
		RB_U8(p, RB_PL_1E) = 1;
		rbb_MsgDtor(&msg);
	}
}

/* 0x00189720  RollerBall/CRollerball_Game.cpp: deleting destructor (flags bit 0: free) */
void *CRollerBallSumo_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallSumo_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x00189740  RollerBall/CRollerball_Game.cpp
Initialise (sumo): the five platforms "platform<n>01" (radii and heights from the tables
0x002b4d08.., a platform object of 0xd0 bytes each), the variation timer, the ring
(radius 2030 at the height +0x40 = 310), the four arms "sumo_arm<n>01" and their
"retract" sub-objects, the ramp "ramp_platform01", the players' ramp animations, a shadow
per player, then Reset. Returns 1. */
int CRollerBallSumo_Init(void *self, void *package)
{
	unsigned char *s = self;
	unsigned int crc[5];
	char name[256];
	float pos[4];
	unsigned char *entry, **arm;
	void *obj;
	int i, j;

	(void)package;
	RB_F32(s, 0x40) = 310.0f;
	for (i = 0; i < 5; i++) {
		sprintf(name, "platform%d01", i + 1);
		crc[i] = bkCRCString(name);
	}
	for (i = 0; i < 5; i++) {
		unsigned char *pf = s + 0x4c + i * 0x18;

		RB_I32(pf, -4) = 0;
		RB_F32(pf, 0) = RB_VAR(float, 0x002b4d30u + i * 4);
		RB_F32(pf, 8) = RB_VAR(float, 0x002b4d08u + i * 4);
		RB_F32(pf, 4) = RB_VAR(float, 0x002b4d1cu + i * 4);
		entry = rb_FindWorldObject(crc[i]);
		if (entry != NULL)
			RB_PTR(pf, 0xc) = entry;
		obj = RB_NEW(0xd0, RB_FILE_GAME, 0x8fc);
		if (obj != NULL)
			obj = rbb_PlatformCtor(obj);
		RB_PTR(pf, 0x10) = obj;
		pos[0] = 0.0f;
		pos[1] = 600.0f;
		pos[2] = 0.0f;
		pos[3] = 1.0f;
		rbb_PlatformInit(obj, pos, RB_VAR(int, 0x002b4d58u + i * 4));
		rbb_PlatformSetTime(RB_PTR(pf, 0x10), RB_VAR(float, 0x002b4d44u + i * 4));
		rbb_PlatformSet48(RB_PTR(pf, 0x10), RB_VAR(float, 0x002b4d1cu + i * 4));
		rbb_PlatformSet74(RB_PTR(pf, 0x10), ~i & 1);
	}
	CDemoCountdown_Init((int)(uintptr_t)(s + 0x08));
	pos[0] = 0.0f;
	pos[1] = RB_F32(s, 0x40);
	pos[2] = 0.0f;
	rbb_RingInit(s + 0xc0, pos, 2030.0f, 0x60, 0x60);
	rbb_RingSet(s + 0xc0, RB_VAR(float, 0x002b4d08u) - 50.0f, 2030.0f);
	arm = (unsigned char **)(s + 0x264);
	for (i = 0; i < 4; i++, arm += 2) {
		sprintf(name, "sumo_arm%d01", i + 1);
		crc[i] = bkCRCString(name);
		for (j = 0; j < RB_I32(RB_GAME, 0xfd8); j++) {
			if (crc[i] == RB_U32(RB_GAME, 0x908 + j * 0x3c)) {
				arm[0] = RB_GAME + 0x8d0 + j * 0x3c;
				break;
			}
		}
		if (arm[0] != NULL)
			arm[1] = (unsigned char *)baFindSubObject(RB_PTR(arm[0], 0x30), "retract");
	}
	entry = rb_FindWorldObject(bkCRCString("ramp_platform01"));
	if (entry != NULL)
		RB_PTR(s, 0x284) = entry;
	if (RB_FIRST_PLAYER() != NULL) {
		rb_AddPlayerTexAnims((const char *const *)RB_ADDR(0x002b4cf8u));
		rb_AddPlayerShadows(0x97d);
	}
	RB_VFN(self, 0x14, void (*)(void *))(self);
	return 1;
}

/* 0x00189b40  RollerBall/CRollerball_Game.cpp
Play state (sumo): the round ends when fewer than two players are left, the countdown
ran out, or (team game) the two players left are partners. The players still in get the
rank "number of players left", the best rank wins (winner message 3, others 6), the
winners are placed on a circle (radius: value A - 200, at least 220 - 50) at height 725,
results state with 5 s per winner (7 s for one human winner), the platforms stop, the
scores are set and the HUD shows the winner text. */
void CRollerBallSumo_UpdatePlay(void *self)
{
	struct RB_Msg msg;
	unsigned char *first, *p, *last = NULL, *q;
	int done = 0, active = 0, winners, minrank, i;
	float step, angle, radius, pos[4];
	void **obj;

	rbb_MsgCtor(&msg);
	first = RB_FIRST_PLAYER();
	if (first == NULL) {
		done = 1;
	} else {
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			last = p;
			if (RB_U8(p, RB_PL_ACTIVE) != 0)
				active++;
		}
		if (active < 2)
			done = 1;
	}
	if (CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN) <= 0.0f)
		done = 1;
	if (DAT_002baebc != 0 && active == 2) {
		for (q = first; q != NULL && RB_U8(q, RB_PL_ACTIVE) == 0; q = RB_NEXT(q))
			;
		if (q != NULL)
			last = q;
		if ((RB_PTR(last, RB_PL_TEAMMATE) == NULL ||
				RB_U8(RB_PTR(last, RB_PL_TEAMMATE), RB_PL_ACTIVE) == 0) && !done)
			goto out;
	} else if (!done) {
		goto out;
	}
	rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 1);
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1A) = 0;
		RB_U8(p, RB_PL_1C) = 0;
	}
	if (DAT_002baebc != 0 && active == 2) {
		for (q = first; q != NULL && RB_U8(q, RB_PL_ACTIVE) == 0; q = RB_NEXT(q))
			;
		if (q != NULL)
			last = q;
		if (RB_PTR(last, RB_PL_TEAMMATE) != NULL &&
				RB_U8(RB_PTR(last, RB_PL_TEAMMATE), RB_PL_ACTIVE) == 0) {
			for (p = first; p != NULL; p = RB_NEXT(p)) {
				if (RB_U8(p, RB_PL_ACTIVE) == 0)
					RB_I32(p, RB_PL_RANK) = 2;
			}
		}
	}
	active = 0;
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) != 0)
			active++;
	}
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) != 0)
			RB_I32(p, RB_PL_RANK) = active;
	}
	winners = 0;
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_WINNER) != 0)
			winners++;
	}
	if (first == NULL || winners < 1) {
		winners = 0;
		minrank = 4;
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (RB_I32(p, RB_PL_RANK) < minrank)
				minrank = RB_I32(p, RB_PL_RANK);
		}
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (RB_I32(p, RB_PL_RANK) == minrank) {
				RB_U8(p, RB_PL_WINNER) = 1;
				winners++;
			}
		}
	}
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		msg.b9 = 1;
		msg.b8 = 1;
		msg.unique = 0;
		msg.prio = 2;
		msg.id = 3;
		msg.value = 0.5f;
		if (RB_U8(p, RB_PL_WINNER) == 0) {
			msg.arg = 6;
			rbb_PostMsg(p, &msg);
			RB_U8(p, RB_PL_1D) = 0;
		} else {
			msg.arg = 3;
			rbb_PostMsg(p, &msg);
			RB_U8(p, RB_PL_1D) = 1;
		}
	}
	angle = 0.0f;
	pos[1] = 725.0f;
	pos[3] = 0.0f; /* the original leaves this word of the stack unset */
	step = 6.28318548f / (float)winners;
	radius = RB_VFN(self, 0x24, float (*)(void *))(self) - 200.0f;
	if (radius < RB_VAR(float, 0x002b4d2cu))
		radius = RB_VAR(float, 0x002b4d2cu) - 50.0f;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_WINNER) == 0)
			continue;
		pos[0] = cosf(angle) * radius;
		pos[2] = sinf(angle) * radius;
		rbb_PlayerPlace(p, pos);
		RB_F32(p, RB_PL_VEL) = 0.0f;
		angle = angle - step;
		RB_F32(p, RB_PL_VEL + 4) = 0.0f;
		RB_F32(p, RB_PL_VEL + 8) = 0.0f;
		RB_U8(p, RB_PL_1B) = 0;
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
	}
	obj = (void **)((unsigned char *)self + 0x5c);
	for (i = 5; i != 0; i--, obj += 6)
		rbb_Platform_197580(*obj);
	RB_VFN(self, 0x18, void (*)(void *))(self);
	{
		int scores[4];

		scores[0] = DAT_002baee8;
		scores[1] = DAT_002baf10;
		scores[3] = DAT_002baf60;
		scores[2] = DAT_002baf38;
		CLHud_CreateWinnerText(DAT_002ecfac, scores, 0);
	}
out:
	rbb_MsgDtor(&msg);
}

/* a random time 600..1800 (the arms of the sumo ring) */
static float rb_RandomArmTime(void)
{
	return ((float)rb_rand() * RB_RAND_SCALE + 0.5f) * 1200.0f;
}

/* 0x00189f80  RollerBall/CRollerball_Game.cpp
Reset (sumo): players lose +0x1a, +0x1b; intro state; camera mode 1; the platforms are
reset; the arms' animations are flushed and rewound; four random arm times; the variation
timer is reset. */
void CRollerBallSumo_Reset(void *self)
{
	unsigned char *s = self;
	unsigned char *p, **arm;
	struct BActorInstance *inst;
	int i;

	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1A) = 0;
		RB_U8(p, RB_PL_1B) = 0;
	}
	RB_I16(RB_GAME, RB_GO_STATE) = 1;
	rbb_CameraSetMode(RB_GAME + RB_GO_CAMERA, 1);
	for (i = 0; i < 5; i++) {
		RB_I32(s, 0x48 + i * 0x18) = 0;
		rbb_PlatformReset(RB_PTR(s, 0x5c + i * 0x18));
	}
	arm = (unsigned char **)(s + 0x264);
	for (i = 4; i != 0; i--, arm += 2) {
		inst = RB_PTR(arm[0], 0x30);
		baFlushAnimations(inst, RB_PTR(inst, 0x134), 1, 0);
		inst = RB_PTR(arm[0], 0x30);
		baSetNodeTime(inst, RB_PTR(inst, 0x134), 0, 1);
	}
	RB_U8(s, 0x260) = 0;
	RB_F32(s, 0x288) = rb_RandomArmTime();
	RB_F32(s, 0x28c) = rb_RandomArmTime();
	RB_F32(s, 0x290) = rb_RandomArmTime();
	RB_F32(s, 0x294) = rb_RandomArmTime();
	CRollerBallTimer_Reset(s + 0x08);
}

/* ======================================================================== race paths */

/* an ASE shape read by rollerball_b (vtable 0x00274b4c) and the ASE text reader
(vtable 0x00274b8c) */
struct RB_AseShape {
	const void *vtbl;
	float *points; /* 4 floats per point */
	int count;
};
struct RB_AseReader {
	const void *vtbl;
	char *data;
	char *cur;
	char *end;
};

/* a 2D segment hit (bmSegmentSegment) */
struct RB_Hit2D {
	float v[4];
	int flag; /* +0x10 */
	int pad[3];
};

/* opens the ASE file NAME of PACKAGE in READER; 0 when it is missing */
static int rb_AseOpen(struct RB_AseReader *reader, void *package, const char *name)
{
	char path[1024];
	uint size;
	void *data;

	sprintf(path, "%s", name);
	data = bkLoadFile((struct bkPackage *)package, path, NULL, &size, NULL, 0);
	if (data == NULL)
		return 0;
	rbb_AseReaderOpen(reader, data, (int)size);
	return 1;
}

/* the path records (0x30 bytes, rollerball_b) behind a count word, built with the array
constructor iterator (0x001888d0 / 0x001888e0) */
static unsigned char *rb_NewPathRecords(int count, int line)
{
	int *block;
	unsigned char *records;
	int i;

	block = RB_NEW(count * 0x30 + 4, RB_FILE_GAME, line);
	if (block == NULL)
		return NULL;
	records = (unsigned char *)(block + 1);
	*block = count;
	for (i = 0; i < count; i++)
		CRollerBallPath_Construct(records + i * 0x30);
	return records;
}

/* deletes path records made by rb_NewPathRecords (array destructor iterator, then free):
first each record's point array (+0x24) */
static void rb_DeletePathRecords(unsigned char *records, int count)
{
	int i, n;

	for (i = 0; i < count; i++) {
		if (RB_PTR(records, i * 0x30 + 0x24) != NULL) {
			RB_DELETE(RB_PTR(records, i * 0x30 + 0x24));
			RB_PTR(records, i * 0x30 + 0x24) = NULL;
		}
	}
	n = RB_I32(records, -4);
	for (i = n - 1; i >= 0; i--)
		FUN_001888e0(records + i * 0x30);
	RB_DELETE(records - 4);
}

/* reads the AI lines of an ASE file: the "Start" line (its first two points are the 2D
start line: x0, z0, x1, z1 into START) and the lines "Line01".. into the path records
(each record's next line index is (i + 1) mod NPOINTS) */
static void rb_ReadAiLines(struct RB_AseReader *reader, struct RB_AseShape *shape,
	float **start_points, float *start, unsigned char *records, int nlines, int npoints)
{
	char name[1024];
	int i;

	CFire_TextReader_Rewind((struct CoTextReader *)reader);
	rbb_AseReadNamedShape(shape, reader, "Start");
	*start_points = shape->points;
	start[0] = shape->points[0];
	start[1] = shape->points[2];
	start[2] = shape->points[4];
	start[3] = shape->points[6];
	CFire_TextReader_Rewind((struct CoTextReader *)reader);
	for (i = 1; nlines > 0 && i <= nlines; i++) {
		sprintf(name, "Line%02d", i);
		rbb_AseReadNamedShape(shape, reader, name);
		rbb_PathSetPoints(records + (i - 1) * 0x30, shape->points, shape->count);
		RB_I32(records, (i - 1) * 0x30 + 0x2c) = i % npoints;
		CFire_TextReader_Rewind((struct CoTextReader *)reader);
	}
}

/* the distance along the race line POINTS (2D, x and z) up to each point, starting with
the segment from the last point; the last entry is 0. Returns the total. */
static float rb_LineDistances(const float *points, float *dist, int count)
{
	float total = 0.0f, dx, dz;
	int prev = count - 1, cur = 0, i;

	for (i = 0; i < count; i++) {
		dx = points[prev * 4] - points[cur * 4];
		dz = points[prev * 4 + 2] - points[cur * 4 + 2];
		total = sqrtf(dx * dx + dz * dz) + total;
		dist[i] = total;
		prev = (prev + 1) % count;
		cur = (cur + 1) % count;
	}
	dist[count - 1] = 0.0f;
	return total;
}

/* ======================================================================== CRollerBall_Race2 */

/* Variation "race2" (vtable 0x00273b08): one lap race on race2\paths.
+0x04 line points, +0x08 point count, +0x0c distances, +0x10 length, +0x14 start points,
+0x18 the start line (x0, z0, x1, z1), +0x28 AI line records, +0x2c record count,
+0x30 the players' last positions (16 bytes each). */

/* 0x0018a140  RollerBall/CRollerball_Game.cpp: deleting destructor (flags bit 0: free) */
void *CRollerBallRace2_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00273b08;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018a160  RollerBall/CRollerball_Game.cpp
Initialise (race2): the race line race2line.ase, the AI lines race2ai.ase, the line
distances, a shadow per player, then Reset. Returns 0 when a file is missing. */
int CRollerBallRace2_Init(void *self, void *package)
{
	unsigned char *s = self;
	struct RB_AseReader reader;
	struct RB_AseShape shape;
	unsigned char *records;
	float *start_points;
	int count;

	rbb_AseReaderCtor(&reader);
	rbb_AseShapesCtor(&shape);
	if (!rb_AseOpen(&reader, package, "race2\\paths\\race2line.ase")) {
		rbb_AseShapesDtor(&shape);
		rbb_AseReaderDtor(&reader);
		return 0;
	}
	rbb_AseReadShape(&shape, &reader);
	RB_PTR(s, 0x04) = shape.points;
	RB_I32(s, 0x08) = shape.count;
	RB_PTR(s, 0x0c) = RB_NEW(shape.count << 2, RB_FILE_GAME, 0xe47);
	count = RB_I32(s, 0x08);
	RB_I32(s, 0x2c) = count;
	records = rb_NewPathRecords(count, 0xe4c);
	RB_PTR(s, 0x28) = records;
	if (!rb_AseOpen(&reader, package, "race2\\paths\\race2ai.ase")) {
		rbb_AseShapesDtor(&shape);
		rbb_AseReaderDtor(&reader);
		return 0;
	}
	rb_ReadAiLines(&reader, &shape, &start_points, RB_FP(s, 0x18), RB_PTR(s, 0x28),
		RB_I32(s, 0x2c), RB_I32(s, 0x08));
	RB_PTR(s, 0x14) = start_points;
	RB_F32(s, 0x10) = 0.0f;
	if (RB_I32(s, 0x08) > 0)
		RB_F32(s, 0x10) = rb_LineDistances(RB_PTR(s, 0x04), RB_PTR(s, 0x0c), RB_I32(s, 0x08));
	else
		((float *)RB_PTR(s, 0x0c))[RB_I32(s, 0x08) - 1] = 0.0f;
	rb_AddPlayerShadows(0xe91);
	RB_VFN(self, 0x14, void (*)(void *))(self);
	rbb_AseShapesDtor(&shape);
	rbb_AseReaderDtor(&reader);
	return 1;
}

/* 0x0018a4f0  RollerBall/CRollerball_Game.cpp
Shutdown (race2): the AI line records and their points, the distances, the race line, the
start points. */
void CRollerBallRace2_Shutdown(void *self)
{
	unsigned char *s = self;

	if (RB_PTR(s, 0x28) != NULL) {
		rb_DeletePathRecords(RB_PTR(s, 0x28), RB_I32(s, 0x2c));
		RB_PTR(s, 0x28) = NULL;
	}
	if (RB_PTR(s, 0x0c) != NULL) {
		RB_DELETE(RB_PTR(s, 0x0c));
		RB_PTR(s, 0x0c) = NULL;
	}
	if (RB_PTR(s, 0x04) != NULL) {
		RB_DELETE(RB_PTR(s, 0x04));
		RB_PTR(s, 0x04) = NULL;
	}
	if (RB_PTR(s, 0x14) != NULL) {
		RB_DELETE(RB_PTR(s, 0x14));
		RB_PTR(s, 0x14) = NULL;
	}
}

/* the crossing of the start line by a player's last move (+0x40e0 -> +0x40d0, x and z):
1 forward (laps +1, unless the backward count +0x41f4 has to be paid back first),
-1 backward (+0x41f4 - 1), 0 no crossing */
static int rb_CrossStart(unsigned char *p, float *line)
{
	struct RB_Hit2D hit;
	float seg[4];

	hit.flag = 0;
	seg[0] = RB_F32(p, 0x40e0);
	seg[1] = RB_F32(p, 0x40e8);
	seg[2] = RB_F32(p, RB_PL_POS);
	seg[3] = RB_F32(p, RB_PL_POS + 8);
	if (bmSegmentSegment(line, seg, (struct BHit2D *)&hit) == 0)
		return 0;
	if (rbb_SideOfLine(seg, line) >= 0.0f) {
		RB_I32(p, RB_PL_41F4) = RB_I32(p, RB_PL_41F4) - 1;
	} else if (RB_I32(p, RB_PL_41F4) != 0) {
		RB_I32(p, RB_PL_41F4) = RB_I32(p, RB_PL_41F4) + 1;
	} else {
		RB_I32(p, RB_PL_41F0) = RB_I32(p, RB_PL_41F0) + 1;
	}
	return 1;
}

/* 0x0018a5a0  RollerBall/CRollerball_Game.cpp
Play state (race2): players cross the start line (one lap); a player that finished gets
its place. The round ends when three players finished or the countdown ran out: finished
players are winners by place (first: message 3, others 6); unfinished players lose. */
void CRollerBallRace2_UpdatePlay(void *self)
{
	unsigned char *s = self;
	struct RB_Msg msg;
	unsigned char *first, *p, *q, *last = NULL;
	int *save;
	int finished, winners, n;

	rbb_MsgCtor(&msg);
	first = RB_FIRST_PLAYER();
	save = (int *)(s + 0x38);
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) == 0 || RB_I32(p, RB_PL_41F0) >= 1)
			continue;
		last = p;
		if (rb_CrossStart(p, RB_FP(s, 0x18))) {
			RB_I32(p, RB_PL_SCORE) = RB_I32(p, RB_PL_41F0);
			if (RB_I32(p, RB_PL_41F0) >= 1) {
				n = 0;
				for (q = first; q != NULL; q = RB_NEXT(q)) {
					if (q != p && RB_I32(q, RB_PL_41F0) >= 1)
						n++;
				}
				RB_I32(p, RB_PL_RANK) = n;
				continue;
			}
		}
		if (RB_U8(p, RB_PL_18) != 0) {
			save[-2] = RB_I32(p, RB_PL_POS);
			save[-1] = RB_I32(p, RB_PL_POS + 4);
			save[0] = RB_I32(p, RB_PL_POS + 8);
			save[1] = RB_I32(p, RB_PL_POS + 12);
		}
		save += 4;
	}
	finished = 0;
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		last = p;
		if (RB_I32(p, RB_PL_41F0) >= 1)
			finished++;
	}
	if (finished < 3 && CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN) > 0.0f)
		goto out;
	CLSamples_PlayGeneric(0, 0xff, 0, 0, NULL);
	rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 1);
	if (finished != 0) {
		winners = 0;
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			RB_U8(p, RB_PL_1A) = 0;
			if (RB_I32(p, RB_PL_41F0) < 1) {
				RB_U8(p, RB_PL_WINNER) = 0;
				if (RB_U8(p, RB_PL_AI) != 0)
					RB_U8(p, RB_PL_1C) = 0;
				else
					RB_U8(p, RB_PL_1A) = 0;
			} else if (RB_I32(p, RB_PL_RANK) >= 1) {
				RB_U8(p, RB_PL_WINNER) = 0;
				msg.b8 = 1;
				msg.b9 = 1;
				msg.id = 3;
				msg.prio = 2;
				msg.unique = 0;
				msg.value = 0.5f;
				msg.arg = 6;
				rbb_PostMsg(p, &msg);
			} else {
				msg.id = 3;
				msg.prio = 2;
				msg.unique = 0;
				msg.b8 = 1;
				msg.b9 = 1;
				msg.value = 0.5f;
				msg.arg = 3;
				rbb_PostMsg(p, &msg);
				winners++;
			}
		}
	} else {
		/* the original changes the last player of the list on every pass, and counts the
		winners as the address of its position save pointer (the result time becomes
		huge, so the results wait for a button) */
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			RB_U8(last, RB_PL_WINNER) = 0;
			if (RB_U8(last, RB_PL_AI) != 0)
				RB_U8(last, RB_PL_1C) = 0;
			else
				RB_U8(last, RB_PL_1A) = 0;
		}
		winners = (int)(uintptr_t)save;
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
	}
	RB_VFN(self, 0x18, void (*)(void *))(self);
	{
		int scores[4];

		scores[0] = DAT_002baee8;
		scores[1] = DAT_002baf10;
		scores[3] = DAT_002baf60;
		scores[2] = DAT_002baf38;
		CLHud_CreateWinnerText(DAT_002ecfac, scores, 0);
	}
out:
	rbb_MsgDtor(&msg);
}

/* 0x0018a990  RollerBall/CRollerball_Game.cpp
Reset (race2): players lose +0x1a, +0x1b, the last player's position is kept at +0x30;
intro state; camera mode 1. */
void CRollerBallRace2_Reset(void *self)
{
	unsigned char *p;

	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1A) = 0;
		RB_U8(p, RB_PL_1B) = 0;
		RB_I32(self, 0x30) = RB_I32(p, RB_PL_POS);
		RB_I32(self, 0x34) = RB_I32(p, RB_PL_POS + 4);
		RB_I32(self, 0x38) = RB_I32(p, RB_PL_POS + 8);
		RB_I32(self, 0x3c) = RB_I32(p, RB_PL_POS + 12);
	}
	RB_I16(RB_GAME, RB_GO_STATE) = 1;
	rbb_CameraSetMode(RB_GAME + RB_GO_CAMERA, 1);
}

/* ======================================================================== CRollerBall_Race */

/* Variation "race" (vtable 0x00273b50): three lap race on race\paths with four race lines.
+0x04..+0x10 the four race lines' points, +0x14 point count, +0x18 distances, +0x1c
length, +0x20 start points, +0x24 the start line (x0, z0, x1, z1), +0x34 AI line records,
+0x38 record count, +0x3c the players' last positions (16 bytes each), +0x7c the video
screen, +0x80 and +0x94 two finish emitters (position, particle system at +0x90 / +0xa4),
+0xa8 a counter. */

/* 0x0018aa00  RollerBall/CRollerball_Game.cpp: deleting destructor (flags bit 0: free) */
void *CRollerBallRace_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00273b50;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018aa20  RollerBall/CRollerball_Game.cpp
Initialise (race): the race lines raceline1..4.ase, the AI lines raceai.ase, the line
distances, the video screen race\tex\zscreen.bmp, the two finish emitters (positions
0x002b4d6c..), a shadow per player, the players' laps (-1), the active goal animation,
then Reset. Returns 0 when a file is missing. */
int CRollerBallRace_Init(void *self, void *package)
{
	unsigned char *s = self;
	struct RB_AseReader reader;
	struct RB_AseShape shape;
	char name[1024];
	float *start_points;
	unsigned char *p, *e, *ps;
	void *screen;
	struct bkResource *res;
	unsigned char frames;
	int i, count;

	rbb_AseReaderCtor(&reader);
	rbb_AseShapesCtor(&shape);
	for (i = 1; i <= 4; i++) {
		sprintf(name, "race\\paths\\raceline%d.ase", i);
		if (!rb_AseOpen(&reader, package, name))
			goto fail;
		rbb_AseReadShape(&shape, &reader);
		RB_PTR(s, i * 4) = shape.points;
	}
	RB_I32(s, 0x14) = shape.count;
	RB_PTR(s, 0x18) = RB_NEW(shape.count << 2, RB_FILE_GAME, 0x11b9);
	count = RB_I32(s, 0x14);
	RB_I32(s, 0x38) = count;
	RB_PTR(s, 0x34) = rb_NewPathRecords(count, 0x11be);
	if (!rb_AseOpen(&reader, package, "race\\paths\\raceai.ase"))
		goto fail;
	rb_ReadAiLines(&reader, &shape, &start_points, RB_FP(s, 0x24), RB_PTR(s, 0x34),
		RB_I32(s, 0x38), RB_I32(s, 0x14));
	RB_PTR(s, 0x20) = start_points;
	RB_F32(s, 0x1c) = 0.0f;
	if (RB_I32(s, 0x14) > 0)
		RB_F32(s, 0x1c) = rb_LineDistances(RB_PTR(s, 0x04), RB_PTR(s, 0x18), RB_I32(s, 0x14));
	else
		((float *)RB_PTR(s, 0x18))[RB_I32(s, 0x14) - 1] = 0.0f;
	screen = RB_NEW(0x58, RB_FILE_GAME, 0x11fc);
	if (screen != NULL)
		CRollerBallScreen_Ctor(screen);
	RB_PTR(s, 0x7c) = screen;
	CRollerBallScreen_Init(screen, "race\\tex\\zscreen.bmp", 0x100, 0x100);
	CRollerBallList_Add(RB_GAME + RB_GO_VIDEO, RB_PTR(s, 0x7c));
	for (i = 0; i < 2; i++) {
		e = s + 0x80 + i * 0x14;
		RB_U32(e, 0x0) = RB_VAR(unsigned int, 0x002b4d6cu + i * 0x10);
		RB_U32(e, 0x4) = RB_VAR(unsigned int, 0x002b4d70u + i * 0x10);
		RB_U32(e, 0x8) = RB_VAR(unsigned int, 0x002b4d74u + i * 0x10);
		RB_U32(e, 0xc) = RB_VAR(unsigned int, 0x002b4d78u + i * 0x10);
		ps = RB_NEW(0xa8, RB_FILE_GAME, 0x1209);
		if (ps != NULL)
			CParticleSystem_Construct((struct CParticleSystem *)ps);
		RB_PTR(e, 0x10) = ps;
		CParticleSystem_Init((struct CParticleSystem *)ps, NULL, 800);
		ps = RB_PTR(e, 0x10);
		RB_F32(ps, 0x54) = rb_bitsf(0x3e20d97cu);
		RB_F32(ps, 0x6c) = 0.5f;
		RB_F32(ps, 0x58) = 0.3f;
		RB_F32(ps, 0x60) = 15.0f;
		RB_F32(ps, 0x70) = 30.0f;
		CParticleSystem_SetTexture((struct CPSHeader *)RB_PTR(e, 0x10), RB_I32(RB_GAME, 0x1eec));
		CParticleSystem_SetTexture2((struct CParticleSystem *)RB_PTR(e, 0x10),
			RB_I32(RB_GAME, 0x1ee8));
		CParticleSystem_SetPosition((struct CPSHeader *)RB_PTR(e, 0x10), (float *)e);
		CRollerBallList_Add(RB_GAME + RB_GO_PARTICLES, RB_PTR(e, 0x10));
	}
	rb_AddPlayerShadows(0x1225);
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p))
		RB_I32(p, RB_PL_41F0) = -1;
	e = RB_GAME + 0x1f00 + RB_I32(RB_GAME, 0x2018) * 0x1c;
	res = bLoadResource(NULL, "race\\texanim\\activegoalanim.gif", 0, 0xdefa);
	RB_PTR(e, 4) = res;
	frames = RB_U8(res, 0x37);
	RB_U16(e, 0xc) = frames;
	RB_I16(e, 8) = 0;
	RB_U16(e, 0xa) = (unsigned short)(frames - 1);
	RB_I16(e, 0xe) = 0;
	RB_I16(e, 0x10) = 1;
	RB_F32(e, 0x18) = 3.0f;
	RB_U8(e, 0x12) = 1;
	RB_F32(e, 0x14) = 0.0f;
	RB_I32(RB_GAME, 0x2018) = RB_I32(RB_GAME, 0x2018) + 1;
	RB_VFN(self, 0x14, void (*)(void *))(self);
	rbb_AseShapesDtor(&shape);
	rbb_AseReaderDtor(&reader);
	return 1;
fail:
	rbb_AseShapesDtor(&shape);
	rbb_AseReaderDtor(&reader);
	return 0;
}

/* 0x0018af90  RollerBall/CRollerball_Game.cpp
Shutdown (race): the AI line records and their points, the distances, the four race
lines, the start points. */
void CRollerBallRace_Shutdown(void *self)
{
	unsigned char *s = self;
	int i;

	if (RB_PTR(s, 0x34) != NULL) {
		rb_DeletePathRecords(RB_PTR(s, 0x34), RB_I32(s, 0x38));
		RB_PTR(s, 0x34) = NULL;
	}
	if (RB_PTR(s, 0x18) != NULL) {
		RB_DELETE(RB_PTR(s, 0x18));
		RB_PTR(s, 0x18) = NULL;
	}
	for (i = 1; i <= 4; i++) {
		if (RB_PTR(s, i * 4) != NULL) {
			RB_DELETE(RB_PTR(s, i * 4));
			RB_PTR(s, i * 4) = NULL;
		}
	}
	if (RB_PTR(s, 0x20) != NULL) {
		RB_DELETE(RB_PTR(s, 0x20));
		RB_PTR(s, 0x20) = NULL;
	}
}

/* the catch-up of the computer players: the leader (ORDER[0]) gets the speed and
acceleration of the variation (0x002b5134 / 0x002b5138, records of 0x28 bytes); the others
get more by how far they are behind (progress difference x K, at most 1) */
static void rb_RaceCatchUp(unsigned char **order, float k)
{
	int v = RB_I32(RB_GAME, RB_GO_VARIATION);
	float lead = RB_F32(order[0], 0x41ec);
	float speed = RB_VAR(float, 0x002b5134u + v * 0x28);
	float accel = RB_VAR(float, 0x002b5138u + v * 0x28);
	float f, t;
	int i;

	RB_F32(order[0], 0x416c) = speed;
	RB_F32(order[0], 0x4170) = accel;
	for (i = 1; i < 4; i++) {
		t = (lead - RB_F32(order[i], 0x41ec)) * k;
		f = 0.0f;
		if (t >= 0.0f) {
			f = t;
			if (t > 1.0f)
				f = 1.0f;
		}
		v = RB_I32(RB_GAME, RB_GO_VARIATION);
		RB_F32(order[i], 0x416c) = speed * f + RB_VAR(float, 0x002b5134u + v * 0x28);
		RB_F32(order[i], 0x4170) = f * accel + RB_VAR(float, 0x002b5138u + v * 0x28);
	}
}

/* 0x0018b050  RollerBall/CRollerball_Game.cpp
Play state (race): players cross the start line (laps; the score is laps + 1, 0..3); a
player that finished 3 laps gets its place, and the first one fires the finish emitters in
its colour (+0x41c0). The computer players catch up by difficulty (game +0x20: 1 or 2).
The round ends when three players finished or the countdown ran out: by place (or, when
nobody finished, by progress +0x41ec) the winners get message 3, the others 6; the players
go back to their start points (game +0x6ac). */
void CRollerBallRace_UpdatePlay(void *self)
{
	unsigned char *s = self;
	struct RB_Msg msg;
	unsigned char *first, *p, *q, *chosen;
	unsigned char *order[4];
	unsigned char taken[4];
	int *save;
	int finished, winners, n, k, idx, chosen_idx;
	float best;

	rbb_MsgCtor(&msg);
	first = RB_FIRST_PLAYER();
	save = (int *)(s + 0x44);
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) == 0 || RB_I32(p, RB_PL_41F0) >= 3)
			continue;
		if (rb_CrossStart(p, RB_FP(s, 0x24))) {
			n = RB_I32(p, RB_PL_41F0) + 1;
			RB_I32(p, RB_PL_SCORE) = n;
			if (n < 0)
				RB_I32(p, RB_PL_SCORE) = 0;
			if (RB_I32(p, RB_PL_SCORE) > 3)
				RB_I32(p, RB_PL_SCORE) = 3;
			if (RB_I32(p, RB_PL_41F0) > 2) {
				n = 0;
				for (q = first; q != NULL; q = RB_NEXT(q)) {
					if (q != p && RB_I32(q, RB_PL_41F0) > 2)
						n++;
				}
				RB_U8(p, RB_PL_1C) = 0;
				RB_U8(p, RB_PL_1A) = 0;
				RB_I32(p, RB_PL_RANK) = n;
				if (n == 0) {
					for (k = 0; k < 2; k++) {
						CParticleSystem_Start(RB_PTR(s, 0x90 + k * 0x14),
							(short)RB_I32(p, 0x41c0), (short)RB_I32(p, 0x41c4),
							(short)RB_I32(p, 0x41c8));
						RB_U8(RB_PTR(s, 0x90 + k * 0x14), 0x50) = 1;
					}
				}
				continue;
			}
		}
		if (RB_U8(p, RB_PL_18) != 0) {
			save[-2] = RB_I32(p, RB_PL_POS);
			save[-1] = RB_I32(p, RB_PL_POS + 4);
			save[0] = RB_I32(p, RB_PL_POS + 8);
			save[1] = RB_I32(p, RB_PL_POS + 12);
		}
		save += 4;
	}
	/* the players by progress (+0x41ec), best first. The original keeps the previous
	choice when no player is left (fewer than four players); the port does the same and
	starts with the first player instead of the variation object. */
	memset(taken, 0, sizeof taken);
	chosen = first;
	chosen_idx = 0;
	for (k = 0; k < 4; k++) {
		best = -99999.0f;
		idx = 0;
		for (q = first; q != NULL; q = RB_NEXT(q), idx++) {
			if (taken[idx & 3] == 0 && best < RB_F32(q, 0x41ec)) {
				best = RB_F32(q, 0x41ec);
				chosen = q;
				chosen_idx = idx;
			}
		}
		order[k] = chosen;
		taken[chosen_idx & 3] = 1;
	}
	switch (RB_I32(RB_GAME, 0x20)) {
	case 0:
		break;
	case 1:
		rb_RaceCatchUp(order, 6.0f);
		break;
	case 2:
		rb_RaceCatchUp(order, 24.0f);
		break;
	default:
		bkPrintf("ERROR: Unhandled difficulty\n");
		break;
	}
	finished = 0;
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_I32(p, RB_PL_41F0) > 2)
			finished++;
	}
	if (finished < 3 && CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN) > 0.0f)
		goto out;
	CLSamples_PlayGeneric(0, 0xff, 0, 0, NULL);
	rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 1);
	if (finished == 0) {
		best = 0.0f;
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (best < RB_F32(p, 0x41ec))
				best = RB_F32(p, 0x41ec);
		}
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			RB_U8(p, RB_PL_1A) = 0;
			RB_U8(p, RB_PL_1C) = 0;
			RB_U8(p, RB_PL_WINNER) = best <= RB_F32(p, 0x41ec) ? 1 : 0;
		}
	} else {
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			RB_U8(p, RB_PL_1A) = 0;
			RB_U8(p, RB_PL_1C) = 0;
			if (RB_I32(p, RB_PL_41F0) < 3)
				RB_U8(p, RB_PL_WINNER) = 0;
			else
				RB_U8(p, RB_PL_WINNER) = RB_I32(p, RB_PL_RANK) < 1;
		}
	}
	winners = 0;
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		msg.b9 = 1;
		msg.b8 = 1;
		msg.unique = 0;
		msg.prio = 2;
		msg.value = 0.5f;
		if (RB_U8(p, RB_PL_WINNER) == 0) {
			msg.id = 3;
			msg.arg = 6;
			rbb_PostMsg(p, &msg);
		} else {
			msg.id = 3;
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
		rbb_PlayerPlace(p, (const float *)(RB_GAME + 0x6ac + RB_I32(p, RB_PL_SLOT) * 0x20));
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
out:
	rbb_MsgDtor(&msg);
}

/* 0x0018b8b0  RollerBall/CRollerball_Game.cpp
Reset (race): players lose +0x1a, +0x1b, the last player's position is kept at +0x3c;
intro state; camera mode 1; the finish emitters reset (virtual +0xc); the players' laps
-1 and headings -pi/4; the counter +0xa8 is cleared. */
void CRollerBallRace_Reset(void *self)
{
	unsigned char *s = self;
	unsigned char *p;
	void *ps;
	int i;

	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1A) = 0;
		RB_U8(p, RB_PL_1B) = 0;
		RB_I32(s, 0x3c) = RB_I32(p, RB_PL_POS);
		RB_I32(s, 0x40) = RB_I32(p, RB_PL_POS + 4);
		RB_I32(s, 0x44) = RB_I32(p, RB_PL_POS + 8);
		RB_I32(s, 0x48) = RB_I32(p, RB_PL_POS + 12);
	}
	RB_I16(RB_GAME, RB_GO_STATE) = 1;
	rbb_CameraSetMode(RB_GAME + RB_GO_CAMERA, 1);
	for (i = 0; i < 2; i++) {
		ps = RB_PTR(s, 0x90 + i * 0x14);
		RB_VFN(ps, 0x0c, void (*)(void *))(ps);
	}
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		RB_I32(p, RB_PL_41F0) = -1;
		RB_F32(p, 0x41a4) = -0.785398185f;
		RB_F32(p, RB_PL_HEADING) = -0.785398185f;
	}
	RB_I32(s, 0xa8) = 0;
}

/* 0x0018b8c0  RollerBall/CRollerball_Game.cpp
Not a function: Ghidra's entry in the middle of 0x0018b8b0 (the player loop, entered with
the list in EAX and 0 in EBX). Nothing calls it; its code is in CRollerBallRace_Reset. */
void CRollerBallRace_ResetChunk(void *self)
{
	(void)self;
}

/* ======================================================================== CRollerBall_Drop */

/* 0x0018b990  RollerBall/CRollerball_Game.cpp: deleting destructor (flags bit 0: free) */
void *CRollerBallDrop_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallDrop_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018b9b0  RollerBall/CRollerball_Game.cpp
Initialise (drop): +0x40 = 450; the five platforms "platform<n>01" (values from the
tables 0x002b4d8c.., a platform object of 0xd0 bytes each), the variation timer, a shadow
per player, the booster animation, then Reset. Returns 1. */
int CRollerBallDrop_Init(void *self, void *package)
{
	unsigned char *s = self;
	unsigned int crc[5];
	char name[256];
	float pos[4];
	unsigned char *pf, *entry, *e;
	struct bkResource *res;
	unsigned char frames;
	void *obj;
	int i;

	(void)package;
	RB_F32(s, 0x40) = 450.0f;
	for (i = 0; i < 5; i++) {
		sprintf(name, "platform%d01", i + 1);
		crc[i] = bkCRCString(name);
	}
	for (i = 0; i < 5; i++) {
		pf = s + 0x48 + i * 0x58;
		RB_I32(pf, -4) = 0;
		RB_F32(pf, 0) = RB_VAR(float, 0x002b4d8cu + i * 4);
		RB_F32(pf, 8) = RB_VAR(float, 0x002b4da0u + i * 4);
		RB_F32(pf, 4) = RB_VAR(float, 0x002b4db4u + i * 4);
		entry = rb_FindWorldObject(crc[i]);
		if (entry != NULL)
			RB_PTR(pf, 0xc) = entry;
		obj = RB_NEW(0xd0, RB_FILE_GAME, 0x170e);
		if (obj != NULL)
			obj = rbb_PlatformCtor(obj);
		RB_PTR(pf, 0x10) = obj;
		pos[0] = 0.0f;
		pos[1] = 600.0f;
		pos[2] = 0.0f;
		pos[3] = 0.0f; /* the original leaves this word of the stack unset */
		rbb_PlatformInit(obj, pos, RB_VAR(int, 0x002b4ddcu + i * 4));
		rbb_PlatformSetTime(RB_PTR(pf, 0x10), RB_VAR(float, 0x002b4dc8u + i * 4));
		rbb_PlatformSet48(RB_PTR(pf, 0x10), RB_VAR(float, 0x002b4db4u + i * 4));
		rbb_PlatformSet74(RB_PTR(pf, 0x10), ~i & 1);
	}
	CDemoCountdown_Init((int)(uintptr_t)(s + 0x08));
	rb_AddPlayerShadows(0x1728);
	e = RB_GAME + 0x1f00 + RB_I32(RB_GAME, 0x2018) * 0x1c;
	res = bLoadResource(NULL, "drop\\alphatex\\booster.gif", 0, 0xdefa);
	RB_PTR(e, 4) = res;
	frames = RB_U8(res, 0x37);
	RB_U16(e, 0xa) = (unsigned short)(frames - 1);
	RB_U16(e, 0xc) = frames;
	RB_I16(e, 8) = 0;
	RB_I16(e, 0xe) = 0;
	RB_I16(e, 0x10) = 1;
	RB_F32(e, 0x18) = 4.0f;
	RB_U8(e, 0x12) = 1;
	RB_F32(e, 0x14) = 0.0f;
	RB_I32(RB_GAME, 0x2018) = RB_I32(RB_GAME, 0x2018) + 1;
	RB_VFN(self, 0x14, void (*)(void *))(self);
	return 1;
}

/* 0x0018bc40  RollerBall/CRollerball_Game.cpp
Play state (drop): when at most one player is left the round ends: the players left get
the rank "number left" (+0x41f0), winners (+0x17) get message 3, the others 6; the winners
are placed on a circle (radius: value A (+0x38) - 200, at least 285 - 50) at height 725;
results state; the platforms stop; scores and winner text. */
void CRollerBallDrop_UpdatePlay(void *self)
{
	struct RB_Msg msg;
	unsigned char *first, *p;
	int active = 0, winners, i;
	float step, angle, radius, pos[4];
	void **obj;

	rbb_MsgCtor(&msg);
	first = RB_FIRST_PLAYER();
	if (first != NULL) {
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (RB_U8(p, RB_PL_ACTIVE) != 0)
				active++;
		}
		if (active > 1)
			goto out;
	}
	rbb_EntitiesFreeze(RB_GAME + RB_GO_ENTITIES, 1);
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		RB_U8(p, RB_PL_1A) = 0;
		RB_U8(p, RB_PL_1C) = 0;
	}
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) != 0)
			RB_I32(p, RB_PL_41F0) = active;
	}
	winners = 0;
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_WINNER) != 0)
			winners++;
	}
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		msg.b9 = 1;
		msg.b8 = 1;
		msg.unique = 0;
		msg.prio = 2;
		msg.id = 3;
		msg.arg = RB_U8(p, RB_PL_WINNER) == 0 ? 6 : 3;
		msg.value = 0.5f;
		rbb_PostMsg(p, &msg);
	}
	angle = 0.0f;
	pos[1] = 725.0f;
	pos[3] = 0.0f; /* the original leaves this word of the stack unset */
	step = 6.28318548f / (float)winners;
	radius = RB_VFN(self, 0x38, float (*)(void *))(self) - 200.0f;
	if (radius < RB_VAR(float, 0x002b4dc4u))
		radius = RB_VAR(float, 0x002b4dc4u) - 50.0f;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_WINNER) == 0)
			continue;
		pos[0] = cosf(angle) * radius;
		pos[2] = sinf(angle) * radius;
		rbb_PlayerPlace(p, pos);
		RB_F32(p, RB_PL_VEL) = 0.0f;
		angle = angle - step;
		RB_F32(p, RB_PL_VEL + 4) = 0.0f;
		RB_F32(p, RB_PL_VEL + 8) = 0.0f;
		RB_U8(p, RB_PL_1B) = 0;
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
	}
	obj = (void **)((unsigned char *)self + 0x58);
	for (i = 5; i != 0; i--, obj += 0x16)
		rbb_Platform_197580(*obj);
	RB_VFN(self, 0x18, void (*)(void *))(self);
	{
		int scores[4];

		scores[0] = DAT_002baee8;
		scores[1] = DAT_002baf10;
		scores[3] = DAT_002baf60;
		scores[2] = DAT_002baf38;
		CLHud_CreateWinnerText(DAT_002ecfac, scores, 0);
	}
out:
	rbb_MsgDtor(&msg);
}

/* the drop platforms' random times 400..1200 */
static float rb_RandomDropTime(void)
{
	return ((float)rb_rand() * RB_RAND_SCALE + 0.5f) * 800.0f;
}

/* 0x0018bf40  RollerBall/CRollerball_Game.cpp
Reset (drop): the score table is ranked; players out of the results (DAT_002b9c48) take
no part; the players in (+0x15) are placed on a circle of radius 600 at height 725 facing
the middle (start angle by player count: 2 pi, 3 210 degrees, 4 225 degrees); intro
state; camera mode 1; the platforms reset; four random times; the variation timer is
reset. Fewer than two players: debug break. */
void CRollerBallDrop_Reset(void *self)
{
	unsigned char *s = self;
	unsigned char *p;
	int *out;
	int n, i;
	float angle, step, heading, pos[4], axis[4];

	CScoreTable_Rank((struct CScoreTable *)DAT_002b9968);
	out = DAT_002b9c48;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), out++) {
		RB_U8(p, RB_PL_1B) = 0;
		RB_U8(p, RB_PL_1A) = 0;
		if (*out != 0) {
			RB_U8(p, RB_PL_1C) = 0;
			RB_U8(p, RB_PL_15) = 0;
			RB_U8(p, RB_PL_AI) = 1;
			RB_U8(p, RB_PL_ACTIVE) = 0;
			RB_U8(p, RB_PL_WINNER) = 0;
			RB_U8(p, 0x16) = 0;
		}
	}
	n = 0;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_15) != 0)
			n++;
	}
	if (n <= 1)
		DbgBreakPoint();
	step = 6.28318548f / (float)n;
	switch (n) {
	case 2:
		angle = 3.14159274f;
		break;
	case 3:
		angle = rb_bitsf(0x406a927fu);
		break;
	case 4:
		angle = rb_bitsf(0x407b53d1u);
		break;
	default:
		/* the original reuses the count's bits as the angle */
		angle = rb_bitsf((unsigned int)n);
		break;
	}
	pos[1] = 725.0f;
	pos[3] = 0.0f; /* the original leaves this word of the stack unset */
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_15) == 0)
			continue;
		pos[0] = cosf(angle) * 600.0f;
		pos[2] = sinf(angle) * 600.0f;
		rbb_PlayerPlace(p, pos);
		heading = atan2f(pos[0], pos[2]) + 3.14159274f;
		RB_F32(p, 0x41a4) = heading;
		RB_F32(p, RB_PL_HEADING) = heading;
		rbb_PlayerSet41a4(p, heading);
		axis[3] = -RB_F32(p, RB_PL_HEADING);
		axis[0] = 0.0f;
		axis[1] = 1.0f;
		axis[2] = 0.0f;
		bmAxisAngleToQuaternion(RB_FP(p, 0x4120), axis);
		angle = angle - step;
	}
	RB_I16(RB_GAME, RB_GO_STATE) = 1;
	rbb_CameraSetMode(RB_GAME + RB_GO_CAMERA, 1);
	for (i = 0; i < 5; i++) {
		RB_I32(s, 0x44 + i * 0x58) = 0;
		rbb_PlatformReset(RB_PTR(s, 0x58 + i * 0x58));
	}
	RB_F32(s, 0x200) = rb_RandomDropTime();
	RB_F32(s, 0x204) = rb_RandomDropTime();
	RB_F32(s, 0x208) = rb_RandomDropTime();
	RB_F32(s, 0x20c) = rb_RandomDropTime();
	CRollerBallTimer_Reset(s + 0x08);
}

/* 0x0018c000  RollerBall/CRollerball_Game.cpp
Not a function: Ghidra's entry in the middle of 0x0018bf40 (after the start angle is
chosen, with the frame of 0x0018bf40). Nothing calls it; its code is in
CRollerBallDrop_Reset. */
void CRollerBallDrop_ResetChunk(void)
{
}

/* ======================================================================== game: initialise */

/* 0x0018c220  RollerBall/CRollerball_Game.cpp
Initialise: keeps the difficulty (core's DAT_002bae70 at +0x20), loads the package
resources, initialises every sub-system in order (logging the first one that fails and
skipping the rest), then creates the variation object for game +0x1c (0 base, 1 sumo,
2 race2, 3 race, 4 drop) with its countdown (60 s, or 120 s for the races) and initialises
it with the variation package. */
void CRollerBallGame_Initialise(unsigned char *self)
{
	unsigned char *ents = self + RB_GO_ENTITIES;
	unsigned char *input = self + RB_GO_PHYSICS;
	void *gen;
	void *var;

	RB_I32(self, 0x20) = DAT_002bae70;
	bLoadPackageResources(RB_PTR(self, RB_GO_PACKAGE), 1, 0xdefa, 0);
	bLoadPackageResources(RB_PTR(self, RB_GO_GENPACKAGE), 1, 0xdefa, 0);
	gen = RB_PTR(self, RB_GO_GENPACKAGE);
	if (CRollerBallShaders_Init(self + RB_GO_SHADERS) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - shaderSystem.Initialise FAILED!\n");
		goto variation;
	}
	if (CRollerBallTextures_Init(self + RB_GO_TEXTURES, gen) == 0)
		bkPrintf("CRollerBall_Game::Initialise - textureSystem.Initialise FAILED!\n");
	if (CRollerBallLights_Init(self + RB_GO_LIGHTS) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - lightSystem.Initialise FAILED!\n");
	} else if (CDemolition_ObjectList_Init((struct CDemolition_ObjectList *)(self + RB_GO_PARTICLES),
			gen) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - particleSystem.Initialise FAILED!\n");
	} else if (CRollerBallClock_Init(self + RB_GO_CLOCK) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - timer.Initialise FAILED!\n");
	} else if (CDemoCountdown_Init((int)(uintptr_t)(self + RB_GO_COUNTDOWN)) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - countdown.Initialise FAILED!\n");
	} else if (RB_CALL(FUN_00191a50, int (*)(void *, void *))(self + RB_GO_GOALS, gen) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - goalSystem.Initialise FAILED!\n");
	} else if (CRollerBallRespawn_Init(self + RB_GO_RESPAWN, gen) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - respawnSystem.Initialise FAILED!\n");
	} else if (CRollerBallSpotFX_Init(self + RB_GO_SPOTFX, gen) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - spotFXSystem.Initialise FAILED!\n");
	} else if (RB_CALL(FUN_001966a0, int (*)(void *, void *, const char *))(self + RB_GO_WORLD,
			RB_PTR(self, RB_GO_PACKAGE),
			RB_VAR(const char *, 0x002b4b90u + RB_I32(self, RB_GO_VARIATION) * 4)) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - world.Initialise FAILED!\n");
	} else if (RB_CALL(FUN_001927e0, int (*)(void *, void *))(ents, gen) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - entitySystem.Initialise FAILED!\n");
	} else if (RB_CALL(FUN_00192f40, int (*)(void *))(self + RB_GO_CAMERA) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - camera.Initialise FAILED!\n");
	} else if (CRollerBallPhysics_Init(input) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - simulation.Initialise FAILED!\n");
	} else if (RB_CALL(AI_CanMove, int (*)(void *, void *))(self + RB_GO_HUD, gen) == 0) {
		/* 0x000b06c0 is a folded "return 1" (ret 4): the HUD's Initialise */
		bkPrintf("CRollerBall_Game::Initialise - overlaySystem.Initialise FAILED!\n");
	} else if (CDemoDebug_Init(self + RB_GO_DEBUG) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - debug.Initialise FAILED!\n");
	} else if (CRollerBallDirect3D_Init(self + RB_GO_DIRECT3D) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - direct3D.Initialise FAILED!\n");
	} else if (CRollerBallSfx_Init(self + RB_GO_SFX, gen) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - sfxSystem.Initialise FAILED!\n");
	} else if (CRollerBallInput_Init(self + RB_GO_INPUT) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - input.Initialise FAILED!\n");
	} else if (CRollerBallVideo_Init(self + RB_GO_VIDEO) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - videoScreenSystem.Initialise FAILED!\n");
	} else if (CRollerBallFloaty_Init(self + RB_GO_FLOATY, gen) == 0) {
		bkPrintf("CRollerBall_Game::Initialise - infoSystem.Initialise FAILED!\n");
	} else {
		if (CDemoRumble_Init(self + RB_GO_RUMBLE, gen) == 0)
			bkPrintf("CDemolition_Game::Initialise - rumbleSystem.Initialise FAILED!\n");
		if (RB_CALL(FUN_00196900, int (*)(void *))(self + RB_GO_SHADOWS) == 0)
			bkPrintf("CDemolition_Game::Initialise - shadowSystem.Initialise FAILED!\n");
		CRollerBallPhysics_SetPlayers(input, (void *)(uintptr_t)rbb_Entities_192350(ents));
	}
variation:
	switch (RB_I32(self, RB_GO_VARIATION)) {
	case 0:
		CRollerBallTimer_Set(self + RB_GO_COUNTDOWN, 60.0f);
		var = RB_NEW(8, RB_FILE_GAME, 0x2cc);
		if (var != NULL)
			*(const void *const **)var = vtable_00273a78;
		break;
	case 1:
		CRollerBallTimer_Set(self + RB_GO_COUNTDOWN, 60.0f);
		var = RB_NEW(0x2a0, RB_FILE_GAME, 0x2d1);
		if (var != NULL)
			var = CRollerBallSumo_Ctor(var);
		break;
	case 2:
		CRollerBallTimer_Set(self + RB_GO_COUNTDOWN, 120.0f);
		var = RB_NEW(0x70, RB_FILE_GAME, 0x2d6);
		if (var != NULL)
			*(const void *const **)var = vtable_00273b08;
		break;
	case 3:
		CRollerBallTimer_Set(self + RB_GO_COUNTDOWN, 120.0f);
		var = RB_NEW(0xac, RB_FILE_GAME, 0x2db);
		if (var != NULL)
			*(const void *const **)var = vtable_00273b50;
		break;
	case 4:
		CRollerBallTimer_Set(self + RB_GO_COUNTDOWN, 60.0f);
		var = RB_NEW(0x210, RB_FILE_GAME, 0x2e0);
		if (var != NULL)
			var = CRollerBallDrop_Ctor(var);
		break;
	default:
		goto init;
	}
	RB_PTR(self, RB_GO_VAROBJ) = var;
init:
	var = RB_PTR(self, RB_GO_VAROBJ);
	if (RB_VFN(var, 0x00, int (*)(void *, void *))(var, RB_PTR(self, RB_GO_PACKAGE)) == 0)
		bkPrintf("CRollerBall_Game::Initialise - variation.Initialise FAILED!\n");
}

/* ======================================================================== variation updates */

/* 0x0018c720  RollerBall/CRollerball_Game.cpp
Update (base): progress (+0x41ec) is the score relative to the best score; then the
state: intro, play, results, leave. */
void CRollerBallBase_Update(void *self, float dt)
{
	unsigned char *p, *first = RB_FIRST_PLAYER();
	int best = 0;
	float f;

	(void)dt;
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (best < RB_I32(p, RB_PL_SCORE))
			best = RB_I32(p, RB_PL_SCORE);
	}
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		f = 0.0f;
		if (best > 0)
			f = (1.0f / (float)best) * (float)RB_I32(p, RB_PL_SCORE);
		RB_F32(p, 0x41ec) = f;
	}
	switch (RB_I16(RB_GAME, RB_GO_STATE)) {
	case 1:
		CRollerBallBase_UpdateIntro(self);
		return;
	case 2:
		CRollerBallBase_UpdatePlay(self);
		return;
	case 3:
		CRollerBallVariation_UpdateResults();
		return;
	case 4:
		rb_UpdateLeave();
		return;
	}
}

/* a player that fell off the platform (y - radius at or below LEVEL) in sumo: out of the
round with the rank N (winner when N < 2), sounds, commentary with more than 2 left */
static void rb_SumoFallOff(void *self, unsigned char *p, int n)
{
	RB_U8(p, RB_PL_WINNER) = n < 2;
	RB_U8(p, RB_PL_1A) = 0;
	RB_I32(p, 0x41e8) = 1;
	RB_I32(p, RB_PL_41F0) = RB_I32(self, 0x40); /* the bits of the float height */
	RB_U8(p, RB_PL_ACTIVE) = 0;
	RB_U8(p, RB_PL_1D) = 0;
	RB_I32(p, RB_PL_RANK) = n;
	rbb_Ring_196f70((unsigned char *)self + 0xc0, RB_FP(p, RB_PL_POS), -6.0f, 200.0f,
		RB_F32(p, 0x413c));
	CRollerBallSfx_Play(RB_GAME + RB_GO_SFX, 0xc, 0x7f, 0, 0, NULL);
	CRollerBallSfx_Play(RB_GAME + RB_GO_SFX, 8, 0x7f, 0, 0, NULL);
	if (n > 2)
		CLSamples_PlayCommentary(0x37, RB_I32(p, RB_PL_INDEX), 0xff, 0, 0, NULL);
}

/* the players' taunt timers (per slot at SELF + OFF): when a timer runs out and the player
is near the edge (outside value A - 50) a new random time (SCALE) and a taunt sample */
static void rb_UpdateTaunts(void *self, unsigned char *first, float dt, int off, float scale,
	float edge)
{
	unsigned char *p;
	float *t;

	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) == 0)
			continue;
		t = RB_FP(self, off + RB_I32(p, RB_PL_SLOT) * 4);
		*t = *t - dt;
		if (*t < 0.0f) {
			*t = 0.0f;
			if (edge - 50.0f < sqrtf(RB_F32(p, RB_PL_POS) * RB_F32(p, RB_PL_POS) +
					RB_F32(p, RB_PL_POS + 8) * RB_F32(p, RB_PL_POS + 8))) {
				*t = ((float)rb_rand() * RB_RAND_SCALE + 0.5f) * scale;
				CLSamples_PlayFromSet((char)RB_U8(p, RB_PL_INDEX), 0xe, RB_FP(p, RB_PL_POS),
					DAT_00293928, 0, -1, 0);
			}
		}
	}
}

/* 0x0018c800  RollerBall/CRollerball_Game.cpp
Update (sumo): at 59.3 s left the arms retract (once) and the players may move; the arms
animate; the ring updates; progress is the countdown; taunts; the five platforms rise in
turn (state 0: waits for its time, 1: rising until the platform object stops, then the
ring shrinks, 2: sinks with the timer's fraction, 3: done); players that fall below the
ring are out (a second pass with 200 more when one fell); then the state. */
void CRollerBallSumo_Update(void *self, float dt)
{
	unsigned char *s = self;
	unsigned char *first, *p, **arm;
	struct BActorInstance *inst;
	float t, t2, v, frac, a[4], b[4], out[4], pos[4];
	unsigned char *pf;
	int i, n, fell;

	first = RB_FIRST_PLAYER();
	t = CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN);
	t2 = CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN);
	if (RB_U8(s, 0x260) == 0 && t2 <= rb_bitsf(0x426d3333u) /* 0x00274348: 59.3 */) {
		arm = (unsigned char **)(s + 0x264);
		for (i = 4; i != 0; i--, arm += 2) {
			inst = RB_PTR(arm[0], 0x30);
			baFlushAnimations(inst, RB_PTR(inst, 0x134), 1, 0);
			inst = RB_PTR(arm[0], 0x30);
			baQueueAnimation(inst, RB_PTR(inst, 0x134), (struct BAnim *)arm[1], 0x400, -1, 0,
				NULL, NULL);
		}
		for (p = first; p != NULL; p = RB_NEXT(p))
			RB_U8(p, RB_PL_1A) = 1;
		RB_U8(RB_PTR(s, 0x284), 1) = 0;
		RB_U8(RB_PTR(s, 0x284), 2) = 0;
		RB_U8(s, 0x260) = 1;
	}
	arm = (unsigned char **)(s + 0x264);
	for (i = 4; i != 0; i--, arm += 2) {
		inst = RB_PTR(arm[0], 0x30);
		baUpdateAnimations(inst, RB_PTR(inst, 0x134), dt, NULL);
		baPrepActorInstance(RB_PTR(arm[0], 0x30), 1);
	}
	RB_CALL(FUN_00196e30, void (*)(void *, float))(s + 0xc0, dt);
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) != 0)
			RB_F32(p, 0x41ec) = 1.0f - t * 0.0166666675f;
	}
	v = RB_VFN(self, 0x24, float (*)(void *))(self);
	rb_UpdateTaunts(self, first, dt, 0x288, 1200.0f, v);
	CRollerBallTimer_Update(s + 0x08);
	t = CRollerBallTimer_Remaining(s + 0x08);
	for (i = 0; i < 5; i++) {
		pf = s + 0x48 + i * 0x18;
		rbb_Platform_197fa0(RB_PTR(pf, 0x14), dt);
		switch (RB_I32(pf, 0)) {
		case 0:
			if (t2 < RB_F32(pf, 4)) {
				rbb_PlatformSet50(RB_PTR(pf, 0x14),
					(float)rb_rand() * RB_RAND_SCALE * 6.28318548f);
				rbb_Platform_197f40(RB_PTR(pf, 0x14));
				RB_I32(pf, 0) = 1;
			}
			break;
		case 1:
			if (RB_U8(RB_PTR(pf, 0x14), 0x78) == 0) {
				RB_I32(pf, 0) = 2;
				CRollerBallTimer_Set(s + 0x08, 1.0f);
				CRollerBallTimer_Start(s + 0x08);
				if (i - 1 >= 0)
					RB_U8(RB_PTR(pf, -4), 0x79) = 0;
				rbb_RingSet(s + 0xc0, RB_VAR(float, 0x002b4d1cu + i * 4) - 50.0f, 2030.0f);
				pos[0] = 0.0f;
				pos[1] = 0.0f;
				pos[2] = 0.0f;
				rbb_Ring_196f70(s + 0xc0, pos, -10.0f, RB_F32(pf, 8) - 50.0f, RB_F32(pf, 0xc));
				CRollerBallSfx_Play(RB_GAME + RB_GO_SFX, 10, 0xff, 0, 0, NULL);
				RB_U8(RB_PTR(pf, 0x10), 1) = 0;
				RB_U8(RB_PTR(pf, 0x10), 2) = 0;
			}
			break;
		case 2:
			frac = RB_F32(s + 0x08, 0x34); /* 0x000965f0 (folded): the timer's fraction */
			b[0] = 0.0f;
			b[1] = 0.0f;
			b[2] = 0.0f;
			a[0] = 0.0f;
			a[1] = -830.0f;
			a[2] = 0.0f;
			bmVectorLerp(out, b, a, frac);
			bmMatrixTranslation(RB_FP(RB_PTR(pf, 0x10), 0x34), out[0], out[1], out[2]);
			if (t <= 0.0f)
				RB_I32(pf, 0) = 3;
			break;
		}
	}
	n = 0;
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) != 0)
			n++;
	}
	fell = 0;
	if (first != NULL) {
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (RB_U8(p, RB_PL_15) != 0 && RB_U8(p, RB_PL_ACTIVE) != 0 &&
					RB_F32(p, RB_PL_POS + 4) - RB_F32(p, 0x413c) <= RB_F32(s, 0x40) &&
					RB_I32(p, 0x41e8) != 1) {
				rb_SumoFallOff(self, p, n);
				fell = 1;
			}
		}
		if (fell) {
			n = 0;
			for (p = first; p != NULL; p = RB_NEXT(p)) {
				if (RB_U8(p, RB_PL_ACTIVE) != 0)
					n++;
			}
			for (p = first; p != NULL; p = RB_NEXT(p)) {
				if (RB_U8(p, RB_PL_15) != 0 && RB_U8(p, RB_PL_ACTIVE) != 0 &&
						RB_F32(p, RB_PL_POS + 4) - RB_F32(p, 0x413c) <=
						RB_F32(s, 0x40) + 200.0f &&
						RB_I32(p, 0x41e8) != 1)
					rb_SumoFallOff(self, p, n);
			}
		}
	}
	switch (RB_I16(RB_GAME, RB_GO_STATE)) {
	case 1:
		CRollerBallBase_UpdateIntro(self);
		return;
	case 2:
		CRollerBallSumo_UpdatePlay(self);
		return;
	case 3:
		CRollerBallSumo_UpdateResults(self);
		return;
	case 4:
		rb_UpdateLeave();
		return;
	}
}

/* a player's race progress (+0x41ec) from its AI line index (AI object at +0x4204,
+0x4c): the distance along the race line POINTS / DIST of LENGTH, a third per lap (LAPS is
the lap part); changes of more than 1/6 are ignored. RACE: progress 0 while the laps are
negative. */
static void rb_RaceProgress(unsigned char *p, const float *points, const float *dist,
	int npoints, float length, float laps, int race)
{
	float dx, dz, d, prog;
	int idx;

	idx = RB_I32(RB_PTR(p, 0x4204), 0x4c);
	if (idx < 1)
		idx = npoints;
	dx = RB_F32(p, RB_PL_POS) - points[(idx - 1) * 4];
	dz = RB_F32(p, RB_PL_POS + 8) - points[(idx - 1) * 4 + 2];
	d = sqrtf(dx * dx + dz * dz) + dist[idx - 1];
	if (RB_I32(p, RB_PL_41F4) != 0)
		d = 0.0f;
	prog = (d / length) * 0.333333343f + laps;
	if (prog <= 1.0f) {
		if (prog < 0.0f)
			prog = 0.0f;
	} else {
		prog = 1.0f;
	}
	if (fabsf(RB_F32(p, 0x41ec) - prog) < 0.166666672f) {
		if (race && RB_I32(p, RB_PL_41F0) < 0)
			RB_F32(p, 0x41ec) = 0.0f;
		else
			RB_F32(p, 0x41ec) = prog;
	}
}

/* the AI line a player is on (AI object at +0x4204: +0x4c line index, +0x50 next index,
+0x3c the target point): the first line record that contains its position; the target is
the next line's point of POINTS */
static void rb_RaceFindLine(unsigned char *p, unsigned char *records, int nrecords,
	const float *points)
{
	unsigned char *ai = RB_PTR(p, 0x4204);
	float pos[4];
	int i, next;

	pos[0] = RB_F32(p, RB_PL_POS);
	pos[2] = RB_F32(p, RB_PL_POS + 8);
	pos[3] = RB_F32(p, RB_PL_POS + 12);
	pos[1] = 0.0f; /* the original leaves this word of the stack unset */
	for (i = 0; i < nrecords; i++) {
		if (rbb_Path_1981e0(records + i * 0x30, pos) != 0) {
			RB_I32(ai, 0x4c) = i;
			next = RB_I32(records, i * 0x30 + 0x2c);
			RB_I32(ai, 0x50) = next;
			RB_F32(ai, 0x3c) = points[next * 4];
			RB_F32(ai, 0x40) = points[next * 4 + 1];
			RB_F32(ai, 0x44) = points[next * 4 + 2];
			RB_F32(ai, 0x48) = points[next * 4 + 3];
			break;
		}
	}
}

/* 0x0018cf10  RollerBall/CRollerball_Game.cpp
Update (race2): players that fell (y < -1600) respawn at their last position; the AI line
and the progress of the players (one lap); then the state. */
void CRollerBallRace2_Update(void *self, float dt)
{
	unsigned char *s = self;
	unsigned char *p;
	int k = 0x30;

	(void)dt;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) == 0)
			continue;
		if (RB_F32(p, RB_PL_POS + 4) < -1600.0f)
			CRollerBallRespawn_Respawn(RB_GAME + RB_GO_RESPAWN, p, RB_FP(s, k));
		if (RB_I32(p, RB_PL_41F0) < 1)
			rb_RaceFindLine(p, RB_PTR(s, 0x28), RB_I32(s, 0x2c), RB_PTR(s, 0x04));
		rb_RaceProgress(p, RB_PTR(s, 0x04), RB_PTR(s, 0x0c), RB_I32(s, 0x08), RB_F32(s, 0x10),
			(float)RB_I32(p, RB_PL_41F0), 0);
		k += 0x10;
	}
	switch (RB_I16(RB_GAME, RB_GO_STATE)) {
	case 2:
		CRollerBallRace2_UpdatePlay(self);
		return;
	case 3:
		CRollerBallVariation_UpdateResults();
		return;
	case 4:
		rb_UpdateLeave();
		return;
	}
}

/* 0x0018d180  RollerBall/CRollerball_Game.cpp
Update (race): a player is on the track (+0x1d) when it is above the track (840) and near
an AI line; players below 780 respawn at their last position; the AI line (on the race
line of the AI's lane +0x54) and the progress (three laps); then the state. */
void CRollerBallRace_Update(void *self, float dt)
{
	unsigned char *s = self;
	unsigned char *p, *rec, *ai;
	float r, *pos;
	int i, k = 0, on;

	(void)dt;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) == 0)
			continue;
		ai = RB_PTR(p, 0x4204);
		r = RB_F32(p, 0x413c) + RB_F32(p, 0x413c);
		pos = RB_FP(p, RB_PL_POS);
		on = 0;
		if (rb_bitsf(0x44520000u) /* 0x00274350: 840 */ < RB_F32(p, RB_PL_POS + 4) -
				RB_F32(p, 0x413c)) {
			for (i = 0; i < RB_I32(s, 0x38); i++) {
				rec = (unsigned char *)RB_PTR(s, 0x34) + i * 0x30;
				if (pos[0] - r <= RB_F32(rec, 0x14) && RB_F32(rec, 4) <= r + pos[0] &&
						RB_F32(p, RB_PL_POS + 8) - r <= RB_F32(rec, 0x1c) &&
						RB_F32(rec, 0xc) <= r + RB_F32(p, RB_PL_POS + 8) &&
						(rbb_Path_1981e0(rec, pos) != 0 ||
						rbb_PathNear(rec, pos, r, NULL) != 0)) {
					on = 1;
					break;
				}
			}
		}
		RB_U8(p, RB_PL_1D) = (unsigned char)on;
		if (RB_F32(p, RB_PL_POS + 4) < rb_bitsf(0x44430000u) /* 0x0027434c: 780 */)
			CRollerBallRespawn_Respawn(RB_GAME + RB_GO_RESPAWN, p, RB_FP(s, k + 0x3c));
		if (RB_I32(p, RB_PL_41F0) < 3)
			rb_RaceFindLine(p, RB_PTR(s, 0x34), RB_I32(s, 0x38),
				RB_PTR(s, 4 + RB_I32(ai, 0x54) * 4));
		rb_RaceProgress(p, RB_PTR(s, 0x04), RB_PTR(s, 0x18), RB_I32(s, 0x14), RB_F32(s, 0x1c),
			(float)RB_I32(p, RB_PL_41F0) * 0.333333343f, 1);
		k += 0x10;
	}
	switch (RB_I16(RB_GAME, RB_GO_STATE)) {
	case 2:
		CRollerBallRace_UpdatePlay(self);
		return;
	case 3:
		CRollerBallVariation_UpdateResults();
		return;
	case 4:
		rb_UpdateLeave();
		return;
	}
}

/* 0x0018d4f0  RollerBall/CRollerball_Game.cpp
Update (drop): players out of the results are marked on the HUD (score box +0x54);
progress is the countdown; taunts; the five platforms fall in turn (state 0: waits for its
time, 1: shaking until the platform object stops, then it starts to fall with a random
spin, 2: falls with gravity 3 and turns until below -10000, 3: gone); players that fall
below the platform level (+0x40) are out (the last one wins); then the state. */
void CRollerBallDrop_Update(void *self, float dt)
{
	unsigned char *s = self;
	unsigned char *first, *p, *e;
	float t, t2, v, vy;
	float rot[16], trans[16];
	int i, n;

	first = RB_FIRST_PLAYER();
	t = CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN);
	t2 = CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN);
	for (i = 0; i < 4; i++) {
		if (DAT_002b9c48[i] != 0 && RB_PTR(DAT_002ecfac, 0x180 + i * 4) != NULL)
			RB_I16(RB_PTR(DAT_002ecfac, 0x180 + i * 4), 0x54) = 1;
	}
	for (p = first; p != NULL; p = RB_NEXT(p)) {
		if (RB_U8(p, RB_PL_ACTIVE) != 0)
			RB_F32(p, 0x41ec) = 1.0f - t * 0.0166666675f;
	}
	v = RB_VFN(self, 0x38, float (*)(void *))(self);
	rb_UpdateTaunts(self, first, dt, 0x200, 800.0f, v);
	for (i = 0; i < 5; i++) {
		e = s + 0x44 + i * 0x58;
		rbb_Platform_197fa0(RB_PTR(e, 0x14), dt + dt);
		switch (RB_I32(e, 0)) {
		case 0:
			if (t2 < RB_F32(e, 4)) {
				rbb_PlatformSet50(RB_PTR(e, 0x14),
					(float)rb_rand() * RB_RAND_SCALE * 6.28318548f);
				rbb_Platform_197f40(RB_PTR(e, 0x14));
				RB_I32(e, 0) = 1;
			}
			break;
		case 1:
			if (RB_U8(RB_PTR(e, 0x14), 0x78) == 0) {
				RB_I32(e, 0) = 2;
				RB_F32(e, 0x28) = 0.0f;
				RB_F32(e, 0x2c) = 0.0f;
				RB_F32(e, 0x30) = 0.0f;
				RB_F32(e, 0x18) = 0.0f;
				RB_F32(e, 0x1c) = 0.0f;
				RB_F32(e, 0x20) = 0.0f;
				RB_F32(e, 0x3c) = 0.0f;
				RB_F32(e, 0x38) = (float)rb_rand() * RB_RAND_SCALE * 0.0349065848f -
					0.0174532924f;
				RB_F32(e, 0x48) = 0.0f;
				RB_F32(e, 0x4c) = 0.0f;
				RB_F32(e, 0x50) = 0.0f;
				RB_F32(e, 0x40) = (float)rb_rand() * RB_RAND_SCALE * 0.0349065848f -
					0.0174532924f;
				if (i - 1 >= 0)
					RB_U8(RB_PTR(e, -0x44), 0x79) = 0;
				RB_U8(RB_PTR(e, 0x10), 1) = 0;
				RB_U8(RB_PTR(e, 0x10), 2) = 0;
			}
			break;
		case 2:
			vy = RB_F32(e, 0x2c) - 3.0f;
			RB_F32(e, 0x2c) = vy;
			RB_F32(e, 0x18) = dt * RB_F32(e, 0x28) + RB_F32(e, 0x18);
			RB_F32(e, 0x1c) = vy * dt + RB_F32(e, 0x1c);
			RB_F32(e, 0x20) = dt * RB_F32(e, 0x30) + RB_F32(e, 0x20);
			RB_F32(e, 0x48) = dt * RB_F32(e, 0x38) + RB_F32(e, 0x48);
			RB_F32(e, 0x50) = dt * RB_F32(e, 0x40) + RB_F32(e, 0x50);
			RB_F32(e, 0x38) = RB_F32(e, 0x38) * 0.99000001f;
			RB_F32(e, 0x3c) = RB_F32(e, 0x3c) * 0.99000001f;
			RB_F32(e, 0x40) = RB_F32(e, 0x40) * 0.99000001f;
			bmMatrixRotationXZ(rot, RB_F32(e, 0x50), RB_F32(e, 0x48));
			bmMatrixTranslation(trans, RB_F32(e, 0x18), RB_F32(e, 0x1c), RB_F32(e, 0x20));
			if ((((uintptr_t)RB_P(RB_PTR(e, 0x10), 4) | (uintptr_t)trans | (uintptr_t)rot) &
					0xf) == 0)
				bmMatrixMultiplyCopy(RB_FP(RB_PTR(e, 0x10), 0x34), trans, rot);
			else
				bmMatrixMultiply(RB_FP(RB_PTR(e, 0x10), 0x34), trans, rot);
			if (RB_F32(e, 0x1c) < -10000.0f) {
				RB_I32(e, 0) = 3;
				RB_U8(RB_PTR(e, 0x10), 0) = 0;
			}
			break;
		}
	}
	n = 0;
	if (first != NULL) {
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (RB_U8(p, RB_PL_ACTIVE) != 0)
				n++;
		}
		for (p = first; p != NULL; p = RB_NEXT(p)) {
			if (RB_U8(p, RB_PL_15) == 0 || RB_U8(p, RB_PL_ACTIVE) == 0 ||
					!(RB_F32(p, RB_PL_POS + 4) - RB_F32(p, 0x413c) <= RB_F32(s, 0x40)) ||
					RB_I32(p, 0x41e8) == 2)
				continue;
			if (n < 2) {
				RB_I32(p, RB_PL_41F0) = 0;
			} else {
				RB_U8(p, RB_PL_WINNER) = 0;
				RB_U8(p, RB_PL_1A) = 0;
				RB_I32(p, 0x41e8) = 2;
				RB_U8(p, RB_PL_ACTIVE) = 0;
				RB_U8(p, RB_PL_1D) = 0;
				RB_I32(p, RB_PL_41F0) = n;
				CRollerBallSfx_Play(RB_GAME + RB_GO_SFX, 8, 0xff, 0, 0, NULL);
				CLSamples_PlayFromSet((char)RB_U8(p, RB_PL_INDEX), 0xd, RB_FP(p, RB_PL_POS),
					DAT_00293928, 0, -1, 0);
				if (n > 2)
					CLSamples_PlayCommentary(0x37, RB_I32(p, RB_PL_INDEX), 0xff, 0, 0, NULL);
				n--;
			}
		}
	}
	switch (RB_I16(RB_GAME, RB_GO_STATE)) {
	case 2:
		CRollerBallDrop_UpdatePlay(self);
		return;
	case 3:
		CRollerBallVariation_UpdateResults();
		return;
	case 4:
		rb_UpdateLeave();
		return;
	}
}

/* ======================================================================== variation vtables */

/* 0x00273a78  base (goals) */
const void *const vtable_00273a78[18] = {
	(const void *)CRollerBallBase_Init,              /* +0x00 */
	(const void *)FUN_001d5950,                      /* +0x04 Shutdown (empty) */
	(const void *)CRollerBallVariation_Start,        /* +0x08 */
	(const void *)CRollerBallBase_Update,            /* +0x0c */
	(const void *)FUN_001d5950,                      /* +0x10 Draw (empty) */
	(const void *)CRollerBallVariation_Reset,        /* +0x14 */
	(const void *)CRollerBallVariation_SetScores,    /* +0x18 */
	(const void *)FUN_00151650,                      /* +0x1c (returns 0) */
	(const void *)CRollerBallVariation_Zero,         /* +0x20 */
	(const void *)CRollerBallVariation_Zero,         /* +0x24 */
	(const void *)CRollerBallVariation_Zero,         /* +0x28 */
	(const void *)CRollerBallVariation_Zero,         /* +0x2c */
	(const void *)FUN_00151650,                      /* +0x30 */
	(const void *)FUN_00151650,                      /* +0x34 */
	(const void *)CRollerBallVariation_Zero,         /* +0x38 */
	(const void *)CRollerBallVariation_Zero,         /* +0x3c */
	(const void *)CRollerBallVariation_Zero,         /* +0x40 */
	(const void *)CRollerBallBase_DeletingDtor,      /* +0x44 */
};

/* 0x00273ac0  sumo */
const void *const vtable_00273ac0[18] = {
	(const void *)CRollerBallSumo_Init,
	(const void *)CRollerBallSumo_Shutdown,
	(const void *)CRollerBallSumo_Start,
	(const void *)CRollerBallSumo_Update,
	(const void *)CRollerBallSumo_Draw,
	(const void *)CRollerBallSumo_Reset,
	(const void *)CRollerBallSumo_SetScores,
	(const void *)CRollerBallVariation_Get4,
	(const void *)CRollerBallVariation_TimerFraction,
	(const void *)CRollerBallSumo_GetValueA,
	(const void *)CRollerBallSumo_GetValueB,
	(const void *)CRollerBallSumo_GetTime,
	(const void *)FUN_00151650,
	(const void *)FUN_00151650,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallSumo_DeletingDtor,
};

/* 0x00273b08  race2 */
const void *const vtable_00273b08[18] = {
	(const void *)CRollerBallRace2_Init,
	(const void *)CRollerBallRace2_Shutdown,
	(const void *)CRollerBallRace2_Start,
	(const void *)CRollerBallRace2_Update,
	(const void *)FUN_001d5950,
	(const void *)CRollerBallRace2_Reset,
	(const void *)CRollerBallRace2_SetScores,
	(const void *)FUN_00151650,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallRace2_Get28,
	(const void *)CRollerBallRace2_Get2c,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallRace2_DeletingDtor,
};

/* 0x00273b50  race */
const void *const vtable_00273b50[18] = {
	(const void *)CRollerBallRace_Init,
	(const void *)CRollerBallRace_Shutdown,
	(const void *)CRollerBallVariation_Start,
	(const void *)CRollerBallRace_Update,
	(const void *)FUN_001d5950,
	(const void *)CRollerBallRace_Reset,
	(const void *)CRollerBallRace_SetScores,
	(const void *)FUN_00151650,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallRace_Get34,
	(const void *)CRollerBallRace_Get38,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallRace_DeletingDtor,
};

/* 0x00273b98  drop */
const void *const vtable_00273b98[18] = {
	(const void *)CRollerBallDrop_Init,
	(const void *)CRollerBallDrop_Shutdown,
	(const void *)CRollerBallDrop_Start,
	(const void *)CRollerBallDrop_Update,
	(const void *)CRollerBallDrop_Draw,
	(const void *)CRollerBallDrop_Reset,
	(const void *)CRollerBallDrop_SetScores,
	(const void *)CRollerBallVariation_Get4,
	(const void *)CRollerBallVariation_TimerFraction,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)CRollerBallVariation_Zero,
	(const void *)FUN_00151650,
	(const void *)FUN_00151650,
	(const void *)CRollerBallDrop_GetValueA,
	(const void *)CRollerBallDrop_GetValueB,
	(const void *)CRollerBallDrop_GetTime,
	(const void *)CRollerBallDrop_DeletingDtor,
};
