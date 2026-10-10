/*
TEST_DSOUND_GUEST.C

Guest test of the dsound module (DirectSound buffers, the WMA media object) as the game
uses them; built and run by src/dsound/tests/run_tests.sh (a guest image like the smoke
test, run in the host). Exit 0 when every check passes, else the number of the first
failed check; each check prints a line.

The mixer runs on its own clock (no audio device: OPENFRENZY_AUDIO_ENABLED=false), in real
time at 48 kHz, so the checks on play cursors wait and allow a margin.

1  DirectSoundCreate, GetCaps, the listener (SetAllParameters as 0x000559c0 fills it)
2  a 2D one-shot: 16-bit mono 22050 Hz, SetFrequency(44100) (0x00055550): the cursor
   moves at 44.1 kHz, GetStatus is PLAYING, then 0 after the end (0x00055720 frees it)
3  a 3D one-shot (0x000557d0): DSBCAPS_CTRL3D, position, velocity, distances, play
4  a looping Xbox ADPCM buffer (0x000573e0): it wraps, Stop holds the cursor,
   SetCurrentPosition moves it, Lock splits a region at the end
5  the WMA music as BSOUND/xbAudioStreamWMA.cpp drives it: stream 0 of Music.xbp read from
   the disc image through a callback over a 128 KB ring (0x00056780), WmaCreateDecoder
   (0x00057060), a looping 256 KB 16-bit stereo buffer at the returned rate, Process into
   it ahead of GetCurrentPosition (0x00056d20) while it plays, then the whole stream
   decoded; writes the samples to OPENFRENZY_DSOUND_TEST_PCM for run_tests.sh to compare with the
   native decoder's (to 1 LSB: musl's sin and cos are not Apple's) (tests/test_wma.c)
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "xtl.h"
#include "dsound_xdk.h"
#include "xiso_fs.h"

static int failures;

static void check(int number, int passed, const char *what)
{
	printf("check %d: %s: %s\n", number, passed ? "ok" : "FAIL", what);
	if (!passed && !failures)
		failures = number;
}

static void sleep_ms(long milliseconds)
{
	struct timespec time = { milliseconds / 1000, (milliseconds % 1000) * 1000000L };

	nanosleep(&time, NULL);
}

static DWORD cursor(LPDIRECTSOUNDBUFFER buffer)
{
	DWORD play = 0, write = 0;

	IDirectSoundBuffer_GetCurrentPosition(buffer, &play, &write);
	return play;
}

static DWORD status(LPDIRECTSOUNDBUFFER buffer)
{
	DWORD value = 0xffffffff;

	IDirectSoundBuffer_GetStatus(buffer, &value);
	return value;
}

static long now_ms(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec * 1000L + time.tv_nsec / 1000000L;
}

/* ---------- 5: the music */

struct ring
{
	const unsigned char *file;   /* the .wma in the package */
	unsigned long size;
	unsigned char window[0x20000 + 4096];
	unsigned long reads;
};

/* the game's callback (0x00056780) gives a pointer into its ring of the file; here
the bytes are copied to a window, as it copies a read across the ring's end */
static HRESULT CALLBACK music_read(LPVOID context, DWORD offset, DWORD bytes, LPVOID *data)
{
	struct ring *ring = context;

	ring->reads++;
	if (offset >= ring->size || bytes > sizeof(ring->window))
		return E_FAIL;
	if (bytes > ring->size - offset)
		bytes = ring->size - offset;
	memcpy(ring->window, ring->file + offset, bytes);
	*data = ring->window;
	return (HRESULT)bytes;
}

static double music_energy;

/* FNV-1a over the bytes, and the samples' energy for the RMS */
static unsigned int fnv1a(unsigned int hash, const void *data, unsigned long size)
{
	const unsigned char *bytes = data;
	const short *samples = data;
	unsigned long i;

	for (i = 0; i < size; i++)
		hash = (hash ^ bytes[i]) * 16777619u;
	for (i = 0; i < size / 2; i++)
		music_energy += (double)samples[i] * samples[i];
	return hash;
}

static void music_check(LPDIRECTSOUND sound)
{
	static const unsigned char asf_guid[16] =
		{ 0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C };
	const char *path = getenv("OPENFRENZY_ISO");
	struct xiso_fs fs;
	struct xiso_entry entry;
	unsigned char *package;
	struct ring *ring;
	unsigned long offset, header_size = 0, data_size = 0;
	WAVEFORMATEX format, pcm_format;
	XMediaObject *decoder = NULL;
	LPDIRECTSOUNDBUFFER buffer = NULL;
	DSBUFFERDESC description;
	unsigned char *pcm;
	DWORD completed = 0, write_position = 0, total = 0;
	XMEDIAPACKET packet;
	HRESULT result;
	unsigned int hash = 2166136261u;
	long start;
	DWORD moved;
	int loops = 0;
	const char *dump_path = getenv("OPENFRENZY_DSOUND_TEST_PCM");
	FILE *dump = dump_path ? fopen(dump_path, "wb") : NULL;

	if (!path || xiso_fs_open(&fs, path))
	{
		check(5, 0, "open the disc image (OPENFRENZY_ISO)");
		return;
	}
	if (xiso_fs_find(&fs, "Packages\\Music.xbp", &entry) || !(package = malloc(entry.size))
		|| xiso_fs_read(&fs, &entry, 0, package, entry.size) != (long)entry.size)
	{
		check(5, 0, "read Packages\\Music.xbp from the disc image");
		return;
	}
	for (offset = 0; offset + 30 <= entry.size; offset += 2048)
		if (!memcmp(package + offset, asf_guid, 16))
			break;
	memcpy(&header_size, package + offset + 16, 4);
	memcpy(&data_size, package + offset + header_size + 16, 4);
	ring = calloc(1, sizeof(*ring));
	ring->file = package + offset;
	ring->size = header_size + data_size;
	check(5, offset + 30 <= entry.size && ring->size <= entry.size - offset, "find the first WMA stream in Music.xbp");

	memset(&format, 0xcc, sizeof(format));
	result = WmaCreateDecoder(music_read, ring, 0, &format, &decoder);
	check(5, result >= 0 && decoder, "WmaCreateDecoder");
	if (result < 0 || !decoder)
		return;
	check(5, format.wFormatTag == WAVE_FORMAT_PCM && format.nChannels == 2 && format.nSamplesPerSec == 44100
		&& format.wBitsPerSample == 16 && format.nBlockAlign == 4 && format.cbSize == 0,
		"the decoded format is 16-bit stereo 44.1 kHz PCM");

	/* the buffer as bAudioStreamCreateSoundBuffer (0x000569b0) makes it */
	memset(&pcm_format, 0, sizeof(pcm_format));
	pcm_format.wFormatTag = WAVE_FORMAT_PCM;
	pcm_format.nChannels = 2;
	pcm_format.nSamplesPerSec = 22050; /* the game's default; 0x00057060 sets the real one */
	pcm_format.wBitsPerSample = 16;
	pcm_format.nBlockAlign = 4;
	pcm_format.nAvgBytesPerSec = 22050 * 4;
	pcm_format.cbSize = 0x12;
	memset(&description, 0, sizeof(description));
	description.dwSize = sizeof(description);
	description.dwFlags = 0x1a0;
	description.lpwfxFormat = &pcm_format;
	result = IDirectSound_CreateSoundBuffer(sound, &description, &buffer, NULL);
	check(5, result >= 0 && buffer, "create the music buffer");
	pcm = calloc(1, 0x40000);
	IDirectSoundBuffer_SetBufferData(buffer, pcm, 0x40000);
	IDirectSoundBuffer_SetFrequency(buffer, format.nSamplesPerSec);

	/* fill the ring once (0x00056d20 decodes ahead of the cursor), then play and
	keep it filled for a second */
	memset(&packet, 0, sizeof(packet));
	packet.pvBuffer = pcm;
	packet.dwMaxSize = 0x2f000;
	packet.pdwCompletedSize = &completed;
	result = decoder->lpVtbl->Process(decoder, NULL, &packet);
	check(5, result >= 0 && completed == 0x2f000, "Process fills the 188 KB the game asks for first");
	write_position = completed;
	total = completed;
	hash = fnv1a(hash, pcm, completed);
	if (dump)
		fwrite(pcm, 1, completed, dump);
	IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);
	start = now_ms();
	while (now_ms() - start < 1000)
	{
		DWORD play = cursor(buffer);
		/* decode up to 0x2f000 bytes past the play cursor, in 8 KB steps, as the game */
		DWORD target = ((play + 0x2f000) & ~0x1fffu) & 0x3ffff;

		while (write_position != target)
		{
			DWORD bytes = target > write_position ? target - write_position : 0x40000 - write_position;

			completed = 0;
			packet.pvBuffer = pcm + write_position;
			packet.dwMaxSize = bytes;
			decoder->lpVtbl->Process(decoder, NULL, &packet);
			hash = fnv1a(hash, pcm + write_position, completed);
			if (dump)
				fwrite(pcm + write_position, 1, completed, dump);
			total += completed;
			write_position = (write_position + bytes) & 0x3ffff;
			loops++;
		}
		sleep_ms(20);
	}
	moved = cursor(buffer);
	/* about 1 s at 44.1 kHz, 4 bytes a frame */
	check(5, status(buffer) == (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING) && moved > 4 * 44100 * 6 / 10
		&& moved < 4 * 44100 * 15 / 10, "the music plays at 44.1 kHz from the ring the game refills");
	printf("  music: played to byte %lu in 1 s, %d refills\n", (unsigned long)moved, loops);
	IDirectSoundBuffer_Stop(buffer);

	/* the rest of the stream, to its end (Process gives fewer bytes) */
	for (;;)
	{
		completed = 0;
		packet.pvBuffer = pcm;
		packet.dwMaxSize = 0x10000;
		decoder->lpVtbl->Process(decoder, NULL, &packet);
		hash = fnv1a(hash, pcm, completed);
		if (dump)
			fwrite(pcm, 1, completed, dump);
		total += completed;
		if (completed < 0x10000)
			break;
	}
	check(5, total / 4 > 44100 * 10, "the whole stream decodes (more than 10 s)");
	printf("  music: %lu frames, rms %.3f, hash %08x, %lu reads\n", (unsigned long)(total / 4),
		total ? sqrt(music_energy / (total / 2)) : 0.0, hash, ring->reads);
	check(5, decoder->lpVtbl->Discontinuity(decoder) >= 0 && decoder->lpVtbl->Release(decoder) == 0,
		"Discontinuity and Release");
	check(5, IDirectSoundBuffer_Release(buffer) == 0, "release the music buffer");
	if (dump)
		fclose(dump);
	free(pcm);
	free(ring);
	free(package);
	xiso_fs_close(&fs);
}

int main(void)
{
	LPDIRECTSOUND sound = NULL;
	DSCAPS caps;
	DS3DLISTENER listener;
	WAVEFORMATEX format;
	DSBUFFERDESC description;
	LPDIRECTSOUNDBUFFER buffer = NULL, buffer3d = NULL, looping = NULL;
	short *tone;
	unsigned char *adpcm;
	DWORD position, held, i;
	long start, elapsed;
	LPVOID part1, part2;
	DWORD size1, size2;

	setvbuf(stdout, NULL, _IOLBF, 0);
	/* 1 */
	check(1, DirectSoundCreate(NULL, &sound, NULL) >= 0 && sound, "DirectSoundCreate");
	memset(&caps, 0, sizeof(caps));
	check(1, IDirectSound_GetCaps(sound, &caps) >= 0 && caps.dwFree2DBuffers > 0, "GetCaps");
	memset(&listener, 0, sizeof(listener));
	listener.dwSize = sizeof(listener);
	listener.vOrientFront.z = 1.0f;
	listener.vOrientTop.y = 1.0f;
	listener.flDistanceFactor = 3.048f;
	listener.flRolloffFactor = 1.0f;
	listener.flDopplerFactor = 8.0f;
	check(1, sizeof(DS3DLISTENER) == 0x40, "DS3DLISTENER is 0x40 bytes, as the game's dwSize");
	check(1, IDirectSound_SetAllParameters(sound, &listener, 1) >= 0
		&& IDirectSound_CommitDeferredSettings(sound) >= 0, "SetAllParameters");

	/* 2 */
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = 22050;
	format.wBitsPerSample = 16;
	format.nBlockAlign = 2;
	format.nAvgBytesPerSec = 44100;
	memset(&description, 0, sizeof(description));
	description.dwSize = sizeof(description);
	description.lpwfxFormat = &format;
	check(2, IDirectSound_CreateSoundBuffer(sound, &description, &buffer, NULL) >= 0 && buffer, "create a 2D buffer");
	/* 0.5 s of a 440 Hz tone at 22050 Hz: 11025 frames; at 44100 Hz it lasts 0.25 s */
	tone = malloc(11025 * sizeof(short));
	for (i = 0; i < 11025; i++)
		tone[i] = (short)(8000.0 * sin(2 * 3.14159265358979 * 440.0 * i / 22050.0));
	check(2, IDirectSoundBuffer_SetBufferData(buffer, tone, 11025 * sizeof(short)) >= 0, "SetBufferData");
	IDirectSoundBuffer_SetVolume(buffer, -600);
	check(2, IDirectSoundBuffer_SetFrequency(buffer, 44100) >= 0, "SetFrequency 44100");
	check(2, status(buffer) == 0 && cursor(buffer) == 0, "not playing before Play");
	start = now_ms();
	IDirectSoundBuffer_Play(buffer, 0, 0, 0);
	check(2, status(buffer) == DSBSTATUS_PLAYING, "GetStatus says PLAYING");
	sleep_ms(100);
	position = cursor(buffer);
	elapsed = now_ms() - start;
	/* frames = bytes / 2; expect about elapsed ms at 44.1 kHz, within the mixer's 10 ms
	steps and scheduling */
	printf("  2D: %lu frames in %ld ms (%.0f frames/s)\n", (unsigned long)(position / 2), elapsed,
		position / 2 * 1000.0 / elapsed);
	check(2, position / 2 > (DWORD)(44.1 * (elapsed - 40)) && position / 2 < (DWORD)(44.1 * (elapsed + 40)),
		"the play cursor moves at the frequency set (44.1 kHz, not the data's 22.05)");
	sleep_ms(300);
	check(2, status(buffer) == 0, "GetStatus is 0 once the one-shot has played (0x00055720 frees it then)");
	check(2, cursor(buffer) == 11025 * sizeof(short), "the cursor stops at the end");
	check(2, IDirectSoundBuffer_Release(buffer) == 0, "Release");

	/* 3 */
	description.dwFlags = DSBCAPS_CTRL3D;
	check(3, IDirectSound_CreateSoundBuffer(sound, &description, &buffer3d, NULL) >= 0, "create a 3D buffer");
	IDirectSoundBuffer_SetBufferData(buffer3d, tone, 11025 * sizeof(short));
	check(3, IDirectSoundBuffer_SetPosition(buffer3d, 10.0f, 0.0f, 5.0f, 1) >= 0
		&& IDirectSoundBuffer_SetVelocity(buffer3d, 0.0f, 0.0f, 0.0f, 1) >= 0
		&& IDirectSoundBuffer_SetMinDistance(buffer3d, 2.0f, 1) >= 0
		&& IDirectSoundBuffer_SetMaxDistance(buffer3d, 100.0f, 1) >= 0
		&& IDirectSound_CommitDeferredSettings(sound) >= 0, "the 3D settings");
	IDirectSoundBuffer_Play(buffer3d, 0, 0, 0);
	sleep_ms(60);
	check(3, status(buffer3d) == DSBSTATUS_PLAYING && cursor(buffer3d) > 0, "a 3D buffer plays");
	/* the game stops and releases a voice it cuts short (0x000556e0) */
	check(3, IDirectSoundBuffer_Stop(buffer3d) >= 0 && status(buffer3d) == 0, "Stop");
	check(3, IDirectSoundBuffer_Release(buffer3d) == 0, "Release");

	/* 4: 20 blocks of stereo Xbox ADPCM (72 bytes, 64 frames each): 1280 frames */
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_XBOX_ADPCM;
	format.nChannels = 2;
	format.nSamplesPerSec = 32000;
	format.wBitsPerSample = 4;
	format.nBlockAlign = 72;
	description.dwFlags = 0x1a0;
	check(4, IDirectSound_CreateSoundBuffer(sound, &description, &looping, NULL) >= 0, "create an ADPCM buffer");
	adpcm = calloc(20, 72);
	for (i = 0; i < 20; i++)
	{
		/* a block header per channel: first sample 1000 * block, step index 10 */
		adpcm[i * 72 + 0] = (unsigned char)((1000 * i) & 0xff);
		adpcm[i * 72 + 1] = (unsigned char)((1000 * i) >> 8);
		adpcm[i * 72 + 2] = 10;
		adpcm[i * 72 + 4] = adpcm[i * 72 + 0];
		adpcm[i * 72 + 5] = adpcm[i * 72 + 1];
		adpcm[i * 72 + 6] = 10;
		memset(adpcm + i * 72 + 8, 0x17, 64);
	}
	IDirectSoundBuffer_SetBufferData(looping, adpcm, 20 * 72);
	IDirectSoundBuffer_Play(looping, 0, 0, DSBPLAY_LOOPING);
	sleep_ms(150);
	/* 150 ms at 32 kHz is 4800 frames, almost 4 times round */
	check(4, status(looping) == (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING), "a looping buffer keeps playing past its end");
	position = cursor(looping);
	check(4, position < 20 * 72 && position % 72 == 0, "the ADPCM cursor is a block boundary in the data");
	IDirectSoundBuffer_Stop(looping);
	held = cursor(looping);
	sleep_ms(50);
	check(4, status(looping) == 0 && cursor(looping) == held, "Stop holds the cursor");
	check(4, IDirectSoundBuffer_SetCurrentPosition(looping, 5 * 72) >= 0 && cursor(looping) == 5 * 72,
		"SetCurrentPosition");
	check(4, IDirectSoundBuffer_Lock(looping, 18 * 72, 4 * 72, &part1, &size1, &part2, &size2, 0) >= 0
		&& part1 == adpcm + 18 * 72 && size1 == 2 * 72 && part2 == adpcm && size2 == 2 * 72
		&& IDirectSoundBuffer_Unlock(looping, part1, size1, part2, size2) >= 0, "Lock splits at the end");
	check(4, IDirectSoundBuffer_Release(looping) == 0, "Release");

	/* 5 */
	music_check(sound);

	check(6, IDirectSound_Release(sound) == 0, "IDirectSound_Release");
	free(tone);
	free(adpcm);
	printf("dsound guest test: %s\n", failures ? "FAILED" : "passed");
	return failures;
}
