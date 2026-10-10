/*
Demolition/CModule_Demolition.c  <-  C:\BlitzParty\Game\Src\Demolition\CModule_Demolition.cpp

The Demolition entry of the game list (a CGame, see core): its name and the
four variation names, and the init, shutdown and start that core calls. The
constructor and destructor (0x000e7af0, 0x000e7b20) are unplaced in the
table; they set this file's vtable, so they are placed here.
*/

#include "demolition_a.h"

static const char CModule_Demolition_cpp[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CModule_Demolition.cpp";

void *CModuleDemolition_DeletingDtor(void *self, unsigned int flags);
int CModuleDemolition_Init(void *self);
void CModuleDemolition_Shutdown(void *self);
int CModuleDemolition_Start(void *self, int variation);

/* 0x002699e8: the vtable of the game entry (core's struct CGameVtbl) */
static const void *const vtable_002699e8[] = {
	(const void *)CModuleDemolition_DeletingDtor, /* [0] +0x00 */
	(const void *)CModuleDemolition_Init,         /* [1] +0x04 */
	(const void *)CModuleDemolition_Shutdown,     /* [2] +0x08 */
	(const void *)CModuleDemolition_Start,        /* [3] +0x0c */
};

/* 0x000e7af0  Demolition/CModule_Demolition.cpp (file inferred from the vtable)
The constructor: the game name and the four variation names. */
void *CModuleDemolition_Ctor(void *self)
{
	DA_GameCtor(self, "Demolition", "Population Explosion", "Misguided Missile", "Boom Town",
		"Blast Man Standing");
	*(const void *const **)self = vtable_002699e8;
	return self;
}

/* 0x000e7b20  Demolition/CModule_Demolition.cpp (file inferred from the vtable) */
void CModuleDemolition_Dtor(void *self)
{
	*(const void *const **)self = vtable_002699e8;
	DA_GameDtor(self);
}

/* 0x000e7b30  Demolition/CModule_Demolition.cpp
vtable slot 2: deletes the game object (its virtual deleting destructor). The pointer is not
cleared. */
void CModuleDemolition_Shutdown(void *self)
{
	(void)self;
	DA_Nop(DAT_002ff85c);
	if (DAT_002ff85c != NULL)
		DA_VFN(DAT_002ff85c, 0x00, void *(*)(void *, unsigned int))(DAT_002ff85c, 1);
}

/* 0x000e7b50  Demolition/CModule_Demolition.cpp
vtable slot 3: starts variation VARIATION (1-based) on the game object. */
int CModuleDemolition_Start(void *self, int variation)
{
	(void)self;
	return CDemolitionGame_Start(DAT_002ff85c, variation);
}

/* 0x000e7b60  Demolition/CModule_Demolition.cpp
The scalar deleting destructor (flags bit 0: free the object). */
void *CModuleDemolition_DeletingDtor(void *self, unsigned int flags)
{
	CModuleDemolition_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000e7b80  Demolition/CModule_Demolition.cpp
vtable slot 1: makes the game object (0x7be0 bytes, line 0x37). Always succeeds. */
int CModuleDemolition_Init(void *self)
{
	void *memory;

	(void)self;
	memory = DA_New(DG_SIZE, CModule_Demolition_cpp, 0x37);
	if (memory == NULL)
		DAT_002ff85c = NULL;
	else
		DAT_002ff85c = CDemolitionGame_Ctor(memory);
	return DA_Return1();
}
