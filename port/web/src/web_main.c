/*
WEB_MAIN.C

The web build's entry point and the WebAssembly memory's layout (docs/WEB.md).
Adapted from OpenCE (github.com/OpenCommunityEdition, CC0) port/web/src/web_main.c at
commit e0b581b1.

main runs on a worker of its own (Emscripten's PROXY_TO_PTHREAD), where it may block as
the game does. It mounts the site's Origin Private File System at /data (WasmFS), where the
page has put the player's disc image (/data/game.xiso.iso, port/web/site/app.js) and where
the saves are kept (/data/saves); then it starts the game (src/runtime/of_boot.c's main,
renamed of_game_main by port/web/CMakeLists.txt).
*/

#include <emscripten.h>
#include <emscripten/wasmfs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "web_shared.h"

int of_game_main(int argc, char **argv);
/* web_library.js: a line for the page (kind 0 a status, 3 a fatal error) */
void web_js_post(int kind, const char *text);

/* ---------- memory

The Xbox contiguous window (src/runtime/platform.h) is the top 128 MB of the WebAssembly
memory, 0x80000000 to 0x88000000 (port/web/CMakeLists.txt makes the memory that size). sbrk
grows the C heap up to the heap size this reports, so reporting the window's start keeps the
heap below it. */

#define WEB_HEAP_LIMIT 0x80000000UL

size_t emscripten_get_heap_size(void)
{
	size_t size = (size_t)__builtin_wasm_memory_size(0) << 16;

	return size > WEB_HEAP_LIMIT ? WEB_HEAP_LIMIT : size;
}

/* ---------- what the page reads (web_shared.h) */

volatile int32_t web_shared[WEB_SHARED_WORDS];

EMSCRIPTEN_KEEPALIVE volatile int32_t *web_state(void)
{
	return web_shared;
}

/* ---------- start */

int main(int argc, char **argv)
{
	backend_t opfs;
	int index;

	/* --NAME=value arguments from the page (port/web/site/app.js) become environment
	variables: the settings (src/runtime/of_config.c, OPENFRENZY_*) */
	for (index = 1; index < argc; index++)
	{
		const char *equals = strchr(argv[index], '=');

		if (!strncmp(argv[index], "--", 2) && equals && equals - argv[index] - 2 < 128)
		{
			char name[128];
			size_t length = (size_t)(equals - argv[index] - 2);

			memcpy(name, argv[index] + 2, length);
			name[length] = '\0';
			setenv(name, equals + 1, 1);
		}
	}

	opfs = wasmfs_create_opfs_backend();
	if (!opfs || wasmfs_create_directory("/data", 0777, opfs) != 0)
	{
		web_js_post(3, "The browser's private storage (OPFS) cannot be opened.");
		return EXIT_FAILURE;
	}
	mkdir("/data/saves", 0777);
	setenv("OPENFRENZY_DATA_ROOT", "/data", 0);
	setenv("OPENFRENZY_SAVE_ROOT", "/data/saves", 0);
	setenv("OPENFRENZY_ISO", "/data/game.xiso.iso", 0);
	/* the canvas fills the page; the page's own button makes it fullscreen */
	setenv("OPENFRENZY_DISPLAY_FULLSCREEN", "false", 0);
	web_js_post(0, "starting");
	return of_game_main(argc, argv);
}
