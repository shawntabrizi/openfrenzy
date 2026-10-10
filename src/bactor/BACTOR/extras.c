/*
BACTOR/extras.c  <-  C:\Babel\Xbox\Src\BACTOR\extras.cpp

"Extras": named systems of small fixed-size items (particles, debris, ...)
with callbacks to create, update, delete and draw them, and a placement
(position, rotation, optionally relative to a matrix and a node).

0x0003c780 is tagged with extras.cpp; the functions around it are placed
here by address order (file inferred).

The item creation callback gets the variadic arguments of baAddExtraItem
as a va_list (the original passes a pointer to them on the stack).
*/

#include <stdarg.h>
#include <string.h>

#include "bactor.h"
#include "bactor_extern.h"

/* 0x002bc328: the head of the extra system list (next at 0x08, prev at 0x0c) */
struct BExtraSystem DAT_002bc328;

typedef int (*BExtraSystemFn)(struct BExtraSystem *system, int deleted);
typedef int (*BExtraItemFn)(struct BExtraSystem *system, void *item);
typedef int (*BExtraInitFn)(struct BExtraSystem *system, void *item, void *args);
typedef int (*BExtraListFn)(struct BExtraSystem *system, int numItems, void **items, void *arg);

/* 0x0003c3c0  BACTOR/extras.cpp (file inferred) */
int baInitExtras(void)
{
	DAT_002bc328.prev = &DAT_002bc328;
	DAT_002bc328.next = &DAT_002bc328;
	return 1;
}

/* 0x0003c3e0  BACTOR/extras.cpp (file inferred)
deletes every item of a system (NULL: of every system) */
void baClearExtraSystem(struct BExtraSystem *system)
{
	struct BExtraSystem *s;

	if (system == NULL)
	{
		for (s = DAT_002bc328.next; s != &DAT_002bc328; s = s->next)
			baClearExtraSystem(s);
		return;
	}
	if (system->numItems == 0)
		return;
	if (system->deleteItem != NULL)
	{
		do
		{
			system->numItems--;
			((BExtraItemFn)system->deleteItem)(system, system->items[system->numItems]);
		} while (system->numItems != 0);
	}
	system->numItems = 0;
	system->nextFree = 0;
	memset(system->used, 0, (system->maxItems & 0x3fffffff) * sizeof(int));
}

/* 0x0003c470  BACTOR/extras.cpp (file inferred)
computes the matrix of a system from its position, rotation and parent.
Original register argument: eax system. */
void bUpdateExtraMatrix(struct BExtraSystem *system)
{
	float translation[16];
	float local[16];
	float rotation[16];
	float parent[16];

	system->flags &= ~4u;
	ba_MatTranslation(translation, system->pos[0], system->pos[1], system->pos[2]);
	ba_MatFromQuat(rotation, system->rot);
	if (system->parentMatrix == NULL)
	{
		if (((uintptr_t)system & 0xf) == 0)
			ba_MatMulAligned(system->matrix, translation, rotation);
		else
			ba_MatMul(system->matrix, translation, rotation);
		return;
	}
	if ((((uintptr_t)local | (uintptr_t)rotation | (uintptr_t)translation) & 0xf) == 0)
		ba_MatMulAligned(local, translation, rotation);
	else
		ba_MatMul(local, translation, rotation);
	if (system->parentNode == NULL)
	{
		if ((((uintptr_t)system->parentMatrix | (uintptr_t)system | (uintptr_t)local) & 0xf) == 0)
			ba_MatMulAligned(system->matrix, system->parentMatrix, local);
		else
			ba_MatMul(system->matrix, system->parentMatrix, local);
		return;
	}
	if ((((uintptr_t)system->parentNode | (uintptr_t)system->parentMatrix | (uintptr_t)parent) & 0xf) == 0)
		ba_MatMulAligned(parent, system->parentMatrix, system->parentNode->world);
	else
		ba_MatMul(parent, system->parentMatrix, system->parentNode->world);
	if ((((uintptr_t)system | (uintptr_t)local | (uintptr_t)parent) & 0xf) == 0)
		ba_MatMulAligned(system->matrix, parent, local);
	else
		ba_MatMul(system->matrix, parent, local);
}

/* 0x0003c620  BACTOR/extras.cpp (file inferred)
updates a system (NULL: every system) */
void baUpdateExtras(struct BExtraSystem *system, void *arg)
{
	struct BExtraSystem *s;

	if (system == NULL)
	{
		for (s = DAT_002bc328.next; s != &DAT_002bc328; s = s->next)
			baUpdateExtras(s, arg);
		return;
	}
	if ((system->flags & 4) != 0 || system->parentMatrix != NULL)
		bUpdateExtraMatrix(system);
	if (system->update != NULL && system->numItems != 0)
		((BExtraListFn)system->update)(system, system->numItems, system->items, arg);
}

/* 0x0003c690  BACTOR/extras.cpp (file inferred)
draws a system (NULL: every system) */
void baDrawExtras(struct BExtraSystem *system, void *arg)
{
	struct BExtraSystem *s;

	if (system == NULL)
	{
		for (s = DAT_002bc328.next; s != &DAT_002bc328; s = s->next)
			baDrawExtras(s, arg);
		return;
	}
	if (system->draw != NULL && system->numItems != 0)
	{
		ba_bdSetWorldMatrix((system->flags & 2) == 0 ? system->matrix : bactor_identity);
		((BExtraListFn)system->draw)(system, system->numItems, system->items, arg);
	}
}

/* 0x0003c710  BACTOR/extras.cpp (file inferred)
deletes the index-th live item; 0 when the delete callback refuses */
int baDeleteExtraItem(struct BExtraSystem *system, int index)
{
	char *item;

	if (system->deleteItem != NULL &&
		((BExtraItemFn)system->deleteItem)(system, system->items[index]) == 0)
		return 0;
	item = system->items[index];
	system->used[(item - system->itemData) / system->itemSize] = 0;
	if (index != system->numItems - 1)
		memmove(&system->items[index], &system->items[index + 1], (system->numItems - index) * 4 - 4);
	system->numItems--;
	return 1;
}

/* 0x0003c780  BACTOR/extras.cpp
creates an extra system with room for maxItems items of itemSize bytes */
struct BExtraSystem *baCreateExtraSystem(const char *name, BExtraCallback initItem, BExtraCallback update,
	BExtraCallback deleteItem, BExtraCallback draw, BExtraCallback systemCallback, int itemSize, int maxItems,
	uint32_t flags, int userDataSize)
{
	struct BExtraSystem *system;
	unsigned int size = (unsigned int)(itemSize + 0xf) & ~0xfu;
	unsigned int userSize = (unsigned int)(userDataSize + 0xf) & ~0xfu;
	unsigned int count = (unsigned int)(maxItems + 1) & ~1u;
	char *p;

	system = ba_bkAllocNamedFill((size + 8) * count + sizeof(struct BExtraSystem) + userSize, 0,
		"C:\\Babel\\Xbox\\Src\\BACTOR\\extras.cpp", 0x4f, 0x2001, "Extra System");
	if (system == NULL)
		return NULL;
	system->userData = system + 1;
	p = (char *)(system + 1) + userSize;
	system->used = (int *)p;
	p += count * 4;
	system->itemData = p + count * 4;
	system->items = (void **)p;
	system->prev = DAT_002bc328.prev;
	system->next = &DAT_002bc328;
	DAT_002bc328.prev->next = system;
	DAT_002bc328.prev = system;
	system->nameHash = ba_bkHash(name);
	system->itemSize = (int)size;
	system->initItem = initItem;
	system->deleteItem = deleteItem;
	system->flags = flags;
	system->draw = draw;
	system->maxItems = (int)count;
	system->numItems = 0;
	system->nextFree = 0;
	system->update = update;
	system->userDataSize = (int)userSize;
	system->systemCallback = systemCallback;
	system->maxUsed = 0;
	system->pos[2] = 0.0f;
	system->pos[1] = 0.0f;
	system->pos[0] = 0.0f;
	system->pos[3] = 1.0f;
	system->rot[2] = 0.0f;
	system->rot[1] = 0.0f;
	system->rot[0] = 0.0f;
	system->rot[3] = 1.0f;
	system->parentMatrix = NULL;
	system->parentNode = NULL;
	if (systemCallback != NULL)
		((BExtraSystemFn)systemCallback)(system, 0);
	bUpdateExtraMatrix(system);
	return system;
}

/* 0x0003c8b0  BACTOR/extras.cpp (file inferred)
deletes a system (NULL: every system) */
void baDeleteExtraSystem(struct BExtraSystem *system)
{
	if (system == NULL)
	{
		while (DAT_002bc328.next != &DAT_002bc328)
			baDeleteExtraSystem(DAT_002bc328.next);
		return;
	}
	baClearExtraSystem(system);
	if (system->systemCallback != NULL)
		((BExtraSystemFn)system->systemCallback)(system, 1);
	system->next->prev = system->prev;
	system->prev->next = system->next;
	ba_bkFree(system);
}

/* 0x0003c920  BACTOR/extras.cpp (file inferred)
adds an item (when full and the system recycles, the first deletable item
goes); the variadic arguments go to the creation callback. Returns the item,
or NULL. */
void *baAddExtraItem(struct BExtraSystem *system, ...)
{
	va_list ap;
	int i;

	if (system->numItems == system->maxItems)
	{
		if (system->maxItems == 0 || (system->flags & 1) == 0)
			return NULL;
		for (i = 0; i < system->numItems; i++)
		{
			if (baDeleteExtraItem(system, i) != 0)
				goto add;
		}
		return NULL;
	}
add:
	while (system->used[system->nextFree] != 0)
		system->nextFree = (system->nextFree + 1) % system->maxItems;
	system->items[system->numItems] = system->itemData + system->itemSize * system->nextFree;
	system->used[system->nextFree] = 1;
	system->numItems++;
	if (system->maxUsed < system->numItems)
		system->maxUsed = system->numItems;
	if (system->initItem != NULL)
	{
		va_start(ap, system);
		((BExtraInitFn)system->initItem)(system, system->items[system->numItems - 1], (void *)ap);
		va_end(ap);
	}
	system->nextFree = (system->nextFree + 1) % system->maxItems;
	return system->items[system->numItems - 1];
}

/* 0x0003ca10  BACTOR/extras.cpp (file inferred) */
void baDeleteAllExtras(void)
{
	while (DAT_002bc328.next != &DAT_002bc328)
		baDeleteExtraSystem(DAT_002bc328.next);
}
