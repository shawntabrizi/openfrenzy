/*
bInput: input maps and channels, pad polling and rumble (the XInput side of
C:\Babel\Xbox\Src\BINPUT\input.cpp; the table gives these functions to bkernel, the rest of
input.cpp, biInit and biCreateMap, is in module engine_misc).

An input code (uint) is: bits 0-7 the control (0xb9-0xbc thumb axes LX, LY, RX, RY;
0xbd-0xc4 digital buttons; 0xc5-0xcc analog buttons), bit 8 negate, bits 16-19 the pad
(0xe: every pad, 0xf: none), bits 24-30 a scale in percent (0: 100).

The original source file is not known for certain (gap functions).
*/

#include "bkernel_core.h"

#include <math.h>
#include <string.h>

/* ---------- globals (the input module and engine_misc share some of them) */

/* 0x0028512c: the first rumble update has no previous time */
int DAT_0028512c = 1;
/* 0x00285130: port masks of XGetDevices / XGetDeviceChanges: 4 pads, then 8 memory unit
slots (top, bottom) */
const uint DAT_00285130[BI_MAX_MEMORY_UNITS] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00010000, 0x00020000, 0x00040000, 0x00080000,
};
/* 0x00285150: the first feedback update resets the feedback blocks */
int DAT_00285150 = 1;

/* 0x002cb570: rumble effects, per pad and motor */
biRumbleMotor DAT_002cb570[BI_MAX_PADS * 2];
/* 0x002cc1b0: the pads changed since the last biUpdateInput */
int DAT_002cc1b0;
/* 0x002cc1b4: when set, only this map is updated */
biMap *DAT_002cc1b4;
/* 0x002cc1b8: the circular list of maps */
biMap *DAT_002cc1b8;
/* 0x002cc1bc: rumble pause count */
int DAT_002cc1bc;
/* 0x002cc1c0: rumble time (timestamp ticks while not paused), 0x002cc1c8: last update */
ulonglong DAT_002cc1c0;
ulonglong DAT_002cc1c8;
/* 0x002cc1d0: motor speeds per pad (left, right) */
int DAT_002cc1d0[BI_MAX_PADS][2];
/* 0x002cc1f0: the pads, 0x002cc280: connected pads */
biDevice DAT_002cc1f0[BI_MAX_PADS];
int DAT_002cc280;
/* 0x002cc284: memory unit slots, 0x002cc3a4: connected memory units */
biDevice DAT_002cc284[BI_MAX_MEMORY_UNITS];
int DAT_002cc3a4;
/* 0x002cc3a8: the connected pads in port order */
biDevice *DAT_002cc3a8[BI_MAX_PADS];
/* 0x002cc3b8: tick count of the last device check */
DWORD DAT_002cc3b8;
/* 0x002cc3c0: last feedback update */
ulonglong DAT_002cc3c0;
/* 0x002cc3c8: send the feedback even when unchanged */
int DAT_002cc3c8[BI_MAX_PADS];
/* 0x002cc3d8: feedback blocks (XInputSetState completes asynchronously) */
XINPUT_FEEDBACK DAT_002cc3d8[BI_MAX_PADS];

/* ---------- maps and channels */

/* unlinks a map from the list (the head moves on, or the list empties) */
static void biUnlinkMap(biMap *map)
{
	map->next->prev = map->prev;
	map->prev->next = map->next;
	if (biMapList == map)
		biMapList = map->next == map ? NULL : map->next;
}

/* 0x00048970 */
void biDeleteMap(struct biMap *map)
{
	biUnlinkMap(map);
	bkHeapFree(map);
}

/* 0x000489b0 */
void biDeleteAllMaps(void)
{
	biMap *map;

	while ((map = biMapList) != NULL)
	{
		biUnlinkMap(map);
		bkHeapFree(map);
	}
}

/* 0x00048a10
biCreateChannel: a channel in a free slot of map; scale 0 means 0x7ff8. */
struct biChannel *biCreateChannel(struct biMap *map, const char *name, int type, uint flags,
	int scale, int acceleration, int deceleration)
{
	biChannel *channel;
	int index;

	if (map->numChannels == map->maxChannels)
	{
		bkPrintf("biCreateChannel: ERROR - Map is full\n");
		return NULL;
	}
	for (index = 0; index < map->maxChannels; index++)
	{
		if (map->channels[index].type == 0)
			break;
	}
	channel = &map->channels[index];
	strcpy(channel->name, name);
	channel->type = type;
	channel->numBindings = 0;
	channel->value = 0;
	channel->flags = flags;
	channel->map = map;
	channel->state = 0;
	channel->scale = scale != 0 ? scale : BI_INPUT_MAX;
	channel->repeatDelay = (uint)(bkTimerFrequency / 3);
	channel->repeatRate = (uint)(bkTimerFrequency / 20);
	channel->acceleration = acceleration;
	channel->deceleration = deceleration;
	map->numChannels++;
	return channel;
}

/* 0x00048b00 */
void biDeleteChannel(struct biChannel *channel)
{
	channel->type = 0;
	channel->map->numChannels--;
}

/* 0x00048b20
biBindInput: binds an input (and, for a signed axis, the input of the other direction).
Inputs for every pad (pad 0xe) are bound once per pad. */
int biBindInput(struct biChannel *channel, uint input, uint other)
{
	biBinding *binding;
	uint value;
	uint otherValue;
	int pad;

	if ((input & 0xf0000) != 0xe0000 && (other & 0xf0000) != 0xe0000)
	{
		if (channel->numBindings == BI_CHANNEL_INPUTS)
		{
			bkPrintf("biBindInput: ERROR - Channel '%s' is full\n", channel->name);
			DbgBreakPoint();
			return 0;
		}
		binding = &channel->bindings[channel->numBindings];
		binding->inputs[0] = input;
		binding->inputs[1] = other;
		binding->count = 1;
		biReadInput(input, channel->type, &value);
		biReadInput(other, channel->type, &otherValue);
		channel->numBindings++;
		channel->value = otherValue | value;
		return 1;
	}
	for (pad = 0; pad < BI_MAX_PADS; pad++)
	{
		biBindInput(channel, input != 0 ? (uint)pad << 16 | (input & 0xfff0ffff) : 0,
			other != 0 ? (uint)pad << 16 | (other & 0xfff0ffff) : 0);
	}
	return 1;
}

/* 0x00048c40
biBindMultiInput: binds count inputs that must all be held (the first gives the value). */
int biBindMultiInput(struct biChannel *channel, int count, ...)
{
	biBinding *binding;
	va_list arguments;
	uint value;
	int index;

	if (channel->numBindings == BI_CHANNEL_INPUTS)
	{
		bkPrintf("biBindMultiInput: ERROR - Channel is full\n");
		return 0;
	}
	if (count >= 7)
	{
		bkPrintf("biBindMultiInput: ERROR - Too many inputs\n");
		return 0;
	}
	binding = &channel->bindings[channel->numBindings];
	va_start(arguments, count);
	for (index = 0; index < count; index++)
	{
		binding->inputs[index] = va_arg(arguments, uint);
		biReadInput(binding->inputs[index], channel->type, &value);
		if (value != 0)
			channel->value = (int)value;
	}
	va_end(arguments);
	binding->count = count;
	if (channel->type == 3 && count == 1)
		binding->inputs[1] = 0;
	channel->numBindings++;
	return 1;
}

/* 0x00048d10 */
void biClearMap(struct biMap *map)
{
	int index;

	for (index = 0; index < map->maxChannels; index++)
		map->channels[index].value = 0;
}

/* 0x00048d40 */
void biClearChannel(struct biChannel *channel)
{
	channel->value = 0;
	channel->state = 2;
}

/* the extra inputs of a binding (from first on) that must be held; 0 when one is not */
static uint biBindingModifiers(biBinding *binding, int first)
{
	uint mask = BI_INPUT_MAX;
	uint value;
	int index;

	for (index = first; index < binding->count; index++)
	{
		if (biReadInput(binding->inputs[index], 1, &value) != 0)
			mask &= value;
	}
	return mask;
}

/* moves a channel's value toward target by its acceleration */
static void biApproach(biChannel *channel, int target)
{
	int value = channel->value;

	if (value < target)
	{
		value += channel->acceleration;
		channel->value = value;
		if (target < value)
			channel->value = target;
	}
	else
	{
		value -= channel->acceleration;
		channel->value = value;
		if (value < target)
			channel->value = target;
	}
}

/* lets a channel's value return toward 0 by its deceleration */
static void biDecelerate(biChannel *channel)
{
	int value = channel->value;
	int deceleration = channel->deceleration;

	if (value > 0)
	{
		channel->value = value - deceleration;
		if (value - deceleration < 0)
			channel->value = 0;
	}
	else if (value < 0)
	{
		channel->value = value + deceleration;
		if (value + deceleration > 0)
			channel->value = 0;
	}
}

/* the value of a digital channel (type 1) */
static void biUpdateDigital(biChannel *channel)
{
	biBinding *binding;
	uint all;
	uint value;
	int any;
	int index;
	int input;

	channel->value = 0;
	for (index = 0; index < channel->numBindings; index++)
	{
		binding = &channel->bindings[index];
		if (binding->count <= 0)
			continue;
		any = 0;
		all = BI_INPUT_MAX;
		for (input = 0; input < binding->count; input++)
		{
			if (biReadInput(binding->inputs[input], 1, &value) != 0)
			{
				any |= 1;
				all &= value;
			}
		}
		if (any != 0 && all != 0)
		{
			channel->value = channel->scale;
			return;
		}
	}
}

/* the value of an analog channel (type 2) */
static void biUpdateAnalog(biChannel *channel)
{
	biBinding *binding;
	uint value;
	int moved = 0;
	int target;
	int index;

	if (channel->acceleration == 0)
		channel->value = 0;
	for (index = 0; index < channel->numBindings; index++)
	{
		binding = &channel->bindings[index];
		if (binding->count > 1 && biBindingModifiers(binding, 1) == 0)
			break;
		if (biReadInput(binding->inputs[0], 2, &value) != 0 && value != 0)
		{
			target = (int)(channel->scale * value) / BI_INPUT_MAX;
			if (channel->acceleration == 0)
				channel->value = target;
			else
				biApproach(channel, target);
			if (binding->inputs[0] & 0x100)
				channel->value = -channel->value;
			moved = 1;
			break;
		}
	}
	if (channel->deceleration != 0 && moved == 0)
		biDecelerate(channel);
}

/* the value of a signed axis channel (type 3): one axis, or two inputs (plus, minus) */
static void biUpdateAxis(biChannel *channel)
{
	biBinding *binding;
	uint value;
	uint plus;
	uint minus;
	int moved = 0;
	int target;
	int index;

	if (channel->acceleration == 0)
		channel->value = 0;
	for (index = 0; index < channel->numBindings; index++)
	{
		binding = &channel->bindings[index];
		if (binding->count > 2 && biBindingModifiers(binding, 2) == 0)
			break;
		if (binding->inputs[1] == 0)
		{
			if (biReadInput(binding->inputs[0], 3, &value) != 0 && value != 0)
			{
				channel->value = (int)(channel->scale * value) / BI_INPUT_MAX;
				if (binding->inputs[0] & 0x100)
					channel->value = -channel->value;
				/* the original does not count this as a move: the deceleration still runs */
				break;
			}
			continue;
		}
		if (biReadInput(binding->inputs[0], 2, &plus) == 0)
			plus = 0;
		if (biReadInput(binding->inputs[1], 2, &minus) == 0)
			minus = 0;
		value = plus - minus;
		if (value == 0)
			continue;
		if (channel->acceleration == 0)
		{
			channel->value = (int)(channel->scale * value) / BI_INPUT_MAX;
		}
		else
		{
			target = (int)(channel->scale * value) / BI_INPUT_MAX;
			/* turning to the other direction: snap to 0 first */
			if ((target < 1 && channel->value > 0) || (target >= 1 && channel->value < 0))
			{
				if (channel->flags & 8)
					channel->value = 0;
			}
			biApproach(channel, target);
		}
		if (binding->inputs[0] & 0x100)
			channel->value = -channel->value;
		moved = 1;
		break;
	}
	if (channel->deceleration != 0 && moved == 0)
		biDecelerate(channel);
}

/* the once / auto-repeat behaviour of a channel with a value */
static void biUpdateRepeat(biChannel *channel)
{
	ulonglong now;
	longlong elapsed;

	if (channel->value == 0)
	{
		channel->state = 0;
		return;
	}
	if (channel->flags & 1)
	{
		/* once: only the first update reports the value */
		channel->state++;
		if (channel->state > 1)
			channel->value = 0;
		return;
	}
	if ((channel->flags & 4) == 0)
	{
		channel->state++;
		return;
	}
	switch (channel->state)
	{
	case 0:
		channel->repeatTime = bkTimerRead();
		channel->state = 1;
		break;
	case 1:
		now = bkTimerRead();
		elapsed = (longlong)bkTimerDelta(channel->repeatTime, now);
		if (elapsed < (longlong)channel->repeatDelay)
		{
			channel->value = 0;
		}
		else
		{
			channel->state = 3;
			channel->repeatTime = now;
		}
		break;
	case 3:
		now = bkTimerRead();
		elapsed = (longlong)bkTimerDelta(channel->repeatTime, now);
		if (elapsed >= (longlong)channel->repeatRate)
			channel->repeatTime = now;
		else
			channel->value = 0;
		break;
	default:
		channel->value = 0;
		break;
	}
}

/* 0x00048d60
biUpdateInput: polls the pads and updates every channel of every enabled map (or only of
the exclusive map). Raises "_PadChange" when pads came or went; returns that flag. */
int biUpdateInput(void)
{
	biMap *map;
	biChannel *channel;
	int index;
	int changed;

	biPollDevices();
	map = biMapList;
	if (map != NULL)
	{
		do
		{
			if ((map->flags & 1) == 0 && (DAT_002cc1b4 == NULL || map == DAT_002cc1b4))
			{
				for (index = 0; index < map->maxChannels; index++)
				{
					channel = &map->channels[index];
					if (channel->type == 0 || channel->numBindings == 0)
						continue;
					if (channel->type == 1)
						biUpdateDigital(channel);
					else if (channel->type == 2)
						biUpdateAnalog(channel);
					else if (channel->type == 3)
						biUpdateAxis(channel);
					biUpdateRepeat(channel);
				}
			}
			map = map->next;
		} while (map != biMapList);
	}
	changed = DAT_002cc1b0;
	if (changed != 0)
		bkGenerateEvent("_PadChange", NULL);
	DAT_002cc1b0 = 0;
	return changed;
}

/* ---------- rumble */

/* 0x00049380
biAddRumble: a rumble effect on a pad's motor (0 left, 1 right): strength (0-0x7fff), a
duration and a start delay in milliseconds, attack and decay in percent (attack -1: a
symmetric ramp), and a waveform (0 flat, 1 sine, 2 square, 3 triangle, 4 random). 1 when
added. */
int biAddRumble(int pad, int motor, int strength, int duration, int attack, int decay, int delay,
	int waveform)
{
	biRumbleMotor *slot;
	biRumbleEffect *effect;
	longlong frequency = bkTimerFrequency;
	int index;
	int release;

	if (motor >= 2 || motor >= 4 || strength == 0)
		return 0;
	slot = &DAT_002cb570[motor + pad * 2];
	if (slot->count == 8)
		return 0;
	for (index = 0; index < 8; index++)
	{
		if (slot->effects[index].strength == 0)
			break;
	}
	effect = &slot->effects[index];
	effect->strength = strength;
	effect->period = (longlong)((ulonglong)((longlong)delay * frequency) / 1000);
	effect->duration = (longlong)((ulonglong)((longlong)duration * frequency) / 1000);
	effect->start = (longlong)DAT_002cc1c0;
	effect->waveform = waveform;
	if (attack == -1)
	{
		release = (100 - decay) * 5;
		effect->attack = release;
	}
	else
	{
		effect->attack = attack * 10;
		release = (100 - attack - decay) * 10;
	}
	effect->decay = release;
	slot->count++;
	return 1;
}

/* 0x00049490
biAddRumbleList: count effects of 7 words each (motor, strength, duration, attack, decay,
delay, waveform). */
void biAddRumbleList(int pad, int count, const int *effects)
{
	for (; count > 0; count--, effects += 7)
	{
		biAddRumble(pad, effects[0], effects[1], effects[2], effects[3], effects[4], effects[5],
			effects[6]);
	}
}

/* 0x000494e0 */
int biPauseRumble(void)
{
	DAT_002cc1bc++;
	if (DAT_002cc1bc == 1)
		biStopMotors(-1);
	return DAT_002cc1bc;
}

/* 0x00049500
Returns the new pause count (eax at the original's ret; CRumble.c keeps it). */
int biResumeRumble(void)
{
	DAT_002cc1bc--;
	return DAT_002cc1bc;
}

/* 0x00049510
biClearRumble: stops and removes the effects of a pad (-1: every pad). */
void biClearRumble(int pad)
{
	biStopMotors(pad);
	if (pad == -1)
	{
		memset(DAT_002cb570, 0, sizeof(DAT_002cb570));
		return;
	}
	memset(&DAT_002cb570[pad * 2], 0, 2 * sizeof(biRumbleMotor));
}

/* 0x00049550 */
void biShutdownInput(void)
{
	biDeleteAllMaps();
	biCloseDevices();
}

/* 0x00049560
biUpdateRumble: at most 15 times a second, mixes the effects of each motor and sets the
motor speeds (sent by biUpdateFeedback). */
void biUpdateRumble(void)
{
	ulonglong now = bkTimerRead();
	ulonglong elapsed;
	ulonglong rumbleNow;
	biRumbleMotor *slot;
	biRumbleEffect *effect;
	longlong progress;
	double phase;
	int pad;
	int motor;
	int index;
	int sum;
	int level;

	if (DAT_0028512c != 0)
	{
		/* the original adds an uninitialised local here; the port adds 0 */
		elapsed = 0;
		DAT_0028512c = 0;
	}
	else
	{
		elapsed = bkTimerDelta(DAT_002cc1c8, now);
		if ((longlong)elapsed < bkTimerFrequency / 15)
			return;
	}
	DAT_002cc1c8 = now;
	if (DAT_002cc1bc != 0)
		return;
	DAT_002cc1c0 += elapsed;
	rumbleNow = DAT_002cc1c0;
	for (pad = 0; pad < BI_MAX_PADS; pad++)
	{
		for (motor = 0; motor < 2; motor++)
		{
			slot = &DAT_002cb570[motor + pad * 2];
			if (slot->count == 0)
				continue;
			sum = 0;
			for (index = 0; index < 8; index++)
			{
				effect = &slot->effects[index];
				if (effect->strength == 0)
					continue;
				if (effect->period != 0)
				{
					/* the start delay */
					if ((longlong)bkTimerDelta(effect->start, rumbleNow) < effect->period)
						continue;
					effect->period = 0;
					effect->start = (longlong)rumbleNow;
				}
				progress = (longlong)(((ulonglong)bkTimerDelta(effect->start, rumbleNow) * 1000) /
					(ulonglong)effect->duration);
				if ((int)progress >= 1000)
				{
					effect->strength = 0;
					slot->count--;
					continue;
				}
				if ((int)progress < effect->attack)
					level = effect->strength * (int)progress / effect->attack;
				else if ((int)progress > 1000 - effect->decay)
					level = (1000 - (int)progress) * effect->strength / effect->decay;
				else
					level = effect->strength;
				if (effect->waveform >= 1 && effect->waveform <= 4)
				{
					phase = fmod((double)((longlong)rumbleNow % bkTimerFrequency) /
						(double)bkTimerFrequency * 5.0, 1.0);
					switch (effect->waveform)
					{
					case 1: /* sine */
						level = (int)((double)level * fabsf((float)sin(phase * 3.1415925f)));
						break;
					case 2: /* square */
						if (phase <= 0.5)
							level = 0;
						break;
					case 3: /* triangle */
						if (phase <= 0.5)
							level = (int)((double)level * phase * 2.0);
						else
							level = (int)((1.0 - phase) * 2.0 * (double)level);
						break;
					case 4: /* random */
						DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
						level = (int)(((uint)DAT_00285630 >> 16) & 0x7fff) * level / 0x7fff;
						break;
					}
				}
				sum += level;
			}
			if (sum > 0x7fff)
				sum = 0x7fff;
			biSetMotor(pad, motor == 0 ? 2 : 3, sum * 0xffff / 0x7fff, 0);
			biSetMotor(pad, 5, sum * 0xff / 0x7fff, 0);
		}
	}
}

/* ---------- devices */

/* 0x00049990 (static)
biGetAttachedDevices: opens the pads present and counts the memory units. */
void biGetAttachedDevices(void)
{
	DWORD devices;
	int index;

	memset(DAT_002cc1f0, 0, sizeof(DAT_002cc1f0));
	memset(DAT_002cc3a8, 0, sizeof(DAT_002cc3a8));
	DAT_002cc280 = 0;
	devices = XGetDevices(XDEVICE_TYPE_GAMEPAD);
	bkPrintf("GetAttachedDevices: device mask 0x%08x\n", devices);
	for (index = 0; index < BI_MAX_PADS; index++)
	{
		DAT_002cc1f0[index].port = index;
		if ((DAT_00285130[index] & devices) == 0)
		{
			DAT_002cc1f0[index].connected = 0;
			continue;
		}
		DAT_002cc1f0[index].connected = 1;
		DAT_002cc1f0[index].handle = XInputOpen(XDEVICE_TYPE_GAMEPAD, index % 4, index > 3, NULL);
		DAT_002cc3a8[DAT_002cc280] = &DAT_002cc1f0[index];
		DAT_002cc280++;
		bkPrintf("GetAttachedDevices: Connected pad found at port %d\n", index);
	}
	devices = XGetDevices(XDEVICE_TYPE_MEMORY_UNIT);
	bkPrintf("GetAttachedDevices: memory card mask 0x%08x\n", devices);
	for (index = 0; index < BI_MAX_MEMORY_UNITS; index++)
	{
		DAT_002cc284[index].port = index;
		if ((DAT_00285130[index] & devices) == 0)
		{
			DAT_002cc284[index].connected = 0;
			continue;
		}
		DAT_002cc284[index].connected = 1;
		DAT_002cc3a4++;
		bkPrintf("GetAttachedDevices: Connected memory card found at port %d\n", index);
	}
}

/* 0x00049ac0
biCheckDeviceChanges: opens and closes pads that came and went, recounts memory units, and
raises "_MemCardChange". */
void biCheckDeviceChanges(void)
{
	DWORD insertions;
	DWORD removals;
	int index;

	if (XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals))
	{
		if (removals != 0)
		{
			for (index = 0; index < BI_MAX_PADS; index++)
			{
				if (DAT_00285130[index] & removals)
				{
					XInputClose(DAT_002cc1f0[index].handle);
					DAT_002cc1f0[index].connected = 0;
				}
			}
		}
		if (insertions != 0)
		{
			for (index = 0; index < BI_MAX_PADS; index++)
			{
				if (DAT_00285130[index] & insertions)
				{
					DAT_002cc1f0[index].handle = XInputOpen(XDEVICE_TYPE_GAMEPAD, index % 4, index > 3,
						NULL);
					DAT_002cc1f0[index].connected = 1;
				}
			}
		}
		DAT_002cc280 = 0;
		for (index = 0; index < BI_MAX_PADS; index++)
		{
			if (DAT_002cc1f0[index].connected != 0)
				DAT_002cc3a8[DAT_002cc280++] = &DAT_002cc1f0[index];
		}
		DAT_002cc1b0 = 1;
		bkPrintf("bInput: There are now %d input devices connected (add 0x%08x, remove 0x%08x)\n",
			DAT_002cc280, insertions, removals);
	}
	if (XGetDeviceChanges(XDEVICE_TYPE_MEMORY_UNIT, &insertions, &removals))
	{
		for (index = 0; index < BI_MAX_MEMORY_UNITS; index++)
		{
			if (removals != 0 && (DAT_00285130[index] & removals))
				DAT_002cc284[index].connected = 0;
		}
		for (index = 0; index < BI_MAX_MEMORY_UNITS; index++)
		{
			if (insertions != 0 && (DAT_00285130[index] & insertions))
				DAT_002cc284[index].connected = 1;
		}
		DAT_002cc3a4 = 0;
		for (index = 0; index < BI_MAX_MEMORY_UNITS; index++)
		{
			if (DAT_002cc284[index].connected != 0)
				DAT_002cc3a4++;
		}
		bkGenerateEvent("_MemCardChange", NULL);
		bkPrintf("bInput: There are now %d Memory Cards connected (add 0x%08x, remove 0x%08x)\n",
			DAT_002cc3a4, insertions, removals);
	}
}

/* 0x00049d40
bInitInput: the XInput devices (4 pads, 4 memory units) and the pads present. */
void bInitInput(void)
{
	XDEVICE_PREALLOC_TYPE types[2];

	memset(DAT_002cc1f0, 0, sizeof(DAT_002cc1f0));
	DAT_002cc280 = 0;
	memset(DAT_002cc284, 0, sizeof(DAT_002cc284));
	DAT_002cc3a4 = 0;
	memset(DAT_002cc3a8, 0, sizeof(DAT_002cc3a8));
	DAT_002cc3b8 = 0;
	types[0].DeviceType = XDEVICE_TYPE_GAMEPAD;
	types[0].dwPreallocCount = 4;
	types[1].DeviceType = XDEVICE_TYPE_MEMORY_UNIT;
	types[1].dwPreallocCount = 4;
	XInitDevices(2, types);
	biGetAttachedDevices();
	DAT_002cc3b8 = GetTickCount();
	bkPrintf("bInitInput found %d connected input devices\n", DAT_002cc280);
}

/* 0x00049db0
biCloseDevices: closes the connected pads and forgets every device. */
void biCloseDevices(void)
{
	int index;

	for (index = 0; index < DAT_002cc280; index++)
		XInputClose(DAT_002cc3a8[index]->handle);
	memset(DAT_002cc1f0, 0, sizeof(DAT_002cc1f0));
	DAT_002cc280 = 0;
	memset(DAT_002cc284, 0, sizeof(DAT_002cc284));
	DAT_002cc3a4 = 0;
	memset(DAT_002cc3a8, 0, sizeof(DAT_002cc3a8));
	DAT_002cc3b8 = 0;
}

/* 0x00049df0
biPollDevices: checks for device changes once a second and reads the connected pads. */
void biPollDevices(void)
{
	DWORD now = GetTickCount();
	int index;

	if (now - DAT_002cc3b8 > 1000)
	{
		DAT_002cc3b8 = now;
		biCheckDeviceChanges();
	}
	for (index = 0; index < DAT_002cc280; index++)
		XInputGetState(DAT_002cc3a8[index]->handle, &DAT_002cc3a8[index]->state);
}

/* a thumb axis with a dead zone of 10000, as 0 to 0x7fff */
static int biThumbValue(int axis)
{
	if (axis < 0)
	{
		if (axis < -9999)
			return (axis + 10000) * 0x7fff / 0x58ef;
		return 0;
	}
	if (axis < 10000)
		return 0;
	return (axis - 10000) * 0x7fff / 0x58ef;
}

/* 0x00049e40
biReadInput: the value of an input code (0 to 0x7ff8, signed for axes) in mode 1 (digital),
2 (analog magnitude) or 3 (signed). Returns 1 (also for a disconnected pad, with 0). */
int biReadInput(uint input, int mode, uint *value)
{
	biDevice *device;
	uint control = input & 0xff;
	uint scale;
	int axis;
	int result;

	*value = 0;
	if ((input & 0xf0000) == 0xf0000)
		return 1;
	device = &DAT_002cc1f0[((int)input >> 16) & 0xf];
	if (device->connected == 0)
		return 1;
	scale = ((int)input >> 24) & 0x7f;
	if (scale == 0)
		scale = 100;
	if (control > 0xbc && control < 0xc5)
	{
		/* digital button */
		result = (device->state.Gamepad.wButtons & (1u << ((control - 0xbd) & 0x1f))) ? BI_INPUT_MAX : 0;
		*value = (uint)((int)(result * scale) / 100);
		return 1;
	}
	if (control > 0xc4 && control < 0xcd)
	{
		/* analog button */
		uint pressure = device->state.Gamepad.bAnalogButtons[control - 0xc5];

		if (mode == 1)
			*value = pressure > 0x40 ? BI_INPUT_MAX : 0;
		else if (pressure < 0x1e)
			*value = 0;
		else
			*value = (uint)((int)((pressure - 0x1e) * BI_INPUT_MAX) / 0xe1);
		*value = (uint)((int)(*value * scale) / 100);
	}
	if (control < 0xb9 || control > 0xbc)
		return 1;
	switch (control)
	{
	case 0xb9:
		axis = device->state.Gamepad.sThumbLX;
		if (axis == -0x8000)
			axis = -0x7fff;
		break;
	case 0xba:
		axis = device->state.Gamepad.sThumbLY;
		if (axis == -0x8000)
			axis = -0x7fff;
		axis = -axis;
		break;
	case 0xbb:
		axis = device->state.Gamepad.sThumbRX;
		if (axis == -0x8000)
			axis = -0x7fff;
		break;
	default:
		axis = device->state.Gamepad.sThumbRY;
		if (axis == -0x8000)
			axis = -0x7fff;
		axis = -axis;
		break;
	}
	result = biThumbValue(axis) * BI_INPUT_MAX / 0x7fff;
	if (mode == 1)
	{
		if ((input & 0x100) == 0)
			*value = result < 0x1fff ? 0 : BI_INPUT_MAX;
		else
			*value = result <= -0x1fff ? BI_INPUT_MAX : 0;
	}
	else
	{
		if (mode == 2 && result < 0)
			result = -result;
		*value = (uint)result;
	}
	*value = (uint)((int)(*value * scale) / 100);
	return 1;
}

/* 0x0004a0d0
biGetPadPort: the port of the index-th connected pad, or -1. */
int biGetPadPort(int index)
{
	if (DAT_002cc280 <= index)
		return -1;
	return DAT_002cc3a8[index]->port;
}

/* 0x0004a0f0
biUpdateFeedback: at most 15 times a second, sends changed motor speeds to the pads
(not while a pad's previous XInputSetState is pending). */
void biUpdateFeedback(void)
{
	ulonglong now = bkTimerRead();
	int pad;

	if (DAT_00285150 != 0)
	{
		memset(DAT_002cc3d8, 0, sizeof(DAT_002cc3d8));
		memset(DAT_002cc3c8, 0, sizeof(DAT_002cc3c8));
		DAT_00285150 = 0;
		for (pad = 0; pad < BI_MAX_PADS; pad++)
			DAT_002cc3d8[pad].Header.dwStatus = 0x3e6;
	}
	else if ((longlong)(now - DAT_002cc3c0) < bkTimerFrequency / 15)
	{
		return;
	}
	DAT_002cc3c0 = now;
	for (pad = 0; pad < BI_MAX_PADS; pad++)
	{
		if (DAT_002cc1f0[pad].connected == 0 || DAT_002cc3d8[pad].Header.dwStatus == ERROR_IO_PENDING)
			continue;
		if (DAT_002cc3c8[pad] == 0 &&
			DAT_002cc3d8[pad].Rumble.wLeftMotorSpeed == (uint)DAT_002cc1d0[pad][0] &&
			DAT_002cc3d8[pad].Rumble.wRightMotorSpeed == (uint)DAT_002cc1d0[pad][1])
			continue;
		DAT_002cc3d8[pad].Rumble.wLeftMotorSpeed = (ushort)DAT_002cc1d0[pad][0];
		DAT_002cc3d8[pad].Rumble.wRightMotorSpeed = (ushort)DAT_002cc1d0[pad][1];
		DAT_002cc3c8[pad] = XInputSetState(DAT_002cc1f0[pad].handle, &DAT_002cc3d8[pad]) != ERROR_IO_PENDING;
	}
}

/* 0x0004a220
biSetMotor: what 2 left, 3 right, 4 both (speed, other); 0 when the pad is not connected. */
int biSetMotor(int pad, int what, int speed, int other)
{
	if (DAT_002cc1f0[pad].connected == 0)
		return 0;
	if (what == 2)
	{
		DAT_002cc1d0[pad][0] = speed;
	}
	else if (what == 3)
	{
		DAT_002cc1d0[pad][1] = speed;
	}
	else if (what == 4)
	{
		DAT_002cc1d0[pad][1] = other;
		DAT_002cc1d0[pad][0] = speed;
	}
	return 1;
}

/* 0x0004a290
biStopMotors: stops a pad's motors (a negative pad: every pad). */
void biStopMotors(int pad)
{
	int index;

	if (pad >= 4)
		return;
	if (pad < 0)
	{
		for (index = 0; index < BI_MAX_PADS; index++)
			biSetMotor(index, 4, 0, 0);
		return;
	}
	biSetMotor(pad, 4, 0, 0);
}
