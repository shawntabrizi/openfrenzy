/*
RollerBall/unplaced_systems.c  <-  C:\BlitzParty\Game\Src\RollerBall\ (files unknown)

The small sub-systems of the game object, gap functions between CRollerball_Game.cpp
and CRollerBall_Light.cpp (0x0018dad0-0x0018fcd0): rumble, floating score texts, shader
the video screens,
sound effects, Direct3D, debug, the simulation clock, timers, info, input, and the
abstract input objects (the shader system is in CRollerball_Shader.c). Each class has a one-slot vtable (the deleting destructor) unless
noted.
*/

#include "rollerball_a_ext.h"

/* ======================================================================== rumble (+0x28f0) */

/* 0x00274354  vtable of the rumble system */
const void *const vtable_00274354[1] = { (const void *)CRollerBallRumble_DeletingDtor };

/* 0x0018dad0  RollerBall (gap): constructor */
void CRollerBallRumble_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274354;
}

/* 0x0018dae0  RollerBall (gap): destructor */
void CRollerBallRumble_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274354;
}

/* a rumble record (+4..+0x1c) for PLAYER: strength x 32767, DURATION ms, then 10, 80, 0, 4;
sent to classlib_ui's rumble manager when the player is a pad player */
static void rb_Rumble(unsigned char *self, int player, float strength, int duration)
{
	RB_I32(self, 4) = 0;
	RB_I32(self, 8) = RB_FTOL(strength * 32767.0f);
	RB_I32(self, 0xc) = duration;
	RB_I32(self, 0x10) = 10;
	RB_I32(self, 0x14) = 0x50;
	RB_I32(self, 0x18) = 0;
	RB_I32(self, 0x1c) = 4;
	if ((&DAT_002baed8)[player * 10] != 0)
		CLRumble_Modify((int)(uintptr_t)DAT_002ed324, player, 1,
			(undefined4)(uintptr_t)(self + 4));
}

/* 0x0018daf0  RollerBall (gap): a short rumble (500 ms) */
void CRollerBallRumble_Short(void *self, int player, float strength)
{
	rb_Rumble(self, player, strength, 500);
}

/* 0x0018db60  RollerBall (gap): a long rumble (1000 ms) */
void CRollerBallRumble_Long(void *self, int player, float strength)
{
	rb_Rumble(self, player, strength, 1000);
}

/* 0x0018dbd0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallRumble_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00274354;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== floaty texts (+0x2850) */

/* 0x00274358  vtable of the floating score texts */
const void *const vtable_00274358[1] = { (const void *)CRollerBallFloaty_DeletingDtor };

/* 0x0018dbf0  RollerBall (gap): constructor */
void CRollerBallFloaty_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274358;
}

/* 0x0018dc00  RollerBall (gap): destructor */
void CRollerBallFloaty_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274358;
}

/* 0x0018dc10  RollerBall (gap)
Initialise: the font handgotl.ttf (+8) and a floating score system of 10 texts (+0x10,
bumpercars_b's 0x000a74d0). Returns 0 without the font. */
int CRollerBallFloaty_Init(void *self, void *package)
{
	struct bkResource *font;

	(void)package;
	font = bLoadResource(NULL, "handgotl.ttf", 3, 0xdefa);
	RB_PTR(self, 8) = font;
	if (font == NULL)
		return 0;
	BCarFloatyScore_Init((struct BCarFloatyScore *)RB_P(self, 0x10), 10);
	return 1;
}

/* 0x0018dc50  RollerBall (gap): Shutdown (deletes the extra system at +0x10) */
void CRollerBallFloaty_Shutdown(void *self)
{
	baDeleteExtraSystem(RB_PTR(self, 0x10));
}

/* 0x0018dc60  RollerBall (gap)
Update: the texts face the camera (+0x14 = -camera angle, game +0x139c). */
void CRollerBallFloaty_Update(void *self, float dt)
{
	RB_F32(self, 0x14) = -RB_F32(RB_GAME, 0x139c);
	baUpdateExtras(RB_PTR(self, 0x10), RB_FARG(dt));
}

/* 0x0018dc90  RollerBall (gap): Draw */
void CRollerBallFloaty_Draw(void *self)
{
	baDrawExtras(RB_PTR(self, 0x10), NULL);
}

/* 0x0018dca0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallFloaty_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00274358;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== the 0x100-byte system (+0x2740) */

/* 0x00274468  vtable of the system at game +0x2740 (only constructed) */
const void *const vtable_00274468[1] = { (const void *)CRollerBall2740_DeletingDtor };

/* 0x0018ddb0  RollerBall (gap): constructor */
void CRollerBall2740_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274468;
}

/* 0x0018ddc0  RollerBall (gap): destructor */
void CRollerBall2740_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274468;
}

/* 0x0018ddd0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBall2740_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00274468;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== video screens */

/* A video screen (0x58 bytes, vtable 0x0027446c): +4 next screen, +8 its render target,
+0xc..+0x18 the target's width, height, colour and z depth, +0x1c the eye, +0x2c the
target point, +0x3c and +0x40 orbit angles, +0x4c the time to the next shot, +0x50 the
shot (0 follow, 1 and 2 orbit), +0x54 the player shown. */

/* 0x0027446c  vtable of a video screen */
const void *const vtable_0027446c[1] = { (const void *)CRollerBallScreen_DeletingDtor };

/* 0x0018ddf0  RollerBall (missing from the table: a callback pushed by 0x0018de30)
The texture's select callback: stage 0 selects the screen's render target as the
texture. Returns 0 then, else 1. */
int FUN_0018ddf0(void *texture, int stage, void *context)
{
	(void)texture;
	if (stage == 0) {
		bdSetRenderTargetTexture(RB_PTR(context, 8), 0);
		return 0;
	}
	return 1;
}

/* 0x0018de20  RollerBall (gap): constructor */
void CRollerBallScreen_Ctor(void *self)
{
	*(const void *const **)self = vtable_0027446c;
}

/* 0x0018de30  RollerBall (gap)
Initialise: a WIDTH x HEIGHT render target (32-bit colour and z) that replaces the texture
TEXTURE whenever it is drawn; first shot in 0 s, orbit angle -60 degrees. Returns 1. */
int CRollerBallScreen_Init(void *self, const char *texture, int width, int height)
{
	struct bdRenderTarget *rt;
	struct bkResource *tex;

	RB_PTR(self, 4) = NULL;
	rt = bdCreateRenderTarget(width, height, 0x20, 0x20, 0);
	RB_PTR(self, 8) = rt;
	bdRenderTargetGetInfo(rt, RB_P(self, 0xc), RB_P(self, 0x10), RB_P(self, 0x14),
		RB_P(self, 0x18));
	tex = bLoadResource(NULL, texture, 0, 0xdefa);
	if (tex == NULL)
		bkPrintf("ERROR: CRollerBall_VideoScreen::Initialise Count not find texture <%s>!!!\n",
			texture);
	bdTextureSetCallback((struct bdTexture *)tex, 0, (void *)FUN_0018ddf0, self);
	RB_PTR(self, 0x54) = NULL;
	RB_F32(self, 0x40) = 0.0f;
	RB_F32(self, 0x4c) = 0.0f;
	RB_F32(self, 0x3c) = rb_bitsf(0xbf860a92u);
	RB_I32(self, 0x50) = -1;
	return 1;
}

/* 0x0018ded0  RollerBall (gap)
Render (when the variation has video screens, 0x002b4bb0): into the render target, cleared
to the variation's background colour, from the eye to the target: the platforms, then the
players (virtual +0x24) between the light system's switch (virtual +0xc of game +0x25bc);
then back to the back buffer and the full viewport. */
void CRollerBallScreen_Render(void *self)
{
	int v = RB_I32(RB_GAME, RB_GO_VARIATION);
	unsigned char *p;
	void *lights;

	if (RB_VS(int, v, 0x002b4bb0u) == 0)
		return;
	bdSetRenderTarget(RB_PTR(self, 8), RB_VS(unsigned int, v, 0x002b4bdcu),
		RB_VS(unsigned int, v, 0x002b4be0u), RB_VS(unsigned int, v, 0x002b4be4u), 0, 1.0f, 3);
	bdSetViewport(0, 0, RB_I32(self, 0xc), RB_I32(self, 0x10));
	bdSetCameraLookAt(RB_FP(self, 0x1c), RB_FP(self, 0x2c), 0.0f);
	bdUpdateView();
	bdSetRenderState(6, 0, 0);
	bdPushRenderState(3, 1, 0);
	CRollerballWorld_DrawPlatforms(RB_GAME + RB_GO_WORLD);
	lights = RB_PTR(RB_GAME, 0x25bc);
	RB_VFN(lights, 0x0c, void (*)(void *, int))(lights, 1);
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p))
		RB_VFN(p, 0x24, void (*)(void *, int))(p, 0);
	lights = RB_PTR(RB_GAME, 0x25bc);
	RB_VFN(lights, 0x0c, void (*)(void *, int))(lights, 0);
	bdPopRenderState(3);
	bdSetRenderTarget(NULL, 0, 0, 0, 0, 1.0f, 2);
	bdSetViewport(0, 0, DAT_002bc40c, DAT_002bc410);
}

/* 0x0018dfe0  RollerBall (gap): video screen system constructor */
void CRollerBallVideo_Ctor(void *self)
{
	*(const void *const **)self = vtable_002744bc;
}

/* 0x0018dff0  RollerBall (gap): video screen system destructor */
void CRollerBallVideo_Dtor(void *self)
{
	*(const void *const **)self = vtable_002744bc;
}

/* 0x0018e000  RollerBall (gap): Initialise (empty list). Returns 1. */
int CRollerBallVideo_Init(void *self)
{
	RB_PTR(self, 4) = NULL;
	return 1;
}

/* 0x0018e010  RollerBall (gap)
Adds ITEM at the head of the list at +4 (items linked by their +4). Also used for the
particle system list (game +0x2ad8). */
void CRollerBallList_Add(void *self, void *item)
{
	if (RB_PTR(self, 4) == NULL) {
		RB_PTR(item, 4) = NULL;
		RB_PTR(self, 4) = item;
		return;
	}
	RB_PTR(item, 4) = RB_PTR(self, 4);
	RB_PTR(self, 4) = item;
}

/* 0x0018e040  RollerBall (gap)
Shutdown: every screen's render target is deleted and the screen too (virtual deleting
destructor). */
void CRollerBallVideo_Shutdown(void *self)
{
	unsigned char *s, *next;

	s = RB_PTR(self, 4);
	while (s != NULL) {
		next = RB_PTR(s, 4);
		bdDeleteRenderTarget(RB_PTR(s, 8));
		RB_VFN(s, 0x00, void *(*)(void *, unsigned int))(s, 1);
		s = next;
	}
}

/* 0x0018e070  RollerBall (gap): Draw (renders every screen) */
void CRollerBallVideo_Draw(void *self)
{
	unsigned char *s;

	for (s = RB_PTR(self, 4); s != NULL; s = RB_PTR(s, 4))
		CRollerBallScreen_Render(s);
}

/* 0x0018e090  RollerBall (gap): video screen deleting destructor (flags bit 0: free) */
void *CRollerBallScreen_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0027446c;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018e0b0  RollerBall (gap)
Shot 0 (follow): the eye is the player's position raised by half its radius, looking
along its heading. */
void CRollerBallScreen_Follow(void *self, float dt)
{
	unsigned char *p = RB_PTR(self, 0x54);
	float v[4], out[4];

	(void)dt;
	if (p == NULL)
		return;
	RB_F32(self, 0x1c) = RB_F32(p, RB_PL_POS);
	RB_F32(self, 0x20) = RB_F32(p, RB_PL_POS + 4);
	RB_F32(self, 0x24) = RB_F32(p, RB_PL_POS + 8);
	RB_F32(self, 0x28) = RB_F32(p, RB_PL_POS + 12);
	p = RB_PTR(self, 0x54);
	v[0] = 0.0f;
	v[1] = 0.0f;
	RB_F32(self, 0x20) = RB_F32(p, 0x413c) * 0.5f + RB_F32(self, 0x20);
	v[2] = RB_F32(p, 0x413c);
	v[3] = 0.0f;
	bmVectorRotateY(out, v, RB_F32(p, RB_PL_HEADING));
	RB_F32(self, 0x2c) = out[0] + RB_F32(self, 0x1c);
	RB_F32(self, 0x34) = out[2] + RB_F32(self, 0x24);
}

/* 0x0018e160  RollerBall (gap)
Shots 1 and 2 (orbit): the eye circles the player at 500, half a degree per frame. */
void CRollerBallScreen_Orbit(void *self, float dt)
{
	unsigned char *p;
	float v[4], out[4];

	if (RB_PTR(self, 0x54) == NULL)
		return;
	RB_F32(self, 0x40) = dt * rb_bitsf(0x3c0efa35u) /* 0x002744c0 */ + RB_F32(self, 0x40);
	RB_F32(self, 0x40) = CLMath_WrapAngle(RB_F32(self, 0x40));
	p = RB_PTR(self, 0x54);
	RB_F32(self, 0x2c) = RB_F32(p, RB_PL_POS);
	RB_F32(self, 0x30) = RB_F32(p, RB_PL_POS + 4);
	RB_F32(self, 0x34) = RB_F32(p, RB_PL_POS + 8);
	RB_F32(self, 0x38) = RB_F32(p, RB_PL_POS + 12);
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 500.0f;
	v[3] = 0.0f;
	bmVectorRotateXY(out, v, RB_F32(self, 0x3c), RB_F32(self, 0x40));
	RB_F32(self, 0x1c) = out[0] + RB_F32(self, 0x2c);
	RB_F32(self, 0x24) = out[2] + RB_F32(self, 0x34);
}

/* 0x0018e210  RollerBall (gap): picks another random player of the first four */
void CRollerBallScreen_PickPlayer(void *self)
{
	unsigned char *p;
	unsigned int k;

	do {
		k = (unsigned int)rb_rand() & 3;
		p = RB_FIRST_PLAYER();
		for (; k != 0; k--)
			p = RB_NEXT(p);
	} while (p == RB_PTR(self, 0x54));
	RB_PTR(self, 0x54) = p;
}

/* 0x0018e270  RollerBall (gap): video screen system deleting destructor */
void *CRollerBallVideo_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002744bc;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018e290  RollerBall (gap)
Update (when the variation has video screens): when the shot time runs out a new random
shot (not the same) of another player for 150..450 frames; then the shot. */
void CRollerBallVideo_UpdateScreen(void *self, float dt)
{
	unsigned int shot;
	float t;

	if (RB_VS(int, RB_I32(RB_GAME, RB_GO_VARIATION), 0x002b4bb0u) == 0)
		return;
	t = RB_F32(self, 0x4c) - dt;
	RB_F32(self, 0x4c) = t;
	if (t <= 0.0f) {
		do
			shot = (unsigned int)rb_rand() % 3;
		while (shot == RB_U32(self, 0x50));
		RB_U32(self, 0x50) = shot;
		CRollerBallScreen_PickPlayer(self);
		RB_F32(self, 0x4c) = ((float)rb_rand() * RB_RAND_SCALE + 0.5f) * 300.0f;
	}
	switch (RB_I32(self, 0x50)) {
	case 0:
		CRollerBallScreen_Follow(self, dt);
		return;
	case 1:
	case 2:
		CRollerBallScreen_Orbit(self, dt);
		return;
	}
}

/* 0x0018e370  RollerBall (gap): Update (every screen) */
void CRollerBallVideo_Update(void *self, float dt)
{
	unsigned char *s;

	for (s = RB_PTR(self, 4); s != NULL; s = RB_PTR(s, 4))
		CRollerBallVideo_UpdateScreen(s, dt);
}

/* 0x002744bc  vtable of the video screen system */
const void *const vtable_002744bc[1] = { (const void *)CRollerBallVideo_DeletingDtor };
