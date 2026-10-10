/*
CarryOn/CFire_ASEImport.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_ASEImport.cpp

Reads the shapes (paths) of named nodes from a 3D Studio ASCII export (variant 4:
paths\fuses.ase, nodes "Fuse", "Paint", "Convey"), and the small text reader it reads
with (vtable 0x00267bb8; rowing_b 0x001989c0 sets one up on a buffer, 0x001b9e00 reads a
line; other minigames use it too).

0x000c3ae0 is tagged with CFire_ASEImport.cpp; the reader methods and the class's
constructor/destructor around it are placed here by address (file inferred).
*/

#include "carryon_a.h"

/* rowing_b: the reader's line read (reads up to the end of the line, skips one byte) */
#define co_TextReader_ReadLine ((void (*)(struct CoTextReader *, char *))FUN_001b9e00)

/* 0x00267bb8  the vtable of the text reader */
const void *const vtable_00267bb8[] = {
	(const void *)CFire_TextReader_DeletingDtor,    /* [0] +0x00 */
};

/* 0x00267bbc  the vtable of CFire_ASEImport */
const void *const vtable_00267bbc[] = {
	(const void *)CFire_ASEImport_DeletingDtor,     /* [0] +0x00 */
};

/* 0x000c3a20  (file inferred) the reader's constructor */
struct CoTextReader *CFire_TextReader_Ctor(struct CoTextReader *self)
{
	self->vtable = vtable_00267bb8;
	return self;
}

/* 0x000c3a30  (file inferred) the reader's destructor */
void CFire_TextReader_Dtor(struct CoTextReader *self)
{
	self->vtable = vtable_00267bb8;
}

/* 0x000c3a40  (file inferred) back to the start of the buffer */
void CFire_TextReader_Rewind(struct CoTextReader *self)
{
	self->cursor = self->start;
}

/* 0x000c3a50  (file inferred)
Skips to the end of the line, then over the line breaks. */
void CFire_TextReader_SkipLine(struct CoTextReader *self)
{
	char *end = self->end;

	if (self->cursor < end)
	{
		while (*self->cursor != '\n' && *self->cursor != '\r')
		{
			self->cursor++;
			if (self->cursor >= self->end)
				break;
		}
		if (!(self->cursor < end))
			return;
		while (*self->cursor == '\n' || *self->cursor == '\r')
		{
			self->cursor++;
			if (self->cursor >= self->end)
				return;
		}
	}
}

/* 0x000c3aa0  (file inferred) the reader's scalar deleting destructor */
struct CoTextReader *CFire_TextReader_DeletingDtor(struct CoTextReader *self, unsigned int flags)
{
	self->vtable = vtable_00267bb8;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000c3ac0  (file inferred) the constructor */
struct CFireASEImport *CFire_ASEImport_Ctor(struct CFireASEImport *self)
{
	self->vtable = vtable_00267bbc;
	return self;
}

/* 0x000c3ad0  (file inferred) the destructor */
void CFire_ASEImport_Dtor(struct CFireASEImport *self)
{
	self->vtable = vtable_00267bbc;
}

/* 0x000c3ae0  CarryOn/CFire_ASEImport.cpp
Reads the shapes of the nodes whose NODE_NAME line contains `name`: counts their
SHAPE_VERTEXCOUNT lines, allocates that many paths (*paths, *count), then reads each
shape's vertices (SHAPE_VERTEX_KNOT / SHAPE_VERTEX_INTERP, x z y order into x y z) and
whether it is closed. A node name ending in a number puts its shape at that index
(1-based, clamped); others take the next one. Paths that got no shape are removed. */
void CFire_ASEImport_ReadPaths(struct CFireASEImport *self, struct CoTextReader *reader,
	void **paths, int *count, const char *name)
{
	char line[0x400];
	char number[32];
	struct CoAsePath *list;
	float *point;
	char *digit;
	char inNode = 0;
	char haveIndex = 0;
	int index = -1;
	int vertex = 0;
	int n;
	int i, j;

	(void)self;
	CFire_TextReader_Rewind(reader);
	while (reader->cursor < reader->end)
	{
		co_TextReader_ReadLine(reader, line);
		CFire_TextReader_SkipLine(reader);
		if (strstr(line, "NODE_NAME") != NULL)
		{
			if (strstr(line, name) != NULL)
				inNode = 1;
			else
			{
				inNode = 0;
				continue;
			}
		}
		else if (!inNode)
			continue;
		if (strstr(line, "SHAPE_VERTEXCOUNT") != NULL)
			(*count)++;
	}
	list = bkHeapAlloc(*count * 0xc, CO_FILE_ASE, 0xc2, 0x1001);
	*paths = list;
	for (i = 0; i < *count; i++)
	{
		((struct CoAsePath *)*paths)[i].points = NULL;
		((struct CoAsePath *)*paths)[i].numPoints = 0;
		((struct CoAsePath *)*paths)[i].closed = 0;
	}
	if (*count == 0)
		return;

	CFire_TextReader_Rewind(reader);
	inNode = 0;
	while (reader->cursor < reader->end)
	{
		list = *paths;
		co_TextReader_ReadLine(reader, line);
		CFire_TextReader_SkipLine(reader);
		if (strstr(line, "NODE_NAME") != NULL)
		{
			if (strstr(line, name) == NULL)
			{
				inNode = 0;
				continue;
			}
			inNode = 1;
			if (haveIndex)
				continue;
			sscanf(line, "%*s%*c%*c%s", number);
			for (digit = number; (*digit < '0' || *digit > '9') && *digit != '\0'; digit++)
				;
			if (digit < number + strlen(number))
			{
				n = atol(digit) - 1;
				if (n > *count - 1)
					index = *count - 1;
				else
					index = n < 0 ? 0 : n;
			}
			else
				index++;
			haveIndex = 1;
			continue;
		}
		if (inNode && strstr(line, "SHAPE_VERTEXCOUNT") != NULL)
		{
			sscanf(line, "%*s%d", &list[index].numPoints);
			if (list[index].points != NULL)
				bkHeapFree(list[index].points);
			list[index].points = bkHeapAlloc(list[index].numPoints << 4, CO_FILE_ASE, 0x10a, 0x2001);
			vertex = 0;
			haveIndex = 0;
			continue;
		}
		if (strstr(line, "SHAPE_CLOSED") != NULL)
		{
			if (inNode)
				list[index].closed = 1;
			continue;
		}
		if (!inNode)
			continue;
		if ((strstr(line, "SHAPE_VERTEX_KNOT") != NULL || strstr(line, "SHAPE_VERTEX_INTERP") != NULL)
				&& list[index].points != NULL)
		{
			point = list[index].points + vertex * 4;
			sscanf(line, "%*s%*s%f%f%f", &point[0], &point[2], &point[1]);
			vertex++;
		}
	}

	/* drop the paths that got no shape */
	for (i = 0; i < *count; i++)
	{
		list = *paths;
		if (list[i].points != NULL)
			continue;
		for (j = i; j < *count - 1; j++)
			list[j] = list[j + 1];
		i--;
		(*count)--;
	}
}

/* 0x000c3eb0  (file inferred)
Frees `count` paths read by CFire_ASEImport_ReadPaths (last first) and the list. */
void CFire_ASEImport_FreePaths(struct CFireASEImport *self, void *paths, int count)
{
	struct CoAsePath *path;

	(void)self;
	if (count != 0)
	{
		path = (struct CoAsePath *)paths + count;
		do
		{
			path--;
			if (path->points != NULL)
			{
				bkHeapFree(path->points);
				path->points = NULL;
			}
		} while (--count != 0);
	}
	bkHeapFree(paths);
}

/* 0x000c3f00  CarryOn/CFire_ASEImport.cpp (file inferred) the scalar deleting destructor */
struct CFireASEImport *CFire_ASEImport_DeletingDtor(struct CFireASEImport *self, unsigned int flags)
{
	self->vtable = vtable_00267bbc;
	if (flags & 1)
		bkHeapFree(self);
	return self;
}
