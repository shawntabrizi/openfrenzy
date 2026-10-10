/*
ENGINE_MISC_TEST_SHIM.H

Force-included when src/engine_misc/tests/run_tests.sh compiles BMATHS/unplaced.c for the
host (64-bit): stands in for of_game.h (the layout checks are off, since the host's
pointers are 8 bytes) and declares this module's collision functions and the helpers of
other modules they call (defined in maths_test.c.in with the original semantics).
*/

#ifndef ENGINE_MISC_TEST_SHIM_H
#define ENGINE_MISC_TEST_SHIM_H

#include <stddef.h>
#include <string.h>

#define OF_ASSERT_SIZE(type, size)
#define OF_ASSERT_OFFSET(type, member, offset)

struct BRay;
struct BSphere;
struct BTriangle;
struct BEdge;
struct BCollHit;
struct BCollMesh;

/* this module (engine_misc.decls) */
void bmCollMeshFixup(int package, int name);
int bmRaySphere(struct BRay *ray, struct BSphere *sphere, struct BCollHit *hit);
int bmRayMesh(struct BRay *ray, struct BCollMesh *mesh, int *count, struct BCollHit *hits);
int bmSphereSideOfPlane(struct BSphere *sphere, float *delta, float *plane);
int bmSweepSphereSphere(struct BSphere *a, struct BSphere *b, float *dir, float length, struct BCollHit *hit);
void bmSegmentSphere(float *segment, struct BSphere *sphere, struct BCollHit *hit);
int bmSweepSphereTriangle(struct BSphere *sphere, struct BTriangle *tri, float *delta, struct BCollHit *hit);
int bmSweepSphereMesh(struct BSphere *sphere, struct BCollMesh *mesh, float *delta, int *count, struct BCollHit *hits);
void bmSweepSphereSphereVec(struct BSphere *a, struct BSphere *b, float *vec, struct BCollHit *hit);
int bmSweepSphereEdge(struct BSphere *sphere, struct BEdge *edge, float *delta, struct BCollHit *hit);
int bmRayTriangle(struct BRay *ray, struct BTriangle *tri, struct BCollHit *hit);
int bmClosestLinePoints(float *a, float *b, float *s, float *t);
float bmProjectFactor(float *a, float *b);
void bmVec2Divide(float *out, float *in, float s);

/* other modules' helpers that engine_misc_extern.h casts */
void FUN_00019230(float *out, const float *a, const float *b);
void FUN_000384a0(float *out, const float *in, float s);
void FUN_0001e9d0(float *out, const float *a, const float *b);
void FUN_0001ea00(float *out, const float *in, float s);
void FUN_00036ac0(float *out, const float *a, const float *b);
void FUN_000514a0(float *out, const float *in);
void *FUN_0004e390(int, int, int, int, int, int);

#endif
