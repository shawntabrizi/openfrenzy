/*
C:\Babel\Xbox\Src\bKernel\event.cpp: named events. An event has clients: callbacks, called
when the event is raised, and queues of 0x100-byte parameter strings, read with
bkGetEventMessage.

0x0004d4f0, 0x0004d590 and 0x0004d620 name this file (assert paths). The other functions
here work on the same lists and lie in the same block of code, so they are placed here
too.
*/

#include "bkernel_core.h"

#include <string.h>

/* 0x002d02d8: sentinel of the list of events */
bkEvent DAT_002d02d8;
/* 0x002d0328: guards the client lists */
HANDLE DAT_002d0328;

/* 0x0004d4d0 */
void bkInitEvents(void)
{
	bkEventList.prev = &bkEventList;
	bkEventList.next = &bkEventList;
	bkEventMutex = bkCreateLock();
}

/* finds an event by the crc of its name; NULL when there is none */
static bkEvent *bkFindEvent(uint crc)
{
	bkEvent *event;

	for (event = bkEventList.next; event != &bkEventList; event = event->next)
	{
		if (event->crc == crc)
			return event;
	}
	return NULL;
}

/* 0x0004d4f0  bKernel/event.cpp
bkCreateEvent: creates an event (nothing to do when it exists). 0 when out of memory. */
int bkCreateEvent(const char *name)
{
	uint crc = bkCRCString(name);
	bkEvent *event;

	if (bkFindEvent(crc) != NULL)
		return 1;
	event = (bkEvent *)bkHeapAllocEx(sizeof(bkEvent), "C:\\Babel\\Xbox\\Src\\bKernel\\event.cpp",
		0x5b, 0x2001, "Event", 0);
	if (event == NULL)
		return 0;
	event->prev = bkEventList.prev;
	event->next = &bkEventList;
	bkEventList.prev->next = event;
	event->next->prev = event;
	strcpy(event->name, name);
	event->clients.prev = &event->clients;
	event->clients.next = &event->clients;
	event->crc = crc;
	event->numQueues = 0;
	return 1;
}

/* links a new client at the end of an event's clients */
static void bkLinkEventClient(bkEvent *event, bkEventClient *client)
{
	bkEventClient *last = event->clients.prev;

	client->next = &event->clients;
	client->prev = last;
	last->next = client;
	client->next->prev = client;
}

/* 0x0004d590  bKernel/event.cpp
bkAddEventCallback: callback(eventName, parameters, context) runs each time the event is
raised. NULL when there is no such event. */
struct bkEventClient *bkAddEventCallback(const char *name, bkEventCallback callback, void *context)
{
	bkEvent *event = bkFindEvent(bkCRCString(name));
	bkEventClient *client;

	if (event == NULL)
		return NULL;
	client = (bkEventClient *)bkHeapAllocEx(sizeof(bkEventClient),
		"C:\\Babel\\Xbox\\Src\\bKernel\\event.cpp", 0x7c, 0x2001, "Event Client (Callback)", 0);
	if (client == NULL)
		return NULL;
	bkLinkEventClient(event, client);
	client->event = event;
	client->kind = BK_EVENT_CLIENT_CALLBACK;
	client->callback = callback;
	client->callbackContext = context;
	return client;
}

/* 0x0004d620  bKernel/event.cpp
bkAddEventQueue: a queue of size parameter strings for the event (flags:
BK_EVENT_QUEUE_*). NULL when there is no such event. */
struct bkEventClient *bkAddEventQueue(const char *name, int size, uint flags)
{
	bkEvent *event = bkFindEvent(bkCRCString(name));
	bkEventClient *client;

	if (event == NULL)
		return NULL;
	client = (bkEventClient *)bkHeapAllocEx(size * 0x100 + sizeof(bkEventClient),
		"C:\\Babel\\Xbox\\Src\\bKernel\\event.cpp", 0x9d, 0x2001, "Event Client (Queue)", 0);
	if (client == NULL)
		return NULL;
	bkLinkEventClient(event, client);
	client->queueSize = size;
	client->flags = flags;
	client->event = event;
	client->kind = BK_EVENT_CLIENT_QUEUE;
	client->queue = (char *)(client + 1);
	client->queued = 0;
	event->numQueues++;
	return client;
}

/* 0x0004d6d0
bkGetEventMessage: takes the oldest (or with BK_EVENT_QUEUE_LIFO the newest) parameter
string of a queue into parameters (when not NULL). 0 when the queue is empty. */
int bkGetEventMessage(struct bkEventClient *client, char *parameters)
{
	if (client->queued == 0)
		return 0;
	bkLock(bkEventMutex);
	if ((client->flags & BK_EVENT_QUEUE_LIFO) == 0)
	{
		if (parameters != NULL && client->queue != NULL)
			strcpy(parameters, client->queue);
		if (client->queued > 1)
			memmove(client->queue, client->queue + 0x100, (client->queued - 1) * 0x100);
		client->queued--;
	}
	else
	{
		client->queued--;
		if (parameters != NULL && client->queue + client->queued * 0x100 != NULL)
			strcpy(parameters, client->queue + client->queued * 0x100);
	}
	bkUnlock(bkEventMutex);
	return 1;
}

/* frees every client of an event (the loop of 0x0004d780 and 0x0004da50) */
static void bkFreeEventClients(bkEvent *event)
{
	bkEventClient *node = event->clients.next;

	while (node != &event->clients)
	{
		node = node->next;
		bkHeapFree(node->prev);
		node->prev = NULL;
	}
	event->clients.prev = &event->clients;
	event->clients.next = &event->clients;
}

/* 0x0004d780 (static in the original: the event in EBX)
bkDestroyEvent: frees an event and its clients. */
void bkDestroyEvent(struct bkEvent *event)
{
	bkLock(bkEventMutex);
	bkFreeEventClients(event);
	event->next->prev = event->prev;
	event->prev->next = event->next;
	bkUnlock(bkEventMutex);
	bkHeapFree(event);
}

/* 0x0004d7f0
bkDeleteEvent: deletes an event by name (NULL: every event). */
void bkDeleteEvent(const char *name)
{
	bkEvent *node;
	bkEvent *event;

	if (name != NULL)
	{
		event = bkFindEvent(bkCRCString(name));
		if (event != NULL)
			bkDestroyEvent(event);
		return;
	}
	node = bkEventList.next;
	while (node != &bkEventList)
	{
		node = node->next;
		bkDestroyEvent(node->prev);
	}
}

/* 0x0004d860
bkRemoveEventClient: unlinks and frees one client. */
void bkRemoveEventClient(struct bkEventClient *client)
{
	bkLock(bkEventMutex);
	client->next->prev = client->prev;
	client->prev->next = client->next;
	if (client->kind == BK_EVENT_CLIENT_QUEUE)
		client->event->numQueues--;
	bkHeapFree(client);
	bkUnlock(bkEventMutex);
}

/* 0x0004d8b0
bkGenerateEvent: raises an event: calls its callbacks and adds parameters (NULL: an empty
string) to its queues. 0 when there is no such event. */
int bkGenerateEvent(const char *name, const char *parameters)
{
	bkEvent *event;
	bkEventClient *client;
	char *slot;

	bkLock(bkEventMutex);
	event = bkFindEvent(bkCRCString(name));
	if (event == NULL)
	{
		bkUnlock(bkEventMutex);
		return 0;
	}
	for (client = event->clients.next; client != &event->clients; client = client->next)
	{
		if (client->kind == BK_EVENT_CLIENT_CALLBACK)
		{
			client->callback(name, parameters, client->callbackContext);
			continue;
		}
		if (client->queued == client->queueSize)
		{
			if ((client->flags & BK_EVENT_QUEUE_OVERWRITE) == 0)
			{
				bkPrintf("bkGenerateEvent: *** WARNING *** Event '%s' lost parameters '%s' due to queue overflow ***\n",
					name, parameters != NULL ? parameters : "[NULL]");
				continue;
			}
			if (client->queueSize > 1)
				memmove(client->queue, client->queue + 0x100, (client->queueSize - 1) * 0x100);
			client->queued--;
		}
		slot = client->queue + client->queued * 0x100;
		if (parameters == NULL)
			*slot = '\0';
		else
			strcpy(slot, parameters);
		client->queued++;
	}
	bkUnlock(bkEventMutex);
	return 1;
}

/* 0x0004da50
bkFreeEventClientsByName: frees every client of an event. The original does not check
that the event exists (it would use a NULL event). */
void bkFreeEventClientsByName(const char *name)
{
	bkEvent *event = bkFindEvent(bkCRCString(name));

	bkLock(bkEventMutex);
	bkFreeEventClients(event);
	bkUnlock(bkEventMutex);
}

/* 0x0004dae0
bkShutdownEvents: deletes every event and the lock. */
void bkShutdownEvents(void)
{
	bkEvent *node = bkEventList.next;

	while (node != &bkEventList)
	{
		node = node->next;
		bkDestroyEvent(node->prev);
	}
	bkDestroyLock(bkEventMutex);
}
