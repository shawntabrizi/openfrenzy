/*
PCM_COMPARE.C

Compares two raw 16-bit stereo PCM files (ours and FFmpeg's decode of the same WMA stream).
FFmpeg's command line drops the decoder's first 4096 samples (its start delay), so ours is
read OFFSET frames ahead (default 4096).

  pcm_compare OURS.raw REFERENCE.raw [OFFSET]

Prints the largest difference, the samples that differ by more than 1 (float rounding:
our FFT is not FFmpeg's), the first such frame and the signal-to-difference ratio. Exits
0 when no sample differs by more than 1 and at least 99% of the reference is covered.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static short *load(const char *path, long *frames)
{
	FILE *file = fopen(path, "rb");
	long size;
	short *data;

	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = malloc((size_t)size + 4);
	if (!data || fread(data, 1, (size_t)size, file) != (size_t)size)
	{
		fclose(file);
		free(data);
		return NULL;
	}
	fclose(file);
	*frames = size / 4;
	return data;
}

int main(int argc, char **argv)
{
	long ours_frames, reference_frames, offset = argc > 3 ? atol(argv[3]) : 4096, i, count;
	short *ours, *reference;
	long larger = 0, first = -1, maximum = 0;
	double signal = 0, error = 0;

	if (argc < 3)
	{
		fprintf(stderr, "usage: %s OURS.raw REFERENCE.raw [OFFSET]\n", argv[0]);
		return 2;
	}
	ours = load(argv[1], &ours_frames);
	reference = load(argv[2], &reference_frames);
	if (!ours || !reference)
	{
		fprintf(stderr, "cannot read the inputs\n");
		return 2;
	}
	count = ours_frames - offset < reference_frames ? ours_frames - offset : reference_frames;
	for (i = 0; i < count * 2; i++)
	{
		long difference = labs((long)ours[i + offset * 2] - reference[i]);

		if (difference > maximum)
			maximum = difference;
		if (difference > 1)
		{
			larger++;
			if (first < 0)
				first = i / 2;
		}
		signal += (double)reference[i] * reference[i];
		error += (double)difference * difference;
	}
	printf("frames: ours %ld (offset %ld), reference %ld, compared %ld; max difference %ld, %ld samples > 1",
		ours_frames, offset, reference_frames, count, maximum, larger);
	if (first >= 0)
		printf(" (first at frame %ld = %.3f s)", first, first / 44100.0);
	printf(", SNR %.1f dB\n", error > 0 ? 10 * log10(signal / error) : INFINITY);
	return larger == 0 && count >= reference_frames * 99 / 100 ? 0 : 1;
}
