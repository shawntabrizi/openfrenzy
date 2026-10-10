/*
Rowing/unplaced_input.c  <-  Rowing gap 0x001af720-0x001afa50 (no file tag)

Input channels: a player's sticks and buttons (player +0x170, see Rowing/CRowing_Player.c)
and the game's pause channel (game +0x94). No string names the source file.

The player's input (fields at +0x170 + offset): +0x00 the player index, +0x04 the map,
+0x08 leftX, +0x0c leftY, +0x10 rightX, +0x14 rightY, +0x18 action1 (no debounce),
+0x1c action2 (no debounce), +0x20 action1, +0x24 action2, +0x28..+0x34 cleared.
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"

#define g_RowPlayer(i) RW_VAR(void *, 0x0030f490 + (i) * 4)

/* 0x001af720  Rowing (gap)
Creates player INDEX's input map and binds it to the pad in the player's record
(DAT_002baed4[index * 10 + 3]): the sticks (also the d-pad on the left stick), A for
action 1, B and X for action 2. Returns 1. */
int FUN_001af720(void *self, int index)
{
	struct biMap *map;
	unsigned int pad;

	RA_I32(self, 0x0) = index;
	map = (struct biMap *)biCreateMap(8);
	RA_PTRF(self, 0x4) = map;
	RA_PTRF(self, 0x8) = biCreateChannel(map, "leftX", 3, 0, 0, 0, 0);
	RA_PTRF(self, 0xc) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x4), "leftY", 3, 0, 0, 0, 0);
	RA_PTRF(self, 0x10) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x4), "rightX", 3, 0, 0, 0, 0);
	RA_PTRF(self, 0x14) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x4), "rightY", 3, 0, 0, 0, 0);
	RA_PTRF(self, 0x20) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x4), "action1", 1, 1, 0, 0, 0);
	RA_PTRF(self, 0x24) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x4), "action2", 1, 1, 0, 0, 0);
	RA_PTRF(self, 0x18) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x4), "action1ndb", 1, 0, 0, 0, 0);
	RA_PTRF(self, 0x1c) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x4), "action2ndb", 1, 0, 0, 0, 0);
	pad = (unsigned int)DAT_002baed4[index * 10 + 3];
	biBindInput((struct biChannel *)RA_PTRF(self, 0x20), pad | 0xc5, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x24), pad | 0xc7, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x24), pad | 0xc6, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x18), pad | 0xc5, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x1c), pad | 0xc7, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x1c), pad | 0xc6, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x8), pad | 0xc0, pad | 0xbf);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x8), 0xa7, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0xc), 0xa8, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x10), 0xa9, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x14), 0xaa, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x8), pad | 0xb9, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0xc), pad | 0xba, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x10), pad | 0xbb, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x14), pad | 0xbc, 0);
	RA_I32(self, 0x28) = 0;
	RA_I32(self, 0x2c) = 0;
	RA_I32(self, 0x30) = 0;
	RA_I32(self, 0x34) = 0;
	return 1;
}

/* 0x001af950  Rowing (gap)
Frees a player's input map. */
void FUN_001af950(void *self)
{
	biDeleteMap((struct biMap *)RA_PTRF(self, 0x4));
}

/* 0x001af960  Rowing (gap)
Creates the game's "pause" channel (+0x00 the map, +0x04 the channel), bound to input
0xb6. Returns 1. */
int FUN_001af960(void *self)
{
	RA_PTRF(self, 0x0) = biCreateMap(4);
	RA_PTRF(self, 0x4) = biCreateChannel((struct biMap *)RA_PTRF(self, 0x0), "pause", 1, 0, 0, 0, 0);
	biBindInput((struct biChannel *)RA_PTRF(self, 0x4), 0xb6, 0);
	return 1;
}

/* 0x001af9a0  Rowing (gap)
Frees the pause channel's map. */
void FUN_001af9a0(void *self)
{
	biDeleteMap((struct biMap *)RA_PTRF(self, 0x0));
}

/* 0x001af9b0  Rowing (gap)
1 when any of the four players holds an action button (no debounce). */
int FUN_001af9b0(void)
{
	int i, held = 0;

	for (i = 0; i < 4; i++)
		if (RA_I32(RA_PTRF(g_RowPlayer(i), 0x190), 0x14) != 0 || RA_I32(RA_PTRF(g_RowPlayer(i), 0x194), 0x14) != 0)
			held = 1;
	return held;
}
