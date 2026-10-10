/*
Demolition/CDemolition_CharLib.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_CharLib.cpp

Two classes (only 0x000f8dc0 is tagged with this file; the others are gap functions around it,
"file inferred"):

CDemolition_CharLib (vtable 0x0026b2c0, message receiver vtable 0x0026b3b4 at +0x1130): the
  CharLib character base of the Demolition players and the remote bomb. It derives from
  demolition_d's CharLib AI (DemoCLAI, 0x0010d580) and has a CharLib event system
  (DemoCharEventSystem, 0x0010cee0) at +0x1130. Fields from +0x1140 (see
  CDemolition_Player.c): +0x1140 next, +0x1144 position, +0x1154 position copy.

The character manager (game +0x64, vtable 0x0026b3b8; called CDemolition_CharLibMgr here, the
  file name suggests the original class was CDemolition_CharLib itself):
  +0x04 package id   +0x08 the first character (linked at +0x1140)
  +0x0c player[4]    +0x1c models[12] (the characters, their "opt" models and their
  controllers)   +0x4c the route context (core_modes_b's six lists, charlib)
  +0x94 the route tables (new CLRouteTable[900])   +0x98..+0xa4 zero   +0xa8 colour set
*/

#include "demolition_b.h"
#include "demolition_b_internal.h"

#include <stdio.h>
#include <stdint.h>

#define DB_CHARLIB_FILE "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_CharLib.cpp"

/* 0x00292c78  the number of route grid cells (charlib, defined by core_modes_b) */
extern int DAT_00292c78;
/* 0x002f289c  the players' CharLib AI objects */
extern int DAT_002f289c[];
/* 0x00283ea4  the colour set (core) */
extern int DAT_00283ea4;

/* 0x00297a50  the character model names (by character index) */
const char *const DAT_00297a50[6] = { "dub", "fly", "geena", "zak", "jade", "jet" };
/* 0x00297a68  the "opt" model names */
const char *const DAT_00297a68[6] = {
	"dubopt", "flyopt", "geenaopt", "zakopt", "jadeopt", "jetopt",
};
/* 0x00297a80  the controller model names */
const char *const DAT_00297a80[6] = {
	"dub_controller", "fly_controller", "geena_controller", "zak_controller",
	"jade_controller", "jet_controller",
};
/* 0x00297a98  the shadow parameters per variation (3 used, 1 pad) */
const float DAT_00297a98[4][4] = {
	{ 1.0f, -1.4f, 0.0f, 0.0f },
	{ 1.0f, -1.4f, 0.0f, 0.0f },
	{ 1.0f, -1.4f, 0.0f, 0.0f },
	{ 0.0f, 1000.0f, 0.0f, 0.0f },
};

/* ====================================================================================
   CDemolition_CharLib: the character base
   ==================================================================================== */

/* 0x000f8a50  (file inferred) constructor */
struct CDemolition_CharLib *CDemolition_CharLib_Construct(struct CDemolition_CharLib *self)
{
	char *s = (char *)self;

	DemoCLAI_Ctor(s);
	DemoCharEventSystem_Ctor(s + 0x1130);
	DB_PTR(s, 0x1130) = (void *)vtable_0026b3b4;
	DB_PTR(s, 0) = (void *)vtable_0026b2c0;
	return self;
}

/* 0x000f8ab0  (file inferred) a do-nothing virtual of one parameter that returns 0 (the
   unused AI state slots of the Demolition characters, and other vtables) */
int CDemolition_CharLib_Nothing(void *self, int arg)
{
	(void)self;
	(void)arg;
	return 0;
}

/* 0x000f8ac0  (file inferred) destructor */
void CDemolition_CharLib_Destruct(struct CDemolition_CharLib *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026b2c0;
	DB_PTR(s, 0x1130) = (void *)vtable_0026b3b4;
	DemoCharEventSystem_Dtor(s + 0x1130);
	DemoCLAI_Dtor(s);
}

/* ====================================================================================
   The character manager (game +0x64)
   ==================================================================================== */

/* 0x000f8b20  (file inferred) destructor */
void CDemolition_CharLibMgr_Destruct(struct CDemolition_CharLibMgr *self)
{
	DB_PTR(self, 0) = (void *)vtable_0026b3b8;
	cmb_six_lists_dtor((char *)self + 0x4c);
}

/* 0x000f8b30  (file inferred) Update: aims the shadows, then updates every character
   (virtual +0xe4) */
void CDemolition_CharLibMgr_Update(struct CDemolition_CharLibMgr *self, float dt)
{
	char *c;

	Shadows_Aim();
	for (c = (char *)DB_PTR(self, 8); c != NULL; c = (char *)DB_PTR(c, 0x1140))
		DB_VFN(c, 0xe4, void (*)(void *, float))(c, dt);
}

/* 0x000f8b60  (file inferred) Draw: the characters (virtual +0xe8) with render state 6 off,
   3 pushed on, and the light group 4 (and the object at game +0x6cc8) switched off */
void CDemolition_CharLibMgr_Draw(struct CDemolition_CharLibMgr *self)
{
	char *c;
	void *object;

	bdSetRenderState(6, 0, 0);
	bdPushRenderState(3, 1, 0);
	CDemoLightSystem_EnableGroup((int)(uintptr_t)(DAT_002ff85c + 0x6ca4), 0, 4);
	object = DB_PTR(DAT_002ff85c, 0x6cc8);
	DB_VFN(object, 0xc, void (*)(void *, int))(object, 1);
	for (c = (char *)DB_PTR(self, 8); c != NULL; c = (char *)DB_PTR(c, 0x1140))
		DB_VFN(c, 0xe8, void (*)(void *))(c);
	bdPopRenderState(3);
	CDemoLightSystem_EnableGroup((int)(uintptr_t)(DAT_002ff85c + 0x6ca4), 1, 4);
	object = DB_PTR(DAT_002ff85c, 0x6cc8);
	DB_VFN(object, 0xc, void (*)(void *, int))(object, 0);
}

/* 0x000f8c00  (file inferred) DrawFx: the characters' effects (virtual +0xec) */
void CDemolition_CharLibMgr_DrawFx(struct CDemolition_CharLibMgr *self)
{
	char *c;

	bdSetWorldIdentity();
	for (c = (char *)DB_PTR(self, 8); c != NULL; c = (char *)DB_PTR(c, 0x1140))
		DB_VFN(c, 0xec, void (*)(void *))(c);
}

/* 0x000f8c30  (file inferred) the characters of players that are not human become CPU
   players (+0x1171), in list order against the player records */
void CDemolition_CharLibMgr_UpdateInput(struct CDemolition_CharLibMgr *self)
{
	char *c;
	int i = 0;

	for (c = (char *)DB_PTR(self, 8); c != NULL; c = (char *)DB_PTR(c, 0x1140), i++)
		DB_U8(c, 0x1171) = DAT_002baed4[i * 10 + 1] == 0;
}

/* 0x000f8c60  (file inferred) freezes (or frees) every character: virtual +0x44 with
   "active" = !frozen */
void CDemolition_CharLibMgr_SetFrozen(struct CDemolition_CharLibMgr *self, int frozen)
{
	char *c;

	for (c = (char *)DB_PTR(self, 8); c != NULL; c = (char *)DB_PTR(c, 0x1140))
		DB_VFN(c, 0x44, void (*)(void *, int))(c, frozen == 0);
}

/* 0x000f8ca0  (file inferred) CDemolition_CharLib: deleting destructor (vtable 0x0026b2c0
   slot 18) */
void *CDemolition_CharLib_DeletingDestruct(struct CDemolition_CharLib *self, unsigned int flags)
{
	CDemolition_CharLib_Destruct(self);
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000f8c90  (file inferred) the same through the message receiver (+0x1130; vtable
   0x0026b3b4 slot 0) */
void *CDemolition_CharLib_Receiver_DeletingDestruct(void *receiver, unsigned int flags)
{
	return CDemolition_CharLib_DeletingDestruct(
		(struct CDemolition_CharLib *)((char *)receiver - 0x1130), flags);
}

/* 0x000f8cc0  (file inferred) CDemolition_CharLib: sets the position of the actor
   (actor +0xd0) and of the character (+0x1144, +0x1154) */
void CDemolition_CharLib_SetPosition(struct CDemolition_CharLib *self, float *pos)
{
	char *s = (char *)self;

	memcpy(DB_ADDR(DB_PTR(s, 0x30), 0xd0), pos, 16);
	memcpy(s + 0x1144, pos, 16);
	memcpy(s + 0x1154, pos, 16);
}

/* 0x000f8d30  (file inferred) CDemolition_CharLib: sets the rotation of the actor (+0x1e4
   and +0x1d4) */
void CDemolition_CharLib_SetRotation(struct CDemolition_CharLib *self, float *euler)
{
	char *s = (char *)self;

	memcpy(DB_ADDR(DB_PTR(s, 0x30), 0x1e4), euler, 16);
	memcpy(DB_ADDR(DB_PTR(s, 0x30), 0x1d4), euler, 16);
}

/* 0x000f8d80  (file inferred) constructor */
struct CDemolition_CharLibMgr *CDemolition_CharLibMgr_Construct(struct CDemolition_CharLibMgr *self)
{
	DB_PTR(self, 0) = (void *)vtable_0026b3b8;
	cmb_six_lists_ctor((char *)self + 0x4c);
	return self;
}

/* 0x000f8da0  (file inferred) deleting destructor (vtable 0x0026b3b8 slot 0) */
void *CDemolition_CharLibMgr_DeletingDestruct(struct CDemolition_CharLibMgr *self,
	unsigned int flags)
{
	CDemolition_CharLibMgr_Destruct(self);
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000ad830  CharLib/Route.cpp: the constructor of one route table (0x18 bytes). Missing
   from the function table (reached only through the "eh vector constructor iterator"), so
   it is local here, as in carryon_a. */
static void *db_RouteTable_Ctor(void *self)
{
	DB_PTR(self, 0) = bkHeapAlloc(DAT_00292c78 * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp",
		0x23d, 0x2001);
	DB_PTR(self, 4) = bkHeapAlloc(DAT_00292c78, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp",
		0x23e, 0x2001);
	return self;
}

/* 0x000f8dc0  Demolition/CDemolition_CharLib.cpp
   Init(package): the route grid (900 cells, 30 x 30), the 12 models, the four players
   (CDemolition_Player, created from the last; each with the AI "brain" of the variation),
   their team mates and brains, then the shadows. Returns 0 when a model is missing. */
int CDemolition_CharLibMgr_Init(struct CDemolition_CharLibMgr *self, void *package)
{
	char *m = (char *)self;
	char path[256];
	int *block;
	char *tables;
	int i;
	char *player;
	void *brain;
	int shadowMode = 0;
	int v;

	DB_PTR(m, 8) = NULL;
	DAT_00292c78 = 900;
	block = (int *)bkHeapAlloc(0x5464, DB_CHARLIB_FILE, 0xa6, 0x1001);
	if (block == NULL)
		tables = NULL;
	else
	{
		/* new CLRouteTable[900] (eh vector constructor iterator, destructor 0x000ad880) */
		tables = (char *)(block + 1);
		*block = 900;
		for (i = 0; i < 900; i++)
			db_RouteTable_Ctor(tables + i * 0x18);
	}
	DB_PTR(m, 0x94) = tables;
	FUN_000c39b0((undefined4)(uintptr_t)(m + 0x4c), (undefined4)(uintptr_t)tables, 0x1e, 0x1e);

	/* models 0-3: the players' characters, 4-7: their "opt" models, 8-11: their
	   controllers */
	for (i = 0; i < 12; i++)
	{
		int character = DAT_002baed4[(i & 3) * 10];

		switch (i >> 2)
		{
		case 0:
			sprintf(path, "characters\\%s.obe", DAT_00297a50[character]);
			break;
		case 1:
			sprintf(path, "characters\\%s.obe", DAT_00297a68[character]);
			break;
		case 2:
			sprintf(path, "models\\%s.obe", DAT_00297a80[character]);
			break;
		}
		DB_PTR(m, 0x1c + i * 4) = bLoadResource((struct bkPackage *)package, path, 1, 0xdefa);
		if (DB_PTR(m, 0x1c + i * 4) == NULL)
		{
			bkPrintf("ERROR: Failed to load object %s!\n", path);
			return 0;
		}
	}

	for (i = 3; i >= 0; i--)
	{
		player = (char *)bkHeapAlloc(0x13e0, DB_CHARLIB_FILE, 0xda, 0x1001);
		/* the original keeps every allocation in one local, which the shadow mode below
		   falls back to for an unknown variation */
		shadowMode = (int)(uintptr_t)player;
		if (player != NULL)
			player = (char *)CDemolition_Player_Construct((struct CDemolition_Player *)player);
		DAT_002f289c[i] = (int)(uintptr_t)player;
		DB_VFN(player, 0xdc, int (*)(void *, int))(player, i);
		v = DB_I32(DAT_002ff85c, 0x28);
		if (v >= 0 && v <= 3)
		{
			switch (v)
			{
			case 0:
				brain = bkHeapAlloc(0x1d8, DB_CHARLIB_FILE, 0xe5, 0x1001);
				shadowMode = (int)(uintptr_t)brain;
				if (brain != NULL)
					brain = DemoAI0_Ctor((struct DemoAI *)brain);
				break;
			case 1:
				brain = bkHeapAlloc(0x1c8, DB_CHARLIB_FILE, 0xe8, 0x1001);
				shadowMode = (int)(uintptr_t)brain;
				if (brain != NULL)
					brain = DemoAI1_Ctor((struct DemoAI *)brain);
				break;
			case 2:
				brain = bkHeapAlloc(0x1b4, DB_CHARLIB_FILE, 0xeb, 0x1001);
				shadowMode = (int)(uintptr_t)brain;
				if (brain != NULL)
					brain = DemoAI2_Ctor((struct DemoAI *)brain);
				break;
			default:
				brain = bkHeapAlloc(0x1c8, DB_CHARLIB_FILE, 0xee, 0x1001);
				shadowMode = (int)(uintptr_t)brain;
				if (brain != NULL)
					brain = DemoAI3_Ctor((struct DemoAI *)brain);
				break;
			}
			DB_PTR(player, 0x118c) = brain;
		}
		DB_PTR(player, 0x1140) = DB_PTR(m, 8);
		DB_PTR(m, 8) = player;
		DB_PTR(m, 0xc + i * 4) = player;
	}

	for (i = 0; i < 4; i++)
	{
		char *p = (char *)(uintptr_t)DAT_002f289c[i];
		void *pbrain;

		CDemolition_Player_FindTeamMate((struct CDemolition_Player *)p);
		pbrain = DB_PTR(p, 0x118c);
		DB_VFN(pbrain, 0, void (*)(void *, void *))(pbrain, p);
		DB_U8(p, 0x1171) = DAT_002baed4[i * 10 + 1] == 0;
	}

	v = DB_I32(DAT_002ff85c, 0x28);
	switch (v)
	{
	case 0:
	case 1:
	case 2:
		shadowMode = 2;
		break;
	case 3:
		shadowMode = 0;
		break;
	}
	Shadows_Init(shadowMode, DAT_00297a98[v][0], DAT_00297a98[v][1], DAT_00297a98[v][2]);
	v = DB_I32(DAT_002ff85c, 0x28);
	if (v >= 0 && v <= 2)
	{
		Shadows_SetFov(db_bitsf(0x3edf66f3), db_bitsf(0x3ea78d36));   /* 0.4363, 0.3272 */
		Shadows_SetDistance(2000.0f);
	}
	DB_I32(m, 0x98) = 0;
	DB_I32(m, 0x9c) = 0;
	DB_I32(m, 0xa0) = 0;
	DB_I32(m, 0xa4) = 0;
	DB_I32(m, 0xa8) = DAT_00283ea4;
	return 1;
}

/* 0x000f9210  (file inferred) Shutdown: the shadows, every character (virtual +0xe0, then
   the deleting destructor +0x48), the models, the route tables, the CharLib actors and the
   package's models */
void CDemolition_CharLibMgr_Shutdown(struct CDemolition_CharLibMgr *self)
{
	char *m = (char *)self;
	char *c;
	char *next;
	char *tables;
	int i;

	Shadows_DeleteAll();
	for (c = (char *)DB_PTR(m, 8); c != NULL; c = next)
	{
		next = (char *)DB_PTR(c, 0x1140);
		DB_VFN(c, 0xe0, void (*)(void *))(c);
		DB_VFN(c, 0x48, void *(*)(void *, unsigned int))(c, 1);
	}
	for (i = 0; i < 12; i++)
	{
		if (DB_PTR(m, 0x1c + i * 4) != NULL)
		{
			bkDeleteResource((struct bkResource *)DB_PTR(m, 0x1c + i * 4));
			DB_PTR(m, 0x1c + i * 4) = NULL;
		}
	}
	tables = (char *)DB_PTR(m, 0x94);
	if (tables != NULL)
	{
		/* delete[] (eh vector destructor iterator, destructor 0x000ad880) */
		int *block = (int *)tables - 1;

		for (i = *block - 1; i >= 0; i--)
			CLRouteHeights_Free2((struct CLRouteHeights *)(tables + i * 0x18));
		bkHeapFree(block);
		DB_PTR(m, 0x94) = NULL;
	}
	Actor_DeleteAll();
	bkDeletePackageResources(DB_U32(m, 4), 1);
}
