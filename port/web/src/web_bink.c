/*
WEB_BINK.C

Bink video for the web build: the Darwin host's port/macos/host/host_bink.c (the host_bink_*
calls of src/dsound/bink_host.c, decoded with FFmpeg), compiled here as it is. Its host.h
gives it the host's log, which is defined below; the guard name keeps the Darwin header out.
Without FFmpeg (OPENFRENZY_BINK_FFMPEG 0) every open fails and the game skips its videos.
*/

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/* port/macos/host/host.h, as far as host_bink.c uses it */
#define __OPENFRENZY_MACOS_HOST_H
#define HOST_LOG_INFO 4
#define HOST_LOG_WARN 5
#define HOST_LOG_ERROR 6

void host_logf(int priority, const char *format, ...) __attribute__((format(printf, 2, 3)));

void host_logf(int priority, const char *format, ...)
{
	char line[1024];
	va_list arguments;

	(void)priority;
	va_start(arguments, format);
	vsnprintf(line, sizeof(line), format, arguments);
	va_end(arguments);
	fprintf(stderr, "openfrenzy: %s\n", line);
}

#include "../../macos/host/host_bink.c"
