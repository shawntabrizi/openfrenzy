/*
Meta/unplaced.c  <-  C:\BlitzParty\Game\Src\Meta\ (file not known)

Gap functions of the Meta directory whose source file is not known.

Vents (0x0015d4c0-0x0015e3a0): particle emitters placed by the level script, switched on
and off in a cycle, that push (and hurt) players inside their radius. They sit between
CLightning.cpp and CMeta_ExplSmoke.cpp; no assert names their file.
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x00308e60  the vents */
struct MbList DAT_00308e60;

/* 0x00308990, 0x00308ad8, 0x00308c20  the debug cylinder: top ring, bottom ring (line
   strips of 9 vertices) and the wall (a strip of 16 vertices) */
static struct MbVertex DAT_00308990[9];
static struct MbVertex DAT_00308ad8[9];
static struct MbVertex DAT_00308c20[16];

/* 0x002b1ef4  the "SONIC_RING" particle type (owner: the particle system) */
extern char DAT_002b1ef4[];

#define ALIGNED16(a, b) (((((uintptr_t)(a)) | ((uintptr_t)(b))) & 0xf) == 0)

/* 0x0015d4c0  Meta (file unknown)
Vent constructor: the first particle type, its colour (0 = the type's colour), the tag,
whether it starts active, and the emit rate. Adds the vent to the vent list. */
struct CMetaVent *CMetaVent_Init(struct CMetaVent *self, void *type, int colour, short tag,
	short active, float rate)
{
	self->type[0] = type;
	if (colour == 0)
		colour = -1;
	self->colour[0] = colour;
	self->tag = tag;
	self->rate[0] = rate;
	self->active = active;
	self->on = 0;
	self->numCycle = 0;
	self->numTypes = 1;
	self->accum[0] = 0.0f;
	self->timer = 0;
	self->cycle = 0;
	self->flags = 0;
	self->radius = 0.0f;
	self->length = 0.0f;
	self->vel[0] = 0.0f;
	self->vel[1] = 0.0f;
	self->vel[2] = 0.0f;
	self->vel[3] = 1.0f;
	self->pos[0] = 0.0f;
	self->pos[1] = 0.0f;
	self->pos[2] = 0.0f;
	self->pos[3] = 1.0f;
	self->velSpreadXZ = 0.0f;
	self->velSpreadY = 0.0f;
	self->posSpreadXZ = 0.0f;
	self->posSpreadY = 0.0f;
	self->parent = NULL;
	if (active != 0)
		self->timer = 1;
	MB_LIST_INSERT(&DAT_00308e60, self, 0);
	return self;
}

/* 0x0015d590  Meta (file unknown)
Adds a step to the on/off cycle (6 at most). Returns 0 when it is full. */
int CMetaVent_AddCycle(struct CMetaVent *self, short offTime, short onTime)
{
	if (self->numCycle > 5)
		return 0;
	self->offTime[self->numCycle] = offTime;
	self->onTime[self->numCycle] = onTime;
	self->numCycle++;
	return 1;
}

/* 0x0015d5d0  Meta (file unknown)
Adds a particle type (5 at most). */
void CMetaVent_AddType(struct CMetaVent *self, void *type, int colour, float rate)
{
	if (self->numTypes < 5)
	{
		self->type[self->numTypes] = type;
		if (colour == 0)
			colour = -1;
		self->colour[self->numTypes] = colour;
		self->rate[self->numTypes] = rate;
		self->accum[self->numTypes] = 0.0f;
		self->numTypes++;
	}
}

/* 0x0015d610  Meta (file unknown)
Debug drawing: a cylinder of `radius` and `height` at `pos`, along `dir`, in the colour
(r, g, b, a). */
void MetaVent_DrawCylinder(float *pos, float *dir, float radius, float height, short r,
	short g, short b, short a)
{
	float point[3];
	float quat[4];
	float rotation[16];
	float translation[16];
	float world[16];
	float spoke[3];
	float angle = 0.0f;
	unsigned int colour;
	int i;

	point[0] = dir[0];
	point[1] = dir[1];
	point[2] = dir[2];
	CLQuat_FromDirection(quat, point);
	FUN_000515c0(rotation, quat);
	MB_DUAL(FUN_00050250, void (*)(undefined4 *, undefined4, undefined4, undefined4),
		void (*)(float *, float, float, float),
		((undefined4 *)translation, mb_fbits(pos[0]), mb_fbits(pos[1]), mb_fbits(pos[2])),
		(translation, pos[0], pos[1], pos[2]));
	if (ALIGNED16(world, translation) && ALIGNED16(rotation, rotation))
		FUN_00050150(world, translation, rotation);
	else
		FUN_0004ff30(world, translation, rotation);
	FUN_00043930((undefined4 *)world);
	spoke[0] = radius;
	spoke[1] = 0.0f;
	spoke[2] = 0.0f;
	for (i = 0; i < 8; i++)
	{
		FUN_00051170(point, spoke, angle);
		angle = angle + 0.78539813f;                    /* 0x00263094: pi/4 */
		colour = DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bc898[a] | DAT_002bce18[r];
		DAT_00308c20[i * 2].x = point[0];
		DAT_00308c20[i * 2].y = 0.0f;
		DAT_00308c20[i * 2].z = point[2];
		DAT_00308c20[i * 2].colour = colour;
		DAT_00308c20[i * 2 + 1].x = point[0];
		DAT_00308c20[i * 2 + 1].y = height;
		DAT_00308c20[i * 2 + 1].z = point[2];
		DAT_00308c20[i * 2 + 1].colour = colour;
		DAT_00308ad8[i].x = point[0];
		DAT_00308ad8[i].y = 0.0f;
		DAT_00308ad8[i].z = point[2];
		DAT_00308ad8[i].colour = colour;
		DAT_00308990[i].x = point[0];
		DAT_00308990[i].y = height;
		DAT_00308990[i].z = point[2];
		DAT_00308990[i].colour = colour;
	}
	colour = DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bc898[a] | DAT_002bce18[r];
	DAT_00308990[8].x = radius;
	DAT_00308ad8[8].x = radius;
	DAT_00308990[8].y = height;
	DAT_00308ad8[8].y = 0.0f;
	DAT_00308ad8[8].z = 0.0f;
	DAT_00308990[8].z = 0.0f;
	DAT_00308ad8[8].colour = colour;
	DAT_00308990[8].colour = colour;
	bdSetTexture(0, NULL);
	MB_SET_RS(3, 0);
	MB_SET_RS(7, 1);
	MB_SET_RS(6, 1);
	bdDrawPrimitive(1, DAT_00308c20, 0x10);
	bdDrawPrimitive(2, DAT_00308ad8, 9);
	bdDrawPrimitive(2, DAT_00308990, 9);
	MB_SET_RS(3, MB_U32(DAT_003055d0, 0x48));
	MB_SET_RS(7, 3);
	MB_SET_RS(6, 0);
}

/* 0x0015d8f0  Meta (file unknown)
Particle velocity and its random spread (horizontal, vertical). */
void CMetaVent_SetVelocity(struct CMetaVent *self, float *vel, float spreadXZ, float spreadY)
{
	self->vel[0] = vel[0];
	self->vel[1] = vel[1];
	self->vel[2] = vel[2];
	self->vel[3] = vel[3];
	self->velSpreadXZ = spreadXZ;
	self->velSpreadY = spreadY;
}

/* 0x0015d930  Meta (file unknown)
Position and its random spread (horizontal, vertical). */
void CMetaVent_SetPosition(struct CMetaVent *self, float *pos, float spreadXZ, float spreadY)
{
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->posSpreadXZ = spreadXZ;
	self->posSpreadY = spreadY;
}

/* 0x0015d970  Meta (file unknown)
Emits one particle of type `index`, with random spread, moved forward by a random part
of a frame. */
void CMetaVent_Emit(struct CMetaVent *self, int index)
{
	float pos[4];
	float vel[4];
	float angle;
	float s;
	float c;
	void *particle;

	pos[0] = self->pos[0];
	pos[1] = self->pos[1];
	pos[2] = self->pos[2];
	pos[3] = self->pos[3];
	vel[0] = self->vel[0];
	vel[2] = self->vel[2];
	vel[3] = self->vel[3];
	vel[1] = self->vel[1];
	if (self->posSpreadY != 0.0f)
		pos[1] = (float)mb_rand() * self->posSpreadY * MB_RAND_SCALE + pos[1];
	if (self->posSpreadXZ != 0.0f)
	{
		angle = (float)mb_rand() * MB_RAND_ANGLE;
		s = (float)mb_rand() * MB_RAND_SCALE * self->posSpreadXZ;
		c = (float)mb_rand() * MB_RAND_SCALE * self->posSpreadXZ;
		pos[0] = sinf(angle) * s + pos[0];
		pos[2] = cosf(angle) * c + pos[2];
	}
	if (self->velSpreadY != 0.0f)
		vel[1] = (float)mb_rand() * self->velSpreadY * MB_RAND_SCALE + vel[1];
	if (self->velSpreadXZ != 0.0f)
	{
		angle = (float)mb_rand() * MB_RAND_ANGLE;
		vel[0] = sinf(angle) * self->velSpreadXZ + vel[0];
		vel[2] = cosf(angle) * self->velSpreadXZ + vel[2];
	}
	if (self->parent != NULL)
		FUN_00051170(vel, vel, MB_F32(self->parent, 0x1f8));
	FUN_00050ff0(pos, pos, 1.0f, vel, (float)mb_rand() * DAT_0028fda8 * MB_RAND_SCALE);
	particle = (void *)FUN_00080d60((int)self->type[index], (undefined4)pos, (undefined4)vel, 0,
		(uint)self->colour[index]);
	if (self->type[index] == (void *)DAT_002b1ef4)
		MB_F32(particle, 0x8c) = self->radius * 0.02f;  /* 0x0025588c */
}

/* 0x0015dc40  Meta (file unknown)
Switches the vent: 1 = on (starts its cycle at a random step), 0 = off, other values
are stored as they are. */
void CMetaVent_SetActive(struct CMetaVent *self, int mode)
{
	if (mode == 1)
	{
		if (self->active == 0)
		{
			if (self->numCycle < 1)
			{
				self->active = 1;
				self->on = 1;
				self->timer = 0;
				return;
			}
			self->on = 0;
			self->timer = 1;
			self->cycle = (short)(mb_rand() % (int)self->numCycle);
		}
		self->active = 1;
	}
	else
	{
		self->active = (short)mode;
		if (mode == 0)
		{
			self->on = 0;
			self->timer = 0;
		}
	}
}

/* 0x0015dcc0  Meta (file unknown)
Pushes player `player` (its position `pos`) out of the vent. With `test` set it only
returns 1 when the player is inside. A spherical vent hurts the player too. */
int CMetaVent_Push(struct CMetaVent *self, int player, float *pos, int test)
{
	float d[3];
	float dist;
	float along;
	float px, py, pz;
	float fAlong;
	float fSide;
	float push;
	int meta = DAT_003075bc[player];
	int character;

	d[0] = pos[0] - self->pos[0];
	d[2] = pos[2] - self->pos[2];
	d[1] = (pos[1] - self->pos[1]) + 100.0f;           /* 0x00255544 */
	if ((self->flags & 0x10) == 0)
	{
		dist = sqrtf(d[1] * d[1] + d[0] * d[0] + d[2] * d[2]);
		if (!(dist < self->radius))
			return 0;
		if (test != 0)
			return 1;
		if (MB_I16(meta, 0x36) == 10 || MB_I16(meta, 0x36) == 0xd)
			return 0;
		bkPrintf("VENT *** OUCH!\n");
		if (MB_F32(DAT_003075bc[player], 0x10a4) == 0.0f)
			((void (*)(int, float *, float, int, int))FUN_00156c40)(DAT_003075bc[player],
				self->pos, DAT_002b0a60, 0, 1);
		if ((self->flags & 8) == 0)
		{
			if (dist <= 0.0f)
				return 0;
			dist = (self->strength / dist) * DAT_0028fda8;
			d[0] = d[0] * dist;
			d[1] = dist * d[1];
			d[2] = dist * d[2];
		}
		else
		{
			FUN_0001ea00(d, self->dir, self->strength);
		}
		character = MB_I32(DAT_003075bc[player], 0x30);
		MB_F32(character, 0x194) = d[0] + MB_F32(character, 0x194);
		character = MB_I32(DAT_003075bc[player], 0x30);
		MB_F32(character, 0x198) = d[1] + MB_F32(character, 0x198);
		character = MB_I32(DAT_003075bc[player], 0x30);
		MB_F32(character, 0x19c) = d[2] + MB_F32(character, 0x19c);
		return 0;
	}
	along = d[0] * self->dir[0] + d[1] * self->dir[1] + d[2] * self->dir[2];
	if (!(0.0f <= along && along <= self->length + self->length))
		return 0;
	pz = along * self->dir[2];
	px = along * self->dir[0] - d[0];
	py = along * self->dir[1] - d[1];
	dist = sqrtf(px * px + py * py + (pz - d[2]) * (pz - d[2]));
	if (!(dist <= self->radius + self->radius))
		return 0;
	if (test != 0)
		return 1;
	fAlong = 1.0f;                                      /* 0x00253850 */
	if (self->length <= along)
		fAlong = 2.0f - along / self->length;          /* 0x00257318 */
	fSide = 1.0f;
	if (self->radius <= dist)
		fSide = 2.0f - dist / self->radius;
	push = fSide * self->strength * fAlong;
	FUN_0001ea00(d, self->dir, DAT_0028fda8 * push);
	character = MB_I32(DAT_003075bc[player], 0x30);
	MB_F32(character, 0x194) = d[0] + MB_F32(character, 0x194);
	character = MB_I32(DAT_003075bc[player], 0x30);
	MB_F32(character, 0x198) = d[1] + MB_F32(character, 0x198);
	character = MB_I32(DAT_003075bc[player], 0x30);
	MB_F32(character, 0x19c) = d[2] + MB_F32(character, 0x19c);
	if (push > DAT_002b0a5c)
		FUN_00156ba0(DAT_003075bc[player], 0, 1);
	return 0;
}

/* 0x0015e020  Meta (file unknown)
Makes the vent a cylinder along `dir` that pushes players: radius, length, strength
and flags (0x10 is added). */
void CMetaVent_SetPush(struct CMetaVent *self, float *dir, float radius, float length,
	float strength, unsigned int flags)
{
	self->dir[0] = dir[0];
	self->dir[1] = dir[1];
	self->dir[2] = dir[2];
	self->dir[3] = dir[3];
	self->length = length;
	self->radius = radius;
	self->strength = strength;
	self->flags = flags | 0x10;
}

/* 0x0015e080  Meta (file unknown)
Deletes every vent. */
void MetaVents_DeleteAll(void)
{
	void *vent;
	void *next;

	vent = MB_LIST_NEXT(&DAT_00308e60, NULL);
	while (vent != NULL)
	{
		next = MB_LIST_REMOVE(&DAT_00308e60, vent);
		MB_DELETE(vent);
		vent = MB_LIST_NEXT(&DAT_00308e60, next);
	}
}

/* 0x0015e0c0  Meta (file unknown)
Switches every vent with `tag` (CMetaVent_SetActive). Returns 1 when one was found. */
int MetaVents_SetActive(int tag, int mode)
{
	struct CMetaVent *vent;
	int found = 0;

	for (vent = MB_LIST_NEXT(&DAT_00308e60, NULL); vent != NULL;
		vent = MB_LIST_NEXT(&DAT_00308e60, vent))
	{
		if ((int)vent->tag == tag)
		{
			CMetaVent_SetActive(vent, mode);
			found = 1;
		}
	}
	return found;
}

/* 0x0015e110  Meta (file unknown)
Debug drawing of a push cylinder: its size, and twice its size (where the push ends). */
void CMetaVent_DrawDebug(struct CMetaVent *self)
{
	MetaVent_DrawCylinder(self->pos, self->dir, self->radius, self->length, 0xf0, 0xf0, 0xf0,
		0xf0);
	MetaVent_DrawCylinder(self->pos, self->dir, self->radius + self->radius,
		self->length + self->length, 0x5a, 0x5a, 0x5a, 0x5a);
}

/* 0x0015e180  Meta (file unknown)
Per frame: follows the parent, runs the on/off cycle, emits particles while on, and
pushes the players (flags 1/2 while on, 4 while off) during the board phase. */
void CMetaVent_Update(struct CMetaVent *self)
{
	float rotated[3];
	float *parentPos;
	int i;

	for (;;)
	{
		if (self->parent != NULL)
		{
			FUN_00051170(rotated, self->offset, MB_F32(self->parent, 0x1f8));
			parentPos = MB_FP(self->parent, 0xe0);
			self->pos[0] = rotated[0] + parentPos[0];
			self->pos[1] = rotated[1] + parentPos[1];
			self->pos[2] = rotated[2] + parentPos[2];
		}
		if (self->active == 0)
			return;
		if (self->timer < 1)
			break;
		self->timer--;
		if (self->timer != 0)
			break;
		i = self->cycle;
		if (self->on != 0)
		{
			/* on -> off */
			self->on = 0;
			self->timer = self->offTime[i];
			if (self->timer != 0)
				break;
			self->on = 0;
			self->timer = 1;
		}
		else
		{
			/* off -> on, then the next step of the cycle */
			self->on = 1;
			self->timer = self->onTime[i];
			self->cycle = (short)((i + 1) % (int)self->numCycle);
			if (self->timer != 0)
				break;
			self->on = 0;
			self->timer = 1;
		}
	}
	if (self->on != 0)
	{
		for (i = 0; i < self->numTypes; i++)
		{
			self->accum[i] = DAT_0028fda8 * self->rate[i] + self->accum[i];
			while (self->accum[i] > 1.0f)
			{
				self->accum[i] = self->accum[i] - 1.0f;
				CMetaVent_Emit(self, i);
			}
		}
		if ((self->flags & 3) == 0 || MB_I32(DAT_0030564c, 0x68) != 1)
			return;
	}
	else
	{
		if ((self->flags & 5) == 0 || MB_I32(DAT_0030564c, 0x68) != 1)
			return;
	}
	for (i = 0; i < DAT_002bae94; i++)
		CMetaVent_Push(self, i, MB_FP(MB_I32(DAT_003075bc[i], 0x30), 0xd0), 0);
}

/* 0x0015e370  Meta (file unknown)
Updates every vent. */
void MetaVents_Update(void)
{
	struct CMetaVent *vent;

	for (vent = MB_LIST_NEXT(&DAT_00308e60, NULL); vent != NULL;
		vent = MB_LIST_NEXT(&DAT_00308e60, vent))
		CMetaVent_Update(vent);
}

/* 0x0015e3a0  Meta (file unknown)
Debug drawing of every vent (not called by the game). */
void MetaVents_DrawDebug(void)
{
	struct CMetaVent *vent;

	for (vent = MB_LIST_NEXT(&DAT_00308e60, NULL); vent != NULL;
		vent = MB_LIST_NEXT(&DAT_00308e60, vent))
		CMetaVent_DrawDebug(vent);
}

/* ---------- a timer that stops while the game is paused (0x0016bab0-0x0016bc60, after
   LevelLoad.cpp; used by the explosions' shock wave) */

/* 0x0016bab0  Meta (file unknown)
Resets the timer (stopped, duration 0); the tick rate comes from bKernel. Returns 1. */
int MbTimer_Reset(struct MbTimer *self)
{
	self->frequency = DAT_002cd7a0;
	self->running = 0;
	self->paused = 0;
	self->frozen = 0;
	self->start = 0;
	self->duration = 0.0f;
	self->remaining = 0.0f;
	return 1;
}

/* 0x0016bae0  Meta (file unknown)
Per frame: holds the timer while the game is paused, then computes the time left and
the progress (0..1). */
void MbTimer_Update(struct MbTimer *self)
{
	unsigned long long elapsed;
	double seconds;

	if (self->frozen != 0)
		return;
	if (self->paused != 0)
	{
		if (DAT_002ecdec == 0)
		{
			self->paused = 0;
			self->start = self->start + bkTimerDelta(self->pauseStart, bkTimerRead());
		}
		return;
	}
	if (DAT_002ecdec != 0)
	{
		self->paused = 1;
		self->pauseStart = bkTimerRead();
	}
	if (self->running == 0)
	{
		seconds = 0.0;
		self->progress = 0.0f;
	}
	else
	{
		elapsed = bkTimerDelta(self->start, bkTimerRead());
		if (self->durationMs == 0.0f)
		{
			self->progress = 1.0f;
			seconds = (double)(long long)elapsed / (double)self->frequency;
		}
		else
		{
			self->progress = (float)(((double)(long long)(elapsed * 1000ull) /
				(double)self->frequency) / (double)self->durationMs);
			seconds = (double)(long long)elapsed / (double)self->frequency;
		}
	}
	self->remaining = (float)((double)self->duration - seconds);
	if ((double)self->duration - seconds < 0.0)
		self->remaining = 0.0f;
	if (self->progress > 1.0f)
		self->progress = 1.0f;
}

/* 0x0016bc10  Meta (file unknown)
Sets the duration in seconds and stops the timer. */
void MbTimer_SetDuration(struct MbTimer *self, float seconds)
{
	self->duration = seconds;
	self->running = 0;
	self->durationMs = seconds * 1000.0f;           /* 0x00255898 */
}

/* 0x0016bc30  Meta (file unknown)
Starts the timer now. */
void MbTimer_Start(struct MbTimer *self)
{
	self->running = 1;
	self->start = bkTimerRead();
}

/* 0x0016bc50  Meta (file unknown) */
float MbTimer_GetRemaining(struct MbTimer *self)
{
	return self->remaining;
}

/* 0x0016bc60  Meta (file unknown) */
float MbTimer_GetProgress(struct MbTimer *self)
{
	return self->progress;
}

/* 0x00162f20  Meta (file unknown)
"return this": a trivial constructor that the linker shares between several classes
(callers in other modules too). */
void *Meta_ReturnThis(void *self)
{
	return self;
}

/* ---------- particle emitters (0x001635e0-0x00163c60, after CWater.cpp): lines, circles
   and water drip lines that the level script places. The list is at 0x003090d8. */

/* 0x003090d0  sin of a slowly turning phase (0x003090d4), updated with the emitters */
float DAT_003090d0;
float DAT_003090d4;
/* 0x003090d8  the emitters (count at 0x003090e0) */
struct MbList DAT_003090d8;
/* 0x002b0ca8  the phase's speed */
float DAT_002b0ca8 = 0.15f;

/* 0x001635e0  Meta (file unknown)
Sets the emit rate: a line eases towards it, a water line takes it at once, a circle
ignores it. */
void CMetaEmitter_SetRate(struct CMetaEmitter *self, float rate)
{
	if (self->type != 0)
	{
		if (self->type != 1)
			return;
		self->rate = rate;
	}
	self->targetRate = rate;
}

/* 0x00163600  Meta (file unknown)
A line emitter from `from` to `to` of particle `ptype` in `colour`; the rate is per unit
of length / 100. */
struct CMetaEmitter *CMetaEmitter_InitLine(struct CMetaEmitter *self, void *ptype,
	unsigned int colour, float *from, float *to, float rate, int flags, int arg)
{
	float dx;
	float dy;
	float dz;
	float len;

	self->p0[0] = from[0];
	self->p0[1] = from[1];
	self->p0[2] = from[2];
	self->p0[3] = from[3];
	self->p1[0] = to[0];
	self->p1[1] = to[1];
	self->p1[2] = to[2];
	self->p1[3] = to[3];
	self->vel[3] = 1.0f;
	self->vel[0] = 0.0f;
	self->vel[1] = 0.0f;
	self->vel[2] = 0.0f;
	self->accum = 0.0f;
	self->rate = rate;
	self->targetRate = rate;
	dx = from[0] - to[0];
	dy = from[1] - to[1];
	dz = from[2] - to[2];
	self->ptype = ptype;
	self->arg = arg;
	self->colour = colour;
	self->type = 0;
	self->flags = flags;
	len = sqrtf(dy * dy + dx * dx + dz * dz) * 0.01f;     /* 0x0025372c */
	self->lengthFactor = len;
	self->perFrame = len * rate;
	MB_LIST_INSERT(&DAT_003090d8, self, 0);
	return self;
}

/* 0x001636d0  Meta (file unknown)
A water drip line from `from` to `to`: splashes of `strength` on the first water area. */
struct CMetaEmitter *CMetaEmitter_InitWaterLine(struct CMetaEmitter *self, float strength,
	float *from, float *to, float rate, int flags, int arg)
{
	float dx;
	float dy;
	float dz;
	float len;

	self->p0[0] = from[0];
	self->p0[1] = from[1];
	self->p0[2] = from[2];
	self->p0[3] = from[3];
	self->p1[0] = to[0];
	self->p1[1] = to[1];
	self->p1[2] = to[2];
	self->p1[3] = to[3];
	self->vel[0] = 0.0f;
	self->vel[1] = 0.0f;
	self->vel[2] = 0.0f;
	self->vel[3] = 1.0f;
	self->spread[3] = 1.0f;
	self->spread[0] = 0.0f;
	self->spread[1] = 0.0f;
	self->spread[2] = 0.0f;
	self->accum = 0.0f;
	self->rate = rate;
	self->targetRate = rate;
	dx = from[0] - to[0];
	dy = from[1] - to[1];
	dz = from[2] - to[2];
	self->arg = arg;
	MB_F32(self, 0x68) = strength;
	self->type = 1;
	self->flags = flags;
	len = sqrtf(dy * dy + dx * dx + dz * dz) * 0.01f;
	self->lengthFactor = len;
	self->perFrame = len * rate;
	MB_LIST_INSERT(&DAT_003090d8, self, 0);
	return self;
}

/* 0x001637a0  Meta (file unknown)
A circle emitter of `radius` around `centre`. */
struct CMetaEmitter *CMetaEmitter_InitCircle(struct CMetaEmitter *self, void *ptype,
	unsigned int colour, float *centre, float radius, float rate, int flags, int arg)
{
	float len = radius * 0.06283185f;                      /* 0x00270210: 2 pi / 100 */

	self->type = 2;
	self->p0[0] = centre[0];
	self->p0[1] = centre[1];
	self->p0[2] = centre[2];
	self->p0[3] = centre[3];
	self->p1[0] = 0.0f;
	self->p1[1] = 0.0f;
	self->p1[2] = 0.0f;
	self->p1[3] = 1.0f;
	self->lengthFactor = len;
	self->p1[3] = 1.0f;
	self->p1[2] = radius;
	self->perFrame = len * rate;
	self->targetRate = rate;
	self->rate = rate;
	self->ptype = ptype;
	self->arg = arg;
	self->colour = colour;
	self->accum = 0.0f;
	self->flags = flags;
	MB_LIST_INSERT(&DAT_003090d8, self, 0);
	return self;
}

/* the velocity of a particle: random inside the spread */
static void emitter_velocity(struct CMetaEmitter *self, float *v)
{
	v[0] = Core_RandUnit() * self->spread[0] + self->vel[0];
	v[1] = Core_RandUnit() * self->spread[1] + self->vel[1];
	v[3] = 1.0f;
	v[2] = Core_RandUnit() * self->spread[2] + self->vel[2];
}

/* 0x00163840  Meta (file unknown)
Emits one particle (or a splash) at a random point of the line or circle; flag 1 puts it
on the first water area's surface, flag 2 gives it a velocity. */
void CMetaEmitter_Emit(struct CMetaEmitter *self)
{
	float pos[4];
	float vel[4];
	float r;
	float angle;

	if (self->type == 0)
	{
		r = (float)mb_rand() * MB_RAND_SCALE;
		FUN_00050ff0(pos, self->p0, r, self->p1, 1.0f - r);
		if ((self->flags & 1) != 0 && DAT_003090b8[0] != NULL)
			pos[1] = pos[1] + DAT_003090b8[0]->level;
		if ((self->flags & 2) != 0)
		{
			emitter_velocity(self, vel);
			FUN_00080d60((int)self->ptype, (undefined4)pos, (undefined4)vel, 0, self->colour);
			return;
		}
	}
	else if (self->type == 1)
	{
		r = (float)mb_rand() * MB_RAND_SCALE;
		FUN_00050ff0(pos, self->p0, r, self->p1, 1.0f - r);
		MetaWater_Splash(pos, MB_F32(self, 0x68));
		return;
	}
	else if (self->type == 2)
	{
		angle = (float)mb_rand() * MB_RAND_ANGLE;
		if (self->p1[2] == 0.0f)
		{
			pos[0] = self->p0[0];
			pos[1] = self->p0[1];
			pos[2] = self->p0[2];
			pos[3] = self->p0[3];
		}
		else
		{
			FUN_00051170(pos, self->p1, angle);
			pos[0] = pos[0] + self->p0[0];
			pos[1] = pos[1] + self->p0[1];
			pos[2] = pos[2] + self->p0[2];
		}
		if ((self->flags & 1) != 0 && DAT_003090b8[0] != NULL)
			pos[1] = pos[1] + DAT_003090b8[0]->level;
		if ((self->flags & 2) != 0)
		{
			emitter_velocity(self, vel);
			FUN_00051170(vel, vel, angle);
			FUN_00080d60((int)self->ptype, (undefined4)pos, (undefined4)vel, 0, self->colour);
			return;
		}
	}
	else
	{
		return;
	}
	FUN_00080d60((int)self->ptype, (undefined4)pos, (undefined4)DAT_002ea478, 0, self->colour);
}

/* 0x00163ae0  Meta (file unknown)
Gives the particles a velocity with a random spread. */
void CMetaEmitter_SetVelocity(struct CMetaEmitter *self, float *vel, float *spread)
{
	self->vel[0] = vel[0];
	self->vel[1] = vel[1];
	self->vel[2] = vel[2];
	self->vel[3] = vel[3];
	self->spread[0] = spread[0];
	self->spread[1] = spread[1];
	self->spread[2] = spread[2];
	self->spread[3] = spread[3];
	self->flags = self->flags | 2;
}

/* 0x00163b20  Meta (file unknown)
Deletes every emitter. */
void MetaEmitters_DeleteAll(void)
{
	void *e;

	e = MB_LIST_NEXT(&DAT_003090d8, NULL);
	while (e != NULL)
	{
		MB_LIST_REMOVE(&DAT_003090d8, e);
		if (e != NULL)
			MB_DELETE(e);
		e = MB_LIST_NEXT(&DAT_003090d8, NULL);
	}
}

/* 0x00163b60  Meta (file unknown)
Per frame: the rate eases to its target (a water line dies away), then emits. Returns the
emitter. */
struct CMetaEmitter *CMetaEmitter_Update(struct CMetaEmitter *self)
{
	float d;

	if (self->type == 0 || self->type == 2)
	{
		if (self->rate != self->targetRate)
		{
			d = self->targetRate - self->rate;
			if (d <= -0.005f || 0.005f <= d)      /* 0x00270214, 0x002632d8 */
				self->rate = DAT_0028fda8 * d * 0.04f + self->rate;   /* 0x0026fefc */
			else
				self->rate = self->targetRate;
			self->perFrame = self->lengthFactor * self->rate;
		}
	}
	else if (self->type == 1)
	{
		if (self->rate > 0.0f)
			self->rate = self->rate * 0.5f;
		if (self->rate < 0.1f)
			self->rate = 0.0f;
		self->perFrame = self->lengthFactor * self->rate;
	}
	if (self->rate > 0.0f)
	{
		self->accum = DAT_0028fda8 * self->perFrame + self->accum;
		while (self->accum > 1.0f)
		{
			self->accum = self->accum - 1.0f;
			CMetaEmitter_Emit(self);
		}
	}
	return self;
}

/* 0x00163c60  Meta (file unknown)
Per frame: the phase, then every emitter. */
void MetaEmitters_UpdateAll(void)
{
	struct CMetaEmitter *e;

	if (DAT_003090d8.count == 0)
		return;
	DAT_003090d4 = DAT_002b0ca8 * DAT_0028fda8 + DAT_003090d4;
	DAT_003090d4 = CLMath_WrapAngle(DAT_003090d4);
	DAT_003090d0 = sinf(DAT_003090d4);
	e = MB_LIST_NEXT(&DAT_003090d8, NULL);
	while (e != NULL)
	{
		e = CMetaEmitter_Update(e);
		e = MB_LIST_NEXT(&DAT_003090d8, e);
	}
}
