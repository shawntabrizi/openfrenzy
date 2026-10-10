/*
SDL_PLATFORM.C

openfrenzy runtime module: the SDL3 window, GLES context and event loop behind
the XDK layer (the interface of sdl_platform.h, from OpenCE).

Written for this port from the interface and from OpenCE port/linux/src/sdl_platform.c
(commit 231708d, CC0), whose window/context set-up and swap are followed here. Left out:
Halo's map extraction, internet-play invites, scoreboard pointer, menus' pointer,
touch input, window icon, updater, rebinding UI. The guest calls SDL through
port/guest/runtime/guest_sdl.c, so only the SDL functions listed there may be used.

Settings (of_config.c): display.vsync, display.resolution_scaling ("native" draws
at the window's pixels, "original" at 640x480). The host decides the window mode
(OPENFRENZY_WINDOWED=1 for a window, else borderless fullscreen).
*/

#include "platform.h"
#include "sdl_platform.h"
#include "gl.h"
#include "port_config.h"

#include <SDL3/SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* the Xbox picture: 640x480 (with "original" resolution scaling, d3d8_gl.c scales it to the drawable) */
#define XBOX_WIDTH 640
#define XBOX_HEIGHT 480

int gl_functions_load(void);

static SDL_Window *platform_window;
static SDL_GLContext platform_gl_context;
/* platform_video_initialize: 0 not tried yet, 1 the window and context are up, -1 it failed
(Direct3D then runs without a window; a second call does not retry) */
static int platform_video_state;
static SDL_ThreadID platform_event_thread;
static BOOL platform_sdl_started;

static pthread_mutex_t input_lock = PTHREAD_MUTEX_INITIALIZER;
static struct platform_input_state input_state;
/* keys and buttons pressed since the last read: a press and release between
two reads still counts */
static unsigned char keys_pressed[SDL_SCANCODE_COUNT];
static unsigned char mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_COUNT];
static BOOL screenshot_requested;
static BOOL menus_active;

#define KEYSTROKE_QUEUE_SIZE 64
static struct platform_keystroke keystroke_queue[KEYSTROKE_QUEUE_SIZE];
static unsigned long keystroke_head, keystroke_count;

BOOL platform_sdl_initialize(void)
{
	if (platform_sdl_started)
		return TRUE;
	SDL_SetHint(SDL_HINT_APP_NAME, "OpenFrenzy");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
	{
		platform_log("SDL_Init failed: %s", SDL_GetError());
		return FALSE;
	}
	platform_sdl_started = TRUE;
	input_state.focused = TRUE;
	return TRUE;
}

/* the game data is the player's disc image (vfs module); there is nothing to offer */
BOOL platform_offer_game_data(const char *destination)
{
	(void)destination;
	return FALSE;
}

/* the size the game draws at: the window's pixels (on a Retina display, two
for each point), from OpenCE's platform_screen_mode. d3d8_gl.c keeps the
picture 4:3 and puts black bars at the sides or the top and bottom. FALSE
where display.resolution_scaling is "original": the game then draws the
Xbox's 640x480, which the display blit scales up. */
BOOL platform_screen_mode(long *width, long *height)
{
	/* (the size last given, for a window that has none: minimized) */
	static long last_width, last_height;
	int pixel_width = 0, pixel_height = 0;

	if (!strcmp(config_string("display.resolution_scaling"), "original"))
		return FALSE;
	if (!platform_window)
	{
		*width = XBOX_WIDTH;
		*height = XBOX_HEIGHT;
		return TRUE;
	}
	SDL_GetWindowSizeInPixels(platform_window, &pixel_width, &pixel_height);
	if (pixel_width <= 0 || pixel_height <= 0)
	{
		*width = last_width;
		*height = last_height;
		return last_width > 0;
	}
	*width = last_width = pixel_width;
	*height = last_height = pixel_height;
	return TRUE;
}

int platform_display_resolutions(long *widths, long *heights, int maximum)
{
	(void)widths;
	(void)heights;
	(void)maximum;
	return 0;
}

int platform_window_sizes(long *widths, long *heights, int maximum)
{
	(void)widths;
	(void)heights;
	(void)maximum;
	return 0;
}

int platform_audio_devices(int recording, char (*names)[PLATFORM_AUDIO_DEVICE_NAME_SIZE], int maximum)
{
	(void)recording;
	(void)names;
	(void)maximum;
	return 0;
}

/* the system's default device; the host owns device selection */
SDL_AudioDeviceID platform_audio_device(int recording, const char *name)
{
	(void)name;
	return recording ? SDL_AUDIO_DEVICE_DEFAULT_RECORDING : SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
}

static BOOL video_initialize(unsigned long width, unsigned long height)
{
	if (!platform_sdl_initialize())
		return FALSE;
	/* ES 3.2 where the driver has it, otherwise 3.0 (ANGLE on Metal gives 3.0) */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	platform_window = SDL_CreateWindow("OpenFrenzy", (int)(width ? width * 2 : 1280),
		(int)(height ? height * 2 : 960), SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (!platform_window)
	{
		platform_log("SDL_CreateWindow failed: %s", SDL_GetError());
		return FALSE;
	}
	platform_gl_context = SDL_GL_CreateContext(platform_window);
	if (!platform_gl_context)
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		platform_gl_context = SDL_GL_CreateContext(platform_window);
	}
	if (!platform_gl_context)
	{
		platform_log("cannot create an OpenGL ES context: %s", SDL_GetError());
		return FALSE;
	}
	SDL_GL_MakeCurrent(platform_window, platform_gl_context);
	if (!gl_functions_load())
		return FALSE;
	SDL_GL_SetSwapInterval(config_boolean("display.vsync") ? 1 : 0);
	platform_event_thread = SDL_GetCurrentThreadID();
	platform_log("OpenGL %s on %s", (const char *)glGetString(GL_VERSION), (const char *)glGetString(GL_RENDERER));
	return TRUE;
}

/* Direct3D_CreateDevice (d3d8_gl.c) calls this once, on the game's main thread (bStartDisplay,
0x00042020, runs there: main 0x0002d540 calls core_init directly); that thread then pumps the
events (D3DDevice_Present). width and height are the back buffer's (640x480). */
BOOL platform_video_initialize(unsigned long width, unsigned long height)
{
	if (!platform_video_state)
	{
		platform_video_state = video_initialize(width, height) ? 1 : -1;
		if (platform_video_state < 0)
			platform_gl_context = NULL;
	}
	return platform_video_state > 0;
}

void platform_display_apply(void)
{
	if (platform_window)
		SDL_GL_SetSwapInterval(config_boolean("display.vsync") ? 1 : 0);
}

void platform_video_drawable_size(int *width, int *height)
{
	SDL_GetWindowSizeInPixels(platform_window, width, height);
}

/* Should be in window coordinates (points), which pointer events use; the guest runtime has
only SDL_GetWindowSizeInPixels (port/guest/runtime/guest_sdl.c), so on a Retina display this
is twice the points. Only d3d8_gl.c's menu pointer reads it, which this game does not use. */
void platform_video_window_size(int *width, int *height)
{
	SDL_GetWindowSizeInPixels(platform_window, width, height);
}

void platform_video_swap(void)
{
	SDL_GL_SwapWindow(platform_window);
}

void platform_mouse_capture(BOOL capture)
{
	if (platform_window)
		SDL_SetWindowRelativeMouseMode(platform_window, capture ? true : false);
}

/* ---------- keyboard queue (the XDK debug keyboard, XInputDebugGetKeystroke)

From OpenCE port/linux/src/sdl_platform.c (231708d, CC0): Windows virtual keys and
XINPUT_DEBUG_KEYSTROKE_FLAG_* (xdk_xkbd.h: ctrl 0x01, shift 0x02, alt 0x04, key up 0x40). */

/* Windows virtual key code for an SDL scancode (the Xbox debug keyboard
reports virtual keys) */
static BYTE virtual_key_from_scancode(SDL_Scancode scancode)
{
	if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
		return (BYTE)('A' + (scancode - SDL_SCANCODE_A));
	if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
		return (BYTE)('1' + (scancode - SDL_SCANCODE_1));
	if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12)
		return (BYTE)(0x70 + (scancode - SDL_SCANCODE_F1));
	if (scancode >= SDL_SCANCODE_KP_1 && scancode <= SDL_SCANCODE_KP_9)
		return (BYTE)(0x61 + (scancode - SDL_SCANCODE_KP_1));
	switch (scancode)
	{
	case SDL_SCANCODE_0: return '0';
	case SDL_SCANCODE_KP_0: return 0x60;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return 0x0d;
	case SDL_SCANCODE_ESCAPE: return 0x1b;
	case SDL_SCANCODE_BACKSPACE: return 0x08;
	case SDL_SCANCODE_TAB: return 0x09;
	case SDL_SCANCODE_SPACE: return 0x20;
	case SDL_SCANCODE_MINUS: return 0xbd;
	case SDL_SCANCODE_EQUALS: return 0xbb;
	case SDL_SCANCODE_LEFTBRACKET: return 0xdb;
	case SDL_SCANCODE_RIGHTBRACKET: return 0xdd;
	case SDL_SCANCODE_BACKSLASH: return 0xdc;
	case SDL_SCANCODE_SEMICOLON: return 0xba;
	case SDL_SCANCODE_APOSTROPHE: return 0xde;
	case SDL_SCANCODE_GRAVE: return 0xc0;
	case SDL_SCANCODE_COMMA: return 0xbc;
	case SDL_SCANCODE_PERIOD: return 0xbe;
	case SDL_SCANCODE_SLASH: return 0xbf;
	case SDL_SCANCODE_CAPSLOCK: return 0x14;
	case SDL_SCANCODE_PRINTSCREEN: return 0x2c;
	case SDL_SCANCODE_SCROLLLOCK: return 0x91;
	case SDL_SCANCODE_PAUSE: return 0x13;
	case SDL_SCANCODE_INSERT: return 0x2d;
	case SDL_SCANCODE_HOME: return 0x24;
	case SDL_SCANCODE_PAGEUP: return 0x21;
	case SDL_SCANCODE_DELETE: return 0x2e;
	case SDL_SCANCODE_END: return 0x23;
	case SDL_SCANCODE_PAGEDOWN: return 0x22;
	case SDL_SCANCODE_RIGHT: return 0x27;
	case SDL_SCANCODE_LEFT: return 0x25;
	case SDL_SCANCODE_DOWN: return 0x28;
	case SDL_SCANCODE_UP: return 0x26;
	case SDL_SCANCODE_NUMLOCKCLEAR: return 0x90;
	case SDL_SCANCODE_KP_DIVIDE: return 0x6f;
	case SDL_SCANCODE_KP_MULTIPLY: return 0x6a;
	case SDL_SCANCODE_KP_MINUS: return 0x6d;
	case SDL_SCANCODE_KP_PLUS: return 0x6b;
	case SDL_SCANCODE_KP_PERIOD: return 0x6e;
	case SDL_SCANCODE_LCTRL: return 0xa2;
	case SDL_SCANCODE_RCTRL: return 0xa3;
	case SDL_SCANCODE_LSHIFT: return 0xa0;
	case SDL_SCANCODE_RSHIFT: return 0xa1;
	case SDL_SCANCODE_LALT: return 0xa4;
	case SDL_SCANCODE_RALT: return 0xa5;
	default: return 0;
	}
}

static CHAR ascii_from_key(SDL_Keycode key, SDL_Keymod modifiers)
{
	BOOL shift = (modifiers & SDL_KMOD_SHIFT) != 0;
	static const char shifted_digits[] = ")!@#$%^&*(";

	if (key >= 'a' && key <= 'z')
		return (CHAR)((shift ^ ((modifiers & SDL_KMOD_CAPS) != 0)) ? key - 32 : key);
	if (key >= '0' && key <= '9')
		return (CHAR)(shift ? shifted_digits[key - '0'] : key);
	if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
		return '\r';
	if (key == SDLK_BACKSPACE)
		return '\b';
	if (key == SDLK_TAB)
		return '\t';
	if (key == SDLK_ESCAPE)
		return 0x1b;
	if (key >= 32 && key < 127)
	{
		if (!shift)
			return (CHAR)key;
		switch (key)
		{
		case '-': return '_';
		case '=': return '+';
		case '[': return '{';
		case ']': return '}';
		case '\\': return '|';
		case ';': return ':';
		case '\'': return '"';
		case ',': return '<';
		case '.': return '>';
		case '/': return '?';
		case '`': return '~';
		default: return (CHAR)key;
		}
	}
	return 0;
}

/* called with input_lock held; a full queue drops its oldest keystroke */
static void queue_keystroke(const SDL_KeyboardEvent *event)
{
	struct platform_keystroke *keystroke;
	BYTE flags = 0;

	if (keystroke_count == KEYSTROKE_QUEUE_SIZE)
	{
		keystroke_head = (keystroke_head + 1) % KEYSTROKE_QUEUE_SIZE;
		keystroke_count--;
	}
	keystroke = &keystroke_queue[(keystroke_head + keystroke_count) % KEYSTROKE_QUEUE_SIZE];
	if (event->mod & SDL_KMOD_CTRL) flags |= 0x01;
	if (event->mod & SDL_KMOD_SHIFT) flags |= 0x02;
	if (event->mod & SDL_KMOD_ALT) flags |= 0x04;
	if (event->mod & SDL_KMOD_CAPS) flags |= 0x08;
	if (event->mod & SDL_KMOD_NUM) flags |= 0x10;
	if (!event->down) flags |= 0x40;
	if (event->repeat) flags |= 0x80;
	keystroke->virtual_key = virtual_key_from_scancode(event->scancode);
	keystroke->ascii = event->down ? ascii_from_key(event->key, event->mod) : 0;
	keystroke->flags = flags;
	keystroke_count++;
}

BOOL platform_next_keystroke(struct platform_keystroke *keystroke)
{
	BOOL found = FALSE;

	pthread_mutex_lock(&input_lock);
	if (keystroke_count)
	{
		*keystroke = keystroke_queue[keystroke_head];
		keystroke_head = (keystroke_head + 1) % KEYSTROKE_QUEUE_SIZE;
		keystroke_count--;
		found = TRUE;
	}
	pthread_mutex_unlock(&input_lock);
	return found;
}

/* ---------- events */

/* one event into the input state (platform_pump_events; tests call it with made-up events) */
void platform_handle_event(const SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		platform_log("quit requested");
		exit(EXIT_SUCCESS);
	}
	pthread_mutex_lock(&input_lock);
	switch (event->type)
	{
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
		if (event->key.scancode < SDL_SCANCODE_COUNT)
		{
			input_state.keys[event->key.scancode] = event->key.down ? 1 : 0;
			if (event->key.down)
				keys_pressed[event->key.scancode] = 1;
		}
		queue_keystroke(&event->key);
		break;
	case SDL_EVENT_MOUSE_MOTION:
		input_state.mouse_dx += event->motion.xrel;
		input_state.mouse_dy += event->motion.yrel;
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
		if (event->button.button < PLATFORM_MOUSE_BUTTON_COUNT)
		{
			input_state.mouse_buttons[event->button.button] = event->button.down ? 1 : 0;
			if (event->button.down)
				mouse_buttons_pressed[event->button.button] = 1;
		}
		break;
	case SDL_EVENT_MOUSE_WHEEL:
		input_state.mouse_wheel += event->wheel.y;
		break;
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		if (!input_state.focused)
			platform_log("window focus gained: keyboard on");
		input_state.focused = TRUE;
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		/* no key stays held while another application has the keyboard */
		if (input_state.focused)
			platform_log("window focus lost: keyboard off, keys released");
		input_state.focused = FALSE;
		memset(input_state.keys, 0, sizeof(input_state.keys));
		memset(input_state.mouse_buttons, 0, sizeof(input_state.mouse_buttons));
		break;
	default:
		break;
	}
	pthread_mutex_unlock(&input_lock);
}

/* debug.test_keys (OPENFRENZY_DEBUG_TEST_KEYS), port-only: real SDL events for automated runs of
the keyboard path. "NAME@MS[+HOLD][,...]": key NAME (an SDL scancode name, any case: RETURN,
SPACE, ESCAPE, UP, J, KEYPAD 8 ...) goes down MS milliseconds after the first keyboard read
(the first XInputGetState of pad 1, the time base of debug.test_input) and up HOLD ms later
(default 150). FOCUS_LOST@MS / FOCUS_GAINED@MS post the window focus events. The events go
through SDL_PushEvent on the event thread, then SDL_PollEvent and platform_handle_event, as a
key press does. (integration round 5) */
#define TEST_KEY_COUNT 256
#define TEST_KEY_FOCUS_LOST (-1)
#define TEST_KEY_FOCUS_GAINED (-2)

static struct test_key
{
	int scancode; /* or TEST_KEY_FOCUS_* */
	Uint64 at, hold;
	int sent; /* 0 nothing, 1 down (or the focus event), 2 up */
	char name[24];
} test_keys[TEST_KEY_COUNT];
static int test_key_count = -1;
/* SDL_GetTicks of the first platform_input_read (0: not yet) */
static Uint64 test_keys_start;

static void test_keys_parse(void)
{
	const char *script = config_string("debug.test_keys");

	test_key_count = 0;
	while (script && *script && test_key_count < TEST_KEY_COUNT)
	{
		struct test_key *key = &test_keys[test_key_count];
		size_t length = strcspn(script, "@,");
		char *end;

		if (script[length] != '@' || length >= sizeof(key->name))
		{
			platform_log("test_keys: syntax error at '%s'; the rest is ignored", script);
			break;
		}
		memcpy(key->name, script, length);
		key->name[length] = 0;
		key->at = strtoull(script + length + 1, &end, 10);
		key->hold = *end == '+' ? strtoull(end + 1, &end, 10) : 150;
		if (!strcasecmp(key->name, "FOCUS_LOST"))
			key->scancode = TEST_KEY_FOCUS_LOST;
		else if (!strcasecmp(key->name, "FOCUS_GAINED"))
			key->scancode = TEST_KEY_FOCUS_GAINED;
		else
			key->scancode = (int)SDL_GetScancodeFromName(key->name);
		if (key->scancode == SDL_SCANCODE_UNKNOWN)
			platform_log("test_keys: unknown key '%s'", key->name);
		else
			test_key_count++;
		script = end + strcspn(end, ",");
		if (*script == ',')
			script++;
	}
}

static void test_keys_push(void)
{
	Uint64 elapsed;
	int index;

	if (test_key_count < 0)
		test_keys_parse();
	if (!test_key_count || !test_keys_start)
		return;
	elapsed = SDL_GetTicks() - test_keys_start;
	for (index = 0; index < test_key_count; index++)
	{
		struct test_key *key = &test_keys[index];
		SDL_Event event;
		BOOL down;

		if (key->sent == 2 || elapsed < key->at)
			continue;
		memset(&event, 0, sizeof(event));
		if (key->scancode < 0)
		{
			event.type = key->scancode == TEST_KEY_FOCUS_LOST ? SDL_EVENT_WINDOW_FOCUS_LOST : SDL_EVENT_WINDOW_FOCUS_GAINED;
			key->sent = 2;
			down = TRUE;
		}
		else
		{
			down = key->sent == 0;
			if (!down && elapsed < key->at + key->hold)
				continue;
			event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
			event.key.scancode = (SDL_Scancode)key->scancode;
			event.key.down = down;
			key->sent = down ? 1 : 2;
		}
		if (!SDL_PushEvent(&event))
			platform_log("test_keys: SDL_PushEvent failed: %s", SDL_GetError());
		else if (down)
			platform_log("test_keys: %s at %llu ms", key->name, (unsigned long long)elapsed);
	}
}

/* main thread only (the thread that made the window): macOS delivers window events there */
void platform_pump_events(void)
{
	SDL_Event event;

	if (!platform_sdl_started || (platform_event_thread && SDL_GetCurrentThreadID() != platform_event_thread))
		return;
	test_keys_push();
	while (SDL_PollEvent(&event))
		platform_handle_event(&event);
}

void platform_input_read(struct platform_input_state *state, BOOL consume_motion)
{
	unsigned int index;

	pthread_mutex_lock(&input_lock);
	if (!test_keys_start)
		test_keys_start = SDL_GetTicks();
	*state = input_state;
	for (index = 0; index < SDL_SCANCODE_COUNT; index++)
		state->keys[index] |= keys_pressed[index];
	for (index = 0; index < PLATFORM_MOUSE_BUTTON_COUNT; index++)
		state->mouse_buttons[index] |= mouse_buttons_pressed[index];
	memset(keys_pressed, 0, sizeof(keys_pressed));
	memset(mouse_buttons_pressed, 0, sizeof(mouse_buttons_pressed));
	state->menus = menus_active;
	if (consume_motion)
	{
		input_state.mouse_dx = 0.0f;
		input_state.mouse_dy = 0.0f;
		input_state.mouse_wheel = 0.0f;
	}
	pthread_mutex_unlock(&input_lock);
}

/* ---------- Halo's menus, pointer and scoreboard hooks: inert in this game */

void platform_menus_set_active(BOOL active)
{
	menus_active = active;
}

void platform_binding_capture_begin(void)
{
}

int platform_binding_capture_poll(int *input)
{
	(void)input;
	return 3;
}

void platform_ui_pointer_set_active(BOOL active)
{
	(void)active;
}

BOOL platform_ui_pointer_read(struct platform_ui_pointer *pointer)
{
	memset(pointer, 0, sizeof(*pointer));
	return FALSE;
}

BOOL platform_scoreboard_pointer(BOOL offered, struct platform_ui_pointer *pointer)
{
	(void)offered;
	memset(pointer, 0, sizeof(*pointer));
	return FALSE;
}

void platform_scoreboard_scroll(int open, long *notches, long *pages)
{
	(void)open;
	*notches = 0;
	*pages = 0;
}

void platform_screen_keyboard(BOOL show, BOOL password)
{
	(void)show;
	(void)password;
}

void platform_screenshot_request(void)
{
	screenshot_requested = TRUE;
}

BOOL platform_screenshot_take_request(void)
{
	BOOL requested = screenshot_requested;

	screenshot_requested = FALSE;
	return requested;
}
