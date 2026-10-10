/*
CarryOn/CFire_Arena.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Arena.cpp

The CarryOn arena (one object, DAT_002f7970): the rockets that the spawners at the arena
edges throw in (or that drop from the sky), their shadows, and the players' launch pads.
A player carries a rocket to the pad; with a rocket in every slot the pad launches them
as one firework shell, and the player scores by the mix of rocket types.

0x000cd5c0, 0x000ce140, 0x000ce3a0 and 0x000ce420 are tagged with CFire_Arena.cpp,
0x000cd720-0x000ce130 lie between them; 0x000ce490-0x000cef40 work on the same objects
(file inferred).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_Arena.cpp"

/* Storage: the globals below lie in carryon_a's blocks of_co_data2 / of_co_bss2 (original
   layout and XBE values, src/carryon_a/tools/gen_carryon_data.py); carryon_a owns them and
   these are declarations only (integration round 1: carryon_b had its own copies). */

/* 0x002f7970  the arena */
extern struct CFire_Arena *DAT_002f7970;
/* 0x002f7984  the shadow vertex being written */
extern float *DAT_002f7984;

/* 0x002943f8  the launch pad models of the six characters: "objects\\launch_dub.obe",
   "..._fly", "..._geena", "..._zak", "..._jade", "..._jet" (0x00268994-0x00268918) */
extern const char *const DAT_002943f8[6];

/* 0x00294410  the score of a launch by its rockets: [c0 + 4 * (c1 + 4 * c3)] with cN the
   number of rockets of type N (type 2 does not count); nonzero: [3] 50, [6] 30, [9] 18,
   [12] 35, [18] 25, [21] 5, [24] 12, [33] 15, [36] 10, [48] 20 */
extern int DAT_00294410[64];

/* 0x00294510  where the rocket models sit on a pad's node (0, 0, 0, 1) */
extern float DAT_00294510[4];

/* 0x00294520-0x00294578 and 0x002f7974-0x002f7980: the rocket types a spawner throws
   (scripted, DAT_00294110), by parts a firework and direction (DAT_00294608). Values:
   0x00294520 {4}, 0x00294524 {3, 2}, 0x0029452c / 34 / 3c {0, 1}, 0x00294544 {4, 3},
   0x0029454c / 54 {0, 1}, 0x0029455c / 68 {3, 3, 1}, 0x00294574 / 78 {1}; the four .bss
   lists 0x002f7974-0x002f7980 {0}. */
/* 0x0029457c, 0x00294594, 0x002945b4, 0x002945d8: random rocket types for 2-5 types
   {0,0,0,1,1,1}, {..,3,3}, {..,3,3,4}, {0,0,0,1,1,1,2,2,3,3,4} */
extern const int DAT_0029457c[6];
extern const int DAT_00294594[8];
extern const int DAT_002945b4[9];
extern const int DAT_002945d8[11];
/* 0x00294608  the lists above: 7980, 797c, 4578, 4574, 4568, 455c, 7978, 7974, 4554,
   454c, 4544, 453c, 4534, 452c, 4524, 4520 */
extern int *DAT_00294608[16];
/* 0x00294648  the number of entries of each list {1,1,1,1,3,3,1,1,2,2,2,2,2,2,2,1} */
extern int DAT_00294648[16];

/* the player in slot i (CCarryOn_Player of carryon_a) */
#define CB_PLAYER(i) (DAT_002f6d28[i])
/* a player's position (its body's +0xd0) */
#define CB_PLAYER_POS(p) CB_FP(CB_PTR(p, 0x30), 0xd0)

/* out = a * b: the aligned or the unaligned matrix product, as the original chooses by the
   addresses (both give the same product) */
static void cb_matrix_multiply(float *out, const float *a, const float *b)
{
	if ((((uintptr_t)out | (uintptr_t)a | (uintptr_t)b) & 0xf) == 0)
		bmMatrixMultiplyCopy(out, a, b);
	else
		bmMatrixMultiply(out, a, b);
}

/* 0x000cd5c0  CarryOn/CFire_Arena.cpp
Constructor of a player's launch pad at pos: its model (the player's character) stands
150 behind and 30 above. */
struct CFire_Pad *CFire_Pad_ctor(struct CFire_Pad *self, float *pos, int player,
	struct CFire_Fireworks *fireworks)
{
	struct BActorInstance *actor;

	memset(self, 0, 0x68);
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->player = player;
	self->radius = 100.0f;
	self->count = 0;
	self->fireworks = fireworks;
	CB_PTR(CB_PLAYER(player), 0x119c) = self;
	self->model = bLoadResource(*(struct bkPackage **)DAT_002f6d5c,
		DAT_002943f8[DAT_002baed4[player * 10]], 1, 0xdefa);
	actor = baInitActorInstance(NULL, (struct BActor *)self->model, 2, CB_FILE, 0x3f);
	self->actor = actor;
	CB_F32(actor, 0xc0) = pos[0];
	CB_F32(actor, 0xc4) = pos[1];
	CB_F32(actor, 0xc8) = pos[2];
	CB_F32(actor, 0xcc) = pos[3];
	CB_F32(self->actor, 0xc8) = CB_F32(self->actor, 0xc8) - 150.0f;   /* 0x00262df8 */
	CB_F32(self->actor, 0xc4) = CB_F32(self->actor, 0xc4) + 30.0f;    /* 0x002553a8 */
	self->top = baFindNode(CB_ACTOR_ROOT(self->actor), "c");
	self->slots[0] = baFindNode(CB_ACTOR_ROOT(self->actor), "a");
	self->slots[1] = baFindNode(CB_ACTOR_ROOT(self->actor), "b");
	self->slots[2] = baFindNode(CB_ACTOR_ROOT(self->actor), "c");
	self->launchAnim = baFindSubObject(self->actor, "launch");
	return self;
}

/* 0x000cd720  CarryOn/CFire_Arena.cpp (bracketed)
The player puts the rocket they carry (one) into the next slot. */
void CFire_Pad_AddRocket(struct CFire_Pad *self)
{
	void *player = CB_PLAYER(self->player);
	int type;

	if (CB_I32(player, 0x1130) != 1 || !(self->count < DAT_00294108))
		return;
	type = CB_I32(player, 0x1158);
	self->types[self->count] = type;
	self->count++;
	CB_CALL(FUN_000c0d30, void (*)(void *, int, int))(player, 4, -1);
	self->lastType = type;
	self->addTime = 18.0f;
}

/* 0x000cd770  CarryOn/CFire_Arena.cpp (bracketed)
Launches the pad's rockets as one shell from its top: the player scores by the mix of
types and cheers (more when all rockets were of one type). */
void CFire_Pad_Launch(struct CFire_Pad *self)
{
	void *player = CB_PLAYER(self->player);
	struct CFire_Shell *shell;
	signed char counts[6];
	float pos[4];
	float vel[4];
	int score;
	int most;
	int c;
	int j;

	baGetNodePosition(self->actor, self->top, pos);
	vel[0] = (float)cb_rand() * 0.000305185094f - 5.0f;   /* 0x00265808, 0x002536c4 */
	vel[2] = (float)cb_rand() * 0.000305185094f - 5.0f;
	vel[1] = 34.0f;
	vel[3] = 0.0f;    /* not set by the original */
	most = 1;
	for (c = 0; c < 6; c++)
	{
		counts[c] = 0;
		for (j = 0; j < DAT_00294108; j++)
			if (self->types[j] == c)
				counts[c]++;
		if (most < counts[c])
			most = counts[c];
	}
	score = DAT_00294410[counts[0] + 4 * (counts[1] + 4 * counts[3])];
	CB_CALL(FUN_000c0c40, void (*)(void *, int))(player, score);
	CB_CALL(FUN_000c0c60, void (*)(void *, int, unsigned int))(player, score, 0xffffffffu);
	AI_PlayVoice(player, most < DAT_00294108 ? 4 : 10, DAT_00293928, 0.0f, 0, -1, 0);
	shell = CFire_Fireworks_AddShell(self->fireworks, pos, vel, counts[0], counts[1], counts[2],
		counts[3], counts[4], self->player);
	if (shell != NULL)
	{
		shell->fuse = shell->fuse * 0.75f;              /* 0x00253844 */
		shell->strength = (most * 4 - 4) / (DAT_00294108 - 1);
	}
	self->count = 0;
	*DAT_002f7960 = 1;
}

/* 0x000cd940  CarryOn/CFire_Arena.cpp (bracketed)
A player at the pad adds their rocket; with all slots full the launch animation starts
and 10 steps later the rockets go up. */
void CFire_Pad_Update(struct CFire_Pad *self, float dt)
{
	void *player = CB_PLAYER(self->player);
	float dx, dz, t;

	if (self->addTime != 0.0f)
	{
		t = self->addTime - dt;
		self->addTime = t;
		if (t <= 0.0f)
			self->addTime = 0.0f;
	}
	if (self->launchDelay == 0)
	{
		dx = CB_PLAYER_POS(player)[0] - self->pos[0];
		dz = CB_PLAYER_POS(player)[2] - self->pos[2];
		if (dx * dx + dz * dz < self->radius * self->radius)
			CFire_Pad_AddRocket(self);
		if (DAT_00294108 <= self->count)
		{
			self->launchDelay = 10;
			baQueueAnimation(self->actor, CB_ACTOR_ROOT(self->actor), (struct BAnim *)self->launchAnim,
				0x400, -1, 0, NULL, NULL);
		}
	}
	else
	{
		self->launchDelay--;
		if (self->launchDelay == 0)
			CFire_Pad_Launch(self);
	}
	baUpdateAnimations(self->actor, CB_ACTOR_ROOT(self->actor), dt, NULL);
	baPrepActorInstance(self->actor, 1);
}

/* 0x000cda30  CarryOn/CFire_Arena.cpp (bracketed)
Draws the pad: the rocket just added flies into place from above, the others sit on
their slot nodes (debug: the pad's circle). */
void CFire_Pad_Draw(struct CFire_Pad *self, unsigned int passes)
{
	float trans[16];
	float rot[16];
	float tmp[16];
	float dir[4];
	float q[4];
	float swap;
	float x, y, z;
	struct BActorInstance *model;
	char *node;
	int colour;
	int i;

	if (self->count != 0 && (passes & 1) != 0)
		CB_CALL(FUN_000c8170, void (*)(int))(0x40);
	if (self->addTime != 0.0f)
	{
		memcpy(trans, DAT_002855f0, sizeof trans);
		memcpy(rot, DAT_002855f0, sizeof rot);
		/* the flight direction (0, 60, 20) as (x, z, -y) */
		{
			static const float from[4] = { 0.0f, 60.0000038f, 20.0f, 0.0f };   /* 0x00268a10 */

			bmVectorSetLength(dir, from, 1.0f);
		}
		swap = -dir[1];
		dir[1] = dir[2];
		dir[2] = swap;
		bmQuaternionFromDirection(q, dir, 0.0f);
		if (18.0f <= self->addTime)                     /* 0x0026872c */
		{
			bdSetWorldMatrix((float *)DAT_002855f0);
			bmQuaternionToMatrix(rot, q);
			x = 0.0f + self->pos[0] + 0.0f;             /* 0x002689ec, 0x00268a10 */
			y = 118.0f + self->pos[1] + 60.0000038f;
			z = -74.0f + self->pos[2] + 20.0f;
		}
		else
		{
			bdSetWorldMatrix((float *)DAT_002855f0);
			bmQuaternionToMatrix(rot, q);
			x = 0.0f * self->addTime * 0.055555556f + self->pos[0] + 0.0f;
			y = 60.0000038f * self->addTime * 0.055555556f + self->pos[1] + 118.0f;
			z = 20.0f * self->addTime * 0.055555556f + self->pos[2] + -74.0f;
		}
		model = CB_AT(DAT_002f6d5c, 0x298 + self->lastType * 4, struct BActorInstance *);
		baPrepActorInstance(model, 1);
		bmMatrixTranslation(trans, x, y, z);
		cb_matrix_multiply((float *)model, trans, rot);
		bmMatrixToQuaternion(CB_FP(model, 0xe0), rot);
		CB_F32(model, 0xc0) = x;
		CB_F32(model, 0xc4) = y;
		CB_F32(model, 0xc8) = z;
		baDrawActorInstance(model, (int)passes);
	}
	for (i = 0; i < self->count; i++)
	{
		if (i == self->count - 1 && !(self->addTime <= 0.0f))
			continue;
		model = CB_AT(DAT_002f6d5c, 0x2e4 + self->types[i] * 4, struct BActorInstance *);
		node = (char *)self->slots[i];
		bdSetWorldMatrix((float *)DAT_002855f0);
		CB_F32(model, 0xc0) = DAT_00294510[0];
		CB_F32(model, 0xc4) = DAT_00294510[1];
		CB_F32(model, 0xc8) = DAT_00294510[2];
		CB_F32(model, 0xcc) = DAT_00294510[3];
		baPrepActorInstance(model, 1);
		if ((((uintptr_t)node | (uintptr_t)model | (uintptr_t)tmp) & 0xf) == 0)
			bmMatrixMultiplyCopy(tmp, (float *)(node + 0x80), (float *)model);
		else
			bmMatrixMultiply(tmp, (float *)(node + 0x80), (float *)model);
		cb_matrix_multiply((float *)model, (float *)self->actor, tmp);
		baDrawActorInstance(model, (int)passes);
	}
	if (self->count != 0 && (passes & 1) != 0)
		CB_CALL(FUN_000c8200, void (*)(void))();
	if (DAT_00294118 < 1)
	{
		colour = DAT_002baed4[self->player * 10] * 0x60;
		CB_DUAL(FUN_000c8230, void (*)(undefined4, undefined4, undefined4, undefined4),
			void (*)(float *, float, float, unsigned int),
			((undefined4)(uintptr_t)self->pos, cb_fbits(self->radius), cb_fbits(10.0f),
				(unsigned int)DAT_00290719[colour] << 9 | (unsigned int)DAT_0029071a[colour] << 1 |
				(unsigned int)DAT_00290718[colour] << 17 | 0xff000000u),
			(self->pos, self->radius, 10.0f,
				(unsigned int)DAT_00290719[colour] << 9 | (unsigned int)DAT_0029071a[colour] << 1 |
				(unsigned int)DAT_00290718[colour] << 17 | 0xff000000u));
		return;
	}
	baDrawActorInstance(self->actor, (int)passes);
}

/* the rocket type a spawner throws next */
static int cb_spawner_type(struct CFire_Spawner *self, float angle)
{
	int index;

	if (DAT_00294110 == 0)
	{
		if (DAT_0029410c == 2)
			return DAT_0029457c[cb_rand() % 6];
		if (DAT_0029410c == 3)
			return DAT_00294594[cb_rand() % 8];
		if (DAT_0029410c == 4)
			return DAT_002945b4[cb_rand() % 9];
		return DAT_002945d8[cb_rand() % 0xb];
	}
	switch (DAT_0029410c)
	{
	case 2:
		index = self->dir;
		break;
	case 3:
		index = self->dir + 4;
		break;
	case 4:
		index = self->dir + 8;
		break;
	case 5:
		index = self->dir + 0xc;
		break;
	default:
		/* the original indexes with the bits of the throw angle (a slot it reuses) */
		memcpy(&index, &angle, sizeof index);
		break;
	}
	if (DAT_00294648[index] == 1)
		return DAT_00294608[index][0];
	return DAT_00294608[index][cb_rand() % DAT_00294648[index]];
}

/* 0x000cde00  CarryOn/CFire_Arena.cpp (bracketed)
Every 30 steps (while fewer than DAT_00294124 rockets fly) the spawner throws a free
rocket at one of 8 angles that is not in use, unless there are enough of its type. */
void CFire_Spawner_Update(struct CFire_Spawner *self, struct CFire_Rocket *rockets, int *active)
{
	struct CFire_Rocket *rocket;
	int counts[6];
	float angle;
	unsigned int slot;
	int type;
	int i;

	if (0.0f < self->timer)
		self->timer = self->timer - 1.0f;
	for (i = 0; i < 8; i++)
	{
		if (self->slots[i] != NULL &&
			(self->slots[i]->state < 2 || 7 < self->slots[i]->state))
			self->slots[i] = NULL;
	}
	if (!(self->timer <= 0.0f) || !(*active < DAT_00294124))
		return;
	self->timer = 30.0f;
	slot = (unsigned int)cb_rand() & 7;
	if (self->slots[slot] != NULL)
		return;
	angle = ((float)slot - 3.5f) * 0.196349531f;           /* 0x00268a24, 0x00268a20 */
	if (self->dir == 0)
		angle = angle + 2.3561945f;                       /* 0x002657fc */
	else if (self->dir == 1)
		angle = angle - 2.3561945f;
	else
		angle = angle - 3.1415925f;                       /* 0x00256750 */
	memset(counts, 0, sizeof counts);
	for (rocket = rockets; rocket != NULL; rocket = rocket->next)
		if (1 < rocket->state && rocket->state < 7)
			counts[rocket->type]++;
	if (rockets == NULL)
		return;
	for (rocket = rockets; rocket->state != 0; )
	{
		rocket = rocket->next;
		if (rocket == NULL)
			return;
	}
	self->slots[slot] = rocket;
	type = cb_spawner_type(self, angle);
	if (counts[type] < DAT_00294128[type])
	{
		CFire_Rocket_Throw(rocket, self->pos, angle, 10.0f, 30.0f, type,
			CB_AT(DAT_002f6d5c, 0x298 + type * 4, struct BActorInstance *));
		*active = *active + 1;
	}
}

/* 0x000ce110  CarryOn/CFire_Arena.cpp (bracketed)
Constructor: everything 0; the arena becomes DAT_002f7970. */
struct CFire_Arena *CFire_Arena_ctor(struct CFire_Arena *self)
{
	memset(self, 0, 0x1c);
	DAT_002f7970 = self;
	return self;
}

/* 0x000ce130  CarryOn/CFire_Arena.cpp (bracketed) */
void CFire_Arena_dtor(struct CFire_Arena *self)
{
	(void)self;
	DAT_002f7970 = NULL;
}

/* 0x000ce140  CarryOn/CFire_Arena.cpp
Makes the rockets (variant 0: 20, variant 2: 80; none otherwise) and their shadow quads. */
void CFire_Arena_Init(struct CFire_Arena *self, int variant, struct CFire_Fireworks *fireworks)
{
	struct CFire_Rocket *prev;
	struct CFire_Rocket *rocket;
	unsigned int colour;
	float *v;
	int n;
	int i;

	self->variant = variant;
	self->fireworks = fireworks;
	self->rockets = NULL;
	self->pads = NULL;
	self->spawners = NULL;
	if (variant == 0)
		n = 0x14;
	else if (variant == 2)
		n = 0x50;
	else
		return;
	self->rockets = NULL;
	for (i = 0; i < n; i++)
	{
		prev = self->rockets;
		rocket = bkHeapAlloc(0x3c, CB_FILE, variant == 0 ? 0x377 : 0x38c, 0x1001);
		if (rocket != NULL)
			rocket = CFire_Rocket_ctor(rocket, 0);
		self->rockets = rocket;
		rocket->next = prev;
	}
	self->shadows = bkHeapAlloc(n * 0xd8, CB_FILE, 0x398, 0x2001);
	self->shadowTexture = bLoadResource(NULL, "alphatex\\fwork_shadow.bmp", 0, 0xdefa);
	v = self->shadows;
	for (i = 0; i < n; i++)
	{
		colour = DAT_002bc898[255] | DAT_002bd640[128] | DAT_002bc490[128] | DAT_002bce18[128];
		/* 6 vertices of 9 floats: xyz, normal, colour, uv */
		CB_U32(v, 0x18) = colour;
		CB_U32(v, 0x3c) = colour;
		CB_U32(v, 0x60) = colour;
		CB_U32(v, 0x84) = colour;
		CB_U32(v, 0xa8) = colour;
		CB_U32(v, 0xcc) = colour;
		CB_F32(v, 0x1c) = 0.0f;
		CB_F32(v, 0x20) = 0.0f;
		CB_F32(v, 0x40) = 0.0f;
		CB_F32(v, 0x44) = 0.0f;
		CB_F32(v, 0x64) = 1.0f;
		CB_F32(v, 0x68) = 0.0f;
		CB_F32(v, 0x88) = 0.0f;
		CB_F32(v, 0x8c) = 1.0f;
		CB_F32(v, 0xac) = 1.0f;
		CB_F32(v, 0xb0) = 1.0f;
		CB_F32(v, 0xd0) = 1.0f;
		CB_F32(v, 0xd4) = 1.0f;
		v = (float *)((char *)v + 0xd8);
	}
}

/* 0x000ce3a0  CarryOn/CFire_Arena.cpp
Adds a player's launch pad. */
void CFire_Arena_AddPad(struct CFire_Arena *self, float *pos, int player)
{
	struct CFire_Pad *prev = self->pads;
	struct CFire_Pad *pad;

	pad = bkHeapAlloc(0x68, CB_FILE, 0x3c4, 0x1001);
	if (pad != NULL)
		pad = CFire_Pad_ctor(pad, pos, player, self->fireworks);
	self->pads = pad;
	pad->next = prev;
}

/* 0x000ce420  CarryOn/CFire_Arena.cpp
Adds a spawner at the arena edge (dir: its throw direction). */
void CFire_Arena_AddSpawner(struct CFire_Arena *self, float *pos, int dir)
{
	struct CFire_Spawner *prev = self->spawners;
	struct CFire_Spawner *spawner;

	spawner = bkHeapAlloc(0x3c, CB_FILE, 0x3d2, 0x1001);
	if (spawner == NULL)
	{
		/* the original stores the old head through the NULL pointer */
		self->spawners = NULL;
		*(struct CFire_Spawner *volatile *)&((struct CFire_Spawner *volatile)spawner)->next = prev;
		return;
	}
	memset(spawner, 0, 0x3c);
	spawner->pos[0] = pos[0];
	spawner->pos[1] = pos[1];
	spawner->pos[2] = pos[2];
	spawner->pos[3] = pos[3];
	spawner->dir = dir;
	self->spawners = spawner;
	spawner->next = prev;
}

/* 0x000ce490  CarryOn/CFire_Arena.cpp (file inferred)
Draws the rockets (lit by light group 4) and the pads (group 8); debug: the spawners. */
void CFire_Arena_Draw(struct CFire_Arena *self, unsigned int passes)
{
	struct CFire_Rocket *rocket;
	struct CFire_Pad *pad;
	struct CFire_Spawner *spawner;

	if (passes & 1)
		CB_CALL(FUN_000c8170, void (*)(int))(0x40);
	CFire_Lights_EnableGroup(DAT_002f7988, 2, 0);
	CFire_Lights_EnableGroup(DAT_002f7988, 1, 0);
	CFire_Lights_EnableGroup(DAT_002f7988, 8, 0);
	CFire_Lights_EnableGroup(DAT_002f7988, 4, 1);
	bdSetRenderState(3, 1, 0);
	for (rocket = self->rockets; rocket != NULL; rocket = rocket->next)
		CFire_Rocket_Draw(rocket, passes);
	if (passes & 1)
		CB_CALL(FUN_000c8200, void (*)(void))();
	CFire_Lights_EnableGroup(DAT_002f7988, 2, 0);
	CFire_Lights_EnableGroup(DAT_002f7988, 4, 0);
	CFire_Lights_EnableGroup(DAT_002f7988, 1, 0);
	CFire_Lights_EnableGroup(DAT_002f7988, 8, 1);
	bdSetRenderState(3, 1, 0);
	for (pad = self->pads; pad != NULL; pad = pad->next)
		CFire_Pad_Draw(pad, passes);
	for (spawner = self->spawners; spawner != NULL; spawner = spawner->next)
	{
		if (DAT_002f6d60 != 0)
			CB_DUAL(FUN_000c8230, void (*)(undefined4, undefined4, undefined4, undefined4),
				void (*)(float *, float, float, unsigned int),
				((undefined4)(uintptr_t)spawner->pos, cb_fbits(100.0f), cb_fbits(10.0f), 0xff00ff00u),
				(spawner->pos, 100.0f, 10.0f, 0xff00ff00u));
	}
}

/* 0x000ce5c0  CarryOn/CFire_Arena.cpp (file inferred)
Draws a shadow under every rocket in the air (up to 500 above its ground; the higher,
the smaller). */
void CFire_Arena_DrawShadows(struct CFire_Arena *self)
{
	struct CFire_Rocket *rocket = self->rockets;
	float *v;
	float h, size, x1, x2, y, z1, z2;

	if (rocket == NULL)
		return;
	v = self->shadows;
	do
	{
		DAT_002f7984 = v;
		if (1 < rocket->state && rocket->state < 8)
		{
			h = rocket->pos[1] - rocket->groundY;
			if (h < 500.0f && 0.0f <= h)               /* 0x002686fc */
			{
				DAT_002f7984 = v + 0x36;
				size = (1.0f - h * 0.002f) * 50.0f;     /* 0x002672b0, 0x002559b8 */
				x1 = rocket->pos[0] - size;
				y = rocket->groundY + 5.0f;
				z1 = rocket->pos[2] - size;
				z2 = rocket->pos[2] + size;
				x2 = rocket->pos[0] + size;
				v[0x00] = x1;
				v[0x01] = y;
				v[0x02] = z1;
				v[0x09] = x1;
				v[0x0a] = y;
				v[0x0b] = z1;
				v[0x12] = x1;
				v[0x13] = y;
				v[0x14] = z2;
				v[0x1b] = x2;
				v[0x1c] = y;
				v[0x1d] = z1;
				v[0x24] = x2;
				v[0x25] = y;
				v[0x26] = z2;
				v[0x2d] = x2;
				v[0x2e] = y;
				v[0x2f] = z2;
			}
		}
		rocket = rocket->next;
		v = DAT_002f7984;
	} while (rocket != NULL);
	if (DAT_002f7984 == self->shadows)
		return;
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(5, 0, 0);
	bdSetWorldMatrix((float *)DAT_002855f0);
	Blend_SetMode(3);
	bdSetRenderState(6, 1, 0);
	bdSetTexture(0, (struct bdTexture *)self->shadowTexture);
	bdDrawPrimitiveFVF(4, self->shadows, (int)((char *)DAT_002f7984 - (char *)self->shadows) / 0x24,
		0x152);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(3, 0, 0);
}

/* 0x000ce7a0  CarryOn/CFire_Arena.cpp (file inferred)
One step: (variant 0) only DAT_002943b4 rockets are in use and in phases 3-4 one may drop
from the sky; the rockets and pads move; in phases 3 and 4 and above 5 the spawners act
in a shuffled order. */
void CFire_Arena_Update(struct CFire_Arena *self, float dt, int phase)
{
	struct CFire_Rocket *rocket;
	struct CFire_Pad *pad;
	struct CFire_Spawner *spawner;
	struct CFire_Spawner *order[20];
	struct CFire_Spawner *tmp;
	int active;
	int n;
	int a, b;
	int i;

	if (self->variant == 0)
	{
		i = 0;
		for (rocket = self->rockets; rocket != NULL; rocket = rocket->next)
		{
			if (i < DAT_002943b4)
			{
				if (rocket->state == 8)
					rocket->state = 0;
			}
			else
			{
				rocket->state = 8;
			}
			i++;
		}
		if (2 < phase && phase < 5 && (float)cb_rand() * 3.0518509e-05f < DAT_00294120)
		{
			for (rocket = self->rockets; rocket != NULL; rocket = rocket->next)
			{
				if (rocket->state == 0)
				{
					CFire_Rocket_Drop(rocket, CB_AT(DAT_002f6d5c, 0x298, struct BActorInstance *));
					break;
				}
			}
		}
	}
	active = 0;
	for (rocket = self->rockets; rocket != NULL; rocket = rocket->next)
	{
		CFire_Rocket_Update(rocket, dt);
		if (1 < rocket->state && rocket->state < 7)
			active++;
	}
	for (pad = self->pads; pad != NULL; pad = pad->next)
		CFire_Pad_Update(pad, dt);
	if (!(2 < phase) || phase == 5)
		return;
	spawner = self->spawners;
	if (spawner == NULL)
		return;
	n = 0;
	do
	{
		order[n] = spawner;
		spawner = spawner->next;
		n++;
	} while (spawner != NULL);
	for (i = 0; i < 10; i++)
	{
		a = cb_rand() % n;
		b = cb_rand() % n;
		tmp = order[a];
		order[a] = order[b];
		order[b] = tmp;
	}
	for (i = 0; i < n; i++)
		CFire_Spawner_Update(order[i], self->rockets, &active);
}

/* the player (other than `except`, human) that holds the rocket, 4 when none */
static int cb_rocket_holder(struct CFire_Rocket *rocket, int except)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (i != except && DAT_002baed8[i * 10] == 0 && CB_PTR(CB_PLAYER(i), 0x1188) == rocket)
			break;
	}
	return i;
}

/* 0x000cec70  CarryOn/CFire_Arena.cpp (file inferred)
1 when a rocket flies that no other human player holds (player -1: any rocket flies). */
int CFire_Arena_AnyFreeRocket(struct CFire_Arena *self, int player)
{
	struct CFire_Rocket *rocket;

	for (rocket = self->rockets; rocket != NULL; rocket = rocket->next)
	{
		if (2 < rocket->state && rocket->state < 7)
		{
			if (cb_rocket_holder(rocket, player) == 4 || player == -1)
				return 1;
		}
	}
	return 0;
}

/* 0x000cece0  CarryOn/CFire_Arena.cpp (file inferred)
The nearest flying rocket that no other human player holds (player -1: any). */
struct CFire_Rocket *CFire_Arena_NearestFreeRocket(struct CFire_Arena *self, float *pos, int player)
{
	struct CFire_Rocket *rocket;
	struct CFire_Rocket *best = NULL;
	float bestDist = 1e10f;
	float d;

	for (rocket = self->rockets; rocket != NULL; rocket = rocket->next)
	{
		if (!(2 < rocket->state && rocket->state < 7))
			continue;
		if (cb_rocket_holder(rocket, player) == 4 || player == -1)
		{
			d = (rocket->pos[0] - pos[0]) * (rocket->pos[0] - pos[0]) +
				(rocket->pos[1] - pos[1]) * (rocket->pos[1] - pos[1]) +
				(rocket->pos[2] - pos[2]) * (rocket->pos[2] - pos[2]);
			if (d < bestDist)
			{
				best = rocket;
				bestDist = d;
			}
		}
	}
	return best;
}

/* 0x000cedb0  CarryOn/CFire_Arena.cpp (file inferred)
A random rocket that lies on the ground (state 6), NULL when none. */
struct CFire_Rocket *CFire_Arena_RandomLyingRocket(struct CFire_Arena *self)
{
	struct CFire_Rocket *rocket;
	int n = 0;

	rocket = self->rockets;
	if (rocket == NULL)
		return NULL;
	do
	{
		if (rocket->state == 6)
			n++;
		rocket = rocket->next;
	} while (rocket != NULL);
	if (n == 0)
		return NULL;
	if (n == 1)
		n = 0;
	else
		n = cb_rand() % n;
	for (rocket = self->rockets; rocket != NULL; rocket = rocket->next)
	{
		if (rocket->state == 6)
		{
			if (n == 0)
				return rocket;
			n--;
		}
	}
	return NULL;
}

/* 0x000cee20  CarryOn/CFire_Arena.cpp (file inferred)
A random rocket of the type that lies on the ground, NULL when none. */
struct CFire_Rocket *CFire_Arena_RandomLyingRocketOfType(struct CFire_Arena *self, int type)
{
	struct CFire_Rocket *rocket;
	int n = 0;

	rocket = self->rockets;
	if (rocket == NULL)
		return NULL;
	do
	{
		if (rocket->state == 6 && rocket->type == type)
			n++;
		rocket = rocket->next;
	} while (rocket != NULL);
	if (n == 0)
		return NULL;
	if (n == 1)
		n = 0;
	else
		n = cb_rand() % n;
	for (rocket = self->rockets; rocket != NULL; rocket = rocket->next)
	{
		if (rocket->state == 6 && rocket->type == type)
		{
			if (n == 0)
				return rocket;
			n--;
		}
	}
	return NULL;
}

/* 0x000ceea0  CarryOn/CFire_Arena.cpp (file inferred)
Frees the rockets, the pads (and their models), the spawners and the shadows. */
void CFire_Arena_Free(struct CFire_Arena *self)
{
	struct CFire_Rocket *rocket;
	struct CFire_Pad *pad;
	struct CFire_Spawner *spawner;

	rocket = self->rockets;
	while (rocket != NULL)
	{
		self->rockets = rocket->next;
		FUN_001d5950();    /* the rocket's empty destructor (folded) */
		bkHeapFree(rocket);
		rocket = self->rockets;
	}
	pad = self->pads;
	while (pad != NULL)
	{
		self->pads = pad->next;
		if (pad->actor != NULL)
			baDeleteActorInstance(pad->actor);
		if (pad->model != NULL)
			bkDeleteResource(pad->model);
		bkHeapFree(pad);
		pad = self->pads;
	}
	spawner = self->spawners;
	while (spawner != NULL)
	{
		self->spawners = spawner->next;
		bkHeapFree(spawner);
		spawner = self->spawners;
	}
	if (self->shadows != NULL)
	{
		bkHeapFree(self->shadows);
		self->shadows = NULL;
	}
}

/* 0x000cef40  CarryOn/CFire_Arena.cpp (file inferred) */
void CFire_Arena_DrawShadowPass(struct CFire_Arena *self)
{
	bdSetRenderState(3, 0, 0);
	CFire_Arena_DrawShadows(self);
}
