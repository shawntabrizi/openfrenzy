/*
BACTOR/camera.c  <-  C:\Babel\Xbox\Src\BACTOR\camera.cpp

Cameras: an eye and a target point (each a fixed position, a pointer to a
position, a point on a node of an actor instance, angles, or a callback),
a field of view and near/far planes. baUpdateCamera hands the current
camera to bDisplay.

0x0003c000 is tagged with camera.cpp; the functions around it are placed
here by address order (file inferred).
*/

#include <stdarg.h>

#include "bactor.h"
#include "bactor_extern.h"

/* 0x002bc2c0: the head of the camera list (next at 0x58, prev at 0x5c) */
struct BCamera DAT_002bc2c0;
/* 0x002bc320: the current camera */
struct BCamera *DAT_002bc320;

/* 0x0003bef0  BACTOR/camera.cpp (file inferred) */
int baInitCameras(void)
{
	DAT_002bc2c0.prev = &DAT_002bc2c0;
	DAT_002bc2c0.next = &DAT_002bc2c0;
	return 1;
}

/* 0x0003bf10  BACTOR/camera.cpp (file inferred)
makes a camera current; returns the previous one */
struct BCamera *baSetCurrentCamera(struct BCamera *camera)
{
	struct BCamera *previous = DAT_002bc320;

	DAT_002bc320 = camera;
	return previous;
}

/* 0x0003bf20  BACTOR/camera.cpp (file inferred)
reads a camera point from variadic arguments: the type, then
  BCAM_POS, BCAM_QUAT: 4 doubles
  BCAM_POSPTR: a float pointer, a double
  BCAM_NODE: a matrix pointer (an instance), a node pointer (or NULL), 4 doubles
  BCAM_EULER: 3 doubles, an int
  BCAM_CALLBACK: a function pointer, a value.
args points to a va_list (the original takes the argument pointer in eax and
returns the advanced pointer through esi). */
void bReadCameraPoint(struct BCameraPoint *point, void *args)
{
	va_list *ap = (va_list *)args;
	int type = va_arg(*ap, int);
	void *p;

	point->type = type;
	switch (type)
	{
	case BCAM_POS:
	case BCAM_QUAT:
		point->v[0] = (float)va_arg(*ap, double);
		point->v[1] = (float)va_arg(*ap, double);
		point->v[2] = (float)va_arg(*ap, double);
		point->v[3] = (float)va_arg(*ap, double);
		break;
	case BCAM_POSPTR:
		p = va_arg(*ap, void *);
		*(void **)&point->v[0] = p;
		point->v[1] = (float)va_arg(*ap, double);
		break;
	case BCAM_NODE:
		p = va_arg(*ap, void *);
		*(void **)&point->v[0] = p;
		p = va_arg(*ap, void *);
		*(void **)&point->v[1] = p;
		point->v[2] = (float)va_arg(*ap, double);
		point->v[3] = (float)va_arg(*ap, double);
		point->v[4] = (float)va_arg(*ap, double);
		point->v[5] = (float)va_arg(*ap, double);
		break;
	case BCAM_EULER:
		point->v[0] = (float)va_arg(*ap, double);
		point->v[1] = (float)va_arg(*ap, double);
		point->v[2] = (float)va_arg(*ap, double);
		*(int *)&point->v[3] = va_arg(*ap, int);
		break;
	case BCAM_CALLBACK:
		p = va_arg(*ap, void *);
		*(void **)&point->v[0] = p;
		p = va_arg(*ap, void *);
		*(void **)&point->v[1] = p;
		break;
	}
}

/* 0x0003c000  BACTOR/camera.cpp
creates a camera (allocated when camera is NULL); the eye and then the target
point follow as variadic arguments (see bReadCameraPoint) */
struct BCamera *baCreateCamera(struct BCamera *camera, uint32_t flags, ...)
{
	va_list ap;

	if (camera == NULL)
	{
		camera = ba_bkAlloc(sizeof(struct BCamera), "C:\\Babel\\Xbox\\Src\\BACTOR\\camera.cpp", 0xb3, 0x2002);
		if (camera == NULL)
			return NULL;
		camera->flags = flags | 1;
	}
	else
	{
		camera->flags = flags;
	}
	va_start(ap, flags);
	bReadCameraPoint(&camera->eye, &ap);
	bReadCameraPoint(&camera->target, &ap);
	va_end(ap);
	camera->fov = DAT_002be588;
	camera->aspect = DAT_002be58c;
	camera->nearZ = DAT_002be580;
	camera->farZ = DAT_002be584;
	camera->next = &DAT_002bc2c0;
	camera->prev = DAT_002bc2c0.prev;
	DAT_002bc2c0.prev = camera;
	camera->prev->next = camera;
	return camera;
}

/* 0x0003c0b0  BACTOR/camera.cpp (file inferred)
sets the target point of a camera (variadic, see bReadCameraPoint) */
void baSetCameraTarget(struct BCamera *camera, ...)
{
	va_list ap;

	va_start(ap, camera);
	bReadCameraPoint(&camera->target, &ap);
	va_end(ap);
}

/* 0x0003c0d0  BACTOR/camera.cpp (file inferred)
evaluates a camera point: out (4 floats are written by BCAM_NODE) and the
extra value (roll).
Original register arguments: esi point, edi out, ebx extra. */
void bGetCameraPoint(struct BCameraPoint *point, float *out, float *extra)
{
	float local[4];
	float offset[4];

	switch (point->type)
	{
	case BCAM_POS:
		out[0] = point->v[0];
		out[1] = point->v[1];
		out[2] = point->v[2];
		*extra = point->v[3];
		return;
	case BCAM_POSPTR:
	{
		const float *p = *(float **)&point->v[0];

		out[0] = p[0];
		out[1] = p[1];
		out[2] = p[2];
		*extra = point->v[1];
		return;
	}
	case BCAM_NODE:
	{
		const float *matrix = *(float **)&point->v[0];
		struct BActorNode *node = *(struct BActorNode **)&point->v[1];

		if (node == NULL)
		{
			local[0] = point->v[2];
			local[1] = point->v[3];
			local[2] = point->v[4];
			local[3] = 1.0f;
		}
		else
		{
			offset[0] = point->v[2];
			offset[1] = point->v[3];
			offset[2] = point->v[4];
			offset[3] = 1.0f;
			ba_MatTransform(local, node->world, offset);
		}
		ba_MatTransform(out, matrix, local);
		*extra = point->v[5];
		return;
	}
	case BCAM_CALLBACK:
		(*(void (**)(float *, float *, void *))&point->v[0])(out, extra, *(void **)&point->v[1]);
		return;
	}
}

/* 0x0003c1c0  BACTOR/camera.cpp (file inferred)
sets bDisplay's camera from the current camera; 0 when there is none */
int baUpdateCamera(void)
{
	struct BCamera *camera = DAT_002bc320;
	float eye[4];
	float target[4];
	float extra;

	if (camera == NULL)
		return 0;
	switch (camera->target.type)
	{
	case BCAM_POS:
	case BCAM_POSPTR:
	case BCAM_NODE:
	case BCAM_CALLBACK:
		bGetCameraPoint(&camera->target, target, &extra);
		bGetCameraPoint(&DAT_002bc320->eye, eye, &extra);
		ba_bdSetCameraLookAt(eye, target, extra);
		break;
	case BCAM_QUAT:
		/* the target is a quaternion */
		bGetCameraPoint(&camera->eye, eye, &extra);
		ba_bdSetCameraPos(eye[0], eye[1], eye[2]);
		ba_MatFromQuat(DAT_002be330, DAT_002bc320->target.v);
		break;
	case BCAM_EULER:
		/* the target is three angles and a rotation order */
		bGetCameraPoint(&camera->eye, eye, &extra);
		ba_bdSetCameraPos(eye[0], eye[1], eye[2]);
		ba_bdSetCameraAngles(DAT_002bc320->target.v[0], DAT_002bc320->target.v[1], DAT_002bc320->target.v[2],
			*(int *)&DAT_002bc320->target.v[3]);
		break;
	}
	camera = DAT_002bc320;
	if (camera->fov != DAT_002be588 || camera->aspect != DAT_002be58c)
	{
		ba_bdSetFov(camera->fov, camera->aspect);
		camera = DAT_002bc320;
	}
	if (camera->nearZ != DAT_002be580 || camera->farZ != DAT_002be584)
		ba_bdSetNearFar(camera->nearZ, camera->farZ);
	return 1;
}

/* 0x0003c330  BACTOR/camera.cpp (file inferred)
deletes a camera (NULL: all of them) */
void baDeleteCamera(struct BCamera *camera)
{
	if (camera == NULL)
	{
		while (DAT_002bc2c0.next != &DAT_002bc2c0)
			baDeleteCamera(DAT_002bc2c0.next);
		return;
	}
	if (camera == DAT_002bc320)
		DAT_002bc320 = NULL;
	camera->next->prev = camera->prev;
	camera->prev->next = camera->next;
	if (camera->flags & 1)
		ba_bkFree(camera);
}

/* 0x0003c390  BACTOR/camera.cpp (file inferred) */
void baDeleteAllCameras(void)
{
	while (DAT_002bc2c0.next != &DAT_002bc2c0)
		baDeleteCamera(DAT_002bc2c0.next);
}
