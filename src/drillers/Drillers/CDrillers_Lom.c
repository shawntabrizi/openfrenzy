/*
Drillers/CDrillers_Lom.c  <-  C:\BlitzParty\Game\Src\Drillers\CDrillers_Lom.cpp

The level object manager (one static object at 0x002a54d8, vtable 0x0026c310): it loads a
.lom file (a ParseMap text map) of the variant's folder ("driller", "token", "s_trails",
"snare"): "World" nodes are BACTOR sets (<folder>\<name>.set), "Object" nodes with
"Instance" children are actors (city-obj\<name>.obe) and effect generators (steam, corona,
sparks, h_steam, light); the instance "wcentre" is the world centre. It draws the scenery
turned with the camera arena (and wobbling in Erodeo).

0x0011b470 names this file in an allocation; the functions from 0x0011add0 to 0x0011c440
work on the same object (file inferred). 0x0011add0, 0x0011b0a0, 0x0011b420, 0x0011bc40
and 0x0011c3f0 are not in of_functions.tsv.

The object, fields at original offsets:
  +0x004 folder name   +0x084 object name   +0x104 (char) an object is open
  +0x108 turn (degrees)   +0x13c wobble (degrees)   +0x140 wobble amplitude
  +0x1b4 resources (0x74 bytes each), count at +0x1e44
  +0x1e48 actors (64), count at +0x1f48
  +0x1f4c instances (0x38 bytes: position, rotation, scale, BActorInstance * at +0x30,
          flag at +0x34), count at +0x2d4c
  +0x2d50 sets (0x78 bytes: position, rotation, scale, matrix at +0x30, the set at +0x70,
          a second resource at +0x74), count at +0x4b50
*/

#include "drillers.h"

#define DR_LOM_ADDR 0x002a54d8u
#define LOM(off, type) DR_AT(lom, off, type)
#define LOM_INST(k, off, type) DR_AT(lom, 0x1f4c + (k) * 0x38 + (off), type)
#define LOM_SET(k, off, type) DR_AT(lom, 0x2d50 + (k) * 0x78 + (off), type)

/* the world centre (the "wcentre" instance) */
#define DR_CENTRE DR_PTR(float, 0x002a55e8u)
/* the girder instances of Rubble Alliance (N1, N2, E1, E2, S1, S2, W1, W2) */
#define DR_GIRDER(k) DR_VAR(int, 0x002a55f4u + (k) * 4)
/* the variant package */
#define DR_PACKAGE DR_VAR(struct bkPackage *, 0x002aa908u)
/* the round (game +0x68), frozen (game +0x84) */
#define DR_ROUND DR_VAR(int, 0x002ffa58u)
#define DR_FROZEN DR_VAR(signed char, 0x002ffa74u)
/* the "mono01" instance; counters */
#define DR_MONO DR_VAR(int, 0x00302020u)
#define DR_WOBBLE_T DR_VAR(int, 0x00302024u)
#define DR_ANIM_T DR_VAR(int, 0x00302028u)
#define DR_LOM_COUNT DR_VAR(int, 0x0030201cu)
/* the animated texture of the scenery, the sky actor (resources) */
#define DR_TEX_SCENERY DR_VAR(struct bdTexture *, 0x002aa974u)
#define DR_SKY DR_VAR(char *, 0x002aaaecu)
/* tuning: 0 hides the scenery */
#define DR_TUNE_SCENERY DR_VAR(float, 0x002aae20u)

#define DR_DEG 0.017453292f

/* 0x0026c310  vtable of the level object manager (rebuilt from the XBE) */
const void *const vtable_0026c310[1] = {
	(const void *)DrLom_DeletingDtor, /* +0x00 */
};

/* 0x000509b0 (bKernel; not in of_functions.tsv and not defined by bkernel yet): a rotation
   about y by `y`, then about z by `z` (a private copy of the library routine) */
static void dr_MatrixRotationYZ(float *m, float y, float z)
{
	float cy = cosf(y);
	float sy = sinf(y);
	float cz = cosf(z);
	float sz = sinf(z);

	m[12] = 0.0f;
	m[9] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;
	m[0] = cz * cy;
	m[4] = -(sz * cy);
	m[8] = sy;
	m[1] = sz;
	m[5] = cz;
	m[2] = -(cz * sy);
	m[6] = sz * sy;
	m[10] = cy;
}

/* reads a float property of a ParseMap node, `def` when it is missing */
static float dr_get_float(int node, const char *name, float def)
{
	float v;

	if (ParseMap_GetFloat(node, name, (undefined4 *)&v) == 0)
		v = def;
	return v;
}

/* reads an int property of a ParseMap node, `def` when it is missing */
static int dr_get_int(int node, const char *name, int def)
{
	int v;

	if (ParseMap_GetInt(node, name, (undefined4 *)&v) == 0)
		v = def;
	return v;
}

/* 0x0011add0  Drillers (not in of_functions.tsv; ParseMap callback for "Instance")
First pass: the instance "wcentre" sets the world centre. */
int FUN_0011add0(int node, int arg)
{
	char *name;
	float pos[4];

	(void)arg;
	name = ParseMap_GetString(node, "Name");
	pos[0] = dr_get_float(node, "PosX", 0.0f);
	pos[1] = dr_get_float(node, "PosY", 0.0f);
	pos[2] = dr_get_float(node, "PosZ", 0.0f);
	pos[3] = 1.0f;
	(void)dr_get_float(node, "RotX", 1e-05f);
	(void)dr_get_float(node, "RotY", 0.0f);
	(void)dr_get_float(node, "RotZ", 0.0f);
	(void)dr_get_float(node, "RotW", 0.0f);
	(void)dr_get_float(node, "SclX", 1.0f);
	(void)dr_get_float(node, "SclY", 1.0f);
	(void)dr_get_float(node, "SclZ", 0.0f);
	if (strcmp(name, "wcentre") == 0) {
		DR_CENTRE[0] = pos[0];
		DR_CENTRE[1] = pos[1];
		DR_CENTRE[2] = pos[2];
		return 2;
	}
	return 0;
}

/* 0x0011af70  Drillers (gap; CDrillers_Lom.cpp inferred)
Frees everything the manager loaded. */
void DrLom_Free(void *self)
{
	char *lom = (char *)self;
	int k;

	CLIntroCam_Destroy();
	for (k = 0; k < LOM(0x1e44, int); k++) {
		if (LOM(0x1b4 + k * 0x74, struct bkResource *) != NULL) {
			bkDeleteResource(LOM(0x1b4 + k * 0x74, struct bkResource *));
			LOM(0x1b4 + k * 0x74, struct bkResource *) = NULL;
		}
	}
	for (k = 0; k < LOM(0x2d4c, int); k++) {
		if (LOM_INST(k, 0x30, struct BActorInstance *) != NULL) {
			baDeleteActorInstance(LOM_INST(k, 0x30, struct BActorInstance *));
			LOM_INST(k, 0x30, struct BActorInstance *) = NULL;
		}
	}
	for (k = 0; k < LOM(0x1f48, int); k++) {
		if (LOM(0x1e48 + k * 4, struct bkResource *) != NULL) {
			bkDeleteResource(LOM(0x1e48 + k * 4, struct bkResource *));
			LOM(0x1e48 + k * 4, struct bkResource *) = NULL;
		}
	}
	for (k = 0; k < LOM(0x4b50, int); k++) {
		if (LOM_SET(k, 0x70, struct bkResource *) != NULL) {
			bkDeleteResource(LOM_SET(k, 0x70, struct bkResource *));
			LOM_SET(k, 0x70, struct bkResource *) = NULL;
		}
		if (LOM_SET(k, 0x74, struct bkResource *) != NULL) {
			bkDeleteResource(LOM_SET(k, 0x74, struct bkResource *));
			LOM_SET(k, 0x74, struct bkResource *) = NULL;
		}
	}
}

/* 0x0011b080  Drillers (gap; CDrillers_Lom.cpp inferred; vtable 0x0026c310 +0x00) */
void *DrLom_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026c310;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x0011b0a0  Drillers (not in of_functions.tsv; ParseMap callback for "World")
A set: <folder>\<name>.set at the node's position (relative to the world centre),
rotation and scale. Stops the walk (2) when the set is missing or 64 are loaded. */
int FUN_0011b0a0(int node, int arg)
{
	char *lom = (char *)(uintptr_t)arg;
	char path[128];
	float pos[3];
	float rot[4];
	float scale[3];
	float s[16];
	float t[16];
	float r[16];
	float rs[16];
	int k;

	sprintf(path, "%s\\%s.set", lom + 4, ParseMap_GetString(node, "Name"));
	_strlwr(path);
	pos[0] = dr_get_float(node, "PosX", 0.0f);
	pos[1] = dr_get_float(node, "PosY", 0.0f);
	pos[2] = dr_get_float(node, "PosZ", 0.0f);
	k = LOM(0x4b50, int);
	LOM_SET(k, 0x0, float) = pos[0];
	LOM_SET(k, 0x4, float) = pos[1];
	LOM_SET(k, 0x8, float) = pos[2];
	LOM_SET(k, 0xc, float) = 1.0f;
	rot[0] = dr_get_float(node, "RotX", 1e-05f);
	rot[1] = dr_get_float(node, "RotY", 0.0f);
	rot[2] = dr_get_float(node, "RotZ", 0.0f);
	rot[3] = dr_get_float(node, "RotW", 0.0f);
	LOM_SET(k, 0x10, float) = rot[0];
	LOM_SET(k, 0x14, float) = rot[1];
	LOM_SET(k, 0x18, float) = rot[2];
	LOM_SET(k, 0x1c, float) = rot[3];
	scale[0] = dr_get_float(node, "SclX", 1.0f);
	scale[1] = dr_get_float(node, "SclY", 1.0f);
	scale[2] = dr_get_float(node, "SclZ", 1.0f);
	pos[0] = pos[0] - DR_CENTRE[0];
	pos[1] = pos[1] - DR_CENTRE[1];
	pos[2] = pos[2] - DR_CENTRE[2];
	LOM_SET(k, 0x20, float) = scale[0];
	LOM_SET(k, 0x24, float) = scale[1];
	LOM_SET(k, 0x28, float) = scale[2];
	LOM_SET(k, 0x2c, float) = 1.0f;
	LOM_SET(k, 0x70, struct bkResource *) = bLoadResource(DR_PACKAGE, path, 6, 0xdefa);
	if (LOM_SET(k, 0x70, struct bkResource *) == NULL) {
		bkPrintf("*** ERROR *** Could not find level [%s]\n", path);
		return 2;
	}
	bmMatrixScaling(s, scale[0], scale[1], scale[2]);
	bmMatrixTranslation(t, pos[0], pos[1], pos[2]);
	bmQuaternionToMatrix(r, rot);
	bmMatrixMultiplyCopy(rs, r, s);
	bmMatrixMultiplyCopy(&LOM_SET(k, 0x30, float), t, rs);
	LOM(0x4b50, int) = k + 1;
	return k + 1 < 0x40 ? 0 : 2;
}

/* 0x0011b420  Drillers (not in of_functions.tsv; ParseMap callback for "Object", first
pass): keeps the object's name and walks its instances for the world centre. */
int FUN_0011b420(int node, int arg)
{
	char *lom = (char *)(uintptr_t)arg;

	strcpy(lom + 0x84, ParseMap_GetString(node, "Name"));
	LOM(0x104, char) = 1;
	ParseMap_Walk(node, (int)(uintptr_t)FUN_0011add0, (undefined4)arg, (int)(uintptr_t)"Instance");
	return 2;
}

/* 0x0011b470  Drillers/CDrillers_Lom.cpp (ParseMap callback for "Instance", second pass)
An instance of the open object (its position relative to the world centre): effect objects
add an effect generator and stop; the girders and "mono01" are remembered; the first
instance of an object loads its actor (city-obj\<name>.obe); every instance gets an actor
instance ("vbuild01" ones are flagged). "wcentre" is skipped. */
int DrLom_AddInstance(int node, int arg)
{
	static const char *const girders[8] = {
		"GirderN1", "GirderN2", "GirderE1", "GirderE2",
		"GirderS1", "GirderS2", "GirderW1", "GirderW2",
	};
	char *lom = (char *)(uintptr_t)arg;
	char path[132];
	char *name;
	char *object;
	float pos[4];
	float rot[4];
	float scale[4];
	struct BActorInstance *inst;
	int red;
	int green;
	int blue;
	int size;
	int k;
	int n;

	name = ParseMap_GetString(node, "Name");
	pos[0] = dr_get_float(node, "PosX", 0.0f);
	pos[1] = dr_get_float(node, "PosY", 0.0f);
	pos[2] = dr_get_float(node, "PosZ", 0.0f);
	pos[3] = 1.0f;
	rot[0] = dr_get_float(node, "RotX", 1e-05f);
	rot[1] = dr_get_float(node, "RotY", 0.0f);
	rot[2] = dr_get_float(node, "RotZ", 0.0f);
	rot[3] = dr_get_float(node, "RotW", 0.0f);
	scale[0] = dr_get_float(node, "SclX", 1.0f);
	scale[1] = dr_get_float(node, "SclY", 1.0f);
	scale[2] = dr_get_float(node, "SclZ", 1.0f);
	scale[3] = 1.0f;
	if (strcmp(name, "wcentre") == 0)
		return 0;
	if (strcmp(name, "mono01") == 0)
		DR_MONO = LOM(0x2d4c, int);
	pos[0] = pos[0] - DR_CENTRE[0];
	pos[1] = pos[1] - DR_CENTRE[1];
	pos[2] = pos[2] - DR_CENTRE[2];
	if (LOM(0x104, char) != 0) {
		object = lom + 0x84;
		if (strncmp("steam", object, 5) == 0) {
			if (baAddExtraItem(g_DrFx->gen, (double)pos[0], (double)pos[1], (double)pos[2], 1) !=
					NULL)
				bkPrintf("*** STEAM effect spotted and gen point added\n");
			return 2;
		}
		if (strncmp("corona", object, 6) == 0) {
			red = dr_get_int(node, "RED", 0x80);
			green = dr_get_int(node, "GREEN", 0x80);
			blue = dr_get_int(node, "BLUE", 0x80);
			if (baAddExtraItem(g_DrFx->gen, (double)pos[0], (double)pos[1], (double)pos[2], 2, red,
					green, blue) != NULL)
				bkPrintf("*** CORONA effect spotted and gen point added\n");
			return 2;
		}
		if (strncmp("sparks", object, 6) == 0) {
			if (baAddExtraItem(g_DrFx->gen, (double)pos[0], (double)pos[1], (double)pos[2], 3,
					(double)rot[1]) != NULL)
				bkPrintf("*** SPARKS effect spotted and gen point added\n");
			return 2;
		}
		if (strncmp("h_steam", object, 7) == 0) {
			if (baAddExtraItem(g_DrFx->gen, (double)pos[0], (double)pos[1], (double)pos[2], 4,
					(double)rot[1]) != NULL)
				bkPrintf("*** H_STEAM effect spotted and gen point added\n");
			return 2;
		}
		if (strncmp("light", object, 5) == 0) {
			size = dr_get_int(node, "SIZE", 100);
			red = dr_get_int(node, "RED", 0x40);
			green = dr_get_int(node, "GREEN", 0x40);
			blue = dr_get_int(node, "BLUE", 0x40);
			if (baAddExtraItem(g_DrFx->gen, (double)pos[0], (double)pos[1], (double)pos[2], 5, size,
					red, green, blue) != NULL)
				bkPrintf("*** LIGHT effect spotted and gen point added\n");
			return 2;
		}
		for (k = 0; k < 8; k++) {
			if (strncmp(girders[k], object, 8) == 0)
				DR_GIRDER(k) = LOM(0x2d4c, int);
		}
		n = LOM(0x1f48, int);
		sprintf(path, "city-obj\\%s.obe", object);
		_strlwr(path);
		LOM(0x1e48 + n * 4, struct bkResource *) = bLoadResource(DR_PACKAGE, path, 1, 0xdefa);
		if (LOM(0x1e48 + n * 4, struct bkResource *) == NULL) {
			bkPrintf("*** ERROR *** Could not load actor [%s]\n", path);
			return 2;
		}
		LOM(0x1f48, int) = LOM(0x1f48, int) + 1;
	}
	k = LOM(0x2d4c, int);
	inst = baInitActorInstance(NULL,
		LOM(0x1e44 + LOM(0x1f48, int) * 4, struct BActor *), 1, DR_FILE_LOM, 0x1c4);
	LOM_INST(k, 0x30, struct BActorInstance *) = inst;
	if (inst == NULL) {
		bkPrintf("*** ERROR *** Could not create actor instance [%s]\n", path);
		return 2;
	}
	LOM_INST(k, 0x00, float) = pos[0];
	LOM_INST(k, 0x04, float) = pos[1];
	LOM_INST(k, 0x08, float) = pos[2];
	LOM_INST(k, 0x0c, float) = pos[3];
	LOM_INST(k, 0x10, float) = rot[0];
	LOM_INST(k, 0x14, float) = rot[1];
	LOM_INST(k, 0x18, float) = rot[2];
	LOM_INST(k, 0x1c, float) = rot[3];
	LOM_INST(k, 0x20, float) = scale[0];
	LOM_INST(k, 0x24, float) = scale[1];
	LOM_INST(k, 0x28, float) = scale[2];
	LOM_INST(k, 0x2c, float) = scale[3];
	DR_F32(inst, 0xc0) = pos[0];
	DR_F32(inst, 0xc4) = pos[1];
	DR_F32(inst, 0xc8) = pos[2];
	DR_F32(inst, 0xcc) = pos[3];
	DR_F32(inst, 0xe0) = rot[0];
	DR_F32(inst, 0xe4) = rot[1];
	DR_F32(inst, 0xe8) = rot[2];
	DR_F32(inst, 0xec) = rot[3];
	DR_F32(inst, 0xd0) = scale[0];
	DR_F32(inst, 0xd4) = scale[1];
	DR_F32(inst, 0xd8) = scale[2];
	DR_F32(inst, 0xdc) = scale[3];
	baPrepActorInstance(inst, 1);
	LOM(0x2d4c, int) = LOM(0x2d4c, int) + 1;
	LOM_INST(k, 0x34, int) = strcmp("vbuild01", name) == 0;
	LOM(0x104, char) = 0;
	return 0;
}

/* the world matrix of the scenery: the turn (Erodeo: and the wobble) and, in Rubble
   Alliance, the round's layer */
static void dr_lom_turn(char *lom, float *m)
{
	if (g_DrVariant == 2)
		dr_MatrixRotationYZ(m, LOM(0x108, float) * DR_DEG, LOM(0x13c, float) * DR_DEG);
	else
		bmMatrixRotationY(m, LOM(0x108, float) * DR_DEG);
}

/* draws the sets with `pass` */
static void dr_lom_draw_sets(char *lom, const float *base, int pass)
{
	float world[16];
	int k;

	for (k = 0; k < LOM(0x4b50, int); k++) {
		bmMatrixMultiplyCopy(world, base, &LOM_SET(k, 0x30, float));
		bdSetWorldMatrix(world);
		baDrawSet(LOM_SET(k, 0x70, struct BSet *), pass);
	}
}

/* 0x0011bc40  Drillers (not in of_functions.tsv)
Draws the scenery (unless switched off by tuning): the sets, then every actor instance
turned with the scenery (the girder of the current turn is left out in rounds 2 and 3 of
Rubble Alliance; there all is lifted to the round's layer), their second pass, the sets'
second pass, and the sky actor (variants 3, 4). The animated texture advances. */
void FUN_0011bc40(void *self)
{
	char *lom = (char *)self;
	char *inst;
	float rot[16];
	float trans[16];
	float base[16];
	float q[4];
	float qy[4];
	float qz[4];
	float qr[4];
	float keep[4];
	float x;
	float y;
	float z;
	float ang = 0.0f;
	int girder;
	int a;
	int k;

	if (DR_FROZEN == 0)
		DR_ANIM_T++;
	bdTextureSetFrame(DR_TEX_SCENERY, DR_ANIM_T / 2);
	bdSetRenderState(4, 1, 0);
	if (2 < g_DrVariant)
		bdSetRenderState(3, 0, 0);
	if (g_DrVariant == 2 && DR_FROZEN == 0) {
		if (LOM(0x140, float) == 0.0f)
			DR_WOBBLE_T = 0;
		DR_WOBBLE_T++;
		LOM(0x13c, float) = sinf(((float)DR_WOBBLE_T + (float)DR_WOBBLE_T) * DR_DEG) *
			LOM(0x140, float);
	}
	if (DR_TUNE_SCENERY == 0.0f)
		return;
	D3DDevice_SetRenderState_TextureFactor(0x40000000);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLOROP, 0xe);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLORARG1, 2);
	D3DDevice_SetTextureStageState(1, D3DTSS_COLORARG2, 1);
	D3DDevice_SetTextureStageState(1, D3DTSS_ADDRESSU, 1);
	D3DDevice_SetTextureStageState(1, D3DTSS_ADDRESSV, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 2);
	dr_lom_turn(lom, rot);
	if (g_DrVariant == 1)
		y = (float)(DR_ROUND - 1) * 20.0f * 80.0f;
	else
		y = 0.0f;
	bmMatrixTranslation(trans, 0.0f, y, 0.0f);
	bmMatrixMultiplyCopy(base, rot, trans);
	dr_lom_draw_sets(lom, base, 1);
	bdSetWorldIdentity();
	for (k = 0; k < LOM(0x2d4c, int); k++) {
		inst = LOM_INST(k, 0x30, char *);
		if (g_DrVariant == 1) {
			a = DR_FTOL(LOM(0x108, float));
			girder = -1;
			if (a == 0x10e) {
				ang = LOM(0x108, float) + 180.0f;
				girder = DR_ROUND == 2 ? DR_GIRDER(2) : DR_ROUND == 3 ? DR_GIRDER(3) : -1;
			} else if (a == 0xb4) {
				ang = LOM(0x108, float);
				girder = DR_ROUND == 2 ? DR_GIRDER(4) : DR_ROUND == 3 ? DR_GIRDER(5) : -1;
			} else if (a == 0) {
				ang = LOM(0x108, float);
				girder = DR_ROUND == 2 ? DR_GIRDER(0) : DR_ROUND == 3 ? DR_GIRDER(1) : -1;
			} else if (a == 0x5a) {
				ang = LOM(0x108, float) + 180.0f;
				girder = DR_ROUND == 2 ? DR_GIRDER(6) : DR_ROUND == 3 ? DR_GIRDER(7) : -1;
			}
			if (girder != -1 && k == girder)
				continue;
			DR_F32(inst, 0xc4) = (float)(DR_ROUND - 1) * 20.0f * 80.0f + DR_F32(inst, 0xc4);
		} else {
			ang = LOM(0x108, float);
		}
		keep[0] = DR_F32(inst, 0xe0);
		keep[1] = DR_F32(inst, 0xe4);
		keep[2] = DR_F32(inst, 0xe8);
		keep[3] = DR_F32(inst, 0xec);
		q[0] = keep[0];
		q[1] = keep[1];
		q[2] = keep[2];
		q[3] = keep[3];
		if (g_DrVariant == 2) {
			bmQuaternionRotationY(qy, ang * DR_DEG);
			bmQuaternionRotationZ(qz, -LOM(0x13c, float) * DR_DEG);
			bmQuaternionMultiply(qr, qy, qz);
		} else {
			bmQuaternionRotationY(qr, ang * DR_DEG);
		}
		bmQuaternionMultiply((float *)(inst + 0xe0), q, qr);
		x = DR_F32(inst, 0xc0);
		z = DR_F32(inst, 0xc8);
		y = DR_F32(inst, 0xc4);
		dr_lom_turn(lom, rot);
		bmTransformVector4InPlace(rot, (float *)(inst + 0xc0));
		if (DR_FROZEN == 0)
			baUpdateAnimations((struct BActorInstance *)inst,
				DR_AT(inst, 0x134, struct BActorNode *), 1.0f, NULL);
		baPrepActorInstance((struct BActorInstance *)inst, 1);
		baDrawActorInstance((struct BActorInstance *)inst, 1);
		if (g_DrVariant == 1)
			DR_F32(inst, 0xc4) = DR_F32(inst, 0xc4) - (float)(DR_ROUND - 1) * 20.0f * 80.0f;
		DR_F32(inst, 0xe0) = keep[0];
		DR_F32(inst, 0xe4) = keep[1];
		DR_F32(inst, 0xe8) = keep[2];
		DR_F32(inst, 0xec) = keep[3];
		DR_F32(inst, 0xc0) = x;
		if (g_DrVariant == 2)
			DR_F32(inst, 0xc4) = y;
		DR_F32(inst, 0xc8) = z;
	}
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	for (k = 0; k < LOM(0x2d4c, int); k++) {
		bdSetTexture(1, NULL);
		baDrawActorInstance(LOM_INST(k, 0x30, struct BActorInstance *), 4);
	}
	dr_lom_draw_sets(lom, base, 4);
	if (2 < g_DrVariant) {
		bdSetRenderState(3, 1, 0);
		DR_F32(DR_SKY, 0xc0) = -DR_CENTRE[0];
		DR_F32(DR_SKY, 0xc4) = -DR_CENTRE[1];
		DR_F32(DR_SKY, 0xc8) = -DR_CENTRE[2];
		DR_F32(DR_SKY, 0xcc) = 1.0f;
		baPrepActorInstance((struct BActorInstance *)DR_SKY, 1);
		baDrawActorInstance((struct BActorInstance *)DR_SKY, 1);
	}
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(4, 0, 0);
	bdSetTexture(1, NULL);
}

/* 0x0011c3f0  Drillers (not in of_functions.tsv; ParseMap callback for "Object", second
pass): keeps the object's name and adds its instances. */
int FUN_0011c3f0(int node, int arg)
{
	char *lom = (char *)(uintptr_t)arg;

	strcpy(lom + 0x84, ParseMap_GetString(node, "Name"));
	LOM(0x104, char) = 1;
	ParseMap_Walk(node, (int)(uintptr_t)DrLom_AddInstance, (undefined4)arg,
		(int)(uintptr_t)"Instance");
	return 2;
}

/* 0x0011c440  Drillers (gap; CDrillers_Lom.cpp inferred)
LoadLOMFile: the variant's folder, then the .lom file `name` of `package`: the world
centre, the sets, the instances and the intro camera; every instance with an animation
"default" plays it. Returns 0 when the file cannot be loaded or parsed. */
int DrLom_Load(void *self, struct bkPackage *package, const char *name)
{
	char *lom = (char *)self;
	struct BActorSubObject *anim;
	unsigned int size;
	void *file;
	int *map;
	int k;

	switch (g_DrVariant) {
	case 1:
		memcpy(lom + 4, "driller", 8);
		break;
	case 2:
		memcpy(lom + 4, "token", 6);
		break;
	case 3:
		memcpy(lom + 4, "s_trails", 9);
		break;
	case 4:
		memcpy(lom + 4, "snare", 6);
		break;
	}
	LOM(0x4b50, int) = 0;
	LOM(0x1f48, int) = 0;
	LOM(0x2d4c, int) = 0;
	DR_LOM_COUNT = 0;
	file = bkLoadFile(package, name, NULL, &size, NULL, 0);
	if (file == NULL) {
		bkPrintf("LoadLOMFile ERROR: Could not load [%s]\n", name);
		return 0;
	}
	map = ParseMap_Load((char *)file, 2, (int)size);
	if (map == NULL) {
		bkPrintf("LoadLOMFile ERROR: Parsing [%s]\n", name);
		return 0;
	}
	ParseMap_WalkMap((int)(uintptr_t)map, (int)(uintptr_t)FUN_0011b420, (undefined4)(uintptr_t)lom,
		(int)(uintptr_t)"Object");
	ParseMap_WalkMap((int)(uintptr_t)map, (int)(uintptr_t)FUN_0011b0a0, (undefined4)(uintptr_t)lom,
		(int)(uintptr_t)"World");
	ParseMap_WalkMap((int)(uintptr_t)map, (int)(uintptr_t)FUN_0011c3f0, (undefined4)(uintptr_t)lom,
		(int)(uintptr_t)"Object");
	CLIntroCam_Load((int)(uintptr_t)map, (undefined4)(uintptr_t)lom);
	ParseMap_Free(map);
	bkPrintf("***SEARCHING %d OBJECTS***\n", LOM(0x2d4c, int));
	for (k = 0; k < LOM(0x2d4c, int); k++) {
		anim = baFindSubObject(LOM_INST(k, 0x30, struct BActorInstance *), "default");
		if (anim != NULL) {
			bkPrintf("***FOUND %d\n", k);
			baQueueAnimation(LOM_INST(k, 0x30, struct BActorInstance *),
				DR_AT(LOM_INST(k, 0x30, char *), 0x134, struct BActorNode *), (struct BAnim *)anim,
				0x400, -1, 1, NULL, NULL);
			bkPrintf("sframe %d  eframe %d   ticks %d\n", DR_I32(anim, 4), DR_I32(anim, 8),
				DR_I32(anim, 0xc));
		}
	}
	return 1;
}
