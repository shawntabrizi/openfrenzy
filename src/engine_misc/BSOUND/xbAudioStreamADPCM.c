/*
BSOUND/xbAudioStreamADPCM.c: C:\Babel\Xbox\Src\BSOUND\xbAudioStreamADPCM.cpp, streams
of Xbox ADPCM read from a package: the 252 KB DirectSound buffer is two halves of
0x1f800 bytes; when the play cursor crosses into one half (a DirectSound position
notification, served by the "AudioStreamPlayCursor" thread) the other half is loaded.
0x000575f0 is tagged; 0x00057200-0x000573e0 lie before it and use ADPCM-only fields,
0x00057890 follows it ("bStopAudioStreamADPCM"). 0x000574c0 (block-loaded callback) and
0x00057570 (play-cursor thread) are missing from the function table.
*/

#include "engine_misc_sound.h"
#include "engine_misc_extern.h"

#define HALF_SIZE 0x1f800

/* 0x00057200  BSOUND (xbAudioStreamADPCM.cpp?) */
void bsDeleteAudioStreamADPCM(struct BAudioStream *stream)
{
	bsStopAudioStream(stream);
	if (stream->owned > -1)
		em_bkFree(stream);
}

/* 0x00057220  BSOUND (xbAudioStreamADPCM.cpp?)
Starts loading the next block into the free half of the play buffer (silence after
the end). The original takes the stream in ESI. */
void bsAdpcmReadBlock(struct BAudioStream *stream)
{
	int bytesPerBlock, samples, offset, left, bytes;

	if (stream->block < 0) {
		memset((char *)stream->pcm + stream->half * HALF_SIZE, 0, 0x7e00 * 4);
		return;
	}
	if (stream->block >= stream->numBlocks) {
		if ((stream->flags & BSTREAM_LOOP) == 0) {
			stream->block = -1;
			memset((char *)stream->pcm + stream->half * HALF_SIZE, 0, 0x7e00 * 4);
			return;
		}
		stream->block = 0;
		stream->flags |= BSTREAM_LOADPENDING; /* here: "has looped" */
	}
	if (stream->block == 1 && (stream->flags & BSTREAM_LOADPENDING) && stream->callback != NULL) {
		stream->callback(stream, 1);
		stream->flags &= ~BSTREAM_LOADPENDING;
	}
	bytesPerBlock = (stream->flags & BSTREAM_STEREO) ? 0x48 : 0x24;
	samples = HALF_SIZE / bytesPerBlock;
	if (stream->block == 0)
		stream->samplesLoaded = 0;
	offset = stream->block * HALF_SIZE;
	left = stream->totalSamples - stream->samplesLoaded;
	if (left >= samples) {
		if (em_bkBackgroundRead(stream->slot, (char *)stream->pcm + stream->half * HALF_SIZE, (int)stream->file, 0,
			((int *)stream->package)[1] * stream->fileIndex + offset, HALF_SIZE, 0, stream->eventName, 0) != 0)
			stream->flags |= BSTREAM_LOADPENDING;
		stream->samplesLoaded += samples;
		return;
	}
	bytes = left * bytesPerBlock;
	if (em_bkBackgroundRead(stream->slot, (char *)stream->pcm + stream->half * HALF_SIZE, (int)stream->file, 0,
		((int *)stream->package)[1] * stream->fileIndex + offset, (bytes + 0x7ff) & ~0x7ff, 0, stream->eventName,
		0) != 0)
		stream->flags |= BSTREAM_LOADPENDING;
	memset((char *)stream->pcm + stream->half * HALF_SIZE + bytes, 0, HALF_SIZE - bytes);
	stream->samplesLoaded = stream->totalSamples;
}

/* 0x000573e0  BSOUND (xbAudioStreamADPCM.cpp?)
Creates the stream's Xbox ADPCM DirectSound buffer. */
int bsAdpcmCreateBuffer(struct BAudioStream *stream)
{
	struct {
		WAVEFORMATEX wfx;
		unsigned short wSamplesPerBlock;
	} format;
	DSBUFFERDESC desc;
	HRESULT hr;
	int frequency;

	memset(&desc, 0, sizeof(desc));
	desc.dwBufferBytes = 0;
	frequency = stream->playFrequency;
	desc.lpwfxFormat = &format.wfx;
	format.wfx.cbSize = 2;
	format.wfx.nChannels = (unsigned short)(((stream->flags & BSTREAM_STEREO) != 0) + 1);
	format.wfx.nBlockAlign = (unsigned short)(format.wfx.nChannels * 0x24);
	format.wfx.nAvgBytesPerSec = (frequency / 64) * (unsigned int)format.wfx.nChannels * 0x24;
	desc.dwSize = 0x18;
	desc.dwFlags = 0x1a0;
	format.wfx.nSamplesPerSec = (unsigned int)frequency;
	format.wfx.wBitsPerSample = 4;
	format.wfx.wFormatTag = WAVE_FORMAT_XBOX_ADPCM;
	format.wSamplesPerBlock = 0x40;
	hr = IDirectSound_CreateSoundBuffer(DAT_002db808, &desc, &stream->buffer, NULL);
	if (hr < 0) {
		em_bkPrintf("bAudioStreamCreateSoundBuffer: Could not create sound buffer '%s'\n", em_bdErrorString(hr));
		return 0;
	}
	return 1;
}

/* 0x000574c0  BSOUND (xbAudioStreamADPCM.cpp?) (not in the function table)
The background loader's callback when a half has arrived: the first one starts
playback (or pauses) and arms the play-cursor notification. */
void FUN_000574c0(const char *eventName, int unused, struct BAudioStream *stream)
{
	struct EmDSBPositionNotify notify;

	(void)eventName;
	(void)unused;
	stream->flags &= ~BSTREAM_LOADPENDING;
	if (stream->state == 3) {
		if (stream->flags & BSTREAM_STARTPAUSED) {
			stream->state = 2;
		} else {
			IDirectSoundBuffer_Play(stream->buffer, 0, 0, 1);
			stream->state = 1;
		}
		notify.dwOffset = HALF_SIZE;
		notify.hEventNotify = (HANDLE)stream->thread;
		IDirectSoundBuffer_SetNotificationPositions(stream->buffer, 1, &notify);
		stream->notifyHalf = 1;
		stream->block++;
		stream->half = (stream->half + 1) % 2;
		bsAdpcmReadBlock(stream);
		return;
	}
	stream->half = (stream->half + 1) % 2;
}

/* 0x00057570  BSOUND (xbAudioStreamADPCM.cpp?) (not in the function table)
The "AudioStreamPlayCursor" thread's function, run when the play cursor reaches the
notified position: re-arms the notification at the other half and loads the next block;
after the end it stops the stream. */
int FUN_00057570(struct BAudioStream *stream)
{
	struct EmDSBPositionNotify notify;

	if (stream->block == -1) {
		bsStopAudioStream(stream);
		if (stream->callback != NULL) {
			stream->callback(stream, 0);
			return 1;
		}
	} else {
		stream->notifyHalf = (stream->notifyHalf - 1) & 1;
		notify.dwOffset = (DWORD)(stream->notifyHalf * HALF_SIZE);
		notify.hEventNotify = (HANDLE)stream->thread;
		IDirectSoundBuffer_SetNotificationPositions(stream->buffer, 1, &notify);
		stream->block++;
		bsAdpcmReadBlock(stream);
	}
	return 1;
}

/* 0x000575f0  BSOUND/xbAudioStreamADPCM.cpp (bPlayAudioStream) */
int bsPlayAudioStreamADPCM(struct BAudioStream *stream, unsigned int flags, int volume, int pan, int frequency,
	void *callback)
{
	char path[0x100];
	void *pcm;
	int i;

	(void)flags;
	bsStopAudioStream(stream);
	if (DAT_002db81c == 2) {
		em_bkPrintf("bPlayAudioStreamADPC: The maximum number of streams are already playing\n");
		return 0;
	}
	for (i = 0; i < 2; i++) {
		if (DAT_002db810[1 + i] == NULL) {
			stream->slotIndex = i;
			stream->slot = i + 1;
			stream->eventName = DAT_00285a90[i];
			break;
		}
	}
	em_bkEventSetup(stream->eventName);
	stream->event = em_bkEventCallback(stream->eventName, (void *)FUN_000574c0, stream);
	if (bsAdpcmCreateBuffer(stream) == 0)
		return 0;
	if ((stream->flags & BSTREAM_KEEPBUFFERS) == 0) {
		/* the original passes the group (0x24) again as a sixth argument */
		pcm = em_bkAllocNamed(0x3f000, "C:\\Babel\\Xbox\\Src\\BSOUND\\xbAudioStreamADPCM.cpp", 0x15a, 0x2001,
			em_bkGroupName(0x24), 0x24);
		stream->pcm = pcm;
		if (pcm == NULL) {
			em_bkPrintf("bPlayAudioStream: out of memory\n");
			IDirectSoundBuffer_Release(stream->buffer);
			stream->buffer = NULL;
			return 0;
		}
	} else {
		pcm = stream->pcm;
	}
	memset(pcm, 0, 0xfc00 * 4);
	IDirectSoundBuffer_SetBufferData(stream->buffer, stream->pcm, 0x3f000);
	strcpy(path, DAT_002d0758);
	strcat(path, (char *)stream->package + 0x40);
	strcat(path, ".xbp");
	stream->file = CreateFileA(path, 0x80000000, 1, NULL, 3, 0x20000080, NULL);
	if (stream->file == INVALID_HANDLE_VALUE) {
		em_bkPrintf("bPlayAudioStream: could not open file '%s' for background loading\n", path);
		stream->file = NULL;
		IDirectSoundBuffer_Release(stream->buffer);
		stream->buffer = NULL;
		if ((stream->flags & BSTREAM_KEEPBUFFERS) == 0) {
			em_bkFree(stream->pcm);
			stream->pcm = NULL;
		}
		return 0;
	}
	stream->block = 0;
	stream->half = 0;
	stream->samplesLoaded = 0;
	stream->thread = em_bkCreateThread((void *)FUN_00057570, stream, "AudioStreamPlayCursor");
	stream->state = 3;
	IDirectSoundBuffer_SetVolume(stream->buffer, DAT_00285658[(unsigned int)(DAT_00285a8c * volume) / 0xff]);
	if (frequency == 0) {
		stream->playFrequency = stream->frequency;
	} else {
		IDirectSoundBuffer_SetFrequency(stream->buffer, frequency);
		stream->playFrequency = frequency;
	}
	stream->pan = pan;
	stream->volume = volume;
	stream->callback = (void (*)(struct BAudioStream *, int))callback;
	DAT_002db81c++;
	DAT_002db810[stream->slot] = stream;
	bsAdpcmReadBlock(stream);
	return 1;
}

/* 0x00057890  BSOUND (xbAudioStreamADPCM.cpp?) (bStopAudioStreamADPCM) */
void bsStopAudioStreamADPCM(struct BAudioStream *stream)
{
	if (stream->slot == 4)
		return;
	if (stream->flags & BSTREAM_LOADPENDING) {
		em_bkPrintf("bStopAudioStreamADPCM: *** Blocking until background load has completed ***\n");
		while (stream->flags & BSTREAM_LOADPENDING)
			Sleep(0);
	}
	if (stream->buffer != NULL) {
		IDirectSoundBuffer_Stop(stream->buffer);
		if (stream->buffer != NULL) {
			while (IDirectSoundBuffer_Release(stream->buffer) != 0)
				;
			stream->buffer = NULL;
		}
	}
	if (stream->thread != 0) {
		em_bkKillThread(stream->thread);
		stream->thread = 0;
	}
	stream->state = 0;
	if (stream->event != 0) {
		em_bkEventDelete(stream->event);
		stream->event = 0;
	}
	if (stream->file != NULL) {
		CloseHandle(stream->file);
		stream->file = NULL;
	}
	if ((stream->flags & BSTREAM_KEEPBUFFERS) == 0 && stream->pcm != NULL) {
		em_bkFree(stream->pcm);
		stream->pcm = NULL;
	}
	DAT_002db810[stream->slot] = NULL;
	DAT_002db81c--;
	stream->slot = 4;
}
