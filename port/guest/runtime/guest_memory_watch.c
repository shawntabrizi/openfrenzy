/* Adapted from ref/OpenCE-mac-zimm3rmann (github.com/zimm3rmann/halo-ce-ios-macos @ e55684a) port/android/guest/runtime/guest_memory_watch.c (CC0). Renamed HALO_* to OPENFRENZY_*. */
/*
GUEST_MEMORY_WATCH.C

Guest memory write tracking (port/linux/src/memory_watch.c's interface).
Page protection faults are delivered to the host, which owns signal
handling in the Android process, so the tracking itself lives there
(port/android/host/host_memory.c).
*/

#include "platform.h"
#include "guest_host.h"

void memory_watch_initialize(void)
{
	host_memory_watch_initialize();
}

void memory_watch_protect(unsigned long address, unsigned long size)
{
	host_memory_watch_protect(address, size);
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
	return host_memory_watch_generation(address, size);
}

unsigned long memory_watch_serial(void)
{
	return host_memory_watch_serial();
}

void memory_watch_prepare_write(void *address, unsigned long size)
{
	host_memory_watch_prepare_write((unsigned int)address, size);
}

void memory_watch_forget(void *address, unsigned long size)
{
	host_memory_watch_forget((unsigned int)address, size);
}

/* openfrenzy: the OpenCE layer at 231708d also calls this once per frame */
void memory_watch_begin_frame(void)
{
	host_memory_watch_begin_frame();
}
