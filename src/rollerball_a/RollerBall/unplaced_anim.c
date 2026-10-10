/*
RollerBall/unplaced_anim.c  <-  C:\BlitzParty\Game\Src\RollerBall\ (file unknown)

Two gap functions in front of CModule_RollerBall.cpp: per-frame callbacks of a CharLib
character (charlib actor at +0x30, a sub-object at +0x6c, velocity at +0x7c, timers at
+0x12fc..+0x1310). Both are cdecl and return 0.
*/

#include "rollerball_a.h"

/* 0x00187570  RollerBall (gap)
`first`: play animation 0x11. While the timer at +0x1300 runs, the velocity is the vector
(0, 0, -speed +0x1308) turned by the heading +0x1310; the timer counts down in frames.
At 0 the velocity is cleared. */
int RollerBall_CharPush(unsigned char *self, int first)
{
	float v[4];
	float t;

	if (first != 0)
		Actor_PlayAnim(RB_PTR(self, 0x30), 0x11, 0, 0, 1.0f, 0.1f, 1);
	RB_I32(RB_PTR(self, 0x6c), 0x14) = 0;
	if (RB_F32(self, 0x1300) == 0.0f) {
		RB_F32(self, 0x84) = 0.0f;
		RB_F32(self, 0x80) = 0.0f;
		RB_F32(self, 0x7c) = 0.0f;
	} else {
		v[2] = -RB_F32(self, 0x1308);
		v[0] = 0.0f;
		v[1] = 0.0f;
		v[3] = 0.0f;
		bmVectorRotateY(v, v, RB_F32(self, 0x1310));
		RB_F32(self, 0x7c) = v[0];
		RB_F32(self, 0x80) = v[1];
		RB_F32(self, 0x84) = v[2];
		RB_F32(self, 0x88) = v[3];
		t = RB_F32(self, 0x1300) - DAT_0028fda8;
		RB_F32(self, 0x1300) = t;
		if (t < 0.0f)
			RB_F32(self, 0x1300) = 0.0f;
	}
	return 0;
}

/* 0x00187660  RollerBall (gap)
When +0x40 > +0x12fc: animation state 0x14 plays 0x16, 0x13 plays 0x15 (and the state is
cleared); the actor's +0x234 is cleared. Then gravity on the y velocity and the same timed
push as 0x00187570 with the timer +0x1304 and speed +0x130c. */
int RollerBall_CharFall(unsigned char *self)
{
	float v[4];
	float t;
	short state;

	if (RB_F32(self, 0x40) > RB_F32(self, 0x12fc)) {
		state = RB_I16(self, 0x12f2);
		if (state == 0x14) {
			Actor_PlayAnim(RB_PTR(self, 0x30), 0x16, 0, 0, 1.0f, 0.1f, 1);
			RB_I16(self, 0x12f2) = 0;
		} else if (state == 0x13) {
			Actor_PlayAnim(RB_PTR(self, 0x30), 0x15, 0, 0, 1.0f, 0.1f, 1);
			RB_I16(self, 0x12f2) = 0;
		}
		RB_I16(RB_PTR(self, 0x30), 0x234) = 0;
	}
	RB_F32(self, 0x80) = RB_F32(self, 0x80) - DAT_0028fda8 * DAT_00292c84 * 0.5f;
	RB_I32(RB_PTR(self, 0x6c), 0x14) = 0;
	if (RB_F32(self, 0x1304) == 0.0f) {
		RB_F32(self, 0x84) = 0.0f;
		RB_F32(self, 0x80) = 0.0f;
		RB_F32(self, 0x7c) = 0.0f;
	} else {
		v[2] = -RB_F32(self, 0x130c);
		v[0] = 0.0f;
		v[1] = 0.0f;
		v[3] = 0.0f;
		bmVectorRotateY(v, v, RB_F32(self, 0x1310));
		RB_F32(self, 0x7c) = v[0];
		RB_F32(self, 0x84) = v[2];
		RB_F32(self, 0x80) = v[1];
		RB_F32(self, 0x88) = v[3];
		t = RB_F32(self, 0x1304) - DAT_0028fda8;
		RB_F32(self, 0x1304) = t;
		if (t < 0.0f)
			RB_F32(self, 0x1304) = 0.0f;
	}
	return 0;
}
