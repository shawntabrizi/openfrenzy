/*
XKERNEL_TEST.C

openfrenzy (xkernel module). Guest-image tests of the kernel/XAPI/C library
replacement, run by src/xkernel/tests/run_tests.sh. Each check states why the
game depends on the behaviour. Exit code 0 when every check passed, else the
number of failed checks.
*/

#include "platform.h"
#include "xkernel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
static int checks;

#define CHECK(condition, why) check((condition) != 0, #condition, why, __LINE__)

static void check(int ok, const char *text, const char *why, int line)
{
	checks++;
	if (!ok)
	{
		failures++;
		printf("xkernel_test: FAIL line %d: %s (%s)\n", line, text, why);
	}
}

/* ---------- C library */

static DWORD WINAPI rand_thread(LPVOID parameter)
{
	*(int *)parameter = rand();
	return 0;
}

static void test_rand(void)
{
	/* the game's random sequences (AI, board events) come from MSVC's rand */
	static const int expected[] = { 41, 18467, 6334, 26500, 19169, 15724, 11478, 29358 };
	int index, first_in_thread = -1;
	HANDLE thread;

	srand(1);
	for (index = 0; index < (int)(sizeof(expected) / sizeof(expected[0])); index++)
		CHECK(rand() == expected[index], "MSVC rand sequence from seed 1");
	for (index = 0; index < 10000; index++)
	{
		int value = rand();

		if (value < 0 || value > 0x7fff)
		{
			CHECK(0, "rand stays in 0..0x7fff (code scales it by 1/32768)");
			break;
		}
	}
	/* the Xbox C runtime keeps one seed per thread, starting at 1 */
	thread = CreateThread(NULL, 0, rand_thread, &first_in_thread, 0, NULL);
	CHECK(thread != NULL, "CreateThread");
	CHECK(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0, "thread finishes");
	CloseHandle(thread);
	CHECK(first_in_thread == 41, "a new thread's rand starts from seed 1");
}

struct tagged
{
	int key;
	char tag;
};

static int compare_tagged(const void *a, const void *b)
{
	return ((const struct tagged *)a)->key - ((const struct tagged *)b)->key;
}

static int compare_int(const void *a, const void *b)
{
	int x = *(const int *)a, y = *(const int *)b;

	return x < y ? -1 : x > y;
}

static void test_qsort(void)
{
	struct tagged short_run[3] = { { 1, 'a' }, { 1, 'b' }, { 1, 'c' } };
	static int values[2000];
	int index, sorted = 1;

	/* equal elements end in MSVC's order (its selection sort for <= 8
	elements gives b, c, a here), so ties (equal scores) show as on the Xbox */
	qsort(short_run, 3, sizeof(short_run[0]), compare_tagged);
	CHECK(short_run[0].tag == 'b' && short_run[1].tag == 'c' && short_run[2].tag == 'a',
		"MSVC qsort order of equal elements");

	srand(7);
	for (index = 0; index < 2000; index++)
		values[index] = rand() % 50;
	qsort(values, 2000, sizeof(values[0]), compare_int);
	for (index = 1; index < 2000; index++)
		sorted &= values[index - 1] <= values[index];
	CHECK(sorted, "qsort sorts a large array with many duplicates");
}

static void test_strings(void)
{
	char buffer[64];

	CHECK(lstrlenA(NULL) == 0, "lstrlenA(NULL) is 0 as in Win32");
	CHECK(lstrlenA("abc") == 3, "lstrlenA");
	CHECK(lstrcpyA(buffer, "hello") == buffer && !strcmp(buffer, "hello"), "lstrcpyA");
	memset(buffer, 'x', sizeof(buffer));
	lstrcpynA(buffer, "abcdef", 4);
	CHECK(!strcmp(buffer, "abc"), "lstrcpynA copies count - 1 characters and terminates");
	lstrcpynA(buffer, "ab", 10);
	CHECK(!strcmp(buffer, "ab"), "lstrcpynA of a shorter string");
	CHECK(wsprintfA(buffer, "%d-%s-%04x", 42, "x", 0xbeef) == 9 && !strcmp(buffer, "42-x-beef"),
		"wsprintfA formats like sprintf");
}

/* ---------- synchronisation and threads */

static void test_events_and_mutexes(void)
{
	HANDLE event = CreateEventA(NULL, FALSE, FALSE, NULL);
	HANDLE manual = CreateEventA(NULL, TRUE, TRUE, NULL);
	HANDLE mutex = CreateMutexA(NULL, FALSE, NULL);
	DWORD start;

	CHECK(event && manual && mutex, "create event and mutex");
	start = GetTickCount();
	CHECK(WaitForSingleObject(event, 50) == WAIT_TIMEOUT, "unsignaled event times out");
	CHECK(GetTickCount() - start >= 45, "the timeout waited about 50 ms");
	SetEvent(event);
	CHECK(WaitForSingleObject(event, 0) == WAIT_OBJECT_0, "signaled auto-reset event");
	CHECK(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "auto-reset event resets after one wait");
	CHECK(WaitForSingleObject(manual, 0) == WAIT_OBJECT_0 && WaitForSingleObject(manual, 0) == WAIT_OBJECT_0,
		"manual-reset event stays signaled");
	ResetEvent(manual);
	CHECK(WaitForSingleObject(manual, 0) == WAIT_TIMEOUT, "ResetEvent");
	CHECK(WaitForSingleObject(mutex, 0) == WAIT_OBJECT_0 && WaitForSingleObject(mutex, 0) == WAIT_OBJECT_0,
		"a mutex is recursive for its owner");
	CHECK(ReleaseMutex(mutex) && ReleaseMutex(mutex), "release both levels");
	CHECK(!ReleaseMutex(mutex), "release of an unowned mutex fails");
	CloseHandle(event);
	CloseHandle(manual);
	CloseHandle(mutex);
}

static volatile LONG apc_value;
static volatile LONG apc_runs;

static VOID NTAPI record_apc(ULONG_PTR data)
{
	apc_value = (LONG)data;
	InterlockedIncrement((LPLONG)&apc_runs);
}

struct apc_worker
{
	HANDLE ready;
	HANDLE never;
	DWORD sleep_result;
	DWORD wait_result;
	LONG value_after_sleep;
};

/* the game's loader thread: sleeps alertably until the main thread queues
work to it with QueueUserAPC (bkernel 0x0004ce60) */
static DWORD WINAPI apc_worker_main(LPVOID parameter)
{
	struct apc_worker *worker = parameter;

	SetEvent(worker->ready);
	worker->sleep_result = SleepEx(INFINITE, TRUE);
	worker->value_after_sleep = apc_value;
	SetEvent(worker->ready);
	worker->wait_result = WaitForSingleObjectEx(worker->never, INFINITE, TRUE);
	return 7;
}

static void test_cross_thread_apc(void)
{
	struct apc_worker worker;
	HANDLE thread;
	DWORD exit_code = 0;

	memset(&worker, 0, sizeof(worker));
	worker.ready = CreateEventA(NULL, FALSE, FALSE, NULL);
	worker.never = CreateEventA(NULL, TRUE, FALSE, NULL);
	thread = CreateThread(NULL, 0, apc_worker_main, &worker, 0, NULL);
	CHECK(thread != NULL, "CreateThread");
	CHECK(WaitForSingleObject(worker.ready, 5000) == WAIT_OBJECT_0, "worker started");
	Sleep(20);
	CHECK(QueueUserAPC(record_apc, thread, 1234), "QueueUserAPC to another thread");
	CHECK(WaitForSingleObject(worker.ready, 5000) == WAIT_OBJECT_0, "the APC woke SleepEx(INFINITE, TRUE)");
	CHECK(worker.sleep_result == WAIT_IO_COMPLETION, "SleepEx returns WAIT_IO_COMPLETION after an APC");
	CHECK(worker.value_after_sleep == 1234, "the APC ran on the worker with its data");
	Sleep(20);
	CHECK(QueueUserAPC(record_apc, thread, 5678), "second QueueUserAPC");
	CHECK(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0,
		"the APC woke an alertable WaitForSingleObjectEx and the thread ended");
	CHECK(worker.wait_result == WAIT_IO_COMPLETION, "alertable wait returns WAIT_IO_COMPLETION");
	CHECK(apc_value == 5678 && apc_runs == 2, "both APCs ran once");
	CHECK(GetExitCodeThread(thread, &exit_code) && exit_code == 7, "GetExitCodeThread");
	CloseHandle(thread);
	CloseHandle(worker.ready);
	CloseHandle(worker.never);
}

static DWORD WINAPI suspended_main(LPVOID parameter)
{
	(void)parameter;
	/* an APC queued before the thread ran is delivered by its first
	alertable wait, which then does not sleep */
	return SleepEx(10000, TRUE) == WAIT_IO_COMPLETION ? 0 : 1;
}

static void test_apc_before_start(void)
{
	HANDLE thread = CreateThread(NULL, 0, suspended_main, NULL, CREATE_SUSPENDED, NULL);
	DWORD exit_code = 99, start;

	apc_runs = 0;
	CHECK(QueueUserAPC(record_apc, thread, 9), "queue to a suspended thread");
	start = GetTickCount();
	ResumeThread(thread);
	CHECK(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0, "suspended thread ran");
	CHECK(GetTickCount() - start < 5000, "the queued APC ended the sleep early");
	CHECK(GetExitCodeThread(thread, &exit_code) && exit_code == 0 && apc_runs == 1 && apc_value == 9,
		"queued APC delivered on the first alertable wait");
	CloseHandle(thread);
}

static void test_own_thread_apc(void)
{
	apc_runs = 0;
	CHECK(QueueUserAPC(record_apc, GetCurrentThread(), 3), "queue to the current thread");
	CHECK(apc_runs == 0, "an APC never runs before an alertable wait");
	Sleep(1);
	CHECK(apc_runs == 0, "a non-alertable Sleep does not run APCs");
	CHECK(SleepEx(0, TRUE) == WAIT_IO_COMPLETION && apc_runs == 1 && apc_value == 3,
		"SleepEx(0, TRUE) runs the current thread's APCs");
	CHECK(SleepEx(10, TRUE) == 0, "an alertable sleep without APCs times out normally");
}

/* ---------- time */

static void test_time(void)
{
	LARGE_INTEGER kernel_time, counter1, counter2, frequency;
	SYSTEMTIME system_time, local_time;
	FILETIME file_time;
	unsigned long long from_system, from_kernel;
	DWORD start = GetTickCount();

	Sleep(30);
	CHECK(GetTickCount() - start >= 25, "Sleep(30) sleeps about 30 ms");
	KeQuerySystemTime(&kernel_time);
	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	from_kernel = (unsigned long long)kernel_time.QuadPart;
	from_system = ((unsigned long long)file_time.dwHighDateTime << 32) | file_time.dwLowDateTime;
	CHECK(from_system + 20000000ULL > from_kernel && from_kernel + 20000000ULL > from_system,
		"KeQuerySystemTime agrees with GetSystemTime (100 ns since 1601)");
	CHECK(system_time.wYear >= 2024, "GetSystemTime is the real date");
	GetLocalTime(&local_time);
	CHECK(local_time.wYear >= 2024 && local_time.wMonth >= 1 && local_time.wMonth <= 12 &&
		local_time.wDay >= 1 && local_time.wDay <= 31 && local_time.wDayOfWeek <= 6,
		"GetLocalTime fills a valid date");
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&counter1);
	Sleep(10);
	QueryPerformanceCounter(&counter2);
	CHECK(frequency.QuadPart > 0 && counter2.QuadPart - counter1.QuadPart >= frequency.QuadPart / 200,
		"the performance counter advances at its frequency");
}

/* ---------- memory */

static void test_memory(void)
{
	MEMORYSTATUS status;
	unsigned char *block, *block2;
	unsigned long index, nonzero = 0;

	/* core_init sizes its heaps from these: they must look like an Xbox */
	GlobalMemoryStatus(&status);
	CHECK(status.dwTotalPhys == 64UL * 1024 * 1024 && status.dwAvailPhys <= status.dwTotalPhys,
		"GlobalMemoryStatus reports a 64 MB Xbox");

	/* 0x0004bc10: XPhysicalAlloc(size, MAXULONG_PTR, 0, PAGE_READWRITE) */
	block = XPhysicalAlloc(1024 * 1024, 0xffffffffUL, 0, PAGE_READWRITE);
	CHECK(block != NULL, "XPhysicalAlloc of 1 MB");
	if (block)
	{
		CHECK((unsigned long)block >= 0x80000000UL && (unsigned long)block < 0x88000000UL,
			"contiguous memory is in the Xbox window 0x80000000-0x88000000");
		for (index = 0; index < 1024 * 1024; index++)
			nonzero |= block[index];
		CHECK(nonzero == 0, "new contiguous memory is zeroed");
		memset(block, 0xab, 1024 * 1024);
		XPhysicalProtect(block, 1024 * 1024, PAGE_READWRITE);
		XPhysicalFree(block);
	}
	block2 = MmAllocateContiguousMemory(0x10000);
	CHECK(block2 != NULL && platform_is_contiguous(block2), "MmAllocateContiguousMemory");
	if (block2)
	{
		block2[0x10000 - 1] = 1;
		MmFreeContiguousMemory(block2);
	}
	/* the game probes 0x3100000 bytes (49 MB) and shrinks until it fits */
	block = XPhysicalAlloc(0x3100000, 0xffffffffUL, 0, PAGE_READWRITE);
	CHECK(block != NULL, "XPhysicalAlloc of the game's 49 MB main heap");
	if (block)
		XPhysicalFree(block);
}

/* ---------- XAPI */

static void test_xapi(void)
{
	XSOUNDTRACK_DATA soundtrack;

	CHECK(XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_M, "NTSC console (USA disc)");
	CHECK(XGetGameRegion() == XC_GAME_REGION_NA, "North American region");
	CHECK(XGetVideoFlags() == 0, "4:3 480i video flags");
	CHECK(XGetLanguage() >= XC_LANGUAGE_ENGLISH, "a valid language");
	/* the expiry check 0x0004f930 must find no "sig" section and go on */
	SetLastError(0);
	CHECK(XLoadSection("sig") == NULL && GetLastError() == ERROR_NOT_FOUND, "XLoadSection(\"sig\") fails");
	CHECK(!XFreeSection("sig"), "XFreeSection fails");
	CHECK(XFindFirstSoundtrack(&soundtrack) == INVALID_HANDLE_VALUE &&
		GetLastError() == ERROR_NO_MORE_FILES, "no custom soundtracks");
	CHECK(XOpenSoundtrackSong(1, FALSE) == INVALID_HANDLE_VALUE, "no soundtrack songs");
	DbgBreakPoint(); /* must return */
}

/* ---------- the disc, through the vfs module and this module's APCs */

static volatile DWORD completion_bytes;
static volatile DWORD completion_error = 0xffffffff;

static VOID WINAPI read_completion(DWORD error, DWORD bytes, LPOVERLAPPED overlapped)
{
	(void)overlapped;
	completion_error = error;
	completion_bytes = bytes;
}

static void test_disc(void)
{
	char magic[5] = { 0 };
	char sector[2048];
	DWORD read = 0;
	OVERLAPPED overlapped;
	HANDLE file = CreateFileA("D:\\default.xbe", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);

	CHECK(file != INVALID_HANDLE_VALUE, "open D:\\default.xbe on the disc image (the vfs module's D: drive)");
	if (file == INVALID_HANDLE_VALUE)
		return;
	CHECK(ReadFile(file, magic, 4, &read, NULL) && read == 4 && !memcmp(magic, "XBEH", 4),
		"the disc's default.xbe starts with XBEH");
	CHECK(GetFileSize(file, NULL) == 2834432, "default.xbe size from the disc");
	CloseHandle(file);

	/* the loader thread's pattern: ReadFileEx, then an alertable wait runs
	the completion routine */
	file = CreateFileA("d:\\Packages\\BootSeq.xbp", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, NULL);
	CHECK(file != INVALID_HANDLE_VALUE, "open a game package on the disc");
	if (file == INVALID_HANDLE_VALUE)
		return;
	memset(&overlapped, 0, sizeof(overlapped));
	CHECK(ReadFileEx(file, sector, sizeof(sector), &overlapped, read_completion), "ReadFileEx");
	CHECK(SleepEx(5000, TRUE) == WAIT_IO_COMPLETION, "the read completion arrives as an APC");
	CHECK(completion_error == 0 && completion_bytes == sizeof(sector), "2048 bytes read by ReadFileEx");
	CloseHandle(file);
}

int main(void)
{
	const char *iso = getenv("OPENFRENZY_ISO");

	test_rand();
	test_qsort();
	test_strings();
	test_events_and_mutexes();
	test_cross_thread_apc();
	test_apc_before_start();
	test_own_thread_apc();
	test_time();
	test_memory();
	test_xapi();
	if (iso && *iso)
		test_disc();
	else
		printf("xkernel_test: SKIPPED the disc checks (OPENFRENZY_ISO is not set)\n");
	printf("xkernel_test: %d checks, %d failed\n", checks, failures);
	return failures;
}
