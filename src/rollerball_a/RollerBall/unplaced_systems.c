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

/* ======================================================================== sound effects (+0x26fc) */

/* 0x002745d8  vtable of the sound effect system */
const void *const vtable_002745d8[1] = { (const void *)CRollerBallSfx_DeletingDtor };

/* 0x0018e3a0  RollerBall (gap): constructor */
void CRollerBallSfx_Ctor(void *self)
{
	*(const void *const **)self = vtable_002745d8;
}

/* 0x0018e3b0  RollerBall (gap): destructor */
void CRollerBallSfx_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745d8;
}

/* 0x0018e3c0  RollerBall (gap)
Initialise: the 14 samples named at 0x002b4fe0 (sfx\ball2ball01.wav, ...) from PACKAGE into
+4.. Returns 1. */
int CRollerBallSfx_Init(void *self, void *package)
{
	const char *const *name = (const char *const *)RB_ADDR(0x002b4fe0u);
	struct bkResource **slot = (struct bkResource **)RB_P(self, 4);
	int i;

	for (i = 0; i < 14; i++, name++, slot++) {
		*slot = NULL;
		*slot = bLoadResource((struct bkPackage *)package, *name, 2, 0xdefa);
		if (*slot != NULL)
			bkPrintf("Loaded sample <%s>\n", *name);
	}
	return 1;
}

/* 0x0018e420  RollerBall (gap): Shutdown: stops every sound and deletes the samples */
void CRollerBallSfx_Shutdown(void *self)
{
	struct bkResource **slot = (struct bkResource **)RB_P(self, 4);
	int i;

	bsStopAll();
	for (i = 0; i < 14; i++, slot++) {
		if (*slot != NULL) {
			bkDeleteResource(*slot);
			*slot = NULL;
		}
	}
}

/* 0x0018e450  RollerBall (gap)
Plays sample SAMPLE (volume, pan, frequency, flags as bsPlaySample). Returns the channel,
or -1 when the sample is missing. Also used by Demolition (its sfx system has the same
layout). */
int CRollerBallSfx_Play(void *self, int sample, int volume, int pan, unsigned int frequency,
	float *flags)
{
	struct BSample *s = RB_AT(self, 4 + sample * 4, struct BSample *);

	if (s != NULL)
		return bsPlaySample(s, volume, pan, (int)frequency, (unsigned int)(uintptr_t)flags);
	return -1;
}

/* 0x0018e490  RollerBall (gap): sets a channel's frequency. The original is a method of the
sound effect system (ret 8) that does not use ECX; the port drops `this`, so the table's
two-argument shape (Demolition's callers) stays valid. */
void CRollerBallSfx_SetFrequency(int channel, int frequency)
{
	bsSetChannelFrequency(channel, frequency);
}

/* 0x0018e4b0  RollerBall (gap): sets a channel's volume (ECX unused, as 0x0018e490) */
void CRollerBallSfx_SetVolume(int channel, int volume)
{
	bsSetChannelVolume(channel, (unsigned short)volume);
}

/* 0x0018e4d0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallSfx_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745d8;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== Direct3D (+0x26f4) */

/* 0x002745dc  vtable of the Direct3D helper */
const void *const vtable_002745dc[1] = { (const void *)CRollerBallDirect3D_DeletingDtor };

/* 0x0018e4f0  RollerBall (gap): constructor */
void CRollerBallDirect3D_Ctor(void *self)
{
	*(const void *const **)self = vtable_002745dc;
}

/* 0x0018e500  RollerBall (gap): destructor */
void CRollerBallDirect3D_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745dc;
}

/* 0x0018e510  RollerBall (gap): Initialise: keeps the device (bdisplay's 0x002bc47c). Returns 1. */
int CRollerBallDirect3D_Init(void *self)
{
	RB_PTR(self, 4) = DAT_002bc47c;
	return 1;
}

/* 0x0018e520  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallDirect3D_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745dc;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018e540  RollerBall (gap)
Texture stage 1 modulates the texture factor's alpha (ALPHA << 24): colour op 14, alpha
op from the factor, texture coordinates 3. Also used by Demolition. A method of the
Direct3D helper (game +0x26f4, ret 4) that does not use ECX: the port drops `this`. */
void CRollerBall_SetFactorAlphaStage(int alpha)
{
	D3DDevice_SetRenderState_TextureFactor((unsigned long)alpha << 0x18);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xa, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
}

/* 0x0018e5d0  RollerBall (gap)
Stencil on: write reference 1 always (stencil function 0x207 ALWAYS, pass op 0x1e01
REPLACE, mask 1). The inlined D3DDevice_SetRenderState calls keep the state table. */
void CRollerBall_StencilWrite(void)
{
	D3DDevice_SetRenderState_StencilEnable(1);
	D3DDevice_SetRenderState_Simple(0x40368, 1);
	D3D__RenderState[71] = 1;      /* 0x00203a6c */
	D3DDevice_SetRenderState_Simple(0x40378, 0x1e01);
	D3D__RenderState[69] = 0x1e01; /* 0x00203a64 */
	D3DDevice_SetRenderState_Simple(0x40360, 1);
	D3D__RenderState[73] = 1;      /* 0x00203a74 */
	D3DDevice_SetRenderState_Simple(0x40364, 0x207);
	D3D__RenderState[70] = 0x207;  /* 0x00203a68 */
}

/* 0x0018e640  RollerBall (gap): stencil off (also used by BumperCars) */
void CRollerBall_StencilOff(void)
{
	D3DDevice_SetRenderState_StencilEnable(0);
}

/* 0x0018e650  RollerBall (gap)
Stencil on: draw where the stencil is not 0 (function 0x202 ... reference mask 1). */
void CRollerBall_StencilTest(void)
{
	D3DDevice_SetRenderState_StencilEnable(1);
	D3DDevice_SetRenderState_Simple(0x40364, 0x202);
	D3D__RenderState[70] = 0x202;  /* 0x00203a68 */
	D3DDevice_SetRenderState_Simple(0x4036c, 1);
	D3D__RenderState[72] = 1;      /* 0x00203a70 */
}

/* ======================================================================== debug (+0x26e8) */

/* 0x002745e0  vtable of the debug system */
const void *const vtable_002745e0[1] = { (const void *)CRollerBallDebug_DeletingDtor };

/* 0x0018e690  RollerBall (gap): constructor */
void CRollerBallDebug_Ctor(void *self)
{
	*(const void *const **)self = vtable_002745e0;
}

/* 0x0018e6a0  RollerBall (gap): destructor */
void CRollerBallDebug_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745e0;
}

/* 0x0018e6b0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallDebug_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745e0;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== frame clock (+0x25f0) */

/* The frame clock: +0x80 tick frequency, +0x88 now, +0x90 last frame, +0x98 frame time in
ms, +0x9c frame time in frames (+0x9c, read as game +0x268c), +0xb4 paused, +0xb8 pause
start. */

/* 0x002745e4  vtable of the frame clock */
const void *const vtable_002745e4[1] = { (const void *)CRollerBallClock_DeletingDtor };

/* 0x0018e6d0  RollerBall (gap): constructor */
void CRollerBallClock_Ctor(void *self)
{
	*(const void *const **)self = vtable_002745e4;
}

/* 0x0018e6e0  RollerBall (gap): destructor */
void CRollerBallClock_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745e4;
}

/* 0x0018e6f0  RollerBall (gap)
Initialise: the bKernel timer, the tick frequency (bkernel's 0x002cd7a0), now. Returns 1. */
int CRollerBallClock_Init(void *self)
{
	unsigned long long now;

	bInitTimer();
	RB_F32(self, 0x9c) = 0.0f;
	RB_F32(self, 0x98) = 0.0f;
	RB_AT(self, 0x80, long long) = DAT_002cd7a0;
	now = bkTimerRead();
	RB_AT(self, 0x88, unsigned long long) = now;
	RB_I32(self, 0x94) = RB_I32(self, 0x8c);
	RB_U8(self, 0xa0) = 0;
	RB_I32(self, 0xa8) = 0;
	RB_I32(self, 0xac) = 0;
	RB_I32(self, 0xb0) = 0;
	RB_U8(self, 0xb4) = 0;
	RB_I32(self, 0x90) = (int)(unsigned int)now;
	return 1;
}

/* 0x0018e770  RollerBall (gap)
Update: the frame time in ms (+0x98) and in frames of 1000 / DAT_002ba98c ms (+0x9c);
while the game is paused (DAT_002ecdec) the clock stands, and the pause is skipped. */
void CRollerBallClock_Update(void *self)
{
	unsigned long long now, delta;
	double ms;

	if (RB_U8(self, 0xb4) == 0) {
		if (DAT_002ecdec != 0) {
			RB_U8(self, 0xb4) = 1;
			RB_AT(self, 0xb8, unsigned long long) = bkTimerRead();
		}
		now = bkTimerRead();
		RB_AT(self, 0x88, unsigned long long) = now;
		delta = bkTimerDelta(RB_AT(self, 0x90, unsigned long long), now);
		RB_I32(self, 0x90) = RB_I32(self, 0x88);
		RB_I32(self, 0x94) = RB_I32(self, 0x8c);
		ms = (double)(long long)(delta * 1000ull) / (double)RB_AT(self, 0x80, long long);
		RB_F32(self, 0x98) = (float)ms;
		RB_F32(self, 0x9c) = (float)(ms / (1000.0 / (double)DAT_002ba98c));
	} else if (DAT_002ecdec == 0) {
		RB_U8(self, 0xb4) = 0;
		now = bkTimerRead();
		delta = bkTimerDelta(RB_AT(self, 0xb8, unsigned long long), now);
		RB_AT(self, 0x90, unsigned long long) = RB_AT(self, 0x90, unsigned long long) + delta;
	}
}

/* 0x0018e880  RollerBall (gap): Reset: no frame time, now is the last frame */
void CRollerBallClock_Reset(void *self)
{
	unsigned long long now;

	RB_F32(self, 0x9c) = 0.0f;
	RB_F32(self, 0x98) = 0.0f;
	now = bkTimerRead();
	RB_AT(self, 0x88, unsigned long long) = now;
	RB_AT(self, 0x90, unsigned long long) = now;
	RB_U8(self, 0xb4) = 0;
}

/* 0x0018ea80  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallClock_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745e4;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== timers */

/* A count-down timer (vtable 0x002745e8; also used by Demolition): +8 running, +9 paused,
+0xa stopped, +0x10 start ticks, +0x18 tick frequency (set by CDemoCountdown_Init),
+0x20 pause start, +0x28 duration (s), +0x2c duration (ms), +0x30 remaining (s),
+0x34 elapsed fraction 0..1. */

/* 0x002745e8  vtable of a timer */
const void *const vtable_002745e8[1] = { (const void *)CRollerBallTimer_DeletingDtor };

/* 0x0018e8c0  RollerBall (gap): constructor */
void CRollerBallTimer_Ctor(void *self)
{
	*(const void *const **)self = vtable_002745e8;
}

/* 0x0018e8d0  RollerBall (gap): destructor */
void CRollerBallTimer_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745e8;
}

/* 0x0018e8e0  RollerBall (gap)
Update (unless stopped): the remaining time and the elapsed fraction; while the game is
paused (DAT_002ecdec) the timer stands, and the pause is added to the start. */
void CRollerBallTimer_Update(void *self)
{
	unsigned long long now, delta;
	double elapsed;
	float remaining;

	if (RB_U8(self, 0xa) != 0)
		return;
	if (RB_U8(self, 9) != 0) {
		if (DAT_002ecdec == 0) {
			RB_U8(self, 9) = 0;
			now = bkTimerRead();
			delta = bkTimerDelta(RB_AT(self, 0x20, unsigned long long), now);
			RB_AT(self, 0x10, unsigned long long) = RB_AT(self, 0x10, unsigned long long) + delta;
		}
		return;
	}
	if (DAT_002ecdec != 0) {
		RB_U8(self, 9) = 1;
		RB_AT(self, 0x20, unsigned long long) = bkTimerRead();
	}
	if (RB_U8(self, 8) == 0) {
		elapsed = 0.0;
		RB_F32(self, 0x34) = 0.0f;
	} else {
		now = bkTimerRead();
		delta = bkTimerDelta(RB_AT(self, 0x10, unsigned long long), now);
		if (RB_F32(self, 0x2c) == 0.0f) {
			RB_F32(self, 0x34) = 1.0f;
		} else {
			RB_F32(self, 0x34) = (float)(((double)(long long)(delta * 1000ull) /
				(double)RB_AT(self, 0x18, long long)) / (double)RB_F32(self, 0x2c));
		}
		elapsed = (double)(long long)delta / (double)RB_AT(self, 0x18, long long);
	}
	remaining = (float)((double)RB_F32(self, 0x28) - elapsed);
	RB_F32(self, 0x30) = remaining;
	if (remaining < 0.0f)
		RB_F32(self, 0x30) = 0.0f;
	if (RB_F32(self, 0x34) > 1.0f)
		RB_F32(self, 0x34) = 1.0f;
}

/* 0x0018ea10  RollerBall (gap): Set: duration SECONDS, not running */
void CRollerBallTimer_Set(void *self, float seconds)
{
	RB_F32(self, 0x28) = seconds;
	RB_U8(self, 8) = 0;
	RB_F32(self, 0x2c) = seconds * 1000.0f;
}

/* 0x0018ea30  RollerBall (gap): Start: running from now */
void CRollerBallTimer_Start(void *self)
{
	RB_U8(self, 8) = 1;
	RB_AT(self, 0x10, unsigned long long) = bkTimerRead();
}

/* 0x0018ea50  RollerBall (gap): the remaining time (s) */
float CRollerBallTimer_Remaining(void *self)
{
	return RB_F32(self, 0x30);
}

/* 0x0018ea60  RollerBall (gap): Stop (the remaining time stays) */
void CRollerBallTimer_Stop(void *self)
{
	RB_U8(self, 0xa) = 1;
}

/* 0x0018ea70  RollerBall (gap): Reset: not running, not paused, not stopped */
void CRollerBallTimer_Reset(void *self)
{
	RB_U8(self, 8) = 0;
	RB_U8(self, 9) = 0;
	RB_U8(self, 0xa) = 0;
}

/* 0x0018eaa0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallTimer_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745e8;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== input (+0x25cc) */

/* 0x002745ec  vtable of the input system */
const void *const vtable_002745ec[1] = { (const void *)CRollerBallInput_DeletingDtor };

/* 0x0018eac0  RollerBall (gap): constructor */
void CRollerBallInput_Ctor(void *self)
{
	*(const void *const **)self = vtable_002745ec;
}

/* 0x0018ead0  RollerBall (gap): destructor */
void CRollerBallInput_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745ec;
}

/* 0x0018eae0  RollerBall (gap)
Initialise: every player gets its index (virtual +0x18); an input map of 6 channels (+4)
with one "skip" channel per pad (+0xc..+0x18, bound to the player's pad | 0xc5). Returns
1. */
int CRollerBallInput_Init(void *self)
{
	unsigned char *p;
	struct biChannel **ch;
	int *pad;
	int i = 0;

	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), i++)
		RB_VFN(p, 0x18, void (*)(void *, int))(p, i);
	RB_PTR(self, 4) = biCreateMap(6);
	ch = (struct biChannel **)RB_P(self, 0xc);
	for (pad = &DAT_002baee0; pad < &DAT_002baee0 + 40; pad += 10, ch++) {
		*ch = biCreateChannel(RB_PTR(self, 4), "skip", 1, 1, 0, 0, 0);
		biBindInput(*ch, (uint)*pad | 0xc5, 0);
	}
	return 1;
}

/* 0x0018eb70  RollerBall (gap): Shutdown: the channels and the map */
void CRollerBallInput_Shutdown(void *self)
{
	struct biChannel **ch = (struct biChannel **)RB_P(self, 0xc);
	int i;

	for (i = 4; i != 0; i--, ch++)
		biDeleteChannel(*ch);
	biDeleteMap(RB_PTR(self, 4));
}

/* 0x0018ebb0  RollerBall (gap)
Update: every player reads its input (virtual +0x1c) with the frame time (game +0x268c). */
void CRollerBallInput_Update(void *self)
{
	float dt = RB_F32(RB_GAME, 0x268c);
	unsigned char *p;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p))
		RB_VFN(p, 0x1c, void (*)(void *, float))(p, dt);
}

/* 0x0018ebf0  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallInput_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745ec;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* ======================================================================== physics (+0x25c0) */

/* The entities' collision state (rollerball_b's entities, layout from this code):
+0x18 on the ground this frame, +0x1b moves, +0x20 contact flags (1 world, 2 entities),
+0x24 + 4i world object i hit, +0x4c + 4i its hit count (at most 30), +0x74 + 0x618 i its
hits (0x34 bytes each: +0x20 time, +0x2c normal), +0x3d64 entity contacts, +0x3d68 the
other entities, +0x3d78 + 0xd0 k the contact records (+0x4 time, +0x10 normal),
+0x40b8 the collision sphere (centre, +0x40c8 radius), +0x40cc the body (vtable
0x002745f0), +0x40d0 position, +0x40e0 last position, +0x40f0 velocity, +0x4110 the last
world normal, +0x4174 / +0x4178 push factors, +0x417c bounce, +0x4198 time in the air. */

/* 0x002745f0  vtable of an entity's physics body (constructed by rollerball_b) */
const void *const vtable_002745f0[1] = { (const void *)CRollerBallBody_DeletingDtor };

/* 0x002745f4  vtable of the physics system */
const void *const vtable_002745f4[1] = { (const void *)CRollerBallPhysics_DeletingDtor };

/* 0x0018ec10  RollerBall (gap): body constructor */
void CRollerBallBody_Ctor(void *self)
{
	*(const void *const **)self = vtable_002745f0;
}

/* 0x0018ec20  RollerBall (gap): body destructor */
void CRollerBallBody_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745f0;
}

/* 0x0018ec30  RollerBall (gap): constructor */
void CRollerBallPhysics_Ctor(void *self)
{
	*(const void *const **)self = vtable_002745f4;
}

/* 0x0018ec40  RollerBall (gap): destructor */
void CRollerBallPhysics_Dtor(void *self)
{
	*(const void *const **)self = vtable_002745f4;
}

/* 0x0018ec50  RollerBall (gap): Initialise: on, no entities. Returns 1. */
int CRollerBallPhysics_Init(void *self)
{
	RB_U8(self, 4) = 1;
	RB_PTR(self, 8) = NULL;
	return 1;
}

/* 0x0018ec60  RollerBall (gap): the entity list (its first entity) */
void CRollerBallPhysics_SetPlayers(void *self, void *first)
{
	RB_PTR(self, 8) = first;
}

/* 0x0018ec70  RollerBall (gap): body deleting destructor (flags bit 0: free) */
void *CRollerBallBody_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745f0;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018ec90  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallPhysics_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_002745f4;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* bmSweepSphereSphereVec (engine_misc 0x00053af0), inlined: the original returns the hit
result of bmSweepSphereSphere, but engine_misc declares 0x00053af0 void */
static int rb_SweepSphereSphereVec(void *a, void *b, float *vec, void *hit)
{
	float len, inv, dir[4];

	len = sqrtf(vec[2] * vec[2] + vec[1] * vec[1] + vec[0] * vec[0]);
	inv = 1.0f / len;
	dir[0] = inv * vec[0];
	dir[1] = inv * vec[1];
	dir[2] = inv * vec[2];
	dir[3] = 0.0f; /* not set by the original */
	return bmSweepSphereSphere(a, b, dir, len, hit);
}

/* the velocity is almost 0 (each component within the threshold 0x00285644 = 0.05, which
the code sets first): it becomes 0 */
static int rb_Resting(unsigned char *e)
{
	if (fabsf(RB_F32(e, RB_PL_VEL)) <= DAT_00285644 &&
			fabsf(RB_F32(e, RB_PL_VEL + 4)) <= DAT_00285644 &&
			fabsf(RB_F32(e, RB_PL_VEL + 8)) <= DAT_00285644) {
		RB_F32(e, RB_PL_VEL) = 0.0f;
		RB_F32(e, RB_PL_VEL + 4) = 0.0f;
		RB_F32(e, RB_PL_VEL + 8) = 0.0f;
		return 1;
	}
	return 0;
}

/* 0x0018ecb0  RollerBall (gap)
The moving entities that entity E hits this frame (sphere sweeps with its velocity):
contact records with the normal from the other entity to E. Returns the count (0 and a
cleared velocity when E rests). */
int CRollerBallPhysics_CollideEntities(void *self, unsigned char *e)
{
	unsigned char *o;
	float n[4];
	int count = 0, off = 0, slot = 0x3d68;

	DAT_00285644 = 0.05f;
	for (o = RB_PTR(self, 8); o != NULL; o = RB_NEXT(o)) {
		if (o == e || RB_U8(o, RB_PL_1B) == 0)
			continue;
		RB_I32(e, 0x3d78 + off) = 9;
		if (rb_Resting(e))
			return 0;
		RB_F32(o, 0x40b8) = RB_F32(o, RB_PL_POS);
		RB_F32(o, 0x40bc) = RB_F32(o, RB_PL_POS + 4);
		RB_F32(o, 0x40c0) = RB_F32(o, RB_PL_POS + 8);
		RB_F32(o, 0x40c4) = RB_F32(o, RB_PL_POS + 12);
		if (rb_SweepSphereSphereVec(e + 0x40b8, o + 0x40b8, RB_FP(e, RB_PL_VEL),
				e + 0x3d78 + off) != 0) {
			n[0] = RB_F32(e, RB_PL_POS) - RB_F32(o, RB_PL_POS);
			n[1] = RB_F32(e, RB_PL_POS + 4) - RB_F32(o, RB_PL_POS + 4);
			n[2] = RB_F32(e, RB_PL_POS + 8) - RB_F32(o, RB_PL_POS + 8);
			n[3] = 0.0f; /* the original leaves this word of the stack unset */
			bmVectorNormalise(n, n);
			RB_F32(e, 0x3d88 + off) = n[0];
			RB_F32(e, 0x3d8c + off) = n[1];
			RB_F32(e, 0x3d90 + off) = n[2];
			RB_F32(e, 0x3d94 + off) = n[3];
			count++;
			RB_PTR(e, slot) = o;
			slot += 4;
			off += 0xd0;
		}
	}
	RB_I32(e, 0x3d64) = count;
	return count;
}

/* 0x0018ee70  RollerBall (gap)
The world objects (game +0x158, 0x88 bytes each, active at +1, collision mesh at +0x78)
that entity E hits this frame (sphere sweep against each mesh, at most 30 hits); a hit
puts E on the ground (+0x18). Returns the number of objects hit (0 and a cleared velocity
when E rests). */
int CRollerBallPhysics_CollideWorld(void *self, unsigned char *e)
{
	float sphere[5];
	unsigned char *obj;
	int n = RB_I32(RB_GAME, 0x6a8);
	int i, hits = 0;

	(void)self;
	DAT_00285644 = 0.05f;
	for (i = 0; i < n; i++) {
		obj = RB_GAME + 0x158 + i * 0x88;
		if (RB_U8(obj, 1) == 0) {
			RB_I32(e, 0x24 + i * 4) = 0;
			continue;
		}
		sphere[0] = RB_F32(e, RB_PL_POS);
		sphere[1] = RB_F32(e, RB_PL_POS + 4);
		sphere[2] = RB_F32(e, RB_PL_POS + 8);
		sphere[3] = RB_F32(e, RB_PL_POS + 12);
		sphere[4] = RB_F32(e, 0x40c8);
		RB_I32(e, 0x74 + i * 0x618) = 0xd;
		RB_I32(e, 0x4c + i * 4) = 0x1e;
		if (rb_Resting(e))
			return 0;
		if (bmSweepSphereMesh((struct BSphere *)sphere, RB_PTR(obj, 0x78), RB_FP(e, RB_PL_VEL),
				(int *)RB_P(e, 0x4c + i * 4), (struct BCollHit *)RB_P(e, 0x74 + i * 0x618)) == 0) {
			RB_I32(e, 0x24 + i * 4) = 0;
		} else {
			RB_U8(e, RB_PL_18) = 1;
			hits++;
			RB_I32(e, 0x24 + i * 4) = 1;
		}
	}
	return hits;
}

/* clamps the length of velocity V to 40 */
static void rb_ClampSpeed(float *v)
{
	if (40.0f < sqrtf(v[2] * v[2] + v[1] * v[1] + v[0] * v[0])) {
		bmVectorNormalise(v, v);
		v[0] = v[0] * 40.0f;
		v[1] = v[1] * 40.0f;
		v[2] = v[2] * 40.0f;
	}
}

/* 0x0018f050  RollerBall (gap)
Resolves entity E's first contact: a world contact (the earliest hit time) slides E along
the average normal of the hits up to that time (with bounce +0x417c; message 1 to E for a
wall, message 2 for a landing after 20 frames in the air), an entity contact pushes both
entities (factors +0x4174, +0x4178; in sumo after 58.5 s left and in drop an extra push
of 3.5 with the speed clamped to 40; message 0 to E). Returns the contact flags (0 when
there was no contact). */
int CRollerBallPhysics_Resolve(void *self, unsigned char *e)
{
	struct RB_Msg msg;
	int nobj = RB_I32(RB_GAME, 0x6a8);
	int world, ents, i, j, k, count, objs = 0, result;
	float tw = 1.0f, te = 1.0f, nx, ny, nz, sx, sy, sz, inv, d, bounce, tmp[4];
	float *v = RB_FP(e, RB_PL_VEL);
	unsigned char *h, *o;

	(void)self;
	rbb_MsgCtor(&msg);
	world = RB_I32(e, 0x20) & 1;
	if (world != 0) {
		for (i = 0; i < nobj; i++) {
			if (RB_I32(e, 0x24 + i * 4) != 0) {
				if (RB_F32(e, 0x78 + i * 0x618) < tw)
					tw = RB_F32(e, 0x78 + i * 0x618);
				objs++;
			}
		}
	}
	ents = RB_I32(e, 0x20) & 2;
	if (ents != 0) {
		for (k = 0; k < RB_I32(e, 0x3d64); k++) {
			if (RB_F32(e, 0x3d7c + k * 0xd0) < te)
				te = RB_F32(e, 0x3d7c + k * 0xd0);
		}
	}
	if (world == 0) {
		if (ents == 0) {
			rbb_MsgDtor(&msg);
			return 0;
		}
	} else if (ents == 0 || tw <= te) {
		nx = 0.0f;
		ny = 0.0f;
		nz = 0.0f;
		for (i = 0; i < nobj; i++) {
			if (RB_I32(e, 0x24 + i * 4) == 0)
				continue;
			h = e + 0x74 + i * 0x618;
			sx = RB_F32(h, 0x10);
			sy = RB_F32(h, 0x14);
			sz = RB_F32(h, 0x18);
			count = 1;
			for (j = 1; j < RB_I32(e, 0x4c + i * 4); j++) {
				if (tw < RB_F32(h, 0x34 * (j - 1) + 0x38))
					break;
				count = j + 1;
				sx = sx + RB_F32(h, 0x34 * j + 0x10);
				sy = sy + RB_F32(h, 0x34 * j + 0x14);
				sz = sz + RB_F32(h, 0x34 * j + 0x18);
			}
			inv = 1.0f / (float)count;
			nx = inv * sx + nx;
			ny = inv * sy + ny;
			nz = inv * sz + nz;
		}
		inv = 1.0f / (float)objs;
		nx = nx * inv;
		ny = ny * inv;
		nz = inv * nz;
		if (ny <= 0.6f) {
			msg.f[0] = v[0];
			msg.f[2] = v[2];
			msg.f[1] = v[1];
			msg.f[3] = v[3];
			msg.id = 1;
			msg.prio = 2;
			msg.unique = 1;
			msg.f[4] = nx;
			msg.f[5] = ny;
			msg.f[6] = nz;
			msg.f[7] = 0.0f; /* unset in the original */
			rbb_PostMsg(e, &msg);
		}
		if (RB_F32(e, 0x4198) >= 20.0f) {
			msg.f[0] = v[0];
			msg.f[2] = v[2];
			msg.f[1] = v[1];
			msg.f[3] = v[3];
			msg.id = 2;
			msg.prio = 2;
			msg.unique = 1;
			msg.f[4] = nx;
			msg.f[5] = ny;
			msg.f[6] = nz;
			msg.f[7] = 0.0f; /* unset in the original */
			rbb_PostMsg(e, &msg);
		}
		bmVectorNormalise(tmp, v); /* the result is not used */
		bounce = RB_F32(e, 0x417c);
		d = nx * v[0] + ny * v[1] + nz * v[2];
		sx = v[0] - nx * d;
		sy = v[1] - ny * d;
		sz = v[2] - nz * d;
		v[0] = sx - bounce * nx * d;
		v[1] = sy - ny * d * bounce;
		v[2] = sz - nz * d * bounce;
		RB_F32(e, 0x4110) = nx;
		RB_F32(e, 0x4114) = ny;
		RB_F32(e, 0x4118) = nz;
		RB_F32(e, 0x411c) = 0.0f; /* unset in the original */
		result = RB_I32(e, 0x20);
		rbb_MsgDtor(&msg);
		return result;
	}
	for (k = 0; k < RB_I32(e, 0x3d64); k++) {
		float n0 = RB_F32(e, 0x3d88 + k * 0xd0);
		float n1 = RB_F32(e, 0x3d8c + k * 0xd0);
		float n2 = RB_F32(e, 0x3d90 + k * 0xd0);
		float n3 = RB_F32(e, 0x3d94 + k * 0xd0);
		float kp, ko, vx, vy, vz;

		d = n0 * v[0] + n1 * v[1] + n2 * v[2];
		vx = n0 * d;
		vy = n1 * d;
		vz = n2 * d;
		kp = RB_F32(e, 0x4174);
		v[0] = (v[0] - vx) - vx * kp;
		v[1] = (v[1] - vy) - vy * kp;
		v[2] = (v[2] - vz) - vz * kp;
		ko = RB_F32(e, 0x4178);
		o = RB_PTR(e, 0x3d68 + k * 4);
		RB_F32(o, RB_PL_VEL) = vx * ko + RB_F32(o, RB_PL_VEL);
		RB_F32(o, RB_PL_VEL + 4) = vy * ko + RB_F32(o, RB_PL_VEL + 4);
		RB_F32(o, RB_PL_VEL + 8) = vz * ko + RB_F32(o, RB_PL_VEL + 8);
		if ((RB_I32(RB_GAME, RB_GO_VARIATION) == 1 &&
				CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN) <= 58.5f) ||
				RB_I32(RB_GAME, RB_GO_VARIATION) == 4) {
			v[0] = n0 * 3.5f + v[0];
			v[1] = n1 * 3.5f + v[1];
			v[2] = n2 * 3.5f + v[2];
			rb_ClampSpeed(v);
			RB_F32(o, RB_PL_VEL) = -n0 * 3.5f + RB_F32(o, RB_PL_VEL);
			RB_F32(o, RB_PL_VEL + 4) = -n1 * 3.5f + RB_F32(o, RB_PL_VEL + 4);
			RB_F32(o, RB_PL_VEL + 8) = -n2 * 3.5f + RB_F32(o, RB_PL_VEL + 8);
			rb_ClampSpeed(RB_FP(o, RB_PL_VEL));
		}
		msg.d[0] = (unsigned int)(uintptr_t)RB_PTR(e, 0x3d68 + k * 4);
		msg.f[1] = v[0];
		msg.f[2] = v[1];
		msg.f[3] = v[2];
		msg.f[4] = v[3];
		msg.id = 0;
		msg.prio = 2;
		msg.unique = 0;
		msg.f[5] = n0;
		msg.f[6] = n1;
		msg.f[7] = n2;
		msg.f[8] = n3;
		rbb_PostMsg(e, &msg);
	}
	result = RB_I32(e, 0x20);
	rbb_MsgDtor(&msg);
	return result;
}

/* 0x0018f860  RollerBall (gap)
The physics step of every moving entity: time in the air, forces and movement
(rollerball_b), then up to 30 rounds of collision and contact resolution until the entity
can move by its velocity. */
void CRollerBallPhysics_Step(void *self, float dt)
{
	unsigned char *e;
	int i, flags;

	for (e = RB_PTR(self, 8); e != NULL; e = RB_NEXT(e)) {
		if (RB_U8(e, RB_PL_1B) == 0)
			continue;
		if (RB_U8(e, RB_PL_18) == 0)
			RB_F32(e, 0x4198) = dt + RB_F32(e, 0x4198);
		else
			RB_F32(e, 0x4198) = 0.0f;
		RB_U8(e, RB_PL_18) = 0;
		rbb_EntityForces(e, dt);
		rbb_EntityMove(e, dt);
		RB_I32(e, 0x40e0) = RB_I32(e, RB_PL_POS);
		RB_I32(e, 0x40e4) = RB_I32(e, RB_PL_POS + 4);
		RB_I32(e, 0x40e8) = RB_I32(e, RB_PL_POS + 8);
		RB_I32(e, 0x40ec) = RB_I32(e, RB_PL_POS + 12);
		for (i = 0; i < 0x1e; i++) {
			RB_I32(e, 0x20) = 0;
			RB_I32(e, 0x40b8) = RB_I32(e, RB_PL_POS);
			RB_I32(e, 0x40bc) = RB_I32(e, RB_PL_POS + 4);
			RB_I32(e, 0x40c0) = RB_I32(e, RB_PL_POS + 8);
			RB_I32(e, 0x40c4) = RB_I32(e, RB_PL_POS + 12);
			if (CRollerBallPhysics_CollideWorld(self, e) != 0)
				RB_U32(e, 0x20) = RB_U32(e, 0x20) | 1;
			if (CRollerBallPhysics_CollideEntities(self, e) != 0)
				RB_U32(e, 0x20) = RB_U32(e, 0x20) | 2;
			if (RB_I32(e, 0x20) != 0) {
				flags = CRollerBallPhysics_Resolve(self, e);
				RB_I32(e, 0x20) = flags;
				if (flags != 0)
					continue;
			}
			rbb_EntityPreMove(e);
			RB_F32(e, RB_PL_POS) = RB_F32(e, RB_PL_POS) + RB_F32(e, RB_PL_VEL);
			RB_F32(e, RB_PL_POS + 4) = RB_F32(e, RB_PL_POS + 4) + RB_F32(e, RB_PL_VEL + 4);
			RB_F32(e, RB_PL_POS + 8) = RB_F32(e, RB_PL_POS + 8) + RB_F32(e, RB_PL_VEL + 8);
			RB_I32(e, 0x40b8) = RB_I32(e, RB_PL_POS);
			RB_I32(e, 0x40bc) = RB_I32(e, RB_PL_POS + 4);
			RB_I32(e, 0x40c0) = RB_I32(e, RB_PL_POS + 8);
			RB_I32(e, 0x40c4) = RB_I32(e, RB_PL_POS + 12);
			break;
		}
		rbb_PlayerReset(e);
	}
}

/* 0x0018f9f0  RollerBall (gap): Update: the physics step (jmp to 0x0018f860) */
void CRollerBallPhysics_Update(void *self, float dt)
{
	CRollerBallPhysics_Step(self, dt);
}

/* ======================================================================== spot effects (+0x2060) */

/* Ten spot effects of 0x88 bytes from +4 (vtable 0x00274680): +4 position, +0x14 active,
+0x15 visible, +0x18 age, +0x1c a particle emitter (rollerball_b, CRollerBall_Particle.cpp;
+0x2c / +0x30 its counters, +0x6c alive), +0x80 duration. */

/* 0x00274680  vtable of a spot effect */
const void *const vtable_00274680[1] = { (const void *)CRollerBallSpot_DeletingDtor };

/* 0x00274684  vtable of the spot effect system */
const void *const vtable_00274684[1] = { (const void *)CRollerBallSpotFX_DeletingDtor };

/* 0x0018fe10  RollerBall (gap): spot effect constructor */
void *CRollerBallSpot_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274680;
	rbb_Particle_19d120(RB_P(self, 0x1c));
	return self;
}

/* 0x0018fe30  RollerBall (gap): spot effect destructor */
void CRollerBallSpot_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274680;
	rbb_ParticlesDtor(RB_P(self, 0x1c));
}

/* 0x0018fe40  RollerBall (gap): constructor (the ten effects, array constructor iterator) */
void *CRollerBallSpotFX_Ctor(void *self)
{
	int i;

	*(const void *const **)self = vtable_00274684;
	for (i = 0; i < 10; i++)
		CRollerBallSpot_Ctor((unsigned char *)self + 4 + i * 0x88);
	return self;
}

/* 0x0018fe70  RollerBall (gap): destructor (array destructor iterator, last first) */
void CRollerBallSpotFX_Dtor(void *self)
{
	int i;

	*(const void *const **)self = vtable_00274684;
	for (i = 9; i >= 0; i--)
		CRollerBallSpot_Dtor((unsigned char *)self + 4 + i * 0x88);
}

/* 0x0018fe90  RollerBall (gap): Shutdown: every emitter's virtual +0 */
void CRollerBallSpotFX_Shutdown(void *self)
{
	unsigned char *ps;
	int i;

	for (i = 0; i < 10; i++) {
		ps = (unsigned char *)self + 0x20 + i * 0x88;
		RB_VFN(ps, 0x00, void (*)(void *))(ps);
	}
}

/* 0x0018fec0  RollerBall (gap)
Update: active effects age and update their emitter (virtual +4); a dead emitter
(+0x50 of the emitter) ends the effect. */
void CRollerBallSpotFX_Update(void *self, float dt)
{
	unsigned char *ps;
	int i;

	for (i = 0; i < 10; i++) {
		ps = (unsigned char *)self + 0x20 + i * 0x88;
		if (RB_U8(ps, -8) == 0)
			continue;
		RB_F32(ps, -4) = dt + RB_F32(ps, -4);
		RB_VFN(ps, 0x04, void (*)(void *, float))(ps, dt);
		if (RB_U8(ps, 0x50) == 0) {
			RB_U8(ps, -8) = 0;
			RB_U8(ps, -7) = 0;
		}
	}
}

/* 0x0018ff10  RollerBall (gap): Draw: visible effects (virtual +8 of the emitter) */
void CRollerBallSpotFX_Draw(void *self)
{
	unsigned char *ps;
	int i;

	for (i = 0; i < 10; i++) {
		ps = (unsigned char *)self + 0x20 + i * 0x88;
		if (RB_U8(ps, -7) != 0)
			RB_VFN(ps, 0x08, void (*)(void *))(ps);
	}
}

/* 0x0018ff40  RollerBall (gap): spot effect deleting destructor (flags bit 0: free) */
void *CRollerBallSpot_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallSpot_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018ff60  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallSpotFX_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallSpotFX_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x0018ff80  RollerBall (gap)
Initialise: every emitter gets 50 particles, the textures game +0x1ef0 / +0x1ef4, and is
reset (virtual +0xc); no effect is active. Returns 1. */
int CRollerBallSpotFX_Init(void *self, void *package)
{
	unsigned char *ps;
	int i;

	(void)package;
	for (i = 0; i < 10; i++) {
		ps = (unsigned char *)self + 0x20 + i * 0x88;
		rbb_Particle_19d150(ps, NULL, 0x32);
		CParticleSystem_SetTexture((struct CPSHeader *)ps, RB_I32(RB_GAME, 0x1ef0));
		rbb_Particle_19d110(ps, RB_I32(RB_GAME, 0x1ef4));
		RB_U8(ps, -8) = 0;
		RB_U8(ps, -7) = 0;
		RB_F32(ps, -4) = 0.0f;
		RB_VFN(ps, 0x0c, void (*)(void *))(ps);
	}
	return 1;
}

/* 0x0018fff0  RollerBall (gap)
Starts a spot effect at POS moving along DIR for DURATION (x 50 particles): a free effect
(or one whose emitter has room), else the oldest. */
void CRollerBallSpotFX_Spawn(void *self, const float *pos, const float *dir, float duration)
{
	unsigned char *e = NULL, *c;
	float oldest = 0.0f;
	int n, i;

	n = RB_FTOL(duration * 50.0f);
	for (i = 0; i < 10; i++) {
		c = (unsigned char *)self + 4 + i * 0x88;
		if (RB_U8(c, 0x14) == 0 || n < RB_I32(c, 0x2c) - RB_I32(c, 0x30)) {
			e = c;
			break;
		}
		if (oldest <= RB_F32(c, 0x18)) {
			e = c;
			oldest = RB_F32(c, 0x18);
		}
	}
	RB_F32(e, 4) = pos[0];
	RB_F32(e, 8) = pos[1];
	RB_F32(e, 0xc) = pos[2];
	RB_F32(e, 0x10) = pos[3];
	CParticleSystem_SetPosition((struct CPSHeader *)RB_P(e, 0x1c), (float *)pos);
	RbParticle_SetDirection(RB_P(e, 0x1c), dir);
	RB_F32(e, 0x80) = duration;
	rbb_Particle_19d0c0(RB_P(e, 0x1c), n);
	RB_U8(e, 0x6c) = 1;
	RB_U8(e, 0x14) = 1;
	RB_U8(e, 0x15) = 1;
	RB_F32(e, 0x18) = 0.0f;
}

/* ======================================================================== HUD (+0x201c) */

/* +4 an overlay list (classlib_ui), +0x10 the players' scores (4 ints). */

/* 0x00274688  vtable of the HUD system */
const void *const vtable_00274688[1] = { (const void *)CRollerBallHud_DeletingDtor };

/* 0x001900b0  RollerBall (gap): constructor */
void *CRollerBallHud_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274688;
	CLList_Init((int *)RB_P(self, 4));
	return self;
}

/* 0x001900d0  RollerBall (gap): destructor (the list's destructor is empty) */
void CRollerBallHud_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274688;
	FUN_001d5950();
}

/* 0x001900e0  RollerBall (gap): Draw: the overlays (with z) */
void CRollerBallHud_Draw(void *self)
{
	(void)self;
	if (RB_I32(RB_GAME, 0x13b0) == 2)
		FUN_001d5950();
	COverlay_DrawAll(1);
}

/* 0x00190100  RollerBall (gap): the race laps on the HUD bars (3, a finished player 4) */
void CRollerBallHud_SetLaps(void *self)
{
	unsigned char *p;
	int i = 0;

	(void)self;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), i++)
		CLHud_SetFraction(DAT_002ecfac, i,
			RB_I32(p, RB_PL_41F0) < 3 ? RB_I32(p, RB_PL_SCORE) : 4, 3);
}

/* 0x00190140  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallHud_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallHud_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x00190160  RollerBall (gap)
Begin: the overlays, projector, score boxes and bars, zero scores, debris (1000), the
referee bot, then by variation the score texts and bar positions (classlib_ui's
DAT_002ecf90 / DAT_00290be4 tables) and the clock (60 s, 120 s for the races; none for
the drop variation). */
void CRollerBallHud_Begin(void *self)
{
	float *x = (float *)DAT_002ecf90;
	float clock = 0.0f;

	FUN_00072720((undefined4)(uintptr_t)RB_P(self, 4));
	CProjector_Reset();
	CLHud_CreateScoreBoxes(DAT_002ecfac);
	CLHud_CreateBars(DAT_002ecfac);
	RB_I32(DAT_002ecfac, 0x3c) = 0;
	RB_F32(DAT_002ecfac, 0x40) = 1.0f;
	RB_I32(self, 0x10) = 0;
	RB_I32(self, 0x14) = 0;
	RB_I32(self, 0x18) = 0;
	RB_I32(self, 0x1c) = 0;
	Debris_Init(NULL, 1000);
	RefBot_Create();
	switch (RB_I32(RB_GAME, RB_GO_VARIATION)) {
	case 0:
		clock = 60.0f;
		CLHud_CreateScoreTexts(DAT_002ecfac, (int *)RB_P(self, 0x10));
		x[0] = 15.0f;
		DAT_00290be4[0] = 40.0f;
		x[1] = 0.0f;
		DAT_00290be4[1] = 40.0f;
		x[2] = 5.0f;
		DAT_00290be4[2] = 35.0f;
		x[3] = -10.0f;
		DAT_00290be4[3] = 40.0f;
		break;
	case 1:
		clock = 60.0f;
		x[0] = 5.0f;
		DAT_00290be4[0] = 40.0f;
		x[1] = -13.0f;
		DAT_00290be4[1] = 40.0f;
		x[2] = -5.0f;
		DAT_00290be4[2] = 40.0f;
		x[3] = 13.0f;
		DAT_00290be4[3] = 40.0f;
		break;
	case 2:
		clock = 120.0f;
		CLHud_CreateScoreTexts(DAT_002ecfac, (int *)RB_P(self, 0x10));
		break;
	case 3:
		clock = 120.0f;
		CLHud_CreateScoreLabels(DAT_002ecfac, (char *)"", 1);
		break;
	case 4:
		clock = 60.0f;
		CLHud_CreateScoreTexts(DAT_002ecfac, (int *)RB_P(self, 0x10));
		break;
	}
	if (RB_I32(RB_GAME, RB_GO_VARIATION) == 4)
		CLHud_CreateClock(DAT_002ecfac, -1.0f);
	else
		CLHud_CreateClock(DAT_002ecfac, clock);
}

/* 0x00190330  RollerBall (gap)
Update: the scores (race: the laps), the progress bars (HUD +0x4c..), the clock (none for
the drop variation), the bars, the referee bot, the overlays and the projector. */
void CRollerBallHud_Update(void *self, float dt)
{
	unsigned char *p;
	int *score;
	int off;

	(void)dt;
	if (RB_I32(RB_GAME, RB_GO_VARIATION) == 3) {
		CRollerBallHud_SetLaps(self);
	} else {
		score = (int *)RB_P(self, 0x10);
		for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), score++)
			*score = RB_I32(p, RB_PL_SCORE);
		CLHud_SetScores(DAT_002ecfac, (int *)RB_P(self, 0x10));
	}
	off = 0x4c;
	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p), off += 4)
		RB_F32(DAT_002ecfac, off) = RB_F32(p, 0x41ec);
	if (RB_I32(RB_GAME, RB_GO_VARIATION) == 4)
		CLHud_SetClock(DAT_002ecfac, -1.0f, 1);
	else
		CLHud_SetClock(DAT_002ecfac, CRollerBallTimer_Remaining(RB_GAME + RB_GO_COUNTDOWN), 1);
	CLHud_UpdateBars(DAT_002ecfac);
	RefBot_Update();
	COverlay_UpdateAll();
	CProjector_UpdateAll();
}

/* ======================================================================== texture animations (+0x1ee0) */

/* +4.. 7 textures (spotfx\sprites\..., 0x002b5068), +0x20 ten animations of 0x1c bytes
(vtable 0x00274708: +4 texture, +8 first frame, +0xa last, +0xc frames, +0xe frame,
+0x10 flags (1 loop, 2 ping-pong), +0x12 running, +0x13 backwards, +0x14 time, +0x18 time
per frame), +0x138 their count. */

/* 0x00274708  vtable of a texture animation */
const void *const vtable_00274708[1] = { (const void *)CRollerBallTexAnim_DeletingDtor };

/* 0x0027470c  vtable of the texture animation system */
const void *const vtable_0027470c[1] = { (const void *)CRollerBallTextures_DeletingDtor };

/* 0x00190400  RollerBall (missing from the table: the array constructor callback of
0x001904f0): texture animation constructor */
void FUN_00190400(void *self)
{
	*(const void *const **)self = vtable_00274708;
}

/* 0x00190410  RollerBall (missing from the table: the array destructor callback):
texture animation destructor */
void FUN_00190410(void *self)
{
	*(const void *const **)self = vtable_00274708;
}

/* 0x00190420  RollerBall (gap)
Update of an animation: every PERIOD the next frame (once: stops at the last; loop: back
to the first; ping-pong: turns at both ends); the texture shows the frame. */
void CRollerBallTexAnim_Update(void *self, float dt)
{
	float t = dt + RB_F32(self, 0x14);

	RB_F32(self, 0x14) = t;
	if (!(RB_F32(self, 0x18) < t))
		return;
	while (RB_F32(self, 0x18) < RB_F32(self, 0x14))
		RB_F32(self, 0x14) = RB_F32(self, 0x14) - RB_F32(self, 0x18);
	if ((RB_U16(self, 0x10) & 1) == 0) {
		RB_I16(self, 0xe) = (short)(RB_I16(self, 0xe) + 1);
		if (RB_I16(self, 0xa) <= RB_I16(self, 0xe))
			RB_U8(self, 0x12) = 0;
	} else if ((RB_U16(self, 0x10) & 2) == 0) {
		RB_I16(self, 0xe) = (short)(RB_I16(self, 0xe) + 1);
		if (RB_I16(self, 0xa) <= RB_I16(self, 0xe))
			RB_I16(self, 0xe) = RB_I16(self, 8);
	} else if (RB_U8(self, 0x13) == 0) {
		RB_I16(self, 0xe) = (short)(RB_I16(self, 0xe) + 1);
		if (RB_I16(self, 0xa) <= RB_I16(self, 0xe)) {
			RB_I16(self, 0xe) = (short)(RB_I16(self, 0xe) - 1);
			RB_U8(self, 0x13) = 1;
		}
	} else {
		RB_I16(self, 0xe) = (short)(RB_I16(self, 0xe) - 1);
		if (RB_I16(self, 0xe) < RB_I16(self, 8)) {
			RB_I16(self, 0xe) = (short)(RB_I16(self, 0xe) + 1);
			RB_U8(self, 0x13) = 0;
		}
	}
	bdTextureSetFrame(RB_PTR(self, 4), RB_I16(self, 0xe));
}

/* 0x001904f0  RollerBall (gap): constructor (the ten animations) */
void *CRollerBallTextures_Ctor(void *self)
{
	int i;

	*(const void *const **)self = vtable_0027470c;
	for (i = 0; i < 10; i++)
		FUN_00190400((unsigned char *)self + 0x20 + i * 0x1c);
	return self;
}

/* 0x00190520  RollerBall (gap): destructor (the ten animations, last first) */
void CRollerBallTextures_Dtor(void *self)
{
	int i;

	*(const void *const **)self = vtable_0027470c;
	for (i = 9; i >= 0; i--)
		FUN_00190410((unsigned char *)self + 0x20 + i * 0x1c);
}

/* 0x00190540  RollerBall (gap)
Initialise: no animations; the 7 sprite textures (0x002b5068). Returns 0 when one is
missing. */
int CRollerBallTextures_Init(void *self, void *package)
{
	const char *const *name = (const char *const *)RB_ADDR(0x002b5068u);
	struct bkResource **slot = (struct bkResource **)RB_P(self, 4);
	int i;

	(void)package;
	RB_I32(self, 0x138) = 0;
	for (i = 0; i < 7; i++, name++, slot++) {
		*slot = bLoadResource(NULL, *name, 0, 0xdefa);
		if (*slot == NULL)
			return 0;
	}
	return 1;
}

/* 0x00190590  RollerBall (gap): Update: every running animation */
void CRollerBallTextures_Update(void *self, float dt)
{
	int i;

	for (i = 0; i < RB_I32(self, 0x138); i++) {
		if (RB_U8(self, 0x20 + i * 0x1c + 0x12) != 0)
			CRollerBallTexAnim_Update((unsigned char *)self + 0x20 + i * 0x1c, dt);
	}
}

/* 0x001905e0  RollerBall (gap): texture animation deleting destructor */
void *CRollerBallTexAnim_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00274708;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x00190600  RollerBall (gap): deleting destructor (flags bit 0: free) */
void *CRollerBallTextures_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallTextures_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}
