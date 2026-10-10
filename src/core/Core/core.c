/*
CORE.C  <-  C:\BlitzParty\Game\Src\Core\core.cpp

openfrenzy core module: game start-up (core_init), the 13 games, the frame clock and the
main loop. Decompiled from orig/default.xbe (functional, not byte-matching).
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"
#include "core_internal.h"

static const char core_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\core.cpp";

/* 0x0002c8e0  Core/core.cpp
Makes the 13 game objects and initialises each. If one fails, shuts down the ones before it
and returns 0. */
int Core_CreateGames(void)
{
	static void *(*const ctors[CORE_GAME_COUNT])(void *) = {
		ext_Game0_Ctor, ext_Game1_Ctor, ext_Game2_Ctor, ext_Game3_Ctor, ext_Game4_Ctor,
		ext_Game5_Ctor, ext_Game6_Ctor, ext_Game7_Ctor, ext_Game8_Ctor, ext_Game9_Ctor,
		ext_Game10_Ctor, ext_Game11_Ctor, ext_Game12_Ctor,
	};
	/* object sizes and source lines of the original `new` (line 0x142 is not used) */
	static const unsigned int sizes[CORE_GAME_COUNT] = {
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x20, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	};
	static const int lines[CORE_GAME_COUNT] = {
		0x139, 0x13a, 0x13b, 0x13c, 0x13d, 0x13e, 0x13f, 0x140, 0x141, 0x143, 0x144, 0x145, 0x146,
	};
	int i, j;

	for (i = 0; i < CORE_GAME_COUNT; i++)
		g_Games[i] = NULL;
	for (i = 0; i < CORE_GAME_COUNT; i++)
	{
		void *memory = ext_New(sizes[i], core_cpp, lines[i], 0x1001);

		g_Games[i] = memory ? (struct CGame *)ctors[i](memory) : NULL;
	}
	for (i = 0; i < CORE_GAME_COUNT; i++)
	{
		struct CGame *game = g_Games[i];

		if (game && !game->vtbl->init(game))
		{
			for (j = 0; j < i; j++)
			{
				if (g_Games[j])
					g_Games[j]->vtbl->shutdown(g_Games[j]);
			}
			return 0;
		}
	}
	return 1;
}

/* 0x0002cc90  Core/core.cpp
Once per frame: the clock, and the frame time factor DAT_0028fda8 (1.0 at 60 Hz) from the
vertical blanks since the last frame, clamped to 1-5. */
void Core_UpdateFrameTime(void)
{
	struct core_field_status status = { 0, 0 };
	uint64_t now = ext_ClockRead();
	uint64_t elapsed = ext_ClockDiff(g_LastClock, now);
	int vblanks;
	double frames, scale;

	g_LastClock = now;
	g_FrameSeconds = ext_ClockToSeconds(elapsed);
	D3DDevice_GetDisplayFieldStatus(&status);
	vblanks = (int)status.vblank_count - g_LastVBlank;
	frames = (double)vblanks;
	if (vblanks < 0)
		frames += 4294967296.0;
	if (frames > 5.0)
		frames = 5.0;
	else if (frames < 1.0)
		frames = 1.0;
	g_LastVBlank = (int)status.vblank_count;
	scale = frames / (double)g_RefreshRate * 60.0;
	if (g_TurboCheat)
		scale *= 1.5;
	DAT_0028fda8 = (float)((double)g_FrameScale * scale);
}

/* 0x0002cd80  Core/core.cpp  (core_init)
Start-up, the main loop until g_Quit, and shutdown. */
void Core_Init(void)
{
	struct core_memory_status status_start, status_now;
	char log_name[16];
	unsigned char frame_timer[0x40];   /* bkernel timer objects; the first is a 64-bit count */
	unsigned char slow_timer[0x24];
	int reserve;
	int i;
	const char *package;
	char *option;

	memset(DAT_002d08a8, 0, sizeof(DAT_002d08a8));
	DAT_002d0750 = -1;
	XMountUtilityDrive(0);
	GlobalMemoryStatus(&status_start);
	ext_DisplayPreInit();
	GlobalMemoryStatus(&status_now);
	reserve = 0x3100000;
	while (!ext_TryReserve(0, reserve, 1))
		reserve -= 0x19000;
	ext_DebugPrintf("%4d free of physical memory\n", status_now.dwAvailPhys);
	ext_DebugPrintf("Allocated %d\n", reserve);
	Core_PrintMemoryStatus(&status_start);
	Core_PrintMemoryStatus(&status_now);

	memcpy(log_name, "d:\\debugLog.txt", 16);
	DAT_002db824 = 1;
	g_DebugLog = ext_LogOpen(0, log_name, 0xe);
	ext_LogSelect(g_DebugLog);
	ext_DebugPrintf("%4d free of physical memory\n", status_now.dwAvailPhys);
	ext_DebugPrintf("Allocated %d\n", reserve);
	Core_PrintMemoryStatus(&status_start);
	Core_PrintMemoryStatus(&status_now);
	g_Log2 = ext_LogOpen(0, NULL, 0);
	ext_LogSelect(g_Log2);

	memset(CORE_ADDR(0x002ba918), 0, 0x2e * 4);  /* display settings to the clock (0x2ba9d0) */
	{
		void *memory = ext_New(0x5804, core_cpp, 0x2ab, 0x1001);

		g_Settings = memory ? ext_SettingsCtor(memory) : NULL;
	}
	Core_ReadLaunchInfo();
	Core_ReadVideoSettings();
	if (Core_OpenDisplay())
	{
		ext_DebugPrintf("Screen setup done\n");
		ext_GuiSetup();
		ext_DebugPrintf("Gui setup done\n");
		Core_InitGameState();
		ext_DebugPrintf("Game state setup done\n");
		if (Core_CreateGames())
		{
			ext_DebugPrintf("Module setup done\n");
			CGameMode_Root_CreateDebugMenu();
			ext_DebugPrintf("Gui setup done\n");
			Core_CreateDebugInput();
			ext_FUN_00082a00();
			ext_DebugPrintf("PCSystem setup done\n");
			ext_FUN_0004eed0(1);

			/* preload the packages listed at 0x002842c0, up to the empty string */
			for (i = 0, package = g_PreloadPackages[0]; strlen(package) != 0;
				package = g_PreloadPackages[++i])
				ext_PreloadPackage(package, i == 0, 0xdefa, 0);

			{
				void *memory = ext_New(0x7c, core_cpp, 0x2f6, 0x1001);

				CGameModeStack_Push(&g_ModeStack, memory ? ext_CGameMode_Root_Ctor(memory) : NULL, 1);
			}
			ext_DebugPrintf("Root setup done\n");
			ext_TimerInit(frame_timer, 100);
			ext_TimerInit(slow_timer, 1);
			if (ext_ChannelExists("sound"))
				g_SoundChannel = ext_ChannelOpen("sound", 100, 0);
			else
				g_SoundChannel = NULL;

			option = ext_SettingsGet(g_Settings, "game");
			if (option)
			{
				/* "game=<name> [variation=<n>]" on the command line: start that game */
				sprintf(g_RootModeName, "Root");
				for (i = 0; i < CORE_GAME_COUNT; i++)
				{
					struct CGame *game = g_Games[i];

					if (game && _stricmp(game->name, option) == 0)
					{
						char *variation = ext_SettingsGet(g_Settings, "variation");
						int ok;

						if (variation)
							ok = game->vtbl->start(game, atol(variation));
						else
							ok = game->vtbl->start(game, 1);
						ext_DebugPrintf("Starting game %d (%s) %s\n", i, g_Games[i]->name,
							ok ? "SUCCEEDED" : "FAILED");
						break;
					}
				}
			}
			else
			{
				void *memory = ext_New(0x28, core_cpp, 0x32b, 0x1001);

				CGameModeStack_Push(&g_ModeStack, memory ? ext_CGameMode_13a10_Ctor(memory) : NULL, 1);
			}

			ext_FUN_000559a0(CORE_VAR(float, 0x00284414));
			memcpy(DAT_002be330, DAT_002855f0, sizeof(DAT_002be330));
			g_LastClock = ext_ClockRead();
			ext_PadsChanged();

			while (!g_Quit)
			{
				ext_TimerTick(slow_timer);
				ext_TimerTick(frame_timer);
				ext_ClockRead();
				Core_UpdatePads();
				/* attract mode: back to "Root" and the demo when the timer runs out */
				if (g_DemoTimer > 0.0f && !g_DemoTimerOff)
				{
					g_DemoTimer = g_DemoTimer - DAT_0028fda8;
					if (g_DemoTimer <= 0.0f)
					{
						void *memory;

						g_DemoTimer = -1.0f;
						CGameModeStack_PopTo(&g_ModeStack, "Root", 0);
						memory = ext_New(0xc4, core_cpp, 0x35b, 0x1001);
						CGameModeStack_Push(&g_ModeStack, memory ? ext_CGameMode_18350_Ctor(memory) : NULL, 1);
					}
				}
				if (DAT_002ecdec < 0x14)
					CGameModeStack_Update(&g_ModeStack);
				if (DAT_002ecdec == 0)
				{
					ext_FUN_00071b10();
					Core_PollSoundChannel();
				}
				CGameModeStack_Process(&g_ModeStack);
				ext_FUN_00055240(1);
				if (ext_BeginScene())
				{
					Core_UpdateFrameTime();
					CGameModeStack_Draw(&g_ModeStack);
					ext_FUN_000762e0(DAT_002ecd90);
					if (g_ShowFps)
					{
						void *font = ext_FindResource(0, "handgotl.ttf", 3, 0xdefa);

						if (font)
						{
							int saved_mode;

							if (g_FrameCount % 20 == 0)
								ext_WSprintf(g_FpsText, u"%0.1f", 60.0 / (double)DAT_0028fda8);
							saved_mode = DAT_002be57c;
							ext_SetRenderMode(0);
							ext_FUN_000438b0();
							ext_DrawText(font, g_FpsText, ext_WideStrlen(g_FpsText), 240.0f, 200.0f,
								0x7f, 0x7f, 0x7f, 0x7f, NULL, NULL);
							ext_SetRenderMode(saved_mode);
						}
					}
					ext_EndScene();
				}
				CGameModeStack_PostDraw(&g_ModeStack);
				ext_FUN_00047cb0();
				if (ext_TimerDue(frame_timer))
					g_Fps = (float)((double)(int64_t)DAT_002cd7a0 / (double)*(int64_t *)frame_timer);
				if (!g_NoHeapCheck)
				{
					for (i = 0; i < 2; i++)
					{
						uint32_t *block = DAT_002db814[i];

						if (block && !ext_HeapCheck(9, block[3], block[0], *(uint16_t *)&block[1]))
							DbgBreakPoint();
					}
				}
				ext_SetClearColour(g_ClearRed, g_ClearGreen, g_ClearBlue, g_ClearAlpha);
				g_FrameCount++;
			}

			CGameModeStack_PopAll(&g_ModeStack);
			for (i = 0; i < CORE_GAME_COUNT; i++)
			{
				if (g_Games[i])
				{
					g_Games[i]->vtbl->shutdown(g_Games[i]);
					if (g_Games[i])
						g_Games[i]->vtbl->destroy(g_Games[i], 1);
				}
			}
			if (g_Settings)
			{
				void *settings = g_Settings;

				ext_Nothing(settings);   /* its destructor is empty */
				ext_Delete(settings);
			}
			ext_FUN_00047de0();
			ext_FUN_0004b620(-1);
			if (g_DebugLog)
				ext_LogClose(g_DebugLog);
			if (g_Log2)
				ext_LogClose(g_Log2);
		}
		ext_DisplayClose();
	}
	ext_FUN_0004f860();
}
