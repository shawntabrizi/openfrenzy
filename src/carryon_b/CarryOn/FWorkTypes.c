/*
CarryOn/FWorkTypes.c  <-  C:\BlitzParty\Game\Src\CarryOn\FWorkTypes.cpp

The particle types of the fireworks:

- CFire_FWorkStars: the stars of a burst, point sprites that brighten to their colour,
  then fade out (pool of 0x54-byte stars).
- CFire_FWorkSmoke: the smoke of a rocket trail, camera-facing quads that fade (pool of
  0x40-byte puffs).
- CFire_RandomColour: a random bright colour (one channel 255).

0x000d77e0 and 0x000d7c90 are tagged with FWorkTypes.cpp, 0x000d7900-0x000d7b50 lie
between them; 0x000d7720 (just before) and 0x000d7da0-0x000d7f00 (after) belong to the
same classes (file inferred).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\FWorkTypes.cpp"

#define CB_RAND01 3.0518509e-05f    /* 1/32767 (0x00253740) */

/* the frame timer of the CarryOn game (its +0x18) */
#define CB_GAME_TIMER ((struct CFire_Timer *)((char *)DAT_002f6d5c + 0x18))

/* 0x000d7720  CarryOn/FWorkTypes.cpp (file inferred)
A random bright colour: one channel 255 (chosen at random), the others 0-255. */
void CFire_RandomColour(float *r, float *g, float *b)
{
	int which = cb_rand() % 3;
	float c1 = (float)(cb_rand() & 0xff);
	float c2 = (float)(cb_rand() & 0xff);

	if (which == 0)
	{
		*r = 255.0f;
		*g = c1;
		*b = c2;
		return;
	}
	*r = c1;
	if (which != 1)
	{
		*g = c2;
		*b = 255.0f;
		return;
	}
	*g = 255.0f;
	*b = c2;
}

/* 0x000d77e0  CarryOn/FWorkTypes.cpp
Makes `count` stars (all free) and their point sprites. */
int CFire_FWorkStars_Init(struct CFire_FWorkStars *self, int count, float pointSize)
{
	struct bkResource *texture;
	struct CFire_PointSprites *points;
	int i;

	texture = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
	self->count = count;
	self->stars = bkHeapAlloc(count * 0x54, CB_FILE, 0x7f, 0x2001);
	if (self->stars == NULL)
	{
		self->count = 0;
		return 0;
	}
	memset(self->stars, 0, (size_t)count * 0x54);
	self->free = self->stars;
	self->used = NULL;
	for (i = 0; i < count - 1; i++)
		self->stars[i].next = &self->stars[i + 1];
	self->stars[count - 1].next = NULL;
	points = bkHeapAlloc(0x9c, CB_FILE, 0x91, 0x1001);
	if (points != NULL)
		points = CFire_PointSprites_ctor(points);
	self->points = points;
	CFire_PointSprites_Init(points, 2, texture, count, pointSize);
	return 1;
}

/* 0x000d7900  CarryOn/FWorkTypes.cpp (bracketed)
Moves the stars (gravity), brightens them; from their fade-out time they fade to black
over the rest of their life. */
void CFire_FWorkStars_Update(struct CFire_FWorkStars *self)
{
	struct CFire_Star **link = &self->used;
	struct CFire_Star *star = self->used;
	struct CFire_Star *next;
	float life, c0, c1, c2, k;

	while (star != NULL)
	{
		life = star->life - 1.0f;
		star->life = life;
		star->vel[1] = star->gravity + star->vel[1];
		c0 = star->colour[0] + star->fade[0];
		star->colour[0] = c0;
		c1 = star->fade[1] + star->colour[1];
		star->colour[1] = c1;
		c2 = star->colour[2] + star->fade[2];
		star->colour[2] = c2;
		if (star->fadingOut == 0 && life < star->fadeOutAt)
		{
			star->fadingOut = 1;
			k = 1.0f / life;
			star->fade[0] = -(c0 * k);
			star->fade[1] = -(c1 * k);
			star->fade[2] = -(k * c2);
		}
		star->pos[0] = star->pos[0] + star->vel[0];
		star->pos[1] = star->pos[1] + star->vel[1];
		star->pos[2] = star->pos[2] + star->vel[2];
		next = star->next;
		if (life <= 0.0f)
		{
			*link = next;
			star->next = self->free;
			self->free = star;
		}
		else
		{
			link = &star->next;
		}
		star = next;
	}
}

/* 0x000d7a10  CarryOn/FWorkTypes.cpp (bracketed)
Draws the stars (when the "Fireworks" tweak is on), timed as section 3 of the frame
timer. */
void CFire_FWorkStars_Draw(struct CFire_FWorkStars *self)
{
	struct CFire_Star *star;
	float pos[4];

	if (self->used == NULL || DAT_00294114 == 0)
		return;
	bdSetWorldMatrix((float *)DAT_002855f0);
	pos[3] = 1.0f;
	/* the timer's section start (identical code folded with rowing_a 0x001af540) */
	CB_CALL(FUN_001af540, void (*)(void *, int))(CB_GAME_TIMER, 3);
	CFire_PointSprites_Begin(self->points);
	for (star = self->used; star != NULL; star = star->next)
	{
		pos[0] = star->pos[0];
		pos[1] = star->pos[1];
		pos[2] = star->pos[2];
		CFire_PointSprites_Add(self->points, pos, star->size,
			DAT_002bd640[(int)star->colour[2]] | DAT_002bc490[(int)star->colour[1]] |
			DAT_002bce18[(int)star->colour[0]] | DAT_002bc898[(int)star->alpha]);
	}
	CFire_Timer_End(CB_GAME_TIMER, 3);
	CFire_PointSprites_Flush(self->points);
}

/* 0x000d7b50  CarryOn/FWorkTypes.cpp (bracketed)
A star that brightens to (r, g, b) until fadeAt of its life (50-100 steps), then fades
out; NULL when the pool is used up. */
struct CFire_Star *CFire_FWorkStars_Add(struct CFire_FWorkStars *self, float *pos, float *vel,
	float gravity, float size, float r, float g, float b, float fadeAt)
{
	struct CFire_Star *star = self->free;
	float life;

	if (star == NULL)
		return NULL;
	self->free = star->next;
	star->next = self->used;
	self->used = star;
	star->pos[0] = pos[0];
	star->pos[1] = pos[1];
	star->pos[2] = pos[2];
	star->pos[3] = 1.0f;
	star->size = size * 0.75f + size * 0.25f * (float)cb_rand() * CB_RAND01;
	star->vel[0] = vel[0];
	star->vel[1] = vel[1];
	star->vel[2] = vel[2];
	star->gravity = gravity;
	star->unk20 = 0.0f;
	star->colour[0] = 0.0f;
	star->colour[1] = 0.0f;
	star->colour[2] = 0.0f;
	life = (float)cb_rand() * CB_RAND01 * 50.0f + 50.0f;   /* 0x002559b8 */
	star->fadingOut = 0;
	star->alpha = 127.0f;
	star->life = life;
	star->fadeOutAt = (1.0f - fadeAt) * life;
	star->fade[0] = r / (star->life - star->fadeOutAt);
	star->fade[1] = g / (star->life - star->fadeOutAt);
	star->fade[2] = b / (star->life - star->fadeOutAt);
	return star;
}

/* 0x000d7c90  CarryOn/FWorkTypes.cpp
Makes `count` smoke puffs (all free) and their quads. */
int CFire_FWorkSmoke_Init(struct CFire_FWorkSmoke *self, int count)
{
	struct bkResource *texture;
	struct CFire_QuadSprites *quads;
	int i;

	texture = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
	self->count = count;
	self->puffs = bkHeapAlloc(count * 0x40, CB_FILE, 0x16a, 0x2001);
	if (self->puffs == NULL)
	{
		self->count = 0;
		return 0;
	}
	memset(self->puffs, 0, (size_t)count * 0x40);
	self->free = self->puffs;
	self->used = NULL;
	for (i = 0; i < count - 1; i++)
		self->puffs[i].next = &self->puffs[i + 1];
	self->puffs[count - 1].next = NULL;
	quads = bkHeapAlloc(200, CB_FILE, 0x17c, 0x1001);
	if (quads != NULL)
		quads = CFire_QuadSprites_ctor(quads);
	self->quads = quads;
	CFire_QuadSprites_Init(quads, 1, texture, count);
	return 1;
}

/* 0x000d7da0  CarryOn/FWorkTypes.cpp (file inferred)
Moves and fades the puffs (alpha 0-127); they go when they are faded or their life is
over. */
void CFire_FWorkSmoke_Update(struct CFire_FWorkSmoke *self)
{
	struct CFire_SmokePuff **link = &self->used;
	struct CFire_SmokePuff *puff = self->used;
	struct CFire_SmokePuff *next;
	float life, alpha;
	int gone;

	while (puff != NULL)
	{
		life = puff->life - 1.0f;
		puff->life = life;
		alpha = puff->fade + puff->alpha;
		puff->alpha = alpha;
		gone = alpha < 0.0f;
		if (gone)
			puff->alpha = 0.0f;
		if (127.0f < puff->alpha)                       /* 0x00255348 */
			puff->alpha = 127.0f;
		puff->pos[0] = puff->pos[0] + puff->vel[0];
		puff->pos[1] = puff->pos[1] + puff->vel[1];
		puff->pos[2] = puff->pos[2] + puff->vel[2];
		next = puff->next;
		if (gone || life <= 0.0f)
		{
			*link = next;
			puff->next = self->free;
			self->free = puff;
		}
		else
		{
			link = &puff->next;
		}
		puff = next;
	}
}

/* 0x000d7e50  CarryOn/FWorkTypes.cpp (file inferred)
Draws the puffs (when the "Fireworks" tweak is on). */
void CFire_FWorkSmoke_Draw(struct CFire_FWorkSmoke *self)
{
	struct CFire_SmokePuff *puff;

	if (self->used == NULL || DAT_00294114 == 0)
		return;
	bdSetWorldMatrix((float *)DAT_002855f0);
	CFire_QuadSprites_Begin(self->quads);
	for (puff = self->used; puff != NULL; puff = puff->next)
		CFire_QuadSprites_Add(self->quads, puff->pos, puff->size,
			DAT_002bd640[(int)puff->grey[2]] | DAT_002bc490[(int)puff->grey[1]] |
			DAT_002bce18[(int)puff->grey[0]] | DAT_002bc898[(int)puff->alpha]);
	CFire_QuadSprites_Flush(self->quads);
}

/* 0x000d7f00  CarryOn/FWorkTypes.cpp (file inferred)
A grey puff at pos drifting at up to 1% of `speed` (plus vy upwards), fading over 20-40
steps. */
void CFire_FWorkSmoke_Add(struct CFire_FWorkSmoke *self, float *pos, float vy, float speed)
{
	struct CFire_SmokePuff *puff = self->free;
	float spread;
	float half;
	float grey;

	if (puff == NULL)
		return;
	self->free = puff->next;
	puff->next = self->used;
	self->used = puff;
	puff->pos[0] = pos[0];
	puff->pos[1] = pos[1];
	puff->pos[2] = pos[2];
	puff->pos[3] = 1.0f;
	puff->size = speed * 0.75f + speed * 0.25f * (float)cb_rand() * CB_RAND01;
	spread = speed * 0.01f;                             /* 0x0025372c */
	half = speed * 0.005f;                              /* 0x002632d8 */
	puff->vel[0] = (float)cb_rand() * CB_RAND01 * spread - half;
	puff->vel[1] = ((float)cb_rand() * CB_RAND01 * spread - half) + vy;
	puff->unk20 = 0.0f;
	puff->vel[2] = (float)cb_rand() * CB_RAND01 * spread - half;
	grey = (float)cb_rand() * CB_RAND01 * 64.0f + 32.0f;   /* 0x00253f38, 0x002632d4 */
	puff->grey[0] = grey;
	puff->grey[1] = grey;
	puff->grey[2] = grey;
	puff->alpha = 64.0f;
	puff->life = (float)cb_rand() * CB_RAND01 * 20.0f + 20.0f;
	puff->fade = -64.0f / puff->life;                   /* 0x00268cac */
}
