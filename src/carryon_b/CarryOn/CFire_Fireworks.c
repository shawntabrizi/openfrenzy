/*
CarryOn/CFire_Fireworks.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Fireworks.cpp

The fireworks of CarryOn: per variant a set of particle systems (stars, smoke, glitter,
sparks, classlib_ui's CLFireworks) plus two pools of their own:

- rockets (0x34 bytes) that rise trailing smoke and burst into 400 stars;
- shells (0x7c bytes), launched from the players' launch pads (variant 3) or by type: each
  rises with a trail (smoke, glitter, stars, a spiral), bursts (sparks, rings of stars,
  glitter, a flash of light) and may split into smaller shells.

The shell's types (CFire_Shell.types): [0] the spark burst, [1] the rings of stars,
[2] how many shells it splits into, [3] the trail and glitter, [4] the spiral trail; the
tables 0x00294688-0x00294908 give their parameters.

0x000cf200 is tagged with CFire_Fireworks.cpp; the other functions work on the same
object (file inferred). 0x000cf190 (the CLFireworks burst callback) is missing from the
function table.
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_Fireworks.cpp"

#define CB_RAND01 3.0518509e-05f    /* 1/32767 (0x00253740) */

/* ---------- the shell tables (.data 0x00294688-0x00294928) */
/* storage: carryon_a's of_co_data2 (gen_carryon_data.py); declarations only */

/* by spark burst type (shell types[0]): sparks, their fade-in (and by trail type: spread) */
extern int DAT_00294688[8];  /* 0, 30, 100, 600, 600, 1000, 400, 600 */
extern float DAT_002946a8[8];  /* 45.0f, 45.0f, 45.0f, 45.0f, 60.0f, 80.0f, 45.0f, 45.0f */
extern float DAT_002946c8[8];  /* 0.01f, 0.01f, 0.1f, 0.3f, 0.3f, 0.3f, 0.01f, 0.01f */
/* by ring type (types[1]): stars per ring, their size, rings, ring speed */
extern int DAT_002946e8[8];  /* 0, 30, 100, 500, 500, 700, 600, 600 */
extern float DAT_00294708[8];  /* 40.0f, 40.0f, 35.0f, 30.0f, 25.0f, 25.0f, 30.0f, 30.0f */
extern int DAT_00294728[8];  /* 1, 1, 1, 1, 2, 3, 1, 1 */
extern float DAT_00294748[8];  /* 10.0f, 10.0f, 10.0f, 20.0f, 20.0f, 40.0f, 20.0f, 20.0f */
/* the same for variant 2 with 3 parts a firework (types[1] 0-3) */
extern int DAT_00294768[4];  /* 0, 100, 600, 1800 */
extern float DAT_00294778[4];  /* 40.0f, 40.0f, 35.0f, 30.0f */
extern int DAT_00294788[4];  /* 1, 1, 1, 1 */
extern float DAT_00294798[4];  /* 10.0f, 20.0f, 25.0f, 30.0f */
/* by trail type: glitter at the burst, its size and speed */
extern int DAT_002947a8[8];  /* 0, 0, 0, 20, 150, 800, 300, 0 */
extern float DAT_002947c8[8];  /* 20.0f, 20.0f, 20.0f, 30.0f, 30.0f, 30.0f, 30.0f, 20.0f */
extern float DAT_002947e8[8];  /* 1.0f, 1.0f, 1.0f, 5.0f, 15.0f, 40.0f, 30.0f, 30.0f */
/* by split type (types[2]): the shells it splits into */
extern int DAT_00294808[8];  /* 0, 3, 8, 16, 32, 64, 0, 0 */
/* [parts a firework (0-4)][part]: the trail type of a part */
extern int DAT_00294828[5][8];
/* { 0, 0, 0, 0, 0, 0, 6, 7 }
   { 0, 5, 0, 0, 0, 0, 6, 7 }
   { 0, 3, 5, 0, 0, 0, 6, 7 }
   { 0, 2, 3, 5, 0, 0, 6, 7 }
   { 0, 2, 3, 4, 5, 0, 6, 7 } */
/* 0x002948e8, 0x002948f8, 0x00294908  the x, y and z axes */
extern float DAT_002948e8[4];  /* 1.0f, 0.0f, 0.0f, 1.0f */
extern float DAT_002948f8[4];  /* 0.0f, 1.0f, 0.0f, 1.0f */
extern float DAT_00294908[4];  /* 0.0f, 0.0f, 1.0f, 1.0f */
/* 0x00294918  the types[0..4] of the four shell kinds of CFire_Fireworks_AddShellOfType */
extern int DAT_00294918[4][5];
/* { 0, 0, 0, 0, 0 }
   { 0, 6, 0, 0, 0 }
   { 6, 0, 0, 6, 0 }
   { 0, 0, 0, 7, 6 } */

/* the arguments of a light flash: (channel + 127) / 2 */
#define CB_HALF(x) ((x) / 2)

/* 0x000cf190  CarryOn/CFire_Fireworks.cpp (file inferred; missing from the function table)
The burst callback of the CLFireworks: the burst, two sounds and a light flash in the
rocket's colour. */
void FUN_000cf190(void *fireworks, float *rocket)
{
	CB_DUAL(FUN_0007fc30, void (*)(int, int), void (*)(void *, float *),
		((int)(uintptr_t)fireworks, (int)(uintptr_t)rocket), (fireworks, rocket));
	CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 2, -1, 0xff);
	CB_CALL(FUN_000c7880, void (*)(void *, int, int))(DAT_002f6d5c, -1, 1);
	CFire_Lights_Flash(DAT_002f7988, rocket + 1, (int)rocket[9] + 0x20, (int)rocket[10] + 0x20,
		(int)rocket[11] + 0x20);
}

/* 0x000cf200  CarryOn/CFire_Fireworks.cpp
Makes the particle systems of the variant (0: CLFireworks and sparks; 2: stars, smoke,
glitter, sparks and shells; 3 and up: two star sets, smoke, sparks, glitter and shells),
then the rocket and shell pools. */
void CFire_Fireworks_Init(struct CFire_Fireworks *self, int variant)
{
	struct bkResource *glow;
	struct bkResource *smoke;
	void *cl;
	struct CFire_Sparks *sparks;
	struct CFire_QuadSprites *quads;
	int i;

	memset(self, 0, 0x3c);
	if (variant == 0)
	{
		glow = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
		smoke = bLoadResource(NULL, "alphatex\\firework_smoke.bmp", 0, 0xdefa);
		cl = bkHeapAlloc(0x1c, CB_FILE, 0x48, 0x1001);
		if (cl == NULL)
			cl = NULL;
		else
			cl = CB_DUAL(FUN_0007ffa0, int *(*)(int *, int, uint, int, int, uint, uint),
				void *(*)(void *, int, struct bkResource *, float, int, struct bkResource *, float),
				((int *)cl, 800, (uint)(uintptr_t)smoke, (int)cb_fbits(50.0f), 30000,
					(uint)(uintptr_t)glow, cb_fbits(60.0f)),
				(cl, 800, smoke, 50.0f, 30000, glow, 60.0f));
		self->clFireworks = cl;
		CB_PTR(cl, 0xc) = (void *)FUN_000cf190;
		sparks = bkHeapAlloc(0x20, CB_FILE, 0x55, 0x1001);
		if (sparks != NULL)
			sparks = CFire_Sparks_ctor(sparks);
		self->sparks = sparks;
		glow = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
		CFire_Sparks_Init(self->sparks, glow, 500);
	}
	else if (variant != 1)
	{
		if (variant == 2)
		{
			self->stars = bkHeapAlloc(0x14, CB_FILE, 0x62, 0x1001);
			CFire_FWorkStars_Init(self->stars, 20000, 55.0f);
			self->smoke = bkHeapAlloc(0x14, CB_FILE, 0x65, 0x1001);
			CFire_FWorkSmoke_Init(self->smoke, 2000);
			self->glitter = bkHeapAlloc(0x118, CB_FILE, 0x67, 0x1001);
			CFire_Glitter_Init(self->glitter, 20000, 40.0f);
			sparks = bkHeapAlloc(0x20, CB_FILE, 0x6a, 0x1001);
			if (sparks != NULL)
				sparks = CFire_Sparks_ctor(sparks);
			self->sparks = sparks;
			glow = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
			CFire_Sparks_Init(self->sparks, glow, 2000);
			self->numShells = 100;
		}
		else
		{
			self->stars = bkHeapAlloc(0x14, CB_FILE, 0x80, 0x1001);
			CFire_FWorkStars_Init(self->stars, 10000, 100.0f);
			self->stars2 = bkHeapAlloc(0x14, CB_FILE, 0x82, 0x1001);
			CFire_FWorkStars_Init(self->stars2, 10000, 50.0f);
			self->smoke = bkHeapAlloc(0x14, CB_FILE, 0x85, 0x1001);
			CFire_FWorkSmoke_Init(self->smoke, 800);
			sparks = bkHeapAlloc(0x20, CB_FILE, 0x89, 0x1001);
			if (sparks != NULL)
				sparks = CFire_Sparks_ctor(sparks);
			self->sparks = sparks;
			glow = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
			CFire_Sparks_Init(self->sparks, glow, 2000);
			self->glitter = bkHeapAlloc(0x118, CB_FILE, 0x91, 0x1001);
			CFire_Glitter_Init(self->glitter, 18000, 45.0f);
			self->numShells = 100;
		}
	}
	if (self->stars == NULL)
	{
		self->usedRockets = NULL;
		self->numRockets = 0;
	}
	else
	{
		self->numRockets = 0x32;
		self->rockets = bkHeapAlloc(0xa28, CB_FILE, 0x9e, 0x2001);
		if (self->rockets == NULL)
		{
			self->numRockets = 0;
			return;
		}
		memset(self->rockets, 0, (size_t)self->numRockets * 0x34);
		self->usedRockets = NULL;
		self->freeRockets = self->rockets;
		for (i = 0; i < self->numRockets - 1; i++)
			self->rockets[i].next = &self->rockets[i + 1];
		self->rockets[self->numRockets - 1].next = NULL;
	}
	if (self->numShells == 0)
	{
		self->usedShells = NULL;
		self->shells = NULL;
		return;
	}
	self->shells = bkHeapAlloc(self->numShells * 0x7c, CB_FILE, 0xb6, 0x2001);
	if (self->shells == NULL)
	{
		self->numShells = 0;
		return;
	}
	memset(self->shells, 0, (size_t)self->numShells * 0x7c);
	self->usedShells = NULL;
	self->freeShells = self->shells;
	for (i = 0; i < self->numShells - 1; i++)
		self->shells[i].next = &self->shells[i + 1];
	self->shells[self->numShells - 1].next = NULL;
	glow = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
	quads = bkHeapAlloc(200, CB_FILE, 0xd0, 0x1001);
	if (quads != NULL)
		quads = CFire_QuadSprites_ctor(quads);
	self->quads = quads;
	CFire_QuadSprites_Init(quads, 2, glow, self->numShells);
}

/* 0x000cf6d0  CarryOn/CFire_Fireworks.cpp (file inferred)
A sphere of 400 stars flying out at 14 (plus vel). */
void CFire_Fireworks_BurstStars(struct CFire_Fireworks *self, float *pos, float *vel, float r,
	float g, float b)
{
	float dir[3];
	float v[4];
	int n;

	for (n = 0x190; n != 0; n--)
	{
		dir[0] = (float)cb_rand() * 0.000305185094f - 5.0f;    /* 0x00265808 */
		dir[1] = (float)cb_rand() * 0.000305185094f - 5.0f;
		dir[2] = (float)cb_rand() * 0.000305185094f - 5.0f;
		bmVectorSetLength(v, dir, 14.0f);
		v[0] = v[0] + vel[0];
		v[1] = v[1] + vel[1];
		v[2] = v[2] + vel[2];
		CFire_FWorkStars_Add(self->stars, pos, v, -0.15f, 40.0f, r, g, b, 0.5f);
	}
}

/* 0x000cf810  CarryOn/CFire_Fireworks.cpp (file inferred)
A ring (a sphere of radius speed/2) of `count` stars; the more stars, the darker each. */
void CFire_Fireworks_BurstRing(struct CFire_Fireworks *self, float *pos, float *vel, int count,
	float size, float speed, float r, float g, float b)
{
	float dir[3];
	float v[4];
	float dim;
	float half;
	int n;

	dim = 1.0f / ((float)count * 0.00111111114f + 1.0f);  /* 0x00268a94 */
	if (count <= 0)
		return;
	half = speed * 0.5f;
	for (n = count; n != 0; n--)
	{
		dir[0] = (float)cb_rand() * speed * CB_RAND01 - half;
		dir[1] = (float)cb_rand() * speed * CB_RAND01 - half;
		dir[2] = (float)cb_rand() * speed * CB_RAND01 - half;
		bmVectorSetLength(v, dir, half);
		v[0] = v[0] + vel[0];
		v[1] = v[1] + vel[1];
		v[2] = v[2] + vel[2];
		CFire_FWorkStars_Add(self->stars, pos, v, -0.15f, size, r * dim, g * dim, dim * b, 0.5f);
	}
}

/* 0x000cf9a0  CarryOn/CFire_Fireworks.cpp (file inferred)
Launches a CLFireworks rocket in the colour (life 40, 180 sparks at 14 living 50). */
int CFire_Fireworks_Launch(struct CFire_Fireworks *self, float *pos, float *vel, float r, float g,
	float b)
{
	CB_DUAL(FUN_0007fe80,
		int (*)(int, undefined4 *, undefined4 *, undefined4, float, float, float, undefined4,
			undefined4, undefined4),
		int (*)(void *, float *, float *, float, float, float, float, int, float, float),
		((int)(uintptr_t)self->clFireworks, (undefined4 *)pos, (undefined4 *)vel, cb_fbits(40.0f),
			r, g, b, 0xb4, cb_fbits(14.0f), cb_fbits(50.0f)),
		(self->clFireworks, pos, vel, 40.0f, r, g, b, 0xb4, 14.0f, 50.0f));
	return 0;
}

/* 0x000cf9e0  CarryOn/CFire_Fireworks.cpp (file inferred)
Launches a CLFireworks rocket in a random bright colour. */
int CFire_Fireworks_LaunchRandom(struct CFire_Fireworks *self, float *pos, float *vel)
{
	float r, g, b;

	CFire_RandomColour(&r, &g, &b);
	CFire_Fireworks_Launch(self, pos, vel, r, g, b);
	return 0;
}

/* 0x000cfa40  CarryOn/CFire_Fireworks.cpp (file inferred)
Raises *height to the highest rocket or shell (scaled). */
void CFire_Fireworks_MaxHeight(struct CFire_Fireworks *self, float *height, float scale)
{
	struct CFire_FWRocket *rocket;
	struct CFire_Shell *shell;

	for (rocket = self->usedRockets; rocket != NULL; rocket = rocket->next)
		if (*height < scale * rocket->pos[1])
			*height = scale * rocket->pos[1];
	for (shell = self->usedShells; shell != NULL; shell = shell->next)
		if (*height < scale * shell->pos[1])
			*height = scale * shell->pos[1];
}

/* 0x000cfaa0  CarryOn/CFire_Fireworks.cpp (file inferred)
A shell bursts: rings of stars (in variant 2 each ring in a random colour), sparks,
glitter, sounds and a light flash in the mixed colours. */
void CFire_Fireworks_Burst(struct CFire_Fireworks *self, struct CFire_Shell *shell)
{
	float vel[4];
	float r, g, b, most, speed, sparkFadeIn, glitterSize, glitterSpeed;
	float ringSpeed;
	float ringSize;
	int ringCount;
	int rings;
	int i;
	int cr = 0x7f, cg = 0x7f, cb = 0x7f;
	int sparkCount;
	int type;
	int sr, sg, sb;
	int glitter;
	int t1;

	vel[0] = shell->vel[0] * 0.25f;                     /* 0x00257b88 */
	vel[1] = shell->vel[1] * 0.25f;
	vel[2] = shell->vel[2] * 0.25f;
	vel[3] = 0.0f;
	CB_CALL(FUN_000c7880, void (*)(void *, int, int))(DAT_002f6d5c, -1, 0);
	t1 = shell->types[1];
	if (CB_I32(DAT_002f6d5c, 0x250) == 2 && DAT_00294108 == 3 && t1 <= 3)
	{
		ringCount = DAT_00294768[t1];
		ringSize = DAT_00294778[t1];
		rings = DAT_00294788[t1];
		ringSpeed = DAT_00294798[t1];
	}
	else
	{
		ringCount = DAT_002946e8[t1];
		ringSize = DAT_00294708[t1];
		rings = DAT_00294728[t1];
		ringSpeed = DAT_00294748[t1];
	}
	if (CB_I32(DAT_002f6d5c, 0x250) == 2)
	{
		for (i = 1; i <= rings; i++)
		{
			speed = ((float)i * ringSpeed) / (float)rings;
			CFire_RandomColour(&r, &g, &b);
			most = r;
			if (r < g)
				most = g;
			if (most < b)
				most = b;
			most = 1.0f / most;
			r = r * most * 63.0f;                       /* 0x002630c8 */
			g = g * most * 63.0f;
			b = most * b * 63.0f;
			cr = CB_HALF((int)(r + r + (float)cr));
			cg = CB_HALF((int)(g + g + (float)cg));
			cb = CB_HALF((int)(b + b + (float)cb));
			CFire_Fireworks_BurstRing(self, shell->pos, vel, ringCount, ringSize, speed, r, g, b);
		}
	}
	else
	{
		for (i = 1; i <= rings; i++)
			CFire_Fireworks_BurstRing(self, shell->pos, vel, ringCount, ringSize,
				((float)i * ringSpeed) / (float)rings, shell->burstColour[0], shell->burstColour[1],
				shell->burstColour[2]);
	}
	if (self->sparks != NULL)
	{
		sparkCount = DAT_00294688[shell->types[0]];
		sparkFadeIn = DAT_002946c8[shell->types[0]];
		type = DAT_00294828[DAT_00294108][shell->types[0]];
		if (CB_I32(DAT_002f6d5c, 0x250) == 2)
		{
			if (shell->types[0] == DAT_00294108)
			{
				sr = -1;
				sg = 0;
				sb = 0;
			}
			else
			{
				sr = 0x80;
				sg = 100;
				sb = 0x28;
			}
		}
		else
		{
			sr = (int)shell->burstColour[0];
			sg = (int)shell->burstColour[1];
			sb = (int)shell->burstColour[2];
			sparkFadeIn = 0.01f;
		}
		if (DAT_00294688[type] != 0)
		{
			CFire_Sparks_Burst(self->sparks, shell->pos, sparkCount, DAT_002946a8[type], sr, sg, sb,
				sparkFadeIn);
			if (sr == -1)
			{
				cr = CB_HALF(cr + 0x7f);
				cg = CB_HALF(cg + 0x7f);
				cb = CB_HALF(cb + 0x7f);
			}
			else
			{
				cr = CB_HALF(sr + cr);
				cg = CB_HALF(sg + cg);
				cb = CB_HALF(sb + cb);
			}
		}
	}
	type = DAT_00294828[DAT_00294108][shell->types[3]];
	glitter = DAT_002947a8[type];
	glitterSize = DAT_002947c8[type];
	glitterSpeed = DAT_002947e8[type];
	CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 1, -1, 0xff);
	CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 7, shell->strength, 0x80);
	if (glitter != 0)
	{
		CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 3, -1, 0xff);
		cr = CB_HALF(cr + 0x7f);
		cg = CB_HALF(cg + 0x7f);
		cb = CB_HALF(cb);
		for (; glitter > 0; glitter--)
			CFire_Glitter_EmitSphere(self->glitter, shell->pos, 0.2f, glitterSize, glitterSpeed);
	}
	CFire_Lights_Flash(DAT_002f7988, shell->pos, cr, cg, cb);
}

/* 0x000cff30  CarryOn/CFire_Fireworks.cpp (file inferred)
Draws a glow on the first shell that has a spiral trail or splits deeply (when the
"Fireworks" tweak is on; only that one shell). */
void CFire_Fireworks_DrawShells(struct CFire_Fireworks *self)
{
	struct CFire_Shell *shell;

	if (self->shells == NULL || DAT_00294114 == 0)
		return;
	CFire_QuadSprites_Begin(self->quads);
	shell = self->usedShells;
	if (shell != NULL)
	{
		while (shell->types[4] == 0 && shell->types[2] < DAT_00294108 - 1 &&
			(CB_I32(DAT_002f6d5c, 0x250) != 2 || 3 < DAT_00294108))
		{
			shell = shell->next;
			if (shell == NULL)
			{
				CFire_QuadSprites_Flush(self->quads);
				return;
			}
		}
		CFire_QuadSprites_Add(self->quads, shell->pos, 80.0f,
			DAT_002bd640[(int)shell->colour[2]] | DAT_002bc490[(int)shell->colour[1]] |
			DAT_002bce18[(int)shell->colour[0]] | DAT_002bc898[127]);   /* DAT_002bca94 */
	}
	CFire_QuadSprites_Flush(self->quads);
}

/* 0x000d0000  CarryOn/CFire_Fireworks.cpp (file inferred)
Launches a shell of the given types; its colour is the player's (varied, variant 3) or a
random one at a quarter. Its spiral axis is across its velocity. NULL when all are in
use. */
struct CFire_Shell *CFire_Fireworks_AddShell(struct CFire_Fireworks *self, float *pos, float *vel,
	int t0, int t1, int t2, int t3, int t4, int player)
{
	struct CFire_Shell *shell = self->freeShells;
	float *axis;
	float cross[4];
	int colour;
	int r, g, b, most;
	int r1, r2, r3;

	if (shell == NULL)
		return NULL;
	CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 5, -1, 0x80);
	self->freeShells = shell->next;
	shell->next = self->usedShells;
	self->usedShells = shell;
	shell->pos[0] = pos[0];
	shell->pos[1] = pos[1];
	shell->pos[2] = pos[2];
	shell->pos[3] = 1.0f;
	shell->fuse = 40.0f;
	shell->vel[0] = vel[0];
	shell->vel[1] = vel[1];
	shell->vel[2] = vel[2];
	shell->vel[3] = vel[3];
	shell->types[0] = (signed char)t0;
	shell->types[1] = (signed char)t1;
	shell->types[3] = (signed char)t3;
	shell->gravity = 0.0f;
	shell->types[2] = (signed char)t2;
	shell->types[4] = (signed char)t4;
	shell->player = player;
	if (CB_I32(DAT_002f6d5c, 0x250) == 3 && player != -1)
	{
		colour = DAT_002baed4[player * 10] * 0x60;
		r1 = cb_rand();
		r2 = cb_rand();
		r3 = cb_rand();
		r = (DAT_00290718[colour] - 0x20) + (r1 & 0x3f);
		g = (DAT_00290719[colour] - 0x20) + (r2 & 0x3f);
		b = (DAT_0029071a[colour] - 0x20) + (r3 & 0x3f);
		if (r < 0)
			r = 0;
		if (g < 0)
			g = 0;
		if (b < 0)
			b = 0;
		most = r;
		if (r < g)
			most = g;
		if (most < b)
			most = b;
		shell->colour[0] = (float)((r * 0x3f) / most);
		shell->colour[1] = (float)((g * 0x3f) / most);
		shell->colour[2] = (float)((b * 0x3f) / most);
	}
	else
	{
		CFire_RandomColour(&shell->colour[0], &shell->colour[1], &shell->colour[2]);
		shell->colour[0] = shell->colour[0] * 0.25f;
		shell->colour[1] = shell->colour[1] * 0.25f;
		shell->colour[2] = shell->colour[2] * 0.25f;
	}
	shell->burstColour[1] = shell->colour[1];
	shell->colourVel[0] = 0.0f;
	shell->colourVel[1] = 0.0f;
	shell->colourVel[2] = 0.0f;
	shell->colourVel[3] = 0.0f;
	shell->burstColour[0] = shell->colour[0];
	shell->burstColour[2] = shell->colour[2];
	shell->alpha = 127.0f;
	/* the axis most across the velocity (the original compares x with y twice) */
	if (!(shell->vel[0] <= shell->vel[1]) && !(shell->vel[0] <= shell->vel[1]))
	{
		axis = DAT_002948e8;
	}
	else if (shell->vel[1] <= shell->vel[0])
	{
		axis = DAT_00294908;
	}
	else
	{
		axis = DAT_002948f8;
		if (shell->vel[1] <= shell->vel[2])
			axis = DAT_00294908;
	}
	cross[0] = axis[2] * shell->vel[1] - shell->vel[2] * axis[1];
	cross[1] = axis[0] * shell->vel[2] - shell->vel[0] * axis[2];
	cross[2] = shell->vel[0] * axis[1] - axis[0] * shell->vel[1];
	cross[3] = 0.0f;    /* not set by the original */
	bmVectorSetLength(shell->axis, cross, 1.0f);
	return shell;
}

/* 0x000d02e0  CarryOn/CFire_Fireworks.cpp (file inferred)
Launches a shell of kind 0-3 (DAT_00294918); kind 3 flies twice as fast, with gravity,
and its fuse is half as long again. */
void CFire_Fireworks_AddShellOfType(struct CFire_Fireworks *self, float *pos, float *vel, int type,
	int player)
{
	struct CFire_Shell *shell;
	float vy;

	shell = CFire_Fireworks_AddShell(self, pos, vel, (signed char)DAT_00294918[type][0],
		(unsigned char)DAT_00294918[type][1], (unsigned char)DAT_00294918[type][2],
		(unsigned char)DAT_00294918[type][3], (unsigned char)DAT_00294918[type][4], player);
	if (shell == NULL || type != 3)
		return;
	vy = shell->vel[1] + shell->vel[1];
	shell->vel[1] = vy;
	shell->gravity = -(vy / shell->fuse);
	shell->fuse = shell->fuse * 1.5f;                   /* 0x002538a0 */
}

/* 0x000d0360  CarryOn/CFire_Fireworks.cpp (file inferred)
Frees the pools and the particle systems. */
void CFire_Fireworks_Free(struct CFire_Fireworks *self)
{
	struct CFire_QuadSprites *quads;
	void *cl;

	if (self->rockets != NULL)
	{
		bkHeapFree(self->rockets);
		self->rockets = NULL;
	}
	if (self->shells != NULL)
	{
		quads = self->quads;
		if (quads != NULL)
		{
			CFire_PointSprites_Free((struct CFire_PointSprites *)quads);
			bkHeapFree(quads);
		}
		bkHeapFree(self->shells);
		self->shells = NULL;
	}
	if (self->stars != NULL)
	{
		CFire_Pool_Free((struct CFire_Pool *)self->stars);
		bkHeapFree(self->stars);
		self->stars = NULL;
	}
	if (self->stars2 != NULL)
	{
		CFire_Pool_Free((struct CFire_Pool *)self->stars2);
		bkHeapFree(self->stars2);
		self->stars2 = NULL;
	}
	if (self->smoke != NULL)
	{
		CFire_Pool_Free((struct CFire_Pool *)self->smoke);
		bkHeapFree(self->smoke);
		self->smoke = NULL;
	}
	if (self->glitter != NULL)
	{
		CFire_Glitter_Free(self->glitter);
		bkHeapFree(self->glitter);
		self->glitter = NULL;
	}
	if (self->sparks != NULL)
	{
		/* the sparks' free (identical code folded with rowing_b 0x001b47c0) */
		CB_CALL(FUN_001b47c0, void (*)(void *))(self->sparks);
		if (self->sparks != NULL)
			CB_VFN(self->sparks, 0, struct CFire_Sparks *(*)(struct CFire_Sparks *, unsigned int))(
				self->sparks, 1);
		self->sparks = NULL;
	}
	cl = self->clFireworks;
	if (cl != NULL)
	{
		/* the CLFireworks free (identical code folded with classlib_ui 0x00080100) */
		CB_CALL(FUN_00080100, void (*)(void *))(cl);
		bkHeapFree(cl);
		self->clFireworks = NULL;
	}
}

/* 0x000d0450  CarryOn/CFire_Fireworks.cpp (file inferred)
Draws everything (when the "Fireworks" tweak is on). */
void CFire_Fireworks_Draw(struct CFire_Fireworks *self)
{
	if (DAT_00294114 == 0)
		return;
	bdSetRenderState(3, 0, 0);
	if (self->stars != NULL)
		CFire_FWorkStars_Draw(self->stars);
	if (self->stars2 != NULL)
		CFire_FWorkStars_Draw(self->stars2);
	if (self->smoke != NULL)
		CFire_FWorkSmoke_Draw(self->smoke);
	if (self->glitter != NULL)
		CFire_Glitter_Draw(self->glitter);
	if (self->sparks != NULL)
		CFire_Sparks_Draw(self->sparks);
	CFire_Fireworks_DrawShells(self);
	if (self->clFireworks != NULL)
		CLFireworks_Draw((int)(uintptr_t)self->clFireworks);
}

/* 0x000d04c0  CarryOn/CFire_Fireworks.cpp (file inferred)
Moves the rockets (they trail smoke); when its fuse is out a rocket bursts into stars
with sounds and a light flash, and goes. */
void CFire_Fireworks_UpdateRockets(struct CFire_Fireworks *self)
{
	struct CFire_FWRocket **link = &self->usedRockets;
	struct CFire_FWRocket *rocket = self->usedRockets;
	struct CFire_FWRocket *next;
	float fuse;
	int burst;

	while (rocket != NULL)
	{
		fuse = rocket->fuse - 1.0f;
		rocket->fuse = fuse;
		burst = fuse <= 0.0f;
		if (burst)
		{
			rocket->vel[0] = rocket->vel[0] * 0.25f;
			rocket->vel[1] = rocket->vel[1] * 0.25f;
			rocket->vel[2] = rocket->vel[2] * 0.25f;
			CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 2, -1, 0xff);
			CB_CALL(FUN_000c7880, void (*)(void *, int, int))(DAT_002f6d5c, -1, 1);
			CFire_Fireworks_BurstStars(self, rocket->pos, rocket->vel, rocket->colour[0],
				rocket->colour[1], rocket->colour[2]);
			CFire_Lights_Flash(DAT_002f7988, rocket->pos, CB_HALF((int)rocket->colour[0] + 0x7f),
				CB_HALF((int)rocket->colour[1] + 0x7f), CB_HALF((int)rocket->colour[2] + 0x7f));
		}
		rocket->pos[0] = rocket->pos[0] + rocket->vel[0];
		rocket->pos[1] = rocket->pos[1] + rocket->vel[1];
		rocket->pos[2] = rocket->pos[2] + rocket->vel[2];
		CFire_FWorkSmoke_Add(self->smoke, rocket->pos, 0.0f, 40.0f);
		next = rocket->next;
		if (burst)
		{
			*link = next;
			rocket->next = self->freeRockets;
			self->freeRockets = rocket;
		}
		else
		{
			link = &rocket->next;
		}
		rocket = next;
	}
}

/* 0x000d0610  CarryOn/CFire_Fireworks.cpp (file inferred)
A bursting shell splits into smaller shells (one type step less) flying out at up to 10
(downwards at half speed). When the bursting shell is the first of the list, its link
moves on to the new first shell so that its removal keeps them. */
void CFire_Fireworks_Split(struct CFire_Fireworks *self, struct CFire_Shell *shell,
	struct CFire_Shell ***link)
{
	struct CFire_Shell *added;
	float vel[4];
	int t0, t1, t3, t4;
	int n;

	for (n = DAT_00294808[shell->types[2]]; n > 0; n--)
	{
		vel[0] = (float)cb_rand() * 0.000610370189f - 10.0f;    /* 0x002537d4 */
		vel[1] = (float)cb_rand() * 0.000610370189f - 10.0f;
		if (vel[1] < 0.0f)
			vel[1] = vel[1] * 0.5f;
		vel[2] = (float)cb_rand() * 0.000610370189f - 10.0f;
		vel[3] = 0.0f;    /* not set by the original */
		t0 = shell->types[0];
		if (1 < shell->types[0])
			t0--;
		t1 = shell->types[1];
		if (1 < shell->types[1])
			t1--;
		t3 = shell->types[3];
		if (1 < shell->types[3])
			t3--;
		t4 = shell->types[4];
		if (1 < shell->types[4])
			t4--;
		added = CFire_Fireworks_AddShell(self, shell->pos, vel, t0, t1, 0, t3, t4, shell->player);
		if (added != NULL)
			added->fuse = 50.0f;
		if (*link == &self->usedShells && **link == added)
		{
			bkPrintf("Linked List Tidy\n");
			*link = &added->next;
		}
	}
}

/* adds a spiral star of a shell at `side` x 20 along its axis, flying at `speed` x the axis */
static void cb_spiral_star(struct CFire_Fireworks *self, struct CFire_Shell *shell, float side,
	float speed)
{
	float pos[4];
	float vel[4];

	vel[0] = shell->axis[0] * speed;
	vel[1] = shell->axis[1] * speed;
	vel[3] = 0.0f;
	vel[2] = shell->axis[2] * speed;
	pos[0] = shell->axis[0] * 20.0f * side + shell->pos[0];
	pos[1] = shell->axis[1] * 20.0f * side + shell->pos[1];
	pos[2] = shell->axis[2] * 20.0f * side + shell->pos[2];
	pos[3] = 0.0f;    /* not set by the original */
	CFire_FWorkStars_Add(self->stars2, pos, vel, -0.15f, 20.0f, shell->colour[0], shell->colour[1],
		shell->colour[2], 0.01f);
}

/* 0x000d0790  CarryOn/CFire_Fireworks.cpp (file inferred)
Moves the shells: the fuse (burst and split at 0), the colour fade before the burst (and
in variant 3 a ring at that moment), the trail by type, the spiral trail; burst shells go. */
void CFire_Fireworks_UpdateShells(struct CFire_Fireworks *self)
{
	struct CFire_Shell **link;
	struct CFire_Shell *shell;
	struct CFire_Shell *next;
	float c0, c1, c2, vy, k, fuse;
	float c, s;
	float dir[4];
	float cross[4];
	float side[4];
	int burst;
	int i;

	if (self->shells == NULL)
		return;
	link = &self->usedShells;
	shell = self->usedShells;
	while (shell != NULL)
	{
		burst = 0;
		fuse = shell->fuse - 1.0f;
		shell->fuse = fuse;
		if (fuse <= 0.0f)
		{
			burst = 1;
			CFire_Fireworks_Burst(self, shell);
			if (shell->types[2] != 0)
				CFire_Fireworks_Split(self, shell, &link);
		}
		c0 = shell->colour[0];
		c1 = shell->colour[1];
		shell->colour[0] = shell->colourVel[0] + c0;
		c2 = shell->colour[2];
		shell->colour[1] = shell->colourVel[1] + c1;
		shell->colour[2] = c2 + shell->colourVel[2];
		shell->alpha = shell->colourVel[3] + shell->alpha;
		if (shell->fuse < 20.0f && shell->colourVel[0] == 0.0f && shell->colourVel[1] == 0.0f &&
			shell->colourVel[2] == 0.0f && shell->fuse != 0.0f)
		{
			k = 1.0f / shell->fuse;
			shell->colourVel[0] = -((shell->colourVel[0] + c0) * k);
			shell->colourVel[1] = -((shell->colourVel[1] + c1) * k);
			shell->colourVel[2] = -(k * (c2 + shell->colourVel[2]));
			if (CB_I32(DAT_002f6d5c, 0x250) == 3 && shell->types[4] != 0)
			{
				dir[0] = shell->vel[0] * 0.25f;
				dir[1] = shell->vel[1] * 0.25f;
				dir[2] = shell->vel[2] * 0.25f;
				dir[3] = 0.0f;
				CFire_Fireworks_BurstRing(self, shell->pos, dir, 300, 30.0f, 100.0f,
					shell->burstColour[0], shell->burstColour[1], shell->burstColour[2]);
			}
		}
		shell->pos[0] = shell->pos[0] + shell->vel[0];
		shell->pos[1] = shell->pos[1] + shell->vel[1];
		shell->pos[2] = shell->pos[2] + shell->vel[2];
		vy = shell->vel[1];
		shell->vel[1] = shell->gravity + vy;
		switch (DAT_00294828[DAT_00294108][shell->types[3]])
		{
		case 0:
			if (CB_I32(DAT_002f6d5c, 0x250) == 3)
			{
				dir[0] = Core_RandUnit() * 5.0f - 2.5f;  /* 0x002536c4, 0x002551b4 */
				dir[1] = Core_RandUnit() * 5.0f - 2.5f;
				dir[2] = Core_RandUnit() * 5.0f - 2.5f;
				CFire_FWorkStars_Add(self->stars2, shell->pos, dir, -0.15f, 20.0f, shell->colour[0],
					shell->colour[1], shell->colour[2], 0.01f);
			}
			CFire_FWorkSmoke_Add(self->smoke, shell->pos, 0.0f, 40.0f);
			break;
		case 1:
			CFire_Glitter_Emit(self->glitter, shell->pos, 0.2f, 30.0f, 1.5f);
			break;
		case 7:
			if (!(0.0f < shell->gravity + vy))
				break;
			/* rising: as case 2 */
			/* fall through */
		case 2:
			CFire_Glitter_Emit(self->glitter, shell->pos, 0.2f, 30.0f, 1.5f);
			CFire_Glitter_Emit(self->glitter, shell->pos, 0.2f, 30.0f, 1.5f);
			break;
		case 3:
		case 4:
			for (i = 0; i < 3; i++)
				CFire_Glitter_Emit(self->glitter, shell->pos, 0.2f, 30.0f, 2.0f);
			break;
		case 5:
			for (i = 0; i < 4; i++)
				CFire_Glitter_Emit(self->glitter, shell->pos, 0.2f, 30.0f, 4.0f);
			break;
		case 6:
			dir[0] = (float)cb_rand() * 0.000152592547f - 2.5f;   /* 0x00266454 */
			dir[1] = (float)cb_rand() * 0.000152592547f - 2.5f;
			dir[2] = (float)cb_rand() * 0.000152592547f - 2.5f;
			CFire_FWorkStars_Add(self->stars2, shell->pos, dir, -0.15f, 20.0f, shell->colour[0],
				shell->colour[1], shell->colour[2], 0.01f);
			CFire_Glitter_Emit(self->glitter, shell->pos, 0.2f, 30.0f, 1.5f);
			break;
		default:
			break;
		}
		if (1 < shell->types[4])
		{
			/* the spiral: turn the axis 30 degrees about the velocity */
			c = (float)cos(0.5235987305641174);            /* 0x00268ab8 (double) */
			s = (float)sin(0.5235987305641174);
			cross[0] = shell->vel[1] * shell->axis[2] - shell->vel[2] * shell->axis[1];
			cross[1] = shell->axis[0] * shell->vel[2] - shell->vel[0] * shell->axis[2];
			cross[2] = shell->vel[0] * shell->axis[1] - shell->vel[1] * shell->axis[0];
			cross[3] = 0.0f;
			bmVectorSetLength(side, cross, 1.0f);
			dir[0] = side[0] * s + c * shell->axis[0];
			dir[1] = side[1] * s + c * shell->axis[1];
			dir[2] = side[2] * s + c * shell->axis[2];
			dir[3] = 0.0f;
			bmVectorSetLength(shell->axis, dir, 1.0f);
		}
		if (self->stars2 != NULL)
		{
			switch (shell->types[4])
			{
			case 0:
			case 1:
				break;
			default:
				cb_spiral_star(self, shell, -1.0f, -3.0f);   /* 0x00268ab0 */
				/* fall through */
			case 4:
				cb_spiral_star(self, shell, 1.0f, 3.0f);     /* 0x0025384c */
				/* fall through */
			case 3:
				cb_spiral_star(self, shell, -1.0f, -0.2f);   /* 0x00268aac */
				/* fall through */
			case 2:
				cb_spiral_star(self, shell, 1.0f, 0.2f);     /* 0x00257c7c */
				break;
			}
		}
		next = shell->next;
		if (burst)
		{
			*link = next;
			shell->next = self->freeShells;
			self->freeShells = shell;
		}
		else
		{
			link = &shell->next;
		}
		shell = next;
	}
}

/* 0x000d0f40  CarryOn/CFire_Fireworks.cpp (file inferred)
One step of everything. */
void CFire_Fireworks_Update(struct CFire_Fireworks *self)
{
	if (self->stars != NULL)
		CFire_FWorkStars_Update(self->stars);
	if (self->stars2 != NULL)
		CFire_FWorkStars_Update(self->stars2);
	if (self->smoke != NULL)
		CFire_FWorkSmoke_Update(self->smoke);
	if (self->glitter != NULL)
		CFire_Glitter_Update(self->glitter);
	if (self->sparks != NULL)
		CFire_Sparks_Update(self->sparks, 1.0f);
	if (self->clFireworks != NULL)
		CB_DUAL(FUN_0007fdc0, void (*)(int, undefined4), void (*)(void *, float),
			((int)(uintptr_t)self->clFireworks, cb_fbits(1.0f)), (self->clFireworks, 1.0f));
	CFire_Fireworks_UpdateRockets(self);
	CFire_Fireworks_UpdateShells(self);
}
