/*
CarryOn/CFire_LaunchRack.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_LaunchRack.cpp

The launch rack: the players drop their rockets into its slots. When the fill time runs
out the rack launches the rockets one by one from the middle outwards, each as a firework
in its team's colour. Rockets of half the slots or more give a big show.

0x000ca740 is tagged with CFire_LaunchRack.cpp; the other functions work on the same
object and vtable (file inferred).
*/

#include "carryon_b.h"

#define CB_FILE "C:\\BlitzParty\\Game\\Src\\CarryOn\\CFire_LaunchRack.cpp"

/* 0x00268788  vtable of CFire_LaunchRack */
const void *const vtable_00268788[1] = {
	(const void *)CFire_LaunchRack_dtor,
};

/* 0x0029439c  the rocket models of the six team colours */
static const char *const DAT_0029439c[6] = {
	"objects\\rack_rocket_green.obe",   /* 0x002685f4 */
	"objects\\rack_rocket_red.obe",     /* 0x002685d8 */
	"objects\\rack_rocket_blue.obe",    /* 0x002685b8 */
	"objects\\rack_rocket_yellow.obe",  /* 0x00268598 */
	"objects\\rack_rocket_orange.obe",  /* 0x00268578 */
	"objects\\rack_rocket_pink.obe",    /* 0x00268558 */
};

/* the slot that is filled or launched as the i-th: from the middle outwards */
static int cb_rack_slot(int i)
{
	if ((i & 1) == 0)
		return DAT_00294390 / 2 + i / 2;
	return DAT_00294390 / 2 - i / 2 - 1;
}

/* rack-local x of a slot: the slots spread over the rack with two slots of room at
   each end */
static float cb_rack_slot_x(struct CFire_LaunchRack *self, int slot)
{
	return (self->halfWidth - -self->halfWidth) *
		(((float)slot + 2.0f) / ((float)DAT_00294390 + 4.0f)) + -self->halfWidth;
}

/* the rack's world matrix: turned by its angle about y, at its position */
static void cb_rack_matrix(struct CFire_LaunchRack *self, float *m)
{
	bmMatrixRotationY(m, self->angle);
	m[12] = self->pos[0];
	m[13] = self->pos[1];
	m[14] = self->pos[2];
}

/* 0x000ca730  CarryOn/CFire_LaunchRack.cpp (file inferred) */
struct CFire_LaunchRack *CFire_LaunchRack_ctor(struct CFire_LaunchRack *self)
{
	self->vtbl = vtable_00268788;
	return self;
}

/* 0x000ca740  CarryOn/CFire_LaunchRack.cpp
Places the rack (size width x depth, turned by angle) and loads the six rocket models. */
int CFire_LaunchRack_Init(struct CFire_LaunchRack *self, struct CFire_Fireworks *fireworks,
	float *pos, float width, float depth, float angle)
{
	struct bkResource *model;
	int i;

	self->fillTime = 0.0f;
	self->launchesLeft = 0;
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->halfWidth = width * 0.5f;                     /* 0x002536d0 */
	self->angle = angle;
	self->halfDepth = depth * 0.5f;
	self->fireworks = fireworks;
	memset(self->slots, 0, sizeof self->slots);
	for (i = 0; i < 6; i++)
	{
		model = bLoadResource(*(struct bkPackage **)DAT_002f6d5c, DAT_0029439c[i], 1, 0xdefa);
		self->models[i] = model;
		if (model != NULL)
		{
			self->actors[i] = baInitActorInstance(NULL, (struct BActor *)model, 1, CB_FILE, 0x5c);
			CB_F32(self->actors[i], 0xcc) = 1.0f;
			CB_F32(self->actors[i], 0xdc) = 1.0f;
		}
	}
	return 1;
}

/* 0x000ca830  CarryOn/CFire_LaunchRack.cpp (file inferred)
Frees the rocket models and their instances. */
void CFire_LaunchRack_Free(struct CFire_LaunchRack *self)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		if (self->actors[i] != NULL)
		{
			baDeleteActorInstance(self->actors[i]);
			self->actors[i] = NULL;
		}
		if (self->models[i] != NULL)
		{
			bkDeleteResource(self->models[i]);
			self->models[i] = NULL;
		}
	}
}

/* 0x000ca880  CarryOn/CFire_LaunchRack.cpp (file inferred)
Every player that stands on the rack gets carryon_a's event 4 (0x000c0d30). */
void CFire_LaunchRack_CheckPlayers(struct CFire_LaunchRack *self)
{
	float m[16];
	float d[4];
	void *body;
	int i;

	bmMatrixRotationY(m, -self->angle);
	for (i = 0; i < DAT_002bae94; i++)
	{
		body = CB_PTR(DAT_002f6d28[i], 0x30);
		d[0] = self->pos[0] - CB_F32(body, 0xd0);
		d[1] = self->pos[1] - CB_F32(body, 0xd4);
		d[2] = self->pos[2] - CB_F32(body, 0xd8);
		d[3] = 0.0f;    /* not set by the original; the matrix has no translation */
		bmTransformVector4InPlace(m, d);
		if (-self->halfWidth < d[0] && d[0] < self->halfWidth &&
			-self->halfDepth < d[2] && d[2] < self->halfDepth)
			CB_CALL(FUN_000c0d30, void (*)(void *, int, int))(DAT_002f6d28[i], 4, -1);
	}
}

/* 0x000ca970  CarryOn/CFire_LaunchRack.cpp (file inferred)
Starts the launches (unless they run already): the fill time restarts. */
int CFire_LaunchRack_StartLaunch(struct CFire_LaunchRack *self)
{
	int full;
	int i;

	if (self->launchesLeft != 0)
		return 0;
	self->fillTime = DAT_00294394;
	self->launchesLeft = DAT_00294390;
	self->launchTime = DAT_00294398;
	full = 0;
	for (i = 0; i < DAT_00294390; i++)
		if (self->slots[i] != 0)
			full++;
	self->bigShow = DAT_00294390 / 2 <= full;
	return 1;
}

/* 0x000ca9e0  CarryOn/CFire_LaunchRack.cpp (file inferred)
The world position of a slot and (vel not NULL) a launch velocity: outwards with the
slot, up 20-26, forwards 4-24. */
void CFire_LaunchRack_SlotLaunch(struct CFire_LaunchRack *self, int slot, float *pos, float *vel)
{
	float m[16];

	bmMatrixRotationY(m, self->angle);
	pos[1] = 0.0f;
	pos[2] = 0.0f;
	pos[3] = 1.0f;
	pos[0] = cb_rack_slot_x(self, slot);
	if (vel != NULL)
	{
		vel[0] = (pos[0] / self->halfWidth) * 15.0f;    /* 0x002553a0 */
		vel[1] = (float)cb_rand() * 0.000183111f + 20.0f;   /* 0x00266d88, 0x00255530 */
		vel[3] = 1.0f;
		vel[2] = (float)cb_rand() * 0.00061037f + 4.0f;     /* 0x002537d4, 0x00257c40 */
		bmTransformVector4InPlace(m, vel);
	}
	m[12] = self->pos[0];
	m[13] = self->pos[1];
	m[14] = self->pos[2];
	bmTransformVector4InPlace(m, pos);
}

/* 0x000cab20  CarryOn/CFire_LaunchRack.cpp (file inferred)
The point of the rack's centre line (20 in from each end) nearest to `pos`. */
void CFire_LaunchRack_NearestPoint(struct CFire_LaunchRack *self, float *out, float *pos)
{
	float m[16];
	float ends[8];
	float a[4];
	float b[4];
	float d[3];
	float v[3];
	float dir[4];
	float maxT;
	float t;

	maxT = self->halfWidth - 10.0f;                     /* 0x002536d8 */
	maxT = maxT + maxT;
	cb_rack_matrix(self, m);
	ends[0] = 20.0f - self->halfWidth;                  /* 0x00255530 */
	ends[1] = 0.0f;
	ends[2] = 0.0f;
	ends[3] = 1.0f;
	ends[4] = self->halfWidth - 20.0f;
	ends[5] = 0.0f;
	ends[6] = 0.0f;
	ends[7] = 1.0f;
	bmTransformVector4(a, m, ends);
	bmTransformVector4(b, m, ends + 4);
	d[0] = b[0] - a[0];
	d[1] = b[1] - a[1];
	d[2] = b[2] - a[2];
	v[0] = pos[0] - a[0];
	v[1] = pos[1] - a[1];
	v[2] = pos[2] - a[2];
	bmVectorSetLength(dir, d, 1.0f);
	t = dir[0] * v[0] + dir[1] * v[1] + dir[2] * v[2];
	if (t < 0.0f)
		t = 0.0f;
	if (maxT < t)
		t = maxT;
	out[3] = 1.0f;
	out[0] = dir[0] * t + a[0];
	out[1] = dir[1] * t + a[1];
	out[2] = dir[2] * t + a[2];
}

/* 0x000cacf0  CarryOn/CFire_LaunchRack.cpp (file inferred)
Draws the rockets in the slots (and, when DAT_002f6d60 is set, the rack outline and the
fill time as debug lines). */
void CFire_LaunchRack_Draw(struct CFire_LaunchRack *self)
{
	/* a line at the back of the rack: (-w, 0, -80) to (w, 0, -80); [8..11] keeps the
	   right end */
	float line[12];
	/* a slot position, then (debug) the rack outline */
	float pts[24];
	float m[16];
	float out[4];
	float f;
	int i;
	int character;
	struct BActorInstance *actor;

	line[0] = -self->halfWidth;
	line[1] = 0.0f;
	line[2] = -80.0f;
	line[3] = 1.0f;
	line[4] = self->halfWidth;
	line[5] = 0.0f;
	line[6] = -80.0f;
	line[7] = 1.0f;
	line[8] = self->halfWidth;
	line[9] = 0.0f;
	line[10] = -80.0f;
	line[11] = 1.0f;
	pts[0] = -500.0f;
	pts[1] = 0.0f;
	pts[2] = 0.0f;
	pts[3] = 1.0f;
	pts[4] = 500.0f;
	pts[5] = 30.0f;
	pts[6] = 0.0f;
	pts[7] = 1.0f;
	if (DAT_002f6d60 != 0)
	{
		pts[0] = line[0];
		pts[4] = line[8];
		pts[8] = line[0];
		pts[9] = 0.0f;
		pts[10] = -self->halfDepth;
		pts[11] = 1.0f;
		pts[12] = line[8];
		pts[13] = 0.0f;
		pts[14] = -self->halfDepth;
		pts[15] = 1.0f;
		pts[16] = line[8];
		pts[17] = 0.0f;
		pts[18] = self->halfDepth;
		pts[19] = 1.0f;
		pts[20] = line[0];
		pts[21] = 0.0f;
		pts[22] = self->halfDepth;
		pts[23] = 1.0f;
		cb_rack_matrix(self, m);
		bdSetWorldMatrix(m);
		CB_CALL(FUN_000c7a20, void (*)(float *, int, int, unsigned int))(pts + 8, 4, 3, 0xffff0040);
		f = 0.0f;
		if (self->launchesLeft == 0)
			f = self->fillTime / DAT_00294394;
		line[4] = (line[8] - line[0]) * f + line[0];
		CB_CALL(FUN_000c7a20, void (*)(float *, int, int, unsigned int))(line, 2, 2, 0xff00ff00);
	}
	bdSetRenderState(3, 1, 0);
	cb_rack_matrix(self, m);
	bdSetWorldMatrix(m);
	for (i = 0; i < DAT_00294390; i++)
	{
		if (self->slots[i] == 0)
			continue;
		pts[0] = (line[8] - line[0]) *
			(((float)i + 2.0f) / ((float)DAT_00294390 + 4.0f)) + line[0];
		bmTransformVector4(out, m, pts);
		character = DAT_002baeac[self->slots[i] * 10];
		actor = self->actors[character];
		CB_F32(actor, 0xc0) = out[0];
		CB_F32(actor, 0xc4) = out[1];
		CB_F32(actor, 0xc8) = out[2];
		baPrepActorInstance(actor, 1);
		baDrawActorInstance(actor, 7);
	}
}

/* 0x000cb050  CarryOn/CFire_LaunchRack.cpp (file inferred)
Puts a rocket of `team` into the first free slot from the middle outwards. A full rack
starts the launches; returns 0 then. */
int CFire_LaunchRack_AddRocket(struct CFire_LaunchRack *self, int team)
{
	int i;
	int slot;

	for (i = 0; i < DAT_00294390; i++)
	{
		slot = cb_rack_slot(i);
		if (self->slots[slot] == 0)
		{
			self->slots[slot] = team;
			break;
		}
	}
	if (i != DAT_00294390)
		return 1;
	if (self->launchesLeft == 0)
		CFire_LaunchRack_StartLaunch(self);
	return 0;
}

/* 0x000cb0c0  CarryOn/CFire_LaunchRack.cpp (file inferred)
1 when `count` slots are free; else starts the launches (unless they run) and returns 0. */
int CFire_LaunchRack_HasRoom(struct CFire_LaunchRack *self, int count)
{
	int i;
	int empty = 0;

	for (i = 0; i < DAT_00294390; i++)
		if (self->slots[cb_rack_slot(i)] == 0)
			empty++;
	if (count <= empty)
		return 1;
	if (self->launchesLeft == 0)
		CFire_LaunchRack_StartLaunch(self);
	return 0;
}

/* 0x000cb130  CarryOn/CFire_LaunchRack.cpp (file inferred)
Scalar deleting destructor (vtable slot 0). */
struct CFire_LaunchRack *CFire_LaunchRack_dtor(struct CFire_LaunchRack *self, unsigned int flags)
{
	self->vtbl = vtable_00268788;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000cb150  CarryOn/CFire_LaunchRack.cpp (file inferred)
While the fill time runs, a spark runs along the fuse; when it is out (and fewer than 5
rockets fly) the launches start. While they run, two sparks run to the next slot and each
7.5 time units one slot launches its rocket. */
void CFire_LaunchRack_Update(struct CFire_LaunchRack *self, float dt, struct CFire_Sparks *sparks)
{
	float m[16];
	float p[4];
	float q[4];
	float pos[4];
	float vel[4];
	float fuse;
	float t;
	int i;
	int slot;
	int team;
	int colour;
	int r, g, b;
	int r1, r2, r3;
	int most;
	unsigned int k;

	cb_rack_matrix(self, m);
	if (self->fillTime <= 0.0f)
	{
		if (self->launchesLeft == 0 && DAT_00293b44 < 5)
		{
			CFire_LaunchRack_StartLaunch(self);
			self->hasRockets = 0;
			for (i = 0; i < DAT_00294390; i++)
			{
				if (self->slots[i] != 0)
				{
					self->hasRockets = 1;
					break;
				}
			}
		}
	}
	else
	{
		p[3] = 1.0f;
		/* the fuse: up the post (150 high), then along the rack */
		fuse = (DAT_00294394 * 150.0f) / (self->halfWidth + 150.0f);   /* 0x00262df8 */
		if (fuse <= self->fillTime)
		{
			p[1] = -175.0f;
			p[2] = -80.0f;
			p[0] = ((self->fillTime - fuse) * self->halfWidth) / (DAT_00294394 - fuse);
		}
		else
		{
			p[0] = 0.0f;
			p[1] = -25.0f - (1.0f / fuse) * self->fillTime * 150.0f;    /* 0x002687c4 */
			p[2] = (1.0f / fuse) * self->fillTime * -80.0f;              /* 0x002687c0 */
		}
		bmTransformVector4(q, m, p);
		CFire_Sparks_Emit(sparks, q, 4, 100, 0x32, 10);
		self->fillTime = self->fillTime - dt;
	}
	CFire_LaunchRack_CheckPlayers(self);
	if (self->launchesLeft == 0)
		return;
	p[1] = -25.0f;
	p[2] = 0.0f;
	p[3] = 1.0f;
	p[0] = ((self->launchTime / DAT_00294398 + (float)(DAT_00294390 - self->launchesLeft)) /
		(float)(DAT_00294390 + 2)) * self->halfWidth;
	bmTransformVector4(q, m, p);
	CFire_Sparks_Emit(sparks, q, 4, 0x80, 0x50, 0x14);
	p[0] = -p[0];
	bmTransformVector4(q, m, p);
	CFire_Sparks_Emit(sparks, q, 4, 0x80, 0x50, 0x14);
	if (self->hasRockets != 0)
		*DAT_002f7960 = 1;
	t = self->launchTime - dt;
	self->launchTime = t;
	if (!(t <= 0.0f))
		return;
	self->launchesLeft--;
	if (self->launchesLeft == DAT_00294390 / 4 && self->bigShow != 0)
		CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 0, -1, 0x80);
	k = (unsigned int)(DAT_00294390 - self->launchesLeft - 1);
	slot = cb_rack_slot((int)k);
	if (self->slots[slot] == 0)
		return;
	self->launchTime = DAT_00294398;
	CFire_LaunchRack_SlotLaunch(self, slot, pos, vel);
	team = self->slots[slot];
	if (team < 1 || team > 4)
	{
		CFire_Fireworks_LaunchRandom(self->fireworks, pos, vel);
	}
	else
	{
		/* the team colour, each channel varied by -32..31 */
		colour = DAT_002baeac[team * 10] * 0x60;
		r1 = cb_rand();
		r2 = cb_rand();
		r3 = cb_rand();
		r = (DAT_00290718[colour] - 0x20) + (r1 & 0x3f);
		g = (DAT_00290719[colour] - 0x20) + (r2 & 0x3f);
		b = (r3 & 0x3f) - 0x20 + DAT_0029071a[colour];
		if (r < 0)
			r = 0;
		if (g < 0)
			g = 0;
		if (b < 0)
			b = 0;
		most = r;
		if (r < g)
			most = g;
		if (most < b)
			most = b;
		CFire_Fireworks_Launch(self->fireworks, pos, vel, (float)((r * 0x3f) / most),
			(float)((g * 0x3f) / most), (float)((b * 0x3f) / most));
		CB_CALL(FUN_000c77c0, int (*)(void *, int, int, int))(DAT_002f6d5c, 5, -1, 0x80);
	}
	self->slots[slot] = 0;
}
