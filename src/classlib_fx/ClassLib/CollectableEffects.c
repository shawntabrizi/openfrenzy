/*
ClassLib/CollectableEffects.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\CollectableEffects.cpp

A burst of turning rays around a collected item: the rays grow to their full length,
hold, then shrink while their colour fades. Only 0x000bf1f0 is tagged with
CollectableEffects.cpp; 0x000bf080 and 0x000bf5b0 work on the same object (file inferred).
*/

#include "classlib_fx.h"

static const char s_CollectFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\CollectableEffects.cpp";

/* 0x000bf080  ClassLib (gap; file inferred: CollectableEffects.cpp)
Turns the rays and places their tips at the current radius. When done (state 3) the rays
are freed first. The direction vector is rotated again for each ray (it is not reset). */
void CCollectEffect_UpdateRays(CCollectEffect *self)
{
	float radius;
	float v[4];
	CCollectRay *ray;
	int n;

	radius = self->radius;
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 1.0f;
	v[3] = 1.0f;
	self->iter = 0;
	if (self->state == 3 && self->rays != NULL) {
		CLF_Free(self->rays);
		self->rays = NULL;
		self->iter = 0;
		self->numRays = 0;
	}
	self->iter = 0;
	if (0 < self->numRays) {
		do {
			ray = &self->rays[self->iter];
			ray->rot[0] = DAT_0028fda8 * 0.00200000009f + ray->rot[0];
			ray->rot[1] = DAT_0028fda8 * 0.00200000009f + ray->rot[1];
			ray->rot[2] = DAT_0028fda8 * 0.00200000009f + ray->rot[2];
			FUN_00051310(v, v, ray->rot[0], ray->rot[1], ray->rot[2]);
			if (self->state != 3) {
				ray->tip[0] = v[0] * radius + ray->base[0];
				ray->tip[1] = v[1] * radius + ray->base[1];
				ray->tip[2] = v[2] * radius + ray->base[2];
			}
			ray->tip[3] = 1.0f;
			n = self->iter;
			self->iter = n + 1;
		} while (n + 1 < self->numRays);
	}
}

/* 0x000bf1f0  ClassLib/CollectableEffects.cpp
Init: `numRays` rays at `pos` with random colours and directions. */
void CCollectEffect_Init(CCollectEffect *self, float *pos, int numRays)
{
	CCollectRay *ray;
	float f;
	int i;

	self->numRays = numRays;
	self->progress = 0.0f;
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->unknown14[0] = 0;
	self->unknown14[1] = 0;
	self->unknown14[2] = 0;
	self->unknown14[3] = 0;
	self->colour[0] = CLF_Rand() % 0x7f;
	self->colour[1] = CLF_Rand() % 0x7f;
	self->colour[2] = CLF_Rand() % 0x7f;
	self->colour[3] = CLF_Rand() % 0x7f;
	self->maxRadius = 200.0f;
	self->radius = 1.0f;
	self->texture = 0;
	self->texture = CLF_FindTexture(0, "projray3.bmp", 0, 0xdefa);
	self->unknown2c = 6.0f;
	self->rays = CLF_Alloc(self->numRays * 0x54, s_CollectFile, 0x50, 0x2001);
	for (i = 0; i < self->numRays; i++) {
		ray = &self->rays[i];
		ray->base[0] = self->pos[0];
		ray->base[1] = self->pos[1];
		ray->base[3] = 1.0f;
		ray->base[2] = self->pos[2];
		ray->tip[0] = ray->base[0];
		ray->tip[1] = ray->base[1];
		ray->tip[2] = ray->base[2];
		ray->tip[3] = ray->base[3];
		/* three numbers are drawn; only the third is used, for all three angles */
		CLF_Rand();
		CLF_Rand();
		f = (float)CLF_Rand() * 0.000191753439f;
		ray->rot[3] = 1.0f;
		ray->rot[1] = f;
		ray->rot[0] = f - 0.314159244f;
		ray->rot[2] = f - 0.314159244f;
		ray->colour[0] = CLF_Rand() % 0x7f;
		ray->colour[1] = CLF_Rand() % 0x7f;
		ray->colour[2] = CLF_Rand() % 0x7f;
		ray->colour[3] = CLF_Rand() % 0x7f;
	}
	CLF_Rand();
	CLF_Rand();
	f = (float)CLF_Rand() * 1.52592554e-06f;
	self->spin[3] = 1.0f;
	self->spin[0] = f - 0.00500000035f;
	self->spin[1] = f - 0.0250000004f;
	self->spin[2] = f - 0.0250000004f;
	CLF_Rand();
	CLF_Rand();
	f = (float)CLF_Rand() * 0.000191753439f;
	self->rot[3] = 1.0f;
	self->rot[2] = f - 0.314159244f;
	self->rot[0] = f - 0.314159244f;
	self->rot[1] = f;
	CCollectEffect_UpdateRays(self);
	self->top[0] = pos[0];
	self->top[1] = pos[1];
	self->top[2] = pos[2];
	self->top[3] = pos[3];
	self->state = 0;
	self->top[1] = self->top[1] + 70.0f;
}

/* 0x000bf5b0  ClassLib (gap; file inferred: CollectableEffects.cpp)
Per-frame update of the state machine (grow, hold, shrink and fade). Returns the
progress (0 to 1). */
float CCollectEffect_Update(CCollectEffect *self)
{
	float inv;
	float r;

	switch (self->state) {
	case 0:
		if (self->radius <= self->maxRadius)
			self->radius = DAT_0028fda8 * 2.5f + self->radius;
		else
			self->state = 1;
		break;
	case 2:
		if (0.0f <= self->radius) {
			r = self->radius - DAT_0028fda8 * 4.5f;
			self->radius = r;
			inv = 1.0f / self->maxRadius;
			self->fadeColour[0] = CLF_FTOL(inv * r * 89.0f);
			self->fadeColour[1] = CLF_FTOL(inv * self->radius * 63.0f);
			self->fadeColour[2] = CLF_FTOL(inv * self->radius * 27.0f);
			self->fadeColour[3] = CLF_FTOL(inv * self->radius * 103.0f);
		} else {
			self->state = 3;
		}
		break;
	case 1:
		if (self->maxRadius < self->radius) {
			if (self->progress < 1.0f) {
				self->progress = DAT_0028fda8 * 0.0500000007f + self->progress;
			} else {
				self->progress = 1.0f;
				self->state = 2;
			}
		}
		break;
	default:
		break;
	}
	CCollectEffect_UpdateRays(self);
	return self->progress;
}
