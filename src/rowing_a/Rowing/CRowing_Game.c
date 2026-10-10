/*
Rowing/CRowing_Game.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_Game.cpp

Game 10 of the 13 CGame objects (CGame vtable 0x00274e74, "Jetboats"), and the static
CRowing_Game object at 0x0030f0b0 that all of its slots run on: loading (packages, the
tweak menus), Initialise (world, players, camera, sprites, vents, timer, the variant's
water and lights), the start of a variant and the render pass.

The CRowing_Game object (0x0030f0b0; fields at original offsets, see rowing_a.h):
  +0x000 flags: bits 2-3 variant - 1; bit 0 passed to the camera's overlay draw
  +0x004 game state (6: results), +0x008, +0x00c
  +0x014 the variant package (packages\row_v%d), +0x018 its id
  +0x01c the general package (packages\row_gen), +0x020 its id
  +0x024 number of players (at most 4)
  +0x028, +0x02c, +0x030 variant timings (WFall start, Wpool start side, Wpool start in)
  +0x034 the time limit in ms
  +0x03c detail level ("detail" setting)
  +0x040 the seagulls, +0x044 the gull model, +0x048 a gull obstacle (11 words)
  +0x074 the "cargo lifter" node of the world actor
  +0x078 the game mode (rowing_b 0x001b60c0)
  +0x07c the world, +0x080 the water (CWaterField or a subclass), +0x084 the camera
  +0x088 the sprites, +0x08c (0x001aff40), +0x090 the vents (RowParticles)
  +0x094 the input, +0x098 the frame timer (profiler bars), +0x09c the debug text
  +0x0a0 the tokens (variant 3), +0x0a4 the lights, +0x0a9-0x0ab the ambient colour
  +0x0ac the floaty scores, +0x0b0 the floaty font
  +0x0b4, +0x0b8, +0x0bc spark, smoke and glow systems (variant 4)
  +0x0c8 100 world objects (variant 4)
  +0x258, +0x25c animated water jet textures, +0x260, +0x264 their frame counts,
  +0x268, +0x26c their frames
  +0x270 (rowing_b 0x001b47b0, variant 3)
  +0x274 the buoys (world +0x268 of them)
  +0x294 ten light handles, +0x2e4 one light per player
  +0x2f4, +0x2f8-0x304 the four tweak menus (JetGame, JetHandling, JetAI, JetLight)
  +0x30c, +0x310 two particle systems (the winner's sparkles)
  +0x314, +0x328 the colour set, +0x32c, +0x330 saved sound fall-off distances
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"
#include "bdisplay_types.h"

#define GAME_I32(off) RA_I32(self, off)
#define GAME_F32(off) RA_F32(self, off)
#define GAME_PTR(off) RA_PTRF(self, off)
#define GAME_VARIANT ((RA_U32(self, 0) >> 2) & 3)

/* the four world colour lights and their tweakable copies (.bss) */
#define g_LightR(i) RW_VAR(int, 0x0030f000 + (i) * 4)
#define g_LightB(i) RW_VAR(int, 0x0030f028 + (i) * 4)
#define g_LightG(i) RW_VAR(int, 0x0030f070 + (i) * 4)

/* 0x00274e74  vtable of game 10's CGame (rebuilt from the XBE) */
const void *const vtable_00274e74[4] = {
	(const void *)CGameRowing_DeletingDtor, /* +0x00 */
	(const void *)CGameRowing_Init,         /* +0x04 */
	(const void *)CGameRowing_Shutdown,     /* +0x08 */
	(const void *)CGameRowing_Start,        /* +0x0c */
};

/* 0x002758a8  the character colours (r, g, b) of the six characters */
static const int ra_CharacterColours[6][3] = {
	{ 0x2e, 0xa7, 0x24 }, { 0xd2, 0x00, 0x00 }, { 0x20, 0x71, 0xcd },
	{ 0xf8, 0xce, 0x00 }, { 0xf2, 0x82, 0x1e }, { 0xec, 0x6b, 0xae },
};

/* 0x002b5478  the world names of the variants (.data pointers to "race", ...) */
#define g_WorldNames RA_GPTR(const char *, 0x002b5478)

/* 0x0019d530  Rowing (gap; CRowing_Game.cpp inferred)
Game 10's constructor (core's Core_CreateGames). */
void *CGameRowing_Ctor(void *self)
{
	CGame_Ctor6((unsigned int *)self, "Jetboats", (unsigned int)(uintptr_t)"Harbouring Grudges",
		(unsigned int)(uintptr_t)"Water Gate Scramble", (unsigned int)(uintptr_t)" Eddy, Steady, Go!",
		(unsigned int)(uintptr_t)"Current Affair");
	*(const void *const **)self = vtable_00274e74;
	return self;
}

/* 0x0019d560  Rowing (gap; CRowing_Game.cpp inferred) */
void CGameRowing_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274e74;
	CGame_Dtor((unsigned int *)self);
}

/* 0x0019d570  Rowing/CRowing_Game.cpp (vtable slot 1)
Thunk "mov ecx, 0x0030f0b0; jmp 0x000a6fa0": that function returns 1. */
int CGameRowing_Init(void *self)
{
	(void)self;
	return BCarFloaty_DeleteItem();
}

/* 0x0019d580  Rowing/CRowing_Game.cpp (vtable slot 2)
Thunk "mov ecx, 0x0030f0b0; jmp 0x001d5950" (an empty function). */
void CGameRowing_Shutdown(void *self)
{
	(void)self;
	FUN_001d5950();
}

/* 0x0019d590  Rowing/CRowing_Game.cpp (vtable slot 3)
Thunk "mov ecx, 0x0030f0b0; jmp 0x0019e970". */
int CGameRowing_Start(void *self, int variant)
{
	(void)self;
	return FUN_0019e970(RA_GPTR(void, 0x0030f0b0), variant);
}

/* 0x0019d5a0  Rowing/CRowing_Game.cpp (vtable slot 0) */
void *CGameRowing_DeletingDtor(void *self, unsigned int flags)
{
	CGameRowing_Dtor(self);
	if (flags & 1)
		RA_DELETE(self);
	return self;
}

/* 0x0019d8d0  Rowing (gap; CRowing_Game.cpp inferred)
Thunk "add ecx, 0x2a8; jmp 0x001a5770": the method of the object at +0x2a8. */
void FUN_0019d8d0(void *self)
{
	FUN_001a5770((undefined4 *)RA_P(self, 0x2a8));
}

/* 0x0019d8e0  Rowing (gap; missing from the table)
Back to the 2D projection with no texture. */
void FUN_0019d8e0(void)
{
	bdSetProjectionMode(1);
	bdSetTexture(0, NULL);
}

/* 0x0019d900  Rowing (gap; missing from the table)
While the game mode loads in the background (+0x14 of the game mode): a message at most
once a second, and the game mode's UpdateLoading (slot +0x24) while background loading is
on. */
void FUN_0019d900(void *self)
{
	void *mode = GAME_PTR(0x78);
	unsigned long long now;
	long long ms;
	float freq, elapsed;

	if (RA_I32(mode, 0x14) == 0)
		return;
	now = bkTimerRead();
	freq = (float)DAT_002cd7a0;
	ms = (long long)bkTimerDelta(RW_VAR(unsigned long long, 0x0030f3e8), now) * 1000;
	elapsed = (float)ms / freq;
	if (elapsed < 0.0f || elapsed >= 1000.0f) {
		RW_VAR(unsigned long long, 0x0030f3e8) = now;
		bkPrintf("Rowing: Background Loading...\n");
	}
	if (DAT_002ba9a4 != 0)
		RA_VFN(GAME_PTR(0x78), 0x24, void (*)(void *))(GAME_PTR(0x78));
}

/* 0x0019d9b0  Rowing (gap; CRowing_Game.cpp inferred)
When FLAG is 1: places the two particle systems (+0x30c, +0x310) at the two ends of the
world's finish line (world +0x274, +0x284; the defaults at 0x002b54c4/0x002b54e4 when the
world has fewer than two) and starts them in PLAYER's character colour. */
void FUN_0019d9b0(void *self, int player, int flag)
{
	CPSHeader *ps0, *ps1;
	void *world;
	const int *colour;

	ps0 = (CPSHeader *)GAME_PTR(0x30c);
	ps1 = (CPSHeader *)GAME_PTR(0x310);
	if (flag != 1 || ps0 == NULL || ps1 == NULL)
		return;
	world = GAME_PTR(0x7c);
	if ((RA_U8(self, 0) & 0xc) != 0) {
		if (RA_I32(world, 0x270) < 2) {
			CParticleSystem_SetPosition(ps0, RA_GPTR(float, 0x002b54e4));
			CParticleSystem_SetPosition(ps1, RA_GPTR(float, 0x002b54f4));
		} else {
			CParticleSystem_SetPosition(ps0, RA_FP(world, 0x274));
			CParticleSystem_SetPosition(ps1, RA_FP(GAME_PTR(0x7c), 0x284));
		}
	} else if (RA_I32(world, 0x270) < 2) {
		CParticleSystem_SetPosition(ps0, RA_GPTR(float, 0x002b54c4));
		CParticleSystem_SetPosition(ps1, RA_GPTR(float, 0x002b54d4));
	} else {
		CParticleSystem_SetPosition(ps0, RA_FP(world, 0x274));
		CParticleSystem_SetPosition(ps1, RA_FP(GAME_PTR(0x7c), 0x284));
	}
	colour = ra_CharacterColours[DAT_002baed4[player * 10]];
	CParticleSystem_Start((struct CParticleSystem *)GAME_PTR(0x30c), (short)colour[0], (short)colour[1],
		(short)colour[2]);
	RA_U8(GAME_PTR(0x30c), 0x50) = 1;
	colour = ra_CharacterColours[DAT_002baed4[player * 10]];
	CParticleSystem_Start((struct CParticleSystem *)GAME_PTR(0x310), (short)colour[0], (short)colour[1],
		(short)colour[2]);
	RA_U8(GAME_PTR(0x310), 0x50) = 1;
}

/* the player objects (CRowing_Player, 0x3e8 bytes), 0x0030f490[4] */
#define g_RowPlayer(i) RW_VAR(void *, 0x0030f490 + (i) * 4)

/* 0x0019dad0  Rowing (gap; CRowing_Game.cpp inferred)
The winner when the game is over, else -1. Variant 1: the last boat still afloat (place
+0xe8 == 1 when the others have a place); variant 2: the race leader (+0x8c of the game
mode's results) when all but one finished, at least 1000 ahead (+0x128), or when the
time ran out (V12&4 Timelimit); variant 3: when the time ran out; variant 4: the only boat
that has the token (+0x37c) and is not sunk (+0x349), else the one in first place. */
int FUN_0019dad0(void *self)
{
	int n, i, count, winner, last;
	void *p;

	winner = -1;
	switch (GAME_VARIANT) {
	case 0:
		n = GAME_I32(0x24);
		count = 0;
		for (i = 0; i < n; i++) {
			if (RA_I32(g_RowPlayer(i), 0xe8) == 1)
				winner = i;
			if (RA_I32(g_RowPlayer(i), 0xe8) > 0)
				count++;
		}
		if (count >= n - 1)
			return winner;
		break;
	case 1:
		if (RW_VAR(int, 0x002b5488) == 0 || GAME_F32(0x34) != 0.0f) {
			n = GAME_I32(0x24);
			count = 0;
			last = -1;
			for (i = 0; i < n; i++) {
				if (RA_I32(g_RowPlayer(i), 0xe8) > 0)
					count++;
				else
					last = i;
			}
			if (count < n - 1)
				return -1;
			if (count == n - 1) {
				for (i = 0; i < n; i++) {
					if (i != last && RA_F32(g_RowPlayer(last), 0x128) <
							RA_F32(g_RowPlayer(i), 0x128) + 1000.0f)
						return -1;
				}
			}
		}
		return *(int *)GAME_PTR(0x8c);
	case 2:
		if (GAME_F32(0x34) != 0.0f)
			return -1;
		return *(int *)GAME_PTR(0x8c);
	case 3:
		count = 0;
		last = -1;
		if (GAME_I32(0x24) < 1)
			return -1;
		for (i = 0; i < GAME_I32(0x24); i++) {
			p = g_RowPlayer(i);
			if (RA_U8(p, 0x37c) != 0 && RA_U8(p, 0x349) == 0) {
				count++;
				winner = i;
			}
			if (RA_I32(p, 0xe8) == 1)
				last = i;
		}
		if (count == 1)
			return winner;
		if (count == 0)
			return last;
		break;
	default:
		return -1;
	}
	if (RW_VAR(int, 0x002b5488) != 0 && GAME_F32(0x34) == 0.0f)
		return *(int *)GAME_PTR(0x8c);
	return -1;
}

/* the camera's FOV-only update (rowing_a 0x001ae2f0) and the identity matrix */
#define RA_IDENTITY ((float *)DAT_002855f0)

/* lights on/off: the light types of the lights object */
static void ra_EnableLights(void *lights, const int (*list)[2], int count)
{
	int i;

	for (i = 0; i < count; i++)
		rwb_Lights_Enable(lights, list[i][0], list[i][1]);
}

/* 0x0019dc80  Rowing (gap; missing from the table; CRowing_Game.cpp inferred)
The render pass (the game mode's Render slot, TOP != 0): camera, world lights, sky and
gulls, the boats, the world, the water, the variant's extras, particles and sprites, the
debug text and the overlays. FUN_001af680 marks the frame timer's profiler bars. */
void FUN_0019dc80(void *self, int top)
{
	static const int lightsA[][2] = { { 1, 0 }, { 4, 0 }, { 6, 0 }, { 7, 0 }, { 2, 0 }, { 5, 0 }, { 8, 0 }, { 3, 1 } };
	static const int lightsB[][2] = { { 4, 0 }, { 5, 0 }, { 6, 0 }, { 7, 0 }, { 1, 0 }, { 2, 0 } };
	static const int lightsC[][2] = { { 8, 0 }, { 1, 0 }, { 6, 0 }, { 7, 0 }, { 2, 0 }, { 3, 0 }, { 5, 0 } };
	static const int lightsW0[][2] = { { 3, 0 }, { 1, 0 }, { 6, 0 }, { 7, 0 }, { 4, 0 }, { 2, 0 }, { 5, 0 } };
	static const int lightsW1[][2] = { { 3, 0 }, { 1, 1 }, { 6, 1 }, { 7, 1 }, { 4, 1 }, { 2, 1 }, { 5, 0 } };
	static const int lightsW2[][2] = { { 3, 0 }, { 1, 1 }, { 6, 1 }, { 7, 1 }, { 4, 1 }, { 2, 1 }, { 5, 1 } };
	void *timer = GAME_PTR(0x98);
	void *lights = GAME_PTR(0xa4);
	void *water;
	void *light;
	void *world;
	unsigned int variant;
	unsigned short wide[2];
	bdPrintFormat format;
	float width, height, alpha, pos[4], wfall, centre;
	unsigned long long now, delta;
	float *colours[4];
	int i, j, *strips;

	if (top == 0)
		return;
	FUN_001af680(timer, 0xff800000);
	for (i = 0; i < 4; i++) {
		light = RA_PTRF(self, 0x294 + i * 4);
		if (light != NULL && (RA_I32(light, 0x20) != g_LightR(i) || RA_I32(light, 0x24) != g_LightG(i) ||
				RA_I32(light, 0x28) != g_LightB(i)))
			rwb_Lights_SetColour(GAME_PTR(0xa4), light, g_LightR(i), g_LightG(i), g_LightB(i));
	}
	rwb_World_Update(GAME_PTR(0x7c));
	if (DAT_002baeb8 != 0) {
		FUN_001ae290(GAME_PTR(0x84));
	} else {
		if (CLIntroCam_Update(0.0f, 0) != 0 && DebugCam_IsActive() == 0) {
			if (DAT_002f26c4 == NULL) {
				FUN_001ae290(GAME_PTR(0x84));
				goto camera_done;
			}
			RW_VAR(int, 0x002b54a8) = 0;
			BCarFreeCamera_Update((struct BCarFreeCamera *)DAT_002f26c4, RA_F32(GAME_PTR(0x98), 0x9c));
		} else if (DAT_002f26c4 != NULL) {
			RW_VAR(int, 0x002b54a8) = 0;
			BCarFreeCamera_Update((struct BCarFreeCamera *)DAT_002f26c4, RA_F32(GAME_PTR(0x98), 0x9c));
		}
		bdSetNearFar(40.0f, 80000.0f);
	}
camera_done:
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(0, 1, 0);
	baSetAmbientLight(RA_U8(self, 0xa9), RA_U8(self, 0xaa), RA_U8(self, 0xab));
	bdSetRenderState(8, 0, 0);
	rwb_Sprites_DrawOpaque(GAME_PTR(0x88));
	FUN_001af680(GAME_PTR(0x98), 0xff008000);
	rwb_Lights_SetMode(GAME_PTR(0xa4), 0);
	ra_EnableLights(GAME_PTR(0xa4), lightsA, 8);
	bdSetRenderState(3, 0, 0);
	if (GAME_PTR(0x40) != NULL)
		CSeagulls_Draw(GAME_PTR(0x40));
	FUN_001af680(GAME_PTR(0x98), 0xff800080);
	if (RW_VAR(int, 0x0030f0ec) != 0) {
		bdSetWorldMatrix(RA_IDENTITY);
		baSetAmbientLight(0x40, 0x40, 0x40);
		FUN_001d5950();
		baSetAmbientLight(RA_U8(self, 0xa9), RA_U8(self, 0xaa), RA_U8(self, 0xab));
	}
	ra_EnableLights(GAME_PTR(0xa4), lightsB, 6);
	if (GAME_I32(0x04) == 6) {
		baSetAmbientLight(0x60, 0x60, 0x60);
		rwb_Lights_Enable(GAME_PTR(0xa4), 3, 0);
		rwb_Lights_Enable(GAME_PTR(0xa4), 8, 1);
	} else {
		baSetAmbientLight(0x60, 0x60, 0x60);
		rwb_Lights_Enable(GAME_PTR(0xa4), 3, 1);
	}
	if ((RA_U8(self, 0) & 0xc) == 0xc)
		bdSetRenderState(4, 1, 0);
	bdSetRenderState(3, 1, 0);
	for (i = 0; i < GAME_I32(0x24); i++)
		FUN_001aab80(g_RowPlayer(i), RW_VAR(int, 0x002b54a4));
	if ((RA_U8(self, 0) & 0xc) == 0xc)
		bdSetRenderState(4, 0, 0);
	baSetAmbientLight(RA_U8(self, 0xa9), RA_U8(self, 0xaa), RA_U8(self, 0xab));
	FUN_001af680(GAME_PTR(0x98), 0xff600060);
	bdSetRenderState(4, 1, 0);
	bdSetRenderState(8, 1, 0);
	ra_EnableLights(GAME_PTR(0xa4), lightsC, 7);
	bdSetRenderState(3, 0, 0);
	rwb_World_DrawA(GAME_PTR(0x7c), GAME_I32(0x3c));
	FUN_001af680(GAME_PTR(0x98), 0xffff0000);
	rwb_World_DrawB(GAME_PTR(0x7c), GAME_I32(0x3c));
	FUN_001af680(GAME_PTR(0x98), 0xff200020);
	bdSetRenderState(4, 0, 0);
	rwb_Lights_SetMode(GAME_PTR(0xa4), 1);
	switch (RW_VAR(int, 0x002b54b8)) {
	case 0:
		ra_EnableLights(GAME_PTR(0xa4), lightsW0, 7);
		break;
	case 1:
		ra_EnableLights(GAME_PTR(0xa4), lightsW1, 7);
		break;
	case 2:
		ra_EnableLights(GAME_PTR(0xa4), lightsW2, 7);
		break;
	}
	water = GAME_PTR(0x80);
	if (water != NULL && RW_VAR(int, 0x002b54ac) != 0) {
		FUN_001af680(GAME_PTR(0x98), 0xff008000);
		bdSetWorldMatrix(RA_IDENTITY);
		variant = GAME_VARIANT;
		if (variant <= 2) {
			if (RW_VAR(int, 0x002b54b0) != 0)
				RA_VFN(GAME_PTR(0x80), 0x18, void (*)(void *))(GAME_PTR(0x80));
			else
				RA_VFN(GAME_PTR(0x80), 0x14, void (*)(void *))(GAME_PTR(0x80));
		} else if (variant == 3) {
			RA_VFN(GAME_PTR(0x80), 0x14, void (*)(void *))(GAME_PTR(0x80));
		}
		FUN_001af680(GAME_PTR(0x98), 0xff0000ff);
		bdSetRenderState(3, 1, 0);
		if (RW_VAR(int, 0x002b54c0) != 0)
			CWaterField_SetMaterial(0xffffffffu, 0xff333333u, RW_VAR(float, 0x002b5504 + GAME_VARIANT * 4), 1);
		variant = GAME_VARIANT;
		if (variant == 2 || variant == 3)
			RA_VFN(GAME_PTR(0x80), 0x1c, int (*)(void *, int))(GAME_PTR(0x80), 1);
		else
			RA_VFN(GAME_PTR(0x80), 0x1c, int (*)(void *, int))(GAME_PTR(0x80), 0);
		if (RW_VAR(int, 0x002b54bc) != 0)
			RA_VFN(GAME_PTR(0x80), 0x20, int (*)(void *, float, int))(GAME_PTR(0x80), 12000.0f, 0);
		bdSetRenderState(3, 0, 0);
		if (RW_VAR(int, 0x002b54c0) != 0)
			CWaterField_ResetMaterial();
		FUN_001af680(GAME_PTR(0x98), 0xff000080);
	}
	bdSetRenderState(3, 0, 0);
	rwb_Lights_SetMode(GAME_PTR(0xa4), 0);
	ra_EnableLights(GAME_PTR(0xa4), lightsW0, 7);
	switch (GAME_VARIANT) {
	case 0:
		/* the cargo lifter */
		if (GAME_PTR(0x74) != NULL) {
			if (RW_VAR(int, 0x002b5474) != 0)
				baShowNode((struct BActorNode *)GAME_PTR(0x74), 1);
			else
				baHideNode((struct BActorNode *)GAME_PTR(0x74), 1);
		}
		break;
	case 1:
		alpha = GAME_I32(0x04) == 6 ? 0.1f : 1.0f;
		for (i = 0; i < RA_I32(GAME_PTR(0x7c), 0x268); i++)
			rwb_Buoy_Draw(RA_PTRF(self, 0x274 + i * 4), alpha);
		break;
	case 2:
		if (GAME_I32(0x04) != 6) {
			rwb_Tokens_Draw(GAME_PTR(0xa0));
			bdSetRenderState(8, 0, 0);
			bdSetRenderState(3, 0, 0);
			rwb_Tokens_DrawBlended(GAME_PTR(0xa0));
		}
		break;
	case 3:
		/* the waterfall's edges follow the tweakable width (WFall Width) */
		world = GAME_PTR(0x7c);
		for (i = 0; i < RA_I32(world, 0x134); i++) {
			strips = (int *)((char *)RA_PTRF(world, 0x138) + i * 0xc);
			for (j = 0; j < strips[0]; j++) {
				float *x = (float *)((char *)RA_PTRF(strips, 4) + j * 8);

				wfall = RW_VAR(float, 0x002b5630) * 0.5f;
				centre = RW_VAR(float, 0x002b5634);
				*x = *x >= 0.0f ? wfall + centre : centre - wfall;
				world = GAME_PTR(0x7c);
				strips = (int *)((char *)RA_PTRF(world, 0x138) + i * 0xc);
			}
			world = GAME_PTR(0x7c);
		}
		break;
	}
	bdSetRenderState(8, 0, 0);
	rwb_Lights_Draw(GAME_PTR(0xa4));
	if (GAME_PTR(0x270) != NULL)
		rwb_Obj270_Draw(GAME_PTR(0x270));
	for (i = 0; i < 2; i++) {
		void *ps = RA_PTRF(self, 0x30c + i * 4);

		if (ps != NULL)
			RA_VFN(ps, 0x08, void (*)(void *))(ps);
	}
	FUN_001af680(GAME_PTR(0x98), 0xffffffff);
	bdSetWorldMatrix(RA_IDENTITY);
	baDrawExtras(NULL, NULL);
	FUN_001af680(GAME_PTR(0x98), 0xff808080);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 0, 0);
	for (i = 0; i < GAME_I32(0x24); i++)
		FUN_001ada40(g_RowPlayer(i));
	FUN_001af680(GAME_PTR(0x98), 0xff404000);
	if (GAME_I32(0x3c) != 0)
		rwb_World_DrawDetail(GAME_PTR(0x7c));
	FUN_001af680(GAME_PTR(0x98), 0xff808000);
	FUN_001af680(GAME_PTR(0x98), 0xffffff00);
	if ((RA_U8(self, 0) & 0xc) == 0xc) {
		RA_VFN(GAME_PTR(0xb4), 0x08, void (*)(void *))(GAME_PTR(0xb4));
		RA_VFN(GAME_PTR(0xb8), 0x08, void (*)(void *))(GAME_PTR(0xb8));
		RA_VFN(GAME_PTR(0xbc), 0x08, void (*)(void *))(GAME_PTR(0xbc));
	}
	FUN_001af680(GAME_PTR(0x98), 0xff000000);
	if (RW_VAR(int, 0x002b54a0) != 0)
		rwb_Particles_Draw(GAME_PTR(0x90));
	rwb_Sprites_DrawBlended(GAME_PTR(0x88));
	FUN_001af680(GAME_PTR(0x98), 0xff00ffff);
	FUN_001af680(GAME_PTR(0x98), 0xffffff00);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 0, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(2, 3, 0);
	bdSetWorldMatrix(RA_IDENTITY);
	/* a space printed twice (flushes the debug font's state) */
	format.x = 0.0f;
	format.y = 0.0f;
	format.width = 640.0f;
	format.height = 480.0f;
	format.hAlign = 0;
	format.vAlign = 1;
	format.wrap = 0;
	format.lineSpacing = 0.0f;
	bkStringToWide(wide, " ");
	bdGetStringSize((struct bdFont *)RA_PTRF(GAME_PTR(0x9c), 4), wide, 1, &width, &height);
	bdPrintFontLite((struct bdFont *)RA_PTRF(GAME_PTR(0x9c), 4), wide, 1, width * -0.5f, 80.0f, 0x7f, 0, 0,
		0x7f, NULL, NULL);
	bdPrintFont((struct bdFont *)RA_PTRF(GAME_PTR(0x9c), 4), wide, 1, &format, 0x7f, 0, 0, 0x7f, NULL, NULL);
	FUN_001d5950();
	if (RA_I32(GAME_PTR(0x98), 0x234) != 0) {
		bdSetRenderState(3, 0, 0);
		now = bkTimerRead();
		RW_VAR(unsigned long long, 0x0030f3f8) = now;
		delta = bkTimerDelta(RW_VAR(unsigned long long, 0x0030f3f0), now);
		RW_VAR(unsigned long long, 0x0030f3f0) = RW_VAR(unsigned long long, 0x0030f3f8);
		FUN_001afaa0(GAME_PTR(0x9c), -300.0f, 100.0f, 0xff, 0xff, 0xff, 0x40, "FPS:%.2f",
			(double)bkTimerRate((long long)delta));
		DebugCam_GetPosition(pos);
		FUN_001afaa0(GAME_PTR(0x9c), -300.0f, 200.0f, 0xff, 0xff, 0xff, 0x40, "CAM:%.0f,%.0f,%.0f",
			(double)pos[0], (double)pos[1], (double)pos[2]);
		FUN_001afaa0(GAME_PTR(0x9c), -300.0f, 180.0f, 0xff, 0xff, 0, 0x40, "Polys:%d", DAT_002bc48c);
		DAT_002bc48c = 0;
		bdSetRenderState(3, 1, 0);
	}
	FUN_001af680(GAME_PTR(0x98), 0xff008080);
	FUN_001d5950();
	FUN_001afd20(GAME_PTR(0x9c));
	if (GAME_I32(0x3c) == 0)
		FUN_001a6e40();
	FUN_001af680(GAME_PTR(0x98), 0xff004040);
	if (RW_VAR(int, 0x002b54a8) != 0) {
		baSetAmbientLight(0x60, 0x64, 0x7f);
		FUN_001ae2f0(GAME_PTR(0x84));
		COverlay_DrawAll(1);
		FUN_001ae2f0(GAME_PTR(0x84));
	}
	FUN_001ae050(GAME_PTR(0x84), RA_U32(self, 0) & 1);
	FUN_001d5950();
	FUN_001af680(GAME_PTR(0x98), 0xff008000);
	FUN_001af570(GAME_PTR(0x98));
	/* the debris colours are the four players' colours (evaluated from player 3 down) */
	colours[3] = FUN_001a74a0(g_RowPlayer(3));
	colours[2] = FUN_001a74a0(g_RowPlayer(2));
	colours[1] = FUN_001a74a0(g_RowPlayer(1));
	colours[0] = FUN_001a74a0(g_RowPlayer(0));
	DebrisColours_SetAll(colours[0], colours[1], colours[2], colours[3]);
	(void)lights;
	(void)timer;
}

/* 0x0019e970  Rowing (gap; missing from the table; CRowing_Game.cpp inferred)
Start of a variant (CGame slot 3 on the game object): pushes the Rowing game mode and the
minigame intro (game 0x4f + VARIANT, picture "row%d.bmp"). */
int FUN_0019e970(void *self, int variant)
{
	char name[20];
	void *memory, *mode, *intro;

	memory = RA_NEW(0x38, RA_FILE_GAME, 0x15f);
	mode = memory != NULL ? rwb_GameMode_Ctor(memory) : NULL;
	if (mode == NULL)
		return 0;
	RA_U32(self, 0) ^= ((unsigned int)(variant * 4 - 4) ^ RA_U32(self, 0)) & 0xc;
	GAME_PTR(0x78) = mode;
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode, 1) == 0) {
		GAME_PTR(0x78) = NULL;
		return 0;
	}
	sprintf(name, "row%d.bmp", variant);
	memory = RA_NEW(0x300, RA_FILE_GAME, 0x17a);
	intro = memory != NULL
		? (void *)CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)memory, variant + 0x4f, name)
		: NULL;
	return CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)intro, 1) != 0;
}

/* 0x0019eaa0  Rowing (gap; CRowing_Game.cpp inferred)
Marks the cells of WATER outside the course as fixed (flags 1): a cell is inside when the
segment from (X, Z) to the cell crosses the world's outlines (world +0x13c outlines at
+0x140: count, points, closed) an odd number of times. */
void FUN_0019eaa0(CWaterField *water, void *world, float x, float z)
{
	float seg[4], edge[4];
	unsigned char hit[16];
	float px, pz, cellX, cellZ;
	int nx, nz, i, j, k, n, inside;
	char *outline;
	float *a, *b;

	pz = water->p.z0;
	px = water->p.x0;
	nx = water->p.nx;
	cellX = water->cellX;
	cellZ = water->cellZ;
	nz = water->p.nz;
	seg[0] = x;
	seg[1] = z;
	for (i = 0; i < nx; i++) {
		float cz = pz;

		for (j = 0; j < nz; j++) {
			seg[2] = px;
			seg[3] = cz;
			inside = 0;
			for (k = 0; k < RA_I32(world, 0x13c); k++) {
				outline = (char *)RA_PTRF(world, 0x140) + k * 0xc;
				if (RA_U8(outline, 8) == 0) {
					a = (float *)RA_PTRF(outline, 4);
					n = RA_I32(outline, 0) - 2;
					b = a + 2;
				} else {
					b = (float *)RA_PTRF(outline, 4);
					n = RA_I32(outline, 0) - 1;
					a = b + RA_I32(outline, 0) * 2 - 2;
				}
				for (; n >= 0; n--) {
					edge[0] = a[0];
					edge[1] = a[1];
					edge[2] = b[0];
					edge[3] = b[1];
					if (bmSegmentSegment(seg, edge, (struct BHit2D *)hit) != 0)
						inside ^= 1;
					a = b;
					b += 2;
				}
			}
			if (!inside)
				water->flags[water->p.nx * j + i] = 1;
			cz += cellZ;
		}
		px += cellX;
	}
}

/* the variant's water, allocated with SIZE bytes; VTABLE replaces CWaterField's when not
NULL. Returns NULL (and deletes it) when Init fails. */
static CWaterField *ra_CreateWater(void *self, unsigned int size, int line, const void *const *vtable,
	CWaterParams *params)
{
	void *memory = RA_NEW(size, RA_FILE_GAME, line);
	CWaterField *water = NULL;

	if (memory != NULL) {
		water = (CWaterField *)ra_WaterField_Ctor(memory);
		if (vtable != NULL)
			*(const void *const **)memory = vtable;
		water = (CWaterField *)memory;
	}
	GAME_PTR(0x80) = water;
	if (RA_VFN(water, 0x04, int (*)(void *, CWaterParams *))(water, params) == 0) {
		if (GAME_PTR(0x80) != NULL)
			RA_VFN(GAME_PTR(0x80), 0x00, void *(*)(void *, int))(GAME_PTR(0x80), 1);
		GAME_PTR(0x80) = NULL;
	}
	return (CWaterField *)GAME_PTR(0x80);
}

/* the two "helix" particle systems of variants 1, 2 (+0x30c, +0x310), and
the animated texture of the water jets */
static void ra_CreateHelixSystems(void *self, int line)
{
	struct bkResource *helix = bLoadResource(NULL, "tex_special\\helixtwist.bmp", RA_RES_TEXTURE, 0xdefa);
	struct bkResource *corona = bLoadResource(NULL, "tex_special\\corona01.bmp", RA_RES_TEXTURE, 0xdefa);
	void *memory;
	int i;

	for (i = 0; i < 2; i++) {
		memory = RA_NEW(0xa8, RA_FILE_GAME, line);
		if (memory != NULL)
			CParticleSystem_Construct((struct CParticleSystem *)memory);
		RA_PTRF(self, 0x30c + i * 4) = memory;
		CParticleSystem_Init((struct CParticleSystem *)memory, NULL, 800);
		RA_F32(RA_PTRF(self, 0x30c + i * 4), 0x70) = 30.0f;
		RA_F32(RA_PTRF(self, 0x30c + i * 4), 0x6c) = 5.0f;
		CParticleSystem_SetTexture((struct CPSHeader *)RA_PTRF(self, 0x30c + i * 4), (int)(uintptr_t)corona);
		CParticleSystem_SetTexture2((struct CParticleSystem *)RA_PTRF(self, 0x30c + i * 4), (int)(uintptr_t)helix);
	}
}

/* 0x0019ec50  Rowing/CRowing_Game.cpp
Variant 1 ("Harbouring Grudges", the harbour): the jet texture, the grey water, the
lights, the seagulls (30 when detailed) and the cargo lifter. */
void FUN_0019ec50(void *self, void *package)
{
	CWaterParams wp;
	float pos[3][4], dir[2][4];
	int size, gulls;
	struct bkResource *tex;
	void *memory;

	tex = bLoadResource(NULL, "tex_obj\\long loop jet.gif", RA_RES_TEXTURE, 0xdefa);
	GAME_PTR(0x258) = tex;
	if (tex != NULL)
		bdTextureGetInfo((struct bdTexture *)tex, NULL, NULL, NULL, (unsigned int *)RA_P(self, 0x260), NULL);
	GAME_I32(0x268) = 0;
	if (GAME_I32(0x3c) != 0) {
		size = RW_VAR(int, 0x002b54b4);
		gulls = 0x1e;
	} else {
		size = 0x20;
		gulls = 0;
	}
	wp = DAT_002ae578;
	wp.tension = wp.tension * rw_bitsf(0x3eaaaaab);
	wp.x0 = -2000.0f;
	wp.y = -61.0f;
	wp.z0 = -3000.0f;
	wp.sizeX = 5000.0f;
	wp.textureName = "tex_special\\grey_water.bmp";
	wp.heightScale = 1.0f;
	wp.nx = size;
	wp.nz = size;
	ra_CreateWater(self, 0x9090, 0x22d, NULL, &wp);
	pos[1][0] = 300.0f;
	pos[1][1] = 480.0f;
	pos[1][2] = -740.0f;
	pos[1][3] = 1.0f;
	pos[0][0] = 50.0f;
	pos[0][1] = 15000.0f;
	pos[0][2] = -100.0f;
	pos[0][3] = 1.0f;
	pos[2][0] = 1088.0f;
	pos[2][1] = 464.0f;
	pos[2][2] = -1153.0f;
	pos[2][3] = 1.0f;
	dir[0][0] = 0.0f;
	dir[0][1] = 0.70700002f;
	dir[0][2] = -0.70700002f;
	dir[0][3] = 1.0f;
	GAME_PTR(0x294) = rwb_Lights_AddPoint(GAME_PTR(0xa4), 1, pos[1], 0x19, 0x3f, 0x3f, rw_bitsf(0x360637bd), 1);
	GAME_PTR(0x298) = rwb_Lights_AddPoint(GAME_PTR(0xa4), 1, pos[0], 0x20, 0x40, 0x60, rw_bitsf(0x31abcc77), 1);
	GAME_PTR(0x29c) = rwb_Lights_AddPoint(GAME_PTR(0xa4), 1, pos[2], 0x40, 0x14, 10, rw_bitsf(0x360637bd), 1);
	GAME_PTR(0x2a0) = rwb_Lights_AddDirectional(GAME_PTR(0xa4), 3, dir[0], 0x7f, 0x7f, 0x7f, 0);
	dir[1][0] = 0.70700002f;
	dir[1][1] = 0.70700002f;
	dir[1][2] = 0.0f;
	dir[1][3] = 1.0f;
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 8, dir[1], 0x7f, 0x7f, 0x7f, 0);
	if (GAME_PTR(0x80) != NULL) {
		FUN_0019eaa0((CWaterField *)GAME_PTR(0x80), GAME_PTR(0x7c), -10000.0f, 0.0f);
		RA_VFN(GAME_PTR(0x80), 0x08, void (*)(void *))(GAME_PTR(0x80));
	}
	if (gulls != 0) {
		memcpy(RA_P(self, 0x48), RA_GPTR(void, 0x002b5514), 0xb * 4);
		GAME_PTR(0x44) = bLoadResource((struct bkPackage *)package, "objects\\gull.obe", RA_RES_ACTOR, 0xdefa);
		memory = RA_NEW(0x70, RA_FILE_GAME, 0x27c);
		GAME_PTR(0x40) = memory != NULL
			? (void *)CSeagulls_Init((struct CSeagulls *)memory, gulls, &DAT_002b2670, (int)(uintptr_t)GAME_PTR(0x44))
			: NULL;
		CSeagulls_AddObstacle((struct CSeagulls *)GAME_PTR(0x40), (int)(uintptr_t)RA_GPTR(void, 0x002b5514));
		CSeagulls_AddObstacle((struct CSeagulls *)GAME_PTR(0x40), (int)(uintptr_t)RA_P(self, 0x48));
	}
	if (RA_PTRF(GAME_PTR(0x7c), 0x2f4) == NULL)
		GAME_PTR(0x74) = NULL;
	else
		GAME_PTR(0x74) = baFindNode((struct BActorNode *)RA_PTRF(RA_PTRF(RA_PTRF(GAME_PTR(0x7c), 0x2f4), 0xc), 0x134),
			"cargo lifter");
	GAME_F32(0x34) = RW_VAR(float, 0x002b548c) * 1000.0f;
	RA_U8(self, 0xa9) = 8;
	RA_U8(self, 0xaa) = 0x20;
	RA_U8(self, 0xab) = 0x40;
	ra_CreateHelixSystems(self, 0x296);
}

/* 0x0019f0f0  Rowing/CRowing_Game.cpp
Variant 2 ("Water Gate Scramble"): the water, four lights, the helix systems and 10
seagulls; the world uses outline mode 2 with the gate table at 0x002b5540. */
void FUN_0019f0f0(void *self, void *package)
{
	static const unsigned char colours[4][3] = {
		{ 0x7f, 0x30, 0x7f }, { 0x7f, 0x30, 0x7f }, { 0x00, 0x7f, 0x00 }, { 0x00, 0x7f, 0x00 },
	};
	static const int types[4] = { 4, 4, 1, 1 };
	static const unsigned int falloff[4] = { 0x360637bd, 0x360637bd, 0x34210fb0, 0x34210fb0 };
	static const float positions[4][4] = {
		{ -811.0f, 400.0f, 0.0f, 1.0f }, { 667.0f, 400.0f, 0.0f, 1.0f },
		{ 800.0f, 2616.0f, 3318.0f, 1.0f }, { -1855.0f, 2616.0f, 3318.0f, 1.0f },
	};
	CWaterParams wp;
	float pos[4], dir[4];
	void *memory;
	int i;

	wp = DAT_002ae578;
	wp.tension = wp.tension * 0.5f;
	if (GAME_I32(0x3c) != 0) {
		wp.nx = 0x80;
		wp.nz = 0x54;
	} else {
		wp.nx = 0x20;
		wp.nz = 0x15;
	}
	wp.x0 = -3780.0f;
	wp.y = -20.0f;
	wp.z0 = -2330.0f;
	wp.sizeX = 7100.0f;
	wp.textureName = "tex_map\\water.bmp";
	ra_CreateWater(self, 0x9090, 0x2cf, NULL, &wp);
	for (i = 0; i < 4; i++) {
		memcpy(pos, positions[i], sizeof pos);
		RA_PTRF(self, 0x294 + i * 4) = rwb_Lights_AddPoint(GAME_PTR(0xa4), types[i], pos, colours[i][0],
			colours[i][1], colours[i][2], rw_bitsf(falloff[i]), 1);
	}
	dir[0] = 0.5f;
	dir[1] = 2.0f;
	dir[2] = -0.5f;
	dir[3] = 1.0f;
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 3, dir, 0x7f, 0x7f, 0x7f, 0);
	dir[0] = 0.70700002f;
	dir[1] = 0.70700002f;
	dir[2] = 0.0f;
	dir[3] = 1.0f;
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 8, dir, 0x7f, 0x7f, 0x7f, 0);
	if (GAME_PTR(0x80) != NULL) {
		FUN_0019eaa0((CWaterField *)GAME_PTR(0x80), GAME_PTR(0x7c), 0.1f, 0.1f);
		RA_VFN(GAME_PTR(0x80), 0x08, void (*)(void *))(GAME_PTR(0x80));
	}
	GAME_F32(0x34) = RW_VAR(float, 0x002b5490) * 1000.0f;
	RA_U8(self, 0xa9) = 8;
	RA_U8(self, 0xaa) = 0x20;
	RA_U8(self, 0xab) = 0x40;
	ra_CreateHelixSystems(self, 0x329);
	GAME_PTR(0x44) = bLoadResource((struct bkPackage *)package, "objects\\gull.obe", RA_RES_ACTOR, 0xdefa);
	memory = RA_NEW(0x70, RA_FILE_GAME, 0x33f);
	GAME_PTR(0x40) = memory != NULL
		? (void *)CSeagulls_Init((struct CSeagulls *)memory, 10, &DAT_002b2670, (int)(uintptr_t)GAME_PTR(0x44))
		: NULL;
	RA_U8(GAME_PTR(0x7c), 0x130) = 0;
	RA_I32(GAME_PTR(0x7c), 0x128) = 2;
	RA_PTRF(GAME_PTR(0x7c), 0x12c) = RA_GPTR(void, 0x002b5540);
}

/* the water of variant 3 (0x92a8 bytes, vtable 0x00274f60) and of variant 4 (0x90b0 bytes,
vtable 0x00274f8c): CWaterField subclasses whose methods are in this file */
/* 0x00274f60  vtable of variant 3's water (rebuilt from the XBE) */
const void *const vtable_00274f60[11] = {
	(const void *)FUN_0019fba0,        /* +0x00 deleting destructor */
	(const void *)FUN_001a56f0,        /* +0x04 Init */
	(const void *)CWaterField_Rebuild, /* +0x08 */
	(const void *)CWaterField_Ripple,  /* +0x0c */
	(const void *)CWaterField_Scroll,  /* +0x10 */
	(const void *)FUN_001a4460,        /* +0x14 */
	(const void *)FUN_001a3b00,        /* +0x18 */
	(const void *)CWaterField_Draw,    /* +0x1c */
	(const void *)FUN_001a4700,        /* +0x20 */
	(const void *)CWaterField_RandomDrop, /* +0x24 */
	(const void *)FUN_001a49b0,        /* +0x28 */
};

/* 0x00274f8c  vtable of variant 4's water (rebuilt from the XBE) */
const void *const vtable_00274f8c[11] = {
	(const void *)FUN_001a0260,        /* +0x00 deleting destructor */
	(const void *)FUN_001a4170,        /* +0x04 Init */
	(const void *)CWaterField_Rebuild, /* +0x08 */
	(const void *)FUN_001a41b0,        /* +0x0c */
	(const void *)CWaterField_Scroll,  /* +0x10 */
	(const void *)CWaterField_BuildVertices,  /* +0x14 */
	(const void *)CWaterField_UpdateVertices, /* +0x18 */
	(const void *)FUN_001a5010,        /* +0x1c */
	(const void *)FUN_001a43b0,        /* +0x20 */
	(const void *)CWaterField_RandomDrop, /* +0x24 */
	(const void *)CWaterField_Splash,  /* +0x28 */
};

/* 0x0019f590  Rowing/CRowing_Game.cpp
Variant 3 (" Eddy, Steady, Go!", the whirlpools): the "zzap" water texture, the wrapping
whirlpool water, six lights with the sun texture, the sun flare and the floaty font. */
void FUN_0019f590(void *self, void *package)
{
	static const unsigned char colours[6][3] = {
		{ 0x7f, 0x7f, 0x50 }, { 0x7f, 0x7f, 0x50 }, { 0x7f, 0x7f, 0x50 }, { 0x7f, 0x7f, 0x50 },
		{ 0x7f, 0x00, 0x00 }, { 0x7f, 0x00, 0x00 },
	};
	static const int types[6] = { 6, 6, 6, 6, 7, 7 };
	static const unsigned int falloff[6] = {
		0x360637bd, 0x360637bd, 0x360637bd, 0x360637bd, 0x3356bf95, 0x3356bf95,
	};
	static const float positions[6][4] = {
		{ 521.0f, 330.0f, 1239.0f, 1.0f }, { 476.0f, 330.0f, -1274.0f, 1.0f },
		{ -486.0f, 330.0f, -1283.0f, 1.0f }, { -552.0f, 330.0f, 1261.0f, 1.0f },
		{ -390.0f, 500.0f, 2440.0f, 1.0f }, { 390.0f, 500.0f, 2440.0f, 1.0f },
	};
	CWaterParams wp;
	float pos[4], dirA[4], dirB[4];
	char name[16];
	struct bkResource *tex;
	void *memory, *obj;
	int i;

	tex = bLoadResource(NULL, "tex_special\\zzap.gif", RA_RES_TEXTURE, 0xdefa);
	GAME_PTR(0x258) = tex;
	if (tex == NULL)
		bkPrintf("*** NO ANIM TEX ***\n");
	else
		bdTextureGetInfo((struct bdTexture *)tex, NULL, NULL, NULL, (unsigned int *)RA_P(self, 0x260), NULL);
	GAME_I32(0x268) = 0;
	GAME_I32(0x2c) = RW_VAR(int, 0x002b5650);
	GAME_I32(0x30) = RW_VAR(int, 0x002b5644);
	GAME_F32(0x34) = RW_VAR(float, 0x002b5494) * 1000.0f;
	wp = DAT_002ae578;
	wp.nx = 0x60;
	wp.nz = 0x60;
	wp.x0 = 0.0f;
	wp.y = -51.0f;
	wp.z0 = 0.0f;
	wp.sizeX = 1800.0f;
	wp.textureName = "tex_map\\water.bmp";
	wp.wrap = 1;
	wp.heightScale = 1.0f;
	wp.normalScale = 1.0f;
	ra_CreateWater(self, 0x92a8, 0x38b, vtable_00274f60, &wp);
	for (i = 0; i < 6; i++) {
		memcpy(pos, positions[i], sizeof pos);
		RA_PTRF(self, 0x294 + i * 4) = rwb_Lights_AddPoint(GAME_PTR(0xa4), types[i], pos, colours[i][0],
			colours[i][1], colours[i][2], rw_bitsf(falloff[i]), 1);
	}
	strcpy(name, "tex_map\\sun.bmp");
	tex = bLoadResource(NULL, name, RA_RES_TEXTURE, 0xdefa);
	rwb_Light_SetTexture(GAME_PTR(0x294), tex, 60.0f);
	rwb_Light_SetTexture(GAME_PTR(0x298), tex, 60.0f);
	rwb_Light_SetTexture(GAME_PTR(0x29c), tex, 60.0f);
	rwb_Light_SetTexture(GAME_PTR(0x2a0), tex, 60.0f);
	rwb_Light_SetTexture(GAME_PTR(0x2a4), tex, 100.0f);
	rwb_Light_SetTexture(GAME_PTR(0x2a8), tex, 100.0f);
	rwb_Lights_SetFlags(GAME_PTR(0xa4), 7, 2, 0);
	GAME_I32(0x2c8) = 0;
	GAME_I32(0x2c4) = 0;
	GAME_I32(0x2c0) = 0;
	GAME_I32(0x2bc) = 0;
	dirA[0] = 1.0f;
	dirA[1] = 0.40000001f;
	dirA[2] = -0.5f;
	dirA[3] = 1.0f;
	dirB[0] = 0.1f;
	dirB[1] = 1.0f;
	dirB[2] = -0.1f;
	dirB[3] = 1.0f;
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 3, dirA, 0x7f, 0x7f, 0x7f, 0);
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 3, dirB, 0x7f, 0x7f, 0x7f, 0);
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 8, dirA, 0x7f, 0x7f, 0x7f, 0);
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 8, dirB, 0x7f, 0x7f, 0x7f, 0);
	strcpy(name, "tex_map\\sun.bmp");
	tex = bLoadResource(NULL, name, RA_RES_TEXTURE, 0xdefa);
	memory = RA_NEW(0x20, RA_FILE_GAME, 0x3f1);
	obj = memory != NULL ? rwb_Obj270_Ctor(memory) : NULL;
	GAME_PTR(0x270) = obj;
	rwb_Obj270_Init(obj, tex, 2000);
	RA_U8(self, 0xa9) = 8;
	RA_U8(self, 0xaa) = 0x20;
	RA_U8(self, 0xab) = 0x40;
	GAME_PTR(0xb0) = bLoadResource((struct bkPackage *)package, "floaty.ttf", 3, 0xdefa);
}

/* 0x0019fba0  Rowing/CRowing_Game.cpp
Deleting destructor of variant 3's water (vtable 0x00274f60 slot 0). */
void *FUN_0019fba0(void *self, unsigned int flags)
{
	FUN_001a43c0(self);
	if (flags & 1)
		RA_DELETE(self);
	return self;
}

/* 0x0019fbc0  Rowing/CRowing_Game.cpp
Variant 4 ("Current Affair", the waterfall): the two water jet textures, the waterfall
water, 100 floating objects (50 mines, 50 from the debris table 0x002b5550), two lights
and the spark, smoke and glow systems of the explosions. */
void FUN_0019fbc0(void *self, void *package)
{
	static const unsigned char colours[2][3] = { { 0x7f, 0x28, 0x28 }, { 0x28, 0x7f, 0x7f } };
	static const unsigned int falloff[2] = { 0x3556bf95, 0x3556bf95 };
	static const float positions[2][4] = { { -381.0f, 500.0f, 0.0f, 1.0f }, { 319.0f, 500.0f, 0.0f, 1.0f } };
	CWaterParams wp;
	float pos[4], rot[4], lpos[4], dir[4];
	char name[64];
	struct bkResource *tex, *explosion, *sparks;
	void *memory, *objPackage;
	const char *objName;
	int i, r;

	tex = bLoadResource(NULL, "tex_special\\wjet.bmp", RA_RES_TEXTURE, 0xdefa);
	GAME_PTR(0x258) = tex;
	if (tex == NULL)
		bkPrintf("*** NO ANIM TEX ***\n");
	else
		bdTextureGetInfo((struct bdTexture *)tex, NULL, NULL, NULL, (unsigned int *)RA_P(self, 0x260), NULL);
	GAME_I32(0x268) = 0;
	tex = bLoadResource(NULL, "tex_special\\wjet2.bmp", RA_RES_TEXTURE, 0xdefa);
	GAME_PTR(0x25c) = tex;
	if (tex == NULL)
		bkPrintf("*** NO ANIM2 TEX ***\n");
	else
		bdTextureGetInfo((struct bdTexture *)tex, NULL, NULL, NULL, (unsigned int *)RA_P(self, 0x264), NULL);
	GAME_I32(0x26c) = 0;
	wp = DAT_002ae578;
	if (GAME_I32(0x3c) != 0) {
		wp.nx = (int)(RW_VAR(float, 0x002b5630) * 0.0792079195f);
		wp.nz = 0x78;
	} else {
		wp.nx = (int)(RW_VAR(float, 0x002b5630) * 0.0214521457f);
		wp.nz = 0x20;
	}
	wp.x0 = (RW_VAR(float, 0x002b5634) - RW_VAR(float, 0x002b5630) * 0.5f) - 125.0f;
	wp.sizeX = RW_VAR(float, 0x002b5630) + 250.0f;
	wp.y = 0.0f;
	wp.z0 = -1740.0f;
	wp.textureName = "tex_special\\grey_water.bmp";
	wp.heightScale = 0.0f;
	ra_CreateWater(self, 0x90b0, 0x489, vtable_00274f8c, &wp);
	if (GAME_PTR(0x80) != NULL)
		RA_VFN(GAME_PTR(0x80), 0x08, void (*)(void *))(GAME_PTR(0x80));
	GAME_I32(0x28) = RW_VAR(int, 0x002b5638);
	GAME_F32(0x34) = RW_VAR(float, 0x002b5498) * 1000.0f;
	GAME_I32(0x314) = 0;
	pos[0] = 0.0f;
	pos[1] = 300.0f;
	pos[2] = 100.0f;
	pos[3] = 1.0f;
	rot[0] = 0.0f;
	rot[1] = 0.0f;
	rot[2] = 0.0f;
	rot[3] = -1.0f;
	for (i = 0; i < 100; i++) {
		if (i < 50) {
			objName = "mine";
			sprintf(name, "objects\\%s.obe", objName);
			objPackage = package;
		} else {
			r = rw_rand() & 0xf;
			objName = RW_VAR(const char *, 0x002b5554 + r * 8);
			if (RW_VAR(int, 0x002b5550 + r * 8) == 0) {
				sprintf(name, "objects\\%s.obe", objName);
				objPackage = package;
			} else {
				sprintf(name, "%s.obe", objName);
				objPackage = NULL;
			}
		}
		_strlwr(name);
		pos[0] = (float)rw_rand() * 0.0152592547f - 250.0f;
		pos[2] = pos[2] + 300.0f;
		RA_PTRF(self, 0xc8 + i * 4) = rwb_World_AddObject(RA_PTRF(GAME_PTR(0x7c), 0x26c), objPackage, name, pos, rot);
	}
	for (i = 0; i < 2; i++) {
		memcpy(lpos, positions[i], sizeof lpos);
		RA_PTRF(self, 0x294 + i * 4) = rwb_Lights_AddPoint(GAME_PTR(0xa4), 1, lpos, colours[i][0],
			colours[i][1], colours[i][2], rw_bitsf(falloff[i]), 1);
	}
	dir[0] = 1.0f;
	dir[1] = 0.40000001f;
	dir[2] = -0.5f;
	dir[3] = 1.0f;
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 3, dir, 0x60, 0x60, 0x60, 0);
	dir[0] = 1.0f;
	dir[1] = 0.40000001f;
	dir[2] = -0.5f;
	dir[3] = 1.0f;
	rwb_Lights_AddDirectional(GAME_PTR(0xa4), 8, dir, 0x7f, 0x7f, 0x7f, 0);
	RA_U8(self, 0xa9) = 8;
	RA_U8(self, 0xaa) = 0x20;
	RA_U8(self, 0xab) = 0x40;
	explosion = bLoadResource(NULL, "tex_special\\explosion.bmp", RA_RES_TEXTURE, 0xdefa);
	sparks = bLoadResource(NULL, "tex_special\\sparkburst01.bmp", RA_RES_TEXTURE, 0xdefa);
	memory = RA_NEW(0x74, RA_FILE_GAME, 0x503);
	if (memory != NULL)
		CSparkSystem_Construct((struct CSparkSystem *)memory);
	GAME_PTR(0xb4) = memory;
	CSparkSystem_Init((struct CSparkSystem *)memory, NULL, 0x32);
	CParticleSystem_SetTexture((struct CPSHeader *)GAME_PTR(0xb4), (int)(uintptr_t)sparks);
	memory = RA_NEW(0x60, RA_FILE_GAME, 0x507);
	if (memory != NULL)
		CSmokeSystem_Construct((struct CSmokeSystem *)memory);
	GAME_PTR(0xb8) = memory;
	CSmokeSystem_Init((struct CSmokeSystem *)memory, NULL, 0x32);
	CParticleSystem_SetTexture((struct CPSHeader *)GAME_PTR(0xb8), (int)(uintptr_t)explosion);
	memory = RA_NEW(0x68, RA_FILE_GAME, 0x50b);
	if (memory != NULL)
		CGlowSystem_Construct((struct CGlowSystem *)memory);
	GAME_PTR(0xbc) = memory;
	CGlowSystem_Init((struct CGlowSystem *)memory, NULL, 0x32);
	CParticleSystem_SetTexture((struct CPSHeader *)GAME_PTR(0xbc), (int)(uintptr_t)explosion);
}

/* 0x001a0260  Rowing/CRowing_Game.cpp
Deleting destructor of variant 4's water (vtable 0x00274f8c slot 0). */
void *FUN_001a0260(void *self, unsigned int flags)
{
	FUN_001a0280(self);
	if (flags & 1)
		RA_DELETE(self);
	return self;
}

/* 0x001a0280  Rowing/CRowing_Game.cpp
Thunk to CWaterField_Destruct (0x00138ff0). */
void FUN_001a0280(void *self)
{
	CWaterField_Destruct((CWaterField *)self);
}

/* 0x001a0290  Rowing/CRowing_Game.cpp
Called by the game mode's constructor (rowing_b 0x001b60c0): saves the sound fall-off
distances, creates the four tweak menus (JetGame, JetHandling, JetAI, JetLight), clears the
objects and starts loading packages\row_v<variant> and packages\row_gen in the background. */
void CRowingGame_Load(void *self)
{
	static const char *const menuNames[4] = { "JetGame", "JetHandling", "JetAI", "JetLight" };
	static const int menuLines[4] = { 0x538, 0x539, 0x53a, 0x53b };
	char unused[20];
	char path[0x200];
	void *memory;
	int i;

	memset(path, 0, sizeof path);
	GAME_F32(0x32c) = DAT_0028ffa8;
	GAME_F32(0x330) = DAT_0028ffac;
	sprintf(unused, "jetboats");
	for (i = 0; i < 4; i++) {
		memory = RA_NEW(0xe8, RA_FILE_GAME, menuLines[i]);
		RA_PTRF(self, 0x2f8 + i * 4) = memory != NULL
			? (void *)CTweakMenu_Ctor((struct CTweakMenu *)memory, menuNames[i])
			: NULL;
	}
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "Detail", RA_GPTR(int, 0x0030f0ec));
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x2f8), "Closeup", RA_GPTR(int, 0x0030f0a4), 1, 0, 2, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x2f8), "BoatDetail", RA_GPTR(int, 0x002b54a4), 1, 0, 2, 0);
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "BigAnim", RA_GPTR(int, 0x002b5920));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "Lifter", RA_GPTR(int, 0x002b5474));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "Water", RA_GPTR(int, 0x002b54ac));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "WaterTest", RA_GPTR(int, 0x002b54b0));
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x2f8), "V1WaterSize", RA_GPTR(int, 0x002b54b4), 1, 16, 128, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x2f8), "Waterlights", RA_GPTR(int, 0x002b54b8), 1, 0, 2, 0);
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "ActorWorld", RA_GPTR(int, 0x002b5924));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "AILines", RA_GPTR(int, 0x0030f0ac));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "DebugCam", RA_GPTR(int, 0x0030f0a8));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "Timer", RA_GPTR(int, 0x0030f09c));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "AllHuman", RA_GPTR(int, 0x0030f0a0));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "V12&4 Timelimit", RA_GPTR(int, 0x002b5488));
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "(V1    Time)", RA_GPTR(float, 0x002b548c), 5.0f, 10.0f, 1800.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "(V2    Time)", RA_GPTR(float, 0x002b5490), 5.0f, 10.0f, 1800.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "(V4    Time)", RA_GPTR(float, 0x002b5498), 5.0f, 10.0f, 1800.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "Missed Gate", RA_GPTR(float, 0x002b56d0), 1.0f, 0.0f, 50.0f);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x2f8), "Num Tokens", RA_GPTR(int, 0x002b598c), 1, 0, 64, 0);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "Token Freq", RA_GPTR(float, 0x002b5990), 0.001f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "Wpool Time ", RA_GPTR(float, 0x002b5494), 5.0f, 10.0f, 1800.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "WPool Penalty", RA_GPTR(float, 0x002b56c8), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "WPool Spawntime", RA_GPTR(float, 0x0030f4a0), 0.5f, 0.0f, 10.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2f8), "WFall Width", RA_GPTR(float, 0x002b5630), 10.0f, 100.0f, 2000.0f);
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2f8), "V4Steam", RA_GPTR(int, 0x002b54a0));
	CTweakMenu_AddBool((struct CTweakMenu *)GAME_PTR(0x2fc), "Sesame Street Ctrls", RA_GPTR(int, 0x002b567c));
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "AngSpin", RA_GPTR(float, 0x002b5750), 0.0001f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "AngMax", RA_GPTR(float, 0x002b5754), 0.005f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "AngDamp", RA_GPTR(float, 0x002b5670), 0.0025f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "Accel", RA_GPTR(float, 0x002b5758), 0.005f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "Strafe", RA_GPTR(float, 0x002b575c), 0.005f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "VelDampBase", RA_GPTR(float, 0x002b565c), 0.0025f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "VelDamp1", RA_GPTR(float, 0x002b5660), 0.0025f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "VelDamp2", RA_GPTR(float, 0x002b5668), 0.005f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "VelDampSpd1", RA_GPTR(float, 0x002b5664), 0.2f, 0.0f, 100.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "VelDampSpd2", RA_GPTR(float, 0x002b566c), 0.5f, 0.0f, 100.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "WFall start", RA_GPTR(float, 0x002b5638), 1.0f, 0.0f, 1000.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "WFall delta", RA_GPTR(float, 0x002b563c), 0.001f, 0.0f, 10.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "WFall max", RA_GPTR(float, 0x002b5640), 10.0f, 0.0f, 1000.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "Wpool start in", RA_GPTR(float, 0x002b5644), 10.0f, 0.0f, 10000.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "Wpool delta in", RA_GPTR(float, 0x002b5648), 0.01f, 0.0f, 100.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "Wpool max   in", RA_GPTR(float, 0x002b564c), 10.0f, 0.0f, 100000.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "Wpool start side", RA_GPTR(float, 0x002b5650), 0.1f, 0.0f, 100.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "Wpool delta side", RA_GPTR(float, 0x002b5654), 0.001f, 0.0f, 100.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x2fc), "Wpool max   side", RA_GPTR(float, 0x002b5658), 0.01f, 0.0f, 100.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "EZ: ConcernAng", RA_GPTR(float, 0x002b5724), 0.01f, 0.0f, 4.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "EZ: ReverseAng", RA_GPTR(float, 0x002b572c), 0.01f, 0.0f, 4.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "EZ:      Speed", RA_GPTR(float, 0x002b5738), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "EZ:   DeltaSpd", RA_GPTR(float, 0x002b5744), 0.01f, 0.0f, 2.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "MD: ConcernAng", RA_GPTR(float, 0x002b5728), 0.01f, 0.0f, 4.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "MD: ReverseAng", RA_GPTR(float, 0x002b5730), 0.01f, 0.0f, 4.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "MD:      Speed", RA_GPTR(float, 0x002b573c), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "MD:   DeltaSpd", RA_GPTR(float, 0x002b5748), 0.01f, 0.0f, 2.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "HD: ConcernAng", RA_GPTR(float, 0x0030f4a8), 0.01f, 0.0f, 4.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "HD: ReverseAng", RA_GPTR(float, 0x002b5734), 0.01f, 0.0f, 4.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "HD:      Speed", RA_GPTR(float, 0x002b5740), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "HD:   DeltaSpd", RA_GPTR(float, 0x002b574c), 0.01f, 0.0f, 2.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x300), "EaseOffDist   ", RA_GPTR(float, 0x002b5720), 5.0f, 0.0f, 1000.0f);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "FogR", RA_GPTR(int, 0x002b5928), 1, 0, 255, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "FogG", RA_GPTR(int, 0x002b592c), 1, 0, 255, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "FogB", RA_GPTR(int, 0x002b5930), 1, 0, 255, 0);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x304), "FogNear", RA_GPTR(float, 0x002b5934), 100.0f, 0.0f, 1000000.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)GAME_PTR(0x304), "FogFar", RA_GPTR(float, 0x002b5938), 100.0f, 500.0f, 1000000.0f);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 0 R", RA_GPTR(int, 0x0030f000), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 0 G", RA_GPTR(int, 0x0030f070), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 0 B", RA_GPTR(int, 0x0030f028), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 1 R", RA_GPTR(int, 0x0030f004), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 1 G", RA_GPTR(int, 0x0030f074), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 1 B", RA_GPTR(int, 0x0030f02c), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 2 R", RA_GPTR(int, 0x0030f008), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 2 G", RA_GPTR(int, 0x0030f078), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 2 B", RA_GPTR(int, 0x0030f030), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 3 R", RA_GPTR(int, 0x0030f00c), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 3 G", RA_GPTR(int, 0x0030f07c), 1, 0, 127, 0);
	CTweakMenu_AddInt((struct CTweakMenu *)GAME_PTR(0x304), "Light 3 B", RA_GPTR(int, 0x0030f034), 1, 0, 127, 0);
	GAME_PTR(0x40) = NULL;
	GAME_PTR(0x80) = NULL;
	GAME_PTR(0xa0) = NULL;
	GAME_PTR(0x270) = NULL;
	GAME_PTR(0x2f0) = NULL;
	GAME_PTR(0x2ec) = NULL;
	GAME_PTR(0x2e8) = NULL;
	GAME_PTR(0x2e4) = NULL;
	sprintf(path, "packages\\row_v%d", GAME_VARIANT + 1);
	bkPrintf("Background Loading Begin...\n");
	GAME_PTR(0x14) = ra_AddPackage(path, 0);
	if (GAME_PTR(0x14) == NULL)
		bkPrintf("ERROR rowing package failed to load\n");
	GAME_PTR(0x1c) = ra_AddPackage("packages\\row_gen", 0);
	if (GAME_PTR(0x1c) == NULL) {
		bkPrintf("ERROR rowing package failed to load\n");
		GAME_I32(0x328) = DAT_00283ea4;
	} else {
		GAME_I32(0x328) = DAT_00283ea4;
	}
	GAME_I32(0xc0) = 0;
}

/* the score table's (core_modes_b, 0x002b9968) fields used here */
#define RA_SCORES_I32(addr) (*(int *)(DAT_002b9968 + ((addr) - 0x002b9968)))

/* MSVC rand() % n, inlined (seed in DAT_00285630) */
static int ra_RandMod(int n)
{
	return rw_rand() % n;
}

/* 0x001a0dd0  Rowing/CRowing_Game.cpp
Initialise (the game mode's FinishedLoading, via the thunk at 0x001b6170): loads the
packages' resources, creates the lights, the leader table, the debug text, the world, the
sound, the players (start order: the rubber-band order of the last results, shuffled within
equal places, or a random order), the input, the camera, the sprites, the vents, the frame
timer, the tokens, the floaty scores and the variant's water and lights. */
void CRowingGame_Initialise(void *self)
{
	int rubber[4], order[8];
	void *memory, *player;
	const char *value;
	int i, base, count, round, k, a, b, t, arg;
	unsigned int ia, ib;

	bLoadPackageResources((struct bkPackage *)GAME_PTR(0x14), 1, 0xdefa, 0);
	bLoadPackageResources((struct bkPackage *)GAME_PTR(0x1c), 1, 0xdefa, 0);
	memset(RA_P(self, 0x294), 0, 10 * 4);
	GAME_I32(0x18) = (int)bkGetPackageId((struct bkPackage *)GAME_PTR(0x14));
	GAME_I32(0x20) = (int)bkGetPackageId((struct bkPackage *)GAME_PTR(0x1c));
	memory = RA_NEW(0x48, RA_FILE_GAME, 0x5f4);
	GAME_PTR(0xa4) = memory != NULL ? rwb_Lights_Ctor(memory) : NULL;
	memory = RA_NEW(0xf8, RA_FILE_GAME, 0x5f5);
	GAME_PTR(0x8c) = memory != NULL ? FUN_001aff40(memory, (int)GAME_VARIANT) : NULL;
	value = Settings_Get((struct Settings *)DAT_002ba9a0, "detail");
	GAME_I32(0x3c) = value != NULL ? atol(value) : 1;
	value = Settings_Get((struct Settings *)DAT_002ba9a0, "allhuman");
	RW_VAR(int, 0x0030f0a0) = value != NULL ? atol(value) : 0;
	value = Settings_Get((struct Settings *)DAT_002ba9a0, "difficulty");
	if (value != NULL)
		DAT_002bae70 = atol(value);
	memory = RA_NEW(0x6d70, RA_FILE_GAME, 0x60a);
	GAME_PTR(0x9c) = memory != NULL ? FUN_001afa50(memory) : NULL;
	if (FUN_001afa60(GAME_PTR(0x9c), GAME_PTR(0x1c)) == 0) {
		bkPrintf("CRowing_Game::Initialise - debug.Initialise FAILED!\n");
		goto variant;
	}
	GAME_I32(0x24) = DAT_002bae94;
	if (DAT_002bae94 > 4)
		GAME_I32(0x24) = 4;
	FUN_001a5950();
	memory = RA_NEW(0x37c, RA_FILE_GAME, 0x63b);
	GAME_PTR(0x7c) = memory != NULL ? rwb_World_Ctor(memory) : NULL;
	if (rwb_World_Init(GAME_PTR(0x7c), GAME_PTR(0x14), g_WorldNames[GAME_VARIANT]) == 0) {
		bkPrintf("CRowing_Game::Initialise - world.Initialise FAILED!\n");
		goto variant;
	}
	rwb_Sound_Init(GAME_PTR(0x14), GAME_PTR(0x1c));
	RA_U32(self, 0) &= ~3u;
	bkPrintf("Sound ok\n");
	FUN_001a57b0(0, 0, 3.0f, 0.3f, 0);
	for (i = 0; i < DAT_002bae94; i++) {
		DAT_002baee8[i * 10] = 0;
		order[i] = i;
	}
	CScoreTable_Rank((struct CScoreTable *)DAT_002b9968);
	bkPrintf("playerInPosition List = %d %d %d %d\n", RA_SCORES_I32(0x002b9c58), RA_SCORES_I32(0x002b9c5c),
		RA_SCORES_I32(0x002b9c60), RA_SCORES_I32(0x002b9c64));
	bkPrintf("num players in position = %d %d %d %d\n", RA_SCORES_I32(0x002b9c68), RA_SCORES_I32(0x002b9c6c),
		RA_SCORES_I32(0x002b9c70), RA_SCORES_I32(0x002b9c74));
	for (i = 0; i < DAT_002bae94; i++)
		rubber[i] = RA_SCORES_I32(0x002b9c58 + i * 4);
	bkPrintf("Pre-Shuffling List = %d %d %d %d\n", rubber[0], rubber[1], rubber[2], rubber[3]);
	/* shuffle the players that share a place: 20 random swaps within each of the first
	three groups (the last group is not shuffled) */
	base = 0;
	for (k = 0; k < 3; k++) {
		count = RA_SCORES_I32(0x002b9c68 + k * 4);
		if (count > 1) {
			for (round = 0; round < 20; round++) {
				a = ra_RandMod(count) + base;
				b = ra_RandMod(count) + base;
				t = rubber[a];
				rubber[a] = rubber[b];
				rubber[b] = t;
			}
		}
		base += count;
	}
	for (round = 0; round < 20; round++) {
		ia = (unsigned int)rw_rand() & 3;
		ib = (unsigned int)rw_rand() & 3;
		t = order[ia];
		order[ia] = order[ib];
		order[ib] = t;
	}
	bkPrintf("Rubber order is %d %d %d %d\n", rubber[0], rubber[1], rubber[2], rubber[3]);
	bkPrintf("Random order is %d %d %d %d\n", order[0], order[1], order[2], order[3]);
	for (i = 0; i < DAT_002bae94; i++)
		order[rubber[i] + 4] = i;
	arg = 0;
	for (i = 0; i < DAT_002bae94; i++) {
		switch (GAME_VARIANT) {
		case 0:
		case 1:
			arg = order[i + 4];
			break;
		case 2:
		case 3:
			arg = order[i];
			break;
		}
		memory = RA_NEW(1000, RA_FILE_GAME, 0x6d1);
		player = NULL;
		if (memory != NULL) {
			FUN_001a5750(RA_P(memory, 0x2a8));
			player = memory;
		}
		g_RowPlayer(i) = player;
		FUN_001abc70(player, GAME_PTR(0x14), GAME_PTR(0x1c), i, arg, RA_SCORES_I32(0x002b9c48 + i * 4));
	}
	bkPrintf("Players ok\n");
	GAME_PTR(0x94) = RA_NEW(8, RA_FILE_GAME, 0x6d9);
	if (FUN_001af960(GAME_PTR(0x94)) == 0) {
		bkPrintf("CRowing_Game::Initialise - input.Initialise FAILED!\n");
		goto variant;
	}
	bkPrintf("Input ok\n");
	memory = RA_NEW(0xc0, RA_FILE_GAME, 0x6e3);
	GAME_PTR(0x84) = memory != NULL ? FUN_001add20(memory) : NULL;
	if (FUN_001add40(GAME_PTR(0x84), (int)GAME_VARIANT) == 0) {
		bkPrintf("CRowing_Game::Initialise - camera.Initialise FAILED!\n");
		goto variant;
	}
	bkPrintf("Camera ok\n");
	memory = RA_NEW(1, RA_FILE_GAME, 0x6ec);
	GAME_PTR(0x88) = memory != NULL ? FUN_001b12a0(memory) : NULL;
	RA_CALL(FUN_001b12c0, int (*)(void *))(GAME_PTR(0x88));
	bkPrintf("Sprites ok\n");
	memory = RA_NEW(0x28, RA_FILE_GAME, 0x6f1);
	GAME_PTR(0x90) = memory;
	switch (GAME_VARIANT) {
	case 1:
		rwb_Particles_Init(memory, GAME_PTR(0x88), 500, 200);
		break;
	case 2:
	case 3:
		rwb_Particles_Init(memory, GAME_PTR(0x88), 500, 10);
		break;
	default:
		rwb_Particles_Init(memory, GAME_PTR(0x88), 10, 10);
		break;
	}
	bkPrintf("Vents ok\n");
	memory = RA_NEW(0x240, RA_FILE_GAME, 0x706);
	GAME_PTR(0x98) = memory != NULL ? FUN_001af380(memory) : NULL;
	if (FUN_001af3d0(GAME_PTR(0x98)) == 0) {
		bkPrintf("CRowing_Game::Initialise - timer.Initialise FAILED!\n");
		goto variant;
	}
	bkPrintf("Timer ok\n");
	if ((RA_U8(self, 0) & 0xc) == 8) {
		GAME_PTR(0xa0) = RA_NEW(1, RA_FILE_GAME, 0x712);
		rwb_Tokens_Init(GAME_PTR(0xa0), GAME_PTR(0x14));
		bkPrintf("Tokens ok\n");
	}
	GAME_PTR(0xac) = RA_NEW(0x90, RA_FILE_GAME, 0x717);
	BCarFloatyScore_Init((struct BCarFloatyScore *)GAME_PTR(0xac), 0x14);
	RA_F32(DAT_002f27c4, 0x80) = RA_F32(DAT_002f27c4, 0x80) * 30.0f;
	RA_F32(DAT_002f27c4, 0x88) = RA_F32(DAT_002f27c4, 0x88) * 60.0f;
	RA_F32(DAT_002f27c4, 0x84) = 0.050000001f;
variant:
	switch (GAME_VARIANT) {
	case 0:
		FUN_0019ec50(self, GAME_PTR(0x14));
		break;
	case 1:
		FUN_0019f0f0(self, GAME_PTR(0x14));
		break;
	case 2:
		FUN_0019f590(self, GAME_PTR(0x14));
		break;
	case 3:
		FUN_0019fbc0(self, GAME_PTR(0x14));
		break;
	}
	bkPrintf("Variation Init ok\n");
	for (i = 0; i < 4; i++) {
		void *light = RA_PTRF(self, 0x294 + i * 4);

		if (light != NULL) {
			g_LightR(i) = RA_I32(light, 0x20);
			g_LightG(i) = RA_I32(RA_PTRF(self, 0x294 + i * 4), 0x24);
			g_LightB(i) = RA_I32(RA_PTRF(self, 0x294 + i * 4), 0x28);
		}
	}
	for (i = 0; i < DAT_002bae94; i++) {
		float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

		RA_PTRF(self, 0x2e4 + i * 4) = rwb_Lights_AddPoint(GAME_PTR(0xa4), 5, pos, 0x7f, 0x7f, 0x20,
			rw_bitsf(0x3727c5ac), 1);
	}
	GAME_I32(0x2f4) = 0;
	bkPrintf("Package Closure ok\n");
	DebugCam_InitWatch(RA_GPTR(int, 0x0030f0a8));
	bkPrintf("DB Camera ok\n");
	GAME_I32(0x04) = 0;
	GAME_I32(0x08) = 0;
	GAME_I32(0x0c) = 0;
}
