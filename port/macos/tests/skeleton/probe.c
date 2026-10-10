/*
PROBE.C

Guest program whose exit code depends on rebased memory access: a global (image .data), a
.bss array written through a pointer, a stack array, and a 32-bit pointer stored in a struct.
It exits with 42 only if every access reached the right arena address and pointers are 4 bytes.
The skeleton test checks that code; an empty main() alone could not fail this way.
*/

struct node
{
	struct node *next; /* 4 bytes in the guest, as in the Xbox structs */
	int value;
};

_Static_assert(sizeof(void *) == 4, "guest pointers must be 32-bit");
_Static_assert(sizeof(long) == 4, "guest long must be 32-bit");
_Static_assert(sizeof(struct node) == 8, "Xbox struct layout");

static struct node tail = {0, 30};
struct node head = {&tail, 10};
volatile int scratch[16];

int main(int argc, char **argv)
{
	volatile int local[4];
	volatile int *p = scratch;
	struct node *n;
	int sum = 0;

	(void)argv;
	local[0] = 1;
	p[argc] = 2;            /* argc is 1: scratch[1] */
	for (n = &head; n; n = n->next)
		sum += n->value;
	return sum + scratch[1] + local[0] - 1; /* 10 + 30 + 2 + 1 - 1 = 42 */
}
