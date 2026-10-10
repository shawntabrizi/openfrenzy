/*
SMOKE.C

Platform smoke test for the full guest stack (musl, guest runtime, host imports,
SDL3 window, GLES 3 on ANGLE/Metal, file access, the vfs disc reader):

1. opens a window and a GLES context, and clears it to a changing colour for
   OPENFRENZY_SMOKE_SECONDS seconds (default 3);
2. opens the disc image (OPENFRENZY_ISO, default <data root>/game.xiso.iso),
   finds default.xbe on it and checks the "XBEH" magic, then reads a game
   package header (Packages\BootSeq.xbp).

Exit code 0 when every step passed, else the number of the failed step.
With SDL_VIDEO_DRIVER=dummy (tests), step 1 is skipped.
*/

/* before SDL: musl's alloca.h must come first (as port/guest/runtime/guest_sdl.c) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <GLES3/gl3.h>

#include "xiso_fs.h"

typedef void (GL_APIENTRY *clear_color_proc)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (GL_APIENTRY *clear_proc)(GLbitfield);

static int window_step(void)
{
	const char *seconds_text = getenv("OPENFRENZY_SMOKE_SECONDS");
	const char *driver = getenv("SDL_VIDEO_DRIVER");
	unsigned long long seconds = seconds_text ? strtoull(seconds_text, NULL, 10) : 3;
	SDL_Window *window;
	SDL_GLContext context;
	clear_color_proc clear_color;
	clear_proc clear;
	unsigned long long start, now;
	unsigned int frames = 0;

	if (driver && !strcmp(driver, "dummy"))
	{
		printf("smoke: window step skipped (SDL_VIDEO_DRIVER=dummy)\n");
		return 0;
	}
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		printf("smoke: SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}
	window = SDL_CreateWindow("openfrenzy smoke", 640, 480, SDL_WINDOW_OPENGL);
	if (!window)
	{
		printf("smoke: SDL_CreateWindow failed: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	context = SDL_GL_CreateContext(window);
	if (!context || !SDL_GL_MakeCurrent(window, context))
	{
		printf("smoke: GLES context failed: %s\n", SDL_GetError());
		return 1;
	}
	clear_color = (clear_color_proc)SDL_GL_GetProcAddress("glClearColor");
	clear = (clear_proc)SDL_GL_GetProcAddress("glClear");
	if (!clear_color || !clear)
	{
		printf("smoke: GLES entry points missing\n");
		return 1;
	}
	SDL_GL_SetSwapInterval(1);
	start = SDL_GetTicks();
	do
	{
		SDL_Event event;
		float t;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
				seconds = 0;
		}
		now = SDL_GetTicks();
		t = (float)((now - start) % 2000) / 2000.0f;
		clear_color(t, 0.3f, 1.0f - t, 1.0f);
		clear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(window);
		frames++;
	} while (now - start < seconds * 1000);
	printf("smoke: window ok, %u frames in %llu ms\n", frames, now - start);
	return 0;
}

static int disc_step(void)
{
	char default_path[1024];
	const char *path = getenv("OPENFRENZY_ISO");
	struct xiso_fs fs;
	struct xiso_entry entry;
	unsigned char bytes[16];

	if (!path)
	{
		const char *root = getenv("OPENFRENZY_DATA_ROOT");

		snprintf(default_path, sizeof(default_path), "%s/game.xiso.iso", root ? root : ".");
		path = default_path;
	}
	if (xiso_fs_open(&fs, path))
	{
		printf("smoke: cannot open disc image %s\n", path);
		return 2;
	}
	if (xiso_fs_find(&fs, "default.xbe", &entry) ||
		xiso_fs_read(&fs, &entry, 0, bytes, 4) != 4 || memcmp(bytes, "XBEH", 4))
	{
		printf("smoke: default.xbe missing or bad on %s\n", path);
		return 2;
	}
	printf("smoke: disc ok: default.xbe, %u bytes, magic XBEH\n", entry.size);
	if (xiso_fs_find(&fs, "Packages\\BootSeq.xbp", &entry) ||
		xiso_fs_read(&fs, &entry, 0, bytes, 8) != 8)
	{
		printf("smoke: Packages\\BootSeq.xbp missing\n");
		return 3;
	}
	printf("smoke: Packages\\BootSeq.xbp, %u bytes, header %02x%02x%02x%02x %02x%02x%02x%02x\n",
		entry.size, bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
	xiso_fs_close(&fs);
	return 0;
}

int main(int argc, char **argv)
{
	int result;

	(void)argc;
	(void)argv;
	setvbuf(stdout, NULL, _IONBF, 0);
	printf("smoke: guest started, sizeof(void *) = %u\n", (unsigned)sizeof(void *));
	result = window_step();
	if (!result)
		result = disc_step();
	printf("smoke: %s\n", result ? "FAILED" : "passed");
	return result;
}
