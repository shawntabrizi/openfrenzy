/*
WMA_DECODER.H

Windows Media Audio v1/v2 decoder and a sequential ASF reader (wma_decoder.c), used by the
WMA media object (wma_xmo.c) that replaces the XBE's WMADEC/WMADECXM sections. The game's
music (Packages/Music.xbp, 29 streams) is WMA v2 (0x161), stereo, 44.1 kHz, 128 kbps.

This file depends only on the C library, so it also builds natively for tests
(src/dsound/tests/test_wma.c). The codec is a port of FFmpeg's (LGPL 2.1 or later).
*/

#ifndef OPENFRENZY_WMA_DECODER_H
#define OPENFRENZY_WMA_DECODER_H

#include <stdint.h>

#define WMA_FORMAT_TAG_V1 0x0160
#define WMA_FORMAT_TAG_V2 0x0161

/* the most samples (per channel) one superframe gives: 16 frames of 2048 */
#define WMA_MAXIMUM_SUPERFRAME_SAMPLES (16 * 2048)

/* ---------- the codec: one superframe (block_align bytes) at a time */

struct wma_codec;

/* extra: the bytes after WAVEFORMATEX (cbSize of them). NULL if the format is not
supported. */
struct wma_codec *wma_codec_create(int format_tag, int channels, int sample_rate,
	int bytes_per_second, int block_align, const uint8_t *extra, int extra_size);
void wma_codec_destroy(struct wma_codec *codec);
/* forget the bit reservoir (after a seek or a gap) */
void wma_codec_flush(struct wma_codec *codec);
/* back to the state after create (the start of a stream) */
void wma_codec_reset(struct wma_codec *codec);
/* decodes one superframe of `size` (>= block_align) bytes into interleaved 16-bit
samples; output holds WMA_MAXIMUM_SUPERFRAME_SAMPLES frames. Returns the frames
written (0 when the superframe only fills the bit reservoir), or -1 on a bitstream
error (the reservoir is reset; decoding can go on with the next superframe). */
int wma_codec_decode(struct wma_codec *codec, const uint8_t *data, int size, int16_t *output);

/* ---------- the ASF file reader */

/* Gives `size` bytes of the file at `offset` in *data. Returns the bytes it gives
(at most size), or a negative number on failure. The data must stay valid until
the next call. The reader reads the header once, then the data packets in order,
each once, and never past the data object. */
typedef int (*wma_read_proc)(void *context, uint32_t offset, uint32_t size, const uint8_t **data);

struct wma_file_format
{
	int format_tag;
	int channels;
	int sample_rate;
	int bytes_per_second;
	int block_align;
	uint32_t packet_size;
	uint32_t packet_count;
	uint32_t first_packet_offset;
	/* play duration in 100 ns units and the preroll in ms, from the file properties */
	uint64_t play_duration;
	uint64_t preroll;
};

struct wma_file;

/* reads and checks the header. NULL if it is not an ASF file with one WMA v1/v2
audio stream. */
struct wma_file *wma_file_open(wma_read_proc read, void *context, struct wma_file_format *format);
void wma_file_close(struct wma_file *file);
/* decodes up to `frames` frames of interleaved 16-bit samples. Returns the frames
written; fewer than asked only at the end of the file (0 after it). */
uint32_t wma_file_read(struct wma_file *file, int16_t *output, uint32_t frames);
/* TRUE (1) once every packet was read and decoded and the samples given out */
int wma_file_finished(const struct wma_file *file);
/* superframes that failed to decode and packets that could not be parsed */
uint32_t wma_file_errors(const struct wma_file *file);
/* superframes that only continued a frame (all their bits went to the reservoir), and
the output frame where the first came; FFmpeg decodes these wrongly (wma_decoder.c) */
uint32_t wma_file_continuations(const struct wma_file *file, uint64_t *first_frame);
/* restart at the first packet (the header is not read again) */
void wma_file_rewind(struct wma_file *file);

#endif
