/*
XINPUT_TEST.C

Tests of the input module (src/input) as a guest image. Run with
src/input/tests/run_tests.sh <build dir>. Exit code: the number of failed checks.

What they protect:
	- the XDK structure layouts the game indexes by hand (bInput keeps an
	  XINPUT_STATE at +0x0c of a 36-byte port record, and writes the rumble
	  speeds at +0x42 of a 0x46-byte XINPUT_FEEDBACK);
	- the keyboard drives player 1, gamepads go to the ports the game sees;
	- a pad keeps its port when another is unplugged (four players);
	- XGetDevices / XGetDeviceChanges report what bInput needs to open and
	  close handles (0x00049990, 0x00049ac0);
	- XInputSetState returns ERROR_IO_PENDING with dwStatus completed, so the
	  game's rumble loop (0x0004a0f0) sends only changes;
	- the real SDL source works headless (no gamepads, the keyboard's port).
*/

#include "platform.h"
#include "input_xpp.h"

#include <SDL3/SDL_scancode.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition) check((condition), #condition, __LINE__)

static void check(int ok, const char *text, int line)
{
	if (!ok)
	{
		failures++;
		printf("FAIL line %d: %s\n", line, text);
	}
}

/* ---------- layouts */

_Static_assert(sizeof(void *) == 4, "guest pointers are 32-bit");
_Static_assert(sizeof(XINPUT_GAMEPAD) == 18, "XINPUT_GAMEPAD");
_Static_assert(sizeof(XINPUT_STATE) == 22, "XINPUT_STATE (pack 2)");
_Static_assert(offsetof(XINPUT_STATE, Gamepad) == 4, "XINPUT_STATE.Gamepad");
_Static_assert(offsetof(XINPUT_GAMEPAD, sThumbLX) == 10, "XINPUT_GAMEPAD.sThumbLX");
_Static_assert(offsetof(XINPUT_FEEDBACK, Rumble) == 0x42, "XINPUT_FEEDBACK.Rumble");
_Static_assert(sizeof(XINPUT_FEEDBACK) == 0x46, "XINPUT_FEEDBACK");

/* ---------- mapping */

static void test_keyboard(void)
{
	unsigned char keys[SDL_SCANCODE_COUNT];
	XINPUT_GAMEPAD pad;

	memset(keys, 0, sizeof(keys));
	input_keyboard_to_pad(keys, &pad);
	CHECK(pad.wButtons == 0 && pad.sThumbLX == 0 && pad.sThumbLY == 0 && pad.bAnalogButtons[XINPUT_GAMEPAD_A] == 0);

	keys[SDL_SCANCODE_W] = 1;
	input_keyboard_to_pad(keys, &pad);
	CHECK(pad.sThumbLY == 32767 && pad.sThumbLX == 0);

	keys[SDL_SCANCODE_D] = 1;
	input_keyboard_to_pad(keys, &pad);
	/* a diagonal stays on the unit circle */
	CHECK(pad.sThumbLY > 23000 && pad.sThumbLY < 23300 && pad.sThumbLX == pad.sThumbLY);

	memset(keys, 0, sizeof(keys));
	keys[SDL_SCANCODE_UP] = 1;
	keys[SDL_SCANCODE_SPACE] = 1;
	keys[SDL_SCANCODE_RETURN] = 1;
	keys[SDL_SCANCODE_E] = 1;
	input_keyboard_to_pad(keys, &pad);
	CHECK(pad.wButtons == (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_START));
	CHECK(pad.sThumbLY == 32767);
	CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_A] == 0xff);
	CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] == 0xff);
	CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_B] == 0);
}

static void test_gamepad(void)
{
	struct input_pad_reading reading;
	XINPUT_GAMEPAD pad;

	memset(&reading, 0, sizeof(reading));
	memset(&pad, 0, sizeof(pad));
	reading.buttons[SDL_GAMEPAD_BUTTON_SOUTH] = 1;
	reading.buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = 1;
	reading.buttons[SDL_GAMEPAD_BUTTON_START] = 1;
	reading.axes[SDL_GAMEPAD_AXIS_LEFTY] = -32768; /* SDL: pushed up */
	reading.axes[SDL_GAMEPAD_AXIS_RIGHTX] = 1000;
	reading.axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = 32767;
	input_gamepad_to_pad(&reading, &pad);
	CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_A] == 0xff);
	CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] == 0xff);
	CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] == 0);
	CHECK(pad.wButtons == XINPUT_GAMEPAD_START);
	CHECK(pad.sThumbLY == 32767); /* Xbox: up is positive */
	CHECK(pad.sThumbRX == 1000);
	CHECK(pad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] == 255);

	/* a stick pushed less than the keyboard does not override it */
	pad.sThumbLX = 32767;
	reading.axes[SDL_GAMEPAD_AXIS_LEFTX] = -500;
	input_gamepad_to_pad(&reading, &pad);
	CHECK(pad.sThumbLX == 32767);
}

static void test_ports(void)
{
	unsigned int ports[INPUT_PORT_COUNT] = { 0, 0, 0, 0 };
	unsigned int three[] = { 5, 6, 7 };
	unsigned int without_6[] = { 5, 7 };
	unsigned int added_8[] = { 5, 7, 8 };
	unsigned int five[] = { 5, 7, 8, 9, 10 };

	input_assign_ports(ports, three, 3);
	CHECK(ports[0] == 5 && ports[1] == 6 && ports[2] == 7 && ports[3] == 0);
	/* player 3 keeps port 2 when player 2 leaves */
	input_assign_ports(ports, without_6, 2);
	CHECK(ports[0] == 5 && ports[1] == 0 && ports[2] == 7 && ports[3] == 0);
	/* a new pad takes the free port */
	input_assign_ports(ports, added_8, 3);
	CHECK(ports[0] == 5 && ports[1] == 8 && ports[2] == 7 && ports[3] == 0);
	/* a fifth pad waits */
	input_assign_ports(ports, five, 5);
	CHECK(ports[0] == 5 && ports[1] == 8 && ports[2] == 7 && ports[3] == 9);
	input_assign_ports(ports, NULL, 0);
	CHECK(ports[0] == 0 && ports[1] == 0 && ports[2] == 0 && ports[3] == 0);
}

/* ---------- the XPP functions on a fake source */

static unsigned int fake_ids[8];
static int fake_count;
static struct input_pad_reading fake_readings[8];
static unsigned char fake_keys[SDL_SCANCODE_COUNT];
static unsigned int rumble_id;
static WORD rumble_left, rumble_right;
static int rumble_calls;

static int fake_list(unsigned int *ids, int capacity)
{
	int index;

	for (index = 0; index < fake_count && index < capacity; index++)
		ids[index] = fake_ids[index];
	return index;
}

static BOOL fake_read(unsigned int id, struct input_pad_reading *reading)
{
	int index;

	for (index = 0; index < fake_count; index++)
	{
		if (fake_ids[index] == id)
		{
			*reading = fake_readings[index];
			return TRUE;
		}
	}
	return FALSE;
}

static void fake_rumble(unsigned int id, WORD left, WORD right)
{
	rumble_id = id;
	rumble_left = left;
	rumble_right = right;
	rumble_calls++;
}

static BOOL fake_keyboard(unsigned char *keys)
{
	memcpy(keys, fake_keys, sizeof(fake_keys));
	return TRUE;
}

static const struct input_source fake_source = { fake_list, fake_read, fake_rumble, fake_keyboard };

static void plug(unsigned int id)
{
	memset(&fake_readings[fake_count], 0, sizeof(fake_readings[0]));
	fake_ids[fake_count++] = id;
}

static void unplug(unsigned int id)
{
	int index;

	for (index = 0; index < fake_count; index++)
	{
		if (fake_ids[index] == id)
		{
			memmove(&fake_ids[index], &fake_ids[index + 1], (fake_count - index - 1) * sizeof(fake_ids[0]));
			memmove(&fake_readings[index], &fake_readings[index + 1],
				(fake_count - index - 1) * sizeof(fake_readings[0]));
			fake_count--;
			return;
		}
	}
}

static void test_xpp(void)
{
	XDEVICE_PREALLOC_TYPE types[2] = { { XDEVICE_TYPE_GAMEPAD, 4 }, { XDEVICE_TYPE_MEMORY_UNIT, 4 } };
	DWORD insertions, removals;
	HANDLE pad0, pad1, pad2;
	XINPUT_STATE state;
	XINPUT_FEEDBACK feedback;
	DWORD packet;

	input_set_source(&fake_source);
	fake_count = 0;
	memset(fake_keys, 0, sizeof(fake_keys));

	/* bInitInput (0x00049d40) and GetAttachedDevices (0x00049990), no pads */
	XInitDevices(2, types);
	CHECK(XGetDevices(XDEVICE_TYPE_GAMEPAD) == XDEVICE_PORT0_MASK); /* the keyboard */
	CHECK(XGetDevices(XDEVICE_TYPE_MEMORY_UNIT) == 0);
	CHECK(!XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals));
	CHECK(!XGetDeviceChanges(XDEVICE_TYPE_MEMORY_UNIT, &insertions, &removals));
	pad0 = XInputOpen(XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT0, XDEVICE_NO_SLOT, NULL);
	CHECK(pad0 != NULL);
	CHECK(XInputOpen(XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT1, XDEVICE_NO_SLOT, NULL) == NULL);
	CHECK(XInputOpen(XDEVICE_TYPE_GAMEPAD, 4, XDEVICE_NO_SLOT, NULL) == NULL);

	/* the keyboard plays on port 0 */
	CHECK(XInputGetState(pad0, &state) == ERROR_SUCCESS);
	CHECK(state.Gamepad.wButtons == 0);
	packet = state.dwPacketNumber;
	fake_keys[SDL_SCANCODE_SPACE] = 1;
	CHECK(XInputGetState(pad0, &state) == ERROR_SUCCESS);
	CHECK(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] == 0xff);
	CHECK(state.dwPacketNumber == packet + 1);
	/* no change, same packet */
	CHECK(XInputGetState(pad0, &state) == ERROR_SUCCESS);
	CHECK(state.dwPacketNumber == packet + 1);

	/* two pads: the first joins the keyboard on port 0, the second is port 1 */
	plug(101);
	plug(102);
	CHECK(XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals));
	CHECK(insertions == XDEVICE_PORT1_MASK && removals == 0);
	pad1 = XInputOpen(XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT1, XDEVICE_NO_SLOT, NULL);
	CHECK(pad1 != NULL);
	fake_readings[0].buttons[SDL_GAMEPAD_BUTTON_NORTH] = 1;
	fake_readings[1].axes[SDL_GAMEPAD_AXIS_LEFTX] = 20000;
	CHECK(XInputGetState(pad0, &state) == ERROR_SUCCESS);
	CHECK(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] == 0xff); /* keyboard */
	CHECK(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] == 0xff); /* pad 101 */
	CHECK(XInputGetState(pad1, &state) == ERROR_SUCCESS);
	CHECK(state.Gamepad.sThumbLX == 20000);
	CHECK(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] == 0); /* not the keyboard's */

	/* rumble: completes at once, reports ERROR_IO_PENDING like the Xbox */
	memset(&feedback, 0, sizeof(feedback));
	feedback.Header.dwStatus = ERROR_IO_PENDING;
	feedback.Rumble.wLeftMotorSpeed = 0x1234;
	feedback.Rumble.wRightMotorSpeed = 0x5678;
	rumble_calls = 0;
	CHECK(XInputSetState(pad1, &feedback) == ERROR_IO_PENDING);
	CHECK(feedback.Header.dwStatus == ERROR_SUCCESS);
	CHECK(rumble_calls == 1 && rumble_id == 102 && rumble_left == 0x1234 && rumble_right == 0x5678);

	/* four players; pad 102 leaves, the others keep their ports */
	plug(103);
	plug(104);
	CHECK(XGetDevices(XDEVICE_TYPE_GAMEPAD) == 0xf);
	CHECK(!XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals)); /* XGetDevices reset them */
	pad2 = XInputOpen(XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT2, XDEVICE_NO_SLOT, NULL);
	fake_readings[2].buttons[SDL_GAMEPAD_BUTTON_EAST] = 1; /* pad 103 */
	unplug(102);
	CHECK(XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals));
	CHECK(insertions == 0 && removals == XDEVICE_PORT1_MASK);
	CHECK(XInputGetState(pad1, &state) == ERROR_DEVICE_NOT_CONNECTED);
	CHECK(state.Gamepad.sThumbLX == 0);
	CHECK(XInputSetState(pad1, &feedback) == ERROR_DEVICE_NOT_CONNECTED);
	CHECK(feedback.Header.dwStatus == ERROR_DEVICE_NOT_CONNECTED);
	CHECK(XInputGetState(pad2, &state) == ERROR_SUCCESS);
	CHECK(state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] == 0xff); /* still pad 103 on port 2 */
	XInputClose(pad1);

	/* a new pad on the freed port; then one replaced between two polls */
	plug(105);
	CHECK(XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals));
	CHECK(insertions == XDEVICE_PORT1_MASK && removals == 0);
	pad1 = XInputOpen(XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT1, XDEVICE_NO_SLOT, NULL);
	CHECK(pad1 != NULL);
	unplug(105);
	plug(106);
	CHECK(XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals));
	CHECK(insertions == XDEVICE_PORT1_MASK && removals == XDEVICE_PORT1_MASK);
	CHECK(XInputGetState(pad1, &state) == ERROR_DEVICE_NOT_CONNECTED); /* the old handle */

	/* the keyboard's port has no motors but accepts the call */
	rumble_calls = 0;
	unplug(101);
	CHECK(XInputSetState(pad0, &feedback) == ERROR_IO_PENDING);
	CHECK(feedback.Header.dwStatus == ERROR_SUCCESS && rumble_calls == 0);

	/* a closed handle */
	XInputClose(pad0);
	CHECK(XInputGetState(pad0, &state) == ERROR_DEVICE_NOT_CONNECTED);
	XInputClose(pad1);
	XInputClose(pad2);
}

/* ---------- the real SDL source, headless */

static void test_sdl(void)
{
	XDEVICE_PREALLOC_TYPE types[2] = { { XDEVICE_TYPE_GAMEPAD, 4 }, { XDEVICE_TYPE_MEMORY_UNIT, 4 } };
	XINPUT_STATE state;
	DWORD mask;
	HANDLE pad0;

	input_set_source(NULL);
	XInitDevices(2, types);
	mask = XGetDevices(XDEVICE_TYPE_GAMEPAD);
	printf("SDL source: gamepad port mask 0x%08lx\n", (unsigned long)mask);
	CHECK((mask & XDEVICE_PORT0_MASK) != 0);
	pad0 = XInputOpen(XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT0, XDEVICE_NO_SLOT, NULL);
	CHECK(pad0 != NULL);
	CHECK(XInputGetState(pad0, &state) == ERROR_SUCCESS);
	XInputClose(pad0);
}

int main(void)
{
	test_keyboard();
	test_gamepad();
	test_ports();
	test_xpp();
	test_sdl();
	printf("xinput_test: %d failure(s)\n", failures);
	return failures;
}
