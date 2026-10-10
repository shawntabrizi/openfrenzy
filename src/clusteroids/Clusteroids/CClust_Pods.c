/*
Clusteroids/CClust_Pods.c  <-  C:\BlitzParty\Game\Src\Clusteroids\CClust_Pods.cpp

The pods ("clusteroids"): 60 records of 0x80 bytes (0x002fedf8, allocated by 0x000e4b50,
tagged with CClust_Pods.cpp) in two linked lists: the free pods (0x002fedfc) and each
player's chain (player +0x674). Collector also has 59 "scenic" pods (0x002fee00) that fly
from a delivered chain into the gate. 0x000e3150-0x000e57d0 (gap) around the tagged function
work on the same records; their file is inferred.

Pod record:
  +0x00 next              +0x04 float attached time (Tails: permanent after 0x002f7d64)
  +0x08 short popped      +0x0a short owner (model)    +0x0c short no-pickup frames
  +0x10 x  +0x14 z        +0x18 old x  +0x1c old z     +0x20 vx  +0x24 vz
  +0x28 roll  +0x2c heading  +0x30 speed               +0x38 short inactive
  +0x3c float brightness (fades in to 256)             +0x40 int hit sound delay
  +0x44 ?                 +0x7c the respawn debris piece
Scenic pod: +0x0a short model, +0x10/+0x14 start, +0x18/+0x1c old, +0x28 roll,
  +0x2c heading, +0x30 speed, +0x3a short active, +0x48 short falling, +0x4c-0x54 position,
  +0x58-0x68 target, +0x6c-0x74 velocity, +0x78.
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define g_Pods CL_VAR(char *, 0x002fedf8)
#define g_FreePods CL_VAR(char *, 0x002fedfc)
#define g_ScenicPods CL_VAR(char *, 0x002fee00)
#define g_PodCount CL_VAR(short, 0x002f7f6c)
#define CL_POD(i) (g_Pods + (i) * 0x80)
#define CL_NEXT(pod) CL_AT(pod, 0, char *)

/* the arena radius minus the ship radius (35) */
#define CL_ARENA_INNER (CL_VAR(float, 0x002f7f68) - 35.0f)

/* the character colour of player i as 0x00rrggbb */
static unsigned int cl_player_colour(int player)
{
	int character = DAT_002baed4[player * 10] * 0x60;

	return ((unsigned int)DAT_00290718[character] << 16) |
		((unsigned int)DAT_00290718[character + 1] << 8) | DAT_00290718[character + 2];
}

/* the score display of a pod count: count / 4 for every player */
static void cl_set_pod_quarters(void)
{
	short quarter = (short)(g_PodCount / 4);

	CL_I16(CL_PLAYER(0), 0x652) = quarter;
	CL_I16(CL_PLAYER(1), 0x652) = quarter;
	CL_I16(CL_PLAYER(2), 0x652) = quarter;
	CL_I16(CL_PLAYER(3), 0x652) = quarter;
}

/* 0x000e3150  Clusteroids (gap; CClust_Pods.cpp inferred)
The clear leader: the best player when the lead over the second is at least 10 (Mines),
any lead (Collector) or at least 30 (others); otherwise -1. */
int ClustPods_GetLeader(void)
{
	short best = -1;
	short bestPlayer = 0;
	short lowest = 99;
	short lowestPlayer = 0;
	short second;
	short other;
	short score;
	short i;

	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0) {
			score = CL_I16(CL_PLAYER(i), PL_SCORE);
			if (best < score) {
				best = score;
				bestPlayer = i;
			}
			if (score < lowest) {
				lowest = score;
				lowestPlayer = i;
			}
		}
	}
	second = -1;
	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0 && bestPlayer != i && second < CL_I16(CL_PLAYER(i), PL_SCORE))
			second = CL_I16(CL_PLAYER(i), PL_SCORE);
	}
	/* the original also finds the second lowest score; the result is not used */
	other = 99;
	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0 && lowestPlayer != i && CL_I16(CL_PLAYER(i), PL_SCORE) < other)
			other = CL_I16(CL_PLAYER(i), PL_SCORE);
	}
	(void)other;
	if (g_ClVariant == 3) {
		if (9 < (int)best - (int)second)
			return (int)bestPlayer;
	} else if (g_ClVariant == 4) {
		if (second != best)
			return (int)bestPlayer;
	} else if (0x1d < (int)best - (int)second) {
		return (int)bestPlayer;
	}
	return -1;
}

/* 0x000e3260  Clusteroids (gap; CClust_Pods.cpp inferred)
Collector: a scenic pod from `pod`'s place flies to the gate (velocity 5 towards the
origin, 0 high). */
void ClustPods_AddScenic(char *pod, short model)
{
	char *scenic;
	float dir[4];
	int i;

	for (i = 0; i < 0x3b; i++) {
		scenic = g_ScenicPods + i * 0x80;
		if (CL_I16(scenic, 0x3a) == 0)
			break;
	}
	if (i == 0x3b) {
		bkPrintf("No more scenics pods available\n");
		return;
	}
	CL_U32(scenic, 0x10) = CL_U32(pod, 0x10);
	CL_U32(scenic, 0x14) = CL_U32(pod, 0x14);
	CL_I16(scenic, 0xa) = model;
	CL_I16(scenic, 0x3a) = 1;
	CL_U32(scenic, 0x4c) = CL_U32(pod, 0x10);
	CL_F32(scenic, 0x50) = ClustArena_GetHeight(CL_F32(pod, 0x10), CL_F32(pod, 0x14));
	dir[0] = -CL_F32(scenic, 0x4c);
	CL_U32(scenic, 0x54) = CL_U32(pod, 0x14);
	CL_F32(scenic, 0x60) = 0.0f;
	CL_F32(scenic, 0x64) = 0.0f;
	CL_F32(scenic, 0x58) = 0.0f;
	CL_F32(scenic, 0x5c) = 0.0f;
	CL_F32(scenic, 0x68) = 0.0f;
	dir[1] = CL_F32(scenic, 0x60) - CL_F32(scenic, 0x50);
	dir[2] = CL_F32(scenic, 0x64) - CL_F32(scenic, 0x54);
	dir[3] = 0.0f;
	CLVec3_Normalize(dir);
	CL_F32(scenic, 0x6c) = dir[0] * 5.0f;
	CL_F32(scenic, 0x70) = dir[1] * 5.0f;
	CL_F32(scenic, 0x78) = 0.0f;
	dir[2] = dir[2] * 5.0f;
	CL_I16(scenic, 0x48) = 0;
	CL_F32(scenic, 0x74) = dir[2];
}

/* 0x000e3350  Clusteroids (gap; CClust_Pods.cpp inferred)
A bullet hit `pod` in player's chain (`prev` the pod before it, NULL for the first):
the end of the chain from it goes back to the free pods. Tails keeps pods that became
permanent. */
void ClustPods_Release(int *pod, int *prev, short player)
{
	int *last;
	int *head;
	short count;
	short i;

	count = 1;
	for (last = (int *)*pod; last != NULL; last = (int *)*last)
		count = count + 1;
	if (1 < count) {
		i = 0;
		if (prev == NULL) {
			for (; i < count - 1; i++) {
				prev = pod;
				pod = (int *)*prev;
			}
		} else {
			for (; i < count - 1; i++) {
				pod = (int *)*pod;
				prev = (int *)*prev;
			}
		}
	}
	if (g_FreePods == NULL) {
		if (g_ClVariant != 1 || cl_bitsf((unsigned int)pod[1]) <= CL_VAR(float, 0x002f7d64)) {
			g_FreePods = (char *)pod;
			if (prev == NULL) {
				CL_PTR(CL_PLAYER(player), PL_PODS) = NULL;
				return;
			}
			*prev = 0;
		}
	} else if (g_ClVariant != 1 || cl_bitsf((unsigned int)pod[1]) <= CL_VAR(float, 0x002f7d64)) {
		head = (int *)*(int **)g_FreePods;
		*(int **)g_FreePods = pod;
		last = pod;
		while (*last != 0)
			last = (int *)*last;
		*last = (int)head;
		if (prev != NULL) {
			*prev = 0;
			return;
		}
		CL_PTR(CL_PLAYER(player), PL_PODS) = NULL;
	}
}

/* 0x000e3440  Clusteroids (gap; CClust_Pods.cpp inferred)
The distance from `pod` to the nearest other free pod (9999999 if none). */
float ClustPods_NearestFreePod(char *pod)
{
	char *other;
	float a[4];
	float b[4];
	float distance;
	float best = 9999999.0f;

	for (other = g_FreePods; other != NULL; other = CL_NEXT(other)) {
		if (CL_I16(other, 0x38) != 0 || pod == other)
			continue;
		a[0] = CL_F32(pod, 0x10);
		a[2] = CL_F32(pod, 0x14);
		a[1] = 0.0f;
		a[3] = 0.0f;
		b[0] = CL_F32(other, 0x10);
		b[1] = 0.0f;
		b[2] = CL_F32(other, 0x14);
		b[3] = 0.0f;
		distance = ClustVec3_Distance(a, b);
		if (distance < best)
			best = distance;
	}
	return best;
}

/* 0x000e34e0  Clusteroids (gap; CClust_Pods.cpp inferred)
The distance from `pod` to the nearest ship. */
float ClustPods_NearestPlayer(char *pod)
{
	float a[4];
	float b[4];
	float distance;
	float best = 9999999.0f;
	int i;

	for (i = 0; i < 4; i++) {
		a[2] = CL_F32(CL_PLAYER(i), PL_Z);
		b[0] = CL_F32(pod, 0x10);
		a[0] = CL_F32(CL_PLAYER(i), PL_X);
		b[2] = CL_F32(pod, 0x14);
		a[1] = 0.0f;
		a[3] = 0.0f;
		b[1] = 0.0f;
		b[3] = 0.0f;
		distance = ClustVec3_Distance(a, b);
		if (distance < best)
			best = distance;
	}
	return best;
}

/* a random place for `pod` between 290 units and the arena edge, at least 70 units from
   the other free pods */
static void cl_place_pod(char *pod)
{
	float angle;
	float radius;
	int r;

	do {
		r = cl_rand();
		angle = (float)r * 1.9175345e-4f;
		r = cl_rand();
		radius = (CL_ARENA_INNER - 290.0f) * (float)r * 3.0518509e-05f + 290.0f;
		CL_F32(pod, 0x10) = sinf(angle) * radius;
		CL_F32(pod, 0x14) = cosf(angle) * radius;
	} while (ClustPods_NearestFreePod(pod) <= 70.0f);
}

/* a random drift for `pod` (vx, vz in -5..5) */
static void cl_pod_drift(char *pod)
{
	int r1 = cl_rand();
	int r2 = cl_rand();

	CL_F32(pod, 0x20) = ((float)r1 - 16383.5f) * 0.000305185094f;
	CL_F32(pod, 0x24) = ((float)r2 - 16383.5f) * 0.000305185094f;
}

/* 0x000e3570  Clusteroids (gap; CClust_Pods.cpp inferred)
Collector: links the pods, clears the scenic pods and places 0x002f7f6c pods (models
0-3 in turn), all fading in. */
void ClustPods_InitCollector(void)
{
	char *pod;
	int i;

	cl_set_pod_quarters();
	for (i = 0; i < 59; i++)
		CL_NEXT(CL_POD(i)) = CL_POD(i + 1);
	CL_NEXT(CL_POD(59)) = NULL;
	for (i = 0; i < 60; i++) {
		CL_I16(CL_POD(i), 0x38) = 1;
		CL_I16(g_ScenicPods + i * 0x80, 0x3a) = 0;
		CL_U32(CL_POD(i), 0x7c) = 0;
	}
	g_FreePods = g_Pods;
	for (i = 0; i < g_PodCount; i++) {
		pod = CL_POD(i);
		cl_place_pod(pod);
		CL_I16(pod, 0xa) = (short)(i & 3);
		cl_pod_drift(pod);
		CL_U32(pod, 0x40) = 0;
		CL_I16(pod, 0x38) = 0;
		CL_F32(pod, 0x3c) = 256.0f;
	}
}

/* 0x000e37a0  Clusteroids (gap; CClust_Pods.cpp inferred)
The other variants: links the pods and places 0x002f7f6c of them. */
void ClustPods_Reset(void)
{
	char *pod;
	int i;

	cl_set_pod_quarters();
	for (i = 0; i < 59; i++) {
		CL_NEXT(CL_POD(i)) = CL_POD(i + 1);
		CL_I16(CL_POD(i), 0x38) = 1;
	}
	CL_NEXT(CL_POD(59)) = NULL;
	CL_I16(CL_POD(59), 0x38) = 1;
	g_FreePods = g_Pods;
	for (i = 0; i < g_PodCount; i++) {
		pod = CL_POD(i);
		cl_place_pod(pod);
		cl_pod_drift(pod);
		CL_F32(pod, 0x4) = 0.0f;
		CL_I16(pod, 0x8) = 0;
		CL_I16(pod, 0x38) = 0;
		CL_F32(pod, 0x3c) = 256.0f;
		CL_U32(pod, 0x7c) = 0;
		CL_U32(pod, 0x40) = 0;
	}
}

/* 0x000e39d0  Clusteroids (gap; CClust_Pods.cpp inferred)
Tails: five more pods (at most 60) appear with a "RESPAWN" effect, away from the other
pods and the ships. */
int ClustPods_SpawnTailsPods(void)
{
	char *pod;
	float pos[4];
	float angle;
	float radius;
	int first;
	int r;
	int i;

	first = g_PodCount;
	if (0x3c < first + 5)
		return 0;
	g_PodCount = g_PodCount + 5;
	cl_set_pod_quarters();
	for (i = first; i < g_PodCount; i++) {
		pod = CL_POD(i);
		do {
			do {
				r = cl_rand();
				angle = (float)r * 1.9175345e-4f;
				r = cl_rand();
				radius = CL_ARENA_INNER * (float)r * 3.0518509e-05f;
				CL_F32(pod, 0x10) = sinf(angle) * radius;
				CL_F32(pod, 0x14) = cosf(angle) * radius;
			} while (ClustPods_NearestFreePod(pod) <= 70.0f);
		} while (ClustPods_NearestPlayer(pod) <= 70.0f);
		cl_pod_drift(pod);
		CL_F32(pod, 0x4) = 0.0f;
		CL_I16(pod, 0x8) = 0;
		CL_I16(pod, 0x38) = 0;
		CL_F32(pod, 0x3c) = 0.0f;
		CL_U32(pod, 0x7c) = 0;
		pos[0] = CL_F32(pod, 0x10);
		pos[1] = 12.0f;
		pos[2] = CL_F32(pod, 0x14);
		pos[3] = 0.0f;
		CL_PTR(pod, 0x7c) = Debris_Add((struct DebrisType *)CL_ADDR(0x00296bc8), pos, CL_FP(pod, 0x6c), 0,
			0xffffffff);
	}
	ClustSound_Play(0x12, 100);
	return 1;
}

/* 0x000e3c20  Clusteroids (gap; CClust_Pods.cpp inferred)
The heading of the direction (x, z): atan2 in the game's convention. */
float ClustPods_Heading(float x, float z)
{
	if (z == 0.0f && x == 0.0f)
		return 0.0f;
	if (0.0f < z) {
		if (0.0f < x)
			return (float)atan2((double)x / (double)z, 1.0);
		return (float)-atan2(-((double)x / (double)z), 1.0);
	}
	if (0.0f <= z) {
		if (0.0f < x)
			return 1.5707963f;
		return -1.57079625f;
	}
	if (0.0f < x)
		return (float)(atan2(-((double)z / (double)x), 1.0) + (double)1.5707963f);
	if (x < 0.0f)
		return (float)((double)-1.57079625f - atan2((double)z / (double)x, 1.0));
	return 3.1415925f;
}

/* 0x000e3d00  Clusteroids (gap; CClust_Pods.cpp inferred)
Player picks up `pod` (removed from the free list by the caller): it goes to the end of
the player's chain, sometimes with a voice line. */
void ClustPods_Attach(short player, char *pod)
{
	char *last;
	float pos[4];
	int i;

	CL_PTR(pod, 0) = NULL;
	CL_F32(pod, 0x4) = 0.0f;
	CL_I16(pod, 0x8) = 0;
	CL_F32(pod, 0x20) = 0.0f;
	CL_F32(pod, 0x24) = 0.0f;
	if (CL_PTR(pod, 0x7c) != NULL) {
		CL_U32(CL_PTR(pod, 0x7c), 0x4) = CL_U32(pod, 0x10);
		CL_U32(CL_PTR(pod, 0x7c), 0x8) = 0;
		CL_U32(CL_PTR(pod, 0x7c), 0xc) = CL_U32(pod, 0x14);
		CL_U32(CL_PTR(pod, 0x7c), 0x10) = 0;
		CL_U32(CL_PTR(pod, 0x7c), 0xa0) = CL_U32(CL_PTR(pod, 0x7c), 0xa0) & 0xffff7fff;
		CL_PTR(pod, 0x7c) = NULL;
	}
	ClustSound_Play(0xc, 100);
	if (0.8f < (float)cl_rand() * 3.0518509e-05f) {
		pos[0] = CL_F32(CL_PLAYER(player), PL_X);
		pos[2] = CL_F32(CL_PLAYER(player), PL_Z);
		pos[1] = 0.0f;
		pos[3] = 0.0f;
		ClustSound_PlayVoice((int)player, 0xb);
	}
	last = CL_PTR(CL_PLAYER(player), PL_PODS);
	if (last == NULL) {
		CL_PTR(CL_PLAYER(player), PL_PODS) = pod;
		CL_U32(pod, 0x44) = 0;
	} else {
		while (CL_NEXT(last) != NULL)
			last = CL_NEXT(last);
		CL_NEXT(last) = pod;
	}
	for (i = 0; i < 4; i++) {
		if (CL_PTR(CL_PLAYER(i), PL_TARGET_POD) == pod)
			CL_PTR(CL_PLAYER(i), PL_TARGET_POD) = NULL;
	}
}

/* 0x000e3e20  Clusteroids (gap; CClust_Pods.cpp inferred)
Collector: the scenic pods fly to the gate, then fall (y below -1500: done). */
void ClustPods_UpdateScenic(void)
{
	char *scenic;
	float *pos;
	float dir[4];
	float origin[4];
	float dx;
	float dz;
	int offset;

	origin[3] = 0.0f;
	origin[2] = 0.0f;
	origin[1] = 0.0f;
	origin[0] = 0.0f;
	for (offset = 0; offset < 0x1d80; offset += 0x80) {
		scenic = g_ScenicPods + offset;
		if (CL_I16(scenic, 0x3a) == 0)
			continue;
		CL_U32(scenic, 0x18) = CL_U32(scenic, 0x4c);
		CL_U32(scenic, 0x1c) = CL_U32(scenic, 0x54);
		pos = CL_FP(scenic, 0x4c);
		pos[0] = g_ClFrameTime * CL_F32(scenic, 0x6c) + pos[0];
		CL_F32(scenic, 0x50) = g_ClFrameTime * CL_F32(scenic, 0x70) + CL_F32(scenic, 0x50);
		CL_F32(scenic, 0x54) = g_ClFrameTime * CL_F32(scenic, 0x74) + CL_F32(scenic, 0x54);
		dx = pos[0] - CL_F32(scenic, 0x18);
		dz = CL_F32(scenic, 0x54) - CL_F32(scenic, 0x1c);
		dx = sqrtf(dx * dx + dz * dz);
		CL_F32(scenic, 0x30) = dx;
		CL_F32(scenic, 0x28) = dx * 0.01f + CL_F32(scenic, 0x28) + 0.01f;
		CL_F32(scenic, 0x2c) = ClustPods_Heading(CL_F32(scenic, 0x6c), CL_F32(scenic, 0x74));
		if (CL_I16(scenic, 0x48) == 0) {
			if (ClustVec3_Distance(origin, pos) < 30.0f) {
				CL_F32(scenic, 0x60) = -500.0f;
				dir[0] = -pos[0];
				CL_F32(scenic, 0x64) = 0.0f;
				CL_F32(scenic, 0x5c) = 0.0f;
				CL_F32(scenic, 0x68) = 0.0f;
				dir[1] = CL_F32(scenic, 0x60) - CL_F32(scenic, 0x50);
				dir[2] = CL_F32(scenic, 0x64) - CL_F32(scenic, 0x54);
				dir[3] = 0.0f;
				CLVec3_Normalize(dir);
				CL_F32(scenic, 0x6c) = dir[0] * 20.0f;
				CL_F32(scenic, 0x70) = dir[1] * 20.0f;
				CL_F32(scenic, 0x78) = 0.0f;
				CL_I16(scenic, 0x48) = 1;
				CL_F32(scenic, 0x74) = dir[2] * 20.0f;
			}
		} else if (CL_F32(scenic, 0x50) < -1500.0f) {
			CL_I16(scenic, 0x3a) = 0;
		}
	}
}

/* a pod popped in Tails: the first time it becomes permanent ("X1" debris, sound) */
static void cl_tails_pop(int player, char *pod, float x)
{
	float pos[4];
	float vel[4];

	pos[0] = x;
	pos[1] = 12.0f;
	pos[2] = CL_F32(pod, 0x14);
	pos[3] = 0.0f;
	vel[0] = 0.0f;
	vel[1] = 6.0f;
	vel[2] = 0.0f;
	vel[3] = 0.0f;
	Debris_Add((struct DebrisType *)CL_ADDR(0x00296dac), pos, vel, 0, cl_player_colour(player));
	CL_I16(pod, 0x8) = 1;
	ClustSound_Play(0x14, 100);
}

/* keeps `pod` inside the arena (bouncing back) */
static void cl_pod_wall(char *pod)
{
	float dir[4];

	if (CL_ARENA_INNER < fabsf(sqrtf(CL_F32(pod, 0x10) * CL_F32(pod, 0x10) +
			CL_F32(pod, 0x14) * CL_F32(pod, 0x14)))) {
		dir[0] = CL_F32(pod, 0x10);
		dir[1] = 0.0f;
		dir[2] = CL_F32(pod, 0x14);
		dir[3] = 0.0f;
		CLVec3_Normalize(dir);
		CL_F32(pod, 0x10) = CL_ARENA_INNER * dir[0];
		CL_F32(pod, 0x14) = CL_ARENA_INNER * dir[2];
		CL_F32(pod, 0x20) = -CL_F32(pod, 0x20);
		CL_F32(pod, 0x24) = -CL_F32(pod, 0x24);
	}
}

/* 0x000e3fb0  Clusteroids (gap; CClust_Pods.cpp inferred)
The players' pod chains: the first pod 66 units from the ship, every next one 60 units from the one before; pods bounce off the arena wall, the
ships and each other; the chain length goes to +0x650. Tails: a pod attached longer than
0x002f7d64 pops (becomes permanent). */
void ClustPods_UpdateHeld(void)
{
	char *pl;
	char *other;
	char *pod;
	char *link;
	char *prev;
	float x, z, dx, dz, d, inv;
	float ox, oz;
	float lastX, lastZ;
	float dir[4];
	float popX;
	short count;
	short player;
	short k;

	popX = 0.0f;
	for (player = 0; player < 4; player++) {
		pl = CL_PLAYER(player);
		pod = CL_PTR(pl, PL_PODS);
		count = 0;
		if (pod != NULL) {
			CL_F32(pod, 0x4) = g_ClFrameTime + CL_F32(pod, 0x4);
			count = 1;
			if (g_ClVariant == 1 && CL_VAR(float, 0x002f7d64) < CL_F32(pod, 0x4) && CL_I16(pod, 0x8) == 0) {
				popX = CL_F32(pod, 0x10);
				cl_tails_pop(player, pod, popX);
				ClustSound_PlayVoice(player, 4);
			}
			x = CL_F32(pod, 0x20) * 0.625f;
			CL_U32(pod, 0x18) = CL_U32(pod, 0x10);
			CL_U32(pod, 0x1c) = CL_U32(pod, 0x14);
			x = x + CL_F32(pod, 0x10);
			CL_F32(pod, 0x10) = x;
			z = CL_F32(pod, 0x24) * 0.625f + CL_F32(pod, 0x14);
			CL_F32(pod, 0x14) = z;
			if (CL_ARENA_INNER < fabsf(sqrtf(z * z + x * x))) {
				dir[0] = x;
				dir[1] = 0.0f;
				dir[2] = CL_F32(pod, 0x14);
				dir[3] = 0.0f;
				CLVec3_Normalize(dir);
				CL_F32(pod, 0x10) = CL_ARENA_INNER * dir[0];
				CL_F32(pod, 0x14) = CL_ARENA_INNER * dir[2];
				CL_F32(pod, 0x20) = -CL_F32(pod, 0x20);
				CL_F32(pod, 0x24) = -CL_F32(pod, 0x24);
			}
			CL_F32(pod, 0x20) = CL_F32(pod, 0x20) - CL_VAR(float, 0x002f7d40) * CL_F32(pod, 0x20);
			CL_F32(pod, 0x24) = CL_F32(pod, 0x24) - CL_VAR(float, 0x002f7d40) * CL_F32(pod, 0x24);
			/* 66 units from the ship */
			ox = CL_F32(pod, 0x10);
			oz = CL_F32(pod, 0x14);
			dx = CL_F32(pod, 0x10) - CL_F32(pl, PL_X);
			dz = CL_F32(pod, 0x14) - CL_F32(pl, PL_Z);
			inv = 1.0f / sqrtf(dx * dx + dz * dz);
			dz = inv * dz * 66.0f;
			CL_F32(pod, 0x10) = inv * dx * 66.0f + CL_F32(pl, PL_X);
			CL_F32(pod, 0x14) = dz + CL_F32(pl, PL_Z);
			CL_F32(pod, 0x20) = CL_F32(pod, 0x20) - (ox - CL_F32(pod, 0x10)) * 0.5f;
			CL_F32(pod, 0x24) = CL_F32(pod, 0x24) - (oz - CL_F32(pod, 0x14)) * 0.5f;
			/* push the other ships away */
			for (k = 0; k < 4; k++) {
				other = CL_PLAYER(k);
				if (k == player)
					continue;
				d = sqrtf((CL_F32(pod, 0x10) - CL_F32(other, PL_X)) * (CL_F32(pod, 0x10) - CL_F32(other, PL_X)) +
					(CL_F32(pod, 0x14) - CL_F32(other, PL_Z)) * (CL_F32(pod, 0x14) - CL_F32(other, PL_Z)));
				if (d < 70.0f) {
					ox = CL_F32(other, PL_X);
					dir[0] = CL_F32(other, PL_X) - CL_F32(pod, 0x10);
					oz = CL_F32(other, PL_Z);
					dir[1] = 0.0f;
					dir[2] = CL_F32(other, PL_Z) - CL_F32(pod, 0x14);
					dir[3] = 0.0f;
					CLVec3_Normalize(dir);
					d = 70.0f - d;
					CL_F32(other, PL_X) = dir[0] * d + CL_F32(other, PL_X);
					CL_F32(other, PL_Z) = d * dir[2] + CL_F32(other, PL_Z);
					CL_F32(pod, 0x20) = CL_F32(pod, 0x20) - (ox - CL_F32(pod, 0x10)) * 0.5f;
					CL_F32(pod, 0x24) = CL_F32(pod, 0x24) - (oz - CL_F32(pod, 0x14)) * 0.5f;
				}
			}
			d = sqrtf((CL_F32(pod, 0x10) - CL_F32(pod, 0x18)) * (CL_F32(pod, 0x10) - CL_F32(pod, 0x18)) +
				(CL_F32(pod, 0x14) - CL_F32(pod, 0x1c)) * (CL_F32(pod, 0x14) - CL_F32(pod, 0x1c)));
			CL_F32(pod, 0x30) = d;
			CL_F32(pod, 0x28) = d * 0.01f + CL_F32(pod, 0x28) + 0.01f;
			CL_F32(pod, 0x2c) = ClustPods_Heading(CL_F32(pod, 0x20), CL_F32(pod, 0x24));

			/* the rest of the chain */
			prev = pod;
			for (link = CL_NEXT(pod); link != NULL; link = CL_NEXT(link)) {
				popX = CL_F32(link, 0x20) + CL_F32(link, 0x10);
				count = count + 1;
				CL_U32(link, 0x18) = CL_U32(link, 0x10);
				CL_U32(link, 0x1c) = CL_U32(link, 0x14);
				CL_F32(link, 0x10) = popX;
				CL_F32(link, 0x14) = CL_F32(link, 0x14) + CL_F32(link, 0x24);
				CL_F32(link, 0x4) = g_ClFrameTime + CL_F32(link, 0x4);
				if (g_ClVariant == 1 && CL_VAR(float, 0x002f7d64) < CL_F32(link, 0x4) && CL_I16(link, 0x8) == 0)
					cl_tails_pop(player, link, popX);
				cl_pod_wall(link);
				lastX = CL_F32(link, 0x10);
				lastZ = CL_F32(link, 0x14);
				CL_F32(link, 0x20) = CL_F32(link, 0x20) - CL_VAR(float, 0x002f7d40) * CL_F32(link, 0x20);
				CL_F32(link, 0x24) = CL_F32(link, 0x24) - CL_VAR(float, 0x002f7d40) * CL_F32(link, 0x24);
				/* 60 units from the pod before */
				dx = CL_F32(link, 0x10) - CL_F32(prev, 0x10);
				dz = CL_F32(link, 0x14) - CL_F32(prev, 0x14);
				inv = 1.0f / sqrtf(dx * dx + dz * dz);
				CL_F32(link, 0x10) = inv * dx * 60.0f + CL_F32(prev, 0x10);
				CL_F32(link, 0x14) = inv * dz * 60.0f + CL_F32(prev, 0x14);
				/* not closer than 60 to any pod before it in the chain */
				for (other = pod; other != link; other = CL_NEXT(other)) {
					dx = CL_F32(link, 0x10) - CL_F32(other, 0x10);
					dz = CL_F32(link, 0x14) - CL_F32(other, 0x14);
					d = sqrtf(dx * dx + dz * dz);
					if (d < 60.0f) {
						inv = 1.0f / d;
						lastX = CL_F32(link, 0x10);
						lastZ = CL_F32(link, 0x14);
						CL_F32(link, 0x10) = inv * dx * 60.0f + CL_F32(other, 0x10);
						CL_F32(link, 0x14) = inv * dz * 60.0f + CL_F32(other, 0x14);
						CL_F32(other, 0x20) = CL_F32(other, 0x20) - (CL_F32(link, 0x10) - lastX) * 0.5f;
						CL_F32(other, 0x24) = CL_F32(other, 0x24) - (CL_F32(link, 0x14) - lastZ) * 0.5f;
					}
				}
				/* the ships */
				for (k = 0; k < 4; k++) {
					other = CL_PLAYER(k);
					dx = CL_F32(link, 0x10) - CL_F32(other, PL_X);
					dz = CL_F32(link, 0x14) - CL_F32(other, PL_Z);
					d = sqrtf(dx * dx + dz * dz);
					if (d < 70.0f) {
						inv = 1.0f / d;
						ox = CL_F32(link, 0x10);
						oz = CL_F32(link, 0x14);
						CL_F32(link, 0x10) = inv * dx * 70.0f + CL_F32(other, PL_X);
						CL_F32(link, 0x14) = inv * dz * 70.0f + CL_F32(other, PL_Z);
						if (k != player) {
							dir[0] = CL_F32(other, PL_X) - CL_F32(link, 0x10);
							dir[1] = 0.0f;
							dir[2] = CL_F32(other, PL_Z) - CL_F32(link, 0x14);
							dir[3] = 0.0f;
							CLVec3_Normalize(dir);
							d = 70.0f - d;
							CL_F32(other, PL_X) = dir[0] * d + CL_F32(other, PL_X);
							CL_F32(other, PL_Z) = d * dir[2] + CL_F32(other, PL_Z);
						}
						CL_F32(link, 0x20) = CL_F32(link, 0x20) - (CL_F32(link, 0x10) - ox) * 0.5f;
						CL_F32(link, 0x24) = CL_F32(link, 0x24) - (CL_F32(link, 0x14) - oz) * 0.5f;
					}
				}
				x = CL_F32(link, 0x20) - (lastX - CL_F32(link, 0x10)) * 0.5f;
				CL_F32(link, 0x20) = x;
				z = CL_F32(link, 0x24) - (lastZ - CL_F32(link, 0x14)) * 0.5f;
				CL_F32(link, 0x24) = z;
				d = sqrtf((CL_F32(link, 0x10) - CL_F32(link, 0x18)) * (CL_F32(link, 0x10) - CL_F32(link, 0x18)) +
					(CL_F32(link, 0x14) - CL_F32(link, 0x1c)) * (CL_F32(link, 0x14) - CL_F32(link, 0x1c)));
				CL_F32(link, 0x30) = d;
				CL_F32(link, 0x28) = d * 0.01f + CL_F32(link, 0x28) + 0.01f;
				CL_F32(link, 0x2c) = ClustPods_Heading(x, z);
				prev = link;
			}
		}
		CL_I16(pl, 0x650) = count;
	}
}

/* 0x000e4990  Clusteroids (gap; CClust_Pods.cpp inferred)
A delivered pod reappears somewhere else (Collector: between 125 units and the edge)
with a "RESPAWN" effect; no pick-up for 10 frames. */
void ClustPods_Respawn(char *pod)
{
	float pos[4];
	float angle;
	float radius;
	int r;

	do {
		do {
			r = cl_rand();
			angle = (float)r * 1.9175345e-4f;
			r = cl_rand();
			if (g_ClVariant == 4)
				radius = (float)(((double)CL_VAR(float, 0x002f7f68) - 160.0) * (double)r *
					(double)3.0518509e-05f + 125.0);
			else
				radius = (float)(((double)CL_VAR(float, 0x002f7f68) - 35.0) * (double)r *
					(double)3.0518509e-05f);
			CL_F32(pod, 0x10) = sinf(angle) * radius;
			CL_F32(pod, 0x14) = cosf(angle) * radius;
		} while (ClustPods_NearestFreePod(pod) <= 70.0f);
	} while (ClustPods_NearestPlayer(pod) <= 70.0f);
	CL_I16(pod, 0xc) = 10;
	pos[0] = CL_F32(pod, 0x10);
	pos[2] = CL_F32(pod, 0x14);
	pos[1] = 12.0f;
	pos[3] = 0.0f;
	cl_pod_drift(pod);
	/* the original passes the velocity of pod 10 (0x002fedf8 + 0x56c) */
	CL_PTR(pod, 0x7c) = Debris_Add((struct DebrisType *)CL_ADDR(0x00296bc8), pos, (float *)(g_Pods + 0x56c), 0,
		0xffffffff);
	ClustSound_Play(0x12, 100);
}

/* 0x000e4b50  Clusteroids/CClust_Pods.cpp */
void ClustPods_Init(void)
{
	g_Pods = (char *)CL_NEW(0x1e00, CL_FILE_PODS, 0x21, 0x2001);
	g_ScenicPods = (char *)CL_NEW(0x1e00, CL_FILE_PODS, 0x22, 0x2001);
	memset(g_Pods, 0, 0x780 * 4);
	memset(g_ScenicPods, 0, 0x780 * 4);
	CL_VAR(int, 0x002fedf4) = 0;
	g_FreePods = g_Pods;
	if (g_ClVariant == 4) {
		ClustPods_InitCollector();
		return;
	}
	ClustPods_Reset();
}

/* the free pods' movement shared by ClustPods_UpdateCarried / ClustPods_UpdateFree */
static void cl_move_free_pod(char *pod, float maxSpeedSq, float maxSpeed)
{
	float speed;
	float x;
	float z;
	float d;
	char *piece;

	CL_U32(pod, 0x18) = CL_U32(pod, 0x10);
	CL_U32(pod, 0x1c) = CL_U32(pod, 0x14);
	speed = CL_F32(pod, 0x24) * CL_F32(pod, 0x24) + CL_F32(pod, 0x20) * CL_F32(pod, 0x20);
	if (maxSpeedSq < speed) {
		speed = 1.0f / sqrtf(speed);
		CL_F32(pod, 0x20) = CL_F32(pod, 0x20) * maxSpeed * speed;
		CL_F32(pod, 0x24) = CL_F32(pod, 0x24) * maxSpeed * speed;
	}
	x = CL_F32(pod, 0x20) * 0.625f + CL_F32(pod, 0x10);
	CL_F32(pod, 0x10) = x;
	z = CL_F32(pod, 0x24) * 0.625f + CL_F32(pod, 0x14);
	CL_F32(pod, 0x14) = z;
	cl_pod_wall(pod);
	ClustPods_Collide((int *)pod);
	cl_pod_wall(pod);
	d = sqrtf((CL_F32(pod, 0x10) - CL_F32(pod, 0x18)) * (CL_F32(pod, 0x10) - CL_F32(pod, 0x18)) +
		(CL_F32(pod, 0x14) - CL_F32(pod, 0x1c)) * (CL_F32(pod, 0x14) - CL_F32(pod, 0x1c)));
	CL_F32(pod, 0x30) = d;
	CL_F32(pod, 0x28) = d * 0.01f + CL_F32(pod, 0x28) + 0.01f;
}

/* the respawn effect follows the pod until it ends; the pod fades in */
static void cl_pod_effect(char *pod)
{
	char *piece = CL_PTR(pod, 0x7c);
	float brightness;

	if (piece != NULL) {
		CL_U32(piece, 0x4) = CL_U32(pod, 0x10);
		CL_U32(piece, 0x8) = 0;
		CL_U32(piece, 0xc) = CL_U32(pod, 0x14);
		CL_U32(piece, 0x10) = 0;
		if (CL_I32(piece, 0x9c) == 1 && CL_F32(piece, 0x7c) <= 0.0f) {
			Debris_StopPiece((struct DebrisPiece *)piece);
			CL_PTR(pod, 0x7c) = NULL;
		}
	}
	brightness = (float)((double)g_ClFrameTime * 7.5 + (double)CL_F32(pod, 0x3c));
	CL_F32(pod, 0x3c) = brightness;
	if (256.0f < brightness)
		CL_F32(pod, 0x3c) = 256.0f;
}

/* the ships pick up the free pods they touch (unless they were just hit) */
static void cl_pick_up_pods(void)
{
	char **link;
	char *pod;
	short player;

	link = &g_FreePods;
	while (*link != NULL) {
		player = ClustPods_HitPlayer(*link);
		if (player == -1 || CL_F32(CL_PLAYER(player), 0x664) != 0.0f) {
			link = (char **)*link;
		} else {
			pod = *link;
			*link = CL_NEXT(pod);
			ClustPods_Attach(player, pod);
		}
	}
}

/* 0x000e4bd0  Clusteroids (gap; CClust_Pods.cpp inferred)
Collector: the free pods move (speed at most 10), bounce and collide; the ships pick
them up. */
void ClustPods_UpdateCarried(void)
{
	char *pod;

	for (pod = g_FreePods; pod != NULL; pod = CL_NEXT(pod)) {
		if (CL_I16(pod, 0xc) != 0)
			CL_I16(pod, 0xc) = CL_I16(pod, 0xc) - 1;
		cl_move_free_pod(pod, 100.0f, 10.0f);
		CL_F32(pod, 0x2c) = (float)atan2((double)CL_F32(pod, 0x20), (double)CL_F32(pod, 0x24));
		cl_pod_effect(pod);
	}
	cl_pick_up_pods();
}

/* 0x000e4ea0  Clusteroids (gap; CClust_Pods.cpp inferred)
The other variants: the free pods move (speed at most 10; Mines 5) and the ships pick
them up; counts the active free pods (0x002f8f5e). */
void ClustPods_UpdateFree(void)
{
	char *pod;

	for (pod = g_FreePods; pod != NULL; pod = CL_NEXT(pod)) {
		if (CL_I16(pod, 0x38) == 0)
			CL_VAR(short, 0x002f8f5e) = CL_VAR(short, 0x002f8f5e) + 1;
		if (g_ClVariant == 3)
			cl_move_free_pod(pod, 25.0f, 5.0f);
		else
			cl_move_free_pod(pod, 100.0f, 10.0f);
		CL_F32(pod, 0x2c) = ClustPods_Heading(CL_F32(pod, 0x20), CL_F32(pod, 0x24));
		cl_pod_effect(pod);
	}
	cl_pick_up_pods();
}

/* 0x000e51b0  Clusteroids (gap; CClust_Pods.cpp inferred)
Collector: a ship with pods within 125 units of a gate (0x002f8734.., 0x10 apart)
delivers up to three of them: scenic pods fly in, the pods respawn, the delivery count
(+0xc) goes to the scoring. +0x654 counts the frames the ship stays at its own gate. */
void ClustGates_UpdateDeliveries(void)
{
	char *pl;
	char *pod;
	char *next;
	char *head;
	float dx;
	float dz;
	short player;
	short gate;
	short n;

	for (player = 0; player < 4; player++) {
		pl = CL_PLAYER(player);
		if (CL_PTR(pl, PL_PODS) == NULL)
			continue;
		for (gate = 0; gate < 4; gate++) {
			n = 0;
			dx = CL_VAR(float, 0x002f8734 + gate * 0x10) - CL_F32(pl, PL_X);
			dz = CL_VAR(float, 0x002f873c + gate * 0x10) - CL_F32(pl, PL_Z);
			if (15625.0f <= dx * dx + dz * dz) {
				if (player == gate)
					CL_U32(pl, 0x654) = 0;
				continue;
			}
			pod = CL_PTR(pl, PL_PODS);
			if (pod == NULL)
				continue;
			CL_I16(pl, 0xc) = 0;
			do {
				CL_I16(pl, 0xc) = CL_I16(pl, 0xc) + 1;
				ClustPods_AddScenic(pod, player);
				next = CL_NEXT(pod);
				CL_PTR(pl, PL_PODS) = next;
				CL_NEXT(pod) = NULL;
				head = pod;
				if (g_FreePods != NULL) {
					CL_NEXT(pod) = CL_NEXT(g_FreePods);
					CL_NEXT(g_FreePods) = pod;
					head = g_FreePods;
				}
				g_FreePods = head;
				ClustPods_Respawn(pod);
				pod = CL_PTR(pl, PL_PODS);
			} while (pod != NULL && (n = n + 1, n < 3));
			if (player == gate)
				CL_I32(pl, 0x654) = CL_I32(pl, 0x654) + 1;
		}
	}
}

/* 0x000e52f0  Clusteroids (gap; CClust_Pods.cpp inferred)
The first player holding pods, or NULL. */
char *ClustPods_FirstHolder(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (CL_PTR(CL_PLAYER(i), PL_PODS) != NULL)
			return CL_PLAYER(i);
	}
	return NULL;
}

/* the non-permanent pods of a chain (Tails) */
static int cl_count_loose(char *pod)
{
	int count = 0;

	for (; pod != NULL; pod = CL_NEXT(pod)) {
		if (CL_F32(pod, 0x4) < CL_VAR(float, 0x002f7d64))
			count = count + 1;
	}
	return count;
}

/* 0x000e5320  Clusteroids (gap; CClust_Pods.cpp inferred)
The longest chain of loose pods (in *best) and how many players have it. */
int ClustPods_CountLongest(int unused, int *best)
{
	int count;
	int players;
	int i;

	(void)unused;
	*best = -1;
	for (i = 0; i < 4; i++) {
		if (CL_PTR(CL_PLAYER(i), PL_PODS) == NULL)
			continue;
		count = cl_count_loose(CL_PTR(CL_PLAYER(i), PL_PODS));
		if (count != 0 && *best < count)
			*best = count;
	}
	players = 0;
	for (i = 0; i < 4; i++) {
		if (CL_PTR(CL_PLAYER(i), PL_PODS) == NULL)
			continue;
		count = cl_count_loose(CL_PTR(CL_PLAYER(i), PL_PODS));
		if (count != 0 && count == *best)
			players = players + 1;
	}
	return players;
}

/* 0x000e53d0  Clusteroids (gap; CClust_Pods.cpp inferred) */
int ClustPods_CountLoose(int player)
{
	return cl_count_loose(CL_PTR(CL_PLAYER(player), PL_PODS));
}

/* 0x000e5410  Clusteroids (gap; CClust_Pods.cpp inferred) */
int ClustPods_CountLooseOf(char *player)
{
	return cl_count_loose(CL_PTR(player, PL_PODS));
}

/* 0x000e5440  Clusteroids (gap; CClust_Pods.cpp inferred) */
float ClustVec3_Distance(float *a, float *b)
{
	double dx = (double)a[0] - (double)b[0];
	double dy = (double)a[1] - (double)b[1];
	double dz = (double)a[2] - (double)b[2];

	return (float)sqrt(dx * dx + dy * dy + dz * dz);
}

/* 0x000e5480  Clusteroids (gap; CClust_Pods.cpp inferred)
Pod Hog AI: the player to rob: the clear leader if it holds pods (team games: the
leader of the other team), otherwise a random player holding pods; NULL if none. */
char *ClustPods_ChooseVictim(int player)
{
	int holders[4];
	unsigned int count;
	int who;

	who = ClustPods_GetLeader();
	if (who != -1) {
		if (DAT_002baebc == 0) {
			if (CL_PTR(CL_PLAYER(who), PL_PODS) != NULL) {
				if (3 < who || who < 0)
					return NULL;
				return CL_PLAYER(who);
			}
		} else if (DAT_002baef4[player * 10] != DAT_002baef4[who * 10]) {
			goto found;
		}
	}
	count = CL_PTR(CL_PLAYER(0), PL_PODS) != NULL;
	if (count != 0)
		holders[0] = 0;
	if (CL_PTR(CL_PLAYER(1), PL_PODS) != NULL)
		holders[count++] = 1;
	if (CL_PTR(CL_PLAYER(2), PL_PODS) != NULL)
		holders[count++] = 2;
	if (CL_PTR(CL_PLAYER(3), PL_PODS) != NULL)
		holders[count++] = 3;
	if (count == 0)
		return NULL;
	/* the original indexes backwards with a negative product */
	who = holders[-CL_FTOL((double)cl_rand() * (double)count * (double)-3.0518509e-05f)];
found:
	if (who < 4 && -1 < who)
		return CL_PLAYER(who);
	return NULL;
}

/* 0x000e55b0  Clusteroids (gap; CClust_Pods.cpp inferred)
The nearest other ship to `pos` (its record; the distance in *distance), or NULL. */
char *ClustPods_NearestShip(int player, float *pos, float *distance)
{
	char *nearest = NULL;
	float best = 9999999.0f;
	float d;
	int i;

	*distance = 0.0f;
	for (i = 0; i < 4; i++) {
		if (player == i)
			continue;
		d = sqrtf((pos[0] - CL_F32(CL_PLAYER(i), PL_X)) * (pos[0] - CL_F32(CL_PLAYER(i), PL_X)) +
			pos[1] * pos[1] + (pos[2] - CL_F32(CL_PLAYER(i), PL_Z)) * (pos[2] - CL_F32(CL_PLAYER(i), PL_Z)));
		if (d < best) {
			nearest = CL_PLAYER(i);
			*distance = d;
			best = d;
		}
	}
	return nearest;
}

/* 0x000e5640  Clusteroids (gap; CClust_Pods.cpp inferred)
The nearest free pod to player that no other AI is going for (unless fewer than three
free pods are left). */
char *ClustPods_NearestFree(int player)
{
	char *pod;
	char *best = NULL;
	float bestDistance = 9999999.0f;
	float d;
	int taken;
	int i;

	for (pod = g_FreePods; pod != NULL; pod = CL_NEXT(pod)) {
		taken = 0;
		for (i = 0; i < 4; i++) {
			if (player != i && CL_PTR(CL_PLAYER(i), PL_TARGET_POD) == pod) {
				taken = 1;
				break;
			}
		}
		if (CL_I16(pod, 0x38) != 0 || (taken && 2 < CL_VAR(short, 0x002f8f5e)))
			continue;
		d = sqrtf((CL_F32(CL_PLAYER(player), PL_X) - CL_F32(pod, 0x10)) *
				(CL_F32(CL_PLAYER(player), PL_X) - CL_F32(pod, 0x10)) +
			(CL_F32(CL_PLAYER(player), PL_Z) - CL_F32(pod, 0x14)) *
				(CL_F32(CL_PLAYER(player), PL_Z) - CL_F32(pod, 0x14)));
		if (d < bestDistance) {
			best = pod;
			bestDistance = d;
		}
	}
	return best;
}

/* the last ship (within 99999 units) holding pods that `player` can rob; with `humans`
   only human players */
static char *cl_find_robbable(int player, int humans)
{
	char *found = NULL;
	char *pl;
	char *pod;
	int count;
	int i;

	for (i = 0; i < 4; i++) {
		if (player == i || (humans && DAT_002baed8[i * 10] == 0))
			continue;
		pl = CL_PLAYER(i);
		pod = CL_PTR(pl, PL_PODS);
		count = 0;
		if (pod == NULL)
			continue;
		for (; pod != NULL; pod = CL_NEXT(pod)) {
			if (CL_F32(pod, 0x4) < CL_VAR(float, 0x002f7d64) || g_ClVariant == 4)
				count = count + 1;
		}
		if (count != 0 &&
			sqrtf((CL_F32(CL_PLAYER(player), PL_X) - CL_F32(pl, PL_X)) *
					(CL_F32(CL_PLAYER(player), PL_X) - CL_F32(pl, PL_X)) +
				(CL_F32(CL_PLAYER(player), PL_Z) - CL_F32(pl, PL_Z)) *
					(CL_F32(CL_PLAYER(player), PL_Z) - CL_F32(pl, PL_Z))) < 99999.0f)
			found = pl;
	}
	return found;
}

/* 0x000e5700  Clusteroids (gap; CClust_Pods.cpp inferred) */
char *ClustPods_FindRobbable(int player)
{
	return cl_find_robbable(player, 0);
}

/* 0x000e57d0  Clusteroids (gap; CClust_Pods.cpp inferred) */
char *ClustPods_FindRobbableHuman(int player)
{
	return cl_find_robbable(player, 1);
}
