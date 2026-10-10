/*
CharLib/unplaced.c  (charlib)

CharLib functions whose original source file is not known: a vector setter at the very
start of the CharLib block (used by other modules) and the angle-turning helper between
Route.cpp and Actor.cpp.
*/

#include "charlib.h"

/* 0x000ab4c0  CharLib (file unknown)
Sets the float[4] at +0x48 of an object (called by other modules with their own objects;
the class is not known). */
void Obj_SetVector48(void *self, float *v)
{
	CHL_F32(self, 0x48) = v[0];
	CHL_F32(self, 0x4c) = v[1];
	CHL_F32(self, 0x50) = v[2];
	CHL_F32(self, 0x54) = v[3];
}

/* 0x000add50  CharLib (file unknown)
Turns an angle towards `target` by at most rate x frame time per frame, and at most the
remaining distance / smoothing; snaps when within 0.01. The result is wrapped to
[0, 2pi). It turns the short way round. */
void CLMath_TurnTowards(float *angle, float target, float rate, float smoothing)
{
	float d = FUN_00070670(*angle, target);
	float step;
	float k;

	if ((double)d <= 0.01) {
		*angle = target;
		*angle = FUN_000703a0(target);
		return;
	}
	if (target <= *angle && *angle <= target) {
		*angle = FUN_000703a0(*angle);
		return;
	}
	k = 1.0f / smoothing;
	step = chl_time_scale * rate;
	if ((*angle < target && target - *angle <= 3.1415925f) || (target < *angle && 3.1415925f < *angle - target)) {
		/* increase */
		if (step < FUN_00070670(*angle, target) * k)
			*angle = chl_time_scale * rate + *angle;
		else
			*angle = FUN_00070670(*angle, target) * (1.0f / smoothing) + *angle;
	} else {
		/* decrease */
		if (step < FUN_00070670(*angle, target) * k)
			*angle = *angle - chl_time_scale * rate;
		else
			*angle = *angle - FUN_00070670(*angle, target) * (1.0f / smoothing);
	}
	*angle = FUN_000703a0(*angle);
}
