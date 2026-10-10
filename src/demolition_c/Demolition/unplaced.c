/*
Demolition/unplaced.c  <-  gap functions of C:\BlitzParty\Game\Src\Demolition\ without a known file

- CGameMode_Demolition (vtable 0x0026b96c): the game mode of Demolition, a CGameMode (core_modes_a)
  whose methods hand over to the game object DAT_002ff85c (CDemolition_Game, demolition_a).
  Probably CDemolition_Game.cpp or a CGameMode_Demolition.cpp.
- DcShape (vtable 0x0026b9a0): an outline in the XZ plane (bounds, point-in-outline test,
  circle collision), filled from the ASE reader (CDemolition_ASEImport.c).
- the world's ground height under a point, an angle turn helper, and a ray test against a
  collision mesh, used by the particles and by demolition_d.
*/

#include "demolition_c_internal.h"

/* 0x002ea488  the CharLib lists (charlib): static objects at +0, actors at +0x18 */
extern int *DAT_002ea488;

/* ======================================================================== CGameMode_Demolition */

extern const void *const vtable_0026b96c[13];

/* 0x001019c0  Demolition (gap)
Constructor: a CGameMode named "Demolition". */
struct CGameMode *CGameMode_Demolition_Construct(struct CGameMode *self)
{
	CGameMode_ctor(self, "Demolition");
	*(const void *const **)self = vtable_0026b96c;
	return self;
}

/* 0x001019e0  Demolition (gap)
Destructor. */
void CGameMode_Demolition_Destruct(struct CGameMode *self)
{
	*(const void *const **)self = vtable_0026b96c;
	CGameMode_dtor(self);
}

/* 0x001019f0  Demolition (gap)
Pushed (vtable +0x10): starts the game and its background loading. */
void CGameMode_Demolition_Pushed(struct CGameMode *self)
{
	FUN_000e7f90((int)DC_GAME);
	DC_I32(self, 0x14) = 1;
}

/* 0x00101a10  Demolition (gap)
Popped (vtable +0x0c). */
void CGameMode_Demolition_Popped(struct CGameMode *self)
{
	(void)self;
	FUN_000e9670((int)DC_GAME);
}

/* 0x00101a20  Demolition (gap)
LoadingStarted (vtable +0x2c): CDemolition_Game::Initialise. */
void CGameMode_Demolition_LoadingStarted(struct CGameMode *self)
{
	(void)self;
	FUN_000eaf80((undefined4 *)DC_GAME);
}

/* 0x00101a30  Demolition (gap)
Vtable +0x30: sets flag 1 (draws over the modes below) and hands over to the game. */
void CGameMode_Demolition_Slot30(struct CGameMode *self)
{
	DC_U32(self, 0x10) = DC_U32(self, 0x10) | 1;
	FUN_000e9870(DC_GAME);
}

/* 0x00101a40  Demolition (gap)
Suspend and Resume (vtable +0x04, +0x08): the game's empty method (0x001d5950). */
void CGameMode_Demolition_Suspend(struct CGameMode *self)
{
	(void)self;
	FUN_001d5950();
}

/* 0x00101a50  Demolition (gap)
Input (vtable +0x14). */
void CGameMode_Demolition_Input(struct CGameMode *self, int active)
{
	(void)self;
	FUN_000e8010(DC_GAME, active);
}

/* 0x00101a60  Demolition (gap)
Update (vtable +0x18). */
void CGameMode_Demolition_Update(struct CGameMode *self, int active)
{
	(void)self;
	FUN_000e9920(DC_GAME, active);
}

/* 0x00101a70  Demolition (gap)
Render (vtable +0x1c). */
void CGameMode_Demolition_Render(struct CGameMode *self, int active)
{
	(void)self;
	FUN_000e9ac0(DC_GAME, active);
}

/* 0x00101a80  Demolition (gap)
Deleting destructor (vtable +0x00). */
struct CGameMode *CGameMode_Demolition_DeletingDtor(struct CGameMode *self, unsigned char flags)
{
	CGameMode_Demolition_Destruct(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x0026b96c  (slot 8 is an empty function, 0x001ec260; slots 9 and 10 are CGameMode's) */
const void *const vtable_0026b96c[13] = {
	(const void *)CGameMode_Demolition_DeletingDtor,
	(const void *)CGameMode_Demolition_Suspend,
	(const void *)CGameMode_Demolition_Suspend,
	(const void *)CGameMode_Demolition_Popped,
	(const void *)CGameMode_Demolition_Pushed,
	(const void *)CGameMode_Demolition_Input,
	(const void *)CGameMode_Demolition_Update,
	(const void *)CGameMode_Demolition_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)CGameMode_Demolition_LoadingStarted,
	(const void *)CGameMode_Demolition_Slot30,
};

/* ======================================================================== DcShape */

extern const void *const vtable_0026b9a0[1];

/* 0x00101aa0  Demolition (gap)
Constructor: no points. */
struct DcShape *DcShape_Construct(struct DcShape *self)
{
	self->vtable = vtable_0026b9a0;
	self->points = NULL;
	self->count = 0;
	return self;
}

/* 0x00101ac0  Demolition (gap)
1 when p is inside the outline (XZ, even-odd rule), else 0. */
uint DcShape_Contains(struct DcShape *self, float *p)
{
	float *v;
	float *w;
	float x1;
	float x2;
	float z2;
	uint crossings;
	int n;
	int i;

	crossings = 0;
	if (!(self->min[0] <= p[0] && p[0] <= self->max[0] && self->min[2] <= p[2] &&
	      p[2] <= self->max[2]))
		return 0;
	n = self->count;
	i = 0;
	if (0 < n)
	{
		v = self->points[0];
		do
		{
			i = i + 1;
			x1 = v[0];
			w = self->points[i % n];
			x2 = w[0];
			z2 = w[2];
			if ((p[2] <= v[2]) != (p[2] <= z2))
			{
				if ((p[0] <= x1) == (p[0] <= x2))
				{
					if (p[0] <= x1)
						crossings = crossings + 1;
				}
				else if (p[0] <= x2 - ((z2 - p[2]) * (x1 - x2)) / (v[2] - z2))
				{
					crossings = crossings + 1;
				}
			}
			v = v + 4;
		} while (i < n);
	}
	return crossings & 1;
}

/* the highest triangle of a collision mesh under p (inlined twice in 0x00101c20); a triangle is
   0xa8 bytes: points at +0, normal at +0x30, XZ bounds at +0x40 (x) and +0x50 (z) */
static int dc_mesh_ground(int *mesh, float *p, float radius, float *best, float **tri, int check)
{
	float *t;
	float h;
	int found;
	int k;

	found = 0;
	for (k = 0; k < mesh[1]; k++)
	{
		t = (float *)(mesh[0] + k * 0xa8);
		if (p[0] - radius <= t[0x11] && t[0x10] <= radius + p[0] && p[2] - radius <= t[0x15] &&
		    t[0x14] <= radius + p[2] && (!check || -5.18969491e+11f /* 0x00267128 */ < t[0xd]) &&
		    CRColl_PointOverTriXZ(t, p) != 0)
		{
			h = ((t[0] - p[0]) * t[0xc] + (t[2] - p[2]) * t[0xe]) / t[0xd] + t[1];
			if (h < p[1] && *best < h)
			{
				*best = h;
				*tri = t;
				found = 1;
			}
		}
	}
	return found;
}

/* a mesh's XZ bounds (floats 2, 4, 6, 8 of the mesh header) overlap the circle */
static int dc_mesh_near(int *mesh, float *p, float radius)
{
	float *f = (float *)mesh;

	return f[2] <= radius + p[0] && p[0] - radius <= f[6] && f[4] <= radius + p[2] &&
	       p[2] - radius <= f[8];
}

/* 0x00101c20  Demolition (gap)
The height of the ground under p (within `radius`): the highest triangle below p of the
CharLib static objects and actors (whose collision meshes are refreshed first when flagged).
Returns -1000000 when there is none; the triangle and the actor go to `tri` and `actor`. */
float DcWorld_GroundHeight(float *p, float radius, void **tri, void **actor)
{
	float best;
	float *t;
	int *mesh;
	char *obj;

	best = -1000000.0f;
	for (obj = (char *)CLList_Next((unsigned int *)DAT_002ea488, NULL); obj != NULL;
	     obj = (char *)CLList_Next((unsigned int *)DAT_002ea488, (unsigned int *)obj))
	{
		if (DC_I32(obj, 0x21c) != 0)
			continue;
		mesh = DC_PTR(obj, 400);
		if (!dc_mesh_near(mesh, p, radius) || mesh[1] <= 0)
			continue;
		if (dc_mesh_ground(mesh, p, radius, &best, &t, 0) && tri != NULL)
			*tri = t;
	}
	for (obj = (char *)CLList_Next((unsigned int *)(DAT_002ea488 + 6), NULL); obj != NULL;
	     obj = (char *)CLList_Next((unsigned int *)(DAT_002ea488 + 6), (unsigned int *)obj))
	{
		if (DC_U8(obj, 0x3d4) != 0)
			CRColl_RefreshMesh(obj, NULL);
		if ((DC_U8(obj, 0x230) & 0x20) != 0)
			continue;
		mesh = DC_PTR(obj, 0x1a0);
		if (mesh == NULL || !dc_mesh_near(mesh, p, radius) || mesh[1] <= 0)
			continue;
		if (dc_mesh_ground(mesh, p, radius, &best, &t, 1))
		{
			if (tri != NULL)
				*tri = t;
			if (actor != NULL)
				*actor = obj;
		}
	}
	return best;
}

/* 0x00101f50  Demolition (gap)
Turns angle `from` towards `to` by at most `maxStep` (the short way round). */
float Dc_TurnTowards(float from, float to, float maxStep)
{
	float d;

	if (from == to)
		return to;
	d = to - from;
	if (3.14159250f < d) /* 0x00256750 */
		d = d - 6.28318548f;
	if (d < -3.14159250f)
		d = d + 6.28318548f;
	if (maxStep < d)
		d = maxStep;
	if (d < -maxStep)
		d = -maxStep;
	return d + from;
}

/* 0x00101fd0  Demolition (gap)
Deleting destructor (vtable slot 0); the points are not owned. */
struct DcShape *DcShape_DeletingDtor(struct DcShape *self, unsigned char flags)
{
	self->vtable = vtable_0026b9a0;
	if (self->points != NULL)
		self->points = NULL;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00102000  Demolition (gap)
Sets the outline's points, its bounds, centre and XZ radius. */
void DcShape_SetPoints(struct DcShape *self, float (*points)[4], int count)
{
	float *p;
	float dx;
	float dz;
	int i;

	self->max[0] = -99999.0f;
	self->max[1] = -99999.0f;
	self->max[2] = -99999.0f;
	self->min[0] = 99999.0f;
	self->min[1] = 99999.0f;
	self->min[2] = 99999.0f;
	self->points = points;
	self->count = count;
	for (i = 0; i < count; i++)
	{
		p = points[i];
		if (self->max[0] < p[0])
			self->max[0] = p[0];
		if (self->max[1] < p[1])
			self->max[1] = p[1];
		if (self->max[2] < p[2])
			self->max[2] = p[2];
		if (p[0] < self->min[0])
			self->min[0] = p[0];
		if (p[1] < self->min[1])
			self->min[1] = p[1];
		if (p[2] < self->min[2])
			self->min[2] = p[2];
	}
	self->centre[0] = (self->max[0] - self->min[0]) * 0.5f + self->min[0];
	self->centre[1] = (self->max[1] - self->min[1]) * 0.5f + self->min[1];
	self->centre[2] = (self->max[2] - self->min[2]) * 0.5f + self->min[2];
	dx = self->max[0] - self->centre[0];
	dz = self->max[2] - self->centre[2];
	self->radius = sqrtf(dx * dx + dz * dz);
}

/* 0x00102120  Demolition (gap)
Collides a circle (pos x, z and `radius`) with the outline's edges. Returns the number of
edges hit (at most 10 are kept); `out` (if any) gets the average of the first three floats
of the hits. */
int DcShape_Collide(struct DcShape *self, float *pos, float radius, float *out)
{
	float hits[10][8];
	float seg[4];
	float circle[3];
	float sum[3];
	float inv;
	int n;
	int i;
	int j;

	circle[0] = pos[0];
	circle[1] = pos[2];
	circle[2] = radius;
	n = 0;
	for (i = 0; i < 10; i++)
		memcpy(&hits[i][4], &(unsigned int){ 4 }, 4); /* the original sets only the first */
	for (i = 0; i < self->count;)
	{
		j = i + 1;
		seg[0] = self->points[i][0];
		seg[1] = self->points[i][2];
		seg[2] = self->points[j % self->count][0];
		seg[3] = self->points[j % self->count][2];
		if (bmCircleSweepCircle(seg, circle, (struct BHit2D *)hits[n < 10 ? n : 9]) != 0)
			n = n + 1;
		i = j;
	}
	if (n != 0 && out != NULL)
	{
		sum[0] = 0.0f;
		sum[1] = 0.0f;
		sum[2] = 0.0f;
		for (i = 0; i < n && i < 10; i++)
		{
			sum[0] = sum[0] + hits[i][0];
			sum[1] = sum[1] + hits[i][1];
			sum[2] = sum[2] + hits[i][2];
		}
		inv = 1.0f / (float)n;
		out[0] = inv * sum[0];
		out[1] = inv * sum[1];
		out[2] = inv * sum[2];
	}
	return n;
}

/* a ray and a hit of engine_misc (struct BRay, struct BCollHit) */
struct DcRay {
	float origin[4];
	float dir[4];
	float length;
};
struct DcRayHit {
	unsigned int flags;
	float t;
	float uv[2];
	float normal[4];
	float point[4];
	int code;
};

/* 0x00102310  Demolition (gap)
Casts the ray pos + t * dir (t in 0-1) at a collision mesh. On a hit, `hit` gets the point
and `normal` the surface normal; returns 1, else 0. */
int DcCollide_Ray(void *mesh, float *pos, float *dir, float *hit, float *normal)
{
	struct DcRay ray;
	struct DcRayHit h;
	int count;

	ray.origin[0] = pos[0];
	ray.origin[2] = pos[2];
	ray.origin[1] = pos[1];
	ray.origin[3] = pos[3];
	ray.dir[1] = dir[1];
	ray.dir[0] = dir[0];
	ray.dir[2] = dir[2];
	ray.dir[3] = dir[3];
	ray.length = 0.0f; /* not set by the original */
	count = 1;
	h.flags = 0xd;
	if (bmRayMesh((struct BRay *)&ray, (struct BCollMesh *)mesh, &count, (struct BCollHit *)&h) == 0)
		return 0;
	normal[0] = h.normal[0];
	normal[1] = h.normal[1];
	normal[2] = h.normal[2];
	normal[3] = h.normal[3];
	hit[0] = h.t * dir[0] + pos[0];
	hit[1] = h.t * dir[1] + pos[1];
	hit[2] = h.t * dir[2] + pos[2];
	return 1;
}

/* 0x0026b9a0 */
const void *const vtable_0026b9a0[1] = { (const void *)DcShape_DeletingDtor };
