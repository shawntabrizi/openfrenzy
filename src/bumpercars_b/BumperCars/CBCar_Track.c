/*
BumperCars/CBCar_Track.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Track.cpp

Skid marks: a ring buffer of track points, each a left and a right edge around the wheel
position, drawn as textured quads (with a joint triangle) on the ground. 0x000a6260 is tagged
with CBCar_Track.cpp; the methods after it are gap functions of the same class (file
inferred). TankedUp uses the class too.
*/

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"

#define BCB_SRC_TRACK "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Track.cpp"

/* a track point (0x3c bytes) */
struct BCarTrackPoint {
	int joined;               /* 0x00 continues the previous point */
	float time;               /* 0x04 */
	float alpha;              /* 0x08 */
	float pos[4];             /* 0x0c */
	float left[4];            /* 0x1c */
	float right[4];           /* 0x2c */
};

struct BCarTrack {
	int maxPoints;                  /* 0x00 */
	int first;                      /* 0x04 */
	int last;                       /* 0x08 */
	int disabled;                   /* 0x0c */
	int joined;                     /* 0x10 the next point continues the mark */
	float width;                    /* 0x14 */
	float time;                     /* 0x18 */
	float spacing;                  /* 0x1c frames */
	struct BCarTrackPoint *points;  /* 0x20 */
	struct bdTexture *texture;      /* 0x24 the quads */
	struct bdTexture *texture2;     /* 0x28 the joint triangles */
};
OF_ASSERT_SIZE(struct BCarTrack, 0x2c);

/* a vertex of the draw (FVF 0x152: position, normal, diffuse, one texture coordinate) */
struct BCarTrackVertex {
	float pos[3];
	float normal[3];
	unsigned int diffuse;
	float u;
	float v;
};

/* 0x002f26c8: the quad, 0x002f2758: the joint triangle (the normals stay 0) */
#define g_TrackQuad ((struct BCarTrackVertex *)BCB_ADDR(0x002f26c8))
#define g_TrackJoint ((struct BCarTrackVertex *)BCB_ADDR(0x002f2758))

/* bdisplay's colour tables (alpha, red, green, blue already shifted into place) */
extern unsigned int DAT_002bc490[];
extern unsigned int DAT_002bc898[];
extern unsigned int DAT_002bce18[];
extern unsigned int DAT_002bd640[];

/* |a - b|^2 (0x00051540; bkernel names it bmNothing2 and makes it empty, but the original
returns the squared distance on the x87 stack) */
static float bcb_dist_sq(const float *a, const float *b)
{
	float x = a[0] - b[0];
	float y = a[1] - b[1];
	float z = a[2] - b[2];

	return z * z + x * x + y * y;
}

static void bcb_copy3(float *out, const float *in)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/* 0x000a6260  BumperCars/CBCar_Track.cpp
Constructor: `maxPoints` points (at least 2, else the track is disabled), half `width`
wide, a point every `spacing` seconds. */
struct BCarTrack *BCarTrack_Construct(struct BCarTrack *self, int maxPoints, float width,
	float spacing, struct bdTexture *texture, struct bdTexture *texture2)
{
	self->spacing = spacing * 60.0f;                           /* 0x00254e98 */
	self->width = width;
	self->disabled = 0;
	self->first = 0;
	self->last = 0;
	self->maxPoints = maxPoints;
	self->time = 0.0f;
	self->joined = 0;
	self->texture = texture;
	self->texture2 = texture2;
	if (maxPoints < 2)
	{
		bkPrintf("%s(%d) : Constructor - invalid length [%d]. Track system is disabled\n",
			BCB_SRC_TRACK, 0x1f, maxPoints);
		self->disabled = 1;
		return self;
	}
	self->points = (struct BCarTrackPoint *)bkHeapAlloc((uint)(maxPoints * 0x3c), BCB_SRC_TRACK,
		0x25, 0x2001);
	if (self->points == NULL)
	{
		bkPrintf("%s(%d) : Constructor - no memory available for tracks. Track system is disabled\n",
			BCB_SRC_TRACK, 0x29);
		self->disabled = 1;
	}
	return self;
}

/* 0x000a6310  (gap; file inferred) */
void BCarTrack_Destruct(struct BCarTrack *self)
{
	if (self->disabled == 0)
	{
		BCB_DELETE(self->points);
		self->points = NULL;
	}
}

/* 0x000a6330  (gap; file inferred) */
void BCarTrack_Update(struct BCarTrack *self, float dt)
{
	if (self->disabled == 0)
		self->time = dt + self->time;
}

/* 0x000a6350  (gap; file inferred)
Ends the mark: the next point starts a new one. */
void BCarTrack_End(struct BCarTrack *self)
{
	if (self->disabled == 0)
		self->joined = 0;
}

/* 0x000a6360  (gap; file inferred)
Adds a point at `pos` on the ground with normal `normal`, the car moving along `dir`: the
edges are `width` either side, across the direction on the ground. */
void BCarTrack_AddPoint(struct BCarTrack *self, float *pos, float *normal, float *dir, float alpha)
{
	struct BCarTrackPoint *point;
	float across[4];
	float ahead[3];
	float up[3];
	float width;
	int index;

	if (self->disabled != 0)
		return;
	index = (self->last + 1) % self->maxPoints;
	self->last = index;
	if (self->first == index)
		self->first = (self->first + 1) % self->maxPoints;
	point = &self->points[index];
	point->pos[0] = pos[0];
	point->pos[1] = pos[1];
	point->pos[2] = pos[2];
	point->pos[3] = pos[3];
	point->joined = self->joined;
	point->alpha = alpha;
	point->time = self->time;
	self->joined = 1;
	ahead[0] = pos[0] + dir[0];
	ahead[1] = dir[1] + pos[1];
	ahead[2] = dir[2] + pos[2];
	up[0] = pos[0] + normal[0];
	up[1] = normal[1] + pos[1];
	up[2] = normal[2] + pos[2];
	bmTriangleNormal(across, pos, ahead, up);
	bmVectorNormalise(across, across);
	width = self->width;
	point->left[0] = across[0] * width + point->pos[0];
	point->left[1] = across[1] * width + point->pos[1];
	point->left[2] = across[2] * width + point->pos[2];
	point->right[0] = point->pos[0] - across[0] * width;
	point->right[1] = point->pos[1] - across[1] * width;
	point->right[2] = point->pos[2] - across[2] * width;
}

/* 0x000a64c0  (gap; file inferred)
Draws the marks: for every joined pair of points a joint triangle (texture2) where the
inner edge turns, and a quad (texture) from the previous edges to this point's. */
void BCarTrack_Draw(struct BCarTrack *self)
{
	struct BCarTrackPoint *point;
	unsigned int colour;
	int index;
	int first;
	int c;
	float prevLeft[4];
	float prevRight[4];
	float a[4];
	float b[4];
	float v;

	if (self->disabled != 0)
		return;
	bdPushRenderState(5, 0, 0);
	bdSetWorldIdentity();
	g_TrackJoint[0].u = 0.0f;
	g_TrackJoint[0].v = 0.0f;
	g_TrackJoint[1].u = 1.0f;
	g_TrackJoint[1].v = 0.0f;
	g_TrackJoint[2].u = 1.0f;
	g_TrackJoint[2].v = 1.0f;
	g_TrackQuad[0].u = 0.0f;
	g_TrackQuad[0].v = 0.0f;
	g_TrackQuad[1].u = 1.0f;
	g_TrackQuad[1].v = 0.0f;
	index = self->first;
	first = 1;
	if (index != self->last)
	{
		do
		{
			index = (index + 1) % self->maxPoints;
			point = &self->points[index];
			if (point->joined != 0 && !first)
			{
				c = BCB_FTOL(1.0f * 127.0f);                    /* 0x00253850, 0x00255348 */
				colour = DAT_002bc898[c] | DAT_002bd640[c] | DAT_002bc490[c] | DAT_002bce18[c];
				g_TrackQuad[0].diffuse = colour;
				g_TrackQuad[1].diffuse = colour;
				g_TrackQuad[2].diffuse = colour;
				g_TrackQuad[3].diffuse = colour;
				g_TrackJoint[0].diffuse = colour;
				g_TrackJoint[1].diffuse = colour;
				g_TrackJoint[2].diffuse = colour;
				if (bcb_dist_sq(prevLeft, point->left) <= bcb_dist_sq(prevRight, point->right))
				{
					/* the left edge moved less: keep it, square the right */
					a[0] = point->left[0];
					a[1] = point->left[1];
					a[2] = point->left[2];
					a[3] = point->left[3];
					b[0] = (prevRight[0] + a[0]) - prevLeft[0];
					b[1] = (prevRight[1] + a[1]) - prevLeft[1];
					b[2] = (prevRight[2] + a[2]) - prevLeft[2];
					if (point->joined != 0)
					{
						bcb_copy3(g_TrackJoint[2].pos, point->right);
						bcb_copy3(g_TrackJoint[0].pos, a);
						bcb_copy3(g_TrackJoint[1].pos, b);
					}
				}
				else
				{
					b[0] = point->right[0];
					b[1] = point->right[1];
					b[2] = point->right[2];
					a[0] = (prevLeft[0] + b[0]) - prevRight[0];
					a[1] = (prevLeft[1] + b[1]) - prevRight[1];
					a[2] = (prevLeft[2] + b[2]) - prevRight[2];
					if (point->joined != 0)
					{
						bcb_copy3(g_TrackJoint[2].pos, point->left);
						bcb_copy3(g_TrackJoint[0].pos, b);
						bcb_copy3(g_TrackJoint[1].pos, a);
					}
				}
				bdSetTexture(0, self->texture2);
				bdDrawPrimitiveFVF(4, g_TrackJoint, 3, 0x152);
				v = bmVectorDistance(prevLeft, a) / self->width;
				if (1.0f < v)
					v = 1.0f;
				g_TrackQuad[2].v = v;
				g_TrackQuad[3].v = v;
				bcb_copy3(g_TrackQuad[0].pos, prevLeft);
				bcb_copy3(g_TrackQuad[1].pos, prevRight);
				bcb_copy3(g_TrackQuad[2].pos, a);
				bcb_copy3(g_TrackQuad[3].pos, b);
				g_TrackQuad[2].u = 0.0f;
				g_TrackQuad[3].u = 1.0f;
				bdSetTexture(0, self->texture);
				bdDrawPrimitiveFVF(4, g_TrackQuad, 4, 0x152);
			}
			prevRight[0] = point->right[0];
			prevRight[1] = point->right[1];
			prevRight[2] = point->right[2];
			prevRight[3] = point->right[3];
			prevLeft[0] = point->left[0];
			prevLeft[1] = point->left[1];
			prevLeft[2] = point->left[2];
			prevLeft[3] = point->left[3];
			first = 0;
		} while (index != self->last);
	}
	bdPopRenderState(5);
}
