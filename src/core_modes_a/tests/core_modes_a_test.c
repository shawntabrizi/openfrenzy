/*
core_modes_a module tests: one guest image (src/core_modes_a/tests/run_tests.sh builds it from
the game build's objects of src/core_modes_a and the generated weak stubs).

The game image globs every .c under src/core_modes_a, so this file compiles to nothing unless
OPENFRENZY_CORE_MODES_A_TEST is defined.

What the tests pin down, and why it matters to the game:
- rand: Core_Rand must give the MSVC rand() sequence (seed 1: 41, 18467, ...). The attract
  mode, the "computertext" beeps, the cheers and the fireworks are picked from it; another
  generator would make the port's front end behave differently from the original.
- glyph count: the typing effect stops (and its sound stops) when it has typed as many
  characters as BG_CountGlyphs says; "@...]" markup counts as one character plus its ']' in
  the original, and the port keeps that.
- free characters: character select must never give two players the same character; the
  search wraps around and a player's own character counts as free for that player.
- typing: a CBGTypeText types one character per charDelay frames, jumps over "\n" (two
  characters) counting a line, and reveals exactly what it typed (the glyph callback reads
  "revealed"); its alpha moves towards the colour's alpha at alphaRate and it reports
  "finished" (0) only when it has faded to 0.
- matrix text: the grid scrolls up by "scroll" characters each interval and fills the new
  characters with '0'/'1'.
*/

#ifdef OPENFRENZY_CORE_MODES_A_TEST

#include "core_modes_a.h"
#include "core_modes_a_modes.h"
#include "core_modes_a_globals.h"

#include <stdlib.h>
#include <string.h>

/* globals of other modules that this test image does not link */
float DAT_0028fda8 = 1.0f;
int DAT_00290754;
char DAT_002900c8[6 * 0x108];
unsigned short DAT_00283c7c[6 * 16];

static int failures;
static int checks;

#define CHECK(condition, ...) \
	do { \
		checks++; \
		if (!(condition)) { \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s: ", __FILE__, __LINE__, #condition); \
			fprintf(stderr, __VA_ARGS__); \
			fprintf(stderr, "\n"); \
		} \
	} while (0)

/* ---------- rand */

static void test_rand(void)
{
	static const unsigned int msvc[5] = { 41, 18467, 6334, 26500, 19169 };
	int i;

	DAT_00285630 = 1;
	for (i = 0; i < 5; i++) {
		unsigned int r = Core_Rand();
		CHECK(r == msvc[i], "rand #%d = %u, MSVC gives %u", i, r, msvc[i]);
	}
	DAT_00285630 = 1;
	CHECK(Core_RandUnit() == (float)41 * (1.0f / 32767.0f), "RandUnit is rand() / 32767");
	CHECK(DAT_00285630 == 1u * 0x343fdu + 0x269ec3u, "both advance the one seed");
}

/* ---------- glyph count */

static void test_glyph_count(void)
{
	CHECK(BG_CountGlyphs((unsigned short *)u"abc") == 3, "plain text counts its characters");
	CHECK(BG_CountGlyphs((unsigned short *)u"a\\nb") == 4, "\\n is two characters here");
	CHECK(BG_CountGlyphs((unsigned short *)u"@c01]x") == 3, "markup: '@c01' + ']' + 'x'");
}

/* ---------- free characters */

static void test_free_characters(void)
{
	DAT_002baed0[0].character = 2;
	DAT_002baed0[1].character = -1;
	DAT_002baed0[2].character = 4;
	DAT_002baed0[3].character = -1;
	CHECK(CharSel_NextFreeCharacter(2, 0) == 2, "a player's own character is free for it");
	CHECK(CharSel_NextFreeCharacter(2, 1) == 3, "another player's character is skipped");
	CHECK(CharSel_NextFreeCharacter(4, 1) == 5, "next after a taken one");
	CHECK(CharSel_NextFreeCharacter(6, 1) == 0, "6 wraps to 0");
	CHECK(CharSel_PrevFreeCharacter(-1, 1) == 5, "-1 wraps to 5");
	CHECK(CharSel_PrevFreeCharacter(4, 1) == 3, "previous skips a taken one");
	CHECK(CharSel_PrevFreeCharacter(2, 1) == 1, "previous skips a taken one, down");
}

/* ---------- typing */

static void test_typing(void)
{
	static wchar16 buffer[16];
	struct CBGTypeText t;
	int finished;
	int frame;

	memset(&t, 0, sizeof(t));
	memcpy(buffer, u"ab\\ncd", 7 * sizeof(wchar16));
	t.buf.text.base.vtbl = vtable_00253810;
	t.buf.text.text = buffer;
	t.buf.buffer = buffer;
	t.buf.capacity = 16;
	t.buf.text.color[3] = 0x7f;
	t.alphaRate = 30.0f;
	t.glow = 0.0f;
	t.glowTarget = 0.0f;
	t.charDelay = 1.0f;
	t.startDelay = 0.0f;
	t.typeSound = 0;
	t.voice = -2;
	DAT_0028fda8 = 1.0f;

	CBGTypeText_Update(&t);
	CHECK(t.typed == 1 && t.revealed == 1, "one character a frame (typed %d)", t.typed);
	CBGTypeText_Update(&t);
	CHECK(t.typed == 4 && t.lines == 1, "\\n is skipped and counted (typed %d, lines %d)", t.typed, t.lines);
	CBGTypeText_Update(&t);
	CBGTypeText_Update(&t);
	CHECK(t.typed == 6 && t.revealed == 6, "all typed (typed %d)", t.typed);
	CBGTypeText_Update(&t);
	CHECK(t.typed == 6, "no more than the text (typed %d)", t.typed);
	CHECK(t.alpha == 127.0f, "alpha reached the colour's alpha (%f)", (double)t.alpha);

	t.buf.text.color[3] = 0;
	finished = 1;
	for (frame = 0; frame < 10 && finished; frame++)
		finished = CBGTypeText_Update(&t);
	CHECK(!finished && t.alpha == 0.0f, "fades out and reports it (%f)", (double)t.alpha);
}

/* ---------- matrix text */

static void test_matrix(void)
{
	static wchar16 grid[8];
	struct CBGMatrixText m;
	int i;
	int ok = 1;

	memset(&m, 0, sizeof(m));
	m.base.vtbl = vtable_00253788;
	m.buffer = grid;
	m.cols = 4;
	m.rows = 2;
	m.scroll = 4;
	m.interval = 1.0f;
	for (i = 0; i < 8; i++)
		grid[i] = 'A' + i;
	DAT_0028fda8 = 1.0f;
	CHECK(CBGMatrixText_Update(&m) == 1 && grid[0] == 'A', "no step before the interval");
	CHECK(CBGMatrixText_Update(&m) == 1, "a step");
	CHECK(grid[0] == 'E' && grid[3] == 'H', "the second row moved up");
	for (i = 4; i < 8; i++)
		ok = ok && (grid[i] == '0' || grid[i] == '1');
	CHECK(ok, "the new row is binary");
	CHECK(m.cursor == 7 && m.timer == 1.0f, "cursor at the end, the interval taken off");
}

int main(void)
{
	test_rand();
	test_glyph_count();
	test_free_characters();
	test_typing();
	test_matrix();
	fprintf(stderr, "core_modes_a tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
