/*
ENGINE_MISC_SOUND.H

Babel BSOUND (C:\Babel\Xbox\Src\BSOUND\): sample channels (xbSound?), audio streams
(xbAudioStream.cpp, xbAudioStreamWMA.cpp, xbAudioStreamADPCM.cpp) and soundtrack play
lists (xbPlayList.cpp). Also the XDK declarations this module needs that the shared
XDK headers lack (engine_misc/BSOUND/platform_fallback.c has weak stand-ins for the
ones the platform layer does not implement yet).

Owner: engine_misc.
*/

#ifndef OPENFRENZY_ENGINE_MISC_SOUND_H
#define OPENFRENZY_ENGINE_MISC_SOUND_H

#include "xtl.h"

/* ---------- XDK pieces missing from src/runtime/include/xdk */

/* DS3DLISTENER (IDirectSound_SetAllParameters): 0x40 bytes */
struct EmDS3DListener {
	DWORD dwSize;
	float vPosition[3];
	float vVelocity[3];
	float vOrientFront[3];
	float vOrientTop[3];
	float flDistanceFactor;
	float flRolloffFactor;
	float flDopplerFactor;
};

/* DSBPOSITIONNOTIFY (IDirectSoundBuffer_SetNotificationPositions) */
struct EmDSBPositionNotify {
	DWORD dwOffset;
	HANDLE hEventNotify;
};

/* XMEDIAPACKET */
struct EmXMediaPacket {
	void *pvBuffer;
	DWORD dwMaxSize;
	DWORD *pdwCompletedSize;
	DWORD *pdwStatus;
	HANDLE hCompletionEvent;
	long long *prtTimestamp;
};

/* XMediaObject (the WMA decoder) */
struct EmXMediaObject;
struct EmXMediaObjectVtbl {
	ULONG (*AddRef)(struct EmXMediaObject *self);
	ULONG (*Release)(struct EmXMediaObject *self);
	HRESULT (*GetInfo)(struct EmXMediaObject *self, void *info);
	HRESULT (*GetStatus)(struct EmXMediaObject *self, DWORD *status);
	HRESULT (*Process)(struct EmXMediaObject *self, const struct EmXMediaPacket *in,
		const struct EmXMediaPacket *out);
	HRESULT (*Discontinuity)(struct EmXMediaObject *self);
	HRESULT (*Flush)(struct EmXMediaObject *self);
};
struct EmXMediaObject {
	const struct EmXMediaObjectVtbl *lpVtbl;
};

/* XSOUNDTRACK_DATA (0x4c bytes) */
struct EmSoundtrackData {
	UINT uSoundtrackId;
	UINT uSongCount;
	UINT uSoundtrackLength;
	WCHAR szName[32];
};

/* WMA decoder callback: (context, byte offset, bytes wanted, &data); returns bytes */
typedef int (*EmWmaDataCallback)(void *context, DWORD offset, DWORD size, void **data);

/* DirectSound (DSOUND section, 3925): entry points the platform layer does not have
yet (docs/LIBRARY_CALLS.md: MISSING) */
HRESULT WINAPI IDirectSound_SetAllParameters(LPDIRECTSOUND sound, const struct EmDS3DListener *listener,
	DWORD apply);
HRESULT WINAPI IDirectSoundBuffer_SetFrequency(LPDIRECTSOUNDBUFFER buffer, DWORD frequency);
HRESULT WINAPI IDirectSoundBuffer_SetPosition(LPDIRECTSOUNDBUFFER buffer, FLOAT x, FLOAT y, FLOAT z,
	DWORD apply);
HRESULT WINAPI IDirectSoundBuffer_SetVelocity(LPDIRECTSOUNDBUFFER buffer, FLOAT x, FLOAT y, FLOAT z,
	DWORD apply);
HRESULT WINAPI IDirectSoundBuffer_SetMinDistance(LPDIRECTSOUNDBUFFER buffer, FLOAT distance, DWORD apply);
HRESULT WINAPI IDirectSoundBuffer_SetMaxDistance(LPDIRECTSOUNDBUFFER buffer, FLOAT distance, DWORD apply);
HRESULT WINAPI IDirectSoundBuffer_GetStatus(LPDIRECTSOUNDBUFFER buffer, LPDWORD status);
HRESULT WINAPI IDirectSoundBuffer_GetCurrentPosition(LPDIRECTSOUNDBUFFER buffer, LPDWORD play_cursor,
	LPDWORD write_cursor);
HRESULT WINAPI IDirectSoundBuffer_SetNotificationPositions(LPDIRECTSOUNDBUFFER buffer, DWORD count,
	const struct EmDSBPositionNotify *notifies);

/* WMADECXM section 0x002324a0: creates a WMA decoder media object that pulls the
compressed file through the callback; reports the decoded format */
HRESULT WINAPI WmaCreateDecoder(EmWmaDataCallback callback, void *context, DWORD lookahead,
	WAVEFORMATEX *format, struct EmXMediaObject **object);

/* XAPI soundtracks (dashboard music; module xkernel) and files */
HANDLE WINAPI XFindFirstSoundtrack(struct EmSoundtrackData *data);
BOOL WINAPI XFindNextSoundtrack(HANDLE find, struct EmSoundtrackData *data);
BOOL WINAPI XGetSoundtrackSongInfo(DWORD soundtrack_id, UINT index, DWORD *song_id, DWORD *song_length,
	WCHAR *name, UINT name_count);
HANDLE WINAPI XOpenSoundtrackSong(DWORD song_id, BOOL asynchronous);
/* XFindClose is declared by xdk_pdb.h */

/* kernel (module xkernel) */
VOID NTAPI DbgBreakPoint(VOID);

/* ---------- samples (sample channels) */

/* a sample resource (".wav" in a package), fixed up by bsSampleFixup: header 0x68 bytes */
struct BSample {
	int owned;                /* 0x00 >= 0: allocated (freed by bsDeleteSample) */
	short group;              /* 0x04 */
	short pad06;
	unsigned char pad08[0x24 - 0x08];
	unsigned int flags;       /* 0x24 default play flags */
	char channels;            /* 0x28 copy of the format's channel count (1: may be 3D) */
	unsigned char pad29[0x30 - 0x29];
	DSBUFFERDESC desc;        /* 0x30 */
	WAVEFORMATEX format;      /* 0x48 */
	unsigned char pad5a[0x5c - 0x5a];
	void *data;               /* 0x5c */
	int field_60;             /* 0x60 */
	unsigned int dataSize;    /* 0x64 */
};
OF_ASSERT_OFFSET(struct BSample, desc, 0x30);
OF_ASSERT_OFFSET(struct BSample, format, 0x48);
OF_ASSERT_OFFSET(struct BSample, data, 0x5c);
OF_ASSERT_SIZE(struct BSample, 0x68);

/* a moving sound's position callback: (position to fill, argument of bsUpdate, context) */
typedef void (*BSoundPosCallback)(float *pos, int arg, void *context);

/* a sample channel: 0x54 bytes, 64 of them at 0x002da308 (60 used) */
struct BSoundChannel {
	float pos[4];             /* 0x00 */
	float prevPos[4];         /* 0x10 */
	int status;               /* 0x20 0 free, 1 playing, 2 paused */
	struct BSample *sample;   /* 0x24 */
	float *posPtr;            /* 0x28 followed position when there is no callback */
	BSoundPosCallback callback; /* 0x2c */
	void *context;            /* 0x30 */
	unsigned int flags;       /* 0x34 BSCHAN_* */
	unsigned short volume;    /* 0x38 current (attenuated) volume 0-255 */
	unsigned short baseVolume; /* 0x3a */
	int pan;                  /* 0x3c */
	float maxDist;            /* 0x40 */
	float minDist;            /* 0x44 */
	unsigned int playFlags;   /* 0x48 */
	int field_4c;             /* 0x4c */
	LPDIRECTSOUNDBUFFER buffer; /* 0x50 */
};
OF_ASSERT_SIZE(struct BSoundChannel, 0x54);

#define BSCHAN_RESERVED 1
#define BSCHAN_MOVING 2
#define BSCHAN_HW3D 4

#define BSOUND_CHANNELS_ALLOC 64

extern struct BSoundChannel DAT_002da308[BSOUND_CHANNELS_ALLOC];
extern int DAT_00285654;              /* number of channels used (60) */
extern int DAT_00285650;              /* sound effect volume 0-255 */
extern const LONG DAT_00285658[256];  /* volume 0-255 to hundredths of a dB */
extern float DAT_00285a58;            /* distance factor */
extern int DAT_00285a8c;              /* music (stream) volume 0-255 */
extern LPDIRECTSOUND DAT_002db808;    /* the DirectSound object */
extern int DAT_002db80c;              /* 1: DirectSound up (3D possible) */

/* ---------- audio streams */

/* an audio stream (ADPCM file or WMA): 0xb0 bytes */
struct BAudioStream {
	int owned;                /* 0x00 >= 0: allocated */
	unsigned char pad04[0x24 - 0x04];
	int frequency;            /* 0x24 */
	int bits;                 /* 0x28 */
	unsigned int formatFlags; /* 0x2c bit 1: stereo */
	unsigned char pad30[0x34 - 0x30];
	void *package;            /* 0x34 package that holds the file (NULL: own file) */
	int fileIndex;            /* 0x38 */
	void *pcm;                /* 0x3c play buffer */
	LPDIRECTSOUNDBUFFER buffer; /* 0x40 */
	int thread;               /* 0x44 play-cursor thread (ADPCM) */
	int numBlocks;            /* 0x48 */
	int block;                /* 0x4c next block to load */
	int half;                 /* 0x50 half of the play buffer to fill (ADPCM) */
	void *encoded;            /* 0x54 encoded data (WMA, 2 x 64 KB) */
	struct EmXMediaObject *decoder; /* 0x58 WMA decoder */
	int samplesLoaded;        /* 0x5c (ADPCM) */
	int kind;                 /* 0x60 0 ADPCM, 1 WMA */
	int event;                /* 0x64 background load event */
	HANDLE file;              /* 0x68 */
	int slot;                 /* 0x6c 1 + stream slot, 4 = stopped */
	int field_70;             /* 0x70 */
	int state;                /* 0x74 0 stopped, 1 playing, 2 paused, 3 starting */
	int volume;               /* 0x78 */
	int pan;                  /* 0x7c */
	int playFrequency;        /* 0x80 */
	unsigned int flags;       /* 0x84 BSTREAM_* */
	void (*callback)(struct BAudioStream *stream, int looped); /* 0x88 */
	int field_8c;             /* 0x8c */
	void *data90;             /* 0x90 (fixed up) */
	const char *eventName;    /* 0x94 */
	int slotIndex;            /* 0x98 */
	int notifyHalf;           /* 0x9c (ADPCM) */
	int lastBuffer;           /* 0xa0 (WMA) */
	int decodeWritePos;       /* 0xa4 (WMA) */
	int totalSamples;         /* 0xa8 (ADPCM) */
	int endCount;             /* 0xac (WMA) */
};
OF_ASSERT_OFFSET(struct BAudioStream, kind, 0x60);
OF_ASSERT_OFFSET(struct BAudioStream, flags, 0x84);
OF_ASSERT_SIZE(struct BAudioStream, 0xb0);

#define BSTREAM_LOOP 0x1
#define BSTREAM_STEREO 0x2
#define BSTREAM_KEEPBUFFERS 0x8
#define BSTREAM_PAUSED 0x10
#define BSTREAM_STARTPAUSED 0x20
#define BSTREAM_LOADPENDING 0x1000000

extern struct BAudioStream *DAT_002db810[3]; /* [0] the active play list (xbPlayList),
	[1], [2] the two stream slots */
extern int DAT_002db81c;                     /* streams playing */
extern int DAT_002db824;                     /* 1: stream debug output */
extern const char *const DAT_00285a90[2];    /* the slots' background-load events */
extern char DAT_002d0758[0x100];             /* the data path (module bkernel) */

/* functions missing from the function table (docs/ARCHITECTURE.md 11) */
int WINAPI FUN_00056780(void *context, DWORD offset, DWORD size, void **data);
void FUN_000574c0(const char *eventName, int unused, struct BAudioStream *stream);
int FUN_00057570(struct BAudioStream *stream);

/* port debugging, not in the XBE (BSOUND/unplaced.c): with OPENFRENZY_DSOUND_LOG, one line a
sample or stream start: its file name, DirectSound buffer, channel, volume */
void bsLogStart(const char *what, const void *resource, LPDIRECTSOUNDBUFFER buffer, int channel, int volume,
	int frequency, unsigned int flags);

/* ---------- play lists (soundtracks) */

/* a play list: 0x74 bytes */
struct BPlayList {
	int soundtrack;           /* 0x00 index */
	unsigned int soundtrackId; /* 0x04 */
	unsigned int flags;       /* 0x08 1 loop, 2 ?, 4 paused */
	int state;                /* 0x0c 0 idle, 1 playing, 2 paused */
	int song;                 /* 0x10 */
	int volume;               /* 0x14 */
	int pan;                  /* 0x18 */
	int frequency;            /* 0x1c */
	HANDLE file;              /* 0x20 */
	struct BAudioStream *stream; /* 0x24 */
	unsigned int numSongs;    /* 0x28 */
	unsigned int *songIds;    /* 0x2c */
	WCHAR name[32];           /* 0x30 */
	WCHAR (*songNames)[32];   /* 0x70 */
};
OF_ASSERT_SIZE(struct BPlayList, 0x74);

#endif
