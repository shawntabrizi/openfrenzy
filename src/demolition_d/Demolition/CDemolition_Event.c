/*
Demolition/CDemolition_Event.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\CDemolition_Event.cpp

CDemolition_CharLibEventSystem: a priority ring of CharLib events (0x24 bytes: id, kind,
priority, 5 words of data; demolition_b's struct DbMessage is one). Each player has one at
+0x1130 and receives its hit messages through DemoCharEventSystem_Push. Only 0x0010cf30
(initialiseEventQueue) is tagged with this file; the event class and the other methods
around it are inferred.
*/

#include "dd_eventqueue.h"

static const char s_EventFile[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Event.cpp";

/* 0x0026bd20  the event's vtable */
const void *const vtable_0026bd20[1] = {
	(const void *)FUN_0010cff0, /* [0] vector deleting destructor */
};

/* 0x0026bd24  the event system's vtable */
const void *const vtable_0026bd24[1] = {
	(const void *)FUN_0010d1d0, /* [0] scalar deleting destructor */
};

/* 0x0010cec0  Demolition (gap; file inferred)
CDemolition_CharLibEvent constructor. */
void *DemoCharEvent_Ctor(void *event)
{
	DemoCharEvent *self = (DemoCharEvent *)event;

	self->vtable = (const void *const *)vtable_0026bd20;
	return self;
}

/* 0x0010ced0  Demolition (gap; file inferred)
CDemolition_CharLibEvent destructor. */
void DemoCharEvent_Dtor(void *event)
{
	DemoCharEvent *self = (DemoCharEvent *)event;

	self->vtable = (const void *const *)vtable_0026bd20;
}

/* 0x0010cee0  Demolition (gap; file inferred)
CDemolition_CharLibEventSystem constructor (an empty queue). */
void *DemoCharEventSystem_Ctor(void *system)
{
	DemoEventQueue *self = (DemoEventQueue *)system;

	self->vtable = (const void *const *)vtable_0026bd24;
	self->capacity = 0;
	self->count = 0;
	self->head = 0;
	self->events = NULL;
	return self;
}

/* 0x0010cf00  Demolition (gap; file inferred)
CDemolition_CharLibEventSystem destructor. */
void DemoCharEventSystem_Dtor(void *system)
{
	DemoEventQueue *self = (DemoEventQueue *)system;

	self->vtable = (const void *const *)vtable_0026bd24;
	ddq_free_events(self);
}

/* 0x0010cf30  Demolition/CDemolition_Event.cpp
CDemolition_CharLibEventSystem::initialiseEventQueue: makes room for `size` events (a power
of 2, else 16). */
void DemoCharEventSystem_InitQueue(int system, unsigned int size)
{
	DemoEventQueue *self = (DemoEventQueue *)system;
	unsigned int *block;
	unsigned int count;
	unsigned int i;

	if ((size & (size - 1)) != 0) {
		bkPrintf("ERROR: 'CDemolition_CharLibEventSystem::initialiseEventQueue' Queue size must be a power of 2\n");
		size = 0x10;
	}
	count = size & 0xffff;
	self->capacity = (unsigned short)size;
	block = (unsigned int *)bkHeapAlloc(count * 0x24 + 4, s_EventFile, 99, 0x1001);
	if (block != NULL) {
		block[0] = count;
		for (i = 0; i < count; i++)
			DemoCharEvent_Ctor((DemoCharEvent *)((char *)(block + 1) + i * 0x24));
		self->events = block + 1;
		return;
	}
	self->events = NULL;
}

/* 0x0010cff0  Demolition (gap; file inferred)
CDemolition_CharLibEvent vector deleting destructor: with flag 2 the whole array (count at
[-1]), with flag 1 also frees it. */
void *DemoCharEvent_Delete(void *event, unsigned int flags)
{
	DemoCharEvent *self = (DemoCharEvent *)event;
	unsigned int count;
	unsigned int *block;

	if ((flags & 2) != 0) {
		block = (unsigned int *)self - 1;
		count = block[0];
		while (count-- != 0)
			DemoCharEvent_Dtor(self + count);
		if ((flags & 1) != 0)
			bkHeapFree(block);
		return block;
	}
	self->vtable = (const void *const *)vtable_0026bd20;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x0010d050  Demolition (gap; file inferred)
CDemolition_CharLibEventSystem: adds an event (see ddq_push). */
void DemoCharEventSystem_Push(int system, int event)
{
	DemoEventQueue *self = (DemoEventQueue *)system;
	const char *ev = (const char *)event;

	ddq_push(self, ev, 0x24, 5);
}

/* 0x0010d1d0  Demolition (gap; file inferred)
CDemolition_CharLibEventSystem scalar deleting destructor. */
void *DemoCharEventSystem_Delete(void *system, unsigned int flags)
{
	DemoEventQueue *self = (DemoEventQueue *)system;

	self->vtable = (const void *const *)vtable_0026bd24;
	ddq_free_events(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}
