/*
Rowing/unplaced_body.c  <-  gap functions of C:\BlitzParty\Game\Src\Rowing\ (0x001a5750-0x001a73c0)

The floating bodies of the jet boat game: every boat and every floating object (rowing_b's
floaties, buoys, mines) owns a body (vtable 0x00275854; one slot, the deleting destructor)
that bobs on the water, follows the current, slides around obstacles and turns with the
water's surface. All bodies are on one list (0x0030f41c, linked through +4). The bodies
avoid a table of round colliders (0x0030f420, 0x38 bytes each).

A body (fields at original offsets; the boat's body is at +0x2a8 of the boat):
  +0x00 vtable 0x00275854  +0x04 next body  +0x08 kind (1: no spin damping)
  +0x0c position (4 floats)  +0x1c rotation (axis-angle; +0x20 the heading)
  +0x2c, +0x30, +0x34 the bob offset  +0x3c the roll  +0x4c the bob phase
  +0x54 1.0  +0x58 the spin  +0x5c velocity (x, y, z)  +0x6c the move of this frame
  +0x7c the current (x, y, z)  +0x8c the water normal (x, y, z), +0x98  +0x9c the
  water height  +0xa0 above the water  +0xa1 sunk  +0xa2 fell off  +0xa3 frozen
  +0xb0 the actor instance (its +0xc0 position and +0xe0 rotation quaternion are set)

A collider (0x38 bytes): +0x00 kind (0-3: a boat), +0x04 on, +0x08 radius, +0x0c a copy
of the position, +0x1c the position (float *), +0x20 an impulse to add to (float *),
+0x24 the radius for bodies, +0x30 a hit test callback, +0x34 its context.
*/

#include "rowing_a.h"

#define BODY_F32(off) RA_F32(self, off)
#define BODY_I32(off) RA_I32(self, off)
#define BODY_U8(off) RA_U8(self, off)

/* the shared wave (.bss): phase, (unused), speed, amplitude, (unused) */
#define g_WavePhase RW_VAR(float, 0x0030f408)
#define g_WaveSpeed RW_VAR(float, 0x0030f410)
#define g_WaveAmplitude RW_VAR(float, 0x0030f418)
/* the body list */
#define g_BodyList RW_VAR(void *, 0x0030f41c)
/* the colliders: array, count, capacity */
#define g_Colliders RW_VAR(unsigned char *, 0x0030f420)
#define g_ColliderCount RW_VAR(int, 0x0030f424)
#define g_ColliderMax RW_VAR(int, 0x0030f428)
#define RA_COLLIDER(i) (g_Colliders + (i) * 0x38)

/* the game object's fields used here */
#define g_GameFlags RW_VAR(unsigned int, 0x0030f0b0)
#define g_GameState RW_VAR(int, 0x0030f0b4)
#define g_GameTimer RW_VAR(void *, 0x0030f148)
#define g_GameWater RW_VAR(void *, 0x0030f130)
#define RA_FRAME_MS (RA_F32(g_GameTimer, 0x98) * 0.0303030312f)

/* 0x00275854  vtable of a body (rebuilt from the XBE) */
const void *const vtable_00275854[1] = {
	(const void *)FUN_001a59d0, /* +0x00 deleting destructor */
};

/* 0x001a5750  Rowing (gap)
A body's constructor. */
void FUN_001a5750(void *self)
{
	*(const void *const **)self = vtable_00275854;
	BODY_I32(0xb0) = 0;
}

/* 0x001a5770  Rowing (gap)
A body's destructor. */
void FUN_001a5770(void *self)
{
	*(const void *const **)self = vtable_00275854;
}

/* 0x001a5780  Rowing (gap)
Takes the body off the body list. */
void FUN_001a5780(void *self)
{
	void **link = RA_GPTR(void *, 0x0030f41c);

	if (*link == NULL)
		return;
	while (*link != self) {
		link = (void **)RA_P(*link, 4);
		if (*link == NULL)
			break;
	}
	if (*link != NULL)
		*link = RA_PTRF(self, 4);
}

/* 0x001a57b0  Rowing (gap)
Sets the shared wave: phase, speed and amplitude (B and E are stored but not used). */
void FUN_001a57b0(float phase, float b, float amplitude, float speed, float e)
{
	g_WavePhase = phase;
	RW_VAR(float, 0x0030f40c) = b;
	g_WaveAmplitude = amplitude;
	g_WaveSpeed = speed;
	RW_VAR(float, 0x0030f414) = e;
}

/* 0x001a57f0  Rowing (gap)
Once per frame: the wave phase moves on by its speed and wraps at 2 pi (in 24-bit fixed
point). */
void FUN_001a57f0(void)
{
	unsigned int n = (unsigned int)(long long)((g_WaveSpeed + g_WavePhase) * 0.159154937f * 16777216.0f)
		& 0xffffff;

	g_WavePhase = (float)(int)n * 5.96046448e-08f * 6.28318548f;
}

/* 0x001a5830  Rowing (gap)
Turns quaternion FROM towards TO by at most MAXANGLE (OUT may be FROM). Returns 1 when
OUT reached TO. */
int FUN_001a5830(float *out, float *from, float *to, float maxAngle)
{
	float sum, diff, dot, angle;

	sum = (from[3] + to[3]) * (from[3] + to[3]) + (from[2] + to[2]) * (from[2] + to[2]) +
		(from[1] + to[1]) * (from[1] + to[1]) + (from[0] + to[0]) * (from[0] + to[0]);
	diff = (from[3] - to[3]) * (from[3] - to[3]) + (from[2] - to[2]) * (from[2] - to[2]) +
		(from[1] - to[1]) * (from[1] - to[1]) + (from[0] - to[0]) * (from[0] - to[0]);
	dot = from[0] * to[0] + from[1] * to[1] + to[3] * from[3] + from[2] * to[2];
	if (!(diff < sum))
		dot = -dot;
	if (dot < 1.0f)
		angle = fabsf((float)acos((double)dot));
	else
		angle = 0.0f;
	if (angle > maxAngle) {
		bmQuaternionSlerp(out, from, to, maxAngle / angle);
		return 0;
	}
	out[0] = to[0];
	out[1] = to[1];
	out[2] = to[2];
	out[3] = to[3];
	return 1;
}

/* 0x001a5950  Rowing (gap)
Empties the collider table. */
void FUN_001a5950(void)
{
	g_Colliders = NULL;
	g_ColliderCount = 0;
	g_ColliderMax = 0;
}

/* 0x001a5970  Rowing (gap)
Frees the collider table. */
void FUN_001a5970(void)
{
	free(g_Colliders);
	g_Colliders = NULL;
	g_ColliderCount = 0;
	g_ColliderMax = 0;
}

/* 0x001a5990  Rowing (gap)
Switches collider INDEX on or off. */
void FUN_001a5990(int index, int on)
{
	RA_I32(RA_COLLIDER(index), 4) = on;
}

/* 0x001a59b0  Rowing (gap)
Sets collider INDEX's position and impulse pointers. */
void FUN_001a59b0(int index, float *pos, float *impulse)
{
	unsigned char *c = RA_COLLIDER(index);

	RA_PTRF(c, 0x1c) = pos;
	RA_PTRF(c, 0x20) = impulse;
}

/* 0x001a59d0  Rowing (gap)
A body's deleting destructor (vtable slot 0). */
void *FUN_001a59d0(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00275854;
	if (flags & 1)
		RA_DELETE(self);
	return self;
}

/* 0x001a59f0  Rowing (gap)
Sets a body up for actor instance INST at POS with rotation ROT (axis-angle) and puts it
on the body list; the bob phase is random. */
int FUN_001a59f0(void *self, void *inst, float *pos, float *rot, int kind)
{
	int r = rw_rand();

	BODY_I32(0x08) = kind;
	BODY_F32(0x2c) = 0.0f;
	BODY_F32(0x30) = 0.0f;
	BODY_F32(0x34) = 0.0f;
	BODY_F32(0x3c) = 0.0f;
	BODY_F32(0x40) = 0.0f;
	BODY_F32(0x44) = 0.0f;
	RA_PTRF(self, 0xb0) = inst;
	BODY_F32(0x4c) = (float)r * 0.000191753454f;
	BODY_F32(0x0c) = pos[0];
	BODY_F32(0x10) = pos[1];
	BODY_F32(0x14) = pos[2];
	BODY_F32(0x18) = pos[3];
	BODY_F32(0x1c) = rot[0];
	BODY_F32(0x20) = rot[1];
	BODY_F32(0x24) = rot[2];
	BODY_F32(0x28) = rot[3];
	BODY_F32(0x58) = 0.0f;
	BODY_F32(0x5c) = 0.0f;
	BODY_F32(0x60) = 0.0f;
	BODY_F32(0x64) = 0.0f;
	BODY_U8(0xa0) = 0;
	BODY_U8(0xa2) = 0;
	BODY_U8(0xa1) = 0;
	BODY_U8(0xa3) = 0;
	BODY_F32(0x54) = 1.0f;
	RA_PTRF(self, 4) = g_BodyList;
	g_BodyList = self;
	return 1;
}

/* 0x001a5ad0  Rowing (gap)
The bob: the height offset follows the shared wave, the roll lags it by a quarter turn. */
void FUN_001a5ad0(void *self)
{
	float a = g_WavePhase + BODY_F32(0x4c);

	BODY_F32(0x30) = sinf(a) * g_WaveAmplitude;
	BODY_F32(0x3c) = (sinf(a - 1.57079625f) - 0.5f) * 0.039999999f;
}

/* the body has sunk (or left the course): no water under it */
static void ra_Sink(void *self)
{
	BODY_U8(0xa1) = 1;
	BODY_F32(0x9c) = -100000.0f;
}

/* 0x001a5b00  Rowing (gap)
The water at the body: the current (+0x7c), the surface normal (+0x8c) and height (+0x9c)
for the variant: flat water (variants 1, 2), the whirlpool (variant 3: a current around
and into the centre, sinking inside radius 100), the waterfall (variant 4: the current
over the edge, the slopes of the lip and the foot). Sets +0xa0 when the body is above the
water. */
void FUN_001a5b00(void *self)
{
	float x, z, dist, strength, tangent[3], flow[3], v[3], t, a;

	switch ((g_GameFlags >> 2) & 3) {
	case 0:
		BODY_F32(0x7c) = 0.0f;
		BODY_F32(0x80) = 0.0f;
		BODY_F32(0x84) = 0.0f;
		BODY_F32(0x8c) = 0.0f;
		BODY_F32(0x90) = 1.0f;
		BODY_F32(0x94) = 0.0f;
		BODY_F32(0x9c) = -51.0f;
		break;
	case 1:
		BODY_F32(0x7c) = 0.0f;
		BODY_F32(0x80) = 0.0f;
		BODY_F32(0x84) = 0.0f;
		BODY_F32(0x8c) = 0.0f;
		BODY_F32(0x90) = 1.0f;
		BODY_F32(0x94) = 0.0f;
		BODY_F32(0x9c) = -10.0f;
		break;
	case 2:
		/* the whirlpool: a current around the centre and one into it */
		z = BODY_F32(0x14);
		x = BODY_F32(0x0c);
		dist = sqrtf(z * z + x * x);
		if (!(dist >= 1.0f)) {
			dist = 1.0f;
			strength = 0.0f;
		} else if (!(dist >= 100.0f)) {
			strength = 0.0f;
		} else {
			strength = RW_VAR(float, 0x0030f0dc);
		}
		tangent[0] = -z;
		tangent[1] = 0.0f;
		tangent[2] = x;
		bmVectorSetLength(flow, tangent, strength);
		v[0] = flow[0] + BODY_F32(0x0c);
		v[1] = 0.0f;
		v[2] = flow[2] + BODY_F32(0x14);
		BODY_F32(0x84) = flow[2];
		BODY_F32(0x7c) = flow[0];
		BODY_F32(0x80) = 0.0f;
		dist = dist - 3.0f;
		if (dist <= 1.0f)
			dist = 1.0f;
		bmVectorSetLength(flow, v, dist);
		BODY_F32(0x7c) = flow[0] - BODY_F32(0x0c);
		BODY_F32(0x84) = flow[2] - BODY_F32(0x14);
		v[0] = -x;
		v[1] = 0.0f;
		v[2] = -z;
		bmVectorSetLength(flow, v, (RW_VAR(float, 0x0030f0e0) / dist) * strength);
		BODY_F32(0x7c) = flow[0] + BODY_F32(0x7c);
		BODY_F32(0x84) = flow[2] + BODY_F32(0x84);
		BODY_F32(0x9c) = FUN_001a4b70(g_GameWater, x, z, RA_FP(self, 0x8c));
		if (dist < 100.0f || BODY_U8(0xa1) != 0)
			ra_Sink(self);
		break;
	case 3:
		/* the waterfall: the current over the edge grows near the lip */
		BODY_F32(0x7c) = 0.0f;
		BODY_F32(0x80) = 0.0f;
		t = RW_VAR(float, 0x0030f0d8);
		if (BODY_F32(0x14) > -230.0f && t < 200.0f && BODY_I32(0x08) == 0)
			BODY_F32(0x84) = -t - (200.0f - (-30.0f - BODY_F32(0x14))) * (200.0f - t) * 0.0049999999f;
		else
			BODY_F32(0x84) = -t;
		BODY_F32(0x8c) = 0.0f;
		BODY_F32(0x90) = 1.0f;
		BODY_F32(0x94) = 0.0f;
		BODY_F32(0x9c) = 0.0f;
		if (BODY_U8(0xa1) != 0) {
			ra_Sink(self);
			break;
		}
		if (!(BODY_F32(0x10) < 150.0f)) {
			if (!(BODY_F32(0x14) < 30.0f)) {
				/* on the upper river */
				BODY_F32(0x90) = 1.0f;
				BODY_F32(0x94) = 0.0f;
				BODY_F32(0x9c) = 300.0f;
				break;
			}
			if (!(BODY_F32(0x14) < -30.0f)) {
				/* on the lip */
				BODY_F32(0x9c) = 300.0f;
				a = (30.0f - BODY_F32(0x14)) * 0.0261799376f;
				BODY_F32(0x94) = sinf(a);
				BODY_F32(0x90) = cosf(a);
				break;
			}
		}
		if (!(BODY_F32(0x14) < -1800.0f)) {
			BODY_F32(0x90) = 1.0f;
			BODY_F32(0x94) = 0.0f;
			BODY_F32(0x9c) = 0.0f;
			break;
		}
		if (!(BODY_F32(0x14) < -1860.0f)) {
			/* the foot */
			BODY_F32(0x9c) = 0.0f;
			a = (1800.0f - BODY_F32(0x14)) * 0.0261799376f;
			BODY_F32(0x94) = sinf(a);
			BODY_F32(0x90) = cosf(a);
			break;
		}
		ra_Sink(self);
		break;
	}
	BODY_U8(0xa0) = BODY_F32(0x10) <= BODY_F32(0x9c) ? 0 : 1;
}

/* 0x001a5f00  Rowing (gap)
A body's frame (DT in frames): the water (FUN_001a5b00), the spin and heading, the
vertical speed (sinks above the water, floats up below it), on the water: the spin and
velocity damping towards the current (JetHandling tweaks), the bob and roll; the move of
this frame (+0x6c), the turn towards the water's surface and the spin of the instance. */
void FUN_001a5f00(void *self, float dt)
{
	void *inst;
	float h, k, vy, dx, dz, speed, damp, s, c, ms_dt;
	float nX, nY, nZ, A, B, f0, f1, f2, r0, r1, r2, m0, m1, m2;
	float m[16], q[4], old[4];
	unsigned int n;

	FUN_001a5b00(self);
	n = (unsigned int)(long long)((dt * BODY_F32(0x58) + BODY_F32(0x20)) * 0.159154937f * 16777216.0f)
		& 0xffffff;
	BODY_F32(0x20) = (float)(int)n * 5.96046448e-08f * 6.28318548f;
	inst = RA_PTRF(self, 0xb0);
	RA_F32(inst, 0xc0) = BODY_F32(0x0c);
	RA_F32(inst, 0xc4) = BODY_F32(0x10);
	RA_F32(inst, 0xc8) = BODY_F32(0x14);
	RA_F32(inst, 0xcc) = BODY_F32(0x18);
	/* the vertical speed */
	h = BODY_F32(0x10) - BODY_F32(0x9c);
	if (h > -9.99999975e-05f && h < 9.99999975e-05f && BODY_F32(0x60) > -9.99999975e-05f &&
			BODY_F32(0x60) < 9.99999975e-05f) {
		BODY_F32(0x60) = 0.0f;
	} else {
		k = h * 0.1f + 0.5f;
		if (k > 1.0f)
			k = 1.0f;
		else if (k < 0.0f)
			k = 0.0f;
		BODY_F32(0x60) = BODY_F32(0x60) - RA_FRAME_MS * 7.0f;
		vy = (1.0f - k) * RA_FRAME_MS * 11.2f + BODY_F32(0x60);
		BODY_F32(0x60) = vy;
		if (k < 1.0f)
			BODY_F32(0x60) = vy * 0.80000001f;
	}
	if (BODY_F32(0x10) < -1500.0f)
		BODY_U8(0xa2) = 1;
	if (BODY_U8(0xa0) == 0) {
		/* on the water: damping */
		if (BODY_I32(0x08) != 1)
			BODY_F32(0x58) = (float)pow((double)RW_VAR(float, 0x002b5670), (double)dt) * BODY_F32(0x58);
		dx = BODY_F32(0x5c) - BODY_F32(0x7c);
		dz = BODY_F32(0x64) - BODY_F32(0x84);
		speed = sqrtf(dz * dz + dx * dx);
		if (speed < RW_VAR(float, 0x002b5664))
			damp = speed * ((RW_VAR(float, 0x002b5660) - RW_VAR(float, 0x002b565c)) / RW_VAR(float, 0x002b5664)) +
				RW_VAR(float, 0x002b565c);
		else
			damp = (RW_VAR(float, 0x002b5668) - RW_VAR(float, 0x002b5660)) * (speed - RW_VAR(float, 0x002b5664)) /
				(RW_VAR(float, 0x002b566c) - RW_VAR(float, 0x002b5664)) + RW_VAR(float, 0x002b5660) +
				RW_VAR(float, 0x002b565c);
		if (damp > 0.89999998f)
			damp = 0.89999998f;
		k = (float)pow((double)(1.0f - damp), (double)dt);
		BODY_F32(0x5c) = (BODY_F32(0x5c) - BODY_F32(0x7c)) * k + BODY_F32(0x7c);
		BODY_F32(0x64) = (BODY_F32(0x64) - BODY_F32(0x84)) * k + BODY_F32(0x84);
		/* the bob */
		inst = RA_PTRF(self, 0xb0);
		RA_F32(inst, 0xc0) = BODY_F32(0x2c) + RA_F32(inst, 0xc0);
		RA_F32(inst, 0xc4) = RA_F32(inst, 0xc4) + BODY_F32(0x30);
		RA_F32(inst, 0xc8) = RA_F32(inst, 0xc8) + BODY_F32(0x34);
		bmQuaternionRotationX(q, BODY_F32(0x3c));
		inst = RA_PTRF(self, 0xb0);
		bmQuaternionMultiply(RA_FP(inst, 0xe0), RA_FP(inst, 0xe0), q);
	}
	if (BODY_U8(0xa3) != 0) {
		BODY_F32(0x6c) = 0.0f;
		BODY_F32(0x5c) = 0.0f;
		BODY_F32(0x70) = 0.0f;
		BODY_F32(0x60) = 0.0f;
		BODY_F32(0x74) = 0.0f;
		BODY_F32(0x64) = 0.0f;
	}
	BODY_F32(0x6c) = RA_FRAME_MS * BODY_F32(0x5c);
	BODY_F32(0x70) = RA_FRAME_MS * BODY_F32(0x60);
	BODY_F32(0x74) = RA_FRAME_MS * BODY_F32(0x64);
	if (BODY_U8(0xa0) == 0 || g_GameState < 5) {
		/* turn towards the water's surface: a frame from the heading and the normal */
		s = sinf(BODY_F32(0x20));
		c = cosf(BODY_F32(0x20));
		BODY_F32(0x8c) = -BODY_F32(0x8c);
		BODY_F32(0x94) = -BODY_F32(0x94);
		nX = BODY_F32(0x8c);
		nY = BODY_F32(0x90);
		nZ = BODY_F32(0x94);
		A = c * nZ + nY * 0.0f + -s * nX;
		B = s * nZ + nY * 0.0f + c * nX;
		if (B > A) {
			m0 = c * nY - nZ * 0.0f;
			m1 = -s * nZ - c * nX;
			m2 = nX * 0.0f - -s * nY;
			f0 = m1 * nZ - m2 * nY;
			f1 = m2 * nX - m0 * nZ;
			f2 = m0 * nY - m1 * nX;
		} else {
			r0 = nZ * 0.0f - s * nY;
			r1 = s * nX - c * nZ;
			r2 = c * nY - nX * 0.0f;
			m0 = r2 * nY - r1 * nZ;
			m1 = r0 * nZ - r2 * nX;
			m2 = r1 * nX - r0 * nY;
			f0 = r0;
			f1 = r1;
			f2 = r2;
		}
		memcpy(m, DAT_002855f0, sizeof m);
		m[0] = m0;
		m[1] = m1;
		m[2] = m2;
		m[3] = 1.0f;
		m[4] = nX;
		m[5] = nY;
		m[6] = nZ;
		m[7] = BODY_F32(0x98);
		m[8] = f0;
		m[9] = f1;
		m[10] = f2;
		m[11] = 1.0f;
		bmMatrixToQuaternion(q, m);
		inst = RA_PTRF(self, 0xb0);
		FUN_001a5830(RA_FP(inst, 0xe0), RA_FP(inst, 0xe0), q, dt * 0.2f);
	}
	ms_dt = dt * BODY_F32(0x58);
	bmQuaternionRotationY(q, ms_dt);
	inst = RA_PTRF(self, 0xb0);
	old[0] = RA_F32(inst, 0xe0);
	old[1] = RA_F32(inst, 0xe4);
	old[2] = RA_F32(inst, 0xe8);
	old[3] = RA_F32(inst, 0xec);
	bmQuaternionMultiply(RA_FP(inst, 0xe0), q, old);
}

/* a collider's hit test: (context, who, segment, &t, impulse out) */
typedef int (*RaHitTest)(void *ctx, void *who, float *seg, float *t, float *out);

/* 0x001a6500  Rowing (gap)
Slides the body's move (+0x6c) against the course outlines (COUNT outlines at OUTLINES:
count, points, closed): the circle of RADIUS is swept along the move; on the first hit the
move stops there, the body is pushed out to RADIUS + 0.1 from the wall and the velocity
into the wall is removed (x 1.5 + 0.01). Returns 1 when the move was cut. In variant 4 a
body that would fall beyond z -2500 loses its z speed. */
int FUN_001a6500(void *self, int count, unsigned char *outlines, float radius)
{
	float circle[3], move[2], dir[2], seg[4], best, wall[4], margin, len;
	float minX, maxX, minZ, maxZ, p[2], q[4], closest[2], foot[2], perp, along;
	float circle2[3], move2[2], offX, offZ, newPos[3], rr, push[3], n[3], vdot;
	struct BHit2D hit;
	float *a, *b;
	int k, i, n2;
	unsigned char *outline;

	margin = radius * 1.04999995f;
	circle[0] = BODY_F32(0x0c);
	circle[1] = BODY_F32(0x14);
	circle[2] = radius;
	best = 1000.0f;
	if (BODY_F32(0x6c) > -9.99999975e-05f && BODY_F32(0x6c) < 9.99999975e-05f &&
			BODY_F32(0x74) > -9.99999975e-05f && BODY_F32(0x74) < 9.99999975e-05f) {
		BODY_F32(0x6c) = 0.0f;
		BODY_F32(0x74) = 0.0f;
		return 0;
	}
	move[1] = BODY_F32(0x74);
	move[0] = BODY_F32(0x6c);
	hit.flags = 9;
	len = 1.0f / sqrtf(move[1] * move[1] + move[0] * move[0]);
	dir[0] = move[0] * len;
	dir[1] = len * move[1];
	if (BODY_F32(0x6c) < 0.0f) {
		minX = BODY_F32(0x6c) + BODY_F32(0x0c) - margin;
		maxX = margin + BODY_F32(0x0c);
	} else {
		minX = BODY_F32(0x0c) - margin;
		maxX = BODY_F32(0x6c) + BODY_F32(0x0c) + margin;
	}
	if (BODY_F32(0x74) < 0.0f) {
		minZ = BODY_F32(0x74) + BODY_F32(0x14) - margin;
		maxZ = margin + BODY_F32(0x14);
	} else {
		minZ = BODY_F32(0x14) - margin;
		maxZ = margin + BODY_F32(0x74) + BODY_F32(0x14);
	}
	if (count <= 0)
		goto no_hit;
	for (k = 0; k < count; k++) {
		outline = outlines + k * 0xc;
		if (RA_U8(outline, 8) == 0) {
			a = (float *)RA_PTRF(outline, 4);
			n2 = RA_I32(outline, 0) - 2;
			b = a + 2;
		} else {
			b = (float *)RA_PTRF(outline, 4);
			n2 = RA_I32(outline, 0) - 1;
			a = b + RA_I32(outline, 0) * 2 - 2;
		}
		for (i = n2; i >= 0; i--, a = b, b += 2) {
			if (!(a[0] >= minX || b[0] >= minX))
				continue;
			if (!(a[0] <= maxX || b[0] <= maxX))
				continue;
			if (!(a[1] >= minZ || b[1] >= minZ))
				continue;
			if (!(a[1] <= maxZ || b[1] <= maxZ))
				continue;
			seg[0] = a[0];
			seg[1] = a[1];
			seg[2] = b[0];
			seg[3] = b[1];
			if (bmCircleSweepSegment(circle, seg, move, &hit) == 0) {
				/* no hit while moving: is the circle already on the wall? */
				p[0] = BODY_F32(0x0c);
				p[1] = BODY_F32(0x14);
				if (FUN_001b9c30(seg, p, q, closest, foot, &along) < radius) {
					best = 0.0f;
					memcpy(wall, seg, sizeof wall);
				}
				continue;
			}
			{
				float t1 = hit.t;

				offX = dir[0] * 0.1f;
				circle2[2] = radius;
				p[0] = offX + circle[0];
				offZ = dir[1] * 0.1f;
				p[1] = offZ + circle[1];
				if (FUN_001b9c30(seg, p, q, closest, &perp, foot) <= radius) {
					best = t1;
					memcpy(wall, seg, sizeof wall);
					continue;
				}
				move2[0] = move[0] - offX;
				move2[1] = move[1] - offZ;
				circle2[0] = offX + circle[0];
				circle2[1] = offZ + circle[1];
				if (bmCircleSweepSegment(circle2, seg, move2, &hit) != 0 && t1 < best) {
					best = t1;
					memcpy(wall, seg, sizeof wall);
				}
			}
		}
	}
	if (best <= 1.0f) {
		newPos[0] = best * BODY_F32(0x6c) + BODY_F32(0x0c);
		newPos[1] = best * BODY_F32(0x70) + BODY_F32(0x10);
		newPos[2] = best * BODY_F32(0x74) + BODY_F32(0x14);
		rr = radius + 0.1f;
		p[0] = newPos[0];
		p[1] = newPos[2];
		if (FUN_001b9c30(wall, p, closest, q, foot, &along) < rr) {
			push[0] = closest[0] - newPos[0];
			push[1] = 0.0f;
			push[2] = closest[1] - newPos[2];
			bmVectorSetLength(n, push, rr);
			newPos[0] = closest[0] - n[0];
			newPos[1] = newPos[1] - n[1];
			newPos[2] = closest[1] - n[2];
			bmVectorSetLength(n, push, 1.0f);
			vdot = n[0] * BODY_F32(0x5c) + n[1] * BODY_F32(0x60) + n[2] * BODY_F32(0x64);
			if (vdot >= 0.0f) {
				vdot = vdot * 1.5f + 0.0099999998f;
				BODY_F32(0x5c) = BODY_F32(0x5c) - n[0] * vdot;
				BODY_F32(0x60) = BODY_F32(0x60) - n[1] * vdot;
				BODY_F32(0x64) = BODY_F32(0x64) - n[2] * vdot;
			}
		}
		BODY_F32(0x6c) = newPos[0] - BODY_F32(0x0c);
		BODY_F32(0x70) = newPos[1] - BODY_F32(0x10);
		BODY_F32(0x74) = newPos[2] - BODY_F32(0x14);
		return 1;
	}
no_hit:
	if ((g_GameFlags & 0xc) == 0xc && BODY_F32(0x74) + BODY_F32(0x14) < -2500.0f) {
		BODY_F32(0x64) = 0.0f;
		BODY_F32(0x74) = 0.0f;
	}
	return 0;
}

/* the segment across a collider facing the line from it to POINT: from the collider's
centre CENTRE (x, -, z) at RADIUS / cos(pi/4), 45 degrees either side */
static void ra_ColliderEdge(float *edge, const float *point, const float *centre, float r)
{
	float a = atan2f(point[0] - centre[0], point[1] - centre[2]);
	float d = r / (float)cos((double)0.785398126f);

	edge[0] = sinf(a - 0.785398126f) * d + centre[0];
	edge[1] = cosf(a - 0.785398126f) * d + centre[2];
	edge[2] = sinf(a + 0.785398126f) * d + centre[0];
	edge[3] = cosf(a + 0.785398126f) * d + centre[2];
}

/* 0x001a6b50  Rowing (gap)
Where SEG (x0, z0, x1, z1) crosses the edge of a round obstacle of RADIUS at CENTRE:
*T is the fraction along SEG (1 when it does not cross). Returns the hit. */
int FUN_001a6b50(float *seg, float *centre, float radius, float *t)
{
	float edge[4];
	struct BHit2D hit;
	int r;

	hit.flags = 1;
	ra_ColliderEdge(edge, seg, centre, radius);
	r = bmSegmentSegment(seg, edge, &hit);
	*t = r != 0 ? hit.t : 1.0f;
	return r;
}

/* 0x001a6bf0  Rowing (gap)
The first collider (but SKIP) that SEG (x0, z0, x1, z1) of WHO hits: its own hit test,
or its round edge (boats: radius 60). *T is the fraction along SEG; *IMPULSE gets the
collider's impulse (or 0). Returns 1 on a hit. */
int FUN_001a6bf0(void *who, float *seg, float *t, float *impulse, int skip)
{
	unsigned char *c, *bestC;
	float best, hitT, minX, maxX, minZ, maxZ, edge[4], *p;
	struct BHit2D hit;
	int i, r;

	best = 1.0f;
	hit.flags = 1;
	bestC = NULL;
	if (seg[0] > seg[2]) {
		minX = seg[2];
		maxX = seg[0];
	} else {
		minX = seg[0];
		maxX = seg[2];
	}
	if (seg[1] > seg[3]) {
		minZ = seg[3];
		maxZ = seg[1];
	} else {
		minZ = seg[1];
		maxZ = seg[3];
	}
	for (i = 0; i < g_ColliderCount; i++) {
		c = RA_COLLIDER(i);
		if (i == skip || RA_I32(c, 4) == 0)
			continue;
		p = (float *)RA_PTRF(c, 0x1c);
		if (!(minX <= RA_F32(c, 8) + p[0] && p[0] - RA_F32(c, 8) <= maxX && minZ <= p[2] + RA_F32(c, 8) &&
				p[2] - RA_F32(c, 8) <= maxZ))
			continue;
		if (RA_PTRF(c, 0x30) == NULL) {
			if (RA_I32(c, 0) < 0 || RA_I32(c, 0) > 3)
				ra_ColliderEdge(edge, seg, p, RA_F32(c, 0x24));
			else
				ra_ColliderEdge(edge, seg, p, 60.0f);
			r = bmSegmentSegment(seg, edge, &hit);
			if (r == 0)
				continue;
			hitT = hit.t;
		} else {
			r = ((RaHitTest)RA_PTRF(c, 0x30))(RA_PTRF(c, 0x34), who, seg, &hitT, impulse);
		}
		if (r != 0 && hitT < best) {
			best = hitT;
			bestC = c;
		}
	}
	*t = best;
	if (best >= 1.0f)
		return 0;
	if (RA_PTRF(bestC, 0x20) != NULL) {
		p = (float *)RA_PTRF(bestC, 0x20);
		impulse[0] = p[0];
		impulse[1] = p[1];
		impulse[2] = p[2];
		impulse[3] = p[3];
		return 1;
	}
	impulse[3] = 0.0f;
	impulse[2] = 0.0f;
	impulse[1] = 0.0f;
	impulse[0] = 0.0f;
	return 1;
}

/* 0x001a6e40  Rowing (gap)
Debug: draws the colliders' circles (green: radius, red: body radius, yellow: boats) at
least 20 above the water. */
void FUN_001a6e40(void)
{
	unsigned char *c;
	float pos[4], floor;
	int i;

	for (i = 0; i < g_ColliderCount; i++) {
		c = RA_COLLIDER(i);
		if (RA_I32(c, 4) == 0)
			continue;
		memcpy(pos, RA_PTRF(c, 0x1c), sizeof pos);
		floor = RA_F32(g_GameWater, 8) + 20.0f;
		if (pos[1] <= floor)
			pos[1] = floor;
		FUN_001afba0(pos, RA_F32(c, 8), 0, 0x7f, 0);
		FUN_001afba0(pos, RA_F32(c, 0x24), 0x7f, 0, 0);
		if (RA_I32(c, 0) >= 0 && RA_I32(c, 0) < 4)
			FUN_001afba0(pos, 60.0f, 0x7f, 0x7f, 0);
	}
}

/* 0x001a6f20  Rowing (gap)
Copies collider INDEX's position into its +0x0c. */
void FUN_001a6f20(int index)
{
	unsigned char *c = RA_COLLIDER(index);
	float *p = (float *)RA_PTRF(c, 0x1c);

	RA_F32(c, 0x0c) = p[0];
	RA_F32(c, 0x10) = p[1];
	RA_F32(c, 0x14) = p[2];
	RA_F32(c, 0x18) = p[3];
}

/* 0x001a6f50  Rowing (gap)
Pushes the body (radius RADIUS) off the colliders (but SKIP): a move that would end
inside one stops at its edge (+ 0.1); the velocity towards it is removed (the boats of the
waterfall bounce harder) and the collider gets the matching impulse. Returns 1 when the
move was cut. */
int FUN_001a6f50(void *self, float radius, int skip)
{
	float margin, minX, maxX, minZ, maxZ, newX, newZ, segA[4], segB[4], p[2], d, rr, s;
	float dirX, dirZ, len, q[2], foot[2], perp, along, diff[3], n[3], vdot, a, b;
	unsigned char *c;
	float *cp;
	int i, cut;

	margin = radius * 1.04999995f;
	if (BODY_F32(0x6c) < 0.0f) {
		newX = BODY_F32(0x6c) + BODY_F32(0x0c);
		minX = newX - margin;
		maxX = margin + BODY_F32(0x0c);
	} else {
		minX = BODY_F32(0x0c) - margin;
		newX = BODY_F32(0x6c) + BODY_F32(0x0c);
		maxX = newX + margin;
	}
	if (BODY_F32(0x74) < 0.0f) {
		newZ = BODY_F32(0x74) + BODY_F32(0x14);
		minZ = newZ - margin;
		maxZ = margin + BODY_F32(0x14);
	} else {
		minZ = BODY_F32(0x14) - margin;
		newZ = BODY_F32(0x74) + BODY_F32(0x14);
		maxZ = margin + newZ;
	}
	cut = 0;
	if (!(fabsf(BODY_F32(0x6c)) >= 1e-4f) && !(fabsf(BODY_F32(0x74)) >= 1e-4f)) {
		BODY_F32(0x6c) = 0.0f;
		BODY_F32(0x74) = 0.0f;
		return 0;
	}
	len = 1.0f / sqrtf(BODY_F32(0x6c) * BODY_F32(0x6c) + BODY_F32(0x74) * BODY_F32(0x74));
	dirX = len * BODY_F32(0x6c);
	dirZ = len * BODY_F32(0x74);
	segA[0] = BODY_F32(0x0c);
	segA[1] = BODY_F32(0x14);
	segA[2] = newX;
	segA[3] = newZ;
	for (i = 0; i < g_ColliderCount; i++) {
		c = RA_COLLIDER(i);
		if (RA_I32(c, 4) == 0 || i == skip)
			continue;
		cp = (float *)RA_PTRF(c, 0x1c);
		if (!(cp[0] + RA_F32(c, 0x24) >= minX) || !(cp[0] - RA_F32(c, 0x24) <= maxX) ||
				!(cp[2] + RA_F32(c, 0x24) >= minZ) || !(cp[2] - RA_F32(c, 0x24) <= maxZ))
			continue;
		p[0] = cp[0];
		p[1] = cp[2];
		d = FUN_001b9c30(segA, p, q, foot, &perp, &along);
		rr = radius + RA_F32(c, 0x24);
		if (!(d < rr))
			continue;
		rr = rr + 0.1f;
		s = along - sqrtf(rr * rr - perp * perp);
		segB[0] = dirX * 0.1f + segA[0];
		segB[1] = dirZ * 0.1f + segA[1];
		segB[2] = segA[2];
		segB[3] = segA[3];
		if (FUN_001b9c30(segB, p, q, foot, &perp, &along) < radius + RA_F32(c, 0x24)) {
			cut = 1;
			segA[2] = dirX * s + segA[0];
			segA[3] = dirZ * s + segA[1];
		}
		Core_Vec3Sub(diff, cp, RA_FP(self, 0x0c));
		bmVectorSetLength(n, diff, 1.0f);
		vdot = n[2] * BODY_F32(0x64) + n[1] * BODY_F32(0x60) + n[0] * BODY_F32(0x5c);
		if (vdot < 0.0f)
			continue;
		if ((g_GameFlags & 0xc) == 0xc) {
			if (BODY_I32(0x08) == 0 && RA_I32(c, 0) >= 0 && RA_I32(c, 0) <= 3) {
				a = vdot * 1.5f + 0.5f;
				b = a;
			} else {
				a = (BODY_I32(0x08) == 0 ? vdot : vdot * 1.5f) + 0.5f;
				if (RA_I32(c, 0) >= 0 && RA_I32(c, 0) <= 3)
					b = vdot + 0.5f;
				else
					b = vdot * 1.5f + 0.5f;
			}
		} else {
			a = vdot + 0.0099999998f;
			b = a;
		}
		BODY_F32(0x5c) = BODY_F32(0x5c) - n[0] * a;
		BODY_F32(0x60) = BODY_F32(0x60) - n[1] * a;
		BODY_F32(0x64) = BODY_F32(0x64) - n[2] * a;
		if (RA_PTRF(c, 0x20) != NULL) {
			float *imp = (float *)RA_PTRF(c, 0x20);

			imp[0] = n[0] * b + imp[0];
			imp[1] = n[1] * b + imp[1];
			imp[2] = n[2] * b + imp[2];
		}
	}
	if (cut == 0)
		return 0;
	BODY_F32(0x6c) = segA[2] - BODY_F32(0x0c);
	BODY_F32(0x74) = segA[3] - BODY_F32(0x14);
	return 1;
}

/* 0x001a73c0  Rowing (gap)
Adds a collider (kind, radius, position, impulse, body radius, hit test and its context);
the table grows by doubling (at least 8). Returns its index, or -1. */
int FUN_001a73c0(int kind, float radius, float *pos, float *impulse, float bodyRadius, void *hitTest,
	void *context)
{
	unsigned char *table = g_Colliders, *c;
	int max = g_ColliderMax, index;

	if (g_ColliderCount == g_ColliderMax) {
		max = g_ColliderMax * 2;
		if (max < 8)
			max = 8;
		table = (unsigned char *)realloc(g_Colliders, max * 0x38);
		if (table == NULL) {
			bkPrintf("memory problemo\n");
			return -1;
		}
	}
	g_ColliderMax = max;
	g_Colliders = table;
	c = RA_COLLIDER(g_ColliderCount);
	RA_I32(c, 0) = kind;
	RA_F32(c, 8) = radius;
	RA_PTRF(c, 0x1c) = pos;
	RA_PTRF(c, 0x20) = impulse;
	RA_PTRF(c, 0x30) = hitTest;
	RA_F32(c, 0x24) = bodyRadius;
	RA_PTRF(c, 0x34) = context;
	RA_I32(c, 4) = 1;
	FUN_001a6f20(g_ColliderCount);
	index = g_ColliderCount;
	g_ColliderCount++;
	return index;
}
