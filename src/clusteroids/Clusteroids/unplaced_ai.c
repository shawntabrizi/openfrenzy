/*
Clusteroids/unplaced_ai.c  -  gap functions of C:\BlitzParty\Game\Src\Clusteroids\ without a
known source file: the computer players (0x000e58b0-0x000e6650, after CClust_Pods.cpp).

Each variant's AI gives the thrust direction (*x, *z, scaled by the AI speed +0x824) and
sets the AI buttons (+0x80c fire, +0x80e boost). Player record fields of the AI:
  +0x808 short state          +0x814 / +0x81c roaming target x, z
  +0x824 speed (x 32768)      +0x828 aim error   +0x82c turn rate
  +0x830 fire rate            +0x834 boost rate  (both 0-1: fire when random > rate)
  +0x838 / +0x840 aim offset  +0x848 the player record to rob   +0x84c short counter
  +0x678 the pod it goes for
Core_RandUnit (core_modes_a) gives a random 0-1.
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define CL_TWO_PI 6.28318501f

extern int DAT_002baef4[];          /* the team of player i: [i * 10] (core) */

/* a new random aim offset (up to 50 x aim error) for a new target pod */
static void cl_new_aim_offset(char *pl)
{
	float angle = Core_RandUnit() * CL_TWO_PI;

	CL_F32(pl, 0x83c) = 0.0f;
	CL_F32(pl, 0x844) = 0.0f;
	CL_F32(pl, 0x838) = cosf(angle) * 50.0f * CL_F32(pl, 0x828);
	CL_F32(pl, 0x840) = sinf(angle) * 50.0f * CL_F32(pl, 0x828);
}

/* the thrust towards (dx, dz) at full AI speed */
static void cl_steer(char *pl, float dx, float dz, float *x, float *z)
{
	float dir[4];

	dir[0] = dx;
	dir[1] = 0.0f;
	dir[2] = dz;
	dir[3] = 0.0f;
	CLVec3_Normalize(dir);
	*x = dir[0] * CL_F32(pl, 0x824);
	*z = dir[2] * CL_F32(pl, 0x824);
}

/* the thrust towards `pod` plus the aim offset */
static void cl_steer_to_pod(char *pl, char *pod, float *x, float *z)
{
	cl_steer(pl, (CL_F32(pl, 0x838) + CL_F32(pod, 0x10)) - CL_F32(pl, PL_X),
		(CL_F32(pl, 0x840) + CL_F32(pod, 0x14)) - CL_F32(pl, PL_Z), x, z);
}

/* 0x000e58b0  Clusteroids (gap)
Collector AI: 0 collect free pods (with three, or at random, go to the gate); 1 fly to
the gate (gate 0's position, 0x002f8734) and deliver; 2 chase a player with pods and
shoot. */
void ClustAI_Collector(int player, float *x, float *z)
{
	char *pl = CL_PLAYER(player);
	char *pod;
	char *victim;
	float dx;
	float dz;
	float outX;
	float outZ;
	short state;

	pod = ClustPods_NearestFree(player);
	state = CL_I16(pl, 0x808);
	CL_I16(pl, 0x80c) = 0;
	CL_I16(pl, 0x80e) = 0;
	if (state == 0) {
		if (CL_I16(pl, 0x650) == 3)
			goto deliver;
		if (pod != CL_PTR(pl, PL_TARGET_POD)) {
			if (CL_VAR(float, 0x002f7d90) < Core_RandUnit())
				goto deliver;
			cl_new_aim_offset(pl);
		}
		if (pod != NULL) {
			CL_PTR(pl, PL_TARGET_POD) = pod;
			cl_steer_to_pod(pl, pod, x, z);
			return;
		}
		if (Core_RandUnit() <= 0.75f) {
			victim = ClustPods_FindRobbable(player);
			CL_PTR(pl, PL_TARGET_POD) = NULL;
			CL_PTR(pl, 0x848) = victim;
			CL_I16(pl, 0x808) = 2;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		CL_PTR(pl, PL_TARGET_POD) = NULL;
		CL_I16(pl, 0x808) = 1;
		*x = 0.0f;
		*z = 0.0f;
		return;
deliver:
		CL_I16(pl, 0x808) = 1;
		*x = 0.0f;
		*z = 0.0f;
		return;
	}
	if (state == 1) {
		dx = CL_VAR(float, 0x002f8734) - CL_F32(pl, PL_X);
		dz = CL_VAR(float, 0x002f873c) - CL_F32(pl, PL_Z);
		if (dx * dx + dz * dz <= 15625.0f) {
			outX = 0.0f;
			outZ = 0.0f;
			if (pod == NULL) {
				CL_PTR(pl, 0x848) = ClustPods_FindRobbable(player);
				CL_I16(pl, 0x808) = 2;
			} else {
				CL_I16(pl, 0x808) = 0;
			}
		} else {
			cl_steer(pl, dx, dz, &outX, &outZ);
		}
		if (CL_F32(pl, 0x830) < (float)cl_rand() * 3.0518509e-05f)
			CL_I16(pl, 0x80c) = 1;
		*x = outX;
		*z = outZ;
		return;
	}
	if (state != 2) {
		/* not reached (states 0-2); the original returns an uninitialised local and the
		   player index's bits */
		*x = 0.0f;
		*z = cl_bitsf((unsigned int)player);
		return;
	}
	victim = CL_PTR(pl, 0x848);
	if (victim == NULL) {
		CL_I16(pl, 0x80c) = 1;
		CL_I16(pl, 0x808) = 0;
		*x = 0.0f;
		*z = 0.0f;
		return;
	}
	if (DAT_002baebc == 0 || DAT_002baef4[player * 10] != DAT_002baef4[CL_I16(victim, 0x12) * 10]) {
		if (0 < ClustPods_CountLooseOf(victim)) {
			Core_RandUnit();
			cl_steer(pl, CL_F32(victim, PL_X) - CL_F32(pl, PL_X), CL_F32(victim, PL_Z) - CL_F32(pl, PL_Z), x, z);
			CL_I16(pl, 0x80c) = 1;
			return;
		}
	} else if (Core_RandUnit() <= CL_F32(pl, 0x830)) {
		goto no_fire;
	}
	CL_I16(pl, 0x80c) = 1;
no_fire:
	CL_I16(pl, 0x808) = 0;
	*x = 0.0f;
	*z = 0.0f;
}

/* 0x000e5cb0  Clusteroids (gap)
A new roaming target for player: a random point at arena-radius distance from the ship
that is inside the arena (and, with `avoid`, at least 300 from the nearest other ship);
the centre after 10 tries. */
void ClustAI_PickRoamTarget(int player, int avoid)
{
	char *pl = CL_PLAYER(player);
	float pos[4];
	float radius;
	float distance;
	float angle;
	int bad;
	int tries;

	radius = CL_VAR(float, 0x002f7f68) - 35.0f;
	tries = 0;
	pos[1] = 0.0f;
	pos[3] = 0.0f;
	for (;;) {
		bad = 0;
		angle = (float)cl_rand() * 1.9175344e-4f;
		pos[0] = (float)((double)cosf(angle) * (double)radius + (double)CL_F32(pl, PL_X));
		pos[2] = (float)((double)sinf(angle) * (double)radius + (double)CL_F32(pl, PL_Z));
		distance = fabsf(sqrtf(pos[0] * pos[0] + pos[2] * pos[2]));
		if (radius < distance ||
			(avoid != 0 && ClustPods_NearestShip(player, pos, &distance) != NULL && distance < 300.0f))
			bad = 1;
		tries = tries + 1;
		if (10 < tries) {
			pos[0] = 0.0f;
			pos[2] = 0.0f;
			break;
		}
		if (!bad)
			break;
	}
	CL_F32(pl, 0x814) = pos[0];
	CL_F32(pl, 0x818) = 0.0f;
	CL_F32(pl, 0x81c) = pos[2];
	CL_F32(pl, 0x820) = 0.0f;
}

/* 0x000e5de0  Clusteroids (gap)
Tails AI: 0 collect free pods; 1 chase the player in +0x848 (with at least three loose
pods); 2 when no free pod is left: roam if it has the single longest chain, else chase
a player with pods (Pod Hog: often a human); 3 roam and boost while no pod is free. */
void ClustAI_Tails(int player, float *x, float *z)
{
	char *pl = CL_PLAYER(player);
	char *pod;
	char *target;
	float dx;
	float dz;
	float outX;
	float outZ;
	int best;
	int count;

	pod = ClustPods_NearestFree(player);
	CL_I16(pl, 0x80c) = 0;
	CL_I16(pl, 0x80e) = 0;
	switch (CL_I16(pl, 0x808)) {
	case 0:
		outX = 0.0f;
		outZ = 0.0f;
		if (pod == NULL) {
			CL_I16(pl, 0x808) = 2;
		} else {
			if (pod != CL_PTR(pl, PL_TARGET_POD))
				cl_new_aim_offset(pl);
			cl_steer_to_pod(pl, pod, &outX, &outZ);
			if (CL_F32(pl, 0x830) < Core_RandUnit()) {
				CL_PTR(pl, PL_TARGET_POD) = pod;
				CL_I16(pl, 0x80c) = 1;
				*x = outX;
				*z = outZ;
				return;
			}
		}
		CL_PTR(pl, PL_TARGET_POD) = pod;
		*x = outX;
		*z = outZ;
		return;
	case 1:
		target = CL_PTR(pl, 0x848);
		if (target == NULL || ClustPods_CountLooseOf(target) < 3) {
			CL_I16(pl, 0x808) = 0;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		Core_RandUnit();
		dx = CL_F32(target, PL_X) - CL_F32(pl, PL_X);
		dz = CL_F32(target, PL_Z) - CL_F32(pl, PL_Z);
		break;
	case 2:
		if (pod != NULL) {
			CL_I16(pl, 0x808) = 0;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		count = ClustPods_CountLongest(player, &best);
		if (ClustPods_CountLoose(player) == best && count == 1) {
			ClustAI_PickRoamTarget(player, 0);
			CL_I16(pl, 0x808) = 3;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		if ((g_ClVariant != 2 || Core_RandUnit() <= 0.85f ||
				(target = ClustPods_FindRobbableHuman(player)) == NULL) &&
			(target = ClustPods_FindRobbable(player)) == NULL) {
			CL_I16(pl, 0x808) = 0;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		Core_RandUnit();
		dx = CL_F32(target, PL_X) - CL_F32(pl, PL_X);
		dz = CL_F32(target, PL_Z) - CL_F32(pl, PL_Z);
		break;
	case 3:
		if (CL_VAR(short, 0x002f8f5e) != 0 || ClustPods_CountLoose(player) == 0) {
			CL_I16(pl, 0x808) = 0;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		dx = CL_F32(pl, 0x814) - CL_F32(pl, PL_X);
		dz = CL_F32(pl, 0x81c) - CL_F32(pl, PL_Z);
		if (dx * dx + dz * dz <= 100.0f) {
			ClustAI_PickRoamTarget(player, 1);
			outZ = 0.0f;
			outX = 0.0f;
		} else {
			cl_steer(pl, dx, dz, &outX, &outZ);
		}
		if (CL_F32(pl, 0x834) < Core_RandUnit())
			CL_I16(pl, 0x80e) = 1;
		*x = outX;
		*z = outZ;
		return;
	default:
		/* not reached; see ClustAI_Collector */
		*x = 0.0f;
		*z = cl_bitsf((unsigned int)player);
		return;
	}
	cl_steer(pl, dx, dz, &outX, &outZ);
	if (CL_F32(pl, 0x830) < Core_RandUnit())
		CL_I16(pl, 0x80c) = 1;
	*x = outX;
	*z = outZ;
}

/* the aim at the first pod of `holder`'s chain: directly while it is young (5 frames),
   otherwise led by its movement x 0x002f7d8c ("aiShootAheadRate") when faster than 10 */
static void cl_aim_at_chain(char *pl, char *holder, float *dx, float *dz, float *aimX, float *aimZ)
{
	char *head = CL_PTR(holder, PL_PODS);
	float vx;
	float vz;
	float leadX;
	float leadZ;

	if (CL_F32(head, 0x4) <= 5.0f) {
		*dx = CL_F32(head, 0x10) - CL_F32(pl, PL_X);
		*dz = CL_F32(head, 0x14) - CL_F32(pl, PL_Z);
		*aimX = CL_F32(head, 0x10);
		*aimZ = CL_F32(head, 0x14);
		return;
	}
	vx = CL_F32(head, 0x10) - CL_F32(head, 0x18);
	vz = CL_F32(head, 0x14) - CL_F32(head, 0x1c);
	leadX = 0.0f;
	leadZ = 0.0f;
	if (10.0f < sqrtf(vx * vx + vz * vz)) {
		leadX = CL_VAR(float, 0x002f7d8c) * vx;
		leadZ = CL_VAR(float, 0x002f7d8c) * vz;
	}
	*dx = (leadX + CL_F32(head, 0x10)) - CL_F32(pl, PL_X);
	*dz = (leadZ + CL_F32(head, 0x14)) - CL_F32(pl, PL_Z);
	*aimX = leadX + CL_F32(head, 0x10);
	*aimZ = leadZ + CL_F32(head, 0x14);
}

/* the shared state 2 of Mines and Pod Hog: roam with the boost while holding pods */
static void cl_roam_with_pods(char *pl, int player, char *pod, float *x, float *z)
{
	float dx;
	float dz;

	if (CL_PTR(pl, PL_PODS) == NULL) {
		CL_I16(pl, 0x808) = (short)(pod == NULL);
		*x = 0.0f;
		*z = 0.0f;
		return;
	}
	dx = CL_F32(pl, 0x814) - CL_F32(pl, PL_X);
	dz = CL_F32(pl, 0x81c) - CL_F32(pl, PL_Z);
	if (dx * dx + dz * dz <= 100.0f) {
		ClustAI_PickRoamTarget(player, 1);
		CL_I16(pl, 0x80e) = 1;
		*x = 0.0f;
		*z = 0.0f;
		return;
	}
	cl_steer(pl, dx, dz, x, z);
	CL_I16(pl, 0x80e) = 1;
}

/* 0x000e61f0  Clusteroids (gap)
Mines AI: 0 collect free pods; 1 shoot at the first player holding pods (the aim point
in 0x002f9a88 / 0x002f9a90 for debugging); 2 roam while holding pods; 3 wait (+0x84c). */
void ClustAI_Mines(int player, float *x, float *z)
{
	char *pl = CL_PLAYER(player);
	char *pod;
	char *holder;
	float dx;
	float dz;
	float outX;
	float outZ;

	pod = ClustPods_NearestFree(player);
	CL_VAR(float, 0x002f9a88) = 0.0f;
	CL_VAR(float, 0x002f9a90) = 0.0f;
	CL_I16(pl, 0x80c) = 0;
	CL_I16(pl, 0x80e) = 0;
	switch (CL_I16(pl, 0x808)) {
	case 0:
		if (pod != CL_PTR(pl, PL_TARGET_POD))
			cl_new_aim_offset(pl);
		if (pod == NULL) {
			if (CL_PTR(pl, PL_PODS) == NULL) {
				CL_PTR(pl, PL_TARGET_POD) = NULL;
				CL_I16(pl, 0x808) = 1;
				*x = 0.0f;
				*z = 0.0f;
				return;
			}
			CL_I16(pl, 0x808) = 2;
			ClustAI_PickRoamTarget(player, 1);
			CL_PTR(pl, PL_TARGET_POD) = NULL;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		CL_PTR(pl, PL_TARGET_POD) = pod;
		cl_steer_to_pod(pl, pod, x, z);
		return;
	case 1:
		holder = ClustPods_FirstHolder();
		if (holder == NULL) {
			CL_I16(pl, 0x808) = 0;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		if (holder == pl) {
			CL_I16(pl, 0x808) = 2;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		cl_aim_at_chain(pl, holder, &dx, &dz, CL_FP(CL_ADDR(0x002f9a88), 0), CL_FP(CL_ADDR(0x002f9a90), 0));
		cl_steer(pl, dx, dz, &outX, &outZ);
		if (CL_F32(pl, 0x830) < Core_RandUnit())
			CL_I16(pl, 0x80c) = 1;
		if (CL_F32(pl, 0x834) < Core_RandUnit())
			CL_I16(pl, 0x80e) = 1;
		*x = outX;
		*z = outZ;
		return;
	case 2:
		cl_roam_with_pods(pl, player, pod, x, z);
		return;
	case 3:
		CL_I16(pl, 0x84c) = CL_I16(pl, 0x84c) - 1;
		if (CL_I16(pl, 0x84c) < 1)
			CL_I16(pl, 0x808) = 0;
		*x = 0.0f;
		*z = 0.0f;
		return;
	default:
		/* not reached; see ClustAI_Collector */
		*x = 0.0f;
		*z = cl_bitsf((unsigned int)player);
		return;
	}
}

/* 0x000e6650  Clusteroids (gap)
Pod Hog AI: 0 collect free pods (none left: roam with its pods or go robbing); 1 shoot
at the first pod of the victim (ClustPods_ChooseVictim; not a team mate); 2 roam while
holding pods. */
void ClustAI_PodHog(int player, float *x, float *z)
{
	char *pl = CL_PLAYER(player);
	char *pod;
	char *victim;
	char *current;
	float dx;
	float dz;
	float aimX;
	float aimZ;
	float outX;
	float outZ;
	short state;

	pod = ClustPods_NearestFree(player);
	state = CL_I16(pl, 0x808);
	CL_I16(pl, 0x80c) = 0;
	CL_I16(pl, 0x80e) = 0;
	if (state == 0) {
		if (pod != CL_PTR(pl, PL_TARGET_POD))
			cl_new_aim_offset(pl);
		if (pod == NULL) {
			if (CL_PTR(pl, PL_PODS) != NULL && 0.5f < Core_RandUnit()) {
				CL_I16(pl, 0x808) = 2;
				ClustAI_PickRoamTarget(player, 1);
				CL_PTR(pl, PL_TARGET_POD) = NULL;
				*x = 0.0f;
				*z = 0.0f;
				return;
			}
			CL_PTR(pl, PL_TARGET_POD) = NULL;
			CL_I16(pl, 0x808) = 1;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		CL_PTR(pl, PL_TARGET_POD) = pod;
		cl_steer_to_pod(pl, pod, x, z);
		return;
	}
	if (state == 1) {
		victim = ClustPods_ChooseVictim(player);
		if (DAT_002baebc != 0 && victim != NULL &&
			DAT_002baef4[player * 10] == DAT_002baef4[CL_I16(victim, 0x12) * 10]) {
			CL_I16(pl, 0x808) = 2;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		current = CL_PTR(pl, 0x848);
		if (current == NULL)
			CL_PTR(pl, 0x848) = victim;
		else if (CL_PTR(current, PL_PODS) != NULL)
			victim = current;
		if (victim == NULL) {
			CL_I16(pl, 0x808) = 0;
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		if (victim == pl || CL_PTR(victim, PL_PODS) == NULL) {
			if (CL_PTR(pl, PL_PODS) != NULL) {
				CL_I16(pl, 0x808) = 2;
				*x = 0.0f;
				*z = 0.0f;
				return;
			}
			*x = 0.0f;
			*z = 0.0f;
			return;
		}
		cl_aim_at_chain(pl, victim, &dx, &dz, &aimX, &aimZ);
		cl_steer(pl, dx, dz, &outX, &outZ);
		if (CL_F32(pl, 0x830) < Core_RandUnit())
			CL_I16(pl, 0x80c) = 1;
		if (CL_F32(pl, 0x834) < Core_RandUnit())
			CL_I16(pl, 0x80e) = 1;
		*x = outX;
		*z = outZ;
		return;
	}
	if (state == 2) {
		cl_roam_with_pods(pl, player, pod, x, z);
		return;
	}
	/* not reached; see ClustAI_Collector */
	*x = 0.0f;
	*z = cl_bitsf((unsigned int)player);
}
