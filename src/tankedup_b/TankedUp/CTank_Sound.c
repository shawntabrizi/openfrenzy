/*
TankedUp/CTank_Sound.c  <-  CTank_Sound (file inferred from the class name in its debug
text; no source path in the XBE)

The sound bank of TankedUp (CTank_Game +0xa34, 0xec bytes): 28 samples
"sounds\<name>.wav" and a play helper.
  +0x00 the 28 sample names    +0x70 the 28 samples (BSample *)
  +0xe0 loaded                 +0xe4, +0xe8 the saved sound distances DAT_0028ffac/a8
*/

#include "tankedup_b.h"

#define SND_NAMES 0x00
#define SND_SAMPLES 0x70
#define SND_LOADED 0xe0
#define SND_COUNT 0x1c

extern float DAT_0028ffa8;          /* classlib_ui: the sound fall-off distances */
extern float DAT_0028ffac;

/* 0x001d5880  TankedUp (gap; CTank_Sound inferred)
CTank_Sound constructor: the sample names (it also sets the loaded flag). */
void *TankSound_Ctor(void *self)
{
	static const char *const names[SND_COUNT] = {
		"tanks_01a", "tanks_01b", "tanks_02a", "tanks_03a", "tanks_04a", "tanks_04b",
		"tanks_04c", "tanks_05a", "tanks_05b", "tanks_05c", "tanks_06a", "tanks_06b",
		"tanks_06c", "tanks_07c", "tanks_09a", "tanks_10a", "tanks_11a", "tanks_12a",
		"tanks_12b", "tanks_13a", "tanks_14a", "hitprotected", "hitrock01", "hitrock02",
		"hitrock03", "wallraising1", "hazard", "tankcol",
	};
	int i;

	TU_I32(self, SND_LOADED) = 1;
	for (i = 0; i < SND_COUNT; i++)
		TU_AT(self, SND_NAMES + i * 4, const char *) = names[i];
	return self;
}

/* 0x001d5960  TankedUp (gap; CTank_Sound inferred)
CTank_Sound::Initialise: sets the sound distances (10000, 4000; the old ones are kept) and
loads the samples. */
int TankSound_Initialise(void *self)
{
	char path[512];
	void *sample;
	int i;

	TU_F32(self, 0xe4) = DAT_0028ffac;
	TU_F32(self, 0xe8) = DAT_0028ffa8;
	DAT_0028ffac = 10000.0f;
	DAT_0028ffa8 = 4000.0f;
	TU_I32(self, SND_LOADED) = 1;
	for (i = 0; i < SND_COUNT; i++) {
		_snprintf(path, 0x1ff, "sounds\\%s.wav", TU_AT(self, SND_NAMES + i * 4, const char *));
		sample = bLoadResource(NULL, path, 2, 0xdefa);
		TU_PTR(self, SND_SAMPLES + i * 4) = sample;
		if (sample == NULL)
			TU_DEBUG("CTank_Sound::Initialise - can't load sound sample #%d [%s](%s)\n", i,
				TU_AT(self, SND_NAMES + i * 4, const char *), path);
	}
	return 1;
}

/* 0x001d5a10  TankedUp (gap; CTank_Sound inferred)
CTank_Sound::Free: restores the sound distances and stops every sound. */
void TankSound_Free(void *self)
{
	int i;

	DAT_0028ffac = TU_F32(self, 0xe4);
	DAT_0028ffa8 = TU_F32(self, 0xe8);
	TU_I32(self, SND_LOADED) = 0;
	bsStopAll();
	for (i = 0; i < SND_COUNT; i++)
		TU_PTR(self, SND_SAMPLES + i * 4) = NULL;
}

/* 0x001d5a50  TankedUp (gap; CTank_Sound inferred)
CTank_Sound::Play: plays sample `sound` at `volume` (0-255) with `pitch` added to its
frequency (0: as recorded); when no channel is free it takes channel sound % 64. `pos` and
`unused` are not used. The channel, or -1. */
int TankSound_Play(void *self, unsigned int sound, void *pos, int volume, int unused, int pitch)
{
	struct BSample *sample;
	int frequency = 0;
	int channel;

	(void)pos;
	(void)unused;
	if (TU_I32(self, SND_LOADED) == 0)
		return -1;
	sample = (struct BSample *)TU_PTR(self, SND_SAMPLES + sound * 4);
	if (sample == NULL || volume < 0)
		return -1;
	if (volume > 0xff)
		volume = 0xff;
	if (pitch != 0)
		frequency = TU_I32(sample, 0x20) + pitch;
	channel = bsPlaySample(sample, volume, 0, frequency, 0);
	if (channel == -1) {
		channel = (int)sound % 64;
		bsStopChannel(channel);
		channel = bsPlaySampleOnChannel(channel, (struct BSample *)TU_PTR(self, SND_SAMPLES + sound * 4),
			volume, 0, frequency, 1);
	}
	return channel;
}
