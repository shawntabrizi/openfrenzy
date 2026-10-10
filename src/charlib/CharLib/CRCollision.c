/*
CharLib/CRCollision.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\CRCollision.cpp (tags at 0x000b3d50 and
0x000b3dc0, allocations at source lines 2450 and 2474).

Collision of the CharLib actors against triangle meshes: the collision mesh of an actor
(built from its "_coll.obe" mesh or its own geometry), point-in-triangle tests, the
contact list of one collision step and its resolution into push vectors.

The functions from 0x000b1bb0 to 0x000b3840 lie between the AI player code and the two
CRCollision.cpp tags, whose line numbers are near the end of a long file: they are placed
here (inferred).
*/

#include "charlib.h"
#include "charlib_actor.h"
#include "charlib_coll.h"

/* 0x002ba99c  frame counter (core) */
extern int DAT_002ba99c;
/* the character table (classlib_ui, 0x60 bytes per character; +0 the name) and the
player table (core, 0x28 bytes per player; +0 the character) */
extern unsigned char DAT_002906f8[6 * 0x60];
extern int DAT_002baed4[];

/* bkernel 0x00051540: squared distance of two points. (bkernel's port declares it as an
empty bmNothing2(void); the original returns |a - b|^2 in ST0. Computed here; reported.) */
static float coll_dist_sq(const float *a, const float *b)
{
	float dx = a[0] - b[0];
	float dy = a[1] - b[1];
	float dz = a[2] - b[2];

	return dz * dz + dy * dy + dx * dx;
}

/* the two axes to project a triangle on: the axis of the largest normal component is
dropped (0x000b22c0 and 0x000b2b50 pick them the same way) */
static void coll_tri_axes(const float *tri, int *u, int *v)
{
	float nx = CHL_F32(tri, TRI_NORMAL + 0x0);
	float ny = CHL_F32(tri, TRI_NORMAL + 0x4);
	float nz = CHL_F32(tri, TRI_NORMAL + 0x8);

	if (nx <= 0.0f)
		nx = -nx;
	if (ny <= 0.0f)
		ny = -ny;
	if (nz <= 0.0f)
		nz = -nz;
	if (ny <= nx) {
		*u = 1;
		*v = nz <= nx ? 2 : 0;
	} else {
		*u = 0;
		*v = nz <= ny ? 2 : 1;
	}
}

/* the edge test of 0x000b22c0 / 0x000b2b50: p is on the inner side of every edge in the
projection on axes u, v */
static int coll_point_in_tri_2d(const float *tri, const float *p, int u, int v)
{
	float e1[3];
	float e2[3];
	float d[3];
	int a = 1;
	int b = 2;
	int k;

	for (k = 0; k < 3; k++) {
		const float *base = tri + k * 4;
		const float *va = tri + a * 4;
		const float *vb = tri + b * 4;

		e1[0] = va[0] - base[0];
		e1[1] = va[1] - base[1];
		e1[2] = va[2] - base[2];
		e2[0] = vb[0] - base[0];
		e2[1] = vb[1] - base[1];
		e2[2] = vb[2] - base[2];
		d[0] = p[0] - base[0];
		d[1] = p[1] - base[1];
		d[2] = p[2] - base[2];
		if ((e2[u] * e1[v] - e2[v] * e1[u]) * (d[u] * e1[v] - d[v] * e1[u]) < 0.0f)
			return 0;
		a++;
		b++;
		if (2 < b)
			b = 0;
		else if (2 < a)
			a = 0;
	}
	return 1;
}

/* 0x000b1bb0  CharLib (CRCollision.cpp, inferred)
1 when p lies over the triangle in the x/z plane (0.01 tolerance); 0 for vertical
triangles. */
int CRColl_PointOverTriXZ(float *tri, float *p)
{
	float e1[3];
	float e2[3];
	float d[3];
	double c_edge;
	double c_point;
	int a = 1;
	int b = 2;
	int k;

	if (CHL_F32(tri, TRI_NORMAL + 0x4) == 0.0f)
		return 0;
	for (k = 0; k < 3; k++) {
		float *base = tri + k * 4;

		FUN_00070580(e1, tri + a * 4, base);
		FUN_00070580(e2, tri + b * 4, base);
		FUN_00070580(d, p, base);
		c_edge = (double)(e2[0] * e1[2] - e2[2] * e1[0]);
		c_point = (double)(d[0] * e1[2] - d[2] * e1[0]);
		if ((0.01 < c_edge && c_point < -0.01) || (c_edge < -0.01 && 0.01 < c_point))
			return 0;
		b = (b + 1) % 3;
		a = (a + 1) % 3;
	}
	return 1;
}

/* 0x000b1cd0  CharLib (CRCollision.cpp, inferred)
Scales the x/z part of v up to at least `length` (never down). */
void CLVec3_MinLengthXZ(float *v, float length)
{
	float len2 = v[2] * v[2] + v[0] * v[0];

	if (len2 != 0.0f) {
		length = length / sqrtf(len2);
		if (1.0f <= length) {
			v[0] = length * v[0];
			v[2] = length * v[2];
		}
	}
}

/* 0x000b1e00  CharLib (CRCollision.cpp, inferred)
Pushes actor b out of actor a where their collision spheres (character data 0x7c, 0x2c
bytes each, count 0x15d) overlap. Spheres with a height (+0x24) are vertical capsules: a
pair that overlaps only over the top is ignored. b with a mass (block 0x188) of 1e6 or
more does not move. Returns 1 when b was moved. */
int CRColl_PushApart(void *a, void *b)
{
	float pa[5][4];
	float pb[5][4];
	unsigned char *cda = ACT_CD(a);
	unsigned char *cdb;
	unsigned int na;
	unsigned int nb;
	unsigned int i;
	unsigned int j;
	int moved = 0;
	float dx;
	float dy;
	float dz;
	float dxz2;
	float d2;
	float reach;
	float h;
	float d;
	float nx;
	float ny;
	float nz;
	float limit;
	float push;

	if (CHL_PTR(b, ACT_BLOCK188) != NULL && 1e6f <= *(float *)CHL_PTR(b, ACT_BLOCK188))
		return 0;
	na = CHL_U8(cda, CD_SPHERE_COUNT);
	for (i = 0; i < na; i++) {
		pa[i][0] = CHL_F32(cda, CD_SPHERES + i * 0x2c + 0x0) + CHL_F32(a, ACT_POS + 0x0);
		pa[i][1] = CHL_F32(cda, CD_SPHERES + i * 0x2c + 0x4) + CHL_F32(a, ACT_POS + 0x4);
		pa[i][2] = CHL_F32(a, ACT_POS + 0x8) + CHL_F32(cda, CD_SPHERES + i * 0x2c + 0x8);
	}
	cdb = ACT_CD(b);
	nb = CHL_U8(cdb, CD_SPHERE_COUNT);
	for (j = 0; j < nb; j++) {
		pb[j][0] = CHL_F32(cdb, CD_SPHERES + j * 0x2c + 0x0) + CHL_F32(b, ACT_POS + 0x0);
		pb[j][1] = CHL_F32(cdb, CD_SPHERES + j * 0x2c + 0x4) + CHL_F32(b, ACT_POS + 0x4);
		pb[j][2] = CHL_F32(b, ACT_POS + 0x8) + CHL_F32(cdb, CD_SPHERES + j * 0x2c + 0x8);
	}
	for (i = 0; i < na; i++) {
		cdb = ACT_CD(b);
		for (j = 0; j < (unsigned int)CHL_U8(cdb, CD_SPHERE_COUNT); j++) {
			float rb = CHL_F32(cdb, CD_SPHERES + j * 0x2c + 0x20);
			float hb = CHL_F32(cdb, CD_SPHERES + j * 0x2c + 0x24);

			cda = ACT_CD(a);
			dx = pb[j][0] - pa[i][0];
			dy = pb[j][1] - pa[i][1];
			dz = pb[j][2] - pa[i][2];
			dxz2 = dx * dx + dz * dz;
			d2 = dy * dy + dxz2;
			reach = rb + CHL_F32(cda, CD_SPHERES + i * 0x2c + 0x20);
			if (!(d2 <= reach * reach))
				goto next;
			h = 0.0f;
			if (0.0f < CHL_F32(cda, CD_SPHERES + i * 0x2c + 0x24))
				h = CHL_F32(cda, CD_SPHERES + i * 0x2c + 0x24);
			if (hb <= 0.0f) {
				if (0.0f < h) {
					h = h + rb;
					if (0.0f < h && h * h < dxz2)
						goto next;
				}
			} else {
				if (h == 0.0f)
					h = CHL_F32(cda, CD_SPHERES + i * 0x2c + 0x20);
				h = h + hb;
				if (0.0f < h && h * h < dxz2)
					goto next;
			}
			moved = 1;
			d = sqrtf(d2);
			if (d == 0.0f) {
				nx = 0.0f;
				ny = 1.0f;
				nz = 0.0f;
			} else {
				nz = 1.0f / d;
				nx = dx * nz;
				ny = dy * nz;
				nz = nz * dz;
			}
			limit = 9999999.0f;
			if (0.0f < h && (nx != 0.0f || nz != 0.0f))
				limit = h / (nx * nx + nz * nz);
			push = CHL_F32(cda, CD_SPHERES + j * 0x2c + 0x20) +
				CHL_F32(ACT_CD(b), CD_SPHERES + i * 0x2c + 0x20);
			if (limit < push)
				push = limit;
			push = push - d;
			CHL_F32(b, ACT_POS + 0x0) = nx * push + CHL_F32(b, ACT_POS + 0x0);
			CHL_F32(b, ACT_POS + 0x4) = push * ny + CHL_F32(b, ACT_POS + 0x4);
			CHL_F32(b, ACT_POS + 0x8) = push * nz + CHL_F32(b, ACT_POS + 0x8);
next:
			cdb = ACT_CD(b);
		}
	}
	return moved;
}

/* 0x000b21b0  CharLib (CRCollision.cpp, inferred)
1 when triangle b lies in the plane of triangle a (each vertex within 0.1 of it) and
faces the same way (normals' dot >= 0.95). */
int CRColl_SamePlane(float *a, float *b)
{
	float d0 = (b[0] - a[0]) * a[12] + (b[1] - a[1]) * a[13] + (b[2] - a[2]) * a[14];
	float d1 = (b[4] - a[0]) * a[12] + (b[5] - a[1]) * a[13] + (b[6] - a[2]) * a[14];
	float d2 = (b[8] - a[0]) * a[12] + (b[9] - a[1]) * a[13] + (b[10] - a[2]) * a[14];

	if (d0 <= 0.0f)
		d0 = -d0;
	if (!((double)d0 <= 0.1))
		return 0;
	if (d1 <= 0.0f)
		d1 = -d1;
	if (!((double)d1 <= 0.1))
		return 0;
	if (d2 <= 0.0f)
		d2 = -d2;
	if ((double)d2 <= 0.1 && 0.95 <= (double)(a[12] * b[12] + a[13] * b[13] + a[14] * b[14]))
		return 1;
	return 0;
}

/* 0x000b22c0  CharLib (CRCollision.cpp, inferred)
Projects p onto the triangle's plane (into out) and returns 1 when the projection lies
inside the triangle. */
int CRColl_ProjectInTri(float *tri, float *p, float *out)
{
	int u;
	int v;

	FUN_00050ff0(out, p, 1.0f, tri + 12,
		-((p[0] - tri[0]) * tri[12] + (p[1] - tri[1]) * tri[13] + (p[2] - tri[2]) * tri[14]));
	coll_tri_axes(tri, &u, &v);
	return coll_point_in_tri_2d(tri, out, u, v);
}

/* 0x000b24e0  CharLib (CRCollision.cpp, inferred)
The point of the triangle's edges (then vertices) nearest to p within `radius`, into out
(w = 1). Returns its distance, or 0 when nothing is within the radius. (When only a
vertex is near, the distance returned is that of the last vertex tested, as in the
original.) */
float CRColl_NearestEdgePoint(float *p, float *tri, float radius, float *out)
{
	float edge_points[3][4];
	float best = radius * radius;
	float best_vertex = 99999.0f;
	float t;
	float d2 = 0.0f;
	int best_edge = -1;
	int nearest = -1;
	int i;

	for (i = 0; i < 3; i++) {
		float *dir = CHL_FIELD(float, tri, TRI_EDGE_DIRS + i * 0x10);

		t = (dir[1] * p[1] + p[0] * dir[0] + dir[2] * p[2]) - CHL_F32(tri, TRI_EDGE_STARTS + i * 4);
		if (0.0f <= t && t <= CHL_F32(tri, TRI_EDGE_LENGTHS + i * 4)) {
			FUN_00050ff0(edge_points[i], tri + i * 4, 1.0f, dir, t);
			t = (p[0] - edge_points[i][0]) * (p[0] - edge_points[i][0]) +
				(p[1] - edge_points[i][1]) * (p[1] - edge_points[i][1]) +
				(p[2] - edge_points[i][2]) * (p[2] - edge_points[i][2]);
			if (t < best) {
				best_edge = i;
				best = t;
			}
		}
	}
	if (best_edge < 0) {
		for (i = 0; i < 3; i++) {
			float *vtx = tri + i * 4;

			d2 = (p[0] - vtx[0]) * (p[0] - vtx[0]) + (p[1] - vtx[1]) * (p[1] - vtx[1]) +
				(p[2] - vtx[2]) * (p[2] - vtx[2]);
			if (d2 < best_vertex) {
				best_vertex = d2;
				nearest = i;
			}
		}
		if (-1 < nearest) {
			out[0] = tri[nearest * 4 + 0];
			out[1] = tri[nearest * 4 + 1];
			out[2] = tri[nearest * 4 + 2];
			out[3] = 1.0f;
			if (best_vertex < best)
				return sqrtf(d2);
		}
		return 0.0f;
	}
	out[0] = edge_points[best_edge][0];
	out[1] = edge_points[best_edge][1];
	out[2] = edge_points[best_edge][2];
	out[3] = 1.0f;
	return sqrtf(best);
}

/* 0x000b26e0  CharLib (CRCollision.cpp, inferred)
A platform moved: shifts its collision bounding box by the movement and marks it
(0x3d4) so that its triangles are moved before the next test (CRColl_MovePlatformMesh). */
void CRColl_MovePlatformBox(void *platform)
{
	float dx = CHL_F32(platform, 0xe0) - CHL_F32(platform, 0x1b4);
	float dy = CHL_F32(platform, 0xe4) - CHL_F32(platform, 0x1b8);
	float dz = CHL_F32(platform, 0xe8) - CHL_F32(platform, 0x1bc);
	void *mesh = CHL_PTR(platform, 0x1a0);

	CHL_F32(mesh, MESH_MIN + 0x0) = dx + CHL_F32(mesh, MESH_MIN + 0x0);
	CHL_F32(mesh, MESH_MIN + 0x4) = dy + CHL_F32(mesh, MESH_MIN + 0x4);
	CHL_F32(mesh, MESH_MIN + 0x8) = dz + CHL_F32(mesh, MESH_MIN + 0x8);
	CHL_F32(mesh, MESH_MAX + 0x0) = dx + CHL_F32(mesh, MESH_MAX + 0x0);
	CHL_F32(mesh, MESH_MAX + 0x4) = dy + CHL_F32(mesh, MESH_MAX + 0x4);
	CHL_F32(mesh, MESH_MAX + 0x8) = dz + CHL_F32(mesh, MESH_MAX + 0x8);
	CHL_U8(platform, 0x3d4) = 1;
}

/* 0x000b2770  CharLib (CRCollision.cpp, inferred)
Computes the bounding boxes of the triangles of an actor's collision mesh and of the
mesh; with edges != 0 also the edge directions (lengths too with lengths != 0), and
always the edge start projections and the plane distance. */
void CRColl_PrepareMesh(void *actor, int lengths, int edges)
{
	void *mesh = CHL_PTR(actor, ACT_COLLMESH);
	float *tri = *(float **)mesh;
	float inv;
	int i;

	memcpy(CHL_FIELD(char, mesh, MESH_MIN), tri, 0x10);
	mesh = CHL_PTR(actor, ACT_COLLMESH);
	memcpy(CHL_FIELD(char, mesh, MESH_MAX), CHL_FIELD(char, mesh, MESH_MIN), 0x10);
	for (i = 0; i < CHL_I32(CHL_PTR(actor, ACT_COLLMESH), MESH_COUNT); i++, tri += TRI_FLOATS) {
		float *box = tri + 16; /* min x, max x, min y, max y, min z, max z */

		box[1] = tri[0];
		box[0] = tri[0];
		box[2] = tri[1];
		box[3] = tri[1];
		box[4] = tri[2];
		box[5] = tri[2];
		if (tri[4] < box[0])
			box[0] = tri[4];
		if (tri[0] < tri[4])
			box[1] = tri[4];
		if (tri[5] < box[2])
			box[2] = tri[5];
		if (tri[1] < tri[5])
			box[3] = tri[5];
		if (tri[6] < box[4])
			box[4] = tri[6];
		if (tri[2] < tri[6])
			box[5] = tri[6];
		if (tri[8] < box[0])
			box[0] = tri[8];
		if (box[1] < tri[8])
			box[1] = tri[8];
		if (tri[9] < box[2])
			box[2] = tri[9];
		if (box[3] < tri[9])
			box[3] = tri[9];
		if (tri[10] < box[4])
			box[4] = tri[10];
		if (box[5] < tri[10])
			box[5] = tri[10];
		mesh = CHL_PTR(actor, ACT_COLLMESH);
		if (box[0] < CHL_F32(mesh, MESH_MIN + 0x0))
			CHL_F32(mesh, MESH_MIN + 0x0) = box[0];
		if (CHL_F32(mesh, MESH_MAX + 0x0) < box[1])
			CHL_F32(mesh, MESH_MAX + 0x0) = box[1];
		if (box[2] < CHL_F32(mesh, MESH_MIN + 0x4))
			CHL_F32(mesh, MESH_MIN + 0x4) = box[2];
		if (CHL_F32(mesh, MESH_MAX + 0x4) < box[3])
			CHL_F32(mesh, MESH_MAX + 0x4) = box[3];
		if (box[4] < CHL_F32(mesh, MESH_MIN + 0x8))
			CHL_F32(mesh, MESH_MIN + 0x8) = box[4];
		if (CHL_F32(mesh, MESH_MAX + 0x8) < box[5])
			CHL_F32(mesh, MESH_MAX + 0x8) = box[5];
		if (edges != 0) {
			tri[22] = tri[4] - tri[0];
			tri[23] = tri[5] - tri[1];
			tri[24] = tri[6] - tri[2];
			tri[26] = tri[8] - tri[4];
			tri[27] = tri[9] - tri[5];
			tri[28] = tri[10] - tri[6];
			tri[30] = tri[0] - tri[8];
			tri[31] = tri[1] - tri[9];
			tri[32] = tri[2] - tri[10];
			if (lengths != 0) {
				tri[34] = sqrtf(tri[24] * tri[24] + tri[23] * tri[23] + tri[22] * tri[22]);
				tri[35] = sqrtf(tri[28] * tri[28] + tri[27] * tri[27] + tri[26] * tri[26]);
				tri[36] = sqrtf(tri[32] * tri[32] + tri[31] * tri[31] + tri[30] * tri[30]);
			}
			inv = 1.0f / tri[34];
			tri[22] = inv * tri[22];
			tri[23] = inv * tri[23];
			tri[24] = inv * tri[24];
			inv = 1.0f / tri[35];
			tri[26] = inv * tri[26];
			tri[27] = inv * tri[27];
			tri[28] = inv * tri[28];
			inv = 1.0f / tri[36];
			tri[30] = inv * tri[30];
			tri[31] = inv * tri[31];
			tri[32] = inv * tri[32];
		}
		tri[37] = tri[24] * tri[2] + tri[22] * tri[0] + tri[23] * tri[1];
		tri[38] = tri[26] * tri[4] + tri[27] * tri[5] + tri[28] * tri[6];
		tri[39] = tri[30] * tri[8] + tri[31] * tri[9] + tri[32] * tri[10];
		tri[40] = tri[14] * tri[2] + tri[12] * tri[0] + tri[13] * tri[1];
	}
}

/* 0x000b2b50  CharLib (CRCollision.cpp, inferred)
1 when p (already in the triangle's plane) lies inside the triangle. */
int CRColl_PointInTri(float *tri, float *p)
{
	int u;
	int v;

	coll_tri_axes(tri, &u, &v);
	return coll_point_in_tri_2d(tri, p, u, v);
}

/* 0x000b2d40  CharLib (CRCollision.cpp, inferred)
For a capsule sphere (radius +0x20, half height +0x24) at `centre` touching the edge of a
contact's triangle: tests the point of the capsule's rim nearest to the contact against
the triangle (projecting it into the contact position). Returns 0 for a vertical
triangle or when the projection falls outside the rim. */
int CRColl_CapsuleRimTest(float *sphere, float *centre, struct CRContact *contact)
{
	float d[4];
	float a[4];
	float b[4];
	float p[4];
	float e;
	int inside;

	if (CHL_F32(contact->tri, TRI_NORMAL + 0x4) == 0.0f)
		return 0;
	d[0] = centre[0] - contact->pos[0];
	d[1] = centre[1] - contact->pos[1];
	d[2] = centre[2] - contact->pos[2];
	a[0] = d[2] - d[1] * 0.0f;
	a[1] = d[0] * 0.0f - d[2] * 0.0f;
	a[2] = d[1] * 0.0f - d[0];
	b[0] = d[2] * a[1] - a[2] * d[1];
	b[1] = d[0] * a[2] - d[2] * a[0];
	b[2] = a[0] * d[1] - d[0] * a[1];
	FUN_000514a0(a, b); /* the result is overwritten below, as in the original */
	a[0] = centre[0];
	a[2] = centre[2];
	a[3] = centre[3];
	a[1] = centre[1] - sqrtf(sphere[8] * sphere[8] - sphere[9] * sphere[9]);
	d[0] = contact->pos[0] - a[0];
	d[2] = contact->pos[2] - a[2];
	d[1] = 0.0f;
	FUN_000514a0(b, d);
	e = sphere[9];
	b[0] = b[0] * e;
	b[1] = b[1] * e;
	b[2] = b[2] * e;
	p[0] = a[0] + b[0];
	p[1] = b[1] + a[1];
	p[2] = a[2] + b[2];
	inside = CRColl_ProjectInTri(contact->tri, p, contact->pos);
	if (inside != 0 &&
	    e * e < (contact->pos[0] - b[0]) * (contact->pos[0] - b[0]) +
		    (contact->pos[2] - b[2]) * (contact->pos[2] - b[2]))
		return 0;
	return inside;
}

/* 0x000b2f20  CharLib (CRCollision.cpp, inferred)
Resolves the contacts of one collision step (newest first) for an actor: records up to 5
touching planes in its character data, bounces it when its block (0x188) asks for it,
finds the floor it stands on (the platform too) and builds the push vectors of the walls
and edges (coll_pushes). */
void CRColl_ResolveContacts(void *actor)
{
	struct CRContact *c;
	unsigned char *cd;
	float *vel;
	float speed;
	float len;
	float h;
	float dx;
	float dz;
	float d2;
	float d;
	float inv;
	float over;
	int planes = 0;
	int index;
	int k;
	unsigned int s;

	for (index = coll_contact_count - 1; -1 < index; index--) {
		c = &coll_contacts[index];
		if (c->ignore == 1)
			continue;
		cd = ACT_CD(actor);
		s = c->sphere;
		if (0.0f < CHL_F32(cd, CD_SPHERES + s * 0x2c + 0x24)) {
			h = CHL_F32(cd, CD_SPHERES + s * 0x2c + 0x24);
			if (h * h < FUN_00070db0(c->pos, coll_sphere_centres[s]) &&
			    CRColl_CapsuleRimTest(CHL_FIELD(float, cd, CD_SPHERES + s * 0x2c), coll_sphere_centres[s], c) == 0)
				continue;
		}
		CHL_PTR(ACT_CD(actor), planes * 4) = c->tri;
		if (CHL_F32(CHL_PTR(actor, ACT_BLOCK188), 0x24) < 0.0f) {
			/* bounce: slide along the plane, then reverse x/z with the bounce factor */
			vel = CHL_FIELD(float, actor, ACT_VEL);
			speed = sqrtf(vel[2] * vel[2] + vel[1] * vel[1] + vel[0] * vel[0]);
			FUN_00050ff0(vel, vel, 1.0f, CHL_FIELD(float, c->tri, TRI_NORMAL),
				-(vel[0] * CHL_F32(c->tri, TRI_NORMAL + 0x0) + vel[1] * CHL_F32(c->tri, TRI_NORMAL + 0x4) +
				  vel[2] * CHL_F32(c->tri, TRI_NORMAL + 0x8)));
			len = sqrtf(vel[2] * vel[2] + vel[1] * vel[1] + vel[0] * vel[0]);
			if (len != 0.0f) {
				len = 1.0f / len;
				vel[0] = -(speed * CHL_F32(CHL_PTR(actor, ACT_BLOCK188), 0x24) * len * vel[0]);
				vel[2] = -(speed * CHL_F32(CHL_PTR(actor, ACT_BLOCK188), 0x24) * len * vel[2]);
			}
			CHL_F32(CHL_PTR(actor, ACT_BLOCK188), 0x24) = CHL_F32(CHL_PTR(actor, ACT_BLOCK188), 0x28) *
				CHL_F32(CHL_PTR(actor, ACT_BLOCK188), 0x24);
		}
		memcpy(CHL_FIELD(char, ACT_CD(actor), CD_CONTACT_POINTS + planes * 0x10), c->pos, 0x10);
		CHL_U8(ACT_CD(actor), CD_CONTACT_EDGE + planes) = (unsigned char)c->edge;
		k = coll_push_count;
		planes++;
		if (4 < planes)
			break;
		if (c->edge == 0) {
			if (c->floor == 1) {
				float *n = CHL_FIELD(float, c->tri, TRI_NORMAL);

				h = coll_sphere_radii[s] - c->dist / ((n[2] + n[0]) * 0.0f + n[1]);
				if (h <= coll_floor_height) {
					coll_on_ground = 1;
				} else {
					CHL_PTR(ACT_CD(actor), CD_PLATFORM) = c->platform;
					coll_floor_contact = (short)index;
					coll_floor_height = h;
					CHL_I16(ACT_CD(actor), CD_FLOOR_PLANE) = (short)(planes - 1);
					coll_on_ground = 1;
				}
				continue;
			}
			{
				float *n = CHL_FIELD(float, c->tri, TRI_NORMAL);

				len = coll_sphere_radii[s] - c->dist;
				coll_push_contact[k] = (short)index;
				coll_pushes[k][3] = 1.0f;
				coll_push_length[k] = len;
				coll_pushes[k][2] = len * n[2];
				coll_pushes[k][0] = n[0] * len;
				coll_pushes[k][1] = len * n[1];
				if (0.7f < n[1])
					coll_on_ground = 1;
				if (n[1] < 0.1f && -0.1f < n[1])
					coll_steep = 1;
			}
		} else {
			dx = coll_sphere_centres[s][0] - c->pos[0];
			h = coll_sphere_centres[s][1] - c->pos[1];
			dz = coll_sphere_centres[s][2] - c->pos[2];
			coll_push_contact[k] = (short)index;
			coll_pushes[k][3] = 0.0f;
			d = sqrtf(dx * dx + h * h + dz * dz);
			inv = 1.0f / d;
			d = coll_sphere_radii[s] - d;
			coll_push_length[k] = d;
			coll_pushes[k][2] = d * inv * dz;
			coll_pushes[k][0] = d * inv * dx;
			coll_pushes[k][1] = d * inv * h;
		}
		if (0.0f < coll_push_length[k]) {
			k++;
			coll_push_count = k;
		}
		/* capsules: keep the push inside the capsule's radius in x/z */
		cd = ACT_CD(actor);
		if (0.0f < CHL_F32(cd, CD_SPHERES + s * 0x2c + 0x24)) {
			float *push = coll_pushes[k - 1];

			dx = (c->pos[0] - push[0]) - coll_sphere_centres[s][0];
			dz = (c->pos[2] - push[2]) - coll_sphere_centres[s][2];
			d2 = dx * dx + dz * dz;
			h = CHL_F32(cd, CD_SPHERES + s * 0x2c + 0x24);
			if (h * h < d2) {
				d = sqrtf(d2);
				inv = 1.0f / d;
				over = d - CHL_F32(cd, CD_SPHERES + s * 0x2c + 0x24);
				push[0] = over * dx * inv + push[0];
				push[1] = over * 0.0f + push[1];
				push[2] = over * inv * dz + push[2];
				len = coll_push_length[k - 1] - (d - CHL_F32(ACT_CD(actor), CD_SPHERES + s * 0x2c + 0x24));
				coll_push_length[k - 1] = len;
				if (len == 0.0f)
					coll_push_count = k - 1;
			}
		}
	}
	CHL_I16(ACT_CD(actor), CD_PLANE_COUNT) = (short)planes;
	if ((CHL_U32(actor, ACT_FLAGS) & 0x4000) != 0 && 0 < planes)
		CHL_U32(actor, ACT_FLAGS) = CHL_U32(actor, ACT_FLAGS) & 0xffffbfff;
}

/* 0x000b34e0  CharLib (CRCollision.cpp, inferred)
Merges edge contacts at the same point (within 0.01): of a cluster only the first stays;
edge contacts behind a face contact's plane are ignored. Returns the cluster contact on
the highest floor (normal y > 0.7), or -1. */
int CRColl_MergeEdgeContacts(void)
{
	short cluster[200];
	struct CRContact *ci;
	struct CRContact *cj;
	int best = -1;
	int n;
	int i;
	int j;
	int m;

	if (1 < coll_contact_count) {
		for (i = coll_contact_count - 1; 0 < i; i--) {
			ci = &coll_contacts[i];
			if (ci->edge == 0 || ci->ignore != 0)
				continue;
			n = 1;
			cluster[0] = (short)i;
			for (j = coll_contact_count - 1; -1 < j; j--) {
				cj = &coll_contacts[j];
				if (cj->edge != 1 && cj->ignore == 0 &&
				    (ci->pos[0] - CHL_F32(cj->tri, 0x0)) * CHL_F32(cj->tri, TRI_NORMAL + 0x0) +
				    (ci->pos[1] - CHL_F32(cj->tri, 0x4)) * CHL_F32(cj->tri, TRI_NORMAL + 0x4) +
				    (ci->pos[2] - CHL_F32(cj->tri, 0x8)) * CHL_F32(cj->tri, TRI_NORMAL + 0x8) < 0.0f)
					ci->ignore = 1;
			}
			if (ci->ignore != 0)
				continue;
			j = coll_contact_count - 1;
			if (-1 < j) {
				for (; -1 < j; j--) {
					cj = &coll_contacts[j];
					if (cj->edge != 0 && i != j && cj->ignore == 0 && (ci->floor != 1 || cj->floor != 1) &&
					    (double)coll_dist_sq(ci->pos, cj->pos) < 1e-4)
						cluster[n++] = (short)j;
				}
				if (n != 1) {
					coll_contacts[cluster[0]].ignore = 0;
					for (m = 1; m < n; m++) {
						int other = cluster[m];

						coll_contacts[other].ignore = 1;
						if (0.7f < CHL_F32(coll_contacts[other].tri, TRI_NORMAL + 0x4) &&
						    (best < 0 || coll_contacts[best].pos[1] < coll_contacts[other].pos[1]))
							best = other;
					}
					continue;
				}
			}
			ci->ignore = 1;
		}
	}
	if (coll_contacts[0].edge == 1)
		coll_contacts[0].ignore = 1;
	return best;
}

/* 0x000b3840  CharLib (CRCollision.cpp, inferred)
Adds a contact of sphere `sphere` (centre `point`, radius) of an actor with a triangle
when the sphere reaches the triangle's plane from the front: inside the triangle (a face
contact) or near an edge or vertex. A contact on the same plane as an earlier one
replaces it or is dropped. A platform with a seesaw axis (0x444) is turned by the
actor's weight. More than 199 contacts are refused with a log message. */
void CRColl_AddContact(float *tri, void *actor, float *point, float radius, void *platform, int sphere,
	int tag)
{
	struct CRContact *c = &coll_contacts[coll_contact_count];
	struct CRContact *other;
	float local[4];
	float dist;
	float d1;
	float d2;
	float sign;
	unsigned int dist2_bits;
	unsigned int sign_bits;
	unsigned char *pivot;
	int held = 0;
	int i;
	int n;

	c->sphere = (short)sphere;
	dist = (point[0] - tri[0]) * tri[12] + (point[2] - tri[2]) * tri[14] + (point[1] - tri[1]) * tri[13];
	c->dist = dist;
	if (radius < dist || dist < 0.0f)
		return;
	c = &coll_contacts[coll_contact_count];
	c->edge = CRColl_ProjectInTri(tri, point, c->pos) == 0;
	if (c->edge != 0) {
		c = &coll_contacts[coll_contact_count];
		c->edge_dist = CRColl_NearestEdgePoint(point, tri, radius, c->pos);
		if (c->edge_dist == 0.0f)
			return;
	}
	n = coll_contact_count;
	if (0xc6 < n) {
		chl_Printf("%d %s collided with too many collision planes!! %f %f\n", DAT_002ba99c,
			*(const char **)(DAT_002906f8 + DAT_002baed4[CHL_U16(actor, ACT_PLAYER) * 10] * 0x60),
			(double)CHL_F32(ACT_CD(actor), 0xa0), (double)CHL_F32(ACT_CD(actor), 0x9c));
		return;
	}
	c = &coll_contacts[n];
	if (CHL_F32(tri, TRI_NORMAL + 0x4) <= 0.7f) {
		c->floor = 0;
	} else {
		c->floor = 1;
		c->edge = CRColl_PointOverTriXZ(tri, point) == 0;
		n = coll_contact_count;
	}
	/* the loop counter shares its stack slot with `radius` in the original */
	memcpy(&dist2_bits, &radius, 4);
	if ((c->floor != 1 || c->edge != 0) && n != 0) {
		for (i = 0; i < coll_contact_count; i++) {
			dist2_bits = (unsigned int)i;
			other = &coll_contacts[i];
			if (CRColl_SamePlane(tri, other->tri) == 0)
				continue;
			if (other->edge == 0)
				return;
			if (CRColl_ProjectInTri(tri, point, local) != 0) {
				c = &coll_contacts[coll_contact_count];
				other->ignore = 1;
				c->edge = 0;
				c->tri = tri;
				memcpy(c->pos, local, 0x10);
				c->sphere = (short)sphere;
			} else {
				c = &coll_contacts[coll_contact_count];
				if (!(c->edge_dist < other->edge_dist))
					return;
				other->ignore = 1;
				c->tri = tri;
				FUN_000127e0(c->pos, c->pos);
				c->sphere = (short)sphere;
			}
		}
		dist2_bits = (unsigned int)i;
		n = coll_contact_count;
	}
	c = &coll_contacts[n];
	c->tri = tri;
	if (platform != NULL && c->edge == 0 && (pivot = CHL_PTR(platform, 0x444)) != NULL &&
	    CHL_F32(pivot, 0x18) != 0.0f && CHL_PTR(actor, ACT_BLOCK188) != NULL &&
	    *(float *)CHL_PTR(actor, ACT_BLOCK188) != 0.0f) {
		/* the actor's weight turns a seesaw platform about its axis (pivot[0]) */
		float *vel = CHL_FIELD(float, actor, ACT_VEL);
		float *spin;

		sign_bits = (unsigned int)sphere;
		memcpy(&sign, &sign_bits, 4);
		switch (pivot[0]) {
		case 0:
			sign = -1.0f;
			d1 = c->pos[1] - CHL_F32(platform, 0xe4);
			d2 = c->pos[2] - CHL_F32(platform, 0xe8);
			dist = d2 * d2 + d1 * d1;
			memcpy(&dist2_bits, &dist, 4);
			if (!(d1 * vel[2] - d2 * vel[1] > 0.0f))
				sign = 1.0f;
			break;
		case 1:
			d1 = c->pos[0] - CHL_F32(platform, 0xe0);
			d2 = c->pos[2] - CHL_F32(platform, 0xe8);
			dist = d1 * d1 + d2 * d2;
			memcpy(&dist2_bits, &dist, 4);
			sign = (d1 * vel[2] - d2 * vel[0] > 0.0f) ? 1.0f : -1.0f;
			break;
		case 2:
			d1 = c->pos[1] - CHL_F32(platform, 0xe4);
			d2 = c->pos[0] - CHL_F32(platform, 0xe0);
			dist = d2 * d2 + d1 * d1;
			memcpy(&dist2_bits, &dist, 4);
			sign = (d1 * vel[0] - d2 * vel[1] > 0.0f) ? 1.0f : -1.0f;
			break;
		default:
			break;
		}
		memcpy(&dist, &dist2_bits, 4);
		spin = CHL_FIELD(float, platform, 0x1d4 + pivot[0] * 4);
		*spin = (sqrtf(dist) * FUN_00036b00(vel, tri + 12) * sign * *(float *)CHL_PTR(actor, ACT_BLOCK188) *
			coll_seesaw_factor) / CHL_F32(CHL_PTR(platform, 0x444), 0x18) + *spin;
	}
	c->platform = NULL;
	if (platform != NULL) {
		unsigned char *cd;
		int count;

		c->platform = platform;
		cd = ACT_CD(actor);
		count = CHL_I32(cd, CD_TOUCHED_COUNT);
		if (count < 5) {
			for (i = 0; i < count; i++)
				if (CHL_PTR(cd, CD_TOUCHED + i * 4) == platform)
					break;
			if (i == count) {
				CHL_PTR(cd, CD_TOUCHED + i * 4) = platform;
				CHL_I32(ACT_CD(actor), CD_TOUCHED_COUNT) = CHL_I32(ACT_CD(actor), CD_TOUCHED_COUNT) + 1;
				if ((CHL_U32(platform, 0x450) & 0x8000) != 0)
					held = 1;
			}
		}
	}
	c->ignore = held;
	c->tag = (short)tag;
	coll_contact_count = coll_contact_count + 1;
}

/* 0x000b3d50  CharLib/CRCollision.cpp
Builds an actor's collision mesh from its own geometry (no "_coll.obe" mesh). */
void CRColl_BuildMesh(void *actor)
{
	void *mesh;

	if (CHL_PTR(actor, ACT_COLLMESH) != NULL)
		return;
	chl_PrepInstance(ACT_INST(actor), 1);
	mesh = chl_Calloc(0x2c, 0, "C:\\BlitzParty\\Game\\Src\\CharLib\\CRCollision.cpp", 0x992, 0x2001);
	CHL_PTR(actor, ACT_COLLMESH) = mesh;
	*(void **)CHL_PTR(actor, ACT_COLLMESH) = chl_GetActorVertices(ACT_INST(actor), CHL_PTR(actor, ACT_NODE), 0,
		NULL, CHL_FIELD(int, mesh, MESH_COUNT), NULL, NULL, 0x10000);
	CRColl_PrepareMesh(actor, 1, 1);
}

/* 0x000b3dc0  CharLib/CRCollision.cpp
Builds an actor's collision mesh from another instance (the "_coll.obe" mesh). */
void CRColl_BuildMeshFrom(void *actor, void *inst)
{
	void *mesh;

	if (CHL_PTR(actor, ACT_COLLMESH) != NULL)
		return;
	chl_PrepInstance(inst, 1);
	mesh = chl_Calloc(0x2c, 0, "C:\\BlitzParty\\Game\\Src\\CharLib\\CRCollision.cpp", 0x9aa, 0x2001);
	CHL_PTR(actor, ACT_COLLMESH) = mesh;
	*(void **)CHL_PTR(actor, ACT_COLLMESH) = chl_GetActorVertices(inst, CHL_PTR(inst, 0x134), 0, NULL,
		CHL_FIELD(int, mesh, MESH_COUNT), NULL, NULL, 0x10000);
	CRColl_PrepareMesh(actor, 1, 1);
}
