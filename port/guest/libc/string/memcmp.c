/* openfrenzy: replaces musl 1.2.5 src/string/memcmp.c (a byte loop).

The guest compares large blocks on every draw: d3d8_ff_uniforms (src/d3d8/d3d8_ff.c)
compares 1440 bytes of fixed-function uniforms per draw call, and the program and
sampler caches in d3d8_gl.c compare their keys. With musl's byte loop, memcmp was
35 % of the main thread in MouseWhack (about 8600 draws a frame; `sample`,
docs/INTEGRATION_NOTES.md "Frame pacing"). This compares 8 bytes at a time and
finds the first different byte with the byte loop, so the result is the same as
musl's (sign of the first different unsigned byte). */
#include <stdint.h>
#include <string.h>

int memcmp(const void *vl, const void *vr, size_t n)
{
	const unsigned char *l = vl, *r = vr;

	for (; n >= 8; n -= 8, l += 8, r += 8) {
		uint64_t a, b;
		__builtin_memcpy(&a, l, 8);
		__builtin_memcpy(&b, r, 8);
		if (a != b)
			break;
	}
	for (; n && *l == *r; n--, l++, r++);
	return n ? *l - *r : 0;
}
