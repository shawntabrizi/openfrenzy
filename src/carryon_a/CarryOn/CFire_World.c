/*
CarryOn/CFire_World.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_World.cpp

The level of the Fireworks game (0x23c8 bytes, CFireGame.level), loaded from a LOM
script (map\<name>.lom, or test\firetest.lom with the resource's test flag) through
classlib_ui's ParseMap: "World" blocks are level layers (map\<name>.obe sets with .col
collision), "Object" blocks name an object whose "Instance" children place it, or are
special by name: the three sky domes, launch pads ("ponker<n>"), start positions
("rack": the rocket racks), glow pots, the dish, spawn points, universe sets,
platforms and lights.

0x000c6d00 is tagged with CFire_World.cpp; the functions around it work on the same
object (file inferred). The three ParseMap callbacks 0x000c5610, 0x000c59a0 and
0x000c6c00 are missing from the function table (reached only as callback addresses
from 0x000c6c50 and 0x000c6c00); they are defined here with their FUN_ names (declared
in carryon_a_types.h) and reported to the architect.

The object is reached with raw offsets (CW_*): the original's arrays share counters in
ways a C struct would hide (the universe count at +0x410 is also the second universe
entry's first word).
*/

#include "carryon_a.h"

/* the level's fields (byte offsets) */
#define CW_NAME 0x000           /* char[0x80] the level name */
#define CW_OBJNAME 0x080        /* char[0x80] the current "Object" block's name */
#define CW_NEWOBJ 0x100         /* char: the object is not loaded yet */
#define CW_PACKAGE 0x104        /* the level package */
#define CW_LAYER 0x108          /* layers, 0x88 bytes: pos[4], rot[4], scale[4], pad[4],
                                   matrix[16] +0x40, set +0x80, collision +0x84 */
#define CW_LAYER_SIZE 0x88
#define CW_NUM_LAYERS 0x2a0
#define CW_PAD 0x2a4            /* launch pads, 0x14 bytes: pos[4], yaw */
#define CW_NUM_PADS 0x344
#define CW_START 0x348          /* start positions (racks), 0x14 bytes: pos[4], yaw */
#define CW_NUM_STARTS 0x398
#define CW_UNIVERSE 0x39c       /* universe sets, 0x74 bytes: pos[4], rot[4], scale[4],
                                   matrix[16] +0x30, set +0x70 */
#define CW_UNIVERSE_SIZE 0x74
#define CW_NUM_UNIVERSE 0x410
#define CW_OBJECT 0x414         /* objects, 8 bytes: model, name CRC */
#define CW_NUM_OBJECTS 0x514
#define CW_SOLID 0x518          /* instances, 0x3c bytes: pos[4], rot[4], scale[4], inst
                                   +0x30, anim +0x34, name CRC +0x38 */
#define CW_INSTANCE_SIZE 0x3c
#define CW_NUM_SOLID 0x1418
#define CW_ALPHA 0x141c         /* alpha / fuse mesh instances, as CW_SOLID */
#define CW_NUM_ALPHA 0x231c
#define CW_NUM_SPAWNS 0x2320
#define CW_SPAWN 0x2324         /* spawn points (charlib RespawnPoint), by index */
#define CW_SKY_MODEL 0x2354     /* skyback, skyadd, skysub models */
#define CW_SKY_INST 0x2360      /* their instances */
#define CW_SKY_ANGLE 0x236c     /* their rotations */
#define CW_GLOW_POT 0x2368      /* glow pots by number (1..4), float[4] each */
#define CW_DISH 0x23b8          /* "thedish" position */

/* the original multiplies with the copy variant when the pointers are 16-byte aligned
(the same product) */
static void co_WorldMatrix(float *out, const float *a, const float *b)
{
	if ((((unsigned int)out | (unsigned int)a | (unsigned int)b) & 0xf) == 0)
		bmMatrixMultiplyCopy(out, a, b);
	else
		bmMatrixMultiply(out, a, b);
}

/* reads PosX/Y/Z (0), RotX/Y/Z/W (0) and SclX/Y/Z (1) of a ParseMap node */
static void co_ReadTransform(int node, float *pos, float *rot, float *scale)
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

/* the yaw of a rotation: the axis-angle's angle, negated (into -pi..pi) when the axis
points down */
static float co_YawOf(float *rot)
{
	float axisAngle[4];
	float yaw;

	bmQuaternionToAxisAngle(axisAngle, rot);
	yaw = axisAngle[3];
	if (axisAngle[1] < 0.0f)
	{
		yaw = -yaw;
		while (yaw < -3.1415925f)                           /* 0x00253f3c */
			yaw = yaw + 6.2831855f;                         /* 0x00255930 */
		while (!(yaw < 3.1415925f))                         /* 0x00256750 */
			yaw = yaw - 6.2831855f;
	}
	return yaw;
}

/* 0x000c5610  CarryOn/CFire_World.cpp (missing from the function table)
ParseMap callback of a "World" block: a level layer map\<Name>.obe (a set) with its
collision map\<Name>.col, placed by the block's transform. Returns 2 (stop) when the
set is missing or when 3 layers are loaded, else 0. */
int FUN_000c5610(int node, unsigned char *world)
{
	char name[128];
	float pos[4], rot[4], scale[4];
	float translation[16], rotation[16];
	unsigned char *layer;
	const char *levelName;
	int count;

	levelName = ParseMap_GetString(node, "Name");
	sprintf(name, CO_I32(g_fireRes, 0x258) == 0 ? "map\\%s.obe" : "test\\%s.obe", levelName);
	_strlwr(name);
	co_ReadTransform(node, pos, rot, scale);
	layer = world + CO_I32(world, CW_NUM_LAYERS) * CW_LAYER_SIZE + CW_LAYER;
	memcpy(layer + 0x00, pos, 16);
	memcpy(layer + 0x10, rot, 16);
	memcpy(layer + 0x20, scale, 16);
	CO_PTR(layer, 0x80) = bLoadResource(CO_PTR(world, CW_PACKAGE), name, 6, 0xdefa);
	if (CO_PTR(layer, 0x80) == NULL)
	{
		bkPrintf("*** ERROR *** Could not find level [%s]\n", name);
		return 2;
	}
	sprintf(name, CO_I32(g_fireRes, 0x258) == 0 ? "map\\%s.col" : "test\\%s.col", levelName);
	_strlwr(name);
	CO_PTR(layer, 0x84) = bLoadResource(CO_PTR(world, CW_PACKAGE), name, 7, 0xdefa);
	bmMatrixTranslation(translation, pos[0], pos[1], pos[2]);
	bmQuaternionToMatrix(rotation, rot);
	co_WorldMatrix(CO_FP(layer, 0x40), translation, rotation);
	count = CO_I32(world, CW_NUM_LAYERS) + 1;
	CO_I32(world, CW_NUM_LAYERS) = count;
	return count < 3 ? 0 : 2;
}

/* 0x000c59a0  CarryOn/CFire_World.cpp (missing from the function table)
ParseMap callback of an "Instance" of the current "Object" block (CW_OBJNAME): see the
top of this file for the special names; anything else is an instance of the object
model objects\<object>.obe (loaded with the block's first instance), solid or (names
with "alpha" / "fusemesh") drawn with alpha. Returns 2 on a missing model, else 0. */
int FUN_000c59a0(int node, unsigned char *world)
{
	char path[128];
	float pos[4], rot[4], scale[4];
	float translation[16], rotation[16];
	char *name;
	unsigned char *entry;
	void *inst;
	int index;
	int n;

	path[0] = '\0';                 /* (the original may print it before it is filled) */
	name = ParseMap_GetString(node, "Name");
	_strlwr(name);
	co_ReadTransform(node, pos, rot, scale);

	if (strstr(name, "skyback") != NULL)
	{
		CO_PTR(world, CW_SKY_MODEL + 0) = bLoadResource(CO_PTR(world, CW_PACKAGE), "objects\\skyback.obe", 1, 0xdefa);
		if (CO_PTR(world, CW_SKY_MODEL + 0) != NULL)
		{
			inst = baInitActorInstance(NULL, CO_PTR(world, CW_SKY_MODEL + 0), 1, CO_FILE_WORLD, 0x187);
			CO_PTR(world, CW_SKY_INST + 0) = inst;
			memcpy(CO_ADDR(inst, 0xc0), pos, 16);
		}
	}
	else if (strstr(name, "skyadd") != NULL)
	{
		CO_PTR(world, CW_SKY_MODEL + 4) = bLoadResource(CO_PTR(world, CW_PACKAGE), "objects\\skyadd.obe", 1, 0xdefa);
		if (CO_PTR(world, CW_SKY_MODEL + 4) != NULL)
		{
			inst = baInitActorInstance(NULL, CO_PTR(world, CW_SKY_MODEL + 4), 1, CO_FILE_WORLD, 0x191);
			CO_PTR(world, CW_SKY_INST + 4) = inst;
			memcpy(CO_ADDR(inst, 0xc0), pos, 16);
		}
	}
	else if (strstr(name, "skysub") != NULL)
	{
		CO_PTR(world, CW_SKY_MODEL + 8) = bLoadResource(CO_PTR(world, CW_PACKAGE), "objects\\skysub.obe", 1, 0xdefa);
		if (CO_PTR(world, CW_SKY_MODEL + 8) != NULL)
		{
			inst = baInitActorInstance(NULL, CO_PTR(world, CW_SKY_MODEL + 8), 1, CO_FILE_WORLD, 0x19b);
			CO_PTR(world, CW_SKY_INST + 8) = inst;
			memcpy(CO_ADDR(inst, 0xc0), pos, 16);
		}
	}
	else if (strstr(name, "ponker") != NULL)
	{
		index = atol(name + 6) - 1;
		bkPrintf("We have a ponker %d!\n", index);
		entry = world + CW_PAD + index * 0x14;
		memcpy(entry, pos, 16);
		CO_F32(entry, 0x10) = co_YawOf(rot);
		CO_I32(world, CW_NUM_PADS) = CO_I32(world, CW_NUM_PADS) + 1;
	}
	else if (strstr(name, "rack") != NULL)
	{
		index = atol(name + 1) - 1;
		bkPrintf("We have a frocket %s -> %d!\n", name, index);
		if (index >= 4)
			bkPrintf("*** ERROR *** Invalid index for start position [%d]\n", index);
		else
		{
			entry = world + CW_START + index * 0x14;
			memcpy(entry, pos, 16);
			CO_F32(entry, 0x10) = co_YawOf(rot);
			/* stand 200 in front of the rack */
			CO_F32(entry, 0x0) = sinf(CO_F32(entry, 0x10)) * 200.0f + CO_F32(entry, 0x0);
			CO_F32(entry, 0x8) = cosf(CO_F32(entry, 0x10)) * 200.0f + CO_F32(entry, 0x8);
			CO_I32(world, CW_NUM_STARTS) = CO_I32(world, CW_NUM_STARTS) + 1;
		}
	}
	else if (strstr(name, "glow_pot") != NULL)
	{
		n = atol(name + 8);
		memcpy(world + CW_GLOW_POT + n * 0x10, pos, 16);
	}
	else if (strstr(name, "thedish") != NULL)
		memcpy(world + CW_DISH, pos, 16);
	else if (strstr(name, "spawn") != NULL)
	{
		index = atol(name + 5) - 1;
		bkPrintf("New Spawn. Index %d at %2.0f %2.0f %2,0f\n", index, (double)pos[0],
			(double)pos[1], (double)pos[2]);
		inst = bkHeapAlloc(0x20, CO_FILE_WORLD, 0x244, 0x1001);
		CO_PTR(world, CW_SPAWN + index * 4) = inst == NULL ? NULL : RespawnPoint_Init(inst, pos, 0);
		CO_I32(world, CW_NUM_SPAWNS) = CO_I32(world, CW_NUM_SPAWNS) + 1;
	}
	else if (strstr(name, "universe") != NULL)
	{
		sprintf(path, "objects\\%s.obe", (char *)world + CW_OBJNAME);
		_strlwr(path);
		n = CO_I32(world, CW_NUM_UNIVERSE);
		entry = world + n * CW_UNIVERSE_SIZE + CW_UNIVERSE;
		CO_PTR(entry, 0x70) = bLoadResource(CO_PTR(world, CW_PACKAGE), path, 6, 0xdefa);
		if (CO_PTR(entry, 0x70) == NULL)
		{
			bkPrintf("*** ERROR *** Could not load universe object [%s]\n", path);
			return 2;
		}
		memcpy(entry + 0x00, pos, 16);
		memcpy(entry + 0x10, rot, 16);
		memcpy(entry + 0x20, scale, 16);
		bmMatrixTranslation(translation, pos[0], pos[1], pos[2]);
		bmQuaternionToMatrix(rotation, rot);
		co_WorldMatrix(CO_FP(entry, 0x30), translation, rotation);
		CO_I32(world, CW_NUM_UNIVERSE) = CO_I32(world, CW_NUM_UNIVERSE) + 1;
	}
	else if (strstr(name, "platform") != NULL)
	{
		n = CO_I32(world, CW_NUM_LAYERS);
		sprintf(path, "objects\\%s.obe", (char *)world + CW_OBJNAME);
		_strlwr(path);
		entry = world + n * CW_LAYER_SIZE + CW_LAYER;
		CO_PTR(entry, 0x80) = bLoadResource(CO_PTR(world, CW_PACKAGE), path, 6, 0xdefa);
		if (CO_PTR(entry, 0x80) == NULL)
		{
			bkPrintf("*** ERROR *** Could not find level [%s]\n", path);
			return 2;
		}
		sprintf(path, "objects\\%s.col", (char *)world + CW_OBJNAME);
		_strlwr(path);
		CO_PTR(world, CO_I32(world, CW_NUM_LAYERS) * CW_LAYER_SIZE + CW_LAYER + 0x84) =
			bLoadResource(CO_PTR(world, CW_PACKAGE), path, 7, 0xdefa);
		if (CO_PTR(world, CO_I32(world, CW_NUM_LAYERS) * CW_LAYER_SIZE + CW_LAYER + 0x84) == NULL)
		{
			bkPrintf("ERROR: Collision mesh [%s] not found\n", path);
			return 2;
		}
		/* platforms sit at the origin */
		pos[1] = 0.0f;
		pos[2] = 0.0f;
		pos[0] = 0.0f;
		memcpy(entry + 0x00, pos, 16);
		memcpy(entry + 0x10, rot, 16);
		memcpy(entry + 0x20, scale, 16);
		bmMatrixTranslation(translation, pos[0], pos[1], pos[2]);
		bmQuaternionToMatrix(rotation, rot);
		co_WorldMatrix(CO_FP(entry, 0x40), translation, rotation);
		CO_I32(world, CW_NUM_LAYERS) = CO_I32(world, CW_NUM_LAYERS) + 1;
	}
	else if (strstr(name, "light") == NULL)
	{
		/* an instance of the object (the first instance loads its model) */
		if (CO_U8(world, CW_NEWOBJ) != 0)
		{
			n = CO_I32(world, CW_NUM_OBJECTS);
			bkPrintf("Object: %s\n", (char *)world + CW_OBJNAME);
			sprintf(path, "objects\\%s.obe", (char *)world + CW_OBJNAME);
			_strlwr(path);
			CO_PTR(world, CW_OBJECT + n * 8) = bLoadResource(CO_PTR(world, CW_PACKAGE), path, 1, 0xdefa);
			if (CO_PTR(world, CW_OBJECT + n * 8) == NULL)
				bkPrintf("*** WARNING *** Could not load actor [%s]\n", path);
			_strlwr((char *)world + CW_OBJNAME);
			CO_U32(world, CW_OBJECT + n * 8 + 4) = bkCRCString((char *)world + CW_OBJNAME);
			CO_I32(world, CW_NUM_OBJECTS) = CO_I32(world, CW_NUM_OBJECTS) + 1;
		}
		if (CO_PTR(world, CW_OBJECT - 8 + CO_I32(world, CW_NUM_OBJECTS) * 8) != NULL)
		{
			bkPrintf("Instance of %s\n", (char *)world + CW_OBJNAME);
			if (strstr(name, "alpha") == NULL && strstr(name, "fusemesh") == NULL)
			{
				n = CO_I32(world, CW_NUM_SOLID);
				entry = world + n * CW_INSTANCE_SIZE + CW_SOLID;
				CO_I32(world, CW_NUM_SOLID) = n + 1;
			}
			else
			{
				n = CO_I32(world, CW_NUM_ALPHA);
				entry = world + n * CW_INSTANCE_SIZE + CW_ALPHA;
				CO_I32(world, CW_NUM_ALPHA) = n + 1;
			}
			inst = baInitActorInstance(NULL,
				CO_PTR(world, CW_OBJECT - 8 + CO_I32(world, CW_NUM_OBJECTS) * 8), 1, CO_FILE_WORLD, 0x308);
			CO_PTR(entry, 0x30) = inst;
			if (inst == NULL)
			{
				bkPrintf("*** ERROR *** Could not create actor instance [%s]\n", path);
				return 2;
			}
			memcpy(entry + 0x00, pos, 16);
			memcpy(entry + 0x10, rot, 16);
			memcpy(entry + 0x20, scale, 16);
			memcpy(CO_ADDR(inst, 0xc0), pos, 16);
			memcpy(CO_ADDR(inst, 0xe0), rot, 16);
			memcpy(CO_ADDR(inst, 0xd0), scale, 16);
			CO_PTR(entry, 0x34) = baFindSubObject(inst, "default");
			if (CO_PTR(entry, 0x34) != NULL)
				baQueueAnimation(inst, CO_PTR(inst, 0x134), CO_PTR(entry, 0x34), 0x400, -1, 1, NULL, NULL);
			else
			{
				CO_PTR(entry, 0x34) = baFindSubObject(inst, "default_noloop");
				if (CO_PTR(entry, 0x34) != NULL)
					baQueueAnimation(inst, CO_PTR(inst, 0x134), CO_PTR(entry, 0x34), 0x400, -1, 0, NULL, NULL);
			}
			baPrepActorInstance(inst, 1);
			CO_U32(entry, 0x38) = bkCRCString(name);
		}
	}
	CO_U8(world, CW_NEWOBJ) = 0;
	return 0;
}

/* 0x000c6640  CarryOn/CFire_World.cpp (file inferred)
Frees the level's models and instances. */
void CFire_World_Unload(void *world)
{
	unsigned char *w = world;
	int i;

	for (i = 0; i < CO_I32(w, CW_NUM_UNIVERSE); i++)
	{
		if (CO_PTR(w, CW_UNIVERSE + 0x70 + i * CW_UNIVERSE_SIZE) != NULL)
		{
			bkDeleteResource(CO_PTR(w, CW_UNIVERSE + 0x70 + i * CW_UNIVERSE_SIZE));
			CO_PTR(w, CW_UNIVERSE + 0x70 + i * CW_UNIVERSE_SIZE) = NULL;
		}
	}
	for (i = 0; i < CO_I32(w, CW_NUM_SOLID); i++)
	{
		if (CO_PTR(w, CW_SOLID + 0x30 + i * CW_INSTANCE_SIZE) != NULL)
		{
			baDeleteActorInstance(CO_PTR(w, CW_SOLID + 0x30 + i * CW_INSTANCE_SIZE));
			CO_PTR(w, CW_SOLID + 0x30 + i * CW_INSTANCE_SIZE) = NULL;
		}
	}
	for (i = 0; i < CO_I32(w, CW_NUM_ALPHA); i++)
	{
		if (CO_PTR(w, CW_ALPHA + 0x30 + i * CW_INSTANCE_SIZE) != NULL)
		{
			baDeleteActorInstance(CO_PTR(w, CW_ALPHA + 0x30 + i * CW_INSTANCE_SIZE));
			CO_PTR(w, CW_ALPHA + 0x30 + i * CW_INSTANCE_SIZE) = NULL;
		}
	}
	for (i = 0; i < CO_I32(w, CW_NUM_OBJECTS); i++)
	{
		if (CO_PTR(w, CW_OBJECT + i * 8) != NULL)
		{
			bkDeleteResource(CO_PTR(w, CW_OBJECT + i * 8));
			CO_PTR(w, CW_OBJECT + i * 8) = NULL;
		}
	}
	for (i = 0; i < CO_I32(w, CW_NUM_LAYERS); i++)
	{
		if (CO_PTR(w, CW_LAYER + 0x80 + i * CW_LAYER_SIZE) != NULL)
		{
			bkDeleteResource(CO_PTR(w, CW_LAYER + 0x80 + i * CW_LAYER_SIZE));
			CO_PTR(w, CW_LAYER + 0x80 + i * CW_LAYER_SIZE) = NULL;
		}
		if (CO_PTR(w, CW_LAYER + 0x84 + i * CW_LAYER_SIZE) != NULL)
		{
			bkDeleteResource(CO_PTR(w, CW_LAYER + 0x84 + i * CW_LAYER_SIZE));
			CO_PTR(w, CW_LAYER + 0x84 + i * CW_LAYER_SIZE) = NULL;
		}
	}
	for (i = 0; i < 3; i++)
	{
		if (CO_PTR(w, CW_SKY_INST + i * 4) != NULL)
		{
			baDeleteActorInstance(CO_PTR(w, CW_SKY_INST + i * 4));
			CO_PTR(w, CW_SKY_INST + i * 4) = NULL;
		}
		if (CO_PTR(w, CW_SKY_MODEL + i * 4) != NULL)
		{
			bkDeleteResource(CO_PTR(w, CW_SKY_MODEL + i * 4));
			CO_PTR(w, CW_SKY_MODEL + i * 4) = NULL;
		}
	}
}

/* 0x000c67a0  CarryOn/CFire_World.cpp (file inferred)
Animates the instances and turns the three sky domes. */
void CFire_World_Update(void *world, float step)
{
	unsigned char *w = world;
	void *inst;
	int i;

	for (i = 0; i < CO_I32(w, CW_NUM_SOLID); i++)
	{
		inst = CO_PTR(w, CW_SOLID + 0x30 + i * CW_INSTANCE_SIZE);
		if (CO_PTR(w, CW_SOLID + 0x34 + i * CW_INSTANCE_SIZE) != NULL)
			baUpdateAnimations(inst, CO_PTR(inst, 0x134), step, NULL);
		baPrepActorInstance(inst, 1);
	}
	for (i = 0; i < CO_I32(w, CW_NUM_ALPHA); i++)
	{
		inst = CO_PTR(w, CW_ALPHA + 0x30 + i * CW_INSTANCE_SIZE);
		if (CO_PTR(w, CW_ALPHA + 0x34 + i * CW_INSTANCE_SIZE) != NULL)
			baUpdateAnimations(inst, CO_PTR(inst, 0x134), step, NULL);
		baPrepActorInstance(inst, 1);
	}
	CO_F32(w, CW_SKY_ANGLE + 0) = step * 0.001f + CO_F32(w, CW_SKY_ANGLE + 0);    /* 0x0025767c */
	CO_F32(w, CW_SKY_ANGLE + 4) = step * 0.002f + CO_F32(w, CW_SKY_ANGLE + 4);    /* 0x002672b0 */
	CO_F32(w, CW_SKY_ANGLE + 8) = step * 0.003f + CO_F32(w, CW_SKY_ANGLE + 8);    /* 0x002680e0 */
}

/* 0x000c6890  CarryOn/CFire_World.cpp (file inferred)
Draws the sky: the back dome, then the subtractive and additive domes. */
void CFire_World_DrawSky(void *world)
{
	unsigned char *w = world;
	unsigned int old;

	bdSetRenderState(3, 0, 0);
	if (CO_PTR(w, CW_SKY_INST + 0) != NULL)
	{
		bmQuaternionRotationY(CO_FP(CO_PTR(w, CW_SKY_INST + 0), 0xe0), CO_F32(w, CW_SKY_ANGLE + 0));
		baPrepActorInstance(CO_PTR(w, CW_SKY_INST + 0), 1);
		baDrawActorInstance(CO_PTR(w, CW_SKY_INST + 0), 7);
	}
	bdSetRenderState(6, 1, 0);
	if (CO_PTR(w, CW_SKY_INST + 4) != NULL && CO_PTR(w, CW_SKY_INST + 8) != NULL)
	{
		old = bdGetRenderState(7, NULL);
		bdSetRenderState(7, 1, 0);
		bmQuaternionRotationY(CO_FP(CO_PTR(w, CW_SKY_INST + 4), 0xe0), CO_F32(w, CW_SKY_ANGLE + 4));
		bmQuaternionRotationY(CO_FP(CO_PTR(w, CW_SKY_INST + 8), 0xe0), CO_F32(w, CW_SKY_ANGLE + 8));
		Blend_SetMode(3);
		baPrepActorInstance(CO_PTR(w, CW_SKY_INST + 8), 1);
		baDrawActorInstance(CO_PTR(w, CW_SKY_INST + 8), 7);
		bdSetRenderState(7, old, 0);
		Blend_SetMode(1);
		baPrepActorInstance(CO_PTR(w, CW_SKY_INST + 4), 1);
		baDrawActorInstance(CO_PTR(w, CW_SKY_INST + 4), 7);
	}
	bdSetRenderState(6, 0, 0);
}

/* 0x000c69c0  CarryOn/CFire_World.cpp (file inferred)
At the highest "Detail" (2) draws the universe sets (passes 1 and 2). */
void CFire_World_DrawFar(void *world)
{
	unsigned char *w = world;
	int i;

	if (DAT_00294118 != 2)
		return;
	for (i = 0; i < CO_I32(w, CW_NUM_UNIVERSE); i++)
	{
		bdSetWorldMatrix(CO_FP(w, CW_UNIVERSE + 0x30 + i * CW_UNIVERSE_SIZE));
		baDrawSet(CO_PTR(w, CW_UNIVERSE + 0x70 + i * CW_UNIVERSE_SIZE), 1);
		baDrawSet(CO_PTR(w, CW_UNIVERSE + 0x70 + i * CW_UNIVERSE_SIZE), 2);
	}
}

/* 0x000c6a20  CarryOn/CFire_World.cpp (file inferred)
Draws the level layers (passes 1 and 2). */
void CFire_World_DrawNear(void *world)
{
	unsigned char *w = world;
	int i;

	bdSetRenderState(3, 1, 0);
	for (i = 0; i < CO_I32(w, CW_NUM_LAYERS); i++)
	{
		bdSetWorldMatrix(CO_FP(w, CW_LAYER + 0x40 + i * CW_LAYER_SIZE));
		baDrawSet(CO_PTR(w, CW_LAYER + 0x80 + i * CW_LAYER_SIZE), 1);
		baDrawSet(CO_PTR(w, CW_LAYER + 0x80 + i * CW_LAYER_SIZE), 2);
	}
}

/* 0x000c6a80  CarryOn/CFire_World.cpp (file inferred)
Draws the solid instances, the first "Detail" + 1 of them. */
void CFire_World_DrawObjects(void *world)
{
	unsigned char *w = world;
	int i;

	if (DAT_00294118 <= 0)
		return;
	bdSetWorldMatrix(DAT_002855f0);
	bdSetRenderState(3, 0, 0);
	for (i = 0; i < CO_I32(w, CW_NUM_SOLID); i++)
	{
		if (i >= DAT_00294118 + 2)
			return;
		baDrawActorInstance(CO_PTR(w, CW_SOLID + 0x30 + i * CW_INSTANCE_SIZE), 7);
	}
}

/* 0x000c6af0  CarryOn/CFire_World.cpp (file inferred)
The alpha pass: with "Alphabits" the universe sets and the layers (pass 4), then the
alpha instances. */
void CFire_World_DrawAlpha(void *world)
{
	unsigned char *w = world;
	int i;

	bdSetRenderState(6, 1, 0);
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(1);
	bdSetRenderState(5, 0, 0);
	if (DAT_00293a58 != 0)
	{
		for (i = 0; i < CO_I32(w, CW_NUM_UNIVERSE); i++)
		{
			bdSetWorldMatrix(CO_FP(w, CW_UNIVERSE + 0x30 + i * CW_UNIVERSE_SIZE));
			baDrawSet(CO_PTR(w, CW_UNIVERSE + 0x70 + i * CW_UNIVERSE_SIZE), 4);
		}
		if (DAT_00293a58 != 0)
		{
			for (i = 0; i < CO_I32(w, CW_NUM_LAYERS); i++)
			{
				bdSetWorldMatrix(CO_FP(w, CW_LAYER + 0x40 + i * CW_LAYER_SIZE));
				baDrawSet(CO_PTR(w, CW_LAYER + 0x80 + i * CW_LAYER_SIZE), 4);
			}
		}
	}
	for (i = 0; i < CO_I32(w, CW_NUM_ALPHA); i++)
		baDrawActorInstance(CO_PTR(w, CW_ALPHA + 0x30 + i * CW_INSTANCE_SIZE), 7);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
}

/* 0x000c6c00  CarryOn/CFire_World.cpp (missing from the function table)
ParseMap callback of an "Object" block: remembers its name and walks its "Instance"
children. Returns 2. */
int FUN_000c6c00(int node, unsigned char *world)
{
	strcpy((char *)world + CW_OBJNAME, ParseMap_GetString(node, "Name"));
	CO_U8(world, CW_NEWOBJ) = 1;
	ParseMap_Walk(node, (int)FUN_000c59a0, (undefined4)world, (int)"Instance");
	return 2;
}

/* 0x000c6c50  CarryOn/CFire_World.cpp (file inferred)
Loads and parses a LOM file from `package`: the "World" and "Object" blocks and the
intro camera. Returns 0 when it cannot be loaded or parsed. */
int CFire_World_LoadLOM(void *world, void *package, const char *name)
{
	unsigned int size;
	void *data;
	int *map;

	data = bkLoadFile(package, name, NULL, &size, NULL, 0);
	if (data == NULL)
	{
		bkPrintf("LoadLOMFile ERROR: Could not load [%s]\n", name);
		return 0;
	}
	map = ParseMap_Load(data, 2, size);
	if (map == NULL)
	{
		bkPrintf("LoadLOMFile ERROR: Parsing [%s]\n", name);
		return 0;
	}
	ParseMap_WalkMap((int)map, (int)FUN_000c5610, (undefined4)world, (int)"World");
	ParseMap_WalkMap((int)map, (int)FUN_000c6c00, (undefined4)world, (int)"Object");
	CLIntroCam_Load((int)map, (undefined4)world);
	ParseMap_Free(map);
	return 1;
}

/* 0x000c6d00  CarryOn/CFire_World.cpp
Clears the level, loads map\<name>.lom (test\firetest.lom in test mode) and, when the
LOM has no spawn points, adds four default ones (charlib RespawnPoint; they register
themselves). Returns 0 when the LOM cannot be loaded. (The original guards the
allocations with a C++ exception frame.) */
int CFire_World_Load(void *world, void *package, int variant, const char *name)
{
	unsigned char *w = world;
	char path[256];
	float pos[4];
	void *mem;

	(void)variant;
	memset(w, 0, 0x8f2 * 4);
	strcpy((char *)w, name);
	CO_PTR(w, CW_PACKAGE) = package;
	CO_PTR(w, CW_SKY_MODEL + 8) = NULL;
	CO_PTR(w, CW_SKY_MODEL + 4) = NULL;
	CO_PTR(w, CW_SKY_MODEL + 0) = NULL;
	CO_F32(w, CW_SKY_ANGLE + 8) = 0.0f;
	CO_F32(w, CW_SKY_ANGLE + 4) = 0.0f;
	CO_F32(w, CW_SKY_ANGLE + 0) = 0.0f;
	sprintf(path, CO_I32(g_fireRes, 0x258) == 0 ? "map\\%s.lom" : "test\\firetest.lom", name);
	_strlwr(path);
	if (CFire_World_LoadLOM(w, package, path) == 0)
		return 0;
	if (CO_I32(w, CW_NUM_SPAWNS) == 0)
	{
		pos[0] = 500.0f;
		pos[1] = 8000.0f;
		pos[2] = -500.0f;
		pos[3] = 0.0f;
		bkPrintf("Adding default spawns\n");
		mem = bkHeapAlloc(0x20, CO_FILE_WORLD, 0x77, 0x1001);
		if (mem != NULL)
			RespawnPoint_Init(mem, pos, 0);
		pos[2] = 300.0f;
		mem = bkHeapAlloc(0x20, CO_FILE_WORLD, 0x79, 0x1001);
		if (mem != NULL)
			RespawnPoint_Init(mem, pos, 0);
		pos[2] = 500.0f;
		pos[0] = -500.0f;
		mem = bkHeapAlloc(0x20, CO_FILE_WORLD, 0x7c, 0x1001);
		if (mem != NULL)
			RespawnPoint_Init(mem, pos, 0);
		pos[2] = -300.0f;
		mem = bkHeapAlloc(0x20, CO_FILE_WORLD, 0x7f, 0x1001);
		if (mem != NULL)
			RespawnPoint_Init(mem, pos, 0);
	}
	return 1;
}
