/*
XBTEXTURE.C  <-  C:\Babel\Xbox\Src\BDISPLAY\xbTexture.cpp

Textures: setting up the ones loaded from packages, creating new ones,
animation frames, selecting them into texture stages, locking.
0x00041520-0x00041d70. Only bdCreateTexture (0x00041b10) carries the file's
assert tag; the functions before it (after the last xbDrawprim.cpp tag) all
work on textures, so their file is inferred.
*/

#include "bdisplay_internal.h"

#define TEXTURE_FILE "C:\\Babel\\Xbox\\Src\\BDISPLAY\\xbTexture.cpp"

/* 0x00041520  BDISPLAY/xbTexture.cpp (inferred): frees every texture resource */
void bdDeleteAllTextures(void)
{
	FUN_0004ba60(1, 0xffff);
}

/* 0x00041530  BDISPLAY/xbTexture.cpp (inferred): the texture resource's
destructor (bKernel calls it for resource type 0): takes it out of the
texture stages; frees it unless it lives in package memory (first word < 0) */
void bdTextureDelete(bdTexture *texture)
{
	int stage = 0;

	do
	{
		if (DAT_002be5d4[stage] == texture)
		{
			DAT_002be5d4[stage] = NULL;
			D3DDevice_SetTexture(stage, NULL);
		}
		D3DDevice_SetPalette(stage, NULL);
		stage++;
	} while (stage < 4);
	if (-1 < *(int *)texture)
		bdFree(texture);
}

/* 0x00041580  BDISPLAY/xbTexture.cpp (inferred): shows animation frame
frame (modulo the frame count; negative counts from the end) */
void bdTextureSetFrame(bdTexture *texture, int frame)
{
	unsigned int numFrames = texture->numFrames;

	if (frame < 0)
		frame = (int)(numFrames - (unsigned int)(-frame % (int)numFrames));
	texture->current = &texture->frames[frame % (int)numFrames].d3d;
	texture->frame = (WORD)(frame % (int)numFrames);
}

/* 0x000415b0  BDISPLAY/xbTexture.cpp (inferred) */
void bdTextureGetInfo(bdTexture *texture, unsigned int *width, unsigned int *height, unsigned int *format,
	unsigned int *numFrames, unsigned int *frame)
{
	if (width != NULL)
		*width = texture->width;
	if (height != NULL)
		*height = texture->height;
	if (format != NULL)
		*format = texture->format;
	if (numFrames != NULL)
		*numFrames = texture->numFrames;
	if (frame != NULL)
		*frame = texture->frame;
}

/* 0x00041600  BDISPLAY/xbTexture.cpp (inferred): shows the animation frame
that is current at time (modulo the total time) */
void bdTextureSetTime(bdTexture *texture, int time)
{
	int totalTime = texture->totalTime;
	unsigned int numFrames;
	int elapsed, frame;
	int *frameTime;

	if (totalTime == 0)
		return;
	if (time < 0)
		time = totalTime - -time % totalTime;
	numFrames = texture->numFrames;
	elapsed = 0;
	frame = 0;
	if (numFrames == 0)
		return;
	frameTime = &texture->frames[0].time;
	while (elapsed += *frameTime, elapsed <= time % totalTime)
	{
		frame++;
		frameTime += 6;
		if ((int)(unsigned int)texture->numFrames <= frame)
			return;
	}
	if (frame < 0)
		frame = (int)(numFrames - (unsigned int)(-frame % (int)numFrames));
	texture->current = &texture->frames[frame % (int)numFrames].d3d;
	texture->frame = (WORD)(frame % (int)numFrames);
}

/* 0x00041680  BDISPLAY/xbTexture.cpp (inferred): which 0 sets the select
callback, 1 the deselect callback */
void bdTextureSetCallback(bdTexture *texture, int which, void *callback, void *context)
{
	if (which == 0)
	{
		texture->select = (bdTextureCallback)callback;
		texture->selectContext = context;
		return;
	}
	if (which == 1)
	{
		texture->deselect = (bdTextureCallback)callback;
		texture->deselectContext = context;
	}
}

/* stage 0's colour and alpha operation (D3DTOP_*; 4 with no texture, 5 with one) */
static void set_stage0_ops(DWORD op)
{
	D3DDevice_SetTextureStageState(0, D3DTSS_COLOROP, op);
	D3DDevice_SetTextureStageState(0, D3DTSS_ALPHAOP, op);
}

/* 0x000416c0  BDISPLAY/xbTexture.cpp (inferred): selects texture into a
stage (if the stage is enabled, DAT_002bee34). NULL, or a texture without a
frame, clears the stage; 1 does nothing. The old texture's deselect
callback runs first; the new one's select callback may refuse. Returns 1. */
int bdSetTexture(int stage, bdTexture *texture)
{
	bdTexture *old;

	if (texture != (bdTexture *)1 && DAT_002bee34[stage] != 0)
	{
		if (texture == NULL || texture->current == NULL)
		{
			D3DDevice_SetTexture(stage, NULL);
			if (DAT_002be5d4[stage] != NULL)
			{
				if (stage == 0)
					set_stage0_ops(4);
				old = DAT_002be5d4[stage];
				if (old->deselect != NULL)
					old->deselect(old, stage, old->deselectContext);
				if (DAT_002be5d4[stage]->format == BDTEXFMT_PAL8)
					D3DDevice_SetPalette(stage, NULL);
			}
			DAT_002be5d4[stage] = NULL;
		}
		else
		{
			if (DAT_002bc444[stage] != 0)
			{
				DAT_002bc444[stage] = 0;
				D3DDevice_SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
			}
			old = DAT_002be5d4[stage];
			if (old == NULL)
			{
				if (stage == 0)
					set_stage0_ops(5);
			}
			else if (old->deselect != NULL)
			{
				old->deselect(old, stage, old->deselectContext);
			}
			if (texture->select == NULL || texture->select(texture, stage, texture->selectContext) != 0)
			{
				DAT_002be5d4[stage] = texture;
				D3DDevice_SetTexture(stage, (D3DBaseTexture *)texture->current);
				if (texture->format == BDTEXFMT_PAL8)
				{
					D3DDevice_SetPalette(stage, &texture->palette);
					return 1;
				}
			}
		}
	}
	return 1;
}

/* 0x00041810  BDISPLAY/xbTexture.cpp (inferred): clears a stage (if enabled) */
void bdClearTexture(int stage)
{
	bdTexture *old;

	if (DAT_002bee34[stage] == 0)
		return;
	D3DDevice_SetTexture(stage, NULL);
	if (DAT_002be5d4[stage] != NULL)
	{
		if (stage == 0)
			set_stage0_ops(4);
		old = DAT_002be5d4[stage];
		if (old->deselect != NULL)
			old->deselect(old, stage, old->deselectContext);
		if (DAT_002be5d4[stage]->format == BDTEXFMT_PAL8)
			D3DDevice_SetPalette(stage, NULL);
		DAT_002be5d4[stage] = NULL;
	}
}

/* the D3D format and bits per texel of a Babel texture format; other
formats leave both unset in the original (here 0) */
static void texture_format(int format, D3DFORMAT *d3dFormat, int *bitsPerPixel)
{
	switch (format)
	{
	case BDTEXFMT_ARGB8888: *d3dFormat = (D3DFORMAT)0x06; *bitsPerPixel = 0x20; break;
	case BDTEXFMT_ARGB1555: *d3dFormat = (D3DFORMAT)0x02; *bitsPerPixel = 0x10; break;
	case BDTEXFMT_PAL8: *d3dFormat = (D3DFORMAT)0x0b; *bitsPerPixel = 8; break;
	case BDTEXFMT_ARGB4444: *d3dFormat = (D3DFORMAT)0x04; *bitsPerPixel = 0x10; break;
	case BDTEXFMT_LIN_ARGB8888: *d3dFormat = (D3DFORMAT)0x12; *bitsPerPixel = 0x20; break;
	case BDTEXFMT_LIN_RGB565: *d3dFormat = (D3DFORMAT)0x11; *bitsPerPixel = 0x10; break;
	default: *d3dFormat = (D3DFORMAT)0; *bitsPerPixel = 0; break;
	}
}

/* 0x00041890  BDISPLAY/xbTexture.cpp (inferred): sets up a texture loaded
from a package: the frame headers follow the 0x80-byte header, the palette
(0x400 bytes, P8 only) and the frames' texels follow them, each at a
multiple of 0x80 from the start */
void bdTextureSetup(bdTexture *texture)
{
	D3DFORMAT format;
	int bitsPerPixel, frameBytes, frame;
	unsigned int data;
	bdTextureFrame *frames;

	frames = (bdTextureFrame *)((BYTE *)texture + 0x80);
	texture->frames = frames;
	texture->current = &frames->d3d;
	data = ((((unsigned int)texture->numFrames * 0x18 + 0x7f) >> 7) + 1) * 0x80;
	texture_format(texture->format, &format, &bitsPerPixel);
	if (texture->format == BDTEXFMT_PAL8)
	{
		texture->palette.Common = 1;
		texture->palette.Data = data;
		texture->palette.Lock = 0;
		D3DResource_Register((D3DResource *)&texture->palette, texture);
		data += 0x400;
	}
	frameBytes = (texture->numTexels * bitsPerPixel >> 3) + 0x7f;
	texture->bitsPerPixel = (BYTE)bitsPerPixel;
	frame = 0;
	if (texture->numFrames != 0)
	{
		do
		{
			XGSetTextureHeader(texture->width, texture->height, texture->levels, 0, format, 0,
				&texture->frames[frame].d3d, data, (int)((unsigned int)texture->width * bitsPerPixel) >> 3);
			D3DResource_Register((D3DResource *)&texture->frames[frame].d3d, texture);
			data += ((frameBytes + (frameBytes >> 31 & 0x7f)) >> 7) * 0x80;
			frame++;
		} while (frame < (int)(unsigned int)texture->numFrames);
	}
	FUN_0004e8f0();
}

/* 0x00041a10  BDISPLAY/xbTexture.cpp (inferred): loads a texture from a package */
bdTexture *bdLoadTexture(void *package, unsigned int crc)
{
	bdTexture *texture = (bdTexture *)bdFindResource(package, crc);

	if (texture == NULL)
		return NULL;
	bdTextureSetup(texture);
	return texture;
}

/* 0x00041a40  BDISPLAY/xbTexture.cpp (inferred): locks a frame for
writing; returns the texels (also kept in lockedBits) */
void *bdTextureLock(bdTexture *texture, int frame, unsigned int *width, unsigned int *height, int *pitch,
	unsigned int *format, DWORD **palette)
{
	D3DLOCKED_RECT locked;

	*width = texture->width;
	*height = texture->height;
	*format = texture->format;
	if (texture->format == BDTEXFMT_PAL8)
	{
		D3DPalette_Lock(&texture->palette, palette, 0);
		texture->lockedPalette = *palette;
	}
	D3DTexture_LockRect(&texture->frames[frame].d3d, 0, &locked, NULL, 0);
	if (pitch != NULL)
		*pitch = locked.Pitch;
	texture->lockedBits = locked.pBits;
	return locked.pBits;
}

/* 0x00041ac0  BDISPLAY/xbTexture.cpp (inferred): flushes the writes of
bdTextureLock (sizes in dwords) */
void bdTextureUnlock(bdTexture *texture)
{
	FUN_0004e900(texture->lockedBits,
		(int)((unsigned int)texture->bitsPerPixel * texture->height * texture->width) >> 5);
	if (texture->format == BDTEXFMT_PAL8)
		FUN_0004e900(texture->lockedPalette, 0x100);
}

/* 0x00041b10  BDISPLAY/xbTexture.cpp: a new texture of numFrames frames
(format 0: BDTEXFMT_LIN_ARGB8888); the third argument is not used */
bdTexture *bdCreateTexture(int width, int height, int unused, int numFrames, short flags, int format)
{
	D3DFORMAT d3dFormat;
	int bitsPerPixel, paletteBytes, frameBytes, offset;
	unsigned int headerBytes, data;
	bdTexture *texture;
	DWORD *links;

	(void)unused;
	if (format == 0)
		format = BDTEXFMT_LIN_ARGB8888;
	paletteBytes = format == BDTEXFMT_PAL8 ? 0x400 : 0;
	texture_format(format, &d3dFormat, &bitsPerPixel);
	frameBytes = (bitsPerPixel * width * height >> 3) + 0x7f;
	frameBytes = ((frameBytes + (frameBytes >> 31 & 0x7f)) >> 7) * 0x80;
	headerBytes = (unsigned int)(numFrames * 0x18) + 0x7fu & 0xffffff80u;
	texture = (bdTexture *)bdAllocFill(frameBytes * numFrames + headerBytes + 0x100 + paletteBytes, 0,
		TEXTURE_FILE, 0x1e2, 0x2001, "Texture");
	texture->format = (WORD)format;
	texture->width = (WORD)width;
	texture->height = (WORD)height;
	texture->flags = (WORD)flags;
	texture->numTexels = width * height;
	texture->frames = (bdTextureFrame *)((BYTE *)texture + 0x80);
	texture->current = (D3DTexture *)((BYTE *)texture + 0x80);
	texture->bitsPerPixel = (BYTE)bitsPerPixel;
	texture->levels = 1;
	texture->numFrames = (BYTE)numFrames;
	texture->frame = 0;
	if (format == BDTEXFMT_PAL8)
	{
		texture->palette.Common = 1;
		texture->palette.Data = headerBytes + 0x80;
		texture->palette.Lock = 0;
		D3DResource_Register((D3DResource *)&texture->palette, texture);
	}
	/* the texels start at the next 0x80-byte boundary of the address */
	data = (-(unsigned int)(uintptr_t)texture & 0x7fu) + headerBytes + 0x80 + paletteBytes;
	if (0 < numFrames)
	{
		offset = 0;
		do
		{
			((bdTextureFrame *)((BYTE *)texture->frames + offset))->time = 0;
			XGSetTextureHeader(width, height, 1, 0, d3dFormat, 0,
				(D3DTexture *)((BYTE *)texture->frames + offset), data, bitsPerPixel * width >> 3);
			D3DResource_Register((D3DResource *)((BYTE *)texture->frames + offset), texture);
			data += frameBytes;
			offset += 0x18;
			numFrames--;
		} while (numFrames != 0);
	}
	/* the resource header's list links point at the texture itself */
	links = (DWORD *)texture->resource;
	links[5] = (DWORD)(uintptr_t)texture;
	links[4] = (DWORD)(uintptr_t)texture;
	links[7] = (DWORD)(uintptr_t)texture;
	links[6] = (DWORD)(uintptr_t)texture;
	return texture;
}
