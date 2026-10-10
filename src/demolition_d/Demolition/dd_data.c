/*
Demolition/dd_data.c  (module demolition_d)

Initialised data owned by demolition_d (rebuilt from the XBE .data section).
*/

#include "demolition_d.h"

/* 0x00298bb0  the brain parameters, by game variation * 3 + difficulty (easy, medium, hard):
   the brains' params[0..4]. Names inferred from their use: [0] aggression (revenge, attack
   chance), [1] leader bias, [2] skill (scan radius, bombs remembered), [3] aim (variation 3),
   [4] reaction (scan interval) */
float DAT_00298bb0[12][5] = {
	{ 0.25f, 0.25f, 0.25f, 0.25f, 0.25f },
	{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f },
	{ 0.75f, 0.75f, 0.75f, 0.75f, 0.75f },
	{ 0.150000006f, 0.150000006f, 0.150000006f, 0.25f, 0.25f },
	{ 0.300000012f, 0.349999994f, 0.300000012f, 0.5f, 0.5f },
	{ 0.850000024f, 0.949999988f, 0.75f, 0.75f, 0.75f },
	{ 0.25f, 0.25f, 0.25f, 0.25f, 0.25f },
	{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f },
	{ 0.75f, 0.75f, 0.75f, 0.75f, 0.75f },
	{ 0.25f, 0.25f, 0.25f, 0.150000006f, 0.150000006f },
	{ 0.5f, 0.5f, 0.5f, 0.300000012f, 0.300000012f },
	{ 0.75f, 0.75f, 0.75f, 0.550000012f, 0.949999988f },
};
