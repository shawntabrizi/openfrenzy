/*
Clusteroids/CClust_Trails.c  <-  C:\BlitzParty\Game\Src\Clusteroids\CClust_Trails.cpp

The light trails the ships leave: a pool of 800 trail segments (0x494 bytes each,
allocated by 0x000dc9a0, tagged with CClust_Trails.cpp), an active list and a free list.
Every frame each ship adds a segment between its last and its current position (a box
of 8 quads in the player's colour); segments fade out and return to the free list.
0x000dc080-0x000dc930 (gap, next to the tagged function) are inferred to be in this file.

Globals: 0x002fa1d0 the pool, 0x002fa1d4 the active list, 0x002fa1d8 the free list.
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define CL_TRAIL_SIZE 0x494
#define CL_TRAIL_COUNT 800

/* a trail segment */
struct ClustTrail {
	struct ClustTrail *next;      /* 0x000 */
	short player;                 /* 0x004 player + 1 (1-4); otherwise one quad only */
	short pad;
	unsigned int verts[32][9];    /* 0x008 8 quads of x, y, z, nx, ny, nz, colour, u, v (FVF 0x152) */
	float alpha0;                 /* 0x488 alpha of the new end */
	float alpha1;                 /* 0x48c alpha of the old end */
	float fade;                   /* 0x490 subtracted per frame */
};
OF_ASSERT_SIZE(struct ClustTrail, CL_TRAIL_SIZE);

#define g_TrailPool CL_VAR(char *, 0x002fa1d0)
#define g_TrailActive CL_VAR(struct ClustTrail *, 0x002fa1d4)
#define g_TrailFree CL_VAR(struct ClustTrail *, 0x002fa1d8)

/* 0x000dc080  Clusteroids (gap; CClust_Trails.cpp inferred)
Puts every segment of the pool on the free list. */
void ClustTrails_Reset(void)
{
	int offset;

	g_TrailActive = NULL;
	g_TrailFree = (struct ClustTrail *)g_TrailPool;
	for (offset = 0; offset < (CL_TRAIL_COUNT - 1) * CL_TRAIL_SIZE; offset += CL_TRAIL_SIZE)
		*(char **)(g_TrailPool + offset) = g_TrailPool + offset + CL_TRAIL_SIZE;
	*(char **)(g_TrailPool + (CL_TRAIL_COUNT - 1) * CL_TRAIL_SIZE) = NULL;
}

/* 0x000dc0d0  Clusteroids (gap; CClust_Trails.cpp inferred) */
void ClustTrails_Free(void)
{
	CL_DELETE(g_TrailPool);
	g_TrailPool = NULL;
}

/* the colour of a trail vertex: alpha from `alpha`, the player's character colour (each
   channel doubled) */
static unsigned int cl_trail_colour(float alpha, short red, short green, short blue)
{
	return DAT_002bc898[CL_FTOL(alpha)] | DAT_002bd640[blue] | DAT_002bc490[green] |
		DAT_002bce18[red];
}

static void cl_trail_colours(struct ClustTrail *trail)
{
	int character;
	short red;
	short green;
	short blue;
	int quad;

	character = DAT_002baeac[trail->player * 10] * 0x60;
	red = (short)(DAT_00290718[character] << 1);
	green = (short)(DAT_00290718[character + 1] << 1);
	blue = (short)(DAT_00290718[character + 2] << 1);
	for (quad = 0; quad < 8; quad++) {
		trail->verts[quad * 4 + 0][6] = cl_trail_colour(trail->alpha0, red, green, blue);
		trail->verts[quad * 4 + 1][6] = cl_trail_colour(trail->alpha0, red, green, blue);
		trail->verts[quad * 4 + 2][6] = cl_trail_colour(trail->alpha1, red, green, blue);
		trail->verts[quad * 4 + 3][6] = cl_trail_colour(trail->alpha1, red, green, blue);
	}
}

/* 0x000dc0f0  Clusteroids (gap; CClust_Trails.cpp inferred)
Adds a segment for `player` (a player record) from its last position (+0x30, +0x34,
heading +0x58) to its current one (+0x18, +0x1c, heading +0x634): four strips (side
offsets -7, -4, 4, 7, half length 14) as 8 quads at heights 15/21 and -9/-3. Returns
the segment, or NULL when the pool is empty. */
void *ClustTrails_Add(char *player, short id)
{
	static const float side[4] = { -7.0f, -4.0f, 4.0f, 7.0f };
	static const float height[16] = {
		15.0f, 21.0f, 15.0f, 21.0f, 21.0f, 15.0f, 21.0f, 15.0f,
		-9.0f, -3.0f, -9.0f, -3.0f, -3.0f, -9.0f, -3.0f, -9.0f,
	};
	struct ClustTrail *trail;
	float newX[4], newZ[4], oldX[4], oldZ[4];
	float newSin, newCos, oldSin, oldCos;
	float along, across;
	int k;
	int half;
	int q;
	int s;
	unsigned int *v;

	trail = g_TrailFree;
	if (trail == NULL)
		return NULL;
	g_TrailFree = trail->next;
	trail->next = g_TrailActive;
	g_TrailActive = trail;

	newSin = sinf(CL_F32(player, 0x634) + 1.5707963f);
	newCos = cosf(CL_F32(player, 0x634) + 1.5707963f);
	oldSin = sinf(CL_F32(player, 0x58) + 1.5707963f);
	oldCos = cosf(CL_F32(player, 0x58) + 1.5707963f);
	for (k = 0; k < 4; k++) {
		along = 14.0f + 14.0f;
		across = side[k] + side[k];
		newX[k] = along * newCos - across * newSin;
		newZ[k] = along * newSin + across * newCos;
		oldX[k] = along * oldCos - across * oldSin;
		oldZ[k] = along * oldSin + across * oldCos;
	}
	for (half = 0; half < 2; half++) {
		for (q = 0; q < 16; q++) {
			v = trail->verts[half * 16 + q];
			s = half * 2 + (q & 1);
			if ((q & 2) == 0) {
				((float *)v)[0] = newX[s] + CL_F32(player, 0x18);
				((float *)v)[2] = newZ[s] + CL_F32(player, 0x1c);
			} else {
				((float *)v)[0] = oldX[s] + CL_F32(player, 0x30);
				((float *)v)[2] = oldZ[s] + CL_F32(player, 0x34);
			}
			((float *)v)[1] = height[q];
		}
	}
	trail->player = id;
	CL_U32(player, 0x30) = CL_U32(player, 0x18);
	CL_U32(player, 0x34) = CL_U32(player, 0x1c);
	CL_U32(player, 0x58) = CL_U32(player, 0x634);
	trail->player = id;
	trail->alpha0 = 188.0f;
	trail->alpha1 = 156.0f;
	trail->fade = 32.0f;
	cl_trail_colours(trail);
	return trail;
}

/* 0x000dc760  Clusteroids (gap; CClust_Trails.cpp inferred)
Fades the segments; a segment whose alpha reaches 0 goes back to the free list. */
void ClustTrails_Update(void)
{
	struct ClustTrail **link;
	struct ClustTrail *trail;
	float alpha0;
	float alpha1;

	link = &g_TrailActive;
	trail = g_TrailActive;
	while (trail != NULL) {
		alpha0 = trail->alpha0 - trail->fade;
		trail->alpha0 = alpha0;
		alpha1 = trail->alpha1 - trail->fade;
		trail->alpha1 = alpha1;
		if (alpha0 > 0.0f && alpha1 > 0.0f) {
			cl_trail_colours(trail);
			link = &trail->next;
		} else {
			*link = trail->next;
			trail->next = g_TrailFree;
			g_TrailFree = trail;
		}
		trail = *link;
	}
}

/* 0x000dc930  Clusteroids (gap; CClust_Trails.cpp inferred) */
void ClustTrails_Draw(void)
{
	struct ClustTrail *trail;
	int quad;

	for (trail = g_TrailActive; trail != NULL; trail = trail->next) {
		if (trail->player < 1 || trail->player > 4) {
			bdDrawPrimitiveFVF(4, trail->verts[0], 4, 0x152);
		} else {
			for (quad = 0; quad < 8; quad++)
				bdDrawPrimitiveFVF(4, trail->verts[quad * 4], 4, 0x152);
		}
	}
}

/* 0x000dc9a0  Clusteroids/CClust_Trails.cpp */
void ClustTrails_Init(void)
{
	g_TrailPool = (char *)CL_NEW(CL_TRAIL_COUNT * CL_TRAIL_SIZE, CL_FILE_TRAILS, 0x1c, 0x2001);
	ClustTrails_Reset();
}
