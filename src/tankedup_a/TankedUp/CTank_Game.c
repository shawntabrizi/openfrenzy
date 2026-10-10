/*
TankedUp/CTank_Game.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Game.cpp

CTank_Game, the static game object at 0x00312f50: it owns the subsystems (see tankedup_a.h),
loads "game.cfg" (the variation's files and timer), starts and frees the game, runs the
state machine of the subsystems, and draws them in order.

0x001c1fb0 is tagged with this file (an allocation); the gap functions 0x001c1880-0x001c1f30
and 0x001c2240-0x001c2cf0 are placed here because their debug strings name CTank_Game
("CTank_Game::LoadGameSettings", "CTank_Game::LoadGameDetails",
"CTank_Game::InitGameOneTime") or because they work on its subsystem offsets.
*/

#include "tankedup_a.h"

/* 0x001c1880  TankedUp/CTank_Game.cpp (inferred)
The constructor (core's static constructor runs it on 0x00312f50): constructs the
subsystems in member order. 0x000befa0 stores 0 into the first word, 0x00162f20 does
nothing; both are shared trivial constructors. */
struct CTank_Game *CTankGame_Ctor(struct CTank_Game *self)
{
	CLList_Init((int *)self->list);
	TUB_Sound_Ctor(TU_SYS(self, TG_SOUND));
	CTankEntities_Ctor(TU_SYS(self, TG_ENTITIES));
	CTankWorld_Ctor(TU_SYS(self, TG_WORLD));
	CTankPlayers_Ctor(TU_SYS(self, TG_PLAYERS));
	CTankRoute_Ctor(TU_SYS(self, TG_ROUTE));
	CTankCamera_Ctor(TU_SYS(self, TG_CAMERA));
	TU_I32(self, TG_SPAWN) = 0;          /* 0x000befa0 */
	/* lights: 0x00162f20 (nothing) */
	CTankTimer_Ctor((struct TuTimer *)TU_SYS(self, TG_TIMER));
	/* debris: 0x00162f20 (nothing) */
	TU_I32(self, TG_BOOM) = 0;           /* 0x000befa0 */
	TUB_Chars_Ctor(TU_SYS(self, TG_CHARS));
	self->mode = NULL;
	self->started = 0;
	self->hud = 0;
	self->variation = -1;
	return self;
}

/* 0x001c1990  TankedUp/CTank_Game.cpp (inferred)
The destructor: destroys the subsystems in reverse order. Only the camera and the player
system have a destructor with code; the others are the empty function 0x001d5950. */
void CTankGame_Dtor(struct CTank_Game *self)
{
	CTankCamera_Dtor(TU_SYS(self, TG_CAMERA));
	CTankPlayers_Dtor(TU_SYS(self, TG_PLAYERS));
}

/* 0x001c1b20  TankedUp/CTank_Game.cpp (inferred)
Frees the game at the end of a variation: every subsystem, the overlays, the projectors
and both packages. */
void CTankGame_Free(struct CTank_Game *self)
{
	self->started = 0;
	if (self->valid == 0)
		return;
	CTankPlayers_Free(TU_SYS(self, TG_PLAYERS));
	TUB_Entities_Free(TU_SYS(self, TG_ENTITIES));
	CTankBoom_Free(TU_SYS(self, TG_BOOM));
	CTankRoute_Free(TU_SYS(self, TG_ROUTE));
	CTankWorld_Free(TU_SYS(self, TG_WORLD));
	TUB_Sound_Free(TU_SYS(self, TG_SOUND));
	CTankCamera_Free(TU_SYS(self, TG_CAMERA));
	CTankSpawn_Free(TU_SYS(self, TG_SPAWN));
	TUB_Chars_Free(TU_SYS(self, TG_CHARS));
	CTankLights_Free((struct TuLight *)TU_SYS(self, TG_LIGHTS));
	/* timer: empty Free (0x001d5950) */
	TUB_Debug_Free(TU_ADDR(0x003150b0));
	TankDebris_Free(TU_SYS(self, TG_DEBRIS));
	if (self->hud != 0) {
		COverlay_DeleteAll();
		self->hud = 0;
	}
	CProjector_DeleteAll();
	if (self->package != NULL)
		bkClosePackageThunk((struct bkPackage *)self->package);
	if (self->levelPackage != NULL)
		bkClosePackageThunk((struct bkPackage *)self->levelPackage);
}

/* 0x001c1c60  TankedUp/CTank_Game.cpp (inferred)
Once a frame after the update, while the game runs: the players, the debug object and
the camera. */
void CTankGame_Process(struct CTank_Game *self)
{
	if (self->valid != 0 && self->started != 0) {
		CTankPlayers_Process(TU_SYS(self, TG_PLAYERS));
		TUB_Debug_Update(TU_ADDR(0x003150b0));
		CTankCamera_Process(TU_SYS(self, TG_CAMERA));
	}
}

/* 0x001c1ca0  TankedUp/CTank_Game.cpp (inferred)
Input. Not active: calls the game mode's virtual function at +0x24 when its word +0x14 is
set. Active and not paused: the players' input, then the camera's. */
void CTankGame_Input(struct CTank_Game *self, int active)
{
	if (active == 0) {
		void *mode = self->mode;

		if (TU_I32(mode, 0x14) != 0)
			TU_VFN(mode, 0x24, void (*)(void *))(mode);
		return;
	}
	if (self->valid != 0 && DAT_002ecdec == 0) {
		CTankPlayers_Input(TU_SYS(self, TG_PLAYERS));
		/* debug object input: empty (0x001d5950) */
		CTankCamera_Input(TU_SYS(self, TG_CAMERA));
	}
}

/* 0x001c1d00  TankedUp/CTank_Game.cpp (inferred)
Render: camera, then the systems in order. The debug object's switches (TU_DBG_DRAW_*)
select what is drawn; a debug camera (TU_DBG_CAMERA != -1) draws only the route and the
HUD. The global is read again before every test, as the original does. */
void CTankGame_Render(struct CTank_Game *self, int active)
{
	void *camera;

	if (active == 0 || self->valid == 0)
		return;
	baUpdateCamera();
	bdUpdateView();
	bdSetRenderState(8, 0, 0);
	baSetBrightness(0x100);
	camera = TU_SYS(self, TG_CAMERA);
	CTankCamera_SetView(camera);
	if (TU_DBG_DRAW_WORLD != 0 && TU_DBG_CAMERA == -1)
		CTankWorld_DrawSky(TU_SYS(self, TG_WORLD),
			TU_Timer_GetElapsed(TU_SYS(self, TG_TIMER)));
	if (TU_DBG_DRAW_BOOM != 0 && TU_DBG_CAMERA == -1)
		CTankBoom_DrawScorches(TU_SYS(self, TG_BOOM));
	if (TU_DBG_DRAW_ENTITIES != 0 && TU_DBG_CAMERA == -1)
		TUB_Entities_Draw(TU_SYS(self, TG_ENTITIES));
	if (TU_DBG_CAMERA != -1)
		CTankRoute_Draw(TU_SYS(self, TG_ROUTE));
	/* (else: the spawn system's empty Draw, 0x001d5950) */
	if (TU_DBG_DRAW_CHARS != 0 && TU_DBG_CAMERA == -1)
		TUB_Chars_Draw(TU_SYS(self, TG_CHARS));
	if (TU_DBG_DRAW_ENTITIES != 0 && TU_DBG_CAMERA == -1)
		TUB_Entities_DrawLate(TU_SYS(self, TG_ENTITIES));
	if (TU_DBG_DRAW_WORLD != 0 && TU_DBG_CAMERA == -1)
		CTankWorld_Draw(TU_SYS(self, TG_WORLD));
	if (TU_DBG_DRAW_DEBRIS != 0 && TU_DBG_CAMERA == -1)
		TankDebris_Draw(TU_SYS(self, TG_DEBRIS));
	bdSetRenderState(7, 3, 0);
	if (TU_DBG_DRAW_BOOM != 0) {
		if (TU_DBG_CAMERA != -1)
			goto skip_lights;
		CTankBoom_DrawShockwaves(TU_SYS(self, TG_BOOM));
	}
	if (TU_DBG_CAMERA == -1)
		CTankLights_Draw((struct TuLight *)TU_SYS(self, TG_LIGHTS));
skip_lights:
	if (TU_DBG_DRAW_PLAYERS != 0 && TU_DBG_CAMERA == -1)
		CTankPlayers_Draw(TU_SYS(self, TG_PLAYERS));
	CTankCamera_Draw(camera);
}

/* 0x001c1eb0  TankedUp/CTank_Game.cpp (inferred)
Pauses the game: the timer, the entities, the world and the camera (the other systems
point at the empty function 0x001d5950). */
void CTankGame_Pause(struct CTank_Game *self)
{
	TU_DPRINTF("Warning the game has been pause.  Beware of low flying tanks!!!\n");
	CTankTimer_Pause((struct TuTimer *)TU_SYS(self, TG_TIMER));
	CTankEntities_Pause(TU_SYS(self, TG_ENTITIES));
	CTankWorld_Pause(TU_SYS(self, TG_WORLD));
	CTankCamera_Pause(TU_SYS(self, TG_CAMERA));
}

/* 0x001c1f30  TankedUp/CTank_Game.cpp (inferred)
Resumes the game (the counterpart of CTankGame_Pause). */
void CTankGame_Resume(struct CTank_Game *self)
{
	CTankTimer_StartOrResume((struct TuTimer *)TU_SYS(self, TG_TIMER));
	CTankEntities_Resume(TU_SYS(self, TG_ENTITIES));
	CTankWorld_Resume(TU_SYS(self, TG_WORLD));
	CTankCamera_Resume(TU_SYS(self, TG_CAMERA));
}

/* 0x001c1fb0  TankedUp/CTank_Game.cpp
Starts variation `variation`: makes the TankedUp game mode and pushes it. The setting
"variation" (command line / ini) overrides the variation. Returns 0 when the push failed. */
int CTankGame_Start(struct CTank_Game *self, int variation)
{
	void *mode;
	char *text;

	mode = TU_NEW(0x2c, TU_FILE_GAME, 0x5a);
	if (mode != NULL)
		mode = TUB_GameMode_Ctor(mode);
	TU_DBG_VARIATION = variation;
	text = Settings_Get(DAT_002ba9a0, "variation");
	if (text != NULL)
		variation = atol(text);
	self->variation = variation;
	self->mode = mode;
	if (CGameModeStack_Push(TU_MODESTACK, (struct CGameMode *)mode, 1) == 0) {
		self->mode = NULL;
		self->variation = -1;
		return 0;
	}
	return 1;
}

/* the new state of one subsystem: logs a change, keeps the highest state */
#define TG_STATE(name, call)                                                        \
	do {                                                                            \
		int next_ = (call);                                                         \
		if (next_ != self->state)                                                   \
			TU_DPRINTF("State change in " name "(%d->%d)\n", self->state, next_);    \
		if (state <= next_)                                                         \
			state = next_;                                                          \
	} while (0)

/* 0x001c2240  TankedUp/CTank_Game.cpp (inferred)
The game's update. Returns 1 when the pause menu took the frame, else 0.

Before state 6 the pause menu runs (in state 1 only once the camera's intro time at +0x13c
of the camera's current shot reached 60). Pausing and resuming follow the global pause
flag. Each subsystem's update returns its wished state; the highest wins and every system
hears about the change. The frame step is the timer's frame length, at most 10. State 7
ends the variation: the game mode's word +0x28 is set unless g_StartState is 2. */
int CTankGame_Update(struct CTank_Game *self, int active)
{
	int state = self->state;
	float step;

	if (active == 0 || self->valid == 0)
		return 0;
	if (state < 6
		&& (state != 1 || TU_F32(TU_PTR(self, TG_CAMERA + 0x6c), 0x13c) >= 60.0f)
		&& TU_PauseMenu_Update() != 0)
		return 1;
	if (DAT_002ecdec == 0) {
		if (self->paused == 0)
			goto update;
		TU_DPRINTF("Game UN-pausing\n");
		if (self->started != 0)
			CTankGame_Resume(self);
		self->paused = 0;
	} else {
		if (self->paused != 0)
			return 0;
		TU_DPRINTF("Game pausing\n");
		CTankGame_Pause(self);
		self->paused = 1;
	}
	if (DAT_002ecdec != 0)
		return 0;
update:
	CTankPlayers_BeginFrame(TU_SYS(self, TG_PLAYERS));
	step = CTankTimer_GetFrame((struct TuTimer *)TU_SYS(self, TG_TIMER));
	if (step > 10.0f)
		step = 10.0f;
	TG_STATE("worldSystem", CTankWorld_Update(TU_SYS(self, TG_WORLD), step, self->state));
	TG_STATE("entitySystem", TUB_Entities_Update(TU_SYS(self, TG_ENTITIES), step, self->state));
	TG_STATE("boomSystem", CTankBoom_Update(TU_SYS(self, TG_BOOM), step, self->state));
	TG_STATE("playerSystem", CTankPlayers_Update(TU_SYS(self, TG_PLAYERS), step, self->state));
	TG_STATE("cameraSystem", CTankCamera_Update(TU_SYS(self, TG_CAMERA), step, self->state));
	TG_STATE("routingSystem", CTankSystem_KeepState(TU_SYS(self, TG_ROUTE), step, self->state));
	TG_STATE("spawnSystem", CTankSystem_KeepState(TU_SYS(self, TG_SPAWN), step, self->state));
	TG_STATE("charSystem", TUB_Chars_Update(TU_SYS(self, TG_CHARS), step, self->state));
	TG_STATE("lightSystem", CTankLights_Update((struct TuLight *)TU_SYS(self, TG_LIGHTS), step,
		self->state));
	TG_STATE("debrisSystem", TankDebris_Update(TU_SYS(self, TG_DEBRIS), step, self->state));
	if (state != self->state) {
		TU_DPRINTF("****************\n* STATE CHANGE *\n* ------------ *\n* New state %d  *\n"
			"****************\n", state);
		TU_EmptyStateChange(TU_SYS(self, TG_WORLD), self->state, state);
		TUB_Entities_StateChange(TU_SYS(self, TG_ENTITIES), self->state, state);
		TU_EmptyStateChange(TU_SYS(self, TG_BOOM), self->state, state);
		CTankPlayers_StateChange(TU_SYS(self, TG_PLAYERS), self->state, state);
		CTankCamera_StateChange(TU_SYS(self, TG_CAMERA), self->state, state);
		TU_EmptyStateChange(TU_SYS(self, TG_ROUTE), self->state, state);
		TU_EmptyStateChange(TU_SYS(self, TG_SPAWN), self->state, state);
		TUB_Chars_StateChange(TU_SYS(self, TG_CHARS), self->state, state);
		TU_EmptyStateChange(TU_SYS(self, TG_LIGHTS), self->state, state);
		TU_EmptyStateChange(TU_SYS(self, TG_DEBRIS), self->state, state);
		self->state = state;
	}
	COverlay_UpdateAll();
	CProjector_UpdateAll();
	if (state == 7) {
		TU_DPRINTF("******************************************\nENDDING TANKEDUP VARIATION %d\n"
			"******************************************\n", self->variation);
		if (DAT_002bae98 != 2)
			TU_I32(self->mode, 0x28) = 1;
	}
	return 0;
}

#undef TG_STATE

/* 0x001c2660  TankedUp/CTank_Game.cpp (inferred: "CTank_Game::LoadGameSettings")
Reads the block "variation<n> {" ... "}" of game.cfg: OBJECTFILE, ENTITYFILE, ROUTEFILE,
SPAWNFILE and CAMERAFILE (file names) and TIMER (seconds). Lines starting with "//" are
comments. Returns 1 when the block was found. */
int CTankGame_LoadGameSettings(struct CTank_Game *self, struct TuTextReader *reader)
{
	char header[512];
	char line[512];
	char *text;
	int found = 0;
	int result;

	self->objectFile[0] = 0;
	self->entityFile[0] = 0;
	self->routeFile[0] = 0;
	self->cameraFile[0] = 0;
	self->spawnFile[0] = 0;
	self->timerSeconds = 0;
	while (reader->cur < reader->end) {
		text = TUB_Reader_ReadLine(reader, line);
		if (strncmp(text, "//", 2) == 0)
			continue;
		_snprintf(header, 0x1ff, "variation%d {", self->variation);
		if (_stricmp(line, header) != 0)
			continue;
		found = 1;
		while (reader->cur < reader->end) {
			if (_stricmp(line, "}") == 0)
				break;
			text = TUB_Reader_ReadLine(reader, line);
			if (strncmp(text, "//", 2) == 0)
				continue;
			result = 0;
			if (_strnicmp(text, "OBJECTFILE ", 0xb) == 0) {
				result = 2;
				if (BCarText_GetName(text, self->objectFile, 0x1ff) != 0)
					result = 1;
			}
			if (_strnicmp(text, "ENTITYFILE ", 0xb) == 0) {
				result = 2;
				if (BCarText_GetName(text, self->entityFile, 0x1ff) != 0)
					result = 1;
			}
			if (_strnicmp(text, "ROUTEFILE ", 10) == 0) {
				result = 2;
				if (BCarText_GetName(text, self->routeFile, 0x1ff) != 0)
					result = 1;
			}
			if (_strnicmp(text, "SPAWNFILE ", 10) == 0) {
				result = 2;
				if (BCarText_GetName(text, self->spawnFile, 0x1ff) != 0)
					result = 1;
			}
			if (_strnicmp(text, "CAMERAFILE ", 10) == 0) {
				result = 2;
				if (BCarText_GetName(text, self->cameraFile, 0x1ff) != 0)
					result = 1;
			}
			if (_strnicmp(text, "TIMER ", 6) == 0) {
				result = 2;
				if (sscanf(text, "%*s%d", &self->timerSeconds) == 1)
					result = 1;
			}
			if (_stricmp(text, "}") != 0) {
				sscanf(text, "%s", header);
				if (result == 0)
					TU_DPRINTF("CTank_Game::LoadGameSettings(\"%s\", %d): Invalid Command \"%s\"\n",
						reader->name, reader->line, header);
				else if (result == 2)
					TU_DPRINTF("CTank_Game::LoadGameSettings(\"%s\", %d): Bad use of Command \"%s\"\n",
						reader->name, reader->line, header);
			}
		}
	}
	return found;
}

/* 0x001c2940  TankedUp/CTank_Game.cpp (inferred: "CTank_Game::LoadGameDetails")
Loads game.cfg from the common package and initialises every subsystem in order. A
failure clears `valid` and returns 0. */
int CTankGame_LoadGameDetails(struct CTank_Game *self)
{
	struct TuTextReader reader;
	unsigned int size;
	void *buffer;
	void *world, *route, *entities, *camera, *players;

	TUB_Reader_Ctor(&reader);
	buffer = bkLoadFile((struct bkPackage *)self->package, "game.cfg", NULL, &size, NULL, 0);
	if (buffer == NULL) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - Main Configuration file [%s] not found\n",
			"game.cfg");
		self->valid = 0;
		TUB_Reader_Dtor(&reader);
		return 0;
	}
	BCarTextReader_Init((struct BCarTextReader *)&reader, (char *)buffer, (int)size, "game.cfg");
	if (CTankGame_LoadGameSettings(self, &reader) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - Failed to load variation [%d]!!!!\n",
			self->variation);
		self->valid = 0;
		TUB_Reader_Dtor(&reader);
		return 0;
	}
	if (TUB_Debug_Initialise(TU_ADDR(0x003150b0)) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - debug.Initialise() FAILED!\n");
		self->valid = 0;
		TUB_Reader_Dtor(&reader);
		return 0;
	}
	world = TU_SYS(self, TG_WORLD);
	route = TU_SYS(self, TG_ROUTE);
	entities = TU_SYS(self, TG_ENTITIES);
	camera = TU_SYS(self, TG_CAMERA);
	players = TU_SYS(self, TG_PLAYERS);
	if (TUB_Sound_Initialise(TU_SYS(self, TG_SOUND)) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - soundSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (CTankTimer_Initialise((struct TuTimer *)TU_SYS(self, TG_TIMER),
			self->timerSeconds) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - timerSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (CTankLights_Initialise((struct TuLight *)TU_SYS(self, TG_LIGHTS)) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - lightSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (CTankWorld_Initialise(world, self->levelPackage, self->objectFile,
			TU_SYS(self, TG_SOUND), TU_SYS(self, TG_LIGHTS)) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - worldSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (TankDebris_Initialise(TU_SYS(self, TG_DEBRIS), self->package, world) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - debrisSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (CTankRoute_Initialise(route, self->levelPackage, self->routeFile, world) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - routeSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (CTankSpawn_Initialise(TU_SYS(self, TG_SPAWN), self->levelPackage,
			self->spawnFile) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - spawnSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (CTankBoom_Initialise(TU_SYS(self, TG_BOOM)) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - boomSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (TUB_Entities_Initialise(entities, self->levelPackage, self->entityFile, world,
			route, TU_SYS(self, TG_SPAWN), TU_SYS(self, TG_DEBRIS), TU_SYS(self, TG_LIGHTS),
			TU_SYS(self, TG_SOUND), TU_SYS(self, TG_BOOM)) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - entitySystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (CTankCamera_Initialise(camera, self->levelPackage, self->cameraFile,
			entities) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - cameraSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (CTankPlayers_Initialise(players, entities, (struct TuTimer *)TU_SYS(self, TG_TIMER),
			world, camera, TU_SYS(self, TG_SOUND), TU_SYS(self, TG_LIGHTS)) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - playerSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else if (TUB_Chars_Initialise(TU_SYS(self, TG_CHARS), players, TU_SYS(self, TG_LIGHTS)) == 0) {
		TU_DPRINTF("CTank_Game::LoadGameDetails - charSystem.Initialise() FAILED!\n");
		self->valid = 0;
	} else {
		CTankEntities_SetLinks(entities, camera, players);
		CTankRoute_LinkEntities(route, entities);
		TUB_Reader_Dtor(&reader);
		return 1;
	}
	TUB_Reader_Dtor(&reader);
	return 0;
}

/* 0x001c2cf0  TankedUp/CTank_Game.cpp (inferred: "CTank_Game::InitGameOneTime")
Starts the variation once its packages are loaded: loads the packages' resources and the
game details. On failure every subsystem is freed again. */
void CTankGame_InitGameOneTime(struct CTank_Game *self)
{
	TU_DPRINTF("******************************************\nSTARTING TANKEDUP VARIATION %d\n"
		"******************************************\n", self->variation);
	if (self->valid == 0)
		return;
	if (self->package != NULL)
		bLoadPackageResources((struct bkPackage *)self->package, 0xffffffff, 0xdefa, 0);
	if (self->levelPackage != NULL)
		bLoadPackageResources((struct bkPackage *)self->levelPackage, 0xffffffff, 0xdefa, 0);
	self->paused = 0;
	if (CTankGame_LoadGameDetails(self) != 0) {
		CProjector_Reset();
		return;
	}
	TUB_Entities_Free(TU_SYS(self, TG_ENTITIES));
	CTankBoom_Free(TU_SYS(self, TG_BOOM));
	CTankRoute_Free(TU_SYS(self, TG_ROUTE));
	CTankWorld_Free(TU_SYS(self, TG_WORLD));
	TUB_Sound_Free(TU_SYS(self, TG_SOUND));
	CTankPlayers_Free(TU_SYS(self, TG_PLAYERS));
	CTankCamera_Free(TU_SYS(self, TG_CAMERA));
	CTankSpawn_Free(TU_SYS(self, TG_SPAWN));
	TUB_Chars_Free(TU_SYS(self, TG_CHARS));
	CTankLights_Free((struct TuLight *)TU_SYS(self, TG_LIGHTS));
	/* timer: empty Free (0x001d5950) */
	TUB_Debug_Free(TU_ADDR(0x003150b0));
	TankDebris_Free(TU_SYS(self, TG_DEBRIS));
	TU_DPRINTF("CTank_Game::InitGameOneTime - Failed to load game details, game not loaded\n");
}
