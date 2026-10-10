/*
CHARLIB.H

Shared declarations of the charlib module (original source C:\BlitzParty\Game\Src\CharLib\:
Route, Actor, CRCollision, AI, lights, Platform, Collect, CharLibShadow, CCamera and the gap
functions around them). Three text blocks: 0x000ab4c0-0x000ba6d0, 0x000be6d0-0x000bf080
and 0x000d4890-0x000d7660 (docs/modules.tsv).

Other modules may include this header. Only the charlib agent edits it.

Conventions (docs/ARCHITECTURE.md):
- Layouts are the original 32-bit ones. Objects whose full layout is not known are reached
  through the offset accessors below (CHL_F32(p, 0xd0) is the float at p + 0xd0) and the
  named field macros of each object.
- charlib's own globals live in data blocks with the original layout (charlib_data.h);
  they are named here with readable macros. Globals of other modules keep their Ghidra
  names (DAT_<addr>); CharLib/charlib_extern.c gives them tentative (or weak, for
  initialised data) definitions.
- Calls into other modules go through the function-pointer casts in charlib_extern.h,
  which name the prototype the x86 call sites show (thiscall `this` first).
*/

#ifndef OPENFRENZY_CHARLIB_H
#define OPENFRENZY_CHARLIB_H

#include "xtl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charlib_data.h"

/* ======================================================================== helpers */

/* field accessors: the TYPE at byte offset OFF of object P */
#define CHL_AT(type, p, off) (*(type *)((char *)(p) + (off)))
#define CHL_F32(p, off) CHL_AT(float, p, off)
#define CHL_I32(p, off) CHL_AT(int, p, off)
#define CHL_U32(p, off) CHL_AT(unsigned int, p, off)
#define CHL_I16(p, off) CHL_AT(short, p, off)
#define CHL_U16(p, off) CHL_AT(unsigned short, p, off)
#define CHL_I8(p, off) CHL_AT(signed char, p, off)
#define CHL_U8(p, off) CHL_AT(unsigned char, p, off)
#define CHL_PTR(p, off) CHL_AT(void *, p, off)
/* address of the field at byte offset OFF */
#define CHL_FIELD(type, p, off) ((type *)((char *)(p) + (off)))

/* MSVC rand(), inlined into the game: holdrand is the C library's seed at 0x00285630
(initial value 1, owned by the C runtime). Ghidra prints the increment 0x269ec3 as
"<string at 0x00269eb0> + 0x13". */
extern unsigned int DAT_00285630;
static inline int CHL_Rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* x87 _ftol (FUN_001de998): truncating conversion */
#define CHL_FTOL(x) ((int)(longlong)(x))

/* x87 FPATAN as Ghidra prints it: fpatan(y, x) = atan2(y, x) */
#define CHL_FPATAN(y, x) atan2((double)(y), (double)(x))

/* "no ground" height of the route grid and of the collision queries (1e6) */
#define CHL_NO_HEIGHT 1000000.0f

#include "charlib_extern.h"

/* ======================================================================== shared state */

/* 0x0028fda8  frame time scale: 1.0 at the nominal frame rate (classlib_ui .data) */
extern float DAT_0028fda8;
#define chl_time_scale DAT_0028fda8

/* 0x002bae94  number of players in the game (core .bss) */
extern int DAT_002bae94;
#define chl_player_count DAT_002bae94

/* 0x002ea488  the CharLib actor list (a CLList *, ClassLib .bss) */
extern int *DAT_002ea488;
#define chl_actor_list DAT_002ea488

/* ======================================================================== route grid (Route.cpp)

The level is covered by a grid of route_cols x route_rows cells (24 x 24 over 2400 x 2400
units from (-1200, -1200)). Each cell keeps the ground heights sampled by ray casts and,
per destination cell, the next cell on the route there once a search found it. */

struct CLRouteCell {
	short *next_hop;      /* 0x00 per destination cell: next cell towards it */
	signed char *known;   /* 0x04 per destination cell: 0 not searched, 1 route, -1 none */
	float top;            /* 0x08 highest ground in the cell (CHL_NO_HEIGHT: no ground) */
	float bottom;         /* 0x0c lowest ground in the cell */
	float top_up;         /* 0x10 highest hit of the upward ray pass */
	float bottom_up;      /* 0x14 lowest hit of the upward ray pass */
};
OF_ASSERT_SIZE(struct CLRouteCell, 0x18);

/* a route search between two cells: a depth-first search that runs at most
route_max_iterations steps per call and continues on the next call */
#define CL_ROUTE_MAX_DEPTH 1000
struct CLRoute {
	int state;            /* 0x000 0: start a new search; >0: calls the search ran so far */
	int unk004;           /* 0x004 route_unk_100 (100) at start */
	int depth;            /* 0x008 */
	short path[CL_ROUTE_MAX_DEPTH];  /* 0x00c cells on the search stack (path[0] unused) */
	short from[CL_ROUTE_MAX_DEPTH];  /* 0x7dc the cell each stack entry was reached from */
	int pad_fac;          /* 0xfac */
	short *cost;          /* 0xfb0 per cell: steps from dst_cell (32000: not reached) */
	short first_cell;     /* 0xfb4 */
	short unk_fb6;        /* 0xfb6 FUN_000b6860 result */
	short src_cell;       /* 0xfb8 the cell the actor is in (the search goal) */
	short dst_cell;       /* 0xfba the destination cell (the search start) */
	short goal_col;       /* 0xfbc column of src_cell */
	short goal_row;       /* 0xfbe row of src_cell */
	short *waypoints;     /* 0xfc0 */
	short waypoint_count; /* 0xfc4 */
	short pad_fc6;        /* 0xfc6 */
	short *parent;        /* 0xfc8 per cell: the cell it was reached from */
	float dst_centre[4];  /* 0xfcc centre of dst_cell (x, ground, z) */
	float dst_pos[4];     /* 0xfdc the destination position */
	int unk_fec;          /* 0xfec */
	int unk_ff0;          /* 0xff0 */
};
OF_ASSERT_OFFSET(struct CLRoute, from, 0x7dc);
OF_ASSERT_OFFSET(struct CLRoute, cost, 0xfb0);
OF_ASSERT_OFFSET(struct CLRoute, parent, 0xfc8);
OF_ASSERT_OFFSET(struct CLRoute, unk_ff0, 0xff0);

/* height buffers handed to CLRoute_SetHeights (Route.cpp 0x000ad7d0) */
struct CLRouteHeights {
	float *top;           /* 0x0 route_cells floats */
	float *bottom;        /* 0x4 route_cells floats (same allocation) */
};

/* 0x00292c48  most search steps per CLRoute_Search call (3000) */
#define route_max_iterations CHL_VAR(int, 0x00292c48)
/* 0x00292c4c / 0x00292c50  grid columns / rows (24 x 24) */
#define route_cols CHL_VAR(int, 0x00292c4c)
#define route_rows CHL_VAR(int, 0x00292c50)
/* 0x00292c54  initial CLRoute.unk004 (100) */
#define route_unk_100 CHL_VAR(int, 0x00292c54)
/* 0x00292c58  highest step up between neighbour cells (350) */
#define route_max_climb CHL_VAR(float, 0x00292c58)
/* 0x00292c5c  lowest step (largest drop) when route_check_drop is set (-10000) */
#define route_max_drop CHL_VAR(float, 0x00292c5c)
/* 0x00292c60 / 0x00292c64  world x / z of the grid corner (-1200, -1200) */
#define route_origin_x CHL_VAR(float, 0x00292c60)
#define route_origin_z CHL_VAR(float, 0x00292c64)
/* 0x00292c68 / 0x00292c6c  world size of the grid in x / z (2400, 2400) */
#define route_size_x CHL_VAR(float, 0x00292c68)
#define route_size_z CHL_VAR(float, 0x00292c6c)
/* 0x00292c70  spread of the 3 x 3 height samples in a cell (0.9 half cells) */
#define route_sample_spread CHL_VAR(float, 0x00292c70)
/* 0x00292c74  height of the debug grid above the ground (10) */
#define route_debug_lift CHL_VAR(float, 0x00292c74)
/* 0x00292c78  number of cells (576) */
#define route_cells CHL_VAR(int, 0x00292c78)

/* 0x002f27d4  routes stored in the next-hop tables (statistic) */
#define route_stat_hops CHL_VAR(int, 0x002f27d4)
/* 0x002f27d8  the grid (struct CLRouteCell[route_cells]) */
#define route_grid CHL_VAR(struct CLRouteCell *, 0x002f27d8)
/* 0x002f27dc  deepest search stack seen (statistic) */
#define route_stat_depth CHL_VAR(int, 0x002f27dc)
/* 0x002f27e0  also refuse steps down by more than route_max_drop */
#define route_check_drop CHL_VAR(int, 0x002f27e0)
/* 0x002f27e8  the four vertices of the debug grid quad (0x24 bytes each) */
#define route_debug_quad CHL_ARR(unsigned char, 0x002f27e8)

#endif
