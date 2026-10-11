/*
WEB_BINK.C

Bink video for the web build (port/guest/runtime/guest_host.h host_bink_*). Without FFmpeg
every open fails and the game skips its videos (src/dsound/bink_host.c), as a native build
with OPENFRENZY_BINK_FFMPEG off does.
*/

#include "guest_host.h"

unsigned int host_bink_open(int descriptor, long long offset, long long size, unsigned int *info)
{
	(void)descriptor;
	(void)offset;
	(void)size;
	(void)info;
	return 0;
}

int host_bink_decode_frame(unsigned int bink)
{
	(void)bink;
	return 0;
}

int host_bink_copy_frame(unsigned int bink, void *destination, int pitch, int width, int height)
{
	(void)bink;
	(void)destination;
	(void)pitch;
	(void)width;
	(void)height;
	return 0;
}

int host_bink_read_audio(unsigned int bink, void *buffer, int size)
{
	(void)bink;
	(void)buffer;
	(void)size;
	return 0;
}

void host_bink_close(unsigned int bink)
{
	(void)bink;
}
