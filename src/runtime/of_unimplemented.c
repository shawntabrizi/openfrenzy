/*
OF_UNIMPLEMENTED.C

What the generated weak stubs (build/.../gen/of_stubs.c) call for an original
function that is not decompiled yet. openfrenzy runtime module.

  OPENFRENZY_STUB_LOG=all   log every call, not only the first one per function
  OPENFRENZY_STUB_ABORT=1   stop the game at the first unimplemented function

The log goes to stderr: "UNIMPLEMENTED 0x0002cd80 FUN_0002cd80 (#12)", where
#12 counts the distinct stubs reached so far.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEEN_SLOTS 8192u   /* power of two, larger than the 4818 original functions */

static unsigned int seen[SEEN_SLOTS];
static unsigned int seen_count;
static int log_all = -1;
static int abort_first = -1;

static int flag(const char *name, const char *value)
{
	const char *setting = getenv(name);

	return setting && !strcmp(setting, value);
}

/* inserts address; returns 1 if it was new */
static int remember(unsigned int address)
{
	unsigned int slot = (address * 2654435761u) & (SEEN_SLOTS - 1);

	while (seen[slot])
	{
		if (seen[slot] == address)
			return 0;
		slot = (slot + 1) & (SEEN_SLOTS - 1);
	}
	if (seen_count >= SEEN_SLOTS - 1)
		return 0;
	seen[slot] = address;
	seen_count++;
	return 1;
}

void of_unimplemented(unsigned int address, const char *name)
{
	int first;

	if (log_all < 0)
	{
		log_all = flag("OPENFRENZY_STUB_LOG", "all");
		abort_first = flag("OPENFRENZY_STUB_ABORT", "1");
	}
	first = remember(address);
	if (first || log_all)
		fprintf(stderr, "UNIMPLEMENTED 0x%08x %s (#%u)\n", address, name, seen_count);
	if (abort_first)
	{
		fprintf(stderr, "OPENFRENZY_STUB_ABORT=1: stopping at the first unimplemented function\n");
		abort();
	}
}
