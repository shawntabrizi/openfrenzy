/*
Meta/Searchlights.c  <-  C:\BlitzParty\Game\Src\Meta\Searchlights.cpp

Searchlights on the Meta board: a light model that follows a key-framed path and sweeps a
cone; a player who stays in the cone long enough sets off the light's big gun (BigGun.cpp),
which may hurt players in a narrower cone. Drawn as a model (lit redder the more a player
is detected) or as a clipped beam. All searchlights are in the list at 0x003090a8.

0x00162640 is tagged with Searchlights.cpp; the draw function before it and the update and
list functions after it work on the same objects (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x003090a8  the searchlights */
struct MbList DAT_003090a8;

/* 0x002b0c88, 0x002b0c8c  the light's target is smoothed (1) with this factor */
int DAT_002b0c88 = 1;
float DAT_002b0c8c = 0.82f;

/* 0x0030a090  the board's ambient light, 0x00BBGGRR (meta_a) */
extern unsigned int DAT_0030a090;
/* 0x002ea488  charlib's actor list */
extern int *DAT_002ea488;

/* the highest detection of any player, as the game rounds it */
static int searchlight_max_detection(struct CSearchlight *self)
{
	int m = 0;
	int i;

	for (i = 0; i < DAT_002bae94; i++)
	{
		if ((float)m < self->detection[i])
			m = (int)self->detection[i];
	}
	return m;
}

/* 0x00162260  Meta (file inferred: Searchlights.cpp)
Draws every searchlight: the model, redder while a player is being detected, or the beam
(paler while detected); then the big gun's laser. */
void Searchlights_DrawAll(void)
{
	struct CSearchlight *s;
	int m;
	int k;

	for (s = MB_LIST_NEXT(&DAT_003090a8, NULL); s != NULL;
		s = MB_LIST_NEXT(&DAT_003090a8, s))
	{
		if (s->state != 1)
		{
			if ((MB_U8(s, 0x88) & 1) == 0)
			{
				MB_SET_RS(5, 0);
				MB_SET_RS(7, 3);
				MB_SET_RS(6, 1);
				MB_SET_RS(10, 1);
				MB_SET_RS(3, 1);
				m = searchlight_max_detection(s);
				k = 0x7f - (int)((float)(m * 0x7f) / s->threshold);
				baSetBrightness((int)(MB_F32(s->actor, 0xe0) / s->scale * 256.0f)); /* 0x00257b90 */
				baSetAmbientLight(0x7f, k, k);
				MB_SET_RS(4, 1);
				baDrawActorInstance((struct BActorInstance *)MB_ADDR(s->actor, 0x10), 7);
				baSetBrightness(0x100);
			}
			else
			{
				FUN_000438b0();
				MB_SET_RS(6, 1);
				MB_SET_RS(10, 1);
				s->beamEnd[1] = s->beamEnd[1] + s->beamEndLength;
				m = searchlight_max_detection(s);
				if (m == 0)
				{
					k = 0x46;
					s->beamColour[0] = 0x46;
				}
				else
				{
					k = 0x46 - (int)((float)(m * 0x46) / s->threshold);
					s->beamColour[0] = 0x46;
				}
				s->beamColour[1] = k;
				s->beamColour[2] = k;
				bdDrawBeamClipped(s->beamStart, s->beamStartWidth, s->beamStartLength,
					s->beamStartColour, s->beamEnd, s->beamEndWidth, s->beamEndLength,
					s->beamColour, (struct bdTexture *)s->beamTexture, NULL, 0.0f);
			}
		}
		if (s->gun != NULL)
			CBigGun_DrawLaser(s->gun, s->target);
	}
	baSetAmbientLight(DAT_0030a090 & 0xff, (DAT_0030a090 >> 8) & 0xff,
		(DAT_0030a090 >> 16) & 0xff);
	MB_SET_RS(5, 1);
	MB_SET_RS(6, 0);
	MB_SET_RS(10, 0);
	MB_SET_RS(3, MB_U32(DAT_003055d0, 0x48));
	MB_SET_RS(4, 0);
}

/* 0x00162640  Meta/Searchlights.cpp
Constructor: the light model `name` at `pos` (scale `scale`, out of charlib's actor list),
and when `gunId` is not -1 its big gun (model `gunModel`, laser from bone `gunBone`, at
`gunPos`). */
struct CSearchlight *CSearchlight_Construct(struct CSearchlight *self, const char *name,
	int unused, float *pos, float scale, int arg6, int gunId, int flags, const char *gunModel,
	const char *gunBone, float *gunPos)
{
	struct CBigGun *gun;

	self->flags = flags;
	self->targetValid = 0;
	self->actor = Actor_LoadAny(name, pos, 8, NULL, 0);
	CLList_Remove(DAT_002ea488, (int *)self->actor);
	MB_LIST_INSERT(&DAT_003090a8, self, 0);
	MB_F32(self->actor, 0xe8) = scale;
	MB_F32(self->actor, 0xe0) = scale;
	self->scale = scale;
	self->arg6c = arg6;
	self->state = 0;
	self->detection[3] = 0.0f;
	self->detection[2] = 0.0f;
	self->detection[1] = 0.0f;
	self->detection[0] = 0.0f;
	self->path.hasKeys = 0;
	self->path.count = 1;
	self->cone = (float)atan2((double)(scale * 250.0f), 3300.0); /* 0x00262d34, 0x00270198 */
	self->pathTime[0] = 0.0f;
	self->pathTime[1] = 0.0f;
	self->pathTime[2] = 0.0f;
	self->pathTime[3] = 0.0f;
	self->pathTime[4] = 0.0f;
	self->threshold = 50.0f;
	self->gun = NULL;
	if (gunId != -1)
	{
		gun = (struct CBigGun *)MB_NEW(0x28, "C:\\BlitzParty\\Game\\Src\\Meta\\Searchlights.cpp",
			0x4e);
		if (gun == NULL)
			gun = NULL;
		else
			gun = CBigGun_Construct(gun);
		self->gun = gun;
		/* as in the original, a failed allocation writes through NULL */
		gun->pos[0] = gunPos[0];
		gun->pos[1] = gunPos[1];
		gun->pos[2] = gunPos[2];
		gun->pos[3] = gunPos[3];
		CBigGun_Init(self->gun, gunModel, gunBone, gunId);
	}
	return self;
}

/* 0x001627a0  Meta (file inferred: Searchlights.cpp)
Per frame: follows the path and turns the light to its (smoothed) target; state 0: detects
players in the cone (the big gun fires when one stays long enough, and hurts players in a
narrower cone while firing), 2: grows back to its size, 3: shrinks and switches off. */
void CSearchlight_Update(struct CSearchlight *self)
{
	float key[4];
	float dir[4];
	int meta;
	int character;
	short st;
	int i;

	/* the cone axis is only computed with a path (the original reads it anyway) */
	dir[0] = 0.0f;
	dir[1] = 0.0f;
	dir[2] = 0.0f;
	dir[3] = 0.0f;
	FUN_000b8240(self->pathTime);
	if (self->path.hasKeys != 0)
	{
		bGetKey3(key, (struct BKeyTrack3 *)&self->path, (int)self->pathTime[0]);
		if (DAT_002b0c88 == 0 || self->targetValid == 0)
		{
			self->target[0] = key[0];
			self->target[1] = key[1];
			self->target[2] = key[2];
			self->target[3] = key[3];
			self->targetValid = 1;
		}
		else
		{
			FUN_00050ff0(self->target, self->target, DAT_002b0c8c, key, 1.0f - DAT_002b0c8c);
		}
		dir[0] = self->target[0] - MB_F32(self->actor, 0xd0);
		dir[1] = self->target[1] - MB_F32(self->actor, 0xd4);
		dir[2] = self->target[2] - MB_F32(self->actor, 0xd8);
		FUN_000514a0(dir, dir);
		CLQuat_FromDirection(MB_FP(self->actor, 0xf0), dir);
	}
	baPrepActorInstance((struct BActorInstance *)MB_ADDR(self->actor, 0x10), 1);
	if (self->state == 0)
	{
		if (MB_I32(DAT_0030564c, 0x68) == 1)
		{
			for (i = 0; i < DAT_002bae94; i++)
			{
				meta = DAT_003075bc[i];
				character = MB_I32(meta, 0x30);
				if (CLMath_SphereInCone(MB_FP(character, 0xd0),
					MB_F32(MB_PTR(character, 0x18c), 0x9c), MB_FP(self->actor, 0xd0), dir,
					self->cone) == 0)
				{
					self->detection[i] = self->detection[i] * 0.5f;
					continue;
				}
				if (MB_I32(self->gun->laser, 0x64) == 1)
				{
					st = MB_I16(DAT_003075bc[i], 0x36);
					if (st != 10 && st != 0xd)
					{
						character = MB_I32(DAT_003075bc[i], 0x30);
						if (CLMath_SphereInCone(MB_FP(character, 0xd0),
							MB_F32(MB_PTR(character, 0x18c), 0x9c) * 0.5f,
							MB_FP(self->actor, 0xd0), dir, self->cone * 0.0009765625f) != 0)
						{
							meta = DAT_003075bc[i];
							((void (*)(int, float *, float, int, int))FUN_00156c40)(meta,
								MB_FP(MB_I32(meta, 0x30), 0xd0), 10000.0f,
								(int)Core_Rand() % 2 + 1, 1);
						}
					}
				}
				self->detection[i] = DAT_0028fda8 + self->detection[i];
				if (self->threshold < self->detection[i])
				{
					if (self->gun != NULL)
					{
						MB_I32(self->gun->laser, 0x64) = 1;
						MB_I32(self->gun->laser, 0x68) = 1;
						MB_F32(self->gun->laser, 0x6c) = 0.0f;
					}
					self->detection[i] = 0.0f;
				}
			}
		}
	}
	else if (self->state == 2)
	{
		MB_F32(self->actor, 0xe0) = (self->scale - MB_F32(self->actor, 0xe0)) * 0.25f +
			MB_F32(self->actor, 0xe0);
		MB_F32(self->actor, 0xe8) = MB_F32(self->actor, 0xe0);
		if (self->scale - MB_F32(self->actor, 0xe0) <= 0.01f)  /* 0x0025372c */
			self->state = 0;
	}
	else if (self->state == 3)
	{
		MB_F32(self->actor, 0xe0) = MB_F32(self->actor, 0xe0) * 0.75f;   /* 0x00253844 */
		MB_F32(self->actor, 0xe8) = MB_F32(self->actor, 0xe0);
		if (MB_F32(self->actor, 0xe0) <= 0.01f)
		{
			self->state = 1;
			self->targetValid = 0;
			MB_I32(self->gun->laser, 0x64) = 0;
			MB_I32(self->gun->laser, 0x68) = 0;
			MB_I32(self->gun->laser, 0x6c) = 0;
			bsStopSample((struct BSample *)DAT_00309260);
			for (i = 0; i < DAT_002bae94; i++)
				self->detection[i] = 0.0f;
		}
	}
	if (self->gun != NULL)
		CBigGun_Aim(self->gun, self->target);
	self->target[3] = 1.0f;
	self->cone = (float)atan2((double)(MB_F32(self->actor, 0xe0) * 250.0f), 3300.0);
	MB_F32(self->actor, 0xdc) = 1.0f;
}

/* 0x00162b60  Meta (file inferred: Searchlights.cpp)
Updates every searchlight. */
void Searchlights_UpdateAll(void)
{
	struct CSearchlight *s;

	for (s = MB_LIST_NEXT(&DAT_003090a8, NULL); s != NULL;
		s = MB_LIST_NEXT(&DAT_003090a8, s))
		CSearchlight_Update(s);
}

/* 0x00162b90  Meta (file inferred: Searchlights.cpp)
Deletes a searchlight with its actor, path keys and big gun. Returns the list
predecessor. */
void *CSearchlight_Delete(struct CSearchlight *self)
{
	void *prev;
	struct CBigGun *gun;

	prev = MB_LIST_REMOVE(&DAT_003090a8, self);
	Actor_Delete(self->actor);
	if (self->path.hasKeys != 0)
	{
		MB_DELETE(self->path.keys);
		self->path.keys = NULL;
	}
	if (self->gun != NULL)
	{
		CBigGun_Free(self->gun);
		gun = self->gun;
		if (gun != NULL)
		{
			CBigGun_Destruct(gun);
			MB_DELETE(gun);
		}
		self->gun = NULL;
	}
	MB_DELETE(self);
	return prev;
}

/* 0x00162c10  Meta (file inferred: Searchlights.cpp)
Deletes every searchlight. */
void Searchlights_DeleteAll(void)
{
	void *s;

	s = MB_LIST_NEXT(&DAT_003090a8, NULL);
	while (s != NULL)
	{
		s = CSearchlight_Delete((struct CSearchlight *)s);
		s = MB_LIST_NEXT(&DAT_003090a8, s);
	}
}
