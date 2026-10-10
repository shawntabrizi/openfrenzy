/*
Drillers/unplaced_map.c  <-  C:\BlitzParty\Game\Src\Drillers\ (file not known)

The map of Rubble Alliance and Erodeo (DrMap, drillers.h; one static object at 0x00298da0,
vtable 0x0026c294): 2W x 2H cells of 80 units (Erodeo 160) that the drillers drill down,
each drawn as a box of 19 vertices in one vertex buffer, the grid lines, the layers under
the map and the edge circle of Quantum Snare.

These gap functions lie between CDrillers_Effects.cpp (0x00114230) and CDrillers_Lom.cpp
(0x0011b470); no assert names their file. 0x001151f0, 0x001154b0, 0x00115760, 0x001160f0
and 0x001164d0 are not in of_functions.tsv. The table entry 0x00115443 is a chunk of
0x00115410 (Drillers/drillers_chunks.c).
*/

#include "drillers.h"

/* the map size: cell size, half the number of cells across z and x */
#define DR_CELL DR_VAR(int, 0x002ff9e0u)
#define DR_HALF_Z DR_VAR(int, 0x002ff9e4u)
#define DR_HALF_X DR_VAR(int, 0x002ff9e8u)
/* the game's frame counter (game +0x1c) and round (game +0x68) */
#define DR_FRAME DR_VAR(int, 0x002ffa0cu)
#define DR_ROUND DR_VAR(int, 0x002ffa58u)
/* the per-round angles of the layers (game +0xa0) */
#define DR_ROUND_ANGLES(i) DR_VAR(int, 0x002ffa90u + (i) * 4)
/* game +0x34: draw the arena of variants 3 and 4 */
#define DR_DRAW_ARENA DR_VAR(signed char, 0x002ffa14u)
/* the textures of the layers, one per round (resources) */
#define DR_TEX_LAYER(i) DR_VAR(int, 0x002aa930u + (i) * 4)
/* the arena actor of Erodeo (resources) */
#define DR_ARENA_ACTOR DR_VAR(struct BActorInstance *, 0x002aaaf4u)
/* the "cells drop" sample (resources) */
#define DR_SAMPLE_DROP DR_VAR(struct BSample *, 0x002aa9c0u)
/* tuning values (float; 0 switches the feature off) */
#define DR_TUNE_CELLS DR_VAR(float, 0x002aae30u)     /* draw the cells */
#define DR_TUNE_FLATTEN DR_VAR(float, 0x002aae38u)   /* a falling cell is gone at once */
#define DR_TUNE_NO_EDGE DR_VAR(float, 0x002aae20u)   /* no edge circle (variants 3, 4) */
/* the last vertex buffer locks */
#define DR_LOCK_CELLS DR_VAR(DrMapVertex *, 0x00301c80u)
#define DR_LOCK_GRID DR_VAR(bdVertex *, 0x00301c84u)
#define DR_LOCK_GRID2 DR_VAR(bdVertex *, 0x00301c88u)
/* the four vertices of the layer quads and the edge lines */
#define DR_VTX_LAYER DR_PTR(bdVertex, 0x00301c90u)
/* the players' cells (player +0x18, +0x19) */
#define DR_PLAYER_X(i) DR_PL(signed char, i, 0x00298e8cu)
#define DR_PLAYER_Z(i) DR_PL(signed char, i, 0x00298e8du)

#define DR_DEG 0.017453292f

/* 0x0026c294  vtable of the map (rebuilt from the XBE) */
const void *const vtable_0026c294[1] = {
	(const void *)DrMap_DeletingDtor, /* +0x00 */
};

/* the corners of a cell box and the 19-vertex strip around them (top, then the sides) */
static const unsigned char dr_box_strip[DR_CELL_VERTS] = {
	0, 0, 1, 2, 3, 3, 7, 2, 6, 0, 4, 1, 5, 3, 7, 5, 6, 4, 4,
};

/* 0x001148f0  Drillers (gap; file not known)
Fills cell x, z (from the centre): full height, inner, solid, plain look. */
void DrMap_FillCell(DrMap *self, char x, char z)
{
	DrCell *cell = &self->columns[DR_HALF_X + x][DR_HALF_Z + z];

	cell->height = 0x400;
	cell->state = 1;
	cell->unk2d3 = 1;
	cell->look = 0;
}

/* the grid colour */
static unsigned int dr_grid_colour(DrMap *self)
{
	return DAT_002bd640[self->gridB] | DAT_002bce18[self->gridR] | DAT_002bc490[self->gridG] |
		DAT_002bca28;
}

/* 0x00114990  Drillers (gap; file not known)
Builds the map: the grid lines (green), the box of every cell (19 vertices: position,
normal, grey top and red sides, the texture across the whole map), and fills every
cell. */
void DrMap_Build(DrMap *self)
{
	float corner[8][3];
	float in[4];
	float out[4];
	float cell = (float)DR_CELL;
	float scale;
	float u;
	float w;
	bdVertex *g;
	DrMapVertex *v;
	int k;
	int i;
	int x;
	int z;
	int n;

	corner[0][0] = 0.0f;
	corner[0][1] = 0.0f;
	corner[0][2] = 0.0f;
	corner[1][0] = cell;
	corner[1][1] = 0.0f;
	corner[1][2] = 0.0f;
	corner[2][0] = 0.0f;
	corner[2][1] = 0.0f;
	corner[2][2] = cell;
	corner[3][0] = cell;
	corner[3][1] = 0.0f;
	corner[3][2] = cell;
	corner[4][0] = 0.0f;
	corner[4][1] = -80.0f;
	corner[4][2] = 0.0f;
	corner[5][0] = cell;
	corner[5][1] = -80.0f;
	corner[5][2] = 0.0f;
	corner[6][0] = 0.0f;
	corner[6][1] = -80.0f;
	corner[6][2] = cell;
	corner[7][0] = cell;
	corner[7][1] = -80.0f;
	corner[7][2] = cell;
	scale = 24.0f;
	if (g_DrVariant == 1)
		scale = 11.789474f;
	scale = 1.0f / (512.0f / scale);
	bdVertexBufferCreate(&self->vbCells, 0x76c0, 0x252, 0, 1);
	bdVertexBufferCreate(&self->vbGrid, 0x1900, 0x152, 0, 1);
	bkPrintf("**vertex buffer created\n");

	g = (bdVertex *)bdVertexBufferLock(self->vbGrid, 0);
	self->gridR = 0x20;
	self->gridB = 0x20;
	self->gridG = 0x7f;
	self->numGridVerts = 0;
	DR_LOCK_GRID = g;
	for (i = 0; i <= DR_HALF_Z * 2; i++) {
		n = self->numGridVerts;
		g[n].x = (float)-(DR_HALF_X * DR_CELL);
		g[n].y = 0.0f;
		g[n].z = (float)((i - DR_HALF_Z) * DR_CELL);
		g[n].color = dr_grid_colour(self);
		n = ++self->numGridVerts;
		g[n].x = (float)(DR_HALF_X * DR_CELL);
		g[n].y = 0.0f;
		g[n].z = (float)((i - DR_HALF_Z) * DR_CELL);
		g[n].color = dr_grid_colour(self);
		self->numGridVerts++;
	}
	for (i = 0; i <= DR_HALF_X * 2; i++) {
		n = self->numGridVerts;
		g[n].x = (float)((i - DR_HALF_X) * DR_CELL);
		g[n].y = 0.0f;
		g[n].z = (float)-(DR_HALF_Z * DR_CELL);
		g[n].color = dr_grid_colour(self);
		n = ++self->numGridVerts;
		g[n].x = (float)((i - DR_HALF_X) * DR_CELL);
		g[n].y = 0.0f;
		g[n].z = (float)(DR_HALF_Z * DR_CELL);
		g[n].color = dr_grid_colour(self);
		self->numGridVerts++;
	}
	self->numGridVerts--;
	bdVertexBufferUnlock(self->vbGrid);

	v = (DrMapVertex *)bdVertexBufferLock(self->vbCells, 0);
	self->numCellVerts = 0;
	DR_LOCK_CELLS = v;
	for (x = -DR_HALF_X; x < DR_HALF_X; x++) {
		for (z = -DR_HALF_X; z < DR_HALF_X; z++) {
			for (i = 0; i < DR_CELL_VERTS; i++) {
				DrMapVertex *p = &v[self->numCellVerts];

				k = dr_box_strip[i];
				p->pos[0] = (float)(x * DR_CELL) + corner[k][0];
				p->pos[1] = (float)(i < 5) * 0.1f + corner[k][1];
				p->pos[2] = (float)(z * DR_CELL) + corner[k][2];
				switch (k) {
				case 0:
				case 4:
					u = scale * 0.0f;
					w = scale * 0.0f - 0.00390625f;
					break;
				case 1:
				case 5:
					u = scale;
					w = scale * 0.0f - 0.00390625f;
					break;
				case 2:
				case 6:
					u = scale * 0.0f;
					w = scale - 0.00390625f;
					break;
				default:
					u = scale;
					w = scale - 0.00390625f;
					break;
				}
				if (g_DrVariant == 2) {
					u = u - 0.001953125f;
				} else {
					u = u + 0.00390625f;
					w = w + 0.005859375f;
				}
				u = u + 0.065625f;
				w = w + 0.065625f;
				p->uv0[0] = 1.0f - ((float)(DR_HALF_X + x) * scale + u);
				p->uv0[1] = (float)(DR_HALF_Z + z) * scale + w;
				p->uv1[0] = 1.0f - ((float)(DR_HALF_X + x) * scale + u);
				p->uv1[1] = (float)(DR_HALF_Z + z) * scale + w;
				in[0] = 0.0f;
				in[1] = corner[k][1] - (float)DR_CELL * 0.5f;
				in[2] = 0.0f;
				in[3] = 0.0f;
				bmVectorNormalise(out, in);
				v = DR_LOCK_CELLS;
				p = &v[self->numCellVerts];
				p->normal[0] = -out[0];
				p->normal[1] = -out[1];
				p->normal[2] = -out[2];
				if (i < 5)
					p->colour = DAT_002bca94 | DAT_002bd83c | DAT_002bc68c | DAT_002bd014;
				else
					p->colour = DAT_002bd6a0 | DAT_002bc4f0 | DAT_002bd138 | DAT_002bca94;
				self->numCellVerts++;
			}
		}
	}
	bdVertexBufferUnlock(self->vbCells);
	for (x = 0; x < DR_HALF_X * 2; x++) {
		for (z = 0; z < DR_HALF_Z * 2; z++)
			DrMap_FillCell(self, (char)(x - DR_HALF_X), (char)(z - DR_HALF_Z));
	}
}

/* sets the height of every grid vertex */
static void dr_grid_set_y(DrMap *self, float y)
{
	bdVertex *g = (bdVertex *)bdVertexBufferLock(self->vbGrid, 0);
	int i;

	self->numGridVerts = 0;
	DR_LOCK_GRID2 = g;
	for (i = 0; i <= DR_HALF_Z * 2; i++) {
		g[self->numGridVerts].y = y;
		g[++self->numGridVerts].y = y;
		self->numGridVerts++;
	}
	for (i = 0; i <= DR_HALF_X * 2; i++) {
		g[self->numGridVerts].y = y;
		g[++self->numGridVerts].y = y;
		self->numGridVerts++;
	}
	bdVertexBufferUnlock(self->vbGrid);
}

/* 0x001151f0  Drillers (not in of_functions.tsv)
The grid lines at game over (state 6, at 0) and in Rubble Alliance's drop between rounds
(state 7, at 1600); the heights are set on the first frame of the state. */
void FUN_001151f0(void *map)
{
	DrMap *self = (DrMap *)map;

	if (g_DrState != 6) {
		if (g_DrState != 7) {
			self->drawCount = 0;
			return;
		}
		if (self->drawCount != 0)
			goto draw;
		dr_grid_set_y(self, 1600.0f);
		if (g_DrState != 6)
			goto draw;
	}
	if (self->drawCount == 0)
		dr_grid_set_y(self, 0.0f);
draw:
	self->drawCount++;
	bdSetTexture(0, NULL);
	bdSetWorldIdentity();
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdDrawVertexBuffer(1, self->vbGrid, 0, self->numGridVerts);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
}

/* 0x00115410  Drillers (gap; file not known; the table entry 0x00115443 is its chunk)
Cell x, z (indices) of an inner cell: is it held? A drilled cell is gone (state 0, yes);
a cell next to one held by the edge (state 2, the 8 neighbours) is held too. Returns 0 when
it is not held yet. */
char DrMap_CheckHeld(DrMap *self, char x, char z)
{
	DrCell *cell = &self->columns[(int)x][(int)z];
	int dx;
	int dz;

	if (cell->height < 0x400) {
		cell->state = 0;
		return 1;
	}
	for (dx = -1; dx < 2; dx++) {
		for (dz = -1; dz < 2; dz++) {
			if ((dx != 0 || dz != 0) &&
					self->columns[(int)x + dx][(int)z + dz].state == 2) {
				cell->state = 2;
				return 1;
			}
		}
	}
	return 0;
}

/* 0x001154b0  Drillers (not in of_functions.tsv)
Loose cells: once two cells next to the players are drilled (counted over all players),
the cells that no chain of cells holds to the map's edge are released (state 0; they fall
in DrMap_Draw), with a sound. */
void FUN_001154b0(void *map)
{
	DrMap *self = (DrMap *)map;
	DrCell *cell;
	int holes = 0;
	int loose;
	int changed;
	int dropped;
	int px;
	int pz;
	int p;
	int dx;
	int dz;
	int x;
	int z;

	if (g_DrState == 6)
		return;
	for (p = 0; p < 4; p++) {
		px = DR_PLAYER_X(p);
		pz = DR_PLAYER_Z(p);
		for (dx = -1; dx < 2; dx++) {
			for (dz = -1; dz < 2; dz++) {
				if (dx == 0 && dz == 0)
					continue;
				if (dx == -1 && px == 0)
					continue;
				if ((dz != -1 || pz != 0) && (dx != 1 || px != DR_HALF_X * 2 - 1) &&
						(dz != 1 || pz != DR_HALF_Z * 2 - 1) &&
						self->columns[px + dx][pz + dz].height < 0x400)
					holes++;
			}
		}
		if (holes < 2)
			continue;
		for (z = 0; z < DR_HALF_Z * 2; z++) {
			for (x = 0; x < DR_HALF_X * 2; x++) {
				cell = &self->columns[x][z];
				if (cell->height < 0x400)
					cell->state = 0;
				else if (x == 0 || x == DR_HALF_X * 2 - 1 || z == 0 || z == DR_HALF_Z * 2 - 1)
					cell->state = 2;
				else
					cell->state = 1;
			}
		}
		do {
			changed = 0;
			loose = 0;
			if (DR_HALF_Z * 2 < 1)
				return;
			for (z = 0; z < DR_HALF_Z * 2; z++) {
				for (x = 0; x < DR_HALF_X * 2; x++) {
					if (self->columns[x][z].state == 1) {
						if (DrMap_CheckHeld(self, (char)x, (char)z) == 0)
							loose++;
						else
							changed = 1;
					}
				}
			}
		} while (changed);
		if (loose != 0) {
			dropped = 0;
			for (z = 0; z < DR_HALF_Z * 2; z++) {
				for (x = 0; x < DR_HALF_X * 2; x++) {
					if (self->columns[x][z].state == 1) {
						self->columns[x][z].state = 0;
						dropped++;
					}
				}
			}
			if (dropped != 0)
				bsPlaySample(DR_SAMPLE_DROP, 0x46, 0, 0, 0);
		}
		return;
	}
}

/* 0x00115710  Drillers (gap; file not known) */
void DrMap_Free(DrMap *self)
{
	bdVertexBufferDestroy(self->vbCells);
	bdVertexBufferDestroy(self->vbGrid);
	bkPrintf("**vertex buffer destroyed\n");
}

/* 0x00115740  Drillers (gap; file not known; vtable 0x0026c294 +0x00) */
DrMap *DrMap_DeletingDtor(DrMap *self, unsigned int flags)
{
	self->vtable = vtable_0026c294;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* the second texture coordinates of a cell's look: a 1/32 tile of the detail texture,
   moved by (du, dw) */
static void dr_cell_look(DrMapVertex *v, float du, float dw)
{
	unsigned int i;
	unsigned int n;
	float u;
	float w;

	for (i = 0; (int)i < DR_CELL_VERTS; i = n) {
		n = i + 1;
		u = (float)(n & 1) * 0.03125f;
		w = (float)((n & 3) < 2) * 0.03125f;
		if (u == 0.0f)
			u = u + 0.001953125f;
		else
			u = u - 0.001953125f;
		if (w == 0.0f)
			w = w + 0.001953125f;
		else
			w = w - 0.001953125f;
		v[i].uv1[0] = u + du;
		v[i].uv1[1] = w + dw;
	}
}

/* 0x00115760  Drillers (not in of_functions.tsv)
A falling cell (height below 0x400, not 0): while high, every fourth frame (by column) its
box tips about its centre (more the further it has fallen and the further from the map's
centre) and sinks 32; the height drops 8 (Rubble Alliance) or 12 per call; at 0 the box
is gone. A cell gets a random look as it starts to fall, and cracks below 0x80. The
second texture coordinates follow the look. */
void FUN_00115760(void *map, char cx, char cz, DrMapVertex *verts)
{
	DrMap *self = (DrMap *)map;
	DrMapVertex *box;
	DrCell *cell;
	float t1[16];
	float t2[16];
	float rot[16];
	float m[16];
	float mt[16];
	float p[4];
	float n[4];
	float centre;
	float t;
	float s;
	short tilt;
	int x = cx;
	int z = cz;
	int i;

	box = &verts[(z + x * DR_HALF_X * 2) * DR_CELL_VERTS];
	cell = &self->columns[x][z];
	if (cell->height < 0x400 && cell->height != 0) {
		tilt = (short)(0xff - (short)(((int)cell->height + ((cell->height >> 31) & 7)) >> 3));
		if (0x98 < tilt) {
			if (DR_TUNE_FLATTEN != 0.0f)
				cell->height = 0;
			if (((x + DR_FRAME) & 3) == 0) {
				centre = (box[6].pos[1] + box[0].pos[1]) * 0.5f;
				bmMatrixTranslation(t1,
					-((float)((x - DR_HALF_X) * DR_CELL) + (float)DR_CELL * 0.5f), -centre,
					-((float)((z - DR_HALF_Z) * DR_CELL) + (float)DR_CELL * 0.5f));
				bmMatrixTranslation(t2,
					(float)((x - DR_HALF_X) * DR_CELL) + (float)DR_CELL * 0.5f, centre,
					(float)((z - DR_HALF_Z) * DR_CELL) + (float)DR_CELL * 0.5f);
				t = (float)(tilt - 0xa8);
				s = sinf(DR_DEG * t);
				bmMatrixRotationXYZ(rot, t * 0.125f * (float)(x - DR_HALF_X) * DR_DEG * 0.5f, s,
					(float)(z - DR_HALF_Z) * s * DR_DEG * 0.4f);
				bmMatrixMultiplyCopy(mt, rot, t1);
				bmMatrixMultiplyCopy(m, t2, mt);
			}
			for (i = 0; i < DR_CELL_VERTS; i++) {
				if (((x + DR_FRAME) & 3) == 0) {
					p[0] = box[i].pos[0];
					p[1] = box[i].pos[1];
					p[2] = box[i].pos[2];
					p[3] = 1.0f;
					bmTransformVector4InPlace(m, p);
					box[i].pos[0] = p[0];
					box[i].pos[1] = p[1];
					box[i].pos[2] = p[2];
					p[0] = box[i].normal[0];
					p[1] = box[i].normal[1];
					p[2] = box[i].normal[2];
					p[3] = 1.0f;
					bmTransformVector4InPlace(m, p);
					bmVectorNormalise(n, p);
					box[i].normal[0] = n[0];
					box[i].normal[1] = n[1];
					box[i].normal[2] = n[2];
				}
				box[i].pos[1] = box[i].pos[1] - 32.0f;
			}
		}
		if (g_DrVariant == 1)
			cell->height = (short)(cell->height - 8);
		else
			cell->height = (short)(cell->height - 0xc);
		if (cell->height < 0) {
			for (i = 0; i < DR_CELL_VERTS; i++) {
				box[i].pos[0] = 0.0f;
				box[i].pos[1] = 0.0f;
				box[i].pos[2] = 0.0f;
			}
			cell->height = 0;
		}
		if (cell->height < 0x80) {
			if (cell->look == 1)
				cell->look = 2;
			if (cell->look == 4)
				cell->look = 5;
		} else if (cell->look == 0) {
			if ((dr_rand() & 1) == 0)
				cell->look = 4;
			else
				cell->look = 1;
		}
	}
	if (cell->height == 0)
		return;
	switch (cell->look) {
	case 1:
		dr_cell_look(box, 0.03125f, 0.0f);
		break;
	case 4:
		dr_cell_look(box, 0.0f, 0.03125f);
		break;
	case 2:
		dr_cell_look(box, 0.0625f, 0.0f);
		break;
	case 5:
		dr_cell_look(box, 0.03125f, 0.03125f);
		break;
	case 3:
		dr_cell_look(box, 0.09375f, 0.0f);
		break;
	case 6:
		dr_cell_look(box, 0.0625f, 0.03125f);
		break;
	}
}

/* 0x001160f0  Drillers (not in of_functions.tsv)
Draws the cells (when switched on): at game over every full cell may start to fall (1 in
4 per frame); a released cell starts to fall the same way; the falling cells move (not
while paused). The boxes are drawn with the round's texture blended with a detail
texture on stage 1; in Rubble Alliance not before state 4. Erodeo then draws its arena
actor with the texture factor. */
void FUN_001160f0(void *map)
{
	DrMap *self = (DrMap *)map;
	DrMapVertex *verts;
	DrCell *cell;
	signed char x;
	signed char z;

	if (DR_TUNE_CELLS == 0.0f)
		return;
	if (g_DrState == 6) {
		for (z = 0; z < DR_HALF_Z * 2; z++) {
			for (x = 0; x < DR_HALF_X * 2; x++) {
				cell = &self->columns[(int)x][(int)z];
				if (0x3ff < cell->height && (dr_rand() & 3) == 0)
					cell->height = 0x3ff;
			}
		}
	}
	bdSetWorldIdentity();
	verts = (DrMapVertex *)bdVertexBufferLock(self->vbCells, 0);
	for (z = 0; z < DR_HALF_Z * 2; z++) {
		for (x = 0; x < DR_HALF_X * 2; x++) {
			cell = &self->columns[(int)x][(int)z];
			if (cell->state == 0 && 0x3ff < cell->height && (dr_rand() & 3) == 0)
				cell->height = 0x3ff;
			if (DAT_002ecdec == 0)
				FUN_00115760(self, x, z, verts);
		}
	}
	bdVertexBufferUnlock(self->vbCells);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(9, 0xff, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(7, 2, 0);
	bdPushRenderState(1, 2, 0);
	bdSetTexture(0, (struct bdTexture *)(uintptr_t)DR_TEX_LAYER(DR_ROUND));
	bdSetTexture(1, (struct bdTexture *)(uintptr_t)DR_TEX_LAYER(DR_ROUND));
	D3DDevice_SetTextureStageState(1, D3DTSS_COLOROP, 0xc);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLORARG1, 2);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLORARG2, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	D3DDevice_SetTextureStageState(1, D3DTSS_ADDRESSU, 1);
	D3DDevice_SetTextureStageState(1, D3DTSS_ADDRESSV, 1);
	D3DDevice_SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
	bdSetRenderState(3, 1, 0);
	baSetAmbientLight(0, 0, 0);
	if (g_DrVariant != 1 || 3 < g_DrState)
		bdDrawVertexBuffer(4, self->vbCells, 0, self->numCellVerts);
	baSetAmbientLight(0x40, 0x40, 0x40);
	bdSetRenderState(3, 0, 0);
	bdPopRenderState(1);
	bdSetWorldIdentity();
	bdSetTexture(1, NULL);
	if (g_DrVariant == 2) {
		D3DDevice_SetRenderState_TextureFactor(0x40000000);
		D3DDevice_SetTextureStageState(1, D3DTSS_COLOROP, 0xe);
		D3DDevice_SetTextureStageState(1, D3DTSS_COLORARG1, 2);
		D3DDevice_SetTextureStageState(1, D3DTSS_COLORARG2, 1);
		D3DDevice_SetTextureStageState(1, D3DTSS_ADDRESSU, 1);
		D3DDevice_SetTextureStageState(1, D3DTSS_ADDRESSV, 1);
		D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
		D3DDevice_SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 2);
		baPrepActorInstance(DR_ARENA_ACTOR, 1);
		baDrawActorInstance(DR_ARENA_ACTOR, 1);
		bdSetTexture(1, NULL);
	}
}

/* sets the four vertices of a layer quad at height y */
static void dr_layer_quad(float y)
{
	bdVertex *v = DR_VTX_LAYER;

	v[0].x = (float)(DR_CELL * DR_HALF_X);
	v[0].y = y;
	v[0].z = (float)(DR_HALF_Z * DR_CELL);
	v[1].x = (float)-(DR_CELL * DR_HALF_X);
	v[2].z = (float)-(DR_HALF_Z * DR_CELL);
	v[1].y = v[0].y;
	v[1].z = v[0].z;
	v[2].x = v[0].x;
	v[2].y = v[0].y;
	v[3].x = v[1].x;
	v[3].y = v[0].y;
	v[3].z = v[2].z;
}

/* 0x001164d0  Drillers (not in of_functions.tsv)
Rubble Alliance: the layers under the map (the next rounds' textures, 1600 apart, turned
by the round angles; the current layer's underside one cell down, lit, before the play);
the arena of variants 3 and 4 (0x0011bc40); without the tuning switch 0x002aae20, the edge
circle (36 grey lines, radius 3000) in variants 3 and 4. */
void FUN_001164d0(void *map)
{
	bdVertex *v = DR_VTX_LAYER;
	float m[16];
	float a;
	unsigned int colour;
	int i;

	(void)map;
	if (g_DrVariant == 1) {
		colour = DAT_002bca94 | DAT_002bd740 | DAT_002bc590 | DAT_002bcf18;
		v[0].color = colour;
		v[0].u = 0.0f;
		v[0].v = 1.0f;
		v[1].u = 0.9375f;
		v[1].v = 1.0f;
		v[2].u = 0.0f;
		v[2].v = 0.0625f;
		v[3].u = 0.9375f;
		v[3].v = 0.0625f;
		v[0].nx = 0.0f;
		v[0].ny = 1.0f;
		v[0].nz = 0.0f;
		v[1].nx = 0.0f;
		v[1].ny = 1.0f;
		v[1].nz = 0.0f;
		v[2].nx = 0.0f;
		v[2].ny = 1.0f;
		v[2].nz = 0.0f;
		v[3].nx = 0.0f;
		v[3].ny = 1.0f;
		v[3].nz = 0.0f;
		v[1].color = colour;
		v[2].color = colour;
		v[3].color = colour;
		bdPushRenderState(1, 2, 0);
		for (i = 0; i < 3; i++) {
			if (i == 0) {
				if (4 < g_DrState)
					continue;
				bdPushRenderState(3, 1, 0);
				baSetAmbientLight(0x7f, 0x7f, 0x7f);
			}
			if (((DR_ROUND & 3) != 0 || i != 1) && (DR_ROUND != 3 || i == 0)) {
				bmMatrixRotationY(m, (float)DR_ROUND_ANGLES(i) * DR_DEG);
				bdSetWorldMatrix(m);
				dr_layer_quad((float)i * -1600.0f);
				bdSetTexture(0, (struct bdTexture *)(uintptr_t)DR_TEX_LAYER(DR_ROUND + i));
				bdDrawPrimitiveFVF(4, v, 4, 0x152);
				if (i == 0) {
					dr_layer_quad(-(float)DR_CELL);
					bdSetTexture(0, (struct bdTexture *)(uintptr_t)DR_TEX_LAYER(DR_ROUND));
					bdDrawPrimitiveFVF(4, v, 4, 0x152);
					bdPopRenderState(3);
					baSetAmbientLight(0x40, 0x40, 0x40);
				}
			}
		}
		bdPopRenderState(1);
	}
	bdSetWorldIdentity();
	if (DR_DRAW_ARENA != 0) {
		if (2 < g_DrVariant)
			bdSetRenderState(3, 1, 0);
		FUN_0011bc40(DR_PTR(void, 0x002a54d8u));
		bdSetRenderState(3, 0, 0);
	}
	if (DR_TUNE_NO_EDGE == 0.0f && 2 < g_DrVariant) {
		bdSetTexture(0, NULL);
		bdSetWorldIdentity();
		for (i = 0; i < 0x24; ) {
			a = (float)i * 10.0f * DR_DEG;
			v[0].color = DAT_002bd83c | DAT_002bc68c | DAT_002bd014 | DAT_002bca94;
			i++;
			v[0].y = 0.0f;
			v[1].y = 0.0f;
			v[0].x = sinf(a) * 3000.0f;
			v[0].z = cosf(a) * 3000.0f;
			a = (float)i * 10.0f * DR_DEG;
			v[1].x = sinf(a) * 3000.0f;
			v[1].z = cosf(a) * 3000.0f;
			v[1].color = v[0].color;
			bdDrawPrimitiveFVF(1, v, 2, 0x152);
		}
	}
}
