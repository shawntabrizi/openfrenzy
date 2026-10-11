/*
XINPUT_MAP.C

openfrenzy input module: how the keyboard and SDL gamepads become Xbox
controller states (XINPUT_GAMEPAD), and which SDL gamepad sits on which
controller port. Pure functions: no SDL calls (only SDL's enums), so the tests
can check them directly.

New for this port. The gamepad mapping follows OpenCE port/linux/src/xinput_sdl.c
(commit 231708d, CC0): SDL's face buttons by position, the Duke's white and
black buttons on the shoulders, triggers 0..255, stick Y flipped.

Keyboard (player 1, merged with the gamepad on port 0):

	W A S D, arrows      left stick        arrows also        D-pad
	keypad 8 4 5 6       right stick       Z, keypad 0        left / right stick click
	Space, J             A                 Backspace, K       B
	F, U                 X                 R, I               Y
	C, O                 black             V, P               white
	Q, 1                 left trigger      E, 3               right trigger
	Enter, Escape        Start             Tab                Back
*/

#include "platform.h"
#include "input_xpp.h"

#include <SDL3/SDL_scancode.h>
#include <stdlib.h>
#include <string.h>

#define FULL 0xff

static int held(const unsigned char *keys, SDL_Scancode a, SDL_Scancode b)
{
	return keys[a] || (b != SDL_SCANCODE_UNKNOWN && keys[b]);
}

/* -1, 0 or 1 from a pair of keys */
static int axis(int negative, int positive)
{
	return (positive ? 1 : 0) - (negative ? 1 : 0);
}

/* full deflection; diagonals on the unit circle, as a real stick */
static void stick_from_keys(int x, int y, SHORT *out_x, SHORT *out_y)
{
	float length = (x && y) ? 0.70710678f : 1.0f;

	*out_x = (SHORT)(x * 32767 * length);
	*out_y = (SHORT)(y * 32767 * length);
}

void input_keyboard_to_pad(const unsigned char *keys, XINPUT_GAMEPAD *pad)
{
	static const struct
	{
		SDL_Scancode a, b;
		int index;
	} analog[] =
	{
		{ SDL_SCANCODE_SPACE, SDL_SCANCODE_J, XINPUT_GAMEPAD_A },
		{ SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_K, XINPUT_GAMEPAD_B },
		{ SDL_SCANCODE_F, SDL_SCANCODE_U, XINPUT_GAMEPAD_X },
		{ SDL_SCANCODE_R, SDL_SCANCODE_I, XINPUT_GAMEPAD_Y },
		{ SDL_SCANCODE_C, SDL_SCANCODE_O, XINPUT_GAMEPAD_BLACK },
		{ SDL_SCANCODE_V, SDL_SCANCODE_P, XINPUT_GAMEPAD_WHITE },
		{ SDL_SCANCODE_Q, SDL_SCANCODE_1, XINPUT_GAMEPAD_LEFT_TRIGGER },
		{ SDL_SCANCODE_E, SDL_SCANCODE_3, XINPUT_GAMEPAD_RIGHT_TRIGGER },
	};
	static const struct
	{
		SDL_Scancode a, b;
		WORD mask;
	} digital[] =
	{
		{ SDL_SCANCODE_UP, SDL_SCANCODE_UNKNOWN, XINPUT_GAMEPAD_DPAD_UP },
		{ SDL_SCANCODE_DOWN, SDL_SCANCODE_UNKNOWN, XINPUT_GAMEPAD_DPAD_DOWN },
		{ SDL_SCANCODE_LEFT, SDL_SCANCODE_UNKNOWN, XINPUT_GAMEPAD_DPAD_LEFT },
		{ SDL_SCANCODE_RIGHT, SDL_SCANCODE_UNKNOWN, XINPUT_GAMEPAD_DPAD_RIGHT },
		{ SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE, XINPUT_GAMEPAD_START },
		{ SDL_SCANCODE_TAB, SDL_SCANCODE_UNKNOWN, XINPUT_GAMEPAD_BACK },
		{ SDL_SCANCODE_Z, SDL_SCANCODE_UNKNOWN, XINPUT_GAMEPAD_LEFT_THUMB },
		{ SDL_SCANCODE_KP_0, SDL_SCANCODE_UNKNOWN, XINPUT_GAMEPAD_RIGHT_THUMB },
	};
	unsigned int index;
	int x, y;

	memset(pad, 0, sizeof(*pad));
	for (index = 0; index < sizeof(analog) / sizeof(analog[0]); index++)
	{
		if (held(keys, analog[index].a, analog[index].b))
			pad->bAnalogButtons[analog[index].index] = FULL;
	}
	for (index = 0; index < sizeof(digital) / sizeof(digital[0]); index++)
	{
		if (held(keys, digital[index].a, digital[index].b))
			pad->wButtons |= digital[index].mask;
	}
	x = axis(held(keys, SDL_SCANCODE_A, SDL_SCANCODE_LEFT), held(keys, SDL_SCANCODE_D, SDL_SCANCODE_RIGHT));
	y = axis(held(keys, SDL_SCANCODE_S, SDL_SCANCODE_DOWN), held(keys, SDL_SCANCODE_W, SDL_SCANCODE_UP));
	stick_from_keys(x, y, &pad->sThumbLX, &pad->sThumbLY);
	x = axis(keys[SDL_SCANCODE_KP_4], keys[SDL_SCANCODE_KP_6]);
	y = axis(keys[SDL_SCANCODE_KP_5], keys[SDL_SCANCODE_KP_8]);
	stick_from_keys(x, y, &pad->sThumbRX, &pad->sThumbRY);
}

/* SDL's Y axes point down, the Xbox's up. A centred stick (0) stays 0; SDL's -32768
(fully up) has no positive mirror and gives 32767. (Until round 6 this was -value - 1,
so a centred SDL stick gave LY = RY = -1: found with debug.virtual_pads.) */
static SHORT flip(short value)
{
	return (SHORT)(value == -32768 ? 32767 : -(int)value);
}

static int magnitude(int value)
{
	return value < 0 ? -value : value;
}

static void merge_axis(SHORT *pad_value, SHORT value)
{
	if (magnitude(value) > magnitude(*pad_value))
		*pad_value = value;
}

static void merge_analog(XINPUT_GAMEPAD *pad, int index, int value)
{
	if (value > 255)
		value = 255;
	if (value > pad->bAnalogButtons[index])
		pad->bAnalogButtons[index] = (BYTE)value;
}

void input_gamepad_to_pad(const struct input_pad_reading *reading, XINPUT_GAMEPAD *pad)
{
	static const struct
	{
		SDL_GamepadButton button;
		WORD mask;
	} digital[] =
	{
		{ SDL_GAMEPAD_BUTTON_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP },
		{ SDL_GAMEPAD_BUTTON_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN },
		{ SDL_GAMEPAD_BUTTON_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT },
		{ SDL_GAMEPAD_BUTTON_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT },
		{ SDL_GAMEPAD_BUTTON_START, XINPUT_GAMEPAD_START },
		{ SDL_GAMEPAD_BUTTON_BACK, XINPUT_GAMEPAD_BACK },
		{ SDL_GAMEPAD_BUTTON_LEFT_STICK, XINPUT_GAMEPAD_LEFT_THUMB },
		{ SDL_GAMEPAD_BUTTON_RIGHT_STICK, XINPUT_GAMEPAD_RIGHT_THUMB },
	};
	static const struct
	{
		SDL_GamepadButton button;
		int index;
	} analog[] =
	{
		{ SDL_GAMEPAD_BUTTON_SOUTH, XINPUT_GAMEPAD_A },
		{ SDL_GAMEPAD_BUTTON_EAST, XINPUT_GAMEPAD_B },
		{ SDL_GAMEPAD_BUTTON_WEST, XINPUT_GAMEPAD_X },
		{ SDL_GAMEPAD_BUTTON_NORTH, XINPUT_GAMEPAD_Y },
		/* the Duke's white and black buttons sit where later pads have shoulders */
		{ SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, XINPUT_GAMEPAD_WHITE },
		{ SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, XINPUT_GAMEPAD_BLACK },
	};
	unsigned int index;

	for (index = 0; index < sizeof(digital) / sizeof(digital[0]); index++)
	{
		if (reading->buttons[digital[index].button])
			pad->wButtons |= digital[index].mask;
	}
	for (index = 0; index < sizeof(analog) / sizeof(analog[0]); index++)
	{
		if (reading->buttons[analog[index].button])
			pad->bAnalogButtons[analog[index].index] = FULL;
	}
	/* triggers: SDL 0..32767 to 0..255 */
	if (reading->axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] > 0)
		merge_analog(pad, XINPUT_GAMEPAD_LEFT_TRIGGER, reading->axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] * 255 / 32767);
	if (reading->axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0)
		merge_analog(pad, XINPUT_GAMEPAD_RIGHT_TRIGGER, reading->axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] * 255 / 32767);
	merge_axis(&pad->sThumbLX, reading->axes[SDL_GAMEPAD_AXIS_LEFTX]);
	merge_axis(&pad->sThumbLY, flip(reading->axes[SDL_GAMEPAD_AXIS_LEFTY]));
	merge_axis(&pad->sThumbRX, reading->axes[SDL_GAMEPAD_AXIS_RIGHTX]);
	merge_axis(&pad->sThumbRY, flip(reading->axes[SDL_GAMEPAD_AXIS_RIGHTY]));
}

void input_assign_ports(unsigned int ports[INPUT_PORT_COUNT], const unsigned int *ids, int count)
{
	int port, index;

	/* free the ports of pads that are gone */
	for (port = 0; port < INPUT_PORT_COUNT; port++)
	{
		BOOL present = FALSE;

		for (index = 0; index < count && !present; index++)
			present = ports[port] != 0 && ids[index] == ports[port];
		if (!present)
			ports[port] = 0;
	}
	/* new pads take the lowest free port, in connection order */
	for (index = 0; index < count; index++)
	{
		BOOL assigned = FALSE;

		if (ids[index] == 0)
			continue;
		for (port = 0; port < INPUT_PORT_COUNT && !assigned; port++)
			assigned = ports[port] == ids[index];
		for (port = 0; port < INPUT_PORT_COUNT && !assigned; port++)
		{
			if (ports[port] == 0)
			{
				ports[port] = ids[index];
				assigned = TRUE;
			}
		}
	}
}

/* ---------- debug.test_input scripts (xinput_sdl.c applies them) */

int input_script_next(const char **cursor, struct input_script_press *press)
{
	const char *item = *cursor;
	size_t length;

	if (!*item)
		return 0;
	press->port = 0;
	/* "P<n>:" picks pad n (1-4); without it, pad 1 (port 0) */
	if (item[0] == 'P' && item[1] >= '1' && item[1] <= '0' + INPUT_PORT_COUNT && item[2] == ':')
	{
		press->port = item[1] - '1';
		item += 3;
	}
	length = strcspn(item, "@,");
	if (length >= sizeof(press->name) || item[length] != '@')
		return 0;
	memcpy(press->name, item, length);
	press->name[length] = 0;
	press->at = strtoll(item + length + 1, NULL, 10);
	item += strcspn(item, ",");
	if (*item == ',')
		item++;
	*cursor = item;
	return 1;
}

DWORD input_script_ports(const char *script)
{
	struct input_script_press press;
	DWORD mask = 0;

	while (script && input_script_next(&script, &press))
		mask |= 1u << press.port;
	return mask;
}

BOOL input_script_apply(const char *name, XINPUT_GAMEPAD *pad)
{
	static const struct { const char *name; int index; } analog[] =
	{
		{ "A", XINPUT_GAMEPAD_A }, { "B", XINPUT_GAMEPAD_B }, { "X", XINPUT_GAMEPAD_X },
		{ "Y", XINPUT_GAMEPAD_Y }, { "BLACK", XINPUT_GAMEPAD_BLACK },
		{ "WHITE", XINPUT_GAMEPAD_WHITE }, { "LT", XINPUT_GAMEPAD_LEFT_TRIGGER },
		{ "RT", XINPUT_GAMEPAD_RIGHT_TRIGGER },
	};
	static const struct { const char *name; WORD mask; } digital[] =
	{
		{ "UP", XINPUT_GAMEPAD_DPAD_UP }, { "DOWN", XINPUT_GAMEPAD_DPAD_DOWN },
		{ "LEFT", XINPUT_GAMEPAD_DPAD_LEFT }, { "RIGHT", XINPUT_GAMEPAD_DPAD_RIGHT },
		{ "START", XINPUT_GAMEPAD_START }, { "BACK", XINPUT_GAMEPAD_BACK },
		{ "LTHUMB", XINPUT_GAMEPAD_LEFT_THUMB }, { "RTHUMB", XINPUT_GAMEPAD_RIGHT_THUMB },
	};
	unsigned int index;

	for (index = 0; index < sizeof(digital) / sizeof(digital[0]); index++)
	{
		if (!strcmp(name, digital[index].name))
		{
			pad->wButtons |= digital[index].mask;
			return TRUE;
		}
	}
	for (index = 0; index < sizeof(analog) / sizeof(analog[0]); index++)
	{
		if (!strcmp(name, analog[index].name))
		{
			pad->bAnalogButtons[analog[index].index] = FULL;
			return TRUE;
		}
	}
	/* full stick deflection: LX+ LX- LY+ LY- RX+ RX- RY+ RY- (Xbox axes: +Y is up) */
	if ((name[0] == 'L' || name[0] == 'R') && (name[1] == 'X' || name[1] == 'Y') &&
		(name[2] == '+' || name[2] == '-') && name[3] == 0)
	{
		SHORT value = name[2] == '+' ? 32767 : -32768;

		if (name[0] == 'L')
			*(name[1] == 'X' ? &pad->sThumbLX : &pad->sThumbLY) = value;
		else
			*(name[1] == 'X' ? &pad->sThumbRX : &pad->sThumbRY) = value;
		return TRUE;
	}
	return FALSE;
}
