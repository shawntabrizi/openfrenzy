/*
TEST_WMA.C

Native (host) test of the WMA decoder (src/dsound/wma_decoder.c) on the game's own music.

It opens the disc image, reads Packages/Music.xbp from it in place (src/vfs/xiso_fs.c), finds
the ASF streams in the package (each starts on a 2048-byte boundary with the ASF header
GUID) and decodes each one through wma_file_* with a read callback that checks the
contract the game's callback (0x00056780) needs: reads are contiguous and in order, never
past the data object, and each request fits the game's 64 KB load blocks.

Pass when every stream opens as WMA v2, decodes with no errors, gives the samples its
packets hold (frames = superframes x 2048 minus the reservoir's first frame), and is not
silent.

  test_wma ISO                       all streams of Packages/Music.xbp
  test_wma ISO --package P           another package (e.g. Packages/ScratchMusic.xbp)
  test_wma ISO --stream N --dump-asf F --dump-pcm F
                                     one stream; write its ASF bytes and the decoded
                                     16-bit stereo PCM (for tests/compare_ffmpeg.sh)

Build: tests/run_tests.sh (Apple clang, no guest toolchain needed).
*/

#include "wma_decoder.h"
#include "xiso_fs.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char asf_header_guid[16] =
	{ 0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C };

struct source
{
	const unsigned char *bytes;   /* the stream's bytes (in the package) */
	unsigned long size;
	unsigned char window[65536];
	unsigned long next_offset;    /* where the next read must start */
	int violations;
	unsigned long requests;
	FILE *dump;
};

/* the game's ring buffer gives at most what was loaded; requests here must be
contiguous and small (the game's buffer is two 64 KB halves) */
static int read_proc(void *context, uint32_t offset, uint32_t size, const uint8_t **data)
{
	struct source *source = context;

	source->requests++;
	if (offset != source->next_offset)
	{
		if (source->violations++ < 5)
			fprintf(stderr, "  read at %u, expected the next at %lu\n", offset, source->next_offset);
	}
	if (size > sizeof(source->window) / 2)
	{
		if (source->violations++ < 5)
			fprintf(stderr, "  read of %u bytes is larger than a 64 KB half\n", size);
		size = sizeof(source->window) / 2;
	}
	if (offset >= source->size)
	{
		if (source->violations++ < 5)
			fprintf(stderr, "  read at %u is past the stream (%lu bytes)\n", offset, source->size);
		return -1;
	}
	if (size > source->size - offset)
		size = (uint32_t)(source->size - offset);
	memcpy(source->window, source->bytes + offset, size);
	source->next_offset = offset + size;
	*data = source->window;
	if (source->dump)
		fwrite(source->bytes + offset, 1, size, source->dump);
	return (int)size;
}

static int decode_stream(int index, const unsigned char *bytes, unsigned long size, const char *dump_asf,
	const char *dump_pcm)
{
	static int16_t pcm[4096 * 2];
	struct source *source = calloc(1, sizeof(*source));
	struct wma_file_format format;
	struct wma_file *file;
	FILE *pcm_out = NULL;
	unsigned long long frames = 0;
	unsigned int hash = 2166136261u;
	double energy = 0;
	int failed = 0;
	uint32_t got;

	source->bytes = bytes;
	source->size = size;
	if (dump_asf)
		source->dump = fopen(dump_asf, "wb");
	if (dump_pcm)
		pcm_out = fopen(dump_pcm, "wb");
	file = wma_file_open(read_proc, source, &format);
	if (!file)
	{
		printf("stream %2d: FAIL: not opened as WMA\n", index);
		free(source);
		return 1;
	}
	while ((got = wma_file_read(file, pcm, 4096)) > 0)
	{
		uint32_t i;

		for (i = 0; i < got * 2; i++)
			energy += (double)pcm[i] * pcm[i];
		/* FNV-1a over the bytes (little-endian), as tests/test_dsound_guest.c */
		for (i = 0; i < got * 4; i++)
			hash = (hash ^ ((const unsigned char *)pcm)[i]) * 16777619u;
		if (pcm_out)
			fwrite(pcm, sizeof(int16_t) * 2, got, pcm_out);
		frames += got;
	}
	/* 2048 samples a frame; the first frame of the stream only fills the bit
	reservoir, so each packet's superframe gives (frames in it) x 2048 and the total
	is a multiple of 2048 */
	if (format.format_tag != WMA_FORMAT_TAG_V2 || format.channels != 2 || format.sample_rate != 44100)
	{
		printf("stream %2d: FAIL: format 0x%x %d ch %d Hz\n", index, format.format_tag, format.channels,
			format.sample_rate);
		failed = 1;
	}
	if (wma_file_errors(file))
	{
		printf("stream %2d: FAIL: %u decode errors\n", index, wma_file_errors(file));
		failed = 1;
	}
	if (source->violations)
	{
		printf("stream %2d: FAIL: %d read contract violations\n", index, source->violations);
		failed = 1;
	}
	if (!frames || frames % 2048)
	{
		printf("stream %2d: FAIL: %llu frames (not a multiple of 2048)\n", index, frames);
		failed = 1;
	}
	{
		/* the play duration (100 ns units) includes the preroll (ms) */
		double seconds = format.play_duration / 1e7 - format.preroll / 1e3;
		double decoded = frames / 44100.0;
		double rms = frames ? sqrt(energy / (frames * 2.0)) : 0;

		if (fabs(decoded - seconds) > 0.25)
		{
			printf("stream %2d: FAIL: %.3f s decoded, the header says %.3f s\n", index, decoded, seconds);
			failed = 1;
		}
		if (rms < 50)
		{
			printf("stream %2d: FAIL: silent (rms %.1f)\n", index, rms);
			failed = 1;
		}
		if (!wma_file_finished(file))
		{
			printf("stream %2d: FAIL: not finished\n", index);
			failed = 1;
		}
		uint64_t first_continuation = 0;
		uint32_t continuations = wma_file_continuations(file, &first_continuation);

		printf("stream %2d: %s %7lu bytes, %4u packets, %8llu frames = %7.3f s (header %7.3f s), rms %.3f, %lu reads, hash %08x, %u continuation superframes",
			index, failed ? "FAIL" : "ok  ", size, format.packet_count, frames, decoded, seconds, rms,
			source->requests, hash, continuations);
		if (continuations)
			printf(" (first at frame %llu)", (unsigned long long)first_continuation);
		printf("\n");
	}
	/* rewind and decode the first second again: the same samples (the game restarts
	a stream for its loop by creating a new decoder; rewind is the cheap way) */
	if (!failed)
	{
		static int16_t first[44100 * 2], again[44100 * 2];
		struct source *second = calloc(1, sizeof(*second));
		struct wma_file *file2;

		second->bytes = bytes;
		second->size = size;
		file2 = wma_file_open(read_proc, second, NULL);
		if (file2 && wma_file_read(file2, first, 44100) == 44100)
		{
			wma_file_rewind(file2);
			/* rewinding reads the packets again from the first one */
			second->next_offset = format.first_packet_offset;
			if (wma_file_read(file2, again, 44100) != 44100 || memcmp(first, again, sizeof(first)))
			{
				printf("stream %2d: FAIL: rewind gives other samples\n", index);
				failed = 1;
			}
		}
		else
		{
			printf("stream %2d: FAIL: second open\n", index);
			failed = 1;
		}
		wma_file_close(file2);
		free(second);
	}
	wma_file_close(file);
	if (source->dump)
		fclose(source->dump);
	if (pcm_out)
		fclose(pcm_out);
	free(source);
	return failed;
}

int main(int argc, char **argv)
{
	const char *package = "Packages/Music.xbp";
	const char *dump_asf = NULL, *dump_pcm = NULL;
	int only = -1, i, count = 0, failures = 0;
	struct xiso_fs fs;
	struct xiso_entry entry;
	unsigned char *bytes;
	unsigned long offset;

	if (argc < 2)
	{
		fprintf(stderr, "usage: %s ISO [--package P] [--stream N] [--dump-asf F] [--dump-pcm F]\n", argv[0]);
		return 2;
	}
	for (i = 2; i + 1 < argc; i += 2)
	{
		if (!strcmp(argv[i], "--package"))
			package = argv[i + 1];
		else if (!strcmp(argv[i], "--stream"))
			only = atoi(argv[i + 1]);
		else if (!strcmp(argv[i], "--dump-asf"))
			dump_asf = argv[i + 1];
		else if (!strcmp(argv[i], "--dump-pcm"))
			dump_pcm = argv[i + 1];
	}
	if (xiso_fs_open(&fs, argv[1]))
	{
		fprintf(stderr, "cannot open the disc image %s\n", argv[1]);
		return 2;
	}
	if (xiso_fs_find(&fs, package, &entry) || entry.is_directory)
	{
		fprintf(stderr, "%s is not on the disc\n", package);
		return 2;
	}
	bytes = malloc(entry.size);
	if (!bytes || xiso_fs_read(&fs, &entry, 0, bytes, entry.size) != (long)entry.size)
	{
		fprintf(stderr, "cannot read %s\n", package);
		return 2;
	}
	printf("%s: %u bytes from the disc image\n", package, entry.size);
	for (offset = 0; offset + 30 <= entry.size; offset += 2048)
	{
		unsigned long long header_size, data_size, total;

		if (memcmp(bytes + offset, asf_header_guid, 16))
			continue;
		header_size = 0;
		memcpy(&header_size, bytes + offset + 16, 8);
		if (offset + header_size + 24 > entry.size)
			continue;
		memcpy(&data_size, bytes + offset + header_size + 16, 8);
		total = header_size + data_size;
		if (offset + total > entry.size)
			total = entry.size - offset;
		if (only < 0 || only == count)
			failures += decode_stream(count, bytes + offset, (unsigned long)total, dump_asf, dump_pcm);
		count++;
	}
	printf("%d streams, %d failed\n", count, failures);
	free(bytes);
	xiso_fs_close(&fs);
	if (!count)
		return 1;
	return failures ? 1 : 0;
}
