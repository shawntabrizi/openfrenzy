/*
BDISPLAY_TYPES.H

Types of the Babel display layer (C:\Babel\Xbox\Src\BDISPLAY: xbDisplay.cpp,
xbDrawprim.cpp, xbTexture.cpp, xbFont.cpp, xbRender.cpp), recovered from the
XBE (0x0003ecc0-0x00047300). Offsets are the original (ILP32) ones and are
checked below. Other modules may include this header; only the bdisplay
module edits it.

It also declares the few library functions this module calls that the
shared XDK headers do not declare yet (see the end of the file).
*/

#ifndef OPENFRENZY_BDISPLAY_TYPES_H
#define OPENFRENZY_BDISPLAY_TYPES_H

#include "xtl.h"

/* ---------- vertex formats (FVF codes the engine passes to SetVertexShader) */

/* XYZRHW | DIFFUSE | TEX1: screen-space vertex, 0x1c bytes */
#define BD_FVF_TLVERTEX 0x144
/* XYZ | NORMAL | DIFFUSE | TEX1: 0x24 bytes; the default shader */
#define BD_FVF_VERTEX 0x152

typedef struct bdTLVertex
{
	float x, y, z, rhw; /* 0x00 */
	DWORD color;        /* 0x10 */
	float u, v;         /* 0x14 */
} bdTLVertex;
OF_ASSERT_SIZE(bdTLVertex, 0x1c);

typedef struct bdVertex
{
	float x, y, z;    /* 0x00 */
	float nx, ny, nz; /* 0x0c */
	DWORD color;      /* 0x18 */
	float u, v;       /* 0x1c */
} bdVertex;
OF_ASSERT_SIZE(bdVertex, 0x24);

/* Babel primitive types (index into DAT_00284c14, the D3DPRIMITIVETYPE of
each); BDPRIMTYPE_QUADLIST is drawn as one triangle strip per four vertices */
#define BDPRIMTYPE_POINTLIST 0
#define BDPRIMTYPE_LINELIST 1
#define BDPRIMTYPE_LINESTRIP 2
#define BDPRIMTYPE_TRILIST 3
#define BDPRIMTYPE_TRISTRIP 4
#define BDPRIMTYPE_TRIFAN 5
#define BDPRIMTYPE_QUADLIST 6

/* ---------- vertex buffers (xbDrawprim.cpp, "Vertex Buffer Header") */

#define BD_VB_MAX_BUFFERS 8

typedef struct bdVertexBuffer
{
	D3DVertexBuffer *buffer[BD_VB_MAX_BUFFERS]; /* 0x00 one per rotating copy */
	int numVertices;                            /* 0x20 */
	int stride;                                 /* 0x24 from the FVF */
	int flags;                                  /* 0x28 */
	int fvf;                                    /* 0x2c */
	struct bdVertexBuffer *prev;                /* 0x30 list of all vertex buffers */
	struct bdVertexBuffer *next;                /* 0x34 */
	int numBuffers;                             /* 0x38 */
	int current;                                /* 0x3c buffer the next lock uses */
	int drawBuffer;                             /* 0x40 buffer the stream source uses */
	void *data;                                 /* 0x44 one block for all copies */
	void *locked;                               /* 0x48 last lock */
	D3DVertexBuffer headers[1];                 /* 0x4c numBuffers of them */
} bdVertexBuffer;
OF_ASSERT_OFFSET(bdVertexBuffer, numVertices, 0x20);
OF_ASSERT_OFFSET(bdVertexBuffer, prev, 0x30);
OF_ASSERT_OFFSET(bdVertexBuffer, data, 0x44);
OF_ASSERT_OFFSET(bdVertexBuffer, headers, 0x4c);

/* the list head (0x002bcdc8) only uses the fields up to next */
typedef struct bdVertexBufferList
{
	BYTE unused[0x30];
	bdVertexBuffer *prev; /* 0x30 */
	bdVertexBuffer *next; /* 0x34 */
	BYTE unused2[0x18];
} bdVertexBufferList;
OF_ASSERT_SIZE(bdVertexBufferList, 0x50);

/* ---------- index buffers (xbDrawprim.cpp, "Index Buffer") */

typedef struct bdIndexBuffer
{
	int numIndices;              /* 0x00 */
	int usage;                   /* 0x04 always 0x10 */
	int format;                  /* 0x08 always 0x65 (D3DFMT_INDEX16) */
	struct bdIndexBuffer *prev;  /* 0x0c list of all index buffers */
	struct bdIndexBuffer *next;  /* 0x10 */
	D3DIndexBuffer d3d;          /* 0x14 */
	WORD indices[2];             /* 0x20 numIndices of them */
} bdIndexBuffer;
OF_ASSERT_OFFSET(bdIndexBuffer, d3d, 0x14);
OF_ASSERT_OFFSET(bdIndexBuffer, indices, 0x20);

/* ---------- textures (xbTexture.cpp, "Texture"; also loaded from packages) */

/* Babel texture formats (bdTexture.format) and their D3D formats */
#define BDTEXFMT_ARGB8888 1   /* D3DFMT_A8R8G8B8 (6), 32 bpp, swizzled */
#define BDTEXFMT_ARGB1555 3   /* D3DFMT_A1R5G5B5 (2), 16 bpp, swizzled */
#define BDTEXFMT_PAL8 5       /* D3DFMT_P8 (0xb), 8 bpp, 256-entry palette */
#define BDTEXFMT_ARGB4444 9   /* D3DFMT_A4R4G4B4 (4), 16 bpp, swizzled */
#define BDTEXFMT_LIN_ARGB8888 12 /* D3DFMT_LIN_A8R8G8B8 (0x12), 32 bpp */
#define BDTEXFMT_LIN_RGB565 13   /* D3DFMT_LIN_R5G6B5 (0x11), 16 bpp */

/* one animation frame: the D3D texture header and how long it shows */
typedef struct bdTextureFrame
{
	D3DTexture d3d; /* 0x00 */
	int time;       /* 0x14 */
} bdTextureFrame;
OF_ASSERT_SIZE(bdTextureFrame, 0x18);

struct bdTexture;
/* select / deselect callbacks (0x4c, 0x54): (texture, stage, context);
select returns 0 to refuse the texture */
typedef int (*bdTextureCallback)(struct bdTexture *texture, int stage, void *context);

typedef struct bdTexture
{
	BYTE resource[0x20];          /* 0x00 bKernel resource header */
	D3DTexture *current;          /* 0x20 frame shown now */
	bdTextureFrame *frames;       /* 0x24 numFrames of them */
	int numTexels;                /* 0x28 width * height */
	WORD format;                  /* 0x2c BDTEXFMT_* */
	WORD width;                   /* 0x2e */
	WORD height;                  /* 0x30 */
	WORD flags;                   /* 0x32 */
	WORD frame;                   /* 0x34 index of current */
	BYTE levels;                  /* 0x36 */
	BYTE numFrames;               /* 0x37 */
	int totalTime;                /* 0x38 sum of the frame times */
	BYTE unknown3c;               /* 0x3c */
	BYTE bitsPerPixel;            /* 0x3d */
	WORD unknown3e;               /* 0x3e */
	D3DPalette palette;           /* 0x40 BDTEXFMT_PAL8 only */
	bdTextureCallback select;     /* 0x4c */
	void *selectContext;          /* 0x50 */
	bdTextureCallback deselect;   /* 0x54 */
	void *deselectContext;        /* 0x58 */
	void *lockedBits;             /* 0x5c bdTextureLock */
	void *lockedPalette;          /* 0x60 */
	BYTE unknown64[0x1c];         /* 0x64 */
} bdTexture;                    /* 0x80, then the frames and the texel data */
OF_ASSERT_OFFSET(bdTexture, current, 0x20);
OF_ASSERT_OFFSET(bdTexture, format, 0x2c);
OF_ASSERT_OFFSET(bdTexture, numFrames, 0x37);
OF_ASSERT_OFFSET(bdTexture, bitsPerPixel, 0x3d);
OF_ASSERT_OFFSET(bdTexture, palette, 0x40);
OF_ASSERT_OFFSET(bdTexture, select, 0x4c);
OF_ASSERT_OFFSET(bdTexture, lockedPalette, 0x60);
OF_ASSERT_SIZE(bdTexture, 0x80);

/* ---------- render targets (xbRender.cpp, "Render Target Header") */

/* bdCreateRenderTarget flags (0xb0) */
#define BDRT_PRIVATEZ 1     /* own colour depth and Z buffer */
#define BDRT_NOWRITECOMBINE 8

typedef struct bdRenderTarget
{
	/* 0x00: laid out as a bdTexture, so a render target can be selected
	as a texture (current, frames, width, height are used) */
	bdTexture texture;              /* 0x00 */
	bdTextureFrame frame;           /* 0x80 copy of colour */
	struct bdRenderTarget *prev;    /* 0x98 list of all render targets */
	struct bdRenderTarget *next;    /* 0x9c */
	int width;                      /* 0xa0 */
	int height;                     /* 0xa4 */
	int colourDepth;                /* 0xa8 bits */
	int zDepth;                     /* 0xac bits, 0: the screen's Z buffer */
	int flags;                      /* 0xb0 BDRT_* */
	int viewport[4];                /* 0xb4 x, y, width, height */
	int clip[4];                    /* 0xc4 x, y, width, height */
	void *colourMemory;             /* 0xd4 */
	void *zMemory;                  /* 0xd8 */
	int memorySize;                 /* 0xdc of each */
	D3DTexture colour;              /* 0xe0 */
	D3DSurface zSurface;            /* 0xf4 */
	D3DSurface *colourSurface;      /* 0x10c */
} bdRenderTarget;
OF_ASSERT_OFFSET(bdRenderTarget, frame, 0x80);
OF_ASSERT_OFFSET(bdRenderTarget, prev, 0x98);
OF_ASSERT_OFFSET(bdRenderTarget, width, 0xa0);
OF_ASSERT_OFFSET(bdRenderTarget, viewport, 0xb4);
OF_ASSERT_OFFSET(bdRenderTarget, clip, 0xc4);
OF_ASSERT_OFFSET(bdRenderTarget, colourMemory, 0xd4);
OF_ASSERT_OFFSET(bdRenderTarget, colour, 0xe0);
OF_ASSERT_OFFSET(bdRenderTarget, zSurface, 0xf4);
OF_ASSERT_OFFSET(bdRenderTarget, colourSurface, 0x10c);
OF_ASSERT_SIZE(bdRenderTarget, 0x110);

/* ---------- fonts (xbFont.cpp; loaded from packages) */

typedef struct bdGlyph
{
	short u, v;          /* texel position in the font texture */
	short width, height; /* width 0: glyph not present */
} bdGlyph;

#define BDFONT_FIRST_CHAR 0x20
#define BDFONT_NUM_GLYPHS 0xe0

typedef struct bdFont
{
	BYTE resource[0x20];               /* 0x000 bKernel resource header */
	DWORD unknown20;                   /* 0x020 */
	bdTexture *texture;                /* 0x024 */
	float height;                      /* 0x028 line height */
	DWORD unknown2c;                   /* 0x02c */
	bdGlyph glyphs[BDFONT_NUM_GLYPHS]; /* 0x030 characters 0x20-0xff */
	BYTE unknown730[0x50];             /* 0x730 */
	bdTexture fontTexture;             /* 0x780 the texture, embedded */
} bdFont;
OF_ASSERT_OFFSET(bdFont, texture, 0x24);
OF_ASSERT_OFFSET(bdFont, glyphs, 0x30);
OF_ASSERT_OFFSET(bdFont, fontTexture, 0x780);

/* the glyph used for a character the font does not have (0x728, the last
glyph: character 0xff) */
#define BDFONT_MISSING_GLYPH (BDFONT_NUM_GLYPHS - 1)

/* one laid-out character of bPrintFontVerts (0x002bef00), 0x30 bytes */
typedef struct bdPrintChar
{
	void *texture;  /* 0x00 bdTexture of the glyph or inline picture */
	float advance;  /* 0x04 */
	float height;   /* 0x08 */
	float u[4];     /* 0x0c */
	float v[4];     /* 0x1c */
	int code;       /* 0x2c character - 0x20; -1 inline picture, -2 new line */
} bdPrintChar;
OF_ASSERT_SIZE(bdPrintChar, 0x30);

#define BD_PRINT_CODE_PICTURE (-1)
#define BD_PRINT_CODE_NEWLINE (-2)

/* one line of bPrintFontVerts (the caller's array), 0x10 bytes */
typedef struct bdPrintLine
{
	bdPrintChar *chars; /* 0x00 */
	int numChars;       /* 0x04 */
	float width;        /* 0x08 */
	float height;       /* 0x0c */
} bdPrintLine;
OF_ASSERT_SIZE(bdPrintLine, 0x10);

/* the box a string is printed in (bdPrintFont's format argument) */
typedef struct bdPrintFormat
{
	float x, y;          /* 0x00 */
	float width, height; /* 0x08 */
	int hAlign;          /* 0x10 0 left, 2 centre, 3 right, 5 justified */
	int vAlign;          /* 0x14 1 top, 2 centre, 4 bottom */
	int wrap;            /* 0x18 */
	float lineSpacing;   /* 0x1c */
} bdPrintFormat;
OF_ASSERT_SIZE(bdPrintFormat, 0x20);

/* what a print callback gets: a block on the caller's stack (bPrintFontVerts
0x00045d.., PrintFontLite). reason 0: after each character's four vertices
(current, index, line); 1: "@(text)" (record, text; nonzero keeps the
record); 2: "@[name]" inline picture (record, text; 0 drops the record).
PrintFontLite sets only vertices, current, line (0), index and context. */
typedef struct bdPrintCallbackInfo
{
	float *vertices;           /* 0x00 start of the vertex block */
	float *current;            /* 0x04 the character's vertices */
	struct bdPrintChar *chars; /* 0x08 DAT_002bef00 */
	struct bdPrintChar *record; /* 0x0c */
	char *text;                /* 0x10 between the brackets */
	int line;                  /* 0x14 */
	int index;                 /* 0x18 character in the line */
	void *context;             /* 0x1c */
} bdPrintCallbackInfo;
OF_ASSERT_SIZE(bdPrintCallbackInfo, 0x20);
typedef int (*bdPrintCallback)(int reason, bdPrintCallbackInfo *info);

/* ---------- library functions the shared XDK headers do not declare yet.
The d3d8 module owns them (its untracked src/d3d8/d3d8_extra.h has the same
declarations; game code cannot include it yet). Weak fallbacks for the link
are in bdisplay_xdk_fallback.c. */

#ifndef OPENFRENZY_D3D8_EXTRA_H
typedef struct _D3DCOLORVALUE
{
	float r, g, b, a;
} D3DCOLORVALUE;

typedef struct _D3DMATERIAL8
{
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Ambient;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Emissive;
	float Power;
} D3DMATERIAL8;

/* 0x001f54a0 */ void WINAPI D3DDevice_SetMaterial(CONST D3DMATERIAL8 *material);
/* 0x00201390 */ void WINAPI D3DDevice_DrawVerticesUP(D3DPRIMITIVETYPE primitive_type, UINT vertex_count,
	CONST void *vertex_data, UINT vertex_stride);
/* 0x002022f0 (the XBE's symbol: D3D_GetDeviceCaps) */
HRESULT WINAPI Direct3D_GetDeviceCaps(UINT adapter, D3DDEVTYPE device_type, D3DCAPS8 *caps);
/* 0x002285c1 */
VOID WINAPI XGSetTextureHeader(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
	D3DPOOL pool, D3DTexture *texture, UINT data, UINT pitch);
#endif
OF_ASSERT_SIZE(D3DMATERIAL8, 0x44);

/* XPhysicalProtect is the XBE's 0x001e8823; xdk_pdb.h declares it */

/* RAD Bink (the XBE's bink region, replaced by src/dsound/bink_null.c) */
typedef struct BINK *HBINK;
/* 0x0005a090 (XbSymbolDatabase calls it D3D_SetPushBufferSize; it stores the
two allocator callbacks) */
void __stdcall RADSetMemory(void *(__stdcall *allocate)(unsigned long size), void (__stdcall *release)(void *memory));
HBINK __stdcall BinkOpen(const char *name, unsigned long flags);
void __stdcall BinkClose(HBINK bink);
long __stdcall BinkDoFrame(HBINK bink);
void __stdcall BinkNextFrame(HBINK bink);
long __stdcall BinkWait(HBINK bink);
long __stdcall BinkCopyToBuffer(HBINK bink, void *destination, long destination_pitch,
	unsigned long destination_height, unsigned long x, unsigned long y, unsigned long flags);
void __stdcall BinkGetSummary(HBINK bink, void *summary);

#endif
