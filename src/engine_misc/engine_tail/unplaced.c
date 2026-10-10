/*
engine_tail/unplaced.c: engine code linked after LIBC (region LIB:engine_tail,
0x001ea360-0x001ec550), C++ without asserts, so no source file is known:
- 2D collision tests (circle and segments), the 2D kin of BMATHS;
- a free-flying debug camera controller driven by a BINPUT map (vtable 0x0027d27c,
  base 0x0027d2b8) around a BACTOR camera;
- a camera with a projection (vtable base 0x0027d2c4) and a texture projector derived
  from it (vtable 0x0027d2d0) that renders an actor into a texture and projects it (the
  game's shadows).
Methods take this as their first parameter (the original passes it in ECX).
*/

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "xtl.h"
#include "engine_misc_maths.h"
#include "engine_misc_gui.h"
#include "engine_misc_extern.h"
#include "bdisplay_types.h"
#include "bdisplay_globals.h"

#define EPS 0.001f    /* 0x00285644 */
#define TWO_PI 6.28318548f /* 0x00255930 */
#define HALF_PI 1.57079625f /* 0x00253898 */
#define STICK_SCALE 3.05250323e-05f /* 0x00266ffc */

/* ---------- 2D collision */

/* 0x001ea360  LIB:engine_tail
A circle moving along a segment {x0, y0, x1, y1} against a circle {x, y, r}. Fails when
the start is inside. */
int bmCircleSweepCircle(float *seg, float *circle, struct BHit2D *hit)
{
	float r2, dx, dy, ex, ey, len, inv, proj, disc, t, px, py;

	r2 = circle[2] * circle[2];
	if (r2 <= (seg[0] - circle[0]) * (seg[0] - circle[0]) + (seg[1] - circle[1]) * (seg[1] - circle[1])) {
		dx = circle[0] - seg[0];
		dy = circle[1] - seg[1];
		ex = seg[2] - seg[0];
		ey = seg[3] - seg[1];
		len = sqrtf(ex * ex + ey * ey);
		inv = 1.0f / len;
		proj = dx * inv * ex + inv * ey * dy;
		disc = r2 - ((dx * dx + dy * dy) - proj * proj);
		if (0.0f <= disc && (t = proj - sqrtf(disc), -EPS <= t)) {
			if (t < EPS) {
				if (hit->flags & BCOLL_T)
					hit->t = 0.0f;
				if (hit->flags & BCOLL_POINT) {
					hit->point[0] = seg[0];
					hit->point[1] = seg[1];
				}
				if (hit->flags & BCOLL_NORMAL) {
					hit->normal[0] = seg[0] - circle[0];
					hit->normal[1] = seg[1] - circle[1];
					bmVec2Divide(hit->normal, hit->normal, circle[2]);
				}
				return 1;
			}
			if (t < len) {
				if (hit->flags & BCOLL_T)
					hit->t = inv * t;
				px = inv * ex * t + seg[0];
				py = inv * ey * t + seg[1];
				if (hit->flags & BCOLL_POINT) {
					hit->point[1] = py;
					hit->point[0] = px;
				}
				if ((hit->flags & BCOLL_NORMAL) == 0)
					return 1;
				hit->normal[0] = px - circle[0];
				hit->normal[1] = py - circle[1];
				bmVec2Divide(hit->normal, hit->normal, circle[2]);
				return 1;
			}
		}
	}
	return 0;
}

/* 0x001ea530  LIB:engine_tail
Two segments {x0, y0, x1, y1}: where a crosses b (touching and overlapping included). */
int bmSegmentSegment(float *a, float *b, struct BHit2D *hit)
{
	float ax, ay, bx, by, bdy, nax, c, sa, sb, f, hiA, loA, hiB, loB, ox, oy, inv, len;

	ax = a[2] - a[0];
	ay = a[3] - a[1];
	bx = b[2];
	by = b[0];
	bdy = b[3] - b[1];
	nax = a[0] - a[2];
	c = a[1] * a[2] - a[3] * a[0];
	sa = nax * b[1] + ay * b[0] + c;
	sb = ay * b[2] + nax * b[3] + c;
	if (!(fabsf(sa) < EPS || fabsf(sb) < EPS || (sa < 0.0f) != (sb < 0.0f)))
		return 0;
	c = b[2] * b[1] - b[3] * b[0];
	sa = bdy * a[0] + (b[0] - b[2]) * a[1] + c;
	sb = bdy * a[2] + (b[0] - b[2]) * a[3] + c;
	if (!(fabsf(sa) < EPS || fabsf(sb) < EPS || (sa < 0.0f) != (sb < 0.0f)))
		return 0;
	if (EPS <= fabsf(sa) || EPS <= fabsf(sb)) {
		/* a proper crossing */
		f = fabsf(sa) / (fabsf(sa) + fabsf(sb));
		if (hit->flags & BCOLL_T)
			hit->t = f;
		if (hit->flags & BCOLL_POINT) {
			hit->point[0] = ax * f;
			hit->point[1] = f * ay;
			hit->point[0] = hit->point[0] + a[0];
			hit->point[1] = hit->point[1] + a[1];
		}
		if (hit->flags & BCOLL_NORMAL) {
			hit->normal[1] = bx - by;
			hit->normal[0] = -bdy;
		}
		return 1;
	}
	/* collinear: the overlap */
	ox = 0.0f;
	if (nax != 0.0f) {
		hiA = a[0] <= a[2] ? a[2] : a[0];
		loA = a[0] <= a[2] ? a[0] : a[2];
		hiB = b[0] <= b[2] ? b[2] : b[0];
		loB = b[0] <= b[2] ? b[0] : b[2];
		if (hiA < loB)
			return 0;
		if (hiB <= hiA) {
			if (hiB < loA)
				return 0;
			if (loA <= loB)
				return 0;
			ox = hiB - loA;
		} else {
			ox = hiA - loB;
		}
	}
	oy = 0.0f;
	if (ay != 0.0f) {
		hiA = a[1] <= a[3] ? a[3] : a[1];
		loA = a[1] <= a[3] ? a[1] : a[3];
		hiB = b[1] <= b[3] ? b[3] : b[1];
		loB = b[1] <= b[3] ? b[1] : b[3];
		if (hiA < loB)
			return 0;
		if (hiB <= hiA) {
			if (hiB < loA)
				return 0;
			if (loA <= loB)
				return 0;
			oy = hiB - loA;
		} else {
			oy = hiA - loB;
		}
	}
	inv = 1.0f / sqrtf(ay * ay + ax * ax);
	len = sqrtf(ox * ox + oy * oy) * inv;
	if (hit->flags & BCOLL_T)
		hit->t = len;
	if (hit->flags & BCOLL_POINT) {
		hit->point[0] = ax * len;
		hit->point[1] = len * ay;
		hit->point[0] = hit->point[0] + a[0];
		hit->point[1] = hit->point[1] + a[1];
	}
	if (hit->flags & BCOLL_NORMAL) {
		hit->normal[0] = -(ax * inv);
		hit->normal[1] = -(inv * ay);
		return 1;
	}
	return 1;
}

/* 0x001ea9c0  LIB:engine_tail
A circle {x, y, r} moving by delta against a segment: the segment is pushed out by r
towards the circle; a crossing near an end becomes a circle test against that end. */
int bmCircleSweepSegment(float *circle, float *seg, float *delta, struct BHit2D *hit)
{
	float dx, nx, ny, inv, ox, oy, s1, s2, as1, ag, s3, s4, a3, a4, cross, f;
	float path[4], moved[4], endCircle[3];

	dx = seg[2] - seg[0];
	path[0] = circle[0];
	path[1] = circle[1];
	nx = -(seg[3] - seg[1]);
	inv = 1.0f / sqrtf(nx * nx + dx * dx);
	nx = inv * nx;
	ny = inv * dx;
	path[2] = circle[0] + delta[0];
	path[3] = delta[1] + circle[1];
	ox = nx * circle[2];
	oy = circle[2] * ny;
	if (0.0f <= nx * delta[0] + ny * delta[1]) {
		moved[0] = seg[0] - ox;
		moved[1] = seg[1] - oy;
		moved[2] = seg[2] - ox;
		moved[3] = seg[3] - oy;
	} else {
		moved[0] = ox + seg[0];
		moved[1] = oy + seg[1];
		moved[2] = ox + seg[2];
		moved[3] = oy + seg[3];
	}
	cross = moved[2] * moved[1] - moved[0] * moved[3];
	s1 = path[0] * (moved[3] - moved[1]) + path[1] * (moved[0] - moved[2]) + cross;
	as1 = fabsf(s1);
	s2 = path[2] * (moved[3] - moved[1]) + path[3] * (moved[0] - moved[2]) + cross;
	if ((s1 < 0.0f) != (s2 < 0.0f)) {
		cross = path[2] * path[1] - path[3] * path[0];
		s3 = (path[3] - path[1]) * moved[0] + (path[0] - path[2]) * moved[1] + cross;
		a3 = fabsf(s3);
		s4 = (path[3] - path[1]) * moved[2] + (path[0] - path[2]) * moved[3] + cross;
		a4 = fabsf(s4);
		if (EPS < a3 && EPS < a4 && (s3 < 0.0f) == (s4 < 0.0f)) {
			/* passes an end of the segment: test the end as a circle */
			endCircle[2] = circle[2];
			if (a4 <= a3) {
				endCircle[0] = seg[2];
				endCircle[1] = seg[3];
			} else {
				endCircle[0] = seg[0];
				endCircle[1] = seg[1];
			}
			return bmCircleSweepCircle(path, endCircle, hit);
		}
		ag = fabsf(s2);
		if (as1 <= EPS || ag <= EPS || (s1 < 0.0f) != (s2 < 0.0f)) {
			f = as1 / (ag + as1);
			if (hit->flags & BCOLL_T)
				hit->t = f;
			if (hit->flags & BCOLL_POINT) {
				hit->point[0] = f * delta[0];
				hit->point[1] = f * delta[1];
				hit->point[0] = path[0] + hit->point[0];
				hit->point[1] = path[1] + hit->point[1];
			}
			if (hit->flags & BCOLL_NORMAL) {
				hit->normal[0] = nx;
				hit->normal[1] = ny;
			}
			return 1;
		}
	}
	return 0;
}

/* ---------- the debug camera controller */

/* 0xa8 bytes */
struct BCamController {
	const void *const *vtable;   /* 0x00 */
	unsigned char camera[0x60];  /* 0x04 a BACTOR camera (camera.cpp) */
	int field_64;                /* 0x64 */
	float pos[4];                /* 0x68 */
	float angleA;                /* 0x78 turned by UpDown */
	float angleB;                /* 0x7c turned by LeftRight */
	float angleC;                /* 0x80 */
	float speedScale;            /* 0x84 */
	struct BInputChannel *leftRight;   /* 0x88 */
	struct BInputChannel *upDown;      /* 0x8c */
	struct BInputChannel *forwardBack; /* 0x90 */
	struct BInputChannel *strafe;      /* 0x94 */
	struct BInputChannel *turbo;       /* 0x98 */
	struct BInputChannel *slow;        /* 0x9c */
	struct BInputMap *map;       /* 0xa0 */
	unsigned int flags;          /* 0xa4 bit 0: invert UpDown */
};
OF_ASSERT_OFFSET(struct BCamController, pos, 0x68);
OF_ASSERT_SIZE(struct BCamController, 0xa8);

/* a pure virtual slot (MSVC's _purecall at 0x001e0323 ends the program) */
static void em_purecall(void)
{
	em_bkPrintf("pure virtual function call\n");
	abort();
}

/* 0x0027d2b8: the controller base class */
static const void *const vtable_0027d2b8[] = {
	(const void *)FUN_001eb430,
	(const void *)em_purecall,
	(const void *)FUN_001eb420,
};
/* 0x0027d27c: the camera controller */
static const void *const vtable_0027d27c[] = {
	(const void *)FUN_001eb3e0,
	(const void *)FUN_001eb220,
	(const void *)FUN_001eb420,
};

/* 0x001ead50  LIB:engine_tail
The camera controller's constructor. */
struct BCamController *bCamController_Construct(struct BCamController *self)
{
	bCamControllerBase_Construct(self);
	self->pos[0] = 0.0f;
	self->pos[1] = 0.0f;
	self->pos[2] = 0.0f;
	self->angleB = 0.0f;
	self->angleA = 0.0f;
	self->angleC = 0.0f;
	self->strafe = NULL;
	self->slow = NULL;
	self->turbo = NULL;
	self->forwardBack = NULL;
	self->upDown = NULL;
	self->leftRight = NULL;
	self->map = NULL;
	self->vtable = vtable_0027d27c;
	self->pos[3] = 1.0f;
	self->speedScale = STICK_SCALE;
	/* tag 1: position pointer and a double; tag 4: three doubles and a word */
	em_baCreateCamera(self->camera, 0, 1, self->pos, 0.0, 4, 0.0, 0.0, 0.0, 5);
	return self;
}

/* 0x001eae00  LIB:engine_tail
The camera controller's destructor. */
void bCamController_Destruct(struct BCamController *self)
{
	self->vtable = vtable_0027d27c;
	em_baDeleteCamera(self->camera);
	if (self->map != NULL)
		em_biDeleteMap(self->map);
	bCamControllerBase_Destruct(self);
}

/* 0x001eae70  LIB:engine_tail */
void bCamController_Setup(struct BCamController *self, float x, float y, float z, float angleA, float angleB,
	float angleC, float speed, unsigned int flags)
{
	self->pos[0] = x;
	self->pos[1] = y;
	self->speedScale = speed * STICK_SCALE;
	self->pos[2] = z;
	self->angleA = angleA;
	self->angleB = angleB;
	self->angleC = angleC;
	self->flags = flags;
}

/* 0x001eaec0  LIB:engine_tail
Makes the input map (first time) and binds the controls to a pad (-1: the defaults
only). */
void bCamController_BindInput(struct BCamController *self, int pad)
{
	unsigned int p;

	if (self->map == NULL) {
		self->map = biCreateMap(6);
		if (self->map == NULL)
			return;
	}
	self->leftRight = em_biCreateChannel(self->map, "LeftRight", 3, 2, 0, 0, 0);
	self->upDown = em_biCreateChannel(self->map, "UpDown", 3, 2, 0, 0, 0);
	self->forwardBack = em_biCreateChannel(self->map, "ForwardBack", 3, 2, 0, 0, 0);
	self->strafe = em_biCreateChannel(self->map, "Strafe", 3, 2, 0, 0, 0);
	self->turbo = em_biCreateChannel(self->map, "Turbo", 1, 2, 0, 0, 0);
	self->slow = em_biCreateChannel(self->map, "Slow", 1, 2, 0, 0, 0);
	em_biBind(self->leftRight, 5, 0);
	if (pad != -1) {
		p = (unsigned int)pad << 0x10;
		em_biBind(self->leftRight, p | 0xa0000a7, 0);
		em_biBind(self->leftRight, p | 0xa0000b9, 0);
		em_biBind(self->leftRight, p | 0xa0000d3, 0);
	}
	em_biBind(self->upDown, 6, 0);
	if (pad != -1) {
		p = (unsigned int)pad << 0x10;
		em_biBind(self->upDown, p | 0xa0000a8, 0);
		em_biBind(self->upDown, p | 0xa0000ba, 0);
		em_biBind(self->upDown, p | 0xa0000d4, 0);
	}
	em_biBind(self->forwardBack, 0x9a, 0x9b);
	if (pad != -1) {
		p = (unsigned int)pad << 0x10;
		em_biBind(self->forwardBack, p | 0x1aa, 0);
		em_biBind(self->forwardBack, p | 0x1bc, 0);
		em_biBind(self->forwardBack, p | 0x1d6, 0);
	}
	em_biBind(self->strafe, 0x9d, 0x9c);
	if (pad != -1) {
		p = (unsigned int)pad << 0x10;
		em_biBind(self->strafe, p | 0xa9, 0);
		em_biBind(self->strafe, p | 0xbb, 0);
		em_biBind(self->strafe, p | 0xd5, 0);
	}
	em_biBind(self->turbo, 0x69, 0);
	if (pad != -1) {
		p = (unsigned int)pad << 0x10;
		em_biBind(self->turbo, p | 0xb2, 0);
		em_biBind(self->turbo, p | 0xcc, 0);
		em_biBind(self->turbo, p | 0xdc, 0);
	}
	em_biBind(self->slow, 0x67, 0);
	if (pad != -1) {
		p = (unsigned int)pad << 0x10;
		em_biBind(self->slow, p | 0xaf, 0);
		em_biBind(self->slow, p | 0xcb, 0);
		em_biBind(self->slow, p | 0xdb, 0);
	}
}

/* 0x001eb220  LIB:engine_tail (vtable 0x0027d27c[1])
Turns and moves the camera from the controls; speed scales the movement. */
void bCamController_Update(struct BCamController *self, float speed)
{
	float k, a, move[4], turned[4];

	if (self->upDown != NULL) {
		k = (self->flags & 1) ? -0.5f : 0.5f;
		a = (float)self->upDown->value * STICK_SCALE * k + self->angleA;
		self->angleA = a;
		if (a < 0.0f)
			self->angleA = a + TWO_PI;
		if (TWO_PI < self->angleA)
			self->angleA = self->angleA - TWO_PI;
	}
	if (self->leftRight != NULL) {
		a = (float)self->leftRight->value * STICK_SCALE * 0.5f + self->angleB;
		self->angleB = a;
		if (a < 0.0f)
			self->angleB = a + TWO_PI;
		if (TWO_PI < self->angleB)
			self->angleB = self->angleB - TWO_PI;
	}
	k = speed;
	if (self->turbo != NULL && self->turbo->value != 0)
		k = speed * 5.0f;
	else if (self->slow != NULL && self->slow->value != 0)
		k = speed * 0.2f;
	move[0] = self->strafe != NULL ? (float)self->strafe->value * self->speedScale * k : 0.0f;
	move[1] = 0.0f;
	move[2] = self->forwardBack != NULL ? (float)self->forwardBack->value * self->speedScale * k : 0.0f;
	em_bmVecRotate(turned, move, self->angleA, self->angleB);
	self->pos[0] = turned[0] + self->pos[0];
	self->pos[1] = turned[1] + self->pos[1];
	self->pos[2] = turned[2] + self->pos[2];
	em_baSetCamera(self->camera, 4, (double)self->angleA, (double)self->angleB, 0.0, 5);
}

/* 0x001eb3e0  LIB:engine_tail (vtable 0x0027d27c[0], scalar deleting destructor) */
struct BCamController *bCamController_Delete(struct BCamController *self, unsigned char flags)
{
	bCamController_Destruct(self);
	if (flags & 1)
		em_bkFree(self);
	return self;
}

/* 0x001eb400  LIB:engine_tail (the base class constructor) */
void bCamControllerBase_Construct(struct BCamController *self)
{
	self->vtable = vtable_0027d2b8;
}

/* 0x001eb410  LIB:engine_tail (the base class destructor) */
void bCamControllerBase_Destruct(struct BCamController *self)
{
	self->vtable = vtable_0027d2b8;
}

/* 0x001eb420  LIB:engine_tail (vtable 0x0027d2b8[2], 0x0027d27c[2])
Makes the controller's camera current (the previous one is not kept). */
void bCamController_Select(struct BCamController *self)
{
	em_baSelectCamera(self->camera);
}

/* 0x001eb430  LIB:engine_tail (vtable 0x0027d2b8[0], the base's scalar deleting destructor) */
struct BCamController *bCamControllerBase_Delete(struct BCamController *self, unsigned char flags)
{
	self->vtable = vtable_0027d2b8;
	if (flags & 1)
		em_bkFree(self);
	return self;
}

/* ---------- the camera and the texture projector */

struct BCamera {
	const void *const *vtable;   /* 0x000 */
	int type;                    /* 0x004 0: look at target, 1: target from the matrix */
	float pos[4];                /* 0x008 */
	float target[4];             /* 0x018 */
	float rotation[16];          /* 0x028 */
	float view[16];              /* 0x068 transpose of rotation */
	float distance;              /* 0x0a8 */
	unsigned int flags;          /* 0x0ac bit 0 draw the frustum, 1 follow an object,
	                                2-4 angles, 5 follow a target */
	unsigned char *attached;     /* 0x0b0 */
	float offset[4];             /* 0x0b4 */
	float *followPos;            /* 0x0c4 */
	float *followTarget;         /* 0x0c8 */
	float *angleX;               /* 0x0cc (or a quaternion) */
	float *angleY;               /* 0x0d0 */
	float *angleZ;               /* 0x0d4 */
	float fovX, fovY;            /* 0x0d8 */
	float cotX, cotY;            /* 0x0e0 */
	float nearClip, farClip;     /* 0x0e8 */
	float projection[16];        /* 0x0f0 */
	float scaleX, scaleY;        /* 0x130 */
	int field_138;               /* 0x138 */
	int field_13c;               /* 0x13c */
	/* the projector */
	int texture;                 /* 0x140 */
	int colour[3];               /* 0x144 */
	int stageMode[4];            /* 0x150 per texture stage */
};
OF_ASSERT_OFFSET(struct BCamera, flags, 0xac);
OF_ASSERT_OFFSET(struct BCamera, fovX, 0xd8);
OF_ASSERT_OFFSET(struct BCamera, projection, 0xf0);
OF_ASSERT_OFFSET(struct BCamera, texture, 0x140);

/* 0x0027d2c4: the camera base class; 0x0027d2d0: the projector */
static const void *const vtable_0027d2c4[] = {
	(const void *)FUN_001eb660,
	(const void *)em_purecall,
	(const void *)em_purecall,
};
static const void *const vtable_0027d2d0[] = {
	(const void *)FUN_001ec180,
	(const void *)FUN_001ec540,
	(const void *)FUN_001ec1a0,
};

/* bDisplay's camera and state (module bdisplay, bdisplay_globals.h; tentative
definitions merged with bdisplay's): the view matrix, the camera's rotation, position,
clip planes and fields of view, and the count of projected textures per stage
(0x002bee80, indexed by stage as the original does). The frustum colour parts
DAT_002bcc94/DAT_002bda3c/DAT_002bc88c/DAT_002bd214 are bdisplay's macros. */
float DAT_002be270[16];
float DAT_002be330[16];
float DAT_002be570[3];
float DAT_002be580, DAT_002be584, DAT_002be588, DAT_002be58c;
/* 0x002bee80-0x002bee8c: projected textures per texture stage. The original indexes
0x002bee80 by stage; the port has a symbol per address (bactor uses DAT_002bee84). */
int DAT_002bee80, DAT_002bee84, DAT_002bee88, DAT_002bee8c;

static int *stage_projections(int stage)
{
	switch (stage) {
	case 0:
		return &DAT_002bee80;
	case 1:
		return &DAT_002bee84;
	case 2:
		return &DAT_002bee88;
	default:
		return &DAT_002bee8c;
	}
}

/* 0x002da2c8 (.bss): the projection's template; nothing writes it, so it stays zero */
static const float s_projectionTemplate[16];

/* the original picks an SSE (aligned) or a plain matrix product by the operands'
alignment; both give the same product */
static void matrix_multiply(float *out, const float *a, const float *b)
{
	if ((((uintptr_t)out | (uintptr_t)a | (uintptr_t)b) & 0xf) == 0)
		em_bmMatMul(out, a, b);
	else
		em_bmMatMulUnaligned(out, a, b);
}

/* 0x001eb450  LIB:engine_tail (the camera base destructor) */
void bCamera_Destruct(struct BCamera *self)
{
	self->vtable = vtable_0027d2c4;
}

/* 0x001eb460  LIB:engine_tail
The texture projection: scale/offset by (scaleX, scaleY) of a perspective. */
void bCamera_BuildProjection(struct BCamera *self)
{
	float persp[16], scale[16], offset[16], tmp[16];

	memcpy(persp, s_projectionTemplate, sizeof(persp));
	persp[0] = self->cotX;
	persp[5] = self->cotY;
	persp[11] = 1.0f;
	persp[10] = self->farClip / (self->farClip - self->nearClip);
	em_bmMatScale(scale, self->scaleX, -self->scaleY, 1.0f);
	em_bmMatTranslation(offset, self->scaleX, self->scaleY, 0.0f);
	matrix_multiply(tmp, scale, persp);
	matrix_multiply(self->projection, offset, tmp);
}

/* 0x001eb590  LIB:engine_tail
The camera's constructor. type 0 looks at a target 100 ahead, type 1 uses the matrix
and a distance of 100. */
struct BCamera *bCamera_Construct(struct BCamera *self, int type, unsigned int flags)
{
	static const float identity[16] = { /* 0x002855f0 */
		1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	};
	double t;

	self->vtable = vtable_0027d2c4;
	self->pos[0] = 0.0f;
	self->pos[1] = 0.0f;
	self->pos[2] = 0.0f;
	self->pos[3] = 1.0f;
	self->type = type;
	if (type == 0) {
		self->target[0] = 0.0f;
		self->target[1] = 0.0f;
		self->target[2] = -100.0f;
		self->target[3] = 1.0f;
	} else {
		memcpy(self->rotation, identity, sizeof(identity));
		self->distance = 100.0f;
	}
	t = tan(0.7853981256484985); /* 0x00275880 (double) */
	self->fovX = HALF_PI;
	self->fovY = HALF_PI;
	self->scaleY = 0.5f;
	self->scaleX = 0.5f;
	self->nearClip = 5.0f;
	self->farClip = 5000.0f;
	self->flags = flags & 0xffff0001;
	self->cotX = (float)(1.0 / t);
	self->cotY = (float)(1.0 / t);
	bCamera_BuildProjection(self);
	self->field_13c = 0;
	return self;
}

/* 0x001eb660  LIB:engine_tail (vtable 0x0027d2c4[0], scalar deleting destructor) */
struct BCamera *bCamera_Delete(struct BCamera *self, unsigned char flags)
{
	self->vtable = vtable_0027d2c4;
	if (flags & 1)
		em_bkFree(self);
	return self;
}

/* 0x001eb680  LIB:engine_tail
Type 1: the target from the matrix and the distance. Otherwise the matrices from the
direction to the target. */
void bCamera_UpdateMatrix(struct BCamera *self)
{
	float in[4], out[4];
	double dx, dz, yaw, h, pitch;
	float dy;

	if (self->type == 1) {
		in[2] = -self->distance;
		in[0] = 0.0f;
		in[1] = 0.0f;
		in[3] = 1.0f;
		em_bmMatTransformVec(out, self->view, in);
		self->target[0] = out[0] + self->pos[0];
		self->target[1] = out[1] + self->pos[1];
		self->target[2] = out[2] + self->pos[2];
		return;
	}
	dx = (double)self->target[0] - self->pos[0];
	dy = self->target[1] - self->pos[1];
	dz = (double)self->target[2] - self->pos[2];
	if (dx == 0.0)
		yaw = dz < 0.0 ? -HALF_PI : HALF_PI;
	else
		yaw = atan2(dz, dx);
	yaw = yaw + HALF_PI;
	h = em_bmSqrt((float)(dx * dx + dz * dz));
	if (h == 0.0)
		pitch = dy < 0.0f ? -HALF_PI : HALF_PI;
	else
		pitch = atan2(dy / h, 1.0);
	em_bmMatRotXY(self->rotation, (float)-pitch, (float)yaw);
	em_bmMatTranspose(self->view, self->rotation);
}

/* 0x001eb7c0  LIB:engine_tail
Follows an object (offset NULL: half its height above it); follow != 0 keeps following
in bCamera_Update. */
void bCamera_Attach(struct BCamera *self, unsigned char *object, float *offset, int follow)
{
	unsigned char *bounds;

	self->attached = object;
	if (offset == NULL) {
		bounds = *(unsigned char **)(object + 0x130);
		self->offset[0] = 0.0f;
		self->offset[2] = 0.0f;
		self->offset[3] = 1.0f;
		self->offset[1] = (*(float *)(bounds + 0x6c) + *(float *)(bounds + 0x68)) * 0.5f;
	} else {
		self->offset[0] = offset[0];
		self->offset[1] = offset[1];
		self->offset[2] = offset[2];
		self->offset[3] = offset[3];
	}
	if (follow != 0) {
		self->flags |= 2;
		return;
	}
	self->flags &= ~2u;
	self->pos[0] = *(float *)(object + 0xc0) + self->offset[0];
	self->pos[1] = self->offset[1] + *(float *)(object + 0xc4);
	self->pos[2] = self->offset[2] + *(float *)(object + 0xc8);
	bCamera_UpdateMatrix(self);
}

/* 0x001eb890  LIB:engine_tail
Sets the angles (and the distance unless 0). */
void bCamera_SetAngles(struct BCamera *self, float x, float y, float z, float distance)
{
	self->flags &= ~0x1cu;
	em_bmMatRotXYZ(self->rotation, -x, -y, -z);
	em_bmMatTranspose(self->view, self->rotation);
	if (distance != 0.0f)
		self->distance = distance;
	bCamera_UpdateMatrix(self);
}

/* 0x001eb900  LIB:engine_tail
Per frame: follows its object/position, target or angles. */
void bCamera_Update(struct BCamera *self)
{
	float *p;
	unsigned char *object;
	float matrix[16];

	if (self->flags & 2) {
		object = self->attached;
		if (object == NULL) {
			p = self->followPos;
			self->pos[0] = p[0] + self->offset[0];
			self->pos[1] = p[1] + self->offset[1];
			self->pos[2] = p[2] + self->offset[2];
		} else {
			self->pos[0] = *(float *)(object + 0xc0) + self->offset[0];
			self->pos[1] = *(float *)(object + 0xc4) + self->offset[1];
			self->pos[2] = *(float *)(object + 200) + self->offset[2];
			self->pos[3] = 1.0f;
		}
	}
	if (self->flags & 0x20) {
		p = self->followTarget;
		self->target[0] = p[0];
		self->target[1] = p[1];
		self->target[2] = p[2];
		self->target[3] = p[3];
		bCamera_UpdateMatrix(self);
		return;
	}
	if (self->flags & 4) {
		em_bmMatRotXYZ(self->rotation, -*self->angleX, -*self->angleY, -*self->angleZ);
		em_bmMatTranspose(self->view, self->rotation);
		bCamera_UpdateMatrix(self);
		return;
	}
	if (self->flags & 8) {
		em_bmMatFromQuat(self->view, self->angleX);
		em_bmMatTranspose(self->rotation, self->view);
		bCamera_UpdateMatrix(self);
		return;
	}
	if (self->flags & 0x10) {
		memcpy(self->view, self->angleX, 64);
		memcpy(matrix, self->angleX, 64);
		em_bmMatTranspose(self->rotation, matrix);
	}
	bCamera_UpdateMatrix(self);
}

/* 0x001eba70  LIB:engine_tail
Sets the fields of view (radians) and clip planes; 0 keeps a value. */
void bCamera_SetFov(struct BCamera *self, float fovX, float fovY, float nearClip, float farClip)
{
	if (fovX != 0.0f) {
		self->fovX = fovX;
		self->cotX = (float)(1.0 / tan((double)fovX * 0.5f));
	}
	if (fovY != 0.0f) {
		self->fovY = fovY;
		self->cotY = (float)(1.0 / tan((double)fovY * 0.5f));
	}
	if (nearClip != 0.0f)
		self->nearClip = nearClip;
	if (farClip != 0.0f)
		self->farClip = farClip;
	bCamera_BuildProjection(self);
}

/* a vertex of the frustum fan: 36 bytes */
struct FrustumVertex {
	float pos[3];
	float normal[3];
	unsigned int colour;
	float uv[2];
};

/* 0x001ebb20  LIB:engine_tail
Draws the projector's frustum into the stencil buffer (bit 0x80), seen with matrix. */
void bCamera_DrawFrustum(struct BCamera *self, float *matrix)
{
	struct FrustumVertex v[6];
	float inverse[16];
	float a, s1, c1, s2;
	unsigned int colour;

	em_bmMatInvert(inverse, matrix);
	em_bdSetWorldMatrix(inverse);
	D3DDevice_SetTexture(0, NULL);
	D3DDevice_SetRenderState(D3DRS_COLORWRITEENABLE, 0);
	D3DDevice_SetRenderState(D3DRS_STENCILFUNC, 0x207);
	D3DDevice_SetRenderState(D3DRS_STENCILPASS, 0x1e01);
	D3DDevice_SetRenderState(D3DRS_STENCILWRITEMASK, 0x80);
	D3DDevice_SetRenderState(D3DRS_STENCILREF, 0x80);
	D3DDevice_SetRenderState_StencilEnable(1);
	D3DDevice_SetRenderState(D3DRS_ZWRITEENABLE, 0);
	D3DDevice_SetRenderState(D3DRS_ALPHATESTENABLE, 0);
	em_bdSetVertexMode(4, 0, 0);
	memset(v, 0, sizeof(v)); /* the original leaves normals and uvs uninitialised */
	a = self->fovX * 0.5f;
	s1 = sinf(a) * self->farClip;
	c1 = cosf(a) * self->farClip;
	s2 = sinf(self->fovY * 0.5f) * self->farClip;
	colour = DAT_002bcc94 | DAT_002bda3c | DAT_002bc88c | DAT_002bd214;
	v[0].pos[0] = 0.0f;
	v[0].pos[1] = 0.0f;
	v[0].pos[2] = 0.0f;
	v[0].colour = colour;
	v[1].pos[0] = -s1;
	v[1].pos[1] = s2;
	v[1].pos[2] = c1;
	v[1].colour = colour;
	v[2].pos[0] = s1;
	v[2].pos[1] = s2;
	v[2].pos[2] = c1;
	v[2].colour = colour;
	v[3].pos[0] = s1;
	v[3].pos[1] = -s2;
	v[3].pos[2] = c1;
	v[3].colour = colour;
	v[4].pos[0] = -s1;
	v[4].pos[1] = -s2;
	v[4].pos[2] = c1;
	v[4].colour = colour;
	v[5] = v[1];
	em_bdDrawPrimitive(5, v, 6);
	em_bdResetVertexMode(4);
	D3DDevice_SetRenderState_StencilEnable(0);
	D3DDevice_SetRenderState(D3DRS_ZWRITEENABLE, 1);
	D3DDevice_SetRenderState(D3DRS_ALPHATESTENABLE, 1);
	D3DDevice_SetRenderState(D3DRS_COLORWRITEENABLE, 0x1010101);
	D3DDevice_SetRenderState(D3DRS_STENCILFUNC, 0x202);
	D3DDevice_SetRenderState_StencilEnable(1);
	D3DDevice_SetRenderState(D3DRS_STENCILMASK, 0x80);
}

/* 0x001ebdb0  LIB:engine_tail
The projector's constructor: a camera and a w x h render texture (w rounded up to 32). */
struct BCamera *bProjector_Construct(struct BCamera *self, int type, int w, int h, unsigned int flags)
{
	int width;

	bCamera_Construct(self, type, flags);
	width = (w + 0x1f) & ~0x1f;
	self->scaleX = (float)width * 0.5f;
	self->colour[2] = 0x40;
	self->colour[1] = 0x40;
	self->colour[0] = 0x40;
	self->scaleY = (float)h * 0.5f;
	self->vtable = vtable_0027d2d0;
	self->texture = em_bdCreateRenderTexture(width, h, 0x10, 0, (flags & 0x10000 | 0x2000) >> 0xd);
	bCamera_BuildProjection(self);
	return self;
}

/* 0x001ebe80  LIB:engine_tail (the projector's destructor) */
void bProjector_Destruct(struct BCamera *self)
{
	self->vtable = vtable_0027d2d0;
	if (self->texture != 0)
		em_bdReleaseTexture(self->texture);
	bCamera_Destruct(self);
}

/* 0x001ebee0  LIB:engine_tail
Renders an actor (NULL: the attached one) into the projector's texture from the
projector (flags 1: clear to white; 4: back to the screen after). */
void bProjector_Render(struct BCamera *self, unsigned char *actor, int arg, unsigned char flags)
{
	int prev, saved140[3], saved150[3], i;
	float savedPos[3], savedMatrix[16];
	float saved580[4];

	if (actor == NULL)
		actor = self->attached;
	if (flags & 1)
		em_bdSetRenderTarget(self->texture, 0xff, 0xff, 0xff, 0xff, 1.0f, 1);
	else
		em_bdSetRenderTarget(self->texture, 0, 0, 0, 0, 1.0f, 0);
	prev = (int)em_baSelectCamera(NULL);
	if (prev == 0) {
		savedPos[2] = DAT_002be578;
		savedPos[0] = DAT_002be570[0];
		savedPos[1] = DAT_002be574;
		memcpy(savedMatrix, DAT_002be330, 64);
		saved580[2] = DAT_002be588;
		saved580[3] = DAT_002be58c;
		saved580[0] = DAT_002be580;
		saved580[1] = DAT_002be584;
	}
	if (self->type == 0) {
		em_bdLookAt(self->target, self->pos, 0.0f);
	} else {
		em_bdSetCameraPosition(self->target[0], self->target[1], self->target[2]);
		memcpy(DAT_002be330, self->rotation, 64);
	}
	em_bdSetFov(self->fovX, self->fovY);
	em_bdSetClipPlanes(self->nearClip, self->farClip);
	em_bdApplyCamera();
	memcpy(saved140, actor + 0x140, 12);
	memcpy(saved150, actor + 0x150, 12);
	em_baSetRenderPass(actor, 0, 8, self->colour);
	for (i = 1; i < 3; i++)
		em_baSetRenderPass(actor, i, 0, NULL);
	em_baDrawActor(actor, arg);
	memcpy(actor + 0x140, saved140, 12);
	memcpy(actor + 0x150, saved150, 12);
	if (flags & 4)
		em_bdSetRenderTarget(0, 0, 0, 0, 0, 1.0f, 0);
	if (prev != 0) {
		em_baSelectCamera((void *)prev);
		em_bdApplyCamera();
		return;
	}
	DAT_002be574 = savedPos[1];
	DAT_002be570[0] = savedPos[0];
	DAT_002be578 = savedPos[2];
	memcpy(DAT_002be330, savedMatrix, 64);
	DAT_002be588 = saved580[2];
	DAT_002be58c = saved580[3];
	DAT_002be580 = saved580[0];
	DAT_002be584 = saved580[1];
	em_bdApplyCamera();
}

/* 0x001ec160  LIB:engine_tail */
void bProjector_SetColour(struct BCamera *self, int colour)
{
	self->colour[2] = colour;
	self->colour[1] = colour;
	self->colour[0] = colour;
}

/* 0x001ec180  LIB:engine_tail (vtable 0x0027d2d0[0], scalar deleting destructor) */
struct BCamera *bProjector_Delete(struct BCamera *self, unsigned char flags)
{
	bProjector_Destruct(self);
	if (flags & 1)
		em_bkFree(self);
	return self;
}

/* 0x001ec1a0  LIB:engine_tail (vtable 0x0027d2d0[2])
Takes the projected texture off a texture stage. */
void bProjector_Unapply(struct BCamera *self, int stage)
{
	int mode;

	mode = self->stageMode[stage];
	if (mode > -1 && mode < 2 && stage == 0) {
		em_bdPopState(6);
		em_bdSetTextureStage(10, 0, 0);
		em_bdResetVertexMode(6);
		em_bdResetVertexMode(7);
	}
	em_bdSetTextureStage(0xd, stage, 1);
	em_bdPopState(2);
	em_bdSetStageTexture(stage, 0);
	if (stage != 0)
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_COLOROP, 1);
	D3DDevice_SetTextureState_Deferred(stage, D3DTSS_ADDRESSU, 1);
	D3DDevice_SetTextureState_Deferred(stage, D3DTSS_ADDRESSV, 1);
	D3DDevice_SetTextureState_TexCoordIndex(stage, stage);
	D3DDevice_SetTextureState_Deferred(stage, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
	if (self->flags & 1)
		D3DDevice_SetRenderState_StencilEnable(0);
	(*stage_projections(stage))--;
}

/* 0x001ec260  LIB:engine_tail
An empty method (the original is "ret 4": this in ECX and one argument). */
void FUN_001ec260(void)
{
}

/* 0x001ec270  LIB:engine_tail
Projects the texture on a texture stage from the projector, through the current view
(mode 0: modulate, 1: modulate on stage 0, 2: blend on later stages). */
void bProjector_Apply(struct BCamera *self, int stage, int mode)
{
	float viewInverse[16], translate[16], m1[16], m2[16], m3[16];

	em_bmMatInvert(viewInverse, DAT_002be270);
	em_bmMatTranslation(translate, -self->target[0], -self->target[1], -self->target[2]);
	matrix_multiply(m1, self->rotation, translate);
	matrix_multiply(m2, m1, viewInverse);
	matrix_multiply(m3, self->projection, m2);
	if (self->flags & 1)
		bCamera_DrawFrustum(self, m1);
	em_bdSetTexture(self->texture, stage);
	D3DDevice_SetTextureState_Deferred(stage, D3DTSS_ADDRESSU, 4);
	D3DDevice_SetTextureState_Deferred(stage, D3DTSS_ADDRESSV, 4);
	D3DDevice_SetTextureState_BorderColor(stage, 0xffffffff);
	self->stageMode[stage] = mode;
	if (mode == 0 || (mode == 1 && stage == 0)) {
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_COLOROP, 2);
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_COLORARG1, 2);
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_ALPHAOP, 2);
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_ALPHAARG1, 2);
		D3DDevice_SetRenderState(D3DRS_SRCBLEND, 0x306);
		D3DDevice_SetRenderState(D3DRS_DESTBLEND, 0);
		D3DDevice_SetRenderState(D3DRS_BLENDOP, 0x8006);
		em_bdSetVertexMode(7, 2, 0);
		em_bdSetVertexMode(6, 1, 0);
		em_bdPushState(6);
	} else if (mode == 1 || mode == 2) {
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_COLOROP, 4);
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_COLORARG1, 1);
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_COLORARG2, 2);
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_ALPHAOP, 2);
		D3DDevice_SetTextureState_Deferred(stage, D3DTSS_ALPHAARG1, 1);
	}
	D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)(stage + 2), (const D3DMATRIX *)m3);
	D3DDevice_SetTextureState_Deferred(stage, D3DTSS_TEXTURETRANSFORMFLAGS, 0x103);
	D3DDevice_SetTextureState_TexCoordIndex(stage, 0x20000);
	em_bdSetTextureStage(0xd, stage, 0);
	em_bdPushState(2);
	(*stage_projections(stage))++;
}

/* 0x001ec540  LIB:engine_tail (vtable 0x0027d2d0[1]) */
void bProjector_ApplyDefault(struct BCamera *self, int stage)
{
	bProjector_Apply(self, stage, 0);
}
