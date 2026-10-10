/*
BumperCars/CBCar_Explosion.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Explosion.cpp

Explosions (a bomb powerup, an expired powerup): a list (CBCar_Game + 0x4be8, struct
BCarExplosions) of shock waves. Each one throws 150 sparks and 12 smoke puffs (debris types
of bumpercars_a's ClassLib/Debris.cpp), keeps smoking, and draws a growing, fading flat ring.
0x000a3b20 is tagged with CBCar_Explosion.cpp; 0x000a34b0-0x000a3bf0 are its functions.

The .data of this file (0x00292940-0x00292c44: 3 sprite definitions, their animations, the
debris stage tables, the streak and the 3 debris types) is defined here with symbols in place
of the original pointers.
*/

#include "bcb_items.h"

static const char s_ExplosionFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Explosion.cpp";

/* ---------- data (layouts of classlib_fx's CSpriteDef / CSpriteAnim and bumpercars_a's
DebrisParams / DebrisStreak / DebrisType) */

struct BCarSpriteDef {
	char name[0x40];          /* 0x00 */
	int *frames;              /* 0x40 */
	short numFrames;          /* 0x44 */
	unsigned short flags;     /* 0x46 */
};

struct BCarSpriteAnim {
	struct BCarSpriteDef *def;  /* 0x0 */
	float frameTime;            /* 0x4 */
	unsigned int flags;         /* 0x8 */
};

struct BCarDebrisParams {
	int alpha[4];
	int red[4];
	int green[4];
	int blue[4];
	float life[3];
	float lifeRandom;
	float size[4];
	float spin;
	float spinRandom;
	float maxSpeed;
	float drag;
	float bounce;
	float gravity;
};

struct BCarDebrisStreak {
	float length;
	float width0a;
	float width1a;
	float width0b;
	float width1b;
	int alpha;
};

struct BCarDebrisType {
	const char *name;                     /* 0x00 */
	struct BCarDebrisParams *params;      /* 0x04 */
	float *spinInit;                      /* 0x08 */
	struct BCarSpriteAnim *spriteAnim;    /* 0x0c */
	int actorDebris;                      /* 0x10 */
	float windResistance;                 /* 0x14 */
	void *onDeath;                        /* 0x18 */
	void *trail;                          /* 0x1c */
	float trailInterval;                  /* 0x20 */
	struct BCarDebrisStreak *streak;      /* 0x24 */
	unsigned int flags;                   /* 0x28 */
};

/* 0x00292940  the spark sprites: "debris\sprites\spark", "...sparkburst", "...aismoke" */
struct BCarSpriteDef DAT_00292940[3] = {
	{ "debris\\sprites\\spark", NULL, 0, 1 },
	{ "debris\\sprites\\sparkburst", NULL, 0, 1 },
	{ "debris\\sprites\\aismoke", NULL, 0, 2 },
};

/* 0x00292a18 */
static struct BCarSpriteAnim s_ExplosionAnims[3] = {
	{ &DAT_00292940[0], 0.0f, 0 },
	{ &DAT_00292940[1], 0.0f, 0 },
	{ &DAT_00292940[2], 1.0f, 0 },
};

/* 0x00292a40  the sparks of the first 50 */
static struct BCarDebrisParams s_SparkParams1 = {
	{ 0, 0x7f, 0x78, 0 }, { 0, 0x7f, 0x7f, 0 }, { 0x7f, 0x7f, 0x40, 0 }, { 0x7f, 0x7f, 0x40, 0 },
	{ 0.4f, 1.2f, 0.6f }, 0.3f, { 0.8f, 4.0f, 4.0f, 4.8f },
	-0.78539807f, 0.39269903f, 0.0f, 0.99f, 0.0f, 0.8f,
};

/* 0x00292ab8  the other 100 sparks */
static struct BCarDebrisParams s_SparkParams2 = {
	{ 0, 0x7f, 0x64, 0 }, { 0x7f, 0x7f, 0x40, 0 }, { 0x40, 0x40, 0x40, 0 }, { 0, 0, 0, 0 },
	{ 0.4f, 1.2f, 0.6f }, 0.3f, { 0.8f, 4.0f, 4.0f, 4.8f },
	-0.78539807f, 0.39269903f, 0.0f, 0.99f, 0.0f, 0.8f,
};

/* 0x00292b30  the smoke */
static struct BCarDebrisParams s_SmokeParams = {
	{ 0x50, 0x50, 0x1e, 0 }, { 0x50, 0x50, 0x1e, 0 }, { 0x50, 0x50, 0x1e, 0 }, { 0x50, 0x50, 0x1e, 0 },
	{ 0.0f, 0.3f, 0.2f }, 0.1f, { 1.0f, 7.0f, 15.0f, 25.0f },
	-0.78539807f, 0.39269903f, 0.0f, 0.7f, 0.0f, 0.4f,
};

/* 0x00292ba8 */
static struct BCarDebrisStreak s_SparkStreak = { 5.0f, 4.0f, 2.0f, 4.0f, 2.0f, 0x28 };

/* 0x00292bc0  EXPLOSION_DEBRIS_SPARK1, EXPLOSION_DEBRIS_SPARK2 (0x00292bec) */
struct BCarDebrisType DAT_00292bc0[2] = {
	{ "EXPLOSION_DEBRIS_SPARK1", &s_SparkParams1, NULL, &s_ExplosionAnims[0], 0, 0.0f, NULL, NULL,
	  0.0f, &s_SparkStreak, 0x2081 },
	{ "EXPLOSION_DEBRIS_SPARK2", &s_SparkParams2, NULL, &s_ExplosionAnims[1], 0, 0.0f, NULL, NULL,
	  0.0f, &s_SparkStreak, 0x2081 },
};

/* 0x00292c18  EXPLOSION_DEBRIS_SMOKE */
struct BCarDebrisType DAT_00292c18 = {
	"EXPLOSION_DEBRIS_SMOKE", &s_SmokeParams, NULL, &s_ExplosionAnims[2], 0, 0.0f, NULL, NULL,
	0.0f, NULL, 0,
};

/* ---------- code */

/* 0x000a34b0  (gap; file inferred: CBCar_Explosion.cpp)
Constructor of the explosion list: the shock wave texture and the spark sprites. */
struct BCarExplosions *BCarExplosions_Construct(struct BCarExplosions *self)
{
	self->first = NULL;
	self->texture = bLoadResource(NULL, "debris\\textures\\shockwave.bmp", 0, 0xdefa);
	Sprite_LoadDefs((struct CSpriteDef *)DAT_00292940, 3, 0);
	return self;
}

/* 0x000a34f0  (gap; file inferred: CBCar_Explosion.cpp)
Constructor of an explosion at `pos` (moved along by every spark), `radius` wide, lasting
`duration` seconds: throws 150 sparks (50 of type SPARK1) and 12 smoke puffs. */
struct BCarExplosion *BCarExplosion_Construct(struct BCarExplosion *self, float *pos,
	float radius, float duration, struct bdTexture *texture)
{
	float dir[4];
	float speed;
	int i;

	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->texture = texture;
	self->radius = radius;
	self->duration = duration;
	self->time = 0.0f;
	for (i = 0; i < 0x96; i++)
	{
		dir[0] = (float)bcb_rand() * 3.0518509e-05f - 0.5f;   /* 0x00253740 */
		dir[1] = (float)bcb_rand() * 1.5259255e-05f;          /* 0x0025389c */
		dir[3] = 1.0f;
		dir[2] = (float)bcb_rand() * 3.0518509e-05f - 0.5f;
		bmVectorNormalise(dir, dir);
		pos[0] = dir[0] + pos[0];
		pos[1] = dir[1] + pos[1];
		pos[2] = dir[2] + pos[2];
		speed = (float)bcb_rand() * 0.00091555528f;           /* 0x00265804 */
		dir[0] = dir[0] * speed;
		dir[1] = dir[1] * speed;
		dir[2] = speed * dir[2];
		bcb_DebrisAdd(&DAT_00292bc0[0x31 < i ? 1 : 0], pos, dir, 0, 0xffffffffu);
	}
	for (i = 0xc; i != 0; i--)
	{
		dir[0] = (float)bcb_rand() * 0.00061037019f;          /* 0x002537d4 */
		dir[1] = (float)bcb_rand() * 0.0012207404f;           /* 0x00266610 */
		dir[3] = 1.0f;
		dir[2] = (float)bcb_rand() * 0.00061037019f;
		bcb_DebrisAdd(&DAT_00292c18, pos, dir, 0, 0xffffffffu);
	}
	return self;
}

/* 0x000a3740  (gap; file inferred: CBCar_Explosion.cpp)
Explosion_Update: advances the time (dt in frames); while young it keeps smoking. Returns 1
when the explosion is over. */
int BCarExplosion_Update(struct BCarExplosion *self, float dt)
{
	float dir[4];

	self->time = dt * 0.016666668f + self->time;             /* 0x00262b7c */
	if ((float)(bcb_rand() % 0x78) < (1.0f - self->time / self->duration) * 30.0f) /* 0x002553a8 */
	{
		dir[0] = (float)bcb_rand() * 0.00061037019f;
		dir[1] = (float)bcb_rand() * 0.0012207404f;
		dir[3] = 1.0f;
		dir[2] = (float)bcb_rand() * 0.00061037019f;
		bcb_DebrisAdd(&DAT_00292c18, self->pos, dir, 0, 0xffffffffu);
	}
	if (self->duration <= self->time)
		return 1;
	return 0;
}

/* the shock wave quad (4 vertices of 0x24 bytes, FVF 0x152), 0x002f2190 */
struct BCarVertex {
	float pos[3];
	float normal[3];
	unsigned int colour;
	float uv[2];
};

/* 0x000a3880  (gap; file inferred: CBCar_Explosion.cpp)
Explosion_Draw: the shock wave, a textured square around the centre that grows to the radius
and fades, drawn 4 times turned by a quarter. `view` is not used. */
void BCarExplosion_Draw(struct BCarExplosion *self, int view)
{
	struct BCarVertex *v = (struct BCarVertex *)BCB_ADDR(0x002f2190);
	float m[16];
	float fraction;
	float size;
	float k;
	int alpha;

	(void)view;
	fraction = self->time / self->duration;
	size = fraction * self->radius;
	alpha = BCB_FTOL((1.0f - fraction) * 127.0f);            /* 0x00255348 */
	bdSetTexture(0, self->texture);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(3, 0, 0);
	bmMatrixTranslation(m, self->pos[0], self->pos[1], self->pos[2]);
	bdSetWorldMatrix(m);
	k = 0.0f;
	do
	{
		float a = k * 1.5707963f;                              /* 0x00253898 */
		float s;
		float c;
		unsigned int colour;

		v[0].pos[0] = 0.0f;
		v[0].pos[1] = 0.0f;
		v[0].pos[2] = 0.0f;
		v[1].pos[1] = 0.0f;
		v[3].pos[1] = 0.0f;
		v[2].pos[1] = 0.0f;
		v[0].uv[0] = 1.0f;
		v[0].uv[1] = 1.0f;
		v[1].uv[0] = 1.0f;
		v[1].uv[1] = 0.0f;
		v[2].uv[0] = 0.0f;
		v[2].uv[1] = 1.0f;
		v[3].uv[0] = 0.0f;
		v[3].uv[1] = 0.0f;
		s = sinf(a) * size;
		v[1].pos[0] = s;
		colour = DAT_002bc490[alpha] | DAT_002bce18[alpha] | DAT_002bd640[alpha] | DAT_002bc898[alpha];
		v[0].colour = colour;
		v[1].colour = colour;
		v[2].colour = colour;
		v[3].colour = colour;
		c = cosf(a) * size;
		v[1].pos[2] = c;
		v[3].pos[0] = s + c;
		v[3].pos[2] = c - s;
		v[2].pos[0] = c;
		v[2].pos[2] = -s;
		bdDrawPrimitiveFVF(4, v, 4, 0x152);
		k = k + 1.0f;
	} while (k < 4.0f);                                        /* 0x00257c40 */
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(5);
	bdPopRenderState(3);
}

/* 0x000a3aa0  (gap; file inferred: CBCar_Explosion.cpp)
Explosions_Update: updates every explosion and deletes the finished ones. */
void BCarExplosions_Update(struct BCarExplosions *self, float dt)
{
	struct BCarExplosionNode **link = &self->first;

	while (*link != NULL)
	{
		if (BCarExplosion_Update((*link)->explosion, dt) == 0)
		{
			link = &(*link)->next;
		}
		else
		{
			struct BCarExplosionNode *node = *link;

			*link = node->next;
			if (node->explosion != NULL)
				BCB_DELETE(node->explosion);
			BCB_DELETE(node);
		}
	}
}

/* 0x000a3af0  (gap; file inferred: CBCar_Explosion.cpp)
Explosions_Draw: draws every explosion. */
void BCarExplosions_Draw(struct BCarExplosions *self, int view)
{
	struct BCarExplosionNode *node;

	for (node = self->first; node != NULL; node = node->next)
		BCarExplosion_Draw(node->explosion, view);
}

/* 0x000a3b20  BumperCars/CBCar_Explosion.cpp
Explosions_Add: a new explosion at `pos`, `radius` wide, lasting a second. Returns 1, or 0
when out of memory. */
int BCarExplosions_Add(struct BCarExplosions *self, float *pos, float radius)
{
	struct BCarExplosionNode *node;
	struct BCarExplosion *explosion;
	void *mem;

	node = (struct BCarExplosionNode *)BCB_NEW(8, s_ExplosionFile, 0x157);
	if (node == NULL)
		return 0;
	mem = BCB_NEW(0x20, s_ExplosionFile, 0x15a);
	if (mem == NULL)
		explosion = NULL;
	else
		explosion = BCarExplosion_Construct((struct BCarExplosion *)mem, pos, radius, 1.0f, self->texture);
	node->explosion = explosion;
	if (explosion == NULL)
	{
		BCB_DELETE(node);
		return 0;
	}
	node->next = self->first;
	self->first = node;
	return 1;
}

/* 0x000a3bf0  (gap; file inferred: CBCar_Explosion.cpp)
Explosions_Delete: frees every explosion and the spark sprites. */
void BCarExplosions_Delete(struct BCarExplosions *self)
{
	while (self->first != NULL)
	{
		struct BCarExplosionNode *node = self->first;

		self->first = node->next;
		if (node->explosion != NULL)
			BCB_DELETE(node->explosion);
		BCB_DELETE(node);
	}
	Sprite_FreeDefs((struct CSpriteDef *)DAT_00292940, 3);
}
