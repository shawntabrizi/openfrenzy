/*
RollerBall/CRollerBall_Player.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerBall_Player.cpp (heap tags of 0x00199ba0):
CRollerBall_Player (0x427c bytes, vtable 0x00274c08), the player ball: a CRollerball_Entity
(ball physics, event queue) with the character riding on it, its input channels, its
controller (the AI states of player controllers, +0x4204) and the rolling sound. The
functions other than 0x00199ba0 have no tag; placed by address order (0x00198a00-
0x0019a4e0).

Virtual functions: [0] scalar deleting destructor, [1] shutdown, [2] update, [3] draw,
[6] input set-up, [7] control, [8] input free, [9] draw (reflection pass).

Player fields (besides RBE_* in rollerball_b.h):
  +0x15 updated, +0x16 drawn, +0x18 in the air, +0x1a steerable, +0x1b, +0x1c the
  controller runs, +0x1d the shadow is drawn, +0x1e (bytes)
  +0x40c8 radius (copy)      +0x416c/+0x4170 roll force in the air / on the ground
  +0x4198 time in the air    +0x41a8 input map, +0x41ac/+0x41b0 stick x/y,
  +0x41b4/+0x41b8 buttons    +0x41bc byte
  +0x41cc ball, +0x41d0 character, +0x41d4 character ("opt" model), +0x41d8 hover pod,
  +0x41dc shadow ball (actor instances); +0x41e0 the character instance shown
  +0x41e8 state (0 play, 1 on a floor, 2 falling)   +0x41f0 floor height
  +0x4218 the character's rotation   +0x422c animations[16] (names at 0x002b5288)
  +0x426c team mate   +0x4270/+0x4274 rolling sound started / its channel
  +0x4278 brightness (0x100 normal)
*/

#include "rollerball_b.h"

#define RBPL_UPDATED 0x15
#define RBPL_DRAWN 0x16
#define RBPL_AIRBORNE 0x18
#define RBPL_STEERABLE 0x1a
#define RBPL_AI_RUNS 0x1c
#define RBPL_SHADOW 0x1d
#define RBPL_RADIUS2 0x40c8
#define RBPL_AIR_FORCE 0x416c
#define RBPL_ROLL_FORCE 0x4170
#define RBPL_AIR_TIME 0x4198
#define RBPL_INPUT 0x41a8
#define RBPL_STICK_X 0x41ac
#define RBPL_STICK_Y 0x41b0
#define RBPL_BUTTON1 0x41b4
#define RBPL_BUTTON2 0x41b8
#define RBPL_INST_BALL 0x41cc
#define RBPL_INST_CHAR 0x41d0
#define RBPL_INST_CHAR_OPT 0x41d4
#define RBPL_INST_POD 0x41d8
#define RBPL_INST_SHADOW 0x41dc
#define RBPL_INST_SHOWN 0x41e0
#define RBPL_STATE 0x41e8
#define RBPL_FLOOR 0x41f0
#define RBPL_CHAR_ROT 0x4218
#define RBPL_ANIMS 0x422c
#define RBPL_TEAMMATE 0x426c
#define RBPL_ROLL_ON 0x4270
#define RBPL_ROLL_CHANNEL 0x4274
#define RBPL_BRIGHTNESS 0x4278

/* per-variation settings (records of 0x40 bytes from 0x002b4ba8): the scenery is drawn,
the character rides on the ball */
#define rbb_SceneryOn() (RBD_I(0x002b4ba8 + rbb_Variation * 0x40) != 0)
#define rbb_RiderOn() (RBD_I(0x002b4bac + rbb_Variation * 0x40) != 0)
/* per-variation ball physics (records of 0x28 bytes from 0x002b5130) */
#define rbb_Physics(addr0) RBD_F((addr0) + rbb_Variation * 0x28)

/* core: team game, debug free camera, sole-player debug view, turbo cheat */
extern int DAT_002baebc;
extern int DAT_002ba56c;
extern int DAT_00293928;
extern int DAT_00283ea4;

static const char rbb_PlayerFile[] = "C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerBall_Player.cpp";

/* 0x00274c08  CRollerBall_Player vtable */
const void *const rbb_vtable_00274c08[10] = {
	(const void *)FUN_001990f0, /* +0x00 scalar deleting destructor */
	(const void *)FUN_00199110, /* +0x04 shutdown */
	(const void *)FUN_00199f30, /* +0x08 update */
	(const void *)FUN_00198f10, /* +0x0c draw */
	(const void *)FUN_001d5950, /* +0x10 (empty) */
	(const void *)FUN_001d5950, /* +0x14 (empty) */
	(const void *)FUN_00198a30, /* +0x18 input set-up */
	(const void *)FUN_00199170, /* +0x1c control */
	(const void *)FUN_00198e30, /* +0x20 input free */
	(const void *)FUN_00199040, /* +0x24 draw (reflection) */
};

/* place an actor instance: rotation `rot` (quaternion) at `pos` */
static void rbb_PlaceActor(void *inst, const float *rot, const float *pos)
{
	memcpy(RBB_V(inst, 0xe0), rot, 16);
	memcpy(RBB_V(inst, 0xc0), pos, 16);
}

/* 0x00198a00  RollerBall/CRollerBall_Player.cpp (inferred)
Constructor. */
void *CRollerballPlayer_Ctor(void *self)
{
	CRollerballEntity_Ctor(self);
	*(const void *const **)self = rbb_vtable_00274c08;
	return self;
}

/* 0x00198a20  RollerBall/CRollerBall_Player.cpp (inferred)
Destructor. */
void CRollerballPlayer_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274c08;
	CRollerballEntity_Dtor(self);
}

/* 0x00198a30  RollerBall/CRollerBall_Player.cpp (inferred)
Input set-up for pad `pad` (0-3): the stick and two buttons on the pad, plus keyboard
keys for pads 0-3. */
int CRollerballPlayer_InitInput(void *self, int pad)
{
	struct biMap *map = (struct biMap *)biCreateMap(4);
	struct biChannel *lr;
	struct biChannel *ud;
	struct biChannel *b1;
	struct biChannel *b2;

	RBB_P(self, RBPL_INPUT) = map;
	RBB_P(self, RBPL_STICK_X) = biCreateChannel(map, "left/right", 3, 0, 0, 0, 0);
	RBB_P(self, RBPL_STICK_Y) = biCreateChannel((struct biMap *)RBB_P(self, RBPL_INPUT), "up/down", 3, 0, 0, 0, 0);
	RBB_P(self, RBPL_BUTTON1) = biCreateChannel((struct biMap *)RBB_P(self, RBPL_INPUT), "action1", 1, 1, 0, 0, 0);
	RBB_P(self, RBPL_BUTTON2) = biCreateChannel((struct biMap *)RBB_P(self, RBPL_INPUT), "action2", 1, 1, 0, 0, 0);
	lr = RBB_P(self, RBPL_STICK_X);
	ud = RBB_P(self, RBPL_STICK_Y);
	b1 = RBB_P(self, RBPL_BUTTON1);
	b2 = RBB_P(self, RBPL_BUTTON2);
	switch (pad) {
	case 0:
		biBindInput(lr, 0x1400009d, 0x1400009c);
		biBindInput(ud, 0x1400009a, 0x1400009b);
		biBindInput(b1, 0x69, 0);
		biBindInput(b2, 0x67, 0);
		biBindInput(lr, 0xc0, 0xbf);
		biBindInput(ud, 0xbd, 0xbe);
		biBindInput(lr, 0xb9, 0);
		biBindInput(ud, 0x1ba, 0);
		biBindInput(b1, 0xc5, 0);
		biBindInput(b2, 0xc6, 0);
		break;
	case 1:
		biBindInput(lr, 0x14000048, 0x14000045);
		biBindInput(ud, 0x1400005b, 0x14000057);
		biBindInput(b1, 0x68, 0);
		biBindInput(b2, 0x66, 0);
		biBindInput(lr, 0x100c0, 0x100bf);
		biBindInput(ud, 0x100bd, 0x100be);
		biBindInput(lr, 0x100b9, 0);
		biBindInput(ud, 0x101ba, 0);
		biBindInput(b1, 0x100c5, 0);
		biBindInput(b2, 0x100c6, 0);
		break;
	case 2:
		biBindInput(lr, 0x14000050, 0x1400004e);
		biBindInput(ud, 0x1400004d, 0x1400004f);
		biBindInput(b1, 0x4c, 0);
		biBindInput(b2, 0x52, 0);
		biBindInput(lr, 0x200c0, 0x200bf);
		biBindInput(ud, 0x200bd, 0x200be);
		biBindInput(lr, 0x200b9, 0);
		biBindInput(ud, 0x201ba, 0);
		biBindInput(b1, 0x200c5, 0);
		biBindInput(b2, 0x200c6, 0);
		break;
	case 3:
		biBindInput(lr, 0x14000088, 0x14000086);
		biBindInput(ud, 0x1400008a, 0x14000087);
		biBindInput(b1, 0x83, 0);
		biBindInput(b2, 0x82, 0);
		biBindInput(lr, 0x300c0, 0x300bf);
		biBindInput(ud, 0x300bd, 0x300be);
		biBindInput(lr, 0x300b9, 0);
		biBindInput(ud, 0x301ba, 0);
		biBindInput(b1, 0x300c5, 0);
		biBindInput(b2, 0x300c6, 0);
		break;
	}
	return 1;
}

/* 0x00198e30  RollerBall/CRollerBall_Player.cpp (inferred)
Free the input channels and map. */
void CRollerballPlayer_FreeInput(void *self)
{
	biDeleteChannel((struct biChannel *)RBB_P(self, RBPL_STICK_X));
	biDeleteChannel((struct biChannel *)RBB_P(self, RBPL_STICK_Y));
	biDeleteChannel((struct biChannel *)RBB_P(self, RBPL_BUTTON1));
	biDeleteChannel((struct biChannel *)RBB_P(self, RBPL_BUTTON2));
	biDeleteMap((struct biMap *)RBB_P(self, RBPL_INPUT));
}

/* 0x00198e80  RollerBall/CRollerBall_Player.cpp (inferred)
In a team game: the first other player of the same team is the team mate. */
void CRollerballPlayer_FindTeamMate(void *self)
{
	char *e;
	int i;
	int me;

	if (DAT_002baebc == 0) {
		RBB_P(self, RBPL_TEAMMATE) = NULL;
		return;
	}
	e = rbb_FirstPlayer;
	if (e == NULL)
		return;
	me = RBB_I(self, RBE_PLAYER);
	/* the team of the i-th entity in the list (list order is player order) */
	for (i = 0; RBB_I(e, RBE_PLAYER) == me || rbb_PlayerTeam(me) != rbb_PlayerTeam(i); i++) {
		e = RBB_P(e, RBE_NEXT);
		if (e == NULL)
			return;
	}
	RBB_P(self, RBPL_TEAMMATE) = e;
	bkPrintf("TEAMMODE: FoundTeamMate %d for player %d\n", i, me);
}

/* 0x00198f10  RollerBall/CRollerBall_Player.cpp (inferred)
Draw: the ball, the rider (when the variation shows it and it is not the sole-player
view of this player) and the hover pod; darker or brighter when +0x4278 is not 0x100. */
void CRollerballPlayer_Draw(void *self)
{
	int i;

	if (RBB_C(self, RBPL_DRAWN) == 0)
		return;
	if (RBB_I(self, RBPL_BRIGHTNESS) != 0x100) {
		bdPushRenderState(6, 1, 0);
		bdSetRenderState(10, 0, 0);
		baSetBrightness(RBB_I(self, RBPL_BRIGHTNESS));
	}
	if (!rbb_SceneryOn()) {
		for (i = 0; i < 5; i++)
			if (i == 0)
				baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_BALL), 7);
	} else {
		for (i = 0; i < 5; i++) {
			if (i == 0) {
				baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_BALL), 1);
				baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_BALL), 2);
			} else if (i == 1) {
				if (rbb_RiderOn() && (RBB_I(rbb_Game, 0x44) == 0 || RBB_P(rbb_Game, 0x48) != self)) {
					baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN), 1);
					baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN), 2);
				}
			} else if (i == 3) {
				baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_POD), 7);
			}
		}
	}
	if (RBB_I(self, RBPL_BRIGHTNESS) != 0x100) {
		baSetBrightness(0x100);
		bdPopRenderState(6);
	}
}

/* 0x00199040  RollerBall/CRollerBall_Player.cpp (inferred)
Draw for the reflection pass: the ball, and the rider when `rider` is set. */
void CRollerballPlayer_DrawReflection(void *self, int rider)
{
	int i;

	if (RBB_C(self, RBPL_DRAWN) == 0 || !rbb_SceneryOn())
		return;
	for (i = 0; i < 5; i++) {
		if (i == 0) {
			baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_BALL), 1);
			baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_BALL), 2);
		} else if (i == 1 && rider != 0 && rbb_RiderOn()) {
			baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN), 1);
			baDrawActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN), 2);
		}
	}
}

/* 0x001990e0  RollerBall/CRollerBall_Player.cpp (inferred)
The heading to turn to. */
void CRollerballPlayer_SetTargetHeading(void *self, float heading)
{
	RBB_F(self, RBE_HEADING2) = heading;
}

/* 0x001990f0  RollerBall/CRollerBall_Player.cpp (inferred)
Scalar deleting destructor. */
void *CRollerballPlayer_DeletingDtor(void *self, unsigned int flags)
{
	CRollerballPlayer_Dtor(self);
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x00199110  RollerBall/CRollerBall_Player.cpp (inferred)
Shutdown: the five actor instances, the controller (virtual +0x0c, then freed) and the
input (virtual +0x20). */
void CRollerballPlayer_Shutdown(void *self)
{
	void *ctl;
	int i;

	for (i = 0; i < 5; i++)
		baDeleteActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_BALL + i * 4));
	ctl = RBB_P(self, RBE_CONTROLLER);
	if (ctl != NULL) {
		RBB_VFN(ctl, 0x0c, void (*)(void *))(ctl);
		RBB_DELETE(ctl);
		RBB_P(self, RBE_CONTROLLER) = NULL;
	}
	RBB_VFN(self, 0x20, void (*)(void *))(self);
}

/* 0x00199170  RollerBall/CRollerBall_Player.cpp (inferred)
Control: a human steers with the stick relative to the camera (or, in the sole-player
view, relative to the ball's heading); a computer player runs its controller
(virtual +0x04). The steering is a push of the roll force (air or ground). */
void CRollerballPlayer_Control(void *self, float dt)
{
	float dir[4];
	float fwd[4];
	float v[4];
	int sx;
	int sy;

	RBB_B(self, 0x41bc) = 0;
	if (RBB_C(self, RBE_COMPUTER) != 0) {
		if (RBB_C(self, RBPL_AI_RUNS) != 0) {
			void *ctl = RBB_P(self, RBE_CONTROLLER);
			RBB_VFN(ctl, 0x04, void (*)(void *, float))(ctl, dt);
		}
		return;
	}
	if (RBB_C(self, RBPL_STEERABLE) == 0)
		return;
	sx = RBB_I(RBB_P(self, RBPL_STICK_X), 0x14);
	sy = RBB_I(RBB_P(self, RBPL_STICK_Y), 0x14);
	if (RBB_I(rbb_Game, 0x44) == 0 || RBB_P(rbb_Game, 0x48) != self) {
		double cam;
		float a;
		float force;
		if (sx == 0 && sy == 0)
			return;
		/* game + 0x1374/0x137c: the camera's eye x and z */
		cam = atan2((double)RBB_F(rbb_Game, 0x1374) - (double)RBB_F(self, RBE_POS + 0),
			(double)RBB_F(rbb_Game, 0x137c) - (double)RBB_F(self, RBE_POS + 8));
		fwd[0] = 0.0f;
		fwd[1] = 0.0f;
		fwd[2] = 1.0f;
		fwd[3] = 1.0f;
		a = (float)(atan2((double)sx, (double)sy) + cam + (double)3.1415925f);
		bmVectorRotateY(dir, fwd, a);
		bmVectorNormalise(v, dir);
		force = RBB_C(self, RBPL_AIRBORNE) == 0 ? RBB_F(self, RBPL_ROLL_FORCE) : RBB_F(self, RBPL_AIR_FORCE);
		v[0] = v[0] * force;
		RBB_F(self, RBE_HEADING2) = a;
		v[1] = v[1] * force;
		v[2] = v[2] * force;
		v[3] = 0.0f;   /* not set in the original */
		CRollerballEntity_SetPush(self, v);
	} else {
		float stick;
		float a;
		if (sx == 0 && sy == 0)
			return;
		fwd[0] = 0.0f;
		stick = (float)atan2((double)sx, (double)sy);
		fwd[1] = 0.0f;
		fwd[2] = 1.0f;
		fwd[3] = 1.0f;
		a = (float)(atan2((double)sx, (double)sy) + (double)RBB_F(self, RBE_HEADING));
		bmVectorRotateY(dir, fwd, a);
		bmVectorNormalise(v, dir);
		cmb_vec3_scale(v, v, RBB_C(self, RBPL_AIRBORNE) == 0 ? RBB_F(self, RBPL_ROLL_FORCE)
			: RBB_F(self, RBPL_AIR_FORCE));
		if (stick <= 2.3561945f || 3.92699075f <= stick)
			RBB_F(self, RBE_HEADING2) = a;
		v[3] = 0.0f;   /* not set in the original */
		CRollerballEntity_SetPush(self, v);
	}
}

/* 0x001993b0  RollerBall/CRollerBall_Player.cpp (inferred)
Turn the character towards the target heading (faster the further it is) and set its
rotation. */
void CRollerballPlayer_Turn(void *self, float dt)
{
	float delta = CLMath_AngleDelta(RBB_F(self, RBE_HEADING), RBB_F(self, RBE_HEADING2));
	float axis_angle[4];

	RBB_F(self, RBE_HEADING) = Dc_TurnTowards(RBB_F(self, RBE_HEADING), RBB_F(self, RBE_HEADING2),
		dt * 0.0250000004f * fabsf(delta));
	axis_angle[0] = 0.0f;
	axis_angle[1] = 1.0f;
	axis_angle[2] = 0.0f;
	axis_angle[3] = -(RBB_F(self, RBE_HEADING) + 3.14159274f);
	bmAxisAngleToQuaternion(RBB_V(self, RBPL_CHAR_ROT), axis_angle);
}

/* 0x00199460  RollerBall/CRollerBall_Player.cpp (inferred)
State 1 (on a floor at +0x41f0): below it, push up; just above it and nearly still,
stop falling. */
void CRollerballPlayer_UpdateFloor(void *self)
{
	float y = RBB_F(self, RBE_POS + 4);

	if (y <= RBB_F(self, RBPL_FLOOR)) {
		float push[4];
		RBB_F(self, RBE_FORCE + 4) = -0.5f;
		RBB_U(self, RBE_DRAG) = 0x3dcccccdu;   /* 0.1 */
		push[0] = 0.0f;
		push[1] = 1.5f;
		push[2] = 0.0f;
		push[3] = 0.0f;   /* not set in the original */
		CRollerballEntity_SetPush(self, push);
		return;
	}
	if (RBB_F(self, RBPL_FLOOR) <= y && y < RBB_F(self, RBPL_FLOOR) + 5.0f &&
		fabsf(RBB_F(self, RBE_VEL + 4)) < 1.0f) {
		RBB_F(self, RBE_FORCE + 4) = 0.0f;
		RBB_F(self, RBE_VEL + 4) = 0.0f;
	}
}

/* pop the event at the head of the queue (when there is one) */
static void rbb_PopEvent(void *self)
{
	if (RBB_US(self, 0x0c) == 0)
		return;
	RBB_US(self, 0x0a) = (unsigned short)((RBB_US(self, 0x08) - 1) & (RBB_US(self, 0x0a) + 1));
	RBB_US(self, 0x0c) = (unsigned short)(RBB_US(self, 0x0c) - 1);
}

/* 0x00199530  RollerBall/CRollerBall_Player.cpp (inferred)
Handle the first queued event (CRollerBall_EntityEvent): 0 hit by a ball (+0x08 the
other entity, +0x0c relative velocity, +0x1c contact normal): sound, sparks, a voice
sample and rumble by the impact speed; 2 hit the floor (+0x08 velocity): sound, voice,
rumble; 3 play animation +0x0c (+0x08 blend in over +0x10, +0x09 loop); other events are
dropped. `dt` is not used. */
void CRollerballPlayer_ProcessEvents(void *self, float dt)
{
	char *ev;
	char *rumble = rbb_Game + 0x28f0;

	(void)dt;
	if (RBB_US(self, 0x0c) == 0)
		return;
	ev = (char *)RBB_P(self, 0x04) + RBB_US(self, 0x0a) * 0x2c;
	switch (RBB_US(ev, 0x04)) {
	case 0: {
		float speed = (float)sqrt((double)RBB_F(ev, 0x0c) * RBB_F(ev, 0x0c) +
			(double)RBB_F(ev, 0x14) * RBB_F(ev, 0x14));
		if (10.0f <= speed) {
			float k = 50.0f / speed;
			if (1.0f < k)
				k = 1.0f;
			else if (k < 0.0f)
				k = 0.0f;
			Core_Rand();   /* the result is not used */
			rbb_PlaySound(RBB_SND_BALL2BALL1, RBB_FTOL(k * 127.0f), 0, 0x5622, NULL);
		}
		if (0.1f <= speed) {
			char *e = (char *)RBB_P(self, 0x04) + RBB_US(self, 0x0a) * 0x2c + 0x1c;
			float normal[4];
			float at[4];
			normal[0] = -RBB_F(e, 0x00);
			normal[3] = RBB_F(e, 0x0c);
			normal[1] = -RBB_F(e, 0x04);
			normal[2] = -RBB_F(e, 0x08);
			cmb_vec3_scale(at, normal, RBB_F(self, RBE_RADIUS) * 0.949999988f);
			cmb_vec3_add(at, at, RBB_V(self, RBE_POS));
			if (speed * 0.0334448181f != 0.0f)   /* rollerball_a 0x0018fff0: spot effects */
				FUN_0018fff0(rbb_Game + 0x2060, at,
					normal, 0.1f);
		}
		if (20.0f <= speed && (float)Core_Rand() * 3.05185094e-05f < 0.25f)
			CLSamples_PlayFromSet(RBB_C(self, RBE_CHARACTER), 2, RBB_V(self, RBE_POS), DAT_00293928, 0,
				-1, 0);
		if (15.0f <= speed) {
			float k = speed * 0.0199999996f;
			char *other;
			if (k < 0.0f)
				k = 0.0f;
			else if (1.0f < k)
				k = 1.0f;
			CRollerBallRumble_Long(rumble, RBB_I(self, RBE_PLAYER), k);
			other = RBB_P((char *)RBB_P(self, 0x04) + RBB_US(self, 0x0a) * 0x2c, 0x08);
			CRollerBallRumble_Long(rumble, RBB_I(other, RBE_PLAYER), k);
		}
		break;
	}
	case 2:
		if (0.1f <= cmb_vec3_length(RBB_V(ev, 0x08))) {
			Core_Rand();   /* the result is not used */
			rbb_PlaySound(RBB_SND_BALL2FLOOR1, 0xff, 0, 0x5622, NULL);
			CLSamples_PlayFromSet(RBB_C(self, RBE_CHARACTER), 1, RBB_V(self, RBE_POS), DAT_00293928, 0,
				-1, 0);
			CRollerBallRumble_Short(rumble, RBB_I(self, RBE_PLAYER), 1.0f);
		}
		break;
	case 3: {
		float blend = RBB_F(ev, 0x10);
		int loop = RBB_B(ev, 0x09);
		int idx = RBB_I(ev, 0x0c);
		void *inst = RBB_P(self, RBPL_INST_SHOWN);
		void *node = RBB_P(inst, 0x134);
		void *anim = RBB_P(self, RBPL_ANIMS + idx * 4);
		if (RBB_C(ev, 0x08) != 0) {
			/* blend in (mode 3); the original has the same call for both loop values */
			baFlushAnimations(inst, node, 1, 0);
			if (anim != NULL)
				bQueueAnim((struct BActorInstance *)inst, (struct BActorNode *)node, 3, (struct BAnim *)anim,
					NULL, 0x400, 0x400, -1, -1, blend, 1, NULL, NULL);
		} else if (anim != NULL) {
			baFlushAnimations(inst, node, 1, 0);
			bQueueAnim((struct BActorInstance *)inst, (struct BActorNode *)node, 1, (struct BAnim *)anim,
				NULL, 0x400, 0x400, -1, -1, 0.0f, loop != 0 ? 1u : 0u, NULL, NULL);
		}
		break;
	}
	default:
		break;
	}
	rbb_PopEvent(self);
}

/* 0x001999e0  RollerBall/CRollerBall_Player.cpp (inferred)
Show the character model of `mode` (1: the character +0x41d0; 0: the "opt" model +0x41d4,
not while core's debug free camera is on), look up its 16 animations, start "still",
and place it on the ball. */
void CRollerballPlayer_SetAnimMode(void *self, int mode)
{
	void *inst;
	void *still;
	float off[3];
	int i;

	if (mode == 0) {
		if (DAT_002ba56c == 0) {
			if (RBB_P(self, RBPL_INST_SHOWN) == RBB_P(self, RBPL_INST_CHAR_OPT))
				return;
			RBB_P(self, RBPL_INST_SHOWN) = RBB_P(self, RBPL_INST_CHAR_OPT);
			for (i = 0; i < 16; i++)
				RBB_P(self, RBPL_ANIMS + i * 4) = baFindSubObject(
					(struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN), RBD_SV(0x002b5288)[i]);
		}
	} else {
		if (RBB_P(self, RBPL_INST_SHOWN) == RBB_P(self, RBPL_INST_CHAR))
			return;
		RBB_P(self, RBPL_INST_SHOWN) = RBB_P(self, RBPL_INST_CHAR);
		for (i = 0; i < 16; i++)
			RBB_P(self, RBPL_ANIMS + i * 4) = baFindSubObject(
				(struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN), RBD_SV(0x002b5288)[i]);
	}
	inst = RBB_P(self, RBPL_INST_SHOWN);
	still = RBB_P(self, RBPL_ANIMS + 15 * 4);   /* "still" */
	if (still != NULL) {
		void *node = RBB_P(inst, 0x134);
		baFlushAnimations(inst, node, 1, 0);
		bQueueAnim((struct BActorInstance *)inst, (struct BActorNode *)node, 1, (struct BAnim *)still, NULL,
			0x400, 0x400, -1, -1, 0.0f, 1, NULL, NULL);
	}
	inst = RBB_P(self, RBPL_INST_SHOWN);
	memcpy(RBB_V(inst, 0xe0), RBB_V(self, RBPL_CHAR_ROT), 16);
	/* the rider's offset of the character (0x002b52c8, float[4] each) */
	bmVectorRotateY(off, RBD_FV(0x002b52c8 + RBB_I(self, RBE_CHARACTER) * 0x10),
		RBB_F(self, RBE_HEADING) + 3.14159274f);
	inst = RBB_P(self, RBPL_INST_SHOWN);
	RBB_F(inst, 0xc0) = off[0] + RBB_F(self, RBE_POS + 0);
	RBB_F(inst, 0xc4) = off[1] + RBB_F(self, RBE_POS + 4);
	RBB_F(inst, 0xc8) = off[2] + RBB_F(self, RBE_POS + 8);
	baUpdateAnimations((struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN),
		(struct BActorNode *)RBB_P(RBB_P(self, RBPL_INST_SHOWN), 0x134), 1.0f, NULL);
	baPrepActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN), 1);
	Core_SetCharacterColours(RBB_P(self, RBPL_INST_SHOWN), DAT_00283ea4);
}

/* 0x00199ba0  RollerBall/CRollerBall_Player.cpp
Set the player up: player `player` (its character from core's records), its actor
instances (game + 0x144c ball, + 0x142c character, + 0x143c "opt" character, + 0x145c
hover pod, + 0x1460 shadow ball), the ball body at `pos` with rotation `rot` and mass
`mass`, the variation's ball physics, an event queue of 16, and its heading (away from
the centre in "base", "sumo" and "drop", -45 degrees in "race"). */
int CRollerballPlayer_Init(void *self, int player, float *pos, float *rot, float mass)
{
	void *char_actor;
	void *opt_actor;
	void *pod_actor;
	void *shadow_actor;
	float axis_angle[4];
	int i;

	for (i = 0; i < 5; i++)
		RBB_I(self, 0x41f0 + i * 4) = 0;
	RBB_I(self, RBE_PLAYER) = player;
	RBB_I(self, RBE_CHARACTER) = rbb_PlayerCharacter(player);
	RBB_P(self, RBE_NEXT) = NULL;
	RBB_I(self, RBE_SCORE) = 0;
	RBB_I(self, RBPL_STATE) = 0;
	RBB_I(self, 0x41ec) = 0;
	RBB_I(self, RBPL_AIR_TIME) = 0;
	RBB_I(self, RBPL_BRIGHTNESS) = 0x100;
	RBB_B(self, RBE_ACTIVE) = 1;
	RBB_B(self, RBPL_UPDATED) = 1;
	RBB_B(self, RBE_WINNER) = 1;
	RBB_B(self, RBPL_DRAWN) = 1;
	RBB_B(self, RBPL_STEERABLE) = 1;
	RBB_B(self, 0x1b) = 1;
	RBB_B(self, RBE_COMPUTER) = 0;
	RBB_B(self, RBPL_AI_RUNS) = 0;
	RBB_B(self, RBPL_SHADOW) = 1;
	RBB_B(self, 0x1e) = 0;
	char_actor = RBB_P(rbb_Game, 0x142c + player * 4);
	opt_actor = RBB_P(rbb_Game, 0x143c + player * 4);
	pod_actor = RBB_P(rbb_Game, 0x145c);
	shadow_actor = RBB_P(rbb_Game, 0x1460);
	RBB_P(self, RBPL_INST_BALL) = baInitActorInstance(NULL, (struct BActor *)RBB_P(rbb_Game, 0x144c + player * 4),
		1, rbb_PlayerFile, 0x80);
	RBB_P(self, RBPL_INST_CHAR) = baInitActorInstance(NULL, (struct BActor *)char_actor, 2, rbb_PlayerFile, 0x81);
	RBB_P(self, RBPL_INST_CHAR_OPT) = baInitActorInstance(NULL, (struct BActor *)opt_actor, 2, rbb_PlayerFile, 0x82);
	RBB_P(self, RBPL_INST_POD) = baInitActorInstance(NULL, (struct BActor *)pod_actor, 1, rbb_PlayerFile, 0x83);
	RBB_P(self, RBPL_INST_SHADOW) = baInitActorInstance(NULL, (struct BActor *)shadow_actor, 1, rbb_PlayerFile, 0x84);
	baSetInstanceAnimations((struct BActor *)char_actor, (struct BActorInstance *)RBB_P(self, RBPL_INST_CHAR_OPT));
	RBB_P(self, RBPL_INST_SHOWN) = NULL;
	CDemoBody_InitState((unsigned int *)RBB_ADDR(self, RBE_BODY), (unsigned int *)pos, (unsigned int *)rot, mass);
	RBB_F(self, RBE_RADIUS) = 120.0f;
	RBB_F(self, RBPL_RADIUS2) = 120.0f;
	CRollerballEntity_ClearPush(self);
	RBB_F(self, RBE_FORCE + 0) = 0.0f;
	RBB_F(self, RBE_FORCE + 4) = rbb_Physics(0x002b5130);
	RBB_F(self, RBE_FORCE + 8) = 0.0f;
	RBB_F(self, RBE_DRAG) = rbb_Physics(0x002b5140);
	RBB_F(self, RBE_KINETIC_FRICTION) = rbb_Physics(0x002b5144);
	RBB_F(self, RBE_STATIC_FRICTION) = rbb_Physics(0x002b5148);
	RBB_F(self, RBPL_AIR_FORCE) = rbb_Physics(0x002b5134);
	RBB_F(self, RBPL_ROLL_FORCE) = rbb_Physics(0x002b5138);
	RBB_F(self, 0x4174) = rbb_Physics(0x002b514c);
	RBB_F(self, 0x4178) = rbb_Physics(0x002b5150);
	RBB_F(self, 0x417c) = rbb_Physics(0x002b5154);
	CRollerBallEntityEventSystem_InitQueue(self, 0x10);
	bmQuaternionToAxisAngle(axis_angle, rot);
	RBB_I(self, 0x4228) = 0;
	RBB_F(self, RBE_HEADING) = axis_angle[3] + 3.14159274f;
	switch (rbb_Variation) {
	case 0:
	case 1:
	case 4: {
		float h = (float)(atan2((double)pos[0], (double)pos[2]) + (double)3.14159274f);
		RBB_F(self, RBE_HEADING2) = h;
		RBB_F(self, RBE_HEADING) = h;
		break;
	}
	case 3:
		RBB_F(self, RBE_HEADING2) = -0.785398185f;
		RBB_F(self, RBE_HEADING) = -0.785398185f;
		axis_angle[0] = 0.0f;
		axis_angle[1] = 1.0f;
		axis_angle[2] = 0.0f;
		axis_angle[3] = -2.3561945f;
		bmAxisAngleToQuaternion(RBB_V(self, RBE_ROTATION), axis_angle);
		break;
	}
	axis_angle[3] = -(RBB_F(self, RBE_HEADING) + 3.14159274f);
	RBB_F(self, RBE_HEADING2) = RBB_F(self, RBE_HEADING);
	RBB_P(self, RBPL_TEAMMATE) = NULL;
	RBB_I(self, RBPL_ROLL_ON) = 0;
	axis_angle[0] = 0.0f;
	axis_angle[1] = 1.0f;
	axis_angle[2] = 0.0f;
	bmAxisAngleToQuaternion(RBB_V(self, RBPL_CHAR_ROT), axis_angle);
	CRollerballPlayer_SetAnimMode(self, 1);
	return 1;
}

/* 0x00199f30  RollerBall/CRollerBall_Player.cpp (inferred)
Per-frame update (virtual +0x08): events, the floor states (falling below -10000 takes
the player out), turning, the actor instances (ball, rider, hover pod 40 below, shadow
ball), the sound listener of the player slot, and the rolling sound (pitch and volume by
the ground speed; silent in the air for more than 20). */
void CRollerballPlayer_Update(void *self, float dt)
{
	void *inst;
	int state;
	int i;
	float speed;
	float k;
	int freq;

	if (RBB_C(self, RBPL_UPDATED) == 0)
		return;
	CRollerballPlayer_ProcessEvents(self, dt);
	state = RBB_I(self, RBPL_STATE);
	if (state == 1) {
		CRollerballPlayer_UpdateFloor(self);
	} else if (state == 2) {
		if (RBB_F(self, RBE_POS + 4) <= -10000.0f) {
			RBB_B(self, RBPL_UPDATED) = 0;
			RBB_B(self, RBPL_DRAWN) = 0;
			RBB_B(self, 0x1b) = 0;
			RBB_B(self, RBPL_AI_RUNS) = 0;
			RBB_B(self, RBPL_STEERABLE) = 0;
		}
	}
	if (state == 0 || state == 1 || state == 2)
		CRollerballPlayer_Turn(self, dt);

	if (!rbb_SceneryOn()) {
		for (i = 0; i < 5; i++) {
			if (i == 0 || i == 4) {
				inst = RBB_P(self, i == 0 ? RBPL_INST_BALL : RBPL_INST_SHADOW);
				rbb_PlaceActor(inst, RBB_V(self, RBE_ROTATION), RBB_V(self, RBE_POS));
				baPrepActorInstance((struct BActorInstance *)inst, 1);
			}
		}
	} else {
		for (i = 0; i < 5; i++) {
			switch (i) {
			case 0:
				inst = RBB_P(self, RBPL_INST_BALL);
				rbb_PlaceActor(inst, RBB_V(self, RBE_ROTATION), RBB_V(self, RBE_POS));
				baPrepActorInstance((struct BActorInstance *)inst, 1);
				break;
			case 1:
				if (rbb_RiderOn()) {
					float off[3];
					inst = RBB_P(self, RBPL_INST_SHOWN);
					memcpy(RBB_V(inst, 0xe0), RBB_V(self, RBPL_CHAR_ROT), 16);
					bmVectorRotateY(off, RBD_FV(0x002b52c8 + RBB_I(self, RBE_CHARACTER) * 0x10),
						RBB_F(self, RBE_HEADING) + 3.14159274f);
					inst = RBB_P(self, RBPL_INST_SHOWN);
					RBB_F(inst, 0xc0) = off[0] + RBB_F(self, RBE_POS + 0);
					RBB_F(inst, 0xc4) = off[1] + RBB_F(self, RBE_POS + 4);
					RBB_F(inst, 0xc8) = off[2] + RBB_F(self, RBE_POS + 8);
					inst = RBB_P(self, RBPL_INST_SHOWN);
					if (RBB_S(rbb_Game, 0x24) != 2)   /* not while playing */
						baUpdateAnimations((struct BActorInstance *)inst,
							(struct BActorNode *)RBB_P(inst, 0x134), dt, NULL);
					baPrepActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_SHOWN), 1);
				}
				break;
			case 3:
				inst = RBB_P(self, RBPL_INST_POD);
				rbb_PlaceActor(inst, RBB_V(self, RBPL_CHAR_ROT), RBB_V(self, RBE_POS));
				RBB_F(RBB_P(self, RBPL_INST_POD), 0xc4) = RBB_F(RBB_P(self, RBPL_INST_POD), 0xc4) - 40.0f;
				baPrepActorInstance((struct BActorInstance *)RBB_P(self, RBPL_INST_POD), 1);
				break;
			case 4:
				inst = RBB_P(self, RBPL_INST_SHADOW);
				rbb_PlaceActor(inst, RBB_V(self, RBE_ROTATION), RBB_V(self, RBE_POS));
				baPrepActorInstance((struct BActorInstance *)inst, 1);
				break;
			}
		}
	}

	/* bumpercars_a 0x00082a20-0x00082ae0: the sound listener position of slot 0-3 */
	switch (RBB_I(self, RBE_PLAYER)) {
	case 0:
		DebrisColours_Set0(RBB_V(self, RBE_POS));
		break;
	case 1:
		DebrisColours_Set1(RBB_V(self, RBE_POS));
		break;
	case 2:
		DebrisColours_Set2(RBB_V(self, RBE_POS));
		break;
	case 3:
		DebrisColours_Set3(RBB_V(self, RBE_POS));
		break;
	}

	if (RBB_I(self, RBPL_ROLL_ON) == 0) {
		int ch = rbb_PlaySound(RBB_SND_BALLROLL, 0xc0, 0, 0, NULL);
		RBB_I(self, RBPL_ROLL_CHANNEL) = ch;
		CRollerBallSfx_SetVolume(ch, 0);
		RBB_I(self, RBPL_ROLL_ON) = 1;
		return;
	}
	speed = 0.0f;
	if (RBB_F(self, RBPL_AIR_TIME) <= 20.0f)
		speed = (float)sqrt((double)RBB_F(self, RBE_VEL + 0) * RBB_F(self, RBE_VEL + 0) +
			(double)RBB_F(self, RBE_VEL + 8) * RBB_F(self, RBE_VEL + 8));
	k = speed * 0.0166666675f;
	if (1.0f < k)
		k = 1.0f;
	else if (k < 0.0f)
		k = 0.0f;
	freq = RBB_FTOL(k * 22050.0f);
	if (freq < 0xbc)
		freq = 0xbc;
	CRollerBallSfx_SetFrequency(RBB_I(self, RBPL_ROLL_CHANNEL), freq);
	k = speed * 0.0500000007f;
	if (1.0f < k)
		k = 1.0f;
	else if (k < 0.0f)
		k = 0.0f;
	CRollerBallSfx_SetVolume(RBB_I(self, RBPL_ROLL_CHANNEL), RBB_FTOL(k * 192.0f));
}
