/*
META_A_FOREIGN.C

Functions that meta_a needs but that are missing from src/include/of_functions.tsv
(Ghidra never made them functions; they are reached only through a table or a pointer).
Each is defined weak, so the owner of the address range can define it too; the
architect should add them to the table (see the final report of meta_a).
*/

#include "meta_a.h"

/* 0x000ad830  CharLib/Route.cpp (charlib range): constructor of the 0x18-byte route
   height object that CGameMode_Meta allocates per route cell (new[] at
   CGameMode_Meta.cpp line 0x254); the array destructor is FUN_000ad880. */
__attribute__((weak)) undefined4 *FUN_000ad830(undefined4 *self)
{
	self[0] = (undefined4)MA_NEW(DAT_00292c78 * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23d, 0x2001);
	self[1] = (undefined4)MA_NEW(DAT_00292c78, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23e, 0x2001);
	return self;
}

