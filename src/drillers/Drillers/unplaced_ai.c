/*
Drillers/unplaced_ai.c  <-  C:\BlitzParty\Game\Src\Drillers\ (file not known)

The computer players (the AI object, one static object at 0x002aadf8, vtable 0x0026cbac)
and its geometry helpers. These gap functions lie between CDrillers_Resources.cpp
(0x0011e1e0) and CDrillers_Particle.cpp (0x00123550); no assert names their file.
The AI object's tuning, set by DrAi_Init from the variant and the difficulty:
  +0x04 skill (the reach of the side checks; variants 3, 4: the probe range)
  +0x08 caution (variants 1, 2: other players block a cell when it is 0.5 or less)
  +0x0c .. +0x48 more tuning values (not read by the code of this module)
The table entry 0x00121b30 is not a function: it is case 2 of the switch in DrAi_Init.

Headings are in degrees: 0 is +z (the next row), 90 is +x (the next column), 180 is -z,
270 is -x.
*/

#include "drillers.h"
#include "engine_misc_maths.h"

#define AI(off, type) DR_AT(self, off, type)

extern int DAT_002bae70; /* difficulty of the CPU players, 0-2 */

#define DR_HALF_Z DR_VAR(int, 0x002ff9e4u) /* the map has 2 * DR_HALF_Z rows (z) */
#define DR_HALF_X DR_VAR(int, 0x002ff9e8u) /* and 2 * DR_HALF_X columns (x) */
#define DR_CELL_SIZE DR_VAR(int, 0x002ff9e0u)
#define DR_SPEED_BONUS DR_VAR(float, 0x002ffa78u)
#define DR_BOOST(i) DR_VAR(float, 0x002a54b4u + (i) * 4)
#define DR_AI_OBJ DR_PTR(void, 0x002aadf8u)

/* fields of player record i (see drillers.h) */
#define DR_P_X(i) DR_PF(i, 0x04, float)
#define DR_P_Z(i) DR_PF(i, 0x08, float)
#define DR_P_HEADING(i) DR_PF(i, 0x0c, float)
#define DR_P_SPEED(i) DR_PF(i, 0x18, float)
#define DR_P_COL(i) DR_PF(i, 0x1c, signed char)
#define DR_P_ROW(i) DR_PF(i, 0x1d, signed char)
#define DR_P_LASTCOL(i) DR_PF(i, 0x1e, signed char)
#define DR_P_LASTROW(i) DR_PF(i, 0x1f, signed char)
#define DR_P_PLAYING(i) DR_PF(i, 0x20, signed char)
#define DR_P_TRAIL_LEN(i) DR_PF(i, 0x64, short)
#define DR_P_TURN(i) DR_PF(i, 0x66c, int)
#define DR_P_TARGETCOL(i) DR_PF(i, 0x730, signed char)
#define DR_P_TARGETROW(i) DR_PF(i, 0x731, signed char)

/* the height of cell x, z (0x400 when whole) */
#define DR_HEIGHT(x, z) (DR_MAP_CELL(g_DrMap, x, z)->height)

/* 0x0026cbac  vtable of the AI object (rebuilt from the XBE; the word after slot 0 is
   the float -180.0, not a pointer) */
const void *const vtable_0026cbac[1] = {
	(const void *)DrAi_DeletingDtor, /* +0x00 */
};

/* 0x00121590  Drillers (gap; file not known; not in the function table)
Is the way ahead of player `player` (on heading `heading`) blocked: the edge of the map, a
drilled cell, or another player in play on the next cell or the one after it, or with
the next cell as its target? */
int FUN_00121590(float heading, int player)
{
	int col, row, k;

	switch (DR_FTOL(heading)) {
	case 90:
		col = DR_P_COL(player);
		if (col == DR_HALF_X * 2 - 1)
			return 1;
		row = DR_P_ROW(player);
		if (DR_HEIGHT(col + 1, row) < 0x400)
			return 1;
		for (k = 0; k < 4; k++) {
			if (k == player || DR_P_PLAYING(k) == 0)
				continue;
			if (col + 1 == DR_P_COL(k) && row == DR_P_ROW(k))
				return 1;
			if (col + 2 == DR_P_COL(k) && row == DR_P_ROW(k))
				return 1;
			if (col + 1 == DR_P_TARGETCOL(k) && row == DR_P_TARGETROW(k))
				return 1;
		}
		return 0;
	case 0:
		row = DR_P_ROW(player);
		if (row == DR_HALF_Z * 2 - 1)
			return 1;
		col = DR_P_COL(player);
		if (DR_HEIGHT(col, row + 1) < 0x400)
			return 1;
		for (k = 0; k < 4; k++) {
			if (k == player || DR_P_PLAYING(k) == 0)
				continue;
			if (col == DR_P_COL(k) && row + 1 == DR_P_ROW(k))
				return 1;
			if (col == DR_P_COL(k) && row + 2 == DR_P_ROW(k))
				return 1;
			if (col == DR_P_TARGETCOL(k) && row + 1 == DR_P_TARGETROW(k))
				return 1;
		}
		return 0;
	case 180:
		row = DR_P_ROW(player);
		if (row == 0)
			return 1;
		col = DR_P_COL(player);
		if (DR_HEIGHT(col, row - 1) < 0x400)
			return 1;
		for (k = 0; k < 4; k++) {
			if (k == player || DR_P_PLAYING(k) == 0)
				continue;
			if (col == DR_P_COL(k) && row - 1 == DR_P_ROW(k))
				return 1;
			if (col == DR_P_COL(k) && row - 2 == DR_P_ROW(k))
				return 1;
			if (col == DR_P_TARGETCOL(k) && row - 1 == DR_P_TARGETROW(k))
				return 1;
		}
		return 0;
	case 270:
		col = DR_P_COL(player);
		if (col == 0)
			return 1;
		row = DR_P_ROW(player);
		if (DR_HEIGHT(col - 1, row) < 0x400)
			return 1;
		for (k = 0; k < 4; k++) {
			if (k == player || DR_P_PLAYING(k) == 0)
				continue;
			if (col - 1 == DR_P_COL(k) && row == DR_P_ROW(k))
				return 1;
			if (col - 2 == DR_P_COL(k) && row == DR_P_ROW(k))
				return 1;
			if (col - 1 == DR_P_TARGETCOL(k) && row == DR_P_TARGETROW(k))
				return 1;
		}
		return 0;
	}
	return 0;
}

/* 0x00121880  Drillers (gap; file not known; not in the function table)
The target cell of player `player`: the next cell on its heading. */
void FUN_00121880(int player)
{
	switch (DR_FTOL(DR_P_HEADING(player))) {
	case 180:
		DR_P_TARGETCOL(player) = DR_P_COL(player);
		DR_P_TARGETROW(player) = (signed char)(DR_P_ROW(player) - 1);
		break;
	case 0:
		DR_P_TARGETCOL(player) = DR_P_COL(player);
		DR_P_TARGETROW(player) = (signed char)(DR_P_ROW(player) + 1);
		break;
	case 90:
		DR_P_TARGETCOL(player) = (signed char)(DR_P_COL(player) + 1);
		DR_P_TARGETROW(player) = DR_P_ROW(player);
		break;
	case 270:
		DR_P_TARGETCOL(player) = (signed char)(DR_P_COL(player) - 1);
		DR_P_TARGETROW(player) = DR_P_ROW(player);
		break;
	}
}

/* 0x00121920  Drillers (gap; file not known)
The difference a - b of two headings (degrees), brought into -180..180 (inputs within
two turns). `self` (the AI object) is not used. */
float DrAi_AngleDiff(void *self, float a, float b)
{
	float d = a - b;

	(void)self;
	if (d > 360.0f)
		d = d - 720.0f;
	if (d < -360.0f)
		d = d + 720.0f;
	if (d <= -180.0f || d > 180.0f) {
		if (d <= 180.0f)
			return d + 360.0f;
		return d - 360.0f;
	}
	return d;
}

/* 0x00121990  Drillers (gap; file not known)
Is the point px, pz inside the polygon of `n` points (x, z pairs; the last point repeats
the first)? Crossing count along +x over the n - 1 edges, from the last edge back. `self`
(the AI object) is not used. */
char DrAi_InPolygon(void *self, float px, float pz, int n, const float *poly)
{
	const float *a;
	const float *b;
	unsigned int count = 0;
	int k;

	(void)self;
	for (k = n - 1; 0 < k; k--) {
		a = &poly[(k - 1) * 2];
		b = &poly[k * 2];
		if ((pz <= a[1]) == (pz <= b[1]))
			continue;
		if ((px <= a[0]) == (px <= b[0])) {
			if (px <= a[0])
				count++;
		} else if (px <= b[0] - ((b[1] - pz) * (a[0] - b[0])) / (a[1] - b[1])) {
			count++;
		}
	}
	return (char)(count & 1);
}

/* 0x00121a80  Drillers (gap; file not known; its switch case 2 is the table entry
0x00121b30)
The AI tuning for the variant and the CPU difficulty (DAT_002bae70: 0, 1, 2). */
void DrAi_Init(void *self)
{
	AI(0x18, float) = 0.6f;
	if (g_DrVariant == 1)
		AI(0x1c, float) = 16.0f;
	else
		AI(0x1c, float) = 24.0f;
	AI(0x28, float) = 1.0f;
	AI(0x2c, float) = 0.0f;
	AI(0x30, float) = 1.0f;
	AI(0x34, float) = 1.0f;
	AI(0x38, float) = 1.0f;
	AI(0x40, float) = 0.0f;
	AI(0x3c, float) = 0.0f;
	AI(0x44, float) = 0.0f;
	AI(0x48, float) = 0.0f;
	switch (g_DrVariant) {
	case 1:
		AI(0x04, float) = 1.0f;
		AI(0x08, float) = 0.4f;
		AI(0x0c, float) = 0.0f;
		AI(0x10, float) = 0.0f;
		AI(0x14, float) = 0.0f;
		AI(0x20, float) = 0.0f;
		AI(0x24, float) = 1.0f;
		if (DAT_002bae70 == 0) {
			AI(0x04, float) = 0.8f;
			AI(0x08, float) = 1.0f;
		} else if (DAT_002bae70 == 1) {
			AI(0x08, float) = 0.4f;
			AI(0x04, float) = 0.8f;
		} else if (DAT_002bae70 == 2) {
			AI(0x04, float) = 1.0f;
			AI(0x08, float) = 0.1f;
		}
		break;
	case 2:
		AI(0x04, float) = 0.5f;
		AI(0x08, float) = 0.2f;
		AI(0x0c, float) = 0.0f;
		AI(0x10, float) = 0.0f;
		AI(0x14, float) = 0.5f;
		AI(0x20, float) = 0.0f;
		AI(0x24, float) = 1.0f;
		if (DAT_002bae70 == 0) {
			AI(0x04, float) = 0.2f;
			AI(0x08, float) = 1.0f;
			AI(0x14, float) = 0.4f;
		} else if (DAT_002bae70 == 1) {
			AI(0x04, float) = 0.5f;
			AI(0x14, float) = 0.5f;
			AI(0x08, float) = 0.2f;
		} else if (DAT_002bae70 == 2) {
			AI(0x04, float) = 1.0f;
			AI(0x08, float) = 0.1f;
			AI(0x14, float) = 1.0f;
		}
		break;
	case 3:
		AI(0x04, float) = 1.0f;
		AI(0x08, float) = 1.0f;
		AI(0x0c, float) = 0.0f;
		AI(0x10, float) = 0.0f;
		AI(0x14, float) = 0.0f;
		AI(0x20, float) = 10.0f;
		AI(0x24, float) = 1.0f;
		AI(0x18, float) = 0.7f;
		if (DAT_002bae70 == 0) {
			AI(0x04, float) = 0.8f;
			AI(0x08, float) = 0.8f;
		} else if (DAT_002bae70 == 1) {
			AI(0x04, float) = 0.9f;
			AI(0x08, float) = 0.9f;
		} else if (DAT_002bae70 == 2) {
			AI(0x04, float) = 1.0f;
			AI(0x08, float) = 1.0f;
		}
		break;
	case 4:
		AI(0x04, float) = 0.0f;
		AI(0x08, float) = 1.0f;
		AI(0x0c, float) = 1.0f;
		AI(0x10, float) = 0.0f;
		AI(0x14, float) = 1.0f;
		AI(0x20, float) = 32.0f;
		AI(0x24, float) = 1.0f;
		if (DAT_002bae70 == 0) {
			AI(0x0c, float) = 0.8f;
			AI(0x14, float) = 0.8f;
			AI(0x08, float) = 0.8f;
		} else if (DAT_002bae70 == 1) {
			AI(0x0c, float) = 0.9f;
			AI(0x14, float) = 0.9f;
			AI(0x08, float) = 0.9f;
		} else if (DAT_002bae70 == 2) {
			AI(0x08, float) = 1.0f;
			AI(0x0c, float) = 1.0f;
			AI(0x14, float) = 1.0f;
		}
		break;
	}
}

/* 0x00121c60  Drillers (gap; file not known; vtable 0x0026cbac +0x00) */
void *DrAi_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026cbac;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* the free cells in a line from cell col, row in direction dx, dz (one of them 0), up to
   `reach` cells: whole cells, not the outer row or column */
static int dr_ai_room(int col, int row, int dx, int dz, float reach)
{
	int count = 0;
	int k = 1;
	int x, z;

	if (!(1.0f < reach))
		return 0;
	for (;;) {
		x = col + k * dx;
		z = row + k * dz;
		if ((dx < 0 && x <= 0) || (dx > 0 && DR_HALF_X * 2 - 1 <= x))
			break;
		if ((dz < 0 && z <= 0) || (dz > 0 && DR_HALF_Z * 2 - 1 <= z))
			break;
		if (DR_HEIGHT(x, z) < 0x400)
			break;
		count++;
		k++;
		if (!((float)k < reach))
			break;
	}
	return count;
}

/* the 90 degree turn of FUN_00121c80: to the side with more room (`left` the room for a
   turn of -90, `right` for +90), a coin toss between equal sides, none without room */
static void dr_ai_turn_to_room(int player, int left, int right)
{
	if (left > right) {
		DR_P_HEADING(player) = DR_P_HEADING(player) - 90.0f;
	} else if (left < right) {
		DR_P_HEADING(player) = DR_P_HEADING(player) + 90.0f;
	} else if (left != 0 || right != 0) {
		if ((dr_rand() & 1) != 0)
			DR_P_HEADING(player) = DR_P_HEADING(player) - 90.0f;
		else
			DR_P_HEADING(player) = DR_P_HEADING(player) + 90.0f;
	}
}

/* 0x00121c80  Drillers (gap; file not known; not in the function table)
A CPU player of variant 1 (Drill Rally) on a new cell: the speed (256; on hard, 496 for 4
frames in 8), no boost; then when the cell ahead is blocked (the edge, a drilled cell, or
with caution 0.5 or less another player in play), or by chance 1 in 8, a turn to the
side with more room (reach: skill * 100 / 3 + 10 cells). */
void FUN_00121c80(void *self, int player_arg)
{
	signed char player = (signed char)player_arg;
	float caution = AI(0x08, float) * 100.0f;
	float reach = (float)((double)AI(0x04, float) * 100.0 * (double)0.33333334f + 10.0);
	int col = DR_P_COL(player);
	int row;
	int blocked, k, tx, tz, dir;
	int left, right;

	if (DR_P_LASTCOL(player) == DR_P_COL(player) && DR_P_LASTROW(player) == DR_P_ROW(player))
		return;
	row = DR_P_ROW(player);
	DR_P_LASTROW(player) = (signed char)row;
	DR_VAR(int, 0x003020ccu) += 1;
	DR_P_LASTCOL(player) = (signed char)col;
	DR_BOOST(player) = 0.0f;
	DR_P_SPEED(player) = 256.0f;
	if (DAT_002bae70 == 2 &&
		(signed char)((player << 5) + DR_VAR(int, 0x003020ccu)) < 0)
		DR_P_SPEED(player) = 496.0f;

	dir = DR_FTOL(DR_P_HEADING(player));
	switch (dir) {
	case 90:
		tx = col + 1;
		tz = row;
		blocked = col == DR_HALF_X * 2 - 1 ? 1 : DR_HEIGHT(col + 1, row) < 0x400;
		break;
	case 0:
		tx = col;
		tz = row + 1;
		blocked = row == DR_HALF_Z * 2 - 1 ? 1 : DR_HEIGHT(col, row + 1) < 0x400;
		break;
	case 180:
		tx = col;
		tz = row - 1;
		blocked = row == 0 ? 1 : DR_HEIGHT(col, row - 1) < 0x400;
		break;
	case 270:
		tx = col - 1;
		tz = row;
		blocked = col == 0 ? 1 : DR_HEIGHT(col - 1, row) < 0x400;
		break;
	default:
		return;
	}
	for (k = 0; k < 4; k++) {
		if (!(caution > 50.0f) && k != player && DR_P_PLAYING(k) == 1 &&
			DR_P_COL(k) == tx && DR_P_ROW(k) == tz)
			blocked = 1;
	}
	if ((dr_rand() & 7) != 0 && !blocked)
		return;
	switch (dir) {
	case 90:
		right = dr_ai_room(col, row, 0, -1, reach);
		left = dr_ai_room(col, row, 0, 1, reach);
		break;
	case 0:
		left = dr_ai_room(col, row, -1, 0, reach);
		right = dr_ai_room(col, row, 1, 0, reach);
		break;
	case 180:
		right = dr_ai_room(col, row, -1, 0, reach);
		left = dr_ai_room(col, row, 1, 0, reach);
		break;
	default: /* 270 */
		left = dr_ai_room(col, row, 0, -1, reach);
		right = dr_ai_room(col, row, 0, 1, reach);
		break;
	}
	dr_ai_turn_to_room(player, left, right);
}

/* a heading 90 degrees off, brought into 0..270 (above 270: 0; below 0: 270) */
static float dr_ai_wrap(float heading)
{
	if (!(heading <= 270.0f))
		return 0.0f;
	if (heading < 0.0f)
		return 270.0f;
	return heading;
}

/* 0x00122530  Drillers (gap; file not known; not in the function table)
A CPU player of variant 2 (Erodeo) on a new cell: the speed (128; on hard, 248 for 4 frames
in 8), no boost. When the way ahead is blocked it turns to a free side (a coin toss when
both are free); else it heads for the nearest token (on easy, gold tokens only): along the
axis with the larger distance, unless it already moves toward the token on its axis, and
on easy and medium only by chance 1 in 2. Then the target cell follows the heading. */
void FUN_00122530(void *self, int player_arg)
{
	signed char player = (signed char)player_arg;
	float left, right, leftw, rightw;
	float px, pz, best, dx, dz;
	int k, nearest, ahead;
	DrTokenSlot *t;

	(void)self;
	if (DR_P_LASTCOL(player) == DR_P_COL(player) && DR_P_LASTROW(player) == DR_P_ROW(player))
		return;
	DR_P_LASTCOL(player) = DR_P_COL(player);
	DR_P_LASTROW(player) = DR_P_ROW(player);
	DR_BOOST(player) = 0.0f;
	DR_P_SPEED(player) = 128.0f;
	if (DAT_002bae70 == 2 &&
		(signed char)((player << 5) + DR_VAR(int, 0x003020d0u)) < 0)
		DR_P_SPEED(player) = 248.0f;
	DR_VAR(int, 0x003020d0u) += 1;

	if (FUN_00121590(DR_P_HEADING(player), player) != 0) {
		left = DR_P_HEADING(player) - 90.0f;
		leftw = dr_ai_wrap(left);
		right = DR_P_HEADING(player) + 90.0f;
		rightw = dr_ai_wrap(right);
		if (FUN_00121590(leftw, player) != 0) {
			DR_P_HEADING(player) = right;
		} else if (FUN_00121590(rightw, player) != 0) {
			DR_P_HEADING(player) = left;
		} else {
			DR_P_HEADING(player) =
				(float)(0x5a - (dr_rand() & 1) * 0xb4) + DR_P_HEADING(player);
		}
		FUN_00121880(player);
		return;
	}

	/* the nearest token, in cells from the player's cell. Token 0 is always taken
	   first; the original starts the best distance from the stack slot that held the
	   row number (an int read as a float, at most a denormal), so with token 0
	   inactive only a token on the player's own cell is found. */
	px = (float)DR_P_COL(player);
	pz = (float)DR_P_ROW(player);
	nearest = -1;
	best = dr_bitsf((unsigned int)(int)DR_P_ROW(player));
	for (k = 0; k < 22; k++) {
		double ddx, ddz, dd;

		t = &g_DrFx->tokens[k];
		if (t->active != 0 && (DAT_002bae70 != 0 || t->type == 2)) {
			ddx = ((double)DR_HALF_X + t->x) - px;
			ddz = ((double)DR_HALF_Z + t->z) - pz;
			dd = sqrt(ddx * ddx + ddz * ddz);
			if (k == 0 || dd < (double)best) {
				best = (float)dd;
				nearest = k;
			}
		}
	}
	if (nearest != -1) {
		t = &g_DrFx->tokens[nearest];
		dx = (float)(((double)DR_HALF_X + t->x) - px);
		dz = (float)(((double)DR_HALF_Z + t->z) - pz);
		ahead = 0;
		if (DR_P_HEADING(player) == 270.0f && dx < 0.0f)
			ahead = 1;
		if (DR_P_HEADING(player) == 90.0f && dx > 0.0f)
			ahead = 1;
		if (DR_P_HEADING(player) == 180.0f && dz < 0.0f)
			ahead = 1;
		if (DR_P_HEADING(player) == 0.0f && dz > 0.0f)
			ahead = 1;
		if ((DAT_002bae70 >= 2 || (dr_rand() & 1) == 0) && !ahead) {
			if (fabsf(dz) < fabsf(dx)) {
				if (dx < 0.0f)
					DR_P_HEADING(player) = 270.0f;
				else
					DR_P_HEADING(player) = 90.0f;
			} else {
				if (dz < 0.0f)
					DR_P_HEADING(player) = 180.0f;
				else
					DR_P_HEADING(player) = 0.0f;
			}
		}
	}
	FUN_00121880(player);
}

/* 0x00122960  Drillers (gap; file not known; not in the function table)
The first trail segment of a player in play that the probe x0, z0 - x1, z1 crosses (the
player's own first four segments left out; all in 1/10 units): 1000 less the distance
from the probe's start to the crossing, or 0 when nothing is crossed. */
float FUN_00122960(void *self, int player_arg, float x0, float z0, float x1, float z1)
{
	signed char player = (signed char)player_arg;
	float probe[4], seg[4];
	struct BHit2D hit;
	double dx, dz;
	int i, j;

	(void)self;
	/* the original never clears hit.flags before or-ing in 8 (a stack word) */
	hit.flags = 0;
	probe[0] = x0 * 0.1f;
	probe[1] = z0 * 0.1f;
	probe[2] = x1 * 0.1f;
	probe[3] = z1 * 0.1f;
	for (i = 0; i < 4; i++) {
		if (DR_P_PLAYING(i) != 1)
			continue;
		for (j = 0; j < DR_P_TRAIL_LEN(i) - 2; j++) {
			if (player == i && j < 4)
				continue;
			hit.flags |= 8;
			seg[0] = DR_TRAIL_X(i, j) * 0.1f;
			seg[1] = DR_TRAIL_Z(i, j) * 0.1f;
			seg[2] = DR_TRAIL_X(i, j + 1) * 0.1f;
			seg[3] = DR_TRAIL_Z(i, j + 1) * 0.1f;
			if (bmSegmentSegment(probe, seg, &hit) != 0) {
				dx = (double)probe[0] - hit.point[0];
				dz = (double)probe[1] - hit.point[1];
				return (float)(1000.0 - sqrt(dz * dz + dx * dx));
			}
		}
	}
	return 0.0f;
}

/* 0x00122ac0  Drillers (gap; file not known; not in the function table)
As FUN_00122960 for the 16 snare loops (those on, not of player `player`): 1000 less the
distance to the first loop segment crossed, or 0. */
float FUN_00122ac0(void *self, int player_arg, float x0, float z0, float x1, float z1)
{
	signed char player = (signed char)player_arg;
	float probe[4], seg[4];
	struct BHit2D hit;
	double dx, dz;
	int k, j;

	(void)self;
	hit.flags = 0;
	probe[0] = x0 * 0.1f;
	probe[1] = z0 * 0.1f;
	probe[2] = x1 * 0.1f;
	probe[3] = z1 * 0.1f;
	for (k = 0; k < 16; k++) {
		if (!(0 < DR_LOOP(k, 0x6acc, int)) || DR_LOOP(k, 0x6ad8, int) == player)
			continue;
		for (j = 0; j < DR_LOOP(k, 0x6ad4, int) - 1; j++) {
			seg[0] = DR_LOOP_X(k, j) * 0.1f;
			hit.flags |= 8;
			seg[1] = DR_LOOP_Z(k, j) * 0.1f;
			seg[2] = DR_LOOP_X(k, j + 1) * 0.1f;
			seg[3] = DR_LOOP_Z(k, j + 1) * 0.1f;
			if (bmSegmentSegment(probe, seg, &hit) != 0) {
				dx = (double)probe[0] - hit.point[0];
				dz = (double)probe[1] - hit.point[1];
				return (float)(1000.0 - sqrt(dz * dz + dx * dx));
			}
		}
	}
	return 0.0f;
}

#define DR_DEG_TO_RAD 0.017453292f
#define DR_RAD_TO_DEG 57.29578f

/* the five probes of the steering in variants 3 and 4: at heading -60, -30, 0, +30, +60
   degrees, `range` long from the head of the trail (x, z); `test` gives 1000 less the
   distance to what each crosses, or 0 */
typedef float (*DrAiProbe)(void *self, int player, float x0, float z0, float x1, float z1);

static void dr_ai_probes(void *self, int player, float range, DrAiProbe test, float d[5])
{
	static const float offsets[5] = {-60.0f, -30.0f, 0.0f, 30.0f, 60.0f};
	float x0 = DR_TRAIL_X((signed char)player, 0);
	float z0 = DR_TRAIL_Z((signed char)player, 0);
	double a;
	int k;

	for (k = 0; k < 5; k++) {
		a = ((double)DR_P_HEADING((signed char)player) + offsets[k]) * (double)DR_DEG_TO_RAD;
		d[k] = test(self, player, x0, z0, (float)(sin(a) * (double)range + x0),
			(float)(cos(a) * (double)range + z0));
	}
}

/* 0x00122c10  Drillers (gap; file not known; not in the function table)
A CPU player of variants 3 and 4 (the free-moving jet bikes): the speed (1024 + the
bonus); five probes against the trails (range skill * 400 + 500, half when there are
human players and none of them still plays). With nothing ahead, by chance 1 in 16 a new
random drift of -1, 0 or +1 degree per frame; else a sharp turn (16) away from the side
with more danger. Unless the humans are all out, a player farther than (3 * player + 25)
cells from the centre eases 1/8 of the way back toward it. The turn eases off: +-2 ends
it, a sharp turn loses 1 a frame. */
void FUN_00122c10(void *self, int player_arg)
{
	signed char player = (signed char)player_arg;
	float range, px, pz, d[5];
	double dist, diff;
	int humans = 0, playing, all_out = 0;
	int turn;

	if (DR_PF(0, 0x704, char) != 0)
		humans = 1;
	if (DR_PF(1, 0x704, char) != 0)
		humans++;
	if (DR_PF(2, 0x704, char) != 0)
		humans++;
	if (DR_PF(3, 0x704, char) != 0)
		humans++;
	if (humans != 0) {
		playing = 0;
		if (DR_PF(0, 0x704, char) != 0 && DR_P_PLAYING(0) != 0)
			playing = 1;
		if (DR_PF(1, 0x704, char) != 0 && DR_P_PLAYING(1) != 0)
			playing++;
		if (DR_PF(2, 0x704, char) != 0 && DR_P_PLAYING(2) != 0)
			playing++;
		if (DR_PF(3, 0x704, char) != 0 && DR_P_PLAYING(3) != 0)
			playing++;
		if (playing == 0)
			all_out = 1;
	}
	DR_P_SPEED(player) = DR_SPEED_BONUS + 1024.0f;
	range = (float)((double)AI(0x04, float) * 100.0 * 4.0 + 500.0);
	if (all_out)
		range = range * 0.5f;
	dr_ai_probes(self, player_arg, range, FUN_00122960, d);

	if (d[2] == 0.0f && d[1] == 0.0f && d[3] == 0.0f && d[0] == 0.0f && d[4] == 0.0f) {
		if (((dr_rand()) & 0xf) == 0)
			DR_P_TURN(player) = dr_rand() % 3 - 1;
	} else if (d[4] + d[3] < d[1] + d[0]) {
		DR_P_TURN(player) = 0x10;
	} else {
		DR_P_TURN(player) = -0x10;
	}

	if (!all_out) {
		px = (float)(((double)DR_P_X(player) * (double)0.0009765625f - DR_HALF_X) *
			(double)DR_CELL_SIZE);
		pz = (float)(((double)DR_P_Z(player) * (double)0.0009765625f - DR_HALF_Z) *
			(double)DR_CELL_SIZE);
		dist = sqrt((double)pz * pz + (double)px * px);
		if ((double)DR_CELL_SIZE * ((double)(player * 3) + 25.0) < dist) {
			diff = atan2(px, pz) * (double)DR_RAD_TO_DEG + 180.0 -
				(double)DR_P_HEADING(player);
			if (diff > 360.0)
				diff = diff - 720.0;
			if (diff < -360.0)
				diff = diff + 720.0;
			if (diff <= -180.0 || diff > 180.0) {
				if (diff <= 180.0)
					diff = diff + 360.0;
				else
					diff = diff - 360.0;
			}
			DR_P_HEADING(player) = (float)(diff * 0.125 + (double)DR_P_HEADING(player));
		}
	}

	turn = DR_P_TURN(player);
	switch (turn) {
	case -2:
	case 2:
		DR_P_TURN(player) = 0;
		return;
	case -1:
	case 0:
	case 1:
		return;
	}
	if (turn < -2)
		DR_P_TURN(player) = turn + 1;
	if (2 < DR_P_TURN(player))
		DR_P_TURN(player) = DR_P_TURN(player) - 1;
	DR_P_HEADING(player) = (float)DR_P_TURN(player) + DR_P_HEADING(player);
}

/* 0x001230b0  Drillers (gap; file not known; not in the function table)
A CPU player of variant 4 (Quantum Snare) drawing a loop: the speed (1024 + the bonus);
five probes (range 600) against the other players' snare loops. With nothing ahead and a
trail of 10 points or more, it circles the nearest token: the aim point lies 500 to its
side (left for even players, right for odd); once the aim is within 300 the loop is ended
(player +0x674 = -1), else the player turns toward it by a step of 16 (easy), 11
(medium) or 6 (hard), +-4 at random. With a loop ahead, a turn of 16 away from the side
with more danger. Without an active token the aim is the point (-1, the player number
read as a float), as in the original. */
void FUN_001230b0(void *self, int player_arg)
{
	signed char player = (signed char)player_arg;
	float d[5];
	float x0, z0, best, aimz, ax, dz, tx, tz, af;
	double aimx, a, dist, half, cell, txe, tze;
	int k, nearest, step;
	DrTokenSlot *t;

	DR_P_SPEED(player) = DR_SPEED_BONUS + 1024.0f;
	x0 = DR_TRAIL_X(player, 0);
	z0 = DR_TRAIL_Z(player, 0);
	step = 0;
	dr_ai_probes(self, player_arg, 600.0f, FUN_00122ac0, d);

	if (!(d[2] == 0.0f && d[1] == 0.0f && d[3] == 0.0f && d[0] == 0.0f && d[4] == 0.0f)) {
		if (d[4] + d[3] < d[1] + d[0])
			DR_P_HEADING(player) = DR_P_HEADING(player) + 16.0f;
		else
			DR_P_HEADING(player) = DR_P_HEADING(player) - 16.0f;
		return;
	}
	if (DR_P_TRAIL_LEN(player) < 10)
		return;

	best = -1.0f;
	nearest = -1;
	aimx = -1.0;
	aimz = dr_bitsf((unsigned int)player_arg);
	for (k = 0; k < 22; k++) {
		t = &g_DrFx->tokens[k];
		if (t->active == 0)
			continue;
		cell = (double)DR_CELL_SIZE;
		half = (double)(DR_CELL_SIZE / 2);
		txe = cell * t->x + half;
		tze = cell * t->z + half;
		dist = sqrt(tze * (float)tze + txe * txe);
		if (dist < (double)best || best == -1.0f) {
			best = (float)dist;
			nearest = k;
		}
	}
	if (nearest != -1) {
		t = &g_DrFx->tokens[nearest];
		cell = (double)DR_CELL_SIZE;
		half = (double)(DR_CELL_SIZE / 2);
		tx = (float)(cell * t->x + half);
		tz = (float)(cell * t->z + half);
		a = atan2((double)tx - x0, (double)tz - z0);
		if ((player_arg & 1) != 0)
			a = a + (double)1.5707964f;
		else
			a = a - (double)1.5707964f;
		af = (float)a;
		aimx = sin(a) * 500.0 + tx;
		aimz = (float)(cos((double)af) * 500.0 + tz);
	}
	switch (DAT_002bae70) {
	case 0:
		step = 16;
		break;
	case 1:
		step = 11;
		break;
	case 2:
		step = 6;
		break;
	}
	ax = (float)(aimx - x0);
	dz = aimz - z0;
	if (!(300.0 < sqrt(((double)aimz - z0) * dz + (double)ax * ax))) {
		DR_PF(player, 0x674, int) = -1;
		return;
	}
	step = step + (int)(Core_Rand() & 7) - 4;
	if (DrAi_AngleDiff(self, (float)(atan2((double)ax, (double)dz) * (double)DR_RAD_TO_DEG),
		    DR_P_HEADING(player)) > 0.0f)
		DR_P_HEADING(player) = (float)step + DR_P_HEADING(player);
	else
		DR_P_HEADING(player) = DR_P_HEADING(player) - (float)step;
}
