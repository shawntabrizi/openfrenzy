/*
BINPUT/input.c: C:\Babel\Xbox\Src\BINPUT\input.cpp, the one function of it that the
function table gives to engine_misc (biCreateMap). The rest of input.cpp
(0x00048970-0x0004a0b2: channels, binding, pad polling) is in module bkernel.
*/

#include "engine_misc_gui.h"
#include "engine_misc_extern.h"

/* 0x002cb570: the pads' state (0xc40 bytes) and 0x002cc1b0 (.bss, used by bkernel's
input.cpp functions too; common symbols) */
unsigned int DAT_002cb570[0x310];
int DAT_002cc1b0;

/* 0x002cc1b8: first map of the circular list of all input maps (.bss; bkernel's
input.cpp functions use it too, merged as a common symbol) */
struct BInputMap *DAT_002cc1b8;

/* 0x00048880  BINPUT (input.cpp?)
Initialises the input system: the table puts it in BGUI's gap, but it clears BINPUT's
pad state and sits right before biCreateMap, so it is placed here. */
void biInit(int param)
{
	DAT_002cc1b0 = 0;
	em_biInitPads(param);
	memset(DAT_002cb570, 0, sizeof(DAT_002cb570));
	em_bkCreateEvent("_PadChange");
	em_bkCreateEvent("_MemCardChange");
}

/* 0x000488c0  BINPUT/input.cpp
Creates an input map with room for maxChannels channels and links it into the list
of maps. */
struct BInputMap *biCreateMap(int maxChannels)
{
	struct BInputMap *map;

	map = (struct BInputMap *)em_bkAllocNamed(maxChannels * (int)sizeof(struct BInputChannel) +
		(int)sizeof(struct BInputMap), "C:\\Babel\\Xbox\\Src\\BINPUT\\input.cpp", 0x53, 0x2001,
		"Input Map", 0);
	if (map == NULL) {
		em_bkPrintf("biCreateMap: ERROR - Out of memory\n");
		return NULL;
	}
	map->maxChannels = maxChannels;
	map->numChannels = 0;
	map->channels = (struct BInputChannel *)(map + 1);
	memset(map + 1, 0, ((unsigned int)(maxChannels * (int)sizeof(struct BInputChannel)) >> 2) * 4);
	map->field_0c = 0;
	if (DAT_002cc1b8 != NULL) {
		map->prev = DAT_002cc1b8->prev;
		map->next = DAT_002cc1b8;
		DAT_002cc1b8->prev->next = map;
		DAT_002cc1b8->prev = map;
		return map;
	}
	DAT_002cc1b8 = map;
	map->next = map;
	map->prev = map;
	return map;
}
