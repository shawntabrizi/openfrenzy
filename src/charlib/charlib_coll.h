/*
CHARLIB_COLL.H

charlib module. Collision data of CRCollision.cpp: the collision mesh of an actor (or
platform), its triangles, the character data fields the collision uses, and the contact
list of one collision step (charlib .bss).

Other modules may include this header. Only the charlib agent edits it.
*/

#ifndef OPENFRENZY_CHARLIB_COLL_H
#define OPENFRENZY_CHARLIB_COLL_H

/* ---------- collision mesh (0x2c bytes, ACT_COLLMESH; platforms keep one at 0x1a0) */

#define MESH_TRIS 0x0              /* float *: the triangles (TRI_FLOATS floats each) */
#define MESH_COUNT 0x4             /* int */
#define MESH_MIN 0x8               /* float[4]: bounding box */
#define MESH_MAX 0x18              /* float[4] */

/* ---------- collision triangle (0xa8 bytes) */

#define TRI_FLOATS 42
/* 0x00 v0, 0x10 v1, 0x20 v2 (float[4] each) */
#define TRI_NORMAL 0x30            /* float[3] */
#define TRI_BOX 0x40               /* min x, max x, min y, max y, min z, max z */
#define TRI_EDGE_DIRS 0x58         /* float[4] x 3: unit edge directions v0->v1, v1->v2, v2->v0 */
#define TRI_EDGE_LENGTHS 0x88      /* float[3] */
#define TRI_EDGE_STARTS 0x94       /* float[3]: dot(edge dir, edge start) */
#define TRI_PLANE 0xa0             /* float: dot(normal, v0) */

/* ---------- character data fields used by the collision (see charlib_actor.h) */

#define CD_PLANES 0x00             /* float *[5]: triangles touched in the last step */
#define CD_CONTACT_POINTS 0x14     /* float[4] x 5 */
#define CD_PLANE_COUNT 0x64        /* short */
#define CD_SPHERES 0x7c            /* collision spheres, 0x2c bytes each: offset (x, y, z) at
                                      0x00, radius at 0x20, capsule half height at 0x24 */
#define CD_CONTACT_EDGE 0x158      /* unsigned char[5]: the contact was an edge contact */
#define CD_SPHERE_COUNT 0x15d      /* unsigned char */
#define CD_FLOOR_PLANE 0x15e       /* short: index of the floor in CD_PLANES */
#define CD_TOUCHED 0x1bc           /* platforms touched this step (5) */
#define CD_TOUCHED_COUNT 0x1d0     /* int */

/* ---------- contacts of one collision step */

struct CRContact {
	float *tri;                /* 0x00 */
	float dist;                /* 0x04 distance of the sphere centre from the plane */
	float edge_dist;           /* 0x08 distance to the nearest edge point */
	int unk0c;                 /* 0x0c */
	int edge;                  /* 0x10 1: edge or vertex contact, 0: inside the face */
	int unk14;                 /* 0x14 */
	int floor;                 /* 0x18 the triangle is a floor (normal y > 0.7) */
	float pos[4];              /* 0x1c contact point */
	int unk2c;                 /* 0x2c */
	int ignore;                /* 0x30 */
	short sphere;              /* 0x34 the actor's sphere */
	short tag;                 /* 0x36 */
	void *platform;            /* 0x38 */
};
OF_ASSERT_SIZE(struct CRContact, 0x3c);

/* 0x002f2940  the contacts (200) */
#define coll_contacts CHL_ARR(struct CRContact, 0x002f2940)
/* 0x002f5940  the contact of the floor found / 0x002f5944 its height */
#define coll_floor_contact CHL_VAR(short, 0x002f5940)
#define coll_floor_height CHL_VAR(float, 0x002f5944)
/* 0x002f5948  contact of each push vector */
#define coll_push_contact CHL_ARR(short, 0x002f5948)
/* 0x002f5af8  push vectors (x, y, z, w: 1 for faces, 0 for edges); the original reads
index -1 (0x002f5ae8) when no push was added, which stays inside the block */
#define coll_pushes ((float (*)[4])CHL_ADDR(0x002f5af8))
/* 0x002f67a8  radius of each sphere of the moving actor */
#define coll_sphere_radii CHL_ARR(float, 0x002f67a8)
/* 0x002f67c0  centre of each sphere (float[4]) */
#define coll_sphere_centres ((float (*)[4])CHL_ADDR(0x002f67c0))
/* 0x002f6810  the actor stands on something */
#define coll_on_ground CHL_VAR(int, 0x002f6810)
/* 0x002f6818  length of each push vector */
#define coll_push_length CHL_ARR(float, 0x002f6818)
/* 0x002f6b38  push vectors */
#define coll_push_count CHL_VAR(int, 0x002f6b38)
/* 0x002f6b3c  touching a vertical wall */
#define coll_steep CHL_VAR(int, 0x002f6b3c)
/* 0x002f6b44  contacts */
#define coll_contact_count CHL_VAR(int, 0x002f6b44)
/* 0x00293970  how much an actor's weight turns a seesaw (1e-4) */
#define coll_seesaw_factor CHL_VAR(float, 0x00293970)

#endif
