/*
Clusteroids/unplaced_bullets.c  -  gap functions of C:\BlitzParty\Game\Src\Clusteroids\
without a known source file: bullets and collisions (0x000e47f0 and 0x000e6af0-0x000e7a40,
after CClust_Pods.cpp).

Bullets: 60 records of 0x2c bytes from 0x002fee08:
  +0x00 short (1: not drawn)  +0x04 x  +0x08 z  +0x0c vx  +0x10 vz  +0x14 short owner
  +0x18 life (75 frames, 0: free)  +0x1c short  +0x20 heading  +0x24 spin  +0x28 255
The Tails spawn points (0x002f7f88, 0x002f7f84 of them, 0x34 bytes: +0 x, +4 z, +0x20
strength) push the ships and bend the floor (ClustArena_GetHeight).
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define CL_BULLET(i) ((char *)CL_ADDR(0x002fee08) + (i) * 0x2c)
#define CL_BULLET_COUNT 60
#define g_FreePods CL_VAR(char *, 0x002fedfc)
#define g_SpawnCount CL_VAR(short, 0x002f7f84)
#define g_SpawnPoints CL_VAR(float *, 0x002f7f88)

/* sparks and the bullet-hit sound at (x, z) */
static void cl_hit_sparks(float x, float z)
{
	float pos[4];

	pos[0] = x;
	pos[1] = -13.2f;
	pos[2] = z;
	pos[3] = 0.0f;
	ClustArena_SparkRing(pos);
}

/* 0x000e47f0  Clusteroids (gap)
Moves the bullets (0.625 x velocity per frame) with a "BULLETTRAIL"; a bullet slower than
1 or leaving the arena dies (sparks and a sound at the wall). */
void ClustBullets_Move(void)
{
	char *bullet;
	float step[4];
	float pos[4];
	int i;

	for (i = 0; i < CL_BULLET_COUNT; i++) {
		bullet = CL_BULLET(i);
		if (CL_F32(bullet, 0x18) <= 0.0f)
			continue;
		step[0] = CL_F32(bullet, 0xc) * 0.625f * g_ClFrameTime;
		step[2] = CL_F32(bullet, 0x10) * 0.625f * g_ClFrameTime;
		if (1.0 <= (double)fabsf(CL_F32(bullet, 0xc)) || 1.0 <= (double)fabsf(CL_F32(bullet, 0x10))) {
			pos[0] = step[0] + CL_F32(bullet, 0x4);
			pos[1] = -13.2f;
			CL_F32(bullet, 0x4) = pos[0];
			pos[3] = 0.0f;
			step[1] = 0.0f;
			pos[2] = step[2] + CL_F32(bullet, 0x8);
			step[3] = 0.0f;
			CL_F32(bullet, 0x8) = pos[2];
			CL_F32(bullet, 0x24) = g_ClFrameTime * 0.2f + CL_F32(bullet, 0x24);
			CLVec3_Normalize(step);
			step[0] = step[0] * 0.8f;
			step[2] = step[2] * 0.8f;
			Debris_Add((struct DebrisType *)CL_ADDR(0x00296e88), pos, step, 0, 0x7f7f7f);
			if (CL_VAR(float, 0x002f7f68) - 35.0f < fabsf(sqrtf(CL_F32(bullet, 0x4) * CL_F32(bullet, 0x4) +
					CL_F32(bullet, 0x8) * CL_F32(bullet, 0x8)))) {
				CL_F32(bullet, 0x28) = 0.0f;
				CL_F32(bullet, 0x18) = 0.0f;
				cl_hit_sparks(CL_F32(bullet, 0x4), CL_F32(bullet, 0x8));
				ClustSound_Play(9, 100);
			}
		} else {
			CL_F32(bullet, 0x28) = 0.0f;
			CL_F32(bullet, 0x18) = 0.0f;
		}
	}
}

/* starts bullet `bullet` of `player` at (x, z) along heading `angle` */
static void cl_start_bullet(char *bullet, short player, double x, double z, double angle)
{
	CL_I16(bullet, 0x14) = player;
	CL_F32(bullet, 0x18) = 75.0f;
	CL_F32(bullet, 0x28) = 255.0f;
	CL_I16(bullet, 0x1c) = 0;
	CL_I16(bullet, 0x0) = 0;
	CL_F32(bullet, 0x4) = (float)x;
	CL_F32(bullet, 0x8) = (float)z;
	CL_F32(bullet, 0x20) = (float)angle;
	CL_F32(bullet, 0xc) = (float)((double)CL_VAR(float, 0x002f7db0) * sin(angle));
	CL_F32(bullet, 0x10) = (float)-((double)CL_VAR(float, 0x002f7db0) * cos(angle));
}

/* 0x000e6af0  Clusteroids (gap)
Player fires a bullet from 35 units ahead of the ship; with the power-up (+0x14) two
more at +-pi/32. */
void ClustBullets_Fire(short player)
{
	char *pl = CL_PLAYER(player);
	double heading;
	double x;
	double z;
	short i;

	if (CL_I16(pl, PL_OUT) != 0)
		return;
	heading = (double)CL_F32(pl, 0x634);
	for (i = 0; i < CL_BULLET_COUNT; i++) {
		if (CL_F32(CL_BULLET(i), 0x18) == 0.0f) {
			cl_start_bullet(CL_BULLET(i), player, 35.0 * sin(heading) + (double)CL_F32(pl, PL_X),
				-35.0 * cos(heading) + (double)CL_F32(pl, PL_Z), heading);
			break;
		}
	}
	if (!(0.0f < CL_F32(pl, 0x14)) || CL_BULLET_COUNT <= i)
		return;
	while (CL_F32(CL_BULLET(i), 0x18) != 0.0f) {
		i = i + 1;
		if (0x3b < i)
			return;
	}
	x = sin(heading) * 35.0 + (double)CL_F32(pl, PL_X);
	z = cos(heading) * -35.0 + (double)CL_F32(pl, PL_Z);
	cl_start_bullet(CL_BULLET(i), player, x, z, (double)CL_F32(pl, 0x634) + (double)0.0981747657f);
	for (; i < CL_BULLET_COUNT; i++) {
		if (CL_F32(CL_BULLET(i), 0x18) == 0.0f) {
			cl_start_bullet(CL_BULLET(i), player, x, z, (double)CL_F32(pl, 0x634) - (double)0.0981747657f);
			return;
		}
	}
}

/* 0x000e6d50  Clusteroids (gap) */
void ClustBullets_Init(void)
{
	int i;

	for (i = 0; i < CL_BULLET_COUNT; i++)
		CL_F32(CL_BULLET(i), 0x18) = 0.0f;
}

/* 0x000e6d70  Clusteroids (gap)
Tails: the spawn points push the ships away: a negative point within 200 units always,
a positive one within 300 units only while the ship was just hit (+0x664). The strength
is the first point's (+0x20) in the original. */
void ClustArena_PushFromSpawnPoints(void)
{
	char *pl;
	float *point;
	float inv;
	float dx;
	float dz;
	float force;
	float sum;
	float d;
	int player;
	int k;

	if (g_SpawnCount == 0)
		return;
	for (player = 0; player < 4; player++) {
		pl = CL_PLAYER(player);
		if (g_SpawnCount < 1)
			continue;
		point = g_SpawnPoints;
		for (k = (unsigned short)g_SpawnCount; k != 0; k--) {
			inv = 1.0f / CL_VAR(float, 0x002f7d10);
			dx = (CL_F32(pl, PL_X) - point[0]) * inv;
			dz = (CL_F32(pl, PL_Z) - point[1]) * inv;
			if (dx == 0.0f && dz == 0.0f) {
				dx = (float)cl_rand() * 4.0f * 3.0518509e-05f - 2.0f;
				dz = (float)cl_rand() * 4.0f * 3.0518509e-05f - 2.0f;
			}
			force = 0.0f;
			if (0.0f <= point[8]) {
				if (CL_F32(pl, 0x664) != 0.0f) {
					sum = fabsf(dx) + fabsf(dz);
					d = sqrtf(dx * dx + dz * dz);
					if (d < 300.0f)
						force = (300.0f - d) * g_SpawnPoints[8] * 0.025f;
					goto push;
				}
			} else {
				sum = fabsf(dx) + fabsf(dz);
				d = sqrtf(dx * dx + dz * dz);
				if (d < 200.0f)
					force = (200.0f - d) * g_SpawnPoints[8] * 0.005f;
push:
				sum = 1.0f / sum;
				CL_F32(pl, 0x5c) = sum * force * dx + CL_F32(pl, 0x5c);
				CL_F32(pl, 0x60) = sum * force * dz + CL_F32(pl, 0x60);
			}
			point = point + 0xd;
		}
	}
}

/* 0x000e6f70  Clusteroids (gap)
The player whose ship touches `pod` (within 70), or -1. In Collector a ship carrying
three pods or more bounces the pod off instead. */
short ClustPods_HitPlayer(char *pod)
{
	char *pl;
	char *link;
	float dir[4];
	float dx;
	float dz;
	float d;
	float rel;
	float inv;
	short count;
	short player;

	for (player = 0; player < 4; player++) {
		if (CL_I16(pod, 0x38) != 0)
			continue;
		pl = CL_PLAYER(player);
		count = 0;
		dx = CL_F32(pod, 0x10) - CL_F32(pl, PL_X);
		dz = CL_F32(pod, 0x14) - CL_F32(pl, PL_Z);
		d = sqrtf(dx * dx + dz * dz);
		if (!(d <= 70.0f))
			continue;
		if ((g_ClVariant == 4 || g_ClVariant == 2) && CL_PTR(pl, PL_PODS) != NULL) {
			count = 1;
			for (link = CL_PTR(CL_PTR(pl, PL_PODS), 0); link != NULL; link = CL_PTR(link, 0))
				count = count + 1;
		}
		if (g_ClVariant != 4 || count < 3)
			return player;
		dir[1] = 0.0f;
		dir[3] = 0.0f;
		rel = CL_F32(pl, 0x5c) - CL_F32(pod, 0x20);
		inv = CL_F32(pl, 0x60) - CL_F32(pod, 0x24);
		rel = sqrtf(rel * rel + inv * inv) * 0.5f;
		inv = 1.0f / (fabsf(dx) + fabsf(dz));
		dx = inv * rel * dx;
		CL_F32(pod, 0x20) = dx + CL_F32(pod, 0x20);
		dz = inv * rel * dz;
		CL_F32(pod, 0x24) = dz + CL_F32(pod, 0x24);
		dir[0] = CL_F32(pl, PL_X) - CL_F32(pod, 0x10);
		dir[2] = CL_F32(pl, PL_Z) - CL_F32(pod, 0x14);
		CLVec3_Normalize(dir);
		d = 70.0f - d;
		CL_F32(pl, 0x648) = 128.0f;
		CL_F32(pl, PL_X) = dir[0] * d + CL_F32(pl, PL_X);
		CL_F32(pl, PL_Z) = d * dir[2] + CL_F32(pl, PL_Z);
		CL_F32(pl, 0x5c) = CL_F32(pl, 0x5c) - dx;
		CL_F32(pl, 0x60) = CL_F32(pl, 0x60) - dz;
	}
	return -1;
}

/* 0x000e7120  Clusteroids (gap)
Free pods within 70 units of `pod` exchange half their relative speed. */
void ClustPods_Collide(int *pod)
{
	char *self = (char *)pod;
	char *other;
	float dx;
	float dz;
	float d;
	float rel;
	float inv;

	for (other = g_FreePods; other != NULL; other = CL_PTR(other, 0)) {
		if (self == other)
			continue;
		if (other != NULL && other == CL_PTR(other, 0))
			bkPrintf("error\n");
		if (CL_I16(self, 0x38) != 0 || CL_I16(other, 0x38) != 0)
			continue;
		dx = CL_F32(self, 0x10) - CL_F32(other, 0x10);
		dz = CL_F32(self, 0x14) - CL_F32(other, 0x14);
		d = sqrtf(dx * dx + dz * dz);
		if (!(d <= 70.0f))
			continue;
		rel = sqrtf((CL_F32(other, 0x20) - CL_F32(self, 0x20)) * (CL_F32(other, 0x20) - CL_F32(self, 0x20)) +
			(CL_F32(other, 0x24) - CL_F32(self, 0x24)) * (CL_F32(other, 0x24) - CL_F32(self, 0x24))) * 0.5f;
		inv = 1.0f / (fabsf(dx) + fabsf(dz));
		dx = inv * rel * dx;
		CL_F32(self, 0x20) = dx + CL_F32(self, 0x20);
		dz = inv * rel * dz;
		CL_F32(self, 0x24) = dz + CL_F32(self, 0x24);
		CL_F32(other, 0x20) = CL_F32(other, 0x20) - dx;
		CL_F32(other, 0x24) = CL_F32(other, 0x24) - dz;
	}
}

/* 0x000e7210  Clusteroids (gap)
Ships within 70 units push apart and exchange a quarter of their relative speed; a
sound and rumble on the first bump (+0x680 delay). */
void ClustPlayers_Collide(void)
{
	char *a;
	char *b;
	float dir[4];
	float dx;
	float dz;
	float d;
	float vax, vbx, vaz, vbz;
	float rel;
	float inv;
	short i;
	short k;

	for (i = 0; i < 4; i++) {
		a = CL_PLAYER(i);
		for (k = 0; k < 4; k++) {
			b = CL_PLAYER(k);
			if (i == k || CL_I16(a, PL_OUT) != 0 || CL_I16(b, PL_OUT) != 0)
				continue;
			dx = CL_F32(a, PL_X) - CL_F32(b, PL_X);
			dz = CL_F32(a, PL_Z) - CL_F32(b, PL_Z);
			d = sqrtf(dx * dx + dz * dz);
			if (!(d <= 70.0f))
				continue;
			dir[1] = 0.0f;
			dir[3] = 0.0f;
			vax = CL_F32(a, 0x5c);
			vbx = CL_F32(b, 0x5c);
			vaz = CL_F32(a, 0x60);
			vbz = CL_F32(b, 0x60);
			dir[0] = dx;
			dir[2] = dz;
			CLVec3_Normalize(dir);
			d = 70.0f - d;
			CL_F32(a, PL_X) = dir[0] * d + CL_F32(a, PL_X);
			CL_F32(a, PL_Z) = d * dir[2] + CL_F32(a, PL_Z);
			rel = sqrtf((vax - vbx) * (vax - vbx) + (vaz - vbz) * (vaz - vbz)) * 0.25f;
			inv = 1.0f / (fabsf(dx) + fabsf(dz));
			dx = inv * rel * dx;
			CL_F32(a, 0x5c) = dx + CL_F32(a, 0x5c);
			dz = inv * rel * dz;
			CL_F32(a, 0x60) = dz + CL_F32(a, 0x60);
			CL_F32(b, 0x5c) = CL_F32(b, 0x5c) - dx;
			CL_F32(b, 0x60) = CL_F32(b, 0x60) - dz;
			CL_F32(a, 0x648) = 255.0f;
			CL_F32(b, 0x648) = 255.0f;
			if (CL_F32(a, 0x680) == 0.0f) {
				ClustSound_Play(6, 100);
				CL_F32(a, 0x680) = 100.0f;
				ClustPlayer_Rumble(i, 0);
			}
		}
	}
}

/* a bullet hit a pod of player's chain: the pod is pushed, sparks, sound, rumble, voice */
static void cl_chain_pod_hit(char *bullet, char *pod, int player)
{
	float pos[4];

	CL_F32(bullet, 0x18) = 0.0f;
	CL_F32(pod, 0x20) = CL_F32(bullet, 0xc) * 0.5f + CL_F32(pod, 0x20);
	CL_F32(pod, 0x24) = CL_F32(bullet, 0x10) * 0.5f + CL_F32(pod, 0x24);
	cl_hit_sparks(CL_F32(pod, 0x10), CL_F32(pod, 0x14));
	if (CL_I32(pod, 0x40) == 0) {
		ClustSound_Play(1, 100);
		CL_I32(pod, 0x40) = 100;
		ClustPlayer_Rumble(player, 0);
	}
	if (0.8f < (float)cl_rand() * 3.0518509e-05f) {
		pos[0] = CL_F32(CL_PLAYER(player), PL_X);
		pos[2] = CL_F32(CL_PLAYER(player), PL_Z);
		pos[1] = 0.0f;
		pos[3] = 0.0f;
		ClustSound_PlayVoice(player, 0xc);
	}
}

/* 0x000e73f0  Clusteroids (gap)
Bullet hits: a free pod is pushed; another ship is pushed (sparks, voice, sound,
rumble); a pod of another ship's chain is hit: the last pod of that chain comes loose
(ClustPods_Release; its owner +0xa becomes the shooter), the ship cannot pick up pods for
60 frames (+0x664). */
void ClustBullets_HitTest(void)
{
	char *bullet;
	char *pod;
	char *pl;
	char *link;
	char *prev;
	float dx;
	float dz;
	float pos[4];
	short player;
	int hit;
	int i;

	for (i = 0; i < CL_BULLET_COUNT; i++) {
		bullet = CL_BULLET(i);
		if (CL_F32(bullet, 0x18) == 0.0f)
			continue;
		/* free pods */
		for (pod = g_FreePods; pod != NULL; pod = CL_PTR(pod, 0)) {
			dx = CL_F32(bullet, 0x4) - CL_F32(pod, 0x10);
			dz = CL_F32(bullet, 0x8) - CL_F32(pod, 0x14);
			if (CL_I16(pod, 0x38) != 0)
				continue;
			if (2450.0f <= dx * dx + dz * dz) {
				CL_I32(pod, 0x40) = 0;
			} else {
				CL_F32(bullet, 0x18) = 0.0f;
				cl_hit_sparks(CL_F32(pod, 0x10), CL_F32(pod, 0x14));
				CL_F32(pod, 0x20) = CL_F32(bullet, 0xc) + CL_F32(pod, 0x20);
				CL_F32(pod, 0x24) = CL_F32(bullet, 0x10) + CL_F32(pod, 0x24);
				if (CL_I32(pod, 0x40) == 0) {
					ClustSound_Play(1, 100);
					CL_I32(pod, 0x40) = 100;
				}
			}
		}
		/* ships */
		for (player = 0; player < 4; player++) {
			pl = CL_PLAYER(player);
			if (player == CL_I16(bullet, 0x14) || CL_F32(bullet, 0x18) == 0.0f)
				continue;
			dx = CL_F32(bullet, 0x4) - CL_F32(pl, PL_X);
			dz = CL_F32(bullet, 0x8) - CL_F32(pl, PL_Z);
			if (!(dx * dx + dz * dz < 1295.0f))
				continue;
			CL_F32(pl, 0x5c) = CL_F32(bullet, 0xc) + CL_F32(pl, 0x5c);
			hit = cl_rand();
			CL_F32(pl, 0x60) = CL_F32(bullet, 0x10) + CL_F32(pl, 0x60);
			CL_F32(bullet, 0x18) = 0.0f;
			CL_F32(pl, 0x648) = 255.0f;
			if (0.8f < (float)hit * 3.0518509e-05f) {
				pos[0] = CL_F32(pl, PL_X);
				pos[2] = CL_F32(pl, PL_Z);
				pos[1] = 0.0f;
				pos[3] = 0.0f;
				ClustSound_PlayVoice(player, 2);
			}
			cl_hit_sparks(CL_F32(pl, PL_X), CL_F32(pl, PL_Z));
			if (CL_F32(pl, 0x680) == 0.0f) {
				ClustSound_Play(6, 100);
				CL_F32(pl, 0x680) = 100.0f;
				ClustPlayer_Rumble(player, 0);
			}
		}
		/* the ships' pod chains */
		for (player = 0; player < 4; player++) {
			pl = CL_PLAYER(player);
			hit = 0;
			pod = CL_PTR(pl, PL_PODS);
			if (player == CL_I16(bullet, 0x14) || pod == NULL || CL_F32(bullet, 0x18) == 0.0f)
				continue;
			dx = CL_F32(bullet, 0x4) - CL_F32(pod, 0x10);
			dz = CL_F32(bullet, 0x8) - CL_F32(pod, 0x14);
			if (2450.0f <= dx * dx + dz * dz) {
				CL_I32(pod, 0x40) = 0;
			} else {
				hit = CL_PTR(pod, 0) != NULL;
				if (!hit)
					CL_I16(pod, 0xa) = CL_I16(bullet, 0x14);
				ClustPods_Release((int *)pod, NULL, player);
				CL_F32(pl, 0x664) = 60.0f;
				cl_chain_pod_hit(bullet, pod, player);
			}
			prev = pod;
			link = CL_PTR(pod, 0);
			while (link != NULL && CL_F32(bullet, 0x18) != 0.0f) {
				dx = CL_F32(bullet, 0x4) - CL_F32(link, 0x10);
				dz = CL_F32(bullet, 0x8) - CL_F32(link, 0x14);
				if (2450.0f <= dx * dx + dz * dz) {
					CL_I32(link, 0x40) = 0;
				} else {
					if (CL_PTR(link, 0) == NULL)
						CL_I16(link, 0xa) = CL_I16(bullet, 0x14);
					else
						hit = 1;
					ClustPods_Release((int *)link, (int *)prev, player);
					cl_chain_pod_hit(bullet, link, player);
				}
				if (CL_PTR(link, 0) == NULL && hit)
					CL_I16(link, 0xa) = CL_I16(bullet, 0x14);
				prev = link;
				link = CL_PTR(link, 0);
			}
		}
	}
}

/* 0x000e7a40  Clusteroids (gap)
The floor height at (x, z): the Tails spawn points raise (negative strength) or sink it
by 500 / distance (scaled by 0x002f7d58), each between -400 and 200. */
float ClustArena_GetHeight(float x, float z)
{
	double height = 0.0;
	double dx;
	double dz;
	double d;
	double v;
	float *point;
	unsigned int k;

	if (0 < g_SpawnCount) {
		point = g_SpawnPoints;
		for (k = (unsigned short)g_SpawnCount; k != 0; k--) {
			dx = ((double)x - (double)point[0]) * (1.0 / (double)CL_VAR(float, 0x002f7d58));
			dz = ((double)z - (double)point[1]) * (1.0 / (double)CL_VAR(float, 0x002f7d58));
			d = sqrt(dx * dx + dz * dz);
			if (d == 0.0)
				d = 1.0;
			v = -((500.0 / d) * (double)point[8]);
			if (-400.0 <= v) {
				if (200.0 < v)
					v = 200.0;
			} else {
				v = -400.0;
			}
			point = point + 0xd;
			height = v + height;
		}
	}
	return (float)height;
}
