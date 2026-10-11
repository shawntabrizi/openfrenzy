/*
Core/unplaced.c  (module core_modes_a)

Functions of C:\BlitzParty\Game\Src\Core\ in 0x00011000-0x0001b8e0 whose source
file is not known (gaps between tagged functions). Most belong to the file
before or after them; see the comment of each function.
*/

#include <wchar.h>
#include "core_modes_a.h"
#include "core_modes_a_modes.h"
#include "core_modes_a_globals.h"

/* ---------- 0x00012760-0x000128c0: after BG_Effects.cpp */

/* 0x00012760  Core (unplaced): CBGText scalar deleting destructor */
struct CBGText *CBGText_Delete(struct CBGText *self, unsigned int flags)
{
	CBGText_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00012780  Core (unplaced): CBGTextBuf scalar deleting destructor */
struct CBGTextBuf *CBGTextBuf_Delete(struct CBGTextBuf *self, unsigned int flags)
{
	CBGTextBuf_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x000127a0  Core (unplaced): CBGTypeTextBox scalar deleting destructor */
struct CBGTypeTextBox *CBGTypeTextBox_Delete(struct CBGTypeTextBox *self, unsigned int flags)
{
	CBGTypeTextBox_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x000127c0  Core (unplaced): CBGTypeTextBox destructor (the code of CBGTypeText_dtor) */
void CBGTypeTextBox_dtor(struct CBGTypeTextBox *self)
{
	struct CBGTypeText *type = &self->type;

	type->buf.text.base.vtbl = vtable_00253810;
	if (type->typeSound != 0 && type->voice >= 0)
		FUN_000546e0(type->voice);
	CBGTextBuf_dtor(&type->buf);
}

/* 0x000127d0  Core (unplaced): operator delete */
void Core_OperatorDelete(void *block)
{
	CMA_Delete(block);
}

/* 0x000127e0  Core (unplaced): copy 4 floats */
void Core_Vec4Copy(float *dst, float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}

/* 0x00012800  Core (unplaced): CGameMode constructor (the class of FinishedLoading,
   Core/CGameMode_AskAgain.cpp) */
struct CGameMode *CGameMode_ctor(struct CGameMode *self, char *name)
{
	self->vtbl = vtable_002538a4;
	CMA_List_Init(&self->list);
	self->name = name;
	self->refCount = 0;
	self->flags = 0;
	FUN_0004c290((const char *)name);
	return self;
}

/* 0x00012860  Core (unplaced): CGameMode destructor */
void CGameMode_dtor(struct CGameMode *self)
{
	self->vtbl = vtable_002538a4;
	FUN_0004c2c0();
	FUN_001d5950(); /* ~CMA_List (nothing) */
}

/* 0x000128b0  Core (unplaced): the mode stack pushed this mode */
void CGameMode_AddRef(struct CGameMode *self)
{
	self->refCount = self->refCount + 1;
}

/* 0x000128c0  Core (unplaced): the mode stack popped this mode: delete it with the last
   reference (returns 0), else return the references left */
int CGameMode_Release(struct CGameMode *self)
{
	if (self->refCount == 1) {
		CMA_VFN(self, 0, void *(*)(void *, unsigned int))(self, 1);
		return 0;
	}
	self->refCount = self->refCount - 1;
	return self->refCount;
}

/* ---------- 0x00013bd0-0x00014380: before CGameMode_CharacterSelect.cpp (its helpers) */

OF_ASSERT_SIZE(struct CMA_CharSelPanel, 0x44);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, tweakMenu, 0x98);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, actors, 0xd4);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, marker, 0x130);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, overlays, 0x1a0);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, owner, 0x1dc);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, group, 0x230);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, panels, 0x294);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, stage, 0x3a8);
OF_ASSERT_OFFSET(struct CGameMode_CharacterSelect, ready, 0x3ac);

/* the fields of a COverlay (classlib_ui) that character select uses */
#define OVL_I32(o, off) CMA_FIELD(int, (o), (off))
#define OVL_F32(o, off) CMA_FIELD(float, (o), (off))

/* 0x00013bd0  Core (unplaced): CMA_CharSelPanel constructor (array constructor callback) */
struct CMA_CharSelPanel *CMA_CharSelPanel_ctor(struct CMA_CharSelPanel *self)
{
	CMA_List_Init(&self->overlays);
	return self;
}

/* 0x00013be0 (not in the function table): CMA_CharSelPanel destructor */
static void CMA_CharSelPanel_dtor(struct CMA_CharSelPanel *self)
{
	(void)self;
	FUN_001d5950(); /* ~CMA_List (nothing) */
}

/* 0x00013bf0  Core (unplaced): CGameMode_CharacterSelect destructor */
void CGameMode_CharacterSelect_dtor(struct CGameMode_CharacterSelect *self)
{
	int i;

	self->menu.base.vtbl = vtable_00253dc4;
	if ((void *)self == (void *)DAT_002ba554)
		DAT_002ba554 = 0;
	for (i = 3; i >= 0; i--) /* 'eh vector destructor iterator' */
		CMA_CharSelPanel_dtor(&self->panels[i]);
	FUN_001d5950(); /* ~CMA_List of self->overlays (nothing) */
	FUN_00030190((undefined4 *)self);
}

/* 0x00013c80  Core (unplaced): pushed: reset the selection state */
void CGameMode_CharacterSelect_Pushed(struct CGameMode_CharacterSelect *self)
{
	int i;

	self->menu.repeatRate[3] = 0xf;
	self->menu.repeatRate[1] = 0xf;
	self->menu.repeatRate[6] = 100000000;
	self->menu.repeatRate[5] = 100000000;
	self->menu.repeatRate[4] = 100000000;
	self->confirmed = 0;
	self->menu.base.loadState = 0;
	self->menuState = 0;
	self->quit = 0;
	DAT_002bae90 = DAT_002bae94;
	DAT_002bae8c = 0;
	for (i = 0; i < DAT_002bae94; i++) {
		DAT_002baed0[i].character = -1;
		DAT_002baed0[i].joined = 0;
		self->celebrate[i] = 0;
		self->arrowCount[i] = 0;
		self->selection[i] = 0;
		self->inGame[i] = 0;
	}
	for (i = 0; i < 6; i++)
		self->owner[i] = -1;
}

/* 0x00013d20  Core (unplaced): the character overlay of PLAYER slides out (left when
   FROMRIGHT is set) and the info box hides */
void CharSel_SlideOut(struct CGameMode_CharacterSelect *self, int player, int fromRight)
{
	struct CMA_CharSelPanel *panel = &self->panels[player];

	if (panel->overlay != 0) {
		if (panel->shown != 0 && panel->shown != panel->overlay) {
			FUN_000748c0((int)panel->shown);
			panel->shown = 0;
		}
		panel->shown = panel->overlay;
		OVL_F32(panel->shown, 0x64) = fromRight == 0 ? 600.0f : -600.0f;
		OVL_F32(panel->shown, 0x2e0) = DAT_00281b00;
	}
	CMA_VFN(panel->infoBox, 0x14, void (*)(void *, float))(panel->infoBox, 0.0f);
}

/* 0x00013da0  Core (unplaced): the first character from C on (wrapping) that no other player
   than PLAYER has chosen */
int CharSel_NextFreeCharacter(int c, int player)
{
	if (c == 6)
		c = 0;
	for (;;) {
		int p;

		for (p = 0; p < 4; p++)
			if (DAT_002baed0[p].character == c)
				break;
		if (p == 4 || p == player) {
			if (c + 1 != 0)
				return c;
		}
		c = c + 1;
		if (c >= 6)
			c = 0;
	}
}

/* 0x00013df0  Core (unplaced): as CharSel_NextFreeCharacter, going down */
int CharSel_PrevFreeCharacter(int c, int player)
{
	if (c == -1)
		c = 5;
	for (;;) {
		int p;

		for (p = 0; p < 4; p++)
			if (DAT_002baed0[p].character == c)
				break;
		if (p == 4 || p == player) {
			if (c + 1 != 0)
				return c;
		}
		c = c - 1;
		if (c < 0)
			c = 5;
	}
}

/* 0x00013e50  Core (unplaced): stage 2: fade the markers out once, then go to stage 3 */
void CharSel_HideMarkers(struct CGameMode_CharacterSelect *self)
{
	int i;
	int c;

	if (DAT_002b967c != 0) {
		self->stage = 3;
		DAT_002b967c = 0;
		return;
	}
	for (i = 0; i < DAT_002bae94; i++)
		CMA_VFN(self->panels[i].infoBox, 0x14, void (*)(void *, float))(self->panels[i].infoBox, 0.0f);
	for (c = 0; c < 6; c++) {
		int p;

		if (self->owner[c] == -1)
			OVL_I32(self->charOverlay[c], 0x2b8) = 0;
		for (p = 0; p < 4; p++) {
			OVL_I32(self->marker[c][p], 0x2f4) = 1;
			OVL_I32(self->marker[c][p], 0x2bc) = -8;
		}
	}
	DAT_002b967c = 1;
}

/* 0x00013f30  Core (unplaced): render an effect group as 2D text over the scene */
void CharSel_RenderGroup(struct CBGEffectGroup *group)
{
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(7, 1, 0);
	FUN_00043b70(0);
	FUN_000416c0(0, 0);
	FUN_000438b0();
	CMA_VFN(group, 0x0c, void (*)(void *))(group);
	FUN_00044b00(8, 0, 0);
	FUN_00044b00(7, 1, 0);
	FUN_00044b00(5, 1, 0);
}

/* 0x00013fb0  Core (unplaced): stage 4 (the menu after the choice): highlight the current entry
   once; when every portrait has faded, show the markers of the joined players and go back to
   stage 0 */
void CharSel_UpdateMenu(struct CGameMode_CharacterSelect *self)
{
	int i;

	if (self->menuState == 0) {
		for (i = 0; i < 4; i++) {
			if (i == DAT_002bae70 || i == 3)
				CBGTypeText_Select(&self->menuText[i]->type, 0.0f);
			else
				CBGTypeText_Deselect(&self->menuText[i]->type);
			self->menuText[i]->type.alpha = 0.0f;
		}
		self->menuState = 1;
	}
	if (self->menuState == 2) {
		for (i = 0; i < 6; i++)
			if (OVL_I32(self->charOverlay[i], 0x2f0) & 1)
				break;
		if (i == 6) {
			for (i = 0; i < DAT_002bae94; i++) {
				self->celebrate[i] = 0;
				if (DAT_002baed0[i].joined != 0)
					OVL_F32(self->marker[self->selection[i]][i], 0x2b8) = 100.0f;
			}
			self->stage = 0;
			self->menuState = 0;
		}
	}
}

/* 0x00014090  Core (unplaced): PLAYER leaves */
void CharSel_PlayerLeave(struct CGameMode_CharacterSelect *self, int player)
{
	struct CMA_CharSelPanel *panel = &self->panels[player];

	panel->charText->buf.text.color[3] = 0;
	if (DAT_002baed0[player].joined != 0) {
		DAT_002baed0[player].joined = 0;
		DAT_002bae90 = DAT_002bae90 + 1;
		DAT_002bae8c = DAT_002bae8c - 1;
	}
	self->inGame[player] = 0;
	OVL_I32(self->marker[self->selection[player]][player], 0x2b8) = 0;
	if (panel->overlay != 0) {
		if (panel->shown != 0 && panel->shown != panel->overlay) {
			FUN_000748c0((int)panel->shown);
			panel->shown = 0;
		}
		panel->shown = panel->overlay;
		OVL_F32(panel->overlay, 0x64) = 600.0f;
		OVL_F32(panel->shown, 0x2e0) = DAT_00281b00;
	}
	CMA_VFN(panel->infoBox, 0x14, void (*)(void *, float))(panel->infoBox, 0.0f);
	panel->joinText->buf.text.color[3] = 0x7f;
	panel->readyText->buf.text.color[3] = 0;
}

/* 0x00014170  Core (unplaced): play animation ANIM (index into the table 0x00292ca0) on ACTOR:
   from the start unless KEEP, looping when LOOP, SPEED in 1/1024, blended over BLEND when not 0,
   on TRACK (0: the actor's current one) */
void CharSel_PlayAnim(int actor, int anim, int loop, int keep, float speed, float blend, int track)
{
	int sequence = (int)FUN_00035c70((struct BActorInstance *)actor, DAT_00292ca0[anim].name);

	if (sequence == 0)
		return;
	if (keep == 0)
		FUN_00038190((void *)actor, (struct BActorNode *)(track != 0 ? track : CMA_FIELD(int, actor, 0x134)), 1, 0);
	if (blend == 0.0f)
		FUN_00035770((struct BActorInstance *)actor, (struct BActorNode *)track, (struct BAnim *)sequence, (int)((double)speed * 1024.0), 0xffffffff, loop != 0, 0, 0);
	else
		CMA_Actor_PlayBlended(actor, track, sequence, (int)((double)speed * 1024.0), -1, blend,
			loop != 0, 0, 0);
	CMA_Actor_Update(actor, track != 0 ? track : CMA_FIELD(int, actor, 0x134), 0.0f, 0);
	FUN_00034bf0((struct BActorInstance *)actor, 1);
}

/* 0x00014250  Core (unplaced): the choice is made: stop the cheers and push the next mode */
void CharSel_Finish(struct CGameMode_CharacterSelect *self)
{
	int i;

	for (i = 0; i < 5; i++)
		if (self->cheerVoice[i] >= 0)
			FUN_000548a0(self->cheerVoice[i], 0);
	FUN_0007cc90();
	FUN_0007d590();
	self->menuState = 0;
	if (DAT_002bae98 == 1)
		CMA_ModeStack_Push((void *)DAT_002ba55c, 1);
	else
		CMA_ModeStack_Push((void *)DAT_002ba558, 1);
}

/* 0x000142c0  Core (unplaced): set the target position of a COverlay's mover (the object at
   overlay + 0x5c) and start it */
void COverlayMover_SetTarget(void *self, float x, float y, float z)
{
	CMA_FIELD(float, self, 0x8) = x;
	CMA_FIELD(float, self, 0x10) = z;
	CMA_FIELD(float, self, 0xc) = y;
	CMA_FIELD(unsigned int, self, 0x28c) |= 4;
}

/* 0x000142f0  Core (unplaced): CGameMode_CharacterSelect constructor */
struct CGameMode_CharacterSelect *CGameMode_CharacterSelect_ctor(struct CGameMode_CharacterSelect *self)
{
	int i;

	FUN_00030090((undefined4 *)self, (undefined4 *)"CharacterSelect");
	self->menu.base.vtbl = vtable_00253dc4;
	CMA_List_Init(&self->overlays);
	for (i = 0; i < 4; i++) /* 'eh vector constructor iterator' */
		CMA_CharSelPanel_ctor(&self->panels[i]);
	self->menu.base.refCount = self->menu.base.refCount + 1;
	self->confirmed = 0;
	return self;
}

/* 0x00014380  Core (unplaced): CGameMode_CharacterSelect scalar deleting destructor */
struct CGameMode_CharacterSelect *CGameMode_CharacterSelect_Delete(struct CGameMode_CharacterSelect *self,
	unsigned int flags)
{
	CGameMode_CharacterSelect_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* ---------- 0x000199c0-0x0001a7e0: CGameMode_GameStats (vtable 0x0025513c), file unknown */

OF_ASSERT_OFFSET(struct CGameMode_GameStats, standalone, 0x139);

/* the score table of module core_modes_b (CScoreTable at 0x002b9968): round, then 25 ints a
   round; +0x38 + 4 * player: the round's points */
extern int DAT_002b9968[0xc4];
#define CMA_ROUND_POINTS(r, p) DAT_002b9968[14 + (r) * 25 + (p)]

extern int DAT_002bae74;            /* core: the game size chosen */
extern int DAT_002bae88;            /* core: rounds of the game (0: a single game) */
extern int DAT_002baeb4;            /* core */
extern int DAT_002844a0[];          /* core: rounds per game size */

/* the picture of character C (the character table's second name) */
#define CMA_CHAR_PICTURE(c) (*(const char **)((char *)&DAT_00290754 - 0x54 + (c) * 0x60))

/* the text of an overlay in a parameter block (classlib_ui COverlay): font +0x44, text +0x48 */
#define OVP_TEXT(p) ((wchar_t *)((char *)(p) + 0x48))

/* 0x0025513c CGameMode_GameStats */
const void *const vtable_0025513c[] = {
	(const void *)CGameMode_GameStats_Delete,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_GameStats_Popped,
	(const void *)CGameMode_GameStats_Pushed,
	(const void *)FUN_0002fe60,
	(const void *)CGameMode_GameStats_Update,
	(const void *)CGameMode_GameStats_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_GameStats_HandleButton,
};

/* 0x000199c0  Core (unplaced): CGameMode_GameStats constructor (over another mode) */
struct CGameMode_GameStats *CGameMode_GameStats_ctor(struct CGameMode_GameStats *self)
{
	FUN_00030090((undefined4 *)self, (undefined4 *)"GameStats");
	self->menu.base.vtbl = vtable_0025513c;
	CMA_List_Init(&self->overlays);
	self->menu.base.flags |= 2;
	self->closing = 0;
	self->standalone = 0;
	return self;
}

/* 0x00019a30  Core (unplaced): CGameMode_GameStats constructor; STANDALONE: draws the menu */
struct CGameMode_GameStats *CGameMode_GameStats_ctorShow(struct CGameMode_GameStats *self,
	unsigned char standalone)
{
	FUN_00030090((undefined4 *)self, (undefined4 *)"GameStats");
	self->menu.base.vtbl = vtable_0025513c;
	CMA_List_Init(&self->overlays);
	self->menu.base.flags |= 2;
	self->standalone = standalone;
	self->closing = 0;
	return self;
}

/* 0x00019aa0  Core (unplaced): popped */
void CGameMode_GameStats_Popped(struct CGameMode_GameStats *self)
{
	FUN_00072720((undefined4)&self->overlays);
	FUN_00074ba0();
}

/* 0x00019ac0  Core (unplaced): render */
void CGameMode_GameStats_Render(struct CGameMode_GameStats *self, int active)
{
	if (self->standalone != 0)
		FUN_00030310((struct CMenuMode *)self, active);
	FUN_00072720((undefined4)&self->overlays);
	FUN_00073900(1);
}

/* 0x00019b00  Core (unplaced): A or start closes once the counting is over */
int CGameMode_GameStats_HandleButton(struct CGameMode_GameStats *self, int button, int player)
{
	(void)player;
	if (self->timer <= 0.0f) {
		if (button < 4 || button > 5)
			return 0;
		if (self->standalone != 0) {
			CMA_ModeStack_Pop(1);
			return 1;
		}
		self->closing = 1;
	}
	return 1;
}

/* 0x00019b60  Core (unplaced) */
struct CGameMode_GameStats *CGameMode_GameStats_Delete(struct CGameMode_GameStats *self,
	unsigned int flags)
{
	CGameMode_GameStats_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00019b80  Core (unplaced) */
void CGameMode_GameStats_dtor(struct CGameMode_GameStats *self)
{
	FUN_001d5950(); /* ~CMA_List (nothing) */
	FUN_00030190((undefined4 *)self);
}

/* set the common fields of a centred text overlay */
static void GameStats_TextParams(unsigned char *params, int font, short hAlign, short vAlign,
	float x, float y)
{
	CMA_FIELD(int, params, 0x288) = 2;
	CMA_FIELD(short, params, 0x24c) = hAlign;
	CMA_FIELD(int, params, 0x28c) = 5;
	CMA_FIELD(short, params, 0x24e) = vAlign;
	CMA_FIELD(float, params, 0x250) = 128.0f;
	CMA_FIELD(float, params, 0x254) = 128.0f;
	CMA_FIELD(float, params, 0x258) = 128.0f;
	CMA_FIELD(float, params, 0x25c) = 128.0f;
	CMA_FIELD(float, params, 0x2a4) = x;
	CMA_FIELD(float, params, 0x2a8) = y;
	CMA_FIELD(int, params, 0x2ac) = 0;
	CMA_FIELD(int, params, 0x44) = font;
}

/* the glow fields of the last-round and total texts */
static void GameStats_GlowParams(unsigned char *params)
{
	CMA_FIELD(int, params, 0x28c) = 3;
	CMA_FIELD(float, params, 0x328) = 1.0f;
	CMA_FIELD(float, params, 0x320) = 1.0f;
	CMA_FIELD(float, params, 0x318) = 1.0f;
	CMA_FIELD(float, params, 0x340) = 1.0f;
	CMA_FIELD(float, params, 0x338) = 1.0f;
	CMA_FIELD(float, params, 0x330) = 1.0f;
	CMA_FIELD(float, params, 0x344) = 2.0f;
	CMA_FIELD(float, params, 0x33c) = 2.0f;
	CMA_FIELD(float, params, 0x334) = 2.0f;
	CMA_FIELD(float, params, 0x32c) = -0.05f;
	CMA_FIELD(float, params, 0x324) = -0.05f;
	CMA_FIELD(float, params, 0x31c) = -0.05f;
}

/* 0x00019bd0  Core (unplaced): pushed: the headings, a column per round, a row per player */
void CGameMode_GameStats_Pushed(struct CGameMode_GameStats *self)
{
	unsigned char params[0x3b0];
	int font1;
	int font2;
	int rounds;
	int slot;
	int k;
	int i;

	memset(params, 0, sizeof(params));
	FUN_00072720((undefined4)&self->overlays);
	font1 = (int)FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa);
	font2 = (int)FUN_0004bb30(0, "handgotl.ttf", 3, 0xdefa);

	memset(params, 0, sizeof(params));
	GameStats_TextParams(params, font1, 2, 3, 270.0f, 170.0f);
	swprintf(OVP_TEXT(params), (wchar_t *)(DAT_002bae88 == 0 ? CMA_TEXT(0x181) : CMA_TEXT(0xcb + DAT_002baeb4)));
	CMA_Overlay_Create(params);

	memset(params, 0, sizeof(params));
	GameStats_TextParams(params, font2, 0, 1, -270.0f, 140.0f);
	swprintf(OVP_TEXT(params), (wchar_t *)CMA_TEXT(0x18b));
	CMA_Overlay_Create(params);

	memset(params, 0, sizeof(params));
	GameStats_TextParams(params, font2, 2, 1, 270.0f, 140.0f);
	swprintf(OVP_TEXT(params), (wchar_t *)CMA_TEXT(0x180));
	CMA_Overlay_Create(params);

	GameStats_TextParams(params, font2, 2, 1, 270.0f, -180.0f);
	swprintf(OVP_TEXT(params), (wchar_t *)CMA_TEXT(0x10d));
	CMA_Overlay_Create(params);

	/* the round numbers */
	rounds = DAT_002844a0[DAT_002bae74];
	for (k = 0; k < rounds; ) {
		float x = (float)k;

		k++;
		memset(params, 0, sizeof(params));
		GameStats_TextParams(params, font1, 2, 1, x * 60.0f - 160.0f, 140.0f);
		swprintf(OVP_TEXT(params), L"%d", k);
		CMA_Overlay_Create(params);
		rounds = DAT_002844a0[DAT_002bae74];
	}

	/* the totals before this round, and this round's points */
	for (i = 0; i < DAT_002bae94; i++) {
		int r = 0;

		self->total[i] = 0;
		if (DAT_002b9968[0] != 1 && DAT_002b9968[0] - 1 >= 0) {
			for (r = 0; r < DAT_002b9968[0] - 1; r++)
				self->total[i] = self->total[i] + CMA_ROUND_POINTS(r, i);
		}
		self->target[i] = CMA_ROUND_POINTS(r, i);
		self->shown[i] = 0;
		self->counted[i] = 0;
	}
	FUN_0001c200(self->total, self->place, self->order);

	/* a row per player, in place order */
	for (slot = 0; slot < DAT_002bae94; slot++) {
		int p = self->order[slot];
		void *portrait;
		int r;

		if (DAT_002baed0[p].character == -1)
			DAT_002baed0[p].character = p;
		memset(params, 0, sizeof(params));
		CMA_FIELD(int, params, 0x288) = 0;
		CMA_FIELD(int, params, 0x4) = (int)FUN_0004bb30(0, (char *)CMA_CHAR_PICTURE(DAT_002baed0[p].character), 0, 0xdefa);
		CMA_FIELD(float, params, 0x250) = 128.0f;
		CMA_FIELD(float, params, 0xc) = 90.0f - (float)(slot * 60);
		CMA_FIELD(float, params, 0x254) = 128.0f;
		CMA_FIELD(float, params, 0x258) = 128.0f;
		CMA_FIELD(float, params, 0x25c) = 128.0f;
		CMA_FIELD(float, params, 0x2a4) = -270.0f;
		CMA_FIELD(float, params, 0x8) = -270.0f;
		CMA_FIELD(float, params, 0x308) = 96.0f;
		CMA_FIELD(float, params, 0x30c) = 96.0f;
		CMA_FIELD(int, params, 0x260) = 8;
		CMA_FIELD(int, params, 0x268) = 0;
		CMA_FIELD(int, params, 0x264) = 0x7f;
		CMA_FIELD(int, params, 0x28c) = 5;
		CMA_FIELD(float, params, 0x284) = 8.0f;
		CMA_FIELD(float, params, 0x2a8) = CMA_FIELD(float, params, 0xc);
		portrait = CMA_Overlay_Create(params);
		self->portrait[p] = portrait;
		for (r = 0; r < DAT_002b9968[0]; r++) {
			void *text;

			memset(params, 0, sizeof(params));
			CMA_FIELD(float, params, 0x2b4) = (float)r * 60.0f + 110.0f;
			CMA_FIELD(int, params, 0x28c) = 1;
			CMA_FIELD(short, params, 0x24e) = 1;
			CMA_FIELD(int, params, 0x288) = 2;
			CMA_FIELD(short, params, 0x24c) = 2;
			CMA_FIELD(float, params, 0x2b8) = 30.0f;
			CMA_FIELD(int, params, 0x2bc) = 0;
			CMA_FIELD(float, params, 0x250) = 127.0f;
			CMA_FIELD(float, params, 0x254) = 127.0f;
			CMA_FIELD(float, params, 0x258) = 127.0f;
			CMA_FIELD(float, params, 0x25c) = 90.0f;
			if (r < DAT_002b9968[0] - 1) {
				CMA_FIELD(int, params, 0x44) = font2;
				swprintf(OVP_TEXT(params), L"%d", CMA_ROUND_POINTS(r, p));
			} else {
				GameStats_GlowParams(params);
				CMA_FIELD(int, params, 0x44) = font2;
				FUN_0004b130((short *)OVP_TEXT(params), (short *)u"0");
			}
			text = CMA_Overlay_CreateChild(self->portrait[p], params);
			if (r == DAT_002b9968[0] - 1)
				self->roundText[p] = text;
		}
		memset(params, 0, sizeof(params));
		CMA_FIELD(int, params, 0x288) = 2;
		CMA_FIELD(short, params, 0x24c) = 2;
		CMA_FIELD(short, params, 0x24e) = 1;
		CMA_FIELD(float, params, 0x250) = 128.0f;
		CMA_FIELD(float, params, 0x254) = 128.0f;
		CMA_FIELD(float, params, 0x258) = 128.0f;
		CMA_FIELD(float, params, 0x25c) = 90.0f;
		CMA_FIELD(float, params, 0x2b4) = 540.0f;
		CMA_FIELD(float, params, 0x2b8) = 30.0f;
		CMA_FIELD(int, params, 0x2bc) = 0;
		GameStats_GlowParams(params);
		CMA_FIELD(int, params, 0x44) = font1;
		swprintf(OVP_TEXT(params), L"%d", self->total[p]);
		self->totalText[p] = CMA_Overlay_CreateChild(self->portrait[p], params);
	}
	self->unka8 = 0;
	self->timer = 20.0f;
	self->tickSound = (int)FUN_0004bb30(0, "kickimpact2.wav", 2, 0xdefa);
	self->swipeSound = (int)FUN_0004bb30(0, "punchswipe1.wav", 2, 0xdefa);
}

/* 0x0001a520  Core (unplaced): count this round's points into the totals; rows move to the new
   places */
void CGameMode_GameStats_Update(struct CGameMode_GameStats *self, int active)
{
	int counting = 0;
	int i;

	if (DAT_002ecdec != 0)
		return;
	if (self->standalone != 0)
		CMA_MenuMode_Process(self, active);
	for (i = 0; i < 4; i++) {
		int step;
		int left;

		if (self->counted[i] != 0)
			continue;
		left = self->target[i] - self->shown[i];
		step = (int)((double)DAT_0028fda8 * 2.5f);
		if (step >= left)
			step = left;
		self->shown[i] = self->shown[i] + step;
		self->total[i] = self->total[i] + step;
		swprintf((wchar_t *)((char *)self->roundText[i] + 0xa4), L"%d", self->shown[i]);
		swprintf((wchar_t *)((char *)self->totalText[i] + 0xa4), L"%d", self->total[i]);
		if (self->shown[i] == self->target[i]) {
			CMA_FIELD(float, self->roundText[i], 0x37c) = 2.0f;
			CMA_FIELD(int, self->roundText[i], 0x2f8) = 1;
			CMA_FIELD(float, self->roundText[i], 0x2b8) = 127.0f;
			CMA_FIELD(float, self->totalText[i], 0x37c) = 2.0f;
			CMA_FIELD(int, self->totalText[i], 0x2f8) = 1;
			CMA_FIELD(float, self->totalText[i], 0x2b8) = 127.0f;
			if (self->tickSound != 0)
				FUN_00054ba0((struct BSample *)self->tickSound, 0xff, 0, 0, 0);
			self->counted[i] = 1;
		}
		counting++;
	}
	if (counting == 0) {
		self->timer = 0.0f;
	} else {
		int old[4];
		int moved = 0;

		old[0] = self->place[0];
		old[1] = self->place[1];
		old[2] = self->place[2];
		old[3] = self->place[3];
		FUN_0001c200(self->total, self->place, self->order);
		for (i = 0; i < 4; i++) {
			if (old[i] != self->place[i]) {
				void *portrait = self->portrait[i];
				float y = 90.0f - (float)(self->place[i] * 60);
				float d = CMA_FIELD(float, portrait, 0x68) - y;

				moved = 1;
				CMA_FIELD(float, portrait, 0x2e0) = fabsf(d) * 0.1f;
				CMA_FIELD(float, portrait, 0x68) = y;
				CMA_FIELD(int, portrait, 0x2f0) = 1;
			}
		}
		if (moved)
			CMA_PlaySample(self->swipeSound, 0xff, 0, 0, 0);
	}
	FUN_00072720((undefined4)&self->overlays);
	FUN_00074be0();
}
