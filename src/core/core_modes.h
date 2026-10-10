/*
CORE_MODES.H

openfrenzy core module. The game-mode classes of Core\*.cpp: the menus_common.cpp base
(CMenuMode, from 0x00030090) and its screens (CGameMode_Scores, CGameMode_Team).
Layouts are the Xbox ones (ILP32); unknown parts are padding with their offset in the name.
*/

#ifndef OPENFRENZY_CORE_MODES_H
#define OPENFRENZY_CORE_MODES_H

#include <stdint.h>
#include "core_globals.h"

/* menus_common.cpp: a CGameMode (core_modes_a 0x00012800) with button repeat; 0x98 bytes */
struct CMenuMode {
	const void *const *vtbl;      /* +0x00 */
	uint32_t field_04;            /* +0x04 */
	const char *name;             /* +0x08 */
	uint32_t field_0c;            /* +0x0c */
	uint32_t flags;               /* +0x10 1: opaque, 2: present players only, 4: */
	int state;                    /* +0x14 */
	unsigned char base_18[0x10];  /* +0x18 */
	void *actor;                  /* +0x28 the menu character (menus_common) */
	uint8_t held[4][9];           /* +0x2c button held when the mode started */
	int repeat_delay[9];          /* +0x50 frames before a held button repeats */
	int repeat_rate[9];           /* +0x74 */
};
_Static_assert(sizeof(struct CMenuMode) == 0x98, "CMenuMode size");

/* the 9 input buttons of each player (0x002baf78, 0x24 bytes per player); +0x20: frames held */
#define g_PlayerButton(player, button) CORE_VAR(unsigned char *, 0x002baf78 + (player) * 0x24 + (button) * 4)
#define g_PlayerCount CORE_VAR(int, 0x002bae94)
#define g_PlayerCharacter(i) CORE_VAR(int, 0x002baed4 + (i) * CORE_PLAYER_STRIDE)
#define g_PlayerScore(i) CORE_VAR(int, 0x002baee8 + (i) * CORE_PLAYER_STRIDE)
#define g_TeamMode CORE_VAR(int, 0x002baebc)
/* team mode: team t has players g_TeamPlayer(t, 0) and g_TeamPlayer(t, 1) */
#define g_TeamPlayer(t, k) CORE_VAR(int, 0x002baec0 + (t) * 8 + (k) * 4)

/* the menu actors shared by all CMenuMode screens (menus_common.cpp) */
#define g_MenuActor CORE_VAR(unsigned char *, 0x002bb008)
#define g_MenuActorOld CORE_VAR(unsigned char *, 0x002bb00c)
#define g_MenuInputMaps ((void **)CORE_ADDR(0x002bb010))  /* 4 */
#define g_MenuStencil CORE_VAR(int, 0x002bb020)
#define g_MenuModeCount CORE_VAR(int, 0x002bb024)

#endif
