/*
XDK_ENTRY_POINTS.H

openfrenzy: XBE address -> HLE function for the statically linked XDK
libraries (xdk_entry_points.c).
*/

#ifndef OPENFRENZY_XDK_ENTRY_POINTS_H
#define OPENFRENZY_XDK_ENTRY_POINTS_H

enum xdk_entry_kind
{
	/* the HLE layer implements it */
	_xdk_entry_HLE,
	/* game code calls it; no HLE implementation yet: hle is a loud stub */
	_xdk_entry_MISSING,
	/* game code calls it; not yet identified: hle is a loud stub */
	_xdk_entry_UNKNOWN,
	/* only called inside the library; nothing to route: hle is NULL */
	_xdk_entry_INTERNAL,
};

struct xdk_entry_point
{
	unsigned long address; /* in the XBE */
	const char *section;   /* D3D, DSOUND, XGRPH or XPP */
	const char *xbe_name;  /* XbSymbolDatabase name, or FUN_<address> */
	void (*hle)(void);     /* cast to xbe_name's XDK prototype */
	enum xdk_entry_kind kind;
};

extern const struct xdk_entry_point xdk_entry_points[];
extern const unsigned long xdk_entry_point_count;

/* the row for an XBE address, or NULL */
const struct xdk_entry_point *xdk_entry_point_find(unsigned long address);

#endif
