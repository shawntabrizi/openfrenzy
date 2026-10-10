/*
ENGINE_MISC_MATHS.H

Babel BMATHS collision (C:\Babel\Xbox\Src\BMATHS\): animating collision
(animatingCollision.cpp), swept sphere / ray tests against triangles, edges and
spheres, collision meshes with a cell grid, and the 2D tests of the late engine tail.
All the layouts are the original 32-bit ones.

Owner: engine_misc.
*/

#ifndef OPENFRENZY_ENGINE_MISC_MATHS_H
#define OPENFRENZY_ENGINE_MISC_MATHS_H

/* what a test reports (hit->flags says what to fill) */
#define BCOLL_T 0x01          /* t: 0-1 along the movement */
#define BCOLL_UV 0x02         /* u, v: position on the triangle/edge */
#define BCOLL_NORMAL 0x04
#define BCOLL_POINT 0x08
#define BCOLL_REASON 0x10     /* code: why a test failed (0x100 too far, 0x200 behind,
                                 0x800 outside) or 2 (hit an edge) */

/* a hit record: 13 words (0x34 bytes) */
struct BCollHit {
	unsigned int flags;       /* 0x00 BCOLL_* */
	float t;                  /* 0x04 */
	float u, v;               /* 0x08 */
	float normal[4];          /* 0x10 */
	float point[4];           /* 0x20 */
	int code;                 /* 0x30 reason, or the triangle index in mesh queries */
};
OF_ASSERT_SIZE(struct BCollHit, 0x34);

/* a ray: origin, direction (unit or not, per test) and length */
struct BRay {
	float origin[4];
	float dir[4];
	float length;
};

/* a sphere: centre and radius */
struct BSphere {
	float centre[4];
	float radius;
};

/* a triangle (3 points with w) and an edge (2 points) */
struct BTriangle {
	float v[3][4];
};
struct BEdge {
	float a[4];
	float b[4];
};

/* ---------- collision meshes */

/* a cell of a mesh's grid: 16 bytes */
struct BCollCell {
	int count;                /* triangles */
	unsigned short *list;     /* triangle numbers, relative to base */
	short base;
	short pad;
	int pad0c;
};

/* a collision mesh with its grid (fixed up by bmCollMeshFixup) */
struct BCollMesh {
	unsigned char pad00[0x20];
	int cells[3];             /* 0x20 cells along x, y, z */
	float (*verts)[3];        /* 0x2c */
	float cellSize[3];        /* 0x30 */
	unsigned short (*tris)[3]; /* 0x3c */
	float origin[3];          /* 0x40 */
	struct BCollCell *grid;   /* 0x4c */
};
OF_ASSERT_OFFSET(struct BCollMesh, grid, 0x4c);

/* ---------- animating collision (animatingCollision.cpp) */

/* a collision shape of an actor's skeleton (in the collision resource) */
struct BCollShape {
	struct BCollShape *next;  /* 0x00 circular list of siblings */
	int field_04, field_08;
	struct BCollShape *children; /* 0x0c */
	int crc;                  /* 0x10 the instance node it follows */
	unsigned char pad14[0x34 - 0x14];
	float (*verts)[3];        /* 0x34 */
	unsigned int numTris;     /* 0x38 */
	unsigned short (*tris)[3]; /* 0x3c */
};

/* a node of an animating collision (0x78 bytes), one per shape */
struct BCollNode {
	struct BCollNode *next;   /* 0x00 */
	struct BCollNode *prev;   /* 0x04 */
	struct BCollNode *parent; /* 0x08 */
	struct BCollNode *children; /* 0x0c */
	float vec10[4];           /* 0x10 copy of the instance's +0x10 */
	float vec30[4];           /* 0x20 copy of the instance's +0x30 */
	float matrix[16];         /* 0x30 copy of the instance's matrix (+0x80) */
	struct BCollShape *shape; /* 0x70 */
	unsigned char *instance;  /* 0x74 the actor instance node (module bactor) */
};
OF_ASSERT_SIZE(struct BCollNode, 0x78);

/* 0x10 bytes */
struct BAnimatingCollision {
	unsigned char *resource;  /* 0x00 the collision resource (+0x20: its shapes) */
	unsigned char *actor;     /* 0x04 (+0x80: matrix, +0x134: instance list) */
	struct BCollNode *nodes;  /* 0x08 */
	int owned;                /* 0x0c 1: allocated by bmCreateAnimatingCollision */
};

/* a hit against an animating collision: 20 words (0x50 bytes) */
struct BAnimHit {
	unsigned int flags;       /* 0x00 */
	int one;                  /* 0x04 */
	float t;                  /* 0x08 */
	int code;                 /* 0x0c */
	float u, v;               /* 0x10 */
	struct BCollNode *node;   /* 0x18 */
	unsigned char *instance;  /* 0x1c */
	float normal[4];          /* 0x20 */
	float point[4];           /* 0x30 */
	float nodeVec[4];         /* 0x40 the node's vec10 */
};
OF_ASSERT_SIZE(struct BAnimHit, 0x50);

/* ---------- 2D tests (engine tail): a circle {x, y, r}, a segment {x0, y0, x1, y1} */

struct BHit2D {
	float normal[2];          /* 0x00 */
	float point[2];           /* 0x08 */
	unsigned int flags;       /* 0x10 BCOLL_T / BCOLL_NORMAL / BCOLL_POINT */
	float t;                  /* 0x14 */
};

#endif
