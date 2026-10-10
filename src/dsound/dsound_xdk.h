/*
DSOUND_XDK.H

XDK DirectSound, WMA and Bink entry points that the game calls and that the shared XDK
headers (src/runtime/include/xdk/, from OpenCE: Halo used other ones) do not declare.
Game and engine code includes this after "xtl.h". dsound module (src/dsound/).

Original XBE addresses of the entry points (DSOUND 3925 section, XbSymbolDatabase names)
are in the comments; docs/LIBRARY_CALLS.md lists their callers. Prototypes follow the XDK
(the DirectX 8 / Xbox SDK documentation); where the game's call shows the argument use, it
is noted.
*/

#ifndef OPENFRENZY_DSOUND_XDK_H
#define OPENFRENZY_DSOUND_XDK_H

#include "xtl.h"

/* ---------- DirectSound buffers */

/* DSBUFFERDESC.dwFlags. 0x10 is the bit the game sets for 3D samples (0x000557d0
ORs it in before IDirectSound_CreateSoundBuffer); the stream buffers use 0x1a0
(0x000569b0, 0x000573e0): position notify 0x100, volume 0x80, frequency 0x20. */
#define DSBCAPS_CTRL3D 0x00000010
#define DSBCAPS_CTRLFREQUENCY 0x00000020
#define DSBCAPS_CTRLVOLUME 0x00000080

/* IDirectSoundBuffer_GetStatus: the game tests bit 0 to free finished
one-shots (0x00055720) */
#define DSBSTATUS_PLAYING 0x00000001
#define DSBSTATUS_LOOPING 0x00000004

/* IDirectSoundBuffer_Lock flags */
#define DSBLOCK_FROMWRITECURSOR 0x00000001
#define DSBLOCK_ENTIREBUFFER 0x00000002

typedef struct _DSVECTOR3
{
	FLOAT x, y, z;
} DSVECTOR3;

/* IDirectSound_SetAllParameters: the game fills dwSize 0x40, a position, zero
velocity, front and top vectors, distance factor, rolloff 1.0, Doppler 8.0
(0x000559c0) */
typedef struct _DS3DLISTENER
{
	DWORD dwSize;
	DSVECTOR3 vPosition;
	DSVECTOR3 vVelocity;
	DSVECTOR3 vOrientFront;
	DSVECTOR3 vOrientTop;
	FLOAT flDistanceFactor;
	FLOAT flRolloffFactor;
	FLOAT flDopplerFactor;
} DS3DLISTENER, *LPDS3DLISTENER;
typedef const DS3DLISTENER *LPCDS3DLISTENER;

HRESULT WINAPI IDirectSound_SetAllParameters(LPDIRECTSOUND sound, LPCDS3DLISTENER listener, DWORD apply); /* 0x00207296 */

HRESULT WINAPI IDirectSoundBuffer_SetFrequency(LPDIRECTSOUNDBUFFER buffer, DWORD frequency);              /* 0x00207953 */
HRESULT WINAPI IDirectSoundBuffer_SetMaxDistance(LPDIRECTSOUNDBUFFER buffer, FLOAT distance, DWORD apply); /* 0x0020732e */
HRESULT WINAPI IDirectSoundBuffer_SetMinDistance(LPDIRECTSOUNDBUFFER buffer, FLOAT distance, DWORD apply); /* 0x00207352 */
HRESULT WINAPI IDirectSoundBuffer_SetPosition(LPDIRECTSOUNDBUFFER buffer, FLOAT x, FLOAT y, FLOAT z,
	DWORD apply);                                                                                          /* 0x00207376 */
HRESULT WINAPI IDirectSoundBuffer_SetVelocity(LPDIRECTSOUNDBUFFER buffer, FLOAT x, FLOAT y, FLOAT z,
	DWORD apply);                                                                                          /* 0x002073ab */
HRESULT WINAPI IDirectSoundBuffer_GetStatus(LPDIRECTSOUNDBUFFER buffer, LPDWORD status);                   /* 0x002073e0 */
/* play and write cursors, bytes into the buffer data (the WMA music refills its
256 KB ring up to the cursor: 0x00056d20) */
HRESULT WINAPI IDirectSoundBuffer_GetCurrentPosition(LPDIRECTSOUNDBUFFER buffer, LPDWORD play_cursor,
	LPDWORD write_cursor);                                                                                 /* 0x002073fc */
HRESULT WINAPI IDirectSoundBuffer_Lock(LPDIRECTSOUNDBUFFER buffer, DWORD offset, DWORD bytes,
	LPVOID *audio1, LPDWORD bytes1, LPVOID *audio2, LPDWORD bytes2, DWORD flags);                          /* 0x00207438 */
HRESULT WINAPI IDirectSoundBuffer_Unlock(LPDIRECTSOUNDBUFFER buffer, LPVOID audio1, DWORD bytes1,
	LPVOID audio2, DWORD bytes2);                                                                          /* 0x00206a6b */
/* IDirectSoundBuffer_SetNotificationPositions (0x002079cb): when the play cursor
reaches dwOffset (bytes), hEventNotify is set; DSBPN_OFFSETSTOP: when it stops. The
ADPCM streams arm one at a time, half a buffer apart (0x000574c0, 0x00057570). */
#define DSBPN_OFFSETSTOP 0xFFFFFFFF
typedef struct _DSBPOSITIONNOTIFY
{
	DWORD dwOffset;
	HANDLE hEventNotify;
} DSBPOSITIONNOTIFY, *LPDSBPOSITIONNOTIFY;
typedef const DSBPOSITIONNOTIFY *LPCDSBPOSITIONNOTIFY;
HRESULT WINAPI IDirectSoundBuffer_SetNotificationPositions(LPDIRECTSOUNDBUFFER buffer, DWORD count,
	LPCDSBPOSITIONNOTIFY notifies);                                                                        /* 0x002079cb */

HRESULT WINAPI IDirectSoundBuffer_SetMode(LPDIRECTSOUNDBUFFER buffer, DWORD mode, DWORD apply);
HRESULT WINAPI IDirectSoundBuffer_SetMixBins(LPDIRECTSOUNDBUFFER buffer, DWORD mix_bin_mask);
HRESULT WINAPI IDirectSoundBuffer_SetI3DL2Source(LPDIRECTSOUNDBUFFER buffer, LPCDSI3DL2BUFFER source, DWORD apply);
ULONG WINAPI IDirectSoundBuffer_AddRef(LPDIRECTSOUNDBUFFER buffer);

/* ---------- WMA decoder media object (XBE sections WMADEC, WMADECXM) */

/* An XMediaObject (the XDK's media object interface; the decoder's vtable at
0x0027d190 has these seven methods first). The game calls Process (+0x10) to
decode into its PCM ring and Discontinuity (+0x14) when the stream ran out
(0x00056d20). */
typedef struct XMediaObject XMediaObject, *LPXMEDIAOBJECT;
typedef struct XMediaObjectVtbl
{
	ULONG (STDMETHODCALLTYPE *AddRef)(XMediaObject *object);
	ULONG (STDMETHODCALLTYPE *Release)(XMediaObject *object);
	HRESULT (STDMETHODCALLTYPE *GetInfo)(XMediaObject *object, LPXMEDIAINFO information);
	HRESULT (STDMETHODCALLTYPE *GetStatus)(XMediaObject *object, LPDWORD status);
	HRESULT (STDMETHODCALLTYPE *Process)(XMediaObject *object, LPCXMEDIAPACKET input, LPCXMEDIAPACKET output);
	HRESULT (STDMETHODCALLTYPE *Discontinuity)(XMediaObject *object);
	HRESULT (STDMETHODCALLTYPE *Flush)(XMediaObject *object);
} XMediaObjectVtbl;
struct XMediaObject
{
	const XMediaObjectVtbl *lpVtbl;
};

/* Reads dwByteCount bytes of the WMA file at dwOffset: stores a pointer to them in
*ppvData and returns the count (the game's, at 0x00056780, points into its 128 KB
ring of background-loaded 64 KB blocks, and returns dwByteCount). The decoder
reads the file once, in order. */
typedef HRESULT (CALLBACK *LPFNWMAXMODATACALLBACK)(LPVOID pContext, DWORD dwOffset, DWORD dwByteCount,
	LPVOID *ppvData);

/* 0x002324a0 (WMADECXM; the game: 0x00057060 "WMA: BlockLoadedCallback"). Reads the
ASF header through pCallback; pwfxOutput (may be NULL) gets the decoded format,
16-bit PCM (FUN_00233300). dwYieldRate (0 means 1000) is ignored. */
HRESULT WINAPI WmaCreateDecoder(LPFNWMAXMODATACALLBACK pCallback, LPVOID pCallbackContext, DWORD dwYieldRate,
	WAVEFORMATEX *pwfxOutput, LPXMEDIAOBJECT *ppMediaObject);

/* ---------- RAD Bink (XBE region LIB:bink 0x00057980-0x000703a0, replaced by
bink_host.c: the host decodes the movie with FFmpeg; without FFmpeg no movie
opens and the game skips it) */

typedef struct BINK *HBINK;
typedef void *(__stdcall *rad_memory_allocate_proc)(unsigned long size);
typedef void (__stdcall *rad_memory_free_proc)(void *memory);

/* 0x0005a090 (XbSymbolDatabase names it D3D_SetPushBufferSize by mistake: it stores
the two procedures Bink's radmalloc at 0x0005a0b0 calls) */
void __stdcall RADSetMemory(rad_memory_allocate_proc allocate, rad_memory_free_proc release);
/* 0x000591c0 ("Not a Bink file."); the game (bdVideoPlayStream 0x000463c0) returns 6
when it gives NULL */
HBINK __stdcall BinkOpen(const char *name, unsigned long flags);
/* 0x00059f80 (frees the handle and its buffers) */
void __stdcall BinkClose(HBINK bink);

#endif
