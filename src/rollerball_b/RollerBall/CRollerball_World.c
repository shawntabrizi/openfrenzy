/*
RollerBall/CRollerball_World.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerball_World.cpp (heap tags of 0x00195400 and
0x00195930): the level of a variation (game + 0x4c, vtable 0x0027488c, 0x1318 bytes).
Load reads "<level>\roller<level>.lom" (a ParseMap text file): the "World" node is the
landscape, every "Object" node names a model and its "Instance" children place it. The
instance name says what it is: startbank<n>, endpoint<n>, goalbank<n>, respawn<n>,
universe (the sky), platform (a solid part with collision and shadow), light (ignored),
else scenery (an animated actor; "alpha" ones are drawn last).

0x00195400, 0x001965a0 and 0x00195930 (the ParseMap callbacks) are missing from the
function table. The draw functions before 0x001953e0 have no tag; they are placed here
by address order.

Layout:
  +0x004 char[0x80] level name          +0x084 char[0x80] current object name
  +0x104 byte: the object's model is not loaded yet       +0x108 bkPackage *
  +0x10c platforms[10] (0x88 bytes), count +0x65c:
         +0x00/+0x01/+0x02 flags (+0x02 the shadow is drawn), +0x04 pos, +0x14 rotation
         (quaternion), +0x24 scale, +0x34 matrix, +0x74 set, +0x78 collision mesh,
         +0x7c name CRC, +0x80 shadow actor, +0x84 shadow instance
  +0x660 start positions[4] (pos, rotation), count +0x6e0
  +0x6e4 end positions[4] (pos, rotation), count +0x764
  +0x768 universe objects (0x74 bytes: pos, rotation, scale, matrix +0x30, set +0x70);
         the count at +0x7dc follows the first record
  +0x7e0 actors[20] (resource, name CRC), count +0x880
  +0x884 scenery[30] (0x3c bytes: pos, rotation, scale, +0x30 instance, +0x34 animation,
         +0x38 name CRC), count +0xf8c
  +0xf90 alpha scenery[30], count +0x1314
*/

#include "rollerball_b.h"

#define RBW_LEVEL 0x004
#define RBW_OBJECT 0x084
#define RBW_NEW_OBJECT 0x104
#define RBW_PACKAGE 0x108
#define RBW_PLATFORMS 0x10c
#define RBW_PLATFORM_SIZE 0x88
#define RBW_PLATFORM_COUNT 0x65c
#define RBW_STARTS 0x660
#define RBW_START_COUNT 0x6e0
#define RBW_ENDS 0x6e4
#define RBW_END_COUNT 0x764
#define RBW_UNIVERSE 0x768
#define RBW_UNIVERSE_SIZE 0x74
#define RBW_UNIVERSE_COUNT 0x7dc
#define RBW_ACTORS 0x7e0
#define RBW_ACTOR_COUNT 0x880
#define RBW_SCENERY 0x884
#define RBW_SCENERY_COUNT 0xf8c
#define RBW_ALPHA 0xf90
#define RBW_ALPHA_COUNT 0x1314
#define RBW_SCENERY_SIZE 0x3c
#define RBW_SIZE 0x1318

#define RBW_PLATFORM(w, i) ((char *)(w) + RBW_PLATFORMS + (i) * RBW_PLATFORM_SIZE)
#define RBW_UNIVERSE_REC(w, i) ((char *)(w) + RBW_UNIVERSE + (i) * RBW_UNIVERSE_SIZE)

/* per-variation settings (records of 0x40 bytes from 0x002b4ba8): the scenery is drawn
and animated */
#define rbb_SceneryOn() (RBD_I(0x002b4ba8 + rbb_Variation * 0x40) != 0)

/* rollerball_a 0x0018e540 (CRollerBallDirect3D, game + 0x26f4): the texture factor
alpha stage. The x86 code ignores ECX, so the call passes only the alpha (the table
prototype; a direct call, so a changed prototype fails to compile). */
#define rbb_SetAlphaStage(a) FUN_0018e540(a)

static const char rbb_WorldFile[] = "C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerball_World.cpp";

/* 0x0027488c  world vtable */
const void *const rbb_vtable_0027488c[1] = {
	(const void *)FUN_001953e0, /* +0x00 scalar deleting destructor */
};

/* out = a * b with the aligned (SIMD) or the plain bKernel multiply, chosen by the
alignment of the three matrices as in the original (both give the same product) */
static void rbb_MatrixMultiply(float *out, const float *a, const float *b)
{
	if ((((uintptr_t)out | (uintptr_t)a | (uintptr_t)b) & 0xf) == 0)
		bmMatrixMultiplyCopy(out, a, b);
	else
		bmMatrixMultiply(out, a, b);
}

/* copy pos, rotation and scale (12 floats) into an actor instance (+0xc0, +0xe0, +0xd0) */
static void rbb_PlaceInstance(void *inst, const float *prs)
{
	memcpy(RBB_V(inst, 0xc0), prs + 0, 16);
	memcpy(RBB_V(inst, 0xe0), prs + 4, 16);
	memcpy(RBB_V(inst, 0xd0), prs + 8, 16);
}

/* read PosX..Z, RotX..W and SclX..Z of a node: prs = pos (w 1), rotation, scale (w 1) */
static void rbb_ReadTransform(int node, float *prs)
{
	static const char *const names[11] = {
		"PosX", "PosY", "PosZ", "RotX", "RotY", "RotZ", "RotW", "SclX", "SclY", "SclZ", NULL
	};
	static const unsigned char where[10] = { 0, 1, 2, 4, 5, 6, 7, 8, 9, 10 };
	int i;

	for (i = 0; i < 10; i++)
		if (ParseMap_GetFloat(node, names[i], (undefined4 *)&prs[where[i]]) == 0)
			prs[where[i]] = i < 7 ? 0.0f : 1.0f;
	prs[3] = 1.0f;
	prs[11] = 1.0f;
}

/* 0x00194a40  RollerBall/CRollerball_World.cpp (inferred)
Constructor. */
void CRollerballWorld_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_0027488c;
}

/* 0x00194a50  RollerBall/CRollerball_World.cpp (inferred)
Destructor. */
void CRollerballWorld_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_0027488c;
}

/* 0x00194a60  RollerBall/CRollerball_World.cpp (inferred)
Free the level's resources and instances. */
void CRollerballWorld_Shutdown(void *self)
{
	int i;

	for (i = 0; i < RBB_I(self, RBW_UNIVERSE_COUNT); i++) {
		struct bkResource **set = (struct bkResource **)RBB_ADDR(RBW_UNIVERSE_REC(self, i), 0x70);
		if (*set != NULL) {
			bkDeleteResource(*set);
			*set = NULL;
		}
	}
	for (i = 0; i < RBB_I(self, RBW_SCENERY_COUNT); i++) {
		void **inst = (void **)RBB_ADDR(self, RBW_SCENERY + i * RBW_SCENERY_SIZE + 0x30);
		if (*inst != NULL) {
			baDeleteActorInstance((struct BActorInstance *)*inst);
			*inst = NULL;
		}
	}
	for (i = 0; i < RBB_I(self, RBW_ALPHA_COUNT); i++) {
		void **inst = (void **)RBB_ADDR(self, RBW_ALPHA + i * RBW_SCENERY_SIZE + 0x30);
		if (*inst != NULL) {
			baDeleteActorInstance((struct BActorInstance *)*inst);
			*inst = NULL;
		}
	}
	for (i = 0; i < RBB_I(self, RBW_ACTOR_COUNT); i++) {
		struct bkResource **actor = (struct bkResource **)RBB_ADDR(self, RBW_ACTORS + i * 8);
		if (*actor != NULL) {
			bkDeleteResource(*actor);
			*actor = NULL;
		}
	}
	for (i = 0; i < RBB_I(self, RBW_PLATFORM_COUNT); i++) {
		char *p = RBW_PLATFORM(self, i);
		if (RBB_P(p, 0x74) != NULL) {
			bkDeleteResource(RBB_P(p, 0x74));
			RBB_P(p, 0x74) = NULL;
		}
		if (RBB_P(p, 0x78) != NULL) {
			bkDeleteResource(RBB_P(p, 0x78));
			RBB_P(p, 0x78) = NULL;
		}
		if (RBB_P(p, 0x80) != NULL) {
			bkDeleteResource(RBB_P(p, 0x80));
			RBB_P(p, 0x80) = NULL;
		}
		if (RBB_P(p, 0x84) != NULL) {
			baDeleteActorInstance((struct BActorInstance *)RBB_P(p, 0x84));
			RBB_P(p, 0x84) = NULL;
		}
	}
}

/* animate a scenery list */
static void rbb_AnimateScenery(char *list, int count, float dt)
{
	int i;

	for (i = 0; i < count; i++) {
		char *s = list + i * RBW_SCENERY_SIZE;
		struct BActorInstance *inst = (struct BActorInstance *)RBB_P(s, 0x30);
		if (RBB_P(s, 0x34) != NULL) {
			baUpdateAnimations(inst, (struct BActorNode *)RBB_P(inst, 0x134), dt, NULL);
			baPrepActorInstance(inst, 1);
		}
	}
}

/* 0x00194bb0  RollerBall/CRollerball_World.cpp (inferred)
Animate the scenery (when the variation shows it). */
void CRollerballWorld_Update(void *self, float dt)
{
	if (!rbb_SceneryOn())
		return;
	rbb_AnimateScenery((char *)self + RBW_SCENERY, RBB_I(self, RBW_SCENERY_COUNT), dt);
	rbb_AnimateScenery((char *)self + RBW_ALPHA, RBB_I(self, RBW_ALPHA_COUNT), dt);
}

/* draw the universe objects with passes 1 (and 2) */
static void rbb_DrawUniverse(void *self, int both_passes)
{
	int i;

	for (i = 0; i < RBB_I(self, RBW_UNIVERSE_COUNT); i++) {
		char *u = RBW_UNIVERSE_REC(self, i);
		bdSetWorldMatrix(RBB_V(u, 0x30));
		baDrawSet((struct BSet *)RBB_P(u, 0x70), 1);
		if (both_passes)
			baDrawSet((struct BSet *)RBB_P(u, 0x70), 2);
	}
}

/* draw the platforms with passes 1 (and 2) */
static void rbb_DrawPlatformSets(void *self, int both_passes)
{
	int i;

	for (i = 0; i < RBB_I(self, RBW_PLATFORM_COUNT); i++) {
		char *p = RBW_PLATFORM(self, i);
		bdSetWorldMatrix(RBB_V(p, 0x34));
		baDrawSet((struct BSet *)RBB_P(p, 0x74), 1);
		if (both_passes)
			baDrawSet((struct BSet *)RBB_P(p, 0x74), 2);
	}
}

/* draw a scenery list */
static void rbb_DrawScenery(char *list, int count, int passes)
{
	int i;

	for (i = 0; i < count; i++)
		baDrawActorInstance((struct BActorInstance *)RBB_P(list + i * RBW_SCENERY_SIZE, 0x30), passes);
}

/* 0x00194c70  RollerBall/CRollerball_World.cpp (inferred)
Draw the "base" level. */
void CRollerballWorld_DrawBase(void *self)
{
	bdPushRenderState(4, 1, 0);
	if (rbb_SceneryOn()) {
		bdPushRenderState(3, 1, 0);
		rbb_DrawUniverse(self, 1);
	}
	if (rbb_SceneryOn())
		rbb_SetAlphaStage(0x20);
	rbb_DrawPlatformSets(self, 1);
	if (rbb_SceneryOn()) {
		SMEffects_ResetDarkenStage();
		bdPopRenderState(3);
	}
	if (rbb_SceneryOn()) {
		rbb_DrawScenery((char *)self + RBW_SCENERY, RBB_I(self, RBW_SCENERY_COUNT), 7);
		bdSetRenderState(10, 1, 0);
		rbb_DrawScenery((char *)self + RBW_ALPHA, RBB_I(self, RBW_ALPHA_COUNT), 7);
	}
	bdPopRenderState(4);
}

/* the platform shadows whose flag (+0x02) is set */
static void rbb_DrawPlatformShadows(void *self)
{
	int i;

	for (i = 0; i < RBB_I(self, RBW_PLATFORM_COUNT); i++) {
		char *p = RBW_PLATFORM(self, i);
		if (RBB_P(p, 0x84) != NULL && RBB_C(p, 0x02) != 0)
			baDrawActorInstance((struct BActorInstance *)RBB_P(p, 0x84), 7);
	}
}

/* 0x00194e10  RollerBall/CRollerball_World.cpp (inferred)
Draw the platform shadows ("base"). */
void CRollerballWorld_DrawShadowsBase(void *self)
{
	bdPushRenderState(4, 1, 0);
	rbb_DrawPlatformShadows(self);
	bdPopRenderState(4);
}

/* 0x00194e70  RollerBall/CRollerball_World.cpp (inferred)
Draw the "sumo" level. */
void CRollerballWorld_DrawSumo(void *self)
{
	bdPushRenderState(4, 1, 0);
	if (rbb_SceneryOn())
		rbb_DrawUniverse(self, 1);
	if (rbb_SceneryOn())
		rbb_SetAlphaStage(0x20);
	rbb_DrawPlatformSets(self, 0);
	if (rbb_SceneryOn())
		SMEffects_ResetDarkenStage();
	if (rbb_SceneryOn()) {
		rbb_SetAlphaStage(0x20);
		rbb_DrawScenery((char *)self + RBW_SCENERY, RBB_I(self, RBW_SCENERY_COUNT), 7);
		SMEffects_ResetDarkenStage();
		bdSetRenderState(10, 1, 0);
		rbb_DrawScenery((char *)self + RBW_ALPHA, RBB_I(self, RBW_ALPHA_COUNT), 7);
	}
	bdPopRenderState(4);
}

/* 0x00195010  RollerBall/CRollerball_World.cpp (inferred)
Draw the "race" level. */
void CRollerballWorld_DrawRace(void *self)
{
	if (rbb_SceneryOn())
		rbb_DrawUniverse(self, 1);
	rbb_DrawPlatformSets(self, 1);
	if (rbb_SceneryOn()) {
		rbb_DrawScenery((char *)self + RBW_SCENERY, RBB_I(self, RBW_SCENERY_COUNT), 1);
		bdSetRenderState(10, 1, 0);
		rbb_DrawScenery((char *)self + RBW_ALPHA, RBB_I(self, RBW_ALPHA_COUNT), 7);
		rbb_DrawScenery((char *)self + RBW_SCENERY, RBB_I(self, RBW_SCENERY_COUNT), 6);
	}
}

/* 0x00195170  RollerBall/CRollerball_World.cpp (inferred)
Draw the "drop" level. */
void CRollerballWorld_DrawDrop(void *self)
{
	if (rbb_SceneryOn())
		rbb_DrawUniverse(self, 0);
	if (rbb_SceneryOn())
		rbb_SetAlphaStage(0x20);
	rbb_DrawPlatformSets(self, 0);
	if (rbb_SceneryOn())
		SMEffects_ResetDarkenStage();
	if (rbb_SceneryOn()) {
		rbb_SetAlphaStage(0x20);
		rbb_DrawScenery((char *)self + RBW_SCENERY, RBB_I(self, RBW_SCENERY_COUNT), 7);
		SMEffects_ResetDarkenStage();
		bdSetRenderState(10, 1, 0);
		rbb_DrawScenery((char *)self + RBW_ALPHA, RBB_I(self, RBW_ALPHA_COUNT), 7);
	}
}

/* 0x001952f0  RollerBall/CRollerball_World.cpp (inferred)
Draw the platform shadows. */
void CRollerballWorld_DrawShadows(void *self)
{
	rbb_DrawPlatformShadows(self);
}

/* 0x00195340  RollerBall/CRollerball_World.cpp (inferred)
Draw only the platforms (both passes). */
void CRollerballWorld_DrawPlatforms(void *self)
{
	if (rbb_SceneryOn())
		rbb_SetAlphaStage(0x20);
	rbb_DrawPlatformSets(self, 1);
	if (!rbb_SceneryOn())
		return;
	SMEffects_ResetDarkenStage();
}

/* 0x001953e0  RollerBall/CRollerball_World.cpp (inferred)
Scalar deleting destructor. */
void *CRollerballWorld_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_0027488c;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* load a platform's shadow actor (type 1) and make its instance at the transform `prs`;
returns 0 when the instance could not be made (a missing shadow model is not an error) */
static int rbb_LoadPlatformShadow(void *self, char *p, const char *path, const float *prs, int line)
{
	void *actor = bLoadResource((struct bkPackage *)RBB_P(self, RBW_PACKAGE), path, 1, 0xdefa);
	void *inst;

	RBB_P(p, 0x80) = actor;
	if (actor == NULL)
		return 1;
	inst = baInitActorInstance(NULL, (struct BActor *)actor, 1, rbb_WorldFile, line);
	RBB_P(p, 0x84) = inst;
	if (inst == NULL) {
		bkPrintf("*** ERROR *** Could not create shadow actor instance [%s]\n", path);
		return 0;
	}
	rbb_PlaceInstance(inst, prs);
	baPrepActorInstance((struct BActorInstance *)inst, 1);
	return 1;
}

/* 0x00195400  RollerBall/CRollerball_World.cpp (missing from the function table)
ParseMap callback for the "World" node: the landscape, added as a platform (model
"<level>\<name>.obe", collision "_col", shadow "_shad"). Returns 2 (do not walk the
children) on an error or when the platform table is full, else 0. */
int FUN_00195400(int node, void *self)
{
	char path[128];
	float prs[12];
	float trans[16];
	float rot[16];
	const char *name = ParseMap_GetString(node, "Name");
	const char *level = (const char *)self + RBW_LEVEL;
	char *p;
	void *res;
	int count;

	sprintf(path, "%s\\%s.obe", level, name);
	_strlwr(path);
	rbb_ReadTransform(node, prs);
	p = RBW_PLATFORM(self, RBB_I(self, RBW_PLATFORM_COUNT));
	memcpy(RBB_V(p, 0x04), prs + 0, 16);
	memcpy(RBB_V(p, 0x14), prs + 4, 16);
	memcpy(RBB_V(p, 0x24), prs + 8, 16);

	res = bLoadResource((struct bkPackage *)RBB_P(self, RBW_PACKAGE), path, 6, 0xdefa);
	RBB_P(p, 0x74) = res;
	if (res == NULL) {
		bkPrintf("*** ERROR *** Could not find level [%s]\n", path);
		return 2;
	}
	sprintf(path, "%s\\%s_col.obe", level, name);
	res = bLoadResource((struct bkPackage *)RBB_P(self, RBW_PACKAGE), path, 7, 0xdefa);
	RBB_P(p, 0x78) = res;
	if (res == NULL) {
		bkPrintf("ERROR: Collision mesh [%s] not found\n", path);
		return 2;
	}
	sprintf(path, "%s\\%s_shad.obe", level, name);
	_strlwr(path);
	if (!rbb_LoadPlatformShadow(self, p, path, prs, 0xc6))
		return 2;

	bmMatrixTranslation(trans, prs[0], prs[1], prs[2]);
	bmQuaternionToMatrix(rot, prs + 4);
	rbb_MatrixMultiply(RBB_V(p, 0x34), trans, rot);
	RBB_B(p, 0x00) = 1;
	RBB_B(p, 0x01) = 1;
	RBB_B(p, 0x02) = 1;
	count = RBB_I(self, RBW_PLATFORM_COUNT) + 1;
	RBB_I(self, RBW_PLATFORM_COUNT) = count;
	return count < 10 ? 0 : 2;
}

/* 0x00195930  RollerBall/CRollerball_World.cpp (missing from the function table)
ParseMap callback for every "Instance" node of an "Object" (see the file comment).
Returns 2 on an error or when a position table is full, else 0. */
int FUN_00195930(int node, void *self)
{
	char path[128];
	float prs[12];
	float trans[16];
	float rot[16];
	char *name = ParseMap_GetString(node, "Name");
	const char *level = (const char *)self + RBW_LEVEL;
	char *object = (char *)self + RBW_OBJECT;

	path[0] = '\0';   /* printed by one error message before it is set (stack garbage in the original) */
	_strlwr(name);
	rbb_ReadTransform(node, prs);

	if (strstr(name, "startbank") != NULL) {
		int i = atol(name + 9) - 1;
		int n;
		if (i > 3) {
			bkPrintf("*** ERROR *** Invalid index for start position [%d]\n", i);
			return 2;
		}
		memcpy(RBB_V(self, RBW_STARTS + i * 0x20), prs + 0, 16);
		memcpy(RBB_V(self, RBW_STARTS + i * 0x20 + 0x10), prs + 4, 16);
		n = RBB_I(self, RBW_START_COUNT) + 1;
		RBB_I(self, RBW_START_COUNT) = n;
		if (n > 3)
			return 2;
	} else if (strstr(name, "endpoint") != NULL) {
		int i = atol(name + 8) - 1;
		int n;
		if (i > 3) {
			bkPrintf("*** ERROR *** Invalid index for end position [%d]\n", i);
			return 2;
		}
		memcpy(RBB_V(self, RBW_ENDS + i * 0x20), prs + 0, 16);
		memcpy(RBB_V(self, RBW_ENDS + i * 0x20 + 0x10), prs + 4, 16);
		n = RBB_I(self, RBW_END_COUNT) + 1;
		RBB_I(self, RBW_END_COUNT) = n;
		if (n > 3)
			return 2;
	} else if (strstr(name, "goalbank") != NULL) {
		/* rollerball_a 0x00191530: the goal system's goal <n> */
		RBB_FN(FUN_00191530, void, (void *, int, float *, float *))(rbb_GoalSystem, atol(name + 8) - 1,
			prs + 0, prs + 4);
		RBB_B(self, RBW_NEW_OBJECT) = 0;
		return 0;
	} else if (strstr(name, "respawn") != NULL) {
		/* rollerball_a 0x00190910: the respawn system (game + 0x1c50) point <n> */
		RBB_FN(FUN_00190910, void, (void *, int, float *, float *))(rbb_Game + 0x1c50, atol(name + 7) - 1,
			prs + 0, prs + 4);
		RBB_B(self, RBW_NEW_OBJECT) = 0;
		return 0;
	} else if (strstr(name, "universe") != NULL) {
		int i = RBB_I(self, RBW_UNIVERSE_COUNT);
		char *u = RBW_UNIVERSE_REC(self, i);
		void *res;
		sprintf(path, "%s\\objects\\%s.obe", level, object);
		_strlwr(path);
		res = bLoadResource((struct bkPackage *)RBB_P(self, RBW_PACKAGE), path, 6, 0xdefa);
		RBB_P(u, 0x70) = res;
		if (res == NULL) {
			bkPrintf("*** ERROR *** Could not load universe object [%s]\n", path);
			return 2;
		}
		memcpy(RBB_V(u, 0x00), prs, 48);
		bmMatrixTranslation(trans, prs[0], prs[1], prs[2]);
		bmQuaternionToMatrix(rot, prs + 4);
		rbb_MatrixMultiply(RBB_V(u, 0x30), trans, rot);
		RBB_I(self, RBW_UNIVERSE_COUNT) += 1;
		RBB_B(self, RBW_NEW_OBJECT) = 0;
		return 0;
	} else if (strstr(name, "platform") != NULL) {
		int i = RBB_I(self, RBW_PLATFORM_COUNT);
		char *p = RBW_PLATFORM(self, i);
		void *res;
		sprintf(path, "%s\\objects\\%s.obe", level, object);
		_strlwr(path);
		res = bLoadResource((struct bkPackage *)RBB_P(self, RBW_PACKAGE), path, 6, 0xdefa);
		RBB_P(p, 0x74) = res;
		if (res == NULL) {
			bkPrintf("*** ERROR *** Could not find level [%s]\n", path);
			return 2;
		}
		sprintf(path, "%s\\objects\\%s_col.obe", level, object);
		_strlwr(path);
		res = bLoadResource((struct bkPackage *)RBB_P(self, RBW_PACKAGE), path, 7, 0xdefa);
		RBB_P(p, 0x78) = res;
		if (res == NULL) {
			bkPrintf("ERROR: Collision mesh [%s] not found\n", path);
			return 2;
		}
		sprintf(path, "%s\\objects\\%s_shad.obe", level, object);
		_strlwr(path);
		if (!rbb_LoadPlatformShadow(self, p, path, prs, 0x1c7))
			return 2;
		RBB_U(p, 0x7c) = bkCRCString(name);
		/* the platform's own position is the origin (its model is in world space) */
		prs[0] = 0.0f;
		prs[1] = 0.0f;
		prs[2] = 0.0f;
		memcpy(RBB_V(p, 0x04), prs + 0, 16);
		memcpy(RBB_V(p, 0x14), prs + 4, 16);
		memcpy(RBB_V(p, 0x24), prs + 8, 16);
		bmMatrixTranslation(trans, 0.0f, 0.0f, 0.0f);
		bmQuaternionToMatrix(rot, prs + 4);
		rbb_MatrixMultiply(RBB_V(p, 0x34), trans, rot);
		RBB_B(p, 0x00) = 1;
		RBB_B(p, 0x01) = 1;
		RBB_B(p, 0x02) = 1;
		RBB_I(self, RBW_PLATFORM_COUNT) += 1;
		RBB_B(self, RBW_NEW_OBJECT) = 0;
		return 0;
	} else if (strstr(name, "light") == NULL) {
		char *s;
		void *inst;
		void *anim;
		if (RBB_C(self, RBW_NEW_OBJECT) != 0) {
			/* the first instance of the object: load its model */
			int n = RBB_I(self, RBW_ACTOR_COUNT);
			void *res;
			sprintf(path, "%s\\objects\\%s.obe", level, object);
			_strlwr(path);
			res = bLoadResource((struct bkPackage *)RBB_P(self, RBW_PACKAGE), path, 1, 0xdefa);
			RBB_P(self, RBW_ACTORS + n * 8) = res;
			if (res == NULL) {
				bkPrintf("*** ERROR *** Could not load actor [%s]\n", path);
				return 2;
			}
			_strlwr(object);
			RBB_U(self, RBW_ACTORS + n * 8 + 4) = bkCRCString(object);
			RBB_I(self, RBW_ACTOR_COUNT) += 1;
		}
		if (strstr(name, "alpha") == NULL) {
			s = (char *)self + RBW_SCENERY + RBB_I(self, RBW_SCENERY_COUNT) * RBW_SCENERY_SIZE;
			RBB_I(self, RBW_SCENERY_COUNT) += 1;
		} else {
			s = (char *)self + RBW_ALPHA + RBB_I(self, RBW_ALPHA_COUNT) * RBW_SCENERY_SIZE;
			RBB_I(self, RBW_ALPHA_COUNT) += 1;
		}
		/* the last loaded actor */
		inst = baInitActorInstance(NULL,
			(struct BActor *)RBB_P(self, RBW_ACTORS - 8 + RBB_I(self, RBW_ACTOR_COUNT) * 8), 1,
			rbb_WorldFile, 0x24e);
		RBB_P(s, 0x30) = inst;
		if (inst == NULL) {
			bkPrintf("*** ERROR *** Could not create actor instance [%s]\n", path);
			return 2;
		}
		memcpy(RBB_V(s, 0x00), prs, 48);
		rbb_PlaceInstance(inst, prs);
		baPrepActorInstance((struct BActorInstance *)inst, 1);
		anim = baFindSubObject((struct BActorInstance *)inst, "default");
		RBB_P(s, 0x34) = anim;
		if (anim != NULL) {
			baFlushAnimations(inst, RBB_P(inst, 0x134), 1, 0);
			baQueueAnimation((struct BActorInstance *)inst, (struct BActorNode *)RBB_P(inst, 0x134),
				(struct BAnim *)anim, 0x400, -1, 1, NULL, NULL);
		}
		RBB_U(s, 0x38) = bkCRCString(name);
	}
	RBB_B(self, RBW_NEW_OBJECT) = 0;
	return 0;
}

/* 0x001965a0  RollerBall/CRollerball_World.cpp (missing from the function table)
ParseMap callback for an "Object" node: remember its name, and walk its "Instance"
children with 0x00195930. Returns 2: the walk does not go into the children again. */
int FUN_001965a0(int node, void *self)
{
	strcpy((char *)self + RBW_OBJECT, ParseMap_GetString(node, "Name"));
	RBB_B(self, RBW_NEW_OBJECT) = 1;
	ParseMap_Walk(node, (int)(uintptr_t)FUN_00195930, (undefined4)(uintptr_t)self,
		(int)(uintptr_t)"Instance");
	return 2;
}

/* 0x00196500  RollerBall/CRollerball_World.cpp (inferred)
Draw the level of the variation ("race2" has none here). */
void CRollerballWorld_Draw(void *self)
{
	bdSetRenderState(6, 0, 0);
	switch (rbb_Variation) {
	case 0:
		CRollerballWorld_DrawBase(self);
		return;
	case 1:
		CRollerballWorld_DrawSumo(self);
		return;
	case 3:
		CRollerballWorld_DrawRace(self);
		return;
	case 4:
		CRollerballWorld_DrawDrop(self);
		return;
	}
}

/* 0x00196560  RollerBall/CRollerball_World.cpp (inferred)
Draw the platform shadows of the variation. */
void CRollerballWorld_DrawShadowPass(void *self)
{
	switch (rbb_Variation) {
	case 0:
		CRollerballWorld_DrawShadowsBase(self);
		return;
	case 1:
	case 3:
	case 4:
		CRollerballWorld_DrawShadows(self);
		return;
	}
}

/* 0x001965f0  RollerBall/CRollerball_World.cpp (inferred; "LoadLOMFile")
Read a LOM file: the "World" and "Object" nodes, and the intro camera path. */
int CRollerballWorld_LoadLOM(void *self, struct bkPackage *package, const char *path)
{
	uint size;
	void *data;
	int *map;

	data = bkLoadFile(package, path, NULL, &size, NULL, 0);
	if (data == NULL) {
		bkPrintf("LoadLOMFile ERROR: Could not load [%s]\n", path);
		return 0;
	}
	map = ParseMap_Load((char *)data, 2, (int)size);
	if (map == NULL) {
		bkPrintf("LoadLOMFile ERROR: Parsing [%s]\n", path);
		return 0;
	}
	ParseMap_WalkMap((int)(uintptr_t)map, (int)(uintptr_t)FUN_00195400, (undefined4)(uintptr_t)self,
		(int)(uintptr_t)"World");
	ParseMap_WalkMap((int)(uintptr_t)map, (int)(uintptr_t)FUN_001965a0, (undefined4)(uintptr_t)self,
		(int)(uintptr_t)"Object");
	CLIntroCam_Load((int)(uintptr_t)map, (undefined4)(uintptr_t)self);
	ParseMap_Free(map);
	return 1;
}

/* 0x001966a0  RollerBall/CRollerball_World.cpp (inferred)
Load the level `level` ("base", "sumo", ...) from `package`: clear the object (the
original clears all 0x1318 bytes, the vtable pointer included), then read
"<level>\roller<level>.lom". */
int CRollerballWorld_Load(void *self, struct bkPackage *package, const char *level)
{
	char path[256];

	memset(self, 0, RBW_SIZE);
	strcpy((char *)self + RBW_LEVEL, level);
	RBB_P(self, RBW_PACKAGE) = package;
	sprintf(path, "%s\\roller%s.lom", level, level);
	return CRollerballWorld_LoadLOM(self, package, path) != 0;
}
