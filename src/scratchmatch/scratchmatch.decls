# scratchmatch prototype overrides (docs/ARCHITECTURE.md section 11)
# <addr> <prototype>;   Only of_types.h / C types and "struct Name *".

# ---- ScratchMatch/CModule_ScratchMatch.c
001ba560 unsigned int *CModule_ScratchMatch_Ctor(unsigned int *self);
001ba590 void CModule_ScratchMatch_Dtor(unsigned int *self);
001ba5a0 unsigned int *CModule_ScratchMatch_Destroy(unsigned int *self, unsigned char flags);
001ba5c0 int CModule_ScratchMatch_Start(unsigned int *self, int variation);

# ---- ScratchMatch/CGameMode_ScratchMatch.c
001ba6d0 void *CGameMode_ScratchMatch_Ctor(void *self);
001ba6f0 void CGameMode_ScratchMatch_Dtor(void *self);
001ba710 void CGameMode_ScratchMatch_Enter(void *self);
001ba7a0 void CGameMode_ScratchMatch_Start(void *self);
001ba8a0 void CGameMode_ScratchMatch_Draw(void *self, int top);
001ba970 void *CGameMode_ScratchMatch_Destroy(void *self, unsigned char flags);
001ba990 void CGameMode_ScratchMatch_Init(void *self);
001bae30 void CGameMode_ScratchMatch_Process(void *self, int top);
001bb130 void CGameMode_ScratchMatch_Exit(void *self);

# ---- ScratchMatch/CScratchmatch_Sound.c
001bb340 void SMSound_OpenMusicPackage(void);
001bb370 void SMSound_PlaySFX(int index);
001bb3a0 void SMSound_StopSFX(int index);
001bb3c0 void SMSound_PlayVoice(int kind, int player);
001bb410 void SMSound_StartMusic(void);
001bb480 void SMSound_ResumeMusicIfPaused(void);
001bb4b0 void SMSound_StopMusic(void);
001bb4c0 void SMSound_PauseMusic(void);
001bb4e0 void SMSound_ResumeMusic(void);
001bb500 void SMSound_Load(void);
001bb5f0 void SMSound_Free(void);

# ---- ScratchMatch/CScratchmatch_Camera.c
001bb630 void SMCamera_SetView(void);
001bb670 void SMCamera_LoadIntro(struct bkPackage *package);
001bb700 int SMCamera_UpdateIntro(void);
001bb790 void SMCamera_DestroyIntro(void);
001bb7a0 int SMCamera_RestartIntro(void);
001bb7b0 unsigned int SMCamera_Init(void);
001bb8a0 void SMCamera_MoveTowards(float *target, float rate, float pullBack);
001bba70 void SMCamera_FollowLeader(void);
001bbbd0 void SMCamera_Wander(void);

# ---- ScratchMatch/CScratchmatch_Game.c
001bbf90 void SMGame_RankPlayers(void);
001bc180 void SMGame_FreeSequence(void);
001bc1a0 void SMGame_TrimLine(char *line);
001bc200 int SMGame_ParseFirstNumber(const char *line);
001bc270 unsigned int SMGame_GetSequenceEntry(int player, int position);
001bc2b0 void SMGame_StoreResults(void);
001bc420 void SMGame_SetOthersNextEntry(unsigned int player, unsigned int buttons);
001bc480 void SMGame_NextState(void);
001bc640 void SMGame_UpdateIntro(void);
001bc6d0 void SMGame_UpdatePractice(void);
001bc790 void SMGame_UpdatePlay(void);
001bc960 void SMGame_ParseLine(int *out, const char *line);
001bca30 void SMGame_LoadSequence(void);
001bcc30 void SMGame_Init(void);
001bce70 void SMGame_Restart(void);
001bcf00 void SMGame_UpdateOutro(void);

# ---- ScratchMatch/ScratchMatch_GenActors.c
001bcfe0 void SMGenActors_Load(void);
001bd040 void SMGenActors_Free(void);
001bd080 void SMGenActors_DeleteInstances(void);
001bd0d0 void SMGenActors_Draw(void);
001bd110 void SMGenActors_CreateInstances(void);
001bd210 void SMGenActors_Update(void);

# ---- ScratchMatch/unplaced.c (timers)
001bd2e0 void SMTimer_InitAll(void);
001bd330 double SMTimer_Elapsed(int timer);
001bd380 void SMTimer_Reset(int timer);
001bd3a0 void SMTimer_UpdateFrame(void);
001bd400 double SMTimer_FrameTime(void);
001bd410 void SMTimer_Pause(void);
001bd420 void SMTimer_Resume(void);

# ---- ScratchMatch/CScratchmatch_Render.c
001bd480 void SMRender_DrawPlayers(void);
001bd5f0 void SMRender_DrawArena(void);
001bd700 void SMRender_Begin2D(void);
001bd750 void SMRender_LoadAnimTextures(void);
001bd7c0 void SMRender_FreeAnimTextures(void);
001bd810 void SMRender_UpdateAnimTextures(void);
001bd880 void SMRender_Init(void);
001bd980 void SMRender_Free(void);
001bd9e0 void SMRender_SetRotation(struct BActorInstance *instance, float x, float y, float z);
001bdc30 void SMRender_PlacePlayers(void);
001bdda0 void SMRender_Draw(void);

# ---- ScratchMatch/CScratchmatch_Lights.c
001bde50 void SMLights_Free(void);
001bdf00 void SMLights_CheckSpotLights(void);
001bdf40 void SMLights_DisableSpotLights(void);
001bdf90 void SMLights_CycleColours(void);
001bdfe0 void SMLights_EnableStaticLights(void);
001be020 void SMLights_DisableStaticLights(void);
001be060 void SMLights_DrawPods(int pass);
001be0c0 void SMLights_UpdateSpotLights(void);
001be1f0 void SMLights_PlacePods(void);
001be290 void SMLights_Init(void);
001be570 void SMLights_Update(void);

# ---- ScratchMatch/ScratchMatch_Effects.c
001be5d0 void SMEffects_Draw(void);
001be610 void SMEffects_LoadSmoke(void);
001be720 void SMEffects_FreeSmoke(void);
001be7a0 void SMEffects_Init(void);
001be840 void SMEffects_Free(void);
001be890 void SMEffects_UpdateSmoke(void);
001be910 void SMEffects_DrawSmoke(void);
001be9e0 void SMEffects_SetDarkenStage(int level);
001bea60 void SMEffects_ResetDarkenStage(void);
001bea90 void SMEffects_Update(void);

# ---- ScratchMatch/CScratchmatch_Player.c
001beb60 void SMPlayer_FreeAll(void);
001bebd0 void SMPlayer_UpdateAnimations(void);
001bec40 int SMPlayer_AnyButtonPressed(void);
001bec90 void SMPlayer_PlayWinAnimation(int player);
001bed00 void SMPlayer_ReadButtons(int player);
001bee20 int SMPlayer_ButtonsMatch(int player, unsigned int buttons);
001bee70 void SMPlayer_UpdateSequence(void);
001bf000 void SMPlayer_InitAll(void);
001bf5e0 void SMPlayer_UpdateIntroDrop(void);
001bf660 void SMPlayer_UpdateDances(void);
001bf7b0 unsigned int SMPlayer_DecideAIHits(void);
001bf990 int SMPlayer_TimingScore(float timing);
001bf9f0 void SMPlayer_AIHit(int player, unsigned int buttons, float timing);
001bfc40 void SMPlayer_UpdateHits(void);

# ---- ScratchMatch/CScratchmatch_Sprites.c
001bfff0 void SMSprites_Init(void);
001c00f0 void SMSprites_Load(void);
001c0140 void SMSprites_Free(void);
001c0160 void SMSprites_DrawFramerate(void);
001c0220 void SMSprites_StartPulse(int player);
001c0260 void SMSprites_AddText(int player, int type, unsigned int value);
001c03a0 void SMSprites_UpdateTexts(void);
001c0420 void SMSprites_AddIcon(int player, int buttons, float hitTime);
001c0560 void SMSprites_UpdateIcons(void);
001c06c0 void SMSprites_RemoveIcon(unsigned int icon);
001c06f0 unsigned int SMSprites_GetIconButtons(int icon);
001c0710 float SMSprites_GetIconHitTime(int icon);
001c0730 unsigned int SMSprites_GetPlayerIconButtons(int player);
001c0760 void SMSprites_RemovePlayerIcons(int player);
001c07a0 void SMSprites_SetPlayerIconButtons(int player, int buttons);
001c07f0 unsigned int SMSprites_GetPlayerIconSequence(int player);
001c0850 void SMSprites_CrossPreviousIcon(int player);
001c0890 int SMSprites_IsBonusIcon(unsigned int icon);
001c08e0 void SMSprites_DrawSprite(struct bdTexture *texture, float x, float y, unsigned int rgb, unsigned char alpha, float scaleX, float scaleY, float rotation);
001c0ba0 void SMSprites_Draw(void);
001c12b0 void SMSprites_UpdatePulses(void);
001c1320 void SMSprites_UpdatePlayerIcons(void);
001c14e0 int SMSprites_Update(void);

# ---- ScratchMatch/CScratchmatch_Scenics.c
001c1560 struct SMScenic *SMScenic_Ctor(struct SMScenic *self);
001c1570 void SMScenic_Dtor(struct SMScenic *self);
001c1590 void SMScenic_Load(struct SMScenic *self, const char *model);
001c1620 void SMScenic_Draw(struct SMScenic *self);
001c16c0 void SMScenic_Play(struct SMScenic *self);
001c16f0 void SMScenic_Update(struct SMScenic *self, float dt);
