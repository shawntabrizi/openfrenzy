/*
BMATHS/unplaced.c: Babel's collision tests (C:\Babel\Xbox\Src\BMATHS\, file unknown):
rays and swept spheres against spheres, triangles, edges and collision meshes with a
cell grid. 0x00052700-0x000545d0; the function table puts them in BSOUND's gap (they
lie between animatingCollision.cpp and the sound code), but they are maths.

The original computes on the x87 stack (80-bit); this code computes in float (double
where Ghidra shows 80-bit intermediates), so the last bits of results can differ.
*/

#include <math.h>
#include <stdint.h>

#include "engine_misc_maths.h"
#include "engine_misc_extern.h"

#define EPS 0.001f    /* 0x00285644 */
#define EPS2 0.002f   /* 0x0028564c */

/* 0x00051540 (module bkernel): the squared distance of two points, returned in ST0.
bkernel's port defines that address as an empty function (bmNothing2), which would leave
the result undefined, so the same computation is done here. */
static double dist_sq(const float *a, const float *b)
{
	double dx = (double)a[0] - b[0];
	double dy = (double)a[1] - b[1];
	double dz = (double)a[2] - b[2];

	return dz * dz + dy * dy + dx * dx;
}

/* the sign bit of a float, as the original tests it */
static unsigned int sign_bit(float f)
{
	unsigned int bits;

	memcpy(&bits, &f, 4);
	return bits & 0x80000000u;
}

/* 0x00052700  BMATHS (unplaced)
Fixes up a collision mesh found in a package. */
void bmCollMeshFixup(int package, int name)
{
	struct BCollMesh *mesh;
	int n, i;

	mesh = (struct BCollMesh *)em_bkFindResource(package, name, 0, 0, 0, 0);
	if (mesh == NULL)
		return;
	mesh->tris = (unsigned short (*)[3])((char *)mesh->tris + (uintptr_t)mesh);
	n = mesh->cells[2] * mesh->cells[1] * mesh->cells[0];
	mesh->verts = (float (*)[3])((char *)mesh->verts + (uintptr_t)mesh);
	mesh->grid = (struct BCollCell *)((char *)mesh->grid + (uintptr_t)mesh);
	for (i = 0; i < n; i++)
		mesh->grid[i].list = (unsigned short *)((char *)mesh->grid[i].list + (uintptr_t)mesh);
}

/* 0x00052770  BMATHS (unplaced)
A ray (unit direction, length) against a sphere. */
int bmRaySphere(struct BRay *ray, struct BSphere *sphere, struct BCollHit *hit)
{
	float dx, dy, dz, proj, disc, t, inv;
	float p[4];

	dx = sphere->centre[0] - ray->origin[0];
	dy = sphere->centre[1] - ray->origin[1];
	dz = sphere->centre[2] - ray->origin[2];
	proj = dx * ray->dir[0] + dy * ray->dir[1] + dz * ray->dir[2];
	disc = sphere->radius * sphere->radius - ((dx * dx + dy * dy + dz * dz) - proj * proj);
	if (disc < 0.0f || (t = proj - sqrtf(disc)) < -EPS) {
		if (hit->flags & BCOLL_REASON) {
			hit->code = 0x200;
			return 0;
		}
	} else {
		if (t < EPS) {
			/* already touching */
			if (hit->flags & BCOLL_T)
				hit->t = 0.0f;
			if (hit->flags & BCOLL_NORMAL) {
				hit->normal[0] = ray->origin[0] - sphere->centre[0];
				hit->normal[1] = ray->origin[1] - sphere->centre[1];
				hit->normal[2] = ray->origin[2] - sphere->centre[2];
				inv = 1.0f / sphere->radius;
				hit->normal[0] = inv * hit->normal[0];
				hit->normal[1] = inv * hit->normal[1];
				hit->normal[2] = inv * hit->normal[2];
			}
			if (hit->flags & BCOLL_POINT) {
				hit->point[0] = ray->origin[0];
				hit->point[1] = ray->origin[1];
				hit->point[2] = ray->origin[2];
				hit->point[3] = ray->origin[3];
			}
			return 1;
		}
		if (t <= ray->length) {
			p[0] = t * ray->dir[0] + ray->origin[0];
			p[1] = t * ray->dir[1] + ray->origin[1];
			p[2] = t * ray->dir[2] + ray->origin[2];
			p[3] = 0.0f; /* the original leaves this word uninitialised */
			if (hit->flags & BCOLL_T)
				hit->t = t / ray->length;
			if (hit->flags & BCOLL_NORMAL) {
				em_bmVecSub(hit->normal, p, sphere->centre);
				em_bmVecDivide(hit->normal, hit->normal, sphere->radius);
			}
			if (hit->flags & BCOLL_POINT) {
				hit->point[0] = p[0];
				hit->point[1] = p[1];
				hit->point[2] = p[2];
				hit->point[3] = p[3];
			}
			return 1;
		}
		if (hit->flags & BCOLL_REASON)
			hit->code = 0x100;
	}
	return 0;
}

/* 0x00052960  BMATHS (unplaced)
A ray (origin, delta) against a collision mesh: tests the triangles of the cells the
ray's box covers; keeps up to *count hits (each triangle once) sorted by t. */
int bmRayMesh(struct BRay *ray, struct BCollMesh *mesh, int *count, struct BCollHit *hits)
{
	struct BCollHit hit;
	struct BTriangle tri;
	struct BCollCell *cell;
	double inv[3], d;
	int lo[3], hi[3];
	int k, x, y, z, j, i, n, tri_index;

	hit.flags = hits->flags;
	n = 0;
	for (k = 0; k < 3; k++) {
		inv[k] = 1.0 / mesh->cellSize[k];
		d = (double)ray->origin[k] - mesh->origin[k];
		if (ray->dir[k] < 0.0f) {
			lo[k] = (int)((d + ray->dir[k]) * inv[k]);
			hi[k] = (int)(d * inv[k]);
		} else {
			lo[k] = (int)(d * inv[k]);
			hi[k] = (int)((d + ray->dir[k]) * inv[k]);
		}
		if (lo[k] < 0)
			lo[k] = 0;
		if (mesh->cells[k] <= hi[k])
			hi[k] = mesh->cells[k] - 1;
	}
	for (x = lo[0]; x <= hi[0]; x++) {
		for (y = lo[1]; y <= hi[1]; y++) {
			for (z = lo[2]; z <= hi[2]; z++) {
				cell = &mesh->grid[(mesh->cells[1] * x + y) * mesh->cells[2] + z];
				for (j = 0; j < cell->count; j++) {
					tri_index = (unsigned short)(cell->list[j] + cell->base);
					for (i = 0; i < n; i++) {
						if (tri_index == hits[i].code)
							break;
					}
					if (i < n)
						continue;
					memcpy(tri.v[0], mesh->verts[mesh->tris[tri_index][0]], 12);
					memcpy(tri.v[1], mesh->verts[mesh->tris[tri_index][1]], 12);
					memcpy(tri.v[2], mesh->verts[mesh->tris[tri_index][2]], 12);
					if (bmRayTriangle(ray, &tri, &hit) == 0)
						continue;
					if (n == *count) {
						if (hits[n - 1].t < hit.t)
							continue;
						n--;
					}
					i = n;
					while (i > 0 && hit.t <= hits[i - 1].t) {
						hits[i] = hits[i - 1];
						i--;
					}
					hit.code = tri_index;
					hits[i] = hit;
					n++;
				}
			}
		}
	}
	*count = n;
	return n > 0;
}

/* 0x00052ce0  BMATHS (unplaced)
Which side of a plane {n, d} a sphere moving by delta stays on: +1 or -1 when it stays
clear of the plane, 0 when it touches or crosses it. */
int bmSphereSideOfPlane(struct BSphere *sphere, float *delta, float *plane)
{
	float r, dist, end;

	r = EPS + sphere->radius;
	dist = sphere->centre[0] * plane[0] + sphere->centre[1] * plane[1] + sphere->centre[2] * plane[2] + plane[3];
	if (r < fabsf(dist)) {
		end = delta[0] * plane[0] + delta[1] * plane[1] + delta[2] * plane[2] + dist;
		if (sign_bit(dist) == sign_bit(end)) {
			end = fabsf(end);
			if (!(end < r) && !(end == r))
				return (sign_bit(dist) == 0) * 2 - 1;
		}
	}
	return 0;
}

/* 0x00052da0  BMATHS (unplaced)
Sphere a moving along a unit direction for length against sphere b. */
int bmSweepSphereSphere(struct BSphere *a, struct BSphere *b, float *dir, float length, struct BCollHit *hit)
{
	struct BSphere sum;
	struct BRay ray;
	float scale;

	sum.centre[0] = b->centre[0];
	sum.centre[1] = b->centre[1];
	sum.centre[2] = b->centre[2];
	sum.centre[3] = b->centre[3];
	ray.origin[0] = a->centre[0];
	ray.origin[1] = a->centre[1];
	sum.radius = a->radius + b->radius;
	ray.origin[2] = a->centre[2];
	ray.origin[3] = a->centre[3];
	ray.dir[0] = dir[0];
	ray.dir[1] = dir[1];
	ray.dir[2] = dir[2];
	ray.dir[3] = dir[3];
	ray.length = length;
	if (bmRaySphere(&ray, &sum, hit) == 0)
		return 0;
	if (hit->flags & BCOLL_POINT) {
		hit->point[0] = hit->point[0] - b->centre[0];
		hit->point[1] = hit->point[1] - b->centre[1];
		hit->point[2] = hit->point[2] - b->centre[2];
		scale = b->radius / sum.radius;
		hit->point[0] = scale * hit->point[0];
		hit->point[1] = scale * hit->point[1];
		hit->point[2] = scale * hit->point[2];
		hit->point[0] = hit->point[0] + b->centre[0];
		hit->point[1] = hit->point[1] + b->centre[1];
		hit->point[2] = hit->point[2] + b->centre[2];
	}
	return 1;
}

/* 0x00052ea0  BMATHS (unplaced)
A segment {origin, delta} against a sphere. */
void bmSegmentSphere(float *segment, struct BSphere *sphere, struct BCollHit *hit)
{
	struct BRay ray;
	float inv;

	ray.origin[0] = segment[0];
	ray.origin[1] = segment[1];
	ray.origin[2] = segment[2];
	ray.origin[3] = segment[3];
	ray.length = sqrtf(segment[6] * segment[6] + segment[5] * segment[5] + segment[4] * segment[4]);
	inv = 1.0f / ray.length;
	ray.dir[0] = inv * segment[4];
	ray.dir[1] = inv * segment[5];
	ray.dir[2] = inv * segment[6];
	ray.dir[3] = 0.0f; /* not set by the original */
	bmRaySphere(&ray, sphere, hit);
}

/* 0x00052f30  BMATHS (unplaced)
A sphere moving by delta against a triangle: its face, then its three edges. */
int bmSweepSphereTriangle(struct BSphere *sphere, struct BTriangle *tri, float *delta, struct BCollHit *hit)
{
	float e1[3], e2[3], n[4], plane[4], p[3], s[3], q[3], tmpv[3];
	float det, r, u, v, invDet, t, uu, vv;
	struct BEdge edge;
	struct BCollHit tmp;
	int bad, found;

	e1[0] = tri->v[1][0] - tri->v[0][0];
	e1[1] = tri->v[1][1] - tri->v[0][1];
	bad = 0;
	e1[2] = tri->v[1][2] - tri->v[0][2];
	e2[0] = tri->v[2][0] - tri->v[0][0];
	e2[1] = tri->v[2][1] - tri->v[0][1];
	e2[2] = tri->v[2][2] - tri->v[0][2];
	n[0] = e2[2] * e1[1] - e2[1] * e1[2];
	n[1] = e1[2] * e2[0] - e2[2] * e1[0];
	n[2] = e2[1] * e1[0] - e1[1] * e2[0];
	n[3] = 0.0f; /* the original leaves this word uninitialised */
	em_bmVecNormalise(n, n);
	plane[0] = n[0];
	plane[1] = n[1];
	plane[2] = n[2];
	plane[3] = -(n[2] * tri->v[0][2] + n[1] * tri->v[0][1] + n[0] * tri->v[0][0]);
	if (bmSphereSideOfPlane(sphere, delta, plane) != 0)
		return 0;
	p[0] = e2[2] * delta[1] - e2[1] * delta[2];
	p[1] = e2[0] * delta[2] - e2[2] * delta[0];
	p[2] = e2[1] * delta[0] - e2[0] * delta[1];
	det = p[0] * e1[0] + p[1] * e1[1] + p[2] * e1[2];
	r = sphere->radius;
	if (0.0f <= det) {
		s[0] = (sphere->centre[0] - tri->v[0][0]) - n[0] * r;
		s[1] = (sphere->centre[1] - tri->v[0][1]) - n[1] * r;
		s[2] = (sphere->centre[2] - tri->v[0][2]) - n[2] * r;
	} else {
		s[0] = (sphere->centre[0] - tri->v[0][0]) + n[0] * r;
		s[1] = n[1] * r + (sphere->centre[1] - tri->v[0][1]);
		s[2] = n[2] * r + (sphere->centre[2] - tri->v[0][2]);
	}
	q[0] = s[1] * e1[2] - s[2] * e1[1];
	q[1] = e1[0] * s[2] - e1[2] * s[0];
	q[2] = e1[1] * s[0] - s[1] * e1[0];
	u = 0.0f;
	v = 0.0f;
	invDet = 0.0f;
	if (det <= EPS) {
		if (-EPS <= det) {
			bad = 1;
		} else {
			u = p[0] * s[0] + p[1] * s[1] + p[2] * s[2];
			if (0.0f < u || u < det)
				bad = 1;
			v = q[1] * delta[1] + q[2] * delta[2] + q[0] * delta[0];
			if (v <= 0.0f && det <= v + u) {
				if (!bad)
					invDet = 1.0f / det;
			} else {
				bad = 1;
			}
		}
	} else {
		u = p[0] * s[0] + p[1] * s[1] + p[2] * s[2];
		if (u < 0.0f || det < u)
			bad = 1;
		v = q[1] * delta[1] + q[2] * delta[2] + q[0] * delta[0];
		if (v < 0.0f || det < v + u)
			bad = 1;
		else if (!bad)
			invDet = 1.0f / det;
	}
	found = 0;
	tmp.flags = hit->flags;
	if (!bad) {
		t = (q[0] * e2[0] + q[1] * e2[1] + q[2] * e2[2]) * invDet;
		hit->t = t;
		if (!(-EPS2 <= t)) {
			bad = 1;
			hit->t = 1.0f;
		}
	} else {
		hit->t = 1.0f;
	}

	/* edge v0-v1 */
	memcpy(edge.a, tri->v[0], 16);
	memcpy(edge.b, tri->v[1], 16);
	if (bmSweepSphereEdge(sphere, &edge, delta, &tmp) != 0 && tmp.t < hit->t) {
		*hit = tmp;
		found = 1;
	}
	/* edge v0-v2 */
	memcpy(edge.a, tri->v[0], 16);
	memcpy(edge.b, tri->v[2], 16);
	if (bmSweepSphereEdge(sphere, &edge, delta, &tmp) != 0 && tmp.t < hit->t) {
		found = 1;
		if (hit->flags & BCOLL_UV) {
			tmp.v = tmp.u;
			tmp.u = 0.0f;
		}
		*hit = tmp;
	}
	/* edge v1-v2 */
	memcpy(edge.a, tri->v[1], 16);
	memcpy(edge.b, tri->v[2], 16);
	if (bmSweepSphereEdge(sphere, &edge, delta, &tmp) == 0 || hit->t <= tmp.t) {
		if (found == 0) {
			/* the face */
			if (bad) {
				if (hit->flags & BCOLL_REASON)
					hit->code = 0x800;
				return 0;
			}
			if (EPS2 <= hit->t) {
				if (1.0f < hit->t) {
					if (hit->flags & BCOLL_REASON)
						hit->code = 0x100;
					return 0;
				}
				hit->t = hit->t - EPS2;
			} else if (-EPS2 < hit->t) {
				hit->t = 0.0f;
			}
			uu = invDet * u;
			vv = invDet * v;
			if (hit->flags & BCOLL_UV) {
				hit->u = uu;
				hit->v = vv;
			}
			if (hit->flags & BCOLL_POINT) {
				hit->point[0] = e2[0] * vv + e1[0] * uu;
				hit->point[1] = e2[1] * vv + e1[1] * uu;
				hit->point[2] = e2[2] * vv + e1[2] * uu;
				em_bmVecAdd(hit->point, hit->point, tri->v[0]);
			}
			if ((hit->flags & BCOLL_NORMAL) == 0)
				return 1;
			if (0.0f <= det) {
				hit->normal[0] = n[0];
				hit->normal[1] = n[1];
				hit->normal[2] = n[2];
				hit->normal[3] = n[3];
				return 1;
			}
			hit->normal[0] = -n[0];
			hit->normal[1] = -n[1];
			hit->normal[2] = -n[2];
			return 1;
		}
	} else {
		if (hit->flags & BCOLL_UV) {
			tmpv[0] = (edge.b[0] - edge.a[0]) * tmp.u;
			tmpv[1] = (edge.b[1] - edge.a[1]) * tmp.u;
			tmpv[2] = (edge.b[2] - edge.a[2]) * tmp.u;
			tmp.u = bmProjectFactor(e1, tmpv);
			tmp.v = bmProjectFactor(e2, tmpv);
		}
		*hit = tmp;
	}
	if ((hit->flags & BCOLL_REASON) == 0)
		return 1;
	hit->code = 2;
	return 1;
}

/* 0x00053720  BMATHS (unplaced)
A sphere moving by delta against a collision mesh (like bmRayMesh). */
int bmSweepSphereMesh(struct BSphere *sphere, struct BCollMesh *mesh, float *delta, int *count,
	struct BCollHit *hits)
{
	struct BCollHit hit;
	struct BTriangle tri;
	struct BCollCell *cell;
	double inv[3], d;
	int lo[3], hi[3];
	int k, x, y, z, j, i, n, tri_index;

	hit.flags = hits->flags;
	n = 0;
	for (k = 0; k < 3; k++) {
		inv[k] = 1.0 / mesh->cellSize[k];
		d = (double)sphere->centre[k] - mesh->origin[k];
		if (delta[k] < 0.0f) {
			lo[k] = (int)(((d + delta[k]) - sphere->radius) * inv[k]);
			hi[k] = (int)((d + sphere->radius) * inv[k]);
		} else {
			lo[k] = (int)((d - sphere->radius) * inv[k]);
			hi[k] = (int)((d + sphere->radius + delta[k]) * inv[k]);
		}
		if (lo[k] < 0)
			lo[k] = 0;
		if (mesh->cells[k] <= hi[k])
			hi[k] = mesh->cells[k] - 1;
	}
	for (x = lo[0]; x <= hi[0]; x++) {
		for (y = lo[1]; y <= hi[1]; y++) {
			for (z = lo[2]; z <= hi[2]; z++) {
				cell = &mesh->grid[(mesh->cells[1] * x + y) * mesh->cells[2] + z];
				for (j = 0; j < cell->count; j++) {
					tri_index = (unsigned short)(cell->list[j] + cell->base);
					for (i = 0; i < n; i++) {
						if (tri_index == hits[i].code)
							break;
					}
					if (i < n)
						continue;
					memcpy(tri.v[0], mesh->verts[mesh->tris[tri_index][0]], 12);
					memcpy(tri.v[1], mesh->verts[mesh->tris[tri_index][1]], 12);
					memcpy(tri.v[2], mesh->verts[mesh->tris[tri_index][2]], 12);
					if (bmSweepSphereTriangle(sphere, &tri, delta, &hit) == 0)
						continue;
					if (n == *count) {
						if (hits[n - 1].t < hit.t)
							continue;
						n--;
					}
					i = n;
					while (i > 0 && hit.t <= hits[i - 1].t) {
						hits[i] = hits[i - 1];
						i--;
					}
					hit.code = tri_index;
					hits[i] = hit;
					n++;
				}
			}
		}
	}
	*count = n;
	return n > 0;
}

/* 0x00053af0  BMATHS (unplaced)
Sphere a moving by vec against sphere b. */
void bmSweepSphereSphereVec(struct BSphere *a, struct BSphere *b, float *vec, struct BCollHit *hit)
{
	float len, inv, dir[4];

	len = sqrtf(vec[2] * vec[2] + vec[1] * vec[1] + vec[0] * vec[0]);
	inv = 1.0f / len;
	dir[0] = inv * vec[0];
	dir[1] = inv * vec[1];
	dir[2] = inv * vec[2];
	dir[3] = 0.0f; /* not set by the original */
	bmSweepSphereSphere(a, b, dir, len, hit);
}

/* 0x00053b60  BMATHS (unplaced)
A sphere moving by delta against an edge (a capsule test). */
int bmSweepSphereEdge(struct BSphere *sphere, struct BEdge *edge, float *delta, struct BCollHit *hit)
{
	float path[8], e[3], pa[3], pb[3], diff[3], cpt[3];
	float deltaLen, edgeLen, cosA, sinA, r2, s, t, k, hitT, inv;
	double d2, tt;

	path[0] = sphere->centre[0];
	path[1] = sphere->centre[1];
	path[2] = sphere->centre[2];
	path[4] = path[0] + delta[0];
	path[3] = sphere->centre[3];
	path[5] = path[1] + delta[1];
	path[6] = path[2] + delta[2];
	path[7] = 0.0f;
	deltaLen = sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
	e[0] = edge->b[0] - edge->a[0];
	e[1] = edge->b[1] - edge->a[1];
	e[2] = edge->b[2] - edge->a[2];
	edgeLen = sqrtf(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
	if (bmClosestLinePoints(path, (float *)edge, &s, &t) == 0) {
		t = 0.0f;
		s = 0.0f;
	}
	cosA = (e[0] * delta[0] + e[1] * delta[1] + e[2] * delta[2]) / (edgeLen * deltaLen);
	if (fabsf(cosA) < 1.0f)
		sinA = sqrtf(1.0f - cosA * cosA);
	else
		sinA = 0.0f;
	r2 = sphere->radius * sphere->radius;
	pa[0] = s * delta[0] + sphere->centre[0];
	pa[1] = s * delta[1] + sphere->centre[1];
	pa[2] = s * delta[2] + sphere->centre[2];
	pb[0] = e[0] * t + edge->a[0];
	pb[1] = e[1] * t + edge->a[1];
	pb[2] = e[2] * t + edge->a[2];
	d2 = dist_sq(pb, pa);
	if ((double)r2 < d2)
		goto too_far;
	k = (float)sqrt((double)r2 - d2);
	if (sinA != 0.0f)
		k = k / sinA;
	tt = (double)t - ((double)cosA * k) / (double)edgeLen;
	t = (float)tt;
	if (tt <= 1.0) {
		if (t < 0.0f) {
			/* the sphere first reaches the edge's start */
			diff[0] = edge->a[0] - sphere->centre[0];
			t = 0.0f;
			diff[1] = edge->a[1] - sphere->centre[1];
			diff[2] = edge->a[2] - sphere->centre[2];
			s = bmProjectFactor(delta, diff);
			diff[0] = (s * delta[0] + sphere->centre[0]) - edge->a[0];
			diff[1] = (s * delta[1] + sphere->centre[1]) - edge->a[1];
			diff[2] = (s * delta[2] + sphere->centre[2]) - edge->a[2];
			goto endpoint;
		}
	} else {
		/* the sphere first reaches the edge's end */
		diff[0] = edge->b[0] - sphere->centre[0];
		t = 1.0f;
		diff[1] = edge->b[1] - sphere->centre[1];
		diff[2] = edge->b[2] - sphere->centre[2];
		s = bmProjectFactor(delta, diff);
		diff[0] = (s * delta[0] + sphere->centre[0]) - edge->b[0];
		diff[1] = (s * delta[1] + sphere->centre[1]) - edge->b[1];
		diff[2] = (s * delta[2] + sphere->centre[2]) - edge->b[2];
endpoint:
		d2 = (double)diff[0] * diff[0] + (double)diff[1] * diff[1] + (double)diff[2] * diff[2];
		if ((double)r2 < d2) {
			if (hit->flags & BCOLL_REASON)
				hit->code = 0x800;
			return 0;
		}
		k = (float)sqrt((double)r2 - d2);
	}
	hitT = s - (k / deltaLen + EPS2);
	if (EPS2 <= hitT) {
		if (1.0f < hitT)
			goto too_far;
	} else {
		if (hitT <= -EPS2) {
			if (hit->flags & BCOLL_REASON)
				hit->code = 0x200;
			return 0;
		}
		hitT = 0.0f;
	}
	cpt[0] = e[0] * t + edge->a[0];
	cpt[1] = e[1] * t + edge->a[1];
	cpt[2] = e[2] * t + edge->a[2];
	if (hit->flags & BCOLL_T)
		hit->t = hitT;
	if (hit->flags & BCOLL_POINT) {
		hit->point[0] = cpt[0];
		hit->point[1] = cpt[1];
		hit->point[2] = cpt[2];
		hit->point[3] = 0.0f; /* the original stores an uninitialised word */
	}
	if (hit->flags & BCOLL_UV) {
		hit->u = t;
		hit->v = 0.0f;
	}
	if (hit->flags & BCOLL_NORMAL) {
		inv = 1.0f / sphere->radius;
		hit->normal[0] = inv * ((hitT * delta[0] + sphere->centre[0]) - cpt[0]);
		hit->normal[1] = inv * ((hitT * delta[1] + sphere->centre[1]) - cpt[1]);
		hit->normal[2] = inv * ((hitT * delta[2] + sphere->centre[2]) - cpt[2]);
	}
	return 1;

too_far:
	if (hit->flags & BCOLL_REASON)
		hit->code = 0x100;
	return 0;
}

/* 0x00054050  BMATHS (unplaced)
A ray (origin, delta) against a triangle (Moeller-Trumbore, both sides). */
int bmRayTriangle(struct BRay *ray, struct BTriangle *tri, struct BCollHit *hit)
{
	float e1[3], e2[3], s[3], q[4], p[3], scaled[3];
	float det, u, v, inv, t;
	unsigned int flags;

	e1[0] = tri->v[1][0] - tri->v[0][0];
	e1[1] = tri->v[1][1] - tri->v[0][1];
	e1[2] = tri->v[1][2] - tri->v[0][2];
	e2[0] = tri->v[2][0] - tri->v[0][0];
	e2[1] = tri->v[2][1] - tri->v[0][1];
	e2[2] = tri->v[2][2] - tri->v[0][2];
	s[0] = ray->origin[0] - tri->v[0][0];
	s[1] = ray->origin[1] - tri->v[0][1];
	s[2] = ray->origin[2] - tri->v[0][2];
	q[0] = s[1] * e1[2] - s[2] * e1[1];
	q[1] = e1[0] * s[2] - e1[2] * s[0];
	q[2] = e1[1] * s[0] - s[1] * e1[0];
	p[0] = e2[2] * ray->dir[1] - e2[1] * ray->dir[2];
	p[1] = e2[0] * ray->dir[2] - e2[2] * ray->dir[0];
	p[2] = e2[1] * ray->dir[0] - e2[0] * ray->dir[1];
	det = p[0] * e1[0] + p[1] * e1[1] + e1[2] * p[2];
	if (det <= EPS) {
		if (!(det < -EPS && (u = p[0] * s[0] + p[1] * s[1] + p[2] * s[2]) <= 0.0f && det <= u &&
			(v = q[0] * ray->dir[0] + q[1] * ray->dir[1] + q[2] * ray->dir[2]) <= 0.0f && det <= v + u))
			goto outside;
	} else {
		u = p[0] * s[0] + p[1] * s[1] + p[2] * s[2];
		if (!(0.0f <= u && u <= det &&
			(v = q[0] * ray->dir[0] + q[1] * ray->dir[1] + q[2] * ray->dir[2]) >= 0.0f && v + u <= det))
			goto outside;
	}
	inv = 1.0f / det;
	t = (q[0] * e2[0] + q[1] * e2[1] + q[2] * e2[2]) * inv;
	if (t < 0.0f) {
		if ((hit->flags & BCOLL_REASON) == 0)
			return 0;
		hit->code = 0x200;
		return 0;
	}
	if (1.0f < t) {
		if ((hit->flags & BCOLL_REASON) == 0)
			return 0;
		hit->code = 0x100;
		return 0;
	}
	flags = hit->flags;
	if (flags & BCOLL_T)
		hit->t = t;
	if (flags & BCOLL_UV) {
		hit->u = inv * u;
		hit->v = inv * v;
	}
	if (flags & BCOLL_POINT) {
		em_bmVecScale(scaled, e1, inv * u);
		em_bmVecScale(q, e2, inv * v);
		em_bmVecAdd(hit->point, scaled, q);
		em_bmVecAdd(hit->point, hit->point, tri->v[0]);
		hit->point[3] = 1.0f;
	}
	if (hit->flags & BCOLL_NORMAL) {
		em_bmVecCross(q, e1, e2);
		em_bmVecNormalise(q, q);
		if (det < 0.0f) {
			hit->normal[0] = -q[0];
			hit->normal[1] = -q[1];
			hit->normal[2] = -q[2];
			return 1;
		}
		hit->normal[0] = q[0];
		hit->normal[1] = q[1];
		hit->normal[2] = q[2];
		hit->normal[3] = 0.0f; /* the original stores an uninitialised word */
	}
	return 1;

outside:
	if (hit->flags & BCOLL_REASON)
		hit->code = 0x800;
	return 0;
}

/* 0x00054420  BMATHS (unplaced)
The closest points of two lines {p0, p1} and {q0, q1}: *s along the first, *t along
the second. 0 when they are parallel. */
int bmClosestLinePoints(float *a, float *b, float *s, float *t)
{
	float da[3], db[3], c[3], len2, w[3];

	da[0] = a[4] - a[0];
	da[1] = a[5] - a[1];
	da[2] = a[6] - a[2];
	db[0] = b[4] - b[0];
	db[1] = b[5] - b[1];
	db[2] = b[6] - b[2];
	c[0] = da[1] * db[2] - da[2] * db[1];
	c[1] = da[2] * db[0] - da[0] * db[2];
	c[2] = db[1] * da[0] - da[1] * db[0];
	len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
	if (len2 == 0.0f)
		return 0;
	w[0] = b[0] - a[0];
	w[1] = b[1] - a[1];
	w[2] = b[2] - a[2];
	len2 = 1.0f / len2;
	*s = ((db[0] * c[1] - db[1] * c[0]) * w[2] +
		((db[1] * c[2] - db[2] * c[1]) * w[0] - (db[0] * c[2] - db[2] * c[0]) * w[1])) * len2;
	*t = ((da[0] * c[1] - da[1] * c[0]) * w[2] +
		((da[1] * c[2] - da[2] * c[1]) * w[0] - (da[0] * c[2] - da[2] * c[0]) * w[1])) * len2;
	return 1;
}

/* 0x00054590  BMATHS (unplaced)
(a . b) / (a . a): how far along a the projection of b lies. */
float bmProjectFactor(float *a, float *b)
{
	double dot, len2;

	dot = (double)b[2] * a[2] + (double)b[1] * a[1] + (double)a[0] * b[0];
	len2 = (double)a[0] * a[0] + (double)a[1] * a[1] + (double)a[2] * a[2];
	return (float)(dot / len2);
}

/* 0x000545d0  BMATHS (unplaced)
2D: out = in / s. */
void bmVec2Divide(float *out, float *in, float s)
{
	s = 1.0f / s;
	out[0] = s * in[0];
	out[1] = s * in[1];
}
