/*
HALO_HOOKS.C

openfrenzy: inert versions of the Halo-only features that OpenCE's platform
layer (port/xdk/src) calls into. In OpenCE they live in Halo's game code or
in Halo-specific port files (hud_hires.c, text_hires.c, menu_files.c,
touch_input.c, voice_audio.c, source/main/console.c,
port/linux/game/render_interpolation.c). Fuzion Frenzy has none of them, so
each hook reports "nothing here" and the platform layer takes its plain path.
*/

#include "platform.h"
#include "hud_hires.h"
#include "menu_files.h"
#include "text_hires.h"
#include "touch_input.h"
#include "voice_audio.h"
#include "main/console.h"

#include <stdarg.h>
#include <stdio.h>

/* hud_hires.h: no high-res HUD replacement textures */
long hud_hires_override_find(unsigned long address, unsigned long width, unsigned long height,
	unsigned long level0_size)
{
	(void)address; (void)width; (void)height; (void)level0_size;
	return -1;
}

unsigned int hud_hires_override_texture(long asset, unsigned long *levels)
{
	(void)asset;
	if (levels)
		*levels = 0;
	return 0;
}

int hud_hires_override_coverage(long asset)
{
	(void)asset;
	return 0;
}

int hud_hires_override_point_threshold(long asset)
{
	(void)asset;
	return 0;
}

/* menu_files.h: no Halo PC menu art */
unsigned int menu_art_texture(unsigned long data, unsigned long *levels)
{
	(void)data;
	if (levels)
		*levels = 0;
	return 0;
}

/* text_hires.h: no high-res text atlas */
unsigned int text_hires_atlas_texture(unsigned long data)
{
	(void)data;
	return 0;
}

/* touch_input.h: no on-screen touch controls (desktop build) */
void touch_input_gamepad(XINPUT_GAMEPAD *pad)
{
	(void)pad;
}

/* voice_audio.h: no voice chat to mix in */
void voice_audio_mix(float *output, unsigned long frames)
{
	(void)output; (void)frames;
}

/* main/console.h: Halo's debug console; messages go to the log */
unsigned char console_is_active(void)
{
	return 0;
}

void console_printf(unsigned char clear, const char *format, ...)
{
	char message[256];
	va_list arguments;

	(void)clear;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	platform_log("%s", message);
}

/* sdl_platform.h: Halo's render interpolation between 30 Hz ticks; off */
int halo_interpolation_enabled(void)
{
	return 0;
}

/* xbox_textures.c: Halo PC's split-screen menu layout; never */
unsigned char pc_menu_split_players(void)
{
	return 0;
}
