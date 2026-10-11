/*
Demolition/CDemolition_ASEImport.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_ASEImport.cpp

Reads the outline of a named shape from a 3D Studio ASE text file (SHAPE_VERTEXCOUNT and
SHAPE_VERTEX_KNOT lines). The variations (demolition_a 0x000e9cd0, 0x000e9f70, 0x000ea350)
build a DcASEFile reader and a DcASEShape on the stack, read a shape, and give its points to a
DcShape (unplaced.c).

0x00102440 is tagged with CDemolition_ASEImport.cpp. The reader's vtable 0x0026b9e8 lies right
after that path string, so its constructor, destructor and deleting destructor (0x001025a0,
0x001025b0, 0x001025c0; the table puts 0x001025c0 in CDemolition_Particle.cpp) are here too
(file inferred), with the shape's (0x001023f0-0x00102420).
*/

#include "demolition_c_internal.h"

extern const void *const vtable_0026b9a8[1];
extern const void *const vtable_0026b9e8[1];

/* 0x001023f0  Demolition (file inferred: CDemolition_ASEImport.cpp)
Constructor: no points. */
struct DcASEShape *DcASEShape_Construct(struct DcASEShape *self)
{
	self->vtable = vtable_0026b9a8;
	self->points = NULL;
	self->count = 0;
	return self;
}

/* 0x00102410  Demolition (file inferred: CDemolition_ASEImport.cpp)
Destructor (vtable only: the points are not freed). */
void DcASEShape_Destruct(struct DcASEShape *self)
{
	self->vtable = vtable_0026b9a8;
}

/* 0x00102420  Demolition (file inferred: CDemolition_ASEImport.cpp)
Deleting destructor (vtable slot 0). */
struct DcASEShape *DcASEShape_DeletingDtor(struct DcASEShape *self, unsigned char flags)
{
	self->vtable = vtable_0026b9a8;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00102440  Demolition/CDemolition_ASEImport.cpp
Reads the shape `name` from the file: after the line holding the name, SHAPE_VERTEXCOUNT
allocates the points, and each SHAPE_VERTEX_KNOT "x y z" line adds the point (x, z, y)
(3ds max is z up). Stops after the last point or at the end of the file. */
void DcASEShape_Read(struct DcASEShape *self, struct DcASEFile *file, const char *name)
{
	char line[1024];
	float x;
	float y;
	float z;
	int found;
	int total;

	self->count = 0;
	found = 0;
	total = 0; /* the original leaves this unset until SHAPE_VERTEXCOUNT */
	while (file->pos < file->end)
	{
		FUN_001b9e00((int)file, line);
		FUN_000c3a50((struct CoTextReader *)file);
		if (found == 0)
		{
			if (strstr(line, name) != NULL)
				found = 1;
			continue;
		}
		if (strstr(line, "SHAPE_VERTEXCOUNT") != NULL)
		{
			sscanf(line, "%*s%d", &self->count);
			self->points = bkHeapAlloc(self->count << 4, s_DcASEFile, 0x81, 0x1001);
			total = self->count;
			self->count = 0;
		}
		if (strstr(line, "SHAPE_VERTEX_KNOT") != NULL)
		{
			if (self->points != NULL)
			{
				sscanf(line, "%*s%*s%f%f%f", &x, &y, &z);
				self->points[self->count][0] = x;
				self->points[self->count][1] = z;
				self->points[self->count][2] = y;
				self->count = self->count + 1;
			}
			if (total <= self->count)
				return;
		}
	}
}

/* 0x001025a0  Demolition (file inferred: CDemolition_ASEImport.cpp)
The reader's constructor (vtable only). */
struct DcASEFile *DcASEFile_Construct(struct DcASEFile *self)
{
	self->vtable = vtable_0026b9e8;
	return self;
}

/* 0x001025b0  Demolition (file inferred: CDemolition_ASEImport.cpp)
The reader's destructor (vtable only). */
void DcASEFile_Destruct(struct DcASEFile *self)
{
	self->vtable = vtable_0026b9e8;
}

/* 0x001025c0  Demolition (file inferred: CDemolition_ASEImport.cpp; the table says
CDemolition_Particle.cpp)
The reader's deleting destructor (vtable slot 0). */
struct DcASEFile *DcASEFile_DeletingDtor(struct DcASEFile *self, unsigned char flags)
{
	self->vtable = vtable_0026b9e8;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x0026b9a8 */
const void *const vtable_0026b9a8[1] = { (const void *)DcASEShape_DeletingDtor };
/* 0x0026b9e8 */
const void *const vtable_0026b9e8[1] = { (const void *)DcASEFile_DeletingDtor };
