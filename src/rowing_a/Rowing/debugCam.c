/*
Rowing/debugCam.c  <-  C:\BlitzParty\Game\Src\Rowing\debugCam.cpp

The Rowing debug camera: a free-flying bCamController (engine_misc) toggled by the
"DbugCam" input channel or by a watched int. 0x0019d640 and 0x0019d6e0 are tagged with this
file; the gap functions around them (0x0019d5c0-0x0019d630, 0x0019d7d0-0x0019d870) use the
same globals and are inferred to be in it. Other games call these functions too (Drillers
0x00110960, BumperCars, FoxHunt).

Globals (.bss):
  0x0030ef0c  the camera holder (4 bytes: the bCamController), NULL when off
  0x0030ef10  the input map, 0x0030ef14 the "DbugCam" channel (+0x14: pressed)
  0x0030ef18  an int the caller watches: the camera follows its value (may be NULL)
  0x0030ef1c  1 after Init, until Shutdown
*/

#include "rowing_a.h"

#define g_DebugCam RW_VAR(void **, 0x0030ef0c)
#define g_DebugCamMap RW_VAR(struct BInputMap *, 0x0030ef10)
#define g_DebugCamChannel RW_VAR(struct biChannel *, 0x0030ef14)
#define g_DebugCamToggle RW_VAR(int *, 0x0030ef18)
#define g_DebugCamReady RW_VAR(int, 0x0030ef1c)

/* deletes the camera holder: the controller's deleting destructor, then the holder */
static void DebugCam_Delete(void **holder)
{
	if (*holder != NULL)
		RA_VFN(*holder, 0x00, void (*)(void *, int))(*holder, 1);
	*holder = NULL;
	baSetCurrentCamera(NULL);
	bkHeapFree(holder);
}

/* 0x0019d5c0  Rowing (gap; debugCam.cpp inferred)
Creates the input map with the "DbugCam" channel. */
int DebugCam_Init(void)
{
	g_DebugCam = NULL;
	g_DebugCamMap = biCreateMap(1);
	g_DebugCamChannel = biCreateChannel((struct biMap *)g_DebugCamMap, "DbugCam", 1, 1, 0, 0, 0);
	g_DebugCamToggle = NULL;
	g_DebugCamReady = 1;
	return 1;
}

/* 0x0019d610  Rowing (gap; debugCam.cpp inferred)
Init, and the camera follows *TOGGLE from now on. */
int DebugCam_InitWatch(int *toggle)
{
	DebugCam_Init();
	g_DebugCamToggle = toggle;
	return 1;
}

/* 0x0019d630  Rowing (gap; debugCam.cpp inferred) */
int DebugCam_IsActive(void)
{
	return g_DebugCam != NULL;
}

/* 0x0019d640  Rowing/debugCam.cpp
The holder's constructor: a bCamController at (0, 1500, 0) looking down, speed 50, bound to
pad 0. */
void **DebugCam_Ctor(void **self)
{
	void *memory = RA_NEW(0xa8, RA_FILE_DEBUGCAM, 0x22);

	*self = memory != NULL ? (void *)bCamController_Construct((struct BCamController *)memory) : NULL;
	bCamController_Setup((struct BCamController *)*self, 0.0f, 1500.0f, 0.0f, 1.5707963f, 0.0f, 0.0f,
		50.0f, 1);
	bCamController_BindInput((struct BCamController *)*self, 0);
	RA_VFN(*self, 0x08, void (*)(void *))(*self);
	return self;
}

/* 0x0019d6e0  Rowing/debugCam.cpp
Switches the debug camera on or off. Returns 1 when it is on now. */
int DebugCam_Toggle(void)
{
	void **memory;

	if (g_DebugCam != NULL) {
		bkPrintf("Debug Camera Disabled\n");
		if (g_DebugCam != NULL)
			DebugCam_Delete(g_DebugCam);
		g_DebugCam = NULL;
		return 0;
	}
	bkPrintf("Debug Camera Active\n");
	memory = (void **)RA_NEW(4, RA_FILE_DEBUGCAM, 0x90);
	g_DebugCam = memory != NULL ? DebugCam_Ctor(memory) : NULL;
	return 1;
}

/* 0x0019d7d0  Rowing (gap; debugCam.cpp inferred)
The debug camera's position (+0x68 of the controller), or (0, 0, 0, 1) when it is off. */
void DebugCam_GetPosition(float *out)
{
	void *ctrl;

	if (g_DebugCam == NULL) {
		out[2] = 0.0f;
		out[1] = 0.0f;
		out[0] = 0.0f;
		out[3] = 1.0f;
		return;
	}
	ctrl = *g_DebugCam;
	out[0] = RA_F32(ctrl, 0x68);
	out[1] = RA_F32(ctrl, 0x6c);
	out[2] = RA_F32(ctrl, 0x70);
	out[3] = RA_F32(ctrl, 0x74);
}

/* 0x0019d810  Rowing (gap; debugCam.cpp inferred)
Deletes the camera and the input map. */
void DebugCam_Shutdown(void)
{
	void **holder = g_DebugCam;

	if (g_DebugCamReady == 0)
		return;
	g_DebugCamReady = 0;
	if (g_DebugCam != NULL) {
		DebugCam_Delete(holder);
		g_DebugCam = NULL;
	}
	biDeleteMap((struct biMap *)g_DebugCamMap);
}

/* 0x0019d870  Rowing (gap; debugCam.cpp inferred)
Once per frame: the channel or the watched int switches the camera; then it moves. */
void DebugCam_Update(void)
{
	if (RA_I32(g_DebugCamChannel, 0x14) != 0) {
		DebugCam_Toggle();
		RA_I32(g_DebugCamChannel, 0x14) = 0;
	}
	if (g_DebugCamToggle != NULL) {
		if (*g_DebugCamToggle == 0) {
			if (g_DebugCam != NULL)
				DebugCam_Toggle();
			else
				return;
		} else if (g_DebugCam == NULL) {
			DebugCam_Toggle();
		}
	}
	if (g_DebugCam != NULL)
		RA_VFN(*g_DebugCam, 0x04, void (*)(void *, float))(*g_DebugCam, 1.0f);
}
