/*
TankedUp/CTank_Debug.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Debug.cpp

CTank_Debug: the one debug object of TankedUp (0x003150b0, 0x158 bytes, in tankedup_a's
data block). It holds the tweakable settings of the game and their tweak menus. 0x001d6d90
and 0x001d6f10 are tagged with CTank_Debug.cpp; the small methods before them and the free
0x001d79d0 work on the same object (file inferred).

Layout (offset: tweak name, default):
  +0x000 "Respawning inv" 2.0     +0x004 "MoveAng" 60         +0x008 "Smoke Speed" 5
  +0x00c "Router Debug" -1        +0x010 "Col Rings" 0        +0x014 the variation
  +0x018 "Hard skip intro" 1      +0x01c "OverLay" 1          +0x020-0x028 "AmbR/G/B"
  +0x02c-0x034 "lightPlR/G/B"     +0x038 "lightPlDist" 2000   +0x03c "AutoFireAng" 3
  +0x040 "AutoFireDist"           +0x044 "target Rand Dist"   +0x048 "Vindictiveness"
  +0x04c "Move on target"         +0x050 "Boost speedup" 0.5  +0x054 "Boost time" 3
  +0x058 a struct TuTimer (tankedup_a, 0x44 bytes)
  +0x0a0 "controls" 2             +0x0a4 "tankStopingSpeed"   +0x0a8 "accleration" 0.05
  +0x0ac "camLookDown" 900        +0x0b0 AI "speed"           +0x0b4 "token Get Dist" 2000
  +0x0b8 "rotCost" 3000           +0x0bc "Fire up Ang" 25     +0x0c0 "Fire up dist" 0.5
  +0x0c4 "Goal Size" 1            +0x0c8 "CamLookAhead" 500   +0x0cc "Dir Lights" 1
  +0x0d0-0x0ec "RenderWorld", "RenderBoom", "RenderEntity", "RenderOverLay", "RenderChar",
         "RenderCharShield", "RenderDebris", "RenderDecals"    +0x0f0 1 (no menu entry)
  +0x0f4 "Always Draw Chars"      +0x0f8 "Light World"        +0x0fc "Show Frame Rate"
  +0x100 menu "Camera"            +0x104 menu "Entities"      +0x108 menu "World"
  +0x10c-0x114 "Local RF": MoveAng 45, LookAng 15, SideSpace 0
  +0x118-0x12c AI: time 200, up 100, down 0, type 4, left 1, right 1
  +0x130 menu "AI Menu" (2)       +0x134 menu "Debug"         +0x138 menu "MyMenu"
  +0x13c menu "AI Menu"           +0x140 menu "Lights"        +0x144 menu "Render"
  +0x148 menu "Local RF"          +0x14c input map            +0x150 "detail level" channel
  +0x154 the detail level
*/

#include "tankedup_b.h"
#include "tankedup_b_ext.h"

#define DBG_TIMER 0x58
#define DBG_MENU_CAMERA 0x100
#define DBG_MENU_ENTITIES 0x104
#define DBG_MENU_WORLD 0x108
#define DBG_MENU_AI2 0x130
#define DBG_MENU_DEBUG 0x134
#define DBG_MENU_MINE 0x138
#define DBG_MENU_AI 0x13c
#define DBG_MENU_LIGHTS 0x140
#define DBG_MENU_RENDER 0x144
#define DBG_MENU_LOCALRF 0x148
#define DBG_INPUT_MAP 0x14c
#define DBG_INPUT_DETAIL 0x150
#define DBG_DETAIL 0x154

#define TU_MENU(self, off) ((struct CTweakMenu *)TU_PTR(self, off))

/* a new tweak sub-menu of `parent` (0xe8 bytes) or NULL */
static void *tu_new_submenu(void *parent, const char *name, int line)
{
	void *p = bkHeapAlloc(0xe8, TU_FILE_DEBUG, line, 0x1001);

	return p != NULL ? CTweakMenu_CtorSub((struct CTweakMenu *)p, (struct CTweakMenu *)parent, name)
		: NULL;
}

/* a new top-level tweak menu (0xe8 bytes) or NULL */
static void *tu_new_menu(const char *name, int line)
{
	void *p = bkHeapAlloc(0xe8, TU_FILE_DEBUG, line, 0x1001);

	return p != NULL ? CTweakMenu_Ctor((struct CTweakMenu *)p, name) : NULL;
}

static void tu_delete_menu(void *self, int off)
{
	void *menu = TU_PTR(self, off);

	if (menu != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)menu);
		bkHeapFree(menu);
	}
}

/* 0x001d6cb0  TankedUp (gap; CTank_Debug.cpp inferred)
CTank_Debug constructor. */
void *TankDebug_Ctor(void *self)
{
	TA_TimerCtor(TU_ADDR(self, DBG_TIMER));
	TU_I32(self, DBG_DETAIL) = 0;
	return self;
}

/* 0x001d6cd0  TankedUp (gap; CTank_Debug.cpp inferred)
CTank_Debug destructor: the timer's destructor, the empty function 0x001d5950 (a jump). */
void TankDebug_Dtor(void *self)
{
	(void)self;
	FUN_001d5950();
}

/* 0x001d6ce0  TankedUp (gap; CTank_Debug.cpp inferred)
CTank_Debug::CreateInput: the "detail level" button (input 0x70, and 0xcc on player 1's
pad). */
void TankDebug_CreateInput(void *self)
{
	struct BInputMap *map = biCreateMap(1);

	TU_PTR(self, DBG_INPUT_MAP) = map;
	TU_PTR(self, DBG_INPUT_DETAIL) = biCreateChannel((struct biMap *)map, "detail level", 1, 1, 0, 0, 0);
	biBindInput((struct biChannel *)TU_PTR(self, DBG_INPUT_DETAIL), 0x70, 0);
	biBindInput((struct biChannel *)TU_PTR(self, DBG_INPUT_DETAIL), (unsigned int)DAT_002baee0[0] | 0xcc,
		0);
}

/* 0x001d6d40  TankedUp (gap; CTank_Debug.cpp inferred)
CTank_Debug::ReleaseInput. */
void TankDebug_ReleaseInput(void *self)
{
	if (TU_PTR(self, DBG_INPUT_DETAIL) != NULL)
		biDeleteChannel((struct biChannel *)TU_PTR(self, DBG_INPUT_DETAIL));
	TU_PTR(self, DBG_INPUT_DETAIL) = NULL;
	if (TU_PTR(self, DBG_INPUT_MAP) != NULL)
		biDeleteMap((struct biMap *)TU_PTR(self, DBG_INPUT_MAP));
	TU_PTR(self, DBG_INPUT_MAP) = NULL;
}

/* 0x001d6d80  TankedUp (gap; CTank_Debug.cpp inferred)
CTank_Debug::GetDetail. */
int TankDebug_GetDetail(void *self)
{
	return TU_I32(self, DBG_DETAIL);
}

/* 0x001d6d90  TankedUp/CTank_Debug.cpp
CTank_Debug::Initialise: the top-level tweak menus "Camera", "Debug", "World" and
"Entities", and "CamLookAhead" (500). */
int TankDebug_Initialise(void *self)
{
	TU_PTR(self, DBG_MENU_CAMERA) = tu_new_menu("Camera", 0x2f);
	TU_PTR(self, DBG_MENU_DEBUG) = tu_new_menu("Debug", 0x30);
	TU_PTR(self, DBG_MENU_WORLD) = tu_new_menu("World", 0x31);
	TU_PTR(self, DBG_MENU_ENTITIES) = tu_new_menu("Entities", 0x32);
	TU_F32(self, 0xc8) = 500.0f;
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_CAMERA), "CamLookAhead", TU_FP(self, 0xc8), 100.0f, 0.0f,
		10000.0f);
	TU_I32(self, DBG_DETAIL) = 0;
	return 1;
}

/* 0x001d6f10  TankedUp/CTank_Debug.cpp
CTank_Debug::Setup: the default settings (the AI's by difficulty DAT_002bae70 and variation
+0x14) and their tweak menus. */
void TankDebug_Setup(void *self)
{
	struct CTweakMenu *menu;
	int variation;

	CTankTimer_Initialise((struct TuTimer *)TU_ADDR(self, DBG_TIMER), 0);
	TU_F32(self, 0x04) = 60.0f;
	TU_F32(self, 0x08) = 5.0f;
	TU_I32(self, 0x0c) = -1;
	TU_I32(self, 0x1c) = 1;
	TU_I32(self, 0xa0) = 2;
	TU_F32(self, 0xa4) = 0.5f;
	TU_I32(self, 0x18) = 1;
	TU_F32(self, 0x50) = 0.5f;
	TU_F32(self, 0x54) = 3.0f;
	TU_F32(self, 0xa8) = 0.0500000007f;
	TU_F32(self, 0xac) = 900.0f;
	TU_I32(self, 0xc4) = 1;
	TU_F32(self, 0xbc) = 25.0f;
	TU_F32(self, 0xc0) = 0.5f;
	TU_F32(self, 0x00) = 2.0f;
	TU_I32(self, 0x10) = 0;
	TU_I32(self, 0xfc) = 0;
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_DEBUG), "Show Frame Rate", &TU_I32(self, 0xfc));

	TU_PTR(self, DBG_MENU_MINE) = tu_new_submenu(TU_PTR(self, DBG_MENU_DEBUG), "MyMenu", 0x6a);
	menu = TU_MENU(self, DBG_MENU_MINE);
	CTweakMenu_AddBool(menu, "Col Rings", &TU_I32(self, 0x10));
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "Respawning inv", TU_FP(self, 0x00), 0.1f, 0.0f,
		10.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "Fire up Ang", TU_FP(self, 0xbc), 1.0f, 0.0f, 180.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "Fire up dist", TU_FP(self, 0xc0), 0.1f, 1.0f, 5.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "MoveAng", TU_FP(self, 0x04), 1.0f, 0.0f, 180.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "Smoke Speed", TU_FP(self, 0x08), 1.0f, 0.0f, 100.0f);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_MINE), "Router Debug", &TU_I32(self, 0x0c), 1, -1, 4, 0);
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_MINE), "OverLay", &TU_I32(self, 0x1c));
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_MINE), "controls", &TU_I32(self, 0xa0), 1, 1, 3, 0);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "tankStopingSpeed", TU_FP(self, 0xa4), 0.01f, 0.0f,
		1.0f);
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_MINE), "Hard skip intro", &TU_I32(self, 0x18));
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "Boost speedup", TU_FP(self, 0x50), 0.05f, 0.0f,
		1.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "Boost time", TU_FP(self, 0x54), 0.5f, 0.5f, 20.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "accleration", TU_FP(self, 0xa8), 0.01f, 0.01f,
		1.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_MINE), "camLookDown", TU_FP(self, 0xac), 10.0f, -100.0f,
		5000.0f);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_MINE), "Goal Size", &TU_I32(self, 0xc4), 1, 1, 5, 0);

	TU_I32(self, 0xd0) = 1;
	TU_I32(self, 0xd4) = 1;
	TU_I32(self, 0xd8) = 1;
	TU_I32(self, 0xdc) = 1;
	TU_I32(self, 0xe0) = 1;
	TU_I32(self, 0xe4) = 1;
	TU_I32(self, 0xe8) = 1;
	TU_I32(self, 0xec) = 1;
	TU_I32(self, 0xf0) = 1;
	TU_I32(self, 0xf4) = 0;
	TU_I32(self, 0xf8) = 0;
	TU_PTR(self, DBG_MENU_RENDER) = tu_new_submenu(TU_PTR(self, DBG_MENU_DEBUG), "Render", 0x8b);
	menu = TU_MENU(self, DBG_MENU_RENDER);
	CTweakMenu_AddBool(menu, "RenderWorld", &TU_I32(self, 0xd0));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "RenderBoom", &TU_I32(self, 0xd4));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "RenderEntity", &TU_I32(self, 0xd8));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "RenderOverLay", &TU_I32(self, 0xdc));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "RenderChar", &TU_I32(self, 0xe0));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "RenderCharShield", &TU_I32(self, 0xe4));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "RenderDebris", &TU_I32(self, 0xe8));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "RenderDecals", &TU_I32(self, 0xec));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "Light World", &TU_I32(self, 0xf8));
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_RENDER), "Always Draw Chars", &TU_I32(self, 0xf4));

	/* the AI by difficulty (0 easy, 1 medium, 2 hard) */
	if (DAT_002bae70 == 0) {
		TU_DEBUG("GAME DIFFICULTY = EASY\n");
		variation = TU_I32(self, 0x14);
		TU_F32(self, 0x3c) = 3.0f;
		TU_F32(self, 0x40) = 500.0f;
		TU_F32(self, 0x48) = 0.0f;
		TU_F32(self, 0x44) = 1000.0f;
		switch (variation) {
		case 1:
			TU_U32(self, 0xb0) = 0x3f333333; /* 0.7 */
			break;
		case 2:
		case 3:
			TU_U32(self, 0xb0) = 0x3f666666; /* 0.9 */
			break;
		case 4:
			TU_U32(self, 0xb0) = 0x3f75c28f; /* 0.96 */
			break;
		}
		TU_F32(self, 0x4c) = variation == 4 ? 40.0f : 1.0f;
	} else if (DAT_002bae70 == 1) {
		TU_DEBUG("GAME DIFFICULTY = MEDIUM\n");
		TU_F32(self, 0x40) = 1000.0f;
		TU_F32(self, 0x44) = 1000.0f;
		TU_F32(self, 0x3c) = 3.0f;
		TU_F32(self, 0x48) = 0.0f;
		TU_F32(self, 0xb0) = 1.0f;
		TU_F32(self, 0x4c) = TU_I32(self, 0x14) == 4 ? 40.0f : 1.0f;
	} else if (DAT_002bae70 == 2) {
		TU_DEBUG("GAME DIFFICULTY = HARD\n");
		variation = TU_I32(self, 0x14);
		TU_F32(self, 0x3c) = 3.0f;
		TU_F32(self, 0x40) = 1500.0f;
		TU_F32(self, 0x48) = 0.0f;
		TU_F32(self, 0x44) = 100.0f;
		TU_U32(self, 0xb0) = 0x3f8ccccd; /* 1.1 */
		TU_F32(self, 0x4c) = variation == 4 ? 40.0f : 1.0f;
	}
	TU_F32(self, 0xb4) = 2000.0f;
	TU_F32(self, 0xb8) = 3000.0f;
	TU_I32(self, 0x20) = 0x46;
	TU_I32(self, 0x24) = 0x46;
	TU_I32(self, 0x28) = 0x28;
	TU_I32(self, 0x2c) = 0x6e;
	TU_I32(self, 0x30) = 0x78;
	TU_I32(self, 0x34) = 0x78;
	TU_F32(self, 0x38) = 2000.0f;
	TU_I32(self, 0xcc) = 1;

	TU_PTR(self, DBG_MENU_LIGHTS) = tu_new_submenu(TU_PTR(self, DBG_MENU_DEBUG), "Lights", 0xd2);
	menu = TU_MENU(self, DBG_MENU_LIGHTS);
	CTweakMenu_AddBool(menu, "Dir Lights", &TU_I32(self, 0xcc));
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_LIGHTS), "lightPlR", &TU_I32(self, 0x2c), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_LIGHTS), "lightPlG", &TU_I32(self, 0x30), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_LIGHTS), "lightPlB", &TU_I32(self, 0x34), 1, 0, 0xff, 0);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_LIGHTS), "lightPlDist", TU_FP(self, 0x38), 100.0f, 1.0f,
		10000.0f);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_LIGHTS), "AmbR", &TU_I32(self, 0x20), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_LIGHTS), "AmbG", &TU_I32(self, 0x24), 1, 0, 0xff, 0);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_LIGHTS), "AmbB", &TU_I32(self, 0x28), 1, 0, 0xff, 0);

	TU_PTR(self, DBG_MENU_AI) = tu_new_submenu(TU_PTR(self, DBG_MENU_DEBUG), "AI Menu", 0xdc);
	menu = TU_MENU(self, DBG_MENU_AI);
	CTweakMenu_AddFloat(menu, "target Rand Dist", TU_FP(self, 0x44), 100.0f, 0.0f, 10000.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_AI), "AutoFireAng", TU_FP(self, 0x3c), 0.1f, 0.0f, 90.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_AI), "AutoFireDist", TU_FP(self, 0x40), 1.0f, 100.0f,
		10000.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_AI), "Vindictiveness", TU_FP(self, 0x48), 0.01f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_AI), "Move on target", TU_FP(self, 0x4c), 0.1f, 0.0f, 90.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_AI), "speed", TU_FP(self, 0xb0), 0.01f, 0.2f, 1.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_AI), "token Get Dist", TU_FP(self, 0xb4), 10.0f, 0.0f,
		10000.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_AI), "rotCost", TU_FP(self, 0xb8), 10.0f, 0.0f, 10000.0f);

	TU_F32(self, 0x10c) = 45.0f;
	TU_F32(self, 0x110) = 15.0f;
	TU_F32(self, 0x114) = 0.0f;
	TU_PTR(self, DBG_MENU_LOCALRF) = tu_new_submenu(TU_PTR(self, DBG_MENU_DEBUG), "Local RF", 0xee);
	menu = TU_MENU(self, DBG_MENU_LOCALRF);
	CTweakMenu_AddFloat(menu, "MoveAng", TU_FP(self, 0x10c), 1.0f, 0.0f, 90.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_LOCALRF), "LookAng", TU_FP(self, 0x110), 1.0f, 0.0f, 90.0f);
	CTweakMenu_AddFloat(TU_MENU(self, DBG_MENU_LOCALRF), "SideSpace", TU_FP(self, 0x114), 1.0f, -100.0f,
		1000.0f);

	TU_I32(self, 0x118) = 200;
	TU_I32(self, 0x11c) = 100;
	TU_I32(self, 0x120) = 0;
	TU_I32(self, 0x124) = 4;
	TU_I32(self, 0x128) = 1;
	TU_I32(self, 0x12c) = 1;
	TU_PTR(self, DBG_MENU_AI2) = tu_new_submenu(TU_PTR(self, DBG_MENU_DEBUG), "AI Menu", 0xfb);
	menu = TU_MENU(self, DBG_MENU_AI2);
	CTweakMenu_AddInt(menu, "time", &TU_I32(self, 0x118), 10, 1, 10000, 0); /* 0x0027b9b4 */
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_AI2), "up", &TU_I32(self, 0x11c), 1, 0, 100, 0);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_AI2), "down", &TU_I32(self, 0x120), 1, 0, 100, 0);
	CTweakMenu_AddInt(TU_MENU(self, DBG_MENU_AI2), "type", &TU_I32(self, 0x124), 1, 1, 5, 0);
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_AI2), "left", &TU_I32(self, 0x128)); /* 0x00274bb0 */
	CTweakMenu_AddBool(TU_MENU(self, DBG_MENU_AI2), "right", &TU_I32(self, 0x12c));
}

/* 0x001d79d0  TankedUp (gap; CTank_Debug.cpp inferred)
CTank_Debug::Free: the tweak menus and the input. */
void TankDebug_Free(void *self)
{
	tu_delete_menu(self, DBG_MENU_MINE);
	tu_delete_menu(self, DBG_MENU_AI);
	tu_delete_menu(self, DBG_MENU_LIGHTS);
	tu_delete_menu(self, DBG_MENU_RENDER);
	tu_delete_menu(self, DBG_MENU_LOCALRF);
	tu_delete_menu(self, DBG_MENU_DEBUG);
	tu_delete_menu(self, DBG_MENU_AI2);
	TU_PTR(self, DBG_MENU_MINE) = NULL;
	TU_PTR(self, DBG_MENU_AI) = NULL;
	TU_PTR(self, DBG_MENU_LIGHTS) = NULL;
	TU_PTR(self, DBG_MENU_RENDER) = NULL;
	TU_PTR(self, DBG_MENU_LOCALRF) = NULL;
	TU_PTR(self, DBG_MENU_DEBUG) = NULL;
	TU_PTR(self, DBG_MENU_AI2) = NULL;
	FUN_001d5950(); /* an empty call; nothing happens */
	if (TU_PTR(self, DBG_INPUT_DETAIL) != NULL)
		biDeleteChannel((struct biChannel *)TU_PTR(self, DBG_INPUT_DETAIL));
	TU_PTR(self, DBG_INPUT_DETAIL) = NULL;
	if (TU_PTR(self, DBG_INPUT_MAP) != NULL)
		biDeleteMap((struct biMap *)TU_PTR(self, DBG_INPUT_MAP));
	TU_PTR(self, DBG_INPUT_MAP) = NULL;
}
