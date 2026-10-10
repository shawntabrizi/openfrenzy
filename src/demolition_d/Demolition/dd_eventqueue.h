/*
Demolition/dd_eventqueue.h - the priority ring shared by CDemolition_EntityEvent.c and
CDemolition_Event.c (demolition_d only). The two original classes have the same code for
events of different sizes (0x34 and 0x24 bytes); this is that code once, with the size.

An event: vtable +0, short id +4, int kind +8, int priority +0xc, data from +0x10.
*/

#ifndef DEMOLITION_D_EVENTQUEUE_H
#define DEMOLITION_D_EVENTQUEUE_H

#include "demolition_d.h"

#define DDQ_EVENT(q, size, i) ((char *)(q)->events + (unsigned int)(i) * (size))

/* copy an event without its vtable */
static inline void ddq_copy(char *dst, const char *src, int dataWords)
{
	DD_I16(dst, 4) = DD_I16(src, 4);
	DD_I32(dst, 8) = DD_I32(src, 8);
	DD_I32(dst, 0xc) = DD_I32(src, 0xc);
	memcpy(dst + 0x10, src + 0x10, (size_t)dataWords * 4);
}

/* Adds `ev`: an event of the same id (when kind != 0) only gets the new data; else the
   event goes before the first one of lower or equal priority. A full queue drops the new
   event if its priority is below the head's, else loses its last place. */
static inline void ddq_push(DemoEventQueue *q, const char *ev, unsigned int size, int dataWords)
{
	unsigned int idx;
	unsigned int cap;
	unsigned int count;
	unsigned int dst;
	unsigned int src;
	int skip;
	int n;
	int i;

	if (DD_I32(ev, 8) != 0) {
		idx = q->head;
		for (i = 0; i < (int)q->count; i++) {
			if (DD_I16(ev, 4) == DD_I16(DDQ_EVENT(q, size, idx), 4)) {
				memcpy(DDQ_EVENT(q, size, idx) + 0x10, ev + 0x10, (size_t)dataWords * 4);
				return;
			}
			idx = (idx + 1) & (unsigned int)(q->capacity - 1);
		}
	}
	count = q->count;
	cap = q->capacity;
	if (cap <= count) {
		if (DD_I32(ev, 0xc) < DD_I32(DDQ_EVENT(q, size, (q->head + count) & (cap - 1)), 0xc))
			return;
		q->count = (unsigned short)(count - 1);
	}
	idx = q->head;
	skip = 0;
	if (q->count != 0) {
		do {
			if (DD_I32(DDQ_EVENT(q, size, idx), 0xc) <= DD_I32(ev, 0xc))
				break;
			idx = (idx + 1) & (cap - 1);
			skip++;
		} while (skip < (int)q->count);
	}
	n = (int)q->count - skip;
	dst = ((unsigned int)n + idx) & (cap - 1);
	for (i = 0; i < n; i++) {
		src = dst - 1;
		if ((int)src < 0)
			src = (unsigned int)q->capacity - 1;
		ddq_copy(DDQ_EVENT(q, size, dst), DDQ_EVENT(q, size, src), dataWords);
		dst = src;
	}
	ddq_copy(DDQ_EVENT(q, size, idx), ev, dataWords);
	q->count = (unsigned short)(q->count + 1);
}

/* the queue's destructor part: the events array goes through its vector deleting destructor
   (flags 3) when it holds events, else it is freed */
static inline void ddq_free_events(DemoEventQueue *q)
{
	char *events;

	events = (char *)q->events;
	if (events == NULL)
		return;
	if (DD_I32(events, -4) != 0) {
		DD_VFN(events, 0, void *(*)(void *, unsigned int))(events, 3);
		return;
	}
	bkHeapFree(events - 4);
}

#endif
