/*
WMA_DECODER.C

Windows Media Audio v1/v2 decoder and a sequential ASF reader (wma_decoder.h).

The codec is a port of FFmpeg n4.4.4 libavcodec/wma.c, wma_common.c, wma_freqs.c and
wmadec.c (the WMA v1/v2 decoder), made self-contained: FFmpeg's bit reader, VLC tables,
float DSP and MDCT are replaced by the small equivalents below, the decode logic is
FFmpeg's. The ASF reader is new; it handles what the Xbox's WMA decoder (WMADECXM,
WmaCreateDecoder) was given: one audio stream in fixed-size data packets, read in order.

Copyright (c) 2002-2007 The FFmpeg Project (the codec). This file is part of FFmpeg.

FFmpeg is free software; you can redistribute it and/or modify it under the terms of the GNU
Lesser General Public License as published by the Free Software Foundation; either version
2.1 of the License, or (at your option) any later version. FFmpeg is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
License (src/dsound/LICENSE.LGPL-2.1) for more details.

No platform headers: this file also builds natively (src/dsound/tests/test_wma.c).
*/

#include "wma_decoder.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* The platform build force-includes halo_math.h, whose macros send sin, cos, pow and
the others to src/runtime/of_math.c's halo_* wrappers. Those currently call
themselves and never return (reported to the runtime owner). Audio needs no
cross-machine bit-exactness, so this file calls musl's own functions. */
#undef sin
#undef cos
#undef tan
#undef asin
#undef acos
#undef atan
#undef atan2
#undef exp
#undef log
#undef log2
#undef log10
#undef pow

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------- constants (wma.h) */

#define BLOCK_MIN_BITS 7
#define BLOCK_MAX_BITS 11
#define BLOCK_MAX_SIZE (1 << BLOCK_MAX_BITS)
#define BLOCK_NB_SIZES (BLOCK_MAX_BITS - BLOCK_MIN_BITS + 1)
#define HIGH_BAND_MAX_SIZE 16
#define WMA_NB_LSP_COEFS 10
#define MAX_CODED_SUPERFRAME_SIZE 32768
#define MAX_CHANNELS 2
#define NOISE_TAB_SIZE 8192
#define LSP_POW_BITS 7
/* FFmpeg's AV_INPUT_BUFFER_PADDING_SIZE: zero bytes after the reservoir */
#define INPUT_PADDING 64
/* FFmpeg's MIN_CACHE_BITS (the bit reader's guaranteed refill) */
#define MIN_CACHE_BITS 25

struct wma_coef_vlc_table
{
	int n;
	int max_level;
	const uint32_t *huffcodes;
	const uint8_t *huffbits;
	const uint16_t *levels;
};

#include "wma_tables.h"

/* wma_freqs.c */
static const uint16_t wma_critical_freqs[25] =
{
	100, 200, 300, 400, 510, 630, 770, 920,
	1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150,
	3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500,
	24500,
};

/* ---------- bit reader (FFmpeg get_bits.h, big-endian; bits past the end read 0) */

struct bit_reader
{
	const uint8_t *buffer;
	int size_in_bits;
	int index;
};

static void br_init(struct bit_reader *reader, const uint8_t *buffer, int bits)
{
	if (bits < 0)
		bits = 0;
	reader->buffer = buffer;
	reader->size_in_bits = bits;
	reader->index = 0;
}

static uint32_t br_bits(struct bit_reader *reader, int count)
{
	/* the buffer's bytes; past them the reader gives zeros, as FFmpeg's packets
	and its reservoir are padded with zeros */
	int bytes = (reader->size_in_bits + 7) >> 3;
	uint32_t value = 0;

	while (count > 0)
	{
		int position = reader->index;
		int bit = position & 7;
		int take = 8 - bit;
		uint32_t byte = (position >> 3) < bytes ? reader->buffer[position >> 3] : 0;

		if (take > count)
			take = count;
		value = (value << take) | ((byte >> (8 - bit - take)) & ((1u << take) - 1));
		reader->index += take;
		count -= take;
	}
	/* FFmpeg's checked reader stops 8 bits past the end */
	if (reader->index > reader->size_in_bits + 8)
		reader->index = reader->size_in_bits + 8;
	return value;
}

static int br_bit(struct bit_reader *reader)
{
	return (int)br_bits(reader, 1);
}

static void br_skip(struct bit_reader *reader, int count)
{
	reader->index += count;
	if (reader->index > reader->size_in_bits + 8)
		reader->index = reader->size_in_bits + 8;
}

static int br_count(const struct bit_reader *reader)
{
	return reader->index;
}

static int br_left(const struct bit_reader *reader)
{
	return reader->size_in_bits - reader->index;
}

static void br_align(struct bit_reader *reader)
{
	int remainder = (-reader->index) & 7;

	if (remainder)
		br_skip(reader, remainder);
}

/* ---------- variable length codes: a binary tree walked bit by bit */

struct vlc
{
	/* next[b] > 0: an inner node; < 0: leaf -(i + 1) (symbols[i]); 0: no code */
	int32_t (*next)[2];
	int32_t *symbols;
	int node_count;
	int node_capacity;
};

static int vlc_new_node(struct vlc *vlc)
{
	if (vlc->node_count == vlc->node_capacity)
	{
		int capacity = vlc->node_capacity ? vlc->node_capacity * 2 : 64;
		int32_t (*next)[2] = realloc(vlc->next, (size_t)capacity * sizeof(*next));

		if (!next)
			return -1;
		vlc->next = next;
		vlc->node_capacity = capacity;
	}
	vlc->next[vlc->node_count][0] = 0;
	vlc->next[vlc->node_count][1] = 0;
	return vlc->node_count++;
}

static void vlc_free(struct vlc *vlc)
{
	free(vlc->next);
	free(vlc->symbols);
	memset(vlc, 0, sizeof(*vlc));
}

/* codes[i] (right aligned, lengths[i] bits; 0 = no code) gives symbols[i] */
static int vlc_build(struct vlc *vlc, int count, const uint8_t *lengths, int length_stride,
	const uint32_t *codes, const int32_t *symbols)
{
	int i;

	memset(vlc, 0, sizeof(*vlc));
	vlc->symbols = malloc((size_t)count * sizeof(*vlc->symbols));
	if (!vlc->symbols || vlc_new_node(vlc) != 0)
		goto fail;
	for (i = 0; i < count; i++)
	{
		int length = lengths[i * length_stride];
		uint32_t code = codes[i];
		int node = 0;
		int bit;

		vlc->symbols[i] = symbols ? symbols[i] : i;
		if (length <= 0)
			continue;
		if (length > 32)
			goto fail;
		for (bit = length - 1; bit > 0; bit--)
		{
			int b = (code >> bit) & 1;
			int32_t next = vlc->next[node][b];

			if (next < 0)
				goto fail;
			if (!next)
			{
				next = vlc_new_node(vlc);
				if (next < 0)
					goto fail;
				vlc->next[node][b] = next;
			}
			node = next;
		}
		if (vlc->next[node][code & 1])
			goto fail;
		vlc->next[node][code & 1] = -(i + 1);
	}
	return 0;
fail:
	vlc_free(vlc);
	return -1;
}

/* FFmpeg's ff_init_vlc_from_lengths: canonical codes in table order */
static int vlc_build_from_lengths(struct vlc *vlc, int count, const uint8_t *lengths, int length_stride,
	const uint8_t *symbols, int symbol_stride, int offset)
{
	uint32_t codes[64];
	int32_t values[64];
	uint8_t bits[64];
	uint64_t code = 0;
	int i;

	if (count > 64)
		return -1;
	for (i = 0; i < count; i++)
	{
		int length = lengths[i * length_stride];

		bits[i] = (uint8_t)length;
		values[i] = symbols[i * symbol_stride] + offset;
		codes[i] = length ? (uint32_t)(code >> (32 - length)) : 0;
		if (length)
			code += (uint64_t)1 << (32 - length);
	}
	return vlc_build(vlc, count, bits, 1, codes, values);
}

/* the symbol, or -1 for a code that is not in the table */
static int vlc_read(struct bit_reader *reader, const struct vlc *vlc)
{
	int32_t node = 0;
	int depth;

	for (depth = 0; depth < 32; depth++)
	{
		int32_t next = vlc->next[node][br_bit(reader)];

		if (next < 0)
			return vlc->symbols[-next - 1];
		if (!next)
			return -1;
		node = next;
	}
	return -1;
}

/* ---------- inverse MDCT (FFmpeg mdct_template.c: pre-rotation, a complex FFT of n/4
points, post-rotation), the FFT a plain radix-2 one */

struct mdct
{
	int bits;
	float *tcos, *tsin;   /* n/4 */
	float *twiddle;       /* n/8 complex: exp(+2 pi i k / (n/4)) */
	uint16_t *reverse;    /* n/4: bit reversal */
	float *z;             /* n/4 complex scratch */
};

static void mdct_free(struct mdct *mdct)
{
	free(mdct->tcos);
	free(mdct->tsin);
	free(mdct->twiddle);
	free(mdct->reverse);
	free(mdct->z);
	memset(mdct, 0, sizeof(*mdct));
}

static int mdct_init(struct mdct *mdct, int bits, double scale)
{
	int n = 1 << bits, n4 = n >> 2, m_bits = bits - 2;
	double theta = 1.0 / 8.0 + (scale < 0 ? n4 : 0);
	int i;

	memset(mdct, 0, sizeof(*mdct));
	mdct->bits = bits;
	mdct->tcos = malloc((size_t)n4 * sizeof(float));
	mdct->tsin = malloc((size_t)n4 * sizeof(float));
	mdct->twiddle = malloc((size_t)n4 * sizeof(float));
	mdct->reverse = malloc((size_t)n4 * sizeof(uint16_t));
	mdct->z = malloc((size_t)n4 * 2 * sizeof(float));
	if (!mdct->tcos || !mdct->tsin || !mdct->twiddle || !mdct->reverse || !mdct->z)
	{
		mdct_free(mdct);
		return -1;
	}
	scale = sqrt(fabs(scale));
	for (i = 0; i < n4; i++)
	{
		double alpha = 2 * M_PI * (i + theta) / n;

		mdct->tcos[i] = (float)(-cos(alpha) * scale);
		mdct->tsin[i] = (float)(-sin(alpha) * scale);
	}
	for (i = 0; i < n4 / 2; i++)
	{
		double angle = 2 * M_PI * i / n4;

		mdct->twiddle[2 * i] = (float)cos(angle);
		mdct->twiddle[2 * i + 1] = (float)sin(angle);
	}
	for (i = 0; i < n4; i++)
	{
		int reversed = 0, b;

		for (b = 0; b < m_bits; b++)
			if (i & (1 << b))
				reversed |= 1 << (m_bits - 1 - b);
		mdct->reverse[i] = (uint16_t)reversed;
	}
	return 0;
}

/* in place, z[k] = sum_j z[j] exp(+2 pi i j k / m) (FFmpeg's inverse FFT), input in
bit reversed order */
static void fft_inverse_permuted(float *z, const float *twiddle, int m)
{
	int size;

	for (size = 2; size <= m; size <<= 1)
	{
		int half = size >> 1, step = m / size, start;

		for (start = 0; start < m; start += size)
		{
			int k;

			for (k = 0; k < half; k++)
			{
				float wr = twiddle[2 * k * step], wi = twiddle[2 * k * step + 1];
				float *a = z + 2 * (start + k), *b = z + 2 * (start + k + half);
				float tr = b[0] * wr - b[1] * wi;
				float ti = b[0] * wi + b[1] * wr;

				b[0] = a[0] - tr;
				b[1] = a[1] - ti;
				a[0] += tr;
				a[1] += ti;
			}
		}
	}
}

/* FFmpeg's ff_imdct_calc: n/2 coefficients in, n samples out */
static void mdct_inverse(struct mdct *mdct, float *output, const float *input)
{
	int n = 1 << mdct->bits, n2 = n >> 1, n4 = n >> 2, n8 = n >> 3;
	const float *tcos = mdct->tcos, *tsin = mdct->tsin;
	float *z = mdct->z;
	float *half = output + n4;
	int k;

	/* pre rotation */
	for (k = 0; k < n4; k++)
	{
		float are = input[n2 - 1 - 2 * k], aim = input[2 * k];
		int j = mdct->reverse[k];

		z[2 * j] = are * tcos[k] - aim * tsin[k];
		z[2 * j + 1] = are * tsin[k] + aim * tcos[k];
	}
	fft_inverse_permuted(z, mdct->twiddle, n4);
	/* post rotation and reordering */
	for (k = 0; k < n8; k++)
	{
		int a = n8 - k - 1, b = n8 + k;
		float r0, i0, r1, i1;

		r0 = z[2 * a + 1] * tsin[a] - z[2 * a] * tcos[a];
		i1 = z[2 * a + 1] * tcos[a] + z[2 * a] * tsin[a];
		r1 = z[2 * b + 1] * tsin[b] - z[2 * b] * tcos[b];
		i0 = z[2 * b + 1] * tcos[b] + z[2 * b] * tsin[b];
		z[2 * a] = r0;
		z[2 * a + 1] = i0;
		z[2 * b] = r1;
		z[2 * b + 1] = i1;
	}
	memcpy(half, z, (size_t)n2 * sizeof(float));
	for (k = 0; k < n4; k++)
	{
		output[k] = -output[n2 - k - 1];
		output[n - k - 1] = output[n2 + k];
	}
}

/* ---------- the codec state (FFmpeg's WMACodecContext) */

struct wma_codec
{
	int channels;
	int sample_rate;
	int block_align;
	struct bit_reader gb;
	int version;
	int use_bit_reservoir;
	int use_variable_block_len;
	int use_exp_vlc;
	int use_noise_coding;
	int byte_offset_bits;
	struct vlc exp_vlc;
	int exponent_sizes[BLOCK_NB_SIZES];
	uint16_t exponent_bands[BLOCK_NB_SIZES][25];
	int high_band_start[BLOCK_NB_SIZES];
	int coefs_start;
	int coefs_end[BLOCK_NB_SIZES];
	int exponent_high_sizes[BLOCK_NB_SIZES];
	int exponent_high_bands[BLOCK_NB_SIZES][HIGH_BAND_MAX_SIZE];
	struct vlc hgain_vlc;
	int high_band_coded[MAX_CHANNELS][HIGH_BAND_MAX_SIZE];
	int high_band_values[MAX_CHANNELS][HIGH_BAND_MAX_SIZE];
	struct vlc coef_vlc[2];
	uint16_t *run_table[2];
	float *level_table[2];
	uint16_t *int_table[2];
	const struct wma_coef_vlc_table *coef_vlcs[2];
	int frame_len;
	int frame_len_bits;
	int nb_block_sizes;
	int reset_block_lengths;
	int block_len_bits;
	int next_block_len_bits;
	int prev_block_len_bits;
	int block_len;
	int block_num;
	int block_pos;
	uint8_t ms_stereo;
	uint8_t channel_coded[MAX_CHANNELS];
	int exponents_bsize[MAX_CHANNELS];
	float exponents[MAX_CHANNELS][BLOCK_MAX_SIZE];
	float max_exponent[MAX_CHANNELS];
	float coefs1[MAX_CHANNELS][BLOCK_MAX_SIZE];
	float coefs[MAX_CHANNELS][BLOCK_MAX_SIZE];
	float output[BLOCK_MAX_SIZE * 2];
	struct mdct mdct_ctx[BLOCK_NB_SIZES];
	float *windows[BLOCK_NB_SIZES];
	float frame_out[MAX_CHANNELS][BLOCK_MAX_SIZE * 2];
	uint8_t last_superframe[MAX_CODED_SUPERFRAME_SIZE + INPUT_PADDING];
	int last_bitoffset;
	/* bits in the reservoir (FFmpeg's last_superframe_len counts bytes) */
	int reservoir_bits;
	int exponents_initialized[MAX_CHANNELS];
	float noise_table[NOISE_TAB_SIZE];
	int noise_index;
	float noise_mult;
	float lsp_cos_table[BLOCK_MAX_SIZE];
	float lsp_pow_e_table[256];
	float lsp_pow_m_table1[1 << LSP_POW_BITS];
	float lsp_pow_m_table2[1 << LSP_POW_BITS];
	/* one superframe's samples, planar (FFmpeg's AVFrame) */
	float samples[MAX_CHANNELS][WMA_MAXIMUM_SUPERFRAME_SAMPLES];
};

static int av_log2(unsigned value)
{
	int result = 0;

	while (value > 1)
	{
		value >>= 1;
		result++;
	}
	return result;
}

/* wma_common.c */
static int wma_get_frame_len_bits(int sample_rate, int version)
{
	if (sample_rate <= 16000)
		return 9;
	if (sample_rate <= 22050 || (sample_rate <= 32000 && version == 1))
		return 10;
	return 11;
}

/* wma.c init_coef_vlc */
static int init_coef_vlc(struct vlc *vlc, uint16_t **prun_table, float **plevel_table,
	uint16_t **pint_table, const struct wma_coef_vlc_table *vlc_table)
{
	int n = vlc_table->n;
	const uint16_t *levels_table = vlc_table->levels;
	uint16_t *run_table, *int_table;
	float *flevel_table;
	int i, l, j, k, level;

	if (vlc_build(vlc, n, vlc_table->huffbits, 1, vlc_table->huffcodes, NULL) < 0)
		return -1;
	run_table = calloc((size_t)n, sizeof(uint16_t));
	flevel_table = calloc((size_t)n, sizeof(float));
	int_table = calloc((size_t)n, sizeof(uint16_t));
	if (!run_table || !flevel_table || !int_table)
	{
		free(run_table);
		free(flevel_table);
		free(int_table);
		return -1;
	}
	i = 2;
	level = 1;
	k = 0;
	while (i < n)
	{
		int_table[k] = (uint16_t)i;
		l = levels_table[k++];
		for (j = 0; j < l; j++)
		{
			run_table[i] = (uint16_t)j;
			flevel_table[i] = (float)level;
			i++;
		}
		level++;
	}
	*prun_table = run_table;
	*plevel_table = flevel_table;
	*pint_table = int_table;
	return 0;
}

/* wma.c ff_wma_init */
static int wma_init(struct wma_codec *s, int bit_rate, int flags2)
{
	int i;
	float bps1, high_freq;
	volatile float bps;
	int sample_rate1;
	int coef_vlc_table;
	int channels = s->channels, sample_rate = s->sample_rate;

	if (sample_rate <= 0 || sample_rate > 50000 || channels <= 0 || channels > 2 || bit_rate <= 0)
		return -1;

	s->frame_len_bits = wma_get_frame_len_bits(sample_rate, s->version);
	s->next_block_len_bits = s->frame_len_bits;
	s->prev_block_len_bits = s->frame_len_bits;
	s->block_len_bits = s->frame_len_bits;

	s->frame_len = 1 << s->frame_len_bits;
	if (s->use_variable_block_len)
	{
		int nb_max, nb;

		nb = ((flags2 >> 3) & 3) + 1;
		if ((bit_rate / channels) >= 32000)
			nb += 2;
		nb_max = s->frame_len_bits - BLOCK_MIN_BITS;
		if (nb > nb_max)
			nb = nb_max;
		s->nb_block_sizes = nb + 1;
	}
	else
		s->nb_block_sizes = 1;

	s->use_noise_coding = 1;
	high_freq = sample_rate * 0.5;

	sample_rate1 = sample_rate;
	if (s->version == 2)
	{
		if (sample_rate1 >= 44100)
			sample_rate1 = 44100;
		else if (sample_rate1 >= 22050)
			sample_rate1 = 22050;
		else if (sample_rate1 >= 16000)
			sample_rate1 = 16000;
		else if (sample_rate1 >= 11025)
			sample_rate1 = 11025;
		else if (sample_rate1 >= 8000)
			sample_rate1 = 8000;
	}

	bps = (float)bit_rate / (float)(channels * sample_rate);
	s->byte_offset_bits = av_log2((unsigned)(int)(bps * s->frame_len / 8.0 + 0.5)) + 2;
	if (s->byte_offset_bits + 3 > MIN_CACHE_BITS)
		return -1;

	bps1 = bps;
	if (channels == 2)
		bps1 = bps * 1.6;
	if (sample_rate1 == 44100)
	{
		if (bps1 >= 0.61)
			s->use_noise_coding = 0;
		else
			high_freq = high_freq * 0.4;
	}
	else if (sample_rate1 == 22050)
	{
		if (bps1 >= 1.16)
			s->use_noise_coding = 0;
		else if (bps1 >= 0.72)
			high_freq = high_freq * 0.7;
		else
			high_freq = high_freq * 0.6;
	}
	else if (sample_rate1 == 16000)
	{
		if (bps > 0.5)
			high_freq = high_freq * 0.5;
		else
			high_freq = high_freq * 0.3;
	}
	else if (sample_rate1 == 11025)
		high_freq = high_freq * 0.7;
	else if (sample_rate1 == 8000)
	{
		if (bps <= 0.625)
			high_freq = high_freq * 0.5;
		else if (bps > 0.75)
			s->use_noise_coding = 0;
		else
			high_freq = high_freq * 0.65;
	}
	else
	{
		if (bps >= 0.8)
			high_freq = high_freq * 0.75;
		else if (bps >= 0.6)
			high_freq = high_freq * 0.6;
		else
			high_freq = high_freq * 0.5;
	}

	/* the scale factor band sizes for each MDCT block size */
	{
		int a, b, pos, lpos, k, block_len, j, n;
		const uint8_t *table;

		if (s->version == 1)
			s->coefs_start = 3;
		else
			s->coefs_start = 0;
		for (k = 0; k < s->nb_block_sizes; k++)
		{
			block_len = s->frame_len >> k;

			if (s->version == 1)
			{
				lpos = 0;
				for (i = 0; i < 25; i++)
				{
					a = wma_critical_freqs[i];
					b = sample_rate;
					pos = ((block_len * 2 * a) + (b >> 1)) / b;
					if (pos > block_len)
						pos = block_len;
					s->exponent_bands[0][i] = (uint16_t)(pos - lpos);
					if (pos >= block_len)
					{
						i++;
						break;
					}
					lpos = pos;
				}
				s->exponent_sizes[0] = i;
			}
			else
			{
				table = NULL;
				a = s->frame_len_bits - BLOCK_MIN_BITS - k;
				if (a < 3)
				{
					if (sample_rate >= 44100)
						table = exponent_band_44100[a];
					else if (sample_rate >= 32000)
						table = exponent_band_32000[a];
					else if (sample_rate >= 22050)
						table = exponent_band_22050[a];
				}
				if (table)
				{
					n = *table++;
					for (i = 0; i < n; i++)
						s->exponent_bands[k][i] = table[i];
					s->exponent_sizes[k] = n;
				}
				else
				{
					j = 0;
					lpos = 0;
					for (i = 0; i < 25; i++)
					{
						a = wma_critical_freqs[i];
						b = sample_rate;
						pos = ((block_len * 2 * a) + (b << 1)) / (4 * b);
						pos <<= 2;
						if (pos > block_len)
							pos = block_len;
						if (pos > lpos)
							s->exponent_bands[k][j++] = (uint16_t)(pos - lpos);
						if (pos >= block_len)
							break;
						lpos = pos;
					}
					s->exponent_sizes[k] = j;
				}
			}

			s->coefs_end[k] = (s->frame_len - ((s->frame_len * 9) / 100)) >> k;
			s->high_band_start[k] = (int)((block_len * 2 * high_freq) / sample_rate + 0.5);
			n = s->exponent_sizes[k];
			j = 0;
			pos = 0;
			for (i = 0; i < n; i++)
			{
				int start, end;

				start = pos;
				pos += s->exponent_bands[k][i];
				end = pos;
				if (start < s->high_band_start[k])
					start = s->high_band_start[k];
				if (end > s->coefs_end[k])
					end = s->coefs_end[k];
				if (end > start && j < HIGH_BAND_MAX_SIZE)
					s->exponent_high_bands[k][j++] = end - start;
			}
			s->exponent_high_sizes[k] = j;
		}
	}

	/* MDCT windows: sine windows (sinewin_tablegen.h) */
	for (i = 0; i < s->nb_block_sizes; i++)
	{
		int n = 1 << (s->frame_len_bits - i), j;

		s->windows[i] = malloc((size_t)n * sizeof(float));
		if (!s->windows[i])
			return -1;
		for (j = 0; j < n; j++)
			s->windows[i][j] = sinf((j + 0.5) * (M_PI / (2.0 * n)));
	}

	s->reset_block_lengths = 1;

	if (s->use_noise_coding)
	{
		unsigned int seed;
		float norm;

		if (s->use_exp_vlc)
			s->noise_mult = 0.02;
		else
			s->noise_mult = 0.04;
		seed = 1;
		norm = (1.0 / (float)(1LL << 31)) * sqrt(3) * s->noise_mult;
		for (i = 0; i < NOISE_TAB_SIZE; i++)
		{
			seed = seed * 314159 + 1;
			s->noise_table[i] = (float)((int)seed) * norm;
		}
	}

	coef_vlc_table = 2;
	if (sample_rate >= 32000)
	{
		if (bps1 < 0.72)
			coef_vlc_table = 0;
		else if (bps1 < 1.16)
			coef_vlc_table = 1;
	}
	s->coef_vlcs[0] = &wma_coef_vlcs[coef_vlc_table * 2];
	s->coef_vlcs[1] = &wma_coef_vlcs[coef_vlc_table * 2 + 1];
	if (init_coef_vlc(&s->coef_vlc[0], &s->run_table[0], &s->level_table[0], &s->int_table[0],
			s->coef_vlcs[0]) < 0)
		return -1;
	return init_coef_vlc(&s->coef_vlc[1], &s->run_table[1], &s->level_table[1], &s->int_table[1],
		s->coef_vlcs[1]);
}

static int wma_total_gain_to_bits(int total_gain)
{
	if (total_gain < 15)
		return 13;
	else if (total_gain < 32)
		return 12;
	else if (total_gain < 40)
		return 11;
	else if (total_gain < 45)
		return 10;
	else
		return 9;
}

static unsigned int wma_get_large_val(struct bit_reader *gb)
{
	int n_bits = 8;

	if (br_bit(gb))
	{
		n_bits += 8;
		if (br_bit(gb))
		{
			n_bits += 8;
			if (br_bit(gb))
				n_bits += 7;
		}
	}
	return br_bits(gb, n_bits);
}

/* wma.c ff_wma_run_level_decode */
static int wma_run_level_decode(struct bit_reader *gb, const struct vlc *vlc, const float *level_table,
	const uint16_t *run_table, int version, float *ptr, int offset, int num_coefs, int block_len,
	int frame_len_bits, int coef_nb_bits)
{
	int code, level, sign;
	const uint32_t *ilvl = (const uint32_t *)level_table;
	uint32_t *iptr = (uint32_t *)ptr;
	const unsigned int coef_mask = block_len - 1;

	for (; offset < num_coefs; offset++)
	{
		code = vlc_read(gb, vlc);
		if (code > 1)
		{
			offset += run_table[code];
			sign = br_bit(gb) - 1;
			iptr[offset & coef_mask] = ilvl[code] ^ ((uint32_t)sign & 0x80000000u);
		}
		else if (code == 1)
			break;
		else
		{
			if (!version)
			{
				level = (int)br_bits(gb, coef_nb_bits);
				offset += (int)br_bits(gb, frame_len_bits);
			}
			else
			{
				level = (int)wma_get_large_val(gb);
				if (br_bit(gb))
				{
					if (br_bit(gb))
					{
						if (br_bit(gb))
							return -1;
						else
							offset += (int)br_bits(gb, frame_len_bits) + 4;
					}
					else
						offset += (int)br_bits(gb, 2) + 1;
				}
			}
			sign = br_bit(gb) - 1;
			ptr[offset & coef_mask] = (float)((level ^ sign) - sign);
		}
	}
	/* a run past the coded coefficients ("overflow in spectral RLE") */
	if (offset > num_coefs)
		return -1;
	return 0;
}

/* ---------- wmadec.c */

static inline float pow_m1_4(struct wma_codec *s, float x)
{
	union { float f; unsigned int v; } u, t;
	unsigned int e, m;
	float a, b;

	u.f = x;
	e = u.v >> 23;
	m = (u.v >> (23 - LSP_POW_BITS)) & ((1 << LSP_POW_BITS) - 1);
	t.v = ((u.v << LSP_POW_BITS) & ((1 << 23) - 1)) | (127 << 23);
	a = s->lsp_pow_m_table1[m];
	b = s->lsp_pow_m_table2[m];
	return s->lsp_pow_e_table[e] * (a + b * t.f);
}

static void wma_lsp_to_curve_init(struct wma_codec *s, int frame_len)
{
	float wdel, a, b;
	int i, e, m;

	wdel = M_PI / frame_len;
	for (i = 0; i < frame_len; i++)
		s->lsp_cos_table[i] = 2.0f * cos(wdel * i);
	for (i = 0; i < 256; i++)
	{
		e = i - 126;
		s->lsp_pow_e_table[i] = exp2f(e * -0.25);
	}
	b = 1.0;
	for (i = (1 << LSP_POW_BITS) - 1; i >= 0; i--)
	{
		m = (1 << LSP_POW_BITS) + i;
		a = (float)m * (0.5 / (1 << LSP_POW_BITS));
		a = 1 / sqrt(sqrt(a));
		s->lsp_pow_m_table1[i] = 2 * a - b;
		s->lsp_pow_m_table2[i] = b - a;
		b = a;
	}
}

static void wma_lsp_to_curve(struct wma_codec *s, float *out, float *val_max_ptr, int n, float *lsp)
{
	int i, j;
	float p, q, w, v, val_max;

	val_max = 0;
	for (i = 0; i < n; i++)
	{
		p = 0.5f;
		q = 0.5f;
		w = s->lsp_cos_table[i];
		for (j = 1; j < WMA_NB_LSP_COEFS; j += 2)
		{
			q *= w - lsp[j - 1];
			p *= w - lsp[j];
		}
		p *= p * (2.0f - w);
		q *= q * (2.0f + w);
		v = p + q;
		v = pow_m1_4(s, v);
		if (v > val_max)
			val_max = v;
		out[i] = v;
	}
	*val_max_ptr = val_max;
}

static void decode_exp_lsp(struct wma_codec *s, int ch)
{
	float lsp_coefs[WMA_NB_LSP_COEFS];
	int val, i;

	for (i = 0; i < WMA_NB_LSP_COEFS; i++)
	{
		if (i == 0 || i >= 8)
			val = (int)br_bits(&s->gb, 3);
		else
			val = (int)br_bits(&s->gb, 4);
		lsp_coefs[i] = wma_lsp_codebook[i][val];
	}
	wma_lsp_to_curve(s, s->exponents[ch], &s->max_exponent[ch], s->block_len, lsp_coefs);
}

/* pow(10, i / 16.0) for i in -60..95 */
static float pow_tab[156];

static void pow_tab_init(void)
{
	int i;

	if (pow_tab[60] == 1.0f)
		return;
	for (i = 0; i < 156; i++)
		pow_tab[i] = (float)pow(10.0, (i - 60) / 16.0);
}

static int decode_exp_vlc(struct wma_codec *s, int ch)
{
	int last_exp, n, code;
	const uint16_t *ptr;
	float v, max_scale;
	float *q, *q_end;
	const float *ptab = pow_tab + 60;

	ptr = s->exponent_bands[s->frame_len_bits - s->block_len_bits];
	q = s->exponents[ch];
	q_end = q + s->block_len;
	max_scale = 0;
	if (s->version == 1)
	{
		last_exp = (int)br_bits(&s->gb, 5) + 10;
		v = ptab[last_exp];
		max_scale = v;
		n = *ptr++;
		/* FFmpeg's unrolled loop writes n values, 4 when n is 0 */
		if (n <= 0)
			n = 4;
		while (n-- > 0)
			*q++ = v;
	}
	else
		last_exp = 36;

	while (q < q_end)
	{
		code = vlc_read(&s->gb, &s->exp_vlc);
		last_exp += code - 60;
		if ((unsigned)last_exp + 60 >= sizeof(pow_tab) / sizeof(pow_tab[0]))
			return -1;
		v = ptab[last_exp];
		if (v > max_scale)
			max_scale = v;
		n = *ptr++;
		if (n <= 0)
			n = 4;
		/* never past the block (FFmpeg's bands end exactly at it) */
		if (n > q_end - q)
			n = (int)(q_end - q);
		while (n-- > 0)
			*q++ = v;
	}
	s->max_exponent[ch] = max_scale;
	return 0;
}

static void vector_fmul_add(float *dst, const float *src0, const float *src1, const float *src2, int len)
{
	int i;

	for (i = 0; i < len; i++)
		dst[i] = src0[i] * src1[i] + src2[i];
}

static void vector_fmul_reverse(float *dst, const float *src0, const float *src1, int len)
{
	int i;

	src1 += len - 1;
	for (i = 0; i < len; i++)
		dst[i] = src0[i] * src1[-i];
}

static void wma_window(struct wma_codec *s, float *out)
{
	float *in = s->output;
	int block_len, bsize, n;

	if (s->block_len_bits <= s->prev_block_len_bits)
	{
		block_len = s->block_len;
		bsize = s->frame_len_bits - s->block_len_bits;
		vector_fmul_add(out, in, s->windows[bsize], out, block_len);
	}
	else
	{
		block_len = 1 << s->prev_block_len_bits;
		n = (s->block_len - block_len) / 2;
		bsize = s->frame_len_bits - s->prev_block_len_bits;
		vector_fmul_add(out + n, in + n, s->windows[bsize], out + n, block_len);
		memcpy(out + n + block_len, in + n + block_len, n * sizeof(float));
	}

	out += s->block_len;
	in += s->block_len;

	if (s->block_len_bits <= s->next_block_len_bits)
	{
		block_len = s->block_len;
		bsize = s->frame_len_bits - s->block_len_bits;
		vector_fmul_reverse(out, in, s->windows[bsize], block_len);
	}
	else
	{
		block_len = 1 << s->next_block_len_bits;
		n = (s->block_len - block_len) / 2;
		bsize = s->frame_len_bits - s->next_block_len_bits;
		memcpy(out, in, n * sizeof(float));
		vector_fmul_reverse(out + n, in + n, s->windows[bsize], block_len);
		memset(out + n + block_len, 0, n * sizeof(float));
	}
}

/* 0 OK, 1 the last block of the frame, -1 an error */
static int wma_decode_block(struct wma_codec *s)
{
	int n, v, a, ch, bsize;
	int coef_nb_bits, total_gain;
	int nb_coefs[MAX_CHANNELS];
	float mdct_norm;
	struct mdct *mdct;

	if (s->use_variable_block_len)
	{
		n = av_log2(s->nb_block_sizes - 1) + 1;

		if (s->reset_block_lengths)
		{
			s->reset_block_lengths = 0;
			v = (int)br_bits(&s->gb, n);
			if (v >= s->nb_block_sizes)
				return -1;
			s->prev_block_len_bits = s->frame_len_bits - v;
			v = (int)br_bits(&s->gb, n);
			if (v >= s->nb_block_sizes)
				return -1;
			s->block_len_bits = s->frame_len_bits - v;
		}
		else
		{
			s->prev_block_len_bits = s->block_len_bits;
			s->block_len_bits = s->next_block_len_bits;
		}
		v = (int)br_bits(&s->gb, n);
		if (v >= s->nb_block_sizes)
			return -1;
		s->next_block_len_bits = s->frame_len_bits - v;
	}
	else
	{
		s->next_block_len_bits = s->frame_len_bits;
		s->prev_block_len_bits = s->frame_len_bits;
		s->block_len_bits = s->frame_len_bits;
	}

	if (s->frame_len_bits - s->block_len_bits >= s->nb_block_sizes)
		return -1;

	s->block_len = 1 << s->block_len_bits;
	if ((s->block_pos + s->block_len) > s->frame_len)
		return -1;

	if (s->channels == 2)
		s->ms_stereo = (uint8_t)br_bit(&s->gb);
	v = 0;
	for (ch = 0; ch < s->channels; ch++)
	{
		a = br_bit(&s->gb);
		s->channel_coded[ch] = (uint8_t)a;
		v |= a;
	}

	bsize = s->frame_len_bits - s->block_len_bits;

	if (!v)
		goto next;

	total_gain = 1;
	for (;;)
	{
		if (br_left(&s->gb) < 7)
			return -1;
		a = (int)br_bits(&s->gb, 7);
		total_gain += a;
		if (a != 127)
			break;
	}

	coef_nb_bits = wma_total_gain_to_bits(total_gain);

	n = s->coefs_end[bsize] - s->coefs_start;
	for (ch = 0; ch < s->channels; ch++)
		nb_coefs[ch] = n;

	if (s->use_noise_coding)
	{
		for (ch = 0; ch < s->channels; ch++)
		{
			if (s->channel_coded[ch])
			{
				int i, n1;

				n1 = s->exponent_high_sizes[bsize];
				for (i = 0; i < n1; i++)
				{
					a = br_bit(&s->gb);
					s->high_band_coded[ch][i] = a;
					if (a)
						nb_coefs[ch] -= s->exponent_high_bands[bsize][i];
				}
			}
		}
		for (ch = 0; ch < s->channels; ch++)
		{
			if (s->channel_coded[ch])
			{
				int i, n1, val;

				n1 = s->exponent_high_sizes[bsize];
				val = (int)0x80000000;
				for (i = 0; i < n1; i++)
				{
					if (s->high_band_coded[ch][i])
					{
						if (val == (int)0x80000000)
							val = (int)br_bits(&s->gb, 7) - 19;
						else
							val += vlc_read(&s->gb, &s->hgain_vlc);
						s->high_band_values[ch][i] = val;
					}
				}
			}
		}
	}

	if ((s->block_len_bits == s->frame_len_bits) || br_bit(&s->gb))
	{
		for (ch = 0; ch < s->channels; ch++)
		{
			if (s->channel_coded[ch])
			{
				if (s->use_exp_vlc)
				{
					if (decode_exp_vlc(s, ch) < 0)
						return -1;
				}
				else
					decode_exp_lsp(s, ch);
				s->exponents_bsize[ch] = bsize;
				s->exponents_initialized[ch] = 1;
			}
		}
	}

	for (ch = 0; ch < s->channels; ch++)
	{
		if (s->channel_coded[ch] && !s->exponents_initialized[ch])
			return -1;
	}

	for (ch = 0; ch < s->channels; ch++)
	{
		if (s->channel_coded[ch])
		{
			int tindex;
			float *ptr = &s->coefs1[ch][0];

			tindex = (ch == 1 && s->ms_stereo);
			memset(ptr, 0, s->block_len * sizeof(float));
			if (wma_run_level_decode(&s->gb, &s->coef_vlc[tindex], s->level_table[tindex],
					s->run_table[tindex], 0, ptr, 0, nb_coefs[ch], s->block_len, s->frame_len_bits,
					coef_nb_bits) < 0)
				return -1;
		}
		if (s->version == 1 && s->channels >= 2)
			br_align(&s->gb);
	}

	{
		int n4 = s->block_len / 2;

		mdct_norm = 1.0 / (float)n4;
		if (s->version == 1)
			mdct_norm *= sqrt(n4);
	}

	for (ch = 0; ch < s->channels; ch++)
	{
		if (s->channel_coded[ch])
		{
			float *coefs1, *coefs, *exponents, mult, mult1, noise;
			int i, j, n1, last_high_band, esize;
			float exp_power[HIGH_BAND_MAX_SIZE];

			coefs1 = s->coefs1[ch];
			exponents = s->exponents[ch];
			esize = s->exponents_bsize[ch];
			mult = pow(10, total_gain * 0.05) / s->max_exponent[ch];
			mult *= mdct_norm;
			coefs = s->coefs[ch];
			if (s->use_noise_coding)
			{
				mult1 = mult;
				for (i = 0; i < s->coefs_start; i++)
				{
					*coefs++ = s->noise_table[s->noise_index] * exponents[i << bsize >> esize] * mult1;
					s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
				}

				n1 = s->exponent_high_sizes[bsize];

				exponents = s->exponents[ch] + (s->high_band_start[bsize] << bsize >> esize);
				last_high_band = 0;
				for (j = 0; j < n1; j++)
				{
					n = s->exponent_high_bands[s->frame_len_bits - s->block_len_bits][j];
					if (s->high_band_coded[ch][j])
					{
						float e2, v1;

						e2 = 0;
						for (i = 0; i < n; i++)
						{
							v1 = exponents[i << bsize >> esize];
							e2 += v1 * v1;
						}
						exp_power[j] = e2 / n;
						last_high_band = j;
					}
					exponents += n << bsize >> esize;
				}

				exponents = s->exponents[ch] + (s->coefs_start << bsize >> esize);
				for (j = -1; j < n1; j++)
				{
					if (j < 0)
						n = s->high_band_start[bsize] - s->coefs_start;
					else
						n = s->exponent_high_bands[s->frame_len_bits - s->block_len_bits][j];
					if (j >= 0 && s->high_band_coded[ch][j])
					{
						mult1 = sqrt(exp_power[j] / exp_power[last_high_band]);
						mult1 = mult1 * pow(10, s->high_band_values[ch][j] * 0.05);
						mult1 = mult1 / (s->max_exponent[ch] * s->noise_mult);
						mult1 *= mdct_norm;
						for (i = 0; i < n; i++)
						{
							noise = s->noise_table[s->noise_index];
							s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
							*coefs++ = noise * exponents[i << bsize >> esize] * mult1;
						}
						exponents += n << bsize >> esize;
					}
					else
					{
						for (i = 0; i < n; i++)
						{
							noise = s->noise_table[s->noise_index];
							s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
							*coefs++ = ((*coefs1++) + noise) * exponents[i << bsize >> esize] * mult;
						}
						exponents += n << bsize >> esize;
					}
				}

				n = s->block_len - s->coefs_end[bsize];
				mult1 = mult * exponents[(-(1 << bsize)) >> esize];
				for (i = 0; i < n; i++)
				{
					*coefs++ = s->noise_table[s->noise_index] * mult1;
					s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
				}
			}
			else
			{
				for (i = 0; i < s->coefs_start; i++)
					*coefs++ = 0.0;
				n = nb_coefs[ch];
				for (i = 0; i < n; i++)
					*coefs++ = coefs1[i] * exponents[i << bsize >> esize] * mult;
				n = s->block_len - s->coefs_end[bsize];
				for (i = 0; i < n; i++)
					*coefs++ = 0.0;
			}
		}
	}

	if (s->ms_stereo && s->channel_coded[1])
	{
		int i;

		if (!s->channel_coded[0])
		{
			memset(s->coefs[0], 0, sizeof(float) * s->block_len);
			s->channel_coded[0] = 1;
		}
		/* FFmpeg's butterflies_float */
		for (i = 0; i < s->block_len; i++)
		{
			float t = s->coefs[0][i] - s->coefs[1][i];

			s->coefs[0][i] += s->coefs[1][i];
			s->coefs[1][i] = t;
		}
	}

next:
	mdct = &s->mdct_ctx[bsize];

	for (ch = 0; ch < s->channels; ch++)
	{
		int n4, index;

		n4 = s->block_len / 2;
		if (s->channel_coded[ch])
			mdct_inverse(mdct, s->output, s->coefs[ch]);
		else if (!(s->ms_stereo && ch == 1))
			memset(s->output, 0, sizeof(s->output));

		index = (s->frame_len / 2) + s->block_pos - n4;
		wma_window(s, &s->frame_out[ch][index]);
	}

	s->block_num++;
	s->block_pos += s->block_len;
	if (s->block_pos >= s->frame_len)
		return 1;
	else
		return 0;
}

static int wma_decode_frame(struct wma_codec *s, int samples_offset)
{
	int ret, ch;

	s->block_num = 0;
	s->block_pos = 0;
	for (;;)
	{
		ret = wma_decode_block(s);
		if (ret < 0)
			return -1;
		if (ret)
			break;
	}

	for (ch = 0; ch < s->channels; ch++)
	{
		memcpy(s->samples[ch] + samples_offset, s->frame_out[ch], s->frame_len * sizeof(float));
		memmove(&s->frame_out[ch][0], &s->frame_out[ch][s->frame_len], s->frame_len * sizeof(float));
	}
	return 0;
}

/* appends `bits` bits from reader to the reservoir (FFmpeg appends whole bytes; a
bit count keeps a superframe that only continues a frame exact, see below) */
static int reservoir_append(struct wma_codec *s, struct bit_reader *reader, int bits)
{
	if (bits < 0 || s->reservoir_bits + bits > MAX_CODED_SUPERFRAME_SIZE * 8)
		return -1;
	while (bits > 0)
	{
		int take = bits > 24 ? 24 : bits;
		uint32_t value = br_bits(reader, take);
		int i;

		for (i = take - 1; i >= 0; i--)
		{
			int position = s->reservoir_bits++;
			uint8_t mask = (uint8_t)(0x80 >> (position & 7));

			if (value & (1u << i))
				s->last_superframe[position >> 3] |= mask;
			else
				s->last_superframe[position >> 3] &= (uint8_t)~mask;
		}
		bits -= take;
	}
	memset(s->last_superframe + ((s->reservoir_bits + 7) >> 3), 0, INPUT_PADDING);
	return 0;
}

/* wma_decode_superframe: the samples per channel in s->samples, or -1 */
static int wma_decode_superframe(struct wma_codec *s, const uint8_t *buf, int buf_size)
{
	int nb_frames, bit_offset, i, pos, len;
	int samples_offset;

	if (buf_size == 0)
	{
		s->reservoir_bits = 0;
		return 0;
	}
	if (buf_size < s->block_align)
		return -1;
	if (s->block_align)
		buf_size = s->block_align;

	br_init(&s->gb, buf, buf_size * 8);

	if (s->use_bit_reservoir)
	{
		br_skip(&s->gb, 4);
		nb_frames = (int)br_bits(&s->gb, 4) - (s->reservoir_bits <= 0);
		if (nb_frames <= 0)
		{
			/* no frame ends in this superframe: all of it continues the frame in
			the reservoir */
			int is_error = nb_frames < 0 || br_left(&s->gb) <= 8;

			if (is_error)
				return -1;
			/* Like every superframe it has the bit offset field after the 8-bit
			header (here it does not count). FFmpeg n4.4 (and later) appends from
			bit 8, and adds 8 * buf_size - 8 to the reservoir's byte count; either
			breaks the frame, and the decoder loses step until the next superframe
			("overflow in spectral RLE" and other errors). 10 of the game's 29
			tracks have such superframes; with the field skipped every superframe of
			the 32 music streams decodes (tests/test_wma.c). */
			br_skip(&s->gb, s->byte_offset_bits + 3);
			if (reservoir_append(s, &s->gb, br_left(&s->gb)) < 0)
				goto fail;
			return 0;
		}
	}
	else
		nb_frames = 1;

	if (nb_frames * s->frame_len > WMA_MAXIMUM_SUPERFRAME_SAMPLES)
		goto fail;
	samples_offset = 0;

	if (s->use_bit_reservoir)
	{
		bit_offset = (int)br_bits(&s->gb, s->byte_offset_bits + 3);
		if (bit_offset > br_left(&s->gb))
			goto fail;

		if (s->reservoir_bits > 0)
		{
			/* add bit_offset bits to the frame in the reservoir and decode it */
			if (reservoir_append(s, &s->gb, bit_offset) < 0)
				goto fail;
			br_init(&s->gb, s->last_superframe, s->reservoir_bits);
			if (s->last_bitoffset > 0)
				br_skip(&s->gb, s->last_bitoffset);
			if (wma_decode_frame(s, samples_offset) < 0)
				goto fail;
			samples_offset += s->frame_len;
			nb_frames--;
		}

		/* the frames that start in this superframe */
		pos = bit_offset + 4 + 4 + s->byte_offset_bits + 3;
		if (pos >= MAX_CODED_SUPERFRAME_SIZE * 8 || pos > buf_size * 8)
			return -1;
		br_init(&s->gb, buf + (pos >> 3), (buf_size - (pos >> 3)) * 8);
		len = pos & 7;
		if (len > 0)
			br_skip(&s->gb, len);

		s->reset_block_lengths = 1;
		for (i = 0; i < nb_frames; i++)
		{
			if (wma_decode_frame(s, samples_offset) < 0)
				goto fail;
			samples_offset += s->frame_len;
		}

		/* the rest starts the next frame: keep it in the reservoir */
		pos = br_count(&s->gb) + ((bit_offset + 4 + 4 + s->byte_offset_bits + 3) & ~7);
		s->last_bitoffset = pos & 7;
		pos >>= 3;
		len = buf_size - pos;
		if (len > MAX_CODED_SUPERFRAME_SIZE || len < 0)
			goto fail;
		memcpy(s->last_superframe, buf + pos, len);
		memset(s->last_superframe + len, 0, INPUT_PADDING);
		s->reservoir_bits = len * 8;
	}
	else
	{
		if (wma_decode_frame(s, samples_offset) < 0)
			goto fail;
		samples_offset += s->frame_len;
	}
	return samples_offset;

fail:
	s->reservoir_bits = 0;
	return -1;
}

/* ---------- codec interface */

static uint16_t read_le16(const uint8_t *p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t read_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t read_le64(const uint8_t *p)
{
	return (uint64_t)read_le32(p) | ((uint64_t)read_le32(p + 4) << 32);
}

void wma_codec_destroy(struct wma_codec *codec)
{
	int i;

	if (!codec)
		return;
	for (i = 0; i < BLOCK_NB_SIZES; i++)
	{
		mdct_free(&codec->mdct_ctx[i]);
		free(codec->windows[i]);
	}
	vlc_free(&codec->exp_vlc);
	vlc_free(&codec->hgain_vlc);
	for (i = 0; i < 2; i++)
	{
		vlc_free(&codec->coef_vlc[i]);
		free(codec->run_table[i]);
		free(codec->level_table[i]);
		free(codec->int_table[i]);
	}
	free(codec);
}

struct wma_codec *wma_codec_create(int format_tag, int channels, int sample_rate,
	int bytes_per_second, int block_align, const uint8_t *extra, int extra_size)
{
	struct wma_codec *s;
	int flags2 = 0;
	int i;

	if ((format_tag != WMA_FORMAT_TAG_V1 && format_tag != WMA_FORMAT_TAG_V2) || block_align <= 0)
		return NULL;
	pow_tab_init();
	s = calloc(1, sizeof(*s));
	if (!s)
		return NULL;
	s->version = format_tag == WMA_FORMAT_TAG_V1 ? 1 : 2;
	s->channels = channels;
	s->sample_rate = sample_rate;
	s->block_align = block_align;

	/* wma_decode_init */
	if (s->version == 1 && extra_size >= 4)
		flags2 = read_le16(extra + 2);
	else if (s->version == 2 && extra_size >= 6)
		flags2 = read_le16(extra + 4);
	s->use_exp_vlc = flags2 & 0x0001;
	s->use_bit_reservoir = flags2 & 0x0002;
	s->use_variable_block_len = flags2 & 0x0004;
	if (s->version == 2 && extra_size >= 8 && read_le16(extra + 4) == 0xd && s->use_variable_block_len)
		s->use_variable_block_len = 0;

	for (i = 0; i < MAX_CHANNELS; i++)
		s->max_exponent[i] = 1.0;

	if (wma_init(s, bytes_per_second * 8, flags2) < 0)
		goto fail;

	for (i = 0; i < s->nb_block_sizes; i++)
		if (mdct_init(&s->mdct_ctx[i], s->frame_len_bits - i + 1, 1.0 / 32768.0) < 0)
			goto fail;

	if (s->use_noise_coding)
	{
		if (vlc_build_from_lengths(&s->hgain_vlc, 37, &wma_hgain_hufftab[0][1], 2,
				&wma_hgain_hufftab[0][0], 2, -18) < 0)
			goto fail;
	}

	if (s->use_exp_vlc)
	{
		if (vlc_build(&s->exp_vlc, (int)sizeof(wma_scalefactor_bits), wma_scalefactor_bits, 1,
				wma_scalefactor_code, NULL) < 0)
			goto fail;
	}
	else
		wma_lsp_to_curve_init(s, s->frame_len);
	return s;
fail:
	wma_codec_destroy(s);
	return NULL;
}

void wma_codec_flush(struct wma_codec *codec)
{
	codec->last_bitoffset = 0;
	codec->reservoir_bits = 0;
}

void wma_codec_reset(struct wma_codec *codec)
{
	int ch;

	wma_codec_flush(codec);
	memset(codec->frame_out, 0, sizeof(codec->frame_out));
	for (ch = 0; ch < MAX_CHANNELS; ch++)
	{
		codec->exponents_initialized[ch] = 0;
		codec->max_exponent[ch] = 1.0;
	}
	codec->next_block_len_bits = codec->prev_block_len_bits = codec->block_len_bits = codec->frame_len_bits;
	codec->reset_block_lengths = 1;
	codec->noise_index = 0;
}

int wma_codec_decode(struct wma_codec *codec, const uint8_t *data, int size, int16_t *output)
{
	int frames = wma_decode_superframe(codec, data, size);
	int i, ch;

	if (frames <= 0)
		return frames;
	/* FFmpeg's float to 16-bit conversion (libswresample): round, then clip */
	for (i = 0; i < frames; i++)
		for (ch = 0; ch < codec->channels; ch++)
		{
			long value = lrintf(codec->samples[ch][i] * 32768.0f);

			if (value > 32767)
				value = 32767;
			else if (value < -32768)
				value = -32768;
			output[i * codec->channels + ch] = (int16_t)value;
		}
	return frames;
}

/* ---------- ASF reader */

/* object GUIDs as stored (little-endian fields) */
static const uint8_t guid_header[16] =
	{ 0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C };
static const uint8_t guid_data[16] =
	{ 0x36, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C };
static const uint8_t guid_file_properties[16] =
	{ 0xA1, 0xDC, 0xAB, 0x8C, 0x47, 0xA9, 0xCF, 0x11, 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 };
static const uint8_t guid_stream_properties[16] =
	{ 0x91, 0x07, 0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11, 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 };
static const uint8_t guid_audio_media[16] =
	{ 0x40, 0x9E, 0x69, 0xF8, 0x4D, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B };

#define ASF_MAXIMUM_HEADER (256 * 1024)
#define ASF_MAXIMUM_PACKET (64 * 1024)
#define ASF_MAXIMUM_OBJECT (256 * 1024)

struct wma_file
{
	wma_read_proc read;
	void *context;
	struct wma_file_format format;
	int stream_number;
	struct wma_codec *codec;
	uint8_t *packet;
	/* the media object being assembled from payloads, cut into superframes as it
	completes */
	uint8_t *object;
	uint32_t object_size, object_filled;
	int object_number;
	uint32_t next_packet;
	/* superframes of the packets read, not decoded yet */
	uint8_t *queue;
	uint32_t queue_size, queue_capacity, queue_position;
	/* decoded samples not given out yet */
	int16_t *pcm;
	uint32_t pcm_frames, pcm_position;
	int ended;
	/* superframes the codec rejected, and packets that could not be parsed */
	uint32_t errors;
	/* superframes that only continue a frame (decode to 0 samples), and the output
	frame at the first of them */
	uint32_t continuations;
	uint64_t first_continuation, frames_out;
};

/* reads `size` bytes at `offset` into buffer; 0 on success */
static int file_read_exact(struct wma_file *file, uint32_t offset, uint32_t size, uint8_t *buffer)
{
	while (size)
	{
		const uint8_t *data = NULL;
		int got = file->read(file->context, offset, size, &data);

		if (got <= 0 || !data)
			return -1;
		if ((uint32_t)got > size)
			got = (int)size;
		memcpy(buffer, data, (size_t)got);
		buffer += got;
		offset += (uint32_t)got;
		size -= (uint32_t)got;
	}
	return 0;
}

static int parse_header(struct wma_file *file, const uint8_t *header, uint32_t header_size)
{
	uint32_t object_count = read_le32(header + 24);
	uint32_t position = 30;
	int have_file_properties = 0, have_stream = 0;
	uint32_t i;

	for (i = 0; i < object_count && position + 24 <= header_size; i++)
	{
		const uint8_t *object = header + position;
		uint64_t size = read_le64(object + 16);
		const uint8_t *body = object + 24;

		if (size < 24 || size > header_size - position)
			return -1;
		if (!memcmp(object, guid_file_properties, 16) && size >= 24 + 80)
		{
			file->format.play_duration = read_le64(body + 40);
			file->format.preroll = read_le64(body + 56);
			/* minimum and maximum data packet size: equal in a valid file */
			file->format.packet_size = read_le32(body + 68);
			if (read_le32(body + 72) != file->format.packet_size)
				return -1;
			have_file_properties = 1;
		}
		else if (!memcmp(object, guid_stream_properties, 16) && size >= 24 + 54 && !have_stream)
		{
			uint32_t specific_size = read_le32(body + 40);
			uint32_t correction_size = read_le32(body + 44);
			const uint8_t *wave = body + 54;
			int extra_size;

			if (memcmp(body, guid_audio_media, 16))
				goto next;
			if (54 + (uint64_t)specific_size + correction_size > size - 24 || specific_size < 18)
				return -1;
			file->stream_number = read_le16(body + 48) & 0x7f;
			file->format.format_tag = read_le16(wave);
			file->format.channels = read_le16(wave + 2);
			file->format.sample_rate = (int)read_le32(wave + 4);
			file->format.bytes_per_second = (int)read_le32(wave + 8);
			file->format.block_align = read_le16(wave + 12);
			extra_size = read_le16(wave + 16);
			if (extra_size > (int)specific_size - 18)
				extra_size = (int)specific_size - 18;
			/* audio spread (scrambling) with a span over 1 is not supported */
			if (correction_size >= 1 && wave[specific_size] > 1)
				return -1;
			file->codec = wma_codec_create(file->format.format_tag, file->format.channels,
				file->format.sample_rate, file->format.bytes_per_second, file->format.block_align,
				wave + 18, extra_size);
			if (!file->codec)
				return -1;
			have_stream = 1;
		}
	next:
		position += (uint32_t)size;
	}
	return have_file_properties && have_stream ? 0 : -1;
}

struct wma_file *wma_file_open(wma_read_proc read, void *context, struct wma_file_format *format)
{
	struct wma_file *file = calloc(1, sizeof(*file));
	uint8_t top[30], data_top[50];
	uint8_t *header = NULL;
	uint64_t header_size, data_size;

	if (!file)
		return NULL;
	file->read = read;
	file->context = context;
	if (file_read_exact(file, 0, sizeof(top), top) || memcmp(top, guid_header, 16))
		goto fail;
	header_size = read_le64(top + 16);
	if (header_size < 30 || header_size > ASF_MAXIMUM_HEADER)
		goto fail;
	header = malloc((size_t)header_size);
	if (!header)
		goto fail;
	memcpy(header, top, sizeof(top));
	if (file_read_exact(file, 30, (uint32_t)header_size - 30, header + 30)
		|| parse_header(file, header, (uint32_t)header_size))
		goto fail;
	free(header);
	header = NULL;
	/* the data object follows the header */
	if (file_read_exact(file, (uint32_t)header_size, sizeof(data_top), data_top)
		|| memcmp(data_top, guid_data, 16))
		goto fail;
	data_size = read_le64(data_top + 16);
	file->format.first_packet_offset = (uint32_t)header_size + 50;
	if (!file->format.packet_size || file->format.packet_size > ASF_MAXIMUM_PACKET || data_size < 50)
		goto fail;
	file->format.packet_count = (uint32_t)read_le64(data_top + 40);
	/* trust the data object's size over its packet count */
	if ((uint64_t)file->format.packet_count * file->format.packet_size > data_size - 50)
		file->format.packet_count = (uint32_t)((data_size - 50) / file->format.packet_size);
	file->packet = malloc(file->format.packet_size);
	file->pcm = malloc(WMA_MAXIMUM_SUPERFRAME_SAMPLES * sizeof(int16_t) * (size_t)file->format.channels);
	if (!file->packet || !file->pcm)
		goto fail;
	file->object_number = -1;
	if (format)
		*format = file->format;
	return file;
fail:
	free(header);
	wma_file_close(file);
	return NULL;
}

void wma_file_close(struct wma_file *file)
{
	if (!file)
		return;
	wma_codec_destroy(file->codec);
	free(file->packet);
	free(file->object);
	free(file->pcm);
	free(file->queue);
	free(file);
}

void wma_file_rewind(struct wma_file *file)
{
	file->next_packet = 0;
	file->object_number = -1;
	file->object_size = file->object_filled = 0;
	file->pcm_frames = file->pcm_position = 0;
	file->queue_size = file->queue_position = 0;
	file->frames_out = 0;
	file->ended = 0;
	wma_codec_reset(file->codec);
}

int wma_file_finished(const struct wma_file *file)
{
	return file->ended && file->pcm_position >= file->pcm_frames
		&& file->queue_size - file->queue_position < (uint32_t)file->format.block_align;
}

/* a field of 0, 1, 2 or 4 bytes (ASF length type 0-3) */
static int read_typed(const uint8_t **cursor, const uint8_t *end, int type, uint32_t *value)
{
	static const int sizes[4] = { 0, 1, 2, 4 };
	int size = sizes[type & 3];
	const uint8_t *p = *cursor;

	if (end - p < size)
		return -1;
	*value = size == 0 ? 0 : size == 1 ? p[0] : size == 2 ? read_le16(p) : read_le32(p);
	*cursor = p + size;
	return 0;
}

/* a whole media object: queue its superframes (block_align bytes each) */
static void queue_object(struct wma_file *file, const uint8_t *data, uint32_t size)
{
	uint32_t block = (uint32_t)file->format.block_align;

	size -= size % block;
	if (!size)
		return;
	if (file->queue_position)
	{
		memmove(file->queue, file->queue + file->queue_position, file->queue_size - file->queue_position);
		file->queue_size -= file->queue_position;
		file->queue_position = 0;
	}
	if (file->queue_size + size > file->queue_capacity)
	{
		uint32_t capacity = file->queue_size + size;
		uint8_t *queue = realloc(file->queue, capacity);

		if (!queue)
			return;
		file->queue = queue;
		file->queue_capacity = capacity;
	}
	memcpy(file->queue + file->queue_size, data, size);
	file->queue_size += size;
}

/* reads and decodes the next packet; 0 when there are none left */
static int next_packet(struct wma_file *file)
{
	const uint8_t *p, *end;
	uint32_t packet_length = 0, sequence, padding = 0, payload_count = 1;
	uint8_t flags, property_flags, payload_flags = 0;
	uint32_t i;

	if (file->next_packet >= file->format.packet_count)
		return 0;
	if (file_read_exact(file, file->format.first_packet_offset + file->next_packet * file->format.packet_size,
			file->format.packet_size, file->packet))
	{
		file->next_packet = file->format.packet_count;
		return 0;
	}
	file->next_packet++;
	p = file->packet;
	end = p + file->format.packet_size;

	flags = *p++;
	if (flags & 0x80)
	{
		/* error correction data */
		p += flags & 0x0f;
		if (p >= end)
			goto bad;
		flags = *p++;
	}
	if (p >= end)
		goto bad;
	property_flags = *p++;
	if (read_typed(&p, end, flags >> 5, &packet_length) || read_typed(&p, end, flags >> 1, &sequence)
		|| read_typed(&p, end, flags >> 3, &padding) || end - p < 6)
		goto bad;
	p += 6; /* send time, duration */
	if (packet_length && packet_length < file->format.packet_size)
		padding += file->format.packet_size - packet_length;
	if (padding > (uint32_t)(end - p))
		goto bad;
	end -= padding;
	if (flags & 0x01)
	{
		if (p >= end)
			goto bad;
		payload_flags = *p++;
		payload_count = payload_flags & 0x3f;
	}
	for (i = 0; i < payload_count && p < end; i++)
	{
		uint32_t object_number, object_offset, replicated, payload_length;
		uint32_t object_size = 0;
		int stream = *p++ & 0x7f;

		if (read_typed(&p, end, property_flags >> 4, &object_number)
			|| read_typed(&p, end, property_flags >> 2, &object_offset)
			|| read_typed(&p, end, property_flags, &replicated))
			goto bad;
		if (replicated == 1)
		{
			/* compressed payloads: object_offset is the presentation time; then
			sub-payloads of one byte length each a whole object */
			const uint8_t *payload_end;

			if (p >= end)
				goto bad;
			p++; /* presentation time delta */
			if (flags & 0x01)
			{
				if (read_typed(&p, end, payload_flags >> 6, &payload_length) || payload_length > (uint32_t)(end - p))
					goto bad;
			}
			else
				payload_length = (uint32_t)(end - p);
			payload_end = p + payload_length;
			while (p < payload_end)
			{
				uint32_t sub = *p++;

				if (sub > (uint32_t)(payload_end - p))
					goto bad;
				if (stream == file->stream_number)
					queue_object(file, p, sub);
				p += sub;
			}
			continue;
		}
		if (replicated > (uint32_t)(end - p))
			goto bad;
		if (replicated >= 4)
			object_size = read_le32(p);
		p += replicated;
		if (flags & 0x01)
		{
			if (read_typed(&p, end, payload_flags >> 6, &payload_length) || payload_length > (uint32_t)(end - p))
				goto bad;
		}
		else
			payload_length = (uint32_t)(end - p);
		if (stream == file->stream_number)
		{
			if (object_offset == 0 && payload_length == object_size)
				queue_object(file, p, payload_length);
			else
			{
				/* a fragment: assemble the object */
				if (object_offset == 0)
				{
					if (!object_size || object_size > ASF_MAXIMUM_OBJECT)
					{
						p += payload_length;
						continue;
					}
					if (object_size > file->object_size || !file->object)
					{
						uint8_t *object = realloc(file->object, object_size);

						if (!object)
							goto bad;
						file->object = object;
					}
					file->object_size = object_size;
					file->object_filled = 0;
					file->object_number = (int)object_number;
				}
				if ((int)object_number == file->object_number && object_offset == file->object_filled
					&& payload_length <= file->object_size - file->object_filled)
				{
					memcpy(file->object + file->object_filled, p, payload_length);
					file->object_filled += payload_length;
					if (file->object_filled == file->object_size)
					{
						queue_object(file, file->object, file->object_size);
						file->object_number = -1;
					}
				}
				else
					file->object_number = -1; /* lost a fragment: drop the object */
			}
		}
		p += payload_length;
	}
	return 1;
bad:
	file->errors++;
	return 1;
}

uint32_t wma_file_read(struct wma_file *file, int16_t *output, uint32_t frames)
{
	int channels = file->format.channels;
	uint32_t written = 0;

	while (written < frames)
	{
		uint32_t available = file->pcm_frames - file->pcm_position;

		if (!available)
		{
			uint32_t block = (uint32_t)file->format.block_align;

			file->pcm_frames = file->pcm_position = 0;
			if (file->queue_size - file->queue_position >= block)
			{
				int decoded = wma_codec_decode(file->codec, file->queue + file->queue_position, (int)block, file->pcm);

				file->queue_position += block;
				if (decoded < 0)
					file->errors++;
				else if (decoded == 0 && !file->continuations++)
					file->first_continuation = file->frames_out;
				if (decoded > 0)
					file->pcm_frames = (uint32_t)decoded;
				continue;
			}
			if (file->ended)
				break;
			if (!next_packet(file))
				file->ended = 1;
			continue;
		}
		if (available > frames - written)
			available = frames - written;
		memcpy(output + (size_t)written * channels, file->pcm + (size_t)file->pcm_position * channels,
			(size_t)available * channels * sizeof(int16_t));
		file->pcm_position += available;
		file->frames_out += available;
		written += available;
	}
	return written;
}

uint32_t wma_file_errors(const struct wma_file *file)
{
	return file->errors;
}

uint32_t wma_file_continuations(const struct wma_file *file, uint64_t *first_frame)
{
	if (first_frame)
		*first_frame = file->first_continuation;
	return file->continuations;
}
