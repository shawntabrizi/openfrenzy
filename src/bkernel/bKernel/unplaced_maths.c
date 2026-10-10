/*
Babel maths in bKernel's address range: 4x4 matrices (row vectors, D3D layout: m[12..14]
is the translation), vectors and quaternions (x, y, z, w), the random seed, and a few tree
helpers.

The table puts these functions in bKernel (gap functions after package.cpp); they are
probably BMATHS code. The x87 computed in extended precision; the port computes the
trigonometry and the products marked double in the original's float10 in double, and
rounds to float where the original stored a float. Outputs are written in the original
order, so an output that aliases an input behaves as before.
*/

#include "bkernel_core.h"

#include <math.h>
#include <string.h>

/* 0x00285630: the random seed of the game's rand() (seed * 0x343fd + 0x269ec3) */
uint DAT_00285630 = 1;
/* 0x002855f0, the identity matrix, is defined by module bdisplay (BDISPLAY/xbData.c) */

/* ---------- matrices */

/* 0x0004ff30
bmMatrixMultiply: out = b * a (each output read from memory as it is written). */
void bmMatrixMultiply(float *out, const float *a, const float *b)
{
	out[0] = b[3] * a[12] + a[0] * b[0] + a[4] * b[1] + a[8] * b[2];
	out[4] = a[4] * b[5] + a[0] * b[4] + b[7] * a[12] + a[8] * b[6];
	out[8] = a[8] * b[10] + a[4] * b[9] + b[11] * a[12] + b[8] * a[0];
	out[12] = b[12] * a[0] + a[12] * b[15] + a[4] * b[13] + a[8] * b[14];
	out[1] = a[1] * b[0] + a[9] * b[2] + a[5] * b[1] + a[13] * b[3];
	out[5] = b[6] * a[9] + b[7] * a[13] + b[5] * a[5] + a[1] * b[4];
	out[9] = a[1] * b[8] + b[10] * a[9] + b[11] * a[13] + a[5] * b[9];
	out[13] = a[5] * b[13] + a[13] * b[15] + a[9] * b[14] + a[1] * b[12];
	out[2] = a[2] * b[0] + a[10] * b[2] + a[6] * b[1] + a[14] * b[3];
	out[6] = b[6] * a[10] + b[7] * a[14] + b[5] * a[6] + a[2] * b[4];
	out[10] = a[2] * b[8] + b[10] * a[10] + b[11] * a[14] + a[6] * b[9];
	out[14] = a[6] * b[13] + a[10] * b[14] + a[14] * b[15] + a[2] * b[12];
	out[3] = a[3] * b[0] + a[11] * b[2] + a[7] * b[1] + a[15] * b[3];
	out[7] = b[6] * a[11] + b[7] * a[15] + b[5] * a[7] + a[3] * b[4];
	out[11] = a[3] * b[8] + b[10] * a[11] + b[11] * a[15] + a[7] * b[9];
	out[15] = a[7] * b[13] + a[11] * b[14] + a[15] * b[15] + a[3] * b[12];
}

/* 0x00050150
bmMatrixMultiplyCopy: out = b * a, with a read first and b a row at a time (safe when out
is a or b). */
void bmMatrixMultiplyCopy(float *out, const float *a, const float *b)
{
	float m[16];
	float r0, r1, r2, r3;
	int row;

	memcpy(m, a, sizeof(m));
	for (row = 0; row < 4; row++)
	{
		r0 = b[row * 4 + 0];
		r1 = b[row * 4 + 1];
		r2 = b[row * 4 + 2];
		r3 = b[row * 4 + 3];
		out[row * 4 + 0] = r2 * m[8] + r0 * m[0] + r1 * m[4] + r3 * m[12];
		out[row * 4 + 1] = r2 * m[9] + r0 * m[1] + r1 * m[5] + r3 * m[13];
		out[row * 4 + 2] = r2 * m[10] + r0 * m[2] + r1 * m[6] + r3 * m[14];
		out[row * 4 + 3] = r2 * m[11] + r0 * m[3] + r1 * m[7] + r3 * m[15];
	}
}

/* 0x00050250 */
void bmMatrixTranslation(float *m, float x, float y, float z)
{
	m[12] = x;
	m[13] = y;
	m[0] = 1.0f;
	m[5] = 1.0f;
	m[10] = 1.0f;
	m[15] = 1.0f;
	m[14] = z;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[11] = 0.0f;
}

/* 0x000502a0 */
void bmMatrixScaling(float *m, float x, float y, float z)
{
	m[0] = x;
	m[5] = y;
	m[10] = z;
	m[15] = 1.0f;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
}

/* 0x000502f0
bmMatrixInverseAffine: inverts the 3x3 part and the translation of m. 0 (out unchanged)
when the matrix is singular (|det| / (sum of |terms|) below 1e-15). */
int bmMatrixInverseAffine(float *out, const float *m)
{
	float positive;
	float negative;
	float term;
	float det;
	float inverse;

	term = m[10] * m[5] * m[0];
	positive = term;
	negative = 0.0f;
	if (term <= 0.0f)
	{
		positive = 0.0f;
		negative = term;
	}
#define BM_ACCUMULATE(t) do { term = (t); if (term <= 0.0f) negative = term + negative; \
	else positive = term + positive; } while (0)
	BM_ACCUMULATE(m[8] * m[6] * m[1]);
	BM_ACCUMULATE(m[9] * m[2] * m[4]);
	BM_ACCUMULATE(-(m[8] * m[2] * m[5]));
	BM_ACCUMULATE(-(m[10] * m[4] * m[1]));
	BM_ACCUMULATE(-(m[9] * m[6] * m[0]));
#undef BM_ACCUMULATE
	det = negative + positive;
	if (det == 0.0f || fabs((double)det / (double)(positive - negative)) < 1e-15)
		return 0;
	inverse = 1.0f / det;
	out[0] = (m[10] * m[5] - m[9] * m[6]) * inverse;
	out[4] = -((m[10] * m[4] - m[8] * m[6]) * inverse);
	out[8] = (m[9] * m[4] - m[8] * m[5]) * inverse;
	out[1] = -((m[10] * m[1] - m[9] * m[2]) * inverse);
	out[5] = (m[10] * m[0] - m[8] * m[2]) * inverse;
	out[9] = -((m[9] * m[0] - m[8] * m[1]) * inverse);
	out[2] = (m[6] * m[1] - m[2] * m[5]) * inverse;
	out[6] = -((m[0] * m[6] - m[2] * m[4]) * inverse);
	out[10] = (m[0] * m[5] - m[4] * m[1]) * inverse;
	out[12] = -(m[12] * out[0] + out[4] * m[13] + m[14] * out[8]);
	out[13] = -(m[14] * out[9] + out[5] * m[13] + m[12] * out[1]);
	{
		float x = m[12];
		float y = m[13];
		float z = m[14];

		out[11] = 0.0f;
		out[7] = 0.0f;
		out[3] = 0.0f;
		out[15] = 1.0f;
		out[14] = -(z * out[10] + out[6] * y + x * out[2]);
	}
	return 1;
}

/* 0x00050530
bmTransformVector4InPlace: v = v * m. */
void bmTransformVector4InPlace(const float *m, float *v)
{
	float x = v[0];
	float y = v[1];
	float z = v[2];
	float w = v[3];

	v[0] = x * m[0] + y * m[4] + z * m[8] + w * m[12];
	v[1] = x * m[1] + y * m[5] + z * m[9] + w * m[13];
	v[2] = x * m[2] + y * m[6] + z * m[10] + w * m[14];
	v[3] = x * m[3] + y * m[7] + z * m[11] + w * m[15];
}

/* 0x000505c0
bmTransformVector4: out = v * m. */
void bmTransformVector4(float *out, const float *m, const float *v)
{
	float x = v[0];
	float y = v[1];
	float z = v[2];
	float w = v[3];

	out[0] = x * m[0] + y * m[4] + z * m[8] + w * m[12];
	out[1] = x * m[1] + y * m[5] + z * m[9] + w * m[13];
	out[2] = x * m[2] + y * m[6] + z * m[10] + w * m[14];
	out[3] = x * m[3] + y * m[7] + z * m[11] + w * m[15];
}

/* 0x00050650
bmMatrixTranspose (in the original order, so out must not be m). */
void bmMatrixTranspose(float *out, const float *m)
{
	out[0] = m[0];
	out[1] = m[4];
	out[2] = m[8];
	out[3] = m[12];
	out[4] = m[1];
	out[5] = m[5];
	out[6] = m[9];
	out[7] = m[13];
	out[8] = m[2];
	out[9] = m[6];
	out[10] = m[10];
	out[11] = m[14];
	out[12] = m[3];
	out[13] = m[7];
	out[14] = m[11];
	out[15] = m[15];
}

/* 0x000506c0
bmMatrixScaleTranslate: the 3x3 part of m with its rows scaled by sx, sy, sz, translated
to x, y, z. */
void bmMatrixScaleTranslate(float *out, float x, float y, float z, const float *m, float sx,
	float sy, float sz)
{
	float value;

	out[0] = sx * m[0];
	out[4] = sy * m[4];
	value = m[8];
	out[12] = x;
	out[8] = sz * value;
	out[1] = sx * m[1];
	out[5] = sy * m[5];
	value = m[9];
	out[13] = y;
	out[9] = sz * value;
	out[2] = sx * m[2];
	out[6] = sy * m[6];
	value = m[10];
	out[14] = z;
	out[10] = sz * value;
	out[3] = 0.0f;
	out[7] = 0.0f;
	out[11] = 0.0f;
	out[15] = 1.0f;
}

/* 0x00050750 */
void bmMatrixRotationX(float *m, float angle)
{
	double c = cos(angle);
	double s;

	m[0] = 1.0f;
	m[15] = 1.0f;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[5] = (float)c;
	s = sin(angle);
	m[6] = (float)s;
	m[9] = (float)-s;
	m[10] = (float)c;
}

/* 0x000507a0 */
void bmMatrixRotationY(float *m, float angle)
{
	double c = cos(angle);
	double s;

	m[5] = 1.0f;
	m[15] = 1.0f;
	m[1] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[9] = 0.0f;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[0] = (float)c;
	s = sin(angle);
	m[2] = (float)-s;
	m[8] = (float)s;
	m[10] = (float)c;
}

/* 0x000507f0 */
void bmMatrixRotationZ(float *m, float angle)
{
	double c = cos(angle);
	double s;

	m[10] = 1.0f;
	m[15] = 1.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[0] = (float)c;
	s = sin(angle);
	m[1] = (float)s;
	m[4] = (float)-s;
	m[5] = (float)c;
}

/* clears the translation row and column of a rotation matrix */
static void bmClearAffine(float *m)
{
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;
}

/* 0x00050840
bmMatrixRotationXY: rotation by x about X, then by y about Y. */
void bmMatrixRotationXY(float *m, float x, float y)
{
	double cx = cos(x);
	double sx;
	double sy;
	float cy;

	m[4] = 0.0f;
	bmClearAffine(m);
	sx = sin(x);
	cy = (float)cos(y);
	sy = sin(y);
	m[0] = cy;
	m[8] = (float)sy;
	m[1] = (float)(sy * sx);
	m[5] = (float)cx;
	m[9] = (float)-((double)cy * sx);
	m[2] = (float)-(sy * cx);
	m[6] = (float)sx;
	m[10] = (float)((double)cy * cx);
}

/* 0x000508c0
bmMatrixRotationYX: rotation by y about Y, then by x about X. */
void bmMatrixRotationYX(float *m, float x, float y)
{
	double cx = cos(x);
	double sx;
	double sy;
	float cy;

	m[12] = 0.0f;
	m[1] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;
	sx = sin(x);
	cy = (float)cos(y);
	sy = sin(y);
	m[0] = cy;
	m[4] = (float)(sy * sx);
	m[8] = (float)(sy * cx);
	m[5] = (float)cx;
	m[9] = (float)-sx;
	m[2] = (float)-sy;
	m[6] = (float)((double)cy * sx);
	m[10] = (float)((double)cy * cx);
}

/* 0x00050940
bmMatrixRotationXZ: rotation by x about X and by z about Z. */
void bmMatrixRotationXZ(float *m, float x, float z)
{
	double cx = cos(x);
	double sx;
	double cz;
	double sz;

	m[12] = 0.0f;
	m[13] = 0.0f;
	m[2] = 0.0f;
	m[14] = 0.0f;
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;
	sx = sin(x);
	cz = cos(z);
	sz = sin(z);
	m[0] = (float)cz;
	m[4] = (float)-(sz * cx);
	m[8] = (float)(sz * sx);
	m[1] = (float)sz;
	m[5] = (float)(cz * cx);
	m[9] = (float)-(cz * sx);
	m[6] = (float)sx;
	m[10] = (float)cx;
}

/* The three-angle rotations below differ in the order they combine the angles. Their
names are inferred from the products (only bmMatrixRotationYXZ is known from its callers:
pitch, yaw, roll); the arguments are the angles in the original order. */

/* 0x00050a20
bmMatrixRotationXYZ. */
void bmMatrixRotationXYZ(float *m, float x, float y, float z)
{
	double cx = cos(x);
	double sx;
	double sy;
	double sz;
	float cy;
	float cz;

	bmClearAffine(m);
	sx = sin(x);
	cy = (float)cos(y);
	sy = sin(y);
	cz = (float)cos(z);
	sz = sin(z);
	m[0] = cz * cy;
	m[4] = (float)-((double)cy * sz);
	m[8] = (float)sy;
	m[1] = (float)(sz * cx + (double)cz * sy * sx);
	m[5] = (float)((double)cz * cx - (double)(float)(sz * sy) * sx);
	m[9] = (float)-((double)cy * sx);
	m[2] = (float)(sz * sx - (double)cz * sy * cx);
	m[6] = (float)((double)cz * sx + (double)(float)(sz * sy) * cx);
	m[10] = (float)((double)cy * cx);
}

/* 0x00050af0
bmMatrixRotationXZY. */
void bmMatrixRotationXZY(float *m, float x, float y, float z)
{
	double cx = cos(x);
	double sx;
	double cy;
	double sy;
	double cz;
	double sz;

	bmClearAffine(m);
	sx = sin(x);
	cy = cos(y);
	sy = sin(y);
	cz = cos(z);
	sz = sin(z);
	m[0] = (float)(cz * cy);
	m[4] = (float)-sz;
	m[8] = (float)(cz * sy);
	m[1] = (float)(sy * sx + sz * cy * cx);
	m[5] = (float)(cz * cx);
	m[9] = (float)(sz * sy * cx - cy * sx);
	m[2] = (float)((double)(float)(sz * cy) * sx - sy * cx);
	m[6] = (float)(cz * sx);
	m[10] = (float)(cy * cx + (double)(float)(sz * sy) * sx);
}

/* 0x00050ba0
bmMatrixRotationYXZ: the order the game's cameras use (pitch x, yaw y, roll z). */
void bmMatrixRotationYXZ(float *m, float x, float y, float z)
{
	double cx = cos(x);
	double sx;
	double sy;
	double cz;
	double sz;
	float cy;

	bmClearAffine(m);
	sx = sin(x);
	cy = (float)cos(y);
	sy = sin(y);
	cz = cos(z);
	sz = sin(z);
	m[0] = (float)((double)(float)(sz * sy) * sx + (double)(float)(cz * (double)cy));
	m[4] = (float)((double)(float)(cz * sy) * sx - (double)(float)((double)cy * sz));
	m[8] = (float)(sy * cx);
	m[1] = (float)(sz * cx);
	m[5] = (float)(cz * cx);
	m[9] = (float)-sx;
	m[2] = (float)((double)(float)((double)cy * sz) * sx - (double)(float)(cz * sy));
	m[6] = (float)((double)(float)(cz * (double)cy) * sx + (double)(float)(sz * sy));
	m[10] = (float)((double)cy * cx);
}

/* 0x00050c70
bmMatrixRotationYZX. */
void bmMatrixRotationYZX(float *m, float x, float y, float z)
{
	double cx = cos(x);
	double sx;
	double cy;
	double sy;
	double cz;
	float sz;

	m[15] = 1.0f;
	sx = sin(x);
	cy = cos(y);
	sy = sin(y);
	cz = cos(z);
	sz = (float)sin(z);
	m[1] = sz;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[0] = (float)(cz * cy);
	m[14] = 0.0f;
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[4] = (float)(sy * sx - (double)sz * cy * cx);
	m[8] = (float)(sy * cx + (double)sz * cy * sx);
	m[5] = (float)(cz * cx);
	m[9] = (float)-(cz * sx);
	m[2] = (float)-(cz * sy);
	m[6] = (float)(cy * sx + sy * (double)sz * cx);
	m[10] = (float)(cy * cx - sy * (double)sz * sx);
}

/* 0x00050d20
bmMatrixRotationZXY. */
void bmMatrixRotationZXY(float *m, float x, float y, float z)
{
	double cx = cos(x);
	double sx;
	double sy;
	double cz;
	double sz;
	float cy;

	bmClearAffine(m);
	sx = sin(x);
	cy = (float)cos(y);
	sy = sin(y);
	cz = cos(z);
	sz = sin(z);
	m[0] = (float)((double)(float)(cz * (double)cy) - sz * sy * sx);
	m[4] = (float)-(sz * cx);
	m[8] = (float)(sz * (double)cy * sx + cz * sy);
	m[1] = (float)(cz * sy * sx + sz * (double)cy);
	m[5] = (float)(cz * cx);
	m[9] = (float)((double)(float)(sz * sy) - (double)(float)(cz * (double)cy) * sx);
	m[2] = (float)-(sy * cx);
	m[6] = (float)sx;
	m[10] = (float)((double)cy * cx);
}

/* 0x00050dd0
bmMatrixRotationZYX. */
void bmMatrixRotationZYX(float *m, float x, float y, float z)
{
	double cx = cos(x);
	double sx;
	double sy;
	double cz;
	double sz;
	float cy;

	bmClearAffine(m);
	sx = sin(x);
	cy = (float)cos(y);
	sy = sin(y);
	cz = cos(z);
	sz = sin(z);
	m[0] = (float)(cz * (double)cy);
	m[4] = (float)(cz * sy * sx - sz * cx);
	m[8] = (float)(sz * sx + cz * sy * cx);
	m[1] = (float)((double)cy * sz);
	m[5] = (float)(cz * cx + sz * sy * sx);
	m[9] = (float)(sz * sy * cx - cz * sx);
	m[2] = (float)-sy;
	m[6] = (float)((double)cy * sx);
	m[10] = (float)((double)cy * cx);
}

/* 0x00050e80
bmMatrixLookAt: a camera matrix at from looking at to (roll about the view axis). */
void bmMatrixLookAt(float *out, const float *from, const float *to, float roll)
{
	double dx = (double)from[0] - (double)to[0];
	float dy = from[1] - to[1];
	double dz = (double)from[2] - (double)to[2];
	double yaw;
	float pitch;
	float horizontal;

	if (dx == 0.0)
		yaw = dz < 0.0 ? (double)-1.57079625f : (double)1.57079625f;
	else
		yaw = atan2(dz, dx);
	yaw += (double)1.57079625f;
	horizontal = bmFastSqrt((float)(dx * dx + dz * dz));
	if (horizontal == 0.0f)
		pitch = dy < 0.0f ? -1.57079625f : 1.57079625f;
	else
		pitch = (float)atan((double)dy / (double)horizontal);
	bmMatrixRotationYXZ(out, pitch, -(float)yaw, roll);
	out[12] = from[0];
	out[13] = from[1];
	out[14] = from[2];
}

/* 0x00050f70 */
void bmSetRandomSeed(uint seed)
{
	DAT_00285630 = seed;
}

/* 0x00050f80
bmFastSqrt: an approximate square root by halving the exponent bits. */
float bmFastSqrt(float value)
{
	uint bits;
	float result;

	memcpy(&bits, &value, sizeof(bits));
	bits = (uint)(((int)(bits - 0x3f800000) >> 1) + 0x3f800000);
	memcpy(&result, &bits, sizeof(result));
	return result;
}

/* ---------- vectors */

/* 0x00050fa0
bmVectorLerp: out = a + (b - a) * t. */
void bmVectorLerp(float *out, const float *a, const float *b, float t)
{
	float s = 1.0f - t;

	out[0] = t * b[0] + s * a[0];
	out[1] = t * b[1] + s * a[1];
	out[2] = t * b[2] + s * a[2];
}

/* 0x00050ff0
bmVectorBlend: out = a * s + b * t. */
void bmVectorBlend(float *out, const float *a, float s, const float *b, float t)
{
	out[0] = t * b[0] + s * a[0];
	out[1] = t * b[1] + s * a[1];
	out[2] = t * b[2] + s * a[2];
}

/* 0x00051040
bmTriangleNormal: the unit normal of the triangle a, b, c (0 when degenerate). */
void bmTriangleNormal(float *out, const float *a, const float *b, const float *c)
{
	float e1x = b[0] - a[0];
	float e1y = b[1] - a[1];
	float e1z = b[2] - a[2];
	float e2x = c[0] - b[0];
	float e2y = c[1] - b[1];
	float e2z = c[2] - b[2];
	float nx = e2z * e1y - e2y * e1z;
	float ny;
	float nz;
	float length;

	out[0] = nx;
	ny = e2x * e1z - e2z * e1x;
	out[1] = ny;
	nz = e2y * e1x - e2x * e1y;
	out[2] = nz;
	length = sqrtf(nx * nx + ny * ny + nz * nz);
	if (length == 0.0f)
	{
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
		return;
	}
	length = 1.0f / length;
	out[0] = nx * length;
	out[1] = ny * length;
	out[2] = nz * length;
}

/* 0x00051110
bmVectorRotateX. */
void bmVectorRotateX(float *out, const float *v, float angle)
{
	double c = cos(angle);
	float z = v[2];
	float y = v[1];
	double s;

	out[0] = v[0];
	s = sin(angle);
	out[1] = (float)c * y - (float)s * z;
	out[2] = (float)s * y + (float)c * z;
}

/* 0x00051170
bmVectorRotateY. */
void bmVectorRotateY(float *out, const float *v, float angle)
{
	float x = v[0];
	float y = v[1];
	float z = v[2];
	float c = (float)cos(angle);
	float s = (float)sin(angle);

	out[0] = (float)((double)c * (double)x + (double)sin(angle) * (double)z);
	out[1] = y;
	out[2] = (float)((double)c * (double)z - (double)s * (double)x);
}

/* 0x000511d0
bmVectorRotateZ. */
void bmVectorRotateZ(float *out, const float *v, float angle)
{
	float x = v[0];
	float y = v[1];
	float z = v[2];
	double c = cos(angle);
	double s = sin(angle);

	out[0] = (float)(c * (double)x + (double)y * s);
	out[1] = (float)(c * (double)y - s * (double)x);
	out[2] = z;
}

/* 0x00051220
bmVectorRotateXY: about X by a, then about Y by b. */
void bmVectorRotateXY(float *out, const float *v, float a, float b)
{
	float x = v[0];
	float y = v[1];
	float z = v[2];
	double ca = cos(a);
	double sa = sin(a);
	float rz = (float)(sa * (double)y + ca * (double)z);
	double cb = cos(b);
	double sb = sin(b);

	out[0] = (float)cb * x + rz * (float)sb;
	out[1] = (float)(ca * (double)y - sa * (double)z);
	out[2] = rz * (float)cb - (float)sb * x;
}

/* 0x000512a0
bmVectorRotateZY: about Z by a, then about Y by b. */
void bmVectorRotateZY(float *out, const float *v, float a, float b)
{
	float x = v[0];
	float y = v[1];
	float z = v[2];
	double ca = cos(a);
	double sa = sin(a);
	float rx = (float)(ca * (double)x + sa * (double)y);
	double cb = cos(b);
	double sb = sin(b);

	out[0] = (float)(sb * (double)z + (double)rx * cb);
	out[1] = (float)(ca * (double)y - sa * (double)x);
	out[2] = (float)(cb * (double)z - (double)rx * sb);
}

/* 0x00051310
bmVectorRotateXYZ: about X by a, Y by b, Z by c. */
void bmVectorRotateXYZ(float *out, const float *v, float a, float b, float c)
{
	float x = v[0];
	double ca = cos(a);
	double sa = sin(a);
	float rz = (float)(sa * (double)v[1] + ca * (double)v[2]);
	float ry = (float)(ca * (double)v[1] - sa * (double)v[2]);
	double cb = cos(b);
	double sb = sin(b);
	float rx = (float)(cb * (double)x + (double)rz * sb);
	double cc = cos(c);
	double sc = sin(c);

	out[0] = (float)(sc * (double)ry + (double)rx * cc);
	out[1] = (float)(cc * (double)ry - (double)rx * sc);
	out[2] = (float)(cb * (double)rz - sb * (double)x);
}

/* 0x000513b0
bmVectorRotateZXY: about Z by a, X by b, Y by c. */
void bmVectorRotateZXY(float *out, const float *v, float a, float b, float c)
{
	float z = v[2];
	double ca = cos(a);
	double sa = sin(a);
	float ry = (float)ca * v[1] - (float)sa * v[0];
	float rx = (float)((double)(float)ca * (double)v[0] + sa * (double)v[1]);
	double cb = cos(b);
	double sb = sin(b);
	float rz = (float)(sb * (double)ry + cb * (double)z);
	double cc = cos(c);
	double sc = sin(c);

	out[0] = (float)(cc * (double)rx + (double)rz * sc);
	out[1] = (float)(cb * (double)ry - sb * (double)z);
	out[2] = (float)((double)rz * cc - sc * (double)rx);
}

/* 0x00051470
bmVectorLengthSq: |v|^2. Ghidra's output was empty: the result is on the x87 stack (ST0). */
float bmVectorLengthSq(const float *v)
{
	return (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2];
}

/* 0x000514a0
bmVectorNormalise: out = v / |v| (out unchanged when v is 0). */
void bmVectorNormalise(float *out, const float *v)
{
	float length = sqrtf(v[2] * v[2] + v[1] * v[1] + v[0] * v[0]);

	if (length != 0.0f)
	{
		length = 1.0f / length;
		out[0] = length * v[0];
		out[1] = length * v[1];
		out[2] = length * v[2];
	}
}

/* 0x00051500 */
float bmVectorDistance(const float *a, const float *b)
{
	double dx = (double)a[0] - (double)b[0];
	double dy = (double)a[1] - (double)b[1];
	double dz = (double)a[2] - (double)b[2];

	return (float)sqrt(dy * dy + dx * dx + dz * dz);
}

/* 0x00051540
bmVectorDistanceSq: |a - b|^2. Ghidra's output was empty: the result is on the x87 stack (ST0).
Other modules call it through casts to float (*)(const float *, const float *). */
float bmVectorDistanceSq(const float *a, const float *b)
{
	float x = a[0] - b[0];
	float y = a[1] - b[1];
	float z = a[2] - b[2];

	return (z * z + x * x) + y * y;
}

/* 0x00051570
bmVectorSetLength: out = v scaled to length. */
void bmVectorSetLength(float *out, const float *v, float length)
{
	length = length / sqrtf(v[2] * v[2] + v[1] * v[1] + v[0] * v[0]);
	out[0] = length * v[0];
	out[1] = length * v[1];
	out[2] = length * v[2];
}

/* ---------- quaternions (x, y, z, w) */

/* 0x000515c0
bmQuaternionToMatrix. */
void bmQuaternionToMatrix(float *m, const float *q)
{
	float x = q[0];
	float z = q[2];
	float y = q[1];
	float w = q[3];
	float s = 2.0f / (x * x + y * y + z * z + w * w);
	float ys = s * y;
	float zs = s * z;
	float wxs = s * x * w;
	float xxs = s * x * x;

	m[0] = 1.0f - (ys * y + zs * z);
	m[4] = ys * x + zs * w;
	m[8] = zs * x - ys * w;
	m[1] = ys * x - zs * w;
	m[5] = 1.0f - (zs * z + xxs);
	m[9] = wxs + zs * y;
	m[2] = zs * x + ys * w;
	m[6] = zs * y - wxs;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[15] = 1.0f;
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[10] = 1.0f - (ys * y + xxs);
	m[11] = 0.0f;
}

/* 0x00051700
bmMatrixToQuaternion. */
void bmMatrixToQuaternion(float *q, const float *m)
{
	float trace = m[5] + m[10] + m[0];
	float root;

	if (trace > 0.0f)
	{
		root = sqrtf(trace + 1.0f);
		q[3] = 0.5f * root;
		root = 0.5f / root;
		q[0] = (m[9] - m[6]) * root;
		q[1] = (m[2] - m[8]) * root;
		q[2] = (m[4] - m[1]) * root;
		return;
	}
	if (m[5] <= m[0])
	{
		if (m[10] <= m[0])
		{
			root = sqrtf((m[0] - (m[5] + m[10])) + 1.0f);
			q[0] = 0.5f * root;
			root = 0.5f / root;
			q[3] = (m[9] - m[6]) * root;
			q[1] = (m[4] + m[1]) * root;
			q[2] = (m[8] + m[2]) * root;
			return;
		}
	}
	else if (m[10] <= m[5])
	{
		root = sqrtf((m[5] - (m[0] + m[10])) + 1.0f);
		q[1] = 0.5f * root;
		root = 0.5f / root;
		q[3] = (m[2] - m[8]) * root;
		q[2] = (m[9] + m[6]) * root;
		q[0] = (m[4] + m[1]) * root;
		return;
	}
	root = sqrtf((m[10] - (m[5] + m[0])) + 1.0f);
	q[2] = 0.5f * root;
	root = 0.5f / root;
	q[3] = (m[4] - m[1]) * root;
	q[0] = (m[8] + m[2]) * root;
	q[1] = (m[9] + m[6]) * root;
}

/* the common part of the two slerps: from a toward b (already flipped) by t */
static void bmQuaternionSlerpTo(float *out, const float *a, float bx, float by, float bz, float bw,
	float t)
{
	float cosine = bx * a[0] + by * a[1] + bz * a[2] + bw * a[3];
	double scaleA;
	double scaleB;
	double omega;
	double sine;

	if (cosine + 1.0f > 9.99999975e-06f)
	{
		if (1.0f - cosine <= 9.99999975e-06f)
		{
			scaleA = 1.0 - (double)t;
			scaleB = (double)t;
		}
		else
		{
			omega = acos((double)cosine);
			sine = sin(omega);
			scaleA = sin((1.0 - (double)t) * (double)(float)omega) * (double)(float)(1.0 / sine);
			scaleB = sin((double)(float)omega * (double)t) * (double)(float)(1.0 / sine);
		}
		out[0] = (float)(scaleB * (double)bx + scaleA * (double)a[0]);
		out[1] = (float)(scaleB * (double)by + scaleA * (double)a[1]);
		out[2] = (float)(scaleB * (double)bz + scaleA * (double)a[2]);
		out[3] = (float)(scaleB * (double)bw + scaleA * (double)a[3]);
		return;
	}
	/* opposite: go through a perpendicular quaternion */
	out[0] = -a[1];
	out[1] = a[0];
	out[2] = -a[3];
	out[3] = a[2];
	scaleA = sin((1.0 - (double)t) * (double)1.57079637f);
	scaleB = sin((double)t * (double)1.57079637f);
	out[0] = (float)(scaleA * (double)a[0] + scaleB * (double)out[0]);
	out[1] = (float)(scaleA * (double)a[1] + scaleB * (double)out[1]);
	out[2] = (float)(scaleA * (double)a[2] + scaleB * (double)out[2]);
}

/* 0x00051860
bmQuaternionSlerp: the shortest arc from a to b. */
void bmQuaternionSlerp(float *out, const float *a, const float *b, float t)
{
	float x = b[0];
	float y = b[1];
	float z = b[2];
	float w = b[3];

	if ((x + a[0]) * (x + a[0]) + (y + a[1]) * (y + a[1]) + (z + a[2]) * (z + a[2]) +
		(w + a[3]) * (w + a[3]) <=
		(a[0] - x) * (a[0] - x) + (a[1] - y) * (a[1] - y) + (a[2] - z) * (a[2] - z) +
		(a[3] - w) * (a[3] - w))
	{
		x = -x;
		y = -y;
		z = -z;
		w = -w;
	}
	bmQuaternionSlerpTo(out, a, x, y, z, w, t);
}

/* 0x00051a80
bmQuaternionSlerpLong: the longer arc from a to b. */
void bmQuaternionSlerpLong(float *out, const float *a, const float *b, float t)
{
	float x = b[0];
	float y = b[1];
	float z = b[2];
	float w = b[3];

	if ((a[0] - x) * (a[0] - x) + (a[1] - y) * (a[1] - y) + (a[2] - z) * (a[2] - z) +
		(a[3] - w) * (a[3] - w) <
		(x + a[0]) * (x + a[0]) + (y + a[1]) * (y + a[1]) + (z + a[2]) * (z + a[2]) +
		(w + a[3]) * (w + a[3]))
	{
		x = -x;
		y = -y;
		z = -z;
		w = -w;
	}
	bmQuaternionSlerpTo(out, a, x, y, z, w, t);
}

/* 0x00051ca0
bmQuaternionMultiply: out = a * b. */
void bmQuaternionMultiply(float *out, const float *a, const float *b)
{
	out[3] = b[3] * a[3] - (a[1] * b[1] + a[0] * b[0] + a[2] * b[2]);
	out[0] = (b[0] * a[3] + b[3] * a[0] + a[1] * b[2]) - a[2] * b[1];
	out[1] = (a[2] * b[0] + b[1] * a[3] + b[3] * a[1]) - b[2] * a[0];
	out[2] = (a[2] * b[3] + b[2] * a[3] + b[1] * a[0]) - a[1] * b[0];
}

/* 0x00051d30
bmQuaternionToAxisAngle: out = (unit axis, angle); a zero rotation gives the axis
(0, 1, 0). */
void bmQuaternionToAxisAngle(float *out, const float *q)
{
	double angle = acos((double)q[3]) * 2.0;
	float sine = (float)sin(0.5 * angle);
	double inverse;
	double length;

	out[3] = (float)angle;
	if (sine == 0.0f)
	{
		out[0] = 0.0f;
		out[1] = 1.0f;
		out[2] = 0.0f;
		return;
	}
	inverse = 1.0 / (double)sine;
	out[0] = (float)(inverse * (double)q[0]);
	out[1] = (float)(inverse * (double)q[1]);
	out[2] = (float)(inverse * (double)q[2]);
	length = sqrt((double)out[0] * out[0] + (double)out[1] * out[1] + (double)out[2] * out[2]);
	if (length != 0.0)
	{
		length = 1.0 / length;
		out[0] = (float)(length * out[0]);
		out[1] = (float)(length * out[1]);
		out[2] = (float)(length * out[2]);
	}
}

/* 0x00051df0
bmAxisAngleToQuaternion: (axis, angle) to a quaternion. */
void bmAxisAngleToQuaternion(float *q, const float *axisAngle)
{
	double s = sin((double)axisAngle[3] * 0.5);
	double c = cos((double)axisAngle[3] * 0.5);

	q[3] = (float)c;
	q[0] = (float)(s * (double)axisAngle[0]);
	q[1] = (float)(s * (double)axisAngle[1]);
	q[2] = (float)(s * (double)axisAngle[2]);
}

/* 0x00051e30 */
void bmQuaternionRotationX(float *q, float angle)
{
	q[1] = 0.0f;
	q[2] = 0.0f;
	q[3] = (float)cos((double)angle * 0.5);
	q[0] = (float)sin((double)angle * 0.5);
}

/* 0x00051e60 */
void bmQuaternionRotationY(float *q, float angle)
{
	q[0] = 0.0f;
	q[2] = 0.0f;
	q[3] = (float)cos((double)angle * 0.5);
	q[1] = (float)sin((double)angle * 0.5);
}

/* 0x00051e90 */
void bmQuaternionRotationZ(float *q, float angle)
{
	q[1] = 0.0f;
	q[0] = 0.0f;
	q[3] = (float)cos((double)angle * 0.5);
	q[2] = (float)sin((double)angle * 0.5);
}

/* 0x00051ec0
bmQuaternionFromDirection: the rotation that looks along direction, with roll. */
void bmQuaternionFromDirection(float *q, const float *direction, float roll)
{
	float matrix[16];
	double yaw;
	double pitch;

	yaw = atan2((double)direction[2], (double)direction[0]) + (double)1.57079625f;
	while (yaw > (double)3.1415925f)
		yaw -= (double)6.28318548f;
	pitch = atan2((double)direction[1], sqrt((double)direction[2] * direction[2] +
		(double)direction[0] * direction[0]));
	while (pitch > (double)3.1415925f)
		pitch -= (double)6.28318548f;
	bmMatrixRotationYXZ(matrix, (float)pitch, (float)(yaw * -1.0), roll);
	bmMatrixToQuaternion(q, matrix);
}

/* ---------- trees (a circular sibling list per level; node[3] is the first child) */

/* 0x00051f80
bmCountTreeNodes: the nodes of a sibling ring and all their children. */
int bmCountTreeNodes(int *node)
{
	int *current = node;
	int count = 0;

	do
	{
		count++;
		if (current[3] != 0)
			count += bmCountTreeNodes((int *)current[3]);
		current = (int *)current[0];
	} while (current != node);
	return count;
}

/* 0x00051fb0
bmFreeTreeNode: frees a node's data (node[2]) and the node when node[3] bit 0 says so. */
void bmFreeTreeNode(int *node)
{
	bkHeapFree((void *)node[2]);
	node[2] = 0;
	if (node[3] & 1)
		bkHeapFree(node);
}

/* 0x00051fe0
bmRelocateTree: adds base to the offsets of a loaded tree (links, data, children). As in
the original, a node's data offset (node[2]) is relocated into the first node of the ring.
*/
void bmRelocateTree(int base, int *node)
{
	int *current = node;

	do
	{
		current[0] += base;
		current[15] += base;
		current[1] += base;
		current[13] += base;
		if (current[2] != 0)
			node[2] = current[2] + base;
		if (current[3] != 0)
		{
			current[3] += base;
			bmRelocateTree(base, (int *)current[3]);
		}
		current = (int *)current[0];
	} while (current != node);
}
