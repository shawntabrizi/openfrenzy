/*
bkernel module tests: one guest image (src/bkernel/tests/run_tests.sh builds it from the
game build's objects of src/bkernel and the generated weak stubs).

The game image globs every .c under src/bkernel, so this file compiles to nothing unless
OPENFRENZY_BKERNEL_TEST is defined.

Each test checks what the game relies on, not just what the code happens to do:
- heap: freed neighbours merge so memory is reused at the same place (first fit by
  address); package buffers come back 128 byte aligned; everything freed restores the
  free space (no leak in the allocator's own bookkeeping).
- crc: names hash exactly like the packer that built the game's packages (values taken
  from Packages/bgeffects.xbp on the disc).
- events: queues keep order (FIFO / LIFO) and the overflow policy decides which message
  is lost; callbacks see the parameters.
- packages (needs the disc): an opened package and the same package loaded whole give
  the same file bytes and names; the background loader loads and patches a package on
  its own thread and raises the event.
*/

#ifdef OPENFRENZY_BKERNEL_TEST

#include "bkernel_core.h"

#include <stdlib.h>
#include <string.h>

/* bdisplay's global that bkernel reads (not linked into this test) */
float DAT_002855f0[16];

static int failures;
static int checks;

#define CHECK(condition, ...) \
	do \
	{ \
		checks++; \
		if (!(condition)) \
		{ \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s: ", __FILE__, __LINE__, #condition); \
			fprintf(stderr, __VA_ARGS__); \
			fprintf(stderr, "\n"); \
		} \
	} while (0)

/* ---------- heap */

__attribute__((noinline)) static void test_heap(void)
{
	int largestStart;
	int freeStart;
	int largest;
	char *a;
	char *b;
	char *c;
	char *d;
	char *aligned;

	CHECK(bInitHeap(NULL, 0x40000) == 1, "bInitHeap failed");
	freeStart = bkHeapFreeSpace(&largestStart);
	CHECK(largestStart == 0x40000 - 0x30, "a new heap is one free block (largest %d)", largestStart);

	a = (char *)bkHeapAlloc(1000, __FILE__, __LINE__, 0x2006);
	b = (char *)bkHeapAlloc(2000, __FILE__, __LINE__, 0x2006);
	c = (char *)bkHeapAlloc(3000, __FILE__, __LINE__, 0x2006);
	CHECK(a != NULL && b != NULL && c != NULL, "allocations failed");
	CHECK(a < b && b < c, "first fit allocates upward (%p %p %p)", (void *)a, (void *)b, (void *)c);
	CHECK(bkHeapBlockSize(a) >= 1000, "block size %d", bkHeapBlockSize(a));
	memset(a, 0x11, 1000);
	memset(b, 0x22, 2000);
	memset(c, 0x33, 3000);

	/* freeing two neighbours leaves one hole that a bigger block fits into */
	bkHeapFree(b);
	bkHeapFree(a);
	d = (char *)bkHeapAlloc(2900, __FILE__, __LINE__, 0x2006);
	CHECK(d == a, "the merged hole is reused at its start (d %p, a %p)", (void *)d, (void *)a);
	CHECK(c[0] == 0x33 && c[2999] == 0x33, "a neighbour's data survives");

	/* package buffers must be 128 byte aligned (ReadFileEx without buffering) */
	aligned = (char *)bkHeapAllocEx(100, __FILE__, __LINE__, 0x2006, "test", 0x80);
	CHECK(aligned != NULL && ((uint)aligned & 0x7f) == 0, "aligned block %p", (void *)aligned);
	CHECK(bkEnsureAlloc(100, aligned) == aligned, "bkEnsureAlloc accepts an aligned buffer");
	CHECK(bkEnsureAlloc(100000, aligned) == NULL, "bkEnsureAlloc refuses a small buffer");

	bkHeapFree(aligned);
	bkHeapFree(d);
	bkHeapFree(c);
	CHECK(bkHeapFreeSpace(&largest) == freeStart, "everything freed: free space %d, was %d",
		bkHeapFreeSpace(NULL), freeStart);
	CHECK(largest == largestStart, "everything freed merges back into one block (%d)", largest);
	CHECK(bkHeapUsedList.next == &bkHeapUsedList, "the used list is empty");

	/* the allocation context stack holds 31 contexts above the default */
	{
		int pushed = 0;

		while (bkPushHeapContext("context"))
			pushed++;
		CHECK(pushed == 31, "pushed %d contexts", pushed);
		while (bkPopHeapContext())
			pushed--;
		CHECK(pushed == 0 && bkGetHeapContext() == BK_HEAP_CONTEXT_DEFAULT, "popped back to default");
	}
}

/* ---------- crc, hex, timer */

__attribute__((noinline)) static void test_crc(void)
{
	char name[32];

	bkInitCRC();
	CHECK(bkCRCString("") == 0, "empty name");
	/* index entries of Packages/bgeffects.xbp */
	CHECK(bkCRCString("bug.wir") == 0x1391e04a, "bug.wir 0x%08x", bkCRCString("bug.wir"));
	CHECK(bkCRCString("wirethings.txt") == 0xaab7f808, "wirethings.txt");
	CHECK(bkCRCStringNoCase("BUG.Wir") == 0x1391e04a, "case-insensitive crc");
	/* a cache file name carries the crc and the group as hex */
	strcpy(name, bkCacheFileName("Packages\\Bug.xbp", 7));
	CHECK(bkParseHex8(name) == bkCRCStringNoCase("packages\\bug.xbp"), "cache name crc %s", name);
	CHECK(bkParseHex8(name + 9) == 7, "cache name group %s", name);
	/* a timestamp that went backward is taken as wrapped */
	CHECK(bkTimerDelta(5, 10) == 5, "forward delta");
	CHECK(bkTimerDelta(10, 5) == 0xffffffffull - 5, "backward delta 0x%llx", bkTimerDelta(10, 5));
	/* the timestamps run at the Xbox's 733 MHz: a backward delta then adds 5.9 s, as on the
	Xbox. At the 1 MHz performance counter it added 71 minutes, and Hopper ended its round
	at once when paused before the start. */
	{
		ulonglong start = bkTimerRead();
		ulonglong ticks;

		bkSleep(20);
		ticks = bkTimerRead() - start;
		CHECK(ticks > 733333333ull / 50 * 9 / 10 && ticks < 733333333ull / 50 * 3,
			"20 ms is 0x%llx ticks", ticks);
	}
}

/* ---------- events */

static int callbackCount;
static char callbackParameters[64];

__attribute__((noinline)) static void test_callback(const char *name, const char *parameters, void *context)
{
	(void)name;
	callbackCount += (int)(uint)context;
	strcpy(callbackParameters, parameters != NULL ? parameters : "(null)");
}

__attribute__((noinline)) static void test_events(void)
{
	bkEventClient *fifo;
	bkEventClient *overwrite;
	bkEventClient *lifo;
	char message[0x100];

	bkInitEvents();
	CHECK(bkCreateEvent("_Test") == 1, "create");
	CHECK(bkCreateEvent("_Test") == 1, "creating twice is harmless");
	fifo = bkAddEventQueue("_Test", 2, 0);
	overwrite = bkAddEventQueue("_Test", 2, BK_EVENT_QUEUE_OVERWRITE);
	lifo = bkAddEventQueue("_Test", 2, BK_EVENT_QUEUE_LIFO);
	CHECK(bkAddEventCallback("_Test", test_callback, (void *)1) != NULL, "callback client");
	CHECK(bkAddEventQueue("_NoSuchEvent", 2, 0) == NULL, "no client for an unknown event");

	CHECK(bkGenerateEvent("_Test", "one") == 1, "generate");
	bkGenerateEvent("_Test", "two");
	bkGenerateEvent("_Test", "three");
	CHECK(bkGenerateEvent("_NoSuchEvent", "x") == 0, "unknown event");
	CHECK(callbackCount == 3 && strcmp(callbackParameters, "three") == 0, "callback saw %d, '%s'",
		callbackCount, callbackParameters);

	/* a full queue without OVERWRITE loses the new message */
	CHECK(bkGetEventMessage(fifo, message) && strcmp(message, "one") == 0, "fifo first '%s'", message);
	CHECK(bkGetEventMessage(fifo, message) && strcmp(message, "two") == 0, "fifo second '%s'", message);
	CHECK(bkGetEventMessage(fifo, message) == 0, "fifo empty");
	/* with OVERWRITE it loses the oldest */
	CHECK(bkGetEventMessage(overwrite, message) && strcmp(message, "two") == 0, "overwrite '%s'", message);
	CHECK(bkGetEventMessage(overwrite, message) && strcmp(message, "three") == 0, "overwrite '%s'", message);
	/* LIFO takes the newest first */
	CHECK(bkGetEventMessage(lifo, message) && strcmp(message, "two") == 0, "lifo '%s'", message);
	CHECK(bkGetEventMessage(lifo, message) && strcmp(message, "one") == 0, "lifo '%s'", message);

	bkDeleteEvent("_Test");
	CHECK(bkGenerateEvent("_Test", "gone") == 0, "deleted event");
}

/* ---------- command line, wide strings */

__attribute__((noinline)) static void test_strings(void)
{
	char *argv[8];
	ushort wide[64];
	ushort expected[64];
	int count;

	count = bkParseCommandLine("game.xbe  -level \"two words\"\t-x", NULL);
	CHECK(count == 4, "argument count %d", count);
	CHECK(bkParseCommandLine("game.xbe  -level \"two words\"\t-x", argv) == 4, "parse");
	CHECK(strcmp(argv[2], "\"two words\"") == 0, "quoted argument kept whole: %s", argv[2]);
	CHECK(strcmp(argv[3], "-x") == 0, "last argument %s", argv[3]);

	bkStringToWide(expected, "P1 -42 007 3.14 abc xyz Z");
	CHECK(bkWideSprintf(wide, L"P%d %d %03d %.2f %s %ls %c", 1, -42, 7, 3.14159, "abc", L"xyz", 'Z') ==
		bkWideStrlen(expected), "length %d", bkWideStrlen(wide));
	CHECK(memcmp(wide, expected, (bkWideStrlen(expected) + 1) * 2) == 0, "wide sprintf");
	/* %f truncates (no rounding) */
	bkWideSprintf(wide, L"%.1f", 2.99);
	bkStringToWide(expected, "2.9");
	CHECK(memcmp(wide, expected, 8) == 0, "truncation");
	CHECK(bkWideSprintf(wide, L"%q", 1) == 0, "an unknown conversion fails");
}

/* ---------- packages (need Packages/bgeffects.xbp under the data root and the disc) */

__attribute__((noinline)) static void test_packages(void)
{
	bkPackage *opened;
	bkPackage *loaded;
	bkPackageEntry *entry;
	char *fromFile;
	char *fromMemory;
	uint size;

	strcpy(bkRootPath, "d:\\");
	/* the disc stream itself: one large read (bkLoadPackageFile's path) */
	{
		static char probeBuffer[8192];
		FILE *probe = vfs_fopen("d:\\Packages\\bgeffects.xbp", "rb");
		size_t got;

		fseek(probe, 128 * 17, SEEK_SET);
		got = fread(probeBuffer, 1, 6960, probe);
		fclose(probe);
		CHECK(got == 6960, "a 6960 byte read from the disc image gave %u", (uint)got);
	}
	opened = bkOpenPackage("Packages\\bgeffects");
	CHECK(opened != NULL, "bkOpenPackage");
	if (opened == NULL)
		return;
	CHECK(opened->numFiles == 12 && opened->blockSize == 0x80, "12 files of 128 byte blocks");
	CHECK((int)opened->id >= 0, "an opened package is not in memory");
	entry = bkFindPackageEntryByName(opened, "bug.wir");
	CHECK(entry != NULL && entry->size == 6960, "bug.wir is 6960 bytes");
	CHECK(bkFindPackageEntryByName(opened, "nothere.wir") == NULL, "a missing file");
	CHECK(bkGetFilename(opened->id, 0x1391e04a) != NULL &&
		strcmp(bkGetFilename(opened->id, 0x1391e04a), "bug.wir") == 0, "the filename table");
	fromFile = (char *)bkLoadPackageFile(opened, bkCRCString("bug.wir"), NULL, &size, NULL, 0);
	CHECK(fromFile != NULL && size == 6960 && ((uint)fromFile & 0x7f) == 0, "read bug.wir");

	loaded = bkLoadPackage(NULL, "Packages\\bgeffects", NULL);
	CHECK(loaded != NULL && (int)loaded->id < 0, "bkLoadPackage");
	if (loaded != NULL && fromFile != NULL)
	{
		fromMemory = (char *)bkLoadPackageFile(loaded, bkCRCString("bug.wir"), NULL, &size, NULL, 0);
		CHECK(fromMemory != NULL && (char *)fromMemory > (char *)loaded, "points into the package");
		CHECK(fromMemory != NULL && memcmp(fromMemory, fromFile, 6960) == 0, "same bytes both ways");
	}
	bkHeapFree(fromFile);
	bkClosePackage(loaded);
	bkClosePackage(opened);
	CHECK(bkFreeFilenameTable(0x4bb84bcf) == -1, "closing both packages frees the filename table");
}

__attribute__((noinline)) static void test_background_load(void)
{
	bkEventClient *client;
	bkPackage *package;
	char message[0x100];
	uint size = 0;
	int waited;
	int done = 0;

	CHECK(bkCreateEvent("_TestLoad") == 1, "event");
	client = bkAddEventQueue("_TestLoad", 4, 0);
	package = (bkPackage *)bkLoadPackageBackground(NULL, "Packages\\bgeffects", "_TestLoad", &size, NULL);
	CHECK(package != NULL && size == 182272, "queued (size %u)", size);
	if (package == NULL)
		return;
	for (waited = 0; waited < 10000 && !done; waited += 10)
	{
		done = bkGetEventMessage(client, message);
		if (!done)
			Sleep(10);
	}
	CHECK(done, "the load finished within 10 s");
	if (!done)
		return;
	CHECK((int)package->id < 0 && package->numFiles == 12, "patched as a loaded package");
	CHECK((char *)package->entries > (char *)package &&
		(char *)package->entries < (char *)package + size, "the index points into the package");
	CHECK(bkFindPackageEntryByName(package, "bug.wir") != NULL, "its index is usable");
	CHECK(bkIsChannelBusy(0) == 0 && bkIsChannelIdle(0), "the DATA channel is free again");
}

int main(void)
{
	int disc = getenv("BKERNEL_TEST_DISC") != NULL;

	test_heap();
	test_crc();
	test_events();
	test_strings();
	if (disc)
	{
		bkInitKernel();
		CHECK(bInitHeap(NULL, 0x800000) == 1, "heap for the disc tests");
		test_packages();
		test_background_load();
	}
	else
	{
		fprintf(stderr, "SKIP package tests (BKERNEL_TEST_DISC not set)\n");
	}
	fprintf(stderr, "bkernel tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
