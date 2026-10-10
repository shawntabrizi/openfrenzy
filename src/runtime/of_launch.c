/*
OF_LAUNCH.C

Port switch: start one minigame directly. openfrenzy runtime module.

The game keeps a "command line" settings table at *(0x002ba9a0) (constructor
0x00082860, lookup 0x00082870, 0x80 name/value entries of 0xb0 bytes, count at
+0x5800). Core_Init (0x0002cd80) reads "game" and "variation" from it and, when
"game" is set, starts that game instead of the title screen. On the retail disc
nothing fills the table: 0x00082860 only clears the count, and no other code
writes it (it is a PC-build leftover). The port fills it from the environment:

  OPENFRENZY_GAME=<name>       game name of g_Games (case-insensitive), e.g. Hopper
  OPENFRENZY_VARIATION=<n>     variation 1-4 for every minigame (1 when it is not set)

Names: BumperCars, Fireworks, Clusteroids, Demolition, Drillers, Tag, Hopper,
MouseWhack, RollerBall, Jetboats, ScratchMatch, TankedUp (and Meta, the board).

How the original reads the variation (each minigame has 4, named in its constructor):
- Core_Init calls the game's start (vtable +0xc) with atol("variation"), or 1 when it is
  not set. The start argument is 1-based in every game: it names the intro picture
  "<x>%d.bmp" and the intro text 0x2b/0x2f/.../0x57 + n, the same as the board passes.
- Five games read the setting again themselves and then ignore the argument:
  BumperCars (0x00087400), Drillers (0x0010fe50) and TankedUp (0x001c1fb0) take it
  1-based, like the argument; Demolition (0x000e9530) and RollerBall (0x00188cb0) take
  it 0-based (without the setting they use argument - 1). So "variation=4" asks
  Demolition for demo-v5.xbp, which is not on the disc (NULL + 0xc fault in
  bLoadPackageResources).
- The other seven (Fireworks 0x000b1aa0, Clusteroids 0x000d9410, Tag 0x001253c0,
  Hopper 0x0013b9f0, MouseWhack 0x0016cd40, Jetboats 0x0019d590, ScratchMatch
  0x001ba5c0) only use the 1-based argument.
The board (CGameMode_AskAgain, 0x0002dd50) also calls start with variation + 1.
The port takes a 1-based variation for every minigame, checks it is 1-4, and writes
n - 1 for Demolition and RollerBall. A bad value stops the game before it starts.
Variations the retail game never plays (Clusteroids 3, RollerBall 3, ScratchMatch 4) get
a warning; ScratchMatch 4 is refused, because its package is empty.
OPENFRENZY_VARIATION without OPENFRENZY_GAME is refused too: the five games above
would read it again when the board starts them. For Meta the value goes in unchanged
(the board's start takes debug-menu zone numbers, not 1-4).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* the layout of 0x00082860/0x00082870 (see src/bumpercars_a/BumperCars/unplaced.c) */
struct of_settings_entry {
	char name[0x20];
	char value[0x90];
};
struct of_settings {
	struct of_settings_entry entries[0x80];
	int count;
};
_Static_assert(sizeof(struct of_settings) == 0x5804, "settings table size");

/* the names of g_Games (constructors 0x00151040 ... 0x001c1760).
zero_based: the game reads the "variation" setting 0-based.
cut: the variation (1-4) that the retail game never plays: its entry in the playable
table at 0x002839e4 (game * 4 + variation - 1) is 0, so the board (0x0002dd50) and the
random pick (CGameMode_AskAgain) skip it. 0: none.
cut_fails: the cut variation does not load (ScratchMatch: Scratch4.xbp on the disc is an
empty 4 KB package, and SMScenic_Load faults on its NULL actor), so it is refused. */
static const struct {
	const char *name;
	int zero_based;
	int cut;
	int cut_fails;
} games[] = {
	{ "Meta", 0, 0, 0 },       { "BumperCars", 0, 0, 0 }, { "Fireworks", 0, 0, 0 },
	{ "Clusteroids", 0, 3, 0 }, { "Demolition", 1, 0, 0 }, { "Drillers", 0, 0, 0 },
	{ "Tag", 0, 0, 0 },        { "Hopper", 0, 0, 0 },     { "MouseWhack", 0, 0, 0 },
	{ "RollerBall", 1, 3, 0 }, { "Jetboats", 0, 0, 0 },   { "ScratchMatch", 0, 4, 1 },
	{ "TankedUp", 0, 0, 0 },
};
#define VARIATIONS 4

static void add(struct of_settings *settings, const char *name, const char *value, const char *variable)
{
	struct of_settings_entry *entry;

	if (settings->count >= 0x80)
		return;
	entry = &settings->entries[settings->count++];
	snprintf(entry->name, sizeof(entry->name), "%s", name);
	snprintf(entry->value, sizeof(entry->value), "%s", value);
	fprintf(stderr, "openfrenzy: setting %s=%s (from %s)\n", name, value, variable);
}

static void refuse(const char *message, const char *value)
{
	unsigned int i;

	fprintf(stderr, "openfrenzy: %s%s\n", message, value);
	fprintf(stderr, "openfrenzy: OPENFRENZY_GAME is one of:");
	for (i = 0; i < sizeof(games) / sizeof(games[0]); i++)
		fprintf(stderr, " %s", games[i].name);
	fprintf(stderr, "\nopenfrenzy: OPENFRENZY_VARIATION is 1-%d (needs OPENFRENZY_GAME)\n", VARIATIONS);
	exit(2);
}

/* called by Core_Init right after the table's constructor */
void of_settings_from_environment(void *table)
{
	const char *game = getenv("OPENFRENZY_GAME");
	const char *variation = getenv("OPENFRENZY_VARIATION");
	char value[16];
	char *end;
	long n;
	unsigned int i;

	if (table == NULL)
		return;
	if (game != NULL && *game == '\0')
		game = NULL;
	if (variation != NULL && *variation == '\0')
		variation = NULL;
	if (game == NULL)
	{
		if (variation != NULL)
			refuse("OPENFRENZY_VARIATION needs OPENFRENZY_GAME; got OPENFRENZY_VARIATION=", variation);
		return;
	}
	for (i = 0; i < sizeof(games) / sizeof(games[0]); i++)
	{
		if (strcasecmp(games[i].name, game) == 0)
			break;
	}
	if (i == sizeof(games) / sizeof(games[0]))
		refuse("unknown OPENFRENZY_GAME=", game);
	if (variation == NULL || i == 0)
	{
		/* no variation (the game starts its first), or Meta: no 1-4 variations, the
		value goes in unchanged */
		add(table, "game", game, "OPENFRENZY_GAME");
		if (variation != NULL)
			add(table, "variation", variation, "OPENFRENZY_VARIATION");
		return;
	}
	n = strtol(variation, &end, 10);
	if (end == variation || *end != '\0' || n < 1 || n > VARIATIONS)
		refuse("OPENFRENZY_VARIATION must be 1-4; got OPENFRENZY_VARIATION=", variation);
	if (n == games[i].cut && games[i].cut_fails)
		refuse("this variation is cut from the retail game and its package is empty on the disc; got OPENFRENZY_VARIATION=", variation);
	if (n == games[i].cut)
		fprintf(stderr, "openfrenzy: warning: %s variation %ld is cut from the retail game (the board never plays it)\n",
			games[i].name, n);
	snprintf(value, sizeof(value), "%ld", games[i].zero_based ? n - 1 : n);
	fprintf(stderr, "openfrenzy: %s variation %ld of %d%s\n", games[i].name, n, VARIATIONS,
		games[i].zero_based ? " (the game reads it 0-based)" : "");
	add(table, "game", game, "OPENFRENZY_GAME");
	add(table, "variation", value, "OPENFRENZY_VARIATION");
}
