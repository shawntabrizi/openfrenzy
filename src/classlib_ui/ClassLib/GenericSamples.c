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

/* ---------- data (original .data) */

/* 0x0028fdec  commentator packages, one per screen */
static const char *const DAT_0028fdec[CL_COMMENTATOR_SET_COUNT] = {
	"com_frenzyintro", "com_ingame", "com_mapscreen", "com_scorescreen",
	"com_teamscreen", "com_winscreen", "com_zonescorescreen",
};

/* 0x0028fde8  commentator set loaded now (-1: none) */
int DAT_0028fde8 = -1;

/* 0x0028fe08  generic effects */
static const char *const DAT_0028fe08[CL_GENERIC_SAMPLE_COUNT] = {
	"horn.wav", "gen_token_01a.wav", "gen_token_02a.wav", "gen_token_03a.wav",
	"gen_switch_01a.wav", "gen_respawn_01a.wav", "gen_tile_01a.wav", "frontend_02a.wav",
	"frontend_02b.wav", "frontend_04b.wav", "frontend_04a.wav", "punchimpact1.wav",
	"punchimpact2.wav", "kickimpact1.wav", "kickimpact2.wav", "punchswipe1.wav",
	"punchswipe2.wav", "kickswipe1.wav", "kickswipe2.wav", "gen_token_02b.wav",
	"goal_scored_06.wav", "refbotbleep2.wav", "refbotexit.wav", "refbotenter.wav",
	"refbotturn.wav", "refbotlaser.wav",
};

/* 0x0028fe70  character voice kinds */
static const char *const DAT_0028fe70[CL_VOICE_KIND_COUNT] = {
	"jump", "land", "hurt", "knockovr", "score", "pickedup", "steal", "stolen",
	"powerup", "first", "event", "taunt", "revenge", "fall", "bad", "respawn",
	"comelast", "selected", "unselected",
};

/* 0x0028fec0  commentator kinds */
static const char *const DAT_0028fec0[CL_COMMENTARY_KIND_COUNT] = {
	"countdown", "go", "getready", "roundcomplete", "stop", "playerwins", "charname",
	"char1wins", "char2wins", "char3wins", "char4wins", "char5wins", "char6wins",
	"levelname", "zonename", "tiebreak", "draw_2way", "draw_3way", "draw_4way",
	"playerleads", "charnameleads", "charnamemightwin", "charnamejoins",
	"charnamegameover", "newchallenger", "charnameoverall", "newchampion", "almostover",
	"ffintro", "fuzionfrenzy", "teamup", "char1good", "char2good", "char3good",
	"char4good", "char5good", "char6good", "char1rubbish", "char2rubbish",
	"char3rubbish", "char4rubbish", "char5rubbish", "char6rubbish", "selectroute",
	"firststage", "endstage", "movestage", "outlandswrapup", "downtownwrapup",
	"zonewrapup", "sector9wrapup", "waterfrontwrapup", "powerstwrapup",
	"charnamewinsstage", "choosegame", "charnameout", "gamewrapup", "teamwins",
};

/* 0x0028ffa8 / 0x0028ffac  3D sample min / max distance */
float DAT_0028ffa8 = 4000.0f;
float DAT_0028ffac = 10000.0f;

/* 0x0028ffb0  sample length -> commentator delay factor */
static const float DAT_0028ffb0 = 1.75f;

/* 0x0028ffb4  channel of the commentator line that plays now (-1: none) */
int DAT_0028ffb4 = -1;

/* 0x0028ffb8  music track that plays now (-1: none) */
int DAT_0028ffb8 = -1;

/* 0x0028ffc0  music tracks (flags at 0x0028ffc4) */
const struct CLMusicTrack DAT_0028ffc0[CL_MUSIC_TRACK_COUNT] = {
	{ "dub", 0x00800000 },
	{ "fly", 0x00800000 },
	{ "geena", 0x00800000 },
	{ "zak", 0x00800000 },
	{ "jade", 0x00800000 },
	{ "jet", 0x00800000 },
	{ "blitz_cowboy", 0x003ffff0 },
	{ "boom", 0x0040000c },
	{ "chemical_brain", 0x003ffff0 },
	{ "hey_fatty", 0x003ffff0 },
	{ "hypnotise_her", 0x003ffff0 },
	{ "money_jar", 0x003ffff0 },
	{ "newyork1", 0x003ffff0 },
	{ "sidewinder", 0x003ffff0 },
	{ "wah_mix2", 0x003ffff0 },
	{ "mash_it_up", 0x003ffff0 },
	{ "atom_splitter", 0x003ffff0 },
	{ "one_two", 0x003ffff0 },
	{ "fuzionfrenzy", 0x00000002 },
	{ "radio_waves", 0x003ffff0 },
	{ "lullaby_for_broadmore", 0x003ffff0 },
	{ "sharon_tate's_late", 0x003ffff0 },
	{ "casino_royale", 0x003ffff0 },
	{ "dum_dum_boy2", 0x003ffff0 },
	{ "the_ease_of_tomorrow", 0x003ffff0 },
	{ "nightclub", 0x00000000 },
	{ "rok_yer_world", 0x003ffff0 },
	{ "endofgame", 0x00800000 },
};

/* ---------- data (original .bss) */

/* 0x002ea490  generic effect samples */
static int DAT_002ea490[CL_GENERIC_SAMPLE_COUNT];
/* 0x002ea4f8  queued commentator/voice lines */
static struct CLQueuedLine DAT_002ea4f8[5];
/* 0x002ea588  commentator sample sets */
static struct CLSampleSet DAT_002ea588[CL_COMMENTARY_KIND_COUNT];
/* 0x002eb150  character voice sample sets */
static struct CLSampleSet DAT_002eb150[CL_CHARACTER_COUNT][CL_VOICE_KIND_COUNT];
/* 0x002ec878  CharVoice package */
static int DAT_002ec878;
/* 0x002ec87c  commentator package */
static int DAT_002ec87c;
/* 0x002ec880  Packages\Welsh */
static int DAT_002ec880;
/* 0x002ec884  memory block that holds the largest commentator package */
static int DAT_002ec884;
/* 0x002ec888  queued lines */
static int DAT_002ec888;
/* 0x002ec890  music stream samples */
int DAT_002ec890[CL_MUSIC_TRACK_COUNT];

/* globals of other modules (tentative definitions) */
int DAT_00283d74;            /* Welsh commentary selected (core) */
int DAT_00283e0c;            /* commentator lines delay by their length (core data, 0) */
unsigned char DAT_002ba750[0x58]; /* the game mode stack object (core, thiscall FUN_0002b5b0) */

/* ---------- character voices */

/* 0x00071130  ClassLib/GenericSamples.cpp (inferred)
Forgets the voice samples of one character. */
void CLSamples_ClearCharacter(int character)
{
	int kind;
	int i;

	for (kind = 0; kind < CL_VOICE_KIND_COUNT; kind++) {
		for (i = 0; i < 10; i++)
			DAT_002eb150[character][kind].samples[i] = 0;
		DAT_002eb150[character][kind].count = 0;
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
		set = &DAT_002eb150[character][kind];
		set->count = 0;
		set->last = -1;
		set->cooldown = 0.0f;
		for (i = 0; i < 10; i++)
			set->samples[i] = 0;
		while (set->count < 10) {
			sprintf(probe, "%s\\%s%02d.wav", DAT_002906f8[character].name, DAT_0028fe70[kind],
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
				sprintf(name, "%s\\%s%02d.wav", DAT_002906f8[character].name, DAT_0028fe70[kind], i);
				_strlwr(name);
				set->samples[i] = FUN_0004bb30(0, name, 2, 0xdefa);
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
		DAT_002ea490[i] = FUN_0004bb30(0, (char *)DAT_0028fe08[i], 2, 0xdefa);
		if (DAT_002ea490[i] == 0)
			CLU_DebugPrintf("Couldn't Load %s\n", DAT_0028fe08[i]);
	}
	sprintf(path, "%s\\CharVoice", DAT_002ba544);
	DAT_002ec878 = (int)FUN_0004fd50(0, path, 0);
	if (DAT_002ec878 == 0) {
		CLU_DebugPrintf(
			"CGameMode_Root::Pushed *** Could not load package 'Packages\\Character Voice SFX' ***\n");
		return;
	}
	FUN_0004b870(DAT_002ec878, 0xffffffff, 0xdefa, 0);
	for (character = 0; character < CL_CHARACTER_COUNT; character++) {
		for (kind = 0; kind < CL_VOICE_KIND_COUNT; kind++)
			for (i = 0; i < 10; i++)
				DAT_002eb150[character][kind].samples[i] = 0;
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
		return FUN_00054ba0(sample, volume, 0, arg3, flags);
	return FUN_00054c30(sample, pos, volume, arg3, flags, DAT_0028ffa8, DAT_0028ffac);
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
				DAT_002eb150[character][kind].samples[i] = 0;
			DAT_002eb150[character][kind].count = 0;
		}
	}
	for (kind = 0; kind < CL_COMMENTARY_KIND_COUNT; kind++) {
		for (i = 0; i < 10; i++)
			DAT_002ea588[kind].samples[i] = 0;
		DAT_002ea588[kind].count = 0;
	}
	if (DAT_002ec878 != 0)
		FUN_0004fe80((int *)DAT_002ec878);
	if (DAT_002ec87c != 0)
		FUN_0004fe80((int *)DAT_002ec87c);
	if (DAT_002ec884 != 0) {
		FUN_0004c110(DAT_002ec884);
		DAT_002ec884 = 0;
	}
	if (DAT_002ec880 != 0)
		FUN_0004fe80((int *)DAT_002ec880);
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
	if (DAT_002ec884 != 0)
		return;
	for (i = 0; i < CL_COMMENTATOR_SET_COUNT; i++) {
		sprintf(path, "%s\\%s", DAT_002ba544, DAT_0028fdec[i]);
		size = ((int (*)(const char *, int))FUN_0004fef0)(path, 1);
		if (largest < size)
			largest = size;
	}
	DAT_002ec884 = (int)FUN_0004bf10(largest, "C:\\BlitzParty\\Game\\Src\\ClassLib\\GenericSamples.cpp",
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
		set = &DAT_002ea588[kind];
		set->count = 0;
		set->last = -1;
		for (i = 0; i < 10; i++)
			set->samples[i] = 0;
		while (set->count < 10) {
			sprintf(probe, "%s%02d.wav", DAT_0028fec0[kind], set->count);
			_strlwr(probe);
			if (FUN_0004bb30(0, probe, 2, 0xdefa) == 0)
				break;
			set->count = set->count + 1;
		}
		if (set->count != 0) {
			for (i = 0; i < set->count; i++) {
				sprintf(name, "%s%02d.wav", DAT_0028fec0[kind], i);
				_strlwr(name);
				set->samples[i] = FUN_0004bb30(0, name, 2, 0xdefa);
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

	if (DAT_002ec888 < 5) {
		line = &DAT_002ea4f8[DAT_002ec888];
		line->set = set;
		line->kind = kind;
		line->choice = choice;
		line->volume = volume;
		line->flags = flags;
		line->allow_repeat = allow_repeat;
		line->wait_channel = wait_channel;
		DAT_002ec888 = DAT_002ec888 + 1;
	}
}

/* 0x00071690  ClassLib/GenericSamples.cpp (inferred) */
void CLSamples_ClearQueue(void)
{
	DAT_002ec888 = 0;
}

/* 0x000716a0  ClassLib/GenericSamples.cpp (inferred)
Loads the Welsh commentary package (replaces the language commentary). */
void CLSamples_LoadWelsh(void)
{
	if (DAT_002ec880 != 0)
		FUN_0004fe80((int *)DAT_002ec880);
	DAT_002ec880 = (int)FUN_0004fd50(0, "Packages\\Welsh", 0);
	if (DAT_002ec880 != 0) {
		FUN_0004b870(DAT_002ec880, 0xffffffff, 0xdefa, 0);
		CLSamples_LoadCommentary();
	}
}

/* 0x000716f0  ClassLib/GenericSamples.cpp (inferred)
1 when no commentator line is queued or playing. */
int CLSamples_CommentaryIdle(void)
{
	if (DAT_002ec888 != 0)
		return 0;
	if (DAT_0028ffb4 != -1 && FUN_000548d0(DAT_0028ffb4) != 0)
		return 0;
	return 1;
}

/* 0x00071720  ClassLib/GenericSamples.cpp (inferred)
Plays generic effect `index` (see DAT_0028fe08). */
int CLSamples_PlayGeneric(int index, int volume, int arg3, unsigned int flags, float *pos)
{
	int sample = DAT_002ea490[index];

	if (sample == 0)
		return -1;
	if (pos == NULL)
		return FUN_00054ba0(sample, volume, 0, arg3, flags);
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
		set = &DAT_002ea588[kind];
	else
		set = &DAT_002eb150[0][0] + (set_index * CL_VOICE_KIND_COUNT + kind);
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
			return CLSamples_Play(sample, volume, (int)((double)*(int *)(sample + 0x20) * (double)DAT_0028ffb0),
					      flags, pos);
		}
		return CLSamples_Play(set->samples[choice], volume, 0, flags, pos);
	}
	return -1;
}

/* 0x00071880  ClassLib/GenericSamples.cpp (inferred)
Loads the commentator package of a screen (0-6, see DAT_0028fdec). With Welsh
commentary the Welsh package is used instead. */
void CLSamples_LoadCommentatorSet(int set)
{
	char path[128];

	if (DAT_00283d74 == 0) {
		if (DAT_002ec880 != 0) {
			FUN_0004fe80((int *)DAT_002ec880);
			DAT_002ec880 = 0;
			CLSamples_AllocCommentatorMemory();
			CLSamples_LoadCommentary();
		}
		if (set < 0 || 6 < set || set == DAT_0028fde8)
			return;
	} else {
		CLSamples_LoadWelsh();
	}
	CLSamples_AllocCommentatorMemory();
	if (DAT_002ec87c != 0) {
		FUN_0004e010(FUN_0004dce0((undefined4 *)DAT_002ec87c));
		FUN_0004fe80((int *)DAT_002ec87c);
	}
	sprintf(path, "%s\\%s", DAT_002ba544, DAT_0028fdec[set]);
	DAT_002ec87c = (int)FUN_0004fd50(0, path, DAT_002ec884);
	if (DAT_002ec87c == 0) {
		CLU_DebugPrintf("LoadCommentatorSampleSet *** Could not load package *%s **\n", DAT_0028fdec[set]);
		DAT_0028fde8 = -1;
		CLSamples_LoadCommentary();
		return;
	}
	DAT_0028fde8 = set;
	FUN_0004b870(DAT_002ec87c, 0xffffffff, 0xdefa, 0);
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
	struct CLSampleSet *set = &DAT_002eb150[0][0] + (character * CL_VOICE_KIND_COUNT + kind);

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

	if (DAT_002ec888 == 0 || FUN_000548d0(DAT_002ea4f8[0].wait_channel) != 0)
		return;
	if (DAT_002ea4f8[0].set == -1)
		channel = CLSamples_PlayCommentary(DAT_002ea4f8[0].kind, DAT_002ea4f8[0].choice,
						   DAT_002ea4f8[0].volume, DAT_002ea4f8[0].flags,
						   DAT_002ea4f8[0].allow_repeat, NULL);
	else
		channel = CLSamples_PlayFromSet(DAT_002ea4f8[0].set, DAT_002ea4f8[0].kind, NULL,
						DAT_002ea4f8[0].volume, DAT_002ea4f8[0].flags,
						DAT_002ea4f8[0].choice, DAT_002ea4f8[0].allow_repeat);
	DAT_0028ffb4 = channel;
	if (1 < DAT_002ec888) {
		for (i = 1; i < DAT_002ec888; i++) {
			if (DAT_002ea4f8[i].wait_channel == -DAT_002ea4f8[0].wait_channel)
				DAT_002ea4f8[i].wait_channel = channel;
			DAT_002ea4f8[i - 1] = DAT_002ea4f8[i];
		}
	}
	DAT_002ec888 = DAT_002ec888 + -1;
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
			set = &DAT_002eb150[character][kind];
			if (set->cooldown != 0.0f) {
				set->cooldown = set->cooldown - DAT_0028fda8;
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

	root = FUN_0002b5b0((int)DAT_002ba750, (undefined4)"Root");
	if (*(int *)(root + 0x4c) != 0) {
		CLU_DebugPrintf("InitMusic() --------------------\n");
		for (i = 0; i < CL_MUSIC_TRACK_COUNT; i++) {
			CLU_DebugPrintf("%s\n", DAT_0028ffc0[i].name);
			sprintf(name, "%s.wma", DAT_0028ffc0[i].name);
			DAT_002ec890[i] = FUN_0004bb30(*(int *)(root + 0x4c), name, 9, 0xdefa);
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
		if (DAT_002ec890[i] != 0)
			FUN_0004b6e0((int *)DAT_002ec890[i]);
	}
}
