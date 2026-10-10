/*
CarryOn/CFire_Romans.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Romans.cpp

Roman candles: glowing balls that rise with a trail of sparks, burn (dropping glitter
while the game runs) and fall back, each with a spot light that follows it down to the
ground. Drawn as camera-facing quads.

0x000d8760 is tagged with CFire_Romans.cpp; the other functions work on the same object
(file inferred). 0x000d9360 frees any pool of this layout (CFire_FWorkStars and
CFire_FWorkSmoke too).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_Romans.cpp"

/* 0x00294bd0  the spot cone of a rising candle's light (45 degrees) */
float DAT_00294bd0 = 0.785398126f;
/* 0x00294bd4  the narrowest cone while it falls (40 degrees) */
float DAT_00294bd4 = 0.69813168f;

/* 0x000d8760  CarryOn/CFire_Romans.cpp
Makes `count` candles (all free; all but the last with a spot light, off) and their quads. */
int CFire_Romans_Init(struct CFire_Romans *self, int count)
{
	struct bkResource *texture;
	struct CFire_QuadSprites *quads;
	int i;

	texture = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
	self->count = count;
	self->romans = bkHeapAlloc(count * 0x88, CB_FILE, 0x2c, 0x2001);
	if (self->romans == NULL)
	{
		self->count = 0;
		return 0;
	}
	memset(self->romans, 0, (size_t)count * 0x88);
	self->free = self->romans;
	self->used = NULL;
	for (i = 0; i < count - 1; i++)
	{
		self->romans[i].next = &self->romans[i + 1];
		self->romans[i].light = CFire_Lights_AddSpotDefault(DAT_002f7988, 1, DAT_00294bd0, 0x7f,
			0x7f, 0x7f, 0.0005f, 0);
		CFire_Lights_SetFlags(DAT_002f7988, self->romans[i].light, 2, 0);
	}
	self->romans[count - 1].next = NULL;
	quads = bkHeapAlloc(200, CB_FILE, 0x42, 0x1001);
	if (quads != NULL)
		quads = CFire_QuadSprites_ctor(quads);
	self->quads = quads;
	CFire_QuadSprites_Init(quads, 1, texture, count);
	return 1;
}

/* 0x000d88d0  CarryOn/CFire_Romans.cpp (file inferred)
Points the candle's spot light straight down at it from 600 above the ground (800 when
there is no ground below) in the candle's colour, white when it is bright and low. */
void CFire_Romans_UpdateLight(struct CFire_Romans *self, struct CFire_Roman *roman)
{
	float pos[4];
	float target[4];
	float t, f, ground;
	float r, g, b;

	(void)self;
	t = roman->brightness * 0.0104166670f;              /* 1/96 (0x00268d20) */
	f = 1.0f - t;
	pos[0] = roman->pos[0];
	pos[1] = roman->pos[1];
	pos[2] = roman->pos[2];
	pos[3] = roman->pos[3];
	ground = CLColl_GroundProbe(pos, 0.0f, NULL, NULL, 0);
	roman->ground = ground;
	if (!(f < 0.99f))                                   /* 0x002687dc */
	{
		if (ground < 300.0f)                            /* 0x00254f14 */
		{
			if (0.0f < ground)
				f = f * ground * 0.00333333341f;        /* 0x00268d1c */
			else
				f = 0.0f;
		}
	}
	target[0] = pos[0];
	target[2] = pos[2];
	target[3] = pos[3];
	r = t * 0.333333343f * roman->colour[0] + f * 127.0f;   /* 0x0025df94, 0x00255348 */
	g = t * 0.333333343f * roman->colour[1] + f * 127.0f;
	b = t * 0.333333343f * roman->colour[2] + f * 16.0f;    /* 0x00266d90 */
	if (0.0f < ground && ground < 10000.0f)             /* 0x002657a0 */
		pos[1] = (pos[1] - ground) + 600.0f;            /* 0x00255444 */
	else
		pos[1] = 800.0f;
	target[1] = 0.0f;
	CFire_Lights_SetSpot(DAT_002f7988, roman->light, pos, target);
	CFire_Lights_SetColour(DAT_002f7988, roman->light, (int)r, (int)g, (int)b);
}

/* 0x000d8a70  CarryOn/CFire_Romans.cpp (file inferred)
One step of every candle: rising (sparks every 5 steps, burning from 45 steps before its
fuse ends), burning (glitter while 4 or more rockets fly), falling (it fades, its light
narrows, it goes below -500). */
void CFire_Romans_Update(struct CFire_Romans *self, float dt, struct CFire_Glitter *glitter,
	struct CFire_Sparks *sparks)
{
	struct CFire_Roman **link = &self->used;
	struct CFire_Roman *roman = self->used;
	struct CFire_Roman *next;
	struct CFire_Light *light;
	float vel[3];
	float t, f, g, y1, y2, mx, mz, py, angle, b;
	int removed;
	int n;

	while (roman != NULL)
	{
		removed = 0;
		t = roman->timer - 1.0f;
		roman->timer = t;
		if (roman->state == 0)
		{
			if (roman->light->angle != DAT_00294bd0)
				CFire_Lights_SetAngle(DAT_002f7988, roman->light, DAT_00294bd0);
			roman->vel[0] = roman->vel[0] * 0.99f;      /* 0x002687dc */
			roman->vel[2] = roman->vel[2] * 0.99f;
			roman->vel[1] = roman->gravity + roman->vel[1];
			roman->pos[0] = roman->pos[0] + roman->vel[0];
			roman->pos[1] = roman->pos[1] + roman->vel[1];
			roman->pos[2] = roman->pos[2] + roman->vel[2];
			if ((int)roman->timer % 5 == 0)
			{
				vel[0] = roman->vel[0] * -0.25f;        /* 0x002687d4 */
				vel[1] = roman->vel[1] * -0.25f;
				vel[2] = roman->vel[2] * -0.25f;
				CFire_Sparks_EmitRandom(sparks, roman->pos, vel, 5);
			}
			if (roman->vel[1] < 0.0f && 45.0f < roman->timer)  /* 0x00257c30 */
				roman->timer = 45.0f;
			if (roman->timer < 45.0f)
			{
				roman->state = 1;
				roman->burnPos[0] = roman->pos[0];
				roman->burnPos[1] = roman->pos[1];
				roman->burnPos[2] = roman->pos[2];
				roman->burnPos[3] = roman->pos[3];
				roman->burnVel = 0.0f;
			}
		}
		else if (roman->state == 1)
		{
			if (3 < DAT_00293b44)
			{
				if (t < 0.0f)
				{
					roman->timer = 0.0f;
					roman->state = 2;
					roman->fallStart[0] = roman->pos[0];
					roman->fallStart[1] = roman->pos[1];
					roman->fallStart[2] = roman->pos[2];
					roman->fallStart[3] = roman->pos[3];
					roman->fallPos[0] = roman->pos[0];
					roman->fallPos[1] = roman->pos[1];
					roman->fallPos[2] = roman->pos[2];
					roman->fallPos[3] = roman->pos[3];
					roman->fallVel = 0.0f;
					roman->fade = roman->brightness * -0.0333333f;     /* 0x00268d28 */
				}
				n = (int)(sin((double)roman->timer * 0.0698132f) * 3.5f + 0.75f);  /* 0x00268d24, 0x00268a24, 0x00253844 */
				for (; n > 0; n--)
					CFire_Glitter_Emit(glitter, roman->pos, 0.2f, 20.0f, 1.5f);
			}
			roman->vel[0] = roman->vel[0] * 0.95f;      /* 0x00268d2c */
			roman->vel[2] = roman->vel[2] * 0.95f;
			roman->vel[1] = roman->gravity + roman->vel[1];
			roman->pos[0] = roman->pos[0] + roman->vel[0];
			roman->pos[1] = roman->pos[1] + roman->vel[1];
			roman->pos[2] = roman->pos[2] + roman->vel[2];
			f = roman->burnVel - 0.03f;                 /* 0x00268ce8 */
			roman->burnVel = f;
			roman->burnPos[1] = f + roman->burnPos[1];
		}
		else if (roman->state == 2)
		{
			light = roman->light;
			if (DAT_00294bd4 < light->angle)
			{
				angle = light->angle - dt * 0.00628318f;    /* 0x00268d30 */
				if (angle < DAT_00294bd4)
					angle = DAT_00294bd4;
				CFire_Lights_SetAngle(DAT_002f7988, light, angle);
			}
			b = roman->brightness;
			roman->brightness = roman->fade + b;
			if (roman->fade + b < 0.0f)
				roman->brightness = 0.0f;
			roman->vel[0] = roman->vel[0] * 0.95f;
			roman->vel[2] = roman->vel[2] * 0.95f;
			roman->vel[1] = roman->gravity + roman->vel[1];
			roman->fallPos[0] = roman->vel[0] + roman->fallPos[0];
			roman->fallPos[1] = roman->fallPos[1] + roman->vel[1];
			roman->fallPos[2] = roman->fallPos[2] + roman->vel[2];
			f = roman->brightness * 0.0104166670f;      /* 1/96 */
			g = 1.0f - f;
			y1 = roman->burnVel - 0.03f;
			roman->burnVel = y1;
			y1 = y1 + roman->burnPos[1];
			roman->burnPos[1] = y1;
			y2 = roman->fallVel - 0.03f;
			roman->fallVel = y2;
			y2 = y2 + roman->fallStart[1];
			roman->fallStart[1] = y2;
			mx = (roman->fallStart[0] + roman->burnPos[0]) * 0.5f;
			roman->pos[0] = mx;
			mz = (roman->fallStart[2] + roman->burnPos[2]) * 0.5f;
			roman->pos[2] = mz;
			if (y1 <= y2)
				roman->pos[1] = y2;
			else
				roman->pos[1] = y1;
			roman->pos[0] = f * roman->fallPos[0] + mx * g;
			py = g * roman->pos[1] + f * roman->fallPos[1];
			roman->pos[1] = py;
			roman->pos[2] = mz * g + f * roman->fallPos[2];
			if (py < -500.0f)                           /* 0x00268ce4 */
			{
				roman->state = 3;
				removed = 1;
			}
		}
		CFire_Romans_UpdateLight(self, roman);
		next = roman->next;
		if (removed)
		{
			*link = next;
			roman->next = self->free;
			self->free = roman;
			if (roman->light != NULL)
			{
				/* the original switches the first candle's light back on (flag 2 cleared),
				   not this one's */
				CFire_Lights_SetFlags(DAT_002f7988, self->romans[0].light, 2, 2);
				CFire_Lights_Enable(DAT_002f7988, roman->light, 0);
			}
		}
		else
		{
			link = &roman->next;
		}
		roman = next;
	}
}

/* 0x000d8ec0  CarryOn/CFire_Romans.cpp (file inferred)
Draws the candles as quads in their colour and brightness. */
void CFire_Romans_Draw(struct CFire_Romans *self)
{
	struct CFire_Roman *roman;
	float pos[4];

	if (self->used == NULL)
		return;
	bdSetWorldMatrix((float *)DAT_002855f0);
	pos[3] = 1.0f;
	CFire_QuadSprites_Begin(self->quads);
	for (roman = self->used; roman != NULL; roman = roman->next)
	{
		pos[0] = roman->pos[0];
		pos[1] = roman->pos[1];
		pos[2] = roman->pos[2];
		CFire_QuadSprites_Add(self->quads, pos, roman->size,
			DAT_002bd640[(int)roman->colour[2]] | DAT_002bc490[(int)roman->colour[1]] |
			DAT_002bce18[(int)roman->colour[0]] | DAT_002bc898[(int)roman->brightness]);
	}
	CFire_QuadSprites_Flush(self->quads);
}

/* 0x000d8fd0  CarryOn/CFire_Romans.cpp (file inferred)
Launches a candle; NULL when all are in use. */
struct CFire_Roman *CFire_Romans_Launch(struct CFire_Romans *self, float *pos, float *vel,
	float gravity, float size, float r, float g, float b, float fuse)
{
	struct CFire_Roman *roman = self->free;

	if (roman == NULL)
		return NULL;
	self->free = roman->next;
	roman->next = self->used;
	self->used = roman;
	roman->pos[0] = pos[0];
	roman->pos[1] = pos[1];
	roman->pos[2] = pos[2];
	roman->pos[3] = 1.0f;
	roman->size = size * 0.75f + size * 0.25f * (float)cb_rand() * 3.0518509e-05f;
	roman->vel[0] = vel[0];
	roman->vel[1] = vel[1];
	roman->vel[2] = vel[2];
	roman->gravity = gravity;
	roman->colour[0] = r;
	roman->colour[1] = g;
	roman->unk20 = 0.0f;
	roman->colour[2] = b;
	roman->timer = fuse;
	roman->brightness = 96.0f;
	roman->fade = 0.0f;
	CFire_Lights_SetFlags(DAT_002f7988, roman->light, 2, 2);
	CFire_Lights_Enable(DAT_002f7988, roman->light, 1);
	CFire_Romans_UpdateLight(self, roman);
	roman->state = 0;
	return roman;
}

/* 0x000d90f0  CarryOn/CFire_Romans.cpp (file inferred)
The candle nearest to pos in x/z, NULL when none flies. */
struct CFire_Roman *CFire_Romans_Nearest(struct CFire_Romans *self, float *pos)
{
	struct CFire_Roman *roman;
	struct CFire_Roman *best = NULL;
	float bestDist = 1e13f;
	float d;

	roman = self->used;
	if (roman == NULL)
		return NULL;
	do
	{
		d = (roman->pos[0] - pos[0]) * (roman->pos[0] - pos[0]) +
			(roman->pos[2] - pos[2]) * (roman->pos[2] - pos[2]);
		if (d < bestDist)
		{
			best = roman;
			bestDist = d;
		}
		roman = roman->next;
	} while (roman != NULL);
	if (best != NULL)
		return best;
	return NULL;
}

/* 0x000d9150  CarryOn/CFire_Romans.cpp (file inferred)
The number of candles that fly. */
int CFire_Romans_Count(struct CFire_Romans *self)
{
	struct CFire_Roman *roman;
	int n = 0;

	for (roman = self->used; roman != NULL; roman = roman->next)
		n++;
	return n;
}

/* 0x000d9170  CarryOn/CFire_Romans.cpp (file inferred)
The candles 10 to 600 above the ground into `out`, best first: `prefer` (10), falling (5),
burning (2), rising (1); lower ones first among equals. Returns their number. The original
keeps the priorities on its stack in arrays of 8 (more candles overrun them); this keeps
up to 64. */
int CFire_Romans_Pick(struct CFire_Romans *self, int unused, struct CFire_Roman *prefer,
	struct CFire_Roman **out)
{
	struct CFire_Roman *roman;
	int prio[64];
	float ground[64];
	int n = 0;
	int p;
	int i;
	int k;

	(void)unused;
	for (roman = self->used; roman != NULL; roman = roman->next)
	{
		if (!(10.0f <= roman->ground && roman->ground <= 600.0f))   /* 0x002536d8, 0x00255444 */
			continue;
		if (roman == prefer)
			p = 10;
		else if (roman->state == 2)
			p = 5;
		else
			p = (roman->state == 1) + 1;
		for (i = 0; i < n; i++)
		{
			if (prio[i] < p)
				break;
			if (prio[i] <= p && roman->ground < ground[i])
				break;
		}
		for (k = n - 1; k >= i; k--)
		{
			out[k + 1] = out[k];
			ground[k + 1] = ground[k];
			prio[k + 1] = prio[k];
		}
		out[i] = roman;
		ground[i] = roman->ground;
		prio[i] = p;
		n++;
	}
	return n;
}

/* 0x000d9360  CarryOn/CFire_Romans.cpp (file inferred)
Frees a pool's items and its sprites. */
void CFire_Pool_Free(struct CFire_Pool *self)
{
	struct CFire_PointSprites *sprites;

	bkHeapFree(self->items);
	self->items = NULL;
	sprites = self->sprites;
	if (sprites != NULL)
	{
		CFire_PointSprites_Free(sprites);
		bkHeapFree(sprites);
	}
}
