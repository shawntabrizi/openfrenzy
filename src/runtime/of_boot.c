/*
OF_BOOT.C

The game image's main(). It replaces the XBE start-up code (XAPI region, not
decompiled), which on the Xbox runs:

  entry (0x00030f7f)
    -> CreateThread(mainXapiStartup 0x00030f10)
         XapiInitProcess, _rtinit,
         _cinit (0x000340f8): .CRT$XI (LIBC) then .CRT$XC (game static constructors)
         main (0x0002d540)      -> core module
         XapiBootToDash

The platform layer starts itself on first use (OpenCE: platform_sdl_initialize),
so boot only runs the static constructors and the game's main.

Built with the game flags (of_game.h). openfrenzy runtime module.
*/

#include <stdio.h>
#include <stdlib.h>

void of_run_static_constructors(void);   /* generated: gen/of_crt_init.c */

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	fprintf(stderr, "openfrenzy: static constructors\n");
	of_run_static_constructors();
	/* OPENFRENZY_BOOT_ONLY=1 (the ctest guest_guest_runs): stop after the constructors,
	   because the game's main never returns. */
	if (getenv("OPENFRENZY_BOOT_ONLY") != NULL)
	{
		fprintf(stderr, "openfrenzy: OPENFRENZY_BOOT_ONLY set, game main skipped\n");
		return 0;
	}
	fprintf(stderr, "openfrenzy: game main 0x0002d540\n");
	FUN_0002d540();
	/* the Xbox returns to the dashboard here (XapiBootToDash) */
	fprintf(stderr, "openfrenzy: game main returned\n");
	return 0;
}
