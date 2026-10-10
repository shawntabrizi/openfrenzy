/*
BSOUND/xbAudioStream.c: C:\Babel\Xbox\Src\BSOUND\xbAudioStream.cpp, the stream API
shared by ADPCM (xbAudioStreamADPCM.c) and WMA (xbAudioStreamWMA.c) streams: each call
dispatches on the stream's kind. 0x00056270 is tagged; 0x000562d0-0x000565e0 follow it
(the gap before the WMA file) and are placed here because they are kind-independent.
*/

#include <stdarg.h>

#include "engine_misc_sound.h"
#include "engine_misc_extern.h"

/* 0x002db81c, 0x002db820, 0x002db824 (.bss) */
int DAT_002db81c;          /* streams playing */
void *DAT_002db820;        /* the WMA decoder workspace */
int DAT_002db824;          /* 1: stream debug output (xbAudioStreamWMA.c) */
/* 0x00285a8c: music volume */
int DAT_00285a8c = 0xff;

#define STREAM_SLOT(i) (DAT_002db810[1 + (i)])

/* 0x00056270  BSOUND/xbAudioStream.cpp */
int bsInitAudioStreams(void)
{
	STREAM_SLOT(0) = NULL;
	DAT_002db81c = 0;
	STREAM_SLOT(1) = NULL;
	em_bkCreateEvent("_AudioBkgLoad1");
	em_bkCreateEvent("_AudioBkgLoad2");
	DAT_002db820 = em_bkAllocNamed(0x200, "C:\\Babel\\Xbox\\Src\\BSOUND\\xbAudioStream.cpp", 0x42, 0x2001,
		"WMA Decoder workspace", 0);
	return 1;
}

/* 0x000562d0  BSOUND (xbAudioStream.cpp?)
Fixes up a stream resource found in a package. */
void bsAudioStreamFixup(int package, int name)
{
	struct BAudioStream *stream;

	stream = (struct BAudioStream *)em_bkFindResource(package, name, 0, 0, 0, 0);
	if (stream == NULL)
		return;
	stream->package = (void *)package;
	stream->buffer = NULL;
	if (stream->data90 != NULL)
		stream->data90 = (char *)stream->data90 + (int)stream;
	stream->flags = stream->formatFlags;
	stream->pcm = NULL;
	stream->state = 0;
	stream->flags &= ~BSTREAM_KEEPBUFFERS;
	stream->slot = 4;
}

/* 0x00056340  BSOUND (xbAudioStream.cpp?) (Ghidra: switchD_0004b710::caseD_9, a case of
bkernel's resource-type switch) */
void bsDeleteAudioStream(struct BAudioStream *stream)
{
	if (stream->kind == 0) {
		bsDeleteAudioStreamADPCM(stream);
		return;
	}
	if (stream->kind == 1) {
		bsDeleteAudioStreamWMA(stream);
		return;
	}
}

/* 0x00056370  BSOUND (xbAudioStream.cpp?)
flags: 0x20 start paused, 1 toggles looping. A seventh argument, the open file of a WMA
stream that is not in a package (the play lists'), follows the callback: the original
hands its whole stack frame on to bPlayAudioStreamWMA, which reads it only for such a
stream, so callers with package streams pass six arguments. Here it is variadic and read
only in that case. */
int bsPlayAudioStream(struct BAudioStream *stream, unsigned int flags, int volume, int pan, int frequency,
	void *callback, ...)
{
	va_list args;
	void *file;

	stream->flags = stream->formatFlags;
	if (flags & 0x20)
		stream->flags |= BSTREAM_STARTPAUSED;
	if (flags & 1)
		stream->flags ^= BSTREAM_LOOP;
	if (stream->kind != 0) {
		if (stream->kind != 1)
			return 0;
		file = NULL;
		if (stream->package == NULL) {
			va_start(args, callback);
			file = va_arg(args, void *);
			va_end(args);
		}
		return bsPlayAudioStreamWMA(stream, flags, volume, pan, frequency, callback, file);
	}
	return bsPlayAudioStreamADPCM(stream, flags, volume, pan, frequency, callback);
}

/* 0x000563f0  BSOUND (xbAudioStream.cpp?) */
void bsStopAudioStream(struct BAudioStream *stream)
{
	if (stream->kind == 0) {
		bsStopAudioStreamADPCM(stream);
		return;
	}
	if (stream->kind == 1) {
		bsStopAudioStreamWMA(stream);
		return;
	}
}

/* 0x00056420  BSOUND (xbAudioStream.cpp?) */
int bsAudioStreamTestFlags(struct BAudioStream *stream, unsigned int mask)
{
	return (stream->flags & mask) != 0;
}

/* 0x00056440  BSOUND (xbAudioStream.cpp?) */
void bsGetAudioStreamInfo(struct BAudioStream *stream, int *frequency, int *bits, int *formatFlags,
	int *blockTime, int *field8c, void **data90)
{
	if (frequency != NULL)
		*frequency = stream->frequency;
	if (bits != NULL)
		*bits = stream->bits;
	if (formatFlags != NULL)
		*formatFlags = (int)stream->formatFlags;
	if (blockTime != NULL)
		*blockTime = 4000;
	if (field8c != NULL)
		*field8c = stream->field_8c;
	if (data90 != NULL)
		*data90 = stream->data90;
}

/* 0x000564a0  BSOUND (xbAudioStream.cpp?)
Returns the state (0 stopped, 1 playing, 2 paused, 3 starting). */
int bsGetAudioStreamState(struct BAudioStream *stream, int *volume, int *pan, int *frequency)
{
	if (volume != NULL)
		*volume = stream->volume;
	if (pan != NULL)
		*pan = stream->pan;
	if (frequency != NULL)
		*frequency = stream->playFrequency;
	return stream->state;
}

/* 0x000564e0  BSOUND (xbAudioStream.cpp?) */
int bsSetAudioStreamVolume(struct BAudioStream *stream, int volume, int pan)
{
	if (stream->buffer == NULL)
		return 0;
	IDirectSoundBuffer_SetVolume(stream->buffer,
		DAT_00285658[(unsigned int)(DAT_00285a8c * volume) / 0xff]);
	stream->volume = volume;
	stream->pan = pan;
	return 1;
}

/* 0x00056530  BSOUND (xbAudioStream.cpp?)
Sets the music volume and re-applies it to both stream slots. */
void bsSetMusicVolume(int volume)
{
	struct BAudioStream *stream;
	int i;

	if (DAT_00285a8c != volume) {
		DAT_00285a8c = volume;
		for (i = 0; i < 2; i++) {
			stream = STREAM_SLOT(i);
			if (stream != NULL && stream->buffer != NULL)
				IDirectSoundBuffer_SetVolume(stream->buffer,
					DAT_00285658[(stream->volume * DAT_00285a8c) / 0xff]);
		}
	}
}

/* 0x00056590  BSOUND (xbAudioStream.cpp?)
flags 0x10: pause a playing stream; 1: set looping. Returns 1. */
int bsPauseAudioStream(struct BAudioStream *stream, unsigned char flags)
{
	if ((flags & 0x10) && stream->state == 1) {
		IDirectSoundBuffer_Stop(stream->buffer);
		stream->state = 2;
		stream->flags |= BSTREAM_PAUSED;
	}
	if (flags & 1)
		stream->flags |= BSTREAM_LOOP;
	return 1;
}

/* 0x000565e0  BSOUND (xbAudioStream.cpp?)
flags 0x10: resume a paused stream; 1: clear looping. Returns 1. */
int bsResumeAudioStream(struct BAudioStream *stream, unsigned char flags)
{
	if ((flags & 0x10) && stream->state == 2) {
		IDirectSoundBuffer_Play(stream->buffer, 0, 0, 1);
		stream->state = 1;
		stream->flags &= ~BSTREAM_PAUSED;
	}
	if (flags & 1)
		stream->flags &= ~BSTREAM_LOOP;
	return 1;
}
