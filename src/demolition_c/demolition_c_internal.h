/*
demolition_c_internal.h - helpers shared by the demolition_c source files only.
*/

#ifndef DEMOLITION_C_INTERNAL_H
#define DEMOLITION_C_INTERNAL_H

#include "demolition_c.h"

#include <math.h>
#include <stdio.h>

static const char s_DcLiftFile[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Lift.cpp";
static const char s_DcGasJetsFile[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_GasJets.cpp";
static const char s_DcLiftShaftFile[] =
	"C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_LiftShaft.cpp";
static const char s_DcASEFile[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_ASEImport.cpp";
static const char s_DcParticleFile[] =
	"C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Particle.cpp";

/* the game object's fields used here (CDemolition_Game, demolition_a) */
#define DC_GAME DAT_002ff85c

/* ---------- CDemolition_Wall (demolition_d, 0x1c4 bytes, vtable 0x0026bef0): a grid of quads.
   Methods whose table prototype still has an undefined4 where the code passes a float go
   through DC_DUAL. */

/* 0x0010dba0  moves the wall up to height y */
static inline void dc_wall_set_height(void *wall, float y)
{
	DC_DUAL(FUN_0010dba0, void (*)(int, undefined4), void (*)(int, float),
		((int)wall, dc_fbits(y)), ((int)wall, y));
}

/* 0x0010dbc0  turns the wall about y */
static inline void dc_wall_set_angle(void *wall, float angle)
{
	DC_DUAL(FUN_0010dbc0, void (*)(int, undefined4), void (*)(int, float),
		((int)wall, dc_fbits(angle)), ((int)wall, angle));
}

/* 0x0010f4c0  updates the wall (frame time dt) */
static inline void dc_wall_update(void *wall, float dt)
{
	DC_DUAL(FUN_0010f4c0, void (*)(int, undefined4), void (*)(int, float),
		((int)wall, dc_fbits(dt)), ((int)wall, dt));
}

#endif
