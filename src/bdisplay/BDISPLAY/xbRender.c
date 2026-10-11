/*
XBRENDER.C  <-  C:\Babel\Xbox\Src\BDISPLAY\xbRender.cpp

The camera and projections, viewports, the Babel render states (fog,
filtering, blending, ...), and render targets. 0x00043490-0x000459b0.
Only bdCreateRenderTarget (0x00044e90) carries the file's assert tag; the
camera and render-state functions before it (after the last xbFont.cpp
function) and the render-target functions after it are placed here by
what they do (inferred).

Matrices are row-major float[16] (D3DMATRIX); bKernel's multiply makes
out = b x a.
*/

#include "bdisplay_internal.h"

#define RENDER_FILE "C:\\Babel\\Xbox\\Src\\BDISPLAY\\xbRender.cpp"

#define HALF_PI 1.57079625f /* 0x3fc90fda (1.5707964f would be 0x3fc90fdb) */

/* port: how much wider than 4:3 the view is drawn (Hor+). With display.aspect_ratio the
screen has more than 640 columns (bdOpenDisplay). The game gives its fields of view for
4:3, so on the screen the horizontal one widens by the screen's width over 640 and the
vertical one stays: the picture shows more at the sides and nothing is stretched. Each
viewport on the screen widens with it (the whole screen, the character select's quarters).
A render target of the game's own keeps its shape: 1. The original has no such factor. */
static float screen_widening(void)
{
	if (DAT_002bc464 != &DAT_002bcc98 || DAT_002bc40c <= 640)
		return 1.0f;
	return (float)DAT_002bc40c / 640.0f;
}

/* the world * view * projection matrix after a change of the projection */
static void update_world_view_projection(void)
{
	bd_matrix_multiply(DAT_002be470, PTR_DAT_00284c70, DAT_002be430);
	DAT_002be5c8 = 0;
	DAT_002be5c4 = 0;
}

/* 0x00043490  BDISPLAY/xbRender.cpp (inferred) */
void bdSetCameraPosition(float x, float y, float z)
{
	DAT_002be570[0] = x;
	DAT_002be570[1] = y;
	DAT_002be570[2] = z;
}

/* 0x000434b0  BDISPLAY/xbRender.cpp (inferred): the camera rotation from
three angles, in one of six orders (bKernel's rotation builders) */
void bdSetCameraAngles(float x, float y, float z, int order)
{
	switch (order)
	{
	case 0:
		FUN_00050dd0(DAT_002be330, -x, -y, -z);
		return;
	case 1:
		FUN_00050c70(DAT_002be330, -x, -y, -z);
		return;
	case 2:
		FUN_00050d20(DAT_002be330, -x, -y, -z);
		return;
	case 3:
		FUN_00050af0(DAT_002be330, -x, -y, -z);
		return;
	case 4:
		FUN_00050ba0(DAT_002be330, -x, -y, -z);
		return;
	case 5:
		FUN_00050a20(DAT_002be330, -x, -y, -z);
		return;
	default:
		return;
	}
}

/* 0x000435f0  BDISPLAY/xbRender.cpp (inferred): the camera at eye looking
at target, rolled by roll */
void bdSetCameraLookAt(float *eye, float *target, float roll)
{
	float dx, dy, dz, yaw, pitch, distance;

	dx = eye[0] - target[0];
	dy = eye[1] - target[1];
	dz = eye[2] - target[2];
	if (dx == 0.0f)
		yaw = dz < 0.0f ? -HALF_PI : HALF_PI;
	else
		yaw = atan2f(dz, dx);
	yaw = yaw + HALF_PI;
	distance = sqrtf(dz * dz + dx * dx);
	if (distance == 0.0f)
		pitch = dy < 0.0f ? -HALF_PI : HALF_PI;
	else
		pitch = atan2f(dy / distance, 1.0f);
	DAT_002be570[0] = eye[0];
	DAT_002be570[1] = eye[1];
	DAT_002be570[2] = eye[2];
	FUN_00050d20(DAT_002be330, -pitch, yaw, -roll);
}

/* 0x000436e0  BDISPLAY/xbRender.cpp (inferred): the camera at distance
from target, at angles pitch and yaw, looking at it */
void bdSetCameraOrbit(float *target, float pitch, float yaw, float distance)
{
	float c1, s1, c2, s2, t;

	c1 = cosf(pitch);
	s1 = sinf(pitch);
	t = s1 * 0.0f + c1 * -distance;
	c2 = cosf(yaw);
	s2 = sinf(yaw);
	DAT_002be570[0] = (c2 * 0.0f + t * s2) + target[0];
	DAT_002be570[1] = (c1 * 0.0f - s1 * -distance) + target[1];
	DAT_002be570[2] = (c2 * t - s2 * 0.0f) + target[2];
	FUN_00050d20(DAT_002be330, -pitch, -yaw, 0.0f);
}

/* 0x00043790  BDISPLAY/xbRender.cpp (inferred): rebuilds the view matrix
from the camera rotation and position, and the products that use it */
void bdUpdateView(void)
{
	float translation[16];

	FUN_0003c1c0();
	bd_matrix_translation(translation, -DAT_002be570[0], -DAT_002be570[1], -DAT_002be570[2]);
	FUN_0004ff30(DAT_002be270, DAT_002be330, translation);
	if (DAT_002be57c == 1)
		D3DDevice_SetTransform(D3DTS_VIEW, (const D3DMATRIX *)DAT_002be270);
	memcpy(DAT_002be370, DAT_002be270, sizeof(DAT_002be370));
	if (DAT_002be5cc == 0)
		bd_matrix_multiply(DAT_002be430, DAT_002be370, DAT_002be3f0);
	else
		memcpy(DAT_002be430, DAT_002be270, sizeof(DAT_002be430));
	update_world_view_projection();
}

/* 0x000438b0  BDISPLAY/xbRender.cpp (inferred) */
void bdSetWorldIdentity(void)
{
	if (DAT_002be5cc != 0)
		return;
	D3DDevice_SetTransform(D3DTS_WORLD, (const D3DMATRIX *)DAT_002855f0);
	memcpy(DAT_002be430, DAT_002be370, sizeof(DAT_002be430));
	bd_matrix_multiply(DAT_002be470, PTR_DAT_00284c70, DAT_002be370);
	DAT_002be5c4 = 0;
	DAT_002be5cc = 1;
}

/* 0x00043930  BDISPLAY/xbRender.cpp (inferred) */
void bdSetWorldMatrix(float *world)
{
	D3DDevice_SetTransform(D3DTS_WORLD, (const D3DMATRIX *)world);
	memcpy(DAT_002be3f0, world, sizeof(DAT_002be3f0));
	bd_matrix_multiply(DAT_002be430, DAT_002be370, world);
	bd_matrix_multiply(DAT_002be470, PTR_DAT_00284c70, DAT_002be430);
	DAT_002be5c4 = 0;
	DAT_002be5cc = 0;
}

/* 0x000439d0  BDISPLAY/xbRender.cpp (inferred): the 2D projection (one
unit per pixel, origin at the screen centre) */
void bdSetup2DProjection(void)
{
	DAT_002be2f0[1] = 0.0f;
	DAT_002be2f0[2] = 0.0f;
	DAT_002be2f0[3] = 0.0f;
	DAT_002be2f0[4] = 0.0f;
	DAT_002be2f0[6] = 0.0f;
	DAT_002be2f0[7] = 0.0f;
	DAT_002be2f0[8] = 0.0f;
	DAT_002be2f0[9] = 0.0f;
	DAT_002be2f0[10] = 1.0f;
	DAT_002be2f0[11] = 0.0f;
	DAT_002be2f0[12] = 0.0f;
	DAT_002be2f0[13] = 0.0f;
	DAT_002be2f0[14] = DAT_002be5b0;
	DAT_002be2f0[15] = 1.0f;
	DAT_002be2f0[0] = 1.0f / (float)DAT_002bc414;
	DAT_002be2f0[5] = 1.0f / (float)DAT_002bc418;
}

/* 0x00043a90  BDISPLAY/xbRender.cpp (inferred): the perspective projection
from the fields of view and planes, inside the clip window */
void bdSetupProjection(void)
{
	float perspective[16];
	float q = DAT_002be584 / (DAT_002be584 - DAT_002be580);

	memset(perspective, 0, sizeof(perspective));
	perspective[0] = 1.0f / (tanf(DAT_002be588 * 0.5f) * screen_widening());
	perspective[5] = 1.0f / tanf(DAT_002be58c * 0.5f);
	perspective[10] = q;
	perspective[11] = 1.0f;
	perspective[14] = -(DAT_002be580 * q) + DAT_002be5b0;
	FUN_0004ff30(DAT_002be2b0, DAT_002be530, perspective);
}

/* 0x00043b70  BDISPLAY/xbRender.cpp (inferred): 1 perspective, 0 2D */
void bdSetProjectionMode(int mode)
{
	if (DAT_002be57c == mode)
		return;
	DAT_002be57c = mode;
	if (mode == 1)
	{
		D3DDevice_SetTransform(D3DTS_VIEW, (const D3DMATRIX *)DAT_002be270);
		D3DDevice_SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)DAT_002be2b0);
		PTR_DAT_00284c70 = DAT_002be2b0;
	}
	else
	{
		D3DDevice_SetTransform(D3DTS_VIEW, (const D3DMATRIX *)DAT_00284c30);
		D3DDevice_SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)DAT_002be2f0);
		PTR_DAT_00284c70 = DAT_002be2f0;
	}
	update_world_view_projection();
}

/* 0x00043c10  BDISPLAY/xbRender.cpp (inferred): projects count points
(4 floats each) to x, y relative to the viewport centre (y up), depth,
and 1/w */
void bdProjectPoints(float *out, float *in, int count)
{
	float x, y;

	if (DAT_002be5c4 == 0)
	{
		bd_matrix_multiply(DAT_002be4b0, DAT_002be4f0, DAT_002be470);
		DAT_002be5c4 = 1;
	}
	for (; count != 0; count--)
	{
		FUN_000505c0(out, DAT_002be4b0, in);
		if (out[3] == 0.0f)
			out[3] = 1.0f;
		else
			out[3] = 1.0f / out[3];
		x = out[0];
		in += 4;
		out[0] = out[3] * x;
		y = out[1];
		out[1] = out[3] * y;
		out[2] = out[3] * out[2];
		out[0] = out[3] * x - DAT_002be5a0;
		out[1] = DAT_002be5a4 - out[3] * y;
		out += 4;
	}
}

/* 0x00043cf0  BDISPLAY/xbRender.cpp (inferred): transforms by the view
matrix; as in the original, each step moves one float (4 bytes), not one
vector */
void bdTransformPoints(float *out, float *in, int count)
{
	for (; count != 0; count--)
	{
		FUN_000505c0(out, DAT_002be370, in);
		out = (float *)((BYTE *)out + 4);
		in = (float *)((BYTE *)in + 4);
	}
}

/* the clip outcode of a clip-space point */
static BYTE clip_outcode(const float *p)
{
	BYTE code = 0;

	if (p[0] <= -p[3])
		code |= 1;
	if (p[3] <= p[0])
		code |= 2;
	if (p[1] <= -p[3])
		code |= 4;
	if (p[3] <= p[1])
		code |= 8;
	if (p[2] <= 0.0f)
		code |= 0x10;
	if (p[3] <= p[2])
		code |= 0x20;
	return code;
}

/* 0x00043d20  BDISPLAY/xbRender.cpp (inferred): the box (x0-x1, y0-y1,
z0-z1) under matrix against the view: 0 outside, 1 inside, 2 partly */
char bdBoxVisible(float *matrix, float x0, float x1, float y0, float y1, float z0, float z1)
{
	float m[16];
	float corner[4];
	float clip[8][4];
	BYTE codes[8], all, any;
	int i;

	if (DAT_002be5c8 == 0)
	{
		bd_matrix_multiply(DAT_002be3b0, PTR_DAT_00284c70, DAT_002be370);
		DAT_002be5c8 = 1;
	}
	bd_matrix_multiply(m, DAT_002be3b0, matrix);
	corner[0] = x0;
	corner[1] = y0;
	corner[2] = z0;
	corner[3] = 1.0f;
	FUN_000505c0(clip[0], m, corner);
	corner[2] = z1;
	FUN_000505c0(clip[1], m, corner);
	corner[1] = y1;
	FUN_000505c0(clip[2], m, corner);
	corner[2] = z0;
	FUN_000505c0(clip[3], m, corner);
	corner[0] = x1;
	corner[1] = y0;
	corner[2] = z0;
	FUN_000505c0(clip[4], m, corner);
	corner[2] = z1;
	FUN_000505c0(clip[5], m, corner);
	corner[1] = y1;
	FUN_000505c0(clip[6], m, corner);
	corner[2] = z0;
	FUN_000505c0(clip[7], m, corner);
	all = 0xff;
	any = 0;
	for (i = 0; i < 8; i++)
	{
		codes[i] = clip_outcode(clip[i]);
		all &= codes[i];
		any |= codes[i];
	}
	if (all != 0)
		return 0;
	return (char)((any != 0) + 1);
}

/* 0x00044320  BDISPLAY/xbRender.cpp (inferred): whether a sphere at pos
(w set to 1) of radius may be in view */
int bdSphereVisible(float *pos, float radius)
{
	float view[4];

	pos[3] = 1.0f;
	FUN_000505c0(view, DAT_002be430, pos);
	if (DAT_002be580 - radius <= view[2] && view[2] <= DAT_002be584 + radius)
	{
		if (-view[2] <= DAT_002be2b0[0] * radius + DAT_002be2b0[0] * view[0] &&
			DAT_002be2b0[0] * view[0] - DAT_002be2b0[0] * radius <= view[2] &&
			-view[2] <= DAT_002be2b0[5] * radius + DAT_002be2b0[5] * view[1] &&
			DAT_002be2b0[5] * view[1] - DAT_002be2b0[5] * radius <= view[2])
			return 1;
	}
	return 0;
}

/* the view depth of pos (world * view, z / w) */
static float view_depth(const float *pos)
{
	return (DAT_002be430[10] * pos[2] + DAT_002be430[14] * pos[3] + DAT_002be430[6] * pos[1] +
		DAT_002be430[2] * pos[0]) /
		(DAT_002be430[11] * pos[2] + DAT_002be430[15] * pos[3] + DAT_002be430[7] * pos[1] +
		DAT_002be430[3] * pos[0]);
}

/* 0x000443f0  BDISPLAY/xbRender.cpp (inferred): the size in pixels of
width x height at pos; returns the depth, 0 outside the planes */
float bdProjectSize(float *pos, float width, float height, float *outWidth, float *outHeight)
{
	float z = view_depth(pos);

	/* as in the original: a NaN depth fails the far test (returns 0) */
	if (!(z <= DAT_002be580) && z < DAT_002be584)
	{
		*outWidth = DAT_002be2b0[0] * width * (1.0f / z) * DAT_002be598;
		*outHeight = DAT_002be2b0[5] * height * (1.0f / z) * DAT_002be59c;
		return z;
	}
	return 0.0f;
}

/* 0x000444b0  BDISPLAY/xbRender.cpp (inferred): bdProjectSize for a width only */
float bdProjectWidth(float *pos, float width, float *outWidth)
{
	float z = view_depth(pos);

	/* as in the original: a NaN depth fails the far test (returns 0) */
	if (!(z <= DAT_002be580) && z < DAT_002be584)
	{
		*outWidth = ((DAT_002be2b0[0] * width) / z) * DAT_002be598;
		return z;
	}
	return 0.0f;
}

/* 0x00044550  BDISPLAY/xbRender.cpp (inferred): the depth offset added to
both projections */
void bdSetDepthOffset(float offset)
{
	float change = offset - DAT_002be5b0;

	DAT_002be2f0[14] = DAT_002be2f0[14] + change;
	DAT_002be5b0 = offset;
	DAT_002be2b0[14] = change + DAT_002be2b0[14];
	D3DDevice_SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)PTR_DAT_00284c70);
	update_world_view_projection();
}

/* 0x000445e0  BDISPLAY/xbRender.cpp (inferred): the clip window inside
the viewport (clamped to it): the D3D viewport, and a matrix that maps the
viewport's projection onto the window */
void bdSetClipWindow(int x, int y, int width, int height)
{
	D3DVIEWPORT8 viewport;
	float translation[16], scale[16];
	int limit;
	bdRenderTarget *target;

	limit = (int)DAT_002be5a8;
	if (x < limit)
	{
		width = width + (x - limit);
		x = limit;
	}
	limit = (int)DAT_002be5ac;
	if (y < limit)
	{
		height = height + (y - limit);
		y = limit;
	}
	limit = (int)DAT_002be590;
	if (limit < width)
		width = limit;
	limit = (int)DAT_002be594;
	if (limit < height)
		height = limit;
	target = DAT_002bc464;
	target->clip[0] = x;
	target->clip[1] = y;
	target->clip[2] = width;
	target->clip[3] = height;
	if ((float)target->clip[2] == DAT_002be590 && (float)target->clip[3] == DAT_002be594)
	{
		memcpy(DAT_002be530, DAT_002855f0, sizeof(DAT_002be530));
	}
	else
	{
		bd_matrix_translation(translation,
			(DAT_002be5a0 - ((float)target->clip[2] * 0.5f + (float)target->clip[0])) / DAT_002be598,
			(-1.0f / DAT_002be59c) * (DAT_002be5a4 - ((float)target->clip[3] * 0.5f + (float)target->clip[1])),
			0.0f);
		bd_matrix_scale(scale, DAT_002be590 / (float)width, DAT_002be594 / (float)height, 1.0f);
		bd_matrix_multiply(DAT_002be530, scale, translation);
	}
	viewport.X = x;
	viewport.Y = y;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;
	D3DDevice_SetViewport(&viewport);
	bdSetupProjection();
	bdUpdateView();
}

/* 0x00044810  BDISPLAY/xbRender.cpp (inferred) */
void bdSetNearFar(float nearZ, float farZ)
{
	DAT_002be580 = nearZ;
	DAT_002be584 = farZ;
	bdSetupProjection();
	if (DAT_002be57c == 1)
		D3DDevice_SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)DAT_002be2b0);
}

/* 0x00044840  BDISPLAY/xbRender.cpp (inferred): the viewport of the
render target in use; the clip window becomes the whole viewport */
void bdSetViewport(int x, int y, int width, int height)
{
	D3DVIEWPORT8 viewport;
	float translation[16], scale[16];

	viewport.X = x;
	viewport.Y = y;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;
	DAT_002bc464->viewport[0] = x;
	DAT_002bc464->viewport[1] = y;
	DAT_002bc464->viewport[2] = width;
	DAT_002bc464->viewport[3] = height;
	D3DDevice_SetViewport(&viewport);
	DAT_002be590 = (float)width;
	DAT_002be594 = (float)height;
	DAT_002be598 = (float)(width / 2);
	DAT_002be59c = (float)(height / 2);
	DAT_002be5a8 = (float)x;
	DAT_002be5ac = (float)y;
	DAT_002be5a0 = DAT_002be5a8 + DAT_002be598;
	DAT_002be5a4 = DAT_002be5ac + DAT_002be59c;
	bd_matrix_scale(scale, DAT_002be598, -DAT_002be59c, 1.0f);
	bd_matrix_translation(translation, DAT_002be5a8 + DAT_002be598, DAT_002be5ac + DAT_002be59c, 0.0f);
	bd_matrix_multiply(DAT_002be4f0, translation, scale);
	DAT_002be5c4 = 0;
	bdSetClipWindow(x, y, width, height);
}

/* 0x00044a00  BDISPLAY/xbRender.cpp (inferred): fields of view in
radians; fovY 0 derives it from fovX and the viewport's aspect */
void bdSetFieldOfView(float fovX, float fovY)
{
	float fov;

	DAT_002be588 = fovX;
	if (fovY == 0.0f)
	{
		/* (port: the aspect of the viewport as 4:3 had it: screen_widening) */
		fov = atan2f((DAT_002be594 / DAT_002be590) * screen_widening() * tanf(fovX * 0.5f), 1.0f);
		fov = fov + fov;
	}
	else
	{
		fov = fovY;
	}
	DAT_002be58c = fov;
	bdSetupProjection();
	if (DAT_002be57c == 1)
		D3DDevice_SetTransform(D3DTS_PROJECTION, (const D3DMATRIX *)DAT_002be2b0);
}

/* 0x00044a70  BDISPLAY/xbRender.cpp (inferred) */
void bdRenderTargetGetInfo(bdRenderTarget *target, int *width, int *height, int *colourDepth, int *zDepth)
{
	if (width != NULL)
		*width = target->width;
	if (height != NULL)
		*height = target->height;
	if (colourDepth != NULL)
		*colourDepth = target->colourDepth;
	if (zDepth != NULL)
		*zDepth = target->zDepth;
}

/* 0x00044ac0  BDISPLAY/xbRender.cpp (inferred): while a state is locked,
bdSetRenderState ignores it */
int bdLockRenderState(int state)
{
	return ++DAT_002bee44[state];
}

/* 0x00044ae0  BDISPLAY/xbRender.cpp (inferred) */
int bdUnlockRenderState(int state)
{
	return --DAT_002bee44[state];
}

/* 0x00044b00  BDISPLAY/xbRender.cpp (inferred): sets a Babel render
state (kept in DAT_002be600): 1 texture filter, 2 texture wrap (bit 0
wraps u, bit 1 wraps v, else clamp), 3 lighting, 4 cull mode, 5 Z write, 6 alpha blending, 7 Z
compare, 8 fog, 9 the clear of bdFlip (no D3D state), 10 blend mode, 11
colour writes, 12 world matrix (0 identity), 13 texture stage enable */
void bdSetRenderState(int state, unsigned int value, unsigned int extra)
{
	int stage;

	if (DAT_002bee44[state] != 0)
		return;
	DAT_002be600[state * 2] = value;
	DAT_002be600[state * 2 + 1] = extra;
	switch (state)
	{
	case 1:
		for (stage = 0; stage < 4; stage++)
		{
			D3DDevice_SetTextureStageState(stage, D3DTSS_MAGFILTER, DAT_00284cf8[value][0]);
			D3DDevice_SetTextureStageState(stage, D3DTSS_MINFILTER, DAT_00284cf8[value][1]);
			D3DDevice_SetTextureStageState(stage, D3DTSS_MIPFILTER, DAT_00284cf8[value][2]);
		}
		return;
	case 2:
		D3DDevice_SetTextureStageState(0, D3DTSS_ADDRESSU, (~(value * 2) & 2) | 1);
		D3DDevice_SetTextureStageState(0, D3DTSS_ADDRESSV, (~value & 2) | 1);
		return;
	case 3:
		D3DDevice_SetRenderState(D3DRS_LIGHTING, value);
		return;
	case 4:
		D3DDevice_SetRenderState_CullMode(DAT_00284cec[value]);
		return;
	case 5:
		D3DDevice_SetRenderState(D3DRS_ZWRITEENABLE, value);
		return;
	case 6:
		D3DDevice_SetRenderState(D3DRS_ALPHABLENDENABLE, value);
		return;
	case 7:
		D3DDevice_SetRenderState(D3DRS_ZFUNC, DAT_00284ccc[value]);
		return;
	case 8:
		D3DDevice_SetRenderState(D3DRS_FOGENABLE, value);
		return;
	case 10:
		D3DDevice_SetRenderState(D3DRS_BLENDOP, DAT_00284c78[value][2]);
		D3DDevice_SetRenderState(D3DRS_SRCBLEND, DAT_00284c78[value][0]);
		D3DDevice_SetRenderState(D3DRS_DESTBLEND, DAT_00284c78[value][1]);
		return;
	case 11:
		if (value == 0)
		{
			D3DDevice_SetRenderState(D3DRS_COLORWRITEENABLE, 0);
			D3DDevice_SetTransform(D3DTS_WORLD, (const D3DMATRIX *)DAT_002855f0);
			return;
		}
		D3DDevice_SetRenderState(D3DRS_COLORWRITEENABLE, 0x1010101);
		/* fall through: as state 12 */
	case 12:
		if (value != 0 && DAT_002be5cc == 0)
		{
			D3DDevice_SetTransform(D3DTS_WORLD, (const D3DMATRIX *)DAT_002be3f0);
			return;
		}
		D3DDevice_SetTransform(D3DTS_WORLD, (const D3DMATRIX *)DAT_002855f0);
		return;
	case 13:
		DAT_002bee34[value] = (int)extra;
		return;
	default:
		return;
	}
}

/* 0x00044dd0  BDISPLAY/xbRender.cpp (inferred) */
void bdSetFogRange(float start, float end)
{
	D3DDevice_SetRenderState(D3DRS_FOGSTART, bd_float_bits(start));
	D3DDevice_SetRenderState(D3DRS_FOGEND, bd_float_bits(end));
	DAT_002be5e4 = start;
	DAT_002be5e8 = end;
	DAT_002be5ec = 1.0f / (end - start);
}

/* 0x00044e20  BDISPLAY/xbRender.cpp (inferred) */
void bdSetFogColour(unsigned int r, unsigned int g, unsigned int b)
{
	D3DDevice_SetRenderState_FogColor((((r | 0xffffff00u) << 8 | (g & 0xff)) << 8) | (b & 0xff));
	DAT_002be5f4 = g;
	DAT_002be5f0 = r;
	DAT_002be5f8 = b;
}

/* 0x00044e70  BDISPLAY/xbRender.cpp (inferred): the texture factor's
alpha (256 means 255) */
void bdSetTextureFactorAlpha(int alpha)
{
	if (alpha == 0x100)
		alpha = 0xff;
	D3DDevice_SetRenderState_TextureFactor((DWORD)alpha << 24);
}

/* 0x00044e90  BDISPLAY/xbRender.cpp: a render target. Without
BDRT_PRIVATEZ it is 32-bit and uses the screen's Z buffer (so it is
clamped to the screen size); with it, colour and Z depths are 16 or 32
(zDepth 0: no Z buffer; colour depth 8 is an 8-bit luminance target).
Returns NULL when out of memory. */
bdRenderTarget *bdCreateRenderTarget(int width, int height, int colourDepth, int zDepth, unsigned int flags)
{
	bdRenderTarget *target;
	BYTE *colourMemory, *zMemory = NULL;
	unsigned int size;
	D3DFORMAT format;
	DWORD zFormat;
	int pitch;

	target = (bdRenderTarget *)bdAlloc(0x110, RENDER_FILE, 0x12e, 0x2001, "Render Target Header", 0);
	if (target == NULL)
		return NULL;
	memset(target, 0, 0x80);
	target->texture.width = (WORD)width;
	target->texture.height = (WORD)height;
	if ((flags & BDRT_PRIVATEZ) == 0)
	{
		zDepth = 0x20;
		colourDepth = 0x20;
		if (DAT_002bc40c < width)
		{
			bdPrintf("bdCreateRenderTarget: *** specified width (%d) exceeds screen width (%d) and cannot be used without a private Z buffer, clamping to %d ***\n",
				width, DAT_002bc40c, DAT_002bc40c);
			width = DAT_002bc40c;
		}
		if (DAT_002bc410 < height)
		{
			bdPrintf("bdCreateRenderTarget: *** specified height (%d) exceeds screen height (%d) and cannot be used without a private Z buffer, clamping to %d ***\n",
				height, DAT_002bc410, DAT_002bc410);
			height = DAT_002bc410;
		}
		size = (unsigned int)(width * height * 4) + 0xfffu & 0xfffff000u;
		colourMemory = (BYTE *)bdAlloc(size, RENDER_FILE, 0x149, 0x2003, "Render Target Texture", 0x1000);
		if (colourMemory == NULL)
			goto out_of_memory;
		if ((flags & BDRT_NOWRITECOMBINE) == 0)
			bdPhysicalProtect(colourMemory, size, 0x404);
		XGSetTextureHeader(width, height, 1, 0, (D3DFORMAT)0x1e, 0, &target->colour, 0, width * 4);
	}
	else
	{
		if (zDepth != 0)
		{
			if (colourDepth != zDepth)
			{
				bdPrintf("bdCreateRenderTarget: *** RGB and Z depths do not match (rgb %d, z %d), forcing both to %d\n",
					colourDepth, zDepth, colourDepth);
				zDepth = colourDepth;
			}
			if (colourDepth == 0x10)
			{
				format = (D3DFORMAT)0x11;
				zFormat = 0x30;
			}
			else
			{
				format = (D3DFORMAT)0x1e;
				zFormat = 0x2e;
			}
			size = (unsigned int)(width * height * colourDepth >> 3) + 0xfffu & 0xfffff000u;
			colourMemory = (BYTE *)bdAlloc(size, RENDER_FILE, 0x169, 0x2003, "Render Target Texture", 0x1000);
			if (colourMemory == NULL)
				goto out_of_memory;
			if ((flags & BDRT_NOWRITECOMBINE) == 0)
				bdPhysicalProtect(colourMemory, size, 0x404);
			pitch = width * colourDepth >> 3;
			XGSetTextureHeader(width, height, 1, 0, format, 0, &target->colour, 0, pitch);
			D3DResource_Register((D3DResource *)&target->colour, colourMemory);
			target->colour.Common |= 0x80000;
			zMemory = (BYTE *)bdAlloc(size, RENDER_FILE, 0x176, 0x2003, "Render Target Z Buffer", 0x1000);
			if (zMemory == NULL)
			{
				bdPrintf("bdCreateRenderTarget: Out of memory\n");
				bdPhysicalProtect(colourMemory, size, 4);
				bdFree(colourMemory);
				bdFree(target);
				return NULL;
			}
			if ((flags & BDRT_NOWRITECOMBINE) == 0)
				bdPhysicalProtect(zMemory, size, 0x404);
			target->zSurface.Data = 0;
			target->zSurface.Lock = 0;
			target->zSurface.Format = zFormat << 8 | 0x1002a;
			target->zSurface.Size = ((unsigned int)((pitch / 64 - 1) * 0x1000) | (unsigned int)(height - 1)) << 12 |
				(unsigned int)(width - 1);
			target->zSurface.Common = 0xd0001;
			target->zSurface.Parent = NULL;
			D3DResource_Register((D3DResource *)&target->zSurface, zMemory);
			goto registered;
		}
		if (colourDepth == 0x10)
			format = (D3DFORMAT)0x11;
		else
			format = (D3DFORMAT)(colourDepth == 8 ? 0x13 : 0x1e);
		size = (unsigned int)(width * height * colourDepth >> 3) + 0xfffu & 0xfffff000u;
		colourMemory = (BYTE *)bdAlloc(size, RENDER_FILE, 0x195, 0x2003, "Render Target Texture", 0x1000);
		if (colourMemory == NULL)
			goto out_of_memory;
		if ((flags & BDRT_NOWRITECOMBINE) == 0)
			bdPhysicalProtect(colourMemory, size, 0x404);
		XGSetTextureHeader(width, height, 1, 0, format, 0, &target->colour, 0, width * colourDepth >> 3);
	}
	D3DResource_Register((D3DResource *)&target->colour, colourMemory);
	target->colour.Common |= 0x80000;
registered:
	D3DTexture_GetSurfaceLevel(&target->colour, 0, &target->colourSurface);
	target->zMemory = zMemory;
	target->colourMemory = colourMemory;
	target->colourDepth = colourDepth;
	target->memorySize = (int)size;
	target->viewport[1] = 0;
	target->viewport[0] = 0;
	target->clip[1] = 0;
	target->clip[0] = 0;
	target->zDepth = zDepth;
	target->texture.current = &target->colour;
	target->texture.frames = &target->frame;
	target->width = width;
	target->viewport[2] = width;
	target->height = height;
	target->viewport[3] = height;
	target->flags = (int)flags;
	target->clip[2] = width;
	target->clip[3] = height;
	target->prev = &DAT_002bcc98;
	memcpy(&target->frame.d3d, &target->colour, sizeof(D3DTexture));
	target->next = DAT_002bcc98.next;
	DAT_002bcc98.next->prev = target;
	target->prev->next = target;
	return target;

out_of_memory:
	bdPrintf("bdCreateRenderTarget: Out of memory\n");
	bdFree(target);
	return NULL;
}

/* 0x00045330  BDISPLAY/xbRender.cpp (inferred): draws into target (NULL:
the screen) and clears it: flags 1 colour (r, g, b, a), 2 Z (to z) and
stencil. An 8-bit target is cleared by drawing a quad of b. Returns 1. */
int bdSetRenderTarget(bdRenderTarget *target, unsigned int r, unsigned int g, unsigned int b, int a, float z,
	unsigned int flags)
{
	DWORD colour;

	if (target == NULL)
	{
		D3DDevice_SetRenderTarget((D3DSurface *)DAT_002bc480, (D3DSurface *)DAT_002bc484);
		target = &DAT_002bcc98;
	}
	else if (target->zMemory == NULL)
	{
		D3DDevice_SetRenderTarget(target->colourSurface,
			target->zDepth != 0 ? (D3DSurface *)DAT_002bc484 : NULL);
	}
	else
	{
		D3DDevice_SetRenderTarget(target->colourSurface, &target->zSurface);
	}
	DAT_002bc464 = target;
	bdSetViewport(target->viewport[0], target->viewport[1], target->viewport[2], target->viewport[3]);
	bdSetClipWindow(target->clip[0], target->clip[1], target->clip[2], target->clip[3]);
	if (target->colourDepth == 8)
	{
		if (DAT_00284d40 != 0)
		{
			DAT_002bee90[3].z = 0.1f;
			DAT_002bee90[2].z = 0.1f;
			DAT_002bee90[1].z = 0.1f;
			DAT_002bee90[0].z = 0.1f;
			DAT_002bee90[0].rhw = 1.0f;
			DAT_002bee90[2].x = 0.0f;
			DAT_002bee90[1].y = 0.0f;
			DAT_002bee90[0].y = 0.0f;
			DAT_002bee90[0].x = 0.0f;
			DAT_00284d40 = 0;
		}
		DAT_002bee90[0].color = (b & 0xff) | 0xff000000u;
		DAT_002bee90[1].x = (float)target->viewport[2];
		DAT_002bee90[2].y = (float)target->viewport[3];
		DAT_002bee90[3].x = (float)target->viewport[2];
		DAT_002bee90[3].y = (float)target->viewport[3];
		DAT_002bee90[1].color = DAT_002bee90[0].color;
		DAT_002bee90[2].color = DAT_002bee90[0].color;
		DAT_002bee90[3].color = DAT_002bee90[0].color;
		bdDrawPrimitiveTL(BDPRIMTYPE_TRISTRIP, DAT_002bee90, 4);
		return 1;
	}
	colour = ((((r & 0xff) | (unsigned int)a << 8) << 8 | (g & 0xff)) << 8) | (b & 0xff);
	if ((flags & 1) != 0 && (flags & 2) != 0)
	{
		D3DDevice_Clear(0, NULL, 0xf3, colour, z, 0);
		return 1;
	}
	if ((flags & 1) != 0)
	{
		D3DDevice_Clear(0, NULL, 0xf0, colour, z, 0);
		return 1;
	}
	if ((flags & 2) != 0)
		D3DDevice_Clear(0, NULL, 3, 0, z, 0);
	return 1;
}

/* 0x000455a0  BDISPLAY/xbRender.cpp (inferred): selects a render target
as the texture of a stage; its texture matrix scales the coordinates to
texels (the target is a linear texture) */
void bdSetRenderTargetTexture(bdRenderTarget *target, int stage)
{
	float matrix[16];

	if (DAT_002be5d4[stage] == NULL && stage == 0)
	{
		D3DDevice_SetTextureStageState(0, D3DTSS_COLOROP, 5);
		D3DDevice_SetTextureStageState(0, D3DTSS_ALPHAOP, 5);
	}
	if ((target->flags & BDRT_NOWRITECOMBINE) != 0)
		FUN_0004e8f0();
	DAT_002be5d4[stage] = (bdTexture *)target;
	D3DDevice_SetTexture(stage, (D3DBaseTexture *)&target->colour);
	memset(matrix, 0, sizeof(matrix));
	matrix[5] = (float)target->height;
	matrix[10] = 1.0f;
	matrix[15] = 1.0f;
	matrix[0] = (float)target->width;
	D3DDevice_SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, 2);
	D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)(stage + D3DTS_TEXTURE0), (const D3DMATRIX *)matrix);
	DAT_002bc444[stage] = 1;
}

/* 0x00045660  BDISPLAY/xbRender.cpp (inferred): the default render state.
The original clears 0x002be5d0-0x002bee90 in one loop first (so the lock
tests below always pass). */
void bdResetRenderStates(void)
{
	int i;

	DAT_002be5d0 = 0;
	memset(DAT_002be5d4, 0, sizeof(DAT_002be5d4));
	DAT_002be5e4 = 0.0f;
	DAT_002be5e8 = 0.0f;
	DAT_002be5ec = 0.0f;
	DAT_002be5f0 = 0;
	DAT_002be5f4 = 0;
	DAT_002be5f8 = 0;
	memset(DAT_002be600, 0, sizeof(DAT_002be600));
	memset(DAT_002be678, 0, sizeof(DAT_002be678));
	memset(DAT_002bedf8, 0, sizeof(DAT_002bedf8));
	memset(DAT_002bee34, 0, sizeof(DAT_002bee34));
	memset(DAT_002bee44, 0, sizeof(DAT_002bee44));
	DAT_002bee80 = 0;
	DAT_002bee84 = 0;

	DAT_002be5e4 = 10.0f;
	DAT_002be5e8 = 10000.0f;
	DAT_002be5ec = bd_bits_float(0x38d1ecd5); /* 1 / 9990 */
	if (DAT_002bee44[0] == 0)
	{
		DAT_002be600[0] = 0;
		DAT_002be600[1] = 0;
	}
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(1, 2, 0);
	if (DAT_002bee4c == 0)
	{
		DAT_002be600[4] = 3;
		DAT_002be600[5] = 0;
		D3DDevice_SetTextureStageState(0, D3DTSS_ADDRESSU, 1);
		D3DDevice_SetTextureStageState(0, D3DTSS_ADDRESSV, 1);
	}
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(7, 3, 0);
	if (DAT_002bee68 == 0)
	{
		DAT_002be600[18] = 0xff;
		DAT_002be600[19] = 0;
	}
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(8, 0, 0);
	bdSetRenderState(11, 1, 0);
	i = 0;
	do
	{
		bdSetRenderState(13, i, 1);
		i++;
	} while (i < 4);
	D3DDevice_SetRenderState(D3DRS_COLORVERTEX, 1);
	D3DDevice_SetRenderState(D3DRS_DITHERENABLE, 1);
	D3DDevice_SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, 1);
	D3DDevice_SetRenderState(D3DRS_SPECULARMATERIALSOURCE, 0);
	D3DDevice_SetRenderState(D3DRS_ALPHATESTENABLE, 1);
	D3DDevice_SetRenderState(D3DRS_ALPHAFUNC, 0x205);
	D3DDevice_SetRenderState(D3DRS_ALPHAREF, 0);
	D3DDevice_SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
	D3DDevice_SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
	D3DDevice_SetTextureStageState(0, D3DTSS_ALPHAARG2, 0);
	D3DDevice_SetTextureState_TexCoordIndex(0, 0);
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	D3DDevice_SetRenderState(D3DRS_FOGTABLEMODE, 3);
	D3DDevice_SetRenderState(D3DRS_POINTSIZE_MIN, bd_float_bits(0.0f));
	D3DDevice_SetRenderState(D3DRS_POINTSCALE_A, bd_float_bits(0.0f));
	D3DDevice_SetRenderState(D3DRS_POINTSCALE_B, bd_float_bits(0.0f));
	D3DDevice_SetRenderState(D3DRS_POINTSCALE_C, bd_float_bits(1.0f));
}

/* 0x000458a0  BDISPLAY/xbRender.cpp (inferred): deletes a render target
(NULL: all of them) */
void bdDeleteRenderTarget(bdRenderTarget *target)
{
	int stage;

	if (target == NULL)
	{
		while (DAT_002bcc98.prev != &DAT_002bcc98)
			bdDeleteRenderTarget(DAT_002bcc98.prev);
		return;
	}
	if (DAT_002bc464 == target)
		bdSetRenderTarget(NULL, 0, 0, 0, 0, 1.0f, 0);
	stage = 0;
	do
	{
		if (DAT_002be5d4[stage] == (bdTexture *)target)
			bdSetTexture(stage, NULL);
		stage++;
	} while (stage < 4);
	target->prev->next = target->next;
	target->next->prev = target->prev;
	if (target->colourMemory != NULL)
	{
		bdPhysicalProtect(target->colourMemory, target->memorySize, 4);
		bdFree(target->colourMemory);
		target->colourMemory = NULL;
	}
	if (target->zMemory != NULL)
	{
		bdPhysicalProtect(target->zMemory, target->memorySize, 4);
		bdFree(target->zMemory);
		target->zMemory = NULL;
	}
	bdFree(target);
}
