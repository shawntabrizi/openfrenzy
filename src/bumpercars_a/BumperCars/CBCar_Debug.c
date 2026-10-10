/*
BumperCars/CBCar_Debug.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Debug.cpp   (module bumpercars_a)

The BumperCars debug options (the object at CBCar_Game + 0x18, vtable 0x00264194): tweak
menus for the world, the players, the computer drivers and the camera (core's CTweakMenu),
switches for the drawing passes, and an FPS overlay. 0x0008c0c0 is tagged with this file;
the gap functions around it are this class's methods (constructor, destructor, getters,
the overlay).
*/

#include "bumpercars_a.h"

static const char s_DebugFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Debug.cpp";

/* the debug object (0x44 bytes) */
struct CBCar_Debug {
	const void *const *vtable;  /* 0x00 0x00264194 */
	int initialised;            /* 0x04 */
	struct CTweakMenu *world;   /* 0x08 "WorldDebug" */
	struct CTweakMenu *players; /* 0x0c "PlayerDebug" */
	struct CTweakMenu *ai;      /* 0x10 "AIDebug" */
	struct CTweakMenu *camera;  /* 0x14 "CameraDebug" (debug camera only) */
	unsigned char *fpsOverlay;  /* 0x18 a COverlay */
	int overlays;               /* 0x1c "Overlays" */
	int fog;                    /* 0x20 "Fog" */
	int collisionMesh;          /* 0x24 "Show Collision Mesh" */
	int envMap;                 /* 0x28 "Environment Mapping" */
	int characters;             /* 0x2c "Draw Characters" */
	int showFps;                /* 0x30 "Show FPS" */
	int headlights;             /* 0x34 "Show Headlights" */
	int reflections;            /* 0x38 "Show Reflections" */
	int lighting;               /* 0x3c "Lighting" */
	int unknown40;              /* 0x40 */
};
OF_ASSERT_SIZE(struct CBCar_Debug, 0x44);

/* globals after the game object (the engine sound settings; tentative definitions in
ClassLib/bca_foreign.c until bumpercars_b's block names them) */
extern int DAT_002f2048;  /* engine minimum frequency change */
extern int DAT_002f204c;  /* engine maximum volume */
extern int DAT_002f2050;  /* engine minimum volume */
extern int DAT_002f2054;  /* engine frequency change */
extern int DAT_002f2058;  /* engine turning volume */
extern float DAT_002f2170[4]; /* the computer drivers' speed, per player */

/* the game's fields the menus edit, by original address */
#define GAME_F(addr) (&BCG((addr) - 0x002ed458u, float))
#define GAME_I(addr) (&BCG((addr) - 0x002ed458u, int))

#define FPS_FORMAT u"[fps: %2.02f] [average: %2.02f]"

/* 0x00264194  CBCar_Debug vtable */
const void *const vtable_00264194[1] = {
	(const void *)FUN_0008c0a0, /* [0] scalar deleting destructor */
};

/* 0x0008bf50  (gap): CBCar_Debug::CBCar_Debug */
void *CBCar_Debug_Ctor(struct CBCar_Debug *self)
{
	self->vtable = vtable_00264194;
	self->lighting = 1;
	self->unknown40 = 0;
	self->overlays = 1;
	self->fog = 1;
	self->collisionMesh = 0;
	self->headlights = 1;
	self->showFps = 0;
	self->characters = 1;
	self->envMap = 0;
	self->reflections = 0;
	self->initialised = 0;
	return self;
}

/* 0x0008bf90  (gap): CBCar_Debug::~CBCar_Debug */
void CBCar_Debug_Dtor(struct CBCar_Debug *self)
{
	self->vtable = vtable_00264194;
}

/* the game timer's frame rate and average (bumpercars_b) */
static void print_fps(unsigned short *text)
{
	float average = BCA_FN(FUN_00096620, float, (void *))(g_BCarGame->timer);
	float fps = BCA_FN(FUN_00096610, float, (void *))(g_BCarGame->timer);

	bkWideSprintf(text, (const unsigned short *)FPS_FORMAT, (double)fps, (double)average);
}

/* 0x0008bfa0  (gap): updates the FPS overlay's text and visibility */
void CBCar_Debug_Update(struct CBCar_Debug *self, int playing)
{
	(void)playing;
	if (self->initialised == 0)
		return;
	print_fps((unsigned short *)(self->fpsOverlay + 0xa4));
	BCA_AT(self->fpsOverlay, 0x54, unsigned short) = self->showFps == 0;
}

/* 0x0008c000  (gap): the lighting is switched off */
int CBCar_Debug_NoLighting(struct CBCar_Debug *self)
{
	if (self->initialised == 0)
		return 0;
	return self->lighting == 0;
}

/* 0x0008c020  (gap) */
int CBCar_Debug_Get40(struct CBCar_Debug *self)
{
	return self->initialised == 0 ? 0 : self->unknown40;
}

/* 0x0008c030  (gap) */
int CBCar_Debug_EnvMap(struct CBCar_Debug *self)
{
	return self->initialised != 0 ? self->envMap : 1;
}

/* 0x0008c040  (gap) */
int CBCar_Debug_Reflections(struct CBCar_Debug *self)
{
	return self->initialised == 0 ? 0 : self->reflections;
}

/* 0x0008c050  (gap) */
int CBCar_Debug_Overlays(struct CBCar_Debug *self)
{
	return self->initialised != 0 ? self->overlays : 1;
}

/* 0x0008c060  (gap) */
int CBCar_Debug_Fog(struct CBCar_Debug *self)
{
	return self->initialised != 0 ? self->fog : 1;
}

/* 0x0008c070  (gap) */
int CBCar_Debug_CollisionMesh(struct CBCar_Debug *self)
{
	return self->initialised == 0 ? 0 : self->collisionMesh;
}

/* 0x0008c080  (gap) */
int CBCar_Debug_Characters(struct CBCar_Debug *self)
{
	return self->initialised != 0 ? self->characters : 1;
}

/* 0x0008c090  (gap) */
int CBCar_Debug_Headlights(struct CBCar_Debug *self)
{
	return self->initialised != 0 ? self->headlights : 1;
}

/* 0x0008c0a0  (gap): CBCar_Debug scalar deleting destructor */
struct CBCar_Debug *CBCar_Debug_Destroy(struct CBCar_Debug *self, byte flags)
{
	self->vtable = vtable_00264194;
	if ((flags & 1) != 0)
		BCA_DELETE(self);
	return self;
}

static struct CTweakMenu *new_menu(int line, const char *name)
{
	void *menu = BCA_NEW(0xe8, s_DebugFile, line, 0x1001);

	if (menu == NULL)
		return NULL;
	return CTweakMenu_Ctor((struct CTweakMenu *)menu, name);
}

/* 0x0008c0c0  BumperCars/CBCar_Debug.cpp: CBCar_Debug::Initialise: the defaults and the
tweak menus. Returns 1. */
int CBCar_Debug_Initialise(struct CBCar_Debug *self)
{
	struct CTweakMenu *m;
	int variation = g_BCarGame->variation;

	self->initialised = 0;
	DAT_002f2054 = 20000;
	DAT_002f2048 = 0;
	DAT_002f2050 = 0x1e;
	DAT_002f204c = 0x5a;
	DAT_002f2058 = 0x8c;
	self->lighting = 1;
	self->unknown40 = 0;
	self->overlays = 1;
	self->fog = 1;
	self->collisionMesh = 0;
	self->headlights = 1;
	self->characters = 1;
	self->envMap = 0;
	self->reflections = 0;
	self->showFps = 0;

	m = self->world = new_menu(0x70, "WorldDebug");
	CTweakMenu_AddFloat(m, "Gravity", GAME_F(0x002ef994), 0.1f, -10.0f, -0.5f);
	CTweakMenu_AddFloat(self->world, "Friction", GAME_F(0x002ef9a0), 0.001f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->world, "Edge Friction", GAME_F(0x002ef9f8), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->world, "World Bounce", GAME_F(0x002ef9a8), 0.01f, 0.0f, 1.0f);
	if (variation == 2)
		CTweakMenu_AddFloat(self->world, "Keeper Lose Wall Vel", GAME_F(0x002ef9e8), 1.0f, 0.0f, 9999.0f);
	if (variation == 4) {
		CTweakMenu_AddFloat(self->world, "Waypoint Lag Boost", GAME_F(0x002ef9fc), 1.0f, 0.0f, 9999.0f);
		if (g_BCarGame->variation == 4)
			CTweakMenu_AddInt(self->world, "Max Waypoint Lag", GAME_I(0x002efa00), 1, 0, 0x32, 0);
	}
	if (g_BCarGame->variation == 1 || g_BCarGame->variation == 2)
		CTweakMenu_AddInt(self->world, "Lead Score Gap", GAME_I(0x002f072c), 1, 0, 100, 0);
	CTweakMenu_AddFloat(self->world, "Frontal Flatspot", GAME_F(0x002efa08), 1.0f, 0.0f, 360.0f);
	CTweakMenu_AddBool(self->world, "Overlays", &self->overlays);
	CTweakMenu_AddBool(self->world, "DebugCam", GAME_I(0x002efa98));
	CTweakMenu_AddBool(self->world, "Fog", &self->fog);
	CTweakMenu_AddBool(self->world, "Show Collision Mesh", &self->collisionMesh);
	CTweakMenu_AddBool(self->world, "Show Reflections", &self->reflections);
	CTweakMenu_AddBool(self->world, "Environment Mapping", &self->envMap);
	CTweakMenu_AddInt(self->world, "EnvMap Strength", GAME_I(0x002efa90), 1, 0, 0xff, 0);
	CTweakMenu_AddBool(self->world, "Draw Characters", &self->characters);
	CTweakMenu_AddBool(self->world, "Show FPS", &self->showFps);
	CTweakMenu_AddBool(self->world, "Show Headlights", &self->headlights);
	CTweakMenu_AddBool(self->world, "Lighting", &self->lighting);

	m = self->players = new_menu(0x8b, "PlayerDebug");
	CTweakMenu_AddFloat(m, "Entity Bounce", GAME_F(0x002ef9ac), 0.01f, 0.0f, 30.0f);
	CTweakMenu_AddFloat(m, "Collision Bias", GAME_F(0x002ef9b0), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(m, "Max Speed", GAME_F(0x002ef9b4), 1.0f, 10.0f, 1000.0f);
	CTweakMenu_AddFloat(m, "Collision Timeout", GAME_F(0x002ef9b8), 0.1f, 0.0f, 90.0f);
	CTweakMenu_AddFloat(m, "RestartLock", GAME_F(0x002ef9c0), 0.1f, 0.0f, 5.0f);
	CTweakMenu_AddFloat(m, "Acceleration", GAME_F(0x002ef9c4), 0.1f, 0.0f, 300.0f);
	CTweakMenu_AddFloat(m, "Rotation Tick", GAME_F(0x002ef9c8), 0.005f, 0.0f, 0x1.921fb6p+2f);
	CTweakMenu_AddFloat(m, "Brake Fraction", GAME_F(0x002ef9cc), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(m, "Boost Speed", GAME_F(0x002ef9d0), 5.0f, 0.0f, 9999.0f);
	CTweakMenu_AddFloat(m, "Boost Length", GAME_F(0x002ef9d4), 0.05f, 0.0f, 10.0f);
	CTweakMenu_AddInt(self->players, "Boost Limit", GAME_I(0x002ef9d8), 1, -1, 100, 0);
	CTweakMenu_AddFloat(self->players, "Boost Chargetime", GAME_F(0x002ef9dc), 0.1f, 0.0f, 10.0f);
	if (g_BCarGame->variation == 2)
		CTweakMenu_AddFloat(self->players, "Ent Tag Hit Vel", GAME_F(0x002ef9e4), 1.0f, 0.0f, 9999.0f);
	CTweakMenu_AddFloat(self->players, "No brake on hit time", GAME_F(0x002ef9bc), 0.1f, 0.0f, 30.0f);
	CTweakMenu_AddFloat(self->players, "Max Vertical Vel", GAME_F(0x002efa04), 1.0f, 0.0f, 9999.0f);
	CTweakMenu_AddFloat(self->players, "Disable Velocity", GAME_F(0x002ef9ec), 1.0f, -1.0f, 9999.0f);
	CTweakMenu_AddInt(self->players, "Engine FreqMod", &DAT_002f2054, 5, 0, 99999, 0);
	CTweakMenu_AddInt(self->players, "Engine MinFreqMod", &DAT_002f2048, 5, 0, 99999, 0);
	CTweakMenu_AddInt(self->players, "Engine MaxVol", &DAT_002f204c, 1, 0, 0xff, 0);
	CTweakMenu_AddInt(self->players, "Engine MinVol", &DAT_002f2050, 1, 0, 0xff, 0);
	CTweakMenu_AddInt(self->players, "Engine TurnVol", &DAT_002f2058, 1, 0, 0xff, 0);

	m = self->ai = new_menu(0xa4, "AIDebug");
	CTweakMenu_AddFloat(m, "P1 Aggression", GAME_F(0x002efbc0), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P1 Vindictiveness", GAME_F(0x002efbc4), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P1 Cleverness", GAME_F(0x002efbc8), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P1 Accuracy", GAME_F(0x002efbcc), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P1 Speed", &DAT_002f2170[0], 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P2 Aggression", GAME_F(0x002efea0), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P2 Vindictiveness", GAME_F(0x002efea4), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P2 Cleverness", GAME_F(0x002efea8), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P2 Accuracy", GAME_F(0x002efeac), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P2 Speed", &DAT_002f2170[1], 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P3 Aggression", GAME_F(0x002f0180), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P3 Vindictiveness", GAME_F(0x002f0184), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P3 Cleverness", GAME_F(0x002f0188), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P3 Accuracy", GAME_F(0x002f018c), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P3 Speed", &DAT_002f2170[2], 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P4 Aggression", GAME_F(0x002f0460), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P4 Vindictiveness", GAME_F(0x002f0464), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P4 Cleverness", GAME_F(0x002f0468), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P4 Accuracy", GAME_F(0x002f046c), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(self->ai, "P4 Speed", &DAT_002f2170[3], 0.01f, 0.0f, 1.0f);

	if (BCG(0x002efae0 - 0x002ed458u, int) == 4) { /* the camera mode: the debug camera */
		m = self->camera = new_menu(0xbe, "CameraDebug");
		CTweakMenu_AddFloat(m, "Distance", GAME_F(0x002efb80), 10.0f, 0.0f, 99999.0f);
		CTweakMenu_AddFloat(self->camera, "X Angle", GAME_F(0x002efb24), 0.001f, 0.0f, 0x1.921fb4p+1f);
		CTweakMenu_AddFloat(self->camera, "Y Angle", GAME_F(0x002efb28), 0.01f, 0.0f, 0x1.921fb6p+2f);
		CTweakMenu_AddFloat(self->camera, "Y Focus", GAME_F(0x002efb18), 10.0f, -9999.0f, 9999.0f);
		CTweakMenu_AddBool(self->camera, "Rotating", GAME_I(0x002efaec));
	} else {
		self->camera = NULL;
	}
	self->initialised = 1;
	return 1;
}

/* 0x0008c9f0  (gap): creates the FPS overlay (a text overlay, font handgotl.ttf) */
void CBCar_Debug_CreateOverlay(struct CBCar_Debug *self)
{
	int desc[0xec];

	if (self->initialised == 0)
		return;
	memset(desc, 0, sizeof desc);
	desc[0xa2] = 2;                      /* +0x288 */
	BCA_F32(desc, 0x2a4) = 0.0f;
	BCA_F32(desc, 0x2a8) = 180.0f;
	BCA_F32(desc, 0x2ac) = 0.0f;
	BCA_F32(desc, 0x250) = 127.0f;       /* colour */
	BCA_F32(desc, 0x254) = 127.0f;
	BCA_F32(desc, 0x258) = 127.0f;
	BCA_F32(desc, 0x25c) = 127.0f;
	desc[0x11] = (int)(uintptr_t)bLoadResource(NULL, "handgotl.ttf", 3, 0xdefa); /* +0x44 font */
	desc[0xa4] = 0;                      /* +0x290 */
	print_fps((unsigned short *)&desc[0x12]); /* +0x48 text */
	self->fpsOverlay = (unsigned char *)(uintptr_t)COverlay_Create(desc);
	BCA_AT(self->fpsOverlay, 0x54, unsigned short) = self->showFps == 0;
}

/* 0x0008cae0  (gap): deletes the tweak menus */
void CBCar_Debug_Shutdown(struct CBCar_Debug *self)
{
	struct CTweakMenu **menus[4];
	int i;

	menus[0] = &self->world;
	menus[1] = &self->players;
	menus[2] = &self->ai;
	menus[3] = &self->camera;
	for (i = 0; i < 4; i++) {
		if (*menus[i] != NULL) {
			CTweakMenu_Dtor(*menus[i]);
			BCA_DELETE(*menus[i]);
			*menus[i] = NULL;
		}
	}
	self->initialised = 0;
}
