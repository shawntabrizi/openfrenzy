/*
ClassLib/CRay.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\CRay.cpp

Orb ray controls: an orb follows an anchor point; its rays spin around it and are
drawn while a test against the target passes ("CreateOrbRayControl").

0x000a76c0 and 0x000a7800 are tagged with CRay.cpp. The gap functions 0x000a7550-
0x000a7650 and 0x000a7900-0x000a7d00 work on the same objects and list, so they are
placed here (file inferred).
*/

#include "classlib_fx.h"

static const char s_CRayFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\CRay.cpp";

/* 0x000a7550  ClassLib (gap; file inferred: CRay.cpp)
Empties the ray control list (its constructor run again). */
void CRayControl_ResetList(void)
{
	FUN_00071060((int *)&DAT_002f27c8);
}

/* 0x000a7560  ClassLib (gap; file inferred: CRay.cpp)
Constructor: an orb at `anchor` with no rays, linked into the ray control list. */
CRayControl *CRayControl_Init(CRayControl *self, int texture, float *anchor, int *colour0,
                              int *colour1, float *target, void *test)
{
	CLF_ListInit(&self->rays);
	self->target = target;
	self->test = test;
	self->texture = texture;
	self->anchor = anchor;
	self->centre[0] = anchor[0];
	self->centre[1] = anchor[1];
	self->centre[2] = anchor[2];
	self->centre[3] = anchor[3];
	self->angle = 0.0f;
	self->offset[0] = DAT_002ea478[0];
	self->offset[1] = DAT_002ea478[1];
	self->offset[2] = DAT_002ea478[2];
	self->offset[3] = DAT_002ea478[3];
	self->colour0[0] = colour0[0];
	self->colour0[1] = colour0[1];
	self->colour0[2] = colour0[2];
	self->colour0[3] = colour0[3];
	self->colour1[0] = colour1[0];
	self->colour1[1] = colour1[1];
	self->colour1[2] = colour1[2];
	self->numRays = 0;
	self->colour1[3] = colour1[3];
	CLF_ListInsert(&DAT_002f27c8, self, 0);
	return self;
}

/* 0x000a7650  ClassLib (gap; file inferred: CRay.cpp)
Draws the rays of a visible orb. */
void CRayControl_Draw(CRayControl *self)
{
	CRay *r;

	if (self->visible != 0) {
		for (r = CLF_ListNext(&self->rays, NULL); r != NULL; r = CLF_ListNext(&self->rays, r)) {
			r->control->centre[3] = 1.0f;
			r->pos[3] = 1.0f;
			CLF_DrawBeam2(r->control->centre, r->width0, r->width0, r->control->colour0, r->pos,
			              r->width1, r->width1, r->control->colour1, r->control->texture, NULL, 0.0f);
		}
	}
}

/* 0x000a76c0  ClassLib/CRay.cpp
Adds a ray to the orb. */
CRay *CRayControl_AddRay(CRayControl *self, float angle, float speed, float length,
                         float width0, float width1)
{
	CRay *r;

	r = CLF_Alloc(sizeof(CRay), s_CRayFile, 0xf2, 0x1001);
	if (r != NULL) {
		r->speed = speed;
		r->angle = angle;
		r->length = length;
		r->width0 = width0;
		r->length0 = length;
		r->control = self;
		r->width1 = width1;
		CLF_ListInsert(&self->rays, r, 0);
		self->numRays = self->numRays + 1;
		return r;
	}
	return NULL;
}

/* 0x000a7730  ClassLib (gap; file inferred: CRay.cpp)
Spins the ray and places its end: (0, 1, 0) rolled by the ray's angle (plus the orb's
turn), turned to face the target, scaled by the length, from the orb centre. */
void CRay_Update(CRay *self)
{
	float v[4];
	float roll;
	float heading;
	float *target;
	CRayControl *c;

	v[0] = 0.0f;
	v[1] = 1.0f;
	v[2] = 0.0f;
	v[3] = 1.0f;
	self->angle = DAT_0028fda8 * self->speed + self->angle;
	roll = FUN_000703a0(self->angle + self->control->angle);
	c = self->control;
	target = c->target;
	heading = (float)atan2((double)target[0] - (double)c->centre[0],
	                       (double)target[2] - (double)c->centre[2]);
	FUN_000512a0(v, v, roll, heading);
	c = self->control;
	self->pos[0] = v[0] * self->length + c->centre[0];
	self->pos[1] = v[1] * self->length + c->centre[1];
	self->pos[2] = self->length * v[2] + c->centre[2];
	self->pos[3] = 1.0f;
}

/* 0x000a7800  ClassLib/CRay.cpp
CreateOrbRayControl(type, anchor, target, test): type 1 is the rainbow orb. */
CRayControl *CRay_CreateOrbRayControl(int type, float *anchor, float *target, void *test)
{
	int colour1[4];
	int colour0[4];
	int texture;
	void *mem;
	CRayControl *c;

	colour0[0] = 0x7f;
	colour0[1] = 0x7f;
	colour0[2] = 0x7f;
	colour1[0] = 0x7f;
	colour1[1] = 0x7f;
	colour1[2] = 0x7f;
	colour0[3] = 0x28;
	colour1[3] = 0;
	if (type == 1) {
		texture = CLF_FindTexture(0, "rainbow.bmp", 0, 0xdefa);
		if (texture == 0)
			CLF_Printf("CRay.cpp, CreateOrbRayControl(): couldn't find texture\n");
		mem = CLF_Alloc(sizeof(CRayControl), s_CRayFile, 0x173, 0x1001);
		if (mem == NULL)
			c = NULL;
		else
			c = CRayControl_Init(mem, texture, anchor, colour0, colour1, target, test);
		if (c != NULL)
			return c;
		CLF_Printf("couldn't create rayControl\n");
	}
	return NULL;
}

/* 0x000a7900  ClassLib (gap; file inferred: CRay.cpp)
Draws every orb's rays (additive, no z write). */
void CRayControl_DrawAll(void)
{
	void (*set_state)(int, int, int) = (void (*)(int, int, int))FUN_00046cb0;
	CRayControl *c;

	FUN_000438b0();
	set_state(6, 1, 0);
	set_state(10, 1, 0);
	set_state(7, 1, 0);
	set_state(5, 0, 0);
	for (c = CLF_ListNext(&DAT_002f27c8, NULL); c != NULL; c = CLF_ListNext(&DAT_002f27c8, c))
		CRayControl_Draw(c);
	FUN_00046d00(6);
	FUN_00046d00(10);
	FUN_00046d00(7);
	FUN_00046d00(5);
}

/* 0x000a7980  ClassLib (gap; file inferred: CRay.cpp)
Follows the anchor, updates the rays and runs the test: when it passes the rays show and
the orb turns towards the target. */
void CRayControl_Update(CRayControl *self)
{
	float *anchor;
	float *target;
	float delta[4];
	float toOrigin[4];
	byte a[16];
	byte b[16];
	CRay *r;
	float result;

	anchor = self->anchor;
	self->centre[0] = anchor[0];
	self->centre[1] = anchor[1];
	self->centre[2] = anchor[2];
	self->centre[3] = anchor[3];
	self->centre[0] = self->centre[0] + self->offset[0];
	self->centre[1] = self->centre[1] + self->offset[1];
	self->centre[2] = self->centre[2] + self->offset[2];
	self->centre[3] = 1.0f;
	for (r = CLF_ListNext(&self->rays, NULL); r != NULL; r = CLF_ListNext(&self->rays, r))
		CRay_Update(r);
	target = self->target;
	delta[0] = self->centre[0] - target[0];
	delta[1] = self->centre[1] - target[1];
	delta[2] = self->centre[2] - target[2];
	result = ((float (*)(float *, float *, void *, void *))self->test)(target, delta, b, a);
	if (result <= 0.0f) {
		self->visible = 1;
		toOrigin[0] = DAT_002ea478[0] - self->target[0];
		toOrigin[2] = DAT_002ea478[2] - self->target[2];
		toOrigin[1] = 0.0f;
		toOrigin[3] = 1.0f;
		delta[1] = 0.0f;
		delta[3] = 1.0f;
		FUN_000514a0(toOrigin, toOrigin);
		FUN_000514a0(delta, delta);
		self->angle = DAT_00292c44 * (toOrigin[2] * delta[0] - toOrigin[0] * delta[2]);
	} else {
		self->visible = 0;
	}
	if (0 < self->numRays)
		FUN_001d5950(); /* an empty debug function (anchor, 2.0, number of rays) */
}

/* 0x000a7af0  ClassLib (gap; file inferred: CRay.cpp)
Deletes every ray of the orb. */
void CRayControl_DeleteRays(CRayControl *self)
{
	CRay *r;

	r = CLF_ListNext(&self->rays, NULL);
	while (r != NULL) {
		CLF_ListRemove(&self->rays, r);
		if (r != NULL)
			CLF_Free(r);
		r = CLF_ListNext(&self->rays, NULL);
	}
	self->numRays = 0;
}

/* 0x000a7b40  ClassLib (gap; file inferred: CRay.cpp)
Gives the orb two rays per score point (1 to 4); other scores print "no score!". */
void CRayControl_SetScore(CRayControl *self, int score)
{
	CRayControl_DeleteRays(self);
	switch (score) {
	case 4:
		CRayControl_AddRay(self, 2.3561945f, 0.0f, 200.0f, 1.0f, 10.0f);
		CRayControl_AddRay(self, 5.497787f, 0.0f, 200.0f, 1.0f, 10.0f);
		/* fall through */
	case 3:
		CRayControl_AddRay(self, 0.0f, 0.0f, 500.0f, 1.0f, 40.0f);
		CRayControl_AddRay(self, 3.1415925f, 0.0f, 500.0f, 1.0f, 40.0f);
		/* fall through */
	case 2:
		CRayControl_AddRay(self, 0.4f, 0.0f, 300.0f, 1.0f, 20.0f);
		CRayControl_AddRay(self, 3.5415926f, 0.0f, 300.0f, 1.0f, 20.0f);
		/* fall through */
	case 1:
		CRayControl_AddRay(self, 1.57079625f, 0.0f, 210.0f, 1.0f, 14.0f);
		CRayControl_AddRay(self, 4.71238899f, 0.0f, 100.0f, 1.0f, 14.0f);
		return;
	default:
		CLF_Printf("no score!\n");
		return;
	}
}

/* 0x000a7c70  ClassLib (gap; file inferred: CRay.cpp)
Destructor: deletes the rays and unlinks the orb (the list destructor is empty). */
void CRayControl_Destroy(CRayControl *self)
{
	CRayControl_DeleteRays(self);
	CLF_ListRemove(&DAT_002f27c8, self);
	FUN_001d5950();
}

/* 0x000a7cd0  ClassLib (gap; file inferred: CRay.cpp)
Updates every orb. */
void CRayControl_UpdateAll(void)
{
	CRayControl *c;

	for (c = CLF_ListNext(&DAT_002f27c8, NULL); c != NULL; c = CLF_ListNext(&DAT_002f27c8, c))
		CRayControl_Update(c);
}

/* 0x000a7d00  ClassLib (gap; file inferred: CRay.cpp)
Deletes every orb. */
void CRayControl_DeleteAll(void)
{
	CRayControl *c;

	c = CLF_ListNext(&DAT_002f27c8, NULL);
	while (c != NULL) {
		CRayControl_Destroy(c);
		CLF_Free(c);
		c = CLF_ListNext(&DAT_002f27c8, NULL);
	}
}
