/*
CharLib/Route.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\Route.cpp (tags at 0x000ad7d0 and 0x000ad8b0).

The route grid: route_cols x route_rows cells over the level, each with the ground heights
found by ray casts, and a depth-first route search between two cells that the AI players
use to walk around obstacles. Found routes are cached per (cell, destination) pair.

0x000ab4e0-0x000ad620 lie in the gap in front of the first Route.cpp tag and work on the
same grid: placed here (inferred, same object file).
*/

#include "charlib.h"

/* 0x002f289c  the AI players, one per player slot (CharLib .bss); defined in AI.c */
#define route_ai_players CHL_ARR(void *, 0x002f289c)

/* 0x000ab4e0  CharLib (Route.cpp, inferred)
Forgets every cached route ("not searched" for every cell pair) and tells each AI player
to plan again (sets its +0x98 flag and calls its virtual method 6 with -1). */
void CLRoute_ClearKnown(void)
{
	int cell;
	int dest;
	int i;

	route_stat_hops = 0;
	for (cell = 0; cell < route_cells; cell++)
		for (dest = 0; dest < route_cells; dest++)
			route_grid[cell].known[dest] = 0;
	for (i = 0; i < chl_player_count; i++) {
		void *ai = route_ai_players[i];

		if (ai != NULL) {
			CHL_I16(ai, 0x98) = 1;
			((void (*)(void *, int))(*(void ***)ai)[6])(ai, -1);
		}
	}
}

/* the ray-cast ground probe of CRCollision (0x000b3ff0) */
#define route_GroundProbe(pos, radius, tri, platform, mode) \
	FUN_000b3ff0((pos), (radius), (tri), (platform), (mode))

/* 0x000ab570  CharLib (Route.cpp, inferred)
Samples the ground of every grid cell with 3 x 3 vertical rays from y = 990000. The
downward pass (probe mode 1) gives top/bottom (a cell whose highest hit is above
CHL_NO_HEIGHT gets CHL_NO_HEIGHT for both), the upward pass (mode -1) top_up/bottom_up.
Positions are counted in half cells: 1, 3, 5 ... are the cell centres. */
void CLRoute_SampleHeights(void)
{
	float cols2 = (float)(route_cols * 2);
	float rows2 = (float)(route_rows * 2);
	float start_y = 990000.0f;
	float pos[4];
	void *tri;
	void *platform;
	float hx;
	float hz;
	float ox;
	float oz;
	float highest;
	float lowest;
	float h;
	float inv_cols2;
	float inv_rows2;
	struct CLRouteCell *cell;
	int col;
	int row;
	int pass;

	pos[1] = start_y;
	pos[3] = 1.0f;
	for (pass = 0; pass < 2; pass++) {
		for (hx = 1.0f; hx <= cols2; hx += 2.0f) {
			inv_cols2 = 1.0f / cols2;
			inv_rows2 = 1.0f / rows2;
			col = CHL_FTOL(hx) / 2;
			for (hz = 1.0f; hz <= rows2; hz += 2.0f) {
				highest = -1e6f;
				lowest = 1e6f;
				for (ox = -1.0f; ox <= 1.0f; ox += 1.0f) {
					for (oz = -1.0f; oz <= 1.0f; oz += 1.0f) {
						pos[0] = inv_cols2 * route_size_x * (route_sample_spread * ox + hx) +
							route_origin_x;
						pos[2] = inv_rows2 * route_size_z * (route_sample_spread * oz + hz) +
							route_origin_z;
						h = route_GroundProbe(pos, 0.0f, &tri, &platform, pass == 0 ? 1 : -1);
						if (h != CHL_NO_HEIGHT)
							h = start_y - h;
						if (highest < h)
							highest = h;
						if (h < lowest)
							lowest = h;
					}
				}
				row = CHL_FTOL(hz) / 2;
				cell = &route_grid[row * route_cols + col];
				if (pass == 0) {
					if (!(highest <= CHL_NO_HEIGHT)) {
						cell->bottom = CHL_NO_HEIGHT;
						cell->top = CHL_NO_HEIGHT;
					} else {
						cell->top = highest;
						cell->bottom = lowest;
					}
				} else {
					cell->top_up = highest;
					cell->bottom_up = lowest;
				}
			}
		}
	}
}

/* one step of CLRoute_Search: tries to push `next` (a neighbour of `cur`, which is at
row/col) on the search stack. The step must not climb more than route_max_climb (nor drop
more than route_max_drop when route_check_drop is set), must improve the cost of `next`,
and the cost so far plus the grid distance from `cur` to the goal must not exceed the
best cost found for the goal. */
static int route_try(struct CLRoute *r, int cur, int next, int row, int col)
{
	struct CLRouteCell *grid = route_grid;
	float step;
	int a;
	int b;
	int cost_a;
	int dr;
	int dc;

	r->path[r->depth + 1] = (short)next;
	r->from[r->depth + 1] = (short)cur;
	a = r->from[r->depth + 1];
	b = r->path[r->depth + 1];
	step = grid[a].top - grid[b].bottom;
	if (!(step <= route_max_climb))
		return 0;
	if (route_check_drop != 0 && !(route_max_drop <= step))
		return 0;
	cost_a = r->cost[a];
	if (!(cost_a + 1 < r->cost[b]))
		return 0;
	dr = row - r->goal_row;
	dc = col - r->goal_col;
	if (dr < 0)
		dr = -dr;
	if (dc < 0)
		dc = -dc;
	if (dc + cost_a + dr > r->cost[r->src_cell])
		return 0;
	r->cost[b] = (short)(cost_a + 1);
	r->parent[r->path[r->depth + 1]] = r->from[r->depth + 1];
	r->depth = r->depth + 1;
	return 1;
}

/* 0x000ab9f0  CharLib (Route.cpp, inferred)
The route search from cell `start` (dst_cell) towards src_cell: a depth-first search that
prefers the neighbours in the direction of the goal and prunes with the cost table.
Returns -1 when route_max_iterations steps ran out (call again to continue), 1 when the
stack ran empty and the goal was reached (or, with a log message, when the stack grew
deeper than CL_ROUTE_MAX_DEPTH), 0 when the goal cannot be reached. */
int CLRoute_Search(int start, struct CLRoute *r)
{
	int iterations = 0;
	int cur;
	int row;
	int col;
	int cols;
	int rows;

	if (r->state == 0) {
		r->path[1] = (short)start;
		r->depth = 1;
	}
	for (;;) {
		iterations++;
		if (route_max_iterations < iterations)
			return -1;
		if (CL_ROUTE_MAX_DEPTH < r->depth) {
			chl_Printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGHHHHHHHHHHHH!!!!!!!\n");
			return 1;
		}
		if (route_stat_depth < r->depth)
			route_stat_depth = r->depth;
		cur = r->path[r->depth];
		cols = route_cols;
		rows = route_rows;
		row = cur / cols;
		col = cur % cols;

		/* the neighbour order: first the column towards the goal, then the rows (towards
		the goal first), then the column away from it */
		if (r->goal_col < col) {
			if (0 < col && route_try(r, cur, cur - 1, row, col))
				continue;
			if (r->goal_row < row) {
				if (0 < row && route_try(r, cur, cur - cols, row, col))
					continue;
				if (row < rows - 1 && route_try(r, cur, cur + cols, row, col))
					continue;
			} else {
				if (row < rows - 1 && route_try(r, cur, cur + cols, row, col))
					continue;
				if (0 < row && route_try(r, cur, cur - cols, row, col))
					continue;
			}
			if (col < cols - 1 && route_try(r, cur, cur + 1, row, col))
				continue;
		} else if (col < r->goal_col) {
			if (col < cols - 1 && route_try(r, cur, cur + 1, row, col))
				continue;
			if (r->goal_row < row) {
				if (0 < row && route_try(r, cur, cur - cols, row, col))
					continue;
				if (row < rows - 1 && route_try(r, cur, cur + cols, row, col))
					continue;
			} else {
				if (row < rows - 1 && route_try(r, cur, cur + cols, row, col))
					continue;
				if (0 < row && route_try(r, cur, cur - cols, row, col))
					continue;
			}
			if (0 < col && route_try(r, cur, cur - 1, row, col))
				continue;
		} else if (row <= r->goal_row) {
			if (row < rows - 1 && route_try(r, cur, cur + cols, row, col))
				continue;
			if (0 < col && route_try(r, cur, cur - 1, row, col))
				continue;
			if (col < cols - 1 && route_try(r, cur, cur + 1, row, col))
				continue;
			if (0 < row && route_try(r, cur, cur - cols, row, col))
				continue;
		} else {
			if (0 < row && route_try(r, cur, cur - cols, row, col))
				continue;
			if (0 < col && route_try(r, cur, cur - 1, row, col))
				continue;
			if (col < cols - 1 && route_try(r, cur, cur + 1, row, col))
				continue;
			if (row < rows - 1 && route_try(r, cur, cur + cols, row, col))
				continue;
		}
		/* no neighbour left: back up */
		r->depth = r->depth - 1;
		if (r->depth == 0)
			return r->cost[r->src_cell] != 32000;
	}
}

/* 0x000ad4b0  CharLib (Route.cpp, inferred)
The grid cell under a world position (x, z), clamped to [0, route_cells - 1]. */
int CLRoute_CellAt(float *pos)
{
	int col = CHL_FTOL(((double)pos[0] - (double)route_origin_x) * (double)route_cols /
		(double)route_size_x);
	int row = CHL_FTOL(((double)pos[2] - (double)route_origin_z) * (double)route_rows /
		(double)route_size_z);
	int cell = route_cols * row + col;
	int last = route_cells - 1;
	int low = last;

	if (cell <= last)
		low = cell;
	if (low < 0)
		return 0;
	if (last < cell)
		cell = last;
	return cell;
}

/* 0x000ad520  CharLib (Route.cpp, inferred)
The centre of a cell: x and z of its middle, y its lowest ground. (The row is computed
with route_rows as the divisor; the grid is square.) */
void CLRoute_CellCentre(float *out, int cell)
{
	out[0] = route_size_x / (float)(route_cols * 2) +
		(route_size_x / (float)route_cols) * (float)(cell % route_cols) + route_origin_x;
	out[2] = route_size_z / (float)(route_rows * 2) +
		(route_size_z / (float)route_rows) * (float)(cell / route_rows) + route_origin_z;
	out[1] = route_grid[cell].bottom;
}

/* 0x000ad5c0  CharLib (Route.cpp, inferred)
Sets the cell heights from precomputed buffers (top_up = top) and forgets the routes. */
void CLRoute_SetHeights(struct CLRouteHeights *heights)
{
	int i;

	for (i = 0; i < route_cells; i++) {
		route_grid[i].bottom = heights->bottom[i];
		route_grid[i].top = heights->top[i];
		route_grid[i].top_up = route_grid[i].top;
	}
	CLRoute_ClearKnown();
}

/* 0x000ad620  CharLib (Route.cpp, inferred)
Debug view: one flat quad per cell at top_up + route_debug_lift, black for cells without
ground and grey otherwise. */
void CLRoute_DrawDebug(void)
{
	float half_x = route_size_x / (float)(route_cols * 2);
	float half_z = route_size_z / (float)(route_rows * 2);
	unsigned char *v = route_debug_quad;
	float centre[3];
	int col;
	int row;
	int cell;
	int shade;
	unsigned int colour;
	float y;

	chl_SetTexture(0, NULL);
	chl_SetRenderState(6, 0, 0);
	chl_SetRenderState(4, 0, 0);
	for (col = 0; col < route_cols; col++) {
		for (row = 0; row < route_rows; row++) {
			cell = route_cols * row + col;
			CLRoute_CellCentre(centre, cell);
			if (CHL_U32(&route_grid[cell], 0x08) == 0x49742400) /* top == 1e6 */
				shade = 0;
			else
				shade = 0x7f;
			y = route_debug_lift + route_grid[cell].top_up;
			colour = DAT_002bd640[shade] | DAT_002bc490[shade] | DAT_002bc898[0x60] |
				DAT_002bce18[0x7f];
			/* four vertices of 0x24 bytes: x, y, z at 0x0, colour at 0x18 */
			CHL_F32(v, 0x00) = centre[0] - half_x;
			CHL_F32(v, 0x04) = y;
			CHL_F32(v, 0x08) = centre[2] - half_z;
			CHL_U32(v, 0x18) = colour;
			CHL_F32(v, 0x24) = centre[0] + half_x;
			CHL_F32(v, 0x28) = y;
			CHL_F32(v, 0x2c) = centre[2] - half_z;
			CHL_U32(v, 0x3c) = colour;
			CHL_F32(v, 0x48) = centre[0] - half_x;
			CHL_F32(v, 0x4c) = y;
			CHL_F32(v, 0x50) = centre[2] + half_z;
			CHL_U32(v, 0x60) = colour;
			CHL_F32(v, 0x6c) = centre[0] + half_x;
			CHL_F32(v, 0x70) = y;
			CHL_F32(v, 0x74) = centre[2] + half_z;
			CHL_U32(v, 0x84) = colour;
			chl_DrawPrimitive(4, v, 4);
		}
	}
	chl_SetRenderState(4, 1, 0);
}

/* 0x000ad7d0  CharLib/Route.cpp
Height buffers: one allocation of 2 x route_cells floats. */
struct CLRouteHeights *CLRouteHeights_Ctor(struct CLRouteHeights *self)
{
	float *buffer = chl_Alloc(route_cells * 8, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x221,
		0x2001);

	self->top = buffer;
	self->bottom = buffer + route_cells;
	return self;
}

/* 0x000ad810  CharLib/Route.cpp (bracketed) */
void CLRouteHeights_Dtor(struct CLRouteHeights *self)
{
	chl_Free(self->top);
	self->top = NULL;
}

/* 0x000ad830  CharLib/Route.cpp (missing from the function table: Ghidra did not make a
function here; between 0x000ad810 and 0x000ad880)
The 0x18-byte route object that CGameMode_Meta allocates per route cell (new[]): two
buffers, route_cells * 2 and route_cells bytes. CLRouteHeights_Free2 frees them. */
undefined4 *FUN_000ad830(undefined4 *self)
{
	self[0] = (undefined4)(uintptr_t)chl_Alloc(route_cells * 2,
		"C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23d, 0x2001);
	self[1] = (undefined4)(uintptr_t)chl_Alloc(route_cells,
		"C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23e, 0x2001);
	return self;
}

/* 0x000ad880  CharLib/Route.cpp (bracketed)
Frees both pointers separately (no caller in the game). */
void CLRouteHeights_Free2(struct CLRouteHeights *self)
{
	chl_Free(self->top);
	self->top = NULL;
	chl_Free(self->bottom);
	self->bottom = NULL;
}

/* 0x000ad8b0  CharLib/Route.cpp */
struct CLRoute *CLRoute_Ctor(struct CLRoute *self)
{
	self->cost = chl_Alloc(route_cells * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 599, 0x2001);
	self->waypoints = chl_Alloc(route_cells * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 600,
		0x2001);
	self->parent = chl_Alloc(route_cells * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x259,
		0x2001);
	return self;
}

/* 0x000ad930  CharLib (Route.cpp, inferred) */
void CLRoute_Dtor(struct CLRoute *self)
{
	chl_Free(self->cost);
	self->cost = NULL;
	chl_Free(self->waypoints);
	self->waypoints = NULL;
	chl_Free(self->parent);
	self->parent = NULL;
}

/* 0x000ad980  CharLib (Route.cpp, inferred) */
void CLRoute_Rebuild(void)
{
	CLRoute_SampleHeights();
	CLRoute_ClearKnown();
}

/* 0x000ad990  CharLib (Route.cpp, inferred)
Runs (or continues) the search for a route from src_cell to dst_cell. A cached result is
used when there is one. Returns 1 when a route exists or the search goes on, 0 when there
is no route (or the cells are invalid). A finished search stores the route in the
next_hop tables of the cells on it. */
int CLRoute_Update(struct CLRoute *r)
{
	unsigned long long t0 = chl_TimerRead();
	int src = r->src_cell;
	int dst = r->dst_cell;
	int result;
	int i;

	if (src == dst || src < 0 || route_cells <= src || dst < 0 || route_cells <= dst)
		return 0;
	if (route_grid[src].known[dst] != 0) {
		r->state = 0;
		return route_grid[src].known[dst] == 1;
	}
	if (r->state == 0) {
		for (i = 0; i < route_cells; i++)
			r->cost[i] = 32000;
	}
	r->cost[r->dst_cell] = 0;
	r->goal_col = (short)(r->src_cell % route_cols);
	r->goal_row = (short)(r->src_cell / route_cols);
	result = CLRoute_Search(r->dst_cell, r);
	if (result == -1) {
		r->state = r->state + 1;
	} else {
		unsigned int cell;

		if (result == 0) {
			route_grid[r->src_cell].known[r->dst_cell] = -1;
			return 0;
		}
		cell = (unsigned int)(int)r->src_cell;
		r->state = 0;
		if (r->cost[cell] != 32000) {
			do {
				route_grid[cell].known[r->dst_cell] = 1;
				route_grid[cell].next_hop[r->dst_cell] = r->parent[cell];
				cell = (unsigned short)route_grid[cell].next_hop[r->dst_cell];
				route_stat_hops = route_stat_hops + 1;
			} while (cell != (unsigned int)(int)r->dst_cell);
		}
	}
	chl_TimerDelta(t0, chl_TimerRead());
	return 1;
}

/* 0x000adb70  CharLib (Route.cpp, inferred)
Starts a route from `from` to `to` (world positions, x y z w) for an actor at height y.
Returns the first cell to walk to (CLRoute_NextCell), which is also kept in next_cell. */
int CLRoute_Start(struct CLRoute *r, float *from, float *to, float y)
{
	short first = (short)CLRoute_CellAt(from);
	short result;

	r->path[0] = 0;
	r->dst_cell = (short)CLRoute_CellAt(to);
	r->patience = route_patience;
	r->waypoint_count = 1;
	r->cur_cell = first;
	r->waypoints[0] = first;
	r->src_cell = first;
	CLRoute_CellCentre(r->dst_centre, r->dst_cell);
	r->dst_pos[0] = to[0];
	r->dst_pos[1] = to[1];
	r->dst_pos[2] = to[2];
	r->dst_pos[3] = to[3];
	r->state = 0;
	result = (short)CLRoute_NextCell(from, to, y, r);
	r->next_cell = result;
	r->stuck_time = 0.0f;
	r->arrive_radius = 0.0f;
	return result;
}

/* 0x000adc30  CharLib (Route.cpp, inferred)
A random cell centre (x, top, z, 1). With need_ground != 0, draws again until the cell has
ground. */
void CLRoute_RandomPoint(float *out, int need_ground)
{
	int col;
	int row;
	int cell;

	do {
		col = CHL_Rand() % route_cols;
		row = CHL_Rand() % route_rows;
		cell = route_cols * row + col;
		if (route_grid[cell].top < CHL_NO_HEIGHT)
			break;
	} while (need_ground != 0);
	out[0] = (route_size_x / (float)route_cols) * (float)col + route_size_x / (float)(route_cols * 2) +
		route_origin_x;
	out[2] = (route_size_z / (float)route_rows) * (float)row + route_size_z / (float)(route_rows * 2) +
		route_origin_z;
	out[1] = route_grid[cell].top;
	out[3] = 1.0f;
}
