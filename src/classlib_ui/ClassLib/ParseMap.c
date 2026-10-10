/*
ClassLib/ParseMap.c  (classlib_ui)
Original: C:\BlitzParty\Game\Src\ClassLib\ParseMap.cpp (tags 0x0007e940-0x0007f2f0).

A tree of named blocks with typed properties, read from text map files:

    # comment
    Camera
    {
        MAX_FRAMES = 300
        PosX = 1.5
        Name = "position"
        Loop = true
    }

A line without '=' names the next block; '{' opens it, '}' closes it. Values in
quotes are strings, true/false booleans, values with '.' floats, others integers.

Structures (accessed by offset):
  map (4 bytes):       root node
  node (0x34 bytes):   0x00 parent, 0x04 previous sibling, 0x08 next sibling,
                       0x0c first child (circular sibling list), 0x10 name[32],
                       0x30 property list head (a property used as the list head)
  property (0x12c):    0x00 next, 0x04 previous, 0x08 name[32],
                       0x28 type (1 string, 2 int, 3 float, 4 bool), 0x2c value
                       (string: inline, up to 256 characters)
  reader (0x118):      0x000 file name, 0x104 buffer, 0x108 read position,
                       0x10c end, 0x110 mode (1 file read into memory, 2 memory,
                       3 file opened for writing), 0x114 FILE *

The property helpers 0x0007e700-0x0007e890 are placed here (inferred: they work on these
structures).
*/

#include "classlib_ui.h"

#define OV_F(o, off) (*(float *)((o) + (off)))
#define OV_I(o, off) (*(int *)((o) + (off)))
#define OV_U(o, off) (*(unsigned int *)((o) + (off)))

/* 0x002ed220  line buffer of ParseMap_StripLine */
#define clu_parse_line CLU_ARR(char, 0x002ed220)

/* ---------- lookups (thiscall in the original) */

/* 0x0007e700  ClassLib/ParseMap.cpp (inferred)
Finds the property `name` (case-insensitive) in a property list; 0 if none. */
int ParseMap_FindProperty(int head, const char *name)
{
	int property;

	for (property = OV_I(head, 4); property != head; property = OV_I(property, 4)) {
		if (_stricmp(name, (char *)(property + 8)) == 0)
			return property;
	}
	return 0;
}

/* 0x0007e740  ClassLib/ParseMap.cpp (inferred)
The string value of property `name` of a node, or NULL. */
char *ParseMap_GetString(int node, const char *name)
{
	int property = ParseMap_FindProperty(OV_I(node, 0x30), name);

	if (property != 0 && OV_I(property, 0x28) == 1)
		return (char *)(property + 0x2c);
	return NULL;
}

/* 0x0007e770  ClassLib/ParseMap.cpp (inferred)
The integer value of property `name`; returns 0 when there is none. */
int ParseMap_GetInt(int node, const char *name, undefined4 *out)
{
	int property = ParseMap_FindProperty(OV_I(node, 0x30), name);

	if (property != 0 && OV_I(property, 0x28) == 2) {
		*out = OV_U(property, 0x2c);
		return 1;
	}
	return 0;
}

/* 0x0007e7a0  ClassLib/ParseMap.cpp (inferred)
The float value of property `name` (copied as 32 bits); returns 0 when there is none. */
int ParseMap_GetFloat(int node, const char *name, undefined4 *out)
{
	int property = ParseMap_FindProperty(OV_I(node, 0x30), name);

	if (property != 0 && OV_I(property, 0x28) == 3) {
		*out = OV_U(property, 0x2c);
		return 1;
	}
	return 0;
}

/* 0x0007e7d0  ClassLib/ParseMap.cpp (inferred)
Calls callback(property, arg) for every property of a node. */
int ParseMap_ForEachProperty(int node, int callback, undefined4 arg)
{
	int property;

	property = OV_I(OV_I(node, 0x30), 4);
	if (property != OV_I(node, 0x30)) {
		do {
			((int (*)(int, undefined4))callback)(property, arg);
			property = OV_I(property, 4);
		} while (property != OV_I(node, 0x30));
	}
	return 0;
}

/* 0x0007e810  ClassLib/ParseMap.cpp (inferred)
Calls callback(node, arg) for the node (when `name` is NULL or matches its name) and,
depth first, for its descendants. A callback result other than 0 from the node stops
the walk there and is returned; 1 from a descendant stops everything. */
int ParseMap_Walk(int node, int callback, undefined4 arg, int name)
{
	int result;
	int child;

	if ((name == 0 || _stricmp((char *)(node + 0x10), (char *)name) == 0) &&
	    (result = ((int (*)(int, undefined4))callback)(node, arg)) != 0)
		return result;
	child = OV_I(node, 0xc);
	if (child != 0) {
		do {
			if (ParseMap_Walk(child, callback, arg, name) == 1)
				return 1;
			child = OV_I(child, 8);
		} while (child != OV_I(node, 0xc));
	}
	return 0;
}

/* 0x0007e880  ClassLib/ParseMap.cpp (inferred)
ParseMap_Walk from the root of a map. */
int ParseMap_WalkMap(int map, int callback, undefined4 arg, int name)
{
	return ParseMap_Walk(*(int *)map, callback, arg, name);
}

/* 0x0007e890  ClassLib/ParseMap.cpp (inferred; the original takes `line` in EDI)
Strips a line in place: leading blanks, blanks outside quotes and "//" comments. */
char *ParseMap_StripLine(char *line)
{
	char *in;
	char *out;
	char c;
	int quoted;

	strcpy(clu_parse_line, line);
	in = clu_parse_line;
	c = *in;
	if (c == '\0') {
		*line = '\0';
		return line;
	}
	while (c == '\t' || c == ' ') {
		c = in[1];
		in++;
		if (c == '\0') {
			*line = '\0';
			return line;
		}
	}
	out = line;
	quoted = 0;
	for (c = *in; c != '\0'; c = *++in) {
		if (quoted) {
			*out++ = c;
			if (c == '"')
				quoted = 0;
		} else if (c == '"') {
			quoted = 1;
			*out++ = c;
		} else if (c == '/') {
			if (in[1] == '/')
				break;
			*out++ = c;
		} else if (c != '\t' && c != ' ') {
			*out++ = c;
		}
	}
	*out = '\0';
	return line;
}

/* ---------- readers */

/* 0x0007e940  ClassLib/ParseMap.cpp
Opens a reader: mode 1 reads file `name` into memory, mode 2 reads `size` bytes at
`name` (a buffer), mode 3 opens file `name` for writing. NULL on failure. */
char *ParseMap_OpenReader(char *name, int mode, int size)
{
	FILE *file;
	char *reader;
	char *buffer;
	int length;

	if (mode == 1) {
		file = fopen(name, "rt");
		if (file == NULL)
			return NULL;
		fseek(file, 0, SEEK_END);
		length = (int)ftell(file);
		fseek(file, 0, SEEK_SET);
		buffer = (char *)CLU_Alloc(length, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", 0x416, 0x2001);
		if (buffer == NULL) {
			fclose(file);
			return NULL;
		}
		length = (int)fread(buffer, 1, length, file);
		fclose(file);
		reader = (char *)CLU_Alloc(0x118, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", 0x41f, 0x2001);
		*(char **)(reader + 0x108) = buffer;
		*(char **)(reader + 0x104) = buffer;
		*(char **)(reader + 0x10c) = buffer + length;
		*(int *)(reader + 0x114) = 0;
		*(int *)(reader + 0x110) = 1;
		strcpy(reader, name);
		return reader;
	}
	if (mode == 2) {
		reader = (char *)CLU_Alloc(0x118, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", 0x42c, 0x2001);
		*(char **)(reader + 0x108) = name;
		*(char **)(reader + 0x104) = name;
		*(char **)(reader + 0x10c) = name + size;
		*(int *)(reader + 0x114) = 0;
		*(int *)(reader + 0x110) = 2;
		*reader = '\0';
		return reader;
	}
	if (mode == 3) {
		file = fopen(name, "wt");
		if (file != NULL) {
			reader = (char *)CLU_Alloc(0x118, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", 0x43d, 0x2001);
			*(FILE **)(reader + 0x114) = file;
			*(int *)(reader + 0x10c) = 0;
			*(int *)(reader + 0x108) = 0;
			*(int *)(reader + 0x104) = 0;
			*(int *)(reader + 0x110) = 3;
			strcpy(reader, name);
			return reader;
		}
	}
	return NULL;
}

/* 0x0007eaf0  ClassLib/ParseMap.cpp
Closes a reader (frees its buffer or closes its file). */
void ParseMap_CloseReader(int reader)
{
	if (reader == 0)
		return;
	if (OV_I(reader, 0x110) == 1) {
		if (OV_I(reader, 0x104) != 0) {
			FUN_0004c110(OV_I(reader, 0x104));
			OV_I(reader, 0x104) = 0;
		}
	} else if (OV_I(reader, 0x110) == 3 && *(FILE **)(reader + 0x114) != NULL) {
		fclose(*(FILE **)(reader + 0x114));
		FUN_0004c110(reader);
		return;
	}
	FUN_0004c110(reader);
}

/* ---------- building the tree */

/* a new property list head (an empty property) */
static int ParseMap_NewPropertyHead(int line)
{
	int head = (int)CLU_Alloc(300, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", line, 0x1001);

	if (head == 0)
		return 0;
	*(char *)(head + 0x2c) = 0;
	OV_I(head, 0x2c) = 0;
	OV_I(head, 0) = head;
	OV_I(head, 4) = head;
	OV_I(head, 0x28) = 0;
	OV_I(head, 0x2c) = 0;
	memcpy((void *)(head + 8), "ROOT", 5);
	return head;
}

/* 0x0007eb50  ClassLib/ParseMap.cpp (thiscall)
Root node constructor (named "ROOT"). */
int *ParseMap_InitRoot(int *node)
{
	node[1] = (int)node;
	node[2] = (int)node;
	node[0] = 0;
	node[3] = 0;
	memcpy(&node[4], "ROOT", 5);
	node[0xc] = ParseMap_NewPropertyHead(0x91);
	return node;
}

/* 0x0007ebd0  ClassLib/ParseMap.cpp (thiscall)
Node constructor: an unnamed child of `parent` (added at the end of its children). */
int *ParseMap_InitNode(int *node, int parent)
{
	node[1] = (int)node;
	node[2] = (int)node;
	node[0] = 0;
	node[3] = 0;
	*(char *)&node[4] = 0;
	node[0xc] = ParseMap_NewPropertyHead(0xa5);
	if (parent != 0) {
		node[0] = parent;
		if (OV_I(parent, 0xc) == 0) {
			OV_I(parent, 0xc) = (int)node;
			return node;
		}
		node[1] = OV_I(OV_I(parent, 0xc), 4);
		node[2] = OV_I(parent, 0xc);
		OV_I(OV_I(OV_I(parent, 0xc), 4), 8) = (int)node;
		OV_I(OV_I(parent, 0xc), 4) = (int)node;
	}
	return node;
}

/* a new property at the front of a node's property list */
static int ParseMap_NewProperty(int node, int line, int type, const char *name)
{
	int property;
	int head;

	property = (int)CLU_Alloc(300, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", line, 0x1001);
	if (property == 0) {
		property = 0;
	} else {
		head = OV_I(node, 0x30);
		OV_I(property, 4) = head;
		OV_I(property, 0) = OV_I(head, 0);
		OV_I(OV_I(head, 0), 4) = property;
		OV_I(head, 0) = property;
		*(char *)(property + 0x2c) = 0;
		OV_I(property, 0x2c) = 0;
		OV_I(property, 0x28) = 0;
		OV_I(property, 0x2c) = 0;
		*(char *)(property + 8) = 0;
	}
	OV_I(property, 0x28) = type;
	strcpy((char *)(property + 8), name);
	return property;
}

/* 0x0007ec70  ClassLib/ParseMap.cpp (thiscall) */
void ParseMap_AddString(int node, char *name, char *value)
{
	int property = ParseMap_NewProperty(node, 0x1e4, 1, name);

	strcpy((char *)(property + 0x2c), value);
}

/* 0x0007ed00  ClassLib/ParseMap.cpp (thiscall) */
void ParseMap_AddInt(int node, char *name, int value)
{
	int property = ParseMap_NewProperty(node, 0x1f7, 2, name);

	OV_I(property, 0x2c) = value;
}

/* 0x0007ed80  ClassLib/ParseMap.cpp (thiscall) */
void ParseMap_AddFloat(int node, char *name, float value)
{
	int property = ParseMap_NewProperty(node, 0x20a, 3, name);

	OV_F(property, 0x2c) = value;
}

/* 0x0007ee00  ClassLib/ParseMap.cpp (thiscall) */
void ParseMap_AddBool(int node, char *name, char value)
{
	int property = ParseMap_NewProperty(node, 0x21d, 4, name);

	*(char *)(property + 0x2c) = value;
}

/* 0x0007ee80  ClassLib/ParseMap.cpp (thiscall)
Map constructor: creates the root node. */
int *ParseMap_InitMap(int *map)
{
	int root;

	root = (int)CLU_Alloc(0x34, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", 0x301, 0x1001);
	if (root != 0) {
		*map = (int)ParseMap_InitRoot((int *)root);
		return map;
	}
	*map = 0;
	return map;
}

/* 0x0007ef00  ClassLib/ParseMap.cpp (thiscall)
Frees every property of a list and the list head. */
void ParseMap_FreeProperties(int *head)
{
	int *property;

	property = (int *)head[1];
	while (property != head) {
		if (property != NULL) {
			OV_I(property[0], 4) = property[1];
			*(int *)property[1] = property[0];
			FUN_0004c110((int)property);
		}
		property = (int *)head[1];
	}
	OV_I(head[0], 4) = head[1];
	*(int *)head[1] = head[0];
	FUN_0004c110((int)head);
}

/* 0x0007ef50  ClassLib/ParseMap.cpp (thiscall)
Node destructor: frees the children, unlinks the node and frees its properties (not
the node itself). */
void ParseMap_FreeNode(int *node)
{
	int child;
	int parent;

	if (node[3] != 0) {
		child = OV_I(node[3], 8);
		while (child != node[3]) {
			if (child != 0) {
				ParseMap_FreeNode((int *)child);
				FUN_0004c110(child);
			}
			child = OV_I(node[3], 8);
		}
		child = node[3];
		if (child != 0) {
			ParseMap_FreeNode((int *)child);
			FUN_0004c110(child);
		}
	}
	OV_I(node[2], 4) = node[1];
	OV_I(node[1], 8) = node[2];
	parent = node[0];
	if (parent != 0 && *(int **)(parent + 0xc) == node)
		OV_I(parent, 0xc) = node[2] & ((node[2] == node[1]) - 1);
	ParseMap_FreeProperties((int *)node[0xc]);
}

/* 0x0007efe0  ClassLib/ParseMap.cpp (thiscall)
Reads lines from the reader into the node until '}' or the end; '{' reads a child
block (named by the line before it). Returns 1. */
int ParseMap_ParseBlock(int node, int reader)
{
	char block_name[32];
	char line[1024];
	char *in;
	char *end;
	char *out;
	char *text;
	char *name_end;
	char *equals;
	char *value;
	char *open_quote;
	char *close_quote;
	int child;

	while (*(unsigned int *)(reader + 0x108) < *(unsigned int *)(reader + 0x10c)) {
		end = *(char **)(reader + 0x10c);
		out = line;
		in = *(char **)(reader + 0x108);
		for (text = in; text < end && *text != '\n' && *text != '\r'; text++)
			*out++ = *text;
		*out = '\0';
		while (in < end) {
			if (*in == '\n' || *in == '\r')
				break;
			in++;
			end = *(char **)(reader + 0x10c);
		}
		if (in[1] == '\n' || in[1] == '\r')
			*(char **)(reader + 0x108) = in + 2;
		else
			*(char **)(reader + 0x108) = in + 1;
		text = ParseMap_StripLine(line);
		if (*text == '#')
			continue;
		if (*text == '{') {
			child = (int)CLU_Alloc(0x34, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", 0x13a, 0x1001);
			if (child != 0)
				child = (int)ParseMap_InitNode((int *)child, node);
			ParseMap_ParseBlock(child, reader);
			strcpy((char *)(child + 0x10), block_name);
			continue;
		}
		if (*text == '}')
			return 1;
		if (strlen(text) == 0)
			continue;
		for (equals = text; *equals != '\0' && *equals != '='; equals++)
			;
		if (*equals == '\0') {
			strcpy(block_name, text);
			continue;
		}
		name_end = equals;
		while (text < name_end && (*name_end == '=' || *name_end == ' ' || *name_end == '\t'))
			name_end--;
		if (name_end < text)
			continue;
		value = equals + 1;
		name_end[1] = '\0';
		open_quote = strchr(value, '"');
		if (open_quote != NULL) {
			close_quote = strchr(open_quote + 1, '"');
			if (close_quote != NULL && 1 < close_quote - open_quote) {
				*close_quote = '\0';
				ParseMap_AddString(node, text, open_quote + 1);
				continue;
			}
		}
		if (_stricmp(value, "true") == 0) {
			ParseMap_AddBool(node, text, 1);
		} else if (_stricmp(value, "false") == 0) {
			ParseMap_AddBool(node, text, 0);
		} else if (strchr(value, '.') != NULL) {
			while ((*value < '0' || '9' < *value) && *value != '.' && *value != '-')
				value++;
			ParseMap_AddFloat(node, text, (float)atof(value));
		} else if ((*value < '0' || '9' < *value) && *value != '-') {
			CLU_DebugPrintf("Error in mapfile\n");
		} else {
			ParseMap_AddInt(node, text, (int)atol(value));
		}
	}
	return 1;
}

/* 0x0007f2f0  ClassLib/ParseMap.cpp
Reads a map from file `name` (from_file == 1) or from the buffer `name` of `size`
bytes. Returns the map, or 0. */
int *ParseMap_Load(char *name, int from_file, int size)
{
	char *reader;
	int *map;

	reader = ParseMap_OpenReader(name, (from_file != 1) + 1, size);
	if (reader == NULL)
		return NULL;
	map = (int *)CLU_Alloc(4, "C:\\BlitzParty\\Game\\Src\\ClassLib\\ParseMap.cpp", 0x3e4, 0x1001);
	if (map != NULL)
		map = ParseMap_InitMap(map);
	ParseMap_ParseBlock(*map, (int)reader);
	ParseMap_CloseReader((int)reader);
	return map;
}

/* 0x0007f3a0  ClassLib (file unknown, after ParseMap.cpp)
Frees a map and its tree. */
void ParseMap_Free(int *map)
{
	int root;

	if (map == NULL)
		return;
	root = *map;
	if (root != 0) {
		ParseMap_FreeNode((int *)root);
		FUN_0004c110(root);
	}
	FUN_0004c110((int)map);
}
