/*
BSOUND/xbAudioStreamWMA.c: C:\Babel\Xbox\Src\BSOUND\xbAudioStreamWMA.cpp, music streams
compressed with WMA (Packages/Music.xbp and the dashboard's soundtracks). The encoded
file is read in 64 KB blocks into two halves of a 128 KB buffer by the background
loader; the WMA decoder (XDK WMADECXM) pulls from it through a callback and decodes
into a 256 KB looping DirectSound buffer, kept 8 KB-aligned ahead of the write cursor.
0x00056a70 is tagged; the others (0x00056640-0x00057160) are placed here by their
strings ("WMA:", "bStopAudioStreamWMA", ...) and their use of WMA-only fields.
0x00056780 (the decoder's data callback) is missing from the function table.
*/

#include "engine_misc_sound.h"
#include "engine_misc_extern.h"

static const char s_file[] = "C:\\Babel\\Xbox\\Src\\BSOUND\\xbAudioStreamWMA.cpp";

/* 0x002db828, 0x002db82c (.bss): the last request of the decoder (debugging) */
unsigned int DAT_002db828;
int DAT_002db82c;
/* 0x002db830 (.bss): where a request that wraps around the encoded buffer is joined */
unsigned char DAT_002db830[0x100];
/* 0x002db930 (.bss): tick count of the previous decode */
long long DAT_002db930;
/* 0x002db938 (.bss): which of bkernel's fixed heap blocks the next decoder gets */
int DAT_002db938;
/* 0x00285338 (.data, module bkernel; -1): the fixed heap block bkHeapAllocEx returns
while it is not -1. Initialised by bkernel; a common symbol here. */
int DAT_00285338;

/* 0x002d0758 (.bss, module bkernel; a common symbol): the data path, "d:\\" or similar */
char DAT_002d0758[0x100];

/* bDisplay (common symbols): frames per second and the frame counter */
float DAT_002be248;
int DAT_002bc890;

/* 0x00285a90: the background-load event of each stream slot */
const char *const DAT_00285a90[2] = { "_AudioBkgLoad1", "_AudioBkgLoad2" };

#define STREAM_SLOT_REF(slot) (DAT_002db810[(slot)])

/* 0x00056640  BSOUND (xbAudioStreamWMA.cpp?)
Debug output, when DAT_002db824 is set. */
void bsStreamDebug(const char *format, ...)
{
	va_list args;

	if (DAT_002db824 != 0) {
		va_start(args, format);
		em_bkVPrintf(format, args);
		va_end(args);
	}
}

/* 0x00056660  BSOUND (xbAudioStreamWMA.cpp?) (bDeleteAudioStreamWMA) */
void bsDeleteAudioStreamWMA(struct BAudioStream *stream)
{
	struct EmXMediaObject *decoder;

	bsStreamDebug("bDeleteAudioStreamWMA: deleting stream 0x%08x\n", stream);
	bsStopAudioStream(stream);
	decoder = stream->decoder;
	if (decoder != NULL) {
		decoder->lpVtbl->Release(decoder);
		stream->decoder = NULL;
	}
	if (stream->package == NULL || stream->owned > -1)
		em_bkFree(stream);
}

/* 0x000566b0  BSOUND (xbAudioStreamWMA.cpp?)
Starts the background load of the next 64 KB block into half buf of the encoded
buffer. The original takes the stream in ESI and buf in EAX. */
void bsWmaReadBlock(struct BAudioStream *stream, int buf)
{
	int offset;

	if (stream->block >= stream->numBlocks)
		return;
	if (stream->flags & BSTREAM_LOADPENDING)
		em_bkPrintf("WMA: GetNextBlock: WARNING: attempt to read block %d when previous block has not yet arrived\n",
			stream->block);
	bsStreamDebug("WMA: Reading block %d into buf %d for stream 0x%08x\n", stream->block, buf, stream);
	if (stream->package == NULL)
		offset = stream->block << 0x10;
	else
		offset = ((int *)stream->package)[1] * stream->fileIndex + stream->block * 0x10000;
	if (em_bkBackgroundRead(stream->slot, (char *)stream->encoded + (buf << 0x10), (int)stream->file, 0,
		offset, 0x10000, 0, stream->eventName, 0) != 0) {
		stream->flags |= BSTREAM_LOADPENDING;
		bsStreamDebug("WMA: _LOADPENDING flag set\n");
	}
}

/* 0x00056780  BSOUND (xbAudioStreamWMA.cpp?) (not in the function table)
The WMA decoder's data callback: gives it size bytes at offset of the encoded file,
from the 128 KB double buffer (joining a request that wraps), and starts loading the
next block when it moves into the other half. */
int WINAPI FUN_00056780(void *context, DWORD offset, DWORD size, void **data)
{
	struct BAudioStream *stream = (struct BAudioStream *)context;
	unsigned int pos, first;
	int buf;

	DAT_002db828 = offset;
	pos = offset & 0x1ffff;
	buf = (int)pos >> 0x10;
	DAT_002db82c = (int)size;
	if (pos + size < 0x20001) {
		*data = (char *)stream->encoded + pos;
	} else {
		first = 0x20000 - pos;
		memcpy(DAT_002db830, (char *)stream->encoded + pos, first);
		memcpy(DAT_002db830 + first, (char *)stream->encoded + ((buf ^ 1) << 0x10), size - first);
		*data = DAT_002db830;
	}
	if (buf != stream->lastBuffer) {
		stream->block++;
		bsWmaReadBlock(stream, stream->lastBuffer);
		stream->lastBuffer = buf;
		return (int)size;
	}
	return (int)size;
}

/* 0x00056860  BSOUND (xbAudioStreamWMA.cpp?) (bStopAudioStreamWMA) */
void bsStopAudioStreamWMA(struct BAudioStream *stream)
{
	struct EmXMediaObject *decoder;

	bsStreamDebug("bStopAudioStreamWMA: stopping stream 0x%08x #1\n", stream);
	if (stream->slot == 4) {
		bsStreamDebug("bStopAudioStreamWMA: bugging out, channel is invalid\n");
		return;
	}
	if (stream->flags & BSTREAM_LOADPENDING) {
		em_bkPrintf("bStopAudioStreamWMA: *** Blocking until background load has completed ***\n");
		while (em_bkEventTest(stream->event, 0) == 0)
			Sleep(0);
	}
	bsStreamDebug("bStopAudioStreamWMA: no load pending\n");
	decoder = stream->decoder;
	if (decoder != NULL) {
		decoder->lpVtbl->Discontinuity(decoder);
		stream->decoder->lpVtbl->Release(stream->decoder);
		stream->decoder = NULL;
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
	if ((stream->flags & BSTREAM_KEEPBUFFERS) == 0) {
		if (stream->pcm != NULL) {
			em_bkFree(stream->pcm);
			stream->pcm = NULL;
		}
		if (stream->encoded != NULL) {
			em_bkFree(stream->encoded);
			stream->encoded = NULL;
		}
	}
	STREAM_SLOT_REF(stream->slot) = NULL;
	DAT_002db81c--;
	stream->slot = 4;
	bsStreamDebug("bStopAudioStreamWMA: stopped\n");
}

/* 0x000569b0  BSOUND (xbAudioStreamWMA.cpp?)
Creates the stream's 16-bit PCM DirectSound buffer. The original takes the stream in
EDX. */
int bsWmaCreateBuffer(struct BAudioStream *stream)
{
	WAVEFORMATEX format;
	DSBUFFERDESC desc;
	HRESULT hr;
	int bits;

	memset(&desc, 0, sizeof(desc));
	bits = stream->bits;
	desc.dwBufferBytes = 0;
	desc.lpwfxFormat = &format;
	desc.dwSize = 0x18;
	desc.dwFlags = 0x1a0;
	format.nChannels = (unsigned short)(((stream->flags & BSTREAM_STEREO) != 0) + 1);
	format.nBlockAlign = (unsigned short)((int)(bits * (unsigned int)format.nChannels) >> 3);
	format.nSamplesPerSec = stream->frequency;
	format.nAvgBytesPerSec = (int)(stream->frequency * bits * (unsigned int)format.nChannels) >> 3;
	format.cbSize = 0x12;
	format.wBitsPerSample = (unsigned short)bits;
	format.wFormatTag = WAVE_FORMAT_PCM;
	hr = IDirectSound_CreateSoundBuffer(DAT_002db808, &desc, &stream->buffer, NULL);
	if (hr < 0) {
		em_bkPrintf("bAudioStreamCreateSoundBuffer: Could not create sound buffer '%s'\n", em_bdErrorString(hr));
		return 0;
	}
	return 1;
}

/* 0x00056a70  BSOUND/xbAudioStreamWMA.cpp (bPlayAudioStreamWMA)
Takes a free stream slot, creates the buffer, opens the file (the package's .xbp, or
the given file) and starts loading the first block; decoding starts when it arrives
(bsWmaBlockLoaded). */
int bsPlayAudioStreamWMA(struct BAudioStream *stream, unsigned int flags, int volume, int pan, int frequency,
	void *callback, void *file)
{
	char path[0x100];
	int i;

	(void)flags;
	bsStreamDebug("bPlayAudioStreamWMA: trying to play stream 0x%08x\n", stream);
	bsStopAudioStream(stream);
	if (DAT_002db81c == 2) {
		em_bkPrintf("bPlayAudioStreamADPC: The maximum number of streams are already playing\n");
		return 0;
	}
	for (i = 0; i < 2; i++) {
		if (STREAM_SLOT_REF(1 + i) == NULL) {
			stream->slotIndex = i;
			stream->slot = i + 1;
			stream->eventName = DAT_00285a90[i];
			break;
		}
	}
	if (bsWmaCreateBuffer(stream) == 0)
		return 0;
	em_bkEventReset(stream->eventName);
	em_bkCreateEvent(stream->eventName);
	stream->event = em_bkEventWaitHandle(stream->eventName, 4, 0);
	if ((stream->flags & BSTREAM_KEEPBUFFERS) == 0) {
		stream->pcm = em_bkAllocNamed(0x40000, s_file, 0x266, 0x2001, "WMA stream PCM data", 0);
		memset(stream->pcm, 0, 0x40000);
		stream->encoded = em_bkAllocNamed(0x20000, s_file, 0x268, 0x2001, "WMA stream encoded data", 0);
	} else {
		memset(stream->pcm, 0, 0x40000);
	}
	IDirectSoundBuffer_SetBufferData(stream->buffer, stream->pcm, 0x40000);
	if (stream->package == NULL) {
		stream->file = file;
	} else {
		strcpy(path, DAT_002d0758);
		strcat(path, (char *)stream->package + 0x40);
		strcat(path, ".xbp");
		stream->file = CreateFileA(path, 0x80000000, 1, NULL, 3, 0x20000080, NULL);
		if (stream->file == INVALID_HANDLE_VALUE) {
			em_bkPrintf("bPlayAudioStreamWMA: could not open file '%s' for background loading\n", path);
			stream->file = NULL;
			return 0;
		}
	}
	stream->block = 0;
	stream->half = 0;
	bsWmaReadBlock(stream, 0);
	IDirectSoundBuffer_SetVolume(stream->buffer, DAT_00285658[(unsigned int)(DAT_00285a8c * volume) / 0xff]);
	if (frequency == 0) {
		stream->playFrequency = stream->frequency;
	} else {
		IDirectSoundBuffer_SetFrequency(stream->buffer, frequency);
		stream->playFrequency = frequency;
	}
	stream->callback = (void (*)(struct BAudioStream *, int))callback;
	stream->pan = pan;
	stream->volume = volume;
	stream->state = 3;
	STREAM_SLOT_REF(stream->slot) = stream;
	DAT_002db81c++;
	bsStreamDebug("bPlayAudioStreamWMA: Playing stream 0x%08x, channel %d, event '%s'\n", stream,
		stream->slot - 1, stream->eventName);
	return 1;
}

/* 0x00056d20  BSOUND (xbAudioStreamWMA.cpp?)
Decodes into the play buffer up to 8 KB past (and 0x2e000 bytes ahead of) the write
cursor. Once the decoder runs dry the rest is silence; 40 calls later the stream stops
(or restarts when it loops). */
void bsWmaDecode(struct BAudioStream *stream)
{
	struct EmXMediaPacket packet;
	struct EmXMediaObject *decoder;
	long long now;
	DWORD writeCursor, completed;
	int write, next, target, n;

	completed = 0;
	now = em_bkGetTicks();
	IDirectSoundBuffer_GetCurrentPosition(stream->buffer, NULL, &writeCursor);
	write = (((int)writeCursor + 3) / 4) * 4 % 0x40000;
	next = stream->decodeWritePos;
	if ((write > next && write - next < 40000) || next - write > 210000)
		em_bkPrintf("WMA: DecodeStream: *** Write cursor overrun! 0x%08x writeCur %d, nextDecodeWritePos %d (%.2f fps, %I64d ticks)\n",
			stream, write, next, (double)DAT_002be248, now - DAT_002db930);
	target = ((write + 0x2efff) / 0x2000) * 0x2000 % 0x40000;
	DAT_002db930 = now;
	next = stream->decodeWritePos;
	if (target == next)
		return;
	if (target > next) {
		n = target - next;
		if (stream->endCount == 0) {
			packet.pvBuffer = (char *)stream->pcm + next;
			packet.dwMaxSize = (DWORD)n;
			packet.pdwCompletedSize = &completed;
			packet.pdwStatus = NULL;
			packet.hCompletionEvent = (HANDLE)stream;
			packet.prtTimestamp = NULL;
			decoder = stream->decoder;
			decoder->lpVtbl->Process(decoder, NULL, &packet);
			if ((int)completed < n)
				memset((char *)stream->pcm + stream->decodeWritePos + completed, 0, n - completed);
		} else {
			memset((char *)stream->pcm + next, 0, n);
			stream->endCount++;
		}
		stream->decodeWritePos = (stream->decodeWritePos + n) % 0x40000;
	} else {
		/* wraps around the end of the buffer */
		n = 0x40000 - next;
		if (stream->endCount == 0) {
			packet.pvBuffer = (char *)stream->pcm + next;
			packet.dwMaxSize = (DWORD)n;
			packet.pdwCompletedSize = &completed;
			packet.pdwStatus = NULL;
			packet.hCompletionEvent = (HANDLE)stream;
			packet.prtTimestamp = NULL;
			decoder = stream->decoder;
			decoder->lpVtbl->Process(decoder, NULL, &packet);
			if ((int)completed < n)
				memset((char *)stream->pcm + stream->decodeWritePos + completed, 0, n - completed);
			if (target != 0 && (int)completed == n) {
				packet.pvBuffer = stream->pcm;
				packet.dwMaxSize = (DWORD)target;
				packet.pdwCompletedSize = &completed;
				packet.pdwStatus = NULL;
				packet.hCompletionEvent = (HANDLE)stream;
				packet.prtTimestamp = NULL;
				decoder = stream->decoder;
				n = target;
				decoder->lpVtbl->Process(decoder, NULL, &packet);
				if ((int)completed < target)
					memset((char *)stream->pcm + completed, 0, target - completed);
			}
		} else {
			memset((char *)stream->pcm + next, 0, n);
			if (target != 0) {
				memset(stream->pcm, 0, target);
				n = target;
			}
			stream->endCount++;
		}
		stream->decodeWritePos = target;
	}
	if (stream->endCount == 0) {
		if ((int)completed < n) {
			/* the decoder ran dry: the song has ended */
			stream->decoder->lpVtbl->Discontinuity(stream->decoder);
			stream->endCount = 1;
		}
		return;
	}
	if (stream->endCount == 0x28) {
		bsStopAudioStream(stream);
		if (stream->flags & BSTREAM_LOOP) {
			if (stream->callback != NULL)
				stream->callback(stream, 1);
			bsPlayAudioStreamWMA(stream, stream->flags, stream->volume, stream->pan, stream->playFrequency,
				(void *)stream->callback, NULL);
			return;
		}
		if (stream->callback != NULL)
			stream->callback(stream, 0);
	}
}

/* 0x00057060  BSOUND (xbAudioStreamWMA.cpp?) (BlockLoadedCallback)
The first block has arrived: creates the decoder, decodes the first part, loads the
next block and starts playing (or pauses). Later blocks only clear the pending flag.
The original takes the stream in EAX (and ignores two stack arguments). */
void bsWmaBlockLoaded(struct BAudioStream *stream)
{
	WAVEFORMATEX format;

	bsStreamDebug("WMA: BlockLoadedCallback : finished loading block %d for stream 0x%08x\n", stream->block,
		stream);
	stream->flags &= ~BSTREAM_LOADPENDING;
	if (stream->state != 3)
		return;
	stream->decodeWritePos = 0;
	stream->lastBuffer = 0;
	stream->decoder = NULL;
	DAT_00285338 = DAT_002db938;
	DAT_002db938 ^= 1;
	WmaCreateDecoder(FUN_00056780, stream, 0, &format, &stream->decoder);
	DAT_00285338 = -1;
	IDirectSoundBuffer_SetFrequency(stream->buffer, format.nSamplesPerSec);
	stream->frequency = (int)format.nSamplesPerSec;
	stream->endCount = 0;
	bsWmaDecode(stream);
	em_bkThreadYield();
	stream->block++;
	bsWmaReadBlock(stream, 1);
	if (stream->flags & BSTREAM_STARTPAUSED) {
		stream->state = 2;
		return;
	}
	IDirectSoundBuffer_Play(stream->buffer, 0, 0, 1);
	stream->state = 1;
}

/* 0x00057160  BSOUND (xbAudioStreamWMA.cpp?)
Per frame, one stream slot each frame: handles a finished block load, else decodes. */
void bsWmaUpdate(void)
{
	struct BAudioStream *stream;

	stream = STREAM_SLOT_REF(1 + (DAT_002bc890 & 1));
	if (stream != NULL && stream->kind == 1) {
		if (em_bkEventTest(stream->event, 0) != 0) {
			bsWmaBlockLoaded(stream);
			return;
		}
		if (stream->decoder != NULL)
			bsWmaDecode(stream);
	}
}
