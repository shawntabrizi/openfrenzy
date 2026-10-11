/*
CarryOn/CFire_Game.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Game.cpp

The Fireworks minigame (CarryOn): the game object at 0x00293ad8 loads the packages, the
level, the players and the variant's machinery (launcher, glitter, conveyors, paint),
runs the frame and draws it. The carryon_b game mode (0x000d22c0-0x000d2430) calls it.

Functions tagged with CFire_Game.cpp (allocations with the file name): 0x000bad10,
0x000bae70, 0x000bba60, 0x000bc3d0, 0x000bd820. The others lie between them or right
before them and work on the same object (file inferred).
*/

#include "carryon_a.h"

/* 0x00267468  the vtable of CFire_Game */
const void *const vtable_00267468[] = {
	(const void *)CFire_Game_DeletingDtor,  /* [0] +0x00 */
};

/* 0x000ad830  CharLib/Route.cpp: the constructor of one route table (0x18 bytes):
two arrays of the grid's cells (a short and a byte per cell). The address is missing
from the function table (only the "eh vector constructor iterator" call at 0x000bad98
reaches it), so it is local here; reported to the architect. */
static void *co_RouteTable_Ctor(void *self)
{
	CO_PTR(self, 0x0) = bkHeapAlloc(DAT_00292c78 * 2,
		"C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23d, 0x2001);
	CO_PTR(self, 0x4) = bkHeapAlloc(DAT_00292c78,
		"C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23e, 0x2001);
	return self;
}

/* 0x000ba700  CarryOn/CFire_Game.cpp (file inferred)
Variant 3: loads the four ramp textures alphatex\p1ramp_anim.gif .. p4ramp_anim.gif. */
void CFire_Game_LoadRampTextures(struct CFireGame *self)
{
	char name[80];
	int i;

	for (i = 0; i < 4; i++)
	{
		sprintf(name, "alphatex\\p%dramp_anim.gif", i + 1);
		self->rampTex[i] = bLoadResource(NULL, name, 0, 0xdefa);
	}
	self->rampFrame = 0.0f;
}

/* 0x000ba760  CarryOn/CFire_Game.cpp (file inferred)
Variant 3: animates the ramp textures (8 frames per character colour, half a frame per
frame); each spawn point shows its player's colour. */
void CFire_Game_UpdateRampTextures(struct CFireGame *self, float step)
{
	float frame;
	int i;

	self->rampFrame = step * 0.5f + self->rampFrame;    /* 0x002536d0 */
	if ((int)self->rampFrame >= 8)
	{
		frame = self->rampFrame;
		do
			frame -= 8.0f;                                  /* 0x0025552c */
		while ((int)frame >= 8);
		self->rampFrame = frame;
	}
	for (i = 0; i < 4; i++)
		bdTextureSetFrame(self->rampTex[self->spawn[i]],
			(int)self->rampFrame + DAT_002baed4[i][0] * 8);
}

/* 0x000ba7f0  CarryOn/CFire_Game.cpp (file inferred)
Variants 1 and 4: an overlay panel per player (alphatex\panel_middle.bmp) in the player's
colour, half transparent; hidden for CPU players. */
void CFire_Game_CreatePanels(struct CFireGame *self)
{
	int desc[0xec];
	void *texture;
	int panel;
	int i;

	memset(desc, 0, sizeof desc);
	texture = bLoadResource(NULL, "alphatex\\panel_middle.bmp", 0, 0xdefa);
	for (i = 0; i < 4; i++)
	{
		if (texture == NULL)
		{
			self->panels[i] = NULL;
			continue;
		}
		/* the overlay description (0x3b0 bytes): texture, colour, flags */
		memset(desc, 0, sizeof desc);
		desc[0xa2] = 0;
		*(float *)&desc[0x94] = (float)DAT_00290718[DAT_002baed4[i][0] * 0x60 + 0];
		*(float *)&desc[0x95] = (float)DAT_00290718[DAT_002baed4[i][0] * 0x60 + 1];
		*(float *)&desc[0x96] = (float)DAT_00290718[DAT_002baed4[i][0] * 0x60 + 2];
		*(float *)&desc[0x97] = 127.0f;                 /* alpha */
		desc[0xa4] = 0x8000;
		desc[1] = (int)texture;
		/* a child of the score HUD's overlay of this player */
		panel = COverlay_CreateChild((int)CO_PTR(DAT_002ecfac, 0xf0 + i * 4), (undefined4 *)desc);
		self->panels[i] = (void *)panel;
		CO_U16(panel, 0x54) = (unsigned short)(DAT_002baed4[i][1] == 0);
	}
}

/* 0x000ba910  CarryOn/CFire_Game.cpp (file inferred)
Variants 1 and 4: shows the HUD markers (overlays of the score HUD) that match each
player's firework count (1..5) and colours them; at 5 the player's panel lights up in
full colour, otherwise it is drawn at half (the marker) or a third (the panel) of it.
`step` (the frame time) is not used. */
void CFire_Game_UpdatePanels(struct CFireGame *self, float step)
{
	unsigned char *rgb;
	void *hud;
	void *marker;
	int count;
	int i;

	for (i = 0; i < 4; i++)
	{
		hud = DAT_002ecfac;
		count = CO_I32(g_coPlayers[i], 0x1130);
		CO_U16(CO_PTR(hud, 0x110 + i * 0x10), 0x54) = count == 1 ? 0 : 1;
		CO_U16(CO_PTR(hud, 0x114 + i * 0x10), 0x54) = count == 2 ? 0 : 1;
		CO_U16(CO_PTR(hud, 0x118 + i * 0x10), 0x54) = count == 3 ? 0 : 1;
		CO_U16(CO_PTR(hud, 0x11c + i * 0x10), 0x54) = count == 4 ? 0 : 1;
		rgb = &DAT_00290718[DAT_002baed4[i][0] * 0x60];
		marker = CO_PTR(hud, 0xe0 + i * 4);
		if (count == 5)
		{
			CO_U16(CO_PTR(hud, 0x11c + i * 0x10), 0x54) = 0;
			CO_F32(marker, 0x2b8) = 127.0f;
			CO_F32(marker, 0x2ac) = (float)rgb[0];
			CO_F32(marker, 0x2b0) = (float)rgb[1];
			CO_F32(marker, 0x2b4) = (float)rgb[2];
			if (self->panels[i] != NULL)
			{
				CO_F32(self->panels[i], 0x2b8) = 127.0f;
				CO_F32(self->panels[i], 0x2ac) = (float)rgb[0];
				CO_F32(self->panels[i], 0x2b0) = (float)rgb[1];
				CO_F32(self->panels[i], 0x2b4) = (float)rgb[2];
			}
		}
		else
		{
			CO_F32(marker, 0x2b8) = 127.0f;
			CO_F32(marker, 0x2ac) = (float)(rgb[0] >> 1);
			CO_F32(marker, 0x2b0) = (float)(rgb[1] >> 1);
			CO_F32(marker, 0x2b4) = (float)(rgb[2] >> 1);
			if (self->panels[i] != NULL)
			{
				CO_F32(self->panels[i], 0x2b8) = 127.0f;
				CO_F32(self->panels[i], 0x2ac) = (float)(rgb[0] / 3);
				CO_F32(self->panels[i], 0x2b0) = (float)(rgb[1] / 3);
				CO_F32(self->panels[i], 0x2b4) = (float)(rgb[2] / 3);
			}
		}
	}
}

/* 0x000babe0  CarryOn/CFire_Game.cpp (file inferred)
Sets the variant (0..3). charlib's CModule_CarryOn_Start calls it. */
void CFire_Game_SetVariantIndex(struct CFireGame *self, int variant)
{
	self->flags ^= ((unsigned int)variant * 2 ^ self->flags) & 6;
}

/* 0x000bac00  CarryOn/CFire_Game.cpp (file inferred)
Loads an animated texture that CFire_Game_UpdateAnimTextures plays at `speed` frames
per frame. Returns its index, or -1 when it does not load. */
int CFire_Game_AddAnimTexture(struct CFireGame *self, const char *name, float speed)
{
	int index;

	self->animTex[self->numAnim] = bLoadResource(NULL, name, 0, 0xdefa);
	if (self->animTex[self->numAnim] == NULL)
		return -1;
	bdTextureGetInfo(self->animTex[self->numAnim], NULL, NULL, NULL,
		&self->animFrames[self->numAnim], NULL);
	self->animSpeed[self->numAnim] = speed;
	self->animFrame[self->numAnim] = 0.0f;
	index = self->numAnim;
	self->numAnim = index + 1;
	return index;
}

/* 0x000bac90  CarryOn/CFire_Game.cpp (file inferred)
Advances the animated textures and wraps them at their frame count. */
void CFire_Game_UpdateAnimTextures(struct CFireGame *self)
{
	int i;

	for (i = 0; i < self->numAnim; i++)
	{
		self->animFrame[i] = self->animSpeed[i] + self->animFrame[i];
		if ((int)self->animFrame[i] >= (int)self->animFrames[i])
		{
			do
				self->animFrame[i] = self->animFrame[i] - (float)(int)self->animFrames[i];
			while ((int)self->animFrame[i] >= (int)self->animFrames[i]);
		}
		bdTextureSetFrame(self->animTex[i], (int)self->animFrame[i]);
	}
}

/* 0x000bad10  CarryOn/CFire_Game.cpp
Starts loading in the background (the carryon_b mode's first activation): the route
tables, the variant's package packages\carryon_v<n> and packages\carryon_gen. */
void CFire_Game_BeginLoad(struct CFireGame *self)
{
	char name[128];
	int count;
	int *block;
	char *tables;
	int i;

	self->state = CO_STATE_LOADING;
	self->savedNear = DAT_0028ffa8;
	self->savedFar = DAT_0028ffac;
	count = DAT_00292c78;
	block = bkHeapAlloc(count * 0x18 + 4, CO_FILE_GAME, 0x22d, 0x1001);
	if (block == NULL)
		tables = NULL;
	else
	{
		/* new CLRouteTable[count] (eh vector constructor iterator, destructor 0x000ad880) */
		tables = (char *)(block + 1);
		*block = count;
		for (i = 0; i < count; i++)
			co_RouteTable_Ctor(tables + i * 0x18);
	}
	DAT_002f6b88 = tables;
	FUN_000c39b0((undefined4)DAT_002f6ba0, (undefined4)tables, 0x18, 0x18);
	sprintf(name, "packages\\carryon_v%d", CO_VARIANT(&g_fireGame) + 1);
	bkPrintf("Background Loading Begin...\n");
	DAT_002f6b90 = co_LoadPackage(name, 0);
	if (DAT_002f6b90 == NULL)
		bkPrintf("ERROR firework package failed to load\n");
	DAT_002f6b94 = co_LoadPackage("packages\\carryon_gen", 0);
	if (DAT_002f6b94 == NULL)
		bkPrintf("ERROR firework generic package failed to load\n");
	self->numAnim = 0;
	self->rumble = DAT_00283ea4;
}

/* 0x000bae70  CarryOn/CFire_Game.cpp
"Init One Time": loads the resources, the camera, the collision, the character models,
the players, the ASE paths (variant 4), the level and the carried objects. (The
original guards each allocation with a C++ exception frame; the port has none.) */
void CFire_Game_InitOnce(struct CFireGame *self)
{
	/* the per-variant shadow light direction */
	static const float lightDir[4][3] = {
		{ 1.0f, -1.4f, 0.0f },
		{ 1.0f, 1.4f, 0.0f },
		{ 0.6f, 0.6f, 0.0f },
		{ 0.6f, 2.9f, 0.0f },
	};
	struct CoTextReader reader;
	struct CFireASEImport ase;
	char name[100];
	float origin[4];
	void *levelPackage;
	void *genericPackage;
	unsigned int size;
	void *buffer;
	void *mem;
	int variant;
	int i;

	CFire_TextReader_Ctor(&reader);
	CFire_ASEImport_Ctor(&ase);
	levelPackage = DAT_002f6b90;
	genericPackage = DAT_002f6b94;
	bkPrintf("opened level package\n");
	self->state = CO_STATE_LOADED;
	bLoadPackageResources(DAT_002f6b90, 1, 0xdefa, 0);
	bLoadPackageResources(DAT_002f6b94, 1, 0xdefa, 0);
	mem = bkHeapAlloc(0x3e0, CO_FILE_GAME, 0x277, 0x1001);
	if (mem == NULL)
		g_fireRes = NULL;
	else
		g_fireRes = CFire_Resource_Ctor(mem, levelPackage, genericPackage,
			CO_VARIANT(self), DAT_002bae94);
	Debris_Init(NULL, 1000);
	co_CharLibInitShadows();
	mem = bkHeapAlloc(0xd4, CO_FILE_GAME, 0x27e, 0x1001);
	if (mem == NULL)
		g_fireCam = NULL;
	else
		g_fireCam = CFire_Camera_Ctor(mem, CO_VARIANT(&g_fireGame));
	origin[0] = 0.0f;
	origin[1] = 0.0f;
	origin[2] = 0.0f;
	origin[3] = 0.0f;
	sprintf(name, CO_I32(g_fireRes, 0x258) == 0 ? "map\\collide.obe" : "test\\collide.obe");

	/* the route grid of this level (the previous one is restored by _Shutdown) */
	self->savedBounds[0] = DAT_00292c60;
	self->savedBounds[3] = DAT_00292c6c;
	variant = CO_VARIANT(&g_fireGame);
	self->savedBounds[1] = DAT_00292c64;
	DAT_00292c60 = DAT_00293d10[variant][0];
	self->savedBounds[2] = DAT_00292c68;
	DAT_00292c64 = DAT_00293d10[variant][1];
	DAT_00292c6c = DAT_00293d10[variant][3];
	DAT_00292c68 = DAT_00293d10[variant][2];
	if (DAT_00292c68 / (float)DAT_00292c4c <= DAT_00292c6c / (float)DAT_00292c50)
		CO_F32(g_fireRes, 0x374) = DAT_00292c6c / (float)DAT_00292c50;
	else
		CO_F32(g_fireRes, 0x374) = DAT_00292c68 / (float)DAT_00292c4c;

	self->collision = Actor_Load(name, DAT_002f6b90, origin, 0, NULL, 0);
	CLRoute_Rebuild();
	for (i = 0; i < 6; i++)
	{
		DAT_00293a68[i].actor = bLoadResource(DAT_002f6b94, DAT_00293a68[i].name, 1, 0);
		DAT_00293a68[i].anim = bLoadResource(DAT_002f6b94, DAT_00293a68[i].animName, 1, 0);
	}

	for (i = 0; i < DAT_002bae94; i++)
	{
		DAT_00293cfc[0] = (float)(co_rand() % 0xaf0 - 0x578);
		DAT_00293cfc[2] = (float)(co_rand() % 0xaf0 - 0x578);
		mem = bkHeapAlloc(0x1204, CO_FILE_GAME, 0x2cd, 0x1001);
		if (mem == NULL)
			g_coPlayers[i] = NULL;
		else
			g_coPlayers[i] = CCarryOn_Player_Ctor(mem, i, DAT_00293cfc);
		DAT_002f289c[i] = g_coPlayers[i];
	}
	bdSetWorldMatrix(DAT_002855f0);
	bdSetProjectionMode(1);

	/* variant 4: the fuses, paint paths and conveyors from the ASE file */
	if (DAT_00293ac8[CO_VARIANT(self)] != NULL)
	{
		buffer = bkLoadFile(levelPackage, DAT_00293ac8[CO_VARIANT(self)], NULL, &size, NULL, 0);
		if (buffer != NULL)
		{
			co_TextReader_Init(&reader, buffer, size);
			if ((self->flags & 6) == 6)
			{
				self->numFuses = 0;
				CFire_ASEImport_ReadPaths(&ase, &reader, &self->fuses, &self->numFuses, "Fuse");
				self->numPaintPaths = 0;
				CFire_ASEImport_ReadPaths(&ase, &reader, &self->paintPaths,
					&self->numPaintPaths, "Paint");
				self->numConvey = 0;
				CFire_ASEImport_ReadPaths(&ase, &reader, &self->convey, &self->numConvey,
					"Convey");
			}
		}
	}

	mem = bkHeapAlloc(0x23c8, CO_FILE_GAME, 0x2fc, 0x1001);
	self->level = mem == NULL ? NULL : Meta_ReturnThis(mem);
	variant = CO_VARIANT(self);
	if (CFire_World_Load(self->level, levelPackage, variant, PTR_s_rocket_0029418c[variant]) != 0)
	{
		self->ended = 0;
		co_IntroCam_Init();
		mem = bkHeapAlloc(0x68, CO_FILE_GAME, 0x306, 0x1001);
		if (mem == NULL)
			self->carry = NULL;
		else
			self->carry = CFire_Carry_Ctor(mem, levelPackage, genericPackage, CO_VARIANT(self));
		variant = CO_VARIANT(self);
		co_SetShadowLight(2, lightDir[variant][0], lightDir[variant][1], lightDir[variant][2]);
	}
	CFire_ASEImport_Dtor(&ase);
	CFire_TextReader_Dtor(&reader);
}

/* 0x000bb480  CarryOn/CFire_Game.cpp (file inferred)
Ranks the players (core's score table): `order` lists them by rank with ties shuffled,
`shuffle` lists them at random (20 random swaps). Also clears the scores. */
void CFire_Game_ShuffleOrder(struct CFireGame *self)
{
	int *numAtRank;
	int first;
	int a, b, t;
	int i, n;

	for (i = 0; i < DAT_002bae94; i++)
	{
		DAT_002baed4[i][5] = 0;
		self->shuffle[i] = i;
	}
	CScoreTable_Rank((struct CScoreTable *)DAT_002b9968);
	bkPrintf("playerInPosition List = %d %d %d %d\n", DAT_002b9c58[0], DAT_002b9c58[1],
		DAT_002b9c58[2], DAT_002b9c58[3]);
	bkPrintf("num players in position = %d %d %d %d\n", DAT_002b9c68[0], DAT_002b9c68[1],
		DAT_002b9c68[2], DAT_002b9c68[3]);
	for (i = 0; i < DAT_002bae94; i++)
		self->order[i] = DAT_002b9c58[i];
	bkPrintf("Pre-Shuffling List = %d %d %d %d\n", self->order[0], self->order[1],
		self->order[2], self->order[3]);

	/* shuffle the players that share a rank: 20 random swaps inside each group */
	first = 0;
	for (numAtRank = DAT_002b9c68; numAtRank < DAT_002b9c68 + 3; numAtRank++)
	{
		if (*numAtRank > 1)
		{
			for (n = 0; n < 20; n++)
			{
				a = first + co_rand() % *numAtRank;
				b = first + co_rand() % *numAtRank;
				t = self->order[a];
				self->order[a] = self->order[b];
				self->order[b] = t;
			}
		}
		first += *numAtRank;
	}

	for (n = 0; n < 20; n++)
	{
		a = co_rand() & 3;
		b = co_rand() & 3;
		t = self->shuffle[a];
		self->shuffle[a] = self->shuffle[b];
		self->shuffle[b] = t;
	}
	bkPrintf("Rubber order is %d %d %d %d\n", self->order[0], self->order[1], self->order[2],
		self->order[3]);
	bkPrintf("Random order is %d %d %d %d\n", self->shuffle[0], self->shuffle[1],
		self->shuffle[2], self->shuffle[3]);
}

/* 0x000bba60  CarryOn/CFire_Game.cpp
The rest of "Init One Time": fireworks, spawn points, the variant's machinery, the
package ids and the "FireTweaks" tweak menu. */
void CFire_Game_InitObjects(struct CFireGame *self)
{
	float launcherPos[4];
	float beltA[4];
	float beltB[4];
	float paintPos[4];
	char name[20];
	void *mem;
	int i;
	float duration;

	self->fireworks = bkHeapAlloc(0x3c, CO_FILE_GAME, 0x380, 0x1001);
	co_Fireworks_Init(self->fireworks, CO_VARIANT(self));
	mem = bkHeapAlloc(0x1c, CO_FILE_GAME, 899, 0x1001);
	self->spawner = mem == NULL ? NULL : co_Spawner_Ctor(mem);
	co_Spawner_Init(self->spawner, CO_VARIANT(self), self->fireworks);
	CFire_Game_ShuffleOrder(self);
	self->spawn[self->order[0]] = DAT_00293d50[0];
	self->spawn[self->order[1]] = DAT_00293d50[1];
	self->spawn[self->order[2]] = DAT_00293d50[2];
	self->spawn[self->order[3]] = DAT_00293d50[3];
	bkPrintf("Spawnpoints = %d %d %d %d\n", self->spawn[0], self->spawn[1], self->spawn[2],
		self->spawn[3]);
	for (i = 0; i < CO_I32(self->level, 0x398); i++)
		co_Spawner_AddPoint(self->spawner,
			CO_ADDR(self->level, (self->spawn[i] + 0x2a) * 0x14), i);
	for (i = 0; i < CO_I32(self->level, 0x344); i++)
		co_Spawner_AddPad(self->spawner, CO_ADDR(self->level, 0x2a4 + i * 0x14), i);

	self->launcher = NULL;
	self->conveyor[0] = NULL;
	self->conveyor[1] = NULL;
	self->paint = NULL;
	self->obj02c = NULL;
	self->int04c = 0;
	switch (CO_VARIANT(self))
	{
	case 0:
		launcherPos[0] = -5.0f;
		launcherPos[1] = 0.0f;
		launcherPos[2] = -1100.0f;
		launcherPos[3] = 1.0f;
		mem = bkHeapAlloc(0x130, CO_FILE_GAME, 0x3b1, 0x1001);
		if (mem == NULL)
			self->launcher = NULL;
		else
		{
			co_Launcher_Ctor(mem);
			self->launcher = mem;
		}
		co_Launcher_Init(self->launcher, self->fireworks, launcherPos, 1200.0f, 280.0f, 0.0f);
		duration = DAT_002940f8;
		break;
	case 1:
		mem = bkHeapAlloc(0x128, CO_FILE_GAME, 0x3b8, 0x1001);
		self->glitter = mem == NULL ? NULL : co_Glitter_Ctor(mem);
		co_Nothing();                                   /* an empty init of the glitter */
		duration = DAT_002940fc;
		break;
	case 2:
		duration = DAT_00294100;
		break;
	case 3:
		beltA[0] = 0.0f;
		beltA[1] = 0.0f;
		beltA[2] = -850.0f;
		beltA[3] = 1.0f;
		beltB[0] = 0.0f;
		beltB[1] = 0.0f;
		beltB[2] = -650.0f;
		beltB[3] = 1.0f;
		paintPos[0] = 0.0f;
		paintPos[1] = 0.0f;
		paintPos[2] = 450.0f;
		paintPos[3] = 1.0f;
		beltA[1] = beltA[1] - CLColl_GroundProbe(beltA, 0.0f, NULL, NULL, 0);
		beltB[1] = beltB[1] - CLColl_GroundProbe(beltB, 0.0f, NULL, NULL, 0);
		mem = bkHeapAlloc(0x330, CO_FILE_GAME, 0x3d5, 0x1001);
		self->conveyor[0] = mem == NULL ? NULL : CFire_Belt_Ctor(mem);
		CFire_Belt_Init(self->conveyor[0], self->fireworks, self->convey, 0.016f, 3, 0x18, 0xe);
		mem = bkHeapAlloc(0x330, CO_FILE_GAME, 0x3d7, 0x1001);
		self->conveyor[1] = mem == NULL ? NULL : CFire_Belt_Ctor(mem);
		CFire_Belt_Init(self->conveyor[1], self->fireworks,
			(char *)self->convey + 0xc, 0.0103f, 5, 0x18, 8);
		self->paint = bkHeapAlloc(0x24, CO_FILE_GAME, 0x3e1, 0x1001);
		co_Paint_Init(self->paint, paintPos, 0.31416f, self->fuses, self->numFuses,
			self->paintPaths, self->numPaintPaths);
		duration = DAT_00294104;
		break;
	default:
		goto noDuration;
	}
	self->timeLeft = duration * 1000.0f;                /* 0x00255898 */
noDuration:
	DAT_002f6b84 = bkGetPackageId(DAT_002f6b94);
	DAT_002f6b80 = bkGetPackageId(DAT_002f6b90);
	DAT_002f6b8c = bLoadResource(NULL, "map\\collide.obe", 1, 0);
	self->frame = 0;
	self->winnerShown = 0;
	sprintf(name, "fwk_v%d", CO_VARIANT(self));
	mem = bkHeapAlloc(0xe8, CO_FILE_GAME, 0x42b, 0x1001);
	self->tweaks = mem == NULL ? NULL : CTweakMenu_Ctor(mem, "FireTweaks");
	CTweakMenu_AddBool(self->tweaks, "Debug Lines", &DAT_002f6d60);
	CTweakMenu_AddInt(self->tweaks, "Detail", &DAT_00294118, 1, 0, 100, 0);
	CTweakMenu_AddBool(self->tweaks, "PlayerDetail", &DAT_0029411c);
	CTweakMenu_AddBool(self->tweaks, "PlayerShadows", &DAT_00293a54);
	CTweakMenu_AddBool(self->tweaks, "TA's Panel", &DAT_00293a50);
	CTweakMenu_AddBool(self->tweaks, "Timer", &DAT_002f6b98);
	CTweakMenu_AddBool(self->tweaks, "Alphabits", &DAT_00293a58);
	CTweakMenu_AddBool(self->tweaks, "Fireworks", &DAT_00294114);
	CTweakMenu_AddBool(self->tweaks, "PointLights", &DAT_00293a60);
	CTweakMenu_AddFloat(self->tweaks, "Basic Burden", &DAT_002f6d3c, 0.025f, 0.0f, 1.0f);
	switch (CO_VARIANT(self))
	{
	case 0:
		CTweakMenu_AddFloat(self->tweaks, "V1:Duration", &DAT_002940f8, 1.0f, 0.0f, 1000.0f);
		CTweakMenu_AddFloat(self->tweaks, "V1:FWorkWeight", &DAT_00293f38, 0.025f, 0.0f, 1.0f);
		CTweakMenu_AddInt(self->tweaks, "V1:Count", &DAT_002943b4, 1, 0, 20, 0);
		CTweakMenu_AddInt(self->tweaks, "V1:Launcher", &DAT_00294390, 1, 0, 50, 0);
		CTweakMenu_AddFloat(self->tweaks, "V1:LInterval", &DAT_00294394, 60.0f, 0.0f, 1800.0f);
		CTweakMenu_AddFloat(self->tweaks, "V1:LSpeed", &DAT_00294398, 1.0f, 0.0f, 60.0f);
		CTweakMenu_AddBool(self->tweaks, "V1:Instant KO", &DAT_002f6d38);
		CTweakMenu_AddFloat(self->tweaks, "V1:Probability", &DAT_00294120, 0.001f, 0.0f, 1.0f);
		break;
	case 1:
		CTweakMenu_AddFloat(self->tweaks, "V2:Duration", &DAT_002940fc, 1.0f, 0.0f, 1000.0f);
		CTweakMenu_AddFloat(self->tweaks, "V2:GlitterWeight", &DAT_00293f3c, 0.025f, 0.0f, 1.0f);
		CTweakMenu_AddFloat(self->tweaks, "V2:Glitter Loss", &DAT_00293f18, 0.01f, 0.0f, 1.0f);
		break;
	case 2:
		CTweakMenu_AddFloat(self->tweaks, "V3:Duration", &DAT_00294100, 1.0f, 0.0f, 1000.0f);
		CTweakMenu_AddFloat(self->tweaks, "V3:PartWeight", &DAT_00293f40, 0.025f, 0.0f, 1.0f);
		CTweakMenu_AddInt(self->tweaks, "V3:Parts/Firework", &DAT_00294108, 1, 2, 5, 0);
		CTweakMenu_AddInt(self->tweaks, "V3:Part Types", &DAT_0029410c, 1, 2, 5, 0);
		CTweakMenu_AddBool(self->tweaks, "V3:Non-Random Ponk", &DAT_00294110);
		CTweakMenu_AddBool(self->tweaks, "V3:Steal on Hurt", &DAT_00293f24);
		CTweakMenu_AddBool(self->tweaks, "V3:Steal on KO", &DAT_00293f28);
		CTweakMenu_AddBool(self->tweaks, "V3:Instant KO", &DAT_00293f2c);
		CTweakMenu_AddInt(self->tweaks, "V3:Lightning Max", &DAT_00294128, 1, 0, 10, 0);
		CTweakMenu_AddInt(self->tweaks, "V3:Comet Max", &DAT_0029412c, 1, 0, 10, 0);
		CTweakMenu_AddInt(self->tweaks, "V3:Pandy Max", &DAT_00294134, 1, 0, 10, 0);
		break;
	case 3:
		CTweakMenu_AddBool(self->tweaks, "Conveyors", &DAT_00293a5c);
		CTweakMenu_AddFloat(self->tweaks, "V4:Duration", &DAT_00294104, 1.0f, 0.0f, 1000.0f);
		CTweakMenu_AddBool(self->tweaks, "V4:Launch Anyones", &DAT_00294348);
		CTweakMenu_AddBool(self->tweaks, "V4:GivePointsAway", &DAT_0029434c);
		CTweakMenu_AddBool(self->tweaks, "V4:PaintKeepsGoing", &DAT_00294350);
		break;
	}
	CTweakMenu_AddInt(self->tweaks, "FogR", &DAT_0029413c[CO_VARIANT(self)], 1, 0, 0xff, 0);
	CTweakMenu_AddInt(self->tweaks, "FogG", &DAT_0029414c[CO_VARIANT(self)], 1, 0, 0xff, 0);
	CTweakMenu_AddInt(self->tweaks, "FogB", &DAT_0029415c[CO_VARIANT(self)], 1, 0, 0xff, 0);
	CTweakMenu_AddFloat(self->tweaks, "FogNear", &DAT_0029416c[CO_VARIANT(self)], 100.0f, 0.0f,
		1000000.0f);
	CTweakMenu_AddFloat(self->tweaks, "FogFar", &DAT_0029417c[CO_VARIANT(self)], 100.0f, 500.0f,
		1000000.0f);
}

/* 0x000bc3d0  CarryOn/CFire_Game.cpp
Starts the round (the carryon_b mode's activation): spawns the players, sets up the
controls, the camera, the HUD (clock and score bars), the variant's extras and the
lights. */
void CFire_Game_Start(struct CFireGame *self)
{
	void *mem;
	int *ranks;
	int i;

	baFlushShaderCache();
	mem = bkHeapAlloc(0xc, CO_FILE_GAME, 0x497, 0x1001);
	if (mem != NULL)
		CLList_Init(mem);
	self->overlays = mem;
	DAT_0028ffa8 = 4000.0f;
	DAT_0028ffac = 10000.0f;
	if (DAT_002ba56c != 0)
	{
		mem = bkHeapAlloc(0x26c, CO_FILE_GAME, 0x4b7, 0x1001);
		if (mem != NULL)
			co_DemoCam_Ctor(mem, 0, 180.0f, 1);
	}

	/* spawn: variant 3 at the spawn point given by the rank, variant 4 at the point of
	   the player's rank, the others where the AI likes */
	switch (CO_VARIANT(self))
	{
	case 2:
		for (i = 0; i < 4; i++)
			CO_VFN(g_coPlayers[i], 0x28, void (*)(void *, void *))(g_coPlayers[i],
				CO_PTR(self->level, 0x2324 + self->spawn[i] * 4));
		break;
	case 3:
		ranks = self->order;
		for (i = 0; i < 4; i++)
			CO_VFN(g_coPlayers[ranks[i]], 0x28, void (*)(void *, void *))(g_coPlayers[ranks[i]],
				CO_PTR(self->level, 0x2324 + i * 4));
		break;
	default:
		for (i = 0; i < DAT_002bae94; i++)
			CO_VFN(g_coPlayers[i], 0x24, void (*)(void *))(g_coPlayers[i]);
		break;
	}
	CCarryOn_Player_CreateControls();
	CFire_Camera_Reset(g_fireCam);
	co_Overlays_SetList(self->overlays);
	CFire_Resource_UpdateDetail(g_fireRes);
	mem = bkHeapAlloc(0x5c, CO_FILE_GAME, 0x4f3, 0x1001);
	self->sky = mem == NULL ? NULL : co_Sky_Ctor(mem);
	CLHud_CreateBars((int)DAT_002ecfac);
	self->scores[3] = 0;
	self->scores[2] = 0;
	self->scores[1] = 0;
	self->scores[0] = 0;
	CLHud_CreateScoreBoxes((int)DAT_002ecfac);
	switch (CO_VARIANT(self))
	{
	case 0:
		CLHud_CreateClock((int)DAT_002ecfac, DAT_002940f8);
		co_Hud_SetCountdown(DAT_002ecfac, 10.0f);
		CLHud_CreateScoreTexts((int)DAT_002ecfac, self->scores);
		self->barMax = 30.0f;
		self->barGrow = 0.05f;
		CFire_Game_CreatePanels(self);
		break;
	case 1:
		co_Glitter_Start(self->glitter);
		CLHud_CreateClock((int)DAT_002ecfac, DAT_002940fc);
		co_Hud_SetCountdown(DAT_002ecfac, 10.0f);
		CLHud_CreateScoreTexts((int)DAT_002ecfac, self->scores);
		self->barMax = 300.0f;
		self->barGrow = 0.2f;
		CO_F32(DAT_002ecfac, 0x40) = 20.0f;
		CO_F32(DAT_002ecfac, 0x3c) = 0.0f;
		CFire_Game_AddAnimTexture(self, "alphatex\\reanim.gif", 1.0f);
		CFire_Game_AddAnimTexture(self, "alphatex\\glitjar_anim.gif", 1.0f);
		goto lights;
	case 2:
		CLHud_CreateClock((int)DAT_002ecfac, DAT_00294100);
		co_Hud_SetCountdown(DAT_002ecfac, 10.0f);
		CLHud_CreateScoreTexts((int)DAT_002ecfac, self->scores);
		self->barMax = 100.0f;
		self->barGrow = 0.05f;
		CFire_Game_LoadRampTextures(self);
		break;
	default:
		CLHud_CreateClock((int)DAT_002ecfac, DAT_00294104);
		co_Hud_SetCountdown(DAT_002ecfac, 10.0f);
		CLHud_CreateScoreTexts((int)DAT_002ecfac, self->scores);
		self->barMax = 50.0f;
		self->barGrow = 0.05f;
		CFire_Game_CreatePanels(self);
		break;
	}
	CO_F32(DAT_002ecfac, 0x40) = 20.0f;
	CO_F32(DAT_002ecfac, 0x3c) = 0.0f;
lights:
	self->state = (DAT_002baeb8 != 0) * 2 + 2;          /* intro, or play in demo mode */
	self->lights[0] = co_Sky_CreateLight(self->sky, 4, DAT_00293d60[CO_VARIANT(self)] + 0x00,
		0x7f, 0x7f, 0x7f, 0);
	self->lights[1] = co_Sky_CreateLight(self->sky, 4, DAT_00293d60[CO_VARIANT(self)] + 0x10,
		0x7f, 0x7f, 0x7f, 0);
	self->lights[2] = co_Sky_CreateLight(self->sky, 4, DAT_00293d60[CO_VARIANT(self)] + 0x20,
		0x7f, 0x7f, 0x7f, 0);
	self->projector = bkHeapAlloc(0x90, CO_FILE_GAME, 0x566, 0x1001);
	co_ExtraFx_Init(self->projector, 20);
	CO_F32(DAT_002f27c4, 0x80) = CO_F32(DAT_002f27c4, 0x80) * 30.0f;   /* 0x002553a8 */
	CO_F32(DAT_002f27c4, 0x88) = CO_F32(DAT_002f27c4, 0x88) * 60.0f;   /* 0x00254e98 */
	CO_F32(DAT_002f27c4, 0x84) = 0.12f;
	CFire_Resource_ReserveChannel(g_fireRes, CFire_Resource_FindSample(g_fireRes, 10,
		CO_VARIANT(self), 0x80));
	CProjector_Reset();
	RefBot_Create();
}

/* 0x000bc890  CarryOn/CFire_Game.cpp (file inferred)
Frees the route tables and closes both packages (the mode ends before the game ran). */
void CFire_Game_ClosePackages(void)
{
	int *block;
	int i;

	if (DAT_002f6b88 != NULL)
	{
		/* delete[] (eh vector destructor iterator, destructor 0x000ad880) */
		block = (int *)DAT_002f6b88 - 1;
		for (i = *block - 1; i >= 0; i--)
			CLRouteHeights_Free2((struct CLRouteHeights *)((char *)DAT_002f6b88 + i * 0x18));
		bkHeapFree(block);
		DAT_002f6b88 = NULL;
	}
	if (DAT_002f6b94 != NULL)
	{
		bkPrintf("closing generic meta package\n");
		bkClosePackage(DAT_002f6b94);
		DAT_002f6b94 = NULL;
	}
	if (DAT_002f6b90 != NULL)
	{
		bkPrintf("closing meta level package\n");
		bkClosePackage(DAT_002f6b90);
		DAT_002f6b90 = NULL;
	}
}

/* 0x000bc920  CarryOn/CFire_Game.cpp (file inferred)
While the packages load in the background: logs "Fireworks: Background Loading..." at
most once a second and lets the game mode (vtable +0x24) poll the loader. */
void CFire_Game_BackgroundLoad(struct CFireGame *self, int skip)
{
	unsigned long long now;
	float rate;
	float ms;

	if (skip != 0 || self->mode == NULL || CO_I32(self->mode, 0x14) == 0)
		return;
	now = bkTimerRead();
	rate = (float)DAT_002cd7a0;
	ms = (float)(long long)(bkTimerDelta(DAT_002f6be8, now) * 1000) / rate;
	if (ms < 0.0f || ms >= 1000.0f)
	{
		DAT_002f6be8 = now;
		bkPrintf("Fireworks: Background Loading...\n");
	}
	if (CO_I32(self->mode, 0x14) != 0)
		CO_VFN(self->mode, 0x24, void (*)(void *))(self->mode);
}

/* 0x000bc9f0  CarryOn/CFire_Game.cpp (file inferred)
"Debug Lines": prints the camera position, frame rate and timings, and outlines the
route grid. */
void CFire_Game_DrawDebug(void)
{
	D3DCAPS8 caps;
	float pos[4];
	float rect[4][4];
	unsigned long long delta;
	float fps;

	co_Timer_Stop(CO_ADDR(g_fireRes, 0x18), 2);
	D3DDevice_GetDeviceCaps(&caps);
	DAT_002f6bf0 = bkTimerRead();
	delta = bkTimerDelta(DAT_002f6bf8, DAT_002f6bf0);
	DAT_002f6bf8 = DAT_002f6bf0;
	fps = bkTimerRate((long long)delta);
	co_IntroCam_GetPos(pos);
	CFire_Resource_Printf(g_fireRes, 200.0f, 200.0f, 0xff00407f, 0, "X:%.2f", (double)pos[0]);
	CFire_Resource_Printf(g_fireRes, 200.0f, 184.0f, 0xff00407f, 0, "Y:%.2f", (double)pos[1]);
	CFire_Resource_Printf(g_fireRes, 200.0f, 168.0f, 0xff00407f, 0, "Z:%.2f", (double)pos[2]);
	CFire_Resource_Printf(g_fireRes, 200.0f, 136.0f, 0xff00407f, 0, "FPS:%.2f", (double)fps);
	CFire_Resource_Printf(g_fireRes, 200.0f, 120.0f, 0xff00407f, 0, "Update:%.2f",
		(double)CO_F32(g_fireRes, 0x30));
	CFire_Resource_Printf(g_fireRes, 200.0f, 104.0f, 0xff00407f, 0, "Render:%.2f",
		(double)CO_F32(g_fireRes, 0x48));
	CFire_Resource_Printf(g_fireRes, 200.0f, 88.0f, 0xff00407f, 0, "External:%.2f",
		(double)(CO_F32(g_fireRes, 0x60) - (CO_F32(g_fireRes, 0x48) + CO_F32(g_fireRes, 0x30))));
	CFire_Resource_Printf(g_fireRes, 200.0f, 72.0f, 0xff00407f, 0, "Three:%.2f",
		(double)CO_F32(g_fireRes, 0x78));
	CFire_Resource_Printf(g_fireRes, 200.0f, 56.0f, 0xff00407f, 0, "Polys:%d",
		(int)FUN_0003f010());

	/* the route grid's outline */
	rect[0][0] = DAT_00292c60;
	rect[0][1] = 0.0f;
	rect[0][2] = DAT_00292c64;
	rect[0][3] = 1.0f;
	rect[1][0] = DAT_00292c60;
	rect[1][1] = 0.0f;
	rect[1][2] = DAT_00292c6c + DAT_00292c64;
	rect[1][3] = 1.0f;
	rect[2][0] = DAT_00292c68 + DAT_00292c60;
	rect[2][1] = 0.0f;
	rect[2][2] = rect[1][2];
	rect[2][3] = 1.0f;
	rect[3][0] = rect[2][0];
	rect[3][1] = 0.0f;
	rect[3][2] = DAT_00292c64;
	rect[3][3] = 1.0f;
	DAT_002bc48c = 0;
	CFire_Resource_DrawLines(&rect[0][0], 4, 1, 0xff00ff00);
	co_Timer_Mark(CO_ADDR(g_fireRes, 0x18), 2);
}

/* 0x000bccb0  CarryOn/CFire_Game.cpp (file inferred)
Draws the frame when `draw` is set (the carryon_b mode's render): the camera, the sky
lights, the level layers, the variant's machinery, the players, the shadows, the
fireworks and the HUD. Each pass is timed with a colour (the resource's frame timer at
+0x18). (Ghidra split this function at 0x000bccbf; it is one function.) */
void CFire_Game_Draw(struct CFireGame *self, int draw)
{
	void *held[4];
	unsigned int saved[30];        /* 0x78 bytes in the original frame */
	void *timer;
	void *sky;
	unsigned int oldState;
	unsigned long long delta;
	float ms;
	int i;

	if (draw == 0)
		return;
	timer = CO_ADDR(g_fireRes, 0x18);
	co_Timer_Start(timer, 0xff000000);
	if (DAT_002f26c4 == NULL || self->state != CO_STATE_PLAY)
		CFire_Camera_Apply(g_fireCam);
	bdSetProjectionMode(1);
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

	/* the three sky lights: the first one of the list is the main light */
	sky = self->sky;
	if (self->state == CO_STATE_INTRO)
	{
		co_Sky_SetLight(sky, self->lights[0], 2, 2);
		co_Sky_SetLight(sky, self->lights[2], 2, 0);
		co_Sky_SetLight(sky, self->lights[1], 2, 0);
	}
	else if (self->state == CO_STATE_END)
	{
		co_Sky_SetLight(sky, self->lights[2], 2, 2);
		co_Sky_SetLight(sky, self->lights[0], 2, 0);
		co_Sky_SetLight(sky, self->lights[1], 2, 0);
	}
	else
	{
		co_Sky_SetLight(sky, self->lights[1], 2, 2);
		co_Sky_SetLight(sky, self->lights[2], 2, 0);
		co_Sky_SetLight(sky, self->lights[0], 2, 0);
	}
	co_Timer_Mark(timer, 1);

	/* the level */
	baSetAmbientLight(DAT_00293e50[CO_VARIANT(self)][0], DAT_00293e50[CO_VARIANT(self)][1],
		DAT_00293e50[CO_VARIANT(self)][2]);
	CFire_Resource_ApplyFog(g_fireRes);
	bdSetRenderState(4, 1, 0);
	co_Sky_EnableLights(self->sky, 4, 0);
	co_Sky_EnableLights(self->sky, 2, 0);
	co_Sky_EnableLights(self->sky, 1, 0);
	co_Sky_EnableLights(self->sky, 8, 0);
	co_Sky_EnableLights(self->sky, 0x10, 0);
	CFire_World_DrawSky(self->level);
	CFire_World_DrawFar(self->level);
	co_Timer_Start(timer, 0xff004000);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(4, 0, 0);
	if (self->glitter != NULL)
		co_Glitter_Draw(self->glitter, 7);
	co_Spawner_Draw(self->spawner, 7);
	if (self->launcher != NULL)
	{
		co_Sky_EnableLights(self->sky, 2, 0);
		co_Sky_EnableLights(self->sky, 1, 0);
		co_Sky_EnableLights(self->sky, 8, 0);
		co_Sky_EnableLights(self->sky, 4, 1);
		bdSetRenderState(3, 1, 0);
		co_Launcher_Draw(self->launcher);
	}
	co_Sky_EnableLights(self->sky, 4, 0);
	co_Sky_EnableLights(self->sky, 2, 0);
	co_Sky_EnableLights(self->sky, 1, 0);
	co_Sky_EnableLights(self->sky, 8, 0);
	bdSetRenderState(3, 0, 0);
	if (DAT_00293a5c != 0)
	{
		if (self->conveyor[0] != NULL)
			CFire_Belt_Draw(self->conveyor[0]);
		if (self->conveyor[1] != NULL)
			CFire_Belt_Draw(self->conveyor[1]);
	}

	/* the players */
	bdSetRenderState(4, 1, 0);
	co_Sky_EnableLights(self->sky, 1, 0);
	co_Sky_EnableLights(self->sky, 8, 0);
	co_Sky_EnableLights(self->sky, 2, 0);
	co_Sky_EnableLights(self->sky, 4, 1);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(4, 1, 0);
	baSetAmbientLight(DAT_00293e20[CO_VARIANT(self)][0], DAT_00293e20[CO_VARIANT(self)][1],
		DAT_00293e20[CO_VARIANT(self)][2]);
	CCarryOn_Player_DrawAll();
	bdSetRenderState(4, 0, 0);
	co_Timer_Start(timer, 0xff800080);

	/* the near level layers */
	bdSetRenderState(4, 1, 0);
	co_Sky_EnableLights(self->sky, 4, 0);
	if (DAT_00293a60 != 0)
	{
		co_Sky_EnableLights(self->sky, 1, 1);
		co_Sky_EnableLights(self->sky, 2, 1);
		co_Sky_EnableLights(self->sky, 8, 1);
	}
	else
	{
		co_Sky_EnableLights(self->sky, 1, 0);
		co_Sky_EnableLights(self->sky, 2, 0);
		co_Sky_EnableLights(self->sky, 8, 0);
	}
	co_Sky_EnableLights(self->sky, 0x10, 0);
	baSetAmbientLight(DAT_00293e50[CO_VARIANT(self)][0], DAT_00293e50[CO_VARIANT(self)][1],
		DAT_00293e50[CO_VARIANT(self)][2]);
	CFire_World_DrawNear(self->level);
	co_Timer_Start(timer, 0xffff0000);
	co_Sky_EnableLights(self->sky, 4, 0);
	co_Sky_EnableLights(self->sky, 2, 0);
	co_Sky_EnableLights(self->sky, 1, 0);
	co_Sky_EnableLights(self->sky, 8, 0);
	co_Sky_EnableLights(self->sky, 0x10, 0);
	CFire_World_DrawObjects(self->level);
	bdSetRenderState(4, 0, 0);
	co_Timer_Start(timer, 0xff400040);
	co_Sky_EnableLights(self->sky, 2, 0);
	co_Sky_EnableLights(self->sky, 1, 0);
	co_Sky_EnableLights(self->sky, 8, 0);
	co_Sky_EnableLights(self->sky, 4, 1);
	bdSetRenderState(3, 1, 0);
	if (self->carry != NULL)
		CFire_Carry_Draw(self->carry, 7);

	/* the players' shadows, with what they carry */
	if (DAT_00293a54 != 0)
	{
		bdSetWorldMatrix(DAT_002855f0);
		bdSetRenderState(6, 0, 0);
		bdSetRenderState(3, 0, 0);
		bdSetRenderState(8, 0, 0);
		bdSetProjectionMode(1);
		for (i = 0; i < 4; i++)
			held[i] = self->carry != NULL ? CFire_Carry_GetHeld(self->carry, i) : NULL;
		co_Shadows_Render(held);
		baDrawActorInstance(CO_PTR(self->collision, 0x184), 7);
		co_Shadows_End();
		D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	}
	bdSetWorldMatrix(DAT_002855f0);
	bdSetProjectionMode(1);
	co_Timer_Start(timer, 0xff808080);
	bdSetRenderState(4, 0, 0);
	CFire_World_DrawAlpha(self->level);
	co_Timer_Start(timer, 0xff800000);
	if (self->paint != NULL)
	{
		co_Sky_EnableLights(self->sky, 4, 0);
		co_Sky_EnableLights(self->sky, 2, 0);
		co_Sky_EnableLights(self->sky, 1, 0);
		co_Sky_EnableLights(self->sky, 8, 0);
		bdSetRenderState(3, 0, 0);
		co_Paint_Draw(self->paint);
	}
	co_Timer_Start(timer, 0xff0000ff);
	for (i = 0; i < DAT_002bae94; i++)
		CCarryOn_Player_DrawEffects(g_coPlayers[i]);
	bdSaveRenderStates(saved);
	baDrawExtras(NULL, NULL);
	bdRestoreRenderStates(saved);

	/* the fireworks */
	co_Timer_Start(timer, 0xff000080);
	bdSetRenderState(3, 0, 0);
	oldState = bdGetRenderState(1, NULL);
	bdSetRenderState(1, 1, 0);
	co_Fireworks_Draw(self->fireworks);
	bdSetRenderState(1, oldState, 0);
	co_Spawner_DrawEffects(self->spawner);
	if (self->glitter != NULL)
		co_Glitter_DrawEffects(self->glitter);
	if (self->obj02c != NULL)
		co_Obj2c_Draw(self->obj02c);
	co_Nothing();                                       /* the sky's empty draw */
	co_Timer_Start(timer, 0xffffff00);

	/* the HUD */
	bdSetRenderState(3, 0, 0);
	bdSetProjectionMode(0);
	bdSetRenderState(1, 1, 0);
	bdSetWorldMatrix(DAT_002855f0);
	if (DAT_002f6d60 != 0)
		CFire_Game_DrawDebug();
	for (i = 0; i < DAT_002bae94; i++)
	{
		self->scores[i] = CO_I32(g_coPlayers[i], 0x1148);
		DAT_002baed4[i][5] = self->scores[i];
		CO_F32(DAT_002ecfac, 0x4c + i * 4) = (float)CO_I32(g_coPlayers[i], 0x1148);
		if ((float)CO_I32(g_coPlayers[i], 0x1148) > CO_F32(DAT_002ecfac, 0x40))
			CO_F32(DAT_002ecfac, 0x40) = (float)CO_I32(g_coPlayers[i], 0x1148);
	}
	co_Hud_Draw(DAT_002ecfac);
	bdSetProjectionMode(1);
	bdSetTexture(0, NULL);
	bdSetWorldMatrix(DAT_002855f0);
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(3, 1, 0);
	bdSetFieldOfView(1.2217305f, 0.9162979f);
	bdSetWorldMatrix(DAT_002855f0);
	co_Sky_EnableLights(self->sky, 4, 1);
	if (DAT_00293a50 != 0)
		COverlay_DrawAll(1);
	if (self->glitter != NULL && DAT_00293a50 != 0)
		co_Glitter_DrawHud(self->glitter);
	CFire_Camera_DrawFade(g_fireCam, CO_F32(g_fireRes, 0xb4));
	co_Timer_Start(timer, 0xff00ff00);
	if (DAT_002f6b98 != 0)
	{
		bdSetRenderState(3, 0, 0);
		DAT_002f6c00 = bkTimerRead();
		delta = bkTimerDelta(DAT_002f6c08, DAT_002f6c00);
		DAT_002f6c08 = DAT_002f6c00;
		ms = bkTimerRate((long long)delta);
		co_Timer_Draw(timer);
		CFire_Resource_Printf(g_fireRes, -260.0f, 150.0f, 0xff00ff00, 2, "%2.2f", (double)ms);
	}
	co_Timer_Stop(timer, 1);
}

/* 0x000bd590  CarryOn/CFire_Game.cpp (file inferred)
In the intro and at the end: finds the (up to) four players nearest to the camera along
its view (players behind it or off screen count as far away; at the end the winner the
camera looks at counts as nearest) and tells each player (vtable +0x44) whether it is
one of them (0) or not (1). In the intro only the two nearest count. */
void CFire_Game_UpdateNearPlayers(struct CFireGame *self)
{
	float eye[4];
	float target[4];
	float delta[4];
	float dir[4];
	float depth;
	float best0 = 1000.0f, best1 = 1000.0f, best2 = 1000.0f, best3 = 1000.0f;
	int near0 = -1, near1 = -1, near2 = -1, near3 = -1;
	void *actor;
	int i;

	if (self->state == CO_STATE_INTRO || self->state == CO_STATE_END)
	{
		CFire_Camera_GetEye(g_fireCam, eye);
		CFire_Camera_GetTarget(g_fireCam, target);
		delta[0] = target[0] - eye[0];
		delta[1] = target[1] - eye[1];
		delta[2] = target[2] - eye[2];
		bmVectorSetLength(dir, delta, 1.0f);
		for (i = 0; i < 4; i++)
		{
			actor = CO_PTR(g_coPlayers[i], 0x30);
			depth = dir[0] * (CO_F32(actor, 0xd0) - eye[0]) + dir[1] * (CO_F32(actor, 0xd4) - eye[1])
				+ dir[2] * (CO_F32(actor, 0xd8) - eye[2]);
			if (depth < -100.0f)                            /* 0x00267a00 */
				depth = 1e7f;
			/* the radius of the model at the player's index in the model table */
			if (bdSphereVisible(CO_FP(actor, 0xd0),
					CO_F32(DAT_00293a68[CO_U16(actor, 0x230)].actor, 0x5c)) == 0)
				depth = 1e7f;                               /* 0x002679fc */
			if (self->state == CO_STATE_END && CO_PTR(g_fireCam, 0x44) == g_coPlayers[i])
				depth = -1000.0f;                           /* 0x002679f8 */
			if (depth < best0)
			{
				best3 = best2;
				best2 = best1;
				best1 = best0;
				near3 = near2;
				near2 = near1;
				near1 = near0;
				best0 = depth;
				near0 = i;
			}
			else if (depth < best1)
			{
				best3 = best2;
				best2 = best1;
				near3 = near2;
				near2 = near1;
				best1 = depth;
				near1 = i;
			}
			else if (depth < best2)
			{
				best3 = best2;
				near3 = near2;
				best2 = depth;
				near2 = i;
			}
			else if (depth < best3)
			{
				best3 = depth;
				near3 = i;
			}
		}
	}
	if (self->state == CO_STATE_INTRO)
	{
		near2 = -1;
		near3 = -1;
	}
	for (i = 0; i < 4; i++)
	{
		if (i == near0 || i == near1 || i == near2 || i == near3)
			CO_VFN(g_coPlayers[i], 0x44, void (*)(void *, int))(g_coPlayers[i], 0);
		else
			CO_VFN(g_coPlayers[i], 0x44, void (*)(void *, int))(g_coPlayers[i], 1);
	}
}

/* 0x000bd800  CarryOn/CFire_Game.cpp (vtable 0x00267468 slot 0)
The scalar deleting destructor. */
struct CFireGame *CFire_Game_DeletingDtor(struct CFireGame *self, unsigned int flags)
{
	self->vtable = vtable_00267468;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000bd820  CarryOn/CFire_Game.cpp
Creates the CarryOn game mode and sets the variant (1-based). charlib's
CModule_CarryOn_Start calls it. Returns 1, or 0 when the mode could not be made. */
int CFire_Game_SetVariant(struct CFireGame *self, int variant)
{
	void *mode;

	mode = bkHeapAlloc(0x38, CO_FILE_GAME, 0x1ba, 0x1001);
	mode = mode == NULL ? NULL : co_Mode_Ctor(mode);
	if (mode == NULL)
	{
		bkPrintf("NULL MODE\n");
		return 0;
	}
	bkPrintf("SETTING GAME MODE\n");
	self->mode = mode;
	self->flags ^= ((unsigned int)variant * 2 - 2 ^ self->flags) & 6;
	return 1;
}

/* 0x000bd8e0  CarryOn/CFire_Game.cpp (file inferred)
Ends the game (the carryon_b mode's exit): deletes everything InitOnce, InitObjects and
Start made, restores the display planes and the route grid, closes the packages. */
void CFire_Game_Shutdown(struct CFireGame *self)
{
	struct CFireASEImport ase;
	void *object;
	int *block;
	int i;

	baFlushShaderCache();
	object = DAT_002f26c4;
	DAT_0028ffa8 = self->savedNear;
	DAT_0028ffac = self->savedFar;
	if (DAT_002f26c4 != NULL)
	{
		co_DemoCam_Dtor(object);
		bkHeapFree(object);
	}
	Shadows_DeleteAll();
	CTweakMenu_SaveAll();
	CLRumble_Shutdown((int *)DAT_002ed324);
	if (DAT_002f6b88 != NULL)
	{
		block = (int *)DAT_002f6b88 - 1;
		for (i = *block - 1; i >= 0; i--)
			CLRouteHeights_Free2((struct CLRouteHeights *)((char *)DAT_002f6b88 + i * 0x18));
		bkHeapFree(block);
		DAT_002f6b88 = NULL;
	}
	if (self->projector != NULL)
	{
		baDeleteExtraSystem(self->projector[0]);
		bkHeapFree(self->projector);
		self->projector = NULL;
	}
	if ((object = self->sky) != NULL)
	{
		co_Sky_Delete(object);
		bkHeapFree(object);
		self->sky = NULL;
	}
	if ((object = self->tweaks) != NULL)
	{
		CTweakMenu_Dtor(object);
		bkHeapFree(object);
		self->tweaks = NULL;
	}
	if ((object = g_fireCam) != NULL)
	{
		CFire_Camera_Dtor(object);
		bkHeapFree(object);
		g_fireCam = NULL;
	}
	DAT_00292c60 = self->savedBounds[0];
	DAT_00292c64 = self->savedBounds[1];
	DAT_00292c68 = self->savedBounds[2];
	DAT_00292c6c = self->savedBounds[3];
	if (self->winnerShown != 0)
		CLHud_DeleteWinnerText((int)DAT_002ecfac);
	if ((object = self->obj02c) != NULL)
	{
		co_Obj2c_Delete(object);
		bkHeapFree(object);
		self->obj02c = NULL;
	}
	if (self->level != NULL)
	{
		CFire_World_Unload(self->level);
		if ((object = self->level) != NULL)
		{
			co_Nothing();                               /* the level's empty destructor */
			bkHeapFree(object);
		}
		self->level = NULL;
	}
	if (self->fireworks != NULL)
	{
		co_Fireworks_Delete(self->fireworks);
		bkHeapFree(self->fireworks);
		self->fireworks = NULL;
	}
	if (self->launcher != NULL)
	{
		co_Launcher_Delete(self->launcher);
		if (self->launcher != NULL)
			CO_VFN(self->launcher, 0x0, void (*)(void *, int))(self->launcher, 1);
		self->launcher = NULL;
	}
	for (i = 0; i < 2; i++)
	{
		if (self->conveyor[i] != NULL)
		{
			co_Nothing();                               /* empty: the belt's clean-up */
			if ((object = self->conveyor[i]) != NULL)
			{
				co_Nothing();                           /* empty: its destructor */
				bkHeapFree(object);
			}
			self->conveyor[i] = NULL;
		}
	}
	if (self->paint != NULL)
	{
		co_Paint_Delete(self->paint);
		bkHeapFree(self->paint);
		self->paint = NULL;
	}
	co_IntroCam_Delete();
	if (self->spawner != NULL)
	{
		co_Spawner_Delete(self->spawner);
		if ((object = self->spawner) != NULL)
		{
			co_Spawner_Dtor(object);
			bkHeapFree(object);
		}
		self->spawner = NULL;
	}
	CFire_ASEImport_Ctor(&ase);
	if (CO_VARIANT(self) == 1)
	{
		if ((object = self->glitter) != NULL)
		{
			co_Glitter_Delete(object);
			bkHeapFree(object);
			self->glitter = NULL;
		}
	}
	else if (CO_VARIANT(self) == 3)
	{
		CFire_ASEImport_FreePaths(&ase, self->fuses, self->numFuses);
		CFire_ASEImport_FreePaths(&ase, self->paintPaths, self->numPaintPaths);
		CFire_ASEImport_FreePaths(&ase, self->convey, self->numConvey);
	}
	CFire_ASEImport_Dtor(&ase);
	for (i = 0; i < DAT_002bae94; i++)
	{
		if (g_coPlayers[i] == NULL)
			continue;
		if (CO_PTR(g_coPlayers[i], 0x5c) != NULL)
		{
			biDeleteMap(CO_PTR(g_coPlayers[i], 0x5c));
			biDeleteMap(CO_PTR(g_coPlayers[i], 0x74));
		}
		if ((object = g_coPlayers[i]) != NULL)
		{
			CCarryOn_Player_Dtor(object);
			bkHeapFree(object);
		}
		g_coPlayers[i] = NULL;
		DAT_002f289c[i] = NULL;
	}
	if ((object = self->carry) != NULL)
	{
		CFire_Carry_Dtor(object);
		bkHeapFree(object);
		self->carry = NULL;
	}
	if ((object = g_fireRes) != NULL)
	{
		CFire_Resource_Dtor(object);
		bkHeapFree(object);
	}
	g_fireRes = NULL;
	CLDebris_Start();
	RefBot_Delete();
	CProjector_DeleteAll();
	co_CharLib_DeleteFx();
	co_CharLib_DeleteShadows();
	Actor_DeleteAll();
	if (DAT_002f6b8c != NULL)
	{
		bkDeleteResource(DAT_002f6b8c);
		DAT_002f6b8c = NULL;
	}
	if (DAT_002f6b94 != NULL)
	{
		bkPrintf("closing generic meta package\n");
		bkClosePackage(DAT_002f6b94);
		DAT_002f6b94 = NULL;
	}
	if (DAT_002f6b90 != NULL)
	{
		bkPrintf("closing meta level package\n");
		bkClosePackage(DAT_002f6b90);
		DAT_002f6b90 = NULL;
	}
	bkDeletePackageResources(DAT_002f6b80, 1);
	bkDeletePackageResources(DAT_002f6b84, 1);
	if (self->overlays != NULL)
	{
		COverlay_DeleteAll();
		if ((object = self->overlays) != NULL)
		{
			co_Nothing();                               /* the list's empty destructor */
			bkHeapFree(object);
		}
		self->overlays = NULL;
	}
}

/* 0x000bdd50  CarryOn/CFire_Game.cpp (file inferred)
One frame (the carryon_b mode's update, when `update` is set): the intro / countdown /
play / end states, the players, the variant, the clock, and at the end the scores and
the way back (the next tournament game or the previous mode). */
void CFire_Game_Update(struct CFireGame *self, int update)
{
	unsigned int saved[30];        /* 0x78 bytes in the original frame */
	int scores[4];
	void *timer;
	float dt;
	int best;
	int i;

	if (update == 0)
		return;
	CFire_Resource_UpdateDetail(g_fireRes);
	timer = CO_ADDR(g_fireRes, 0x18);
	co_Timer_Start(timer, 0xffffffff);
	if (DAT_002ecdec != 0)
	{
		CFire_Camera_Update(g_fireCam, 0.0f);
		co_PauseMenu_Update(DAT_002ecd90);
		return;
	}
	if (DAT_00283e58 != 0)
		CO_F32(g_fireRes, 0xb4) = CO_F32(g_fireRes, 0xb4) * 1.5f;    /* 0x002538a0 */
	if (DAT_00283ea4 != self->rumble)
	{
		self->rumble = DAT_00283ea4;
		for (i = 0; i < DAT_002bae94; i++)
			Core_SetCharacterColours(CO_ADDR(CO_PTR(g_coPlayers[i], 0x30), 0x10), DAT_00283ea4);
	}
	co_Timer_Mark(timer, 0);
	switch (self->state)
	{
	case CO_STATE_LOADING:
	case CO_STATE_LOADED:
		if (DAT_002baeb8 != 0)
		{
			self->state = CO_STATE_PLAY;
			goto players;
		}
		self->state = CO_STATE_INTRO;
		/* fall through */
	case CO_STATE_INTRO:
		if (CO_I32(g_fireCam, 0x4) != 0)
		{
			CLHud_StartBars(DAT_002ecfac, 280.0f);
			self->state = CO_STATE_COUNTDOWN;
		}
		break;
	case CO_STATE_COUNTDOWN:
		if (co_Hud_CountdownDone(DAT_002ecfac) != 0)
			self->state = CO_STATE_PLAY;
		break;
	case CO_STATE_PLAY:
		if (co_IntroCam_Active() == 0)
			self->timeLeft = self->timeLeft - CO_F32(g_fireRes, 0xb0);
		break;
	case CO_STATE_END:
		break;
	default:
		goto players;
	}
	CCarryOn_Player_UpdateControls(g_fireRes);
	Actor_UpdateAll();
players:
	for (i = 0; i < DAT_002bae94; i++)
		CCarryOn_Player_Update(g_coPlayers[i]);
	co_CharLib_UpdateShadows();
	CProjector_UpdateAll();
	co_CharLib_UpdateFx();
	FUN_000ba6d0();
	Actor_UpdateAnimations();
	co_UpdateExtras(NULL, DAT_0028fda8 * 0.016666668f);     /* 0x00262b7c */
	if (DAT_00293a50 != 0)
		COverlay_UpdateAll();
	co_IntroCam_Update();
	dt = CO_F32(g_fireRes, 0xb4);
	CFire_World_Update(self->level, dt);
	switch (CO_VARIANT(self))
	{
	case 0:
		CFire_Game_UpdatePanels(self, CO_F32(g_fireRes, 0xb4));
		break;
	case 1:
		co_Glitter_Update(self->glitter, CO_F32(g_fireRes, 0xb4));
		break;
	case 2:
		CFire_Game_UpdateRampTextures(self, CO_F32(g_fireRes, 0xb4));
		break;
	case 3:
		CFire_Game_UpdatePanels(self, CO_F32(g_fireRes, 0xb4));
		break;
	}

	/* the bars grow while the leader is past three quarters of them */
	best = 0;
	for (i = 0; i < CO_I32(g_fireRes, 0x254); i++)
		if (CO_I32(g_coPlayers[i], 0x1148) > best)
			best = CO_I32(g_coPlayers[i], 0x1148);
	if (self->barMax * 0.75f < (float)best)                 /* 0x00253844 */
		self->barMax = CO_F32(g_fireRes, 0xb4) * self->barGrow + self->barMax;
	CO_F32(self->projector, 0x4) = 1.5707964f - CFire_Camera_GetYaw(g_fireCam);  /* 0x00253898 */
	co_Fireworks_Update(self->fireworks);
	if (self->launcher != NULL)
		co_Launcher_Update(self->launcher, CO_F32(g_fireRes, 0xb4), CO_PTR(self->fireworks, 0x8));
	if (self->conveyor[0] != NULL)
		CFire_Belt_Update(self->conveyor[0], CO_F32(g_fireRes, 0xb4));
	if (self->conveyor[1] != NULL)
		CFire_Belt_Update(self->conveyor[1], CO_F32(g_fireRes, 0xb4));
	if (self->paint != NULL)
		co_Paint_Update(self->paint, CO_F32(g_fireRes, 0xb4), CO_PTR(self->fireworks, 0x0),
			CO_PTR(self->fireworks, 0x8), self->conveyor, 2);
	if (self->obj02c != NULL)
		co_Obj2c_Update(self->obj02c, CO_F32(g_fireRes, 0xb4));
	if (self->carry != NULL)
		CFire_Carry_Update(self->carry, CO_F32(g_fireRes, 0xb4));
	co_Spawner_Update(self->spawner, CO_F32(g_fireRes, 0xb4), self->state);
	co_Sky_Update(self->sky, CO_F32(g_fireRes, 0xb4));
	CFire_Game_UpdateAnimTextures(self);

	/* the time is up */
	if (self->timeLeft <= 0.0f)
	{
		for (i = 0; i < DAT_002bae94; i++)
			DAT_002baed4[i][5] = CO_I32(g_coPlayers[i], 0x1148);
		if (self->ended < 2)
		{
			self->ended = 2;
			CFire_Camera_StartEnd(g_fireCam);
			scores[0] = DAT_002baee8[0];
			scores[1] = DAT_002baf10;
			scores[2] = DAT_002baf38;
			scores[3] = DAT_002baf60;
			co_Hud_ShowWinner(DAT_002ecfac, scores, 0);
			self->winnerShown = 1;
			self->state = CO_STATE_END;
			self->endTime = 0.0f;
		}
		else
		{
			self->endTime = CO_F32(g_fireRes, 0xb0) + self->endTime;
			if (CO_I32(g_fireCam, 0x4) == 5)
			{
				if (DAT_002bae98 != 2)
				{
					CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, 1);
					return;
				}
				CGameModeStack_PopTo((struct CGameModeStack *)DAT_002ba750, s_BGeffects_00283b18, 1);
				CO_VFN(DAT_002ba95c, 0xc, void (*)(void *, int))(DAT_002ba95c, CO_VARIANT(self) + 1);
				return;
			}
		}
		self->timeLeft = 0.0f;
	}
	bdSaveRenderStates(saved);
	RefBot_Update();
	bdRestoreRenderStates(saved);
	co_Debris_SetPlayers(CO_FP(CO_PTR(g_coPlayers[0], 0x30), 0xd0),
		CO_FP(CO_PTR(g_coPlayers[1], 0x30), 0xd0), CO_FP(CO_PTR(g_coPlayers[2], 0x30), 0xd0),
		CO_FP(CO_PTR(g_coPlayers[3], 0x30), 0xd0));
	self->frame = self->frame + 1;
	if (co_IntroCam_Active() == 0)
	{
		if (DAT_002f26c4 == NULL || self->state != CO_STATE_PLAY)
			CFire_Camera_Update(g_fireCam, CO_F32(g_fireRes, 0xb4));
		else
		{
			DAT_00293a50 = 0;
			co_DemoCam_Update(DAT_002f26c4, CO_F32(g_fireRes, 0xb4));
		}
	}
	CFire_Game_UpdateNearPlayers(self);
	bdSetFieldOfView(1.2217305f, 0.9162979f);
	bdSetWorldMatrix(DAT_002855f0);
	bdSetProjectionMode(1);
	CLHud_SetClock((int)DAT_002ecfac, self->timeLeft * 0.001f, 1);  /* 0x0025767c */
	bdSetProjectionMode(0);
	CLHud_SetScores((int)DAT_002ecfac, self->scores);
	co_Timer_Stop(timer, 0);
	if (CFire_Camera_IsReady(g_fireCam) == 0 || co_PauseMenu_Update(DAT_002ecd90) != 1)
		co_Timer_Start(timer, 0xffffffff);
}
