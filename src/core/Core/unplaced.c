/*
UNPLACED.C  <-  C:\BlitzParty\Game\Src\Core\ (functions whose source file is not known)

openfrenzy core module: the game-mode stack (CGameModeStack), cheats, the CGame base
class, start-up helpers of core.cpp and main. Decompiled from orig/default.xbe
(functional, not byte-matching). Move a function to its file when the file is known.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"
#include "core_internal.h"

/* ====================================================================== CGameModeStack */

/* 0x0002b470  (Core) CGameModeStack: update every mode (vtable +0x14), unless paused */
void CGameModeStack_Update(struct CGameModeStack *self)
{
	int i;

	if (DAT_002cb550 == 0)
	{
		for (i = 0; i < self->count; i++)
			self->modes[i]->vtbl->update(self->modes[i], i == self->count - 1);
	}
}

/* 0x0002b4b0  (Core) CGameModeStack: count pauses (DAT_002ecdec += 2 while paused), then
call vtable +0x18 of every mode */
void CGameModeStack_Process(struct CGameModeStack *self)
{
	int i;

	if (DAT_002cb550 == 0)
	{
		if (g_PausedLast != 0)
			DAT_002ecdec -= 2;
	}
	else if (g_PausedLast == 0)
		DAT_002ecdec += 2;
	g_PausedLast = DAT_002cb550;
	for (i = 0; i < self->count; i++)
		self->modes[i]->vtbl->process(self->modes[i], i == self->count - 1);
}

/* 0x0002b520  (Core) CGameModeStack: vtable +0x20 of every mode, unless paused */
void CGameModeStack_PostDraw(struct CGameModeStack *self)
{
	int i;

	if (DAT_002cb550 == 0)
	{
		for (i = 0; i < self->count; i++)
			self->modes[i]->vtbl->post_draw(self->modes[i], i == self->count - 1);
	}
}

/* 0x0002b560  (Core) CGameModeStack::ShowStack */
void CGameModeStack_Show(struct CGameModeStack *self)
{
	int i;

	ext_DebugPrintf("CGameModeStack::ShowStack - stack has %d entries :\n", self->count);
	for (i = self->count - 1; i >= 0; i--)
		ext_DebugPrintf("  %d : %s\n", i, self->modes[i]->name);
	ext_DebugPrintf("--------------\n");
}

/* 0x0002b5b0  (Core) CGameModeStack: the topmost mode called NAME, or NULL */
struct CGameMode *CGameModeStack_Find(struct CGameModeStack *self, const char *name)
{
	int i;

	for (i = self->count - 1; i >= 0; i--)
	{
		if (_stricmp(self->modes[i]->name, name) == 0)
			return self->modes[i];
	}
	return NULL;
}

/* 0x0002b5f0  (Core) log the top of the stack and the Babel heap */
void CGameModeStack_ShowTop(void)
{
	int largest;
	int total = ext_HeapFree(&largest);

	ext_DebugPrintf("\nTop of stack now: %s\nBabel TOTAL FREE: %d Kb  Babel LARGEST FREE: %d Kb\n",
		g_ModeStack.modes[g_ModeStack.count - 1]->name, total / 1024, largest / 1024);
	ext_FUN_0004c300();
}

/* the top opaque mode at or below index FROM - 1 (0 when none) */
static void CGameModeStack_FindFirstDrawn(struct CGameModeStack *self)
{
	int i;

	self->first_drawn = 0;
	for (i = self->count - 1; i >= 0; i--)
	{
		if (self->modes[i]->flags & 1)
		{
			self->first_drawn = i;
			return;
		}
	}
}

/* 0x0002b640  (Core) CGameModeStack::Push. RESUME: also call the new mode's resume. */
int CGameModeStack_Push(struct CGameModeStack *self, struct CGameMode *mode, int resume)
{
	int count;

	if (self->count == CGAMEMODESTACK_MAX)
	{
		ext_DebugPrintf("CGameModeStack::Push *** ERROR: STACK OVERFLOW ***\n");
		return 0;
	}
	ext_CGameMode_OnPush(mode);
	self->modes[self->count] = mode;
	if (mode->flags & 1)
		self->first_drawn = self->count;
	if (self->count > 0)
	{
		struct CGameMode *below;

		D3D_KickOffAndWaitForIdle();
		ext_FUN_0003efa0(0x152, 0);
		below = self->modes[self->count - 1];
		below->vtbl->suspend(below);
	}
	count = ++self->count;
	mode->vtbl->enter(mode);
	if (count != self->count)
	{
		/* enter() changed the stack */
		CGameModeStack_ShowTop();
		return 0;
	}
	if (resume)
		mode->vtbl->resume(mode);
	CGameModeStack_ShowTop();
	return 1;
}

/* 0x0002b6f0  (Core) CGameModeStack::Pop. RESUME: resume the mode below. */
int CGameModeStack_Pop(struct CGameModeStack *self, int resume)
{
	struct CGameMode *top;

	D3D_KickOffAndWaitForIdle();
	ext_FUN_0003efa0(0x152, 0);
	if (self->count == 0)
	{
		ext_DebugPrintf("CGameModeStack::Pop *** ERROR: STACK UNDERFLOW ***\n");
		return 0;
	}
	self->count--;
	top = self->modes[self->count];
	top->vtbl->suspend(top);
	top = self->modes[self->count];
	top->vtbl->exit(top);
	ext_CGameMode_OnPop(top);
	CGameModeStack_FindFirstDrawn(self);
	if (self->count > 0 && resume)
	{
		struct CGameMode *below = self->modes[self->count - 1];

		below->vtbl->resume(below);
	}
	CGameModeStack_ShowTop();
	return 1;
}

/* 0x0002b790  (Core) CGameModeStack: pop down to the topmost mode called NAME (exact,
case-sensitive). Only the last pop resumes, unless RESUME. */
int CGameModeStack_PopTo(struct CGameModeStack *self, const char *name, int resume)
{
	int i;

	if (self->count == 0)
	{
		ext_DebugPrintf("CGameModeStack::Pop *** ERROR: STACK UNDERFLOW ***\n");
		return 0;
	}
	for (i = self->count - 1; i >= 0; i--)
	{
		if (strcmp(self->modes[i]->name, name) == 0)
		{
			while (i + 1 < self->count)
				CGameModeStack_Pop(self, self->count <= i + 2 ? 1 : resume);
			CGameModeStack_FindFirstDrawn(self);
			return 1;
		}
	}
	return 0;
}

/* 0x0002b870  (Core) CGameModeStack: pop everything */
void CGameModeStack_PopAll(struct CGameModeStack *self)
{
	while (self->count != 0)
		CGameModeStack_Pop(self, 0);
	self->first_drawn = 0;
}

/* 0x0002b8a0  (Core) CGameModeStack: draw (vtable +0x1c) from the top opaque mode up */
void CGameModeStack_Draw(struct CGameModeStack *self)
{
	int i;

	CGameModeStack_FindFirstDrawn(self);
	for (i = self->first_drawn; i < self->count; i++)
		self->modes[i]->vtbl->draw(self->modes[i], i == self->count - 1);
}

/* ====================================================================== cheats */

/* 0x0002b8f0  (Core) cheat input: a map with 4 buttons (codes 0xc5-0xc8) on PLAYER's pad */
void Core_CheatInputCreate(int player)
{
	int i;

	g_CheatInputLen = 0;
	memset(g_CheatInput, 0, 0x11 * sizeof(int));
	g_CheatInputMap = ext_InputMapCreate(4);
	for (i = 0; i < 4; i++)
		g_CheatButtons[i] = ext_InputMapAdd(g_CheatInputMap, "Cheat", 1, 1, 0, 0, 0);
	ext_InputBind(g_CheatButtons[0], g_PlayerPort(player) | 0xc5, 0);
	ext_InputBind(g_CheatButtons[1], g_PlayerPort(player) | 0xc6, 0);
	ext_InputBind(g_CheatButtons[2], g_PlayerPort(player) | 199, 0);
	ext_InputBind(g_CheatButtons[3], g_PlayerPort(player) | 200, 0);
}

/* 0x0002b9d0  (Core) */
void Core_CheatInputFree(void)
{
	ext_InputMapFree(g_CheatInputMap);
}

/* the localised name of cheat K (text table entry 0x1a6 + K) */
static const char *Core_CheatText(int index)
{
	return *(const char **)((char *)*(void **)((char *)DAT_002ecc74 + 0x20) + index * 0x10);
}

/* 0x0002b9e0  (Core) message for a cheat with several states */
void Core_CheatMessageState(int cheat, int state)
{
	int value;
	const uint16_t *format;
	const char *text;

	if (g_Cheats[cheat].states == 2 || state == 0)
	{
		if (cheat == CORE_CHEAT_DEBUG)
		{
			g_CheatMessageTime = 60.0f;
			return;
		}
		text = Core_CheatText(0x1a6 + cheat);
		value = (int)(uintptr_t)Core_CheatText(state == 0 ? 0x1ad : 0x1ac);  /* "OFF" / "ON" */
		format = u"%S %S";
	}
	else
	{
		text = Core_CheatText(0x1a6 + cheat);
		value = state;
		format = u"%S %d";
	}
	ext_WSprintf(g_CheatMessage, format, text, value);
	g_CheatMessageTime = 60.0f;
}

/* 0x0002ba70  (Core) set the colour matrix of bone NAME of an actor: colour set SET
(0: white), entry INDEX of the table at 0x00283f48 */
void Core_SetBoneColour(void *actor, const char *name, int index, int set)
{
	float *node;

	if (set == 0)
	{
		g_BoneColour[0] = 1.0f;
		g_BoneColour[5] = 1.0f;
		g_BoneColour[10] = 1.0f;
	}
	else
	{
		const float *rgb = g_BoneColours + (set * 0x11 - 0x11 + index) * 4;

		g_BoneColour[0] = rgb[0];
		g_BoneColour[5] = rgb[1];
		g_BoneColour[10] = rgb[2];
	}
	node = ((float *(*)(void *, const char *))FUN_00035b90)(*(void **)((char *)actor + 0x134), name);
	if (node)
		memcpy((char *)node + 0x100, g_BoneColour, 16 * sizeof(float));
}

/* 0x0002bb00  (Core) colour every bone of a character with colour set SET */
void Core_SetCharacterColours(void *actor, int set)
{
	Core_SetBoneColour(actor, "root", 0, set);
	Core_SetBoneColour(actor, "heart", 1, set);
	Core_SetBoneColour(actor, "rhip", 2, set);
	Core_SetBoneColour(actor, "lhip", 2, set);
	Core_SetBoneColour(actor, "rnee", 3, set);
	Core_SetBoneColour(actor, "lnee", 3, set);
	Core_SetBoneColour(actor, "rankle", 4, set);
	Core_SetBoneColour(actor, "lankle", 4, set);
	Core_SetBoneColour(actor, "guts", 5, set);
	Core_SetBoneColour(actor, "tits", 6, set);
	Core_SetBoneColour(actor, "collar", 7, set);
	Core_SetBoneColour(actor, "neck", 8, set);
	Core_SetBoneColour(actor, "headroot", 9, set);
	Core_SetBoneColour(actor, "chin", 10, set);
	Core_SetBoneColour(actor, "lshoulder", 0xb, set);
	Core_SetBoneColour(actor, "rshoulder", 0xb, set);
	Core_SetBoneColour(actor, "lelbow", 0xc, set);
	Core_SetBoneColour(actor, "relbow", 0xc, set);
	Core_SetBoneColour(actor, "lwrist", 0xd, set);
	Core_SetBoneColour(actor, "rwrist", 0xd, set);
	Core_SetBoneColour(actor, "lpalm", 0xe, set);
	Core_SetBoneColour(actor, "rpalm", 0xe, set);
	Core_SetBoneColour(actor, "lfingers", 0xf, set);
	Core_SetBoneColour(actor, "rfingers", 0xf, set);
	Core_SetBoneColour(actor, "lthumb", 0x10, set);
	Core_SetBoneColour(actor, "rthumb", 0x10, set);
}

/* 0x0002bc90  (Core) switch cheat K on */
void Core_CheatOn(int cheat)
{
	g_Cheats[cheat].state = 1;
	if (g_Cheats[cheat].states == 2)
	{
		if (cheat != CORE_CHEAT_DEBUG)
			ext_WSprintf(g_CheatMessage, u"%S %S", Core_CheatText(0x1a6 + cheat), Core_CheatText(0x1ac));
	}
	else
		ext_WSprintf(g_CheatMessage, u"%S %d", Core_CheatText(0x1a6 + cheat), 1);
	g_CheatMessageTime = 60.0f;
	if (cheat == CORE_CHEAT_DEBUG)
		ext_LogSelect(g_DebugLog);
}

/* 0x0002bd20  (Core) switch cheat K off */
void Core_CheatOff(int cheat)
{
	g_Cheats[cheat].state = 0;
	if (cheat != CORE_CHEAT_DEBUG)
		ext_WSprintf(g_CheatMessage, u"%S %S", Core_CheatText(0x1a6 + cheat), Core_CheatText(0x1ad));
	g_CheatMessageTime = 60.0f;
	if (cheat == CORE_CHEAT_DEBUG)
		ext_LogSelect(g_Log2);
}

/* 0x0002bd90  (Core) a cheat sequence was entered: toggle or step cheat K */
void Core_CheatToggle(int cheat)
{
	g_CheatInputLen = 0;
	memset(g_CheatInput, 0, 0x11 * sizeof(int));
	ext_PlaySfx(5, 0x7f, 0, 0, 0);
	if (g_Cheats[cheat].states != 2)
	{
		g_Cheats[cheat].state = (g_Cheats[cheat].state + 1) % g_Cheats[cheat].states;
		Core_CheatMessageState(cheat, g_Cheats[cheat].state);
		return;
	}
	if (g_Cheats[cheat].state != 0)
		Core_CheatOff(cheat);
	else
		Core_CheatOn(cheat);
}

/* 0x0002be10  (Core) per frame: record a pressed cheat button, then test the sequences */
void Core_CheatUpdate(void)
{
	static const int codes[4] = { 0xc5, 0xc6, 0xc7, 0xc8 };
	int button, cheat, matched;

	for (button = 0; button < 4; button++)
	{
		if (*(int *)((char *)g_CheatButtons[button] + 0x14) != 0)
		{
			if (g_CheatInputLen < 0x10)
				g_CheatInput[g_CheatInputLen++] = codes[button];
			else
			{
				memset(g_CheatInput, 0, 0x11 * sizeof(int));
				g_CheatInputLen = 0;
			}
			break;
		}
	}
	for (cheat = 0; cheat < CORE_CHEAT_COUNT; cheat++)
	{
		int *sequence = g_CheatSequences + cheat * 0x13;

		for (matched = 0; matched < g_CheatInputLen; matched++)
		{
			if (g_CheatInput[matched] != sequence[matched])
				break;
		}
		if (sequence[matched] == 0)
			Core_CheatToggle(cheat);
	}
}

/* ====================================================================== CGame base class */

/* the CGame base vtable 0x00256b18 */
static const void *const vtable_00256b18[] = {
	(const void *)FUN_0002bf70, /* destroy */
	(const void *)FUN_000a6fa0, /* init (a shared function) */
	(const void *)FUN_001d5950, /* shutdown (empty) */
	(const void *)FUN_000f8ab0, /* start (a shared function) */
};

/* 0x0002bf10  (Core) CGame::CGame(name) */
void CGame_Ctor(unsigned int *self, const char *name)
{
	self[0] = (uint32_t)(uintptr_t)vtable_00256b18;
	self[1] = (uint32_t)(uintptr_t)name;
}

/* 0x0002bf30  (Core) CGame::CGame(name, a, b, c, d) */
void CGame_Ctor6(unsigned int *self, const char *name, unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
	self[1] = (uint32_t)(uintptr_t)name;
	self[2] = a;
	self[3] = b;
	self[0] = (uint32_t)(uintptr_t)vtable_00256b18;
	self[4] = c;
	self[5] = d;
}

/* 0x0002bf60  (Core) CGame::~CGame */
void CGame_Dtor(unsigned int *self)
{
	self[0] = (uint32_t)(uintptr_t)vtable_00256b18;
}

/* 0x0002bf70  (Core) CGame scalar deleting destructor */
unsigned int *CGame_Destroy(unsigned int *self, int flags)
{
	self[0] = (uint32_t)(uintptr_t)vtable_00256b18;
	if (flags & 1)
		ext_Delete(self);
	return self;
}

/* 0x0002bf90  (Core) Schlick's gain curve of T (0-1) with gain G */
float Core_Gain(float gain, float t)
{
	double exponent = log((double)(1.0f - gain)) / log(0.5);

	if (t < 0.5f)
		return (float)(pow((double)t + (double)t, exponent) * 0.5);
	return (float)(1.0 - pow(2.0 - ((double)t + (double)t), exponent) * 0.5);
}

/* ====================================================================== start-up (core.cpp) */

/* 0x0002c020  (Core) controllers: pause when a player's pad is unplugged (DAT_002ecdec += 20),
resume when it comes back */
void Core_UpdatePads(void)
{
	int ports[4];
	int i;

	g_PadChangedPort = -1;
	if (g_PadsChannel != NULL && ext_ChannelReceive(g_PadsChannel, NULL))
		g_PadsChanged = 1;
	g_PadsChanged |= ext_PadsChanged();
	if (g_PadsChanged != 0 || DAT_002ecdec > 0x13)
	{
		if (g_PadsPrimary == -1 && g_PadsChanged != 0xffffffff)
			g_PadsPrimary = 0;
		if (g_PadsChanged != 0 && g_MusicOn != 0)
		{
			DAT_002ecde8 = DAT_002909b0;
			g_FlagBAAD8 = 1;
		}
		if (g_PadsPrimary < 1)
		{
			if (g_PadsPrimary == 0)
			{
				if (DAT_002ecdec < 0x14)
				{
					if (ext_PadPort(0) == -1)
					{
						if (DAT_002ecdec == 0)
							ext_PauseStart(DAT_002ecd90);
						DAT_002ecdec += 0x14;
					}
				}
				else if (ext_PadPort(0) != -1)
				{
					DAT_002ecdec -= 0x14;
					if (DAT_002ecdec == 0)
						ext_PauseResume(DAT_002ecd90);
				}
			}
		}
		else
		{
			ports[0] = ports[1] = ports[2] = ports[3] = -1;
			for (i = 0; i < 4; i++)
			{
				int port = ext_PadPort(i);

				if (port != -1)
					ports[port] = port;
			}
			if (DAT_002ecdec > 0x13)
			{
				if (g_PlayerPresent(0) != 0 && ports[0] != -1 && g_PadLastPort[0] == -1)
				{
					DAT_002ecdec -= 0x14;
					g_PadChangedPort = 0;
				}
				if (g_PlayerPresent(1) != 0 && ports[1] != -1 && g_PadLastPort[1] == -1)
				{
					DAT_002ecdec -= 0x14;
					g_PadChangedPort = 1;
				}
				if (g_PlayerPresent(2) != 0 && ports[2] != -1 && g_PadLastPort[2] == -1)
				{
					DAT_002ecdec -= 0x14;
					g_PadChangedPort = 2;
				}
				if (g_PlayerPresent(3) != 0 && ports[3] != -1 && g_PadLastPort[3] == -1)
				{
					DAT_002ecdec -= 0x14;
					g_PadChangedPort = 3;
				}
				if (DAT_002ecdec == 0)
					ext_PauseResume(DAT_002ecd90);
			}
			for (i = 0; i < 4; i++)
			{
				if (g_PadLastPort[i] != -1 && ports[i] == -1 && g_PlayerPresent(i) != 0)
				{
					if (DAT_002ecdec == 0)
						ext_PauseStart(DAT_002ecd90);
					DAT_002ecdec += 0x14;
				}
			}
		}
		g_PadLastPort[0] = ports[0];
		g_PadLastPort[1] = ports[1];
		g_PadLastPort[2] = ports[2];
		g_PadLastPort[3] = ports[3];
	}
	g_PadsChanged = 0;
}

/* 0x0002c270  (Core) debug buttons: "Screenshot" (0xc9) and "Debug Pos" (0xca) */
void Core_CreateDebugInput(void)
{
	g_DebugInputMap = ext_InputMapCreate(2);
	g_ScreenshotButton = ext_InputMapAdd(g_DebugInputMap, "Screenshot", 1, 1, 0, 0, 0);
	ext_InputBind(g_ScreenshotButton, 0xc9, 0);
	g_DebugPosButton = ext_InputMapAdd(g_DebugInputMap, "Debug Pos", 1, 0, 0, 0, 0);
	ext_InputBind(g_DebugPosButton, 0xca, 0);
}

/* 0x0002c2e0  (Core) back to the start screen: pop to "PressStart" (or "MiniGameSelect"),
else to the root mode and push the first mode */
void Core_ResetToStart(void)
{
	ext_FUN_00071690();
	ext_FUN_00054730();
	g_FirstMode = g_DefaultFirstMode;
	DAT_0028198c = -1;
	g_FlagBAE9C = 1;
	if (g_StartState == 1 && g_GameType != 0)
	{
		if (CGameModeStack_PopTo(&g_ModeStack, "MiniGameSelect", 0))
			return;
	}
	if (CGameModeStack_PopTo(&g_ModeStack, g_PressStartName, 0))
		return;
	CGameModeStack_PopTo(&g_ModeStack, g_RootModeName, 1);
	CGameModeStack_Push(&g_ModeStack, g_FirstMode, 1);
}

/* 0x0002c3f0  (Core) a jump to 0x0002c2e0 */
void Core_ResetToStart_Thunk(void)
{
	Core_ResetToStart();
}

/* 0x0002c400  (Core) video standard and refresh rate (PAL 50 Hz, or 60 with PAL60) */
void Core_ReadVideoSettings(void)
{
	g_VideoStandard = (int)XGetVideoStandard();
	g_VideoFlags = (unsigned int)XGetVideoFlags();
	if (g_VideoStandard == 3)
	{
		g_RefreshRate = 50.0f;
		if (g_VideoFlags & 0x40)
			g_RefreshRate = 60.0f;
		g_Display60Hz = 0;
		return;
	}
	g_RefreshRate = 60.0f;
	g_Display60Hz = 1;
}

/* 0x0002c460  (Core) the "sound" debug channel: "<addr> <addr> <name> <a> <b> <flags>"
plays sample NAME */
void Core_PollSoundChannel(void)
{
	char message[0x1000];
	char name[0x400];
	unsigned int address, unused;
	int a, b, flags;

	if (g_SoundChannel != NULL && ext_ChannelReceive(g_SoundChannel, message))
	{
		void *sample;

		sscanf(message, "%X %X %s %d %d %d", &address, &unused, name, &a, &b, &flags);
		_strlwr(name);
		sample = ext_FindResource(0, name, 2, 0xdefa);
		/* ADDRESS is an Xbox address sent by the PC tool: meaningless in the port */
		ext_PlaySample(sample, a, b, 0, (flags & 1) ? (void *)(uintptr_t)(address + 0xc0) : NULL);
	}
}

/* 0x0002c520  (Core) log the launch data; started from the demo launcher ("XDEMOS")? */
void Core_ReadLaunchInfo(void)
{
	g_DemoTimer = 0.0f;
	ext_DebugPrintf("\n\nLAUNCH TYPE: %d\n\n", DAT_002d0750);
	ext_DebugPrintf("\n\n");
	if (DAT_002d0750 == 0)
	{
		const int *data = (const int *)DAT_002d08a8;

		ext_DebugPrintf("\n\n%d\n%d\n%d\n\n", data[0], data[1], data[2]);
	}
	if (memcmp(DAT_002d08a8, "XDEMOS", 7) == 0)
		g_LaunchedFromDemos = 1;
}

/* 0x0002c5a0  (Core) */
void Core_HeapStats(void)
{
	int largest;

	ext_HeapFree(&largest);
}

/* 0x0002c5b0  (Core) log a MEMORYSTATUS (-1: the current one; NULL: none), then physical use */
void Core_PrintMemoryStatus(struct core_memory_status *status)
{
	struct core_memory_status now;

	if (status != NULL)
	{
		if (status == (struct core_memory_status *)(uintptr_t)0xffffffffu)
		{
			GlobalMemoryStatus(&now);
			status = &now;
		}
		ext_DebugPrintf("\n\n");
		ext_DebugPrintf("%4d total Mb of virtual memory\n", status->dwTotalVirtual >> 20);
		ext_DebugPrintf("%4d free Mb of virtual memory\n", status->dwAvailVirtual >> 20);
		ext_DebugPrintf("%4d total Mb of physical memory\n", status->dwTotalPhys >> 20);
		ext_DebugPrintf("%4d free of physical memory\n", status->dwAvailPhys);
		ext_DebugPrintf("%4d total Mb of paging file\n", status->dwTotalPageFile >> 20);
		ext_DebugPrintf("%4d free Mb of paging file\n", status->dwAvailPageFile >> 20);
		ext_DebugPrintf("%4d percent of memory is in use\n", status->dwMemoryLoad >> 20);
		ext_DebugPrintf("\n\n");
	}
	ext_DebugPrintf("\n\nPHYS MEM: %d\n\n", ext_PhysMem());
}

/* 0x0002c680  (Core) open the display from the settings (xres, yres, bpp, zdepth, fullscreen) */
int Core_OpenDisplay(void)
{
	char *value;
	int flags;

	value = ext_SettingsGet(g_Settings, "xres");
	g_DisplayWidth = value ? atol(value) : 0x280;
	value = ext_SettingsGet(g_Settings, "yres");
	g_DisplayHeight = value ? atol(value) : 0x1e0;
	value = ext_SettingsGet(g_Settings, "bpp");
	g_DisplayBpp = value ? atol(value) : 0x20;
	value = ext_SettingsGet(g_Settings, "zdepth");
	g_DisplayZDepth = value ? atol(value) : 0x20;
	value = ext_SettingsGet(g_Settings, "fullscreen");
	g_DisplayFullscreen = value ? atol(value) : 0;
	flags = (g_DisplayFullscreen != 0 ? 0 : 0x20) | (g_Display60Hz != 0 ? 2 : 0) | 0x40;
	if (!ext_DisplayOpen(g_DisplayWidth, g_DisplayHeight, g_DisplayBpp, g_DisplayZDepth, flags))
		return 0;
	ext_DebugPrintf("Open Display done: %f %f\n", (double)DAT_002be588, (double)DAT_002be58c);
	ext_DisplayGet(&g_DisplayActualWidth, &g_DisplayActualHeight, &g_DisplayActualBpp,
		&g_DisplayActualZDepth);
	ext_DebugPrintf("Get Display done: %f %f\n", (double)DAT_002be588, (double)DAT_002be58c);
	if (g_DisplayFullscreen == 0)
	{
		g_DisplayBpp = g_DisplayActualBpp;
		g_DisplayZDepth = g_DisplayActualZDepth;
	}
	ext_SetClipPlanes(40.0f, 10000.0f);
	g_ClearRed = 0;
	g_ClearGreen = 0;
	g_ClearBlue = 0;
	g_ClearAlpha = 0xff;
	/* D3DTSS_ADDRESSU/V/W (13, 14, 15) of stages 2 and 3: clamp, clamp, wrap */
	D3DDevice_SetTextureState_Deferred(2, 0xd, 2);
	D3DDevice_SetTextureState_Deferred(2, 0xe, 2);
	D3DDevice_SetTextureState_Deferred(2, 0xf, 0);
	D3DDevice_SetTextureState_Deferred(3, 0xd, 2);
	D3DDevice_SetTextureState_Deferred(3, 0xe, 2);
	D3DDevice_SetTextureState_Deferred(3, 0xf, 0);
	ext_DebugPrintf("Set View done\n");
	return 1;
}

/* ====================================================================== main */

/* 0x0002d540  (Core) main: runs core_init through bKernel (0x0004e690 calls it), then the
original spins forever so that main never returns to XapiBootToDash.
Port: it returns instead, so that src/runtime/of_boot.c ends the process (a spinning guest
would hang the host and the tests). This is the only intended difference. */
void Core_Main(void)
{
	ext_RunMain(Core_Init, 0);
}
