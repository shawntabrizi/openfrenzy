/*
CarryOn/CFire_Camera.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Camera.cpp

The camera of the Fireworks game (struct CFireCamera, g_fireCam). It starts with the
level's intro camera (classlib_ui CIntroCam, skippable with "skip"), then follows the
game: the charlib CCamera on the players (variants 0 and 1, mode 3), a camera on the
players' bounds (variant 2, mode 1) or on the fireworks (variant 3, mode 2). At the end it
cycles over the winners (modes 4/5) and fades out.

0x000c89b0 is tagged with CFire_Camera.cpp; the functions from 0x000c8250 to 0x000c91a0
work on the same object (file inferred).
*/

#include "carryon_a.h"

/* 0x000c8250  CarryOn/CFire_Camera.cpp (file inferred)
Leaves the intro: sets the game camera of the variant (variant 3: mode 2, variant 2:
mode 1, else mode 3, the charlib camera). */
void CFire_Camera_StartGame(struct CFireCamera *self)
{
	if (self->variant == 3 || self->variant == 2)
	{
		self->target[3] = 1.0f;
		self->targetGoal[3] = 1.0f;
		self->eye[3] = 1.0f;
		self->eyeGoal[3] = 1.0f;
		self->target[0] = 0.0f;
		self->target[1] = 0.0f;
		self->target[2] = 0.0f;
		self->eye[0] = 2000.0f;
		self->eye[1] = 2000.0f;
		self->eye[2] = -2000.0f;
		self->mode = self->variant == 3 ? 2 : 1;
		self->zoom = 0.0f;
		self->hold = 0.0f;
		return;
	}
	if (self->variant == 0)
	{
		self->eye[0] = 1000.0f;
		self->eye[1] = 1000.0f;
		self->eye[2] = -1000.0f;
	}
	self->mode = 3;
}

/* 0x000c8340  CarryOn/CFire_Camera.cpp (file inferred)
1 when no fade runs: no end fade-out, the fade-in done, the screen fully lit. */
int CFire_Camera_IsReady(struct CFireCamera *self)
{
	if (self->fadeOut == 0 && self->fadeIn == 0 && 1.0f <= self->volume)
		return 1;
	return 0;
}

/* 0x000c8370  CarryOn/CFire_Camera.cpp (file inferred)
Draws the fades: the screen fade-in (classlib_ui), the darkening while `volume` is below
1, and the end fade-out. */
void CFire_Camera_DrawFade(struct CFireCamera *self, float step)
{
	float fade;
	int alpha;

	if (self->fadeIn != 0 && CLFade_Draw(step) != 0)
		self->fadeIn = 0;
	if (self->frames < 3)
		self->frames++;
	if (self->volume < 1.0f)
	{
		alpha = 0xff - (int)(self->volume * 255.0f);
		if (alpha < 0)
			alpha = 0;
		else if (alpha > 0xff)
			alpha = 0xff;
		CLScreen_DrawDarken(alpha, 1);
	}
	if (self->fadeOut != 0)
	{
		fade = self->fade;
		if (!(fade < 1.0f))
			fade = 1.0f;
		CLScreen_DrawDarken((int)(fade * 255.0f), 1);
	}
}

/* 0x000c8420  CarryOn/CFire_Camera.cpp (file inferred)
The winner to show: the leaders take turns, one every 240 frames since the end (360 in a
tournament with one leader). NULL when there is none (in a team game, also when no
player leads). */
struct CCarryOnPlayer *CFire_Camera_PickWinner(struct CFireCamera *self)
{
	float period;
	int count;
	int n;
	int seen;
	int i;

	count = 0;
	if (DAT_002baebc == 0)
	{
		CCarryOn_Player_IsLeading(g_coPlayers[0], &count);
		if (count == 0)
			return NULL;
	}
	else
		CCarryOn_Player_IsLeading(g_coPlayers[0], &count);
	period = 240.0f;
	if (DAT_002bae98 == 1 && count == 1)
		period = 360.0f;
	n = (int)(self->endTime / period);
	if (n >= count)
		return NULL;
	seen = 0;
	for (i = 0; i < DAT_002bae94; i++)
	{
		if (CCarryOn_Player_IsLeading(g_coPlayers[i], NULL) != -1)
		{
			if (seen == n)
				return g_coPlayers[i];
			seen++;
		}
	}
	return NULL;
}

/* 0x000c8550  CarryOn/CFire_Camera.cpp (file inferred)
Starts the end camera (mode 4): "skip" is bound to the pads of the human leaders (to
every pad when there is none). */
void CFire_Camera_StartEnd(struct CFireCamera *self)
{
	int bound = 0;
	int i;

	self->endTime = 0.0f;
	self->mode = 4;
	biDeleteChannel(self->skip);
	self->skip = biCreateChannel(self->inputMap, "skip", 1, 1, 0, 0, 0);
	for (i = 0; i < 4; i++)
	{
		if (CCarryOn_Player_IsLeading(g_coPlayers[i], NULL) >= 0 && DAT_002baed4[i][1] != 0)
		{
			biBindInput(self->skip, (unsigned int)DAT_002baed4[i][3] | 0xc5, 0);
			bound = 1;
		}
	}
	if (!bound)
		biBindInput(self->skip, 0xe00c5, 0);
}

/* 0x000c85f0  CarryOn/CFire_Camera.cpp (file inferred)
The camera's yaw: atan2 of the view direction's z and x. */
float CFire_Camera_GetYaw(struct CFireCamera *self)
{
	return (float)atan2((double)self->target[2] - (double)self->eye[2],
		(double)self->target[0] - (double)self->eye[0]);
}

/* 0x000c8610  CarryOn/CFire_Camera.cpp (file inferred) */
void CFire_Camera_GetEye(struct CFireCamera *self, float *out)
{
	out[0] = self->eye[0];
	out[1] = self->eye[1];
	out[2] = self->eye[2];
	out[3] = self->eye[3];
}

/* 0x000c8640  CarryOn/CFire_Camera.cpp (file inferred) */
void CFire_Camera_GetTarget(struct CFireCamera *self, float *out)
{
	out[0] = self->target[0];
	out[1] = self->target[1];
	out[2] = self->target[2];
	out[3] = self->target[3];
}

/* 0x000c8670  CarryOn/CFire_Camera.cpp (file inferred)
Moves the eye and the target 5% of the way to their goals per frame (at once in the
first 3 frames). */
void CFire_Camera_Lerp(struct CFireCamera *self, float step)
{
	if (self->frames < 3)
	{
		memcpy(self->target, self->targetGoal, sizeof self->target);
		memcpy(self->eye, self->eyeGoal, sizeof self->eye);
		return;
	}
	step = step * 0.05f;           /* 0x00258b54 */
	bmVectorLerp(self->target, self->target, self->targetGoal, step);
	bmVectorLerp(self->eye, self->eye, self->eyeGoal, step);
}

/* 0x000c8730  CarryOn/CFire_Camera.cpp (file inferred)
Sets the view: near 40, far 80000, the eye looking at the target. */
void CFire_Camera_Apply(struct CFireCamera *self)
{
	bdSetNearFar(40.0f, 80000.0f);
	bdSetCameraLookAt(self->eye, self->target, 0.0f);
	bdUpdateView();
}

/* 0x000c8760  CarryOn/CFire_Camera.cpp (file inferred)
Gives the charlib camera the players to follow (variants 0 and 1). */
void CFire_Camera_Reset(struct CFireCamera *self)
{
	int i;

	if (self->variant == 3 || self->variant == 2)
		return;
	for (i = 0; i < DAT_002bae94; i++)
		CCamera_AddTarget(DAT_002f7964, CO_FP(CO_PTR(DAT_002f289c[i], CP_ACTOR), CA_POS), 0,
			DAT_002ea478);
}

/* 0x000c87b0  CarryOn/CFire_Camera.cpp (file inferred)
The camera heading for charlib (DAT_002f7a2c), and each AI player's heading from the eye
(AI +0x1118), for screen-relative controls. */
void CFire_Camera_UpdateHeadings(struct CFireCamera *self)
{
	void *actor;
	int i;

	DAT_002f7a2c = CLMath_WrapAngle((float)atan2((double)self->target[0] - (double)self->eye[0],
		(double)self->target[2] - (double)self->eye[2]));
	for (i = 0; i < DAT_002bae94; i++)
	{
		actor = CO_PTR(DAT_002f289c[i], CP_ACTOR);
		CO_F32(DAT_002f289c[i], 0x1118) = CLMath_WrapAngle((float)atan2(
			(double)CO_F32(actor, CA_POS) - (double)self->eye[0],
			(double)CO_F32(actor, CA_POS + 8) - (double)self->eye[2]));
	}
}

/* 0x000c8840  CarryOn/CFire_Camera.cpp (file inferred)
The players' bounding box, its centre and their mean position. */
void CFire_Camera_UpdateBounds(struct CFireCamera *self)
{
	float sumX = 0.0f;
	float sumY = 0.0f;
	float sumZ = 0.0f;
	float x, y, z;
	void *actor;
	int i;

	self->max[0] = -99999.0f;
	self->max[1] = -99999.0f;
	self->max[2] = -99999.0f;
	self->min[0] = 99999.0f;
	self->min[1] = 99999.0f;
	self->min[2] = 99999.0f;
	for (i = 0; i < DAT_002bae94; i++)
	{
		actor = CO_PTR(g_coPlayers[i], CP_ACTOR);
		x = CO_F32(actor, CA_POS);
		y = CO_F32(actor, CA_POS + 4);
		z = CO_F32(actor, CA_POS + 8);
		if (self->max[0] < x)
			self->max[0] = x;
		if (x < self->min[0])
			self->min[0] = x;
		if (self->max[1] < y)
			self->max[1] = y;
		if (y < self->min[1])
			self->min[1] = y;
		if (self->max[2] < z)
			self->max[2] = z;
		if (z < self->min[2])
			self->min[2] = z;
		sumX = x + sumX;
		sumY = y + sumY;
		sumZ = z + sumZ;
	}
	self->centre[0] = (self->min[0] + self->max[0]) * 0.5f;
	self->centre[1] = (self->max[1] + self->min[1]) * 0.5f;
	self->centre[2] = (self->min[2] + self->max[2]) * 0.5f;
	self->mean[0] = sumX / (float)DAT_002bae94;
	self->mean[1] = sumY / (float)DAT_002bae94;
	self->mean[2] = sumZ / (float)DAT_002bae94;
}

/* 0x000c89b0  CarryOn/CFire_Camera.cpp
Clears the camera, sets the field of view; for variants 0 and 1 sets the charlib camera's
start (DAT_00294b64-0x00294bb8 from this module's table) and creates it (DAT_002f7964,
mode 5). Creates the "skip" input on every pad; in demo mode starts the game camera at
once. */
struct CFireCamera *CFire_Camera_Ctor(struct CFireCamera *self, int variant)
{
	void *mem;

	memset(self, 0, 0x35 * 4);
	self->variant = variant;
	bdSetFieldOfView(1.22173047f, 0.916297853f);
	self->mode = 0;
	if (self->variant != 3 && self->variant != 2)
	{
		DAT_00294b98[0] = DAT_00294338[0];
		DAT_00294b98[1] = DAT_00294338[1];
		DAT_00294b98[2] = DAT_00294338[2];
		DAT_00294b98[3] = DAT_00294338[3];
		DAT_00294ba8[0] = DAT_00294328[0];
		DAT_00294b64 = 1700.0f;
		DAT_00294ba8[1] = DAT_00294328[1];
		DAT_00294ba8[2] = DAT_00294328[2];
		DAT_00294ba8[3] = DAT_00294328[3];
		DAT_00294bb8 = -0.5f;
		mem = bkHeapAlloc(0x574, CO_FILE_CAMERA, 0x49, 0x1001);
		DAT_002f7964 = mem == NULL ? NULL : CCamera_Ctor(mem, 5);
	}
	self->inputMap = biCreateMap(4);
	self->skip = biCreateChannel(self->inputMap, "skip", 1, 1, 0, 0, 0);
	biBindInput(self->skip, 0xe00c5, 0);
	if (DAT_002baeb8 != 0)
		CFire_Camera_StartGame(self);
	return self;
}

/* 0x000c8b10  CarryOn/CFire_Camera.cpp (file inferred)
Deletes the charlib camera, the input map and the intro camera. */
void CFire_Camera_Dtor(struct CFireCamera *self)
{
	void *camera = DAT_002f7964;

	if (camera != NULL)
	{
		CCamera_Dtor(camera);
		bkHeapFree(camera);
		DAT_002f7964 = NULL;
	}
	biDeleteMap(self->inputMap);
	CLIntroCam_Destroy();
}

/* 0x000c8b50  CarryOn/CFire_Camera.cpp (file inferred)
Mode 0: plays the level's intro camera. When it ends (or fails), or "skip" is pressed,
the camera jumps to the intro's last frame and the game camera starts. */
void CFire_Camera_UpdateIntro(struct CFireCamera *self, float step)
{
	float fov[1];
	float t;
	int result;
	int done = 0;

	result = CLIntroCam_Update(step, 1);
	if (result == 0)
	{
		t = (float)(int)CLIntroCam_GetTime();
		CLIntroCam_GetPosition(self->eye, t);
		CLIntroCam_GetTarget(self->target, t);
		CLIntroCam_GetFov(fov, t);
		memcpy(self->eyeGoal, self->eye, sizeof self->eyeGoal);
		memcpy(self->targetGoal, self->target, sizeof self->targetGoal);
	}
	else
	{
		if (result == -1)
			bkPrintf("***INTRO CAMERA ERROR***\n");
		done = 1;
	}
	CFire_Camera_Lerp(self, step);
	bdSetNearFar(40.0f, 80000.0f);
	bdSetCameraLookAt(self->eye, self->target, 0.0f);
	bdUpdateView();
	if (!done)
	{
		if (step <= 0.0f)
			return;
		if (CO_I32(self->skip, 0x14) == 0)
			return;
		t = (float)CLIntroCam_GetFrameCount() - 1.0f;
		CLIntroCam_GetPosition(self->eye, t);
		CLIntroCam_GetTarget(self->target, t);
	}
	CFire_Camera_StartGame(self);
}

/* 0x000c8cb0  CarryOn/CFire_Camera.cpp (file inferred)
Modes 4/5, the end: looks at the current winner from behind and above (400 units at
-30 degrees pitch, 135 degrees yaw; the winner is turned to face it). After 120 frames
"skip" starts the fade-out; after 4 fades the mode becomes 5 (the game ends). */
void CFire_Camera_UpdateEnd(struct CFireCamera *self, float step)
{
	struct CCarryOnPlayer *winner;
	float offset[4];
	float rotated[4];
	float x, y, z, w;
	void *actor;
	int changed;

	self->endTime = step + self->endTime;
	if (120.0f < self->endTime && self->fadeOut == 0 && 0.0f < step
		&& CO_I32(self->skip, 0x14) != 0)
	{
		bkPrintf("PreEmptive Fadeout\n");
		self->wantFadeOut = 1;
	}
	winner = CFire_Camera_PickWinner(self);
	if (winner == NULL)
	{
		if (CLSamples_CommentaryIdle() != 0)
			self->wantFadeOut = 1;
		winner = self->winner;
	}
	changed = winner != self->winner;
	if (changed)
	{
		self->winner = winner;
		self->wantFadeIn = 1;
	}
	actor = CO_PTR(winner, CP_ACTOR);
	x = CO_F32(actor, CA_POS);
	y = CO_F32(actor, CA_POS + 4) + 100.0f;
	z = CO_F32(actor, CA_POS + 8);
	w = CO_F32(actor, CA_POS + 12);
	if (changed)
	{
		CO_F32(actor, 0x1d8) = 2.3561945f;
		CO_F32(CO_PTR(winner, CP_ACTOR), CA_YAW) = 2.3561945f;
	}
	offset[0] = 0.0f;
	offset[1] = 0.0f;
	offset[2] = 400.0f;
	bmVectorRotateXY(rotated, offset, -0.52359879f, 2.3561945f);
	self->eye[3] = 1.0f;
	self->eye[0] = rotated[0] + x;
	self->eye[1] = rotated[1] + y;
	self->eye[2] = rotated[2] + z;
	self->target[0] = x;
	self->target[1] = y;
	self->target[2] = z;
	self->target[3] = w;
	self->eyeGoal[0] = rotated[0] + x;
	self->eyeGoal[2] = rotated[2] + z;
	self->eyeGoal[1] = rotated[1] + y;
	self->eyeGoal[3] = 1.0f;
	self->targetGoal[1] = y;
	self->targetGoal[0] = x;
	self->targetGoal[2] = z;
	self->targetGoal[3] = w;
	CFire_Camera_Lerp(self, step);
	bdSetNearFar(40.0f, 80000.0f);
	bdSetCameraLookAt(self->eye, self->target, 0.0f);
	bdUpdateView();
	if (self->fadeOut != 0)
	{
		self->fade = step * 0.0333333351f + self->fade;    /* 0x00255b64 */
		if (1.0f <= self->fade)
		{
			self->fade = 1.0f;
			self->fadeCount++;
			if (self->fadeCount > 3)
				self->mode = 5;
		}
	}
}

/* 0x000c8ed0  CarryOn/CFire_Camera.cpp (file inferred)
Mode 2 (variant 3): looks at the arena centre and rises with the highest firework
(carryon_b CFire_Fireworks_MaxHeight), swaying from side to side. */
void CFire_Camera_UpdateFireworks(struct CFireCamera *self, float step)
{
	float height;
	float zoom;

	self->targetGoal[0] = 0.0f;
	self->targetGoal[1] = 500.0f;
	self->targetGoal[2] = 750.0f;
	height = 0.0f;
	CFire_Fireworks_MaxHeight(g_fireGame.fireworks, &height, 1.0f);
	height = (height - self->targetGoal[1]) * 0.2f;
	if (height < 90.0f)
		height = 90.0f;
	if (height <= self->zoom)
	{
		if (self->hold <= 0.0f)
		{
			zoom = self->zoom - 3.0f;
			self->zoom = zoom;
			if (zoom < height)
				self->zoom = height;
		}
		else
			self->hold = self->hold - step * 0.5f;
	}
	else
	{
		self->hold = 20.0f;
		self->zoom = self->zoom + 15.0f;
	}
	self->targetGoal[1] = self->zoom + self->targetGoal[1];
	self->phase = step * 0.00999999978f + self->phase;     /* 0x0025372c */
	self->eyeGoal[0] = sinf(self->phase) * 500.0f;
	self->eyeGoal[1] = 800.0f - self->zoom * 0.2f;
	self->eyeGoal[2] = -900.0f - self->zoom * 0.5f;
	CFire_Camera_Lerp(self, step);
	bdSetNearFar(40.0f, 80000.0f);
	bdSetCameraLookAt(self->eye, self->target, 0.0f);
	bdUpdateView();
}

/* 0x000c9040  CarryOn/CFire_Camera.cpp (file inferred)
Mode 1 (variant 2): follows the players' mean x from the front, pulling back with their
spread. The target height `zoom` jumps up 30 and sinks back 20 per frame after a hold. */
void CFire_Camera_UpdatePlayers(struct CFireCamera *self, float step)
{
	float base;

	self->targetGoal[0] = 0.0f;
	self->targetGoal[1] = -500.0f;
	self->targetGoal[2] = -50.0f;
	base = self->targetGoal[1];
	if (base <= self->zoom)
	{
		if (self->hold <= 0.0f)
		{
			self->zoom = self->zoom - 20.0f;
			if (self->zoom < base)
				self->zoom = base;
		}
		else
			self->hold = self->hold - step;
	}
	else
	{
		self->zoom = self->zoom + 30.0f;
		self->hold = 20.0f;
	}
	self->targetGoal[1] = self->zoom;
	CFire_Camera_UpdateBounds(self);
	self->eyeGoal[0] = DAT_00294324 * self->mean[0];
	self->eyeGoal[1] = 1400.0f - self->targetGoal[1] * 0.125f;
	self->eyeGoal[2] = fabsf(self->mean[0] - self->centre[0]) * 0.25f - 2500.0f;
	CFire_Camera_Lerp(self, step);
	bdSetNearFar(40.0f, 80000.0f);
	bdSetCameraLookAt(self->eye, self->target, 0.0f);
	bdUpdateView();
}

/* 0x000c91a0  CarryOn/CFire_Camera.cpp (file inferred)
Per frame: fades the sound in (sets the effect volume once `volume` reaches 1), runs the
mode, copies the eye and the target to the resource (+0x25c, +0x26c), starts the
requested fades and feeds the intro camera of rowing_a when it runs. Mode 3 eases from
this camera to the charlib camera (`blend`). */
void CFire_Camera_Update(struct CFireCamera *self, float step)
{
	unsigned char *cam;
	float blend;

	if (self->volume < 1.0f)
	{
		self->volume = step * 0.0333333351f + self->volume;
		if (1.0f <= self->volume)
			bsSetSfxVolume(DAT_002bae6c);
	}
	switch (self->mode)
	{
	case 0:
		CFire_Camera_UpdateIntro(self, step);
		CFire_Camera_UpdateHeadings(self);
		break;
	case 1:
		CFire_Camera_UpdatePlayers(self, step);
		CFire_Camera_UpdateHeadings(self);
		break;
	case 2:
		CFire_Camera_UpdateFireworks(self, step);
		CFire_Camera_UpdateHeadings(self);
		break;
	case 3:
		CCamera_Update(DAT_002f7964);
		if (step != 0.0f)
		{
			cam = DAT_002f7964;
			if (self->snap != 0)
				self->blendTarget = 0.5f;
			if (self->blend < self->blendTarget)
			{
				blend = self->blend + 0.05f;               /* 0x00258b54 */
				self->blend = blend;
				if (self->blendTarget <= blend)
				{
					self->blend = self->blendTarget;
					self->blendTarget = 0.0f;
				}
			}
			else
			{
				if (0.0f < self->blendTarget)
					self->blendTarget = 0.0f;
				blend = self->blend - 0.00499999989f;      /* 0x002632d8 */
				self->blend = blend;
				if (blend <= self->blendTarget)
					self->blend = self->blendTarget;
			}
			if (self->blend != 0.0f)
			{
				CO_F32(cam, 0x534) = (CO_F32(cam, 0x534) - CO_F32(cam, 0x544)) * self->blend
					+ CO_F32(cam, 0x534);
				cam = DAT_002f7964;
				CO_F32(cam, 0x538) = (CO_F32(cam, 0x538) - CO_F32(cam, 0x548)) * self->blend
					+ CO_F32(cam, 0x538);
				cam = DAT_002f7964;
				CO_F32(cam, 0x53c) = (CO_F32(cam, 0x53c) - CO_F32(cam, 0x54c)) * self->blend
					+ CO_F32(cam, 0x53c);
				cam = DAT_002f7964;
				bdSetCameraLookAt(CO_FP(cam, 0x534), CO_FP(cam, 0x544), 0.0f);
				bdUpdateView();
			}
			self->snap = 0;
		}
		cam = DAT_002f7964;
		memcpy(self->eye, CO_FP(cam, 0x534), sizeof self->eye);
		memcpy(self->target, CO_FP(cam, 0x544), sizeof self->target);
		CFire_Camera_Apply(self);
		break;
	case 4:
	case 5:
		CFire_Camera_UpdateEnd(self, step);
		CFire_Camera_UpdateHeadings(self);
		CFire_Camera_Apply(self);
		break;
	}
	memcpy(g_fireRes->eye, self->eye, sizeof g_fireRes->eye);
	memcpy(g_fireRes->target, self->target, sizeof g_fireRes->target);
	if (self->wantFadeIn != 0)
	{
		self->fadeIn = 1;
		self->wantFadeIn = 0;
	}
	if (self->wantFadeOut != 0)
	{
		self->fadeOut = 1;
		self->wantFadeOut = 0;
	}
	if (co_IntroCam_Active() != 0)
		co_IntroCam_GetPos(g_fireRes->eye);
}
