/*
CGAMEMODE_TEAM.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_Team.cpp

openfrenzy core module: the team screen. The four characters stand in a row, swap places
a few times (random, then the swap that the score table asks for) and form two teams.
Vtable 0x0025667c. Decompiled from orig/default.xbe (functional, not byte-matching).
The constructor (0x0002a5c0) is in the source-map gap before this file; it sets this
class's vtable, so it is placed here.
*/

#include <math.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"
#include "core_modes.h"

static const char team_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_Team.cpp";

/* a popup text (core_modes_a 0x00011c50): vtable +0x04 show, +0x10 (int) open, +0x14 close */
typedef void *const *Popup;

/* CGameMode_Team, 0x198 bytes */
struct CGameMode_Team {
	struct CMenuMode base;           /* +0x000 */
	unsigned char list[0xc];         /* +0x098 */
	unsigned char *boxes[4];         /* +0x0a4 a text box per player; +0x74: its actor */
	unsigned char light[0x70];       /* +0x0b4 */
	int shuffles;                    /* +0x124 swaps left */
	int swap_a;                      /* +0x128 */
	int swap_b;                      /* +0x12c */
	int last;                        /* +0x130 */
	float swap_time;                 /* +0x134 */
	float state;                     /* +0x138 0, 1 accept, >1 fading out */
	int slots[4];                    /* +0x13c the place of each player */
	Popup title;                     /* +0x14c */
	Popup team_captions[2];          /* +0x150 */
	Popup captions[4];               /* +0x158 */
	Popup accept;                    /* +0x168 */
	void *const *fader;              /* +0x16c an object (core_modes_a 0x00013820), 0x24 bytes */
	unsigned char fader_rest[0x20];  /* +0x170; +0x17c: the popup list */
	void *font_bold;                 /* +0x190 */
	void *font_light;                /* +0x194 */
};
_Static_assert(sizeof(struct CGameMode_Team) == 0x198, "CGameMode_Team size");

/* slot K of the vtable of the object P */
#define VCALL(p, k) (((void *const *)*(void *const *)(p))[k])
#define POPUP_LIST(self) ((void *)((unsigned char *)(self) + 0x17c))
#define POPUP_FLAGS(p) (*(uint32_t *)((unsigned char *)(p) + 0x8c))
#define ext_PopupCtor ((Popup (*)(void *, int, int, const void *, void *))FUN_00011c50)
#define ext_PopupInit ((void (*)(Popup, int))FUN_000114b0)

extern const void *const vtable_0025667c[14];
extern unsigned char DAT_00292ca0[];       /* charlib: animations, 0x44 bytes each (name, speed) */
extern unsigned char DAT_002b9968[];       /* core_modes_b: the score table object */
extern int DAT_002b9c58, DAT_002b9c64;     /* core_modes_b: the swap the table asks for */
extern int DAT_002b95ac;                   /* core_modes_a: character select loaded */
extern void *DAT_002b9670;                 /* core_modes_a: Packages\CharacterSel */
extern int DAT_00281b0c[3];                /* core_modes_a: ambient light */
extern int DAT_00281b18[3];                /* core_modes_a: light colour */
extern unsigned char DAT_002900c8[];       /* classlib_ui: characters, 0x108 bytes each */

static const float team_light_dir[4] = { 0.3f, 0.3f, -0.5f, 1.0f };

/* add POPUP to the popup list (at the end) and show it */
static void CGameMode_Team_ShowPopup(struct CGameMode_Team *self, Popup popup)
{
	ext_ListAdd(POPUP_LIST(self), popup != NULL ? (void *)((unsigned char *)popup + 4) : NULL, 1);
	((void (*)(Popup))VCALL(popup, 1))(popup);
}

static Popup CGameMode_Team_NewPopup(int line, int x, int y, const void *text, void *font)
{
	void *memory = ext_New(0x90, team_cpp, line, 0x1001);

	return memory ? ext_PopupCtor(memory, x, y, text, font) : NULL;
}

/* 0x0002a5c0  (Core, CGameMode_Team.cpp) CGameMode_Team::CGameMode_Team */
struct CGameMode_Team *CGameMode_Team_Ctor(struct CGameMode_Team *self)
{
	CMenuMode_Ctor(&self->base, "Team");
	self->base.vtbl = vtable_0025667c;
	ext_ListInit(self->list);
	((void *(*)(void *))FUN_00013820)(&self->fader);
	self->base.repeat_rate[6] = 100000000;
	self->base.repeat_rate[5] = 100000000;
	self->base.repeat_rate[4] = 100000000;
	self->base.flags |= 2;
	self->base.field_0c++;
	return self;
}

/* 0x0002a650  Core/CGameMode_Team.cpp  popped (vtable +0x0c) */
void CGameMode_Team_Popped(struct CGameMode_Team *self)
{
	((void (*)(void *))FUN_000135e0)(&self->fader);
	ext_LightFree(self->light);
	ext_FUN_00072720(self->list);
	ext_TextFreeAll();
	((void (*)(void))FUN_000129d0)();
}

/* 0x0002a690  Core/CGameMode_Team.cpp  draw (vtable +0x1c) */
void CGameMode_Team_Draw(struct CGameMode_Team *self, int top)
{
	if (top == 0)
		return;
	ext_FUN_00072720(self->list);
	((void (*)(int, int, int))FUN_00046cb0)(8, 0, 0);
	((void (*)(int, int, int))FUN_00046cb0)(3, 1, 0);
	ext_FUN_00073900(0);
	((void (*)(int))FUN_00046d00)(3);
	((void (*)(void *))self->fader[3])(&self->fader);
	if (self->state != 0.0f)
		ext_FadeTo((int)self->state, 0);
	((void (*)(int))FUN_00046d00)(8);
}

/* 0x0002a720  Core/CGameMode_Team.cpp  a button (vtable +0x34): A or B when the teams stand */
int CGameMode_Team_Button(struct CGameMode_Team *self, int button, int player)
{
	(void)player;
	if (self->state != 0.0f && button > 3 && button < 6 && *(uint32_t *)&self->state == 0x3f800000)
	{
		ext_PlaySfx(9, 0x7f, 0, 0, 0);
		((void (*)(void *, int))self->fader[5])(&self->fader, 0);
		self->state = 2.0f;
	}
	return 0;
}

/* 0x0002a790  Core/CGameMode_Team.cpp  play animation ANIM (0x00292ca0) on PLAYER's character
from frame FRAME (in 1/1024), looping when LOOP; KEEP: do not reset the flags; BLEND != 0:
blend in */
void CGameMode_Team_PlayAnim(struct CGameMode_Team *self, int player, int anim, int loop, int keep,
	float frame, float blend)
{
	unsigned char *actor = *(unsigned char **)(self->boxes[player] + 0x74);
	void *sequence = ext_ActorFindAnim(actor, (const char *)(DAT_00292ca0 + anim * 0x44));

	if (sequence == NULL)
		return;
	if (keep == 0)
	{
		actor = *(unsigned char **)(self->boxes[player] + 0x74);
		ext_ActorSetFlags(actor, *(void **)(actor + 0x134), 1, 0);
	}
	actor = *(unsigned char **)(self->boxes[player] + 0x74);
	if (blend == 0.0f)
		ext_ActorPlay(actor, *(void **)(actor + 0x134), sequence, (int)((double)frame * 1024.0), -1,
			loop != 0, 0, 0);
	else
		ext_ActorPlayBlend(actor, *(void **)(actor + 0x134), sequence, (int)((double)frame * 1024.0), -1,
			blend, loop != 0, 0, 0);
	actor = *(unsigned char **)(self->boxes[player] + 0x74);
	ext_ActorUpdate(actor, *(void **)(actor + 0x134), 0.0f, 0);
	ext_ActorSetVisible(*(unsigned char **)(self->boxes[player] + 0x74), 1);
}

/* 0x0002a8d0  (Core, CGameMode_Team.cpp) CGameMode_Team::~CGameMode_Team */
void CGameMode_Team_Dtor(struct CGameMode_Team *self)
{
	((void (*)(void *))FUN_00013870)(&self->fader);
	ext_Nothing(self->list);
	CMenuMode_Dtor(&self->base);
}

/* 0x0002a8b0  Core/CGameMode_Team.cpp  scalar deleting destructor (vtable [0]) */
struct CGameMode_Team *CGameMode_Team_Destroy(struct CGameMode_Team *self, int flags)
{
	CGameMode_Team_Dtor(self);
	if (flags & 1)
		ext_Delete(self);
	return self;
}

/* 0x0002a930  Core/CGameMode_Team.cpp  pushed (vtable +0x10): light, fonts and captions */
void CGameMode_Team_Pushed(struct CGameMode_Team *self)
{
	unsigned short text[0x40];
	unsigned char *table = *(unsigned char **)((unsigned char *)DAT_002ecc74 + 0x20);
	int i, x;

	self->base.state = 0;
	if (DAT_002b95ac != 0)
		((void (*)(void *))self->base.vtbl[11])(self);
	else if (DAT_002b9670 == NULL)
	{
		DAT_002b9670 = ext_LoadPackageAsync("Packages\\CharacterSel", 1);
		if (DAT_002b9670 == NULL)
			ext_DebugPrintf("CGameMode_Team::Pushed *** Could not load package 'Packages\\CharacterSel' ***\n");
		self->base.state = 2;
	}
	ext_LightInit(self->light, 0);
	memcpy(self->light + 0x14, DAT_00281b18, 3 * sizeof(int));
	ext_LightDirection(self->light, team_light_dir);
	ext_LightEnable(self->light, 1);
	ext_SetAmbient(DAT_00281b0c[0], DAT_00281b0c[1], DAT_00281b0c[2]);
	self->swap_time = 0.0f;
	self->shuffles = 6;
	self->last = -1;
	self->state = 0.0f;
	self->font_bold = ext_FindResource(0, "handgotb.ttf", 3, 0xdefa);
	self->font_light = ext_FindResource(0, "handgotl.ttf", 3, 0xdefa);
	self->title = CGameMode_Team_NewPopup(0x53, -320, 200, *(void **)(table + 0x1750), self->font_bold);
	POPUP_FLAGS(self->title) |= 8;
	ext_PopupInit(self->title, 0);
	CGameMode_Team_ShowPopup(self, self->title);
	for (i = 0, x = -0x1e0; i < 2; i++, x += 0x140)
	{
		self->team_captions[i] = CGameMode_Team_NewPopup(0x5b, x, 0xa0, *(void **)(table + 0x1760 + i * 0x10),
			self->font_bold);
		POPUP_FLAGS(self->team_captions[i]) |= 8;
		ext_PopupInit(self->team_captions[i], 0);
	}
	for (i = 0; i < 4; i++)
	{
		if (g_PlayerPresent(i) != 0)
			ext_WSprintf(text, *(const uint16_t **)(table + 0x1870 + i * 0x10));
		else
			ext_WSprintf(text, u"%S", *(const char **)(table + 0x1020));
		self->captions[i] = CGameMode_Team_NewPopup(0x65, (i / 2) * 0x78 - 0x212 + i * 0x5f, -0x5f, text,
			self->font_light);
		POPUP_FLAGS(self->captions[i]) |= 8;
		ext_PopupInit(self->captions[i], 0);
	}
	self->accept = CGameMode_Team_NewPopup(0x69, -400, -140, *(void **)(table + 0x10d0), self->font_bold);
	POPUP_FLAGS(self->accept) |= 0x10;
	ext_PopupInit(self->accept, 0);
}

/* 0x0002ad30  Core/CGameMode_Team.cpp  loaded (vtable +0x2c): a text box with each character */
void CGameMode_Team_Loaded(struct CGameMode_Team *self)
{
	unsigned char box[0x3b0];
	int i;

	memset(box, 0, sizeof(box));
	((void (*)(int))FUN_00071880)(2);
	ext_PlayVoice(0x1e, -1, 0xff, 0, 0, 0);
	ext_FUN_00072720(self->list);
	for (i = 0; i < 4; i++)
	{
		float x;

		memset(box, 0, sizeof(box));
		self->slots[i] = i;
		*(int32_t *)(box + 0x288) = 1;
		*(int32_t *)(box + 0x290) = 0x2000;
		if (i == 3)
		{
			*(int32_t *)(box + 0x290) = 0x6000;
			*(uint32_t *)(box + 0x384) = 0x41200000;  /* 10.0f */
		}
		strcpy((char *)box + 0x2c4, (const char *)(DAT_002900c8 + g_PlayerCharacter(i) * 0x108));
		x = (float)((double)i * 200.0 - 300.0);
		*(int32_t *)(box + 0x28c) = 4;
		*(uint32_t *)(box + 0x2a8) = 0xc2c80000;  /* -100 */
		*(uint32_t *)(box + 0x2ac) = 0x442f0000;  /* 700 */
		*(uint32_t *)(box + 0x0c) = 0xc2c80000;
		*(uint32_t *)(box + 0x10) = 0x442f0000;
		*(uint32_t *)(box + 0x14) = *(uint32_t *)(box + 0x2b0);
		*(float *)(box + 0x2a4) = x;
		*(uint32_t *)(box + 0x328) = 0x3f800000;
		*(uint32_t *)(box + 0x320) = 0x3f800000;
		*(uint32_t *)(box + 0x318) = 0x3f800000;
		*(uint32_t *)(box + 0x25c) = 0x43800000;  /* 256 */
		*(float *)(box + 0x08) = x;
		self->boxes[i] = ext_TextCreate(box);
		CGameMode_Team_PlayAnim(self, i, 0x20, 1, 0, 1.0f, 0.0f);
	}
}

/* 0x0002aeb0  Core/CGameMode_Team.cpp  start the next swap: random first, the last one is the
swap the score table asks for (core_modes_b 0x0001bd70) */
void CGameMode_Team_Shuffle(struct CGameMode_Team *self)
{
	unsigned char *a, *b;
	uint32_t keep[2];
	float ax, ay;
	int slot;
	void *sample;

	self->swap_a = -1;
	if (self->shuffles == 1)
	{
		int target = DAT_002b9c58, other, value, i;

		((void (*)(void *))FUN_0001bd70)(DAT_002b9968);
		if (self->last == DAT_002b9c58 || self->last + 1 == DAT_002b9c58)
		{
			other = DAT_002b9c64;
			target = DAT_002b9c58;
		}
		else
		{
			other = DAT_002b9c58;
			target = DAT_002b9c64;
		}
		self->swap_b = other;
		value = self->slots[target];
		for (i = 0; i < 4; i++)
		{
			if (i == other)
				continue;
			if (self->slots[i] == (value % 2 == 0 ? value + 1 : value - 1))
			{
				self->swap_a = i;
				break;
			}
		}
		if (self->swap_a == -1)
		{
			self->shuffles = 0;
			return;
		}
	}
	else
	{
		int pick;

		do
		{
			DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
			pick = (int)((DAT_00285630 >> 16) & 0x7fff) % 3;
			self->swap_a = pick;
		} while (pick == self->last);
		self->swap_b = pick + 1;
	}
	self->last = self->swap_a;
	self->swap_time = CORE_VAR(float, 0x00283c68);
	self->shuffles--;
	/* swap the target places (+0x64: x, y, z, w) of the two boxes */
	a = self->boxes[self->swap_a];
	b = self->boxes[self->swap_b];
	ax = *(float *)(a + 0x64);
	ay = *(float *)(a + 0x68);
	keep[0] = *(uint32_t *)(a + 0x6c);
	keep[1] = *(uint32_t *)(a + 0x70);
	memcpy(a + 0x64, b + 0x64, 0x10);
	*(float *)(b + 0x64) = ax;
	*(float *)(b + 0x68) = ay;
	memcpy(b + 0x6c, keep, 8);
	slot = self->slots[self->swap_a];
	self->slots[self->swap_a] = self->slots[self->swap_b];
	self->slots[self->swap_b] = slot;
	sample = ext_FindResource(0, "selectplayer1.wav", 2, 0xdefa);
	ext_PlaySample(sample, 0xff, 0, 0, 0);
}

/* 0x0002b0a0  Core/CGameMode_Team.cpp  process (vtable +0x18): animate a swap on a half
circle, start the next one, then form the teams and wait for A; fade out and pop */
void CGameMode_Team_Process(struct CGameMode_Team *self, int top)
{
	int i;

	if (DAT_002ecdec != 0 || top == 0)
		return;
	((void (*)(void *))self->fader[2])(&self->fader);
	if (self->base.state != 0)
	{
		((void (*)(void *))self->base.vtbl[9])(self);
		return;
	}
	if (self->swap_time > 0.0f)
	{
		unsigned char *a = self->boxes[self->swap_a];
		unsigned char *b = self->boxes[self->swap_b];
		double t = (double)self->swap_time - (double)DAT_0028fda8;
		float mid_x;
		double mid_z, dx, angle, c, s_f;

		if (t < 0.0)
			t = 0.0;
		self->swap_time = (float)t;
		mid_x = (float)(((double)*(float *)(a + 0x64) + (double)*(float *)(b + 0x64)) * 0.5);
		mid_z = ((double)*(float *)(a + 0x6c) + (double)*(float *)(b + 0x6c)) * 0.5;
		dx = (double)mid_x - (double)*(float *)(a + 0x64);
		angle = t * (double)3.1415925f / (double)CORE_VAR(float, 0x00283c68);
		s_f = (double)(float)sin(angle);
		c = cos(angle);
		*(float *)(a + 0x300) = (float)((double)CORE_VAR(float, 0x00283c6c) * c * dx + (double)mid_x);
		*(float *)(a + 0x308) = (float)((double)CORE_VAR(float, 0x00283c6c) * s_f * dx + mid_z);
		*(float *)(b + 0x300) = (float)((double)mid_x - (double)CORE_VAR(float, 0x00283c6c) * c * dx);
		*(float *)(b + 0x308) = (float)(mid_z - (double)CORE_VAR(float, 0x00283c6c) * s_f * dx);
	}
	else if (self->shuffles != 0)
		CGameMode_Team_Shuffle(self);
	else if (self->state == 0.0f)
	{
		/* the teams stand: show the captions, walk to the team places */
		self->state = 1.0f;
		CGameMode_Team_ShowPopup(self, self->accept);
		((void (*)(Popup, int))VCALL(self->title, 5))(self->title, 0);
		self->title = NULL;
		for (i = 0; i < 2; i++)
		{
			CGameMode_Team_ShowPopup(self, self->team_captions[i]);
			((void (*)(Popup, int))VCALL(self->team_captions[i], 4))(self->team_captions[i], 0);
		}
		for (i = 0; i < 4; i++)
		{
			int slot;
			unsigned char *box;
			int *player = (int *)CORE_ADDR(0x002baef4) + i * 10;   /* +0x20 team, +0x24 place */

			CGameMode_Team_PlayAnim(self, i, 0, 1, 0, 1.0f, 0.03f);
			slot = self->slots[i];
			*(float *)((unsigned char *)self->captions[i] + 0x10) =
				(float)((double)((slot / 2) * 0x91) + (double)slot * 85.0 - 520.0);
			CGameMode_Team_ShowPopup(self, self->captions[i]);
			((void (*)(Popup, int))VCALL(self->captions[i], 4))(self->captions[i], 0);
			box = self->boxes[i];
			memcpy(box + 0x64, box + 0x300, 0x10);
			*(uint32_t *)(box + 0x2e0) = 0x40a00000;  /* 5.0f */
			if (self->slots[i] < 2)
			{
				player[0] = 0;
				player[1] = self->slots[i];
				CORE_VAR(int, 0x002baec0 + self->slots[i] * 4) = i;
				if (self->slots[i] == 1)
					*(float *)(box + 0x64) = *(float *)(box + 0x64) - 75.0f;
			}
			else
			{
				player[0] = 1;
				player[1] = self->slots[i] - 2;
				CORE_VAR(int, 0x002baec0 + self->slots[i] * 4) = i;
				if (self->slots[i] == 2)
					*(float *)(box + 0x64) = *(float *)(box + 0x64) + 75.0f;
			}
		}
	}
	else if (self->state > 1.0f)
	{
		self->state = DAT_0028fda8 * 6.0f + self->state;
		if (self->state >= 265.0f)
		{
			CGameModeStack_Pop(&g_ModeStack, 1);
			return;
		}
	}
	ext_FUN_00072720(self->list);
	ext_TextUpdateAll();
}

/* 0x0025667c: the CGameMode_Team vtable */
const void *const vtable_0025667c[14] = {
	(const void *)CGameMode_Team_Destroy,  /* [0] */
	(const void *)FUN_001d5950,             /* [1] */
	(const void *)FUN_0001a910,             /* [2] resume (core_modes_a) */
	(const void *)CGameMode_Team_Popped,    /* [3] */
	(const void *)CGameMode_Team_Pushed,    /* [4] */
	(const void *)CMenuMode_Update,         /* [5] */
	(const void *)CGameMode_Team_Process,   /* [6] */
	(const void *)CGameMode_Team_Draw,      /* [7] */
	(const void *)FUN_001ec260,             /* [8] */
	(const void *)FUN_00012930,             /* [9] */
	(const void *)FUN_000128e0,             /* [10] */
	(const void *)CGameMode_Team_Loaded,    /* [11] */
	(const void *)FUN_001d5950,             /* [12] */
	(const void *)CGameMode_Team_Button,    /* [13] */
};
