/*
XBDATA.C

The data of the Babel display layer (BDISPLAY), by original address. The
original source files are xbDisplay.cpp, xbDrawprim.cpp, xbTexture.cpp,
xbFont.cpp and xbRender.cpp; which file defined each variable is not known,
so they are kept together here. Declarations: bdisplay_globals.h.

Uninitialised data are tentative definitions (no initialiser), so another
module's tentative definition of the same name merges with them.
*/

#include "bdisplay_types.h"
#include "bdisplay_globals.h"

/* ---------- .data */

/* 0x00284c10 */ struct bdVertexBuffer *DAT_00284c10 = (struct bdVertexBuffer *)1;
/* 0x00284c14 */ uint32_t DAT_00284c14[7] = {
	D3DPT_POINTLIST, D3DPT_LINELIST, D3DPT_LINESTRIP, D3DPT_TRIANGLELIST,
	D3DPT_TRIANGLESTRIP, D3DPT_TRIANGLEFAN, 0,
};
/* 0x00284c30 */ float DAT_00284c30[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};
/* 0x00284c70 */ float *PTR_DAT_00284c70 = DAT_002be2b0;
/* 0x00284c78 { SRCBLEND, DESTBLEND, BLENDOP } (D3DBLEND_*, D3DBLENDOP_*: the GL values) */
uint32_t DAT_00284c78[7][3] = {
	{ 0x302, 0x303, 0x8006 }, /* SRCALPHA, INVSRCALPHA, ADD */
	{ 0x302, 0x001, 0x8006 }, /* SRCALPHA, ONE, ADD */
	{ 0x306, 0x300, 0x8006 }, /* DESTCOLOR, SRCCOLOR, ADD */
	{ 0x302, 0x001, 0x800b }, /* SRCALPHA, ONE, REVSUBTRACT */
	{ 0x001, 0x000, 0x8006 }, /* ONE, ZERO, ADD */
	{ 0x304, 0x305, 0x8006 }, /* DESTALPHA, INVDESTALPHA, ADD */
	{ 0x304, 0x001, 0x8006 }, /* DESTALPHA, ONE, ADD */
};
/* 0x00284ccc D3DCMP_*: NEVER, ALWAYS, LESSEQUAL, LESS, NOTEQUAL, GREATEREQUAL, EQUAL, GREATER */
uint32_t DAT_00284ccc[8] = { 0x200, 0x207, 0x203, 0x201, 0x206, 0x204, 0x202, 0x205 };
/* 0x00284cec D3DCULL_*: NONE, CCW, CW */
uint32_t DAT_00284cec[3] = { 0, 0x901, 0x900 };
/* 0x00284cf8 D3DTEXF_* (1 point, 2 linear) for MAGFILTER, MINFILTER, MIPFILTER */
uint32_t DAT_00284cf8[6][3] = {
	{ 0, 0, 0 }, { 1, 1, 0 }, { 2, 2, 0 }, { 1, 1, 1 }, { 2, 2, 1 }, { 2, 2, 2 },
};
/* 0x00284d40 */ int DAT_00284d40 = 1;
/* 0x00284d48 */ bdErrorName DAT_00284d48[27] = {
	{ (int32_t)0x88760818u, "D3DERR_WRONGTEXTUREFORMAT" },
	{ (int32_t)0x88760819u, "D3DERR_UNSUPPORTEDCOLOROPERATION" },
	{ (int32_t)0x8876081au, "D3DERR_UNSUPPORTEDCOLORARG" },
	{ (int32_t)0x8876081bu, "D3DERR_UNSUPPORTEDALPHAOPERATION" },
	{ (int32_t)0x8876081cu, "D3DERR_UNSUPPORTEDALPHAARG" },
	{ (int32_t)0x8876081du, "D3DERR_TOOMANYOPERATIONS" },
	{ (int32_t)0x8876081eu, "D3DERR_CONFLICTINGTEXTUREFILTER" },
	{ (int32_t)0x8876081fu, "D3DERR_UNSUPPORTEDFACTORVALUE" },
	{ (int32_t)0x88760821u, "D3DERR_CONFLICTINGRENDERSTATE" },
	{ (int32_t)0x88760822u, "D3DERR_UNSUPPORTEDTEXTUREFILTER" },
	{ (int32_t)0x88760826u, "D3DERR_CONFLICTINGTEXTUREPALETTE" },
	{ (int32_t)0x88760827u, "D3DERR_DRIVERINTERNALERROR" },
	{ (int32_t)0x88760866u, "D3DERR_NOTFOUND" },
	{ (int32_t)0x88760867u, "D3DERR_MOREDATA" },
	{ (int32_t)0x88760868u, "D3DERR_DEVICELOST" },
	{ (int32_t)0x88760869u, "D3DERR_DEVICENOTRESET" },
	{ (int32_t)0x8876086au, "D3DERR_NOTAVAILABLE" },
	{ (int32_t)0x8876017cu, "D3DERR_OUTOFVIDEOMEMORY" },
	{ (int32_t)0x8876086bu, "D3DERR_INVALIDDEVICE" },
	{ (int32_t)0x8876086cu, "D3DERR_INVALIDCALL" },
	{ (int32_t)0x8878001eu, "DSERR_CONTROLUNAVAIL" },
	{ (int32_t)0x88780032u, "DSERR_INVALIDCALL" },
	{ (int32_t)0x80004005u, "DSERR_GENERIC" },
	{ (int32_t)0x8007000eu, "DSERR_OUTOFMEMORY" },
	{ (int32_t)0x80004001u, "DSERR_UNSUPPORTED" },
	{ (int32_t)0x88780078u, "DSERR_NODRIVER" },
	{ (int32_t)0x80040110u, "DSERR_NOAGGREGATION" },
};
/* 0x00284e20 */ int DAT_00284e20 = 27;
/* 0x002855f0 */ float DAT_002855f0[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};

/* ---------- .bss: display state */

/* 0x002bc3ec */ int DAT_002bc3ec;
/* 0x002bc400 */ uint32_t DAT_002bc400;
/* 0x002bc408 */ int DAT_002bc408;
/* 0x002bc40c */ int DAT_002bc40c;
/* 0x002bc410 */ int DAT_002bc410;
/* 0x002bc414 */ int DAT_002bc414;
/* 0x002bc418 */ int DAT_002bc418;
/* 0x002bc41c */ int DAT_002bc41c;
/* 0x002bc420 */ int DAT_002bc420;
/* 0x002bc424 */ int DAT_002bc424;
/* 0x002bc428 */ float DAT_002bc428;
/* 0x002bc42c */ float DAT_002bc42c;
/* 0x002bc430 */ int DAT_002bc430;
/* 0x002bc434 */ int DAT_002bc434;
/* 0x002bc438 */ int DAT_002bc438;
/* 0x002bc43c */ int DAT_002bc43c;
/* 0x002bc440 */ int DAT_002bc440;
/* 0x002bc444 */ int DAT_002bc444[4];
/* 0x002bc464 */ struct bdRenderTarget *DAT_002bc464;
/* 0x002bc468 */ int DAT_002bc468;
/* 0x002bc46c */ int DAT_002bc46c;
/* 0x002bc470 */ int DAT_002bc470;
/* 0x002bc474 */ uint32_t DAT_002bc474;
/* 0x002bc478 */ void *DAT_002bc478;
/* 0x002bc47c */ void *DAT_002bc47c;
/* 0x002bc480 */ void *DAT_002bc480;
/* 0x002bc484 */ void *DAT_002bc484;
/* 0x002bc488 */ void *DAT_002bc488;
/* 0x002bc48c */ int DAT_002bc48c;

/* 0x002bc490 */ unsigned int DAT_002bc490[256];
/* 0x002bc890 */ int DAT_002bc890;
/* 0x002bc898 */ unsigned int DAT_002bc898[256];
/* 0x002bcc98 */ struct bdRenderTarget DAT_002bcc98;
/* 0x002bcda8 */ struct bdIndexBuffer DAT_002bcda8;
/* 0x002bcdc8 */ struct bdVertexBufferList DAT_002bcdc8;
/* 0x002bce18 */ unsigned int DAT_002bce18[256];
/* 0x002bd218 */ unsigned long long DAT_002bd218;
/* 0x002bd220 */ float DAT_002bd220[256];
/* 0x002bd620 */ uint32_t DAT_002bd620[8];
/* 0x002bd640 */ unsigned int DAT_002bd640[256];
/* 0x002bda40 */ int DAT_002bda40[256];
/* 0x002bde40 */ int DAT_002bde40[256];
/* 0x002be240 */ unsigned long long DAT_002be240;
/* 0x002be248 */ float DAT_002be248;
/* 0x002be25c */ int DAT_002be25c;
/* 0x002be260 */ float DAT_002be260;
/* 0x002be264 */ void *DAT_002be264;
/* 0x002be268 */ int DAT_002be268;

/* ---------- .bss: camera and matrices */

/* 0x002be270 */ float DAT_002be270[16];
/* 0x002be2b0 */ float DAT_002be2b0[16];
/* 0x002be2f0 */ float DAT_002be2f0[16];
/* 0x002be330 */ float DAT_002be330[16];
/* 0x002be370 */ float DAT_002be370[16];
/* 0x002be3b0 */ float DAT_002be3b0[16];
/* 0x002be3f0 */ float DAT_002be3f0[16];
/* 0x002be430 */ float DAT_002be430[16];
/* 0x002be470 */ float DAT_002be470[16];
/* 0x002be4b0 */ float DAT_002be4b0[16];
/* 0x002be4f0 */ float DAT_002be4f0[16];
/* 0x002be530 */ float DAT_002be530[16];
/* 0x002be570 */ float DAT_002be570[3];
/* 0x002be57c */ int DAT_002be57c;
/* 0x002be580 */ float DAT_002be580;
/* 0x002be584 */ float DAT_002be584;
/* 0x002be588 */ float DAT_002be588;
/* 0x002be58c */ float DAT_002be58c;
/* 0x002be590 */ float DAT_002be590;
/* 0x002be594 */ float DAT_002be594;
/* 0x002be598 */ float DAT_002be598;
/* 0x002be59c */ float DAT_002be59c;
/* 0x002be5a0 */ float DAT_002be5a0;
/* 0x002be5a4 */ float DAT_002be5a4;
/* 0x002be5a8 */ float DAT_002be5a8;
/* 0x002be5ac */ float DAT_002be5ac;
/* 0x002be5b0 */ float DAT_002be5b0;
/* 0x002be5c4 */ int DAT_002be5c4;
/* 0x002be5c8 */ int DAT_002be5c8;
/* 0x002be5cc */ int DAT_002be5cc;

/* ---------- .bss: render state */

/* 0x002be5d0 */ int DAT_002be5d0;
/* 0x002be5d4 */ struct bdTexture *DAT_002be5d4[4];
/* 0x002be5e4 */ float DAT_002be5e4;
/* 0x002be5e8 */ float DAT_002be5e8;
/* 0x002be5ec */ float DAT_002be5ec;
/* 0x002be5f0 */ uint32_t DAT_002be5f0;
/* 0x002be5f4 */ uint32_t DAT_002be5f4;
/* 0x002be5f8 */ uint32_t DAT_002be5f8;
/* 0x002be600 */ uint32_t DAT_002be600[BD_NUM_STATES * 2];
/* 0x002be678 */ uint32_t DAT_002be678[BD_NUM_STATES * BD_STATE_STACK * 2];
/* 0x002bedf8 */ int DAT_002bedf8[BD_NUM_STATES];
/* 0x002bee34 */ int DAT_002bee34[4];
/* 0x002bee44 */ int DAT_002bee44[BD_NUM_STATES];
/* 0x002bee80 */ int DAT_002bee80;
/* 0x002bee84 */ int DAT_002bee84;
/* 0x002bee90 */ struct bdTLVertex DAT_002bee90[4];
/* 0x002bef00 */ struct bdPrintChar DAT_002bef00[0x400];

/* ---------- .bss: end-scene objects */

/* 0x002caf08 */ void **DAT_002caf08;
/* 0x002caf0c */ void **DAT_002caf0c;
/* 0x002caf10 */ void **DAT_002caf10;
