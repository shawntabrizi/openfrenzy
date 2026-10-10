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

/* ParseMap callbacks of CRollerball_World.cpp ("World", "Instance", "Object" nodes) */
int FUN_00195400(int node, void *self);
int FUN_00195930(int node, void *self);
int FUN_001965a0(int node, void *self);

/* the race path record's destructor (rollerball_a calls it) */
void FUN_00198110(void *self);

/* race2 controller: approach the path point (jump table of 0x0019cd90) */
void FUN_0019bdd0(void *self);

#endif
