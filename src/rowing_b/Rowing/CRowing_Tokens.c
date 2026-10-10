/*
Rowing/CRowing_Tokens.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_Tokens.cpp

The collectable tokens of the Rowing layout 2 (the classlib_fx/demolition_d CToken system):
they appear at random on the open water (200 to 1000 from the centre, 100 apart), float on
the waves and face the water's normal.

0x001b57d0 is tagged with CRowing_Tokens.cpp; 0x001b5730-0x001b5a00 work with the tokens
(file inferred).
*/

#include "rowing_b.h"

/* 0x002b598c  at most this many tokens; 0x002b5990  the chance per frame (0-1) that a
   token appears is 1 - this */
#define g_RowTokenMax RW_VAR(int, 0x002b598c)
#define g_RowTokenChance RW_VAR(float, 0x002b5990)

/* the token float height (classlib_fx) */
extern float DAT_00298d48;

/* the water height and normal at (x, z) (rowing_a 0x001a4b70, thiscall on the water) */
#define rb_WaterHeight(water, x, z, normal) \
	RB_CALL(FUN_001a4b70, float (*)(void *, float, float, float *))((water), (x), (z), (normal))

/* 0x001b5730  Rowing (gap; CRowing_Tokens.cpp inferred)
Loads the token assets (shadows on). */
void RowTokens_LoadAssets(int unused)
{
	(void)unused;
	CToken_LoadAssets(1, NULL);
}

/* 0x001b5740  Rowing (gap; CRowing_Tokens.cpp inferred)
The nearest collectable token within radius + 40 of (x, z), or NULL. */
struct CToken *RowTokens_FindNear(float x, float z, float radius)
{
	float pos[4];

	pos[0] = x;
	pos[2] = z;
	pos[1] = 0.0f;
	pos[3] = 1.0f;
	return CToken_FindNearest(pos, radius + 40.0f, 3, 1);
}

/* 0x001b5790  Rowing (gap; CRowing_Tokens.cpp inferred) */
void RowTokens_DrawGlow(void)
{
	if ((g_RowVariant & 0xc) == 8)
		CToken_DrawGlowAll();
}

/* 0x001b57b0  Rowing (gap; CRowing_Tokens.cpp inferred) */
void RowTokens_DrawShadows(void)
{
	if ((g_RowVariant & 0xc) == 8) {
		CToken_DrawShadowAll();
		CToken_DrawGlowSprites();
	}
}

/* 0x001b57d0  Rowing/CRowing_Tokens.cpp
Adds a token of a random kind (0-2) at a random place on the water when there are fewer
than the maximum. (The original guards the allocation with a C++ exception frame.) */
void RowTokens_Spawn(void)
{
	void *water = g_RowWater;
	void *token;
	float pos[4];
	float normal[4];
	float dx, dz;
	float d;
	int count = 0;
	int ok;

	for (token = CToken_Next(NULL); token != NULL; token = CToken_Next(token))
		count++;
	if (count >= g_RowTokenMax)
		return;
	do {
		pos[0] = (float)(rb_rand() % 2000 - 1000);
		pos[2] = (float)(rb_rand() % 2000 - 1000);
		ok = 1;
		d = pos[2] * pos[2] + pos[0] * pos[0];
		if (!(d <= 1000000.0f))                                     /* 0x00265b60 */
			ok = 0;
		if (d < 40000.0f)                                           /* 0x0026fd74 */
			ok = 0;
		for (token = CToken_Next(NULL); token != NULL; token = CToken_Next(token)) {
			dx = RB_F32(token, 0x180) - pos[0];
			if (dx < -100.0f || 100.0f < dx)
				continue;
			dz = RB_F32(token, 0x188) - pos[2];
			if (dz < -100.0f || 100.0f < dz)
				continue;
			ok = 0;
		}
	} while (!ok);
	pos[1] = rb_WaterHeight(water, pos[0], pos[2], normal);
	normal[0] = -normal[0];
	pos[3] = 1.0f;
	normal[2] = -normal[2];
	token = RB_NEW(0x290, RB_FILE_TOKENS, 0x188, 0x1001);
	if (token != NULL)
		CToken_Init(token, pos, rb_rand() % 3, 1.0f, normal);
}

/* 0x001b5a00  Rowing (gap; CRowing_Tokens.cpp inferred)
In layout 2: maybe adds a token, floats every token on the waves (facing the water's
normal) and updates them. */
void RowTokens_Update(void)
{
	void *water;
	void *token;
	float normal[4];
	float y;

	if ((g_RowVariant & 0xc) != 8)
		return;
	if (g_RowTokenChance < rb_randf())
		RowTokens_Spawn();
	water = g_RowWater;
	for (token = CToken_Next(NULL); token != NULL; token = CToken_Next(token)) {
		y = rb_WaterHeight(water, RB_F32(token, 0xd0), RB_F32(token, 0xd8), normal)
			+ DAT_00298d48 + 5.0f;
		RB_F32(token, 0xd4) = y;
		RB_F32(token, 0x184) = y - (DAT_00298d48 + 5.0f);
		normal[0] = -normal[0];
		normal[2] = -normal[2];
		memcpy(RB_ADDR(token, 0x190), normal, 16);
	}
	CToken_UpdateAll();
}
