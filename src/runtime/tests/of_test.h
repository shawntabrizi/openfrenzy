/*
OF_TEST.H

Small helpers for the runtime module's guest tests (run_tests.sh).
A test prints "ok"/"FAIL" lines and returns of_test_result() from main().
*/

#ifndef OF_TEST_H
#define OF_TEST_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int of_test_failures;

#define OF_CHECK(condition, ...) \
	do \
	{ \
		if (condition) \
			printf("  ok   "); \
		else \
		{ \
			printf("  FAIL "); \
			of_test_failures++; \
		} \
		printf(__VA_ARGS__); \
		printf("  [%s]\n", #condition); \
	} while (0)

static int of_test_result(const char *name)
{
	printf("%s: %s (%d failed checks)\n", name, of_test_failures ? "FAILED" : "passed", of_test_failures);
	return of_test_failures ? 1 : 0;
}

/* a watchdog: if the test is still running after seconds, print why and exit with status 3
(a hang is a failure, not a timeout of the runner) */
struct of_watchdog
{
	unsigned int seconds;
	const char *what;
};

static void *of_watchdog_thread(void *argument)
{
	struct of_watchdog *watchdog = argument;

	sleep(watchdog->seconds);
	printf("  FAIL watchdog: still blocked after %u s in %s\n", watchdog->seconds, watchdog->what);
	fflush(stdout);
	exit(3);
	return NULL;
}

static void of_watchdog_start(struct of_watchdog *watchdog)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, of_watchdog_thread, watchdog) == 0)
		pthread_detach(thread);
	else
		printf("  warning: cannot start the watchdog thread\n");
}

#endif
