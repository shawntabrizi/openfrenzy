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
  OPENFRENZY_VARIATION=<n>     variation (the game uses 1 when it is not set)

Names: BumperCars, Fireworks, Clusteroids, Demolition, Drillers, Tag, Hopper,
MouseWhack, RollerBall, Jetboats, ScratchMatch, TankedUp (and Meta, the board).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void add(struct of_settings *settings, const char *name, const char *variable)
{
	const char *value = getenv(variable);
	struct of_settings_entry *entry;

	if (value == NULL || *value == '\0' || settings->count >= 0x80)
		return;
	entry = &settings->entries[settings->count++];
	snprintf(entry->name, sizeof(entry->name), "%s", name);
	snprintf(entry->value, sizeof(entry->value), "%s", value);
	fprintf(stderr, "openfrenzy: setting %s=%s (from %s)\n", name, value, variable);
}

/* called by Core_Init right after the table's constructor */
void of_settings_from_environment(void *table)
{
	if (table == NULL)
		return;
	add(table, "game", "OPENFRENZY_GAME");
	add(table, "variation", "OPENFRENZY_VARIATION");
}
