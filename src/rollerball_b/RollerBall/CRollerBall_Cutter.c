/*
RollerBall/CRollerBall_Cutter.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerBall_Cutter.cpp (heap tags of 0x001975b0): the
cutter of the "drop" variation (vtable 0x00274ad0): a beam sweeps once around a circle of
radius +0x48 about +0x54 over +0x40 seconds, throwing sparks at its tip and leaving a
scorch mark that cools from orange to dark; it ends with the hazard sound. The functions
other than 0x001975b0 have no tag; placed by address order.

Layout:
  +0x08 frame timer            +0x40 duration (s)       +0x44 progress 0..1
  +0x48 radius                 +0x4c current angle      +0x50 start angle
  +0x54 centre (float[4])      +0x64 tip (float[4], relative x/z, absolute y)
  +0x74 turn clockwise         +0x78 on, +0x79 drawn, +0x7a running, +0x7b paused,
  +0x7c/+0x7d beam halves drawn, +0x7e scorch drawn (bytes)
  +0x80 spark system (classlib_fx CSparkPointSystem)       +0x84 hazard sound played
  +0x9c beam segments, +0xa0 segments now, +0xa4 segments built, +0xa8 (n x 12 bytes,
        unused), +0xac beam width
  +0xb0 scorch segments, +0xb4 now, +0xb8 built, +0xbc colour per segment (r, g, b << 7),
        +0xc0 scorch width
  +0xc4, +0xc8 beam vertex buffers (two halves), +0xcc scorch vertex buffer
Vertices are FVF 0x152 (position, normal, colour, uv; 9 words); a segment is two
vertices of a strip.
*/

#include "rollerball_b.h"

#define RBCU_TIMER 0x08
#define RBCU_DURATION 0x40
#define RBCU_PROGRESS 0x44
#define RBCU_RADIUS 0x48
#define RBCU_ANGLE 0x4c
#define RBCU_START_ANGLE 0x50
#define RBCU_CENTRE 0x54
#define RBCU_TIP 0x64
#define RBCU_REVERSE 0x74
#define RBCU_ON 0x78
#define RBCU_DRAWN 0x79
#define RBCU_RUNNING 0x7a
#define RBCU_PAUSED 0x7b
#define RBCU_BEAM1_ON 0x7c
#define RBCU_BEAM2_ON 0x7d
#define RBCU_SCORCH_ON 0x7e
#define RBCU_SPARKS 0x80
#define RBCU_HAZARD 0x84
#define RBCU_BEAM_N 0x9c
#define RBCU_BEAM_NOW 0xa0
#define RBCU_BEAM_BUILT 0xa4
#define RBCU_BEAM_MEM 0xa8
#define RBCU_BEAM_WIDTH 0xac
#define RBCU_SCORCH_N 0xb0
#define RBCU_SCORCH_NOW 0xb4
#define RBCU_SCORCH_BUILT 0xb8
#define RBCU_SCORCH_COLOURS 0xbc
#define RBCU_SCORCH_WIDTH 0xc0
#define RBCU_VB_BEAM1 0xc4
#define RBCU_VB_BEAM2 0xc8
#define RBCU_VB_SCORCH 0xcc

static const char rbb_CutterFile[] = "C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerBall_Cutter.cpp";

/* the colour component tables (core): alpha, red, green, blue */
extern unsigned int DAT_002bc898[256], DAT_002bce18[256], DAT_002bc490[256], DAT_002bd640[256];
#define RBB_ARGB(a, r, g, b) (DAT_002bc898[a] | DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bd640[b])

/* a vertex of FVF 0x152 */
struct RbbVertex {
	float pos[3];
	float normal[3];
	unsigned int colour;
	float u;
	float v;
};
OF_ASSERT_SIZE(struct RbbVertex, 0x24);

/* 0x00274ad0  cutter vtable */
const void *const rbb_vtable_00274ad0[1] = {
	(const void *)FUN_00197590, /* +0x00 scalar deleting destructor */
};

/* 0x001973c0  RollerBall/CRollerBall_Cutter.cpp (inferred)
Constructor. */
void *CRollerBallCutter_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274ad0;
	rbb_TimerCtor(RBB_ADDR(self, RBCU_TIMER));
	return self;
}

/* 0x001973e0  RollerBall/CRollerBall_Cutter.cpp (inferred)
Destructor. */
void CRollerBallCutter_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274ad0;
	rbb_TimerDtor(RBB_ADDR(self, RBCU_TIMER));
}

/* 0x001973f0  RollerBall/CRollerBall_Cutter.cpp (inferred)
Draw the beam (textures of game + 0x1ef8, blended) and the scorch mark (+0x1efc). */
void CRollerBallCutter_Draw(void *self)
{
	if (RBB_C(self, RBCU_DRAWN) == 0)
		return;
	bdSetWorldIdentity();
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	if (RBB_I(self, RBCU_BEAM_NOW) != 0) {
		bdSetTexture(0, (struct bdTexture *)RBB_P(rbb_Game, 0x1ef8));
		bdSetRenderState(10, 3, 0);
		if (RBB_C(self, RBCU_BEAM1_ON) != 0)
			bdDrawVertexBuffer(4, (struct bdVertexBuffer *)RBB_P(self, RBCU_VB_BEAM1), 0,
				RBB_I(self, RBCU_BEAM_NOW) << 1);
		if (RBB_C(self, RBCU_BEAM2_ON) != 0)
			bdDrawVertexBuffer(4, (struct bdVertexBuffer *)RBB_P(self, RBCU_VB_BEAM2), 0,
				RBB_I(self, RBCU_BEAM_NOW) << 1);
	}
	if (RBB_C(self, RBCU_SCORCH_ON) != 0) {
		bdSetTexture(0, (struct bdTexture *)RBB_P(rbb_Game, 0x1efc));
		bdSetRenderState(10, 1, 0);
		if (RBB_I(self, RBCU_SCORCH_NOW) != 0)
			bdDrawVertexBuffer(4, (struct bdVertexBuffer *)RBB_P(self, RBCU_VB_SCORCH), 0,
				RBB_I(self, RBCU_SCORCH_NOW) * 2);
	}
	bdPopRenderState(5);
	bdPopRenderState(6);
}

/* 0x001974f0  RollerBall/CRollerBall_Cutter.cpp (inferred)
Reset: off, the sparks cleared (virtual +0x0c), nothing built. */
void CRollerBallCutter_Reset(void *self)
{
	void *sparks;

	RBB_B(self, RBCU_ON) = 0;
	RBB_B(self, RBCU_DRAWN) = 0;
	RBB_B(self, RBCU_RUNNING) = 0;
	RBB_B(self, RBCU_PAUSED) = 0;
	RBB_B(self, RBCU_BEAM1_ON) = 0;
	RBB_B(self, RBCU_BEAM2_ON) = 0;
	RBB_B(self, RBCU_SCORCH_ON) = 0;
	sparks = RBB_P(self, RBCU_SPARKS);
	RBB_VFN(sparks, 0x0c, void (*)(void *))(sparks);
	RBB_I(self, RBCU_BEAM_NOW) = 0;
	RBB_I(self, RBCU_BEAM_BUILT) = 0;
	RBB_I(self, RBCU_SCORCH_NOW) = 0;
	RBB_I(self, RBCU_SCORCH_BUILT) = 0;
	RBB_I(self, RBCU_HAZARD) = 0;
}

/* 0x00197540  RollerBall/CRollerBall_Cutter.cpp (inferred) */
void CRollerBallCutter_SetRadius(void *self, float radius)
{
	RBB_F(self, RBCU_RADIUS) = radius;
}

/* 0x00197550  RollerBall/CRollerBall_Cutter.cpp (inferred)
Set the duration (also the timer's). */
void CRollerBallCutter_SetDuration(void *self, float seconds)
{
	RBB_F(self, RBCU_DURATION) = seconds;
	rbb_TimerSetDuration(RBB_ADDR(self, RBCU_TIMER), seconds);
}

/* 0x00197560  RollerBall/CRollerBall_Cutter.cpp (inferred) */
void CRollerBallCutter_SetReverse(void *self, int reverse)
{
	RBB_I(self, RBCU_REVERSE) = reverse;
}

/* 0x00197570  RollerBall/CRollerBall_Cutter.cpp (inferred) */
void CRollerBallCutter_SetStartAngle(void *self, float angle)
{
	RBB_F(self, RBCU_START_ANGLE) = angle;
}

/* 0x00197580  RollerBall/CRollerBall_Cutter.cpp (inferred) */
void CRollerBallCutter_Pause(void *self)
{
	RBB_B(self, RBCU_PAUSED) = 1;
}

/* 0x00197590  RollerBall/CRollerBall_Cutter.cpp (inferred)
Scalar deleting destructor. */
void *CRollerBallCutter_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallCutter_Dtor(self);
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x001975b0  RollerBall/CRollerBall_Cutter.cpp
Set up the cutter at `centre` with `segments` beam and scorch segments: the spark
system (100 sparks, game textures +0x1ef0/+0x1ef4, rising; added to the game's particle
list at +0x2ad8) and the three vertex buffers. Returns 0 when a buffer cannot be made. */
int CRollerBallCutter_Init(void *self, float *centre, int segments)
{
	void *mem;
	void *sparks;
	float up[3];

	rbb_TimerInit(RBB_ADDR(self, RBCU_TIMER));
	RBB_F(self, RBCU_ANGLE) = 0.0f;
	memcpy(RBB_V(self, RBCU_CENTRE), centre, 16);
	mem = RBB_NEW(0x98, rbb_CutterFile, 0x59);
	sparks = NULL;
	if (mem != NULL) {
		CSparkPointSystem_Construct((struct CSparkPointSystem *)mem);
		sparks = mem;
	}
	RBB_P(self, RBCU_SPARKS) = sparks;
	CSparkPointSystem_Init((struct CSparkPointSystem *)sparks, NULL, 100);
	CParticleSystem_SetTexture((struct CPSHeader *)RBB_P(self, RBCU_SPARKS), RBB_I(rbb_Game, 0x1ef0));
	CParticleSystem_SetTexture2((struct CParticleSystem *)RBB_P(self, RBCU_SPARKS), RBB_I(rbb_Game, 0x1ef4));
	up[0] = 0.0f;
	up[1] = 1.0f;
	up[2] = 0.0f;
	Obj_SetVector48(RBB_P(self, RBCU_SPARKS), up);
	/* rollerball_a 0x0018e010: add to the game's particle system list */
	RBB_FN(FUN_0018e010, void, (void *, void *))(rbb_Game + 0x2ad8, RBB_P(self, RBCU_SPARKS));

	RBB_P(self, RBCU_BEAM_MEM) = RBB_NEW(segments * 0xc, rbb_CutterFile, 0x67);
	RBB_I(self, RBCU_BEAM_N) = segments;
	RBB_I(self, RBCU_BEAM_NOW) = 0;
	RBB_I(self, RBCU_BEAM_BUILT) = 0;
	RBB_F(self, RBCU_BEAM_WIDTH) = 20.0f;
	RBB_P(self, RBCU_SCORCH_COLOURS) = RBB_NEW(segments * 0xc, rbb_CutterFile, 0x6e);
	RBB_I(self, RBCU_SCORCH_N) = segments;
	RBB_I(self, RBCU_SCORCH_NOW) = 0;
	RBB_I(self, RBCU_SCORCH_BUILT) = 0;
	RBB_F(self, RBCU_SCORCH_WIDTH) = 20.0f;
	if (bdVertexBufferCreate((struct bdVertexBuffer **)RBB_ADDR(self, RBCU_VB_BEAM1),
		RBB_I(self, RBCU_BEAM_N) << 1, 0x152, 0, 1) == 0)
		return 0;
	if (bdVertexBufferCreate((struct bdVertexBuffer **)RBB_ADDR(self, RBCU_VB_BEAM2),
		RBB_I(self, RBCU_BEAM_N) << 1, 0x152, 0, 1) == 0)
		return 0;
	if (bdVertexBufferCreate((struct bdVertexBuffer **)RBB_ADDR(self, RBCU_VB_SCORCH),
		RBB_I(self, RBCU_SCORCH_N) << 1, 0x152, 0, 1) == 0)
		return 0;
	CRollerBallCutter_Reset(self);
	return 1;
}

/* 0x001977d0  RollerBall/CRollerBall_Cutter.cpp (inferred)
Free the vertex buffers and the segment arrays. */
void CRollerBallCutter_Shutdown(void *self)
{
	if (RBB_P(self, RBCU_VB_BEAM1) != NULL)
		bdVertexBufferDestroy((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_BEAM1));
	if (RBB_P(self, RBCU_VB_BEAM2) != NULL)
		bdVertexBufferDestroy((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_BEAM2));
	if (RBB_P(self, RBCU_VB_SCORCH) != NULL)
		bdVertexBufferDestroy((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_SCORCH));
	if (RBB_P(self, RBCU_BEAM_MEM) != NULL) {
		RBB_DELETE(RBB_P(self, RBCU_BEAM_MEM));
		RBB_P(self, RBCU_BEAM_MEM) = NULL;
	}
	if (RBB_P(self, RBCU_SCORCH_COLOURS) != NULL) {
		RBB_DELETE(RBB_P(self, RBCU_SCORCH_COLOURS));
		RBB_P(self, RBCU_SCORCH_COLOURS) = NULL;
	}
}

/* 0x00197850  RollerBall/CRollerBall_Cutter.cpp (inferred)
Build the beam segments swept since the last call: from the circle at the radius to a
point 0-12 units (random) plus the beam width inside it (first half) and outside it
(second half). The texture u runs 0.02 x radius / segments per segment, mirrored at 1
(the original's mirroring inside the loop changes nothing; kept). The angle and u are
kept in x87 precision in the original; double here. `dt` is not used. */
void CRollerBallCutter_BuildBeam(void *self, float dt)
{
	int n = RBB_I(self, RBCU_BEAM_N);
	int now;
	int built;
	float step;
	float du;
	double angle;
	double u;
	int flip;
	int i;
	struct RbbVertex *v1;
	struct RbbVertex *v2;

	(void)dt;
	now = RBB_FTOL((float)n * RBB_F(self, RBCU_PROGRESS));
	built = RBB_I(self, RBCU_BEAM_BUILT);
	RBB_I(self, RBCU_BEAM_NOW) = now;
	if (built >= now)
		return;
	n = n - 1;
	if (RBB_I(self, RBCU_REVERSE) == 0)
		step = 6.28318548f / (float)n;
	else
		step = -6.28318548f / (float)n;
	angle = (double)((float)built * step + RBB_F(self, RBCU_START_ANGLE));
	v1 = bdVertexBufferLock((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_BEAM1), 0);
	v2 = bdVertexBufferLock((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_BEAM2), 0);
	if (v1 == NULL && v2 == NULL)
		return;
	i = RBB_I(self, RBCU_BEAM_BUILT);
	v1 += i * 2;
	v2 += i * 2;
	flip = 0;
	du = RBB_F(self, RBCU_RADIUS) * 0.02f * (1.0f / (float)RBB_I(self, RBCU_BEAM_N));
	u = (double)i * du;
	if (1.0 < u) {
		do {
			u = u - 1.0;
			flip = !flip;
		} while (1.0 < u);
		if (flip)
			u = 1.0 - u;
	}
	for (; i < RBB_I(self, RBCU_BEAM_NOW); i++) {
		float c = (float)cos(angle);
		float s;
		float r1;
		float r2;
		float radius = RBB_F(self, RBCU_RADIUS);
		float cx = RBB_F(self, RBCU_CENTRE + 0);
		float y = RBB_F(self, RBCU_CENTRE + 4);
		float cz = RBB_F(self, RBCU_CENTRE + 8);
		float in_r;
		float out_r;
		unsigned int dark = RBB_ARGB(127, 0, 0, 0);
		unsigned int grey = RBB_ARGB(127, 42, 42, 42);

		r1 = (float)rbb_rand() * 3.05185094e-05f * 12.0f;
		s = (float)sin(angle);
		r2 = (float)rbb_rand() * 3.05185094e-05f * 12.0f;
		in_r = radius - (r1 + RBB_F(self, RBCU_BEAM_WIDTH));
		out_r = r2 + RBB_F(self, RBCU_BEAM_WIDTH) + radius;

		/* first half: inner edge (v = 1) to the circle (v = 0) */
		v1[0].pos[0] = cx - c * in_r;
		v1[0].pos[1] = y;
		v1[0].pos[2] = cz - s * in_r;
		v1[0].colour = dark;
		v1[0].u = (float)u;
		v1[0].v = 1.0f;
		v1[1].pos[0] = cx - c * radius;
		v1[1].pos[1] = y;
		v1[1].pos[2] = cz - s * radius;
		v1[1].colour = grey;
		v1[1].u = (float)u;
		v1[1].v = 0.0f;
		/* second half: the circle (v = 0) to the outer edge (v = 1) */
		v2[0].pos[0] = cx - c * radius;
		v2[0].pos[1] = y;
		v2[0].pos[2] = cz - s * radius;
		v2[0].colour = grey;
		v2[0].u = (float)(1.0 - u);
		v2[0].v = 0.0f;
		v2[1].pos[0] = cx - c * out_r;
		v2[1].pos[1] = y;
		v2[1].pos[2] = cz - s * out_r;
		v2[1].colour = dark;
		v2[1].u = (float)(1.0 - u);
		v2[1].v = 1.0f;

		if (flip) {
			u = u - du;
			if (1.0 < u) {
				flip = 0;
				u = 1.0 - (1.0 - u);
			}
		} else {
			u = u + du;
			if (u < 0.0) {
				flip = 1;
				u = -(-u);
			}
		}
		angle = angle + step;
		v1 += 2;
		v2 += 2;
	}
	bdVertexBufferUnlock((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_BEAM1));
	bdVertexBufferUnlock((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_BEAM2));
	RBB_I(self, RBCU_BEAM_BUILT) = RBB_I(self, RBCU_BEAM_NOW);
}

/* 0x00197bd0  RollerBall/CRollerBall_Cutter.cpp (inferred)
Build the scorch mark: segments built before cool down (red - 0x41, green - 0x20 per
call, in 1/128 steps), new ones start orange (127, 64, 0) between a random inner and outer
edge around the circle. `dt` is not used. */
void CRollerBallCutter_BuildScorch(void *self, float dt)
{
	int n = RBB_I(self, RBCU_SCORCH_N);
	float step;
	float du;
	float u;
	double angle;
	int built;
	int i;
	struct RbbVertex *v;

	(void)dt;
	RBB_I(self, RBCU_SCORCH_NOW) = RBB_FTOL((float)n * RBB_F(self, RBCU_PROGRESS));
	n = n - 1;
	if (RBB_I(self, RBCU_REVERSE) == 0)
		step = 6.28318548f / (float)n;
	else
		step = -6.28318548f / (float)n;
	built = RBB_I(self, RBCU_SCORCH_BUILT);
	angle = (double)((float)built * step + RBB_F(self, RBCU_START_ANGLE));
	v = bdVertexBufferLock((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_SCORCH), 0);
	if (v == NULL)
		return;
	du = RBB_F(self, RBCU_RADIUS) * 0.02f * (1.0f / (float)RBB_I(self, RBCU_SCORCH_N));
	u = (float)RBB_I(self, RBCU_SCORCH_BUILT) * du;
	for (i = 0; i < RBB_I(self, RBCU_SCORCH_NOW); i++, v += 2) {
		int *col = (int *)RBB_P(self, RBCU_SCORCH_COLOURS) + i * 3;
		if (i < RBB_I(self, RBCU_SCORCH_BUILT)) {
			col[0] = col[0] - 0x41;
			col[1] = col[1] - 0x20;
			if (col[0] < 0)
				col[0] = 0;
			if (col[1] < 0)
				col[1] = 0;
			if (col[2] < 0)
				col[2] = 0;
			col = (int *)RBB_P(self, RBCU_SCORCH_COLOURS) + i * 3;
			v[0].colour = RBB_ARGB(127, col[0] >> 7, col[1] >> 7, col[2] >> 7);
			col = (int *)RBB_P(self, RBCU_SCORCH_COLOURS) + i * 3;
			v[1].colour = RBB_ARGB(127, col[0] >> 7, col[1] >> 7, col[2] >> 7);
		} else {
			double c = cos(angle);
			double s = sin(angle);
			double r1 = (double)rbb_rand() * (double)3.05185094e-05f;
			float r2 = (float)rbb_rand() * 3.05185094e-05f;
			double in_r = (double)RBB_F(self, RBCU_RADIUS) - (r1 + r1 + (double)RBB_F(self, RBCU_SCORCH_WIDTH));
			double out_r = (double)(r2 + r2) + (double)RBB_F(self, RBCU_SCORCH_WIDTH) +
				(double)RBB_F(self, RBCU_RADIUS);
			col[0] = 0x3f80;   /* 127 << 7 */
			col[1] = 0x2000;   /* 64 << 7 */
			col[2] = 0;
			v[0].pos[0] = (float)(-(in_r * c) + (double)RBB_F(self, RBCU_CENTRE + 0));
			v[0].colour = RBB_ARGB(127, 127, 64, 0);
			v[0].pos[2] = -((float)in_r * (float)s) + RBB_F(self, RBCU_CENTRE + 8);
			v[0].pos[1] = RBB_F(self, RBCU_CENTRE + 4);
			v[0].u = u;
			v[0].v = 0.95f;
			v[1].colour = RBB_ARGB(127, 127, 64, 0);
			v[1].pos[0] = (float)(-(out_r * c) + (double)RBB_F(self, RBCU_CENTRE + 0));
			v[1].u = u;
			v[1].pos[1] = RBB_F(self, RBCU_CENTRE + 4);
			u = u + du;
			v[1].pos[2] = (float)(-(out_r * (double)(float)s) + (double)RBB_F(self, RBCU_CENTRE + 8));
			v[1].v = 0.05f;
			if (1.0f < u)
				u = u - 1.0f;
			angle = angle + step;
		}
	}
	bdVertexBufferUnlock((struct bdVertexBuffer *)RBB_P(self, RBCU_VB_SCORCH));
	RBB_I(self, RBCU_SCORCH_BUILT) = RBB_I(self, RBCU_SCORCH_NOW);
}

/* 0x00197f40  RollerBall/CRollerBall_Cutter.cpp (inferred)
Start the sweep. */
void CRollerBallCutter_Start(void *self)
{
	float angle;

	rbb_TimerStart(RBB_ADDR(self, RBCU_TIMER));
	RBB_B(self, RBCU_RUNNING) = 1;
	angle = RBB_F(self, RBCU_ANGLE);
	RBB_B(self, RBCU_ON) = 1;
	RBB_B(self, RBCU_DRAWN) = 1;
	RBB_B(self, RBCU_BEAM1_ON) = 1;
	RBB_B(self, RBCU_BEAM2_ON) = 1;
	RBB_B(self, RBCU_SCORCH_ON) = 1;
	RBB_I(self, RBCU_BEAM_NOW) = 0;
	RBB_I(self, RBCU_BEAM_BUILT) = 0;
	RBB_I(self, RBCU_SCORCH_NOW) = 0;
	RBB_I(self, RBCU_SCORCH_BUILT) = 0;
	RBB_F(self, RBCU_TIP + 4) = RBB_F(self, RBCU_CENTRE + 4);
	RBB_F(self, RBCU_TIP + 0) = (float)(cos((double)angle) * -(double)RBB_F(self, RBCU_RADIUS));
	RBB_F(self, RBCU_TIP + 8) = (float)(sin((double)RBB_F(self, RBCU_ANGLE)) * -(double)RBB_F(self, RBCU_RADIUS));
}

/* 0x00197fa0  RollerBall/CRollerBall_Cutter.cpp (inferred)
Per-frame update while on and running: the angle from the timer (unless paused), the
tip and its sparks, the beam and scorch; the hazard sound at 85%. Returns 1 when the
sweep has ended (then the cutter switches off), else 0. */
int CRollerBallCutter_Update(void *self, float dt)
{
	float turn;
	float angle;
	char *sparks;

	if (RBB_C(self, RBCU_ON) == 0 || RBB_C(self, RBCU_RUNNING) == 0)
		return 0;
	if (RBB_C(self, RBCU_PAUSED) == 0) {
		rbb_TimerUpdate(RBB_ADDR(self, RBCU_TIMER));
		RBB_F(self, RBCU_PROGRESS) = rbb_TimerProgress(RBB_ADDR(self, RBCU_TIMER));
	}
	turn = -6.28318548f;
	if (RBB_I(self, RBCU_REVERSE) == 0)
		turn = 6.28318548f;
	RBB_F(self, RBCU_ANGLE) = RBB_F(self, RBCU_PROGRESS) * turn;
	angle = RBB_F(self, RBCU_START_ANGLE) + RBB_F(self, RBCU_ANGLE);
	RBB_F(self, RBCU_ANGLE) = angle;
	CLMath_WrapAngle(angle);   /* the result is not used */
	sparks = RBB_P(self, RBCU_SPARKS);
	RBB_F(self, RBCU_TIP + 4) = RBB_F(self, RBCU_CENTRE + 4);
	RBB_F(self, RBCU_TIP + 0) = (float)(cos((double)RBB_F(self, RBCU_ANGLE)) * -(double)RBB_F(self, RBCU_RADIUS));
	RBB_F(self, RBCU_TIP + 8) = (float)(sin((double)RBB_F(self, RBCU_ANGLE)) * -(double)RBB_F(self, RBCU_RADIUS));
	memcpy(RBB_V(sparks, 0x1c), RBB_V(self, RBCU_TIP), 16);
	FUN_0019d0c0((int *)RBB_P(self, RBCU_SPARKS), 2);
	RBB_B(RBB_P(self, RBCU_SPARKS), 0x58) = 1;
	CRollerBallCutter_BuildBeam(self, dt);
	CRollerBallCutter_BuildScorch(self, dt);
	if (RBB_I(self, RBCU_HAZARD) == 0 && 0.850000024f < RBB_F(self, RBCU_PROGRESS)) {
		rbb_PlaySound(RBB_SND_HAZARD, 0xff, 0, 0, 0);
		RBB_I(self, RBCU_HAZARD) = 1;
	}
	if (1.0f <= RBB_F(self, RBCU_PROGRESS)) {
		RBB_B(self, RBCU_ON) = 0;
		RBB_B(self, RBCU_RUNNING) = 0;
		RBB_B(self, RBCU_BEAM2_ON) = 0;
		RBB_B(self, RBCU_SCORCH_ON) = 0;
		return 1;
	}
	return 0;
}
