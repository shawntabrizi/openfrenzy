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

#endif
