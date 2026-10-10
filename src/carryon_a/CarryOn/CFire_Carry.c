/*
CarryOn/CFire_Carry.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Carry.cpp

What the players carry: per player and type a model instance held between the hands
(variant 1: rockets of 1..5 fireworks; variant 2: the glitter net, whose glitter cloud
is drawn as a waving cone; variant 3: the five firework parts; variant 4: the tapir with
a smoke trail when lit). The carry object is DAT_002f6d4c.

0x000c3f20 and 0x000c4d60 are tagged with CFire_Carry.cpp; the functions between and
after them work on the same objects (file inferred).
*/

#include "carryon_a.h"

/* carryon_b: the flame/smoke emitter (0x002f798c) */
#define co_Flames_Emit ((void (*)(void *, float *, float, float, float))FUN_000d1b10)

/* the world matrix of a node times an instance's matrix; the original uses the copy
variant when all three matrices are 16-byte aligned (same result) */
static void co_MatrixMultiply(float *out, const float *a, const float *b)
{
	if ((((unsigned int)out | (unsigned int)a | (unsigned int)b) & 0xf) == 0)
		bmMatrixMultiplyCopy(out, a, b);
	else
		bmMatrixMultiply(out, a, b);
}

/* 0x000c3f20  CarryOn/CFire_Carry.cpp
Creates the carried object of `player` of `type` with `model` (none: an empty slot):
its instance plays "default" (looped) or "default_noloop"; the glitter net gets a
vertex buffer and random wave phases, the tapir its trail buffers. */
struct CoCarried *CFire_Carried_Ctor(struct CoCarried *self, int player, int type, void *model)
{
	int i;

	memset(self, 0, sizeof *self);
	self->player = player;
	if (model == NULL)
		return self;
	self->streak = bLoadResource(NULL, "alphatex\\tapir_streak.bmp", 0, 0xdefa);
	self->inst = baInitActorInstance(NULL, model, 2, CO_FILE_CARRY, 0x27);
	self->anim = baFindSubObject(self->inst, "default");
	if (self->anim != NULL)
		baQueueAnimation(self->inst, CO_PTR(self->inst, 0x134), self->anim, 0x400, -1, 1, NULL, NULL);
	else
	{
		self->anim = baFindSubObject(self->inst, "default_noloop");
		if (self->anim != NULL)
			baQueueAnimation(self->inst, CO_PTR(self->inst, 0x134), self->anim, 0x400, -1, 0, NULL, NULL);
	}
	self->handle = baFindNode(CO_PTR(self->inst, 0x134), "handle");
	if (self->handle == NULL)
		self->handle = CO_PTR(self->inst, 0x134);
	self->type = type;
	if (type == 5)
	{
		if (bdVertexBufferCreate((struct bdVertexBuffer **)&self->vb, 0x24, 0x152, 0, 1) != 0)
		{
			for (i = 0; i < 10; i++)
			{
				self->speed[i] = 0.3f;
				self->phase[i] = (float)co_rand() * 0.00019175344f;   /* 0x00263520 */
			}
		}
	}
	else if (type == 0xb)
	{
		self->trail = bkHeapAlloc(0x1e0, CO_FILE_CARRY, 0x49, 0x2001);
		self->trailVel = bkHeapAlloc(0x1e0, CO_FILE_CARRY, 0x4a, 0x2001);
		self->trailCount = 0;
		if (bdVertexBufferCreate((struct bdVertexBuffer **)&self->vb, 0x40, 0x144, 0, 1) != 0)
		{
			self->flame = baFindNode(CO_PTR(self->inst, 0x134), "flame");
			if (self->flame == NULL)
				self->flame = CO_PTR(self->inst, 0x134);
		}
	}
	return self;
}

/* 0x000c4110  CarryOn/CFire_Carry.cpp (file inferred)
Finds the hand nodes of the player's character while the object is held. */
void CFire_Carried_UpdateHands(struct CoCarried *self)
{
	void *actor = CO_PTR(g_coPlayers[self->player], CP_ACTOR);
	int character = DAT_002baed4[self->player][0];

	if (self->visible == 0)
	{
		self->leftHand = NULL;
		self->rightHand = NULL;
		return;
	}
	self->leftHand = baFindNode(CO_PTR(actor, 0x144),
		*(const char **)&DAT_00290718[character * 0x60 - 0x10]);   /* "lpalm" names */
	self->rightHand = baFindNode(CO_PTR(actor, 0x144),
		*(const char **)&DAT_00290718[character * 0x60 - 0xc]);    /* "rpalm" names */
	if (self->leftHand == NULL)
	{
		self->leftHand = CO_PTR(actor, 0x144);
		self->rightHand = CO_PTR(actor, 0x144);
	}
}

/* 0x000c41b0  CarryOn/CFire_Carry.cpp (file inferred)
Places the object: the glitter net between the hands, facing the player, while the jar
animation runs; anything else at the left hand. Returns the instance. */
void *CFire_Carried_Place(struct CoCarried *self)
{
	struct CCarryOnPlayer *player;
	float left[4], right[4];
	float axisAngle[4];
	float q[4];
	float m[16];
	float x, y, z;
	void *inst = self->inst;

	if (inst == NULL)
		return NULL;
	player = g_coPlayers[self->player];
	if (self->type == 5 && player->jarAnim != 0)
	{
		baGetNodePosition(CO_ADDR(CO_PTR(player, CP_ACTOR), 0x10), self->leftHand, left);
		baGetNodePosition(CO_ADDR(CO_PTR(player, CP_ACTOR), 0x10), self->rightHand, right);
		axisAngle[0] = 0.0f;
		axisAngle[1] = 1.0f;
		axisAngle[2] = 0.0f;
		x = (right[0] + left[0]) * 0.5f;
		y = (right[1] + left[1]) * 0.5f;
		z = (right[2] + left[2]) * 0.5f;
		axisAngle[3] = -CO_F32(CO_PTR(player, CP_ACTOR), CA_YAW);
		bmAxisAngleToQuaternion(q, axisAngle);
		CO_F32(self->inst, 0xe0) = q[0];
		CO_F32(self->inst, 0xe4) = q[1];
		CO_F32(self->inst, 0xe8) = q[2];
		CO_F32(self->inst, 0xec) = q[3];
		CO_F32(self->inst, 0xc0) = x;
		CO_F32(self->inst, 0xc4) = y;
		CO_F32(self->inst, 0xc8) = z;
		CO_F32(self->inst, 0xcc) = 1.0f;
	}
	else
	{
		CO_F32(inst, 0xe0) = DAT_00285634[0];           /* the identity quaternion */
		CO_F32(inst, 0xe4) = DAT_00285634[1];
		CO_F32(inst, 0xe8) = DAT_00285634[2];
		CO_F32(inst, 0xec) = DAT_00285634[3];
		CO_F32(self->inst, 0xc0) = 0.0f;
		CO_F32(self->inst, 0xc4) = 0.0f;
		CO_F32(self->inst, 0xc8) = 0.0f;
		CO_F32(self->inst, 0xcc) = 1.0f;
		baPrepActorInstance(self->inst, 1);
		co_MatrixMultiply(m, CO_FP(self->leftHand, 0x80), self->inst);
		co_MatrixMultiply(self->inst, CO_FP(CO_PTR(player, CP_ACTOR), 0x10), m);
		bmMatrixToQuaternion(CO_FP(self->inst, 0xe0), self->inst);
		CO_F32(self->inst, 0xc0) = CO_F32(self->inst, 0x30);
		CO_F32(self->inst, 0xc4) = CO_F32(self->inst, 0x34);
		CO_F32(self->inst, 0xc8) = CO_F32(self->inst, 0x38);
		CO_F32(self->inst, 0xcc) = CO_F32(self->inst, 0x3c);
	}
	baPrepActorInstance(self->inst, 1);
	return self->inst;
}

/* 0x000c43f0  CarryOn/CFire_Carry.cpp (file inferred)
While held: animates the object; the glitter net's waves wander (speeds 0.0001..0.5);
the lit tapir (player +0x1144) leaves a smoke trail of up to 30 drifting points from its
flame node, which fades out once it is no longer lit. */
void CFire_Carried_Update(struct CoCarried *self, float step)
{
	struct CCarryOnPlayer *player = g_coPlayers[self->player];
	float m[16];
	float *p;
	float *v;
	int i;

	if (self->visible == 0)
		return;
	if (self->inst != NULL)
	{
		CO_F32(self->inst, 0xc8) = 0.0f;
		CO_F32(self->inst, 0xc4) = 0.0f;
		CO_F32(self->inst, 0xc0) = 0.0f;
		CO_F32(self->inst, 0xcc) = 1.0f;
		if (self->anim != NULL)
			baUpdateAnimations(self->inst, CO_PTR(self->inst, 0x134), step, NULL);
		baPrepActorInstance(self->inst, 1);
	}
	if (self->type == 5)
	{
		for (i = 0; i < 10; i++)
		{
			self->speed[i] = (float)co_rand() * 3.051851e-06f - 0.05f + self->speed[i];  /* 0x00265744, 0x00258b54 */
			if (self->speed[i] <= 0.5f)
			{
				if (self->speed[i] < 0.0001f)                   /* 0x00266ff4 */
					self->speed[i] = 0.0001f;
			}
			else
				self->speed[i] = 0.5f;
			self->phase[i] = self->phase[i] + self->speed[i];
			if (self->phase[i] > 6.283185f)                     /* 0x00262300 */
			{
				do
					self->phase[i] = self->phase[i] - 6.283185f;
				while (self->phase[i] > 6.283185f);
			}
		}
	}
	else if (self->type == 0xb)
	{
		if (player->int1144 == 0)
		{
			if (self->trailCount != 0)
				self->trailCount--;
		}
		else
		{
			self->trailCount++;
			if (self->trailCount > 0x1e)
				self->trailCount = 0x1e;
			for (i = 29; i > 0; i--)
			{
				memcpy(&self->trail[i * 4], &self->trail[(i - 1) * 4], 16);
				memcpy(&self->trailVel[i * 4], &self->trailVel[(i - 1) * 4], 16);
			}
			baPrepActorInstance(self->inst, 1);
			co_MatrixMultiply(m, CO_FP(self->leftHand, 0x80), self->inst);
			co_MatrixMultiply(self->inst, CO_FP(CO_PTR(player, CP_ACTOR), 0x10), m);
			baGetNodePosition(self->inst, self->flame, self->trail);
			co_Flames_Emit(DAT_002f798c, self->trail, 30.0f, 8.0f, 6.0f);
			self->trailVel[0] = ((float)co_rand() - 16384.0f) * 6.1035156e-05f;   /* 0x00267ce8, 0x00267ce4 */
			self->trailVel[1] = ((float)co_rand() - 16384.0f) * 6.1035156e-05f + 10.0f;
			self->trailVel[2] = ((float)co_rand() - 16384.0f) * 6.1035156e-05f;
			self->trailVel[3] = 1.0f;
		}
		for (i = 0; i < self->trailCount; i++)
		{
			v = &self->trailVel[i * 4];
			p = &self->trail[i * 4];
			v[0] = ((float)co_rand() - 16384.0f) * 6.1035157e-06f + v[0];     /* 0x00267ce0 */
			v[1] = ((float)co_rand() - 16384.0f) * 6.1035157e-06f + v[1];
			v[2] = ((float)co_rand() - 16384.0f) * 6.1035157e-06f + v[2];
			p[0] = p[0] + v[0];
			p[1] = p[1] + v[1];
			p[2] = p[2] + v[2];
		}
	}
}

/* 0x000c4830  CarryOn/CFire_Carry.cpp (file inferred)
Draws the object while held. The glitter net also draws its glitter as a waving cone
(11 rim points, alpha blended, brighter with more glitter); the tapir its smoke trail as
a textured polyline. */
void CFire_Carried_Draw(struct CoCarried *self, unsigned int passes)
{
	struct CCarryOnPlayer *player = g_coPlayers[self->player];
	struct CoVertex *vertices;
	struct CoVertex *vtx;
	unsigned char *rgb;
	unsigned int oldBlend;
	int *colour;
	float amount, base, wave, width;
	float angle, s, c;
	float radius;
	int r, g, b;
	int i;

	if (self->visible == 0 || self->inst == NULL)
		return;
	CFire_Carried_Place(self);
	baDrawActorInstance(self->inst, passes);
	if (self->type == 5)
	{
		if (player->glitter <= 0)
			return;
		oldBlend = bdGetRenderState(3, NULL);
		bdSetRenderState(3, 0, 0);
		bdSetWorldMatrix(self->inst);
		amount = (float)player->glitter * 0.01f;
		rgb = &DAT_00290718[DAT_002baed4[self->player][0] * 0x60];
		r = rgb[0] << 1;
		g = rgb[1] << 1;
		b = rgb[2] << 1;
		if (amount < 0.1f)
		{
			/* (the original scales red, then takes green and blue from the scaled red) */
			r = (int)((float)r * amount * 10.0f);
			g = (int)((float)r * amount * 10.0f);
			b = g;
		}
		base = amount * 1.3f;                               /* 0x00266448 */
		width = amount * 0.5f;
		vertices = bdVertexBufferLock(self->vb, 4);
		if (vertices == NULL)
			return;
		angle = 0.0f;
		vtx = vertices;
		for (i = 0; i < 11; i++)
		{
			s = sinf(angle);
			c = cosf(angle);
			wave = sinf(self->phase[i % 10]) * width + base;
			radius = (DAT_002940c4 - DAT_002940c0) * wave + DAT_002940c0;
			vtx[0].x = DAT_002940c0 * s;
			vtx[0].y = DAT_002940b8;
			vtx[0].z = DAT_002940c0 * c;
			vtx[0].colour = DAT_002bce18[r] | DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bca94;
			vtx[0].u = 0.0f;
			vtx[0].v = 0.0f;
			vtx[1].x = radius * s;
			vtx[1].y = wave * (DAT_002940bc - DAT_002940b8) + DAT_002940b8;
			vtx[1].z = radius * c;
			vtx[1].colour = DAT_002bc898[0] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
			vtx[1].u = 0.0f;
			vtx[1].v = 0.0f;
			angle = angle + 0.62831849f;                    /* 0x00267cec */
			vtx += 2;
		}
		/* close the strip and cross back through the inner points */
		vtx[0] = vtx[-1];
		vtx[1] = vertices[0];
		vtx[2] = vertices[0];
		vtx[3] = vertices[18];
		vtx[4] = vertices[2];
		vtx[5] = vertices[16];
		vtx[6] = vertices[4];
		vtx[7] = vertices[14];
		vtx[8] = vertices[6];
		vtx[9] = vertices[12];
		vtx[10] = vertices[8];
		vtx[11] = vertices[10];
		bdVertexBufferUnlock(self->vb);
		bdSetTexture(0, NULL);
		bdSetRenderState(6, 1, 0);
		Blend_SetMode(1);
		bdSetRenderState(5, 0, 0);
		bdDrawVertexBuffer(4, self->vb, 0, 0x22);
		bdSetRenderState(6, 0, 0);
		bdSetRenderState(5, 1, 0);
		bdSetRenderState(3, oldBlend, 0);
		bdSetWorldMatrix(DAT_002855f0);
		return;
	}
	if (self->type != 0xb || self->trailCount < 3)
		return;
	vertices = bdVertexBufferLock(self->vb, 4);
	if (vertices == NULL)
		return;
	rgb = &DAT_00290718[DAT_002baed4[self->player][0] * 0x60];
	for (i = 0; i < self->trailCount; i++)
	{
		colour = &DAT_002f6d58[i * 4];
		colour[2] = (rgb[0] * 0x40) / 0x100 + 0x20;
		colour[1] = (rgb[1] * 0x40) / 0x100 + 0x20;
		colour[0] = (rgb[2] * 0x40) / 0x100 + 0x20;
		colour[3] = (self->trailCount - i) * 0x7f / self->trailCount;
	}
	bdSetWorldMatrix(DAT_002855f0);
	i = bdDrawPolyline3D(self->trailCount, self->trail, DAT_002f6d50, DAT_002f6d54, DAT_002f6d58,
		(float *)vertices, 0.0f);
	bdVertexBufferUnlock(self->vb);
	if (i == 0)
		return;
	bdSetTexture(0, self->streak);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 0, 0);
	bdDrawVertexBuffer(4, self->vb, 0, self->trailCount * 2 + 4);
	bdSetProjectionMode(1);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* 0x000c4d60  CarryOn/CFire_Carry.cpp
Loads the variant's carried models from the level package and makes every player's
objects: variant 1 rockets of 1..5 (rocket1.obe..rocket5.obe), variant 2 the glitter net,
variant 3 the five parts, variant 4 the tapir and its trail tables. (The original guards
each allocation with a C++ exception frame.) */
struct CFireCarry *CFire_Carry_Ctor(struct CFireCarry *self, void *levelPackage,
	void *genericPackage, int variant)
{
	static const int lines[3][5] = {
		{ 0x2aa, 0x2ab, 0x2ac, 0x2ad, 0x2ae },
		{ 0x2d5, 0x2d6, 0x2d7, 0x2d8, 0x2d9 },
	};
	void *mem;
	int p, t, i;

	(void)genericPackage;
	memset(self, 0, sizeof *self);
	self->variant = variant;
	DAT_002f6d4c = self;
	switch (variant)
	{
	case 0:
		for (t = 0; t < 5; t++)
			self->models[t] = bLoadResource(levelPackage, DAT_002940e4[t], 1, 0xdefa);
		for (p = 0; p < 4; p++)
		{
			for (t = 0; t < 5; t++)
			{
				mem = bkHeapAlloc(0x88, CO_FILE_CARRY, lines[0][t], 0x1001);
				self->items[p][t] = mem == NULL ? NULL : CFire_Carried_Ctor(mem, p, t, self->models[t]);
			}
		}
		break;
	case 1:
		self->models[0] = bLoadResource(levelPackage, DAT_002940e0[0], 1, 0xdefa);
		for (p = 0; p < 4; p++)
		{
			mem = bkHeapAlloc(0x88, CO_FILE_CARRY, 0x2bf, 0x1001);
			self->items[p][0] = mem == NULL ? NULL : CFire_Carried_Ctor(mem, p, 5, self->models[0]);
		}
		break;
	case 2:
		for (t = 0; t < 5; t++)
			self->models[t] = bLoadResource(levelPackage, DAT_002940cc[t], 1, 0xdefa);
		for (p = 0; p < 4; p++)
		{
			for (t = 0; t < 5; t++)
			{
				mem = bkHeapAlloc(0x88, CO_FILE_CARRY, lines[1][t], 0x1001);
				self->items[p][t] = mem == NULL ? NULL : CFire_Carried_Ctor(mem, p, 6 + t, self->models[t]);
			}
		}
		break;
	case 3:
		self->models[0] = bLoadResource(levelPackage, DAT_002940c8[0], 1, 0xdefa);
		for (p = 0; p < 4; p++)
		{
			mem = bkHeapAlloc(0x88, CO_FILE_CARRY, 0x2ec, 0x1001);
			self->items[p][0] = mem == NULL ? NULL : CFire_Carried_Ctor(mem, p, 0xb, self->models[0]);
		}
		DAT_002f6d50 = bkHeapAlloc(0x78, CO_FILE_CARRY, 0x2ef, 0x2001);
		DAT_002f6d54 = bkHeapAlloc(0x78, CO_FILE_CARRY, 0x2f0, 0x2001);
		DAT_002f6d58 = bkHeapAlloc(0x1e0, CO_FILE_CARRY, 0x2f1, 0x2001);
		for (i = 0; i < 0x1e; i++)
		{
			DAT_002f6d50[i] = 20.0f;
			DAT_002f6d58[i * 4 + 0] = 0x40;
			DAT_002f6d58[i * 4 + 1] = 0x40;
			DAT_002f6d58[i * 4 + 2] = 0x40;
			DAT_002f6d58[i * 4 + 3] = 0x40;
		}
		break;
	}
	return self;
}

/* 0x000c52b0  CarryOn/CFire_Carry.cpp (file inferred)
Draws every held object (in the first pass inside the resource's stage set-up). */
void CFire_Carry_Draw(struct CFireCarry *self, unsigned int passes)
{
	int p, t;

	if (passes & 1)
		CFire_Resource_BeginStage(0x40);
	for (p = 0; p < 4; p++)
		for (t = 0; t < 5; t++)
			if (self->items[p][t] != NULL)
				CFire_Carried_Draw(self->items[p][t], passes);
	if (passes & 1)
		CFire_Resource_EndStage();
}

/* 0x000c5310  CarryOn/CFire_Carry.cpp (file inferred)
Finds the hand nodes again (the player switched its model). */
void CFire_Carry_UpdateHeld(struct CFireCarry *self, int player)
{
	int t;

	for (t = 0; t < 5; t++)
		if (self->items[player][t] != NULL)
			CFire_Carried_UpdateHands(self->items[player][t]);
}

/* 0x000c5340  CarryOn/CFire_Carry.cpp (file inferred)
The instance of what `player` holds (placed), or NULL. */
void *CFire_Carry_GetHeld(struct CFireCarry *self, int player)
{
	int t;

	for (t = 0; t < 5; t++)
		if (self->items[player][t] != NULL && self->items[player][t]->visible != 0)
			return CFire_Carried_Place(self->items[player][t]);
	return NULL;
}

/* 0x000c5390  CarryOn/CFire_Carry.cpp (file inferred)
The destructor: frees the models, the objects and the trail tables. */
void CFire_Carry_Dtor(struct CFireCarry *self)
{
	struct CoCarried *item;
	int p, t;

	DAT_002f6d4c = NULL;
	for (t = 0; t < 5; t++)
		if (self->models[t] != NULL)
			bkDeleteResource(self->models[t]);
	for (p = 0; p < 4; p++)
	{
		for (t = 0; t < 5; t++)
		{
			item = self->items[p][t];
			if (item == NULL)
				continue;
			if (item->inst != NULL)
			{
				baDeleteActorInstance(item->inst);
				item->inst = NULL;
			}
			if (item->trail != NULL)
			{
				bkHeapFree(item->trail);
				item->trail = NULL;
				bkHeapFree(item->trailVel);
				item->trail = NULL;
				item->trailVel = NULL;
			}
			if (item->vb != NULL)
				bdVertexBufferDestroy(item->vb);
			bkHeapFree(item);
		}
	}
	if (DAT_002f6d50 != NULL)
	{
		bkHeapFree(DAT_002f6d50);
		DAT_002f6d50 = NULL;
	}
	if (DAT_002f6d54 != NULL)
	{
		bkHeapFree(DAT_002f6d54);
		DAT_002f6d54 = NULL;
	}
	if (DAT_002f6d58 != NULL)
	{
		bkHeapFree(DAT_002f6d58);
		DAT_002f6d58 = NULL;
	}
}

/* shows (1) or hides (0) a held object and finds its hand nodes when it changes */
static void co_ShowCarried(struct CoCarried *item, int visible)
{
	if (visible)
	{
		if (item->visible != 1)
		{
			item->visible = 1;
			CFire_Carried_UpdateHands(item);
		}
	}
	else if (item->visible != 0)
	{
		item->visible = 0;
		CFire_Carried_UpdateHands(item);
	}
}

/* 0x000c5490  CarryOn/CFire_Carry.cpp (file inferred)
Shows what each player carries (variant 1: the rocket of its firework count; variant 3:
the part it carries; variants 2 and 4: always the net / tapir; nothing at the end) and
updates the shown objects. */
void CFire_Carry_Update(struct CFireCarry *self, float step)
{
	struct CCarryOnPlayer *player;
	struct CoCarried *item;
	int p, t;

	for (p = 0; p < 4; p++)
	{
		player = g_coPlayers[p];
		if (g_fireGame.state == CO_STATE_END)
		{
			for (t = 0; t < 5; t++)
			{
				item = self->items[p][t];
				if (item != NULL && item->visible != 0)
				{
					item->visible = 0;
					CFire_Carried_UpdateHands(item);
				}
			}
		}
		else
		{
			switch (self->variant)
			{
			case 0:
				for (t = 0; t < 5; t++)
					if (self->items[p][t] != NULL)
						co_ShowCarried(self->items[p][t], player->fireworks == t + 1);
				break;
			case 1:
			case 3:
				if (self->items[p][0]->visible != 1)
				{
					self->items[p][0]->visible = 1;
					CFire_Carried_UpdateHands(self->items[p][0]);
				}
				break;
			case 2:
				for (t = 0; t < 5; t++)
					if (self->items[p][t] != NULL)
						co_ShowCarried(self->items[p][t],
							player->fireworks != 0 && t == player->partTypes[0]);
				break;
			}
		}
		for (t = 0; t < 5; t++)
			if (self->items[p][t] != NULL)
				CFire_Carried_Update(self->items[p][t], step);
	}
}
