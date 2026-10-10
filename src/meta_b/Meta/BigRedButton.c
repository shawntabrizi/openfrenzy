/*
Meta/BigRedButton.c  <-  C:\BlitzParty\Game\Src\Meta\BigRedButton.cpp

Big red buttons on the Meta board: up to 16 (0x00309060), each on a model with a
"brb_bone" bone and "on"/"off" animations and a flashing light texture. A player standing
near the button and facing it presses it; the level script finds buttons by tag.

0x00161d80 is tagged with BigRedButton.cpp; the functions around it work on the same
array (file inferred).

States (+0x3c): 0 up, 1 down, 2 going down, 3 going up, 4 done.
Modes (+0x34): 0 stays down until released, 1 pops up again, 2 once only.
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x002b0be8  the button's animations */
struct MbAnimName DAT_002b0be8[2] = {
	{ "on", 1.0f },
	{ "off", 1.0f },
};

/* 0x00309058  number of buttons */
int DAT_00309058;
/* 0x0030905c  buttons that are down (or moving) */
int DAT_0030905c;
/* 0x00309060  the buttons */
struct CBigRedButton *DAT_00309060[16];

/* 0x002bae70  the game's difficulty or handicap (0..2; core_modes_a) */
extern int DAT_002bae70;

/* 0x00161c00  Meta (file inferred: BigRedButton.cpp)
Clears the button array. */
void BigRedButton_Init(void)
{
	int i;

	for (i = 0; i < 16; i++)
		DAT_00309060[i] = NULL;
	DAT_00309058 = 0;
	DAT_0030905c = 0;
}

/* 0x00161c20  Meta (file inferred: BigRedButton.cpp)
The index of the button with `tag` (the model's tag at +0x3d0), or -1. */
int BigRedButton_FindTag(int tag)
{
	int i;

	for (i = 0; i < DAT_00309058; i++)
	{
		if (MB_I32(DAT_00309060[i]->model, 0x3d0) == tag)
			return i;
	}
	bkPrintf("Couldn't find BRB with tag %d.\nPerhaps the puzzle is before the BRB in the LND.\n"
		" It shouldn't be!\n", tag);
	return -1;
}

/* 0x00161c70  Meta (file inferred: BigRedButton.cpp)
The first enabled button that can be pressed now, or NULL. */
struct CBigRedButton *BigRedButton_FindPressable(void)
{
	struct CBigRedButton *b;
	int i;

	for (i = 0; i < DAT_00309058; i++)
	{
		b = DAT_00309060[i];
		if (((b->state == 1 && b->mode == 0) || b->state == 0) && b->enabled != 0)
			return DAT_00309060[i];
	}
	return NULL;
}

/* 0x00161cc0  Meta (file inferred: BigRedButton.cpp)
Releases the button: plays "off" and goes up. Returns 1. */
int CBigRedButton_Release(struct CBigRedButton *self)
{
	FUN_000af690((int)MB_ADDR(self->model, 0x10), 1, 0, 0, 1.0f, 0.0f, 1);
	self->state = 3;
	DAT_0030905c++;
	return 1;
}

/* 0x00161d00  Meta (file inferred: BigRedButton.cpp)
Resets the press count of the button with `tag`. */
void BigRedButton_ClearPresses(int tag)
{
	int i = BigRedButton_FindTag(tag);

	DAT_00309060[i]->presses = 0;
}

/* 0x00161d20  Meta (file inferred: BigRedButton.cpp)
Enables or disables the button with `tag`; a disabled button's light stops. */
void BigRedButton_Enable(int tag, int enabled)
{
	int i = BigRedButton_FindTag(tag);
	struct CBigRedButton *b;

	if (i < 0)
		return;
	b = DAT_00309060[i];
	if (b == NULL)
		return;
	b->enabled = enabled;
	MB_F32(b->model, 0x344) = (float)enabled;
	if (b->enabled == 0)
		AnimTex_SetFrameList((struct CLFList *)MB_ADDR(b->model, 0x338), 0);
}

/* 0x00161d80  Meta/BigRedButton.cpp
Constructor: a button on `model` (its bone gives the position and heading), pressed from
within `radius` and facing it within `tolerance`. Adds it to the array. */
struct CBigRedButton *CBigRedButton_Construct(struct CBigRedButton *self, void *model,
	int mode, float radius, float tolerance, int enabled)
{
	struct BActorNode *bone;
	void *anim;
	int i;

	DAT_00309060[DAT_00309058] = self;
	self->radius2 = radius * radius;
	self->tolerance = tolerance;
	self->model = model;
	self->state = 0;
	self->prevState = 0;
	self->mode = mode;
	self->presses = 0;
	self->enabled = enabled;
	bone = baFindNode((struct BActorNode *)MB_PTR(model, 0x154), "brb_bone");
	self->bone = bone;
	if (bone == NULL)
	{
		bkPrintf("couldn't find brb bone\n");
	}
	else
	{
		baGetNodePosition((struct BActorInstance *)MB_ADDR(model, 0x20), bone, self->pos);
		self->heading = CLMath_HeadingOf(MB_FP(self->bone, 0x30));
	}
	MB_PTR(model, 0x334) = DAT_002b0be8;
	for (i = 0; i < 2; i++)
		MB_PTR(self->model, 0x274 + i * 4) = baFindSubObject(
			(struct BActorInstance *)MB_ADDR(model, 0x20), DAT_002b0be8[i].name);
	DAT_00309058++;
	anim = MB_NEW(0x18, "C:\\BlitzParty\\Game\\Src\\Meta\\BigRedButton.cpp", 0x10d);
	if (anim != NULL)
		AnimTex_Init((struct CAnimTex *)anim, (char *)"brblight.gif", 20.0f,
			(struct CLFList *)MB_ADDR(self->model, 0x338));
	MB_F32(self->model, 0x344) = (float)self->enabled;
	return self;
}

/* 0x00161ed0  Meta (file inferred: BigRedButton.cpp)
Per frame: the button's state machine (counts moving buttons in DAT_0030905c). Returns
1 while the button is moving. */
int CBigRedButton_Update(struct CBigRedButton *self)
{
	switch (self->state)
	{
	case 0:
		if ((self->mode == 0 || self->mode == 1) && self->prevState != 0)
		{
			self->prevState = 0;
			DAT_0030905c--;
		}
		break;
	case 1:
		if (self->mode == 0 && self->prevState != 1)
		{
			self->prevState = 1;
			DAT_0030905c--;
		}
		if (self->mode == 1)
		{
			self->prevState = self->state;
			DAT_0030905c--;
			CBigRedButton_Release(self);
		}
		if (self->mode == 2)
		{
			self->prevState = self->state;
			DAT_0030905c--;
			self->state = 4;
			return 0;
		}
		break;
	case 2:
		FUN_000b3e40((int)self->model, 0);
		if (MB_I16(self->model, 0x244) == 0)
			return 1;
		self->prevState = self->state;
		self->state = 1;
		self->presses++;
		return 1;
	case 3:
		FUN_000b3e40((int)self->model, 0);
		if (MB_I16(self->model, 0x244) != 0)
		{
			self->prevState = self->state;
			self->state = 0;
		}
		return 1;
	}
	return 0;
}

/* 0x00161fd0  Meta (file inferred: BigRedButton.cpp)
1 when `character` is within the button's radius and faces it. */
int CBigRedButton_CanPress(struct CBigRedButton *self, void *character)
{
	float dx;
	float dy;
	float dz;

	baGetNodePosition((struct BActorInstance *)MB_ADDR(self->model, 0x20), self->bone,
		self->pos);
	dx = MB_F32(character, 0xd0) - self->pos[0];
	dy = MB_F32(character, 0xd4) - self->pos[1];
	dz = MB_F32(character, 0xd8) - self->pos[2];
	if (dz * dz + dy * dy + dx * dx < self->radius2)
	{
		if (CLMath_AngleDistance(self->heading, MB_F32(character, 0x1e8)) < self->tolerance)
			return 1;
		bkPrintf("PLAYER facing wrong way %f %f\n", (double)self->heading,
			(double)MB_F32(character, 0x1e8));
	}
	return 0;
}

/* 0x00162080  Meta (file inferred: BigRedButton.cpp)
Presses the button: each player gets a random reaction (cheer or not, by the handicap),
the "on" animation and a sound. Returns 1 (0 when disabled). */
int CBigRedButton_Press(struct CBigRedButton *self)
{
	int i;
	int player;

	if (self->enabled == 0)
		return 0;
	for (i = 0; i < 4; i++)
	{
		player = DAT_003075bc[i];
		if (mb_rand() % 100 < DAT_002bae70 * 0x32)
		{
			MB_F32(player, 0x1130) = 240.0f;
			if (MB_I32(player, 4) == 0xb)
				MB_VFN(player, 0x18, void (*)(int, int))(player, -1);
		}
		else
		{
			MB_F32(player, 0x1130) = -60.0f;
		}
	}
	FUN_000af690((int)MB_ADDR(self->model, 0x10), 0, 0, 0, 1.0f, 0.0f, 1);
	CLSamples_PlayGeneric(4, 0x7f, 0, 0, NULL);
	self->state = 2;
	DAT_0030905c++;
	return 1;
}

/* 0x00162150  Meta (file inferred: BigRedButton.cpp)
Toggles an enabled button: down -> release, up -> press. Returns 1 when it did. */
int CBigRedButton_Toggle(struct CBigRedButton *self)
{
	if (self->enabled != 0)
	{
		if (self->state == 1)
		{
			CBigRedButton_Release(self);
			return 1;
		}
		if (self->state == 0)
		{
			CBigRedButton_Press(self);
			return 1;
		}
	}
	return 0;
}

/* 0x00162180  Meta (file inferred: BigRedButton.cpp)
Deletes every button. */
void BigRedButton_DeleteAll(void)
{
	int i;

	for (i = 0; i < DAT_00309058; i++)
	{
		if (DAT_00309060[i] != NULL)
			MB_DELETE(DAT_00309060[i]);
	}
	for (i = 0; i < 16; i++)
		DAT_00309060[i] = NULL;
	DAT_00309058 = 0;
	DAT_0030905c = 0;
}

/* 0x001621d0  Meta (file inferred: BigRedButton.cpp)
The first enabled button that `character` can press now, or NULL. */
struct CBigRedButton *BigRedButton_FindPressableBy(void *character)
{
	struct CBigRedButton *b;
	int i;

	for (i = 0; i < DAT_00309058; i++)
	{
		b = DAT_00309060[i];
		if (((b->state == 1 && b->mode == 0) || b->state == 0) && b->enabled != 0 &&
			CBigRedButton_CanPress(b, character) != 0)
			return DAT_00309060[i];
	}
	return NULL;
}

/* 0x00162230  Meta (file inferred: BigRedButton.cpp)
Updates every button. */
void BigRedButton_UpdateAll(void)
{
	int i;

	for (i = 0; i < DAT_00309058; i++)
		CBigRedButton_Update(DAT_00309060[i]);
}
