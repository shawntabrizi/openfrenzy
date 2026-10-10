/*
CGAMEMODE_PRESSSTART.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_PressStart.cpp

The title screen: the spinning Fuzion Frenzy logo, "press start", the legal line, the
Blitz and Microsoft logos. Start fades out to the city map (first time) or the main mode;
idling starts the attract mode. The constructor (0x00026180) is unplaced in the table.
*/

#include "xtl.h"

#include "core_modes_b.h"

static const char cmb_file_pressstart[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_PressStart.cpp";

#define CMB_TEXTBOX(memory, x, y, text, font) \
	((void *(*)(void *, int, int, const unsigned short *, int))FUN_00011c50)((memory), (x), (y), (text), (font))
#define CMB_OD_SIZE 0x3b0
#define cmb_overlay_create(d) (((void *(*)(void *))FUN_00074500)(d))
#define cmb_render_state(state, value) (((void (*)(int, int, int))FUN_00044b00)((state), (value), 0))

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

/* 0x00255e9c  CGameMode_PressStart vtable */
const void *const vtable_00255e9c[14] = {
	(const void *)FUN_000262d0, /* [0] scalar deleting destructor */
	(const void *)FUN_00026200, /* [1] Suspend */
	(const void *)FUN_00026350, /* [2] Start */
	(const void *)FUN_001d5950, /* [3] empty */
	(const void *)FUN_001d5950, /* [4] empty */
	(const void *)FUN_0002fe60, /* [5] core: button repeat dispatch */
	(const void *)FUN_00026790, /* [6] Update */
	(const void *)FUN_00026a30, /* [7] Render */
	(const void *)FUN_001ec260, /* [8] empty */
	(const void *)FUN_00012930, /* [9] core_modes_a */
	(const void *)FUN_000128e0, /* [10] core_modes_a */
	(const void *)FUN_001d5950, /* [11] empty */
	(const void *)FUN_001d5950, /* [12] empty */
	(const void *)FUN_00026290, /* [13] OnButton */
};

/* 0x00026180  (unplaced): constructor */
struct CGameMode_PressStart *CGameMode_PressStart_ctor(struct CGameMode_PressStart *self)
{
	CMB_GAMEMODE_CTOR(self, "PressStart");
	self->base.vtable = vtable_00255e9c;
	((void (*)(void *))FUN_00071050)(self->list_b4);
	((void (*)(void *))FUN_00071050)(self->list);
	self->base.refs++;
	self->saved_flag = -1;
	return self;
}

/* 0x00026200  Core/CGameMode_PressStart.cpp: Suspend */
void CGameMode_PressStart_Suspend(struct CGameMode_PressStart *self)
{
	((void (*)(void))FUN_0004c2c0)();
	((void (*)(void *))FUN_00034930)(self->logo);
	if (self->press_start != NULL)
		CMB_VFN(self->press_start, 0x00, void *(*)(void *, int))(self->press_start, 1);
	if (self->legal != NULL)
		CMB_VFN(self->legal, 0x00, void *(*)(void *, int))(self->legal, 1);
	if (self->trademark != NULL)
		CMB_VFN(self->trademark, 0x00, void *(*)(void *, int))(self->trademark, 1);
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(void))FUN_00074ba0)();
	self->press_start = NULL;
	self->legal = NULL;
	self->trademark = NULL;
	self->base.flags &= ~1u;
	self->saved_flag = DAT_00284408;
}

/* 0x00026290  Core/CGameMode_PressStart.cpp */
int CGameMode_PressStart_OnButton(struct CGameMode_PressStart *self, int button, int pad)
{
	self->idle_time = 0.0f;
	if (button == 5 && self->pressed == 0) {
		CMB_PLAY_SAMPLE(9, 0x7f, 0, 0, 0);
		self->pressed = 1;
	}
	return 0;
}

/* 0x000262d0  Core/CGameMode_PressStart.cpp */
void *CGameMode_PressStart_DeletingDtor(struct CGameMode_PressStart *self, unsigned char flags)
{
	CGameMode_PressStart_dtor(self);
	if ((flags & 1) != 0)
		CMB_DELETE(self);
	return self;
}

/* 0x000262f0  Core/CGameMode_PressStart.cpp: destructor */
void CGameMode_PressStart_dtor(struct CGameMode_PressStart *self)
{
	((void (*)(void *))FUN_001d5950)(self->list);
	((void (*)(void *))FUN_001d5950)(self->list_b4);
	CMB_GAMEMODE_DTOR(self);
}

/* 0x00026350  Core/CGameMode_PressStart.cpp: Start */
void CGameMode_PressStart_Start(struct CGameMode_PressStart *self)
{
	unsigned int desc[CMB_OD_SIZE / 4];
	void *memory;
	void *text;
	int font;
	int model;
	int anim;
	int i;

	((void (*)(int))FUN_0004c290)(self->base.package);
	((void (*)(int, int, int, int, int, int))FUN_000724b0)(2, 0, 0xff, -20, 0x14, 0);
	((void (*)(int))FUN_00071880)(2);
	font = CMB_RESOURCE4(0, "handgotb.ttf", 3, 0xdefa);
	((void (*)(void *))FUN_00072720)(self->list);
	memory = CMB_NEW(0x90, cmb_file_pressstart, 0x52);
	text = memory == NULL ? NULL : CMB_TEXTBOX(memory, -320, -40, CMB_TEXT(0x10b), font);
	self->press_start = text;
	CMB_U32(text, 0x8c) |= 8;
	cmb_text_select(self->press_start, 0.0f);
	memory = CMB_NEW(0x90, cmb_file_pressstart, 0x56);
	self->legal = memory == NULL ? NULL
		: CMB_TEXTBOX(memory, -190, -200, CMB_TEXT(0x1b6), CMB_RESOURCE4(0, "handgotl.ttf", 3, 0xdefa));
	cmb_text_select(self->legal, 0.0f);
	CMB_U32(self->legal, 0x8c) |= 0x2c;
	CMB_U32(self->legal, 0x8c) &= ~1u;
	CMB_F32(self->legal, 0x34) = 0.75f;
	CMB_F32(self->legal, 0x7c) = 350.0f;
	CMB_I32(self->legal, 0x80) = 1;
	memory = CMB_NEW(0x90, cmb_file_pressstart, 0x5e);
	self->trademark = memory == NULL ? NULL : CMB_TEXTBOX(memory, 0xe6, 100, L"TM", font);
	((void (*)(void *))FUN_000114e0)(self->trademark);
	text = self->trademark;
	CMB_I32(text, 0x30) = 0x28;
	CMB_I32(text, 0x24) = 0x7f;
	CMB_I32(text, 0x28) = 0x7f;
	CMB_I32(text, 0x2c) = 0x7f;
	CMB_F32(self->trademark, 0x34) = 0.4f;

	/* the Blitz and Microsoft logos */
	memset(desc, 0, sizeof desc);
	CMB_I32(desc, 0x288) = 0;
	CMB_I32(desc, 0x268) = 0;
	CMB_I32(desc, 0x264) = 0x7f;
	CMB_I32(desc, 0x28c) = 1;
	CMB_I32(desc, 0x298) = 1;
	CMB_F32(desc, 0x2a4) = -260.0f;
	CMB_F32(desc, 0x2a8) = -136.0f;
	CMB_F32(desc, 0x308) = 64.0f;
	CMB_F32(desc, 0x30c) = 64.0f;
	CMB_I32(desc, 0x04) = CMB_RESOURCE4(0, "blitz_logo.bmp", 0, 0xdefa);
	CMB_F32(desc, 0x250) = 127.0f;
	CMB_F32(desc, 0x254) = 127.0f;
	CMB_F32(desc, 0x258) = 127.0f;
	CMB_F32(desc, 0x25c) = 127.0f;
	cmb_overlay_create(desc);
	CMB_F32(desc, 0x2a4) = 170.0f;
	CMB_F32(desc, 0x2a8) = -156.0f;
	CMB_F32(desc, 0x308) = 128.0f;
	CMB_F32(desc, 0x30c) = 32.0f;
	CMB_I32(desc, 0x04) = CMB_RESOURCE4(0, "mslogo.gif", 0, 0xdefa);
	cmb_overlay_create(desc);
	memset(desc, 0, sizeof desc);

	DAT_00284408 = self->saved_flag;
	self->fade = 265.0f;
	self->pressed = 0;
	if (DAT_002baad8 != 0) {
		DAT_002baae8 = 1;
		DAT_002baad8 = 0;
	}
	self->base.flags |= 1;
	self->idle_time = 0.0f;
	for (i = 0; i < 4; i++) {
		((void (*)(int))FUN_00048d40)(DAT_002baf78[i][5]);
		((void (*)(int))FUN_00048d40)(DAT_002baf78[i][4]);
	}
	self->scroll = 0.0f;
	self->shine = 1.0f;
	DAT_002ba940 = 0;
	DAT_002ba944 = 0;
	DAT_002ba948 = 0;
	DAT_002ba94c = 0xff;
	model = CMB_RESOURCE4(0, "fusion_frontend.obe", 1, 0xdefa);
	if (model == 0) {
		CMB_DEBUG_PRINTF("cgamemode_pressstart::gainfocus(): couldn't find logo model\n");
		DbgBreakPoint();
	}
	((void *(*)(void *, int, int, const char *, int))FUN_00036b20)(self->logo, model, 1,
		cmb_file_pressstart, 0xa5);
	anim = ((int (*)(void *, const char *))FUN_00035c70)(self->logo, "default");
	((void (*)(void *, void *, int, int, int, int, int, int))FUN_00035770)(self->logo,
		self->logo_node, anim, 0, -1, 0, 0, 0);
	((void (*)(int))FUN_0004eed0)(1);
}

/* 0x00026790  Core/CGameMode_PressStart.cpp */
void CGameMode_PressStart_Update(struct CGameMode_PressStart *self, int active)
{
	void *memory;
	void *mode;
	float value;

	if (active == 0 || DAT_002ecdec != 0)
		return;
	CGameMode_Root_PollGUI();
	CMB_VFN(self->press_start, 0x08, void (*)(void *))(self->press_start);
	if (DAT_002baadc != 0) {
		CMB_PUSH_MODE(DAT_002ba560, 1);
		return;
	}
	if (self->pressed != 0) {
		/* fade out, then the map (first time) or the next mode */
		value = DAT_0028fda8 * 12.0f + self->fade;
		self->fade = value;
		if (value > 265.0f) {
			if (DAT_002ba538 != 0) {
				CMB_PUSH_MODE(DAT_002ba560, 1);
				return;
			}
			memory = CMB_NEW(0x5e8, cmb_file_pressstart, 0xf7);
			mode = memory == NULL ? NULL : CGameMode_Map_ctor(memory, 0);
			CMB_PUSH_MODE(mode, 1);
			DAT_002ba538 = 1;
		}
		return;
	}
	cmb_actor_animate(self->logo, self->logo_node, DAT_0028fda8 * 60.0f, 0);
	((void (*)(void *, int))FUN_00034bf0)(self->logo, 1);
	self->scroll = _DAT_002839a4 * DAT_0028fda8 + self->scroll;
	if (self->shine > 0.0f) {
		value = _DAT_002839a8 * DAT_0028fda8 + self->shine;
		self->shine = value;
		if (value <= 0.0f) {
			if (_DAT_002839b0 != 0)
				((int (*)(int, int, int, int, int, int))FUN_000719a0)(0x1d, -1, 0xff, 0, 0, 0);
			_DAT_002839b0 = 1;
			self->shine = 0.0f;
		}
	}
	value = self->fade - DAT_0028fda8 * 5.0f;
	self->fade = value;
	if (value < 0.0f)
		self->fade = 0.0f;
	value = DAT_0028fda8 + self->idle_time;
	self->idle_time = value;
	if (DAT_00283760 < value && (_DAT_002ba9c8 <= 0.0f || 600.0f < _DAT_002ba9c8)) {
		self->idle_time = 0.0f;
		CMB_PUSH_MODE(DAT_002b95cc, 1);
		_DAT_002839b0 = 0;
	}
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(void))FUN_00074be0)();
}

/* 0x00026a30  Core/CGameMode_PressStart.cpp */
void CGameMode_PressStart_Render(struct CGameMode_PressStart *self, int active)
{
	float eye[4];
	float at[4];
	float matrix[16];
	float shine;

	if (active == 0)
		return;
	cmb_kick_off_and_wait_for_idle();
	eye[0] = 0.0f;
	eye[1] = 0.0f;
	eye[2] = -320.0f;
	eye[3] = 1.0f;
	at[0] = 0.0f;
	at[1] = 0.0f;
	at[2] = 0.0f;
	at[3] = 1.0f;
	((void (*)(float *, float *, float))FUN_000435f0)(eye, at, 0.0f);
	cmb_set_clip(1.0f, 1000.0f);
	((void (*)(float, float))FUN_00044a00)(DAT_002839ac, 0.0f);
	((void (*)(void))FUN_00043790)();
	((void (*)(int))FUN_00043b70)(1);
	cmb_render_state(8, 0);
	cmb_render_state(5, 0);
	cmb_render_state(7, 3);
	cmb_render_state(6, 1);
	cmb_render_state(10, 1);
	cmb_render_state(3, 0);
	/* the identity matrix (copied from 0x002855f0) with a texture offset */
	shine = self->shine;
	memset(matrix, 0, sizeof matrix);
	matrix[0] = 1.0f;
	matrix[5] = 1.0f;
	matrix[10] = 1.0f;
	matrix[15] = 1.0f;
	matrix[8] = self->scroll;
	matrix[9] = shine * shine;
	cmb_render_state(2, 9);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)9, 2);
	D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)2, (const D3DMATRIX *)matrix);
	((int (*)(void *, int))FUN_00034d50)(self->logo, 7);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)9, 0);
	CMB_VFN(self->press_start, 0x0c, void (*)(void *))(self->press_start);
	CMB_VFN(self->legal, 0x0c, void (*)(void *))(self->legal);
	CMB_VFN(self->trademark, 0x0c, void (*)(void *))(self->trademark);
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(int))FUN_00073900)(1);
	if (self->fade != 0.0f)
		((void (*)(int, int))FUN_00071c20)((int)self->fade, 1);
}
