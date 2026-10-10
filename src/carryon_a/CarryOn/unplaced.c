/*
CarryOn/unplaced.c

Gap functions of the carryon_a range without a known source file.

0x000ba6d0 lies between the last CharLib function (0x000ba470) and CFire_Game.cpp. Other
minigames call it every frame too (0x00126220, 0x0014ea80, 0x00173f80), so it is most
likely the end of a CharLib file.
*/

#include "carryon_a.h"

/* 0x000ba6d0  (file not known; CharLib?)
Updates every item of the list at +0xc of the route context (DAT_002ea488) with charlib
0x000ba470, which returns the node to continue from. */
void FUN_000ba6d0(void)
{
	unsigned int *list = (unsigned int *)((char *)DAT_002ea488 + 0xc);
	unsigned int node;

	node = CLList_Next(list, NULL);
	while (node != 0)
		node = CLList_Next(list, (unsigned int *)co_RouteItem_Update((void *)node));
}
