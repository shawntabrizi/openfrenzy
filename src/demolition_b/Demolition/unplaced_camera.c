/*
Demolition/unplaced_camera.c  <-  C:\BlitzParty\Game\Src\Demolition\ (file not known)

The Demolition camera (game +0x1298, vtable 0x0026b0f0; probably CDemolition_Camera.cpp).
Gap functions between the bombs (CDemolition_Bomb.cpp) and the world (CDemolition_World.cpp).

Camera fields:
  +0x08 eye (x, y, z, w)   +0x18 target (x, y, z, w)   +0x2c pitch   +0x30 yaw
  +0x3c roll   +0x40 field of view (x)
  +0x44 mode: 1 intro fly-by, 2 countdown, 3 play, 4 results, 5 still
  +0x4c fade in, +0x50 fade to dark, +0x54 fade from dark (running); +0x58, +0x5c, +0x60
        the requests that start them on the next update
  +0x64 extra distance   +0x68 minimum, +0x6c maximum distance of the eye in front of the
        players   +0x70 eye, +0x74 target follow speed
  +0x78 a countdown timer (demolition_a CDemoCountdown; rollerball_a methods)
  +0xb0 the player shown by the results camera   +0xd4 the results duration
  variation 3 (the arena): +0xd8 aim (x, z), +0xe0 position (x, z), +0xe8 velocity (x, z),
        +0xf0 target distance, +0xf4 eye height, +0xf8 eye distance
*/

#include "demolition_b.h"
#include "demolition_b_internal.h"

/* 0x002f289c  the players' CharLib AI objects (charlib) */
extern int DAT_002f289c[];
/* 0x002baeb8  the mini game is a demo / skip-intro flag (core) */
extern int DAT_002baeb8;
/* 0x002ecfac  the HUD (classlib_ui CLHud *) */
extern int DAT_002ecfac;

#define DB_CAM_EYE(s) DB_FP(s, 0x08)
#define DB_CAM_TARGET(s) DB_FP(s, 0x18)

/* the eye, target and roll of the camera to the display */
static void db_camera_look(char *s)
{
	bdSetCameraLookAt(DB_CAM_EYE(s), DB_CAM_TARGET(s), DB_F32(s, 0x3c));
}

/* the players' AI objects face the camera's direction `angle` (+0x1118) */
static void db_camera_turn_players(double angle)
{
	int i;

	for (i = 0; i < DAT_002bae94; i++)
		DB_F32((char *)(uintptr_t)DAT_002f289c[i], 0x1118) = (float)(-angle + (double)1.5707964f);
}

/* 0x000f5730  constructor */
struct CDemolition_Camera *CDemolition_Camera_Construct(struct CDemolition_Camera *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026b0f0;
	CDemoCountdown_Ctor((unsigned int *)(s + 0x78));
	return self;
}

/* 0x000f5750  destructor */
void CDemolition_Camera_Destruct(struct CDemolition_Camera *self)
{
	char *s = (char *)self;

	DB_PTR(s, 0) = (void *)vtable_0026b0f0;
	CDemoCountdown_Dtor((unsigned int *)(s + 0x78));
}

/* 0x000f5760  sets the mode; the countdown mode (1) also fades in from dark */
void CDemolition_Camera_SetMode(struct CDemolition_Camera *self, int mode)
{
	DB_I32(self, 0x44) = mode;
	if (mode == 1)
		DB_I32(self, 0x54) = 1;
}

/* 0x000f5780  applies the field of view and the view (`dt` is not used) */
void CDemolition_Camera_Apply(struct CDemolition_Camera *self, float dt)
{
	char *s = (char *)self;

	(void)dt;
	bdSetFieldOfView(DB_F32(s, 0x40), DB_F32(s, 0x40) * 3.0f * 0.25f);
	db_camera_look(s);
	bdUpdateView();
}

/* 0x000f57c0  draws the screen fades: the fade-in (CLFade_Draw with the frame step cap,
   game +0x789c), the fade to dark and the fade from dark (timer +0x78, 1 second) */
void CDemolition_Camera_DrawFades(struct CDemolition_Camera *self)
{
	char *s = (char *)self;
	float t;

	if (DB_I32(s, 0x4c) != 0 && CLFade_Draw(DB_F32(DAT_002ff85c, 0x789c)) != 0)
		DB_I32(s, 0x4c) = 0;
	if (DB_I32(s, 0x50) != 0)
	{
		t = BCarTimer_GetRemaining((struct BCarTimer *)(s + 0x78));
		if (t >= 1.0f)
			DB_I32(s, 0x50) = 0;
		CLScreen_DrawDarken((int)(t * 255.0f), 1);
	}
	if (DB_I32(s, 0x54) != 0)
	{
		t = BCarTimer_GetRemaining((struct BCarTimer *)(s + 0x78));
		if (t >= 1.0f)
			DB_I32(s, 0x54) = 0;
		CLScreen_DrawDarken((int)((1.0f - t) * 255.0f), 1);
	}
}

/* 0x000f5870  resets the camera (the arena variation 3 gets its own distances) and the
   intro fly-by */
void CDemolition_Camera_Reset(struct CDemolition_Camera *self)
{
	char *s = (char *)self;

	DB_I32(s, 0x44) = 0;
	DB_I32(s, 0x48) = 0;
	DB_F32(s, 0x18) = 0.0f;
	DB_F32(s, 0x1c) = 0.0f;
	DB_F32(s, 0x20) = 0.0f;
	DB_F32(s, 0x08) = 0.0f;
	DB_F32(s, 0x0c) = 0.0f;
	DB_F32(s, 0x10) = 0.0f;
	DB_F32(s, 0x30) = 0.0f;
	DB_F32(s, 0x34) = 0.0f;
	DB_F32(s, 0x3c) = 0.0f;
	DB_U32(s, 0x40) = 0x3f9c61aa;   /* 1.2217 (70 degrees) */
	FUN_0018ea70((void *)(s + 0x78));
	if (DB_I32(DAT_002ff85c, 0x28) == 3)
	{
		DB_U32(s, 0x30) = 0x3f490fdb;   /* pi / 4 */
		DB_U32(s, 0x2c) = 0xbf32b8c2;   /* -0.6981 (-40 degrees) */
		DB_F32(s, 0x68) = 400.0f;
		DB_F32(s, 0x6c) = 2500.0f;
		DB_U32(s, 0x70) = 0x3ccccccd;   /* 0.025 */
		DB_U32(s, 0x74) = 0x3c4ccccd;   /* 0.0125 */
		DB_F32(s, 0x64) = 400.0f;
		DB_F32(s, 0xd8) = 0.0f;
		DB_F32(s, 0xdc) = 0.0f;
		DB_F32(s, 0xe0) = 0.0f;
		DB_F32(s, 0xe4) = 0.0f;
		DB_F32(s, 0xe8) = 0.0f;
		DB_F32(s, 0xec) = 0.0f;
		DB_F32(s, 0xf0) = -200.0f;
		DB_F32(s, 0xf8) = -1500.0f;
		DB_F32(s, 0xf4) = 1300.0f;
	}
	CLIntroCam_Restart();
	DB_I32(s, 0x4c) = 0;
	DB_I32(s, 0x50) = 0;
	DB_I32(s, 0x54) = 0;
	DB_I32(s, 0x58) = 0;
	DB_I32(s, 0x5c) = 0;
	DB_I32(s, 0x60) = 0;
	DB_PTR(s, 0xb0) = NULL;
}

/* 0x000f5950  the player the results camera shows now: the players with the flag +0x116f
   share the results time (+0xd4) in equal parts, in list order */
void *CDemolition_Camera_ResultsPlayer(struct CDemolition_Camera *self)
{
	char *s = (char *)self;
	char *first = DB_GAME_PLAYERS();
	char *p;
	int count = 0;
	int index;
	int i;
	float part;

	if (first == NULL)
		return NULL;
	for (p = first; p != NULL; p = DB_PLAYER_NEXT(p))
		if (DB_U8(p, 0x116f) != 0)
			count++;
	if (count == 0)
		return NULL;
	part = DB_F32(s, 0xd4) / (float)count;
	index = (int)((DB_F32(s, 0xd4) - FUN_0018ea50((void *)(s + 0x78))) / part);
	if (index >= count)
		index = count - 1;
	i = 0;
	for (p = first; p != NULL; p = DB_PLAYER_NEXT(p))
	{
		if (DB_U8(p, 0x116f) == 0)
			continue;
		if (i == index)
			return p;
		i++;
	}
	return NULL;
}

/* 0x000f59f0  deleting destructor (vtable 0x0026b0f0 slot 0) */
void *CDemolition_Camera_DeletingDestruct(struct CDemolition_Camera *self, unsigned int flags)
{
	CDemolition_Camera_Destruct(self);
	if (flags & 1)
		bkHeapFree(self);
	return self;
}

/* 0x000f5a10  Init: the follow distances of the variation, then a reset. Always succeeds. */
int CDemolition_Camera_Init(struct CDemolition_Camera *self)
{
	char *s = (char *)self;

	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
	case 1:
		DB_F32(s, 0x68) = 500.0f;
		DB_F32(s, 0x6c) = 3250.0f;
		DB_F32(s, 0x64) = 800.0f;
		DB_U32(s, 0x70) = 0x3ccccccd;   /* 0.025 */
		DB_U32(s, 0x74) = 0x3ccccccd;
		break;
	case 2:
		DB_F32(s, 0x68) = 500.0f;
		DB_F32(s, 0x6c) = 3000.0f;
		DB_F32(s, 0x64) = 500.0f;
		DB_U32(s, 0x70) = 0x3ccccccd;
		DB_U32(s, 0x74) = 0x3ccccccd;
		break;
	}
	CDemoCountdown_Init((int)(s + 0x78));
	CDemolition_Camera_Reset(self);
	return 1;
}

/* 0x000f5a70  mode 1: the intro fly-by (CLIntroCam). When it ends (or fails), or a human
   player presses a button (input channel +0x14), or the game is a demo, the camera fades
   in and goes to the countdown (mode 2); the HUD bars start and the variation object is
   told (virtual +0x14). `dt` is not used. */
void CDemolition_Camera_UpdateIntro(struct CDemolition_Camera *self, float dt)
{
	char *s = (char *)self;
	int ended = 0;
	int skipped = 0;
	int result;
	int i;
	float t;
	float eye[4];
	float target[4];
	float fov;

	(void)dt;
	result = CLIntroCam_Update(1.0f, 1);
	if (result == 0)
	{
		t = (float)(int)CLIntroCam_GetTime();
		CLIntroCam_GetPosition(eye, t);
		CLIntroCam_GetTarget(target, t);
		CLIntroCam_GetRoll(DB_FP(s, 0x3c), t);
		CLIntroCam_GetFov(&fov, t);   /* read but not used */
		memcpy(DB_CAM_EYE(s), eye, sizeof eye);
		memcpy(DB_CAM_TARGET(s), target, sizeof target);
	}
	else
	{
		if (result == -1)
			bkPrintf("***CAMERA ERROR***\n");
		ended = 1;
	}
	for (i = 0; i < 4; i++)
	{
		if (DAT_002baed4[i * 10 + 1] != 0 &&
			DB_I32(DB_PTR(DAT_002ff85c, 0x77ec + i * 4), 0x14) != 0)
		{
			skipped = 1;
			break;
		}
	}
	if (!skipped && DAT_002baeb8 != 0)
		skipped = 1;
	if (skipped || ended)
	{
		DB_I32(s, 0x58) = 1;
		fov = db_bitsf(0x3f9c61aa);   /* stored to the unused local */
		(void)fov;
		DB_F32(s, 0x3c) = 0.0f;
		if (skipped)
		{
			t = (float)(CLIntroCam_GetFrameCount() - 1);
			CLIntroCam_GetPosition(DB_CAM_EYE(s), t);
			CLIntroCam_GetTarget(DB_CAM_TARGET(s), t);
		}
		DB_I32(s, 0x44) = 2;
		FUN_001d5950();
		CLHud_StartBars((float *)(uintptr_t)DAT_002ecfac, 280.0f);
		{
			void *variation = DB_PTR(DAT_002ff85c, 0x79a0);

			DB_VFN(variation, 0x14, void (*)(void *))(variation);
		}
	}
	db_camera_look(s);
}

/* 0x000f5c10  modes 2 and 3 of the arena (variation 3): the camera circles at a fixed yaw
   and slides (spring with damping 0.97) towards its aim (+0xd8) at the height of the
   variation's floor (variation object +0x16b0). `mode` is not used. */
void CDemolition_Camera_UpdateArena(struct CDemolition_Camera *self, float dt, int mode)
{
	char *s = (char *)self;
	float yaw = DB_F32(s, 0x30);
	float c = cosf(yaw);
	float sn = sinf(yaw);
	float targetOff[3];
	float eyeOff[3];
	float dx, dz, len, inv, k;
	float v[4];
	float r[4];
	float floorY;
	float *eye = DB_CAM_EYE(s);
	float *target = DB_CAM_TARGET(s);

	(void)mode;
	targetOff[0] = c * DB_F32(s, 0xf0);
	targetOff[2] = sn * DB_F32(s, 0xf0);
	eyeOff[0] = c * DB_F32(s, 0xf8);
	eyeOff[1] = DB_F32(s, 0xf4);
	eyeOff[2] = sn * DB_F32(s, 0xf8);
	dx = DB_F32(s, 0xd8) - DB_F32(s, 0xe0);
	dz = DB_F32(s, 0xdc) - DB_F32(s, 0xe4);
	len = sqrtf(dz * dz + dx * dx);
	if (len != 0.0f)
	{
		inv = 1.0f / len;
		dx = inv * dx;
		dz = inv * dz;
	}
	k = dt * 0.05f;
	DB_F32(s, 0xe8) = k * dx + DB_F32(s, 0xe8);
	DB_F32(s, 0xec) = k * dz + DB_F32(s, 0xec);
	DB_F32(s, 0xe8) = DB_F32(s, 0xe8) * 0.97f;
	DB_F32(s, 0xec) = DB_F32(s, 0xec) * 0.97f;
	DB_F32(s, 0xe0) = DB_F32(s, 0xe8) + DB_F32(s, 0xe0);
	DB_F32(s, 0xe4) = DB_F32(s, 0xe4) + DB_F32(s, 0xec);
	/* the original passes (z, 0, x): position +0xe4 as x, +0xe0 as z */
	v[0] = DB_F32(s, 0xe4);
	v[1] = 0.0f;
	v[2] = DB_F32(s, 0xe0);
	v[3] = 0.0f;
	bmVectorRotateY(r, v, -yaw);
	floorY = DB_F32(DB_PTR(DAT_002ff85c, 0x79a0), 0x16b0);
	/* the w components were uninitialised locals in the original; they are not written */
	eye[0] = r[0] + eyeOff[0];
	eye[1] = floorY + (r[1] + eyeOff[1]);
	eye[2] = r[2] + eyeOff[2];
	target[0] = r[0] + targetOff[0];
	target[1] = floorY + r[1];
	target[2] = r[2] + targetOff[2];
	db_camera_turn_players((double)yaw);
	DB_U32(s, 0x40) = 0x3fc90fdb;   /* pi / 2 */
	db_camera_look(s);
}

/* The results cameras (0x000f5e30, 0x000f5f60, 0x000f60e0) share this: look at `player`
   (at 0.8 of its height) from 400 units away, pitched down 20 degrees, at the yaw +0x30. */
static void db_camera_results_view(char *s, char *player)
{
	float target[4];
	float v[4];
	float out[4];
	float *eye = DB_CAM_EYE(s);

	target[0] = DB_F32(player, 0x1144);
	target[1] = DB_F32(player, 0x1168) * 0.8f + DB_F32(player, 0x1148);
	target[2] = DB_F32(player, 0x114c);
	target[3] = DB_F32(player, 0x1150);
	DB_U32(s, 0x2c) = 0xbeb2b8c2;   /* -0.3491 (-20 degrees) */
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 400.0f;
	v[3] = 0.0f;
	bmVectorRotateXY(out, v, db_bitsf(0xbeb2b8c2), DB_F32(s, 0x30));
	/* eye w: an uninitialised local in the original; not written */
	eye[0] = out[0] + target[0];
	eye[1] = out[1] + target[1];
	eye[2] = out[2] + target[2];
	memcpy(DB_CAM_TARGET(s), target, sizeof target);
	db_camera_look(s);
}

/* the player the results camera shows changed: turn its AI object to `yaw` (+0x1d8, +0x1e8)
   and put it in state 0x11 (celebrate) */
static void db_camera_results_turn(char *player, float yaw)
{
	char *ai = (char *)DB_PTR(player, 0x30);

	DB_F32(ai, 0x1d8) = yaw;
	ai = (char *)DB_PTR(player, 0x30);
	DB_F32(ai, 0x1e8) = DB_F32(ai, 0x1d8);
}

/* the player shown now; 1 when it changed (then the camera fades in again) */
static char *db_camera_results_player(char *s, int *changed)
{
	char *player = CDemolition_Camera_ResultsPlayer((struct CDemolition_Camera *)s);

	*changed = 0;
	if (player != NULL && player != (char *)DB_PTR(s, 0xb0))
	{
		*changed = 1;
		DB_PTR(s, 0xb0) = player;
		DB_I32(s, 0x58) = 1;
	}
	return player;
}

/* 0x000f5e30  mode 4 of variations 0 and 1: the results camera, yaw 135 degrees */
void CDemolition_Camera_ResultsA(struct CDemolition_Camera *self, float dt)
{
	char *s = (char *)self;
	char *player;
	int changed;

	(void)dt;
	player = db_camera_results_player(s, &changed);
	if (player == NULL)
		return;
	DB_U32(s, 0x30) = 0x4016cbe4;   /* 3 pi / 4 */
	if (changed)
	{
		db_camera_results_turn(player, db_bitsf(0x4016cbe4));
		AI_SetState(player, 0x11);
	}
	db_camera_results_view(s, player);
}

/* 0x000f5f60  mode 4 of variation 2: as 0x000f5e30 (a hit message is built and destroyed
   around the state change, unused) */
void CDemolition_Camera_ResultsB(struct CDemolition_Camera *self, float dt)
{
	char *s = (char *)self;
	char *player;
	int changed;
	struct DbMessage message;

	(void)dt;
	player = db_camera_results_player(s, &changed);
	if (player == NULL)
		return;
	DB_U32(s, 0x30) = 0x4016cbe4;   /* 3 pi / 4 */
	if (changed)
	{
		db_camera_results_turn(player, db_bitsf(0x4016cbe4));
		FUN_0010cec0((undefined4 *)&message);
		AI_SetState(player, 0x11);
		FUN_0010ced0((undefined4 *)&message);
	}
	db_camera_results_view(s, player);
}

/* 0x000f60e0  mode 4 of variation 3: the results camera looks at the player from the
   arena's centre (yaw atan2(x, z) + pi); no roll */
void CDemolition_Camera_ResultsC(struct CDemolition_Camera *self, float dt)
{
	char *s = (char *)self;
	char *player;
	int changed;
	float yaw;
	struct DbMessage message;

	(void)dt;
	DB_F32(s, 0x3c) = 0.0f;
	player = db_camera_results_player(s, &changed);
	if (player == NULL)
		return;
	yaw = (float)(atan2((double)DB_F32(player, 0x1144), (double)DB_F32(player, 0x114c)) +
		(double)3.1415927f);
	DB_F32(s, 0x30) = yaw;
	if (changed)
	{
		db_camera_results_turn(player, yaw);
		FUN_0010cec0((undefined4 *)&message);
		AI_SetState(player, 0x11);
		FUN_0010ced0((undefined4 *)&message);
	}
	db_camera_results_view(s, player);
}

/* 0x000f6270  fits the view to `count` points (x, y, z; w is set to 1): in the camera
   space of (pitch, yaw), the points below their mean height drop by 200; the target is the
   centre of the bounds, the eye is pulled back far enough that the field of view (+0x40,
   horizontally and at 0.75 vertically) holds all points, clamped to the distances
   +0x64/+0x68/+0x6c. Results in camera-space order (x, y, z, 1) rotated back to the world. */
void CDemolition_Camera_Fit(struct CDemolition_Camera *self, float *eye, float *target,
	float (*points)[4], int count, float pitch, float yaw)
{
	char *s = (char *)self;
	float m[16];
	float local[20][4];
	float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
	double inv;
	float meanY;
	double meanZ;
	float sinHalf;
	double tanV;
	float lo;
	double hi;
	double distY;
	double distX;
	double dist;
	double v;
	float centreX;
	float centreY;
	float eyeLocal[4];
	float targetLocal[4];
	float eyeZ;
	int i;

	bmMatrixRotationZXY(m, pitch, yaw, 0.0f);
	for (i = 0; i < count; i++)
	{
		points[i][3] = 1.0f;
		bmTransformVector4(local[i], m, points[i]);
		sumX += local[i][0];   /* summed, not used */
		sumY += local[i][1];
		sumZ += local[i][2];
	}
	(void)sumX;
	inv = 1.0 / (double)count;
	meanY = (float)(sumY * inv);
	meanZ = inv * sumZ;
	for (i = 0; i < count; i++)
		if (local[i][1] < meanY)
			local[i][1] = local[i][1] - 200.0f;

	/* vertical: half the field of view times 0.75 (the original also finds the largest z,
	   which is not used) */
	sinHalf = (float)sin((double)DB_F32(s, 0x40) * 0.5);
	tanV = (double)sinHalf * 0.75;
	lo = (float)((double)local[0][1] - (double)local[0][2] * tanV);
	hi = (double)local[0][2] * tanV + (double)local[0][1];
	for (i = 0; i < count; i++)
	{
		v = (double)local[i][1] - tanV * (double)local[i][2];
		if (v > (double)lo)
			lo = (float)v;
		v = tanV * (double)local[i][2] + (double)local[i][1];
		if (v < hi)
			hi = (float)v;
	}
	centreY = (float)(((double)lo + hi) * 0.5);
	distY = ((double)lo - hi) * 0.5 * (-1.0 / tanV);

	/* horizontal */
	lo = (float)((double)local[0][0] - (double)local[0][2] * (double)sinHalf);
	hi = (double)local[0][2] * (double)sinHalf + (double)local[0][0];
	for (i = 0; i < count; i++)
	{
		v = (double)local[i][0] - (double)sinHalf * (double)local[i][2];
		if (v > (double)lo)
			lo = (float)v;
		v = (double)sinHalf * (double)local[i][2] + (double)local[i][0];
		if (v < hi)
			hi = (float)v;
	}
	centreX = (float)(((double)lo + hi) * 0.5);
	distX = ((double)lo - hi) * 0.5 * (-1.0 / (double)sinHalf);
	dist = distY;
	if (distX < distY)
		dist = (float)distX;

	targetLocal[0] = centreX;
	targetLocal[1] = centreY;
	targetLocal[2] = (float)meanZ;
	targetLocal[3] = 1.0f;
	eyeLocal[0] = centreX;
	eyeLocal[1] = centreY;
	v = dist - (double)DB_F32(s, 0x64);
	eyeZ = (float)v;
	if (meanZ - v < (double)DB_F32(s, 0x68))
		eyeZ = (float)(meanZ - (double)DB_F32(s, 0x68));
	if (meanZ - (double)eyeZ > (double)DB_F32(s, 0x6c))
		eyeZ = (float)(meanZ - (double)DB_F32(s, 0x6c));
	eyeLocal[2] = eyeZ;
	eyeLocal[3] = 1.0f;
	bmMatrixRotationYXZ(m, -pitch, -yaw, 0.0f);
	bmTransformVector4(eye, m, eyeLocal);
	bmTransformVector4(target, m, targetLocal);
}

/* Moves the camera to the fitted eye and target: at once (`mode` bit 2) or following at
   the speeds +0x70/+0x74; then the players' AI objects face the camera. */
static void db_camera_fit_finish(char *s, float *eye, float *target, float dt, int mode)
{
	double angle;

	if (mode & 2)
	{
		memcpy(DB_CAM_EYE(s), eye, 16);
		memcpy(DB_CAM_TARGET(s), target, 16);
	}
	else
	{
		bmVectorLerp(DB_CAM_EYE(s), DB_CAM_EYE(s), eye, dt * DB_F32(s, 0x70));
		bmVectorLerp(DB_CAM_TARGET(s), DB_CAM_TARGET(s), target, dt * DB_F32(s, 0x74));
	}
	angle = atan2((double)DB_F32(s, 0x08) - (double)DB_F32(s, 0x18),
		(double)DB_F32(s, 0x10) - (double)DB_F32(s, 0x20));
	db_camera_turn_players(angle);
}

/* the players' positions (+0x1154) raised by 0.8 of their height (+0x1168); returns the
   number of points */
static int db_camera_player_points(float (*points)[4], int n, int withBombs)
{
	char *p;

	for (p = DB_GAME_PLAYERS(); p != NULL; p = DB_PLAYER_NEXT(p))
	{
		memcpy(points[n], DB_FP(p, 0x1154), 16);
		points[n][1] = DB_F32(p, 0x1168) * 0.8f + points[n][1];
		n++;
		if (withBombs)
		{
			char *bomb = (char *)DB_PTR(p, 0x11a0);

			if (DB_U8(bomb, 0x116d) != 0)
			{
				memcpy(points[n], DB_FP(bomb, 0x1154), 16);
				n++;
			}
		}
	}
	return n;
}

/* 0x000f68d0  modes 2 and 3 of variation 0: fit the view to the players (pitch -30, yaw
   45 degrees) */
void CDemolition_Camera_FollowFrag(struct CDemolition_Camera *self, float dt, int mode)
{
	char *s = (char *)self;
	float points[20][4];
	float eye[4];
	float target[4];
	int n;

	n = db_camera_player_points(points, 0, 0);
	DB_U32(s, 0x30) = 0x3f490fdb;   /* pi / 4 */
	DB_U32(s, 0x2c) = 0xbf060a92;   /* -0.5236 (-30 degrees) */
	CDemolition_Camera_Fit(self, eye, target, points, n, db_bitsf(0xbf060a92),
		db_bitsf(0x3f490fdb));
	db_camera_fit_finish(s, eye, target, dt, mode);
}

/* 0x000f6a40  modes 2 and 3 of variation 1: the players and their armed remote bombs
   (pitch -25 degrees) */
void CDemolition_Camera_FollowRemote(struct CDemolition_Camera *self, float dt, int mode)
{
	char *s = (char *)self;
	float points[20][4];
	float eye[4];
	float target[4];
	int n;

	n = db_camera_player_points(points, 0, 1);
	DB_U32(s, 0x30) = 0x3f490fdb;   /* pi / 4 */
	DB_U32(s, 0x2c) = 0xbedf66f3;   /* -0.4363 (-25 degrees) */
	CDemolition_Camera_Fit(self, eye, target, points, n, db_bitsf(0xbedf66f3),
		db_bitsf(0x3f490fdb));
	db_camera_fit_finish(s, eye, target, dt, mode);
}

/* 0x000f6c00  modes 2 and 3 of variation 2: the players and four points of the world
   (game +0x7c0, +0x7e0, +0x800, +0x820) */
void CDemolition_Camera_FollowTimed(struct CDemolition_Camera *self, float dt, int mode)
{
	char *s = (char *)self;
	float points[20][4];
	float eye[4];
	float target[4];
	int n;
	int i;

	n = db_camera_player_points(points, 0, 0);
	for (i = 0; i < 4; i++)
		memcpy(points[n++], DB_FP(DAT_002ff85c, 0x7c0 + i * 0x20), 16);
	DB_U32(s, 0x30) = 0x3f490fdb;   /* pi / 4 */
	DB_U32(s, 0x2c) = 0xbf060a92;   /* -30 degrees */
	CDemolition_Camera_Fit(self, eye, target, points, n, db_bitsf(0xbf060a92),
		db_bitsf(0x3f490fdb));
	db_camera_fit_finish(s, eye, target, dt, mode);
}

/* 0x000f6890  mode 4: the results camera of the variation */
void CDemolition_Camera_UpdateResults(struct CDemolition_Camera *self, float dt)
{
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
		CDemolition_Camera_ResultsA(self, dt);
		break;
	case 1:
		CDemolition_Camera_ResultsA(self, dt);
		break;
	case 2:
		CDemolition_Camera_ResultsB(self, dt);
		break;
	case 3:
		CDemolition_Camera_ResultsC(self, dt);
		break;
	}
}

/* 0x000f6e30  mode 2: the countdown; the view snaps to the players (mode 2) until the HUD
   countdown ends, then play (mode 3) */
void CDemolition_Camera_UpdateCountdown(struct CDemolition_Camera *self, float dt)
{
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
		CDemolition_Camera_FollowFrag(self, dt, 2);
		break;
	case 1:
		CDemolition_Camera_FollowRemote(self, dt, 2);
		break;
	case 2:
		CDemolition_Camera_FollowTimed(self, dt, 2);
		break;
	case 3:
		CDemolition_Camera_UpdateArena(self, dt, 2);
		break;
	}
	if (CLHud_UpdateCountdown(DAT_002ecfac) != 0)
		DB_I32(self, 0x44) = 3;
}

/* 0x000f6eb0  mode 3: play; the view follows the players */
void CDemolition_Camera_UpdatePlay(struct CDemolition_Camera *self, float dt)
{
	switch (DB_I32(DAT_002ff85c, 0x28))
	{
	case 0:
		CDemolition_Camera_FollowFrag(self, dt, 1);
		break;
	case 1:
		CDemolition_Camera_FollowRemote(self, dt, 1);
		break;
	case 2:
		CDemolition_Camera_FollowTimed(self, dt, 1);
		break;
	case 3:
		CDemolition_Camera_UpdateArena(self, dt, 1);
		break;
	}
}

/* 0x000f6f10  Update: the timer, the mode, the fade requests, then the view */
void CDemolition_Camera_Update(struct CDemolition_Camera *self, float dt)
{
	char *s = (char *)self;

	FUN_0018e8e0((void *)(s + 0x78));
	switch (DB_I32(s, 0x44))
	{
	case 1:
		CDemolition_Camera_UpdateIntro(self, dt);
		break;
	case 2:
		CDemolition_Camera_UpdateCountdown(self, dt);
		break;
	case 3:
		CDemolition_Camera_UpdatePlay(self, dt);
		break;
	case 4:
		CDemolition_Camera_UpdateResults(self, dt);
		break;
	case 5:
		break;
	}
	if (DB_I32(s, 0x58) != 0)
	{
		DB_I32(s, 0x4c) = 1;
		DB_I32(s, 0x58) = 0;
	}
	if (DB_I32(s, 0x5c) != 0)
	{
		DB_I32(s, 0x50) = 1;
		DB_I32(s, 0x5c) = 0;
	}
	if (DB_I32(s, 0x60) != 0)
	{
		DB_I32(s, 0x54) = 1;
		DB_I32(s, 0x60) = 0;
	}
	bdSetFieldOfView(DB_F32(s, 0x40), DB_F32(s, 0x40) * 3.0f * 0.25f);
	db_camera_look(s);
	bdUpdateView();
}
