/*
ClassLib/DebrisSys.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\DebrisSys.cpp

The debris system. Only its wind set-up is in this module's address range; the rest
of DebrisSys.cpp is in the BumperCars block (bumpercars_a).
*/

#include "classlib_fx.h"

static const char s_DebrisSysFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\DebrisSys.cpp";

/* 0x00086880  ClassLib/DebrisSys.cpp
Allocates the wind state (0x128 bytes) at self + 4: no gusts, direction (0, 0, 0, 1).
Returns 0 when out of memory. */
int DebrisSys_InitWind(int self)
{
	int wind;

	CLF_Printf("allocating memory for wind\n");
	wind = (int)CLF_Alloc(0x128, s_DebrisSysFile, 0x4b, 0x2001);
	*(int *)(self + 4) = wind;
	if (wind == 0)
		return 0;
	*(int *)(wind + 0x120) = 0;
	*(int *)(*(int *)(self + 4) + 0x124) = 0;
	*(float *)(*(int *)(self + 4) + 0x10) = 0.0f;
	*(float *)(*(int *)(self + 4) + 0x14) = 0.0f;
	*(float *)(*(int *)(self + 4) + 0x18) = 0.0f;
	*(float *)(*(int *)(self + 4) + 0x1c) = 1.0f;
	return 1;
}
