/*
TEST_INPUT_EVENTS.C

sdl_platform.c's input state, fed with made-up SDL events through
platform_handle_event (what platform_pump_events does with each polled event).

Why it matters:
- xinput_sdl.c (input module) builds the controllers' state from
  platform_input_read: a key pressed and released between two reads must still
  count once, or quick taps are lost; a lost focus must release every key, or a
  key stays held while the player is in another application.
- XInputDebugGetKeystroke passes platform_keystroke on as XDK values: Windows
  virtual keys ('A', VK_RETURN 0x0d, VK_F1 0x70) and the flags of xdk_xkbd.h
  (shift 0x02, key up 0x40). The first version sent SDL keycodes ('a') and 1
  for key up, which XInputDebugGetKeystroke reads as ctrl.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "platform.h"
#include "sdl_platform.h"
#include "of_test.h"

static void key(SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod modifiers, BOOL down, BOOL repeat)
{
	SDL_Event event;

	memset(&event, 0, sizeof(event));
	event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
	event.key.scancode = scancode;
	event.key.key = keycode;
	event.key.mod = modifiers;
	event.key.down = down ? true : false;
	event.key.repeat = repeat ? true : false;
	platform_handle_event(&event);
}

static void drain_keystrokes(void)
{
	struct platform_keystroke keystroke;

	while (platform_next_keystroke(&keystroke))
		;
}

int main(void)
{
	static struct platform_input_state state;
	struct platform_keystroke keystroke;
	SDL_Event event;
	int index, count;

	setvbuf(stdout, NULL, _IONBF, 0);

	/* ---- keystrokes as the XDK debug keyboard reports them */
	key(SDL_SCANCODE_A, SDLK_A, SDL_KMOD_LSHIFT, TRUE, FALSE);
	OF_CHECK(platform_next_keystroke(&keystroke), "a key down queues a keystroke");
	OF_CHECK(keystroke.virtual_key == 'A', "virtual key of A is 'A' (0x%02x)", keystroke.virtual_key);
	OF_CHECK(keystroke.ascii == 'A', "shift+a gives ASCII 'A' (0x%02x)", (unsigned char)keystroke.ascii);
	OF_CHECK(keystroke.flags == XINPUT_DEBUG_KEYSTROKE_FLAG_SHIFT, "flags: shift only (0x%02x)", keystroke.flags);
	key(SDL_SCANCODE_A, SDLK_A, SDL_KMOD_NONE, FALSE, FALSE);
	OF_CHECK(platform_next_keystroke(&keystroke) && (keystroke.flags & XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP),
		"a key up carries XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP (flags 0x%02x)", keystroke.flags);
	OF_CHECK(!(keystroke.flags & XINPUT_DEBUG_KEYSTROKE_FLAG_CTRL), "a key up is not read as ctrl");
	OF_CHECK(keystroke.ascii == 0, "a key up has no ASCII");
	OF_CHECK(!platform_next_keystroke(&keystroke), "the queue is then empty");

	key(SDL_SCANCODE_RETURN, SDLK_RETURN, SDL_KMOD_NONE, TRUE, FALSE);
	key(SDL_SCANCODE_F1, SDLK_F1, SDL_KMOD_NONE, TRUE, FALSE);
	key(SDL_SCANCODE_UP, SDLK_UP, SDL_KMOD_LCTRL, TRUE, TRUE);
	platform_next_keystroke(&keystroke);
	OF_CHECK(keystroke.virtual_key == 0x0d && keystroke.ascii == '\r', "Return is VK_RETURN 0x0d, '\\r'");
	platform_next_keystroke(&keystroke);
	OF_CHECK(keystroke.virtual_key == 0x70, "F1 is VK_F1 0x70 (0x%02x)", keystroke.virtual_key);
	platform_next_keystroke(&keystroke);
	OF_CHECK(keystroke.virtual_key == 0x26, "Up is VK_UP 0x26 (0x%02x)", keystroke.virtual_key);
	OF_CHECK(keystroke.flags == (XINPUT_DEBUG_KEYSTROKE_FLAG_CTRL | 0x80), "ctrl + repeat flags (0x%02x)", keystroke.flags);

	/* a full queue keeps the newest keystrokes */
	drain_keystrokes();
	for (index = 0; index < 70; index++)
		key(SDL_SCANCODE_1 + (index % 9), SDLK_1 + (index % 9), SDL_KMOD_NONE, TRUE, FALSE);
	for (count = 0; platform_next_keystroke(&keystroke); count++)
		;
	OF_CHECK(count == 64, "the queue holds 64 keystrokes (%d)", count);
	OF_CHECK(keystroke.virtual_key == '1' + (69 % 9), "the last one kept is the newest ('%c')", keystroke.virtual_key);

	/* ---- the state xinput_sdl.c reads */
	platform_input_read(&state, TRUE);
	key(SDL_SCANCODE_SPACE, SDLK_SPACE, SDL_KMOD_NONE, TRUE, FALSE);
	key(SDL_SCANCODE_SPACE, SDLK_SPACE, SDL_KMOD_NONE, FALSE, FALSE);
	platform_input_read(&state, TRUE);
	OF_CHECK(state.keys[SDL_SCANCODE_SPACE] == 1, "a tap between two reads counts once");
	platform_input_read(&state, TRUE);
	OF_CHECK(state.keys[SDL_SCANCODE_SPACE] == 0, "and only once");

	key(SDL_SCANCODE_W, SDLK_W, SDL_KMOD_NONE, TRUE, FALSE);
	platform_input_read(&state, TRUE);
	OF_CHECK(state.keys[SDL_SCANCODE_W] == 1, "a held key reads as down");
	memset(&event, 0, sizeof(event));
	event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
	platform_handle_event(&event);
	platform_input_read(&state, TRUE);
	OF_CHECK(state.keys[SDL_SCANCODE_W] == 0 && !state.focused, "a lost focus releases every key");
	event.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
	platform_handle_event(&event);
	platform_input_read(&state, TRUE);
	OF_CHECK(state.focused, "focus comes back");

	/* mouse motion adds up until a read consumes it */
	memset(&event, 0, sizeof(event));
	event.type = SDL_EVENT_MOUSE_MOTION;
	event.motion.xrel = 3.0f;
	event.motion.yrel = -2.0f;
	platform_handle_event(&event);
	platform_handle_event(&event);
	platform_input_read(&state, FALSE);
	OF_CHECK(state.mouse_dx == 6.0f && state.mouse_dy == -4.0f, "motion adds up (%g, %g)", state.mouse_dx, state.mouse_dy);
	platform_input_read(&state, TRUE);
	OF_CHECK(state.mouse_dx == 6.0f, "a read without consume_motion keeps it");
	platform_input_read(&state, TRUE);
	OF_CHECK(state.mouse_dx == 0.0f && state.mouse_dy == 0.0f, "a consuming read resets it");

	memset(&event, 0, sizeof(event));
	event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
	event.button.button = SDL_BUTTON_LEFT;
	event.button.down = true;
	platform_handle_event(&event);
	event.type = SDL_EVENT_MOUSE_BUTTON_UP;
	event.button.down = false;
	platform_handle_event(&event);
	platform_input_read(&state, TRUE);
	OF_CHECK(state.mouse_buttons[SDL_BUTTON_LEFT] == 1, "a click between two reads counts");

	return of_test_result("test_input_events");
}
