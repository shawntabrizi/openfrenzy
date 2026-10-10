/*
CarryOn/unplaced.c

Gap functions of the carryon_a range without a known source file.

0x000ba6d0 lies between the last CharLib function (0x000ba470) and CFire_Game.cpp. Other
minigames call it every frame too (0x00126220, 0x0014ea80, 0x00173f80), so it is most
likely the end of a CharLib file.
*/

#include "carryon_a.h"

/* 0x000ba6d0  (file not known; CharLib?)
Updates every item of the list at +0xc of the route context (DAT_002ea488) with charlib
0x000ba470, which returns the node to continue from. */
void FUN_000ba6d0(void)
{
	unsigned int *list = (unsigned int *)((char *)DAT_002ea488 + 0xc);
	unsigned int node;

	node = CLList_Next(list, NULL);
	while (node != 0)
		node = CLList_Next(list, (unsigned int *)co_RouteItem_Update((void *)node));
}

/* 0x000c39b0  CarryOn (file not known; between CCarryOn_Player.cpp and CFire_ASEImport.cpp)
Makes a route context current for charlib's AI: saves the previous tables and context
(DAT_002f6d44, DAT_002f6d48), clears the AI player table and six more charlib slots and
sets the route grid size. Seven other minigames call it too (meta_a, mousewhack, foxhunt,
hopper, demolition_b), so it keeps the table prototype (32-bit values). */
void FUN_000c39b0(undefined4 context, undefined4 tables, int columns, int rows)
{
	int i;

	DAT_002f6d44 = DAT_002f27d8;
	DAT_002f6d48 = DAT_002ea488;
	DAT_002ea488 = (void *)(uintptr_t)context;
	DAT_002f27d8 = (void *)(uintptr_t)tables;
	for (i = 0; i < DAT_002bae94; i++)
		DAT_002f289c[i] = NULL;
	for (i = 0; i < 6; i++)
		DAT_002f2878[i] = NULL;
	DAT_00292c4c = columns;
	DAT_00292c50 = rows;
	DAT_00292c78 = columns * rows;
}

/*
The conveyor belts of variant 4 ("Carnival Knowledge"; struct CFireBelt) and the pieces
on the fuse and paint paths (struct CoConveyorItem), 0x000c9490-0x000ca060. The file is
not known; carryon_b's CFire_Conveyor.cpp starts right after (0x000ca230) and owns the
piece objects, so these are probably the start of that file.
*/

/* a slot's place on the belt: (slot - pos) wrapped into [0, count) */
static float co_BeltPlace(const struct CFireBelt *belt, int slot)
{
	float n = (float)belt->count;
	float p;

	for (p = (float)slot - belt->pos; n <= p; p = p - n)
		;
	for (; p < 0.0f; p = p + n)
		;
	return p;
}

/* the point at `f` (0..1) along the belt's path */
static void co_BeltPoint(const struct CFireBelt *belt, float f, float *out)
{
	const float *pts = belt->path->points;

	out[0] = (pts[4] - pts[0]) * f + pts[0];
	out[1] = (pts[5] - pts[1]) * f + pts[1];
	out[2] = (pts[6] - pts[2]) * f + pts[2];
	out[3] = 1.0f;
}

/* 0x000c9490  (file not known)
The constructor: clears the belt. */
void *CFire_Belt_Ctor(void *self)
{
	memset(self, 0, 0xcc * 4);
	return self;
}

/* 0x000c94b0  (file not known)
Sets up a belt with `count` slots (a `window` of them visible) on `path`, moving `speed`
slots per frame. The slots start filled with the types 1..4 in turn from a random one. */
void CFire_Belt_Init(void *self, void *fireworks, void *path, float speed, int points,
	int count, int window)
{
	struct CFireBelt *belt = self;
	unsigned int type;
	int i;

	belt->fireworks = fireworks;
	belt->points = points;
	belt->window = window;
	belt->count = count;
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	type = (DAT_00285630 >> 16) & 3;
	for (i = 0; i < count; i++)
	{
		type++;
		belt->types[i] = (int)type;
		type &= 3;
	}
	belt->path = path;
	belt->speed = speed;
	belt->pos = 0.0f;
}

/* 0x000c9520  (file not known)
Moves the belt. With a chance of 20/256 a new firework of a random type appears in the
empty slot entering the window (when fewer than two of that type are on the belt). Passed
fireworks arrive when their timers run out. */
void CFire_Belt_Update(void *self, float step)
{
	struct CFireBelt *belt = self;
	float n;
	int slot;
	int type;
	int same;
	int i;

	belt->pos = step * belt->speed + belt->pos;
	n = (float)belt->count;
	if (n <= belt->pos)
	{
		do
			belt->pos = belt->pos - n;
		while ((float)belt->count <= belt->pos);
	}
	if (belt->pos < 0.0f)
	{
		do
			belt->pos = belt->pos + n;
		while (belt->pos < 0.0f);
	}
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	if (((DAT_00285630 >> 16) & 0xff) < 0x14)
	{
		for (slot = (int)belt->pos - (belt->count - belt->window) / 2; slot < 0;
			slot += belt->count)
			;
		if (belt->types[slot] == 0)
		{
			DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
			type = (int)((DAT_00285630 >> 16) & 3) + 1;
			same = 0;
			for (i = 0; i < belt->count; i++)
				if (belt->types[i] == type)
					same++;
			if (same < 2)
			{
				bkPrintf("    === Swawned a type %d ===\n", type);
				belt->types[slot] = type;
			}
		}
	}
	for (i = 0; i < belt->count; i++)
	{
		if (belt->types[i] == 0)
		{
			belt->pending[i] = 0;
			belt->timers[i] = 0.0f;
		}
		else if (belt->timers[i] != 0.0f)
		{
			belt->timers[i] = belt->timers[i] - step;
			if (belt->timers[i] <= 0.0f)
			{
				belt->types[i] = belt->pending[i];
				belt->timers[i] = 0.0f;
			}
		}
	}
}

/* 0x000c96a0  (file not known)
1 when a firework of `type` (-1: any) is within `width` slots around the place `at`
(0..1 along the window). */
int CFire_Belt_HasItemAt(void *self, float at, float width, int type)
{
	struct CFireBelt *belt = self;
	int count = belt->count;
	float lo;
	float p;
	int i;

	lo = (float)belt->window * at - width * 0.5f;
	for (i = 0; i < count; i++)
	{
		if (belt->types[i] == type || (type == -1 && belt->types[i] != 0))
		{
			p = co_BeltPlace(belt, i);
			if (p < (float)belt->window && lo <= p && p <= lo + width)
				return 1;
		}
	}
	return 0;
}

/* 0x000c97a0  (file not known)
Draws the fireworks in the window: the owner's rack rocket model (by character), or a
debug circle in the type's colour without one. Debug lines show the path. */
void CFire_Belt_Draw(void *self)
{
	struct CFireBelt *belt = self;
	float point[4];
	float p;
	void *inst;
	int type;
	int i;

	if (DAT_002f6d60 != 0)
		CFire_Resource_DrawLines(belt->path->points, 2, 0, 0xffff0040);
	for (i = 0; i < belt->count; i++)
	{
		type = belt->types[i];
		if (type == 0)
			continue;
		p = co_BeltPlace(belt, i);
		if (!(p < (float)belt->window))
			continue;
		co_BeltPoint(belt, p / (float)belt->window, point);
		inst = g_fireRes->floorInst[DAT_002baed4[type - 1][0]];
		if (inst == NULL)
			CFire_Resource_DrawCircleFlags(point, 30.0f, 80.0f, 2, DAT_00294354[type]);
		else
		{
			CO_F32(inst, 0xc0) = point[0];
			CO_F32(inst, 0xc4) = point[1];
			CO_F32(inst, 0xc8) = point[2];
			CO_F32(inst, 0xcc) = 1.0f;
			CO_F32(inst, 0xc4) = CO_F32(inst, 0xc4) + 75.0f;
			baPrepActorInstance(inst, 1);
			baDrawActorInstance(inst, 7);
		}
	}
}

/* 0x000c9950  (file not known)
The place of x along the belt's path (0 at its first point, 1 at the second). */
float CFire_Belt_GetT(void *self, float x)
{
	const float *pts = ((struct CFireBelt *)self)->path->points;

	return (x - pts[0]) / (pts[4] - pts[0]);
}

/* 0x000c9970  (file not known)
A fuse reached the firework in `slot` at `pos`: it launches (carryon_b). Its owner
scores when `player` lit his own; another's firework is launched only with the tweak
"V4:Launch Anyones" (then "V4:GivePointsAway" gives the owner a point and a firework; a
team mate's gives the owner a point; else the launcher loses one). Returns 1 when it
launched. */
int CFire_Belt_Launch(struct CFireBelt *self, int slot, float *pos, int player)
{
	struct CCarryOnPlayer *owner;
	struct CCarryOnPlayer *launcher;
	float vel[4];
	int ownerIndex;
	int r1, r2, r3;

	ownerIndex = self->types[slot] - 1;
	owner = g_coPlayers[ownerIndex];
	launcher = g_coPlayers[player];
	if (ownerIndex == player)
	{
		CCarryOn_Player_AddScore(launcher, self->points);
		CCarryOn_Player_ShowScore(launcher, self->points, ownerIndex);
		AI_PlayVoice(launcher, 4, DAT_00293928, 0.0f, 0, -1, 0);
	}
	else
	{
		if (DAT_00294348 == 0)
			return 0;
		if (DAT_0029434c != 0)
		{
			CCarryOn_Player_AddScore(owner, 1);
			CCarryOn_Player_ShowScore(launcher, 1, ownerIndex);
			if (owner->fireworks < 5)
				owner->fireworks++;
			AI_PlayVoice(launcher, 0xe, DAT_00293928, 0.0f, 0, -1, 0);
		}
		else if (DAT_002baebc != 0)
		{
			if (DAT_002baed4[player][8] == DAT_002baed4[ownerIndex][8])
			{
				CCarryOn_Player_AddScore(owner, 1);
				CCarryOn_Player_ShowScore(launcher, 1, ownerIndex);
				AI_PlayVoice(launcher, 4, DAT_00293928, 0.0f, 0, -1, 0);
			}
			else
			{
				CCarryOn_Player_AddScore(launcher, -1);
				CCarryOn_Player_ShowScore(launcher, -1, player);
				AI_PlayVoice(launcher, 0xe, DAT_00293928, 0.0f, 0, -1, 0);
			}
		}
		else
		{
			CCarryOn_Player_AddScore(launcher, -1);
			CCarryOn_Player_ShowScore(launcher, -1, player);
		}
	}
	r1 = co_rand();
	r2 = co_rand();
	r3 = co_rand();
	vel[0] = (float)r1 * 0.000305185094f - 5.0f;   /* 0x00265808, 0x002536c4 */
	vel[1] = (float)r2 * 0.000305185094f + 18.0f;  /* 0x0026872c */
	vel[2] = (float)r3 * 0.000305185094f - 5.0f;
	vel[3] = 0.0f;
	CFire_Fireworks_AddShellOfType(self->fireworks, pos, vel, DAT_0029436c[owner->scoreText],
		ownerIndex);
	owner->scoreText++;
	if (owner->scoreText == 9)
		owner->scoreText = 8;
	self->types[slot] = 0;
	return 1;
}

/* 0x000c9bd0  (file not known)
Paint reached the firework in `slot`: it passes to `player` (arrives after 18 frames)
with a stream of sparks in the player's colour along the belt. */
int CFire_Belt_Pass(struct CFireBelt *self, int slot, float *pos, int player, void *sparks)
{
	const float *pts;
	float vel[4];
	float dx;
	float length;
	int colour;

	self->timers[slot] = 18.0f;
	self->pending[slot] = player + 1;
	pts = self->path->points;
	dx = pts[0] - pts[4];
	length = sqrtf(dx * dx + (pts[1] - pts[5]) * (pts[1] - pts[5])
		+ (pts[2] - pts[6]) * (pts[2] - pts[6]));
	if (dx < 0.0f)
		length = -length;
	vel[1] = 0.0f;
	vel[2] = 0.0f;
	vel[3] = 1.0f;
	colour = DAT_002baed4[player][0] * 0x60;
	vel[0] = CO_F32(g_fireRes, 0xb4) / (float)self->window * self->speed * length
		* 1.5f;                                 /* 0x002538a0 */
	CFire_Sparks_EmitMoving(sparks, pos, vel, 0x50, (int)DAT_00290718[colour] << 1,
		(int)DAT_00290718[colour + 1] << 1, (int)DAT_00290718[colour + 2] << 1);
	return 1;
}

/* 0x000c9cd0  (file not known)
A fuse or paint head at `pos`: the first firework in the window within 80 units launches
(`pass` 0) or passes to `player`. Returns 1 when one did. */
int CFire_Belt_Hit(struct CFireBelt *self, float *pos, int player, int pass, void *sparks)
{
	float point[4];
	float p;
	int done;
	int i;

	for (i = 0; i < self->count; i++)
	{
		if (self->types[i] == 0)
			continue;
		p = co_BeltPlace(self, i);
		if (!(p < (float)self->window))
			continue;
		co_BeltPoint(self, p / (float)self->window, point);
		if ((pos[0] - point[0]) * (pos[0] - point[0]) + (pos[1] - point[1]) * (pos[1] - point[1])
			+ (pos[2] - point[2]) * (pos[2] - point[2]) < 6400.0f)
		{
			if (pass == 0)
				done = CFire_Belt_Launch(self, i, point, player);
			else
				done = CFire_Belt_Pass(self, i, point, player, sparks);
			if (done != 0)
				return 1;
		}
	}
	return 0;
}

/* 0x000c9e60  (file not known)
Starts a piece of `player` at the path's first point (fuse or paint), with the tail 12
frames behind, and plays sound kind 4. Does nothing while it runs. */
void CFire_ConveyorItem_Start(struct CoConveyorItem *self, int player, int paint)
{
	if (self->active != 0)
		return;
	self->player = player;
	self->paint = paint;
	self->active = 1;
	memcpy(self->pos, self->path->points, 4 * sizeof(float));
	self->point = 1;
	self->point2 = 1;
	self->delay = 12.0f;
	memcpy(self->pos2, self->path->points, 4 * sizeof(float));
	CFire_Resource_FindSample(g_fireRes, 4, -1, 0x80);
}

/* 0x000c9ee0  CarryOn (file not known; carryon_b CFire_Conveyor_Update calls it)
Moves a piece along its path. A fuse smokes and sparks; each frame step it tests the
belts (launch for a fuse, pass for paint) and stops when it hit (paint keeps going with
the tweak "V4:PaintKeepsGoing"). The tail follows after `delay`; the piece stops at the
path's end. */
void CFire_ConveyorItem_Update(struct CoConveyorItem *self, float step, void *smoke,
	void *sparks, struct CFireBelt **belts, int numBelts)
{
	float move;
	int colour;
	int i;

	if (self->active == 0)
		return;
	if (self->paint == 0)
	{
		CFire_FWorkSmoke_Add(smoke, self->pos, 3.0f, 20.0f);
		colour = DAT_002baed4[self->player][0] * 0x60;
		CFire_Sparks_Emit(sparks, self->pos, 4, (int)DAT_00290718[colour] << 1,
			(int)DAT_00290718[colour + 1] << 1, (int)DAT_00290718[colour + 2] << 1);
	}
	for (i = 0; i < numBelts; i++)
	{
		if (CFire_Belt_Hit(belts[i], self->pos, self->player, self->paint, sparks) != 0
			&& (self->paint == 0 || DAT_00294350 == 0))
			self->active = 0;
	}
	move = self->speed * step;
	if (CFire_Resource_MoveTowards(self->pos, self->path->points + self->point * 4, move) != 0)
	{
		if (self->point < self->path->numPoints - 1)
			self->point++;
		else
			self->active = 0;
	}
	if (0.0f < self->delay)
	{
		self->delay = self->delay - step;
		return;
	}
	self->delay = 0.0f;
	if (CFire_Resource_MoveTowards(self->pos2, self->path->points + self->point2 * 4, move) != 0)
	{
		if (self->point2 < self->path->numPoints - 1)
			self->point2++;
		else
			self->active = 0;
	}
}

/* 0x000ca060  CarryOn (file not known; carryon_b CFire_Conveyor_Draw calls it)
Draws a running paint piece as a beam from the head (40 wide, the player's colour
doubled) to the tail (20 wide, the colour), alpha blended. The strip gets a repeated first
and last vertex. */
void CFire_ConveyorItem_Draw(struct CoConveyorItem *self)
{
	int colour1[4];
	int colour2[4];
	float *vertices;
	float *end;
	int colour;
	int count;

	if (self->paint == 0 || self->active == 0 || self->pos[2] == self->pos2[2])
		return;
	self->pos2[3] = 1.0f;
	self->pos[3] = 1.0f;
	bdSetWorldMatrix(DAT_002855f0);
	bdSetTexture(0, self->texture);
	vertices = bdVertexBufferLock(self->vb, 4);
	if (vertices == NULL)
		return;
	colour = DAT_002baed4[self->player][0] * 0x60;
	colour1[0] = DAT_00290718[colour] * 2;
	colour2[0] = colour1[0] / 2;
	colour1[1] = DAT_00290718[colour + 1] * 2;
	colour2[1] = colour1[1] / 2;
	colour1[2] = DAT_00290718[colour + 2] * 2;
	colour2[2] = colour1[2] / 2;
	colour1[3] = 0x80;
	colour2[3] = 0x80;
	end = vertices;
	if (bdDrawBeam3D(self->pos, 40.0f, 40.0f, colour1, self->pos2, 20.0f, 20.0f, colour2,
		NULL, vertices + 7, 0.0f) != 0)
	{
		memcpy(vertices, vertices + 7, 7 * sizeof(float));
		memcpy(vertices + 0x3f, vertices + 0x38, 7 * sizeof(float));
		end = vertices + 0x46;
	}
	bdVertexBufferUnlock(self->vb);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 1, 0);
	Blend_SetMode(1);
	bdSetRenderState(5, 0, 0);
	count = (int)(end - vertices) / 7;
	if (count != 0)
	{
		bdSetDepthOffset(-100.0f);
		bdDrawVertexBuffer(4, self->vb, 0, count);
		bdSetDepthOffset(0.0f);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetProjectionMode(1);
}
