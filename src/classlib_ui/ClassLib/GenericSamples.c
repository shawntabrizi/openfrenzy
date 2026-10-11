/*
ClassLib/GenericSamples.c  (classlib_ui)
Original: C:\BlitzParty\Game\Src\ClassLib\GenericSamples.cpp (tag at 0x000714b0).

Sound samples shared by every game mode:
- 26 generic effects (horn.wav, tokens, punches, refbot ...),
- per character voice sets (6 characters x 19 kinds: jump, land, hurt ...),
  from the "CharVoice" package of the language folder,
- the commentator (58 kinds: countdown, go, playerwins ...), loaded per screen
  from the language folder (com_ingame, com_mapscreen ...), or from
  Packages\Welsh,
- a queue of up to 5 commentator/voice lines that wait for a channel,
- the music streams (InitMusic).

The functions from 0x00071130 to 0x00071bf0 are placed here: 0x000714b0 is tagged
with this file, and the others work on the same tables (inferred).
*/

#include "classlib_ui.h"

/* ---------- types */

/* a set of variants of one sample kind: "<name>00.wav" .. "<name>09.wav" */
struct CLSampleSet {
	int samples[10]; /* 0x00 sample handles (FUN_0004bb30) */
	int count;       /* 0x28 variants found */
	int last;        /* 0x2c variant played last (-1: none) */
	float cooldown;  /* 0x30 frames until the kind may play again (characters only) */
};
OF_ASSERT_SIZE(struct CLSampleSet, 0x34);

#define CL_CHARACTER_COUNT 6
#define CL_VOICE_KIND_COUNT 19
#define CL_COMMENTARY_KIND_COUNT 58
#define CL_GENERIC_SAMPLE_COUNT 26
#define CL_COMMENTATOR_SET_COUNT 7
#define CL_MUSIC_TRACK_COUNT 28

/* a commentator or voice line that waits until a channel is free */
struct CLQueuedLine {
	int set;          /* 0x00 character (0-5), or -1 for the commentator */
	int kind;         /* 0x04 */
	int choice;       /* 0x08 variant, -1 for a random one */
	int volume;       /* 0x0c */
	unsigned int flags; /* 0x10 */
	int allow_repeat; /* 0x14 */
	int wait_channel; /* 0x18 channel that must be silent first */
};
OF_ASSERT_SIZE(struct CLQueuedLine, 0x1c);

/* a music track: stream name and the game kinds it may play in */
struct CLMusicTrack {
	const char *name;   /* 0x00 "<name>.wma" in the music package */
	unsigned int flags; /* 0x04 */
};

/* ---------- globals (classlib_ui_data.h) */

/* 0x0028fda8  frame time scale */
#define clu_time_scale CLU_VAR(float, 0x0028fda8)
/* 0x0028fde8  commentator set loaded now (-1: none) */
#define clu_commentator_set CLU_VAR(int, 0x0028fde8)
/* 0x0028fdec  commentator packages, one per screen: "com_frenzyintro", "com_ingame" ... */
#define clu_commentator_packages CLU_ARR(const char *, 0x0028fdec)
/* 0x0028fe08  generic effects: "horn.wav", "gen_token_01a.wav" ... */
#define clu_generic_names CLU_ARR(const char *, 0x0028fe08)
/* 0x0028fe70  character voice kinds: "jump", "land", "hurt" ... */
#define clu_voice_kinds CLU_ARR(const char *, 0x0028fe70)
/* 0x0028fec0  commentator kinds: "countdown", "go", "getready" ... */
#define clu_commentary_kinds CLU_ARR(const char *, 0x0028fec0)
/* 0x0028ffa8 / 0x0028ffac  3D sample min / max distance (4000, 10000) */
#define clu_sample_min_distance CLU_VAR(float, 0x0028ffa8)
#define clu_sample_max_distance CLU_VAR(float, 0x0028ffac)
/* 0x0028ffb0  sample length -> commentator delay factor (1.75) */
#define clu_commentary_delay_scale CLU_VAR(float, 0x0028ffb0)
/* 0x0028ffb4  channel of the commentator line that plays now (-1: none) */
#define clu_commentary_channel CLU_VAR(int, 0x0028ffb4)
/* 0x0028ffc0  music tracks */
#define clu_music_tracks CLU_ARR(const struct CLMusicTrack, 0x0028ffc0)
/* 0x002906f8  character table */
#define clu_characters CLU_ARR(const struct CLCharacterInfo, 0x002906f8)

/* 0x002ea490  generic effect samples */
#define clu_generic_samples CLU_ARR(int, 0x002ea490)
/* 0x002ea4f8  queued commentator/voice lines */
#define clu_queue CLU_ARR(struct CLQueuedLine, 0x002ea4f8)
/* 0x002ea588  commentator sample sets */
#define clu_commentary_sets CLU_ARR(struct CLSampleSet, 0x002ea588)
/* 0x002eb150  character voice sample sets [character][kind] */
#define clu_voice_sets ((struct CLSampleSet (*)[CL_VOICE_KIND_COUNT])CLU_ADDR(0x002eb150))
/* 0x002ec878  CharVoice package */
#define clu_charvoice_package CLU_VAR(int, 0x002ec878)
/* 0x002ec87c  commentator package */
#define clu_commentator_package CLU_VAR(int, 0x002ec87c)
/* 0x002ec880  Packages\Welsh */
#define clu_welsh_package CLU_VAR(int, 0x002ec880)
/* 0x002ec884  memory block that holds the largest commentator package */
#define clu_commentator_memory CLU_VAR(int, 0x002ec884)
/* 0x002ec888  queued lines */
#define clu_queue_count CLU_VAR(int, 0x002ec888)
/* 0x002ec890  music stream samples */
#define clu_music_samples CLU_ARR(int, 0x002ec890)

/* ---------- character voices */

/* 0x00071130  ClassLib/GenericSamples.cpp (inferred)
Forgets the voice samples of one character. */
void CLSamples_ClearCharacter(int character)
{
	int kind;
	int i;

	for (kind = 0; kind < CL_VOICE_KIND_COUNT; kind++) {
		for (i = 0; i < 10; i++)
			clu_voice_sets[character][kind].samples[i] = 0;
		clu_voice_sets[character][kind].count = 0;
	}
}

/* 0x00071170  ClassLib/GenericSamples.cpp (inferred)
Finds the voice samples "<character>\<kind>NN.wav" of one character. */
void CLSamples_LoadCharacter(int character)
{
	struct CLSampleSet *set;
	char probe[64];
	char name[64];
	int kind;
	int i;
	int sample;

	for (kind = 0; kind < CL_VOICE_KIND_COUNT; kind++) {
		set = &clu_voice_sets[character][kind];
		set->count = 0;
		set->last = -1;
		set->cooldown = 0.0f;
		for (i = 0; i < 10; i++)
			set->samples[i] = 0;
		while (set->count < 10) {
			sprintf(probe, "%s\\%s%02d.wav", clu_characters[character].name, clu_voice_kinds[kind],
				set->count);
			_strlwr(probe);
			if (FUN_0004bb30(0, probe, 2, 0xdefa) == 0)
				break;
			set->count = set->count + 1;
		}
		for (i = 0; i < 10; i++)
			set->samples[i] = 0;
		if (set->count != 0) {
			for (i = 0; i < set->count; i++) {
				sprintf(name, "%s\\%s%02d.wav", clu_characters[character].name, clu_voice_kinds[kind], i);
				_strlwr(name);
				set->samples[i] = (int)FUN_0004bb30(0, name, 2, 0xdefa);
			}
		}
	}
}

/* 0x000712b0  ClassLib/GenericSamples.cpp (inferred)
Loads the generic effects, the CharVoice package and every character's voices. */
void CLSamples_LoadGeneric(void)
{
	char path[256];
	int i;
	int character;
	int kind;

	for (i = 0; i < CL_GENERIC_SAMPLE_COUNT; i++) {
		clu_generic_samples[i] = (int)FUN_0004bb30(0, (char *)clu_generic_names[i], 2, 0xdefa);
		if (clu_generic_samples[i] == 0)
			CLU_DebugPrintf("Couldn't Load %s\n", clu_generic_names[i]);
	}
	sprintf(path, "%s\\CharVoice", DAT_002ba544);
	clu_charvoice_package = (int)FUN_0004fd50(0, path, 0);
	if (clu_charvoice_package == 0) {
		CLU_DebugPrintf(
			"CGameMode_Root::Pushed *** Could not load package 'Packages\\Character Voice SFX' ***\n");
		return;
	}
	FUN_0004b870((struct bkPackage *)clu_charvoice_package, 0xffffffff, 0xdefa, 0);
	for (character = 0; character < CL_CHARACTER_COUNT; character++) {
		for (kind = 0; kind < CL_VOICE_KIND_COUNT; kind++)
			for (i = 0; i < 10; i++)
				clu_voice_sets[character][kind].samples[i] = 0;
		CLSamples_LoadCharacter(character);
	}
}

/* ---------- playing */

/* 0x000713a0  ClassLib/GenericSamples.cpp (inferred)
Plays a sample: positioned at `pos` (3D) when pos is not NULL, else 2D.
Returns the channel, or -1. */
int CLSamples_Play(int sample, int volume, int arg3, unsigned int flags, float *pos)
{
	if (sample == 0)
		return -1;
	if (pos == NULL)
		return FUN_00054ba0((struct BSample *)sample, volume, 0, arg3, flags);
	return FUN_00054c30((struct BSample *)sample, pos, volume, arg3, flags, clu_sample_min_distance, clu_sample_max_distance);
}

/* 0x00071400  ClassLib/GenericSamples.cpp (inferred)
Forgets every voice and commentator sample and frees the sample packages. */
void CLSamples_Shutdown(void)
{
	int character;
	int kind;
	int i;

	for (character = 0; character < CL_CHARACTER_COUNT; character++) {
		for (kind = 0; kind < CL_VOICE_KIND_COUNT; kind++) {
			for (i = 0; i < 10; i++)
				clu_voice_sets[character][kind].samples[i] = 0;
			clu_voice_sets[character][kind].count = 0;
		}
	}
	for (kind = 0; kind < CL_COMMENTARY_KIND_COUNT; kind++) {
		for (i = 0; i < 10; i++)
			clu_commentary_sets[kind].samples[i] = 0;
		clu_commentary_sets[kind].count = 0;
	}
	if (clu_charvoice_package != 0)
		FUN_0004fe80((struct bkPackage *)clu_charvoice_package);
	if (clu_commentator_package != 0)
		FUN_0004fe80((struct bkPackage *)clu_commentator_package);
	if (clu_commentator_memory != 0) {
		FUN_0004c110((void *)clu_commentator_memory);
		clu_commentator_memory = 0;
	}
	if (clu_welsh_package != 0)
		FUN_0004fe80((struct bkPackage *)clu_welsh_package);
}

/* ---------- commentator */

/* 0x000714b0  ClassLib/GenericSamples.cpp
Allocates (once) a memory block as large as the largest commentator package. */
void CLSamples_AllocCommentatorMemory(void)
{
	char path[1024];
	int largest;
	int size;
	int i;

	largest = 0;
	if (clu_commentator_memory != 0)
		return;
	for (i = 0; i < CL_COMMENTATOR_SET_COUNT; i++) {
		sprintf(path, "%s\\%s", DAT_002ba544, clu_commentator_packages[i]);
		size = ((int (*)(const char *, int))FUN_0004fef0)(path, 1);
		if (largest < size)
			largest = size;
	}
	clu_commentator_memory = (int)FUN_0004bf10(largest, "C:\\BlitzParty\\Game\\Src\\ClassLib\\GenericSamples.cpp",
					 0x1d9, 0x2001, FUN_0004c280(), 0x80);
}

/* 0x00071540  ClassLib/GenericSamples.cpp (inferred)
Finds the commentator samples "<kind>NN.wav" of the loaded package. */
void CLSamples_LoadCommentary(void)
{
	struct CLSampleSet *set;
	char probe[64];
	char name[64];
	int kind;
	int i;

	for (kind = 0; kind < CL_COMMENTARY_KIND_COUNT; kind++) {
		set = &clu_commentary_sets[kind];
		set->count = 0;
		set->last = -1;
		for (i = 0; i < 10; i++)
			set->samples[i] = 0;
		while (set->count < 10) {
			sprintf(probe, "%s%02d.wav", clu_commentary_kinds[kind], set->count);
			_strlwr(probe);
			if (FUN_0004bb30(0, probe, 2, 0xdefa) == 0)
				break;
			set->count = set->count + 1;
		}
		if (set->count != 0) {
			for (i = 0; i < set->count; i++) {
				sprintf(name, "%s%02d.wav", clu_commentary_kinds[kind], i);
				_strlwr(name);
				set->samples[i] = (int)FUN_0004bb30(0, name, 2, 0xdefa);
			}
		}
	}
}

/* 0x00071630  ClassLib/GenericSamples.cpp (inferred)
Queues a line that plays when channel `wait_channel` is silent (5 at most). */
void CLSamples_QueueLine(int set, int wait_channel, int kind, int choice, int volume, unsigned int flags,
			 int allow_repeat)
{
	struct CLQueuedLine *line;

	if (clu_queue_count < 5) {
		line = &clu_queue[clu_queue_count];
		line->set = set;
		line->kind = kind;
		line->choice = choice;
		line->volume = volume;
		line->flags = flags;
		line->allow_repeat = allow_repeat;
		line->wait_channel = wait_channel;
		clu_queue_count = clu_queue_count + 1;
	}
}

/* 0x00071690  ClassLib/GenericSamples.cpp (inferred) */
void CLSamples_ClearQueue(void)
{
	clu_queue_count = 0;
}

/* 0x000716a0  ClassLib/GenericSamples.cpp (inferred)
Loads the Welsh commentary package (replaces the language commentary). */
void CLSamples_LoadWelsh(void)
{
	if (clu_welsh_package != 0)
		FUN_0004fe80((struct bkPackage *)clu_welsh_package);
	clu_welsh_package = (int)FUN_0004fd50(0, "Packages\\Welsh", 0);
	if (clu_welsh_package != 0) {
		FUN_0004b870((struct bkPackage *)clu_welsh_package, 0xffffffff, 0xdefa, 0);
		CLSamples_LoadCommentary();
	}
}

/* 0x000716f0  ClassLib/GenericSamples.cpp (inferred)
1 when no commentator line is queued or playing. */
int CLSamples_CommentaryIdle(void)
{
	if (clu_queue_count != 0)
		return 0;
	if (clu_commentary_channel != -1 && FUN_000548d0(clu_commentary_channel) != 0)
		return 0;
	return 1;
}

/* 0x00071720  ClassLib/GenericSamples.cpp (inferred)
Plays generic effect `index` (see clu_generic_names). */
int CLSamples_PlayGeneric(int index, int volume, int arg3, unsigned int flags, float *pos)
{
	int sample = clu_generic_samples[index];

	if (sample == 0)
		return -1;
	if (pos == NULL)
		return FUN_00054ba0((struct BSample *)sample, volume, 0, arg3, flags);
	return CLSamples_Play(sample, volume, arg3, flags, pos);
}

/* 0x00071770  ClassLib/GenericSamples.cpp (inferred)
Plays a variant of a commentator kind (set 6) or of a character voice kind
(set 0-5). choice -1 picks a random variant (not the last one unless
allow_repeat). */
int CLSamples_PlayFromSet(char set_index, int kind, float *pos, int volume, unsigned int flags, int choice,
			  int allow_repeat)
{
	struct CLSampleSet *set;

	if (set_index == 6)
		set = &clu_commentary_sets[kind];
	else
		set = &clu_voice_sets[0][0] + (set_index * CL_VOICE_KIND_COUNT + kind);
	if (choice == -1) {
		if (allow_repeat == 0 && 1 < set->count) {
			do {
				choice = CLU_Rand() % set->count;
			} while (choice == set->last);
		} else {
			if (set->count == 0)
				return -1;
			choice = CLU_Rand() % set->count;
		}
	}
	if (-1 < choice && choice < set->count) {
		set->last = choice;
		if (DAT_00283e0c != 0) {
			int sample = set->samples[choice];
			return CLSamples_Play(sample, volume, (int)((double)*(int *)(sample + 0x20) * (double)clu_commentary_delay_scale),
					      flags, pos);
		}
		return CLSamples_Play(set->samples[choice], volume, 0, flags, pos);
	}
	return -1;
}

/* 0x00071880  ClassLib/GenericSamples.cpp (inferred)
Loads the commentator package of a screen (0-6, see clu_commentator_packages). With Welsh
commentary the Welsh package is used instead. */
void CLSamples_LoadCommentatorSet(int set)
{
	char path[128];

	if (DAT_00283d74 == 0) {
		if (clu_welsh_package != 0) {
			FUN_0004fe80((struct bkPackage *)clu_welsh_package);
			clu_welsh_package = 0;
			CLSamples_AllocCommentatorMemory();
			CLSamples_LoadCommentary();
		}
		if (set < 0 || 6 < set || set == clu_commentator_set)
			return;
	} else {
		CLSamples_LoadWelsh();
	}
	CLSamples_AllocCommentatorMemory();
	if (clu_commentator_package != 0) {
		FUN_0004e010(FUN_0004dce0((struct bkPackage *)clu_commentator_package));
		FUN_0004fe80((struct bkPackage *)clu_commentator_package);
	}
	sprintf(path, "%s\\%s", DAT_002ba544, clu_commentator_packages[set]);
	clu_commentator_package = (int)FUN_0004fd50(0, path, (void *)clu_commentator_memory);
	if (clu_commentator_package == 0) {
		CLU_DebugPrintf("LoadCommentatorSampleSet *** Could not load package *%s **\n", clu_commentator_packages[set]);
		clu_commentator_set = -1;
		CLSamples_LoadCommentary();
		return;
	}
	clu_commentator_set = set;
	FUN_0004b870((struct bkPackage *)clu_commentator_package, 0xffffffff, 0xdefa, 0);
	CLSamples_LoadCommentary();
}

/* 0x000719a0  ClassLib/GenericSamples.cpp (inferred)
Plays a commentator line. With Welsh commentary a random choice (< 0) becomes
variant 0. */
int CLSamples_PlayCommentary(int kind, int choice, int volume, unsigned int flags, int allow_repeat,
			     float *pos)
{
	if (DAT_00283d74 != 0)
		choice = choice & ((choice < 0) - 1);
	return CLSamples_PlayFromSet(6, kind, pos, volume, flags, choice, allow_repeat);
}

/* 0x000719e0  ClassLib/GenericSamples.cpp (inferred)
Plays a character voice line unless the kind is cooling down; then it cools
down for `cooldown` frames. */
int CLSamples_PlayVoice(char character, int kind, float *pos, int volume, float cooldown, unsigned int flags,
			int choice, int allow_repeat)
{
	struct CLSampleSet *set = &clu_voice_sets[0][0] + (character * CL_VOICE_KIND_COUNT + kind);

	if (set->cooldown == 0.0f) {
		set->cooldown = cooldown;
		return CLSamples_PlayFromSet(character, kind, pos, volume, flags, choice, allow_repeat);
	}
	return -1;
}

/* 0x00071a40  ClassLib/GenericSamples.cpp (inferred)
Plays the first queued line once its channel is silent. Later lines that
wait for "-channel" (the channel of this line, not known when queued) get
the new channel. */
void CLSamples_UpdateQueue(void)
{
	int channel;
	int i;

	if (clu_queue_count == 0 || FUN_000548d0(clu_queue[0].wait_channel) != 0)
		return;
	if (clu_queue[0].set == -1)
		channel = CLSamples_PlayCommentary(clu_queue[0].kind, clu_queue[0].choice,
						   clu_queue[0].volume, clu_queue[0].flags,
						   clu_queue[0].allow_repeat, NULL);
	else
		channel = CLSamples_PlayFromSet(clu_queue[0].set, clu_queue[0].kind, NULL,
						clu_queue[0].volume, clu_queue[0].flags,
						clu_queue[0].choice, clu_queue[0].allow_repeat);
	clu_commentary_channel = channel;
	if (1 < clu_queue_count) {
		for (i = 1; i < clu_queue_count; i++) {
			if (clu_queue[i].wait_channel == -clu_queue[0].wait_channel)
				clu_queue[i].wait_channel = channel;
			clu_queue[i - 1] = clu_queue[i];
		}
	}
	clu_queue_count = clu_queue_count + -1;
}

/* 0x00071b10  ClassLib/GenericSamples.cpp (inferred)
Per frame: plays the queue and counts the voice cool-downs down. */
void CLSamples_Update(void)
{
	struct CLSampleSet *set;
	int character;
	int kind;

	CLSamples_UpdateQueue();
	for (character = 0; character < CL_CHARACTER_COUNT; character++) {
		for (kind = 0; kind < CL_VOICE_KIND_COUNT; kind++) {
			set = &clu_voice_sets[character][kind];
			if (set->cooldown != 0.0f) {
				set->cooldown = set->cooldown - clu_time_scale;
				if (set->cooldown < 0.0f)
					set->cooldown = 0.0f;
			}
		}
	}
}

/* ---------- music */

/* 0x00071b60  ClassLib/GenericSamples.cpp (inferred)
InitMusic: finds the "<track>.wma" stream of every track in the music
package of the root game mode. */
void InitMusic(void)
{
	char name[64];
	int root;
	int i;

	root = (int)FUN_0002b5b0((struct CGameModeStack *)DAT_002ba750, (const char *)"Root");
	if (*(int *)(root + 0x4c) != 0) {
		CLU_DebugPrintf("InitMusic() --------------------\n");
		for (i = 0; i < CL_MUSIC_TRACK_COUNT; i++) {
			CLU_DebugPrintf("%s\n", clu_music_tracks[i].name);
			sprintf(name, "%s.wma", clu_music_tracks[i].name);
			clu_music_samples[i] = (int)FUN_0004bb30((struct bkPackage *)(*(int *)(root + 0x4c)), name, 9, 0xdefa);
		}
		CLU_DebugPrintf("--------------------------------\n");
	}
}

/* 0x00071bf0  ClassLib/GenericSamples.cpp (inferred)
Releases the music stream samples. */
void CLMusic_Release(void)
{
	int i;

	for (i = 0; i < CL_MUSIC_TRACK_COUNT; i++) {
		if (clu_music_samples[i] != 0)
			FUN_0004b6e0((struct bkResource *)clu_music_samples[i]);
	}
}
