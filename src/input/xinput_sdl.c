/*
XINPUT_SDL.C

openfrenzy input module (src/input): the Xbox peripheral library (XPP section of
the XBE, 0x0024c3ec-0x0024da0e) on SDL3 gamepads and the keyboard.

Adapted from OpenCE port/linux/src/xinput_sdl.c (commit 231708d, CC0). Kept: the
XPP entry points' shape, the device type tables, the gamepad mapping (now in
xinput_map.c), the debug keyboard queue. Left out (Halo only): mouse look,
key bindings from config.toml, the console, touch controls, split screen
on one gamepad, network test input. Added for this game: XGetDevices, sticky
ports for four players, XInputSetState's XDK completion model.

Fuzion Frenzy is a four player game. Its bInput code (bkernel module):
	0x00049d40 bInitInput      XInitDevices(gamepad x4, memory unit x4), then
	0x00049990 GetAttachedDevices: XGetDevices, XInputOpen(type, port, 0, NULL)
	0x00049ac0 (once a second)  XGetDeviceChanges: XInputClose / XInputOpen
	0x00049df0 (every frame)    XInputGetState for each open pad
	0x0004a0f0 (15 Hz)          XInputSetState (rumble), only when the motor
	                            speeds change or the last call did not return
	                            ERROR_IO_PENDING
	0x00049db0                  XInputClose for each open pad

Ports:
	- Port 0 is always connected: the keyboard (player 1, layout in
	  xinput_map.c), merged with the gamepad on port 0 when there is one.
	- SDL gamepads take the lowest free port in connection order and keep it
	  while connected (input_assign_ports), so unplugging player 2's pad does
	  not move player 3 to port 2. A fifth pad waits for a free port.
	- Memory units: none (saves go to the hard disk, xkernel).

The game may call from any thread; a mutex guards the state. SDL events are
pumped (platform_pump_events) only on the event thread; elsewhere the call
reads what the last pump saw.
*/

#include "platform.h"
#include "sdl_platform.h"
#include "input_xpp.h"

#include <SDL3/SDL.h>
#include <pthread.h>
#include <string.h>

/* port 0's device while the keyboard holds it (not an SDL id: those are
small counters) */
#define KEYBOARD_TOKEN 0xffffffffu

/* ---------- device tables: their addresses are the device types */

XPP_DEVICE_TYPE XDEVICE_TYPE_GAMEPAD_TABLE;
XPP_DEVICE_TYPE XDEVICE_TYPE_MEMORY_UNIT_TABLE;
XPP_DEVICE_TYPE XDEVICE_TYPE_DEBUG_KEYBOARD_TABLE;

/* ---------- state */

struct controller
{
	BOOL open;
	/* the device it was opened on: a new device on the port is not this one */
	unsigned int token;
	DWORD packet_number;
	XINPUT_GAMEPAD previous;
};

static pthread_mutex_t input_lock = PTHREAD_MUTEX_INITIALIZER;
static struct controller controllers[INPUT_PORT_COUNT];
static struct controller keyboard_device;
/* the SDL joystick id on each port, 0 for none */
static unsigned int port_ids[INPUT_PORT_COUNT];
/* the devices that XGetDevices / XGetDeviceChanges last reported, per port */
static unsigned int reported_tokens[INPUT_PORT_COUNT];
static BOOL reported_keyboard;

/* ---------- the SDL source */

static int sdl_list(unsigned int *ids, int capacity)
{
	SDL_JoystickID *list;
	int count = 0, index, found = 0;

	list = SDL_GetGamepads(&count);
	if (!list)
		return 0;
	for (index = 0; index < count && found < capacity; index++)
	{
		/* a gamepad must be opened before it can be read (the runtime's event
		loop does not open them) */
		if (!SDL_GetGamepadFromID(list[index]) && !SDL_OpenGamepad(list[index]))
			continue;
		ids[found++] = (unsigned int)list[index];
	}
	SDL_free(list);
	return found;
}

static BOOL sdl_read(unsigned int id, struct input_pad_reading *reading)
{
	SDL_Gamepad *gamepad = SDL_GetGamepadFromID((SDL_JoystickID)id);
	int index;

	if (!gamepad)
		return FALSE;
	for (index = 0; index < SDL_GAMEPAD_BUTTON_COUNT; index++)
		reading->buttons[index] = SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)index) ? 1 : 0;
	for (index = 0; index < SDL_GAMEPAD_AXIS_COUNT; index++)
		reading->axes[index] = SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)index);
	return TRUE;
}

static void sdl_rumble(unsigned int id, WORD left, WORD right)
{
	SDL_Gamepad *gamepad = SDL_GetGamepadFromID((SDL_JoystickID)id);

	/* the game sends a new speed only when it changes (XInputSetState), so
	the motors run until the next call; SDL's longest duration is 65 s */
	if (gamepad)
		SDL_RumbleGamepad(gamepad, left, right, 0xffff);
}

static BOOL sdl_keyboard(unsigned char *keys)
{
	static struct platform_input_state input;

	platform_input_read(&input, FALSE);
	memcpy(keys, input.keys, SDL_SCANCODE_COUNT);
	return input.focused;
}

static const struct input_source sdl_source = { sdl_list, sdl_read, sdl_rumble, sdl_keyboard };
static const struct input_source *source = &sdl_source;

static void reset_locked(void)
{
	memset(controllers, 0, sizeof(controllers));
	memset(&keyboard_device, 0, sizeof(keyboard_device));
	memset(port_ids, 0, sizeof(port_ids));
	memset(reported_tokens, 0, sizeof(reported_tokens));
	reported_keyboard = FALSE;
}

void input_set_source(const struct input_source *new_source)
{
	pthread_mutex_lock(&input_lock);
	source = new_source ? new_source : &sdl_source;
	reset_locked();
	pthread_mutex_unlock(&input_lock);
}

/* ---------- ports */

/* port 0: the keyboard (always there); ports 1-3: their pad's SDL id, 0 if none */
static unsigned int port_token(int port)
{
	return port == 0 ? KEYBOARD_TOKEN : port_ids[port];
}

/* reads the connected pads and updates the ports; call with the lock held */
static void refresh_locked(void)
{
	unsigned int ids[16];
	int count;

	if (source == &sdl_source)
		platform_pump_events();
	count = source->list(ids, (int)(sizeof(ids) / sizeof(ids[0])));
	input_assign_ports(port_ids, ids, count);
}

static DWORD connected_mask_locked(void)
{
	DWORD mask = 0;
	int port;

	for (port = 0; port < INPUT_PORT_COUNT; port++)
	{
		if (port_token(port))
			mask |= 1u << port;
	}
	return mask;
}

/* ---------- XPP */

/* 0x0024ca33 / 0x0024d379 XInitDevices */
VOID WINAPI XInitDevices(DWORD preallocation_type_count, PXDEVICE_PREALLOC_TYPE preallocation_types)
{
	(void)preallocation_type_count;
	(void)preallocation_types;
	if (source == &sdl_source)
		platform_sdl_initialize();
	pthread_mutex_lock(&input_lock);
	reset_locked();
	refresh_locked();
	pthread_mutex_unlock(&input_lock);
}

/* 0x0024d37e XGetDevices */
DWORD WINAPI XGetDevices(PXPP_DEVICE_TYPE device_type)
{
	DWORD mask = 0;
	int port;

	if (device_type != XDEVICE_TYPE_GAMEPAD)
		return 0; /* no memory units; the debug keyboard is found through XGetDeviceChanges */
	pthread_mutex_lock(&input_lock);
	refresh_locked();
	mask = connected_mask_locked();
	for (port = 0; port < INPUT_PORT_COUNT; port++)
		reported_tokens[port] = port_token(port);
	pthread_mutex_unlock(&input_lock);
	return mask;
}

/* 0x0024d3a0 XGetDeviceChanges: the ports whose device came or went since the
last XGetDevices / XGetDeviceChanges; a pad replaced by another on the same
port is both a removal and an insertion */
BOOL WINAPI XGetDeviceChanges(PXPP_DEVICE_TYPE device_type, PDWORD insertions, PDWORD removals)
{
	int port;

	*insertions = 0;
	*removals = 0;
	if (device_type == XDEVICE_TYPE_GAMEPAD)
	{
		pthread_mutex_lock(&input_lock);
		refresh_locked();
		for (port = 0; port < INPUT_PORT_COUNT; port++)
		{
			unsigned int current = port_token(port);

			if (current == reported_tokens[port])
				continue;
			if (reported_tokens[port])
				*removals |= 1u << port;
			if (current)
				*insertions |= 1u << port;
			reported_tokens[port] = current;
		}
		pthread_mutex_unlock(&input_lock);
	}
	else if (device_type == XDEVICE_TYPE_DEBUG_KEYBOARD)
	{
		pthread_mutex_lock(&input_lock);
		if (!reported_keyboard)
		{
			*insertions = 1;
			reported_keyboard = TRUE;
		}
		pthread_mutex_unlock(&input_lock);
	}
	return *insertions || *removals;
}

/* 0x0024d22d XInputOpen */
HANDLE WINAPI XInputOpen(PXPP_DEVICE_TYPE device_type, DWORD port, DWORD slot,
	PXINPUT_POLLING_PARAMETERS polling_parameters)
{
	HANDLE handle = NULL;

	(void)polling_parameters;
	pthread_mutex_lock(&input_lock);
	if (device_type == XDEVICE_TYPE_GAMEPAD && port < INPUT_PORT_COUNT && slot == XDEVICE_NO_SLOT)
	{
		refresh_locked();
		if (port_token((int)port))
		{
			memset(&controllers[port], 0, sizeof(controllers[port]));
			controllers[port].open = TRUE;
			controllers[port].token = port_token((int)port);
			handle = (HANDLE)&controllers[port];
		}
	}
	else if (device_type == XDEVICE_TYPE_DEBUG_KEYBOARD && port == 0)
	{
		keyboard_device.open = TRUE;
		handle = (HANDLE)&keyboard_device;
	}
	pthread_mutex_unlock(&input_lock);
	if (!handle)
		SetLastError(ERROR_DEVICE_NOT_CONNECTED);
	return handle;
}

/* 0x0024d2a2 XInputClose */
VOID WINAPI XInputClose(HANDLE device)
{
	struct controller *controller = (struct controller *)device;

	pthread_mutex_lock(&input_lock);
	if (controller)
		controller->open = FALSE;
	pthread_mutex_unlock(&input_lock);
}

/* the port of an open gamepad handle whose device is still there, else -1;
call with the lock held */
static int controller_port_locked(HANDLE device)
{
	int port;

	for (port = 0; port < INPUT_PORT_COUNT; port++)
	{
		if (device == (HANDLE)&controllers[port] && controllers[port].open)
			return controllers[port].token == port_token(port) ? port : -1;
	}
	return -1;
}

/* 0x0024d2ae XInputGetState: ERROR_SUCCESS, or ERROR_DEVICE_NOT_CONNECTED (state
zeroed) for a closed handle or a device gone since XInputOpen. The packet
number changes only when the state does, as on the Xbox. */
DWORD WINAPI XInputGetState(HANDLE device, PXINPUT_STATE state)
{
	struct input_pad_reading reading;
	unsigned char keys[SDL_SCANCODE_COUNT];
	XINPUT_GAMEPAD pad;
	int port;

	memset(state, 0, sizeof(*state));
	pthread_mutex_lock(&input_lock);
	refresh_locked();
	port = controller_port_locked(device);
	if (port < 0)
	{
		pthread_mutex_unlock(&input_lock);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	memset(&pad, 0, sizeof(pad));
	if (port == 0 && source->keyboard(keys))
		input_keyboard_to_pad(keys, &pad);
	if (port_ids[port] && source->read(port_ids[port], &reading))
		input_gamepad_to_pad(&reading, &pad);
	if (memcmp(&pad, &controllers[port].previous, sizeof(pad)))
	{
		controllers[port].packet_number++;
		controllers[port].previous = pad;
	}
	state->dwPacketNumber = controllers[port].packet_number;
	state->Gamepad = pad;
	pthread_mutex_unlock(&input_lock);
	return ERROR_SUCCESS;
}

/* 0x0024d31f XInputSetState: sets the motors. On the Xbox the request is
queued: the call returns ERROR_IO_PENDING and Header.dwStatus changes from
ERROR_IO_PENDING to the result when the pad has it. Here it completes at
once (dwStatus ERROR_SUCCESS, hEvent set) but still returns ERROR_IO_PENDING:
the game (0x0004a0f0) sends again every tick while the return is anything
else. */
DWORD WINAPI XInputSetState(HANDLE device, PXINPUT_FEEDBACK feedback)
{
	unsigned int id = 0;
	int port;

	if (!feedback)
		return ERROR_INVALID_PARAMETER;
	pthread_mutex_lock(&input_lock);
	refresh_locked();
	port = controller_port_locked(device);
	if (port >= 0)
		id = port_ids[port];
	if (id)
		source->rumble(id, feedback->Rumble.wLeftMotorSpeed, feedback->Rumble.wRightMotorSpeed);
	pthread_mutex_unlock(&input_lock);
	if (port < 0)
	{
		feedback->Header.dwStatus = ERROR_DEVICE_NOT_CONNECTED;
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	/* (the keyboard alone on port 0 has no motors: accepted, nothing to do) */
	feedback->Header.dwStatus = ERROR_SUCCESS;
	if (feedback->Header.hEvent)
		SetEvent(feedback->Header.hEvent);
	return ERROR_IO_PENDING;
}

/* ---------- the debug keyboard (XDK DEBUG_KEYBOARD; the game does not call it) */

DWORD WINAPI XInputDebugInitKeyboardQueue(PXINPUT_DEBUG_KEYQUEUE_PARAMETERS parameters)
{
	(void)parameters;
	return ERROR_SUCCESS;
}

DWORD WINAPI XInputDebugGetKeystroke(PXINPUT_DEBUG_KEYSTROKE keystroke)
{
	struct platform_keystroke next;

	memset(keystroke, 0, sizeof(*keystroke));
	if (!platform_next_keystroke(&next))
		return ERROR_HANDLE_EOF;
	keystroke->VirtualKey = next.virtual_key;
	keystroke->Ascii = next.ascii;
	keystroke->Flags = next.flags;
	return ERROR_SUCCESS;
}
