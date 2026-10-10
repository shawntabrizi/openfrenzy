/*
Meta/CBomb.c  <-  C:\BlitzParty\Game\Src\Meta\CBomb.cpp

Bombs on the Meta board: five kinds (0 bomb, 1 bomb with a fuse, 2 mine, 3 missile of an
air raid, 4 dynamite) as charlib actors with physics, an explosion each, bomb groups of
level positions that drop bombs at intervals, and air raids that drop missiles over an
area. Lists: bombs 0x00308f48, air raids 0x00308f3c, groups at 0x00308f58 (8 at most).

Tagged with CBomb.cpp: 0x00160680, 0x001608e0, 0x00160bf0, 0x00160cb0, 0x00161250; the
functions between them are bracketed. 0x00161460-0x00161b80 (gap) work on the same lists
(file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* the Meta .data block (meta_a's copy): 0x002b0a90 the bomb models by kind ("bomb.obe",
   "bomb2.obe", "mine3.obe", "missile.obe", "dynamite.obe"; DAT_002b0a90 in meta_b_internal.h),
   0x002b0ab8 "pulse" (the mine's flashing sub-object), and the bomb tweakables: */
extern char s_pulse_002b0ab8[];
#define DAT_002b0ab8 s_pulse_002b0ab8
#define DAT_002b0bc8 MB_META(0x002b0bc8, int)    /* 5: dynamite streaks per frame */
#define DAT_002b0bcc MB_META(0x002b0bcc, float)  /* 5.0: dynamite streak speed base */
#define DAT_002b0bd0 MB_META(0x002b0bd0, int)    /* 2: dynamite streak speed range */
#define DAT_002b0bd4 MB_META(0x002b0bd4, float)  /* 100.0: dynamite streak height */
#define DAT_002b0bd8 MB_META(0x002b0bd8, float)  /* 0.7: streak height per fuse frame */
#define DAT_002b0bdc MB_META(0x002b0bdc, float)  /* 300.0: explosion damage at the centre */
#define DAT_002b0be0 MB_META(0x002b0be0, float)  /* 50.0: explosion player height offset */

/* 0x00308f38  number of bomb groups */
int DAT_00308f38;
/* 0x00308f3c  the air raids */
struct MbList DAT_00308f3c;
/* 0x00308f48  the bombs */
struct MbList DAT_00308f48;
/* 0x00308f58  the bomb groups */
struct MbBombGroup DAT_00308f58[8];

/* 0x002b1ec8 "REDDOT", 0x002b215c "STREAK3": particle types (the particle system's data) */
extern char DAT_002b1ec8[];
extern char DAT_002b215c[];

/* 0x00160680  Meta/CBomb.cpp
Gives an actor the physics blocks a falling bomb needs. */
void CBomb_SetupPhysics(void *actor)
{
	void *block;
	float *mass;

	MB_U32(actor, 0x220) = MB_U32(actor, 0x220) | 8;
	block = bkHeapCalloc(0x204, 0, "C:\\BlitzParty\\Game\\Src\\Meta\\CBomb.cpp", 0xd0, 0x2001);
	MB_PTR(actor, 0x18c) = block;
	MB_F32(block, 0x9c) = 80.0f;
	MB_F32(MB_PTR(actor, 0x18c), 0x90) = 0.0f;
	MB_F32(MB_PTR(actor, 0x18c), 0x80) = 80.0f;
	MB_F32(MB_PTR(actor, 0x18c), 0xa0) = 80.0f;
	MB_U8(MB_PTR(actor, 0x18c), 0x15d) = 1;
	mass = (float *)bkHeapCalloc(0x40, 0, "C:\\BlitzParty\\Game\\Src\\Meta\\CBomb.cpp", 0xd7, 0x2001);
	MB_PTR(actor, 0x188) = mass;
	mass[0] = 400.0f;
}

/* 0x00160720  Meta/CBomb.cpp (bracketed)
Loads the bomb models; no bomb groups yet. */
void CBomb_Init(void)
{
	struct MbBombModel *model;

	DAT_00308f38 = 0;
	for (model = DAT_002b0a90; model < DAT_002b0a90 + 5; model++)
		model->resource = bLoadResource(NULL, model->name, 1, 0);
}

/* 0x00160760  Meta/CBomb.cpp (bracketed)
Lights the fuse of the unlit bombs with `tag` (-2: every bomb, -1: every bomb but
missiles). */
void CBomb_LightFuses(int tag)
{
	struct CBomb *bomb;

	for (bomb = MB_LIST_NEXT(&DAT_00308f48, NULL); bomb != NULL;
		bomb = MB_LIST_NEXT(&DAT_00308f48, bomb))
	{
		if ((bomb->tag == tag || tag == -2 || (tag == -1 && bomb->type != 3)) &&
			mb_fbits(bomb->timer) == 0xbf800000u)
		{
			bomb->timer = bomb->fuse;
			MB_F32(bomb->actor, 0x334) = 1.0f;
			FUN_000ae820((int)bomb->actor, 6.0f, bomb->fuse);
		}
	}
}

/* 0x001607e0  Meta/CBomb.cpp (bracketed)
The bomb with `tag`, or NULL. */
struct CBomb *CBomb_Find(int tag)
{
	struct CBomb *bomb;

	for (bomb = MB_LIST_NEXT(&DAT_00308f48, NULL); bomb != NULL;
		bomb = MB_LIST_NEXT(&DAT_00308f48, bomb))
	{
		if (bomb->tag == tag)
			return bomb;
	}
	return NULL;
}

/* 0x00160810  Meta/CBomb.cpp (bracketed)
Air raid constructor: for `life` frames, a missile every `interval` frames somewhere in
the rectangle from `from` to `to` (x, z), or in the rectangle grown by `margin`; the
missiles fly with velocity `vel` (x, z). */
struct CAirRaid *CAirRaid_Construct(struct CAirRaid *self, float life, float interval,
	float *from, float *to, float margin, float *vel)
{
	bkPrintf("StartAirRaid()\n");
	self->x = from[0];
	self->width = to[0] - from[0];
	self->velX = vel[0];
	self->z = from[1];
	self->depth = to[1] - from[1];
	self->life = life;
	self->velZ = vel[1];
	self->interval = interval;
	self->outerX = self->x - margin;
	self->accum = 0.0f;
	self->outerZ = self->z - margin;
	self->outerWidth = margin + margin + self->width;
	self->outerDepth = margin + margin + self->depth;
	MB_LIST_INSERT(&DAT_00308f3c, self, 0);
	return self;
}

/* 0x001608b0  Meta/CBomb.cpp (bracketed)
Bomb group constructor (the static array at 0x00308f58). */
struct MbBombGroup *MbBombGroup_Construct(struct MbBombGroup *self)
{
	CLList_Init((int *)&self->positions);
	self->count = 0;
	self->type = 0;
	self->interval = 0.0f;
	self->accum = 0.0f;
	return self;
}

/* 0x001608d0  Meta/CBomb.cpp (bracketed)
An empty function (a jump to 0x001d5950) that the linker shares: the bomb group
destructor and empty shutdown functions of BACTOR. */
void Meta_Nothing(void)
{
	FUN_001d5950();
}

/* 0x001608e0  Meta/CBomb.cpp
Bomb constructor: a bomb of `kind` at `pos` with `tag`: its actor, its explosion and the
kind's set-up. */
struct CBomb *CBomb_Construct(struct CBomb *self, int kind, float *pos, int tag)
{
	void *actor;
	void *anim;
	void *trail;
	float ground;

	self->type = kind;
	self->tag = tag;
	self->bombPos = NULL;
	actor = (void *)((int (*)(void *, float *, int, int, int, int))FUN_000af1a0)(
		DAT_002b0a90[kind].resource, pos, 0, 6, 0, 1);
	self->actor = actor;
	MB_F32(actor, 0xd0) = pos[0];
	MB_F32(actor, 0xd4) = pos[1];
	MB_F32(actor, 0xd8) = pos[2];
	MB_F32(actor, 0xdc) = pos[3];
	MB_LIST_INSERT(&DAT_00308f48, self, 0);
	self->pushScale = 12.0f;
	self->radius = 320.0f;
	self->maxPush = 90.0f;
	self->triggerDist = 40000.0f;
	self->fuse = 60.0f;
	self->unk28 = 20.0f;
	self->dropAccum = 0.0f;
	self->dropType = NULL;
	self->dropInterval = 0.0f;
	self->numTrails = 0;
	self->trails[0] = NULL;
	MB_I32(self->actor, 0x220) = 1;
	actor = self->actor;
	ground = FUN_000b3ff0(MB_FP(actor, 0xd0), 0.0f, NULL, NULL, 0);
	self->explosion = NULL;
	self->height = MB_F32(actor, 0xd4) - ground;
	self->explosion = CMetaExplosion_Create();
	switch (self->type)
	{
	case 0:
		CBomb_SetupPhysics(self->actor);
		*(float *)MB_PTR(self->actor, 0x188) = 1000000.0f;
		self->timer = -1.0f;
		CMetaExplosion_Setup(self->explosion, 0x1e, 0x14, 0xc);
		break;
	case 1:
		CMetaExplosion_Setup(self->explosion, 0x1e, 0x14, 0xc);
		CBomb_SetupPhysics(self->actor);
		self->timer = self->fuse;
		break;
	case 2:
		CMetaExplosion_Setup(self->explosion, 0x1e, 0x14, 0xc);
		anim = MB_NEW(0x18, "C:\\BlitzParty\\Game\\Src\\Meta\\CBomb.cpp", 0x8b);
		if (anim != NULL)
			AnimTex_Init((struct CAnimTex *)anim, (char *)"mineflash.gif", 100.0f,
				(struct CLFList *)MB_ADDR(self->actor, 0x328));
		self->fuse = 180.0f;
		MB_PTR(self->actor, 0x324) = DAT_002b0ab8;
		MB_PTR(self->actor, 0x264) = baFindSubObject(
			(struct BActorInstance *)MB_ADDR(self->actor, 0x10), "pulse");
		FUN_000af690((int)self->actor, 0, 1, 0, 1.0f, 0.0f, 1);
		self->timer = -1.0f;
		break;
	case 3:
		CMetaExplosion_Setup(self->explosion, 0x1e, 0x14, 0xc);
		self->dropType = DAT_002b1ec8;
		self->dropAccum = 0.0f;
		self->dropInterval = 0.5f;
		trail = MB_NEW(0x584c, "C:\\BlitzParty\\Game\\Src\\Meta\\CBomb.cpp", 0xb7);
		if (trail == NULL)
			trail = NULL;
		else
			trail = CProjTrail_Init((struct CProjTrail *)trail, DAT_00290da4, 0xa0,
				DAT_00290d80, DAT_00290d90, 30.0f, 1.0f, 0);
		self->trails[0] = trail;
		self->numTrails = 1;
		self->fuse = 0.0f;
		self->timer = -1.0f;
		MB_U32(self->actor, 0x220) = MB_U32(self->actor, 0x220) | 2;
		break;
	case 4:
		CBomb_SetupPhysics(self->actor);
		*(float *)MB_PTR(self->actor, 0x188) = 10.0f;
		self->timer = 200.0f;
		CMetaExplosion_Setup(self->explosion, 0x1e, 0x14, 0xc);
		break;
	}
	return self;
}

/* 0x00160bd0  Meta/CBomb.cpp (bracketed)
Walks the bomb list and does nothing (an emptied debug function). */
void CBomb_DebugAll(void)
{
	void *bomb;

	for (bomb = MB_LIST_NEXT(&DAT_00308f48, NULL); bomb != NULL;
		bomb = MB_LIST_NEXT(&DAT_00308f48, bomb))
	{
	}
}

/* 0x00160bf0  Meta/CBomb.cpp
Adds a bomb position (`pos`, `tag`) to the group with id `groupId` (8 groups at most). */
void MbBombGroup_AddPosition(float *pos, short tag, unsigned int groupId)
{
	int group = 0;
	struct MbBombPos *bp;

	if (DAT_00308f38 > 0)
	{
		do
		{
			if ((unsigned int)DAT_00308f58[group].id == groupId)
				break;
			group++;
		} while (group < DAT_00308f38);
		if (group > 7)
		{
			bkPrintf("ERROR?? TOO MANY BOMB GROUPS see DJH\n");
			return;
		}
	}
	bp = (struct MbBombPos *)MB_NEW(0x20, "C:\\BlitzParty\\Game\\Src\\Meta\\CBomb.cpp", 0x3f5);
	if (bp == NULL)
	{
		bp = NULL;
	}
	else
	{
		bp->pos[0] = pos[0];
		bp->pos[1] = pos[1];
		bp->pos[2] = pos[2];
		bp->pos[3] = pos[3];
		bp->tag = tag;
		bp->used = 0;
		bp->blocker = NULL;
	}
	MB_LIST_INSERT(&DAT_00308f58[group].positions, bp, 0);
	if (group == DAT_00308f38)
	{
		DAT_00308f38++;
		DAT_00308f58[group].id = (unsigned short)groupId;
	}
}

/* 0x00160cb0  Meta/CBomb.cpp
Drops a bomb of `kind` at this position and marks it used. */
struct CBomb *MbBombPos_Drop(struct MbBombPos *self, int kind)
{
	struct CBomb *bomb;

	bomb = (struct CBomb *)MB_NEW(0x5c, "C:\\BlitzParty\\Game\\Src\\Meta\\CBomb.cpp", 0x476);
	if (bomb == NULL)
		bomb = NULL;
	else
		bomb = CBomb_Construct(bomb, kind, self->pos, (int)self->tag);
	self->used = 1;
	/* as in the original, a failed allocation writes through NULL */
	bomb->bombPos = self;
	if (self->blocker != NULL && *self->blocker != 0)
		bkPrintf("!\n");
	return bomb;
}

/* 0x00160d50  Meta/CBomb.cpp (bracketed)
Sets the group with id `groupId` dropping `count` bombs of `kind`, one per `interval`
frames. Returns 0 when there is no such group. */
int MbBombGroup_Start(int kind, unsigned int groupId, int count, float interval)
{
	int group;

	for (group = 0; group < DAT_00308f38; group++)
	{
		if ((unsigned int)DAT_00308f58[group].id == groupId)
		{
			DAT_00308f58[group].count = count;
			DAT_00308f58[group].type = kind;
			DAT_00308f58[group].interval = interval;
			DAT_00308f58[group].accum = interval;
			return 1;
		}
	}
	return 0;
}

/* 0x00160db0  Meta/CBomb.cpp (bracketed)
Drops a bomb at a random free position of the group (the next free one after a random
start). */
struct CBomb *MbBombGroup_DropOne(struct MbBombGroup *self)
{
	struct MbBombPos *start;
	struct MbBombPos *bp;
	int n;

	if (self->positions.count == 0)
	{
		bkPrintf("NO bombPos in group !!!\n");
		return NULL;
	}
	n = (int)((unsigned int)mb_rand() % (unsigned int)self->positions.count) + 1;
	start = NULL;
	for (; n > 0; n--)
		start = MB_LIST_NEXT(&self->positions, start);
	bp = start;
	if (bp->used == 0 && (bp->blocker == NULL || *bp->blocker == 0))
		return MbBombPos_Drop(bp, self->type);
	for (;;)
	{
		do
		{
			bp = MB_LIST_NEXT(&self->positions, bp);
			if (bp == NULL)
				bp = MB_LIST_NEXT(&self->positions, NULL);
			if (bp == start)
			{
				bkPrintf("bombPos's are FULL UP for this group\n");
				return NULL;
			}
		} while (bp->used != 0);
		if (bp->blocker == NULL || *bp->blocker == 0)
			break;
	}
	return MbBombPos_Drop(bp, self->type);
}

/* 0x00160e70  Meta (file inferred: CBomb.cpp)
Deletes a bomb: players stop looking at it, its actor goes, its position is free again.
Returns the list predecessor for the list walk. */
void *CBomb_Delete(struct CBomb *self)
{
	void *prev;
	int i;

	prev = MB_LIST_REMOVE(&DAT_00308f48, self);
	for (i = 0; i < DAT_002bae94; i++)
	{
		if (MB_I32(DAT_003075bc[i], 0x1c) == (int)MB_ADDR(self->actor, 0xd0))
			MB_I32(DAT_003075bc[i], 0x1c) = 0;
	}
	FUN_000adfc0((int)self->actor);
	if (self->bombPos != NULL)
		self->bombPos->used = 0;
	MB_DELETE(self);
	return prev;
}

/* 0x00160ee0  Meta (file inferred: CBomb.cpp)
Deletes every bomb. */
void CBomb_DeleteAll(void)
{
	void *bomb;

	bomb = MB_LIST_NEXT(&DAT_00308f48, NULL);
	while (bomb != NULL)
	{
		bomb = CBomb_Delete((struct CBomb *)bomb);
		bomb = MB_LIST_NEXT(&DAT_00308f48, bomb);
	}
}

/* 0x00160f10  Meta (file inferred: CBomb.cpp)
The bomb goes off: light flash and explosion, pushes and hurts the players in its
radius during the board phase, rumble, a scorch mark on the ground below, then the bomb
is deleted. Returns the list predecessor (CBomb_Delete). */
void *CBomb_Explode(struct CBomb *self)
{
	float *pos = MB_FP(self->actor, 0xd0);
	float *ppos;
	float start[4];
	float down[4];
	float hit[4];
	float normal[4];
	float dx;
	float dy;
	float dz;
	float d2;
	float d;
	float push;
	float px;
	float py;
	float pz;
	float k;
	int character;
	int meta;
	int i;

	/* an emptied debug function (pos, 120.0, 5) */
	FUN_001d5950();
	switch (self->type)
	{
	case 0:
	case 1:
	case 2:
	case 4:
		FUN_000b7380((undefined4)pos, 5000.0f, 0x32, 0x7f, 0x3c, 0, 0);
		if (self->explosion != NULL)
			CMetaExplosion_Start(self->explosion, pos);
		break;
	case 3:
		if (self->explosion != NULL)
			CMetaExplosion_Start(self->explosion, pos);
		FUN_000b7380((undefined4)pos, 5000.0f, 6, 0x7f, 100, 0x50, 0);
		break;
	}
	if (MB_I32(DAT_0030564c, 0x68) == 1)
	{
		for (i = 0; i < DAT_002bae94; i++)
		{
			meta = DAT_003075bc[i];
			character = MB_I32(meta, 0x30);
			ppos = MB_FP(character, 0xd0);
			dx = ppos[0] - pos[0];
			dz = ppos[2] - pos[2];
			dy = DAT_002b0be0 + (ppos[1] - pos[1]);
			d2 = dx * dx + dy * dy + dz * dz;
			if (!(d2 < self->radius * self->radius))
				continue;
			d = sqrtf(d2);
			push = (self->radius - d) * self->pushScale;
			if (!(push <= self->maxPush))
				push = self->maxPush;
			if (d > 1.0f)
			{
				k = push / d;
				px = k * dx;
				py = k * dy;
				pz = k * dz;
			}
			else
			{
				px = DAT_0028fdd8[0] * push;
				py = DAT_0028fdd8[1] * push;
				pz = DAT_0028fdd8[2] * push;
			}
			MB_F32(character, 0x194) = px + MB_F32(character, 0x194);
			MB_F32(character, 0x198) = py + MB_F32(character, 0x198);
			MB_F32(character, 0x19c) = pz + MB_F32(character, 0x19c);
			if (MB_F32(DAT_003075bc[i], 0x10a8) == 0.0f)
			{
				((void (*)(int, float *, float, int, int))FUN_00156c40)(DAT_003075bc[i], pos,
					(DAT_002b0bdc * push) / self->maxPush, 0, 1);
				switch (self->type)
				{
				case 0:
				case 1:
				case 2:
				case 4:
					/* the rumble manager */
					((void (*)(void *, float *, const char *, float))FUN_000804f0)(
						DAT_002ed324, pos, "Bomb Fall", 1000.0f);
					break;
				case 3:
					((void (*)(void *, int, const char *))FUN_000803b0)(DAT_002ed324, i,
						"Air Raid");
					break;
				}
			}
			MB_I32(DAT_003075bc[i], 0x1138) = MB_I32(DAT_003075bc[i], 0x1138) + 1;
		}
	}
	start[0] = pos[0];
	start[2] = pos[2];
	start[3] = pos[3];
	start[1] = pos[1] + 100.0f + 0.1f;                  /* 0x00255544, 0x002536cc */
	down[0] = 0.0f;
	down[1] = -200.0f;
	down[2] = 0.0f;
	down[3] = 1.0f;
	if (((float (*)(float *, float *, float *, float *))FUN_000b4bd0)(start, down, hit,
		normal) > 0.0f)
		CScorchMark_Create(hit, normal, NULL, 0);
	return CBomb_Delete(self);
}

/* 0x00161250  Meta/CBomb.cpp
Per frame: drops the raid's missiles (mostly over the inner area, from height 1600);
deletes the raid when its life is over. Returns the list predecessor or the raid. */
struct CAirRaid *CAirRaid_Update(struct CAirRaid *self)
{
	float pos[4];
	float r;
	struct CBomb *bomb;
	struct CAirRaid *prev;

	self->accum = DAT_0028fda8 + self->accum;
	while (!(self->accum < self->interval))
	{
		pos[3] = 0.0f;
		pos[1] = 1600.0f;
		self->accum = self->accum - self->interval;
		r = (float)mb_rand() * MB_RAND_SCALE;
		if (r <= 0.5f)
		{
			pos[0] = (float)mb_rand() * MB_RAND_SCALE * self->outerWidth + self->outerX;
			pos[2] = (float)mb_rand() * MB_RAND_SCALE * self->outerDepth + self->outerZ;
		}
		else
		{
			pos[0] = (float)mb_rand() * MB_RAND_SCALE * self->width + self->x;
			pos[2] = (float)mb_rand() * MB_RAND_SCALE * self->depth + self->z;
		}
		bomb = (struct CBomb *)MB_NEW(0x5c, "C:\\BlitzParty\\Game\\Src\\Meta\\CBomb.cpp", 0x363);
		if (bomb == NULL)
			bomb = NULL;
		else
			bomb = CBomb_Construct(bomb, 3, pos, 0);
		/* as in the original, a failed allocation reads through NULL */
		MB_F32(bomb->actor, 0x194) = self->velX;
		MB_F32(bomb->actor, 0x198) = 0.0f;
		MB_F32(bomb->actor, 0x19c) = self->velZ;
		MB_F32(bomb->actor, 0x1a0) = 0.0f;
	}
	self->life = self->life - DAT_0028fda8;
	if (!(self->life <= 0.0f))
		return self;
	prev = MB_LIST_REMOVE(&DAT_00308f3c, self);
	MB_DELETE(self);
	return prev;
}

/* 0x00161460  Meta (file inferred: CBomb.cpp)
Deletes the positions of every bomb group. */
void MbBombGroups_DeleteAll(void)
{
	int group;
	void *bp;
	void *next;

	for (group = 0; group < DAT_00308f38; group++)
	{
		bp = MB_LIST_NEXT(&DAT_00308f58[group].positions, NULL);
		while (bp != NULL)
		{
			next = MB_LIST_REMOVE(&DAT_00308f58[group].positions, bp);
			if (bp != NULL)
				MB_DELETE(bp);
			bp = MB_LIST_NEXT(&DAT_00308f58[group].positions, next);
		}
	}
}

/* 0x001614c0  Meta (file inferred: CBomb.cpp)
Per frame: drops the group's bombs, one per interval, while some are left. */
void MbBombGroup_Update(struct MbBombGroup *self)
{
	if (self->count <= 0)
		return;
	self->accum = DAT_0028fda8 + self->accum;
	while (self->interval < self->accum)
	{
		if (self->count < 1)
			return;
		self->accum = self->accum - self->interval;
		MbBombGroup_DropOne(self);
		self->count--;
	}
}

/* 0x00161510  Meta (file inferred: CBomb.cpp)
Deletes every air raid. */
void CAirRaid_DeleteAll(void)
{
	void *raid;
	void *next;

	raid = MB_LIST_NEXT(&DAT_00308f3c, NULL);
	while (raid != NULL)
	{
		next = MB_LIST_REMOVE(&DAT_00308f3c, raid);
		if (raid != NULL)
			MB_DELETE(raid);
		raid = MB_LIST_NEXT(&DAT_00308f3c, next);
	}
}

/* lights the bomb's fuse */
static void bomb_light(struct CBomb *self)
{
	self->timer = self->fuse;
	MB_F32(self->actor, 0x334) = 1.0f;
	FUN_000ae820((int)self->actor, 6.0f, self->fuse);
}

/* 0x00161560  Meta (file inferred: CBomb.cpp)
Per frame: falls (and lands), the mine waits for a player, the missile leaves trails, the
dynamite sparks, dropped particles, then the fuse; at its end the bomb explodes.
Returns the bomb, or the list predecessor when it went off. */
void *CBomb_Update(struct CBomb *self)
{
	float gravity = DAT_00292c84 * 0.25f;               /* 0x00257b88 */
	void *actor = self->actor;
	float above;
	float step[3];
	float dir[4];
	float pos[4];
	float t;
	float scale;
	float a1;
	float a2;
	int i;

	if ((MB_U8(actor, 0x220) & 1) != 0)
	{
		/* falling: remember the last position */
		MB_F32(actor, 0x1a4) = MB_F32(actor, 0xd0);
		MB_F32(actor, 0x1a8) = MB_F32(actor, 0xd4);
		MB_F32(actor, 0x1ac) = MB_F32(actor, 0xd8);
		MB_F32(actor, 0x1b0) = MB_F32(actor, 0xdc);
		if (MB_F32(self->actor, 0xd4) < -400.0f)        /* 0x002699e4 */
		{
			bomb_light(self);
			bkPrintf("bomb missed!");
		}
		actor = self->actor;
		above = MB_F32(actor, 0xd4) - self->height;
		step[0] = DAT_0028fda8 * MB_F32(actor, 0x194);
		step[2] = DAT_0028fda8 * MB_F32(actor, 0x19c);
		step[1] = DAT_0028fda8 * MB_F32(actor, 0x198) -
			DAT_0028fda8 * DAT_0028fda8 * gravity * 0.5f;
		if (above <= -step[1])
		{
			/* landed */
			MB_F32(actor, 0xd4) = MB_F32(actor, 0xd4) - above;
			MB_U32(self->actor, 0x220) = MB_U32(self->actor, 0x220) & 0xfffffffeu;
			if (self->type == 0)
				CMLaser_Show(MB_FP(self->actor, 0xd0), 0xc, 60.0f);
			if (self->type == 0 || self->type == 3)
				bomb_light(self);
			if (self->type == 0)
				*(float *)MB_PTR(self->actor, 0x188) = 2000.0f;
			if (self->type == 4)
				*(float *)MB_PTR(self->actor, 0x188) = 200000.0f;
		}
		else
		{
			MB_F32(actor, 0x198) = MB_F32(actor, 0x198) - DAT_0028fda8 * gravity;
			MB_F32(self->actor, 0xd0) = step[0] + MB_F32(self->actor, 0xd0);
			MB_F32(self->actor, 0xd4) = step[1] + MB_F32(self->actor, 0xd4);
			MB_F32(self->actor, 0xd8) = step[2] + MB_F32(self->actor, 0xd8);
		}
	}
	if (self->type == 2)
	{
		/* the mine: lit when a player comes near */
		if (self->timer < 0.0f)
		{
			for (i = 0; i < DAT_002bae94; i++)
			{
				if (((float (*)(float *, float *))FUN_00051540)(MB_FP(self->actor, 0xd0),
					MB_FP(MB_I32(DAT_003075bc[i], 0x30), 0xd0)) < self->triggerDist)
					bomb_light(self);
			}
		}
	}
	else if (self->type == 3)
	{
		/* the missile: slows down, points along its flight, leaves trails */
		actor = self->actor;
		MB_F32(actor, 0x194) = MB_F32(actor, 0x194) * 0.96f;    /* 0x0027008c */
		MB_F32(self->actor, 0x198) = MB_F32(self->actor, 0x198) * 0.96f;
		MB_F32(self->actor, 0x19c) = MB_F32(self->actor, 0x19c) * 0.96f;
		actor = self->actor;
		step[0] = MB_F32(actor, 0xd0) - MB_F32(actor, 0x1a4);
		step[1] = MB_F32(actor, 0xd4) - MB_F32(actor, 0x1a8);
		step[2] = MB_F32(actor, 0xd8) - MB_F32(actor, 0x1ac);
		FUN_000514a0(dir, step);
		CLQuat_FromDirection(MB_FP(self->actor, 0xf0), dir);
		for (i = 0; i < self->numTrails; i++)
			CProjTrail_AddPoint((struct CProjTrail *)self->trails[i], MB_FP(self->actor, 0xd0));
	}
	else if (self->type == 4)
	{
		/* the dynamite: sparks from the fuse */
		actor = self->actor;
		pos[0] = MB_F32(actor, 0xd0);
		pos[2] = MB_F32(actor, 0xd8);
		pos[1] = DAT_002b0bd8 * self->timer + DAT_002b0bd4 + MB_F32(actor, 0xd4);
		pos[3] = MB_F32(actor, 0xdc);
		for (i = 0; i < DAT_002b0bc8; i++)
		{
			a1 = (float)(mb_rand() % 0xb4) * 3.1415925f * 0.0055555557f; /* 0x00256750, 0x00263518 */
			a2 = (float)(mb_rand() % 0x168) * 3.1415925f * 0.0055555557f;
			FUN_00051220(step, DAT_0028fdc8, a1, a2);
			scale = (float)(mb_rand() % DAT_002b0bd0) + DAT_002b0bcc;
			step[0] = step[0] * scale;
			step[1] = step[1] * scale;
			step[2] = scale * step[2];
			FUN_00080d60((int)DAT_002b215c, (undefined4)pos, (undefined4)step, 1, 0xffffffffu);
		}
	}
	if (self->dropInterval > 0.0f)
	{
		/* particles dropped along the path, spaced in time */
		self->dropAccum = DAT_0028fda8 + self->dropAccum;
		if (self->dropInterval < self->dropAccum)
		{
			t = (self->dropInterval - self->dropAccum) / DAT_0028fda8 + 1.0f;
			FUN_00050ff0(step, MB_FP(self->actor, 0x1a4), t, MB_FP(self->actor, 0xd0), 1.0f - t);
			FUN_00080d60((int)self->dropType, (undefined4)step, (undefined4)DAT_002ea478, 0,
				0xffffffffu);
			self->dropAccum = self->dropAccum - self->dropInterval;
			while (self->dropInterval < self->dropAccum)
			{
				t = self->dropInterval / DAT_0028fda8 + t;
				FUN_00050ff0(step, MB_FP(self->actor, 0x1a4), t, MB_FP(self->actor, 0xd0),
					1.0f - t);
				FUN_00080d60((int)self->dropType, (undefined4)step, (undefined4)DAT_002ea478, 0,
					0xffffffffu);
				self->dropAccum = self->dropAccum - self->dropInterval;
			}
		}
	}
	if (self->timer >= 0.0f)
	{
		self->timer = self->timer - DAT_0028fda8;
		if (self->timer <= 0.0f)
			return CBomb_Explode(self);
	}
	return self;
}

/* 0x00161b80  Meta (file inferred: CBomb.cpp)
Per frame: the bomb groups, the air raids, the bombs. */
void CBomb_UpdateAll(void)
{
	int group;
	void *node;

	for (group = 0; group < DAT_00308f38; group++)
		MbBombGroup_Update(&DAT_00308f58[group]);
	node = MB_LIST_NEXT(&DAT_00308f3c, NULL);
	while (node != NULL)
	{
		node = CAirRaid_Update((struct CAirRaid *)node);
		node = MB_LIST_NEXT(&DAT_00308f3c, node);
	}
	node = MB_LIST_NEXT(&DAT_00308f48, NULL);
	while (node != NULL)
	{
		node = CBomb_Update((struct CBomb *)node);
		node = MB_LIST_NEXT(&DAT_00308f48, node);
	}
}
