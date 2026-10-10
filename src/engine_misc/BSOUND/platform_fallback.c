/*
BSOUND/platform_fallback.c: weak stand-ins for XDK functions that engine_misc calls but
the platform layer does not implement yet (docs/LIBRARY_CALLS.md marks the DirectSound
ones MISSING; the soundtrack and WMA ones are not in the committed platform layer).
A real (strong) definition in src/dsound or src/xkernel replaces each at link time.
Each stand-in does the least that keeps the game's logic sound: nothing plays, so a
buffer reports "not playing", there are no soundtracks, and the WMA decoder produces
silence. This is not decompiled game code (no original address).
*/

#include "engine_misc_sound.h"
#include "engine_misc_extern.h"

#define EM_WEAK __attribute__((weak))

EM_WEAK HRESULT WINAPI IDirectSound_SetAllParameters(LPDIRECTSOUND sound,
	const struct EmDS3DListener *listener, DWORD apply)
{
	(void)sound;
	(void)listener;
	(void)apply;
	return DS_OK;
}

EM_WEAK HRESULT WINAPI IDirectSoundBuffer_SetFrequency(LPDIRECTSOUNDBUFFER buffer, DWORD frequency)
{
	(void)buffer;
	(void)frequency;
	return DS_OK;
}

EM_WEAK HRESULT WINAPI IDirectSoundBuffer_SetPosition(LPDIRECTSOUNDBUFFER buffer, FLOAT x, FLOAT y,
	FLOAT z, DWORD apply)
{
	(void)buffer;
	(void)x;
	(void)y;
	(void)z;
	(void)apply;
	return DS_OK;
}

EM_WEAK HRESULT WINAPI IDirectSoundBuffer_SetVelocity(LPDIRECTSOUNDBUFFER buffer, FLOAT x, FLOAT y,
	FLOAT z, DWORD apply)
{
	(void)buffer;
	(void)x;
	(void)y;
	(void)z;
	(void)apply;
	return DS_OK;
}

EM_WEAK HRESULT WINAPI IDirectSoundBuffer_SetMinDistance(LPDIRECTSOUNDBUFFER buffer, FLOAT distance,
	DWORD apply)
{
	(void)buffer;
	(void)distance;
	(void)apply;
	return DS_OK;
}

EM_WEAK HRESULT WINAPI IDirectSoundBuffer_SetMaxDistance(LPDIRECTSOUNDBUFFER buffer, FLOAT distance,
	DWORD apply)
{
	(void)buffer;
	(void)distance;
	(void)apply;
	return DS_OK;
}

/* not playing */
EM_WEAK HRESULT WINAPI IDirectSoundBuffer_GetStatus(LPDIRECTSOUNDBUFFER buffer, LPDWORD status)
{
	(void)buffer;
	if (status != NULL)
		*status = 0;
	return DS_OK;
}

EM_WEAK HRESULT WINAPI IDirectSoundBuffer_GetCurrentPosition(LPDIRECTSOUNDBUFFER buffer,
	LPDWORD play_cursor, LPDWORD write_cursor)
{
	(void)buffer;
	if (play_cursor != NULL)
		*play_cursor = 0;
	if (write_cursor != NULL)
		*write_cursor = 0;
	return DS_OK;
}

EM_WEAK HRESULT WINAPI IDirectSoundBuffer_SetNotificationPositions(LPDIRECTSOUNDBUFFER buffer, DWORD count,
	const struct EmDSBPositionNotify *notifies)
{
	(void)buffer;
	(void)count;
	(void)notifies;
	return DS_OK;
}

/* ---------- a WMA decoder that decodes nothing */

static ULONG silent_add_ref(struct EmXMediaObject *self)
{
	(void)self;
	return 1;
}

static ULONG silent_release(struct EmXMediaObject *self)
{
	(void)self;
	return 0;
}

static HRESULT silent_get_info(struct EmXMediaObject *self, void *info)
{
	(void)self;
	(void)info;
	return DS_OK;
}

static HRESULT silent_get_status(struct EmXMediaObject *self, DWORD *status)
{
	(void)self;
	if (status != NULL)
		*status = 0;
	return DS_OK;
}

/* produces no bytes: the stream fills its buffer with silence and ends */
static HRESULT silent_process(struct EmXMediaObject *self, const struct EmXMediaPacket *in,
	const struct EmXMediaPacket *out)
{
	(void)self;
	(void)in;
	if (out != NULL) {
		if (out->pdwCompletedSize != NULL)
			*out->pdwCompletedSize = 0;
		if (out->pdwStatus != NULL)
			*out->pdwStatus = 0;
	}
	return DS_OK;
}

static HRESULT silent_discontinuity(struct EmXMediaObject *self)
{
	(void)self;
	return DS_OK;
}

static const struct EmXMediaObjectVtbl s_silent_vtbl = {
	silent_add_ref, silent_release, silent_get_info, silent_get_status, silent_process,
	silent_discontinuity, silent_discontinuity,
};
static struct EmXMediaObject s_silent_decoder = { &s_silent_vtbl };

EM_WEAK HRESULT WINAPI WmaCreateDecoder(EmWmaDataCallback callback, void *context, DWORD lookahead,
	WAVEFORMATEX *format, struct EmXMediaObject **object)
{
	(void)callback;
	(void)context;
	(void)lookahead;
	if (format != NULL) {
		memset(format, 0, sizeof(*format));
		format->wFormatTag = WAVE_FORMAT_PCM;
		format->nChannels = 2;
		format->nSamplesPerSec = 44100;
		format->wBitsPerSample = 16;
		format->nBlockAlign = 4;
		format->nAvgBytesPerSec = 44100 * 4;
	}
	*object = &s_silent_decoder;
	return DS_OK;
}

/* ---------- soundtracks: there are none */

EM_WEAK HANDLE WINAPI XFindFirstSoundtrack(struct EmSoundtrackData *data)
{
	(void)data;
	return INVALID_HANDLE_VALUE;
}

EM_WEAK BOOL WINAPI XFindNextSoundtrack(HANDLE find, struct EmSoundtrackData *data)
{
	(void)find;
	(void)data;
	return FALSE;
}

EM_WEAK BOOL WINAPI XGetSoundtrackSongInfo(DWORD soundtrack_id, UINT index, DWORD *song_id,
	DWORD *song_length, WCHAR *name, UINT name_count)
{
	(void)soundtrack_id;
	(void)index;
	(void)song_id;
	(void)song_length;
	(void)name;
	(void)name_count;
	return FALSE;
}

EM_WEAK HANDLE WINAPI XOpenSoundtrackSong(DWORD song_id, BOOL asynchronous)
{
	(void)song_id;
	(void)asynchronous;
	return INVALID_HANDLE_VALUE;
}

EM_WEAK BOOL WINAPI XFindClose(HANDLE find)
{
	(void)find;
	return TRUE;
}
