/* Copied from ref/OpenCE (commit 231708d) port/linux/src/bink_null.c (CC0). The game calls BinkOpen
   (0x000591c0) for the Blitz logos and the attract video; NULL makes it skip them.

   openfrenzy: the XBE links RAD Bink statically (region LIB:bink 0x00057980-0x000703a0, 222
   functions, its own DirectSound mixer at 0x00069df0 on). Game code calls three of them, all
   from BDISPLAY's bdVideoPlayStream (0x000463c0) and its tail (0x00046b20):
     0x0005a090  RADSetMemory(allocate, free): stores the two procedures that radmalloc
                 (0x0005a0b0) and radfree (0x0005a120) use; XbSymbolDatabase calls it
                 D3D_SetPushBufferSize, a false match on its two stores
     0x000591c0  BinkOpen(name, flags) ("Not a Bink file.")
     0x00059f80  BinkClose(bink): BinkClose's work (stops the sound tracks, frees the
                 buffers, clears the 0x404-byte handle, frees it)
   With BinkOpen NULL bdVideoPlayStream returns 6 at once and never reaches the others.
   Declarations: dsound_xdk.h. */
/*
BINK_NULL.C

The Bink video SDK entry points bink_playback.c uses. There is no Bink
decoder in the native builds (the RAD SDK is proprietary), so BinkOpen reports that a movie cannot be
opened and the game skips it, exactly as it does for a missing movie file.

The prototypes match the declarations in bink_playback.c; the RAD SDK's
RADEXPLINK is __stdcall.
*/

#include "platform.h"
#include "dsound_xdk.h"

typedef void *(__stdcall *bink_sound_system_open_proc)(unsigned long param);

void __stdcall RADSetMemory(rad_memory_allocate_proc allocate, rad_memory_free_proc release)
{
	(void)allocate;
	(void)release;
}

void *__stdcall BinkOpenDirectSound(unsigned long param)
{
	(void)param;
	return NULL;
}

long __stdcall BinkSetSoundSystem(bink_sound_system_open_proc open, unsigned long param)
{
	(void)open;
	(void)param;
	return 0;
}

void __stdcall BinkSetIOSize(unsigned long io_size)
{
	(void)io_size;
}

HBINK __stdcall BinkOpen(const char *name, unsigned long flags)
{
	(void)flags;
	platform_log("Bink video is not supported; skipping \"%s\"", name ? name : "");
	return NULL;
}

/* never reached without an open movie */

void __stdcall BinkClose(HBINK bink) { (void)bink; }
long __stdcall BinkDoFrame(HBINK bink) { (void)bink; return 0; }
void __stdcall BinkNextFrame(HBINK bink) { (void)bink; }
long __stdcall BinkWait(HBINK bink) { (void)bink; return 0; }

long __stdcall BinkCopyToBuffer(HBINK bink, void *destination, long destination_pitch,
	unsigned long destination_height, unsigned long destination_x, unsigned long destination_y,
	unsigned long flags)
{
	(void)bink; (void)destination; (void)destination_pitch; (void)destination_height;
	(void)destination_x; (void)destination_y; (void)flags;
	return 0;
}

void __stdcall BinkGetSummary(HBINK bink, void *summary) { (void)bink; (void)summary; }
void __stdcall BinkGetRealtime(HBINK bink, void *realtime, unsigned long frame_count) { (void)bink; (void)realtime; (void)frame_count; }
