/*
ScratchMatch/ScratchMatch_GenActors.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\ScratchMatch_GenActors.cpp
(module scratchmatch)

"Generic actors": one actor type (maxGenActorInfo: models\hovver_light.obe and its animation
"spin") and two instances of it (maxGenActors, 0x20 bytes each at 0x002b5d50: info index,
position, animation speed, bob height, instance, animation) that hover over the stage.
*/

#include "scratchmatch.h"

static const char s_GenActorsFile[] = "C:\\BlitzParty\\Game\\Src\\ScratchMatch\\ScratchMatch_GenActors.cpp";

/* the actor type (100 bytes from 0x002b5ce8: model name, actor, animation name) */
#define sm_GenInfoActor(i) SM_G(0x002b5d28 + 100 * (i), struct BActor *)
#define sm_GenInfoAnim(i) (SM_A(0x002b5d2c) + 100 * (i))
/* an instance (0x20 bytes from 0x002b5d50) */
#define SMG_INFO(i) SM_I(0x002b5d50 + 0x20 * (i))
#define SMG_POS(i) SM_FV(0x002b5d54 + 0x20 * (i))
#define SMG_SPEED(i) SM_F(0x002b5d60 + 0x20 * (i))
#define SMG_BOB(i) SM_F(0x002b5d64 + 0x20 * (i))
#define SMG_INSTANCE(i) SM_G(0x002b5d68 + 0x20 * (i), struct BActorInstance *)
#define SMG_ANIM(i) SM_G(0x002b5d6c + 0x20 * (i), struct BActorSubObject *)

/* 0x001bcfe0  ScratchMatch/ScratchMatch_GenActors.cpp: loads the actor */
void SMGenActors_Load(void)
{
	char name[64];

	wsprintfA(name, "%s.obe", sm_GenActorModel);
	sm_GenInfoActor(0) = (struct BActor *)bLoadResource(sm_Package, name, 1, 0xdefa);
	if (sm_GenInfoActor(0) == NULL)
		SM_ASSERT_FAIL("ScratchMatch_GenActors.cpp", 0x1d,
			"POINTER ASSERTION FAILURE: maxGenActorInfo[ un32ActorIndex ].pActor");
}

/* 0x001bd040  ScratchMatch/ScratchMatch_GenActors.cpp: frees the actor */
void SMGenActors_Free(void)
{
	if (sm_GenInfoActor(0) == NULL)
		SM_ASSERT_FAIL("ScratchMatch_GenActors.cpp", 0x28,
			"POINTER ASSERTION FAILURE: maxGenActorInfo[ un32ActorIndex ].pActor");
	bkDeleteResource((struct bkResource *)sm_GenInfoActor(0));
	sm_GenInfoActor(0) = NULL;
}

/* 0x001bd080  ScratchMatch/ScratchMatch_GenActors.cpp: deletes the two instances */
void SMGenActors_DeleteInstances(void)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (SMG_INSTANCE(i) == NULL)
			SM_ASSERT_FAIL("ScratchMatch_GenActors.cpp", 0x4c,
				"ASSERTION FAILURE: maxGenActors[ un32GenActor ].pActorInstance != NULL");
		baDeleteActorInstance(SMG_INSTANCE(i));
		SMG_INSTANCE(i) = NULL;
		SMG_ANIM(i) = NULL;
	}
}

/* 0x001bd0d0  ScratchMatch/ScratchMatch_GenActors.cpp (bracketed): draws the instances (tweak
"Draw Extras") */
void SMGenActors_Draw(void)
{
	int i;

	if (sm_DrawExtras == 1) {
		for (i = 0; i < 2; i++) {
			baDrawActorInstance(SMG_INSTANCE(i), 1);
			baDrawActorInstance(SMG_INSTANCE(i), 4);
		}
	}
}

/* 0x001bd110  ScratchMatch/ScratchMatch_GenActors.cpp: makes the instances and starts their
animation at a random time */
void SMGenActors_CreateInstances(void)
{
	struct BActorInstance *instance;
	struct BActorSubObject *anim;
	int i;

	for (i = 0; i < 2; i++) {
		if (SMG_INSTANCE(i) != NULL)
			SM_ASSERT_FAIL("ScratchMatch_GenActors.cpp", 0x35,
				"ASSERTION FAILURE: maxGenActors[ un32GenActor ].pActorInstance == NULL");
		instance = baInitActorInstance(NULL, sm_GenInfoActor(SMG_INFO(i)), 2, s_GenActorsFile, 0x36);
		SMG_INSTANCE(i) = instance;
		anim = baFindSubObject(instance, sm_GenInfoAnim(SMG_INFO(i)));
		SMG_ANIM(i) = anim;
		if (anim == NULL)
			SM_ASSERT_FAIL("ScratchMatch_GenActors.cpp", 0x3a,
				"POINTER ASSERTION FAILURE: maxGenActors[ un32GenActor ].pAnimSegment");
		baFlushAnimations(SMG_INSTANCE(i), SMG_INSTANCE(i)->root, 1, 0);
		baQueueAnimation(SMG_INSTANCE(i), SMG_INSTANCE(i)->root, (struct BAnim *)SMG_ANIM(i), 0,
			(int)(sm_rand() % 0x3c), 1, NULL, NULL);
	}
}

/* 0x001bd210  (gap; the instances' frame, after the file's tagged functions): bobs the two
lights up and down (out of phase) and runs their animation */
void SMGenActors_Update(void)
{
	struct BActorInstance *instance;
	float wave;
	unsigned int i;

	for (i = 0; i < 2; i++) {
		bdSetWorldMatrix(DAT_002855f0);
		wave = (float)sin((sm_ufloat(i) * 3.14159274f + SMTimer_Elapsed(SMT_BEAT)) * 2.0);
		SMG_BOB(i) = fabsf(wave) * 40.0f;
		SMG_INSTANCE(i)->pos[0] = SMG_POS(i)[0];
		SMG_INSTANCE(i)->pos[1] = SMG_POS(i)[1] + SMG_BOB(i);
		SMG_INSTANCE(i)->pos[2] = SMG_POS(i)[2];
		instance = SMG_INSTANCE(i);
		baUpdateAnimations(instance, instance->root, (float)(SMTimer_FrameTime() * SMG_SPEED(i)), NULL);
		if (SMG_INSTANCE(i) != NULL)
			baPrepActorInstance(SMG_INSTANCE(i), 1);
	}
}
