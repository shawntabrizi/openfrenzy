/*
Drillers/unplaced_snare.c  <-  C:\BlitzParty\Game\Src\Drillers\ (file not known)

The closing of a snare loop in variant 4 (Quantum Snare). The object at 0x002aae60 (vtable
0x0026cc44, which foxhunt's CModule_FoxHunt.c defines) holds the loop's polygon: x, z
pairs from +0x04. The function lies after CDrillers_Particle.cpp (it follows
DrRain_Create, 0x00124bc0); no assert names its file. Its caller is the drillers helper
0x00125090, which the function table gives to foxhunt.
*/

#include "drillers.h"

#define DR_AI_OBJ DR_PTR(void, 0x002aadf8u)
#define DR_HALF_Z DR_VAR(int, 0x002ff9e4u)
#define DR_HALF_X DR_VAR(int, 0x002ff9e8u)
#define DR_CELL_SIZE DR_VAR(int, 0x002ff9e0u)
#define DR_SAMPLE(addr) DR_VAR(struct BSample *, addr)
#define DR_FONT_FLOATY DR_VAR(int, 0x002aa98cu)
/* the floaty score's system and its "big" flag (bumpercars_b's BCarFloatyScore +0x8c) */
#define DR_FLOATY_SYSTEM DR_VAR(struct BExtraSystem *, 0x003014e8u)
#define DR_FLOATY_BIG DR_AT(g_DrFx->floaty, 0x8c, int)

/* 0x00124c00  Drillers (gap; after CDrillers_Particle.cpp)
Player `player` closed a loop of its first `count` trail points (the polygon ends with
point 0 again). Every other player (not of the same team) whose head is inside is hit:
a hit timer of 64 (or -64 when it ran negative), and when it was not hit yet: the "5"
score floaty, the white flash, the hit sound, the effect group and a scorch mark. Every
token inside that has bounced more than twice is collected: 1, 3 or 5 points (gold);
the total is added to the player's points twice, as in the original. With any token: a
sound, a speech sample for 4 or more, and the total as a big floaty score at the
tokens' mean position. */
void DrSnare_Close(void *self, int player_arg, int count)
{
	signed char player = (signed char)player_arg;
	float *poly = DR_FP(self, 4);
	int gained = 0, collected = 0, any = 0;
	float sum_x = 0.0f, sum_z = 0.0f;
	float pos[3];
	float tx, tz, px, pz;
	double cell, half, inv;
	char buf[40];
	int k, character;
	int *hit;
	DrTokenSlot *t;

	for (k = 0; k < count; k++) {
		poly[k * 2] = DR_TRAIL_X(player, k);
		poly[k * 2 + 1] = DR_TRAIL_Z(player, k);
	}
	poly[count * 2] = DR_TRAIL_X(player, 0);
	poly[count * 2 + 1] = DR_TRAIL_Z(player, 0);

	for (k = 0; k < 4; k++) {
		if (k == player)
			continue;
		if (DAT_002baebc != 0 && DAT_002baef4[player * 10] == DAT_002baef4[k * 10])
			continue;
		if (DrAi_InPolygon(DR_AI_OBJ, DR_TRAIL_X(k, 0), DR_TRAIL_Z(k, 0), count + 1, poly) ==
			0)
			continue;
		hit = &DR_PF(k, 0x6f4, int);
		if (0 < *hit) {
			*hit = 0x40;
		} else if (*hit < 0) {
			*hit = -0x40;
		} else {
			DR_PF(player, 0x6fc, float) = 150.0f;
			*hit = 0x40;
			DrFx_AddScoreFloaty(DR_TRAIL_X(k, 0), DR_TRAIL_Z(k, 0), 2, (char)player_arg);
			DR_PF(k, 0x6f8, float) = 255.0f;
			px = (float)((double)DR_PF(k, 0x04, float) * (double)0.0009765625f - DR_HALF_X);
			pz = (float)((double)DR_PF(k, 0x08, float) * (double)0.0009765625f - DR_HALF_Z);
			px = (float)((double)px * DR_CELL_SIZE);
			pz = (float)((double)DR_CELL_SIZE * pz);
			pos[0] = px;
			pos[1] = (float)DR_CELL_SIZE;
			pos[2] = pz;
			bsPlaySample(DR_SAMPLE(0x002aa9c8u), 200, 0, 0, 0);
			DrFxGroup_Start(&g_DrFx->groups[k], pos, k);
			baAddExtraItem(g_DrFx->scorch, (double)px, (double)((float)k + 15.0f),
				(double)pz);
		}
	}

	for (k = 0; k < 22; k++) {
		t = &g_DrFx->tokens[k];
		if (t->active == 0 || !(2.0f < t->bounces))
			continue;
		cell = (double)DR_CELL_SIZE;
		half = (double)(DR_CELL_SIZE / 2);
		tx = (float)(cell * t->x + half);
		tz = (float)(cell * t->z + half);
		if (DrAi_InPolygon(DR_AI_OBJ, tx, tz, count + 1, poly) == 0)
			continue;
		sum_x = sum_x + tx;
		DR_PF(player, 0x6fc, float) = 150.0f;
		sum_z = sum_z + tz;
		switch (t->type) {
		case 0:
			DR_PF(player, 0x60, int) += 1;
			gained += 1;
			break;
		case 1:
			DR_PF(player, 0x60, int) += 3;
			gained += 3;
			break;
		case 2:
			DR_PF(player, 0x60, int) += 5;
			gained += 5;
			break;
		}
		t->active = 0;
		CToken_Collect(t->token);
		collected++;
		any = 1;
	}
	if (any)
		bsPlaySample(DR_SAMPLE(0x002aa9b4u), 200, 0, 0, 0);
	if (3 < collected)
		CLSamples_PlayFromSet((char)DAT_002baed4[player * 10], 4, NULL, 200, 0, -1, 0);
	DR_PF(player, 0x60, int) += gained;
	sprintf(buf, "%d", gained);
	if (0 < collected) {
		inv = 1.0 / (double)collected;
		character = DAT_002baed4[player * 10];
		DR_FLOATY_BIG = 1;
		baAddExtraItem(DR_FLOATY_SYSTEM, DR_FONT_FLOATY, (double)sum_x * inv, 500.0,
			(double)sum_z * inv, (int)DAT_00290718[character * 0x60],
			(int)DAT_00290718[character * 0x60 + 1], (int)DAT_00290718[character * 0x60 + 2],
			buf);
		DR_FLOATY_BIG = 0;
	}
}
