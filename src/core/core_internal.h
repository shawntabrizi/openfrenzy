/*
CORE_INTERNAL.H

openfrenzy core module. Functions of the core module that are missing from
src/include/of_functions.tsv (Ghidra did not find them; reported to the architect), and
small helpers shared by core files. Functions in the table get their prototypes from
core.decls through of_decls.h.
*/

#ifndef OPENFRENZY_CORE_INTERNAL_H
#define OPENFRENZY_CORE_INTERNAL_H

/* 0x001f4e10: destructor (atexit) of the object at 0x002ba4f0; not in the table */
void FUN_001f4e10(void);

/* 0x00026d40: debug menu handler (core_modes_b range, not in the table); weak stub in
Core/CGameMode_Root.c */
void FUN_00026d40(void *event);

/* port (src/runtime/of_launch.c): fills the settings table from OPENFRENZY_GAME and
OPENFRENZY_VARIATION; the retail game leaves it empty */
void of_settings_from_environment(void *table);

#endif
