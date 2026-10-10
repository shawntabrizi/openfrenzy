/*
Clusteroids/CClust_Tokens.c  <-  C:\BlitzParty\Game\Src\Clusteroids\CClust_Tokens.cpp

The power-up tokens: 50 slots of 0x2c bytes (allocated by 0x000e2d10, tagged with
CClust_Tokens.cpp). A token appears when no player has a power-up; a player touching it
gets the three-way gun for 400 frames (player +0x14). 0x000e2ae0-0x000e3030 (gap) around
the tagged function work on the same slots; their file is inferred.

Token slot: +0x04 short kind (0: power-up; the index of its actor at 0x002f9890),
+0x08 x, +0x0c y, +0x10 z, +0x14 w, +0x18 spin angle, +0x1c short active.
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define CL_TOKEN_COUNT 50
#define CL_TOKEN_SIZE 0x2c
#define g_Tokens CL_VAR(char *, 0x002fedf0)
#define CL_TOKEN(i) (g_Tokens + (i) * CL_TOKEN_SIZE)
#define g_TokenCount CL_VAR(int, 0x002f9880)     /* power-up tokens on the field */

#define CL_ACTOR(addr) ((struct BActorInstance *)CL_ADDR(addr))

/* 0x000e2ae0  Clusteroids (gap; CClust_Tokens.cpp inferred)
Clears every token and restarts the power-up's spin animation. */
void ClustTokens_Reset(void)
{
	int i;

	for (i = 0; i < CL_TOKEN_COUNT; i++) {
		CL_I16(CL_TOKEN(i), 0x1c) = 0;
		CL_F32(CL_TOKEN(i), 0x18) = 0.0f;
	}
	baFlushAnimations(CL_ADDR(0x002f9890), CL_VAR(struct BActorNode *, 0x002f99c4), 1, 0);
	baQueueAnimation(CL_ACTOR(0x002f9890), CL_VAR(struct BActorNode *, 0x002f99c4),
		CL_VAR(struct BAnim *, 0x002f9a00), 0x400, -1, 1, NULL, NULL);
}

/* 0x000e2b40  Clusteroids (gap; CClust_Tokens.cpp inferred)
Puts a token of `kind` at (x, z) with a projector spiral; returns 0 when every slot is
used. */
int ClustTokens_Add(float x, float z, int kind)
{
	char *token;
	int i;

	for (i = 0; i < CL_TOKEN_COUNT; i++) {
		token = CL_TOKEN(i);
		if (CL_I16(token, 0x1c) != 0)
			continue;
		CL_F32(token, 0x8) = x;
		CL_F32(token, 0x10) = z;
		CL_I16(token, 0x1c) = 1;
		CL_I16(token, 0x4) = (short)kind;
		CL_F32(token, 0xc) = 0.0f;
		CL_F32(token, 0x14) = 0.0f;
		if (kind == 0)
			g_TokenCount = g_TokenCount + 1;
		CProjector_SpiralAtEx(CL_FP(token, 0x8), 100.0f, 75.0f, 2, 2.0f, -1);
		CLSamples_PlayGeneric(1, 0xff, 0, 0, NULL);
		return 1;
	}
	return 0;
}

/* 0x000e2bd0  Clusteroids (gap; CClust_Tokens.cpp inferred)
A player within 45 units of a token collects it (voice line, the power-up timer, the
"power-up" marker text). */
void ClustTokens_Collect(void)
{
	char *pl;
	char *token;
	float dx;
	float dz;
	float pos[4];
	int player;
	int i;

	for (player = 0; player < 4; player++) {
		pl = CL_PLAYER(player);
		if (CL_I16(pl, PL_OUT) != 0)
			continue;
		for (i = 0; i < CL_TOKEN_COUNT; i++) {
			token = CL_TOKEN(i);
			if (CL_I16(token, 0x1c) == 0)
				continue;
			dx = CL_F32(pl, PL_X) - CL_F32(token, 0x8);
			dz = CL_F32(pl, PL_Z) - CL_F32(token, 0x10);
			if (dx * dx + dz * dz < 2025.0f) {
				pos[0] = CL_F32(pl, PL_X);
				pos[2] = CL_F32(pl, PL_Z);
				pos[1] = 0.0f;
				pos[3] = 0.0f;
				CLSamples_PlayVoice((char)DAT_002baed4[player * 10], 5, pos, 0xff, 0.0f, 0, -1, 1);
				CL_F32(pl, 0x14) = 400.0f;
				CL_I16(token, 0x1c) = 0;
				if (CL_I16(token, 0x4) == 0) {
					CLHud_ShowMarkerText(DAT_002ecfac, player, 1);
					g_TokenCount = g_TokenCount - 1;
				}
				CLSamples_PlayGeneric(2, 0xff, 0, 0, NULL);
			}
		}
	}
}

/* 0x000e2d10  Clusteroids/CClust_Tokens.cpp */
void ClustTokens_Init(void)
{
	g_Tokens = (char *)CL_NEW(CL_TOKEN_COUNT * CL_TOKEN_SIZE, CL_FILE_TOKENS, 0x1e, 0x2001);
	ClustTokens_Reset();
}

/* 0x000e2d40  Clusteroids (gap; CClust_Tokens.cpp inferred)
Collects tokens; during play, when no token is out and no player has a power-up, puts a
new one at a random place between 290 units and the arena edge; spins the tokens. */
void ClustTokens_Update(void)
{
	char *token;
	float angle;
	float radius;
	int r;
	int i;

	ClustTokens_Collect();
	if (g_TokenCount == 0 && g_ClPhase == 2 &&
		CL_F32(CL_PLAYER(0), 0x14) + CL_F32(CL_PLAYER(1), 0x14) + CL_F32(CL_PLAYER(2), 0x14) +
		CL_F32(CL_PLAYER(3), 0x14) <= 0.0f) {
		r = cl_rand();
		angle = (float)r * 0.000191753f;
		r = cl_rand();
		radius = ((CL_VAR(float, 0x002f7f68) - 35.0f) - 290.0f) * (float)r * 3.0518509e-05f + 290.0f;
		ClustTokens_Add(sinf(angle) * radius, cosf(angle) * radius, 0);
	}
	if (CL_VAR(char, 0x002fa361) != 0)
		return;
	baUpdateAnimations(CL_ACTOR(0x002f9890), CL_VAR(struct BActorNode *, 0x002f99c4), g_ClFrameTime, NULL);
	if (CL_VAR(char, 0x002fa361) != 0)
		return;
	for (i = 0; i < CL_TOKEN_COUNT; i++) {
		token = CL_TOKEN(i);
		if (CL_I16(token, 0x1c) != 0) {
			CL_F32(token, 0x18) = g_ClFrameTime * 0.05f + CL_F32(token, 0x18);
			CL_F32(token, 0x18) = CLMath_WrapAngle(CL_F32(token, 0x18));
		}
	}
}

/* 0x000e2e90  Clusteroids (gap; CClust_Tokens.cpp inferred)
Draws the token actors (an environment-mapped second texture stage). */
void ClustTokens_Draw(void)
{
	float m[16];
	char *token;
	int index;
	int i;

	baSetBrightness(0x100);
	bdSetWorldIdentity();
	D3DDevice_SetRenderState_TextureFactor(0xbe000000);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0x12);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)10, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
	for (i = 0; i < CL_TOKEN_COUNT; i++) {
		token = CL_TOKEN(i);
		if (CL_I16(token, 0x1c) == 0)
			continue;
		bdSetWorldIdentity();
		index = CL_I16(token, 0x4);
		CL_VAR(float, 0x002f9950 + index * 0x170) = CL_F32(token, 0x8);
		CL_VAR(float, 0x002f9954 + index * 0x170) = CL_F32(token, 0xc) - 25.0f;
		CL_VAR(float, 0x002f9958 + index * 0x170) = CL_F32(token, 0x10);
		CL_VAR(float, 0x002f995c + index * 0x170) = CL_F32(token, 0x14);
		bmMatrixRotationY(m, 0.0f);
		bmMatrixToQuaternion((float *)CL_ADDR(0x002f9970 + index * 0x170), m);
		baPrepActorInstance(CL_ACTOR(0x002f9890 + index * 0x170), 1);
		baDrawActorInstance(CL_ACTOR(0x002f9890 + index * 0x170), 7);
	}
	bdSetTexture(1, NULL);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
}

/* 0x000e3030  Clusteroids (gap; CClust_Tokens.cpp inferred)
Draws a glow sprite 45 units above each token. */
void ClustTokens_DrawGlow(void)
{
	char *token;
	int i;

	baSetBrightness(0x100);
	bdSetWorldIdentity();
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	bdSetTexture(1, NULL);
	baSetBrightness(0x100);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(10, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetDepthOffset(-1.5f);
	for (i = 0; i < CL_TOKEN_COUNT; i++) {
		token = CL_TOKEN(i);
		if (CL_I16(token, 0x1c) == 0)
			continue;
		bdSetWorldIdentity();
		CL_F32(token, 0xc) = CL_F32(token, 0xc) + 45.0f;
		CL_F32(token, 0x14) = 1.0f;
		bdDrawSprite3D(CL_FP(token, 0x8), 70.0f, 70.0f, CL_F32(token, 0x18),
			CL_VAR(struct bdTexture *, 0x002f8f50), 0, 0, 0x7f, 0x7f, 0x7f, 0x38, NULL, 0.0f);
		CL_F32(token, 0xc) = CL_F32(token, 0xc) - 45.0f;
	}
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(7, 3, 0);
	bdPopRenderState(5);
	bdSetDepthOffset(0.0f);
}
