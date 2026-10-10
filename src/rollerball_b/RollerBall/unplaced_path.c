/*
RollerBall/unplaced_path.c  (module rollerball_b)

Gap functions 0x001980f0-0x00198590 (original file not known; between
CRollerBall_Cutter.cpp and the game mode): the path record (0x30 bytes, vtable
0x00274b14) that the race variations make from ASE shapes (rollerball_a): a closed
polygon on the x/z plane with its bounding box, tested for "inside" and collided with a
circle; and a quaternion normalise used by several games.

Path record: +0x04 min (x, y, z), +0x14 max (x, y, z), +0x24 points (float[4] each; not
owned), +0x28 point count.

0x00198110 (the destructor) is missing from the function table; rollerball_a calls it.
*/

#include "rollerball_b.h"

#define RBP_MIN 0x04
#define RBP_MAX 0x14
#define RBP_POINTS 0x24
#define RBP_COUNT 0x28

/* engine_misc: a circle sweep against a segment (struct BHit2D: normal[2], point[2],
flags +0x10, t +0x14) */
#define RBB_BCOLL_NORMAL 0x04

/* 0x00274b14  path vtable */
const void *const rbb_vtable_00274b14[1] = {
	(const void *)FUN_00198370, /* +0x00 scalar deleting destructor */
};

/* 0x001980f0  (file unknown)
Constructor. */
void *CRollerBallPath_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274b14;
	RBB_P(self, RBP_POINTS) = NULL;
	RBB_I(self, RBP_COUNT) = 0;
	return self;
}

/* 0x00198110  (file unknown; missing from the function table)
Destructor (the points are not owned). */
void FUN_00198110(void *self)
{
	*(const void *const **)self = rbb_vtable_00274b14;
	if (RBB_P(self, RBP_POINTS) != NULL)
		RBB_P(self, RBP_POINTS) = NULL;
}

/* 0x00198130  (file unknown)
Use `count` points (float[4] each) and compute their bounding box. */
void CRollerBallPath_SetPoints(void *self, float *points, int count)
{
	int i;

	RBB_U(self, RBP_MAX + 0) = 0xc7c34f80u;   /* -99999 */
	RBB_U(self, RBP_MAX + 4) = 0xc7c34f80u;
	RBB_U(self, RBP_MAX + 8) = 0xc7c34f80u;
	RBB_U(self, RBP_MIN + 0) = 0x47c34f80u;   /* 99999 */
	RBB_U(self, RBP_MIN + 4) = 0x47c34f80u;
	RBB_U(self, RBP_MIN + 8) = 0x47c34f80u;
	RBB_P(self, RBP_POINTS) = points;
	RBB_I(self, RBP_COUNT) = count;
	for (i = 0; i < RBB_I(self, RBP_COUNT); i++, points += 4) {
		if (RBB_F(self, RBP_MAX + 0) < points[0])
			RBB_F(self, RBP_MAX + 0) = points[0];
		if (RBB_F(self, RBP_MAX + 4) < points[1])
			RBB_F(self, RBP_MAX + 4) = points[1];
		if (RBB_F(self, RBP_MAX + 8) < points[2])
			RBB_F(self, RBP_MAX + 8) = points[2];
		if (points[0] < RBB_F(self, RBP_MIN + 0))
			RBB_F(self, RBP_MIN + 0) = points[0];
		if (points[1] < RBB_F(self, RBP_MIN + 4))
			RBB_F(self, RBP_MIN + 4) = points[1];
		if (points[2] < RBB_F(self, RBP_MIN + 8))
			RBB_F(self, RBP_MIN + 8) = points[2];
	}
}

/* 0x001981e0  (file unknown)
Whether `pos` is inside the polygon (x/z; crossing count parity). */
unsigned int CRollerBallPath_Contains(void *self, float *pos)
{
	unsigned int crossings = 0;
	int count;
	int i;
	float *p;

	if (!(RBB_F(self, RBP_MIN + 0) <= pos[0] && pos[0] <= RBB_F(self, RBP_MAX + 0) &&
		RBB_F(self, RBP_MIN + 8) <= pos[2] && pos[2] <= RBB_F(self, RBP_MAX + 8)))
		return 0;
	count = RBB_I(self, RBP_COUNT);
	p = RBB_P(self, RBP_POINTS);
	for (i = 0; i < count; p += 4) {
		float *q;
		float ax = p[0];
		float bx;
		float bz;
		i++;
		q = (float *)RBB_P(self, RBP_POINTS) + (i % count) * 4;
		bx = q[0];
		bz = q[2];
		if ((pos[2] <= p[2]) != (pos[2] <= bz)) {
			if ((pos[0] <= ax) == (pos[0] <= bx)) {
				if (pos[0] <= ax)
					crossings++;
			} else if (pos[0] <= bx - ((bz - pos[2]) * (ax - bx)) / (p[2] - bz)) {
				crossings++;
			}
		}
	}
	return crossings & 1;
}

/* 0x00198340  (file unknown)
Which side of the line {x1, y1, x2, y2} the point `a` (x, y) is on (the cross product;
x87 result). */
float CRollerBallPath_SideOfLine(float *a, float *line)
{
	return (float)(((double)a[0] - (double)line[0]) * ((double)a[1] - (double)line[3]) -
		((double)a[1] - (double)line[1]) * ((double)a[0] - (double)line[2]));
}

/* 0x00198370  (file unknown)
Scalar deleting destructor. */
void *CRollerBallPath_DeletingDtor(void *self, unsigned int flags)
{
	FUN_00198110(self);
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x001983a0  (file unknown)
Collide a circle at `pos` (x, z) of `radius` with the polygon's edges; returns the
number of edges hit and, in `out`, the mean of the first three words of the hit records
(normal x, normal y, point x). The original asks for normals (flags 4) only in the first
hit record; the flags of the other records are stack garbage. Here every record is
zeroed and asks for normals. */
int CRollerBallPath_Collide(void *self, float *pos, float radius, float *out)
{
	float circle[3];
	float seg[4];
	float hits[10][8];   /* 0x20-byte records (the BHit2D fields, padded) */
	int count;
	int n = 0;
	int i;

	memset(hits, 0, sizeof hits);
	for (i = 0; i < 10; i++)
		((unsigned int *)hits[i])[4] = RBB_BCOLL_NORMAL;
	circle[0] = pos[0];
	circle[1] = pos[2];
	circle[2] = radius;
	count = RBB_I(self, RBP_COUNT);
	for (i = 0; i < count;) {
		float *pts = RBB_P(self, RBP_POINTS);
		seg[0] = pts[i * 4 + 0];
		seg[1] = pts[i * 4 + 2];
		i++;
		seg[2] = pts[(i % count) * 4 + 0];
		seg[3] = pts[(i % count) * 4 + 2];
		if (bmCircleSweepCircle(seg, circle, (struct BHit2D *)hits[n]) != 0)
			n++;
		count = RBB_I(self, RBP_COUNT);
	}
	if (n != 0 && out != NULL) {
		float inv;
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
		for (i = 0; i < n; i++) {
			out[0] = out[0] + hits[i][0];
			out[1] = out[1] + hits[i][1];
			out[2] = out[2] + hits[i][2];
		}
		inv = 1.0f / (float)n;
		out[0] = inv * out[0];
		out[1] = inv * out[1];
		out[2] = inv * out[2];
	}
	return n;
}

/* 0x00198590  (file unknown; also called by demolition_b and demolition_c)
Normalise a quaternion (unchanged when its length is 0). */
void FUN_00198590(float *q)
{
	float len2 = q[3] * q[3] + q[2] * q[2] + q[1] * q[1] + q[0] * q[0];
	float k = 1.0f;

	if (0.0f < len2)
		k = 1.0f / sqrtf(len2);
	q[0] = k * q[0];
	q[1] = k * q[1];
	q[2] = k * q[2];
	q[3] = k * q[3];
}
