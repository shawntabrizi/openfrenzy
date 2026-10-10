/* openfrenzy: the one piece of OpenCE's port/linux/game/cache_file_formats.h
that xbox_textures.c needs (Halo Custom Edition texture channel orders).
Fuzion Frenzy never sets an order other than _custom_edition_channels_xbox. */
#ifndef OPENFRENZY_HALO_CACHE_FILE_FORMATS_H
#define OPENFRENZY_HALO_CACHE_FILE_FORMATS_H

enum custom_edition_channel_order
{
	_custom_edition_channels_xbox,
	_custom_edition_channels_multipurpose,
	_custom_edition_channels_hud_meter,

	NUMBER_OF_CUSTOM_EDITION_CHANNEL_ORDERS
};

#endif
