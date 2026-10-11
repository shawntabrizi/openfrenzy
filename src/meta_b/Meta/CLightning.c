/*
Meta/CLightning.c  <-  C:\BlitzParty\Game\Src\Meta\CLightning.cpp

Lightning on the Meta board: a manager (the global at 0x00308460) holds up to 8 groups
of bolts, each group a ClassLib list. The level script adds bolts to groups; a timer
strikes a random bolt of one group, and meta_a's bolt functions (0x0015ca40 start,
0x0015cb80 regenerate, 0x0015cc80 draw) do the rest. Also the spiral-beam lightning
strike on a player (0x0015cef0).

0x0015cef0, 0x0015d090 are tagged with CLightning.cpp, 0x0015d010 is bracketed; the
other functions of this file are methods of the same manager (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x00308458  number of bolts that are alive (draw is skipped when 0) */
short DAT_00308458;
/* 0x00308460  the lightning manager */
struct CLightningMgr DAT_00308460;

/* 0x0015cef0  Meta/CLightning.cpp
A lightning strike from `from` to `to` (float[4] each): a white spiral beam with both
ends raised by 50, and a fading green light (charlib, 0xe8 bytes) where it strikes. */
void CLightning_Strike(float *from, float *to)
{
	float top[4];
	float bottom[4];
	int colour[3];
	int black[3];
	void *light;

	top[0] = from[0];
	top[1] = from[1] + 50.0f;       /* 0x002559b8 */
	top[2] = from[2];
	top[3] = from[3];
	bottom[0] = to[0];
	bottom[1] = to[1] + 50.0f;
	bottom[2] = to[2];
	bottom[3] = to[3];
	colour[0] = 0;
	colour[1] = 0x7f;
	colour[2] = 0x46;
	black[0] = 0;
	black[1] = 0;
	black[2] = 0;
	CProjector_SpiralBeam(top, bottom, DAT_002b0a4c, DAT_002b0a54, DAT_002b0a50, DAT_002b0a58,
		(struct CProjSpiralType *)DAT_00290dc8, 10.0f, 1, 0xffffffffu);
	light = MB_NEW(0xe8, "C:\\BlitzParty\\Game\\Src\\Meta\\CLightning.cpp", 0x29f);
	if (light != NULL)
	{
		/* charlib Light_InitFadingPoint: a light at the bottom fading from (0, 127, 70) to
		   black (ranges 0.0008, 0.008, 80 frames, no delay) */
		MB_DUAL(FUN_000b6fa0,
			int (*)(int, undefined4 *, undefined4 *, undefined4 *, undefined4, undefined4,
				undefined4, undefined4),
			void *(*)(void *, int *, int *, float *, float, float, float, float),
			((int)light, (undefined4 *)colour, (undefined4 *)black, (undefined4 *)bottom,
				0x3a51b717u, 0x3c03126fu, 0x42a00000u, 0),
			(light, colour, black, bottom, 0.0008f, 0.008f, 80.0f, 0.0f));
	}
}

/* 0x0015d010  Meta/CLightning.cpp (bracketed)
Deletes every bolt of every group and resets the manager. */
void CLightningMgr_Reset(struct CLightningMgr *self)
{
	int group;
	void *bolt;
	void *next;

	for (group = 0; group < (int)self->numGroups; group++)
	{
		bolt = MB_LIST_NEXT(&self->groups[group], NULL);
		while (bolt != NULL)
		{
			next = MB_LIST_REMOVE(&self->groups[group], bolt);
			if (bolt != NULL)
				MB_DELETE(bolt);
			bolt = MB_LIST_NEXT(&self->groups[group], next);
		}
	}
	self->numGroups = 0;
	self->groupIds[0] = 0;
	DAT_00308458 = 0;
	self->strikeTime = 0.0f;
	self->strikeInterval = 0.0f;
	self->strikeAccum = 0.0f;
}

/* 0x0015d090  Meta/CLightning.cpp
Adds a bolt (0x48 bytes) to the group with id `groupId` (a new group when it is not
there yet). `pos` is float[4]; `arg` is the LIGHTNING line's 4th number; `tag` names
the bolt for the level script. */
void CLightningMgr_AddBolt(struct CLightningMgr *self, float *pos, float arg, short tag,
	unsigned short groupId)
{
	unsigned short count = self->numGroups;
	int group = 0;
	struct CLightningBolt *bolt;

	if (count != 0)
	{
		do
		{
			if ((int)groupId == (int)self->groupIds[group])
				break;
			group++;
		} while (group < (int)self->numGroups);
		if (group > 7)
		{
			bkPrintf("TOO many lighting groups %d!\n", group);
			return;
		}
	}
	if (group == (int)count)
		self->numGroups = (unsigned short)(count + 1);
	bolt = (struct CLightningBolt *)MB_NEW(0x48, "C:\\BlitzParty\\Game\\Src\\Meta\\CLightning.cpp",
		0x82);
	if (bolt == NULL)
	{
		bolt = NULL;
	}
	else
	{
		bolt->pos[0] = pos[0];
		bolt->pos[1] = pos[1];
		bolt->pos[2] = pos[2];
		bolt->pos[3] = pos[3];
		bolt->arg = arg;
		bolt->tag = tag;
		bolt->life = 0.0f;
	}
	MB_LIST_INSERT(&self->groups[group], bolt, 0);
	self->groupIds[group] = (short)groupId;
}

/* 0x0015d160  Meta (file inferred: CLightning.cpp)
Strikes a bolt of group `groupId`: the bolt with `tag`, or a random one when tag is 0. */
void CLightningMgr_StrikeGroup(struct CLightningMgr *self, short tag, unsigned short groupId)
{
	int count = self->numGroups;
	int found = -1;
	int i;
	int n;
	struct CLightningBolt *bolt;

	if (count > 0)
	{
		for (i = 0; i < count; i++)
		{
			if ((int)self->groupIds[i] == (int)groupId)
				found = i;
		}
		if (found >= 0)
		{
			if (tag != 0)
			{
				for (bolt = MB_LIST_NEXT(&self->groups[found], NULL); bolt != NULL;
					bolt = MB_LIST_NEXT(&self->groups[found], bolt))
				{
					if (bolt->tag == tag)
					{
						FUN_0015ca40((int)bolt);
						return;
					}
				}
			}
			else if (self->groups[found].count != 0)
			{
				n = mb_rand() % self->groups[found].count + 1;
				if (n < 1)
					return;
				bolt = NULL;
				do
				{
					bolt = MB_LIST_NEXT(&self->groups[found], bolt);
					n--;
				} while (n != 0);
				if (bolt == NULL)
					return;
				FUN_0015ca40((int)bolt);
				return;
			}
			bkPrintf("Couldn't find Lightning tag %d group%d\n", (int)(unsigned short)tag,
				(int)groupId);
			return;
		}
	}
	bkPrintf("bad lightning group %d\n", (int)groupId);
}

/* 0x0015d260  Meta (file inferred: CLightning.cpp)
Counts a bolt's life down; sets its brightness (+0x3c): fading in over the first 4
frames (life 22..18, regenerated each frame), then fading out. Returns the bolt. */
struct CLightningBolt *CLightningBolt_Update(struct CLightningBolt *self)
{
	float life = self->life - DAT_0028fda8;
	float b;

	self->life = life;
	/* strictly below 0 (x86: fcomp; test ah,5; jp): a life of exactly 0 takes the fade
	   branch below and leaves the live count alone */
	if (life < 0.0f)
	{
		self->life = 0.0f;
		DAT_00308458--;
		return self;
	}
	if (life >= 18.0f)                              /* 0x0026872c */
	{
		FUN_0015cb80((int)self);
		b = (22.0f - self->life) * 0.25f;          /* 0x0026c2f8, 0x00257b88 */
		self->brightness = b;
		self->brightness = b * b;
		return self;
	}
	b = life * 0.055555556f;                        /* 0x00268a0c: 1/18 */
	self->brightness = b;
	self->brightness = b * b;
	return self;
}

/* 0x0015d300  Meta (file inferred: CLightning.cpp)
The automatic strike timer: every strikeInterval frames a random bolt of strikeGroup. */
void CLightningMgr_UpdateTimer(struct CLightningMgr *self)
{
	self->strikeAccum = DAT_0028fda8 + self->strikeAccum;
	while (self->strikeAccum > self->strikeInterval)
	{
		self->strikeAccum = self->strikeAccum - self->strikeInterval;
		CLightningMgr_StrikeGroup(self, 0, self->strikeGroup);
	}
	self->strikeTime = self->strikeTime - DAT_0028fda8;
}

/* 0x0015d360  Meta (file inferred: CLightning.cpp)
Draws every live bolt (additive, no texture, no culling). */
void CLightningMgr_Draw(struct CLightningMgr *self)
{
	int group;
	struct CLightningBolt *bolt;

	if (DAT_00308458 == 0)
		return;
	bdSetTexture(0, NULL);
	MB_SET_RS(3, 0);
	FUN_00043b70(1);
	MB_SET_RS(6, 1);
	MB_SET_RS(10, 1);
	MB_SET_RS(7, 1);
	for (group = 0; group < (int)self->numGroups; group++)
	{
		for (bolt = MB_LIST_NEXT(&self->groups[group], NULL); bolt != NULL;
			bolt = MB_LIST_NEXT(&self->groups[group], bolt))
		{
			if (bolt->life > 0.0f)
				FUN_0015cc80(bolt->start, bolt->end, 0xc, bolt->brightness);
		}
	}
	MB_SET_RS(3, MB_U32(DAT_003055d0, 0x48));
	MB_SET_RS(10, 0);
	MB_SET_RS(7, 3);
	MB_SET_RS(6, 0);
}

/* 0x0015d440  Meta (file inferred: CLightning.cpp)
Per frame: the strike timer while strikeTime > 0, then every live bolt. */
void CLightningMgr_Update(struct CLightningMgr *self)
{
	int group;
	struct CLightningBolt *bolt;

	if (self->strikeTime > 0.0f)
		CLightningMgr_UpdateTimer(self);
	if (DAT_00308458 == 0 || self->numGroups == 0)
		return;
	for (group = 0; group < (int)self->numGroups; group++)
	{
		for (bolt = MB_LIST_NEXT(&self->groups[group], NULL); bolt != NULL;
			bolt = MB_LIST_NEXT(&self->groups[group], bolt))
		{
			if (bolt->life > 0.0f)
				CLightningBolt_Update(bolt);
		}
	}
}
