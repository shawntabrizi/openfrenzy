/*
ROLLERBALL_B_MISSING.H

Functions of rollerball_b that are missing from src/include/of_functions.tsv (Ghidra did
not find them: they are only reached through jump tables or function pointers). They keep
their FUN_<addr> names; the architect adds them to the table (module report).

Owner: rollerball_b.
*/

#ifndef OPENFRENZY_ROLLERBALL_B_MISSING_H
#define OPENFRENZY_ROLLERBALL_B_MISSING_H

/* camera end shots (jump table of 0x00194800) */
void FUN_00193120(void *self, float dt);
void FUN_00193260(void *self, float dt);
void FUN_00193360(void *self, float dt);

#endif
