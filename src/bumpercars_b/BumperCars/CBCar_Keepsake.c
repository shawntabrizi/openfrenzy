/*
BumperCars/CBCar_Keepsake.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Keepsake.cpp

The keepsakes of the keeper variant (list at 0x002f0770, struct BCarKeepsakeInfo): spinning
items that a car picks up and carries to a goal. Each has an orb of light rays (classlib_fx
CRay) that shows while the line to the goal is clear of the arena's collision mesh, takes the
colour of its carrier and is drawn with an ambient light of that colour.
0x000a24f0 is tagged with CBCar_Keepsake.cpp; the gap functions after it are keepsake
functions too (file inferred).
*/

#include "bcb_items.h"

static const char s_KeepsakeFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Keepsake.cpp";

/* 0x000a24f0  BumperCars/CBCar_Keepsake.cpp
Keepsake_LoadInformation: reads a KEEPSAKE { } block (MODEL, DISABLEMODEL, SCALE, MASS,
RESPAWN_DELAY, POSITION, RADIUS, SPIN, PL_COLOUR_TEX, DRAWOFFSET, PICKUPOFFSET) and adds the
keepsake to the list. */
void BCarKeepsake_Load(struct BCarTextReader *reader)
{
	struct BCarKeepsakeInfo *keepsake;
	char word[512];
	char buf[512];
	char *line;
	float spin;
	int status;
	int i;

	keepsake = (struct BCarKeepsakeInfo *)BCB_NEW(0x110, s_KeepsakeFile, 0x17);
	if (keepsake == NULL)
		return;
	keepsake->colour[2] = 0x7f;
	keepsake->colour[1] = 0x7f;
	keepsake->colour[0] = 0x7f;
	keepsake->actor = NULL;
	keepsake->disableModel = NULL;
	keepsake->scale = 1.0f;
	keepsake->entity = NULL;
	keepsake->next = NULL;
	keepsake->spin = 0.0f;
	keepsake->rays = NULL;
	keepsake->frame = 0;
	for (i = 0; i < 0x34; i++)
		((unsigned int *)&keepsake->params)[i] = 0;
	keepsake->colourTexture = NULL;
	buf[0] = '\0';
	line = buf;
	while (reader->cursor < reader->end)
	{
		if (_stricmp(line, "}") == 0)
			break;
		line = BCarText_ReadLine(reader, buf);
		BCarText_NextLine(reader);
		if (strncmp(line, "//", 2) == 0)
			continue;
		status = 0;
		if (_strnicmp(line, "MODEL ", 6) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, word, 0x1ff) != 0)
			{
				keepsake->actor = (struct BActor *)bLoadResource(NULL, word, 1, 0xdefa);
				status = 1;
			}
		}
		if (_strnicmp(line, "DISABLEMODEL ", 0xd) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, word, 0x1ff) != 0)
			{
				keepsake->disableModel = (struct BActor *)bLoadResource(NULL, word, 1, 0xdefa);
				status = 1;
			}
		}
		if (_strnicmp(line, "SCALE ", 6) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &keepsake->scale) == 1)
				status = 1;
		}
		if (_strnicmp(line, "MASS ", 5) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &keepsake->params.mass) == 1)
				status = 1;
		}
		if (_strnicmp(line, "RESPAWN_DELAY ", 0xe) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &keepsake->params.respawnDelay) == 1)
				status = 1;
		}
		if (_strnicmp(line, "POSITION ", 9) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f%f%f", &keepsake->params.pos[0], &keepsake->params.pos[1],
				    &keepsake->params.pos[2]) == 3)
				status = 1;
		}
		if (_strnicmp(line, "RADIUS ", 7) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &keepsake->params.radius) == 1)
				status = 1;
		}
		if (_strnicmp(line, "SPIN ", 5) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &spin) == 1)
			{
				keepsake->spin = spin * 0.017453292f;      /* 0x002640f8 */
				status = 1;
			}
		}
		if (_strnicmp(line, "PL_COLOUR_TEX ", 0xe) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, word, 0x1ff) != 0)
			{
				keepsake->colourTexture = (struct bdTexture *)bLoadResource(NULL, word, 0, 0xdefa);
				status = 1;
			}
		}
		if (_strnicmp(line, "DRAWOFFSET ", 0xb) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &keepsake->params.drawOffset) == 1)
				status = 1;
		}
		if (_strnicmp(line, "PICKUPOFFSET ", 0xd) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &keepsake->params.carriedOffset) == 1)
				status = 1;
		}
		if (_stricmp(line, "}") != 0)
		{
			sscanf(line, "%s", word);
			bcb_BadCommand(reader, s_KeepsakeFile, 0x9d, "Keepsake_LoadInformation", status, word, line);
		}
	}
	keepsake->params.angle = 0.0f;
	keepsake->next = g_KeepsakeList;
	g_KeepsakeList = keepsake;
}

/* 0x000a2950  (gap; file inferred: CBCar_Keepsake.cpp)
Keepsake_RayTest(origin, dir, point, normal): the test of a keepsake's light rays (a CRay
callback): casts the ray against the arena's collision mesh. Returns the nearest hit time
(with its point and normal), or 0 when nothing is hit. */
float BCarKeepsake_RayTest(float *origin, float *dir, float *point, float *normal)
{
	struct BRay ray;
	struct BCollHit hits[60];
	int count;
	float best;
	int i;

	if (sqrtf(dir[2] * dir[2] + dir[1] * dir[1] + dir[0] * dir[0]) == 0.0f)
		return 0.0f;
	DAT_00285644 = 0.01f;                                       /* 0x3c23d70a */
	ray.origin[0] = origin[0];
	ray.origin[1] = origin[1];
	ray.origin[2] = origin[2];
	ray.origin[3] = 1.0f;
	ray.dir[0] = dir[0];
	ray.dir[1] = dir[1];
	ray.dir[2] = dir[2];
	ray.dir[3] = 1.0f;
	ray.length = sqrtf(dir[2] * dir[2] + dir[1] * dir[1] + dir[0] * dir[0]);
	count = 0x3c;
	hits[0].flags = 0xd;
	if (bmRayMesh(&ray, g_WorldCollision, &count, hits) == 0)
		return 0.0f;
	best = hits[0].t;
	normal[0] = hits[0].normal[0];
	normal[1] = hits[0].normal[1];
	normal[2] = hits[0].normal[2];
	normal[3] = hits[0].normal[3];
	point[1] = hits[0].point[1];
	point[0] = hits[0].point[0];
	point[3] = hits[0].point[3];
	point[2] = hits[0].point[2];
	for (i = 1; i < count; i++)
	{
		if (hits[i].t < best)
		{
			best = hits[i].t;
			normal[0] = hits[i].normal[0];
			normal[1] = hits[i].normal[1];
			normal[2] = hits[i].normal[2];
			normal[3] = hits[i].normal[3];
			point[0] = hits[i].point[0];
			point[1] = hits[i].point[1];
			point[2] = hits[i].point[2];
			point[3] = hits[i].point[3];
		}
	}
	normal[3] = 1.0f;
	point[3] = 1.0f;
	return best;
}

/* adds one light ray to a keepsake's orb */
static void bcb_AddRay(void *rays, float angle, float length, float width)
{
	bcb_RayAdd(rays, angle, 0.0f, length, 1.0f, width);
}

/* 0x000a2af0  (gap; file inferred: CBCar_Keepsake.cpp)
Keepsakes_Init: makes the entity of every keepsake (type 3, can be picked up), its light
spot "keepsakeN" and its orb of 8 rays. Returns 0 when a keepsake has no model or its
entity fails. */
int BCarKeepsakes_Init(void)
{
	struct BCarKeepsakeInfo *keepsake;
	char name[256];
	int n = 1;

	bcb_RayResetList();
	for (keepsake = g_KeepsakeList; keepsake != NULL; keepsake = keepsake->next)
	{
		struct BCarEntity *e;

		if (keepsake->actor == NULL)
		{
			bkPrintf("Player_Init - Couldn't find keepsake actor\n");
			return 0;
		}
		keepsake->entity = BCarEntity_Create(keepsake->actor, keepsake->disableModel,
			keepsake->scale, &keepsake->params);
		if (keepsake->entity == NULL)
		{
			bkPrintf("Player_Init - Couldn't initialise keepsake entity\n");
			return 0;
		}
		_snprintf(name, 0xff, "keepsake%d", n);
		BCarLight_Attach(BCB_LIGHTS, name, keepsake->entity);
		BCarEntity_SetType(keepsake->entity, BCAR_ENT_KEEPSAKE);
		keepsake->entity->pickup = 1;
		e = keepsake->entity;
		keepsake->pos[0] = e->pos[0];
		keepsake->pos[1] = e->pos[1];
		keepsake->pos[2] = e->pos[2];
		keepsake->pos[3] = e->pos[3];
		keepsake->pos[1] = keepsake->entity->carriedOffset + keepsake->pos[1];
		keepsake->rays = bcb_RayCreateOrb(1, keepsake->pos, g_KeepsakeRayHit, (void *)BCarKeepsake_RayTest);
		if (keepsake->rays != NULL)
		{
			bcb_AddRay(keepsake->rays, 2.3561945f, 1000.0f, 30.0f);  /* 0x4016cbe4 */
			bcb_AddRay(keepsake->rays, 5.497787f, 1000.0f, 30.0f);   /* 0x40afede0 */
			bcb_AddRay(keepsake->rays, 0.0f, 2500.0f, 120.0f);
			bcb_AddRay(keepsake->rays, 3.1415925f, 2500.0f, 120.0f); /* 0x40490fda */
			bcb_AddRay(keepsake->rays, 0.4f, 1500.0f, 60.0f);        /* 0x3ecccccd */
			bcb_AddRay(keepsake->rays, 3.5415926f, 1500.0f, 60.0f);  /* 0x4062a974 */
			bcb_AddRay(keepsake->rays, 1.5707963f, 1050.0f, 42.0f);  /* 0x3fc90fda */
			bcb_AddRay(keepsake->rays, 4.712389f, 500.0f, 42.0f);    /* 0x4096cbe4 */
		}
		n++;
	}
	return 1;
}

/* 0x000a2d40  (gap; file inferred: CBCar_Keepsake.cpp)
Keepsakes_Update: the keepsakes spin, their orbs follow them (above the carrier) and take
the carrier's colour; then the rays update. */
void BCarKeepsakes_Update(void)
{
	struct BCarKeepsakeInfo *keepsake;

	for (keepsake = g_KeepsakeList; keepsake != NULL; keepsake = keepsake->next)
	{
		struct BCarEntity *e;
		float offset;
		int i;

		keepsake->entity->targetHeading = CLMath_WrapAngle(keepsake->entity->targetHeading + keepsake->spin);
		keepsake->entity->rot[1] = keepsake->entity->heading;
		e = keepsake->entity;
		keepsake->pos[0] = e->pos[0];
		keepsake->pos[1] = e->pos[1];
		keepsake->pos[2] = e->pos[2];
		keepsake->pos[3] = e->pos[3];
		e = keepsake->entity;
		if (e->pickup == 0)
		{
			if (e->carrier != NULL)
				keepsake->pos[1] = e->carrier->drawOffset + e->carrier->radius + keepsake->pos[1];
			offset = e->drawOffset;
		}
		else
		{
			offset = e->carriedOffset;
		}
		keepsake->frame = 0;
		keepsake->colour[2] = 0x7f;
		keepsake->colour[1] = 0x7f;
		keepsake->pos[1] = offset + keepsake->pos[1];
		keepsake->colour[0] = 0x7f;
		for (i = 0; i < BCB_NUM_PLAYERS; i++)
		{
			int character;

			if (g_BCarPlayers[i].entity != keepsake->entity->carrier)
				continue;
			character = DAT_002baed4[i * 10];
			BCB_I32(keepsake->rays, 0x4c) = (int)BCB_CHAR_COLOUR(character, 0);
			keepsake->colour[0] = BCB_I32(keepsake->rays, 0x4c);
			BCB_I32(keepsake->rays, 0x50) = (int)BCB_CHAR_COLOUR(character, 1);
			keepsake->colour[1] = BCB_I32(keepsake->rays, 0x50);
			BCB_I32(keepsake->rays, 0x54) = (int)BCB_CHAR_COLOUR(character, 2);
			keepsake->colour[2] = BCB_I32(keepsake->rays, 0x54);
			BCB_I32(keepsake->rays, 0x58) = 0x3c;
			keepsake->frame = character + 1;
		}
	}
	bcb_RayUpdateAll();
}

/* 0x000a3080  (gap; file inferred: CBCar_Keepsake.cpp)
Keepsakes_Draw(glow, glowPass): draws the live keepsakes in their carrier's colour (an
ambient light of that colour in the normal pass), with their floor reflection; in the glow
pass only the fast ones, brighter with speed. The normal pass also draws the rays. */
void BCarKeepsakes_Draw(int glow, int glowPass)
{
	struct BCarKeepsakeInfo *keepsake;
	int passes = glow != 0 ? 6 : 1;
	int ambient[3];

	bdPushRenderState(10, 1, 0);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(8, 0, 0);
	bdPushRenderState(4, 1, 0);
	if (glow == 0)
	{
		bdPushRenderState(3, 1, 0);
		ambient[1] = DAT_00284b14;
		ambient[2] = DAT_00284b18;
		ambient[0] = DAT_00284b10;
	}
	else
	{
		bdPushRenderState(3, 0, 0);
	}
	for (keepsake = g_KeepsakeList; keepsake != NULL; keepsake = keepsake->next)
	{
		struct BCarEntity *e = keepsake->entity;
		struct BActorInstance *inst;

		if (e->dead != 0 || e->respawnTime != 0.0f)
			continue;
		if (glowPass != 0 && bcb_SpeedBrightness(keepsake->entity) == 0)
			continue;
		if (keepsake->colourTexture != NULL)
			bdTextureSetFrame(keepsake->colourTexture, keepsake->frame);
		if (glow == 0)
			baSetAmbientLight(keepsake->colour[0], keepsake->colour[1], keepsake->colour[2]);
		if (BCB_DEBUG_GET(FUN_0008c040) != 0 && glowPass == 0)
		{
			BCB_CALL(FUN_001d6820, void (*)(void))();
			BCB_CALL(FUN_001d6860, void (*)(void))();
			BCarRender_BeginReflection(1);
			bcb_DrawReflected(keepsake->entity->inst, passes);
			BCarRender_EndReflection();
			BCB_CALL(FUN_0018e640, void (*)(void))();
		}
		inst = keepsake->entity->inst;
		inst->rot[0] = DAT_00285634[0];
		inst->rot[1] = DAT_00285634[1];
		inst->rot[2] = DAT_00285634[2];
		inst->rot[3] = DAT_00285634[3];
		baPrepActorInstance(keepsake->entity->inst, 1);
		baDrawActorInstance(keepsake->entity->inst, passes);
		if (glow == 0)
			baSetAmbientLight(ambient[0], ambient[1], ambient[2]);
		if (keepsake->colourTexture != NULL)
			bdTextureSetFrame(keepsake->colourTexture, 0);
	}
	if (glowPass != 0)
		baSetBrightness(0x100);
	bdPopRenderState(3);
	bdPopRenderState(8);
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(4);
	if (glow == 0)
		baSetAmbientLight(ambient[0], ambient[1], ambient[2]);
	else if (glowPass == 0 && g_BCarGameState < 4)
		bcb_RayDrawAll();
}

/* 0x000a3450  (gap; file inferred: CBCar_Keepsake.cpp)
Keepsakes_Delete: frees the keepsakes and their orbs, and every ray. */
void BCarKeepsakes_Delete(void)
{
	struct BCarKeepsakeInfo *keepsake = g_KeepsakeList;

	while (keepsake != NULL)
	{
		void *rays = keepsake->rays;
		struct BCarKeepsakeInfo *next = keepsake->next;

		if (rays != NULL)
		{
			bcb_RayDestroy(rays);
			BCB_DELETE(rays);
		}
		BCB_DELETE(keepsake);
		keepsake = next;
	}
	g_KeepsakeList = NULL;
	bcb_RayDeleteAll();
}
