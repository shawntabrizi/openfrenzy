/*
WMA_XMO.C

WmaCreateDecoder: the XDK's WMA decoder media object (XBE sections WMADEC and WMADECXM,
entry 0x002324a0), on the decoder in wma_decoder.c. dsound module. Not from OpenCE (Halo
had no WMA).

How the game uses it (engine_misc, BSOUND/xbAudioStreamWMA.cpp):
- 0x00057060 (BlockLoadedCallback, once the first 64 KB of the .wma is loaded) creates the
  decoder with its read callback 0x00056780 and sets the music buffer's frequency from the
  format it returns;
- 0x00056d20 calls Process (vtable +0x10) with no input and an output packet into its
  256 KB PCM ring, up to the play cursor; fewer bytes than asked mean the end of the
  stream, and it calls Discontinuity (+0x14), then fills silence and restarts the track.

The XBE's object (vtable 0x0027d190: AddRef 0x00232670, Release 0x002326b0, GetInfo
0x00233220, GetStatus 0x00233260, Process 0x00232c10, Discontinuity 0x00233250, Flush
0x002329e0) decodes synchronously when created without a completion event, which is how
the game creates it; so does this one. The values GetInfo and GetStatus give are the XBE's.
*/

#include "platform.h"
#include "dsound_xdk.h"
#include "wma_decoder.h"

#include <stdlib.h>
#include <string.h>

struct wma_xmo
{
	XMediaObject object;      /* first: the interface pointer is the object's */
	ULONG reference_count;
	LPFNWMAXMODATACALLBACK callback;
	LPVOID context;
	struct wma_file *file;
	struct wma_file_format format;
	/* OPENFRENZY_DSOUND_LOG (dsound_sdl.c): decoded since the last line, and its peak */
	uint32_t log_frames;
	int log_peak;
};

int dsound_log_enabled(void);
int dsound_pcm16_peak(const void *data, unsigned long bytes);

/* dsound_sdl.c refers to this so that the linker loads this file with it: the game
image may hold a weak stand-in of WmaCreateDecoder (engine_misc's
BSOUND/platform_fallback.c), and an archive member that defines only a symbol already
defined, if weakly, is never loaded */
const char dsound_wma_xmo_linked = 1;

static struct wma_xmo *xmo_from_interface(XMediaObject *object)
{
	return (struct wma_xmo *)object;
}

/* the most the XBE's decoder asks of the callback at once (0x00234133, 0x00233d2d:
reads of up to 0x80 bytes). The game's callback (0x00056780) depends on it: a read
across the end of its 128 KB ring is joined in a 0x100-byte buffer (0x002db830),
which a larger read overflows. file_read_exact asks again for the rest. */
#define XMO_MAXIMUM_READ 0x80

/* wma_read_proc on the game's callback, which returns the byte count (an XDK
callback may return S_OK: all of them) */
static int xmo_read(void *context, uint32_t offset, uint32_t size, const uint8_t **data)
{
	struct wma_xmo *xmo = context;
	LPVOID pointer = NULL;
	HRESULT result;

	if (size > XMO_MAXIMUM_READ)
		size = XMO_MAXIMUM_READ;
	result = xmo->callback(xmo->context, offset, size, &pointer);

	if (result < 0 || !pointer)
		return -1;
	*data = pointer;
	if (result > 0 && (uint32_t)result < size)
		return (int)result;
	return (int)size;
}

static ULONG STDMETHODCALLTYPE xmo_add_reference(XMediaObject *object)
{
	return ++xmo_from_interface(object)->reference_count;
}

static ULONG STDMETHODCALLTYPE xmo_release(XMediaObject *object)
{
	struct wma_xmo *xmo = xmo_from_interface(object);

	if (--xmo->reference_count)
		return xmo->reference_count;
	wma_file_close(xmo->file);
	free(xmo);
	return 0;
}

/* 0x00233220: flags 0xd, output size channels << 12 */
static HRESULT STDMETHODCALLTYPE xmo_get_info(XMediaObject *object, LPXMEDIAINFO information)
{
	struct wma_xmo *xmo = xmo_from_interface(object);

	if (!information)
		return E_INVALIDARG;
	memset(information, 0, sizeof(*information));
	information->dwFlags = 0xd;
	information->dwOutputSize = (unsigned long)xmo->format.channels << 12;
	return S_OK;
}

/* 0x00233260: 2 (accepts output packets) for a synchronous decoder */
static HRESULT STDMETHODCALLTYPE xmo_get_status(XMediaObject *object, LPDWORD status)
{
	(void)object;
	if (status)
		*status = 2;
	return S_OK;
}

/* decodes into the output packet (the game passes no input) */
static HRESULT STDMETHODCALLTYPE xmo_process(XMediaObject *object, LPCXMEDIAPACKET input, LPCXMEDIAPACKET output)
{
	struct wma_xmo *xmo = xmo_from_interface(object);
	LPCXMEDIAPACKET packet = output ? output : input;
	uint32_t frame_bytes, frames, decoded;

	if (!packet || !packet->pvBuffer)
		return E_INVALIDARG;
	frame_bytes = 2u * (uint32_t)xmo->format.channels;
	frames = packet->dwMaxSize / frame_bytes;
	decoded = frames ? wma_file_read(xmo->file, packet->pvBuffer, frames) : 0;
	if (dsound_log_enabled())
	{
		/* once a second of decoded PCM: its peak (0 to 32768) */
		int peak = dsound_pcm16_peak(packet->pvBuffer, decoded * frame_bytes);

		if (peak > xmo->log_peak)
			xmo->log_peak = peak;
		xmo->log_frames += decoded;
		if (xmo->log_frames >= (uint32_t)xmo->format.sample_rate || decoded < frames)
		{
			platform_log("dsound: WMA %p decoded %u frames (%u Hz, %u channels), peak %d%s", (void *)xmo,
				xmo->log_frames, (unsigned)xmo->format.sample_rate, (unsigned)xmo->format.channels,
				xmo->log_peak, decoded < frames ? ", end of stream" : "");
			xmo->log_frames = 0;
			xmo->log_peak = 0;
		}
	}
	if (packet->pdwCompletedSize)
		*packet->pdwCompletedSize = decoded * frame_bytes;
	if (packet->pdwStatus)
		*packet->pdwStatus = XMEDIAPACKET_STATUS_SUCCESS;
	if (packet->hCompletionEvent)
		SetEvent(packet->hCompletionEvent);
	return S_OK;
}

/* 0x00233250 */
static HRESULT STDMETHODCALLTYPE xmo_discontinuity(XMediaObject *object)
{
	(void)object;
	return S_OK;
}

/* 0x002329e0 goes back to the start of the file */
static HRESULT STDMETHODCALLTYPE xmo_flush(XMediaObject *object)
{
	wma_file_rewind(xmo_from_interface(object)->file);
	return S_OK;
}

static const XMediaObjectVtbl xmo_vtable =
{
	xmo_add_reference,
	xmo_release,
	xmo_get_info,
	xmo_get_status,
	xmo_process,
	xmo_discontinuity,
	xmo_flush,
};

/* 0x002324a0. E_FAIL (0x80004005) without a callback, as the XBE's, and
E_OUTOFMEMORY when it cannot allocate; also E_FAIL when the stream is not WMA
(the XBE's fails in its Flush). The game only tests the sign. */
HRESULT WINAPI WmaCreateDecoder(LPFNWMAXMODATACALLBACK pCallback, LPVOID pCallbackContext, DWORD dwYieldRate,
	WAVEFORMATEX *pwfxOutput, LPXMEDIAOBJECT *ppMediaObject)
{
	struct wma_xmo *xmo;

	(void)dwYieldRate;
	if (ppMediaObject)
		*ppMediaObject = NULL;
	if (!pCallback || !ppMediaObject)
		return E_FAIL;
	xmo = calloc(1, sizeof(*xmo));
	if (!xmo)
		return E_OUTOFMEMORY;
	xmo->object.lpVtbl = &xmo_vtable;
	xmo->reference_count = 1;
	xmo->callback = pCallback;
	xmo->context = pCallbackContext;
	xmo->file = wma_file_open(xmo_read, xmo, &xmo->format);
	if (!xmo->file)
	{
		platform_log("dsound: WmaCreateDecoder: not a WMA v1/v2 ASF stream");
		free(xmo);
		return E_FAIL;
	}
	/* FUN_00233300: 16-bit PCM of the stream's channels and rate */
	if (pwfxOutput)
	{
		pwfxOutput->wFormatTag = WAVE_FORMAT_PCM;
		pwfxOutput->nChannels = (WORD)xmo->format.channels;
		pwfxOutput->nSamplesPerSec = (DWORD)xmo->format.sample_rate;
		pwfxOutput->wBitsPerSample = 16;
		pwfxOutput->nBlockAlign = (WORD)(2 * xmo->format.channels);
		pwfxOutput->nAvgBytesPerSec = pwfxOutput->nBlockAlign * pwfxOutput->nSamplesPerSec;
		pwfxOutput->cbSize = 0;
	}
	*ppMediaObject = &xmo->object;
	return S_OK;
}
