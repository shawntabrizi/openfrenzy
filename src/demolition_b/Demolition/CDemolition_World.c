/*
Demolition/CDemolition_World.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_World.cpp

The level of the Demolition game (game +0x110, vtable 0x0026b0f8, 0x1184 bytes), loaded from
<level>\demo_<level>.lom through ParseMap (classlib_ui), like CFire_World of CarryOn
(carryon_a). Only 0x000f7b90 is tagged with this file (its assert path); the other
functions are the methods around it (they use the same fields), "file inferred" below.

World fields:
  +0x004 char[0x80] the level folder (also the LOM name)
  +0x084 char[0x80] the name of the current "Object" block
  +0x104 byte: the next "Instance" is the first of its "Object" block (loads the model)
  +0x108 the package
  +0x10c layers[10] (0x90 bytes; count +0x6ac): +0x00 loaded, +0x04 position, +0x14
         rotation (quaternion), +0x24 scale, +0x44 world matrix, +0x84 the set (.obe),
         +0x88 the collision set (_col.obe), +0x8c CRC of the name (platform layers)
  +0x6b0 start positions[4] (0x20 bytes: position, rotation; count +0x730)
  +0x8b8 universe objects (0x74 bytes; count +0x92c, which the second entry overlaps):
         +0x00 position, +0x10 rotation, +0x20 scale, +0x30 world matrix, +0x70 the set
  +0x930 object models[8] (actor, CRC of the object name; count +0x970)
  +0x974 instances[14] (count +0xd34), +0xd38 "alpha" instances[14] (count +0x10f8),
  +0x10fc "shadow" instance (count +0x113c), +0x1140 "decal" instance (count +0x1180):
         0x40 bytes: +0x00 position, +0x10 rotation, +0x20 scale, +0x30 BActorInstance,
         +0x34 its "default" animation, +0x38 CRC of the name, +0x3c ZBIAS
*/

#include "demolition_b.h"
#include "demolition_b_internal.h"
#include "d3d8_extra.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* 0x00297520  the variation settings (demolition_a, 0x3c bytes each); +0x00: the variation
   has universe objects and animated instances */
extern int DAT_00297520[];
#define DB_VARIATION_HAS_OBJECTS() (DAT_00297520[DB_I32(DAT_002ff85c, 0x28) * 15] != 0)

#define DB_WORLD_FILE "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_World.cpp"

/* the layer `n` (0x90 bytes from +0x10c) */
#define DB_LAYER(world, n) ((char *)(world) + 0x10c + (n) * 0x90)

/* a block's transform: Pos (default 0, w 1), Rot (default 0), Scl (default 1, w 1) */
static void db_read_transform(int node, float *pos, float *rot, float *scale)
{
	if (ParseMap_GetFloat(node, "PosX", (undefined4 *)&pos[0]) == 0)
		pos[0] = 0.0f;
	if (ParseMap_GetFloat(node, "PosY", (undefined4 *)&pos[1]) == 0)
		pos[1] = 0.0f;
	if (ParseMap_GetFloat(node, "PosZ", (undefined4 *)&pos[2]) == 0)
		pos[2] = 0.0f;
	pos[3] = 1.0f;
	if (ParseMap_GetFloat(node, "RotX", (undefined4 *)&rot[0]) == 0)
		rot[0] = 0.0f;
	if (ParseMap_GetFloat(node, "RotY", (undefined4 *)&rot[1]) == 0)
		rot[1] = 0.0f;
	if (ParseMap_GetFloat(node, "RotZ", (undefined4 *)&rot[2]) == 0)
		rot[2] = 0.0f;
	if (ParseMap_GetFloat(node, "RotW", (undefined4 *)&rot[3]) == 0)
		rot[3] = 0.0f;
	if (ParseMap_GetFloat(node, "SclX", (undefined4 *)&scale[0]) == 0)
		scale[0] = 1.0f;
	if (ParseMap_GetFloat(node, "SclY", (undefined4 *)&scale[1]) == 0)
		scale[1] = 1.0f;
	if (ParseMap_GetFloat(node, "SclZ", (undefined4 *)&scale[2]) == 0)
		scale[2] = 1.0f;
	scale[3] = 1.0f;
}

/* out = translation * rotation; the original picks the aligned (bmMatrixMultiplyCopy) or the
   unaligned multiply (bmMatrixMultiply) by the addresses */
static void db_world_matrix(float *out, float *translation, float *rotation, uintptr_t align)
{
	if ((align & 0xf) == 0)
		bmMatrixMultiplyCopy(out, translation, rotation);
	else
		bmMatrixMultiply(out, translation, rotation);
}

/* 0x000f6fe0  (file inferred) constructor */
struct CDemolition_World *CDemolition_World_Construct(struct CDemolition_World *self)
{
	DB_PTR(self, 0) = (void *)vtable_0026b0f8;
	return self;
}

/* 0x000f6ff0  (file inferred) destructor */
void CDemolition_World_Destruct(struct CDemolition_World *self)
{
	DB_PTR(self, 0) = (void *)vtable_0026b0f8;
}

/* 0x000f7000  (file inferred) Shutdown: frees the universe sets, the instances of the four
   lists, the object models and the layer sets */
void CDemolition_World_Shutdown(struct CDemolition_World *self)
{
	char *w = (char *)self;
	int i;

	for (i = 0; i < DB_I32(w, 0x92c); i++)
	{
		if (DB_PTR(w, 0x928 + i * 0x74) != NULL)
		{
			bkDeleteResource((struct bkResource *)DB_PTR(w, 0x928 + i * 0x74));
			DB_PTR(w, 0x928 + i * 0x74) = NULL;
		}
	}
	for (i = 0; i < DB_I32(w, 0xd34); i++)
	{
		if (DB_PTR(w, 0x9a4 + i * 0x40) != NULL)
		{
			baDeleteActorInstance((struct BActorInstance *)DB_PTR(w, 0x9a4 + i * 0x40));
			DB_PTR(w, 0x9a4 + i * 0x40) = NULL;
		}
	}
	for (i = 0; i < DB_I32(w, 0x10f8); i++)
	{
		if (DB_PTR(w, 0xd68 + i * 0x40) != NULL)
		{
			baDeleteActorInstance((struct BActorInstance *)DB_PTR(w, 0xd68 + i * 0x40));
			DB_PTR(w, 0xd68 + i * 0x40) = NULL;
		}
	}
	for (i = 0; i < DB_I32(w, 0x113c); i++)
	{
		if (DB_PTR(w, 0x112c + i * 0x40) != NULL)
		{
			baDeleteActorInstance((struct BActorInstance *)DB_PTR(w, 0x112c + i * 0x40));
			DB_PTR(w, 0x112c + i * 0x40) = NULL;
		}
	}
	for (i = 0; i < DB_I32(w, 0x1180); i++)
	{
		if (DB_PTR(w, 0x1170 + i * 0x40) != NULL)
		{
			baDeleteActorInstance((struct BActorInstance *)DB_PTR(w, 0x1170 + i * 0x40));
			DB_PTR(w, 0x1170 + i * 0x40) = NULL;
		}
	}
	for (i = 0; i < DB_I32(w, 0x970); i++)
	{
		if (DB_PTR(w, 0x930 + i * 8) != NULL)
		{
			bkDeleteResource((struct bkResource *)DB_PTR(w, 0x930 + i * 8));
			DB_PTR(w, 0x930 + i * 8) = NULL;
		}
	}
	for (i = 0; i < DB_I32(w, 0x6ac); i++)
	{
		char *layer = DB_LAYER(w, i);

		if (DB_PTR(layer, 0x84) != NULL)
		{
			bkDeleteResource((struct bkResource *)DB_PTR(layer, 0x84));
			DB_PTR(layer, 0x84) = NULL;
		}
		if (DB_PTR(layer, 0x88) != NULL)
		{
			bkDeleteResource((struct bkResource *)DB_PTR(layer, 0x88));
			DB_PTR(layer, 0x88) = NULL;
		}
	}
}

/* 0x000f71b0  (file inferred) Update: animates the instances that have an animation (solid
   and alpha), in the variations with objects */
void CDemolition_World_Update(struct CDemolition_World *self, float dt)
{
	char *w = (char *)self;
	int i;

	if (!DB_VARIATION_HAS_OBJECTS())
		return;
	for (i = 0; i < DB_I32(w, 0xd34); i++)
	{
		char *e = w + 0x9a4 + i * 0x40;

		if (DB_PTR(e, 4) != NULL)
		{
			struct BActorInstance *inst = (struct BActorInstance *)DB_PTR(e, 0);

			baUpdateAnimations(inst, (struct BActorNode *)DB_PTR(inst, 0x134), dt, NULL);
			baPrepActorInstance((struct BActorInstance *)DB_PTR(e, 0), 1);
		}
	}
	for (i = 0; i < DB_I32(w, 0x10f8); i++)
	{
		char *e = w + 0xd68 + i * 0x40;

		if (DB_PTR(e, 4) != NULL)
		{
			struct BActorInstance *inst = (struct BActorInstance *)DB_PTR(e, 0);

			baUpdateAnimations(inst, (struct BActorNode *)DB_PTR(inst, 0x134), dt, NULL);
			baPrepActorInstance((struct BActorInstance *)DB_PTR(e, 0), 1);
		}
	}
}

/* the universe sets (pass 1, and pass 2 when `both`) at their matrices */
static void db_world_draw_universe(char *w, int both)
{
	int i;

	for (i = 0; i < DB_I32(w, 0x92c); i++)
	{
		bdSetWorldMatrix(DB_FP(w, 0x8e8 + i * 0x74));
		baDrawSet((struct BSet *)DB_PTR(w, 0x928 + i * 0x74), 1);
		if (both)
			baDrawSet((struct BSet *)DB_PTR(w, 0x928 + i * 0x74), 2);
	}
}

/* the layer sets (passes 1 and 2) with render state 3 pushed to 1 */
static void db_world_draw_layers(char *w)
{
	int i;

	bdPushRenderState(3, 1, 0);
	for (i = 0; i < DB_I32(w, 0x6ac); i++)
	{
		char *layer = DB_LAYER(w, i);

		bdSetWorldMatrix(DB_FP(layer, 0x44));
		baDrawSet((struct BSet *)DB_PTR(layer, 0x84), 1);
		baDrawSet((struct BSet *)DB_PTR(layer, 0x84), 2);
	}
	bdPopRenderState(3);
}

/* the instances of one list (count at `countOff`, instance pointers from `instOff`) */
static void db_world_draw_list(char *w, int countOff, int instOff)
{
	int i;

	for (i = 0; i < DB_I32(w, countOff); i++)
		baDrawActorInstance((struct BActorInstance *)DB_PTR(w, instOff + i * 0x40), 7);
}

/* 0x000f7270  (file inferred) the solid world of variation 0: universe (both passes),
   layers, instances */
void CDemolition_World_DrawFrag(struct CDemolition_World *self)
{
	char *w = (char *)self;

	if (DB_VARIATION_HAS_OBJECTS())
		db_world_draw_universe(w, 1);
	db_world_draw_layers(w);
	if (DB_VARIATION_HAS_OBJECTS())
		db_world_draw_list(w, 0xd34, 0x9a4);
}

/* 0x000f7380  (file inferred) the "shadow" instances */
void CDemolition_World_DrawShadowObjects(struct CDemolition_World *self)
{
	char *w = (char *)self;

	if (DB_I32(w, 0x113c) != 0)
		db_world_draw_list(w, 0x113c, 0x112c);
}

/* 0x000f73c0  (file inferred) the solid world of variation 1: universe (pass 1 only),
   layers, instances */
void CDemolition_World_DrawRemote(struct CDemolition_World *self)
{
	char *w = (char *)self;

	if (DB_VARIATION_HAS_OBJECTS())
		db_world_draw_universe(w, 0);
	db_world_draw_layers(w);
	if (DB_VARIATION_HAS_OBJECTS())
		db_world_draw_list(w, 0xd34, 0x9a4);
}

/* 0x000f74c0  (file inferred) the alpha instances of variation 1 (render state 3 off, and
   10 off while they draw) */
void CDemolition_World_DrawAlphaRemote(struct CDemolition_World *self)
{
	char *w = (char *)self;

	bdSetRenderState(3, 0, 0);
	if (DB_VARIATION_HAS_OBJECTS() && DB_I32(w, 0x10f8) != 0)
	{
		bdSetRenderState(10, 0, 0);
		db_world_draw_list(w, 0x10f8, 0xd68);
	}
}

/* 0x000f7540  (file inferred) the solid world of variation 2, inside a begin (0x20) / end of
   the game's Direct3D helper (game +0x79c8) */
void CDemolition_World_DrawTimed(struct CDemolition_World *self)
{
	char *w = (char *)self;
	void *helper;

	if (DB_VARIATION_HAS_OBJECTS())
	{
		/* 0x0018e540 is a thiscall (ECX = helper) that never reads ECX; its only argument is
		   the alpha (original 0x000f7562: push 0x20). */
		FUN_0018e540(0x20);
		db_world_draw_universe(w, 0);
	}
	db_world_draw_layers(w);
	if (!DB_VARIATION_HAS_OBJECTS())
		return;
	db_world_draw_list(w, 0xd34, 0x9a4);
	if (DB_I32(w, 0x10f8) != 0)
	{
		bdSetRenderState(10, 1, 0);
		db_world_draw_list(w, 0x10f8, 0xd68);
	}
	helper = DAT_002ff85c + 0x79c8;
	DB_DUAL(FUN_001bea60, void (*)(void), void (*)(void *), (), (helper));
}

/* 0x000f76b0  (file inferred) the world of variation 3 (no layers): universe (pass 1),
   instances, alpha instances */
void CDemolition_World_DrawArena(struct CDemolition_World *self)
{
	char *w = (char *)self;

	if (DB_VARIATION_HAS_OBJECTS())
		db_world_draw_universe(w, 0);
	if (!DB_VARIATION_HAS_OBJECTS())
		return;
	db_world_draw_list(w, 0xd34, 0x9a4);
	if (DB_I32(w, 0x10f8) != 0)
	{
		bdSetRenderState(10, 1, 0);
		db_world_draw_list(w, 0x10f8, 0xd68);
	}
}

/* 0x000f77a0  (file inferred) deleting destructor (vtable 0x0026b0f8 slot 0) */
void *CDemolition_World_DeletingDestruct(struct CDemolition_World *self, unsigned int flags)
{
	DB_PTR(self, 0) = (void *)vtable_0026b0f8;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000f77c0  (file inferred; missing from the function table)
   ParseMap callback of a "World" block: a level layer <level>\<Name>.obe with its collision
   <level>\<Name>_col.obe and the collision objects <level>\collide.obe (charlib Actor_Load
   at the block's position), placed by the block's transform. Returns 2 (stop) on a missing
   set or when 10 layers are loaded, else 0. */
int FUN_000f77c0(int node, char *world)
{
	char path[128];
	float pos[4], rot[4], scale[4];
	float translation[16], rotation[16];
	const char *name;
	char *layer;
	int count;

	name = ParseMap_GetString(node, "Name");
	sprintf(path, "%s\\%s.obe", world + 4, name);
	_strlwr(path);
	db_read_transform(node, pos, rot, scale);
	layer = DB_LAYER(world, DB_I32(world, 0x6ac));
	memcpy(layer + 0x04, pos, 16);
	memcpy(layer + 0x14, rot, 16);
	memcpy(layer + 0x24, scale, 16);
	DB_PTR(layer, 0x84) = bLoadResource((struct bkPackage *)DB_PTR(world, 0x108), path, 6, 0xdefa);
	if (DB_PTR(layer, 0x84) == NULL)
	{
		bkPrintf("*** ERROR *** Could not find level [%s]\n", path);
		return 2;
	}
	sprintf(path, "%s\\%s_col.obe", world + 4, name);
	layer = DB_LAYER(world, DB_I32(world, 0x6ac));
	DB_PTR(layer, 0x88) = bLoadResource((struct bkPackage *)DB_PTR(world, 0x108), path, 7, 0xdefa);
	if (DB_PTR(layer, 0x88) == NULL)
	{
		bkPrintf("ERROR: Collision mesh [%s] not found\n", path);
		return 2;
	}
	sprintf(path, "%s\\collide.obe", world + 4);
	Actor_Load(path, DB_PTR(world, 0x108), pos, 0, NULL, 0);
	bmMatrixTranslation(translation, pos[0], pos[1], pos[2]);
	bmQuaternionToMatrix(rotation, rot);
	layer = DB_LAYER(world, DB_I32(world, 0x6ac));
	db_world_matrix(DB_FP(layer, 0x44), translation, rotation,
		(uintptr_t)world | (uintptr_t)rotation | (uintptr_t)translation);
	DB_U8(DB_LAYER(world, DB_I32(world, 0x6ac)), 0) = 1;
	count = DB_I32(world, 0x6ac) + 1;
	DB_I32(world, 0x6ac) = count;
	return count < 10 ? 0 : 2;
}

/* 0x000f7b90  Demolition/CDemolition_World.cpp
   ParseMap callback of an "Instance" of the current "Object" block (+0x84). By name:
     start<n>   start position n (0-3); stops after the fourth
     universe   a universe set objects\<object>.obe at the block's transform
     hazbase    a hazard light (demolition_a, 0x550 bytes) added to the lights (game +0x6ca4)
     light      nothing (the original only looks for "point")
     corona     a corona of colour (R, G, B; default 127) in the lights
     steam, collide, jet   nothing
     platform   a layer objects\<object>.obe (+ _col.obe) at the origin with the rotation
     crate      a crate (demolition_d, 0x54 bytes) turned by the rotation's angle, added to
                the platforms (game +0x79a4)
     else       an instance of objects\<object>.obe (loaded with the block's first instance)
                in the "alpha", "shadow", "decal" or the solid list, with its "default"
                animation and ZBIAS
   Returns 2 on an error, else 0. */
int CDemolition_World_ParseInstance(int node, char *world)
{
	char path[128];
	float pos[4], rot[4], scale[4];
	float translation[16], rotation[16];
	float axisAngle[4];
	float turn[4];
	char *name;
	char *entry;
	char *layer;
	int index;
	int n;
	int r, g, b;
	float zbias;
	void *object;

	path[0] = '\0';   /* the original may print it before it is filled */
	name = ParseMap_GetString(node, "Name");
	_strlwr(name);
	db_read_transform(node, pos, rot, scale);

	if (strstr(name, "start") != NULL)
	{
		index = (int)atol(name + 5);
		if (index >= 4)
		{
			bkPrintf("*** ERROR *** Invalid index for start position [%d]\n", index);
			return 2;
		}
		memcpy(world + 0x6b0 + index * 0x20, pos, 16);
		memcpy(world + 0x6c0 + index * 0x20, rot, 16);
		DB_I32(world, 0x730)++;
		if (DB_I32(world, 0x730) >= 4)
			return 2;
	}
	else if (strstr(name, "universe") != NULL)
	{
		sprintf(path, "%s\\objects\\%s.obe", world + 4, world + 0x84);
		_strlwr(path);
		n = DB_I32(world, 0x92c);
		entry = world + 0x8b8 + n * 0x74;
		DB_PTR(entry, 0x70) = bLoadResource((struct bkPackage *)DB_PTR(world, 0x108), path, 6, 0xdefa);
		if (DB_PTR(entry, 0x70) == NULL)
		{
			bkPrintf("*** ERROR *** Could not load universe object [%s]\n", path);
			return 2;
		}
		memcpy(entry + 0x00, pos, 16);
		memcpy(entry + 0x10, rot, 16);
		memcpy(entry + 0x20, scale, 16);
		bmMatrixTranslation(translation, pos[0], pos[1], pos[2]);
		bmQuaternionToMatrix(rotation, rot);
		db_world_matrix(DB_FP(entry, 0x30), translation, rotation,
			(uintptr_t)DB_FP(entry, 0x30) | (uintptr_t)rotation | (uintptr_t)translation);
		DB_I32(world, 0x92c)++;
	}
	else if (strstr(name, "hazbase") != NULL)
	{
		object = bkHeapAlloc(0x550, DB_WORLD_FILE, 0x183, 0x1001);
		if (object != NULL)
			object = CDemoHazardLight_Ctor(object);
		DB_VFN(object, 4, void (*)(void *))(object);
		CDemoLight_SetPosition((int)(uintptr_t)object, (unsigned int *)pos);
		CDemoLightSystem_Add(DAT_002ff85c + 0x6ca4, object);
	}
	else if (strstr(name, "light") != NULL)
	{
		(void)strstr(name, "point");   /* the original's result is not used */
	}
	else if (strstr(name, "corona") != NULL)
	{
		if (ParseMap_GetInt(node, "R", (undefined4 *)&r) == 0)
			r = 0x7f;
		if (ParseMap_GetInt(node, "G", (undefined4 *)&g) == 0)
			g = 0x7f;
		if (ParseMap_GetInt(node, "B", (undefined4 *)&b) == 0)
			b = 0x7f;
		CDemoLightSystem_AddSpark(DAT_002ff85c + 0x6ca4, pos, r, g, b);
	}
	else if (strstr(name, "steam") != NULL || strstr(name, "collide") != NULL ||
		strstr(name, "jet") != NULL)
	{
		/* nothing */
	}
	else if (strstr(name, "platform") != NULL)
	{
		n = DB_I32(world, 0x6ac);
		sprintf(path, "%s\\objects\\%s.obe", world + 4, world + 0x84);
		_strlwr(path);
		layer = DB_LAYER(world, n);
		DB_PTR(layer, 0x84) = bLoadResource((struct bkPackage *)DB_PTR(world, 0x108), path, 6, 0xdefa);
		if (DB_PTR(layer, 0x84) == NULL)
		{
			bkPrintf("*** ERROR *** Could not find level [%s]\n", path);
			return 2;
		}
		sprintf(path, "%s\\objects\\%s_col.obe", world + 4, world + 0x84);
		_strlwr(path);
		DB_PTR(DB_LAYER(world, DB_I32(world, 0x6ac)), 0x88) =
			bLoadResource((struct bkPackage *)DB_PTR(world, 0x108), path, 7, 0xdefa);
		if (DB_PTR(DB_LAYER(world, DB_I32(world, 0x6ac)), 0x88) == NULL)
		{
			bkPrintf("ERROR: Collision mesh [%s] not found\n", path);
			return 2;
		}
		DB_U32(DB_LAYER(world, DB_I32(world, 0x6ac)), 0x8c) = bkCRCString(name);
		pos[0] = 0.0f;
		pos[1] = 0.0f;
		pos[2] = 0.0f;
		memcpy(layer + 0x04, pos, 16);
		memcpy(layer + 0x14, rot, 16);
		memcpy(layer + 0x24, scale, 16);
		bmMatrixTranslation(translation, pos[0], pos[1], pos[2]);
		bmQuaternionToMatrix(rotation, rot);
		db_world_matrix(DB_FP(layer, 0x44), translation, rotation,
			(uintptr_t)world | (uintptr_t)rotation | (uintptr_t)translation);
		DB_U8(DB_LAYER(world, DB_I32(world, 0x6ac)), 0) = 1;
		DB_I32(world, 0x6ac)++;
	}
	else if (strstr(name, "crate") != NULL)
	{
		object = bkHeapAlloc(0x54, DB_WORLD_FILE, 0x23f, 0x1001);
		if (object != NULL)
			object = DemoCrate_Ctor(object);
		DB_VFN(object, 4, void (*)(void *))(object);
		CDemoPlatform_SetColour(object, pos);
		bmQuaternionToAxisAngle(axisAngle, rot);
		turn[0] = 0.0f;
		turn[1] = axisAngle[3];
		turn[2] = 0.0f;
		turn[3] = 0.0f;   /* uninitialised in the original */
		CDemoPlatform_SetPosition(object, turn);
		FUN_0018e010((void *)(uintptr_t)(DAT_002ff85c + 0x79a4), (void *)(uintptr_t)object);
	}
	else
	{
		if (DB_U8(world, 0x104) != 0)
		{
			n = DB_I32(world, 0x970);
			sprintf(path, "%s\\objects\\%s.obe", world + 4, world + 0x84);
			_strlwr(path);
			DB_PTR(world, 0x930 + n * 8) =
				bLoadResource((struct bkPackage *)DB_PTR(world, 0x108), path, 1, 0xdefa);
			if (DB_PTR(world, 0x930 + n * 8) == NULL)
			{
				bkPrintf("*** ERROR *** Could not load actor [%s]\n", path);
				return 2;
			}
			_strlwr(world + 0x84);
			DB_U32(world, 0x934 + n * 8) = bkCRCString(world + 0x84);
			DB_I32(world, 0x970)++;
		}
		if (strstr(name, "alpha") != NULL)
			entry = world + 0xd38 + DB_I32(world, 0x10f8)++ * 0x40;
		else if (strstr(name, "shadow") != NULL)
			entry = world + 0x10fc + DB_I32(world, 0x113c)++ * 0x40;
		else if (strstr(name, "decal") != NULL)
			entry = world + 0x1140 + DB_I32(world, 0x1180)++ * 0x40;
		else
			entry = world + 0x974 + DB_I32(world, 0xd34)++ * 0x40;
		/* the model loaded last */
		DB_PTR(entry, 0x30) = baInitActorInstance(NULL,
			(struct BActor *)DB_PTR(world, 0x928 + DB_I32(world, 0x970) * 8), 1, DB_WORLD_FILE,
			0x28c);
		if (DB_PTR(entry, 0x30) == NULL)
		{
			bkPrintf("*** ERROR *** Could not create actor instance [%s]\n", path);
			return 2;
		}
		memcpy(entry + 0x00, pos, 16);
		memcpy(entry + 0x10, rot, 16);
		memcpy(entry + 0x20, scale, 16);
		memcpy(DB_ADDR(DB_PTR(entry, 0x30), 0xc0), pos, 16);
		memcpy(DB_ADDR(DB_PTR(entry, 0x30), 0xe0), rot, 16);
		memcpy(DB_ADDR(DB_PTR(entry, 0x30), 0xd0), scale, 16);
		baPrepActorInstance((struct BActorInstance *)DB_PTR(entry, 0x30), 1);
		DB_PTR(entry, 0x34) = baFindSubObject((struct BActorInstance *)DB_PTR(entry, 0x30), "default");
		if (DB_PTR(entry, 0x34) != NULL)
		{
			struct BActorInstance *inst = (struct BActorInstance *)DB_PTR(entry, 0x30);

			baFlushAnimations(inst, (struct BActorNode *)DB_PTR(inst, 0x134), 1, 0);
			inst = (struct BActorInstance *)DB_PTR(entry, 0x30);
			baQueueAnimation(inst, (struct BActorNode *)DB_PTR(inst, 0x134),
				(struct BAnim *)DB_PTR(entry, 0x34), 0x400, -1, 1, NULL, NULL);
		}
		DB_U32(entry, 0x38) = bkCRCString(name);
		if (ParseMap_GetFloat(node, "ZBIAS", (undefined4 *)&zbias) == 0)
			zbias = 0.0f;
		DB_F32(entry, 0x3c) = zbias;
	}
	DB_U8(world, 0x104) = 0;
	return 0;
}

/* 0x000f86d0  (file inferred) draws the solid world of the variation (render state 6 off) */
void CDemolition_World_DrawSolid(struct CDemolition_World *self)
{
	bdSetRenderState(6, 0, 0);
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
		CDemolition_World_DrawFrag(self);
		break;
	case 1:
		CDemolition_World_DrawRemote(self);
		break;
	case 2:
		CDemolition_World_DrawTimed(self);
		break;
	case 3:
		CDemolition_World_DrawArena(self);
		break;
	}
}

/* 0x000f8730  (file inferred) draws the alpha instances (variation 1 only) */
void CDemolition_World_DrawAlpha(struct CDemolition_World *self)
{
	if (DB_I32(DAT_002ff85c, 0x28) == 1)
		CDemolition_World_DrawAlphaRemote(self);
}

/* 0x000f8750  (file inferred) draws the "shadow" instances (all variations) */
void CDemolition_World_DrawShadow(struct CDemolition_World *self)
{
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
	case 1:
	case 2:
	case 3:
		CDemolition_World_DrawShadowObjects(self);
		break;
	}
}

/* 0x000f8790  (file inferred) the "decal" instances, alpha blended (D3D render states
   BLENDOP (74) = ADD, SRCBLEND (62) = ZERO, DESTBLEND (63) = SRCCOLOR, set inline in the
   original) */
void CDemolition_World_DrawDecalObjects(struct CDemolition_World *self)
{
	char *w = (char *)self;

	if (DB_I32(w, 0x1180) == 0)
		return;
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(7, 1, 0);
	bdPushRenderState(5, 0, 0);
	D3DDevice_SetRenderState_Simple(0x40350, 0x8006);
	D3D__RenderState[74] = 0x8006;
	D3DDevice_SetRenderState_Simple(0x40344, 0);
	D3D__RenderState[62] = 0;
	D3DDevice_SetRenderState_Simple(0x40348, 0x301);
	D3D__RenderState[63] = 0x301;
	db_world_draw_list(w, 0x1180, 0x1170);
	bdPopRenderState(7);
	bdPopRenderState(5);
	bdPopRenderState(6);
}

/* 0x000f8850  (file inferred; missing from the function table)
   ParseMap callback of an "Object" block: remembers its name and walks its "Instance"
   children. Returns 2. */
int FUN_000f8850(int node, char *world)
{
	strcpy(world + 0x84, ParseMap_GetString(node, "Name"));
	DB_U8(world, 0x104) = 1;
	ParseMap_Walk(node, (int)(uintptr_t)CDemolition_World_ParseInstance, (undefined4)(uintptr_t)world,
		(int)(uintptr_t)"Instance");
	return 2;
}

/* 0x000f88a0  (file inferred) draws the decal instances (render state 4 pushed to 1) */
void CDemolition_World_DrawDecals(struct CDemolition_World *self)
{
	bdPushRenderState(4, 1, 0);
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
	case 1:
	case 2:
	case 3:
		CDemolition_World_DrawDecalObjects(self);
		break;
	}
	bdPopRenderState(4);
}

/* 0x000f8930  (file inferred) loads and parses the LOM file `name` from `package`: the
   "World" and "Object" blocks and the intro camera. Returns 0 when it cannot be loaded or
   parsed. */
int CDemolition_World_LoadLOM(struct CDemolition_World *self, void *package, const char *name)
{
	unsigned int size;
	void *data;
	int *map;

	data = bkLoadFile((struct bkPackage *)package, name, NULL, &size, NULL, 0);
	if (data == NULL)
	{
		bkPrintf("LoadLOMFile ERROR: Could not load [%s]\n", name);
		return 0;
	}
	map = ParseMap_Load((char *)data, 2, (int)size);
	if (map == NULL)
	{
		bkPrintf("LoadLOMFile ERROR: Parsing [%s]\n", name);
		return 0;
	}
	ParseMap_WalkMap((int)(uintptr_t)map, (int)(uintptr_t)FUN_000f77c0, (undefined4)(uintptr_t)self,
		(int)(uintptr_t)"World");
	ParseMap_WalkMap((int)(uintptr_t)map, (int)(uintptr_t)FUN_000f8850, (undefined4)(uintptr_t)self,
		(int)(uintptr_t)"Object");
	CLIntroCam_Load((int)(uintptr_t)map, (undefined4)(uintptr_t)self);
	ParseMap_Free(map);
	return 1;
}

/* 0x000f89e0  (file inferred) Init: clears the world, then loads <level>\demo_<level>.lom.
   Returns nonzero when it loaded. */
int CDemolition_World_Init(struct CDemolition_World *self, void *package, const char *level)
{
	char *w = (char *)self;
	char path[256];

	memset(w, 0, 0x461 * 4);
	strcpy(w + 4, level);
	DB_PTR(w, 0x108) = package;
	sprintf(path, "%s\\demo_%s.lom", level, level);
	return CDemolition_World_LoadLOM(self, package, path) != 0;
}
