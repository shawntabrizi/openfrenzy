/*
Meta/BigGun.c  <-  C:\BlitzParty\Game\Src\Meta\BigGun.cpp

The searchlights' big guns: a gun model whose laser bone turns towards the searchlight's
target; when it fires, a laser beam (with a spiral trail) burns the ground and a cutter
throws sparks where it hits. The guns are in the list at 0x00309268 (node at +0x20).

Objects: CBigGun (0x28) -> CBigGunLaser (0x74) -> CBigGunCutter (0x9c, 500 sparks of 0x78).

Tagged with BigGun.cpp: 0x00165000, 0x00165510, 0x001659e0; bracketed 0x001650d0-0x00165630.
0x00164b50-0x00164e90 (the gun constructor, the cutter's parameters and the draw functions)
and 0x00165a90-0x00165e90 (aim, free) work on the same objects (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x00309258  a_laser.bmp */
void *DAT_00309258;
/* 0x0030925c  the wide laser beam's texture (NULL: untextured; nothing sets it here) */
void *DAT_0030925c;
/* 0x00309260  military_17a.wav, the laser sound */
void *DAT_00309260;
/* 0x00309268  the big guns */
struct MbList DAT_00309268;

/* 0x002b0cb8-0x002b0cd0  laser colours (scaled by the fading alpha / 128) */
int DAT_002b0cb8 = 0xd2;            /* inner beam red */
int DAT_002b0cbc = 0;               /* inner beam green */
int DAT_002b0cc0 = 0x32;            /* inner beam blue */
int DAT_002b0cc4 = 0x7f;
int DAT_002b0cc8 = 0;               /* outer beam red */
int DAT_002b0ccc = 10;              /* outer beam green */
int DAT_002b0cd0 = 10;              /* outer beam blue */

#define ALIGNED16(a, b, c) \
	(((((uintptr_t)(a)) | ((uintptr_t)(b)) | ((uintptr_t)(c))) & 0xf) == 0)

/* 0x00164b50  Meta (file inferred: BigGun.cpp)
Constructor: loads the shared sound and texture once and adds the gun to the list. */
struct CBigGun *CBigGun_Construct(struct CBigGun *self)
{
	if (DAT_00309260 == NULL)
		DAT_00309260 = bLoadResource(NULL, "military_17a.wav", 2, 0xdefa);
	if (DAT_00309258 == NULL)
		DAT_00309258 = bLoadResource(NULL, "a_laser.bmp", 0, 0xdefa);
	MB_LIST_INSERT(&DAT_00309268, &self->next, 0);
	self->laser = NULL;
	return self;
}

/* 0x00164bc0  Meta (file inferred: BigGun.cpp)
The cutter's default spark parameters. */
void MbCutterParams_Init(struct MbCutterParams *p)
{
	p->flags = (unsigned char)((p->flags & 0xfe) | 2);
	p->gravity = -0.5f;
	p->speed = 10.0f;
	p->damping = 0.97f;
	p->unk10 = 0.01f;
	p->unk14 = 1.0f;
	p->unk18 = 0x10;
	p->unk1c = 0x20;
	p->colour[0] = 8;
	p->colour[1] = 4;
	p->colour[2] = 0;
	p->endColour[0] = 0x60;
	p->endColour[1] = 0x40;
	p->endColour[2] = 0x20;
	p->sizeMin = 3;
	p->sizeRange = 2;
}

/* 0x00164c40  Meta (file inferred: BigGun.cpp)
Draws the live sparks: a streak each (10 strip vertices, joined by repeated ends) and a
glow sprite. */
void CBigGunCutter_Draw(struct CBigGunCutter *self)
{
	char *base;
	char *out;
	char *slot;
	struct MbSpark *s;
	int c1[4];
	int c2[4];
	int count;
	int i;

	if ((self->params.flags & 1) == 0 || self->numActive <= 0)
		return;
	FUN_000438b0();
	bdSetTexture(0, (struct bdTexture *)self->texture);
	base = (char *)bdVertexBufferLock(self->vb, 4);
	if (base == NULL)
		return;
	MB_SET_RS(4, 0);
	out = base;
	slot = base + 0x1c;
	for (i = 0; i < self->numSparks; i++)
	{
		s = &self->sparks[i];
		if ((s->flags & 1) == 0)
			continue;
		c1[0] = s->colour[0] >> 7;
		c1[1] = s->colour[1] >> 7;
		c1[2] = s->colour[2] >> 7;
		c1[3] = 0x80;
		c2[0] = s->endColour[0] >> 7;
		c2[1] = s->endColour[1] >> 7;
		c2[2] = s->endColour[2] >> 7;
		c2[3] = 0x80;
		s->pos[3] = 1.0f;
		s->prevPos[3] = 1.0f;
		if (bdDrawBeam3D(s->prevPos, 1.0f, 1.0f, c1, s->pos, s->size, s->size, c2, NULL,
			(float *)slot, 0.0f) != 0)
		{
			/* 8 vertices at slots 1..8; slot 0 repeats slot 1, slot 9 repeats slot 8 */
			memcpy(out, slot, 0x1c);
			out = out + 0x118;
			memcpy(slot + 0xe0, slot + 0xc4, 0x1c);
			slot = slot + 0x118;
		}
	}
	bdVertexBufferUnlock(self->vb);
	FUN_00043b70(1);
	MB_SET_RS(10, 1);
	count = (int)(out - base) / 0x1c;
	if (count != 0)
		bdDrawVertexBuffer(4, self->vb, 0, count);
	FUN_00043b70(1);
	for (i = 0; i < self->numSparks; i++)
	{
		s = &self->sparks[i];
		if ((s->flags & 1) == 0)
			continue;
		c1[0] = s->endColour[0] >> 7;
		c1[1] = s->endColour[1] >> 7;
		c1[2] = s->endColour[2] >> 7;
		s->pos[3] = 1.0f;
		bdDrawSprite3D(s->pos, 30.0f, 30.0f, 0.0f, (struct bdTexture *)self->texGlow, 0, 0,
			c1[0], c1[1], c1[2], 0x7f, NULL, 0.0f);
	}
}

/* 0x00164e90  Meta (file inferred: BigGun.cpp)
Draws the laser while it is visible: it fades out after firing; a wide beam, the cutter's
sparks and a thin bright beam. (The two arguments are not used.) */
void CBigGunLaser_Draw(struct CBigGunLaser *self, void *unused1, void *unused2)
{
	int outer[4];
	int inner[4];
	int a;

	if (self->visible == 0)
		return;
	if (self->firing == 0)
	{
		if (self->alpha < 1)
		{
			self->visible = 0;
			self->alpha = 0x7f;
			return;
		}
		self->alpha = self->alpha - 0xf;
		if (self->alpha < 0)
			self->alpha = 0;
	}
	a = self->alpha;
	outer[1] = a * DAT_002b0ccc >> 7;
	outer[0] = a * DAT_002b0cc8 >> 7;
	inner[0] = a * DAT_002b0cb8 >> 7;
	outer[2] = a * DAT_002b0cd0 >> 7;
	inner[1] = a * DAT_002b0cbc >> 7;
	inner[2] = a * DAT_002b0cc0 >> 7;
	outer[3] = 0x7f;
	inner[3] = 0x7f;
	FUN_000438b0();
	MB_SET_RS(6, 1);
	MB_SET_RS(5, 0);
	MB_SET_RS(7, 2);
	MB_SET_RS(10, 3);
	bdDrawBeam3D(self->muzzle, 25.0f, 25.0f, outer, self->hit, 25.0f, 25.0f, outer,
		(struct bdTexture *)DAT_0030925c, NULL, 0.0f);
	CBigGunCutter_Draw(self->cutter);
	MB_SET_RS(10, 1);
	bdDrawBeam3D(self->muzzle, 15.0f, 15.0f, inner, self->hit, 15.0f, 15.0f, inner,
		(struct bdTexture *)DAT_00309258, NULL, 0.0f);
}

/* 0x00165000  Meta/BigGun.cpp
Cutter constructor: textures, the spark parameters, `count` sparks (all dead) and a vertex
buffer of 10 vertices per spark. */
void CBigGunCutter_Init(struct CBigGunCutter *self, int count)
{
	int i;

	self->texScorch = bLoadResource(NULL, "cutterscorch.bmp", 0, 0xdefa);
	self->texGlow = bLoadResource(NULL, "cutterglow.bmp", 0, 0xdefa);
	self->sparks = NULL;
	self->numActive = 0;
	self->texture = NULL;
	self->dir[0] = 0.0f;
	self->dir[1] = 1.0f;
	self->dir[2] = 0.0f;
	self->dir[3] = 1.0f;
	MbCutterParams_Init(&self->params);
	self->numSparks = count;
	self->sparks = (struct MbSpark *)MB_NEW(count * 0x78,
		"C:\\BlitzParty\\Game\\Src\\Meta\\BigGun.cpp", 0x32d);
	self->params.flags = (unsigned char)(self->params.flags & 0xfe);
	for (i = 0; i < self->numSparks; i++)
		self->sparks[i].flags = (unsigned char)(self->sparks[i].flags & 0xfe);
	bdVertexBufferCreate(&self->vb, self->numSparks * 10, 0x144, 0, 1);
}

/* 0x001650d0  Meta/BigGun.cpp (bracketed)
Starts a spark at the cutter's position: random life (16..47 frames), colours fading to 0,
random size and a random velocity mostly along the cutter's direction. */
void CBigGunCutter_StartSpark(struct CBigGunCutter *self, struct MbSpark *s)
{
	short life;
	float dx;
	float dz;
	float speed;
	int r1;
	int r2;

	s->flags = (unsigned char)(s->flags | 1);
	s->age = 0;
	s->life = (short)(mb_rand() % 32 + 0x10);
	s->colour[0] = (short)(self->params.colour[0] << 7);
	life = s->life;
	s->colour[1] = (short)(self->params.colour[1] << 7);
	s->colour[2] = (short)(self->params.colour[2] << 7);
	s->colourStep[0] = (short)-(s->colour[0] / life);
	s->colourStep[1] = (short)-(s->colour[1] / life);
	s->colourStep[2] = (short)-(s->colour[2] / life);
	s->endColour[0] = (short)(self->params.endColour[0] << 7);
	s->endColour[1] = (short)(self->params.endColour[1] << 7);
	s->endColour[2] = (short)(self->params.endColour[2] << 7);
	s->endColourStep[0] = (short)-(s->endColour[0] / life);
	s->endColourStep[1] = (short)-(s->endColour[1] / life);
	s->endColourStep[2] = (short)-(s->endColour[2] / life);
	s->unk4c = 0;
	s->size = (float)(mb_rand() % self->params.sizeRange + self->params.sizeMin);
	s->pos[0] = self->pos[0];
	s->pos[1] = self->pos[1];
	s->pos[2] = self->pos[2];
	s->pos[3] = 1.0f;
	s->prevPos[0] = self->pos[0];
	s->prevPos[1] = self->pos[1];
	s->prevPos[2] = self->pos[2];
	s->prevPos[3] = 1.0f;
	r1 = mb_rand();
	r2 = mb_rand();
	dx = ((float)r1 * 6.103702e-06f + self->dir[0]) - 0.1f;    /* 0x00265734, 0x002536cc */
	dz = ((float)r2 * 6.103702e-06f + self->dir[2]) - 0.1f;
	speed = (float)mb_rand() * MB_RAND_SCALE * self->params.speed;
	s->vel[0] = speed * dx;
	s->vel[1] = speed * self->dir[1];
	s->vel[2] = speed * dz;
}

/* 0x00165300  Meta/BigGun.cpp (bracketed)
Starts up to `count` dead sparks at `pos`. */
void CBigGunCutter_Emit(struct CBigGunCutter *self, int count, float *pos)
{
	struct MbSpark *s;
	int started = 0;
	int i;

	for (i = 0; i < self->numSparks; i++)
	{
		s = &self->sparks[i];
		if ((s->flags & 1) != 0)
			continue;
		CBigGunCutter_StartSpark(self, s);
		s->pos[0] = pos[0];
		s->pos[1] = pos[1];
		s->pos[2] = pos[2];
		s->pos[3] = pos[3];
		s->origin[0] = pos[0];
		s->origin[1] = pos[1];
		s->origin[2] = pos[2];
		started++;
		s->origin[3] = pos[3];
		if (started >= count)
			return;
	}
}

/* 0x00165380  Meta/BigGun.cpp (bracketed)
Per frame: moves to `pos`, emits 10 sparks while `emit`, moves the live sparks (gravity,
damping, colour fades, not below their start height) and ends the dead ones. */
void CBigGunCutter_Update(struct CBigGunCutter *self, float *pos, int emit)
{
	struct MbSpark *s;
	unsigned char flags;
	short age;
	float k;
	int i;

	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	if (emit != 0)
		CBigGunCutter_Emit(self, 10, self->pos);
	self->params.flags = (unsigned char)(self->params.flags | 3);
	self->numActive = 0;
	if ((self->params.flags & 1) == 0)
		return;
	for (i = 0; i < self->numSparks; i++)
	{
		s = &self->sparks[i];
		flags = s->flags;
		if ((flags & 1) == 0)
			continue;
		age = s->age;
		s->age = (short)(age + 1);
		if (age < s->life)
		{
			s->prevPos[0] = s->pos[0];
			s->prevPos[1] = s->pos[1];
			s->prevPos[2] = s->pos[2];
			s->prevPos[3] = s->pos[3];
			s->pos[0] = DAT_0028fda8 * s->vel[0] + s->pos[0];
			s->pos[1] = DAT_0028fda8 * s->vel[1] + s->pos[1];
			s->pos[2] = DAT_0028fda8 * s->vel[2] + s->pos[2];
			k = self->params.damping;
			s->vel[0] = k * s->vel[0];
			s->vel[1] = k * s->vel[1];
			s->vel[2] = k * s->vel[2];
			k = DAT_0028fda8 * self->params.gravity;
			s->colour[0] = (short)(s->colour[0] + s->colourStep[0]);
			s->colour[1] = (short)(s->colour[1] + s->colourStep[1]);
			s->colour[2] = (short)(s->colour[2] + s->colourStep[2]);
			s->endColour[0] = (short)(s->endColour[0] + s->endColourStep[0]);
			s->vel[1] = k + s->vel[1];
			s->endColour[1] = (short)(s->endColour[1] + s->endColourStep[1]);
			s->endColour[2] = (short)(s->endColour[2] + s->endColourStep[2]);
			self->numActive++;
			if ((self->params.flags & 2) != 0 && s->pos[1] < s->origin[1])
				s->pos[1] = s->origin[1];
		}
		else
		{
			s->flags = (unsigned char)(flags & 0xfe);
		}
	}
	if (self->numActive < 1)
		self->params.flags = (unsigned char)(self->params.flags & 0xfe);
}

/* 0x001654f0  Meta/BigGun.cpp (bracketed)
Draws the gun's laser (searchlight draw). */
void CBigGun_DrawLaser(struct CBigGun *self, float *target)
{
	CBigGunLaser_Draw(self->laser, self->actor, target);
}

/* 0x00165510  Meta/BigGun.cpp
Laser constructor: the spiral trail (white beam, 180 points) and the cutter. */
void CBigGunLaser_Init(struct CBigGunLaser *self, void *actor)
{
	int head[4];
	int tail[4];
	void *trail;
	struct CBigGunCutter *cutter;

	self->actor = actor;
	head[0] = 0;
	head[1] = 0;
	head[2] = 0x7f;
	head[3] = 0x7f;
	tail[0] = 0;
	tail[1] = 0;
	tail[2] = 0x7f;
	tail[3] = 0x7f;
	trail = MB_NEW(0x584c, "C:\\BlitzParty\\Game\\Src\\Meta\\BigGun.cpp", 0x1ae);
	if (trail == NULL)
		trail = NULL;
	else
		trail = CProjTrail_Init((struct CProjTrail *)trail,
			(int)bLoadResource(NULL, "whitebeam.bmp", 0, 0xdefa), 0xb4, head, tail, 6.0f,
			6.0f, 0);
	self->trail = trail;
	/* as in the original, a failed allocation writes through NULL */
	MB_F32(trail, 0x54) = MB_F32(trail, 0x54) * 4.0f;   /* 0x00257c40 */
	cutter = (struct CBigGunCutter *)MB_NEW(0x9c, "C:\\BlitzParty\\Game\\Src\\Meta\\BigGun.cpp",
		0x1b3);
	if (cutter == NULL)
	{
		cutter = NULL;
	}
	else
	{
		cutter->texGlow = NULL;
		cutter->texScorch = NULL;
	}
	self->cutter = cutter;
	CBigGunCutter_Init(cutter, 500);
}

/* 0x00165630  Meta/BigGun.cpp (bracketed)
Per frame while firing: the laser runs for `duration` frames with its sound; it goes from
the gun's "laser3" bone towards `target` (1.4 times as far, stopped by the collision world)
and lays a spiral trail along the beam. Then the cutter. */
void CBigGunLaser_Update(struct CBigGunLaser *self, void *actor, float *target)
{
	char name[16];
	struct BActorNode *bone;
	float start[4];
	float delta[4];
	float normal[4];
	float dir[4];
	float axis[4];
	float perp[4];
	float pt[4];
	float off[4];
	float len;
	float k;
	float t;
	float step;
	float phase;
	float twist;
	int i;

	if (self->firing != 0)
	{
		self->time = DAT_0028fda8 + self->time;
		CLSamples_Play((int)DAT_00309260, 0xb4, 0, 2, NULL);
		if (self->duration < self->time)
		{
			self->firing = 0;
			bsStopSample((struct BSample *)DAT_00309260);
		}
		sprintf(name, "laser3");
		bone = baFindNode((struct BActorNode *)MB_PTR(actor, 0x144), name);
		baGetNodePosition((struct BActorInstance *)MB_ADDR(actor, 0x10), bone, self->muzzle);
		self->muzzle[3] = 1.0f;
		self->hitRaw[3] = 1.0f;
		start[0] = self->muzzle[0];
		start[1] = self->muzzle[1];
		start[2] = self->muzzle[2];
		start[3] = self->muzzle[3];
		delta[0] = (target[0] - self->muzzle[0]) * 1.4f;   /* 0x00263098 */
		delta[1] = (target[1] - self->muzzle[1]) * 1.4f;
		delta[2] = (target[2] - self->muzzle[2]) * 1.4f;
		((float (*)(float *, float *, float *, float *))FUN_000b4bd0)(start, delta,
			self->hitRaw, normal);
		self->hit[0] = self->hitRaw[0];
		self->hit[1] = self->hitRaw[1];
		self->hit[2] = self->hitRaw[2];
		self->hit[3] = self->hitRaw[3];
		dir[0] = self->hit[0] - self->muzzle[0];
		dir[3] = 1.0f;
		dir[1] = self->hit[1] - self->muzzle[1];
		dir[2] = self->hit[2] - self->muzzle[2];
		phase = 0.0f;
		len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
		k = 1.0f / len;
		axis[0] = dir[0] * k;
		axis[1] = dir[1] * k;
		axis[2] = dir[2] * k;
		/* a direction across the beam: axis x up */
		perp[0] = axis[2] * DAT_0028fdc8[1] - axis[1] * DAT_0028fdc8[2];
		perp[1] = axis[0] * DAT_0028fdc8[2] - axis[2] * DAT_0028fdc8[0];
		perp[2] = axis[1] * DAT_0028fdc8[0] - axis[0] * DAT_0028fdc8[1];
		k = sqrtf(perp[0] * perp[0] + perp[1] * perp[1] + perp[2] * perp[2]);
		if (k == 0.0f)
		{
			perp[0] = DAT_0028fdb8[0];
			perp[1] = DAT_0028fdb8[1];
			perp[2] = DAT_0028fdb8[2];
			perp[3] = DAT_0028fdb8[3];
		}
		else
		{
			k = 1.0f / k;
			perp[0] = perp[0] * k;
			perp[1] = perp[1] * k;
			perp[2] = perp[2] * k;
		}
		step = len * 0.0055555557f;                         /* 0x00263518: 1/180 */
		twist = 0.0f;
		for (i = 0; i < 0xb4; i++)
		{
			t = (float)i * 0.0055555557f;
			FUN_00050ff0(pt, self->muzzle, 1.0f - t, self->hit, t);
			axis[3] = CLMath_WrapAngle(phase);
			CLVec3_RotateAboutAxis(off, perp, axis);
			off[3] = 1.0f;
			off[0] = pt[0] + off[0] * 15.0f;               /* 0x002553a0 */
			off[1] = pt[1] + off[1] * 15.0f;
			off[2] = pt[2] + off[2] * 15.0f;
			CProjTrail_AddPoint((struct CProjTrail *)self->trail, off);
			/* a distance along the beam that the original keeps but does not use */
			twist = twist + step;
			if (twist > 4.0f)
				twist = twist - 4.0f;
			phase = phase + 0.62831849f;                     /* 0x00267cec */
		}
	}
	CBigGunCutter_Update(self->cutter, self->hit, self->firing);
}

/* 0x001659e0  Meta/BigGun.cpp
Gun set-up: the gun model at the gun's position, its laser bone, and the laser. */
void CBigGun_Init(struct CBigGun *self, const char *model, const char *bone, int id)
{
	void *res;
	void *actor;
	struct CBigGunLaser *laser;

	res = bLoadResource(NULL, model, 1, 0);
	self->pos[3] = 1.0f;
	self->id = id;
	actor = ((void *(*)(void *, float *, void *, int, int, int))FUN_000af1a0)(res, self->pos,
		NULL, 3, 0, 1);
	self->actor = actor;
	self->bone = baFindNode((struct BActorNode *)MB_PTR(actor, 0x144), bone);
	laser = (struct CBigGunLaser *)MB_NEW(0x74, "C:\\BlitzParty\\Game\\Src\\Meta\\BigGun.cpp",
		0xcb);
	if (laser != NULL)
	{
		laser->time = 0.0f;
		laser->duration = 240.0f;
		laser->alpha = 0x7f;
		laser->firing = 0;
		laser->visible = 0;
		self->laser = laser;
		CBigGunLaser_Init(laser, self->actor);
		return;
	}
	self->laser = NULL;
	CBigGunLaser_Init(NULL, self->actor);
}

/* the angle of a direction around the vertical, as the gun measures it: atan2(z, x) + pi/2
   wrapped to (-pi, pi], negated */
static double gun_heading(double z, double x)
{
	double a = atan2(z, x) + (double)1.5707963f;            /* 0x00253898 */

	while (a > (double)3.1415925f)
		a = a - (double)6.2831855f;                         /* 0x00255930 */
	return a * -1.0;                                        /* 0x00253854 */
}

/* 0x00165a90  Meta (file inferred: BigGun.cpp)
Aims the gun's laser bone at `target` (half way per frame, the short or the long way
round), then updates the laser. */
void CBigGun_Aim(struct CBigGun *self, float *target)
{
	float trans[16];
	float local[16];
	float world[16];
	float inverse[16];
	float rot[16];
	float quat[4];
	float q[4];
	float d[4];
	float v[4];
	double a;
	float yaw;
	float pitch;
	struct BActorNode *bone = self->bone;
	char *parent;

	self->pos[3] = 1.0f;
	MB_DUAL(FUN_00050250, void (*)(undefined4 *, undefined4, undefined4, undefined4),
		void (*)(float *, float, float, float),
		((undefined4 *)trans, MB_U32(bone, 0x10), MB_U32(bone, 0x14), MB_U32(bone, 0x18)),
		(trans, MB_F32(bone, 0x10), MB_F32(bone, 0x14), MB_F32(bone, 0x18)));
	parent = (char *)MB_PTR(self->bone, 0x148);
	if (parent == NULL)
		memcpy(local, trans, sizeof local);
	else if (ALIGNED16(parent, local, trans))
		FUN_00050150(local, (float *)(parent + 0x80), trans);
	else
		FUN_0004ff30(local, (float *)(parent + 0x80), trans);
	if (ALIGNED16(self->actor, world, local))
		FUN_00050150(world, MB_FP(self->actor, 0x10), local);
	else
		FUN_0004ff30(world, MB_FP(self->actor, 0x10), local);
	d[0] = target[0] - world[12];
	d[3] = 1.0f;
	d[1] = target[1] - world[13];
	world[12] = 0.0f;
	world[13] = 0.0f;
	d[2] = target[2] - world[14];
	world[14] = 0.0f;
	FUN_000502f0(inverse, world);
	FUN_00050530(inverse, d);
	/* yaw */
	a = gun_heading((double)d[2], (double)d[0]);
	yaw = (float)a;
	if (a > (double)3.1415925f)
		yaw = 3.1415925f;
	else if (yaw < -3.1415925f)
		yaw = -3.1415925f;
	/* pitch */
	a = atan2((double)d[1], sqrt((double)d[0] * (double)d[0] + (double)d[2] * (double)d[2]));
	if (a > (double)3.1415925f)
	{
		do
			a = a - (double)6.2831855f;
		while (a > (double)3.1415925f);
	}
	pitch = (float)a;
	if (a > (double)3.1415925f)
		pitch = 3.1415925f;
	else if (a < (double)-3.1415925f)
		pitch = -3.1415925f;
	FUN_00050ba0(local, pitch, yaw, 0.0f);
	FUN_00051700(quat, local);
	/* the bone's present heading */
	FUN_000515c0(rot, MB_FP(self->bone, 0x30));
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = -1.0f;
	v[3] = 1.0f;
	FUN_00050530(rot, v);
	a = atan2((double)v[2], (double)v[0]) + (double)1.5707963f;
	if (a > (double)3.1415925f)
		a = a - (double)6.2831855f;
	a = a * -1.0;
	if (a > (double)3.1415925f)
		a = (double)3.1415925f;
	else if (a < (double)-3.1415925f)
		a = (double)-3.1415925f;
	a = (double)yaw - a;
	if (a > (double)3.1415925f || a < (double)-3.1415925f)
		FUN_00051a80(q, MB_FP(self->bone, 0x30), quat, 0.5f);
	else
		FUN_00051860(q, MB_FP(self->bone, 0x30), quat, 0.5f);
	MB_F32(self->bone, 0x30) = q[0];
	MB_F32(self->bone, 0x34) = q[1];
	MB_F32(self->bone, 0x38) = q[2];
	MB_F32(self->bone, 0x3c) = q[3];
	CBigGunLaser_Update(self->laser, self->actor, target);
}

/* frees a laser's cutter and the laser (0x00165e20 and 0x00165e90) */
static void gun_free_laser(struct CBigGun *self)
{
	struct CBigGunLaser *laser = self->laser;
	struct CBigGunCutter *cutter;

	if (laser == NULL)
		return;
	cutter = laser->cutter;
	if (cutter != NULL)
	{
		if (cutter->vb != NULL)
			bdVertexBufferDestroy(cutter->vb);
		if (cutter->sparks != NULL)
		{
			MB_DELETE(cutter->sparks);
			cutter->sparks = NULL;
		}
		cutter->numSparks = 0;
		cutter->numActive = 0;
		cutter->texGlow = NULL;
		cutter->texScorch = NULL;
		MB_DELETE(cutter);
		laser->cutter = NULL;
	}
	MB_DELETE(laser);
	self->laser = NULL;
}

/* 0x00165e20  Meta (file inferred: BigGun.cpp)
Frees the gun's laser and takes the gun off the list. */
void CBigGun_Free(struct CBigGun *self)
{
	gun_free_laser(self);
	MB_LIST_REMOVE(&DAT_00309268, &self->next);
}

/* 0x00165e90  Meta (file inferred: BigGun.cpp)
Destructor: forgets the shared sound and texture, frees the laser. */
void CBigGun_Destruct(struct CBigGun *self)
{
	if (DAT_00309260 != NULL)
		DAT_00309260 = NULL;
	if (DAT_00309258 != NULL)
		DAT_00309258 = NULL;
	gun_free_laser(self);
}
