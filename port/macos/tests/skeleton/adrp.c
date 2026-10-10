/*
ADRP.C

Guest test for the "codegen trap" (docs/INTEGRATION_NOTES.md, docs/ARCHITECTURE.md section 4).
Guest code runs at BIAS + address, so adrp/adr give the biased address (0x100_xxxxxxxx). LLVM's
ILP32 instruction selection believes that a global address has zero upper 32 bits. Without the
of-rebase fix, every 64-bit use of a global address carries the bias: a zext to 64 bits, a
64-bit compare, a `lsr #32`, or two pointers packed into one register. A guest pointer is a
32-bit value, so each check below must see the upper half as 0.

The test also covers what must keep working: jump tables and constant pools, which llc loads
through adrp as 64-bit bases. Exit code 0 = pass; N = the first check that failed.
*/

#include <stdint.h>

int global_word = 5;
static int static_array[8] = {1, 2, 3, 4, 5, 6, 7, 8};
struct pair { void *a; void *b; };

volatile uint64_t sink64;
volatile uint32_t sink32;

__attribute__((noinline)) static int callee_function(int x) { return x + 1; }

/* The callee sees a pointer argument; ILP32 assumes the caller zero-extended it. */
__attribute__((noinline)) static uint64_t widen(const void *p)
{
	return (uint64_t)(uintptr_t)p;
}

/* A 16-byte struct of pointers by value: llc packs both into one register pair. */
__attribute__((noinline)) static uint32_t pair_high(struct pair p)
{
	return (uint32_t)(uintptr_t)p.b;
}

__attribute__((noinline)) static int is_global(const void *p)
{
	return p == &global_word;
}

/* switch with a jump table (dense cases) */
__attribute__((noinline)) static int dispatch(int i)
{
	switch (i) {
	case 0: return 11;
	case 1: return 23;
	case 2: return 37;
	case 3: return 41;
	case 4: return 53;
	case 5: return 67;
	case 6: return 79;
	case 7: return 83;
	default: return -1;
	}
}

/* a double literal that llc loads from a constant pool */
__attribute__((noinline)) static double scale(double x)
{
	return x * 3.14159265358979;
}

int main(int argc, char **argv)
{
	volatile int index = argc - 1; /* 0, opaque to the optimizer */
	int local = 7;
	struct pair packed = {&global_word, &static_array[index + 3]};

	(void)argv;
	/* 1. (uint64_t)(uintptr_t)&global stored as a 64-bit value */
	sink64 = (uint64_t)(uintptr_t)&global_word;
	if (sink64 >> 32)
		return 1;
	/* 2. the same with a variable index (adrp + add + register offset) */
	sink64 = (uint64_t)(uintptr_t)&static_array[index + 2];
	if (sink64 >> 32)
		return 2;
	/* 3. a pointer argument widened by the callee */
	if (widen(&global_word) >> 32)
		return 3;
	/* 4. two pointers packed into one register (the vfs_fopen case) */
	if (pair_high(packed) != (uint32_t)(uintptr_t)&static_array[3])
		return 4;
	sink32 = (uint32_t)(uintptr_t)&static_array[3];
	if (pair_high(packed) != sink32)
		return 5;
	/* 6. a pointer loaded from memory compared with a global address */
	{
		const void *volatile from_memory = &global_word;
		if (!is_global(from_memory) || from_memory != (const void *)&global_word)
			return 6;
	}
	/* 7. a function address as a 64-bit integer, then a call through it */
	{
		int (*volatile fn)(int) = callee_function;
		sink64 = (uint64_t)(uintptr_t)callee_function;
		if (sink64 >> 32)
			return 7;
		if (fn(41) != 42)
			return 8;
	}
	/* 9. a stack address (sp is also biased) */
	if (widen(&local) >> 32)
		return 9;
	sink64 = (uint64_t)(uintptr_t)&local;
	if (sink64 >> 32)
		return 10;
	/* 11. memory access through the global still works */
	if (global_word + static_array[index + 7] != 13)
		return 11;
	/* 12. jump table */
	if (dispatch(index + 5) != 67 || dispatch(index + 2) != 37)
		return 12;
	/* 13. constant pool */
	if (scale((double)(index + 2)) < 6.28 || scale((double)(index + 2)) > 6.29)
		return 13;
	return 0;
}
