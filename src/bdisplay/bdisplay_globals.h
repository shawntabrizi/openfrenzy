/*
BDISPLAY_GLOBALS.H

The data of the Babel display layer, by its Ghidra name (the symbol other
modules use too), with the original address. Defined in
BDISPLAY/xbData.c; uninitialised ones are tentative definitions, so another
module's tentative definition of the same name merges with them (-fcommon).

The original code often clears or copies a run of these globals in one loop.
The port clears each symbol instead (see the functions that say so).

Other modules: include this header instead of declaring these names, and
use the alias macros at the end for addresses inside an array (for example
DAT_002bc4f0 is DAT_002bc490[24]): a separate symbol at such an address would
be a different variable.
*/

#ifndef OPENFRENZY_BDISPLAY_GLOBALS_H
#define OPENFRENZY_BDISPLAY_GLOBALS_H

#include <stdint.h>

struct bdVertexBuffer;
struct bdRenderTarget;
struct bdTexture;
struct bdPrintChar;
struct bdTLVertex;
struct bdIndexBuffer;
struct bdVertexBufferList;

/* ---------- initialised data (.data 0x00284c10-0x00285630) */

/* 0x00284c10 vertex buffer set as stream 0; 1 (no buffer yet) at start */
extern struct bdVertexBuffer *DAT_00284c10;
/* 0x00284c14 Babel primitive type -> D3DPRIMITIVETYPE (entry 6, quads, is drawn as strips) */
extern uint32_t DAT_00284c14[7];
/* 0x00284c30 identity: the view matrix of the 2D projection */
extern float DAT_00284c30[16];
/* 0x00284c70 the projection matrix in use: DAT_002be2b0 (3D) or DAT_002be2f0 (2D) */
extern float *PTR_DAT_00284c70;
/* 0x00284c78 blend modes: { SRCBLEND, DESTBLEND, BLENDOP } */
extern uint32_t DAT_00284c78[7][3];
/* 0x00284ccc Z compare functions */
extern uint32_t DAT_00284ccc[8];
/* 0x00284cec cull modes */
extern uint32_t DAT_00284cec[3];
/* 0x00284cf8 texture filters: { MAGFILTER, MINFILTER, MIPFILTER } */
extern uint32_t DAT_00284cf8[6][3];
/* 0x00284d40 1 until the render-target clear quad is set up */
extern int DAT_00284d40;
/* 0x00284d48 D3D/DirectSound error names */
typedef struct bdErrorName
{
	int32_t code;
	const char *name;
} bdErrorName;
extern bdErrorName DAT_00284d48[27];
/* 0x00284e20 number of DAT_00284d48 entries */
extern int DAT_00284e20;
/* 0x002855f0 identity matrix */
extern float DAT_002855f0[16];

/* ---------- display state (.bss; 0x002bc408-0x002bc48c are cleared together) */

extern int DAT_002bc3ec;      /* 0x002bc3ec device's MaxActiveLights */
extern uint32_t DAT_002bc400; /* 0x002bc400 vertex shader (FVF) set now; 1 at start */
extern int DAT_002bc408;      /* 0x002bc408 display flags (bdOpenDisplay); bit 0: open */
extern int DAT_002bc40c;      /* 0x002bc40c screen width */
extern int DAT_002bc410;      /* 0x002bc410 screen height */
extern int DAT_002bc414;      /* 0x002bc414 screen width / 2 */
extern int DAT_002bc418;      /* 0x002bc418 screen height / 2 */
extern int DAT_002bc41c;      /* 0x002bc41c colour bits (32) */
extern int DAT_002bc420;      /* 0x002bc420 Z bits (24) */
extern int DAT_002bc424;      /* 0x002bc424 stencil bits (8) */
extern float DAT_002bc428;    /* 0x002bc428 0.5 */
extern float DAT_002bc42c;    /* 0x002bc42c height / width / 2 */
extern int DAT_002bc430;      /* 0x002bc430 1 */
extern int DAT_002bc434;      /* 0x002bc434 1 */
extern int DAT_002bc438;      /* 0x002bc438 device has vertex shaders */
extern int DAT_002bc43c;      /* 0x002bc43c 3 */
extern int DAT_002bc440;      /* 0x002bc440 device started */
extern int DAT_002bc444[4];   /* 0x002bc444 texture stage has a texture matrix */
extern struct bdRenderTarget *DAT_002bc464; /* 0x002bc464 render target in use */
extern int DAT_002bc468;      /* 0x002bc468 inside a bdBeginScene */
extern int DAT_002bc46c;      /* 0x002bc46c multisample x */
extern int DAT_002bc470;      /* 0x002bc470 multisample y */
extern uint32_t DAT_002bc474; /* 0x002bc474 D3DMULTISAMPLE_* */
extern void *DAT_002bc478;    /* 0x002bc478 Direct3D object */
extern void *DAT_002bc47c;    /* 0x002bc47c D3D device */
extern void *DAT_002bc480;    /* 0x002bc480 back buffer surface */
extern void *DAT_002bc484;    /* 0x002bc484 depth/stencil surface */
extern void *DAT_002bc488;    /* 0x002bc488 front buffer surface */
extern int DAT_002bc48c;      /* 0x002bc48c */

/* colour channel tables: index 0-255 -> the channel's bits of a D3DCOLOR */
extern unsigned int DAT_002bc490[256]; /* 0x002bc490 green << 8 */
extern int DAT_002bc890;               /* 0x002bc890 frames presented */
extern unsigned int DAT_002bc898[256]; /* 0x002bc898 alpha << 24 */
extern unsigned int DAT_002bce18[256]; /* 0x002bce18 red << 16 */
extern unsigned int DAT_002bd640[256]; /* 0x002bd640 blue */
extern float DAT_002bd220[256];        /* 0x002bd220 i * 2 / 255, clamped to 1 */
extern int DAT_002bda40[256];          /* 0x002bda40 i */
extern int DAT_002bde40[256];          /* 0x002bde40 i */

/* 0x002bcc98 the screen as a render target (list head of all render targets) */
extern struct bdRenderTarget DAT_002bcc98;
/* 0x002bcda8 list head of all index buffers (prev 0x002bcdb4, next 0x002bcdb8) */
extern struct bdIndexBuffer DAT_002bcda8;
/* 0x002bcdc8 list head of all vertex buffers (prev 0x002bcdf8, next 0x002bcdfc) */
extern struct bdVertexBufferList DAT_002bcdc8;

extern unsigned long long DAT_002bd218; /* 0x002bd218 time of the last clear */
extern uint32_t DAT_002bd620[8];        /* 0x002bd620 frame-rate timer (bKernel) */
extern unsigned long long DAT_002be240; /* 0x002be240 last frame time */
extern float DAT_002be248;              /* 0x002be248 frames per second */
extern int DAT_002be25c;                /* 0x002be25c */
extern float DAT_002be260;              /* 0x002be260 dummy present time, ms */
extern void *DAT_002be264;              /* 0x002be264 font vertex ring (0x1000 vertices) */
extern int DAT_002be268;                /* 0x002be268 next free vertex of the ring */

/* ---------- camera and matrices (0x002be330-0x002be5c8 are cleared together) */

extern float DAT_002be270[16]; /* 0x002be270 view (camera rotation * translation) */
extern float DAT_002be2b0[16]; /* 0x002be2b0 perspective projection */
extern float DAT_002be2f0[16]; /* 0x002be2f0 2D projection */
extern float DAT_002be330[16]; /* 0x002be330 camera rotation */
extern float DAT_002be370[16]; /* 0x002be370 view */
extern float DAT_002be3b0[16]; /* 0x002be3b0 view * projection */
extern float DAT_002be3f0[16]; /* 0x002be3f0 world */
extern float DAT_002be430[16]; /* 0x002be430 world * view */
extern float DAT_002be470[16]; /* 0x002be470 world * view * projection */
extern float DAT_002be4b0[16]; /* 0x002be4b0 world * view * projection * viewport */
extern float DAT_002be4f0[16]; /* 0x002be4f0 viewport (clip space to pixels) */
extern float DAT_002be530[16]; /* 0x002be530 clip window (viewport inside the render target) */
extern float DAT_002be570[3];  /* 0x002be570 camera position */
extern int DAT_002be57c;       /* 0x002be57c projection: 1 perspective, 0 2D */
extern float DAT_002be580;     /* 0x002be580 near plane */
extern float DAT_002be584;     /* 0x002be584 far plane */
extern float DAT_002be588;     /* 0x002be588 horizontal field of view */
extern float DAT_002be58c;     /* 0x002be58c vertical field of view */
extern float DAT_002be590;     /* 0x002be590 viewport width */
extern float DAT_002be594;     /* 0x002be594 viewport height */
extern float DAT_002be598;     /* 0x002be598 viewport width / 2 */
extern float DAT_002be59c;     /* 0x002be59c viewport height / 2 */
extern float DAT_002be5a0;     /* 0x002be5a0 viewport centre x */
extern float DAT_002be5a4;     /* 0x002be5a4 viewport centre y */
extern float DAT_002be5a8;     /* 0x002be5a8 viewport x */
extern float DAT_002be5ac;     /* 0x002be5ac viewport y */
extern float DAT_002be5b0;     /* 0x002be5b0 depth offset of the projections */
extern int DAT_002be5c4;       /* 0x002be5c4 DAT_002be4b0 is up to date */
extern int DAT_002be5c8;       /* 0x002be5c8 DAT_002be3b0 is up to date */
extern int DAT_002be5cc;       /* 0x002be5cc world matrix is the identity */

/* ---------- render state (0x002be5d0-0x002bee90 are cleared together) */

#define BD_NUM_STATES 15
#define BD_STATE_STACK 16

extern int DAT_002be5d0;                 /* 0x002be5d0 */
extern struct bdTexture *DAT_002be5d4[4]; /* 0x002be5d4 texture per stage */
extern float DAT_002be5e4;               /* 0x002be5e4 fog start */
extern float DAT_002be5e8;               /* 0x002be5e8 fog end */
extern float DAT_002be5ec;               /* 0x002be5ec 1 / (fog end - fog start) */
extern uint32_t DAT_002be5f0;            /* 0x002be5f0 fog red */
extern uint32_t DAT_002be5f4;            /* 0x002be5f4 fog green */
extern uint32_t DAT_002be5f8;            /* 0x002be5f8 fog blue */
/* 0x002be600 bdSetRenderState values: [state * 2] value, [state * 2 + 1] extra */
extern uint32_t DAT_002be600[BD_NUM_STATES * 2];
/* 0x002be678 bdPushRenderState stacks: [(state * 16 + depth) * 2 (+ 1)] */
extern uint32_t DAT_002be678[BD_NUM_STATES * BD_STATE_STACK * 2];
extern int DAT_002bedf8[BD_NUM_STATES];  /* 0x002bedf8 stack depth per state */
extern int DAT_002bee34[4];              /* 0x002bee34 texture stage enabled */
extern int DAT_002bee44[BD_NUM_STATES];  /* 0x002bee44 state locked (bdLockRenderState) */
extern int DAT_002bee80;                 /* 0x002bee80 (bactor, engine_misc) */
extern int DAT_002bee84;                 /* 0x002bee84 (bactor) */
extern struct bdTLVertex DAT_002bee90[4]; /* 0x002bee90 render-target clear quad */

/* 0x002bef00 bPrintFontVerts's characters */
extern struct bdPrintChar DAT_002bef00[0x400];

/* ---------- the video player's and GUI's data */

/* 0x002caf08-0x002caf10 objects told to end the scene (bdEndScene calls
their first virtual function); only written outside this module */
extern void **DAT_002caf08;
extern void **DAT_002caf0c;
extern void **DAT_002caf10;

/* the BGUI block (owner engine_misc, src/engine_misc/engine_misc_gui.h:
one block 0x002caf18-0x002cb56c with a symbol at each of these addresses) */
struct BInputMap;
struct BInputChannel;
struct BGuiMenu;
struct BGuiEvent;
extern int DAT_002caf18, DAT_002caf1c, DAT_002caf20, DAT_002caf24, DAT_002caf28, DAT_002caf2c;
extern int DAT_002caf30, DAT_002caf34, DAT_002caf38, DAT_002caf3c;
extern int DAT_002caf40, DAT_002caf44, DAT_002caf48, DAT_002caf4c, DAT_002caf50, DAT_002caf54;
extern int DAT_002caf58, DAT_002caf5c, DAT_002caf60, DAT_002caf64, DAT_002caf68, DAT_002caf6c;
extern int DAT_002caf70, DAT_002caf74, DAT_002caf78;
extern struct BInputMap *DAT_002caf7c;
extern struct BInputChannel *DAT_002caf80, *DAT_002caf84, *DAT_002caf88, *DAT_002caf8c;
extern struct BInputChannel *DAT_002caf90, *DAT_002caf94, *DAT_002caf98, *DAT_002caf9c;
extern struct BInputChannel *DAT_002cafa0;
extern struct BGuiMenu DAT_002cafa4;
extern struct BGuiMenu *DAT_002cb53c, *DAT_002cb540, *DAT_002cb548, *DAT_002cb54c;
extern int DAT_002cb550, DAT_002cb554;
extern int (*DAT_002cb558)(struct BGuiEvent *event);
/* 0x002cc1b4 input map the GUI or the video player owns (engine_misc) */
extern struct BInputMap *DAT_002cc1b4;
/* 0x002d0758 root folder of relative names (bkernel) */
extern char DAT_002d0758[0x100];

/* ---------- aliases: addresses inside the arrays above that other modules use */

#define DAT_00284d2c (DAT_00284cf8[4][1])
#define DAT_002bc4f0 (DAT_002bc490[24])
#define DAT_002bc538 (DAT_002bc490[42])
#define DAT_002bc558 (DAT_002bc490[50])
#define DAT_002bc590 (DAT_002bc490[64])
#define DAT_002bc598 (DAT_002bc490[66])
#define DAT_002bc5a8 (DAT_002bc490[70])
#define DAT_002bc5f8 (DAT_002bc490[90])
#define DAT_002bc610 (DAT_002bc490[96])
#define DAT_002bc620 (DAT_002bc490[100])
#define DAT_002bc648 (DAT_002bc490[110])
#define DAT_002bc68c (DAT_002bc490[127])
#define DAT_002bc690 (DAT_002bc490[128])
#define DAT_002bc7b0 (DAT_002bc490[200])
#define DAT_002bc88c (DAT_002bc490[255])
#define DAT_002bc910 (DAT_002bc898[30])
#define DAT_002bc918 (DAT_002bc898[32])
#define DAT_002bc938 (DAT_002bc898[40])
#define DAT_002bc958 (DAT_002bc898[48])
#define DAT_002bc960 (DAT_002bc898[50])
#define DAT_002bc974 (DAT_002bc898[55])
#define DAT_002bc988 (DAT_002bc898[60])
#define DAT_002bc998 (DAT_002bc898[64])
#define DAT_002bca18 (DAT_002bc898[96])
#define DAT_002bca28 (DAT_002bc898[100])
#define DAT_002bca94 (DAT_002bc898[127])
#define DAT_002bca98 (DAT_002bc898[128])
#define DAT_002bcaf0 (DAT_002bc898[150])
#define DAT_002bcc08 (DAT_002bc898[220])
#define DAT_002bcc94 (DAT_002bc898[255])
#define DAT_002bce40 (DAT_002bce18[10])
#define DAT_002bcec0 (DAT_002bce18[42])
#define DAT_002bcf18 (DAT_002bce18[64])
#define DAT_002bcf30 (DAT_002bce18[70])
#define DAT_002bcf98 (DAT_002bce18[96])
#define DAT_002bcfa8 (DAT_002bce18[100])
#define DAT_002bd014 (DAT_002bce18[127])
#define DAT_002bd018 (DAT_002bce18[128])
#define DAT_002bd138 (DAT_002bce18[200])
#define DAT_002bd214 (DAT_002bce18[255])
#define DAT_002bd6a0 (DAT_002bd640[24])
#define DAT_002bd6e8 (DAT_002bd640[42])
#define DAT_002bd6f8 (DAT_002bd640[46])
#define DAT_002bd740 (DAT_002bd640[64])
#define DAT_002bd7c0 (DAT_002bd640[96])
#define DAT_002bd7d0 (DAT_002bd640[100])
#define DAT_002bd83c (DAT_002bd640[127])
#define DAT_002bd840 (DAT_002bd640[128])
#define DAT_002bda3c (DAT_002bd640[255])
#define DAT_002be2c4 (DAT_002be2b0[5])
#define DAT_002be2e8 (DAT_002be2b0[14])
#define DAT_002be334 (DAT_002be330[1])
#define DAT_002be340 (DAT_002be330[4])
#define DAT_002be344 (DAT_002be330[5])
#define DAT_002be350 (DAT_002be330[8])
#define DAT_002be354 (DAT_002be330[9])
#define DAT_002be574 (DAT_002be570[1])
#define DAT_002be578 (DAT_002be570[2])
#define DAT_002be604 (DAT_002be600[1])
#define DAT_002be610 (DAT_002be600[4])
#define DAT_002be618 (DAT_002be600[6])
#define DAT_002be620 (DAT_002be600[8])
#define DAT_002be630 (DAT_002be600[12])
#define DAT_002be648 (DAT_002be600[18])
#define DAT_002bee38 (DAT_002bee34[1])
#define DAT_002bee4c (DAT_002bee44[2])
#define DAT_002bee68 (DAT_002bee44[9])

#endif
