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
Back to the 2D projection with no texture (the game mode's slot via the thunk at
0x001b6110; GAME is in ECX and not used). */
void FUN_0019d8e0(void *game)
{
	(void)game;
	bdSetProjectionMode(1);
	bdSetTexture(0, NULL);
}

/* 0x0019d900  Rowing (gap; missing from the table)
While the game mode loads in the background (+0x14 of the game mode): a message at most
once a second, and the game mode's UpdateLoading (slot +0x24) while background loading is
on. */
void FUN_0019d900(void *self, int top)
{
	void *mode = GAME_PTR(0x78);
	unsigned long long now;
	long long ms;
	float freq, elapsed;

	(void)top;
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
	FUN_001a57b0(0.0f, 0.0f, 3.0f, 0.30000001f, 0.0f);
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

extern unsigned char DAT_00290f00[]; /* bumpercars_a: the "aisteam1" sprite defs (6) */
extern int DAT_002ed324[];           /* classlib_ui: the rumble object */
extern int DAT_002bae98;             /* core: the play mode (2: the board game) */
extern int DAT_002ba56c;             /* bumpercars_b: the debug free camera is enabled */
extern void *DAT_002ba97c;           /* core: the current CGame */
extern int DAT_00283e58;             /* core: the "fast" cheat */
extern int DAT_00283ef0;             /* core: a single human player gets the camera */
extern int DAT_002baed8[];           /* core: per player record [i * 10]: human */

#define g_CurrentFrameTime(self) RA_F32(RA_PTRF(self, 0x98), 0x9c)

/* 0x001a1ac0  Rowing/CRowing_Game.cpp
Begin (called by the game mode, rowing_b 0x001b6180, when the intro is over): sound
fall-off distances, the engine sounds, the HUD list, the debris, the "aisteam1" sprites,
the projector and the referee bot, the players' start, the buoys (world +0x1c8, one per
world +0x268) and the debug free camera with its five points. */
void CRowingGame_Begin(void *self)
{
	void *memory, *buoy, *list;
	int i;

	baFlushShaderCache();
	DAT_0028ffa8 = 20000.0f;
	DAT_0028ffac = 30000.0f;
	GAME_I32(0x308) = 0;
	rwb_Sound_Track(g_RowSound, rwb_Sound_Play(0, 0x80));
	rwb_Sound_Track(g_RowSound, rwb_Sound_Play(3, 0x80));
	if ((RA_U8(self, 0) & 0xc) == 8)
		rwb_Sound_Track(g_RowSound, rwb_Sound_Play(0xe, 0x96));
	if ((RA_U8(self, 0) & 0xc) == 0xc) {
		GAME_I32(0x318) = rwb_Sound_PlayAt(4, RA_GPTR(float, 0x002b5620), 0x7f);
		rwb_Sound_Track(g_RowSound, GAME_I32(0x318));
		GAME_I32(0x31c) = rwb_Sound_PlayAt(5, RA_GPTR(float, 0x002b5620), 0x7f);
		rwb_Sound_Track(g_RowSound, GAME_I32(0x31c));
		GAME_F32(0x320) = 1.0f;
		GAME_F32(0x324) = 1.0f;
	}
	memory = RA_NEW(0xc, RA_FILE_GAME, 0x7bd);
	list = NULL;
	if (memory != NULL) {
		CLList_Init((int *)memory);
		list = memory;
	}
	GAME_PTR(0x38) = list;
	FUN_00072720((undefined4)(uintptr_t)list);
	FUN_001b0f70(GAME_PTR(0x8c));
	Debris_Init(NULL, 1000);
	Sprite_LoadDefs((struct CSpriteDef *)DAT_00290f00, 6, 0);
	CProjector_Reset();
	RefBot_Create();
	for (i = 0; i < GAME_I32(0x24); i++)
		FUN_001a8940(g_RowPlayer(i), 4);
	for (i = 0; i < RA_I32(GAME_PTR(0x7c), 0x268); i++) {
		memory = RA_NEW(0xe0, RA_FILE_GAME, 0x7d2);
		buoy = memory != NULL
			? rwb_Buoy_Ctor(memory, RA_FP(GAME_PTR(0x7c), 0x1c8 + i * 0x10), RA_I32(GAME_PTR(0x7c), 0x248 + i * 4))
			: NULL;
		RA_PTRF(self, 0x274 + i * 4) = buoy;
	}
	if (DAT_002ba56c != 0) {
		memory = RA_NEW(0x26c, RA_FILE_GAME, 0x7e1);
		if (memory != NULL)
			BCarFreeCamera_Construct((struct BCarFreeCamera *)memory, 0, 20.0f, 1);
		for (i = 0; i < 5; i++)
			BCarFreeCamera_AddPoint((struct BCarFreeCamera *)DAT_002f26c4, RA_GPTR(float, 0x002b55d0 + i * 0x10));
	}
}

/* 0x001a1d60  Rowing/CRowing_Game.cpp
Variant 3: steam around the whirlpool at the water's centre, more the less time is left
(1 + 2t puffs, t = the elapsed fraction of the whirlpool time). */
void CRowingGame_EmitSteam(void *self)
{
	float t, n, spread, rise, size, height, pos[4];
	int i;

	if (RA_U8(self, 8) & 1)
		return;
	t = (RW_VAR(float, 0x002b5494) - GAME_F32(0x34) * 0.001f) / RW_VAR(float, 0x002b5494);
	n = (0.5f + t) + (0.5f + t);
	if (!(0.0f < n))
		return;
	spread = t + t + 1.0f;
	rise = 200.0f * t + 100.0f;
	size = (t + 1.0f) * 20.0f;
	i = 0;
	do {
		height = FUN_001a4b10(GAME_PTR(0x80), 0.0f, 0.0f);
		pos[0] = ((float)rw_rand() * 0.00305185094f - 50.0f) * spread;
		pos[2] = ((float)rw_rand() * 0.00305185094f - 50.0f) * spread;
		pos[1] = (height + 150.0f) - (float)rw_rand() * rise * rw_bitsf(0x38000100);
		pos[3] = 1.0f;
		RowParticles_AddSteam((struct RowParticles *)GAME_PTR(0x90), pos,
			(float)rw_rand() * size * rw_bitsf(0x38000100) + 30.0f, 0.0f);
		i++;
	} while ((float)i < n);
}

/* 0x001a1f20  Rowing/CRowing_Game.cpp
The camera: the intro fly-by (CLIntroCam) until it ends or a player presses a button,
then the race camera (FUN_001aeb90). In variant 2 the intro starts slowly. */
void CRowingGame_UpdateCamera(void *self)
{
	void *cam;
	float fov, step, eye[4], target[4];
	int i, skip, frames, n;

	FUN_001adea0(GAME_PTR(0x84), g_CurrentFrameTime(self));
	if (DAT_002baeb8 != 0) {
		if ((RA_U32(self, 0) & 1) == 0) {
			RA_U32(self, 0) |= 1;
			GAME_I32(0x04) = 1;
			GAME_I32(0x08) = 0;
			GAME_I32(0x0c) = 0;
		}
		FUN_001aeb90(GAME_PTR(0x84), g_CurrentFrameTime(self));
		if (GAME_I32(0x08) != 0)
			return;
		cam = GAME_PTR(0x84);
		RA_F32(cam, 0x0c) = RA_F32(cam, 0x1c);
		RA_F32(cam, 0x10) = RA_F32(cam, 0x20);
		RA_F32(cam, 0x14) = RA_F32(cam, 0x24);
		RA_F32(cam, 0x18) = RA_F32(cam, 0x28);
		RA_F32(cam, 0x2c) = RA_F32(cam, 0x3c);
		RA_F32(cam, 0x30) = RA_F32(cam, 0x40);
		RA_F32(cam, 0x34) = RA_F32(cam, 0x44);
		RA_F32(cam, 0x38) = RA_F32(cam, 0x48);
		return;
	}
	if ((RA_U8(self, 0) & 1) == 0) {
		CLIntroCam_GetCurrentFov(&fov);
		FUN_001ae2e0(GAME_PTR(0x84), fov);
		skip = 0;
		for (i = 0; i < GAME_I32(0x24); i++) {
			if (FUN_001a76e0(g_RowPlayer(i)) != 0)
				skip = 1;
		}
		if (!skip) {
			if ((RA_U8(self, 0) & 0xc) == 4) {
				n = GAME_I32(0x08);
				if (n < 1)
					step = g_CurrentFrameTime(self);
				else if (n < 10)
					step = 0.0f;
				else if (n > 0x1d)
					step = g_CurrentFrameTime(self);
				else
					step = (float)(n - 10) * g_CurrentFrameTime(self) * 0.050000001f;
			} else {
				step = g_CurrentFrameTime(self);
			}
			if (CLIntroCam_Update(step, 0) != 1)
				return;
		}
		RA_U32(self, 0) |= 1;
		GAME_I32(0x04) = 1;
		GAME_I32(0x08) = 0;
		GAME_I32(0x0c) = 0;
		CLIntroCam_Stop();
		frames = CLIntroCam_GetFrameCount() - 1;
		CLIntroCam_GetPosition(eye, (float)frames);
		frames = CLIntroCam_GetFrameCount() - 1;
		CLIntroCam_GetTarget(target, (float)frames);
		bdSetFieldOfView(1.22173f, 0.91629785f);
		FUN_001ae2e0(GAME_PTR(0x84), 1.22173f);
	}
	FUN_001aeb90(GAME_PTR(0x84), g_CurrentFrameTime(self));
}

/* 0x001a2130  Rowing/CRowing_Game.cpp
Picks the two boats nearest to the camera (in front of it, visible, afloat and with a
driver) for the detailed models; the others get the simple ones (FUN_001abb70). Only during
the intro (state 0) and the results (state 6). */
void CRowingGame_UpdateCloseups(void *self)
{
	float best, second, d, eye[4], target[4], diff[3], dir[3];
	void *p, *cam;
	int i, count, bestIdx, secondIdx;

	count = 0;
	bestIdx = -1;
	secondIdx = -1;
	best = 2000.0f;
	second = 2000.0f;
	if (GAME_I32(0x04) == 6 || GAME_I32(0x04) == 0) {
		if (GAME_I32(0x04) == 0) {
			CLIntroCam_GetPosition(eye, CLIntroCam_GetTime());
			CLIntroCam_GetTarget(target, CLIntroCam_GetTime());
		} else {
			cam = GAME_PTR(0x84);
			eye[0] = RA_F32(cam, 0x2c);
			eye[1] = RA_F32(cam, 0x30);
			eye[2] = RA_F32(cam, 0x34);
			eye[3] = RA_F32(cam, 0x38);
			target[0] = RA_F32(cam, 0x0c);
			target[1] = RA_F32(cam, 0x10);
			target[2] = RA_F32(cam, 0x14);
			target[3] = RA_F32(cam, 0x18);
		}
		diff[0] = target[0] - eye[0];
		diff[1] = target[1] - eye[1];
		diff[2] = target[2] - eye[2];
		bmVectorSetLength(dir, diff, 1.0f);
		for (i = 0; i < 4; i++) {
			p = g_RowPlayer(i);
			d = dir[0] * (RA_F32(p, 0x2b4) - eye[0]) + dir[1] * (RA_F32(p, 0x2b8) - eye[1]) +
				dir[2] * (RA_F32(p, 0x2bc) - eye[2]);
			if (d < -100.0f)
				d = 1e7f;
			if (bdSphereVisible(RA_FP(p, 0x2b4), 300.0f) == 0)
				d = 1e7f;
			if (RA_U8(g_RowPlayer(i), 0x34a) != 0 || RA_U8(g_RowPlayer(i), 0x37c) == 0)
				d = 1e7f;
			if (d < best) {
				second = best;
				secondIdx = bestIdx;
				best = d;
				bestIdx = i;
				if (count < 2)
					count++;
			} else if (d < second) {
				second = d;
				secondIdx = i;
				if (count < 2)
					count++;
			}
		}
	}
	for (i = 0; i < 4; i++)
		FUN_001abb70(g_RowPlayer(i), (i == bestIdx || i == secondIdx) ? 0 : 1, 0);
}

/* deletes a tweak menu */
static void ra_DeleteMenu(void *self, int off)
{
	void *menu = RA_PTRF(self, off);

	if (menu != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)menu);
		RA_DELETE(menu);
		RA_PTRF(self, off) = NULL;
	}
}

/* 0x001a2330  Rowing/CRowing_Game.cpp
The game mode left before loading finished: closes the packages and deletes the tweak
menus (rowing_b 0x001b6200). */
void CRowingGame_Abort(void *self)
{
	if (GAME_PTR(0x14) != NULL)
		bkClosePackageThunk((struct bkPackage *)GAME_PTR(0x14));
	if (GAME_PTR(0x1c) != NULL)
		bkClosePackageThunk((struct bkPackage *)GAME_PTR(0x1c));
	ra_DeleteMenu(self, 0x2f8);
	ra_DeleteMenu(self, 0x2fc);
	ra_DeleteMenu(self, 0x300);
	ra_DeleteMenu(self, 0x304);
}

/* deletes a particle system (Release, then the deleting destructor) */
static void ra_DeleteSystem(void *ps)
{
	RA_VFN(ps, 0x00, void (*)(void *))(ps);
	if (ps != NULL)
		RA_VFN(ps, 0x14, void (*)(void *, int))(ps, 1);
}

/* 0x001a23e0  Rowing/CRowing_Game.cpp
Shutdown (rowing_b 0x001b6200): restores the sound fall-off distances and deletes
everything Initialise and Begin created; closes the packages unless the game mode still
loads in the background. */
void CRowingGame_Shutdown(void *self)
{
	void *p;
	int i;

	DAT_0028ffa8 = GAME_F32(0x32c);
	DAT_0028ffac = GAME_F32(0x330);
	baFlushShaderCache();
	p = DAT_002f26c4;
	if (DAT_002f26c4 != NULL) {
		BCarFreeCamera_Destruct((struct BCarFreeCamera *)DAT_002f26c4);
		RA_DELETE(p);
	}
	RefBot_Delete();
	CLDebris_Start();
	Sprite_FreeDefs((struct CSpriteDef *)DAT_00290f00, 6);
	CTweakMenu_SaveAll();
	CLRumble_Shutdown(DAT_002ed324);
	if ((RA_U8(self, 0) & 0xc) == 0xc && GAME_PTR(0xb4) != NULL) {
		ra_DeleteSystem(GAME_PTR(0xb4));
		GAME_PTR(0xb4) = NULL;
		ra_DeleteSystem(GAME_PTR(0xb8));
		ra_DeleteSystem(GAME_PTR(0xbc));
	}
	if (GAME_PTR(0xb0) != NULL) {
		bkDeleteResource((struct bkResource *)GAME_PTR(0xb0));
		GAME_PTR(0xb0) = NULL;
	}
	p = GAME_PTR(0x8c);
	if (p != NULL) {
		FUN_001d5950();
		RA_DELETE(p);
		GAME_PTR(0x8c) = NULL;
	}
	if (GAME_PTR(0xac) != NULL) {
		baDeleteExtraSystem(*(struct BExtraSystem **)GAME_PTR(0xac));
		RA_DELETE(GAME_PTR(0xac));
		GAME_PTR(0xac) = NULL;
	}
	if (GAME_I32(0x308) != 0)
		CLHud_DeleteWinnerText(DAT_002ecfac);
	ra_DeleteMenu(self, 0x2f8);
	ra_DeleteMenu(self, 0x2fc);
	ra_DeleteMenu(self, 0x300);
	ra_DeleteMenu(self, 0x304);
	p = GAME_PTR(0xa4);
	if (p != NULL) {
		rwb_Lights_Dtor(p);
		RA_DELETE(p);
		GAME_PTR(0xa4) = NULL;
	}
	for (i = 0; i < 2; i++) {
		p = RA_PTRF(self, 0x30c + i * 4);
		if (p != NULL) {
			ra_DeleteSystem(p);
			RA_PTRF(self, 0x30c + i * 4) = NULL;
		}
	}
	if (GAME_PTR(0x9c) != NULL) {
		FUN_001d5950();
		if (GAME_PTR(0x9c) != NULL)
			RA_VFN(GAME_PTR(0x9c), 0x00, void (*)(void *, int))(GAME_PTR(0x9c), 1);
		GAME_PTR(0x9c) = NULL;
	}
	if (GAME_PTR(0x80) != NULL) {
		RA_VFN(GAME_PTR(0x80), 0x00, void (*)(void *, int))(GAME_PTR(0x80), 1);
		GAME_PTR(0x80) = NULL;
	}
	if (GAME_PTR(0x98) != NULL) {
		FUN_001af450(GAME_PTR(0x98));
		if (GAME_PTR(0x98) != NULL)
			RA_VFN(GAME_PTR(0x98), 0x00, void (*)(void *, int))(GAME_PTR(0x98), 1);
		GAME_PTR(0x98) = NULL;
	}
	if (GAME_PTR(0x270) != NULL) {
		rwb_Obj270_Dtor(GAME_PTR(0x270));
		if (GAME_PTR(0x270) != NULL)
			RA_VFN(GAME_PTR(0x270), 0x00, void (*)(void *, int))(GAME_PTR(0x270), 1);
		GAME_PTR(0x270) = NULL;
	}
	if (GAME_PTR(0x84) != NULL) {
		FUN_001d5950();
		p = GAME_PTR(0x84);
		if (p != NULL) {
			FUN_001d5950();
			RA_DELETE(p);
		}
		GAME_PTR(0x84) = NULL;
	}
	if (GAME_PTR(0x90) != NULL) {
		RowParticles_Free((struct RowParticles *)GAME_PTR(0x90));
		RA_DELETE(GAME_PTR(0x90));
		GAME_PTR(0x90) = NULL;
	}
	if (GAME_PTR(0x88) != NULL) {
		RowSprites_Free();
		p = GAME_PTR(0x88);
		if (p != NULL) {
			FUN_001d5950();
			RA_DELETE(p);
		}
		GAME_PTR(0x88) = NULL;
	}
	if (GAME_PTR(0x94) != NULL) {
		FUN_001af9a0(GAME_PTR(0x94));
		RA_DELETE(GAME_PTR(0x94));
		GAME_PTR(0x94) = NULL;
	}
	if (GAME_PTR(0xa0) != NULL) {
		CDemoTokenSystem_Shutdown(GAME_PTR(0xa0));
		RA_DELETE(GAME_PTR(0xa0));
		GAME_PTR(0xa0) = NULL;
	}
	for (i = 0; i < 4; i++) {
		p = g_RowPlayer(i);
		if (p != NULL) {
			FUN_001adbf0(p);
			p = g_RowPlayer(i);
			if (p != NULL) {
				FUN_0019d8d0(p);
				RA_DELETE(p);
			}
			g_RowPlayer(i) = NULL;
		}
	}
	if (GAME_PTR(0x7c) != NULL) {
		for (i = 0; i < RA_I32(GAME_PTR(0x7c), 0x268); i++) {
			p = RA_PTRF(self, 0x274 + i * 4);
			if (p != NULL) {
				rwb_Buoy_Dtor(p);
				RA_DELETE(p);
				RA_PTRF(self, 0x274 + i * 4) = NULL;
			}
		}
		rwb_World_Shutdown(GAME_PTR(0x7c));
		p = GAME_PTR(0x7c);
		if (p != NULL) {
			FUN_001d5950();
			RA_DELETE(p);
		}
		GAME_PTR(0x7c) = NULL;
	}
	FUN_001a5970();
	if (g_RowSound != NULL)
		rwb_Sound_Shutdown(g_RowSound);
	bkDeletePackageResources((uint)GAME_I32(0x18), 1);
	bkDeletePackageResources((uint)GAME_I32(0x20), 1);
	DebugCam_Shutdown();
	CProjector_DeleteAll();
	if (GAME_PTR(0x38) != NULL) {
		COverlay_DeleteAll();
		p = GAME_PTR(0x38);
		if (p != NULL) {
			FUN_001d5950();
			RA_DELETE(p);
		}
		GAME_PTR(0x38) = NULL;
	}
	p = GAME_PTR(0x40);
	if (p != NULL) {
		CSeagulls_Release((struct CSeagulls *)p);
		RA_DELETE(p);
		GAME_PTR(0x40) = NULL;
		bkDeleteResource((struct bkResource *)GAME_PTR(0x44));
	}
	if (RA_I32(GAME_PTR(0x78), 0x14) == 0) {
		if (GAME_PTR(0x14) != NULL)
			bkClosePackageThunk((struct bkPackage *)GAME_PTR(0x14));
		if (GAME_PTR(0x1c) != NULL)
			bkClosePackageThunk((struct bkPackage *)GAME_PTR(0x1c));
	}
}

/* steps an animated texture: FRAME (float, game +off) advances by the frame time and wraps
at the frame count (game +countOff) */
static void ra_AnimateTexture(void *self, int texOff, int countOff, int frameOff)
{
	struct bdTexture *tex = (struct bdTexture *)RA_PTRF(self, texOff);
	int frames;

	if (tex == NULL)
		return;
	frames = RA_I32(self, countOff);
	if (frames == 0)
		return;
	RA_F32(self, frameOff) = g_CurrentFrameTime(self) + RA_F32(self, frameOff);
	if ((int)RA_F32(self, frameOff) >= frames) {
		float f = (float)frames;

		do
			RA_F32(self, frameOff) = RA_F32(self, frameOff) - f;
		while ((int)RA_F32(self, frameOff) >= RA_I32(self, countOff));
	}
	bdTextureSetFrame(tex, (int)RA_F32(self, frameOff));
}

/* 0x001a2890  Rowing (missing from the table; CRowing_Game.cpp inferred)
The update (the game mode's Update slot via the thunk at 0x001b6130, TOP != 0): pause,
the frame clock, the colour set, the single-human camera, the boats, the camera, the
world, ambient sounds, the variant's animation and effects (water jets, buoys, steam,
waterfall, mines), the boat lights, the extras, the game state machine (0 intro, 1 start,
5 race, 6 results) and the water. */
void FUN_001a2890(void *self, int top)
{
	void *p, *obj;
	float pos[4], t, amount;
	int i, n, count, human, found, scores[4], drops;
	unsigned int r;

	if (top == 0)
		return;
	FUN_001af460(GAME_PTR(0x98));
	if (DAT_002ecdec != 0) {
		if ((RA_U8(self, 0) & 1) == 0) {
			CLIntroCam_Update(0.0f, 0);
			CLPause_Update((undefined4 *)DAT_002ecd90);
			return;
		}
		FUN_001aeb90(GAME_PTR(0x84), 0.0f);
		CLPause_Update((undefined4 *)DAT_002ecd90);
		return;
	}
	FUN_001af540(GAME_PTR(0x98), 0);
	CFire_Timer_End((struct CFire_Timer *)GAME_PTR(0x98), 0);
	FUN_001a57f0();
	FUN_001af680(GAME_PTR(0x98), 0xffffffff);
	if (DAT_00283e58 != 0)
		g_CurrentFrameTime(self) = g_CurrentFrameTime(self) * 1.5f;
	if (DAT_00283ea4 != GAME_I32(0x328)) {
		GAME_I32(0x328) = DAT_00283ea4;
		for (i = 0; i < DAT_002bae94; i++)
			Core_SetCharacterColours(RA_PTRF(g_RowPlayer(i), 0xc4), DAT_00283ea4);
	}
	GAME_I32(0xc0) = 0;
	if (DAT_00283ef0 != 0) {
		count = 0;
		human = -1;
		for (i = 0; i < DAT_002bae94; i++) {
			if (DAT_002baed8[i * 10] != 0) {
				count++;
				human = i;
			}
		}
		if (count == 1) {
			GAME_I32(0xc0) = count;
			GAME_I32(0xc4) = human;
		}
	}
	for (i = 0; i < GAME_I32(0x24); i++)
		FUN_001ac8f0(g_RowPlayer(i), g_CurrentFrameTime(self));
	FUN_001af680(GAME_PTR(0x98), 0xff0000ff);
	*(int *)GAME_PTR(0x84) = RW_VAR(int, 0x0030f0a4);
	CRowingGame_UpdateCamera(self);
	if (GAME_PTR(0xa0) != NULL)
		rwb_Tokens_Update(GAME_PTR(0xa0));
	rwb_World_Tick(GAME_PTR(0x7c), g_CurrentFrameTime(self));
	for (i = 0; i < 2; i++) {
		p = RA_PTRF(self, 0x30c + i * 4);
		if (p != NULL)
			RA_VFN(p, 0x04, void (*)(void *, float))(p, g_CurrentFrameTime(self));
	}
	if (GAME_PTR(0x40) != NULL)
		CSeagulls_Tick((struct CSeagulls *)GAME_PTR(0x40), g_CurrentFrameTime(self));
	/* ambient sounds: a random one of 0x16-0x18 about every 3 s, 0x19/0x1a now and then */
	r = (unsigned int)rw_rand();
	RW_VAR(int, 0x0030f404)--;
	if (r % 1000 < 12 || RW_VAR(int, 0x0030f404) < 1) {
		rwb_Sound_PlayRandom(rw_rand() % 3 + 0x16, 0x40);
		RW_VAR(int, 0x0030f404) = 0xb4;
	}
	r = (unsigned int)rw_rand();
	RW_VAR(int, 0x0030f400)++;
	if (r % 1000 < 6 && RW_VAR(int, 0x0030f400) >= 0x1c2) {
		rwb_Sound_PlayRandom((rw_rand() & 1) + 0x19, 0xff);
		RW_VAR(int, 0x0030f400) = 0;
	}
	switch (GAME_VARIANT) {
	case 0:
		ra_AnimateTexture(self, 0x258, 0x260, 0x268);
		/* the gull obstacle box follows the cargo lifter */
		if (RA_PTRF(GAME_PTR(0x7c), 0x2f4) != NULL && GAME_PTR(0x74) != NULL) {
			baGetNodePosition((struct BActorInstance *)RA_PTRF(RA_PTRF(GAME_PTR(0x7c), 0x2f4), 0xc),
				(struct BActorNode *)GAME_PTR(0x74), pos);
			GAME_F32(0x48) = pos[0] - 300.0f;
			GAME_F32(0x4c) = pos[1] - 300.0f;
			GAME_F32(0x50) = pos[2] - 300.0f;
			GAME_F32(0x58) = pos[0] + 300.0f;
			GAME_F32(0x5c) = pos[1] + 300.0f;
			GAME_F32(0x60) = pos[2] + 300.0f;
		}
		break;
	case 1:
		for (i = 0; i < RA_I32(GAME_PTR(0x7c), 0x268); i++)
			rwb_Buoy_Update(RA_PTRF(self, 0x274 + i * 4), g_CurrentFrameTime(self));
		break;
	case 2:
		ra_AnimateTexture(self, 0x258, 0x260, 0x268);
		CRowingGame_EmitSteam(self);
		break;
	case 3:
		/* the waterfall's two sounds wander in pitch between 0.5 and 2 */
		GAME_F32(0x320) = (float)rw_rand() * 3.05185097e-07f + GAME_F32(0x320);
		GAME_F32(0x324) = (float)rw_rand() * 3.05185097e-07f + GAME_F32(0x324);
		if (GAME_F32(0x320) > 2.0f)
			GAME_F32(0x320) = 2.0f;
		if (GAME_F32(0x324) > 2.0f)
			GAME_F32(0x324) = 2.0f;
		if (GAME_F32(0x320) < 0.5f)
			GAME_F32(0x320) = 0.5f;
		if (GAME_F32(0x324) < 0.5f)
			GAME_F32(0x324) = 0.5f;
		rwb_Sound_SetPitch(GAME_I32(0x318), 4, GAME_F32(0x320));
		rwb_Sound_SetPitch(GAME_I32(0x31c), 5, GAME_F32(0x324));
		ra_AnimateTexture(self, 0x258, 0x260, 0x268);
		ra_AnimateTexture(self, 0x25c, 0x264, 0x26c);
		/* spray at the foot and at the top of the waterfall */
		{
			float w = RW_VAR(float, 0x002b5630), c = RW_VAR(float, 0x002b5634);
			float x, z;

			x = (c - w * 0.5f) + (float)rw_rand() * (w - 40.0f) * rw_bitsf(0x38000100);
			z = -((float)rw_rand() * 0.00457777642f) - 40.0f;
			pos[0] = x;
			pos[1] = -40.0f;
			pos[2] = z;
			RowParticles_AddSteam((struct RowParticles *)GAME_PTR(0x90), pos, 40.0f,
				(float)rw_rand() * 0.00122074038f + 20.0f);
			x = (c - w * 0.5f) + (float)rw_rand() * (w + 40.0f) * rw_bitsf(0x38000100);
			pos[1] = (float)rw_rand() * 0.00915555283f - 200.0f;
			z = (-((float)rw_rand() * 0.00457777642f) - 1800.0f) - 40.0f;
			pos[0] = x;
			pos[2] = z;
			RowParticles_AddSteam((struct RowParticles *)GAME_PTR(0x90), pos, 40.0f,
				(float)rw_rand() * 0.000610370189f + 20.0f);
			for (i = 0; i < 2; i++) {
				w = RW_VAR(float, 0x002b5630);
				c = RW_VAR(float, 0x002b5634);
				pos[0] = (c - w * 0.5f) + (float)rw_rand() * (w + 30.0f) * rw_bitsf(0x38000100);
				pos[2] = (float)rw_rand() * 0.00183111057f - 1830.0f;
				pos[1] = 0.0f;
				obj = RowParticles_AddSteam((struct RowParticles *)GAME_PTR(0x90), pos, 30.0f,
					(float)rw_rand() * 0.000305185094f + 10.0f);
				if (obj != NULL)
					RA_F32(obj, 0x18) = RA_F32(obj, 0x18) * rw_bitsf(0x3eaaaaab);
			}
		}
		if (GAME_PTR(0x80) != NULL) {
			float base = RW_VAR(float, 0x002b5634) - RW_VAR(float, 0x002b5630) * 0.5f;

			r = (unsigned int)rw_rand();
			RA_VFN(GAME_PTR(0x80), 0x28, int (*)(void *, float, float, float, float, float))(GAME_PTR(0x80),
				(float)r * RW_VAR(float, 0x002b5630) * rw_bitsf(0x38000100) + base, 0.0f, -5.0f,
				32.44f, 62.44f);
		}
		/* a mine starts drifting over the waterfall every "mine time" */
		if (GAME_F32(0x314) < 0.0f) {
			GAME_I32(0x314) = RW_VAR(int, 0x002b549c);
			found = 0;
			for (i = 0; i < 10 && !found; i++) {
				obj = RA_PTRF(self, 0xc8 + (rw_rand() % 100) * 4);
				if ((RA_U8(obj, 0x18) & 1) == 0) {
					found = 1;
					rwb_Mine_Trigger(obj, 0.0f, GAME_F32(0x28), 1);
				}
			}
		} else {
			GAME_F32(0x314) = GAME_F32(0x314) - g_CurrentFrameTime(self);
		}
		RA_VFN(GAME_PTR(0xb4), 0x04, void (*)(void *, float))(GAME_PTR(0xb4), g_CurrentFrameTime(self));
		RA_VFN(GAME_PTR(0xb8), 0x04, void (*)(void *, float))(GAME_PTR(0xb8), g_CurrentFrameTime(self));
		RA_VFN(GAME_PTR(0xbc), 0x04, void (*)(void *, float))(GAME_PTR(0xbc), g_CurrentFrameTime(self));
		break;
	}
	CRowingGame_UpdateCloseups(self);
	FUN_001b10a0(GAME_PTR(0x8c), g_CurrentFrameTime(self));
	/* each boat's light floats 250 ahead of it and 150 up */
	if (GAME_PTR(0x2e4) != NULL) {
		for (i = 0; i < DAT_002bae94; i++) {
			p = g_RowPlayer(i);
			pos[0] = RA_F32(p, 0x2b4);
			pos[1] = RA_F32(p, 0x2b8);
			pos[2] = RA_F32(p, 0x2bc);
			pos[3] = RA_F32(p, 0x2c0);
			pos[0] = sinf(RA_F32(p, 0x2c8)) * 250.0f + pos[0];
			pos[1] = pos[1] + 150.0f;
			pos[2] = pos[2] - cosf(RA_F32(p, 0x2c8)) * 250.0f;
			rwb_Lights_SetPosition(GAME_PTR(0xa4), RA_PTRF(self, 0x2e4 + i * 4), pos);
			GAME_I32(0x2f4) = rw_rand() & 3;
		}
	}
	{
		void *cam = GAME_PTR(0x84);

		RA_F32(GAME_PTR(0xac), 4) = 1.57079625f -
			atan2f(RA_F32(cam, 0x14) - RA_F32(cam, 0x34), RA_F32(cam, 0x0c) - RA_F32(cam, 0x2c));
	}
	FUN_001af680(GAME_PTR(0x98), 0xffffffff);
	CProjector_UpdateAll();
	baUpdateExtras(NULL, (void *)(uintptr_t)rw_fbits(DAT_0028fda8 * 0.0166666675f));
	/* debug markers (empty functions in the release build) */
	p = g_RowPlayer(*(int *)GAME_PTR(0x8c));
	pos[0] = RA_F32(p, 0x2b4);
	pos[1] = RA_F32(p, 0x2b8);
	pos[2] = RA_F32(p, 0x2bc);
	pos[3] = RA_F32(p, 0x2c0);
	if (pos[1] < -100.0f)
		pos[1] = -100.0f;
	FUN_001d5950();
	if ((rw_rand() & 0x7fff) > 0x7e90) {
		p = g_RowPlayer(rw_rand() & 3);
		pos[0] = RA_F32(p, 0x2b4);
		pos[1] = RA_F32(p, 0x2b8);
		pos[2] = RA_F32(p, 0x2bc);
		pos[3] = RA_F32(p, 0x2c0);
		if (pos[1] < -100.0f)
			pos[1] = -100.0f;
		FUN_001d5950();
	}
	FUN_001d5950();
	RefBot_Update();
	FUN_001af680(GAME_PTR(0x98), 0xff808000);
	RowParticles_Update((struct RowParticles *)GAME_PTR(0x90));
	n = GAME_I32(0x08) + 1;
	GAME_I32(0x08) = n;
	GAME_F32(0x0c) = g_CurrentFrameTime(self) + GAME_F32(0x0c);
	switch (GAME_I32(0x04)) {
	case 0:
		if ((RA_U8(self, 0) & 0xc) == 0 && n == 1)
			rwb_Sound_Play(0x15, 0x80);
		break;
	case 1:
		if (DAT_002baeb8 == 0) {
			if (n == 1) {
				CLHud_StartBars((float *)(uintptr_t)DAT_002ecfac, 280.0f);
				break;
			}
			if (CLHud_UpdateCountdown(DAT_002ecfac) == 0)
				break;
		}
		for (i = 0; i < GAME_I32(0x24); i++)
			FUN_001a8940(g_RowPlayer(i), 0);
		GAME_I32(0x04) = 5;
		GAME_I32(0x08) = 0;
		GAME_I32(0x0c) = 0;
		break;
	case 5:
		switch (GAME_VARIANT) {
		case 2:
			GAME_F32(0x30) = RA_F32(GAME_PTR(0x98), 0x98) * 0.0303030312f * RW_VAR(float, 0x002b5648) + GAME_F32(0x30);
			if (GAME_F32(0x30) > RW_VAR(float, 0x002b564c))
				GAME_F32(0x30) = RW_VAR(float, 0x002b564c);
			GAME_F32(0x2c) = RA_F32(GAME_PTR(0x98), 0x98) * 0.0303030312f * RW_VAR(float, 0x002b5654) + GAME_F32(0x2c);
			if (GAME_F32(0x2c) > RW_VAR(float, 0x002b5658))
				GAME_F32(0x2c) = RW_VAR(float, 0x002b5658);
			break;
		case 3:
			GAME_F32(0x28) = RA_F32(GAME_PTR(0x98), 0x98) * 0.0303030312f * RW_VAR(float, 0x002b563c) + GAME_F32(0x28);
			if (GAME_F32(0x28) > RW_VAR(float, 0x002b5640))
				GAME_F32(0x28) = RW_VAR(float, 0x002b5640);
			break;
		}
		if (DebugCam_IsActive() == 0) {
			GAME_F32(0x34) = GAME_F32(0x34) - RA_F32(GAME_PTR(0x98), 0x98);
			if (GAME_F32(0x34) < 0.0f)
				GAME_I32(0x34) = 0;
		}
		GAME_I32(0x10) = FUN_0019dad0(self);
		if (GAME_I32(0x10) >= 0) {
			GAME_I32(0x04) = 6;
			GAME_I32(0x08) = 0;
			GAME_I32(0x0c) = 0;
			FUN_001ade80(GAME_PTR(0x84), -1);
			FUN_001b0e50(GAME_PTR(0x8c));
			if ((RA_U8(self, 0) & 0xc) == 8)
				FUN_001a5610(GAME_PTR(0x80), 0.0f);
			for (i = 0; i < GAME_I32(0x24); i++)
				FUN_001a8940(g_RowPlayer(i), RA_I32(g_RowPlayer(i), 0x3b8) != 0 ? 2 : 3);
			scores[0] = DAT_002baee8[0];
			scores[1] = DAT_002baee8[10];
			scores[2] = DAT_002baee8[20];
			scores[3] = DAT_002baee8[30];
			CLHud_CreateWinnerText(DAT_002ecfac, scores, 0);
			GAME_I32(0x308) = 1;
		}
		break;
	case 6:
		r = RA_U32(self, 0);
		if (r & 2) {
			if (DAT_002bae98 != 2) {
				CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, 1);
				return;
			}
			CGameModeStack_PopTo((struct CGameModeStack *)DAT_002ba750, "BGeffects", 1);
			RA_VFN(DAT_002ba97c, 0x0c, int (*)(void *, int))(DAT_002ba97c, (int)((r >> 2) & 3) + 1);
			return;
		}
		break;
	}
	if (GAME_PTR(0x80) != NULL) {
		drops = 0;
		amount = 0.0f;
		switch (GAME_VARIANT) {
		case 0:
			drops = 100;
			amount = 0.050000001f;
			break;
		case 1:
			drops = 0x50;
			amount = 0.050000001f;
			break;
		case 2:
			t = (RW_VAR(float, 0x002b5494) - GAME_F32(0x34) * 0.001f) / RW_VAR(float, 0x002b5494);
			if (GAME_I32(0x04) == 6)
				t = 0.0f;
			drops = 100;
			amount = 0.1f;
			CWaterField_ScrollUV((CWaterField *)GAME_PTR(0x80), 0.0f,
				(t + 1.0f) * g_CurrentFrameTime(self) * 0.00200000009f);
			FUN_001a5610(GAME_PTR(0x80), t);
			break;
		case 3:
			drops = 0x50;
			amount = 0.1f;
			RA_VFN(GAME_PTR(0x80), 0x10, void (*)(void *, float))(GAME_PTR(0x80), g_CurrentFrameTime(self) * 6.0f);
			CWaterField_ScrollUV((CWaterField *)GAME_PTR(0x80), 0.0f, g_CurrentFrameTime(self) * 0.0099999998f);
			break;
		}
		if (RW_VAR(int, 0x002b54ac) != 0) {
			RA_VFN(GAME_PTR(0x80), 0x0c, void (*)(void *))(GAME_PTR(0x80));
			for (; drops > 0; drops--)
				RA_VFN(GAME_PTR(0x80), 0x24, void (*)(void *, float))(GAME_PTR(0x80), amount);
		}
	}
	FUN_001ec260();
	if (GAME_PTR(0x270) != NULL)
		rwb_Obj270_Update(GAME_PTR(0x270), g_CurrentFrameTime(self));
	rwb_Sound_Update(g_RowSound);
	if (RW_VAR(int, 0x002b54a8) != 0)
		COverlay_UpdateAll();
	DebugCam_Update();
	FUN_001d5950();
	FUN_001af680(GAME_PTR(0x98), 0xff808080);
	if (RW_VAR(int, 0x0030f09c) != RA_I32(GAME_PTR(0x98), 0x234))
		RA_I32(GAME_PTR(0x98), 0x234) = RW_VAR(int, 0x0030f09c);
	FUN_001af680(GAME_PTR(0x98), 0xff404040);
	if (FUN_001ae420(GAME_PTR(0x84)) != 0)
		CLPause_Update((undefined4 *)DAT_002ecd90);
}

/* 0x001a289c  Rowing/CRowing_Game.cpp (table entry inside FUN_001a2890)
Not a function start: 0x001a289c is the "push esi" in the prologue of the update
0x001a2890 (Ghidra made a function at a branch target). Nothing calls it; it is kept as
an empty definition so the table entry has a body. */
void FUN_001a289c(void)
{
}

/* ======================================================================
   The water of variant 3 (the whirlpool) and of variant 4 (the waterfall): CWaterField
   subclasses (vtables 0x00274f60 and 0x00274f8c above).

   Whirlpool (0x92a8 bytes): +0x9090 the cell positions (x, y, z per cell, 12 bytes),
   +0x9094 and +0x9098 the cos and sin of each cell's spiral angle, +0x909c 1.0,
   +0x90a0 the radius R, +0x90a4 the depth D, +0x90a8.. the height of each row
   (H[k] = *(float *)(self + 0x90a4 + 4k), k = 1..nz).
   Waterfall (0x90b0 bytes): +0x9090, +0x9094, +0x9098 three texture phases; while drawing,
   +0x909c/+0x90a0 the left/right edge, +0x90a4 the v step, +0x90a8/+0x90ac the last point.
   ====================================================================== */

#define WATER_F32(off) RA_F32(self, off)
#define WATER_H(k) RA_F32(self, 0x90a4 + (k) * 4)

/* a water vertex (FVF 0x152) */
static void ra_SetNormal(CWaterVertex *v, CWaterField *w, float a, float b)
{
	int ix = (int)(a * w->p.normalScale) + 0x10;
	int iz;

	if (ix < 0)
		ix = 0;
	else if (ix > 0x1f)
		ix = 0x1f;
	iz = (int)(b * w->p.normalScale) + 0x10;
	if (iz < 0)
		iz = 0;
	else if (iz > 0x1f)
		iz = 0x1f;
	ix = iz * 0x20 + ix;
	v->normal[0] = w->normals[ix][0];
	v->normal[1] = w->normals[ix][1];
	v->normal[2] = w->normals[ix][2];
}

/* the slope of a whirlpool cell turned into the spiral's frame (C, S: the cell's cos, sin) */
static void ra_SpiralSlope(float dx, float dz, float c, float s, float *a, float *b)
{
	*a = dx * s - dz * c;
	*b = -(dx * c) - dz * s;
}

/* 0x001a3b00  Rowing/CRowing_Game.cpp (vtable 0x00274f60 slot 6)
Whirlpool UpdateVertices: every vertex gets its row's height (the outer rows stay
flat); the inner rows get texture coordinates from the ripple slopes and a normal from the
slope turned into the spiral's frame (the rows wrap left to right). */
int FUN_001a3b00(void *self)
{
	CWaterField *w = (CWaterField *)self;
	CWaterVertex *v;
	float *c, *cosA, *sinA, *hp;
	float u, vv, rowV, y, dH, dx, dz, a, b;
	int nx, row, col, i;

	v = (CWaterVertex *)bdVertexBufferLock((struct bdVertexBuffer *)(uintptr_t)w->vb, 0);
	vv = w->vOffset;
	c = &w->cells[0][0];
	nx = w->p.nx;
	y = WATER_H(w->p.nz);
	for (i = 0; i < nx; i++, v++)
		v->pos[1] = y;
	c += nx * 2;
	cosA = (float *)RA_PTRF(self, 0x9094) + nx;
	sinA = (float *)RA_PTRF(self, 0x9098) + nx;
	hp = &WATER_H(w->p.nz - 2);
	for (row = w->p.nz - 2; row != 0; row--, hp--) {
		y = hp[1];
		dH = hp[0] - hp[2];
		u = w->uOffset;
		rowV = vv;
		vv = vv + w->unknown906c;
		for (col = 0; col < nx; col++) {
			if (col == 0)
				dx = c[2] - c[nx * 2 - 2];
			else if (col == nx - 1)
				dx = c[2 - nx * 2] - c[-2];
			else
				dx = c[2] - c[-2];
			v->pos[1] = y;
			dz = c[nx * 2] - c[-nx * 2];
			v->uv[0] = dx * w->p.slopeUV + u;
			v->uv[1] = dz * w->p.slopeUV + rowV;
			ra_SpiralSlope(dx, dz + dH, *cosA, *sinA, &a, &b);
			ra_SetNormal(v, w, a, b);
			u = u + w->uvStep;
			v++;
			c += 2;
			cosA++;
			sinA++;
		}
	}
	y = WATER_H(1);
	for (i = 0; i < nx; i++)
		v[i].pos[1] = y;
	bdVertexBufferUnlock((struct bdVertexBuffer *)(uintptr_t)w->vb);
	return 1;
}

/* 0x001a3ee0  Rowing/CRowing_Game.cpp
Finds the whirlpool cell nearest to (X, Z) by halving steps through the spiral grid.
(The fourth candidate uses the same cell as the third, as in the original.) */
void FUN_001a3ee0(void *self, float x, float z, int *col, int *row)
{
	CWaterField *w = (CWaterField *)self;
	float *pos = (float *)RA_PTRF(self, 0x9090);
	float d[4], *p;
	int c, r, nx, base, i;

	*col = w->p.nx / 2;
	*row = w->p.nz / 2;
	c = *col / 2;
	r = *row / 2;
	while (c > 0 && r > 0) {
		nx = w->p.nx;
		base = (*row - r) * nx;
		p = pos + (base - c + *col) * 3;
		d[0] = (p[0] - x) * (p[0] - x) + (p[2] - z) * (p[2] - z);
		p = pos + (base + *col + c) * 3;
		d[1] = (p[0] - x) * (p[0] - x) + (p[2] - z) * (p[2] - z);
		i = *col + ((*row + r) * nx - c);
		p = pos + i * 3;
		d[2] = (p[0] - x) * (p[0] - x) + (p[2] - z) * (p[2] - z);
		d[3] = d[2];
		if (d[0] <= d[1] && d[0] <= d[2] && d[0] <= d[3]) {
			*col -= c;
			if (*col < 0)
				*col = 0;
			*row -= r;
			if (*row < 0)
				*row = 0;
		} else if (d[1] <= d[0] && d[1] <= d[2] && d[1] <= d[3]) {
			*col += c;
			if (*col >= w->p.nx)
				*col = w->p.nx - 1;
			*row -= r;
			if (*row < 0)
				*row = 0;
		} else if (d[2] <= d[0] && d[2] <= d[1] && d[2] <= d[3]) {
			*col -= c;
			if (*col < 0)
				*col = 0;
			*row += r;
			if (*row >= w->p.nz)
				*row = w->p.nz - 1;
		} else {
			*col += c;
			if (*col >= w->p.nx)
				*col = w->p.nx - 1;
			*row += r;
			if (*row >= w->p.nz)
				*row = w->p.nz - 1;
		}
		c /= 2;
		r /= 2;
	}
}

/* 0x001a4170  Rowing/CRowing_Game.cpp (vtable 0x00274f8c slot 1)
Waterfall Init: the surface 5 lower and flat, then CWaterField's Init; the texture phases
start at 0. */
int FUN_001a4170(void *self, CWaterParams *params)
{
	int result;

	params->y = params->y - 5.0f;
	params->heightScale = 0.0f;
	result = CWaterField_Init((CWaterField *)self, params);
	WATER_F32(0x9098) = 0.0f;
	WATER_F32(0x9094) = 0.0f;
	WATER_F32(0x9090) = 0.0f;
	return result;
}

/* the waterfall's texture phases wrap at 1 */
static float ra_WrapPhase(float phase)
{
	if (phase > 1.0f) {
		do
			phase = phase - 1.0f;
		while (phase > 1.0f);
	}
	return phase;
}

/* 0x001a41b0  Rowing/CRowing_Game.cpp (vtable 0x00274f8c slot 3)
Waterfall Ripple: CWaterField's ripple, and the three texture phases move on. */
void FUN_001a41b0(void *self)
{
	float a, b, c;

	CWaterField_Ripple((CWaterField *)self);
	a = WATER_F32(0x9090) + 0.0099999998f;
	WATER_F32(0x9090) = a;
	b = WATER_F32(0x9094) + 0.013f;
	WATER_F32(0x9094) = b;
	c = WATER_F32(0x9098) + 0.017f;
	WATER_F32(0x9098) = c;
	if (a > 1.0f)
		WATER_F32(0x9090) = ra_WrapPhase(WATER_F32(0x9090));
	if (b > 1.0f)
		WATER_F32(0x9094) = ra_WrapPhase(WATER_F32(0x9094));
	if (c > 1.0f)
		WATER_F32(0x9098) = ra_WrapPhase(WATER_F32(0x9098));
}

/* 0x001a4290  Rowing/CRowing_Game.cpp
Draws one quad of the waterfall (corners (X0|X1, Y0, Z0) coloured C0 and (X0|X1, Y1, Z1)
coloured C1) with texture coordinates from the positions (U0 + x / V0 + (y + z) * VSCALE),
blended unless both colours are opaque. */
int FUN_001a4290(void *self, CWaterVertex *v, float u0, float v0, float vScale, float x0, float y0, float z0,
	float x1, float y1, float z1, unsigned int c0, unsigned int c1)
{
	float k;
	int i;

	if ((c0 & c1 & 0xff000000u) == 0xff000000u) {
		bdSetRenderState(6, 0, 0);
	} else {
		bdSetRenderState(10, 0, 0);
		bdSetRenderState(6, 1, 0);
	}
	v[0].pos[0] = x0;
	v[0].pos[1] = y0;
	v[0].pos[2] = z0;
	v[1].pos[0] = x1;
	v[1].pos[1] = y0;
	v[1].pos[2] = z0;
	v[2].pos[0] = x0;
	v[2].pos[1] = y1;
	v[2].pos[2] = z1;
	v[3].pos[0] = x1;
	v[3].pos[1] = y1;
	v[3].pos[2] = z1;
	v[0].colour = c0;
	v[1].colour = c0;
	v[3].colour = c1;
	v[2].colour = c1;
	for (i = 0; i < 4; i++) {
		v[i].uv[0] = (v[i].pos[0] - x0) * WATER_F32(0x9070) + u0;
		k = WATER_F32(0x9070);
		v[i].normal[0] = 0.0f;
		v[i].normal[1] = 1.0f;
		v[i].normal[2] = 0.0f;
		v[i].uv[1] = (((v[i].pos[2] - z0) + v[i].pos[1]) - y0) * k * vScale + v0;
	}
	bdDrawPrimitiveFVF(4, v, 4, 0x152);
	return 1;
}

/* 0x001a43b0  Rowing/CRowing_Game.cpp (vtable 0x00274f8c slot 8)
The waterfall draws no edges. */
int FUN_001a43b0(void *self, float height, int blend)
{
	(void)self;
	(void)height;
	(void)blend;
	return 1;
}

/* 0x001a43c0  Rowing/CRowing_Game.cpp
The whirlpool's destructor: its three arrays, then CWaterField's destructor (called twice
in the original: once by the class, once by the compiler's base destructor call). */
void FUN_001a43c0(void *self)
{
	*(const void *const **)self = vtable_00274f60;
	if (RA_PTRF(self, 0x9090) != NULL) {
		RA_DELETE(RA_PTRF(self, 0x9090));
		RA_PTRF(self, 0x9090) = NULL;
	}
	if (RA_PTRF(self, 0x9094) != NULL) {
		RA_DELETE(RA_PTRF(self, 0x9094));
		RA_PTRF(self, 0x9094) = NULL;
	}
	if (RA_PTRF(self, 0x9098) != NULL) {
		RA_DELETE(RA_PTRF(self, 0x9098));
		RA_PTRF(self, 0x9098) = NULL;
	}
	CWaterField_Destruct((CWaterField *)self);
	CWaterField_Destruct((CWaterField *)self);
}

/* 0x001a4460  Rowing/CRowing_Game.cpp (vtable 0x00274f60 slot 5)
Whirlpool BuildVertices: every vertex at its spiral position with its row's height; texture
coordinates and the turned normal like UpdateVertices (fixed cells: an upward normal).
The flags give the wrap: 2 left edge, 3 right edge. */
int FUN_001a4460(void *self)
{
	CWaterField *w = (CWaterField *)self;
	CWaterVertex *v;
	unsigned char *flag = w->flags;
	float *p = (float *)RA_PTRF(self, 0x9090);
	float *c, *sinA, *cosA, *hp;
	float rowU, rowV, u, vv, y, dH, dx, dz, a, b;
	int rows, col, nx;

	v = (CWaterVertex *)bdVertexBufferLock((struct bdVertexBuffer *)(uintptr_t)w->vb, 0);
	sinA = (float *)RA_PTRF(self, 0x9098);
	cosA = (float *)RA_PTRF(self, 0x9094);
	c = &w->cells[0][0];
	rowV = w->vOffset;
	rowU = w->uOffset;
	hp = (float *)RA_P(self, 0x90a0 + w->p.nz * 4);
	for (rows = w->p.nz; rows != 0; rows--, hp--) {
		y = hp[1];
		if (rows == w->p.nz || rows == 1)
			dH = 0.0f;
		else
			dH = hp[0] - hp[2];
		u = rowU;
		vv = rowV;
		rowU = rowU + RA_F32(self, 0x9064);
		rowV = rowV + w->unknown906c;
		nx = w->p.nx;
		for (col = nx; col != 0; col--) {
			switch ((signed char)*flag) {
			case 0:
				dx = c[2] - c[-2];
				goto slope;
			case 2:
				dx = c[2] - c[w->p.nx * 2 - 2];
				goto slope;
			case 3:
				dx = c[2 - w->p.nx * 2] - c[-2];
			slope:
				dz = c[w->p.nx * 2] - c[-w->p.nx * 2];
				v->colour = w->p.colour;
				v->pos[0] = p[0];
				v->pos[1] = w->p.heightScale * c[0] + y;
				v->pos[2] = p[2];
				ra_SpiralSlope(dx, dz + dH, *cosA, *sinA, &a, &b);
				v->uv[0] = dx * w->p.slopeUV + u;
				v->uv[1] = dz * w->p.slopeUV + vv;
				ra_SetNormal(v, w, a, b);
				break;
			case 1:
				v->pos[0] = p[0];
				v->pos[1] = w->p.heightScale * c[0] + y;
				v->pos[2] = p[2];
				v->uv[0] = u;
				v->colour = w->p.colour;
				v->uv[1] = vv;
				v->normal[0] = 0.0f;
				v->normal[1] = 1.0f;
				v->normal[2] = 0.0f;
				break;
			default:
				break;
			}
			u = u + w->uvStep;
			v++;
			c += 2;
			vv = vv + RA_F32(self, 0x9068);
			flag++;
			sinA++;
			p += 3;
			cosA++;
		}
	}
	bdVertexBufferUnlock((struct bdVertexBuffer *)(uintptr_t)w->vb);
	return 1;
}

/* 0x001a4700  Rowing/CRowing_Game.cpp (vtable 0x00274f60 slot 8)
Whirlpool DrawEdges: a skirt of quads (6-vertex strips, built in the object's edge buffer
at +0x48) from the outer ring out to radius R + 500 and then to HEIGHT, the texture
wrapping. BLEND is not used. */
int FUN_001a4700(void *self, float height, int blend)
{
	CWaterField *w = (CWaterField *)self;
	CWaterVertex *verts = (CWaterVertex *)RA_P(self, 0x48);
	float X[6], Y[6], Z[6], U[6], V[6];
	unsigned int C[6];
	float inv, outer, u, du, v0, ax, bx, az, bz;
	float *pa, *pb;
	int nx, i, k, base;

	(void)blend;
	outer = w->p.sizeX + 500.0f;
	nx = w->p.nx;
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)0xa, 1);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
	u = w->uOffset;
	du = (float)nx * w->uvStep / (float)w->p.nx;
	v0 = w->vOffset;
	if (nx <= 0)
		return 1;
	inv = 1.0f / w->p.sizeX;
	V[0] = v0;
	V[1] = v0;
	for (i = 0; i < nx; i++) {
		Y[2] = Y[3] = Y[4] = Y[5] = w->p.y;
		base = (w->p.nz - 1) * w->p.nx;
		pa = (float *)RA_PTRF(self, 0x9090) + (base + i) * 3;
		pb = (float *)RA_PTRF(self, 0x9090) + (base + (i + 1) % w->p.nx) * 3;
		ax = inv * pa[0];
		X[0] = pa[0];
		X[2] = outer * ax;
		Z[0] = pa[2];
		Y[0] = pa[1];
		bx = inv * pb[0];
		X[1] = pb[0];
		X[3] = outer * bx;
		Z[1] = pb[2];
		Y[1] = pb[1];
		X[4] = ax * height;
		X[5] = bx * height;
		az = inv * pa[2];
		Z[2] = outer * az;
		bz = inv * pb[2];
		Z[3] = outer * bz;
		Z[4] = az * height;
		Z[5] = bz * height;
		U[0] = u;
		U[1] = u + du;
		U[2] = u;
		U[3] = u + du;
		U[4] = u;
		U[5] = u + du;
		V[2] = w->unknown906c * 50.0f + v0;
		V[3] = V[2];
		V[4] = w->unknown906c * 200.0f + v0;
		V[5] = V[4];
		C[0] = w->p.colour;
		C[1] = w->p.colour;
		C[2] = C[3] = C[4] = C[5] = w->p.colour & 0xff000000u;
		for (k = 0; k < 6; k++) {
			verts[k].pos[0] = X[k] + w->p.x0;
			verts[k].pos[1] = Y[k];
			verts[k].pos[2] = Z[k] + w->p.z0;
			verts[k].normal[0] = 0.0f;
			verts[k].normal[1] = 1.0f;
			verts[k].normal[2] = 0.0f;
			verts[k].colour = C[k];
			verts[k].uv[0] = U[k];
			verts[k].uv[1] = V[k];
		}
		bdDrawPrimitiveFVF(4, verts, 6, 0x152);
		verts += 6;
		u = u + du;
	}
	return 1;
}

/* 0x001a49b0  Rowing/CRowing_Game.cpp (vtable 0x00274f60 slot 10)
Whirlpool Splash: pushes the cells near (X, Z) down to DEPTH inside INNER, fading out to
OUTER (a 20 x 20 window of the grid, wrapping left to right). */
int FUN_001a49b0(void *self, float x, float z, float depth, float inner, float outer)
{
	CWaterField *w = (CWaterField *)self;
	float *p, d2, h, outer2;
	int col, row, r, rEnd, c, cc;

	FUN_001a3ee0(self, x, z, &col, &row);
	r = row - 0x14;
	if (r < 0)
		r = 0;
	for (rEnd = r + 0x14; r < rEnd; r++) {
		if (r >= w->p.nz)
			return 1;
		outer2 = outer * outer;
		for (c = col - 10; c < col + 10; c++) {
			cc = c;
			if (c < 0)
				cc = w->p.nx + c;
			if (cc >= w->p.nx)
				cc -= w->p.nx;
			p = (float *)RA_PTRF(self, 0x9090) + (w->p.nx * r + cc) * 3;
			d2 = (p[0] - x) * (p[0] - x) + (p[2] - z) * (p[2] - z);
			if (!(d2 < outer2))
				continue;
			if (inner * inner <= d2)
				h = (1.0f - (bmFastSqrt(d2) - inner) / (outer - inner)) * depth;
			else
				h = depth;
			if (h < w->cells[w->p.nx * r + cc][0]) {
				w->cells[w->p.nx * r + cc][0] = h;
				w->cells[w->p.nx * r + cc][1] = 0.0f;
			}
		}
	}
	return 1;
}

/* the whirlpool's surface height at distance D from the centre */
static float ra_WhirlHeight(void *self, float d)
{
	float s;

	if (d < WATER_F32(0x90a0)) {
		s = sinf(d / WATER_F32(0x90a0) * 1.57079625f) - 1.0f;
		return s * WATER_F32(0x90a4) * s + WATER_F32(0x8);
	}
	return WATER_F32(0x8);
}

/* 0x001a4b10  Rowing/CRowing_Game.cpp
The whirlpool's surface height at (X, Z) (the centre is the origin). */
float FUN_001a4b10(void *self, float x, float z)
{
	return ra_WhirlHeight(self, bmFastSqrt(z * z + x * x));
}

/* 0x001a4b70  Rowing/CRowing_Game.cpp
The whirlpool's height at (X, Z) and its surface normal (NORMAL). */
float FUN_001a4b70(void *self, float x, float z, float *normal)
{
	float d, height, h1, h2, slope, dir;

	d = bmFastSqrt(z * z + x * x);
	if (!(d < WATER_F32(0x90a0))) {
		normal[0] = 0.0f;
		normal[1] = 1.0f;
		normal[2] = 0.0f;
		return WATER_F32(0x8);
	}
	height = ra_WhirlHeight(self, d);
	h1 = FUN_001a4b10(self, d - 0.0099999998f, 0.0f);
	h2 = FUN_001a4b10(self, d + 0.0099999998f, 0.0f);
	slope = (float)atan2((double)(h2 - h1), (double)0.02f);
	dir = atan2f(x, z);
	normal[1] = cosf(slope);
	normal[0] = sinf(dir) * sinf(slope);
	normal[2] = cosf(dir) * sinf(slope);
	return height;
}

/* 0x001a4c70  Rowing/CRowing_Game.cpp
Emits one pair of waterfall vertices at height/depth (H, Z) across the edges
(+0x909c, +0x90a0) facing ANGLE, and advances *CURSOR; *VPOS (the running v coordinate)
moves by the distance from the last point times +0x90a4. */
void FUN_001a4c70(void *self, unsigned char **cursor, float h, float z, float angle, float u, float *vpos,
	unsigned int colour)
{
	CWaterVertex *v = (CWaterVertex *)*cursor;
	float dx = h - WATER_F32(0x90a8), dz = z - WATER_F32(0x90ac);

	*vpos = *vpos - bmFastSqrt(dz * dz + dx * dx) * WATER_F32(0x90a4);
	WATER_F32(0x90a8) = h;
	WATER_F32(0x90ac) = z;
	v[0].pos[0] = WATER_F32(0x909c);
	v[1].pos[0] = WATER_F32(0x90a0);
	v[1].pos[1] = h;
	v[0].pos[1] = h;
	v[1].pos[2] = z;
	v[0].pos[2] = z;
	v[1].colour = colour;
	v[0].colour = colour;
	v[0].uv[0] = u;
	v[1].uv[0] = (WATER_F32(0x90a0) - WATER_F32(0x909c)) * WATER_F32(0x9070) + u;
	v[1].uv[1] = *vpos;
	v[0].uv[1] = v[1].uv[1];
	v[1].normal[0] = 0.0f;
	v[0].normal[0] = 0.0f;
	v[1].normal[1] = cosf(angle);
	v[0].normal[1] = v[1].normal[1];
	v[1].normal[2] = -sinf(angle);
	v[0].normal[2] = v[1].normal[2];
	*cursor += 0x48;
}

/* 0x001a4d80  Rowing/CRowing_Game.cpp
The waterfall's top: (unless CONTINUED) a flat run from z 2000 to the lip at height
H + 300, the rounded lip (a quarter circle of radius H + 60 around (240, 60)) and the
drop to (water height, -H). */
void FUN_001a4d80(void *self, unsigned char **cursor, unsigned int colour, float h, float u, float vpos,
	int continued)
{
	float r = h + 60.0f, top = h + 300.0f, a;

	WATER_F32(0x90a4) = WATER_F32(0x9070);
	WATER_F32(0x90a8) = top;
	WATER_F32(0x90ac) = 2000.0f;
	if (continued == 0) {
		FUN_001a4c70(self, cursor, top, 2000.0f, 0.0f, u, &vpos, colour);
		FUN_001a4c70(self, cursor, top, r, 0.0f, u, &vpos, colour);
	} else {
		WATER_F32(0x90a8) = top;
		WATER_F32(0x90ac) = r;
	}
	a = 0.0f;
	do {
		WATER_F32(0x90a4) = WATER_F32(0x90a4) * 0.89999998f;
		FUN_001a4c70(self, cursor, cosf(a) * r + 240.0f, 60.0f - sinf(a) * r, -a, u, &vpos, colour);
		a = a + 0.39269906f;
	} while (a < 1.57079625f);
	FUN_001a4c70(self, cursor, WATER_F32(0x8), -h, -1.57079625f, u, &vpos, colour);
}

/* 0x001a4ed0  Rowing/CRowing_Game.cpp
The waterfall's foot: a quarter circle of radius H + 60 from the water height down to
z -1740, its alpha rising by 8 per step, and (unless CONTINUED) the run out to
(-2000, -1800 - H). */
void FUN_001a4ed0(void *self, unsigned char **cursor, unsigned int colour, float h, float u, float vpos,
	int continued)
{
	float r = h + 60.0f, x, z, a;
	unsigned int alpha = colour >> 24;

	WATER_F32(0x90a8) = h + WATER_F32(0x8);
	WATER_F32(0x90a4) = WATER_F32(0x9070);
	WATER_F32(0x90ac) = -1740.0f;
	a = 0.0f;
	do {
		x = cosf(a) * r + (WATER_F32(0x8) - 60.0f);
		z = -1740.0f - sinf(a) * r;
		WATER_F32(0x90a4) = WATER_F32(0x90a4) * 0.89999998f;
		if (alpha != 0xff) {
			alpha += 8;
			if (alpha > 0xff)
				alpha = 0xff;
			colour = (colour & 0xffffffu) | (alpha << 24);
		}
		FUN_001a4c70(self, cursor, x, z, -a, u, &vpos, colour);
		a = a + 0.39269906f;
	} while (a < 1.57079625f);
	if (continued == 0)
		FUN_001a4c70(self, cursor, -2000.0f, -1800.0f - h, -1.57079625f, u, &vpos, colour);
}

/* the vertex count from the start of a buffer to the cursor */
#define RA_VERTS_FROM(start, cursor) ((int)((cursor) - (unsigned char *)(start)) / 0x24)

/* 0x001a5010  Rowing/CRowing_Game.cpp (vtable 0x00274f8c slot 7)
Waterfall Draw: the falling sheet in two layers, the pool (CWaterField's Draw), two foam
quads and the foot in 20 strips plus a translucent layer. BLEND is not used. */
int FUN_001a5010(void *self, int blend)
{
	CWaterField *w = (CWaterField *)self;
	unsigned char *cursor;
	float x0, span, y;
	int i;

	(void)blend;
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)0xa, 1);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
	WATER_F32(0x909c) = (RW_VAR(float, 0x002b5634) - RW_VAR(float, 0x002b5630) * 0.5f) - 10.0f;
	WATER_F32(0x90a0) = RW_VAR(float, 0x002b5630) * 0.5f + RW_VAR(float, 0x002b5634) + 10.0f;
	bdSetRenderState(5, 1, 0);
	bdSetTexture(0, (struct bdTexture *)(uintptr_t)w->texture);
	bdSetRenderState(2, 3, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(3, 0, 0);
	cursor = (unsigned char *)RA_P(self, 0x48);
	FUN_001a4d80(self, &cursor, 0xff606060u, 0.0f, 0.0f, WATER_F32(0x9090), 0);
	bdDrawPrimitiveFVF(4, RA_P(self, 0x48), RA_VERTS_FROM(RA_P(self, 0x48), cursor), 0x152);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	cursor = (unsigned char *)RA_P(self, 0x3a8);
	FUN_001a4d80(self, &cursor, 0x40606060u, 4.0f, 5.0f, WATER_F32(0x9094), 0);
	bdDrawPrimitiveFVF(4, RA_P(self, 0x3a8), RA_VERTS_FROM(RA_P(self, 0x3a8), cursor), 0x152);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(6, 0, 0);
	CWaterField_Draw(w, 0);
	bdSetTexture(0, (struct bdTexture *)(uintptr_t)w->texture);
	bdSetRenderState(2, 3, 0);
	bdSetRenderState(3, 0, 0);
	y = WATER_F32(0x8) + 1.66666663f;
	FUN_001a4290(self, (CWaterVertex *)RA_P(self, 0x708), 0.5f, WATER_F32(0x9090), 1.0f, WATER_F32(0x909c), y,
		-900.0f, WATER_F32(0x90a0), y, 100.0f, 0x606060u, 0x60606060u);
	y = WATER_F32(0x8) + 3.33333325f;
	FUN_001a4290(self, (CWaterVertex *)RA_P(self, 0x798), 0.25f, WATER_F32(0x9094), 1.0f, WATER_F32(0x909c), y,
		-650.0f, WATER_F32(0x90a0), y, 100.0f, 0x606060u, 0x60606060u);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(3, 1, 0);
	x0 = WATER_F32(0x909c);
	span = WATER_F32(0x90a0) - x0;
	for (i = 0; i < 0x14; i++) {
		WATER_F32(0x909c) = (float)i * span * 0.050000001f + x0;
		WATER_F32(0x90a0) = (float)(i + 1) * span * 0.050000001f + x0;
		cursor = (unsigned char *)RA_P(self, 0x8b8);
		/* the original passes no CONTINUED argument here: the callee reads the caller's
		saved ESI (the object, never 0) */
		FUN_001a4ed0(self, &cursor, w->p.colour, 0.0f,
			(WATER_F32(0x909c) - w->p.x0) * WATER_F32(0x9070) + w->uOffset,
			(-1740.0f - w->p.z0) * WATER_F32(0x9070) + w->vOffset, 1);
		bdDrawPrimitiveFVF(4, RA_P(self, 0x8b8), RA_VERTS_FROM(RA_P(self, 0x8b8), cursor), 0x152);
	}
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	cursor = (unsigned char *)RA_P(self, 0xc18);
	FUN_001a4ed0(self, &cursor, w->p.colour & 0xffffffu, 4.0f, 5.0f, WATER_F32(0x9094), 0);
	bdDrawPrimitiveFVF(4, RA_P(self, 0xc18), RA_VERTS_FROM(RA_P(self, 0xc18), cursor), 0x152);
	bdSetRenderState(6, 0, 0);
	return 1;
}
