/*
TankedUp/CTank_MoveQue.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_MoveQue.cpp

A ring of moves (4 floats each) that the AI tanks replay: Put adds a move three times per
frame unit, Get returns the oldest and drops one per frame unit (keeping the last).
  +0x00 the moves (16 bytes each)   +0x04 write index   +0x08 read index
  +0x0c count                       +0x10 capacity
0x001de780 is tagged with CTank_MoveQue.cpp; the methods after it are trailing functions of
the module range (file inferred). 0x001de950 is an empty C runtime function.
*/

#include "tankedup_b.h"

#define MQ_MOVES 0x00
#define MQ_WRITE 0x04
#define MQ_READ 0x08
#define MQ_COUNT 0x0c
#define MQ_CAPACITY 0x10

#define MQ_MOVE(self, i) ((float *)((char *)TU_PTR(self, MQ_MOVES) + (i) * 0x10))

/* 0x001de780  TankedUp/CTank_MoveQue.cpp
CTank_MoveQue constructor: room for `count` moves. */
void *TankMoveQue_Ctor(void *self, int count)
{
	TU_PTR(self, MQ_MOVES) = bkHeapAlloc((uint)(count << 4), TU_FILE_MOVEQUE, 8, 0x2001);
	TU_I32(self, MQ_CAPACITY) = count;
	TU_I32(self, MQ_WRITE) = 0;
	TU_I32(self, MQ_READ) = 0;
	TU_I32(self, MQ_COUNT) = 0;
	return self;
}

/* 0x001de7c0  TankedUp (trailing; CTank_MoveQue.cpp inferred)
The oldest move (zeros when empty); then one move per frame unit is dropped, never the
last. */
void TankMoveQue_Get(void *self, float *a, float *b, float *c, float *d)
{
	float *move;
	int i;

	if (TU_I32(self, MQ_COUNT) == 0) {
		*a = 0.0f;
		*b = 0.0f;
		*c = 0.0f;
		*d = 0.0f;
		return;
	}
	move = MQ_MOVE(self, TU_I32(self, MQ_READ));
	*a = move[0];
	*b = move[1];
	*c = move[2];
	*d = move[3];
	i = 0;
	if (0.0f < DAT_0028fda8) {
		do {
			if (1 < TU_I32(self, MQ_COUNT)) {
				TU_I32(self, MQ_COUNT) = TU_I32(self, MQ_COUNT) - 1;
				TU_I32(self, MQ_READ) = (TU_I32(self, MQ_READ) + 1) % TU_I32(self, MQ_CAPACITY);
			}
			i++;
		} while ((float)i < DAT_0028fda8);
	}
}

/* 0x001de890  TankedUp (trailing; CTank_MoveQue.cpp inferred)
Adds a move three times per frame unit while there is room. */
void TankMoveQue_Put(void *self, float a, float b, float c, float d)
{
	float *move;
	int i = 0;

	if (0.0f < DAT_0028fda8 * 3.0f) { /* 0x0025384c */
		do {
			if (TU_I32(self, MQ_COUNT) < TU_I32(self, MQ_CAPACITY)) {
				move = MQ_MOVE(self, TU_I32(self, MQ_WRITE));
				move[0] = a;
				move[1] = b;
				move[2] = c;
				move[3] = d;
				TU_I32(self, MQ_COUNT) = TU_I32(self, MQ_COUNT) + 1;
				TU_I32(self, MQ_WRITE) = (TU_I32(self, MQ_WRITE) + 1) % TU_I32(self, MQ_CAPACITY);
			}
			i++;
		} while ((float)i < DAT_0028fda8 * 3.0f);
	}
}

/* 0x001de940  TankedUp (trailing; CTank_MoveQue.cpp inferred)
CTank_MoveQue destructor: frees the moves. */
void TankMoveQue_Free(void *self)
{
	bkHeapFree(TU_PTR(self, MQ_MOVES));
}

/* 0x001de950  (trailing; an empty function of the C runtime tables, 0x002b74b8) */
void FUN_001de950(void)
{
}
