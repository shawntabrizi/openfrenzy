/*
Core/CGameMode_AskAgain.c  <-  C:\BlitzParty\Game\Src\Core\CGameMode_AskAgain.cpp

The object file holds more than the AskAgain screen: the CGameMode loading hooks,
the character voice packages, the attract mode, the 3D background mode
(CGameMode_BGEffects) with its effect group, the boot sequence and the Bink
video player call.
*/

#include "core_modes_a.h"
#include "core_modes_a_modes.h"

#include "core_modes_a_globals.h"

/* platform (XDK) functions (declared here, not through xtl.h) */
long IDirectSound_GetCaps(void *sound, void *caps);
void D3DDevice_GetDisplayFieldStatus(void *status);

/* classlib_ui: the intro camera (thiscall on DAT_002ecfac) */
#define CMA_IntroCam_Reset() ((void (*)(void *))FUN_000796c0)(DAT_002ecfac)
#define CMA_IntroCam_Set(p) ((void (*)(void *, void *))FUN_0007a200)(DAT_002ecfac, (p))
#define CMA_IntroCam_Start() FUN_0007be80() /* ECX = DAT_002ecfac, unused */

OF_ASSERT_SIZE(struct CGameMode, 0x28);
OF_ASSERT_SIZE(struct CGameModeMenu, 0x98);
OF_ASSERT_SIZE(struct CGameMode_AskAgain, 0xc0);
OF_ASSERT_SIZE(struct CBGEffectGroup, 0x20);     /* new(0x20) */
OF_ASSERT_SIZE(struct CGameMode_BGEffects, 0x84);
OF_ASSERT_OFFSET(struct CGameMode_BGEffects, group, 0x64);

static const char s_AskAgain_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_AskAgain.cpp";

/* ---------- vtables */

/* 0x002538a4 CGameMode */
const void *const vtable_002538a4[] = {
	(const void *)CGameMode_Delete,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
};

/* 0x00253928 CGameMode_AskAgain */
const void *const vtable_00253928[] = {
	(const void *)CGameMode_AskAgain_Delete,
	(const void *)CGameMode_AskAgain_Suspend,
	(const void *)CGameMode_AskAgain_Resume,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_AskAgain_Pushed,
	(const void *)FUN_0002fe60,
	(const void *)CGameMode_AskAgain_Update,
	(const void *)CGameMode_AskAgain_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_AskAgain_HandleButton,
};

/* 0x002539c0 CGameMode_AttractMode */
const void *const vtable_002539c0[] = {
	(const void *)CGameMode_AttractMode_Delete,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_AttractMode_Resume,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
};

/* 0x00253bac CBGEffectGroup */
const void *const vtable_00253bac[] = {
	(const void *)CBGEffectGroup_Delete,
	(const void *)FUN_001d5950,
	(const void *)CBGEffectGroup_Update,
	(const void *)CBGEffectGroup_Render,
	(const void *)CBGEffectGroup_Show,
	(const void *)CBGEffectGroup_Hide,
};

/* 0x00253bc4 CGameMode_BGEffects */
const void *const vtable_00253bc4[] = {
	(const void *)CGameMode_BGEffects_Delete,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_BGEffects_Resume,
	(const void *)CGameMode_BGEffects_Popped,
	(const void *)CGameMode_BGEffects_Pushed,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_BGEffects_Update,
	(const void *)CGameMode_BGEffects_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_ClearLoadFlags,
	(const void *)CGameMode_BGEffects_LoadingStarted,
	(const void *)FUN_001d5950,
};

/* 0x00253c04 CGameMode_BootSequence */
const void *const vtable_00253c04[] = {
	(const void *)CGameMode_BootSequence_Delete,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_BootSequence_Resume,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
};

/* restore the clear colour of the menus */
static void CMA_ResetClearColour(void)
{
	DAT_002ba940 = DAT_002844cc;
	DAT_002ba944 = DAT_002844d0;
	DAT_002ba948 = DAT_002844d4;
	DAT_002ba94c = DAT_002844d8;
}

/* ---------- CGameMode */

/* 0x000128e0  Core/CGameMode_AskAgain.cpp: CGameMode::FinishedLoading */
void CGameMode_FinishedLoading(struct CGameMode *self)
{
	struct CGameMode *intro;

	self->loadState = 0;
	DAT_002d02ac = 0;
	DAT_002ba9ac = 0;
	intro = CMA_ModeStack_Find("MiniGameIntro");
	if (intro != 0) {
		CMA_DebugPrintf("CGameMode::FinishedLoading reports loading complete\n");
		CMA_FIELD(int, intro, 0x2f4) = 1;
	}
}

/* 0x00012930  Core/CGameMode_AskAgain.cpp: poll the background packages; when all are in,
   call LoadingStarted (if the mode is loading) and FinishedLoading */
void CGameMode_UpdateLoading(struct CGameMode *self)
{
	self->unk04 = 0;
	if (FUN_0004d6d0((struct bkEventClient *)DAT_002ba9a4, 0) == 1)
		DAT_002ba9b0 = DAT_002ba9b0 - 1;
	if (FUN_0004d6d0((struct bkEventClient *)DAT_002ba9a8, 0) == 1) {
		DAT_002ba9b0 = DAT_002ba9b0 - 1;
		FUN_0004b870((struct bkPackage *)DAT_002b9670, 0xffffffff, 0xdefa, 0);
		DAT_002b95ac = 1;
	}
	if (DAT_002ba9b0 == 0) {
		FUN_0004c290((const char *)self->name);
		if (self->loadState == 2)
			CMA_VFN(self, 0x2c, void (*)(struct CGameMode *))(self);
		CMA_VFN(self, 0x28, void (*)(struct CGameMode *))(self);
		FUN_0004c2c0();
	}
}

/* 0x000129d0  Core/CGameMode_AskAgain.cpp: wait for the CharacterSel package (updating the
   loading of the top mode), then finish and release it */
void Core_FinishCharacterSelPackage(void)
{
	if (DAT_002b9670 == 0)
		return;
	while (DAT_002b95ac == 0) {
		FUN_0004c6a0(10);
		{
			struct CGameMode *top = CMA_MODE_AT(CMA_MODE_COUNT - 1);
			CMA_VFN(top, 0x24, void (*)(struct CGameMode *))(top);
		}
	}
	FUN_0004e010(FUN_0004dce0((struct bkPackage *)DAT_002b9670));
	FUN_0004db40((struct bkPackage *)DAT_002b9670);
	DAT_002b9670 = 0;
	DAT_002b95ac = 0;
}

/* 0x00012a40  Core/CGameMode_AskAgain.cpp: CGameMode scalar deleting destructor */
struct CGameMode *CGameMode_Delete(struct CGameMode *self, unsigned int flags)
{
	CGameMode_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00012a60  Core/CGameMode_AskAgain.cpp: load the voice packages of the characters the
   players chose and release the others (FLUSH: stop the voices first) */
void Core_LoadCharacterVoices(int flush)
{
	char path[128];
	int count;
	int i;

	Core_FinishCharacterSelPackage();
	if (flush != 0)
		FUN_00071880(1);
	count = DAT_002bae94;
	for (i = 0; i < 6; i++) {
		int used = 0;
		int j;

		DAT_002b95b0[i] = 0;
		for (j = 0; j < count; j++) {
			if (DAT_002baed0[j].character == i) {
				used = 1;
				if (CMA_CHAR_VOICE(i) == 0) {
					DAT_002b95b0[i] = 1;
					break;
				}
			}
		}
		if (j < count && DAT_002b95b0[i] != 0)
			continue;
		if (!used && CMA_CHAR_VOICE(i) != 0) {
			FUN_0004db40((struct bkPackage *)CMA_CHAR_VOICE(i));
			CMA_CHAR_VOICE(i) = 0;
			FUN_00071130(i);
			count = DAT_002bae94;
		}
	}
	for (i = 0; i < 6; i++) {
		if (DAT_002b95b0[i] != 0) {
			sprintf(path, "%s\\%s", (const char *)DAT_002ba544, CMA_CHAR_NAME(i));
			CMA_CHAR_VOICE(i) = CMA_LoadPackageAsync(path, 0);
		}
	}
}

/* ---------- CGameMode_AskAgain */

/* 0x00012b50  Core/CGameMode_AskAgain.cpp */
struct CGameMode_AskAgain *CGameMode_AskAgain_ctor(struct CGameMode_AskAgain *self)
{
	FUN_00030090((struct CMenuMode *)self, "AskAgain");
	self->menu.base.vtbl = vtable_00253928;
	CMA_List_Init(&self->overlays);
	FUN_00026bf0();
	self->unkb0 = 0;
	self->font = 0;
	self->selection = 0;
	self->menu.base.flags |= 2;
	return self;
}

/* 0x00012bd0  Core/CGameMode_AskAgain.cpp: pushed */
void CGameMode_AskAgain_Pushed(struct CGameMode_AskAgain *self)
{
	self->font = (int)FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa);
}

/* 0x00012c00  Core/CGameMode_AskAgain.cpp: suspend: fade the texts out */
void CGameMode_AskAgain_Suspend(struct CGameMode_AskAgain *self)
{
	int i;

	for (i = 0; i < 2; i++) {
		struct CBGTypeText *option = self->options[i];

		if (option == 0)
			break;
		CMA_VFN(option, 0x14, void (*)(void *, float))(option, 0.0f);
		self->options[i] = 0;
	}
	if (self->question != 0) {
		CMA_VFN(self->question, 0x14, void (*)(void *, float))(self->question, 0.0f);
		self->question = 0;
	}
	FUN_00072720((undefined4)&self->overlays);
	FUN_00074ba0();
}

/* 0x00012c60  Core/CGameMode_AskAgain.cpp: buttons: 0 up, 2 down, 4/5 select */
int CGameMode_AskAgain_HandleButton(struct CGameMode_AskAgain *self, int button, int player)
{
	(void)player;
	switch (button) {
	case 0:
		CBGTypeText_Deselect(self->options[self->selection]);
		self->selection = self->selection - 1;
		if (self->selection < 0) {
			self->selection = 0;
			CBGTypeText_Select(self->options[self->selection], 0.0f);
			return 0;
		}
		CMA_PlayGeneric(7, 0x7f, 0, 0, 0);
		CBGTypeText_Select(self->options[self->selection], 0.0f);
		return 0;
	case 2:
		CBGTypeText_Deselect(self->options[self->selection]);
		self->selection = self->selection + 1;
		if (self->selection == 2)
			self->selection = 1;
		else
			CMA_PlayGeneric(7, 0x7f, 0, 0, 0);
		CBGTypeText_Select(self->options[self->selection], 0.0f);
		break;
	case 4:
	case 5:
		CMA_PlayGeneric(9, 0x7f, 0, 0, 0);
		if (self->selection == 0) {
			int kind;
			void *game;

			DAT_0028198c = 1;
			CMA_VFN(self, 0x04, void (*)(void *))(self);
			CMA_ModeStack_Pop(0);
			kind = PTR_s_packages_english_002839e0[DAT_002baeb0 + DAT_002baeac * 4];
			if ((kind < 0 ? -kind : kind) == 3)
				DAT_002baebc = 1;
			game = DAT_002ba954[DAT_002838f4];
			CMA_VFN(game, 0x0c, int (*)(void *, int))(game, DAT_002838f8);
			return 1;
		}
		if (self->selection == 1) {
			DAT_0028198c = -1;
			CMA_ModeStack_Pop(1);
			return 1;
		}
		break;
	}
	return 0;
}

/* 0x00012e00  Core/CGameMode_AskAgain.cpp */
void CGameMode_AskAgain_Render(struct CGameMode_AskAgain *self, int active)
{
	if (active == 0)
		return;
	if (DAT_002b95c8 != 0)
		FUN_00071c20(0xff, 0);
	FUN_00072720((undefined4)&self->overlays);
	FUN_00073900(1);
}

/* 0x00012e40  Core/CGameMode_AskAgain.cpp */
void CGameMode_AskAgain_Update(struct CGameMode_AskAgain *self, int active)
{
	if (active != 0 && DAT_002ecdec == 0) {
		DAT_002b95c8 = 0;
		FUN_00072720((undefined4)&self->overlays);
		FUN_00074be0();
	}
}

/* 0x00012e70  Core/CGameMode_AskAgain.cpp */
struct CGameMode_AskAgain *CGameMode_AskAgain_Delete(struct CGameMode_AskAgain *self,
	unsigned int flags)
{
	CGameMode_AskAgain_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00012e90  Core/CGameMode_AskAgain.cpp */
void CGameMode_AskAgain_dtor(struct CGameMode_AskAgain *self)
{
	FUN_001d5950(); /* ~CMA_List (nothing) */
	FUN_00030190((struct CMenuMode *)self);
}

/* 0x00012ee0  Core/CGameMode_AskAgain.cpp: resume: build the question and the two answers */
void CGameMode_AskAgain_Resume(struct CGameMode_AskAgain *self)
{
	static const int answerText[2] = { 1, 2 };
	struct CGameMode *select;
	void *memory;
	int i;
	int y;

	select = CMA_ModeStack_Find("MiniGameSelect");
	FUN_00054730();
	FUN_00054690(DAT_002bae6c);
	CMA_ResetClearColour();
	DAT_00284408 = 1;
	DAT_002b95c8 = 1;
	DAT_002bae9c = 4;
	FUN_00071690();
	memory = CMA_New(0xb0, s_AskAgain_cpp, 0x5b, 0x1001);
	self->question = memory == 0 ? 0
		: CBGTypeTextBox_ctor(memory, -220, 175, CMA_TEXT(0x1ba), self->font);
	CBGTypeText_SetGreyStyle(&self->question->type);
	CBGEffect_AddToBackground(&self->question->type.buf.text.base, 1);
	if (DAT_002b95d0 != 0)
		DAT_002b95d0->active = 1;
	for (i = 0, y = 50; y > -50; i++, y -= 50) {
		memory = CMA_New(0x90, s_AskAgain_cpp, 100, 0x1001);
		self->options[i] = memory == 0 ? 0
			: CBGTypeText_ctor(memory, -200, y, CMA_TEXT(answerText[i]), self->font);
		if (i == self->selection)
			CBGTypeText_Select(self->options[i], 0.0f);
		else
			CBGTypeText_Deselect(self->options[i]);
		CBGEffect_AddToBackground(&self->options[i]->buf.text.base, 1);
	}
	FUN_000724b0(2, 0, 0xff, 0xffffffec, 0x14, 0);
	FUN_00072720((undefined4)&self->overlays);
	if (select != 0) {
		CMA_IntroCam_Reset();
		CMA_IntroCam_Set((char *)select + 0x1b0);
		CMA_IntroCam_Start();
	}
}

/* ---------- CGameMode_AttractMode */

/* 0x00013110  Core/CGameMode_AskAgain.cpp */
struct CGameMode_AttractMode *CGameMode_AttractMode_ctor(struct CGameMode_AttractMode *self)
{
	CGameMode_ctor(&self->base, "AttractMode");
	self->base.refCount = self->base.refCount + 1;
	self->base.vtbl = vtable_002539c0;
	self->playVideo = 1;
	return self;
}

/* 0x00013140  Core/CGameMode_AskAgain.cpp */
struct CGameMode_AttractMode *CGameMode_AttractMode_Delete(struct CGameMode_AttractMode *self,
	unsigned int flags)
{
	CGameMode_AttractMode_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00013160  Core/CGameMode_AskAgain.cpp (a jump to the CGameMode destructor) */
void CGameMode_AttractMode_dtor(struct CGameMode_AttractMode *self)
{
	CGameMode_dtor(&self->base);
}

/* 0x00013170  Core/CGameMode_AskAgain.cpp: every other resume plays the attract video;
   otherwise pick random players and a random game and start it */
void CGameMode_AttractMode_Resume(struct CGameMode_AttractMode *self)
{
	struct {
		unsigned int free2D, free3D, freeSGEs, memory;
	} caps;
	int used[6];
	unsigned int seed;
	int game;
	int variation;
	int last;
	int kind;
	int p;

	DAT_00292c50 = 0x18;
	DAT_00292c4c = 0x18;
	memset(used, 0, sizeof(used));
	DAT_002ecde8 = 0;
	DAT_00292c78 = 0x240;
	IDirectSound_GetCaps(DAT_002db808, &caps);
	CMA_DebugPrintf("Free2DBuffers = %d, Free3DBuffers = %d, FreeBufferSGEs = %d, MemoryAllocated = %d Kb\n",
		caps.free2D, caps.free3D, caps.freeSGEs, caps.memory >> 10);
	if (self->playVideo != 0) {
		FUN_00071ff0(0xffffff01);
		if (Core_PlayVideo("Packages\\Video\\ATTRACT.bik", 8) != 0)
			DAT_00283760 = 1800.0f;
		DAT_002baeb8 = 0;
		self->playVideo = 0;
		CMA_ModeStack_Pop(1);
		return;
	}
	if (DAT_002baeb8 != 0) {
		self->playVideo = 1;
		DAT_002baeb8 = 0;
		CMA_ModeStack_Pop(1);
		return;
	}

	/* four players with different random characters */
	seed = DAT_00285630;
	for (p = 0; p < 4; p++) {
		int c;

		DAT_002baed0[p].joined = 0;
		do {
			seed = seed * 0x343fd + 0x269ec3;
			c = (int)((seed >> 16) & 0x7fff) % 6;
			DAT_00285630 = seed;
		} while (used[c] != 0);
		DAT_002baed0[p].character = c;
		used[c] = 1;
	}

	/* a random game, not the last one, with a variation that exists */
	last = DAT_00281990;
	DAT_002bae90 = 4;
	DAT_002bae8c = 0;
	do {
		seed = seed * 0x343fd + 0x269ec3;
		game = (int)((seed >> 16) & 0x7fff) % 13;
		DAT_00285630 = seed;
		seed = seed * 0x343fd + 0x269ec3;
		DAT_00285630 = seed;
		DAT_002baeac = game;
		if (game == 0)
			variation = (int)((seed >> 16) & 0x7fff) % 6;
		else
			variation = (int)((seed >> 16) & 0x7fff) % 4;
		DAT_002baeb0 = variation;
	} while (game == last ||
		(game != 0 && PTR_s_packages_english_002839e4[variation + game * 4] == 0));
	kind = PTR_s_packages_english_002839e4[variation + game * 4];
	DAT_002baebc = (kind < 0 ? -kind : kind) == 3;
	DAT_002baeb8 = 1;
	DAT_002bae98 = 1;
	DAT_00281990 = game;
	{
		void *module = DAT_002ba954[game];
		CMA_VFN(module, 0x0c, int (*)(void *, int))(module, variation + 1);
	}
}

/* ---------- CGameMode_BGEffects */

/* 0x000133c0  Core/CGameMode_AskAgain.cpp: pushed: the two fonts */
void CGameMode_BGEffects_Pushed(struct CGameMode_BGEffects *self)
{
	self->font1 = (int)FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa);
	self->font2 = (int)FUN_0004bb30(0, "ocraext.ttf", 3, 0xdefa);
}

/* 0x00013400  Core/CGameMode_AskAgain.cpp: popped */
void CGameMode_BGEffects_Popped(struct CGameMode_BGEffects *self)
{
	if (self->package != 0)
		FUN_0004db40((struct bkPackage *)self->package);
	Core_FinishCharacterSelPackage();
	DAT_002b95d4 = 0;
}

/* 0x00013420  Core/CGameMode_AskAgain.cpp */
void CGameMode_BGEffects_Update(struct CGameMode_BGEffects *self, int active)
{
	(void)active;
	if (DAT_002ecdec != 0)
		return;
	if (self->base.loadState != 0)
		CMA_VFN(self, 0x24, void (*)(void *))(self);
	if (self->active != 0) {
		CMA_VFN(&self->group, 0x08, int (*)(void *))(&self->group);
		CMA_ResetClearColour();
	}
}

/* 0x00013480  Core/CGameMode_AskAgain.cpp: camera, fog and the effects */
void CGameMode_BGEffects_Render(struct CGameMode_BGEffects *self, int active)
{
	(void)active;
	CMA_SetClipPlanes(1.0f, self->farPlane);
	FUN_00044a00(self->fov, (float)((double)self->fov * 0.75f));
	FUN_000435f0(self->eye, self->target, 0.0f);
	FUN_00043790();
	FUN_00044dd0(self->fogStart, self->farPlane);
	FUN_00044e20(DAT_002844cc, DAT_002844d0, DAT_002844d4);
	FUN_00044b00(4, 0, 0);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 0, 0);
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(7, 1, 0);
	FUN_00044b00(8, 1, 0);
	FUN_00043b70(0);
	FUN_00044b00(1, 2, 0);
	FUN_000438b0();
	CMA_VFN(&self->group, 0x0c, void (*)(void *))(&self->group);
}

/* 0x00013560  Core/CGameMode_AskAgain.cpp: background loading finished: start loading the
   CharacterSel package */
void CGameMode_BGEffects_LoadingStarted(struct CGameMode_BGEffects *self)
{
	if (DAT_002b95ac != 0) {
		self->base.loadState = 0;
		return;
	}
	CMA_DebugPrintf("CGameMode_BGEffects: Background loading complete, starting wireframe effect\n");
	if (DAT_002b9670 == 0) {
		DAT_002b9670 = CMA_LoadPackageAsync("Packages\\CharacterSel", 1);
		if (DAT_002b9670 == 0)
			CMA_DebugPrintf("CGameMode_MainMenu::GainFocus *** Could not load package 'Packages\\CharacterSel' ***\n");
	}
}

/* 0x000135b0  Core/CGameMode_AskAgain.cpp: FinishedLoading of BGEffects and CharacterSelect */
void CGameMode_ClearLoadFlags(struct CGameMode *self)
{
	(void)self;
	DAT_002d02ac = 0;
	DAT_002ba9ac = 0;
}

/* ---------- CBGEffect and CBGEffectGroup */

/* 0x000135c0  Core/CGameMode_AskAgain.cpp: CBGEffect::Show */
void CBGEffect_Show(struct CBGEffect *self, float time)
{
	(void)time;
	self->flags |= 4;
}

/* 0x000135d0  Core/CGameMode_AskAgain.cpp: CBGEffect::Hide */
void CBGEffect_Hide(struct CBGEffect *self, float time)
{
	(void)time;
	self->flags |= 8;
}

/* 0x000135e0  Core/CGameMode_AskAgain.cpp: delete every child */
void CBGEffectGroup_DeleteAll(struct CBGEffectGroup *self)
{
	struct CMA_ListNode *node = self->children.next;

	while (node != (struct CMA_ListNode *)&self->children) {
		CMA_List_Remove(&self->children, node);
		if (node != 0 && (struct CBGEffect *)((char *)node - 4) != 0) {
			struct CBGEffect *child = (struct CBGEffect *)((char *)node - 4);
			CMA_VFN(child, 0, void *(*)(void *, unsigned int))(child, 1);
		}
		node = self->children.next;
	}
}

/* 0x00013620  Core/CGameMode_AskAgain.cpp: update the children (deleting the finished ones);
   returns 0 when hidden and empty */
int CBGEffectGroup_Update(struct CBGEffectGroup *self)
{
	struct CMA_ListNode *node;

	for (node = CMA_List_Next(&self->children, 0); node != 0;
		node = CMA_List_Next(&self->children, node)) {
		struct CBGEffect *child = (struct CBGEffect *)((char *)node - 4);

		if (CMA_VFN(child, 0x08, int (*)(void *))(child) == 0 && self->deleteFinished != 0) {
			node = CMA_List_Remove(&self->children, node);
			CMA_VFN(child, 0, void *(*)(void *, unsigned int))(child, 1);
		}
	}
	if ((self->base.flags & 8) != 0 && self->children.count == 0) {
		CMA_ResetClearColour();
		return 0;
	}
	return 1;
}

/* 0x000136c0  Core/CGameMode_AskAgain.cpp */
void CBGEffectGroup_Render(struct CBGEffectGroup *self)
{
	struct CMA_ListNode *node;

	for (node = CMA_List_Next(&self->children, 0); node != 0;
		node = CMA_List_Next(&self->children, node)) {
		struct CBGEffect *child = (struct CBGEffect *)((char *)node - 4);
		CMA_VFN(child, 0x0c, void (*)(void *))(child);
	}
}

/* 0x000136f0  Core/CGameMode_AskAgain.cpp */
void CBGEffectGroup_Show(struct CBGEffectGroup *self, float time)
{
	struct CMA_ListNode *node;

	self->base.flags = (self->base.flags & ~8u) | 4;
	for (node = CMA_List_Next(&self->children, 0); node != 0;
		node = CMA_List_Next(&self->children, node)) {
		struct CBGEffect *child = (struct CBGEffect *)((char *)node - 4);
		CMA_VFN(child, 0x10, void (*)(void *, float))(child, time);
	}
}

/* 0x00013740  Core/CGameMode_AskAgain.cpp */
void CBGEffectGroup_Hide(struct CBGEffectGroup *self, float time)
{
	struct CMA_ListNode *node;

	self->base.flags = (self->base.flags & ~4u) | 8;
	for (node = CMA_List_Next(&self->children, 0); node != 0;
		node = CMA_List_Next(&self->children, node)) {
		struct CBGEffect *child = (struct CBGEffect *)((char *)node - 4);
		CMA_VFN(child, 0x14, void (*)(void *, float))(child, time);
	}
}

/* 0x00013790  Core/CGameMode_AskAgain.cpp: resume: start loading Packages\bgeffects once */
void CGameMode_BGEffects_Resume(struct CGameMode_BGEffects *self)
{
	if (DAT_002b95d4 != 0)
		return;
	if (self->package == 0) {
		self->package = CMA_LoadPackageAsync("Packages\\bgeffects", 0);
		if (self->package == 0) {
			CMA_DebugPrintf("CGameMode_BGEffects::GainFocus() : couldn't start background loading package\n");
			return;
		}
		self->base.loadState = 2;
	}
	DAT_002b95d4 = 1;
}

/* 0x000137e0  Core/CGameMode_AskAgain.cpp: add an effect to the BGEffects list (and Init it) */
void CBGEffect_AddToBackground(struct CBGEffect *self, int init)
{
	CMA_List_Insert(&DAT_002b95d0->group.children, self == 0 ? 0 : &self->node, 1);
	if (init != 0)
		CMA_VFN(self, 0x04, void (*)(void *))(self);
}

/* 0x00013820  Core/CGameMode_AskAgain.cpp */
struct CBGEffectGroup *CBGEffectGroup_ctor(struct CBGEffectGroup *self)
{
	self->base.flags = 3;
	self->base.vtbl = vtable_00253bac;
	CMA_List_Init(&self->children);
	self->deleteFinished = 1;
	return self;
}

/* 0x00013870  Core/CGameMode_AskAgain.cpp */
void CBGEffectGroup_dtor(struct CBGEffectGroup *self)
{
	self->base.vtbl = vtable_00253bac;
	CBGEffectGroup_DeleteAll(self);
	FUN_001d5950(); /* ~CMA_List (nothing) */
	self->base.vtbl = vtable_00253744;
}

/* 0x000138d0  Core/CGameMode_AskAgain.cpp */
struct CGameMode_BGEffects *CGameMode_BGEffects_ctor(struct CGameMode_BGEffects *self)
{
	CGameMode_ctor(&self->base, "BGeffects");
	self->base.vtbl = vtable_00253bc4;
	CBGEffectGroup_ctor(&self->group);
	self->farPlane = 250.0f;
	self->fogStart = 100.0f;
	self->fov = 1.0471975f;
	self->eye[0] = 0.0f;
	self->eye[1] = 0.0f;
	self->eye[2] = -150.0f;
	self->eye[3] = 1.0f;
	self->target[3] = 1.0f;
	self->target[0] = 0.0f;
	self->target[1] = 0.0f;
	self->target[2] = 0.0f;
	DAT_002b95d0 = self;
	self->package = 0;
	self->base.refCount = self->base.refCount + 1;
	self->active = 1;
	return self;
}

/* 0x00013970  Core/CGameMode_AskAgain.cpp */
void CGameMode_BGEffects_dtor(struct CGameMode_BGEffects *self)
{
	self->base.vtbl = vtable_00253bc4;
	CBGEffectGroup_dtor(&self->group);
	CGameMode_dtor(&self->base);
}

/* 0x000139d0  Core/CGameMode_AskAgain.cpp */
struct CBGEffectGroup *CBGEffectGroup_Delete(struct CBGEffectGroup *self, unsigned int flags)
{
	CBGEffectGroup_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x000139f0  Core/CGameMode_AskAgain.cpp */
struct CGameMode_BGEffects *CGameMode_BGEffects_Delete(struct CGameMode_BGEffects *self,
	unsigned int flags)
{
	CGameMode_BGEffects_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* ---------- CGameMode_BootSequence */

/* 0x00013a10  Core/CGameMode_AskAgain.cpp */
struct CGameMode *CGameMode_BootSequence_ctor(struct CGameMode *self)
{
	CGameMode_ctor(self, "BootSequence");
	self->vtbl = vtable_00253c04;
	return self;
}

/* 0x00013a30  Core/CGameMode_AskAgain.cpp */
void CGameMode_BootSequence_dtor(struct CGameMode *self)
{
	self->vtbl = vtable_00253c04;
	CGameMode_dtor(self);
}

/* 0x00013a40  Core/CGameMode_AskAgain.cpp */
struct CGameMode *CGameMode_BootSequence_Delete(struct CGameMode *self, unsigned int flags)
{
	CGameMode_BootSequence_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00013a60  Core/CGameMode_AskAgain.cpp: play a Bink video from the disc. NAME without
   ".bik" gets the refresh rate appended ("Packages\Video\BLITZ" -> "...BLITZ60.bik").
   The demo timer loses the frames the video took. */
int Core_PlayVideo(char *name, int flags)
{
	char path[0x1000];
	struct {
		unsigned int field;
		unsigned int vblankCount;
	} status;
	int result;

	if (strstr(name, ".bik") != 0)
		sprintf(path, "%s%s", DAT_002d0758, name);
	else
		sprintf(path, "%s%s%d.bik", DAT_002d0758, name, (int)_DAT_002ba98c);
	result = CMA_PlayBink(path, flags);
	if (_DAT_002ba9c8 > 0.0f) {
		int frames;
		double timer;

		D3DDevice_GetDisplayFieldStatus(&status);
		frames = (int)status.vblankCount - DAT_002baaec;
		timer = (double)_DAT_002ba9c8 - (double)(unsigned int)frames;
		if (1.0 > timer)
			_DAT_002ba9c8 = 1.0f;
		else
			_DAT_002ba9c8 = (float)timer;
		DAT_002baaec = (int)status.vblankCount;
	}
	DAT_002baae8 = -1;
	return result;
}

/* 0x00013b60  Core/CGameMode_AskAgain.cpp: boot: the logo videos (not when started from the
   demo launcher), then the background and the first menu */
void CGameMode_BootSequence_Resume(struct CGameMode *self)
{
	(void)self;
	if (DAT_002baadc == 0) {
		Core_PlayVideo("Packages\\Video\\MICROSOFT.bik", 0);
		Core_PlayVideo("Packages\\Video\\BLITZ", 0);
		if (Core_PlayVideo("Packages\\Video\\ATTRACT.bik", 0) != 0)
			DAT_00283760 = 1800.0f;
	}
	CMA_ModeStack_Pop(1);
	CMA_ModeStack_Push(DAT_002b95d0, 1);
	CMA_ModeStack_Push((void *)DAT_002ba564, 1);
}
