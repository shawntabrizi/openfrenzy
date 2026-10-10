/*
TankedUp/unplaced.c  <-  gap functions of C:\BlitzParty\Game\Src\TankedUp\ (0x001d62d0-
0x001d6c40, between CTank_Char.cpp and CTank_Debug.cpp; their files are not known)

The empty function 0x001d5950, ranking, the world mesh debug draw, the "<playerN>" colour tags of model names, render
state helpers, direction angles, the light cone debug draw and point helpers.
*/

#include "tankedup_b.h"
#include "tankedup_b_data.h"
#include "tankedup_b_ext.h"

extern int DAT_00283ef0;            /* core: follow a single human player (debug) */

/* 0x001d5950  TankedUp (gap)
An empty function (`ret`): the compiler folded TankedUp's debug print and several empty
methods into it; many vtables of other games point at it too. Callers pass arguments that
it ignores. */
void FUN_001d5950(void)
{
}

/* 0x001d62d0  TankedUp (gap)
The ranks (1 = best) of 4 scores; equal scores share a rank. */
void TankRanks(int *scores, int *ranks)
{
	int i;
	int j;
	int rank;

	for (i = 0; i < 4; i++) {
		rank = 1;
		for (j = 0; j < i; j++) {
			if (scores[i] < scores[j])
				rank = rank + 1;
			if (scores[j] < scores[i])
				ranks[j] = ranks[j] + 1;
		}
		ranks[i] = rank;
	}
}

/* the shade of a vertex by its height: 0x7f - y * 127 * 0.0005, clamped to 0..0x7f
   (0x00255348 = 127, 0x00257c64 = 0.0005) */
static int tu_height_shade(const float *v)
{
	int shade = 0x7f - TU_FTOL(v[1] * 127.0f * 0.000500000024f);

	if (shade >= 0x80)
		shade = 0x7f;
	else if (shade < 0)
		shade = 0;
	return shade;
}

/* 0x001d6330  TankedUp (gap)
Draws part `part` of a collision mesh (+0x2c vertices of 12 bytes, +0x3c triangles of 3
indices, +0x4c parts of 16 bytes {count, short indices, short base}) in `colour`
(0xAARRGGBB, through the colour tables): as outlines, or with `shaded` as triangles
darkened by height. */
void TankMesh_DrawPart(void *mesh, int part, unsigned int colour, int shaded)
{
	int *info = (int *)((char *)TU_PTR(mesh, 0x4c) + part * 0x10);
	unsigned short *tri;
	float *v;
	unsigned int rgb;
	int k;

	bdSetTexture(0, NULL);
	DAT_00315430[0].colour = DAT_002bc898[colour >> 24] | DAT_002bd640[(colour >> 16) & 0xff] |
		DAT_002bc490[(colour >> 8) & 0xff] | DAT_002bce18[colour & 0xff];
	DAT_00315430[1].colour = DAT_00315430[0].colour;
	DAT_00315430[2].colour = DAT_00315430[0].colour;
	DAT_00315430[3].colour = DAT_00315430[0].colour;
	rgb = DAT_002bd640[(colour >> 16) & 0xff] | DAT_002bc490[(colour >> 8) & 0xff] |
		DAT_002bce18[colour & 0xff];
	for (k = 0; k < info[0]; k++) {
		tri = (unsigned short *)TU_PTR(mesh, 0x3c) +
			(unsigned short)((unsigned short)(((short *)(intptr_t)info[1])[k] + (short)info[2]) * 3);
		if (shaded == 0) {
			v = (float *)TU_PTR(mesh, 0x2c) + tri[0] * 3;
			memcpy(DAT_00315430[0].pos, v, 12);
			memcpy(DAT_00315430[3].pos, v, 12);
			memcpy(DAT_00315430[1].pos, (float *)TU_PTR(mesh, 0x2c) + tri[1] * 3, 12);
			memcpy(DAT_00315430[2].pos, (float *)TU_PTR(mesh, 0x2c) + tri[2] * 3, 12);
			bdDrawPrimitiveFVF(2, DAT_00315430, 4, 0x152);
		} else {
			v = (float *)TU_PTR(mesh, 0x2c) + tri[0] * 3;
			memcpy(DAT_00315430[0].pos, v, 12);
			DAT_00315430[0].colour = DAT_002bc898[tu_height_shade(v)] | rgb;
			v = (float *)TU_PTR(mesh, 0x2c) + tri[1] * 3;
			memcpy(DAT_00315430[1].pos, v, 12);
			DAT_00315430[1].colour = DAT_002bc898[tu_height_shade(v)] | rgb;
			v = (float *)TU_PTR(mesh, 0x2c) + tri[2] * 3;
			memcpy(DAT_00315430[2].pos, v, 12);
			DAT_00315430[2].colour = DAT_002bc898[tu_height_shade(v)] | rgb;
			bdDrawPrimitiveFVF(3, DAT_00315430, 3, 0x152);
		}
	}
}

/* 0x001d6600  TankedUp (gap)
Replaces the first `tag` in `name` (at most 512 characters) by the letter `letter`. */
void TankChar_SubstituteTag(char *name, const char *tag, int letter)
{
	char buffer[512];
	char *found;

	found = strstr(name, tag); /* 0x001df0f0 is MSVC strstr */
	if (found == NULL)
		return;
	*found = 0;
	sprintf(buffer, "%s%c%s", name, (int)(char)letter, found + strlen(tag));
	sprintf(name, "%s", buffer);
}

/* 0x001d6680  TankedUp (gap)
Draws every part of a collision mesh (+0x20 * +0x24 * +0x28 parts) with TankMesh_DrawPart. */
void TankMesh_Draw(void *mesh, unsigned int colour, int shaded)
{
	int i;

	bdSetRenderState(3, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(10, 1, 0);
	bdSetTexture(0, NULL);
	for (i = 0; i < TU_I32(mesh, 0x20) * TU_I32(mesh, 0x24) * TU_I32(mesh, 0x28); i++)
		TankMesh_DrawPart(mesh, i, colour, shaded);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(3, 1, 0);
}

/* 0x001d6750  TankedUp (gap)
Shows the frame of an animated texture that is current `time` ticks into its animation
(+0x37 frames, +0x24 frame table of 0x18 bytes with the length at +0x14); a time past the
end changes nothing. */
void TankTexture_SetTime(void *texture, int time)
{
	int count = TU_U8(texture, 0x37);
	char *frames;
	int total = 0;
	int i;

	if (count <= 0)
		return;
	frames = (char *)TU_PTR(texture, 0x24);
	for (i = 0; i < count; i++)
		total = total + TU_I32(frames + i * 0x18, 0x14);
	if (total == 0)
		return;
	if (time < 0)
		time = 0;
	if (total < time)
		time = total;
	total = 0;
	for (i = 0; i < count; i++) {
		total = total + TU_I32(frames + i * 0x18, 0x14);
		if (time < total) {
			bdTextureSetFrame((struct bdTexture *)texture, i);
			return;
		}
	}
}

/* 0x001d67c0  TankedUp (gap)
Single-human mode: the debug follow mode (0x00283ef0) in a one-minigame game; *player (may
be NULL) gets the first human player. */
int TankChar_SingleHumanMode(int *player)
{
	if (DAT_00283ef0 == 0 || DAT_002bae8c != 1)
		return 0;
	if (player == NULL)
		return 1;
	for (*player = 0; *player < 4; *player = *player + 1) {
		if (DAT_002baed8[*player * 10] != 0)
			return 1;
	}
	return 0;
}

/* 0x001d6820  TankedUp (gap)
Render states for the floor decals: z function 0x202 (equal), state 72 on. */
void TankDraw_DecalStates(void)
{
	D3DDevice_SetRenderState_Simple(0x40364, 0x202);
	D3D__RenderState[70] = 0x202; /* 0x00203a68 */
	D3DDevice_SetRenderState_Simple(0x4036c, 1);
	D3D__RenderState[72] = 1;     /* 0x00203a70 */
}

/* 0x001d6860  TankedUp (gap)
The stencil test on. */
void TankDraw_StencilOn(void)
{
	D3DDevice_SetRenderState_StencilEnable(1);
}

/* 0x001d6870  TankedUp (gap)
The heading (around y, wrapped) and pitch (wrapped) of direction `dir`. */
void TankEnts_DirToAngles(float *dir, float *pitch, float *heading)
{
	float h;
	float c;
	float flat;

	h = CLMath_WrapAngle((float)atan2((double)dir[0], (double)dir[2]));
	*heading = h;
	c = (float)cos((double)h);
	if (c == 0.0f)
		flat = fabsf(dir[2]);
	else
		flat = fabsf(dir[2] / c);
	*pitch = CLMath_WrapAngle((float)atan2((double)dir[1], (double)flat));
}

/* 0x001d68f0  TankedUp (gap)
Puts each player's colour letter ("grbyop", by character) for "<player1>".."<player4>"
into a model name. */
void TankChar_SubstituteColours(char *name)
{
	TankChar_SubstituteTag(name, "<player1>", (unsigned char)s_grbyop_002b74a4[DAT_002baed4[0]]);
	TankChar_SubstituteTag(name, "<player2>", (unsigned char)s_grbyop_002b74a4[DAT_002baefc]);
	TankChar_SubstituteTag(name, "<player3>", (unsigned char)s_grbyop_002b74a4[DAT_002baf24]);
	TankChar_SubstituteTag(name, "<player4>", (unsigned char)s_grbyop_002b74a4[DAT_002baf4c]);
}

/* 0x001d6960  TankedUp (gap)
Debug: a light's cone from `pos` to a circle round `target` (radius tan(angle) * distance),
the apex in colour (r, g, b, alpha 127). */
void TankDebug_DrawCone(float *pos, float *target, float angle, int r, int g, int b)
{
	float dx, dy, dz;
	float radius;
	float yaw;
	float squash;
	float a;
	float d;
	unsigned int rim;
	int i;

	bdPushRenderState(6, 1, 0);
	bdSetTexture(0, NULL);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(4, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(3, 0, 0);
	bdSetWorldMatrix(DAT_002855f0);
	dx = target[0] - pos[0];
	dy = target[1] - pos[1];
	dz = target[2] - pos[2];
	radius = (float)tan((double)angle) * sqrtf(dy * dy + dx * dx + dz * dz);
	yaw = (float)atan2((double)dx, (double)dz);
	if (yaw > 6.28318548f)
		yaw = yaw - 6.28318548f;
	if (yaw < 0.0f)
		yaw = yaw + 6.28318548f;
	yaw = yaw + 1.57079625f; /* 0x00253898 */
	squash = radius * 0.666666687f; /* 0x00266898 */
	DAT_003154c0[0].pos[0] = pos[0];
	DAT_003154c0[0].pos[1] = pos[1];
	DAT_003154c0[0].pos[2] = pos[2];
	/* 0x002bca94 = DAT_002bc898[127] */
	DAT_003154c0[0].colour = DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bc898[127];
	rim = DAT_002bc898[0] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
	for (i = 1; i < 34; i++) {
		a = (float)i * 0.196349546f; /* 0x0027b940 = pi / 16 */
		d = radius - (float)sin((double)(a - yaw)) * squash;
		DAT_003154c0[i].pos[0] = (float)sin((double)a) * d + target[0];
		DAT_003154c0[i].pos[1] = target[1];
		DAT_003154c0[i].colour = rim;
		DAT_003154c0[i].pos[2] = (float)cos((double)a) * d + target[2];
	}
	bdDrawPrimitiveFVF(5, DAT_003154c0, 0x22, 0x152);
	bdPopRenderState(4);
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(5);
	bdPopRenderState(3);
}

/* 0x001d6b40  TankedUp (gap)
Texture stage 1 modulates by the texture factor alpha `alpha` (for see-through models). */
void TankDraw_SetAlphaFactor(int alpha)
{
	D3DDevice_SetRenderState_TextureFactor((DWORD)alpha << 24);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0x12);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xa, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
}

/* 0x001d6bd0  TankedUp (gap)
`point` turned by `angle` around the y axis through `centre`. */
void TankPoint_RotateAround(float *out, float *point, float angle, float *centre)
{
	float v[3];

	v[0] = point[0] - centre[0];
	v[1] = point[1] - centre[1];
	v[2] = point[2] - centre[2];
	bmVectorRotateY(v, v, angle);
	out[0] = v[0] + centre[0];
	out[1] = v[1] + centre[1];
	out[2] = v[2] + centre[2];
}

/* 0x001d6c40  TankedUp (gap)
Moves `point` to distance `distance` from `centre` along its direction from it. */
void TankPoint_SetDistance(float *point, float distance, float *centre)
{
	point[0] = point[0] - centre[0];
	point[1] = point[1] - centre[1];
	point[2] = point[2] - centre[2];
	bmVectorNormalise(point, point);
	point[0] = distance * point[0] + centre[0];
	point[1] = distance * point[1] + centre[1];
	point[2] = distance * point[2] + centre[2];
}
