/*
INPUT_XPP.H

openfrenzy input module (src/input): the XPP (Xbox peripheral) entry points the
game calls, and the seams that the module's tests use. See xinput_sdl.c.

Include after "platform.h" (or "xtl.h"), which declares the XDK types.
*/

#ifndef OPENFRENZY_INPUT_XPP_H
#define OPENFRENZY_INPUT_XPP_H

#include <SDL3/SDL_gamepad.h>

/* ---------- XPP entry points not declared by the XDK headers */

/* 0x0024d37e XGetDevices: the ports with a device of this type, as an
XDEVICE_PORTn_MASK bit mask. It also resets the change tracking of
XGetDeviceChanges for this type (XDK behaviour). The game calls it from
bInput GetAttachedDevices (0x00049990) for gamepads and memory units. */
DWORD WINAPI XGetDevices(PXPP_DEVICE_TYPE device_type);

/* ---------- mapping (xinput_map.c), pure functions */

#define INPUT_PORT_COUNT 4

/* one SDL gamepad's state, as read from SDL */
struct input_pad_reading
{
	unsigned char buttons[SDL_GAMEPAD_BUTTON_COUNT]; /* nonzero: held */
	short axes[SDL_GAMEPAD_AXIS_COUNT];              /* SDL range; triggers 0..32767 */
};

/* clears pad, then sets what the keyboard holds (keys: SDL_SCANCODE_COUNT
entries, nonzero held). The key layout is in xinput_map.c. */
void input_keyboard_to_pad(const unsigned char *keys, XINPUT_GAMEPAD *pad);

/* merges a gamepad into pad: buttons are ORed, an analog value or stick
axis replaces pad's when it is larger in magnitude */
void input_gamepad_to_pad(const struct input_pad_reading *reading, XINPUT_GAMEPAD *pad);

/* sticky port assignment: ports[] holds the SDL joystick id on each port (0:
none). Ids no longer in ids[] free their port; new ids take the lowest free
port. Ids beyond INPUT_PORT_COUNT pads wait for a free port. */
void input_assign_ports(unsigned int ports[INPUT_PORT_COUNT], const unsigned int *ids, int count);

/* ---------- debug.test_input scripts (pure; xinput_sdl.c times and applies them)

A script is "ITEM[,ITEM...]", ITEM = "[P<n>:]CONTROL@MS": pad n (1-4, default 1) holds
CONTROL for 150 ms from MS milliseconds after the first XInputGetState call. */

struct input_script_press
{
	int port;       /* 0-3 */
	char name[16];  /* the control */
	long long at;   /* ms */
};

/* the press at *cursor, and *cursor moved past it; 0 at the end or on a syntax error
(the rest of the script is then ignored) */
int input_script_next(const char **cursor, struct input_script_press *press);

/* the ports a script presses on, as an XDEVICE_PORTn_MASK bit mask */
DWORD input_script_ports(const char *script);

/* holds the control called name on pad; FALSE if there is no such control. Controls:
START BACK UP DOWN LEFT RIGHT LTHUMB RTHUMB (digital), A B X Y BLACK WHITE LT RT (full
pressure), LX+ LX- LY+ LY- RX+ RX- RY+ RY- (full stick deflection; +Y is up) */
BOOL input_script_apply(const char *name, XINPUT_GAMEPAD *pad);

/* ---------- test seams (xinput_sdl.c) */

/* where the module reads devices from; the default reads SDL and
platform_input_read. A test replaces it to drive the XPP functions without
real devices. */
struct input_source
{
	/* the connected gamepads' SDL joystick ids (nonzero), in connection order */
	int (*list)(unsigned int *ids, int capacity);
	/* the state of one; FALSE if it is gone */
	BOOL (*read)(unsigned int id, struct input_pad_reading *reading);
	void (*rumble)(unsigned int id, WORD left, WORD right);
	/* fills keys (SDL_SCANCODE_COUNT entries); FALSE if the window has no focus */
	BOOL (*keyboard)(unsigned char *keys);
};

/* NULL restores the SDL source; resets the module's device state */
void input_set_source(const struct input_source *source);

#endif
