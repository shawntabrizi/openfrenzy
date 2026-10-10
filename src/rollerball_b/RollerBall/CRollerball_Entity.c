/*
RollerBall/CRollerball_Entity.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerball_Entity.cpp (heap tag of 0x001927e0):

- CRollerball_Entity (vtable 0x0027482c): the base of the player balls (CRollerBall_Player,
  0x427c bytes). It derives from CRollerBall_EntityEventSystem (an event queue, +0x04) and
  holds a physics body at +0x40cc (rollerball_a's class, vtable 0x002745f0). Methods here:
  ball physics (forces, integration, rolling rotation) and the push of a goal.
  The Entity methods have no tag; their place is inferred from the address order.
- CRollerball_EntitySystem (vtable 0x00274844, in the game object): loads the character,
  ball and pod models, makes the four players with their controllers (AI state objects),
  and updates and draws them.
*/

#include "rollerball_b.h"

/* 0x0027482c  CRollerball_Entity vtable */
const void *const rbb_vtable_0027482c[6] = {
	(const void *)FUN_00192460, /* +0x00 scalar deleting destructor */
	(const void *)FUN_001d5950, /* +0x04 (empty) */
	(const void *)FUN_001ec260, /* +0x08 update (empty) */
	(const void *)FUN_001d5950, /* +0x0c draw (empty) */
	(const void *)FUN_001d5950, /* +0x10 (empty) */
	(const void *)FUN_001d5950, /* +0x14 (empty) */
};

/* 0x00274844  CRollerball_EntitySystem vtable */
const void *const rbb_vtable_00274844[1] = {
	(const void *)FUN_001927c0, /* +0x00 scalar deleting destructor */
};

static const char rbb_EntityFile[] = "C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerball_Entity.cpp";

/* 0x00192170  RollerBall/CRollerball_Entity.cpp (inferred)
CRollerball_Entity constructor. */
void *CRollerballEntity_Ctor(void *self)
{
	CRollerBallEntityEventSystem_Ctor(self);
	*(const void *const **)self = rbb_vtable_0027482c;
	/* rollerball_a 0x0018ec10: the physics body's constructor */
	FUN_0018ec10(RBB_ADDR(self, RBE_BODY));
	return self;
}

/* 0x001921c0  RollerBall/CRollerball_Entity.cpp (inferred)
CRollerball_Entity destructor. */
void CRollerballEntity_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_0027482c;
	FUN_0018ec20(RBB_ADDR(self, RBE_BODY));
	CRollerBallEntityEventSystem_Dtor(self);
}

/* 0x00192220  RollerBall/CRollerball_Entity.cpp (inferred)
velocity += acceleration * dt * time scale */
void CRollerballEntity_Integrate(void *self, float dt)
{
	dt = dt * RBB_F(self, RBE_TIME_SCALE);
	RBB_F(self, RBE_VEL + 0) = dt * RBB_F(self, RBE_ACCEL + 0) + RBB_F(self, RBE_VEL + 0);
	RBB_F(self, RBE_VEL + 4) = dt * RBB_F(self, RBE_ACCEL + 4) + RBB_F(self, RBE_VEL + 4);
	RBB_F(self, RBE_VEL + 8) = dt * RBB_F(self, RBE_ACCEL + 8) + RBB_F(self, RBE_VEL + 8);
}

/* 0x00192270  RollerBall/CRollerball_Entity.cpp (inferred) */
void CRollerballEntity_ClearPush(void *self)
{
	RBB_F(self, RBE_PUSH + 0) = 0.0f;
	RBB_F(self, RBE_PUSH + 4) = 0.0f;
	RBB_F(self, RBE_PUSH + 8) = 0.0f;
}

/* 0x00192290  RollerBall/CRollerball_Entity.cpp (inferred)
CRollerball_EntitySystem constructor (the game object's member). */
void CRollerballEntitySystem_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274844;
}

/* 0x001922a0  RollerBall/CRollerball_Entity.cpp (inferred)
CRollerball_EntitySystem destructor. */
void CRollerballEntitySystem_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274844;
}

/* 0x001922b0  RollerBall/CRollerball_Entity.cpp (inferred)
Delete the entities, then the models. */
void CRollerballEntitySystem_Shutdown(void *self)
{
	char *e = RBB_P(self, RBES_LIST);
	int i;

	while (e != NULL) {
		char *next = RBB_P(e, RBE_NEXT);
		RBB_VFN(e, 0x04, void (*)(void *))(e);
		RBB_VFN(e, 0x00, void *(*)(void *, int))(e, 1);
		e = next;
	}
	for (i = 0; i < 8; i++) {   /* 4 characters, 4 "opt" characters */
		struct bkResource **res = (struct bkResource **)RBB_ADDR(self, RBES_CHARACTERS + i * 4);
		if (*res != NULL) {
			bkDeleteResource(*res);
			*res = NULL;
		}
	}
	for (i = 0; i < 4; i++) {   /* the balls */
		struct bkResource **res = (struct bkResource **)RBB_ADDR(self, RBES_BALLS + i * 4);
		if (*res != NULL) {
			bkDeleteResource(*res);
			*res = NULL;
		}
	}
	for (i = 0; i < 2; i++) {   /* hoverpod, shadowball */
		struct bkResource **res = (struct bkResource **)RBB_ADDR(self, RBES_MODELS + i * 4);
		if (*res != NULL) {
			bkDeleteResource(*res);
			*res = NULL;
		}
	}
}

/* 0x00192350  RollerBall/CRollerball_Entity.cpp (inferred) */
void *CRollerballEntitySystem_GetFirst(void *self)
{
	return RBB_P(self, RBES_LIST);
}

/* 0x00192360  RollerBall/CRollerball_Entity.cpp (inferred)
Update every entity (virtual +0x08). */
void CRollerballEntitySystem_Update(void *self, float dt)
{
	char *e;

	for (e = RBB_P(self, RBES_LIST); e != NULL; e = RBB_P(e, RBE_NEXT))
		RBB_VFN(e, 0x08, void (*)(void *, float))(e, dt);
}

/* 0x00192390  RollerBall/CRollerball_Entity.cpp (inferred)
Draw every entity (virtual +0x0c), between the game shader's begin and end. */
void CRollerballEntitySystem_Draw(void *self)
{
	char *e;
	void *shader;

	bdSetRenderState(6, 0, 0);
	bdPushRenderState(3, 1, 0);
	shader = RBB_P(rbb_Game, 0x25bc);
	RBB_VFN(shader, 0x0c, void (*)(void *, int))(shader, 1);
	for (e = RBB_P(self, RBES_LIST); e != NULL; e = RBB_P(e, RBE_NEXT))
		RBB_VFN(e, 0x0c, void (*)(void *))(e);
	bdPopRenderState(3);
	shader = RBB_P(rbb_Game, 0x25bc);
	RBB_VFN(shader, 0x0c, void (*)(void *, int))(shader, 0);
}

/* 0x00192400  RollerBall/CRollerball_Entity.cpp (inferred)
Entities of players without a human get the computer flag (+0x19). The list order is
player 0, 1, 2, 3. (A method of the entity system; the x86 code does not use ECX.) */
void CRollerballEntitySystem_UpdateComputerFlags(void *self)
{
	char *e = rbb_FirstPlayer;
	int i = 0;

	(void)self;
	for (; e != NULL; e = RBB_P(e, RBE_NEXT), i++)
		RBB_B(e, RBE_COMPUTER) = rbb_PlayerHuman(i) == 0;
}

/* 0x00192430  RollerBall/CRollerball_Entity.cpp (inferred)
Set the animation set `mode` of every player (0x001999e0). (A method of the entity
system; the x86 code does not use ECX.) */
void CRollerballEntitySystem_SetAnimMode(void *self, int mode)
{
	char *e;

	(void)self;
	for (e = rbb_FirstPlayer; e != NULL; e = RBB_P(e, RBE_NEXT))
		CRollerballPlayer_SetAnimMode(e, mode);
}

/* 0x00192460  RollerBall/CRollerball_Entity.cpp (inferred)
CRollerball_Entity scalar deleting destructor. */
void *CRollerballEntity_DeletingDtor(void *self, unsigned int flags)
{
	CRollerballEntity_Dtor(self);
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x00192480  RollerBall/CRollerball_Entity.cpp (inferred)
Place the ball (body and the copy at +0x40b8). */
void CRollerballEntity_SetPosition(void *self, float *pos)
{
	CDemoBody_SetPosition(RBB_ADDR(self, RBE_BODY), pos);
	RBB_F(self, RBE_START_POS + 0) = pos[0];
	RBB_F(self, RBE_START_POS + 4) = pos[1];
	RBB_F(self, RBE_START_POS + 8) = pos[2];
	RBB_F(self, RBE_START_POS + 12) = pos[3];
}

/* 0x001924c0  RollerBall/CRollerball_Entity.cpp (inferred)
The ball's acceleration: drag, the roll force (input), rolling friction against the
ground normal (+0x4110) and the push. */
void CRollerballEntity_ComputeForces(void *self)
{
	float inv_scale;
	float gravity;
	float along;
	float side[3];
	float len2;
	float k;

	RBB_F(self, RBE_ACCEL + 0) = 0.0f;
	RBB_F(self, RBE_ACCEL + 4) = 0.0f;
	RBB_F(self, RBE_ACCEL + 8) = 0.0f;
	RBB_F(self, RBE_ACCEL + 0) = -(RBB_F(self, RBE_DRAG) * RBB_F(self, RBE_VEL + 0));
	RBB_F(self, RBE_ACCEL + 4) = -(RBB_F(self, RBE_VEL + 4) * RBB_F(self, RBE_DRAG));
	RBB_F(self, RBE_ACCEL + 8) = -(RBB_F(self, RBE_VEL + 8) * RBB_F(self, RBE_DRAG));
	inv_scale = 1.0f / RBB_F(self, RBE_TIME_SCALE);
	RBB_F(self, RBE_ACCEL + 0) = inv_scale * RBB_F(self, RBE_FORCE + 0) + RBB_F(self, RBE_ACCEL + 0);
	RBB_F(self, RBE_ACCEL + 4) = inv_scale * RBB_F(self, RBE_FORCE + 4) + RBB_F(self, RBE_ACCEL + 4);
	RBB_F(self, RBE_ACCEL + 8) = inv_scale * RBB_F(self, RBE_FORCE + 8) + RBB_F(self, RBE_ACCEL + 8);

	gravity = RBB_F(self, RBE_FORCE + 4);   /* the vertical force */
	along = RBB_F(self, RBE_NORMAL + 0) * RBB_F(self, RBE_VEL + 0) +
		RBB_F(self, RBE_NORMAL + 4) * RBB_F(self, RBE_VEL + 4) +
		RBB_F(self, RBE_NORMAL + 8) * RBB_F(self, RBE_VEL + 8);
	side[0] = RBB_F(self, RBE_VEL + 0) - along * RBB_F(self, RBE_NORMAL + 0);
	side[1] = RBB_F(self, RBE_VEL + 4) - along * RBB_F(self, RBE_NORMAL + 4);
	side[2] = RBB_F(self, RBE_VEL + 8) - along * RBB_F(self, RBE_NORMAL + 8);
	len2 = side[0] * side[0] + side[1] * side[1] + side[2] * side[2];
	if (len2 <= 0.03f) {
		if (len2 <= 0.0f)
			goto push;
		bmVectorNormalise(side, side);
		k = len2 * 33.3333359f * RBB_F(self, RBE_STATIC_FRICTION);
	} else {
		bmVectorNormalise(side, side);
		k = RBB_F(self, RBE_KINETIC_FRICTION);
	}
	k = inv_scale * gravity * k;
	RBB_F(self, RBE_ACCEL + 0) = side[0] * k + RBB_F(self, RBE_ACCEL + 0);
	RBB_F(self, RBE_ACCEL + 4) = side[1] * k + RBB_F(self, RBE_ACCEL + 4);
	RBB_F(self, RBE_ACCEL + 8) = side[2] * k + RBB_F(self, RBE_ACCEL + 8);
push:
	RBB_F(self, RBE_ACCEL + 0) = RBB_F(self, RBE_ACCEL + 0) + RBB_F(self, RBE_PUSH + 0);
	RBB_F(self, RBE_ACCEL + 4) = RBB_F(self, RBE_ACCEL + 4) + RBB_F(self, RBE_PUSH + 4);
	RBB_F(self, RBE_ACCEL + 8) = RBB_F(self, RBE_ACCEL + 8) + RBB_F(self, RBE_PUSH + 8);
}

/* 0x001926e0  RollerBall/CRollerball_Entity.cpp (inferred)
Roll the ball: turn its orientation (+0x4120) about the horizontal axis normal to the
velocity, by the distance travelled over the circumference. */
void CRollerballEntity_UpdateRoll(void *self)
{
	float vz = RBB_F(self, RBE_VEL + 8);
	float vx = RBB_F(self, RBE_VEL + 0);
	float len = sqrtf(vx * vx + vz * vz);
	float axis_angle[4];
	float q[4];
	float *rot;

	if (len == 0.0f)
		return;
	axis_angle[1] = 0.0f;
	axis_angle[0] = -(vz * (1.0f / len));
	axis_angle[2] = (1.0f / len) * vx;
	axis_angle[3] = (len * 1.57079625f) / (RBB_F(self, RBE_RADIUS) + RBB_F(self, RBE_RADIUS));
	bmAxisAngleToQuaternion(q, axis_angle);
	rot = RBB_V(self, RBE_ROTATION);
	bmQuaternionMultiply(rot, rot, q);
	FUN_00198590(rot);
}

/* 0x00192790  RollerBall/CRollerball_Entity.cpp (inferred)
Set the push (4 floats). */
void CRollerballEntity_SetPush(void *self, float *push)
{
	RBB_F(self, RBE_PUSH + 0) = push[0];
	RBB_F(self, RBE_PUSH + 4) = push[1];
	RBB_F(self, RBE_PUSH + 8) = push[2];
	RBB_F(self, RBE_PUSH + 12) = push[3];
}

/* 0x001927c0  RollerBall/CRollerball_Entity.cpp (inferred)
CRollerball_EntitySystem scalar deleting destructor. */
void *CRollerballEntitySystem_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274844;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x001927e0  RollerBall/CRollerball_Entity.cpp
Load the models of the four players' characters, their balls and the hover pod and
shadow ball, then make the four players (from player 3 down to 0, each put first in the
list) with the controller of the variation, and start them. */
int CRollerballEntitySystem_Init(void *self, struct bkPackage *package)
{
	char name[256];
	struct bkResource *res;
	char *e = NULL;
	int i;
	int player;
	int start_off;

	RBB_P(self, RBES_LIST) = NULL;
	for (i = 0; i < 4; i++) {
		sprintf(name, "characters\\%s.obe", rbb_CharacterModel(rbb_PlayerCharacter(i)));
		res = bLoadResource(package, name, 1, 0xdefa);
		RBB_P(self, RBES_CHARACTERS + i * 4) = res;
		if (res == NULL)
			goto failed;
	}
	for (i = 0; i < 4; i++) {
		sprintf(name, "characters\\%s.obe", rbb_CharacterOptModel(rbb_PlayerCharacter(i)));
		res = bLoadResource(package, name, 1, 0xdefa);
		RBB_P(self, RBES_OPT_CHARACTERS + i * 4) = res;
		if (res == NULL)
			goto failed;
	}
	for (i = 0; i < 4; i++) {
		sprintf(name, "models\\%s.obe", rbb_CharacterBall(rbb_PlayerCharacter(i)));
		res = bLoadResource(package, name, 1, 0xdefa);
		RBB_P(self, RBES_BALLS + i * 4) = res;
		if (res == NULL)
			goto failed;
	}
	for (i = 0; i < 2; i++) {   /* "hoverpod", "shadowball" */
		sprintf(name, "models\\%s.obe", RBD_SV(0x002b50e0)[i]);
		res = bLoadResource(package, name, 1, 0xdefa);
		RBB_P(self, RBES_MODELS + i * 4) = res;
		if (res == NULL)
			goto failed;
	}

	player = 3;
	start_off = 0x60;   /* game + 0x6ac: 4 start positions, 0x20 bytes each */
	for (i = 3; i >= 0; i--) {
		void *mem = RBB_NEW(0x427c, rbb_EntityFile, 0x266);
		int character;
		e = NULL;
		if (mem != NULL)
			e = CRollerballPlayer_Ctor(mem);
		character = rbb_PlayerCharacter(i);
		CRollerballPlayer_Init(e, player, (float *)(rbb_Game + start_off + 0x6ac),
			(float *)(rbb_Game + start_off + 0x6bc), RBD_F(0x002b513c + rbb_Variation * 0x28));
		if ((unsigned int)rbb_Variation <= 4) {
			void *ctl;
			switch (rbb_Variation) {
			case 0:
				mem = RBB_NEW(0x184, rbb_EntityFile, 0x275);
				ctl = mem != NULL ? CRollerballControlBase_Ctor(mem) : NULL;
				break;
			case 1:
				mem = RBB_NEW(0x98, rbb_EntityFile, 0x278);
				ctl = mem != NULL ? CRollerballControlSumo_Ctor(mem) : NULL;
				break;
			case 2:
				mem = RBB_NEW(0x54, rbb_EntityFile, 0x27b);
				ctl = mem != NULL ? CRollerballControlRace2_Ctor(mem) : NULL;
				break;
			case 3:
				mem = RBB_NEW(0x5c, rbb_EntityFile, 0x27e);
				ctl = mem != NULL ? CRollerballControlRace_Ctor(mem) : NULL;
				break;
			default:
				mem = RBB_NEW(0x98, rbb_EntityFile, 0x281);
				ctl = mem != NULL ? CRollerballControlDrop_Ctor(mem) : NULL;
				break;
			}
			RBB_P(e, RBE_CONTROLLER) = ctl;
		}
		RBB_I(e, RBE_COLOUR + 0) = (int)((unsigned int)rbb_CharacterColour(character)[0] >> 1);
		RBB_I(e, RBE_COLOUR + 4) = (int)((unsigned int)rbb_CharacterColour(character)[1] >> 1);
		RBB_I(e, RBE_COLOUR + 8) = (int)((unsigned int)rbb_CharacterColour(character)[2] >> 1);
		RBB_P(e, RBE_NEXT) = RBB_P(self, RBES_LIST);
		player--;
		start_off -= 0x20;
		RBB_P(self, RBES_LIST) = e;
	}
	/* `player` is -1 here: the original reads the "human" word of the record before
	player 0 (0x002baeb0) for every entity. Kept; 0x00192400 sets the right flags later. */
	do {
		void *ctl;
		CRollerballPlayer_FindTeamMate(e);
		ctl = RBB_P(e, RBE_CONTROLLER);
		RBB_VFN(ctl, 0x00, void (*)(void *, void *))(ctl, e);
		RBB_B(e, RBE_COMPUTER) = rbb_PlayerHuman(player) == 0;
		e = RBB_P(e, RBE_NEXT);
	} while (e != NULL);
	RBB_I(self, RBES_COLOUR_SET) = DAT_00283ea4;
	return 1;

failed:
	bkPrintf("ERROR: Failed to load object %s!\n", name);
	return 0;
}
