/*
XBVIDEO.C  <-  C:\Babel\Xbox\Src\BDISPLAY\xbVideo.cpp

Bink video playback into a texture. 0x00046390-0x00046c86. The file name
is the one the allocator callback passes (0x00259344). In the port, Bink is
replaced by src/dsound/bink_null.c: BinkOpen fails, so bdVideoPlayStream
returns 6 and the game skips the video.
*/

#include "bdisplay_internal.h"

#define VIDEO_FILE "C:\\Babel\\Xbox\\Src\\BDISPLAY\\xbVideo.cpp"

/* the BINK structure's first fields */
typedef struct bdBink
{
	unsigned int width;
	unsigned int height;
	unsigned int frames;
	unsigned int frameNum; /* playback stops when it is 0 */
} bdBink;

/* what the callback's info points at */
typedef struct bdVideoInfo
{
	unsigned int width;
	unsigned int height;
	unsigned int frames;
} bdVideoInfo;

typedef struct bdVideoFrameInfo
{
	bdTexture *texture;
	unsigned int width;
	unsigned int height;
	unsigned int frames;
} bdVideoFrameInfo;

/* callback(reason, frame, info, context): reason 0 before playback (0 to
refuse), 2 before each frame is drawn (0: do not draw it), 3 after each
frame (0: stop), 1 after playback */
typedef int (*bdVideoCallback)(int reason, int frame, void *info, void *context);

/* 0x00046390  BDISPLAY/xbVideo.cpp: Bink's allocator (not in the function
table; found through bdVideoPlayStream's RADSetMemory call) */
void *__stdcall bdVideoAlloc(unsigned long size)
{
	return bdNew((int)size, VIDEO_FILE, 0x1d, 0x2003);
}

/* 0x000463b0  BDISPLAY/xbVideo.cpp: Bink's free (not in the function table) */
void __stdcall bdVideoFree(void *memory)
{
	bdFree(memory);
}

/* waits until bKernel's background loading is paused (1) or resumed (0) */
static void pause_background_load(int pause)
{
	while (FUN_0004cf90(pause) == 0)
		Sleep(0);
}

/* 0x000463c0  BDISPLAY/xbVideo.cpp: plays a Bink file full screen. flags:
1 no input map (no skipping by button), 2 print a summary, 4 do not hand
the input map to the GUI's slot (DAT_002cc1b4), 8 stop on memory unit or
controller changes. Returns 0 at the end, 1 memory unit change, 2
controller change, 3 a button was pressed, 4 the callback stopped it, 5 the
callback refused to start, 6 the file or the texture failed. The part from
0x00046b20 on (the summary and the clean-up) is this function too. */
int bdVideoPlayStream(const char *fileName, unsigned int flags, void *callback, void *context)
{
	bdVideoCallback notify = (bdVideoCallback)callback;
	bdBink *bink;
	bdTexture *texture;
	bdTLVertex quad[4];
	unsigned int savedStates[BD_NUM_STATES * 2];
	void *channels[8];
	unsigned int summary[31];
	void *map = NULL;
	void *memoryCardEvents = NULL, *controllerEvents = NULL;
	bdVideoInfo startInfo, endInfo, frameEndInfo;
	bdVideoFrameInfo frameInfo;
	unsigned int width, height, noInput, events, input, colour;
	int pitch, result, frame, pad;
	unsigned int format;
	float w, h, halfW, halfH;
	void *bits;

	pause_background_load(1);
	RADSetMemory(bdVideoAlloc, bdVideoFree);
	result = 0;
	bink = (bdBink *)BinkOpen(fileName, 0);
	if (bink == NULL)
	{
		pause_background_load(0);
		bdPrintf("bdVideoPlayStream: *** ERROR - BinkOpen failed on file '%s' ***\n", fileName);
		return 6;
	}
	w = (float)bink->width;
	h = (float)bink->height;
	halfW = w * 0.5f;
	halfH = h * 0.5f;
	texture = bdCreateTexture((int)bink->width, (int)bink->height, 1, 1, 0, 0);
	if (texture == NULL)
	{
		BinkClose((HBINK)bink);
		pause_background_load(0);
		bdPrintf("bdVideoPlayStream: *** ERROR - Could not create texture for file '%s' ***\n", fileName);
		return 6;
	}

	/* the frame, centred on a 640 x 480 screen, in texels (linear texture) */
	colour = DAT_002bc898[128] | DAT_002bd640[128] | DAT_002bc490[128] | DAT_002bce18[128];
	quad[0].x = 320.0f - halfW;
	quad[0].y = 240.0f - halfH;
	quad[0].z = 0.1f;
	quad[0].rhw = 1.0f;
	quad[0].color = colour;
	quad[0].u = 0.0f;
	quad[0].v = 0.0f;
	quad[1].x = halfW + 320.0f;
	quad[1].y = quad[0].y;
	quad[1].z = 0.1f;
	quad[1].rhw = 1.0f;
	quad[1].color = colour;
	quad[1].u = w;
	quad[1].v = 0.0f;
	quad[2].x = quad[0].x;
	quad[2].y = halfH + 240.0f;
	quad[2].z = 0.1f;
	quad[2].rhw = 1.0f;
	quad[2].color = colour;
	quad[2].u = 0.0f;
	quad[2].v = h;
	quad[3].x = quad[1].x;
	quad[3].y = quad[2].y;
	quad[3].z = 0.1f;
	quad[3].rhw = 1.0f;
	quad[3].color = colour;
	quad[3].u = w;
	quad[3].v = h;

	bdSaveRenderStates(savedStates);
	bdSetRenderState(2, 0xc, 0);
	bdSetRenderState(7, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(8, 0, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(3, 0, 0);
	D3DDevice_SetRenderState(D3DRS_ALPHATESTENABLE, 0);
	bdSetTexture(0, texture);
	bdClearTexture(1);

	/* any button on any controller stops the video */
	noInput = flags & 1;
	if (noInput == 0)
	{
		map = (void *)FUN_000488c0(8);
		for (pad = 0; pad < 4; pad++)
		{
			input = (unsigned int)pad << 16;
			channels[pad * 2] = (void *)FUN_00048a10(map, "Quit", 1, 1, 0, 0, 0);
			FUN_00048b20(channels[pad * 2], input | 0xc5, 0);
			FUN_00048b20(channels[pad * 2], input | 0xc6, 0);
			FUN_00048b20(channels[pad * 2], input | 0xc7, 0);
			FUN_00048b20(channels[pad * 2], input | 0xc8, 0);
			FUN_00048b20(channels[pad * 2], input | 0xc9, 0);
			FUN_00048b20(channels[pad * 2], input | 0xca, 0);
			FUN_00048d40(channels[pad * 2]);
			channels[pad * 2 + 1] = (void *)FUN_00048a10(map, "Quit", 1, 1, 0, 0, 0);
			FUN_00048b20(channels[pad * 2 + 1], input | 0xc1, 0);
			FUN_00048b20(channels[pad * 2 + 1], input | 0xc2, 0);
			FUN_00048b20(channels[pad * 2 + 1], input | 0xcb, 0);
			FUN_00048b20(channels[pad * 2 + 1], input | 0xcc, 0);
			FUN_00048b20(channels[pad * 2 + 1], input | 0xc3, 0);
			FUN_00048b20(channels[pad * 2 + 1], input | 0xc4, 0);
			FUN_00048d40(channels[pad * 2 + 1]);
		}
		if ((flags & 4) == 0)
			DAT_002cc1b4 = (struct BInputMap *)map;
	}
	events = flags & 8;
	if (events != 0)
	{
		memoryCardEvents = (void *)FUN_0004d620("_MemCardChange", 1, 0);
		controllerEvents = (void *)FUN_0004d620("_PadChange", 1, 0);
	}

	if (notify != NULL)
	{
		startInfo.width = bink->width;
		startInfo.height = bink->height;
		startInfo.frames = bink->frames;
		if (notify(0, 0, &startInfo, context) == 0)
		{
			result = 5;
			goto done;
		}
		frameEndInfo.width = bink->width;
		frameEndInfo.height = bink->height;
		frameEndInfo.frames = bink->frames;
		frameInfo.width = bink->width;
		frameInfo.height = bink->height;
		frameInfo.frames = bink->frames;
		endInfo.width = bink->width;
		endInfo.height = bink->height;
		endInfo.frames = bink->frames;
	}

	frame = 0;
	if (0 < (int)bink->frames)
	{
		for (;;)
		{
			BinkDoFrame((HBINK)bink);
			bits = bdTextureLock(texture, 0, &width, &height, &pitch, &format, NULL);
			BinkCopyToBuffer((HBINK)bink, bits, pitch, height, 0, 0, 3);
			bdTextureUnlock(texture);
			if (notify == NULL)
			{
				bdDrawPrimitiveTL(BDPRIMTYPE_TRISTRIP, quad, 4);
			}
			else
			{
				frameInfo.texture = texture;
				if (notify(2, frame, &frameInfo, context) != 0)
					bdDrawPrimitiveTL(BDPRIMTYPE_TRISTRIP, quad, 4);
			}
			BinkNextFrame((HBINK)bink);
			bdFlip(0, 0, 0, 0xff);
			while (BinkWait((HBINK)bink) != 0)
				Sleep(0);
			FUN_00048d60();
			if (noInput == 0)
			{
				for (pad = 0; pad < 4; pad++)
				{
					if (*(int *)((BYTE *)channels[pad * 2] + 0x14) != 0 ||
						*(int *)((BYTE *)channels[pad * 2 + 1] + 0x14) != 0)
						break;
				}
				if (pad < 4)
				{
					result = 3;
					break;
				}
				if (events != 0)
				{
					if (FUN_0004d6d0(memoryCardEvents, NULL) != 0)
					{
						result = 1;
						break;
					}
					if (FUN_0004d6d0(controllerEvents, NULL) != 0)
					{
						result = 2;
						break;
					}
				}
			}
			if (notify != NULL && notify(3, frame, &frameEndInfo, context) == 0)
			{
				result = 4;
				break;
			}
			if (bink->frameNum == 0)
				break;
			frame++;
			if ((int)bink->frames <= frame)
				break;
		}
	}

done:
	if (events != 0)
	{
		FUN_0004d860(memoryCardEvents);
		FUN_0004d860(controllerEvents);
	}
	if (notify != NULL)
		notify(1, 0, &endInfo, context);
	if ((flags & 2) != 0)
	{
		memset(summary, 0, sizeof(summary));
		BinkGetSummary((HBINK)bink, summary);
		bdPrintf("\nBink playback summary for '%s'\n", fileName);
		bdPrintf("  Dimensions : %d x %d\n", summary[0], summary[1]);
		bdPrintf("  Total playback time : %d ms\n", summary[2]);
		bdPrintf("  Desired rate : %.2f fps\n", (double)((float)summary[5] / (float)summary[6]));
		bdPrintf("  Actual rate : %.2f fps\n", (double)(((float)summary[9] * 1000.0f) / (float)summary[2]));
		bdPrintf("  Number of frames : %d (%d skipped)\n", summary[8], summary[10]);
		bdPrintf("  Device read speed :\t%.2f Kb/s\n", (double)((float)summary[19] * bd_bits_float(0x3a800000)));
		bdPrintf("  Highest memory used : %d Kb\n", summary[26] >> 10);
	}
	BinkClose((HBINK)bink);
	bdClearTexture(0);
	bdRestoreRenderStates(savedStates);
	D3DDevice_SetRenderState(D3DRS_ALPHATESTENABLE, 1);
	bdTextureDelete(texture);
	if (noInput == 0)
	{
		FUN_00048970(map);
		if ((flags & 4) == 0)
			DAT_002cc1b4 = NULL;
	}
	pause_background_load(0);
	return result;
}

/* 0x00046b20  BDISPLAY/xbVideo.cpp: not a function. Ghidra started one
here, inside bdVideoPlayStream (its summary and clean-up, written above);
nothing calls it. STUB */
undefined4 FUN_00046b20(void)
{
	/* STUB: see above */
	of_unimplemented(0x00046b20, "FUN_00046b20 (not a function: the middle of bdVideoPlayStream)");
	return 0;
}
