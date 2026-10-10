/*
RollerBall/CRollerBall_Light.c  <-  C:\BlitzParty\Game\Src\RollerBall\CRollerBall_Light.cpp

The light system (game +0x25b4, vtable 0x00274644) and its lights. Initialise
(0x0018fcf0) is tagged with this file; the light classes and the system's other methods
before it (0x0018fa10-0x0018fcd0) are gap functions placed here (inferred: the system's
vtable sits next to this file's name in .rdata). Demolition calls the same light methods.

A light (vtable 0x002745fc abstract, 0x00274614 directional, 0x0027462c point): +4 the
BACTOR light, +8 its type (1 point, 2 directional), +0xc next light of the system,
+0x10 on, +0x11 changed, +0x14 position (4 floats), +0x24 range, +0x28..+0x30 colour,
+0x34 direction (directional lights, 0x44 bytes; point lights are 0x34 bytes).
*/

#include "rollerball_a_ext.h"

/* a pure virtual slot (MSVC's _purecall at 0x001e0323 ends the program) */
static void rb_purecall(void)
{
	bkPrintf("R6025 pure virtual function call (CRollerBall_Light)\n");
	abort();
}

/* 0x002745fc  vtable of the abstract light */
const void *const vtable_002745fc[6] = {
	(const void *)CRollerBallLight_DeletingDtor,
	(const void *)rb_purecall,
	(const void *)rb_purecall,
	(const void *)rb_purecall,
	(const void *)rb_purecall,
	(const void *)rb_purecall,
};

/* 0x00274614  vtable of a directional light */
const void *const vtable_00274614[6] = {
	(const void *)CRollerBallLight_DerivedDeletingDtor, /* +0x00 */
	(const void *)CRollerBallDirLight_Init,             /* +0x04 */
	(const void *)CDemoLight_Shutdown,                  /* +0x08 */
	(const void *)CDemoDirLight_Enable,                 /* +0x0c */
	(const void *)FUN_001ec260,                         /* +0x10 Update (empty) */
	(const void *)FUN_001d5950,                         /* +0x14 (empty) */
};

/* 0x0027462c  vtable of a point light */
const void *const vtable_0027462c[6] = {
	(const void *)CRollerBallLight_DerivedDeletingDtor, /* +0x00 */
	(const void *)CRollerBallPointLight_Init,           /* +0x04 */
	(const void *)CDemoLight_Shutdown,                  /* +0x08 */
	(const void *)CRollerBallPointLight_Enable,         /* +0x0c */
	(const void *)CRollerBallPointLight_Update,         /* +0x10 */
	(const void *)FUN_001d5950,                         /* +0x14 (empty) */
};

/* 0x00274644  vtable of the light system */
const void *const vtable_00274644[1] = { (const void *)CRollerBallLights_DeletingDtor };

/* 0x0018fa10  RollerBall (gap): SetColour (marks the light changed) */
void CRollerBallLight_SetColour(void *self, int r, int g, int b)
{
	RB_I32(self, 0x28) = r;
	RB_I32(self, 0x2c) = g;
	RB_I32(self, 0x30) = b;
	RB_U8(self, 0x11) = 1;
}

/* 0x0018fa30  RollerBall (gap): SetRange (marks the light changed) */
void CRollerBallLight_SetRange(void *self, float range)
{
	RB_F32(self, 0x24) = range;
	RB_U8(self, 0x11) = 1;
}

/* 0x0018fa40  RollerBall (gap): Initialise (directional): a BACTOR light of type 2, off.
Returns 1. */
int CRollerBallDirLight_Init(void *self)
{
	RB_PTR(self, 0xc) = NULL;
	RB_I32(self, 8) = 2;
	RB_PTR(self, 4) = baCreateLight(NULL, 0);
	RB_U8(self, 0x10) = 0;
	return 1;
}

/* 0x0018fa70  RollerBall (gap): point light constructor */
void CRollerBallPointLight_Ctor(void *self)
{
	*(const void *const **)self = vtable_0027462c;
}

/* 0x0018fa80  RollerBall (gap): light destructor (the abstract base) */
void CRollerBallLight_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745fc;
}

/* 0x0018fa90  RollerBall (gap): Initialise (point): a BACTOR light of type 1, range 0.01,
off. Returns 1. */
int CRollerBallPointLight_Init(void *self)
{
	RB_PTR(self, 0xc) = NULL;
	RB_I32(self, 8) = 1;
	RB_PTR(self, 4) = baCreateLight(NULL, 0);
	RB_F32(self, 0x24) = 0.01f;
	RB_U8(self, 0x11) = 1;
	RB_U8(self, 0x10) = 0;
	return 1;
}

/* 0x0018fad0  RollerBall (gap): Update (point): a changed light that is on gets its position,
range and colour (when the range is positive) and is enabled */
void CRollerBallPointLight_Update(void *self, float dt)
{
	(void)dt;
	if (RB_U8(self, 0x10) == 0 || RB_U8(self, 0x11) == 0)
		return;
	if (RB_F32(self, 0x24) > 0.0f) {
		baSetPointLight(RB_PTR(self, 4), RB_FP(self, 0x14), RB_F32(self, 0x24));
		baSetLightColour(RB_PTR(self, 4), RB_I32(self, 0x28), RB_I32(self, 0x2c),
			RB_I32(self, 0x30));
		baEnableLight(RB_PTR(self, 4), 1);
	}
	RB_U8(self, 0x11) = 0;
}

/* 0x0018fb30  RollerBall (gap): Enable (point) */
void CRollerBallPointLight_Enable(void *self, int on)
{
	RB_U8(self, 0x10) = (unsigned char)on;
	if ((char)on != 0 && RB_F32(self, 0x24) > 0.0f) {
		baSetPointLight(RB_PTR(self, 4), RB_FP(self, 0x14), RB_F32(self, 0x24));
		baSetLightColour(RB_PTR(self, 4), RB_I32(self, 0x28), RB_I32(self, 0x2c),
			RB_I32(self, 0x30));
	}
	baEnableLight(RB_PTR(self, 4), RB_U8(self, 0x10));
}

/* 0x0018fb90  RollerBall (gap): light system constructor */
void CRollerBallLights_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274644;
}

/* 0x0018fba0  RollerBall (gap): light system destructor */
void CRollerBallLights_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274644;
}

/* 0x0018fbb0  RollerBall (gap): adds LIGHT at the head of the list (+4, lights linked by
+0xc) */
void CRollerBallLights_Add(void *self, void *light)
{
	if (RB_PTR(self, 4) == NULL) {
		RB_PTR(light, 0xc) = NULL;
		RB_PTR(self, 4) = light;
		return;
	}
	RB_PTR(light, 0xc) = RB_PTR(self, 4);
	RB_PTR(self, 4) = light;
}

/* 0x0018fbe0  RollerBall (gap): Shutdown: every light and the main light (+8) are shut
down (virtual +8) and deleted (virtual deleting destructor) */
void CRollerBallLights_Shutdown(void *self)
{
	unsigned char *l, *next;
	void *main;

	l = RB_PTR(self, 4);
	while (l != NULL) {
		next = RB_PTR(l, 0xc);
		RB_VFN(l, 0x08, void (*)(void *))(l);
		RB_VFN(l, 0x00, void *(*)(void *, unsigned int))(l, 1);
		l = next;
	}
	main = RB_PTR(self, 8);
	RB_VFN(main, 0x08, void (*)(void *))(main);
	main = RB_PTR(self, 8);
	if (main != NULL)
		RB_VFN(main, 0x00, void *(*)(void *, unsigned int))(main, 1);
}

/* 0x0018fc30  RollerBall (gap): Update: every light (virtual +0x10) */
void CRollerBallLights_Update(void *self, float dt)
{
	unsigned char *l;

	for (l = RB_PTR(self, 4); l != NULL; l = RB_PTR(l, 0xc))
		RB_VFN(l, 0x10, void (*)(void *, float))(l, dt);
}

/* 0x0018fc60  RollerBall (gap): abstract light deleting destructor (flags bit 0: free) */
void *CRollerBallLight_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745fc;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018fc80  RollerBall (gap): SetPosition (marks the light changed) */
void CRollerBallLight_SetPosition(void *self, const float *pos)
{
	RB_F32(self, 0x14) = pos[0];
	RB_F32(self, 0x18) = pos[1];
	RB_F32(self, 0x1c) = pos[2];
	RB_F32(self, 0x20) = pos[3];
	RB_U8(self, 0x11) = 1;
}

/* 0x0018fcb0  RollerBall (gap): point and directional light deleting destructor */
void *CRollerBallLight_DerivedDeletingDtor(void *self, unsigned int flags)
{
	CRollerBallLight_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018fcd0  RollerBall (gap): light system deleting destructor (flags bit 0: free) */
void *CRollerBallLights_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00274644;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018fcf0  RollerBall/CRollerBall_Light.cpp
Initialise: no lights; the main light (+8) is a point light (variations 0, 1, 4: position
from 0x002b5018 + 16 x variation, range 0.0002) or a directional light (2, 3: the
direction from the same table); grey 196 and on. Returns 1. */
int CRollerBallLights_Init(void *self)
{
	unsigned char *l = NULL;
	unsigned char *obj;
	int v;

	RB_PTR(self, 4) = NULL;
	switch (RB_I32(RB_GAME, RB_GO_VARIATION)) {
	case 0:
	case 1:
	case 4:
		obj = RB_NEW(0x34, RB_FILE_LIGHT, 0x210);
		if (obj != NULL) {
			*(const void *const **)obj = vtable_0027462c;
			l = obj;
		}
		RB_PTR(self, 8) = l;
		RB_VFN(l, 0x04, int (*)(void *))(l);
		v = RB_I32(RB_GAME, RB_GO_VARIATION);
		RB_U32(l, 0x14) = RB_VAR(unsigned int, 0x002b5018u + v * 0x10);
		RB_U32(l, 0x18) = RB_VAR(unsigned int, 0x002b501cu + v * 0x10);
		RB_U32(l, 0x1c) = RB_VAR(unsigned int, 0x002b5020u + v * 0x10);
		RB_U32(l, 0x20) = RB_VAR(unsigned int, 0x002b5024u + v * 0x10);
		RB_F32(l, 0x24) = rb_bitsf(0x3951b717u);
		break;
	case 2:
	case 3:
		obj = RB_NEW(0x44, RB_FILE_LIGHT, 0x204);
		if (obj != NULL) {
			*(const void *const **)obj = vtable_00274614;
			l = obj;
		}
		RB_PTR(self, 8) = l;
		RB_VFN(l, 0x04, int (*)(void *))(l);
		v = RB_I32(RB_GAME, RB_GO_VARIATION);
		RB_U32(l, 0x34) = RB_VAR(unsigned int, 0x002b5018u + v * 0x10);
		RB_U32(l, 0x38) = RB_VAR(unsigned int, 0x002b501cu + v * 0x10);
		RB_U32(l, 0x3c) = RB_VAR(unsigned int, 0x002b5020u + v * 0x10);
		RB_U32(l, 0x40) = RB_VAR(unsigned int, 0x002b5024u + v * 0x10);
		break;
	default:
		goto colour;
	}
	RB_U8(l, 0x11) = 1;
colour:
	l = RB_PTR(self, 8);
	RB_I32(l, 0x28) = 0xc4;
	RB_I32(l, 0x2c) = 0xc4;
	RB_I32(l, 0x30) = 0xc4;
	RB_U8(l, 0x11) = 1;
	l = RB_PTR(self, 8);
	RB_VFN(l, 0x0c, void (*)(void *, int))(l, 1);
	return 1;
}
