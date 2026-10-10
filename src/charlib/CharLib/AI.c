/*
CharLib/AI.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\AI.cpp (tag at 0x000b6710, source line 179).

Route helpers of the AI players on the route grid (Route.c): straight-line walkability
between two cells, the farthest cell of a route that can be reached in a straight line,
and the next cell to walk to.

0x000b6520-0x000b66a0 lie just before the AI.cpp tag and are only used by it: placed
here (inferred).
*/

#include "charlib.h"

/* the step test of the line walks: from the previous cell's lowest ground at y, the next
cell's highest ground may not climb more than route_max_climb (nor drop more than
route_max_drop when route_check_drop is set) */
static int ai_step_ok(int cell, float *y)
{
	float step = route_grid[cell].top - *y;

	if (route_max_climb < step || (route_check_drop != 0 && step < route_max_drop))
		return 0;
	*y = route_grid[cell].bottom;
	return 1;
}

/* 0x000b6520  CharLib (AI.cpp, inferred)
Walks the cells of a line that is longer in x (Bresenham: dx steps of sx in x, dz rows of
sz in z) from (col, row) starting at height y; 1 when every step can be walked. */
int AI_WalkLineX(int col, int row, int dx, int dz, int sx, int sz, float y)
{
	int err2 = dz - dx;
	int err = dz * 2 - dx;
	int base;
	int add;

	if (dx != 0) {
		base = route_cols * row;
		do {
			dx--;
			add = dz * 2;
			if (-1 < err) {
				base = base + route_cols * sz;
				add = err2 * 2;
			}
			err = err + add;
			col = col + sx;
			if (!ai_step_ok(base + col, &y))
				return 0;
		} while (dx != 0);
	}
	return 1;
}

/* 0x000b65e0  CharLib (AI.cpp, inferred)
The same for a line that is longer in z. */
int AI_WalkLineZ(int col, int row, int dx, int dz, int sx, int sz, float y)
{
	int err2 = dx - dz;
	int err = dx * 2 - dz;
	int base;
	int add;

	if (dz != 0) {
		base = route_cols * row;
		do {
			dz--;
			add = dx * 2;
			if (-1 < err) {
				col = col + sx;
				add = err2 * 2;
			}
			err = err + add;
			base = base + route_cols * sz;
			if (!ai_step_ok(base + col, &y))
				return 0;
		} while (dz != 0);
	}
	return 1;
}

/* the line walk from cell a to (cdx, cdz) cells away (signed) */
static int ai_walk_line(int a, int dx, int dz, float y)
{
	int sx;
	int sz;

	if (dx < 1) {
		dx = -dx;
		if (dz < 1) {
			dz = -dz;
			sz = -1;
		} else {
			sz = 1;
		}
		sx = -1;
	} else if (dz < 1) {
		sz = -1;
		dz = -dz;
		sx = 1;
	} else {
		sz = 1;
		sx = 1;
	}
	if (dz < dx)
		return AI_WalkLineX(a % route_cols, a / route_cols, dx, dz, sx, sz, y);
	return AI_WalkLineZ(a % route_cols, a / route_cols, dx, dz, sx, sz, y);
}

/* 0x000b66a0  CharLib (AI.cpp, inferred)
1 when the AI can walk in a straight line from cell a to cell b (starting at height y). */
int AI_CanWalkStraight(int a, int b, float y)
{
	return ai_walk_line(a, b % route_cols - a % route_cols, b / route_cols - a / route_cols, y);
}

/* 0x000b6710  CharLib/AI.cpp
Follows the route from `from` to `to` (through the cells' next-hop tables, or through
r->parent when a search object is given) and returns the farthest cell on it (but not the
first step) that can be walked to in a straight line from `from`; -1 when there is none or
the route is broken. */
int CLRoute_PathStep(struct CLRoute *r, int from, int to, float y)
{
	unsigned short *path = chl_Alloc(route_cells * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\AI.cpp", 0xb3, 0x2001);
	unsigned int cell = (unsigned int)from & 0xffff;
	unsigned short next;
	int n = 0;
	int visible = 0;

	if (r == NULL) {
		while (cell != (unsigned int)to) {
			if (route_cells <= n)
				goto fail;
			next = (unsigned short)route_grid[cell].next_hop[to];
			path[n] = next;
			n++;
			cell = next;
		}
	} else {
		while (cell != (unsigned int)to) {
			if (route_cells <= n)
				goto fail;
			next = (unsigned short)r->parent[cell];
			path[n] = next;
			n++;
			cell = next;
		}
	}
	if (n < route_cells) {
		while (n != 0) {
			if (visible != 0) {
				next = path[n];
				chl_Free(path);
				return next;
			}
			n--;
			cell = path[n];
			visible = ai_walk_line(from, (int)(cell % (unsigned int)route_cols) - from % route_cols,
				(int)(cell / (unsigned int)route_cols) - from / route_cols, y);
		}
	}
fail:
	chl_Free(path);
	return -1;
}

/* 0x000b6860  CharLib (AI.cpp, inferred)
The next cell to walk to from `from` towards `to`: `to` itself when it can be walked to
in a straight line; else (re)runs the route search: -2 while it is still running, the
farthest straight-line cell of the route or the next hop when found, -1 when there is no
route. */
int CLRoute_NextCell(float *from, float *to, float y, struct CLRoute *r)
{
	int a = CLRoute_CellAt(from);
	int b = CLRoute_CellAt(to);
	int step;

	if (AI_CanWalkStraight(a, b, y) != 0)
		return b;
	r->src_cell = (short)a;
	r->dst_cell = (short)b;
	if (CLRoute_Update(r) != 0) {
		if (r->state != 0)
			return -2;
		if (route_grid[a].known[b] == 1) {
			step = CLRoute_PathStep(NULL, a, b, y);
			if (step != -1)
				return step;
			return (unsigned short)route_grid[a].next_hop[b];
		}
	}
	return -1;
}
