/*
Demolition/CDemolition_GasJets.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_GasJets.cpp

Variation 2 of Demolition: 23 gas jets in the floor, in 4 rows from the edges to a centre jet
(the centre jet ends every row). At random times (2-6 s, as 200-400 frames) a wave of fire runs
along the rows, either from the outside in or from the centre out, led by a light per row.
A burning jet takes a quarter of the health of a player standing in it (then 100 frames of
safety) and sends the player a hit message.

CDemolition_GasJets (vtable 0x0026b750, 0x288 bytes) is at +0x18 of the variation object.
0x000fe930-0x000feac0 are tagged with CDemolition_GasJets.cpp; the gap functions around them
use the same objects (file inferred).
*/

#include "demolition_c_internal.h"
#include "demolition_c_particle.h"

/* 0x002989c8  the jet positions */
static const float DAT_002989c8[23][4] = {
	{ 1500.03003f, 192.330002f, 1000.09003f, 0.0f },
	{ 1500.03003f, 192.330002f, 800.090027f, 0.0f },
	{ 1500.03003f, 192.330002f, 600.090027f, 0.0f },
	{ 1500.03003f, 192.330002f, 400.089996f, 0.0f },
	{ 1500.03003f, 192.330002f, 200.089996f, 0.0f },
	{ 2700.03003f, 192.330002f, 0.0900000036f, 0.0f },
	{ 2500.03003f, 192.330002f, 0.0900000036f, 0.0f },
	{ 2300.03003f, 192.330002f, 0.0900000036f, 0.0f },
	{ 2100.03003f, 192.330002f, 0.0900000036f, 0.0f },
	{ 1900.03003f, 192.330002f, 0.0900000036f, 0.0f },
	{ 1700.03003f, 192.330002f, 0.0900000036f, 0.0f },
	{ 1500.03003f, 192.330002f, -999.909973f, 0.0f },
	{ 1500.03003f, 192.330002f, -799.909973f, 0.0f },
	{ 1500.03003f, 192.330002f, -599.909973f, 0.0f },
	{ 1500.03003f, 192.330002f, -399.910004f, 0.0f },
	{ 1500.03003f, 192.330002f, -199.910004f, 0.0f },
	{ 300.029999f, 192.330002f, 0.0900000036f, 0.0f },
	{ 500.029999f, 192.330002f, 0.0900000036f, 0.0f },
	{ 700.030029f, 192.330002f, 0.0900000036f, 0.0f },
	{ 900.030029f, 192.330002f, 0.0900000036f, 0.0f },
	{ 1100.03003f, 192.330002f, 0.0900000036f, 0.0f },
	{ 1300.03003f, 192.330002f, 0.0900000036f, 0.0f },
	{ 1500.0f, 192.330002f, 0.0900000036f, 0.0f },
};

/* the vtables (each holds the deleting destructor only) */
extern const void *const vtable_0026b748[1];
extern const void *const vtable_0026b74c[1];
extern const void *const vtable_0026b750[1];

/* ---------- helpers */

/* the jet's gas system and its virtual calls (classlib layout) */
#define DC_JET_GAS(jet) ((DcGasSystem *)(jet)->gas)
#define DC_PS_CALL(obj, slot) (((void (*)(void *))(*(void *const *const *)(obj))[slot])(obj))

/* CDemolition_Light (demolition_a) virtual calls */
#define DC_LIGHT_DELETE(l, f) (((void (*)(void *, int))(*(void *const *const *)(l))[0])((l), (f)))
#define DC_LIGHT_INIT(l) (((void (*)(void *))(*(void *const *const *)(l))[1])(l))
#define DC_LIGHT_RELEASE(l) (((void (*)(void *))(*(void *const *const *)(l))[2])(l))
#define DC_LIGHT_SET_ON(l, on) (((void (*)(void *, int))(*(void *const *const *)(l))[3])((l), (on)))
#define DC_LIGHT_UPDATE(l, dt) (((void (*)(void *, float))(*(void *const *const *)(l))[4])((l), (dt)))

/* 0x0018fa30 (rollerball_a)  a light's range; the table prototype has an undefined4 for the float */
static inline void dc_light_set_range(void *light, float range)
{
	DC_DUAL(FUN_0018fa30, void (*)(int, undefined4), void (*)(int, float),
		((int)light, dc_fbits(range)), ((int)light, range));
}

/* 0x000ebff0 (demolition_a)  floating text "-%d" at `pos` (as demolition_b's db_popup_int) */
#define DC_POPUP_TABLE void (*)(int, float *, undefined4, undefined4, undefined4, undefined4, undefined4)
#define DC_POPUP_REAL void (*)(void *, float *, int, int, int, int, const char *, ...)
static inline void dc_popup_int(float *pos, int r, int g, int b, const char *format, int value)
{
	DC_DUAL(FUN_000ebff0, DC_POPUP_TABLE, DC_POPUP_REAL,
		((int)(DC_GAME + 0x7900), pos, r, g, b, 0x7f, (undefined4)format),
		(DC_GAME + 0x7900, pos, r, g, b, 0x7f, format, value));
}

/* the hit message of demolition_d (as demolition_b's struct DbMessage) */
struct DcMessage {
	void *vtable;       /* 0x00 */
	short type;         /* 0x04 */
	short pad06;
	int kind;           /* 0x08 */
	int mode;           /* 0x0c */
	float pos[4];       /* 0x10 */
	float value;        /* 0x20 */
};
OF_ASSERT_SIZE(struct DcMessage, 0x24);

/* lights a jet up (inlined in the spreading functions) */
static void dc_jet_ignite(struct DcGasJet *jet)
{
	if (jet->firing != 0)
		return;
	jet->firing = 1;
	DC_JET_GAS(jet)->p.active = 1;
	DC_JET_GAS(jet)->p.emitting = 1;
	jet->flameOn = 1;
	jet->flameDraw = 1;
	jet->fireTime = 30.0f;
}

/* starts a row's light at a jet, fading in (inlined in 0x000fe620 and 0x000fe770) */
static void dc_light_start(struct DcJetLight *l, struct DcGasJet *jet)
{
	float pos[4];

	pos[0] = jet->pos[0];
	pos[1] = jet->pos[1] + 200.0f; /* 0x002536f0 */
	pos[2] = jet->pos[2];
	pos[3] = 1.0f; /* not set by the original; SetPosition overwrites it with 1 */
	l->mode = 1;
	FUN_000edbc0((int)l->light, (undefined4 *)pos);
	DC_LIGHT_SET_ON(l->light, 1);
	FUN_0018ea10((int)l->timer, 0.100000001f);
	FUN_0018ea30((int)l->timer);
}

/* moves the rows' lights between the first and the last jet (inlined in 0x000fedf0 and
   0x000ff050) */
static void dc_lights_follow(struct CDemolition_GasJets *self, float f)
{
	struct DcGasJet *first;
	struct DcGasJet *last;
	float pos[4];
	int g;

	for (g = 0; g < 4; g++)
	{
		first = self->groups[g].jets[0];
		last = self->groups[g].jets[self->groups[g].count - 1];
		pos[3] = first->pos[3];
		pos[0] = (last->pos[0] - first->pos[0]) * f + first->pos[0];
		pos[2] = (last->pos[2] - first->pos[2]) * f + first->pos[2];
		pos[1] = (last->pos[1] - first->pos[1]) * f + first->pos[1] + 200.0f;
		FUN_000edbc0((int)self->lights[g].light, (undefined4 *)pos);
	}
}

/* the end of a wave: the lights fade out (inlined in 0x000fedf0 and 0x000ff050) */
static void dc_wave_end(struct CDemolition_GasJets *self)
{
	int g;

	self->state = 0;
	for (g = 0; g < 4; g++)
	{
		self->lights[g].mode = 2;
		FUN_0018ea10((int)self->lights[g].timer, 0.200000003f);
		FUN_0018ea30((int)self->lights[g].timer);
	}
	self->active = 0;
}

/* ---------- functions */

/* 0x000fe240  Demolition (file inferred: CDemolition_GasJets.cpp)
Constructor of a row light. */
struct DcJetLight *DcJetLight_Construct(struct DcJetLight *self)
{
	self->vtable = vtable_0026b748;
	FUN_000ec2f0((undefined4 *)self->timer);
	return self;
}

/* 0x000fe260  Demolition (file inferred: CDemolition_GasJets.cpp)
Destructor of a row light. */
void DcJetLight_Destruct(struct DcJetLight *self)
{
	self->vtable = vtable_0026b748;
	FUN_000ec300((undefined4 *)self->timer);
}

/* 0x000fe270  Demolition (file inferred: CDemolition_GasJets.cpp)
Updates a row light: fades it in (mode 1) or out (mode 2, then off) over its timer, then
updates the light. */
void DcJetLight_Update(struct DcJetLight *self, float dt)
{
	float f;

	FUN_0018e8e0((int)self->timer);
	if (self->mode == 1)
	{
		f = BCarTimer_GetRemaining((struct BCarTimer *)self->timer);
		FUN_0018fa10((int)self->light, 0, 0, (int)(f * 64.0f)); /* 0x00253f38 */
		if (!(f < 1.0f))
			self->mode = 0;
	}
	else if (self->mode == 2)
	{
		f = BCarTimer_GetRemaining((struct BCarTimer *)self->timer);
		FUN_0018fa10((int)self->light, 0, 0, (int)((1.0f - f) * 64.0f));
		if (!(f < 1.0f))
		{
			self->mode = 0;
			DC_LIGHT_SET_ON(self->light, 0);
		}
	}
	DC_LIGHT_UPDATE(self->light, dt);
}

/* 0x000fe330  Demolition (file inferred: CDemolition_GasJets.cpp)
Destructor of a jet. */
void DcGasJet_Destruct(struct DcGasJet *self)
{
	self->vtable = vtable_0026b74c;
	DcGasSystem_Destruct(DC_JET_GAS(self));
}

/* 0x000fe340  Demolition (file inferred: CDemolition_GasJets.cpp)
Draws a jet's flame: two additive beams up from 50 above the jet, a blue one and a narrower
grey one, with the game's flame texture. */
void DcGasJet_DrawFlame(struct DcGasJet *self)
{
	float base[4];
	float top[4];
	int black[4];
	int colour[4];
	int tex;
	float w;

	if (self->flameDraw == 0)
		return;
	tex = DC_I32(DC_GAME, 0x6c58);
	base[0] = self->pos[0];
	base[1] = self->pos[1] + 50.0f; /* 0x002559b8 */
	base[2] = self->pos[2];
	base[3] = 1.0f;
	top[3] = 1.0f;
	black[0] = 0;
	black[1] = 0;
	black[2] = 0;
	black[3] = 0;
	bdSetProjectionMode(1);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	w = self->flameWidth;
	colour[0] = 0;
	colour[1] = 0;
	colour[2] = 0x60;
	colour[3] = self->flameAlpha;
	top[0] = self->pos[0];
	top[1] = self->flameJitter + self->pos[1] + self->flameHeight;
	top[2] = self->pos[2];
	bdDrawBeam3D(top, 0.0f, 0.0f, black, base, w, w, colour, (struct bdTexture *)tex, NULL, 0.0f);
	w = self->flameWidth * 0.5f;
	colour[0] = 0x60;
	colour[1] = 0x60;
	colour[2] = 0x60;
	colour[3] = self->flameAlpha;
	top[0] = self->pos[0];
	top[1] = self->flameHeight * 0.699999988f /* 0x00266aa8 */ + self->flameJitter + self->pos[1];
	top[2] = self->pos[2];
	bdDrawBeam3D(top, 0.0f, 0.0f, black, base, w, w, colour, (struct bdTexture *)tex, NULL, 0.0f);
	bdSetProjectionMode(1);
	bdPopRenderState(6);
	bdPopRenderState(5);
}

/* 0x000fe4d0  Demolition (file inferred: CDemolition_GasJets.cpp)
Constructor: the 4 row lights and the wave timer. */
struct CDemolition_GasJets *CDemolition_GasJets_Construct(struct CDemolition_GasJets *self)
{
	int i;

	self->vtable = vtable_0026b750;
	for (i = 0; i < 4; i++)
		DcJetLight_Construct(&self->lights[i]);
	FUN_000ec2f0((undefined4 *)self->timer);
	return self;
}

/* 0x000fe540  Demolition (file inferred: CDemolition_GasJets.cpp)
Destructor. */
void CDemolition_GasJets_Destruct(struct CDemolition_GasJets *self)
{
	int i;

	self->vtable = vtable_0026b750;
	FUN_000ec300((undefined4 *)self->timer);
	for (i = 3; i >= 0; i--)
		DcJetLight_Destruct(&self->lights[i]);
}

/* 0x000fe5b0  Demolition (file inferred: CDemolition_GasJets.cpp)
Shutdown: releases and deletes the jets and the lights. */
void CDemolition_GasJets_Shutdown(struct CDemolition_GasJets *self)
{
	int i;

	for (i = 0; i < 23; i++)
	{
		DC_PS_CALL(self->jets[i]->gas, 0);
		if (self->jets[i] != NULL)
			((void (*)(void *, int))(*(void *const *const *)self->jets[i])[0])(self->jets[i], 1);
		self->jets[i] = NULL;
	}
	for (i = 0; i < 4; i++)
	{
		DC_LIGHT_RELEASE(self->lights[i].light);
		if (self->lights[i].light != NULL)
			DC_LIGHT_DELETE(self->lights[i].light, 1);
	}
}

/* 0x000fe610  Demolition (file inferred: CDemolition_GasJets.cpp; added to the table in
integration round 2)
Lets the waves start (clears "stopped"). Variation 2 slot 3 (Start) calls it. */
void CDemolition_GasJets_Start(struct CDemolition_GasJets *self)
{
	self->stopped = 0;
}

/* 0x000fe620  Demolition (file inferred: CDemolition_GasJets.cpp)
Starts a wave from the outside (state 1): a sound, and each row's light at its first jet. */
void CDemolition_GasJets_StartOutside(struct CDemolition_GasJets *self)
{
	struct DcGasJet *jet;
	int g;

	self->state = 1;
	FUN_0018ea10((int)self->timer, 1.0f);
	FUN_0018ea30((int)self->timer);
	FUN_0018e450((int)(DC_GAME + 0x79d0), 0x18, 0xff, 0, 0, 0);
	for (g = 0; g < 4; g++)
	{
		jet = self->groups[g].jets[0];
		dc_light_start(&self->lights[g], jet);
		jet = self->groups[g].jets[0];
		jet->flameOn = 1;
		jet->flameDraw = 1;
	}
}

/* 0x000fe700  Demolition (file inferred: CDemolition_GasJets.cpp)
State 1: after a second, a second sound and the wave runs in (state 2). */
void CDemolition_GasJets_WaitOutside(struct CDemolition_GasJets *self, float dt)
{
	(void)dt;
	FUN_0018e8e0((int)self->timer);
	if (BCarTimer_GetRemaining((struct BCarTimer *)self->timer) < 1.0f)
		return;
	FUN_0018e450((int)(DC_GAME + 0x79d0), 0x16, 0xff, 0, 0, 0);
	self->state = 2;
	FUN_0018ea10((int)self->timer, 1.0f);
	FUN_0018ea30((int)self->timer);
}

/* 0x000fe770  Demolition (file inferred: CDemolition_GasJets.cpp)
Starts a wave from the centre (state 3): a sound, and each row's light at the centre jet. */
void CDemolition_GasJets_StartCentre(struct CDemolition_GasJets *self)
{
	struct DcGasJet *jet;
	int g;

	self->state = 3;
	FUN_0018ea10((int)self->timer, 1.0f);
	FUN_0018ea30((int)self->timer);
	FUN_0018e450((int)(DC_GAME + 0x79d0), 0x18, 0xff, 0, 0, 0);
	for (g = 0; g < 4; g++)
	{
		jet = self->groups[g].jets[self->groups[g].count - 1];
		dc_light_start(&self->lights[g], jet);
		jet = self->groups[g].jets[self->groups[g].count - 1];
		jet->flameOn = 1;
		jet->flameDraw = 1;
	}
}

/* 0x000fe850  Demolition (file inferred: CDemolition_GasJets.cpp)
State 3: after a second, a second sound and the wave runs out (state 4). */
void CDemolition_GasJets_WaitCentre(struct CDemolition_GasJets *self, float dt)
{
	(void)dt;
	FUN_0018e8e0((int)self->timer);
	if (BCarTimer_GetRemaining((struct BCarTimer *)self->timer) < 1.0f)
		return;
	FUN_0018e450((int)(DC_GAME + 0x79d0), 0x16, 0xff, 0, 0, 0);
	self->state = 4;
	FUN_0018ea10((int)self->timer, 1.0f);
	FUN_0018ea30((int)self->timer);
}

/* 0x000fe8c0  Demolition (file inferred: CDemolition_GasJets.cpp)
Resets the jets: no wave, flames down, gas reset. */
void CDemolition_GasJets_ResetJets(struct CDemolition_GasJets *self)
{
	struct DcGasJet *jet;
	int i;

	self->state = 0;
	self->active = 0;
	for (i = 0; i < 23; i++)
	{
		jet = self->jets[i];
		jet->enabled = 1;
		jet->visible = 1;
		jet->firing = 0;
		jet->flameOn = 0;
		jet->flameDraw = 0;
		jet->flameHeight = 0.0f;
		jet->flameMax = 200.0f;
		jet->flameWidth = 30.0f;
		jet->flameAlpha = 0x40;
		jet->flameJitter = 0.0f;
		DC_PS_CALL(jet->gas, 3);
	}
}

/* 0x000fe930  Demolition/CDemolition_GasJets.cpp
Deleting destructor of a row light (vtable slot 0). */
struct DcJetLight *DcJetLight_DeletingDtor(struct DcJetLight *self, unsigned char flags)
{
	DcJetLight_Destruct(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x000fe950  Demolition/CDemolition_GasJets.cpp
Creates a row light: a CDemolition_Light, dark blue (0, 0, 64), off. Returns 1. */
int DcJetLight_Init(struct DcJetLight *self)
{
	void *light;

	light = bkHeapAlloc(0x34, s_DcGasJetsFile, 0x5e, 0x1001);
	if (light != NULL)
		FUN_000ed640((undefined4 *)light); /* the constructor returns `light` */
	self->light = light;
	DC_LIGHT_INIT(light);
	FUN_0018fa10((int)self->light, 0, 0, 0x40);
	dc_light_set_range(self->light, 0.000500000024f);
	DC_LIGHT_SET_ON(self->light, 0);
	FUN_000ec310((int)self->timer);
	self->mode = 0;
	DC_LIGHT_SET_ON(self->light, 0);
	return 1;
}

/* 0x000fea00  Demolition/CDemolition_GasJets.cpp
Deleting destructor of a jet (vtable slot 0). */
struct DcGasJet *DcGasJet_DeletingDtor(struct DcGasJet *self, unsigned char flags)
{
	DcGasJet_Destruct(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x000fea20  Demolition/CDemolition_GasJets.cpp
Sets a jet up: its gas (10 puffs, the game's gas texture) and a flame at rest. Returns 1. */
int DcGasJet_Init(struct DcGasJet *self)
{
	DcGasSystem_Init(DC_JET_GAS(self), NULL, 10);
	CParticleSystem_SetTexture((struct CPSHeader *)self->gas, DC_I32(DC_GAME, 0x6c5c));
	self->enabled = 1;
	self->visible = 1;
	self->firing = 0;
	self->flameOn = 0;
	self->flameDraw = 0;
	self->flameHeight = 0.0f;
	self->flameMax = 200.0f;
	self->flameWidth = 30.0f;
	self->flameAlpha = 0x40;
	self->flameJitter = 0.0f;
	DC_PS_CALL(self->gas, 3);
	return 1;
}

/* 0x000feaa0  Demolition/CDemolition_GasJets.cpp
Deleting destructor (vtable slot 0). */
struct CDemolition_GasJets *CDemolition_GasJets_DeletingDtor(struct CDemolition_GasJets *self,
                                                             unsigned char flags)
{
	CDemolition_GasJets_Destruct(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x000feac0  Demolition/CDemolition_GasJets.cpp
Initialise (variation 2's Initialise 0x000ea350): creates the 23 jets, the 4 rows (5, 6, 5
and 6 jets, each ending with the centre jet) and their lights, and waits 200-400 frames for
the first wave. Returns 1. */
int CDemolition_GasJets_Initialise(struct CDemolition_GasJets *self)
{
	static const int s_rowLength[4] = { 5, 6, 5, 6 };
	struct DcGasJet *jet;
	int next;
	int g;
	int k;
	int i;

	self->stopped = 1;
	for (i = 0; i < 23; i++)
	{
		jet = bkHeapAlloc(0x9c, s_DcGasJetsFile, 0x2d3, 0x1001);
		if (jet != NULL)
		{
			jet->vtable = vtable_0026b74c;
			DcGasSystem_Construct(DC_JET_GAS(jet));
		}
		self->jets[i] = jet;
		DcGasJet_Init(jet);
		jet = self->jets[i];
		jet->pos[0] = DAT_002989c8[i][0];
		jet->pos[1] = DAT_002989c8[i][1];
		jet->pos[2] = DAT_002989c8[i][2];
		jet->pos[3] = DAT_002989c8[i][3];
		CParticleSystem_SetPosition((struct CPSHeader *)jet->gas, jet->pos);
	}
	next = 0;
	for (g = 0; g < 4; g++)
	{
		self->groups[g].count = s_rowLength[g];
		for (k = 0; k < self->groups[g].count; k++)
			self->groups[g].jets[k] = self->jets[next++];
	}
	for (g = 0; g < 4; g++)
	{
		self->groups[g].jets[self->groups[g].count] = self->jets[22];
		self->groups[g].count = self->groups[g].count + 1;
	}
	for (i = 0; i < 4; i++)
		DcJetLight_Init(&self->lights[i]);
	FUN_000ec310((int)self->timer);
	self->waitTime = (float)dc_rand() * 3.05185094e-05f * 200.0f + 200.0f;
	CDemolition_GasJets_ResetJets(self);
	return 1;
}

/* 0x000fecf0  Demolition (file inferred: CDemolition_GasJets.cpp)
Stops the jets (the end of a round): no new wave, the jets stop burning. */
void CDemolition_GasJets_Stop(struct CDemolition_GasJets *self)
{
	struct DcGasJet *jet;
	int i;

	self->stopped = 1;
	self->state = 0;
	for (i = 0; i < 23; i++)
	{
		jet = self->jets[i];
		jet->firing = 0;
		DC_JET_GAS(jet)->p.emitting = 0;
		jet->flameOn = 0;
	}
}

/* 0x000fed20  Demolition (file inferred: CDemolition_GasJets.cpp)
State 0: unless stopped, counts down to the next wave (then 200-400 frames again) and starts
it from the outside or the centre at random. */
void CDemolition_GasJets_Wait(struct CDemolition_GasJets *self, float dt)
{
	float t;

	if (self->stopped != 0)
		return;
	t = self->waitTime - dt;
	self->waitTime = t;
	if (!(t <= 0.0f))
		return;
	self->waitTime = (float)dc_rand() * 3.05185094e-05f * 200.0f + 200.0f;
	if ((dc_rand() & 1) == 0)
		CDemolition_GasJets_StartOutside(self);
	else
		CDemolition_GasJets_StartCentre(self);
	self->active = 1;
}

/* 0x000fedf0  Demolition (file inferred: CDemolition_GasJets.cpp)
State 2: over a second the fire runs from the first jet of each row to the centre, with the
row lights; then the lights fade out and the wave ends. */
void CDemolition_GasJets_SpreadIn(struct CDemolition_GasJets *self, float dt)
{
	float f;
	int g;

	(void)dt;
	FUN_0018e8e0((int)self->timer);
	f = BCarTimer_GetRemaining((struct BCarTimer *)self->timer);
	for (g = 0; g < 4; g++)
		dc_jet_ignite(self->groups[g].jets[(int)((float)(self->groups[g].count - 1) * f)]);
	dc_lights_follow(self, f);
	if (1.0f <= f)
		dc_wave_end(self);
}

/* 0x000ff050  Demolition (file inferred: CDemolition_GasJets.cpp)
State 4: over a second the fire runs from the centre out to the first jet of each row. */
void CDemolition_GasJets_SpreadOut(struct CDemolition_GasJets *self, float dt)
{
	float f;
	int count;
	int g;

	(void)dt;
	FUN_0018e8e0((int)self->timer);
	f = BCarTimer_GetRemaining((struct BCarTimer *)self->timer);
	for (g = 0; g < 4; g++)
	{
		count = self->groups[g].count;
		dc_jet_ignite(self->groups[g].jets[count - (int)((float)(count - 1) * f) - 1]);
	}
	dc_lights_follow(self, 1.0f - f);
	if (1.0f <= f)
		dc_wave_end(self);
}

/* 0x000ff200  Demolition (file inferred: CDemolition_GasJets.cpp)
Draws the visible jets: flame, then gas. */
void CDemolition_GasJets_Draw(struct CDemolition_GasJets *self)
{
	struct DcGasJet *jet;
	int i;

	for (i = 0; i < 23; i++)
	{
		jet = self->jets[i];
		if (jet->visible != 0)
		{
			DcGasJet_DrawFlame(jet);
			DC_PS_CALL(jet->gas, 2);
		}
	}
}

/* 0x000ff240  Demolition (file inferred: CDemolition_GasJets.cpp)
Updates a jet: its gas; its flame (higher and wider while burning; growing 20 a frame while on,
else sinking 30, with a random flicker); and while burning (30 frames), burns the players in
it (radius 120, height 300). */
void DcGasJet_Update(struct DcGasJet *self, float dt)
{
	struct DcMessage msg;
	float textPos[4];
	char *player;
	float h;
	int health;
	int left;
	int damage;

	((void (*)(void *, float))(*(void *const *const *)self->gas)[1])(self->gas, dt);
	if (self->flameDraw != 0)
	{
		if (self->firing != 0)
		{
			self->flameMax = 300.0f;
			self->flameWidth = 50.0f;
		}
		else
		{
			self->flameMax = 200.0f;
			self->flameWidth = 30.0f;
		}
		if (self->flameOn != 0)
		{
			h = dt * 20.0f + self->flameHeight; /* 0x00255530 */
			self->flameHeight = h;
			if (!(h < self->flameMax))
				self->flameHeight = self->flameMax;
			self->flameDraw = 1;
			self->flameJitter = (float)dc_rand() * 3.05185094e-05f * 20.0f - 10.0f;
		}
		else
		{
			h = self->flameHeight - dt * 30.0f; /* 0x002553a8 */
			self->flameHeight = h;
			if (h <= 0.0f)
			{
				self->flameHeight = 0.0f;
				self->flameDraw = 0;
			}
			self->flameJitter = 0.0f;
		}
		self->flameAlpha = (dc_rand() & 0xf) + 0x40;
	}
	if (self->firing == 0)
		return;
	h = self->fireTime - dt;
	self->fireTime = h;
	if (h <= 0.0f)
	{
		self->firing = 0;
		DC_JET_GAS(self)->p.emitting = 0;
		self->flameOn = 0;
	}
	for (player = DC_PTR(DC_GAME, 0x6c); player != NULL; player = DC_PTR(player, 0x1140))
	{
		if (!(DC_F32(player, 0x1198) <= 0.0f))
			continue;
		if (FUN_000eee30(self->pos, 120.0f, 300.0f, DC_FP(player, 0x1144), DC_F32(player, 0x1164),
			DC_F32(player, 0x1168)) == 0)
			continue;
		DC_F32(player, 0x1198) = 100.0f;
		textPos[0] = DC_F32(player, 0x1144);
		textPos[1] = DC_F32(player, 0x1168) + DC_F32(player, 0x1168) + DC_F32(player, 0x1148);
		textPos[2] = DC_F32(player, 0x114c);
		textPos[3] = DC_F32(player, 0x1150);
		health = DC_I32(player, 0x1194);
		left = (int)((float)health * 0.75f); /* 0x00253844 */
		damage = health - left;
		DC_I32(player, 0x1194) = health - damage;
		if (DC_I32(player, 0x1194) < 0)
			DC_I32(player, 0x1194) = 0;
		if (0 < damage)
			dc_popup_int(textPos, DC_I32(player, 0x1178), DC_I32(player, 0x117c),
				DC_I32(player, 0x1180), "-%d", damage);
		FUN_0010cec0((undefined4 *)&msg);
		msg.pos[1] = self->pos[1];
		msg.pos[0] = self->pos[0];
		msg.pos[2] = self->pos[2];
		msg.type = 0;
		msg.mode = 2;
		msg.kind = 1;
		msg.pos[3] = self->pos[3];
		msg.value = 240.0f;
		FUN_0010d050((int)(player + 0x1130), (int)&msg);
		FUN_0010ced0((undefined4 *)&msg);
	}
}

/* 0x000ff570  Demolition (file inferred: CDemolition_GasJets.cpp)
Update (through the variation): the enabled jets, the row lights, then the wave state. */
void CDemolition_GasJets_Update(struct CDemolition_GasJets *self, float dt)
{
	int i;

	for (i = 0; i < 23; i++)
	{
		if (self->jets[i]->enabled != 0)
			DcGasJet_Update(self->jets[i], dt);
	}
	for (i = 0; i < 4; i++)
		DcJetLight_Update(&self->lights[i], dt);
	switch (self->state)
	{
	case 0:
		CDemolition_GasJets_Wait(self, dt);
		break;
	case 1:
		CDemolition_GasJets_WaitOutside(self, dt);
		break;
	case 2:
		CDemolition_GasJets_SpreadIn(self, dt);
		break;
	case 3:
		CDemolition_GasJets_WaitCentre(self, dt);
		break;
	case 4:
		CDemolition_GasJets_SpreadOut(self, dt);
		break;
	default:
		break;
	}
}

/* 0x0026b748 */
const void *const vtable_0026b748[1] = { (const void *)DcJetLight_DeletingDtor };
/* 0x0026b74c */
const void *const vtable_0026b74c[1] = { (const void *)DcGasJet_DeletingDtor };
/* 0x0026b750 */
const void *const vtable_0026b750[1] = { (const void *)CDemolition_GasJets_DeletingDtor };
