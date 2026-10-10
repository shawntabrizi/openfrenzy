/*
Drillers/CDrillers_Resources.c  <-  C:\BlitzParty\Game\Src\Drillers\CDrillers_Resources.cpp

The resources object (one static object at 0x002aa900, vtable 0x0026c5a0): it loads and
frees what a variant needs (the scenery .lom, the map columns, fonts, the bikes and the
characters, coins, the arena, sounds, textures, lights) and the tuning menus. Fields at
original offsets:
  +0x008 the variant package (0x002aa908)
  +0x00c, +0x010, +0x014 the red, green and blue bytes of the four players' characters
  +0x018 .. +0x03c arena textures (+0x034 .. the layers)
  +0x058 .. +0x07c effect textures (steam +0x064, corona +0x068, scorch +0x06c, the
         snare's band +0x070, the scenery's animated screen +0x074, sparks +0x078,
         dust +0x07c)
  +0x084, +0x088, +0x08c fonts   +0x090 the package id
  +0x094 .. +0x0a0 the player lights, +0x0b0 the camera light
  +0x0b4 .. +0x0f4 samples   +0x158, +0x15c the "ai menu" and "misc. menu" tweak menus

0x0011e1e0 names this file in its allocations; 0x0011e140, 0x0011e1c0 and 0x0011f770 work on
the same object (file inferred). The address 0x0011f1c0 in the drillers tool's entry list is
a switch case of 0x0011e1e0, not a function.
*/

#include "drillers.h"

#define RES(off, type) DR_AT(self, off, type)
#define DR_PACKAGE_OF(self) RES(0x8, struct bkPackage *)

/* the character table of classlib_ui (0x60 bytes per character; +0x08 the portrait) */
extern unsigned char DAT_00290700[];
/* the character colour set (core) */
extern int DAT_00283ea4;
/* the collision epsilon (engine_misc) */
extern float DAT_00285644;
/* the demo camera of bumpercars_b */
extern void *DAT_002f26c4;

#define DR_LOM DR_PTR(void, 0x002a54d8u)
#define DR_RES_ID(off) DR_VAR(struct bkResource *, off)
#define DR_INST(addr) DR_VAR(struct BActorInstance *, addr)

/* 0x0026c5a0  vtable of the resources object (rebuilt from the XBE) */
const void *const vtable_0026c5a0[1] = {
	(const void *)DrRes_DeletingDtor, /* +0x00 */
};

/* 0x0011e140  Drillers (gap; CDrillers_Resources.cpp inferred)
Puts the render states back to the game's defaults. */
void DrRes_ResetStates(void *self)
{
	(void)self;
	bdSetProjectionMode(1);
	bdSetTexture(0, NULL);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(8, 0, 0);
	bdSetRenderState(9, 0xff, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(3, 0, 0);
}

/* 0x0011e1c0  Drillers (gap; CDrillers_Resources.cpp inferred; vtable 0x0026c5a0 +0x00) */
void *DrRes_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026c5a0;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* a resource of the variant package */
static struct bkResource *dr_load(void *self, const char *name, int type)
{
	return bLoadResource(DR_PACKAGE_OF(self), name, type, 0xdefa);
}

/* an actor instance of `actor`: prepared, at animation time `time` */
static struct BActorInstance *dr_instance(struct bkResource *actor, int slots, int line, int time)
{
	struct BActorInstance *inst;

	inst = baInitActorInstance(NULL, (struct BActor *)actor, slots, DR_FILE_RESOURCES, line);
	baPrepActorInstance(inst, 1);
	baSetNodeTime(inst, DR_AT(inst, 0x134, struct BActorNode *), time, 1);
	return inst;
}

/* 0x0011e1e0  Drillers/CDrillers_Resources.cpp
Loads the variant: the tuning menus and values, the fog of the variant, the players'
colours, the package's resources, the scenery (.lom of the variant's folder), the map
columns (variants 1, 2), fonts, the bikes ("<letter>bike" / "<letter>jetbike" by
character) and the characters, coins (variants 2, 4), the arena surface (3, 4), Erodeo's
swing bit, the sounds and textures of the variant, the character portraits, the lights. */
void DrRes_Load(void *self)
{
	static const char letters[6] = { 'g', 'r', 'b', 'y', 'o', 'p' };
	static const char *const lomNames[4] = {
		"driller\\driller.lom", "token\\token.lom", "s_trails\\s_trails.lom", "snare\\snare.lom",
	};
	char bikes[4][40];
	float dir[4];
	struct CTweakMenu *menu;
	struct BLight **lights;
	int character;
	int v;
	int i;

	for (i = 0; i < 16; i++)
		sprintf(DR_PTR(char, 0x002aacb0u + i * 0x14), "%s", "");
	DR_VAR(int, 0x002aaba8u) = 1;
	v = g_DrVariant;
	switch (v) {
	case 1:
		DR_VAR(float, 0x00301c2cu) = 67.0f;
		DR_VAR(float, 0x00301c3cu) = 60.0f;
		DR_VAR(float, 0x00301c4cu) = 54.0f;
		DR_VAR(float, 0x00301c5cu) = 30000.0f;
		DR_VAR(float, 0x00301c6cu) = 90000.0f;
		break;
	case 2:
		DR_VAR(float, 0x00301c30u) = 67.0f;
		DR_VAR(float, 0x00301c40u) = 60.0f;
		DR_VAR(float, 0x00301c50u) = 54.0f;
		DR_VAR(float, 0x00301c60u) = 15000.0f;
		DR_VAR(float, 0x00301c70u) = 85000.0f;
		break;
	case 3:
		DR_VAR(float, 0x00301c34u) = 45.0f;
		DR_VAR(float, 0x00301c44u) = 30.0f;
		DR_VAR(float, 0x00301c54u) = 75.0f;
		DR_VAR(float, 0x00301c64u) = 15000.0f;
		DR_VAR(float, 0x00301c74u) = 85000.0f;
		break;
	case 4:
		DR_VAR(float, 0x00301c38u) = 45.0f;
		DR_VAR(float, 0x00301c48u) = 30.0f;
		DR_VAR(float, 0x00301c58u) = 75.0f;
		DR_VAR(float, 0x00301c68u) = 15000.0f;
		DR_VAR(float, 0x00301c78u) = 85000.0f;
		break;
	}
	menu = (struct CTweakMenu *)DR_NEW(0xe8, DR_FILE_RESOURCES, 0xa3, 0x1001);
	RES(0x158, struct CTweakMenu *) = menu != NULL ? CTweakMenu_Ctor(menu, "ai menu") : NULL;
	menu = (struct CTweakMenu *)DR_NEW(0xe8, DR_FILE_RESOURCES, 0xa4, 0x1001);
	RES(0x15c, struct CTweakMenu *) = menu != NULL ? CTweakMenu_Ctor(menu, "misc. menu") : NULL;
	menu = RES(0x158, struct CTweakMenu *);
	if (DR_VAR(float, 0x002aadfcu) != 0.0f)
		CTweakMenu_AddFloat(menu, "aggressiveness", DR_PTR(float, 0x002aadfcu), 0.05f, 0.0f, 1.0f);
	if (DR_VAR(float, 0x002aae00u) != 0.0f)
		CTweakMenu_AddFloat(menu, "vindictiveness", DR_PTR(float, 0x002aae00u), 0.05f, 0.0f, 1.0f);
	if (DR_VAR(float, 0x002aae04u) != 0.0f)
		CTweakMenu_AddFloat(menu, "greediness", DR_PTR(float, 0x002aae04u), 0.05f, 0.0f, 1.0f);
	if (DR_VAR(float, 0x002aae08u) != 0.0f)
		CTweakMenu_AddFloat(menu, "kleptomania", DR_PTR(float, 0x002aae08u), 0.05f, 0.0f, 1.0f);
	if (DR_VAR(float, 0x002aae0cu) != 0.0f)
		CTweakMenu_AddFloat(menu, "cleverness", DR_PTR(float, 0x002aae0cu), 0.05f, 0.0f, 1.0f);
	menu = RES(0x15c, struct CTweakMenu *);
	CTweakMenu_AddFloat(menu, "draw timers", DR_PTR(float, 0x002aae34u), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "draw grid", DR_PTR(float, 0x002aae30u), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "draw men", DR_PTR(float, 0x002aae28u), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "draw bike", DR_PTR(float, 0x002aae2cu), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "quick tiles", DR_PTR(float, 0x002aae38u), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "detail", DR_PTR(float, 0x002aae20u), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "fps", DR_PTR(float, 0x002aae24u), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "game speed", DR_PTR(float, 0x002aae10u), 0.05f, 0.1f, 2.0f);
	CTweakMenu_AddFloat(menu, "max.tokens", DR_PTR(float, 0x002aae14u), 1.0f, 1.0f, 22.0f);
	CTweakMenu_AddFloat(menu, "jump(0) boost(1)", DR_PTR(float, 0x002aae1cu), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "fog r", DR_PTR(float, 0x00301c28u + g_DrVariant * 4), 1.0f, 0.0f,
		255.0f);
	CTweakMenu_AddFloat(menu, "fog g", DR_PTR(float, 0x00301c38u + g_DrVariant * 4), 1.0f, 0.0f,
		255.0f);
	CTweakMenu_AddFloat(menu, "fog b", DR_PTR(float, 0x00301c48u + g_DrVariant * 4), 1.0f, 0.0f,
		255.0f);
	CTweakMenu_AddFloat(menu, "fog near", DR_PTR(float, 0x00301c58u + g_DrVariant * 4), 100.0f,
		100.0f, 100000.0f);
	CTweakMenu_AddFloat(menu, "fog far", DR_PTR(float, 0x00301c68u + g_DrVariant * 4), 100.0f,
		100.0f, 100000.0f);
	CTweakMenu_AddFloat(menu, "erodeo test", DR_PTR(float, 0x002aae3cu), 1.0f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(menu, "shield test", DR_PTR(float, 0x002aae40u), 1.0f, 0.0f, 1.0f);
	if (DR_VAR(float, 0x002aae18u) != 0.0f)
		CTweakMenu_AddFloat(menu, "turn speed", DR_PTR(float, 0x002aae18u), 1.0f, 8.0f, 64.0f);
	CLRumble_CreateMenu((int *)DAT_002ed324);
	for (i = 0; i < 4; i++) {
		character = DAT_002baed4[i * 10];
		RES(0xc + i, unsigned char) = DAT_00290718[character * 0x60];
		RES(0x10 + i, unsigned char) = DAT_00290718[character * 0x60 + 1];
		RES(0x14 + i, unsigned char) = DAT_00290718[character * 0x60 + 2];
	}
	RES(0x90, unsigned int) = bkGetPackageId(DR_PACKAGE_OF(self));
	bLoadPackageResources(DR_PACKAGE_OF(self), 1, 0xdefa, 0);
	if (1 <= g_DrVariant && g_DrVariant <= 4)
		DrLom_Load(DR_LOM, DR_PACKAGE_OF(self), lomNames[g_DrVariant - 1]);
	if (g_DrVariant < 3) {
		for (i = 0; i < 42; i++)
			DR_VAR(void *, 0x00298da4u + i * 4) = DR_NEW(0x7770, DR_FILE_RESOURCES, 0xe8, 0x2001);
	}
	RES(0x84, struct bkResource *) = bLoadResource(NULL, "handgotb.ttf", 3, 0);
	RES(0x88, struct bkResource *) = bLoadResource(NULL, "handgotl.ttf", 3, 0);
	if (g_DrVariant == 2 || g_DrVariant == 4)
		RES(0x8c, struct bkResource *) = bLoadResource(DR_PACKAGE_OF(self), "fonts\\floaty.ttf",
			3, 0);
	for (i = 0; i < 4; i++)
		sprintf(bikes[i], g_DrVariant < 3 ? "models\\%cbike.obe" : "models\\%cjetbike.obe",
			(int)letters[DAT_002baed4[i * 10]]);
	for (i = 0; i < 4; i++)
		DR_PF(i, 0x3c, struct bkResource *) = dr_load(self, bikes[i], 1);
	for (i = 0; i < 4; i++) {
		character = DAT_002baed4[i * 10];
		DR_PF(i, 0x44, struct bkResource *) = dr_load(self,
			DR_VAR(const char *, 0x002aa8a0u + character * 0x10), 1);
		DR_PF(i, 0x48, struct bkResource *) = dr_load(self,
			DR_VAR(const char *, 0x002aa8a8u + character * 0x10), 1);
	}
	for (i = 0; i < 4; i++) {
		DR_PF(i, 0x4c, struct BActorInstance *) =
			dr_instance(DR_PF(i, 0x3c, struct bkResource *), 0, 0x10f, 0);
		DR_PF(i, 0x54, struct BActorInstance *) =
			dr_instance(DR_PF(i, 0x44, struct bkResource *), 1, 0x113, 0x400);
		DR_PF(i, 0x58, struct BActorInstance *) =
			dr_instance(DR_PF(i, 0x48, struct bkResource *), 1, 0x117, 0x400);
		DR_PF(i, 0x50, struct BActorInstance *) = DR_PF(i, 0x58, struct BActorInstance *);
		DR_PF(i, 0x40, struct bkResource *) = DR_PF(i, 0x48, struct bkResource *);
		baSetInstanceAnimations(DR_PF(i, 0x44, struct BActor *),
			DR_PF(i, 0x58, struct BActorInstance *));
		baPrepActorInstance(DR_PF(i, 0x50, struct BActorInstance *), 1);
		baSetNodeTime(DR_PF(i, 0x50, struct BActorInstance *),
			DR_AT(DR_PF(i, 0x50, char *), 0x134, struct BActorNode *), 0x400, 1);
		if (DAT_00283ea4 != 0) {
			Core_SetCharacterColours(DR_PF(i, 0x50, void *), DAT_00283ea4);
			Core_SetCharacterColours(DR_PF(i, 0x54, void *), DAT_00283ea4);
		}
	}
	if (g_DrVariant == 2 || g_DrVariant == 4) {
		DR_RES_ID(0x00301480u) = dr_load(self, "models\\bcoin.obe", 1);
		DR_RES_ID(0x00301484u) = dr_load(self, "models\\scoin.obe", 1);
		DR_RES_ID(0x00301488u) = dr_load(self, "models\\gcoin.obe", 1);
		for (i = 0; i < 3; i++)
			DR_INST(0x0030148cu + i * 4) = dr_instance(DR_RES_ID(0x00301480u + i * 4), 0, 0x143, 0);
	}
	if (2 < g_DrVariant) {
		DR_RES_ID(0x002aaae8u) = dr_load(self, "city-obj\\surface.obe", 1);
		DR_INST(0x002aaaecu) = dr_instance(DR_RES_ID(0x002aaae8u), 0, 0x14c, 0);
	}
	if (g_DrVariant == 2) {
		DR_RES_ID(0x002aaaf0u) = dr_load(self, "models\\swingbit.obe", 1);
		DR_INST(0x002aaaf4u) = dr_instance(DR_RES_ID(0x002aaaf0u), 0, 0x154, 0);
	}
	switch (g_DrVariant) {
	case 1:
		RES(0xb8, struct bkResource *) = dr_load(self, "sfx\\drillerbike1.wav", 2);
		RES(0xbc, struct bkResource *) = dr_load(self, "sfx\\drillv1_02a.wav", 2);
		RES(0xc0, struct bkResource *) = dr_load(self, "sfx\\drillv1_03c.wav", 2);
		RES(0xc8, struct bkResource *) = dr_load(self, "sfx\\driller_02e.wav", 2);
		RES(0xd0, struct bkResource *) = dr_load(self, "sfx\\gen_token_02b.wav", 2);
		RES(0xd4, struct bkResource *) = dr_load(self, "sfx\\streetamb1.wav", 2);
		RES(0xd8, struct bkResource *) = dr_load(self, "sfx\\horn1.wav", 2);
		RES(0xdc, struct bkResource *) = dr_load(self, "sfx\\horn2.wav", 2);
		RES(0xe0, struct bkResource *) = dr_load(self, "sfx\\horn3.wav", 2);
		RES(0xe4, struct bkResource *) = dr_load(self, "sfx\\horn4.wav", 2);
		RES(0xe8, struct bkResource *) = dr_load(self, "sfx\\horn5.wav", 2);
		RES(0xec, struct bkResource *) = dr_load(self, "sfx\\horn7.wav", 2);
		RES(0xf0, struct bkResource *) = dr_load(self, "sfx\\blimp1.wav", 2);
		RES(0xf4, struct bkResource *) = dr_load(self, "sfx\\drillv2_03a.wav", 2);
		break;
	case 2:
		RES(0xb4, struct bkResource *) = dr_load(self, "sfx\\jetboats_19a.wav", 2);
		RES(0xb8, struct bkResource *) = dr_load(self, "sfx\\drillerbike1.wav", 2);
		RES(0xbc, struct bkResource *) = dr_load(self, "sfx\\drillv1_02a.wav", 2);
		RES(0xc0, struct bkResource *) = dr_load(self, "sfx\\drillv1_03c.wav", 2);
		RES(0xc8, struct bkResource *) = dr_load(self, "sfx\\driller_02e.wav", 2);
		RES(0xd4, struct bkResource *) = dr_load(self, "sfx\\streetamb1.wav", 2);
		RES(0xd8, struct bkResource *) = dr_load(self, "sfx\\horn1.wav", 2);
		RES(0xdc, struct bkResource *) = dr_load(self, "sfx\\horn2.wav", 2);
		RES(0xe0, struct bkResource *) = dr_load(self, "sfx\\horn3.wav", 2);
		RES(0xe4, struct bkResource *) = dr_load(self, "sfx\\horn4.wav", 2);
		RES(0xe8, struct bkResource *) = dr_load(self, "sfx\\horn5.wav", 2);
		RES(0xec, struct bkResource *) = dr_load(self, "sfx\\horn7.wav", 2);
		RES(0xf4, struct bkResource *) = dr_load(self, "sfx\\drillv2_03a.wav", 2);
		break;
	case 3:
		RES(0xb8, struct bkResource *) = dr_load(self, "sfx\\drillv1_01a.wav", 2);
		RES(0xc8, struct bkResource *) = dr_load(self, "sfx\\driller_02e.wav", 2);
		RES(0xcc, struct bkResource *) = dr_load(self, "sfx\\drillv3_01a.wav", 2);
		RES(0xd0, struct bkResource *) = dr_load(self, "sfx\\gen_token_02b.wav", 2);
		RES(0xd4, struct bkResource *) = dr_load(self, "sfx\\streetamb1.wav", 2);
		RES(0xd8, struct bkResource *) = dr_load(self, "sfx\\horn1.wav", 2);
		RES(0xdc, struct bkResource *) = dr_load(self, "sfx\\horn2.wav", 2);
		RES(0xe0, struct bkResource *) = dr_load(self, "sfx\\horn3.wav", 2);
		RES(0xe4, struct bkResource *) = dr_load(self, "sfx\\horn4.wav", 2);
		RES(0xe8, struct bkResource *) = dr_load(self, "sfx\\horn5.wav", 2);
		RES(0xec, struct bkResource *) = dr_load(self, "sfx\\horn7.wav", 2);
		break;
	case 4:
		RES(0xb8, struct bkResource *) = dr_load(self, "sfx\\drillv1_01a.wav", 2);
		RES(0xb4, struct bkResource *) = dr_load(self, "sfx\\jetboats_19a.wav", 2);
		RES(0xc4, struct bkResource *) = dr_load(self, "sfx\\drillv4_01a.wav", 2);
		RES(0xc8, struct bkResource *) = dr_load(self, "sfx\\driller_02e.wav", 2);
		RES(0xd4, struct bkResource *) = dr_load(self, "sfx\\streetamb1.wav", 2);
		RES(0xd8, struct bkResource *) = dr_load(self, "sfx\\horn1.wav", 2);
		RES(0xdc, struct bkResource *) = dr_load(self, "sfx\\horn2.wav", 2);
		RES(0xe0, struct bkResource *) = dr_load(self, "sfx\\horn3.wav", 2);
		RES(0xe4, struct bkResource *) = dr_load(self, "sfx\\horn4.wav", 2);
		RES(0xe8, struct bkResource *) = dr_load(self, "sfx\\horn5.wav", 2);
		RES(0xec, struct bkResource *) = dr_load(self, "sfx\\horn7.wav", 2);
		break;
	}
	RES(0x18, struct bkResource *) = bLoadResource(NULL, "arenatex\\sbsetile.bmp", 0, 0xdefa);
	RES(0x1c, struct bkResource *) = bLoadResource(NULL, "arenatex\\stilec1n.bmp", 0, 0xdefa);
	RES(0x20, struct bkResource *) = bLoadResource(NULL, "arenatex\\stilec2n.bmp", 0, 0xdefa);
	RES(0x24, struct bkResource *) = bLoadResource(NULL, "arenatex\\stilec3n.bmp", 0, 0xdefa);
	RES(0x28, struct bkResource *) = bLoadResource(NULL, "arenatex\\stilec1e.bmp", 0, 0xdefa);
	RES(0x2c, struct bkResource *) = bLoadResource(NULL, "arenatex\\stilec2e.bmp", 0, 0xdefa);
	RES(0x30, struct bkResource *) = bLoadResource(NULL, "arenatex\\stilec3e.bmp", 0, 0xdefa);
	if (g_DrVariant == 1) {
		RES(0x34, struct bkResource *) = bLoadResource(NULL, "arenatex\\tex_1_1.bmp", 0, 0xdefa);
		RES(0x38, struct bkResource *) = bLoadResource(NULL, "arenatex\\tex_1_2.bmp", 0, 0xdefa);
		RES(0x3c, struct bkResource *) = bLoadResource(NULL, "arenatex\\tex_1_3.bmp", 0, 0xdefa);
	} else if (g_DrVariant == 2) {
		RES(0x34, struct bkResource *) = bLoadResource(NULL, "arenatex\\textures2.bmp", 0,
			0xdefa);
	}
	for (i = 0; i < 4; i++) {
		char *name = bikes[i];  /* the original reuses the bike name buffers (0x28 apart) */

		sprintf(name, "chartex\\%s",
			*(const char *const *)(DAT_00290700 + DAT_002baed4[i * 10] * 0x60));
		DR_VAR(struct bkResource *, 0x002aab10u + i * 4) = bLoadResource(NULL, name, 0, 0xdefa);
		DR_VAR(int, 0x002aab20u + i * 0x10) = 0;
		DR_VAR(int, 0x002aab24u + i * 0x10) = 0;
		DR_VAR(int, 0x002aab28u + i * 0x10) = 0;
		DR_VAR(int, 0x002aab2cu + i * 0x10) = 0;
		DR_VAR(int, 0x002aab30u + i * 0x10) = 0;
	}
	RES(0x74, struct bkResource *) = bLoadResource(NULL, "ciry-tex\\vidscreen.gif", 0, 0xdefa);
	RES(0x58, struct bkResource *) = bLoadResource(NULL, "effects\\smoke.bmp", 0, 0xdefa);
	RES(0x5c, struct bkResource *) = bLoadResource(NULL, "effects\\coinshdi.bmp", 0, 0xdefa);
	RES(0x60, struct bkResource *) = bLoadResource(NULL, "effects\\shock.bmp", 0, 0xdefa);
	RES(0x64, struct bkResource *) = bLoadResource(NULL, "effects\\steam_01.bmp", 0, 0xdefa);
	RES(0x68, struct bkResource *) = bLoadResource(NULL, "effects\\corona01.bmp", 0, 0xdefa);
	if (2 < g_DrVariant)
		RES(0x6c, struct bkResource *) = bLoadResource(NULL, "effects\\scorch.bmp", 0, 0xdefa);
	if (g_DrVariant == 4)
		RES(0x70, struct bkResource *) = bLoadResource(NULL, "effects\\base1.bmp", 0, 0xdefa);
	RES(0x78, struct bkResource *) = bLoadResource(NULL, "effects\\sparkburst01.bmp", 0, 0xdefa);
	RES(0x7c, struct bkResource *) = bLoadResource(NULL, "effects\\explosion.bmp", 0, 0xdefa);
	lights = &RES(0x94, struct BLight *);
	lights[0] = baCreateLight(NULL, 0);
	RES(0x98, struct BLight *) = baCreateLight(NULL, 0);
	RES(0x9c, struct BLight *) = baCreateLight(NULL, 0);
	RES(0xa0, struct BLight *) = baCreateLight(NULL, 0);
	RES(0xb0, struct BLight *) = baCreateLight(NULL, 0);
	baEnableLight(lights[0], 1);
	baEnableLight(RES(0x98, struct BLight *), 2 < g_DrVariant);
	baEnableLight(RES(0x9c, struct BLight *), 2 < g_DrVariant);
	baEnableLight(RES(0xa0, struct BLight *), 2 < g_DrVariant);
	baEnableLight(RES(0xb0, struct BLight *), 0);
	if (g_DrVariant < 3) {
		dir[0] = -1.0f;
		dir[1] = -0.5f;
		dir[2] = 0.0f;
		dir[3] = 1.0f;
		baSetDirectionalLight(lights[0], dir);
		baSetLightColour(lights[0], 0x40, 0x40, 0x40);
	} else {
		dir[2] = 0.0f;
		dir[1] = 0.0f;
		dir[0] = 0.0f;
		dir[3] = 1.0f;
		for (i = 0; i < 4; i++) {
			character = DAT_002baed4[i * 10];
			baSetPointLight(lights[i], dir, 0.00125f);
			baSetLightColour(lights[i], DAT_00290718[character * 0x60],
				DAT_00290718[character * 0x60 + 1], DAT_00290718[character * 0x60 + 2]);
		}
		dir[0] = 0.0f;
		dir[1] = 1.0f;
		dir[2] = 0.0f;
		dir[3] = 1.0f;
		baSetDirectionalLight(RES(0xb0, struct BLight *), dir);
		baSetLightColour(RES(0xb0, struct BLight *), 0x7f, 0x7f, 0x7f);
	}
}

/* 0x0011f770  Drillers (gap; CDrillers_Resources.cpp inferred)
Frees the variant: palettes, the demo camera, the referee robot, the effects' particle
systems, the scenery, the projectors, the tweak menus, the rumble, the map columns, the
tokens; the render and texture states back to defaults; the actor instances and
resources, the reserved channels, the sounds, the floaty font, the package's resources,
the extra systems and the lights; the package. */
void DrRes_Free(void *self)
{
	struct CTweakMenu *menu;
	void *camera;
	int i;

	for (i = 0; i < 4; i++)
		D3DDevice_SetPalette((unsigned long)i, NULL);
	camera = DAT_002f26c4;
	if (camera != NULL) {
		BCarFreeCamera_Destruct((struct BCarFreeCamera *)camera);
		DR_DELETE(camera);
	}
	RefBot_Delete();
	CLDebris_Start();
	DAT_00285644 = 0.001f;
	DrFxGroup_Release(&g_DrFx->groups[0]);
	DrFxGroup_Release(&g_DrFx->groups[1]);
	DrFxGroup_Release(&g_DrFx->groups[2]);
	DrFxGroup_Release(&g_DrFx->groups[3]);
	DrEffects_ReleaseSparks(g_DrFx);
	DrLom_Free(DR_LOM);
	CProjector_DeleteAll();
	menu = RES(0x158, struct CTweakMenu *);
	if (menu != NULL) {
		CTweakMenu_Dtor(menu);
		DR_DELETE(menu);
	}
	menu = RES(0x15c, struct CTweakMenu *);
	if (menu != NULL) {
		CTweakMenu_Dtor(menu);
		DR_DELETE(menu);
	}
	CLRumble_StopAll((int)(uintptr_t)DAT_002ed324);
	CLRumble_Shutdown((int *)DAT_002ed324);
	if (g_DrVariant < 3) {
		for (i = 0; i < 42; i++) {
			DR_DELETE(DR_VAR(void *, 0x00298da4u + i * 4));
			DR_VAR(void *, 0x00298da4u + i * 4) = NULL;
		}
	}
	CToken_DeleteAll();
	DrRes_ResetStates(self);
	bdSetTexture(0, NULL);
	bdSetTexture(1, NULL);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0);
	D3DDevice_SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLOROP, 1);
	for (i = 0; i < 4; i++) {
		baDeleteActorInstance(DR_PF(i, 0x4c, struct BActorInstance *));
		baDeleteActorInstance(DR_PF(i, 0x54, struct BActorInstance *));
		baDeleteActorInstance(DR_PF(i, 0x58, struct BActorInstance *));
	}
	if (g_DrVariant == 2)
		baDeleteActorInstance(DR_INST(0x002aaaf4u));
	else if (1 < g_DrVariant)
		baDeleteActorInstance(DR_INST(0x002aaaecu));
	if (g_DrVariant == 2 || g_DrVariant == 4) {
		baDeleteActorInstance(DR_INST(0x0030148cu));
		baDeleteActorInstance(DR_INST(0x00301490u));
		baDeleteActorInstance(DR_INST(0x00301494u));
	}
	for (i = 0; i < 4; i++) {
		bkDeleteResource(DR_PF(i, 0x3c, struct bkResource *));
		bkDeleteResource(DR_PF(i, 0x44, struct bkResource *));
		bkDeleteResource(DR_PF(i, 0x48, struct bkResource *));
	}
	if (g_DrVariant == 2)
		bkDeleteResource(DR_RES_ID(0x002aaaf0u));
	else if (1 < g_DrVariant)
		bkDeleteResource(DR_RES_ID(0x002aaae8u));
	if (g_DrVariant == 2 || g_DrVariant == 4) {
		bkDeleteResource(DR_RES_ID(0x00301480u));
		bkDeleteResource(DR_RES_ID(0x00301484u));
		bkDeleteResource(DR_RES_ID(0x00301488u));
	}
	bsStopSample(DR_VAR(struct BSample *, 0x002aa9d4u));
	bsReserveChannel(DR_VAR(int, 0x002aaa04u), 0);
	bsReserveChannel(DR_VAR(int, 0x002aaa08u), 0);
	bsReserveChannel(DR_VAR(int, 0x002aaa0cu), 0);
	bsReserveChannel(DR_VAR(int, 0x002aaa10u), 0);
	bsReserveChannel(DR_VAR(int, 0x002aaa40u), 0);
	bsReserveChannel(DR_VAR(int, 0x002aaa44u), 0);
	switch (g_DrVariant) {
	case 1:
		bkDeleteResource(RES(0xb8, struct bkResource *));
		bkDeleteResource(RES(0xbc, struct bkResource *));
		bkDeleteResource(RES(0xc0, struct bkResource *));
		bkDeleteResource(RES(0xc8, struct bkResource *));
		bkDeleteResource(RES(0xd0, struct bkResource *));
		bkDeleteResource(RES(0xd4, struct bkResource *));
		bkDeleteResource(RES(0xd8, struct bkResource *));
		bkDeleteResource(RES(0xdc, struct bkResource *));
		bkDeleteResource(RES(0xe0, struct bkResource *));
		bkDeleteResource(RES(0xe4, struct bkResource *));
		bkDeleteResource(RES(0xe8, struct bkResource *));
		bkDeleteResource(RES(0xec, struct bkResource *));
		bkDeleteResource(RES(0xf0, struct bkResource *));
		bkDeleteResource(RES(0xf4, struct bkResource *));
		break;
	case 2:
		bkDeleteResource(RES(0xb4, struct bkResource *));
		bkDeleteResource(RES(0xb8, struct bkResource *));
		bkDeleteResource(RES(0xbc, struct bkResource *));
		bkDeleteResource(RES(0xc0, struct bkResource *));
		bkDeleteResource(RES(0xc8, struct bkResource *));
		bkDeleteResource(RES(0xd4, struct bkResource *));
		bkDeleteResource(RES(0xd8, struct bkResource *));
		bkDeleteResource(RES(0xdc, struct bkResource *));
		bkDeleteResource(RES(0xe0, struct bkResource *));
		bkDeleteResource(RES(0xe4, struct bkResource *));
		bkDeleteResource(RES(0xe8, struct bkResource *));
		bkDeleteResource(RES(0xec, struct bkResource *));
		bkDeleteResource(RES(0xf4, struct bkResource *));
		break;
	case 3:
		bkDeleteResource(RES(0xb8, struct bkResource *));
		bkDeleteResource(RES(0xc8, struct bkResource *));
		bkDeleteResource(RES(0xcc, struct bkResource *));
		bkDeleteResource(RES(0xd0, struct bkResource *));
		bkDeleteResource(RES(0xd4, struct bkResource *));
		bkDeleteResource(RES(0xd8, struct bkResource *));
		bkDeleteResource(RES(0xdc, struct bkResource *));
		bkDeleteResource(RES(0xe0, struct bkResource *));
		bkDeleteResource(RES(0xe4, struct bkResource *));
		bkDeleteResource(RES(0xe8, struct bkResource *));
		bkDeleteResource(RES(0xec, struct bkResource *));
		break;
	case 4:
		bkDeleteResource(RES(0xb8, struct bkResource *));
		bkDeleteResource(RES(0xb4, struct bkResource *));
		bkDeleteResource(RES(0xc4, struct bkResource *));
		bkDeleteResource(RES(0xc8, struct bkResource *));
		bkDeleteResource(RES(0xd4, struct bkResource *));
		bkDeleteResource(RES(0xd8, struct bkResource *));
		bkDeleteResource(RES(0xdc, struct bkResource *));
		bkDeleteResource(RES(0xe0, struct bkResource *));
		bkDeleteResource(RES(0xe4, struct bkResource *));
		bkDeleteResource(RES(0xe8, struct bkResource *));
		bkDeleteResource(RES(0xec, struct bkResource *));
		break;
	}
	if (g_DrVariant == 2 || g_DrVariant == 4)
		bkDeleteResource(RES(0x8c, struct bkResource *));
	bkDeletePackageResources(RES(0x90, unsigned int), 1);
	baDeleteExtraSystem(NULL);
	for (i = 0; i < 8; i++) {
		struct BLight **light = &RES(0x94 + i * 4, struct BLight *);

		if (*light != NULL) {
			baEnableLight(*light, 0);
			baDeleteLight(*light);
			*light = NULL;
		}
	}
	bkClosePackageThunk(DR_PACKAGE_OF(self));
}
