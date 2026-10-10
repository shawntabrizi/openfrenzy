/*
CMETAPLAYER.C  <-  C:\BlitzParty\Game\Src\Meta\CMetaPlayer.cpp

Only 0x00158050 carries this file's path; the rest of CMetaPlayer is in the gap files
unplaced_metaplayer.c and unplaced_ai.c (layout: see unplaced_metaplayer.c).
*/

#include "meta_a.h"

/* the tile type table (0x002b0930, 6 x 0x10 bytes: name, two words, frame count) */
#define MA_TILE_NAME(type) (*(const char **)MA_META_ADDR(0x002b0930u + (type) * 0x10))

/* load the carried-orb model `name` (frame 00_00) and hold it with a beam to the camera */
static int ma_hold_orb(int ai, const char *name, float scale)
{
	char path[64];
	int orb;

	sprintf(path, "%s00_00.obe", name);
	orb = (int)Actor_LoadAny(path, (float *)DAT_002ea478, 10, NULL, 0);
	MA_F32(orb, 0xe0) = scale;
	MA_F32(orb, 0xe4) = scale;
	MA_F32(orb, 0xe8) = scale;
	MA_PTR(ai, 0x1148) = CRay_CreateOrbRayControl(1, MA_FP(orb, 0x40), MA_FP(DAT_003055d4, 0x534),
		(void *)FUN_000b4bd0);
	AI_Hold((void *)ai, (void *)orb, 0);
	return orb;
}

/* 0x00158050  Meta/CMetaPlayer.cpp: collect `count` orbs (4 at most; the fourth makes
   the super orb) */
void FUN_00158050(int ai, int count)
{
	int orb;
	void *memory;

	if (MA_I32(ai, 0x1140) >= 4)
		return;
	if (MA_I32(ai, 0x1140) == 0 && count < 4)
		ma_hold_orb(ai, MA_TILE_NAME(2), 0.25f); /* "gold_tile" (0x002b0950) */
	count = count + MA_I32(ai, 0x1140);
	if (4 < count)
		count = 4;
	MA_I32(ai, 0x1140) = count;
	if (count == 4) {
		if (MA_I32(ai, 0x10bc) != 0 && MA_I32(MA_PTR(ai, 0x10bc), 0x21c) == 10) {
			/* (the original frees the beam without clearing +0x1148; it is replaced below) */
			if (MA_I32(ai, 0x1148) != 0) {
				CRayControl_Destroy((struct CRayControl *)MA_PTR(ai, 0x1148));
				MA_DELETE(MA_PTR(ai, 0x1148));
			}
			Actor_Delete(MA_PTR(ai, 0x10bc));
			MA_I32(ai, 0x10bc) = 0;
		}
		orb = ma_hold_orb(ai, MA_TILE_NAME(3), 0.4f); /* "colour_tile_super" (0x002b0960) */
		memory = MA_NEW(0x18, "C:\\BlitzParty\\Game\\Src\\Meta\\CMetaPlayer.cpp", 0x5d7, 0x1001);
		if (memory != NULL)
			AnimTex_Init((struct CAnimTex *)memory, "smokey1.gif", 16.666666f, (struct CLFList *)(orb + 0x328));
		MA_F32(orb, 0x334) = 1.0f;
	}
	CRayControl_SetScore((struct CRayControl *)MA_PTR(ai, 0x1148), MA_I32(ai, 0x1140));
}
