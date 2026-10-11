/*
WEB_ICALL.C

The report of port/web/tools/wasm_icall_adapt.py: the first indirect call of each function
table entry whose declared type is not the callee's (the adapter then passes the arguments
as AArch64 would; docs/WEB.md). build-web/openfrenzy.icall.txt names the entries and types.
*/

#include <emscripten.h>
#include <stdint.h>

#include "platform.h"

#define REPORTED_ENTRIES 8192

static uint32_t reported[REPORTED_ENTRIES / 32];

EMSCRIPTEN_KEEPALIVE void of_web_icall_mismatch(int index, int caller_type, int callee_type)
{
	uint32_t bit;

	if (index < 0 || index >= REPORTED_ENTRIES)
		return;
	bit = 1u << (index & 31);
	if (__atomic_fetch_or(&reported[index >> 5], bit, __ATOMIC_RELAXED) & bit)
		return;
	platform_log("web: indirect call of table entry %d as type %d, its type is %d (adapted)",
		index, caller_type, callee_type);
}
