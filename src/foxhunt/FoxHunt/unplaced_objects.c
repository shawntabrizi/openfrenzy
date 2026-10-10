/*
FoxHunt/unplaced_objects.c  (module foxhunt; gap functions 0x00135800-0x00136530 of
C:\BlitzParty\Game\Src\FoxHunt\, file unknown)

The beach's big red button (brb_large.obe; 0x50 bytes, the buttons at 0x003022fc, count
0x003022f4, presses 0x003022f8), the sharks (shark.obe; 0x60 bytes, list 0x00302300 with
the node at +4) and the helpers of the flag list (0x0030230c).
*/

#include "foxhunt.h"

extern char *DAT_002ba96c;            /* core: the game (+0x18 stage) */

#define STAGE() FH_AI(DAT_002ba96c, 0x18)

/* ======================================================================== the button */

/* 0x00135800  (gap): loads the button model and clears the buttons */
void FoxHunt_Buttons_Init(void)
{
	FH_I(0x003022fc) = 0;
	FH_P(0x002adfdc) = (char *)bLoadResource(NULL, FH_P(0x002adfd8), 1, 0);
	if (FH_P(0x002adfdc) == NULL)
		bkPrintf("\t\t\t\t\t\t\t\t\t\t\t\tbutton\tEEEeeeeeeeeekkkkk!!!!!\n");
	FH_I(0x003022f4) = 0;
	FH_I(0x003022f8) = 0;
}

/* 0x00135840  (gap): a big red button: `kind` (+0x3c: 0 press, 1 press and release, 2
one press), the press range² (+0x20) and angle (+0x24), `b` (+0x38), at `pos` turned to
`heading`; its "brb_bone" node gives the press point */
void *FoxHunt_Button_Ctor(void *self, int kind, float range, float angle, int b, int unused, float *pos, float heading)
{
	char *actor;
	int i, off;

	(void)unused;
	actor = (char *)Actor_Create(FH_P(0x002adfdc), pos, NULL, 4, 0, 1);
	FH_AP(self, 0x34) = actor;
	FH_AI(actor, 0x220) = 0x100;
	Actor_InitCharData(FH_AP(self, 0x34));
	FH_AF(FH_AP(self, 0x34), 0xe0) = 0.15f;
	FH_AF(FH_AP(self, 0x34), 0xe4) = 0.15f;
	FH_AF(FH_AP(self, 0x34), 0xe8) = 0.15f;
	FH_AP(FH_AP(self, 0x34), 0x324) = FH_A(0x002adf50);
	FH_AF(FH_AP(self, 0x34), 0x1e4) = heading;
	for (i = 0, off = 0x264; off < 0x26c; off += 4, i += 0x44)
		FH_AP(FH_AP(self, 0x34), off) = (char *)baFindSubObject((struct BActorInstance *)(FH_AP(self, 0x34) + 0x10),
			FH_AP(FH_AP(self, 0x34), 0x324) + i);
	FH_PV(0x003022fc)[FH_I(0x003022f4)] = (char *)self;
	FH_AF(self, 0x20) = range;
	FH_AF(self, 0x24) = angle;
	FH_AI(self, 0x48) = 0;
	FH_AI(self, 0x4c) = 0;
	FH_AI(self, 0x3c) = kind;
	FH_AI(self, 0x44) = 0;
	FH_AI(self, 0x38) = b;
	FH_AP(self, 0x30) = (char *)baFindNode((struct BActorNode *)FH_AP(FH_AP(self, 0x34), 0x144), "brb_bone");
	if (FH_AP(self, 0x30) == NULL) {
		bkPrintf("couldn't find brb bone\n");
	} else {
		baGetNodePosition((struct BActorInstance *)(FH_AP(self, 0x34) + 0x10), (struct BActorNode *)FH_AP(self, 0x30),
			FH_AV(self, 0xc));
		FH_AF(self, 0x1c) = CLMath_HeadingOf(FH_AV(FH_AP(self, 0x30), 0x30));
	}
	FH_AI(self, 0x28) = 0;
	FH_AP(self, 0x28) = (char *)bLoadResource(NULL, "textures\\button.gif", 0, 0xdefa);
	if (FH_AP(self, 0x28) == NULL)
		bkPrintf("no anim texture for brb\n");
	FH_AS(FH_AP(self, 0x34), 0x234) = 1;
	bdTextureSetFrame((struct bdTexture *)FH_AP(self, 0x28), 1);
	FH_AI(self, 0x1c) = 0;
	FH_I(0x003022f4)++;
	return self;
}

/* 0x001359d0  (gap): presses the button (when it is enabled, +0x38): plays "down", drops
the pool (0x00137fe0). Returns 1 when pressed. */
int FoxHunt_Button_Press(void *self)
{
	if (FH_AI(self, 0x38) == 0)
		return 0;
	Actor_PlayAnim(FH_AP(self, 0x34), 0, 0, 0, 1.0f, 0.0f, 1);
	FH_AI(self, 0x48) = 2;
	FUN_00137fe0();
	FH_I(0x003022f8)++;
	return 1;
}

/* 0x00135a20  (gap): releases the button ("up") */
int FoxHunt_Button_Release(void *self)
{
	Actor_PlayAnim(FH_AP(self, 0x34), 1, 0, 0, 1.0f, 0.0f, 1);
	FH_AI(self, 0x48) = 3;
	FH_I(0x003022f8)++;
	return 1;
}

/* 0x00135a60  (gap): the button's frame: a player pressing action next to it in front of
it (not hurt or down) presses it (state 7, the "escalator" sound); then the button's states:
0 up, 1 down, 2 going down, 3 going up */
int FoxHunt_Button_Update(void *self)
{
	char *p;
	int i;
	float h, d;

	bdTextureSetFrame((struct bdTexture *)FH_AP(self, 0x28), FH_AS(FH_AP(self, 0x34), 0x234) != 0);
	for (i = 0; i < DAT_002bae94; i++) {
		p = fh_Players[i];
		if (FH_AI(FH_AP(p, 0x6c), 0x14) == 0 || (FH_AI(self, 0x48) != 1 && FH_AI(self, 0x48) != 0) ||
			FH_AS(p, 0x36) == 5 || FH_AS(p, 0x36) == 10)
			continue;
		d = fh_DistanceSquared(FH_AV(FH_AP(self, 0x34), 0xd0), FH_AV(FH_AP(p, 0x30), 0xd0));
		if (!(d < FH_AF(self, 0x20)))
			continue;
		h = CLMath_HeadingTo(FH_AV(FH_AP(self, 0x34), 0xd0), FH_AV(FH_AP(fh_Players[i], 0x30), 0xd0));
		if (fabsf(CLMath_AngleDistance(h, FH_AF(FH_AP(self, 0x34), 0x1e8))) < FH_AF(self, 0x24)) {
			FoxHunt_Button_Press(self);
			FH_AI(fh_Players[i], 0x1184) = 1;
			AI_SetState(fh_Players[i], 7);
			FH_AF(FH_AP(fh_Players[i], 0x30), 0x1d8) = h - 3.1415925f;
			FHunt_Sound_Play(fh_Speech, 0xe, 0xff, 0);
		}
	}
	switch (FH_AI(self, 0x48)) {
	case 0:
		if ((FH_AI(self, 0x3c) == 0 || FH_AI(self, 0x3c) == 1) && FH_AI(self, 0x4c) != 0) {
			FH_AI(self, 0x4c) = 0;
			FH_I(0x003022f8)--;
		}
		break;
	case 1:
		if (FH_AI(self, 0x3c) == 0 && FH_AI(self, 0x4c) != 1) {
			FH_AI(self, 0x4c) = 1;
			FH_I(0x003022f8)--;
		}
		if (FH_AI(self, 0x3c) == 1) {
			FH_AI(self, 0x4c) = FH_AI(self, 0x48);
			FH_I(0x003022f8)--;
			FoxHunt_Button_Release(self);
		}
		if (FH_AI(self, 0x3c) == 2) {
			FH_AI(self, 0x4c) = FH_AI(self, 0x48);
			FH_I(0x003022f8)--;
			FH_AI(self, 0x48) = 4;
			return 0;
		}
		break;
	case 2:
		if (FH_AS(FH_AP(self, 0x34), 0x234) != 0) {
			FH_AI(self, 0x4c) = 2;
			FH_AI(self, 0x48) = 1;
			FH_AI(self, 0x44)++;
		}
		return 1;
	case 3:
		if (FH_AS(FH_AP(self, 0x34), 0x234) != 0) {
			FH_AI(self, 0x4c) = 3;
			FH_AI(self, 0x48) = 0;
		}
		return 1;
	}
	return 0;
}

/* 0x00135cd0  (gap): the actor is within the button's range² and faces like its press
point (within the angle). Returns 1 then. */
int FoxHunt_Button_IsNear(void *self, char *actor)
{
	char *b = FH_AP(self, 0x34);
	float dx = FH_AF(b, 0xd0) - FH_AF(actor, 0xd0);
	float dy = FH_AF(b, 0xd4) - FH_AF(actor, 0xd4);
	float dz = FH_AF(b, 0xd8) - FH_AF(actor, 0xd8);

	if (dx * dx + dy * dy + dz * dz < FH_AF(self, 0x20) &&
		CLMath_AngleDistance(FH_AF(self, 0x1c), FH_AF(actor, 0x1e8)) < FH_AF(self, 0x24))
		return 1;
	return 0;
}

/* 0x00135d50  (gap): updates every button */
void FoxHunt_UpdateButtons(void)
{
	int i;

	for (i = 0; i < FH_I(0x003022f4); i++)
		FoxHunt_Button_Update(FH_PV(0x003022fc)[i]);
}

/* ======================================================================== the sharks */

/* 0x00135d80  (gap): a shark (index `index`) swimming from `pos` along its route of
4-6 points (the town or the beach) */
void *FoxHunt_Shark_Ctor(void *self, float *pos, int index)
{
	char *actor;
	int i, off;

	actor = (char *)Actor_Create(FH_P(0x002ae064), pos, NULL, 4, 0, 1);
	FH_AP(self, 0xc) = actor;
	FH_AI(actor, 0x220) = 0x100;
	CLList_Insert((int *)FH_ADDR(0x00302300), (int *)((char *)self + 4), 0);
	Actor_InitCharData(FH_AP(self, 0xc));
	FH_AP(FH_AP(self, 0xc), 0x324) = FH_A(0x002ae068);
	baFlushAnimations(FH_AP(self, 0xc) + 0x10, (struct BActorNode *)FH_AP(FH_AP(self, 0xc), 0x144), 1, 0);
	for (i = 0, off = 0x264; off < 0x26c; off += 4, i += 0x44) {
		FH_AP(FH_AP(self, 0xc), off) = (char *)baFindSubObject((struct BActorInstance *)(FH_AP(self, 0xc) + 0x10),
			FH_AP(FH_AP(self, 0xc), 0x324) + i);
		bkPrintf("anim seg\t\t\t\t\t\t\t\t\t\t\t\t\t\t%d \n", FH_AI(FH_AP(self, 0xc), off));
	}
	Actor_PlayAnim(FH_AP(self, 0xc), 0, 1, 0, 2.0f, 0.1f, 1);
	FH_AI(FH_AP(self, 0xc), 0x1e8) = 0;
	FH_AI(self, 0x0) = index;
	FH_AU(self, 0x18) = 0x3cf5c28fu;   /* 0.03: turn speed */
	FH_AF(self, 0x14) = 1.2f;          /* swim speed */
	FH_AI(self, 0x2c) = STAGE() != 4;
	if (STAGE() == 3)
		FH_AI(self, 0x1c) = 1;
	else if (STAGE() == 4)
		FH_AI(self, 0x1c) = 4;
	else if (STAGE() < 3)
		FH_AI(self, 0x1c) = 0;
	FH_AF(self, 0x20) = 150.0f;
	FH_AI(FH_AP(self, 0xc), 0x194) = 0;
	FH_AI(FH_AP(self, 0xc), 0x198) = 0;
	FH_AI(FH_AP(self, 0xc), 0x19c) = 0;
	FH_AI(self, 0x28) = 0;
	FH_AF(FH_AP(self, 0xc), 0xe0) = 1.5f;
	FH_AF(FH_AP(self, 0xc), 0xe4) = 1.5f;
	FH_AF(FH_AP(self, 0xc), 0xe8) = 1.5f;
	if (STAGE() > 2) {
		FH_AF(self, 0x50) = FH_FV(0x002adfe8)[FH_AI(self, 0x1c) * 3];
		FH_AF(self, 0x54) = FH_FV(0x002adfe8)[FH_AI(self, 0x1c) * 3 + 1];
		FH_AF(self, 0x58) = FH_FV(0x002adfe8)[FH_AI(self, 0x1c) * 3 + 2];
		FH_AF(self, 0x5c) = 1.0f;
		return self;
	}
	if (STAGE() < 3) {
		FH_AF(self, 0x50) = FH_FV(0x002ae030)[FH_AI(self, 0x1c) * 3];
		FH_AF(self, 0x54) = FH_FV(0x002ae030)[FH_AI(self, 0x1c) * 3 + 1];
		FH_AF(self, 0x58) = FH_FV(0x002ae030)[FH_AI(self, 0x1c) * 3 + 2];
		FH_AF(self, 0x5c) = 1.0f;
	}
	return self;
}

/* 0x00135f70  (gap): loads the shark model */
void FoxHunt_LoadSharkModel(void)
{
	FH_P(0x002ae064) = (char *)bLoadResource(NULL, FH_P(0x002ae060), 1, 8);
	if (FH_P(0x002ae064) == NULL)
		bkPrintf("\t\t\t\t\t\t\t\t\t\t\t\t\tEEEeeeeeeeeekkkkk!!!!!\n");
	bkPrintf("Shark Model Loaded\n");
}

/* 0x00135fb0  (gap): puts the shark at the results place (-1400, 975, 2300), still.
Returns its list node. */
char *FoxHunt_Shark_Reset(void *self)
{
	FH_AF(FH_AP(self, 0xc), 0xd0) = -1400.0f;
	FH_AF(FH_AP(self, 0xc), 0xd8) = 2300.0f;
	FH_AF(FH_AP(self, 0xc), 0xd4) = 975.0f;
	FH_AF(FH_AP(self, 0xc), 0x194) = FH_AF(FH_AP(self, 0xc), 0x194) * 0.0f;
	FH_AF(FH_AP(self, 0xc), 0x198) = FH_AF(FH_AP(self, 0xc), 0x198) * 0.0f;
	FH_AF(FH_AP(self, 0xc), 0x19c) = FH_AF(FH_AP(self, 0xc), 0x19c) * 0.0f;
	FH_AU(FH_AP(self, 0xc), 0x1e8) = 0x405d2b0au;   /* 3.4558 */
	FH_AU(FH_AP(self, 0xc), 0x1d8) = 0x405d2b0au;
	return (char *)self + 4;
}

/* an angle difference wrapped to [-pi, pi] and clamped to +-limit */
static double turn(double d, float limit)
{
	if (d < 0.0)
		d = d + (double)6.28318548f;
	if ((double)6.28318548f < d)
		d = d - (double)6.28318548f;
	if ((double)3.1415925f < d)
		d = d - (double)6.28318548f;
	if ((double)limit < d)
		d = (double)limit;
	if (d < (double)-limit)
		d = (double)-limit;
	return d;
}

/* 0x00136040  (gap): the shark's frame (when it swims, +0x2c): turns to its next route
point (+0x50) and swims there, picking the next point when it arrives. Returns its list
node. */
char *FoxHunt_Shark_Update(void *self)
{
	char *a;
	float *target = FH_AV(self, 0x50);
	double d;
	float flat[4];
	float flat_target[4];
	int k;

	if (FH_AI(self, 0x2c) == 0)
		return (char *)self + 4;
	a = FH_AP(self, 0xc);
	FH_AF(a, 0x19c) = DAT_0028fda8 * FH_AF(self, 0x14) + FH_AF(a, 0x19c);
	d = (double)CLMath_HeadingTo(FH_AV(FH_AP(self, 0xc), 0xd0), target);
	FH_AF(self, 0x24) = (float)d;
	d = turn(d - (double)FH_AF(self, 0x28), FH_AF(self, 0x18));
	d = d + (double)FH_AF(self, 0x28);
	FH_AF(self, 0x28) = (float)d;
	FH_AF(self, 0x28) = CLMath_WrapAngle((float)d);
	bmVectorRotateXYZ(FH_AV(FH_AP(self, 0xc), 0x194), FH_AV(FH_AP(self, 0xc), 0x194), 0.0f, FH_AF(self, 0x28), 0.0f);
	a = FH_AP(self, 0xc);
	flat[0] = FH_AF(a, 0xd0);
	flat[2] = FH_AF(a, 0xd8);
	flat[3] = FH_AF(a, 0xdc);
	flat_target[0] = target[0];
	flat_target[2] = FH_AF(self, 0x58);
	flat_target[3] = FH_AF(self, 0x5c);
	flat[1] = 0.0f;
	flat_target[1] = 0.0f;
	if (bmVectorDistance(flat, flat_target) < FH_AF(self, 0x20)) {
		if (STAGE() == 3 || STAGE() == 4) {
			if (STAGE() == 3) {
				if (FH_AI(self, 0x1c) < 4)
					FH_AI(self, 0x1c)++;
				else
					FH_AI(self, 0x1c)--;
			} else {
				if (FH_AI(self, 0x1c) < 5)
					FH_AI(self, 0x1c)++;
				else
					FH_AI(self, 0x1c) = 4;
			}
			k = FH_AI(self, 0x1c) * 3;
			target[0] = FH_FV(0x002adfe8)[k];
			FH_AF(self, 0x54) = FH_FV(0x002adfe8)[k + 1];
			FH_AF(self, 0x58) = FH_FV(0x002adfe8)[k + 2];
		}
		if (STAGE() == 1) {
			if (FH_AI(self, 0x1c) < 3)
				FH_AI(self, 0x1c)++;
			else
				FH_AI(self, 0x1c) = 0;
			k = FH_AI(self, 0x1c) * 3;
			target[0] = FH_FV(0x002ae030)[k];
			FH_AF(self, 0x54) = FH_FV(0x002ae030)[k + 1];
			FH_AF(self, 0x58) = FH_FV(0x002ae030)[k + 2];
		}
	}
	a = FH_AP(self, 0xc);
	FH_AF(a, 0xd0) = FH_AF(a, 0x194) + FH_AF(a, 0xd0);
	a = FH_AP(self, 0xc);
	FH_AF(a, 0xd8) = FH_AF(a, 0x19c) + FH_AF(a, 0xd8);
	a = FH_AP(self, 0xc);
	FH_AF(a, 0xd4) = FH_AF(a, 0x198) + FH_AF(a, 0xd4);
	d = (double)FUN_001334f0(FH_AF(FH_AP(self, 0xc), 0x194), FH_AF(FH_AP(self, 0xc), 0x19c));
	a = FH_AP(self, 0xc);
	d = turn(d - (double)FH_AF(a, 0x1e8), FH_AF(self, 0x18));
	FH_AF(a, 0x1e8) = (float)(d + (double)FH_AF(a, 0x1e8));
	FH_AF(FH_AP(self, 0xc), 0x1e8) = CLMath_WrapAngle(FH_AF(FH_AP(self, 0xc), 0x1e8));
	FH_AF(FH_AP(self, 0xc), 0x194) = FH_AF(FH_AP(self, 0xc), 0x194) * 0.8f;
	FH_AF(FH_AP(self, 0xc), 0x198) = FH_AF(FH_AP(self, 0xc), 0x198) * 0.1f;
	FH_AF(FH_AP(self, 0xc), 0x19c) = FH_AF(FH_AP(self, 0xc), 0x19c) * 0.8f;
	if (FH_AF(FH_AP(self, 0xc), 0x19c) < 0.75f)
		FH_AF(FH_AP(self, 0xc), 0x19c) = DAT_0028fda8 * 0.75f;
	return (char *)self + 4;
}

/* 0x00136390  (gap): updates every shark */
void FoxHunt_UpdateSharks(void)
{
	char *node = fh_ListNext(FH_LIST(0x00302300), NULL);

	while (node != NULL)
		node = fh_ListNext(FH_LIST(0x00302300), FoxHunt_Shark_Update(node - 4));
}

/* 0x001363c0  (gap): puts every shark at the results place */
void FoxHunt_ResetSharks(void)
{
	char *node = fh_ListNext(FH_LIST(0x00302300), NULL);

	while (node != NULL)
		node = fh_ListNext(FH_LIST(0x00302300), FoxHunt_Shark_Reset(node - 4));
}

/* 0x001363f0  (not in the function table; called by the game mode's exit): frees every
shark */
void FUN_001363f0(void)
{
	char *node = fh_ListNext(FH_LIST(0x00302300), NULL);
	char *shark;
	char *next;

	if (node == NULL)
		return;
	do {
		shark = node - 4;
		next = fh_ListRemove(FH_LIST(0x00302300), shark + 4);
		bkPrintf("Deleted Shark no %d <--------------\n", FH_AI(shark, 0));
		bkHeapFree(shark);
		node = fh_ListNext(FH_LIST(0x00302300), next);
	} while (node != NULL);
}

/* ======================================================================== the flags */

/* 0x00136440  (gap): loads the flag models (4 pairs at 0x002ae0f0) */
void FoxHunt_LoadFlagModels(void)
{
	unsigned int a;

	for (a = 0x002ae0f4u; a < 0x002ae114u; a += 8)
		*(char **)FH_ADDR(a) = (char *)bLoadResource(NULL, *(char **)FH_ADDR(a - 4), 1, 0);
}

/* 0x00136470  (gap): drops flag `id` (state 2; Victim: 4) */
void FoxHunt_DropFlag(int id)
{
	char *f;

	for (f = fh_ListNext(FH_LIST(0x0030230c), NULL); f != NULL; f = fh_ListNext(FH_LIST(0x0030230c), f)) {
		if (FH_AI(f, 0xc) == id)
			FH_AI(f, 0x14) = (STAGE() == 4) * 2 + 2;
	}
}

/* 0x001364c0  (gap): resets every flag (state 3) and nobody has one */
void FoxHunt_ResetFlags(void)
{
	char *f;
	int i;

	for (f = fh_ListNext(FH_LIST(0x0030230c), NULL); f != NULL; f = fh_ListNext(FH_LIST(0x0030230c), f)) {
		FH_AI(f, 0x14) = 3;
		for (i = 0; i < DAT_002bae94; i++) {
			FH_AI(fh_Players[i], 0x11c8) = -1;
			FH_AI(fh_Players[i], 0x11c4) = 0;
		}
	}
}

/* 0x00136530  (gap): resets flag `id` (state 3) */
void FoxHunt_ResetFlag(int id)
{
	char *f;

	for (f = fh_ListNext(FH_LIST(0x0030230c), NULL); f != NULL; f = fh_ListNext(FH_LIST(0x0030230c), f)) {
		if (FH_AI(f, 0xc) == id)
			FH_AI(f, 0x14) = 3;
	}
}
