/*
Rowing/CRowing_ASEImport.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_ASEImport.cpp

Reads the shapes (2D paths: x and the second coordinate of each knot) of a 3D Studio ASCII
export, all of them (RowAseImport_ReadAllShapes) or those of the nodes whose NODE_NAME line
contains one or two names (RowAseImport_ReadShapes), and the small text reader it reads
with (vtable 0x00276728; the line read 0x001b9e00 is used by other minigames too).
The Rowing copy of CarryOn's CFire_ASEImport.cpp (whose paths are 3D).

0x001b9ea0 and 0x001ba160 are tagged with CRowing_ASEImport.cpp; the reader and importer
constructors and destructors just before them are placed here by address (file inferred).
The reader's rewind and skip-line methods are carryon_a's (0x000c3a40, 0x000c3a50); its
open method is rollerball_b's (0x001989c0).
*/

#include "rowing_b.h"

/* 0x00276728  vtable of the text reader */
const void *const vtable_00276728[] = {
	(const void *)RowAseFile_DeletingDtor,      /* [0] +0x00 */
};

/* 0x0027672c  vtable of the importer */
const void *const vtable_0027672c[] = {
	(const void *)RowAseImport_DeletingDtor,    /* [0] +0x00 */
};

/* 0x001b9de0  Rowing (gap; CRowing_ASEImport.cpp inferred) the reader's constructor */
struct RowAseFile *RowAseFile_ctor(struct RowAseFile *self)
{
	self->text.vtable = vtable_00276728;
	return self;
}

/* 0x001b9df0  Rowing (gap; CRowing_ASEImport.cpp inferred) the reader's destructor */
void RowAseFile_dtor(struct RowAseFile *self)
{
	self->text.vtable = vtable_00276728;
}

/* 0x001b9e00  Rowing (gap; CRowing_ASEImport.cpp inferred)
Copies the line at the cursor (up to a line break or the end) into `line` and moves the
cursor one byte past it. (`self` is the reader: struct RowAseText *; other modules call
this with an int.) */
void RowAseText_ReadLine(int self, char *line)
{
	struct RowAseText *text = (struct RowAseText *)self;
	char c;

	if (text->cursor < text->end) {
		do {
			c = *text->cursor;
			if (c == '\n' || c == '\r')
				break;
			*line++ = c;
			text->cursor++;
		} while (text->cursor < text->end);
	}
	*line = '\0';
	text->cursor++;
}

/* 0x001b9e40  Rowing (gap; CRowing_ASEImport.cpp inferred) the reader's scalar deleting
destructor */
struct RowAseFile *RowAseFile_DeletingDtor(struct RowAseFile *self, unsigned int flags)
{
	self->text.vtable = vtable_00276728;
	if (flags & 1)
		RB_DELETE(self);
	return self;
}

/* 0x001b9e60  Rowing (gap; CRowing_ASEImport.cpp inferred) the importer's constructor */
struct RowAseImport *RowAseImport_ctor(struct RowAseImport *self)
{
	self->vtable = vtable_0027672c;
	return self;
}

/* 0x001b9e70  Rowing (gap; CRowing_ASEImport.cpp inferred) the importer's destructor */
void RowAseImport_dtor(struct RowAseImport *self)
{
	self->vtable = vtable_0027672c;
}

/* 0x001b9e80  Rowing (gap; CRowing_ASEImport.cpp inferred) the importer's scalar deleting
destructor */
struct RowAseImport *RowAseImport_DeletingDtor(struct RowAseImport *self, unsigned int flags)
{
	self->vtable = vtable_0027672c;
	if (flags & 1)
		RB_DELETE(self);
	return self;
}

/* the index of the shape a NODE_NAME line names: the number in its name (1-based,
   clamped to the paths), else the one after `index` */
static int rb_ase_node_index(const char *line, int index, int count)
{
	char number[32];
	const char *digit;
	int n;

	sscanf(line, "%*s%*c%*c%s", number);
	for (digit = number; (*digit < '0' || *digit > '9') && *digit != '\0'; digit++)
		;
	if (digit < number + strlen(number)) {
		n = atoi(digit) - 1;
		if (n > count - 1)
			return count - 1;
		return n < 0 ? 0 : n;
	}
	return index + 1;
}

/* the reader's rewind and skip-line (carryon_a 0x000c3a40, 0x000c3a50, thiscall) */
#define rb_ase_rewind(file) RB_CALL(FUN_000c3a40, void (*)(struct RowAseFile *))(file)
#define rb_ase_skip(file) RB_CALL(FUN_000c3a50, void (*)(struct RowAseFile *))(file)

/* the next line: read it, skip the line breaks after it */
static void rb_ase_line(struct RowAseFile *file, char *line)
{
	RowAseText_ReadLine((int)&file->text, line);
	rb_ase_skip(file);
}

/* allocates `count` empty paths */
static struct RowPath *rb_ase_paths(int count, int line)
{
	struct RowPath *paths;
	int i;

	paths = RB_NEW(count * 0xc, RB_FILE_ASE, line, 0x1001);
	for (i = 0; i < count; i++) {
		paths[i].points = NULL;
		paths[i].count = 0;
		paths[i].closed = 0;
	}
	return paths;
}

/* 0x001b9ea0  Rowing/CRowing_ASEImport.cpp
Reads every shape of the file: counts the SHAPE_VERTEXCOUNT lines, allocates that many
paths (*paths, *count), then reads each shape's knots and whether it is closed. A node
name ending in a number puts its shape at that index (1-based, clamped); others take the
next one. (The reader is not rewound before the count.) */
void RowAseImport_ReadAllShapes(struct RowAseImport *self, struct RowAseFile *file,
	struct RowPath **paths, int *count)
{
	char line[0x400];
	struct RowPath *path;
	char haveIndex = 0;
	int index = -1;
	int vertex = 0;

	(void)self;
	while (file->text.cursor < file->text.end) {
		rb_ase_line(file, line);
		if (strstr(line, "SHAPE_VERTEXCOUNT") != NULL)
			(*count)++;
	}
	*paths = rb_ase_paths(*count, 0x48);
	rb_ase_rewind(file);
	while (file->text.cursor < file->text.end) {
		rb_ase_line(file, line);
		if (strstr(line, "NODE_NAME") != NULL) {
			if (!haveIndex) {
				index = rb_ase_node_index(line, index, *count);
				haveIndex = 1;
			}
		} else if (strstr(line, "SHAPE_CLOSED") != NULL) {
			(*paths)[index].closed = 1;
		} else if (strstr(line, "SHAPE_VERTEXCOUNT") != NULL) {
			if (!haveIndex)
				index = (index + 1) % *count;
			path = &(*paths)[index];
			sscanf(line, "%*s%d", &path->count);
			if (path->points != NULL)
				RB_DELETE(path->points);
			path->points = RB_NEW(path->count << 3, RB_FILE_ASE, 0x8b, 0x1001);
			vertex = 0;
			haveIndex = 0;
		} else if (strstr(line, "SHAPE_VERTEX_KNOT") != NULL
			|| strstr(line, "SHAPE_VERTEX_INTERP") != NULL) {
			path = &(*paths)[index];
			if (path->points != NULL) {
				sscanf(line, "%*s%*s%f%f", &path->points[vertex * 2], &path->points[vertex * 2 + 1]);
				vertex++;
			}
		}
	}
}

/* the NODE_NAME line belongs to the wanted nodes: it contains `name` and, when given,
   `name2` */
static int rb_ase_wanted(const char *line, const char *name, const char *name2)
{
	if (strstr(line, name) == NULL)
		return 0;
	return name2 == NULL || strstr(line, name2) != NULL;
}

/* 0x001ba160  Rowing/CRowing_ASEImport.cpp
Reads the shapes of the nodes whose NODE_NAME line contains `name` (and `name2` when
given): counts their SHAPE_VERTEXCOUNT lines, allocates that many paths (*paths, *count),
then reads each shape's knots and whether it is closed. A node name ending in a number
puts its shape at that index (1-based, clamped); others take the next one. Paths that got
no shape are removed. */
void RowAseImport_ReadShapes(struct RowAseImport *self, struct RowAseFile *file,
	struct RowPath **paths, int *count, const char *name, const char *name2)
{
	char line[0x400];
	struct RowPath *list;
	struct RowPath *path;
	char haveIndex = 0;
	char inNode = 0;
	int index;
	int vertex = 0;
	int i, j;

	(void)self;
	rb_ase_rewind(file);
	while (file->text.cursor < file->text.end) {
		rb_ase_line(file, line);
		if (strstr(line, "NODE_NAME") != NULL) {
			inNode = (char)rb_ase_wanted(line, name, name2);
			if (!inNode)
				continue;
		} else if (!inNode) {
			continue;
		}
		if (strstr(line, "SHAPE_VERTEXCOUNT") != NULL)
			(*count)++;
	}
	*paths = rb_ase_paths(*count, 0xd9);
	if (*count == 0)
		return;

	rb_ase_rewind(file);
	index = -1;
	inNode = 0;
	while (file->text.cursor < file->text.end) {
		rb_ase_line(file, line);
		if (strstr(line, "NODE_NAME") != NULL) {
			if (!rb_ase_wanted(line, name, name2)) {
				inNode = 0;
				continue;
			}
			inNode = 1;
			if (!haveIndex) {
				index = rb_ase_node_index(line, index, *count);
				haveIndex = 1;
			}
		} else if (inNode && strstr(line, "SHAPE_VERTEXCOUNT") != NULL) {
			path = &(*paths)[index];
			sscanf(line, "%*s%d", &path->count);
			if (path->points != NULL)
				RB_DELETE(path->points);
			path->points = RB_NEW(path->count << 3, RB_FILE_ASE, 0x12c, 0x1001);
			vertex = 0;
			haveIndex = 0;
		} else if (strstr(line, "SHAPE_CLOSED") != NULL) {
			if (inNode)
				(*paths)[index].closed = 1;
		} else if (inNode && (strstr(line, "SHAPE_VERTEX_KNOT") != NULL
			|| strstr(line, "SHAPE_VERTEX_INTERP") != NULL)) {
			path = &(*paths)[index];
			if (path->points != NULL) {
				sscanf(line, "%*s%*s%f%f", &path->points[vertex * 2], &path->points[vertex * 2 + 1]);
				vertex++;
			}
		}
	}

	/* drop the paths that got no shape */
	for (i = 0; i < *count; i++) {
		list = *paths;
		if (list[i].points != NULL)
			continue;
		for (j = i; j < *count - 1; j++)
			list[j] = list[j + 1];
		i--;
		(*count)--;
	}
}
