/*
CORE_TWEAK.H

openfrenzy core module. CTweakMenu and CTweakItem (Core/CTweakMenu.cpp): debug menu pages
that edit game variables. Layouts are the Xbox ones (ILP32).
*/

#ifndef OPENFRENZY_CORE_TWEAK_H
#define OPENFRENZY_CORE_TWEAK_H

#include <stdint.h>
#include "core_data.h"

struct CTweakMenu;

/* 0x1a4 bytes */
struct CTweakItem {
	char name[0x80];               /* +0x000 */
	char text[0x80];               /* +0x080 "name: value" shown in the menu */
	const char *labels[0x20];      /* +0x100 names of the values of an int item, or NULL */
	int *ivar;                     /* +0x180 */
	int *bvar;                     /* +0x184 */
	float *fvar;                   /* +0x188 */
	float reset;                   /* +0x18c the value at creation */
	float step;                    /* +0x190 */
	float max;                     /* +0x194 */
	float min;                     /* +0x198 */
	struct CTweakMenu *menu;       /* +0x19c */
	void *submenu;                 /* +0x1a0 Up/Down/Reset/... */
};

/* 0xe8 bytes; linked into the list at 0x002bae5c */
struct CTweakMenu {
	void *next;                    /* +0x00 list node */
	void *prev;                    /* +0x04 */
	int count;                     /* +0x08 items */
	int id_base;                   /* +0x0c menu ids: id_base + item * 32 + action */
	int id;                        /* +0x10 the id of this page in its parent */
	void *menu;                    /* +0x14 engine_misc menu */
	struct CTweakItem *items[32];  /* +0x18 */
	char name[0x40];               /* +0x98 */
	int is_top;                    /* +0xd8 */
	unsigned char list[0xc];       /* +0xdc an (unused) list head */
};

_Static_assert(sizeof(struct CTweakItem) == 0x1a4, "CTweakItem size");
_Static_assert(sizeof(struct CTweakMenu) == 0xe8, "CTweakMenu size");

/* the root debug menu (made in CGameMode_Root.cpp, 0x00027540) */
#define g_DebugMenu CORE_VAR(void *, 0x002ba988)

#endif
