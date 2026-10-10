/*
ClassLib/unplaced.c  (classlib_ui)

ClassLib functions whose original source file is not known yet: the vector and
angle helpers at the start of the ClassLib block (0x000703a0-0x00071030), the
intrusive list helpers (0x00071050-0x00071110) and other gap functions.

Vectors are float[3] (x, y, z) or float[4] (x, y, z, w); "XZ" helpers work on
the ground plane. Angles are radians.
*/

#include "classlib_ui.h"

/* ---------- globals (classlib_ui_data.h) */

/* 0x0028fda8  frame time scale (1.0 at the nominal frame rate) */
#define clu_time_scale CLU_VAR(float, 0x0028fda8)
/* 0x0028fdac  axis order of the Euler rotation built by CLQuat_FromEuler: y, z, x */
#define clu_euler_order CLU_ARR(int, 0x0028fdac)

/* ---------- angles */

/* 0x000703a0  ClassLib (file unknown)
Wraps an angle once into [0, 2pi) (one turn at most). */
float CLMath_WrapAngle(float angle)
{
	double a = (double)angle;

	if ((double)6.2831855f < a)
		a = a - (double)6.2831855f;
	if (a < 0.0)
		a = a + (double)6.2831855f;
	return (float)a;
}

/* 0x000703d0  ClassLib (file unknown)
Signed difference a - b, brought into (-pi, pi] with one turn of correction. */
float CLMath_AngleDelta(float a, float b)
{
	double d = (double)a - (double)b;

	if (d <= (double)-3.1415925f || d > (double)3.1415925f) {
		if ((double)3.1415925f < d)
			return (float)((double)(float)d - (double)6.283185f);
		d = ((double)a + (double)6.283185f) - (double)b;
	}
	return (float)d;
}

/* ---------- XZ plane geometry */

/* 0x00070420  ClassLib (file unknown)
Returns 1 when segment a-b crosses segment c-d in the XZ plane. */
int CLMath_SegmentsCrossXZ(float *a, float *b, float *c, float *d)
{
	float t;
	float den;

	t = (a[0] - c[0]) * (c[2] - d[2]) - (a[2] - c[2]) * (c[0] - d[0]);
	den = (c[0] - d[0]) * (b[2] - a[2]) - (c[2] - d[2]) * (b[0] - a[0]);
	if (den <= 0.0f) {
		if (t > 0.0f)
			return 0;
		if (t < den)
			return 0;
	} else {
		if (t < 0.0f)
			return 0;
		if (den < t)
			return 0;
	}
	t = (a[2] - c[2]) * (b[0] - a[0]) - (a[0] - c[0]) * (b[2] - a[2]);
	if (den <= 0.0f) {
		if (t > 0.0f)
			return 0;
		if (t < den)
			return 0;
	} else {
		if (t < 0.0f)
			return 0;
		if (den < t)
			return 0;
	}
	if (den == 0.0f)
		return 0;
	return 1;
}

/* 0x00070550  ClassLib (file unknown)
dst += v * scale */
void CLVec3_AddScaled(float *dst, float *v, float scale)
{
	dst[0] = scale * v[0] + dst[0];
	dst[1] = scale * v[1] + dst[1];
	dst[2] = scale * v[2] + dst[2];
}

/* 0x00070580  ClassLib (file unknown)
dst.xz = a.xz - b.xz (dst.y is not written) */
void CLVec3_SubXZ(float *dst, float *a, float *b)
{
	dst[0] = a[0] - b[0];
	dst[2] = a[2] - b[2];
}

/* 0x000705a0  ClassLib (file unknown)
box: centre at [0..2], half sizes at [4..6]. 1 when the point is inside
(the lower faces are inclusive, as in the original compares). */
int CLBox_ContainsPoint(float *box, float *p)
{
	if (box[0] - box[4] <= p[0] && p[0] <= box[0] + box[4] &&
	    box[1] - box[5] <= p[1] && p[1] <= box[5] + box[1] &&
	    box[2] - box[6] <= p[2] && p[2] <= box[6] + box[2])
		return 1;
	return 0;
}

/* 0x00070610  ClassLib (file unknown)
Heading (rotation about Y) of an orientation: transforms (0, 0, 1) by the
matrix built from `orientation` and returns atan2(x, -z). */
float CLMath_HeadingOf(float *orientation)
{
	float v[4];
	float m[16];

	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 1.0f;
	v[3] = 1.0f;
	FUN_000515c0(m, orientation);
	FUN_00050530(m, v);
	return (float)CLU_FPATAN(v[0], -(double)v[2]);
}

/* 0x00070670  ClassLib (file unknown)
Absolute angular distance between a and b, in [0, pi]. */
float CLMath_AngleDistance(float a, float b)
{
	float d = a - b;

	if (fabsf(d) <= 3.1415925f)
		return fabsf(d);
	if (b < a)
		return (float)(((double)b - (double)a) + (double)6.283185f);
	return (float)((double)d + (double)6.283185f);
}

/* 0x000706f0  ClassLib (file unknown)
Heading of a direction in the XZ plane, atan2(x, z) wrapped into [0, 2pi). */
float CLMath_HeadingXZ(float *dir)
{
	double a = CLU_FPATAN(dir[0], dir[2]);

	if ((double)6.2831855f < a)
		a = a - (double)6.2831855f;
	if (a < 0.0)
		a = a + (double)6.2831855f;
	return (float)a;
}

/* 0x00070730  ClassLib (file unknown)
Point in triangle (XZ plane): counts the crossings of a ray from p towards +X
(25000 units long) with the three edges. */
int CLMath_PointInTriangleXZ(float *p, float *a, float *b, float *c)
{
	float far_point[4];
	unsigned char crossings;

	far_point[0] = p[0] + 25000.0f;
	far_point[1] = 0.0f;
	far_point[2] = 0.0f;
	crossings = CLMath_SegmentsCrossXZ(p, far_point, a, b) != 0;
	if (CLMath_SegmentsCrossXZ(p, far_point, b, c) != 0)
		crossings = crossings + 1;
	if (CLMath_SegmentsCrossXZ(p, far_point, c, a) != 0)
		crossings = crossings + 1;
	return crossings & 1;
}

/* ---------- rotations */

/* 0x000707c0  ClassLib (file unknown)
Rotates v about the unit axis axis_angle[0..2] by the angle axis_angle[3]
(Rodrigues). When v is parallel to the axis, out = v (4 floats copied). */
void CLVec3_RotateAboutAxis(float *out, float *v, float *axis_angle)
{
	float ax, ay, az;
	float along, px, py, pz, cx, cy, cz;
	float len, clen, c, s;

	along = v[2] * axis_angle[2] + axis_angle[1] * v[1] + v[0] * axis_angle[0];
	ay = axis_angle[1];
	az = axis_angle[2];
	px = v[0] - along * axis_angle[0];
	py = v[1] - along * ay;
	pz = v[2] - along * az;
	cx = pz * axis_angle[1] - py * axis_angle[2];
	cy = px * axis_angle[2] - pz * axis_angle[0];
	cz = py * axis_angle[0] - px * axis_angle[1];
	len = sqrtf(px * px + py * py + pz * pz);
	if (len == 0.0f) {
		out[0] = v[0];
		out[1] = v[1];
		out[2] = v[2];
		out[3] = v[3];
		return;
	}
	clen = sqrtf(cx * cx + cy * cy + cz * cz);
	if (clen != 0.0f) {
		len = len / clen;
		cx = cx * len;
		cy = cy * len;
		cz = cz * len;
	}
	c = (float)cos((double)axis_angle[3]);
	s = (float)sin((double)axis_angle[3]);
	ax = axis_angle[0];
	out[0] = s * cx + px * c + along * ax;
	out[1] = s * cy + py * c + along * ay;
	out[2] = s * cz + pz * c + along * az;
}

/* 0x00070940  ClassLib (file unknown)
Normalises v in place; a zero vector stays zero. */
void CLVec3_Normalize(float *v)
{
	float len = sqrtf(v[2] * v[2] + v[1] * v[1] + v[0] * v[0]);

	if (len != 0.0f) {
		len = 1.0f / len;
		v[0] = len * v[0];
		v[1] = len * v[1];
		v[2] = len * v[2];
	}
}

/* 0x00070990  ClassLib (file unknown)
A vector perpendicular to v (unit length in the general case). */
void CLVec3_Perpendicular(float *out, float *v)
{
	char zeros;
	int i;

	zeros = v[0] == 0.0f;
	if (v[1] == 0.0f)
		zeros = zeros + 1;
	if (v[2] == 0.0f)
		zeros = zeros + 1;
	if (zeros == 2) {
		if (v[0] == 0.0f) {
			out[0] = 1.0f;
			out[1] = 0.0f;
			out[2] = 0.0f;
			return;
		}
		if (v[1] == 0.0f) {
			out[0] = 0.0f;
			out[1] = 1.0f;
			out[2] = 0.0f;
			return;
		}
	} else if (zeros == 1) {
		for (i = 0; i < 3; i++)
			out[i] = v[i] == 0.0f ? 1.0f : 0.0f;
		return;
	} else if (zeros == 0) {
		out[0] = 1.0f;
		out[1] = 1.0f;
		out[2] = -((v[1] + v[0]) / v[2]);
		CLVec3_Normalize(out);
	}
}

/* 0x00070a90  ClassLib (file unknown)
Quaternion (x, y, z, w) that turns +Y onto the direction -dir (dir need not be
normalised in X/Z; dir[1] is the cosine of the angle). */
void CLQuat_FromDirection(float *q, float *dir)
{
	float nx, nz, axis_z;
	double half, s;
	int down;

	nx = -dir[0];
	nz = -dir[2];
	if (nx != 0.0f) {
		axis_z = sqrtf(1.0f / ((nz * nz) / (nx * nx) + 1.0f));
		q[2] = axis_z;
		if (0.0f < nx)
			q[2] = -axis_z;
		half = acos((double)dir[1]) * (double)0.5f;
		q[1] = 0.0f;
		s = sin(half);
		q[0] = (float)-((((double)nz * (double)q[2]) / (double)nx) * s);
		q[2] = (float)(s * (double)q[2]);
		q[3] = (float)cos(half);
		return;
	}
	if (nz == 0.0f) {
		down = dir[1] <= 0.0f;
		q[1] = 0.0f;
		q[0] = 0.0f;
		if (down) {
			q[3] = 0.0f;
			q[2] = -1.0f;
			return;
		}
		q[2] = 0.0f;
		q[3] = 1.0f;
		return;
	}
	q[2] = 0.0f;
	half = acos((double)dir[1]) * (double)0.5f;
	q[3] = (float)half;
	if (nz <= 0.0f) {
		s = sin(half);
		q[1] = 0.0f;
		q[0] = (float)-s;
		q[3] = (float)cos(half);
		return;
	}
	s = sin(half);
	q[1] = 0.0f;
	q[0] = (float)s;
	q[3] = (float)cos(half);
}

/* 0x00070be0  ClassLib (file unknown)
Rotates v by the rotation `rotation` (converted to axis-angle by FUN_00051d30). */
void CLVec3_Rotate(float *out, float *v, float *rotation)
{
	float axis_angle[4];

	FUN_00051d30(axis_angle, rotation);
	CLVec3_RotateAboutAxis(out, v, axis_angle);
}

/* 0x00070c10  ClassLib (file unknown)
Builds the quaternion q from Euler angles: euler[0] about X, y_reference -
euler[1] about Y, euler[2] about Z, applied in the order of clu_euler_order
(Y first, then the non-zero ones of Z and X). */
void CLQuat_FromEuler(float *q, float *euler, float y_reference)
{
	float axis_angle[4];
	float previous[4];
	float step[4];
	int *order;
	int axis;

	if (clu_euler_order[0] == 0) {
		axis_angle[1] = 0.0f;
		axis_angle[0] = 1.0f;
		axis_angle[2] = 0.0f;
		axis_angle[3] = euler[0];
	} else if (clu_euler_order[0] == 1) {
		axis_angle[0] = 0.0f;
		axis_angle[1] = 1.0f;
		axis_angle[2] = 0.0f;
		axis_angle[3] = y_reference - euler[1];
	} else if (clu_euler_order[0] == 2) {
		axis_angle[0] = 0.0f;
		axis_angle[2] = 1.0f;
		axis_angle[1] = 0.0f;
		axis_angle[3] = euler[2];
	}
	FUN_00051df0(q, axis_angle);
	for (order = &clu_euler_order[1]; order < &clu_euler_order[3]; order++) {
		axis = *order;
		if (euler[axis] == 0.0f)
			continue;
		if (axis == 0) {
			axis_angle[0] = 1.0f;
			axis_angle[2] = 0.0f;
			axis_angle[1] = 0.0f;
			axis_angle[3] = euler[axis];
		} else if (axis == 1) {
			axis_angle[0] = 0.0f;
			axis_angle[1] = 1.0f;
			axis_angle[2] = 0.0f;
			axis_angle[3] = y_reference - euler[1];
		} else if (axis == 2) {
			axis_angle[0] = 0.0f;
			axis_angle[2] = 1.0f;
			axis_angle[1] = 0.0f;
			axis_angle[3] = euler[axis];
		}
		FUN_00051df0(step, axis_angle);
		previous[1] = q[1];
		previous[2] = q[2];
		previous[0] = q[0];
		previous[3] = q[3];
		FUN_00051ca0(q, previous, step);
	}
}

/* 0x00070d70  ClassLib (file unknown)
Quaternion of the axis-angle aa (axis aa[0..2], angle aa[3]), angle negated. */
void CLQuat_FromAxisAngleInverse(float *q, float *aa)
{
	double s = sin((double)aa[3] * (double)-0.5f);
	double c = cos((double)aa[3] * (double)-0.5f);

	q[3] = (float)c;
	q[0] = (float)(s * (double)aa[0]);
	q[1] = (float)(s * (double)aa[1]);
	q[2] = (float)(s * (double)aa[2]);
}

/* 0x00070db0  ClassLib (file unknown)
Squared distance in the XZ plane. */
float CLVec3_DistanceSqXZ(float *a, float *b)
{
	return (float)(((double)a[0] - (double)b[0]) * ((double)a[0] - (double)b[0]) +
		       ((double)a[2] - (double)b[2]) * ((double)a[2] - (double)b[2]));
}

/* 0x00070de0  ClassLib (file unknown)
Heading from `from` to `to` in the XZ plane, wrapped into [0, 2pi). */
float CLMath_HeadingTo(float *from, float *to)
{
	double a = CLU_FPATAN((double)to[0] - (double)from[0], (double)to[2] - (double)from[2]);

	if ((double)6.2831855f < a)
		a = a - (double)6.2831855f;
	if (a < 0.0)
		a = a + (double)6.2831855f;
	return (float)a;
}

/* 0x00070e20  ClassLib (file unknown)
1 when the sphere (centre `centre`, radius `radius`) is inside the cone at
`apex` with unit axis `axis` and half angle `half_angle`. */
int CLMath_SphereInCone(float *centre, float radius, float *apex, float *axis, float half_angle)
{
	float dx, dy, dz, dist, inv;
	double angle, spread;

	dx = centre[0] - apex[0];
	dy = centre[1] - apex[1];
	dz = centre[2] - apex[2];
	dist = sqrtf(dz * dz + dx * dx + dy * dy);
	inv = 1.0f / dist;
	dx = dx * inv;
	dy = inv * dy;
	dz = dz * inv;
	angle = acos((double)(dz * axis[2]) + (double)(dy * axis[1]) + (double)(dx * axis[0]));
	spread = CLU_FPATAN(radius, dist);
	if (angle <= spread + (double)half_angle)
		return 1;
	return 0;
}

/* 0x00070ed0  ClassLib (file unknown)
Moves pos towards target by speed * time scale (divided by `scale` when it is
not 0). Returns 1 (and copies 4 floats of target) when it arrives. */
int CLVec3_MoveTowards(float *pos, float *target, float speed, float scale)
{
	float delta[3];
	float dist;

	delta[0] = target[0] - pos[0];
	delta[1] = target[1] - pos[1];
	delta[2] = target[2] - pos[2];
	dist = sqrtf(delta[1] * delta[1] + delta[0] * delta[0] + delta[2] * delta[2]);
	if (scale != 0.0f)
		dist = dist / scale;
	speed = clu_time_scale * speed;
	if (dist <= speed) {
		pos[0] = target[0];
		pos[1] = target[1];
		pos[2] = target[2];
		pos[3] = target[3];
		return 1;
	}
	FUN_00050ff0(pos, pos, 1.0f, delta, speed / dist);
	return 0;
}

/* 0x00070f80  ClassLib (file unknown)
Removes the component of v along n twice (reflection off a plane with normal
n), always pushing v to the side n points to. */
void CLVec3_Reflect(float *v, float *n)
{
	float d, ny, nz;

	d = n[2] * v[2] + v[0] * n[0] + v[1] * n[1];
	if (0.0f < d)
		d = -d;
	d = d * -2.0f;
	ny = n[1];
	nz = n[2];
	v[0] = d * n[0] + v[0];
	v[1] = d * ny + v[1];
	v[2] = d * nz + v[2];
}

/* 0x00070ff0  ClassLib (file unknown)
cylinder: base centre [0..2], radius [4], height [5]. 1 when p is inside. */
int CLCylinder_ContainsPoint(float *cyl, float *p)
{
	if (cyl[1] <= p[1] && p[1] <= cyl[5] + cyl[1] &&
	    (p[0] - cyl[0]) * (p[0] - cyl[0]) + (p[2] - cyl[2]) * (p[2] - cyl[2]) <= cyl[4] * cyl[4])
		return 1;
	return 0;
}

/* ---------- intrusive doubly linked list

struct CLU_List { node *next; node *prev; int count; } -- the list head is
itself a node (next/prev at the same offsets as in every node). */

/* 0x00071050  ClassLib (file unknown)
List constructor: an empty list points at itself. */
void CLList_Init(int *list)
{
	list[1] = (int)list;
	list[0] = (int)list;
	list[2] = 0;
}

/* 0x00071060  ClassLib (file unknown)
Same as CLList_Init (a second copy of the inline constructor). */
void CLList_Init2(int *list)
{
	list[1] = (int)list;
	list[0] = (int)list;
	list[2] = 0;
}

/* 0x00071070  ClassLib (file unknown)
Inserts node: at the tail (before the head) when at_tail != 0, else at the
head. */
void CLList_Insert(int *list, int *node, int at_tail)
{
	int *last;
	int first;

	if (at_tail != 0) {
		last = (int *)list[1];
		node[1] = (int)last;
		node[0] = *last;
		*(int **)(*last + 4) = node;
		*last = (int)node;
		list[2] = list[2] + 1;
		return;
	}
	first = list[0];
	node[0] = first;
	node[1] = *(int *)(first + 4);
	**(int ***)(first + 4) = node;
	*(int **)(first + 4) = node;
	list[2] = list[2] + 1;
}

/* 0x000710b0  ClassLib (file unknown)
Unlinks node (if linked) and returns its previous node. */
int CLList_Remove(int *list, int *node)
{
	if (node[0] != 0) {
		*(int *)node[1] = node[0];
		*(int *)(node[0] + 4) = node[1];
		list[2] = list[2] + -1;
		node[0] = 0;
	}
	return node[1];
}

/* 0x000710e0  ClassLib (file unknown)
Unlinks every node. */
void CLList_Clear(int *list)
{
	int *node;
	int *next;

	next = (int *)list[0];
	while (node = next, node != list) {
		next = (int *)node[0];
		if (next != NULL) {
			*(int **)node[1] = next;
			*(int *)(node[0] + 4) = node[1];
			list[2] = list[2] + -1;
			node[0] = 0;
		}
	}
}

/* 0x00071110  ClassLib (file unknown)
Next node after `node` (the first one when node is NULL), or 0 at the end. */
unsigned int CLList_Next(unsigned int *list, unsigned int *node)
{
	if (node == NULL)
		node = list;
	return *node & (((unsigned int *)*node == list) - 1);
}

/* ---------- screen fades, loading screens and music (0x00071c20-0x00072720) */

/* 0x002ec900  the custom soundtrack stream of the music that plays now (0: disc music) */
#define clu_music_custom CLU_VAR(int, 0x002ec900)
/* 0x002ec904  the screen fade (CLFade_Draw) runs */
#define clu_fade_active CLU_VAR(int, 0x002ec904)
/* 0x002ec908  frames left in the screen fade */
#define clu_fade_time CLU_VAR(float, 0x002ec908)
/* 0x002ec910  quad of CLScreen_DrawDarken */
#define clu_darken_quad CLU_ARR(struct CLQuadVertex, 0x002ec910)
/* 0x002ec9a0  quad of CLFade_Draw */
#define clu_fade_quad CLU_ARR(struct CLQuadVertex, 0x002ec9a0)
/* 0x002eca3c  set by FUN_00072720 */
#define clu_eca3c CLU_VAR(undefined4, 0x002eca3c)
/* 0x002900a0  3x3 blur weights of CLSprite_DrawBlurred */
#define clu_blur_weights CLU_ARR(float, 0x002900a0)
/* 0x0028ffb8  music track that plays now (-1: none) */
#define clu_music_track CLU_VAR(int, 0x0028ffb8)
/* 0x0028ffc4  music track flags (8-byte table entries) */
#define clu_music_flags(i) CLU_VAR(unsigned int, 0x0028ffc4 + (i) * 8)
/* 0x002ec890  music stream samples */
#define clu_music_samples CLU_ARR(int, 0x002ec890)

/* globals of other modules (tentative definitions in classlib_ui_extern.c) */
extern int DAT_002ba928;                 /* screen width (core) */
extern int DAT_002ba92c;                 /* screen height (core) */
extern int DAT_002ba9ac;                 /* loading screen: bytes to load (core) */
extern int DAT_002ba9b0;                 /* loading screen: files to load (core) */
extern int DAT_002bae6c;                 /* music volume (core) */
extern int DAT_002bae80;                 /* custom soundtrack count (core) */
extern int DAT_002bae84;                 /* custom soundtrack in use (core) */
extern int DAT_003055e0;                 /* game state: 2 = no music changes */
extern unsigned int DAT_002bc490[256];   /* colour tables by component value (bdisplay) */
extern unsigned int DAT_002bc898[256];
extern unsigned int DAT_002bcc94[256];
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bd640[256];
extern float DAT_002855f0[16];           /* identity matrix (engine) */

/* 0x00072060  ClassLib (file unknown); missing from of_functions.tsv
Draw callback of CLSprite_DrawBlurred: counts the draws in sprite->0x88 and
moves the quad off screen once the count passes sprite->0x84. */
int FUN_00072060(int pass, int *info)
{
	int sprite = info[7];
	float *quad;

	if (pass == 0) {
		*(int *)(sprite + 0x88) = *(int *)(sprite + 0x88) + 1;
		if (*(int *)(sprite + 0x84) < *(int *)(sprite + 0x88)) {
			quad = (float *)info[1];
			quad[0] = -1000.0f;
			quad[1] = -1000.0f;
			quad[2] = 0.0f;
			quad[9] = -1000.0f;
			quad[10] = -1000.0f;
			quad[11] = 0.0f;
			quad[18] = -1000.0f;
			quad[19] = -1000.0f;
			quad[20] = 0.0f;
			quad[27] = -1000.0f;
			quad[28] = -1000.0f;
			quad[29] = 0.0f;
		}
	}
	return 1;
}

/* 0x00071c20  ClassLib (file unknown)
Darkens the whole screen with a black quad of alpha `alpha` (0-255); with
`music` set, the music volume goes down by the same amount. */
void CLScreen_DrawDarken(int alpha, int music)
{
	unsigned int colour;

	if (alpha < 0x100) {
		if (alpha < 0)
			alpha = 0;
	} else {
		alpha = 0xff;
	}
	if (music != 0)
		FUN_00054690(((0xff - alpha) * DAT_002bae6c) / 0xff);
	if (alpha == 0)
		return;
	FUN_000438b0();
	FUN_00043b70(0);
	FUN_00044b00(7, 1, 0);
	FUN_00044b00(5, 0, 0);
	clu_darken_quad[2].x = (float)(DAT_002ba928 >> 1);
	clu_darken_quad[1].y = (float)(DAT_002ba92c >> 1);
	clu_darken_quad[0].x = -clu_darken_quad[2].x;
	colour = DAT_002bc898[alpha] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
	clu_darken_quad[0].colour = colour;
	clu_darken_quad[0].y = -clu_darken_quad[1].y;
	clu_darken_quad[0].z = 0.0f;
	clu_darken_quad[1].z = 0.0f;
	clu_darken_quad[2].z = 0.0f;
	clu_darken_quad[3].z = 0.0f;
	clu_darken_quad[1].x = clu_darken_quad[0].x;
	clu_darken_quad[1].colour = colour;
	clu_darken_quad[2].y = clu_darken_quad[0].y;
	clu_darken_quad[2].colour = colour;
	clu_darken_quad[3].x = clu_darken_quad[2].x;
	clu_darken_quad[3].y = clu_darken_quad[1].y;
	clu_darken_quad[3].colour = colour;
	FUN_000416c0(0, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 0, 0);
	FUN_0003f2d0(4, (int)clu_darken_quad, 4);
	FUN_00044b00(5, 1, 0);
	FUN_00044b00(6, 0, 0);
	FUN_00044b00(7, 3, 0);
	FUN_00043b70(1);
}

/* 0x00071db0  ClassLib (file unknown)
Screen fade from black over 60 frames. The first call starts it (and draws,
unless step is -1). Returns 1 when the fade has ended. */
int CLFade_Draw(float step)
{
	int saved[30];
	int alpha;
	int a;
	unsigned int colour;

	if (clu_fade_active == 0) {
		clu_fade_active = 1;
		clu_fade_time = 60.0f;
		if (step == -1.0f)
			return 0;
	} else {
		clu_fade_time = clu_fade_time - step;
		if (clu_fade_time <= 0.0f) {
			clu_fade_active = 0;
			clu_fade_time = 0.0f;
			return 1;
		}
	}
	FUN_000438b0();
	FUN_00046d40((undefined4 *)saved);
	alpha = (int)((double)clu_fade_time * (double)265.0f * (double)0.016666668f);
	a = 0xff;
	if (alpha < 0x100)
		a = alpha;
	FUN_00043b70(0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(8, 0, 0);
	FUN_00044b00(7, 1, 0);
	FUN_00044b00(10, 1, 0);
	FUN_00044b00(4, 0, 0);
	FUN_000416c0(0, 0);
	colour = DAT_002bd640[a] | DAT_002bc490[a] | DAT_002bce18[a] | DAT_002bcc94[0];
	clu_fade_quad[0].x = -320.0f;
	clu_fade_quad[0].y = -240.0f;
	clu_fade_quad[0].z = 0.0f;
	clu_fade_quad[1].x = 320.0f;
	clu_fade_quad[1].y = -240.0f;
	clu_fade_quad[1].z = 0.0f;
	clu_fade_quad[2].x = -320.0f;
	clu_fade_quad[2].y = 240.0f;
	clu_fade_quad[2].z = 0.0f;
	clu_fade_quad[3].x = 320.0f;
	clu_fade_quad[3].y = 240.0f;
	clu_fade_quad[3].z = 0.0f;
	clu_fade_quad[0].colour = colour;
	clu_fade_quad[1].colour = colour;
	clu_fade_quad[2].colour = colour;
	clu_fade_quad[3].colour = colour;
	FUN_0003f2d0(4, (int)clu_fade_quad, 4);
	FUN_00043b70(1);
	FUN_00046d60(saved);
	return 0;
}

/* 0x00071f90  ClassLib (file unknown)
Adds the size of the loading-screen resource of a package ("csLoading" for
cut scenes, else "mgLoading") to the loading totals. */
void CLLoading_AddPackage(undefined4 package, int cutscene)
{
	const char *name;
	int size;

	if (cutscene == 0)
		name = "mgLoading";
	else
		name = "csLoading";
	if (FUN_0004d320(0, package, (undefined4)name, &size, 0) != 0) {
		DAT_002ba9ac = DAT_002ba9ac + size;
		DAT_002ba9b0 = DAT_002ba9b0 + 1;
	}
}

/* 0x00071ff0  ClassLib (file unknown)
Fades the music out over `frames` frames (stops a custom soundtrack). */
void CLMusic_FadeOut(int frames)
{
	int root;

	root = FUN_0002b5b0((int)DAT_002ba750, (undefined4)"Root");
	if (0 < frames)
		frames = -frames;
	if (clu_music_custom != 0) {
		FUN_00055df0(clu_music_custom);
		clu_music_track = -1;
	}
	*(int *)(root + 100) = 0;
	*(int *)(root + 0x6c) = frames;
	if (*(int *)(root + 0x54) != 0 && *(int *)(root + 0x5c) + frames < 1) {
		FUN_000563f0(*(int *)(root + 0x54));
		*(int *)(root + 0x54) = 0;
		clu_music_track = -1;
	}
}

/* 0x000720e0  ClassLib (file unknown)
Draws a sprite nine times around `pos` (3x3, spacing scale * spread), with
the alpha `alpha` weighted by the blur kernel at 0x002900a0. With `sprite`
set, FUN_00072060 counts the draws. */
void CLSprite_DrawBlurred(undefined4 a1, undefined4 a2, undefined4 a3, float *pos, undefined4 a5, undefined4 a6,
			  undefined4 a7, int alpha, float spread, float scale, int sprite)
{
	float spacing;
	float falpha;
	float dx;
	float matrix[16];
	float local_pos[8];
	const float *weight;
	int i, j;

	spacing = spread * scale;
	falpha = (float)alpha;
	memcpy(matrix, DAT_002855f0, sizeof(matrix));
	memcpy(local_pos, pos, sizeof(local_pos));
	local_pos[1] = 0.0f;
	local_pos[0] = 0.0f;
	matrix[10] = scale;
	matrix[5] = scale;
	matrix[0] = scale;
	weight = clu_blur_weights;
	for (i = 0; i < 3; i++) {
		dx = (float)(((double)i - 1.0) * (double)spacing);
		for (j = 0; j < 3; j++) {
			matrix[12] = pos[0] - dx;
			matrix[13] = (float)((double)pos[1] - ((double)j - 1.0) * (double)spacing);
			FUN_00043930((undefined4 *)matrix);
			if (sprite != 0) {
				*(int *)(sprite + 0x88) = 0;
				FUN_00043400(a1, a2, a3, (undefined4)local_pos, a5, a6, a7,
					     (undefined4)(int)((double)falpha * (double)*weight), (undefined4)FUN_00072060,
					     (undefined4)sprite);
			} else {
				FUN_00043400(a1, a2, a3, (undefined4)local_pos, a5, a6, a7,
					     (undefined4)(int)((double)falpha * (double)*weight), 0, 0);
			}
			weight++;
		}
	}
	FUN_000438b0();
}

/* 0x000722a0  ClassLib (file unknown)
Restarts the custom soundtrack on a random track. */
void CLMusic_NextCustomTrack(void)
{
	if (DAT_002bae84 != 0) {
		FUN_00055e30(DAT_002bae84);
		DAT_002bae84 = (int)FUN_00055c40(DAT_002bae80 + -1);
	}
}

/* 0x000722d0  ClassLib (file unknown)
Plays music track `track` (or a custom soundtrack track when one is in use
and `disc_only` is 0); the music that played before fades out when
`crossfade` is set. */
void CLMusic_Play(int track, undefined4 start, undefined4 volume, undefined4 fade, int crossfade, int disc_only,
		  unsigned int flags)
{
	int root;
	int limit;

	root = FUN_0002b5b0((int)DAT_002ba750, (undefined4)"Root");
	if (DAT_003055e0 == 2 || track < 0)
		return;
	if (DAT_002bae84 == 0 || disc_only != 0)
		limit = 0x1c;
	else
		limit = *(int *)(DAT_002bae84 + 0x28);
	if (track >= limit)
		return;
	clu_music_track = track;
	if (*(int *)(root + 0x54) != 0) {
		if (*(int *)(root + 0x58) != 0)
			FUN_000563f0(*(int *)(root + 0x58));
		*(int *)(root + 0x58) = *(int *)(root + 0x54);
		*(int *)(root + 0x60) = *(int *)(root + 0x5c);
		*(int *)(root + 0x68) = *(int *)(root + 100);
		*(int *)(root + 0x70) = *(int *)(root + 0x6c);
	}
	*(undefined4 *)(root + 0x5c) = start;
	*(undefined4 *)(root + 100) = volume;
	*(undefined4 *)(root + 0x6c) = fade;
	if ((DAT_002bae84 == 0 || disc_only != 0) && clu_music_samples[track] != 0)
		*(int *)(root + 0x54) = clu_music_samples[track];
	if (crossfade == 0 || *(int *)(root + 0x58) == 0)
		*(int *)(root + 0x74) = 0;
	else
		*(int *)(root + 0x74) = 1;
	if (DAT_002bae84 != 0 && disc_only == 0) {
		CLMusic_NextCustomTrack();
		clu_music_custom = DAT_002bae84;
		FUN_00055ff0(DAT_002bae84, clu_music_track, 2, volume, 0, 0);
		return;
	}
	clu_music_custom = 0;
	if (*(int *)(root + 0x74) == 0 && *(int *)(root + 0x54) != 0) {
		CLMusic_NextCustomTrack();
		/* the original pushes a seventh argument (0) that the callee does not read */
		FUN_00056370(*(int *)(root + 0x54), flags, start, 0, 0, 0);
		if ((clu_music_flags(track) & 0x800000) == 0)
			FUN_00056590(*(int *)(root + 0x54), 1);
	}
}

/* 0x00072440  ClassLib (file unknown)
Switches to music track `track` unless it plays already: fades the current
music out over `fade_out` frames and crossfades into the new track. */
void CLMusic_Switch(int track, undefined4 volume, int fade_out, undefined4 fade, int disc_only, unsigned int flags)
{
	if (DAT_002bae84 == 0) {
	no_custom:
		if (clu_music_custom == 0)
			goto check;
	} else {
		if (clu_music_custom == DAT_002bae84) {
			if (disc_only == 0)
				goto check;
			goto no_custom;
		}
		if (disc_only != 0)
			goto no_custom;
	}
	clu_music_track = -1;
check:
	if (track != clu_music_track) {
		CLMusic_FadeOut(fade_out);
		CLMusic_Play(track, 0, volume, fade, 1, disc_only, flags);
	}
}

/* 0x000724b0  ClassLib (file unknown)
Plays a random music track whose flags match `kinds` (or a random custom
soundtrack track). With `force` 0 the current track stays when it matches. */
void CLMusic_PlayRandom(unsigned int kinds, int force, undefined4 volume, int fade_out, undefined4 fade,
			unsigned int flags)
{
	int root;
	int track;
	int same;

	if (DAT_002bae84 == 0 || (kinds & 0x3ffff0) == 0) {
		if (force != 0 || clu_music_track == -1 || (clu_music_flags(clu_music_track) & kinds) == 0) {
			do {
				track = CLU_Rand() % 0x1c;
			} while ((clu_music_flags(track) & kinds) == 0);
			CLMusic_Switch(track, volume, fade_out, fade, (kinds & 0x3ffff0) == 0, flags);
		}
		return;
	}
	track = CLU_Rand();
	track = track % (int)FUN_00055fd0(DAT_002bae84);
	if (DAT_002bae84 == 0)
		same = clu_music_custom == 0;
	else
		same = clu_music_custom == DAT_002bae84;
	if (!same)
		clu_music_track = -1;
	if (track == clu_music_track)
		return;
	CLMusic_FadeOut(fade_out);
	root = FUN_0002b5b0((int)DAT_002ba750, (undefined4)"Root");
	if (DAT_003055e0 == 2 || track < 0)
		return;
	if (DAT_002bae84 == 0) {
		if (0x1b < track)
			return;
	} else if (*(int *)(DAT_002bae84 + 0x28) <= track) {
		return;
	}
	clu_music_track = track;
	if (*(int *)(root + 0x54) != 0) {
		if (*(int *)(root + 0x58) != 0)
			FUN_000563f0(*(int *)(root + 0x58));
		*(int *)(root + 0x58) = *(int *)(root + 0x54);
		*(int *)(root + 0x60) = *(int *)(root + 0x5c);
		*(int *)(root + 0x68) = *(int *)(root + 100);
		*(int *)(root + 0x70) = *(int *)(root + 0x6c);
	}
	*(int *)(root + 0x5c) = 0;
	*(undefined4 *)(root + 100) = volume;
	*(undefined4 *)(root + 0x6c) = fade;
	if (DAT_002bae84 == 0 && clu_music_samples[track] != 0)
		*(int *)(root + 0x54) = clu_music_samples[track];
	if (*(int *)(root + 0x58) == 0)
		*(int *)(root + 0x74) = 0;
	else
		*(int *)(root + 0x74) = 1;
	if (DAT_002bae84 != 0) {
		CLMusic_NextCustomTrack();
		clu_music_custom = DAT_002bae84;
		FUN_00055ff0(DAT_002bae84, clu_music_track, 2, volume, 0, 0);
		return;
	}
	clu_music_custom = 0;
	if (*(int *)(root + 0x74) == 0 && *(int *)(root + 0x54) != 0) {
		CLMusic_NextCustomTrack();
		FUN_00056370(*(int *)(root + 0x54), flags, 0, 0, 0, 0);
		if ((clu_music_flags(track) & 0x800000) == 0)
			FUN_00056590(*(int *)(root + 0x54), 1);
	}
}

/* 0x00072720  ClassLib (file unknown) */
void FUN_00072720(undefined4 value)
{
	clu_eca3c = value;
}

/* ---------- debris effect system glue (0x00080bb0-0x00080cc0)

These functions use the globals of the debris effect system of classlib_fx (0x002ed340
and on, the effect object at 0x00290ea8); they sit at the end of this module's address
range, so the original file is probably classlib_fx's first file. */

extern int DAT_002ed340;          /* debris system disabled */
extern int DAT_002ed344;          /* live debris count */
extern int DAT_002ed348;          /* debris system started */
extern int DAT_002ed34c;          /* debris effect system */
extern int DAT_002ed44c;          /* extra debris effect system */
extern unsigned char DAT_00290ea8[]; /* debris effect object (classlib_fx data) */

/* 0x00080bb0  ClassLib (file unknown)
Starts the debris system (once): stops the old effect system and restarts it. */
void CLDebris_Start(void)
{
	if (DAT_002ed340 != 0)
		return;
	DAT_002ed348 = 1;
	if (DAT_002ed34c != 0)
		FUN_0003c8b0(DAT_002ed34c);
	FUN_00086350((int *)DAT_00290ea8);
	FUN_00085200();
}

/* 0x00080bf0 and 0x00080c50 (debris callbacks missing from the table) are defined by
bumpercars_a (ClassLib/Debris.c), which owns the debris system. */

/* 0x00080cc0  ClassLib (file unknown)
Stops the debris effects. */
void CLDebris_Stop(void)
{
	FUN_000863c0((int *)DAT_00290ea8);
	FUN_0003c690(DAT_002ed34c, 0);
	FUN_000861b0((undefined4 *)DAT_00290ea8);
	FUN_0003c690(DAT_002ed44c, 0);
}
