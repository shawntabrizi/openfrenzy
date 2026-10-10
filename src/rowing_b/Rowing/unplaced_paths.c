/*
Rowing/unplaced_paths.c  <-  C:\BlitzParty\Game\Src\Rowing\ (file not known)

Geometry on the level's 2D paths (CRowing_World.cpp's RowPath: x and the second coordinate
of each point): the crossing of two segments, the progress of a boat between two
checkpoints, the side of a gate and the gate passing state machine, and the distance from
a point to a segment.

0x001b93a0-0x001b9c30 lie between CRowing_objects.cpp and CRowing_ASEImport.cpp (gap
functions; their file is not known). The angles use the engine's 24-bit binary angles
(an angle / 2 pi x 2^24, truncated and masked).
*/

#include "rowing_b.h"

/* 0x0030f12c  the level: +0x144 checkpoint count, +0x148 checkpoints (struct RowPath) */
#define RB_CHECKPOINTS() ((struct RowPath *)RB_PTR(g_RowWorld, 0x148))
#define RB_NUM_CHECKPOINTS() RB_I32(g_RowWorld, 0x144)

/* an angle in [0, 2 pi) through the engine's 24-bit binary angle (0x00275860, 0x0027585c,
   0x00275858) */
static float rb_binary_angle(float angle)
{
	unsigned int bits = (unsigned int)(long long)(angle * 0.159154937f * 16777216.0f) & 0xffffff;

	return (float)(int)bits * 5.96046448e-08f * 6.28318548f;
}

/* an angle wrapped into [-pi, pi) */
static float rb_wrap_pi(float a)
{
	while (3.1415925f <= a)                                         /* 0x00256750 */
		a = a - 6.28318501f;                                        /* 0x00262300 */
	while (a < -3.1415925f)                                         /* 0x00253f3c */
		a = a + 6.28318501f;
	return a;
}

/* 0x001b93a0  Rowing (gap; file not known)
Where the lines through segments a and b (x0, z0, x1, z1) cross: out (x, z). Returns 0 when
they are parallel or the crossing is more than 10000 segment lengths away. */
int RowSegment_Intersect(float *out, float *a, float *b)
{
	float ax = a[2] - a[0];
	float az = a[3] - a[1];
	float bx = b[2] - b[0];
	float bz = b[3] - b[1];
	float p = bx * az;
	float q = ax * bz;
	float num;
	float den;
	float t;

	if (p - q == 0.0f)
		return 0;
	num = (b[0] - a[0]) * bz - (b[1] - a[1]) * bx;
	den = q - p;
	if (den == 0.0f)
		return 0;
	den = 1.0f / den;
	t = den * num;
	if (!(t <= 10000.0f))                                           /* 0x002657a0 */
		return 0;
	if (t < -10000.0f)                                              /* 0x00262ef8 */
		return 0;
	out[0] = den * num * ax + a[0];
	out[1] = den * num * az + a[1];
	return 1;
}

/* 0x001b9490  Rowing (gap; file not known)
How far a boat at (x, z) is between checkpoint `index` and the next one: *frac 0 at the
first, 1 at the next, and (when `heading` is given) the race direction there. When the
two checkpoint lines meet, the progress is the angle around their crossing point (the boat
must lie in the ring between the nearest and farthest checkpoint end); else it is the
distance along the first checkpoint's normal. Returns 0 when the boat is outside (with
*frac 100 when it is outside the ring). */
int RowCheckpoint_Progress(int index, float x, float z, float *frac, float *heading)
{
	struct RowPath *checkpoints = RB_CHECKPOINTS();
	int next = (index + 1) % RB_NUM_CHECKPOINTS();
	float segA[4];
	float segB[4];
	float cross[2];
	float d1, d2, d3, d4, dq;
	float lo, hi;
	float qx, qz;
	float angA, rel, span;
	float *a;
	float *b;
	float ux, uz, vx, vz, len, inv, nx, nz, s, t;

	RowPath_GetSegment(&checkpoints[index], segA);
	RowPath_GetSegment(&RB_CHECKPOINTS()[next], segB);
	if (RowSegment_Intersect(cross, segA, segB) != 0) {
		d1 = (segA[0] - cross[0]) * (segA[0] - cross[0]) + (segA[1] - cross[1]) * (segA[1] - cross[1]);
		d2 = (segA[2] - cross[0]) * (segA[2] - cross[0]) + (segA[3] - cross[1]) * (segA[3] - cross[1]);
		d3 = (segB[0] - cross[0]) * (segB[0] - cross[0]) + (segB[1] - cross[1]) * (segB[1] - cross[1]);
		d4 = (segB[2] - cross[0]) * (segB[2] - cross[0]) + (segB[3] - cross[1]) * (segB[3] - cross[1]);
		qx = x - cross[0];
		qz = z - cross[1];
		dq = qz * qz + qx * qx;
		lo = d1;
		if (d2 < lo)
			lo = d2;
		if (d3 < lo)
			lo = d3;
		if (d4 < lo)
			lo = d4;
		hi = d1;
		if (hi < d2)
			hi = d2;
		if (hi < d3)
			hi = d3;
		if (hi < d4)
			hi = d4;
		if (dq < lo || hi < dq) {
			*frac = 100.0f;
			return 0;
		}
		angA = rb_binary_angle((float)atan2((double)(segA[2] - segA[0]), (double)(segA[3] - segA[1])));
		rel = rb_binary_angle((float)atan2((double)qx, (double)qz) + 3.1415925f) - angA;
		span = rb_binary_angle((float)atan2((double)(segB[2] - segB[0]), (double)(segB[3] - segB[1])))
			- angA;
		rel = rb_wrap_pi(rel);
		span = rb_wrap_pi(span);
		*frac = rel / span;
		if (!(*frac <= 1.0f) || *frac < 0.0f)
			return 0;
		if (heading != NULL)
			*heading = (*frac * span + angA) - 1.57079625f;          /* 0x00253898 */
		return 1;
	}
	a = checkpoints[index].points;
	b = RB_CHECKPOINTS()[next].points;
	ux = a[2] - a[0];
	uz = a[3] - a[1];
	vx = b[0] - a[0];
	vz = b[1] - a[1];
	qx = x - a[0];
	qz = z - a[1];
	len = sqrtf(vz * vz + vx * vx);
	inv = 1.0f / len;
	*frac = (inv * vz * qz + vx * inv * qx) / len;
	if (*frac < 0.0f || !(*frac <= 1.0f))
		return 0;
	len = sqrtf(uz * uz + ux * ux);
	inv = 1.0f / len;
	nx = ux * inv;
	nz = inv * uz;
	s = nz * qz + nx * qx;
	t = nz * vz + nx * vx;
	vx = b[2] - a[0];
	vz = b[3] - a[1];
	if (s < t * *frac)
		return 0;
	if (((nz * vz + nx * vx) - len) * *frac + len < s)
		return 0;
	if (heading != NULL)
		*heading = (float)atan2((double)-nz, (double)nx);
	return 1;
}

/* 0x001b99e0  Rowing (gap; file not known)
The side of the gate segment that (x, z) is on: *side its signed distance from the gate
line. Returns 1 when the point is beside the gate (within 200 of it); 0 otherwise (*side
1000 when it is not level with the gate). */
int RowGate_Side(float *side, float x, float z, struct RowPath *gate)
{
	float *p = gate->points;
	float dx = p[2] - p[0];
	float dz = p[3] - p[1];
	float len = sqrtf(dz * dz + dx * dx);
	float inv = 1.0f / len;
	float nx = inv * dx;
	float nz = inv * dz;
	float qx = x - p[0];
	float qz = z - p[1];
	float along = qx * nx + qz * nz;
	float d;

	if (along < 0.0f || !(along <= len)) {
		*side = 1000.0f;
		return 0;
	}
	d = qz * nx + qx * -nz;
	*side = d;
	if (d < -200.0f)                                                /* 0x002536ec */
		return 0;
	if (!(d <= 200.0f))                                             /* 0x002536f0 */
		return 0;
	return 1;
}

/* 0x001b9ac0  Rowing (gap; file not known)
The gate passing of a boat at (x, z). *state 0: looking for a gate not passed yet that the
boat is beside (*state becomes its number, positive on the front side, negative behind).
Else the boat must stay beside that gate (else *state 0): crossing to 20 behind from the
front passes it (done 1, returns 1), crossing to 20 in front from behind is the wrong way
(done 2, returns 2); either way the gates before it that are not done become 2 (missed).
Returns 0 otherwise. */
int RowGates_Check(float x, float z, int *state, char *done, int count, struct RowPath *gates)
{
	float side;
	int s = *state;
	int result = 0;
	int n;
	int i;

	if (s != 0) {
		n = s < 0 ? -s : s;
		i = n - 1;
		if (RowGate_Side(&side, x, z, &gates[i]) == 0) {
			*state = 0;
			return result;
		}
		if (s > 0) {
			if (!(20.0f < side))                                    /* 0x00255530 */
				return result;
			i--;
			*state = 0;
			done[i + 1] = 1;
			result = 1;
		} else {
			if (!(side < -20.0f))                                   /* 0x002536c0 */
				return result;
			i--;
			*state = 0;
			done[i + 1] = 2;
			result = 2;
		}
		for (; i >= 0; i--)
			if (done[i] == 0)
				done[i] = 2;
		return result;
	}
	for (i = 0; i < count; i++) {
		if (done[i] != 0)
			continue;
		if (RowGate_Side(&side, x, z, &gates[i]) == 0)
			continue;
		if (side < 0.0f)
			*state = i + 1;
		else
			*state = -1 - i;
		return result;
	}
	return result;
}

/* 0x001b9c30  Rowing (gap; file not known)
The distance from p (x, z) to the segment (x0, z0, x1, z1): *closest the nearest point on
it, *foot the foot of the perpendicular on its line, *perp the distance to the line, *along
the distance along it. */
float RowSegment_Distance(float *seg, float *p, float *closest, float *foot, float *perp, float *along)
{
	float dx = seg[2] - seg[0];
	float dz = seg[3] - seg[1];
	float len = sqrtf(dz * dz + dx * dx);
	float inv, ux, uz, qx, qz, d, h;

	if (len == 0.0f) {
		closest[0] = seg[0];
		closest[1] = seg[1];
		foot[0] = seg[0];
		foot[1] = seg[1];
		qx = p[0] - seg[0];
		qz = p[1] - seg[1];
		d = sqrtf(qz * qz + qx * qx);
		*perp = d;
		*along = d;
		return *perp;
	}
	inv = 1.0f / len;
	ux = inv * dx;
	uz = inv * dz;
	qx = p[0] - seg[0];
	qz = p[1] - seg[1];
	*along = qz * uz + qx * ux;
	foot[0] = ux * *along;
	foot[1] = uz * *along;
	foot[0] = foot[0] + seg[0];
	foot[1] = foot[1] + seg[1];
	d = sqrtf(qz * qz + qx * qx);
	h = d * d - *along * *along;
	if (0.0f < h)
		*perp = sqrtf(h);
	else
		*perp = 0.0f;
	if (*along <= 0.0f) {
		closest[0] = seg[0];
		closest[1] = seg[1];
		return d;
	}
	if (!(*along < len)) {
		closest[0] = seg[2];
		closest[1] = seg[3];
		qx = p[0] - seg[2];
		qz = p[1] - seg[3];
		return sqrtf(qz * qz + qx * qx);
	}
	closest[0] = foot[0];
	closest[1] = foot[1];
	return *perp;
}
