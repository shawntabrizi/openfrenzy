/*
Demolition/unplaced_charlibai.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\ (gap: file not known)

The CharLib AI base of the Demolition players (vtable 0x0026bdc0; demolition_b's
CDemolition_CharLib derives from it, constructor 0x000f8a50), and the thunks that the CharLib
AI tables call.

A CharLib AI (charlib_ai.h) keeps three tables: +0x1094 the state functions
`void (*)(void *ai, int entered)`, +0x1098 the condition functions `int (*)(void *ai)` and
+0x109c the priorities. The Demolition tables (demolition_b: 0x00298820 / 0x002987d8 for the
players, 0x0029796c / 0x00297964 for the remote bomb) hold these thunks, which forward to
virtual slots of the AI: conditions 0..17 to slots 0x4c..0x90 (x86 `jmp`, so the result is
passed back), states 0..17 to slots 0x94..0xd8. The base slots return 0 / do nothing
(0x00151650, 0x000f8ab0).
*/

#include "demolition_d.h"

/* 0x0026bdc0  the vtable (slots 5, 6, 14 and 15 are the CRT's _purecall: 0) */
static const void *const vtable_0026bdc0[55] = {
	(const void *)FUN_001d5950, /* [0] +0x00 tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [1] +0x04 tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [2] +0x08 tankedup_b TankedUp */
	(const void *)FUN_000b1860, /* [3] +0x0c charlib CharLib */
	(const void *)FUN_00151650, /* [4] +0x10 meta_a Meta */
	(const void *)0, /* [5] 0x001e0323: not a game/engine function */
	(const void *)0, /* [6] 0x001e0323: not a game/engine function */
	(const void *)FUN_00151650, /* [7] +0x1c meta_a Meta */
	(const void *)FUN_000b0560, /* [8] +0x20 charlib CharLib */
	(const void *)FUN_000b0370, /* [9] +0x24 charlib CharLib */
	(const void *)FUN_000b0cb0, /* [10] +0x28 charlib CharLib */
	(const void *)FUN_001d5950, /* [11] +0x2c tankedup_b TankedUp */
	(const void *)FUN_000b06c0, /* [12] +0x30 charlib CharLib */
	(const void *)FUN_000a6fa0, /* [13] +0x34 bumpercars_b BumperCars */
	(const void *)0, /* [14] 0x001e0323: not a game/engine function */
	(const void *)0, /* [15] 0x001e0323: not a game/engine function */
	(const void *)FUN_000b0770, /* [16] +0x40 charlib CharLib */
	(const void *)FUN_001ec260, /* [17] +0x44 engine_misc LIB:engine_tail */
	(const void *)DemoCLAI_DeletingDtor, /* [18] +0x48 demolition_d (in CDemolition_Crate.c) */
	(const void *)FUN_00151650, /* [19] +0x4c meta_a Meta */
	(const void *)FUN_00151650, /* [20] +0x50 meta_a Meta */
	(const void *)FUN_00151650, /* [21] +0x54 meta_a Meta */
	(const void *)FUN_00151650, /* [22] +0x58 meta_a Meta */
	(const void *)FUN_00151650, /* [23] +0x5c meta_a Meta */
	(const void *)FUN_00151650, /* [24] +0x60 meta_a Meta */
	(const void *)FUN_00151650, /* [25] +0x64 meta_a Meta */
	(const void *)FUN_00151650, /* [26] +0x68 meta_a Meta */
	(const void *)FUN_00151650, /* [27] +0x6c meta_a Meta */
	(const void *)FUN_00151650, /* [28] +0x70 meta_a Meta */
	(const void *)FUN_00151650, /* [29] +0x74 meta_a Meta */
	(const void *)FUN_00151650, /* [30] +0x78 meta_a Meta */
	(const void *)FUN_00151650, /* [31] +0x7c meta_a Meta */
	(const void *)FUN_00151650, /* [32] +0x80 meta_a Meta */
	(const void *)FUN_00151650, /* [33] +0x84 meta_a Meta */
	(const void *)FUN_00151650, /* [34] +0x88 meta_a Meta */
	(const void *)FUN_00151650, /* [35] +0x8c meta_a Meta */
	(const void *)FUN_00151650, /* [36] +0x90 meta_a Meta */
	(const void *)FUN_000f8ab0, /* [37] +0x94 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [38] +0x98 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [39] +0x9c demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [40] +0xa0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [41] +0xa4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [42] +0xa8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [43] +0xac demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [44] +0xb0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [45] +0xb4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [46] +0xb8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [47] +0xbc demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [48] +0xc0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [49] +0xc4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [50] +0xc8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [51] +0xcc demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [52] +0xd0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [53] +0xd4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [54] +0xd8 demolition_b Demolition */
};

/* 0x0010d220  Demolition (gap)
Condition 0: the AI's virtual slot 0x4c. */
int DemoCLAI_Cond00(void *ai)
{
	return DD_VFN(ai, 0x4c, int (*)(void *))(ai);
}

/* 0x0010d230  Demolition (gap)
Condition 1: the AI's virtual slot 0x50. */
int DemoCLAI_Cond01(void *ai)
{
	return DD_VFN(ai, 0x50, int (*)(void *))(ai);
}

/* 0x0010d240  Demolition (gap)
Condition 2: the AI's virtual slot 0x54. */
int DemoCLAI_Cond02(void *ai)
{
	return DD_VFN(ai, 0x54, int (*)(void *))(ai);
}

/* 0x0010d250  Demolition (gap)
Condition 3: the AI's virtual slot 0x58. */
int DemoCLAI_Cond03(void *ai)
{
	return DD_VFN(ai, 0x58, int (*)(void *))(ai);
}

/* 0x0010d260  Demolition (gap)
Condition 4: the AI's virtual slot 0x5c. */
int DemoCLAI_Cond04(void *ai)
{
	return DD_VFN(ai, 0x5c, int (*)(void *))(ai);
}

/* 0x0010d270  Demolition (gap)
Condition 5: the AI's virtual slot 0x60. */
int DemoCLAI_Cond05(void *ai)
{
	return DD_VFN(ai, 0x60, int (*)(void *))(ai);
}

/* 0x0010d280  Demolition (gap)
Condition 6: the AI's virtual slot 0x64. */
int DemoCLAI_Cond06(void *ai)
{
	return DD_VFN(ai, 0x64, int (*)(void *))(ai);
}

/* 0x0010d290  Demolition (gap)
Condition 7: the AI's virtual slot 0x68. */
int DemoCLAI_Cond07(void *ai)
{
	return DD_VFN(ai, 0x68, int (*)(void *))(ai);
}

/* 0x0010d2a0  Demolition (gap)
Condition 8: the AI's virtual slot 0x6c. */
int DemoCLAI_Cond08(void *ai)
{
	return DD_VFN(ai, 0x6c, int (*)(void *))(ai);
}

/* 0x0010d2b0  Demolition (gap)
Condition 9: the AI's virtual slot 0x70. */
int DemoCLAI_Cond09(void *ai)
{
	return DD_VFN(ai, 0x70, int (*)(void *))(ai);
}

/* 0x0010d2c0  Demolition (gap)
Condition 10: the AI's virtual slot 0x74. */
int DemoCLAI_Cond10(void *ai)
{
	return DD_VFN(ai, 0x74, int (*)(void *))(ai);
}

/* 0x0010d2d0  Demolition (gap)
Condition 11: the AI's virtual slot 0x78. */
int DemoCLAI_Cond11(void *ai)
{
	return DD_VFN(ai, 0x78, int (*)(void *))(ai);
}

/* 0x0010d2e0  Demolition (gap)
Condition 12: the AI's virtual slot 0x7c. */
int DemoCLAI_Cond12(void *ai)
{
	return DD_VFN(ai, 0x7c, int (*)(void *))(ai);
}

/* 0x0010d2f0  Demolition (gap)
Condition 13: the AI's virtual slot 0x80. */
int DemoCLAI_Cond13(void *ai)
{
	return DD_VFN(ai, 0x80, int (*)(void *))(ai);
}

/* 0x0010d300  Demolition (gap)
Condition 14: the AI's virtual slot 0x84. */
int DemoCLAI_Cond14(void *ai)
{
	return DD_VFN(ai, 0x84, int (*)(void *))(ai);
}

/* 0x0010d310  Demolition (gap)
Condition 15: the AI's virtual slot 0x88. */
int DemoCLAI_Cond15(void *ai)
{
	return DD_VFN(ai, 0x88, int (*)(void *))(ai);
}

/* 0x0010d320  Demolition (gap)
Condition 16: the AI's virtual slot 0x8c. */
int DemoCLAI_Cond16(void *ai)
{
	return DD_VFN(ai, 0x8c, int (*)(void *))(ai);
}

/* 0x0010d330  Demolition (gap)
Condition 17: the AI's virtual slot 0x90. */
int DemoCLAI_Cond17(void *ai)
{
	return DD_VFN(ai, 0x90, int (*)(void *))(ai);
}

/* 0x0010d340  Demolition (gap)
State 0: the AI's virtual slot 0x94. */
void DemoCLAI_State00(void *ai, int entered)
{
	DD_VFN(ai, 0x94, void (*)(void *, int))(ai, entered);
}

/* 0x0010d360  Demolition (gap)
State 1: the AI's virtual slot 0x98. */
void DemoCLAI_State01(void *ai, int entered)
{
	DD_VFN(ai, 0x98, void (*)(void *, int))(ai, entered);
}

/* 0x0010d380  Demolition (gap)
State 2: the AI's virtual slot 0x9c. */
void DemoCLAI_State02(void *ai, int entered)
{
	DD_VFN(ai, 0x9c, void (*)(void *, int))(ai, entered);
}

/* 0x0010d3a0  Demolition (gap)
State 3: the AI's virtual slot 0xa0. */
void DemoCLAI_State03(void *ai, int entered)
{
	DD_VFN(ai, 0xa0, void (*)(void *, int))(ai, entered);
}

/* 0x0010d3c0  Demolition (gap)
State 4: the AI's virtual slot 0xa4. */
void DemoCLAI_State04(void *ai, int entered)
{
	DD_VFN(ai, 0xa4, void (*)(void *, int))(ai, entered);
}

/* 0x0010d3e0  Demolition (gap)
State 5: the AI's virtual slot 0xa8. */
void DemoCLAI_State05(void *ai, int entered)
{
	DD_VFN(ai, 0xa8, void (*)(void *, int))(ai, entered);
}

/* 0x0010d400  Demolition (gap)
State 6: the AI's virtual slot 0xac. */
void DemoCLAI_State06(void *ai, int entered)
{
	DD_VFN(ai, 0xac, void (*)(void *, int))(ai, entered);
}

/* 0x0010d420  Demolition (gap)
State 7: the AI's virtual slot 0xb0. */
void DemoCLAI_State07(void *ai, int entered)
{
	DD_VFN(ai, 0xb0, void (*)(void *, int))(ai, entered);
}

/* 0x0010d440  Demolition (gap)
State 8: the AI's virtual slot 0xb4. */
void DemoCLAI_State08(void *ai, int entered)
{
	DD_VFN(ai, 0xb4, void (*)(void *, int))(ai, entered);
}

/* 0x0010d460  Demolition (gap)
State 9: the AI's virtual slot 0xb8. */
void DemoCLAI_State09(void *ai, int entered)
{
	DD_VFN(ai, 0xb8, void (*)(void *, int))(ai, entered);
}

/* 0x0010d480  Demolition (gap)
State 10: the AI's virtual slot 0xbc. */
void DemoCLAI_State10(void *ai, int entered)
{
	DD_VFN(ai, 0xbc, void (*)(void *, int))(ai, entered);
}

/* 0x0010d4a0  Demolition (gap)
State 11: the AI's virtual slot 0xc0. */
void DemoCLAI_State11(void *ai, int entered)
{
	DD_VFN(ai, 0xc0, void (*)(void *, int))(ai, entered);
}

/* 0x0010d4c0  Demolition (gap)
State 12: the AI's virtual slot 0xc4. */
void DemoCLAI_State12(void *ai, int entered)
{
	DD_VFN(ai, 0xc4, void (*)(void *, int))(ai, entered);
}

/* 0x0010d4e0  Demolition (gap)
State 13: the AI's virtual slot 0xc8. */
void DemoCLAI_State13(void *ai, int entered)
{
	DD_VFN(ai, 0xc8, void (*)(void *, int))(ai, entered);
}

/* 0x0010d500  Demolition (gap)
State 14: the AI's virtual slot 0xcc. */
void DemoCLAI_State14(void *ai, int entered)
{
	DD_VFN(ai, 0xcc, void (*)(void *, int))(ai, entered);
}

/* 0x0010d520  Demolition (gap)
State 15: the AI's virtual slot 0xd0. */
void DemoCLAI_State15(void *ai, int entered)
{
	DD_VFN(ai, 0xd0, void (*)(void *, int))(ai, entered);
}

/* 0x0010d540  Demolition (gap)
State 16: the AI's virtual slot 0xd4. */
void DemoCLAI_State16(void *ai, int entered)
{
	DD_VFN(ai, 0xd4, void (*)(void *, int))(ai, entered);
}

/* 0x0010d560  Demolition (gap)
State 17: the AI's virtual slot 0xd8. */
void DemoCLAI_State17(void *ai, int entered)
{
	DD_VFN(ai, 0xd8, void (*)(void *, int))(ai, entered);
}

/* 0x0010d580  Demolition (gap)
Constructor: the CharLib AI (charlib), then this vtable. */
void *DemoCLAI_Ctor(void *self)
{
	AI_Ctor(self);
	*(const void *const **)self = vtable_0026bdc0;
	return self;
}

/* 0x0010d5a0  Demolition (gap)
Destructor. */
void DemoCLAI_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026bdc0;
	AI_Dtor(self);
}
