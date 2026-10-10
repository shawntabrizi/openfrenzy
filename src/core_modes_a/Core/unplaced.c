/*
Core/unplaced.c  (module core_modes_a)

Functions of C:\BlitzParty\Game\Src\Core\ in 0x00011000-0x0001b8e0 whose source
file is not known (gaps between tagged functions). Most belong to the file
before or after them; see the comment of each function.
*/

#include "core_modes_a.h"
#include "core_modes_a_modes.h"

/* ---------- 0x00012760-0x000128c0: after BG_Effects.cpp */

/* 0x00012760  Core (unplaced): CBGText scalar deleting destructor */
struct CBGText *CBGText_Delete(struct CBGText *self, unsigned int flags)
{
	CBGText_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00012780  Core (unplaced): CBGTextBuf scalar deleting destructor */
struct CBGTextBuf *CBGTextBuf_Delete(struct CBGTextBuf *self, unsigned int flags)
{
	CBGTextBuf_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x000127a0  Core (unplaced): CBGTypeTextBox scalar deleting destructor */
struct CBGTypeTextBox *CBGTypeTextBox_Delete(struct CBGTypeTextBox *self, unsigned int flags)
{
	CBGTypeTextBox_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x000127c0  Core (unplaced): CBGTypeTextBox destructor (the code of CBGTypeText_dtor) */
void CBGTypeTextBox_dtor(struct CBGTypeTextBox *self)
{
	struct CBGTypeText *type = &self->type;

	type->buf.text.base.vtbl = vtable_00253810;
	if (type->typeSound != 0 && type->voice >= 0)
		FUN_000546e0(type->voice);
	CBGTextBuf_dtor(&type->buf);
}

/* 0x000127d0  Core (unplaced): operator delete */
void Core_OperatorDelete(void *block)
{
	CMA_Delete(block);
}

/* 0x000127e0  Core (unplaced): copy 4 floats */
void Core_Vec4Copy(float *dst, float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}

/* 0x00012800  Core (unplaced): CGameMode constructor (the class of FinishedLoading,
   Core/CGameMode_AskAgain.cpp) */
struct CGameMode *CGameMode_ctor(struct CGameMode *self, char *name)
{
	self->vtbl = vtable_002538a4;
	CMA_List_Init(&self->list);
	self->name = name;
	self->refCount = 0;
	self->flags = 0;
	FUN_0004c290((undefined4)name);
	return self;
}

/* 0x00012860  Core (unplaced): CGameMode destructor */
void CGameMode_dtor(struct CGameMode *self)
{
	self->vtbl = vtable_002538a4;
	FUN_0004c2c0();
	FUN_001d5950(); /* ~CMA_List (nothing) */
}

/* 0x000128b0  Core (unplaced): the mode stack pushed this mode */
void CGameMode_AddRef(struct CGameMode *self)
{
	self->refCount = self->refCount + 1;
}

/* 0x000128c0  Core (unplaced): the mode stack popped this mode: delete it with the last
   reference (returns 0), else return the references left */
int CGameMode_Release(struct CGameMode *self)
{
	if (self->refCount == 1) {
		CMA_VFN(self, 0, void *(*)(void *, unsigned int))(self, 1);
		return 0;
	}
	self->refCount = self->refCount - 1;
	return self->refCount;
}
