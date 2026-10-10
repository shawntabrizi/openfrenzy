/*
TankedUp/Decal.c  <-  C:\BlitzParty\Game\Src\TankedUp\Decal.cpp

The track decals of the tanks: a ring of track points, each a left and a right edge around
the tank's position, drawn on the ground as textured quads (and a joint triangle where the
track turns), fading with age. The layout is bumpercars_b's BCarTrack (CBCar_Track.cpp);
TankedUp uses BCarTrack_Update/_End/_Destruct (0x000a6330, 0x000a6350, 0x000a6310) from
there and has its own constructor, AddPoint and Draw here.

  +0x00 points      +0x04 first     +0x08 last      +0x0c disabled   +0x10 joined
  +0x14 width       +0x18 time      +0x1c life (frames)              +0x20 the points
  +0x24 texture     +0x28 texture2 (not used by the draw)            +0x2c the last direction
A point (0x3c bytes): +0x00 joined, +0x04 time, +0x08 alpha, +0x0c pos[4], +0x1c left[4],
+0x2c right[4].
*/

#include "tankedup_b.h"
#include "tankedup_b_data.h"
#include "tankedup_b_ext.h"

#define DC_MAX 0x00
#define DC_FIRST 0x04
#define DC_LAST 0x08
#define DC_DISABLED 0x0c
#define DC_JOINED 0x10
#define DC_WIDTH 0x14
#define DC_TIME 0x18
#define DC_LIFE 0x1c
#define DC_POINTS 0x20
#define DC_TEXTURE 0x24
#define DC_TEXTURE2 0x28
#define DC_LAST_DIR 0x2c

#define DC_POINT(self, i) ((char *)TU_PTR(self, DC_POINTS) + (i) * 0x3c)

/* 0x001de090  TankedUp/Decal.cpp
Decal constructor: `maxPoints` points (at least 2, else the decal is disabled), edges
`width` from the centre, points that live `life` seconds. */
void *TankDecal_Ctor(void *self, int maxPoints, float width, float life, void *texture, void *texture2)
{
	TU_F32(self, DC_LIFE) = life * 60.0f; /* 0x00254e98 */
	TU_I32(self, DC_MAX) = maxPoints;
	TU_I32(self, DC_DISABLED) = 0;
	TU_I32(self, DC_FIRST) = 0;
	TU_I32(self, DC_LAST) = 0;
	TU_F32(self, DC_TIME) = 0.0f;
	TU_I32(self, DC_JOINED) = 0;
	TU_F32(self, DC_LAST_DIR) = 0.0f;
	TU_F32(self, DC_LAST_DIR + 4) = 0.0f;
	TU_F32(self, DC_LAST_DIR + 8) = 0.0f;
	TU_F32(self, DC_LAST_DIR + 12) = 1.0f;
	TU_PTR(self, DC_TEXTURE2) = texture2;
	TU_F32(self, DC_WIDTH) = width;
	TU_PTR(self, DC_TEXTURE) = texture;
	if (TU_I32(self, DC_MAX) < 2) {
		bkPrintf("Decal::Constructor - invalid length [%d]. Track system is disabled\n", TU_I32(self, DC_MAX));
		TU_I32(self, DC_DISABLED) = 1;
		return self;
	}
	TU_PTR(self, DC_POINTS) = bkHeapAlloc((uint)(TU_I32(self, DC_MAX) * 0x3c), TU_FILE_DECAL, 0x1f, 0x2001);
	if (TU_PTR(self, DC_POINTS) == NULL) {
		bkPrintf("Decal::Constructor - no memory available for tracks. Track system is disabled\n");
		TU_I32(self, DC_DISABLED) = 1;
	}
	return self;
}

/* 0x001de140  TankedUp (gap; Decal.cpp inferred)
Adds a point at `pos` on ground with normal `normal`, the tank heading along `dir`, when it
is the first, 100 or more from the last point, or the direction changed (squared change
0.04 or more). */
void TankDecal_AddPoint(void *self, float *pos, float *normal, float *dir, float alpha)
{
	char *point;
	float ahead[3];
	float up[3];
	float across[4];
	float width;
	int index;

	if (TU_I32(self, DC_DISABLED) != 0)
		return;
	if (TU_I32(self, DC_FIRST) != TU_I32(self, DC_LAST) &&
		tu_dist_sq(pos, TU_FP(DC_POINT(self, TU_I32(self, DC_LAST)), 0xc)) < 10000.0f && /* 0x002657a0 */
		tu_dist_sq(dir, TU_FP(self, DC_LAST_DIR)) < 0.0400000028f) /* 0x0027c4ec */
		return;
	index = (TU_I32(self, DC_LAST) + 1) % TU_I32(self, DC_MAX);
	TU_I32(self, DC_LAST) = index;
	if (TU_I32(self, DC_FIRST) == index)
		TU_I32(self, DC_FIRST) = (TU_I32(self, DC_FIRST) + 1) % TU_I32(self, DC_MAX);
	point = DC_POINT(self, index);
	memcpy(point + 0xc, pos, 16);
	TU_I32(point, 0) = TU_I32(self, DC_JOINED);
	TU_F32(point, 8) = alpha;
	TU_F32(point, 4) = TU_F32(self, DC_TIME);
	TU_I32(self, DC_JOINED) = 1;
	ahead[0] = pos[0] + dir[0];
	ahead[1] = dir[1] + pos[1];
	ahead[2] = pos[2] + dir[2];
	up[0] = pos[0] + normal[0];
	up[1] = normal[1] + pos[1];
	up[2] = normal[2] + pos[2];
	bmTriangleNormal(across, pos, ahead, up);
	bmVectorNormalise(across, across);
	width = TU_F32(self, DC_WIDTH);
	TU_F32(point, 0x1c) = across[0] * width + TU_F32(point, 0xc);
	TU_F32(point, 0x20) = across[1] * width + TU_F32(point, 0x10);
	TU_F32(point, 0x24) = across[2] * width + TU_F32(point, 0x14);
	TU_F32(point, 0x2c) = TU_F32(point, 0xc) - across[0] * width;
	TU_F32(point, 0x30) = TU_F32(point, 0x10) - across[1] * width;
	TU_F32(point, 0x34) = TU_F32(point, 0x14) - across[2] * width;
	memcpy(TU_ADDR(self, DC_LAST_DIR), dir, 16);
}

static void tu_copy3(float *out, const float *in)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/* 0x001de300  TankedUp (gap; Decal.cpp inferred)
Draws the track: for every joined pair of points a joint triangle and a quad from the
previous edges to this point's, faded by the point's age (points older than the life are
dropped from the tail). */
void TankDecal_Draw(void *self)
{
	struct TankVertex *joint = DAT_00315a28;
	struct TankVertex *quad = DAT_00315998;
	char *point;
	float prevLeft[4];
	float prevRight[4];
	float a[4];
	float b[4];
	float age;
	float alpha;
	float v;
	unsigned int colour;
	int index;
	int first;
	int c;

	if (TU_I32(self, DC_DISABLED) != 0)
		return;
	bdPushRenderState(5, 0, 0);
	bdSetWorldMatrix(DAT_002855f0);
	joint[0].uv[0] = 0.0f;
	joint[0].uv[1] = 0.0f;
	joint[1].uv[0] = 1.0f;
	joint[1].uv[1] = 0.0f;
	joint[2].uv[0] = 1.0f;
	joint[2].uv[1] = 1.0f;
	quad[0].uv[0] = 0.0f;
	quad[0].uv[1] = 0.0f;
	quad[1].uv[0] = 1.0f;
	quad[1].uv[1] = 0.0f;
	index = TU_I32(self, DC_FIRST);
	first = 1;
	bdSetTexture(0, (struct bdTexture *)TU_PTR(self, DC_TEXTURE));
	if (index != TU_I32(self, DC_LAST)) {
		do {
			index = (index + 1) % TU_I32(self, DC_MAX);
			point = DC_POINT(self, index);
			if (TU_I32(point, 0) != 0 && !first) {
				age = TU_F32(self, DC_TIME) - TU_F32(point, 4);
				if (age > TU_F32(self, DC_LIFE))
					TU_I32(self, DC_FIRST) = index;
				alpha = (1.0f - age / TU_F32(self, DC_LIFE)) * TU_F32(point, 8);
				if (alpha <= 0.0f)
					alpha = 0.0f;
				else if (alpha > 1.0f)
					alpha = 1.0f;
				c = TU_FTOL(alpha * 127.0f); /* 0x00255348 */
				colour = DAT_002bc898[c] | DAT_002bd640[c] | DAT_002bc490[c] | DAT_002bce18[c];
				joint[1].colour = colour;
				joint[2].colour = colour;
				quad[0].colour = colour;
				quad[1].colour = colour;
				quad[2].colour = colour;
				quad[3].colour = colour;
				joint[0].colour = colour;
				if (tu_dist_sq(prevRight, TU_FP(point, 0x2c)) < tu_dist_sq(prevLeft, TU_FP(point, 0x1c))) {
					/* the right edge moved less: keep it, square the left */
					memcpy(b, TU_FP(point, 0x2c), 12);
					a[0] = (prevLeft[0] + b[0]) - prevRight[0];
					a[1] = (prevLeft[1] + b[1]) - prevRight[1];
					a[2] = (prevLeft[2] + b[2]) - prevRight[2];
					tu_copy3(joint[0].pos, b);
					tu_copy3(joint[1].pos, a);
					tu_copy3(joint[2].pos, TU_FP(point, 0x1c));
				} else {
					memcpy(a, TU_FP(point, 0x1c), 16);
					b[0] = (prevRight[0] + a[0]) - prevLeft[0];
					b[1] = (prevRight[1] + a[1]) - prevLeft[1];
					b[2] = (prevRight[2] + a[2]) - prevLeft[2];
					tu_copy3(joint[0].pos, a);
					tu_copy3(joint[1].pos, b);
					tu_copy3(joint[2].pos, TU_FP(point, 0x2c));
				}
				bdDrawPrimitiveFVF(4, joint, 3, 0x152);
				v = bmVectorDistance(prevRight, b) / TU_F32(self, DC_WIDTH);
				if (v > 1.0f)
					v = 1.0f;
				quad[2].uv[1] = v;
				quad[3].uv[1] = v;
				tu_copy3(quad[0].pos, prevLeft);
				tu_copy3(quad[1].pos, prevRight);
				tu_copy3(quad[2].pos, a);
				tu_copy3(quad[3].pos, b);
				quad[2].uv[0] = 0.0f;
				quad[3].uv[0] = 1.0f;
				bdDrawPrimitiveFVF(4, quad, 4, 0x152);
			}
			memcpy(prevRight, TU_FP(point, 0x2c), 16);
			memcpy(prevLeft, TU_FP(point, 0x1c), 16);
			first = 0;
		} while (index != TU_I32(self, DC_LAST));
	}
	bdPopRenderState(5);
}
