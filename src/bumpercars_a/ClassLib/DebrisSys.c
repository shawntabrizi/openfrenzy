/*
ClassLib/DebrisSys.c  <-  C:\BlitzParty\Game\Src\ClassLib\DebrisSys.cpp   (module bumpercars_a)

The wind of the debris system: a base direction plus up to four gusts that swell and fade
with time, and up to four swirls that turn pieces around a vertical axis. These are gap
functions after DebrisSys_InitWind (0x00086880, classlib_fx, which allocates the wind);
the file is inferred from that neighbour.
*/

#include "bumpercars_a.h"

int DebrisSys_InitWind(int self);

/* 0x000868f0  (gap, DebrisSys.cpp inferred): this frame's wind: the base direction plus each
gust's direction times its strength, which swings between its minimum and maximum with
its period; each swirl's strength swings likewise */
void DebrisSys_UpdateWind(struct DebrisSysData *data)
{
	struct DebrisWind *wind = data->wind;
	struct DebrisGust *gust;
	struct DebrisSwirl *swirl;
	double strength;
	int i;

	wind->dir[0] = wind->base[0];
	wind->dir[1] = wind->base[1];
	wind->dir[2] = wind->base[2];
	wind->dir[3] = wind->base[3];
	for (i = 0; i < wind->numGusts; i++) {
		gust = &wind->gusts[i];
		strength = ((double)gust->maximum - (double)gust->minimum) *
			(sin((double)data->time / (double)gust->period * (double)0x1.921fb4p+2f) + 1.0) * 0.5 +
			(double)gust->minimum;
		wind->dir[0] = (float)(strength * (double)gust->dir[0] + (double)wind->dir[0]);
		wind->dir[1] = (float)(strength * (double)gust->dir[1]) + wind->dir[1];
		wind->dir[2] = (float)(strength * (double)gust->dir[2]) + wind->dir[2];
	}
	for (i = 0; i < wind->numSwirls; i++) {
		swirl = &wind->swirls[i];
		swirl->strength = (float)(((double)swirl->maximum - (double)swirl->minimum) *
			(sin((double)data->time / (double)swirl->period * (double)0x1.921fb4p+2f) + 1.0) * 0.5 +
			(double)swirl->minimum);
	}
}

/* 0x000869d0  (gap): sets the base wind direction */
void DebrisSys_SetWindDirection(struct DebrisSysData *data, float *dir)
{
	struct DebrisWind *wind = data->wind;

	if (wind != NULL) {
		wind->base[0] = dir[0];
		wind->base[1] = dir[1];
		wind->base[2] = dir[2];
		wind->base[3] = dir[3];
	}
}

/* 0x00086a00  (gap): the swirls' push on a piece at pos (out, xyz): each swirl within twice
its radius (in XZ) turns the piece about its centre, most strongly at the radius.
Returns the number of swirls that act. */
int DebrisSys_SwirlForce(float *out, struct DebrisSysData *data, float *pos)
{
	struct DebrisWind *wind;
	struct DebrisSwirl *swirl;
	float d[4], turned[4], dist2, e;
	int i, n = 0;

	if (data->wind == NULL) {
		BCA_Printf("WARNING no wind \n");
		return 0;
	}
	out[0] = 0.0f;
	out[1] = 0.0f;
	out[2] = 0.0f;
	out[3] = 1.0f;
	wind = data->wind;
	for (i = 0; i < wind->numSwirls; i++) {
		swirl = &wind->swirls[i];
		d[0] = pos[0] - swirl->pos[0];
		d[2] = pos[2] - swirl->pos[2];
		d[1] = 0.0f;
		d[3] = 1.0f;
		dist2 = d[2] * d[2] + d[0] * d[0];
		if (dist2 <= swirl->radius * swirl->radius * 4.0f) {
			n++;
			e = sqrtf(dist2) - swirl->radius;
			if (e < 0.0f)
				e = e * -1.0f;
			FUN_00051170(turned, d, ((1.0f - e / swirl->radius) / DAT_002ba990) * swirl->strength);
			out[0] = (turned[0] - d[0]) + out[0];
			out[1] = (turned[1] - d[1]) + out[1];
			out[2] = (turned[2] - d[2]) + out[2];
		}
		wind = data->wind;
	}
	return n;
}

/* 0x00086b70  (gap): gives a debris system its wind (DebrisSys_InitWind on its user data) */
int DebrisSys_InitSystemWind(struct BExtraSystem *system)
{
	return DebrisSys_InitWind((int)(uintptr_t)system->userData);
}
