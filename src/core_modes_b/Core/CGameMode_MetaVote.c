/*
CGAMEMODE_METAVOTE.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_MetaVote.cpp

After a round, each player turns the round score into chips and votes with them: a chip
falls from the player's stack onto the vote pile (button 1, "right") or goes back (button 3,
"left"); A or start ends the player's vote. CPU players keep a random part. The results go
to the player records (core) for the board game.

The constructor and destructor (0x00021820, 0x00021880) and the end of the vote
(0x000218e0) are unplaced in the function table; they are this class's and live here.
*/

#include "xtl.h"

#include "core_modes_b.h"

static const char cmb_file_metavote[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_MetaVote.cpp";

/* 0x00283898 / 0x002838b0  character portraits: greyed and lit (.data pointer tables) */
static const char *const cmb_portrait_grey[6] = {
	"dub_g.bmp", "fly_g.bmp", "geena_g.bmp", "zak_g.bmp", "jade_g.bmp", "jet_g.bmp",
};
static const char *const cmb_portrait_on[6] = {
	"dub_on.bmp", "fly_on.bmp", "geena_on.bmp", "zak_on.bmp", "jade_on.bmp", "jet_on.bmp",
};

/* bactor FUN_000380f0: advance an actor's animation (actor, node, frames, flags) */
static void cmb_actor_animate(void *actor, void *node, float frames, int flags)
{
	CMB_DUAL(FUN_000380f0, undefined4 (*)(int, undefined4, undefined4, undefined4),
		int (*)(void *, void *, float, int),
		((int)actor, (undefined4)node, cmb_fbits(frames), flags), (actor, node, frames, flags));
}

/* bdisplay FUN_00044810: near and far planes */
static void cmb_set_clip(float znear, float zfar)
{
	CMB_DUAL(FUN_00044810, void (*)(undefined4, undefined4), void (*)(float, float),
		(cmb_fbits(znear), cmb_fbits(zfar)), (znear, zfar));
}

#define cmb_render_state(state, value) (((void (*)(int, int, int))FUN_00044b00)((state), (value), 0))
#define cmb_play(sample, volume, frequency) \
	(((int (*)(int, int, int, int, int))FUN_000713a0)((sample), (volume), (frequency), 0, 0))
#define CMB_OD_SIZE 0x3b0
#define cmb_overlay_create(d) (((void *(*)(void *))FUN_00074500)(d))
#define cmb_wcscpy(dst, src) (((void (*)(void *, const unsigned short *))FUN_0004b130)((dst), (src)))

/* 0x00255a6c  CGameMode_MetaVote vtable */
const void *const vtable_00255a6c[14] = {
	(const void *)FUN_00021970, /* [0] scalar deleting destructor */
	(const void *)FUN_001d5950, /* [1] empty */
	(const void *)FUN_001d5950, /* [2] empty */
	(const void *)FUN_00021c40, /* [3] Stop */
	(const void *)FUN_00021990, /* [4] Load */
	(const void *)FUN_0002fe60, /* [5] core: button repeat dispatch */
	(const void *)FUN_00022f40, /* [6] Update */
	(const void *)FUN_00022010, /* [7] Render */
	(const void *)FUN_001ec260, /* [8] empty */
	(const void *)FUN_00012930, /* [9] core_modes_a: background-load poll */
	(const void *)FUN_000128e0, /* [10] core_modes_a: FinishedLoading */
	(const void *)FUN_000233b0, /* [11] Loaded */
	(const void *)FUN_001d5950, /* [12] empty */
	(const void *)FUN_00022280, /* [13] OnButton */
};

/* 0x00021820  (unplaced): constructor */
struct CGameMode_MetaVote *CGameMode_MetaVote_ctor(struct CGameMode_MetaVote *self)
{
	CMB_GAMEMODE_CTOR(self, "MetaVote");
	self->base.vtable = vtable_00255a6c;
	((void (*)(void *))FUN_00071050)(self->list);
	return self;
}

/* 0x00021880  (unplaced): destructor */
void CGameMode_MetaVote_dtor(struct CGameMode_MetaVote *self)
{
	self->base.vtable = vtable_00255a6c;
	((void (*)(void *))FUN_001d5950)(self->list);
	CMB_GAMEMODE_DTOR(self);
}

/* 0x000218e0  (unplaced): everybody voted; store the results in the player records */
void CGameMode_MetaVote_Finish(struct CGameMode_MetaVote *self)
{
	DAT_002baed4[0][7] = self->spent[0];
	DAT_002baed4[0][5] = self->chips[0] * 10;
	DAT_002baed4[1][7] = self->spent[1];
	DAT_002baed4[1][5] = self->chips[1] * 10;
	DAT_002baed4[2][7] = self->spent[2];
	DAT_002baed4[2][5] = self->chips[2] * 10;
	DAT_002baed4[3][7] = self->spent[3];
	DAT_002baed4[3][5] = self->chips[3] * 10;
	self->state = 4;
	self->timer = 0.0f;
}

/* 0x00021970  Core/CGameMode_MetaVote.cpp */
void *CGameMode_MetaVote_DeletingDtor(struct CGameMode_MetaVote *self, unsigned char flags)
{
	CGameMode_MetaVote_dtor(self);
	if ((flags & 1) != 0)
		CMB_DELETE(self);
	return self;
}

/* 0x00021990  Core/CGameMode_MetaVote.cpp */
void CGameMode_MetaVote_Load(struct CGameMode_MetaVote *self)
{
	int order[4];
	int player;
	int chips;
	int keep;
	int swapped;
	int temp;
	int i;

	((void (*)(void *))FUN_00046d40)(self->saved_state);
	self->saved_fov[0] = DAT_002be588;
	self->saved_fov[1] = DAT_002be58c;
	self->saved_clip[0] = DAT_002be580;
	self->saved_clip[1] = DAT_002be584;
	((void (*)(void *))FUN_00072720)(self->list);
	if (DAT_002b95d0 != 0)
		CMB_U8(DAT_002b95d0, 0x58) = 1;
	self->font_bold = CMB_RESOURCE4(0, "handgotb.ttf", 3, 0xdefa);
	self->font_light = CMB_RESOURCE4(0, "handgotl.ttf", 3, 0xdefa);
	if (DAT_002bae8c == 0) {
		DAT_002baed4[0][1] = 1;
		DAT_002bae8c = 1;
	}
	self->voting = 0;
	for (player = 0; player < DAT_002bae94; player++) {
		self->chips[player] = g_score_table.rounds[g_score_table.round].sum[player];
		self->spent[player] = 0;
		self->active[player] = 1;
		self->voting++;
		if (DAT_002baed4[player][1] == 0 || DAT_003055e0 != 0) {
			/* a CPU player keeps 10% to 10% + 2k + 29% of the chips, at least 3 to vote */
			chips = self->chips[player];
			keep = ((cmb_rand() % (DAT_0029074c[player * 0x18] * 2 + 0x1e)) + 10) * chips / 100;
			self->keep[player] = keep;
			if (chips - keep < 3)
				self->keep[player] = chips - 3;
			if (self->keep[player] < 0)
				self->keep[player] = 0;
		}
		self->tokens[player] = ((void *(*)(unsigned int, int, const char *, int, unsigned int))FUN_0004c4b0)(
			0x3c0, 0, cmb_file_metavote, 0xac, 0x2001);
		self->token_count[player] = 0;
	}
	((void *(*)(void *, unsigned int))FUN_000382f0)(self->light, 4);
	((void (*)(void *, const float *))FUN_00038550)(self->light, DAT_00283888);
	((void (*)(void *, int, int, int))FUN_00038470)(self->light, 0x7f, 0x7f, 0x7f);
	self->blink = 0.0f;
	self->timer = 0.0f;
	self->state = 1;
	/* place by chips, highest first (bubble sort) */
	for (i = 0; i < DAT_002bae94; i++)
		order[i] = i;
	do {
		swapped = 0;
		if (DAT_002bae94 - 1 < 1)
			break;
		for (i = 0; i < DAT_002bae94 - 1; i++) {
			temp = order[i];
			if (self->chips[temp] < self->chips[order[i + 1]]) {
				swapped = 1;
				order[i] = order[i + 1];
				order[i + 1] = temp;
			}
		}
	} while (swapped);
	for (i = 0; i < DAT_002bae94; i++)
		self->place[order[i]] = i;
	self->package = ((int (*)(const char *, int))FUN_00071f90)("Packages\\ScoreScreen", 0);
	self->base.state = 2;
}

/* 0x00021c40  Core/CGameMode_MetaVote.cpp */
void CGameMode_MetaVote_Stop(struct CGameMode_MetaVote *self)
{
	int player;
	int k;

	((int (*)(void *, void *))FUN_000710b0)((char *)DAT_002b95d0 + 0x74,
		DAT_002ba228 == 0 ? NULL : (char *)DAT_002ba228 + 4);
	if (DAT_002ba228 != 0)
		CMB_VFN(DAT_002ba228, 0x00, void *(*)(void *, int))((void *)DAT_002ba228, 1);
	DAT_002ba228 = 0;
	cmb_set_clip(self->saved_clip[0], self->saved_clip[1]);
	((void (*)(float, float))FUN_00044a00)(self->saved_fov[0], self->saved_fov[1]);
	((void (*)(void *))FUN_00046d60)(self->saved_state);
	for (player = 0; player < 4; player++) {
		for (k = 0; k < 0x1e; k++) {
			if (self->tokens[player][k].actor != NULL)
				((void (*)(void *))FUN_00034930)(self->tokens[player][k].actor);
		}
		CMB_DELETE(self->tokens[player]);
		self->tokens[player] = NULL;
	}
	((void (*)(void *))FUN_000384d0)(self->light);
	if (self->package != 0)
		((void (*)(int))FUN_0004db40)(self->package);
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(void))FUN_00074ba0)();
}

/* 0x00021d50  Core/CGameMode_MetaVote.cpp: the falling and bouncing chips */
void CGameMode_MetaVote_MoveTokens(struct CGameMode_MetaVote *self)
{
	struct CMetaToken *token;
	float stretch;
	int player;
	int volume;
	int left;

	for (player = 0; player < DAT_002bae94; player++) {
		token = self->tokens[player];
		for (left = self->token_count[player]; left != 0; left--, token++) {
			cmb_actor_animate(token->actor, CMB_PTR(token->actor, 0x134), DAT_0028fda8 * 60.0f, 0);
			CMB_F32(token->actor, 0xc0) = token->y;
			CMB_F32(token->actor, 0xc4) = token->z;
			stretch = fabsf(token->velocity) * DAT_002838d0 + 1.0f;
			CMB_F32(token->actor, 0xd0) = DAT_002838d4 * stretch;
			CMB_F32(token->actor, 0xd4) = DAT_002838d4 / stretch;
			if (token->gravity >= 0.0f) {
				if (token->y < token->floor)
					token->velocity = DAT_0028fda8 * token->gravity + token->velocity;
			} else {
				if (token->floor < token->y)
					token->velocity = DAT_0028fda8 * token->gravity + token->velocity;
			}
			token->y = DAT_0028fda8 * token->velocity + token->y;
			if ((token->gravity > 0.0f && token->y > token->floor) ||
			    (token->gravity < 0.0f && token->y < token->floor)) {
				/* bounce */
				volume = (int)(fabsf(token->velocity) * 40.0f);
				if (volume > 0x80)
					volume = 0x80;
				cmb_play(self->chink, volume, cmb_rand() % 2000 + 21000);
				token->y = token->floor;
				token->velocity = DAT_002838cc * token->velocity;
				if (fabsf(token->velocity) < 0.5f)
					token->velocity = 0.0f;
			}
			if (self->state == 4 && token->y < -210.0f) {
				/* a returned chip left the screen */
				cmb_play(self->chink, 0x80, cmb_rand() % 2000 + 21000);
				if (self->rain != 0) {
					((int (*)(int, int, int, int, int))FUN_00054ba0)(self->rain, 0xff, -100,
						cmb_rand() % 5000 + 22000, 0);
					self->rain = 0;
				}
				self->token_count[player]--;
				break;
			}
		}
	}
}

/* 0x00022010  Core/CGameMode_MetaVote.cpp */
void CGameMode_MetaVote_Render(struct CGameMode_MetaVote *self, int active)
{
	unsigned char saved[0x78];
	float eye[4];
	float at[4];
	float clip_near;
	float clip_far;
	float fov;
	float aspect;
	int player;
	int k;

	((void (*)(void *, int))FUN_00030310)(self, active);
	if (self->base.state != 0)
		return;
	((void (*)(void *))FUN_00046d40)(saved);
	aspect = DAT_002be58c;
	fov = DAT_002be588;
	clip_far = DAT_002be584;
	clip_near = DAT_002be580;
	((void (*)(int))FUN_00043b70)(1);
	eye[0] = 0.0f;
	eye[1] = 0.0f;
	eye[2] = -320.0f;
	eye[3] = 1.0f;
	at[0] = 0.0f;
	at[1] = 0.0f;
	at[2] = 0.0f;
	at[3] = 1.0f;
	((void (*)(float *, float *, float))FUN_000435f0)(eye, at, 0.0f);
	cmb_set_clip(100.0f, 1000.0f);
	((void (*)(float, float))FUN_00044a00)(1.5707964f, 0.0f);
	((void (*)(void))FUN_00043790)();
	((void (*)(int, int, int))FUN_00038440)(0, 0, 0);
	((void (*)(void *, int))FUN_00038370)(self->light, 1);
	cmb_render_state(8, 0);
	cmb_render_state(7, 3);
	cmb_render_state(5, 1);
	cmb_render_state(3, 1);
	cmb_render_state(6, 0);
	for (player = 0; player < 4; player++) {
		for (k = 0; k < self->token_count[player]; k++) {
			((void (*)(void *, int))FUN_00034bf0)(self->tokens[player][k].actor, 1);
			((int (*)(void *, int))FUN_00034d50)(self->tokens[player][k].actor, 1);
		}
	}
	cmb_render_state(3, 0);
	cmb_render_state(4, 1);
	cmb_render_state(7, 1);
	cmb_render_state(5, 0);
	cmb_render_state(6, 1);
	cmb_render_state(10, 1);
	for (player = 0; player < 4; player++) {
		for (k = 0; k < self->token_count[player]; k++)
			((int (*)(void *, int))FUN_00034d50)(self->tokens[player][k].actor, 4);
	}
	cmb_render_state(4, 0);
	cmb_set_clip(clip_near, clip_far);
	((void (*)(float, float))FUN_00044a00)(fov, aspect);
	((void (*)(void *))FUN_00046d60)(saved);
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(int))FUN_00073900)(1);
	if (self->state == 5)
		((void (*)(int, int))FUN_00071c20)((int)((1.0f - self->timer) * 265.0f), 1);
}

/* 0x00022280  Core/CGameMode_MetaVote.cpp */
int CGameMode_MetaVote_OnButton(struct CGameMode_MetaVote *self, int button, int pad)
{
	struct CMetaToken *token;
	char *overlay;

	if (self->state != 3)
		return 0;
	if (self->active[pad] == 0 && button != 6)
		return 0;
	switch (button) {
	case 1: /* vote a chip */
		if (self->chips[pad] > 0) {
			token = &self->tokens[pad][self->spent[pad]];
			token->gravity = DAT_002838c8;
			token->velocity = 0.0f;
			token->floor = 180.0f - (float)self->spent[pad] * 12.5f;
			self->spent[pad]++;
			self->chips[pad]--;
			CMB_PLAY_SAMPLE(10, 0x7f, 0, 0, 0);
			return 0;
		}
		break;
	case 3: /* take a chip back */
		if (self->spent[pad] > 3) {
			self->spent[pad]--;
			self->chips[pad]++;
			CMB_PLAY_SAMPLE(9, 0x7f, 0, 0, 0);
			token = &self->tokens[pad][self->spent[pad]];
			token->velocity = 0.0f;
			token->floor = (float)self->chips[pad] * 12.5f - 85.0f;
			token->gravity = -DAT_002838c8;
			return 0;
		}
		break;
	case 4:
	case 5: /* done */
		CMB_PLAY_SAMPLE(9, 0x7f, 0, 0, 0);
		self->active[pad] = 0;
		if (DAT_002baed4[pad][1] != 0 && self->timer == 0.0f)
			self->timer = 10.0f;
		CMB_I32(self->portrait[pad], 0x60) =
			CMB_RESOURCE4(0, cmb_portrait_on[DAT_002baed4[pad][0]], 0, 0xdefa);
		overlay = self->chips_text[pad];
		CMB_I32(overlay, 0x2c4) = 0x40;
		CMB_I32(overlay, 0x2bc) = -4;
		CMB_I32(overlay, 0x2f4) = 1;
		CMB_I32(self->button[pad], 0x2b8) = 0;
		self->voting--;
		if (self->voting == 0)
			CGameMode_MetaVote_Finish(self);
		break;
	case 6: /* back in */
		if (self->active[pad] == 0 && self->spent[pad] + self->chips[pad] > 3) {
			self->active[pad] = 1;
			CMB_I32(self->portrait[pad], 0x60) =
				CMB_RESOURCE4(0, cmb_portrait_grey[DAT_002baed4[pad][0]], 0, 0xdefa);
			overlay = self->chips_text[pad];
			CMB_I32(overlay, 0x2f4) = 1;
			CMB_I32(overlay, 0x2bc) = 4;
			CMB_F32(self->button[pad], 0x2b8) = 127.0f;
			self->voting++;
			return 0;
		}
		break;
	}
	return 0;
}

/* 0x00022530  Core/CGameMode_MetaVote.cpp: make chip `index` of a player, on the stack */
struct CMetaToken *CGameMode_MetaVote_MakeToken(struct CGameMode_MetaVote *self, int player, int index)
{
	struct CMetaToken *token;
	float y;
	int frames;
	void *actor;

	y = 180.0f - (float)index * 12.5f;
	token = &self->tokens[player][index];
	self->token_count[player] = index + 1;
	token->floor = y;
	token->f18 = 0.0f;
	token->velocity = 0.0f;
	token->gravity = DAT_002838c8;
	token->base = (110.0f - (float)(self->place[player] * 0x32)) + 20.0f;
	token->z = token->base;
	token->y = y;
	actor = ((void *(*)(void *, int, int, const char *, int))FUN_00036b20)(NULL,
		self->tile_model[player], 1, cmb_file_metavote, 0x38e);
	token->actor = actor;
	CMB_F32(actor, 0xd0) = 0.175f;
	CMB_F32(actor, 0xd4) = 0.175f;
	CMB_F32(actor, 0xd8) = 0.175f;
	CMB_F32(actor, 0xdc) = 1.0f;
	self->anim = ((int (*)(void *, const char *))FUN_00035c70)(token->actor, "default");
	if (self->anim == 0)
		DbgBreakPoint();
	frames = CMB_I32(self->anim, 8) - CMB_I32(self->anim, 4);
	((void (*)(void *, void *, int, int, int, int, int, int))FUN_00035770)(token->actor,
		CMB_PTR(token->actor, 0x134), self->anim, 0, cmb_rand() % frames + CMB_I32(self->anim, 4),
		1, 0, 0);
	return token;
}

/* set the common fields of a centred text overlay */
static void cmb_vote_text(unsigned int *desc, int font, float x, float y)
{
	CMB_I32(desc, 0x44) = font;
	CMB_F32(desc, 0x250) = (float)(DAT_002844fc[0] / 2);
	CMB_F32(desc, 0x254) = (float)(DAT_002844fc[1] / 2);
	CMB_F32(desc, 0x258) = (float)(DAT_002844fc[2] / 2);
	CMB_F32(desc, 0x2a4) = x;
	CMB_F32(desc, 0x2a8) = y;
	CMB_F32(desc, 0x2ac) = 0.0f;
}

/* 0x00022660  Core/CGameMode_MetaVote.cpp: build the screen */
void CGameMode_MetaVote_Start(struct CGameMode_MetaVote *self)
{
	unsigned int desc[CMB_OD_SIZE / 4];
	char name[36];
	struct CMetaToken *token;
	void *memory;
	float row;
	float arrow_y;
	float y;
	int player;
	int alpha;
	int k;

	memset(desc, 0, sizeof desc);
	DAT_002ba228 = 0;
	memory = CMB_NEW(0x24, cmb_file_metavote, 0x131);
	DAT_002ba228 = memory == NULL ? 0 : (int)((void *(*)(void *))FUN_00012730)(memory);
	((void (*)(void *, int))FUN_000137e0)((void *)DAT_002ba228, 1);

	/* the two headings */
	memset(desc, 0, sizeof desc);
	cmb_vote_text(desc, self->font_bold, -255.0f, 200.0f);
	CMB_I32(desc, 0x288) = 2;
	CMB_AT(desc, 0x24c, unsigned short) = 0;
	CMB_AT(desc, 0x24e, unsigned short) = 0;
	CMB_F32(desc, 0x25c) = 0.0f;
	CMB_I32(desc, 0x260) = 8;
	CMB_I32(desc, 0x298) = 1;
	CMB_I32(desc, 0x268) = 0;
	CMB_I32(desc, 0x264) = 0x7f;
	CMB_I32(desc, 0x28c) = 5;
	cmb_wcscpy((char *)desc + 0x48, CMB_TEXT(0x12b));
	cmb_overlay_create(desc);
	CMB_AT(desc, 0x24c, unsigned short) = 2;
	CMB_F32(desc, 0x2a4) = 255.0f;
	CMB_F32(desc, 0x2a8) = 200.0f;
	CMB_F32(desc, 0x2ac) = 0.0f;
	cmb_wcscpy((char *)desc + 0x48, CMB_TEXT(0x12a));
	cmb_overlay_create(desc);

	/* title and hint */
	CMB_I32(desc, 0x44) = self->font_light;
	CMB_AT(desc, 0x24e, unsigned short) = 3;
	CMB_F32(desc, 0x2a4) = 255.0f;
	CMB_F32(desc, 0x2a8) = -190.0f;
	CMB_F32(desc, 0x2ac) = 0.0f;
	CMB_F32(desc, 0x320) = 0.75f;
	CMB_F32(desc, 0x318) = 0.75f;
	CMB_I32(desc, 0x298) = 0;
	cmb_wcscpy((char *)desc + 0x48, CMB_TEXT(0x12c));
	self->title = cmb_overlay_create(desc);
	CMB_I32(desc, 0x28c) |= 1;
	CMB_F32(desc, 0x2a4) = -255.0f;
	CMB_F32(desc, 0x2a8) = -60.0f;
	CMB_F32(desc, 0x2ac) = 0.0f;
	CMB_I32(desc, 0x3c) = 1;
	CMB_AT(desc, 0x24c, unsigned short) = 0;
	CMB_AT(desc, 0x24e, unsigned short) = 0;
	CMB_F32(desc, 0x308) = 560.0f;
	CMB_F32(desc, 0x30c) = 140.0f;
	CMB_I32(desc, 0x260) = 8;
	CMB_I32(desc, 0x268) = 0;
	CMB_I32(desc, 0x264) = 0x7f;
	cmb_wcscpy((char *)desc + 0x48, CMB_TEXT(0x1a5));
	self->hint = cmb_overlay_create(desc);

	for (player = 0; player < 4; player++) {
		if (DAT_002baed4[player][0] == -1)
			DAT_002baed4[player][0] = player;
		row = (float)(self->place[player] * 0x32);
		alpha = self->active[player] != 0 ? 0x7f : 0x50;

		/* portrait */
		memset(desc, 0, sizeof desc);
		CMB_I32(desc, 0x288) = 0;
		CMB_I32(desc, 0x04) = CMB_RESOURCE4(0, cmb_portrait_on[DAT_002baed4[player][0]], 0, 0xdefa);
		CMB_I32(desc, 0x264) = alpha;
		CMB_F32(desc, 0x250) = 128.0f;
		CMB_F32(desc, 0x254) = 128.0f;
		CMB_F32(desc, 0x258) = 128.0f;
		CMB_F32(desc, 0x2a8) = (130.0f - row) + 20.0f;
		CMB_F32(desc, 0x25c) = 0.0f;
		CMB_F32(desc, 0x2a4) = -255.0f;
		CMB_F32(desc, 0x308) = 64.0f;
		CMB_F32(desc, 0x30c) = 64.0f;
		CMB_I32(desc, 0x260) = 8;
		CMB_I32(desc, 0x298) = 1;
		CMB_I32(desc, 0x268) = 0;
		CMB_I32(desc, 0x28c) = 5;
		self->portrait[player] = cmb_overlay_create(desc);

		/* A button */
		memset(desc, 0, sizeof desc);
		CMB_I32(desc, 0x288) = 0;
		CMB_I32(desc, 0x04) = CMB_RESOURCE4(0, "abutton.bmp", 0, 0xdefa);
		CMB_F32(desc, 0x250) = 128.0f;
		CMB_F32(desc, 0x254) = 128.0f;
		CMB_F32(desc, 0x2a8) = (120.0f - row) + 20.0f;
		CMB_F32(desc, 0x258) = 128.0f;
		CMB_F32(desc, 0x25c) = 0.0f;
		CMB_I32(desc, 0x28c) = 5;
		CMB_F32(desc, 0x2a4) = -210.0f;
		CMB_F32(desc, 0x308) = 32.0f;
		CMB_F32(desc, 0x30c) = 32.0f;
		self->button[player] = cmb_overlay_create(desc);

		/* arrows */
		memset(desc, 0, sizeof desc);
		CMB_I32(desc, 0x288) = 0;
		CMB_I32(desc, 0x04) = CMB_RESOURCE4(0, "left_arrow.tga", 0, 0xdefa);
		CMB_F32(desc, 0x2a4) = -120.0f;
		CMB_F32(desc, 0x30c) = 32.0f;
		arrow_y = (126.0f - row) + 20.0f;
		CMB_F32(desc, 0x308) = 32.0f;
		CMB_F32(desc, 0x250) = 128.0f;
		CMB_F32(desc, 0x254) = 128.0f;
		CMB_F32(desc, 0x258) = 128.0f;
		CMB_F32(desc, 0x25c) = 40.0f;
		CMB_F32(desc, 0x2a8) = arrow_y;
		self->left_arrow[player] = cmb_overlay_create(desc);
		memset(desc, 0, sizeof desc);
		CMB_I32(desc, 0x288) = 0;
		CMB_I32(desc, 0x04) = CMB_RESOURCE4(0, "right_arrow.tga", 0, 0xdefa);
		CMB_F32(desc, 0x2a4) = 200.0f;
		CMB_F32(desc, 0x30c) = 32.0f;
		CMB_F32(desc, 0x308) = 32.0f;
		CMB_F32(desc, 0x250) = 128.0f;
		CMB_F32(desc, 0x254) = 128.0f;
		CMB_F32(desc, 0x258) = 128.0f;
		CMB_F32(desc, 0x25c) = 40.0f;
		CMB_F32(desc, 0x2a8) = arrow_y;
		self->right_arrow[player] = cmb_overlay_create(desc);

		/* the two numbers */
		memset(desc, 0, sizeof desc);
		y = (110.0f - row) + 20.0f;
		CMB_I32(desc, 0x288) = 2;
		CMB_AT(desc, 0x24c, unsigned short) = 2;
		cmb_vote_text(desc, self->font_bold, -120.0f, y);
		CMB_AT(desc, 0x24e, unsigned short) = 1;
		CMB_F32(desc, 0x25c) = 0.0f;
		CMB_I32(desc, 0x264) = alpha;
		CMB_I32(desc, 0x260) = 8;
		CMB_I32(desc, 0x298) = 1;
		CMB_I32(desc, 0x268) = 0;
		CMB_I32(desc, 0x28c) = 5;
		cmb_wcscpy((char *)desc + 0x48, L"0");
		self->chips_text[player] = cmb_overlay_create(desc);
		memset(desc, 0, sizeof desc);
		cmb_vote_text(desc, self->font_bold, 234.0f, y);
		CMB_AT(desc, 0x24e, unsigned short) = 1;
		CMB_I32(desc, 0x298) = 1;
		CMB_I32(desc, 0x264) = alpha;
		CMB_I32(desc, 0x288) = 2;
		CMB_AT(desc, 0x24c, unsigned short) = 0;
		CMB_F32(desc, 0x25c) = 0.0f;
		CMB_I32(desc, 0x260) = 8;
		CMB_I32(desc, 0x268) = 0;
		CMB_I32(desc, 0x28c) = 5;
		cmb_wcscpy((char *)desc + 0x48, L"0");
		self->spent_text[player] = cmb_overlay_create(desc);
		memset(desc, 0, sizeof desc);

		/* the chip model of the character and the chips on the stack */
		sprintf(name, "colour_tile0%d_00.obe", DAT_002baed4[player][0]);
		self->tile_model[player] = ((int (*)(int, const char *, int, int))FUN_0004bb30)(0, name, 1, 0);
		if (self->tile_model[player] == 0)
			DbgBreakPoint();
		for (k = 0; k < self->chips[player]; k++) {
			token = CGameMode_MetaVote_MakeToken(self, player, k);
			y = (float)(self->chips[player] - k) * 12.5f - 85.0f;
			token->floor = y;
			token->y = y;
		}
	}
}

/* 0x00022f40  Core/CGameMode_MetaVote.cpp */
void CGameMode_MetaVote_Update(struct CGameMode_MetaVote *self, int active)
{
	struct CMetaToken *token;
	int player;
	int moved;
	int busy;

	if (DAT_002ecdec != 0)
		return;
	((void (*)(int))FUN_00030000)(active);
	if (self->base.state != 0) {
		CMB_VFN(self, 0x24, void (*)(void *))(self);
		return;
	}
	cmb_kick_off_and_wait_for_idle();
	switch (self->state) {
	case 1: /* deal three chips to everybody's vote pile */
		self->timer = self->timer - DAT_0028fda8;
		if (self->timer < 0.0f) {
			moved = 0;
			for (player = 0; player < DAT_002bae94; player++) {
				if (self->spent[player] >= 3)
					continue;
				if (self->chips[player] > 0) {
					token = &self->tokens[player][self->spent[player]];
					self->chips[player]--;
				} else {
					token = CGameMode_MetaVote_MakeToken(self, player, self->spent[player]);
					token->y = -400.0f;
				}
				token->gravity = DAT_002838c8;
				token->floor = 180.0f - (float)self->spent[player] * 12.5f;
				self->spent[player]++;
				moved++;
			}
			if (moved != 0) {
				self->timer = 10.0f;
				break;
			}
			for (player = 0; player < DAT_002bae94; player++) {
				if (self->active[player] != 0)
					CMB_I32(self->portrait[player], 0x60) =
						CMB_RESOURCE4(0, cmb_portrait_grey[DAT_002baed4[player][0]], 0, 0xdefa);
			}
			self->timer = 40.0f;
			self->state = 2;
		}
		break;
	case 2:
		self->timer = self->timer - DAT_0028fda8;
		if (self->timer < 0.0f) {
			self->timer = 0.0f;
			self->state = 3;
			CMB_I32(self->hint, 0x2f4) = 1;
			CMB_I32(self->title, 0x2f4) = 1;
		}
		break;
	case 3: /* voting; CPU players press their buttons */
		for (player = 0; player < DAT_002bae94; player++) {
			if ((DAT_002baed4[player][1] == 0 || DAT_003055e0 != 0) && self->active[player] != 0 &&
			    DAT_002ba99c % 0xc == player * 3) {
				if (self->keep[player] < self->chips[player]) {
					CMB_VFN(self, CMB_VT_ONBUTTON, int (*)(void *, int, int))(self, 1, player);
				} else if (CMB_VFN(self, CMB_VT_ONBUTTON, int (*)(void *, int, int))(self, 4, player) == 1) {
					return;
				}
			}
			CMB_F32(self->left_arrow[player], 0x2b8) =
				(self->active[player] & (self->spent[player] > 3)) != 0 ? 127.0f : 40.0f;
			CMB_F32(self->right_arrow[player], 0x2b8) =
				(self->active[player] & (self->chips[player] >= 10)) != 0 ? 127.0f : 40.0f;
			self->blink = DAT_0028fda8 + self->blink;
			if (self->blink > 60.0f)
				self->blink = self->blink - 60.0f;
			if (self->active[player] != 0 && DAT_002baed4[player][1] != 0)
				CMB_F32(self->button[player], 0x2b8) = self->blink < 30.0f ? 127.0f : 90.0f;
			swprintf((wchar_t *)((char *)self->chips_text[player] + 0xa4), L"%d", self->chips[player] * 10);
			swprintf((wchar_t *)((char *)self->spent_text[player] + 0xa4), L"%d", self->spent[player]);
		}
		break;
	case 4: /* send the unused chips away */
		self->timer = self->timer - DAT_0028fda8;
		if (self->timer < 0.0f) {
			for (player = 0; player < DAT_002bae94; player++) {
				if (self->chips[player] == 0)
					continue;
				token = &self->tokens[player][self->spent[player] + self->chips[player] - 1];
				token->gravity = -DAT_002838c8;
				token->floor = -300.0f;
				token->velocity = 0.0f;
				self->chips[player]--;
			}
			self->timer = 10.0f;
		}
		busy = 0;
		for (player = 0; player < DAT_002bae94; player++) {
			if (self->token_count[player] != self->spent[player])
				busy++;
		}
		if (DAT_002bae94 > 0 && busy != 0)
			break;
		((void (*)(int))FUN_00071ff0)(-10);
		self->state = 5;
		self->timer = 1.0f;
		return;
	case 5: /* fade out, then leave */
		self->timer = self->timer - DAT_0028fda8 * 0.033333335f;
		if (self->timer < 0.0f) {
			CMB_POP_MODE(1);
			return;
		}
		break;
	}
	CGameMode_MetaVote_MoveTokens(self);
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(void))FUN_00074be0)();
}

/* 0x000233b0  Core/CGameMode_MetaVote.cpp: after the package loaded */
void CGameMode_MetaVote_Loaded(struct CGameMode_MetaVote *self)
{
	((void (*)(int))FUN_00054690)(DAT_002bae6c);
	if (self->package != 0) {
		((int (*)(int, int, int, int))FUN_0004b870)(self->package, -1, 0xdefa, 0);
		self->rain = CMB_RESOURCE4(0, "chingching1.wav", 2, 0xdefa);
		self->chink = CMB_RESOURCE4(0, "orbchink.wav", 2, 0xdefa);
		self->base.state = 0;
	}
	CGameMode_MetaVote_Start(self);
	((int (*)(int, int, int, int, int, int))FUN_000719a0)(0x1c, -1, 0xff, 0, 0, 0);
	CMB_VFN(self, CMB_VT_UPDATE, void (*)(void *, int))(self, 1);
}
