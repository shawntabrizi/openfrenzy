/*
Clusteroids/CClust_Arena.c  <-  C:\BlitzParty\Game\Src\Clusteroids\CClust_Arena.cpp

The arena: the models and actors of the level (players' ships and characters, pods,
gates, the city, traffic, the attack ship, doors, spinner, grid, power-up), the flashing
lights around the arena, the two "grid trail" walkers, and the game set-up and reset.

Tagged with CClust_Arena.cpp: 0x000da370 (allocation file name). The functions from
0x000daad0 to 0x000dbd60 (gap, before CClust_Trails.cpp) use the same globals; their file
is inferred.

Globals (original addresses):
  0x002f8f70[4]  ship models ("players\models\<name 1>")      0x002f8f80[4] (<name 2>)
  0x002f8f90[4]  character models ("characters\models\<..>")   0x002f8fa0[4]
  0x002f9130[4]  pod models ("pods\models\<..>"), actors 0x002f9140 (0x170 each)
  0x002f7f94[4]  gate models (Collector), actors 0x002f7fb0; neutral gate 0x002f8570
  0x002f7a70[15] the lights (0x2c each), 0x002f9b40 their sprite vertices
  0x002f7dc8[2]  the grid trail walkers (0x38 each)
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define CL_ACTOR(addr) ((struct BActorInstance *)CL_ADDR(addr))

/* the per-character model names (0x00294c08, 0x300 bytes per character: six names of
   0x80 bytes: ship, ship 2, character, character 2, pod, gate) */
#define CL_MODEL_NAME(character, slot) ((char *)CL_ADDR(0x00294c08) + (character) * 0x300 + (slot) * 0x80)

/* rodata names */
static const char cl_name_open[] = "open";   /* 0x002692a0 */
static const char cl_name_skip[] = "skip";   /* 0x0026868c */
static const char cl_name_spin[] = "spin";   /* 0x002640a8 */

/* 0x000da370  Clusteroids/CClust_Arena.cpp
Loads the models of the arena and makes their actors. */
void ClustArena_LoadModels(void)
{
	char name[256];
	int player;
	int character;
	struct BActor *model;

	for (player = 0; player < 4; player++) {
		character = DAT_002baed4[player * 10];
		sprintf(name, "players\\models\\%s", CL_MODEL_NAME(character, 0));
		CL_VAR(void *, 0x002f8f70 + player * 4) = bLoadResource(NULL, name, 1, 0xdefa);
		sprintf(name, "players\\models\\%s", CL_MODEL_NAME(character, 1));
		CL_VAR(void *, 0x002f8f80 + player * 4) = bLoadResource(NULL, name, 1, 0xdefa);
		sprintf(name, "characters\\models\\%s", CL_MODEL_NAME(character, 2));
		CL_VAR(void *, 0x002f8f90 + player * 4) = bLoadResource(NULL, name, 1, 0xdefa);
		sprintf(name, "characters\\models\\%s", CL_MODEL_NAME(character, 3));
		CL_VAR(void *, 0x002f8fa0 + player * 4) = bLoadResource(NULL, name, 1, 0xdefa);
		sprintf(name, "pods\\models\\%s", CL_MODEL_NAME(character, 4));
		model = (struct BActor *)bLoadResource(NULL, name, 1, 0xdefa);
		CL_VAR(void *, 0x002f9130 + player * 4) = model;
		baInitActorInstance(CL_ACTOR(0x002f9140 + player * 0x170), model, 0, CL_FILE_ARENA, 0x262);
		baPrepActorInstance(CL_ACTOR(0x002f9140 + player * 0x170), 1);
		CL_VAR(float, 0x002f9140 + player * 0x170 + 0xd8) = 0.35f;
		CL_VAR(float, 0x002f9140 + player * 0x170 + 0xd4) = 0.35f;
		CL_VAR(float, 0x002f9140 + player * 0x170 + 0xd0) = 0.35f;
	}
	CL_VAR(void *, 0x002f8f60) = bLoadResource(NULL, "fonts\\gothic.ttf", 3, 0xdefa);
	CL_VAR(void *, 0x002f8f64) = bLoadResource(NULL, "fonts\\gunship.ttf", 3, 0xdefa);

	/* bullet */
	CL_VAR(void *, 0x002f9700) = bLoadResource(NULL, "bullets\\models\\testbullet.obe", 1, 0xdefa);
	baInitActorInstance(CL_ACTOR(0x002f9710), CL_VAR(struct BActor *, 0x002f9700), 0, CL_FILE_ARENA, 0x26d);
	CL_VAR(float, 0x002f97e8) = 0.2f;
	CL_VAR(float, 0x002f97e4) = 0.2f;
	CL_VAR(float, 0x002f97e0) = 0.2f;
	baPrepActorInstance(CL_ACTOR(0x002f9710), 1);

	/* colour tile */
	CL_VAR(void *, 0x002f8fb0) = bLoadResource(NULL, "pods\\models\\colour_tile_white.obe", 1, 0xdefa);
	baInitActorInstance(CL_ACTOR(0x002f8fc0), CL_VAR(struct BActor *, 0x002f8fb0), 0, CL_FILE_ARENA, 0x273);
	baPrepActorInstance(CL_ACTOR(0x002f8fc0), 1);
	CL_VAR(float, 0x002f9098) = 0.35f;
	CL_VAR(float, 0x002f9094) = 0.35f;
	CL_VAR(float, 0x002f9090) = 0.35f;

	/* textures */
	CL_VAR(void *, 0x002f8f34) = bLoadResource(NULL, "sets\\textures\\grid.gif", 0, 0xdefa);
	CL_VAR(void *, 0x002f8f38) = bLoadResource(NULL, "effects\\textures\\electric01.bmp", 0, 0xdefa);
	CL_VAR(void *, 0x002f8f3c) = bLoadResource(NULL, "effects\\textures\\electric02.bmp", 0, 0xdefa);
	CL_VAR(void *, 0x002f8f40) = bLoadResource(NULL, "effects\\textures\\electric03.bmp", 0, 0xdefa);
	CL_VAR(void *, 0x002f8f44) = bLoadResource(NULL, "effects\\textures\\clouds.bmp", 0, 0xdefa);
	CL_VAR(void *, 0x002f8f4c) = bLoadResource(NULL, "effects\\textures\\light.bmp", 0, 0xdefa);
	CL_VAR(void *, 0x002f8f50) = bLoadResource(NULL, "effects\\textures\\tokenglow.bmp", 0, 0xdefa);

	/* sets */
	CL_VAR(void *, 0x002f8784) = bLoadResource(NULL, "sets\\models\\platform.obe", 6, 0xdefa);
	CL_VAR(void *, 0x002f8788) = bLoadResource(NULL, "sets\\models\\cityscape.obe", 6, 0xdefa);
	CL_VAR(void *, 0x002f878c) = bLoadResource(NULL, "sets\\models\\scenics.obe", 6, 0xdefa);

	/* traffic */
	CL_VAR(void *, 0x002f8dbc) = bLoadResource(NULL, "scenics\\models\\traffic.obe", 1, 0xdefa);
	baInitActorInstance(CL_ACTOR(0x002f8dc0), CL_VAR(struct BActor *, 0x002f8dbc), 4, CL_FILE_ARENA, 0x28f);
	baPrepActorInstance(CL_ACTOR(0x002f8dc0), 1);
	CL_VAR(void *, 0x002f8f30) = baFindSubObject(CL_ACTOR(0x002f8dc0), "traffic");

	/* attack ship */
	CL_VAR(void *, 0x002f879c) = bLoadResource(NULL, "scenics\\models\\attackship1.obe", 1, 0xdefa);
	baInitActorInstance(CL_ACTOR(0x002f87b0), CL_VAR(struct BActor *, 0x002f879c), 4, CL_FILE_ARENA, 0x295);
	baPrepActorInstance(CL_ACTOR(0x002f87b0), 1);
	CL_VAR(float, 0x002f8878) = 0.0f;
	CL_VAR(float, 0x002f8874) = 0.0f;
	CL_VAR(float, 0x002f8870) = 0.0f;
	CL_VAR(void *, 0x002f8d70) = baFindSubObject(CL_ACTOR(0x002f87b0), "intro");
	CL_VAR(void *, 0x002f8da0) = NULL;

	/* doors */
	CL_VAR(void *, 0x002f87a0) = bLoadResource(NULL, "sets\\models\\doors.obe", 1, 0xdefa);
	baInitActorInstance(CL_ACTOR(0x002f8920), CL_VAR(struct BActor *, 0x002f87a0), 4, CL_FILE_ARENA, 0x29c);
	baPrepActorInstance(CL_ACTOR(0x002f8920), 1);
	CL_VAR(float, 0x002f89e8) = 0.0f;
	CL_VAR(float, 0x002f89e4) = 0.0f;
	CL_VAR(float, 0x002f89e0) = 0.0f;
	CL_VAR(void *, 0x002f8d74) = baFindSubObject(CL_ACTOR(0x002f8920), "intro");
	CL_VAR(void *, 0x002f8d84) = baFindSubObject(CL_ACTOR(0x002f8920), "opening");
	CL_VAR(void *, 0x002f8d94) = baFindSubObject(CL_ACTOR(0x002f8920), cl_name_open);
	CL_VAR(void *, 0x002f8da4) = baFindSubObject(CL_ACTOR(0x002f8920), cl_name_skip);

	/* spinner */
	CL_VAR(void *, 0x002f87a4) = bLoadResource(NULL, "sets\\models\\spinner.obe", 1, 0xdefa);
	baInitActorInstance(CL_ACTOR(0x002f8a90), CL_VAR(struct BActor *, 0x002f87a4), 4, CL_FILE_ARENA, 0x2a5);
	baPrepActorInstance(CL_ACTOR(0x002f8a90), 1);
	CL_VAR(float, 0x002f8b58) = 0.0f;
	CL_VAR(float, 0x002f8b54) = 0.0f;
	CL_VAR(float, 0x002f8b50) = 0.0f;
	CL_VAR(void *, 0x002f8d78) = baFindSubObject(CL_ACTOR(0x002f8a90), "intro");
	CL_VAR(void *, 0x002f8d88) = baFindSubObject(CL_ACTOR(0x002f8a90), cl_name_spin);
	CL_VAR(void *, 0x002f8da8) = baFindSubObject(CL_ACTOR(0x002f8a90), cl_name_skip);

	/* grid */
	CL_VAR(void *, 0x002f87a8) = bLoadResource(NULL, "sets\\models\\grid2.obe", 1, 0xdefa);
	baInitActorInstance(CL_ACTOR(0x002f8c00), CL_VAR(struct BActor *, 0x002f87a8), 4, CL_FILE_ARENA, 0x2ad);
	baPrepActorInstance(CL_ACTOR(0x002f8c00), 1);
	CL_VAR(float, 0x002f8cc8) = 0.0f;
	CL_VAR(float, 0x002f8cc4) = 0.0f;
	CL_VAR(float, 0x002f8cc0) = 0.0f;
	CL_VAR(void *, 0x002f8d7c) = baFindSubObject(CL_ACTOR(0x002f8c00), "intro");
	CL_VAR(void *, 0x002f8dac) = baFindSubObject(CL_ACTOR(0x002f8c00), cl_name_skip);

	/* power-up */
	CL_VAR(void *, 0x002f988c) = bLoadResource(NULL, "pickups\\models\\powerupattaknew.obe", 1, 0xdefa);
	baInitActorInstance(CL_ACTOR(0x002f9890), CL_VAR(struct BActor *, 0x002f988c), 4, CL_FILE_ARENA, 0x2b5);
	baPrepActorInstance(CL_ACTOR(0x002f9890), 1);
	CL_VAR(void *, 0x002f9a00) = baFindSubObject(CL_ACTOR(0x002f9890), cl_name_spin);

	/* the Collector gates */
	if (g_ClVariant == 4) {
		for (player = 0; player < 4; player++) {
			sprintf(name, "gates\\models\\%s", CL_MODEL_NAME(DAT_002baed4[player * 10], 5));
			model = (struct BActor *)bLoadResource(NULL, name, 1, 0xdefa);
			CL_VAR(void *, 0x002f7f94 + player * 4) = model;
			baInitActorInstance(CL_ACTOR(0x002f7fb0 + player * 0x170), model, 0, CL_FILE_ARENA, 0x2cb);
			baPrepActorInstance(CL_ACTOR(0x002f7fb0 + player * 0x170), 1);
			CL_VAR(float, 0x002f7fb0 + player * 0x170 + 0xd8) = 1.5f;
			CL_VAR(float, 0x002f7fb0 + player * 0x170 + 0xd4) = 1.5f;
			CL_VAR(float, 0x002f7fb0 + player * 0x170 + 0xd0) = 1.5f;
		}
		CL_VAR(void *, 0x002f7fa4) = bLoadResource(NULL, "gates\\models\\gate_neutral.obe", 1, 0xdefa);
		baInitActorInstance(CL_ACTOR(0x002f8570), CL_VAR(struct BActor *, 0x002f7fa4), 0, CL_FILE_ARENA, 0x2d0);
		baPrepActorInstance(CL_ACTOR(0x002f8570), 1);
		CL_VAR(float, 0x002f8648) = 1.5f;
		CL_VAR(float, 0x002f8644) = 1.5f;
		CL_VAR(float, 0x002f8640) = 1.5f;
	}
	CL_VAR(char, 0x002fa361) = 0;
	CL_VAR(char, 0x002fa329) = 0;
}

/* 0x000daad0  Clusteroids (gap; CClust_Arena.cpp inferred)
Deletes the actors, the light, the trail pool and two buffers. */
void ClustArena_Free(void)
{
	int i;

	baDeleteActorInstance(CL_ACTOR(0x002f9710));
	baDeleteActorInstance(CL_ACTOR(0x002f8fc0));
	baDeleteActorInstance(CL_ACTOR(0x002f8dc0));
	for (i = 0; i < 4; i++)
		baDeleteActorInstance(CL_ACTOR(0x002f87b0 + i * 0x170));
	baDeleteActorInstance(CL_ACTOR(0x002f9890));
	for (i = 0; i < 4; i++) {
		baDeleteActorInstance(CL_ACTOR(0x002f9140 + i * 0x170));
		baDeleteActorInstance((struct BActorInstance *)(CL_PLAYER(i) + PL_SHIP));
		baDeleteActorInstance((struct BActorInstance *)(CL_PLAYER(i) + PL_SHIP2));
		baDeleteActorInstance((struct BActorInstance *)(CL_PLAYER(i) + PL_CHAR));
		baDeleteActorInstance((struct BActorInstance *)(CL_PLAYER(i) + PL_CHAR2));
	}
	for (i = 0; i < 5; i++)
		baDeleteActorInstance(CL_ACTOR(0x002f7fb0 + i * 0x170));
	baDeleteLight(CL_VAR(struct BLight *, 0x002f9a74));
	ClustTrails_Free();
	CL_DELETE(CL_VAR(void *, 0x002fedf8));
	CL_VAR(void *, 0x002fedf8) = NULL;
	CL_DELETE(CL_VAR(void *, 0x002fee00));
	CL_VAR(void *, 0x002fee00) = NULL;
	bkPrintf("Everything freed\n");
}

/* 0x000dabe0  Clusteroids (gap; CClust_Arena.cpp inferred)
Tails: the five fixed spawn points (centre and the four corners, 0x34 bytes each from
0x002f7e38); other variants: none. */
void ClustArena_InitSpawnPoints(void)
{
	if (g_ClVariant == 1) {
		CL_VAR(short, 0x002f7f84) = 0;
		CL_VAR(void *, 0x002f7f88) = CL_ADDR(0x002f7e38);
		CL_VAR(float, 0x002f7e38) = 0.0f;
		CL_VAR(float, 0x002f7e3c) = 0.0f;
		CL_VAR(float, 0x002f7e58) = 1.0f;
		CL_VAR(short, 0x002f7e40) = 0;
		CL_VAR(float, 0x002f7e68) = 5.0f;
		CL_VAR(float, 0x002f7e6c) = 400.0f;
		CL_VAR(float, 0x002f7e70) = 400.0f;
		CL_VAR(float, 0x002f7e8c) = -1.0f;
		CL_VAR(short, 0x002f7e74) = 0;
		CL_VAR(float, 0x002f7e9c) = 5.0f;
		CL_VAR(float, 0x002f7ea0) = -400.0f;
		CL_VAR(float, 0x002f7ea4) = 400.0f;
		CL_VAR(float, 0x002f7ec0) = -1.0f;
		CL_VAR(short, 0x002f7ea8) = 0;
		CL_VAR(float, 0x002f7ed0) = 5.0f;
		CL_VAR(float, 0x002f7ed4) = 400.0f;
		CL_VAR(float, 0x002f7ed8) = -400.0f;
		CL_VAR(float, 0x002f7ef4) = -1.0f;
		CL_VAR(short, 0x002f7edc) = 0;
		CL_VAR(float, 0x002f7f04) = 5.0f;
		CL_VAR(float, 0x002f7f08) = -400.0f;
		CL_VAR(float, 0x002f7f0c) = -400.0f;
		CL_VAR(float, 0x002f7f28) = -1.0f;
		CL_VAR(short, 0x002f7f10) = 0;
		CL_VAR(float, 0x002f7f38) = 5.0f;
		return;
	}
	CL_VAR(short, 0x002f7f84) = 0;
}

/* a flashing light around the arena (0x2c bytes, 15 from 0x002f7a70) */
struct ClustLight {
	int on;                   /* 0x00 */
	signed char r, g, b, a;   /* 0x04 */
	float onTime;             /* 0x08 lit until the timer reaches this */
	float period;             /* 0x0c the timer restarts here */
	float timer;              /* 0x10 */
	float unk14;              /* 0x14 1.0 */
	float pos[3];             /* 0x18 */
	float unk24;              /* 0x24 1.0 */
	int unk28;                /* 0x28 */
};
OF_ASSERT_SIZE(struct ClustLight, 0x2c);
#define CL_LIGHTS ((struct ClustLight *)CL_ADDR(0x002f7a70))
#define CL_LIGHT_COUNT 15

/* 0x000dacf0  Clusteroids (gap; CClust_Arena.cpp inferred)
Sets up the 15 lights from the position table at 0x00295e20 (x, y, z, - per light):
even lights red, odd ones orange, with staggered timers. */
void ClustArena_InitLights(void)
{
	struct ClustLight *light;
	const float *pos;
	int i;
	int even;
	int odd;

	even = 0;
	odd = 0;
	for (i = 0; i < CL_LIGHT_COUNT; i++) {
		light = &CL_LIGHTS[i];
		pos = (const float *)CL_ADDR(0x00295e20) + i * 4;
		light->on = 1;
		light->a = 0x7f;
		light->r = 0x72;
		if (i % 2 == 0) {
			light->g = 0;
			light->b = 0;
			light->onTime = 20.0f;
			light->period = 40.0f;
			light->timer = (float)even;
		} else {
			light->g = 0x6f;
			light->b = 0x32;
			light->onTime = 30.0f;
			light->period = 50.0f;
			light->timer = (float)odd;
		}
		light->pos[0] = pos[0];
		light->pos[1] = pos[1];
		light->pos[2] = pos[2];
		even += 2;
		light->unk14 = 1.0f;
		light->unk24 = 1.0f;
		odd += 3;
	}
}

/* 0x000dadb0  Clusteroids (gap; CClust_Arena.cpp inferred)
Flashes the lights: alpha 0x7f until onTime, 0 until period, then the timer restarts. */
void ClustArena_UpdateLights(void)
{
	struct ClustLight *light;
	float timer;

	for (light = CL_LIGHTS; light < CL_LIGHTS + CL_LIGHT_COUNT; light++) {
		if (light->on != 1)
			continue;
		timer = g_ClFrameTime + light->timer;
		light->timer = timer;
		if (light->onTime <= timer) {
			if (light->period <= timer)
				light->timer = 0.0f;
			else
				light->a = 0;
		} else {
			light->a = 0x7f;
		}
	}
}

/* 0x000dae10  Clusteroids (gap; CClust_Arena.cpp inferred)
Draws the lights as additive 3D sprites (vertices at 0x002f9b40, 0x70 bytes each). */
void ClustArena_DrawLights(void)
{
	struct ClustLight *light;
	char *vertices;
	int count;

	if (CL_VAR(void *, 0x002f8f4c) != NULL)
		bdSetTexture(0, CL_VAR(struct bdTexture *, 0x002f8f4c));
	bdSetWorldMatrix(DAT_002855f0);
	vertices = (char *)CL_ADDR(0x002f9b40);
	for (light = CL_LIGHTS; light < CL_LIGHTS + CL_LIGHT_COUNT; light++) {
		if (bdDrawSprite3D(light->pos, 128.0f, 128.0f, 0.0f, CL_VAR(struct bdTexture *, 0x002f8f4c),
				0, 0, light->r, light->g, light->b, light->a, (float *)vertices, 0.0f) != 0)
			vertices += 0x70;
	}
	count = (int)(vertices - (char *)CL_ADDR(0x002f9b40)) / 0x70;
	if (count == 0)
		return;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(8, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(7, 1, 0);
	bdSetProjectionMode(1);
	vertices = (char *)CL_ADDR(0x002f9b40);
	for (; count > 0; count--) {
		bdDrawPrimitiveFVF(4, vertices, 4, 0x144);
		vertices += 0x70;
	}
	bdSetRenderState(7, 3, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(4, 1, 0);
	bdSetProjectionMode(1);
}

/* 0x000daf60  Clusteroids (gap; CClust_Arena.cpp inferred)
A ring of sparks at `pos` (an explosion). */
void ClustArena_SparkRing(float *pos)
{
	Debris_AddRing((struct DebrisType *)CL_ADDR(0x00296c20), 7, pos, 1.6f, 1.6f, 1.2f, 0, 0xffffffff);
	Debris_AddRing((struct DebrisType *)CL_ADDR(0x00296c4c), 7, pos, 0.040000003f, 3.2f, 2.0f, 0, 0xffffffff);
	Debris_AddRing((struct DebrisType *)CL_ADDR(0x00296c20), 2, pos, 3.2f, 1.8000001f, 0.8f, 0, 0xffffffff);
	Debris_AddRing((struct DebrisType *)CL_ADDR(0x00296c4c), 2, pos, 2.4f, 1.6f, 1.2f, 0, 0xffffffff);
}

/* a grid trail walker: runs along the floor grid (86 units) from cell to cell inside a
   circle, leaving a trail (0x38 bytes, 2 from 0x002f7dc8) */
struct ClustWalker {
	float pos[4];             /* 0x00 */
	float target[4];          /* 0x10 the next grid point */
	float unk20[4];           /* 0x20 */
	int dir;                  /* 0x30 0 +z, 1 +x, 2 -z, 3 -x */
	int lastDir;              /* 0x34 */
};
OF_ASSERT_SIZE(struct ClustWalker, 0x38);
#define CL_WALKERS ((struct ClustWalker *)CL_ADDR(0x002f7dc8))

/* 0x000daff0  Clusteroids (gap; CClust_Arena.cpp inferred)
Picks the walker's next grid point: a random direction that does not turn back and stays
within the arena radius (0x002f7f68). */
void ClustWalker_NextTarget(float *self)
{
	struct ClustWalker *walker = (struct ClustWalker *)self;
	float dx;
	float dz;
	int dir;

	walker->lastDir = walker->dir;
	for (;;) {
		dir = cl_rand() % 4;
		dz = 0.0f;
		dx = 0.0f;
		switch (dir) {
		case 0:
			dz = 1.0f;
			if (walker->lastDir == 2)
				continue;
			break;
		case 1:
			dx = 1.0f;
			if (walker->lastDir == 3)
				continue;
			break;
		case 2:
			dz = -1.0f;
			if (walker->lastDir == 0)
				continue;
			break;
		case 3:
			dx = -1.0f;
			if (walker->lastDir == 1)
				continue;
			break;
		}
		walker->target[1] = 0.0f;
		walker->target[3] = 0.0f;
		walker->target[0] = dx * 86.0f + walker->pos[0];
		walker->target[2] = dz * 86.0f + walker->pos[2];
		if (fabsf(sqrtf(walker->target[0] * walker->target[0] +
				walker->target[2] * walker->target[2])) <= CL_VAR(float, 0x002f7f68)) {
			walker->dir = dir;
			return;
		}
	}
}

/* 0x000db0f0  Clusteroids (gap; CClust_Arena.cpp inferred)
Puts both walkers on a random grid point within 300 units of the centre and gives them
a first target. */
void ClustWalkers_Init(void)
{
	struct ClustWalker *walker;
	float angle;
	float radius;
	float dx;
	float dz;
	int x;
	int z;
	int dir;

	for (walker = CL_WALKERS; walker < CL_WALKERS + 2; walker++) {
		angle = (float)cl_rand() * 1.9175345e-4f;
		radius = (float)cl_rand() * 0.00915555283f;
		x = CL_FTOL(sinf(angle) * radius);
		z = CL_FTOL(cosf(angle) * radius);
		x = x - x % 0x56;
		z = z - z % 0x56;
		do {
			dir = cl_rand() % 4;
			dx = 0.0f;
			dz = 0.0f;
			switch (dir) {
			case 0:
				dz = 1.0f;
				break;
			case 1:
				dx = 1.0f;
				break;
			case 2:
				dz = -1.0f;
				break;
			case 3:
				dx = -1.0f;
				break;
			}
			walker->pos[1] = 0.0f;
			walker->pos[0] = (float)x;
			walker->pos[3] = 0.0f;
			walker->pos[2] = (float)z;
			walker->target[1] = 0.0f;
			walker->target[3] = 0.0f;
			walker->target[0] = dx * 86.0f + (float)x;
			walker->target[2] = dz * 86.0f + (float)z;
		} while (fabsf(sqrtf(walker->target[0] * walker->target[0] +
			walker->target[2] * walker->target[2])) > 300.0f);
		walker->dir = dir;
		walker->lastDir = -1;
	}
}

/* 0x000db2a0  Clusteroids (gap; CClust_Arena.cpp inferred)
Moves the walkers 5 units towards their target (a new target when reached) and drops a
"GRIDTRAIL" debris piece below each. */
void ClustWalkers_Update(void)
{
	struct ClustWalker *walker;
	float dir[3];
	float vel[4];

	for (walker = CL_WALKERS; walker < CL_WALKERS + 2; walker++) {
		walker->unk20[0] = walker->pos[0];
		walker->unk20[1] = walker->pos[1];
		walker->unk20[2] = walker->pos[2];
		walker->unk20[3] = walker->pos[3];
		dir[0] = walker->target[0] - walker->pos[0];
		dir[1] = walker->target[1] - walker->pos[1];
		dir[2] = walker->target[2] - walker->pos[2];
		CLVec3_Normalize(dir);
		dir[0] = dir[0] * 5.0f;
		dir[1] = dir[1] * 5.0f;
		dir[2] = dir[2] * 5.0f;
		walker->pos[0] = dir[0] + walker->pos[0];
		walker->pos[1] = dir[1] + walker->pos[1];
		walker->pos[2] = dir[2] + walker->pos[2];
		if (FUN_000e5440(walker->target, walker->pos) <= 5.0f) {
			walker->pos[0] = walker->target[0];
			walker->pos[1] = walker->target[1];
			walker->pos[2] = walker->target[2];
			walker->pos[3] = walker->target[3];
			ClustWalker_NextTarget(walker->pos);
		}
		vel[3] = 0.0f;
		vel[2] = 0.0f;
		vel[1] = 0.0f;
		vel[0] = 0.0f;
		walker->pos[1] = -80.0f;
		Debris_Add((struct DebrisType *)CL_ADDR(0x00296e30), walker->pos, vel, 0, 0x4000);
		walker->pos[1] = 0.0f;
	}
}

/* the per-player start values in ClustGame_Init / ClustGame_Reset (0x002f7f40: x, z of
   each player) */
static void cl_reset_players(void)
{
	char *pl;
	float x;
	float z;
	int i;

	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		x = CL_VAR(float, 0x002f7f40 + i * 8);
		CL_I16(pl, 0x2) = 0;
		CL_F32(pl, 0x18) = x;
		CL_U32(pl, 0x8) = 0;
		CL_F32(pl, 0x30) = x;
		CL_I16(pl, 0x4) = 0;
		CL_F32(pl, 0x38) = x;
		CL_I16(pl, 0xc) = 0;
		z = CL_VAR(float, 0x002f7f44 + i * 8);
		CL_I16(pl, 0xe) = 0;
		CL_F32(pl, 0x1c) = z;
		CL_I16(pl, 0x10) = 0;
		CL_F32(pl, 0x34) = z;
		CL_U32(pl, 0x5c) = 0;
		CL_F32(pl, 0x3c) = z;
		CL_U32(pl, 0x60) = 0;
		CL_I16(pl, 0x650) = 0;
		CL_U32(pl, 0x674) = 0;
		CL_U32(pl, 0x658) = 0;
		CL_U32(pl, 0x648) = 0;
		CL_F32(pl, 0x64c) = 5.0f;
		CL_I16(pl, 0x84c) = 0;
		CL_U32(pl, 0x664) = 0;
		CL_I16(pl, 0x668) = 0;
		CL_I16(pl, 0x66a) = 0;
		CL_I16(pl, 0x0) = 0;
	}
}

/* 0x000db3d0  Clusteroids (gap; CClust_Arena.cpp inferred)
Sets up a game: camera and arena constants, start positions, the variant's rules, the
tweakable values (by difficulty 0x002bae70), the arena models, the four players, spawn
points, pods, tokens, the light, the debug input, the walkers, the lights and timers. */
void ClustGame_Init(void)
{
	int i;
	struct biMap *map;
	struct biChannel *channel;

	bdSetNearFar(40.0f, 30000.0f);
	bdSetRenderState(8, 0, 0);
	CL_VAR(float, 0x00296ee0) = 900.0f;
	CL_VAR(int, 0x002fa290) = 0;
	CL_VAR(short, 0x002fa294) = 0;
	CL_VAR(float, 0x00295e08) = 0.0f;
	CL_VAR(float, 0x00295e0c) = 700.0f;
	CL_VAR(float, 0x00295e10) = -700.0f;
	CL_VAR(float, 0x002f9b24) = 0.0f;
	CL_VAR(float, 0x002f9b28) = 0.0f;
	CL_VAR(float, 0x002f9b2c) = 0.0f;
	CL_VAR(float, 0x00295e14) = 0.0f;
	CL_VAR(float, 0x00295e18) = 700.0f;
	CL_VAR(float, 0x00295e1c) = -700.0f;
	CL_VAR(float, 0x002f9b30) = 0.0f;
	CL_VAR(float, 0x002f9b34) = 0.0f;
	CL_VAR(float, 0x002f9b38) = 0.0f;
	CL_VAR(float, 0x002f7f60) = 840.0f;
	CL_VAR(float, 0x002f7f64) = 840.0f;
	CL_VAR(float, 0x002f7f68) = 600.0f;
	CL_VAR(short, 0x002f7f70) = 0;
	CL_VAR(float, 0x002f8730) = 0.0f;
	CL_VAR(short, 0x002f7f7c) = 0;
	CL_VAR(int, 0x002f9abc) = 0;
	CL_VAR(int, 0x002f9adc) = 0;
	CL_VAR(int, 0x002f7f90) = 6;
	CL_VAR(float, 0x002f8790) = 0.0f;
	CL_VAR(float, 0x002f8f54) = 1.0f;
	CL_VAR(float, 0x002f8f58) = 1.0f;
	CL_VAR(int, 0x002f9880) = 0;
	CL_VAR(int, 0x002f9884) = 0;
	CL_VAR(int, 0x002f9888) = 0;
	CL_VAR(float, 0x002f7f40) = 0.0f;
	CL_VAR(float, 0x002f7f44) = 150.0f;
	CL_VAR(float, 0x002f7f48) = 150.0f;
	CL_VAR(float, 0x002f7f4c) = 0.0f;
	CL_VAR(float, 0x002f7f58) = 0.0f;
	CL_VAR(float, 0x002f7f5c) = -150.0f;
	CL_VAR(float, 0x002f7f50) = -150.0f;
	CL_VAR(float, 0x002f7f54) = 0.0f;
	cl_reset_players();

	if (g_ClVariant == 1) {
		CL_VAR(short, 0x002f7f6c) = 0;
		CL_VAR(short, 0x002f7f8e) = 0;
		CL_VAR(short, 0x002f8f5c) = 0;
		CL_VAR(short, 0x002f7f8c) = 0;
		CL_VAR(short, 0x002f7d60) = 2;
	} else if (g_ClVariant == 2 || g_ClVariant == 3) {
		CL_VAR(short, 0x002f7f6c) = g_ClVariant == 3 ? 1 : g_ClVariant;
		CL_VAR(short, 0x002f7f8e) = 0;
		CL_VAR(short, 0x002f8f5c) = 0;
		CL_VAR(short, 0x002f7d60) = 0;
		CL_VAR(short, 0x002f7f8c) = 0;
	} else if (g_ClVariant == 4) {
		CL_VAR(short, 0x002f7f6c) = 5;
		CL_VAR(short, 0x002f7f8e) = 5;
		CL_VAR(short, 0x002f8f5c) = 0;
		CL_VAR(short, 0x002f7d60) = 0;
		CL_VAR(short, 0x002f7f8c) = 0;
		for (i = 0; i < 5; i++) {
			CL_VAR(float, 0x002f86e0 + i * 0x10) = 0.0f;
			CL_VAR(float, 0x002f86e4 + i * 0x10) = -40.0f;
			CL_VAR(float, 0x002f86e8 + i * 0x10) = 0.0f;
			CL_VAR(int, 0x002f8734) = 0;
			CL_VAR(int, 0x002f8738) = 0;
			CL_VAR(int, 0x002f873c) = 0;
		}
	}

	CL_VAR(float, 0x002f7d54) = 0.0f;
	CL_VAR(float, 0x002f7d50) = 0.0f;
	CL_VAR(float, 0x002f7d58) = 2.0f;
	CL_VAR(float, 0x002f7d44) = 0.0f;
	CL_VAR(float, 0x002f7d48) = 2.0f;
	CL_VAR(float, 0x002f7d40) = 0.1f;
	CL_VAR(float, 0x002f7d0c) = 1.6f;         /* playerAccel */
	CL_VAR(float, 0x002f7d08) = 0.09f;        /* playerDrag */
	CL_VAR(float, 0x002f7d10) = 1.0f;
	CL_VAR(float, 0x002f7d14) = 0.10471976f;     /* playerTurnRate */
	CL_VAR(float, 0x002f7d18) = 35.0f;        /* playerFireDelay */
	CL_VAR(float, 0x002f7d1c) = 200.0f;       /* playerBoostDelay */
	CL_VAR(float, 0x002f7d64) = 525.0f;       /* tailsPodAttachTime */
	CL_VAR(int, 0x002f7d68) = 1;              /* lastPodPermanent */
	if (DAT_002baebc == 0) {
		CL_VAR(float, 0x002f7d6c) = 50.0f;    /* collectorTargetScore */
		CL_VAR(float, 0x002f7d70) = 50.0f;
	} else {
		CL_VAR(float, 0x002f7d6c) = 100.0f;
		CL_VAR(float, 0x002f7d70) = 100.0f;
	}
	CL_VAR(float, 0x002f7d74) = 0.75f;
	CL_VAR(float, 0x002f7d78) = 0.75f;        /* aiMaxSpeed */
	CL_VAR(float, 0x002f7d7c) = 0.5f;         /* aiAccuracy */
	CL_VAR(float, 0x002f7d84) = 0.95f;        /* aiFireRate */
	CL_VAR(float, 0x002f7d88) = 0.95f;        /* aiBoostRate */
	CL_VAR(int, 0x002f7da0) = 0;
	CL_VAR(int, 0x002f7da4) = 0;
	CL_VAR(int, 0x002f7da8) = 0;
	CL_VAR(float, 0x002f7dac) = 10.0f;        /* cameraLookAhead */
	CL_VAR(float, 0x002f7db0) = 25.0f;        /* bulletSpeed */
	CL_VAR(int, 0x002f7db4) = 0;
	CL_VAR(int, 0x002f7db8) = 0;
	CL_VAR(int, 0x002f7dbc) = 1;
	switch (g_ClVariant) {
	case 1:
		if (DAT_002bae70 == 0) {
			CL_VAR(float, 0x002f7d78) = 0.5f;
			CL_VAR(float, 0x002f7d7c) = 1.0f;
			CL_VAR(float, 0x002f7d84) = 0.5f;
			CL_VAR(float, 0x002f7d88) = 0.75f;
		} else if (DAT_002bae70 == 1) {
			CL_VAR(float, 0x002f7d78) = 0.75f;
			CL_VAR(float, 0x002f7d7c) = 0.5f;
			CL_VAR(float, 0x002f7d84) = 0.65f;
			CL_VAR(float, 0x002f7d88) = 0.95f;
		} else if (DAT_002bae70 == 2) {
			CL_VAR(float, 0x002f7d78) = 1.0f;
			CL_VAR(float, 0x002f7d7c) = 0.15f;
			CL_VAR(float, 0x002f7d84) = 0.65f;
			CL_VAR(float, 0x002f7d88) = 0.99f;
		}
		break;
	case 2:
	case 3:
		if (DAT_002bae70 == 0) {
			CL_VAR(float, 0x002f7d78) = 0.5f;
			CL_VAR(float, 0x002f7d7c) = 2.0f;
			CL_VAR(float, 0x002f7d84) = 0.75f;
		} else {
			if (DAT_002bae70 == 1) {
				CL_VAR(float, 0x002f7d78) = 0.75f;
				CL_VAR(float, 0x002f7d7c) = 0.5f;
			} else {
				if (DAT_002bae70 != 2)
					break;
				CL_VAR(float, 0x002f7d78) = 0.8f;
				CL_VAR(float, 0x002f7d7c) = 0.35f;
			}
			CL_VAR(float, 0x002f7d84) = 0.65f;
		}
		CL_VAR(float, 0x002f7d88) = 0.75f;
		break;
	case 4:
		if (DAT_002bae70 == 0) {
			CL_VAR(float, 0x002f7d78) = 0.5f;
			CL_VAR(float, 0x002f7d7c) = 1.0f;
			CL_VAR(float, 0x002f7d84) = 0.5f;
			CL_VAR(float, 0x002f7d88) = 0.75f;
			CL_VAR(float, 0x002f7d90) = 0.25f;
		} else if (DAT_002bae70 == 1) {
			CL_VAR(float, 0x002f7d78) = 0.75f;
			CL_VAR(float, 0x002f7d7c) = 0.5f;
			CL_VAR(float, 0x002f7d84) = 0.65f;
			CL_VAR(float, 0x002f7d88) = 0.95f;
			CL_VAR(float, 0x002f7d90) = 0.5f;
		} else if (DAT_002bae70 == 2) {
			CL_VAR(float, 0x002f7d78) = 0.775f;
			CL_VAR(float, 0x002f7d7c) = 0.45f;
			CL_VAR(float, 0x002f7d84) = 0.9f;
			CL_VAR(float, 0x002f7d88) = 0.99f;
			CL_VAR(float, 0x002f7d90) = 0.5f;
		}
		break;
	}
	CL_VAR(float, 0x002f7d8c) = 15.0f;        /* aiShootAheadRate */
	CL_VAR(float, 0x002f7d80) = 0.15471976f; /* aiTurnRate (0x00269574) */

	ClustArena_LoadModels();
	for (i = 0; i < 4; i++)
		FUN_000dee00(i);
	ClustArena_InitSpawnPoints();
	FUN_000e4b50();
	FUN_000e6d50();
	FUN_000e2d10();

	CL_VAR(struct BLight *, 0x002f9a74) = baCreateLight(NULL, 0);
	if (CL_VAR(struct BLight *, 0x002f9a74) == NULL)
		bkPrintf("Failed to create light1\n");
	CL_VAR(float, 0x002f9a78) = 0.0f;
	CL_VAR(float, 0x002f9a7c) = 1.0f;
	CL_VAR(float, 0x002f9a80) = 0.0f;
	CL_VAR(float, 0x002f9a84) = 1.0f;
	baSetDirectionalLight(CL_VAR(struct BLight *, 0x002f9a74), CL_FP(CL_ADDR(0x002f9a78), 0));
	baSetLightColour(CL_VAR(struct BLight *, 0x002f9a74), 0x7f, 0x7f, 0x7f);
	baEnableLight(CL_VAR(struct BLight *, 0x002f9a74), 1);
	baSetAmbientLight(0x7f, 0x7f, 0x7f);

	CL_VAR(int, 0x002f9ad8) = 0;
	CL_VAR(int, 0x002f9ae0) = 0xff;
	CL_VAR(int, 0x002f8db0) = 0;
	CL_VAR(int, 0x002f8db4) = 0;
	CL_VAR(short, 0x002f8db8) = 0;
	CL_VAR(int, 0x002f7dc0) = 0;
	CL_VAR(int, 0x002f7dc4) = 0;
	CL_VAR(short, 0x002fa23c) = 0;
	CL_VAR(short, 0x002fa23e) = 0;
	CL_VAR(short, 0x002fa240) = 0;
	CL_VAR(short, 0x002fa242) = 0;
	CL_VAR(short, 0x002fa236) = 1;
	CL_VAR(short, 0x002fa238) = 0;
	CL_VAR(short, 0x002fa23a) = 0;

	/* debug buttons 1-4 (the camera follows player 1-4 while one is held) */
	map = (struct biMap *)biCreateMap(4);
	CL_VAR(struct biMap *, 0x002fa260) = map;
	channel = biCreateChannel(map, "debug but 1", 1, 0, 0, 0, 0);
	CL_VAR(struct biChannel *, 0x002fa264) = channel;
	biBindInput(channel, 0xe, 0);
	biBindInput(channel, 0x46000037, 0x4600009a);
	channel = biCreateChannel(map, "debug but 2", 1, 0, 0, 0, 0);
	CL_VAR(struct biChannel *, 0x002fa268) = channel;
	biBindInput(channel, 0xf, 0);
	biBindInput(channel, 0x46000038, 0x4600009a);
	channel = biCreateChannel(map, "debug but 3", 1, 0, 0, 0, 0);
	CL_VAR(struct biChannel *, 0x002fa26c) = channel;
	biBindInput(channel, 0x10, 0);
	biBindInput(channel, 0x46000039, 0x4600009a);
	channel = biCreateChannel(map, "debug but 4", 1, 0, 0, 0, 0);
	CL_VAR(struct biChannel *, 0x002fa270) = channel;
	biBindInput(channel, 0x11, 0);
	biBindInput(channel, 0x4600003a, 0x4600009a);

	CL_VAR(float, 0x002f7d94) = 1000.0f;      /* trailX */
	CL_VAR(float, 0x002f7d98) = 5000.0f;      /* trailY */
	CL_VAR(float, 0x002f7d9c) = 1.0f;         /* trailZ */
	ClustWalkers_Init();

	/* six points: x, y, z, - */
	CL_VAR(int, 0x002f9a0c) = 1;
	CL_VAR(float, 0x002f9a10) = -250.0f;
	CL_VAR(float, 0x002f9a14) = 820.0f;
	CL_VAR(float, 0x002f9a18) = -20.0f;
	CL_VAR(float, 0x002f9a1c) = 0.0f;
	CL_VAR(float, 0x002f9a20) = 250.0f;
	CL_VAR(float, 0x002f9a24) = 820.0f;
	CL_VAR(float, 0x002f9a28) = -20.0f;
	CL_VAR(float, 0x002f9a2c) = 0.0f;
	CL_VAR(float, 0x002f9a30) = -420.0f;
	CL_VAR(float, 0x002f9a34) = 265.0f;
	CL_VAR(float, 0x002f9a38) = -630.0f;
	CL_VAR(float, 0x002f9a3c) = 0.0f;
	CL_VAR(float, 0x002f9a40) = 420.0f;
	CL_VAR(float, 0x002f9a44) = 265.0f;
	CL_VAR(float, 0x002f9a48) = -630.0f;
	CL_VAR(float, 0x002f9a4c) = 0.0f;
	CL_VAR(float, 0x002f9a50) = -420.0f;
	CL_VAR(float, 0x002f9a54) = 265.0f;
	CL_VAR(float, 0x002f9a58) = 595.0f;
	CL_VAR(float, 0x002f9a5c) = 0.0f;
	CL_VAR(float, 0x002f9a60) = 420.0f;
	CL_VAR(float, 0x002f9a64) = 265.0f;
	CL_VAR(float, 0x002f9a68) = 595.0f;
	CL_VAR(float, 0x002f9a6c) = 0.0f;
	for (i = 0; i < 4; i++) {
		CL_VAR(int, 0x002f9ae4 + i * 4) = 0;
		CL_VAR(int, 0x002f9af4 + i * 4) = 0;
	}
	ClustArena_InitLights();
	ClustCountdown_Init();
	ClustCountdown_SetDuration(60.0f);
	ClustCountdown_Start();
}

/* 0x000dbd60  Clusteroids (gap; CClust_Arena.cpp inferred)
Resets a game for another round (the models stay loaded). */
void ClustGame_Reset(void)
{
	int i;

	CL_VAR(float, 0x00295e08) = 0.0f;
	CL_VAR(float, 0x00295e0c) = 700.0f;
	CL_VAR(float, 0x00295e10) = -700.0f;
	CL_VAR(float, 0x002f9b24) = 0.0f;
	CL_VAR(float, 0x002f9b28) = 0.0f;
	CL_VAR(float, 0x002f9b2c) = 0.0f;
	CL_VAR(float, 0x00295e14) = 0.0f;
	CL_VAR(float, 0x00295e18) = 700.0f;
	CL_VAR(float, 0x00295e1c) = -700.0f;
	CL_VAR(float, 0x002f9b30) = 0.0f;
	CL_VAR(float, 0x002f9b34) = 0.0f;
	CL_VAR(float, 0x002f9b38) = 0.0f;
	CL_VAR(float, 0x002f8730) = 0.0f;
	CL_VAR(int, 0x002f9ad8) = 0;
	CL_VAR(int, 0x002f9adc) = 0;
	CL_VAR(int, 0x002f7f90) = 6;
	CL_VAR(float, 0x002f8f54) = 1.0f;
	CL_VAR(float, 0x002f8f58) = 1.0f;
	CL_VAR(int, 0x002f8db0) = 0;
	CL_VAR(int, 0x002f8db4) = 0;
	CL_VAR(short, 0x002f8db8) = 0;
	if (g_ClVariant == 1) {
		CL_VAR(short, 0x002f7f6c) = 0;
		CL_VAR(short, 0x002f7d60) = 2;
	} else if (g_ClVariant == 3 || g_ClVariant == 2) {
		CL_VAR(short, 0x002f7f6c) = g_ClVariant == 3 ? 0 : 3;
		CL_VAR(short, 0x002f7d60) = 0;
	} else {
		goto keep_rules;
	}
	CL_VAR(short, 0x002f7f8c) = 0;
	CL_VAR(short, 0x002f7f8e) = 0;
	CL_VAR(short, 0x002f8f5c) = 0;
keep_rules:
	ClustTrails_Reset();
	FUN_000e6d50();
	CL_VAR(short, 0x002f7f7c) = 0;
	CL_VAR(float, 0x002f7d54) = 0.0f;
	CL_VAR(float, 0x002f7d50) = 0.0f;
	CL_VAR(float, 0x002f7d58) = 2.0f;
	CL_VAR(float, 0x002f7d44) = 0.0f;
	CL_VAR(float, 0x002f7d48) = 2.0f;
	CL_VAR(float, 0x002f7d40) = 0.1f;
	CL_VAR(float, 0x002f7d0c) = 1.6f;
	CL_VAR(float, 0x002f7d08) = 0.04f;
	CL_VAR(float, 0x002f7d10) = 1.0f;
	CL_VAR(float, 0x002f7d14) = 0.10471976f;
	CL_VAR(float, 0x002f7f40) = 0.0f;
	CL_VAR(float, 0x002f7f44) = 150.0f;
	CL_VAR(float, 0x002f7f48) = 150.0f;
	CL_VAR(float, 0x002f7f4c) = 0.0f;
	CL_VAR(float, 0x002f7f58) = 0.0f;
	CL_VAR(float, 0x002f7f5c) = -150.0f;
	CL_VAR(float, 0x002f7f50) = -150.0f;
	CL_VAR(float, 0x002f7f54) = 0.0f;
	cl_reset_players();
	CL_F32(CL_PLAYER(0), 0x634) = 3.1415925f;
	CL_F32(CL_PLAYER(0), 0x638) = 3.1415925f;
	CL_F32(CL_PLAYER(1), 0x634) = 1.5707963f;
	CL_F32(CL_PLAYER(1), 0x638) = 1.5707963f;
	CL_F32(CL_PLAYER(2), 0x634) = 4.712389f;
	CL_F32(CL_PLAYER(2), 0x638) = 4.712389f;
	CL_F32(CL_PLAYER(3), 0x634) = 0.0f;
	CL_F32(CL_PLAYER(3), 0x638) = 0.0f;
	FUN_000e2ae0();
	FUN_000e37a0();
	CL_VAR(float, 0x002fa248) = 0.0f;
	CL_VAR(float, 0x002fa244) = 60.0f;
	CL_VAR(int, 0x002f9a04) = 0;
	CL_VAR(float, 0x002f9a08) = 0.0f;
	ClustWalkers_Init();
	for (i = 0; i < 4; i++) {
		CL_VAR(int, 0x002f9ae4 + i * 4) = 0;
		CL_VAR(int, 0x002f9af4 + i * 4) = 0;
	}
	ClustCountdown_Init();
	ClustCountdown_SetDuration(60.0f);
}
