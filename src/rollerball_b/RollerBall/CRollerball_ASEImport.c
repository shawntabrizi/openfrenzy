/*
RollerBall/CRollerball_ASEImport.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerball_ASEImport.cpp (heap tags of 0x00198730 and
0x00198840): reading shapes (SHAPE_VERTEXCOUNT / SHAPE_VERTEX_KNOT lines) from a 3D
Studio ASE text file, for the race paths of rollerball_a.

- the shape (vtable 0x00274b4c): +0x04 points (float[4] each, x, z, y of the file: the
  ASE z axis is up), +0x08 point count.
- the text reader (vtable 0x00274b8c; the same class as carryon_a's CoTextReader):
  +0x04 start, +0x08 cursor, +0x0c end. Its line functions are COMDAT code in other
  modules (rowing_b 0x001b9e00 reads a line, carryon_a 0x000c3a50 skips to the next).

0x001989c0 (Open) is also called by rowing_b, carryon_a and demolition_a.
*/

#include "rollerball_b.h"

static const char rbb_AseFile[] = "C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerball_ASEImport.cpp";

/* rowing_b 0x001b9e00: copy the current line into `line` */
#define rbb_AseReadLine(rd, line) RBB_FN(FUN_001b9e00, void, (void *, char *))((rd), (line))
/* carryon_a 0x000c3a50: move the cursor to the next line */
#define rbb_AseSkipLine(rd) CFire_TextReader_SkipLine((struct CoTextReader *)(rd))
/* the reader has text left */
#define rbb_AseMore(rd) (RBB_U(rd, 0x08) < RBB_U(rd, 0x0c))

/* 0x00274b4c  shape vtable */
const void *const rbb_vtable_00274b4c[1] = {
	(const void *)FUN_00198710, /* +0x00 scalar deleting destructor */
};

/* 0x00274b8c  text reader vtable */
const void *const rbb_vtable_00274b8c[1] = {
	(const void *)FUN_001989e0, /* +0x00 scalar deleting destructor */
};

/* 0x001986e0  RollerBall/CRollerball_ASEImport.cpp (inferred)
Shape constructor. */
void CRollerBallAseShape_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274b4c;
	RBB_P(self, 0x04) = NULL;
	RBB_I(self, 0x08) = 0;
}

/* 0x00198700  RollerBall/CRollerball_ASEImport.cpp (inferred)
Shape destructor (the points are not freed). */
void CRollerBallAseShape_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274b4c;
}

/* 0x00198710  RollerBall/CRollerball_ASEImport.cpp (inferred)
Shape scalar deleting destructor. */
void *CRollerBallAseShape_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274b4c;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x00198730  RollerBall/CRollerball_ASEImport.cpp
Read every shape point of the rest of the file (each SHAPE_VERTEXCOUNT starts a new
array; the last one wins). */
void CRollerBallAseShape_Read(void *self, void *reader)
{
	char line[1024];
	float x;
	float y;
	float z;

	while (rbb_AseMore(reader)) {
		rbb_AseReadLine(reader, line);
		rbb_AseSkipLine(reader);
		if (strstr(line, "SHAPE_VERTEXCOUNT") != NULL) {
			int *count = (int *)RBB_ADDR(self, 0x08);
			sscanf(line, "%*s%d", count);
			RBB_P(self, 0x04) = RBB_NEW(*count << 4, rbb_AseFile, 0x44);
			*count = 0;
		}
		if (strstr(line, "SHAPE_VERTEX_KNOT") != NULL && RBB_P(self, 0x04) != NULL) {
			float *p;
			sscanf(line, "%*s%*s%f%f%f", &x, &y, &z);
			p = (float *)RBB_P(self, 0x04) + RBB_I(self, 0x08) * 4;
			p[0] = x;
			p[1] = z;
			p[2] = y;
			RBB_I(self, 0x08) += 1;
		}
	}
}

/* 0x00198840  RollerBall/CRollerball_ASEImport.cpp
Read the shape named `name`: skip lines up to the first that contains it, then read its
SHAPE_VERTEXCOUNT and knots until all are read. (The original's count is uninitialised
until the SHAPE_VERTEXCOUNT line; 0 here.) */
void CRollerBallAseShape_ReadNamed(void *self, void *reader, const char *name)
{
	char line[1024];
	int *count = (int *)RBB_ADDR(self, 0x08);
	int found = 0;
	int total = 0;
	float x;
	float y;
	float z;

	*count = 0;
	while (rbb_AseMore(reader)) {
		rbb_AseReadLine(reader, line);
		rbb_AseSkipLine(reader);
		if (!found) {
			if (strstr(line, name) != NULL)
				found = 1;
			continue;
		}
		if (strstr(line, "SHAPE_VERTEXCOUNT") != NULL) {
			sscanf(line, "%*s%d", count);
			RBB_P(self, 0x04) = RBB_NEW(*count << 4, rbb_AseFile, 0x81);
			total = *count;
			*count = 0;
		}
		if (strstr(line, "SHAPE_VERTEX_KNOT") != NULL) {
			if (RBB_P(self, 0x04) != NULL) {
				float *p;
				sscanf(line, "%*s%*s%f%f%f", &x, &y, &z);
				p = (float *)RBB_P(self, 0x04) + *count * 4;
				p[0] = x;
				p[1] = z;
				p[2] = y;
				*count += 1;
			}
			if (total <= *count)
				return;
		}
	}
}

/* 0x001989a0  RollerBall/CRollerball_ASEImport.cpp (inferred)
Text reader constructor. */
void CRollerBallAseReader_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274b8c;
}

/* 0x001989b0  RollerBall/CRollerball_ASEImport.cpp (inferred)
Text reader destructor. */
void CRollerBallAseReader_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274b8c;
}

/* 0x001989c0  RollerBall/CRollerball_ASEImport.cpp (inferred)
Text reader: read `size` bytes at `text`. Keeps the table prototype (other modules call
it with integers). */
void FUN_001989c0(int self, int text, int size)
{
	RBB_I(self, 0x04) = text;
	RBB_I(self, 0x08) = text;
	RBB_I(self, 0x0c) = text + size;
}

/* 0x001989e0  RollerBall/CRollerball_ASEImport.cpp (inferred)
Text reader scalar deleting destructor. */
void *CRollerBallAseReader_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274b8c;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}
