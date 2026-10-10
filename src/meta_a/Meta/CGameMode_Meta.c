/*
CGAMEMODE_META.C  <-  C:\BlitzParty\Game\Src\Meta\CGameMode_Meta.cpp

CGameMode_Meta: the game mode of the board game between the minigames (vtable
0x0026f0c8). It loads the level, makes the four CMetaPlayer objects, runs the round
timer and the phases (state, +0x68):

  10 pushed, waiting for the first start     8 fade in
   0 intro (camera fly-by)                   1 playing (timer runs)
   2 round over, wait for the scores          9 score screen (mode at +0x30)
   3 outro, then the next minigame            7 finished
   5 (Input keeps running when inactive)

Vtable slots (see core_modes_a_modes.h): +0x00 delete, +0x04 Suspend 0x0014fe10,
+0x08 Resume 0x00150330, +0x0c Popped 0x0014faf0, +0x10 Pushed 0x0014da90,
+0x14 Input 0x0014db00, +0x18 Update 0x00150ce0, +0x1c Render 0x0014ee80,
+0x30 StartGame 0x0014dca0, +0x34 LoadLevel 0x0014f670.
*/

#include "meta_a.h"
#include "core_data.h"

static const char ma_file_gamemode_meta[] = "C:\\BlitzParty\\Game\\Src\\Meta\\CGameMode_Meta.cpp";

/* 0x0026f0c8  vtable of CGameMode_Meta */
const void *const vtable_0026f0c8[14] = {
	(const void *)FUN_0014dc80, (const void *)FUN_0014fe10, (const void *)FUN_00150330,
	(const void *)FUN_0014faf0, (const void *)FUN_0014da90, (const void *)FUN_0014db00,
	(const void *)FUN_00150ce0, (const void *)FUN_0014ee80, (const void *)FUN_001ec260,
	(const void *)FUN_00012930, (const void *)FUN_000128e0, (const void *)FUN_001d5950,
	(const void *)FUN_0014dca0, (const void *)FUN_0014f670,
};

/* the mode stack (core, CGameModeStack at 0x002ba750) */
#define MA_MODESTACK ((struct CGameModeStack *)CORE_ADDR(0x002ba750u))

/* the debug text printer of the release build: an empty function (0x001d5950, shared by
   identical-code folding). The calls stay, so their arguments are still evaluated. */
#define MA_DEBUG_TEXT ((void (*)(const void *, ...))FUN_001d5950)

/* names for the debug text */
static const char *const ma_state_names[14] = { /* 0x002b05b4 */
	"STAND", "IDLE", "WALK", "JUMP", "LAND", "ATTACK", "JUMP_ATTACK", "PRESS", "FLAG",
	"FALL", "HURT", "TAUNT", "SCORE", "KNOCKOUT",
};
static const char *const ma_goal_names[] = { /* 0x00293974 (charlib) */
	"Null", "Match", "Steal", "Wait", "Retaliate", "Collect", "Flee", "Fight", "Stand",
	"Get Singles", "Get My Own", "Score", "Stop", "Press",
};

/* a float tweak: CTweakMenu_AddFloat(menu, name, var, step, min, max) */
static void ma_tweak_float(void *menu, const char *name, void *var, float step, float min, float max)
{
	CTweakMenu_AddFloat((struct CTweakMenu *)menu, name, (float *)var, step, min, max);
}

static void ma_tweak_int(void *menu, const char *name, void *var, int step, int min, int max)
{
	CTweakMenu_AddInt((struct CTweakMenu *)menu, name, (int *)var, step, min, max, 0);
}

static void ma_tweak_bool(void *menu, const char *name, void *var)
{
	CTweakMenu_AddBool((struct CTweakMenu *)menu, name, (int *)var);
}

static void *ma_new_tweak_menu(const char *name, int line)
{
	void *memory = MA_NEW(0xe8, ma_file_gamemode_meta, line, 0x1001);
	return memory == NULL ? NULL : CTweakMenu_Ctor((struct CTweakMenu *)memory, name);
}

/* 0x0014da80  Meta/CGameMode_Meta.cpp: destructor */
void FUN_0014da80(undefined4 *self)
{
	*(const void **)self = vtable_0026f0c8;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x0014da90  Meta/CGameMode_Meta.cpp: Pushed: open the generic and the level package */
void FUN_0014da90(int self)
{
	int package;

	DAT_003045b8 = DAT_00283ea4;
	Core_LoadCharacterVoices(0);
	MA_I32(self, 0x14) = 1;
	package = ((int (*)(const char *, int))FUN_00071f90)("Packages\\Meta", 0);
	MA_I32(self, 0x70) = package;
	if (package != 0) {
		MA_PRINTF("opened metageneric package\n");
		/* the level table (core 0x00283c70, 0x20 bytes per level): +0x08 package path */
		package = ((int (*)(const char *, int))FUN_00071f90)(
			*(const char **)((char *)&PTR_s_Packages_MetaSite_00283c78 + MA_I32(self, 0x88) * 0x20), 0);
		MA_I32(self, 0x74) = package;
		if (package != 0)
			return;
	}
	MA_PRINTF("CGameMode_Meta::Pushed *** Could not load package 'Packages\\Meta' ***\n");
}

/* 0x0014db00  Meta/CGameMode_Meta.cpp: Input(active) */
void FUN_0014db00(int *self, int active)
{
	void *game;

	if (active == 0 && self[0x1a] != 5) {
		if (self[5] != 0)
			MA_VFN(self, 0x24, void (*)(void *))(self);
		return;
	}
	MA_DEBUG_TEXT((void *)DAT_003055d0);
	if (MA_I32(DAT_003055d0, 0x44) != 0)
		DAT_0028fda8 = DAT_002b0574 * DAT_0028fda8;
	if (active == 0)
		return;
	if (DAT_002f7a30 != 0)
		FUN_00152eb0((int *)DAT_002f7a34);
	if (self[0x1a] == 9)
		MA_VFN(self[0xc], 0x14, void (*)(void *, int))((void *)self[0xc], 1);
	if (self[0xb] == 0)
		return;
	if (DAT_002baea8 != 0) {
		CGameModeStack_Pop(MA_MODESTACK, 0);
		/* start the chosen minigame (vtable +0x0c Start(variant)) */
		game = (void *)DAT_002ba954[DAT_002baeac];
		MA_VFN(game, 0xc, void (*)(void *, int))(game, DAT_002baeb0 + 1);
		return;
	}
	CGameModeStack_Pop(MA_MODESTACK, 1);
}

/* 0x0014dbd0  Meta/CGameMode_Meta.cpp: constructor */
undefined4 *FUN_0014dbd0(undefined4 *self)
{
	int i;

	CGameMode_ctor((struct CGameMode *)self, "Meta");
	*(const void **)self = vtable_0026f0c8;
	self[0x25] = 0;
	self[0xb] = 0;
	for (i = 0; i < 9; i++)
		self[0xd + i] = 0;
	for (i = 0; i < 0xc; i++)
		DAT_00304574[i] = 0;
	self[0x21] = 0;
	self[0x1a] = 8;
	self[0x1b] = 0;
	self[0x1c] = 0;
	self[0x1d] = 0;
	self[0xc] = 0;
	self[0x19] = 0;
	self[0x23] = 0;
	self[0x24] = 0;
	MA_F32(self, 0x58) = 10.0f;     /* near plane */
	MA_F32(self, 0x5c) = 50000.0f;  /* far plane */
	self[10] = 0;
	MA_PRINTF("StartMalloc\n");
	return self;
}

/* 0x0014dc80  Meta/CGameMode_Meta.cpp: scalar deleting destructor */
undefined4 FUN_0014dc80(undefined4 self, byte flags)
{
	FUN_0014da80((undefined4 *)self);
	if ((flags & 1) != 0)
		MA_DELETE(self);
	return self;
}

/* 0x0014dca0  Meta/CGameMode_Meta.cpp: StartGame (vtable +0x30) */
void FUN_0014dca0(int *self)
{
	void *memory;
	void *mode;
	int scores[4];
	int i;

	DAT_002f28ac = 1;
	if (DAT_002baeb8 == 0 && self[0x1a] != 10) {
		self[0x1a] = 10;
		memory = MA_NEW(0x2b8, ma_file_gamemode_meta, 0x132, 0x1001);
		mode = memory == NULL ? NULL : (void *)FUN_00021820((undefined4 *)memory);
		CGameModeStack_Push(MA_MODESTACK, (struct CGameMode *)mode, 1);
		return;
	}
	MA_VFN(self, 0x34, void (*)(void *))(self);
	self[4] = 1;
	MA_F32(self, 0x7c) = 60.0f;
	FUN_00072720((undefined4)DAT_00305630);
	for (i = 0; i < DAT_002bae94; i++)
		scores[i] = DAT_002baee8[i * 10];
	FUN_000796c0(DAT_002ecfac);
	FUN_0007a080(DAT_002ecfac);
	FUN_00079490(DAT_002ecfac, DAT_002b057c * 0.016666668f); /* 0x00262b7c: 1/60 */
	FUN_0007a200(DAT_002ecfac, (undefined4 *)scores);
	for (i = 0; i < DAT_002bae94; i++) {
		FUN_00156580(DAT_003075bc[i]);
		MA_I32(DAT_003075bc[i], 0x1124) = 0;
		MA_VFN(DAT_003075bc[i], 0x24, void (*)(void *))((void *)DAT_003075bc[i]);
		FUN_000d62c0(DAT_003055d4, MA_I32(DAT_003075bc[i], 0x30) + 0xd0, 0,
			MA_I32(DAT_003075bc[i], 0x30) + 0x194);
	}
	FUN_00044b00(3, MA_U32(DAT_003055d0, 0x48), 0);
	bdSetNearFar(MA_F32(self, 0x58), MA_F32(self, 0x5c));
	if (self[0x24] == 0) {
		self[0x23] = 1;
	} else {
		self[0x23] = 2;
		FUN_00152fb0();
	}
	FUN_00155de0();
	self[0x23] = 0;
	self[0x1a] = 10;
	if (DAT_002baeb8 != 0) {
		/* attract mode: random characters */
		self[0x1a] = 8;
		MA_F32(self, 0x7c) = 60.0f;
		DAT_002baef0[0] = (ma_rand() & 0xf) + 3;
		DAT_002baef0[10] = (ma_rand() & 0xf) + 3;
		DAT_002baef0[20] = (ma_rand() & 0xf) + 3;
		DAT_002baef0[30] = (ma_rand() & 0xf) + 3;
		FUN_000d7420(DAT_003055d4, (undefined4 *)0, 5);
	}
	FUN_000b69d0(1, 0);
}

/* 0x0014df90  Meta/CGameMode_Meta.cpp: the tweak menus */
void FUN_0014df90(int self)
{
	char name[32];
	char title[128];
	void *menu;
	int character;
	int i;

	sprintf(title, "%s", *(const char **)((char *)&PTR_s_Outlands_00283c70 + MA_I32(self, 0x88) * 0x20));

	menu = ma_new_tweak_menu("Stuff", 0x1ae);
	MA_PTR(self, 0x3c) = menu;
	ma_tweak_int(menu, "Super Orb Value", &DAT_002b08c4, 1, 0, 4);
	ma_tweak_bool(MA_PTR(self, 0x3c), "Auto Collect", &DAT_002b05ac);
	ma_tweak_bool(MA_PTR(self, 0x3c), "Auto Score", &DAT_002b05b0);
	if (DAT_002f289c[0] != 0)
		ma_tweak_float(MA_PTR(self, 0x3c), "Shield Time", MA_ADDR(DAT_002f289c[0], 0x10a8), 100.0f, 1.0f, 100000.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "Thickness Base", &DAT_002b0878, 0.1f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "Thickness Amp", &DAT_002b087c, 0.1f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "Thickness Freq", &DAT_002b0880, 0.01f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "R Base", &DAT_002b0884, 1.0f, 0.0f, 255.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "R Amp", &DAT_002b0888, 1.0f, 0.0f, 255.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "R Freq", &DAT_002b088c, 0.05f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "G Base", &DAT_002b0890, 1.0f, 0.0f, 255.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "G Amp", &DAT_002b0894, 1.0f, 0.0f, 255.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "G Freq", &DAT_002b0898, 0.05f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "B Base", &DAT_002b089c, 1.0f, 0.0f, 255.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "B Amp", &DAT_002b08a0, 1.0f, 0.0f, 255.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "B Freq", &DAT_002b08a4, 0.05f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "A Base", &DAT_002b08a8, 1.0f, 0.0f, 255.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "A Amp", &DAT_002b08ac, 1.0f, 0.0f, 255.0f);
	ma_tweak_float(MA_PTR(self, 0x3c), "A Freq", &DAT_002b08b0, 0.05f, 0.0f, 100.0f);
	ma_tweak_int(MA_PTR(self, 0x3c), "Water R", &DAT_002b0580, 1, 0, 0xff);
	ma_tweak_int(MA_PTR(self, 0x3c), "Water G", &DAT_002b0584, 1, 0, 0xff);
	ma_tweak_int(MA_PTR(self, 0x3c), "Water B", &DAT_002b0588, 1, 0, 0xff);
	ma_tweak_int(MA_PTR(self, 0x3c), "Water A", &DAT_002b058c, 1, 0, 0xff);
	ma_tweak_int(MA_PTR(self, 0x3c), "Pause R", &DAT_002ecd84, 1, 0, 0xff);
	ma_tweak_int(MA_PTR(self, 0x3c), "Pause G", &DAT_002ecd88, 1, 0, 0xff);
	ma_tweak_int(MA_PTR(self, 0x3c), "Pause B", &DAT_002909ac, 1, 0, 0xff);

	menu = ma_new_tweak_menu("New Gameplay", 0x1d5);
	MA_PTR(self, 0x34) = menu;
	ma_tweak_int(menu, "Neutral%", &DAT_002b08c0, 1, 0, 100);
	ma_tweak_float(MA_PTR(self, 0x34), "Goal Radius", &DAT_002b0598, 5.0f, 0.0f, 1000.0f);
	ma_tweak_float(MA_PTR(self, 0x34), "Orb Radius", &DAT_002b09ec, 5.0f, 0.0f, 1000.0f);
	for (i = 0; i < 4; i++) {
		sprintf(name, "Ball Score %d", i + 1);
		ma_tweak_int(MA_PTR(self, 0x34), name, &DAT_002b07c8[i], 1, 0, 1000);
	}
	ma_tweak_float(MA_PTR(self, 0x34), "AI Attack Delay", &DAT_002b0850, 1.0f, 0.0f, 1000.0f);

	menu = ma_new_tweak_menu("Generic Tweaks", 0x1e3);
	MA_PTR(self, 0x38) = menu;
	ma_tweak_bool(menu, "Show Framerate", &DAT_00305574);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Blend Speed 1", &DAT_002b078c[0], 0.01f, 0.0f, 1.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Blend Speed 2", &DAT_002b078c[1], 0.01f, 0.0f, 1.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Blend Speed 3", &DAT_002b078c[2], 0.01f, 0.0f, 1.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Blend Speed 4", &DAT_002b078c[3], 0.01f, 0.0f, 1.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Anim Speed 1", &DAT_002b079c[0], 0.01f, 0.0f, 5.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Anim Speed 2", &DAT_002b079c[1], 0.01f, 0.0f, 5.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Anim Speed 3", &DAT_002b079c[2], 0.01f, 0.0f, 5.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Anim Speed 4", &DAT_002b079c[3], 0.01f, 0.0f, 5.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Hurt Anim Speed", &DAT_002b05a4, 0.1f, 0.0f, 10.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "KO Anim Speed", &DAT_002b05a8, 0.1f, 0.0f, 10.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Step Speed", &DAT_002b0784, 0.5f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Step Time", &DAT_002b0788, 0.5f, 0.0f, 100.0f);
	ma_tweak_bool(MA_PTR(self, 0x38), "Steal Orb on Hurt", &DAT_00305658);
	ma_tweak_float(MA_PTR(self, 0x38), "Jump", &DAT_00292c98, 0.5f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Gravity", &DAT_00292c84, 0.1f, 0.0f, 100.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Speedup Power", &DAT_00293a28, 0.1f, 1.0f, 5.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "SuperAttack Knockback", &DAT_00293a2c, 0.1f, 1.0f, 5.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "SuperAttack Hurt", &DAT_00293a30, 0.1f, 1.0f, 5.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "SuperAttack Range", &DAT_00293a38, 0.1f, 1.0f, 5.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "SuperAttack Getup", &DAT_00293a34, 0.1f, 1.0f, 5.0f);
	ma_tweak_bool(MA_PTR(self, 0x38), "draw shadows", &DAT_002b04e4);
	ma_tweak_float(MA_PTR(self, 0x38), "Time left", MA_ADDR(self, 0x7c), 60.0f, 1.0f, 100000.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Active Range", &DAT_002b09ec, 10.0f, 50.0f, 500.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Carry Fac", &DAT_002b0874, 0.005f, 0.0f, 2.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "Attack Ang", &DAT_002b074c, 1.0f, 0.0f, 360.0f);
	ma_tweak_float(MA_PTR(self, 0x38), "GetUp Time", &DAT_002b07b8, 1.0f, 1.0f, 1000.0f);

	/* the rumble effect names (classlib_ui CRumble at 0x002ed324) */
	FUN_00080a20((int *)DAT_002ed324);
	FUN_00080aa0((undefined4 *)DAT_002ed324, (undefined4)"Bomb Fall");
	FUN_00080aa0((undefined4 *)DAT_002ed324, (undefined4)"Air Raid");
	FUN_00080aa0((undefined4 *)DAT_002ed324, (undefined4)"Conveyor");
	FUN_00080aa0((undefined4 *)DAT_002ed324, (undefined4)"Squished : Fish, Crate");
	FUN_00080aa0((undefined4 *)DAT_002ed324, (undefined4)"Generic Knock Down");

	menu = ma_new_tweak_menu("Players", 0x218);
	MA_PTR(self, 0x54) = menu;
	ma_tweak_bool(menu, "High Res Characters", &DAT_002b0590);
	for (i = 0; i < 4; i++) {
		character = DAT_002baed4[i * 10];
		/* the character table (classlib_ui 0x002906f8, 0x60 bytes each): +0x00 name */
		sprintf(name, "P%d - %s", i + 1, *(const char **)((char *)&PTR_DAT_002906f8 + character * 0x60));
		menu = MA_NEW(0xe8, ma_file_gamemode_meta, 0x21e, 0x1001);
		menu = menu == NULL ? NULL :
			CTweakMenu_CtorSub((struct CTweakMenu *)menu, (struct CTweakMenu *)MA_PTR(self, 0x54), name);
		MA_PTR(self, 0x40 + i * 4) = menu;
		ma_tweak_float(menu, "Accel", (char *)&DAT_0029071c + character * 0x60, 0.1f, 0.0f, 100.0f);
		ma_tweak_float(menu, "MAXSPEED", (char *)&DAT_00290720 + character * 0x60, 0.5f, 0.0f, 1000.0f);
		ma_tweak_float(menu, "Attack power", (char *)&DAT_00290734 + character * 0x60, 10.0f, 0.0f, 1000.0f);
		ma_tweak_float(menu, "Attack range", (char *)&DAT_00290728 + character * 0x60, 10.0f, 0.0f, 1000.0f);
		ma_tweak_float(menu, "KO limit", (char *)&DAT_00290738 + character * 0x60, 10.0f, 0.0f, 1000.0f);
		ma_tweak_int(MA_PTR(self, 0x40 + i * 4), "Think time", MA_ADDR(DAT_003075bc[i], 0x10b4), 5, 0, 10000);
		ma_tweak_int(MA_PTR(self, 0x40 + i * 4), "Aggression", (char *)&DAT_0029073c + character * 0x60, 1, 0, 100);
		ma_tweak_int(MA_PTR(self, 0x40 + i * 4), "Retaliation", (char *)&DAT_00290740 + character * 0x60, 1, 0, 100);
		/* (the original passes the same field for Greed and Stealing) */
		ma_tweak_int(MA_PTR(self, 0x40 + i * 4), "Greed", (char *)&DAT_00290740 + 4 + character * 0x60, 1, 0, 100);
		ma_tweak_int(MA_PTR(self, 0x40 + i * 4), "Stealing", (char *)&DAT_00290740 + 4 + character * 0x60, 1, 0, 100);
		ma_tweak_int(MA_PTR(self, 0x40 + i * 4), "Caution", (char *)&DAT_0029074c + character * 0x60, 1, 0, 100);
	}
}

/* 0x0014ea80  Meta/CGameMode_Meta.cpp: update the world for one frame */
void FUN_0014ea80(void)
{
	short state;
	int platform;
	int ground;
	int i;
	float t;

	AnimTex_AdvanceList((struct CLFList *)DAT_0030563c, 1.0f, 1);
	FUN_0015b090(DAT_003082c0);
	if (MA_I32(DAT_0030564c, 0x68) != 1 && MA_I32(DAT_0030564c, 0x68) != 0)
		FUN_000b99f0();
	if (MA_I32(DAT_0030564c, 0x68) == 1) {
		FUN_00155de0();
		FUN_000b99f0();
		MA_F32(DAT_0030564c, 100) = DAT_0028fda8 + MA_F32(DAT_0030564c, 100);
		for (i = 0; i < DAT_002bae94; i++) {
			/* fell off the bottom or the top of the level */
			if (MA_F32(MA_CHAR(DAT_003075bc[i]), 0xd4) <= DAT_00292c88 + 10.0f) /* 0x002536d8 */
				MA_VFN(DAT_003075bc[i], 0x24, void (*)(void *))((void *)DAT_003075bc[i]);
			if (2500.0f <= MA_F32(MA_CHAR(DAT_003075bc[i]), 0xd4)) /* 0x00268700 */
				MA_VFN(DAT_003075bc[i], 0x24, void (*)(void *))((void *)DAT_003075bc[i]);
			if (FUN_00163120(DAT_003075bc[i], 150.0f, 0.0f) != 0 && MA_I16(DAT_003075bc[i], 0x36) != 0xe)
				FUN_000b02f0(DAT_003075bc[i], 0xe);
			if (0 < MA_I32(MA_PTR(MA_CHAR(DAT_003075bc[i]), 0x18c), 0x1d0))
				FUN_00157be0((int *)DAT_003075bc[i], i);
			if (0.0f < DAT_003045a4[i])
				FUN_00080630((int)DAT_002ed324, i, 4, 0x3e83, 0x3e83);
			ground = MA_I32(MA_CHAR(DAT_003075bc[i]), 0x18c);
			platform = MA_I32(ground, 0x1b0);
			if ((platform == 0 || MA_I32(platform, 0x448) == 0) &&
				((platform = MA_I32(ground, 0x1b4)) == 0 || MA_I32(platform, 0x448) == 0)) {
				t = DAT_003045a4[i] - DAT_0028fda8;
				DAT_003045a4[i] = t;
				if (t <= 0.0f) {
					FUN_00080630((int)DAT_002ed324, i, 4, 0, 0);
					DAT_003045a4[i] = 0.0f;
				}
			} else {
				/* on a shaking platform: rumble for 8 frames */
				FUN_00080630((int)DAT_002ed324, i, 4, 0x3e83, 0x3e83);
				DAT_003045a4[i] = 8.0f;
			}
			if (DAT_002baed8[i * 10] == 0)
				MA_VFN(DAT_003075bc[i], 0xc, void (*)(void *))((void *)DAT_003075bc[i]);
		}
		for (i = 0; i < DAT_002bae94; i++)
			MA_I32(DAT_003075bc[i], 0x1128) = 0;
		FUN_000b1100();
		for (i = 0; i < DAT_002bae94; i++) {
			if ((MA_U32(MA_CHAR(DAT_003075bc[i]), 0x220) & 0x8000) != 0) {
				state = MA_I16(DAT_003075bc[i], 0x36);
				if (state != 10 && state != 0xd)
					FUN_00156c40(DAT_003075bc[i], (float *)DAT_002ea478, 10000.0f, (ma_rand() & 1) + 1, 1);
			}
		}
	} else {
		DAT_003045a4[0] = 0.0f;
		DAT_003045a4[1] = 0.0f;
		DAT_003045a4[2] = 0.0f;
		DAT_003045a4[3] = 0.0f;
	}
	FUN_00082b20((undefined4 *)(MA_CHAR(DAT_002f289c[0]) + 0xd0), (undefined4 *)(MA_CHAR(DAT_002f289c[1]) + 0xd0),
		(undefined4 *)(MA_CHAR(DAT_002f289c[2]) + 0xd0), (undefined4 *)(MA_CHAR(DAT_002f289c[3]) + 0xd0));
	FUN_00163c60();
	FUN_001635b0();
	FUN_000af9e0();
	FUN_00072720((undefined4)DAT_00305630);
	COverlay_UpdateAll();
	FUN_00162b60();
	FUN_00162230();
	FUN_000b9ae0();
	FUN_0015b450();
	FUN_000ba6d0();
	FUN_00161b80();
	RefBot_Update();
	CProjector_UpdateAll();
	FUN_000b7c20();
	FUN_00160560();
	FUN_0015fd10();
	FUN_000be9a0();
	FUN_0015f1d0();
	FUN_0015e6e0();
	FUN_0015e370();
	FUN_0015d440((ushort *)DAT_00308460);
	FUN_000af380();
	CRayControl_UpdateAll();
	FUN_000b8b60();
	MA_DUAL(FUN_0003c620, void (*)(int, undefined4), void (*)(int, float),
		(0, ma_fbits(DAT_0028fda8 * 0.016666668f)), (0, DAT_0028fda8 * 0.016666668f));
	FUN_0015ba80();
	FUN_0015b730();
	FUN_00043790();
	if (DAT_002f7a30 == 0) {
		FUN_000d6e80(DAT_003055d4);
		if (MA_I32(DAT_003055d4, 0x568) == 5)
			bdSetFieldOfView(1.2216f, 0.0f); /* 0x3f9c61aa: 70 degrees */
	}
	MA_I32(DAT_0030564c, 0x84) = MA_I32(DAT_0030564c, 0x84) + 1;
}

/* 0x0014ee80  Meta/CGameMode_Meta.cpp: Render(active) */
void FUN_0014ee80(int self, int active)
{
	int character;
	int ch;
	int i;
	int frames;
	int alpha;
	float x;
	float y;
	const char *text;
	const unsigned char *colour;

	if (active == 0)
		return;
	if (MA_I32(self, 0x68) == 7)
		return;
	FUN_00044b00(3, MA_U32(DAT_003055d0, 0x48), 0);
	FUN_000ae4d0();
	FUN_000aff10();
	if (DAT_002b04e4 != 0)
		FUN_000bed50();
	FUN_00160060();
	FUN_0015f790();
	FUN_00160bd0();
	FUN_00044b00(8, 0, 0);
	FUN_00159e10();
	if (DAT_002b0570 != 0)
		FUN_00162f90();
	CProjector_DrawAll();
	FUN_00162260();
	FUN_00080cc0();
	FUN_0015d360((ushort *)DAT_00308460);
	FUN_001649b0();
	CRayControl_DrawAll();
	FUN_0015c580();
	FUN_0015f620();
	FUN_000ba460();
	FUN_0015e710();
	if (DAT_003055d8 != 0) {
		FUN_0003c690(MA_I32(self, 0xa0), 0);
		FUN_001642c0();
	}
	for (i = 0; i < DAT_002bae94; i++) {
		if (MA_I16(DAT_003075bc[i], 0x36) != 0xe) {
			ch = MA_CHAR(DAT_003075bc[i]);
			FUN_0007bb70(i, (undefined4 *)(ch + 0xd0), MA_F32(ch, 0x1e8));
		}
	}
	FUN_0015b500();
	FUN_0015c890();

	/* debug texts (the printer is empty in this build) */
	if (DAT_002ba99c % 0x1e == 0)
		MA_I32(DAT_003055d0, 0x50) = (int)CORE_VAR(float, 0x002ba994);
	if (DAT_00305574 != 0)
		MA_DEBUG_TEXT((void *)DAT_003055d0, 190.0f, 220.0f, 0x7f, 0x7f, 0x7f, 0x7f, 0, "%d",
			MA_I32(DAT_003055d0, 0x50));
	if (MA_I32(DAT_003055d0, 0x40) != 0) {
		MA_DEBUG_TEXT((void *)DAT_003055d0, -300.0f, 180.0f, 0x7f, 0x7f, 0x7f, 0x7f, 0, "%f",
			(double)DAT_002f2890);
		if (DAT_002ba99c % 0x14 == 0) {
			ch = MA_CHAR(DAT_003075bc[0]);
			DAT_00305648 = sqrtf(MA_F32(ch, 0x19c) * MA_F32(ch, 0x19c) +
				MA_F32(ch, 0x198) * MA_F32(ch, 0x198) + MA_F32(ch, 0x194) * MA_F32(ch, 0x194));
		}
		MA_DEBUG_TEXT((void *)DAT_003055d0, 0.0f, 180.0f, 0x7f, 0x7f, 0x7f, 0x7f, 0, "%f",
			(double)DAT_00305648);
		if (MA_I32(DAT_003055d0, 0x40) != 0) {
			for (i = 0; i < DAT_002bae94; i++) {
				x = (float)((i % 2) * 400 - 200);
				y = (float)((i / 2) * -0xb4 + 0x5a);
				character = DAT_002baed4[i * 10];
				colour = (const unsigned char *)&DAT_00290718 + character * 0x60;
				if (MA_I32(DAT_003055d0, 0x40) == 2) {
					FUN_000b0b00(DAT_003075bc[i]);
					if (MA_I32(DAT_003075bc[i], 0x28) != 0 ||
						(float)MA_I32(DAT_003075bc[i], 0x10b4) < MA_F32(DAT_003075bc[i], 0x20))
						text = ma_goal_names[MA_I32(DAT_003075bc[i], 4)];
					else
						text = "Thinking";
					MA_DEBUG_TEXT((void *)DAT_003055d0, x, y, colour[0], colour[1], colour[2], 0x7f, 1, text);
					MA_DEBUG_TEXT((void *)DAT_003055d0, x, y + 20.0f, colour, 1, /* 0x00255530 */
						ma_state_names[MA_U16(DAT_003075bc[i], 0x36)]);
				}
				if (MA_I32(DAT_003055d0, 0x40) == 4)
					MA_DEBUG_TEXT((void *)DAT_003055d0, x, y, colour, 1, "%d",
						CLRoute_CellAt(MA_FP(MA_CHAR(DAT_003075bc[i]), 0xd0)));
			}
			if (MA_I32(DAT_003055d0, 0x40) == 3)
				FUN_0015e3a0();
			if (MA_I32(DAT_003055d0, 0x40) == 4)
				CLRoute_DrawDebug();
		}
	}

	if (MA_I32(self, 0x68) != 9 && MA_I32(self, 0x68) != 3) {
		FUN_00072720((undefined4)DAT_00305630);
		COverlay_DrawAll(1);
	}
	if (MA_I32(DAT_003055d4, 0x568) == 2) {
		FUN_0007e270(0.0f, 0);
	} else {
		FUN_000435f0(MA_FP(DAT_003055d4, 0x4e4), MA_FP(DAT_003055d4, 0x504), 0.0f);
		FUN_00043790();
	}
	text = NULL;
	switch (MA_I32(self, 0x68)) {
	case 0:
		if (MA_I32(DAT_003055d0, 0x40) != 0)
			text = "INTRO";
		break;
	case 1:
		if (MA_I32(DAT_003055d0, 0x40) != 0)
			text = "PLAYING";
		break;
	case 3:
		if (MA_I32(DAT_003055d0, 0x40) != 0)
			MA_DEBUG_TEXT((void *)DAT_003055d0, 0.0f, 220.0f, 0x7f, 0x7f, 0x7f, 0x7f, 1, "OUTRO");
		frames = (int)MA_F32(self, 0x7c);
		CLScreen_DrawDarken(((0x3d - frames) * 0x7f) / 0x3c + 0x80, 1);
		break;
	case 4:
		if (MA_I32(DAT_003055d0, 0x40) != 0)
			text = "GAME OVER";
		break;
	case 6:
		if (MA_I32(DAT_003055d0, 0x40) != 0)
			text = "DEALING";
		break;
	case 8:
		frames = (int)MA_F32(self, 0x7c);
		alpha = ((0x3c - frames) * 300) / 0x3c;
		if (0xff < alpha)
			alpha = 0xff;
		CLScreen_DrawDarken(0xff - alpha, 1);
		break;
	case 9:
		CLScreen_DrawDarken(0x80, 0);
		MA_VFN(MA_PTR(self, 0x30), 0x1c, void (*)(void *, int))(MA_PTR(self, 0x30), 1);
		break;
	default:
		break;
	}
	if (text != NULL)
		MA_DEBUG_TEXT((void *)DAT_003055d0, 0.0f, 220.0f, 0x7f, 0x7f, 0x7f, 0x7f, 1, text);
	if (DAT_002ec904 != 0)
		CLFade_Draw(DAT_0028fda8);
	FUN_00044b00(3, MA_U32(DAT_003055d0, 0x48), 0);
}

/* 0x0014f4a0  Meta/CGameMode_Meta.cpp: make the camera (charlib, 0x574 bytes) */
void FUN_0014f4a0(void)
{
	void *memory;

	memory = MA_NEW(0x574, ma_file_gamemode_meta, 0x8fc, 0x1001);
	if (memory != NULL) {
		DAT_003055d4 = FUN_000d63f0((int)memory, 5);
		return;
	}
	DAT_003055d4 = 0;
}

/* 0x0014f520  Meta/CGameMode_Meta.cpp: update the score HUD */
void FUN_0014f520(int self)
{
	int value;
	int i;
	int high;
	int low;
	int top;
	int bottom;
	int scores[4];
	float timer;

	high = 0;
	low = 99999999;
	top = 5;
	bottom = 0;
	if (MA_I32(self, 0x68) == 1) {
		timer = MA_F32(self, 0x7c);
	} else {
		timer = DAT_002b057c;
		if (MA_I32(self, 0x68) != 8 && MA_I32(self, 0x68) != 0)
			goto scores;
	}
	FUN_000792f0(DAT_002ecfac, timer * 0.016666668f, 1); /* 0x00262b7c */
scores:
	for (i = 0; i < DAT_002bae94; i++)
		scores[i] = DAT_002baee8[i * 10];
	if (0 < DAT_002bae94) {
		for (i = 0; i < DAT_002bae94; i++) {
			value = scores[i];
			if (high < value)
				high = value;
			if (value < low)
				low = value;
		}
		/* the bar scale: multiples of five around the scores */
		if (0 < high) {
			top = (high / 5 + 1) * 5;
			if (5 < low)
				bottom = (low / 5) * 5;
		}
	}
	FUN_00078960(DAT_002ecfac, (undefined4 *)scores);
	FUN_00078c20(DAT_002ecfac, scores);
	MA_F32(DAT_002ecfac, 0x40) = (float)top;
	MA_F32(DAT_002ecfac, 0x3c) = (float)bottom;
	FUN_0007bd80(DAT_002ecfac);
}

/* 0x0014f630  Meta/CGameMode_Meta.cpp: delete the tweak menus */
void FUN_0014f630(int self)
{
	void *menu;
	int i;

	for (i = 0; i < 9; i++) {
		menu = MA_PTR(self, 0x34 + i * 4);
		if (menu != NULL) {
			CTweakMenu_Dtor((struct CTweakMenu *)menu);
			MA_DELETE(menu);
			MA_PTR(self, 0x34 + i * 4) = NULL;
		}
	}
	FUN_00080b60((int *)DAT_002ed324);
}

/* 0x0014f670  Meta/CGameMode_Meta.cpp: LoadLevel (vtable +0x34) */
void FUN_0014f670(int self)
{
	int *block;
	int count;
	int i;
	float position[4];
	void *memory;
	undefined4 player;
	char *entry;
	int seed;

	CLSamples_LoadCommentatorSet(1);
	/* route height buffers: one 0x18-byte object per route cell (new[] with a count) */
	count = DAT_00292c78;
	block = (int *)MA_NEW(DAT_00292c78 * 0x18 + 4, ma_file_gamemode_meta, 0x254, 0x1001);
	if (block == NULL) {
		DAT_00304558 = 0;
	} else {
		*block = count;
		for (i = 0; i < count; i++)
			FUN_000ad830((undefined4 *)((char *)(block + 1) + i * 0x18));
		DAT_00304558 = (int)(block + 1);
	}
	FUN_000c39b0((undefined4)DAT_003055e8, DAT_00304558, 0x18, 0x18);
	FUN_000b6aa0(1);
	MA_I32(self, 0x68) = 8;
	MA_F32(self, 0x7c) = 60.0f;
	FUN_00161c00();
	FUN_00162c40();
	FUN_00152f90();
	bLoadPackageResources((struct bkPackage *)MA_PTR(self, 0x74), 0xffffffff, 0xdefa, 0);
	bLoadPackageResources((struct bkPackage *)MA_PTR(self, 0x70), 0xffffffff, 0xdefa, 0);
	Sprite_LoadDefs((struct CSpriteDef *)DAT_002b0d30, 0xb, 0);
	Sprite_LoadDefs((struct CSpriteDef *)s_aisteam1_00290f00, 6, 0);
	FUN_000827f0((undefined4)FUN_000b4bd0, 4000);
	FUN_000b9bf0();
	FUN_00160720();
	FUN_00159fb0(0);
	CProjector_Reset();
	CProjector_LoadSpiralTextures();
	FUN_0014f4a0();
	FUN_0015b7a0();
	DAT_003055d8 = 0;
	if (FUN_00152e50(self) == 0) {
		MA_PRINTF("Failed to load level\n");
		return;
	}
	FUN_00158e10();
	CLRoute_Rebuild();
	FUN_00159000();
	FUN_0015a770(1);
	MA_I32(self, 0x84) = 0;
	position[0] = 0.0f;
	position[1] = 0.0f;
	position[2] = 0.0f;
	position[3] = 0.0f;
	/* the six characters' textures: two names per 0x108-byte entry at 0x00290128 */
	for (entry = (char *)&DAT_00290148; entry < (char *)&DAT_00290148 + 6 * 0x108; entry += 0x108) {
		*(int *)entry = (int)bLoadResource(NULL, entry - 0x80, 1, 0);
		*(int *)(entry + 0x84) = (int)bLoadResource(NULL, entry + 4, 1, 0);
	}
	for (i = 0; i < DAT_002bae94; i++) {
		memory = MA_NEW(0x1164, ma_file_gamemode_meta, 0x29d, 0x1001);
		player = memory == NULL ? 0 : (undefined4)FUN_00157100((undefined4 *)memory, i, (undefined4)position);
		seed = ma_rand();
		DAT_003075bc[i] = player;
		DAT_002f289c[i] = player;
		DAT_002b04e8 = 1;
		DAT_002b0590 = 1;
		position[0] = (float)(seed % 0xaf0 - 0x578);
		position[2] = (float)(ma_rand() % 0xaf0 - 0x578);
		if (DAT_002bae98 == 2) {
			/* tournament: a random score for the round */
			MA_SCORE_ROUND_SUM(DAT_002b9968, i) = (ma_rand() & 0xf) + 3;
		}
	}
	CScoreTable_Rank((struct CScoreTable *)&DAT_002b9968);
	DAT_003082bc = MA_SCORE_FIRST_PLACE;
	FUN_00152c60();
	if (DAT_00309244 != 0)
		FUN_001649d0();
	if (MA_I32(self, 0x88) == 4)
		FUN_0015b600();
	if (DAT_003055d8 != 0)
		FUN_001645f0((undefined4 *)MA_ADDR(self, 0xa0), DAT_0030455c);
	CRayControl_ResetList();
	RefBot_Create();
	FUN_0015ff80();
	FUN_0015f750();
	MA_DUAL(FUN_000be6d0, void (*)(int, undefined4, undefined4, undefined4), void (*)(int, float, float, int),
		(2, ma_fbits(1.1f), ma_fbits(4.0f), 0), (2, 1.1f, 4.0f, 0));
	if (MA_I32(self, 0x90) != 0) {
		/* a resumed board: restart the clock and clear the tiles */
		MA_F32(DAT_0030564c, 100) = DAT_002b057c;
		FUN_000792f0(DAT_002ecfac, DAT_002b057c * 0.016666668f, 1);
		FUN_0015b400();
		MA_I32(DAT_003082c0, 0xc) = 0;
		for (i = 0x330; i < 0x12d0; i += 4)
			MA_I32(DAT_003082c0, i) = 0;
		for (i = 0; i < 0x14; i++)
			MA_U8(DAT_003082c0, 0x12d8 + i) = 0;
		MA_U8(DAT_003082c0, 0x12ec) = 0;
		for (i = 0; i < DAT_002bae94; i++)
			DAT_002baee8[i * 10] = 0;
		DAT_00309254 = 1;
		FUN_001592e0();
	}
}

/* free the route buffers and everything else that LoadLevel made (shared tail of
   Popped 0x0014faf0 and DeleteFirstHalf 0x0014fe50, written twice in the original) */
static void ma_free_route_buffers(void)
{
	int *block;
	int count;
	int i;

	FUN_000c39b0((undefined4)DAT_003055e8, DAT_00304558, 0x18, 0x18);
	FUN_00072720((undefined4)DAT_00305630);
	if (DAT_00304558 != 0) {
		block = (int *)(DAT_00304558 - 4);
		count = *block;
		/* delete[]: destroy the elements from the last one */
		for (i = count - 1; i >= 0; i--)
			CLRouteHeights_Free2((struct CLRouteHeights *)(DAT_00304558 + i * 0x18));
		MA_DELETE(block);
	}
}

static void ma_free_players(void)
{
	int player;
	int i;

	for (i = 0; i < DAT_002bae94; i++) {
		if (DAT_003075bc[i] != 0) {
			if (MA_I32(DAT_003075bc[i], 0x5c) != 0) {
				FUN_00048970(MA_U32(DAT_003075bc[i], 0x5c));
				MA_I32(DAT_003075bc[i], 0x5c) = 0;
			}
			if (MA_I32(DAT_003075bc[i], 0x74) != 0) {
				FUN_00048970(MA_U32(DAT_003075bc[i], 0x74));
				MA_I32(DAT_003075bc[i], 0x74) = 0;
			}
			player = DAT_003075bc[i];
			if (player != 0) {
				FUN_00155e30((undefined4 *)player);
				MA_DELETE(player);
			}
			DAT_003075bc[i] = 0;
			DAT_002f289c[i] = 0;
		}
	}
}

static void ma_close_package(int self, int offset)
{
	struct bkPackage *package = (struct bkPackage *)MA_PTR(self, offset);

	bkDeletePackageResources(bkGetPackageId(package), 0xffffffff);
	FUN_0004db40((int *)package);
	MA_PTR(self, offset) = NULL;
}

/* 0x0014faf0  Meta/CGameMode_Meta.cpp: Popped: free the level */
void FUN_0014faf0(int self)
{
	int camera;
	int *heights;
	int i;

	DAT_00293928 = DAT_00305570;
	if (MA_PTR(self, 0x30) != NULL) {
		MA_VFN(MA_PTR(self, 0x30), 0x4, void (*)(void *))(MA_PTR(self, 0x30));
		MA_VFN(MA_PTR(self, 0x30), 0xc, void (*)(void *))(MA_PTR(self, 0x30));
		if (MA_PTR(self, 0x30) != NULL)
			MA_VFN(MA_PTR(self, 0x30), 0x0, void (*)(void *, int))(MA_PTR(self, 0x30), 1);
		MA_PTR(self, 0x30) = NULL;
	}
	ma_free_route_buffers();
	camera = DAT_003055d4;
	if (DAT_003055d4 != 0) {
		FUN_000d64a0(DAT_003055d4);
		MA_DELETE(camera);
		DAT_003055d4 = 0;
	}
	for (i = 0; i < 0x64; i++) {
		heights = (int *)DAT_00309f00[i];
		if (heights != NULL) {
			CLRouteHeights_Dtor((struct CLRouteHeights *)heights);
			MA_DELETE(heights);
			DAT_00309f00[i] = 0;
		}
	}
	if (DAT_002f7a34 != 0) {
		MA_VFN(DAT_002f7a34, 0, void (*)(void *, int))((void *)DAT_002f7a34, 1);
		DAT_002f7a30 = 0;
		DAT_002f7a34 = 0;
	}
	FUN_00152e90();
	FUN_00080bb0();
	FUN_000b6aa0(0);
	COverlay_DeleteAll();
	FUN_00162c10();
	FUN_000b7b80();
	FUN_00162180();
	FUN_00160ee0();
	CProjector_DeleteAll();
	FUN_00161460();
	FUN_00161510();
	FUN_00154270();
	FUN_0015b400();
	FUN_000b84a0();
	FUN_000ba430();
	FUN_0015e080();
	FUN_000b9b80();
	FUN_000af350();
	FUN_00163560();
	FUN_00163b20();
	AnimTex_DeleteList((struct CLFList *)DAT_0030563c);
	FUN_0015d010((ushort *)DAT_00308460);
	Sprite_FreeDefs((struct CSpriteDef *)DAT_002b0d30, 0xb);
	Sprite_FreeDefs((struct CSpriteDef *)s_aisteam1_00290f00, 6);
	FUN_0015b4d0();
	CRayControl_DeleteAll();
	MA_DEBUG_TEXT(NULL);
	RefBot_Delete();
	FUN_001603b0();
	FUN_0015fcd0();
	FUN_0015eeb0();
	FUN_0015f650();
	FUN_0015b650();
	FUN_000be460();
	FUN_00163f30();
	FUN_00164b00();
	FUN_00163d80();
	FUN_0015c840();
	FUN_0003c8b0(0);
	SpriteVB_Release((struct CSpriteVB *)DAT_00290ea8);
	FUN_0015b9f0();
	ma_free_players();
	if (MA_I32(self, 0x94) != 0) {
		MA_DELETE(MA_PTR(self, 0x94));
		MA_I32(self, 0x94) = 0;
	}
	if (DAT_003055d0 != 0) {
		MA_DEBUG_TEXT((void *)DAT_003055d0);
		if (DAT_003055d0 != 0)
			MA_VFN(DAT_003055d0, 0, void (*)(void *, int))((void *)DAT_003055d0, 1);
		DAT_003055d0 = 0;
	}
	MA_I32(self, 0x84) = 0;
	if (MA_I32(self, 0x70) != 0) {
		MA_PRINTF("closing generic meta package\n");
		ma_close_package(self, 0x70);
	}
	if (MA_I32(self, 0x74) != 0) {
		MA_PRINTF("closing meta level package\n");
		ma_close_package(self, 0x74);
	}
	MA_PRINTF("EndMalloc\n");
}

/* 0x0014fe10  Meta/CGameMode_Meta.cpp: Suspend */
void FUN_0014fe10(int self)
{
	MA_U32(self, 0x10) = MA_U32(self, 0x10) & 0xfffffffe;
	MA_PRINTF("meta lost focus\n");
	FUN_0014f630(self);
	FUN_00044b00(9, 0xff, 0);
}

/* 0x0014fe50  Meta/CGameMode_Meta.cpp: DeleteFirstHalf: free the level before the
   next minigame (Popped without the score mode, the debug object and some resets) */
void FUN_0014fe50(int self)
{
	int camera;
	int *heights;
	int i;

	MA_PRINTF("\n\nDeleteFirstHalf\n");
	FUN_0014f630(self);
	ma_free_route_buffers();
	camera = DAT_003055d4;
	if (DAT_003055d4 != 0) {
		FUN_000d64a0(DAT_003055d4);
		MA_DELETE(camera);
		DAT_003055d4 = 0;
	}
	for (i = 0; i < 0x64; i++) {
		heights = (int *)DAT_00309f00[i];
		if (heights != NULL) {
			CLRouteHeights_Dtor((struct CLRouteHeights *)heights);
			MA_DELETE(heights);
			DAT_00309f00[i] = 0;
		}
	}
	if (DAT_002f7a34 != 0) {
		MA_VFN(DAT_002f7a34, 0, void (*)(void *, int))((void *)DAT_002f7a34, 1);
		DAT_002f7a30 = 0;
		DAT_002f7a34 = 0;
	}
	FUN_00152e90();
	FUN_00080bb0();
	COverlay_DeleteAll();
	FUN_00162c10();
	FUN_000b7b80();
	FUN_00162180();
	FUN_00160ee0();
	CProjector_DeleteAll();
	FUN_00161460();
	FUN_00161510();
	FUN_00154270();
	FUN_0015b400();
	FUN_000b84a0();
	FUN_000ba430();
	FUN_0015e080();
	FUN_000b9b80();
	FUN_000af350();
	FUN_00163b20();
	FUN_00163560();
	AnimTex_DeleteList((struct CLFList *)DAT_0030563c);
	FUN_0015d010((ushort *)DAT_00308460);
	Sprite_FreeDefs((struct CSpriteDef *)DAT_002b0d30, 0xb);
	Sprite_FreeDefs((struct CSpriteDef *)s_aisteam1_00290f00, 6);
	FUN_0015b4d0();
	CRayControl_DeleteAll();
	RefBot_Delete();
	FUN_001603b0();
	FUN_0015fcd0();
	FUN_0015f650();
	FUN_0015b650();
	FUN_000be460();
	FUN_0015eeb0();
	FUN_00164b00();
	FUN_00163d80();
	FUN_00163f30();
	FUN_0015c840();
	FUN_0015b9f0();
	FUN_0003c8b0(0);
	SpriteVB_Release((struct CSpriteVB *)DAT_00290ea8);
	ma_free_players();
	if (MA_I32(self, 0x94) != 0) {
		MA_DELETE(MA_PTR(self, 0x94));
		MA_I32(self, 0x94) = 0;
	}
	if (MA_I32(self, 0x70) != 0) {
		MA_PRINTF("closing generic meta package\n");
		ma_close_package(self, 0x70);
	}
	if (MA_I32(self, 0x74) != 0) {
		MA_PRINTF("closing meta level package\n");
		ma_close_package(self, 0x74);
	}
}

/* start minigame `slot` of this level with a random variant (1-4) */
static void ma_start_game(int game_index, int variant)
{
	void *game = (void *)DAT_002ba954[game_index];
	MA_VFN(game, 0xc, void (*)(void *, int))(game, variant);
}

/* the level table (core 0x00283c70, 0x20 bytes per level): +0x0c ushort number of
   minigames, +0x10 per minigame 8 bytes: game index, played flag */
#define MA_LEVEL_GAMES(level) (*(unsigned short *)((char *)&DAT_00283c7c + (level) * 0x20))
#define MA_LEVEL_GAME(level, n) (*(int *)((char *)&DAT_00283c80 + ((n) + (level) * 4) * 8))
#define MA_LEVEL_PLAYED(level, n) (*(int *)((char *)&DAT_00283c84 + ((n) + (level) * 4) * 8))

/* 0x00150110  Meta/CGameMode_Meta.cpp: the round is over: pick the next minigame */
void FUN_00150110(int *self)
{
	int rounds;
	int game;
	int variant;
	int count;
	int i;

	if (DAT_003055d0 != 0) {
		MA_DEBUG_TEXT((void *)DAT_003055d0);
		if (DAT_003055d0 != 0)
			MA_VFN(DAT_003055d0, 0, void (*)(void *, int))((void *)DAT_003055d0, 1);
		DAT_003055d0 = 0;
	}
	self[0x1a] = 7;
	DAT_003055d8 = 0;
	for (i = 0; i < DAT_002bae94; i++) {
		if (DAT_003075bc[i] != 0)
			FUN_00156ba0(DAT_003075bc[i], 0, 0);
	}
	FUN_0014fe50((int)self);
	MA_DEBUG_TEXT(NULL);
	rounds = self[0x20] - 1;
	self[0x20] = rounds;
	self[0x24] = 1;
	count = MA_LEVEL_GAMES(self[0x22]);
	if (count >= 2 && rounds >= 0) {
		if (DAT_003055cc < 1 && DAT_002baea0 == 0) {
			/* a minigame of this level that was not played yet */
			do {
				game = (ma_rand()) % (int)MA_LEVEL_GAMES(self[0x22]);
			} while (MA_LEVEL_PLAYED(self[0x22], game) == 1);
			MA_LEVEL_PLAYED(self[0x22], game) = 1;
			/* a variant that exists (core 0x00283a08: 4 flags per game) */
			do {
				variant = ma_rand() & 3;
			} while (DAT_00283a08[variant + game * 4] == 0);
			ma_start_game(MA_LEVEL_GAME(self[0x22], game), variant + 1);
			return;
		}
		/* Resume (vtable +0x08) */
		MA_VFN(self, 0x8, void (*)(void *))(self);
		return;
	}
	if (0 < DAT_003055cc) {
		MA_VFN(self, 0x8, void (*)(void *))(self);
		return;
	}
	game = ma_rand() % (int)MA_LEVEL_GAMES(self[0x22]);
	variant = ma_rand() & 3;
	ma_start_game(MA_LEVEL_GAME(self[0x22], game), variant + 1);
}

/* 0x00150330  Meta/CGameMode_Meta.cpp: Resume */
void FUN_00150330(int *self)
{
	void *memory;

	DAT_00305570 = DAT_00293928;
	DAT_00293928 = 0xff;
	FUN_00072720((undefined4)DAT_00305630);
	self[4] = self[4] | 1;
	MA_PRINTF("meta gained focus\n");
	if (self[0x1a] == 10) {
		MA_VFN(self, 0x30, void (*)(void *))(self);
		FUN_0014df90((int)self);
		self[0x1a] = 8;
		MA_F32(self, 0x7c) = 60.0f;
		bdSetFieldOfView(1.2216f, 0.0f); /* 0x3f9c61aa */
		CGameModeStack_Find(MA_MODESTACK, "Root");
		if (self[0x22] == 1 && DAT_002bae84 == 0 && DAT_002baeb8 == 0 &&
			MA_I32(DAT_002ecfac, 0x88) == 0) {
			MA_I32(DAT_002ecfac, 0x88) = 1;
			CLMusic_Switch(0x19, 0xff, -0xff, 0xff, 1, 0x20);
		}
		return;
	}
	if (DAT_002baea0 == 0 && DAT_002b056c != 0 && self[0x1b] == 3) {
		if (DAT_002bae98 == 1) {
			CGameModeStack_Pop(MA_MODESTACK, 1);
			return;
		}
		self[0x1b] = 7;
		FUN_00150110(self);
		return;
	}
	if (DAT_003055d0 != 0) {
		MA_DEBUG_TEXT((void *)DAT_003055d0);
		if (DAT_003055d0 != 0)
			MA_VFN(DAT_003055d0, 0, void (*)(void *, int))((void *)DAT_003055d0, 1);
		DAT_003055d0 = 0;
	}
	memory = MA_NEW(0x58, ma_file_gamemode_meta, 0x3c4, 0x1001);
	if (memory == NULL)
		DAT_003055d0 = 0;
	else
		DAT_003055d0 = (int)FUN_00152f30((undefined4 *)memory);
	FUN_00152f50(DAT_003055d0);
}

/* the start positions of the players for the score screen: in the level's start data
   (+0x94 of the game object DAT_0030564c): +0x90 flag per player (winner), winners at
   +0x20, the others at +0x60, 16 bytes each */
static void ma_place_player(int start, int player, int *winner_pos, int *other_pos)
{
	int ch = MA_CHAR(DAT_003075bc[player]);
	int at;

	if (MA_I32(start, 0x90 + player * 4) == 0) {
		at = *other_pos;
		*other_pos += 0x10;
	} else {
		at = *winner_pos;
		*winner_pos += 0x10;
	}
	MA_I32(ch, 0xd0) = MA_I32(start, at);
	MA_I32(ch, 0xd4) = MA_I32(start, at + 4);
	MA_I32(ch, 0xd8) = MA_I32(start, at + 8);
	MA_I32(ch, 0xdc) = MA_I32(start, at + 0xc);
}

/* 0x00150510  Meta/CGameMode_Meta.cpp: the phase state machine */
void FUN_00150510(int self)
{
	int scores[4];
	float centre[4];
	undefined4 view[8];
	int winner;
	int i;
	int ch;
	int start;
	int winner_pos;
	int other_pos;
	int anim;
	void *root;
	void *memory;
	int *mode;
	float t;
	float scale;

	if (MA_I32(self, 0x68) == 8 && MA_I32(DAT_003055d4, 0x568) != 2 && DAT_002b0590 != 0) {
		for (i = 0; i < 4; i++)
			MA_VFN(DAT_003075bc[i], 0x40, void (*)(void *, int, int))((void *)DAT_003075bc[i], 1, 0);
		DAT_002b04e8 = DAT_002b0590;
	}
	if (MA_F32(self, 0x7c) <= 0.0f)
		return;
	t = MA_F32(self, 0x7c) - DAT_0028fda8;
	MA_F32(self, 0x7c) = t;
	if (t > 0.0f)
		return;
	switch (MA_I32(self, 0x68)) {
	case 0:
		if (FUN_0007a900((float *)DAT_002ecfac) == 0)
			goto wait_a_frame;
		MA_F32(self, 0x7c) = DAT_002b057c;
		MA_I32(self, 0x68) = 1;
		root = CGameModeStack_Find(MA_MODESTACK, "Root");
		if (MA_I32(self, 0x88) == 1 && DAT_002bae84 == 0 && MA_I32(root, 0x54) != 0) {
			bsResumeAudioStream((struct BAudioStream *)MA_PTR(root, 0x54), 0x10);
			return;
		}
		break;
	case 1:
		/* time up */
		for (i = 0; i < 4; i++)
			FUN_00156ba0(DAT_003075bc[i], 0, 0);
		scores[0] = DAT_002baee8[0];
		scores[1] = DAT_002baf10[0];
		scores[2] = DAT_002baf38[0];
		scores[3] = DAT_002baf60[0];
		CLSamples_LoadCommentatorSet(6);
		winner = FUN_0007ad50(DAT_002ecfac, scores, 1);
		if (winner != -1)
			CLMusic_Switch(DAT_002baed4[winner * 10], 0xb4, -0xff, 0xb4, 1, 0);
		if (MA_I32(DAT_0030564c, 0x94) == 0) {
			if (-1 < winner) {
				FUN_000d7420(DAT_003055d4, (undefined4 *)MA_I32(DAT_003075bc[winner], 0x30), 1);
				FUN_000af690(MA_CHAR(DAT_003075bc[winner]), 0x1b, 0, 0, 1.0f, 0.2f, 1);
				FUN_000af690(MA_CHAR(DAT_003075bc[winner]), 0x23, 1, 1, 1.0f, 0.3f, 1);
			}
			for (i = 0; i < DAT_002bae94; i++) {
				if (winner != i) {
					FUN_000af690(MA_CHAR(DAT_003075bc[i]), 0x1c, 0, 0, 1.0f, 0.2f, 1);
					FUN_000af690(MA_CHAR(DAT_003075bc[i]), 0x24, 1, 1, 1.0f, 0.03f, 1);
				}
			}
		} else {
			/* the level has a podium: winners and losers to their places */
			for (i = 0; i < DAT_002bae94; i++) {
				if (DAT_002ecf50[i] < 1)
					break;
				MA_I32(MA_PTR(DAT_0030564c, 0x94), 0x8c + DAT_002ecf50[i] * 4) = 1;
			}
			centre[0] = 0.0f;
			centre[1] = 0.0f;
			centre[2] = 0.0f;
			centre[3] = 1.0f;
			other_pos = 0x60;
			winner_pos = 0x20;
			for (i = 0; i < DAT_002bae94; i++) {
				MA_VFN(DAT_003075bc[i], 0x40, void (*)(void *, int, int))((void *)DAT_003075bc[i], 0, 0);
				DAT_002b04e8 = DAT_002b0590;
				start = MA_I32(DAT_0030564c, 0x94);
				if (MA_I32(start, i * 4 + 0x90) == 0) {
					ma_place_player(start, i, &winner_pos, &other_pos);
					FUN_000af690(MA_CHAR(DAT_003075bc[i]), 0x1c, 0, 0, 1.0f, 0.2f, 1);
					anim = 0x24;
				} else {
					ma_place_player(start, i, &winner_pos, &other_pos);
					FUN_000af690(MA_CHAR(DAT_003075bc[i]), 0x1b, 0, 0, 1.0f, 0.2f, 1);
					anim = 0x23;
				}
				FUN_000af690(MA_CHAR(DAT_003075bc[i]), anim, 1, 1, 1.0f, 0.03f, 1);
				ch = MA_CHAR(DAT_003075bc[i]);
				MA_I32(ch, 0x1a4) = MA_I32(ch, 0xd0);
				MA_I32(ch, 0x1a8) = MA_I32(ch, 0xd4);
				MA_I32(ch, 0x1ac) = MA_I32(ch, 0xd8);
				MA_I32(ch, 0x1b0) = MA_I32(ch, 0xdc);
				ch = MA_CHAR(DAT_003075bc[i]);
				MA_I32(ch, 0x1b4) = MA_I32(ch, 0xd0);
				MA_I32(ch, 0x1b8) = MA_I32(ch, 0xd4);
				MA_I32(ch, 0x1bc) = MA_I32(ch, 0xd8);
				MA_I32(ch, 0x1c0) = MA_I32(ch, 0xdc);
				ch = MA_CHAR(DAT_003075bc[i]);
				MA_I32(ch, 0x194) = 0;
				MA_I32(ch, 0x198) = 0;
				MA_I32(ch, 0x19c) = 0;
				MA_F32(ch, 0x1a0) = 1.0f;
				MA_I32(MA_PTR(MA_CHAR(DAT_003075bc[i]), 0x18c), 0x1ac) = 0;
				ch = MA_CHAR(DAT_003075bc[i]);
				centre[0] = centre[0] + MA_F32(ch, 0xd0);
				centre[1] = centre[1] + MA_F32(ch, 0xd4);
				centre[2] = centre[2] + MA_F32(ch, 0xd8);
				FUN_000af970(MA_CHAR(DAT_003075bc[i]), (float *)MA_PTR(DAT_0030564c, 0x94));
				MA_I32(MA_CHAR(DAT_003075bc[i]), 0x1e8) = MA_I32(MA_CHAR(DAT_003075bc[i]), 0x1d8);
			}
			/* camera: from the podium's camera position to the players' centre
			   (8 floats: position, then target) */
			start = MA_I32(DAT_0030564c, 0x94);
			view[0] = MA_U32(start, 0);
			scale = 1.0f / (float)DAT_002bae94; /* 0x00253850 */
			view[1] = MA_U32(start, 4);
			view[2] = MA_U32(start, 8);
			view[3] = MA_U32(start, 0xc);
			centre[0] = centre[0] * scale;
			centre[1] = centre[1] * scale;
			centre[2] = centre[2] * scale;
			memcpy(&view[4], centre, sizeof centre);
			FUN_000d7420(DAT_003055d4, view, 3);
		}
		MA_F32(self, 0x7c) = DAT_002baea8 != 0 ? 390.0f : 240.0f;
		MA_I32(self, 0x68) = 2;
		return;
	case 2:
		FUN_00078bd0(DAT_002ecfac);
		if (DAT_002baea8 == 0) {
			scores[0] = DAT_002baee8[0];
			scores[1] = DAT_002baf10[0];
			scores[2] = DAT_002baf38[0];
			scores[3] = DAT_002baf60[0];
			CScoreTable_AddRound((struct CScoreTable *)&DAT_002b9968, scores);
			CScoreTable_EndRound((struct CScoreTable *)&DAT_002b9968);
			memory = MA_NEW(0x13c, ma_file_gamemode_meta, 0x700, 0x1001);
			mode = memory == NULL ? NULL : (int *)FUN_000199c0((undefined4 *)memory);
			MA_PTR(self, 0x30) = mode;
			MA_VFN(mode, 0x10, void (*)(void *))(mode);
			MA_VFN(MA_PTR(self, 0x30), 0x8, void (*)(void *))(MA_PTR(self, 0x30));
			MA_F32(self, 0x7c) = 600.0f;
			MA_I32(self, 0x68) = 9;
			return;
		}
		goto outro;
	case 3:
		if (DAT_002baea0 == 0 && DAT_002b056c != 0) {
			MA_I32(self, 0x6c) = 3;
			MA_I32(self, 0x2c) = 1;
			return;
		}
		FUN_00150110((int *)self);
		break;
	case 8:
		CLScreen_DrawDarken(0, 1);
		if (MA_I32(DAT_003055d4, 0x568) != 2) {
			if (DAT_002baeb8 != 0) {
				MA_I32(self, 0x68) = 1;
				MA_F32(self, 0x7c) = DAT_002b057c;
				return;
			}
			MA_F32(self, 0x7c) = 1.0f;
			MA_I32(self, 0x68) = 0;
			FUN_00078a70((float *)DAT_002ecfac, 280.0f);
			return;
		}
	wait_a_frame:
		MA_F32(self, 0x7c) = 1.0f;
		return;
	case 9:
		MA_VFN(MA_PTR(self, 0x30), 0x4, void (*)(void *))(MA_PTR(self, 0x30));
		MA_VFN(MA_PTR(self, 0x30), 0xc, void (*)(void *))(MA_PTR(self, 0x30));
		if (MA_PTR(self, 0x30) != NULL)
			MA_VFN(MA_PTR(self, 0x30), 0x0, void (*)(void *, int))(MA_PTR(self, 0x30), 1);
		MA_PTR(self, 0x30) = NULL;
	outro:
		MA_F32(self, 0x7c) = 60.0f;
		MA_I32(self, 0x68) = 3;
		return;
	default:
		break;
	}
}

/* the 0x34-byte light/particle description at +0xb0 that Update passes by value */
struct ma_by_value_0x34 {
	unsigned int words[13];
};

/* 0x00150ce0  Meta/CGameMode_Meta.cpp: Update(active) */
void FUN_00150ce0(int self, int active)
{
	int start;
	int winner_pos;
	int other_pos;
	int i;
	struct ma_by_value_0x34 copy;

	if (active == 0)
		return;
	DAT_00294b58 = DAT_002b0594;
	FUN_00044b00(9, 6, 0);
	DAT_00294b54 = 4000.0f;
	if (DAT_00283ea4 != DAT_003045b8) {
		DAT_003045b8 = DAT_00283ea4;
		for (i = 0; i < DAT_002bae94; i++)
			Core_SetCharacterColours((void *)(MA_CHAR(DAT_002f289c[i]) + 0x10), DAT_00283ea4);
	}
	if (DAT_002b04e8 != DAT_002b0590) {
		for (i = 0; i < 4; i++)
			MA_VFN(DAT_002f289c[i], 0x40, void (*)(void *, int, int))((void *)DAT_002f289c[i],
				1 - DAT_002b0590, 0);
		DAT_002b04e8 = DAT_002b0590;
	}
	FUN_00080770(); /* (thiscall on the rumble object 0x002ed324; no parameter used) */
	if (DAT_002ecdec == 0) {
		FUN_0014ea80();
		FUN_00150510(self);
		if (MA_I32(self, 0x68) != 9)
			FUN_0014f520(self);
		FUN_00164990();
		if (MA_I32(self, 0x88) == 4)
			FUN_0015c3c0();
		if (DAT_003055d8 != 0) {
			MA_I32(self, 0xb0) = DAT_0030556c;
			MA_I32(self, 0xb4) = DAT_00305568;
			MA_U8(self, 0xb8) = (unsigned char)DAT_003045b4;
			MA_I32(self, 0xbc) = (int)DAT_003045c4;
			MA_I32(self, 0xc0) = (int)DAT_00304570;
			MA_F32(self, 0xc4) = -100.0f;
			MA_F32(self, 0xc8) = -320.0f;
			MA_F32(self, 0xcc) = -40.0f;
			MA_I32(self, 0xd4) = 0;
			MA_I32(self, 0xd8) = 0;
			MA_I32(self, 0xdc) = 0;
			for (i = 0xc; i != 0; i--) {
				memcpy(&copy, MA_ADDR(self, 0xb0), sizeof copy);
				((undefined4 (*)(int, struct ma_by_value_0x34))FUN_0003c920)(MA_I32(self, 0xa0), copy);
			}
			FUN_00163d40();
		}
		FUN_00164730((int)MA_ADDR(self, 0xa0));
		if (MA_I32(self, 0x68) == 9) {
			MA_VFN(MA_PTR(self, 0x30), 0x18, void (*)(void *, int))(MA_PTR(self, 0x30), 1);
			if (MA_U8(MA_PTR(self, 0x30), 0x138) == 0)
				MA_F32(self, 0x7c) = 1000.0f;
			else
				MA_F32(self, 0x7c) = 1.0f;
		}
		for (i = 0; i < DAT_002bae94; i++) {
			if (MA_I32(self, 0x88) == 5)
				FUN_00163120(DAT_003075bc[i], 0.0f, 0.0f);
		}
	}
	if (MA_I32(self, 0x68) != 9 && MA_I32(self, 0x68) != 3) {
		if (FUN_000772e0((undefined4 *)DAT_002ecd90) != 0)
			return;
	}
	if ((MA_I32(self, 0x68) == 2 || MA_I32(self, 0x68) == 9 || MA_I32(self, 0x68) == 3)) {
		other_pos = 0x60;
		winner_pos = 0x20;
		for (i = 0; i < DAT_002bae94; i++) {
			start = MA_I32(DAT_0030564c, 0x94);
			ma_place_player(start, i, &winner_pos, &other_pos);
		}
	}
	if (DAT_003055e0 != 0 && MA_I32(self, 0x68) == 1 && MA_F32(self, 0x7c) <= 4500.0f) {
		/* demo: leave after 4500 frames */
		CGameModeStack_Pop(MA_MODESTACK, 1);
		ma_start_game(0, 0);
	}
}
