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

static void virtual_pads_attach_all(void);

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
	virtual_pads_attach_all();
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
	case SDL_EVENT_GAMEPAD_REMOVED:
		/* SDL keeps a removed pad's SDL_Gamepad (and the host its handle) until it is
		closed; xinput_sdl.c opens each pad it lists, so close it here (hot-plug) */
		{
			SDL_Gamepad *gamepad = SDL_GetGamepadFromID(event->gdevice.which);

			if (gamepad)
				SDL_CloseGamepad(gamepad);
			platform_log("gamepad %u removed", (unsigned)event->gdevice.which);
		}
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

/* debug.virtual_pads (OPENFRENZY_DEBUG_VIRTUAL_PADS), port-only: SDL virtual gamepads for
automated runs of the real gamepad path (SDL_GetGamepads, SDL_GetGamepadButton/Axis, hot-plug,
SDL_RumbleGamepad), with no hardware. "N[,ITEM...]": N pads (1-4) are attached when SDL starts,
so the game finds them at bInitInput. ITEM = "[P<k>:]CONTROL@MS[+HOLD]": virtual pad k (1-N,
default 1) sets CONTROL MS milliseconds after the first keyboard read (the time base of
debug.test_keys and, within a few ms, debug.test_input) and releases it HOLD ms later
(default 150). Controls:
	A B X Y BLACK WHITE START BACK UP DOWN LEFT RIGHT LTHUMB RTHUMB   buttons (as xinput_map.c:
	                            A = south, WHITE = left shoulder, BLACK = right shoulder)
	LT RT                       full trigger
	LX+ LX- LY+ LY- RX+ RX- RY+ RY-   full stick deflection, Xbox axes (LY+ is up)
	LEFTX=V LEFTY=V RIGHTX=V RIGHTY=V   an SDL axis value, -32768..32767 (SDL: +Y is down)
	LEFT_TRIGGER=V RIGHT_TRIGGER=V      an SDL trigger value, 0..32767
	UNPLUG, PLUG                detach the pad (SDL_DetachVirtualJoystick) / attach it again
	                            (a new SDL id); HOLD is not used
The events go to SDL on the event thread before SDL_PollEvent (platform_pump_events). The host
logs each rumble request a pad gets (`virtual gamepad <id>: rumble low L high H`). A pad's
release sets the control to rest even when another item still holds it. (integration round 6) */
#define VIRTUAL_PAD_MAX 4
#define VIRTUAL_PAD_ITEM_COUNT 512
/* the virtual descriptor: the 15 standard buttons (SOUTH..DPAD_RIGHT) and 6 axes, so a
joystick button / axis index is the SDL_GamepadButton / SDL_GamepadAxis */
#define VIRTUAL_PAD_BUTTONS (SDL_GAMEPAD_BUTTON_DPAD_RIGHT + 1)
#define VIRTUAL_PAD_AXES SDL_GAMEPAD_AXIS_COUNT
enum virtual_pad_kind { VIRTUAL_BUTTON, VIRTUAL_AXIS, VIRTUAL_UNPLUG, VIRTUAL_PLUG };

static struct virtual_pad
{
	SDL_JoystickID id; /* 0 while unplugged */
	SDL_Joystick *joystick;
} virtual_pads[VIRTUAL_PAD_MAX];
static int virtual_pad_count;

static struct virtual_pad_item
{
	int pad;
	int kind;
	int index;  /* button or axis */
	int value;  /* button: 1; axis: the joystick value while set */
	Uint64 at, hold;
	int sent;   /* 0 nothing, 1 set, 2 released */
	char name[24];
} virtual_pad_items[VIRTUAL_PAD_ITEM_COUNT];
static int virtual_pad_item_count;

/* an SDL gamepad axis value as the virtual joystick's raw value: SDL maps a trigger's full
joystick range -32768..32767 to 0..32767 */
static int virtual_axis_raw(int axis, int value)
{
	if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
	{
		if (value < 0)
			value = 0;
		if (value > 32767)
			value = 32767;
		return value * 2 - 32767;
	}
	return value < -32768 ? -32768 : (value > 32767 ? 32767 : value);
}

/* a released axis: sticks centred, triggers at the low end of the joystick range */
static int virtual_axis_rest(int axis)
{
	return axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER ? -32768 : 0;
}

/* the item's control from its name; FALSE if unknown */
static BOOL virtual_pad_control(struct virtual_pad_item *item)
{
	static const struct { const char *name; SDL_GamepadButton button; } buttons[] =
	{
		{ "A", SDL_GAMEPAD_BUTTON_SOUTH }, { "B", SDL_GAMEPAD_BUTTON_EAST },
		{ "X", SDL_GAMEPAD_BUTTON_WEST }, { "Y", SDL_GAMEPAD_BUTTON_NORTH },
		{ "WHITE", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER }, { "BLACK", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER },
		{ "START", SDL_GAMEPAD_BUTTON_START }, { "BACK", SDL_GAMEPAD_BUTTON_BACK },
		{ "UP", SDL_GAMEPAD_BUTTON_DPAD_UP }, { "DOWN", SDL_GAMEPAD_BUTTON_DPAD_DOWN },
		{ "LEFT", SDL_GAMEPAD_BUTTON_DPAD_LEFT }, { "RIGHT", SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
		{ "LTHUMB", SDL_GAMEPAD_BUTTON_LEFT_STICK }, { "RTHUMB", SDL_GAMEPAD_BUTTON_RIGHT_STICK },
	};
	static const struct { const char *name; SDL_GamepadAxis axis; } axes[] =
	{
		{ "LEFTX", SDL_GAMEPAD_AXIS_LEFTX }, { "LEFTY", SDL_GAMEPAD_AXIS_LEFTY },
		{ "RIGHTX", SDL_GAMEPAD_AXIS_RIGHTX }, { "RIGHTY", SDL_GAMEPAD_AXIS_RIGHTY },
		{ "LEFT_TRIGGER", SDL_GAMEPAD_AXIS_LEFT_TRIGGER }, { "RIGHT_TRIGGER", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER },
	};
	const char *name = item->name;
	const char *equals = strchr(name, '=');
	size_t length = equals ? (size_t)(equals - name) : strlen(name);
	unsigned int index;

	if (!equals)
	{
		for (index = 0; index < sizeof(buttons) / sizeof(buttons[0]); index++)
		{
			if (!strcmp(name, buttons[index].name))
			{
				item->kind = VIRTUAL_BUTTON;
				item->index = buttons[index].button;
				item->value = 1;
				return TRUE;
			}
		}
		if (!strcmp(name, "UNPLUG") || !strcmp(name, "PLUG"))
		{
			item->kind = name[0] == 'U' ? VIRTUAL_UNPLUG : VIRTUAL_PLUG;
			return TRUE;
		}
		item->kind = VIRTUAL_AXIS;
		if (!strcmp(name, "LT") || !strcmp(name, "RT"))
		{
			item->index = name[0] == 'L' ? SDL_GAMEPAD_AXIS_LEFT_TRIGGER : SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
			item->value = virtual_axis_raw(item->index, 32767);
			return TRUE;
		}
		/* Xbox stick names: +Y is up, SDL's +Y is down */
		if ((name[0] == 'L' || name[0] == 'R') && (name[1] == 'X' || name[1] == 'Y') &&
			(name[2] == '+' || name[2] == '-') && name[3] == 0)
		{
			BOOL up = name[2] == '+';

			if (name[1] == 'X')
				item->index = name[0] == 'L' ? SDL_GAMEPAD_AXIS_LEFTX : SDL_GAMEPAD_AXIS_RIGHTX;
			else
			{
				item->index = name[0] == 'L' ? SDL_GAMEPAD_AXIS_LEFTY : SDL_GAMEPAD_AXIS_RIGHTY;
				up = !up;
			}
			item->value = up ? 32767 : -32768;
			return TRUE;
		}
		return FALSE;
	}
	for (index = 0; index < sizeof(axes) / sizeof(axes[0]); index++)
	{
		if (strlen(axes[index].name) == length && !strncmp(name, axes[index].name, length))
		{
			item->kind = VIRTUAL_AXIS;
			item->index = axes[index].axis;
			item->value = virtual_axis_raw(item->index, (int)strtol(equals + 1, NULL, 10));
			return TRUE;
		}
	}
	return FALSE;
}

static void virtual_pads_parse(const char *script)
{
	char *end;

	virtual_pad_count = (int)strtol(script, &end, 10);
	if (virtual_pad_count < 0)
		virtual_pad_count = 0;
	if (virtual_pad_count > VIRTUAL_PAD_MAX)
	{
		platform_log("virtual_pads: %d pads asked, %d attached", virtual_pad_count, VIRTUAL_PAD_MAX);
		virtual_pad_count = VIRTUAL_PAD_MAX;
	}
	script = end + strspn(end, ",");
	while (*script && virtual_pad_item_count < VIRTUAL_PAD_ITEM_COUNT)
	{
		struct virtual_pad_item *item = &virtual_pad_items[virtual_pad_item_count];
		size_t length;

		memset(item, 0, sizeof(*item));
		if (script[0] == 'P' && script[1] >= '1' && script[1] <= '4' && script[2] == ':')
		{
			item->pad = script[1] - '1';
			script += 3;
		}
		length = strcspn(script, "@,");
		if (script[length] != '@' || length >= sizeof(item->name))
		{
			platform_log("virtual_pads: syntax error at '%s'; the rest is ignored", script);
			break;
		}
		memcpy(item->name, script, length);
		item->name[length] = 0;
		item->at = strtoull(script + length + 1, &end, 10);
		item->hold = *end == '+' ? strtoull(end + 1, &end, 10) : 150;
		if (item->pad >= virtual_pad_count)
			platform_log("virtual_pads: P%d:%s names a pad that is not attached", item->pad + 1, item->name);
		else if (!virtual_pad_control(item))
			platform_log("virtual_pads: unknown control '%s'", item->name);
		else
			virtual_pad_item_count++;
		script = end + strcspn(end, ",");
		if (*script == ',')
			script++;
	}
}

static void virtual_pad_attach(int pad)
{
	SDL_VirtualJoystickDesc desc;
	char name[32];
	int axis;

	/* The number comes first: SDL keeps only the first 9 characters of the name in a virtual
	   joystick's GUID, and its gamepad mapping (with the name of the first pad) is found by
	   GUID. With "OpenFrenzy virtual pad %d" all 4 pads had one GUID and the name of pad 1. */
	snprintf(name, sizeof(name), "Pad %d OpenFrenzy virtual", pad + 1);
	SDL_INIT_INTERFACE(&desc);
	desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
	desc.naxes = VIRTUAL_PAD_AXES;
	desc.nbuttons = VIRTUAL_PAD_BUTTONS;
	desc.button_mask = (1u << VIRTUAL_PAD_BUTTONS) - 1;
	desc.axis_mask = (1u << VIRTUAL_PAD_AXES) - 1;
	desc.name = name;
	virtual_pads[pad].id = SDL_AttachVirtualJoystick(&desc);
	if (!virtual_pads[pad].id)
	{
		platform_log("virtual_pads: pad %d: SDL_AttachVirtualJoystick failed: %s", pad + 1, SDL_GetError());
		return;
	}
	virtual_pads[pad].joystick = SDL_OpenJoystick(virtual_pads[pad].id);
	if (!virtual_pads[pad].joystick)
		platform_log("virtual_pads: pad %d: SDL_OpenJoystick failed: %s", pad + 1, SDL_GetError());
	for (axis = 0; axis < VIRTUAL_PAD_AXES; axis++)
		SDL_SetJoystickVirtualAxis(virtual_pads[pad].joystick, axis, (Sint16)virtual_axis_rest(axis));
	platform_log("virtual_pads: pad %d attached (SDL joystick id %u)", pad + 1, (unsigned)virtual_pads[pad].id);
}

static void virtual_pad_detach(int pad)
{
	SDL_JoystickID id = virtual_pads[pad].id;

	if (!id)
		return;
	SDL_CloseJoystick(virtual_pads[pad].joystick);
	virtual_pads[pad].joystick = NULL;
	virtual_pads[pad].id = 0;
	if (!SDL_DetachVirtualJoystick(id))
		platform_log("virtual_pads: pad %d: SDL_DetachVirtualJoystick failed: %s", pad + 1, SDL_GetError());
	else
		platform_log("virtual_pads: pad %d detached (SDL joystick id %u)", pad + 1, (unsigned)id);
}

/* platform_sdl_initialize, after SDL_Init: before the game's first XGetDevices */
static void virtual_pads_attach_all(void)
{
	const char *script = config_string("debug.virtual_pads");
	int pad;

	if (!script || !*script)
		return;
	virtual_pads_parse(script);
	for (pad = 0; pad < virtual_pad_count; pad++)
		virtual_pad_attach(pad);
}

static void virtual_pads_push(void)
{
	Uint64 elapsed;
	int index;

	if (!virtual_pad_item_count || !test_keys_start)
		return;
	elapsed = SDL_GetTicks() - test_keys_start;
	for (index = 0; index < virtual_pad_item_count; index++)
	{
		struct virtual_pad_item *item = &virtual_pad_items[index];
		struct virtual_pad *pad = &virtual_pads[item->pad];
		BOOL set;

		if (item->sent == 2 || elapsed < item->at)
			continue;
		if (item->kind == VIRTUAL_UNPLUG || item->kind == VIRTUAL_PLUG)
		{
			item->sent = 2;
			platform_log("virtual_pads: P%d:%s at %llu ms", item->pad + 1, item->name, (unsigned long long)elapsed);
			if (item->kind == VIRTUAL_UNPLUG)
				virtual_pad_detach(item->pad);
			else if (!pad->id)
				virtual_pad_attach(item->pad);
			continue;
		}
		set = item->sent == 0;
		if (!set && elapsed < item->at + item->hold)
			continue;
		item->sent = set ? 1 : 2;
		if (!pad->joystick)
		{
			if (set)
				platform_log("virtual_pads: P%d:%s at %llu ms: the pad is unplugged", item->pad + 1, item->name,
					(unsigned long long)elapsed);
			continue;
		}
		if (item->kind == VIRTUAL_BUTTON)
			SDL_SetJoystickVirtualButton(pad->joystick, item->index, set ? true : false);
		else
			SDL_SetJoystickVirtualAxis(pad->joystick, item->index,
				(Sint16)(set ? item->value : virtual_axis_rest(item->index)));
		if (set)
			platform_log("virtual_pads: P%d:%s at %llu ms", item->pad + 1, item->name, (unsigned long long)elapsed);
	}
}

/* main thread only (the thread that made the window): macOS delivers window events there */
void platform_pump_events(void)
{
	SDL_Event event;

	if (!platform_sdl_started || (platform_event_thread && SDL_GetCurrentThreadID() != platform_event_thread))
		return;
	test_keys_push();
	virtual_pads_push();
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
