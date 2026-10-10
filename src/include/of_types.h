/*
OF_TYPES.H

Ghidra's type names and helper macros, with Xbox (ILP32) sizes, and layout-check
macros. Included by of_game.h, and alone by the generated stub file.
Owner: the runtime module (architect).
*/

#ifndef OPENFRENZY_TYPES_H
#define OPENFRENZY_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ---------- Ghidra types (sizes as on the Xbox) */

typedef unsigned char undefined;
typedef unsigned char undefined1;
typedef unsigned short undefined2;
typedef unsigned int undefined3;   /* a 3-byte value, held in 4 bytes */
typedef unsigned int undefined4;
typedef unsigned long long undefined6;
typedef unsigned long long undefined8;
typedef unsigned char byte;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned int dword;
typedef unsigned long ulong;
typedef long long longlong;
typedef unsigned long long ulonglong;
typedef unsigned long long qword;
/* x87 extended precision: the port computes in double */
typedef double float10;
typedef double unkbyte10;
/* a code address. Ghidra writes calls through it as (*(code *)p)(args): cast to a real
function pointer type instead when an argument or the result is a float. */
typedef int code();

_Static_assert(sizeof(void *) == 4, "the guest is ILP32");
_Static_assert(sizeof(long) == 4, "the guest is ILP32");

/* ---------- Ghidra helper macros */

#define CONCAT11(a, b) ((ushort)(((uint)(byte)(a) << 8) | (byte)(b)))
#define CONCAT12(a, b) ((uint)(((uint)(byte)(a) << 16) | (ushort)(b)))
#define CONCAT13(a, b) ((uint)(((uint)(byte)(a) << 24) | ((uint)(b) & 0xffffff)))
#define CONCAT21(a, b) ((uint)(((uint)(ushort)(a) << 8) | (byte)(b)))
#define CONCAT22(a, b) ((uint)(((uint)(ushort)(a) << 16) | (ushort)(b)))
#define CONCAT31(a, b) ((uint)(((uint)(a) << 8) | (byte)(b)))
#define CONCAT44(a, b) ((ulonglong)(((ulonglong)(uint)(a) << 32) | (uint)(b)))
#define SUB41(x, n) ((byte)((uint)(x) >> ((n) * 8)))
#define SUB42(x, n) ((ushort)((uint)(x) >> ((n) * 8)))
#define SUB81(x, n) ((byte)((ulonglong)(x) >> ((n) * 8)))
#define SUB84(x, n) ((uint)((ulonglong)(x) >> ((n) * 8)))
#define ZEXT14(x) ((uint)(byte)(x))
#define ZEXT24(x) ((uint)(ushort)(x))
#define ZEXT48(x) ((ulonglong)(uint)(x))
#define SEXT14(x) ((int)(signed char)(x))
#define SEXT24(x) ((int)(short)(x))
#define SEXT48(x) ((longlong)(int)(x))

/* ---------- layout checks for structs read from game files or shared with the platform */

#define OF_ASSERT_SIZE(type, size) _Static_assert(sizeof(type) == (size), #type " size")
#define OF_ASSERT_OFFSET(type, member, offset) \
	_Static_assert(offsetof(type, member) == (offset), #type "." #member " offset")

#endif
