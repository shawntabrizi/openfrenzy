/*
FoxHunt/CModule_FoxHunt.c  <-  C:\BlitzParty\Game\Src\FoxHunt\CModule_FoxHunt.cpp
(module foxhunt)

The Tag entry of core's game list (a CGame, vtable 0x0026cc48): name "Tag", variations
"Cooperation", "Energy Ball", "Odd One Out" and "Victim". Start makes the route tables the
CharLib AI players share, pushes the game mode (CGameMode_FoxHunt) and the minigame intro.

The first two functions of the range belong to the Drillers code before it (they use the
Drillers data); the function table gives them to this module.
*/

#include "foxhunt.h"
#include "drillers_data.h"

const char fh_ModuleFile[] = "C:\\BlitzParty\\Game\\Src\\FoxHunt\\CModule_FoxHunt.cpp";

/* 0x0026cc44  vtable of a Drillers object: [0] is its scalar deleting destructor
0x00125070; the words after it are the CModule_FoxHunt vtable (0x0026cc48) */
const void *const vtable_0026cc44[5] = {
	(const void *)FUN_00125070, /* [0] scalar deleting destructor */
	(const void *)FUN_001253a0,
	(const void *)FUN_000a6fa0,
	(const void *)FUN_001d5950,
	(const void *)FUN_001253c0,
};

/* 0x0026cc48  CModule_FoxHunt vtable (the CGame virtuals) */
const void *const vtable_0026cc48[4] = {
	(const void *)FUN_001253a0, /* [0] scalar deleting destructor */
	(const void *)FUN_000a6fa0, /* [1] init (returns 1) */
	(const void *)FUN_001d5950, /* [2] shutdown (empty) */
	(const void *)FUN_001253c0, /* [3] start (variation) */
};

/* 0x00125070  (Drillers object, vtable 0x0026cc44): scalar deleting destructor */
unsigned int *FUN_00125070(unsigned int *self, unsigned char flags)
{
	self[0] = FH_WORD(vtable_0026cc44);
	if ((flags & 1) != 0)
		FH_DELETE(self);
	return self;
}

/* 0x00125090  (Drillers helper, stdcall): tests the segment of player `player`'s path
(Drillers path points 0 and 2) against every segment of the other players' paths (and of
its own path from point 4 on). On a crossing: in variant 4 (Quantum Snare) the player's own
loop is closed (0x00116be0, 0x00124c00); in variant 3 (TailBlazer) the crossed player is
knocked out with a sound, a flash (0x00112de0) and a debris item. */
void FUN_00125090(int player)
{
	int p = (int)(signed char)player;
	int base = p * 0x730;
	float segA[4], segB[4];
	unsigned char hit[0x20];
	int k, i;

	segA[0] = DR_VAR(float, 0x00298edc + base) * 0.1f;
	segA[1] = DR_VAR(float, 0x002990dc + base) * 0.1f;
	segA[2] = DR_VAR(float, 0x00298ee4 + base) * 0.1f;
	segA[3] = DR_VAR(float, 0x002990e4 + base) * 0.1f;
	for (k = 0; k < 4; k++) {
		unsigned int rec = 0x00298ed4u + (unsigned int)k * 0x730;
		if (DR_VAR(unsigned char, rec - 0x44) != 1)
			continue;
		for (i = 1; i < DR_VAR(short, rec) - 2; i++) {
			unsigned int idx;

			if (k == p && i < 4)
				continue;
			idx = ((unsigned int)k * 0x1cc + (unsigned int)i) * 4;
			segB[0] = DR_VAR(float, 0x00298edc + idx) * 0.1f;
			segB[1] = DR_VAR(float, 0x002990dc + idx) * 0.1f;
			segB[2] = DR_VAR(float, 0x00298ee0 + idx) * 0.1f;
			segB[3] = DR_VAR(float, 0x002990e0 + idx) * 0.1f;
			if (segB[0] == segB[2] && segB[1] == segB[3])
				continue;
			if (bmSegmentSegment(segA, segB, (struct BHit2D *)hit) == 0)
				continue;
			if (i < DR_VAR(short, rec)) {
				if (k != p)
					goto crossed;
				if (DR_VAR(int, 0x002ff9fc) == 4) {
					DR_VAR(short, 0x00298ed4 + base) = 4;
					DR_VAR(int, 0x00299590 + base) = 0;
					FUN_00116be0((void *)(uintptr_t)DR_ADDR(0x00298e70), i, p);
				}
			}
			if (k == p && DR_VAR(int, 0x002ff9fc) == 4) {
				DR_VAR(int, 0x002994e0 + base) = 0;
				FUN_00124c00((undefined4 *)DR_ADDR(0x002aae60), (undefined4)player, i);
				continue;
			}
crossed:
			if (DR_VAR(int, 0x002ff9fc) == 3 && DR_VAR(int, 0x002ffa7c) == 0) {
				float scale = (float)DR_VAR(int, 0x002ff9e0);
				float x = (DR_VAR(float, 0x00298e74 + base) * 0.0009765625f - (float)DR_VAR(int, 0x002ff9e8)) * scale;
				float z = (DR_VAR(float, 0x00298e78 + base) * 0.0009765625f - (float)DR_VAR(int, 0x002ff9e4)) * scale;
				float pos[4];

				DR_VAR(unsigned char, 0x00298e90 + base) = 0;
				DR_VAR(float, 0x00299568 + base) = 255.0f;
				pos[0] = x;
				pos[1] = scale;
				pos[2] = z;
				pos[3] = 1.0f;
				bsPlaySample(DR_VAR(struct BSample *, 0x002aa9c8), 200, 0, 0, 0);
				FUN_00112de0((struct DrFxGroup *)(uintptr_t)DR_ADDR(0x00301578 + (unsigned int)p * 0x10c), (float *)pos, p);
				baEnableLight(DR_VAR(struct BLight *, 0x002aa994 + (unsigned int)p * 4), 0);
				if (DR_VAR(int, 0x002ff9fc) == 3)
					baAddExtraItem(DR_VAR(struct BExtraSystem *, 0x003014b8), (double)x,
						(double)((float)p + 15.0f), (double)z);
			}
		}
	}
}

/* 0x00125360  CModule_FoxHunt::CModule_FoxHunt */
unsigned int *CModule_FoxHunt_Ctor(unsigned int *self)
{
	CGame_Ctor6(self, "Tag", (unsigned int)(uintptr_t)"Cooperation", (unsigned int)(uintptr_t)"Energy Ball",
		(unsigned int)(uintptr_t)"Odd One Out", (unsigned int)(uintptr_t)"Victim");
	self[0] = FH_WORD(vtable_0026cc48);
	return self;
}

/* 0x00125390  CModule_FoxHunt::~CModule_FoxHunt */
void CModule_FoxHunt_Dtor(unsigned int *self)
{
	self[0] = FH_WORD(vtable_0026cc48);
	CGame_Dtor(self);
}

/* 0x001253a0  CModule_FoxHunt.cpp: scalar deleting destructor */
unsigned int *CModule_FoxHunt_Destroy(unsigned int *self, unsigned char flags)
{
	CModule_FoxHunt_Dtor(self);
	if ((flags & 1) != 0)
		FH_DELETE(self);
	return self;
}

/* 0x000ad830  CharLib/Route.cpp: constructor of one 0x18-byte route height table (two
arrays over the route grid). Not in the function table: only the "eh vector constructor
iterator" call reaches it; a local copy (charlib owns the original). */
static void *fh_RouteTable_Ctor(void *self)
{
	FH_AT(self, 0x0, void *) = bkHeapAlloc(DAT_00292c78 * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23d, 0x2001);
	FH_AT(self, 0x4, void *) = bkHeapAlloc(DAT_00292c78, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23e, 0x2001);
	return self;
}

/* 0x001253c0  CModule_FoxHunt.cpp: start. Makes the route tables (one per route cell),
pushes the game mode for `variation` and gives the tables to the shared route object
0x003021b8, then pushes the minigame intro ("tag<variation>.bmp"). Returns 1 when the game
mode was pushed. */
int CModule_FoxHunt_Start(unsigned int *self, int variation)
{
	char picture[64];
	int count = DAT_00292c78;
	int *block;
	char *tables;
	char *mode;
	void *intro;
	int i;

	/* new CLRouteTable[count] (eh vector constructor iterator; destructor 0x000ad880) */
	block = (int *)FH_NEW(count * 0x18 + 4, fh_ModuleFile, 0x5e);
	tables = NULL;
	if (block != NULL) {
		tables = (char *)(block + 1);
		*block = count;
		for (i = 0; i < count; i++)
			fh_RouteTable_Ctor(tables + i * 0x18);
	}
	fh_RouteTables = tables;
	mode = (char *)FH_NEW(0x280, fh_ModuleFile, 0x60);
	if (mode != NULL)
		mode = (char *)CGameMode_FoxHunt_Ctor(mode);
	if (mode == NULL)
		return 0;
	FH_AI(mode, 0x180) = variation;
	FH_AI(self, 0x18) = variation;
	FH_AI(self, 0x1c) = 1;
	FUN_000c39b0((undefined4)(uintptr_t)DAT_003021b8, (undefined4)(uintptr_t)tables, 0x18, 0x18);
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode, 1) == 0)
		return 0;
	_snprintf(picture, 0x3f, "tag%d.bmp", variation);
	intro = FH_NEW(0x300, fh_ModuleFile, 0x71);
	if (intro != NULL)
		intro = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)intro, variation + 0x3f, picture);
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)intro, 1) == 0)
		bkPrintf("intro hasnt worked apparantley\n");
	return 1;
}
