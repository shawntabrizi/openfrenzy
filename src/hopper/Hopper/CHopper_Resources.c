/*
Hopper/CHopper_Resources.c  <-  C:\BlitzParty\Game\Src\Hopper\CHopper_Resources.cpp
(module hopper)

The game mode CGameMode "Hopper" (vtable 0x0026e4dc; the module table places its virtuals
here by the vtable; its constructor and destructor 0x0013f590/0x0013f5b0 are gap functions
next to them) forwards to the game object 0x00302798 (CHopper_Game.c).

The resources object (0x002ae948, vtable 0x0026e510) loads everything a variation needs
(models, model instances, platforms, sounds, the intro camera path, lights, the tweak
menu) and frees it again (0x001414a0, a gap function after this file's last tagged one;
file inferred).

The resources object (0x290 bytes, offsets from 0x002ae948):
  +0x08, +0x0c  float: blade speeds (variation 1, tweakable)     +0x10..+0x18 variation 4
  +0x1c, +0x34, +0x3c 1.0   +0x24..+0x30 small constants          +0x44 the variation's package
  +0x48 its package id      +0x4c, +0x50 fonts handgotb/handgotl  +0x54 floaty.ttf
  +0x58 the collision actor (variation 3)   +0x5c the intro camera file
  +0x60..+0x7c sounds       +0xa0 the tweak menu                  +0xb4 water field parameters
  +0xf4 the ring texture (ai_player_ringnogoal.bmp)  +0xf8 the wake texture
  +0x178 the floaty score text system (BCarFloatyScore)  +0x1f8..+0x200 its parameters
  +0x238 a spark texture (variation 3)
The model table: models 0x002aed7c[64], their instances 0x002aee7c[64], the count 0x002aef7c;
extra models 0x002aef80[], instances 0x002af000[], count 0x002af080.
*/

#include "hopper.h"

static const char s_ResFile[] = "C:\\BlitzParty\\Game\\Src\\Hopper\\CHopper_Resources.cpp";

#define RES_P(self, off) HOP_AP(self, off)
#define hop_Package(self) ((struct bkPackage *)RES_P(self, 0x44))
#define hop_Models ((struct bkResource **)HOP_ADDR(0x002aed7c))
#define hop_Instances ((struct BActorInstance **)HOP_ADDR(0x002aee7c))
#define hop_ModelCount HOP_I(0x002aef7c)
#define hop_ExtraModels ((struct bkResource **)HOP_ADDR(0x002aef80))
#define hop_ExtraInstances ((struct BActorInstance **)HOP_ADDR(0x002af000))
#define hop_ExtraCount HOP_I(0x002af080)

extern unsigned char DAT_002ed324[];   /* classlib_ui: the rumble object */
extern uint32_t DAT_002ae578[16];      /* classlib_fx: the water field's default parameters */
extern char *DAT_002f27d8;             /* charlib: the route grid (CLRouteCell[]) */
extern void *DAT_002f26c4;             /* bumpercars_b: the debug free camera */

/* a model instance at time 0 */
static struct BActorInstance *hop_NewInstance(struct bkResource *model, int slots, int line)
{
	struct BActorInstance *inst;

	inst = baInitActorInstance(NULL, (struct BActor *)model, slots, s_ResFile, line);
	baPrepActorInstance(inst, 1);
	baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), 0, 1);
	return inst;
}

static struct bkResource *hop_Load(void *self, const char *name, int type)
{
	return bLoadResource(hop_Package(self), name, type, 0xdefa);
}

/* MSVC "eh vector destructor iterator" + delete[] of route tables (0x18 bytes, destructor
charlib 0x000ad880) */
static void hop_DeleteRouteTables(char *tables)
{
	int count = *(int *)(tables - 4);

	while (count-- > 0)
		CLRouteHeights_Free2((struct CLRouteHeights *)(tables + count * 0x18));
	HOP_DELETE(tables - 4);
}

/* ======================================================================== the game mode */

/* 0x0013f590  (gap): CGameMode_Hopper::CGameMode_Hopper */
void *CGameMode_Hopper_Ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, "Hopper");
	*(const void *const **)self = vtable_0026e4dc;
	return self;
}

/* 0x0013f5b0  (gap): CGameMode_Hopper::~CGameMode_Hopper */
void CGameMode_Hopper_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026e4dc;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x0013f5c0  Hopper/CHopper_Resources.cpp (vtable): enter: loads the character voices and
starts the game's packages (the variation is at +0x28); +0x14 = 1 (loading) */
void CGameMode_Hopper_Enter(void *self)
{
	Core_LoadCharacterVoices(1);
	HOP_I(0x003027d8) = 0;
	HopGame_Enter(hop_Game, HOP_AI(self, 0x28));
	HOP_AI(self, 0x14) = 1;
}

/* 0x0013f5f0  Hopper/CHopper_Resources.cpp (vtable): resume */
void CGameMode_Hopper_Resume(void *self)
{
	(void)self;
	HopGame_Resume(hop_Game);
}

/* 0x0013f600  Hopper/CHopper_Resources.cpp (vtable): suspend */
void CGameMode_Hopper_Suspend(void *self)
{
	(void)self;
	HopGame_Suspend(hop_Game);
}

/* 0x0013f610  Hopper/CHopper_Resources.cpp (vtable): update */
void CGameMode_Hopper_Update(void *self, int top)
{
	(void)self;
	HopGame_Update(hop_Game, top);
}

/* 0x0013f620  Hopper/CHopper_Resources.cpp (vtable): process */
void CGameMode_Hopper_Process(void *self, int top)
{
	(void)self;
	HopGame_Process(hop_Game, top);
}

/* 0x0013f630  Hopper/CHopper_Resources.cpp (vtable): draw */
void CGameMode_Hopper_Draw(void *self, int top)
{
	(void)self;
	HopGame_Draw(hop_Game, top);
}

/* 0x0013f640  Hopper/CHopper_Resources.cpp (vtable): init (the packages are loaded) */
void CGameMode_Hopper_Init(void *self)
{
	(void)self;
	HopGame_Init(hop_Game);
}

/* 0x0013f650  Hopper/CHopper_Resources.cpp (vtable): start */
void CGameMode_Hopper_Start(void *self)
{
	(void)self;
	HopGame_Start(hop_Game);
}

/* 0x0013f660  Hopper/CHopper_Resources.cpp (vtable): scalar deleting destructor */
void *CGameMode_Hopper_Destroy(void *self, byte flags)
{
	CGameMode_Hopper_Dtor(self);
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0013f680  Hopper/CHopper_Resources.cpp (vtable): exit. A mode that never started
(+0x04 == 0) only closes the package and frees the route tables of CModule_Hopper_Start;
otherwise the game shuts down, and the free 3D sound buffers are logged. */
void CGameMode_Hopper_Exit(void *self)
{
	DSCAPS caps;

	if (HOP_AI(self, 0x4) == 0) {
		bkClosePackageThunk(HOP_G(0x002ae98c, struct bkPackage *));
		if (hop_RouteTables != NULL)
			hop_DeleteRouteTables(hop_RouteTables);
		bkPrintf("this house is clean...\n");
		return;
	}
	HopGame_Exit(hop_Game);
	bkPrintf("this house is clean...\n");
	IDirectSound_GetCaps((LPDIRECTSOUND)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", caps.dwFree3DBuffers);
}

/* 0x0026e4dc  CGameMode_Hopper vtable */
const void *const vtable_0026e4dc[13] = {
	(const void *)FUN_0013f660, /* [0] +0x00 scalar deleting destructor */
	(const void *)FUN_0013f600, /* [1] +0x04 suspend */
	(const void *)FUN_0013f5f0, /* [2] +0x08 resume */
	(const void *)FUN_0013f680, /* [3] +0x0c exit */
	(const void *)FUN_0013f5c0, /* [4] +0x10 enter */
	(const void *)FUN_0013f610, /* [5] +0x14 update */
	(const void *)FUN_0013f620, /* [6] +0x18 process */
	(const void *)FUN_0013f630, /* [7] +0x1c draw */
	(const void *)FUN_001ec260, /* [8] +0x20 post draw (empty) */
	(const void *)FUN_00012930, /* [9] +0x24 poll the loading */
	(const void *)FUN_000128e0, /* [10] +0x28 loading finished */
	(const void *)FUN_0013f640, /* [11] +0x2c init */
	(const void *)FUN_0013f650, /* [12] +0x30 start */
};

/* ======================================================================== the resources */

/* 0x0013f710  Hopper/CHopper_Resources.cpp (bracketed): the render states of the game:
perspective, near plane (variation 1: 550, else 5), far 50000, ambient light (variation 4:
64, else 127), no textures, and the standard states */
void HopRes_SetRenderStates(void *self)
{
	(void)self;
	bdSetProjectionMode(1);
	bdSetNearFar(hop_Variation == 1 ? 550.0f : 5.0f, 50000.0f);
	if (hop_Variation == 4)
		baSetAmbientLight(0x40, 0x40, 0x40);
	else
		baSetAmbientLight(0x7f, 0x7f, 0x7f);
	bdSetTexture(0, NULL);
	bdSetTexture(1, NULL);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(8, 1, 0);
	bdSetRenderState(9, 0xff, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(2, 0xc, 1);
}

/* 0x0013f7e0  Hopper/CHopper_Resources.cpp: variation 1 (Prop Rotation): the two
propellers (four blade platforms, the second of each pair turned by pi), cables, the
universe and the sounds */
void HopRes_Load1(void *self)
{
	float zero[4];
	int i;

	hop_ModelCount = 0xe;
	zero[0] = 0.0f;
	zero[1] = 0.0f;
	zero[2] = 0.0f;
	zero[3] = 0.0f;
	RES_P(self, 0xf4) = bLoadResource(NULL, "ai_player_ringnogoal.bmp", 0, 0xdefa);
	RES_P(self, 0x64) = hop_Load(self, "sfx\\hopv1_01a.wav", 2);
	RES_P(self, 0x68) = hop_Load(self, "sfx\\hopv1_02a.wav", 2);
	RES_P(self, 0x6c) = hop_Load(self, "sfx\\hopv1_03a.wav", 2);
	hop_Models[0] = hop_Load(self, "v1\\platform.obe", 1);
	hop_Models[1] = hop_Load(self, "v1\\propell.obe", 1);
	hop_Models[2] = hop_Load(self, "v1\\propell2.obe", 1);
	hop_Models[3] = hop_Load(self, "v1\\cable01.obe", 1);
	hop_Models[4] = hop_Load(self, "v1\\cable02.obe", 1);
	hop_Models[5] = hop_Load(self, "v1\\cable03.obe", 1);
	hop_Models[6] = hop_Load(self, "v1\\cable04.obe", 1);
	hop_Models[7] = hop_Load(self, "v1\\cable05.obe", 1);
	hop_Models[8] = hop_Load(self, "v1\\cable06.obe", 1);
	hop_Models[9] = hop_Load(self, "v1\\universe.obe", 1);
	hop_Models[10] = hop_Load(self, "v1\\propeller.obe", 1);
	hop_Models[11] = hop_Load(self, "v1\\chains.obe", 1);
	hop_Models[12] = hop_Load(self, "v1\\shadows2.obe", 1);
	hop_Models[13] = hop_Load(self, "v1\\shadows.obe", 1);
	Actor_Load("v1\\plat_col.obe", hop_Package(self), zero, 0, NULL, 0);
	hop_Load(self, "v1\\blade_col.obe", 1);
	hop_Platforms[0] = Platform_Create("v1\\blade_col.obe", zero, 0);
	hop_Platforms[1] = Platform_Create("v1\\blade_col.obe", zero, 0);
	HOP_AU(hop_Platforms[1], 0x1f8) = 0x40490fdbu;    /* pi */
	hop_Load(self, "v1\\blade_col2.obe", 1);
	hop_Platforms[2] = Platform_Create("v1\\blade_col2.obe", zero, 0);
	hop_Platforms[3] = Platform_Create("v1\\blade_col2.obe", zero, 0);
	HOP_AU(hop_Platforms[3], 0x1f8) = 0x40490fdbu;
	for (i = 0; i < hop_ModelCount; i++)
		hop_Instances[i] = hop_NewInstance(hop_Models[i], 0, 0x99);
	hop_ModelCount = hop_ModelCount + 9;
	hop_Instances[14] = hop_NewInstance(hop_Models[9], 0, 0x9f);     /* universe */
	hop_Instances[15] = hop_NewInstance(hop_Models[10], 0, 0xa3);    /* propeller */
	hop_Instances[16] = hop_NewInstance(hop_Models[11], 0, 0xa7);    /* chains */
	hop_Instances[17] = hop_NewInstance(hop_Models[3], 0, 0xab);     /* cable01..06 */
	hop_Instances[18] = hop_NewInstance(hop_Models[4], 0, 0xaf);
	hop_Instances[19] = hop_NewInstance(hop_Models[5], 0, 0xb3);
	hop_Instances[20] = hop_NewInstance(hop_Models[6], 0, 0xb7);
	hop_Instances[21] = hop_NewInstance(hop_Models[7], 0, 0xbb);
	hop_Instances[22] = hop_NewInstance(hop_Models[8], 0, 0xbf);
}

/* 0x0013fd50  Hopper/CHopper_Resources.cpp: variation 2 (Centrifugal Farce): the disc of 27
platforms (plat01..27 and their faces face01..27, collision col01..27 as platforms), the
middle platform, the respawn effect (4 instances), 16 spot lights, textures */
void HopRes_Load2(void *self)
{
	char name[40];
	float zero[4];
	int i;

	hop_ModelCount = 0x3f;
	zero[0] = 0.0f;
	zero[1] = 0.0f;
	zero[2] = 0.0f;
	zero[3] = 0.0f;
	HOP_P(0x002aeb50) = hop_Load(self, "v2\\respawnsfx.obe", 1);
	HOP_P(0x002aeb54) = bLoadResource(NULL, "v2\\respawnsfx.obe", 1, 0xdefa);
	HOP_P(0x002aeb58) = bLoadResource(NULL, "v2\\respawnsfx.obe", 1, 0xdefa);
	HOP_P(0x002aeb5c) = bLoadResource(NULL, "v2\\respawnsfx.obe", 1, 0xdefa);
	for (i = 0; i < 4; i++) {
		/* 0x002aeb60[i]: instances of the models at 0x002aeb50[i] */
		HOP_PV(0x002aeb60)[i] = baInitActorInstance(NULL, (struct BActor *)HOP_PV(0x002aeb50)[i], 0, s_ResFile, 0xe5);
		baPrepActorInstance((struct BActorInstance *)HOP_PV(0x002aeb60)[i], 1);
		baSetNodeTime((struct BActorInstance *)HOP_PV(0x002aeb60)[i],
			(struct BActorNode *)HOP_AP(HOP_PV(0x002aeb60)[i], 0x134), 0, 1);
	}
	RES_P(self, 0x64) = hop_Load(self, "sfx\\hopv2_01a.wav", 2);
	RES_P(self, 0x68) = hop_Load(self, "sfx\\hopv2_02a.wav", 2);
	RES_P(self, 0x6c) = hop_Load(self, "sfx\\hopv2_03a.wav", 2);
	hop_Models[0] = hop_Load(self, "v2\\universe.obe", 1);
	HOP_P(0x002aee74) = hop_Load(self, "v2\\univadd.obe", 1);
	hop_Models[1] = hop_Load(self, "v2\\anim01.obe", 1);
	hop_Models[2] = hop_Load(self, "v2\\anim02.obe", 1);
	hop_Models[3] = hop_Load(self, "v2\\anim03.obe", 1);
	hop_Models[4] = hop_Load(self, "v2\\anim04.obe", 1);
	hop_Models[5] = hop_Load(self, "v2\\anim05.obe", 1);
	hop_Models[6] = hop_Load(self, "v2\\anim06.obe", 1);
	hop_Models[7] = hop_Load(self, "v2\\anim07.obe", 1);
	for (i = 1; i <= 0x1b; i++) {
		/* models [8..34] the platforms, [35..61] their faces; [62] is univadd */
		sprintf(name, "v2\\plat%02d.obe", i);
		HOP_PV(0x002aed9c)[i - 1] = hop_Load(self, name, 1);
		sprintf(name, "v2\\face%02d.obe", i);
		HOP_PV(0x002aee08)[i - 1] = hop_Load(self, name, 1);
	}
	Actor_Load("v2\\midplat.obe", hop_Package(self), zero, 0, NULL, 0);
	for (i = 0; i < 0x1b; i++) {
		sprintf(name, "v2\\col%02d.obe", i + 1);
		hop_Load(self, name, 1);
		hop_Platforms[i] = Platform_Create(name, zero, 0);
	}
	hop_ExtraCount = 0x10;
	hop_ExtraModels[0] = hop_Load(self, "v2\\spotbase.obe", 1);
	hop_ExtraModels[1] = hop_Load(self, "v2\\spotlight.obe", 1);
	for (i = 0; i < hop_ModelCount; i++)
		hop_Instances[i] = hop_NewInstance(hop_Models[i], 0, 0x119);
	for (i = 0; i < hop_ExtraCount; i++)
		hop_ExtraInstances[i] = hop_NewInstance(hop_ExtraModels[(unsigned int)i & 1], 0, 0x121);
	HOP_P(0x002aed68) = bLoadResource(NULL, "texanim\\edge.bmp", 0, 0xdefa);
	HOP_P(0x002aed6c) = bLoadResource(NULL, "textures\\beam.bmp", 0, 0xdefa);
}

/* 0x00140180  Hopper/CHopper_Resources.cpp: variation 3 (Careless Torque): eight blades,
cogs, platforms, glows; the two blade platforms and the cog as platforms; the collision
actor; two coloured point lights */
void HopRes_Load3(void *self)
{
	float zero[4];
	float pos[3];
	int i;

	hop_ModelCount = 0x15;
	zero[0] = 0.0f;
	zero[1] = 0.0f;
	zero[2] = 0.0f;
	zero[3] = 0.0f;
	RES_P(self, 0x60) = hop_Load(self, "sfx\\hopv3_01b.wav", 2);
	RES_P(self, 0x64) = hop_Load(self, "sfx\\hopv3_02d.wav", 2);
	RES_P(self, 0x68) = hop_Load(self, "sfx\\hopv3_03a.wav", 2);
	RES_P(self, 0x6c) = hop_Load(self, "sfx\\hopv3_04a.wav", 2);
	RES_P(self, 0x70) = hop_Load(self, "sfx\\hopv3_05b.wav", 2);
	RES_P(self, 0x74) = hop_Load(self, "sfx\\hopv3_02a.wav", 2);
	hop_Models[0] = hop_Load(self, "v3\\blade01.obe", 1);
	hop_Models[1] = hop_Load(self, "v3\\blade02.obe", 1);
	hop_Models[2] = hop_Load(self, "v3\\blade03.obe", 1);
	hop_Models[3] = hop_Load(self, "v3\\blade04.obe", 1);
	hop_Models[4] = hop_Load(self, "v3\\blade05.obe", 1);
	hop_Models[5] = hop_Load(self, "v3\\blade06.obe", 1);
	hop_Models[6] = hop_Load(self, "v3\\blade07.obe", 1);
	hop_Models[7] = hop_Load(self, "v3\\blade08.obe", 1);
	hop_Models[8] = hop_Load(self, "v3\\universe.obe", 1);
	hop_Models[9] = hop_Load(self, "v3\\cog01.obe", 1);
	hop_Models[10] = hop_Load(self, "v3\\cog02.obe", 1);
	hop_Models[11] = hop_Load(self, "v3\\lcog.obe", 1);
	hop_Models[12] = hop_Load(self, "v3\\plat01.obe", 1);
	hop_Models[13] = hop_Load(self, "v3\\plat02.obe", 1);
	hop_Models[14] = hop_Load(self, "v3\\plat01b.obe", 1);
	hop_Models[15] = hop_Load(self, "v3\\plat02b.obe", 1);
	hop_Models[16] = hop_Load(self, "v3\\core.obe", 1);
	hop_Models[17] = hop_Load(self, "v3\\unisemi.obe", 1);
	hop_Models[18] = hop_Load(self, "v3\\uniadd.obe", 1);
	hop_Models[19] = hop_Load(self, "v3\\glow2.obe", 1);
	hop_Models[20] = hop_Load(self, "v3\\glow1.obe", 1);
	hop_Models[21] = hop_Load(self, "v3\\plat01col.obe", 1);
	hop_Models[22] = hop_Load(self, "v3\\plat02col.obe", 1);
	hop_Models[23] = hop_Load(self, "v3\\lcogcol.obe", 1);
	hop_Platforms[0] = Platform_Create("v3\\plat01col.obe", zero, 0);
	hop_Platforms[1] = Platform_Create("v3\\plat02col.obe", zero, 0);
	hop_Platforms[2] = Platform_Create("v3\\lcogcol.obe", zero, 0);
	RES_P(self, 0x58) = Actor_Load("v3\\colision.obe", hop_Package(self), zero, 0, NULL, 0);
	for (i = 0; i < hop_ModelCount; i++)
		hop_Instances[i] = hop_NewInstance(hop_Models[i], 0, 0x174);
	RES_P(self, 0x238) = bLoadResource(NULL, "textures\\sparkburst01.bmp", 0, 0xdefa);
	RES_P(self, 0xf4) = bLoadResource(NULL, "ai_player_ringnogoal.bmp", 0, 0xdefa);
	pos[0] = -2000.0f;
	pos[1] = 270.0f;
	pos[2] = -3000.0f;
	HOP_P(0x002aec28) = baCreateLight(NULL, 0);
	baSetPointLight(HOP_G(0x002aec28, struct BLight *), pos, HOP_F(0x002ae970));
	baSetLightColour(HOP_G(0x002aec28, struct BLight *), 0x7f, 0x40, 0);
	baEnableLight(HOP_G(0x002aec28, struct BLight *), 1);
	pos[0] = 0.0f;
	pos[1] = 700.0f;
	pos[2] = -300.0f;
	HOP_P(0x002aec2c) = baCreateLight(NULL, 0);
	baSetPointLight(HOP_G(0x002aec2c, struct BLight *), pos, HOP_F(0x002ae974));
	baSetLightColour(HOP_G(0x002aec2c, struct BLight *), 0x10, 0x10, 0x60);
	baEnableLight(HOP_G(0x002aec2c, struct BLight *), 1);
}

/* 0x00140680  Hopper/CHopper_Resources.cpp (bracketed): loads a .lom file (the intro camera
path) from a package and gives it to the intro camera. Returns 1, or 0 on an error. */
int HopRes_LoadLOM(void *self, struct bkPackage *package, const char *name)
{
	uint size;
	void *data;
	int *map;

	data = bkLoadFile(package, name, NULL, &size, NULL, 0);
	RES_P(self, 0x5c) = data;
	if (data == NULL) {
		bkPrintf("LoadLOMFile ERROR: Could not load [%s]\n", name);
		return 0;
	}
	map = ParseMap_Load((char *)data, 2, (int)size);
	if (map == NULL) {
		bkPrintf("LoadLOMFile ERROR: Parsing [%s]\n", name);
		return 0;
	}
	CLIntroCam_Load((int)(uintptr_t)map, (undefined4)(uintptr_t)self);
	ParseMap_Free(map);
	return 1;
}

/* 0x00140710  Hopper/CHopper_Resources.cpp (bracketed): scalar deleting destructor */
void *HopRes_Destroy(void *self, byte flags)
{
	*(const void *const **)self = vtable_0026e510;
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026e510  the resources' vtable */
const void *const vtable_0026e510[1] = {
	(const void *)FUN_00140710, /* [0] scalar deleting destructor */
};

/* 0x00140730  Hopper/CHopper_Resources.cpp: variation 4 (Twisted System): the drill (two
collision actors, one 9463 high), the drill's parts, its wake, the steam water field (a
foxhunt CWaterField, 0x9090 bytes), a directional light */
void HopRes_Load4(void *self)
{
	float zero[4];
	float dir[3];
	uint32_t *params;
	void *water;
	int i;

	hop_ModelCount = 4;
	zero[0] = 0.0f;
	zero[1] = 0.0f;
	zero[2] = 0.0f;
	zero[3] = 0.0f;
	RES_P(self, 0x60) = hop_Load(self, "sfx\\hopv4_01a.wav", 2);
	RES_P(self, 0x64) = hop_Load(self, "sfx\\hopv4_02b.wav", 2);
	RES_P(self, 0x68) = hop_Load(self, "sfx\\hopv4_03b.wav", 2);
	RES_P(self, 0x6c) = hop_Load(self, "sfx\\hopper_02c.wav", 2);
	RES_P(self, 0x70) = hop_Load(self, "sfx\\hopper_02d.wav", 2);
	RES_P(self, 0x78) = hop_Load(self, "sfx\\hopv4_06a.wav", 2);
	RES_P(self, 0x74) = hop_Load(self, "sfx\\hopv4_05c.wav", 2);
	RES_P(self, 0x7c) = hop_Load(self, "sfx\\duck.wav", 2);
	hop_Models[0] = hop_Load(self, "v4\\universe.obe", 1);
	hop_Models[1] = hop_Load(self, "v4\\drill.obe", 1);
	hop_Models[3] = hop_Load(self, "v4\\worldadd.obe", 1);
	Actor_Load("v4\\drill.obe", NULL, zero, 0, NULL, 0);
	zero[1] = 9463.0f;
	Actor_Load("v4\\drill.obe", NULL, zero, 0, NULL, 0);
	for (i = 0; i < hop_ModelCount; i++) {
		if (i != 2)
			hop_Instances[i] = hop_NewInstance(hop_Models[i], 0, 0x1ce);
	}
	hop_Instances[2] = hop_NewInstance(hop_Models[1], 0, 0x1d3);
	hop_ExtraCount = 8;
	hop_ExtraModels[0] = hop_Load(self, "v4\\drillob02.obe", 1);
	hop_ExtraModels[1] = hop_Load(self, "v4\\drillob01.obe", 1);
	hop_ExtraModels[2] = hop_Load(self, "v4\\drillob03.obe", 1);
	hop_ExtraModels[3] = hop_Load(self, "v4\\crystal04.obe", 1);
	hop_ExtraModels[4] = hop_Load(self, "v4\\dshadow.obe", 1);
	hop_ExtraModels[5] = hop_Load(self, "v4\\nut.obe", 1);
	hop_ExtraModels[6] = hop_Load(self, "v4\\bar.obe", 1);
	hop_ExtraModels[7] = hop_Load(self, "v4\\steam.obe", 1);
	for (i = 0; i < hop_ExtraCount; i++)
		hop_ExtraInstances[i] = hop_NewInstance(hop_ExtraModels[i], 0, 0x1e4);
	HOP_P(0x002aed68) = bLoadResource(NULL, "textures\\drillpow.bmp", 0, 0xdefa);
	HOP_P(0x002aed6c) = bLoadResource(NULL, "textures\\drill04.bmp", 0, 0xdefa);
	RES_P(self, 0xf4) = bLoadResource(NULL, "ai_player_ringnogoal.bmp", 0, 0xdefa);
	HOP_P(0x002af180) = hop_Load(self, "v4\\drillwake.obe", 1);
	HOP_P(0x002af184) = hop_NewInstance(HOP_G(0x002af180, struct bkResource *), 0, 0x1f2);
	RES_P(self, 0xf8) = bLoadResource(NULL, "textures\\wake.gif", 0, 0xdefa);
	/* the water field's parameters: classlib_fx's defaults, then the steam */
	params = (uint32_t *)((char *)self + 0xb4);
	memcpy(params, DAT_002ae578, 0x10 * 4);
	HOP_AF(self, 0xb4) = -7000.0f;
	HOP_AF(self, 0xbc) = -7000.0f;
	HOP_AF(self, 0xc0) = 14000.0f;
	HOP_AF(self, 0xc4) = 14000.0f;
	RES_P(self, 0xec) = (void *)"textures\\steam_01.bmp";
	HOP_AU(self, 0xe8) = 0xff575757u;
	HOP_AI(self, 0xc8) = 0x40;
	HOP_AI(self, 0xcc) = 0x40;
	HOP_AU(self, 0xd4) = 0x3f7d70a4u;    /* 0.99 */
	water = HOP_NEW(0x9090, s_ResFile, 0x20a);
	if (water == NULL)
		HOP_P(0x002aec20) = NULL;
	else
		HOP_P(0x002aec20) = FUN_00138790((undefined4 *)water);
	water = HOP_P(0x002aec20);
	HOP_VFN(water, 0x4, void (*)(void *, void *))(water, params);
	HOP_P(0x002aec24) = baCreateLight(NULL, 4);
	dir[0] = 0.5f;
	dir[1] = 1.0f;
	dir[2] = 0.5f;
	baSetDirectionalLight(HOP_G(0x002aec24, struct BLight *), dir);
	baSetLightColour(HOP_G(0x002aec24, struct BLight *), 0, 0x40, 0x20);
	baEnableLight(HOP_G(0x002aec24, struct BLight *), 1);
}

/* 0x00140c70  Hopper/CHopper_Resources.cpp: load everything for the variation: defaults,
the lights cleared, the tweak menu, the rumble menu, the floaty score texts, the package's
resources, fonts, effect textures, the 6 characters' models and instances, the tokens,
the variation's own resources, its intro camera path, the shader selector, the two shader
fragments (pointers kept at antics +0x6c / +0x74) and the global directional light */
void HopRes_Load(void *self)
{
	struct CTweakMenu *menu;
	void *mem;
	const char *lom;
	float dir[3];
	int v, i;

	hop_RoundOver = 0;
	HOP_AU(self, 0x24) = 0x393cbe62u;
	HOP_AU(self, 0x30) = 0x393cbe62u;
	HOP_AU(self, 0x28) = 0x3a324207u;
	HOP_AU(self, 0x2c) = 0x39324207u;
	HOP_AF(self, 0x08) = 0.0f;
	HOP_AF(self, 0x0c) = 0.0f;
	HOP_AF(self, 0x10) = 0.0f;
	HOP_AF(self, 0x14) = 0.0f;
	HOP_AF(self, 0x18) = 0.0f;
	HOP_AF(self, 0x34) = 1.0f;
	HOP_AF(self, 0x1c) = 1.0f;
	HOP_AF(self, 0x20) = 0.0f;
	HOP_AF(self, 0x38) = 0.0f;
	HOP_AF(self, 0x3c) = 1.0f;
	memset(HOP_ADDR(0x002aec24), 0, 8 * 4);
	if (hop_Variation == 1) {
		HOP_AF(self, 0x08) = 2.2f;
		HOP_AF(self, 0x0c) = 2.2f;
	} else if (hop_Variation == 4) {
		HOP_AF(self, 0x10) = 2.0f;
		HOP_AF(self, 0x14) = 24.0f;
		HOP_AF(self, 0x18) = 5.0f;
	}
	mem = HOP_NEW(0xe8, s_ResFile, 0x283);
	if (mem == NULL)
		menu = NULL;
	else
		menu = CTweakMenu_Ctor((struct CTweakMenu *)mem, "tweak menu");
	RES_P(self, 0xa0) = menu;
	if (HOP_F(0x002ae950) != 0.0f)
		CTweakMenu_AddFloat(menu, "blade 1 speed", HOP_FV(0x002ae950), 0.1f, -5.0f, 5.0f);
	if (HOP_F(0x002ae954) != 0.0f)
		CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "blade 2 speed", HOP_FV(0x002ae954), 0.1f, -5.0f, 5.0f);
	if (HOP_F(0x002ae958) != 0.0f)
		CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "light (r)", HOP_FV(0x002ae958), 1.0f, 0.0f, 127.0f);
	if (HOP_F(0x002ae95c) != 0.0f)
		CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "light (g)", HOP_FV(0x002ae95c), 1.0f, 0.0f, 127.0f);
	if (HOP_F(0x002ae960) != 0.0f)
		CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "light (b)", HOP_FV(0x002ae960), 1.0f, 0.0f, 127.0f);
	menu = (struct CTweakMenu *)RES_P(self, 0xa0);
	CTweakMenu_AddFloat(menu, "light1 r", HOP_FV(0x002ae96c), 1e-05f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "light2 r", HOP_FV(0x002ae970), 1e-05f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "light3 r", HOP_FV(0x002ae974), 1e-05f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "light4 r", HOP_FV(0x002ae978), 1e-05f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "draw water", HOP_FV(0x002ae984), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "draw timers", HOP_FV(0x002ae980), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "detail", HOP_FV(0x002ae964), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "fps", HOP_FV(0x002ae968), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "slow men", HOP_FV(0x002ae97c), 1.0f, 0.0f, 1.0f);
	v = hop_Variation;
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "fog r", HOP_FV(0x002aeb80) + v, 1.0f, 0.0f, 255.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "fog g", HOP_FV(0x002aeb90) + hop_Variation, 1.0f, 0.0f, 255.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "fog b", HOP_FV(0x002aeba0) + hop_Variation, 1.0f, 0.0f, 255.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "fog near", HOP_FV(0x002aebb0) + hop_Variation, 100.0f, 100.0f,
		100000.0f);
	CTweakMenu_AddFloat((struct CTweakMenu *)RES_P(self, 0xa0), "fog far", HOP_FV(0x002aebc0) + hop_Variation, 100.0f, 100.0f,
		100000.0f);
	CLRumble_CreateMenu((int *)DAT_002ed324);
	BCarFloatyScore_Init((struct BCarFloatyScore *)((char *)self + 0x178), 0x40);
	HOP_AF(self, 0x1fc) = 0.3f;
	HOP_AF(self, 0x1f8) = 1200.0f;
	HOP_AF(self, 0x200) = HOP_AF(self, 0x200) * 60.0f;
	HOP_B(0x003027b0) = 1;
	hop_ExtraCount = 0;
	HOP_AU(self, 0x48) = bkGetPackageId(hop_Package(self));
	bLoadPackageResources(hop_Package(self), 1, 0xdefa, 0);
	RES_P(self, 0x4c) = bLoadResource(NULL, "handgotb.ttf", 3, 0xdefa);
	if (RES_P(self, 0x4c) == NULL)
		bkPrintf("ERROR - FONT NOT LOADED\n");
	RES_P(self, 0x50) = bLoadResource(NULL, "handgotl.ttf", 3, 0xdefa);
	RES_P(self, 0x54) = hop_Load(self, "floaty.ttf", 3);
	HOP_P(0x002affb0) = bLoadResource(NULL, "textures\\bubble.bmp", 0, 0xdefa);
	HOP_P(0x002affd4) = bLoadResource(NULL, "textures\\steam_01.bmp", 0, 0xdefa);
	HOP_P(0x002affe0) = bLoadResource(NULL, "textures\\electric01.bmp", 0, 0xdefa);
	HOP_P(0x002affe4) = bLoadResource(NULL, "textures\\electric02.bmp", 0, 0xdefa);
	HOP_P(0x002affe8) = bLoadResource(NULL, "textures\\electric03.bmp", 0, 0xdefa);
	/* the 6 characters: model and low-detail model (names in the table at 0x002ae818) */
	for (i = 0; i < 6; i++) {
		HOP_PV(0x002afe84)[i * 2] = hop_Load(self, HOP_G(0x002ae818 + i * 0x10, const char *), 1);
		HOP_PV(0x002afe84)[i * 2 + 1] = hop_Load(self, HOP_G(0x002ae820 + i * 0x10, const char *), 1);
	}
	HOP_P(0x002affb8) = hop_Load(self, "tokens\\gcoin.obe", 1);
	if (hop_Variation < 3) {
		HOP_P(0x002affbc) = hop_Load(self, "tokens\\scoin.obe", 1);
		HOP_P(0x002affc0) = hop_Load(self, "tokens\\bcoin.obe", 1);
	}
	for (i = 0; i < 12; i++)
		HOP_PV(0x002afeb4)[i] = hop_NewInstance(HOP_PV(0x002afe84)[i], 1, 0x2d2);
	HOP_P(0x002affc4) = hop_NewInstance(HOP_G(0x002affb8, struct bkResource *), 0, 0x2d7);
	if (hop_Variation < 3) {
		HOP_P(0x002affc8) = hop_NewInstance(HOP_G(0x002affbc, struct bkResource *), 0, 0x2dc);
		HOP_P(0x002affcc) = hop_NewInstance(HOP_G(0x002affc0, struct bkResource *), 0, 0x2e0);
	}
	switch (hop_Variation) {
	case 1:
		HopRes_Load1(self);
		break;
	case 2:
		HopRes_Load2(self);
		break;
	case 3:
		HopRes_Load3(self);
		break;
	case 4:
		HopRes_Load4(self);
		break;
	}
	lom = NULL;
	switch (hop_Variation) {
	case 1:
		lom = "v1\\camera4.lom";
		break;
	case 2:
		lom = "v2\\camera.lom";
		break;
	case 3:
		lom = "v3\\camera3.lom";
		break;
	case 4:
		lom = "v4\\camera2.lom";
		break;
	}
	if (lom != NULL)
		HopRes_LoadLOM(self, hop_Package(self), lom);
	baShutdownShaderSelector();
	baInitShaderSelector();
	mem = HOP_NEW(0x14, s_ResFile, 0x30d);
	HOP_P(0x002ae8dc) = mem == NULL ? NULL : HopShaderLit_Ctor(mem);
	if (HOP_P(0x002ae8dc) == NULL)
		bkPrintf("shader buggered up\n");
	else
		HOP_P(0x002ae8e4) = HOP_P(0x002ae8dc);
	mem = HOP_NEW(0x14, s_ResFile, 0x314);
	HOP_P(0x002ae8e0) = mem == NULL ? NULL : HopShaderColour_Ctor(mem);
	if (HOP_P(0x002ae8e0) == NULL)
		bkPrintf("shader2 buggered up\n");
	else
		HOP_P(0x002ae8ec) = HOP_P(0x002ae8e0);
	dir[0] = -0.2f;
	dir[1] = 0.5f;
	dir[2] = -0.5f;
	HOP_P(0x002aec40) = baCreateLight(NULL, 0);
	baSetDirectionalLight(HOP_G(0x002aec40, struct BLight *), dir);
	baSetLightColour(HOP_G(0x002aec40, struct BLight *), 0x7f, 0x7f, 0x7f);
	baEnableLight(HOP_G(0x002aec40, struct BLight *), 1);
}

/* 0x001414a0  (gap; a resources method, file inferred): frees what HopRes_Load made, unless
the game is over (then the pause menu's exit frees it elsewhere) */
void HopRes_Free(void *self)
{
	void *p;
	int i;

	if (HOP_I(0x003027b8) == 1)
		return;
	p = RES_P(self, 0xa0);
	if (p != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)p);
		HOP_DELETE(p);
	}
	p = DAT_002f26c4;
	if (DAT_002f26c4 != NULL) {
		BCarFreeCamera_Destruct((struct BCarFreeCamera *)DAT_002f26c4);
		HOP_DELETE(p);
	}
	CLRumble_Shutdown((int *)DAT_002ed324);
	Shadows_DeleteAll();
	if (HOP_P(0x002ae8dc) != NULL)
		HOP_DELETE(HOP_P(0x002ae8dc));
	if (HOP_P(0x002ae8e0) != NULL)
		HOP_DELETE(HOP_P(0x002ae8e0));
	for (i = 0; i < 4; i++)
		baSetRenderPass((struct BActorInstance *)((char *)hop_PlayerActor(i) + 0x10), 0, 1, 0);
	if (DAT_002f27d8 != NULL)
		hop_DeleteRouteTables(DAT_002f27d8);
	RefBot_Delete();
	CLDebris_Start();
	CToken_DeleteAll();
	bkDeleteResource((struct bkResource *)RES_P(self, 0x54));
	CLIntroCam_Destroy();
	CProjector_DeleteAll();
	switch (hop_Variation) {
	case 1:
		bkDeleteResource((struct bkResource *)RES_P(self, 0x64));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x68));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x6c));
		break;
	case 2:
		bkDeleteResource((struct bkResource *)RES_P(self, 0x64));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x68));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x6c));
		baDeleteActorInstance(HOP_G(0x002aeb60, struct BActorInstance *));
		baDeleteActorInstance(HOP_G(0x002aeb64, struct BActorInstance *));
		baDeleteActorInstance(HOP_G(0x002aeb68, struct BActorInstance *));
		baDeleteActorInstance(HOP_G(0x002aeb6c, struct BActorInstance *));
		bkDeleteResource(HOP_G(0x002aeb50, struct bkResource *));
		break;
	case 3:
		bkDeleteResource((struct bkResource *)RES_P(self, 0x60));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x64));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x68));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x6c));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x70));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x74));
		break;
	case 4:
		bkDeleteResource((struct bkResource *)RES_P(self, 0x60));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x64));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x68));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x6c));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x70));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x78));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x74));
		bkDeleteResource((struct bkResource *)RES_P(self, 0x7c));
		baDeleteActorInstance(HOP_G(0x002af184, struct BActorInstance *));
		bkDeleteResource(HOP_G(0x002af180, struct bkResource *));
		p = HOP_P(0x002aec20);
		if (p != NULL)
			HOP_VFN(p, 0x0, void (*)(void *, int))(p, 1);
		break;
	}
	Actor_DeleteAll();
	for (i = 0; i < hop_ModelCount; i++) {
		baDeleteActorInstance(hop_Instances[i]);
		/* variation 1's instances 14..22 reuse models; variation 4 has no model 2 */
		if (i >= 0xe && i <= 0x16) {
			if (hop_Variation != 1)
				bkDeleteResource(hop_Models[i]);
		} else if (i != 2 || hop_Variation != 4) {
			bkDeleteResource(hop_Models[i]);
		}
	}
	if (hop_Variation == 3) {
		bkDeleteResource(HOP_G(0x002aedd0, struct bkResource *));
		bkDeleteResource(HOP_G(0x002aedd4, struct bkResource *));
		bkDeleteResource(HOP_G(0x002aedd8, struct bkResource *));
	}
	if (hop_ExtraCount != 0) {
		for (i = 0; i < hop_ExtraCount; i++) {
			baDeleteActorInstance(hop_ExtraInstances[i]);
			if (hop_Variation != 2 || i < 2)
				bkDeleteResource(hop_ExtraModels[i]);
		}
	}
	for (i = 0; i < 12; i++) {
		baDeleteActorInstance((struct BActorInstance *)HOP_PV(0x002afeb4)[i]);
		bkDeleteResource((struct bkResource *)HOP_PV(0x002afe84)[i]);
	}
	baDeleteActorInstance(HOP_G(0x002affc4, struct BActorInstance *));
	bkDeleteResource(HOP_G(0x002affb8, struct bkResource *));
	if (hop_Variation < 3) {
		baDeleteActorInstance(HOP_G(0x002affc8, struct BActorInstance *));
		bkDeleteResource(HOP_G(0x002affbc, struct bkResource *));
		baDeleteActorInstance(HOP_G(0x002affcc, struct BActorInstance *));
		bkDeleteResource(HOP_G(0x002affc0, struct bkResource *));
	}
	bkDeletePackageResources(HOP_AU(self, 0x48), 1);
	bkDeletePackageResources(HOP_AU(self, 0x48), 0xffffffffu);
	bkClosePackageThunk(hop_Package(self));
	RES_P(self, 0x44) = NULL;
}
