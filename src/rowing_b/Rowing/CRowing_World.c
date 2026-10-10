/*
Rowing/CRowing_World.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_World.cpp

The level of the Rowing game (0x37c bytes, in the rowing_a game object), loaded from
map\world.lom through classlib_ui's ParseMap: the "World" block names the level meshes
(map\<name>.obe solid, <name>add.obe additive, <name>semi.obe semi-transparent, and
map\worldset.obe), "Object" blocks place objects through their "Instance" children (start
positions, arrows, ponkers, smoke, the big animated object and the scenery objects of
CRowing_objects.cpp). The paths (collision, water, checkpoints, the racing lines, end
boxes, gates, limits) come from 3D Studio ASCII exports (map\*.ase, CRowing_ASEImport.cpp).

0x001b43c0 is tagged with CRowing_World.cpp; the gap functions 0x001b2c10-0x001b4720 around
it work on the same object (file inferred). The ParseMap callbacks 0x001b3560 ("World"),
0x001b42f0 ("Object") and 0x001b3a10 ("Instance") are missing from the function table:
they are defined here with their FUN_ names (declared in rowing_b.h) and reported.

The level is reached with raw offsets (RW_*), as in carryon_a's CFire_World.c.
*/

#include "rowing_b.h"

/* the level's fields (byte offsets) */
#define RW_NAME 0x000           /* char[0x80] the level name */
#define RW_PACKAGE 0x080        /* the level package */
#define RW_START 0x084          /* start positions, 0x14 bytes: pos[4], yaw */
#define RW_NUM_STARTS 0x124
#define RW_COLLISION 0x134      /* path lists: count, then the RowPath array */
#define RW_WATER 0x13c
#define RW_CHECKPOINTS 0x144
#define RW_RACELINE 0x14c
#define RW_LINE_COUNT 0x154     /* the counts of the 4 race lines a-d (multi-line mode) */
#define RW_LINE_PATHS 0x164     /* their RowPath arrays */
#define RW_ENDBOX 0x174
#define RW_ENDBOX_COUNT 0x17c   /* the counts of the 4 end boxes a-d */
#define RW_ENDBOX_PATHS 0x18c   /* their RowPath arrays */
#define RW_GATE 0x19c
#define RW_LIMIT 0x1a4
#define RW_SOLID_MODEL 0x1ac
#define RW_SOLID_INST 0x1b0
#define RW_ADD_MODEL 0x1b4
#define RW_ADD_INST 0x1b8
#define RW_SEMI_MODEL 0x1bc
#define RW_SEMI_INST 0x1c0
#define RW_WORLDSET 0x1c4
#define RW_ARROW 0x1c8          /* arrows: pos[4] each */
#define RW_ARROW_YAW 0x248      /* their yaws */
#define RW_NUM_ARROWS 0x268
#define RW_OBJECTS 0x26c        /* the scenery objects (CRowing_objects.cpp list head) */
#define RW_NUM_PONKERS 0x270
#define RW_PONKER 0x274         /* pos[4] each */
#define RW_BIGANIM 0x2f4        /* the "biganim" object */
#define RW_NUM_SMOKE 0x2f8
#define RW_SMOKE 0x2fc          /* pos[4] each */

/* 0x0030f0ac  debug drawing of the paths */
#define g_RowDrawPaths RW_VAR(int, 0x0030f0ac)
/* 0x002b5920  the big animated object is visible (bit 0) */
#define g_RowBigAnimShow RW_VAR(unsigned int, 0x002b5920)
/* 0x002b5924  draw the level as one actor instance instead of the world set */
#define g_RowDrawSolid RW_VAR(int, 0x002b5924)
/* 0x002b5928-0x002b5938  the fog: red, green, blue, start, end; 0x002b593c-0x002b597c the
   values of the 4 layouts (5 tables of 4) */
#define g_RowFogR RW_VAR(unsigned int, 0x002b5928)
#define g_RowFogG RW_VAR(unsigned int, 0x002b592c)
#define g_RowFogB RW_VAR(unsigned int, 0x002b5930)
#define g_RowFogStart RW_VAR(float, 0x002b5934)
#define g_RowFogEnd RW_VAR(float, 0x002b5938)

/* the line vertices of the path debug drawing (2 vertices of 0x24 bytes at 0x0030fb88) */
#define g_RowLineVerts ((float *)RW_ADDR(0x0030fb88))
/* the highlighted path of each list in the debug drawing (it moves every 16 frames) */
#define g_RowPathFrame RW_VAR(int, 0x0030fc00)

/* the path lists (count, paths) that RowWorld_Free frees, in its order */
static void rb_free_paths(unsigned char *w, int paths, int count)
{
	RowPaths_Free(RB_PTR(w, paths), RB_I32(w, count));
}

/* reads PosX/Y/Z (0), w 1 */
static void rb_read_pos(int node, float *pos)
{
	if (ParseMap_GetFloat(node, "PosX", (undefined4 *)&pos[0]) == 0)
		pos[0] = 0.0f;
	if (ParseMap_GetFloat(node, "PosY", (undefined4 *)&pos[1]) == 0)
		pos[1] = 0.0f;
	if (ParseMap_GetFloat(node, "PosZ", (undefined4 *)&pos[2]) == 0)
		pos[2] = 0.0f;
	pos[3] = 1.0f;
}

/* reads RotX/Y/Z/W (0) */
static void rb_read_rot(int node, float *rot)
{
	if (ParseMap_GetFloat(node, "RotX", (undefined4 *)&rot[0]) == 0)
		rot[0] = 0.0f;
	if (ParseMap_GetFloat(node, "RotY", (undefined4 *)&rot[1]) == 0)
		rot[1] = 0.0f;
	if (ParseMap_GetFloat(node, "RotZ", (undefined4 *)&rot[2]) == 0)
		rot[2] = 0.0f;
	if (ParseMap_GetFloat(node, "RotW", (undefined4 *)&rot[3]) == 0)
		rot[3] = 0.0f;
}

/* 0x001b2c10  Rowing (gap; CRowing_World.cpp inferred) */
void *RowWorld_ctor(void *self)
{
	memset(self, 0, 0xdf * 4);
	return self;
}

/* loads map\<file> from the level package into the ASE reader; 0 when it is missing */
static int rb_open_ase(unsigned char *w, struct RowAseFile *file, const char *path)
{
	unsigned int size;
	void *data;

	data = bkLoadFile(RB_PTR(w, RW_PACKAGE), path, NULL, &size, NULL, 0);
	if (data == NULL)
		return 0;
	RowAseText_Open(&file->text, data, size);
	return 1;
}

/* 0x001b2c30  Rowing (gap; CRowing_World.cpp inferred)
Reads the paths of the level from map\collision.ase, water.ase, checkpoints.ase,
racingline.ase (racing line, end box, gates, limits) and, in layout 1, multiline.ase (4
race lines and their end boxes). (The original guards the two reader objects with a C++
exception frame.) */
void RowWorld_LoadPaths(void *self)
{
	unsigned char *w = self;
	struct RowAseImport ase;
	struct RowAseFile file;
	char path[0x200];
	unsigned int size;
	void *data;
	int i;

	RowAseFile_ctor(&file);
	RowAseImport_ctor(&ase);

	/* (the original passes the level, "collision" and "ase" as unused sprintf arguments) */
	sprintf(path, "map\\collision.ase");
	if (rb_open_ase(w, &file, path) == 0) {
		RB_PTR(w, RW_COLLISION + 4) = NULL;
		RB_I32(w, RW_COLLISION) = 0;
	} else {
		RB_I32(w, RW_COLLISION) = 0;
		RowAseImport_ReadAllShapes(&ase, &file, RB_ADDR(w, RW_COLLISION + 4), RB_ADDR(w, RW_COLLISION));
	}
	sprintf(path, "map\\water.ase");
	if (rb_open_ase(w, &file, path) == 0) {
		RB_PTR(w, RW_WATER + 4) = NULL;
		RB_I32(w, RW_WATER) = 0;
	} else {
		RB_I32(w, RW_WATER) = 0;
		RowAseImport_ReadAllShapes(&ase, &file, RB_ADDR(w, RW_WATER + 4), RB_ADDR(w, RW_WATER));
	}
	sprintf(path, "map\\checkpoints.ase");
	if (rb_open_ase(w, &file, path) == 0) {
		RB_PTR(w, RW_CHECKPOINTS + 4) = NULL;
		RB_I32(w, RW_CHECKPOINTS) = 0;
	} else {
		RB_I32(w, RW_CHECKPOINTS) = 0;
		RowAseImport_ReadShapes(&ase, &file, RB_ADDR(w, RW_CHECKPOINTS + 4),
			RB_ADDR(w, RW_CHECKPOINTS), "check", NULL);
	}
	for (i = 0; i < 4; i++) {
		RB_I32(w, RW_LINE_COUNT + i * 4) = 0;
		RB_PTR(w, RW_LINE_PATHS + i * 4) = NULL;
		RB_I32(w, RW_ENDBOX_COUNT + i * 4) = 0;
		RB_PTR(w, RW_ENDBOX_PATHS + i * 4) = NULL;
	}
	RB_I32(w, RW_RACELINE) = 0;
	RB_I32(w, RW_ENDBOX) = 0;
	RB_I32(w, RW_GATE) = 0;
	RB_I32(w, RW_LIMIT) = 0;
	RB_PTR(w, RW_RACELINE + 4) = NULL;
	RB_PTR(w, RW_ENDBOX + 4) = NULL;
	RB_PTR(w, RW_GATE + 4) = NULL;
	RB_PTR(w, RW_LIMIT + 4) = NULL;
	sprintf(path, "map\\racingline.ase");
	if (rb_open_ase(w, &file, path) != 0) {
		RowAseImport_ReadShapes(&ase, &file, RB_ADDR(w, RW_RACELINE + 4), RB_ADDR(w, RW_RACELINE),
			"ine", NULL);
		RowAseImport_ReadShapes(&ase, &file, RB_ADDR(w, RW_ENDBOX + 4), RB_ADDR(w, RW_ENDBOX),
			"endbox", NULL);
		RowAseImport_ReadShapes(&ase, &file, RB_ADDR(w, RW_GATE + 4), RB_ADDR(w, RW_GATE),
			"gate", NULL);
		RowAseImport_ReadShapes(&ase, &file, RB_ADDR(w, RW_LIMIT + 4), RB_ADDR(w, RW_LIMIT),
			"limit", NULL);
	}
	if ((g_RowVariant & 0xc) == 4) {
		data = bkLoadFile(RB_PTR(w, RW_PACKAGE), "map\\multiline.ase", NULL, &size, NULL, 0);
		if (data != NULL) {
			static const char *const suffix[4] = { "_a", "_b", "_c", "_d" };

			RowAseText_Open(&file.text, data, size);
			for (i = 0; i < 4; i++)
				RowAseImport_ReadShapes(&ase, &file, RB_ADDR(w, RW_LINE_PATHS + i * 4),
					RB_ADDR(w, RW_LINE_COUNT + i * 4), suffix[i], "raceline");
			for (i = 0; i < 4; i++)
				RowAseImport_ReadShapes(&ase, &file, RB_ADDR(w, RW_ENDBOX_PATHS + i * 4),
					RB_ADDR(w, RW_ENDBOX_COUNT + i * 4), suffix[i], "endbox");
		}
	}
	RowAseImport_dtor(&ase);
	RowAseFile_dtor(&file);
}

/* 0x001b3170  Rowing (gap; CRowing_World.cpp inferred)
Updates the scenery objects and puffs smoke at every smoke point. */
void RowWorld_Update(void *self, float dt)
{
	unsigned char *w = self;
	int i;

	RowObjects_Update(RB_PTR(w, RW_OBJECTS), dt);
	for (i = 0; i < RB_I32(w, RW_NUM_SMOKE); i++)
		RowParticles_AddSmoke(g_RowParticles, RB_FP(w, RW_SMOKE + i * 0x10), 30.0f);
}

/* 0x001b31d0  Rowing (gap; CRowing_World.cpp inferred)
Sets the fog of the layout (an end at or before the start is moved just past it). */
void RowWorld_SetFog(void)
{
	float end = g_RowFogEnd;
	float start;

	if (end <= g_RowFogStart)
		end = g_RowFogStart * 1.00000095f;      /* 0x00268668 */
	start = g_RowFogStart;
	bdSetFogColour(g_RowFogR, g_RowFogG, g_RowFogB);
	bdSetFogRange(start, end);
}

/* 0x001b3230  Rowing (gap; CRowing_World.cpp inferred)
Shows or hides the big animated object and draws the opaque scenery objects (pass 3). */
void RowWorld_DrawObjects(void *self, int draw)
{
	unsigned char *w = self;
	unsigned char *big = RB_PTR(w, RW_BIGANIM);

	if (big != NULL)
		RB_U32(big, 0x18) = RB_U32(big, 0x18) ^ ((RB_U32(big, 0x18) ^ g_RowBigAnimShow) & 1);
	bdSetWorldMatrix((float *)DAT_002855f0);
	if (draw != 0)
		RowObjects_Draw(RB_PTR(w, RW_OBJECTS), 3);
}

/* 0x001b3280  Rowing (gap; CRowing_World.cpp inferred)
Draws the alpha-blended layers: the scenery objects' pass 4, the additive mesh, and (no
z writes) the semi-transparent mesh. */
void RowWorld_DrawBlended(void *self)
{
	unsigned char *w = self;

	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	Blend_SetMode(1);
	RowObjects_Draw(RB_PTR(w, RW_OBJECTS), 4);
	if (RB_PTR(w, RW_ADD_INST) != NULL)
		baDrawActorInstance(RB_PTR(w, RW_ADD_INST), 7);
	bdSetRenderState(10, 0, 0);
	if (RB_PTR(w, RW_SEMI_INST) != NULL)
		baDrawActorInstance(RB_PTR(w, RW_SEMI_INST), 7);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
}

/* one debug line vertex: xyz, colour (fvf 0x152) */
static void rb_line_vertex(float *v, float x, float y, float z, unsigned int colour)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
	RB_U32(v, 0x18) = colour;
}

/* 0x001b3310  Rowing (gap; CRowing_World.cpp inferred)
Debug drawing: the path as lines at height y, the colour fading to an eighth along each
segment (and the closing segment of a closed path). */
void RowPath_Draw(struct RowPath *self, float y, unsigned char r, unsigned char g, unsigned char b)
{
	float *v = g_RowLineVerts;
	unsigned int bright;
	unsigned int dark;
	int i;

	bdSetRenderState(3, 0, 0);
	bdSetTexture(0, NULL);
	bdSetWorldMatrix((float *)DAT_002855f0);
	bdSetDepthOffset(-100.0f);                                      /* 0x00267a00 */
	for (i = 0; self->count != 1 && i < self->count - 1; i++) {
		bright = DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bc898[255] | DAT_002bce18[r];
		dark = DAT_002bc490[g >> 3] | DAT_002bd640[b >> 3] | DAT_002bc898[255] | DAT_002bce18[r >> 3];
		rb_line_vertex(v, self->points[i * 2], y, self->points[i * 2 + 1], bright);
		rb_line_vertex(v + 9, self->points[i * 2 + 2], y, self->points[i * 2 + 3], dark);
		bdDrawPrimitiveFVF(1, v, 2, 0x152);
	}
	if (self->closed != 0) {
		bright = DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bce18[r] | DAT_002bc898[255];
		dark = DAT_002bd640[b >> 3] | DAT_002bc490[g >> 3] | DAT_002bce18[r >> 3] | DAT_002bc898[255];
		rb_line_vertex(v, self->points[0], y, self->points[1], bright);
		rb_line_vertex(v + 9, self->points[self->count * 2 - 2], y, self->points[self->count * 2 - 1],
			dark);
		bdDrawPrimitiveFVF(1, v, 2, 0x152);
	}
	bdSetDepthOffset(0.0f);
}

/* 0x001b3530  Rowing (gap; CRowing_World.cpp inferred by address)
Copies the first segment of a path (its first two points: x0, z0, x1, z1). */
void RowPath_GetSegment(struct RowPath *self, float *out)
{
	out[0] = self->points[0];
	out[1] = self->points[1];
	out[2] = self->points[2];
	out[3] = self->points[3];
}

/* loads one level mesh as an actor instance with the "World" block's transform */
static void *rb_world_instance(unsigned char *w, int modelField, int line, const float *pos,
	const float *rot, const float *scale)
{
	unsigned char *inst;

	inst = (unsigned char *)baInitActorInstance(NULL, RB_PTR(w, modelField), 0, RB_FILE_WORLD, line);
	if (inst == NULL)
		return NULL;
	memcpy(inst + 0xc0, pos, 16);
	memcpy(inst + 0xe0, rot, 16);
	memcpy(inst + 0xd0, scale, 16);
	baPrepActorInstance((struct BActorInstance *)inst, 1);
	return inst;
}

/* 0x001b3560  Rowing/CRowing_World.cpp (missing from the function table)
ParseMap callback of the "World" block: loads the level meshes map\<Name>.obe (solid),
map\worldset.obe, map\<Name>add.obe and, in layouts 1-3, map\<Name>semi.obe, each as an
actor instance with the block's transform. Returns 2. */
int FUN_001b3560(int node, void *world)
{
	unsigned char *w = world;
	char path[0x80];
	float pos[4];
	float rot[4];
	float scale[4];
	char *name;

	name = ParseMap_GetString(node, "Name");
	rb_read_pos(node, pos);
	rb_read_rot(node, rot);
	if (ParseMap_GetFloat(node, "SclX", (undefined4 *)&scale[0]) == 0)
		scale[0] = 0.0f;
	if (ParseMap_GetFloat(node, "SclY", (undefined4 *)&scale[1]) == 0)
		scale[1] = 0.0f;
	if (ParseMap_GetFloat(node, "SclZ", (undefined4 *)&scale[2]) == 0)
		scale[2] = 0.0f;
	scale[3] = 1.0f;

	sprintf(path, "map\\%s.obe", name);
	_strlwr(path);
	RB_PTR(w, RW_SOLID_MODEL) = bLoadResource(RB_PTR(w, RW_PACKAGE), path, 1, 0);
	if (RB_PTR(w, RW_SOLID_MODEL) == NULL) {
		bkPrintf("ERROR: Could not find solid mesh [%s]\n", path);
		return 2;
	}
	RB_PTR(w, RW_SOLID_INST) = rb_world_instance(w, RW_SOLID_MODEL, 0x90, pos, rot, scale);
	if (RB_PTR(w, RW_SOLID_INST) == NULL)
		goto no_instance;

	sprintf(path, "map\\worldset.obe");
	RB_PTR(w, RW_WORLDSET) = bLoadResource(RB_PTR(w, RW_PACKAGE), path, 6, 0xdefa);

	sprintf(path, "map\\%sadd.obe", name);
	_strlwr(path);
	RB_PTR(w, RW_ADD_MODEL) = bLoadResource(RB_PTR(w, RW_PACKAGE), path, 1, 0);
	if (RB_PTR(w, RW_ADD_MODEL) == NULL) {
		bkPrintf("WARNING: Could not find world mesh [%s]\n", path);
	} else {
		RB_PTR(w, RW_ADD_INST) = rb_world_instance(w, RW_ADD_MODEL, 0xb8, pos, rot, scale);
		if (RB_PTR(w, RW_ADD_INST) == NULL)
			goto no_instance;
	}

	if ((g_RowVariant & 0xc) != 0) {
		sprintf(path, "map\\%ssemi.obe", name);
		_strlwr(path);
		RB_PTR(w, RW_SEMI_MODEL) = bLoadResource(RB_PTR(w, RW_PACKAGE), path, 1, 0);
		if (RB_PTR(w, RW_SEMI_MODEL) == NULL) {
			bkPrintf("WARNING: Could not find world mesh [%s]\n", path);
			return 2;
		}
		RB_PTR(w, RW_SEMI_INST) = rb_world_instance(w, RW_SEMI_MODEL, 0xd8, pos, rot, scale);
		if (RB_PTR(w, RW_SEMI_INST) == NULL)
			goto no_instance;
	}
	return 2;

no_instance:
	bkPrintf("ERROR: Could not get instance of model [%s]\n", path);
	return 2;
}

/* the yaw of a rotation quaternion (the angle about the axis, negated when the axis points
   down, then wrapped into [-pi, pi)) */
static float rb_quat_yaw(const float *rot)
{
	float axisAngle[4];
	float yaw;

	bmQuaternionToAxisAngle(axisAngle, rot);
	yaw = axisAngle[3];
	if (axisAngle[1] < 0.0f) {
		yaw = -yaw;
		while (yaw < -3.1415925f)                               /* 0x00253f3c */
			yaw = yaw + 6.28318548f;                            /* 0x00255930 */
		while (3.1415925f <= yaw)                               /* 0x00256750 */
			yaw = yaw - 6.28318548f;
	}
	return yaw;
}

/* "obj_map\<name>" without the name's trailing digits, then ".obe", in lower case */
static void rb_object_path(char *path, const char *name)
{
	size_t len;

	sprintf(path, "obj_map\\%s", name);
	for (len = strlen(path); len != 0 && path[len - 1] >= '0' && path[len - 1] <= '9';
		len = strlen(path))
		path[len - 1] = '\0';
	strcat(path, ".obe");
	_strlwr(path);
}

/* 0x001b3a10  Rowing/CRowing_World.cpp (missing from the function table)
ParseMap callback of an "Instance" child of an "Object" block, by the instance's name:
"start<n>" a start position (n 1-5; the yaw goes to the next free slot), "arrow" an arrow
(layout 1 only), "GoalBank" nothing, "Bouy Xref", "Overhead Crane", "Crane" those
models, "ponker" a ponker position, "biganim" the big animated object, "smoke" a smoke
point; in layout 2 the whirlpools and the main water are skipped; anything else is a
scenery object obj_map\<name without its number>.obe. Returns 0. */
int FUN_001b3a10(int node, void *world)
{
	unsigned char *w = world;
	char path[0x40];
	float pos[4];
	float rot[4];
	float yaw;
	char *name;
	int index;
	int n;

	name = ParseMap_GetString(node, "Name");
	if (ParseMap_GetFloat(node, "PosX", (undefined4 *)&pos[0]) == 0)
		pos[0] = 0.0f;
	if (ParseMap_GetFloat(node, "PosY", (undefined4 *)&pos[1]) == 0)
		pos[1] = 0.0f;
	if (ParseMap_GetFloat(node, "PosZ", (undefined4 *)&pos[2]) == 0)
		pos[2] = 0.0f;
	pos[3] = 1.0f;
	rb_read_rot(node, rot);

	if (strstr(name, "start") != NULL) {
		index = atoi(name + 5) - 1;
		if (index >= 5 || index < 0) {
			bkPrintf("*** ERROR *** Invalid index for start position [%d]\n", index);
			return 0;
		}
		memcpy(RB_ADDR(w, RW_START + index * 0x14), pos, 16);
		yaw = rb_quat_yaw(rot);
		n = RB_I32(w, RW_NUM_STARTS);
		if (n >= 4)
			return 0;
		RB_F32(w, RW_START + 0x10 + n * 0x14) = yaw;
		RB_I32(w, RW_NUM_STARTS) = n + 1;
		return 0;
	}
	if (strstr(name, "arrow") != NULL && (g_RowVariant & 0xc) == 4) {
		n = RB_I32(w, RW_NUM_ARROWS);
		memcpy(RB_ADDR(w, RW_ARROW + n * 0x10), pos, 16);
		RB_F32(w, RW_ARROW_YAW + RB_I32(w, RW_NUM_ARROWS) * 4) = rb_quat_yaw(rot);
		RB_I32(w, RW_NUM_ARROWS) = RB_I32(w, RW_NUM_ARROWS) + 1;
		return 0;
	}
	if (strstr(name, "GoalBank") != NULL) {
		atoi(name + 8);
		return 0;
	}
	if (strstr(name, "Bouy Xref") != NULL) {
		sprintf(path, "obj_map\\%s.obe", "Bouy Xref");
		_strlwr(path);
	} else if (strstr(name, "Overhead Crane") != NULL) {
		sprintf(path, "obj_map\\%s.obe", "Overhead Crane");
		_strlwr(path);
	} else if (strstr(name, "Crane") != NULL) {
		sprintf(path, "obj_map\\crane.obe");
		_strlwr(path);
	} else if (strstr(name, "ponker") != NULL) {
		n = RB_I32(w, RW_NUM_PONKERS);
		memcpy(RB_ADDR(w, RW_PONKER + n * 0x10), pos, 16);
		RB_I32(w, RW_NUM_PONKERS) = RB_I32(w, RW_NUM_PONKERS) + 1;
		return 0;
	} else if (strstr(name, "biganim") != NULL) {
		rb_object_path(path, name);
		RB_PTR(w, RW_BIGANIM) = RowObjects_Add(RB_PTR(w, RW_OBJECTS), RB_PTR(w, RW_PACKAGE), path, pos,
			rot);
		return 0;
	} else if (strstr(name, "smoke") != NULL) {
		n = RB_I32(w, RW_NUM_SMOKE);
		memcpy(RB_ADDR(w, RW_SMOKE + n * 0x10), pos, 16);
		RB_I32(w, RW_NUM_SMOKE) = RB_I32(w, RW_NUM_SMOKE) + 1;
		return 0;
	} else {
		if ((g_RowVariant & 0xc) == 8 && (strstr(name, "Whirlpool") != NULL
			|| strstr(name, "whirlpool") != NULL || strstr(name, "Main Water") != NULL))
			return 0;
		rb_object_path(path, name);
	}
	RowObjects_Add(RB_PTR(w, RW_OBJECTS), RB_PTR(w, RW_PACKAGE), path, pos, rot);
	return 0;
}

/* 0x001b4070  Rowing (gap; CRowing_World.cpp inferred)
Debug drawing: the collision paths (dark red) and the water paths (cyan, at 50). */
void RowWorld_DrawCollision(void *self)
{
	unsigned char *w = self;
	struct RowPath *paths;
	int i;

	for (i = 0; i < RB_I32(w, RW_COLLISION); i++) {
		paths = RB_PTR(w, RW_COLLISION + 4);
		RowPath_Draw(&paths[i], 0.0f, 0x7f, 0, 0);
	}
	for (i = 0; i < RB_I32(w, RW_WATER); i++) {
		paths = RB_PTR(w, RW_WATER + 4);
		RowPath_Draw(&paths[i], 50.0f, 0, 0xff, 0xff);              /* 0x002559b8 */
	}
}

/* steps the highlighted path of a list every 16 frames */
static void rb_step(int addr, unsigned char *w, int countField)
{
	if (RB_I32(w, countField) != 0)
		RW_VAR(int, addr) = (RW_VAR(int, addr) + 1) % RB_I32(w, countField);
}

/* 0x001b40f0  Rowing (gap; CRowing_World.cpp inferred)
Debug drawing: the racing line (white) and the gates (yellow), the highlighted one bright
(each list's highlight moves on every 16 frames). */
void RowWorld_DrawRaceLine(void *self)
{
	unsigned char *w = self;
	struct RowPath *paths;
	int c;
	int i;

	g_RowPathFrame = g_RowPathFrame + 1;
	if ((g_RowPathFrame & 0xf) == 0) {
		rb_step(0x0030fbfc, w, RW_RACELINE);
		rb_step(0x0030fbf8, w, RW_ENDBOX);
		rb_step(0x0030fbf4, w, RW_LIMIT);
		rb_step(0x0030fbf0, w, RW_GATE);
		rb_step(0x0030fbe0, w, RW_LINE_COUNT);
		rb_step(0x0030fbd0, w, RW_ENDBOX_COUNT);
		rb_step(0x0030fbe4, w, RW_LINE_COUNT + 4);
		rb_step(0x0030fbd4, w, RW_ENDBOX_COUNT + 4);
		rb_step(0x0030fbe8, w, RW_LINE_COUNT + 8);
		rb_step(0x0030fbd8, w, RW_ENDBOX_COUNT + 8);
		rb_step(0x0030fbec, w, RW_LINE_COUNT + 0xc);
		rb_step(0x0030fbdc, w, RW_ENDBOX_COUNT + 0xc);
	}
	for (i = 0; i < RB_I32(w, RW_RACELINE); i++) {
		paths = RB_PTR(w, RW_RACELINE + 4);
		c = i == RW_VAR(int, 0x0030fbfc) ? 0xff : 0x80;
		RowPath_Draw(&paths[i], 0.0f, (unsigned char)c, (unsigned char)c, (unsigned char)c);
	}
	for (i = 0; i < RB_I32(w, RW_GATE); i++) {
		paths = RB_PTR(w, RW_GATE + 4);
		c = i == RW_VAR(int, 0x0030fbf0) ? 0xff : 0x80;
		RowPath_Draw(&paths[i], 0.0f, (unsigned char)c, (unsigned char)c, 0);
	}
}

/* 0x001b42f0  Rowing/CRowing_World.cpp (missing from the function table)
ParseMap callback of an "Object" block: walks its "Instance" children. Returns 2. */
int FUN_001b42f0(int node, void *world)
{
	ParseMap_Walk(node, (int)FUN_001b3a10, (undefined4)world, (int)"Instance");
	return 2;
}

/* 0x001b4310  Rowing (gap; CRowing_World.cpp inferred)
Loads and parses a LOM file from `package`: the "World" and "Object" blocks and the intro
camera. Returns 0 when it cannot be loaded or parsed. */
int RowWorld_LoadLOM(void *world, void *package, const char *name)
{
	unsigned int size;
	void *data;
	int *map;

	data = bkLoadFile(package, name, NULL, &size, NULL, 0);
	if (data == NULL) {
		bkPrintf("LoadLOMFile ERROR: Could not load [%s]\n", name);
		return 0;
	}
	map = ParseMap_Load(data, 2, (int)size);
	if (map == NULL) {
		bkPrintf("LoadLOMFile ERROR: Parsing [%s]\n", name);
		return 0;
	}
	ParseMap_WalkMap((int)map, (int)FUN_001b3560, (undefined4)world, (int)"World");
	ParseMap_WalkMap((int)map, (int)FUN_001b42f0, (undefined4)world, (int)"Object");
	CLIntroCam_Load((int)map, (undefined4)world);
	ParseMap_Free(map);
	return 1;
}

/* 0x001b43c0  Rowing/CRowing_World.cpp
Sets the layout's fog, makes the scenery object list, stores the level name and package,
reads the paths and loads map\world.lom. (The original guards the allocation with a C++
exception frame.) */
int RowWorld_Load(void *self, void *package, const char *name)
{
	unsigned char *w = self;
	char path[0x200];
	void *objects;
	int k = (int)RW_LAYOUT() * 4;

	g_RowFogG = RW_VAR(unsigned int, 0x002b594c + k);
	g_RowFogR = RW_VAR(unsigned int, 0x002b593c + k);
	g_RowFogB = RW_VAR(unsigned int, 0x002b595c + k);
	g_RowFogStart = RW_VAR(float, 0x002b596c + k);
	g_RowFogEnd = RW_VAR(float, 0x002b597c + k);
	objects = RB_NEW(4, RB_FILE_WORLD, 0x213, 0x1001);
	if (objects != NULL)
		objects = CCollectEffect_Ctor(objects);     /* charlib 0x000befa0: *list = NULL */
	RB_PTR(w, RW_OBJECTS) = objects;
	RB_I32(w, RW_NUM_STARTS) = 0;
	strcpy((char *)w + RW_NAME, name);
	RB_PTR(w, RW_PACKAGE) = package;
	RowWorld_LoadPaths(w);
	sprintf(path, "map\\world.lom");
	return RowWorld_LoadLOM(w, package, path) != 0;
}

/* 0x001b44f0  Rowing (gap; CRowing_World.cpp inferred)
Frees `count` paths (their points) and the list. */
void RowPaths_Free(struct RowPath *paths, int count)
{
	int i;

	if (paths == NULL)
		return;
	for (i = 0; i < count; i++) {
		if (paths[i].points != NULL) {
			RB_DELETE(paths[i].points);
			paths[i].points = NULL;
		}
	}
	RB_DELETE(paths);
}

/* 0x001b4530  Rowing (gap; CRowing_World.cpp inferred)
Frees the scenery objects, the level meshes, the intro camera and the paths. */
void RowWorld_Free(void *self)
{
	unsigned char *w = self;
	void *objects = RB_PTR(w, RW_OBJECTS);

	if (objects != NULL) {
		RowObjects_Free(objects);
		RB_DELETE(objects);
		RB_PTR(w, RW_OBJECTS) = NULL;
	}
	if (RB_PTR(w, RW_SOLID_INST) != NULL) {
		baDeleteActorInstance(RB_PTR(w, RW_SOLID_INST));
		RB_PTR(w, RW_SOLID_INST) = NULL;
	}
	if (RB_PTR(w, RW_SOLID_MODEL) != NULL) {
		bkDeleteResource(RB_PTR(w, RW_SOLID_MODEL));
		RB_PTR(w, RW_SOLID_MODEL) = NULL;
	}
	if (RB_PTR(w, RW_ADD_INST) != NULL) {
		baDeleteActorInstance(RB_PTR(w, RW_ADD_INST));
		RB_PTR(w, RW_ADD_INST) = NULL;
	}
	if (RB_PTR(w, RW_ADD_MODEL) != NULL) {
		bkDeleteResource(RB_PTR(w, RW_ADD_MODEL));
		RB_PTR(w, RW_ADD_MODEL) = NULL;
	}
	if (RB_PTR(w, RW_SEMI_INST) != NULL) {
		baDeleteActorInstance(RB_PTR(w, RW_SEMI_INST));
		RB_PTR(w, RW_SEMI_INST) = NULL;
	}
	if (RB_PTR(w, RW_SEMI_MODEL) != NULL) {
		bkDeleteResource(RB_PTR(w, RW_SEMI_MODEL));
		RB_PTR(w, RW_SEMI_MODEL) = NULL;
	}
	CLIntroCam_Destroy();
	rb_free_paths(w, RW_COLLISION + 4, RW_COLLISION);
	rb_free_paths(w, RW_WATER + 4, RW_WATER);
	rb_free_paths(w, RW_CHECKPOINTS + 4, RW_CHECKPOINTS);
	rb_free_paths(w, RW_RACELINE + 4, RW_RACELINE);
	rb_free_paths(w, RW_ENDBOX + 4, RW_ENDBOX);
	rb_free_paths(w, RW_GATE + 4, RW_GATE);
	rb_free_paths(w, RW_LIMIT + 4, RW_LIMIT);
	rb_free_paths(w, RW_LINE_PATHS, RW_LINE_COUNT);
	rb_free_paths(w, RW_LINE_PATHS + 4, RW_LINE_COUNT + 4);
	rb_free_paths(w, RW_LINE_PATHS + 8, RW_LINE_COUNT + 8);
	rb_free_paths(w, RW_LINE_PATHS + 0xc, RW_LINE_COUNT + 0xc);
	rb_free_paths(w, RW_ENDBOX_PATHS, RW_ENDBOX_COUNT);
	rb_free_paths(w, RW_ENDBOX_PATHS + 4, RW_ENDBOX_COUNT + 4);
	rb_free_paths(w, RW_ENDBOX_PATHS + 8, RW_ENDBOX_COUNT + 8);
	rb_free_paths(w, RW_ENDBOX_PATHS + 0xc, RW_ENDBOX_COUNT + 0xc);
}

/* 0x001b4720  Rowing (gap; CRowing_World.cpp inferred)
Draws the level: the debug paths (draw 0), else the world set's passes 1 and 2, or the
solid instance when g_RowDrawSolid is set; then the race line debug drawing when on. */
void RowWorld_Draw(void *self, int draw)
{
	unsigned char *w = self;

	bdSetRenderState(3, 0, 0);
	bdSetWorldMatrix((float *)DAT_002855f0);
	if (draw == 0) {
		RowWorld_DrawCollision(w);
	} else if (g_RowDrawSolid == 0) {
		bdSetWorldMatrix((float *)DAT_002855f0);
		baDrawSet(RB_PTR(w, RW_WORLDSET), 1);
		baDrawSet(RB_PTR(w, RW_WORLDSET), 2);
	} else if (RB_PTR(w, RW_SOLID_INST) != NULL) {
		baDrawActorInstance(RB_PTR(w, RW_SOLID_INST), 7);
	}
	if (g_RowDrawPaths != 0)
		RowWorld_DrawRaceLine(w);
}
