/*
RollerBall/CRollerBall_Respawn.c  <-  C:\BlitzParty\Game\Src\RollerBall\CRollerBall_Respawn.cpp

The respawn system (game +0x1c50, vtable 0x00274718): respawn points (0x30 bytes, vtable
0x00274710: +4 next, +8 position, +0x18 direction, +0x28 id, +0x2c in use), linked from +8
(count at +0xc), and eight respawns in progress (0x50 bytes from +0x10, vtable
0x00274714: +8 active, +0xc state, +0x10 a timer, +0x48 the player, +0x4c the point).
AddPoint (0x00190910) is tagged with this file; the methods around it are gap functions
placed here (inferred).
*/

#include "rollerball_a_ext.h"

/* 0x00274710  vtable of a respawn point */
const void *const vtable_00274710[1] = { (const void *)CRollerBallRespawnPoint_DeletingDtor };

/* 0x00274714  vtable of a respawn in progress */
const void *const vtable_00274714[1] = { (const void *)CRollerBallRespawnEntry_DeletingDtor };

/* 0x00274718  vtable of the respawn system */
const void *const vtable_00274718[1] = { (const void *)CRollerBallRespawn_DeletingDtor };

/* 0x00190620  RollerBall (gap): respawn entry constructor */
void *CRollerBallRespawnEntry_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274714;
	CRollerBallTimer_Ctor(RB_P(self, 0x10));
	return self;
}

/* 0x00190640  RollerBall (gap): respawn entry destructor */
void CRollerBallRespawnEntry_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274714;
	CRollerBallTimer_Dtor(RB_P(self, 0x10));
}

/* 0x00190650  RollerBall (gap)
Update of a respawn: state 0 fades the player out over the timer (+0x4278 alpha 256..0),
then puts it on the point (stopped); state 1 spawns the "RESPAWNCHAR" debris at the point
(400 below the player, in its colour) and starts a 0.5 s fade in; state 2 fades in and
then gives the player back (active, moving) and frees the point. */
void CRollerBallRespawnEntry_Update(void *self, float dt)
{
	unsigned char *s = self;
	unsigned char *p;
	void *timer = s + 0x10;
	float f, pos[4];

	(void)dt;
	CRollerBallTimer_Update(timer);
	switch (RB_I32(s, 0xc)) {
	case 0:
		f = RB_F32(timer, 0x34); /* 0x000965f0 (folded): the timer's fraction */
		RB_I32(RB_PTR(s, 0x48), 0x4278) = RB_FTOL((1.0f - f) * 256.0f);
		if (f >= 1.0f) {
			rbb_PlayerPlace(RB_PTR(s, 0x48), (const float *)RB_P(RB_PTR(s, 0x4c), 8));
			RB_F32(RB_PTR(s, 0x48), RB_PL_VEL) = 0.0f;
			RB_F32(RB_PTR(s, 0x48), RB_PL_VEL + 4) = 0.0f;
			RB_F32(RB_PTR(s, 0x48), RB_PL_VEL + 8) = 0.0f;
			RB_U8(RB_PTR(s, 0x48), RB_PL_1B) = 0;
			RB_I32(s, 0xc) = 1;
		}
		break;
	case 1:
		p = RB_PTR(s, 0x48);
		pos[0] = RB_F32(p, RB_PL_POS);
		pos[1] = RB_F32(p, RB_PL_POS + 4) - 400.0f;
		pos[2] = RB_F32(p, RB_PL_POS + 8);
		pos[3] = 1.0f;
		DAT_00290e8c = RB_I32(p, RB_PL_INDEX);
		Debris_Add((struct DebrisType *)DAT_00291734, pos, DAT_002ea478, 0, 0xffffffffu);
		RB_I32(s, 0xc) = 2;
		CRollerBallTimer_Set(timer, 0.5f);
		CRollerBallTimer_Start(timer);
		break;
	case 2:
		f = RB_F32(timer, 0x34);
		RB_I32(RB_PTR(s, 0x48), 0x4278) = RB_FTOL(f * 256.0f);
		if (f >= 1.0f) {
			p = RB_PTR(s, 0x48);
			RB_U8(p, RB_PL_ACTIVE) = 1;
			RB_U8(RB_PTR(s, 0x48), RB_PL_1A) = 1;
			RB_U8(RB_PTR(s, 0x48), 0x16) = 1;
			RB_U8(RB_PTR(s, 0x48), RB_PL_1B) = 1;
			RB_U8(s, 8) = 0;
			RB_U8(RB_PTR(s, 0x4c), 0x2c) = 0;
		}
		break;
	}
}

/* 0x001907c0  RollerBall (gap): constructor (the eight respawn entries) */
void *CRollerBallRespawn_Ctor(void *self)
{
	int i;

	*(const void *const **)self = vtable_00274718;
	for (i = 0; i < 8; i++)
		CRollerBallRespawnEntry_Ctor((unsigned char *)self + 0x10 + i * 0x50);
	return self;
}

/* 0x001907f0  RollerBall (gap): destructor (the entries, last first) */
void CRollerBallRespawn_Dtor(void *self)
{
	int i;

	*(const void *const **)self = vtable_00274718;
	for (i = 7; i >= 0; i--)
		CRollerBallRespawnEntry_Dtor((unsigned char *)self + 0x10 + i * 0x50);
}

/* 0x00190810  RollerBall (gap): Initialise: no points, no respawns, the timers'
frequency. Returns 1. */
int CRollerBallRespawn_Init(void *self, void *package)
{
	int i;

	(void)package;
	RB_PTR(self, 8) = NULL;
	for (i = 0; i < 8; i++) {
		RB_U8(self, 0x18 + i * 0x50) = 0;
		CDemoCountdown_Init((int)(uintptr_t)((unsigned char *)self + 0x20 + i * 0x50));
	}
	return 1;
}

/* 0x00190840  RollerBall (gap): Shutdown: deletes the points (virtual deleting destructor) */
void CRollerBallRespawn_Shutdown(void *self)
{
	unsigned char *pt, *next;

	pt = RB_PTR(self, 8);
	while (pt != NULL) {
		next = RB_PTR(pt, 4);
		RB_VFN(pt, 0x00, void *(*)(void *, unsigned int))(pt, 1);
		pt = next;
	}
}

/* 0x00190860  RollerBall (gap): Update: the active respawns, then every BACTOR extra
system with a frame of 1/60 s */
void CRollerBallRespawn_Update(void *self, float dt)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (RB_U8(self, 0x18 + i * 0x50) != 0)
			CRollerBallRespawnEntry_Update((unsigned char *)self + 0x10 + i * 0x50, dt);
	}
	baUpdateExtras(NULL, RB_FARG(DAT_0028fda8 * 0.0166666675f));
}

/* 0x001908b0  RollerBall (gap): respawn point deleting destructor (flags bit 0: free) */
void *CRollerBallRespawnPoint_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00274710;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x001908d0  RollerBall (gap): respawn entry deleting destructor (flags bit 0: free) */
void *CRollerBallRespawnEntry_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallRespawnEntry_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x001908f0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallRespawn_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallRespawn_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x00190910  RollerBall/CRollerBall_Respawn.cpp
Adds a respawn point ID at POS facing DIR (at the head of the list). */
void CRollerBallRespawn_AddPoint(void *self, int id, const float *pos, const float *dir)
{
	unsigned char *pt;

	pt = RB_NEW(0x30, RB_FILE_RESPAWN, 0x172);
	if (pt != NULL)
		*(const void *const **)pt = vtable_00274710;
	RB_F32(pt, 8) = pos[0];
	RB_F32(pt, 0xc) = pos[1];
	RB_F32(pt, 0x10) = pos[2];
	RB_F32(pt, 0x14) = pos[3];
	RB_F32(pt, 0x18) = dir[0];
	RB_F32(pt, 0x1c) = dir[1];
	RB_F32(pt, 0x20) = dir[2];
	RB_F32(pt, 0x24) = dir[3];
	RB_I32(pt, 0x28) = id;
	if (RB_PTR(self, 8) == NULL)
		RB_PTR(pt, 4) = NULL;
	else
		RB_PTR(pt, 4) = RB_PTR(self, 8);
	RB_PTR(self, 8) = pt;
	RB_I32(self, 0xc) = RB_I32(self, 0xc) + 1;
	RB_U8(pt, 0x2c) = 0;
}

/* 0x001909b0  RollerBall (gap)
Respawns PLAYER (that fell at POS) at the nearest free point (x and z): takes the last
free respawn entry, marks the point used, takes the player out and starts a 0.5 s fade
out with the "fall" sample (13). Returns the entry, or 0 (no free entry or point; the
original then keeps the entry's old point). */
void *CRollerBallRespawn_RespawnAt(void *self, unsigned char *player, const float *pos)
{
	unsigned char *s = self;
	unsigned char *e = NULL, *pt;
	float best = 99999.0f, dx, dz, d;
	int i;

	for (i = 0; i < 8; i++) {
		if (RB_U8(s, 0x18 + i * 0x50) == 0)
			e = s + 0x10 + i * 0x50;
	}
	if (e == NULL)
		return NULL;
	for (pt = RB_PTR(s, 8); pt != NULL; pt = RB_PTR(pt, 4)) {
		if (RB_U8(pt, 0x2c) != 0)
			continue;
		dx = RB_F32(pt, 8) - pos[0];
		dz = RB_F32(pt, 0x10) - pos[2];
		if (fabsf(dx) <= best && fabsf(dz) <= best) {
			d = sqrtf(dx * dx + dz * dz);
			if (d < best) {
				RB_PTR(e, 0x4c) = pt;
				best = d;
			}
		}
	}
	if (RB_PTR(e, 0x4c) == NULL)
		return NULL;
	RB_U8(RB_PTR(e, 0x4c), 0x2c) = 1;
	RB_U8(e, 8) = 1;
	RB_PTR(e, 0x48) = player;
	RB_U8(player, RB_PL_ACTIVE) = 0;
	RB_U8(RB_PTR(e, 0x48), RB_PL_1A) = 0;
	RB_I32(e, 0xc) = 0;
	CRollerBallTimer_Set(e + 0x10, 0.5f);
	CRollerBallTimer_Start(e + 0x10);
	CLSamples_PlayFromSet((char)RB_U8(player, RB_PL_INDEX), 0xd, RB_FP(player, RB_PL_POS),
		DAT_00293928, 0, -1, 0);
	return e;
}

/* 0x00190b60  RollerBall (gap): a jmp to 0x001909b0 */
void *CRollerBallRespawn_Respawn(void *self, unsigned char *player, const float *pos)
{
	return CRollerBallRespawn_RespawnAt(self, player, pos);
}

/* 0x00190b70  RollerBall (missing from the table: a BACTOR animation callback pushed by
rollerball_b 0x001917c0)
The animation of the object CONTEXT ended: clears its +0x58. Returns 1. */
int FUN_00190b70(int reason, void *inst, void *node, void *anim, void *context)
{
	(void)reason;
	(void)inst;
	(void)node;
	(void)anim;
	RB_U8(context, 0x58) = 0;
	return 1;
}
