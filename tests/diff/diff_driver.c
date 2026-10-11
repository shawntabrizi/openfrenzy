/*
diff_driver.c: port side of the differential test (tests/diff/README.md).

run_diff.py generates a C file that defines of_diff_call() (one case per tested function:
a call through the function's real symbol with the argument words cast to the parameter
types) and includes this file. Both are compiled with the game build's command and linked
with the game build's objects (without of_boot.c, whose main() starts the game), so the calls
run the real compiled game code. Static constructors do not run: as on the x86 side, the
globals hold their initial values.

Input (OF_DIFF_CASES, little endian):
  "OFDF" u32 version, u32 scratch base, u32 scratch size, u32 images, u32 ranges, u32 cases
  images x scratch size bytes
  ranges x (u32 guest address, u32 size)      game globals to compare and restore
  cases x (u32 id, u32 function, u32 image, u32 words, words x u32)
Output (OF_DIFF_OUT, text, flushed after each case so that a fault leaves a complete record):
  B id                   before the call
  R id w0 w1 f64bits     return value
  M address hexbytes     changed bytes (scratch, or a global range at its port address)
  E id                   after the call
  DONE
OF_DIFF_FIRST: first case index to run (the harness restarts after a fault).
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

struct of_diff_result
{
	uint32_t w[2];
	double f;
};

int of_diff_call(uint32_t function, const uint32_t *a, struct of_diff_result *r);

static uint32_t rd32(const unsigned char *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void emit_changes(FILE *out, uint32_t address, const unsigned char *now,
	const unsigned char *old, uint32_t size)
{
	uint32_t i = 0;
	while (i < size)
	{
		uint32_t start;
		if (now[i] == old[i])
		{
			i++;
			continue;
		}
		start = i;
		while (i < size && now[i] != old[i])
			i++;
		fprintf(out, "M %08x ", address + start);
		for (uint32_t k = start; k < i; k++)
			fprintf(out, "%02x", now[k]);
		fputc('\n', out);
	}
}

int main(void)
{
	const char *in_path = getenv("OF_DIFF_CASES");
	const char *out_path = getenv("OF_DIFF_OUT");
	const char *first_text = getenv("OF_DIFF_FIRST");
	unsigned long first = first_text ? strtoul(first_text, NULL, 10) : 0;
	FILE *in;
	FILE *out;
	unsigned char *data;
	long size;
	uint32_t base, scratch_size, images, ranges, cases;
	const unsigned char *p;
	const unsigned char *image_data;
	const unsigned char *range_data;
	unsigned char *snapshot;
	unsigned char *scratch;
	uint32_t total = 0;

	if (!in_path || !out_path)
	{
		fprintf(stderr, "diff_driver: OF_DIFF_CASES and OF_DIFF_OUT must be set\n");
		return 2;
	}
	in = fopen(in_path, "rb");
	if (!in)
	{
		fprintf(stderr, "diff_driver: cannot open %s\n", in_path);
		return 2;
	}
	fseek(in, 0, SEEK_END);
	size = ftell(in);
	fseek(in, 0, SEEK_SET);
	data = malloc((size_t)size);
	if (!data || fread(data, 1, (size_t)size, in) != (size_t)size)
	{
		fprintf(stderr, "diff_driver: cannot read %s\n", in_path);
		return 2;
	}
	fclose(in);
	if (memcmp(data, "OFDF", 4) != 0 || rd32(data + 4) != 1)
	{
		fprintf(stderr, "diff_driver: bad case file\n");
		return 2;
	}
	base = rd32(data + 8);
	scratch_size = rd32(data + 12);
	images = rd32(data + 16);
	ranges = rd32(data + 20);
	cases = rd32(data + 24);
	image_data = data + 28;
	range_data = image_data + (size_t)images * scratch_size;
	p = range_data + (size_t)ranges * 8;

	scratch = mmap((void *)(uintptr_t)base, scratch_size, PROT_READ | PROT_WRITE,
		MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (scratch != (unsigned char *)(uintptr_t)base)
	{
		fprintf(stderr, "diff_driver: cannot map the scratch window at 0x%08x\n", base);
		return 2;
	}
	for (uint32_t i = 0; i < ranges; i++)
		total += rd32(range_data + i * 8 + 4);
	snapshot = malloc(total ? total : 1);
	for (uint32_t i = 0, at = 0; i < ranges; i++)
	{
		uint32_t n = rd32(range_data + i * 8 + 4);
		memcpy(snapshot + at, (void *)(uintptr_t)rd32(range_data + i * 8), n);
		at += n;
	}

	out = fopen(out_path, first ? "a" : "w");
	if (!out)
	{
		fprintf(stderr, "diff_driver: cannot write %s\n", out_path);
		return 2;
	}
	for (uint32_t c = 0; c < cases; c++)
	{
		uint32_t id = rd32(p);
		uint32_t function = rd32(p + 4);
		uint32_t image = rd32(p + 8);
		uint32_t words = rd32(p + 12);
		uint32_t args[64];
		struct of_diff_result r;
		const unsigned char *img = image_data + (size_t)image * scratch_size;
		p += 16;
		for (uint32_t i = 0; i < words && i < 64; i++)
			args[i] = rd32(p + i * 4);
		p += words * 4;
		if (c < first)
			continue;
		memcpy(scratch, img, scratch_size);
		memset(&r, 0, sizeof(r));
		fprintf(out, "B %u\n", id);
		fflush(out);
		if (of_diff_call(function, args, &r) != 0)
		{
			fprintf(out, "X %u unknown function %u\n", id, function);
			fflush(out);
			continue;
		}
		{
			uint64_t bits;
			memcpy(&bits, &r.f, 8);
			fprintf(out, "R %u %08x %08x %016llx\n", id, r.w[0], r.w[1], (unsigned long long)bits);
		}
		emit_changes(out, base, scratch, img, scratch_size);
		for (uint32_t i = 0, at = 0; i < ranges; i++)
		{
			uint32_t address = rd32(range_data + i * 8);
			uint32_t n = rd32(range_data + i * 8 + 4);
			unsigned char *now = (unsigned char *)(uintptr_t)address;
			if (memcmp(now, snapshot + at, n) != 0)
			{
				emit_changes(out, address, now, snapshot + at, n);
				memcpy(now, snapshot + at, n);
			}
			at += n;
		}
		fprintf(out, "E %u\n", id);
		fflush(out);
	}
	fprintf(out, "DONE\n");
	fclose(out);
	return 0;
}
