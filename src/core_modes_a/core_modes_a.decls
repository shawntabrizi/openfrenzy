# core_modes_a prototype overrides: <addr> <prototype>;  (docs/ARCHITECTURE.md section 11)
# Prototypes read from the x86 code (thiscall "this" made explicit, float parameters,
# register parameters). Names are ours.

# ---------- Core/BG_Effects.cpp (and the leading function before it)
00011000 void BG_DrawRect(float x, float y, float w, float h, int *rgba);
00011160 void CBGMatrixText_Init(struct CBGMatrixText *self);
00011170 int CBGMatrixText_Update(struct CBGMatrixText *self);
00011210 void CBGMatrixText_Render(struct CBGMatrixText *self);
000112d0 void CBGText_SetText(struct CBGText *self, unsigned short *text);
000112f0 void CBGText_Render(struct CBGText *self);
00011370 int CBGTextBuf_Update(struct CBGTextBuf *self);
00011390 void CBGTextBuf_SetText(struct CBGTextBuf *self, unsigned short *text);
000113e0 int BG_CountGlyphs(unsigned short *text);
00011420 void CBGTypeText_Start(struct CBGTypeText *self, float charDelay, float startDelay, int sound);
000114b0 void CBGTypeText_Select(struct CBGTypeText *self, float pulsePhase);
000114e0 void CBGTypeText_Reset(struct CBGTypeText *self);
00011500 void CBGTypeText_Deselect(struct CBGTypeText *self);
00011520 void CBGTilingBackdrop_Init(struct CBGTilingBackdrop *self);
00011570 void CBGTilingBackdrop_Render(struct CBGTilingBackdrop *self);
00011700 void *ExternalObject_New(unsigned int size);
00011720 void ExternalObject_Delete(void *block);
00011730 unsigned int Core_Rand(void);
00011750 float Core_RandUnit(void);
00011790 struct CBGEffect *CBGEffect_Delete(struct CBGEffect *self, unsigned int flags);
000117b0 struct CBGMatrixText *CBGMatrixText_ctor(struct CBGMatrixText *self, float x, float y, int cols, int rows, int scroll, float interval, int *color);
00011890 void CBGMatrixText_dtor(struct CBGMatrixText *self);
000118e0 void CBGMatrixText_FillLine(struct CBGMatrixText *self, unsigned short *dst, int count);
00011930 unsigned int CBGMatrixTextGlitch_FillLine(struct CBGMatrixText *self, unsigned short *dst, unsigned int count);
000119e0 struct CBGText *CBGText_ctor(struct CBGText *self, int x, int y, unsigned short *text, int font, int *color);
00011a90 void CBGText_dtor(struct CBGText *self);
00011aa0 struct CBGTextBuf *CBGTextBuf_ctor(struct CBGTextBuf *self, int x, int y, unsigned short *text, int font, int *color, int capacity);
00011b70 void CBGTextBuf_dtor(struct CBGTextBuf *self);
00011bc0 void CBGTypeText_dtor(struct CBGTypeText *self);
00011c30 struct CBGTypeText *CBGTypeText_Delete(struct CBGTypeText *self, unsigned int flags);
00011c50 struct CBGTypeText *CBGTypeText_ctor(struct CBGTypeText *self, int x, int y, unsigned short *text, int font);
00011cd0 struct CBGTypeText *CBGTypeText_ctorEmpty(struct CBGTypeText *self, int x, int y, int capacity, int font);
00011d50 int CBGTypeText_Update(struct CBGTypeText *self);
00011fd0 void CBGTypeText_Render(struct CBGTypeText *self);
00012390 void CBGTypeText_Hide(struct CBGTypeText *self, float time);
00012430 void CBGTypeText_SetGreyStyle(struct CBGTypeText *self);
00012480 struct CBGTypeTextBox *CBGTypeTextBox_ctor(struct CBGTypeTextBox *self, int x, int y, unsigned short *text, int font);
000124f0 struct CBGTypeTextBox *CBGTypeTextBox_ctorEmpty(struct CBGTypeTextBox *self, int x, int y, int capacity, int font);
00012560 void CBGTypeTextBox_Render(struct CBGTypeTextBox *self);
00012730 struct CBGTilingBackdrop *CBGTilingBackdrop_ctor(struct CBGTilingBackdrop *self);
00012740 struct CBGMatrixText *CBGMatrixText_Delete(struct CBGMatrixText *self, unsigned int flags);

# ---------- Core (unplaced, between BG_Effects.cpp and CGameMode_AskAgain.cpp)
00012760 struct CBGText *CBGText_Delete(struct CBGText *self, unsigned int flags);
00012780 struct CBGTextBuf *CBGTextBuf_Delete(struct CBGTextBuf *self, unsigned int flags);
000127a0 struct CBGTypeTextBox *CBGTypeTextBox_Delete(struct CBGTypeTextBox *self, unsigned int flags);
000127c0 void CBGTypeTextBox_dtor(struct CBGTypeTextBox *self);
000127d0 void Core_OperatorDelete(void *block);
000127e0 void Core_Vec4Copy(float *dst, float *src);
00012800 struct CGameMode *CGameMode_ctor(struct CGameMode *self, char *name);
00012860 void CGameMode_dtor(struct CGameMode *self);
000128b0 void CGameMode_AddRef(struct CGameMode *self);
000128c0 int CGameMode_Release(struct CGameMode *self);

# ---------- Core/CGameMode_AskAgain.cpp
000128e0 void CGameMode_FinishedLoading(struct CGameMode *self);
00012930 void CGameMode_UpdateLoading(struct CGameMode *self);
000129d0 void Core_FinishCharacterSelPackage(void);
00012a40 struct CGameMode *CGameMode_Delete(struct CGameMode *self, unsigned int flags);
00012a60 void Core_LoadCharacterVoices(int flush);
00012b50 struct CGameMode_AskAgain *CGameMode_AskAgain_ctor(struct CGameMode_AskAgain *self);
00012bd0 void CGameMode_AskAgain_Pushed(struct CGameMode_AskAgain *self);
00012c00 void CGameMode_AskAgain_Suspend(struct CGameMode_AskAgain *self);
00012c60 int CGameMode_AskAgain_HandleButton(struct CGameMode_AskAgain *self, int button, int player);
00012e00 void CGameMode_AskAgain_Render(struct CGameMode_AskAgain *self, int active);
00012e40 void CGameMode_AskAgain_Update(struct CGameMode_AskAgain *self, int active);
00012e70 struct CGameMode_AskAgain *CGameMode_AskAgain_Delete(struct CGameMode_AskAgain *self, unsigned int flags);
00012e90 void CGameMode_AskAgain_dtor(struct CGameMode_AskAgain *self);
00012ee0 void CGameMode_AskAgain_Resume(struct CGameMode_AskAgain *self);
00013110 struct CGameMode_AttractMode *CGameMode_AttractMode_ctor(struct CGameMode_AttractMode *self);
00013140 struct CGameMode_AttractMode *CGameMode_AttractMode_Delete(struct CGameMode_AttractMode *self, unsigned int flags);
00013160 void CGameMode_AttractMode_dtor(struct CGameMode_AttractMode *self);
00013170 void CGameMode_AttractMode_Resume(struct CGameMode_AttractMode *self);
000133c0 void CGameMode_BGEffects_Pushed(struct CGameMode_BGEffects *self);
00013400 void CGameMode_BGEffects_Popped(struct CGameMode_BGEffects *self);
00013420 void CGameMode_BGEffects_Update(struct CGameMode_BGEffects *self, int active);
00013480 void CGameMode_BGEffects_Render(struct CGameMode_BGEffects *self, int active);
00013560 void CGameMode_BGEffects_LoadingStarted(struct CGameMode_BGEffects *self);
000135b0 void CGameMode_ClearLoadFlags(struct CGameMode *self);
000135c0 void CBGEffect_Show(struct CBGEffect *self, float time);
000135d0 void CBGEffect_Hide(struct CBGEffect *self, float time);
000135e0 void CBGEffectGroup_DeleteAll(struct CBGEffectGroup *self);
00013620 int CBGEffectGroup_Update(struct CBGEffectGroup *self);
000136c0 void CBGEffectGroup_Render(struct CBGEffectGroup *self);
000136f0 void CBGEffectGroup_Show(struct CBGEffectGroup *self, float time);
00013740 void CBGEffectGroup_Hide(struct CBGEffectGroup *self, float time);
00013790 void CGameMode_BGEffects_Resume(struct CGameMode_BGEffects *self);
000137e0 void CBGEffect_AddToBackground(struct CBGEffect *self, int init);
00013820 struct CBGEffectGroup *CBGEffectGroup_ctor(struct CBGEffectGroup *self);
00013870 void CBGEffectGroup_dtor(struct CBGEffectGroup *self);
000138d0 struct CGameMode_BGEffects *CGameMode_BGEffects_ctor(struct CGameMode_BGEffects *self);
00013970 void CGameMode_BGEffects_dtor(struct CGameMode_BGEffects *self);
000139d0 struct CBGEffectGroup *CBGEffectGroup_Delete(struct CBGEffectGroup *self, unsigned int flags);
000139f0 struct CGameMode_BGEffects *CGameMode_BGEffects_Delete(struct CGameMode_BGEffects *self, unsigned int flags);
00013a10 struct CGameMode *CGameMode_BootSequence_ctor(struct CGameMode *self);
00013a30 void CGameMode_BootSequence_dtor(struct CGameMode *self);
00013a40 struct CGameMode *CGameMode_BootSequence_Delete(struct CGameMode *self, unsigned int flags);
00013a60 int Core_PlayVideo(char *name, int flags);
00013b60 void CGameMode_BootSequence_Resume(struct CGameMode *self);

# ---------- Core (unplaced, before CGameMode_CharacterSelect.cpp: its helpers)
00013bd0 struct CMA_CharSelPanel *CMA_CharSelPanel_ctor(struct CMA_CharSelPanel *self);
00013bf0 void CGameMode_CharacterSelect_dtor(struct CGameMode_CharacterSelect *self);
00013c80 void CGameMode_CharacterSelect_Pushed(struct CGameMode_CharacterSelect *self);
00013d20 void CharSel_SlideOut(struct CGameMode_CharacterSelect *self, int player, int fromRight);
00013da0 int CharSel_NextFreeCharacter(int c, int player);
00013df0 int CharSel_PrevFreeCharacter(int c, int player);
00013e50 void CharSel_HideMarkers(struct CGameMode_CharacterSelect *self);
00013f30 void CharSel_RenderGroup(struct CBGEffectGroup *group);
00013fb0 void CharSel_UpdateMenu(struct CGameMode_CharacterSelect *self);
00014090 void CharSel_PlayerLeave(struct CGameMode_CharacterSelect *self, int player);
00014170 void CharSel_PlayAnim(int actor, int anim, int loop, int keep, float speed, float blend, int track);
00014250 void CharSel_Finish(struct CGameMode_CharacterSelect *self);
000142c0 void COverlayMover_SetTarget(void *self, float x, float y, float z);
000142f0 struct CGameMode_CharacterSelect *CGameMode_CharacterSelect_ctor(struct CGameMode_CharacterSelect *self);
00014380 struct CGameMode_CharacterSelect *CGameMode_CharacterSelect_Delete(struct CGameMode_CharacterSelect *self, unsigned int flags);

# ---------- Core/CGameMode_CharacterSelect.cpp
000143a0 void CGameMode_CharacterSelect_Resume(struct CGameMode_CharacterSelect *self);
00015000 void CharSel_Celebrate(struct CGameMode_CharacterSelect *self, int player);
00015160 void CGameMode_CharacterSelect_Update(struct CGameMode_CharacterSelect *self, int active);
000154d0 void CharSel_InitCamera(struct CGameMode_CharacterSelect *self);
00015740 void CharSel_AllJoined(struct CGameMode_CharacterSelect *self);
00015bf0 void CharSel_RenderCelebration(struct CGameMode_CharacterSelect *self);
00015e50 int CharSel_UpdateCelebration(struct CGameMode_CharacterSelect *self);
000160a0 void CharSel_ShowCharacter(struct CGameMode_CharacterSelect *self, int player, int character, int dir);
00016330 void CGameMode_CharacterSelect_Suspend(struct CGameMode_CharacterSelect *self);
00016440 void CharSel_Deselect(struct CGameMode_CharacterSelect *self, int player, int anim);
00016650 void CharSel_Confirm(struct CGameMode_CharacterSelect *self, int player, int anim);
000168f0 void CGameMode_CharacterSelect_Input(struct CGameMode_CharacterSelect *self, int active);
00016b80 void CGameMode_CharacterSelect_Render(struct CGameMode_CharacterSelect *self, int active);
00016d90 void CharSel_Join(struct CGameMode_CharacterSelect *self, int player, int fromPad);
00016e90 void CharSel_StartCelebration(struct CGameMode_CharacterSelect *self);
00017070 void CGameMode_CharacterSelect_LoadingStarted(struct CGameMode_CharacterSelect *self);
00017580 int CGameMode_CharacterSelect_HandleButton(struct CGameMode_CharacterSelect *self, int button, int player);

# ---------- Core/CGameMode_Credits.cpp (the unplaced methods are of this class, vtable 0x00254e58)
00017c70 struct CGameMode_Credits *CGameMode_Credits_ctor(struct CGameMode_Credits *self);
00017c90 void CGameMode_Credits_Resume(struct CGameMode_Credits *self);
00017db0 void CGameMode_Credits_Render(struct CGameMode_Credits *self, int active);
00017dc0 int CGameMode_Credits_HandleButton(struct CGameMode_Credits *self, int button, int player);
00017e00 void CGameMode_Credits_Suspend(struct CGameMode_Credits *self);
00017e60 int Credits_UpdateColumn(struct CMA_CreditsColumn *self);
000182e0 void CGameMode_Credits_Update(struct CGameMode_Credits *self, int active);

# ---------- Core/CGameMode_DemoSplash.cpp (the unplaced constructor is this class's)
00018350 struct CGameMode_DemoSplash *CGameMode_DemoSplash_ctor(struct CGameMode_DemoSplash *self);
000183b0 void CGameMode_DemoSplash_Suspend(struct CGameMode_DemoSplash *self);
000183e0 void CGameMode_DemoSplash_Update(struct CGameMode_DemoSplash *self, int active);
00018560 void CGameMode_DemoSplash_Render(struct CGameMode_DemoSplash *self, int active);
000185e0 int CGameMode_DemoSplash_HandleButton(struct CGameMode_DemoSplash *self, int button, int player);
00018630 struct CGameMode_DemoSplash *CGameMode_DemoSplash_Delete(struct CGameMode_DemoSplash *self, unsigned int flags);
00018650 void CGameMode_DemoSplash_dtor(struct CGameMode_DemoSplash *self);
000186a0 void CGameMode_DemoSplash_Resume(struct CGameMode_DemoSplash *self);

# ---------- Core/CGameMode_FullGame.cpp (the unplaced constructor is this class's)
00018800 struct CGameMode_FullGame *CGameMode_FullGame_ctor(struct CGameMode_FullGame *self);
00018830 void CGameMode_FullGame_Pushed(struct CGameMode_FullGame *self);
00018880 int CGameMode_FullGame_HandleButton(struct CGameMode *self, int button, int player);
00018890 void CGameMode_FullGame_PostDraw(struct CGameMode_FullGame *self, int active);
000188b0 void CGameMode_FullGame_Resume(struct CGameMode_FullGame *self);

# ---------- Core/CGameMode_GameSizeMenu.cpp
00018f00 void CGameMode_GameSizeMenu_Suspend(struct CGameMode_GameSizeMenu *self);
00018f80 int CGameMode_GameSizeMenu_HandleButton(struct CGameMode_GameSizeMenu *self, int button, int player);
00019110 void CGameMode_GameSizeMenu_Update(struct CGameMode_GameSizeMenu *self, int active);
00019230 void Core_Vec3Sub(float *out, float *a, float *b);
00019260 struct CGameMode_GameSizeMenu *CGameMode_GameSizeMenu_ctor(struct CGameMode_GameSizeMenu *self);
00019300 struct CGameMode_GameSizeMenu *CGameMode_GameSizeMenu_Delete(struct CGameMode_GameSizeMenu *self, unsigned int flags);
00019320 void CGameMode_GameSizeMenu_dtor(struct CGameMode_GameSizeMenu *self);
00019380 void CGameMode_GameSizeMenu_Resume(struct CGameMode_GameSizeMenu *self);
000197b0 void CGameMode_GameSizeMenu_Render(struct CGameMode_GameSizeMenu *self, int active);

# ---------- Core (unplaced): CGameMode_GameStats (vtable 0x0025513c), file unknown
000199c0 struct CGameMode_GameStats *CGameMode_GameStats_ctor(struct CGameMode_GameStats *self);
00019a30 struct CGameMode_GameStats *CGameMode_GameStats_ctorShow(struct CGameMode_GameStats *self, unsigned char standalone);
00019aa0 void CGameMode_GameStats_Popped(struct CGameMode_GameStats *self);
00019ac0 void CGameMode_GameStats_Render(struct CGameMode_GameStats *self, int active);
00019b00 int CGameMode_GameStats_HandleButton(struct CGameMode_GameStats *self, int button, int player);
00019b60 struct CGameMode_GameStats *CGameMode_GameStats_Delete(struct CGameMode_GameStats *self, unsigned int flags);
00019b80 void CGameMode_GameStats_dtor(struct CGameMode_GameStats *self);
00019bd0 void CGameMode_GameStats_Pushed(struct CGameMode_GameStats *self);
0001a520 void CGameMode_GameStats_Update(struct CGameMode_GameStats *self, int active);

# ---------- Core/CGameMode_GameWinner.cpp (the unplaced constructor is this class's)
0001a7e0 struct CGameMode_GameWinner *CGameMode_GameWinner_ctor(struct CGameMode_GameWinner *self);
0001a820 void CGameMode_GameWinner_Pushed(struct CGameMode_GameWinner *self);
0001a910 void CGameMode_GameWinner_Resume(struct CGameMode_GameWinner *self);
0001a920 void CGameMode_GameWinner_Render(struct CGameMode_GameWinner *self, int active);
0001ab10 void GameWinner_PlayAnim(struct CGameMode_GameWinner *self, int slot, int anim, int loop, int keep, float speed, float blend);
0001abe0 void GameWinner_LoadCamera(struct CGameMode_GameWinner *self, int stage);
0001ac80 void CGameModeMenu_dtor_thunk(void *self);
0001ac90 void CGameMode_GameWinner_Update(struct CGameMode_GameWinner *self, int active);
0001b820 void CGameMode_GameWinner_Popped(struct CGameMode_GameWinner *self);
0001b8e0 void CGameMode_GameWinner_LoadingStarted(struct CGameMode_GameWinner *self);
