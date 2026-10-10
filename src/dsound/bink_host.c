/* Started from ref/OpenCE (commit 231708d) port/linux/src/bink_null.c (CC0), which skipped every
   movie. openfrenzy: the entry points now play the movie, decoded by the host with FFmpeg
   (port/macos/host/host_bink.c, guest_host.h host_bink_*). A host built without FFmpeg
   (CMake OPENFRENZY_BINK_FFMPEG off) fails every open, which gives bink_null.c's behaviour:
   BinkOpen returns NULL and the game skips the movie.

   The XBE links RAD Bink statically (region LIB:bink 0x00057980-0x000703a0, 222 functions,
   its own DirectSound mixer at 0x00069df0 on). Game code calls these, all from BDISPLAY's
   bdVideoPlayStream (0x000463c0, src/bdisplay/BDISPLAY/xbVideo.c):
     0x0005a090  RADSetMemory(allocate, free): stores the two procedures that radmalloc
                 (0x0005a0b0) and radfree (0x0005a120) use; XbSymbolDatabase calls it
                 D3D_SetPushBufferSize, a false match on its two stores
     0x000591c0  BinkOpen(name, flags) ("Not a Bink file.")
     0x000585c0  BinkDoFrame(bink)
     0x00057fb0  BinkCopyToBuffer(bink, dest, pitch, height, x, y, flags): flags 3
                 (BINKSURFACE32: bytes B, G, R, X) into a BDTEXFMT_LIN_ARGB8888 texture
     0x00059e20  BinkNextFrame(bink)
     0x00058b60  BinkWait(bink): nonzero while it is too early for the next frame
     0x00058e00  BinkGetSummary(bink, summary) (with bdVideoPlayStream flag 2 only)
     0x00059f80  BinkClose(bink): stops the sound tracks, frees the buffers, clears the
                 0x404-byte handle, frees it
   The game reads only the first four words of the handle (width, height, frames, frame
   number). Bink played its sound itself (BinkOpenDirectSound, the default on the Xbox);
   here the movie's first audio track goes to a DirectSound stream (dsound_sdl.c).
   Declarations: dsound_xdk.h, src/bdisplay/bdisplay_types.h. */
/*
BINK_HOST.C

The Bink video SDK entry points the game uses, on the host's FFmpeg decoder.
*/

#include "platform.h"
#include "port_config.h"
#include "dsound_xdk.h"
#include "guest_host.h"
#include "vfs_files.h"
#include "xiso_fs.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BINK_DISC_SECTOR 2048ULL
#define BINK_AUDIO_CHUNK 32768

/* RAD's BINK structure, as far as the game reads it, then the port's fields */
struct BINK
{
	unsigned long Width;        /* 0x00 */
	unsigned long Height;       /* 0x04 */
	unsigned long Frames;       /* 0x08 */
	unsigned long FrameNum;     /* 0x0c: the frame to show next, from 1 */
	unsigned long LastFrameNum; /* 0x10 */
	unsigned long FrameRate;    /* 0x14 */
	unsigned long FrameRateDiv; /* 0x18 */

	unsigned int host;          /* host_bink_open's handle */
	unsigned int audio_channels;
	unsigned int audio_rate;
	LPDIRECTSOUND sound;
	LPDIRECTSOUNDSTREAM stream;
	long long start;            /* microseconds, at the first BinkDoFrame */
	unsigned long shown;        /* frames passed to BinkNextFrame */
	unsigned long decoded;
	unsigned long skipped;
};

typedef void *(__stdcall *bink_sound_system_open_proc)(unsigned long param);

static unsigned char audio_chunk[BINK_AUDIO_CHUNK];

static long long now_microseconds(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (long long)now.tv_sec * 1000000LL + now.tv_nsec / 1000;
}

/* the movie's frame time in microseconds */
static long long frame_microseconds(const struct BINK *bink)
{
	if (!bink->FrameRate)
		return 33333;
	return 1000000LL * (long long)(bink->FrameRateDiv ? bink->FrameRateDiv : 1) / (long long)bink->FrameRate;
}

/* opens the movie's audio as a 16-bit PCM DirectSound stream */
static void audio_open(struct BINK *bink)
{
	WAVEFORMATEX format;
	DSSTREAMDESC description;

	if (!bink->audio_channels || !bink->audio_rate)
		return;
	if (DirectSoundCreate(NULL, &bink->sound, NULL) != DS_OK)
	{
		bink->sound = NULL;
		return;
	}
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = (WORD)bink->audio_channels;
	format.nSamplesPerSec = bink->audio_rate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = (WORD)(2 * bink->audio_channels);
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	memset(&description, 0, sizeof(description));
	description.dwMaxAttachedPackets = 16;
	description.lpwfxFormat = &format;
	if (IDirectSound_CreateSoundStream(bink->sound, &description, &bink->stream, NULL) != DS_OK)
		bink->stream = NULL;
}

/* hands the decoded audio to the stream while it takes packets (the
mixer copies each packet, so one buffer serves) */
static void audio_pump(struct BINK *bink)
{
	DWORD status;
	XMEDIAPACKET packet;
	int bytes;

	if (!bink->stream)
		return;
	/* (completes played packets: the game's loop, which calls it, waits for the movie) */
	DirectSoundDoWork();
	for (;;)
	{
		if (IDirectSoundStream_GetStatus(bink->stream, &status) != DS_OK ||
			!(status & XMO_STATUSF_ACCEPT_INPUT_DATA))
			break;
		bytes = host_bink_read_audio(bink->host, audio_chunk, sizeof(audio_chunk));
		if (bytes <= 0)
			break;
		memset(&packet, 0, sizeof(packet));
		packet.pvBuffer = audio_chunk;
		packet.dwMaxSize = (DWORD)bytes;
		if (IDirectSoundStream_Process(bink->stream, &packet, NULL) != DS_OK)
			break;
	}
}

static void audio_close(struct BINK *bink)
{
	if (bink->stream)
		IDirectSoundStream_Release(bink->stream); /* flushes what has not played */
	if (bink->sound)
		IDirectSound_Release(bink->sound);
	bink->stream = NULL;
	bink->sound = NULL;
}

void __stdcall RADSetMemory(rad_memory_allocate_proc allocate, rad_memory_free_proc release)
{
	/* the handle is small and the decoder's memory is the host's */
	(void)allocate;
	(void)release;
}

void *__stdcall BinkOpenDirectSound(unsigned long param)
{
	(void)param;
	return NULL;
}

long __stdcall BinkSetSoundSystem(bink_sound_system_open_proc open, unsigned long param)
{
	(void)open;
	(void)param;
	return 0;
}

void __stdcall BinkSetIOSize(unsigned long io_size)
{
	(void)io_size;
}

/* 0x000591c0: a movie on the disc (d:\...); NULL if it cannot be played */
HBINK __stdcall BinkOpen(const char *name, unsigned long flags)
{
	struct xiso_fs *disc;
	struct xiso_entry entry;
	unsigned int info[7];
	const char *path;
	struct BINK *bink;
	unsigned int host;

	(void)flags;
	/* (OPENFRENZY_DEBUG_SKIP_VIDEOS=true: scripted test runs that expect the title at once) */
	if (config_boolean("debug.skip_videos"))
	{
		platform_log("bink: debug.skip_videos; \"%s\" skipped", name ? name : "");
		return NULL;
	}
	if (!name || !vfs_is_disc_path(name) || !(disc = vfs_disc()))
	{
		platform_log("bink: \"%s\" is not on the disc; skipped", name ? name : "");
		return NULL;
	}
	path = name[0] && name[1] == ':' ? name + 2 : name;
	if (xiso_fs_find(disc, path, &entry) != 0 || entry.is_directory)
	{
		platform_log("bink: \"%s\" not found; skipped", name);
		return NULL;
	}
	host = host_bink_open(disc->descriptor,
		(long long)(disc->partition + entry.sector * BINK_DISC_SECTOR), (long long)entry.size, info);
	if (!host)
	{
		platform_log("bink: cannot play \"%s\"; skipped", name);
		return NULL;
	}
	bink = calloc(1, sizeof(*bink));
	if (!bink)
	{
		host_bink_close(host);
		return NULL;
	}
	bink->host = host;
	bink->Width = info[0];
	bink->Height = info[1];
	bink->Frames = info[2];
	bink->FrameRate = info[3];
	bink->FrameRateDiv = info[4];
	bink->audio_channels = info[5];
	bink->audio_rate = info[6];
	bink->FrameNum = 1;
	audio_open(bink);
	platform_log("bink: \"%s\" %lux%lu, %lu frames at %lu/%lu fps, audio %u ch %u Hz%s", name,
		bink->Width, bink->Height, bink->Frames, bink->FrameRate, bink->FrameRateDiv,
		bink->audio_channels, bink->audio_rate, bink->audio_channels && !bink->stream ? " (no stream)" : "");
	return bink;
}

/* 0x00059f80 */
void __stdcall BinkClose(HBINK bink)
{
	if (!bink)
		return;
	platform_log("bink: closed after %lu of %lu frames, %lld ms", bink->shown, bink->Frames,
		bink->start ? (now_microseconds() - bink->start) / 1000 : 0LL);
	audio_close(bink);
	host_bink_close(bink->host);
	free(bink);
}

/* 0x000585c0: decodes the frame FrameNum */
long __stdcall BinkDoFrame(HBINK bink)
{
	if (!bink)
		return 0;
	if (!bink->start)
		bink->start = now_microseconds();
	if (host_bink_decode_frame(bink->host))
		bink->decoded++;
	audio_pump(bink);
	return 0;
}

/* 0x00059e20 */
void __stdcall BinkNextFrame(HBINK bink)
{
	if (!bink)
		return;
	bink->shown++;
	bink->LastFrameNum = bink->FrameNum;
	if (bink->FrameNum < bink->Frames)
		bink->FrameNum++;
}

/* 0x00058b60: nonzero until the frame time of the next frame has come */
long __stdcall BinkWait(HBINK bink)
{
	if (!bink || !bink->start)
		return 0;
	return now_microseconds() < bink->start + (long long)bink->shown * frame_microseconds(bink);
}

/* 0x00057fb0: the decoded frame into dest at (x, y). Only 32-bit surfaces
(BINKSURFACE32 3, BINKSURFACE32A 5) are written; the game uses 3. */
long __stdcall BinkCopyToBuffer(HBINK bink, void *destination, long destination_pitch,
	unsigned long destination_height, unsigned long destination_x, unsigned long destination_y,
	unsigned long flags)
{
	static int reported;
	unsigned long surface = flags & 0xf;
	unsigned long width, height;

	if (!bink || !destination || destination_pitch <= 0)
		return 0;
	if (surface != 3 && surface != 5)
	{
		if (!reported)
			platform_log("bink: BinkCopyToBuffer surface type %lu not supported", surface);
		reported = 1;
		return 0;
	}
	if ((long)(destination_x * 4) >= destination_pitch || destination_y >= destination_height)
		return 0;
	width = (unsigned long)destination_pitch / 4 - destination_x;
	height = destination_height - destination_y;
	if (!host_bink_copy_frame(bink->host,
		(unsigned char *)destination + destination_y * (unsigned long)destination_pitch + destination_x * 4,
		(int)destination_pitch, (int)width, (int)height))
		return 0;
	return 1;
}

/* 0x00058e00: BINKSUMMARY, the words bdVideoPlayStream prints */
void __stdcall BinkGetSummary(HBINK bink, void *summary)
{
	unsigned long *words = summary;
	long long elapsed;

	if (!bink || !words)
		return;
	elapsed = bink->start ? (now_microseconds() - bink->start) / 1000 : 0;
	words[0] = bink->Width;
	words[1] = bink->Height;
	words[2] = (unsigned long)elapsed;    /* total time, ms */
	words[3] = bink->FrameRate;           /* the file's rate */
	words[4] = bink->FrameRateDiv;
	words[5] = bink->FrameRate;           /* the rate played */
	words[6] = bink->FrameRateDiv;
	words[8] = bink->Frames;
	words[9] = bink->shown;
	words[10] = bink->skipped;
}

void __stdcall BinkGetRealtime(HBINK bink, void *realtime, unsigned long frame_count) { (void)bink; (void)realtime; (void)frame_count; }
