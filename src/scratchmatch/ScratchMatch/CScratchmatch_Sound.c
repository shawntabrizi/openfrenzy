/*
ScratchMatch/CScratchmatch_Sound.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CScratchmatch_Sound.cpp
(module scratchmatch)

The sounds of Scratch Match: ten samples (crowd, bonus, platforms, booing, click), the music
of the variation (a WMA stream from packages\scratchmusic, "mpAudioStream") and the
characters' voices.
*/

#include "scratchmatch.h"

/* 0x001bb340  ScratchMatch/CScratchmatch_Sound.cpp: opens the music package */
void SMSound_OpenMusicPackage(void)
{
	sm_MusicPackage = bkOpenPackage("packages\\scratchmusic");
	if (sm_MusicPackage == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Sound.cpp", 0x3c, "POINTER ASSERTION FAILURE: mpChoonePakIndex");
}

/* 0x001bb370  ScratchMatch/CScratchmatch_Sound.cpp (bracketed): plays sample `index` at the
crowd volume */
void SMSound_PlaySFX(int index)
{
	bsPlaySample(sm_SFX[index], sm_CrowdVolume, 0, 0, 1);
}

/* 0x001bb3a0  ScratchMatch/CScratchmatch_Sound.cpp (bracketed): stops sample `index` */
void SMSound_StopSFX(int index)
{
	bsStopSample(sm_SFX[index]);
}

/* 0x001bb3c0  ScratchMatch/CScratchmatch_Sound.cpp (bracketed): a voice line of `kind` by the
character of `player`, at the speech volume */
void SMSound_PlayVoice(int kind, int player)
{
	float pos[3];

	pos[0] = 0.0f;
	pos[1] = 0.0f;
	pos[2] = 320.0f;
	CLSamples_PlayVoice((char)sm_PlayerRecord(player)[0], kind, pos, sm_SpeechVolume, 0.0f, 0, -1, 1);
}

/* 0x001bb410  ScratchMatch/CScratchmatch_Sound.cpp: starts the music stream (and remembers
its bits per sample / frequency ratio) */
void SMSound_StartMusic(void)
{
	if (sm_Music == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Sound.cpp", 0x9f, "POINTER ASSERTION FAILURE: mpAudioStream");
	bsGetAudioStreamInfo(sm_Music, &sm_MusicFrequency, &sm_MusicBits, NULL, NULL, NULL, NULL);
	sm_MusicRatio = (float)sm_MusicBits / (float)sm_MusicFrequency;
	bsPlayAudioStream(sm_Music, 0x20, 0xff, 0, 0, NULL, 0);
}

/* 0x001bb480  ScratchMatch/CScratchmatch_Sound.cpp (bracketed): resumes the music if it is
paused (flag 2) */
void SMSound_ResumeMusicIfPaused(void)
{
	if (bsAudioStreamTestFlags(sm_Music, 2) != 0)
		bsResumeAudioStream(sm_Music, 0x10);
}

/* 0x001bb4b0  ScratchMatch/CScratchmatch_Sound.cpp (bracketed) */
void SMSound_StopMusic(void)
{
	bsStopAudioStream(sm_Music);
}

/* 0x001bb4c0  ScratchMatch/CScratchmatch_Sound.cpp (bracketed) */
void SMSound_PauseMusic(void)
{
	bsPauseAudioStream(sm_Music, 0x10);
}

/* 0x001bb4e0  ScratchMatch/CScratchmatch_Sound.cpp (bracketed): resumes the music after the
intro */
void SMSound_ResumeMusic(void)
{
	if (sm_State > 0) {
		bsResumeAudioStream(sm_Music, 0x10);
		DirectSoundDoWork();
	}
}

/* 0x001bb500  ScratchMatch/CScratchmatch_Sound.cpp: loads the music of the variation and the
ten samples, starts the music and sets the volumes */
void SMSound_Load(void)
{
	unsigned int i;

	if (sm_Package == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Sound.cpp", 0x44, "POINTER ASSERTION FAILURE: scPakIndex");
	bkPrintf("Loading sounds...\n");
	sm_Music = (struct BAudioStream *)bLoadResource(sm_MusicPackage, sm_MusicNames[sm_GameVariation], 9, 0xdefa);
	if (sm_Music == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Sound.cpp", 0x56, "POINTER ASSERTION FAILURE: mpAudioStream");
	for (i = 0; i < 10; i++) {
		sm_SFX[i] = (struct BSample *)bLoadResource(sm_Package, sm_SampleNames[i], 2, 0xdefa);
		if (sm_SFX[i] == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Sound.cpp", 0x5d, "POINTER ASSERTION FAILURE: mpSFX[ ulFileIndex ]");
	}
	bkPrintf("Loaded sounds\n");
	SMSound_StartMusic();
	sm_MusicVolume = 0xa8;
	sm_CrowdVolume = 0xc0;
	sm_ScratchVolume = 0xff;
	sm_SpeechVolume = 0xa8;
}

/* 0x001bb5f0  (gap; after the sound file's tagged functions, frees what SMSound_Load and
SMSound_OpenMusicPackage made): frees the music stream and closes the music package */
void SMSound_Free(void)
{
	if (sm_Music != NULL) {
		bkDeleteResource((struct bkResource *)sm_Music);
		sm_Music = NULL;
	}
	if (sm_MusicPackage != NULL) {
		bkClosePackage(sm_MusicPackage);
		sm_MusicPackage = NULL;
	}
}
