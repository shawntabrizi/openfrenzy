/*
Rowing/unplaced_particles.c  <-  gap functions at the start of the Rowing range

Three methods of a small particle system class whose vtable (0x00274e3c, rebuilt by
rollerball_b) sits in front of "C:\BlitzParty\Game\Src\RollerBall\CRollerBall_Particle.cpp":
  [0] 0x00104a30  [1] 0x0019d400 Update  [2] 0x0019ce60 Draw  [3] 0x00102d60
  [4] 0x0019d200 Emit  [5] 0x0019d130 deleting destructor
So these functions are probably RollerBall code (CRollerBall_Particle.cpp); the function table
puts them in rowing_a because they lie after the RollerBall range.

The layout is classlib_fx's CPSHeader (particles of classlib_fx's CParticle, 0x68 bytes):
  +0x1c emitter position, +0x40 direction (4 floats), +0x50 active (byte),
  +0x54 speed, +0x58 drag, +0x5c gravity, +0x64 speed scale.
*/

#include "rowing_a.h"


/* 0x0019d200  Rowing (gap; RollerBall/CRollerBall_Particle.cpp inferred)
Emit (vtable slot 4): starts particle P at the emitter with a random life of 16-31 ticks, an
orange colour that fades out and a random velocity around the direction. */
void RbParticle_Emit(void *self, CParticle *p)
{
	int life;
	float scale, vx, vy, vz;

	p->alive = 1;
	p->age = 0;
	life = (rw_rand() & 0xf) + 0x10;
	p->life = (short)life;
	p->colour[0] = 0x400;
	p->colour[1] = 0x200;
	p->colour[2] = 0;
	p->colour2[0] = 0x3000;
	p->colour2[1] = 0x2000;
	p->colour2[2] = 0x1000;
	p->dColour[0] = (short)(-0x400 / life);
	p->dColour[1] = (short)(-0x200 / life);
	p->dColour[2] = 0;
	p->dColour2[0] = (short)(-0x3000 / life);
	p->dColour2[1] = (short)(-0x2000 / life);
	p->dColour2[2] = (short)(-0x1000 / life);
	p->sizeSpeed = 0.0f;
	p->size = (float)((rw_rand() & 1) + 3);
	p->pos[0] = RA_F32(self, 0x1c);
	p->pos[1] = RA_F32(self, 0x20);
	p->pos[2] = RA_F32(self, 0x24);
	p->pos[3] = 1.0f;
	p->prevPos[0] = RA_F32(self, 0x1c);
	p->prevPos[1] = RA_F32(self, 0x20);
	p->prevPos[2] = RA_F32(self, 0x24);
	p->prevPos[3] = 1.0f;
	/* 0x00253740 = 1/32767, 0x00268b28 = 2/32767 */
	vx = (float)rw_rand() * rw_bitsf(0x38000100) + RA_F32(self, 0x40) - 0.5f;
	vy = (float)rw_rand() * rw_bitsf(0x38800100) + RA_F32(self, 0x44) - 0.5f;
	vz = (float)rw_rand() * rw_bitsf(0x38000100) + RA_F32(self, 0x48) - 0.5f;
	scale = RA_F32(self, 0x64) * RA_F32(self, 0x54);
	p->vel[0] = vx * scale;
	p->vel[1] = scale * vy;
	p->vel[2] = scale * vz;
}

/* 0x0019d400  Rowing (gap; RollerBall/CRollerBall_Particle.cpp inferred)
Update (vtable slot 1): moves the live particles by DT, applies the drag (+0x58) and the
gravity (+0x5c), steps the colours and counts the live ones; the system stops (+0x50) when
none is left. */
void RbParticle_Update(void *self, float dt)
{
	CParticle *p;
	short age;
	float k;
	int i;

	RA_I32(self, 0x14) = 0;
	if (RA_U8(self, 0x50) == 0)
		return;
	for (i = 0; i < RA_I32(self, 0x10); i++) {
		p = (CParticle *)((char *)RA_PTRF(self, 0x0c) + i * 0x68);
		if (!p->alive)
			continue;
		age = p->age;
		p->age = age + 1;
		if (age < p->life) {
			p->prevPos[0] = p->pos[0];
			p->prevPos[1] = p->pos[1];
			p->prevPos[2] = p->pos[2];
			p->prevPos[3] = p->pos[3];
			p->pos[0] = dt * p->vel[0] + p->pos[0];
			p->pos[1] = dt * p->vel[1] + p->pos[1];
			p->pos[2] = dt * p->vel[2] + p->pos[2];
			k = RA_F32(self, 0x58);
			p->vel[0] = k * p->vel[0];
			p->vel[1] = k * p->vel[1];
			p->vel[2] = k * p->vel[2];
			k = RA_F32(self, 0x5c);
			p->colour[0] += p->dColour[0];
			p->colour[1] += p->dColour[1];
			p->colour[2] += p->dColour[2];
			p->colour2[0] += p->dColour2[0];
			p->vel[1] = dt * k + p->vel[1];
			p->colour2[1] += p->dColour2[1];
			p->colour2[2] += p->dColour2[2];
			RA_I32(self, 0x14)++;
		} else {
			p->alive = 0;
		}
	}
	if (RA_I32(self, 0x14) < 1)
		RA_U8(self, 0x50) = 0;
}

/* 0x0019d510  Rowing (gap; RollerBall/CRollerBall_Particle.cpp inferred)
Sets the emission direction (+0x40, 4 floats). */
void RbParticle_SetDirection(void *self, const float *dir)
{
	RA_F32(self, 0x40) = dir[0];
	RA_F32(self, 0x44) = dir[1];
	RA_F32(self, 0x48) = dir[2];
	RA_F32(self, 0x4c) = dir[3];
}
