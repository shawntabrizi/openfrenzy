/*
TEST_UNIMPLEMENTED.C

of_unimplemented.c is what every generated weak stub calls. Its log is the
decompilation's to-do list: "UNIMPLEMENTED 0x<addr> <name> (#n)" once per
function, in the order the game first reaches them. Game threads (bKernel's
background loader, audio) reach stubs at the same time, so the once-per-function
rule must hold across threads, and #n must count distinct functions.

The test calls 40 addresses 50 times each from 4 threads and checks the count
of distinct functions (of_unimplemented_distinct). The log itself is checked by
eye in the run log: the guest cannot redirect stderr (freopen needs dup3,
syscall 24, which the host does not implement).
*/

#include "of_test.h"

void of_unimplemented(unsigned int address, const char *name);
unsigned int of_unimplemented_distinct(void);

#define ADDRESSES 40
#define THREADS 4

static void *caller(void *unused)
{
	int round, index;

	(void)unused;
	for (round = 0; round < 50; round++)
	{
		for (index = 0; index < ADDRESSES; index++)
			of_unimplemented(0x00100000u + (unsigned int)index * 16u, "FUN_test");
	}
	return NULL;
}

int main(void)
{
	pthread_t threads[THREADS];
	int index, started = 0;

	setvbuf(stdout, NULL, _IONBF, 0);
	unsetenv("OPENFRENZY_STUB_LOG");
	unsetenv("OPENFRENZY_STUB_ABORT");
	OF_CHECK(of_unimplemented_distinct() == 0, "no stub reached at start");
	for (index = 0; index < THREADS; index++)
		started += pthread_create(&threads[index], NULL, caller, NULL) == 0;
	OF_CHECK(started == THREADS, "%d threads started", started);
	for (index = 0; index < started; index++)
		pthread_join(threads[index], NULL);
	OF_CHECK(of_unimplemented_distinct() == ADDRESSES, "each function counted once across threads (%u for %d)",
		of_unimplemented_distinct(), ADDRESSES);
	of_unimplemented(0x00100000u, "FUN_test");
	OF_CHECK(of_unimplemented_distinct() == ADDRESSES, "a known function is not counted again");
	of_unimplemented(0x00200000u, "FUN_test_new");
	OF_CHECK(of_unimplemented_distinct() == ADDRESSES + 1, "a new function is");
	return of_test_result("test_unimplemented");
}
