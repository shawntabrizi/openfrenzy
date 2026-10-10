/*
PUZZLES.C  <-  C:\BlitzParty\Game\Src\Meta\puzzles.cpp

The board's puzzle scripts (from the level's .lnd file): a puzzle has a condition, a
boolean expression over keywords (VAR_EQUAL 3 1, CHAR_IN_BOX 0 x y z x y z, ...), and
a list of actions (debris, platforms, variables, bombs, sounds, ...) that run when the
condition becomes true.

Puzzle (CLList node in DAT_003075b0):
  +0x00 list links   +0x08 condition tree root   +0x0c CLList of actions
  +0x18 short        +0x1c int (fired)
Condition node (0x34 bytes, bump-allocated through DAT_003075a8):
  +0x00 op (0 leaf, 1 and, 2 or, 3 xor, 4 not)
  leaf: +0x04 keyword, +0x08 short argument, +0x0c float value, +0x10 int/byte,
        +0x14.. shape (box: two corners; sphere: centre + radius at +0x24; cylinder)
  operators: +0x04 first operand, +0x08 second operand
Action (0x54 bytes, zero-filled): +0x08 kind, +0x0c float timer, +0x10 float speed,
  +0x14 short target, +0x18 value/hash/type, +0x1c vector, +0x2c vector or shorts
  (+0x2c, +0x2e, +0x30, +0x32, +0x34, +0x36), +0x3c, +0x40, +0x44, +0x48 floats,
  +0x4c flags, +0x50.
*/

#include "meta_a.h"

static const char ma_file_puzzles[] = "C:\\BlitzParty\\Game\\Src\\Meta\\puzzles.cpp";

/* bkernel FUN_00051540: squared distance of two points (bkernel's bmNothing2 is wrong:
   the original returns ((a - b) . (a - b)) in ST0) */
#define MA_DISTANCE_SQ(a, b) (((float (*)(const float *, const float *))FUN_00051540)((a), (b)))

/* bumpercars_a Debris_AddRing (type, count, pos, out speed, up speed, jitter, frames,
   colour); its out and up speeds are still undefined4 in the function table */
static void ma_debris_ring(void *type, int count, float *pos, float out_speed, float up_speed,
	float jitter, int frames, unsigned int colour)
{
	MA_DUAL(FUN_00081880,
		void (*)(undefined4, int, undefined4, undefined4, undefined4, float, undefined4, undefined4),
		void (*)(void *, int, float *, float, float, float, int, unsigned int),
		((undefined4)type, count, (undefined4)pos, ma_fbits(out_speed), ma_fbits(up_speed), jitter,
			(undefined4)frames, colour),
		(type, count, pos, out_speed, up_speed, jitter, frames, colour));
}

/* the entity list of charlib (DAT_002ea488 points at an object: +0x18 the entities with
   a tag at +0x3d0, +0x24 a second list) */
extern int *DAT_002ea488;
#define MA_ENTITIES ((unsigned int *)((char *)DAT_002ea488 + 0x18))
#define MA_ENTITIES_B ((unsigned int *)((char *)DAT_002ea488 + 0x24))
#define MA_NEXT(list, node) CLList_Next((unsigned int *)(list), (unsigned int *)(node))

/* 0x002b2398  the condition keywords (meta_b FUN_001667e0 looks words up in it; "***"
   ends the table) */
const char *const PTR_s_VAR_EQUAL_002b2398[35] = {
	"VAR_EQUAL", "VAR_NOT_EQUAL", "VAR_GREATER", "VAR_GREATER_OR_EQUAL", "VAR_LESS",
	"VAR_LESS_OR_EQUAL", "CHAR_IN_BOX", "CHAR_IN_SPHERE", "CHAR_IN_CYLINDER", "CAM_IN_BOX",
	"CAM_IN_SPHERE", "CAM_IN_CYLINDER", "COLLECTED", "CHAR_ON_PLATFORM",
	"CHAR_JUST_ON_PLATFORM", "PLATFORM_AT_POINT", "PLATFORM_JUST_AT_POINT", "SOMETIMES",
	"ALWAYS", "NEVER", "BRB_DOWN", "BRB_UP", "BRB_JUST_DOWN", "BRB_JUST_UP", "REPEAT",
	"PLATFORM_DESTROYED", "AFTER_TIME", "BOMB_DESTROYED", "WATER_AT_LEVEL", "FIRST_HALF",
	"SECOND_HALF", "START_OF_FIRST_HALF", "START_OF_SECOND_HALF", "ANIM_ENDED", "***",
};

/* 0x00153080  Meta/puzzles.cpp: add an action to a puzzle */
int FUN_00153080(int puzzle)
{
	int action;

	action = (int)bkHeapCalloc(0x54, 0, ma_file_puzzles, 0x53e, 0x2001);
	if (action != 0)
		CLList_Insert((int *)(puzzle + 0xc), (int *)action, 1);
	return action;
}

/* 0x001530c0  Meta/puzzles.cpp: parse a condition leaf ("KEYWORD arguments") */
void FUN_001530c0(int node, int text)
{
	char word[256];
	short number[2];
	int value[5];
	const char *rest;
	int kind;

	sscanf((const char *)text, "%s", word);
	rest = (const char *)text + strlen(word);
	kind = FUN_001667e0((byte *)word, (undefined4 *)PTR_s_VAR_EQUAL_002b2398);
	MA_I32(node, 4) = kind;
	switch (kind) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		sscanf(rest, "%d %f", number, MA_FP(node, 0xc));
		MA_I16(node, 8) = number[0];
		return;
	case 6:
		sscanf(rest, "%s %f %f %f %f %f %f", word, MA_FP(node, 0x14), MA_FP(node, 0x18),
			MA_FP(node, 0x1c), MA_FP(node, 0x24), MA_FP(node, 0x28), MA_FP(node, 0x2c));
		MA_U8(node, 0x10) = 1;
		return;
	case 7:
		sscanf(rest, "%s %f %f %f %f", word, MA_FP(node, 0x14), MA_FP(node, 0x18),
			MA_FP(node, 0x1c), MA_FP(node, 0x24));
		MA_U8(node, 0x10) = 2;
		return;
	case 8:
		sscanf(rest, "%s %f %f %f %f %f %f", word, MA_FP(node, 0x14), MA_FP(node, 0x18),
			MA_FP(node, 0x1c), MA_FP(node, 0x24), MA_FP(node, 0x28));
		MA_U8(node, 0x10) = 3;
		return;
	case 9:
		sscanf(rest, "%f %f %f %f %f %f", MA_FP(node, 0x14), MA_FP(node, 0x18),
			MA_FP(node, 0x1c), MA_FP(node, 0x24), MA_FP(node, 0x28), MA_FP(node, 0x2c));
		MA_U8(node, 0x10) = 1;
		return;
	case 10:
		sscanf(rest, "%f %f %f %f", MA_FP(node, 0x14), MA_FP(node, 0x18), MA_FP(node, 0x1c),
			MA_FP(node, 0x24));
		MA_U8(node, 0x10) = 2;
		return;
	case 0xb:
		sscanf(rest, "%f %f %f %f %f %f", MA_FP(node, 0x14), MA_FP(node, 0x18),
			MA_FP(node, 0x1c), MA_FP(node, 0x24), MA_FP(node, 0x28));
		MA_U8(node, 0x10) = 3;
		return;
	case 0xc:
	case 0x19:
	case 0x1b:
		sscanf(rest, "%d", number);
		MA_I16(node, 8) = number[0];
		return;
	case 0xd:
	case 0xe:
		sscanf(rest, "%i %i", MA_ADDR(node, 8), MA_ADDR(node, 0x10));
		return;
	case 0xf:
	case 0x10:
		sscanf(rest, "%d %d", number, value);
		MA_F32(node, 0xc) = (float)value[0];
		MA_I16(node, 8) = number[0];
		return;
	case 0x11:
	case 0x1a:
		sscanf(rest, "%f", MA_FP(node, 0xc));
		break;
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		sscanf(rest, "%d %d", number, value);
		MA_I32(node, 0x10) = value[0];
		MA_I16(node, 8) = number[0];
		return;
	case 0x1c:
		sscanf(rest, "%d %f", number, MA_FP(node, 0xc));
		MA_I16(node, 8) = number[0];
		return;
	case 0x21:
		sscanf(rest, "%d", number);
		MA_I16(node, 8) = number[0];
		return;
	}
}

/* 0x001533f0  Meta/puzzles.cpp: turn the infix condition text into prefix form in
   `out`: every operand is followed by '*', an open bracket becomes '@', a close bracket
   '#', and each operator is written at the '@' of its bracket (or in front of all).
   Returns the number of tokens (operands and operators), 0 for the text "0". */
int FUN_001533f0(int out, char *text)
{
	char *buffer = (char *)out;
	char *copy;
	char *token;
	char *end;
	int length;
	int count;
	int read;
	int written;
	int op;
	int i;
	char c;

	written = 0;
	op = -1;
	read = 0;
	count = 0;
	if (*text == '0')
		return 0;
	copy = (char *)MA_NEW(strlen(text) + 1, ma_file_puzzles, 0x64b, 0x2001);
	strcpy(copy, text);
	if (text[0] == '\0')
		goto done;
	do {
		c = text[read];
		if (c == '\n' || c == '\r')
			break;
		switch (c) {
		case '(':
			buffer[written] = '@';
			op = written;
			written++;
			read++;
			break;
		case '!':
		case '&':
		case '^':
		case '|':
			count++;
			if (op == -1) {
				/* no open bracket: make room for the operator at the front */
				for (i = written - 1; i >= 0; i--)
					buffer[i + 1] = buffer[i];
				op = 0;
				written++;
			}
			buffer[op] = c;
			while (op >= 0 && buffer[op] != '@')
				op--;
			read++;
			break;
		case ')':
			if (op != -1) {
				buffer[op] = '#';
				while (op >= 0 && buffer[op] != '@')
					op--;
			}
			read++;
			break;
		case '\t':
		case ' ':
		case '@':
			read++;
			break;
		default:
			/* an operand: copy it and a '*' */
			count++;
			token = strtok(text + read, "()!^&|\r\n");
			end = buffer + written;
			while (*end != '\0')
				end++;
			memcpy(end, token, strlen(token) + 1);
			buffer[written] = text[read];
			length = (int)strlen(token);
			written = written + length;
			buffer[written] = '*';
			written++;
			read = read + length;
			text[read] = copy[read]; /* undo strtok's terminator */
			break;
		}
	} while (text[read] != '\0');
done:
	buffer[written] = '\0';
	MA_DELETE(copy);
	return count;
}

/* 0x001536a0  Meta/puzzles.cpp: build the condition tree from the prefix text */
void FUN_001536a0(undefined4 *node, int *cursor)
{
	char *token;
	char c;

	for (;;) {
		c = *(char *)*cursor;
		while (c == ' ' || c == '\t' || c == '@' || c == '#') {
			*cursor = *cursor + 1;
			c = *(char *)*cursor;
		}
		switch (c) {
		case '!':
			node[0] = 4;
			DAT_003075a8 = DAT_003075a8 + 0x34;
			node[1] = DAT_003075a8;
			*cursor = *cursor + 1;
			node = (undefined4 *)node[1];
			continue;
		case '&':
			node[0] = 1;
			break;
		case '^':
			node[0] = 3;
			break;
		case '|':
			node[0] = 2;
			break;
		default:
			node[0] = 0;
			token = strtok((char *)*cursor, "*");
			*cursor = *cursor + (int)strlen(token);
			FUN_001530c0((int)node, (int)token);
			return;
		}
		/* a binary operator: the first operand, then continue with the second */
		DAT_003075a8 = DAT_003075a8 + 0x34;
		node[1] = DAT_003075a8;
		*cursor = *cursor + 1;
		FUN_001536a0((undefined4 *)node[1], cursor);
		DAT_003075a8 = DAT_003075a8 + 0x34;
		node[2] = DAT_003075a8;
		*cursor = *cursor + 1;
		node = (undefined4 *)node[2];
	}
}

/* the platform an entity of list A with tag `tag`; NULL if none */
static int ma_find_entity(unsigned int *list, int tag)
{
	unsigned int node;

	for (node = MA_NEXT(list, NULL); node != 0; node = MA_NEXT(list, node)) {
		if (MA_I32(node, 0x3d0) == tag)
			return (int)node;
	}
	return 0;
}

/* the point `index` of a platform's path (+0x3dc: 8 bytes per point, at +2) */
static float *ma_path_point(int platform, float index)
{
	return (float *)(MA_I32(platform, 0x3dc) + 2 + (int)index * 8);
}

/* 0x00153850  Meta/puzzles.cpp: evaluate a condition leaf */
uint FUN_00153850(int node)
{
	int value;
	int ground;
	int platform;
	int previous;
	int button;
	int object;
	int i;

	switch (MA_I32(node, 4)) {
	case 0: /* VAR_EQUAL */
		value = (int)MA_F32(node, 0xc);
		return DAT_00305660[MA_I16(node, 8)] == value;
	case 1: /* VAR_NOT_EQUAL */
		value = (int)MA_F32(node, 0xc);
		return DAT_00305660[MA_I16(node, 8)] != value;
	case 2: /* VAR_GREATER */
		value = (int)MA_F32(node, 0xc);
		return value < DAT_00305660[MA_I16(node, 8)];
	case 3: /* VAR_GREATER_OR_EQUAL */
		value = (int)MA_F32(node, 0xc);
		return value <= DAT_00305660[MA_I16(node, 8)];
	case 4: /* VAR_LESS */
		value = (int)MA_F32(node, 0xc);
		return DAT_00305660[MA_I16(node, 8)] < value;
	case 5: /* VAR_LESS_OR_EQUAL */
		value = (int)MA_F32(node, 0xc);
		return DAT_00305660[MA_I16(node, 8)] <= value;
	case 6: /* CHAR_IN_BOX */
		return (uint)CLBox_ContainsPoint(MA_FP(node, 0x14),
			MA_FP(MA_CHAR(DAT_003075bc[MA_I16(node, 8)]), 0xd0));
	case 7: /* CHAR_IN_SPHERE */
		return MA_DISTANCE_SQ(MA_FP(node, 0x14), MA_FP(MA_CHAR(DAT_003075bc[MA_I16(node, 8)]), 0xd0)) <=
			MA_F32(node, 0x24) * MA_F32(node, 0x24) ? 1 : 0;
	case 8: /* CHAR_IN_CYLINDER */
		return (uint)CLCylinder_ContainsPoint(MA_FP(node, 0x14),
			MA_FP(MA_CHAR(DAT_003075bc[MA_I16(node, 8)]), 0xd0));
	case 0xd: /* CHAR_ON_PLATFORM */
		if (MA_I16(node, 8) != -1) {
			platform = MA_I32(MA_PTR(MA_CHAR(DAT_003075bc[MA_I16(node, 8)]), 0x18c), 0x1b0);
			if (platform == 0)
				return 0;
			return MA_I32(platform, 0x3d0) == MA_I32(node, 0x10);
		}
		for (i = 0; i < 4; i++) {
			platform = MA_I32(MA_PTR(MA_CHAR(DAT_003075bc[i]), 0x18c), 0x1b0);
			if (platform != 0 && MA_I32(platform, 0x3d0) == MA_I32(node, 0x10))
				return 1;
		}
		return 0;
	case 0xe: /* CHAR_JUST_ON_PLATFORM */
		if (MA_I16(node, 8) != -1) {
			ground = MA_I32(MA_CHAR(DAT_003075bc[MA_I16(node, 8)]), 0x18c);
			platform = MA_I32(ground, 0x1b0);
			if (platform == 0)
				return 0;
			previous = MA_I32(ground, 0x1b4);
			if (previous != 0 && MA_I32(previous, 0x3d0) == MA_I32(node, 0x10))
				return 0;
			return MA_I32(platform, 0x3d0) == MA_I32(node, 0x10);
		}
		for (i = 0; i < 4; i++) {
			ground = MA_I32(MA_CHAR(DAT_003075bc[i]), 0x18c);
			platform = MA_I32(ground, 0x1b0);
			if (platform != 0 && MA_I32(platform, 0x3d0) == MA_I32(node, 0x10)) {
				previous = MA_I32(ground, 0x1b4);
				if (previous == 0 || MA_I32(previous, 0x3d0) != MA_I32(node, 0x10))
					return 1;
			}
		}
		return 0;
	case 0xf: /* PLATFORM_AT_POINT */
		if (MA_NEXT(MA_ENTITIES, NULL) == 0)
			return 1;
		platform = ma_find_entity(MA_ENTITIES, MA_I16(node, 8));
		if (platform == 0)
			return 1;
		return MA_DISTANCE_SQ(ma_path_point(platform, MA_F32(node, 0xc)), MA_FP(platform, 0xe0)) == 0.0f;
	case 0x10: /* PLATFORM_JUST_AT_POINT */
		if (MA_NEXT(MA_ENTITIES, NULL) == 0)
			return 1;
		platform = ma_find_entity(MA_ENTITIES, MA_I16(node, 8));
		if (platform == 0)
			return 1;
		if (MA_DISTANCE_SQ(ma_path_point(platform, MA_F32(node, 0xc)), MA_FP(platform, 0xe0)) != 0.0f)
			return 0;
		return MA_DISTANCE_SQ(ma_path_point(platform, MA_F32(node, 0xc)), MA_FP(platform, 0x1b4)) != 0.0f;
	case 0x11: /* SOMETIMES: a chance per frame, in 1/10000 */
		return (float)((int)Core_Rand() % 10000) < MA_F32(node, 0xc);
	case 0x13: /* NEVER */
		return 0;
	case 0x14: /* BRB_DOWN */
	case 0x15: /* BRB_UP */
	case 0x16: /* BRB_JUST_DOWN */
	case 0x17: /* BRB_JUST_UP */
		i = FUN_00161c20(MA_I16(node, 8));
		if (i == -1)
			return 0;
		button = DAT_00309060[i];
		if (MA_I32(node, 0x10) != 0 && MA_I32(button, 0x38) != MA_I32(node, 0x10))
			return 0;
		switch (MA_I32(node, 4)) {
		case 0x14:
			return MA_I32(button, 0x3c) == 1;
		case 0x15:
			return MA_I32(button, 0x3c) == 0;
		case 0x16:
			return MA_I32(button, 0x3c) == 1 && MA_I32(button, 0x40) != 1;
		default:
			return MA_I32(button, 0x3c) == 0 && MA_I32(button, 0x40) != 0;
		}
	case 0x18: /* REPEAT */
		return MA_I32(DAT_003075a4, 0x1c) == 0;
	case 0x19: /* PLATFORM_DESTROYED */
		if (ma_find_entity(MA_ENTITIES, MA_I16(node, 8)) != 0)
			return 0;
		object = ma_find_entity(MA_ENTITIES_B, MA_I16(node, 8));
		if (object == 0)
			return 1;
		return MA_U8(object, 0x450) >> 7;
	case 0x1a: /* AFTER_TIME */
		return MA_F32(node, 0xc) < MA_F32(DAT_0030564c, 100);
	case 0x1b: /* BOMB_DESTROYED */
		return CBomb_Find(MA_I16(node, 8)) == NULL;
	case 0x1c: /* WATER_AT_LEVEL */
		object = (int)FUN_00162ee0(MA_I16(node, 8));
		if (object == 0)
			return 0;
		return MA_F32(object, 0x14) == MA_F32(node, 0xc);
	case 0x1d: /* FIRST_HALF */
		return MA_I32(DAT_0030564c, 0x90) == 0;
	case 0x1e: /* SECOND_HALF */
		return MA_U32(DAT_0030564c, 0x90);
	case 0x1f: /* START_OF_FIRST_HALF */
		MA_I16(DAT_003075a4, 0x18) = 1;
		return MA_I32(DAT_0030564c, 0x8c) == 1;
	case 0x20: /* START_OF_SECOND_HALF */
		MA_I16(DAT_003075a4, 0x18) = 1;
		return MA_I32(DAT_0030564c, 0x8c) == 2;
	case 0x21: /* ANIM_ENDED */
		object = FUN_000b83f0(MA_I16(node, 8));
		if (object == 0 || MA_I16(object, 0x244) == 0)
			return 0;
		return 1;
	default: /* CAM_IN_*, COLLECTED, ALWAYS */
		return 1;
	}
}

/* 0x00153f60  Meta/puzzles.cpp: detach an entity from the actor node it was attached to */
void FUN_00153f60(int entity)
{
	if (MA_I32(entity, 0x460) != 0) {
		MA_I32(entity, 0x330) = 0;
		MA_I32(entity, 0x464) = 0;
		MA_I32(entity, 0x460) = 0;
		MA_I32(entity, 0xe0) = MA_I32(entity, 0x50);
		MA_I32(entity, 0xe4) = MA_I32(entity, 0x54);
		MA_I32(entity, 0xe8) = MA_I32(entity, 0x58);
		MA_I32(entity, 0xec) = MA_I32(entity, 0x5c);
		bmMatrixToQuaternion(MA_FP(entity, 0x100), MA_FP(entity, 0x20));
		MA_U32(entity, 0x230) = MA_U32(entity, 0x230) | 2;
	}
}

/* 0x00153fd0  Meta/puzzles.cpp: action kinds 5 and 6: spawn a charlib object */
void FUN_00153fd0(int action)
{
	void *memory;

	switch (MA_I16(action, 0x14)) {
	case 0:
		FUN_000ba1a0((undefined4)(int)MA_F32(action, 0x18), MA_FP(action, 0x2c), MA_F32(action, 0x3c),
			(int)MA_F32(action, 0x40));
		break;
	case 1:
		FUN_000ba2a0((undefined4)(int)MA_F32(action, 0x18), MA_FP(action, 0x2c), MA_F32(action, 0x3c),
			(int)MA_F32(action, 0x40), (int)MA_F32(action, 0x44));
		break;
	case 2:
		memory = MA_NEW(0x250, ma_file_puzzles, 0x595, 0x1001);
		if (memory != NULL)
			MA_DUAL(FUN_000b9d10, int (*)(int, int, undefined4 *, undefined4),
				int (*)(void *, int, float *, float),
				((int)memory, (int)MA_F32(action, 0x18), (undefined4 *)MA_FP(action, 0x2c), ma_fbits(1.0f)),
				(memory, (int)MA_F32(action, 0x18), MA_FP(action, 0x2c), 1.0f));
		break;
	}
}

/* 0x001540d0  Meta/puzzles.cpp: parse the condition text into a tree at *root */
void FUN_001540d0(undefined4 *root, char *text)
{
	int buffer;
	int count;
	int cursor;

	if (strlen(text) == 0)
		return;
	buffer = (int)bkHeapCalloc(strlen(text) * 2, 0, ma_file_puzzles, 0x5aa, 0x2001);
	cursor = buffer;
	count = FUN_001533f0(buffer, text);
	if (count != 0) {
		DAT_003075a8 = (int)bkHeapCalloc(count * 0x34, 0, ma_file_puzzles, 0x5b1, 0x2001);
		*root = DAT_003075a8;
		FUN_001536a0((undefined4 *)*root, &cursor);
	}
	MA_DELETE(buffer);
}

/* 0x00154170  Meta/puzzles.cpp: puzzle constructor (condition text) */
int FUN_00154170(int puzzle, undefined4 text)
{
	CLList_Init((int *)(puzzle + 0xc));
	MA_U16(puzzle, 0x18) = 0xffff;
	MA_I32(puzzle, 8) = 0;
	MA_I32(puzzle, 0x1c) = 0;
	FUN_001540d0((undefined4 *)(puzzle + 8), (char *)text);
	CLList_Insert((int *)DAT_003075b0, (int *)puzzle, 0);
	return puzzle;
}

/* 0x001541f0  Meta/puzzles.cpp: delete a puzzle; returns the list position */
undefined4 FUN_001541f0(int puzzle)
{
	undefined4 next;
	unsigned int action;
	unsigned int after;

	next = (undefined4)CLList_Remove((int *)DAT_003075b0, (int *)puzzle);
	if (MA_I32(puzzle, 8) != 0) {
		MA_DELETE(MA_PTR(puzzle, 8));
		MA_I32(puzzle, 8) = 0;
	}
	action = MA_NEXT(puzzle + 0xc, NULL);
	while (action != 0) {
		after = (unsigned int)CLList_Remove((int *)(puzzle + 0xc), (int *)action);
		MA_DELETE(action);
		action = MA_NEXT(puzzle + 0xc, after);
	}
	FUN_00152f80();
	MA_DELETE(puzzle);
	return next;
}

/* 0x00154270  Meta/puzzles.cpp: delete all puzzles */
void FUN_00154270(void)
{
	unsigned int puzzle;

	puzzle = MA_NEXT(DAT_003075b0, NULL);
	while (puzzle != 0)
		puzzle = MA_NEXT(DAT_003075b0, FUN_001541f0((int)puzzle));
}

/* 0x001542a0  Meta/puzzles.cpp: evaluate a condition tree (no tree: true) */
uint FUN_001542a0(undefined4 *node)
{
	uint left;
	uint right;

	if (node == NULL)
		return 1;
	switch (node[0]) {
	case 0:
		return FUN_00153850((int)node);
	case 1:
		if (FUN_001542a0((undefined4 *)node[1]) == 0)
			return 0;
		return FUN_001542a0((undefined4 *)node[2]) == 0 ? 0 : 1;
	case 2:
		if (FUN_001542a0((undefined4 *)node[1]) != 0)
			return 1;
		return FUN_001542a0((undefined4 *)node[2]) == 0 ? 0 : 1;
	case 3:
		right = FUN_001542a0((undefined4 *)node[2]);
		left = FUN_001542a0((undefined4 *)node[1]);
		return left ^ right;
	case 4:
		return FUN_001542a0((undefined4 *)node[1]) == 0;
	default:
		return 1;
	}
}

/* set or clear `bits` of an entity flag word and print the debug text */
static void ma_entity_flag(int entity, int on, unsigned int set, unsigned int clear,
	const char *text_on, const char *text_off)
{
	if (on != 0) {
		MA_U32(entity, 0x450) = MA_U32(entity, 0x450) | set;
		MA_PRINTF(text_on);
	} else {
		MA_U32(entity, 0x450) = MA_U32(entity, 0x450) & clear;
		MA_PRINTF(text_off);
	}
}

/* attach an entity (or a meta_b object) to the node `hash` of the actor of charlib
   object `index` (FUN_000b83f0), at `offset` */
static void ma_attach(int index, undefined4 hash, float *offset, int *actor_at, int *node_at, float *copy_to)
{
	int object;
	int actor;

	object = FUN_000b83f0(index);
	actor = object + 0x10;
	*actor_at = actor;
	if (actor != 0) {
		*node_at = (int)baFindNodeByHash((struct BActorNode *)MA_PTR(actor, 0x144), hash);
		Core_Vec4Copy(copy_to, offset);
	}
}

/* 0x00154360  Meta/puzzles.cpp: run a puzzle action */
void FUN_00154360(int action)
{
	unsigned int node;
	int entity;
	int target;
	int camera;
	int board;
	int sample;
	int player;
	int i;
	int distance;
	int strength;
	int length;
	float delta[4];
	float raid[6];
	float rotated[4];
	float value;
	signed char sign;
	void *memory;

	target = MA_I16(action, 0x14);
	switch (MA_I32(action, 8)) {
	case 0: /* debris ring at the entities with the tag */
		MA_PRINTF("trying to execute debris target=%i timer=%d\n", target, (double)MA_F32(action, 0xc));
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14)) {
				MA_PRINTF("CreateDebrisCircle\n");
				ma_debris_ring(MA_PTR(action, 0x18), MA_I16(action, 0x2c),
					MA_FP(node, 0xe0), 7.0f, 7.0f, 0.0f, 0, 0xffffffff);
			}
		}
		break;
	case 1: /* destroy the entities */
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14)) {
				FUN_000b9880((int)node, 0);
				FUN_00165f10((int)node);
			}
		}
		break;
	case 2: /* variable = value (flag 1: = variable) */
		if ((MA_U8(action, 0x4c) & 1) == 0)
			DAT_00305660[MA_I16(action, 0x14)] = (int)MA_F32(action, 0x18);
		else
			DAT_00305660[MA_I16(action, 0x14)] = DAT_00305660[(int)MA_F32(action, 0x18)];
		break;
	case 3: /* variable += value */
		if ((MA_U8(action, 0x4c) & 1) == 0)
			DAT_00305660[target] = DAT_00305660[target] + (int)MA_F32(action, 0x18);
		else
			DAT_00305660[target] = DAT_00305660[target] + DAT_00305660[(int)MA_F32(action, 0x18)];
		break;
	case 4: /* variable -= value */
		if ((MA_U8(action, 0x4c) & 1) == 0)
			i = (int)MA_F32(action, 0x18);
		else
			i = DAT_00305660[(int)MA_F32(action, 0x18)];
		DAT_00305660[target] = DAT_00305660[target] - i;
		break;
	case 5:
	case 6:
		FUN_00153fd0(action);
		break;
	case 7: /* platform speed (+0x41c, per 1024), direction and flags */
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) != MA_I16(action, 0x14))
				continue;
			if (MA_F32(node, 0x41c) < 0.0f)
				sign = -1;
			else if (MA_F32(node, 0x41c) <= 0.0f)
				sign = 0;
			else
				sign = 1;
			value = MA_F32(action, 0x18) * 1024.0f; /* 0x00253dfc */
			MA_F32(node, 0x41c) = value;
			if ((0 < MA_I8(node, 0x3d6) && MA_F32(action, 0x18) < 0.0f) ||
				(MA_I8(node, 0x3d6) < 0 && 0.0f < MA_F32(action, 0x18)))
				MA_F32(node, 0x41c) = value * -1.0f;
			MA_I8(node, 0x3d6) = sign;
			if ((MA_U8(action, 0x4c) & 1) == 0)
				MA_U32(node, 0x450) = MA_U32(node, 0x450) & 0xfffffffe;
			else
				MA_U32(node, 0x450) = MA_U32(node, 0x450) | 9;
			if ((MA_U8(action, 0x4c) & 2) == 0)
				MA_U32(node, 0x450) = MA_U32(node, 0x450) & 0xfffffffd;
			else
				MA_U32(node, 0x450) = MA_U32(node, 0x450) | 0x12;
			if ((MA_U8(action, 0x4c) & 4) == 0)
				MA_U32(node, 0x428) = MA_U32(node, 0x428) & 0xfffffffe;
			else
				MA_U32(node, 0x428) = MA_U32(node, 0x428) | 1;
		}
		break;
	case 8:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				MA_U32(MA_PTR(node, 0x448), 0x10) = MA_U32(action, 0x18);
		}
		break;
	case 9:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				MA_U32(MA_PTR(node, 0x444), 0x14) = MA_U32(action, 0x18);
		}
		break;
	case 10: /* tiles */
		FUN_0015a8b0(100000000, MA_I16(action, 0x2c), 0xffffffff, (float)MA_I16(action, 0x30),
			MA_I16(action, 0x2e), NULL);
		break;
	case 0xb:
		FUN_0015a8b0(MA_I16(action, 0x2c), MA_I16(action, 0x2e), 0xffffffff, (float)MA_I16(action, 0x32),
			MA_I16(action, 0x30), NULL);
		break;
	case 0xc: /* remove the tile groups / tiles of a group id (+2 short) */
		if (MA_I16(action, 0x2c) == -1) {
			FUN_0015b400();
			MA_I32(DAT_003082c0, 0xc) = 0;
			break;
		}
		for (node = MA_NEXT(DAT_003082d4, NULL); node != 0; node = MA_NEXT(DAT_003082d4, node)) {
			if (MA_I16(MA_PTR(node, 0x84), 2) == MA_I16(action, 0x2c))
				FUN_0015a630((int)node);
		}
		board = DAT_003082c0;
		for (i = 0; i < MA_I32(board, 0xc); i++) {
			if (MA_I16(MA_PTR(board, 0x10 + i * 4), 2) == MA_I16(action, 0x2c)) {
				FUN_00159350(board, i);
				board = DAT_003082c0;
				i--;
			}
		}
		break;
	case 0xd: /* the same by the int at +0x24 */
		for (node = MA_NEXT(DAT_003082d4, NULL); node != 0; node = MA_NEXT(DAT_003082d4, node)) {
			if (MA_I32(MA_PTR(node, 0x84), 0x24) == MA_I16(action, 0x2c))
				FUN_0015a630((int)node);
		}
		board = DAT_003082c0;
		for (i = 0; i < MA_I32(board, 0xc); i++) {
			if (MA_I32(MA_PTR(board, 0x10 + i * 4), 0x24) == MA_I16(action, 0x2c)) {
				FUN_00159350(board, i);
				board = DAT_003082c0;
				i--;
			}
		}
		break;
	case 0xe:
		MA_I32(DAT_003082c0, 0x330 + MA_I16(action, 0x2c) * 4) = 1;
		break;
	case 0xf:
		MA_I32(DAT_003082c0, 0x330 + MA_I16(action, 0x2c) * 4) = 0;
		break;
	case 0x10: /* animation */
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				FUN_001517e0((int)node + 0x10, MA_U32(action, 0x18), MA_I16(action, 0x2c),
					MA_I16(action, 0x2e), MA_U32(action, 0x3c), MA_F32(action, 0x40));
		}
		break;
	case 0x11:
		MA_PRINTF("activate\n");
		FUN_000b7e40(MA_I16(action, 0x14));
		break;
	case 0x12:
		MA_PRINTF("deactivate\n");
		FUN_000b7ee0(MA_I16(action, 0x14));
		break;
	case 0x13:
		MetaVents_SetActive(MA_I16(action, 0x14), 1);
		break;
	case 0x14:
		MetaVents_SetActive(MA_I16(action, 0x14), 0);
		break;
	case 0x15:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				MA_U32(node, 0x24c) = MA_U32(action, 0x3c);
		}
		break;
	case 0x16:
		FUN_001630c0(MA_I16(action, 0x14), MA_U32(action, 0x3c), MA_U32(action, 0x40));
		break;
	case 0x17:
		FUN_00161d00((undefined4)(int)MA_I16(action, 0x14));
		break;
	case 0x18:
		FUN_00161d20((undefined4)(int)MA_I16(action, 0x14), MA_I16(action, 0x2c));
		break;
	case 0x19:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				ma_entity_flag((int)node, MA_I16(action, 0x2c), 0x100, 0xfffffeff, "Hurty\n", "NOT Hurty\n");
		}
		break;
	case 0x1a:
		CLRoute_Rebuild();
		break;
	case 0x1b:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				ma_entity_flag((int)node, MA_I16(action, 0x2c), 0x200, 0xfffffdff, "Springy\n", "NOT Springy\n");
		}
		break;
	case 0x1c:
		MA_U32(DAT_003082c0, 0x12d4) = MA_U32(action, 0x18);
		break;
	case 0x1d: /* air raid: from (+0x2c), to (+0x34), velocity (+0x44), 2 floats each */
		raid[4] = MA_F32(action, 0x2c);
		raid[5] = MA_F32(action, 0x30);
		raid[2] = MA_F32(action, 0x34);
		raid[3] = MA_F32(action, 0x38);
		raid[0] = MA_F32(action, 0x44);
		raid[1] = MA_F32(action, 0x48);
		MA_PRINTF("Starting Air Raid!\n");
		memory = MA_NEW(0x3c, ma_file_puzzles, 0x47c, 0x1001);
		if (memory != NULL)
			CAirRaid_Construct((struct CAirRaid *)memory, MA_F32(action, 0x18), MA_F32(action, 0x3c),
				&raid[4], &raid[2], MA_F32(action, 0x40), &raid[0]);
		break;
	case 0x1e:
		CLRoute_SetHeights((struct CLRouteHeights *)DAT_00309f00[MA_I16(action, 0x2c)]);
		break;
	case 0x1f: /* trap open / close */
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) != MA_I16(action, 0x14))
				continue;
			if (MA_I16(action, 0x2c) != 0 && (MA_U32(node, 0x450) | 0x800) != 0) {
				FUN_001517e0((int)node + 0x10, bkCRCString("2drop"), 0, 0, ma_fbits(0.8f), 0.0f);
				MA_U32(node, 0x450) = (MA_U32(node, 0x450) & 0xfffff7ff) | 0x400;
				MA_PRINTF("open trap\n");
			} else if ((MA_U32(node, 0x450) | 0x400) != 0) {
				FUN_001517e0((int)node + 0x10, bkCRCString("4close"), 0, 0, ma_fbits(0.7f), 0.0f);
				MA_U32(node, 0x450) = (MA_U32(node, 0x450) & 0xfffffbff) | 0x800;
				MA_PRINTF("close trap\n");
			}
		}
		break;
	case 0x20:
		FUN_000d5970(DAT_003055d4, (undefined4)(int)MA_I16(action, 0x2c), MA_I16(action, 0x2e));
		break;
	case 0x21:
		CLightningMgr_StrikeGroup((struct CLightningMgr *)DAT_00308460, (short)MA_U16(action, 0x14),
			MA_U16(action, 0x2c));
		break;
	case 0x22:
		FUN_0015c8e0((int)DAT_00308460, (undefined4)(int)MA_I16(action, 0x14), MA_U32(action, 0x3c),
			MA_U32(action, 0x40));
		break;
	case 0x23:
		for (node = MA_NEXT(DAT_003090a8, NULL); node != 0; node = MA_NEXT(DAT_003090a8, node)) {
			if (MA_I32(node, 0x6c) == MA_I16(action, 0x14) || MA_I16(action, 0x14) == -1)
				MA_I32(node, 0x68) = 2;
		}
		break;
	case 0x24:
		for (node = MA_NEXT(DAT_003090a8, NULL); node != 0; node = MA_NEXT(DAT_003090a8, node)) {
			if (MA_I32(node, 0x6c) == MA_I16(action, 0x14) || MA_I16(action, 0x14) == -1)
				MA_I32(node, 0x68) = 3;
		}
		break;
	case 0x25: /* a bomb */
		memory = MA_NEW(0x5c, ma_file_puzzles, 0x4af, 0x1001);
		if (memory != NULL)
			CBomb_Construct((struct CBomb *)memory, MA_I16(action, 0x14), MA_FP(action, 0x2c), 0);
		break;
	case 0x26:
		for (node = MA_NEXT(DAT_003090a8, NULL); node != 0; node = MA_NEXT(DAT_003090a8, node)) {
			if (MA_I32(node, 0x6c) == MA_I16(action, 0x14) || MA_I16(action, 0x14) == -1)
				MA_F32(node, 0x34) = MA_F32(action, 0x18) * 1024.0f; /* 0x00253dfc */
		}
		break;
	case 0x27:
		CBomb_LightFuses(MA_I16(action, 0x14));
		break;
	case 0x28:
		MbBombGroup_Start(MA_I16(action, 0x2c), (uint)(int)MA_I16(action, 0x2e), MA_I16(action, 0x30),
			MA_F32(action, 0x18));
		break;
	case 0x29: /* sounds: the sample of slot +0x14, the channel goes to slot +0x36 */
	case 0x2a:
	case 0x2b:
	case 0x2c:
		if (MA_I16(action, 0x36) < 0 || 999 < MA_I16(action, 0x36))
			break;
		sample = DAT_003045c8[MA_I16(action, 0x14)];
		if (sample == 0)
			break;
		switch (MA_I32(action, 8)) {
		case 0x29:
			i = CLSamples_Play(sample, MA_I16(action, 0x2c), 0, 0, NULL);
			break;
		case 0x2a:
			i = CLSamples_Play(sample, MA_I16(action, 0x2c), MA_I16(action, 0x30), MA_I16(action, 0x2e), NULL);
			break;
		default: /* positioned */
			i = CLSamples_Play(sample, MA_I16(action, 0x2c), MA_I16(action, 0x30), MA_I16(action, 0x2e),
				MA_FP(action, 0x2c));
			break;
		}
		DAT_00306600[MA_I16(action, 0x36)] = i;
		break;
	case 0x2d:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				Actor_SetFade((void *)(node + 0x10), MA_F32(action, 0x3c), MA_F32(action, 0x40));
		}
		break;
	case 0x2e:
		FUN_0015be90(MA_FP(action, 0x2c), (uint)(int)MA_I16(action, 0x2c));
		break;
	case 0x2f: /* attach entities to a node of a charlib object */
		FUN_000b7e40(MA_I16(action, 0x14));
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14)) {
				MA_I32(node, 0x330) = 1;
				ma_attach((int)MA_F32(action, 0x3c), MA_U32(action, 0x18), MA_FP(action, 0x2c),
					(int *)MA_ADDR(node, 0x464), (int *)MA_ADDR(node, 0x460), MA_FP(node, 0xe0));
				MA_U32(node, 0x230) = MA_U32(node, 0x230) & 0xfffffffe;
				MA_I32(node, 0x1a8) = 0;
			}
		}
		break;
	case 0x30: /* detach (0x31: and make it fall) */
	case 0x31:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14)) {
				FUN_00153f60((int)node);
				if (MA_I32(action, 8) == 0x31) {
					MA_U32(node, 0x230) = MA_U32(node, 0x230) | 1;
					MA_I32(node, 0x1a8) = 0;
				}
			}
		}
		break;
	case 0x32:
		FUN_000d62c0(DAT_003055d4, (undefined4)(int)MA_FP(action, 0x2c), (undefined4)(int)MA_I16(action, 0x14),
			(undefined4)(int)DAT_002ea478);
		break;
	case 0x33: /* delete camera objects with the tag */
		camera = DAT_003055d4;
		node = MA_NEXT(camera + 0x2b4, NULL);
		while (node != 0) {
			entity = (int)node;
			if (MA_I32(entity, 0x40) == MA_I16(action, 0x14)) {
				node = MA_U32(entity, 4);
				FUN_000d5bb0(entity);
				MA_DELETE(entity);
			}
			node = MA_NEXT(DAT_003055d4 + 0x2b4, node);
		}
		break;
	case 0x34: /* camera target; with a time: the speed */
		camera = DAT_003055d4;
		Core_Vec4Copy(MA_FP(camera, 0x4c0), MA_FP(action, 0x2c));
		if (MA_I16(action, 0x14) == 0) {
			Core_Vec4Copy(MA_FP(DAT_003055d4, 0x4b0), MA_FP(DAT_003055d4, 0x4c0));
			for (i = 0; i < DAT_002b084c; i++)
				FUN_000d6e80(DAT_003055d4);
		} else {
			MA_F32(DAT_003055d4, 0x4d0) = (float)((double)bmVectorDistance(MA_FP(DAT_003055d4, 0x4b0),
				MA_FP(DAT_003055d4, 0x4c0)) / (double)MA_I16(action, 0x14));
		}
		break;
	case 0x35:
		bsStopChannel(DAT_00306600[MA_I16(action, 0x14)]);
		break;
	case 0x36: /* camera shake */
		DAT_002f2878 = 1;
		DAT_002f287c = MA_U32(action, 0x3c);
		DAT_002f2880 = MA_F32(action, 0x40);
		DAT_002f2884 = MA_I16(action, 0x2c);
		DAT_002f2888 = MA_I16(action, 0x2e);
		DAT_002f288c = MA_I16(action, 0x30);
		if (DAT_002f2880 == 0.0f)
			DAT_002f2878 = 0;
		break;
	case 0x37:
		FUN_000b7380((undefined4)(int)MA_FP(action, 0x3c), MA_F32(action, 0x18), MA_I16(action, 0x32),
			(undefined4)(int)MA_I16(action, 0x2c), (undefined4)(int)MA_I16(action, 0x2e),
			(undefined4)(int)MA_I16(action, 0x30), 0);
		break;
	case 0x38:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14)) {
				FUN_000b7490((undefined4)node,
					(undefined4)(int)baFindNodeByHash((struct BActorNode *)MA_PTR(node, 0x154), MA_U32(action, 0x18)),
					(undefined4)(int)MA_FP(action, 0x1c), MA_F32(action, 0x3c),
					(undefined4)(int)MA_I16(action, 0x2c), (undefined4)(int)MA_I16(action, 0x2e),
					(undefined4)(int)MA_I16(action, 0x30), (undefined4)(int)MA_I16(action, 0x32));
				break;
			}
		}
		break;
	case 0x39:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14) && MA_I32(node, 0x44c) != 0)
				MA_I32(MA_PTR(node, 0x44c), 0x44) = MA_I16(action, 0x2c);
		}
		break;
	case 0x3a:
		FUN_00081ed0((undefined4)MA_PTR(action, 0x18), (undefined4)MA_FP(action, 0x2c),
			(int)MA_F32(action, 0x44), MA_F32(action, 0x3c), MA_F32(action, 0x40), (unsigned int)MA_I16(action, 0x14));
		break;
	case 0x3b: /* rumble the pads, weaker with the distance to the entity */
		/* the entity with the tag; with none, the last one; with an empty list, the
		   action itself (as in the original) */
		node = MA_NEXT(MA_ENTITIES, NULL);
		if (node == 0) {
			entity = action;
		} else {
			for (;;) {
				entity = (int)node;
				if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
					break;
				node = MA_NEXT(MA_ENTITIES, node);
				if (node == 0)
					break;
			}
		}
		for (player = 0; player < DAT_002bae94; player++) {
			Core_Vec3Sub(delta, MA_FP(MA_CHAR(DAT_003075bc[player]), 0xd0), MA_FP(entity, 0xe0));
			value = cmb_vec3_length(delta);
			if (value <= 50.0f) { /* 0x002559b8 */
				strength = MA_I16(action, 0x2e);
				length = MA_I16(action, 0x30);
			} else {
				/* 0x00255544 = 100 */
				distance = (int)(((double)value * 100.0) / (double)DAT_002b04ec);
				length = MA_I16(action, 0x30) - (DAT_002b04f8 * distance) / 0x96;
				strength = MA_I16(action, 0x2e) - (DAT_002b04f4 * distance) / 0x96;
			}
			CLRumble_PlayMotor((int)DAT_002ed324, player, MA_I16(action, 0x2c), strength, length,
				MA_U16(action, 0x4c), MA_I16(action, 0x32), MA_I16(action, 0x34), MA_I16(action, 0x36));
		}
		break;
	case 0x3c:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14)) {
				MA_I32(node, 0x238) = MA_I16(action, 0x2c);
				MA_I32(node, 0x23c) = MA_I16(action, 0x2e);
				if ((MA_U32(node, 0x230) & 0x400) == 0)
					MA_U32(node, 0x230) = MA_U32(node, 0x230) | 0x200;
			}
		}
		break;
	case 0x3d:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14)) {
				MA_I32(node, 0x238) = MA_I16(action, 0x2c);
				MA_I32(node, 0x234) = MA_I16(action, 0x2c);
				if ((MA_U32(node, 0x230) & 0x400) == 0)
					MA_U32(node, 0x230) = MA_U32(node, 0x230) | 0x200;
			}
		}
		break;
	case 0x3e:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				MA_F32(node, 0x418) = MA_F32(action, 0x18) * 1024.0f; /* 0x00253dfc */
		}
		break;
	case 0x3f:
		CMLaser_Show(MA_FP(action, 0x2c), MA_I16(action, 0x14), MA_F32(action, 0x3c));
		break;
	case 0x40:
		for (node = MA_NEXT(DAT_00308440, NULL); node != 0; node = MA_NEXT(DAT_00308440, node)) {
			if (MA_I32(node, 0x38) == MA_I16(action, 0x14)) {
				MA_I32(node, 0x3c) = MA_I16(action, 0x2c);
				break;
			}
		}
		break;
	case 0x41:
		for (node = MA_NEXT(DAT_00308440, NULL); node != 0; node = MA_NEXT(DAT_00308440, node)) {
			if (MA_I32(node, 0x38) == MA_I16(action, 0x14))
				ma_attach((int)MA_F32(action, 0x3c), MA_U32(action, 0x18), MA_FP(action, 0x2c),
					(int *)MA_ADDR(node, 0x58), (int *)MA_ADDR(node, 0x54), MA_FP(node, 0x18));
		}
		break;
	case 0x42:
		bmVectorRotateXY(rotated, DAT_0028fdd8, MA_F32(action, 0x3c), MA_F32(action, 0x40));
		FUN_000b72d0((undefined4)(int)MA_I16(action, 0x2c), (undefined4)(int)MA_I16(action, 0x2e),
			(undefined4)(int)MA_I16(action, 0x30), MA_U32(action, 0x44), (undefined4)(int)rotated,
			MA_U32(action, 0x18), 0);
		break;
	case 0x43:
		break;
	case 0x44:
		CMetaExplSmoke_Create(MA_FP(action, 0x2c), MA_I16(action, 0x14), MA_F32(action, 0x3c),
			(int)MA_F32(action, 0x44), MA_F32(action, 0x40), MA_U32(action, 0x50));
		break;
	case 0x45:
		for (node = MA_NEXT(DAT_003090d8, NULL); node != 0; node = MA_NEXT(DAT_003090d8, node)) {
			if (MA_I32(node, 0x5c) == MA_I16(action, 0x14))
				FUN_001635e0((int)node, MA_U32(action, 0x18));
		}
		break;
	case 0x46:
		for (node = MA_NEXT(MA_ENTITIES, NULL); node != 0; node = MA_NEXT(MA_ENTITIES, node)) {
			if (MA_I32(node, 0x3d0) == MA_I16(action, 0x14))
				ma_entity_flag((int)node, MA_I16(action, 0x2c), 0x8100, 0xffff7eff, "Hurty\n", "NOT Hurty\n");
		}
		break;
	case 0x47:
		FUN_00081a30((undefined4)MA_PTR(action, 0x18), MA_I16(action, 0x14), MA_FP(action, 0x1c),
			MA_FP(action, 0x2c), MA_F32(action, 0x3c), MA_F32(action, 0x3c), MA_F32(action, 0x40),
			MA_F32(action, 0x44), MA_F32(action, 0x48), 0, MA_U32(action, 0x50));
		break;
	case 0x48:
		FUN_000b7620((undefined4)(int)MA_I16(action, 0x14));
		break;
	case 0x49:
		FUN_0015b760(MA_I16(action, 0x2c), (undefined4)(int)MA_I16(action, 0x2e));
		break;
	default:
		break;
	}
}
