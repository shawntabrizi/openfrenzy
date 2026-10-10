/*
TEST_CONFIG.C

of_config.c: the platform layer reads its settings through port_config.h. The
renderer and audio decide real behaviour from them (display.vsync paces frames,
debug.null_renderer skips the window, audio.enabled opens the device), so the
order of sources matters: a value written this run wins over the environment
(OPENFRENZY_<SECTION>_<KEY>), which wins over the built-in default.
*/

#include "platform.h"
#include "port_config.h"
#include "of_test.h"

#include <math.h>
#include <string.h>

int main(void)
{
	char text[64];
	unsigned long changes;

	setvbuf(stdout, NULL, _IONBF, 0);
	unsetenv("OPENFRENZY_DISPLAY_VSYNC");
	unsetenv("OPENFRENZY_AUDIO_VOLUME");
	/* (an agent's shell can set it false; the default is what a player gets) */
	unsetenv("OPENFRENZY_AUDIO_ENABLED");

	/* defaults */
	OF_CHECK(config_boolean("display.vsync") == 1, "display.vsync defaults to true");
	OF_CHECK(config_boolean("debug.null_renderer") == 0, "debug.null_renderer defaults to false");
	OF_CHECK(config_boolean("audio.enabled") == 1, "audio.enabled defaults to true: a fresh install has sound");
	OF_CHECK(fabs(config_real("audio.volume") - 1.0) < 1e-9, "audio.volume defaults to 1.0");
	OF_CHECK(config_integer("debug.screenshot_every") == 0, "debug.screenshot_every defaults to 0");
	OF_CHECK(!strcmp(config_string("audio.output_device"), "default"), "audio.output_device is \"default\"");

	/* an unknown setting is not an error for the caller: false, 0, "" */
	OF_CHECK(config_boolean("no.such_setting") == 0, "unknown boolean is false");
	OF_CHECK(!strcmp(config_string("no.such_setting"), ""), "unknown string is empty");
	OF_CHECK(config_text("no.such_setting", text, sizeof(text)) == 0, "config_text reports an unknown setting");

	/* the environment overrides the default */
	setenv("OPENFRENZY_DISPLAY_VSYNC", "false", 1);
	setenv("OPENFRENZY_AUDIO_VOLUME", "0.25", 1);
	OF_CHECK(config_boolean("display.vsync") == 0, "OPENFRENZY_DISPLAY_VSYNC=false turns vsync off");
	OF_CHECK(fabs(config_real("audio.volume") - 0.25) < 1e-9, "OPENFRENZY_AUDIO_VOLUME=0.25 is read");
	OF_CHECK(config_default("display.vsync", text, sizeof(text)) && !strcmp(text, "true"),
		"config_default still gives the built-in value (%s)", text);

	/* a written value overrides the environment, and is counted */
	changes = config_changes();
	OF_CHECK(config_write_boolean("display.vsync", 1) == 1, "config_write_boolean accepts a known setting");
	OF_CHECK(config_boolean("display.vsync") == 1, "the written value wins over the environment");
	OF_CHECK(config_changes() == changes + 1, "config_changes counts the write");
	OF_CHECK(config_write("no.such_setting", "1") == 0, "config_write refuses an unknown setting");
	OF_CHECK(config_changes() == changes + 1, "a refused write is not counted");

	/* the folders are the host's (host_main.c resolves OPENFRENZY_DATA_ROOT and
	OPENFRENZY_SAVE_ROOT), so saves never land in a Halo folder of the player's home */
	{
		const char *data = getenv("OPENFRENZY_DATA_ROOT");
		const char *saves = getenv("OPENFRENZY_SAVE_ROOT");
		char folder[1024], expected[1024];

		OF_CHECK(data && !strcmp(config_string("paths.data"), data), "paths.data is the host's data root (%s)",
			config_string("paths.data"));
		OF_CHECK(saves && !strcmp(config_string("paths.saves"), saves), "paths.saves is the host's save root (%s)",
			config_string("paths.saves"));
		config_folder(folder, sizeof(folder));
		snprintf(expected, sizeof(expected), "%s/", saves ? saves : "");
		OF_CHECK(!strcmp(folder, expected), "config_folder is the save root with '/' (%s)", folder);
		OF_CHECK(!strcmp(platform_data_root(), data ? data : ""), "platform_data_root (vfs) follows it (%s)",
			platform_data_root());
	}

	return of_test_result("test_config");
}
