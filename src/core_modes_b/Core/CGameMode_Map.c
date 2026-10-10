/*
CGAMEMODE_MAP.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_Map.cpp

The city map shown between rounds: six sites (power station, harbour, military base,
stadium, nightclub, building site), the order in which a party visits them, the camera
flights between them, the "round N - game" title and the game choice (core's game
selector, FUN_0002e1b0).

State machine (field `state`, updated by CGameMode_Map_Update):
   1 fade in        2 fade in, then round start   3 fade out, then leave
   4 slow turn      5 prompt countdown            6 random order draw
   7 wait           8 camera flight               9 round title countdown
  11 game choice   12 wait, then camera flight   13 fade others   14 site viewer
*/

#include "xtl.h"

#include "core_modes_b.h"

static const char cmb_file_map[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_Map.cpp";

/* ---------- constants of this file (.rdata) */

static const int cmb_map_black[4] = { 0, 0, 0, 100 };          /* 0x00255598 */
static const int cmb_map_wire[4] = { 20, 23, 26, 255 };        /* 0x002555a8 */
static const int cmb_map_wire_lit[4] = { 20, 90, 150, 255 };   /* 0x002555b8 */
static const float cmb_map_cam_start[4] = { 0.0f, 4000.0f, -6000.0f, 1.0f };   /* 0x00255608 */
static const float cmb_map_target_start[4] = { 0.0f, -750.0f, 0.0f, 1.0f };    /* 0x00255618 */
static const float cmb_map_cam_round[4] = { 0.0f, 2000.0f, -7500.0f, 1.0f };   /* 0x00255628 */
static const float cmb_map_target_round[4] = { 0.0f, 700.0f, 0.0f, 1.0f };    /* 0x00255638 */
#define CMB_MAP_ZFAR_NEAR 1500.0f   /* 0x00255648 */
#define CMB_MAP_ZFAR_FULL 10000.0f  /* 0x0025564c */

/* 0x00283874  light direction (-1, 0, 0, 1), .data */
__attribute__((weak)) float DAT_00283874[4] = { -1.0f, 0.0f, 0.0f, 1.0f };

/* 0x002837f4-0x00283868  per-site file and node names, .data (pointer tables) */
static const char *const cmb_map_site_model[6] = {
	"map_site.obe", "map_nightclub.obe", "map_stadium.obe",
	"map_military.obe", "map_harbour.obe", "map_power.obe",
};
static const char *const cmb_map_site_wire_model[6] = {
	"map_site_w.obe", "map_nightclub_w.obe", "map_stadium_w.obe",
	"map_military_w.obe", "map_harbour_w.obe", "map_power_w.obe",
};
static const char *const cmb_map_site_wire[6] = {
	"map_site.wir", "map_nightclub.wir", "map_stadium.wir",
	"map_military.wir", "map_harbour.wir", "map_power.wir",
};
static const char *const cmb_map_site_node[6] = {
	"site_pos", "nightclub_pos", "stadium_pos", "military_pos", "harbour_pos", "power_pos",
};
static const char *const cmb_map_samples[8] = {
	"computerhum.wav", "computeramb1.wav", "computeramb2.wav", "computerbleep1.wav",
	"computerbleep2.wav", "selectplayer1.wav", "spinningcube1.wav", "spinningcube2.wav",
};

/* 0x00283884  next logo animation (1-7), .data */
__attribute__((weak)) int DAT_00283884 = 1;

/* ---------- calls into other modules with float parameters */

/* bdisplay FUN_00044810: near and far planes */
static void cmb_set_clip(float znear, float zfar)
{
	CMB_DUAL(FUN_00044810, void (*)(undefined4, undefined4), void (*)(float, float),
		(cmb_fbits(znear), cmb_fbits(zfar)), (znear, zfar));
}

/* bdisplay FUN_00040750: draw a sprite at a 3D position */
static void cmb_draw_sprite(const float *pos, float width, float height, int a4, int texture,
                            int a6, int a7, int r, int g, int b, int alpha, int a12, int a13)
{
	CMB_DUAL(FUN_00040750,
		undefined4 (*)(undefined4, undefined4, undefined4, undefined4, undefined4, undefined4,
		               undefined4, undefined4, undefined4, undefined4, undefined4, undefined4,
		               undefined4),
		int (*)(const float *, float, float, int, int, int, int, int, int, int, int, int, int),
		((undefined4)pos, cmb_fbits(width), cmb_fbits(height), a4, texture, a6, a7, r, g, b,
		 alpha, a12, a13),
		(pos, width, height, a4, texture, a6, a7, r, g, b, alpha, a12, a13));
}

/* bactor FUN_00039e20: draw a shadow (wire model, shadow object, light direction, length) */
#define cmb_shadow_draw(actor, shadow, dir, length, flag) \
	(((void (*)(void *, void *, float *, float, int))FUN_00039e20)((actor), (shadow), (dir), (length), (flag)))

/* bdisplay render states (FUN_00044b00) and colour/blend state (FUN_000416c0) */
#define cmb_render_state(state, value) (((void (*)(int, int, int))FUN_00044b00)((state), (value), 0))
#define cmb_blend_reset() (((int (*)(int, int))FUN_000416c0)(0, 0))

/* bactor: prepare and draw an actor */
#define cmb_actor_prep(actor, flags) (((void (*)(void *, int))FUN_00034bf0)((actor), (flags)))
#define cmb_actor_draw(actor, passes) (((int (*)(void *, int))FUN_00034d50)((actor), (passes)))

/* classlib_ui FUN_000713a0: play a sample (sample, volume, frequency, flags, position) */
#define cmb_play(sample, volume, frequency) \
	(((int (*)(int, int, int, int, int))FUN_000713a0)((sample), (volume), (frequency), 0, 0))
/* engine_misc FUN_000546e0: stop a playing sample */
#define cmb_stop(handle) (((void (*)(int))FUN_000546e0)(handle))
/* classlib_ui FUN_000719a0: play commentary */
#define cmb_commentary(kind, choice) \
	(((int (*)(int, int, int, int, int, int))FUN_000719a0)((kind), (choice), 0xff, 0, 0, 0))

/* bkernel FUN_00051170: rotate a vector about the Y axis */
#define cmb_rotate_y(out, in, angle) (((void (*)(float *, const float *, float))FUN_00051170)((out), (in), (angle)))

/* 0x0025589c  CGameMode_Map vtable */
const void *const vtable_0025589c[14] = {
	(const void *)FUN_0001eb10, /* [0] scalar deleting destructor */
	(const void *)FUN_001d5950, /* [1] empty */
	(const void *)FUN_001d5950, /* [2] empty */
	(const void *)FUN_0001ddd0, /* [3] Resume: Stop */
	(const void *)FUN_000208a0, /* [4] Load */
	(const void *)FUN_0002fe60, /* [5] core: button repeat dispatch */
	(const void *)FUN_00021100, /* [6] Update */
	(const void *)FUN_00021630, /* [7] Render */
	(const void *)FUN_001ec260, /* [8] empty */
	(const void *)FUN_00012930, /* [9] core_modes_a: background-load poll */
	(const void *)FUN_000128e0, /* [10] core_modes_a: FinishedLoading */
	(const void *)FUN_00020b50, /* [11] Loaded */
	(const void *)FUN_001d5950, /* [12] empty */
	(const void *)FUN_000216a0, /* [13] OnButton */
};

/* 0x0001ddc0 (not in the function table: reached only through the array destructor
   iterators): CMapSite destructor, core FUN_0002f580 on the wire sprite */
static void cmb_map_site_dtor(struct CMapSite *site)
{
	((void (*)(void *))FUN_0002f580)(site->colour);
}

/* 0x0001ddd0  Core/CGameMode_Map.cpp */
void CGameMode_Map_Stop(struct CGameMode_Map *self)
{
	int i;

	DAT_002bb020 = 0;
	((void (*)(void))FUN_00054730)();
	((void (*)(void *, void *))FUN_0002ff40)(self, NULL);
	if (self->base.state == 0) {
		((void (*)(void *))FUN_000384d0)(self->light);
		for (i = 0; i < 6; i++) {
			if (CMB_I32(self->sites[i].shadow, 0x10) != 0)
				((void (*)(void *))FUN_000393b0)(self->sites[i].shadow);
			((void (*)(void *))FUN_00034930)(self->sites[i].actor);
			((void (*)(void *))FUN_00034930)(self->sites[i].wire);
		}
		((void (*)(void *))FUN_00034930)(self->city);
		((void (*)(int))FUN_0004db40)(self->package);
		((void (*)(int))FUN_0003f650)(self->grid);
	}
	((void (*)(void *))FUN_0002ffd0)(self);
	((void (*)(void))FUN_00074ba0)();
	if (self->selector != NULL)
		CMB_VFN(self->selector, 0x00, void *(*)(void *, int))(self->selector, 1);
}

/* 0x0001de90  Core/CGameMode_Map.cpp: zoom out to the whole map before the last round */
void CGameMode_Map_StartZoomOut(struct CGameMode_Map *self)
{
	int round = self->current;

	self->timer = 200.0f;
	self->state = 12;
	self->path_current = (float)(round - 1);
	if (g_score_table.round == DAT_002844a0[DAT_002bae74] - 1)
		cmb_commentary(0x2d, self->order[round]);
	else
		cmb_commentary(0x2e, self->order[round]);
}

/* 0x0001df10  Core/CGameMode_Map.cpp: start drawing the order of the sites */
void CGameMode_Map_StartDraw(struct CGameMode_Map *self)
{
	int i;

	self->state = 6;
	((void (*)(int))FUN_0002fb50)(DAT_002bae88);
	for (i = 0; i < 6; i++) {
		self->alpha_min[i] = 8;
		self->alpha[i] = 8;
		self->order[i] = -1;
		self->round_of[DAT_002844ac[i]] = -1;
	}
	self->rounds = 0;
	self->path_done = 0.0f;
	self->hum = cmb_play(self->samples[1], 0xc0, 0);
	self->timer2 = 4.0f;
	self->timer = 80.0f;
}

/* 0x0001dfb0  Core/CGameMode_Map.cpp: skip the draw, the fixed order */
void CGameMode_Map_SkipDraw(struct CGameMode_Map *self)
{
	int site;

	while (self->current < DAT_002bae88) {
		self->order[self->current] = DAT_002844ac[self->current];
		site = self->order[self->current];
		self->round_of[site] = self->current;
		self->alpha[site] = 0x100;
		self->current++;
		self->rounds = self->current;
	}
	self->current = 0;
	self->path_done = (float)self->rounds;
	site = self->order[0];
	self->alpha_min[site] = 0x100;
	self->alpha[site] = 0x100;
	self->timer = 120.0f;
	self->state = 4;
}

/* 0x0001e060  Core/CGameMode_Map.cpp: turn the camera slowly */
void CGameMode_Map_Turn(struct CGameMode_Map *self)
{
	cmb_rotate_y(self->cam_pos, cmb_map_cam_start, self->angle);
	self->angle = 0.008f * DAT_0028fda8 + self->angle;
}

/* 0x0001e0a0  Core/CGameMode_Map.cpp: wait for the game choice */
void CGameMode_Map_Choose(struct CGameMode_Map *self)
{
	int *choice;
	unsigned short *name;
	float width;
	float height;
	float pulse;
	int index;

	CMB_VFN(self->selector, 0x04, void (*)(void *))(self->selector);
	choice = CMB_PTR(self->selector, 0x64);
	if (choice != NULL) {
		self->game = choice[0];
		self->variant = choice[1];
		DAT_002baebc = choice[2];
		name = CMB_TEXT(self->variant + 0xcd + self->game * 4);
		((void (*)(void *, const unsigned short *))FUN_0004b130)((char *)self->subtitle + 0xa4, name);
		CMB_I32(self->subtitle, 0x2f0) = 1;
		self->state = 9;
		self->timer = 90.0f;
		if (DAT_002ba63c[g_score_table.round] == 0) {
			DAT_002baeac = self->game;
			DAT_002baeb0 = self->variant;
		} else {
			index = g_score_table.rounds[g_score_table.round].kind + g_score_table.round * 3;
			DAT_002baeac = DAT_002ba658[index];
			DAT_002baeb0 = DAT_00283ab8[index];
		}
		cmb_play(self->samples[7], 0xff, 0);
		if (DAT_002baebc != 0) {
			((void (*)(int, const unsigned short *, int, float *, float *))FUN_00046320)(
				self->font_bold, name,
				((int (*)(const unsigned short *))FUN_0004b110)(name), &width, &height);
			CMB_F32(self->vs, 0x300) = width * 0.5f + 40.0f;
			CMB_AT(self->vs, 0x54, unsigned short) = 0;
			CMB_I32(self->vs, 0x2f4) = 1;
			CMB_I32(self->vs, 0x2f8) = 1;
			CMB_AT(self->vs_text, 0x54, unsigned short) = 0;
			CMB_I32(self->vs_text, 0x2f4) = 1;
			CMB_I32(self->vs_text, 0x2f8) = 1;
			cmb_play(self->samples[5], 0xff, 0);
		}
	}
	pulse = DAT_0028fda8 + DAT_0028fda8 + self->shadow_alpha;
	if (pulse >= 60.0f)
		self->shadow_alpha = 60.0f;
	else
		self->shadow_alpha = pulse;
}

/* 0x0001e290  Core/CGameMode_Map.cpp: fade in */
void CGameMode_Map_StartFadeIn(struct CGameMode_Map *self)
{
	self->state = 1;
	self->fade = 0.0f;
	self->zfar = CMB_MAP_ZFAR_NEAR;
	self->hum = cmb_play(self->samples[1], 0x80, 0);
	if (self->kind == 1)
		cmb_commentary(0x2b, -1);
}

/* 0x0001e300  Core/CGameMode_Map.cpp */
void CGameMode_Map_FadeIn(struct CGameMode_Map *self)
{
	float value;
	int i;

	if (self->kind != 4) {
		cmb_rotate_y(self->cam_pos, cmb_map_cam_start, self->angle);
		self->angle = 0.008f * DAT_0028fda8 + self->angle;
	}
	for (i = 0; i < 6; i++)
		cmb_actor_prep(self->sites[i].actor, 0);
	if (self->fade >= 1.0f) {
		if (self->zfar < CMB_MAP_ZFAR_FULL) {
			value = 100.0f * DAT_0028fda8 + self->zfar;
			self->zfar = value;
			if (value >= CMB_MAP_ZFAR_FULL) {
				cmb_stop(self->hum);
				cmb_play(self->samples[4], 0xff, 0);
				switch (self->kind) {
				case 0:
					self->state = 5;
					self->timer = 240.0f;
					break;
				case 1:
					CGameMode_Map_StartDraw(self);
					return;
				case 4:
					CGameMode_Map_StartZoomOut(self);
					return;
				}
			}
		}
		return;
	}
	value = DAT_0028fda8 * 0.01f + self->fade;
	self->fade = value;
	if (value <= 1.0f)
		return;
	self->fade = 1.0f;
	cmb_stop(self->hum);
	self->hum = cmb_play(self->samples[0], 0xff, 44000);
	cmb_play(self->samples[3], 0xff, 0);
}

/* 0x0001e490  Core/CGameMode_Map.cpp: fade out, then leave */
void CGameMode_Map_FadeOut(struct CGameMode_Map *self)
{
	float value;

	if (self->fade > 0.0f) {
		value = self->fade - DAT_0028fda8 * 0.02f;
		self->fade = value;
		if (value < 0.0f)
			self->fade = 0.0f;
	}
	if (self->zfar > CMB_MAP_ZFAR_NEAR)
		self->zfar = self->zfar - (100.0f * DAT_0028fda8 + 100.0f * DAT_0028fda8);
	if (self->fade == 0.0f && self->zfar <= CMB_MAP_ZFAR_NEAR) {
		if (self->kind == 0) {
			CMB_POP_MODE(0);
			CMB_PUSH_MODE(DAT_002ba560, 1);
			return;
		}
		CMB_POP_MODE(1);
	}
}

/* draw the site models (alpha >= 0x100 lit, between 8 and 0x100 darkened) with the
   directional light; shared by the two render paths */
static void cmb_map_light(struct CGameMode_Map *self)
{
	float inverse[16];

	((void (*)(int, int, int))FUN_00038440)(0x3c, 0x32, 0x28);
	((void (*)(void *, int, int, int))FUN_00038470)(self->light, 0x46, 0x46, 0x5a);
	((void (*)(void *, int))FUN_00038370)(self->light, 1);
	((int (*)(float *, void *))FUN_000502f0)(inverse, DAT_002be370);
	((void (*)(float *, float *, const float *))FUN_000505c0)(self->light_dir, inverse, DAT_00283874);
	((void (*)(void *, const float *))FUN_00038550)(self->light, self->light_dir);
}

/* 0x0001e560  Core/CGameMode_Map.cpp: render while fading in or out (states 1-3) */
void CGameMode_Map_RenderFade(struct CGameMode_Map *self)
{
	int count;
	int i;

	((void (*)(int))FUN_00043b70)(1);
	((void (*)(float *, float *, float))FUN_000435f0)(self->cam_pos, self->cam_target, 0.0f);
	cmb_set_clip(1000.0f, 15000.0f);
	((void (*)(float, float))FUN_00044a00)(1.0471976f, 0.0f);
	((void (*)(void))FUN_00043790)();
	cmb_render_state(8, 0);
	if (self->zfar > 1000.0f) {
		cmb_set_clip(1000.0f, self->zfar);
		cmb_map_light(self);
		cmb_render_state(3, 1);
		cmb_render_state(6, 1);
		cmb_render_state(10, 0);
		cmb_render_state(4, 1);
		cmb_render_state(7, 3);
		cmb_render_state(5, 1);
		for (i = 0; i < 6; i++) {
			if (self->alpha[i] >= 0x100) {
				cmb_blend_reset();
				cmb_actor_prep(self->sites[i].actor, 0);
				cmb_actor_draw(self->sites[i].actor, 7);
			}
		}
		cmb_render_state(7, 3);
		cmb_render_state(5, 0);
		cmb_render_state(4, 1);
		for (i = 0; i < 6; i++) {
			if (self->alpha[i] != 0x100) {
				cmb_blend_reset();
				((void (*)(int))FUN_00035d50)(self->alpha[i]);
				cmb_actor_prep(self->sites[i].actor, 0);
				cmb_actor_draw(self->sites[i].actor, 7);
			}
		}
		cmb_render_state(4, 0);
		cmb_render_state(3, 0);
		cmb_render_state(4, 0);
		cmb_render_state(10, 0);
		cmb_render_state(6, 1);
		((void (*)(int))FUN_00035d50)(0x100);
		cmb_blend_reset();
		for (i = 0; i < 6; i++) {
			if (self->alpha[i] == 0)
				continue;
			((void (*)(void *))FUN_00043930)((char *)self->sites[i].actor + 0x40);
			if (self->alpha[i] < 0x100) {
				cmb_render_state(10, 1);
				memcpy(self->sites[i].colour, cmb_map_wire, sizeof cmb_map_wire);
				((void (*)(void *))FUN_0002f4a0)(self->sites[i].colour);
				cmb_render_state(10, 0);
			} else {
				memcpy(self->sites[i].colour, cmb_map_black, sizeof cmb_map_black);
				((void (*)(void *))FUN_0002f4a0)(self->sites[i].colour);
			}
		}
	}
	if (self->zfar < 15000.0f) {
		cmb_set_clip(self->zfar, 15000.0f);
		cmb_render_state(10, 1);
		count = (int)(self->fade * 84.0f) & ~1;
		if (count != 0) {
			cmb_blend_reset();
			((void (*)(void))FUN_000438b0)();
			((void (*)(int, int, int, int))FUN_0003f4a0)(1, self->grid, 0, count);
		}
		cmb_blend_reset();
		for (i = 0; i < 6; i++) {
			((void (*)(void *))FUN_00043930)((char *)self->sites[i].actor + 0x40);
			memcpy(self->sites[i].colour, cmb_map_wire_lit, sizeof cmb_map_wire_lit);
			((void (*)(void *, unsigned int))FUN_0002f500)(self->sites[i].colour,
				(unsigned int)(int)((float)self->sites[i].sprite_size * self->fade));
		}
	}
	cmb_set_clip(1000.0f, 15000.0f);
}

/* 0x0001e940  Core/CGameMode_Map.cpp: camera flight, t from 0 to 1 (eased) */
void CGameMode_Map_Fly(struct CGameMode_Map *self, float t)
{
	float u;

	if (t >= 0.5f) {
		u = (1.0f - t) * (1.0f - t);
		t = 1.0f - (u + u);
	} else {
		t = t * t + t * t;
	}
	((void (*)(float *, float *, float *, float))FUN_00050fa0)(self->cam_pos, self->pos_from, self->pos_to, t);
	((void (*)(float *, float *, float *, float))FUN_00050fa0)(self->cam_target, self->target_from, self->target_to, t);
}

/* 0x0001e9d0  Core/CGameMode_Map.cpp: out = a + b (3D) */
void cmb_vec3_add(float *out, float *a, float *b)
{
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

/* 0x0001ea00  Core/CGameMode_Map.cpp: out = s * v (3D) */
void cmb_vec3_scale(float *out, float *v, float s)
{
	out[0] = s * v[0];
	out[1] = s * v[1];
	out[2] = s * v[2];
}

/* 0x0001ea30  Core/CGameMode_Map.cpp: length (3D) */
float cmb_vec3_length(float *v)
{
	return (float)sqrt((double)v[2] * (double)v[2] + (double)v[1] * (double)v[1] +
		(double)v[0] * (double)v[0]);
}

/* 0x0001ea60  Core/CGameMode_Map.cpp */
struct CGameMode_Map *CGameMode_Map_ctor(struct CGameMode_Map *self, int kind)
{
	int i;

	CMB_GAMEMODE_CTOR(self, "Map");
	self->base.vtable = vtable_0025589c;
	/* eh vector constructor iterator: 6 sites of 100 bytes, FUN_0001ddb0 */
	for (i = 0; i < 6; i++)
		FUN_0001ddb0(&self->sites[i]);
	((void (*)(void *))FUN_00071050)(self->list);
	self->base.flags &= ~4u;
	if (kind != 0)
		self->base.flags |= 2;
	if (DAT_002bae8c == 0)
		DAT_002baed8[0][0] = 1;
	self->kind = kind;
	self->selector = NULL;
	return self;
}

/* 0x0001eb10  Core/CGameMode_Map.cpp */
void *CGameMode_Map_DeletingDtor(struct CGameMode_Map *self, unsigned char flags)
{
	CGameMode_Map_dtor(self);
	if ((flags & 1) != 0)
		CMB_DELETE(self);
	return self;
}

/* 0x0001eb30  Core/CGameMode_Map.cpp */
void CGameMode_Map_dtor(struct CGameMode_Map *self)
{
	int i;

	((void (*)(void *))FUN_001d5950)(self->list);
	/* eh vector destructor iterator, last element first */
	for (i = 5; i >= 0; i--)
		cmb_map_site_dtor(&self->sites[i]);
	CMB_GAMEMODE_DTOR(self);
}

/* overlay descriptor of classlib_ui FUN_00074500 (0x3b0 bytes); fields by offset */
#define CMB_OD_SIZE 0x3b0
#define CMB_OD_I32(d, off) CMB_I32(d, off)
#define CMB_OD_F32(d, off) CMB_F32(d, off)
#define CMB_OD_U16(d, off) CMB_AT(d, off, unsigned short)
#define CMB_OD_PTR_TEXT(d) CMB_AT(d, 0x248, const unsigned short *)
#define cmb_overlay_create(d) (((void *(*)(void *))FUN_00074500)(d))

/* 0x0001eba0  Core/CGameMode_Map.cpp: fonts, overlays and the ground grid */
void CGameMode_Map_CreateOverlays(struct CGameMode_Map *self)
{
	unsigned int desc[CMB_OD_SIZE / 4];
	unsigned int colour;
	float *vertex;
	float x;
	int line;

	memset(desc, 0, sizeof desc);
	((void (*)(void *))FUN_00072720)(self->list);
	self->font_bold = CMB_RESOURCE4(0, "handgotb.ttf", 3, 0xdefa);
	self->font_light = CMB_RESOURCE4(0, "handgotl.ttf", 3, 0xdefa);
	CMB_RESOURCE4(0, "ocraext.ttf", 3, 0xdefa);

	/* title */
	CMB_OD_I32(desc, 0x44) = self->font_light;
	CMB_OD_I32(desc, 0x288) = 2;
	CMB_OD_U16(desc, 0x24c) = 2;
	CMB_OD_U16(desc, 0x24e) = 0;
	CMB_OD_F32(desc, 0x250) = 127.0f;
	CMB_OD_F32(desc, 0x254) = 127.0f;
	CMB_OD_F32(desc, 0x258) = 127.0f;
	CMB_OD_F32(desc, 0x25c) = 127.0f;
	CMB_OD_F32(desc, 0x2a4) = 260.0f;
	CMB_OD_F32(desc, 0x2a8) = 300.0f;
	CMB_OD_F32(desc, 0x2ac) = 0.0f;
	CMB_OD_F32(desc, 0x08) = 260.0f;
	CMB_OD_F32(desc, 0x0c) = 200.0f;
	CMB_OD_F32(desc, 0x10) = 0.0f;
	CMB_OD_I32(desc, 0x28c) = 4;
	CMB_OD_F32(desc, 0x284) = 4.0f;
	self->title = cmb_overlay_create(desc);

	/* subtitle (the game name) */
	memset(desc, 0, sizeof desc);
	CMB_OD_I32(desc, 0x44) = self->font_bold;
	CMB_OD_I32(desc, 0x290) = 0x8000;
	CMB_OD_I32(desc, 0x288) = 2;
	CMB_OD_U16(desc, 0x24c) = 1;
	CMB_OD_U16(desc, 0x24e) = 1;
	CMB_OD_F32(desc, 0x250) = 127.0f;
	CMB_OD_F32(desc, 0x254) = 127.0f;
	CMB_OD_F32(desc, 0x258) = 127.0f;
	CMB_OD_F32(desc, 0x25c) = 127.0f;
	CMB_OD_F32(desc, 0x2a4) = 0.0f;
	CMB_OD_F32(desc, 0x2a8) = -300.0f;
	CMB_OD_F32(desc, 0x2ac) = 0.0f;
	CMB_OD_F32(desc, 0x08) = 0.0f;
	CMB_OD_F32(desc, 0x0c) = 50.0f;
	CMB_OD_F32(desc, 0x10) = 0.0f;
	CMB_OD_I32(desc, 0x28c) = 4;
	CMB_OD_F32(desc, 0x284) = 8.0f;
	self->subtitle = cmb_overlay_create(desc);

	/* "vs" picture */
	memset(desc, 0, sizeof desc);
	CMB_OD_I32(desc, 0x28c) = 3;
	CMB_OD_I32(desc, 0x288) = 0;
	CMB_OD_F32(desc, 0x250) = 127.0f;
	CMB_OD_F32(desc, 0x254) = 127.0f;
	CMB_OD_F32(desc, 0x258) = 127.0f;
	CMB_OD_F32(desc, 0x25c) = 0.0f;
	CMB_OD_F32(desc, 0x2a4) = 200.0f;
	CMB_OD_F32(desc, 0x2a8) = 50.0f;
	CMB_OD_F32(desc, 0x2ac) = 0.0f;
	CMB_OD_F32(desc, 0x30c) = 64.0f;
	CMB_OD_F32(desc, 0x308) = 64.0f;
	CMB_OD_I32(desc, 0x04) = CMB_RESOURCE4(0, "vs.bmp", 0, 0xdefa);
	CMB_OD_I32(desc, 0x28c) |= 1;
	CMB_OD_I32(desc, 0x260) = 0x14;
	CMB_OD_I32(desc, 0x268) = 0;
	CMB_OD_I32(desc, 0x264) = 0x7f;
	CMB_OD_F32(desc, 0x328) = 20.0f;
	CMB_OD_F32(desc, 0x320) = 20.0f;
	CMB_OD_F32(desc, 0x318) = 20.0f;
	CMB_OD_F32(desc, 0x340) = 1.0f;
	CMB_OD_F32(desc, 0x338) = 1.0f;
	CMB_OD_F32(desc, 0x330) = 1.0f;
	CMB_OD_F32(desc, 0x344) = 20.0f;
	CMB_OD_F32(desc, 0x33c) = 20.0f;
	CMB_OD_F32(desc, 0x334) = 20.0f;
	CMB_OD_F32(desc, 0x32c) = -0.6f;
	CMB_OD_F32(desc, 0x324) = -0.6f;
	CMB_OD_F32(desc, 0x31c) = -0.6f;
	self->vs = cmb_overlay_create(desc);
	CMB_AT(self->vs, 0x54, unsigned short) = 1;

	/* "vs" text, a child of the picture */
	memset(desc, 0, sizeof desc);
	CMB_OD_I32(desc, 0x268) = 0;
	CMB_OD_F32(desc, 0x25c) = 0.0f;
	CMB_OD_I32(desc, 0x44) = self->font_light;
	CMB_OD_I32(desc, 0x288) = 2;
	CMB_OD_I32(desc, 0x260) = 0x14;
	CMB_OD_I32(desc, 0x264) = 0x7f;
	CMB_OD_I32(desc, 0x28c) = 3;
	CMB_OD_I32(desc, 0x290) = 0x8000;
	CMB_OD_F32(desc, 0x250) = 127.0f;
	CMB_OD_F32(desc, 0x254) = 127.0f;
	CMB_OD_F32(desc, 0x258) = 127.0f;
	CMB_OD_U16(desc, 0x24c) = 1;
	CMB_OD_U16(desc, 0x24e) = 1;
	CMB_OD_F32(desc, 0x348) = 0.33f;
	CMB_OD_PTR_TEXT(desc) = CMB_TEXT(0x18c);
	CMB_OD_F32(desc, 0x328) = 20.0f;
	CMB_OD_F32(desc, 0x320) = 20.0f;
	CMB_OD_F32(desc, 0x318) = 20.0f;
	CMB_OD_F32(desc, 0x340) = 1.0f;
	CMB_OD_F32(desc, 0x338) = 1.0f;
	CMB_OD_F32(desc, 0x330) = 1.0f;
	CMB_OD_F32(desc, 0x344) = 20.0f;
	CMB_OD_F32(desc, 0x33c) = 20.0f;
	CMB_OD_F32(desc, 0x334) = 20.0f;
	CMB_OD_F32(desc, 0x32c) = -0.6f;
	CMB_OD_F32(desc, 0x324) = -0.6f;
	CMB_OD_F32(desc, 0x31c) = -0.6f;
	self->vs_text = ((void *(*)(void *, void *))FUN_00073ae0)(self->vs, desc);
	CMB_AT(self->vs_text, 0x54, unsigned short) = 1;

	/* the ground grid: 21 x 2 lines, 600 apart */
	((int (*)(int *, int, int, int, int))FUN_0003f500)(&self->grid, 0x54, 0x152, 0x18, 1);
	vertex = ((float *(*)(int, int))FUN_0003f710)(self->grid, 0);
	if (vertex == NULL) {
		CMB_DEBUG_PRINTF("Grid::Setup() : Failed to lock buffer!\n");
		return;
	}
	x = -6000.0f;
	for (line = 0; line < 0x15; line++) {
		colour = DAT_002bd640[100] | DAT_002bc490[50] | DAT_002bce18[10] | DAT_002bc898[255];
		vertex[0] = x;
		vertex[1] = -650.0f;
		vertex[2] = -6000.0f;
		CMB_AT(vertex, 0x18, unsigned int) = colour;
		vertex[9] = x;
		vertex[10] = -650.0f;
		vertex[11] = 6000.0f;
		CMB_AT(vertex, 0x3c, unsigned int) = colour;
		vertex[0x14] = x;
		vertex[0x13] = -650.0f;
		vertex[0x12] = -6000.0f;
		CMB_AT(vertex, 0x60, unsigned int) = colour;
		vertex[0x1d] = x;
		vertex[0x1c] = -650.0f;
		vertex[0x1b] = 6000.0f;
		CMB_AT(vertex, 0x84, unsigned int) = colour;
		x = x + 600.0f;
		vertex += 0x24;
	}
	((int (*)(int))FUN_0003f790)(self->grid);
}

/* 0x0001f180  Core/CGameMode_Map.cpp: the "press A" prompt, then fade in */
void CGameMode_Map_CreatePrompt(struct CGameMode_Map *self)
{
	unsigned int desc[CMB_OD_SIZE / 4];

	memset(desc, 0, sizeof desc);
	CMB_OD_I32(desc, 0x44) = self->font_bold;
	CMB_OD_I32(desc, 0x288) = 2;
	CMB_OD_I32(desc, 0x260) = 2;
	CMB_OD_I32(desc, 0x290) = 0x8000;
	CMB_OD_F32(desc, 0x2a4) = -250.0f;
	CMB_OD_F32(desc, 0x2a8) = 180.0f;
	CMB_OD_F32(desc, 0x2ac) = 0.0f;
	CMB_OD_F32(desc, 0x308) = 500.0f;
	CMB_OD_F32(desc, 0x30c) = 200.0f;
	CMB_OD_F32(desc, 0x250) = 127.0f;
	CMB_OD_F32(desc, 0x254) = 127.0f;
	CMB_OD_F32(desc, 0x258) = 127.0f;
	CMB_OD_F32(desc, 0x25c) = 0.0f;
	CMB_OD_I32(desc, 0x268) = 0;
	CMB_OD_I32(desc, 0x264) = 0x7f;
	CMB_OD_I32(desc, 0x28c) = 0xb;
	CMB_OD_F32(desc, 0x348) = 4.0f;
	CMB_OD_F32(desc, 0x34c) = 0.0f;
	CMB_OD_F32(desc, 0x350) = 0.1f;
	CMB_OD_F32(desc, 0x328) = 1.0f;
	CMB_OD_F32(desc, 0x320) = 1.0f;
	CMB_OD_F32(desc, 0x318) = 1.0f;
	CMB_OD_I32(desc, 0x298) = 1;
	CMB_OD_U16(desc, 0x24c) = 1;
	CMB_OD_U16(desc, 0x24e) = 0;
	CMB_OD_I32(desc, 0x3c) = 1;
	CMB_OD_PTR_TEXT(desc) = CMB_TEXT(0x1b4);
	self->prompt = cmb_overlay_create(desc);
	CGameMode_Map_StartFadeIn(self);
}

/* 0x0001f2e0  Core/CGameMode_Map.cpp: start the flight to the current site */
void CGameMode_Map_StartFlight(struct CGameMode_Map *self)
{
	float dx;
	float dy;
	float dz;
	float scale;
	int site;
	int i;

	self->state = 8;
	self->timer = 100.0f;
	for (i = 0; i < 6; i++) {
		site = self->order[self->current];
		if (i == site)
			continue;
		dx = self->pos[i][0] - self->pos[site][0];
		dy = self->pos[i][1] - self->pos[site][1];
		dz = self->pos[i][2] - self->pos[site][2];
		scale = 5000.0f / (float)sqrt(dx * dx + dy * dy + dz * dz);
		self->far_pos[i][0] = dx * scale + self->pos[i][0];
		self->far_pos[i][1] = dy * scale + self->pos[i][1];
		self->far_pos[i][2] = dz * scale + self->pos[i][2];
	}
	self->timer = 40.0f;
	memcpy(self->target_from, self->cam_target, sizeof self->target_from);
	memcpy(self->target_to, cmb_map_target_round, sizeof self->target_to);
	memcpy(self->pos_from, self->cam_pos, sizeof self->pos_from);
	memcpy(self->pos_to, cmb_map_cam_round, sizeof self->pos_to);
	self->blend = 0.0f;
}

/* 0x0001f460  Core/CGameMode_Map.cpp: the round title, then the game choice */
void CGameMode_Map_StartRound(struct CGameMode_Map *self)
{
	unsigned int desc[CMB_OD_SIZE / 4];
	const unsigned short *name;
	float w_space;
	float half_space;
	float w_label;
	float w_name;
	float height;
	float left;
	void *memory;
	int site;

	DAT_002baeb4 = self->order[self->current];
	self->state = 11;
	memcpy(self->alpha, self->alpha_min, sizeof self->alpha);
	self->current = g_score_table.round;
	site = self->order[g_score_table.round];
	self->path_alpha = 0xbe;
	swprintf((wchar_t *)((char *)self->title + 0xa4), L"%s %d - %s", CMB_TEXT(0x18b),
		g_score_table.round + 1, CMB_TEXT(site + 0xcb));
	CMB_F32(self->title, 0x2e0) = 4.0f;
	if (self->kind == 3) {
		self->timer = 300.0f;
		if (DAT_00283d74 != 0)
			self->timer = 300.0f + 300.0f;
	} else {
		memset(desc, 0, sizeof desc);
		name = CMB_TEXT(self->game_name + 0x117);
		((void (*)(int, const unsigned short *, int, float *, float *))FUN_00046320)(self->font_bold,
			CMB_TEXT(0x107), ((int (*)(const unsigned short *))FUN_0004b110)(CMB_TEXT(0x107)),
			&w_label, &height);
		((void (*)(int, const unsigned short *, int, float *, float *))FUN_00046320)(self->font_bold,
			name, ((int (*)(const unsigned short *))FUN_0004b110)(name), &w_name, &height);
		((void (*)(int, const unsigned short *, int, float *, float *))FUN_00046320)(self->font_bold,
			L" ", 1, &w_space, &height);
		CMB_OD_I32(desc, 0x44) = self->font_bold;
		CMB_OD_I32(desc, 0x288) = 2;
		CMB_OD_I32(desc, 0x290) = 0x8000;
		half_space = w_space * 0.5f;
		CMB_OD_U16(desc, 0x24c) = 2;
		CMB_OD_U16(desc, 0x24e) = 0;
		CMB_OD_F32(desc, 0x284) = 25.0f;
		CMB_OD_I32(desc, 0x294) = 1;
		CMB_OD_F32(desc, 0x250) = 127.0f;
		CMB_OD_F32(desc, 0x254) = 127.0f;
		CMB_OD_F32(desc, 0x258) = 127.0f;
		CMB_OD_F32(desc, 0x25c) = 127.0f;
		left = (half_space + w_label) - (w_space + w_name + w_label) * 0.5f;
		CMB_OD_F32(desc, 0x2a8) = 150.0f;
		CMB_OD_F32(desc, 0x2ac) = 0.0f;
		CMB_OD_F32(desc, 0x08) = left - half_space;
		CMB_OD_F32(desc, 0x2a4) = CMB_OD_F32(desc, 0x08) - 400.0f;
		CMB_OD_F32(desc, 0x0c) = 150.0f;
		CMB_OD_F32(desc, 0x10) = 0.0f;
		CMB_OD_I32(desc, 0x28c) = 4;
		((void (*)(void *, const unsigned short *))FUN_0004b130)((char *)desc + 0x48, CMB_TEXT(0x107));
		self->label_left = cmb_overlay_create(desc);
		CMB_OD_I32(desc, 0x28c) |= 4;
		CMB_OD_F32(desc, 0x08) = w_space * 0.5f + left;
		CMB_OD_U16(desc, 0x24c) = 0;
		CMB_OD_F32(desc, 0x2a4) = CMB_OD_F32(desc, 0x08) + 400.0f;
		CMB_OD_F32(desc, 0x0c) = 150.0f;
		CMB_OD_F32(desc, 0x10) = 0.0f;
		CMB_OD_F32(desc, 0x2a8) = 150.0f;
		CMB_OD_F32(desc, 0x2ac) = 0.0f;
		((void (*)(void *, const unsigned short *))FUN_0004b130)((char *)desc + 0x48, name);
		self->label_right = cmb_overlay_create(desc);
		memory = CMB_NEW(0x90, cmb_file_map, 0x3ca);
		self->selector = memory == NULL ? NULL : ((void *(*)(void *, int))FUN_0002e1b0)(memory, site);
		cmb_play(self->samples[6], 0xff, 0);
		self->timer2 = 2.0f;
		self->timer = 2.0f;
	}
	memcpy(self->cam_pos, cmb_map_cam_round, sizeof self->cam_pos);
	memcpy(self->cam_target, cmb_map_target_round, sizeof self->cam_target);
}

/* 0x0001f8c0  Core/CGameMode_Map.cpp: draw the order of the sites, one per beat */
void CGameMode_Map_DrawOrder(struct CGameMode_Map *self)
{
	int pick;
	int site;
	int i;

	self->timer = self->timer - DAT_0028fda8;
	self->timer2 = self->timer2 - DAT_0028fda8;
	if (self->timer2 < 0.0f) {
		/* flash a random free site */
		for (i = 0; i < 6; i++) {
			if (self->round_of[i] == -1) {
				self->alpha[i] = 8;
				self->alpha_min[i] = 8;
			}
		}
		do
			pick = cmb_rand() % 6;
		while (self->round_of[pick] != -1);
		self->alpha[pick] = 100;
		self->order[self->current] = pick;
		self->timer2 = 4.0f;
	}
	if (self->timer < 0.0f) {
		/* the next site of the fixed order */
		site = DAT_002844ac[self->current];
		if (self->current == 0)
			cmb_commentary(0x2c, site);
		self->order[self->current] = site;
		self->round_of[site] = self->current;
		self->alpha[site] = 0x100;
		self->alpha_min[site] = 0x100;
		for (i = 0; i < 6; i++) {
			if (self->round_of[i] == -1) {
				self->alpha[i] = 8;
				self->alpha_min[i] = 8;
			}
		}
		self->current++;
		self->timer = 80.0f;
		self->rounds = self->current;
		cmb_play(self->samples[3], 0xff, 0);
		if (self->current == DAT_002bae88) {
			cmb_stop(self->hum);
			self->current = 0;
			self->timer = 120.0f;
			self->state = 4;
		}
	}
	cmb_rotate_y(self->cam_pos, cmb_map_cam_start, self->angle);
	self->angle = 0.008f * DAT_0028fda8 + self->angle;
}

/* 0x0001fac0  Core/CGameMode_Map.cpp: dim the other sites. The original subtracts
   (frame time < 0) from the timer, so the timer only changes for negative frame times. */
void CGameMode_Map_DimOthers(struct CGameMode_Map *self)
{
	int step;
	int value;
	int i;
	float path;

	self->timer = self->timer - (float)(DAT_0028fda8 < 0.0f ? 1 : 0);
	if (self->timer != 0.0f) {
		self->angle = 0.0f;
		self->state = 9;
		CGameMode_Map_StartRound(self);
		return;
	}
	for (i = 0; i < 6; i++) {
		if (self->order[self->current] == i)
			continue;
		step = (int)(8.0f * DAT_0028fda8);
		value = self->alpha[i] - step;
		self->alpha[i] = self->alpha_min[i] > value ? self->alpha_min[i] : value;
	}
	path = (float)self->path_alpha - 8.0f * DAT_0028fda8;
	if (!(190.0f <= path))
		path = 190.0f;
	self->path_alpha = (int)path;
}

/* 0x0001fcc0  Core/CGameMode_Map.cpp: the site viewer, the camera circles the city */
void CGameMode_Map_View(struct CGameMode_Map *self)
{
	float offset[4];
	float angle;

	cmb_rotate_y(self->light_dir, DAT_00283874, self->angle);
	((void (*)(void *, const float *))FUN_00038550)(self->light, self->light_dir);
	cmb_rotate_y(offset, cmb_map_cam_round, self->angle);
	self->cam_pos[0] = offset[0] + self->cam_target[0];
	self->cam_pos[1] = offset[1] + self->cam_target[1];
	self->cam_pos[2] = offset[2] + self->cam_target[2];
	angle = 0.005f * DAT_0028fda8 + self->angle;
	self->angle = angle;
	if (angle > 6.2831855f)
		self->angle = angle - 6.2831855f;
}

/* 0x0001fd80  Core/CGameMode_Map.cpp: render the map */
void CGameMode_Map_Render3D(struct CGameMode_Map *self)
{
	int shadows;
	int i;

	((void (*)(int))FUN_00043b70)(1);
	((void (*)(float *, float *, float))FUN_000435f0)(self->cam_pos, self->cam_target, 0.0f);
	cmb_set_clip(1000.0f, 15000.0f);
	((void (*)(float, float))FUN_00044a00)(1.0471976f, 0.0f);
	((void (*)(void))FUN_00043790)();
	cmb_render_state(6, 1);
	cmb_render_state(8, 0);
	cmb_render_state(3, 0);
	cmb_render_state(10, 0);
	cmb_render_state(3, 1);
	cmb_render_state(7, 3);
	cmb_render_state(5, 1);
	cmb_map_light(self);
	for (i = 0; i < 6; i++) {
		if (self->alpha[i] >= 0x100) {
			cmb_blend_reset();
			cmb_actor_prep(self->sites[i].actor, 0);
			cmb_actor_draw(self->sites[i].actor, 7);
		}
	}
	cmb_render_state(7, 3);
	cmb_render_state(5, 0);
	cmb_render_state(4, 1);
	for (i = 0; i < 6; i++) {
		if (self->alpha[i] != 0 && self->alpha[i] != 0x100) {
			cmb_blend_reset();
			((void (*)(int))FUN_00035d50)(self->alpha[i]);
			cmb_actor_prep(self->sites[i].actor, 0);
			cmb_actor_draw(self->sites[i].actor, 7);
		}
	}
	cmb_render_state(4, 0);
	cmb_render_state(3, 0);
	cmb_render_state(7, 2);
	cmb_render_state(5, 0);
	((void (*)(int))FUN_00035d50)(0x100);
	cmb_render_state(10, 0);
	cmb_render_state(6, 1);
	if (self->shadow_alpha > 0.0f) {
		shadows = 0;
		for (i = 0; i < 6; i++) {
			if (self->alpha[i] < 0x100)
				continue;
			if (CMB_I32(self->sites[i].shadow, 0x10) != 0)
				((void (*)(void *))FUN_000393b0)(self->sites[i].shadow);
			cmb_shadow_draw(self->sites[i].wire, self->sites[i].shadow, self->light_dir, 30000.0f, 1);
			((void (*)(void *))FUN_00043930)((char *)self->sites[i].actor + 0x40);
			((void (*)(void *))FUN_00039fa0)(self->sites[i].shadow);
			shadows++;
		}
		if (shadows != 0)
			((void (*)(int, int, int, int))FUN_0003a0e0)(0, 0, 0, (int)self->shadow_alpha);
	}
	((void (*)(int))FUN_00043b70)(1);
	cmb_blend_reset();
	for (i = 0; i < 6; i++) {
		((void (*)(void *))FUN_00043930)((char *)self->sites[i].actor + 0x40);
		if (self->alpha[i] < 0x100) {
			cmb_render_state(10, 1);
			memcpy(self->sites[i].colour, cmb_map_wire, sizeof cmb_map_wire);
			((void (*)(void *))FUN_0002f4a0)(self->sites[i].colour);
			cmb_render_state(10, 0);
		} else {
			memcpy(self->sites[i].colour, cmb_map_black, sizeof cmb_map_black);
			((void (*)(void *))FUN_0002f4a0)(self->sites[i].colour);
		}
	}
	cmb_render_state(8, 0);
}

/* 0x00020100  Core/CGameMode_Map.cpp: the path through the sites, the nodes and their
   numbers */
void CGameMode_Map_RenderPath(struct CGameMode_Map *self)
{
	float screen[6][4];
	float from[4];
	float to[4];
	float point[4];
	float label[8];
	unsigned short text[2];
	float span;
	float limit;
	float step;
	float s;
	int phase;
	int k;
	int i;

	((void (*)(void))FUN_000438b0)();
	((void (*)(float *, float *, int))FUN_00043c10)(&screen[0][0], &self->pos[0][0], 6);
	cmb_render_state(3, 0);
	cmb_render_state(7, 1);
	cmb_render_state(5, 0);
	phase = (int)self->pulse;
	for (k = 1; (float)k < self->path_done; k++) {
		span = self->path_done - (float)k;
		memcpy(from, self->pos[self->order[k - 1]], sizeof from);
		limit = (self->path_current - (float)k) + 1.0f;
		memcpy(to, self->pos[self->order[k]], sizeof to);
		if (!(span < 1.0f))
			span = 1.0f;
		step = 200.0f / ((float (*)(float *, float *))FUN_00051500)(from, to);
		point[3] = 1.0f;
		for (s = step; s < span; s += step) {
			((void (*)(float *, float *, float *, float))FUN_00050fa0)(point, from, to, s);
			if (s <= limit)
				cmb_draw_sprite(point, 80.0f, 80.0f, 0, self->node_texture, 0, 0, 0x7f, 0x7f,
					0x7f, self->path_alpha / 2, 0, 0);
			else if (phase != 0)
				cmb_draw_sprite(point, 60.0f, 60.0f, 0, self->node_texture, 0, 0, 0x7f, 0x5a,
					0, self->path_alpha / 2, 0, 0);
			else
				cmb_draw_sprite(point, 60.0f, 60.0f, 0, self->node_texture, 0, 0, 0, 0, 0,
					self->path_alpha / 2, 0, 0);
			phase--;
			if (phase < 0)
				phase = 5;
		}
	}
	for (k = 0; k < (int)self->path_done; k++) {
		if (self->current != k || phase < 3)
			cmb_draw_sprite(self->pos[self->order[k]], 120.0f, 120.0f, 0, self->node_texture, 0,
				0, 0x7f, 0x7f, 0x7f, self->path_alpha / 2, 0, 0);
	}
	((void (*)(int))FUN_00043b70)(0);
	for (k = 0; k < (int)self->path_done; k++) {
		label[0] = screen[self->order[k]][0] + 1.0f;
		CMB_AT(label, 0x10, int) = 2;
		label[1] = screen[self->order[k]][1] + 14.0f;
		CMB_AT(label, 0x18, int) = 0;
		text[0] = (unsigned short)(self->first_label + k + 0x31);
		text[1] = 0;
		CMB_AT(label, 0x14, int) = 4;
		CMB_AT(label, 0x1c, int) = 0;
		label[3] = 0.0f;
		label[2] = 0.0f;
		((void (*)(int, unsigned short *, int, float *, int, int, int, int, int, int))FUN_00043400)(
			self->font_light, text, 1, label, 0, 0, 0, 0xff, 0, 0);
		label[0] = label[0] - 1.0f;
		label[1] = label[1] + 1.0f;
		((void (*)(int, unsigned short *, int, float *, int, int, int, int, int, int))FUN_00043400)(
			self->font_light, text, 1, label, 0xff, 0xff, 0xff, 0xff, 0, 0);
	}
}

/* 0x000204f0  Core/CGameMode_Map.cpp: fade in straight to the current round */
void CGameMode_Map_StartRoundFade(struct CGameMode_Map *self)
{
	int site;

	self->zfar = CMB_MAP_ZFAR_NEAR;
	self->state = 2;
	self->fade = 0.0f;
	DAT_002baeb4 = self->order[self->current];
	site = self->order[self->current];
	self->alpha[site] = 0x100;
	self->alpha_min[site] = 0x100;
	self->path_alpha = 0xff;
	memcpy(self->cam_target, cmb_map_target_round, sizeof self->cam_target);
	memcpy(self->cam_pos, cmb_map_cam_round, sizeof self->cam_pos);
	self->hum = cmb_play(self->samples[1], 0x80, 0);
}

/* 0x000205d0  Core/CGameMode_Map.cpp */
void CGameMode_Map_RoundFade(struct CGameMode_Map *self)
{
	if (self->fade < 1.0f)
		self->fade = DAT_0028fda8 * 0.02f + self->fade;
	if (self->fade > 0.5f && self->zfar < CMB_MAP_ZFAR_FULL)
		self->zfar = 100.0f * DAT_0028fda8 + 100.0f * DAT_0028fda8 + self->zfar;
	if (self->fade >= 1.0f && self->zfar > CMB_MAP_ZFAR_FULL) {
		cmb_stop(self->hum);
		cmb_play(self->samples[4], 0xff, 0);
		CGameMode_Map_StartRound(self);
	}
}

/* 0x000206a0  Core/CGameMode_Map.cpp: send the titles away and play the logo out */
void CGameMode_Map_Leave(struct CGameMode_Map *self)
{
	char name[40];
	void *overlay;

	CMB_F32(self->title, 0x68) = 400.0f;
	CMB_F32(self->title, 0x2e0) = 8.0f;
	CMB_I32(self->title, 0x2f0) = 1;
	CMB_F32(self->subtitle, 0x68) = -300.0f;
	CMB_F32(self->subtitle, 0x2e0) = 20.0f;
	CMB_I32(self->subtitle, 0x2f0) = 1;
	if (self->label_left != NULL) {
		CMB_F32(self->label_left, 0x64) = CMB_F32(self->label_left, 0x64) - 400.0f;
		CMB_I32(self->label_left, 0x2f0) = 1;
	}
	if (self->label_right != NULL) {
		CMB_F32(self->label_right, 0x64) = CMB_F32(self->label_right, 0x64) + 400.0f;
		CMB_I32(self->label_right, 0x2f0) = 1;
	}
	overlay = self->vs;
	CMB_F32(overlay, 0x39c) = 0.0f;
	CMB_F32(overlay, 0x394) = 0.0f;
	CMB_F32(overlay, 0x38c) = 0.0f;
	CMB_F32(overlay, 0x384) = 1.0f;
	CMB_F32(overlay, 0x37c) = 1.0f;
	CMB_F32(overlay, 0x374) = 1.0f;
	CMB_F32(overlay, 0x3a0) = 1.0f;
	CMB_F32(overlay, 0x398) = 1.0f;
	CMB_F32(overlay, 0x390) = 1.0f;
	CMB_F32(overlay, 0x388) = -0.2f;
	CMB_F32(overlay, 0x380) = -0.2f;
	CMB_F32(overlay, 0x378) = -0.2f;
	CMB_I32(self->vs, 0x2f8) = 1;
	overlay = self->vs_text;
	CMB_F32(overlay, 0x388) = -0.1f;
	CMB_F32(overlay, 0x380) = -0.1f;
	CMB_F32(overlay, 0x378) = -0.1f;
	CMB_F32(overlay, 0x384) = 1.0f;
	CMB_F32(overlay, 0x37c) = 1.0f;
	CMB_F32(overlay, 0x374) = 1.0f;
	CMB_F32(overlay, 0x39c) = 0.0f;
	CMB_F32(overlay, 0x394) = 0.0f;
	CMB_F32(overlay, 0x38c) = 0.0f;
	CMB_F32(overlay, 0x3a0) = 1.0f;
	CMB_F32(overlay, 0x398) = 1.0f;
	CMB_F32(overlay, 0x390) = 1.0f;
	CMB_I32(self->vs_text, 0x2f8) = 1;
	sprintf(name, "logo_%d_out", self->logo);
	((void (*)(void *, const char *))FUN_0002ff40)(self, name);
	sprintf(name, "frenzy_%d_out.wav", self->logo);
	cmb_play(CMB_RESOURCE4(0, name, 2, 0xdefa), 0xff, 0);
	self->fade = 1.0f;
	self->zfar = CMB_MAP_ZFAR_FULL;
	self->state = 3;
}

/* 0x000208a0  Core/CGameMode_Map.cpp */
void CGameMode_Map_Load(struct CGameMode_Map *self)
{
	int site;
	int i;

	DAT_00284408 = 1;
	self->font_bold = CMB_RESOURCE4(0, "handgotb.ttf", 3, 0xdefa);
	DAT_002bb020 = 1;
	if (self->kind != 0)
		((void (*)(int, int, int, int, int, int))FUN_000724b0)(8, 0, 0xff, -20, 0x14, 0);
	self->package = ((int (*)(const char *, int))FUN_00071f90)("Packages\\Map", 0);
	if (self->package == 0) {
		CMB_DEBUG_PRINTF("Can't load map package\n");
		DbgBreakPoint();
		CMB_POP_MODE(1);
		return;
	}
	CGameMode_Map_CreateOverlays(self);
	DAT_002baebc = 0;
	self->state = 0;
	self->timer = 0.0f;
	self->angle = 0.0f;
	self->shadow_alpha = 60.0f;
	self->exit_timer = 0.0f;
	self->label_left = NULL;
	self->label_right = NULL;
	self->base.state = 2;
	self->logo = 1;
	self->game_name = g_score_table.rounds[g_score_table.round].kind + 1;
	for (i = 0; i < 6; i++) {
		self->order[i] = DAT_002844ac[i];
		self->round_of[DAT_002844ac[i]] = i;
	}
	self->current = g_score_table.round;
	self->first_label = 0;
	self->path_alpha = 0xff;
	self->pulse = 0.0f;
	self->rounds = DAT_002844a0[DAT_002bae74];
	self->path_done = (float)self->rounds;
	self->path_current = (float)self->current;
	for (i = 0; i < 6; i++) {
		CMB_I32(&self->sites[i], 0x44) = 0;
		self->alpha_min[i] = 8;
		self->alpha[i] = 8;
	}
	for (i = 0; i < self->current; i++) {
		site = DAT_002844ac[i];
		self->alpha[site] = 0x50;
		self->alpha_min[site] = 0x50;
	}
	site = self->order[self->current];
	self->alpha_min[site] = 0x100;
	self->alpha[site] = 0x100;
	self->zoom[1] = 0.0f;
	self->zoom[0] = 0.0f;
	CMB_AT(self->zoom, 0x8, unsigned int) = DAT_002bc40c;
	CMB_AT(self->zoom, 0xc, unsigned int) = DAT_002bc410;
	memcpy(self->zoom_from, self->zoom, sizeof self->zoom_from);
	memcpy(self->zoom_to, self->zoom, sizeof self->zoom_to);
}

/* 0x00020b50  Core/CGameMode_Map.cpp: after the package loaded, the models and sounds */
void CGameMode_Map_Loaded(struct CGameMode_Map *self)
{
	char name[40];
	int node;
	int anim;
	int i;

	self->base.state = 0;
	((void (*)(int))FUN_00071880)(2);
	((int (*)(int, int, int, int))FUN_0004b870)(self->package, -1, 0xdefa, 0);
	self->city_model = CMB_RESOURCE4(0, "city_mapdummies.obe", 1, 0xdefa);
	self->city = ((void *(*)(void *, int, int, const char *, int))FUN_00036b20)(NULL,
		self->city_model, 0, cmb_file_map, 0x212);
	cmb_actor_prep(self->city, 1);
	for (i = 0; i < 6; i++) {
		self->sites[i].actor = ((void *(*)(void *, int, int, const char *, int))FUN_00036b20)(NULL,
			CMB_RESOURCE4(0, cmb_map_site_model[i], 1, 0xdefa), 0, cmb_file_map, 0x220);
		self->sites[i].wire = ((void *(*)(void *, int, int, const char *, int))FUN_00036b20)(NULL,
			CMB_RESOURCE4(0, cmb_map_site_wire_model[i], 1, 0xdefa), 0, cmb_file_map, 0x228);
		((void (*)(void *, int, const int *))FUN_0002f5a0)(self->sites[i].colour,
			((int (*)(int, const char *, int, int, int, int))FUN_0004c6b0)(self->package,
				cmb_map_site_wire[i], 0, 0, 0, 0),
			cmb_map_wire);
		node = ((int (*)(void *, const char *))FUN_00035b90)(CMB_PTR(self->city, 0x134), cmb_map_site_node[i]);
		((void (*)(void *, int, float *))FUN_00035ce0)(self->city, node, self->pos[i]);
		CMB_F32(self->sites[i].actor, 0xc0) = self->pos[i][0];
		CMB_F32(self->sites[i].actor, 0xc4) = self->pos[i][1];
		CMB_F32(self->sites[i].actor, 0xc8) = self->pos[i][2];
		CMB_F32(self->sites[i].actor, 0xcc) = self->pos[i][3];
		cmb_actor_prep(self->sites[i].actor, 1);
		cmb_actor_prep(self->sites[i].wire, 1);
		self->pos[i][1] = -300.0f;
	}
	self->node_texture = CMB_RESOURCE4(0, "nodeicon.tga", 0, 0xdefa);
	self->line_texture = CMB_RESOURCE4(0, "line.bmp", 0, 0xdefa);
	((void *(*)(void *, unsigned int))FUN_000382f0)(self->light, 0);
	((void (*)(void *, int, int, int))FUN_00038470)(self->light, 0x46, 0x46, 0x5a);
	((void (*)(void *, const float *))FUN_00038550)(self->light, DAT_00283874);
	memcpy(self->cam_pos, cmb_map_cam_start, sizeof self->cam_pos);
	memcpy(self->cam_target, cmb_map_target_start, sizeof self->cam_target);
	for (i = 0; i < 8; i++) {
		self->samples[i] = CMB_RESOURCE4(0, cmb_map_samples[i], 2, 0xdefa);
		if (self->samples[i] == 0)
			CMB_DEBUG_PRINTF("MAP: Error loading %s\n", cmb_map_samples[i]);
	}
	switch (self->kind) {
	case 0: /* first time: everything lit */
		self->current = 0;
		self->path_done = 0.0f;
		self->rounds = 0;
		self->first_label = 0;
		for (i = 0; i < 6; i++)
			self->alpha[i] = 0x100;
		CGameMode_Map_CreatePrompt(self);
		break;
	case 1: /* draw the order */
		self->current = 0;
		self->rounds = 0;
		self->path_done = 0.0f;
		self->first_label = 0;
		for (i = 0; i < 6; i++)
			self->alpha[i] = 8;
		self->state = 1;
		self->fade = 0.0f;
		self->zfar = CMB_MAP_ZFAR_NEAR;
		self->hum = cmb_play(self->samples[1], 0x80, 0);
		if (self->kind == 1)
			cmb_commentary(0x2b, -1);
		break;
	case 2:
		CGameMode_Map_StartRoundFade(self);
		break;
	case 3: /* round intro with the logo */
		CGameMode_Map_StartRound(self);
		DAT_002baeac = 0;
		DAT_002baeb0 = self->order[self->current];
		self->logo = DAT_00283884;
		DAT_00283884++;
		self->state = 9;
		if (DAT_00283884 > 7)
			DAT_00283884 = 1;
		sprintf(name, "logo_%d_in", self->logo);
		((void (*)(void *, int, const char *))FUN_00030210)(self,
			CMB_RESOURCE4(0, "fusion_round.obe", 1, 0xdefa), name);
		CMB_F32(self->base.menu_model, 0xc4) = CMB_F32(self->base.menu_model, 0xc4) + 50.0f;
		sprintf(name, "logo_%d_flex", self->logo);
		anim = ((int (*)(void *, const char *))FUN_00035c70)(self->base.menu_model, name);
		((void (*)(void *, void *, int, int, int, int, int, int))FUN_00035770)(self->base.menu_model,
			CMB_PTR(self->base.menu_model, 0x134), anim, 0, -1, 0, 0, 0);
		sprintf(name, "frenzy_%d_in.wav", self->logo);
		cmb_play(CMB_RESOURCE4(0, name, 2, 0xdefa), 0xff, 0);
		if (DAT_00283d74 != 0)
			cmb_play(CMB_RESOURCE4(0, "metagameround2.wav", 2, 0xdefa), 0xff, 0);
		else
			cmb_commentary(0x1d, -1);
		break;
	case 4:
		CGameMode_Map_StartFadeIn(self);
		break;
	case 6: /* site viewer */
		self->state = 14;
		for (i = 0; i < 6; i++)
			self->alpha[i] = 0;
		self->alpha[self->current] = 0x100;
		break;
	default:
		for (i = 0; i < 6; i++)
			self->alpha[i] = 0;
		self->state = -1;
		break;
	}
	self->sample_playing = cmb_play(self->samples[0], 0x5a, 0);
}

/* 0x00021080  Core/CGameMode_Map.cpp */
void CGameMode_Map_PromptCountdown(struct CGameMode_Map *self)
{
	cmb_rotate_y(self->cam_pos, cmb_map_cam_start, self->angle);
	self->angle = 0.008f * DAT_0028fda8 + self->angle;
	self->timer = self->timer - DAT_0028fda8;
	if (self->timer <= 0.0f) {
		CMB_I32(self->prompt, 0x2bc) = -4;
		CMB_I32(self->prompt, 0x2f4) = 1;
		CGameMode_Map_Leave(self);
	}
}

/* 0x00021100  Core/CGameMode_Map.cpp */
void CGameMode_Map_Update(struct CGameMode_Map *self, int active)
{
	float value;
	float path;
	int i;

	if (DAT_002ecdec != 0)
		return;
	((void (*)(int))FUN_00030000)(active);
	if (self->exit_timer != 0.0f) {
		self->exit_timer = self->exit_timer - DAT_0028fda8;
		if (self->exit_timer <= 0.0f) {
			CMB_POP_MODE(1);
			return;
		}
	}
	value = 0.2f * DAT_0028fda8 + self->pulse;
	self->pulse = value;
	if (value > 6.0f)
		self->pulse = value - 6.0f;
	if ((float)self->rounds > self->path_done) {
		self->path_done = 0.05f * DAT_0028fda8 + self->path_done;
		if (!(self->path_done < (float)self->rounds)) {
			self->path_done = (float)self->rounds;
			cmb_play(self->samples[4], 0xff, 0);
		}
	}
	if ((float)self->current > self->path_current) {
		self->path_current = 0.05f * DAT_0028fda8 + self->path_current;
		if (!(self->path_current < (float)self->current)) {
			self->path_current = (float)self->current;
			cmb_play(self->samples[4], 0xff, 0);
		}
	}
	((void (*)(void))FUN_00074be0)();
	if (self->selector != NULL)
		CMB_VFN(self->selector, 0x04, void (*)(void *))(self->selector);
	if (self->base.state != 0) {
		CMB_VFN(self, 0x24, void (*)(void *))(self);
		return;
	}
	switch (self->state) {
	case 1:
		CGameMode_Map_FadeIn(self);
		return;
	case 2:
		CGameMode_Map_RoundFade(self);
		return;
	case 3:
		CGameMode_Map_FadeOut(self);
		return;
	case 4:
		CGameMode_Map_Turn(self);
		self->timer = self->timer - DAT_0028fda8;
		if (self->timer < 0.0f)
			CGameMode_Map_StartFlight(self);
		return;
	case 5:
		CGameMode_Map_PromptCountdown(self);
		return;
	case 6:
		self->path_current = 0.0f;
		CGameMode_Map_DrawOrder(self);
		return;
	case 7:
		self->timer = self->timer - DAT_0028fda8;
		return;
	case 8:
		CGameMode_Map_Turn(self);
		CGameMode_Map_Fly(self, self->blend);
		self->blend = 0.02f * DAT_0028fda8 + self->blend;
		if (!(self->blend < 1.0f)) {
			memcpy(self->zoom, self->zoom_to, sizeof self->zoom);
			self->blend = 1.0f;
			self->state = 9;
			CGameMode_Map_StartRound(self);
			return;
		}
		for (i = 0; i < 6; i++) {
			if (self->order[self->current] == i)
				continue;
			value = (float)self->alpha[i] - 8.0f * DAT_0028fda8;
			if ((float)self->alpha_min[i] <= value)
				self->alpha[i] = (int)value;
			else
				self->alpha[i] = (int)(float)self->alpha_min[i];
		}
		path = (float)self->path_alpha - 8.0f * DAT_0028fda8;
		if (!(190.0f <= path))
			path = 190.0f;
		self->path_alpha = (int)path;
		value = DAT_0028fda8 + DAT_0028fda8 + self->shadow_alpha;
		if (!(value < 60.0f))
			value = 60.0f;
		self->shadow_alpha = value;
		return;
	case 9:
		self->timer = self->timer - DAT_0028fda8;
		if (self->timer < 0.0f)
			CGameMode_Map_Leave(self);
		return;
	case 10:
		return;
	case 11:
		CGameMode_Map_Choose(self);
		return;
	case 12:
		self->timer = self->timer - DAT_0028fda8;
		if (self->timer < 0.0f)
			CGameMode_Map_StartFlight(self);
		return;
	case 13:
		CGameMode_Map_DimOthers(self);
		return;
	case 14:
		CGameMode_Map_View(self);
		return;
	}
}

/* 0x00021630  Core/CGameMode_Map.cpp */
void CGameMode_Map_Render(struct CGameMode_Map *self, int active)
{
	if (active != 0 && self->base.state == 0) {
		if (self->state < 1 || self->state > 3) {
			CGameMode_Map_Render3D(self);
			if (self->path_alpha != 0)
				CGameMode_Map_RenderPath(self);
			if (self->selector != NULL)
				((void (*)(void *))FUN_0002d830)(self->selector);
		} else {
			CGameMode_Map_RenderFade(self);
		}
		((void (*)(int))FUN_00073900)(1);
	}
	((void (*)(void *, int))FUN_00030310)(self, active);
}

/* 0x000216a0  Core/CGameMode_Map.cpp */
int CGameMode_Map_OnButton(struct CGameMode_Map *self, int button, int pad)
{
	int i;

	if (self->exit_timer != 0.0f || (button != 4 && button != 5))
		return 0;
	switch (self->state) {
	case 1:
		cmb_stop(self->hum);
		if (self->kind == 0) {
			CMB_POP_MODE(0);
			CMB_PUSH_MODE(DAT_002ba560, 1);
			return 1;
		}
		if (self->kind == 4)
			CGameMode_Map_StartZoomOut(self);
		else
			CGameMode_Map_StartDraw(self);
		break;
	case 2:
	case 4:
	case 8:
		CGameMode_Map_StartRound(self);
		break;
	case 5:
		CMB_POP_MODE(0);
		CMB_PUSH_MODE(DAT_002ba560, 1);
		return 1;
	case 6:
		CGameMode_Map_SkipDraw(self);
		break;
	case 9:
		CMB_PLAY_SAMPLE(9, 0x7f, 0, 0, 0);
		CGameMode_Map_Leave(self);
		break;
	case 14:
		for (i = 0; i < 6; i++)
			self->alpha[i] = 0;
		self->current++;
		if (self->current == 6)
			self->current = 0;
		self->alpha[self->current] = 0x100;
		break;
	default:
		return 0;
	}
	CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
	return 0;
}
