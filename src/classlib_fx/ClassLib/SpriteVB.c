/*
ClassLib/SpriteVB.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\SpriteVB.cpp

A shared vertex buffer for debris quads ("debrisVB"), split into one batch per sprite
type. Only 0x00086500 is tagged with SpriteVB.cpp; 0x000861b0-0x000863c0 work on the
same object, so they are placed here (file inferred).
*/

#include "classlib_fx.h"

static const char s_SpriteVBFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\SpriteVB.cpp";

/* bdisplay vertex buffers (assumed prototypes): create (out, vertices, fvf, a4, a5),
lock (vb, flags) -> vertices, unlock, draw (type, vb, first vertex, count), release */
#define SVB_CreateVB ((int (*)(int *, int, int, int, int))FUN_0003f500)
#define SVB_LockVB ((byte *(*)(int, int))FUN_0003f710)
#define SVB_DrawVB ((void (*)(int, int, int, int))FUN_0003f4a0)

/* 0x000861b0  ClassLib (gap; file inferred: SpriteVB.cpp)
Unlocks the buffer and draws every batch with its model's texture and blend flags. */
void SpriteVB_Draw(CSpriteVB *self)
{
	CSpriteVBBatch *b;
	ushort flags;
	int key;

	if (self->vb != 0 && self->count != 0) {
		FUN_00044b00(4, 0, 0);
		for (b = self->next; b != (CSpriteVBBatch *)self; b = b->next)
			b->numVertices = (int)(b->write - b->start) / 0x1c;
		FUN_0003f790((struct bdVertexBuffer *)self->vb);
		self->locked = 0;
		FUN_00044b00(5, 0, 0);
		FUN_00044b00(6, 1, 0);
		FUN_00043b70(1);
		for (b = self->next; b != (CSpriteVBBatch *)self; b = b->next) {
			key = b->key;
			FUN_000416c0(0, (struct bdTexture *)(**(int **)(**(int **)(key + 0xc) + 0x40)));
			FUN_00044b00(7, (*(uint *)(key + 0x28) & 0x400) == 0 ? 3 : 1, 0);
			flags = *(ushort *)(**(int **)(key + 0xc) + 0x46);
			if ((flags & 1) == 0) {
				if ((flags & 2) == 0)
					FUN_00044b00(10, 0, 0);
				else
					FUN_00044b00(10, 3, 0);
			} else {
				FUN_00044b00(10, 1, 0);
			}
			if (5 < b->numVertices)
				SVB_DrawVB(4, self->vb, b->firstVertex, b->numVertices);
		}
		FUN_00044b00(5, 1, 0);
		FUN_00044b00(10, 0, 0);
		FUN_00043b70(1);
	}
}

/* 0x000862f0  ClassLib (gap; file inferred: SpriteVB.cpp)
debrisVB.ForceBufferSize(num): recreates the buffer with at least 12000 vertices. */
void SpriteVB_ForceBufferSize(CSpriteVB *self, int size)
{
	if (self->vb != 0)
		FUN_0003f650((struct bdVertexBuffer *)self->vb);
	if (size < 12000)
		size = 12000;
	self->vbSize = size;
	if (SVB_CreateVB(&self->vb, size, 0x144, 0, 1) == 0) {
		self->vb = 0;
		self->vbSize = 0;
		CLF_Printf("ERROR: unable to create vertex buffer size %d\n", 0);
	}
}

/* 0x00086350  ClassLib (gap; file inferred: SpriteVB.cpp)
Releases the buffer and deletes every batch. */
void SpriteVB_Release(CSpriteVB *self)
{
	CSpriteVBBatch *b;
	CSpriteVBBatch *next;

	b = self->next;
	if (self->vb != 0) {
		FUN_0003f650((struct bdVertexBuffer *)self->vb);
		self->vb = 0;
		self->vbSize = 0;
	}
	if (self->count != 0) {
		while (b != (CSpriteVBBatch *)self) {
			if (self->last == b)
				self->last = NULL;
			next = b->next;
			b->prev->next = next;
			b->next->prev = b->prev;
			CLF_Free(b);
			self->count = self->count + -1;
			b = next;
		}
	}
}

/* 0x000863c0  ClassLib (gap; file inferred: SpriteVB.cpp)
Deletes empty batches, gives the others their vertex ranges (6 or 10 vertices per
sprite), grows the buffer when needed and locks it. */
void SpriteVB_Lock(CSpriteVB *self)
{
	CSpriteVBBatch *b;
	CSpriteVBBatch *next;
	byte *base;
	int total;
	int size;

	total = 0;
	b = self->next;
	while (b != (CSpriteVBBatch *)self) {
		if (b->count < 1) {
			if (self->last == b)
				self->last = NULL;
			next = b->next;
			b->prev->next = next;
			b->next->prev = b->prev;
			CLF_Free(b);
			self->count = self->count + -1;
			b = next;
		} else {
			b->firstVertex = total;
			if (*(int *)(b->key + 0x24) == 0)
				b->numVertices = b->count * 6;
			else
				b->numVertices = b->count * 10;
			total = total + b->numVertices;
			b = b->next;
		}
	}
	if (self->count == 0)
		return;
	if (self->vbSize < total || self->vb == 0) {
		if (self->vb != 0) {
			CLF_Printf("***WARNING!*** Your debris system Vertex buffer is being resized!\n %d required. "
			           "Use debrisVB.ForceBufferSize(num) to initialise it",
			           total);
			FUN_0003f650((struct bdVertexBuffer *)self->vb);
		}
		size = 12000;
		if (11999 < total)
			size = total;
		self->vbSize = size;
		if (SVB_CreateVB(&self->vb, size, 0x144, 0, 1) == 0) {
			self->vb = 0;
			self->vbSize = 0;
			CLF_Printf("ERROR: unable to create vertex buffer size %d\n", 0);
			return;
		}
	}
	base = SVB_LockVB(self->vb, 4);
	self->lockStart = base;
	self->lockEnd = base;
	if (base != NULL)
		self->locked = 1;
	for (b = self->next; b != (CSpriteVBBatch *)self; b = b->next) {
		b->write = self->lockStart + b->firstVertex * 0x1c;
		b->start = b->write;
	}
	FUN_00043930((float *)&DAT_002855f0);
}

/* 0x00086500  ClassLib/SpriteVB.cpp
Finds (or adds) the batch of sprite type `key` and counts one more sprite in it. */
CSpriteVBBatch *SpriteVB_GetBatch(CSpriteVB *self, int key)
{
	CSpriteVBBatch *b;

	b = self->last;
	if (b != NULL && b->key == key) {
		b->count = b->count + 1;
		return self->last;
	}
	for (b = self->next; b != (CSpriteVBBatch *)self; b = b->next) {
		if (b->key == key) {
			b->count = b->count + 1;
			self->last = b;
			return b;
		}
	}
	b = CLF_Calloc(sizeof(CSpriteVBBatch), 0, s_SpriteVBFile, 0x139, 0x2001);
	b->key = key;
	b->count = 1;
	self->next->prev = b;
	b->next = self->next;
	self->next = b;
	b->prev = (CSpriteVBBatch *)self;
	self->count = self->count + 1;
	self->last = b;
	return b;
}
