/*
ScratchMatch/CScratchmatch_Scenics.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CScratchmatch_Scenics.cpp
(module scratchmatch)

A small scenery class (struct SMScenic, 0x10 bytes): an actor with one animation ("beat")
that is restarted on every beat. Scratch Match has one: the speakers (actors\speakers.obe,
sm_Speakers). The methods were thiscall (`self` in ECX).
*/

#include "scratchmatch.h"

static const char s_ScenicsFile[] = "C:\\BlitzParty\\Game\\Src\\ScratchMatch\\CScratchmatch_Scenics.cpp";

/* 0x001c1560  (gap; the class's constructor) */
struct SMScenic *SMScenic_Ctor(struct SMScenic *self)
{
	self->actor = NULL;
	self->instance = NULL;
	self->speed = 0.0f;
	return self;
}

/* 0x001c1570  (gap; the class's destructor): deletes the instance */
void SMScenic_Dtor(struct SMScenic *self)
{
	if (self->instance != NULL) {
		baDeleteActorInstance(self->instance);
		self->instance = NULL;
	}
}

/* 0x001c1590  ScratchMatch/CScratchmatch_Scenics.cpp: loads `model` from the variation package,
makes its instance at the start of its animation and finds the "beat" animation */
void SMScenic_Load(struct SMScenic *self, const char *model)
{
	struct BActorInstance *instance;

	self->actor = (struct BActor *)bLoadResource(sm_VarPackage, model, 1, 0xdefa);
	instance = baInitActorInstance(NULL, self->actor, 2, s_ScenicsFile, 0x20);
	self->instance = instance;
	baPrepActorInstance(instance, 1);
	baSetNodeTime(self->instance, self->instance->root, 0, 1);
	self->anim = baFindSubObject(self->instance, "beat");
	if (self->anim == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Scenics.cpp", 0x25, "POINTER ASSERTION FAILURE: m_pAnimationSegment");
	self->speed = 50.0f;
}

/* 0x001c1620  (gap; placed with the class): draws it (tweak "Draw Extras") */
void SMScenic_Draw(struct SMScenic *self)
{
	if (sm_DrawExtras == 1) {
		bdSetRenderState(3, 1, 0);
		bdSetRenderState(4, 0, 0);
		bdSetProjectionMode(1);
		bdSetRenderState(0, 1, 0);
		bdSetRenderState(1, 2, 0);
		bdSetRenderState(5, 1, 0);
		bdSetRenderState(6, 0, 0);
		bdSetRenderState(10, 0, 0);
		baSetBrightness(0x100);
		bdSetWorldMatrix(DAT_002855f0);
		baPrepActorInstance(self->instance, 1);
		baDrawActorInstance(self->instance, 7);
	}
}

/* 0x001c16c0  (gap; placed with the class): plays the "beat" animation once */
void SMScenic_Play(struct SMScenic *self)
{
	baQueueAnimation(self->instance, self->instance->root, (struct BAnim *)self->anim, 0x400, -1, 0, NULL, NULL);
}

/* 0x001c16f0  (gap; placed with the class): at the origin, unrotated, and its animation runs */
void SMScenic_Update(struct SMScenic *self, float dt)
{
	struct BActorInstance *instance = self->instance;

	instance->pos[3] = 1.0f;
	instance->pos[0] = 0.0f;
	instance->pos[1] = 0.0f;
	instance->pos[2] = 0.0f;
	instance = self->instance;
	instance->rot[0] = DAT_00285634[0];
	instance->rot[1] = DAT_00285634[1];
	instance->rot[2] = DAT_00285634[2];
	instance->rot[3] = DAT_00285634[3];
	baUpdateAnimations(self->instance, self->instance->root, dt * self->speed, NULL);
}
