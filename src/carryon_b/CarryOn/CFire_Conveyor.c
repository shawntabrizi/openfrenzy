/*
CarryOn/CFire_Conveyor.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Conveyor.cpp

The conveyors: two rows of pieces that run along the conveyor paths. The pieces themselves
are updated and drawn by carryon_a (0x000c9ee0, 0x000ca060).

0x000ca230 is tagged with CFire_Conveyor.cpp; the other functions work on the same object
(file inferred).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_Conveyor.cpp"

/* 0x000ca230  CarryOn/CFire_Conveyor.cpp
Makes `count` pieces on consecutive paths and `count2` more (count2 0: as many) on the
same paths. `unused` is not read. */
void CFire_Conveyor_Init(struct CFire_Conveyor *self, float *pos, float arg, struct CFire_Path *paths,
	int count, int unused, int count2)
{
	struct CFire_ConveyorItem *item;
	int i;

	(void)unused;
	if (count2 == 0)
		count2 = count;
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->unk10 = arg;
	self->numItems = count;
	self->items = bkHeapAlloc(count * 4, CB_FILE, 0x469, 0x2001);
	for (i = 0; i < self->numItems; i++)
	{
		item = bkHeapAlloc(0x48, CB_FILE, 0x46c, 0x1001);
		if (item != NULL)
			memset(item, 0, 0x48);
		self->items[i] = item;
		item = self->items[i];
		item->path = &paths[i];
		item->active = 0;
		item->speed = 18.0f;
		bdVertexBufferCreate(&item->vb, 0x14, 0x144, 0, 1);
		item->texture = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
	}
	self->numItems2 = count2;
	self->items2 = bkHeapAlloc(count2 * 4, CB_FILE, 0x471, 0x2001);
	for (i = 0; i < self->numItems2; i++)
	{
		item = bkHeapAlloc(0x48, CB_FILE, 0x474, 0x1001);
		if (item != NULL)
			memset(item, 0, 0x48);
		self->items2[i] = item;
		item = self->items2[i];
		item->path = &paths[i];
		item->active = 0;
		item->speed = 18.0f;
		bdVertexBufferCreate(&item->vb, 0x14, 0x144, 0, 1);
		item->texture = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
	}
}

/* 0x000ca3f0  CarryOn/CFire_Conveyor.cpp (file inferred)
Updates every piece (carryon_a 0x000c9ee0 with the same arguments). */
void CFire_Conveyor_Update(struct CFire_Conveyor *self, float dt, int arg2, int arg3, int arg4,
	int steps)
{
	int i;

	for (i = 0; i < self->numItems; i++)
		CB_DUAL(FUN_000c9ee0, void (*)(int *, float, undefined4, undefined4, undefined4, int),
			void (*)(struct CFire_ConveyorItem *, float, int, int, int, int),
			((int *)self->items[i], dt, arg2, arg3, arg4, steps),
			(self->items[i], dt, arg2, arg3, arg4, steps));
	for (i = 0; i < self->numItems2; i++)
		CB_DUAL(FUN_000c9ee0, void (*)(int *, float, undefined4, undefined4, undefined4, int),
			void (*)(struct CFire_ConveyorItem *, float, int, int, int, int),
			((int *)self->items2[i], dt, arg2, arg3, arg4, steps),
			(self->items2[i], dt, arg2, arg3, arg4, steps));
}

/* 0x000ca470  CarryOn/CFire_Conveyor.cpp (file inferred)
Draws every piece (carryon_a 0x000ca060). */
void CFire_Conveyor_Draw(struct CFire_Conveyor *self)
{
	int i;

	for (i = 0; i < self->numItems; i++)
		CB_CALL(FUN_000ca060, void (*)(struct CFire_ConveyorItem *))(self->items[i]);
	for (i = 0; i < self->numItems2; i++)
		CB_CALL(FUN_000ca060, void (*)(struct CFire_ConveyorItem *))(self->items2[i]);
}

/* 0x000ca4c0  CarryOn/CFire_Conveyor.cpp (file inferred)
The piece of the first (row2 0) or second row whose path starts nearest to `pos` (x/z),
NULL when none is nearer than 10000. */
struct CFire_ConveyorItem *CFire_Conveyor_FindNearest(struct CFire_Conveyor *self, float *pos,
	int row2)
{
	struct CFire_ConveyorItem **items;
	struct CFire_ConveyorItem *best = NULL;
	float bestDist = 1e8f;                              /* 0x00268784 */
	float dx, dz, d;
	float *start;
	int count;
	int i;

	if (row2 == 0)
	{
		items = self->items;
		count = self->numItems;
	}
	else
	{
		items = self->items2;
		count = self->numItems2;
	}
	for (i = 0; i < count; i++)
	{
		start = items[i]->path->points[0];
		dx = pos[0] - start[0];
		dz = pos[2] - start[2];
		d = dx * dx + dz * dz;
		if (d < bestDist)
		{
			best = items[i];
			bestDist = d;
		}
	}
	return best;
}

/* 0x000ca690  CarryOn/CFire_Conveyor.cpp (file inferred)
Frees the pieces, their vertex buffers and both arrays. */
void CFire_Conveyor_Free(struct CFire_Conveyor *self)
{
	struct CFire_ConveyorItem *item;
	int i;

	for (i = 0; i < self->numItems; i++)
	{
		item = self->items[i];
		if (item != NULL)
		{
			if (item->vb != NULL)
				bdVertexBufferDestroy(item->vb);
			bkHeapFree(item);
		}
	}
	bkHeapFree(self->items);
	self->items = NULL;
	for (i = 0; i < self->numItems2; i++)
	{
		item = self->items2[i];
		if (item != NULL)
		{
			if (item->vb != NULL)
				bdVertexBufferDestroy(item->vb);
			bkHeapFree(item);
		}
	}
	bkHeapFree(self->items2);
	self->items2 = NULL;
}
