/*
D3D8_EXTRA.H

Direct3D 8 (Xbox, D3D8 1.0.3925) and XGRAPHICS declarations that this game
uses and that the shared XDK headers (src/runtime/include/xdk/, from OpenCE's
Halo port) do not have: the fixed-function pipeline (FVF codes, texture stage
operations, materials, lights), the missing device entry points, and the
XGRAPHICS helpers. The platform layer implements them in src/d3d8/
(d3d8_gl.c, d3d8_ff.c, xgraphics.c).

Values are the public Direct3D 8 ones; where the Xbox differs (the D3DTOP_*
order) the Xbox value is used. Structure layouts are checked below against
the copies the XBE's library code makes (D3DMATERIAL8: 0x11 dwords at
0x001f54a0, D3DLIGHT8: 0x1a dwords at 0x001f5cd0).

Game code: #include "d3d8_extra.h" (it includes xtl.h).
*/

#ifndef OPENFRENZY_D3D8_EXTRA_H
#define OPENFRENZY_D3D8_EXTRA_H

#include "xtl.h"

#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL ((HRESULT)0x8876086CL)
#endif

/* ---------- flexible vertex formats (SetVertexShader with an even handle) */

#define D3DFVF_RESERVED0 0x001
#define D3DFVF_POSITION_MASK 0x00E
#define D3DFVF_XYZ 0x002
#define D3DFVF_XYZRHW 0x004
#define D3DFVF_XYZB1 0x006
#define D3DFVF_XYZB2 0x008
#define D3DFVF_XYZB3 0x00a
#define D3DFVF_XYZB4 0x00c
#define D3DFVF_NORMAL 0x010
#define D3DFVF_PSIZE 0x020
#define D3DFVF_DIFFUSE 0x040
#define D3DFVF_SPECULAR 0x080
#define D3DFVF_TEXCOUNT_MASK 0xf00
#define D3DFVF_TEXCOUNT_SHIFT 8
#define D3DFVF_TEX0 0x000
#define D3DFVF_TEX1 0x100
#define D3DFVF_TEX2 0x200
#define D3DFVF_TEX3 0x300
#define D3DFVF_TEX4 0x400
/* bits 16 + 2 * n: the size of texture coordinate set n */
#define D3DFVF_TEXTUREFORMAT1 3
#define D3DFVF_TEXTUREFORMAT2 0
#define D3DFVF_TEXTUREFORMAT3 1
#define D3DFVF_TEXTUREFORMAT4 2
#define D3DFVF_TEXCOORDSIZE1(n) (D3DFVF_TEXTUREFORMAT1 << ((n) * 2 + 16))
#define D3DFVF_TEXCOORDSIZE2(n) (D3DFVF_TEXTUREFORMAT2)
#define D3DFVF_TEXCOORDSIZE3(n) (D3DFVF_TEXTUREFORMAT3 << ((n) * 2 + 16))
#define D3DFVF_TEXCOORDSIZE4(n) (D3DFVF_TEXTUREFORMAT4 << ((n) * 2 + 16))

/* vertex registers of the FVF elements (D3DVSDE_*; the XDK header has 0, 3,
4 and 9-12) */
#define D3DVSDE_BLENDWEIGHT 1
#define D3DVSDE_NORMAL 2
#define D3DVSDE_FOG 5
#define D3DVSDE_POINTSIZE 6
#define D3DVSDE_BACKDIFFUSE 7
#define D3DVSDE_BACKSPECULAR 8

/* ---------- texture stage operations (Xbox order) and arguments */

#define D3DTOP_DISABLE 1
#define D3DTOP_SELECTARG1 2
#define D3DTOP_SELECTARG2 3
#define D3DTOP_MODULATE 4
#define D3DTOP_MODULATE2X 5
#define D3DTOP_MODULATE4X 6
#define D3DTOP_ADD 7
#define D3DTOP_ADDSIGNED 8
#define D3DTOP_ADDSIGNED2X 9
#define D3DTOP_SUBTRACT 10
#define D3DTOP_ADDSMOOTH 11
#define D3DTOP_BLENDDIFFUSEALPHA 12
#define D3DTOP_BLENDCURRENTALPHA 13
#define D3DTOP_BLENDTEXTUREALPHA 14
#define D3DTOP_BLENDFACTORALPHA 15
#define D3DTOP_BLENDTEXTUREALPHAPM 16
#define D3DTOP_PREMODULATE 17
#define D3DTOP_MODULATEALPHA_ADDCOLOR 18
#define D3DTOP_MODULATECOLOR_ADDALPHA 19
#define D3DTOP_MODULATEINVALPHA_ADDCOLOR 20
#define D3DTOP_MODULATEINVCOLOR_ADDALPHA 21
#define D3DTOP_DOTPRODUCT3 22
#define D3DTOP_MULTIPLYADD 23
#define D3DTOP_LERP 24
#define D3DTOP_BUMPENVMAP 25
#define D3DTOP_BUMPENVMAPLUMINANCE 26

#define D3DTA_SELECTMASK 0x0000000f
#define D3DTA_DIFFUSE 0x00000000
#define D3DTA_CURRENT 0x00000001
#define D3DTA_TEXTURE 0x00000002
#define D3DTA_TFACTOR 0x00000003
#define D3DTA_SPECULAR 0x00000004
#define D3DTA_TEMP 0x00000005
#define D3DTA_COMPLEMENT 0x00000010
#define D3DTA_ALPHAREPLICATE 0x00000020

/* D3DTSS_TEXCOORDINDEX: the set in the low word, generation in the high */
#define D3DTSS_TCI_PASSTHRU 0x00000000
#define D3DTSS_TCI_CAMERASPACENORMAL 0x00010000
#define D3DTSS_TCI_CAMERASPACEPOSITION 0x00020000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 0x00030000

/* D3DTSS_TEXTURETRANSFORMFLAGS */
#define D3DTTFF_DISABLE 0
#define D3DTTFF_COUNT1 1
#define D3DTTFF_COUNT2 2
#define D3DTTFF_COUNT3 3
#define D3DTTFF_COUNT4 4
#define D3DTTFF_PROJECTED 256

/* D3DRS_*MATERIALSOURCE */
#define D3DMCS_MATERIAL 0
#define D3DMCS_COLOR1 1
#define D3DMCS_COLOR2 2

/* D3DTS_WORLD1-3 (vertex blending) follow D3DTS_WORLD in the Xbox enum */

/* ---------- materials and lights */

typedef struct _D3DCOLORVALUE
{
	float r, g, b, a;
} D3DCOLORVALUE;

typedef struct _D3DVECTOR
{
	float x, y, z;
} D3DVECTOR;

typedef struct _D3DMATERIAL8
{
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Ambient;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Emissive;
	float Power;
} D3DMATERIAL8;

typedef enum _D3DLIGHTTYPE
{
	D3DLIGHT_POINT = 1,
	D3DLIGHT_SPOT = 2,
	D3DLIGHT_DIRECTIONAL = 3,
	D3DLIGHT_FORCE_DWORD = 0x7fffffff,
} D3DLIGHTTYPE;

typedef struct _D3DLIGHT8
{
	D3DLIGHTTYPE Type;
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Ambient;
	D3DVECTOR Position;
	D3DVECTOR Direction;
	float Range;
	float Falloff;
	float Attenuation0;
	float Attenuation1;
	float Attenuation2;
	float Theta;
	float Phi;
} D3DLIGHT8;

typedef char d3d8_extra_material_size_check[sizeof(D3DMATERIAL8) == 0x11 * 4 ? 1 : -1];
typedef char d3d8_extra_light_size_check[sizeof(D3DLIGHT8) == 0x1a * 4 ? 1 : -1];

/* ---------- display field status */

#define D3DFIELD_ODD 1
#define D3DFIELD_EVEN 2
#define D3DFIELD_PROGRESSIVE 3

typedef struct _D3DFIELD_STATUS
{
	DWORD Field;
	DWORD VBlankCount;
} D3DFIELD_STATUS;

/* ---------- device entry points (XBE address of the library copy) */

/* 0x001f54a0 */ void WINAPI D3DDevice_SetMaterial(CONST D3DMATERIAL8 *material);
/* 0x001f5cd0 */ HRESULT WINAPI D3DDevice_SetLight(DWORD index, CONST D3DLIGHT8 *light);
/* 0x001f5eb0 */ HRESULT WINAPI D3DDevice_LightEnable(DWORD index, BOOL enable);
/* 0x001f5550 */ DWORD WINAPI D3DDevice_InsertFence(void);
/* 0x001f5510 */ void WINAPI D3D_KickOffAndWaitForIdle(void);
/* 0x001f6200 */ void WINAPI D3DDevice_GetDisplayFieldStatus(D3DFIELD_STATUS *status);
/* 0x00201390 */ void WINAPI D3DDevice_DrawVerticesUP(D3DPRIMITIVETYPE primitive_type, UINT vertex_count,
	CONST void *vertex_data, UINT vertex_stride);
/* 0x002022f0 (the XBE's symbol: D3D_GetDeviceCaps) */
HRESULT WINAPI Direct3D_GetDeviceCaps(UINT adapter, D3DDEVTYPE device_type, D3DCAPS8 *caps);
#define D3D_GetDeviceCaps Direct3D_GetDeviceCaps
/* 0x002022e0 is Direct3DCreate8 (xdk_pdb.h; it returns 1 on the Xbox too) */

/* ---------- XGRAPHICS */

/* the buffer XGAssembleShader returns its results in (ID3DXBuffer's Xbox
form): a reference count and the data, as the library's Release
(0x0022184b) and GetBufferPointer (0x00221874) read them */
typedef struct XGBuffer
{
	DWORD refCount;
	void *pData;
	DWORD size;
} XGBuffer;

/* 0x0022184b */ ULONG WINAPI XGBuffer_Release(XGBuffer *buffer);
/* 0x00221874 */ void *WINAPI XGBuffer_GetBufferPointer(XGBuffer *buffer);
DWORD WINAPI XGBuffer_GetBufferSize(XGBuffer *buffer);

/* assembler flags */
#define SASM_DONOTOPTIMIZE 0x00000001
#define SASM_DEBUG 0x00000002
#define SASM_SKIPVALIDATION 0x00000010

/* 0x0022835a: assembles vertex shader text (vs.1.1, xvs.1.1, xvss.1.1,
xvsw.1.1) into NV2A microcode, the form D3DDevice_CreateVertexShader takes:
a header dword (instruction count << 16 | type) and four dwords per
instruction. vs/xvs programs get the screen-space conversion appended, as
the XDK's assembler does. Pixel shaders are not supported (the game
assembles none). The resolver arguments are ignored. */
HRESULT WINAPI XGAssembleShader(LPCSTR source_file_name, LPCVOID source, UINT source_length, DWORD flags,
	XGBuffer **constants, XGBuffer **compiled_shader, XGBuffer **error_log, XGBuffer **listing,
	void *resolver, void *resolver_user_data, DWORD *shader_type);

/* 0x002285c1: fills a texture header (Common, Data, Lock, Format, Size)
for texture memory the caller owns */
VOID WINAPI XGSetTextureHeader(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
	D3DPOOL pool, D3DTexture *texture, UINT data, UINT pitch);

#define D3DUSAGE_BORDERSOURCE_TEXTURE 0x00010000L

#endif
