/*
BumperCars/CBCar_Sound.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Sound.cpp   (module bumpercars_a)

The BumperCars sound effects (CBCar_Game + 0x3320, 0x2e4 bytes): 31 samples
(sounds\<name>.wav) with a play cooldown, played on BSOUND channels, and four delayed
character voice lines. 0x00091fd0 is tagged with this file; the gap functions around it
(0x00091ef0-0x00092450) are this class's methods.
*/

#include "bumpercars_a.h"

static const char s_SoundFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Sound.cpp";

#define BCAR_NUM_SAMPLES 0x1f

/* a sample slot (0x10 bytes) */
struct BCarSoundSample {
	int played;                 /* +0x0 plays (reported at shutdown) */
	int cooldown;               /* +0x4 frames before it may play again */
	int channel;                /* +0x8 the BSOUND channel, -1 */
	struct BSample *sample;     /* +0xc */
};

/* a delayed voice line (0x1c bytes) */
struct BCarSoundVoice {
	int kind;                   /* +0x00 */
	int volume;                 /* +0x04 */
	float delay;                /* +0x08 seconds, -1 when idle */
	float pos[4];               /* +0x0c */
};

struct CBCar_Sound {
	const char *names[BCAR_NUM_SAMPLES];              /* 0x000 */
	struct BCarSoundSample samples[BCAR_NUM_SAMPLES]; /* 0x07c */
	struct BCarSoundVoice voices[4];                  /* 0x26c */
	float savedDistance[2];                           /* 0x2dc the 3D sound distances (0x0028ffa8) */
};
OF_ASSERT_OFFSET(struct CBCar_Sound, voices, 0x26c);
OF_ASSERT_SIZE(struct CBCar_Sound, 0x2e4);

/* classlib_ui data: the 3D sound minimum and maximum distances */
extern float DAT_0028ffa8;
extern float DAT_0028ffac;
/* core: per player (0x28 bytes): the character at 0x002baed4 */
extern int DAT_002baed4[];

/* 0x00091ef0  (gap): CBCar_Sound::CBCar_Sound: the sample names */
void *CBCar_Sound_Ctor(struct CBCar_Sound *self)
{
	static const char *const names[BCAR_NUM_SAMPLES] = {
		"bumper_07b", "bumper_07c", "bumper_07e", "bumper_09a", "bumper_08a", "bumper_08b",
		"bumper_04a", "bumper_04b", "bumper_04c", "bumpko_01a", "bumper_05a", "bumper_05b",
		"bumper_06a", "bumper_02a", "bumper_01a", "bumpko_06a", "bumpko_06b", "explosion01",
		"explosion02", "explosion03", "explosion04", "explosion05", "bumpko_03a", "bumpko_07a",
		"bumpko_08a", "bhockey_04a", "bhockey_04a", "bkeeper_02a", "bkeeper_01a", "bumper_02a",
		"bkeeper_03a",
	};
	int i;

	for (i = 0; i < BCAR_NUM_SAMPLES; i++)
		self->names[i] = names[i];
	return self;
}

/* 0x00091fd0  BumperCars/CBCar_Sound.cpp: CBCar_Sound::Initialise: loads the samples and sets
the 3D sound distances (500, 40000; the old ones are kept). Returns 1. */
int CBCar_Sound_Initialise(struct CBCar_Sound *self)
{
	char name[512];
	int i;

	for (i = 0; i < BCAR_NUM_SAMPLES; i++) {
		snprintf(name, 0x1ff, "sounds\\%s.wav", self->names[i]);
		self->samples[i].sample = (struct BSample *)bLoadResource(NULL, name, 2, 0xdefa);
		if (self->samples[i].sample == NULL)
			BCA_Printf("%s(%d) : Initialise - Could not load sound sample #%d [%s]\n", s_SoundFile, 0x5c, i,
				self->names[i]);
		self->samples[i].cooldown = 0;
		self->samples[i].played = 0;
		self->samples[i].channel = -1;
	}
	for (i = 0; i < 4; i++)
		self->voices[i].delay = -1.0f;
	self->savedDistance[0] = DAT_0028ffa8;
	self->savedDistance[1] = DAT_0028ffac;
	DAT_0028ffa8 = 500.0f;
	DAT_0028ffac = 40000.0f;
	return 1;
}

/* 0x000920b0  (gap): CBCar_Sound::Shutdown: frees the channels, restores the distances and
reports the plays */
void CBCar_Sound_Shutdown(struct CBCar_Sound *self)
{
	int i;

	for (i = 0; i < bsGetNumChannels(); i++)
		bsReserveChannel(i, 0);
	bsStopAll();
	DAT_0028ffa8 = self->savedDistance[0];
	DAT_0028ffac = self->savedDistance[1];
	for (i = 0; i < BCAR_NUM_SAMPLES; i++)
		BCA_Printf("Sample #%d [%s] played %d times\n", i, self->names[i], self->samples[i].played);
}

/* 0x00092120  (gap): plays sample `index` at `volume` (0-255), `pitch` above its rate, on
`channel` (-1: any free one, else channel index % 64). Returns the channel, or -1 while
the sample cools down. */
int CBCar_Sound_Play(struct CBCar_Sound *self, int index, int volume, int pitch, int channel)
{
	struct BCarSoundSample *slot = &self->samples[index];
	struct BSample *sample = slot->sample;
	int frequency = 0, playing, i, c;

	if (sample == NULL)
		return -1;
	if (volume < 0)
		return -1;
	if (volume > 0xff)
		volume = 0xff;
	if (slot->cooldown != 0)
		return -1;
	if (pitch != 0)
		frequency = BCA_I32(sample, 0x20) + pitch;
	if ((unsigned int)index <= 2)
		slot->cooldown = 0x14;
	else if (index == 10 || index == 0xb)
		slot->cooldown = 8;
	else
		slot->cooldown = 3;
	slot->played++;
	if (channel != -1) {
		bsStopChannel(channel);
		playing = bsPlaySampleOnChannel(channel, sample, volume, 0, frequency, 1);
	} else {
		playing = bsPlaySample(sample, volume, 0, frequency, 0);
		if (playing == -1) {
			c = index % 64;
			bsStopChannel(c);
			playing = bsPlaySampleOnChannel(c, sample, volume, 0, frequency, 1);
		}
	}
	slot->channel = playing;
	for (i = 0; i < BCAR_NUM_SAMPLES; i++) {
		if (i != index && self->samples[i].channel == playing)
			self->samples[i].channel = -1;
	}
	return playing;
}

/* 0x00092290  (gap): the channel has finished */
int CBCar_Sound_ChannelDone(int channel)
{
	if (channel == -1)
		return 0;
	return bsGetChannelStatus(channel) == 0;
}

/* 0x000922b0  (gap): pauses every channel */
void CBCar_Sound_PauseAll(void)
{
	bsPauseAll(1);
}

/* 0x000922c0  (gap): resumes every channel */
void CBCar_Sound_ResumeAll(void)
{
	bsPauseAll(0);
}

/* 0x000922d0  (gap): sets a channel's pitch above sample `index`'s rate */
void CBCar_Sound_SetPitch(struct CBCar_Sound *self, int channel, int index, int pitch)
{
	int frequency = 0;

	if (channel == -1)
		return;
	if (pitch != 0)
		frequency = BCA_I32(self->samples[index].sample, 0x20) + pitch;
	bsSetChannelFrequency(channel, frequency);
}

/* 0x00092310  (gap): a commentary line from set 6 */
void CBCar_Sound_Commentary(int kind, int choice, int volume)
{
	CLSamples_PlayFromSet(6, kind, NULL, volume, 0, choice, 0);
}

/* 0x00092340  (gap): counts down the voice lines and the cooldowns */
void CBCar_Sound_Update(struct CBCar_Sound *self, float dt)
{
	struct BCarSoundVoice *voice;
	int i;

	for (i = 0; i < 4; i++) {
		voice = &self->voices[i];
		if (voice->delay >= 0.0f) {
			voice->delay = voice->delay - dt;
			if (voice->delay <= 0.0f) {
				voice->delay = -1.0f;
				CLSamples_PlayVoice((char)DAT_002baed4[i * 10], voice->kind, NULL, voice->volume, 180.0f, 0, -1, 0);
			}
		}
	}
	for (i = 0; i < BCAR_NUM_SAMPLES; i++) {
		if (self->samples[i].cooldown > 0)
			self->samples[i].cooldown--;
	}
}

/* 0x000923d0  (gap): stops everything */
void CBCar_Sound_StopAll(struct CBCar_Sound *self)
{
	int i;

	for (i = 0; i < BCAR_NUM_SAMPLES; i++) {
		if (self->samples[i].channel != -1) {
			bsSetChannelFrequency(self->samples[i].channel, 0);
			bsStopChannel(self->samples[i].channel);
			self->samples[i].channel = -1;
			self->samples[i].played = 0;
		}
	}
	for (i = 0; i < bsGetNumChannels(); i++)
		bsReserveChannel(i, 0);
	for (i = 0; i < 4; i++)
		self->voices[i].delay = -1.0f;
}

/* 0x00092450  (gap): queues player `player`'s voice line: after a random time up to
`delay` seconds (in frames of 1/60) */
void CBCar_Sound_QueueVoice(struct CBCar_Sound *self, int player, int kind, int volume, float *pos, float delay)
{
	struct BCarSoundVoice *voice = &self->voices[player];

	voice->delay = delay * 60.0f * (float)bca_rand() * (1.0f / 32767.0f);
	voice->pos[0] = pos[0];
	voice->pos[1] = pos[1];
	voice->pos[2] = pos[2];
	voice->pos[3] = pos[3];
	voice->kind = kind;
	voice->volume = volume;
}
