/*
Core/CGameMode_GameSizeMenu.c  <-  C:\BlitzParty\Game\Src\Core\CGameMode_GameSizeMenu.cpp

The game size menu (short, medium, long party): three wireframe models
(gamesize2/4/6.wir, core's CVectorShape) over a scrolling smoke texture,
three labels and a help line.
*/

#include "d3d8_extra.h"
#include "core_modes_a.h"
#include "core_modes_a_modes.h"
#include "core_modes_a_globals.h"

/* CGameMode_GameSizeMenu (vtable 0x00255078) */
struct CGameMode_GameSizeMenu {
	struct CGameModeMenu menu;          /* +0x000 */
	struct CBGEffectGroup *group;       /* +0x098 */
	struct CBGTypeText *labels[3];      /* +0x09c */
	unsigned char shapes[3][0x50];      /* +0x0a8 core CVectorShape and its placement */
	float scrollU;                      /* +0x198 */
	float scrollV;                      /* +0x19c */
	int texture;                        /* +0x1a0 projsmoke.bmp */
	int package;                        /* +0x1a4 the Root mode's package */
	int selection;                      /* +0x1a8 */
};
OF_ASSERT_OFFSET(struct CGameMode_GameSizeMenu, selection, 0x1a8);

/* the placement after core's CVectorShape in a shape slot */
#define SHAPE_MODEL(s) CMA_FIELD(int, (s), 0x2c)
#define SHAPE_POS(s) (&CMA_FIELD(float, (s), 0x30))   /* x, y, z, w */
#define SHAPE_SCALE(s) (&CMA_FIELD(float, (s), 0x40)) /* x, y, z, w */

extern int DAT_002bae74;            /* core: the game size chosen */
extern int DAT_002bae88;            /* core: rounds of the game */
extern int DAT_002844a0[];          /* core: rounds per game size */

/* 0x0028360c */
static const char *const PTR_s_gamesize2_0028360c[4] = { "gamesize2", "gamesize4", "gamesize6", 0 };
/* 0x0028362c, 0x0028363c: the shapes' colours; 0x0028364c: the light position */
static int DAT_0028362c[4] = { 0x50, 0x96, 0xff, 0xff };
static int DAT_0028363c[4] = { 0xe, 0x12, 0x1c, 0xff };
static const float DAT_0028364c[4] = { 0.0f, -100.0f, -200.0f, 1.0f };

/* 0x002b971c: the title text */
struct CBGTypeText *DAT_002b971c;

/* core: CVectorShape (thiscall) */
#define CMA_VectorShape_ctor(s) ((void *(*)(void *))FUN_0002f570)(s)
#define CMA_VectorShape_dtor(s) FUN_0002f580((int)(s))
/* bkernel: 4x4 translation and scale matrices (float parameters) */
#define CMA_MatrixTranslation(m, x, y, z) \
	CMA_DUAL(FUN_00050250, void (*)(undefined4 *, undefined4, undefined4, undefined4), \
		void (*)(float *, float, float, float), \
		((undefined4 *)(m), cma_fbits(x), cma_fbits(y), cma_fbits(z)), ((m), (x), (y), (z)))
#define CMA_MatrixScale(m, x, y, z) \
	CMA_DUAL(FUN_000502a0, void (*)(undefined4 *, undefined4, undefined4, undefined4), \
		void (*)(float *, float, float, float), \
		((undefined4 *)(m), cma_fbits(x), cma_fbits(y), cma_fbits(z)), ((m), (x), (y), (z)))

static const char s_GameSizeMenu_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_GameSizeMenu.cpp";

/* 0x00255078 CGameMode_GameSizeMenu */
const void *const vtable_00255078[] = {
	(const void *)CGameMode_GameSizeMenu_Delete,
	(const void *)CGameMode_GameSizeMenu_Suspend,
	(const void *)CGameMode_GameSizeMenu_Resume,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)FUN_0002fe60,
	(const void *)CGameMode_GameSizeMenu_Update,
	(const void *)CGameMode_GameSizeMenu_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_GameSizeMenu_HandleButton,
};

/* 0x00018f00  Core/CGameMode_GameSizeMenu.cpp: suspend: the texts fade in the background
   list; the shapes are freed */
void CGameMode_GameSizeMenu_Suspend(struct CGameMode_GameSizeMenu *self)
{
	int i;

	if (self->group != 0) {
		CBGEffect_AddToBackground(&self->group->base, 1);
		CMA_VFN(self->group, 0x14, void (*)(void *, float))(self->group, 30.0f);
		self->group = 0;
		for (i = 0; i < 3; i++) {
			CMA_VectorShape_dtor(self->shapes[i]);
			FUN_0003f650(SHAPE_MODEL(self->shapes[i]));
			self->labels[i] = 0;
		}
		FUN_0002ff40((int)self, 0);
	}
	FUN_0004c2c0();
}

/* 0x00018f80  Core/CGameMode_GameSizeMenu.cpp: 1 right, 3 left, 4/5 choose, 6 back */
int CGameMode_GameSizeMenu_HandleButton(struct CGameMode_GameSizeMenu *self, int button, int player)
{
	(void)player;
	switch (button) {
	case 1:
		CBGTypeText_Deselect(self->labels[self->selection]);
		self->selection = self->selection + 1;
		if (self->selection == 3)
			self->selection = 0;
		CBGTypeText_Select(self->labels[self->selection], 0.0f);
		SHAPE_SCALE(self->shapes[self->selection])[0] = 1.1f;
		SHAPE_SCALE(self->shapes[self->selection])[1] = 1.1f;
		break;
	case 3:
		CBGTypeText_Deselect(self->labels[self->selection]);
		self->selection = self->selection - 1;
		if (self->selection < 0)
			self->selection = 2;
		SHAPE_SCALE(self->shapes[self->selection])[0] = 1.1f;
		SHAPE_SCALE(self->shapes[self->selection])[1] = 1.1f;
		CBGTypeText_Select(self->labels[self->selection], 0.0f);
		break;
	case 4:
	case 5:
		CMA_PlayGeneric(9, 0x7f, 0, 0, 0);
		DAT_002bae74 = self->selection;
		DAT_002bae88 = DAT_002844a0[DAT_002bae74];
		CMA_ModeStack_Push((void *)DAT_002ba554, 1);
		return 1;
	case 6:
		CMA_PlayGeneric(10, 0x7f, 0, 0, 0);
		CMA_ModeStack_Pop(1);
		return 1;
	default:
		return 0;
	}
	CMA_PlayGeneric(7, 0x7f, 0, 0, 0);
	return 0;
}

/* 0x00019110  Core/CGameMode_GameSizeMenu.cpp: scroll the smoke, settle the shapes' scale */
void CGameMode_GameSizeMenu_Update(struct CGameMode_GameSizeMenu *self, int active)
{
	double v;
	int i;

	if (DAT_002ecdec != 0 || active == 0)
		return;
	v = (double)DAT_0028fda8 * 0.004f + (double)self->scrollU;
	self->scrollU = (float)v;
	if (!(v < 1.0))
		self->scrollU = (float)(v - 1.0);
	v = (double)DAT_0028fda8 * 0.01f + (double)self->scrollV;
	self->scrollV = (float)v;
	if (!(v < 1.0))
		self->scrollV = (float)(v - 1.0);
	for (i = 0; i < 3; i++) {
		float *scale = SHAPE_SCALE(self->shapes[i]);

		v = (double)scale[0] - (double)DAT_0028fda8 * 0.01f;
		scale[0] = 1.0 > v ? 1.0f : (float)v;
		v = (double)scale[1] - (double)DAT_0028fda8 * 0.01f;
		scale[1] = 1.0 > v ? 1.0f : (float)v;
	}
	CMA_MenuMode_Process(self, active);
	if (self->group != 0)
		CMA_VFN(self->group, 0x08, int (*)(void *))(self->group);
}

/* 0x00019230  Core/CGameMode_GameSizeMenu.cpp: out = a - b (3 floats) */
void Core_Vec3Sub(float *out, float *a, float *b)
{
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}

/* 0x00019260  Core/CGameMode_GameSizeMenu.cpp */
struct CGameMode_GameSizeMenu *CGameMode_GameSizeMenu_ctor(struct CGameMode_GameSizeMenu *self)
{
	int i;

	FUN_00030090((undefined4 *)self, (undefined4 *)"GameSizeMenu");
	self->menu.base.vtbl = vtable_00255078;
	for (i = 0; i < 3; i++) /* 'eh vector constructor iterator', element constructor 0x00018ee0 */
		CMA_VectorShape_ctor(self->shapes[i]);
	self->menu.base.refCount = self->menu.base.refCount + 1;
	self->selection = DAT_002bae74;
	self->menu.repeatRate[6] = 100000000;
	self->menu.repeatRate[5] = 100000000;
	self->menu.repeatRate[4] = 100000000;
	self->labels[0] = 0;
	self->labels[1] = 0;
	self->labels[2] = 0;
	return self;
}

/* 0x00019300  Core/CGameMode_GameSizeMenu.cpp */
struct CGameMode_GameSizeMenu *CGameMode_GameSizeMenu_Delete(struct CGameMode_GameSizeMenu *self,
	unsigned int flags)
{
	CGameMode_GameSizeMenu_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00019320  Core/CGameMode_GameSizeMenu.cpp */
void CGameMode_GameSizeMenu_dtor(struct CGameMode_GameSizeMenu *self)
{
	int i;

	for (i = 2; i >= 0; i--) /* 'eh vector destructor iterator', element destructor 0x00018ef0 */
		CMA_VectorShape_dtor(self->shapes[i]);
	FUN_00030190((undefined4 *)self);
}

/* 0x00019380  Core/CGameMode_GameSizeMenu.cpp: resume: shapes, labels, title, help line */
void CGameMode_GameSizeMenu_Resume(struct CGameMode_GameSizeMenu *self)
{
	char name[16];
	float toLight[3];
	struct CGameMode *root;
	struct CBGTypeText *text;
	void *memory;
	int font1;
	int font2;
	int x;
	int i;

	DAT_00284408 = 0;
	FUN_0004c290((undefined4)self->menu.base.name);
	font1 = FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa);
	font2 = FUN_0004bb30(0, "handgotl.ttf", 3, 0xdefa);
	self->texture = FUN_0004bb30(0, "projsmoke.bmp", 0, 0xdefa);
	memory = CMA_New(0x20, s_GameSizeMenu_cpp, 0x54, 0x1001);
	self->group = memory == 0 ? 0 : CBGEffectGroup_ctor(memory);
	root = CMA_ModeStack_Find("Root");
	self->package = CMA_FIELD(int, root, 0x44);
	for (i = 0, x = -480; x < 0; i++, x += 160) {
		unsigned char *shape = self->shapes[i];
		float *pos = SHAPE_POS(shape);
		int data;

		sprintf(name, "%s.wir", PTR_s_gamesize2_0028360c[i]);
		data = ((int (*)(int, const char *, int, int, int, int))FUN_0004c6b0)(self->package, name, 0, 0, 0, 0);
		FUN_0002f5a0((int *)shape, (int *)data, DAT_0028362c);
		pos[1] = 0.0f;
		pos[2] = 0.0f;
		pos[3] = 1.0f;
		pos[0] = (float)(i - 1) * 160.0f;
		toLight[0] = DAT_0028364c[0] - pos[0];
		toLight[1] = DAT_0028364c[1] - pos[1];
		toLight[2] = DAT_0028364c[2] - pos[2];
		SHAPE_MODEL(shape) = 0;
		FUN_0002f710((int)shape, (undefined4 *)toLight, &SHAPE_MODEL(shape), DAT_0028363c, 0);
		SHAPE_SCALE(shape)[0] = 1.0f;
		SHAPE_SCALE(shape)[1] = 1.0f;
		SHAPE_SCALE(shape)[2] = 1.0f;
		SHAPE_SCALE(shape)[3] = 1.0f;

		memory = CMA_New(0x90, s_GameSizeMenu_cpp, 0x6a, 0x1001);
		self->labels[i] = memory == 0 ? 0 : CBGTypeText_ctor(memory, x, -100, CMA_TEXT(0x19f), font1);
		self->labels[i]->style |= 0x48;
		if (i == self->selection)
			CBGTypeText_Select(self->labels[i], 0.0f);
		else
			CBGTypeText_Deselect(self->labels[i]);
		CMA_List_Insert(&self->group->children, self->labels[i] == 0 ? 0 : &self->labels[i]->buf.text.base.node, 1);
		CMA_VFN(self->labels[i], 0x04, void (*)(void *))(self->labels[i]);
	}

	memory = CMA_New(0x90, s_GameSizeMenu_cpp, 0x75, 0x1001);
	DAT_002b971c = memory == 0 ? 0 : CBGTypeText_ctor(memory, -200, 145, CMA_TEXT(0x11), font1);
	CBGTypeText_SetGreyStyle(DAT_002b971c);
	CMA_List_Insert(&self->group->children, DAT_002b971c == 0 ? 0 : &DAT_002b971c->buf.text.base.node, 1);
	CMA_VFN(DAT_002b971c, 0x04, void (*)(void *))(DAT_002b971c);

	memory = CMA_New(0x90, s_GameSizeMenu_cpp, 0x7b, 0x1001);
	text = memory == 0 ? 0 : CBGTypeText_ctorEmpty(memory, -310, -190, 0x100, font2);
	text->style = 0x30;
	text->boxWidth = 560.0f;
	text->buf.text.scale = 0.75f;
	CBGTypeText_Reset(text);
	((int (*)(unsigned short *, const unsigned short *, ...))FUN_0004ae80)(text->buf.buffer, u"%S %S",
		CMA_TEXT(0x18d), CMA_TEXT(0x18e));
	CMA_List_Insert(&self->group->children, &text->buf.text.base.node, 1);
	CMA_VFN(text, 0x04, void (*)(void *))(text);

	for (i = 0; i < 36; i++)
		CMA_FIELD(int, DAT_002baf78[i], 0x20) = 2;
	FUN_00030210((int)self, FUN_0004bb30(0, "over_gamesize.obe", 1, 0xdefa), 0);
	self->scrollV = 0.0f;
	self->scrollU = 0.0f;
}

/* 0x000197b0  Core/CGameMode_GameSizeMenu.cpp: render: the smoke-textured shapes; the chosen one
   is drawn as a wireframe too */
void CGameMode_GameSizeMenu_Render(struct CGameMode_GameSizeMenu *self, int active)
{
	float translation[16];
	float scale[16];
	float world[16];
	D3DMATRIX textureMatrix;
	unsigned char *shape;
	float *pos;
	int i;

	FUN_00030310((int)self, active);
	if (active == 0)
		return;
	D3D_KickOffAndWaitForIdle();
	FUN_00044b00(8, 0, 0);
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(7, 3, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 1, 0);
	memcpy(&textureMatrix, DAT_002855f0, sizeof(textureMatrix));
	((float *)&textureMatrix)[9] = self->scrollV;
	((float *)&textureMatrix)[8] = self->scrollU;
	FUN_000416c0(0, self->texture);
	FUN_00044b00(2, 3, 0);
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)9, 2);
	D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)2, &textureMatrix);

	shape = self->shapes[self->selection];
	pos = SHAPE_POS(shape);
	CMA_MatrixScale(scale, SHAPE_SCALE(shape)[0], SHAPE_SCALE(shape)[1], 1.0f);
	CMA_MatrixTranslation(translation, pos[0], pos[1], pos[2]);
	if ((((uintptr_t)translation | (uintptr_t)world | (uintptr_t)scale) & 0xf) != 0)
		FUN_0004ff30(world, translation, scale);
	else
		FUN_00050150(world, translation, scale);
	FUN_00043930((undefined4 *)world);
	FUN_0002f4a0((int *)shape);
	FUN_00044b00(3, 1, 0);
	for (i = 0; i < 3; i++) {
		int level = i == self->selection ? 0x7f : 0x40;

		shape = self->shapes[i];
		pos = SHAPE_POS(shape);
		FUN_00038440(level, level, level);
		CMA_MatrixTranslation(translation, pos[0], pos[1], pos[2]);
		FUN_00043930((undefined4 *)translation);
		FUN_0003f4a0(3, SHAPE_MODEL(shape), 0, FUN_0003ef90(SHAPE_MODEL(shape)));
	}
	D3DDevice_SetTextureState_Deferred(0, (D3DTEXTURESTAGESTATETYPE)9, 0);
	if (self->group != 0)
		CMA_VFN(self->group, 0x0c, void (*)(void *))(self->group);
}
