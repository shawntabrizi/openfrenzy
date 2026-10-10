/*
BSOUND/unplaced.c: Babel's sample channels (C:\Babel\Xbox\Src\BSOUND\, file not named
by any assert; strings call it bPlaySample / bInitSpecificWave): 60 channels, each an
IDirectSoundBuffer, panned and attenuated in software or positioned by DirectSound's
3D voices. 0x000545f0-0x000559c0.
*/

#include <math.h>

#include "engine_misc_sound.h"
#include "engine_misc_extern.h"
#include "bdisplay_types.h"
#include "bdisplay_globals.h"

/* ---------- globals */

/* 0x002da308 (.bss): the channels */
struct BSoundChannel DAT_002da308[BSOUND_CHANNELS_ALLOC];
/* 0x00285654: channels used */
int DAT_00285654 = 0x3c;
/* 0x00285650: sound effect volume */
int DAT_00285650 = 0xff;
/* 0x00285658: volume 0-255 to DirectSound volume (hundredths of a dB) */
const LONG DAT_00285658[256] = {
#include "volume_table.inc"
};
/* 0x00285a58: distance factor */
float DAT_00285a58 = 1.0f;
/* 0x00285a5c: the I3DL2 listener set at start-up (no reverb) */
static const DSI3DL2LISTENER s_i3dl2Listener = {
	-10000, -10000, 0.0f, 1.0f, 1.0f, -10000, 0.0f, -10000, 0.0f, 0.0f, 0.0f, 5000.0f,
};
/* 0x002db808, 0x002db80c (.bss; other modules read DAT_002db808) */
LPDIRECTSOUND DAT_002db808;
int DAT_002db80c;

/* 0x00285a98: the DSP program image (0x60f8 bytes of Microsoft's standard effects
microcode). It is not copied: the platform's IDirectSound_DownloadEffectsImage ignores
the image. A zero block of the same size is passed. */
static unsigned char s_dspImage[0x60f8];

/* bDisplay's camera (module bdisplay, bdisplay_globals.h; tentative definitions merged
with bdisplay's): the camera rotation and position, which are the listener's.
DAT_002be574/578 are bdisplay's macros for DAT_002be570[1]/[2]. */
float DAT_002be330[16];
float DAT_002be570[3];

#define CHANNEL(i) (&DAT_002da308[i])
#define VOLUME(v) (DAT_00285658[(v)])

/* ---------- functions */

/* 0x000545f0  BSOUND (unplaced) */
int bsInitChannels(void)
{
	memset(DAT_002da308, 0, sizeof(DAT_002da308));
	return 1;
}

/* 0x00054610  BSOUND (unplaced) */
int bsGetNumChannels(void)
{
	return DAT_00285654;
}

/* 0x00054620  BSOUND (unplaced)
Returns how many channels are busy (after updating each one's status). */
int bsCountPlaying(void)
{
	int i, n;

	n = 0;
	for (i = 0; i < DAT_00285654; i++) {
		bsUpdateChannelStatus(i);
		n += CHANNEL(i)->status != 0;
	}
	return n;
}

/* 0x00054660  BSOUND (unplaced)
Reserves (or frees) a channel for the caller's own use. */
void bsReserveChannel(int channel, int reserve)
{
	if (channel > -1 && channel < DAT_00285654) {
		if (reserve != 0) {
			CHANNEL(channel)->flags |= BSCHAN_RESERVED;
			return;
		}
		CHANNEL(channel)->flags &= ~BSCHAN_RESERVED;
	}
}

/* 0x00054690  BSOUND (unplaced)
Sets the sound effect volume and re-applies it to the playing channels. */
void bsSetSfxVolume(int volume)
{
	int i;

	DAT_00285650 = volume;
	for (i = 0; i < DAT_00285654; i++) {
		bsUpdateChannelStatus(i);
		if (CHANNEL(i)->status != 0)
			bsApplyVolume(i);
	}
}

/* 0x000546e0  BSOUND (unplaced) */
void bsStopChannel(int channel)
{
	if (channel > -1 && channel < DAT_00285654) {
		bsUpdateChannelStatus(channel);
		if (CHANNEL(channel)->status != 0) {
			bsReleaseChannel(channel);
			CHANNEL(channel)->status = 0;
			CHANNEL(channel)->sample = NULL;
		}
	}
}

/* 0x00054730  BSOUND (unplaced)
Stops every channel. (0x00054900 is a thunk to it.) The bounds tests are those of
an inlined bsStopChannel. */
void bsStopAll(void)
{
	int i;

	for (i = 0; i < DAT_00285654; i++) {
		bsUpdateChannelStatus(i);
		if (CHANNEL(i)->status != 0 && i < DAT_00285654) {
			bsUpdateChannelStatus(i);
			if (CHANNEL(i)->status != 0) {
				bsReleaseChannel(i);
				CHANNEL(i)->status = 0;
				CHANNEL(i)->sample = NULL;
			}
		}
	}
}

/* 0x000547a0  BSOUND (unplaced)
Stops every channel that plays the sample. */
void bsStopSample(struct BSample *sample)
{
	int i;

	for (i = 0; i < DAT_00285654; i++) {
		if (CHANNEL(i)->sample == sample && i < DAT_00285654) {
			bsUpdateChannelStatus(i);
			if (CHANNEL(i)->status != 0) {
				bsReleaseChannel(i);
				CHANNEL(i)->status = 0;
				CHANNEL(i)->sample = NULL;
			}
		}
	}
}

/* 0x00054800  BSOUND (unplaced)
Pauses (1) or resumes (otherwise) every channel. */
void bsPauseAll(int pause)
{
	int i;

	if (pause == 1) {
		for (i = 0; i < DAT_00285654; i++) {
			bsUpdateChannelStatus(i);
			if (CHANNEL(i)->status == 1) {
				bsPauseChannel(i, 1);
				CHANNEL(i)->status = 2;
			}
		}
		return;
	}
	for (i = 0; i < DAT_00285654; i++) {
		bsUpdateChannelStatus(i);
		if (CHANNEL(i)->status == 2) {
			bsPauseChannel(i, 0);
			CHANNEL(i)->status = 1;
		}
	}
}

/* 0x000548a0  BSOUND (unplaced) */
void bsSetChannelVolume(int channel, unsigned short volume)
{
	if (channel > -1 && channel < DAT_00285654) {
		CHANNEL(channel)->baseVolume = volume;
		CHANNEL(channel)->volume = volume;
		bsApplyVolume(channel);
	}
}

/* 0x000548d0  BSOUND (unplaced) */
int bsGetChannelStatus(int channel)
{
	if (channel > -1 && channel < DAT_00285654) {
		bsUpdateChannelStatus(channel);
		return CHANNEL(channel)->status;
	}
	return 0;
}

/* 0x00054900  BSOUND (unplaced): a jump to bsStopAll */
void bsStopAllThunk(void)
{
	bsStopAll();
}

/* 0x00054910  BSOUND (unplaced)
Software 3D: the volume (0-255) and pan of a sound at pos, heard from the camera. The
original takes pos in ECX and returns pan in [ESI] and volume in [EDI]. */
void bsCalcVolumePan(float *pos, float minDist, float maxDist, int *pan, int *volume)
{
	float dx, dy, dz, dist, len, side, panRange, t;
	float view[4];

	dx = pos[0] - DAT_002be570[0];
	dy = pos[1] - DAT_002be574;
	dz = pos[2] - DAT_002be578;
	dist = sqrtf(dx * dx + dy * dy + dz * dz);
	if (dist > maxDist) {
		*pan = 0;
		*volume = 0;
		return;
	}
	pos[3] = 1.0f;
	em_bdProjectPoint(view, pos, 1);
	len = sqrtf(view[2] * view[2] + view[1] * view[1] + view[0] * view[0]);
	side = len == 0.0f ? 0.0f : view[0] / len;
	panRange = maxDist * 0.25f;
	if (dist < panRange)
		side = side * (dist / panRange);
	*pan = (int)(side * 255.0f);
	t = dist < minDist ? 0.0f : dist - minDist;
	*volume = 0xff - (int)(t * 255.0f / (maxDist - minDist));
}

/* 0x00054a30  BSOUND (unplaced)
Checks the play flags against the channels before a sample is started (the original
takes the sample in EBX). 1: stop the sample's other channels; 2: refuse when the
sample already plays; 4: stop the channels of the sample's group; 8: (writes the group
into every channel's sample and refuses when one is non-zero, as the original does).
Returns 0 to refuse. */
int bsCheckPlayFlags(unsigned char flags, struct BSample *sample)
{
	int i;

	if (flags & 1) {
		for (i = 0; i < DAT_00285654; i++) {
			if (CHANNEL(i)->sample == sample && i < DAT_00285654) {
				bsUpdateChannelStatus(i);
				if (CHANNEL(i)->status != 0) {
					bsReleaseChannel(i);
					CHANNEL(i)->status = 0;
					CHANNEL(i)->sample = NULL;
				}
			}
		}
		return 1;
	}
	if (flags & 2) {
		for (i = 0; i < DAT_00285654; i++) {
			if (CHANNEL(i)->sample == sample)
				return 0;
		}
		return 1;
	}
	if (flags & 4) {
		for (i = 0; i < DAT_00285654; i++) {
			if (CHANNEL(i)->sample->group == sample->group)
				bsStopChannel(i);
		}
		return 1;
	}
	if (flags & 8) {
		for (i = 0; i < DAT_00285654; i++) {
			CHANNEL(i)->sample->group = sample->group;
			if (CHANNEL(i)->sample->group != 0)
				return 0;
		}
	}
	return 1;
}

/* 0x00054b50  BSOUND (unplaced)
Finds a free, unreserved channel; -1 when none or when the flags refuse. The original
takes the flags in EAX and the sample in EBX. */
int bsFindFreeChannel(unsigned int flags, struct BSample *sample)
{
	int i;

	if (bsCheckPlayFlags((unsigned char)flags, sample) == 0)
		return -1;
	for (i = 0; i < DAT_00285654; i++) {
		bsUpdateChannelStatus(i);
		if (CHANNEL(i)->status == 0 && (CHANNEL(i)->flags & BSCHAN_RESERVED) == 0)
			return i;
	}
	return -1;
}

/* the play flags: the sample's defaults toggled by the caller's; 0x20 forces looping
(0x10) on, 0x40 off */
static unsigned int bsMergeFlags(const struct BSample *sample, unsigned int flags)
{
	unsigned int merged;

	merged = sample->flags ^ flags;
	if (flags & 0x20)
		merged |= 0x10;
	else if (flags & 0x40)
		merged &= ~0x10u;
	return merged;
}

/* 0x00054ba0  BSOUND (unplaced)
Plays a sample; returns the channel or -1. */
int bsPlaySample(struct BSample *sample, int volume, int pan, int frequency, unsigned int flags)
{
	unsigned int merged;
	int channel;

	if (volume == 0)
		return -1;
	merged = bsMergeFlags(sample, flags);
	channel = bsFindFreeChannel(merged, sample);
	if (channel != -1 && bsCreateAndPlay(sample, volume, pan, frequency, merged, channel) != 0) {
		CHANNEL(channel)->pan = pan;
		CHANNEL(channel)->baseVolume = (unsigned short)volume;
		CHANNEL(channel)->volume = (unsigned short)volume;
		CHANNEL(channel)->sample = sample;
		CHANNEL(channel)->flags = 0;
		return channel;
	}
	return -1;
}

/* 0x00054c30  BSOUND (unplaced)
Plays a sample at a fixed position: with DirectSound 3D for mono samples once
DirectSound is up, else panned in software. Returns the channel or -1. */
int bsPlaySample3D(struct BSample *sample, float *pos, int volume, int frequency, unsigned int flags,
	float minDist, float maxDist)
{
	unsigned int merged;
	int channel, pan, vol, v;

	if (volume == 0)
		return -1;
	merged = bsMergeFlags(sample, flags);
	channel = bsFindFreeChannel(merged, sample);
	if (channel == -1)
		return -1;
	if (DAT_002db80c == 0 || sample->channels != 1) {
		bsCalcVolumePan(pos, minDist, maxDist, &pan, &vol);
		v = vol * volume >> 8;
		if (v != 0 && bsCreateAndPlay(sample, v, pan, frequency, merged, channel) != 0) {
			CHANNEL(channel)->baseVolume = (unsigned short)volume;
			CHANNEL(channel)->pan = pan;
			CHANNEL(channel)->volume = (unsigned short)((unsigned int)(vol * volume) >> 8);
			CHANNEL(channel)->minDist = minDist;
			CHANNEL(channel)->maxDist = maxDist;
			CHANNEL(channel)->flags = 0;
			CHANNEL(channel)->pos[0] = pos[0];
			CHANNEL(channel)->pos[1] = pos[1];
			CHANNEL(channel)->pos[2] = pos[2];
			CHANNEL(channel)->pos[3] = pos[3];
			CHANNEL(channel)->sample = sample;
			return channel;
		}
	} else if (bsCreateAndPlay3D(sample, pos, volume, frequency, merged, channel, minDist, maxDist) != 0) {
		CHANNEL(channel)->baseVolume = (unsigned short)volume;
		CHANNEL(channel)->volume = (unsigned short)volume;
		CHANNEL(channel)->pan = 0;
		CHANNEL(channel)->minDist = 0.0f;
		CHANNEL(channel)->maxDist = 0.0f;
		CHANNEL(channel)->flags = BSCHAN_HW3D;
		em_bmVecCopy4(CHANNEL(channel)->pos, pos);
		em_bmVecCopy4(CHANNEL(channel)->prevPos, pos);
		CHANNEL(channel)->sample = sample;
		return channel;
	}
	return -1;
}

/* 0x00054e00  BSOUND (unplaced)
Plays a sample that follows a position: with a callback, pos is the start position and
the callback gives the new one each update; without one, pos is followed. */
int bsPlaySampleMoving(struct BSample *sample, float *pos, int volume, int frequency, unsigned int flags,
	void *callback, void *context, float minDist, float maxDist)
{
	unsigned int merged;
	int channel, pan, vol;
	unsigned short vol16;

	if (volume == 0)
		return -1;
	merged = bsMergeFlags(sample, flags);
	channel = bsFindFreeChannel(merged, sample);
	if (channel == -1)
		return -1;
	vol16 = (unsigned short)volume;
	if (DAT_002db80c == 0 || sample->channels != 1) {
		bsCalcVolumePan(pos, minDist, maxDist, &pan, &vol);
		if (bsCreateAndPlay(sample, vol * volume >> 8, pan, frequency, merged, channel) == 0)
			return -1;
		CHANNEL(channel)->baseVolume = vol16;
		CHANNEL(channel)->volume = (unsigned short)((unsigned int)(vol * volume) >> 8);
		CHANNEL(channel)->pan = pan;
		CHANNEL(channel)->minDist = minDist;
		CHANNEL(channel)->maxDist = maxDist;
		CHANNEL(channel)->flags = 0;
	} else {
		if (bsCreateAndPlay3D(sample, pos, volume, frequency, merged, channel, minDist, maxDist) == 0)
			return -1;
		CHANNEL(channel)->pan = 0;
		CHANNEL(channel)->baseVolume = vol16;
		CHANNEL(channel)->volume = vol16;
		CHANNEL(channel)->minDist = minDist;
		CHANNEL(channel)->maxDist = maxDist;
		CHANNEL(channel)->flags = BSCHAN_HW3D;
	}
	CHANNEL(channel)->sample = sample;
	CHANNEL(channel)->flags |= BSCHAN_MOVING;
	CHANNEL(channel)->callback = (BSoundPosCallback)callback;
	CHANNEL(channel)->context = context;
	if (callback == NULL) {
		CHANNEL(channel)->posPtr = pos;
	} else {
		CHANNEL(channel)->pos[0] = pos[0];
		CHANNEL(channel)->pos[1] = pos[1];
		CHANNEL(channel)->pos[2] = pos[2];
		CHANNEL(channel)->pos[3] = pos[3];
	}
	CHANNEL(channel)->prevPos[0] = pos[0];
	CHANNEL(channel)->prevPos[1] = pos[1];
	CHANNEL(channel)->prevPos[2] = pos[2];
	CHANNEL(channel)->prevPos[3] = pos[3];
	return channel;
}

/* 0x00054fe0  BSOUND (unplaced)
Plays a sample on a given channel (stopping what it plays). */
int bsPlaySampleOnChannel(int channel, struct BSample *sample, int volume, int pan, int frequency,
	unsigned int flags)
{
	unsigned int merged, oldFlags;

	if (channel > -1 && channel < DAT_00285654 && volume != 0) {
		merged = bsMergeFlags(sample, flags);
		if (bsCheckPlayFlags((unsigned char)merged, sample) != 0 &&
			bsCreateAndPlay(sample, volume, pan, frequency, merged, channel) != 0) {
			oldFlags = CHANNEL(channel)->flags;
			CHANNEL(channel)->baseVolume = (unsigned short)volume;
			CHANNEL(channel)->volume = (unsigned short)volume;
			CHANNEL(channel)->pan = pan;
			CHANNEL(channel)->sample = sample;
			CHANNEL(channel)->flags = oldFlags & BSCHAN_RESERVED;
			return channel;
		}
	}
	return -1;
}

/* 0x00055090  BSOUND (unplaced)
bsPlaySampleMoving on a given channel. The flag check's result is ignored. */
int bsPlaySampleMovingOnChannel(int channel, struct BSample *sample, float *pos, int volume,
	int frequency, unsigned int flags, void *callback, void *context, float minDist,
	float maxDist)
{
	unsigned int merged, chFlags;
	int pan, vol;

	if (channel < 0 || DAT_00285654 <= channel || volume == 0)
		return -1;
	merged = bsMergeFlags(sample, flags);
	bsCheckPlayFlags((unsigned char)merged, sample);
	if (DAT_002db80c == 0 || sample->channels != 1) {
		bsCalcVolumePan(pos, minDist, maxDist, &pan, &vol);
		if (bsCreateAndPlay(sample, vol * volume >> 8, pan, frequency, merged, channel) == 0)
			return -1;
		CHANNEL(channel)->minDist = minDist;
		chFlags = CHANNEL(channel)->flags;
		CHANNEL(channel)->pan = pan;
		CHANNEL(channel)->volume = (unsigned short)((unsigned int)(vol * volume) >> 8);
		chFlags &= BSCHAN_RESERVED;
	} else {
		if (bsCreateAndPlay3D(sample, pos, volume, frequency, merged, channel, minDist, maxDist) == 0)
			return -1;
		CHANNEL(channel)->minDist = minDist;
		chFlags = CHANNEL(channel)->flags;
		CHANNEL(channel)->pan = 0;
		CHANNEL(channel)->volume = (unsigned short)volume;
		chFlags = (chFlags & BSCHAN_RESERVED) | BSCHAN_HW3D;
	}
	CHANNEL(channel)->maxDist = maxDist;
	CHANNEL(channel)->sample = sample;
	CHANNEL(channel)->flags = chFlags;
	CHANNEL(channel)->baseVolume = (unsigned short)volume;
	CHANNEL(channel)->flags |= BSCHAN_MOVING;
	CHANNEL(channel)->callback = (BSoundPosCallback)callback;
	CHANNEL(channel)->context = context;
	if (callback != NULL) {
		em_bmVecCopy4(CHANNEL(channel)->pos, pos);
		return channel;
	}
	CHANNEL(channel)->posPtr = pos;
	return channel;
}

/* 0x00055240  BSOUND (unplaced)
Per frame: moves the moving sounds (arg is passed to their callbacks), then the
listener. */
void bsUpdate(int arg)
{
	struct BSoundChannel *ch;
	int i, pan, vol;

	for (i = 0; i < DAT_00285654; i++) {
		ch = CHANNEL(i);
		if ((ch->flags & BSCHAN_MOVING) == 0)
			continue;
		bsUpdateChannelStatus(i);
		if (ch->status == 0)
			continue;
		if (ch->flags & BSCHAN_HW3D) {
			if (ch->callback != NULL) {
				ch->callback(ch->pos, arg, ch->context);
			} else {
				ch->pos[0] = ch->posPtr[0];
				ch->pos[1] = ch->posPtr[1];
				ch->pos[2] = ch->posPtr[2];
				ch->pos[3] = ch->posPtr[3];
			}
			bsSetChannelPosition(i, ch->prevPos, ch->pos);
			ch->prevPos[0] = ch->pos[0];
			ch->prevPos[1] = ch->pos[1];
			ch->prevPos[2] = ch->pos[2];
			ch->prevPos[3] = ch->pos[3];
		} else {
			if (ch->callback != NULL) {
				ch->callback(ch->pos, arg, ch->context);
				bsCalcVolumePan(ch->pos, ch->minDist, ch->maxDist, &pan, &vol);
			} else {
				bsCalcVolumePan(ch->posPtr, ch->minDist, ch->maxDist, &pan, &vol);
			}
			ch->pan = pan;
			ch->volume = (unsigned short)((unsigned int)ch->baseVolume * vol >> 8);
			bsApplyVolume(i);
		}
	}
	bsUpdateListener();
}

/* 0x00055390  BSOUND (unplaced)
Fixes up a sample resource found in a package and returns it.
The resource loader (bkernel's table at 0x002852e8) uses the result: the x86 code leaves
the resource (or 0) in EAX, which Ghidra's void prototype hides. */
void *bsSampleFixup(int package, int name)
{
	struct BSample *sample;

	sample = (struct BSample *)em_bkFindResource(package, name, 0, 0, 0, 0);
	if (sample == NULL)
		return NULL;
	sample->data = (char *)sample + sizeof(struct BSample);
	sample->desc.dwSize = 0x18;
	sample->desc.dwFlags = 0xa0;
	sample->desc.dwBufferBytes = 0;
	sample->desc.lpwfxFormat = &sample->format;
	sample->channels = (char)sample->format.nChannels;
	return sample;
}

/* 0x000553e0  BSOUND (unplaced) (Ghidra: switchD_0004b710::caseD_2, a case of
bkernel's resource-type switch)
Deletes a sample: stops it, frees it when it was allocated. */
void bsDeleteSample(struct BSample *sample)
{
	bsStopSample(sample);
	if (sample->owned > -1)
		em_bkFree(sample);
}

/* 0x00055400  BSOUND (unplaced) */
void bsPrintCaps(void)
{
	DSCAPS caps;

	IDirectSound_GetCaps(DAT_002db808, &caps);
	em_bkPrintf("Free2DBuffers = %d, Free3DBuffers = %d, FreeBufferSGEs = %d, MemoryAllocated = %d Kb\n",
		caps.dwFree2DBuffers, caps.dwFree3DBuffers, caps.dwFreeBufferSGEs, caps.dwMemoryAllocated >> 10);
}

/* 0x00055440  BSOUND (unplaced) (bInitSpecificWave) */
int bsInitDirectSound(void)
{
	HRESULT hr;
	DSEFFECTIMAGELOC location;
	LPDSEFFECTIMAGEDESC description;

	DAT_002db808 = NULL;
	DAT_002db80c = 0;
	hr = DirectSoundCreate(NULL, &DAT_002db808, NULL);
	if (hr < 0) {
		em_bkPrintf("bInitSpecificWave: Could not create DirectSound object '%s' (sound has been disabled)\n",
			em_bdErrorString(hr));
		DbgBreakPoint();
		return 0;
	}
	location.dwI3DL2ReverbIndex = 0;
	location.dwCrosstalkIndex = 1;
	hr = IDirectSound_DownloadEffectsImage(DAT_002db808, s_dspImage, sizeof(s_dspImage), &location,
		&description);
	if (hr < 0) {
		em_bkPrintf("bInitSpecificWave: *** FATAL ERROR : Downloading default DSP program failed '%s' ***\n",
			em_bdErrorString(hr));
		DbgBreakPoint();
		return 0;
	}
	em_bkPrintf("bInitSpecificWave: Downloaded default DSP program from static array OK\n");
	DirectSoundUseFullHRTF();
	DAT_002db80c = 1;
	IDirectSound_SetI3DL2Listener(DAT_002db808, &s_i3dl2Listener, 0);
	em_bkPrintf("DirectSound initialised OK\n");
	bsPrintCaps();
	return 1;
}

/* 0x00055520  BSOUND (unplaced) */
void bsShutdownDirectSound(void)
{
	if (DAT_002db808 != NULL) {
		while (IDirectSound_Release(DAT_002db808) != 0)
			;
		DAT_002db808 = NULL;
	}
}

/* 0x00055550  BSOUND (unplaced) (bPlaySample)
Creates the channel's buffer for the sample and plays it. pan is not used. */
int bsCreateAndPlay(struct BSample *sample, int volume, int pan, int frequency, unsigned int flags,
	int channel)
{
	LPDIRECTSOUNDBUFFER *buffer;
	HRESULT hr;
	DSCAPS caps;

	(void)pan;
	buffer = &CHANNEL(channel)->buffer;
	hr = IDirectSound_CreateSoundBuffer(DAT_002db808, &sample->desc, buffer, NULL);
	if (hr < 0) {
		em_bkPrintf("bPlaySample: Could not create sound buffer '%s'\n", em_bdErrorString(hr));
		IDirectSound_GetCaps(DAT_002db808, &caps);
		em_bkPrintf("Free2DBuffers = %d, Free3DBuffers = %d, FreeBufferSGEs = %d, MemoryAllocated = %d Kb\n",
			caps.dwFree2DBuffers, caps.dwFree3DBuffers, caps.dwFreeBufferSGEs,
			caps.dwMemoryAllocated >> 10);
		return 0;
	}
	hr = IDirectSoundBuffer_SetBufferData(*buffer, sample->data, sample->dataSize);
	if (hr < 0) {
		IDirectSoundBuffer_Release(*buffer);
		*buffer = NULL;
		em_bkPrintf("bPlaySample: Could not set data for sound buffer '%s'\n", em_bdErrorString(hr));
		bsPrintCaps();
		return 0;
	}
	IDirectSoundBuffer_SetVolume(*buffer, VOLUME(DAT_00285650 * volume >> 8));
	if (frequency != 0)
		IDirectSoundBuffer_SetFrequency(*buffer, frequency);
	IDirectSoundBuffer_Play(*buffer, 0, 0, flags >> 4 & 1);
	CHANNEL(channel)->playFlags = flags;
	CHANNEL(channel)->status = 1;
	return 1;
}

/* 0x00055680  BSOUND (unplaced) */
void bsApplyVolume(int channel)
{
	if (CHANNEL(channel)->buffer != NULL)
		IDirectSoundBuffer_SetVolume(CHANNEL(channel)->buffer,
			VOLUME((int)((unsigned int)CHANNEL(channel)->volume * DAT_00285650) >> 8));
}

/* 0x000556c0  BSOUND (unplaced) */
void bsSetChannelFrequency(int channel, int frequency)
{
	if (CHANNEL(channel)->buffer != NULL)
		IDirectSoundBuffer_SetFrequency(CHANNEL(channel)->buffer, frequency);
}

/* 0x000556e0  BSOUND (unplaced) */
void bsReleaseChannel(int channel)
{
	LPDIRECTSOUNDBUFFER *buffer;

	buffer = &CHANNEL(channel)->buffer;
	if (*buffer != NULL) {
		IDirectSoundBuffer_Stop(*buffer);
		if (*buffer != NULL) {
			while (IDirectSoundBuffer_Release(*buffer) != 0)
				;
			*buffer = NULL;
		}
	}
}

/* 0x00055720  BSOUND (unplaced)
Frees the buffer of a channel that has finished (a paused one is kept). */
void bsUpdateChannelStatus(int channel)
{
	struct BSoundChannel *ch;
	DWORD status;

	ch = CHANNEL(channel);
	if (ch->buffer != NULL) {
		if (ch->status == 2)
			return;
		IDirectSoundBuffer_GetStatus(ch->buffer, &status);
		if (status & 1)
			return;
		if (ch->buffer != NULL) {
			while (IDirectSoundBuffer_Release(ch->buffer) != 0)
				;
			ch->buffer = NULL;
		}
	}
	ch->status = 0;
}

/* 0x00055790  BSOUND (unplaced) */
void bsPauseChannel(int channel, int pause)
{
	LPDIRECTSOUNDBUFFER buffer;

	buffer = CHANNEL(channel)->buffer;
	if (buffer != NULL) {
		if (pause == 1) {
			IDirectSoundBuffer_Stop(buffer);
			return;
		}
		IDirectSoundBuffer_Play(buffer, 0, 0, CHANNEL(channel)->playFlags >> 4 & 1);
	}
}

/* 0x000557d0  BSOUND (unplaced) (bPlaySample3D)
Creates a DirectSound 3D buffer for a mono sample and plays it at pos. */
int bsCreateAndPlay3D(struct BSample *sample, float *pos, int volume, int frequency, unsigned int flags,
	int channel, float minDist, float maxDist)
{
	LPDIRECTSOUNDBUFFER *buffer;
	HRESULT hr;

	if (sample->format.nChannels != 1) {
		em_bkPrintf("bPlaySample3D: *** ERROR : Attempt to play 3D sound using non-mono sample ***\n");
		DbgBreakPoint();
		return 0;
	}
	buffer = &CHANNEL(channel)->buffer;
	sample->desc.dwFlags |= 0x10;
	hr = IDirectSound_CreateSoundBuffer(DAT_002db808, &sample->desc, buffer, NULL);
	sample->desc.dwFlags &= ~0x10u;
	if (hr < 0) {
		em_bkPrintf("bPlaySample: Could not create sound buffer '%s'\n", em_bdErrorString(hr));
		bsPrintCaps();
		return 0;
	}
	hr = IDirectSoundBuffer_SetBufferData(*buffer, sample->data, sample->dataSize);
	if (hr < 0) {
		IDirectSoundBuffer_Release(*buffer);
		em_bkPrintf("bPlaySample: Could not set data for sound buffer '%s'\n", em_bdErrorString(hr));
		bsPrintCaps();
		return 0;
	}
	IDirectSoundBuffer_SetVolume(*buffer, VOLUME(DAT_00285650 * volume >> 8));
	if (frequency != 0)
		IDirectSoundBuffer_SetFrequency(*buffer, frequency);
	IDirectSoundBuffer_SetPosition(*buffer, pos[0], pos[1], pos[2], 1);
	IDirectSoundBuffer_SetVelocity(*buffer, 0.0f, 0.0f, 0.0f, 1);
	IDirectSoundBuffer_SetMinDistance(*buffer, minDist, 1);
	IDirectSoundBuffer_SetMaxDistance(*buffer, maxDist, 1);
	IDirectSound_CommitDeferredSettings(DAT_002db808);
	IDirectSoundBuffer_Play(*buffer, 0, 0, flags >> 4 & 1);
	CHANNEL(channel)->playFlags = flags;
	CHANNEL(channel)->status = 1;
	return 1;
}

/* 0x00055940  BSOUND (unplaced) */
void bsSetChannelPosition(int channel, float *prevPos, float *pos)
{
	IDirectSoundBuffer_SetPosition(CHANNEL(channel)->buffer, pos[0], pos[1], pos[2], 1);
	IDirectSoundBuffer_SetVelocity(CHANNEL(channel)->buffer, pos[0] - prevPos[0], pos[1] - prevPos[1],
		pos[2] - prevPos[2], 1);
}

/* 0x000559a0  BSOUND (unplaced) */
void bsSetDistanceFactor(float factor)
{
	DAT_00285a58 = factor;
	IDirectSound_SetDistanceFactor(DAT_002db808, factor, 0);
}

/* 0x000559c0  BSOUND (unplaced)
Puts the DirectSound listener at the camera. */
void bsUpdateListener(void)
{
	struct EmDS3DListener listener;
	float in[4], out[4];

	listener.vPosition[0] = DAT_002be570[0];
	listener.vPosition[1] = DAT_002be574;
	listener.dwSize = sizeof(listener);
	listener.vPosition[2] = DAT_002be578;
	listener.vVelocity[0] = 0.0f;
	listener.vVelocity[1] = 0.0f;
	listener.vVelocity[2] = 0.0f;
	in[0] = 0.0f;
	in[1] = 0.0f;
	in[2] = 1.0f;
	in[3] = 1.0f;
	em_bmMatTransformVec(out, DAT_002be330, in);
	listener.vOrientFront[0] = out[0];
	listener.vOrientFront[1] = out[1];
	listener.vOrientFront[2] = out[2];
	in[0] = 0.0f;
	in[1] = 1.0f;
	in[2] = 0.0f;
	in[3] = 1.0f;
	em_bmMatTransformVec(out, DAT_002be330, in);
	listener.vOrientTop[0] = out[0];
	listener.vOrientTop[1] = out[1];
	listener.vOrientTop[2] = out[2];
	listener.flDistanceFactor = DAT_00285a58;
	listener.flRolloffFactor = 1.0f;
	listener.flDopplerFactor = 8.0f;
	IDirectSound_SetAllParameters(DAT_002db808, &listener, 1);
	IDirectSound_CommitDeferredSettings(DAT_002db808);
}

/* 0x000571c0  BSOUND (unplaced)
Starts the sound system: DirectSound, the channels, the streams. */
int bsInitSound(void)
{
	if (bsInitDirectSound() != 0) {
		if (bsInitChannels() != 0) {
			if (bsInitAudioStreams() != 0)
				return 1;
			bsStopAllThunk();
		}
		bsShutdownDirectSound();
	}
	return 0;
}

/* 0x000571f0  BSOUND (unplaced) */
void bsShutdownSound(void)
{
	bsStopAllThunk();
	bsShutdownDirectSound();
}
