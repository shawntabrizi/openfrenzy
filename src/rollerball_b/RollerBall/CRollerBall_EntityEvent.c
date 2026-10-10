/*
RollerBall/CRollerBall_EntityEvent.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerBall_EntityEvent.cpp (heap tag and error text of
0x00196ad0, "CRollerBall_EntityEventSystem::initialiseEventQueue"):

- the event (0x2c bytes, vtable 0x002749d0; rollerball_a's struct RB_Msg): +0x04 short id,
  +0x06 byte unique (a queued event with the same id is replaced), +0x07 byte priority,
  +0x08 nine words of data.
- CRollerBall_EntityEventSystem (vtable 0x002749d4), the base class of the player
  entities: a ring buffer of events (+0x04 array, +0x08 size, a power of 2, +0x0a first,
  +0x0c count) kept in priority order, highest first.
*/

#include "rollerball_b.h"

#define RBEV_ID 0x04
#define RBEV_UNIQUE 0x06
#define RBEV_PRIORITY 0x07
#define RBEV_DATA 0x08
#define RBEV_SIZE 0x2c

#define RBEQ_EVENTS 0x04
#define RBEQ_SIZE 0x08
#define RBEQ_FIRST 0x0a
#define RBEQ_COUNT 0x0c

/* 0x002749d0  event vtable */
const void *const rbb_vtable_002749d0[1] = {
	(const void *)FUN_00196b90, /* +0x00 vector deleting destructor */
};

/* 0x002749d4  CRollerBall_EntityEventSystem vtable */
const void *const rbb_vtable_002749d4[1] = {
	(const void *)FUN_00196d90, /* +0x00 scalar deleting destructor */
};

/* 0x00196a60  RollerBall/CRollerBall_EntityEvent.cpp (inferred)
Event constructor. */
void CRollerBallEvent_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_002749d0;
}

/* 0x00196a70  RollerBall/CRollerBall_EntityEvent.cpp (inferred)
Event destructor. */
void CRollerBallEvent_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_002749d0;
}

/* free the event array: by its vector deleting destructor when it holds events (the
count word in front of the array), else directly */
static void rbb_FreeEvents(void *self)
{
	char *events = RBB_P(self, RBEQ_EVENTS);

	if (events == NULL)
		return;
	if (RBB_I(events, -4) != 0)
		RBB_VFN(events, 0x00, void *(*)(void *, unsigned int))(events, 3);
	else
		RBB_DELETE(events - 4);
}

/* 0x00196a80  RollerBall/CRollerBall_EntityEvent.cpp (inferred)
CRollerBall_EntityEventSystem constructor. */
void CRollerBallEntityEventSystem_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_002749d4;
	RBB_S(self, RBEQ_SIZE) = 0;
	RBB_S(self, RBEQ_COUNT) = 0;
	RBB_S(self, RBEQ_FIRST) = 0;
	RBB_P(self, RBEQ_EVENTS) = NULL;
}

/* 0x00196aa0  RollerBall/CRollerBall_EntityEvent.cpp (inferred)
CRollerBall_EntityEventSystem destructor. */
void CRollerBallEntityEventSystem_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_002749d4;
	rbb_FreeEvents(self);
}

/* 0x00196ad0  RollerBall/CRollerBall_EntityEvent.cpp
initialiseEventQueue: make the event array (`size` events; 16 when `size` is not a power
of 2). The array has its count in the word in front (C++ new[]). */
void CRollerBallEntityEventSystem_InitQueue(void *self, unsigned int size)
{
	unsigned int n;
	unsigned int *mem;
	unsigned int i;

	if ((size & (size - 1)) != 0) {
		bkPrintf("ERROR: 'CRollerBall_EntityEventSystem::initialiseEventQueue' Queue size must be a power of 2\n");
		size = 0x10;
	}
	n = size & 0xffff;
	RBB_S(self, RBEQ_SIZE) = (short)size;
	mem = RBB_NEW(n * RBEV_SIZE + 4, "C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerBall_EntityEvent.cpp", 99);
	if (mem == NULL) {
		RBB_P(self, RBEQ_EVENTS) = NULL;
		return;
	}
	*mem = n;
	for (i = 0; i < n; i++)
		CRollerBallEvent_Ctor((char *)(mem + 1) + i * RBEV_SIZE);
	RBB_P(self, RBEQ_EVENTS) = mem + 1;
}

/* 0x00196b90  RollerBall/CRollerBall_EntityEvent.cpp (inferred)
Event vector deleting destructor: flags & 2 destroys the whole array (count in front),
flags & 1 frees the memory. */
void *CRollerBallEvent_DeletingDtor(void *self, unsigned int flags)
{
	if (flags & 2) {
		int n = RBB_I(self, -4);
		while (n-- > 0)   /* destroyed last to first */
			CRollerBallEvent_Dtor((char *)self + n * RBEV_SIZE);
		if (flags & 1)
			RBB_DELETE((char *)self - 4);
		return (char *)self - 4;
	}
	*(const void *const **)self = rbb_vtable_002749d0;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* copy an event's id, flags and data (not its vtable) */
static void rbb_CopyEvent(char *dst, const char *src)
{
	RBB_S(dst, RBEV_ID) = RBB_S(src, RBEV_ID);
	RBB_B(dst, RBEV_UNIQUE) = RBB_B(src, RBEV_UNIQUE);
	RBB_B(dst, RBEV_PRIORITY) = RBB_B(src, RBEV_PRIORITY);
	memcpy(dst + RBEV_DATA, src + RBEV_DATA, 9 * 4);
}

/* 0x00196bf0  RollerBall/CRollerBall_EntityEvent.cpp (inferred)
Post an event: a unique event replaces the data of a queued one with its id; else it is
inserted before the first queued event of a lower priority. When the queue is full the
event is dropped if its priority is lower than the one at slot first + count (the
original compares with that slot, which is the first event when the queue is full),
else the last event is dropped. */
void CRollerBallEntityEventSystem_Post(void *self, void *event)
{
	char *events = RBB_P(self, RBEQ_EVENTS);
	unsigned int mask;
	unsigned int idx;
	unsigned int dst;
	unsigned short count;
	unsigned short size;
	int skipped;
	int moves;
	int i;

	if (RBB_C(event, RBEV_UNIQUE) != 0) {
		idx = RBB_US(self, RBEQ_FIRST);
		for (i = 0; i < (int)RBB_US(self, RBEQ_COUNT); i++) {
			char *e = events + idx * RBEV_SIZE;
			if (RBB_S(event, RBEV_ID) == RBB_S(e, RBEV_ID)) {
				memcpy(e + RBEV_DATA, (char *)event + RBEV_DATA, 9 * 4);
				return;
			}
			idx = (idx + 1) & (unsigned int)(RBB_US(self, RBEQ_SIZE) - 1);
		}
	}

	count = RBB_US(self, RBEQ_COUNT);
	size = RBB_US(self, RBEQ_SIZE);
	mask = (unsigned int)size - 1;
	if (size <= count) {
		char *e = events + (((unsigned int)RBB_US(self, RBEQ_FIRST) + count) & mask) * RBEV_SIZE;
		if (RBB_B(event, RBEV_PRIORITY) < RBB_B(e, RBEV_PRIORITY))
			return;
		RBB_US(self, RBEQ_COUNT) = (unsigned short)(count - 1);
	}

	/* the insertion slot: before the first event of a lower (or equal) priority */
	idx = RBB_US(self, RBEQ_FIRST);
	skipped = 0;
	while (skipped < (int)RBB_US(self, RBEQ_COUNT)) {
		if (RBB_B(events + idx * RBEV_SIZE, RBEV_PRIORITY) <= RBB_B(event, RBEV_PRIORITY))
			break;
		idx = (idx + 1) & mask;
		skipped++;
	}
	/* move the events behind it one slot back */
	moves = (int)RBB_US(self, RBEQ_COUNT) - skipped;
	dst = ((unsigned int)moves + idx) & mask;
	for (i = 0; i < (int)RBB_US(self, RBEQ_COUNT) - skipped; i++) {
		unsigned int src = dst - 1;
		if ((int)src < 0)
			src = (unsigned int)RBB_US(self, RBEQ_SIZE) - 1;
		rbb_CopyEvent(events + dst * RBEV_SIZE, events + src * RBEV_SIZE);
		dst = src;
	}
	rbb_CopyEvent(events + idx * RBEV_SIZE, (const char *)event);
	RBB_S(self, RBEQ_COUNT) = (short)(RBB_S(self, RBEQ_COUNT) + 1);
}

/* 0x00196d90  RollerBall/CRollerBall_EntityEvent.cpp (inferred)
CRollerBall_EntityEventSystem scalar deleting destructor. */
void *CRollerBallEntityEventSystem_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallEntityEventSystem_Dtor(self);
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}
