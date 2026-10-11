/*
XBDISPLAY.C  <-  C:\Babel\Xbox\Src\BDISPLAY\xbDisplay.cpp

The display: start-up (Direct3D device, colour tables, default states),
shut-down, the per-frame flip, scenes, surface locking and screenshots.
0x00041d70-0x00042e10. Only bSaveSurfaceAsBMP (0x000426e0) carries the
file's assert tag; the others are placed here by what they do (inferred).
*/

#include "bdisplay_internal.h"

#define DISPLAY_FILE "C:\\Babel\\Xbox\\Src\\BDISPLAY\\xbDisplay.cpp"

unsigned int _controlfp(unsigned int new_value, unsigned int mask);

/* 0x002bc408-0x002bc48c: the original clears this run of globals in one loop */
static void clear_display_state(void)
{
	DAT_002bc408 = 0;
	DAT_002bc40c = 0;
	DAT_002bc410 = 0;
	DAT_002bc414 = 0;
	DAT_002bc418 = 0;
	DAT_002bc41c = 0;
	DAT_002bc420 = 0;
	DAT_002bc424 = 0;
	DAT_002bc428 = 0.0f;
	DAT_002bc42c = 0.0f;
	DAT_002bc430 = 0;
	DAT_002bc434 = 0;
	DAT_002bc438 = 0;
	DAT_002bc43c = 0;
	DAT_002bc440 = 0;
	memset(DAT_002bc444, 0, sizeof(DAT_002bc444));
	DAT_002bc464 = NULL;
	DAT_002bc468 = 0;
	DAT_002bc46c = 0;
	DAT_002bc470 = 0;
	DAT_002bc474 = 0;
	DAT_002bc478 = NULL;
	DAT_002bc47c = NULL;
	DAT_002bc480 = NULL;
	DAT_002bc484 = NULL;
	DAT_002bc488 = NULL;
	DAT_002bc48c = 0;
}

/* 0x002be330-0x002be5c8: the original clears this run of globals in one loop */
static void clear_camera_state(void)
{
	memset(DAT_002be330, 0, sizeof(DAT_002be330));
	memset(DAT_002be370, 0, sizeof(DAT_002be370));
	memset(DAT_002be3b0, 0, sizeof(DAT_002be3b0));
	memset(DAT_002be3f0, 0, sizeof(DAT_002be3f0));
	memset(DAT_002be430, 0, sizeof(DAT_002be430));
	memset(DAT_002be470, 0, sizeof(DAT_002be470));
	memset(DAT_002be4b0, 0, sizeof(DAT_002be4b0));
	memset(DAT_002be4f0, 0, sizeof(DAT_002be4f0));
	memset(DAT_002be530, 0, sizeof(DAT_002be530));
	memset(DAT_002be570, 0, sizeof(DAT_002be570));
	DAT_002be57c = 0;
	DAT_002be580 = 0.0f;
	DAT_002be584 = 0.0f;
	DAT_002be588 = 0.0f;
	DAT_002be58c = 0.0f;
	DAT_002be590 = 0.0f;
	DAT_002be594 = 0.0f;
	DAT_002be598 = 0.0f;
	DAT_002be59c = 0.0f;
	DAT_002be5a0 = 0.0f;
	DAT_002be5a4 = 0.0f;
	DAT_002be5a8 = 0.0f;
	DAT_002be5ac = 0.0f;
	DAT_002be5b0 = 0.0f;
	DAT_002be5c4 = 0;
	DAT_002be5c8 = 0;
}

/* 0x00041d70  BDISPLAY/xbDisplay.cpp (inferred): the display layer's
start-up, before the display opens; 0 if FUN_000a6fa0 fails */
int bdInit(void)
{
	clear_display_state();
	clear_camera_state();
	DAT_002bc890 = 0;
	DAT_002bc48c = 0;
	DAT_002bcdc8.next = (bdVertexBuffer *)&DAT_002bcdc8;
	DAT_002bcdc8.prev = (bdVertexBuffer *)&DAT_002bcdc8;
	DAT_002be588 = bd_bits_float(0x3f9c61aa); /* 70 degrees */
	DAT_002be58c = bd_bits_float(0x3f6a927f); /* 52.5 degrees */
	DAT_002be580 = 40.0f;
	DAT_002be584 = 10000.0f;
	DAT_002be25c = 1;
	DAT_002bcda8.next = &DAT_002bcda8;
	DAT_002bcda8.prev = &DAT_002bcda8;
	DAT_002bcc98.next = &DAT_002bcc98;
	DAT_002bcc98.prev = &DAT_002bcc98;
	if (FUN_000a6fa0() == 0)
		return 0;
	DAT_002bc464 = &DAT_002bcc98;
	FUN_0004f890((struct bkProfileTimer *)DAT_002bd620, 0x1e);
	memcpy(DAT_002be330, DAT_002855f0, sizeof(DAT_002be330));
	return 1;
}

/* 0x00041e40  BDISPLAY/xbDisplay.cpp (inferred): frees every texture,
vertex buffer, index buffer and render target */
void bdShutdown(void)
{
	bdDeleteAllTextures();
	while (DAT_002bcdc8.prev != (bdVertexBuffer *)&DAT_002bcdc8)
		bdVertexBufferDestroy(DAT_002bcdc8.prev);
	DAT_002bcdc8.prev = (bdVertexBuffer *)&DAT_002bcdc8;
	while (DAT_002bcda8.prev != &DAT_002bcda8)
		bdIndexBufferDestroy(DAT_002bcda8.prev);
	bdDeleteRenderTarget(NULL);
}

/* 0x00041e90  BDISPLAY/xbDisplay.cpp (inferred) */
void bdGetScreenInfo(int *width, int *height, int *colourBits, int *zBits)
{
	if (width != NULL)
		*width = DAT_002bc40c;
	if (height != NULL)
		*height = DAT_002bc410;
	if (colourBits != NULL)
		*colourBits = DAT_002bc41c;
	if (zBits != NULL)
		*zBits = DAT_002bc420;
}

/* 0x00041ee0  BDISPLAY/xbDisplay.cpp (inferred): ends the frame: back to
the screen, the per-frame work of the other systems, Present, frame
timing, then the clear of the next frame (render state 9 says what to
clear) in colour r, g, b, a */
void bdFlip(unsigned int r, unsigned int g, unsigned int b, int a)
{
	unsigned long long now;
	unsigned long long last = DAT_002bd218;
	unsigned int flags;

	if (DAT_002bc440 == 0)
		return;
	if (DAT_002bc464 != &DAT_002bcc98)
		bdSetRenderTarget(NULL, 0, 0, 0, 0, 1.0f, 0);
	FUN_0004e630();
	FUN_0004f210();
	FUN_00049560();
	FUN_0004a0f0();
	FUN_00056190();
	DirectSoundDoWork();
	FUN_00057160();
	D3DDevice_Present(NULL, NULL, NULL, NULL);
	now = FUN_0004c5f0();
	DAT_002be240 = bdTimerDelta(last, now);
	if (FUN_0004f8c0((struct bkProfileTimer *)DAT_002bd620) != 0)
	{
		long long ticks;

		memcpy(&ticks, DAT_002bd620, sizeof(ticks));
		DAT_002be248 = bdTimerRate(ticks);
	}
	FUN_0004f8b0((struct bkProfileTimer *)DAT_002bd620);
	DAT_002bd218 = now;
	flags = DAT_002be648;
	if (flags != 0)
	{
		D3DDevice_Clear(0, NULL, ((flags & 1) != 0 ? 0xf0 : 0) | (flags >> 1 & 3),
			((((r & 0xff) | (unsigned int)a << 8) << 8 | (g & 0xff)) << 8) | (b & 0xff), 1.0f, 0);
	}
	DAT_002bc890++;
}

/* 0x00042020  BDISPLAY/xbDisplay.cpp (inferred): creates the Direct3D
device at DAT_002bc40c x DAT_002bc410 and sets the default state */
int bStartDisplay(void)
{
	D3DPRESENT_PARAMETERS present;
	D3DCAPS8 caps;
	D3DMATERIAL8 material;
	HRESULT result;
	int freeBefore, i, twice;
	float alpha;

	if (DAT_002bc440 != 0)
		return 0;
	bdPrintf("bStartDisplay: Trying (%dx%d) 32 bpp 24:8 Z/stencil\n", DAT_002bc40c, DAT_002bc410);
	DAT_002bc478 = Direct3DCreate8(0);
	if (DAT_002bc478 == NULL)
	{
		bdPrintf("bStartDisplay: *** ERROR : Direct3DCreate8 Failed ***\n");
		return 0;
	}
	if ((DAT_002bc408 & 0x40) == 0)
	{
		if ((DAT_002bc408 & 0x200) != 0)
		{
			DAT_002bc46c = 2;
			DAT_002bc470 = 2;
			DAT_002bc474 = 0x1222;
			goto multisample_set;
		}
		DAT_002bc46c = 1;
		DAT_002bc474 = 0x11;
	}
	else
	{
		DAT_002bc46c = 2;
		DAT_002bc474 = 0x1121;
	}
	DAT_002bc470 = 1;
multisample_set:
	memset(&present, 0, sizeof(present));
	present.Windowed = 0;
	present.BackBufferFormat = (D3DFORMAT)7;
	present.SwapEffect = 1;
	present.BackBufferWidth = DAT_002bc40c;
	present.BackBufferHeight = DAT_002bc410;
	present.BackBufferCount = 1;
	present.EnableAutoDepthStencil = 1;
	present.AutoDepthStencilFormat = (D3DFORMAT)0x2a;
	present.Flags = 0;
	present.MultiSampleType = DAT_002bc474;
	present.FullScreen_PresentationInterval = (DAT_002bc408 & 0x100) != 0 ? 0x80000000u : 1;
	result = Direct3D_GetDeviceCaps(0, (D3DDEVTYPE)1, &caps);
	if (result < 0)
	{
		bdPrintf("bStartDisplay: GetDeviceCaps failed (%s)\n", bdGetErrorString(result));
		return 0;
	}
	bdPrintf("bStartDisplay: Device supports Vertex Shaders v%d.%d with %d constant registers\n",
		caps.VertexShaderVersion >> 8 & 0xff, caps.VertexShaderVersion & 0xff, caps.MaxVertexShaderConst);
	bdPrintf("bStartDisplay: There are %d texture blend stages with %d simultaneous textures\n",
		caps.MaxTextureBlendStages, caps.MaxSimultaneousTextures);
	bdPrintf("bStartDisplay: Max point size is %.2f\n", (double)caps.MaxPointSize);
	DAT_002bc438 = caps.VertexShaderVersion != 0;
	DAT_002bc430 = 1;
	DAT_002bc434 = 1;
	freeBefore = FUN_0004c2e0();
	result = Direct3D_CreateDevice(0, (D3DDEVTYPE)1, NULL, 0x40, &present, (D3DDevice **)&DAT_002bc47c);
	if (result < 0)
	{
		bdPrintf("bStartDisplay: CreateDevice failed (%s)\n", bdGetErrorString(result));
		return 0;
	}
	bdPrintf("bStartDisplay: Frame buffers use %d Kb of RAM\n", (freeBefore - (int)FUN_0004c2e0()) >> 10);
	D3DDevice_GetBackBuffer(0, 0, (D3DSurface **)&DAT_002bc480);
	result = D3DDevice_GetDepthStencilSurface((D3DSurface **)&DAT_002bc484);
	if (result < 0)
	{
		bdPrintf("bStartDisplay: GetDepthStencilBuffer failed (%s)\n", bdGetErrorString(result));
		if (DAT_002bc47c != NULL)
		{
			while (D3DDevice_Release() != 0)
			{
			}
			DAT_002bc47c = NULL;
		}
		if (DAT_002bc478 != NULL)
		{
			/* Direct3D_Release always returns 1: the original hangs here */
			while (Direct3D_Release() != 0)
				__asm__ volatile("");
			DAT_002bc478 = NULL;
		}
		return 0;
	}
	D3DResource_Release((D3DResource *)DAT_002bc484);
	D3DDevice_GetBackBuffer(-1, 0, (D3DSurface **)&DAT_002bc488);
	DAT_002bc400 = 1;
	DAT_00284c10 = (bdVertexBuffer *)1;
	bdSetVertexShaderAndStream(BD_FVF_VERTEX, NULL);
	memset(&caps, 0, sizeof(caps));
	D3DDevice_GetDeviceCaps(&caps);
	DAT_002bc3ec = (int)caps.MaxActiveLights;
	bdPrintf("Device supports up to %d active HW light sources\n", caps.MaxActiveLights);
	i = 0;
	twice = 0;
	do
	{
		DAT_002bce18[i] = (unsigned int)i << 16;
		DAT_002bc490[i] = (unsigned int)i << 8;
		DAT_002bd640[i] = (unsigned int)i;
		DAT_002bc898[i] = (unsigned int)i << 24;
		alpha = 1.0f;
		if (twice < 0xff)
			alpha = (float)twice * bd_bits_float(0x3b808081); /* 1 / 255 */
		twice += 2;
		DAT_002bd220[i] = alpha;
		DAT_002bde40[i] = i;
		DAT_002bda40[i] = i;
		i++;
	} while (twice < 0x200);
	DAT_002bc414 = DAT_002bc40c / 2;
	DAT_002bc418 = DAT_002bc410 / 2;
	DAT_002bc428 = 0.5f;
	DAT_002bc43c = 3;
	DAT_002bc42c = ((float)DAT_002bc410 / (float)DAT_002bc40c) * 0.5f;
	bdSetViewport(0, 0, DAT_002bc40c, DAT_002bc410);
	memset(&material, 0, sizeof(material));
	material.Diffuse.r = 1.0f;
	material.Diffuse.g = 1.0f;
	material.Diffuse.b = 1.0f;
	/* the original stores 127 at offset 0x2c, which is Specular.a */
	material.Specular.a = 127.0f;
	D3DDevice_SetMaterial(&material);
	DAT_002bc408 |= 1;
	bdResetRenderStates();
	bdSetup2DProjection();
	bdSetupProjection();
	bdSetProjectionMode(1);
	bdSetNearFar(1.0f, 10000.0f);
	bdSetFogRange(8000.0f, 10000.0f);
	bdSetFogColour(0, 0, 0);
	FUN_0003b700();
	D3DDevice_Clear(0, NULL, 0xf3, 0, 1.0f, 0);
	D3DDevice_Present(NULL, NULL, NULL, NULL);
	D3DDevice_Clear(0, NULL, 0xf3, 0, 1.0f, 0);
	D3DDevice_Present(NULL, NULL, NULL, NULL);
	DAT_002bc468 = 0;
	DAT_002bc440 = 1;
	bdPrintf("Dummy present took %f ms\n", (double)DAT_002be260);
	return 1;
}

/* 0x000424e0  BDISPLAY/xbDisplay.cpp (inferred): releases the device.
As in bStartDisplay's failure path, releasing the Direct3D object never
ends (the XDK's Direct3D_Release returns 1): on the Xbox, closing the
display hangs here. */
void bEndDisplay(void)
{
	if (DAT_002bc440 == 0)
		return;
	FUN_0003bc90();
	D3DResource_Release((D3DResource *)DAT_002bc480);
	if (DAT_002bc47c != NULL)
	{
		while (D3DDevice_Release() != 0)
		{
		}
		DAT_002bc47c = NULL;
	}
	if (DAT_002bc478 != NULL)
	{
		while (Direct3D_Release() != 0)
			__asm__ volatile("");
		DAT_002bc478 = NULL;
	}
	DAT_002bc440 = 0;
}

/* 0x00042530  BDISPLAY/xbDisplay.cpp (inferred) */
void bdCloseDisplay(void)
{
	if ((DAT_002bc408 & 1) == 0)
	{
		bdPrintf("bdCloseDisplay: Display is not open\n");
		return;
	}
	bdFontShutdown();
	bEndDisplay();
	DAT_002bc408 &= ~1;
}

/* 0x00042570  BDISPLAY/xbDisplay.cpp (inferred) */
int bdBeginScene(void)
{
	if (DAT_002bc440 == 0)
		return 0;
	DAT_002bc468 = 1;
	return 1;
}

/* 0x00042590  BDISPLAY/xbDisplay.cpp (inferred) */
void bdEndScene(void)
{
	FUN_00047040();
	DAT_002bc468 = 0;
}

/* 0x000425a0  BDISPLAY/xbDisplay.cpp (inferred): locks a surface (the
back and front buffers tiled); returns the bits. bitsPerPixel 16 or 32;
is555 1 for the 5:5:5 formats. */
void *bdLockSurface(void *surface, int *width, int *height, int *bitsPerPixel, int *is555, int *pitch)
{
	D3DSURFACE_DESC desc;
	D3DLOCKED_RECT locked;
	RECT rect;

	D3DSurface_GetDesc((D3DSurface *)surface, &desc);
	rect.left = 0;
	rect.top = 0;
	rect.right = desc.Width;
	rect.bottom = desc.Height;
	locked.Pitch = 0;
	locked.pBits = NULL;
	D3DSurface_LockRect((D3DSurface *)surface, &locked, &rect,
		surface == DAT_002bc480 || surface == DAT_002bc488 ? 0x40 : 0);
	if (width != NULL)
		*width = desc.Width;
	if (height != NULL)
		*height = desc.Height;
	if (pitch != NULL)
		*pitch = locked.Pitch;
	if (is555 != NULL)
		*is555 = 0;
	switch (desc.Format)
	{
	case 0x02:
	case 0x03:
	case 0x10:
	case 0x1c:
		if (bitsPerPixel != NULL)
			*bitsPerPixel = 0x10;
		if (is555 != NULL)
			*is555 = 1;
		break;
	case 0x05:
	case 0x11:
		if (bitsPerPixel != NULL)
			*bitsPerPixel = 0x10;
		break;
	case 0x06:
	case 0x07:
	case 0x12:
	case 0x1e:
		if (bitsPerPixel != NULL)
			*bitsPerPixel = 0x20;
		break;
	default:
		break;
	}
	return locked.pBits;
}

/* the 0x34 bytes after "BM" */
#pragma pack(push, 2)
typedef struct bmp_header
{
	unsigned int fileSize;  /* the original stores (image bytes + 0x34) >> 2 here */
	unsigned int reserved;
	unsigned int dataOffset;
	unsigned int infoSize;
	int width;
	int height;
	unsigned short planes;
	unsigned short bitCount;
	unsigned int rest[6];
} bmp_header;
#pragma pack(pop)
OF_ASSERT_SIZE(bmp_header, 0x34);

static void write_bmp_header(FILE *file, int width, int height, unsigned int imageBytes)
{
	bmp_header header;

	fputc('B', file);
	fputc('M', file);
	memset(&header, 0, sizeof(header));
	header.fileSize = (imageBytes + 0x34) >> 2;
	header.dataOffset = 0x36;
	header.infoSize = 0x28;
	header.width = width;
	header.height = height;
	header.planes = 1;
	header.bitCount = 0x18;
	fwrite(&header, 1, sizeof(header), file);
}

/* 0x000426e0  BDISPLAY/xbDisplay.cpp: writes a surface as a 24-bit BMP
(bottom row first). The XBE's fwrite is the routine Ghidra names
FID_conflict:_fread. */
int bSaveSurfaceAsBMP(void *surface, const char *fileName)
{
	FILE *file;
	BYTE *bits, *row, *out, *buffer, *table;
	int width, height, bitsPerPixel, is555, pitch, x, y, i, rows;
	int result = 1;
	unsigned int imageBytes;

	if (DAT_002bc440 == 0)
		return 0;
	file = fopen(fileName, "wb");
	if (file == NULL)
	{
		bdPrintf("bSaveSurfaceAsBMP: Could not write file '%s'\n", fileName);
		return 0;
	}
	bits = (BYTE *)bdLockSurface(surface, &width, &height, &bitsPerPixel, &is555, &pitch);
	if (bits == NULL)
	{
		bdPrintf("bSaveSurfaceAsBMP: Could not lock/describe back buffer\n");
		fclose(file);
		return 0;
	}
	if (bitsPerPixel == 0x10)
	{
		/* three 64 KB tables: blue, green, red of each 16-bit pixel */
		table = (BYTE *)bdNew(0x30000, DISPLAY_FILE, 700, 0x2003);
		i = 0xffff;
		do
		{
			if (is555 == 0)
			{
				table[i] = (BYTE)(i >> 11) << 3;
				table[0x10000 + i] = (BYTE)(i >> 5) << 2;
			}
			else
			{
				table[i] = (BYTE)(i >> 10) << 3;
				table[0x10000 + i] = (BYTE)(i >> 5) << 3;
			}
			table[0x20000 + i] = (BYTE)i << 3;
			i--;
		} while (-1 < i);
		imageBytes = width * height * 3;
		write_bmp_header(file, width, height, imageBytes);
		buffer = (BYTE *)bdNew(imageBytes, DISPLAY_FILE, 0x2dd, 0x2003);
		row = bits + (height - 1) * (pitch >> 1) * 2;
		if (0 < height)
		{
			out = buffer;
			y = height;
			do
			{
				for (x = 0; x < width; x++)
				{
					unsigned int pixel = ((WORD *)row)[x];

					out[0] = table[0x20000 + pixel];
					out[1] = table[0x10000 + pixel];
					out[2] = table[pixel];
					out += 3;
				}
				row -= (pitch >> 1) * 2;
				y--;
			} while (y != 0);
		}
		fwrite(buffer, imageBytes, 1, file);
		bdFree(buffer);
		bdFree(table);
	}
	else if (bitsPerPixel == 0x18)
	{
		imageBytes = width * height * 3;
		write_bmp_header(file, width, height, imageBytes);
		buffer = (BYTE *)bdNew(imageBytes, DISPLAY_FILE, 0x301, 0x2003);
		row = bits + (height - 1) * pitch;
		if (0 < height)
		{
			out = buffer;
			y = height;
			do
			{
				BYTE *in = row;

				for (x = 0; x < width; x++)
				{
					out[0] = in[0];
					out[1] = in[1];
					out[2] = in[2];
					out += 3;
					in += 3;
				}
				row -= pitch;
				y--;
			} while (y != 0);
		}
		fwrite(buffer, imageBytes, 1, file);
		bdFree(buffer);
	}
	else if (bitsPerPixel == 0x20)
	{
		write_bmp_header(file, width, height, width * height * 3);
		/* sixteen rows at a time */
		buffer = (BYTE *)bdNew(width * 0x30, DISPLAY_FILE, 0x323, 0x2003);
		row = bits + (height - 1) * pitch;
		y = height;
		while (y != 0)
		{
			rows = 0x10;
			if (y < 0x10)
				rows = y;
			if (0 < rows)
			{
				out = buffer;
				for (i = 0; i < rows; i++)
				{
					BYTE *in = row;

					for (x = 0; x < width; x++)
					{
						out[0] = in[0];
						out[1] = in[1];
						out[2] = in[2];
						out += 3;
						in += 4;
					}
					row -= pitch;
				}
			}
			fwrite(buffer, rows * width * 3, 1, file);
			y -= rows;
		}
		bdFree(buffer);
	}
	else
	{
		bdPrintf("bSaveSurfaceAsBMP: Unsupported pixel format\n");
		result = 0;
	}
	fclose(file);
	return result;
}

/* 0x00042ba0  BDISPLAY/xbDisplay.cpp (inferred): saves the front buffer
as <root folder><name> */
void bdScreenShot(const char *name)
{
	char path[256];

	sprintf(path, "%s%s", DAT_002d0758, name);
	bSaveSurfaceAsBMP(DAT_002bc488, path);
}

/* 0x00042be0  BDISPLAY/xbDisplay.cpp (inferred): clears the screen with a
temporary 640 x 480 device (before the display is opened) */
void bdBlankScreen(void)
{
	D3DPRESENT_PARAMETERS present;
	D3DDevice *device;

	Direct3DCreate8(0);
	memset(&present, 0, sizeof(present));
	present.BackBufferWidth = 0x280;
	present.BackBufferHeight = 0x1e0;
	present.BackBufferFormat = (D3DFORMAT)7;
	present.BackBufferCount = 1;
	present.EnableAutoDepthStencil = 1;
	present.AutoDepthStencilFormat = (D3DFORMAT)0x2a;
	present.SwapEffect = 1;
	Direct3D_CreateDevice(0, (D3DDEVTYPE)1, NULL, 0x40, &present, &device);
	D3DDevice_Clear(0, NULL, 0xf3, 0, 1.0f, 0);
	D3DDevice_Present(NULL, NULL, NULL, NULL);
	D3DDevice_Release();
}

/* 0x00042c70  BDISPLAY/xbDisplay.cpp (inferred): opens the display at
width x height, trying smaller resolutions until one works; the third and
fourth arguments are not used */
int bdOpenDisplay(int width, int height, int unused3, int unused4, int flags)
{
	int started;

	(void)unused3;
	(void)unused4;
	if ((DAT_002bc408 & 1) != 0)
	{
		bdPrintf("bdOpenDisplay: Display is already open\n");
		return 0;
	}
	/* port: display.aspect_ratio (src/d3d8/d3d8_gl.c) can make the screen wider than the
	Xbox's 640 columns; the game asks for 640 x 480 (Core_OpenDisplay). The original
	takes width as given. */
	if (width == 640 && height == 480)
		width = (int)halo_screen_width();
	DAT_002bc40c = width;
	DAT_002bc410 = height;
	DAT_002bc41c = 0x20;
	DAT_002bc420 = 0x18;
	DAT_002bc424 = 8;
	DAT_002bc408 = flags;
	started = bStartDisplay();
	for (;;)
	{
		if (started != 0)
		{
			bdFontInit();
			bdPrintf("Display established at (%d x %d) in 32 bpp with a 24 bit Z and 8 bit stencil\n",
				DAT_002bc40c, DAT_002bc410);
			bdSetNearFar(10.0f, 10000.0f);
			bdSetFieldOfView(bd_bits_float(0x3f9c61aa), bd_bits_float(0x3f6a927f));
			_controlfp(0, 8);
			FUN_0004f930();
			return 1;
		}
		bdPrintf("bdOpenDisplay: *** WARNING *** Failed to start display @ (%d x %d), %d bpp, %d Z\n",
			DAT_002bc40c, DAT_002bc410, DAT_002bc41c, DAT_002bc420);
		if (DAT_002bc40c < 0x501)
		{
			if (DAT_002bc40c < 0x481)
			{
				if (DAT_002bc40c < 0x401)
				{
					if (DAT_002bc40c < 0x321)
					{
						if (DAT_002bc40c < 0x281)
						{
							bdPrintf("bdOpenDisplay: *** FATAL ERROR *** Could not open display in any resolution or depth combination\n");
							return 0;
						}
						DAT_002bc40c = 0x280;
						DAT_002bc410 = 0x1e0;
					}
					else
					{
						DAT_002bc40c = 800;
						DAT_002bc410 = 600;
					}
				}
				else
				{
					DAT_002bc40c = 0x400;
					DAT_002bc410 = 0x300;
				}
			}
			else
			{
				DAT_002bc40c = 0x480;
				DAT_002bc410 = 0x360;
			}
		}
		else
		{
			DAT_002bc40c = 0x500;
			DAT_002bc410 = 0x400;
		}
		started = bStartDisplay();
	}
}
