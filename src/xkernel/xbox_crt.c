/*
XBOX_CRT.C

openfrenzy (xkernel module). C library functions of the XBE's LIB:libc region
whose results the game can observe and whose musl versions differ from the
Microsoft C runtime (XDK 3911) the game was linked with:

- rand / srand (0x001e133e): MSVC's linear congruential generator, one seed
  per thread starting at 1, results 0..0x7fff. musl's rand returns 31 bits,
  which breaks code that scales rand() by 1/32768, and a different sequence.
- qsort (0x001df260): MSVC's quicksort, so elements that compare equal end in
  the same order as on the Xbox (musl's smoothsort orders them differently).

These definitions replace musl's: the platform library is linked before
libguestc.a, so musl's rand.o and qsort_nr.o are never pulled in. Everything
else in the region maps to musl or src/runtime/msvc_crt.c (LIBRARY_IDS.md).
*/

#include <stddef.h>
#include <stdlib.h>

/* ---------- rand */

static __thread unsigned int crt_rand_seed = 1;

/* 0x001e133e (_rand): holdrand = holdrand * 214013 + 2531011 */
int rand(void)
{
	crt_rand_seed = crt_rand_seed * 214013u + 2531011u;
	return (int)((crt_rand_seed >> 16) & 0x7fff);
}

void srand(unsigned int seed)
{
	crt_rand_seed = seed;
}

/* ---------- qsort (MSVC's algorithm, from the XBE's 0x001df260) */

#define QSORT_CUTOFF 8
#define QSORT_STACK_SIZE 30

typedef int (*qsort_compare)(const void *, const void *);

static void qsort_swap(char *a, char *b, size_t width)
{
	if (a != b)
	{
		while (width--)
		{
			char value = *a;

			*a++ = *b;
			*b++ = value;
		}
	}
}

/* selection sort for short runs: moves the largest element to the end */
static void qsort_short(char *lo, char *hi, size_t width, qsort_compare compare)
{
	while (hi > lo)
	{
		char *max = lo;
		char *p;

		for (p = lo + width; p <= hi; p += width)
		{
			if (compare(p, max) > 0)
				max = p;
		}
		qsort_swap(max, hi, width);
		hi -= width;
	}
}

void qsort(void *base, size_t count, size_t width, qsort_compare compare)
{
	char *lostk[QSORT_STACK_SIZE];
	char *histk[QSORT_STACK_SIZE];
	char *lo, *hi, *mid, *loguy, *higuy;
	size_t size;
	int stkptr = 0;

	if (count < 2 || width == 0)
		return;
	lo = base;
	hi = (char *)base + width * (count - 1);

recurse:
	size = (size_t)(hi - lo) / width + 1;
	if (size <= QSORT_CUTOFF)
	{
		qsort_short(lo, hi, width, compare);
	}
	else
	{
		/* median of three into lo, mid, hi */
		mid = lo + (size / 2) * width;
		if (compare(lo, mid) > 0)
			qsort_swap(lo, mid, width);
		if (compare(lo, hi) > 0)
			qsort_swap(lo, hi, width);
		if (compare(mid, hi) > 0)
			qsort_swap(mid, hi, width);

		loguy = lo;
		higuy = hi;
		for (;;)
		{
			if (mid > loguy)
			{
				do
					loguy += width;
				while (loguy < mid && compare(loguy, mid) <= 0);
			}
			if (mid <= loguy)
			{
				do
					loguy += width;
				while (loguy <= hi && compare(loguy, mid) <= 0);
			}
			do
				higuy -= width;
			while (higuy > mid && compare(higuy, mid) > 0);
			if (higuy < loguy)
				break;
			qsort_swap(loguy, higuy, width);
			if (mid == higuy)
				mid = loguy;
		}

		/* skip elements equal to the partition element */
		higuy += width;
		if (mid < higuy)
		{
			do
				higuy -= width;
			while (higuy > mid && compare(higuy, mid) == 0);
		}
		if (mid >= higuy)
		{
			do
				higuy -= width;
			while (higuy > lo && compare(higuy, mid) == 0);
		}

		/* sort the smaller part next, push the larger one */
		if (higuy - lo >= hi - loguy)
		{
			if (lo < higuy)
			{
				lostk[stkptr] = lo;
				histk[stkptr] = higuy;
				++stkptr;
			}
			if (loguy < hi)
			{
				lo = loguy;
				goto recurse;
			}
		}
		else
		{
			if (loguy < hi)
			{
				lostk[stkptr] = loguy;
				histk[stkptr] = hi;
				++stkptr;
			}
			if (lo < higuy)
			{
				hi = higuy;
				goto recurse;
			}
		}
	}

	--stkptr;
	if (stkptr >= 0)
	{
		lo = lostk[stkptr];
		hi = histk[stkptr];
		goto recurse;
	}
}
