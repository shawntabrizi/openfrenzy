/*
TILES_META.C  <-  C:\BlitzParty\Game\Src\Meta\Tiles_Meta.cpp

The tile positions of the board and the platforms on them.

A tile position (0x48 bytes, made by 0x0015a040; CLList node at +0x38 in the board's list,
the board is DAT_003082c0):
  +0x00 short in use (a platform stands on it)   +0x02 short group (switch index)
  +0x06 short colour (-1)   +0x08 short           +0x0a/+0x0c/+0x0e short score values
  +0x10 int frame offset (model table)            +0x18 uint type mask (bit 6: placed at load)
  +0x1c byte[6] allowed types                      +0x24 int type
  +0x28 float[4] position                          +0x40 int, +0x44 the tile group object
A platform (0x470 bytes, a charlib actor): +0x10 its actor, +0xe0 position, +0xf0 scale,
  +0x230 flags, +0x234 alpha, +0x334 anim name, +0x338 the smoke list, +0x454 the tile
  (0x8c bytes, see unplaced_tiles.c).
*/

#include "meta_a.h"

static const char ma_file_tiles[] = "C:\\BlitzParty\\Game\\Src\\Meta\\Tiles_Meta.cpp";

/* the tile type table (0x002b0930, 6 x 0x10 bytes: name, two words, frame count) */
#define MA_TILE_FRAMES(type) (*(unsigned int *)MA_META_ADDR(0x002b093cu + (type) * 0x10))
/* the route grid of charlib (Route.cpp): columns, rows, origin, size, cell heights */
#define MA_ROUTE_HEIGHT(cell) (*(float *)((char *)DAT_002f27d8 + 8 + (cell) * 0x18))

/* 0x00159970  Meta/Tiles_Meta.cpp: create the platform of a tile: type, colour (type 4:
   a character), frame offset; 0 when the type has no model or too many super tiles */
int FUN_00159970(int type, uint colour, int frame)
{
	int platform;
	void *memory;
	undefined4 *tile;
	unsigned int index;
	float speed;

	if (type == 4) {
		DAT_003075d4[colour & 0xffff] = DAT_003075d4[colour & 0xffff] + 1;
	} else if (type == 2) {
		DAT_003079b0 = DAT_003079b0 + 1;
	} else if (type == 3) {
		if (1 < DAT_003082b8)
			return 0;
		DAT_003082b8 = DAT_003082b8 + 1;
	}
	index = colour & 0xffff;
	frame = frame + (index + type * 0x14) * 2;
	if (DAT_003075f0[frame] == 0) {
		MA_PRINTF("*** ERROR *** no such tile type: %d %d\n", type, index);
		return 0;
	}
	platform = (int)bkHeapCalloc(0x470, 0, ma_file_tiles, 0x2c2, 0x2001);
	if (platform == 0) {
		MA_PRINTF("*** ERROR *** could not allocate memory for platform\n");
		return 0;
	}
	memory = MA_NEW(0x8c, ma_file_tiles, 0x2cc, 0x1001);
	if (memory == NULL)
		tile = NULL;
	else
		tile = FUN_001596b0((undefined4 *)memory, type, (undefined2)colour);
	MA_PTR(platform, 0x454) = tile;
	if (type == 0 && MA_U8(DAT_003082c0, 0x12d8 + index) == 0) {
		MA_U8(DAT_003082c0, 0x12d8 + index) = 1;
		MA_U8(DAT_003082c0, 0x12ec) = MA_U8(DAT_003082c0, 0x12ec) + 1;
	}
	if (MA_I32(platform, 0x454) == 0) {
		MA_PRINTF("*** ERROR *** could not allocate memory for tile\n");
		return 0;
	}
	Actor_Create((void *)DAT_003075f0[frame], NULL, (void *)(platform + 0x10), 2, 1, 1);
	MA_U32(platform, 0x230) = MA_U32(platform, 0x230) | 0x100000;
	if (type == 4 || type == 0 || type == 2 || type == 3) {
		MA_PTR(platform, 0x334) = MA_META_ADDR(0x002b0990u); /* "default" */
		MA_PTR(platform, 0x274) = baFindSubObject((struct BActorInstance *)(platform + 0x20),
			(const char *)MA_META_ADDR(0x002b0990u));
	}
	if (type == 4 || type == 2 || type == 3) {
		MA_F32(platform, 0xf8) = 0.7f;
		MA_F32(platform, 0xf4) = 0.7f;
		MA_F32(platform, 0xf0) = 0.7f;
	}
	MA_U32(platform, 0x230) = MA_U32(platform, 0x230) | 0x200;
	MA_I32(platform, 0x234) = 0;
	/* the entity list at DAT_002ea488 + 0x18 (ECX at 0x00159b88), where the puzzles and
	   tiles look for platforms */
	CLList_Insert((int *)((char *)DAT_002ea488 + 0x18), (int *)platform, 0);
	MA_I32(MA_PTR(platform, 0x454), 0x34) = platform;
	MA_U32(platform, 0xf4) = *(unsigned int *)MA_META_ADDR(0x002b09dcu); /* tile y scale */
	if (type == 2 || type == 3 || type == 4)
		speed = 0.4f;
	else
		speed = 0.2f;
	Actor_PlayAnim((void *)(platform + 0x10), 0, 1, 0, speed, 0.0f, 1);
	if (MA_PTR(MA_PTR(platform, 0x454), 0x38) != NULL)
		Actor_PlayAnim(MA_PTR(MA_PTR(platform, 0x454), 0x38), 0, 1, 0, 0.2f, 0.0f, 1);
	if (MA_PTR(MA_PTR(platform, 0x454), 0x3c) != NULL)
		Actor_PlayAnim(MA_PTR(MA_PTR(platform, 0x454), 0x3c), 0, 1, 0, 0.2f, 0.0f, 1);
	return platform;
}

/* 0x00159c40  Meta/Tiles_Meta.cpp: place `count` tiles of `type` (random colour) on random
   free cells of the route grid (even cells only; the cell height must be below 1e6) */
void FUN_00159c40(int type, int count)
{
	int platform;
	int half_x;
	int half_z;
	int x;
	int z;
	int cell;

	for (; count != 0; count--) {
		platform = FUN_00159970(type, (unsigned int)ma_rand() % MA_TILE_FRAMES(type), 0);
		if (platform == 0)
			continue;
		half_x = DAT_00292c4c / 2;
		half_z = DAT_00292c50 / 2;
		do {
			x = (ma_rand() % half_x) * 2;
			z = (ma_rand() % half_z) * 2;
			cell = DAT_00292c4c * z + x;
		} while (MA_ROUTE_HEIGHT(cell) >= 1000000.0f || DAT_003079b8[cell] != 0); /* 0x00265b60 */
		DAT_003079b8[cell] = 1;
		MA_F32(platform, 0xe0) = (DAT_00292c68 / (float)DAT_00292c4c) * (float)x +
			DAT_00292c68 / (float)(DAT_00292c4c * 2) + DAT_00292c60;
		MA_F32(platform, 0xe8) = (DAT_00292c6c / (float)DAT_00292c50) * (float)z +
			DAT_00292c6c / (float)(DAT_00292c50 * 2) + DAT_00292c64;
		MA_F32(platform, 0xec) = 1.0f;
		MA_F32(platform, 0xe4) = MA_ROUTE_HEIGHT(cell) - 20.0f; /* 0x00255530 */
		baPrepActorInstance((struct BActorInstance *)(platform + 0x20), 1);
	}
}

/* 0x00159e10  Meta/Tiles_Meta.cpp: draw the tiles: depth pass, faded pass, opaque pass,
   then the glows under them */
void FUN_00159e10(void)
{
	unsigned int tile;

	bdSetWorldMatrix(DAT_002855f0);
	bdPushRenderState(3, MA_U32(DAT_003055d0, 0x48), 0);
	bdPushRenderState(4, 1, 0);
	for (tile = CLList_Next((unsigned int *)DAT_003082d4, NULL); tile != 0;
		tile = CLList_Next((unsigned int *)DAT_003082d4, (unsigned int *)tile))
		Actor_DrawFirstPass((void *)(MA_I32(tile, 0x34) + 0x10));
	for (tile = CLList_Next((unsigned int *)DAT_003082d4, NULL); tile != 0;
		tile = CLList_Next((unsigned int *)DAT_003082d4, (unsigned int *)tile))
		Actor_DrawFaded((void *)(MA_I32(tile, 0x34) + 0x10));
	bdSetRenderState(3, 0, 0);
	for (tile = CLList_Next((unsigned int *)DAT_003082d4, NULL); tile != 0;
		tile = CLList_Next((unsigned int *)DAT_003082d4, (unsigned int *)tile))
		Actor_DrawOpaque((void *)(MA_I32(tile, 0x34) + 0x10));
	bdSetWorldMatrix(DAT_002855f0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetTexture(0, (struct bdTexture *)DAT_003075ec);
	for (tile = CLList_Next((unsigned int *)DAT_003082d4, NULL); tile != 0;
		tile = CLList_Next((unsigned int *)DAT_003082d4, (unsigned int *)tile))
		FUN_00159710((byte *)tile);
	bdSetRenderState(3, MA_U32(DAT_003055d0, 0x48), 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(7, 3, 0);
	bdSetRenderState(5, 1, 0);
	bdPopRenderState(4);
	bdPopRenderState(3);
}

/* 0x00159fb0  Meta/Tiles_Meta.cpp: create the board and clear the grid and the counts
   (the argument is the unused ECX of the original's fastcall) */
void FUN_00159fb0(undefined4 unused)
{
	void *memory;
	int i;

	(void)unused;
	memory = MA_NEW(0x12f0, ma_file_tiles, 0x74e, 0x1001);
	if (memory == NULL)
		DAT_003082c0 = 0;
	else
		DAT_003082c0 = FUN_00159270((int)memory);
	for (i = 0; i < 0x240; i++)
		DAT_003079b8[i] = 0;
	for (i = 0; i < 6; i++)
		DAT_003075d4[i] = 0;
	DAT_003079b0 = 0;
}

/* 0x0015a040  Meta/Tiles_Meta.cpp: CTilePosition_List: add a tile position (group <= 999,
   type mask) at `pos` (its w becomes 0); the original is thiscall on the board */
void *FUN_0015a040(void *board, float *pos, int group, unsigned int mask)
{
	unsigned char *tp;
	int i;

	if (999 < (short)group) {
		MA_PRINTF("****** ERROR!!! Tile group too large!! Maximum value is %d *********\n", 999);
		DbgBreakPoint();
	}
	tp = (unsigned char *)MA_NEW(0x48, ma_file_tiles, 0x7c9, 0x1001);
	if (tp == NULL)
		return NULL;
	MA_I16(tp, 0x00) = 0;
	MA_I16(tp, 0x08) = 0;
	MA_I16(tp, 0x06) = -1;
	MA_I32(tp, 0x24) = 0;
	MA_I32(tp, 0x40) = 0;
	MA_I32(tp, 0x10) = 0;
	MA_I32(tp, 0x14) = 0;
	MA_I32(tp, 0x44) = 0;
	MA_I16(tp, 0x02) = (short)group;
	MA_U32(tp, 0x18) = mask;
	for (i = 0; i < 6; i++) {
		if ((mask & (1u << (i & 0x1f))) == 0) {
			tp[0x1c + i] = 0;
		} else {
			tp[0x1c + i] = 1;
			if (MA_I32(tp, 0x24) == 0)
				MA_I32(tp, 0x24) = i;
		}
	}
	pos[3] = 0.0f;
	MA_F32(tp, 0x28) = pos[0];
	MA_F32(tp, 0x2c) = pos[1];
	MA_F32(tp, 0x30) = pos[2];
	MA_F32(tp, 0x34) = pos[3];
	CLList_Insert((int *)board, (int *)(tp + 0x38), 0);
	return tp;
}

/* 0x0015a110  Meta/Tiles_Meta.cpp: start a timed spark effect (brakespark00.bmp) at `pos`
   for `time` (thiscall on the effect, a tile's +0x4c) */
void FUN_0015a110(undefined4 *self, undefined4 *pos, float time)
{
	self[0] = pos[0];
	self[1] = pos[1];
	self[2] = pos[2];
	self[3] = pos[3];
	self[4] = (undefined4)bLoadResource(NULL, "brakespark00.bmp", 0, 0xdefa);
	*((unsigned char *)self + 0x15) = 0x28;
	*((unsigned char *)self + 0x16) = 0x28;
	*((unsigned char *)self + 0x17) = 0x28;
	*(float *)&self[0xb] = time;
	*(unsigned char *)&self[5] = 0x3c;
	self[6] = 0;
	*(float *)&self[0xc] = time;
	self[8] = self[8] | 1;
	self[7] = 0;
}

/* 0x0015a180  Meta/Tiles_Meta.cpp: put a platform of `type` and `colour` on the tile
   position `tp`; `hidden` leaves it invisible (faded in later) */
int FUN_0015a180(int type, uint colour, undefined2 *tp, int hidden)
{
	int platform;
	void *memory;
	float pos[4];
	float flash[4];
	float scale;
	int other;
	int rays;

	platform = FUN_00159970(type, colour, MA_I32(tp, 0x10));
	if (type == 4) {
		pos[0] = MA_F32(tp, 0x28);
		pos[1] = MA_F32(tp, 0x2c) + 60.0f; /* 0x00254e98 */
		pos[2] = MA_F32(tp, 0x30);
		pos[3] = MA_F32(tp, 0x34);
		/* (the original does not check the platform here) */
		MA_I32(MA_PTR(platform, 0x454), 0x44) = FUN_0015c400(pos, 80.0f, 80.0f, "ai_light_glow_4.bmp",
			0x7f, 0x7f, 0x7f, 0x7f, 75.0f);
		MA_I32(MA_PTR(MA_PTR(platform, 0x454), 0x44), 0x3c) = 1;
	} else if (type == 3) {
		pos[0] = MA_F32(tp, 0x28);
		pos[1] = MA_F32(tp, 0x2c) + 90.0f; /* 0x00255180 */
		pos[2] = MA_F32(tp, 0x30);
		pos[3] = MA_F32(tp, 0x34);
		memory = MA_NEW(0x18, ma_file_tiles, 0x1ff, 0x1001);
		if (memory != NULL)
			AnimTex_Init((struct CAnimTex *)memory, "smokey1.gif", 16.666666f,
				(struct CLFList *)(platform + 0x338));
		MA_F32(platform, 0x344) = 1.0f;
	}
	if (platform == 0) {
		MA_PRINTF("couldn't create platform\n");
		return 0;
	}
	MA_I32(tp, 0x24) = type;
	tp[3] = (short)colour;
	tp[0] = 1;
	tp[4] = 0;
	scale = 0.8f;
	if (type == 4 || type == 2)
		scale = 0.6f;
	MA_F32(platform, 0xf0) = scale;
	MA_F32(platform, 0xf4) = scale;
	MA_F32(platform, 0xf8) = scale;
	MA_PTR(MA_PTR(platform, 0x454), 0x84) = tp;
	MA_F32(platform, 0xe0) = MA_F32(tp, 0x28);
	MA_F32(platform, 0xe4) = MA_F32(tp, 0x2c);
	MA_F32(platform, 0xe8) = MA_F32(tp, 0x30);
	MA_F32(platform, 0xec) = MA_F32(tp, 0x34);
	if (type == 4 || type == 2)
		MA_F32(platform, 0xe4) = MA_F32(platform, 0xe4) + 60.0f; /* 0x00254e98 */
	else
		MA_F32(platform, 0xe4) = MA_F32(platform, 0xe4) + 80.0f; /* 0x0026556c */
	other = MA_I32(MA_PTR(platform, 0x454), 0x38);
	if (other != 0) {
		MA_F32(other, 0xd0) = MA_F32(platform, 0xe0);
		MA_F32(other, 0xd4) = MA_F32(platform, 0xe4);
		MA_F32(other, 0xd8) = MA_F32(platform, 0xe8);
		MA_F32(other, 0xdc) = MA_F32(platform, 0xec);
	}
	other = MA_I32(MA_PTR(platform, 0x454), 0x3c);
	if (other != 0) {
		MA_F32(other, 0xd0) = MA_F32(platform, 0xe0);
		MA_F32(other, 0xd4) = MA_F32(platform, 0xe4);
		MA_F32(other, 0xd8) = MA_F32(platform, 0xe8);
		MA_F32(other, 0xdc) = MA_F32(platform, 0xec);
	}
	if (type == 0) {
		MA_U32(platform, 0x230) = MA_U32(platform, 0x230) | 0x10000;
		MA_F32(platform, 0xe4) = MA_F32(platform, 0xe4) + 20.0f; /* 0x00255530 */
	} else if (type == 2 || type == 4) {
		flash[1] = MA_F32(tp, 0x2c) + 120.0f; /* 0x00255b50 */
		flash[0] = MA_F32(tp, 0x28);
		flash[2] = MA_F32(tp, 0x30);
		flash[3] = 1.0f;
		CProjector_SpiralAt(MA_FP(tp, 0x28), colour & 0xffff);
		Light_Flash(flash, 0x7f, 0x1e, 0);
	} else if (type == 3) {
		CProjector_SpiralAt(MA_FP(tp, 0x28), -1);
		Light_Flash(MA_FP(tp, 0x28), 0x7f, 0x1e, 0);
		MA_PTR(MA_PTR(platform, 0x454), 0x88) = CRay_CreateOrbRayControl(1, MA_FP(platform, 0xe0),
			MA_FP(DAT_003055d4, 0x534), (void *)FUN_000b4bd0);
		rays = MA_I32(MA_PTR(platform, 0x454), 0x88);
		MA_F32(rays, 0x20) = 0.0f;
		MA_F32(rays, 0x24) = 70.0f;
		MA_F32(rays, 0x28) = 0.0f;
		MA_F32(rays, 0x2c) = 1.0f;
#define MA_RAYS ((struct CRayControl *)MA_PTR(MA_PTR(platform, 0x454), 0x88))
		CRayControl_AddRay(MA_RAYS, 0.0f, 0.0f, 500.0f, 1.0f, 34.0f);
		CRayControl_AddRay(MA_RAYS, 3.1415925f, 0.0f, 500.0f, 1.0f, 34.0f);
		CRayControl_AddRay(MA_RAYS, 1.5707963f, 0.0f, 300.0f, 1.0f, 26.0f);
		CRayControl_AddRay(MA_RAYS, 4.712389f, 0.0f, 300.0f, 1.0f, 26.0f);
		CRayControl_AddRay(MA_RAYS, 0.7853981f, 0.0f, 400.0f, 1.0f, 24.0f);
		CRayControl_AddRay(MA_RAYS, 3.9269905f, 0.0f, 400.0f, 1.0f, 24.0f);
		CRayControl_AddRay(MA_RAYS, 2.3561945f, 0.0f, 400.0f, 1.0f, 20.0f);
		CRayControl_AddRay(MA_RAYS, 5.497787f, 0.0f, 400.0f, 1.0f, 20.0f);
#undef MA_RAYS
	}
	if (hidden == 0) {
		baPrepActorInstance((struct BActorInstance *)(platform + 0x20), 1);
	} else {
		MA_I32(MA_PTR(platform, 0x454), 0x18) = 0;
		MA_I32(platform, 0x234) = 0x100;
	}
	MA_U32(platform, 0x230) = MA_U32(platform, 0x230) | 0x20;
	return platform;
}
