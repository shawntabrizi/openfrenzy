/*
Core/CGameMode_CharacterSelect.c  <-  C:\BlitzParty\Game\Src\Core\CGameMode_CharacterSelect.cpp

The character select screen: four player panels (each with the chosen
character's model in a viewport, its name and an info box), the six character
portraits with the players' markers, the menu after the choice and the
celebration camera with the cheering crowd. Helpers before the file's first
tagged function are in Core/unplaced.c (0x00013bd0-0x00014380).

Not functions (Ghidra split them off): 0x00015270 is the tail of
CGameMode_CharacterSelect_Update (0x00015160), 0x000175c0 a case of
CGameMode_CharacterSelect_HandleButton (0x00017580).
*/

#include "core_modes_a.h"
#include "core_modes_a_modes.h"
#include "core_modes_a_globals.h"

static const char s_CharacterSelect_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_CharacterSelect.cpp";

/* the fields of a COverlay (classlib_ui) and an actor (bactor) used here */
#define OVL_I32(o, off) CMA_FIELD(int, (o), (off))
#define OVL_F32(o, off) CMA_FIELD(float, (o), (off))
#define ACTOR_TRACK(a) CMA_FIELD(int, (a), 0x134)

/* the parameter block of COverlay creation (classlib_ui FUN_00074500 / FUN_00073ae0) */
#define CMA_OVERLAY_PARAMS_SIZE 0x3b0
#define OVP_I32(p, off) CMA_FIELD(int, (p), (off))
#define OVP_F32(p, off) CMA_FIELD(float, (p), (off))
/* FUN_00074500: a new overlay (cdecl); FUN_00073ae0: a child overlay of THIS (thiscall) */
#define CMA_Overlay_Create(params) ((void *(*)(void *))FUN_00074500)(params)
#define CMA_Overlay_CreateChild(parent, params) \
	((void *(*)(void *, void *))FUN_00073ae0)((parent), (params))
/* classlib_ui FUN_00070ed0: move POS towards TARGET (vec4) at SPEED with DAMPING */
#define CMA_MoveTowards(pos, target, speed, damping) \
	FUN_00070ed0((float *)(pos), (float *)(target), (speed), (damping))
/* bdisplay FUN_00046cb0 (3 arguments in the x86 code) */
#define CMA_PushRenderState(a, b, c) ((void (*)(int, int, int))FUN_00046cb0)((a), (b), (c))
/* core: the tweak menu (CTweakMenu, thiscall) */
#define CMA_TweakMenu_ctor(self, name) ((void *(*)(void *, const char *))FUN_0002e510)((self), (name))
#define CMA_TweakMenu_AddInt(menu, name, var, step, lo, hi, flags) \
	((void (*)(void *, const char *, int *, int, int, int, int))FUN_0002f250)( \
		(menu), (name), (var), (step), (lo), (hi), (flags))
#define CMA_TweakMenu_AddFloat(menu, name, var, step, lo, hi) \
	CMA_DUAL(FUN_0002f1a0, int (*)(int, undefined4, undefined4, undefined4, undefined4, undefined4), \
		int (*)(void *, const char *, float *, float, float, float), \
		((int)(menu), (undefined4)(name), (undefined4)(var), cma_fbits(step), cma_fbits(lo), \
			cma_fbits(hi)), \
		((menu), (name), (var), (step), (lo), (hi)))

/* the effect EFFECT goes into GROUP and is initialised (the inline of CBGEffect_AddToBackground
   on a group of this mode) */
static void CharSel_AddToGroup(struct CBGEffectGroup *group, struct CBGEffect *effect)
{
	CMA_List_Insert(&group->children, effect == 0 ? 0 : &effect->node, 1);
	CMA_VFN(effect, 0x04, void (*)(void *))(effect);
}

/* bring ANIM (a name of the animation table) up on ACTOR's current track from the start */
static void CharSel_StartIdle(int actor, int anim, int loop)
{
	int sequence = FUN_00035c70(actor, DAT_00292ca0[anim].name);

	if (sequence != 0) {
		FUN_00038190(actor, ACTOR_TRACK(actor), 1, 0);
		FUN_00035770(actor, 0, sequence, 0x400, 0xffffffff, loop, 0, 0);
		CMA_Actor_Update(actor, ACTOR_TRACK(actor), 0.0f, 0);
		FUN_00034bf0(actor, 1);
	}
}

/* put a panel's character model back at its place in the panel */
static void CharSel_ResetModel(void *overlay)
{
	int actor;

	OVL_F32(overlay, 0x300) = DAT_00281af0;
	OVL_F32(overlay, 0x304) = DAT_00281af4;
	OVL_F32(overlay, 0x308) = DAT_00281af8;
	OVL_F32(overlay, 0x30c) = DAT_00281afc;
	OVL_F32(overlay, 0x64) = DAT_00281af0;
	OVL_F32(overlay, 0x68) = DAT_00281af4;
	OVL_F32(overlay, 0x6c) = DAT_00281af8;
	OVL_F32(overlay, 0x70) = DAT_00281afc;
	OVL_F32(overlay, 0x78) = 0.0f;
	OVL_F32(overlay, 0x7c) = 0.0f;
	OVL_F32(overlay, 0x80) = 0.0f;
	OVL_F32(overlay, 0x84) = 1.0f;
	OVL_F32(overlay, 0x88) = 0.0f;
	OVL_F32(overlay, 0x8c) = 0.0f;
	OVL_F32(overlay, 0x90) = 0.0f;
	OVL_F32(overlay, 0x94) = 1.0f;
	OVL_I32(overlay, 0x2ec) |= 0x4000;
	actor = OVL_I32(overlay, 0x74);
	CMA_FIELD(float, actor, 0xe0) = 0.0f;
	CMA_FIELD(float, actor, 0xe4) = 0.0f;
	CMA_FIELD(float, actor, 0xe8) = 0.0f;
	CMA_FIELD(float, actor, 0xec) = 1.0f;
}

/* bactor FUN_00035b90: the bone of a track by name (the table says void) */
#define CMA_Actor_FindBone(track, name) ((int (*)(int, const char *))FUN_00035b90)((track), (name))
/* bkernel FUN_0004c6b0: a file of a package (package, name, flags, &size, ...) */
#define CMA_Package_GetFile(pkg, name, a, size, b, c) \
	((int (*)(int, const char *, int, int *, int, int))FUN_0004c6b0)((pkg), (name), (a), (size), (b), (c))

/* 0x00253dc4 CGameMode_CharacterSelect */
const void *const vtable_00253dc4[] = {
	(const void *)CGameMode_CharacterSelect_Delete,
	(const void *)CGameMode_CharacterSelect_Suspend,
	(const void *)CGameMode_CharacterSelect_Resume,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_CharacterSelect_Pushed,
	(const void *)CGameMode_CharacterSelect_Input,
	(const void *)CGameMode_CharacterSelect_Update,
	(const void *)CGameMode_CharacterSelect_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_ClearLoadFlags,
	(const void *)CGameMode_CharacterSelect_LoadingStarted,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_CharacterSelect_HandleButton,
};

/* 0x000143a0  Core/CGameMode_CharacterSelect.cpp: resume: the tweak menu, the light, the
   panels, the menu and the portraits' overlays; start loading Packages\CharacterSel */
void CGameMode_CharacterSelect_Resume(struct CGameMode_CharacterSelect *self)
{
	unsigned char params[CMA_OVERLAY_PARAMS_SIZE];
	float lightDir[4];
	float w;
	float h;
	int maxLength = 0;
	int i;
	void *memory;
	int font;

	memset(params, 0, sizeof(params));
	FUN_0004c290((undefined4)self->menu.base.name);
	FUN_00054690(DAT_002bae6c);
	memory = CMA_New(0xe8, s_CharacterSelect_cpp, 0x1e1, 0x1001);
	self->tweakMenu = memory == 0 ? 0 : CMA_TweakMenu_ctor(memory, "CharSelect");
	CMA_TweakMenu_AddInt(self->tweakMenu, "Light R", &DAT_00281b18, 1, 0, 0xff, 0);
	CMA_TweakMenu_AddInt(self->tweakMenu, "Light G", &DAT_00281b1c, 1, 0, 0xff, 0);
	CMA_TweakMenu_AddInt(self->tweakMenu, "Light B", &DAT_00281b20, 1, 0, 0xff, 0);
	CMA_TweakMenu_AddFloat(self->tweakMenu, "Light dir X", &DAT_00281b24, 0.1f, -1.0f, 1.0f);
	CMA_TweakMenu_AddFloat(self->tweakMenu, "Light dir Y", &DAT_00281b28, 0.1f, -1.0f, 1.0f);
	CMA_TweakMenu_AddFloat(self->tweakMenu, "Light dir Z", &DAT_00281b2c, 0.1f, -1.0f, 1.0f);
	CMA_TweakMenu_AddInt(self->tweakMenu, "Ambient R", &DAT_00281b0c, 1, 0, 0xff, 0);
	CMA_TweakMenu_AddInt(self->tweakMenu, "Ambient G", &DAT_00281b10, 1, 0, 0xff, 0);
	CMA_TweakMenu_AddInt(self->tweakMenu, "Ambient B", &DAT_00281b14, 1, 0, 0xff, 0);
	DAT_00284408 = 0;
	self->created = 1;

	/* the directional light */
	lightDir[0] = DAT_00281b24;
	lightDir[1] = DAT_00281b28;
	lightDir[2] = DAT_00281b2c;
	lightDir[3] = 1.0f;
	FUN_000382f0((undefined4 *)DAT_002b95e8, 0);
	CMA_FIELD(int, DAT_002b95e8, 0x14) = DAT_00281b18;
	CMA_FIELD(int, DAT_002b95e8, 0x18) = DAT_00281b1c;
	CMA_FIELD(int, DAT_002b95e8, 0x1c) = DAT_00281b20;
	FUN_00038550((undefined4 *)DAT_002b95e8, (undefined4 *)lightDir);
	FUN_00038370((int)DAT_002b95e8, 1);
	FUN_00071880(1);
	FUN_00026bf0();
	FUN_00072720((undefined4)&self->overlays);
	self->ready = 0;
	memory = CMA_New(0x20, s_CharacterSelect_cpp, 0x20a, 0x1001);
	self->group = memory == 0 ? 0 : CBGEffectGroup_ctor(memory);
	self->group->deleteFinished = 0;
	self->font1 = FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa);
	self->font2 = FUN_0004bb30(0, "handgotl.ttf", 3, 0xdefa);

	/* the longest character description (texts 0x111-0x116) */
	for (i = 0x1110; i < 0x1170; i += 0x10) {
		int length = CMA_WStrLen(*(wchar16 **)(CMA_FIELD(int, DAT_002ecc74, 0x20) + i)) + 1;
		if (length > maxLength)
			maxLength = length;
	}

	/* the four panels */
	for (i = 0; i < 4; i++) {
		struct CMA_CharSelPanel *panel = &self->panels[i];
		struct CBGTypeText **text;
		int k;

		self->actors[i] = 0;
		memory = CMA_New(0x20, s_CharacterSelect_cpp, 0x21e, 0x1001);
		panel->group = memory == 0 ? 0 : CBGEffectGroup_ctor(memory);
		panel->group->deleteFinished = 0;
		panel->x = (float)(int)((i % 2) * DAT_002bc40c) * 0.5f;
		panel->unk38 = 0;
		panel->shown = 0;
		panel->overlay = 0;
		panel->y = (float)((i / 2) * DAT_002bc410) * 0.5f;

		memory = CMA_New(0x90, s_CharacterSelect_cpp, 0x227, 0x1001);
		panel->charText = memory == 0 ? 0 : CBGTypeText_ctorEmpty(memory, 0, -60, 0x100, self->font2);
		CBGTypeText_Select(panel->charText, 0.0f);
		panel->charText->alphaRate = 8.0f;
		panel->charText->buf.text.scale = 2.0f;
		CharSel_AddToGroup(panel->group, &panel->charText->buf.text.base);
		panel->charText->buf.text.color[3] = 0;
		panel->charText->alpha = 0.0f;

		memory = CMA_New(0x90, s_CharacterSelect_cpp, 0x231, 0x1001);
		panel->joinText = memory == 0 ? 0 : CBGTypeText_ctor(memory, 0, 0, CMA_TEXT(0x105), self->font2);
		memory = CMA_New(0x90, s_CharacterSelect_cpp, 0x232, 0x1001);
		panel->readyText = memory == 0 ? 0 : CBGTypeText_ctor(memory, 0, 0, CMA_TEXT(0x106), self->font2);
		memory = CMA_New(0x90, s_CharacterSelect_cpp, 0x237, 0x1001);
		panel->nameText = memory == 0 ? 0
			: CBGTypeText_ctor(memory, -95, 120, CMA_TEXT(0x187 + i), self->font2);
		CBGTypeText_Select(panel->nameText, 0.0f);
		panel->nameText->alphaRate = 8.0f;
		panel->nameText->buf.text.scale = 3.0f;
		CharSel_AddToGroup(panel->group, &panel->nameText->buf.text.base);

		/* "join" and "ready" are centred */
		text = &panel->joinText;
		for (k = 0; k < 2; k++, text++) {
			FUN_00046320((*text)->buf.text.font, (int)(*text)->buf.text.text, (*text)->buf.text.length,
				&w, &h);
			(*text)->buf.text.x = -w;
			CBGTypeText_Select(*text, 0.0f);
			(*text)->alphaRate = 8.0f;
			(*text)->buf.text.scale = 2.0f;
			CharSel_AddToGroup(panel->group, &(*text)->buf.text.base);
		}
		panel->unk28 = 0;
		panel->readyText->buf.text.color[3] = 0;
		panel->readyText->alpha = 0.0f;

		/* the description box */
		font = FUN_0004bb30(0, "ocraext.ttf", 3, 0xdefa);
		memory = CMA_New(0xb0, s_CharacterSelect_cpp, 0x24e, 0x1001);
		panel->infoBox = memory == 0 ? 0
			: CBGTypeTextBox_ctorEmpty(memory, DAT_00281a30, DAT_00281a34, maxLength, font);
		CMA_VFN(panel->infoBox, 0x18, void (*)(void *, wchar16 *))(panel->infoBox, CMA_TEXT(0x111));
		panel->infoBox->type.alphaRate = 8.0f;
		panel->infoBox->type.buf.text.scale = 2.0f;
		panel->infoBox->type.style = 4;
		CharSel_AddToGroup(panel->group, &panel->infoBox->type.buf.text.base);
		panel->infoBox->type.alpha = 0.0f;
		panel->infoBox->type.buf.text.color[3] = 0;
		panel->infoBox->type.glow = 0.0f;
		panel->infoBox->type.glowTarget = 0.0f;
	}

	/* the prompt */
	memory = CMA_New(0xb0, s_CharacterSelect_cpp, 0x267, 0x1001);
	self->prompt = memory == 0 ? 0 : CBGTypeTextBox_ctor(memory, 0, -60, CMA_TEXT(0x10b), self->font2);
	self->prompt->layout.height = 0.0f;
	self->prompt->layout.width = 0.0f;
	self->prompt->layout.wrap = 0;
	self->prompt->layout.hAlign = 2;
	CBGTypeText_Select(&self->prompt->type, 0.0f);
	self->prompt->type.alphaRate = 8.0f;
	self->prompt->type.buf.text.color[3] = 0;
	self->prompt->type.alpha = 0.0f;
	CharSel_AddToGroup(self->group, &self->prompt->type.buf.text.base);

	/* the players' name tags */
	memset(params, 0, sizeof(params));
	for (i = 0; i < 4; i++) {
		void *overlay;

		OVP_I32(params, 0x44) = self->font2;
		OVP_F32(params, 0x2a8) = DAT_00281a40[i * 4 + 1] - 92.0f;
		OVP_F32(params, 0x2a4) = DAT_00281a40[i * 4] + 46.0f;
		OVP_I32(params, 0x28c) |= 1;
		OVP_I32(params, 0x288) = 2;
		OVP_I32(params, 0x2ac) = 0;
		OVP_F32(params, 0x250) = 104.0f;
		OVP_F32(params, 0x254) = 118.0f;
		OVP_F32(params, 0x258) = 111.0f;
		OVP_I32(params, 0x25c) = 0;
		OVP_I32(params, 0x268) = 0;
		OVP_I32(params, 0x264) = 0x7f;
		CMA_FIELD(short, params, 0x24c) = 1;
		overlay = CMA_Overlay_Create(params);
		self->playerOverlay[i] = overlay;
		if (DAT_002bae9c == 1)
			OVL_I32(overlay, 0x2b8) = 0;
	}
	OVP_I32(params, 0x28c) = 0;

	/* the menu after the choice (texts 0x14-0x16 and 0x110) */
	for (i = 0; i < 3; i++) {
		struct CBGTypeTextBox *box;

		memory = CMA_New(0xb0, s_CharacterSelect_cpp, 0x287, 0x1001);
		box = memory == 0 ? 0
			: CBGTypeTextBox_ctor(memory, 0, DAT_00281b04 - DAT_00281b08 * i, CMA_TEXT(0x14 + i),
				self->font2);
		self->menuText[i] = box;
		box->layout.height = 0.0f;
		box->layout.width = 0.0f;
		box->layout.wrap = 0;
		box->layout.hAlign = 2;
		box->type.alphaRate = 8.0f;
		box->type.buf.text.color[3] = 0;
		box->type.alpha = 0.0f;
		CharSel_AddToGroup(self->group, &box->type.buf.text.base);
	}
	memory = CMA_New(0xb0, s_CharacterSelect_cpp, 0x292, 0x1001);
	self->menuText[3] = memory == 0 ? 0
		: CBGTypeTextBox_ctor(memory, 0, DAT_00281b08 + 10 + DAT_00281b04, CMA_TEXT(0x110),
			self->font2);
	self->menuText[3]->layout.height = 0.0f;
	self->menuText[3]->layout.width = 0.0f;
	self->menuText[3]->layout.wrap = 0;
	self->menuText[3]->layout.hAlign = 2;
	CBGTypeText_Select(&self->menuText[3]->type, 0.0f);
	self->menuText[3]->type.alphaRate = 8.0f;
	self->menuText[3]->type.buf.text.color[3] = 0;
	self->menuText[3]->type.alpha = 0.0f;
	CharSel_AddToGroup(self->group, &self->menuText[3]->type.buf.text.base);

	self->ready = 1;
	if (DAT_002bae9c == 1) {
		self->stage = 4;
		self->menuState = 0;
		for (i = 0; i < DAT_002bae94; i++) {
			struct CMA_CharSelPanel *panel = &self->panels[i];

			panel->infoBox->type.alpha = 0.0f;
			panel->infoBox->type.buf.text.color[3] = 0;
			panel->charText->alpha = 0.0f;
			panel->charText->buf.text.color[3] = 0;
			panel->joinText->alpha = 0.0f;
			panel->joinText->buf.text.color[3] = 0;
			panel->readyText->alpha = 0.0f;
			panel->readyText->buf.text.color[3] = 0;
		}
	} else {
		self->stage = 0;
	}
	FUN_000724b0(2, 0, 0xff, 0xffffffec, 0x14, 0);
	if (DAT_002b9670 == 0) {
		DAT_002b9670 = CMA_LoadPackageAsync("Packages\\CharacterSel", 1);
		if (DAT_002b9670 == 0) {
			CMA_DebugPrintf("CGameMode_CharacterSelect::GainFocus *** Could not load package 'Packages\\CharacterSel' ***\n");
			return;
		}
	} else if (DAT_002b95ac != 0) {
		CMA_VFN(self, 0x2c, void (*)(void *))(self);
		return;
	}
	self->menu.base.loadState = 2;
}

/* 0x00015000  Core/CGameMode_CharacterSelect.cpp: celebration step of PLAYER: first the
   portrait flies to the player's position, then the name tag shows */
void CharSel_Celebrate(struct CGameMode_CharacterSelect *self, int player)
{
	void *portrait = self->charOverlay[self->selection[player]];

	if (self->celebrate[player] == 0) {
		OVL_I32(portrait, 0x2f0) = 1;
		OVL_F32(portrait, 0x2e0) = 12.0f;
		OVL_F32(portrait, 0x64) = DAT_00281a40[player * 4];
		OVL_F32(portrait, 0x68) = DAT_00281a40[player * 4 + 1];
		OVL_I32(portrait, 0x6c) = 0;
		OVL_I32(portrait, 0x2e8) |= 4;
		self->celebrate[player] = 1;
		return;
	}
	if (self->celebrate[player] == 1 && (OVL_I32(portrait, 0x2f0) & 1) == 0) {
		void *tag = self->playerOverlay[player];

		OVL_I32(tag, 0x2f4) = 1;
		OVL_I32(tag, 0x2bc) = 6;
		if (DAT_002baed0[player].joined != 0) {
			FUN_0004ae80((ushort *)((char *)tag + 0xa4), (ushort *)CMA_TEXT(0x187 + player));
			OVL_I32(tag, 0x2a4) = 0;
			self->celebrate[player] = 2;
			return;
		}
		OVL_I32(tag, 0x2a4) = (int)CMA_TEXT(0x102);
		self->celebrate[player] = 2;
	}
}

/* 0x00015160  Core/CGameMode_CharacterSelect.cpp: update (with 0x00015270, its tail): light,
   cheers of the chosen characters, pulsing portraits, idle animations, info boxes, effects */
void CGameMode_CharacterSelect_Update(struct CGameMode_CharacterSelect *self, int active)
{
	float lightDir[4];
	int c;
	int i;

	if (DAT_002ecdec != 0)
		return;
	if (active != 0) {
		lightDir[0] = DAT_00281b24;
		CMA_FIELD(int, DAT_002b95e8, 0x14) = DAT_00281b18;
		lightDir[1] = DAT_00281b28;
		lightDir[2] = DAT_00281b2c;
		lightDir[3] = 1.0f;
		CMA_FIELD(int, DAT_002b95e8, 0x18) = DAT_00281b1c;
		CMA_FIELD(int, DAT_002b95e8, 0x1c) = DAT_00281b20;
		FUN_00038550((undefined4 *)DAT_002b95e8, (undefined4 *)lightDir);
		FUN_00038440(DAT_00281b0c, DAT_00281b10, DAT_00281b14);
	}
	FUN_00030000(active);
	if (active == 0 || self->ready == 0)
		return;
	if (self->menu.base.loadState != 0) {
		CMA_VFN(self, 0x24, void (*)(void *))(self);
		return;
	}

	/* the portraits */
	for (c = 0; c < 6; c++) {
		void *highlight = self->charHighlight[c];
		int found = 0;
		int p;

		if (DAT_002b9658[c] > 0.0f) {
			double t = (double)DAT_002b9658[c] - (double)DAT_0028fda8;
			DAT_002b9658[c] = (float)t;
			if (t <= 0.0)
				FUN_00071770((char)c, 0x11, 0, 0xff, 0, -1, 0);
		}
		for (p = 0; p < 4; p++) {
			if (self->inGame[p] != 0 && self->selection[p] == c && self->owner[c] == -1) {
				found = 1;
				if (CMA_FIELD(short, highlight, 0x54) == 1) {
					CMA_FIELD(short, highlight, 0x54) = 0;
					OVL_I32(highlight, 0x2b8) = 0;
					self->pulsePhase[c] =
						(float)((double)-3.1415925f / ((double)DAT_00281b38 + (double)DAT_00281b38));
				}
				break;
			}
		}
		if (CMA_FIELD(short, highlight, 0x54) == 0) {
			double phase = (double)DAT_0028fda8 + (double)self->pulsePhase[c];

			self->pulsePhase[c] = (float)phase;
			OVL_F32(highlight, 0x2b8) = (float)((sin(phase * (double)DAT_00281b38) + 1.0) * 64.0);
		}
		if (!found)
			CMA_FIELD(short, highlight, 0x54) = 1;
	}

	/* the panels */
	for (i = 0; i < 4; i++) {
		struct CMA_CharSelPanel *panel = &self->panels[i];
		struct CBGTypeTextBox *box;

		if (panel->overlay != 0) {
			if (panel->animDelay > 0.0f) {
				panel->animDelay = panel->animDelay - DAT_0028fda8;
			} else if (DAT_002ba99c % 200 == i * 50) {
				unsigned int r;
				int track;
				int actor = OVL_I32(panel->overlay, 0x74);

				DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
				r = (DAT_00285630 >> 16) & 0x7fff;
				if ((int)r % 100 < 50 &&
					(track = CMA_Actor_FindBone(ACTOR_TRACK(actor), "neck")) != 0) {
					int extra = (int)Core_Rand() % 2;
					CharSel_PlayAnim(OVL_I32(panel->overlay, 0x74), extra + 0x25, 0, 0, 1.0f, 0.03f, track);
					CharSel_PlayAnim(OVL_I32(panel->overlay, 0x74), DAT_002819bc[i], 1, 1, 1.0f, 0.03f,
						track);
				}
			}
		}
		box = panel->infoBox;
		if (box != 0 && box->type.lines != 0) {
			float target = (float)(box->type.lines * DAT_00281a3c + DAT_00281a34);

			if (target != box->type.buf.text.y) {
				double step = fabs((double)target - (double)box->type.buf.text.y);

				if ((double)DAT_00281a38 < step)
					step = DAT_00281a38;
				box->type.buf.text.y = (float)(step + (double)box->type.buf.text.y);
			}
		}
		FUN_00072720((undefined4)&panel->overlays);
		FUN_00074be0();
		CMA_VFN(panel->group, 0x08, int (*)(void *))(panel->group);
	}
	FUN_00072720((undefined4)&self->overlays);
	FUN_00074be0();
	CMA_VFN(self->group, 0x08, int (*)(void *))(self->group);
}

/* 0x000154d0  Core/CGameMode_CharacterSelect.cpp: the celebration camera of stage camStage:
   one player ("<name>cam.lom") or all four ("groupcam.lom") */
void CharSel_InitCamera(struct CGameMode_CharacterSelect *self)
{
	char file[32];
	int size;
	int data;
	int lom;

	self->cheerTimer = 0.0f;
	self->cheerStarted = 0;
	if (self->camStage < 4) {
		int actor = self->actors[self->camStage];
		int character;

		CharSel_StartIdle(actor, CMA_ANIM_CAMERA_POSE, 1);
		character = DAT_002baed0[self->camStage].character;
		sprintf(file, "%scam.lom", CMA_CHAR_NAME(character));
		_strlwr(file);
		FUN_000719a0(6, DAT_002baed0[self->camStage].character, 0xff, 0, 0, 0);
	} else {
		int i;

		FUN_000719a0(2, 0xffffffff, 0xff, 0, 0, 0);
		for (i = 0; i < 4; i++) {
			int actor = self->actors[i];
			int sequence;

			CharSel_StartIdle(actor, CMA_ANIM_PSYCHED, 0);
			actor = self->actors[i];
			sequence = FUN_00035c70(actor, DAT_00292ca0[CMA_ANIM_RUN].name);
			if (sequence != 0) {
				CMA_Actor_PlayBlended(actor, 0, sequence, 0x400, -1, 0.05f, 1, 0, 0);
				CMA_Actor_Update(actor, ACTOR_TRACK(actor), 0.0f, 0);
				FUN_00034bf0(actor, 1);
			}
			actor = self->actors[i];
			CMA_FIELD(float, actor, 0xc0) = DAT_00281bd8[i * 4];
			CMA_FIELD(float, actor, 0xc4) = DAT_00281bd8[i * 4 + 1];
			CMA_FIELD(float, actor, 0xc8) = DAT_00281bd8[i * 4 + 2];
			CMA_FIELD(float, actor, 0xcc) = DAT_00281bd8[i * 4 + 3];
		}
		sprintf(file, "groupcam.lom");
	}
	data = CMA_Package_GetFile(DAT_002b9670, file, 0, &size, 0, 0);
	if (data == 0) {
		CMA_DebugPrintf("InitialiseCamera - Could not load camera LOM file [%s]\n", file);
		return;
	}
	lom = FUN_0007f2f0(data, 2, size);
	if (lom == 0) {
		CMA_DebugPrintf("InitialiseCamera - Error parsing camera LOM file [%s]\n", file);
		return;
	}
	FUN_0007e680(lom, 0);
	FUN_0007f3a0((int *)lom);
}

/* 0x00015740  Core/CGameMode_CharacterSelect.cpp: every player has chosen (or the menu came
   back): portraits to the row, models back into the panels */
void CharSel_AllJoined(struct CGameMode_CharacterSelect *self)
{
	int i;

	self->menuState = 0;
	FUN_0007cc90();
	FUN_0007d590();
	for (i = 0; i < 4; i++) {
		if (self->actors[i] != 0) {
			FUN_00034930(self->actors[i]);
			self->actors[i] = 0;
		}
	}
	if (DAT_002bae8c == DAT_002bae94) {
		int c;

		self->stage = 0;
		for (c = 0; c < 6; c++) {
			void *portrait = self->charOverlay[c];

			OVL_F32(portrait, 0x2b8) = 127.0f;
			if (self->owner[c] != -1) {
				void *marker = self->marker[c][self->owner[c]];

				OVL_I32(marker, 0x2f4) = 1;
				OVL_I32(marker, 0x2bc) = 8;
				OVL_I32(portrait, 0x2f0) = 1;
				OVL_F32(portrait, 0x2e0) = 12.0f;
				OVL_I32(portrait, 0x6c) = 0;
				OVL_I32(portrait, 0x2e8) |= 4;
				OVL_F32(portrait, 0x64) = (float)c * DAT_00281a2c + DAT_00281a24;
				OVL_F32(portrait, 0x68) = DAT_00281a28;
			}
		}
		for (i = 0; i < 4; i++) {
			struct CMA_CharSelPanel *panel = &self->panels[i];

			DAT_002819bc[i] = 0x20;
			panel->animDelay = 0.0f;
			CharSel_StartIdle(OVL_I32(panel->overlay, 0x74), CMA_ANIM_IDLE_CHARSELECT, 1);
			if (OVL_I32(panel->overlay, 0x10) == 0)
				CMA_List_Insert(&panel->overlays, (char *)panel->overlay + 0x10, 0);
			CharSel_ResetModel(panel->overlay);
		}
	} else {
		self->stage = 4;
		for (i = 0; i < 4; i++) {
			struct CMA_CharSelPanel *panel = &self->panels[i];
			void *portrait;

			DAT_002819bc[i] = 0x20;
			panel->animDelay = 0.0f;
			CharSel_StartIdle(OVL_I32(panel->overlay, 0x74), CMA_ANIM_IDLE_CHARSELECT, 1);
			OVL_I32(self->playerOverlay[i], 0x2f4) = 1;
			OVL_I32(self->playerOverlay[i], 0x2bc) = 0xc;
			portrait = self->charOverlay[self->selection[i]];
			OVL_I32(portrait, 0x2f4) = 1;
			OVL_I32(portrait, 0x2bc) = 0xc;
			CMA_List_Remove(&self->overlays, (char *)panel->overlay + 0x10);
			CMA_List_Insert(&panel->overlays, (char *)panel->overlay + 0x10, 0);
			CharSel_ResetModel(panel->overlay);
		}
		for (i = 0; i < 4; i++)
			self->menuText[i]->type.buf.text.color[3] = 0x7f;
	}
	OVL_F32(self->panels[0].overlay, 0x3e0) = 1.0f;
}

/* 0x00015bf0  Core/CGameMode_CharacterSelect.cpp: render the celebration: advance the camera
   (next stage when it ends), the models mirrored on the floor, the floor, the models */
void CharSel_RenderCelebration(struct CGameMode_CharacterSelect *self)
{
	float dt = DAT_002ecdec != 0 ? 0.0f : DAT_0028fda8;
	uint32_t near;
	uint32_t far;
	int i;

	if (FUN_0007e270(dt, 0) != 0) {
		FUN_0007cc90();
		FUN_0007d590();
		self->camStage = self->camStage + 1;
		if (self->camStage != 5) {
			CharSel_InitCamera(self);
			FUN_0007e270(1.0f, 0);
		}
	}
	CMA_SetClipPlanes(10.0f, 10000.0f);
	FUN_00043b70(1);
	FUN_00044b00(3, 1, 0);
	FUN_00044b00(7, 3, 0);
	FUN_00044b00(5, 1, 0);
	FUN_00044b00(6, 1, 0);
	for (i = 0; i < 4; i++) {
		if (self->camStage == 4 || self->camStage == i) {
			CMA_FIELD(float, self->actors[i], 0xd4) = -1.0f;
			FUN_00034bf0(self->actors[i], 1);
			FUN_00034d50(self->actors[i], 7);
		}
	}

	/* the floor: opaque at the back, half transparent at the front */
	memset(DAT_002b9680, 0, sizeof(DAT_002b9680));
	DAT_002b9680[0].y = 0.0f;
	near = DAT_002bd640[DAT_002844d4] | DAT_002bc490[DAT_002844d0] | DAT_002bce18[DAT_002844cc];
	far = near | DAT_002bc898[255];
	near = near | DAT_002bc898[128];
	DAT_002b9680[0].x = -600.0f;
	DAT_002b9680[0].z = -1500.0f;
	DAT_002b9680[0].color = far;
	DAT_002b9680[1].x = 600.0f;
	DAT_002b9680[1].y = 0.0f;
	DAT_002b9680[1].z = -1500.0f;
	DAT_002b9680[1].color = far;
	DAT_002b9680[2].x = 600.0f;
	DAT_002b9680[2].y = 0.0f;
	DAT_002b9680[2].z = 100.0f;
	DAT_002b9680[2].color = near;
	DAT_002b9680[3].x = -600.0f;
	DAT_002b9680[3].y = 0.0f;
	DAT_002b9680[3].z = 100.0f;
	DAT_002b9680[3].color = near;
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(3, 0, 0);
	FUN_000416c0(0, 0);
	FUN_000438b0();
	FUN_0003f2d0(5, (int)DAT_002b9680, 4);
	FUN_00044b00(3, 1, 0);
	for (i = 0; i < 4; i++) {
		if (self->camStage == 4 || self->camStage == i) {
			CMA_FIELD(float, self->actors[i], 0xd4) = 1.0f;
			FUN_00034bf0(self->actors[i], 1);
			FUN_00034d50(self->actors[i], 7);
		}
	}
}

/* 0x00015e50  Core/CGameMode_CharacterSelect.cpp: the cheers of the celebration; returns 1 when
   it is over (and the next mode is pushed) */
int CharSel_UpdateCelebration(struct CGameMode_CharacterSelect *self)
{
	char name[32];
	int stage = self->camStage;
	int i;

	if (stage == 5) {
		CharSel_Finish(self);
		return 1;
	}
	if (self->cheerStarted == 0) {
		double t = (double)DAT_0028fda8 + (double)self->cheerTimer;

		self->cheerTimer = (float)t;
		if (t > (double)DAT_00281c34) {
			const char *sample = 0;

			self->cheerStarted = 1;
			if (stage < 4) {
				if (DAT_00281c30 != 0) {
					int r;

					do {
						DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
						r = (int)((DAT_00285630 >> 16) & 0x7fff) % DAT_00281c30;
					} while (r == self->lastCheer);
					self->lastCheer = r;
					sprintf(name, "gencheer%02d.wav", r);
					self->cheerVolume[self->camStage] = 255.0f;
					sample = name;
				}
			} else {
				self->cheerStarted = 1;
				self->cheerVolume[stage] = 255.0f;
				DAT_002b9710 = 0;
				sample = "bigcheer00.wav";
			}
			if (sample != 0) {
				int volume = (int)self->cheerVolume[self->camStage];
				int resource = FUN_0004bb30(0, (char *)sample, 2, 0xdefa);

				self->cheerVoice[self->camStage] = CMA_PlaySample(resource, volume, 0, 0, 0);
			}
		}
	}
	if (self->camStage == 4 && DAT_002b9710 == 0) {
		double t = (double)DAT_0028fda8 + (double)self->cheerTimer;

		self->cheerTimer = (float)t;
		if (t > (double)DAT_00281c38) {
			FUN_000719a0(1, 0, 0xff, 0, 0, 0);
			DAT_002b9710 = 1;
		}
	}
	for (i = 0; i < 4; i++)
		CMA_Actor_Update(self->actors[i], ACTOR_TRACK(self->actors[i]), DAT_0028fda8, 0);
	for (i = 0; i < 5; i++) {
		if (self->cheerVoice[i] < 0)
			continue;
		if (FUN_000548d0(self->cheerVoice[i]) == 0) {
			self->cheerVoice[i] = -1;
		} else if (i < 4) {
			double volume;

			FUN_000548a0(self->cheerVoice[i], (undefined2)(int)self->cheerVolume[i]);
			volume = (double)self->cheerVolume[i] - (double)DAT_00281c3c * (double)DAT_0028fda8;
			self->cheerVolume[i] = (float)volume;
			if (volume < 0.0)
				self->cheerVolume[i] = 0.0f;
		}
	}
	return 0;
}

/* 0x000160a0  Core/CGameMode_CharacterSelect.cpp: show CHARACTER in PLAYER's panel: its name,
   the old model slides out (DIR 0 right, 1 left, 2 none) and the new one comes in */
void CharSel_ShowCharacter(struct CGameMode_CharacterSelect *self, int player, int character, int dir)
{
	unsigned char params[CMA_OVERLAY_PARAMS_SIZE];
	struct CMA_CharSelPanel *panel = &self->panels[player];
	struct CBGTypeText *text = panel->charText;
	float w;
	float h;
	void *overlay;

	memset(params, 0, sizeof(params));
	text->buf.text.color[3] = 0x7f;
	FUN_0004ae80((ushort *)text->buf.buffer, (ushort *)CMA_TEXT(0x149 + character));
	text->buf.text.length = CMA_WStrLen(text->buf.buffer);
	FUN_00046320(text->buf.text.font, (int)text->buf.buffer, text->buf.text.length, &w, &h);
	text->buf.text.x = -w;
	FUN_00072720((undefined4)&panel->overlays);

	memset(params, 0, sizeof(params));
	OVP_I32(params, 0x288) = 1;
	OVP_I32(params, 0x290) = 0x6000;
	OVP_F32(params, 0x384) = 1.0f;
	strcpy((char *)params + 0x2c4, DAT_002900c8 + character * 0x108);
	OVP_F32(params, 0x2a8) = DAT_00281af4;
	OVP_F32(params, 0x8) = DAT_00281af0;
	OVP_F32(params, 0x14) = DAT_00281afc;
	OVP_F32(params, 0x2ac) = DAT_00281af8;
	OVP_F32(params, 0x2b0) = DAT_00281afc;
	OVP_I32(params, 0x28c) = 4;
	OVP_F32(params, 0xc) = DAT_00281af4;
	OVP_F32(params, 0x10) = DAT_00281af8;
	if (dir == 2)
		OVP_F32(params, 0x2a4) = 0.0f;
	else
		OVP_F32(params, 0x2a4) = dir == 0 ? -400.0f : 400.0f;
	OVP_F32(params, 0x284) = DAT_00281b00;
	OVP_F32(params, 0x328) = 1.0f;
	OVP_F32(params, 0x320) = 1.0f;
	OVP_F32(params, 0x318) = 1.0f;
	OVP_F32(params, 0x25c) = 256.0f;
	CharSel_SlideOut(self, player, dir);
	overlay = CMA_Overlay_Create(params);
	panel->overlay = overlay;
	CMA_FIELD(float, OVL_I32(overlay, 0x74), 0xe0) = 0.0f;
	CMA_FIELD(float, OVL_I32(panel->overlay, 0x74), 0xe4) = 0.0f;
	CMA_FIELD(float, OVL_I32(panel->overlay, 0x74), 0xe8) = 0.0f;
	CMA_FIELD(float, OVL_I32(panel->overlay, 0x74), 0xec) = 1.0f;
	DAT_002819bc[player] = 0x20;
	panel->animDelay = 0.0f;
	CharSel_StartIdle(OVL_I32(panel->overlay, 0x74), CMA_ANIM_IDLE_CHARSELECT, 1);
}

/* 0x00016330  Core/CGameMode_CharacterSelect.cpp: suspend: free what Resume made */
void CGameMode_CharacterSelect_Suspend(struct CGameMode_CharacterSelect *self)
{
	int i;

	if (self->created == 0)
		return;
	if (self->tweakMenu != 0) {
		void *menu = self->tweakMenu;

		FUN_0002f3e0((int)menu);
		CMA_Delete(menu);
	}
	FUN_0004c2c0();
	FUN_00038370((int)DAT_002b95e8, 0);
	FUN_000384d0((int)DAT_002b95e8);
	self->created = 0;
	FUN_00072720((undefined4)&self->overlays);
	FUN_00074ba0();
	if (self->group != 0) {
		CMA_VFN(self->group, 0, void *(*)(void *, unsigned int))(self->group, 1);
		self->group = 0;
	}
	for (i = 0; i < DAT_002bae94; i++) {
		struct CMA_CharSelPanel *panel = &self->panels[i];

		if (self->actors[i] != 0) {
			FUN_00034930(self->actors[i]);
			self->actors[i] = 0;
		}
		if (panel->overlay != 0 && OVL_I32(panel->overlay, 0x10) == 0)
			CMA_List_Insert(&panel->overlays, (char *)panel->overlay + 0x10, 0);
		FUN_00072720((undefined4)&panel->overlays);
		FUN_00074ba0();
		if (panel->group != 0) {
			CMA_VFN(panel->group, 0, void *(*)(void *, unsigned int))(panel->group, 1);
			panel->group = 0;
		}
	}
	self->ready = 0;
}

/* play SEQUENCE (by animation index) blended over BLEND on the model of PLAYER's panel */
static void CharSel_PanelAnim(struct CGameMode_CharacterSelect *self, int player, int anim,
	float blend, int loop, int restart)
{
	int actor = OVL_I32(self->panels[player].overlay, 0x74);
	int sequence = FUN_00035c70(actor, DAT_00292ca0[anim].name);

	if (sequence != 0) {
		if (restart)
			FUN_00038190(actor, ACTOR_TRACK(actor), 1, 0);
		CMA_Actor_PlayBlended(actor, 0, sequence, 0x400, -1, blend, loop, 0, 0);
		CMA_Actor_Update(actor, ACTOR_TRACK(actor), 0.0f, 0);
		FUN_00034bf0(actor, 1);
	}
}

/* 0x00016440  Core/CGameMode_CharacterSelect.cpp: PLAYER takes back the choice (ANIM: play
   the "deselect" animation) */
void CharSel_Deselect(struct CGameMode_CharacterSelect *self, int player, int anim)
{
	int character;
	int k;

	if (self->confirmed == 1)
		self->prompt->type.buf.text.color[3] = 0;
	if (self->stage != 4)
		CMA_PlayGeneric(10, 0x7f, 0, 0, 0);
	character = DAT_002baed0[player].character;
	OVL_I32(self->charOverlay[character], 0x60) = FUN_0004bb30(0, (char *)PTR_s_dub_off_bmp_00281a0c[character], 0, 0xdefa);
	for (k = 0; k < 6; k++)
		OVL_I32(self->marker[k][player], 0x60) = FUN_0004bb30(0, (char *)PTR_s_p1_select_bmp_00281998[player], 0, 0xdefa);
	character = DAT_002baed0[player].character;
	DAT_002baed0[player].character = -1;
	DAT_002b9658[character] = 0.0f;
	self->owner[self->selection[player]] = -1;
	self->confirmed = self->confirmed - 1;
	if (anim != 0) {
		DAT_002819bc[player] = 0x1e;
		self->panels[player].animDelay = 0.0f;
		CharSel_PanelAnim(self, player, CMA_ANIM_DESELECT, 0.1f, 0, 1);
		DAT_002819bc[player] = 0x20;
		self->panels[player].animDelay = 0.0f;
		CharSel_PanelAnim(self, player, CMA_ANIM_IDLE_CHARSELECT, 0.03f, 1, 0);
		self->panels[player].animDelay = 120.0f;
	}
	self->panels[player].readyText->buf.text.color[3] = 0x7f;
}

/* 0x00016650  Core/CGameMode_CharacterSelect.cpp: PLAYER confirms the character; the others on
   it move to the next free one (ANIM: play "select") */
void CharSel_Confirm(struct CGameMode_CharacterSelect *self, int player, int anim)
{
	int character;
	int j;
	int k;

	if (self->confirmed == 0)
		self->prompt->type.buf.text.color[3] = 0x7f;
	DAT_002baed0[player].character = self->selection[player];
	self->owner[self->selection[player]] = player;
	character = DAT_002baed0[player].character;
	OVL_I32(self->charOverlay[character], 0x60) = FUN_0004bb30(0, (char *)PTR_s_dub_on_bmp_002819f4[character], 0, 0xdefa);
	for (k = 0; k < 6; k++)
		OVL_I32(self->marker[k][player], 0x60) = FUN_0004bb30(0, (char *)PTR_s_p1_confirm_bmp_002819a8[player], 0, 0xdefa);
	self->confirmed = self->confirmed + 1;
	DAT_002baed0[player].character = self->selection[player];
	for (j = 0; j < 4; j++) {
		if (j != player && self->selection[j] == DAT_002baed0[player].character && self->inGame[j] == 1) {
			OVL_I32(self->marker[self->selection[j]][j], 0x2f4) = 1;
			OVL_I32(self->marker[self->selection[j]][j], 0x2bc) = -8;
			self->selection[j] = CharSel_NextFreeCharacter(self->selection[j] + 1, player);
			if (self->selection[j] > 5)
				self->selection[j] = 0;
			OVL_I32(self->marker[self->selection[j]][j], 0x2f4) = 1;
			OVL_I32(self->marker[self->selection[j]][j], 0x2bc) = 8;
			CharSel_ShowCharacter(self, j, self->selection[j], 0);
		}
	}
	if (anim != 0) {
		DAT_002819bc[player] = 0x1d;
		self->panels[player].animDelay = 0.0f;
		CharSel_PanelAnim(self, player, CMA_ANIM_SELECT, 0.1f, 0, 1);
		DAT_002819bc[player] = 0;
		self->panels[player].animDelay = 0.0f;
		CharSel_PanelAnim(self, player, CMA_ANIM_IDLE_READY, 0.03f, 1, 0);
		self->panels[player].animDelay = 120.0f;
		if (self->stage != 1)
			DAT_002b9658[DAT_002baed0[player].character] = DAT_002819cc[DAT_002baed0[player].character];
	}
	self->panels[player].readyText->buf.text.color[3] = 0;
}

/* 0x000168f0  Core/CGameMode_CharacterSelect.cpp: input (vtable +0x14): the stage machine, then
   the menu buttons (core FUN_0002fe60) */
void CGameMode_CharacterSelect_Input(struct CGameMode_CharacterSelect *self, int active)
{
	int i;

	if (DAT_002ecdec != 0 || active == 0 || self->menu.base.loadState != 0 || DAT_002b95ac == 0)
		return;
	self->prompt->type.buf.text.color[3] = 0;
	switch (self->stage) {
	case 1:
		/* the button prompts of the players who have not joined blink in turn */
		for (i = 0; i < DAT_002bae94; i++) {
			void **prompt = &DAT_002baf78[i * 9];
			int k;

			if (DAT_002baed0[i].joined != 0)
				continue;
			for (k = 0; k < 9; k++)
				CMA_FIELD(int, prompt[k], 0x20) = 0;
			if (DAT_002ba99c % 8 == i * 2) {
				if (self->inGame[i] == 0) {
					CMA_FIELD(int, prompt[4], 0x20) = 1;
				} else if (self->arrowCount[i] > 0) {
					CMA_FIELD(int, prompt[1], 0x20) = 1;
					self->arrowCount[i] = self->arrowCount[i] - 1;
				} else if (self->arrowCount[i] < 0) {
					CMA_FIELD(int, prompt[3], 0x20) = 1;
					self->arrowCount[i] = self->arrowCount[i] + 1;
				} else {
					CMA_FIELD(int, prompt[4], 0x20) = 1;
				}
			}
		}
		/* fall through */
	case 0:
		if (self->confirmed == 0) {
			self->prompt->type.buf.text.color[3] = 0;
		} else {
			for (i = 0; i < DAT_002bae94; i++)
				if (self->inGame[i] != 0 && DAT_002baed0[i].character == -1)
					break;
			if (i == DAT_002bae94)
				self->prompt->type.buf.text.color[3] = 0x7f;
			else
				self->prompt->type.buf.text.color[3] = 0;
		}
		break;
	case 2:
		CharSel_HideMarkers(self);
		break;
	case 3:
		for (i = 0; i < DAT_002bae94; i++)
			CharSel_Celebrate(self, i);
		for (i = 0; i < DAT_002bae94; i++)
			if (self->celebrate[i] != 2)
				return;
		for (i = 0; i < DAT_002bae94; i++)
			self->celebrate[i] = 0;
		self->stage = 4;
		break;
	case 4:
		CharSel_UpdateMenu(self);
		break;
	case 5:
		if (CharSel_UpdateCelebration(self) != 0)
			return;
		break;
	}
	FUN_0002fe60((int *)self, active);
	if (self->quit != 0)
		CMA_ModeStack_Pop(1);
}

/* 0x00016b80  Core/CGameMode_CharacterSelect.cpp: render: the four panels (each in its
   viewport) or the celebration, then the 2D effects */
void CGameMode_CharacterSelect_Render(struct CGameMode_CharacterSelect *self, int active)
{
	int i;

	FUN_00030310((int)self, active);
	if (active == 0)
		return;
	CMA_PushRenderState(8, 0, 0);
	if (self->stage == 5) {
		CharSel_RenderCelebration(self);
		FUN_00046d00(8);
		return;
	}
	FUN_00044b00(6, 1, 0);
	for (i = 0; i < 4; i++) {
		struct CMA_CharSelPanel *panel = &self->panels[i];
		float target[4];

		FUN_00044840((int)panel->x, (int)panel->y, DAT_002bc40c / 2, DAT_002bc410 / 2);
		if (self->stage < 2 || self->menuState == 2) {
			if (panel->overlay != 0)
				OVL_F32(panel->overlay, 0x78) = OVL_F32(panel->overlay, 0x78) * 0.5f;
			target[0] = (float)(int)((i % 2) * DAT_002bc40c) * 0.5f;
			target[1] = (float)((i / 2) * DAT_002bc410) * 0.5f;
		} else {
			target[1] = 0.0f;
			target[0] = (float)((double)(DAT_002bc40c * i) * (double)0.16666667f +
				(double)i * (double)DAT_002819b8);
		}
		target[2] = 0.0f;
		target[3] = 1.0f;
		CMA_MoveTowards(&panel->x, target, DAT_00281b74, 0.5f);
		FUN_00044b00(3, DAT_00281b40, 0);
		FUN_00072720((undefined4)&panel->overlays);
		FUN_00073900(0);
		CharSel_RenderGroup(panel->group);
	}
	FUN_00044840(0, 0, DAT_002bc40c, DAT_002bc410);
	FUN_00044b00(3, DAT_00281b40, 0);
	FUN_00044b00(6, 0, 0);
	FUN_00072720((undefined4)&self->overlays);
	FUN_00073900(0);
	CharSel_RenderGroup(self->group);
	FUN_00046d00(8);
}

/* 0x00016d90  Core/CGameMode_CharacterSelect.cpp: PLAYER joins (FROMPAD: by pressing a button:
   counts the player and picks the first free character) */
void CharSel_Join(struct CGameMode_CharacterSelect *self, int player, int fromPad)
{
	if (fromPad != 0) {
		if (self->stage == 0) {
			if (DAT_002baed0[player].joined == 0) {
				DAT_002baed0[player].joined = 1;
				DAT_002bae90 = DAT_002bae90 - 1;
				DAT_002bae8c = DAT_002bae8c + 1;
				DAT_00284408 = 1;
			}
		} else if (DAT_002baed0[player].joined != 0) {
			DAT_002baed0[player].joined = 0;
			DAT_002bae90 = DAT_002bae90 + 1;
			DAT_002bae8c = DAT_002bae8c - 1;
			if (DAT_002bae8c == 0)
				DAT_00284408 = 0;
		}
	}
	self->inGame[player] = 1;
	if (fromPad != 0)
		self->selection[player] = CharSel_NextFreeCharacter(0, player);
	OVL_F32(self->marker[self->selection[player]][player], 0x2b8) = 100.0f;
	CharSel_ShowCharacter(self, player, self->selection[player], fromPad != 0 ? 0 : 2);
	self->panels[player].joinText->buf.text.color[3] = 0;
	self->panels[player].readyText->buf.text.color[3] = 0x7f;
}

/* 0x00016e90  Core/CGameMode_CharacterSelect.cpp: start the celebration: the four characters'
   models, the markers fade, the first camera */
void CharSel_StartCelebration(struct CGameMode_CharacterSelect *self)
{
	int i;
	int c;

	memset(DAT_002b9658, 0, sizeof(DAT_002b9658));
	FUN_00054730();
	self->lastCheer = -1;
	for (i = 0; i < 5; i++)
		self->cheerVoice[i] = -1;
	for (i = 0; i < 4; i++) {
		struct CMA_CharSelPanel *panel = &self->panels[i];
		int model;
		int actor;

		OVL_I32(self->playerOverlay[i], 0x2f4) = 1;
		OVL_I32(self->playerOverlay[i], 0x2bc) = -12;
		CMA_List_Remove(&panel->overlays, (char *)panel->overlay + 0x10);
		model = FUN_0004bb30(0, DAT_002900c8 + DAT_002baed0[i].character * 0x108, 1, 0xdefa);
		actor = FUN_00036b20(0, model, 6, (undefined4)s_CharacterSelect_cpp, 0x7d3);
		self->actors[i] = actor;
		FUN_00034e70(actor, (undefined4 *)ACTOR_TRACK(actor), 0, 1);
		FUN_00038190(self->actors[i], ACTOR_TRACK(self->actors[i]), 1, 0);
		FUN_00034bf0(self->actors[i], 1);
		actor = self->actors[i];
		CMA_FIELD(float, actor, 0xc0) = 0.0f;
		CMA_FIELD(float, actor, 0xc4) = 0.0f;
		CMA_FIELD(float, actor, 0xc8) = 0.0f;
		CMA_FIELD(float, actor, 0xcc) = 1.0f;
	}
	for (c = 0; c < 6; c++) {
		int p;

		OVL_I32(self->charOverlay[c], 0x2b8) = 0;
		for (p = 0; p < 4; p++) {
			OVL_I32(self->marker[c][p], 0x2f4) = 1;
			OVL_I32(self->marker[c][p], 0x2bc) = -12;
		}
	}
	for (i = 0; i < 4; i++)
		self->menuText[i]->type.buf.text.color[3] = 0;
	self->camStage = 0;
	self->cheerStarted = 0;
	CharSel_InitCamera(self);
}

/* 0x00017070  Core/CGameMode_CharacterSelect.cpp: background loading done: the portraits and
   markers; players who already joined (coming back from a game) get their panels */
void CGameMode_CharacterSelect_LoadingStarted(struct CGameMode_CharacterSelect *self)
{
	unsigned char params[CMA_OVERLAY_PARAMS_SIZE];
	int all;
	int c;
	int p;

	memset(params, 0, sizeof(params));
	for (c = 0; c < 6; c++)
		FUN_00071170(c);
	self->menu.base.loadState = 0;
	FUN_00072720((undefined4)&self->overlays);
	memset(params, 0, sizeof(params));
	for (c = 0; c < 6; c++) {
		void *highlight;

		OVP_I32(params, 0x288) = 0;
		OVP_I32(params, 0x4) = FUN_0004bb30(0, (char *)PTR_s_dub_off_bmp_00281a0c[c], 0, 0xdefa);
		OVP_F32(params, 0x2a4) = (float)c * DAT_00281a2c + DAT_00281a24;
		OVP_I32(params, 0x28c) |= 5;
		OVP_I32(params, 0x268) = 0x7f;
		OVP_I32(params, 0x264) = 0x7f;
		OVP_I32(params, 0x26c) = 0x7f;
		OVP_I32(params, 0x270) = 0x7f;
		OVP_I32(params, 0x274) = 0x7f;
		OVP_F32(params, 0x250) = 127.0f;
		OVP_F32(params, 0x254) = 127.0f;
		OVP_F32(params, 0x258) = 127.0f;
		OVP_F32(params, 0x25c) = 127.0f;
		OVP_F32(params, 0x2a8) = DAT_00281a28;
		OVP_I32(params, 0x2ac) = 0;
		OVP_I32(params, 0x8) = 0;
		OVP_I32(params, 0xc) = 0;
		OVP_I32(params, 0x10) = 0;
		OVP_I32(params, 0x278) = 0x50;
		OVP_I32(params, 0x27c) = 0x50;
		OVP_I32(params, 0x280) = 0x50;
		self->charOverlay[c] = CMA_Overlay_Create(params);
		OVP_I32(params, 0x4) = FUN_0004bb30(0, (char *)PTR_s_dub_on_bmp_002819f4[c], 0, 0xdefa);
		highlight = CMA_Overlay_CreateChild(self->charOverlay[c], params);
		self->charHighlight[c] = highlight;
		CMA_FIELD(short, highlight, 0x54) = 1;
		for (p = 0; p < 4; p++) {
			void *marker;

			OVP_I32(params, 0x288) = 0;
			OVP_I32(params, 0x4) = FUN_0004bb30(0, (char *)PTR_s_p1_select_bmp_00281998[p], 0, 0xdefa);
			OVP_F32(params, 0x250) = 127.0f;
			OVP_F32(params, 0x254) = 127.0f;
			OVP_F32(params, 0x258) = 127.0f;
			OVP_I32(params, 0x25c) = 0;
			OVP_F32(params, 0x2b4) = DAT_002819e4[p % 2];
			OVP_I32(params, 0x28c) |= 1;
			OVP_F32(params, 0x2b8) = DAT_002819ec[p / 2];
			OVP_I32(params, 0x2bc) = 0;
			OVP_I32(params, 0x268) = 0;
			OVP_I32(params, 0x264) = 100;
			marker = CMA_Overlay_CreateChild(self->charOverlay[c], params);
			self->marker[c][p] = marker;
			if (DAT_002bae9c == 1)
				OVL_I32(marker, 0x2b8) = 0;
		}
	}
	if (DAT_002bae9c != 0) {
		all = DAT_002bae8c == DAT_002bae94;
		DAT_00284408 = 1;
		self->confirmed = 0;
		for (p = 0; p < DAT_002bae94; p++) {
			struct CMA_CharSelPanel *panel = &self->panels[p];

			if (!all) {
				panel->y = 0.0f;
				panel->x = (float)((double)(DAT_002bc40c * p) * (double)0.16666667f +
					(double)p * (double)DAT_002819b8);
			}
			self->selection[p] = DAT_002baed0[p].character;
			self->inGame[p] = 1;
			OVL_F32(self->marker[self->selection[p]][p], 0x2b8) = 100.0f;
			CharSel_ShowCharacter(self, p, self->selection[p], 2);
			panel->joinText->buf.text.color[3] = 0;
			panel->readyText->buf.text.color[3] = 0x7f;
			CharSel_Confirm(self, p, 0);
			if (!all) {
				void *portrait = self->charOverlay[self->selection[p]];

				OVL_F32(portrait, 0x304) = DAT_00281a40[p * 4 + 1];
				OVL_F32(portrait, 0x300) = DAT_00281a40[p * 4];
				OVL_I32(portrait, 0x308) = 0;
				OVL_F32(self->playerOverlay[p], 0x2b8) = 127.0f;
			}
			panel->infoBox->type.alpha = 0.0f;
			panel->infoBox->type.buf.text.color[3] = 0;
			if (DAT_002baed0[p].joined != 0) {
				FUN_0004ae80((ushort *)((char *)self->playerOverlay[p] + 0xa4),
					(ushort *)CMA_TEXT(0x187 + p));
				OVL_I32(self->playerOverlay[p], 0x2a4) = 0;
			} else {
				OVL_I32(self->playerOverlay[p], 0x2a4) = (int)CMA_TEXT(0x102);
			}
			if (!all)
				self->celebrate[p] = 2;
		}
		if (!all) {
			for (c = 0; c < 6; c++) {
				OVL_F32(self->charOverlay[c], 0x2b8) = self->owner[c] == -1 ? 0.0f : 127.0f;
				OVL_I32(self->charOverlay[c], 0x2f4) = 0;
				for (p = 0; p < 4; p++) {
					OVL_I32(self->marker[c][p], 0x2b8) = 0;
					OVL_I32(self->marker[c][p], 0x2f4) = 0;
				}
			}
		}
	}
	if (DAT_002bae8c == DAT_002bae94)
		CharSel_AllJoined(self);
}

/* 0x00017580  Core/CGameMode_CharacterSelect.cpp: buttons (with 0x000175c0, a case of it):
   0 up, 1 right, 2 down, 3 left, 4 A, 5 start, 6 B */
int CGameMode_CharacterSelect_HandleButton(struct CGameMode_CharacterSelect *self, int button,
	int player)
{
	int old;
	int c;
	int i;

	switch (button) {
	case 0:
	case 2:
		if (self->menuState == 2 || self->stage != 4)
			break;
		CBGTypeText_Deselect(&self->menuText[DAT_002bae70]->type);
		DAT_002bae70 = (DAT_002bae70 + (button == 0 ? 2 : 1)) % 3;
		CBGTypeText_Select(&self->menuText[DAT_002bae70]->type, 0.0f);
		CMA_PlayGeneric(7, 0x7f, 0, 0, 0);
		return 0;
	case 1:
	case 3:
		if (self->stage != 0 && self->stage != 1)
			break;
		if (DAT_002baed0[player].character != -1 || self->inGame[player] != 1)
			break;
		OVL_I32(self->marker[self->selection[player]][player], 0x2f4) = 1;
		OVL_I32(self->marker[self->selection[player]][player], 0x2bc) = -8;
		old = self->selection[player];
		if (button == 1) {
			self->selection[player] = old + 1;
			self->selection[player] = CharSel_NextFreeCharacter(self->selection[player], player);
		} else {
			self->selection[player] = old - 1;
			self->selection[player] = CharSel_PrevFreeCharacter(self->selection[player], player);
		}
		OVL_I32(self->marker[self->selection[player]][player], 0x2f4) = 1;
		OVL_I32(self->marker[self->selection[player]][player], 0x2bc) = 8;
		if (self->selection[player] == old)
			break;
		CharSel_ShowCharacter(self, player, self->selection[player], button == 1 ? 0 : 1);
		CMA_PlayGeneric(7, 0x7f, 0, 0, 0);
		return 0;
	case 4:
		if (self->stage == 4 && self->menuState != 2) {
			CMA_PlayGeneric(9, 0x7f, 0, 0, 0);
			self->stage = 5;
			CharSel_StartCelebration(self);
			return 0;
		}
		if (self->stage == 5) {
			CharSel_Finish(self);
			return 1;
		}
		if (self->stage != 0 && !(self->stage == 1 && DAT_002baed0[player].joined == 0))
			break;
		if (self->inGame[player] == 0) {
			CharSel_Join(self, player, 1);
			if (self->stage == 1)
				break;
			CMA_PlayGeneric(9, 0x7f, 0, 0, 0);
			return 0;
		}
		if (DAT_002baed0[player].character != -1)
			break;
		CharSel_Confirm(self, player, 1);
		if (self->confirmed != DAT_002bae94 || self->stage != 1)
			break;
		self->stage = 2;
		return 0;
	case 5:
		if (self->stage == 5) {
			CharSel_Finish(self);
			return 1;
		}
		if (self->stage == 0 || (self->stage == 1 && DAT_002baed0[player].joined == 0)) {
			if (self->inGame[player] == 0) {
				CharSel_Join(self, player, 1);
				CMA_PlayGeneric(9, 0x7f, 0, 0, 0);
				return 0;
			}
			for (i = 0; i < DAT_002bae94; i++) {
				if (self->inGame[i] != 0 && DAT_002baed0[i].character == -1) {
					CMA_VFN(self, 0x34, int (*)(void *, int, int))(self, 4, player);
					goto menu;
				}
			}
			CMA_PlayGeneric(9, 0x7f, 0, 0, 0);
			if (DAT_002bae8c == DAT_002bae94) {
				DAT_002bae70 = 1;
				self->stage = 5;
				CharSel_StartCelebration(self);
				return 0;
			}
			if (self->confirmed == DAT_002bae94) {
				self->stage = 2;
				return 0;
			}
			self->stage = 1;
			for (i = 0; i < 4; i++)
				self->arrowCount[i] = (int)Core_Rand() % 5 - 2;
			return 0;
		}
menu:
		if (self->stage == 4 && self->menuState != 2) {
			CMA_VFN(self, 0x34, int (*)(void *, int, int))(self, 4, player);
			return 1;
		}
		break;
	case 6:
		if (self->stage == 0) {
			if (DAT_002baed0[player].joined == 0) {
				if (DAT_002bae8c != 0)
					return 0;
				self->quit = 1;
			} else {
				if (DAT_002baed0[player].character != -1) {
					CharSel_Deselect(self, player, 1);
					return 0;
				}
				if (self->inGame[player] == 0)
					return 0;
				CharSel_PlayerLeave(self, player);
				OVL_I32(self->marker[self->selection[player]][player], 0x2f4) = 1;
				OVL_I32(self->marker[self->selection[player]][player], 0x2bc) = -16;
			}
			CMA_PlayGeneric(10, 0x7f, 0, 0, 0);
		} else if (self->stage == 4) {
			if (self->menuState == 2 || DAT_002baed0[player].joined == 0)
				break;
			CMA_PlayGeneric(10, 0x7f, 0, 0, 0);
			self->menuState = 2;
			for (c = 0; c < 6; c++) {
				void *portrait = self->charOverlay[c];

				if (self->owner[c] == -1) {
					OVL_F32(portrait, 0x2b8) = 127.0f;
				} else {
					OVL_I32(portrait, 0x2f0) = 1;
					OVL_F32(portrait, 0x2e0) = 12.0f;
					COverlayMover_SetTarget((char *)portrait + 0x5c,
						(float)c * DAT_00281a2c + DAT_00281a24, DAT_00281a28, 0.0f);
				}
			}
			self->menuText[0]->type.buf.text.color[3] = 0;
			self->menuText[1]->type.buf.text.color[3] = 0;
			self->menuText[2]->type.buf.text.color[3] = 0;
			self->menuText[3]->type.buf.text.color[3] = 0;
			for (i = 0; i < DAT_002bae94; i++) {
				OVL_I32(self->playerOverlay[i], 0x2f4) = 1;
				OVL_I32(self->playerOverlay[i], 0x2bc) = -12;
				if (DAT_002baed0[i].joined == 0) {
					CharSel_Deselect(self, i, 0);
					CharSel_PlayerLeave(self, i);
				}
			}
			return 0;
		} else if (self->stage == 5) {
			CharSel_AllJoined(self);
			return 0;
		}
		break;
	}
	return 0;
}
