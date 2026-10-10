# bkernel: true prototypes and names of the decompiled functions (docs/ARCHITECTURE.md
# section 11). Only of_types.h / C types and "struct Name *".

# ---- xbDebug.cpp
0004b160 struct bkLogFile *bkCreateLogFile(struct bkLogFile *log, const char *name, uint flags);
0004b210 void bkSetLogFile(struct bkLogFile *log);
0004b230 void bkPrintf(const char *format, ...);
0004b380 void bkVPrintf(const char *format, __builtin_va_list arguments);
0004b450 void bkInitLog(void);
0004b540 void bkDeleteLogFile(struct bkLogFile *log);

# ---- heap
# (0x0004bc10, 0x0004be40, 0x0004bec0 took arguments in registers in the original)
0004bc10 void *bkAllocHeapMemory(void *memory, uint size);
0004be40 void bkHeapInsertFree(struct bkHeapBlock *block);
0004bec0 void *bkMallocFallback(uint size, const char *file, int line, ushort flags, const char *context);
0004bc80 void bkShutdownHeap(void);
0004bdc0 void bkResetHeap(void);
0004bf10 void *bkHeapAllocEx(uint size, const char *file, int line, uint flags, const char *context, uint align);
0004c110 void bkHeapFree(void *pointer);
0004c210 int bkHeapBlockSize(void *pointer);
0004c230 int bkHeapFreeSpace(int *largest);
0004c280 const char *bkGetHeapContext(void);
0004c290 int bkPushHeapContext(const char *context);
0004c2c0 int bkPopHeapContext(void);
0004c2e0 uint bkGetFreePhysicalMemory(void);
0004c300 void bkHeapDump(void);
0004c380 int bInitHeap(void *memory, uint size);
0004c420 void *bkHeapCallocEx(uint size, byte fill, const char *file, int line, uint flags, const char *context);
0004c480 void *bkHeapAlloc(uint size, const char *file, int line, uint flags);
0004c4b0 void *bkHeapCalloc(uint size, byte fill, const char *file, int line, uint flags);

# ---- timer
0004c510 void bInitTimer(void);
0004c5f0 ulonglong bkTimerRead(void);
0004c620 float bkTimerRate(longlong ticks);
0004c670 ulonglong bkTimerDelta(ulonglong start, ulonglong end);
0004c6a0 void bkSleep(uint milliseconds);

# ---- misc (profile timer, expiry check, date, printable text)
0004f890 void bkInitProfileTimer(struct bkProfileTimer *timer, int count);
0004f8b0 void bkStartProfileTimer(struct bkProfileTimer *timer);
0004f8c0 int bkStopProfileTimer(struct bkProfileTimer *timer);
0004f930 void bkCheckExpiry(void);
0004fa60 int bkGetTime(byte *time);
0004fab0 void bkMakePrintable(const byte *data, int length, char *text, int size);

# ---- files (file.cpp and its gap neighbours)
0004c6b0 void *bkLoadFile(struct bkPackage *package, const char *name, void *buffer, uint *size, uint *tags, int extra);
0004c710 int bkOpenFile(const char *name, struct _IO_FILE **file);
0004c740 void bkReadFile(struct _IO_FILE *file, void *buffer, uint size);
0004c760 void bkSeekFile(struct _IO_FILE *file, int offset, int origin);
0004c7a0 int bkCloseFile(struct _IO_FILE *file);
0004c7b0 int bkFileLength(struct _IO_FILE *file);
0004db50 void bkInitCRC(void);
0004dbf0 void bkDelayLoop(int unused, int count);
0004dc30 uint bkCRCString(const char *string);
0004dc70 uint bkCRCStringNoCase(const char *string);
0004dcc0 struct bkPackageEntry *bkGetPackageEntry(struct bkPackage *package, int index);
0004dce0 uint bkGetPackageId(struct bkPackage *package);
0004dcf0 void bkSetLanguage(int language);
0004dd00 int bkFindFile(const char *name, struct _IO_FILE **file, char *path, int pathSize);
0004dec0 int bkLoadFilenameTable(struct bkPackage *package, const char *packageName);
0004e010 int bkFreeFilenameTable(uint packageId);
0004e0c0 const char *bkGetFilename(uint packageId, uint crc);
0004e150 struct bkPackageEntry *bkFindPackageEntry(struct bkPackage *package, uint crc);
0004e1c0 struct bkPackageEntry *bkFindPackageEntryByName(struct bkPackage *package, const char *name);
0004e1f0 int bkGetFileSize(const char *name, uint flags);
0004e250 void *bkEnsureAlloc(int size, void *buffer);
0004e2f0 void *bkLoadSingleFile(const char *name, void *buffer, uint *size, int extra);
0004e390 void *bkLoadPackageFile(struct bkPackage *package, uint crc, void *buffer, uint *size, uint *tags, int extra);

# ---- background load (xbBkgload.cpp)
# (0x0004c7f0, 0x0004c9b0 and 0x0004d260 took arguments in registers in the original)
0004c7f0 int bkBkgPackageLoaded(struct bkBkgRequest *request, uint flags);
0004c8b0 void bkFinishRequest(int id);
0004c9b0 void *bkEnsureAllocBkg(void *buffer, int size);
0004ca20 int bkIsChannelBusy(int channel);
0004ca40 int bkIsChannelIdle(int channel);
0004cc10 int bkInitBackgroundLoad(void);
0004cd00 void bkKickScheduler(ulong unused);
0004ce60 int bkScheduleLoad(struct bkBkgRequest *request, void (*callback)(int), int argument);
0004cf90 int bkPauseBackgroundLoad(int pause);
0004cff0 int bQueueBackgroundLoad(int channel, void *buffer, void *handle, const char *fileName, uint offset, uint size, uint ioFlags, const char *eventName, uint flags);
0004d130 int bQueueBackgroundWrite(int channel, void *buffer, void *handle, uint offset, uint size, uint ioFlags, const char *eventName, uint flags);
0004d260 void *bkLoadSingleFileBackground(const char *path, uint *size, void *buffer, const char *eventName, uint flags);
0004d320 void *bkLoadPackageBackground(struct bkPackage *package, const char *name, const char *eventName, uint *size, void *buffer);

# ---- events (event.cpp)
0004d4d0 void bkInitEvents(void);
0004d4f0 int bkCreateEvent(const char *name);
0004d590 struct bkEventClient *bkAddEventCallback(const char *name, void (*callback)(const char *, const char *, void *), void *context);
0004d620 struct bkEventClient *bkAddEventQueue(const char *name, int size, uint flags);
0004d6d0 int bkGetEventMessage(struct bkEventClient *client, char *parameters);
# (0x0004d780 took the event in EBX)
0004d780 void bkDestroyEvent(struct bkEvent *event);
0004d7f0 void bkDeleteEvent(const char *name);
0004d860 void bkRemoveEventClient(struct bkEventClient *client);
0004d8b0 int bkGenerateEvent(const char *name, const char *parameters);
0004da50 void bkFreeEventClientsByName(const char *name);
0004dae0 void bkShutdownEvents(void);

# ---- kernel (xbKernel.cpp)
0004e480 int bkInitKernel(void);
0004e4b0 void bkShutdownKernel(void);
0004e520 void *bAddOSEvent(int (*callback)(int), int parameter, int extra);
0004e5a0 void bkRemoveOSEvent(void *handle);
0004e630 void bkPollOSEvents(void);
0004e690 void bkRun(void (*function)(int), int argument);
0004e6a0 void *bkCreateLock(void);
0004e6b0 int bkLock(void *lock);
0004e6d0 int bkUnlock(void *lock);
0004e6f0 int bkDestroyLock(void *lock);
0004e710 int bkParseCommandLine(const char *line, char **argv);
0004e800 int bkGetCommandLine(void);
0004e890 void bkFreeCommandLine(void);
0004e8f0 void bkNothing(void);
0004e900 void bkFlushWriteCombine(uint *memory, int count);
0004e960 void bkCacheDebugf(const char *format, ...);

# ---- resources
0004a2d0 struct bkStringTable *bkLoadStringTable(struct bkPackage *package, uint crc);
0004b570 void bkInitResources(void);
0004b5b0 void bkAddResource(struct bkResource *resource, struct bkPackage *package, int type, int group);
0004b620 void bkListResources(uint typeMask);
0004b6e0 void bkDeleteResource(struct bkResource *resource);
0004b720 struct bkResource *bkFindOrLoadResource(struct bkPackage *package, uint crc, int type, int group);
0004b7a0 struct bkResource *bkFindResource(int type, uint crc, uint packageId, uint group);
0004b820 struct bkResource *bkGetNextResource(uint type, struct bkResource *resource);
0004b870 int bLoadPackageResources(struct bkPackage *package, uint typeMask, int group, uint tag);
0004b9e0 void bkDeletePackageResources(uint packageId, uint typeMask);
0004ba60 void bkDeleteGroupResources(uint typeMask, uint group);
0004bae0 void bkDeleteAllResources(void);
0004bb30 struct bkResource *bLoadResource(struct bkPackage *package, const char *name, int type, int group);
00052040 void bkFreeDataResource(struct bkResource *resource);

# ---- file cache (xbCache.cpp)
0004d9c0 uint bkParseHex8(const char *text);
0004e980 void bkCacheAbortCopy(void);
0004ea30 void bkCacheDump(int force);
# (0x0004eaf0 took the time in EAX and the name in ECX)
0004eaf0 void bkCacheAddEntry(const struct _FILETIME *time, const char *name);
0004eb50 void bkCacheBuildFileList(void);
0004ecf0 char *bkCacheFileName(const char *source, int group);
0004ed20 void bkShutdownCache(void);
0004ed60 void bkCacheFlush(int everything);
0004ee90 int bkCacheFindFile(const char *source);
0004eed0 void bkCacheEnable(int enable);
0004ef40 void bkCacheStartCopy(void);
0004f210 void bkCacheUpdate(void);
0004f470 void bkCacheLoadingFileNotification(const char *fileName);
0004f510 int bkCacheOpenFile(const char *name, struct _IO_FILE **file, char *path);
0004f5e0 void bInitCache(void);
0004f710 int bkCacheFileListAdd(const char *source, int priority, int group);
0004f7a0 int bkInit(void *memory, uint size, int inputFlags);
0004f860 void bkShutdown(void);

# ---- input (bInput; input.cpp)
00048970 void biDeleteMap(struct biMap *map);
000489b0 void biDeleteAllMaps(void);
00048a10 struct biChannel *biCreateChannel(struct biMap *map, const char *name, int type, uint flags, int scale, int acceleration, int deceleration);
00048b00 void biDeleteChannel(struct biChannel *channel);
00048b20 int biBindInput(struct biChannel *channel, uint input, uint other);
00048c40 int biBindMultiInput(struct biChannel *channel, int count, ...);
00048d10 void biClearMap(struct biMap *map);
00048d40 void biClearChannel(struct biChannel *channel);
00048d60 int biUpdateInput(void);
00049380 int biAddRumble(int pad, int motor, int strength, int duration, int attack, int decay, int delay, int waveform);
00049490 void biAddRumbleList(int pad, int count, const int *effects);
000494e0 int biPauseRumble(void);
00049500 void biResumeRumble(void);
00049510 void biClearRumble(int pad);
00049550 void biShutdownInput(void);
00049560 void biUpdateRumble(void);
00049990 void biGetAttachedDevices(void);
00049ac0 void biCheckDeviceChanges(void);
00049d40 void bInitInput(void);
00049db0 void biCloseDevices(void);
00049df0 void biPollDevices(void);
00049e40 int biReadInput(uint input, int mode, uint *value);
0004a0d0 int biGetPadPort(int index);
0004a0f0 void biUpdateFeedback(void);
0004a220 int biSetMotor(int pad, int what, int speed, int other);
0004a290 void biStopMotors(int pad);

# ---- package.cpp
0004fb00 struct bkPackage *bkOpenPackage(const char *name);
0004fd50 struct bkPackage *bkLoadPackage(struct bkPackage *parent, const char *name, void *buffer);
0004fe80 void bkClosePackage(struct bkPackage *package);
0004fef0 int bkGetPackageSize(const char *name, uint flags);
0004db20 struct bkPackage *bkOpenPackageThunk(const char *name);
0004db30 struct bkPackage *bkLoadPackageThunk(struct bkPackage *parent, const char *name, void *buffer);
0004db40 void bkClosePackageThunk(struct bkPackage *package);

# ---- wide strings
0004a330 void bkStringToWide(ushort *wide, const char *string);
0004a360 ushort *bkWideFormatArgument(ushort *out, int type, int width, int precision, int hasPrecision, byte padChar, ushort exponentChar, char conversion, __builtin_va_list *arguments);
0004ae80 int bkWideSprintf(ushort *out, const ushort *format, ...);
0004b110 int bkWideStrlen(const ushort *string);
0004b130 void bkWideStrcpy(ushort *destination, const ushort *source);

# ---- maths (matrices, vectors, quaternions, trees)
0004ff30 void bmMatrixMultiply(float *out, const float *a, const float *b);
00050150 void bmMatrixMultiplyCopy(float *out, const float *a, const float *b);
00050250 void bmMatrixTranslation(float *m, float x, float y, float z);
000502a0 void bmMatrixScaling(float *m, float x, float y, float z);
000502f0 int bmMatrixInverseAffine(float *out, const float *m);
00050530 void bmTransformVector4InPlace(const float *m, float *v);
000505c0 void bmTransformVector4(float *out, const float *m, const float *v);
00050650 void bmMatrixTranspose(float *out, const float *m);
000506c0 void bmMatrixScaleTranslate(float *out, float x, float y, float z, const float *m, float sx, float sy, float sz);
00050750 void bmMatrixRotationX(float *m, float angle);
000507a0 void bmMatrixRotationY(float *m, float angle);
000507f0 void bmMatrixRotationZ(float *m, float angle);
00050840 void bmMatrixRotationXY(float *m, float x, float y);
000508c0 void bmMatrixRotationYX(float *m, float x, float y);
00050940 void bmMatrixRotationXZ(float *m, float x, float z);
00050a20 void bmMatrixRotationXYZ(float *m, float x, float y, float z);
00050af0 void bmMatrixRotationXZY(float *m, float x, float y, float z);
00050ba0 void bmMatrixRotationYXZ(float *m, float x, float y, float z);
00050c70 void bmMatrixRotationYZX(float *m, float x, float y, float z);
00050d20 void bmMatrixRotationZXY(float *m, float x, float y, float z);
00050dd0 void bmMatrixRotationZYX(float *m, float x, float y, float z);
00050e80 void bmMatrixLookAt(float *out, const float *from, const float *to, float roll);
00050f70 void bmSetRandomSeed(uint seed);
00050f80 float bmFastSqrt(float value);
00050fa0 void bmVectorLerp(float *out, const float *a, const float *b, float t);
00050ff0 void bmVectorBlend(float *out, const float *a, float s, const float *b, float t);
00051040 void bmTriangleNormal(float *out, const float *a, const float *b, const float *c);
00051110 void bmVectorRotateX(float *out, const float *v, float angle);
00051170 void bmVectorRotateY(float *out, const float *v, float angle);
000511d0 void bmVectorRotateZ(float *out, const float *v, float angle);
00051220 void bmVectorRotateXY(float *out, const float *v, float a, float b);
000512a0 void bmVectorRotateZY(float *out, const float *v, float a, float b);
00051310 void bmVectorRotateXYZ(float *out, const float *v, float a, float b, float c);
000513b0 void bmVectorRotateZXY(float *out, const float *v, float a, float b, float c);
00051470 void bmNothing1(void);
000514a0 void bmVectorNormalise(float *out, const float *v);
00051500 float bmVectorDistance(const float *a, const float *b);
00051540 void bmNothing2(void);
00051570 void bmVectorSetLength(float *out, const float *v, float length);
000515c0 void bmQuaternionToMatrix(float *m, const float *q);
00051700 void bmMatrixToQuaternion(float *q, const float *m);
00051860 void bmQuaternionSlerp(float *out, const float *a, const float *b, float t);
00051a80 void bmQuaternionSlerpLong(float *out, const float *a, const float *b, float t);
00051ca0 void bmQuaternionMultiply(float *out, const float *a, const float *b);
00051d30 void bmQuaternionToAxisAngle(float *out, const float *q);
00051df0 void bmAxisAngleToQuaternion(float *q, const float *axisAngle);
00051e30 void bmQuaternionRotationX(float *q, float angle);
00051e60 void bmQuaternionRotationY(float *q, float angle);
00051e90 void bmQuaternionRotationZ(float *q, float angle);
00051ec0 void bmQuaternionFromDirection(float *q, const float *direction, float roll);
00051f80 int bmCountTreeNodes(int *node);
00051fb0 void bmFreeTreeNode(int *node);
00051fe0 void bmRelocateTree(int base, int *node);
