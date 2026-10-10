/*
Demolition/CDemolition_EntityEvent.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\CDemolition_EntityEvent.cpp

CDemolition_EntityEventSystem: a priority ring of entity events (0x34 bytes: id, kind,
priority, 9 words of data). Only 0x0010bd80 (initialiseEventQueue) is tagged with this file;
the event class and the other methods around it are inferred.
*/

#include "dd_eventqueue.h"

static const char s_EntityEventFile[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_EntityEvent.cpp";

/* 0x0026bc00  the event's vtable */
const void *const vtable_0026bc00[1] = {
	(const void *)FUN_0010be40, /* [0] vector deleting destructor */
};

/* 0x0026bc04  the event system's vtable */
const void *const vtable_0026bc04[1] = {
	(const void *)FUN_0010c030, /* [0] scalar deleting destructor */
};

/* 0x0010bd10  Demolition (gap; file inferred)
CDemolition_EntityEvent constructor. */
void *DemoEntityEvent_Ctor(void *event)
{
	DemoEntityEvent *self = (DemoEntityEvent *)event;

	self->vtable = (const void *const *)vtable_0026bc00;
	return self;
}

/* 0x0010bd20  Demolition (gap; file inferred)
CDemolition_EntityEvent destructor. */
void DemoEntityEvent_Dtor(void *event)
{
	DemoEntityEvent *self = (DemoEntityEvent *)event;

	self->vtable = (const void *const *)vtable_0026bc00;
}

/* 0x0010bd30  Demolition (gap; file inferred)
CDemolition_EntityEventSystem constructor (an empty queue). */
void *DemoEntityEventSystem_Ctor(void *system)
{
	DemoEventQueue *self = (DemoEventQueue *)system;

	self->vtable = (const void *const *)vtable_0026bc04;
	self->capacity = 0;
	self->count = 0;
	self->head = 0;
	self->events = NULL;
	return self;
}

/* 0x0010bd50  Demolition (gap; file inferred)
CDemolition_EntityEventSystem destructor. */
void DemoEntityEventSystem_Dtor(void *system)
{
	DemoEventQueue *self = (DemoEventQueue *)system;

	self->vtable = (const void *const *)vtable_0026bc04;
	ddq_free_events(self);
}

/* 0x0010bd80  Demolition/CDemolition_EntityEvent.cpp
CDemolition_EntityEventSystem::initialiseEventQueue: makes room for `size` events (a power of
2, else 16). */
void DemoEntityEventSystem_InitQueue(int system, unsigned int size)
{
	DemoEventQueue *self = (DemoEventQueue *)system;
	unsigned int *block;
	unsigned int count;
	unsigned int i;

	if ((size & (size - 1)) != 0) {
		bkPrintf("ERROR: 'CDemolition_EntityEventSystem::initialiseEventQueue' Queue size must be a power of 2\n");
		size = 0x10;
	}
	count = size & 0xffff;
	self->capacity = (unsigned short)size;
	block = (unsigned int *)bkHeapAlloc(count * 0x34 + 4, s_EntityEventFile, 99, 0x1001);
	if (block != NULL) {
		block[0] = count;
		for (i = 0; i < count; i++)
			DemoEntityEvent_Ctor((DemoEntityEvent *)((char *)(block + 1) + i * 0x34));
		self->events = block + 1;
		return;
	}
	self->events = NULL;
}

/* 0x0010be40  Demolition (gap; file inferred)
CDemolition_EntityEvent vector deleting destructor: with flag 2 the whole array (count at
[-1]), with flag 1 also frees it. */
void *DemoEntityEvent_Delete(void *event, unsigned int flags)
{
	DemoEntityEvent *self = (DemoEntityEvent *)event;
	unsigned int count;
	unsigned int *block;

	if ((flags & 2) != 0) {
		block = (unsigned int *)self - 1;
		count = block[0];
		while (count-- != 0)
			DemoEntityEvent_Dtor(self + count);
		if ((flags & 1) != 0)
			bkHeapFree(block);
		return block;
	}
	self->vtable = (const void *const *)vtable_0026bc00;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x0010bea0  Demolition (gap; file inferred)
CDemolition_EntityEventSystem: adds an event (see ddq_push). */
void DemoEntityEventSystem_Push(int system, int event)
{
	DemoEventQueue *self = (DemoEventQueue *)system;
	const char *ev = (const char *)event;

	ddq_push(self, ev, 0x34, 9);
}

/* 0x0010c030  Demolition (gap; file inferred)
CDemolition_EntityEventSystem scalar deleting destructor. */
void *DemoEntityEventSystem_Delete(void *system, unsigned int flags)
{
	DemoEventQueue *self = (DemoEventQueue *)system;

	self->vtable = (const void *const *)vtable_0026bc04;
	ddq_free_events(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}
