/*
Rowing/Rowing_Sound.c  <-  C:\BlitzParty\Game\Src\Rowing\Rowing_Sound.cpp

The sounds of the Rowing game: 30 samples (table at 0x002b5a18: name, looped, from the
language package) loaded into 0x0030fc08, and a queue (0x5c4 bytes at 0x0030fc80) of up to
50 sounds to play, one a frame, plus up to 16 reserved channels.

0x001b6da0 is tagged with Rowing_Sound.cpp; 0x001b6ba0-0x001b6f50 work on the same data
(file inferred). 0x001b6f50 is longer than the function table says (it ends at
0x001b6fd5).
*/

#include "rowing_b.h"

/* the samples (struct BSample *) */
#define g_RowSamples ((struct BSample **)RW_ADDR(0x0030fc08))
#define RB_NUM_SAMPLES 0x1e
/* the sound queue (0x5c4 bytes): 50 entries of 0x1c bytes, the count at +0x578, the
   reserved channels' count at +0x580 and the channels at +0x584 */
#define g_RowSoundQueue RW_VAR(unsigned char *, 0x0030fc80)
/* the file name buffer (0x0030fc88) */
#define g_RowSoundPath ((char *)RW_ADDR(0x0030fc88))

/* a sample table entry (0x002b5a18) */
struct RowSoundFile {
	const char *name;
	unsigned char looped;       /* loopedsounds\ instead of sounds\ */
	unsigned char language;     /* from the language package */
	unsigned char pad[2];
};
#define g_RowSoundFiles ((struct RowSoundFile *)RW_ADDR(0x002b5a18))

/* a queued sound (0x1c bytes) */
struct RowQueuedSound {
	int kind;                   /* +0x00 0 plain, 1 positional */
	int sample;                 /* +0x04 */
	float pos[4];               /* +0x08 */
	int volume;                 /* +0x18 */
};

/* the rumble manager (classlib_ui CRumble at 0x002ed324) */
extern unsigned char DAT_002ed324[];

/* 0x001b6ba0  Rowing (gap; Rowing_Sound.cpp inferred)
Plays sample `index` at `volume`. Returns the channel, or -1. */
int RowSound_Play(int index, int volume)
{
	if (index < 0 || index >= RB_NUM_SAMPLES || g_RowSamples[index] == NULL)
		return -1;
	return bsPlaySample(g_RowSamples[index], volume, 0, 0, 0);
}

/* 0x001b6be0  Rowing (gap; Rowing_Sound.cpp inferred)
Plays sample `index` following the position `pos` (heard from 20000 to 30000). Returns the
channel, or -1. */
int RowSound_PlayMoving(int index, float *pos, int volume)
{
	if (index < 0 || index >= RB_NUM_SAMPLES || g_RowSamples[index] == NULL)
		return -1;
	return bsPlaySampleMoving(g_RowSamples[index], pos, volume, 0, 0, NULL, NULL, 20000.0f, 30000.0f);
}

/* 0x001b6c30  Rowing (gap; Rowing_Sound.cpp inferred)
Queues sample `index` (when the queue has room). */
void RowSound_Queue(int index, int volume)
{
	struct RowQueuedSound *entry;
	unsigned char *queue;

	if (index < 0 || index >= RB_NUM_SAMPLES || g_RowSamples[index] == NULL)
		return;
	queue = g_RowSoundQueue;
	if (RB_I32(queue, 0x578) >= 0x32)
		return;
	entry = (struct RowQueuedSound *)(queue + RB_I32(queue, 0x578) * 0x1c);
	entry->sample = index;
	entry->kind = 0;
	entry->volume = volume;
	RB_I32(queue, 0x578) = RB_I32(queue, 0x578) + 1;
}

/* 0x001b6c80  Rowing (gap; Rowing_Sound.cpp inferred)
Sets the frequency of `channel` to sample `index`'s frequency x scale. */
void RowSound_SetFrequency(int channel, int index, float scale)
{
	if (index < 0 || index >= RB_NUM_SAMPLES || g_RowSamples[index] == NULL)
		return;
	bsSetChannelFrequency(channel, (int)((float)RB_I32(g_RowSamples[index], 0x20) * scale));
}

/* 0x001b6cc0  Rowing (gap; Rowing_Sound.cpp inferred)
Plays the first queued sound and moves the rest up. */
void RowSound_Update(void *queue)
{
	unsigned char *q = queue;
	struct RowQueuedSound *entry = queue;
	int i;

	if (RB_I32(q, 0x578) == 0)
		return;
	if (entry->kind == 0)
		bsPlaySample(g_RowSamples[entry->sample], entry->volume, 0, 0, 0);
	else if (entry->kind == 1)
		bsPlaySample3D(g_RowSamples[entry->sample], entry->pos, entry->volume, 0, 0, 20000.0f, 30000.0f);
	RB_I32(q, 0x578) = RB_I32(q, 0x578) - 1;
	for (i = 0; i < RB_I32(q, 0x578); i++)
		memcpy(&entry[i], &entry[i + 1], 0x1c);
}

/* 0x001b6d60  Rowing (gap; Rowing_Sound.cpp inferred)
Reserves `channel` (not -1; at most 16). */
void RowSound_Reserve(void *queue, int channel)
{
	unsigned char *q = queue;

	if (channel == -1 || RB_I32(q, 0x580) >= 0x10)
		return;
	bsReserveChannel(channel, 1);
	RB_I32(q, 0x584 + RB_I32(q, 0x580) * 4) = channel;
	RB_I32(q, 0x580) = RB_I32(q, 0x580) + 1;
}

/* 0x001b6da0  Rowing/Rowing_Sound.cpp
Makes the queue, loads the 30 samples (sounds\ or loopedsounds\<name>.wav from `package`,
<name>.wav from `languagePackage`) and the rumble effects. */
void RowSound_Init(void *package, void *languagePackage)
{
	struct RowSoundFile *file;
	unsigned char *queue;
	int i;

	queue = RB_NEW(0x5c4, RB_FILE_SOUND, 0x9b, 0x1001);
	g_RowSoundQueue = NULL;
	if (queue != NULL) {
		memset(queue, 0, 0x171 * 4);
		g_RowSoundQueue = queue;
	}
	RB_I32(g_RowSoundQueue, 0x578) = 0;
	bkPrintf("Sound Reports %d channels available\n", bsGetNumChannels());
	for (i = 0; i < RB_NUM_SAMPLES; i++) {
		file = &g_RowSoundFiles[i];
		if (file->language == 0) {
			sprintf(g_RowSoundPath, file->looped == 0 ? "sounds\\%s.wav" : "loopedsounds\\%s.wav",
				file->name);
			g_RowSamples[i] = (struct BSample *)bLoadResource(package, g_RowSoundPath, 2, 0xdefa);
		} else {
			sprintf(g_RowSoundPath, "%s.wav", file->name);
			g_RowSamples[i] = (struct BSample *)bLoadResource(languagePackage, g_RowSoundPath, 2, 0xdefa);
		}
	}
	CLRumble_CreateMenu((int *)DAT_002ed324);
	CLRumble_AddEffect((int *)DAT_002ed324, "JB_Collide");
	CLRumble_AddEffect((int *)DAT_002ed324, "JB_WrongGate");
	CLRumble_AddEffect((int *)DAT_002ed324, "JB_WfallTop");
	CLRumble_AddEffect((int *)DAT_002ed324, "JB_WfallBot");
	CLRumble_AddEffect((int *)DAT_002ed324, "JB_WPoolMid");
}

/* 0x001b6ee0  Rowing (gap; Rowing_Sound.cpp inferred)
Queues sample `index` at pos (when the queue has room). */
void RowSound_Queue3D(int index, float *pos, int volume)
{
	struct RowQueuedSound *entry;
	unsigned char *queue;

	if (index < 0 || index >= RB_NUM_SAMPLES || g_RowSamples[index] == NULL)
		return;
	queue = g_RowSoundQueue;
	if (RB_I32(queue, 0x578) >= 0x32)
		return;
	entry = (struct RowQueuedSound *)(queue + RB_I32(queue, 0x578) * 0x1c);
	entry->sample = index;
	entry->kind = 1;
	memcpy(entry->pos, pos, 16);
	entry->volume = volume;
	RB_I32(queue, 0x578) = RB_I32(queue, 0x578) + 1;
}

/* 0x001b6f50  Rowing (gap; Rowing_Sound.cpp inferred)
Stops every sound, frees the samples (the slots keep their pointers), releases the
reserved channels and frees the queue. */
void RowSound_Free(void)
{
	unsigned char *queue;
	int i;

	bsStopAll();
	for (i = 0; i < RB_NUM_SAMPLES; i++) {
		if (g_RowSamples[i] != NULL) {
			bsStopSample(g_RowSamples[i]);
			bkDeleteResource((struct bkResource *)g_RowSamples[i]);
		}
	}
	queue = g_RowSoundQueue;
	if (queue != NULL) {
		for (i = 0; i < RB_I32(queue, 0x580); i++)
			bsReserveChannel(RB_I32(queue, 0x584 + i * 4), 0);
		RB_DELETE(queue);
	}
	g_RowSoundQueue = NULL;
}
