/*
UNPLACED_TILES.C  <-  gap functions 0x00158e10-0x00159710 of C:\BlitzParty\Game\Src\Meta\
(no source path in the XBE; between CMetaPlayer.cpp and Tiles_Meta.cpp)

The board's tiles and orbs. A tile (0x8c bytes, CLList node in DAT_003082d4):
  +0x10 short owner (-1: none)   +0x12 ushort colour (type 4: a character)
  +0x18 int: hidden              +0x1c int type (0 orb, 2 flag, 3 super, 4 character, 999)
  +0x34 its entity (charlib actor), +0x38 the node in the board's list
  +0x40.. , +0x44 the meta_b object on it, +0x84 its position record (+0x28 position)
The board (DAT_003082c0, CTilePosition_List): +0x00 CLList of tiles (node at tile +0x38),
  +0x0c queue length, +0x10 queue (200 tiles), +0x330 int[1000] group switches,
  +0x12d4 float, +0x12d8 byte[0x14] colours in the queue, +0x12ec their number.
*/

#include "meta_a.h"

/* the squared distance (bkernel FUN_00051540; see puzzles.c) */
#define MA_DISTANCE_SQ(a, b) (((float (*)(const float *, const float *))FUN_00051540)((a), (b)))

#define MA_TILES ((unsigned int *)DAT_003082d4)
#define MA_NEXT(list, node) CLList_Next((unsigned int *)(list), (unsigned int *)(node))

/* the tile type table (0x002b0930, 6 x 0x10 bytes: name, two words, frame count) */
#define MA_TILE_TYPE(type) ((unsigned int *)MA_META_ADDR(0x002b0930u + (type) * 0x10))

/* 0x00158e10  load the tile models: per type up to 20 frames of 2 parts
   ("%s%02d_%02d.obe"), the frame count goes to the type table; and the glow textures */
void FUN_00158e10(void)
{
	char name[128];
	int *models;
	int type;
	unsigned int frame;
	unsigned int part;
	int i;

	for (i = 0; i < 6; i++)
		DAT_003075d4[i] = 0;
	DAT_003079b0 = 0;
	DAT_003082b8 = 0;
	DAT_003082c8 = (int)bLoadResource(NULL, "tokenglow.bmp", 0, 0xdefa);
	for (type = 0; type < 6; type++) {
		models = (int *)((char *)DAT_003075f0 + type * 0xa0);
		for (frame = 0; frame < 0x14; frame++) {
			for (part = 0; part < 2; part++) {
				sprintf(name, "%s%02d_%02d.obe", (const char *)MA_TILE_TYPE(type)[0], frame, part);
				models[frame * 2 + part] = (int)bLoadResource(NULL, name, 1, 0);
			}
			if (models[frame * 2] == 0) {
				MA_TILE_TYPE(type)[3] = frame;
				break;
			}
		}
	}
	DAT_003075d0 = (int)bLoadResource(NULL, (const char *)PTR_s_ai_orbglow_bmp_002b09d4, 0, 0);
	DAT_003075ec = (int)bLoadResource(NULL, "ai_orbshad.bmp", 0, 0);
}

/* 0x00158f20  CTilePosition_List::AddTileToQueue */
void FUN_00158f20(int board, int hidden, int colour, undefined2 *tile, undefined4 type)
{
	if (MA_I32(board, 0xc) == 200) {
		MA_PRINTF("CTilePosition_List::AddTileToQueue() : ERROR: queue length exceeded!\n");
		DbgBreakPoint();
	}
	*(undefined4 *)(tile + 6) = type;
	*(int *)(tile + 0x12) = hidden;
	tile[0] = 1;
	tile[3] = (short)colour;
	tile[4] = 1;
	MA_PTR(board, 0x10 + MA_I32(board, 0xc) * 4) = tile;
	MA_I32(board, 0xc) = MA_I32(board, 0xc) + 1;
	if (hidden == 0 && MA_U8(DAT_003082c0, 0x12d8 + colour) == 0) {
		MA_U8(DAT_003082c0, 0x12d8 + colour) = 1;
		MA_U8(DAT_003082c0, 0x12ec) = MA_U8(DAT_003082c0, 0x12ec) + 1;
	}
}

/* 0x00158fa0  the next player (after DAT_003082bc, cyclic) that is in the game
   (players' +0x20 at core 0x002baef0); -1 none */
void FUN_00158fa0(void)
{
	int player;

	player = (DAT_003082bc + 1) & 3;
	while (player != DAT_003082bc) {
		if (DAT_002baef0[player * 10] != 0)
			goto found;
		player = (player + 1) & 3;
	}
	DAT_003082bc = -1;
	if (DAT_002baef0[player * 10] == 0)
		return;
found:
	DAT_003082bc = player;
}

/* 0x00159000  walk a list (the original does nothing with the nodes) */
void FUN_00159000(int list)
{
	unsigned int node;

	for (node = MA_NEXT(list, NULL); node != 0; node = MA_NEXT(list, node))
		;
}

/* 0x00159020  the nearest free tile with the given type (-1 any), colour (-1 any),
   owner state (owned: -1 any, 0 no, 1 yes), "a tile like it is owned" state (-1 any),
   character (type 4: the colour must be the character of player `player`; -1 any);
   `flat` measures in x/z only. 0 when none */
int FUN_00159020(float *pos, int type, short colour, int owned, int twin_owned, int player, int flat)
{
	unsigned int tile;
	unsigned int other;
	int twin;
	int best;
	float best_distance;
	float distance;
	float dx;
	float dz;

	best = 0;
	best_distance = 1e11f; /* 0x51ba43b7 */
	for (tile = MA_NEXT(MA_TILES, NULL); tile != 0; tile = MA_NEXT(MA_TILES, tile)) {
		if (MA_I32(tile, 0x18) != 0)
			continue;
		if (owned != -1) {
			if (MA_I16(tile, 0x10) == -1) {
				if (owned != 0)
					continue;
			} else if (owned != 1) {
				continue;
			}
		}
		if (type != -1 && MA_I32(tile, 0x1c) != type)
			continue;
		if (colour != -1 && (int)MA_U16(tile, 0x12) != (int)colour)
			continue;
		if (twin_owned != -1) {
			twin = 0;
			for (other = MA_NEXT(MA_TILES, NULL); other != 0; other = MA_NEXT(MA_TILES, other)) {
				if (MA_I16(other, 0x10) != -1 && MA_I32(other, 0x1c) == MA_I32(tile, 0x1c) &&
					MA_I16(other, 0x12) == MA_I16(tile, 0x12)) {
					twin = 1;
					break;
				}
			}
			if (twin_owned != twin)
				continue;
		}
		if (flat == 0) {
			distance = MA_DISTANCE_SQ(pos, MA_FP(MA_PTR(tile, 0x34), 0xe0));
		} else {
			dz = pos[2] - MA_F32(MA_PTR(tile, 0x34), 0xe8);
			dx = pos[0] - MA_F32(MA_PTR(tile, 0x34), 0xe0);
			distance = (float)((double)dx * dx + (double)dz * dz);
		}
		if (MA_I32(tile, 0x1c) == 4 && player != -1 &&
			(int)MA_U16(tile, 0x12) != DAT_002baed4[player * 10])
			continue;
		if (distance < best_distance) {
			best_distance = distance;
			best = (int)tile;
		}
	}
	return best;
}

/* 0x001591b0  the first free tile with the type, colour, owner state and character */
int FUN_001591b0(int type, short colour, int owned, int player)
{
	unsigned int tile;

	for (tile = MA_NEXT(MA_TILES, NULL); tile != 0; tile = MA_NEXT(MA_TILES, tile)) {
		if (MA_I32(tile, 0x18) != 0)
			continue;
		if (MA_I32(tile, 0x1c) == 4 && player != -1 &&
			(int)MA_U16(tile, 0x12) != DAT_002baed4[player * 10])
			continue;
		if (owned != -1) {
			if (MA_I16(tile, 0x10) == -1) {
				if (owned != 0)
					continue;
			} else if (owned != 1) {
				continue;
			}
		}
		if (type == -1 || MA_I32(tile, 0x1c) == type) {
			if (colour == -1)
				return (int)tile;
			if ((int)MA_U16(tile, 0x12) == (int)colour)
				return (int)tile;
		}
	}
	return 0;
}

/* 0x00159270  the board's constructor */
int FUN_00159270(int board)
{
	int i;

	CLList_Init((int *)board);
	MA_I32(board, 0xc) = 0;
	MA_I32(board, 0x12d0) = 0;
	MA_I32(board, 0x12d8) = 0;
	MA_I32(board, 0x12dc) = 0;
	MA_I32(board, 0x12e0) = 0;
	MA_I32(board, 0x12e4) = 0;
	MA_I32(board, 0x12e8) = 0;
	MA_U8(board, 0x12ec) = 0;
	for (i = 0; i < 1000; i++)
		MA_I32(board, 0x330 + i * 4) = 0;
	MA_I32(board, 0x334) = 1;
	MA_F32(board, 0x12d4) = 60.0f;
	MA_I32(board, 0x330) = 1;
	return board;
}

/* 0x001592e0  find the platform under each movable tile of the board */
void FUN_001592e0(int board)
{
	unsigned int node;
	int *platform;

	for (node = MA_NEXT(board, NULL); node != 0; node = MA_NEXT(board, node)) {
		if (MA_I8(node, -0x20) >= 0)
			continue;
		platform = (int *)MA_ADDR(node, 8);
		if (*platform == 0)
			CLColl_GroundProbe(MA_FP(node, -0x10), 0.0f, NULL, (void **)platform, 0);
		else
			*platform = FUN_000b83f0(*platform);
		if (*platform == 0)
			MA_PRINTF("Moveable tile isn't on platform!\n");
	}
}

/* 0x00159350  remove entry `index` from the board's queue */
void FUN_00159350(int board, int index)
{
	MA_I32(board, 0xc) = MA_I32(board, 0xc) - 1;
	for (; index < MA_I32(board, 0xc); index++)
		MA_PTR(board, 0x10 + index * 4) = MA_PTR(board, 0x14 + index * 4);
}

/* 0x00159380  the tile under `pos` (within the orb radius in x/z and the height window
   around pos[1] - `drop`); 0 when none */
int FUN_00159380(int pos, float drop)
{
	int tile;
	float y;

	tile = FUN_00159020((float *)pos, -1, -1, -1, -1, -1, 1);
	if (tile != 0 &&
		CLVec3_DistanceSqXZ((float *)pos, MA_FP(MA_PTR(tile, 0x84), 0x28)) < DAT_002b09ec * DAT_002b09ec) {
		y = MA_F32(pos, 4) - drop;
		if (MA_F32(MA_PTR(tile, 0x84), 0x2c) - DAT_002b09e8 < y &&
			y < DAT_002b09e8 + MA_F32(MA_PTR(tile, 0x84), 0x2c))
			return tile;
	}
	return 0;
}

/* link each board tile to the meta_b object at its place (both unlinked, within 1.0 in
   x and z); the groups are 8 CLLists of 0x20 bytes at 0x00308f58 */
static int ma_near(unsigned int object, unsigned int tile)
{
	return fabs((double)(MA_F32(object, 8) - MA_F32(tile, 0x28))) < 1.0 &&  /* 0x00257c80 */
		fabs((double)(MA_F32(object, 0x10) - MA_F32(tile, 0x30))) < 1.0;
}

/* 0x00159410  link the board's tiles to the meta_b objects (tile order) */
void FUN_00159410(void)
{
	unsigned int node;
	unsigned int object;
	int group;

	for (node = MA_NEXT(DAT_003082c0, NULL); node != 0; node = MA_NEXT(DAT_003082c0, node)) {
		if (MA_I32(node, 0xc) != 0)
			continue;
		for (group = 0; group < DAT_00308f38; group++) {
			for (object = MA_NEXT(DAT_00308f58 + group * 0x20, NULL); object != 0;
				object = MA_NEXT(DAT_00308f58 + group * 0x20, object)) {
				if (MA_I32(object, 0x1c) == 0 && ma_near(object, node - 0x38)) {
					MA_U32(node, 0xc) = object;
					MA_U32(object, 0x1c) = node - 0x38;
				}
			}
		}
	}
}

/* 0x001594e0  the same, object order */
void FUN_001594e0(void)
{
	unsigned int node;
	unsigned int object;
	int group;

	for (group = 0; group < DAT_00308f38; group++) {
		for (object = MA_NEXT(DAT_00308f58 + group * 0x20, NULL); object != 0;
			object = MA_NEXT(DAT_00308f58 + group * 0x20, object)) {
			if (MA_I32(object, 0x1c) != 0)
				continue;
			for (node = MA_NEXT(DAT_003082c0, NULL); node != 0; node = MA_NEXT(DAT_003082c0, node)) {
				if (MA_I32(node, 0xc) == 0 && ma_near(object, node - 0x38)) {
					MA_U32(node, 0xc) = object;
					MA_U32(object, 0x1c) = node - 0x38;
				}
			}
		}
	}
}

/* bumpercars_a Debris_AddRing / Debris_AddSphere; their float parameters are not all
   floats in the function table yet */
static void ma_debris_ring(void *type, int count, float *pos, float out_speed, float up_speed,
	float jitter, int frames, unsigned int colour)
{
	MA_DUAL(FUN_00081880,
		void (*)(undefined4, int, undefined4, undefined4, undefined4, float, undefined4, undefined4),
		void (*)(void *, int, float *, float, float, float, int, unsigned int),
		((undefined4)type, count, (undefined4)pos, ma_fbits(out_speed), ma_fbits(up_speed), jitter,
			(undefined4)frames, colour),
		(type, count, pos, out_speed, up_speed, jitter, frames, colour));
}

static void ma_debris_sphere(void *type, int count, float step, float *pos, float speed, float spread, int frames)
{
	MA_DUAL(FUN_00081cb0,
		void (*)(undefined4, undefined4, float, undefined4, float, float, undefined4),
		void (*)(void *, int, float, float *, float, float, int),
		((undefined4)type, (undefined4)count, step, (undefined4)pos, speed, spread, (undefined4)frames),
		(type, count, step, pos, speed, spread, frames));
}

/* 0x001595a0  the collect effect at a tile */
void FUN_001595a0(undefined4 pos, int tile)
{
	if (MA_I32(tile, 0x1c) == 4 || MA_I32(tile, 0x1c) != 999) {
		/* PTR_s_STREAK1_002b2104: a debris type of the Meta data block */
		ma_debris_ring(&PTR_s_STREAK1_002b2104, 0x23, (float *)pos, 2.0f, 8.0f, 6.0f, 0, 0xffffffff);
		ma_debris_sphere(&PTR_s_STREAK1_002b2104, 0x23, 60.0f, (float *)pos, 16.0f, 10.0f, 0);
	}
}

/* 0x00159600  a timed tile group: count down and shrink its members */
void FUN_00159600(int group, int tile)
{
	unsigned int member;
	float t;

	if ((MA_U8(group, 0x20) & 1) == 0)
		return;
	t = MA_F32(group, 0x2c) - DAT_0028fda8;
	MA_F32(group, 0x2c) = t;
	if (t <= 0.0f)
		MA_I32(group, 0x24) = 1;
	/* the member list is the one at `tile`->+0x88 + 8 (ECX at 0x0015963c), not in `group` */
	for (member = MA_NEXT(MA_ADDR(MA_PTR(tile, 0x88), 8), NULL); member != 0;
		member = MA_NEXT(MA_ADDR(MA_PTR(tile, 0x88), 8), member))
		MA_F32(member, 0x24) = MA_F32(member, 0x24) - (MA_F32(member, 0x28) / MA_F32(group, 0x30)) * DAT_0028fda8;
	/* 0x002672b4 = 0.9 */
	MA_F32(MA_PTR(tile, 0x34), 0xf0) = MA_F32(MA_PTR(tile, 0x34), 0xf0) - (0.9f / MA_F32(group, 0x30)) * DAT_0028fda8;
	MA_F32(MA_PTR(tile, 0x34), 0xf8) = MA_F32(MA_PTR(tile, 0x34), 0xf8) - (0.9f / MA_F32(group, 0x30)) * DAT_0028fda8;
}

/* 0x001596b0  tile constructor (type, colour) */
undefined4 *FUN_001596b0(undefined4 *tile, undefined4 type, undefined2 colour)
{
	int i;

	tile[0x1c] = 0;
	tile[0x1d] = 0;
	tile[0x1b] = tile[0x1b] & 0xfffffffe;
	for (i = 0; i < 0x23; i++)
		tile[i] = 0;
	*(undefined2 *)((char *)tile + 0x12) = colour;
	tile[7] = type;
	*(undefined2 *)(tile + 4) = 0xffff;
	tile[6] = 1;
	CLList_Insert((int *)DAT_003082d4, (int *)tile, 0);
	tile[0x10] = 0;
	tile[0x11] = 0;
	tile[0x12] = 0;
	return tile;
}

/* one glow vertex (FVF 0x152: position, diffuse, one texture coordinate): 9 dwords */
static void ma_glow_vertex(int v, float x, float y, float z, unsigned int colour)
{
	*(float *)&DAT_003082e0[v * 9 + 0] = x;
	*(float *)&DAT_003082e0[v * 9 + 1] = y;
	*(float *)&DAT_003082e0[v * 9 + 2] = z;
	DAT_003082e0[v * 9 + 6] = colour;
}

/* 0x00159710  draw the glow under a tile */
void FUN_00159710(byte *tile)
{
	int entity;
	int type;
	float lift;
	float *pos;
	const unsigned char *rgb;
	unsigned int colour;

	entity = MA_I32(tile, 0x34);
	type = MA_I32(tile, 0x1c);
	pos = MA_FP(entity, 0xe0);
	lift = 60.0f; /* 0x00254e98 */
	if (type != 4 && type != 2)
		lift = 80.0f; /* 0x0026556c */
	rgb = tile;
	if (type == 4) {
		rgb = (const unsigned char *)&DAT_00290718 + MA_U16(tile, 0x12) * 0x60;
	} else if (type != 0 && type != 2 && type != 3) {
		return;
	}
	if (type == 0)
		rgb = (const unsigned char *)MA_META_ADDR(0x002b0910u) + MA_U16(tile, 0x12) * 4;
	else if (type == 2)
		rgb = (const unsigned char *)MA_META_ADDR(0x002b0928u);
	else if (type == 3)
		rgb = (const unsigned char *)MA_META_ADDR(0x002b092cu);
	/* the colour tables (0x002bc490...: one per channel); alpha 0x37 (0x002bc974) */
	colour = DAT_002bd640[rgb[2]] | DAT_002bce18[rgb[0]] | DAT_002bc490[rgb[1]] | DAT_002bc898[0x37];
	/* 0x0026556c = 80, 0x002538a0 = 1.5 */
	ma_glow_vertex(0, pos[0] - 80.0f, (MA_F32(entity, 0xe4) + 1.5f) - lift, MA_F32(entity, 0xe8) - 80.0f, colour);
	ma_glow_vertex(1, pos[0] - 80.0f, (MA_F32(entity, 0xe4) + 1.5f) - lift, MA_F32(entity, 0xe8) + 80.0f, colour);
	ma_glow_vertex(2, pos[0] + 80.0f, (MA_F32(entity, 0xe4) + 1.5f) - lift, MA_F32(entity, 0xe8) - 80.0f, colour);
	ma_glow_vertex(3, pos[0] + 80.0f, (MA_F32(entity, 0xe4) + 1.5f) - lift, MA_F32(entity, 0xe8) + 80.0f, colour);
	DAT_003082e0[0 * 9 + 7] = 0;
	DAT_003082e0[0 * 9 + 8] = 0;
	DAT_003082e0[1 * 9 + 7] = 0;
	*(float *)&DAT_003082e0[1 * 9 + 8] = 1.0f;
	*(float *)&DAT_003082e0[2 * 9 + 7] = 1.0f;
	DAT_003082e0[2 * 9 + 8] = 0;
	*(float *)&DAT_003082e0[3 * 9 + 7] = 1.0f;
	*(float *)&DAT_003082e0[3 * 9 + 8] = 1.0f;
	bdDrawPrimitiveFVF(4, DAT_003082e0, 4, 0x152);
}
