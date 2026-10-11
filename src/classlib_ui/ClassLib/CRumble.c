/*
ClassLib/CRumble.c  (classlib_ui)
Original: C:\BlitzParty\Game\Src\ClassLib\CRumble.cpp (tags 0x00080800-0x00080aa0).

Controller rumble: named rumble effects (two motors: amplitude, duration, attack,
sustain, delay, style) with a debug tweak menu, played per player (or for all players
with player -1), scaled by distance for positioned effects. The rumble manager is the
object at 0x002ed324. Functions 0x00080100-0x00080b60 are placed here (inferred).

Manager (0x18 bytes):  0x00 tweak menu ("Rumble Tweaks"), 0x04 effect list (CLList),
                       0x10 saved rumble state, 0x14 flags (1 paused, 2 shut down)
Effect (0x4c bytes):   0x00 playing, 0x04 tweak sub-menu, 0x08 name hash,
                       0x0c left motor (on, amplitude, duration, attack, sustain, delay,
                       style: 0x0c-0x24), 0x28 right motor (0x28-0x40), 0x44 list node
*/

#include "classlib_ui.h"

#define OV_F(o, off) (*(float *)((o) + (off)))
#define OV_I(o, off) (*(int *)((o) + (off)))
#define OV_U(o, off) (*(unsigned int *)((o) + (off)))

/* 0x002ecfac  the HUD (rumble is off while its clock at +0x90 is 0) */
#define clu_hud CLU_VAR(int, 0x002ecfac)
/* 0x002ed324  the rumble manager; 0x002ed338 its flags */
#define clu_rumble CLU_ADDR(0x002ed324)
#define clu_rumble_flags CLU_VAR(unsigned int, 0x002ed338)

extern int DAT_002baeb8;     /* core: results shown */
extern int DAT_002bae7c;     /* core: rumble enabled in the options */
extern int DAT_002bae94;     /* core: player count */
extern int DAT_002baed8[40]; /* core: player present (10 ints per player) */
extern float DAT_002ed350[16]; /* per-player world positions */

/* name hash of the engine (bkernel 0x0004dc30; its table prototype returns void) */
#define CL_Hash(name) ((unsigned int (*)(const char *))FUN_0004dc30)(name)

/* 1 when rumble may play for `player` now */
static int CLRumble_Allowed(int self, int player)
{
	return (*(unsigned char *)(self + 0x14) & 1) == 0 && DAT_002baed8[player * 10] != 0 && DAT_002bae7c != 0 &&
	       OV_F(clu_hud, 0x90) != 0.0f;
}

/* 0x00080100  ClassLib/CRumble.cpp (inferred)
Frees the two motor tracks of an effect pair (objects at +0 and +4) and the buffer at
+0x10. */
void CLRumble_FreeTracks(int *self)
{
	int track;
	int object;

	track = self[1];
	if (track != 0) {
		if (OV_I(track, 4) != 0) {
			FUN_0004c110((void *)OV_I(track, 4));
			OV_I(track, 4) = 0;
		}
		object = OV_I(track, 0x10);
		if (object != 0) {
			FUN_000d2770((struct CFire_PointSprites *)object);
			FUN_0004c110((void *)object);
			OV_I(track, 0x10) = 0;
		}
		FUN_0004c110((void *)track);
	}
	track = self[0];
	if (track != 0) {
		if (OV_I(track, 8) != 0) {
			FUN_0004c110((void *)OV_I(track, 8));
			OV_I(track, 8) = 0;
		}
		object = OV_I(track, 0x14);
		if (object != 0) {
			FUN_000d2770((struct CFire_PointSprites *)object);
			FUN_0004c110((void *)object);
			OV_I(track, 0x14) = 0;
		}
		FUN_0004c110((void *)track);
	}
	FUN_0004c110((void *)self[4]);
	self[4] = 0;
}

/* 0x000801a0  ClassLib/CRumble.cpp (inferred; thiscall)
Effect constructor: both motors on, full amplitude, 400 ms, attack and sustain 50,
style 3. */
int *CLRumble_InitEffect(int *self)
{
	self[0] = 0;
	self[4] = 0x7fff;
	self[8] = 0;
	self[5] = 400;
	self[6] = 0x32;
	self[3] = 1;
	self[9] = 3;
	self[7] = 0x32;
	self[0xb] = 0x7fff;
	self[0xf] = 0;
	self[0xc] = 400;
	self[0xd] = 0x32;
	self[10] = 1;
	self[0x10] = 3;
	self[0xe] = 0x32;
	return self;
}

/* 0x00080200  ClassLib/CRumble.cpp (inferred; thiscall)
Manager constructor. */
int *CLRumble_Init(int *self)
{
	CLList_Init(self + 1);
	self[0] = 0;
	return self;
}

/* 0x00080220  ClassLib/CRumble.cpp (inferred)
Manager destructor (its list member's destructor is empty). */
void CLRumble_Destruct(void)
{
	FUN_001d5950();
}

/* 0x00080230  ClassLib/CRumble.cpp (inferred; thiscall)
Pauses rumble (saves the current state). */
void CLRumble_Pause(int self)
{
	OV_U(self, 0x14) = OV_U(self, 0x14) | 1;
	OV_I(self, 0x10) = FUN_000494e0();
}

/* 0x00080250  ClassLib/CRumble.cpp (inferred; thiscall)
Stops all rumble for good. */
void CLRumble_StopAll(int self)
{
	OV_U(self, 0x14) = OV_U(self, 0x14) | 2;
	FUN_00049510(-1);
}

/* 0x00080270  ClassLib/CRumble.cpp (inferred; thiscall)
Resumes rumble. */
void CLRumble_Resume(int self)
{
	OV_U(self, 0x14) = OV_U(self, 0x14) & 0xfffffffe;
	if (0 < OV_I(self, 0x10))
		OV_I(self, 0x10) = ((int (*)(void))FUN_00049500)();
}

/* 0x00080290  ClassLib/CRumble.cpp (inferred; thiscall)
Plays effect `effect` on player `player` (-1: all four). Attack + sustain must not be
over 100 on either motor. */
int CLRumble_Play(int self, int player, int effect)
{
	int i;

	if (effect == 0)
		return 0;
	if (player == -1) {
		for (i = 0; i < 4; i++)
			CLRumble_Play(self, i, effect);
		return 1;
	}
	if (DAT_002baeb8 != 0 || !CLRumble_Allowed(self, player))
		return 0;
	if (100 < OV_I(effect, 0x18) + OV_I(effect, 0x1c)) {
		CLU_DebugPrintf("ERROR: The combined value of the left attack and the left sustain are greater than 100.\n");
		return 0;
	}
	if (100 < OV_I(effect, 0x34) + OV_I(effect, 0x38)) {
		CLU_DebugPrintf("ERROR: The combined value of the right attack and the right sustain are greater than 100.\n");
		return 0;
	}
	if (OV_I(effect, 0xc) != 0)
		FUN_00049380(player, 0, OV_I(effect, 0x10), OV_I(effect, 0x14), OV_I(effect, 0x18), OV_I(effect, 0x1c),
			     OV_I(effect, 0x20), OV_I(effect, 0x24));
	if (OV_I(effect, 0x28) != 0)
		FUN_00049380(player, 1, OV_I(effect, 0x2c), OV_I(effect, 0x30), OV_I(effect, 0x34), OV_I(effect, 0x38),
			     OV_I(effect, 0x3c), OV_I(effect, 0x40));
	return 1;
}

/* 0x000803b0  ClassLib/CRumble.cpp (inferred; thiscall)
Plays the effect named `name` on player `player`. */
void CLRumble_PlayNamed(int self, int player, char *name)
{
	unsigned int hash;
	int node;
	int effect;

	hash = CL_Hash(name);
	effect = 0;
	for (node = (int)CLList_Next((unsigned int *)(self + 4), NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)(self + 4), (unsigned int *)node)) {
		effect = node - 0x44;
		if (*(unsigned int *)(node - 0x3c) == hash)
			break;
	}
	CLRumble_Play(self, player, effect);
}

/* 0x00080400  ClassLib/CRumble.cpp (inferred; thiscall)
Plays one motor of player `player` (-1: all four) with explicit settings. */
int CLRumble_PlayMotor(int self, int player, int motor, int amplitude, int duration, int attack, int sustain,
		       int delay, int style)
{
	int i;

	if (player == -1) {
		for (i = 0; i < 4; i++)
			CLRumble_PlayMotor(self, i, motor, amplitude, duration, attack, sustain, delay, style);
		return 1;
	}
	if (!CLRumble_Allowed(self, player))
		return 0;
	if (amplitude < 0)
		amplitude = 0;
	if (duration < 0)
		duration = 0;
	if (attack + sustain < 0x65) {
		FUN_00049380(player, motor, amplitude, duration, attack, sustain, delay, style);
		return 1;
	}
	CLU_DebugPrintf("ERROR: The combined value of the attack and the sustain are greater than 100.\n");
	return 0;
}

/* 0x000804f0  ClassLib/CRumble.cpp (inferred; thiscall)
Plays the effect named `name` for every player within `range` of `pos`, weaker with
the distance. Returns 0 when there is no such effect. */
int CLRumble_PlayAt(int self, float *pos, char *name, float range)
{
	unsigned int hash;
	int node;
	int player;
	int percent;
	double distance;

	hash = CL_Hash(name);
	for (node = (int)CLList_Next((unsigned int *)(self + 4), NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)(self + 4), (unsigned int *)node)) {
		if (*(unsigned int *)(node - 0x3c) == hash)
			break;
	}
	if (node == 0)
		return 0;
	for (player = 0; player < DAT_002bae94; player++) {
		distance = (double)FUN_00051500(&DAT_002ed350[player * 4], pos);
		if (distance <= (double)range) {
			percent = (int)(distance * (double)100.0f / (double)range);
			CLRumble_PlayMotor(self, player, OV_I(node, -0x38),
					   OV_I(node, -0x34) - (OV_I(node, -0x34) * percent) / 100, OV_I(node, -0x30),
					   OV_I(node, -0x2c), OV_I(node, -0x28), OV_I(node, -0x24), OV_I(node, -0x20));
			CLRumble_PlayMotor(self, player, OV_I(node, -0x1c),
					   OV_I(node, -0x18) - (OV_I(node, -0x18) * percent) / 100, OV_I(node, -0x14),
					   OV_I(node, -0x10), OV_I(node, -0xc), OV_I(node, -8), OV_I(node, -4));
		}
	}
	return 1;
}

/* 0x00080630  ClassLib/CRumble.cpp (inferred; thiscall)
Sets the motors of player `player` (-1: all four) directly (bkernel 0x0004a220). */
int CLRumble_SetMotors(int self, int player, undefined4 a, undefined4 b, undefined4 c)
{
	int i;

	if (player == -1) {
		for (i = 0; i < 4; i++)
			CLRumble_SetMotors(self, i, a, b, c);
		return 1;
	}
	if (!CLRumble_Allowed(self, player))
		return 0;
	FUN_0004a220(player, a, b, c);
	return 1;
}

/* 0x000806d0  ClassLib/CRumble.cpp (inferred; thiscall)
Changes a playing effect of player `player` (-1: all four; bkernel 0x00049490). */
int CLRumble_Modify(int self, int player, undefined4 a, undefined4 b)
{
	int i;

	if (player == -1) {
		for (i = 0; i < 4; i++)
			CLRumble_Modify(self, i, a, b);
		return 1;
	}
	if (!CLRumble_Allowed(self, player))
		return 0;
	FUN_00049490(player, a, (undefined4 *)b);
	return 1;
}

/* 0x00080770  ClassLib/CRumble.cpp (inferred; thiscall on the rumble manager)
Debug: plays every effect marked "playing" in the tweak menu on pad 0 and clears the
mark. The manager is the one at 0x002ed324 (callers in other modules call it without
an argument, as of_functions.tsv declares it). */
void CLRumble_TestEffects(void)
{
	int self = (int)clu_rumble;
	int node;

	for (node = (int)CLList_Next((unsigned int *)(self + 4), NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)(self + 4), (unsigned int *)node)) {
		if (OV_I(node, -0x44) == 0)
			continue;
		if (OV_I(node, -0x38) != 0)
			FUN_00049380(0, 0, OV_I(node, -0x34), OV_I(node, -0x30), OV_I(node, -0x2c), OV_I(node, -0x28),
				     OV_I(node, -0x24), OV_I(node, -0x20));
		if (OV_I(node, -0x1c) != 0)
			FUN_00049380(0, 1, OV_I(node, -0x18), OV_I(node, -0x14), OV_I(node, -0x10), OV_I(node, -0xc),
				     OV_I(node, -8), OV_I(node, -4));
		OV_I(node, -0x44) = 0;
	}
}

/* 0x00080800  ClassLib/CRumble.cpp (thiscall)
Creates the tweak sub-menu `name` of an effect under `menu`. */
void CLRumble_CreateTweaks(int self, int menu, char *name)
{
	int submenu;

	submenu = (int)CLU_Alloc(0xe8, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CRumble.cpp", 0xa3, 0x1001);
	if (submenu == 0)
		submenu = 0;
	else
		submenu = (int)FUN_0002e610((struct CTweakMenu *)submenu, (struct CTweakMenu *)menu, name);
	OV_I(self, 4) = submenu;
	FUN_0002f250((struct CTweakMenu *)submenu, (const char *)"Left Motor:", (int *)(self + 0xc), 1, 0, 1, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Amplitude", (int *)(self + 0x10), 0x80, 1, 0x7fff, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Duration", (int *)(self + 0x14), 10, 0, 100000, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Attack", (int *)(self + 0x18), 10, 0, 100000, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Sustain", (int *)(self + 0x1c), 1, 0, 100, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Delay", (int *)(self + 0x20), 10, 0, 100000, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Style", (int *)(self + 0x24), 1, 0, 4, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Right Motor:", (int *)(self + 0x28), 1, 0, 1, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Amplitude", (int *)(self + 0x2c), 0x80, 1, 0x7fff, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Duration", (int *)(self + 0x30), 10, 0, 100000, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Attack", (int *)(self + 0x34), 10, 0, 100000, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Sustain", (int *)(self + 0x38), 1, 0, 100, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Delay", (int *)(self + 0x3c), 10, 0, 100000, 0);
	FUN_0002f250((struct CTweakMenu *)OV_I(self, 4), (const char *)"Style", (int *)(self + 0x40), 1, 0, 4, 0);
	FUN_0002f340((struct CTweakMenu *)OV_I(self, 4), (const char *)"Test", (int *)self);
}

/* 0x00080a20  ClassLib/CRumble.cpp (thiscall)
Creates the "Rumble Tweaks" menu (once). */
int CLRumble_CreateMenu(int *self)
{
	int menu;

	if (*self == 0) {
		menu = (int)CLU_Alloc(0xe8, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CRumble.cpp", 0xef, 0x1001);
		if (menu == 0)
			menu = 0;
		else
			menu = (int)FUN_0002e510((struct CTweakMenu *)menu, "Rumble Tweaks");
		*self = menu;
	}
	return 1;
}

/* 0x00080aa0  ClassLib/CRumble.cpp (thiscall)
Adds an effect named `name` (default settings, with a tweak sub-menu). */
int CLRumble_AddEffect(int *self, char *name)
{
	int effect;

	effect = (int)CLU_Alloc(0x4c, "C:\\BlitzParty\\Game\\Src\\ClassLib\\CRumble.cpp", 0x16c, 0x1001);
	if (effect == 0)
		effect = 0;
	else
		effect = (int)CLRumble_InitEffect((int *)effect);
	CLRumble_CreateTweaks(effect, *self, name);
	OV_U(effect, 8) = CL_Hash(name);
	CLList_Insert(self + 1, (int *)(effect + 0x44), 0);
	return effect;
}

/* 0x00080b00  ClassLib/CRumble.cpp (inferred; thiscall)
Frees every effect. */
void CLRumble_FreeEffects(int self)
{
	int node;
	int effect;
	int previous;
	int submenu;

	node = (int)CLList_Next((unsigned int *)(self + 4), NULL);
	while (node != 0) {
		effect = node - 0x44;
		previous = CLList_Remove((int *)(self + 4), (int *)(effect + 0x44));
		submenu = OV_I(effect, 4);
		if (submenu != 0) {
			FUN_0002f3e0((struct CTweakMenu *)submenu);
			FUN_0004c110((void *)submenu);
		}
		FUN_0004c110((void *)effect);
		node = (int)CLList_Next((unsigned int *)(self + 4), (unsigned int *)previous);
	}
}

/* 0x00080b60  ClassLib/CRumble.cpp (inferred; thiscall)
Shuts rumble down: stops the motors, frees the effects and the tweak menu. */
void CLRumble_Shutdown(int *self)
{
	int menu;

	clu_rumble_flags = clu_rumble_flags | 2;
	FUN_00049510(-1);
	CLRumble_FreeEffects((int)clu_rumble);
	menu = *self;
	if (menu != 0) {
		FUN_0002f3e0((struct CTweakMenu *)menu);
		FUN_0004c110((void *)menu);
	}
	*self = 0;
}
