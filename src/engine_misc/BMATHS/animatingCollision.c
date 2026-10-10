/*
BMATHS/animatingCollision.c: C:\Babel\Xbox\Src\BMATHS\animatingCollision.cpp, collision
shapes that follow the nodes of an animated actor. 0x00052130 is tagged.
0x00052060 is placed here by its message ("bInitInstanceNodes ... animating collision is
incompatible with supplied actor"); 0x000521c0-0x000526f0 (the function table puts
them in BSOUND's gap) work on the same nodes and are placed here too.
*/

#include "engine_misc_maths.h"
#include "engine_misc_extern.h"

static const char s_file[] = "C:\\Babel\\Xbox\\Src\\BMATHS\\animatingCollision.cpp";

/* 0x00052060  BMATHS (animatingCollision.cpp?) (bInitInstanceNodes)
Builds the nodes for a circular list of shapes (and, recursively, their children) from
nodes[0] on; returns the next free node. */
struct BCollNode *bmInitInstanceNodes(void *instances, struct BCollShape *shapes, struct BCollNode *nodes,
	struct BCollNode *parent)
{
	struct BCollShape *shape;
	struct BCollNode *node, *next;
	unsigned char *instance;

	shape = shapes;
	node = nodes;
	do {
		instance = (unsigned char *)em_baFindInstanceByCrc(instances, shape->crc);
		if (instance == NULL)
			em_bkPrintf("bInitInstanceNodes: Could not find instance node by CRC(0x%08x) - animating collision is incompatible with supplied actor\n",
				shape->crc);
		node->parent = parent;
		node->instance = instance;
		node->shape = shape;
		instance[0x7a] &= 0xfb;
		memcpy(node->vec30, instance + 0x30, 16);
		memcpy(node->vec10, instance + 0x10, 16);
		next = node + 1;
		if (shape->children == NULL) {
			node->children = NULL;
		} else {
			node->children = next;
			next = bmInitInstanceNodes(*(void **)(instance + 0x14c), shape->children, next, node);
		}
		if (shape->next == shapes)
			node->next = nodes;
		else
			node->next = next;
		node->next->prev = node;
		shape = shape->next;
		node = next;
	} while (shape != shapes);
	return node;
}

/* 0x00052130  BMATHS/animatingCollision.cpp
Makes an animating collision (allocated when coll is NULL) for a collision resource
and an actor. */
struct BAnimatingCollision *bmCreateAnimatingCollision(struct BAnimatingCollision *coll,
	unsigned char *resource, unsigned char *actor)
{
	int count;

	if (coll == NULL) {
		coll = (struct BAnimatingCollision *)em_bkAlloc(sizeof(struct BAnimatingCollision), s_file, 0x72, 0x2007);
		if (coll == NULL)
			return NULL;
		coll->owned = 1;
	} else {
		coll->owned = 0;
	}
	coll->resource = resource;
	coll->actor = actor;
	count = em_bmCountShapes(*(void **)(resource + 0x20));
	coll->nodes = (struct BCollNode *)em_bkAllocZero(count * (int)sizeof(struct BCollNode), 0, s_file, 0x80, 0x2007);
	bmInitInstanceNodes(*(void **)(actor + 0x134), *(struct BCollShape **)(resource + 0x20), coll->nodes, NULL);
	return coll;
}

/* 0x000521c0  BMATHS (animatingCollision.cpp?)
Fixes up a collision resource found in a package. */
unsigned char *bmCollisionFixup(int package, int name)
{
	unsigned char *resource;
	int shapes;

	resource = (unsigned char *)em_bkFindResource(package, name, 0, 0, 0, 0);
	if (resource == NULL)
		return NULL;
	shapes = *(int *)(resource + 0x20) + (int)resource;
	*(int *)(resource + 0x20) = shapes;
	em_bmFixupShapes(resource, (void *)shapes);
	return resource;
}

/* 0x00052200  BMATHS (animatingCollision.cpp?)
Sweeps a sphere (in world space) by delta against the triangles of a circular list of
nodes and their children; keeps up to maxHits hits sorted by t. Returns count != NULL. */
int bmCollideNodes(struct BSphere *sphere, void *coll, struct BCollNode *nodes, float *delta, int maxHits,
	int *count, struct BAnimHit *hits)
{
	struct BCollNode *node;
	struct BCollShape *shape;
	struct BCollHit hit;
	struct BSphere local;
	struct BTriangle tri;
	float matrix[16];
	float localDelta[4];
	unsigned short *t;
	unsigned int i;
	int n;

	hit.flags = hits->flags;
	hits->t = 1.0f;
	local.radius = sphere->radius;
	tri.v[2][3] = 1.0f;
	tri.v[1][3] = 1.0f;
	tri.v[0][3] = 1.0f;
	node = nodes;
	do {
		if ((node->instance[0x7a] & 0x10) == 0) {
			shape = node->shape;
			em_bmMatInvert(matrix, node->instance + 0x80);
			em_bmMatTransformVec(local.centre, matrix, sphere->centre);
			matrix[12] = 0.0f;
			matrix[13] = 0.0f;
			matrix[14] = 0.0f;
			em_bmMatTransformVec(localDelta, matrix, delta);
			t = (unsigned short *)shape->tris;
			for (i = 0; i < shape->numTris; i++, t += 3) {
				memcpy(tri.v[0], shape->verts[t[0]], 12);
				memcpy(tri.v[1], shape->verts[t[1]], 12);
				memcpy(tri.v[2], shape->verts[t[2]], 12);
				if (bmSweepSphereTriangle(&local, &tri, localDelta, &hit) == 0)
					continue;
				n = *count;
				if (n == maxHits) {
					if (hits[n - 1].t < hit.t)
						continue;
					*count = n - 1;
				}
				n = *count;
				while (n > 0) {
					if (hits[n - 1].t < hit.t)
						break;
					hits[n] = hits[n - 1];
					n--;
				}
				hits[n].one = 1;
				hits[n].instance = node->instance;
				hits[n].code = hit.code;
				hits[n].flags = hit.flags;
				hits[n].node = node;
				memcpy(hits[n].nodeVec, node->vec10, 16);
				memcpy(hits[n].normal, hit.normal, 16);
				memcpy(hits[n].point, hit.point, 16);
				hits[n].t = hit.t;
				hits[n].u = hit.u;
				hits[n].v = hit.v;
				*count = *count + 1;
			}
		}
		if (node->children != NULL)
			bmCollideNodes(sphere, coll, node->children, delta, maxHits, count, hits);
		node = node->next;
	} while (node != nodes);
	return count != NULL;
}

/* 0x00052510  BMATHS (animatingCollision.cpp?)
Copies the instances' current vectors and matrices into the nodes (recursively). */
void bmUpdateNodes(struct BCollNode *nodes)
{
	struct BCollNode *node;
	struct BCollNode *children;

	node = nodes;
	do {
		memcpy(node->vec10, node->instance + 0x10, 16);
		memcpy(node->vec30, node->instance + 0x30, 16);
		children = node->children;
		memcpy(node->matrix, node->instance + 0x80, 64);
		if (children != NULL)
			bmUpdateNodes(children);
		node = node->next;
	} while (node != nodes);
}

/* 0x00052590  BMATHS (animatingCollision.cpp?)
Sweeps a sphere (in the actor's space) against an animating collision. *count: in, the
room in hits; out, the hits. The original transforms hits[0]'s point and normal back
once per hit (with the first 16 words of the actor, not its matrix at +0x80), as here. */
int bmCollideAnimating(struct BSphere *sphere, struct BAnimatingCollision *coll, float *delta, int *count,
	struct BAnimHit *hits)
{
	struct BSphere world;
	float matrix[16];
	float worldDelta[4];
	float back[16];
	int n, i;

	em_bmMatTransformVec(world.centre, coll->actor + 0x80, sphere->centre);
	world.radius = sphere->radius;
	memcpy(matrix, coll->actor + 0x80, 64);
	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = 0.0f;
	em_bmMatTransformVec(worldDelta, matrix, delta);
	n = 0;
	if (bmCollideNodes(&world, coll, coll->nodes, worldDelta, *count, &n, hits) != 0) {
		for (i = 0; i < n; i++) {
			hits->normal[3] = 1.0f;
			hits->point[3] = 1.0f;
			memcpy(back, coll->actor, 64);
			em_bmMatTransformInPlace(back, hits->point);
			back[12] = 0.0f;
			back[13] = 0.0f;
			back[14] = 0.0f;
			em_bmMatTransformInPlace(back, hits->normal);
		}
	}
	*count = n;
	return n > 0;
}

/* 0x000526f0  BMATHS (animatingCollision.cpp?) */
void bmUpdateAnimatingCollision(struct BAnimatingCollision *coll)
{
	bmUpdateNodes(coll->nodes);
}
