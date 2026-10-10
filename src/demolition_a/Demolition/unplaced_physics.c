/*
Demolition/unplaced_physics.c  <-  C:\BlitzParty\Game\Src\Demolition\ (file not known)

The simulation of the game object (+0x77d4, 0xc bytes: vtable, enabled flag at
+4, first body at +8): the bombs and pieces of demolition_b are spheres that
move by their velocity and collide with the world meshes, with each other and
with the platforms. Each step sweeps a body against all three, keeps the
earliest contact kind, reports contact events (demolition_d's event queue)
and reflects the velocity; up to 10 sub-steps per frame.

The bodies are demolition_b objects; their fields are named by the PB_* byte
offsets of demolition_a.h. Gap functions of Demolition without a known file.
*/

#include "demolition_a.h"

/* a contact event (0x34 bytes) for demolition_d's queue (0x0010bea0); its vtable is set by
   0x0010bd10 */
struct DA_ContactEvent {
	uint32_t vtbl;      /* +0x00 */
	short kind;         /* +0x04 0 body, 1 wall, 2 landing */
	short pad06;
	int replace;        /* +0x08 replace an event of the same kind */
	int priority;       /* +0x0c */
	uint32_t data[9];   /* +0x10 */
};

#define PB_VEC(body, off) ((float *)DA_AT((body), (off)))

/* the engine's sweep tests read this epsilon; the simulation uses 0.05 */
#define DA_SIM_EPSILON 0.05f

/* true when every component of BODY's velocity is within the epsilon */
static int da_BodyResting(char *body)
{
	float *v = PB_VEC(body, PB_VEL);

	return fabsf(v[0]) <= DAT_00285644 && fabsf(v[1]) <= DAT_00285644 && fabsf(v[2]) <= DAT_00285644;
}

/* 0x000ec560
Sweeps BODY against every other active body of the simulation. A hit (up to N) records the
other body, the hit data (first word 9) and the normalised direction from the other body.
A body at rest stops (velocity 0) and nothing is recorded. Returns the number of hits (also
stored at PB_BODYCOUNT). */
int CDemoSimulation_CollideBodies(void *self, void *bodyArg)
{
	char *body = (char *)bodyArg;
	char *other;
	float *data;
	float dir[4];
	int count = 0;

	DAT_00285644 = DA_SIM_EPSILON;
	for (other = DA_FIELD(char *, self, 8); other != NULL; other = DA_FIELD(char *, other, PB_NEXT))
	{
		if (other == body || DA_FIELD(char, other, PB_ACTIVE) == 0)
			continue;
		DA_FIELD(int, body, PB_BODYDATA + count * 0xd0) = 9;
		if (da_BodyResting(body))
		{
			PB_VEC(body, PB_VEL)[0] = 0.0f;
			PB_VEC(body, PB_VEL)[1] = 0.0f;
			PB_VEC(body, PB_VEL)[2] = 0.0f;
			return 0;
		}
		memcpy(DA_AT(other, PB_SPHERE), DA_AT(other, PB_POS), 16);
		if (da_BodyResting(other))
			continue;
		if (DA_SweepSphereSphere(DA_AT(body, PB_SPHERE), DA_AT(other, PB_SPHERE), PB_VEC(body, PB_VEL),
				DA_AT(body, PB_BODYDATA + count * 0xd0)) == 0)
			continue;
		dir[0] = PB_VEC(body, PB_POS)[0] - PB_VEC(other, PB_POS)[0];
		dir[1] = PB_VEC(body, PB_POS)[1] - PB_VEC(other, PB_POS)[1];
		dir[2] = PB_VEC(body, PB_POS)[2] - PB_VEC(other, PB_POS)[2];
		dir[3] = 0.0f; /* not set by the original */
		DA_VectorNormalise(dir, dir);
		data = (float *)DA_AT(body, PB_BODYDATA + 0x10 + count * 0xd0);
		data[0] = dir[0];
		data[1] = dir[1];
		data[2] = dir[2];
		data[3] = dir[3];
		DA_FIELD(char *, body, PB_BODYHIT + count * 4) = other;
		count++;
	}
	DA_FIELD(int, body, PB_BODYCOUNT) = count;
	return count;
}

/* 0x000ec790
Sweeps BODY against every enabled world mesh (the world's mesh table: count at game +0x7bc,
0x90-byte entries from game +0x21c: enabled flag, offset at +0x34, mesh at +0x88). Records
per mesh whether it was hit (PB_MESHHIT) with its hits; sets PB_HIT on a hit. A body at rest
stops and 0 is returned. Returns the number of meshes hit. */
int CDemoSimulation_CollideWorld(void *self, void *bodyArg)
{
	char *body = (char *)bodyArg;
	char *mesh = DA_GAME + 0x21c;
	int meshes = DA_FIELD(int, DA_GAME, 0x7bc);
	float pos[3];
	struct {
		float centre[4];
		float radius;
	} sphere;
	int hits = 0;
	int i;

	(void)self;
	DAT_00285644 = DA_SIM_EPSILON;
	pos[0] = PB_VEC(body, PB_POS)[0];
	pos[1] = PB_VEC(body, PB_POS)[1];
	pos[2] = PB_VEC(body, PB_POS)[2];
	sphere.radius = DA_FIELD(float, body, PB_SPHERE + 0x10);
	sphere.centre[3] = 1.0f; /* not set by the original */
	for (i = 0; i < meshes; i++, mesh += 0x90)
	{
		if (DA_FIELD(char, mesh, 0) == 0)
		{
			DA_FIELD(int, body, PB_MESHHIT + i * 4) = 0;
			continue;
		}
		sphere.centre[0] = pos[0] + DA_FIELD(float, mesh, 0x34);
		sphere.centre[1] = pos[1] + DA_FIELD(float, mesh, 0x38);
		sphere.centre[2] = pos[2] + DA_FIELD(float, mesh, 0x3c);
		DA_FIELD(int, body, PB_MESHDATA + i * 0x618) = 0xd;
		DA_FIELD(int, body, PB_MESHHITS + i * 4) = 0x1e;
		if (da_BodyResting(body))
		{
			PB_VEC(body, PB_VEL)[0] = 0.0f;
			PB_VEC(body, PB_VEL)[1] = 0.0f;
			PB_VEC(body, PB_VEL)[2] = 0.0f;
			return 0;
		}
		if (DA_SweepSphereMesh(&sphere, DA_FIELD(void *, mesh, 0x88), PB_VEC(body, PB_VEL),
				DA_AT(body, PB_MESHHITS + i * 4), DA_AT(body, PB_MESHDATA + i * 0x618)) != 0)
		{
			DA_FIELD(char, body, PB_HIT) = 1;
			hits++;
			DA_FIELD(int, body, PB_MESHHIT + i * 4) = 1;
		}
		else
		{
			DA_FIELD(int, body, PB_MESHHIT + i * 4) = 0;
		}
	}
	return hits;
}

/* 0x000ec950
Collides BODY with the enabled platforms (the platform list of game +0x79a4), at most three
hits (0xf0 bytes each from PB_PLATDATA). A body at rest stops and 0 is returned. Returns the
number of hits (also stored at PB_PLATCOUNT). */
int CDemoSimulation_CollidePlatforms(void *self, void *bodyArg)
{
	char *body = (char *)bodyArg;
	char *platform;
	int offset = 0;
	int count = 0;
	int one;

	(void)self;
	platform = DA_FIELD(char *, DA_GAME, DG_PLATFORMS + 4);
	DAT_00285644 = DA_SIM_EPSILON;
	for (; platform != NULL; platform = DA_FIELD(char *, platform, 4))
	{
		if (DA_FIELD(char, platform, 0x1c) == 0)
			continue;
		DA_FIELD(int, body, PB_PLATDATA + offset) = 5;
		if (da_BodyResting(body))
		{
			PB_VEC(body, PB_VEL)[0] = 0.0f;
			PB_VEC(body, PB_VEL)[1] = 0.0f;
			PB_VEC(body, PB_VEL)[2] = 0.0f;
			return 0;
		}
		PB_VEC(body, PB_VEL)[3] = 1.0f;
		one = 1;
		if (DA_CollideAnimating(DA_AT(body, PB_SPHERE), DA_AT(platform, 0x28), PB_VEC(body, PB_VEL), &one,
				DA_AT(body, PB_PLATDATA + offset)) != 0)
		{
			count++;
			DA_FIELD(char, body, PB_HIT) = 1;
			offset += 0xf0;
		}
		if (offset >= 0x2d0)
			break;
	}
	DA_FIELD(int, body, PB_PLATCOUNT) = count;
	return count;
}

/* removes the part of the velocity V along the unit normal N, then the restitution part
   (V -= N (V.N) (1 + F)), and stores N as the last contact normal of BODY */
static void da_Reflect(char *body, const float *n, float nw, float f)
{
	float *v = PB_VEC(body, PB_VEL);
	float d, tx, ty, tz, rx, ry, rz;

	d = n[0] * v[0] + n[1] * v[1] + n[2] * v[2];
	tx = n[0] * d;
	ty = n[1] * d;
	tz = n[2] * d;
	rx = v[0] - tx;
	ry = v[1] - ty;
	rz = v[2] - tz;
	v[0] = rx - f * tx;
	v[1] = ry - ty * f;
	v[2] = rz - tz * f;
	PB_VEC(body, PB_NORMAL)[0] = n[0];
	PB_VEC(body, PB_NORMAL)[1] = n[1];
	PB_VEC(body, PB_NORMAL)[2] = n[2];
	PB_VEC(body, PB_NORMAL)[3] = nw;
}

/* sends a wall (kind 1) or landing (kind 2) event with BODY's velocity and the normal N */
static void da_SendSurfaceEvent(char *body, struct DA_ContactEvent *ev, short kind, const float *n, float nw)
{
	float *v = PB_VEC(body, PB_VEL);

	ev->kind = kind;
	ev->replace = 1;
	ev->priority = 2;
	memcpy(&ev->data[0], &v[0], 4);
	memcpy(&ev->data[1], &v[1], 4);
	memcpy(&ev->data[2], &v[2], 4);
	memcpy(&ev->data[3], &v[3], 4);
	memcpy(&ev->data[4], &n[0], 4);
	memcpy(&ev->data[5], &n[1], 4);
	memcpy(&ev->data[6], &n[2], 4);
	memcpy(&ev->data[7], &nw, 4);
	DA_ContactAdd(body, ev);
}

/* 0x000ecaa0
The response of BODY to its contacts (PB_CONTACTS: 1 world, 2 bodies, 3 platforms). The
earliest of the three kinds (hit time < 1) is resolved:
- world: the normals of the earliest hits of each hit mesh are averaged; a steep normal
  (y <= 0.6) sends a wall event, a long fall (air time >= 10) a landing event; the velocity
  reflects with the friction (PB_FRICTION);
- bodies: per hit body the velocity reflects with PB_BOUNCE, the other body is pushed along
  the normal with PB_PUSH, and a body event is sent;
- platforms: per hit as for the world; a fall almost straight into the platform (velocity
  direction . normal <= -0.95) first moves the body to the contact.
Returns the contact flags, or 0 when no kind was resolved. */
int CDemoSimulation_Respond(void *self, void *bodyArg)
{
	char *body = (char *)bodyArg;
	struct DA_ContactEvent ev;
	int meshes = DA_FIELD(int, DA_GAME, 0x7bc);
	unsigned int flags = (unsigned int)DA_FIELD(int, body, PB_CONTACTS);
	unsigned int meshFlag = flags & 1, bodyFlag = flags & 2, platFlag = flags & 3;
	float minMesh = 1.0f, minBody = 1.0f, minPlat = 1.0f, best;
	float avg[3], sum[3], scale, nv[4], n[3], nw;
	float *entry;
	int meshHits = 0, kind, i, j, k, used;
	char *other;

	DA_ContactsCtor(&ev);

	/* the earliest hit of each kind */
	if (meshFlag != 0)
	{
		for (i = 0; i < meshes; i++)
		{
			if (DA_FIELD(int, body, PB_MESHHIT + i * 4) != 0)
			{
				if (!(minMesh <= DA_FIELD(float, body, 0x78 + i * 0x618)))
					minMesh = DA_FIELD(float, body, 0x78 + i * 0x618);
				meshHits++;
			}
		}
	}
	if (bodyFlag != 0)
	{
		for (k = 0; k < DA_FIELD(int, body, PB_BODYCOUNT); k++)
		{
			if (DA_FIELD(float, body, 0x3d7c + k * 0xd0) < minBody)
				minBody = DA_FIELD(float, body, 0x3d7c + k * 0xd0);
		}
	}
	if (platFlag != 0)
	{
		for (k = 0; k < DA_FIELD(int, body, PB_PLATCOUNT); k++)
		{
			if (DA_FIELD(float, body, 0x40c4 + k * 0xf0) < minPlat)
				minPlat = DA_FIELD(float, body, 0x40c4 + k * 0xf0);
		}
	}

	/* the kind with the earliest time (when none is earlier than 1, the original falls back to
	   the value of the body flag: 0 or 2) */
	kind = (int)bodyFlag;
	best = 1.0f;
	if (meshFlag != 0 && minMesh < best)
	{
		kind = 0;
		best = minMesh;
	}
	if (bodyFlag != 0 && minBody < best)
	{
		kind = 1;
		best = minBody;
	}
	if (platFlag != 0 && minPlat < best)
	{
		kind = 2;
		best = minPlat;
	}

	switch (kind)
	{
	case 0:
		avg[0] = 0.0f;
		avg[1] = 0.0f;
		avg[2] = 0.0f;
		for (i = 0; i < meshes; i++)
		{
			if (DA_FIELD(int, body, PB_MESHHIT + i * 4) == 0)
				continue;
			entry = (float *)DA_AT(body, 0x84 + i * 0x618);
			sum[0] = entry[0];
			sum[1] = entry[1];
			sum[2] = entry[2];
			used = 1;
			for (j = 1; j < DA_FIELD(int, body, PB_MESHHITS + i * 4); j++)
			{
				if (minMesh < entry[j * 13 - 3])
					break;
				sum[0] += entry[j * 13];
				sum[1] += entry[j * 13 + 1];
				sum[2] += entry[j * 13 + 2];
				used = j + 1;
			}
			scale = 1.0f / (float)used;
			avg[0] += sum[0] * scale;
			avg[1] += sum[1] * scale;
			avg[2] += sum[2] * scale;
		}
		scale = 1.0f / (float)meshHits;
		avg[0] *= scale;
		avg[1] *= scale;
		avg[2] *= scale;
		nw = 0.0f; /* the original sends an unset stack value */
		if (avg[1] <= 0.6f)
			da_SendSurfaceEvent(body, &ev, 1, avg, nw);
		if (DA_FIELD(float, body, PB_AIRTIME) >= 10.0f)
			da_SendSurfaceEvent(body, &ev, 2, avg, nw);
		DA_VectorNormalise(nv, PB_VEC(body, PB_VEL));
		da_Reflect(body, avg, nw, DA_FIELD(float, body, PB_FRICTION));
		break;

	case 1:
		for (k = 0; k < DA_FIELD(int, body, PB_BODYCOUNT); k++)
		{
			float *v = PB_VEC(body, PB_VEL);
			float d, tx, ty, tz, rz, b, p;

			entry = (float *)DA_AT(body, 0x3d88 + k * 0xd0);
			n[0] = entry[0];
			n[1] = entry[1];
			n[2] = entry[2];
			nw = entry[3];
			other = DA_FIELD(char *, body, PB_BODYHIT + k * 4);
			d = n[0] * v[0] + n[1] * v[1] + n[2] * v[2];
			tx = n[0] * d;
			ty = n[1] * d;
			tz = n[2] * d;
			rz = v[2] - tz;
			b = DA_FIELD(float, body, PB_BOUNCE);
			v[0] = (v[0] - tx) - tx * b;
			v[1] = (v[1] - ty) - ty * b;
			v[2] = rz - tz * b;
			p = DA_FIELD(float, body, PB_PUSH);
			PB_VEC(other, PB_VEL)[0] += tx * p;
			PB_VEC(other, PB_VEL)[1] += ty * p;
			PB_VEC(other, PB_VEL)[2] += tz * p;
			ev.kind = 0;
			ev.replace = 0;
			ev.priority = 2;
			ev.data[0] = (uint32_t)(uintptr_t)other;
			memcpy(&ev.data[1], &v[0], 4);
			memcpy(&ev.data[2], &v[1], 4);
			memcpy(&ev.data[3], &v[2], 4);
			memcpy(&ev.data[4], &v[3], 4);
			memcpy(&ev.data[5], &n[0], 4);
			memcpy(&ev.data[6], &n[1], 4);
			memcpy(&ev.data[7], &n[2], 4);
			memcpy(&ev.data[8], &nw, 4);
			DA_ContactAdd(body, &ev);
		}
		break;

	case 2:
		for (k = 0; k < DA_FIELD(int, body, PB_PLATCOUNT); k++)
		{
			float *v = PB_VEC(body, PB_VEL);

			entry = (float *)DA_AT(body, 0x40dc + k * 0xf0);
			n[0] = entry[0];
			n[1] = entry[1];
			n[2] = entry[2];
			nw = entry[3];
			if (n[1] <= 0.6f)
				da_SendSurfaceEvent(body, &ev, 1, n, nw);
			if (DA_FIELD(float, body, PB_AIRTIME) >= 10.0f)
				da_SendSurfaceEvent(body, &ev, 2, n, nw);
			DA_VectorNormalise(nv, v);
			if (nv[0] * n[0] + nv[1] * n[1] + nv[2] * n[2] <= -0.95f)
			{
				PB_VEC(body, PB_POS)[0] += minPlat * v[0];
				PB_VEC(body, PB_POS)[1] += minPlat * v[1];
				PB_VEC(body, PB_POS)[2] += minPlat * v[2];
			}
			da_Reflect(body, n, nw, DA_FIELD(float, body, PB_FRICTION));
		}
		break;

	default:
		DA_ContactsDtor(&ev);
		return 0;
	}
	flags = (unsigned int)DA_FIELD(int, body, PB_CONTACTS);
	DA_ContactsDtor(&ev);
	return (int)flags;
}

/* 0x000ed470
A step of DT: for every enabled, active body: its air time grows unless it touched something
last step; its own pre-step and step (demolition_b); then up to 10 rounds of sweeps and
responses. When a round finds nothing (or nothing is resolved), the body moves by its
velocity. Then its post-step. */
void CDemoSimulation_Step(void *self, float dt)
{
	char *body;
	int round;
	unsigned int contacts;

	for (body = DA_FIELD(char *, self, 8); body != NULL; body = DA_FIELD(char *, body, PB_NEXT))
	{
		if (DA_FIELD(char, body, PB_ENABLED) == 0 || DA_FIELD(char, body, PB_ACTIVE) == 0)
			continue;
		if (DA_FIELD(char, body, PB_HIT) == 0)
			DA_FIELD(float, body, PB_AIRTIME) = dt + DA_FIELD(float, body, PB_AIRTIME);
		else
			DA_FIELD(float, body, PB_AIRTIME) = 0.0f;
		DA_FIELD(char, body, PB_HIT) = 0;
		DA_BodyPreStep(body, dt);
		DA_BodyStep(body, dt);
		memcpy(DA_AT(body, PB_OLDPOS), DA_AT(body, PB_POS), 16);
		for (round = 0; round < 10; round++)
		{
			DA_FIELD(int, body, PB_CONTACTS) = 0;
			memcpy(DA_AT(body, PB_SPHERE), DA_AT(body, PB_POS), 16);
			DA_FIELD(float, body, PB_SPHERE + 0xc) = 1.0f;
			if (CDemoSimulation_CollideWorld(self, body) != 0)
				DA_FIELD(unsigned int, body, PB_CONTACTS) |= 1;
			if (CDemoSimulation_CollideBodies(self, body) != 0)
				DA_FIELD(unsigned int, body, PB_CONTACTS) |= 2;
			if (CDemoSimulation_CollidePlatforms(self, body) != 0)
				DA_FIELD(unsigned int, body, PB_CONTACTS) |= 3;
			contacts = 0;
			if (DA_FIELD(int, body, PB_CONTACTS) != 0)
			{
				contacts = (unsigned int)CDemoSimulation_Respond(self, body);
				DA_FIELD(int, body, PB_CONTACTS) = (int)contacts;
			}
			if (contacts == 0)
			{
				DA_Nop(body);
				PB_VEC(body, PB_POS)[0] = PB_VEC(body, PB_POS)[0] + PB_VEC(body, PB_VEL)[0];
				PB_VEC(body, PB_POS)[1] = PB_VEC(body, PB_POS)[1] + PB_VEC(body, PB_VEL)[1];
				PB_VEC(body, PB_POS)[2] = PB_VEC(body, PB_POS)[2] + PB_VEC(body, PB_VEL)[2];
				memcpy(DA_AT(body, PB_SPHERE), DA_AT(body, PB_POS), 16);
				DA_FIELD(float, body, PB_SPHERE + 0xc) = 1.0f;
				break;
			}
		}
		DA_BodyPostStep(body);
	}
}

/* 0x000ed620
Update: a step of DT while the simulation is enabled (+4), else a step of 0. Called by
demolition_b. */
void CDemoSimulation_Update(void *self, float dt)
{
	if (DA_FIELD(char, self, 4) == 0)
		dt = 0.0f;
	CDemoSimulation_Step(self, dt);
}
