/*
bKernel heap (bkHeap): a first-fit allocator over one big block of memory, with a malloc
fallback before the heap exists, allocation contexts and a leak report.

The original source file is not known (gap functions between xbDebug.cpp and
xbBkgload.cpp in C:\Babel\Xbox\Src\bKernel).
*/

#include "bkernel_core.h"

#include <stdlib.h>
#include <string.h>

/* ---------- globals */

/* 0x00285338: when not -1, every allocation returns this fixed block (a debug aid) */
int bkHeapFixedBlockIndex = -1;
/* 0x00285340: names of the allocation groups (low byte of the flags), 13 bytes each */
const char bkHeapGroupNames[13][13] = {
	"External", "User", "bActor", "bDisplay", "bGUI", "bInput", "bKernel", "bMaths",
	"bSound", "fCamera", "fGUI", "fScene", "fEffects",
};

/* 0x002cce88: the first block of the heap (lowest address) */
bkHeapBlock *bkHeapFirstBlock;
/* 0x002cce8c: the heap memory came from XPhysicalAlloc (freed at shutdown) */
int bkHeapDynamic;
/* 0x002cce90: the heap base, 16 byte aligned */
char *bkHeapBase;
/* 0x002cce94: the heap memory as allocated */
char *bkHeapMemory;
/* 0x002cce98: sentinel of the free list (sorted by address) */
bkHeapBlock bkHeapFreeList;
/* 0x002ccec8: bytes in use */
int bkHeapUsed;
/* 0x002ccecc: sentinel of the used list */
bkHeapBlock bkHeapUsedList;
/* 0x002ccf00, 0x002cd300: two fixed 1 KB blocks (bkHeapFixedBlockIndex) */
char bkHeapFixedBlocks[2][0x400];
/* 0x002cd700 */
int bkHeapPeak;
/* 0x002cd708: the allocation context stack, 0x002cd788: its top */
const char *bkHeapContextStack[32];
int bkHeapContextTop;
/* 0x002cd78c: heap size (0: no heap yet, allocations use malloc) */
uint bkHeapSize;
/* 0x002cd794 */
HANDLE bkHeapMutex;

/* ---------- functions */

/* 0x0004bc10 (static; the memory came in EAX)
Reports the free physical memory before and after; allocates the heap with XPhysicalAlloc
when memory is 0. */
void *bkAllocHeapMemory(void *memory, uint size)
{
	MEMORYSTATUS status;

	GlobalMemoryStatus(&status);
	bkPrintf("Free Physical RAM before heap allocated : %d Kb of %d Kb\n",
		status.dwAvailPhys >> 10, status.dwTotalPhys >> 10);
	if (memory == NULL)
		memory = XPhysicalAlloc(size, (ULONG_PTR)-1, 0, PAGE_READWRITE);
	GlobalMemoryStatus(&status);
	bkPrintf("Free Physical RAM after heap allocated : %d Kb of %d Kb\n",
		status.dwAvailPhys >> 10, status.dwTotalPhys >> 10);
	return memory;
}

/* 0x0004bc80
bkShutdownHeap: prints the blocks still allocated (or that the heap is clean) and frees
dynamic heap memory. */
void bkShutdownHeap(void)
{
	bkHeapBlock *block;
	const char *context;
	char kind[8];
	char contents[32];
	char unused[172];

	if (bkHeapUsedList.next == &bkHeapUsedList)
	{
		bkPrintf("Heap is clean (usage peaked at %d Kb).\n", bkHeapPeak >> 10);
	}
	else
	{
		bkPrintf("\nMemory leaks detected!\n");
		for (block = bkHeapUsedList.next; block != &bkHeapUsedList; block = block->next)
		{
			context = block->context;
			if (context == BK_HEAP_CONTEXT_DEFAULT)
				context = "Default";
			unused[0] = '\0';
			/* the original leaves the kind uninitialised for plain allocations */
			kind[0] = '\0';
			if (block->flags & BK_HEAP_FLAG_MALLOC)
				memcpy(kind, "Malloc", 7);
			else if (block->flags & BK_HEAP_FLAG_NEW)
				memcpy(kind, "New", 4);
			bkMakePrintable((const byte *)(block + 1), (int)block->size - 0x30,
				(char *)contents, sizeof(contents));
			bkPrintf("%s 0x%08x, %8d bytes [%s %s in %s] '%s'\n", unused, (uint)(block + 1),
				(int)block->size - 0x30, context, kind, bkHeapGroupNames[(byte)block->flags],
				contents);
		}
		bkPrintf("Heap usage peaked at %d Kb.\n", bkHeapPeak >> 10);
	}
	if (bkHeapDynamic)
		MmFreeContiguousMemory(bkHeapMemory);
}

/* 0x0004bdc0
bkResetHeap: makes the whole heap one free block. */
void bkResetHeap(void)
{
	bkHeapBlock *first;

	WaitForSingleObject(bkHeapMutex, INFINITE);
	first = (bkHeapBlock *)bkHeapBase;
	bkHeapFirstBlock = first;
	first->offset = 0;
	first->size = bkHeapSize;
	first->used = 0;
	first->flags = 0;
	first->prevPhys = first;
	first->nextPhys = first;
	first->prev = &bkHeapFreeList;
	first->next = &bkHeapFreeList;
	bkHeapFreeList.prev = first;
	bkHeapFreeList.next = first;
	bkHeapUsedList.prev = &bkHeapUsedList;
	bkHeapUsedList.next = &bkHeapUsedList;
	bkHeapUsed = 0;
	bkHeapPeak = 0;
	bkHeapContextTop = 0;
	bkHeapContextStack[0] = BK_HEAP_CONTEXT_DEFAULT;
	ReleaseMutex(bkHeapMutex);
}

/* 0x0004be40 (static; the block came in EAX)
Inserts a block into the free list, which is sorted by address. */
void bkHeapInsertFree(bkHeapBlock *block)
{
	bkHeapBlock *node;
	bkHeapBlock *head = bkHeapFreeList.next;

	if (head == &bkHeapFreeList)
	{
		bkHeapFreeList.next = block;
		bkHeapFreeList.prev = block;
		block->prev = &bkHeapFreeList;
		block->next = &bkHeapFreeList;
		return;
	}
	for (node = head; node != &bkHeapFreeList; node = node->next)
	{
		if (node->offset < block->offset &&
			(node->next == &bkHeapFreeList || node->next->offset > block->offset))
		{
			block->prev = node;
			block->next = node->next;
			block->next->prev = block;
			block->prev->next = block;
			return;
		}
	}
	/* lower than every free block: insert at the front */
	block->next = head;
	block->prev = &bkHeapFreeList;
	head->prev = block;
	block->prev->next = block;
}

/* 0x0004bec0 (static; the size came in EAX)
Allocation before the heap exists: malloc with a block header (nextPhys 0 marks it). */
void *bkMallocFallback(uint size, const char *file, int line, ushort flags,
	const char *context)
{
	uint total = ((size + 0xf) & ~0xfu) + 0x30;
	bkHeapBlock *block = (bkHeapBlock *)malloc(total);

	(void)file;
	(void)line;
	if (block == NULL)
		return NULL;
	block->size = total;
	block->used = 1;
	block->context = context;
	block->flags = flags;
	block->nextPhys = NULL;
	block->magic = BK_HEAP_MAGIC;
	return block + 1;
}

/* 0x0004bf10
bkHeapAllocEx: allocates size bytes from the Babel heap, aligned to align when it is not 0.
file and line are only reported when the heap is full. */
void *bkHeapAllocEx(uint size, const char *file, int line, uint flags, const char *context,
	uint align)
{
	uint payload;
	uint total;
	bkHeapBlock *block;
	bkHeapBlock *first;
	bkHeapBlock *rest;
	bkHeapBlock *after;
	char *aligned;

	if (bkHeapFixedBlockIndex != -1)
	{
		bkPrintf("bkHeapAllocEx: returning fixed block %d\n", bkHeapFixedBlockIndex);
		return bkHeapFixedBlocks[bkHeapFixedBlockIndex];
	}
	if (align != 0)
		size += align;
	if (bkHeapSize == 0)
		return bkMallocFallback(size, file, line, (ushort)flags, BK_HEAP_CONTEXT_DEFAULT);

	payload = (size + 0xf) & ~0xfu;
	total = payload + 0x30;
	WaitForSingleObject(bkHeapMutex, INFINITE);

	for (block = bkHeapFreeList.next; block != &bkHeapFreeList; block = block->next)
	{
		if (block->size >= total)
			break;
	}
	if (block == &bkHeapFreeList)
	{
		ReleaseMutex(bkHeapMutex);
		bkPrintf("%s(%d) : bkHeapAllocDebug: Out of memory (wanted %d bytes)\n", file, line,
			payload);
		return NULL;
	}

	bkHeapUsed += total;
	if (bkHeapUsed > bkHeapPeak)
		bkHeapPeak = bkHeapUsed;
	first = bkHeapFirstBlock;

	if (block->size > total + 0x30)
	{
		/* split: the rest becomes a new free block */
		rest = (bkHeapBlock *)(bkHeapBase + block->offset + total);
		rest->offset = block->offset + total;
		rest->size = block->size - total;
		rest->used = 0;
		rest->flags = 0;
		rest->nextPhys = block->nextPhys;
		rest->prevPhys = block;
		rest->nextPhys->prevPhys = rest;
		rest->prevPhys->nextPhys = rest;
		rest->context = context;
		block->size = total;
		bkHeapInsertFree(rest);

		after = rest->nextPhys;
		if (after != first && after->used == 0)
		{
			/* merge with the free block after it */
			rest->next = after->next;
			after->next->prev = rest;
			rest->size += rest->nextPhys->size;
			rest->nextPhys = rest->nextPhys->nextPhys;
			rest->nextPhys->prevPhys = rest;
		}
	}

	/* off the free list, onto the used list */
	block->next->prev = block->prev;
	block->prev->next = block->next;
	block->used = 1;
	block->flags = (ushort)flags;
	block->context = context;
	if (block == first)
	{
		block->next = bkHeapUsedList.next;
		block->prev = &bkHeapUsedList;
	}
	else
	{
		block->next = block->prevPhys->next;
		block->prev = block->prevPhys;
	}
	block->next->prev = block;
	block->prev->next = block;
	block->magic = BK_HEAP_MAGIC;
	ReleaseMutex(bkHeapMutex);

	if (align != 0)
	{
		aligned = (char *)block + 0x31;
		while (((uint)aligned & (align - 1)) != 0)
			aligned++;
		((bkHeapBlock **)aligned)[-1] = block;
		return aligned;
	}
	return block + 1;
}

/* 0x0004c110
bkHeapFree: frees a block of the heap (or of the malloc fallback); merges free neighbours. */
void bkHeapFree(void *pointer)
{
	bkHeapBlock *block;
	bkHeapBlock *neighbour;
	bkHeapBlock *first;

	if (pointer == NULL || pointer == bkHeapFixedBlocks[0] || pointer == bkHeapFixedBlocks[1])
		return;
	WaitForSingleObject(bkHeapMutex, INFINITE);
	block = ((bkHeapBlock **)pointer)[-1];
	if ((uint)block == BK_HEAP_MAGIC)
		block = (bkHeapBlock *)pointer - 1;
	if (block->nextPhys == NULL)
	{
		ReleaseMutex(bkHeapMutex);
		free(block);
		return;
	}

	bkHeapUsed -= block->size;
	block->next->prev = block->prev;
	block->prev->next = block->next;
	block->used = 0;
	bkHeapInsertFree(block);

	first = bkHeapFirstBlock;
	if (block != first)
	{
		neighbour = block->prevPhys;
		if (neighbour->used == 0)
		{
			neighbour->size += block->size;
			neighbour->nextPhys = block->nextPhys;
			block->nextPhys->prevPhys = neighbour;
			neighbour->next = block->next;
			block->next->prev = neighbour;
			block = neighbour;
		}
	}
	neighbour = block->nextPhys;
	if (neighbour != first && neighbour->used == 0)
	{
		block->size += neighbour->size;
		block->nextPhys = neighbour->nextPhys;
		neighbour->nextPhys->prevPhys = block;
		block->next = neighbour->next;
		neighbour->next->prev = block;
	}
	ReleaseMutex(bkHeapMutex);
}

/* 0x0004c210
bkHeapBlockSize: the usable size of a heap block. */
int bkHeapBlockSize(void *pointer)
{
	bkHeapBlock *block = ((bkHeapBlock **)pointer)[-1];

	if ((uint)block == BK_HEAP_MAGIC)
		block = (bkHeapBlock *)pointer - 1;
	return (int)block->size - 0x30;
}

/* 0x0004c230
bkHeapFreeSpace: bytes free in the heap; *largest receives the largest free block. */
int bkHeapFreeSpace(int *largest)
{
	bkHeapBlock *block;
	int size = 0;

	if (largest != NULL)
	{
		for (block = bkHeapFreeList.next; block != &bkHeapFreeList; block = block->next)
		{
			if (size < (int)block->size - 0x30)
				size = (int)block->size - 0x30;
		}
		*largest = size;
	}
	return (int)bkHeapSize - bkHeapUsed - 0x30;
}

/* 0x0004c280 */
const char *bkGetHeapContext(void)
{
	return bkHeapContextStack[bkHeapContextTop];
}

/* 0x0004c290 */
int bkPushHeapContext(const char *context)
{
	if (bkHeapContextTop == 0x1f)
		return 0;
	bkHeapContextTop++;
	bkHeapContextStack[bkHeapContextTop] = context;
	return 1;
}

/* 0x0004c2c0 */
int bkPopHeapContext(void)
{
	if (bkHeapContextTop == 0)
		return 0;
	bkHeapContextTop--;
	return 1;
}

/* 0x0004c2e0
bkGetFreePhysicalMemory: available physical memory in bytes. */
uint bkGetFreePhysicalMemory(void)
{
	MEMORYSTATUS status;

	GlobalMemoryStatus(&status);
	return status.dwAvailPhys;
}

/* 0x0004c300
bkHeapDump: prints the heap's blocks in address order (runs of used blocks merged). */
void bkHeapDump(void)
{
	bkHeapBlock *block = bkHeapFirstBlock;
	bkHeapBlock *run;
	uint size;

	do
	{
		size = block->size;
		if (block->used == 0)
		{
			bkPrintf("0x%08x, %6d Kb (%10d bytes)      FREE\n", (uint)(block + 1), size >> 10, size);
		}
		else
		{
			for (run = block->nextPhys; run != bkHeapFirstBlock && run->used != 0;
				run = run->nextPhys)
			{
				size += run->size;
			}
			bkPrintf("0x%08x, %6d Kb (%10d bytes) USED\n", (uint)(block + 1), (int)size >> 10,
				size);
			block = run->prevPhys;
		}
		block = block->nextPhys;
	} while (block != bkHeapFirstBlock);
}

/* 0x0004c380
bInitHeap: makes a heap of size bytes in memory (or in new physical memory when memory is
0). */
int bInitHeap(void *memory, uint size)
{
	bkHeapSize = (size + 0xf) & ~0xfu;
	bkHeapMemory = (char *)bkAllocHeapMemory(memory, bkHeapSize);
	if (bkHeapMemory == NULL)
		return 0;
	bkHeapDynamic = (memory == NULL);
	bkPrintf("bInitHeap: Allocated %d Kb %s heap at 0x%08x\n", size >> 10,
		bkHeapDynamic ? "DYNAMIC" : "STATIC", (uint)bkHeapMemory);
	bkHeapBase = (char *)(((uint)bkHeapMemory + 0xf) & ~0xfu);
	bkHeapFreeList.used = 0;
	bkHeapUsedList.used = 1;
	bkHeapMutex = CreateMutexA(NULL, FALSE, "heapMutex");
	bkResetHeap();
	return 1;
}

/* 0x0004c420
bkHeapCallocEx: bkHeapAllocEx without alignment, filled with fill. */
void *bkHeapCallocEx(uint size, byte fill, const char *file, int line, uint flags,
	const char *context)
{
	void *pointer = bkHeapAllocEx(size, file, line, flags, context, 0);

	if (pointer != NULL)
		memset(pointer, fill, size);
	return pointer;
}

/* 0x0004c480
bkHeapAlloc: the game's operator new and malloc (current context, no alignment). */
void *bkHeapAlloc(uint size, const char *file, int line, uint flags)
{
	return bkHeapAllocEx(size, file, line, flags, bkHeapContextStack[bkHeapContextTop], 0);
}

/* 0x0004c4b0
bkHeapCalloc: bkHeapAlloc filled with fill. */
void *bkHeapCalloc(uint size, byte fill, const char *file, int line, uint flags)
{
	void *pointer = bkHeapAllocEx(size, file, line, flags, bkHeapContextStack[bkHeapContextTop], 0);

	if (pointer != NULL)
		memset(pointer, fill, size);
	return pointer;
}
