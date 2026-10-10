/*
ClassLib/classlib_ui_extern.c  (classlib_ui)

Tentative definitions of the globals of other modules that classlib_ui code uses
(docs/ARCHITECTURE.md section 12: -fcommon merges them with the owner's definition,
or with the owner's alias into its data block). The owners are named in
classlib_ui.h. When an owner has not defined a global yet, it stays zero here.
*/

#include "classlib_ui.h"

unsigned int DAT_00285630;       /* the engine's rand() seed, initial value 1 (owner: engine) */
char *DAT_002ba544;              /* core */
unsigned char DAT_002ba750[0x58]; /* core */
int DAT_00283d74;                /* core */
int DAT_00283e0c;                /* core */
