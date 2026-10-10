/*
ClassLib/CSeagulls.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\CSeagulls.cpp

A flock of seagulls (boids): each gull keeps away from close neighbours, aligns with them,
steers towards the flock's target, avoids obstacles (meta_b 0x0016bc70), and flaps or
glides by its pitch. 0x0016cc80 is the gap function right after the last CSeagulls.cpp
function and only drives the flock (file inferred).
*/

#include "classlib_fx.h"

static const char s_CSeagullsFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\CSeagulls.cpp";

/* meta_b 0x0016bc70: obstacle avoidance (gull, obstacle, params, steer, weight) */
#define GULL_Avoid ((int (*)(void *, void *, void *, float *, float *))FUN_0016bc70)

/* 0x0016bea0  ClassLib/CSeagulls.cpp
Adds an obstacle (the array grows by 4). */
void CSeagulls_AddObstacle(CSeagulls *self, int obstacle)
{
	int *grown;
	int cap;

	if (self->maxObstacles <= self->numObstacles) {
		cap = self->maxObstacles + 4;
		grown = CLF_Alloc(cap * 4, s_CSeagullsFile, 0x273, 0x2001);
		if (grown == NULL)
			return;
		memcpy(grown, self->obstacles, (size_t)(self->maxObstacles & 0x3fffffff) * 4);
		CLF_Free(self->obstacles);
		self->obstacles = grown;
		self->maxObstacles = cap;
	}
	self->obstacles[self->numObstacles] = obstacle;
	self->numObstacles = self->numObstacles + 1;
}

/* 0x0016bf20  ClassLib/CSeagulls.cpp
Constructor of a gull: its actor ("glide" and "flap" animations, flapping), a random start
in the box (with `onEdge`, on a random side of it), heading +z at cruise speed. */
void CGull_Init(CGull *self, CFlockParams *p, int onEdge, int model)
{
	float d[4];
	int r;

	memset(self, 0, 0x1a * 4);
	self->actor = (int)FUN_00036b20(NULL, (void *)model, 2, s_CSeagullsFile, 0x41);
	CLF_UpdateActor(self->actor, 1);
	self->glide = (int)CLF_FindAnim(self->actor, "glide");
	self->flap = (int)CLF_FindAnim(self->actor, "flap");
	self->flapping = 0;
	CLF_PlayAnim(self->actor, *(int *)(self->actor + 0x134), self->flap, 0x400, -1, 1, NULL, 0);
	if (onEdge == 0) {
		self->pos[0] = (p->boxMax[0] - p->boxMin[0]) * (float)CLF_Rand() * 3.05185094e-05f + p->boxMin[0];
		self->pos[2] = (p->boxMax[2] - p->boxMin[2]) * (float)CLF_Rand() * 3.05185094e-05f + p->boxMin[2];
	} else {
		r = CLF_Rand() & 3; /* the side */
		if (r == 0) {
			self->pos[0] = (p->boxMax[0] - p->boxMin[0]) * (float)CLF_Rand() * 3.05185094e-05f + p->boxMin[0];
			self->pos[2] = p->boxMin[2];
		} else if (r == 1) {
			self->pos[0] = (p->boxMax[0] - p->boxMin[0]) * (float)CLF_Rand() * 3.05185094e-05f + p->boxMin[0];
			self->pos[2] = p->boxMax[2];
		} else if (r == 2) {
			self->pos[2] = (p->boxMax[2] - p->boxMin[2]) * (float)CLF_Rand() * 3.05185094e-05f + p->boxMin[2];
			self->pos[0] = p->boxMin[0];
		} else {
			self->pos[2] = (p->boxMax[2] - p->boxMin[2]) * (float)CLF_Rand() * 3.05185094e-05f + p->boxMin[2];
			self->pos[0] = p->boxMax[0];
		}
	}
	d[0] = 0.0f;
	d[1] = 0.0f;
	d[2] = 0.0001f;
	self->pos[1] = (p->boxMax[1] - p->boxMin[1]) * (float)CLF_Rand() * 3.05185094e-05f + p->boxMin[1];
	/* four numbers are drawn and not used */
	CLF_Rand();
	CLF_Rand();
	CLF_Rand();
	CLF_Rand();
	FUN_00051570(self->dir, d, 1.0f);
	self->heading = 0.0f;
	self->pitchSpeed = 0.0f;
	self->roll = 0.0f;
	self->yawRate = 0.0f;
	self->speed = p->speed;
}

/* 0x0016c1f0  ClassLib/CSeagulls.cpp
Release: the gulls' actors and the arrays. */
void CSeagulls_Release(CSeagulls *self)
{
	int i;

	if (self->gulls != NULL) {
		for (i = 0; i < self->count; i++)
			FUN_00034930((void *)self->gulls[i].actor);
		CLF_Free(self->gulls);
		self->gulls = NULL;
	}
	if (self->obstacles != NULL) {
		CLF_Free(self->obstacles);
		self->obstacles = NULL;
	}
	self->numObstacles = 0;
	self->maxObstacles = 0;
}

/* 0x0016c260  ClassLib/CSeagulls.cpp
Neighbours: weights 1 - d^2 / r^2 between every pair within the radius; each pair adds a
separation push (positive inside the separation fraction) and the other's direction. */
void CSeagulls_Neighbours(CSeagulls *self)
{
	float *w;
	float r2;
	float k;
	float d[4];
	float dn[4];
	float dx;
	float dy;
	float dz;
	float f;
	float s;
	CGull *a;
	CGull *b;
	int n;
	int i;
	int j;

	n = self->count;
	r2 = self->p.radius * self->p.radius;
	k = 1.0f / (1.0f - self->p.separation);
	w = CLF_Alloc(n * n * 4, s_CSeagullsFile, 0x12e, 0x2001);
	if (w == NULL)
		return;
	for (i = 0; i < self->count; i++) {
		a = &self->gulls[i];
		for (j = (ushort)(i + 1); j < self->count; j = (ushort)(j + 1)) {
			b = &self->gulls[j];
			dx = a->pos[0] - b->pos[0];
			dy = a->pos[1] - b->pos[1];
			dz = a->pos[2] - b->pos[2];
			f = r2 - (dx * dx + dy * dy + dz * dz);
			s = 0.0f;
			if (0.0f <= f)
				s = f / r2;
			w[j * n + i] = s;
			w[i * n + j] = s;
		}
		w[(n + 1) * i] = 0.0f;
		a->align[0] = 0.0f;
		a->align[1] = 0.0f;
		a->align[2] = 0.0f;
		a->align[3] = 1.0f;
		a->steer[0] = 0.0f;
		a->steer[1] = 0.0f;
		a->steer[2] = 0.0f;
		a->steer[3] = 1.0f;
		a->neighbours = 0;
	}
	for (i = 0; i < self->count; i++) {
		a = &self->gulls[i];
		for (j = (ushort)(i + 1); j < self->count; j = (ushort)(j + 1)) {
			s = w[j + i * n];
			b = &self->gulls[j];
			if (0.0f < s) {
				d[0] = a->pos[0] - b->pos[0];
				d[1] = a->pos[1] - b->pos[1];
				d[2] = a->pos[2] - b->pos[2];
				FUN_00051570(dn, d, 1.0f);
				f = s - self->p.separation;
				if (f <= 0.0f)
					f = 0.0f;
				else
					f = f * k;
				if (0.0f < s - (1.0f - self->p.separation))
					f = f - (1.0f - f) * s;
				a->steer[0] = dn[0] * f + a->steer[0];
				a->steer[1] = dn[1] * f + a->steer[1];
				a->steer[2] = dn[2] * f + a->steer[2];
				b->steer[0] = b->steer[0] - dn[0] * f;
				b->steer[1] = b->steer[1] - dn[1] * f;
				b->steer[2] = b->steer[2] - dn[2] * f;
				a->align[0] = s * b->dir[0] + a->align[0];
				a->align[1] = s * b->dir[1] + a->align[1];
				a->align[2] = s * b->dir[2] + a->align[2];
				b->align[0] = s * a->dir[0] + b->align[0];
				b->align[1] = s * a->dir[1] + b->align[1];
				b->align[2] = s * a->dir[2] + b->align[2];
				a->neighbours = a->neighbours + 1;
				b->neighbours = b->neighbours + 1;
			}
		}
	}
	CLF_Free(w);
}

/* 0x0016c5a0  ClassLib/CSeagulls.cpp
Update: steering, pitch and speed, banking, flap/glide animation, then moves each gull
and places its actor. */
void CSeagulls_Update(CSeagulls *self, float dt)
{
	CGull *g;
	float t;
	float f;
	float s;
	float dot;
	float cross;
	float turn;
	float pitch;
	float weight;
	float steer[4];
	float flat[4];
	float flatN[4];
	float toTarget[4];
	float toTargetN[4];
	float m[16];
	int i;
	int k;

	CSeagulls_Neighbours(self);
	t = dt * 0.416666687f;
	for (i = 0; i < self->count; i++) {
		g = &self->gulls[i];
		weight = 1.0f;
		if (g->neighbours != 0) {
			f = 1.0f / (float)g->neighbours;
			g->align[0] = (f * g->align[0] - g->dir[0]) * 1.5f;
			g->align[1] = (f * g->align[1] - g->dir[1]) * 1.5f;
			g->align[2] = (f * g->align[2] - g->dir[2]) * 1.5f;
		}
		steer[0] = g->align[0] + g->steer[0];
		steer[1] = g->align[1] + g->steer[1];
		steer[2] = g->align[2] + g->steer[2];
		toTarget[0] = self->target[0] - g->pos[0];
		toTarget[1] = self->target[1] - g->pos[1];
		toTarget[2] = self->target[2] - g->pos[2];
		FUN_00051570(toTargetN, toTarget, 0.5f);
		steer[0] = toTargetN[0] + steer[0];
		steer[1] = toTargetN[1] + steer[1];
		steer[2] = toTargetN[2] + steer[2];
		for (k = 0; k < self->numObstacles; k++)
			GULL_Avoid(g, (void *)self->obstacles[k], &self->p, steer, &weight);
		if (steer[1] <= 0.00999999978f) {
			if (-0.00999999978f <= steer[1]) {
				g->pitchSpeed = g->pitchSpeed * 0.980000019f;
			} else {
				f = t * self->p.turnRate + g->pitchSpeed;
				g->pitchSpeed = f;
				if (0.800000012f < f)
					g->pitchSpeed = 0.800000012f;
			}
		} else {
			f = g->pitchSpeed - t * self->p.turnRate;
			g->pitchSpeed = f;
			if (f < -0.800000012f)
				g->pitchSpeed = -0.800000012f;
		}
		f = g->speed - g->pitchSpeed * self->p.climb;
		g->speed = f;
		s = (f - self->p.speed) * 0.99000001f + self->p.speed;
		g->speed = s;
		f = self->p.speed * self->p.minSpeed;
		if (s < f)
			g->speed = f;
		f = self->p.maxSpeed * self->p.speed;
		if (f < g->speed)
			g->speed = f;
		flat[0] = steer[0];
		flat[2] = steer[2];
		flat[3] = 0.0f; /* uninitialised in the original; the length ignores it */
		flat[1] = 0.0f;
		FUN_00051570(flatN, flat, 1.0f);
		FUN_00051570(steer, g->dir, 1.0f);
		dot = flatN[0] * steer[0] + flatN[1] * steer[1] + flatN[2] * steer[2];
		if (0.699999988f < dot) {
			dot = dot - 0.699999988f;
			g->speed = 0.00499999989f * dot + g->speed;
		}
		if (dot < -0.699999988f) {
			f = 0.0199999996f * dot + g->speed;
			g->speed = f;
			s = self->p.speed * self->p.minSpeed;
			if (f < s)
				g->speed = s;
		}
		cross = flatN[0] * steer[2] - flatN[2] * steer[0];
		turn = (1.0f - dot) * self->p.bankRate * weight * 0.5f;
		if (cross <= 0.0500000007f) {
			if (-0.0500000007f <= cross)
				g->yawRate = g->yawRate * 0.980000019f;
			else
				g->yawRate = (g->yawRate * 19.0f - turn) * 0.0500000007f;
		} else {
			g->yawRate = (g->yawRate * 19.0f + turn) * 0.0500000007f;
		}
		g->heading = t * g->yawRate + g->heading;
		g->roll = t * g->yawRate * -5.0f;
		if (g->pitchSpeed <= 0.200000003f) {
			if (-0.200000003f <= g->pitchSpeed) {
				pitch = g->pitchSpeed;
				if (-0.00999999978f < pitch)
					goto glide;
			} else {
				pitch = -0.2f;
			}
			if (g->flapping == 0) {
				g->flapping = 1;
				FUN_00038190((void *)g->actor, *(void **)(g->actor + 0x134), 1, 0);
				CLF_PlayAnimBlend(g->actor, *(int *)(g->actor + 0x134), g->flap, 0x400, -1, 0.100000001f,
				                  1, NULL, 0);
			}
		} else {
			pitch = 0.2f;
glide:
			if (g->flapping == 1) {
				g->flapping = 0;
				FUN_00038190((void *)g->actor, *(void **)(g->actor + 0x134), 1, 0);
				CLF_PlayAnimBlend(g->actor, *(int *)(g->actor + 0x134), g->glide, 0x400, -1, 0.100000001f,
				                  1, NULL, 0);
			}
		}
		CLF_UpdateAnim(g->actor, *(int *)(g->actor + 0x134), dt, 0);
		FUN_00050ba0(m, pitch, g->heading, g->roll);
		g->dir[0] = m[8];
		g->dir[1] = m[9];
		g->dir[2] = m[10];
		g->dir[3] = m[11];
		g->pos[0] = t * g->dir[0] * g->speed + g->pos[0];
		g->pos[1] = t * g->dir[1] * g->speed + g->pos[1];
		g->pos[2] = t * g->dir[2] * g->speed + g->pos[2];
		FUN_00050ba0(m, -pitch, g->heading + 3.1415925f, -g->roll);
		FUN_00051700((float *)(g->actor + 0xe0), m);
		*(float *)(g->actor + 0xc0) = g->pos[0];
		*(float *)(g->actor + 0xc4) = g->pos[1];
		*(float *)(g->actor + 0xc8) = g->pos[2];
		*(float *)(g->actor + 0xcc) = g->pos[3];
		*(float *)(g->actor + 0xd8) = self->p.scale;
		*(float *)(g->actor + 0xd4) = *(float *)(g->actor + 0xd8);
		*(float *)(g->actor + 0xd0) = *(float *)(g->actor + 0xd4);
	}
}

/* 0x0016cb30  ClassLib/CSeagulls.cpp
Picks a new random target in the box. */
void CSeagulls_PickTarget(CSeagulls *self)
{
	self->target[0] = (self->p.boxMax[0] - self->p.boxMin[0]) * (float)CLF_Rand() * 3.05185094e-05f +
	                  self->p.boxMin[0];
	self->target[1] = (self->p.boxMax[1] - self->p.boxMin[1]) * (float)CLF_Rand() * 3.05185094e-05f +
	                  self->p.boxMin[1];
	self->target[2] = (self->p.boxMax[2] - self->p.boxMin[2]) * (float)CLF_Rand() * 3.05185094e-05f +
	                  self->p.boxMin[2];
}

/* 0x0016cbf0  ClassLib/CSeagulls.cpp
Constructor: `count` gulls of actor `model` starting on the sides of the box, parameters
`params` (or the defaults). The gulls read `params` itself (NULL is not handled there). */
CSeagulls *CSeagulls_Init(CSeagulls *self, int count, CFlockParams *params, int model)
{
	CFlockParams *src;
	int i;

	memset(self, 0, 0x1c * 4);
	src = params;
	if (params == NULL)
		src = &DAT_002b2670;
	memcpy(&self->p, src, sizeof(CFlockParams));
	self->model = model;
	self->count2 = count;
	self->count = count;
	self->gulls = CLF_Alloc(count * 0x68, s_CSeagullsFile, 0xe9, 0x1001);
	for (i = 0; i < count; i++)
		CGull_Init(&self->gulls[i], params, 1, model);
	CSeagulls_PickTarget(self);
	return self;
}

/* 0x0016cc80  ClassLib (gap; file inferred: CSeagulls.cpp)
Per-frame: now and then (1 in 200) a new target, then the update. */
void CSeagulls_Tick(CSeagulls *self, float dt)
{
	if ((float)CLF_Rand() * 3.05185094e-05f < 0.00499999989f)
		CSeagulls_PickTarget(self);
	CSeagulls_Update(self, dt);
}
