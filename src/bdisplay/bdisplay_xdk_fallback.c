/*
BDISPLAY_XDK_FALLBACK.C

Weak stand-ins for four library functions bdisplay calls that the d3d8
platform module does not implement yet (docs/LIBRARY_CALLS.md: MISSING).
They exist so the game image links; the d3d8 module's own (strong)
definitions replace them. Each logs once when it is used.

Caution for the d3d8 owner: the game image links libofplatform.a as an
archive, and a weak definition here satisfies the reference, so an
archive member that defines one of these functions and nothing else the
image needs is not pulled in. Put the real ones in a member that is linked
anyway (d3d8_gl.c, d3d8_resources.c), then delete this file.

XGSetTextureHeader is implemented (the header encoding of d3d8_resources.c's
create_texture); the others do nothing.
*/

#include "bdisplay_types.h"

void of_unimplemented(unsigned int address, const char *name);

__attribute__((weak)) void WINAPI D3DDevice_SetMaterial(CONST D3DMATERIAL8 *material)
{
	(void)material;
	of_unimplemented(0x001f54a0, "D3DDevice_SetMaterial (bdisplay weak fallback)");
}

__attribute__((weak)) void WINAPI D3DDevice_DrawVerticesUP(D3DPRIMITIVETYPE primitive_type, UINT vertex_count,
	CONST void *vertex_data, UINT vertex_stride)
{
	(void)primitive_type;
	(void)vertex_count;
	(void)vertex_data;
	(void)vertex_stride;
	of_unimplemented(0x00201390, "D3DDevice_DrawVerticesUP (bdisplay weak fallback)");
}

__attribute__((weak)) HRESULT WINAPI Direct3D_GetDeviceCaps(UINT adapter, D3DDEVTYPE device_type, D3DCAPS8 *caps)
{
	(void)adapter;
	(void)device_type;
	of_unimplemented(0x002022f0, "D3D_GetDeviceCaps (bdisplay weak fallback: the device's caps)");
	D3DDevice_GetDeviceCaps(caps);
	return S_OK;
}

/* the Xbox's linear (not swizzled) texture formats, D3DFMT_LIN_* and the YUV ones */
static int format_is_linear(DWORD format)
{
	switch (format)
	{
	case 0x10: case 0x11: case 0x12: case 0x13: case 0x16: case 0x17: case 0x1b: case 0x1c:
	case 0x1d: case 0x1e: case 0x1f: case 0x20: case 0x24: case 0x25: case 0x2e: case 0x2f:
	case 0x30: case 0x31: case 0x35: case 0x36: case 0x37: case 0x3d: case 0x3e: case 0x3f:
	case 0x40: case 0x41:
		return 1;
	default:
		return 0;
	}
}

static DWORD floor_log2(DWORD value)
{
	DWORD result = 0;

	while (value > 1)
	{
		value >>= 1;
		result++;
	}
	return result;
}

/* fills a texture header for memory the caller owns; data is the offset
D3DResource_Register later adds the base address to */
__attribute__((weak)) VOID WINAPI XGSetTextureHeader(UINT width, UINT height, UINT levels, DWORD usage,
	D3DFORMAT format, D3DPOOL pool, D3DTexture *texture, UINT data, UINT pitch)
{
	static int logged;

	(void)usage;
	(void)pool;
	if (!logged)
	{
		logged = 1;
		of_unimplemented(0x002285c1, "XGSetTextureHeader (bdisplay weak fallback, implemented)");
	}
	texture->Common = D3DCOMMON_TYPE_TEXTURE | 1;
	texture->Data = data;
	texture->Lock = 0;
	if (format_is_linear((DWORD)format))
	{
		texture->Format = ((DWORD)format << D3DFORMAT_FORMAT_SHIFT) | (1 << D3DFORMAT_MIPMAP_SHIFT) |
			(2 << D3DFORMAT_DIMENSION_SHIFT) | D3DFORMAT_DMACHANNEL_A;
		texture->Size = ((pitch / D3DTEXTURE_PITCH_ALIGNMENT - 1) << D3DSIZE_PITCH_SHIFT) |
			((height - 1) << D3DSIZE_HEIGHT_SHIFT) | (width - 1);
	}
	else
	{
		if (levels == 0)
			levels = floor_log2(width > height ? width : height) + 1;
		texture->Format = (floor_log2(height) << D3DFORMAT_VSIZE_SHIFT) |
			(floor_log2(width) << D3DFORMAT_USIZE_SHIFT) | ((DWORD)format << D3DFORMAT_FORMAT_SHIFT) |
			(levels << D3DFORMAT_MIPMAP_SHIFT) | (2 << D3DFORMAT_DIMENSION_SHIFT) | D3DFORMAT_DMACHANNEL_A;
		texture->Size = 0;
	}
}
