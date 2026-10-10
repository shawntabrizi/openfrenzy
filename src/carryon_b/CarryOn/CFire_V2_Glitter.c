/*
CarryOn/CFire_V2_Glitter.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_V2_Glitter.cpp

The glitter world of CarryOn (variant 2, one object at DAT_002f7968): Roman candles shoot
from the arena edges, glitter falls into a bowl, each player brings the rockets they carry
to their glow pot, and the glow pot fills with the glitter that the player's rockets make.
A rotating pair of spot lights, four beam lights and a flash light light the show.

0x000cc870 and 0x000ccb50 are tagged with CFire_V2_Glitter.cpp, 0x000cc940-0x000cca90 lie
between them. 0x000cbe30-0x000cc530 (before) and 0x000cce70-0x000cd4a0 (after) work on the
same object (file inferred).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_V2_Glitter.cpp"

/* storage: carryon_a's of_co_data2 / of_co_bss2 (gen_carryon_data.py); declarations only */
/* 0x002f7968  the glitter world */
extern struct CFire_V2Glitter *DAT_002f7968;
/* 0x002f796c  a slowly running phase (0-100) */
extern float DAT_002f796c;

/* 0x002943e0  the glow pot models of the six characters: "objects\\glowpot_dub.obe",
   "..._fly", "..._gee", "..._zak", "..._jade", "..._jet" (0x00268864-0x002687e8) */
extern const char *const DAT_002943e0[6];

/* the character of a player (0-5) */
#define CB_CHARACTER(player) (DAT_002baed4[(player) * 10])

/* 0x000cbe30  CarryOn/CFire_V2_Glitter.cpp (file inferred)
Constructor: everything 0; the object becomes DAT_002f7968. */
struct CFire_V2Glitter *CFire_V2Glitter_ctor(struct CFire_V2Glitter *self)
{
	DAT_002f7968 = self;
	memset(self, 0, 0x128);
	return self;
}

/* 0x000cbe50  CarryOn/CFire_V2_Glitter.cpp (file inferred)
Draws the Roman candles, the glitter, the sparks, the trails and the falling bits. */
void CFire_V2Glitter_Draw(struct CFire_V2Glitter *self)
{
	struct CFire_V2Bit *bit;

	bdSetWorldMatrix((float *)DAT_002855f0);
	bdSetRenderState(3, 0, 0);
	if (self->romans != NULL)
		CFire_Romans_Draw(self->romans);
	if (self->glitter != NULL)
		CFire_Glitter_Draw(self->glitter);
	if (self->sparks != NULL)
		CFire_Sparks_Draw(self->sparks);
	if (self->trails != NULL)
		CFire_Trails_Draw(self->trails);
	if (self->used == NULL)
		return;
	bdSetWorldMatrix((float *)DAT_002855f0);
	PointSprites_Begin((struct CPointSprites *)self->points);
	for (bit = self->used; bit != NULL; bit = bit->next)
		CB_CALL(FUN_0007f3d0, void (*)(void *, float *, unsigned int))(self->points, bit->pos,
			DAT_002bd640[bit->rgb[2]] | DAT_002bc490[bit->rgb[1]] | DAT_002bce18[bit->rgb[0]] |
			DAT_002bc898[127]);   /* DAT_002bca94 */
	CFire_PointSprites_Flush(self->points);
}

/* 0x000cbf20  CarryOn/CFire_V2_Glitter.cpp (file inferred)
Shows the four players' scores (in hundreds) on the HUD bars. */
void CFire_V2Glitter_ShowScores(void)
{
	CB_DUAL(FUN_0007c080, void (*)(int, float, undefined4, undefined4, undefined4, float),
		void (*)(void *, float, float, float, float, float),
		((int)(uintptr_t)DAT_002ecfac, (float)CB_I32(DAT_002f6d28[0], 0x1134) * 0.01f,
			cb_fbits((float)CB_I32(DAT_002f6d28[1], 0x1134) * 0.01f),
			cb_fbits((float)CB_I32(DAT_002f6d28[2], 0x1134) * 0.01f),
			cb_fbits((float)CB_I32(DAT_002f6d28[3], 0x1134) * 0.01f), 1.0f),
		(DAT_002ecfac, (float)CB_I32(DAT_002f6d28[0], 0x1134) * 0.01f,
			(float)CB_I32(DAT_002f6d28[1], 0x1134) * 0.01f,
			(float)CB_I32(DAT_002f6d28[2], 0x1134) * 0.01f,
			(float)CB_I32(DAT_002f6d28[3], 0x1134) * 0.01f, 1.0f));
}

/* 0x000cbf90  CarryOn/CFire_V2_Glitter.cpp (file inferred)
Makes the lights: four beam spots (off), the two rotating spots and the flash light
above the arena centre (off). */
void CFire_V2Glitter_InitLights(struct CFire_V2Glitter *self)
{
	struct CFire_Light *light;
	float pos[4];
	int i;

	for (i = 0; i < 4; i++)
	{
		light = CFire_Lights_AddSpotDefault(DAT_002f7988, 2, 0.785398126f, 0x7f, 0x7f, 0x7f,
			0.0005f, 0);
		self->beamLights[i] = light;
		CFire_Lights_SetFlags(DAT_002f7988, light, 2, 0);
	}
	self->spots[0] = CFire_Lights_AddPointDefault(DAT_002f7988, 0x10, 0, 0x7f, 0x7f, 0.01f, 0);
	self->spots[1] = CFire_Lights_AddPointDefault(DAT_002f7988, 0x10, 0, 0x7f, 0x7f, 0.01f, 0);
	pos[1] = DAT_002943d0[1] + 80.0f;                   /* 0x0026556c */
	pos[0] = DAT_002943d0[0];
	pos[2] = DAT_002943d0[2];
	pos[3] = DAT_002943d0[3];
	self->flash = CFire_Lights_AddPoint(DAT_002f7988, 0x10, pos, 0, 0, 0, 0.0003f, 0);
	CFire_Lights_SetFlags(DAT_002f7988, self->flash, 2, 0);
	CFire_Lights_Enable(DAT_002f7988, self->flash, 0);
}

/* 0x000cc090  CarryOn/CFire_V2_Glitter.cpp (file inferred)
A burst of `count` glitter bits at pos and a falling beam light there (carryon_a). */
void CFire_V2Glitter_Beam(struct CFire_V2Glitter *self, int unused, float *pos, int count)
{
	struct CFire_GlitterBit *bit;

	(void)unused;
	for (; count > 0; count--)
	{
		bit = CFire_Glitter_Emit(self->glitter, pos, 0.2f, 20.0f, 3.0f);
		if (bit != NULL)
			bit->vel[1] = bit->vel[1] + 4.0f;          /* 0x00257c40 */
	}
	self->beamTime[0] = 90.0f;
	self->beamPos[0][0] = pos[0];
	self->beamPos[0][1] = pos[1];
	self->beamPos[0][2] = pos[2];
	self->beamPos[0][3] = pos[3];
	self->beamVel[0][0] = 0.0f;
	self->beamVel[0][1] = -1.5f;
	self->beamVel[0][2] = 0.0f;
	self->beamVel[0][3] = 1.0f;
	CFire_Lights_SetFlags(DAT_002f7988, self->beamLights[0], 2, 2);
	CFire_Lights_Enable(DAT_002f7988, self->beamLights[0], 1);
}

/* 0x000cc150  CarryOn/CFire_V2_Glitter.cpp (file inferred)
Moves the beam lights (they point down from 1000 above and fade in 90 time units),
turns the two spots around the arena centre and fades the flash light. */
void CFire_V2Glitter_UpdateLights(struct CFire_V2Glitter *self, float dt)
{
	float target[4];
	float pos[4];
	float s, c;
	float f;
	int i;
	int colour;

	for (i = 0; i < 4; i++)
	{
		if (!(0.0f < self->beamTime[i]))
			continue;
		self->beamTime[i] = self->beamTime[i] - dt;
		target[1] = 0.0f;
		self->beamPos[i][0] = self->beamVel[i][0] + self->beamPos[i][0];
		self->beamPos[i][1] = self->beamVel[i][1] + self->beamPos[i][1];
		self->beamPos[i][2] = self->beamVel[i][2] + self->beamPos[i][2];
		target[0] = self->beamPos[i][0];
		target[2] = self->beamPos[i][2];
		target[3] = self->beamPos[i][3];
		pos[1] = self->beamPos[i][1] + 1000.0f;         /* 0x00255898 */
		pos[0] = target[0];
		pos[2] = target[2];
		pos[3] = target[3];
		CFire_Lights_SetSpot(DAT_002f7988, self->beamLights[i], pos, target);
		/* 127 at 90 time units left (0x002687d8: 1.4111) */
		CFire_Lights_SetColour(DAT_002f7988, self->beamLights[i],
			(int)(self->beamTime[i] * 1.41111112f), (int)(self->beamTime[i] * 1.41111112f), 0);
		if (self->beamTime[i] < 0.0f)
		{
			CFire_Lights_SetFlags(DAT_002f7988, self->beamLights[i], 2, 0);
			CFire_Lights_Enable(DAT_002f7988, self->beamLights[i], 0);
		}
	}
	f = dt * 0.1f + self->spin;                         /* 0x002536cc */
	self->spin = f;
	if (6.28318501f < f)                                   /* 0x00262300 */
	{
		f = self->spin;
		do
			f = f - 6.28318501f;
		while (6.28318501f < f);
		self->spin = f;
	}
	s = (float)sin(self->spin);
	c = (float)cos(self->spin);
	pos[3] = DAT_002943d0[3];
	pos[0] = DAT_002943d0[0] + s * 300.0f;              /* 0x00254f14 */
	pos[2] = DAT_002943d0[2] + c * 300.0f;
	pos[1] = DAT_002943d0[1] + 160.0f;                  /* 0x002550e0 */
	CFire_Lights_SetPos(DAT_002f7988, self->spots[0], pos);
	pos[0] = DAT_002943d0[0] - s * 300.0f;
	pos[3] = DAT_002943d0[3];
	pos[2] = DAT_002943d0[2] - c * 300.0f;
	pos[1] = DAT_002943d0[1] + 160.0f;
	CFire_Lights_SetPos(DAT_002f7988, self->spots[1], pos);
	if (!(0.0f < self->flashTime))
		return;
	f = self->flashTime - dt * 0.02f;                   /* 0x0025588c */
	self->flashTime = f;
	if (f <= 0.0f)
	{
		self->flashTime = 0.0f;
		CFire_Lights_SetFlags(DAT_002f7988, self->flash, 2, 0);
		CFire_Lights_Enable(DAT_002f7988, self->flash, 0);
	}
	colour = CB_CHARACTER(self->flashPlayer) * 0x60;
	CFire_Lights_SetColour(DAT_002f7988, self->flash,
		(int)((float)DAT_00290718[colour] * self->flashTime),
		(int)((float)DAT_00290719[colour] * self->flashTime),
		(int)((float)DAT_0029071a[colour] * self->flashTime));
}

/* 0x000cc470  CarryOn/CFire_V2_Glitter.cpp (file inferred)
The player brought `amount` rockets to the glow pot: a flash in their colour, and that
much glitter to drop. */
void CFire_V2Glitter_AddGlitter(struct CFire_V2Glitter *self, int player, int amount)
{
	int colour;

	self->flashTime = 1.0f;
	self->flashPlayer = player;
	CFire_Lights_SetFlags(DAT_002f7988, self->flash, 2, 2);
	colour = CB_CHARACTER(player) * 0x60;
	CFire_Lights_SetColour(DAT_002f7988, self->flash, DAT_00290718[colour] << 1,
		DAT_00290719[colour] << 1, DAT_0029071a[colour] << 1);
	self->pending[player] = self->pending[player] + amount;
	CFire_Lights_Enable(DAT_002f7988, self->flash, 1);
}

/* 0x000cc510  CarryOn/CFire_V2_Glitter.cpp (file inferred)
The Roman candle nearest to pos (x/z). */
struct CFire_Roman *CFire_V2Glitter_NearestRoman(struct CFire_V2Glitter *self, float *pos)
{
	return CFire_Romans_Nearest(self->romans, pos);
}

/* 0x000cc520  CarryOn/CFire_V2_Glitter.cpp (file inferred) */
int CFire_V2Glitter_PickRomans(struct CFire_V2Glitter *self, int unused, struct CFire_Roman *prefer,
	struct CFire_Roman **out)
{
	return CFire_Romans_Pick(self->romans, unused, prefer, out);
}

/* 0x000cc530  CarryOn/CFire_V2_Glitter.cpp (file inferred)
Launches a Roman candle from a random arena edge towards the inside, in a random colour
that is not too red. Only when its landing point (after its flight, slowed by 0.99 each
step) is on the ground between 800 and 2000. Returns 1 when launched. */
int CFire_V2Glitter_LaunchRoman(struct CFire_V2Glitter *self)
{
	float r, g, b;
	float pos[4];
	float vel[4];
	float land[4];
	float fuse;
	float steps;
	float sum;
	float term;
	float ground;
	int n;

	if (self->romans == NULL)
		return 0;
	CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 5, -1, 0x80);
	do
	{
		CFire_RandomColour(&r, &g, &b);
		if (r <= b)
			break;
	} while (b < g);
	pos[1] = 0.0f;
	switch (cb_rand() & 3)
	{
	case 0:
		pos[1] = 150.0f;
		pos[0] = DAT_00292c60 + 15.0f;                  /* 0x002553a0 */
		pos[2] = Core_RandUnit() * 2000.0f - 1000.0f;   /* 0x00257c68, 0x00255898 */
		vel[0] = Core_RandUnit() * 15.0f + 5.0f;        /* 0x002536c4 */
		vel[2] = 0.0f;
		break;
	case 1:
		pos[0] = (DAT_00292c68 + DAT_00292c60) - 15.0f;
		pos[2] = Core_RandUnit() * 2000.0f - 1000.0f;
		vel[0] = -(Core_RandUnit() * 15.0f) - 5.0f;
		vel[2] = 0.0f;
		break;
	case 2:
		pos[1] = 150.0f;
		pos[2] = (DAT_00292c6c + DAT_00292c64) - 15.0f;
		pos[0] = Core_RandUnit() * 2000.0f - 1000.0f;
		vel[2] = -(Core_RandUnit() * 15.0f) - 5.0f;
		vel[0] = 0.0f;
		break;
	case 3:
		pos[2] = DAT_00292c64 + 15.0f;
		pos[0] = Core_RandUnit() * 2000.0f - 1000.0f;
		vel[2] = Core_RandUnit() * 15.0f + 5.0f;
		vel[0] = 0.0f;
		break;
	}
	vel[1] = (float)cb_rand() * 9.15555283e-05f + 13.0f;  /* 0x002687e4, 0x002668e8 */
	fuse = (float)cb_rand() * 0.00305185094f + 100.0f;     /* 0x00253784, 0x00255544 */
	vel[3] = 1.0f;
	/* the flight steps: the fuse - 45, or the steps to the top of the arc (y speed x 6.67) */
	steps = fuse - 45.0f;                               /* 0x00257c30 */
	if (vel[1] * 6.66666651f < steps)                   /* 0x002687e0 */
		steps = vel[1] * 6.66666651f;
	sum = 1.0f;
	term = 1.0f;
	for (n = (int)steps; n > 0; n--)
	{
		sum = sum + term;
		term = term * 0.99f;                            /* 0x002687dc */
	}
	land[1] = 1000.0f;
	land[0] = vel[0] * sum + pos[0];
	land[3] = 1.0f;
	land[2] = sum * vel[2] + pos[2];
	ground = CLColl_GroundProbe(land, 0.0f, NULL, NULL, 0);
	if (!(800.0f <= ground && ground <= 2000.0f))       /* 0x002686f8, 0x00257c68 */
		return 0;
	pos[3] = 1.0f;
	CFire_Romans_Launch(self->romans, pos, vel, -0.15f, 100.0f, r, g, b, fuse);
	return 1;
}

/* 0x000cc870  CarryOn/CFire_V2_Glitter.cpp
Constructor of a player's glow pot at pos; its model (the player's character) stands at
modelPos. */
struct CFire_GlowPot *CFire_GlowPot_ctor(struct CFire_GlowPot *self, float *pos, int player,
	float *modelPos)
{
	const char *name;
	struct BActorInstance *actor;

	memset(self, 0, 0x34);
	name = DAT_002943e0[CB_CHARACTER(player)];
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->player = player;
	self->radius = 300.0f;
	CB_PTR(DAT_002f6d28[player], 0x11a0) = self;
	self->model = bLoadResource(*(struct bkPackage **)DAT_002f6d5c, name, 1, 0xdefa);
	if (self->model != NULL)
	{
		actor = baInitActorInstance(NULL, (struct BActor *)self->model, 2, CB_FILE, 0x3c5);
		self->actor = actor;
		CB_F32(actor, 0xc0) = modelPos[0];
		CB_F32(actor, 0xc4) = modelPos[1];
		CB_F32(actor, 0xc8) = modelPos[2];
		CB_F32(actor, 0xcc) = modelPos[3];
		self->fill = baFindSubObject(self->actor, "fill");      /* 0x0026887c */
	}
	return self;
}

/* 0x000cc940  CarryOn/CFire_V2_Glitter.cpp (bracketed)
When the player stands at the pot with rockets (and is not stunned) the rockets go in;
the fill level is the pot's glitter against the most of all pots (at least 500). */
void CFire_GlowPot_Update(struct CFire_GlowPot *self, float unused)
{
	void *player;
	float most;
	float f;
	float r;
	float dx, dz;
	int carried;
	int i;

	(void)unused;
	player = DAT_002f6d28[self->player];
	most = 500.0f;
	if (500.0f < (float)DAT_002f7968->pots[0]->score)  /* 0x002686fc */
		most = (float)DAT_002f7968->pots[0]->score;
	for (i = 1; i < 4; i++)
	{
		f = (float)DAT_002f7968->pots[i]->score;
		if (most < f)
			most = f;
	}
	carried = CB_I32(player, 0x1134);
	if (carried != 0 && CB_F32(player, 0x113c) == 0.0f)
	{
		r = self->radius;
		if (DAT_002baed8[self->player * 10] == 0)
			r = r + 20.0f;                              /* 0x00255530 */
		dx = CB_F32(CB_PTR(player, 0x30), 0xd0) - self->pos[0];
		dz = CB_F32(CB_PTR(player, 0x30), 0xd8) - self->pos[2];
		if (dx * dx + dz * dz < r * r)
		{
			CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 8, 0, 0x80);
			CB_CALL(FUN_000c0c40, void (*)(void *, int))(player, carried);
			CB_CALL(FUN_000c0c60, void (*)(void *, int, unsigned int))(player, carried,
				0xffffffffu);
			CB_I32(player, 0x1134) = CB_I32(player, 0x1134) - carried;
			CFire_V2Glitter_AddGlitter(DAT_002f7968, self->player, carried);
		}
	}
	f = (float)self->score / most;
	self->fillFrac2 = f;
	self->fillFrac = f;
}

/* 0x000cca90  CarryOn/CFire_V2_Glitter.cpp (bracketed)
Draws the pot (debug: its catch circle) with the fill animation at its fill level. */
void CFire_GlowPot_Draw(struct CFire_GlowPot *self, int passes)
{
	struct BActorInstance *actor;
	int colour;
	int start;

	if (DAT_002f6d60 != 0)
	{
		colour = CB_CHARACTER(self->player) * 0x60;
		CB_DUAL(FUN_000c8230, void (*)(undefined4, undefined4, undefined4, undefined4),
			void (*)(float *, float, float, unsigned int),
			((undefined4)(uintptr_t)self->pos, cb_fbits(self->radius), cb_fbits(10.0f),
				(unsigned int)DAT_00290719[colour] << 9 | (unsigned int)DAT_0029071a[colour] << 1 |
				(unsigned int)DAT_00290718[colour] << 17 | 0xff000000u),
			(self->pos, self->radius, 10.0f,
				(unsigned int)DAT_00290719[colour] << 9 | (unsigned int)DAT_0029071a[colour] << 1 |
				(unsigned int)DAT_00290718[colour] << 17 | 0xff000000u));
	}
	actor = self->actor;
	if (actor == NULL || self->fill == NULL)
		return;
	start = CB_I32(self->fill, 4);
	baSetNodeTime(actor, CB_ACTOR_ROOT(actor),
		(int)((float)(CB_I32(self->fill, 8) - start) * self->fillFrac) + start, 1);
	baPrepActorInstance(self->actor, 1);
	baDrawActorInstance(self->actor, passes);
}

/* 0x000ccb50  CarryOn/CFire_V2_Glitter.cpp
Makes the Roman candles, glitter, sparks, trails, the four glow pots, the lights, the dish
and 1000 falling bits with their point sprites. */
void CFire_V2Glitter_Init(struct CFire_V2Glitter *self)
{
	struct bkResource *texture;
	struct CFire_GlowPot *prev;
	struct CFire_GlowPot *pot;
	struct CFire_Sparks *sparks;
	struct CFire_Trails *trails;
	struct CPointSprites *points;
	struct BActorInstance *actor;
	char *arena;
	int i;
	int off;

	self->glitter = bkHeapAlloc(0x118, CB_FILE, 0x51, 0x1001);
	CFire_Glitter_Init(self->glitter, 30000, 45.0f);
	self->romans = bkHeapAlloc(0x14, CB_FILE, 0x54, 0x1001);
	CFire_Romans_Init(self->romans, 0x32);
	sparks = bkHeapAlloc(0x20, CB_FILE, 0x59, 0x1001);
	if (sparks != NULL)
		sparks = CFire_Sparks_ctor(sparks);
	self->sparks = sparks;
	texture = bLoadResource(NULL, "alphatex\\firework_glow.bmp", 0, 0xdefa);
	CFire_Sparks_Init(self->sparks, texture, 2000);
	trails = bkHeapAlloc(0x10, CB_FILE, 0x5e, 0x1001);
	if (trails != NULL)
		trails = (struct CFire_Trails *)Meta_ReturnThis(trails);
	self->trails = trails;
	CFire_Trails_Init(trails, texture, 100);
	for (i = 0, off = 0; off < 0x40; i++, off += 0x10)
	{
		prev = self->potList;
		pot = bkHeapAlloc(0x34, CB_FILE, 0x66, 0x1001);
		if (pot != NULL)
			pot = CFire_GlowPot_ctor(pot, DAT_002943d0, i,
				(float *)((char *)DAT_00293ae8 + 0x2378 + off));
		self->potList = pot;
		pot->next = prev;
		self->pots[i] = self->potList;
	}
	CFire_V2Glitter_InitLights(self);
	CLHud_InitBarGeometry((int)(uintptr_t)DAT_002ecfac);
	self->dishModel = bLoadResource(*(struct bkPackage **)DAT_002f6d5c, "objects\\thedish.obe", 1,
		0xdefa);
	if (self->dishModel != NULL)
	{
		actor = baInitActorInstance(NULL, (struct BActor *)self->dishModel, 2, CB_FILE, 0x74);
		self->dish = actor;
		arena = DAT_00293ae8;
		CB_F32(actor, 0xc0) = CB_F32(arena, 0x23b8);
		CB_F32(actor, 0xc4) = CB_F32(arena, 0x23bc);
		CB_F32(actor, 0xc8) = CB_F32(arena, 0x23c0);
		CB_F32(actor, 0xcc) = CB_F32(arena, 0x23c4);
		self->dishy = baFindSubObject(self->dish, "dishy");
		baHideNode(baFindNode(CB_ACTOR_ROOT(self->dish), "dishglow"), 1);
	}
	self->bits = bkHeapAlloc(40000, CB_FILE, 0x86, 0x2001);
	memset(self->bits, 0, 40000);
	self->used = NULL;
	self->free = self->bits;
	for (i = 0; i < 999; i++)
		self->bits[i].next = &self->bits[i + 1];
	self->bits[999].next = NULL;
	texture = bLoadResource(NULL, "alphatex\\firework_glitter.bmp", 0, 0xdefa);
	points = bkHeapAlloc(0x9c, CB_FILE, 0x95, 0x1001);
	if (points != NULL)
		points = PointSprites_Init(points, 2, (int)(uintptr_t)texture, 1000,
			(int)cb_fbits(45.0f));
	self->points = (struct CFire_PointSprites *)points;
}

/* 0x000cce70  CarryOn/CFire_V2_Glitter.cpp (file inferred)
One frame: sometimes a Roman candle, players collect glitter, glow pots, candles, glitter,
sparks, trails and lights; the falling bits bounce in the bowl and fill their player's
pot when they fade or fall out; each player's pending glitter drops at most 3 a frame. */
void CFire_V2Glitter_Update(struct CFire_V2Glitter *self, float dt)
{
	struct CFire_GlowPot *pot;
	struct CFire_V2Bit **link;
	struct CFire_V2Bit *bit;
	struct CFire_V2Bit *next;
	void *player;
	float x, y, z, vy;
	float dx, dz, d;
	float radius;
	float s, nx, nz, dot;
	unsigned char r, g, b;
	int launched;
	int tries;
	int old;
	int i;
	int n;
	int character;

	self->frame = self->frame + 1;
	DAT_002f796c = dt * 0.0625f + DAT_002f796c;         /* 0x00268914 */
	if (100.0f < DAT_002f796c)
		DAT_002f796c = 0.0f;
	if (DAT_00293b44 == 4 && CFire_Romans_Count(self->romans) < 5 && (cb_rand() & 0x1f) == 0)
	{
		launched = 0;
		tries = 0;
		do
		{
			if (launched != 0)
				break;
			launched = CFire_V2Glitter_LaunchRoman(self);
			tries++;
		} while (tries < 4);
	}
	for (i = 0; i < CB_I32(DAT_002f6d5c, 0x254); i++)
	{
		player = DAT_002f6d28[i];
		if (CB_F32(player, 0x1138) == 0.0f)
		{
			old = CB_I32(player, 0x1134);
			CFire_Glitter_Collect(self->glitter, CB_FP(CB_PTR(player, 0x30), 0xd0), 100.0f, 100.0f,
				CB_ADDR(player, 0x1134));
			if (old != CB_I32(DAT_002f6d28[i], 0x1134))
				CB_F32(DAT_002f6d28[i], 0x113c) = 3.0f;
		}
	}
	for (pot = self->potList; pot != NULL; pot = pot->next)
		CFire_GlowPot_Update(pot, dt);
	if (self->romans != NULL)
		CFire_Romans_Update(self->romans, dt, self->glitter, self->sparks);
	if (self->glitter != NULL)
		CFire_Glitter_Update(self->glitter);
	if (self->sparks != NULL)
		CFire_Sparks_Update(self->sparks, 1.0f);
	if (self->trails != NULL)
		CFire_Trails_Update(self->trails, 1.0f);
	CFire_V2Glitter_UpdateLights(self, dt);
	CLHud_AddSeconds((int)(uintptr_t)DAT_002ecfac, dt);

	link = &self->used;
	bit = self->used;
	while (bit != NULL)
	{
		vy = bit->vel[1] - 0.2f;                        /* 0x00257c7c */
		x = bit->pos[0];
		bit->pos[0] = x + bit->vel[0];
		y = bit->pos[1];
		bit->pos[1] = vy + y;
		z = bit->pos[2];
		bit->pos[2] = z + bit->vel[2];
		r = (unsigned char)((bit->rgb[0] * 99u) / 100u);
		g = (unsigned char)((bit->rgb[1] * 99u) / 100u);
		bit->rgb[1] = g;
		b = (unsigned char)((bit->rgb[2] * 99u) / 100u);
		bit->vel[1] = vy;
		bit->rgb[0] = r;
		bit->rgb[2] = b;
		/* the bowl: radius 100 at the centre's height, growing by half the depth below */
		radius = 100.0f - (DAT_002943d0[1] - (vy + y)) * 0.5f;
		dx = (x + bit->vel[0]) - DAT_002943d0[0];
		dz = (z + bit->vel[2]) - DAT_002943d0[2];
		d = dx * dx + dz * dz;
		if (radius * radius <= d)
		{
			s = 1.0f / sqrtf(d);
			bit->pos[0] = s * dx * radius + DAT_002943d0[0];
			bit->pos[2] = s * dz * radius + DAT_002943d0[2];
			nx = s * dx * -0.707f;                      /* 0x00268910 */
			nz = s * dz * -0.707f;
			dot = vy * 0.707f + nz * bit->vel[2] + nx * bit->vel[0];   /* 0x0026890c */
			if (dot < 0.0f)
			{
				bit->bounced = 1.0f;
				bit->vel[0] = bit->vel[0] - nx * dot * 1.9f;           /* 0x00268908 */
				bit->vel[1] = vy - 1.3433f * dot;                       /* 0x00268904 */
				bit->vel[2] = bit->vel[2] - dot * nz * 1.9f;
			}
		}
		next = bit->next;
		if ((b == 0 && g == 0 && r == 0) || bit->pos[1] < DAT_002943d0[1] - 150.0f)
		{
			self->pots[bit->player]->score = self->pots[bit->player]->score + 1;
			*link = bit->next;
			bit->next = self->free;
			self->free = bit;
			bit = next;
			continue;
		}
		link = &bit->next;
		bit = next;
	}

	for (i = 0; i < 4; i++)
	{
		if (self->pending[i] == 0)
			continue;
		n = 3;
		if (self->pending[i] <= 2)
		{
			n = self->pending[i];
			if (n <= 0)
				continue;
		}
		character = CB_CHARACTER(i);
		do
		{
			bit = self->free;
			if (bit != NULL)
			{
				self->pending[i]--;
				self->free = bit->next;
				bit->next = self->used;
				self->used = bit;
				bit->player = (unsigned char)i;
				bit->rgb[0] = DAT_00290718[character * 0x60];
				bit->rgb[1] = DAT_00290719[character * 0x60];
				bit->rgb[2] = DAT_0029071a[character * 0x60];
				do
				{
					bit->pos[0] = (float)cb_rand() * 0.0054933317f - 90.0f;   /* 0x00268900, 0x00255180 */
					bit->pos[2] = (float)cb_rand() * 0.0054933317f - 90.0f;
				} while (8100.0f < bit->pos[2] * bit->pos[2] + bit->pos[0] * bit->pos[0]);
				bit->pos[1] = DAT_002943d0[1];
				bit->pos[0] = DAT_002943d0[0] + bit->pos[0];
				bit->vel[0] = 0.0f;
				bit->vel[1] = 0.0f;
				bit->vel[2] = 0.0f;
				bit->pos[2] = DAT_002943d0[2] + bit->pos[2];
			}
			n--;
		} while (n != 0);
	}
}

/* 0x000cd370  CarryOn/CFire_V2_Glitter.cpp (file inferred)
Draws the dish lit only by light group 0x10 (its glow animation follows the flash), then
the glow pots additively. */
void CFire_V2Glitter_DrawDish(struct CFire_V2Glitter *self, int passes)
{
	struct BActorInstance *dish;
	struct CFire_GlowPot *pot;
	int start;

	if (self->dish != NULL)
	{
		CFire_Lights_EnableGroup(DAT_002f7988, 2, 0);
		CFire_Lights_EnableGroup(DAT_002f7988, 1, 0);
		CFire_Lights_EnableGroup(DAT_002f7988, 8, 0);
		CFire_Lights_EnableGroup(DAT_002f7988, 4, 0);
		CFire_Lights_EnableGroup(DAT_002f7988, 0x10, 1);
		bdSetRenderState(3, 1, 0);
		if (self->dishy != NULL)
		{
			dish = self->dish;
			start = CB_I32(self->dishy, 4);
			baSetNodeTime(dish, CB_ACTOR_ROOT(dish),
				(int)((float)(CB_I32(self->dishy, 8) - start) * self->flashTime) + start, 1);
		}
		baPrepActorInstance(self->dish, 1);
		baDrawActorInstance(self->dish, passes);
		CFire_Lights_EnableGroup(DAT_002f7988, 0x10, 0);
		bdSetRenderState(3, 0, 0);
	}
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(1);
	for (pot = self->potList; pot != NULL; pot = pot->next)
		CFire_GlowPot_Draw(pot, passes);
}

/* 0x000cd4a0  CarryOn/CFire_V2_Glitter.cpp (file inferred)
Frees everything Init made. */
void CFire_V2Glitter_Free(struct CFire_V2Glitter *self)
{
	struct CFire_GlowPot *pot;
	struct CFire_Trails *trails;
	struct CFire_PointSprites *points;

	DAT_002f7968 = NULL;
	pot = self->potList;
	while (pot != NULL)
	{
		self->potList = pot->next;
		if (pot->actor != NULL)
			baDeleteActorInstance(pot->actor);
		if (pot->model != NULL)
			bkDeleteResource(pot->model);
		bkHeapFree(pot);
		pot = self->potList;
	}
	if (self->romans != NULL)
	{
		CFire_Pool_Free((struct CFire_Pool *)self->romans);
		bkHeapFree(self->romans);
		self->romans = NULL;
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
	if (self->trails != NULL)
	{
		CFire_Trails_Free(self->trails);
		trails = self->trails;
		if (trails != NULL)
		{
			FUN_001d5950();    /* an empty destructor (folded) */
			bkHeapFree(trails);
		}
		self->trails = NULL;
	}
	if (self->dish != NULL)
		baDeleteActorInstance(self->dish);
	if (self->dishModel != NULL)
		bkDeleteResource(self->dishModel);
	points = self->points;
	if (points != NULL)
	{
		CFire_PointSprites_Free(points);
		bkHeapFree(points);
		self->points = NULL;
		bkHeapFree(self->bits);
		self->bits = NULL;
	}
}
