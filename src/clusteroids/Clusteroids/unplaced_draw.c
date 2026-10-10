/*
Clusteroids/unplaced_draw.c  -  gap functions of C:\BlitzParty\Game\Src\Clusteroids\ without
a known source file: the drawing of the game (0x000deac0-0x000deb40 and 0x000e04c0-0x000e28b0,
between CClust_Player.cpp and CClust_Tokens.cpp).

ClustGame_Render (the game mode's Render) times and draws the passes: cityscape, traffic,
street lights, clouds, platform, ships and characters, gates, arena actors, pods, tokens,
bullets, trails, pod connectors, debris; then the HUD bars, the fade and screenshots.

The profiling table (0x002faeb8, 0x118 bytes per pass, 25 passes, 0x002fca10 the count):
+0x00 end ticks, +0x08 start ticks (int64), +0x10 milliseconds, +0x14 name.
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define CL_ACTOR(addr) ((struct BActorInstance *)CL_ADDR(addr))
#define CL_PROFILE(i) ((char *)CL_ADDR(0x002faeb8) + (i) * 0x118)
#define g_ProfileCount CL_VAR(int, 0x002fca10)

/* classlib_ui: CLHud_DrawScoreBars(self, s0, s1, s2, s3, threshold) (thiscall); its table
   prototype still has undefined4 parameters */
#define cl_HudDrawScoreBars ((void (*)(int, float, float, float, float, float))FUN_0007c080)

/* the pod list heads: 0x002fedf8 the pods (60 x 0x80), 0x002fedfc the free pods,
   0x002fee00 the scenic pods (59 x 0x80) */
#define g_FreePods CL_VAR(char *, 0x002fedfc)
#define g_ScenicPods CL_VAR(char *, 0x002fee00)

static long long cl_ticks_per_second(void)
{
	return (long long)(((unsigned long long)DAT_002cd7a4 << 32) | DAT_002cd7a0);
}

/* 0x000deac0  Clusteroids (gap) */
void ClustProfile_Reset(void)
{
	int i;

	g_ProfileCount = 0;
	for (i = 0; i < 25; i++) {
		CL_U32(CL_PROFILE(i), 0x0) = 0;
		CL_U32(CL_PROFILE(i), 0x4) = 0;
		CL_U32(CL_PROFILE(i), 0x8) = 0;
		CL_U32(CL_PROFILE(i), 0xc) = 0;
		CL_U32(CL_PROFILE(i), 0x10) = 0;
		CL_U8(CL_PROFILE(i), 0x14) = 0;
	}
}

/* 0x000deaf0  Clusteroids (gap) */
void ClustProfile_Begin(int index, const char *name)
{
	CL_AT(CL_PROFILE(index), 0x8, unsigned long long) = bkTimerRead();
	sprintf(CL_PROFILE(index) + 0x14, "#%d => %s", g_ProfileCount, name);
	g_ProfileCount = g_ProfileCount + 1;
}

/* 0x000deb40  Clusteroids (gap) */
void ClustProfile_End(int index)
{
	unsigned long long delta;

	CL_AT(CL_PROFILE(index), 0x0, unsigned long long) = bkTimerRead();
	delta = bkTimerDelta(CL_AT(CL_PROFILE(index), 0x8, unsigned long long),
		CL_AT(CL_PROFILE(index), 0x0, unsigned long long));
	CL_F32(CL_PROFILE(index), 0x10) =
		(float)((double)(long long)delta * 1000.0 / (double)cl_ticks_per_second());
}

/* 0x000debb0  Clusteroids (gap; not in the function table: the tail of ClustGame_Draw jumps
here) Prints the pass times. */
void FUN_000debb0(void)
{
	int i;

	for (i = 0; i < g_ProfileCount; i++)
		bkPrintf("%s => %f\n", CL_PROFILE(i) + 0x14, (double)CL_F32(CL_PROFILE(i), 0x10));
	bkPrintf("\n");
}

/* 0x000e04c0  Clusteroids (gap)
Draws player's three electric pod connectors (textures electric01-03): along the five
points of each line (+0x6d4, 0x50 bytes per line) two crossing strips, one vertical (15
up and down) and one sideways (the offset at +0x7c4 / +0x7cc of the line). */
void ClustDraw_PodConnector(int player, int red, int green, int blue, int alpha)
{
	char *pl = CL_PLAYER(player);
	float *v = (float *)CL_ADDR(0x002fec88);   /* 4 vertices of 9 floats, FVF 0x152 */
	unsigned int colour;
	float *p;
	float sideX;
	float sideZ;
	float u;
	float u1;
	int line;
	int seg;

	colour = DAT_002bce18[red] | DAT_002bc490[green] | DAT_002bd640[blue] | DAT_002bc898[alpha];
	((unsigned int *)v)[6] = colour;
	((unsigned int *)v)[15] = colour;
	((unsigned int *)v)[24] = colour;
	((unsigned int *)v)[33] = colour;
	for (line = 0; line < 3; line++) {
		if (CL_VAR(struct bdTexture *, 0x002f8f38 + line * 4) != NULL)
			bdSetTexture(0, CL_VAR(struct bdTexture *, 0x002f8f38 + line * 4));
		sideX = CL_F32(pl, 0x7c4 + line * 0x10);
		sideZ = CL_F32(pl, 0x7cc + line * 0x10);
		u = 0.0f;
		for (seg = 0; seg < 4; seg++) {
			p = CL_FP(pl, 0x6d4 + line * 0x50 + seg * 0x10);
			v[7] = u;
			v[8] = 0.0f;
			u1 = 0.25f + u;
			v[17] = 0.0f;
			v[26] = 1.0f;
			v[35] = 1.0f;
			v[0] = p[0];
			v[1] = p[1] + 15.0f;
			v[2] = p[2];
			v[9] = p[4];
			v[10] = p[5] + 15.0f;
			v[11] = p[6];
			v[18] = p[0];
			v[19] = p[1] - 15.0f;
			v[20] = p[2];
			v[27] = p[4];
			v[28] = p[5] - 15.0f;
			v[29] = p[6];
			v[16] = u1;
			v[25] = u;
			v[34] = u1;
			bdDrawPrimitiveFVF(4, v, 4, 0x152);
			v[0] = sideX + p[0];
			v[1] = p[1];
			v[2] = sideZ + p[2];
			v[9] = sideX + p[4];
			v[10] = p[5];
			v[11] = sideZ + p[6];
			v[18] = p[0] - sideX;
			v[19] = p[1];
			v[20] = p[2] - sideZ;
			v[27] = p[4] - sideX;
			v[28] = p[5];
			v[29] = p[6] - sideZ;
			bdDrawPrimitiveFVF(4, v, 4, 0x152);
			u = u1;
		}
	}
}

/* one cloud layer: 4 x 4 tiles of 12000 units at height `y`, scrolled by `scroll`, each
   tile drawn twice (rows offset by 48000 and 0) */
static void cl_draw_cloud_layer(float scroll, unsigned int y)
{
	float *v = (float *)CL_ADDR(0x002fed18);
	float vNext;
	float z0, z0a, z1, z1a;
	float x0, x1;
	float u;
	float u1;
	int row;
	int col;

	for (row = 0; row < 4; row++) {
		vNext = scroll + 0.25f;
		z0 = (float)row * 12000.0f;
		z0a = z0 - 48000.0f;
		z1 = 12000.0f + z0;
		z1a = z1 - 48000.0f;
		u = 0.0f;
		for (col = 0; col < 4; col++) {
			v[7] = u;
			u1 = 0.25f + u;
			v[8] = scroll;
			v[17] = scroll;
			x0 = (float)col * 12000.0f - 24000.0f;
			((unsigned int *)v)[1] = y;
			((unsigned int *)v)[10] = y;
			((unsigned int *)v)[19] = y;
			x1 = ((float)col * 12000.0f + 12000.0f) - 24000.0f;
			((unsigned int *)v)[28] = y;
			v[0] = x0;
			v[2] = z0a;
			v[9] = x1;
			v[11] = z0a;
			v[16] = u1;
			v[18] = x0;
			v[20] = z1a;
			v[25] = u;
			v[26] = vNext;
			v[27] = x1;
			v[29] = z1a;
			v[34] = u1;
			v[35] = vNext;
			bdDrawPrimitiveFVF(4, v, 4, 0x152);
			((unsigned int *)v)[1] = y;
			((unsigned int *)v)[10] = y;
			((unsigned int *)v)[19] = y;
			((unsigned int *)v)[28] = y;
			v[0] = x0;
			v[2] = z0;
			v[9] = x1;
			v[11] = z0;
			v[18] = x0;
			v[20] = z1;
			v[27] = x1;
			v[29] = z1;
			bdDrawPrimitiveFVF(4, v, 4, 0x152);
			u = u1;
		}
		scroll = vNext;
	}
}

/* 0x000e0720  Clusteroids (gap)
Draws the two scrolling cloud layers below the arena (low at -11000, top at -6000) in the
tweakable cloud colours ("Snap Colours": every channel from red). */
void ClustDraw_Clouds(void)
{
	unsigned int colour;
	unsigned int *c = (unsigned int *)CL_ADDR(0x002fed18);

	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(8, 0, 0);
	bdSetRenderState(2, 6, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(7, 1, 0);
	if (CL_VAR(void *, 0x002f8f44) != NULL)
		bdSetTexture(0, CL_VAR(struct bdTexture *, 0x002f8f44));
	if (CL_VAR(int, 0x002f7db8) == 0)
		colour = DAT_002bc898[CL_VAR(int, 0x00294c04)] | DAT_002bd640[CL_VAR(int, 0x00294c00)] |
			DAT_002bc490[CL_VAR(int, 0x00294bfc)] | DAT_002bce18[CL_VAR(int, 0x00294bf8)];
	else
		colour = DAT_002bc898[CL_VAR(int, 0x00294bf8)] | DAT_002bd640[CL_VAR(int, 0x00294bf8)] |
			DAT_002bc490[CL_VAR(int, 0x00294bf8)] | DAT_002bce18[CL_VAR(int, 0x00294bf8)];
	c[6] = colour;
	c[15] = colour;
	c[24] = colour;
	c[33] = colour;
	bdSetWorldMatrix(DAT_002855f0);
	cl_draw_cloud_layer(CL_VAR(float, 0x002f8f58), 0xc62be000u);     /* -11000 */
	if (CL_VAR(int, 0x002f7db8) == 0)
		colour = DAT_002bc898[CL_VAR(int, 0x00294bf4)] | DAT_002bd640[CL_VAR(int, 0x00294bf0)] |
			DAT_002bc490[CL_VAR(int, 0x00294bec)] | DAT_002bce18[CL_VAR(int, 0x00294be8)];
	else
		colour = DAT_002bc898[CL_VAR(int, 0x00294be8)] | DAT_002bd640[CL_VAR(int, 0x00294be8)] |
			DAT_002bc490[CL_VAR(int, 0x00294be8)] | DAT_002bce18[CL_VAR(int, 0x00294be8)];
	c[6] = colour;
	c[15] = colour;
	c[24] = colour;
	c[33] = colour;
	cl_draw_cloud_layer(CL_VAR(float, 0x002f8f54), 0xc5bb8000u);     /* -6000 */
	bdSetRenderState(7, 3, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(4, 1, 0);
}

/* 0x000e0ce0  Clusteroids (gap)
"Frame Counter" tweak: prints the frame rates centred at y 200. */
void ClustDraw_Fps(void)
{
	char text[256];
	unsigned short wide[256];
	float width;
	float height;

	bdSetProjectionMode(0);
	bdSetRenderState(0, 0, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(1, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetWorldMatrix(DAT_002855f0);
	sprintf(text, "FPS: %f (%f) (%f)", (double)CL_VAR(float, 0x002f7f80), (double)DAT_002ba990,
		(double)CL_VAR(float, 0x002fae98));
	bkStringToWide(wide, text);
	bdGetStringSize(CL_VAR(struct bdFont *, 0x002f8f64), wide, (int)strlen(text), &width, &height);
	bdPrintFontLite(CL_VAR(struct bdFont *, 0x002f8f64), wide, (int)strlen(text), width * -0.5f, 200.0f,
		0xff, 0xff, 0xff, 0xff, NULL, NULL);
}

/* 0x000e0e00  Clusteroids (gap)
Draws the attack ship and doors (intro only), the spinner and the grid (once it rose);
`trans` selects the transparent pass. */
void ClustDraw_ArenaActors(int trans)
{
	float m[16];
	int i;

	bdSetRenderState(4, 1, 0);
	if (CL_VAR(void *, 0x002f8f34) != NULL)
		bdTextureSetFrame(CL_VAR(struct bdTexture *, 0x002f8f34), CL_VAR(int, 0x002f9a04));
	for (i = 0; i < 4; i++) {
		if (i == 3 && CL_VAR(int, 0x002f8db0) == 0)
			continue;
		bmMatrixRotationZ(m, 0.0f);
		bmMatrixToQuaternion((float *)CL_ADDR(0x002f8890 + i * 0x170), m);
		switch (i) {
		case 0:
			bdSetRenderState(4, 0, 0);
			bdSetRenderState(6, 0, 0);
			bdSetRenderState(8, 0, 0);
			if (g_ClPhase != 1)
				continue;
			break;
		case 1:
			bdSetRenderState(4, 1, 0);
			bdSetRenderState(6, 0, 0);
			bdSetRenderState(8, 1, 0);
			if (g_ClPhase != 1)
				continue;
			break;
		default:
			bdSetRenderState(4, 1, 0);
			bdSetRenderState(6, 1, 0);
			bdSetRenderState(10, 1, 0);
			bdSetRenderState(8, 0, 0);
			break;
		}
		baPrepActorInstance(CL_ACTOR(0x002f87b0 + i * 0x170), 1);
		baDrawActorInstance(CL_ACTOR(0x002f87b0 + i * 0x170), trans == 0 ? 1 : 4);
	}
	bdSetRenderState(8, 0, 0);
	bdSetRenderState(4, 1, 0);
}

/* 0x000e0f60  Clusteroids (gap)
Collector: the leader's gate turns (0x002f8730); without a clear leader the neutral gate. */
void ClustDraw_Gates(void)
{
	float m[16];
	short leader;
	short i;

	leader = (short)ClustPods_GetLeader();
	bmMatrixRotationY(m, CL_VAR(float, 0x002f8730));
	if (leader != -1) {
		for (i = 0; i < CL_VAR(short, 0x002f7f8e); i++) {
			if (i != leader)
				continue;
			CL_VAR(float, 0x002f8070 + i * 0x170) = CL_VAR(float, 0x002f86e0 + i * 0x10);
			CL_VAR(float, 0x002f8074 + i * 0x170) = CL_VAR(float, 0x002f86e4 + i * 0x10);
			CL_VAR(float, 0x002f8078 + i * 0x170) = CL_VAR(float, 0x002f86e8 + i * 0x10);
			bmMatrixToQuaternion((float *)CL_ADDR(0x002f8090 + i * 0x170), m);
			baPrepActorInstance(CL_ACTOR(0x002f7fb0 + i * 0x170), 1);
			baDrawActorInstance(CL_ACTOR(0x002f7fb0 + i * 0x170), 1);
		}
		return;
	}
	CL_VAR(float, 0x002f8630) = CL_VAR(float, 0x002f8720);
	CL_VAR(float, 0x002f8634) = CL_VAR(float, 0x002f8724);
	CL_VAR(float, 0x002f8638) = CL_VAR(float, 0x002f8728);
	bmMatrixToQuaternion((float *)CL_ADDR(0x002f8650), m);
	baPrepActorInstance(CL_ACTOR(0x002f8570), 1);
	baDrawActorInstance(CL_ACTOR(0x002f8570), 1);
}

/* 0x000e1060  Clusteroids (gap) */
void ClustDraw_PodConnectors(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0) {
			baSetBrightness(0x100);
			if (CL_PTR(CL_PLAYER(i), PL_PODS) != NULL)
				ClustDraw_PodConnector(i, 0xff, 0xff, 0xff, 0xff);
		}
	}
	baSetBrightness(0x100);
}

/* draws pod actor `actor` at the position, rotated (x, y) */
static void cl_draw_pod_actor(unsigned int actor, float x, float y, float z, float rx, float ry, int trans)
{
	float m[16];

	CL_VAR(float, actor + 0xc0) = x;
	CL_VAR(float, actor + 0xc4) = y;
	CL_VAR(float, actor + 0xc8) = z;
	bmMatrixRotationZYX(m, rx, ry, 0.0f);
	bmMatrixToQuaternion((float *)CL_ADDR(actor + 0xe0), m);
	baPrepActorInstance(CL_ACTOR(actor), 1);
	baDrawActorInstance(CL_ACTOR(actor), trans == 0 ? 1 : 4);
}

/* 0x000e10d0  Clusteroids (gap)
Collector: the pods flying into the gates, fading with their height. */
void ClustDraw_ScenicPods(int trans)
{
	char *pod;
	int index;
	int offset;

	baSetBrightness(0x100);
	for (offset = 0; offset < 0x1d80; offset += 0x80) {
		pod = g_ScenicPods + offset;
		if (CL_I16(pod, 0x3a) == 0)
			continue;
		baSetBrightness(CL_FTOL(255.0 - (double)fabsf(CL_F32(pod, 0x50)) * 0.0006666666666666666 * 255.0));
		index = CL_I16(pod, 0xa);
		cl_draw_pod_actor(0x002f9140 + index * 0x170, CL_F32(pod, 0x4c), CL_F32(pod, 0x50),
			CL_F32(pod, 0x54), CL_F32(pod, 0x28), CL_F32(pod, 0x2c), trans);
	}
	baSetBrightness(0x100);
}

/* 0x000e1200  Clusteroids (gap)
Pod Hog / Collector: the free pods as white tiles. */
void ClustDraw_FreePods(int trans)
{
	char *pod;

	for (pod = g_FreePods; pod != NULL; pod = CL_PTR(pod, 0)) {
		baSetBrightness(CL_FTOL(CL_F32(pod, 0x3c)));
		if (CL_I16(pod, 0x38) == 0)
			cl_draw_pod_actor(0x002f8fc0, CL_F32(pod, 0x10),
				ClustArena_GetHeight(CL_F32(pod, 0x10), CL_F32(pod, 0x14)) + 12.0f, CL_F32(pod, 0x14),
				CL_F32(pod, 0x28), CL_F32(pod, 0x2c) + 3.1415925f, trans);
	}
	baSetBrightness(0x100);
}

/* 0x000e12e0  Clusteroids (gap)
Tails / Mines: the free pods as white tiles (Mines: turned round). */
void ClustDraw_FreeMines(int trans)
{
	char *pod;

	baSetBrightness(0x100);
	for (pod = g_FreePods; pod != NULL; pod = CL_PTR(pod, 0)) {
		if (CL_I16(pod, 0x38) != 0)
			continue;
		baSetBrightness(CL_FTOL(CL_F32(pod, 0x3c)));
		if (g_ClVariant == 3)
			cl_draw_pod_actor(0x002f8fc0, CL_F32(pod, 0x10),
				ClustArena_GetHeight(CL_F32(pod, 0x10), CL_F32(pod, 0x14)) + 12.0f, CL_F32(pod, 0x14),
				CL_F32(pod, 0x28), CL_F32(pod, 0x2c) + 3.1415925f, trans);
		else
			cl_draw_pod_actor(0x002f8fc0, CL_F32(pod, 0x10),
				ClustArena_GetHeight(CL_F32(pod, 0x10), CL_F32(pod, 0x14)) + 12.0f, CL_F32(pod, 0x14),
				CL_F32(pod, 0x28), CL_F32(pod, 0x2c), trans);
	}
	baSetBrightness(0x100);
}

/* 0x000e1440  Clusteroids (gap)
Tails / Mines: the pods held by the players in the player's pod model (Tails: a pod
becomes the player's once attached long enough; the last one only with
"lastPodPermanent"; until then a white tile). */
void ClustDraw_HeldPods(int trans)
{
	char *pod;
	int player;

	baSetBrightness(0x100);
	for (player = 0; player < 4; player++) {
		for (pod = CL_PTR(CL_PLAYER(player), PL_PODS); pod != NULL; pod = CL_PTR(pod, 0)) {
			if (g_ClVariant == 1 &&
				!(CL_VAR(float, 0x002f7d64) <= CL_F32(pod, 0x4) &&
				  (CL_VAR(int, 0x002f7d68) != 0 || CL_PTR(pod, 0) != NULL))) {
				cl_draw_pod_actor(0x002f8fc0, CL_F32(pod, 0x10),
					ClustArena_GetHeight(CL_F32(pod, 0x10), CL_F32(pod, 0x14)) + 12.0f, CL_F32(pod, 0x14),
					CL_F32(pod, 0x28), CL_F32(pod, 0x2c), trans);
				continue;
			}
			cl_draw_pod_actor(0x002f9140 + player * 0x170, CL_F32(pod, 0x10),
				ClustArena_GetHeight(CL_F32(pod, 0x10), CL_F32(pod, 0x14)) + 12.0f, CL_F32(pod, 0x14),
				CL_F32(pod, 0x28), CL_F32(pod, 0x2c), trans);
		}
	}
}

/* 0x000e1620  Clusteroids (gap)
Pod Hog / Collector: the held pods as white tiles. */
void ClustDraw_HeldPodTiles(int trans)
{
	char *pod;
	int player;

	baSetBrightness(0x100);
	for (player = 0; player < 4; player++) {
		for (pod = CL_PTR(CL_PLAYER(player), PL_PODS); pod != NULL; pod = CL_PTR(pod, 0))
			cl_draw_pod_actor(0x002f8fc0, CL_F32(pod, 0x10),
				ClustArena_GetHeight(CL_F32(pod, 0x10), CL_F32(pod, 0x14)) + 12.0f, CL_F32(pod, 0x14),
				CL_F32(pod, 0x28), CL_F32(pod, 0x2c) + 3.1415925f, trans);
	}
}

/* 0x000e16f0  Clusteroids (gap)
Draws the live bullets, fading with their life. */
void ClustDraw_Bullets(void)
{
	float m[16];
	char *bullet;
	int i;

	bdSetWorldMatrix(DAT_002855f0);
	for (i = 0; i < 60; i++) {
		bullet = (char *)CL_ADDR(0x002fee08) + i * 0x2c;
		if (CL_F32(bullet, 0x18) == 0.0f || CL_I16(bullet, 0x0) == 1)
			continue;
		baSetBrightness(CL_FTOL(CL_F32(bullet, 0x18) * 255.0f) / 0x4b);
		CL_VAR(float, 0x002f97d0) = CL_F32(bullet, 0x4);
		CL_VAR(float, 0x002f97d8) = CL_F32(bullet, 0x8);
		CL_VAR(float, 0x002f97d4) = ClustArena_GetHeight(CL_VAR(float, 0x002f97d0), CL_F32(bullet, 0x8)) - 13.2f;
		bmMatrixRotationYXZ(m, 0.0f, -CL_F32(bullet, 0x20), CL_F32(bullet, 0x24));
		bmMatrixToQuaternion((float *)CL_ADDR(0x002f97f0), m);
		baPrepActorInstance(CL_ACTOR(0x002f9710), 1);
		baDrawActorInstance(CL_ACTOR(0x002f9710), 4);
	}
	baSetBrightness(0xff);
}

/* 0x000e17f0  Clusteroids (gap)
A debug line in colour 0x00rrggbb with `alpha`. */
void ClustDraw_Line(float *from, float *to, unsigned int colour, int alpha)
{
	float *v = (float *)CL_ADDR(0x002feda8);

	bdSetTexture(0, NULL);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(0, 1, 0);
	v[0] = from[0];
	v[1] = from[1];
	v[2] = from[2];
	v[9] = to[0];
	v[10] = to[1];
	v[11] = to[2];
	((unsigned int *)v)[6] = DAT_002bce18[(colour >> 16) & 0xff] | DAT_002bc490[(colour >> 8) & 0xff] |
		DAT_002bd640[colour & 0xff] | DAT_002bc898[alpha];
	((unsigned int *)v)[15] = ((unsigned int *)v)[6];
	bdDrawPrimitiveFVF(1, v, 2, 0x152);
	bdSetRenderState(10, 1, 0);
}

/* 0x000e18d0  Clusteroids (gap)
A debug circle of `radius` around (x, y, z): 36 lines. */
void ClustDraw_Circle(float x, float y, float z, float radius, unsigned int colour, int alpha)
{
	float points[37][4];
	int angle;
	int i;

	bdSetWorldMatrix(DAT_002855f0);
	points[0][0] = 0.0f;
	points[0][1] = 0.0f;
	points[0][2] = radius;
	points[0][3] = 1.0f;
	angle = 0;
	for (i = 1; i <= 36; i++) {
		angle += 10;
		bmVectorRotateY(points[i], points[0], (float)angle * 0.0174532942f);
	}
	for (i = 0; i < 37; i++) {
		points[i][0] = x + points[i][0];
		points[i][1] = y + points[i][1];
		points[i][2] = z + points[i][2];
	}
	for (i = 1; i <= 36; i++)
		ClustDraw_Line(points[i - 1], points[i], colour, alpha);
}

/* 0x000e19c0  Clusteroids (gap)
"radiusToggle": the arena, the centre, the camera target, ships, pods, bullets and
tokens as circles. */
void ClustDraw_DebugRadii(void)
{
	char *pl;
	char *pod;
	char *bullet;
	char *token;
	int i;

	baSetBrightness(0x100);
	ClustDraw_Circle(0.0f, 0.0f, 0.0f, CL_VAR(float, 0x002f7f68), 0xffff0000, 0xff);
	ClustDraw_Circle(0.0f, 0.0f, 0.0f, 125.0f, 0xffff0000, 0xff);
	ClustDraw_Circle(CL_VAR(float, 0x002f9b24), 0.0f, CL_VAR(float, 0x002f9b2c), 10.0f, 0xffffffff, 0xff);
	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		if (CL_I16(pl, PL_OUT) != 0)
			continue;
		ClustDraw_Circle(CL_F32(pl, PL_X), 0.0f, CL_F32(pl, PL_Z), 35.0f, 0xffff0000, 0xff);
		for (pod = CL_PTR(pl, PL_PODS); pod != NULL; pod = CL_PTR(pod, 0))
			ClustDraw_Circle(CL_F32(pod, 0x10), 0.0f, CL_F32(pod, 0x14), 35.0f, 0xffff0000, 0xff);
	}
	for (pod = g_FreePods; pod != NULL; pod = CL_PTR(pod, 0)) {
		if (CL_I16(pod, 0x38) == 0)
			ClustDraw_Circle(CL_F32(pod, 0x10), 0.0f, CL_F32(pod, 0x14), 35.0f, 0xffff0000, 0xff);
	}
	for (i = 0; i < 60; i++) {
		bullet = (char *)CL_ADDR(0x002fee08) + i * 0x2c;
		if (CL_F32(bullet, 0x18) != 0.0f && CL_I16(bullet, 0x0) != 1)
			ClustDraw_Circle(CL_F32(bullet, 0x4), 0.0f, CL_F32(bullet, 0x8), 8.0f, 0xffff0000, 0xff);
	}
	for (i = 0; i < 50; i++) {
		token = CL_VAR(char *, 0x002fedf0) + i * 0x2c;
		if (CL_I16(token, 0x1c) != 0)
			ClustDraw_Circle(CL_F32(token, 0x8), 0.0f, CL_F32(token, 0x10), 10.0f, 0xffff0000, 0xff);
	}
}

/* 0x000e1b60  Clusteroids (gap)
Sets player's three connector lines from `from` to `to` (points at 0, 1/4, 1/2, 3/4, 1;
the side offset 15 units across the line). The original also advances the random seed
six times per point (the jitter it computed is not used). */
void ClustPlayer_SetConnector(int player, float *from, float *to)
{
	char *pl = CL_PLAYER(player);
	float *p;
	double angle;
	int line;
	int k;

	for (line = 0; line < 3; line++) {
		angle = atan2((double)to[0] - (double)from[0], (double)to[2] - (double)from[2]) +
			(double)1.5707963f;
		CL_F32(pl, 0x7c4 + line * 0x10) = (float)(sin(angle) * 15.0);
		CL_F32(pl, 0x7cc + line * 0x10) = (float)(cos(angle) * 15.0);
		p = CL_FP(pl, 0x6d4 + line * 0x50);
		p[0] = from[0];
		p[1] = from[1];
		p[2] = from[2];
		p[3] = from[3];
		p[4] = (to[0] - from[0]) * 0.25f + from[0];
		p[5] = (to[1] - from[1]) * 0.25f + from[1];
		p[6] = (to[2] - from[2]) * 0.25f + from[2];
		for (k = 0; k < 6; k++)
			cl_rand();
		p[8] = (to[0] - from[0]) * 0.5f + from[0];
		p[9] = (to[1] - from[1]) * 0.5f + from[1];
		p[10] = (to[2] - from[2]) * 0.5f + from[2];
		for (k = 0; k < 6; k++)
			cl_rand();
		p[12] = (to[0] - from[0]) * 0.75f + from[0];
		p[13] = (to[1] - from[1]) * 0.75f + from[1];
		p[14] = (to[2] - from[2]) * 0.75f + from[2];
		for (k = 0; k < 6; k++)
			cl_rand();
		p[16] = (to[0] - from[0]) + from[0];
		p[17] = (to[1] - from[1]) + from[1];
		p[18] = (to[2] - from[2]) + from[2];
		for (k = 0; k < 6; k++)
			cl_rand();
	}
}

/* 0x000e1df0  Clusteroids (gap)
The ships' screen positions (+0x40, and the look-ahead point +0x50 for the camera), then
the ships (the low-detail ship model while playing) and the characters (the high-detail
one in the intro once the doors opened, at the end, or with "highPolyChars"). */
void ClustDraw_Players(void)
{
	float m[16];
	float screen[4];
	float ahead[4];
	char *pl;
	float x, oldX, z, oldZ;
	unsigned int actor;
	int i;

	bdSetRenderState(5, 1, 0);
	bdSetRenderState(7, 3, 0);
	bdSetRenderState(0xd, 0, 1);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		if (CL_I16(pl, PL_OUT) != 0)
			continue;
		x = CL_F32(pl, PL_X);
		oldX = CL_F32(pl, 0x38);
		CL_U32(pl, 0x48) = CL_U32(pl, 0x40);
		CL_U32(pl, 0x4c) = CL_U32(pl, 0x44);
		z = CL_F32(pl, PL_Z);
		oldZ = CL_F32(pl, 0x3c);
		bdSetWorldMatrix(DAT_002855f0);
		CL_F32(pl, 0x130) = CL_F32(pl, PL_X);
		CL_F32(pl, 0x134) = ClustArena_GetHeight(CL_F32(pl, PL_X), CL_F32(pl, PL_Z));
		CL_F32(pl, 0x138) = CL_F32(pl, PL_Z);
		CL_F32(pl, 0x13c) = 1.0f;
		bdProjectPoints(screen, CL_FP(pl, 0x130), 1);
		CL_F32(pl, 0x130) = CL_F32(pl, PL_X);
		CL_F32(pl, 0x40) = (float)DAT_002ba928 * 0.5f + screen[0];
		CL_F32(pl, 0x44) = (float)DAT_002ba92c * 0.5f + screen[1];
		CL_F32(pl, 0x134) = ClustArena_GetHeight(CL_F32(pl, PL_X), CL_F32(pl, PL_Z));
		CL_F32(pl, 0x138) = CL_F32(pl, PL_Z);
		CL_F32(pl, 0x13c) = 1.0f;
		ahead[1] = CL_F32(pl, 0x134);
		ahead[3] = CL_F32(pl, 0x13c);
		ahead[0] = CL_VAR(float, 0x002f7dac) * (x - oldX) + CL_F32(pl, 0x130);
		ahead[2] = CL_VAR(float, 0x002f7dac) * (z - oldZ) + CL_F32(pl, 0x138);
		bdSetWorldMatrix(DAT_002855f0);
		bdProjectPoints(screen, ahead, 1);
		CL_F32(pl, 0x50) = (float)DAT_002ba928 * 0.5f + screen[0];
		CL_F32(pl, 0x54) = (float)DAT_002ba92c * 0.5f + screen[1];
	}
	bdSetRenderState(3, 1, 0);
	ClustProfile_Begin(6, "Draw Player Ships");
	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		if (CL_I16(pl, PL_OUT) != 0)
			continue;
		actor = (g_ClPhase == 1 || g_ClPhase == 3) ? PL_SHIP : PL_SHIP2;
		CL_U32(pl, actor + 0xc0) = CL_U32(pl, PL_X);
		CL_F32(pl, actor + 0xc4) = ClustArena_GetHeight(CL_F32(pl, PL_X), CL_F32(pl, PL_Z));
		CL_U32(pl, actor + 0xc8) = CL_U32(pl, PL_Z);
		bmMatrixRotationY(m, -CL_F32(pl, 0x634));
		bmMatrixToQuaternion(CL_FP(pl, actor + 0xe0), m);
		baPrepActorInstance((struct BActorInstance *)(pl + actor), 1);
		baDrawActorInstance((struct BActorInstance *)(pl + actor), 1);
	}
	ClustProfile_End(6);
	ClustProfile_Begin(7, "Draw Player Characters");
	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		if (CL_I16(pl, PL_OUT) != 0)
			continue;
		if ((g_ClPhase == 1 && CL_VAR(short, 0x002f8db8) != 0) || g_ClPhase == 3 ||
			CL_VAR(int, 0x002f7db4) != 0)
			actor = PL_CHAR;
		else
			actor = PL_CHAR2;
		CL_U32(pl, actor + 0xc0) = CL_U32(pl, PL_X);
		CL_F32(pl, actor + 0xc4) = ClustArena_GetHeight(CL_F32(pl, PL_X), CL_F32(pl, PL_Z));
		CL_U32(pl, actor + 0xc8) = CL_U32(pl, PL_Z);
		bmMatrixRotationY(m, -CL_F32(pl, 0x634));
		bmMatrixToQuaternion(CL_FP(pl, actor + 0xe0), m);
		baPrepActorInstance((struct BActorInstance *)(pl + actor), 1);
		baDrawActorInstance((struct BActorInstance *)(pl + actor), 1);
	}
	ClustProfile_End(7);
	bdSetRenderState(3, 0, 0);
}

/* 0x000e2210  Clusteroids (gap)
Every player holding pods: the connector from 24 units behind the ship (height 7.2) to
its first pod (height 7.2). */
void ClustPlayers_UpdateConnectors(void)
{
	char *pl;
	char *pod;
	float to[3];
	float from[4];
	double angle;
	int i;

	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		pod = CL_PTR(pl, PL_PODS);
		if (CL_I16(pl, PL_OUT) != 0 || pod == NULL)
			continue;
		to[0] = CL_F32(pod, 0x10);
		to[2] = CL_F32(pod, 0x14);
		angle = (double)CL_F32(pl, 0x634) + (double)1.5707963f;
		from[1] = 7.2f;
		to[1] = 7.2f;
		from[0] = (float)(cos(angle) * 24.0 + (double)CL_F32(pl, PL_X));
		from[2] = (float)(sin(angle) * 24.0 + (double)CL_F32(pl, PL_Z));
		ClustPlayer_SetConnector(i, from, to);
	}
}

/* 0x000e22b0  Clusteroids (gap)
Draws the game in passes, each one timed (ClustProfile_*); "drawTimeToggle" prints the
times. */
void ClustGame_Draw(void)
{
	ClustProfile_Reset();
	ClustProfile_Begin(0, "Draw All");
	bdSetProjectionMode(1);
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(4, 1, 0);
	bdSetRenderState(0, 1, 0);
	bdSetCameraLookAt((float *)CL_ADDR(0x00295e08), (float *)CL_ADDR(0x002f9b24), 0.0f);
	bdUpdateView();
	bdSetWorldMatrix(DAT_002855f0);
	bdSetTexture(0, NULL);
	bdSetRenderState(1, 2, 0);
	bdSetNearFar(40.0f, 100000.0f);
	bdSetFogColour(CL_VAR(unsigned int, 0x00294bd8), CL_VAR(unsigned int, 0x00294bdc),
		CL_VAR(unsigned int, 0x00294be0));
	bdSetFogRange(10000.0f, 40000.0f);
	bdSetRenderState(8, 1, 0);
	if (CL_VAR(int, 0x002f7dc0) != 0)
		ClustDraw_DebugRadii();
	ClustProfile_Begin(1, "Draw Cityscape");
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(7, 3, 0);
	baDrawSet(CL_VAR(struct BSet *, 0x002f8788), 1);
	ClustProfile_End(1);
	bdSetRenderState(4, 1, 0);
	ClustProfile_Begin(2, "Draw Traffic");
	baPrepActorInstance(CL_ACTOR(0x002f8dc0), 1);
	if (g_ClPhase == 2 || g_ClPhase == 3)
		baDrawActorInstance(CL_ACTOR(0x002f8dc0), 1);
	ClustProfile_End(2);
	bdSetRenderState(3, 0, 0);
	ClustProfile_Begin(3, "Draw Street Lights");
	ClustArena_DrawLights();
	ClustProfile_End(3);
	ClustProfile_Begin(4, "Draw Clouds");
	ClustDraw_Clouds();
	ClustProfile_End(4);
	ClustProfile_Begin(5, "Draw Platform");
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(4, 0, 0);
	baDrawSet(CL_VAR(struct BSet *, 0x002f8784), 1);
	if (g_ClPhase == 1)
		baDrawSet(CL_VAR(struct BSet *, 0x002f878c), 1);
	bdSetRenderState(4, 1, 0);
	ClustProfile_End(5);
	bdSetWorldMatrix(DAT_002855f0);
	ClustDraw_Players();
	ClustProfile_Begin(8, "Draw Goals");
	bdSetWorldMatrix(DAT_002855f0);
	if (g_ClVariant == 4)
		ClustDraw_Gates();
	ClustProfile_End(8);
	ClustProfile_Begin(9, "Draw Arena Actors (Solid)");
	ClustDraw_ArenaActors(0);
	ClustProfile_End(9);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(3, 0, 0);
	ClustProfile_Begin(10, "Draw Arena Actors (Trans)");
	ClustDraw_ArenaActors(1);
	ClustProfile_End(10);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(4, 1, 0);
	bdSetRenderState(6, 0, 0);
	ClustProfile_Begin(0xb, "Draw Pods Alpha");
	baSetLightColour(CL_VAR(struct BLight *, 0x002f9a74), 0x20, 0x20, 0x20);
	baSetAmbientLight(0x20, 0x20, 0x20);
	switch (g_ClVariant) {
	case 1:
	case 3:
		if (g_ClPhase != 3)
			ClustDraw_FreeMines(0);
		ClustDraw_HeldPods(0);
		break;
	case 2:
		if (g_ClPhase != 3)
			ClustDraw_FreePods(0);
		ClustDraw_HeldPodTiles(0);
		break;
	case 4:
		if (g_ClPhase != 3) {
			ClustDraw_FreePods(0);
			ClustDraw_ScenicPods(0);
		}
		ClustDraw_HeldPodTiles(0);
		break;
	}
	ClustProfile_End(0xb);
	baSetLightColour(CL_VAR(struct BLight *, 0x002f9a74), 0x7f, 0x7f, 0x7f);
	baSetAmbientLight(0x7f, 0x7f, 0x7f);
	bdSetRenderState(3, 1, 0);
	ClustProfile_Begin(0xc, "Draw Tokens");
	ClustTokens_Draw();
	ClustProfile_End(0xc);
	bdSetRenderState(5, 0, 0);
	ClustProfile_Begin(0xd, "Draw Bullets");
	ClustDraw_Bullets();
	ClustProfile_End(0xd);
	ClustProfile_Begin(0xe, "Draw Tokens glow");
	ClustTokens_DrawGlow();
	ClustProfile_End(0xe);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetTexture(0, NULL);
	bdSetWorldMatrix(DAT_002855f0);
	bdSetRenderState(3, 0, 0);
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 0, 0);
	ClustProfile_Begin(0xf, "Draw Trails");
	ClustTrails_Draw();
	ClustProfile_End(0xf);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	ClustProfile_Begin(0x10, "Draw Pod Connector");
	bdSetRenderState(4, 0, 0);
	bdSetWorldMatrix(DAT_002855f0);
	ClustDraw_PodConnectors();
	ClustProfile_End(0x10);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(4, 1, 0);
	bdSetRenderState(3, 0, 0);
	ClustProfile_Begin(0x11, "Draw Pods Alpha");
	switch (g_ClVariant) {
	case 1:
	case 3:
		if (g_ClPhase != 3)
			ClustDraw_FreeMines(1);
		ClustDraw_HeldPods(1);
		break;
	case 2:
		if (g_ClPhase != 3)
			ClustDraw_FreePods(1);
		ClustDraw_HeldPodTiles(1);
		break;
	case 4:
		if (g_ClPhase != 3) {
			ClustDraw_FreePods(1);
			ClustDraw_ScenicPods(1);
		}
		ClustDraw_HeldPodTiles(1);
		break;
	}
	ClustProfile_End(0x11);
	baSetBrightness(0x100);
	bdSetTexture(0, NULL);
	bdSetWorldMatrix(DAT_002855f0);
	ClustProfile_Begin(0x12, "Draw Extra System");
	if (g_ClPhase == 2 || g_ClPhase == 1)
		CLDebris_Stop();
	ClustProfile_End(0x12);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(4, 0, 0);
	ClustProfile_End(0);
	if (CL_VAR(int, 0x002f7dc4) != 0)
		FUN_000debb0();
}

/* 0x000e28b0  Clusteroids (gap)
Render (the game mode's +0x1c): the frame time of the drawing (0x002fec68..), the game,
the frame counter, the overlays, the HUD bars (the boost charge while playing), the
camera area, the fade (in during the intro, out at the end) and "GRAB%04d.BMP"
screenshots. */
void ClustGame_Render(void)
{
	unsigned long long now;
	unsigned long long delta;
	char name[64];
	float inv;
	float rate;

	CL_VAR(unsigned int, 0x002fa250) = CL_VAR(unsigned int, 0x002fec68);
	CL_VAR(unsigned int, 0x002fa254) = CL_VAR(unsigned int, 0x002fec6c);
	now = bkTimerRead();
	CL_VAR(unsigned int, 0x002fec7c) = (unsigned int)(now >> 32);
	CL_VAR(unsigned int, 0x002fec78) = (unsigned int)now;
	delta = bkTimerDelta(CL_AT(CL_ADDR(0x002fec68), 0, unsigned long long), now);
	CL_VAR(unsigned int, 0x002fec68) = CL_VAR(unsigned int, 0x002fec78);
	CL_VAR(unsigned int, 0x002fec6c) = CL_VAR(unsigned int, 0x002fec7c);
	rate = bkTimerRate((longlong)delta);
	CL_VAR(float, 0x002fec70) = rate;
	CL_VAR(float, 0x002fa244) = rate;
	CL_VAR(unsigned int, 0x002fa258) = CL_VAR(unsigned int, 0x002fec78);
	CL_VAR(unsigned int, 0x002fa25c) = CL_VAR(unsigned int, 0x002fec7c);
	ClustGame_Draw();
	bdSetRenderState(8, 0, 0);
	if (CL_VAR(int, 0x002f7da0) != 0)
		ClustDraw_Fps();
	bdSetRenderState(7, 1, 0);
	COverlay_DrawAll(1);
	if (g_ClPhase == 1 || g_ClPhase == 3) {
		cl_HudDrawScoreBars(DAT_002ecfac, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	} else if (g_ClPhase == 2) {
		inv = 1.0f / CL_VAR(float, 0x002f7d1c);
		cl_HudDrawScoreBars(DAT_002ecfac, 1.0f - CL_F32(CL_PLAYER(0), 0x65c) * inv,
			1.0f - CL_F32(CL_PLAYER(1), 0x65c) * inv, 1.0f - CL_F32(CL_PLAYER(2), 0x65c) * inv,
			1.0f - inv * CL_F32(CL_PLAYER(3), 0x65c), 1.0f);
	}
	if (CL_VAR(int, 0x002f7da8) != 0)
		ClustGame_DrawScreenBar();
	baSetBrightness(0x100);
	bdSetTexture(0, NULL);
	bdSetWorldMatrix(DAT_002855f0);
	bdSetRenderState(7, 1, 0);
	if (CL_VAR(int, 0x002f9adc) != 0 && CLFade_Draw(g_ClFrameTime) != 0)
		CL_VAR(int, 0x002f9adc) = 0;
	CLScreen_DrawDarken(g_ClPhase == 1 ? CL_VAR(int, 0x002f9ae0) : CL_VAR(int, 0x002f9ad8), 1);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 1, 0);
	if (CL_VAR(short, 0x002fa238) != 0) {
		sprintf(name, "GRAB%04d.BMP", (int)CL_VAR(short, 0x002fa23a));
		CL_VAR(short, 0x002fa23a) = CL_VAR(short, 0x002fa23a) + 1;
		bdScreenShot(name);
	}
}
