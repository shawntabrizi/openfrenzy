/*
XBOX_KERNEL.C

Win32/XAPI kernel services for the Linux build: handles, events, mutexes,
threads, asynchronous procedure calls, time, memory and debug output.
*/

#include "platform.h"
#include "xkernel.h"

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define PLATFORM_HANDLE_SIGNATURE 0x686e646cUL /* 'hndl' */

/* ---------- logging */

void platform_log(const char *format, ...)
{
	va_list arguments;

	fputs("openfrenzy: ", stderr);
	va_start(arguments, format);
	vfprintf(stderr, format, arguments);
	va_end(arguments);
	fputc('\n', stderr);
}

void platform_unimplemented(const char *name)
{
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	static const char *reported[512];
	static long reported_count = 0;
	long index;

	pthread_mutex_lock(&lock);
	for (index = 0; index < reported_count; index++)
	{
		if (reported[index] == name)
			break;
	}
	if (index == reported_count && reported_count < (long)(sizeof(reported) / sizeof(reported[0])))
	{
		reported[reported_count++] = name;
		platform_log("%s is not implemented on Linux", name);
	}
	pthread_mutex_unlock(&lock);
}

/* ---------- last error */

static __thread DWORD platform_last_error;

DWORD WINAPI GetLastError(void)
{
	return platform_last_error;
}

void WINAPI SetLastError(DWORD error)
{
	platform_last_error = error;
}

DWORD platform_set_last_error_from_errno(int error_number)
{
	DWORD error;

	switch (error_number)
	{
	case 0: error = ERROR_SUCCESS; break;
	case ENOENT: error = ERROR_FILE_NOT_FOUND; break;
	case ENOTDIR: error = ERROR_PATH_NOT_FOUND; break;
	case EACCES: case EPERM: case EROFS: error = ERROR_ACCESS_DENIED; break;
	case EEXIST: error = ERROR_ALREADY_EXISTS; break;
	case ENOTEMPTY: error = ERROR_DIR_NOT_EMPTY; break;
	case ENOSPC: error = ERROR_DISK_FULL; break;
	case ENOMEM: error = ERROR_NOT_ENOUGH_MEMORY; break;
	case EBADF: error = ERROR_INVALID_HANDLE; break;
	case EINVAL: error = ERROR_INVALID_PARAMETER; break;
	case EMFILE: case ENFILE: error = ERROR_TOO_MANY_OPEN_FILES; break;
	case EBUSY: error = ERROR_BUSY; break;
	default: error = ERROR_GEN_FAILURE; break;
	}
	platform_last_error = error;
	return error;
}

/* ---------- handles */

struct platform_handle *platform_handle_new(long type, void *data,
	void (*destroy)(struct platform_handle *handle))
{
	struct platform_handle *handle = calloc(1, sizeof(*handle));
	pthread_mutexattr_t attributes;

	if (!handle)
		return NULL;
	handle->signature = PLATFORM_HANDLE_SIGNATURE;
	handle->type = type;
	handle->data = data;
	handle->destroy = destroy;
	pthread_mutexattr_init(&attributes);
	pthread_mutex_init(&handle->lock, &attributes);
	pthread_mutexattr_destroy(&attributes);
	pthread_cond_init(&handle->condition, NULL);
	return handle;
}

struct platform_handle *platform_handle_get(HANDLE handle, long type)
{
	struct platform_handle *result = (struct platform_handle *)handle;

	/* GetCurrentProcess() and GetCurrentThread() are the pseudo handles -1
	and -2; any other value in the top page cannot be a heap pointer */
	if (!result || (unsigned long)handle >= 0xfffff000UL ||
		result->signature != PLATFORM_HANDLE_SIGNATURE ||
		(type && result->type != type))
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return NULL;
	}
	return result;
}

void platform_handle_signal(struct platform_handle *handle)
{
	pthread_mutex_lock(&handle->lock);
	handle->signaled = TRUE;
	pthread_cond_broadcast(&handle->condition);
	pthread_mutex_unlock(&handle->lock);
}

BOOL WINAPI CloseHandle(HANDLE object)
{
	struct platform_handle *handle = platform_handle_get(object, 0);

	if (!handle)
		return FALSE;
	if (handle->type == _platform_handle_thread)
	{
		/* the record lives until both the handle is closed and the thread
		has exited; thread_release frees it once both have happened */
		handle->destroy(handle);
		return TRUE;
	}
	if (handle->destroy)
		handle->destroy(handle);
	handle->signature = 0;
	pthread_cond_destroy(&handle->condition);
	pthread_mutex_destroy(&handle->lock);
	free(handle);
	return TRUE;
}

/* ---------- asynchronous procedure calls

openfrenzy: each thread has its own queue, reachable from its thread handle,
so QueueUserAPC can target another thread (the game's loader thread runs the
file-system requests the main thread queues to it). A queue lives as long as
the process: a thread handle may outlive its thread, and an APC queued to an
exited thread then never runs, as on the Xbox. An alertable wait on a handle
registers that handle in its queue, so a new APC also wakes that wait. */

struct platform_apc
{
	struct platform_apc *next;
	platform_apc_routine routine;
	void *context[3];
};

struct platform_apc_queue
{
	pthread_mutex_t lock;
	pthread_cond_t condition;
	struct platform_apc *head;
	struct platform_apc *tail;
	/* the handle an alertable WaitForSingleObjectEx of this thread waits on */
	struct platform_handle *waiting_on;
};

static __thread struct platform_apc_queue *platform_current_apc_queue;

static struct platform_apc_queue *apc_queue_new(void)
{
	struct platform_apc_queue *queue = calloc(1, sizeof(*queue));

	if (!queue)
		return NULL;
	pthread_mutex_init(&queue->lock, NULL);
	pthread_cond_init(&queue->condition, NULL);
	return queue;
}

static struct platform_apc_queue *apc_queue_current(void)
{
	if (!platform_current_apc_queue)
		platform_current_apc_queue = apc_queue_new();
	return platform_current_apc_queue;
}

static BOOL apc_queue_push(struct platform_apc_queue *queue, platform_apc_routine routine,
	void *context0, void *context1, void *context2)
{
	struct platform_apc *apc = calloc(1, sizeof(*apc));
	struct platform_handle *waiting_on;

	if (!apc || !queue)
	{
		free(apc);
		return FALSE;
	}
	apc->routine = routine;
	apc->context[0] = context0;
	apc->context[1] = context1;
	apc->context[2] = context2;
	pthread_mutex_lock(&queue->lock);
	if (queue->tail)
		queue->tail->next = apc;
	else
		queue->head = apc;
	queue->tail = apc;
	waiting_on = queue->waiting_on;
	pthread_cond_broadcast(&queue->condition);
	pthread_mutex_unlock(&queue->lock);
	if (waiting_on)
	{
		/* the waiter holds this lock from its queue check until it sleeps,
		so the broadcast cannot fall between the two */
		pthread_mutex_lock(&waiting_on->lock);
		pthread_cond_broadcast(&waiting_on->condition);
		pthread_mutex_unlock(&waiting_on->lock);
	}
	return TRUE;
}

static BOOL apc_queue_pending(struct platform_apc_queue *queue)
{
	BOOL pending;

	pthread_mutex_lock(&queue->lock);
	pending = queue->head != NULL;
	pthread_mutex_unlock(&queue->lock);
	return pending;
}

void platform_queue_apc(platform_apc_routine routine, void *context0, void *context1, void *context2)
{
	apc_queue_push(apc_queue_current(), routine, context0, context1, context2);
}

long platform_run_apcs(void)
{
	struct platform_apc_queue *queue = apc_queue_current();
	long count = 0;

	if (!queue)
		return 0;
	for (;;)
	{
		struct platform_apc *apc;

		pthread_mutex_lock(&queue->lock);
		apc = queue->head;
		if (apc)
		{
			queue->head = apc->next;
			if (!queue->head)
				queue->tail = NULL;
		}
		pthread_mutex_unlock(&queue->lock);
		if (!apc)
			break;
		apc->routine(apc->context[0], apc->context[1], apc->context[2]);
		free(apc);
		count++;
	}
	return count;
}

/* ---------- waiting */

static void deadline_from_milliseconds(DWORD milliseconds, struct timespec *deadline)
{
	clock_gettime(CLOCK_REALTIME, deadline);
	deadline->tv_sec += milliseconds / 1000;
	deadline->tv_nsec += (long)(milliseconds % 1000) * 1000000L;
	if (deadline->tv_nsec >= 1000000000L)
	{
		deadline->tv_sec++;
		deadline->tv_nsec -= 1000000000L;
	}
}

static BOOL handle_try_acquire(struct platform_handle *handle)
{
	if (handle->type == _platform_handle_mutex)
	{
		if (handle->recursion == 0 || pthread_equal(handle->owner, pthread_self()))
		{
			handle->owner = pthread_self();
			handle->recursion++;
			return TRUE;
		}
		return FALSE;
	}
	if (!handle->signaled)
		return FALSE;
	if (handle->type == _platform_handle_event && !handle->manual_reset)
		handle->signaled = FALSE;
	return TRUE;
}

DWORD WINAPI WaitForSingleObjectEx(HANDLE object, DWORD milliseconds, BOOL alertable)
{
	struct platform_handle *handle;
	struct timespec deadline;
	struct platform_apc_queue *queue;
	DWORD result = WAIT_OBJECT_0;

	if (alertable && platform_run_apcs())
		return WAIT_IO_COMPLETION;

	handle = platform_handle_get(object, 0);
	if (!handle)
		return WAIT_FAILED;
	if (handle->type != _platform_handle_event &&
		handle->type != _platform_handle_mutex &&
		handle->type != _platform_handle_thread)
	{
		/* files and the like are always signaled */
		return WAIT_OBJECT_0;
	}

	if (milliseconds != INFINITE)
		deadline_from_milliseconds(milliseconds, &deadline);
	queue = alertable ? apc_queue_current() : NULL;

	pthread_mutex_lock(&handle->lock);
	if (queue)
	{
		pthread_mutex_lock(&queue->lock);
		queue->waiting_on = handle;
		pthread_mutex_unlock(&queue->lock);
	}
	while (!handle_try_acquire(handle))
	{
		if (queue && apc_queue_pending(queue))
		{
			result = WAIT_IO_COMPLETION;
			break;
		}
		if (milliseconds == 0)
		{
			result = WAIT_TIMEOUT;
			break;
		}
		if (milliseconds == INFINITE)
		{
			pthread_cond_wait(&handle->condition, &handle->lock);
		}
		else if (pthread_cond_timedwait(&handle->condition, &handle->lock, &deadline) == ETIMEDOUT)
		{
			if (!handle_try_acquire(handle))
				result = queue && apc_queue_pending(queue) ? WAIT_IO_COMPLETION : WAIT_TIMEOUT;
			break;
		}
	}
	if (queue)
	{
		pthread_mutex_lock(&queue->lock);
		queue->waiting_on = NULL;
		pthread_mutex_unlock(&queue->lock);
	}
	pthread_mutex_unlock(&handle->lock);
	if (result == WAIT_IO_COMPLETION)
		platform_run_apcs();
	return result;
}

DWORD WINAPI WaitForSingleObject(HANDLE object, DWORD milliseconds)
{
	return WaitForSingleObjectEx(object, milliseconds, FALSE);
}

/* ---------- events */

HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES attributes, BOOL manual_reset,
	BOOL initial_state, LPCSTR name)
{
	struct platform_handle *handle = platform_handle_new(_platform_handle_event, NULL, NULL);

	(void)attributes;
	if (name)
		platform_log("CreateEventA: named event \"%s\" is process-local", name);
	if (!handle)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	handle->manual_reset = manual_reset;
	handle->signaled = initial_state;
	return handle;
}

BOOL WINAPI SetEvent(HANDLE event)
{
	struct platform_handle *handle = platform_handle_get(event, _platform_handle_event);

	if (!handle)
		return FALSE;
	platform_handle_signal(handle);
	return TRUE;
}

BOOL WINAPI ResetEvent(HANDLE event)
{
	struct platform_handle *handle = platform_handle_get(event, _platform_handle_event);

	if (!handle)
		return FALSE;
	pthread_mutex_lock(&handle->lock);
	handle->signaled = FALSE;
	pthread_mutex_unlock(&handle->lock);
	return TRUE;
}

/* ---------- mutexes */

HANDLE WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES attributes, BOOL initial_owner, LPCSTR name)
{
	struct platform_handle *handle = platform_handle_new(_platform_handle_mutex, NULL, NULL);

	(void)attributes;
	(void)name;
	if (!handle)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	if (initial_owner)
	{
		handle->owner = pthread_self();
		handle->recursion = 1;
	}
	return handle;
}

BOOL WINAPI ReleaseMutex(HANDLE mutex)
{
	struct platform_handle *handle = platform_handle_get(mutex, _platform_handle_mutex);
	BOOL result = FALSE;

	if (!handle)
		return FALSE;
	pthread_mutex_lock(&handle->lock);
	if (handle->recursion > 0 && pthread_equal(handle->owner, pthread_self()))
	{
		if (--handle->recursion == 0)
			pthread_cond_broadcast(&handle->condition);
		result = TRUE;
	}
	else
	{
		SetLastError(ERROR_NOT_OWNER);
	}
	pthread_mutex_unlock(&handle->lock);
	return result;
}

/* ---------- critical sections

The XDK maps EnterCriticalSection and friends onto the Rtl* kernel exports;
the structure is large enough to hold a pointer to a recursive mutex. */

static pthread_mutex_t *critical_section_mutex(PRTL_CRITICAL_SECTION section)
{
	static pthread_mutex_t creation_lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t **slot = (pthread_mutex_t **)section;

	if (!*slot)
	{
		pthread_mutex_lock(&creation_lock);
		if (!*slot)
		{
			pthread_mutexattr_t attributes;
			pthread_mutex_t *mutex = malloc(sizeof(*mutex));

			pthread_mutexattr_init(&attributes);
			pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(mutex, &attributes);
			pthread_mutexattr_destroy(&attributes);
			*slot = mutex;
		}
		pthread_mutex_unlock(&creation_lock);
	}
	return *slot;
}

VOID NTAPI RtlInitializeCriticalSection(PRTL_CRITICAL_SECTION section)
{
	memset(section, 0, sizeof(*section));
	critical_section_mutex(section);
}

VOID NTAPI RtlEnterCriticalSection(PRTL_CRITICAL_SECTION section)
{
	pthread_mutex_lock(critical_section_mutex(section));
}

VOID NTAPI RtlLeaveCriticalSection(PRTL_CRITICAL_SECTION section)
{
	pthread_mutex_unlock(critical_section_mutex(section));
}

DWORD NTAPI RtlTryEnterCriticalSection(PRTL_CRITICAL_SECTION section)
{
	return pthread_mutex_trylock(critical_section_mutex(section)) == 0;
}

/* the XDK defines RtlDeleteCriticalSection as a no-op macro */

/* ---------- interlocked operations (see halo_linux_prefix.h) */

LONG WINAPI halo_linux_InterlockedIncrement(LPLONG addend)
{
	return __sync_add_and_fetch(addend, 1);
}

LONG WINAPI halo_linux_InterlockedDecrement(LPLONG addend)
{
	return __sync_sub_and_fetch(addend, 1);
}

LONG WINAPI halo_linux_InterlockedExchange(LPLONG target, LONG value)
{
	return __sync_lock_test_and_set(target, value);
}

LONG WINAPI halo_linux_InterlockedExchangeAdd(LPLONG addend, LONG value)
{
	return __sync_fetch_and_add(addend, value);
}

LONG WINAPI halo_linux_InterlockedCompareExchange(LPLONG destination, LONG exchange, LONG comparand)
{
	return __sync_val_compare_and_swap(destination, comparand, exchange);
}

/* ---------- threads */

struct platform_thread
{
	struct platform_handle *handle;
	LPTHREAD_START_ROUTINE start;
	LPVOID parameter;
	DWORD exit_code;
	BOOL suspended;
	BOOL closed;
	BOOL finished;
	pthread_t thread;
	struct platform_apc_queue *apcs;
};

static void thread_release(struct platform_handle *handle)
{
	struct platform_thread *thread = handle->data;
	BOOL free_now;

	pthread_mutex_lock(&handle->lock);
	free_now = thread->finished;
	thread->closed = TRUE;
	pthread_mutex_unlock(&handle->lock);
	if (free_now)
	{
		handle->signature = 0;
		free(thread);
		pthread_cond_destroy(&handle->condition);
		pthread_mutex_destroy(&handle->lock);
		free(handle);
	}
}

static void *thread_main(void *context)
{
	struct platform_thread *thread = context;
	struct platform_handle *handle = thread->handle;
	DWORD exit_code;
	BOOL free_now;

	platform_current_apc_queue = thread->apcs;
	pthread_mutex_lock(&handle->lock);
	while (thread->suspended)
		pthread_cond_wait(&handle->condition, &handle->lock);
	pthread_mutex_unlock(&handle->lock);

	exit_code = thread->start(thread->parameter);

	pthread_mutex_lock(&handle->lock);
	thread->exit_code = exit_code;
	thread->finished = TRUE;
	handle->signaled = TRUE;
	pthread_cond_broadcast(&handle->condition);
	free_now = thread->closed;
	pthread_mutex_unlock(&handle->lock);
	if (free_now)
	{
		handle->signature = 0;
		free(thread);
		pthread_cond_destroy(&handle->condition);
		pthread_mutex_destroy(&handle->lock);
		free(handle);
	}
	return NULL;
}

HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES attributes, DWORD stack_size,
	LPTHREAD_START_ROUTINE start, LPVOID parameter, DWORD flags, LPDWORD thread_id)
{
	static LONG next_thread_id = 1;
	struct platform_thread *thread = calloc(1, sizeof(*thread));
	struct platform_handle *handle;
	pthread_attr_t thread_attributes;

	(void)attributes;
	if (!thread)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	handle = platform_handle_new(_platform_handle_thread, thread, thread_release);
	if (!handle)
	{
		free(thread);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	thread->handle = handle;
	thread->start = start;
	thread->parameter = parameter;
	thread->exit_code = STILL_ACTIVE;
	thread->suspended = (flags & CREATE_SUSPENDED) != 0;
	thread->apcs = apc_queue_new();
	if (!thread->apcs)
	{
		free(thread);
		free(handle);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}

	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);
	/* Xbox stacks are small; give the host a comfortable minimum */
	pthread_attr_setstacksize(&thread_attributes, stack_size > 0x100000 ? stack_size : 0x100000);
	if (pthread_create(&thread->thread, &thread_attributes, thread_main, thread) != 0)
	{
		pthread_attr_destroy(&thread_attributes);
		free(thread);
		free(handle);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	pthread_attr_destroy(&thread_attributes);
	if (thread_id)
		*thread_id = (DWORD)halo_linux_InterlockedIncrement(&next_thread_id);
	return handle;
}

DWORD WINAPI ResumeThread(HANDLE object)
{
	struct platform_handle *handle = platform_handle_get(object, _platform_handle_thread);
	struct platform_thread *thread;
	DWORD previous;

	if (!handle)
		return (DWORD)-1;
	thread = handle->data;
	pthread_mutex_lock(&handle->lock);
	previous = thread->suspended ? 1 : 0;
	thread->suspended = FALSE;
	pthread_cond_broadcast(&handle->condition);
	pthread_mutex_unlock(&handle->lock);
	return previous;
}

BOOL WINAPI SetThreadPriority(HANDLE object, int priority)
{
	/* Linux does not let unprivileged processes raise thread priority;
	the Xbox scheduling hints are not needed for correctness */
	(void)priority;
	return object == GetCurrentThread() || platform_handle_get(object, _platform_handle_thread) != NULL;
}

BOOL WINAPI GetExitCodeThread(HANDLE object, LPDWORD exit_code)
{
	struct platform_handle *handle = platform_handle_get(object, _platform_handle_thread);
	struct platform_thread *thread;

	if (!handle)
		return FALSE;
	thread = handle->data;
	pthread_mutex_lock(&handle->lock);
	*exit_code = thread->exit_code;
	pthread_mutex_unlock(&handle->lock);
	return TRUE;
}

static void user_apc_trampoline(void *routine, void *data, void *unused)
{
	(void)unused;
	((PAPCFUNC)routine)((ULONG_PTR)data);
}

/* 0x00034274 (XAPI) */
DWORD WINAPI QueueUserAPC(PAPCFUNC routine, HANDLE thread_handle, ULONG_PTR data)
{
	struct platform_apc_queue *queue;

	if (!routine)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	if (thread_handle == GetCurrentThread())
	{
		queue = apc_queue_current();
	}
	else
	{
		struct platform_handle *handle = platform_handle_get(thread_handle, _platform_handle_thread);

		if (!handle)
			return 0;
		queue = ((struct platform_thread *)handle->data)->apcs;
	}
	if (!apc_queue_push(queue, user_apc_trampoline, (void *)routine, (void *)data, NULL))
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}
	return 1;
}

BOOL WINAPI SwitchToThread(void)
{
	sched_yield();
	return TRUE;
}

DWORD WINAPI SleepEx(DWORD milliseconds, BOOL alertable)
{
	struct timespec duration;

	if (alertable && platform_run_apcs())
		return WAIT_IO_COMPLETION;
	if (alertable)
	{
		/* sleep until the time is up or an APC arrives */
		struct platform_apc_queue *queue = apc_queue_current();
		struct timespec deadline;

		if (milliseconds == 0)
			sched_yield();
		else if (queue)
		{
			if (milliseconds != INFINITE)
				deadline_from_milliseconds(milliseconds, &deadline);
			pthread_mutex_lock(&queue->lock);
			while (!queue->head)
			{
				if (milliseconds == INFINITE)
					pthread_cond_wait(&queue->condition, &queue->lock);
				else if (pthread_cond_timedwait(&queue->condition, &queue->lock, &deadline) == ETIMEDOUT)
					break;
			}
			pthread_mutex_unlock(&queue->lock);
		}
		return platform_run_apcs() ? WAIT_IO_COMPLETION : 0;
	}
	if (milliseconds == 0)
	{
		sched_yield();
		return 0;
	}
	if (milliseconds == INFINITE)
	{
		for (;;)
			pause();
	}
	duration.tv_sec = milliseconds / 1000;
	duration.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
	while (nanosleep(&duration, &duration) == -1 && errno == EINTR)
		;
	if (alertable && platform_run_apcs())
		return WAIT_IO_COMPLETION;
	return 0;
}

VOID WINAPI Sleep(DWORD milliseconds)
{
	SleepEx(milliseconds, FALSE);
}

/* ---------- time */

/* The game's clocks count from when it started, as the Xbox's count from
power-on. Its code keeps times in signed 32-bit variables, which a host up
for more than 24.8 days (2^31 ms since boot) overflows: a hosted game then
timed out its own clients. They start at 10 s, about where an Xbox's clock
is by the time the game runs. */
static unsigned long long platform_clock_nanoseconds(void)
{
	static unsigned long long start;
	struct timespec now;
	unsigned long long value, expected = 0;

	clock_gettime(CLOCK_MONOTONIC, &now);
	value = (unsigned long long)now.tv_sec * 1000000000ULL + (unsigned long long)now.tv_nsec;
	__atomic_compare_exchange_n(&start, &expected, value - 10000000000ULL, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
	return value - __atomic_load_n(&start, __ATOMIC_RELAXED);
}

DWORD WINAPI GetTickCount(void)
{
	return (DWORD)(platform_clock_nanoseconds() / 1000000ULL);
}

/* The Xbox performance counter runs at the 733 MHz CPU clock. Report a
microsecond counter instead: coarse enough that 32-bit intermediate
arithmetic in the game stays in range, fine enough for frame timing. */
#define PLATFORM_PERFORMANCE_FREQUENCY 1000000ULL

BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER *count)
{
	count->QuadPart = (LONGLONG)(platform_clock_nanoseconds() / (1000000000ULL / PLATFORM_PERFORMANCE_FREQUENCY));
	return TRUE;
}

BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER *frequency)
{
	frequency->QuadPart = (LONGLONG)PLATFORM_PERFORMANCE_FREQUENCY;
	return TRUE;
}

/* seconds between 1601-01-01 and 1970-01-01 */
#define FILETIME_UNIX_EPOCH_SECONDS 11644473600ULL

void platform_unix_time_to_filetime(unsigned long seconds, unsigned long nanoseconds, FILETIME *file_time)
{
	unsigned long long value = ((unsigned long long)seconds + FILETIME_UNIX_EPOCH_SECONDS) * 10000000ULL +
		nanoseconds / 100;

	file_time->dwLowDateTime = (DWORD)value;
	file_time->dwHighDateTime = (DWORD)(value >> 32);
}

void platform_filetime_to_unix_time(const FILETIME *file_time, unsigned long *seconds, unsigned long *nanoseconds)
{
	unsigned long long value = ((unsigned long long)file_time->dwHighDateTime << 32) | file_time->dwLowDateTime;
	unsigned long long total_seconds = value / 10000000ULL;

	*seconds = total_seconds > FILETIME_UNIX_EPOCH_SECONDS ? (unsigned long)(total_seconds - FILETIME_UNIX_EPOCH_SECONDS) : 0;
	*nanoseconds = (unsigned long)(value % 10000000ULL) * 100;
}

LONG WINAPI CompareFileTime(CONST FILETIME *time1, CONST FILETIME *time2)
{
	unsigned long long value1 = ((unsigned long long)time1->dwHighDateTime << 32) | time1->dwLowDateTime;
	unsigned long long value2 = ((unsigned long long)time2->dwHighDateTime << 32) | time2->dwLowDateTime;

	return value1 < value2 ? -1 : value1 > value2 ? 1 : 0;
}

static const int days_before_month[2][13] =
{
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 },
};

static int is_leap_year(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

BOOL WINAPI SystemTimeToFileTime(CONST SYSTEMTIME *system_time, LPFILETIME file_time)
{
	unsigned long long days = 0;
	unsigned long long value;
	int year;

	if (system_time->wYear < 1601 || system_time->wMonth < 1 || system_time->wMonth > 12 ||
		system_time->wDay < 1 || system_time->wDay > 31)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	for (year = 1601; year < system_time->wYear; year++)
		days += is_leap_year(year) ? 366 : 365;
	days += days_before_month[is_leap_year(system_time->wYear)][system_time->wMonth - 1];
	days += system_time->wDay - 1;
	value = ((days * 24 + system_time->wHour) * 60 + system_time->wMinute) * 60 + system_time->wSecond;
	value = value * 10000000ULL + (unsigned long long)system_time->wMilliseconds * 10000ULL;
	file_time->dwLowDateTime = (DWORD)value;
	file_time->dwHighDateTime = (DWORD)(value >> 32);
	return TRUE;
}

/* 0x00030982 (kernel export): 100 ns units since 1601-01-01, UTC */
VOID NTAPI KeQuerySystemTime(LARGE_INTEGER *current_time)
{
	struct timespec now;

	clock_gettime(CLOCK_REALTIME, &now);
	current_time->QuadPart = (LONGLONG)(((unsigned long long)now.tv_sec + FILETIME_UNIX_EPOCH_SECONDS) * 10000000ULL +
		(unsigned long long)now.tv_nsec / 100);
}

/* 0x001e8de0 (XAPI): the Xbox subtracts the dashboard's time zone bias; the
port uses the host's zone (UTC when the guest C library knows none) */
VOID WINAPI GetLocalTime(LPSYSTEMTIME system_time)
{
	struct timespec now;
	struct tm local;
	time_t seconds;

	clock_gettime(CLOCK_REALTIME, &now);
	seconds = now.tv_sec;
	localtime_r(&seconds, &local);
	system_time->wYear = (WORD)(local.tm_year + 1900);
	system_time->wMonth = (WORD)(local.tm_mon + 1);
	system_time->wDayOfWeek = (WORD)local.tm_wday;
	system_time->wDay = (WORD)local.tm_mday;
	system_time->wHour = (WORD)local.tm_hour;
	system_time->wMinute = (WORD)local.tm_min;
	system_time->wSecond = (WORD)(local.tm_sec > 59 ? 59 : local.tm_sec);
	system_time->wMilliseconds = (WORD)(now.tv_nsec / 1000000L);
}

VOID WINAPI GetSystemTime(LPSYSTEMTIME system_time)
{
	struct timespec now;
	unsigned long long seconds;
	unsigned long days;
	int year = 1970;
	int month = 0;
	int leap;

	clock_gettime(CLOCK_REALTIME, &now);
	seconds = (unsigned long long)now.tv_sec;
	days = (unsigned long)(seconds / 86400);
	system_time->wDayOfWeek = (WORD)((days + 4) % 7); /* 1970-01-01 was a Thursday */
	for (;;)
	{
		unsigned long year_days = is_leap_year(year) ? 366 : 365;

		if (days < year_days)
			break;
		days -= year_days;
		year++;
	}
	leap = is_leap_year(year);
	while (month < 11 && days >= (unsigned long)days_before_month[leap][month + 1])
		month++;
	system_time->wYear = (WORD)year;
	system_time->wMonth = (WORD)(month + 1);
	system_time->wDay = (WORD)(days - days_before_month[leap][month] + 1);
	system_time->wHour = (WORD)((seconds / 3600) % 24);
	system_time->wMinute = (WORD)((seconds / 60) % 60);
	system_time->wSecond = (WORD)(seconds % 60);
	system_time->wMilliseconds = (WORD)(now.tv_nsec / 1000000L);
}

/* ---------- heap memory (GlobalAlloc / LocalAlloc family) */

struct global_block
{
	SIZE_T size;
	SIZE_T reserved;
	/* 16 byte header keeps the payload 16 byte aligned */
};

HGLOBAL WINAPI GlobalAlloc(UINT flags, SIZE_T size)
{
	struct global_block *block = (flags & GMEM_ZEROINIT) ?
		calloc(1, sizeof(*block) + 8 + size) :
		malloc(sizeof(*block) + 8 + size);

	if (!block)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	block->size = size;
	return (char *)block + sizeof(*block) + 8;
}

static struct global_block *global_block_from_pointer(HGLOBAL memory)
{
	return (struct global_block *)((char *)memory - sizeof(struct global_block) - 8);
}

HGLOBAL WINAPI GlobalReAlloc(HGLOBAL memory, SIZE_T size, UINT flags)
{
	struct global_block *block;
	SIZE_T old_size;

	if (!memory)
		return GlobalAlloc(flags, size);
	block = global_block_from_pointer(memory);
	old_size = block->size;
	block = realloc(block, sizeof(*block) + 8 + size);
	if (!block)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	if ((flags & GMEM_ZEROINIT) && size > old_size)
		memset((char *)block + sizeof(*block) + 8 + old_size, 0, size - old_size);
	block->size = size;
	return (char *)block + sizeof(*block) + 8;
}

HLOCAL WINAPI LocalFree(HLOCAL memory)
{
	if (memory)
		free(global_block_from_pointer(memory));
	return NULL;
}

SIZE_T WINAPI LocalSize(HLOCAL memory)
{
	return memory ? global_block_from_pointer(memory)->size : 0;
}

VOID WINAPI GlobalMemoryStatus(LPMEMORYSTATUS status)
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long available = sysconf(_SC_AVPHYS_PAGES);
	long page_size = sysconf(_SC_PAGESIZE);
	/* report at most an Xbox-sized 64 MB so size arithmetic in the game
	cannot overflow 32 bits */
	SIZE_T total = (SIZE_T)64 * 1024 * 1024;
	SIZE_T free_bytes = total;

	if (pages > 0 && available > 0 && page_size > 0 &&
		(unsigned long long)available * page_size < total)
	{
		free_bytes = (SIZE_T)((unsigned long long)available * page_size);
	}
	memset(status, 0, sizeof(*status));
	status->dwLength = sizeof(*status);
	status->dwTotalPhys = total;
	status->dwAvailPhys = free_bytes;
	status->dwTotalVirtual = 0x7ffe0000;
	status->dwAvailVirtual = 0x7ffe0000;
	status->dwMemoryLoad = (DWORD)(100 - (unsigned long long)free_bytes * 100 / total);
}

/* ---------- debug output */

VOID WINAPI OutputDebugStringA(LPCSTR string)
{
	if (string)
		fputs(string, stderr);
}

/* 0x0003097d (kernel export). The game calls it after a failed assertion,
which it has already printed. A native debugger is not attached, so log and
continue, as a retail Xbox without a debugger does. */
VOID NTAPI DbgBreakPoint(VOID)
{
	platform_log("DbgBreakPoint (the game hit a debug break; continuing)");
}

/* ---------- Win32 string helpers (XAPI) */

/* 0x000310c1 */
int WINAPI lstrlenA(LPCSTR string)
{
	return string ? (int)strlen(string) : 0;
}

/* 0x0003101e */
LPSTR WINAPI lstrcpyA(LPSTR destination, LPCSTR source)
{
	return strcpy(destination, source);
}

/* 0x0003105c: copies at most count - 1 characters and always terminates
(when count > 0) */
LPSTR WINAPI lstrcpynA(LPSTR destination, LPCSTR source, int count)
{
	int index = 0;

	if (count <= 0)
		return destination;
	while (index < count - 1 && source[index])
	{
		destination[index] = source[index];
		index++;
	}
	destination[index] = '\0';
	return destination;
}

/* 0x0003099f: this XBE's wsprintfA calls wvsprintfA (0x00030988), which is
the C library's vsprintf (0x001e041f): no 1024-character limit as in Win32 */
int wsprintfA(LPSTR buffer, LPCSTR format, ...)
{
	va_list arguments;
	int result;

	va_start(arguments, format);
	result = vsprintf(buffer, format, arguments);
	va_end(arguments);
	return result;
}
