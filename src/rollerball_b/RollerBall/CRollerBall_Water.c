/*
RollerBall/CRollerBall_Water.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerBall_Water.cpp (heap tags of 0x00196fc0): the
goo pool of the "sumo" variation (vtable 0x00274a78): a classlib_fx CWaterField (the
rippling surface, made with foxhunt's constructor 0x00138790) and the goo plane model
("sumo\objects\gooplane.obe") under it. The functions other than 0x00196fc0 have no tag;
placed by address order.

Layout: +0x10 the goo plane resource, +0x20 its actor instance (embedded), +0x190 the
water field (CWaterField *).
*/

#include "rollerball_b.h"

#define RBWA_PLANE 0x10
#define RBWA_INST 0x20
#define RBWA_FIELD 0x190

/* classlib_fx: the water field's default parameters (CWaterParams, 0x40 bytes) */
extern uint32_t DAT_002ae578[16];

/* per-variation settings (records of 0x40 bytes from 0x002b4ba8): the scenery is drawn */
#define rbb_SceneryOn() (RBD_I(0x002b4ba8 + rbb_Variation * 0x40) != 0)

/* 0x00274a78  water vtable */
const void *const rbb_vtable_00274a78[1] = {
	(const void *)FUN_00196fa0, /* +0x00 scalar deleting destructor */
};

/* 0x00196de0  RollerBall/CRollerBall_Water.cpp (inferred)
Constructor. */
void CRollerBallWater_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274a78;
}

/* 0x00196df0  RollerBall/CRollerBall_Water.cpp (inferred)
Destructor. */
void CRollerBallWater_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274a78;
}

/* 0x00196e00  RollerBall/CRollerBall_Water.cpp (inferred)
Delete the water field, the plane model and its instance. */
void CRollerBallWater_Shutdown(void *self)
{
	void *field = RBB_P(self, RBWA_FIELD);

	if (field != NULL)
		RBB_VFN(field, 0x00, void *(*)(void *, int))(field, 1);
	bkDeleteResource((struct bkResource *)RBB_P(self, RBWA_PLANE));
	baDeleteActorInstance((struct BActorInstance *)RBB_ADDR(self, RBWA_INST));
}

/* 0x00196e30  RollerBall/CRollerBall_Water.cpp (inferred)
Ripple the surface (CWaterField slot 3) and drop 10 random drops of depth 0.025 (slot 9). */
void CRollerBallWater_Update(void *self)
{
	void *field;
	int i;

	if (!rbb_SceneryOn())
		return;
	field = RBB_P(self, RBWA_FIELD);
	RBB_VFN(field, 0x0c, void (*)(void *))(field);
	for (i = 0; i < 10; i++) {
		field = RBB_P(self, RBWA_FIELD);
		RBB_VFN(field, 0x24, void (*)(void *, float))(field, 0.025f);
	}
}

/* 0x00196e80  RollerBall/CRollerBall_Water.cpp (inferred)
Draw the goo plane, then the water surface (CWaterField slots 6 and 7) blended, with a
specular material. The rollerball_a Direct3D helpers (game + 0x26f4) ignore ECX: called
with their table prototypes. */
void CRollerBallWater_Draw(void *self)
{
	void *field;

	if (!rbb_SceneryOn())
		return;
	FUN_0018e5d0();
	bdPushRenderState(0xb, 0, 0);
	baDrawActorInstance((struct BActorInstance *)RBB_ADDR(self, RBWA_INST), 7);
	bdPopRenderState(0xb);
	FUN_0018e650();
	bdSetWorldIdentity();
	field = RBB_P(self, RBWA_FIELD);
	RBB_VFN(field, 0x18, void (*)(void *))(field);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	CWaterField_SetMaterial(0xffffffffu, 0xff333333u, 50.0f, 1);
	field = RBB_P(self, RBWA_FIELD);
	RBB_VFN(field, 0x1c, void (*)(void *, int))(field, 0);
	bdPopRenderState(5);
	bdPopRenderState(6);
	CWaterField_ResetMaterial();
	FUN_0018e640();
}

/* 0x00196f70  RollerBall/CRollerBall_Water.cpp (inferred)
A splash at (pos x, pos z) (CWaterField slot 10). */
void CRollerBallWater_Splash(void *self, float *pos, float depth, float inner, float outer)
{
	void *field = RBB_P(self, RBWA_FIELD);

	RBB_VFN(field, 0x28, int (*)(void *, float, float, float, float, float))(field, pos[0], pos[2],
		depth, inner, outer);
}

/* 0x00196fa0  RollerBall/CRollerBall_Water.cpp (inferred)
Scalar deleting destructor. */
void *CRollerBallWater_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274a78;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x00196fc0  RollerBall/CRollerBall_Water.cpp
Make the water: a square of half-size `size` around `pos` with `nx` x `nz` cells (the
default parameters with: damping 0.99, a quarter of the tension, height scale 1, colour
0xff00f661, texture "spotfx\sprites\steam_01.bmp", uv scale 1), and the goo plane at
`pos`. Returns 0 when the plane model is missing. */
int CRollerBallWater_Init(void *self, float *pos, float size, int nx, int nz)
{
	uint32_t params[16];
	void *mem;
	void *field;
	void *plane;

	memcpy(params, DAT_002ae578, sizeof params);
	params[1] = rbb_fbits(pos[1]);                       /* y */
	params[0] = rbb_fbits(pos[0] - size);                /* x0 */
	params[5] = (uint32_t)nx;
	params[6] = (uint32_t)nz;
	params[2] = rbb_fbits(pos[2] - size);                /* z0 */
	params[8] = 0x3f7d70a4u;                             /* damping 0.99 */
	params[14] = (uint32_t)(uintptr_t)"spotfx\\sprites\\steam_01.bmp";
	params[10] = 0x3f800000u;                            /* height scale 1 */
	params[13] = 0xff00f661u;                            /* colour */
	params[3] = rbb_fbits(size + size);                  /* size x */
	params[15] = 0x3f800000u;                            /* uv scale 1 */
	params[9] = rbb_fbits(rbb_bitsf(params[9]) * 0.25f); /* tension */
	mem = RBB_NEW(0x9090, "C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerBall_Water.cpp", 0x4e);
	field = NULL;
	if (mem != NULL)
		field = FUN_00138790(mem);   /* foxhunt: the CWaterField constructor */
	RBB_P(self, RBWA_FIELD) = field;
	RBB_VFN(field, 0x04, int (*)(void *, void *))(field, params);
	field = RBB_P(self, RBWA_FIELD);
	RBB_VFN(field, 0x08, void (*)(void *))(field);
	plane = bLoadResource((struct bkPackage *)RBB_P(rbb_Game, 0x08), "sumo\\objects\\gooplane.obe", 1, 0xdefa);
	RBB_P(self, RBWA_PLANE) = plane;
	if (plane == NULL)
		return 0;
	baInitActorInstance((struct BActorInstance *)RBB_ADDR(self, RBWA_INST), (struct BActor *)plane, 1,
		"C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerBall_Water.cpp", 0x5a);
	memcpy(RBB_V(self, RBWA_INST + 0xc0), pos, 16);
	baPrepActorInstance((struct BActorInstance *)RBB_ADDR(self, RBWA_INST), 1);
	return 1;
}

/* 0x00197150  RollerBall/CRollerBall_Water.cpp (inferred)
Make only the ring between radius `inner` and `outer` (around the origin) water: the
other cells are fixed (CWaterField flags). */
void CRollerBallWater_SetRing(void *self, float inner, float outer)
{
	char *field = RBB_P(self, RBWA_FIELD);
	float z0 = RBB_F(field, 0x0c);
	float dx = RBB_F(field, 0x904c);
	int nz = RBB_I(field, 0x1c);
	float dz = RBB_F(field, 0x9050);
	int nx = RBB_I(field, 0x18);
	float x = RBB_F(field, 0x04);
	int ix;
	int iz;

	inner = inner * inner;
	outer = outer * outer;
	for (ix = 0; ix < nx; ix++) {
		float z = z0;
		for (iz = 0; iz < nz; iz++) {
			float d2 = z * z + x * x;
			unsigned char *flags = RBB_P(RBB_P(self, RBWA_FIELD), 0x44);
			flags[iz * nx + ix] = (d2 <= inner || outer <= d2) ? 1 : 0;
			z = z + dz;
		}
		x = x + dx;
	}
}
