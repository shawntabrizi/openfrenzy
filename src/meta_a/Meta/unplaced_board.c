/*
UNPLACED_BOARD.C  <-  gap functions 0x0015a630-0x0015b4d0 of C:\BlitzParty\Game\Src\Meta\
(no source path in the XBE; after Tiles_Meta.cpp, before Effects.cpp)

The board at run time: remove a tile, fill the board with tiles (at load and by the
puzzles), collect a tile, the board's queue of delayed tiles, the tiles' update.
Layouts: the tile (0x8c bytes) in unplaced_tiles.c, the tile position (0x48 bytes) and the
platform in Tiles_Meta.c. The board (DAT_003082c0) is a CLList of tile positions (node at
+0x38) plus: +0x08 their number, +0x0c queue length, +0x10 queue, +0x12d0 queue timer,
+0x12d4 float delay of new tiles.
*/

#include "meta_a.h"

#define MA_TILES ((unsigned int *)DAT_003082d4)
#define MA_BOARD ((unsigned int *)DAT_003082c0)
#define MA_NEXT(list, node) CLList_Next((unsigned int *)(list), (unsigned int *)(node))

/* the tile type table (0x002b0930, 6 x 0x10 bytes: name, two words, frame count) */
#define MA_TILE_FRAMES(type) (*(unsigned int *)MA_META_ADDR(0x002b093cu + (type) * 0x10))
/* the colour tile frame count (type 0) and the flag tile frame count (type 2) */
#define MA_COLOURS MA_TILE_FRAMES(0)

/* a colour (type 0 frame) that is not on the board yet; the caller checks that one exists */
static unsigned int ma_free_colour(void)
{
	unsigned int colour;

	do {
		colour = (unsigned int)ma_rand() % MA_COLOURS;
	} while (MA_U8(DAT_003082c0, 0x12d8 + colour) == 1);
	return colour;
}

/* 0x0015a630  remove a tile (fastcall): a super tile not yet collected first plays its
   spark effect and stays; else it leaves the tile list, the counts, the players and the
   tile position, and its platform is deleted. Returns the next tile of the list (the
   tile itself when it stays) */
int FUN_0015a630(int tile)
{
	int next;
	int i;
	unsigned char *used;
	int rays;

	if (MA_I32(tile, 0x1c) == 3 && MA_I32(tile, 0x70) == 0 && MA_I32(tile, 0x74) == 0) {
		FUN_0015a110((undefined4 *)(tile + 0x4c), (undefined4 *)(MA_I32(tile, 0x34) + 0xe0), 90.0f);
		return tile;
	}
	next = CLList_Remove((int *)DAT_003082d4, (int *)tile);
	if (MA_I32(tile, 0x1c) == 4)
		DAT_003075d4[MA_U16(tile, 0x12)] = DAT_003075d4[MA_U16(tile, 0x12)] - 1;
	if (MA_I32(tile, 0x1c) == 2)
		DAT_003079b0 = DAT_003079b0 - 1;
	if (MA_I32(tile, 0x1c) == 3)
		DAT_003082b8 = DAT_003082b8 - 1;
	for (i = 0; i < DAT_002bae94; i++) {
		if (MA_I32(DAT_003075bc[i], 0x1134) == tile)
			MA_I32(DAT_003075bc[i], 0x1134) = 0;
		if (MA_I32(DAT_003075bc[i], 8) == tile)
			MA_I32(DAT_003075bc[i], 8) = 0;
	}
	if (MA_I32(tile, 0x1c) == 0) {
		used = (unsigned char *)DAT_003082c0 + 0x12d8 + MA_U16(tile, 0x12);
		if (*used != 0) {
			*used = 0;
			MA_U8(DAT_003082c0, 0x12ec) = MA_U8(DAT_003082c0, 0x12ec) - 1;
		}
	}
	if (MA_I32(tile, 0x44) != 0)
		MA_I32(MA_PTR(tile, 0x44), 0x3c) = 0;
	if (MA_PTR(tile, 0x84) != NULL)
		*(short *)MA_PTR(tile, 0x84) = 0;
	rays = MA_I32(tile, 0x88);
	if (rays != 0) {
		CRayControl_Destroy((struct CRayControl *)rays);
		MA_DELETE(rays);
		MA_I32(tile, 0x88) = 0;
	}
	if (MA_I32(tile, 0x38) != 0)
		Actor_Delete(MA_PTR(tile, 0x38));
	if (MA_I32(tile, 0x3c) != 0)
		Actor_Delete(MA_PTR(tile, 0x3c));
	Platform_Delete(MA_PTR(tile, 0x34));
	return next;
}

/* 0x0015a770  fill the tile positions marked "placed at load" (mask bit 6): type 0 gets
   colours in pairs (two tiles of each new colour), other types a random frame; `queued`
   puts them in the board's queue (10 s delay) instead of placing them now */
void FUN_0015a770(int queued)
{
	int second;
	unsigned int node;
	unsigned int colour;
	unsigned int previous;
	int tp;

	if (MA_I32(DAT_003082c0, 8) == 0)
		return;
	second = 0;
	node = MA_NEXT(MA_BOARD, NULL);
	previous = node; /* (never used before it is set) */
	for (; node != 0; node = MA_NEXT(MA_BOARD, node)) {
		tp = (int)node - 0x38;
		if ((MA_U8(tp, 0x18) & 0x40) == 0)
			continue;
		if (MA_I32(tp, 0x24) == 0) {
			if (second) {
				second = 0;
				colour = previous;
			} else {
				if (MA_COLOURS <= MA_U8(DAT_003082c0, 0x12ec))
					continue;
				previous = ma_free_colour();
				second = 1;
				colour = previous;
			}
		} else {
			colour = (unsigned int)ma_rand() % MA_TILE_FRAMES(MA_I32(tp, 0x24));
		}
		if (queued == 0)
			FUN_0015a180(MA_I32(tp, 0x24), colour, (undefined2 *)tp, 0);
		else
			FUN_00158f20(DAT_003082c0, MA_I32(tp, 0x24), (int)colour, (undefined2 *)tp, ma_fbits(10.0f));
	}
}

/* is the tile position free for a new tile of `type`: not in use, its group object (if
   any) not active, and either in group `group` or (group -1) its group switched on and the
   type allowed */
static int ma_free_position(unsigned int node, int type, int group)
{
	int tp = (int)node - 0x38;

	if (MA_I16(tp, 0) != 0)
		return 0;
	if (MA_I32(tp, 0x44) != 0 && MA_I16(MA_PTR(tp, 0x44), 0x18) != 0)
		return 0;
	if (group == -1)
		return MA_I32(DAT_003082c0, 0x330 + MA_I16(tp, 2) * 4) != 0 && MA_U8(tp, 0x1c + type) != 0;
	return MA_I16(tp, 2) == group;
}

/* 0x0015a8b0  add `count` tiles of `type` (colour -1: random; type 0 in pairs, type 4 the
   character of the next player that needs one) on random free tile positions (or all on
   `tp`), in group `group` (-1: any switched-on group); `delay` > 0 queues them, < 0 places
   them hidden. With no tile positions the tiles go on random grid cells (0x00159c40).
   Returns the number placed; the early exits return what the original leaves in EAX */
uint FUN_0015a8b0(uint count, int type, uint colour, float delay, int group, short *tp)
{
	unsigned int c;
	unsigned int node;
	unsigned int pick;
	unsigned int index;
	int tries;
	int placed;
	int pair;
	int i;
	int r;

	pair = 0;
	if (MA_I32(DAT_003082c0, 8) == 0) {
		FUN_00159c40(type, (int)count);
		return 0; /* (EAX after the call; nobody reads it) */
	}
	c = colour;
	if (colour == 0xffffffff) {
		if (type == 0) {
			if (MA_COLOURS <= MA_U8(DAT_003082c0, 0x12ec))
				return 0;
			c = ma_free_colour();
		} else if (type == 4) {
			for (tries = 0; tries < 4; tries++) {
				if (DAT_003082bc == -1)
					break;
				if (DAT_003075d4[DAT_002baed4[DAT_003082bc * 10]] < 2)
					break;
				FUN_00158fa0();
			}
			if (DAT_003082bc == -1) {
				if (tries == 4)
					return (uint)DAT_003082bc;
				if (1 < DAT_003079b0)
					return (uint)DAT_003082bc;
				r = (int)Core_Rand();
				if (DAT_002b08c0 <= r % 100)
					return (uint)(r / 100);
				/* no character needs a tile: a flag tile instead */
				type = 2;
				c = Core_Rand() % MA_TILE_FRAMES(2);
			} else {
				c = (unsigned int)DAT_002baed4[DAT_003082bc * 10];
				if (1 < DAT_003075d4[c])
					return (uint)(DAT_003082bc * 0x28);
				DAT_002baef0[DAT_003082bc * 10] = DAT_002baef0[DAT_003082bc * 10] - 1;
				FUN_00158fa0();
			}
		} else {
			c = (unsigned int)ma_rand() % MA_TILE_FRAMES(type);
		}
	}
	CLSamples_PlayGeneric(1, 0x7f, 0, 0, NULL);
	if (tp == NULL) {
		placed = 0;
		for (node = MA_NEXT(MA_BOARD, NULL); node != 0; node = MA_NEXT(MA_BOARD, node))
			if (ma_free_position(node, type, group))
				placed++;
		if ((int)count >= placed)
			count = (uint)placed;
		if (type == 0 && (count & 1) != 0)
			count = count - 1;
	}
	for (i = 0; i < (int)count; i++) {
		pick = (unsigned int)(int)tp;
		if (tp == NULL) {
			/* a random position; draw again until it is a free one */
			for (;;) {
				index = (unsigned int)ma_rand() % MA_U32(DAT_003082c0, 8);
				node = MA_NEXT(MA_BOARD, NULL);
				if (node == 0)
					continue;
				for (r = 0; node != 0; r++, node = MA_NEXT(MA_BOARD, node))
					if ((unsigned int)r == index && ma_free_position(node, type, group))
						break;
				if (node != 0)
					break;
			}
			pick = node - 0x38;
		}
		if (!(delay > 0.0f)) /* (NaN too) */
			FUN_0015a180(type, c, (undefined2 *)pick, delay < 0.0f);
		else
			FUN_00158f20(DAT_003082c0, type, (int)c, (undefined2 *)pick, ma_fbits(delay));
		if (colour == 0xffffffff && type == 0) {
			if (pair == 0) {
				pair = 1;
			} else {
				if (MA_COLOURS <= MA_U8(DAT_003082c0, 0x12ec))
					return (uint)pair;
				c = ma_free_colour();
				pair = 0;
			}
		}
	}
	return count;
}

/* add `points` to the score of the player with character `ch` (core player table) */
#define MA_SCORE_OF(ch) DAT_002baee8[(ch) * 10]

/* 0x0015ac80  CMetaPlayer (thiscall): touch tile `tile`. A colour tile is picked up (or
   put down again when it is the player's own); when another player holds a tile of the
   same colour, the pair is made (and stolen from that player): both are collected. A flag
   or character tile is collected, a super tile gives DAT_002b08c4 orbs */
void FUN_0015ac80(int *ai, int tile)
{
	unsigned int i;
	int other;
	int kind;
	int held;

	if (MA_I16(tile, 0x14) != 0)
		return;
	switch (MA_I32(tile, 0x1c)) {
	case 0:
		if ((int)MA_I16(tile, 0x10) == (int)MA_U16(ai[0xc], 0x230)) {
			MA_I16(tile, 0x10) = -1;
			ai[0x44d] = 0;
			return;
		}
		if (MA_I16(tile, 0x10) != -1)
			return;
		if (ai[0x44d] != 0)
			MA_I16(ai[0x44d], 0x10) = -1;
		for (i = 0; (int)i < DAT_002bae94; i++) {
			held = MA_I32(DAT_003075bc[i], 0x1134);
			if (held == 0 || MA_I32(held, 0x1c) != 0 || MA_I16(held, 0x12) != MA_I16(tile, 0x12))
				continue;
			CLSamples_PlayGeneric(6, 0x7f, 0, 0, NULL);
			other = DAT_003075bc[i];
			if ((int)ai != other) {
				/* half the time the thief cheers, else the victim complains */
				if ((int)Core_Rand() % 100 < 0x32)
					FUN_000b0320((int)ai, 6, DAT_00293928, 180.0f, 0, 0xffffffff, 0);
				else
					FUN_000b0320(other, 7, DAT_00293928, 180.0f, 0, 0xffffffff, 0);
				MA_PRINTF("STEAL!!!\n");
				MA_SCORE_OF(MA_U16(ai[0xc], 0x230)) = MA_SCORE_OF(MA_U16(ai[0xc], 0x230)) + MA_I16(tile, 0xa);
				MA_SCORE_OF(MA_U16(MA_PTR(DAT_003075bc[i], 0x30), 0x230)) =
					MA_SCORE_OF(MA_U16(MA_PTR(DAT_003075bc[i], 0x30), 0x230)) + MA_I16(tile, 0xc);
				if (ai[0x44d] != 0)
					FUN_0015ac80(ai, ai[0x44d]);
			}
			FUN_001595a0((undefined4)(MA_I32(tile, 0x34) + 0xe0), tile);
#define MA_HELD MA_PTR(DAT_003075bc[i], 0x1134)
			CLightning_Strike(MA_FP(MA_PTR(MA_HELD, 0x84), 0x28), MA_FP(MA_PTR(tile, 0x84), 0x28));
			Debris_Add((struct DebrisType *)MA_META_ADDR(0x002b1e44u), /* SMOKE */
				MA_FP(MA_PTR(MA_HELD, 0x84), 0x28), (float *)DAT_002ea478, 0, 0xffffffff);
			Debris_Add((struct DebrisType *)MA_META_ADDR(0x002b1e44u),
				MA_FP(MA_PTR(MA_HELD, 0x84), 0x28), (float *)DAT_002ea478, 0, 0xffffffff);
			MA_F32(MA_PTR(MA_HELD, 0x84), 0xc) = 20.0f;
			MA_I32(MA_HELD, 0x18) = 4;
			if (i != MA_U16(ai[0xc], 0x230)) {
				MA_U16(MA_HELD, 0x10) = MA_U16(ai[0xc], 0x230);
				ai[0x44d] = (int)MA_HELD;
				MA_HELD = NULL;
			}
#undef MA_HELD
			FUN_0015a630(tile);
			FUN_0015a8b0(2, 0, 0xffffffff, MA_F32(DAT_003082c0, 0x12d4), -1, NULL);
			FUN_00158050((int)ai, 1);
			MA_VFN(ai, 0x18, void (*)(int *, int))(ai, -1);
			return;
		}
		if (ai[0x44d] != 0)
			MA_I16(ai[0x44d], 0x10) = -1;
		MA_U16(tile, 0x10) = MA_U16(ai[0xc], 0x230);
		ai[0x44d] = tile;
		return;
	case 2:
	case 4:
	case 3:
		kind = MA_I32(tile, 0x1c);
		FUN_001595a0((undefined4)(MA_I32(tile, 0x34) + 0xe0), tile);
		FUN_00158050((int)ai, kind == 3 ? DAT_002b08c4 : 1);
		/* the fourth orb drops the colour tile held */
		if (ai[0x44d] != 0 && ai[0x450] == 4) {
			MA_I16(ai[0x44d], 0x10) = -1;
			ai[0x44d] = 0;
		}
		MA_SCORE_OF(MA_U16(ai[0xc], 0x230)) = MA_SCORE_OF(MA_U16(ai[0xc], 0x230)) + MA_I16(tile, 0xe);
		CLSamples_PlayGeneric(3, 0x7f, 0, 0, NULL);
		if (kind == 3)
			MA_I32(tile, 0x74) = 1;
		FUN_0015a630(tile);
		return;
	}
}

/* 0x0015b050  CTilePosition_List (thiscall): free all tile positions of the list */
void FUN_0015b050(int list)
{
	unsigned int node;
	unsigned int next;
	int tp;

	node = MA_NEXT(list, NULL);
	while (node != 0) {
		tp = (int)node - 0x38;
		next = (unsigned int)CLList_Remove((int *)list, (int *)(tp + 0x38));
		if (tp != 0)
			MA_DELETE(tp);
		node = MA_NEXT(list, next);
	}
}

/* 0x0015b090  the board's update (fastcall): place the queued tiles whose delay ran out,
   and move the tile positions with the objects they stand on (+0x40) */
void FUN_0015b090(int board)
{
	int tp;
	int i;
	unsigned int node;
	int carrier;

	if (MA_I32(board, 0xc) == 0) {
		MA_F32(board, 0x12d0) = 0.0f;
	} else {
		tp = MA_I32(board, 0x10);
		MA_F32(board, 0x12d0) = DAT_0028fda8 + MA_F32(board, 0x12d0);
		do {
			if (!(MA_F32(board, 0x12d0) >= MA_F32(tp, 0xc)))
				break;
			MA_F32(board, 0x12d0) = MA_F32(board, 0x12d0) - MA_F32(tp, 0xc);
			if (FUN_0015a180(MA_I32(tp, 0x24), MA_U16(tp, 6), (undefined2 *)tp, 0) != 0)
				MA_I16(tp, 8) = 0;
			MA_I32(board, 0xc) = MA_I32(board, 0xc) - 1;
			for (i = 0; i < MA_I32(board, 0xc); i++)
				MA_I32(board, 0x10 + i * 4) = MA_I32(board, 0x14 + i * 4);
			tp = MA_I32(board, 0x10);
		} while (MA_I32(board, 0xc) != 0);
	}
	for (node = MA_NEXT(board, NULL); node != 0; node = MA_NEXT(board, node)) {
		carrier = MA_I32(node, 8);
		if (carrier == 0)
			continue;
		MA_F32(node, -0x10) = MA_F32(node, -0x10) + MA_F32(carrier, 0xe0);
		MA_F32(node, -0xc) = MA_F32(node, -0xc) + MA_F32(carrier, 0xe4);
		MA_F32(node, -8) = MA_F32(node, -8) + MA_F32(carrier, 0xe8);
		carrier = MA_I32(node, 8);
		MA_F32(node, -0x10) = MA_F32(node, -0x10) - MA_F32(carrier, 0x1b4);
		MA_F32(node, -0xc) = MA_F32(node, -0xc) - MA_F32(carrier, 0x1b8);
		MA_F32(node, -8) = MA_F32(node, -8) - MA_F32(carrier, 0x1bc);
	}
}

/* 0x0015b1a0  a tile's update (fastcall); returns the next tile of the list. States
   (+0x18): 0 on its position (follows a moving carrier; a carrier flagged 0x400 throws
   it: state 3), 1 fading in, 2 follows the carrier's height, 3 thrown (30 frames),
   4 removed */
int FUN_0015b1a0(int tile)
{
	float gravity;
	float dx;
	float dy;
	float dz;
	float drop;
	int platform;
	int carrier;

	gravity = DAT_00292c84 * 0.5f; /* 0x002536d0 */
	if (MA_I32(tile, 0x1c) == 3 && (MA_U8(tile, 0x6c) & 1) != 0 && MA_I32(tile, 0x18) != 4) {
		FUN_00159600(tile + 0x4c, tile);
		if (MA_I32(tile, 0x70) != 0)
			return FUN_0015a630(tile);
	}
	switch (MA_I32(tile, 0x18)) {
	case 1:
		if (MA_I32(tile, 0x84) == 0)
			return tile;
		MA_I32(MA_PTR(tile, 0x34), 0x234) = MA_I32(MA_PTR(tile, 0x34), 0x234) +
			(int)(DAT_0028fda8 * 4.0f); /* 0x00257c40 */
		if (0xff < MA_I32(MA_PTR(tile, 0x34), 0x234)) {
			MA_I32(MA_PTR(tile, 0x34), 0x234) = 0x100;
			MA_I32(tile, 0x18) = 0;
			MA_I16(tile, 0x10) = -1;
		}
		/* fall through */
	case 0:
		carrier = MA_I32(MA_PTR(tile, 0x84), 0x40);
		if (carrier != 0) {
			platform = MA_I32(tile, 0x34);
			MA_F32(platform, 0xe0) = MA_F32(carrier, 0xe0) + MA_F32(platform, 0xe0);
			MA_F32(platform, 0xe4) = MA_F32(platform, 0xe4) + MA_F32(carrier, 0xe4);
			MA_F32(platform, 0xe8) = MA_F32(platform, 0xe8) + MA_F32(carrier, 0xe8);
			carrier = MA_I32(MA_PTR(tile, 0x84), 0x40);
			MA_F32(platform, 0xe0) = MA_F32(platform, 0xe0) - MA_F32(carrier, 0x1b4);
			MA_F32(platform, 0xe4) = MA_F32(platform, 0xe4) - MA_F32(carrier, 0x1b8);
			MA_F32(platform, 0xe8) = MA_F32(platform, 0xe8) - MA_F32(carrier, 0x1bc);
			if ((MA_U32(MA_PTR(MA_PTR(tile, 0x84), 0x40), 0x450) & 0x400) != 0) {
				MA_I32(tile, 0x18) = 3;
				MA_I16(tile, 0x16) = 0x1e;
				MA_F32(MA_PTR(tile, 0x34), 0x1d4) = 0.1f;
				MA_F32(MA_PTR(tile, 0x34), 0x1dc) = 0.05f;
				MA_F32(MA_PTR(tile, 0x34), 0x1d8) = 0.03f;
			}
		}
		break;
	case 2:
		carrier = MA_I32(MA_PTR(tile, 0x84), 0x40);
		if (carrier != 0) {
			MA_F32(MA_PTR(tile, 0x34), 0xe4) = MA_F32(carrier, 0xe4);
			return tile;
		}
		break;
	case 3:
		MA_I16(tile, 0x16) = MA_I16(tile, 0x16) - 1;
		platform = MA_I32(tile, 0x34);
		dx = DAT_0028fda8 * MA_F32(platform, 0x1a4);
		dy = DAT_0028fda8 * MA_F32(platform, 0x1a8);
		dz = DAT_0028fda8 * MA_F32(platform, 0x1ac);
		drop = DAT_0028fda8 * DAT_0028fda8 * gravity * 0.5f; /* 0x002536d0 */
		MA_F32(platform, 0x1a8) = MA_F32(platform, 0x1a8) - DAT_0028fda8 * gravity;
		MA_F32(MA_PTR(tile, 0x34), 0xe0) = dx + MA_F32(MA_PTR(tile, 0x34), 0xe0);
		MA_F32(MA_PTR(tile, 0x34), 0xe4) = (dy - drop) + MA_F32(MA_PTR(tile, 0x34), 0xe4);
		MA_F32(MA_PTR(tile, 0x34), 0xe8) = dz + MA_F32(MA_PTR(tile, 0x34), 0xe8);
		if (MA_I16(tile, 0x16) == 0)
			return FUN_0015a630(tile);
		break;
	case 4:
		return FUN_0015a630(tile);
	}
	return tile;
}

/* 0x0015b400  remove all tiles (and clear the grid cells in use) */
void FUN_0015b400(void)
{
	unsigned int tile;
	int i;

	for (i = 0; i < 0x240; i++)
		DAT_003079b8[i] = 0;
	DAT_003082cc = 0;
	tile = MA_NEXT(MA_TILES, NULL);
	while (tile != 0) {
		MA_I32(tile, 0x70) = 1;
		tile = MA_NEXT(MA_TILES, FUN_0015a630((int)tile));
	}
}

/* 0x0015b450  update all tiles */
void FUN_0015b450(void)
{
	unsigned int tile;

	tile = MA_NEXT(MA_TILES, NULL);
	while (tile != 0)
		tile = MA_NEXT(MA_TILES, FUN_0015b1a0((int)tile));
}

/* 0x0015b480  the board's destructor (fastcall): free the tile positions, then the
   CLList base destructor (the empty function 0x001d5950) */
void FUN_0015b480(int board)
{
	MA_I32(board, 0xc) = 0;
	FUN_0015b050(board);
	((void (*)(int))FUN_001d5950)(board);
}

/* 0x0015b4d0  delete the board */
void FUN_0015b4d0(void)
{
	int board;

	board = DAT_003082c0;
	if (DAT_003082c0 != 0) {
		FUN_0015b480(board);
		MA_DELETE(board);
	}
	DAT_003082c0 = 0;
}
