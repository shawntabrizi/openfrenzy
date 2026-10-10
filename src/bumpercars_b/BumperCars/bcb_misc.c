/*
BumperCars/bcb_misc.c  <-  gap functions 0x000a4cb0-0x000a5fa0 (files unknown)

The functions between CBCar_Waypoints.cpp and CBCar_Track.cpp that carry no source tag:
- CGameMode_BumperCars (0x000a4cb0-0x000a4dc0, vtable 0x0026684c): the game mode that core's
  mode stack runs; every method forwards to the CBCar_Game object (bumpercars_a, 0x002ed458).
- text helpers: the quoted name of a configuration line, the configuration text reader
  (vtable 0x0026689c, used by every BumperCars file loader).
- draw helpers: a texture's frame by time, the collision mesh in debug, the reflection
  states, a full screen tint, a wire sphere, a light cone, two D3D state blocks.
- maths: atan2 in [0, 2pi), the centre of 4 points, a rotation tilted to the floor normal.
- the pad rumble shapes, the "same team" test, and the debug free camera (0x002f26c4).
*/

#include "bcb_items.h"
#include "core_data.h"

/* ======================================================================== CGameMode_BumperCars */

#define BCB_GAME ((struct CBCar_Game *)BCB_ADDR(0x002ed458))

/* bumpercars_a's CBCar_Game methods that are missing from the function table (reached by
the jumps below); prototypes as bumpercars_a.h declares them */
void FUN_00086c90(struct CBCar_Game *self);
void FUN_00086ea0(struct CBCar_Game *self);
void FUN_00086ed0(struct CBCar_Game *self);
void FUN_00086f80(struct CBCar_Game *self, int top);
void FUN_000874c0(struct CBCar_Game *self, int top);

/* 0x0026684c  the vtable of CGameMode_BumperCars (slots as core's CGameModeVtbl) */
const void *const vtable_0026684c[13] = {
	(const void *)CGameMode_BumperCars_Destroy, /* +0x00 FUN_000a4da0 */
	(const void *)CGameMode_BumperCars_Suspend, /* +0x04 FUN_000a4d00 */
	(const void *)CGameMode_BumperCars_Resume,  /* +0x08 FUN_000a4cf0 */
	(const void *)CGameMode_BumperCars_Exit,    /* +0x0c FUN_000a4dc0 */
	(const void *)CGameMode_BumperCars_Enter,   /* +0x10 FUN_000a4ce0 */
	(const void *)CGameMode_BumperCars_Update,  /* +0x14 FUN_000a4d10 */
	(const void *)CGameMode_BumperCars_Process, /* +0x18 FUN_000a4d20 */
	(const void *)CGameMode_BumperCars_Draw,    /* +0x1c FUN_000a4d60 */
	(const void *)FUN_001ec260,                 /* +0x20 engine_misc */
	(const void *)FUN_00012930,                 /* +0x24 core_modes_a */
	(const void *)FUN_000128e0,                 /* +0x28 core_modes_a */
	(const void *)CGameMode_BumperCars_Init,    /* +0x2c FUN_000a4d70 */
	(const void *)CGameMode_BumperCars_Start,   /* +0x30 FUN_000a4d80 */
};

/* 0x000a4cb0  (gap)
Constructor: a game mode named "BumperCars" (0x2c bytes; +0x28: pop the mode when the game
ends). */
struct CGameMode *CGameMode_BumperCars_Ctor(struct CGameMode *self)
{
	CGameMode_ctor(self, (char *)"BumperCars");
	BCB_AT(self, 0, const void *) = vtable_0026684c;
	BCB_I32(self, 0x28) = 0;
	return self;
}

/* 0x000a4cd0  (gap) */
void CGameMode_BumperCars_Dtor(struct CGameMode *self)
{
	BCB_AT(self, 0, const void *) = vtable_0026684c;
	CGameMode_dtor(self);
}

/* 0x000a4ce0  (gap)  enter: forwards to the game */
void CGameMode_BumperCars_Enter(struct CGameMode *self)
{
	(void)self;
	FUN_00086c90(BCB_GAME);
}

/* 0x000a4cf0  (gap)  resume */
void CGameMode_BumperCars_Resume(struct CGameMode *self)
{
	(void)self;
	FUN_00086ea0(BCB_GAME);
}

/* 0x000a4d00  (gap)  suspend */
void CGameMode_BumperCars_Suspend(struct CGameMode *self)
{
	(void)self;
	FUN_00086ed0(BCB_GAME);
}

/* 0x000a4d10  (gap)  update */
void CGameMode_BumperCars_Update(struct CGameMode *self, int top)
{
	(void)self;
	FUN_00086f80(BCB_GAME, top);
}

/* 0x000a4d20  (gap)
process: runs the game; when it is over and the mode was pushed to be popped, pops it. */
void CGameMode_BumperCars_Process(struct CGameMode *self, int top)
{
	if (CBCar_Game_Process(BCB_GAME, top) == 0 && BCB_I32(self, 0x28) != 0)
		CGameModeStack_Pop((struct CGameModeStack *)CORE_ADDR(0x002ba750), 1);
}

/* 0x000a4d60  (gap)  draw */
void CGameMode_BumperCars_Draw(struct CGameMode *self, int top)
{
	(void)self;
	FUN_000874c0(BCB_GAME, top);
}

/* 0x000a4d70  (gap)  slot 0x2c: loads the game */
void CGameMode_BumperCars_Init(struct CGameMode *self)
{
	(void)self;
	CBCar_Game_Initialise(BCB_GAME);
}

/* 0x000a4d80  (gap)  slot 0x30: starts the game; the mode becomes opaque */
void CGameMode_BumperCars_Start(struct CGameMode *self)
{
	CBCar_Game_Start(BCB_GAME);
	BCB_AT(self, 0x10, unsigned int) = BCB_AT(self, 0x10, unsigned int) | 1;
}

/* 0x000a4da0  (gap)  the deleting destructor */
struct CGameMode *CGameMode_BumperCars_Destroy(struct CGameMode *self, unsigned char flags)
{
	CGameMode_BumperCars_Dtor(self);
	if ((flags & 1) != 0)
		BCB_DELETE(self);
	return self;
}

/* 0x000a4dc0  (gap)
exit: shuts the game down and prints the free 3D sound buffers. */
void CGameMode_BumperCars_Exit(struct CGameMode *self)
{
	DSCAPS caps;

	(void)self;
	CBCar_Game_Shutdown(BCB_GAME);
	IDirectSound_GetCaps((struct IDirectSound *)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", caps.dwFree3DBuffers);
}

/* ======================================================================== text */

/* 0x000a4e00  (gap)
Copies the text between the first two quotes of `line` into `out` (at most `size`
characters, then a NUL). Returns 0 (and an empty `out`) when there is no quote. */
int BCarText_GetName(char *line, char *out, int size)
{
	int n;

	for (;;)
	{
		if (line == NULL)
		{
			*out = '\0';
			return 0;
		}
		if (*line == '"')
			break;
		line++;
	}
	for (n = 0; (line = line + 1, line != NULL && *line != '"') && n < size; n++)
		out[n] = *line;
	out[n] = '\0';
	return 1;
}

/* 0x0026689c  the vtable of the text reader: the deleting destructor */
const void *const vtable_0026689c[1] = {
	(const void *)BCarTextReader_Destroy,       /* FUN_000a5c60 */
};

/* 0x000a5be0  (gap)  constructor of the text reader */
void BCarTextReader_Construct(struct BCarTextReader *self)
{
	self->vtable = vtable_0026689c;
}

/* 0x000a5bf0  (gap)  destructor of the text reader */
void BCarTextReader_Destruct(struct BCarTextReader *self)
{
	self->vtable = vtable_0026689c;
}

/* 0x000a5c00  (gap)
Starts reading `size` bytes of text named `name` (tabs become spaces). */
void BCarTextReader_Init(struct BCarTextReader *self, char *buffer, int size, const char *name)
{
	_snprintf(self->name, 0x1ff, "%s", name);
	self->buffer = buffer;
	self->cursor = buffer;
	self->end = buffer + size;
	self->line = 1;
	for (; buffer < self->end; buffer++)
	{
		if (*buffer == '\t')
			*buffer = ' ';
	}
}

/* 0x000a5c60  (gap)  the deleting destructor of the text reader */
struct BCarTextReader *BCarTextReader_Destroy(struct BCarTextReader *self, unsigned char flags)
{
	self->vtable = vtable_0026689c;
	if ((flags & 1) != 0)
		BCB_DELETE(self);
	return self;
}

/* ======================================================================== drawing */

/* 0x000a4e50  (gap)
Shows the frame of an animated texture at `time`: the frames from `first` on, each lasting
its own time (frame records of 0x18 bytes, the time at +0x14), loop. */
void BCarTexture_SetFrameAtTime(struct bdTexture *texture, int time, int first)
{
	int count = BCB_AT(texture, 0x37, unsigned char);
	int total = 0;
	int *frame;
	int sum;
	int k;

	if (!(first < count))
		return;
	frame = (int *)((char *)BCB_PTR(texture, 0x24) + 0x14 + first * 0x18);
	for (k = 0; k < count - first; k++)
		total = total + frame[k * 6];
	if (total == 0)
		return;
	if (time < 0)
		time = total - -time % total;
	sum = 0;
	while (sum = sum + *frame, sum <= time % total)
	{
		first++;
		frame = frame + 6;
		if (count <= first)
			return;
	}
	bdTextureSetFrame(texture, first);
}

/* 0x000a4ed0  (gap)
Draws the triangles of cell `cell` of a collision mesh (debug), lines when `solid` is 0, in
the colour 0xAABBGGRR. */
void BCarCollMesh_DrawCell(struct BCollMesh *mesh, int cell, unsigned int colour, int solid)
{
	struct BCarVertex *v = (struct BCarVertex *)BCB_ADDR(0x002f22b0);
	int *list = (int *)((char *)BCB_PTR(mesh, 0x4c) + cell * 0x10);
	unsigned short *tris = (unsigned short *)BCB_PTR(mesh, 0x3c);
	float *verts = (float *)BCB_PTR(mesh, 0x2c);
	unsigned int c;
	int i;

	bdSetTexture(0, NULL);
	c = DAT_002bc898[colour >> 24] | DAT_002bd640[(colour >> 16) & 0xff] |
		DAT_002bc490[(colour >> 8) & 0xff] | DAT_002bce18[colour & 0xff];
	v[0].colour = c;
	v[1].colour = c;
	v[2].colour = c;
	v[3].colour = c;
	for (i = 0; i < list[0]; i++)
	{
		unsigned short t = (unsigned short)((((short *)list[1])[i] + (short)list[2]) * 3);
		unsigned short *tri = &tris[t];
		float *p;
		int type;
		int count;

		p = &verts[tri[0] * 3];
		if (solid == 0)
		{
			v[0].pos[0] = p[0];
			v[0].pos[1] = p[1];
			v[0].pos[2] = p[2];
			v[3].pos[0] = p[0];
			v[3].pos[1] = p[1];
			v[3].pos[2] = p[2];
			count = 4;
			type = 2;
		}
		else
		{
			v[0].pos[0] = p[0];
			v[0].pos[1] = p[1];
			v[0].pos[2] = p[2];
			count = 3;
			type = 3;
		}
		p = &verts[tri[1] * 3];
		v[1].pos[0] = p[0];
		v[1].pos[1] = p[1];
		v[1].pos[2] = p[2];
		p = &verts[tri[2] * 3];
		v[2].pos[0] = p[0];
		v[2].pos[1] = p[1];
		v[2].pos[2] = p[2];
		bdDrawPrimitiveFVF(type, v, count, 0x152);
	}
}

/* 0x000a5050  (gap)
Draws a whole collision mesh (debug), every cell of its grid. */
void BCarCollMesh_Draw(struct BCollMesh *mesh, unsigned int colour, int solid)
{
	int i;

	bdPushRenderState(6, 1, 0);
	bdPushRenderState(4, 0, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(5, 0, 0);
	for (i = 0; i < BCB_I32(mesh, 0x20) * BCB_I32(mesh, 0x24) * BCB_I32(mesh, 0x28); i++)
		BCarCollMesh_DrawCell(mesh, i, colour, solid);
	bdPopRenderState(4);
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(5);
}

/* 0x000a50e0  (gap)
The render states of the floor reflections: the brightness is 0x002efa90 (x3 for mode 0,
which also turns blending off). */
void BCarRender_BeginReflection(int mode)
{
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(7, 1, 0);
	if (mode != 0)
	{
		bdPushRenderState(10, 1, 0);
		baSetBrightness(g_ReflectBrightness);
		return;
	}
	bdPushRenderState(10, 0, 0);
	baSetBrightness(g_ReflectBrightness * 3);
}

/* 0x000a5150  (gap)  restores the states after a reflection */
void BCarRender_EndReflection(void)
{
	baSetBrightness(0x100);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(7, 3, 0);
	bdPopRenderState(10);
}

/* 0x000a5390  (gap)
Tints the whole screen with a flat colour (r, g, b, alpha), drawn as a 2D quad. */
void BCarRender_ScreenTint(int r, int g, int b, int alpha)
{
	struct BCarVertex *v = (struct BCarVertex *)BCB_ADDR(0x002f2340);
	float hw;
	float hh;
	unsigned int colour;

	bdSetProjectionMode(0);
	bdPushRenderState(7, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdSetTexture(0, NULL);
	bdSetWorldIdentity();
	hw = (float)(DAT_002ba928 >> 1);
	hh = (float)(DAT_002ba92c >> 1);
	colour = DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bc898[alpha];
	v[0].pos[0] = -hw;
	v[0].pos[1] = -hh;
	v[1].pos[0] = -hw;
	v[1].pos[1] = hh;
	v[2].pos[0] = hw;
	v[2].pos[1] = -hh;
	v[3].pos[0] = hw;
	v[3].pos[1] = hh;
	v[0].colour = colour;
	v[1].colour = colour;
	v[2].colour = colour;
	v[3].colour = colour;
	v[0].uv[0] = 0.0f;
	v[0].uv[1] = 1.0f;
	v[1].uv[0] = 0.0f;
	v[1].uv[1] = 0.0f;
	v[2].uv[0] = 1.0f;
	v[2].uv[1] = 1.0f;
	v[3].uv[0] = 1.0f;
	v[3].uv[1] = 0.0f;
	v[0].pos[2] = 0.0f;
	v[1].pos[2] = 0.0f;
	v[2].pos[2] = 0.0f;
	v[3].pos[2] = 0.0f;
	bdDrawPrimitiveFVF(4, v, 4, 0x152);
	bdSetProjectionMode(1);
	bdPopRenderState(7);
	bdPopRenderState(5);
	bdPopRenderState(3);
	bdPopRenderState(6);
	bdPopRenderState(10);
}

/* ======================================================================== maths */

/* 0x000a5180  (gap)
1 when entities a and b are cars of the same team in a team game; the player index of each
(-1: not a car) goes to *indexA and *indexB. */
int BCarEntity_SameTeam(struct BCarEntity *a, struct BCarEntity *b, int *indexA, int *indexB)
{
	int ia = -1;
	int ib = -1;

	if (g_BCarPlayers[0].entity == a)
		ia = 0;
	if (g_BCarPlayers[0].entity == b)
		ib = 0;
	if (g_BCarPlayers[1].entity == a)
		ia = 1;
	if (g_BCarPlayers[1].entity == b)
		ib = 1;
	if (g_BCarPlayers[2].entity == a)
		ia = 2;
	if (g_BCarPlayers[2].entity == b)
		ib = 2;
	if (g_BCarPlayers[3].entity == a)
		ia = 3;
	if (g_BCarPlayers[3].entity == b)
		ib = 3;
	if (indexA != NULL)
		*indexA = ia;
	if (indexB != NULL)
		*indexB = ib;
	if (DAT_002baebc != 0 && ia != -1 && ib != -1)
		return DAT_002baed4[ia * 10 + 8] == DAT_002baed4[ib * 10 + 8];   /* team: 0x002baef4 */
	return 0;
}

/* 0x000a5540  (gap)
atan2(y, x) moved into [0, 2pi). */
float BCarMath_Atan2(float y, float x)
{
	double a = atan2((double)y, (double)x);

	if (6.2831855 < a)              /* 0x00255930 */
		a = a - 6.2831855;
	if (a < 0.0)
		a = a + 6.2831855;
	return (float)a;
}

/* 0x000a57a0  (gap)
The centre of 4 points given as (x, z) pairs, on the floor (y 0, w 1). */
void BCarMath_Centre4(float *out, float x1, float z1, float x2, float z2, float x3, float z3,
	float x4, float z4)
{
	out[1] = 0.0f;
	out[3] = 1.0f;
	out[0] = (x4 + x3 + x2 + x1) * 0.25f;                       /* 0x00257b88 */
	out[2] = (z4 + z3 + z2 + z1) * 0.25f;
}

/* 0x000a5af0  (gap)
The rotation `quat` tilted to stand on a floor of normal `normal`: the angle of its axis
grows by pi * (1 - |up . normal|) (used to mirror instances in the floor). */
void BCarMath_TiltToNormal(float *out, float *quat, float *normal)
{
	float q[4];
	float n[4];
	float axisAngle[4];
	float m[16];
	float up[4];

	q[0] = quat[0];
	q[1] = quat[1];
	q[2] = quat[2];
	q[3] = quat[3];
	bmVectorNormalise(n, normal);
	bmQuaternionToAxisAngle(axisAngle, q);
	bmQuaternionToMatrix(m, q);
	up[0] = 0.0f;
	up[1] = 1.0f;
	up[2] = 0.0f;
	up[3] = 1.0f;
	bmTransformVector4InPlace(m, up);
	bmVectorNormalise(up, up);
	axisAngle[3] = (3.1415925f - fabsf(up[0] * n[0] + n[1] * up[1] + n[2] * up[2]) * 3.1415925f) +
		axisAngle[3];                                       /* 0x00256750 */
	bmAxisAngleToQuaternion(out, axisAngle);
}

/* ======================================================================== rumble */

/* the rumble manager (core constructs it at 0x002ed324; classlib_ui's CRumble) */
extern unsigned char DAT_002ed324[];

/* 0x000a5230  (gap)
The pad rumble of a BumperCars event (0 small knock, 1 big hit, 2 medium hit, 3 boost,
4 small knock on the other motor) for `player`, `strength` 0-1. */
void BCarGame_Rumble(int player, int type, float strength)
{
	int amplitude;

	switch (type)
	{
	case 0:
		amplitude = BCB_FTOL(strength * 32767.0f);          /* 0x0025df88 */
		CLRumble_PlayMotor((int)DAT_002ed324, player, 0, amplitude, 200, 0x46, 10, 0, 0);
		return;
	case 1:
		amplitude = BCB_FTOL(strength * 32767.0f);
		CLRumble_PlayMotor((int)DAT_002ed324, player, 0, amplitude, 0x5dc, 0x1e, 0x28, 0, 3);
		CLRumble_PlayMotor((int)DAT_002ed324, player, 1, amplitude, 1000, 10, 0x3c, 0, 4);
		return;
	case 2:
		amplitude = BCB_FTOL(strength * 32767.0f);
		CLRumble_PlayMotor((int)DAT_002ed324, player, 0, amplitude, 500, 10, 0x1e, 0, 4);
		CLRumble_PlayMotor((int)DAT_002ed324, player, 1, amplitude, 400, 0x1e, 0x3c, 0, 3);
		return;
	case 3:
		amplitude = BCB_FTOL(strength * 32767.0f);
		CLRumble_PlayMotor((int)DAT_002ed324, player, 1, amplitude, 0x2ee, -1, 0x3c, 0, 0);
		return;
	case 4:
		amplitude = BCB_FTOL(strength * 32767.0f);
		CLRumble_PlayMotor((int)DAT_002ed324, player, 1, amplitude, 200, 0x46, 10, 0, 3);
		return;
	}
}

/* ======================================================================== shapes */

/* 0x000a5580  (gap)
A wire sphere (debug): `rings` x `segments` quads at `pos`, opaque colour (r, g, b). */
void BCarDraw_WireSphere(float *pos, float radius, int rings, int segments, int r, int g, int b)
{
	struct BCarVertex *v = (struct BCarVertex *)BCB_ADDR(0x002f23d0);
	float stepTheta;
	float stepPhi;
	float theta;
	unsigned int colour;

	bdSetTexture(0, NULL);
	stepTheta = 3.1415925f / (float)rings;                      /* 0x00256750 */
	colour = DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bc898[0xff]; /* 0x002bcc94 */
	theta = 0.0f;
	stepPhi = 6.2831855f / (float)segments;                     /* 0x00255930 */
	v[0].colour = colour;
	v[1].colour = colour;
	v[2].colour = colour;
	v[3].colour = colour;
	do
	{
		double c0 = cos((double)theta);
		float r0 = (float)(sin((double)theta) * (double)radius);
		double c1;
		float r1;
		float phi;

		theta = theta + stepTheta;
		if (3.1415925f < theta)
			theta = 3.1415925f;
		c1 = cos((double)theta);
		r1 = (float)(sin((double)theta) * (double)radius);
		phi = 0.0f;
		do
		{
			double cp = cos((double)phi);
			double sp = sin((double)phi);
			double cp2;
			double sp2;

			phi = phi + stepPhi;
			if (6.2831855f < phi)
				phi = 6.2831855f;
			v[0].pos[0] = (float)(cp * (double)r1 + (double)pos[0]);
			v[0].pos[1] = (float)(c1 * (double)radius) + pos[1];
			v[0].pos[2] = (float)(sp * (double)r1 + (double)pos[2]);
			v[1].pos[0] = (float)((double)r0 * cp) + pos[0];
			v[1].pos[1] = (float)(c0 * (double)radius) + pos[1];
			v[1].pos[2] = (float)((double)r0 * sp) + pos[2];
			cp2 = cos((double)phi);
			v[2].pos[0] = (float)((double)r0 * cp2 + (double)pos[0]);
			v[2].pos[1] = (float)(c0 * (double)radius) + pos[1];
			sp2 = sin((double)phi);
			v[2].pos[2] = (float)((double)r0 * sp2 + (double)pos[2]);
			v[3].pos[0] = (float)(cp2 * (double)r1 + (double)pos[0]);
			v[3].pos[1] = (float)(c1 * (double)radius) + pos[1];
			v[3].pos[2] = (float)(sp2 * (double)r1 + (double)pos[2]);
			bdDrawPrimitiveFVF(2, v, 4, 0x152);
		} while (phi < 6.2831855f);
	} while (theta < 3.1415925f);
}

/* 0x000a58e0  (gap)
A light cone from `from` to the circle around `to` (half angle `angle` radians), in colour
(r, g, b) at the apex fading to black at the rim, textured with `texture`. */
void BCarDraw_LightCone(float *from, float *to, float angle, int r, int g, int b,
	struct bdTexture *texture)
{
	struct BCarVertex *v = (struct BCarVertex *)BCB_ADDR(0x002f2460);
	double dx;
	double dy;
	float dz;
	double t;
	double heading;
	float radius;
	unsigned int black;
	int i;

	bdSetTexture(0, texture);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(4, 1, 0);
	bdSetWorldIdentity();
	dx = (double)to[0] - (double)from[0];
	dy = (double)to[1] - (double)from[1];
	dz = to[2] - from[2];
	t = tan((double)angle);
	heading = atan2(dx, (double)dz);
	if (6.2831855 < heading)                                    /* 0x00255930 */
		heading = heading - 6.2831855;
	if (heading < 0.0)
		heading = heading + 6.2831855;
	v[0].pos[0] = from[0];
	v[0].pos[1] = from[1];
	v[0].pos[2] = from[2];
	v[0].colour = DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bd640[b] | DAT_002bc898[0x7f]; /* 0x002bca94 */
	black = DAT_002bc898[0] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0];
	v[0].uv[0] = 0.5f;
	v[0].uv[1] = 0.5f;
	radius = (float)(t * sqrt((double)dz * (double)dz + dx * dx + dy * dy) * 0.6666667);  /* 0x00266898 */
	for (i = 1; i < 17; i++)
	{
		double a = (double)i * 0.41887903;                        /* 0x00266894 */
		double b2 = a - (heading + 1.5707963);                    /* 0x00253898 */
		double s = sin(b2);
		double rr = (double)radius - 0.0 * s;

		v[i].pos[0] = (float)(sin(a) * rr + (double)to[0]);
		v[i].pos[1] = to[1];
		v[i].pos[2] = (float)(cos(a) * rr + (double)to[2]);
		v[i].colour = black;
		v[i].uv[0] = (float)(s * 0.5 + 0.5);
		v[i].uv[1] = (float)(0.5 - cos(b2) * 0.5);
	}
	bdDrawPrimitiveFVF(5, v, 0x11, 0x152);
	bdPopRenderState(4);
	bdPopRenderState(3);
	bdPopRenderState(5);
	bdPopRenderState(10);
	bdPopRenderState(6);
}

/* 0x000a57f0  (gap)
Texture stage 1 modulates by the texture factor alpha (`alpha` 0-255): used to fade the
second set of the arena. */
void BCarRender_SetFadeStage(int alpha)
{
	D3DDevice_SetRenderState_TextureFactor((unsigned long)alpha << 24);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xa, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
}

/* 0x000a5870  (gap)
The simple render states 71, 69, 73, 70 (D3D__RenderState) for the water of the arena; the
original sets each with D3DDevice_SetRenderState_Simple and stores it in the state copy. */
void BCarRender_SetWaterStates(void)
{
	D3DDevice_SetRenderState_Simple(0x40368, 1);
	D3D__RenderState[71] = 1;
	D3DDevice_SetRenderState_Simple(0x40378, 0x1e01);
	D3D__RenderState[69] = 0x1e01;
	D3DDevice_SetRenderState_Simple(0x40360, 1);
	D3D__RenderState[73] = 1;
	D3DDevice_SetRenderState_Simple(0x40364, 0x207);
	D3D__RenderState[70] = 0x207;
}

/* ======================================================================== the free camera */

/* the debug free camera (0x26c bytes, one at 0x002f26c4): orbits a player's position
(0x002ed350) or blends to one of up to 32 recorded points; pad 1 drives it */
struct BCarFreeCamera {
	float eye[4];               /* 0x000 */
	float look[4];              /* 0x010 */
	float fov;                  /* 0x020 */
	int target;                 /* 0x024 player 0-3 */
	float height;               /* 0x028 */
	float distance;             /* 0x02c */
	float pitch;                /* 0x030 */
	float yaw;                  /* 0x034 */
	int point;                  /* 0x038 -1: orbit */
	float blend;                /* 0x03c */
	int clearPlayers;           /* 0x040 */
	int numPoints;              /* 0x044 */
	float points[32][4];        /* 0x048 */
	struct biMap *map;          /* 0x248 */
	struct biChannel *leftX;    /* 0x24c */
	struct biChannel *leftY;    /* 0x250 */
	struct biChannel *rightX;   /* 0x254 */
	struct biChannel *rightY;   /* 0x258 */
	struct biChannel *action1ndb; /* 0x25c */
	struct biChannel *action2ndb; /* 0x260 */
	struct biChannel *action1;  /* 0x264 */
	struct biChannel *action2;  /* 0x268 */
};
OF_ASSERT_SIZE(struct BCarFreeCamera, 0x26c);

/* the players' positions (classlib_ui, 0x002ed350: float[4] per player) */
extern float DAT_002ed350[];

/* 0x000a5c80  (gap)  destructor of the free camera */
void BCarFreeCamera_Destruct(struct BCarFreeCamera *self)
{
	biDeleteMap(self->map);
	g_FreeCamera = NULL;
}

/* 0x000a5ca0  (gap)
Constructor of the free camera: looking at player `target` from `height` above; with
`clearPlayers` the core player table is emptied each update (an attract demo). */
struct BCarFreeCamera *BCarFreeCamera_Construct(struct BCarFreeCamera *self, int target,
	float height, int clearPlayers)
{
	unsigned int *words = (unsigned int *)self;
	unsigned int port;
	int i;

	for (i = 0; i < 0x9b; i++)
		words[i] = 0;
	self->distance = 1000.0f;
	self->pitch = -0.31415927f;                                 /* 0xbea0d97b */
	self->fov = 1.2217305f;                                     /* 0x3f9c61aa */
	self->point = -1;
	self->clearPlayers = clearPlayers;
	g_FreeCamera = self;
	self->target = target;
	self->height = height;
	self->map = (struct biMap *)biCreateMap(8);
	self->leftX = biCreateChannel(self->map, "leftX", 3, 0, 0, 0, 0);
	self->leftY = biCreateChannel(self->map, "leftY", 3, 0, 0, 0, 0);
	self->rightX = biCreateChannel(self->map, "rightX", 3, 0, 0, 0, 0);
	self->rightY = biCreateChannel(self->map, "rightY", 3, 0, 0, 0, 0);
	self->action1 = biCreateChannel(self->map, "action1", 1, 1, 0, 0, 0);
	self->action2 = biCreateChannel(self->map, "action2", 1, 1, 0, 0, 0);
	self->action1ndb = biCreateChannel(self->map, "action1ndb", 1, 0, 0, 0, 0);
	self->action2ndb = biCreateChannel(self->map, "action2ndb", 1, 0, 0, 0, 0);
	port = (unsigned int)DAT_002baee0[0];
	biBindInput(self->leftX, 0xa7, 0);
	biBindInput(self->leftY, 0xa8, 0);
	biBindInput(self->rightX, 0xa9, 0);
	biBindInput(self->rightY, 0xaa, 0);
	biBindInput(self->leftX, port | 0xb9, 0);
	biBindInput(self->leftY, port | 0xba, 0);
	biBindInput(self->rightX, port | 0xbb, 0);
	biBindInput(self->rightY, port | 0xbc, 0);
	biBindInput(self->action1, port | 0xc5, 0);
	biBindInput(self->action2, port | 0xc7, 0);
	biBindInput(self->action2, port | 0xc6, 0);
	biBindInput(self->action1ndb, port | 0xc5, 0);
	biBindInput(self->action2ndb, port | 0xc7, 0);
	biBindInput(self->action2ndb, port | 0xc6, 0);
	biBindInput(self->leftX, port | 0xc0, port | 0xbf);
	return self;
}

/* 0x000a5f60  (gap)  records a camera point (at most 32) */
void BCarFreeCamera_AddPoint(struct BCarFreeCamera *self, float *point)
{
	if (self->numPoints < 0x20)
	{
		self->points[self->numPoints][0] = point[0];
		self->points[self->numPoints][1] = point[1];
		self->points[self->numPoints][2] = point[2];
		self->points[self->numPoints][3] = point[3];
		self->numPoints = self->numPoints + 1;
	}
}

/* the value of a pad channel */
#define BCB_CHANNEL(c) BCB_I32(c, 0x14)

/* 0x000a5fa0  (gap)
Updates the free camera: action 1 changes the player looked at, action 2 the recorded point
(or back to orbiting); the sticks turn, raise and zoom it (or blend to the point). */
void BCarFreeCamera_Update(struct BCarFreeCamera *self, float dt)
{
	float target[4];

	if (self->clearPlayers != 0)
	{
		int i;

		for (i = 0; i < BCB_NUM_PLAYERS; i++)
			DAT_002baed8[i * 10] = 0;
		CORE_VAR(int, 0x002bae8c) = 0;   /* core: number of human players */
		CORE_VAR(int, 0x002bae90) = 4;   /* core: number of players */
	}
	if (BCB_CHANNEL(self->action1) != 0)
	{
		self->target = (self->target + 1) & 3;
		self->height = self->height;
	}
	if (BCB_CHANNEL(self->action2) != 0)
	{
		self->point = self->point + 1;
		if (self->numPoints <= self->point)
			self->point = -1;
		self->blend = 1.0f;
	}
	self->pitch = (float)BCB_CHANNEL(self->leftY) * 3.0517578e-05f * dt * 0.01f + self->pitch; /* 0x002668f4, 0x0025372c */
	if (!(self->pitch < 1.5704821f))                              /* 0x002668f0 */
		self->pitch = 1.5704821f;
	if (self->pitch <= -1.5704821f)                               /* 0x002668ec */
		self->pitch = -1.5704821f;
	self->yaw = (float)-(BCB_CHANNEL(self->rightX) + BCB_CHANNEL(self->leftX)) * 3.0517578e-05f * dt *
		0.02f + self->yaw;                                         /* 0x0025588c */
	self->distance = (float)BCB_CHANNEL(self->rightY) * 3.0517578e-05f * dt * 13.0f + self->distance; /* 0x002668e8 */
	if (self->distance < 20.0f)                                   /* 0x00255530 */
		self->distance = 20.0f;
	self->blend = (float)BCB_CHANNEL(self->rightY) * 3.0517578e-05f * dt * 0.01f + self->blend;
	if (2.0f < self->blend)                                       /* 0x00257318 */
		self->blend = 2.0f;
	if (self->blend < 0.0f)
		self->blend = 0.0f;
	target[0] = DAT_002ed350[self->target * 4 + 0];
	target[1] = DAT_002ed350[self->target * 4 + 1] + self->height;
	target[2] = DAT_002ed350[self->target * 4 + 2];
	target[3] = DAT_002ed350[self->target * 4 + 3];
	if (self->point == -1)
	{
		float v[4];
		float offset[4];

		v[2] = self->distance;
		v[0] = 0.0f;
		v[1] = 0.0f;
		v[3] = 1.0f;
		bmVectorRotateXY(offset, v, self->pitch, self->yaw);
		self->look[0] = target[0];
		self->look[1] = target[1];
		self->look[2] = target[2];
		self->look[3] = target[3];
		self->eye[0] = offset[0] + target[0];
		self->eye[1] = offset[1] + target[1];
		self->eye[2] = offset[2] + target[2];
	}
	else
	{
		float *p = self->points[self->point];
		float z = p[2] - target[2];

		self->look[0] = target[0];
		self->look[1] = target[1];
		self->look[2] = target[2];
		self->look[3] = target[3];
		self->eye[3] = 1.0f;
		self->eye[0] = (p[0] - self->look[0]) * self->blend + self->look[0];
		self->eye[1] = (p[1] - self->look[1]) * self->blend + self->look[1];
		self->eye[2] = z * self->blend + self->look[2];
	}
	bdSetFieldOfView(self->fov, self->fov * 3.0f * 0.25f);       /* 0x0025384c, 0x00257b88 */
	bdSetCameraLookAt(self->eye, self->look, 0.0f);
	bdUpdateView();
}
