# engine_misc: true prototypes of the functions this module defines (docs/ARCHITECTURE.md 11)
# <addr> <prototype>;

# BGUI (debug menu GUI)
00047370 struct BGuiMenu *bGuiCreateMenu(unsigned int flags);
000473f0 void bGuiDeleteMenu(struct BGuiMenu *menu);
00047460 void bGuiSetRootMenu(struct BGuiMenu *menu);
00047480 struct BGuiItem *bGuiAddItem(struct BGuiMenu *menu, const char *name, unsigned short id, struct BGuiMenu *submenu, unsigned short flags);
00047520 void bGuiDrawPopup(struct BGuiMenu *menu, int x, int y);
00047830 void bGuiDrawMenuBar(struct BGuiMenu *bar);
00047970 void bGuiProcessInput(void);
00047cb0 void bGuiUpdate(void);
00047d20 void bGuiRemoveItem(struct BGuiMenu *menu, struct BGuiItem *item);
00047d80 struct BGuiItem *bGuiGetItem(struct BGuiMenu *menu, int index);
00047da0 struct BGuiItem *bGuiFindItem(struct BGuiMenu *menu, unsigned int id);
00047de0 void bGuiShutdown(void);
00047e30 void bGuiBindDefaultKeys(void);
00047f30 void bGuiLockScreen(void);
00047f80 void bGuiUnlockScreen(void);
00047f90 void bGuiDrawIcon(int icon, int x, int y);
000480d0 void bGuiDrawText(struct BGuiFont *font, int x, int y, const char *text);
00048310 void bGuiFillRect(int x, int y, int w, int h, int r, unsigned int g, unsigned int b);
00048380 void bGuiDrawBox(int x, int y, int w, int h, int style, int r, unsigned int g, unsigned int b);
00048790 void bGuiDrawSeparator(int x, int y, int w, int r, int g, int b);

# BINPUT
00048880 void biInit(int param);
000488c0 struct BInputMap *biCreateMap(int maxChannels);

# BSOUND sample channels
000545f0 int bsInitChannels(void);
00054610 int bsGetNumChannels(void);
00054620 int bsCountPlaying(void);
00054660 void bsReserveChannel(int channel, int reserve);
00054690 void bsSetSfxVolume(int volume);
000546e0 void bsStopChannel(int channel);
00054730 void bsStopAll(void);
000547a0 void bsStopSample(struct BSample *sample);
00054800 void bsPauseAll(int pause);
000548a0 void bsSetChannelVolume(int channel, unsigned short volume);
000548d0 int bsGetChannelStatus(int channel);
00054900 void bsStopAllThunk(void);
00054910 void bsCalcVolumePan(float *pos, float minDist, float maxDist, int *pan, int *volume);
00054a30 int bsCheckPlayFlags(unsigned char flags, struct BSample *sample);
00054b50 int bsFindFreeChannel(unsigned int flags, struct BSample *sample);
00054ba0 int bsPlaySample(struct BSample *sample, int volume, int pan, int frequency, unsigned int flags);
00054c30 int bsPlaySample3D(struct BSample *sample, float *pos, int volume, int frequency, unsigned int flags, float minDist, float maxDist);
00054e00 int bsPlaySampleMoving(struct BSample *sample, float *pos, int volume, int frequency, unsigned int flags, void *callback, void *context, float minDist, float maxDist);
00054fe0 int bsPlaySampleOnChannel(int channel, struct BSample *sample, int volume, int pan, int frequency, unsigned int flags);
00055090 int bsPlaySampleMovingOnChannel(int channel, struct BSample *sample, float *pos, int volume, int frequency, unsigned int flags, void *callback, void *context, float minDist, float maxDist);
00055240 void bsUpdate(int arg);
00055390 void bsSampleFixup(int package, int name);
000553e0 void bsDeleteSample(struct BSample *sample);
00055400 void bsPrintCaps(void);
00055440 int bsInitDirectSound(void);
00055520 void bsShutdownDirectSound(void);
00055550 int bsCreateAndPlay(struct BSample *sample, int volume, int pan, int frequency, unsigned int flags, int channel);
00055680 void bsApplyVolume(int channel);
000556c0 void bsSetChannelFrequency(int channel, int frequency);
000556e0 void bsReleaseChannel(int channel);
00055720 void bsUpdateChannelStatus(int channel);
00055790 void bsPauseChannel(int channel, int pause);
000557d0 int bsCreateAndPlay3D(struct BSample *sample, float *pos, int volume, int frequency, unsigned int flags, int channel, float minDist, float maxDist);
00055940 void bsSetChannelPosition(int channel, float *prevPos, float *pos);
000559a0 void bsSetDistanceFactor(float factor);
000559c0 void bsUpdateListener(void);
