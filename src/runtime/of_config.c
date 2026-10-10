/*
OF_CONFIG.C

openfrenzy runtime module: the settings interface of OpenCE's port_config.h,
without config.toml (OpenCE's port_config.c needs tomlc17 and Halo's menus).

Each setting has a default below (paths.data and paths.saves default to the host's
OPENFRENZY_DATA_ROOT and OPENFRENZY_SAVE_ROOT). An environment variable overrides it:
OPENFRENZY_ + the name in upper case with '.' as '_', for example
OPENFRENZY_DISPLAY_VSYNC=false or OPENFRENZY_AUDIO_VOLUME=0.5.
config_write() changes the value for this run only.
*/

#include "platform.h"
#include "port_config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct setting
{
	const char *name;
	const char *value;
};

/* the settings the platform layer reads (grep config_ in src/), with OpenCE's defaults
where they apply to this game */
static struct setting settings[] =
{
	{ "audio.enabled", "true" },
	{ "audio.reverb", "true" },
	{ "audio.volume", "1.0" },
	{ "audio.output_device", "default" },
	{ "audio.input_device", "default" },
	{ "display.anti_aliasing", "off" },
	{ "display.per_pixel_lighting", "false" },
	{ "display.resolution_scaling", "native" },
	{ "display.screen_width", "0" },
	{ "display.shadow_resolution", "0" },
	{ "display.vsync", "true" },
	{ "display.fullscreen", "false" },
	{ "display.interpolation", "false" },
	{ "game.language", "english" },
	{ "input.invert_mouse", "false" },
	{ "input.mouse_aim_assist", "false" },
	{ "input.mouse_sensitivity", "1.0" },
	{ "input.mouse_vertical_sensitivity", "1.0" },
	{ "paths.data", "" },
	{ "paths.saves", "" },
	{ "paths.custom_edition", "" },
	{ "debug.gl_debug", "false" },
	{ "debug.gpu_debug_flat", "false" },
	{ "debug.gpu_stats", "false" },
	{ "debug.gpu_trace_constants", "false" },
	{ "debug.null_renderer", "false" },
	{ "debug.texture_log", "false" },
	{ "debug.texture_no_cache", "false" },
	{ "debug.gpu_flush_draws", "0" },
	{ "debug.gpu_trace_frame", "0" },
	{ "debug.screenshot_every", "0" },
	{ "debug.skip_videos", "false" },
	{ "debug.gpu_debug_expression", "" },
	{ "debug.gpu_dump_shaders", "" },
	{ "debug.gpu_skip_vertex_shaders", "" },
	{ "debug.screenshot_directory", "" },
	{ "debug.test_input", "" },
	{ "debug.test_keys", "" },
	{ "debug.texture_dump_directory", "" },
};

#define SETTING_COUNT (sizeof(settings) / sizeof(settings[0]))
#define VALUE_SIZE 256

/* values set by config_write, by setting index */
static char written[SETTING_COUNT][VALUE_SIZE];
static unsigned char has_written[SETTING_COUNT];
static unsigned long changes;

static int setting_index(const char *name)
{
	unsigned int index;

	for (index = 0; index < SETTING_COUNT; index++)
	{
		if (!strcmp(settings[index].name, name))
			return (int)index;
	}
	return -1;
}

/* The folders the host has resolved (port/macos/host/host_main.c sets these variables to
absolute paths before the guest starts): the defaults of paths.data and paths.saves, so the
platform layer (vfs) uses the host's data and save roots, not Halo's ~/.local/share/halo-linux. */
static const struct setting host_folders[] =
{
	{ "paths.data", "OPENFRENZY_DATA_ROOT" },
	{ "paths.saves", "OPENFRENZY_SAVE_ROOT" },
};

/* the value as text: written, else the environment, else the default; NULL if unknown */
static const char *value_of(const char *name)
{
	char variable[128] = "OPENFRENZY_";
	size_t length = strlen(variable);
	const char *environment;
	const char *letter;
	unsigned int folder;
	int index = setting_index(name);

	if (index >= 0 && has_written[index])
		return written[index];
	for (letter = name; *letter && length + 1 < sizeof(variable); letter++)
		variable[length++] = *letter == '.' ? '_' : (char)toupper((unsigned char)*letter);
	variable[length] = 0;
	environment = getenv(variable);
	if (environment)
		return environment;
	if (index < 0)
	{
		platform_log("config: unknown setting %s", name);
		return NULL;
	}
	for (folder = 0; folder < sizeof(host_folders) / sizeof(host_folders[0]); folder++)
	{
		if (!strcmp(name, host_folders[folder].name) && (environment = getenv(host_folders[folder].value)) && *environment)
			return environment;
	}
	return settings[index].value;
}

int config_boolean(const char *name)
{
	const char *value = value_of(name);

	return value && (!strcmp(value, "true") || !strcmp(value, "1") || !strcmp(value, "yes"));
}

long config_integer(const char *name)
{
	const char *value = value_of(name);

	return value ? strtol(value, NULL, 10) : 0;
}

double config_real(const char *name)
{
	const char *value = value_of(name);

	return value ? strtod(value, NULL) : 0.0;
}

const char *config_string(const char *name)
{
	const char *value = value_of(name);

	return value ? value : "";
}

int config_write(const char *name, const char *value)
{
	int index = setting_index(name);

	if (index < 0)
		return 0;
	snprintf(written[index], VALUE_SIZE, "%s", value);
	has_written[index] = 1;
	changes++;
	return 1;
}

int config_write_boolean(const char *name, int value)
{
	return config_write(name, value ? "true" : "false");
}

int config_text(const char *name, char *text, size_t size)
{
	const char *value = value_of(name);

	if (!value)
		return 0;
	snprintf(text, size, "%s", value);
	return 1;
}

void config_folder(char *path, size_t size)
{
	snprintf(path, size, "%s/", platform_save_root());
}

char *config_file_read(const char *path, size_t *size)
{
	FILE *file = fopen(path, "rb");
	char *data = NULL;
	long length;

	if (!file)
		return NULL;
	if (!fseek(file, 0, SEEK_END) && (length = ftell(file)) >= 0 && !fseek(file, 0, SEEK_SET))
	{
		data = malloc((size_t)length + 1);
		if (data && fread(data, 1, (size_t)length, file) == (size_t)length)
		{
			data[length] = 0;
			if (size)
				*size = (size_t)length;
		}
		else
		{
			free(data);
			data = NULL;
		}
	}
	fclose(file);
	return data;
}

int config_default(const char *name, char *text, size_t size)
{
	int index = setting_index(name);

	if (index < 0)
		return 0;
	snprintf(text, size, "%s", settings[index].value);
	return 1;
}

unsigned long config_changes(void)
{
	return changes;
}
