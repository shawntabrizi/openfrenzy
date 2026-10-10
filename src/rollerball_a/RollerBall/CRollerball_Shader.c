/*
RollerBall/CRollerball_Shader.c  <-  C:\BlitzParty\Game\Src\RollerBall\CRollerball_Shader.cpp

The shader system (game +0x2840, vtable 0x0027435c): Initialise is tagged with this file;
the constructor, destructors and Shutdown around it are gap functions placed here.
*/

#include "rollerball_a_ext.h"

/* 0x0027435c  vtable of the shader system */
const void *const vtable_0027435c[1] = { (const void *)CRollerBallShaders_DeletingDtor };

/* 0x0018dcc0  RollerBall (gap): constructor */
void CRollerBallShaders_Ctor(void *self)
{
	*(const void *const **)self = vtable_0027435c;
}

/* 0x0018dcd0  RollerBall (gap): destructor */
void CRollerBallShaders_Dtor(void *self)
{
	*(const void *const **)self = vtable_0027435c;
}

/* 0x0018dce0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallShaders_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0027435c;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018dd00  RollerBall/CRollerball_Shader.cpp
Initialise: a shader object (0x24 bytes, rollerball_b 0x0019a4e0) at +4 and a list of one
(8 bytes) at +8 that points to it. Returns 1. */
int CRollerBallShaders_Init(void *self)
{
	void *obj;
	void **list;

	obj = RB_NEW(0x24, RB_FILE_SHADER, 0x37);
	if (obj != NULL)
		obj = rbb_ParticlesCtor(obj);
	RB_PTR(self, 4) = obj;
	list = RB_NEW(8, RB_FILE_SHADER, 0x38);
	RB_PTR(self, 8) = list;
	*list = RB_PTR(self, 4);
	return 1;
}

/* 0x0018dd90  RollerBall (gap; CRollerball_Shader.cpp inferred): Shutdown */
void CRollerBallShaders_Shutdown(void *self)
{
	RB_DELETE(RB_PTR(self, 4));
	RB_DELETE(RB_PTR(self, 8));
}
