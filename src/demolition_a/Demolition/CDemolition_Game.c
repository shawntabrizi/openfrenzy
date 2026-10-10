/*
Demolition/CDemolition_Game.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Game.cpp

The game object CDemolition_Game (0x7be0 bytes, vtable 0x00269aac) and its
four variations (0 frag "Population Explosion", 1 remote "Misguided
Missile", 2 mine "Boom Town", 3 cross "Blast Man Standing"), each a class
with a 15-slot vtable (0x00269ad8, 0x00269b14, 0x00269b50, 0x00269b8c).

The game mode (demolition_c, made in Start) drives the game: it loads the
packages (CDemolitionGame_LoadPackages), initialises (_Initialise), resets
(_Reset), runs a frame (_PollPreRender) and shuts down (_Shutdown).

Five functions are missing from the function table (Ghidra merged them into
their callers): 0x000e8010, 0x000e8400, 0x000e9870, 0x000e9920, 0x000e9ac0.
They keep their FUN_ names (demolition_c calls them by those names). The
constructor, destructor and the package/frame helpers before the first
tagged function (0x000e7bf0-0x000e8220) are unplaced in the table; they are
this class's, so they are here.
*/

#include "demolition_a.h"
#include "xtl.h"

static const char CDemolition_Game_cpp[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Game.cpp";

/* ---------- missed functions of demolition_c that this file calls (tail calls Ghidra merged
   into the callers). Weak stand-ins until demolition_c defines them. */

/* 0x000fcb90, 0x000fd180, 0x000fd540 and 0x000fe610 are in the function table now
   (demolition_c defines them; integration round 2). */

__attribute__((weak)) void FUN_00100ef0(void *self)
{
	(void)self;
	of_unimplemented(0x00100ef0, "FUN_00100ef0 (demolition_c, missing from the table)");
}

__attribute__((weak)) void FUN_00100f60(void *self)
{
	(void)self;
	of_unimplemented(0x00100f60, "FUN_00100f60 (demolition_c, missing from the table)");
}

/* ---------- calls into the sub-objects (demolition_b/c, rollerball_a) */

#define dg_CharLibCtor(p) ((void (*)(void *))FUN_000f8d80)(p)
#define dg_CharLibDtor(p) ((void (*)(void *))FUN_000f8b20)(p)
#define dg_WorldCtor(p) ((void (*)(void *))FUN_000f6fe0)(p)
#define dg_WorldDtor(p) ((void (*)(void *))FUN_000f6ff0)(p)
#define dg_CameraCtor(p) ((void (*)(void *))FUN_000f5730)(p)
#define dg_CameraDtor(p) ((void (*)(void *))FUN_000f5750)(p)
#define dg_EntitiesCtor(p) ((void (*)(void *))FUN_000f53e0)(p)
#define dg_EntitiesDtor(p) ((void (*)(void *))FUN_000f53f0)(p)
#define dg_BombsCtor(p) ((void (*)(void *))FUN_000f1ba0)(p)
#define dg_BombsDtor(p) ((void (*)(void *))FUN_000f1bb0)(p)

#define dg_CameraInit(p) ((int (*)(void *))FUN_000f5a10)(p)
#define dg_CharLibInit(p, pkg) ((int (*)(void *, void *))FUN_000f8dc0)((p), (pkg))
#define dg_SystemInit(p, pkg) ((int (*)(void *, void *))FUN_000f5400)((p), (pkg))
#define dg_WorldInit(p, pkg, level) ((int (*)(void *, void *, const char *))FUN_000f89e0)((p), (pkg), (level))
#define dg_BombsInit(p, pkg) ((int (*)(void *, void *))FUN_000f4bf0)((p), (pkg))

#define dg_EntitiesUpdate(p, dt) ((void (*)(void *, float))FUN_000f5410)((p), (dt))
#define dg_CharLibUpdate(p, dt) ((void (*)(void *, float))FUN_000f8b30)((p), (dt))
#define dg_BombsUpdate(p, dt) ((void (*)(void *, float))FUN_000f1bc0)((p), (dt))
#define dg_ExplosionsUpdate(p, dt) ((void (*)(void *, float))FUN_000f0ea0)((p), (dt))
#define dg_CameraUpdate(p, dt) ((void (*)(void *, float))FUN_000f6f10)((p), (dt))
#define dg_WorldUpdate(p, dt) ((void (*)(void *, float))FUN_000f71b0)((p), (dt))
#define dg_RumbleUpdate(p, dt) ((void (*)(void *, float))FUN_001ec260)((p), (dt))

#define dg_EntitiesShutdown(p) ((void (*)(void *))FUN_000f2130)(p)
#define dg_BombsShutdown(p) ((void (*)(void *))FUN_000f3b30)(p)
#define dg_WorldShutdown(p) ((void (*)(void *))FUN_000f7000)(p)
#define dg_CharLibShutdown(p) ((void (*)(void *))FUN_000f9210)(p)

#define dg_WorldDrawSky(p) ((void (*)(void *))FUN_000f86d0)(p)
#define dg_WorldDraw(p) ((void (*)(void *))FUN_000f88a0)(p)
#define dg_WorldDrawShadowed(p) ((void (*)(void *))FUN_000f8750)(p)
#define dg_WorldDrawTransparent(p) ((void (*)(void *))FUN_000f8730)(p)
#define dg_EntitiesDraw(p) ((void (*)(void *))FUN_000f5440)(p)
#define dg_CharLibDraw(p) ((void (*)(void *))FUN_000f8b60)(p)
#define dg_CharLibDrawFx(p) ((void (*)(void *))FUN_000f8c00)(p)
#define dg_BombsDraw(p) ((void (*)(void *))FUN_000f1bd0)(p)
#define dg_BombsDrawFx(p) ((void (*)(void *))FUN_000f1be0)(p)
#define dg_ExplosionsDraw(p) ((void (*)(void *))FUN_000f0d50)(p)
#define dg_CameraDraw(p) ((void (*)(void *))FUN_000f57c0)(p)

#define dg_ModeCtor(p) ((void *(*)(void *))FUN_001019c0)(p)

/* the variation worlds (demolition_c) */
#define dg_RemoteWorldCtor(p) ((void (*)(void *))FUN_000fd1d0)(p)
#define dg_RemoteWorldDtor(p) ((void (*)(void *))FUN_000fcb70)(p)
#define dg_RemoteWorldShutdown(p) ((void (*)(void *))FUN_000fd300)(p)
#define dg_RemoteWorldStop(p) ((void (*)(void *))FUN_000fcbc0)(p)
#define dg_RemoteWorldBegin(p) ((void (*)(void *))FUN_000fe010)(p)
#define dg_RemoteWorldUpdate(p, dt) ((void (*)(void *, float))FUN_000fde10)((p), (dt))
#define dg_RemoteWorldInit(p, pkg) ((int (*)(void *, void *))FUN_000fe180)((p), (pkg))
#define dg_MineWorldCtor(p) ((void (*)(void *))FUN_000fe4d0)(p)
#define dg_MineWorldDtor(p) ((void (*)(void *))FUN_000fe540)(p)
#define dg_MineWorldShutdown(p) ((void (*)(void *))FUN_000fe5b0)(p)
#define dg_MineWorldBegin(p) ((void (*)(void *))FUN_000fe8c0)(p)
#define dg_MineWorldInit(p, pkg) ((int (*)(void *, void *))FUN_000feac0)((p), (pkg))
#define dg_MineWorldStop(p) ((void (*)(void *))FUN_000fecf0)(p)
#define dg_MineWorldDraw(p) ((void (*)(void *))FUN_000ff200)(p)
#define dg_MineWorldUpdate(p, dt) ((void (*)(void *, float))FUN_000ff570)((p), (dt))
#define dg_CrossWorldStop(p) ((void (*)(void *))FUN_000ff680)(p)
#define dg_CrossWorldDtor(p) ((void (*)(void *))FUN_000ffee0)(p)
#define dg_CrossWorldShutdown(p) ((void (*)(void *))FUN_00100240)(p)
#define dg_CrossWorldLift(p) ((void (*)(void *))FUN_001002b0)(p)
#define dg_CrossWorldCtor(p) ((void (*)(void *))FUN_00101480)(p)
#define dg_CrossWorldInit(p, pkg) ((int (*)(void *, void *))FUN_001016a0)((p), (pkg))
#define dg_CrossWorldUpdate(p, dt) ((void (*)(void *, float))FUN_00101900)((p), (dt))
#define dg_SmokeCtor(p) ((void *(*)(void *))FUN_001031c0)(p)
#define dg_SmokeInit(p, a, b) ((int (*)(void *, int, int))FUN_00105440)((p), (a), (b))
#define dg_SmokeSetRegion(p, region) ((void (*)(void *, void *))FUN_00103350)((p), (region))

/* the ASE path file helpers (demolition_c): a parsed file (0x10 bytes) and a found shape */
struct DG_ASE {
	uint32_t data[4];
};
struct DG_ASEShape {
	uint32_t unk00;
	void *points; /* +0x04 */
	int count;    /* +0x08 */
};

/* the variations' fields */
#define DV_STATE      0x04   /* int (variation 3) */
#define DV_HEIGHTS    0x04   /* variation 0: CLRouteHeights * (8 bytes) */
#define DV_BARRIERS   0x08   /* variation 0: the barrier region */
#define DV_LIGHT      0x0c   /* variation 0: the red point light */
#define DV_WORLD1     0x10   /* variation 1: the remote world (demolition_c) */
#define DV_AREA1      0x2330 /* variation 1: player_area region */
#define DV_REGIONS2   0x04   /* variation 2: 4 regions */
#define DV_WORLD2     0x18   /* variation 2: the mine world (demolition_c) */
#define DV_SMOKES2    0x290  /* variation 2: 2 smoke systems */
#define DV_STEAM2     0x298  /* variation 2: 2 steam regions */
#define DV_TIMER3     0x08   /* variation 3: timer */
#define DV_WORLD3     0x40   /* variation 3: the cross world (demolition_c) */
#define DV_LIFTED3    0x80   /* variation 3: char */
#define DV_SHAKE3     0x16c0 /* variation 3: 2 floats */

/* ---------- vtables */

void *CDemolitionGame_DeletingDtor(void *self, unsigned int flags);
void *CDemoVar0_DeletingDtor(void *self, unsigned int flags);
void *CDemoVar1_DeletingDtor(void *self, unsigned int flags);
void *CDemoVar2_DeletingDtor(void *self, unsigned int flags);
void *CDemoVar3_DeletingDtor(void *self, unsigned int flags);

/* 0x00269aac: CDemolition_Game */
static const void *const vtable_00269aac[] = {
	(const void *)CDemolitionGame_DeletingDtor, /* [0] +0x00 */
};

/* 0x00269ad8: variation 0 (frag) */
static const void *const vtable_00269ad8[] = {
	(const void *)CDemoVar0_Init,           /* [0] +0x00 */
	(const void *)CDemoVar0_Shutdown,       /* [1] +0x04 */
	(const void *)CDemoVar0_Reset,          /* [2] +0x08 */
	(const void *)CDemoVar0_Start,          /* [3] +0x0c */
	(const void *)CDemoVar0_Finish,         /* [4] +0x10 */
	(const void *)CDemoVar0_PostFinish,     /* [5] +0x14 */
	(const void *)CDemoVar0_Update,         /* [6] +0x18 */
	(const void *)FUN_001d5950,             /* [7] +0x1c */
	(const void *)FUN_001d5950,             /* [8] +0x20 */
	(const void *)CDemoVar0_Begin,          /* [9] +0x24 */
	(const void *)CDemoVar_StoreScores,     /* [10] +0x28 */
	(const void *)FUN_000b0770,             /* [11] +0x2c */
	(const void *)FUN_000f8ab0,             /* [12] +0x30 */
	(const void *)FUN_001caa80,             /* [13] +0x34 */
	(const void *)CDemoVar0_DeletingDtor,   /* [14] +0x38 */
};

/* 0x00269b14: variation 1 (remote) */
static const void *const vtable_00269b14[] = {
	(const void *)CDemoVar1_Init,           /* [0] +0x00 */
	(const void *)CDemoVar1_Shutdown,       /* [1] +0x04 */
	(const void *)CDemoVar1_Reset,          /* [2] +0x08 */
	(const void *)CDemoVar1_Start,          /* [3] +0x0c */
	(const void *)CDemoVar1_Finish,         /* [4] +0x10 */
	(const void *)CDemoVar1_PostFinish,     /* [5] +0x14 */
	(const void *)CDemoVar1_Update,         /* [6] +0x18 */
	(const void *)CDemoVar1_Draw,           /* [7] +0x1c */
	(const void *)FUN_001d5950,             /* [8] +0x20 */
	(const void *)CDemoVar1_Begin,          /* [9] +0x24 */
	(const void *)CDemoVar_StoreScores,     /* [10] +0x28 */
	(const void *)CDemoVar1_Damage,         /* [11] +0x2c */
	(const void *)FUN_000f8ab0,             /* [12] +0x30 */
	(const void *)CDemoVar1_GetArea,        /* [13] +0x34 */
	(const void *)CDemoVar1_DeletingDtor,   /* [14] +0x38 */
};

/* 0x00269b50: variation 2 (mine) */
static const void *const vtable_00269b50[] = {
	(const void *)CDemoVar2_Init,           /* [0] +0x00 */
	(const void *)CDemoVar2_Shutdown,       /* [1] +0x04 */
	(const void *)CDemoVar2_Reset,          /* [2] +0x08 */
	(const void *)CDemoVar2_Start,          /* [3] +0x0c */
	(const void *)CDemoVar2_Finish,         /* [4] +0x10 */
	(const void *)CDemoVar_Unfreeze,        /* [5] +0x14 */
	(const void *)CDemoVar2_Update,         /* [6] +0x18 */
	(const void *)CDemoVar2_Draw,           /* [7] +0x1c */
	(const void *)CDemoVar2_DrawOverlay,    /* [8] +0x20 */
	(const void *)CDemoVar2_Begin,          /* [9] +0x24 */
	(const void *)CDemoVar_StoreScores,     /* [10] +0x28 */
	(const void *)FUN_000b0770,             /* [11] +0x2c */
	(const void *)CDemoVar2_GetRegion,      /* [12] +0x30 */
	(const void *)FUN_00151650,             /* [13] +0x34 */
	(const void *)CDemoVar2_DeletingDtor,   /* [14] +0x38 */
};

/* 0x00269b8c: variation 3 (cross) */
static const void *const vtable_00269b8c[] = {
	(const void *)CDemoVar3_Init,           /* [0] +0x00 */
	(const void *)CDemoVar3_Shutdown,       /* [1] +0x04 */
	(const void *)CDemoVar0_Reset,          /* [2] +0x08 */
	(const void *)CDemoVar3_Start,          /* [3] +0x0c */
	(const void *)CDemoVar3_Finish,         /* [4] +0x10 */
	(const void *)CDemoVar_Unfreeze,        /* [5] +0x14 */
	(const void *)CDemoVar3_Update,         /* [6] +0x18 */
	(const void *)CDemoVar3_Draw,           /* [7] +0x1c */
	(const void *)CDemoVar3_DrawOverlay,    /* [8] +0x20 */
	(const void *)CDemoVar3_Begin,          /* [9] +0x24 */
	(const void *)CDemoVar3_StoreScores,    /* [10] +0x28 */
	(const void *)FUN_000b0770,             /* [11] +0x2c */
	(const void *)FUN_000f8ab0,             /* [12] +0x30 */
	(const void *)FUN_00151650,             /* [13] +0x34 */
	(const void *)CDemoVar3_DeletingDtor,   /* [14] +0x38 */
};

/* ---------- construction */

/* 0x000e7bf0  Demolition/CDemolition_Game.cpp (file inferred: the class's constructor)
Constructs the sub-objects in order. */
void *CDemolitionGame_Ctor(void *self)
{
	char *g = (char *)self;

	*(const void *const **)g = vtable_00269aac;
	dg_CharLibCtor(g + DG_CHARLIB);
	dg_WorldCtor(g + DG_WORLD);
	dg_CameraCtor(g + DG_CAMERA);
	dg_EntitiesCtor(g + DG_ENTITIES);
	dg_BombsCtor(g + DG_BOMBS);
	CDemoExplosionSystem_Ctor(g + DG_EXPLOSIONS);
	CDemoTokenSystem_Ctor(g + DG_TOKENS);
	CDemoOverlay_Ctor(g + DG_OVERLAY);
	CDemoSpotFX_Ctor(g + DG_SPOTFX);
	CDemoTextureSystem_Ctor(g + DG_TEXTURES);
	CDemoLightSystem_Ctor(g + DG_LIGHTS);
	CDemoSimulation_Ctor(g + DG_SIMULATION);
	CDemoInput_Ctor(g + DG_INPUT);
	CDemoTimer_Ctor(g + DG_TIMER);
	CDemoCountdown_Ctor((unsigned int *)(g + DG_COUNTDOWN));
	CDemoDebug_Ctor(g + DG_DEBUG);
	CDemoInfo_Ctor(g + DG_INFO);
	CDemoPlatforms_Ctor(g + DG_PLATFORMS);
	CDemoDirect3D_Ctor(g + DG_DIRECT3D);
	CDemoSfx_Ctor(g + DG_SFX);
	CDemoRumble_Ctor(g + DG_RUMBLE);
	CDemoRespawn_Ctor(g + DG_RESPAWN);
	CDemoUnk7b20_Ctor(g + DG_UNK7B20);
	CDemoDecals_Ctor(g + DG_DECALS);
	DA_PSListCtor(g + DG_PARTICLES);
	DA_FIELD(void *, g, DG_MODE) = NULL;
	return self;
}

/* 0x000e7dc0  Demolition/CDemolition_Game.cpp (file inferred: the class's destructor)
Destroys the sub-objects in reverse order. */
void CDemolitionGame_Dtor(void *self)
{
	char *g = (char *)self;

	*(const void *const **)g = vtable_00269aac;
	DA_PSListDtor(g + DG_PARTICLES);
	CDemoDecals_Dtor(g + DG_DECALS);
	CDemoUnk7b20_Dtor(g + DG_UNK7B20);
	CDemoRespawn_Dtor(g + DG_RESPAWN);
	CDemoRumble_Dtor(g + DG_RUMBLE);
	CDemoSfx_Dtor(g + DG_SFX);
	CDemoDirect3D_Dtor(g + DG_DIRECT3D);
	CDemoPlatforms_Dtor(g + DG_PLATFORMS);
	CDemoInfo_Dtor(g + DG_INFO);
	CDemoDebug_Dtor(g + DG_DEBUG);
	CDemoCountdown_Dtor((unsigned int *)(g + DG_COUNTDOWN));
	CDemoTimer_Dtor(g + DG_TIMER);
	CDemoInput_Dtor(g + DG_INPUT);
	CDemoSimulation_Dtor(g + DG_SIMULATION);
	CDemoLightSystem_Dtor(g + DG_LIGHTS);
	CDemoTextureSystem_Dtor(g + DG_TEXTURES);
	CDemoSpotFX_Dtor(g + DG_SPOTFX);
	CDemoOverlay_Dtor(g + DG_OVERLAY);
	CDemoTokenSystem_Dtor(g + DG_TOKENS);
	CDemoExplosionSystem_Dtor(g + DG_EXPLOSIONS);
	dg_BombsDtor(g + DG_BOMBS);
	dg_EntitiesDtor(g + DG_ENTITIES);
	dg_CameraDtor(g + DG_CAMERA);
	dg_WorldDtor(g + DG_WORLD);
	dg_CharLibDtor(g + DG_CHARLIB);
}

/* 0x000e7f90  Demolition/CDemolition_Game.cpp (file inferred)
Called by the game mode before Initialise: loads the character voices and the variation's
package "packages\demo-v<n>" and then the shared "packages\demo-gen". */
void CDemolitionGame_LoadPackages(int self)
{
	char *g = (char *)(uintptr_t)self;
	char name[256];
	void *package;

	DA_LoadCharacterVoices(1);
	sprintf(name, "packages\\demo-v%d", DA_FIELD(int, g, DG_VARIATION) + 1);
	package = DA_AddPackage(name, 0);
	DA_FIELD(void *, g, DG_PACKAGE) = package;
	if (package == NULL)
		return;
	DA_FIELD(unsigned int, g, DG_PACKAGEID) = DA_PackageId(package);
	sprintf(name, "packages\\demo-gen");
	package = DA_AddPackage(name, 0);
	DA_FIELD(void *, g, DG_GENPACKAGE) = package;
	if (package != NULL)
		DA_FIELD(unsigned int, g, DG_GENPACKAGEID) = DA_PackageId(package);
}

/* 0x000e8010  Demolition/CDemolition_Game.cpp (missing from the function table; called by the
game mode, 0x00101a56)
When the mode loses focus (ACTIVE == 0) while its pause menu is up, the mode's slot 9 runs. */
void FUN_000e8010(void *self, int active)
{
	void *mode = DA_FIELD(void *, self, DG_MODE);

	if (active == 0 && DA_FIELD(int, mode, 0x14) != 0)
		DA_VFN(mode, 0x24, void (*)(void *))(mode);
}

/* the per-frame update of every subsystem (shared tail of 0x000e8030 and 0x000e8140) */
static void dg_UpdateSystems(char *g, float dt)
{
	void *variation = DA_FIELD(void *, g, DG_VARIATIONOBJ);

	dg_EntitiesUpdate(g + DG_ENTITIES, dt);
	dg_CharLibUpdate(g + DG_CHARLIB, dt);
	dg_BombsUpdate(g + DG_BOMBS, dt);
	dg_ExplosionsUpdate(g + DG_EXPLOSIONS, dt);
	CDemoTokenSystem_Update(g + DG_TOKENS, dt);
	DA_VCALL(variation)->update(variation, dt);
	dg_CameraUpdate(g + DG_CAMERA, dt);
	CDemoOverlay_Update(g + DG_OVERLAY, dt);
	CDemoSpotFX_Update(g + DG_SPOTFX, dt);
	CDemoObjectList_Update(g + DG_PARTICLES, dt);
	CDemoObjectList_Update(g + DG_DECALS, dt);
	CDemoLightSystem_Update(g + DG_LIGHTS, dt);
	dg_WorldUpdate(g + DG_WORLD, dt);
	CDemoPlatforms_Update(g + DG_PLATFORMS, dt);
	CDemoInfo_Update(g + DG_INFO, dt);
	dg_RumbleUpdate(g + DG_RUMBLE, dt);
	CDemoRespawn_Update(g + DG_RESPAWN, dt);
}

/* 0x000e8030  Demolition/CDemolition_Game.cpp (file inferred)
A frame of the intro state (1): runs while the camera's intro is going (states 1-2); when it
reaches state 3 the play starts (state 2, the countdown starts, variation slot 3). */
void CDemolitionGame_UpdateIntro(void *self, float dt)
{
	char *g = (char *)self;
	int cameraState = DA_FIELD(int, g, DG_CAMERAMODE);
	void *variation;

	if (cameraState <= 0)
		return;
	if (cameraState > 2)
	{
		if (cameraState != 3)
			return;
		DA_FIELD(short, g, DG_STATE) = 2;
		DA_TimerStart(g + DG_COUNTDOWN);
		variation = DA_FIELD(void *, g, DG_VARIATIONOBJ);
		DA_VCALL(variation)->start(variation);
	}
	dg_UpdateSystems(g, dt);
}

/* 0x000e8140  Demolition/CDemolition_Game.cpp (file inferred)
A frame of the play and results states (2-4). */
void CDemolitionGame_UpdatePlay(void *self, float dt)
{
	dg_UpdateSystems((char *)self, dt);
}

/* 0x000e8220  Demolition/CDemolition_Game.cpp (file inferred)
Sets the fog, the ambient light and the clear colour of the current variation. */
void CDemolitionGame_SetAtmosphere(void)
{
	const struct DA_VariationSettings *s;

	s = &DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)];
	if (s->fog == 0)
	{
		DA_SetRenderState(8, 0, 0);
	}
	else
	{
		DA_SetRenderState(8, 1, 0);
		s = &DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)];
		DA_SetFogColour(s->fogR, s->fogG, s->fogB);
		s = &DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)];
		DA_SetFogRange(s->fogStart, s->fogEnd);
	}
	s = &DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)];
	DA_SetAmbientLight(s->ambR, s->ambG, s->ambB);
	DAT_002ba940 = DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)].clearR;
	DAT_002ba944 = DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)].clearG;
	DAT_002ba948 = DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)].clearB;
}

/* ---------- shared variation steps */

/* sets every player to "can act" (active, can move, under control) */
static void dg_EnablePlayers(void)
{
	char *p;

	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_FIELD(char, p, DP_ACTIVE) = 1;
		DA_FIELD(char, p, DP_CANMOVE) = 1;
		DA_FIELD(char, p, DP_CONTROL) = 1;
	}
}

/* the intro pictures (variations' Begin): player I gets picture I of the game's table */
static void dg_BeginIntroPictures(void)
{
	char *p;
	int i;

	for (i = 0; i < 4; i++)
	{
		p = DA_FIELD(char *, DA_GAME, DG_PLAYERBYINDEX + i * 4);
		DA_PlayerSetCharacter(p, DA_GAME + 0x7c0 + i * 0x20);
		DA_Nop(p);
	}
}

/* the reset sequence before a round's intro (0x000e8786 tails): no player under control, the
   characters' turn angles set to 3/4 pi */
static void dg_FacePlayers(void)
{
	char *p;
	float angle;
	uint32_t bits = 0x4016cbe4;

	memcpy(&angle, &bits, sizeof(angle));
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_FIELD(char, p, DP_CONTROL) = 0;
		DA_FIELD(float, DA_FIELD(char *, p, DP_ACTOR), 0x1d8) = angle;
		DA_FIELD(float, DA_FIELD(char *, p, DP_ACTOR), 0x1e8) = angle;
	}
}

/* the winners by score (not a team game): no player alive or under control any more, winner =
   highest score */
static void dg_WinnersByScore(void)
{
	char *p;
	int best;

	p = DA_PLAYER_FIRST();
	best = DA_FIELD(int, p, DP_SCORE);
	for (; p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_FIELD(char, p, DP_CONTROL) = 0;
		DA_FIELD(char, p, DP_ALIVE) = 0;
		if (best <= DA_FIELD(int, p, DP_SCORE))
			best = DA_FIELD(int, p, DP_SCORE);
	}
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
		DA_FIELD(char, p, DP_WINNER) = best <= DA_FIELD(int, p, DP_SCORE);
}

/* the winners of a team game: the team (a player and DP_PARTNER) with the higher total.
   OTHER is the first player of the other team; when the list holds no such player the
   original used whatever its local held (OTHER_DEFAULT). ALIVE_ONLY: only living players can
   win (variation 3), and the first loop clears DP_WINNER instead of DP_ALIVE. */
static void dg_WinnersByTeam(char *other_default, int alive_only)
{
	char *first, *p, *other;
	int teamA, teamB;

	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_FIELD(char, p, DP_CONTROL) = 0;
		if (alive_only)
			DA_FIELD(char, p, DP_WINNER) = 0;
		else
			DA_FIELD(char, p, DP_ALIVE) = 0;
	}
	first = DA_PLAYER_FIRST();
	other = other_default;
	for (p = first; p != NULL; p = DA_PLAYER_NEXT(p))
	{
		if (p != first && p != DA_FIELD(char *, first, DP_PARTNER))
		{
			other = p;
			break;
		}
	}
	teamA = DA_FIELD(int, DA_FIELD(char *, first, DP_PARTNER), DP_SCORE) + DA_FIELD(int, first, DP_SCORE);
	teamB = DA_FIELD(int, other, DP_SCORE) + DA_FIELD(int, DA_FIELD(char *, other, DP_PARTNER), DP_SCORE);
	for (p = first; p != NULL; p = DA_PLAYER_NEXT(p))
	{
		if (!alive_only || DA_FIELD(char, p, DP_ALIVE) != 0)
			DA_FIELD(char, p, DP_WINNER) = 1;
	}
	if (teamB < teamA)
	{
		DA_FIELD(char, other, DP_WINNER) = 0;
		DA_FIELD(char, DA_FIELD(char *, other, DP_PARTNER), DP_WINNER) = 0;
	}
	else if (teamA < teamB)
	{
		DA_FIELD(char, first, DP_WINNER) = 0;
		DA_FIELD(char, DA_FIELD(char *, first, DP_PARTNER), DP_WINNER) = 0;
	}
}

/* the end of a round: results state (3), the winners' pictures, the results time (7 s for a
   single party winner, else 5 s per winner), variation slots 4 and 10, the winner text */
static void dg_EndRound(void *self)
{
	char *p;
	int winners = 0;
	int scores[4];

	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_FIELD(char, p, DP_CANMOVE) = 0;
		if (DA_FIELD(char, p, DP_WINNER) != 0)
			winners++;
	}
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
		DA_PlayerSetCharacter(p, DA_GAME + (DA_FIELD(int, p, DP_INDEX) + 0x3e) * 0x20);
	DA_FIELD(short, DA_GAME, DG_STATE) = 3;
	DA_TimerStop(DA_GAME + DG_COUNTDOWN);
	DA_CameraSetMode(DA_GAME + DG_CAMERA, 4);
	if (DAT_002bae98 == 1 && winners == 1)
		DA_FIELD(float, DA_GAME, DG_RESULTSTIME) = 7.0f;
	else
		DA_FIELD(float, DA_GAME, DG_RESULTSTIME) = (float)winners * 5.0f;
	DA_TimerSet(DA_GAME + DG_STATETIMER, DA_FIELD(float, DA_GAME, DG_RESULTSTIME));
	DA_TimerStart(DA_GAME + DG_STATETIMER);
	DA_VCALL(self)->finish(self);
	DA_VCALL(self)->scores(self);
	scores[0] = DA_PLAYER_POINTS(0);
	scores[1] = DA_PLAYER_POINTS(1);
	scores[2] = DA_PLAYER_POINTS(2);
	scores[3] = DA_PLAYER_POINTS(3);
	DA_HudCreateWinnerText(scores, 0);
}

/* copies the scores into the floats the HUD shows (start of every variation update) */
static void dg_ShowScores(void)
{
	char *p;

	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
		DA_FIELD(float, p, DP_SHOWNSCORE) = (float)DA_FIELD(int, p, DP_SCORE);
}

/* the shared state switch of the variation updates */
static void dg_UpdateState(void *self, float dt, void (*play)(void *, float))
{
	switch (DA_FIELD(short, DA_GAME, DG_STATE))
	{
	case 1:
		CDemoVar_UpdateIntro(self, dt);
		return;
	case 2:
		play(self, dt);
		return;
	case 3:
		CDemoVar_UpdateResults(self, dt);
		return;
	case 4:
		if (DA_TimerGet(DA_GAME + DG_STATETIMER) <= 0.0f)
			DA_FIELD(char, DA_GAME, DG_QUIT) = 1;
		return;
	}
}

/* ---------- variation 0 (frag) and shared slots */

/* 0x000e8300  Demolition/CDemolition_Game.cpp
Variation 0 and 3 slot 2 (Reset): half a second of state time, the street ambience. */
void CDemoVar0_Reset(void *self)
{
	(void)self;
	DA_TimerSet(DA_GAME + DG_STATETIMER, 0.5f);
	DA_TimerStart(DA_GAME + DG_STATETIMER);
	DA_FIELD(int, DA_GAME, DG_STARTING) = 1;
	DA_PlaySfx(DA_GAME + DG_SFX, 0xd, 0x7f, 0, 0, 0);
}

/* 0x000e8360  Demolition/CDemolition_Game.cpp
Variation 0 slot 3 (Start): every player can act. */
void CDemoVar0_Start(void *self)
{
	(void)self;
	dg_EnablePlayers();
}

/* 0x000e8390  Demolition/CDemolition_Game.cpp
Variation 0 slot 4 (Finish): the bombs stop. */
void CDemoVar0_Finish(void *self)
{
	(void)self;
	DA_BombsStop(DA_GAME + DG_BOMBS);
}

/* 0x000e83b0  Demolition/CDemolition_Game.cpp
Variation 0 slot 5 (PostFinish): every player puts the bomb in hand down and the bombs are
disarmed; the characters are free again. */
void CDemoVar0_PostFinish(void *self)
{
	char *p;

	(void)self;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_PlayerPlaceBomb(p, DA_FIELD(void *, DA_FIELD(char *, p, DP_ACTOR), 0x300),
			DA_FIELD(int, p, DP_HAND));
		DA_BombDisarm(DA_FIELD(void *, p, DP_BOMB));
	}
	DA_CharLibSetFrozen(DA_GAME + DG_CHARLIB, 0);
}

/* 0x000e8400  Demolition/CDemolition_Game.cpp (missing from the function table; tail-called by
variation 0's Update, 0x000eb552)
Variation 0, play state: when the countdown ends, the round is over (players holding a
detonator drop their bomb). */
void FUN_000e8400(void *self, float dt)
{
	char *p;

	(void)dt;
	if (!(DA_TimerGet(DA_GAME + DG_COUNTDOWN) <= 0.0f))
		return;
	DA_CharLibSetFrozen(DA_GAME + DG_CHARLIB, 1);
	DA_FIELD(char, DA_GAME, DG_ROUNDOVER) = 1;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		if (DA_FIELD(int, p, DP_DETONATOR) != 0)
			DA_PlayerDropBomb(p);
	}
	/* the original's "other team" fallback is an uninitialised local; NULL here */
	if (DAT_002baebc == 0)
		dg_WinnersByScore();
	else
		dg_WinnersByTeam(NULL, 0);
	dg_EndRound(self);
}

/* 0x000e86e0  Demolition/CDemolition_Game.cpp
Variation 0 slot 9 (Begin): the intro; the team data is reset and each player gets the picture
of its character (0x41 - character). */
void CDemoVar0_Begin(void *self)
{
	char *p;
	int i;

	(void)self;
	DA_FIELD(short, DA_GAME, DG_STATE) = 1;
	DA_CameraSetMode(DA_GAME + DG_CAMERA, 1);
	DA_TeamData_Reset();
	for (i = 0; i < 4; i++)
	{
		p = DA_FIELD(char *, DA_GAME, DG_PLAYERBYINDEX + i * 4);
		DA_PlayerSetCharacter(p, DA_GAME + (0x41 - DAT_002b9c58[i]) * 0x20);
		DA_Nop(p);
	}
	dg_FacePlayers();
}

/* ---------- variation 1 (remote) */

/* 0x000e8790  Demolition/CDemolition_Game.cpp
Variation 1 destructor. */
void CDemoVar1_Dtor(void *self)
{
	*(const void *const **)self = vtable_00269b14;
	dg_RemoteWorldDtor(DA_AT(self, DV_WORLD1));
}

/* 0x000e87a0  Demolition/CDemolition_Game.cpp
Variation 1 slot 2 (Reset): half a second of state time, the ambience louder. */
void CDemoVar1_Reset(void *self)
{
	(void)self;
	DA_TimerSet(DA_GAME + DG_STATETIMER, 0.5f);
	DA_TimerStart(DA_GAME + DG_STATETIMER);
	DA_FIELD(int, DA_GAME, DG_STARTING) = 1;
	DA_PlaySfx(DA_GAME + DG_SFX, 0xd, 0xc0, 0, 0, 0);
}

/* 0x000e8800  Demolition/CDemolition_Game.cpp
Variation 1 slot 3 (Start): every player can act; the remote world starts. */
void CDemoVar1_Start(void *self)
{
	dg_EnablePlayers();
	FUN_000fcb90(DA_AT(self, DV_WORLD1));
}

/* 0x000e8840  Demolition/CDemolition_Game.cpp
Variation 1 slot 4 (Finish): the bombs and the remote world stop. */
void CDemoVar1_Finish(void *self)
{
	DA_BombsStop(DA_GAME + DG_BOMBS);
	dg_RemoteWorldStop(DA_AT(self, DV_WORLD1));
}

/* 0x000e8860  Demolition/CDemolition_Game.cpp
Variation 1 slot 5 (PostFinish): every player lets go of the remote bomb in hand; the
characters are free again. */
void CDemoVar1_PostFinish(void *self)
{
	char *p;

	(void)self;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_PlayerHold(p, DA_FIELD(void *, DA_FIELD(char *, p, DP_ACTOR), 0x30c), DA_FIELD(void *, p, DP_HAND),
			DA_AT(p, DP_HANDPOS), DA_FIELD(int, p, DP_HANDSLOT));
	}
	DA_CharLibSetFrozen(DA_GAME + DG_CHARLIB, 0);
}

/* 0x000e88c0  Demolition/CDemolition_Game.cpp
Variation 1, play state: when the countdown ends, everybody celebrates and the round is over. */
void CDemoVar1_UpdatePlay(void *self, float dt)
{
	char *p;

	(void)dt;
	if (!(DA_TimerGet(DA_GAME + DG_COUNTDOWN) <= 0.0f))
		return;
	DA_CharLibSetFrozen(DA_GAME + DG_CHARLIB, 1);
	DA_FIELD(char, DA_GAME, DG_ROUNDOVER) = 1;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
		DA_PlayerCelebrate(p);
	if (DAT_002baebc == 0)
		dg_WinnersByScore();
	else
		dg_WinnersByTeam(NULL, 0);
	dg_EndRound(self);
}

/* 0x000e8ba0  Demolition/CDemolition_Game.cpp
Variation 1 slot 7 (Draw): the remote world's draw (a tail call into demolition_c). */
void CDemoVar1_Draw(void *self)
{
	FUN_000fd540(DA_AT(self, DV_WORLD1));
}

/* 0x000e8bb0  Demolition/CDemolition_Game.cpp
Variation 1 slot 9 (Begin): the intro; each player gets its own picture. */
void CDemoVar1_Begin(void *self)
{
	DA_FIELD(short, DA_GAME, DG_STATE) = 1;
	DA_CameraSetMode(DA_GAME + DG_CAMERA, 1);
	dg_RemoteWorldBegin(DA_AT(self, DV_WORLD1));
	dg_BeginIntroPictures();
	dg_FacePlayers();
}

/* 0x000e8c50  Demolition/CDemolition_Game.cpp
Variation 1 slot 11 (Damage): an explosion at POS damages the remote world's walls. */
void CDemoVar1_Damage(void *self, float *pos, float radius)
{
	FUN_000fd180(DA_AT(self, DV_WORLD1), pos, radius);
}

/* 0x000e8c60  Demolition/CDemolition_Game.cpp
Variation 1 slot 13: the player area region. */
void *CDemoVar1_GetArea(void *self)
{
	return DA_FIELD(void *, self, DV_AREA1);
}

/* ---------- variation 2 (mine) */

/* 0x000e8c70  Demolition/CDemolition_Game.cpp
Variation 2 destructor. */
void CDemoVar2_Dtor(void *self)
{
	*(const void *const **)self = vtable_00269b50;
	dg_MineWorldDtor(DA_AT(self, DV_WORLD2));
}

/* 0x000e8c80  Demolition/CDemolition_Game.cpp
Variation 2 slot 2 (Reset): half a second of state time; the industrial ambience. */
void CDemoVar2_Reset(void *self)
{
	(void)self;
	DA_TimerSet(DA_GAME + DG_STATETIMER, 0.5f);
	DA_TimerStart(DA_GAME + DG_STATETIMER);
	DA_FIELD(int, DA_GAME, DG_STARTING) = 1;
	DA_PlaySfx(DA_GAME + DG_SFX, 0x17, 0xff, 0, 0, 0);
}

/* 0x000e8ce0  Demolition/CDemolition_Game.cpp
Variation 2 slot 3 (Start): every player can act, the bombs start, the tokens are enabled and
the mine world starts. */
void CDemoVar2_Start(void *self)
{
	dg_EnablePlayers();
	DA_BombsStart(DA_GAME + DG_BOMBS);
	CDemoTokenSystem_Enable(DA_GAME + DG_TOKENS);
	FUN_000fe610(DA_AT(self, DV_WORLD2));
}

/* 0x000e8d40  Demolition/CDemolition_Game.cpp
Variation 2 slot 4 (Finish): the bombs, the tokens and the mine world stop. */
void CDemoVar2_Finish(void *self)
{
	DA_BombsStop(DA_GAME + DG_BOMBS);
	CDemoTokenSystem_Stop(DA_GAME + DG_TOKENS);
	dg_MineWorldStop(DA_AT(self, DV_WORLD2));
}

/* 0x000e8d70  Demolition/CDemolition_Game.cpp
Variation 2, play state: when the countdown ends, the round is over. */
void CDemoVar2_UpdatePlay(void *self, float dt)
{
	(void)dt;
	if (!(DA_TimerGet(DA_GAME + DG_COUNTDOWN) <= 0.0f))
		return;
	DA_CharLibSetFrozen(DA_GAME + DG_CHARLIB, 1);
	DA_FIELD(char, DA_GAME, DG_ROUNDOVER) = 1;
	if (DAT_002baebc == 0)
		dg_WinnersByScore();
	else
		dg_WinnersByTeam(NULL, 0);
	dg_EndRound(self);
}

/* 0x000e9040  Demolition/CDemolition_Game.cpp
Variation 2 slot 7 (Draw): nothing (the mine world's empty draw). */
void CDemoVar2_Draw(void *self)
{
	DA_Nop(DA_AT(self, DV_WORLD2));
}

/* 0x000e9050  Demolition/CDemolition_Game.cpp
Variation 2 slot 8 (DrawOverlay): the mine world's overlay. */
void CDemoVar2_DrawOverlay(void *self)
{
	dg_MineWorldDraw(DA_AT(self, DV_WORLD2));
}

/* 0x000e9060  Demolition/CDemolition_Game.cpp
Variation 2 slot 9 (Begin): the intro; each player gets its own picture; the mine world
begins. */
void CDemoVar2_Begin(void *self)
{
	DA_FIELD(short, DA_GAME, DG_STATE) = 1;
	DA_CameraSetMode(DA_GAME + DG_CAMERA, 1);
	dg_BeginIntroPictures();
	dg_FacePlayers();
	dg_MineWorldBegin(DA_AT(self, DV_WORLD2));
}

/* 0x000e9100  Demolition/CDemolition_Game.cpp
Variation slot 10 (StoreScores) of variations 0-2: each player's score becomes its points. */
void CDemoVar_StoreScores(void *self)
{
	char *p;
	int i = 0;

	(void)self;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_PLAYER_POINTS(i) = DA_FIELD(int, p, DP_SCORE);
		i++;
	}
}

/* 0x000e9130  Demolition/CDemolition_Game.cpp
Variation 2 slot 12: region INDEX (0-3) of the mine. */
void *CDemoVar2_GetRegion(void *self, int index)
{
	return DA_FIELD(void *, self, DV_REGIONS2 + index * 4);
}

/* ---------- variation 3 (cross) */

/* 0x000e9140  Demolition/CDemolition_Game.cpp
Variation 3 constructor. */
void *CDemoVar3_Ctor(void *self)
{
	*(const void *const **)self = vtable_00269b8c;
	CDemoCountdown_Ctor((unsigned int *)DA_AT(self, DV_TIMER3));
	dg_CrossWorldCtor(DA_AT(self, DV_WORLD3));
	return self;
}

/* 0x000e9190  Demolition/CDemolition_Game.cpp
Variation 3 destructor. */
void CDemoVar3_Dtor(void *self)
{
	*(const void *const **)self = vtable_00269b8c;
	dg_CrossWorldDtor(DA_AT(self, DV_WORLD3));
	CDemoCountdown_Dtor((unsigned int *)DA_AT(self, DV_TIMER3));
}

/* 0x000e91f0  Demolition/CDemolition_Game.cpp
Variation 3 slot 0 (Init). */
int CDemoVar3_Init(void *self, void *package)
{
	dg_CrossWorldInit(DA_AT(self, DV_WORLD3), package);
	CDemoCountdown_Init((int)(uintptr_t)DA_AT(self, DV_TIMER3));
	DA_VCALL(self)->begin(self);
	return 1;
}

/* 0x000e9220  Demolition/CDemolition_Game.cpp
Variation 3 slot 1 (Shutdown). */
void CDemoVar3_Shutdown(void *self)
{
	dg_CrossWorldShutdown(DA_AT(self, DV_WORLD3));
	DA_Nop(DA_AT(self, DV_TIMER3));
}

/* 0x000e9240  Demolition/CDemolition_Game.cpp
Variation 3 slot 3 (Start): every player can act, the bombs start, and the variation's own
timer starts. */
void CDemoVar3_Start(void *self)
{
	dg_EnablePlayers();
	DA_BombsStart(DA_GAME + DG_BOMBS);
	DA_TimerStart(DA_AT(self, DV_TIMER3));
}

/* 0x000e9290  Demolition/CDemolition_Game.cpp
Variation 3 slot 4 (Finish): the cross world and the bombs stop. */
void CDemoVar3_Finish(void *self)
{
	dg_CrossWorldStop(DA_AT(self, DV_WORLD3));
	DA_BombsStop(DA_GAME + DG_BOMBS);
}

/* 0x000e92b0  Demolition/CDemolition_Game.cpp
Variation 2 and 3 slot 5 (PostFinish): the characters are free again. */
void CDemoVar_Unfreeze(void *self)
{
	(void)self;
	DA_CharLibSetFrozen(DA_GAME + DG_CHARLIB, 0);
}

/* 0x000e92d0  Demolition/CDemolition_Game.cpp
The results state of every variation: after two seconds of results a winner who has not
finished celebrating, or (when there is none) any present player, can press a button to leave;
the state also ends when the time runs out and the commentary is silent. Leaving: state 4 for
0.7 seconds. */
void CDemoVar_UpdateResults(void *self, float dt)
{
	char *g = DA_GAME;
	char *first, *p;
	float t;
	int skip = 0;
	int i;

	(void)self;
	(void)dt;
	t = DA_TimerGet(g + DG_STATETIMER);
	g = DA_GAME;
	if (t < DA_FIELD(float, g, DG_RESULTSTIME) - 2.0f)
	{
		first = DA_FIELD(char *, g, DG_PLAYERS);
		for (p = first; p != NULL; p = DA_PLAYER_NEXT(p))
		{
			if (DA_FIELD(char, p, DP_WINNER) != 0 && DA_FIELD(char, p, DP_DONE) == 0)
				break;
		}
		if (p == NULL)
		{
			for (i = 0; i < 4; i++)
			{
				if (DA_PLAYER_PRESENT(i) != 0 && DA_FIELD(int, DA_FIELD(char *, g, DG_PLAYERFLAGS + i * 4), 0x14) != 0)
				{
					skip = 1;
					break;
				}
			}
		}
		else
		{
			i = 0;
			for (p = first; p != NULL; p = DA_PLAYER_NEXT(p), i++)
			{
				if (DA_FIELD(char, p, DP_WINNER) != 0 && DA_PLAYER_PRESENT(i) != 0 &&
					DA_FIELD(int, DA_FIELD(char *, g, DG_PLAYERFLAGS + i * 4), 0x14) != 0)
				{
					skip = 1;
					break;
				}
			}
		}
	}
	if ((t <= 0.0f && DA_CommentaryIdle() != 0) || skip)
	{
		DA_FIELD(short, DA_GAME, DG_STATE) = 4;
		DA_TimerSet(DA_GAME + DG_STATETIMER, 0.7f);
		DA_TimerStart(DA_GAME + DG_STATETIMER);
		DA_FIELD(int, DA_GAME, DG_ENDING) = 1;
	}
}

/* 0x000e9410  Demolition/CDemolition_Game.cpp
Variation 3 slot 7 (Draw): the cross world's draw (a tail call into demolition_c). */
void CDemoVar3_Draw(void *self)
{
	FUN_00100ef0(DA_AT(self, DV_WORLD3));
}

/* 0x000e9420  Demolition/CDemolition_Game.cpp
Variation 3 slot 8 (DrawOverlay): the cross world's overlay (a tail call into demolition_c). */
void CDemoVar3_DrawOverlay(void *self)
{
	FUN_00100f60(DA_AT(self, DV_WORLD3));
}

/* 0x000e9430  Demolition/CDemolition_Game.cpp
Variation 3 slot 10 (StoreScores): points by finishing place. For each place 1-4 that some
player holds, the players there get 10, 5, 3 or 1 bonus points (by how many places were
held before), added to the score of the player with the same list position. */
void CDemoVar3_StoreScores(void *self)
{
	char *first, *p;
	int place, rank = 0, bonus = 4, count, i;

	(void)self;
	first = DA_PLAYER_FIRST();
	for (place = 1; place <= 4; place++)
	{
		count = 0;
		if (first == NULL)
			continue;
		for (p = first; p != NULL; p = DA_PLAYER_NEXT(p))
		{
			if (DA_FIELD(int, p, DP_PLACE) == place)
				count++;
		}
		if (count == 0)
			continue;
		i = 0;
		for (p = first; p != NULL; p = DA_PLAYER_NEXT(p), i++)
		{
			if (DA_FIELD(int, p, DP_PLACE) != place)
				continue;
			switch (rank)
			{
			case 0:
				bonus = 10;
				break;
			case 1:
				bonus = 5;
				break;
			case 2:
				bonus = 3;
				break;
			case 3:
				bonus = 1;
				break;
			}
			DA_PLAYER_POINTS(i) = DA_FIELD(int, DA_FIELD(char *, DA_GAME, DG_PLAYERBYINDEX + i * 4), DP_SCORE) + bonus;
		}
		rank++;
	}
}

/* ---------- the game object */

/* 0x000e9510  Demolition/CDemolition_Game.cpp
The scalar deleting destructor of the game object. */
void *CDemolitionGame_DeletingDtor(void *self, unsigned int flags)
{
	CDemolitionGame_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000e9530  Demolition/CDemolition_Game.cpp
Starts variation VARIATION (1-based; the setting "variation" overrides it with a 0-based
number): pushes the Demolition game mode, then the minigame intro with "dem<n>.bmp". Returns
nonzero when both were pushed. */
int CDemolitionGame_Start(void *self, int variation)
{
	char *g = (char *)self;
	char picture[256];
	void *memory;
	void *mode;
	char *setting;

	memory = DA_New(0x28, CDemolition_Game_cpp, 199);
	mode = memory != NULL ? dg_ModeCtor(memory) : NULL;
	DA_FIELD(void *, g, DG_MODE) = mode;
	setting = DA_SettingsGet("variation");
	if (setting == NULL)
		DA_FIELD(int, g, DG_VARIATION) = variation - 1;
	else
		DA_FIELD(int, g, DG_VARIATION) = (int)atol(setting);
	if (DA_PushMode(mode, 1) == 0)
	{
		DA_FIELD(void *, g, DG_MODE) = NULL;
		return 0;
	}
	sprintf(picture, "dem%d.bmp", DA_FIELD(int, g, DG_VARIATION) + 1);
	memory = DA_New(0x300, CDemolition_Game_cpp, 0xdb);
	mode = memory != NULL ? DA_MiniGameIntroCtor(memory, DA_FIELD(int, g, DG_VARIATION) + 0x38, picture) : NULL;
	return DA_PushMode(mode, 1) != 0;
}

/* 0x000e9670  Demolition/CDemolition_Game.cpp
Shutdown (called by the game mode): when the mode did not start (its +4 is 0) only the
packages are closed. Otherwise every subsystem shuts down, the variation is deleted, the saved
engine settings come back and the packages' resources and the packages go. */
void CDemolitionGame_Shutdown(int self)
{
	char *g = (char *)(uintptr_t)self;
	void *variation;

	if (DA_FIELD(int, DA_FIELD(char *, g, DG_MODE), 4) == 0)
	{
		DA_ClosePackage(DA_FIELD(void *, g, DG_PACKAGE));
		DA_ClosePackage(DA_FIELD(void *, g, DG_GENPACKAGE));
		return;
	}
	DA_Nop(g + DG_DEBUG);
	CDemoSpotFX_Shutdown(g + DG_SPOTFX);
	DA_PSListDeleteAll(g + DG_PARTICLES);
	CDemoDecals_Shutdown(g + DG_DECALS);
	CDemoOverlay_Shutdown(g + DG_OVERLAY);
	CDemoLightSystem_Shutdown(g + DG_LIGHTS);
	DA_Nop(g + DG_TIMER);
	DA_Nop(g + DG_COUNTDOWN);
	DA_Nop(g + DG_SIMULATION);
	dg_EntitiesShutdown(g + DG_ENTITIES);
	dg_BombsShutdown(g + DG_BOMBS);
	CDemoExplosionSystem_Shutdown(g + DG_EXPLOSIONS);
	CDemoTokenSystem_Shutdown(g + DG_TOKENS);
	dg_WorldShutdown(g + DG_WORLD);
	CDemoPlatforms_Shutdown(g + DG_PLATFORMS);
	DA_CameraShutdown(g + DG_CAMERA);
	DA_Nop(g + DG_TEXTURES);
	dg_CharLibShutdown(g + DG_CHARLIB);
	DA_InfoShutdown(g + DG_INFO);
	DA_InputShutdown(g + DG_INPUT);
	DA_Nop(g + DG_DIRECT3D);
	CDemoSfx_Shutdown(g + DG_SFX);
	CDemoRumble_Shutdown(g + DG_RUMBLE);
	CDemoRespawn_Shutdown(g + DG_RESPAWN);
	variation = DA_FIELD(void *, g, DG_VARIATIONOBJ);
	if (variation != NULL)
	{
		DA_VCALL(variation)->shutdown(variation);
		DA_Delete(DA_FIELD(void *, g, DG_VARIATIONOBJ));
		DA_FIELD(void *, g, DG_VARIATIONOBJ) = NULL;
	}
	DAT_00292c60 = DA_FIELD(float, g, DG_SAVEDCLIP);
	DAT_00292c64 = DA_FIELD(float, g, DG_SAVEDCLIP + 4);
	DAT_00292c68 = DA_FIELD(float, g, DG_SAVEDCLIP + 8);
	DAT_00292c6c = DA_FIELD(float, g, DG_SAVEDCLIP + 12);
	DAT_00292c78 = 0x18;
	DA_DeletePackageResources(DA_FIELD(unsigned int, g, DG_PACKAGEID), 1);
	DA_DeletePackageResources(DA_FIELD(unsigned int, g, DG_GENPACKAGEID), 1);
	DA_ClosePackage(DA_FIELD(void *, g, DG_PACKAGE));
	DA_ClosePackage(DA_FIELD(void *, g, DG_GENPACKAGE));
	DAT_002ba940 = DA_FIELD(int, g, DG_SAVEDFOG);
	DA_FIELD(char, g, DG_QUIT) = 0;
	DA_FIELD(char, g, DG_ROUNDOVER) = 0;
	DAT_002ba944 = DA_FIELD(int, g, DG_SAVEDFOG + 4);
	DAT_002ba948 = DA_FIELD(int, g, DG_SAVEDFOG + 8);
	DAT_002ba94c = DA_FIELD(int, g, DG_SAVEDFOG + 12);
	DAT_0028ffa8 = DA_FIELD(float, g, DG_SAVEDFAR);
	DAT_0028ffac = DA_FIELD(float, g, DG_SAVEDFAR + 4);
	DA_SetRenderState(8, 0, 0);
	DA_RumbleStopAll();
}

/* 0x000e9870  Demolition/CDemolition_Game.cpp (missing from the function table; called by the
game mode, 0x00101a3a)
Reset before a round: saves and clears the clear colour, saves the far planes and sets them
for the city, sets the atmosphere, the clip planes (1 - 70000), clears the pause, quit and
round-over flags, resets the HUD and the clock, then the variation's slot 2. */
void FUN_000e9870(void *self)
{
	char *g = (char *)self;
	void *variation;

	DA_FIELD(int, g, DG_SAVEDFOG) = DAT_002ba940;
	DA_FIELD(int, g, DG_SAVEDFOG + 4) = DAT_002ba944;
	DA_FIELD(int, g, DG_SAVEDFOG + 8) = DAT_002ba948;
	DA_FIELD(int, g, DG_SAVEDFOG + 12) = DAT_002ba94c;
	DAT_002ba940 = 0;
	DAT_002ba944 = 0;
	DAT_002ba948 = 0;
	DAT_002ba94c = 0;
	DA_FIELD(float, g, DG_SAVEDFAR) = DAT_0028ffa8;
	DA_FIELD(float, g, DG_SAVEDFAR + 4) = DAT_0028ffac;
	DAT_0028ffa8 = 4000.0f;
	DAT_0028ffac = 15000.0f;
	CDemolitionGame_SetAtmosphere();
	DA_SetNearFar(1.0f, 70000.0f);
	DA_FIELD(char, g, DG_PAUSED) = 0;
	DA_FIELD(char, g, DG_QUIT) = 0;
	DA_FIELD(char, g, DG_ROUNDOVER) = 0;
	CDemoOverlay_Reset(g + DG_OVERLAY);
	DA_TimerReset(g + DG_TIMER);
	variation = DA_FIELD(void *, g, DG_VARIATIONOBJ);
	DA_VCALL(variation)->reset(variation);
}

/* 0x000e9920  Demolition/CDemolition_Game.cpp (missing from the function table; called by the
game mode, 0x00101a66)
PollPreRender: a frame while ACTIVE. Applies a changed character colour set, then (unless the
HUD's pause screen holds the game) updates the input; when not paused: the clock, the frame
step (capped at 2, x1.5 with the turbo cheat) and the state's update; a missing controller
pauses. When the game is to quit, the variation stores the scores and the mode is popped. */
void FUN_000e9920(void *self, int active)
{
	char *g = (char *)self;
	char *p;
	float dt;
	void *variation;

	if (active == 0)
		return;
	if (DAT_00283ea4 != DA_FIELD(int, g, DG_CHARCOLOURS))
	{
		p = DA_FIELD(char *, g, DG_PLAYERS);
		DA_FIELD(int, g, DG_CHARCOLOURS) = DAT_00283ea4;
		for (; p != NULL; p = DA_PLAYER_NEXT(p))
		{
			DA_SetCharacterColours(DA_FIELD(char *, p, DP_ACTOR) + 0x10, DAT_00283ea4);
			DA_PrepActorInstance(DA_FIELD(char *, p, DP_ACTOR) + 0x10, 1);
		}
	}
	if (DA_FIELD(int, g, DG_PAUSEB) == 0 && DA_FIELD(int, g, DG_PAUSEA) == 0 &&
		DA_HudCanPause(DAT_002ecd90) != 0)
		return;
	DA_CharLibUpdateInput(g + DG_CHARLIB);
	if (DA_FIELD(char, g, DG_PAUSED) != 0)
	{
		DA_CameraPause(g + DG_CAMERA, 1.0f);
		if (DAT_002ecdec == 0)
			DA_FIELD(char, g, DG_PAUSED) = 0;
		return;
	}
	DA_Nop(g + DG_INPUT);
	DA_ClockTick(g + DG_TIMER);
	dt = DA_FIELD(float, g, DG_ROUNDTIME);
	if (!(dt <= 2.0f))
		dt = 2.0f;
	DA_TimerUpdate(g + DG_COUNTDOWN);
	if (DAT_00283e58 != 0)
		dt = dt * 1.5f;
	switch (DA_FIELD(short, g, DG_STATE))
	{
	case 1:
		CDemolitionGame_UpdateIntro(g, dt);
		break;
	case 2:
	case 3:
	case 4:
		CDemolitionGame_UpdatePlay(g, dt);
		break;
	default:
		DA_Printf("*** ERROR : CDemolition_Game::PollPreRender: Unhandled game state!");
		break;
	}
	if (DAT_002ecdec != 0)
		DA_FIELD(char, g, DG_PAUSED) = 1;
	if (DA_FIELD(char, g, DG_QUIT) == 0)
		return;
	variation = DA_FIELD(void *, g, DG_VARIATIONOBJ);
	DA_VCALL(variation)->scores(variation);
	DA_PopMode(1);
}

/* ---------- variation Init / Shutdown / Update */

/* 0x000e9cb0  Demolition/CDemolition_Game.cpp
Variation 0 scalar deleting destructor (the class has no members to destroy). */
void *CDemoVar0_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00269ad8;
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000e9cd0  Demolition/CDemolition_Game.cpp
Variation 0 slot 0 (Init): the route heights "frag\demo_frag.hmp", the barrier region (shape
"Line01" of "frag\paths\frag_barriers.ase") and a red point light over the square. Returns 0
when the barrier file is missing. */
int CDemoVar0_Init(void *self, void *package)
{
	struct DG_ASE ase;
	struct DG_ASEShape shape;
	char name[1024];
	struct DA_Vec4 lightPos;
	unsigned int size;
	void *memory, *heights, *data, *region, *light;

	DA_ASECtor(&ase);
	DA_ASEListCtor(&shape);
	memory = DA_New(8, CDemolition_Game_cpp, 0x5d5);
	heights = memory != NULL ? DA_RouteHeightsCtor(memory) : NULL;
	DA_FIELD(void *, self, DV_HEIGHTS) = heights;
	data = DA_LoadFile(package, "frag\\demo_frag.hmp", NULL, &size, NULL, 0);
	memcpy(*(void **)DA_FIELD(void *, self, DV_HEIGHTS), data, size);
	DA_RouteSetHeights(DA_FIELD(void *, self, DV_HEIGHTS));
	sprintf(name, "frag\\paths\\frag_barriers.ase");
	data = DA_LoadFile(package, name, NULL, &size, NULL, 0);
	if (data == NULL)
	{
		DA_ASEListDtor(&shape);
		DA_ASEDtor(&ase);
		return 0;
	}
	DA_ASEParse(&ase, data, size);
	memory = DA_New(0x40, CDemolition_Game_cpp, 0x5e7);
	region = memory != NULL ? DA_RegionCtor(memory) : NULL;
	DA_FIELD(void *, self, DV_BARRIERS) = region;
	DA_ASEFind(&shape, &ase, "Line01");
	DA_RegionBuild(DA_FIELD(void *, self, DV_BARRIERS), shape.points, shape.count);
	memory = DA_New(0x34, CDemolition_Game_cpp, 0x5f0);
	light = memory != NULL ? CDemoPointLight_Ctor(memory) : NULL;
	DA_FIELD(void *, self, DV_LIGHT) = light;
	((const struct CDemolition_LightVtbl *)DA_VTBL(light))->init(light);
	DA_LightSetColour(DA_FIELD(void *, self, DV_LIGHT), 0x7f, 0, 0);
	lightPos.x = -1308.0f;
	lightPos.y = 1600.0f;
	lightPos.z = -1308.0f;
	lightPos.w = 0.0f; /* not set by the original; SetPosition overwrites it with 1 */
	CDemoLight_SetPosition((int)(uintptr_t)DA_FIELD(void *, self, DV_LIGHT), (unsigned int *)&lightPos);
	DA_LightSetRange(DA_FIELD(void *, self, DV_LIGHT), 0.0004f);
	light = DA_FIELD(void *, self, DV_LIGHT);
	((const struct CDemolition_LightVtbl *)DA_VTBL(light))->enable(light, 1);
	CDemoLightSystem_Add(DA_GAME + DG_LIGHTS, DA_FIELD(void *, self, DV_LIGHT));
	DA_VCALL(self)->begin(self);
	DA_ASEListDtor(&shape);
	DA_ASEDtor(&ase);
	return 1;
}

/* 0x000e9f50  Demolition/CDemolition_Game.cpp
Variation 1 scalar deleting destructor. */
void *CDemoVar1_DeletingDtor(void *self, unsigned int flags)
{
	CDemoVar1_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000e9f70  Demolition/CDemolition_Game.cpp
Variation 1 slot 0 (Init): the player area (shape "player_area" of
"remote\paths\rcb_barriers.ase") and the remote world. Returns 0 when the file is missing. */
int CDemoVar1_Init(void *self, void *package)
{
	struct DG_ASE ase;
	struct DG_ASEShape shape;
	char name[1024];
	unsigned int size;
	void *memory, *data, *region;

	DA_ASECtor(&ase);
	DA_ASEListCtor(&shape);
	sprintf(name, "remote\\paths\\rcb_barriers.ase");
	data = DA_LoadFile(package, name, NULL, &size, NULL, 0);
	if (data == NULL)
	{
		DA_ASEListDtor(&shape);
		DA_ASEDtor(&ase);
		return 0;
	}
	DA_ASEParse(&ase, data, size);
	memory = DA_New(0x40, CDemolition_Game_cpp, 0x8ab);
	region = memory != NULL ? DA_RegionCtor(memory) : NULL;
	DA_FIELD(void *, self, DV_AREA1) = region;
	DA_ASEFind(&shape, &ase, "player_area");
	DA_RegionBuild(DA_FIELD(void *, self, DV_AREA1), shape.points, shape.count);
	dg_RemoteWorldInit(DA_AT(self, DV_WORLD1), package);
	DA_VCALL(self)->begin(self);
	DA_ASEListDtor(&shape);
	DA_ASEDtor(&ase);
	return 1;
}

/* deletes a region made by the Init functions (its point buffer at +0x38, then the object) */
static void dg_DeleteRegion(void **slot)
{
	void *region = *slot;

	if (region == NULL)
		return;
	if (DA_FIELD(void *, region, 0x38) != NULL)
	{
		DA_Delete(DA_FIELD(void *, region, 0x38));
		DA_FIELD(void *, *slot, 0x38) = NULL;
	}
	if (*slot != NULL)
		DA_VFN(*slot, 0x00, void *(*)(void *, unsigned int))(*slot, 1);
	*slot = NULL;
}

/* 0x000ea0e0  Demolition/CDemolition_Game.cpp
Variation 1 slot 1 (Shutdown). */
void CDemoVar1_Shutdown(void *self)
{
	dg_RemoteWorldShutdown(DA_AT(self, DV_WORLD1));
	dg_DeleteRegion((void **)DA_AT(self, DV_AREA1));
}

/* 0x000ea130  Demolition/CDemolition_Game.cpp
The intro state of every variation: while the camera flies in (camera state 1), the living
player the camera looks at most directly (within 70 degrees, nearest angle) cheers once (one of
two animations, then back to idle) when nearer than 2000. */
void CDemoVar_UpdateIntro(void *self, float dt)
{
	char *g = DA_GAME;
	char *p, *best = NULL;
	float camAngle, angle, delta, dist;
	float bestAngle = 99999.0f, bestDist = 99999.0f;
	int anim;

	(void)self;
	(void)dt;
	if (DA_FIELD(int, g, DG_CAMERAMODE) != 1)
		return;
	camAngle = atan2f(DA_FIELD(float, g, DG_CAMTARGET) - DA_FIELD(float, g, DG_CAMPOS),
		DA_FIELD(float, g, DG_CAMTARGET + 8) - DA_FIELD(float, g, DG_CAMPOS + 8));
	p = DA_FIELD(char *, g, DG_PLAYERS);
	if (p == NULL)
		return;
	for (; p != NULL; p = DA_PLAYER_NEXT(p))
	{
		if (DA_FIELD(char, p, DP_ALIVE) == 0 || DA_FIELD(short, DA_FIELD(char *, p, DP_ACTOR), 0x14a) == 0)
			continue;
		angle = atan2f(DA_FIELD(float, g, DG_CAMTARGET) - DA_FIELD(float, p, DP_POS),
			DA_FIELD(float, g, DG_CAMTARGET + 8) - DA_FIELD(float, p, DP_POS + 8));
		delta = DA_AngleDelta(angle, camAngle);
		g = DA_GAME;
		if (!(fabsf(delta) <= 1.22173047f))
			continue;
		dist = DA_VectorDistance((float *)DA_AT(p, DP_POS), (float *)(DA_GAME + DG_CAMTARGET));
		g = DA_GAME;
		if (fabsf(delta) < bestAngle)
		{
			bestAngle = fabsf(delta);
			best = p;
			bestDist = dist;
		}
	}
	if (best == NULL || DA_FIELD(char, best, DP_CHEERED) != 0 || !(bestDist < 2000.0f))
		return;
	anim = (float)da_rand() * DA_RAND_SCALE < 0.5f ? 0x22 : 0x21;
	DA_PlayAnim(DA_FIELD(void *, best, DP_ACTOR), anim, 0, 0, 1.0f, 0.01f, 1);
	DA_PlayAnim(DA_FIELD(void *, best, DP_ACTOR), 0, 1, 1, 1.0f, 0.01f, 1);
	DA_FIELD(char, best, DP_CHEERED) = 1;
}

/* 0x000ea330  Demolition/CDemolition_Game.cpp
Variation 2 scalar deleting destructor. */
void *CDemoVar2_DeletingDtor(void *self, unsigned int flags)
{
	CDemoVar2_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000ea350  Demolition/CDemolition_Game.cpp
Variation 2 slot 0 (Init): the four mine regions ("mine\paths\mine_region0<i>.ase", shape
"region0<i>"), the two steam regions of "mine\paths\steam_region.ase" ("smoke area", "smoke
area01") with a steam particle system each, and the mine world. Returns 0 when a file is
missing. */
int CDemoVar2_Init(void *self, void *package)
{
	struct DG_ASE ase;
	struct DG_ASEShape shape;
	char name[1024];
	unsigned int size;
	void *memory, *data, *region, *smoke;
	int i;

	DA_ASECtor(&ase);
	DA_ASEListCtor(&shape);
	for (i = 0; i < 4; i++)
	{
		sprintf(name, "mine\\paths\\mine_region0%d.ase", i);
		data = DA_LoadFile(package, name, NULL, &size, NULL, 0);
		if (data == NULL)
			goto failed;
		DA_ASEParse(&ase, data, size);
		memory = DA_New(0x40, CDemolition_Game_cpp, 0xb6a);
		region = memory != NULL ? DA_RegionCtor(memory) : NULL;
		DA_FIELD(void *, self, DV_REGIONS2 + i * 4) = region;
		sprintf(name, "region0%d", i);
		DA_ASEFind(&shape, &ase, name);
		DA_RegionBuild(DA_FIELD(void *, self, DV_REGIONS2 + i * 4), shape.points, shape.count);
	}
	sprintf(name, "mine\\paths\\steam_region.ase");
	data = DA_LoadFile(package, name, NULL, &size, NULL, 0);
	if (data == NULL)
		goto failed;
	for (i = 0; i < 2; i++)
	{
		DA_ASEParse(&ase, data, size);
		if (i == 0)
			sprintf(name, "smoke area");
		else if (i == 1)
			sprintf(name, "smoke area01");
		DA_ASEFind(&shape, &ase, name);
		memory = DA_New(0x40, CDemolition_Game_cpp, 0xb89);
		region = memory != NULL ? DA_RegionCtor(memory) : NULL;
		DA_FIELD(void *, self, DV_STEAM2 + i * 4) = region;
		DA_RegionBuild(region, shape.points, shape.count);
		memory = DA_New(0x6c, CDemolition_Game_cpp, 0xb8f);
		smoke = memory != NULL ? dg_SmokeCtor(memory) : NULL;
		DA_FIELD(void *, self, DV_SMOKES2 + i * 4) = smoke;
		dg_SmokeInit(smoke, 0, 0xf);
		DA_PSSetTexture(DA_FIELD(void *, self, DV_SMOKES2 + i * 4), DA_FIELD(void *, DA_GAME, DG_TEXTURES + 4 + 11 * 4));
		dg_SmokeSetRegion(DA_FIELD(void *, self, DV_SMOKES2 + i * 4), DA_FIELD(void *, self, DV_STEAM2 + i * 4));
		DA_PSListAdd(DA_GAME + DG_PARTICLES, DA_FIELD(void *, self, DV_SMOKES2 + i * 4));
		DA_FIELD(char, DA_FIELD(char *, self, DV_SMOKES2 + i * 4), 0x3c) = 1;
		DA_FIELD(char, DA_FIELD(char *, self, DV_SMOKES2 + i * 4), 0x3d) = 1;
	}
	dg_MineWorldInit(DA_AT(self, DV_WORLD2), package);
	DA_VCALL(self)->begin(self);
	DA_ASEListDtor(&shape);
	DA_ASEDtor(&ase);
	return 1;

failed:
	DA_ASEListDtor(&shape);
	DA_ASEDtor(&ase);
	return 0;
}

/* 0x000ea680  Demolition/CDemolition_Game.cpp
Variation 2 slot 1 (Shutdown): deletes the four regions and the two steam regions (the smoke
systems belong to the particle list), then the mine world. */
void CDemoVar2_Shutdown(void *self)
{
	int i;

	for (i = 0; i < 4; i++)
		dg_DeleteRegion((void **)DA_AT(self, DV_REGIONS2 + i * 4));
	for (i = 0; i < 2; i++)
		dg_DeleteRegion((void **)DA_AT(self, DV_STEAM2 + i * 4));
	dg_MineWorldShutdown(DA_AT(self, DV_WORLD2));
}

/* 0x000ea710  Demolition/CDemolition_Game.cpp
Variation 3 scalar deleting destructor. */
void *CDemoVar3_DeletingDtor(void *self, unsigned int flags)
{
	CDemoVar3_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* the screen shake of variation 3: random new shake offsets when each timer runs out */
static void dg_Var3Shake(void *self, float dt)
{
	float t;

	t = DA_FIELD(float, self, DV_SHAKE3) - dt;
	DA_FIELD(float, self, DV_SHAKE3) = t;
	if (t <= 0.0f)
	{
		DA_FIELD(float, self, DV_SHAKE3) = ((float)da_rand() * DA_RAND_SCALE + 0.5f) * 120.0f;
		DA_FIELD(float, DA_GAME, DG_SHAKEX) = (float)da_rand() * DA_RAND_SCALE * 400.0f - 200.0f;
	}
	t = DA_FIELD(float, self, DV_SHAKE3 + 4) - dt;
	DA_FIELD(float, self, DV_SHAKE3 + 4) = t;
	if (t <= 0.0f)
	{
		DA_FIELD(float, self, DV_SHAKE3 + 4) = ((float)da_rand() * DA_RAND_SCALE + 0.5f) * 80.0f;
		DA_FIELD(float, DA_GAME, DG_SHAKEZ) = (float)da_rand() * DA_RAND_SCALE * 40.0f - 20.0f;
	}
}

/* 0x000ea730  Demolition/CDemolition_Game.cpp
Variation 3, play state. The variation's own timer runs three phases: 0 (shaking, until the
timer is done: then 2 s of phase 1), 1 (until done: 5 s of phase 2), 2 (the lift rises, the
sky moves; at half time the cross world's lift starts; done: phase 3). Players whose score
reached 0 drop out (their place is the number still alive). The round ends when one player
(or one team of two) is left. */
void CDemoVar3_UpdatePlay(void *self, float dt)
{
	char *first, *p, *last;
	void *timer = DA_AT(self, DV_TIMER3);
	float frac, gain;
	int alive;

	switch (DA_FIELD(int, self, DV_STATE))
	{
	case 0:
		DA_TimerUpdate(timer);
		dg_Var3Shake(self, dt);
		if (!(DA_TimerFraction(timer) < 1.0f))
		{
			DA_FIELD(int, self, DV_STATE) = 1;
			DA_TimerSet(timer, 2.0f);
			DA_TimerStart(timer);
			DA_FIELD(float, DA_GAME, DG_SHAKEZ) = 0.0f;
			DA_FIELD(float, DA_GAME, DG_SHAKEX) = 0.0f;
			DA_FIELD(float, DA_FIELD(char *, DA_GAME, DG_VARIATIONOBJ), 0x16b4) = 10.0f;
		}
		break;
	case 1:
		DA_TimerUpdate(timer);
		if (!(DA_TimerFraction(timer) < 1.0f))
		{
			DA_FIELD(int, self, DV_STATE) = 2;
			DA_TimerSet(timer, 5.0f);
			DA_TimerStart(timer);
		}
		break;
	case 2:
		DA_TimerUpdate(timer);
		frac = DA_TimerFraction(timer);
		dt = frac; /* the original reuses the parameter's slot */
		if (!(frac < 1.0f))
			DA_FIELD(int, self, DV_STATE) = 3;
		if (!(frac < 0.5f) && DA_FIELD(char, self, DV_LIFTED3) == 0)
			dg_CrossWorldLift(DA_AT(self, DV_WORLD3));
		gain = DA_Gain(0.8f, dt);
		DA_FIELD(float, DA_GAME, DG_SKYB) = gain * -420.0f - (1.0f - gain) * 1500.0f;
		DA_FIELD(float, DA_GAME, DG_SKYA) = (1.0f - gain) * 1300.0f + gain * 2200.0f;
		break;
	}

	first = DA_PLAYER_FIRST();
	last = NULL;
	alive = 0;
	for (p = first; p != NULL; p = DA_PLAYER_NEXT(p))
	{
		last = p;
		if (DA_FIELD(char, p, DP_ALIVE) != 0)
			alive++;
	}
	for (p = first; p != NULL; p = DA_PLAYER_NEXT(p))
	{
		last = p;
		if (DA_FIELD(char, p, DP_ALIVE) == 0 || DA_FIELD(int, p, DP_SCORE) > 0)
			continue;
		DA_FIELD(char, p, DP_ALIVE) = 0;
		DA_FIELD(int, self, DA_FIELD(int, p, DP_INDEX) * 0x430 + 0x108) = 1;
		*DA_FIELD(char *, self, DA_FIELD(int, p, DP_INDEX) * 0x430 + 0x520) = 0;
		DA_FireShowPlayer(DA_FIELD(int, p, DP_INDEX), 0);
		if (alive >= 3)
			DA_PlayCommentary(0x37, DA_FIELD(int, p, DP_PAD), 0xff, 0, 0, NULL);
		DA_FIELD(uint32_t, DA_FIELD(char *, p, DP_ACTOR), 0x220) &= ~8u;
		DA_FIELD(char, p, DP_CANMOVE) = 0;
		DA_FIELD(char, p, DP_CONTROL) = 0;
		DA_FIELD(char, p, DP_BLOCKED) = 0;
		DA_FIELD(int, p, DP_PLACE) = alive;
	}
	if (DAT_002baebc != 0 && alive == 2)
	{
		for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
		{
			if (DA_FIELD(char, p, DP_ALIVE) != 0)
			{
				last = p;
				break;
			}
		}
		p = DA_FIELD(char *, last, DP_PARTNER);
		if (p == NULL || DA_FIELD(char, p, DP_ALIVE) == 0)
			return;
	}
	else if (alive > 1)
	{
		return;
	}

	DA_CharLibSetFrozen(DA_GAME + DG_CHARLIB, 1);
	DA_FIELD(char, DA_GAME, DG_ROUNDOVER) = 1;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		if (DA_FIELD(void *, p, DP_BOMB) != NULL)
			DA_PlayerDropBomb(p);
	}
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		if (DA_FIELD(char, p, DP_ALIVE) != 0)
			DA_FIELD(int, p, DP_PLACE) = alive;
	}
	if (DAT_002baebc == 0)
		dg_WinnersByScore();
	else
		dg_WinnersByTeam(last, 1);
	dg_EndRound(self);
}

/* 0x000eae20  Demolition/CDemolition_Game.cpp
Variation 3 slot 9 (Begin): the intro with each player's picture; every player starts with 3
lives; phase 0 for 5-15 s; random times for the first shakes. */
void CDemoVar3_Begin(void *self)
{
	char *p;

	DA_FIELD(short, DA_GAME, DG_STATE) = 1;
	DA_CameraSetMode(DA_GAME + DG_CAMERA, 1);
	dg_BeginIntroPictures();
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		DA_FIELD(char, p, DP_CONTROL) = 0;
		DA_FIELD(int, p, DP_SCORE) = 3;
	}
	DA_FIELD(int, self, DV_STATE) = 0;
	DA_TimerSet(DA_AT(self, DV_TIMER3), ((float)da_rand() * DA_RAND_SCALE + 0.5f) * 10.0f);
	DA_FIELD(float, self, DV_SHAKE3) = ((float)da_rand() * DA_RAND_SCALE + 0.5f) * 120.0f;
	DA_FIELD(float, self, DV_SHAKE3 + 4) = ((float)da_rand() * DA_RAND_SCALE + 0.5f) * 80.0f;
}

/* 0x000eaf80  Demolition/CDemolition_Game.cpp
Initialise (called by the game mode): loads the packages' resources, saves and sets the world
clip box, initialises every subsystem (a failure is only reported), rebuilds the routes and
makes the variation object (the countdown: 60 s, 90 s for cross), then the variation's Init. */
void CDemolitionGame_Initialise(unsigned int *self)
{
	char *g = (char *)self;
	void *pkg;
	void *memory, *variation;

	DA_FIELD(int, g, DG_SAVEDCHARSEL) = DAT_002bae70;
	DA_LoadPackageResources(DA_FIELD(void *, g, DG_PACKAGE), 1, 0xdefa, 0);
	DA_LoadPackageResources(DA_FIELD(void *, g, DG_GENPACKAGE), 1, 0xdefa, 0);
	DA_FIELD(float, g, DG_SAVEDCLIP) = DAT_00292c60;
	DA_FIELD(float, g, DG_SAVEDCLIP + 4) = DAT_00292c64;
	DA_FIELD(float, g, DG_SAVEDCLIP + 8) = DAT_00292c68;
	DA_FIELD(float, g, DG_SAVEDCLIP + 12) = DAT_00292c6c;
	DAT_00292c60 = -1500.0f;
	DAT_00292c64 = -1500.0f;
	DAT_00292c68 = 3000.0f;
	DAT_00292c6c = 3000.0f;
	if (dg_CameraInit(g + DG_CAMERA) == 0)
		DA_Printf("CDemolition_Game::Initialise - camera.Initialise FAILED!\n");
	pkg = DA_FIELD(void *, g, DG_GENPACKAGE);
	if (dg_CharLibInit(g + DG_CHARLIB, pkg) == 0)
		DA_Printf("CDemolition_Game::Initialise - charLib.Initialise FAILED!\n");
	if (CDemoPlatforms_Init(g + DG_PLATFORMS, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - platformSystem.Initialise FAILED!\n");
	if (DA_TimerInit(g + DG_TIMER) == 0)
		DA_Printf("CDemolition_Game::Initialise - timer.Initialise FAILED!\n");
	if (CDemoCountdown_Init((int)(uintptr_t)(g + DG_COUNTDOWN)) == 0)
		DA_Printf("CDemolition_Game::Initialise - countdown.Initialise FAILED!\n");
	if (CDemoTextureSystem_Init(g + DG_TEXTURES, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - textureSystem.Initialise FAILED!\n");
	if (CDemoLightSystem_Init(g + DG_LIGHTS, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - lightSystem.Initialise FAILED!\n");
	if (CDemoSpotFX_Init(g + DG_SPOTFX, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - spotFXSystem.Initialise FAILED!\n");
	if (dg_SystemInit(g + DG_PARTICLES, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - particleSystem.Initialise FAILED!\n");
	if (DA_DecalsInit(g + DG_DECALS) == 0)
		DA_Printf("CDemolition_Game::Initialise - decalSystem.Initialise FAILED!\n");
	if (dg_WorldInit(g + DG_WORLD, DA_FIELD(void *, g, DG_PACKAGE), DAT_00297510[DA_FIELD(int, g, DG_VARIATION)]) == 0)
		DA_Printf("CDemolition_Game::Initialise - world.Initialise FAILED!\n");
	if (dg_SystemInit(g + DG_ENTITIES, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - entitySystem.Initialise FAILED!\n");
	if (DA_SimulationInit(g + DG_SIMULATION) == 0)
		DA_Printf("CDemolition_Game::Initialise - simulation.Initialise FAILED!\n");
	if (DA_OverlayInit(g + DG_OVERLAY, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - overlaySystem.Initialise FAILED!\n");
	if (CDemoDebug_Init(g + DG_DEBUG) == 0)
		DA_Printf("CDemolition_Game::Initialise - debug.Initialise FAILED!\n");
	if (dg_BombsInit(g + DG_BOMBS, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - bombSystem.Initialise FAILED!\n");
	if (CDemoExplosionSystem_Init(g + DG_EXPLOSIONS, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - explosionSystem.Initialise FAILED!\n");
	if (CDemoTokenSystem_Init(g + DG_TOKENS, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - tokenSystem.Initialise FAILED!\n");
	if (CDemoInfo_Init(g + DG_INFO, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - infoSystem.Initialise FAILED!\n");
	if (CDemoInput_Init(g + DG_INPUT) == 0)
		DA_Printf("CDemolition_Game::Initialise - input.Initialise FAILED!\n");
	if (DA_Direct3DInit(g + DG_DIRECT3D) == 0)
		DA_Printf("CDemolition_Game::Initialise - direct3D.Initialise FAILED!\n");
	if (CDemoSfx_Init(g + DG_SFX, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - sfxSystem.Initialise FAILED!\n");
	if (CDemoRumble_Init(g + DG_RUMBLE, DA_FIELD(void *, g, DG_GENPACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - rumbleSystem.Initialise FAILED!\n");
	if (DA_Return1() == 0)
		DA_Printf("CDemolition_Game::Initialise - respawnSystem.Initialise FAILED!\n");
	DA_RouteRebuild();
	switch (DA_FIELD(int, g, DG_VARIATION))
	{
	case 0:
		DA_TimerSet(g + DG_COUNTDOWN, 60.0f);
		memory = DA_New(0x10, CDemolition_Game_cpp, 0x2e4);
		if (memory != NULL)
			*(const void *const **)memory = vtable_00269ad8;
		DA_FIELD(void *, g, DG_VARIATIONOBJ) = memory;
		break;
	case 1:
		DA_TimerSet(g + DG_COUNTDOWN, 60.0f);
		memory = DA_New(0x2340, CDemolition_Game_cpp, 0x2e9);
		if (memory != NULL)
		{
			*(const void *const **)memory = vtable_00269b14;
			dg_RemoteWorldCtor(DA_AT(memory, DV_WORLD1));
		}
		DA_FIELD(void *, g, DG_VARIATIONOBJ) = memory;
		break;
	case 2:
		DA_TimerSet(g + DG_COUNTDOWN, 60.0f);
		memory = DA_New(0x2a0, CDemolition_Game_cpp, 0x2ee);
		if (memory != NULL)
		{
			*(const void *const **)memory = vtable_00269b50;
			dg_MineWorldCtor(DA_AT(memory, DV_WORLD2));
		}
		DA_FIELD(void *, g, DG_VARIATIONOBJ) = memory;
		break;
	case 3:
		DA_TimerSet(g + DG_COUNTDOWN, 90.0f);
		memory = DA_New(0x16d0, CDemolition_Game_cpp, 0x2f3);
		DA_FIELD(void *, g, DG_VARIATIONOBJ) = memory != NULL ? CDemoVar3_Ctor(memory) : NULL;
		break;
	default:
		break;
	}
	variation = DA_FIELD(void *, g, DG_VARIATIONOBJ);
	if (DA_VCALL(variation)->init(variation, DA_FIELD(void *, g, DG_PACKAGE)) == 0)
		DA_Printf("CDemolition_Game::Initialise - variation.Initialise FAILED!\n");
}

/* 0x000eb4b0  Demolition/CDemolition_Game.cpp
Variation 0 slot 1 (Shutdown): the route heights and the barrier region. */
void CDemoVar0_Shutdown(void *self)
{
	void *heights = DA_FIELD(void *, self, DV_HEIGHTS);

	if (heights != NULL)
	{
		DA_RouteHeightsDtor(heights);
		DA_Delete(heights);
		DA_FIELD(void *, self, DV_HEIGHTS) = NULL;
	}
	dg_DeleteRegion((void **)DA_AT(self, DV_BARRIERS));
}

/* 0x000eb510  Demolition/CDemolition_Game.cpp
Variation 0 slot 6 (Update). */
void CDemoVar0_Update(void *self, float dt)
{
	dg_ShowScores();
	dg_UpdateState(self, dt, FUN_000e8400);
}

/* 0x000eb5a0  Demolition/CDemolition_Game.cpp
Variation 1 slot 6 (Update): the remote world first. */
void CDemoVar1_Update(void *self, float dt)
{
	dg_RemoteWorldUpdate(DA_AT(self, DV_WORLD1), dt);
	dg_ShowScores();
	dg_UpdateState(self, dt, CDemoVar1_UpdatePlay);
}

/* 0x000eb650  Demolition/CDemolition_Game.cpp
Variation 2 slot 6 (Update): the mine world after the scores. */
void CDemoVar2_Update(void *self, float dt)
{
	dg_ShowScores();
	dg_MineWorldUpdate(DA_AT(self, DV_WORLD2), dt);
	dg_UpdateState(self, dt, CDemoVar2_UpdatePlay);
}

/* 0x000eb700  Demolition/CDemolition_Game.cpp
Variation 3 slot 6 (Update): the cross world first. */
void CDemoVar3_Update(void *self, float dt)
{
	dg_CrossWorldUpdate(DA_AT(self, DV_WORLD3), dt);
	dg_ShowScores();
	dg_UpdateState(self, dt, CDemoVar3_UpdatePlay);
}

/* 0x000e9ac0  Demolition/CDemolition_Game.cpp (missing from the function table; called by the
game mode, 0x00101a76)
Render while ACTIVE: the world, decals and platforms, the variation's draw, the characters'
shadows (no fog), the platforms again with a z bias, the entities and characters (with fog),
then the effects, bombs, tokens, explosions, particles, the variation's overlay, the info
texts, the lights' sparks, the HUD overlay and the camera's overlay. */
void FUN_000e9ac0(void *self, int active)
{
	char *g = (char *)self;
	void *variation;

	if (active == 0)
		return;
	dg_WorldDrawSky(g + DG_WORLD);
	dg_WorldDraw(g + DG_WORLD);
	CDemoDecals_Draw(g + DG_DECALS);
	CDemoPlatforms_Draw(g + DG_PLATFORMS);
	variation = DA_FIELD(void *, g, DG_VARIATIONOBJ);
	DA_VCALL(variation)->draw(variation);
	if (DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)].fog != 0)
		DA_SetRenderState(8, 0, 0);
	DA_PushRenderState(5, 0, 0);
	DA_DrawShadows(g + 0xfc);
	D3DDevice_SetRenderState_ZBias(1);
	dg_WorldDrawShadowed(g + DG_WORLD);
	CDemoPlatforms_Draw(g + DG_PLATFORMS);
	D3DDevice_SetRenderState_ZBias(0);
	DA_DrawShadowsEnd();
	DA_PopRenderState(5);
	DA_SetRenderState(4, 0, 0);
	if (DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)].fog != 0)
		DA_SetRenderState(8, 1, 0);
	dg_EntitiesDraw(g + DG_ENTITIES);
	dg_CharLibDraw(g + DG_CHARLIB);
	if (DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)].fog != 0)
		DA_SetRenderState(8, 0, 0);
	dg_WorldDrawTransparent(g + DG_WORLD);
	CDemoSpotFX_DrawBursts(g + DG_SPOTFX);
	dg_BombsDraw(g + DG_BOMBS);
	CDemoTokenSystem_DrawGlow(g + DG_TOKENS);
	dg_CharLibDrawFx(g + DG_CHARLIB);
	dg_ExplosionsDraw(g + DG_EXPLOSIONS);
	DA_PSListDraw(g + DG_PARTICLES);
	dg_BombsDrawFx(g + DG_BOMBS);
	variation = DA_FIELD(void *, g, DG_VARIATIONOBJ);
	DA_VCALL(variation)->drawOverlay(variation);
	CDemoSpotFX_DrawPuffs(g + DG_SPOTFX);
	DA_Nop(g + DG_RESPAWN);
	CDemoTokenSystem_DrawShadows(g + DG_TOKENS);
	CDemoInfo_Draw(g + DG_INFO);
	CDemoLightSystem_Draw(g + DG_LIGHTS);
	CDemoOverlay_Draw(g + DG_OVERLAY);
	dg_CameraDraw(g + DG_CAMERA);
	if (DAT_00297520[DA_FIELD(int, DA_GAME, DG_VARIATION)].fog != 0)
		DA_SetRenderState(8, 1, 0);
}
