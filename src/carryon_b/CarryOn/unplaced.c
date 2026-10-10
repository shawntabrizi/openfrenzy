/*
CarryOn/unplaced.c  <-  C:\BlitzParty\Game\Src\CarryOn\ (gap functions, source file unknown)

- 0x000cb5d0-0x000cbcc0: the rockets that lie in the arena (CFire_Rocket; the arena of
  CFire_Arena.c owns them). A rocket is thrown in from the arena edge or dropped from the
  sky, bounces 4 times, then lies on the ground until a player picks it up.
- 0x000d19f0-0x000d1dd0: smoke puffs (CFire_Smoke, DAT_002f798c).
- 0x000d1e10-0x000d2270: the frame timer of the game (CFire_Timer, vtable 0x00268b2c) and the
  rectangle that its bar is drawn with.
- 0x000d2290-0x000d24a0: CGameMode_CarryOn (vtable 0x00268b38), the core game mode that
  forwards to carryon_a's game object DAT_00293ad8.
*/

#include "carryon_b.h"

#define CB_RAND01 3.0518509e-05f    /* 1/32767 (0x00253740) */

/* the player in slot i (CCarryOn_Player of carryon_a) and its position (body +0xd0) */
#define CB_PLAYER(i) (DAT_002f6d28[i])
#define CB_PLAYER_POS(p) CB_FP(CB_PTR(p, 0x30), 0xd0)
/* the rockets a player can carry: 5 in variant 0, else 1 */
#define CB_CARRY_LIMIT() ((CB_I32(DAT_002f6d5c, 0x250) == 0) * 4 + 1)

/* ======================================================================== rockets */

/* 0x000cb5d0  (gap) */
struct CFire_Rocket *CFire_Rocket_ctor(struct CFire_Rocket *self, int unused)
{
	(void)unused;
	memset(self, 0, sizeof *self);
	self->state = 0;
	return self;
}

/* 0x000cb5f0  (gap)
Picks a random spot in the arena (x, z in -1000..1000, y 2000) whose ground is flat:
the ground 80 units to each side is within 5 units of the ground below the spot.
Returns the ground height. */
float CFire_Rocket_RandomSpot(float *out)
{
	float spot[4];
	float probe[3];
	float drop, other;

	spot[3] = 1.0f;
	for (;;)
	{
		spot[0] = (float)cb_rand() * (2000.0f / 32767.0f) - 1000.0f;   /* 0x002687cc, 0x00255898 */
		spot[1] = 2000.0f;
		spot[2] = (float)cb_rand() * (2000.0f / 32767.0f) - 1000.0f;
		drop = CLColl_GroundProbe(spot, 0.0f, NULL, NULL, 0);
		if (drop == 1000000.0f)                                      /* 0x00265b60: no ground */
			continue;
		probe[0] = spot[0] - 80.0f;                                  /* 0x0026556c */
		probe[1] = spot[1];
		probe[2] = spot[2];
		other = CLColl_GroundProbe(probe, 0.0f, NULL, NULL, 0);
		if (other - drop < -5.0f || 5.0f < other - drop)             /* 0x002687c8, 0x002536c4 */
			continue;
		probe[0] = spot[0] + 80.0f;
		probe[1] = spot[1];
		probe[2] = spot[2];
		other = CLColl_GroundProbe(probe, 0.0f, NULL, NULL, 0);
		if (other - drop < -5.0f || 5.0f < other - drop)
			continue;
		probe[0] = spot[0];
		probe[1] = spot[1];
		probe[2] = spot[2] + 80.0f;
		other = CLColl_GroundProbe(probe, 0.0f, NULL, NULL, 0);
		if (other - drop < -5.0f || 5.0f < other - drop)
			continue;
		probe[0] = spot[0];
		probe[1] = spot[1];
		probe[2] = spot[2] - 80.0f;
		other = CLColl_GroundProbe(probe, 0.0f, NULL, NULL, 0);
		if (other - drop < -5.0f || 5.0f < other - drop)
			continue;
		break;
	}
	out[0] = spot[0];
	out[1] = spot[1];
	out[2] = spot[2];
	out[3] = spot[3];
	return spot[1] - drop;
}

/* 0x000cb840  (gap)
Drops a free rocket from the sky (y 2000) onto a random flat spot. */
void CFire_Rocket_Drop(struct CFire_Rocket *self, struct BActorInstance *actor)
{
	if (self->state != 0)
		return;
	self->phase = 0.0f;
	self->floorY = 2000.0f;
	self->groundY = CFire_Rocket_RandomSpot(self->pos);
	self->vel[3] = 0.0f;
	self->vel[2] = 0.0f;
	self->vel[1] = 0.0f;
	self->vel[0] = 0.0f;
	self->type = 5;
	self->state = 2;
	self->actor = actor;
}

/* 0x000cb890  (gap)
States: 0 free, 2-5 flying (each bounce on the ground moves to the next state), 6-7 lying.
A player that can carry more rockets and is near picks the rocket up. */
void CFire_Rocket_Update(struct CFire_Rocket *self, float dt)
{
	float radius, dx, dy, dz, vy, phase;
	int i;

	switch (self->state)
	{
	case 0:
		if (CB_I32(DAT_002f6d5c, 0x250) != 0 && CB_I32(DAT_002f6d5c, 0x250) != 2)
			self->type = 5;
		return;
	case 2:
	case 3:
	case 4:
	case 5:
		if (self->vel[0] != 0.0f || self->vel[2] != 0.0f)
			self->groundY = self->pos[1] - CLColl_GroundProbe(self->pos, 0.0f, NULL, NULL, 0);
		self->pos[0] = self->pos[0] + self->vel[0];
		self->pos[1] = self->pos[1] + self->vel[1];
		self->pos[2] = self->pos[2] + self->vel[2];
		vy = self->vel[1] - 1.5f;                                 /* 0x002538a0 gravity */
		self->vel[1] = vy;
		if (vy < 0.0f && self->pos[1] <= self->floorY + 50.0f && self->pos[1] <= self->groundY)
		{
			/* bounce */
			self->vel[1] = vy * -0.25f;                           /* 0x002687d4 */
			self->pos[1] = self->groundY;
			CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 0xd, -1,
				(6 - self->state) * 0x20);
			self->state++;
			self->phase = 0.0f;
		}
		if (self->pos[1] < -1000.0f)                              /* 0x002679f8 */
			self->state = 0;
		break;
	case 6:
	case 7:
		break;
	default:
		return;
	}

	/* the spin animation: lying rockets loop, flying ones run up to 21/53 */
	if (self->state == 6 || self->state == 7)
	{
		phase = dt * (1.0f / 30.0f) + self->phase;               /* 0x00255b64 */
		self->phase = phase;
		if (1.0f <= phase)
		{
			do
				phase = phase - 1.0f;
			while (1.0f <= phase);
			self->phase = phase;
		}
	}
	else
	{
		if (self->phase < 0.396226406f)                           /* 0x002687d0 */
			self->phase = dt * (1.0f / 30.0f) + self->phase;
		if (1.0f <= self->phase)
		{
			phase = self->phase;
			do
				phase = phase - 1.0f;
			while (1.0f <= phase);
			self->phase = phase;
		}
	}

	radius = 100.0f;                                              /* 0x00255544 */
	if (100.0f <= CB_F32(DAT_002f6d5c, 0x374))
		radius = CB_F32(DAT_002f6d5c, 0x374);
	for (i = 0; i < DAT_002bae94; i++)
	{
		if (CB_I32(CB_PLAYER(i), 0x1130) >= CB_CARRY_LIMIT())
			continue;
		dx = self->pos[0] - CB_PLAYER_POS(CB_PLAYER(i))[0];
		dy = self->pos[1] - CB_PLAYER_POS(CB_PLAYER(i))[1];
		dz = self->pos[2] - CB_PLAYER_POS(CB_PLAYER(i))[2];
		if (dx * dx + dz * dz < radius * radius && -100.0f < dy && dy < 100.0f)
		{
			self->state = 0;
			CB_CALL(FUN_000c0af0, int (*)(void *, int, int, int))(CB_PLAYER(i), 0, self->type, 1);
			return;
		}
	}
}

/* 0x000cbb80  (gap)
A player touches the rocket: picks it up if the player can carry more and is at its
height. Returns 1 when picked up. */
int CFire_Rocket_CheckCatch(struct CFire_Rocket *self, int player)
{
	int state = self->state;
	float dy;

	if (state >= 0 && (state < 2 || state == 8))
		return 0;
	if (CB_I32(CB_PLAYER(player), 0x1130) < CB_CARRY_LIMIT())
	{
		dy = self->pos[1] - CB_PLAYER_POS(CB_PLAYER(player))[1];
		if (-100.0f < dy && dy < 100.0f)                          /* 0x00267a00, 0x00255544 */
		{
			self->state = 0;
			CB_CALL(FUN_000c0af0, int (*)(void *, int, int, int))(CB_PLAYER(player), 0,
				self->type, 1);
			return 1;
		}
		if (dy < 0.0f && state == 6)
			self->state = 7;
	}
	return 0;
}

/* 0x000cbc40  (gap)
Throws the rocket from `pos` at `angle` (radians, around y) with horizontal `speed` and
upward speed `vy`. */
void CFire_Rocket_Throw(struct CFire_Rocket *self, float *pos, float angle, float speed, float vy,
	int type, struct BActorInstance *actor)
{
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->floorY = pos[1];
	self->vel[1] = vy;
	self->vel[3] = 0.0f;
	self->type = type;
	self->actor = actor;
	self->state = 2;
	self->vel[0] = sinf(angle) * speed;
	self->vel[2] = cosf(angle) * speed;
	CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 0xb, -1, 0x80);
}

/* 0x000cbcc0  (gap)
Draws the rocket model 75 units above its position (in variant 0 with the spin
animation), or a circle in the rocket type's colour when it has no model. */
void CFire_Rocket_Draw(struct CFire_Rocket *self, unsigned int passes)
{
	struct BActorInstance *actor = self->actor;
	void *anim;
	float circle;
	int start;

	if (actor != NULL)
	{
		CB_F32(actor, 0xc0) = self->pos[0];
		CB_F32(actor, 0xc4) = self->pos[1];
		CB_F32(actor, 0xc8) = self->pos[2];
		CB_F32(actor, 0xcc) = self->pos[3];
		CB_F32(self->actor, 0xc4) = CB_F32(self->actor, 0xc4) + 75.0f;   /* 0x00256748 */
		CB_F32(self->actor, 0xe8) = 0.0f;
		CB_F32(self->actor, 0xe4) = 0.0f;
		CB_F32(self->actor, 0xe0) = 0.0f;
		CB_F32(self->actor, 0xec) = 1.0f;
		anim = CB_PTR(DAT_002f6d5c, 0x2b4);
		if (CB_I32(DAT_002f6d5c, 0x250) == 0 && anim != NULL)
		{
			actor = self->actor;
			start = CB_I32(anim, 4);
			baSetNodeTime(actor, CB_ACTOR_ROOT(actor),
				(int)((float)(CB_I32(anim, 8) - start) * self->phase) + start, 1);
		}
		baPrepActorInstance(self->actor, 1);
	}
	switch (self->state)
	{
	case 0:
	case 1:
	case 8:
		return;
	case 7:
		circle = 40.0f;
		break;
	default:
		circle = 80.0f;
		break;
	}
	if (self->actor != NULL)
	{
		baDrawActorInstance(self->actor, (int)passes);
		return;
	}
	if ((passes & 1) == 0)
		return;
	/* carryon_a declares the third parameter `int flags`; the x86 code passes a float
	   (see the module report) */
	CB_DUAL(FUN_000c8230, void (*)(undefined4, undefined4, undefined4, undefined4),
		void (*)(float *, float, float, unsigned int),
		((undefined4)(uintptr_t)self->pos, cb_fbits(30.0f), cb_fbits(circle),
			DAT_002943b8[self->type]),
		(self->pos, 30.0f, circle, DAT_002943b8[self->type]));
}

/* ======================================================================== smoke */

/* 0x002f798c  the smoke */
struct CFire_Smoke *DAT_002f798c;

/* 0x000d19f0  (gap)
Moves, fades and grows the puffs; a puff whose life is over goes back to the free list. */
void CFire_Smoke_Update(struct CFire_Smoke *self, float unused)
{
	struct CFire_Puff **link = &self->used;
	struct CFire_Puff *puff = self->used;
	struct CFire_Puff *next;
	float life;

	(void)unused;
	while (puff != NULL)
	{
		life = puff->life - 1.0f;
		puff->life = life;
		puff->colour[0] = puff->colourVel[0] + puff->colour[0];
		puff->colour[1] = puff->colourVel[1] + puff->colour[1];
		puff->colour[2] = puff->colourVel[2] + puff->colour[2];
		puff->size = puff->grow + puff->size;
		puff->pos[0] = puff->vel[0] + puff->pos[0];
		puff->pos[1] = puff->pos[1] + puff->vel[1];
		puff->pos[2] = puff->pos[2] + puff->vel[2];
		next = puff->next;
		if (life <= 0.0f)
		{
			*link = next;
			puff->next = self->free;
			self->free = puff;
		}
		else
			link = &puff->next;
		puff = next;
	}
}

/* 0x000d1a80  (gap) */
void CFire_Smoke_Draw(struct CFire_Smoke *self)
{
	struct CFire_Puff *puff;
	unsigned int colour;

	bdSetWorldMatrix((float *)DAT_002855f0);
	CFire_QuadSprites_Begin(self->quads);
	for (puff = self->used; puff != NULL; puff = puff->next)
	{
		colour = DAT_002bd640[(int)puff->colour[2]] | DAT_002bc490[(int)puff->colour[1]];
		colour |= DAT_002bce18[(int)puff->colour[0]];
		colour |= DAT_002bc898[(int)puff->alpha];
		CFire_QuadSprites_Add(self->quads, puff->pos, puff->size, colour);
	}
	CFire_QuadSprites_Flush(self->quads);
}

/* 0x000d1b10  (gap)
Adds a puff at `pos` spread by up to `spread` in x and z, rising, with about `size` and
`life` (frames); its grey shade fades to black over its life. */
void CFire_Smoke_Add(struct CFire_Smoke *self, float *pos, float size, float life, float spread)
{
	struct CFire_Puff *puff = self->free;
	float dx, dz, shade, inv;

	if (puff == NULL)
		return;
	self->free = puff->next;
	puff->next = self->used;
	self->used = puff;
	puff->pos[0] = pos[0];
	puff->pos[1] = pos[1];
	puff->pos[2] = pos[2];
	puff->pos[3] = pos[3];
	puff->pos[3] = 1.0f;
	puff->size = size * 0.75f + size * 0.25f * (float)cb_rand() * CB_RAND01;   /* 0x00253844, 0x00257b88 */
	dx = (float)cb_rand() * (2.0f / 32767.0f) - 1.0f;                       /* 0x00268b28 */
	dz = (float)cb_rand() * (2.0f / 32767.0f) - 1.0f;
	puff->vel[0] = 0.0f;
	puff->pos[0] = dx * spread + puff->pos[0];
	puff->pos[1] = spread * 0.0f + puff->pos[1];
	puff->pos[2] = spread * dz + puff->pos[2];
	puff->vel[1] = (float)cb_rand() * (4.0f / 32767.0f) + 4.0f;             /* 0x00266d8c, 0x00257c40 */
	puff->vel[2] = 0.0f;
	puff->unk20 = 0.0f;
	if (dx < 0.0f)
		dx = -dx;
	if (dz < 0.0f)
		dz = -dz;
	puff->alpha = 255.0f;
	shade = (1.0f - (dz + dx) * 0.5f) * (float)cb_rand() * CB_RAND01;
	puff->life = ((1.0f - shade) * (float)cb_rand() * CB_RAND01 + 1.0f) * life;
	puff->size = puff->size * 0.5f;
	puff->grow = (size / puff->life) * -0.5f;                                /* 0x00257b8c */
	puff->colour[0] = (float)cb_rand() * (64.0f / 32767.0f) + 8.0f;          /* 0x00268b24, 0x0025552c */
	puff->colour[1] = puff->colour[0] * shade;
	puff->colour[2] = (float)cb_rand() * puff->colour[1] * CB_RAND01;
	inv = 1.0f / puff->life;
	puff->colourVel[0] = -(inv * puff->colour[0]);
	puff->colourVel[1] = -(inv * puff->colour[1]);
	puff->colourVel[2] = -(inv * puff->colour[2]);
}

/* 0x000d1dd0  (gap) */
void CFire_Smoke_Free(struct CFire_Smoke *self)
{
	struct CFire_QuadSprites *quads;

	DAT_002f798c = NULL;
	bkHeapFree(self->puffs);
	self->puffs = NULL;
	quads = self->quads;
	if (quads != NULL)
	{
		/* the quads are freed by the point sprites' Free (same leading layout) */
		CFire_PointSprites_Free((struct CFire_PointSprites *)quads);
		bkHeapFree(quads);
	}
}

/* ======================================================================== frame timer */

/* 0x00268b2c  the vtable of CFire_Timer */
const void *const vtable_00268b2c[1] = {
	(const void *)CFire_Timer_dtor_del,   /* +0x00 FUN_000d2270 */
};

/* 0x000d1e10  (gap) */
struct CFire_Timer *CFire_Timer_ctor(struct CFire_Timer *self)
{
	self->vtbl = vtable_00268b2c;
	self->ticksPerSecond = 0;
	self->now = 0;
	self->last = 0;
	self->ms = 0.0f;
	self->frames = 0.0f;
	return self;
}

/* 0x000d1e50  (gap) */
void CFire_Timer_dtor(struct CFire_Timer *self)
{
	self->vtbl = vtable_00268b2c;
}

/* 0x000d1e60  (gap) */
int CFire_Timer_Init(struct CFire_Timer *self)
{
	unsigned long long now;

	self->ticksPerSecond = (unsigned long long)DAT_002cd7a0;
	now = bkTimerRead();
	self->now = now;
	self->last = now;
	self->marks = 0;
	return 1;
}

/* 0x000d1eb0  (gap)
Starts a frame: the time since the last frame in ms and in 60 Hz frames. */
void CFire_Timer_Frame(struct CFire_Timer *self)
{
	long long ms1000;
	float ms;

	self->marks = 0;
	self->now = bkTimerRead();
	ms1000 = (long long)(bkTimerDelta(self->last, self->now) * 1000u);
	self->last = self->now;
	ms = (float)((long double)ms1000 / (long double)(long long)self->ticksPerSecond);
	self->ms = ms;
	self->frames = ms * 0.0600000024f;                    /* 0x00268b30 */
}

/* 0x000d1f50  (gap)  ends a timed section (0-4) */
void CFire_Timer_End(struct CFire_Timer *self, int section)
{
	struct CFire_TimerSection *s;
	long long ms1000;

	if (section >= 5)
		return;
	s = &self->sections[section];
	s->now = bkTimerRead();
	ms1000 = (long long)(bkTimerDelta(s->start, s->now) * 1000u);
	s->ms = (float)((long double)ms1000 / (long double)(long long)self->ticksPerSecond);
}

/* 0x000d1fc0  (gap)  marks the time since the frame start with a colour (at most 50) */
void CFire_Timer_Mark(struct CFire_Timer *self, unsigned int colour)
{
	long long ms1000;

	if (self->marks >= 50)
		return;
	ms1000 = (long long)(bkTimerDelta(self->last, bkTimerRead()) * 1000u);
	self->markColours[self->marks] = colour;
	self->markTimes[self->marks] =
		(float)((long double)ms1000 / (long double)(long long)self->ticksPerSecond);
	self->marks++;
}

/* 0x002f7990  the rectangle's 4 vertices (FVF 0x152: position, normal, colour, uv) */
static struct {
	float pos[3];
	float normal[3];
	unsigned int colour;
	float uv[2];
} DAT_002f7990[4];

/* 0x000d2040  (gap)
Draws a screen-space rectangle (projection mode 0) of colour ARGB `colour`; alpha
blending only when the colour is not opaque. */
void CFire_DrawRect(float x, float y, float w, float h, unsigned int colour)
{
	unsigned int c;

	bdSetProjectionMode(0);
	bdSetTexture(0, NULL);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(5, 0, 0);
	if (colour >> 24 == 0xff)
		bdSetRenderState(6, 0, 0);
	else
	{
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 0, 0);
	}
	bdSetWorldMatrix((float *)DAT_002855f0);
	DAT_002f7990[1].pos[0] = x + w;
	DAT_002f7990[0].pos[0] = x;
	DAT_002f7990[2].pos[0] = x;
	DAT_002f7990[2].pos[1] = y + h;
	DAT_002f7990[1].pos[1] = y;
	DAT_002f7990[0].pos[1] = y;
	c = DAT_002bd640[colour & 0xff] | DAT_002bc490[colour >> 8 & 0xff] |
		DAT_002bce18[colour >> 16 & 0xff] | DAT_002bc898[colour >> 24];
	DAT_002f7990[0].colour = c;
	DAT_002f7990[0].pos[2] = 0.0f;
	DAT_002f7990[1].pos[2] = 0.0f;
	DAT_002f7990[2].pos[2] = 0.0f;
	DAT_002f7990[3].pos[2] = 0.0f;
	DAT_002f7990[1].colour = c;
	DAT_002f7990[2].colour = c;
	DAT_002f7990[3].pos[0] = DAT_002f7990[1].pos[0];
	DAT_002f7990[3].pos[1] = DAT_002f7990[2].pos[1];
	DAT_002f7990[3].colour = c;
	bdDrawPrimitiveFVF(4, DAT_002f7990, 4, 0x152);
	bdSetRenderState(6, 0, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(5, 1, 0);
}

/* 0x000d21a0  (gap)
Draws the frame's marks as a bar at the bottom of the screen (x = ms * 0.015 * 400 - 200)
and two red ticks at x -200 and -100. */
void CFire_Timer_Draw(struct CFire_Timer *self)
{
	float prev, t;
	int i;

	if (self->marks == 0 || self->marks >= 50)
		return;
	prev = 0.0f;
	for (i = 0; i < self->marks; i++)
	{
		t = self->markTimes[i] * 0.015f;                     /* 0x00268b34 */
		CFire_DrawRect(t * 400.0f - 200.0f, -205.0f, (prev - t) * 400.0f, 10.0f,
			self->markColours[i]);                           /* 0x00255914, 0x002536f0 */
		prev = t;
	}
	CFire_DrawRect(-200.0f, -207.0f, 1.0f, 14.0f, 0xffff0000u);
	CFire_DrawRect(-100.0f, -207.0f, 1.0f, 14.0f, 0xffff0000u);
}

/* 0x000d2270  (gap)  the deleting destructor */
struct CFire_Timer *CFire_Timer_dtor_del(struct CFire_Timer *self, unsigned int flags)
{
	self->vtbl = vtable_00268b2c;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* ======================================================================== game mode */

/* 0x00268b38  the vtable of CGameMode_CarryOn (slots as core's CGameMode vtable) */
const void *const vtable_00268b38[13] = {
	(const void *)CGameMode_CarryOn_dtor_del,        /* +0x00 FUN_000d24a0 */
	(const void *)FUN_001d5950,                      /* +0x04 */
	(const void *)FUN_001d5950,                      /* +0x08 */
	(const void *)CGameMode_CarryOn_Popped,          /* +0x0c FUN_000d22e0 */
	(const void *)CGameMode_CarryOn_Pushed,          /* +0x10 FUN_000d22c0 */
	(const void *)CGameMode_CarryOn_Input,           /* +0x14 FUN_000d2360 */
	(const void *)CGameMode_CarryOn_Update,          /* +0x18 FUN_000d2370 */
	(const void *)CGameMode_CarryOn_Render,          /* +0x1c FUN_000d23b0 */
	(const void *)FUN_001ec260,                      /* +0x20 engine_misc */
	(const void *)FUN_00012930,                      /* +0x24 core_modes_a */
	(const void *)FUN_000128e0,                      /* +0x28 core_modes_a */
	(const void *)CGameMode_CarryOn_LoadingStarted,  /* +0x2c FUN_000d2400 */
	(const void *)CGameMode_CarryOn_Start,           /* +0x30 FUN_000d2430 */
};

#define CB_GAMEMODE_LOADSTATE(self) CB_I32(self, 0x14)
#define CB_GAMEMODE_FLAGS(self) CB_U32(self, 0x10)

/* 0x000d2290  (gap) */
struct CGameMode_CarryOn *CGameMode_CarryOn_ctor(struct CGameMode_CarryOn *self)
{
	CGameMode_ctor((struct CGameMode *)self, (char *)"CarryOn");
	CB_AT(self, 0, const void *) = vtable_00268b38;
	return self;
}

/* 0x000d22b0  (gap) */
void CGameMode_CarryOn_dtor(struct CGameMode_CarryOn *self)
{
	CB_AT(self, 0, const void *) = vtable_00268b38;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x000d22c0  (gap)  pushed: loads the voices and starts loading the game */
void CGameMode_CarryOn_Pushed(struct CGameMode_CarryOn *self)
{
	Core_LoadCharacterVoices(1);
	CFire_Game_BeginLoad(&DAT_00293ad8);
	CB_GAMEMODE_LOADSTATE(self) = 1;
}

/* 0x000d22e0  (gap)
popped: closes the packages (still loading) or shuts the game down and restores the clear
colour; prints the free 3D sound buffers. */
void CGameMode_CarryOn_Popped(struct CGameMode_CarryOn *self)
{
	DSCAPS caps;

	if (CB_I32(self, 0x04) == 0)
		CFire_Game_ClosePackages();
	else
	{
		CFire_Game_Shutdown(&DAT_00293ad8);
		FUN_001d5950();    /* an empty method of the game object (folded) */
		DAT_002ba940 = (int)self->savedColour[0];
		DAT_002ba944 = (int)self->savedColour[1];
		DAT_002ba948 = (int)self->savedColour[2];
		DAT_002ba94c = (int)self->savedColour[3];
	}
	IDirectSound_GetCaps((struct IDirectSound *)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", caps.dwFree3DBuffers);
}

/* 0x000d2360  (gap)  input: background loading of the game */
void CGameMode_CarryOn_Input(struct CGameMode_CarryOn *self, int active)
{
	(void)self;
	CFire_Game_BackgroundLoad(&DAT_00293ad8, active);
}

/* 0x000d2370  (gap) */
void CGameMode_CarryOn_Update(struct CGameMode_CarryOn *self, int active)
{
	if (active == 0 || CB_GAMEMODE_LOADSTATE(self) != 0)
		return;
	if (CB_I32(CB_PTR(CB_PLAYER(0), 0x78), 0x14) != 0)
		bkPrintf("Debug Breakpoint...\n");
	CFire_Game_Update(&DAT_00293ad8, active);
}

/* 0x000d23b0  (gap)  render: black clear colour, near/far 40/80000, then the game */
void CGameMode_CarryOn_Render(struct CGameMode_CarryOn *self, int active)
{
	if (active == 0 || CB_GAMEMODE_LOADSTATE(self) != 0)
		return;
	DAT_002ba948 = 0;
	DAT_002ba944 = 0;
	DAT_002ba940 = 0;
	DAT_002ba94c = 0xff;
	bdSetNearFar(40.0f, 80000.0f);
	CFire_Game_Draw(&DAT_00293ad8, active);
}

/* 0x000d2400  (gap) */
void CGameMode_CarryOn_LoadingStarted(struct CGameMode_CarryOn *self)
{
	(void)self;
	bkPrintf("=== Init One Time ===\n");
	CFire_Game_InitOnce(&DAT_00293ad8);
	CFire_Game_InitObjects(&DAT_00293ad8);
}

/* 0x000d2430  (gap)  start: saves the clear colour and sets it to black */
void CGameMode_CarryOn_Start(struct CGameMode_CarryOn *self)
{
	CFire_Game_Start(&DAT_00293ad8);
	CB_GAMEMODE_FLAGS(self) = CB_GAMEMODE_FLAGS(self) | 1;
	bdSetNearFar(40.0f, 80000.0f);
	self->savedColour[0] = (unsigned int)DAT_002ba940;
	self->savedColour[1] = (unsigned int)DAT_002ba944;
	self->savedColour[2] = (unsigned int)DAT_002ba948;
	self->savedColour[3] = (unsigned int)DAT_002ba94c;
	DAT_002ba948 = 0;
	DAT_002ba944 = 0;
	DAT_002ba940 = 0;
	DAT_002ba94c = 0xff;
}

/* 0x000d24a0  (gap)  the deleting destructor */
struct CGameMode_CarryOn *CGameMode_CarryOn_dtor_del(struct CGameMode_CarryOn *self,
	unsigned int flags)
{
	CGameMode_CarryOn_dtor(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}
