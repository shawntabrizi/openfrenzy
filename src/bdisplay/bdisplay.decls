# bdisplay: true prototypes and names of the Babel BDISPLAY functions
# (docs/ARCHITECTURE.md section 11). Types: src/bdisplay/bdisplay_types.h.

# ---------- xbDrawprim.cpp
0003ecc0 int bdFVFStride(int fvf);
0003ef90 int bdVertexBufferGetSize(struct bdVertexBuffer *vb);
0003efa0 void bdSetVertexShaderAndStream(int fvf, struct bdVertexBuffer *vb);
0003f020 int bdClipSegmentNear(float *a, float *b);
0003f1f0 int bdIndexBufferCreate(struct bdIndexBuffer **out, int numIndices);
0003f270 void bdIndexBufferDestroy(struct bdIndexBuffer *ib);
0003f290 unsigned short *bdIndexBufferGetData(struct bdIndexBuffer *ib);
0003f2a0 void bdSetIndexBuffer(struct bdIndexBuffer *ib, int baseVertex);
0003f2d0 void bdDrawPrimitive(int type, void *vertices, int count);
0003f360 void bdDrawPrimitiveTL(int type, void *vertices, int count);
0003f3f0 void bdDrawPrimitiveFVF(int type, void *vertices, int count, int fvf);
0003f4a0 void bdDrawVertexBuffer(int type, struct bdVertexBuffer *vb, int start, int count);
0003f500 int bdVertexBufferCreate(struct bdVertexBuffer **out, int numVertices, int fvf, int flags, int numBuffers);
0003f650 void bdVertexBufferDestroy(struct bdVertexBuffer *vb);
0003f710 void *bdVertexBufferLock(struct bdVertexBuffer *vb, unsigned int flags);
0003f790 int bdVertexBufferUnlock(struct bdVertexBuffer *vb);
0003f7b0 int bdDrawSprite(float *pos, float width, float height, float angle, struct bdTexture *texture, int flipU, int flipV, int r, int g, int b, int a, float *vertices, float zBias);
0003faf0 int bdDrawBeam(float *pos1, float width1, float length1, int *colour1, float *pos2, float width2, float length2, int *colour2, struct bdTexture *texture, float *vertices, float zBias);
00040070 int bdDrawPolylineStrip(int numPoints, float *points, float *widths, int *colours, float *vertices, float zBias);
000406d0 void bdDrawPointSprites(struct bdVertexBuffer *vb, int start, int count, float pointSize);
00040750 int bdDrawSprite3D(float *pos, float width, float height, float angle, struct bdTexture *texture, int flipU, int flipV, int r, int g, int b, int a, float *vertices, float zBias);
000407d0 int bdDrawBeam3D(float *pos1, float width1, float length1, int *colour1, float *pos2, float width2, float length2, int *colour2, struct bdTexture *texture, float *vertices, float zBias);
00040880 int bdDrawPolyline3D(int numPoints, float *points, float *widths, float *screenWidths, int *colours, float *vertices, float zBias);
00040900 int bdDrawBeamClipped(float *pos1, float width1, float length1, int *colour1, float *pos2, float width2, float length2, int *colour2, struct bdTexture *texture, float *vertices, float zBias);
00040cd0 int bdDrawPolylineClipped(int numPoints, float *points, float *widths, float *screenWidths, int *colours, float *vertices, float zBias);

# ---------- xbTexture.cpp
00041520 void bdDeleteAllTextures(void);
00041530 void bdTextureDelete(struct bdTexture *texture);
00041580 void bdTextureSetFrame(struct bdTexture *texture, int frame);
000415b0 void bdTextureGetInfo(struct bdTexture *texture, unsigned int *width, unsigned int *height, unsigned int *format, unsigned int *numFrames, unsigned int *frame);
00041600 void bdTextureSetTime(struct bdTexture *texture, int time);
00041680 void bdTextureSetCallback(struct bdTexture *texture, int which, void *callback, void *context);
000416c0 int bdSetTexture(int stage, struct bdTexture *texture);
00041810 void bdClearTexture(int stage);
00041890 void bdTextureSetup(struct bdTexture *texture);
00041a10 struct bdTexture *bdLoadTexture(void *package, unsigned int crc);
00041a40 void *bdTextureLock(struct bdTexture *texture, int frame, unsigned int *width, unsigned int *height, int *pitch, unsigned int *format, unsigned long **palette);
00041ac0 void bdTextureUnlock(struct bdTexture *texture);
00041b10 struct bdTexture *bdCreateTexture(int width, int height, int unused, int numFrames, short flags, int format);

# ---------- camera, projection, render states, render targets (xbRender.cpp)
00043490 void bdSetCameraPosition(float x, float y, float z);
000434b0 void bdSetCameraAngles(float x, float y, float z, int order);
000435f0 void bdSetCameraLookAt(float *eye, float *target, float roll);
000436e0 void bdSetCameraOrbit(float *target, float pitch, float yaw, float distance);
00043790 void bdUpdateView(void);
000438b0 void bdSetWorldIdentity(void);
00043930 void bdSetWorldMatrix(float *world);
000439d0 void bdSetup2DProjection(void);
00043a90 void bdSetupProjection(void);
00043b70 void bdSetProjectionMode(int mode);
00043c10 void bdProjectPoints(float *out, float *in, int count);
00043cf0 void bdTransformPoints(float *out, float *in, int count);
00043d20 char bdBoxVisible(float *matrix, float x0, float x1, float y0, float y1, float z0, float z1);
00044320 int bdSphereVisible(float *pos, float radius);
000443f0 float bdProjectSize(float *pos, float width, float height, float *outWidth, float *outHeight);
000444b0 float bdProjectWidth(float *pos, float width, float *outWidth);
00044550 void bdSetDepthOffset(float offset);
000445e0 void bdSetClipWindow(int x, int y, int width, int height);
00044810 void bdSetNearFar(float nearZ, float farZ);
00044840 void bdSetViewport(int x, int y, int width, int height);
00044a00 void bdSetFieldOfView(float fovX, float fovY);
00044a70 void bdRenderTargetGetInfo(struct bdRenderTarget *target, int *width, int *height, int *colourDepth, int *zDepth);
00044ac0 int bdLockRenderState(int state);
00044ae0 int bdUnlockRenderState(int state);
00044b00 void bdSetRenderState(int state, unsigned int value, unsigned int extra);
00044e70 void bdSetTextureFactorAlpha(int alpha);
00044e90 struct bdRenderTarget *bdCreateRenderTarget(int width, int height, int colourDepth, int zDepth, unsigned int flags);
000455a0 void bdSetRenderTargetTexture(struct bdRenderTarget *target, int stage);

# ---------- xbDisplay.cpp
00041d70 int bdInit(void);
00041e40 void bdShutdown(void);
00041e90 void bdGetScreenInfo(int *width, int *height, int *colourBits, int *zBits);
00041ee0 void bdFlip(unsigned int r, unsigned int g, unsigned int b, int a);
00042020 int bStartDisplay(void);
000424e0 void bEndDisplay(void);
00042530 void bdCloseDisplay(void);
00042570 int bdBeginScene(void);
00042590 void bdEndScene(void);
000425a0 void *bdLockSurface(void *surface, int *width, int *height, int *bitsPerPixel, int *is555, int *pitch);
000426e0 int bSaveSurfaceAsBMP(void *surface, const char *fileName);
00042ba0 void bdScreenShot(const char *name);
00042be0 void bdBlankScreen(void);
00042c70 int bdOpenDisplay(int width, int height, int unused3, int unused4, int flags);

# ---------- xbFont.cpp
00042e10 void bdFontInit(void);
00042e40 void bdFontShutdown(void);
00042e60 struct bdFont *bdLoadFont(void *package, unsigned int crc);
00042ec0 void bdFontDelete(struct bdFont *font);
00042ef0 int bdGetTextExtent(float *rect, struct bdFont *font, const unsigned short *text, int length, struct bdPrintFormat *format, void *callback, void *context);
00043010 void bdPrintFontBox(struct bdFont *font, const unsigned short *text, int length, struct bdPrintFormat *format, int r, int g, int b, int a, void *callback, void *context);
00043140 void PrintFontLite(struct bdFont *font, const unsigned short *text, int length, float x, float y, int r, int g, int b, int a, void *callback, void *context);
00043400 void bdPrintFont(struct bdFont *font, const unsigned short *text, int length, struct bdPrintFormat *format, int r, int g, int b, int a, void *callback, void *context);
00043440 void bdPrintFontLite(struct bdFont *font, const unsigned short *text, int length, float x, float y, int r, int g, int b, int a, void *callback, void *context);

# ---------- render states and render targets (xbRender.cpp)
00044dd0 void bdSetFogRange(float start, float end);
00044e20 void bdSetFogColour(unsigned int r, unsigned int g, unsigned int b);
00045330 int bdSetRenderTarget(struct bdRenderTarget *target, unsigned int r, unsigned int g, unsigned int b, int a, float z, unsigned int flags);
00045660 void bdResetRenderStates(void);
000458a0 void bdDeleteRenderTarget(struct bdRenderTarget *target);

# ---------- unplaced
000459b0 int bPrintFontVerts(float *vertices, struct bdPrintLine *lines, struct bdFont *font, const unsigned short *text, int length, struct bdPrintFormat *format, int r, int g, int b, int a, void *callback, void *context);
00046320 void bdGetStringSize(struct bdFont *font, const unsigned short *text, int length, float *width, float *height);
00046c90 unsigned int bdGetRenderState(int state, unsigned int *extra);
00046cb0 void bdPushRenderState(int state, unsigned int value, unsigned int extra);
00046d00 void bdPopRenderState(int state);
00046d40 void bdSaveRenderStates(unsigned int *saved);
00046d60 void bdRestoreRenderStates(unsigned int *saved);
00046db0 struct bdTexture *bdLoadTexture2(void *package, unsigned int crc);
00046dc0 void bdTextureDelete2(struct bdTexture *texture);
00046dd0 void bdDrawLinearTexture(struct bdTexture *texture, int x, int y);
00047010 const char *bdGetErrorString(int result);
00047070 int FUN_00047070(void);
00047300 void FUN_00047300(void *callback);

# ---------- xbVideo.cpp (0x00046390 and 0x000463b0, Bink's allocator
# callbacks, are not in the function table: defined in BDISPLAY/xbVideo.c)
000463c0 int bdVideoPlayStream(const char *fileName, unsigned int flags, void *callback, void *context);
