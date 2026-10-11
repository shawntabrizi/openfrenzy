/*
XDK_ENTRY_POINTS.C

openfrenzy: the table that routes the XBE's statically linked XDK library
functions (sections D3D, DSOUND, XGRPH, XPP, WMADECXM) to the HLE layer in src/<platform modules>.
Rows come from xdk_entry_points.inc (scripts/gen_xdk_entry_points.py).
Look-up is by original XBE address, so decompiled game code (or a tool that
resolves FUN_xxxxxxxx call targets) can find the replacement.

This file includes no XDK header on purpose: each HLE function is declared
here only as an address, so one table can hold functions of every signature.
Callers cast hle to the XDK prototype of xbe_name. Rows without an HLE
implementation that game code calls get a loud stub (docs/ARCHITECTURE.md §7).
*/

#include "xdk_entry_points.h"

#include <stddef.h>

/* platform.h (xbox_kernel.c): logs an unimplemented SDK entry point once */
void platform_unimplemented(const char *name);

/* HLE rows: the implementation. MISSING and UNKNOWN rows: a stub that logs
loudly and returns 0 (ignoring any arguments is safe in the AArch64 calling
convention). INTERNAL rows: nothing. */
#define XDK_DECLARE_HLE(address, xbe_name, hle_name) extern void hle_name(void);
#define XDK_DECLARE_STUB(address, xbe_name) \
	static unsigned long xdk_stub_##address(void) { platform_unimplemented(xbe_name); return 0; }
#define XDK_DECLARE_MISSING(address, xbe_name, hle_name) XDK_DECLARE_STUB(address, xbe_name)
#define XDK_DECLARE_UNKNOWN(address, xbe_name, hle_name) XDK_DECLARE_STUB(address, xbe_name)
#define XDK_DECLARE_INTERNAL(address, xbe_name, hle_name)
#define XDK_ENTRY(address, section, xbe_name, hle_name, kind) XDK_DECLARE_##kind(address, xbe_name, hle_name)
#include "xdk_entry_points.inc"
#undef XDK_ENTRY

#define XDK_HLE_ADDRESS_HLE(address, hle_name) (void (*)(void))hle_name
#define XDK_HLE_ADDRESS_MISSING(address, hle_name) (void (*)(void))xdk_stub_##address
#define XDK_HLE_ADDRESS_UNKNOWN(address, hle_name) (void (*)(void))xdk_stub_##address
#define XDK_HLE_ADDRESS_INTERNAL(address, hle_name) NULL
#define XDK_ENTRY(address, section, xbe_name, hle_name, kind) \
	{ address, section, xbe_name, XDK_HLE_ADDRESS_##kind(address, hle_name), _xdk_entry_##kind },
const struct xdk_entry_point xdk_entry_points[] =
{
#include "xdk_entry_points.inc"
};
#undef XDK_ENTRY

const unsigned long xdk_entry_point_count = sizeof(xdk_entry_points) / sizeof(xdk_entry_points[0]);

const struct xdk_entry_point *xdk_entry_point_find(unsigned long address)
{
	unsigned long low = 0;
	unsigned long high = xdk_entry_point_count;

	/* rows are sorted by address */
	while (low < high)
	{
		unsigned long middle = low + (high - low) / 2;

		if (xdk_entry_points[middle].address < address)
			low = middle + 1;
		else
			high = middle;
	}
	if (low < xdk_entry_point_count && xdk_entry_points[low].address == address)
		return &xdk_entry_points[low];
	return NULL;
}
