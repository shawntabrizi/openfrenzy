/*
Demolition/unplaced_effects.c  <-  C:\BlitzParty\Game\Src\Demolition\ (file not known)

Three subsystems of the Demolition game object between CDemolition_Light.cpp
and CDemolition_Token.cpp (0x000ee4d0-0x000eec50, gap functions without a
known file):
- the textures (+0x6c50): the 20 sprite and brick textures;
- the spot effects (+0x1510): 10 spark bursts and 12 smoke/steam puffs, each a
  particle system of demolition_c with an active/visible flag and an age;
- the HUD overlay (+0x14e4): the score boxes, the bars, the clock and the
  projectors and referee of ClassLib.
*/

#include "demolition_a.h"

/* demolition_c's particle systems used by the spot effects */
#define de_BurstCtor(ps) ((void (*)(void *))FUN_001029c0)(ps)
#define de_BurstDtor(ps) ((void (*)(void *))FUN_001029d0)(ps)
#define de_PuffCtor(ps) ((void (*)(void *))FUN_00102f90)(ps)
#define de_PuffDtor(ps) ((void (*)(void *))FUN_00102fa0)(ps)
#define de_BurstInit(ps, a, b) ((int (*)(void *, void *, void *))FUN_00103f60)((ps), (a), (b))
#define de_PuffInit(ps, a, b) ((int (*)(void *, void *, void *))FUN_00104ef0)((ps), (a), (b))
#define de_PuffSetTexture(ps, tex) ((void (*)(void *, void *))FUN_001031b0)((ps), (tex))
#define de_PuffReset(ps) ((void (*)(void *))FUN_00103110)(ps)
#define de_BurstSetDirection(ps, dir) ((void (*)(void *, float *))FUN_00104730)((ps), (dir))
#define de_BurstStart(ps) ((void (*)(void *))FUN_00104700)(ps)

/* a burst (0x7e0 bytes) or a puff (0xb4 bytes) of the spot effects */
#define SB_ACTIVE  0x20 /* burst: char */
#define SB_VISIBLE 0x21 /* burst: char */
#define SB_AGE     0x24 /* burst: float */
#define SB_PS      0x30 /* burst: the particle system */
#define SP_ACTIVE  0x04 /* puff: char */
#define SP_VISIBLE 0x05 /* puff: char */
#define SP_AGE     0x08 /* puff: float */
#define SP_PS      0x0c /* puff: the particle system */

#define DE_BURST(self, i) ((char *)(self) + DA_SPOTFX_BURSTS_AT + (i) * DA_SPOTFX_BURST_SIZE)
#define DE_PUFF(self, i) ((char *)(self) + DA_SPOTFX_PUFFS_AT + (i) * DA_SPOTFX_PUFF_SIZE)

/* 0x0026ab10: the textures */
static const void *const vtable_0026ab10[] = {
	(const void *)CDemoTextureSystem_DeletingDtor, /* [0] */
};

/* 0x0026ab14: a burst */
static const void *const vtable_0026ab14[] = {
	(const void *)CDemoSpotBurst_DeletingDtor, /* [0] */
};

/* 0x0026ab18: a puff */
static const void *const vtable_0026ab18[] = {
	(const void *)CDemoSpotPuff_DeletingDtor, /* [0] */
};

/* 0x0026ab1c: the spot effects */
static const void *const vtable_0026ab1c[] = {
	(const void *)CDemoSpotFX_DeletingDtor, /* [0] */
};

/* 0x0026ab30: the overlay */
static const void *const vtable_0026ab30[] = {
	(const void *)CDemoOverlay_DeletingDtor, /* [0] */
};

/* ---------- the textures (game +0x6c50) */

/* 0x000ee4d0 */
void *CDemoTextureSystem_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026ab10;
	return self;
}

/* 0x000ee4e0 */
void CDemoTextureSystem_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026ab10;
}

/* 0x000ee4f0
Init: loads the 20 textures of DAT_00297820 (from any package; PACKAGE is not used). Always
succeeds. */
int CDemoTextureSystem_Init(void *self, void *package)
{
	struct CDemolition_TextureSystem *ts = (struct CDemolition_TextureSystem *)self;
	int i;

	(void)package;
	for (i = 0; i < DA_TEXTURE_COUNT; i++)
		ts->textures[i] = DA_LoadResource(NULL, DAT_00297820[i], 0, 0xdefa);
	return 1;
}

/* 0x000ee530 */
void *CDemoTextureSystem_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026ab10;
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* ---------- the spot effects (game +0x1510) */

/* 0x000ee550  (an element of the burst array) */
void *CDemoSpotBurst_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026ab14;
	de_BurstCtor(DA_AT(self, SB_PS));
	return self;
}

/* 0x000ee570 */
void CDemoSpotBurst_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026ab14;
	de_BurstDtor(DA_AT(self, SB_PS));
}

/* 0x000ee580  (an element of the puff array) */
void *CDemoSpotPuff_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026ab18;
	de_PuffCtor(DA_AT(self, SP_PS));
	return self;
}

/* 0x000ee5a0 */
void CDemoSpotPuff_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026ab18;
	de_PuffDtor(DA_AT(self, SP_PS));
}

/* 0x000ee5b0
Constructs the 10 bursts and the 12 puffs (the original uses the C++ vector constructor
iterator). */
void *CDemoSpotFX_Ctor(void *self)
{
	int i;

	*(const void *const **)self = vtable_0026ab1c;
	for (i = 0; i < DA_SPOTFX_BURSTS; i++)
		CDemoSpotBurst_Ctor(DE_BURST(self, i));
	for (i = 0; i < DA_SPOTFX_PUFFS; i++)
		CDemoSpotPuff_Ctor(DE_PUFF(self, i));
	return self;
}

/* 0x000ee630
Destroys the puffs, then the bursts, each array from its last element (the C++ vector
destructor iterator). */
void CDemoSpotFX_Dtor(void *self)
{
	int i;

	*(const void *const **)self = vtable_0026ab1c;
	for (i = DA_SPOTFX_PUFFS - 1; i >= 0; i--)
		CDemoSpotPuff_Dtor(DE_PUFF(self, i));
	for (i = DA_SPOTFX_BURSTS - 1; i >= 0; i--)
		CDemoSpotBurst_Dtor(DE_BURST(self, i));
}

/* 0x000ee6a0
Shutdown: slot 0 of every particle system. */
void CDemoSpotFX_Shutdown(void *self)
{
	char *ps;
	int i;

	for (i = 0; i < DA_SPOTFX_BURSTS; i++)
	{
		ps = DE_BURST(self, i) + SB_PS;
		DA_VFN(ps, 0x00, void (*)(void *))(ps);
	}
	for (i = 0; i < DA_SPOTFX_PUFFS; i++)
	{
		ps = DE_PUFF(self, i) + SP_PS;
		DA_VFN(ps, 0x00, void (*)(void *))(ps);
	}
}

/* 0x000ee6f0
Update: every active effect ages and updates (slot 1); one whose system stopped (+0x3c of a
burst system, +0x64 of a puff system) becomes inactive and invisible. */
void CDemoSpotFX_Update(void *self, float dt)
{
	char *e, *ps;
	int i;

	for (i = 0; i < DA_SPOTFX_BURSTS; i++)
	{
		e = DE_BURST(self, i);
		ps = e + SB_PS;
		if (DA_FIELD(char, e, SB_ACTIVE) == 0)
			continue;
		DA_FIELD(float, e, SB_AGE) = dt + DA_FIELD(float, e, SB_AGE);
		DA_VFN(ps, 0x04, void (*)(void *, float))(ps, dt);
		if (DA_FIELD(char, ps, 0x3c) == 0)
		{
			DA_FIELD(char, e, SB_ACTIVE) = 0;
			DA_FIELD(char, e, SB_VISIBLE) = 0;
		}
	}
	for (i = 0; i < DA_SPOTFX_PUFFS; i++)
	{
		e = DE_PUFF(self, i);
		ps = e + SP_PS;
		if (DA_FIELD(char, e, SP_ACTIVE) == 0)
			continue;
		DA_FIELD(float, e, SP_AGE) = dt + DA_FIELD(float, e, SP_AGE);
		DA_VFN(ps, 0x04, void (*)(void *, float))(ps, dt);
		if (DA_FIELD(char, ps, 0x64) == 0)
		{
			DA_FIELD(char, e, SP_ACTIVE) = 0;
			DA_FIELD(char, e, SP_VISIBLE) = 0;
		}
	}
}

/* 0x000ee780
Draws the visible bursts (slot 2). */
void CDemoSpotFX_DrawBursts(void *self)
{
	char *ps;
	int i;

	for (i = 0; i < DA_SPOTFX_BURSTS; i++)
	{
		if (DA_FIELD(char, DE_BURST(self, i), SB_VISIBLE) == 0)
			continue;
		ps = DE_BURST(self, i) + SB_PS;
		DA_VFN(ps, 0x08, void (*)(void *))(ps);
	}
}

/* 0x000ee7b0
Draws the visible puffs (slot 2), not after the round is over. */
void CDemoSpotFX_DrawPuffs(void *self)
{
	char *ps;
	int i;

	if (DA_FIELD(char, DA_GAME, DG_ROUNDOVER) != 0)
		return;
	for (i = 0; i < DA_SPOTFX_PUFFS; i++)
	{
		if (DA_FIELD(char, DE_PUFF(self, i), SP_VISIBLE) == 0)
			continue;
		ps = DE_PUFF(self, i) + SP_PS;
		DA_VFN(ps, 0x08, void (*)(void *))(ps);
	}
}

/* 0x000ee7f0
Takes a free puff: active, visible, age 0, its system reset (slot 3). Returns its particle
system. With no free puff the original prints "Using oldest" and then writes through a null
pointer (it never finds an oldest one). Called by demolition_b. */
void *CDemoSpotFX_GetPuff(void *self)
{
	char *puff = NULL;
	char *ps;
	int i;

	for (i = 0; i < DA_SPOTFX_PUFFS; i++)
	{
		if (DA_FIELD(char, DE_PUFF(self, i), SP_ACTIVE) == 0)
		{
			puff = DE_PUFF(self, i);
			break;
		}
	}
	if (puff == NULL)
		DA_Printf("Using oldest\n");
	DA_FIELD(char, puff, SP_ACTIVE) = 1;
	DA_FIELD(char, puff, SP_VISIBLE) = 1;
	DA_FIELD(float, puff, SP_AGE) = 0.0f;
	ps = puff + SP_PS;
	DA_VFN(ps, 0x0c, void (*)(void *))(ps);
	return ps;
}

/* 0x000ee850 */
void *CDemoSpotBurst_DeletingDtor(void *self, unsigned int flags)
{
	CDemoSpotBurst_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000ee870 */
void *CDemoSpotPuff_DeletingDtor(void *self, unsigned int flags)
{
	CDemoSpotPuff_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000ee890 */
void *CDemoSpotFX_DeletingDtor(void *self, unsigned int flags)
{
	CDemoSpotFX_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000ee8b0
Init: every burst and puff system is initialised and reset, inactive; the puffs use the
spark texture. Always succeeds. */
int CDemoSpotFX_Init(void *self, void *package)
{
	char *e, *ps;
	int i;

	(void)package;
	for (i = 0; i < DA_SPOTFX_BURSTS; i++)
	{
		e = DE_BURST(self, i);
		ps = e + SB_PS;
		de_BurstInit(ps, NULL, NULL);
		DA_FIELD(char, e, SB_ACTIVE) = 0;
		DA_FIELD(char, e, SB_VISIBLE) = 0;
		DA_FIELD(float, e, SB_AGE) = 0.0f;
		DA_VFN(ps, 0x0c, void (*)(void *))(ps);
	}
	for (i = 0; i < DA_SPOTFX_PUFFS; i++)
	{
		e = DE_PUFF(self, i);
		ps = e + SP_PS;
		de_PuffInit(ps, NULL, NULL);
		de_PuffSetTexture(ps, DA_FIELD(void *, DA_GAME, DG_TEXTURES + 4));
		de_PuffReset(ps);
		DA_FIELD(char, e, SP_ACTIVE) = 0;
		DA_FIELD(char, e, SP_VISIBLE) = 0;
		DA_FIELD(float, e, SP_AGE) = 0.0f;
		DA_VFN(ps, 0x0c, void (*)(void *))(ps);
	}
	return 1;
}

/* 0x000ee940
Starts a burst at POS towards DIR: a free burst, else the oldest. Called by demolition_d. */
void CDemoSpotFX_StartBurst(void *self, float *pos, float *dir)
{
	char *burst = NULL;
	float oldest = 0.0f;
	char *ps;
	int i;

	for (i = 0; i < DA_SPOTFX_BURSTS; i++)
	{
		char *e = DE_BURST(self, i);

		if (DA_FIELD(char, e, SB_ACTIVE) == 0)
		{
			burst = e;
			break;
		}
		if (oldest <= DA_FIELD(float, e, SB_AGE))
		{
			burst = e;
			oldest = DA_FIELD(float, e, SB_AGE);
		}
	}
	memcpy(DA_AT(burst, 0x10), pos, 16);
	ps = burst + SB_PS;
	DA_PSSetPosition(ps, pos);
	de_BurstSetDirection(ps, dir);
	de_BurstStart(ps);
	DA_FIELD(char, burst, 0x6c) = 1;
	DA_FIELD(char, burst, SB_ACTIVE) = 1;
	DA_FIELD(char, burst, SB_VISIBLE) = 1;
	DA_FIELD(float, burst, SB_AGE) = 0.0f;
}

/* ---------- the HUD overlay (game +0x14e4) */

/* 0x000ee9e0 */
void *CDemoOverlay_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026ab30;
	DA_ListInit(DA_AT(self, 4));
	return self;
}

/* 0x000eea00 */
void CDemoOverlay_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026ab30;
	DA_Nop(DA_AT(self, 4));
}

/* 0x000eea10
Shutdown: the referee, the overlays and the projectors go; the debris starts over. */
void CDemoOverlay_Shutdown(void *self)
{
	(void)self;
	DA_RefBotDelete();
	DA_OverlayDeleteAll();
	DA_ProjectorDeleteAll();
	DA_DebrisStart();
}

/* 0x000eea30
Draw: the overlays; in frag and cross the four bars show the players' spectator timers
(players not spectating: 0), then the HUD states are set back. */
void CDemoOverlay_Draw(void *self)
{
	float levels[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	char *p;
	int variation, i;

	(void)self;
	if (DA_FIELD(int, DA_GAME, DG_CAMERAMODE) == 2)
		DA_Nop(NULL);
	DA_OverlayDrawAll(1);
	variation = DA_FIELD(int, DA_GAME, DG_VARIATION);
	if (variation != 0 && variation != 3)
		return;
	i = 0;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p), i++)
	{
		if (DA_FIELD(char, p, DP_SPECTATE) != 0)
			levels[i] = DA_TimerFraction(DA_AT(p, 0x11d0));
		else
			levels[i] = 0.0f;
	}
	DA_HudSetBarLevels(levels[0], levels[1], levels[2], levels[3], 1.0f);
	DA_SetProjectionMode(1);
	DA_SetRenderState(5, 1, 0);
	DA_SetRenderState(7, 3, 0);
}

/* 0x000eeaf0 */
void *CDemoOverlay_DeletingDtor(void *self, unsigned int flags)
{
	CDemoOverlay_Dtor(self);
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000eeb10
Reset: the HUD, the projectors, the score boxes and bars (scores 0, range 0-1), the debris
(1000 pieces), the referee; the bar geometry for frag (60 s) and cross; the clock: 60 s for
frag, remote and mine; cross has no clock (-1) and all four bars at 75. */
void CDemoOverlay_Reset(void *self)
{
	struct CDemolition_Overlay *o = (struct CDemolition_Overlay *)self;
	float seconds = 0.0f;
	int i;

	DA_HudInit(o->hud);
	DA_ProjectorReset();
	DA_HudCreateScoreBoxes();
	DA_HudCreateBars();
	DA_FIELD(float, DAT_002ecfac, 0x3c) = 0.0f;
	DA_FIELD(float, DAT_002ecfac, 0x40) = 1.0f;
	o->scores[0] = 0;
	o->scores[1] = 0;
	o->scores[2] = 0;
	o->scores[3] = 0;
	DA_HudCreateScoreTexts(o->scores);
	DA_DebrisInit(NULL, 1000);
	DA_RefBotCreate();
	switch (DA_FIELD(int, DA_GAME, DG_VARIATION))
	{
	case 0:
		seconds = 60.0f;
		DA_HudInitBarGeometry();
		break;
	case 1:
	case 2:
		seconds = 60.0f;
		break;
	case 3:
		seconds = 90.0f;
		DA_HudInitBarGeometry();
		break;
	default:
		break;
	}
	if (DA_FIELD(int, DA_GAME, DG_VARIATION) == 3)
	{
		for (i = 0; i < 4; i++)
		{
			DA_FIELD(float, DAT_002ecfac, 0xa8 + i * 4) = 0.0f;
			DAT_00290be4[i] = 75.0f;
		}
		DA_HudCreateClock(-1.0f);
		return;
	}
	DA_HudCreateClock(seconds);
}

/* 0x000eec50
Update: the scores and their range go to the HUD, the clock shows the countdown (cross: no
clock), the bars update; frag and cross count the HUD's seconds; the referee, the overlays and
the projectors update. */
void CDemoOverlay_Update(void *self, float dt)
{
	struct CDemolition_Overlay *o = (struct CDemolition_Overlay *)self;
	char *p;
	int high = -99999, low = 99999, score, i;

	if (DA_TimerGet(DA_GAME + DG_COUNTDOWN) <= 10.0f)
		DA_Nop(DAT_002ecfac);
	i = 0;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
		o->scores[i++] = DA_FIELD(int, p, DP_SCORE);
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
	{
		score = DA_FIELD(int, p, DP_SCORE);
		if (score >= high)
			high = score;
		if (score <= low)
			low = score;
	}
	DA_FIELD(float, DAT_002ecfac, 0x40) = (float)high;
	DA_FIELD(float, DAT_002ecfac, 0x3c) = (float)low;
	i = 0;
	for (p = DA_PLAYER_FIRST(); p != NULL; p = DA_PLAYER_NEXT(p))
		DA_FIELD(float, DAT_002ecfac, 0x4c + 4 * i++) = DA_FIELD(float, p, DP_SHOWNSCORE);
	DA_HudSetScores(o->scores);
	if (DA_FIELD(int, DA_GAME, DG_VARIATION) == 3)
		DA_HudSetClock(-1.0f, 1);
	else
		DA_HudSetClock(DA_TimerGet(DA_GAME + DG_COUNTDOWN), 1);
	DA_HudUpdateBars();
	if (DA_FIELD(int, DA_GAME, DG_VARIATION) == 0 || DA_FIELD(int, DA_GAME, DG_VARIATION) == 3)
		DA_HudAddSeconds(dt);
	DA_RefBotUpdate();
	DA_OverlayUpdateAll();
	DA_ProjectorUpdateAll();
}
