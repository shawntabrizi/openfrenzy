/*
Meta/CMLaser.c  <-  C:\BlitzParty\Game\Src\Meta\CMLaser.cpp

Laser show beams on the Meta board: each beam starts at an origin, turns slowly, grows to
its full length and bounces off the collision world up to 9 times; its colour fades with
its life. All beams are in the list at 0x00308f1c.

0x0015fd40 is tagged with CMLaser.cpp; the other functions here work on the same beams
and list (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x00308f14  projray3.bmp */
void *DAT_00308f14;
/* 0x00308f18  the collision world the beams bounce off (NULL: no bounces) */
void *DAT_00308f18;
/* 0x00308f1c  the beams (count at 0x00308f24) */
struct MbList DAT_00308f1c;

/* 0x0015f6e0  Meta/CMLaser.cpp (file inferred)
Draws the beam's segments; each widens from half the width at its start. */
void CMLaser_Draw(struct CMLaser *self)
{
	float half;
	int i;

	for (i = 1; i <= self->numPoints; i++)
	{
		half = self->width * 0.5f;                      /* 0x002536d0 */
		bdDrawBeam3D(self->points[i - 1].pos, half, half, self->points[i - 1].colour,
			self->points[i].pos, self->width, self->width, self->points[i].colour,
			(struct bdTexture *)self->texture, NULL, 0.0f);
	}
}

/* 0x0015f750  Meta/CMLaser.cpp (file inferred)
Clears the beam list and loads the texture. */
void CMLaser_Init(void)
{
	CLList_Init2((int *)&DAT_00308f1c);
	DAT_00308f14 = bLoadResource(NULL, "projray3.bmp", 0, 0xdefa);
	if (DAT_00308f14 == NULL)
		bkPrintf("Can't load CMLaser texture: projray3.bmp");
}

/* 0x0015f790  Meta/CMLaser.cpp (file inferred)
Draws every beam (additive). */
void CMLaser_DrawAll(void)
{
	struct CMLaser *laser;

	if (DAT_00308f1c.count == 0)
		return;
	FUN_000438b0();
	((void (*)(int, int, int))FUN_00046cb0)(6, 1, 0);
	((void (*)(int, int, int))FUN_00046cb0)(10, 1, 0);
	((void (*)(int, int, int))FUN_00046cb0)(7, 3, 0);
	((void (*)(int, int, int))FUN_00046cb0)(5, 0, 0);
	for (laser = MB_LIST_NEXT(&DAT_00308f1c, NULL); laser != NULL;
		laser = MB_LIST_NEXT(&DAT_00308f1c, laser))
		CMLaser_Draw(laser);
	FUN_00046d00(6);
	FUN_00046d00(10);
	FUN_00046d00(7);
	FUN_00046d00(5);
}

/* 0x0015f820  Meta/CMLaser.cpp (file inferred)
Traces the beam from its origin along its angles: up to 9 bounces off the collision
world; each bounce takes the rest of the length and mixes the colour towards the end
colour. */
void CMLaser_Trace(struct CMLaser *self)
{
	float length = self->length;
	float dir[4];
	float step[3];
	float hit[4];
	float normal[4];
	float t;
	float keep;
	int n;

	self->numPoints = 0;
	self->points[0].pos[0] = self->origin[0];
	self->points[0].pos[1] = self->origin[1];
	self->points[0].pos[2] = self->origin[2];
	self->points[0].pos[3] = self->origin[3];
	self->points[0].colour[1] = self->colour[1];
	self->points[0].colour[0] = self->colour[0];
	self->points[0].colour[2] = self->colour[2];
	self->points[0].colour[3] = self->colour[3];
	dir[0] = 0.0f;
	dir[1] = 0.0f;
	dir[2] = 1.0f;
	dir[3] = 1.0f;
	self->points[0].pos[3] = 1.0f;
	FUN_00051310(dir, dir, self->angles[0], self->angles[1], self->angles[2]);
	if (self->numPoints >= 9)
		return;
	while (length > 1.0f)
	{
		step[0] = dir[0] * length;
		step[1] = dir[1] * length;
		step[2] = dir[2] * length;
		if (DAT_00308f18 != NULL)
		{
			/* charlib: the fraction of the segment before the first hit, its point
			   and the surface normal */
			t = ((float (*)(float *, float *, float *, float *))FUN_000b4bd0)(
				self->points[self->numPoints].pos, step, hit, normal);
			if (t > 0.0f)
			{
				n = ++self->numPoints;
				self->points[n].pos[0] = hit[0];
				self->points[n].pos[1] = hit[1];
				self->points[n].pos[2] = hit[2];
				self->points[n].pos[3] = hit[3];
				CLVec3_Reflect(dir, normal);
				keep = 1.0f - t;
				length = length * keep;
				n = self->numPoints;
				self->points[n].colour[0] = (int)((float)self->points[n - 1].colour[0] * keep +
					(float)self->endColour[0] * t);
				n = self->numPoints;
				self->points[n].colour[1] = (int)((float)self->points[n - 1].colour[1] * keep +
					(float)self->endColour[1] * t);
				n = self->numPoints;
				self->points[n].colour[2] = (int)((float)self->points[n - 1].colour[2] * keep +
					(float)self->endColour[2] * t);
				n = self->numPoints;
				self->points[n].colour[3] = (int)((float)self->points[n - 1].colour[3] * keep +
					(float)self->endColour[3] * t);
				if (self->numPoints >= 9)
					return;
				continue;
			}
		}
		n = ++self->numPoints;
		self->points[n].pos[0] = step[0] + self->points[n - 1].pos[0];
		self->points[n].pos[1] = step[1] + self->points[n - 1].pos[1];
		self->points[n].pos[2] = step[2] + self->points[n - 1].pos[2];
		self->points[self->numPoints].pos[3] = 1.0f;
		self->points[self->numPoints].colour[0] = self->endColour[0];
		self->points[self->numPoints].colour[1] = self->endColour[1];
		self->points[self->numPoints].colour[2] = self->endColour[2];
		self->points[self->numPoints].colour[3] = self->endColour[3];
		return;
	}
}

/* 0x0015fad0  Meta/CMLaser.cpp (file inferred)
Constructor: adds the beam to the list; its length starts at 10. */
struct CMLaser *CMLaser_Construct(struct CMLaser *self, float *origin, float *angles,
	float *spin, float maxLength, float width, int *colour, int *endColour, float life)
{
	MB_LIST_INSERT(&DAT_00308f1c, self, 0);
	self->origin[0] = origin[0];
	self->origin[1] = origin[1];
	self->origin[2] = origin[2];
	self->origin[3] = origin[3];
	self->colour[0] = colour[0];
	self->colour[1] = colour[1];
	self->colour[2] = colour[2];
	self->colour[3] = colour[3];
	self->endColour[0] = endColour[0];
	self->endColour[1] = endColour[1];
	self->endColour[2] = endColour[2];
	self->endColour[3] = endColour[3];
	self->maxLength = maxLength;
	self->length = 10.0f;
	self->width = width;
	self->life = life;
	self->texture = DAT_00308f14;
	self->spin[0] = spin[0];
	self->spin[1] = spin[1];
	self->spin[2] = spin[2];
	self->spin[3] = spin[3];
	self->angles[0] = angles[0];
	self->angles[1] = angles[1];
	self->angles[2] = angles[2];
	self->angles[3] = angles[3];
	self->numPoints = 0;
	CMLaser_Trace(self);
	return self;
}

/* 0x0015fbb0  Meta/CMLaser.cpp (file inferred)
Per frame: grows the beam, counts its life down (deletes it at the end and returns the
next beam's predecessor for the list walk), fades its colour and turns it. */
struct CMLaser *CMLaser_Update(struct CMLaser *self)
{
	float grow;
	struct CMLaser *next;
	int i;

	if (self->length < self->maxLength)
	{
		grow = (self->maxLength - self->length) * 0.04f;   /* 0x0026fefc */
		if (!(2.0f <= grow))
			grow = 2.0f;
		self->length = grow + self->length;
	}
	if (self->life > 0.0f)
	{
		self->life = self->life - DAT_0028fda8;
		if (self->life <= 0.0f)
		{
			next = MB_LIST_REMOVE(&DAT_00308f1c, self);
			MB_DELETE(self);
			return next;
		}
		for (i = 0; i < 4; i++)
			self->colour[i] = (int)(((float)self->colour[i] * self->life) /
				(DAT_0028fda8 + self->life));
	}
	if (self->spin[0] != 0.0f || self->spin[1] != 0.0f || self->spin[2] != 0.0f)
	{
		self->angles[0] = DAT_0028fda8 * self->spin[0] + self->angles[0];
		self->angles[1] = DAT_0028fda8 * self->spin[1] + self->angles[1];
		self->angles[2] = DAT_0028fda8 * self->spin[2] + self->angles[2];
		CMLaser_Trace(self);
	}
	return self;
}

/* 0x0015fcd0  Meta/CMLaser.cpp (file inferred)
Deletes every beam. */
void CMLaser_DeleteAll(void)
{
	void *laser;

	laser = MB_LIST_NEXT(&DAT_00308f1c, NULL);
	while (laser != NULL)
	{
		MB_LIST_REMOVE(&DAT_00308f1c, laser);
		if (laser != NULL)
			MB_DELETE(laser);
		laser = MB_LIST_NEXT(&DAT_00308f1c, NULL);
	}
}

/* 0x0015fd10  Meta/CMLaser.cpp (file inferred)
Updates every beam. */
void CMLaser_UpdateAll(void)
{
	struct CMLaser *laser;

	laser = MB_LIST_NEXT(&DAT_00308f1c, NULL);
	while (laser != NULL)
	{
		laser = CMLaser_Update(laser);
		laser = MB_LIST_NEXT(&DAT_00308f1c, laser);
	}
}

/* 0x0015fd40  Meta/CMLaser.cpp
Fires `count` beams from `origin` in random directions, all turning the same random way,
in colour (r, g, b, a) fading to black over `life` frames. */
void CMLaser_Fire(float *origin, int count, int r, int g, int b, int a, float life)
{
	int endColour[4];
	int colour[4];
	float spin[4];
	float angles[4];
	float s;
	float yaw;
	struct CMLaser *laser;

	colour[0] = r;
	colour[3] = a;
	colour[1] = g;
	mb_rand();
	mb_rand();
	s = (float)mb_rand() * 1.5259255e-06f - 0.025f;    /* 0x00267a88, 0x00267a84 */
	endColour[0] = 0;
	endColour[1] = 0;
	endColour[2] = 0;
	endColour[3] = 0;
	colour[2] = b;
	spin[3] = 1.0f;
	spin[0] = s;
	spin[1] = s;
	spin[2] = s;
	for (; count > 0; count--)
	{
		angles[3] = 1.0f;
		mb_rand();
		mb_rand();
		yaw = (float)mb_rand() * MB_RAND_ANGLE;
		angles[1] = yaw;
		angles[0] = yaw - 0.31415924f;                  /* 0x00253878 */
		angles[2] = angles[0];
		laser = (struct CMLaser *)MB_NEW(0x1b0, "C:\\BlitzParty\\Game\\Src\\Meta\\CMLaser.cpp",
			0x14a);
		if (laser != NULL)
			CMLaser_Construct(laser, origin, angles, spin, 2000.0f, 16.0f, colour, endColour,
				life);
	}
}

/* 0x0015ff00  Meta (file inferred: CMLaser.cpp)
A laser show: `count` beams in each of six colours. */
void CMLaser_Show(float *origin, int count, float life)
{
	CMLaser_Fire(origin, count, 0x7f, 0, 0, 0x7f, life);
	CMLaser_Fire(origin, count, 0, 0x7f, 0, 0x7f, life);
	CMLaser_Fire(origin, count, 0, 0, 0x7f, 0x7f, life);
	CMLaser_Fire(origin, count, 100, 100, 0, 0x7f, life);
	CMLaser_Fire(origin, count, 100, 0, 100, 0x7f, life);
	CMLaser_Fire(origin, count, 0, 100, 100, 0x7f, life);
}
