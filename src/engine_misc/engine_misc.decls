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

# BSOUND play lists (xbPlayList.cpp)
00055ae0 void bsPlayListEnumSongs(int soundtrack, void *callback, struct EmSoundtrackData data, void *context);
00055b90 int bsPlayListEnum(void *callback, void *context);
00055c40 struct BPlayList *bsPlayListSelect(int soundtrack);
00055df0 void bsPlayListStop(struct BPlayList *list);
00055e30 void bsPlayListDelete(struct BPlayList *list);
00055eb0 int bsPlayListSetFlags(struct BPlayList *list, unsigned char flags);
00055f20 int bsPlayListClearFlags(struct BPlayList *list, unsigned char flags);
00055f90 unsigned short *bsPlayListGetSongName(struct BPlayList *list, int song);
00055fd0 unsigned int bsPlayListGetNumSongs(struct BPlayList *list);
00055fe0 unsigned short *bsPlayListGetName(struct BPlayList *list);
00055ff0 int bsPlayListPlay(struct BPlayList *list, int song, unsigned int flags, int volume, int pan, int frequency);
00056190 void bsPlayListUpdate(void);

# BSOUND audio streams (xbAudioStream.cpp)
00056270 int bsInitAudioStreams(void);
000562d0 void bsAudioStreamFixup(int package, int name);
00056340 void bsDeleteAudioStream(struct BAudioStream *stream);
00056370 int bsPlayAudioStream(struct BAudioStream *stream, unsigned int flags, int volume, int pan, int frequency, void *callback, void *file);
000563f0 void bsStopAudioStream(struct BAudioStream *stream);
00056420 int bsAudioStreamTestFlags(struct BAudioStream *stream, unsigned int mask);
00056440 void bsGetAudioStreamInfo(struct BAudioStream *stream, int *frequency, int *bits, int *formatFlags, int *blockTime, int *field8c, void **data90);
000564a0 int bsGetAudioStreamState(struct BAudioStream *stream, int *volume, int *pan, int *frequency);
000564e0 int bsSetAudioStreamVolume(struct BAudioStream *stream, int volume, int pan);
00056530 void bsSetMusicVolume(int volume);
00056590 int bsPauseAudioStream(struct BAudioStream *stream, unsigned char flags);
000565e0 int bsResumeAudioStream(struct BAudioStream *stream, unsigned char flags);

# BSOUND WMA streams (xbAudioStreamWMA.cpp)
00056640 void bsStreamDebug(const char *format, ...);
00056660 void bsDeleteAudioStreamWMA(struct BAudioStream *stream);
000566b0 void bsWmaReadBlock(struct BAudioStream *stream, int buf);
00056860 void bsStopAudioStreamWMA(struct BAudioStream *stream);
000569b0 int bsWmaCreateBuffer(struct BAudioStream *stream);
00056a70 int bsPlayAudioStreamWMA(struct BAudioStream *stream, unsigned int flags, int volume, int pan, int frequency, void *callback, void *file);
00056d20 void bsWmaDecode(struct BAudioStream *stream);
00057060 void bsWmaBlockLoaded(struct BAudioStream *stream);
00057160 void bsWmaUpdate(void);

# BSOUND start-up (unplaced)
000571c0 int bsInitSound(void);
000571f0 void bsShutdownSound(void);

# BSOUND ADPCM streams (xbAudioStreamADPCM.cpp)
00057200 void bsDeleteAudioStreamADPCM(struct BAudioStream *stream);
00057220 void bsAdpcmReadBlock(struct BAudioStream *stream);
000573e0 int bsAdpcmCreateBuffer(struct BAudioStream *stream);
000575f0 int bsPlayAudioStreamADPCM(struct BAudioStream *stream, unsigned int flags, int volume, int pan, int frequency, void *callback);
00057890 void bsStopAudioStreamADPCM(struct BAudioStream *stream);

# BMATHS animating collision (animatingCollision.cpp)
00052060 struct BCollNode *bmInitInstanceNodes(void *instances, struct BCollShape *shapes, struct BCollNode *nodes, struct BCollNode *parent);
00052130 struct BAnimatingCollision *bmCreateAnimatingCollision(struct BAnimatingCollision *coll, unsigned char *resource, unsigned char *actor);
000521c0 unsigned char *bmCollisionFixup(int package, int name);
00052200 int bmCollideNodes(struct BSphere *sphere, void *coll, struct BCollNode *nodes, float *delta, int maxHits, int *count, struct BAnimHit *hits);
00052510 void bmUpdateNodes(struct BCollNode *nodes);
00052590 int bmCollideAnimating(struct BSphere *sphere, struct BAnimatingCollision *coll, float *delta, int *count, struct BAnimHit *hits);
000526f0 void bmUpdateAnimatingCollision(struct BAnimatingCollision *coll);

# BMATHS collision tests (unplaced)
00052700 void bmCollMeshFixup(int package, int name);
00052770 int bmRaySphere(struct BRay *ray, struct BSphere *sphere, struct BCollHit *hit);
00052960 int bmRayMesh(struct BRay *ray, struct BCollMesh *mesh, int *count, struct BCollHit *hits);
00052ce0 int bmSphereSideOfPlane(struct BSphere *sphere, float *delta, float *plane);
00052da0 int bmSweepSphereSphere(struct BSphere *a, struct BSphere *b, float *dir, float length, struct BCollHit *hit);
00052ea0 void bmSegmentSphere(float *segment, struct BSphere *sphere, struct BCollHit *hit);
00052f30 int bmSweepSphereTriangle(struct BSphere *sphere, struct BTriangle *tri, float *delta, struct BCollHit *hit);
00053720 int bmSweepSphereMesh(struct BSphere *sphere, struct BCollMesh *mesh, float *delta, int *count, struct BCollHit *hits);
00053af0 void bmSweepSphereSphereVec(struct BSphere *a, struct BSphere *b, float *vec, struct BCollHit *hit);
00053b60 int bmSweepSphereEdge(struct BSphere *sphere, struct BEdge *edge, float *delta, struct BCollHit *hit);
00054050 int bmRayTriangle(struct BRay *ray, struct BTriangle *tri, struct BCollHit *hit);
00054420 int bmClosestLinePoints(float *a, float *b, float *s, float *t);
00054590 float bmProjectFactor(float *a, float *b);
000545d0 void bmVec2Divide(float *out, float *in, float s);
