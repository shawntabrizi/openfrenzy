/*
XKERNEL.H

openfrenzy (xkernel module). Declarations of the kernel and XAPI entry points
that this game calls and that the shared XDK headers (src/runtime/include/xdk)
do not declare. Implemented in xbox_kernel.c, xbox_xapi.c and xbox_memory.c.
XMountUtilityDrive and GetFileInformationByHandle belong to the vfs module
(src/vfs/vfs_files.h).

Game code: #include "xkernel.h" (after "xtl.h", which of_game.h does not pull in
on its own). xkernel_libcalls.h maps every XBE address of the
LIB:xapi and LIB:libc regions that the game calls to its replacement.

Prototypes follow the XDK 3911 signatures (public Xbox/Win32 documentation and
the call sites in this XBE).
*/

#ifndef OPENFRENZY_XKERNEL_H
#define OPENFRENZY_XKERNEL_H

#include <xtl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- constants missing from the shared headers */

#ifndef ERROR_NOT_FOUND
#define ERROR_NOT_FOUND 1168L /* 0x490: XLoadSection of an unknown section */
#endif

/* XGetVideoStandard */
#define XC_VIDEO_STANDARD_NTSC_M 1
#define XC_VIDEO_STANDARD_NTSC_J 2
#define XC_VIDEO_STANDARD_PAL_I 3

/* XGetVideoFlags */
#define XC_VIDEO_FLAGS_WIDESCREEN 0x00000001
#define XC_VIDEO_FLAGS_HDTV_720p 0x00000002
#define XC_VIDEO_FLAGS_HDTV_1080i 0x00000004
#define XC_VIDEO_FLAGS_HDTV_480p 0x00000008
#define XC_VIDEO_FLAGS_LETTERBOX 0x00000010
#define XC_VIDEO_FLAGS_PAL_60Hz 0x00000040

/* XGetGameRegion */
#define XC_GAME_REGION_NA 0x00000001
#define XC_GAME_REGION_JAPAN 0x00000002
#define XC_GAME_REGION_RESTOFWORLD 0x00000004

/* ---------- types missing from the shared headers */

typedef VOID (NTAPI *PAPCFUNC)(ULONG_PTR data);

#define MAX_SOUNDTRACK_NAME 32
#define MAX_SONG_NAME 32

typedef struct _XSOUNDTRACK_DATA
{
	UINT uSoundtrackId;
	UINT uSongCount;
	UINT uSoundtrackLength;
	WCHAR szName[MAX_SOUNDTRACK_NAME];
} XSOUNDTRACK_DATA, *PXSOUNDTRACK_DATA;

/* ---------- kernel (xbox_kernel.c, xbox_memory.c) */

VOID NTAPI DbgBreakPoint(VOID);
VOID NTAPI KeQuerySystemTime(LARGE_INTEGER *current_time);
DWORD WINAPI QueueUserAPC(PAPCFUNC routine, HANDLE thread, ULONG_PTR data);
VOID WINAPI GetLocalTime(LPSYSTEMTIME system_time);

void *NTAPI MmAllocateContiguousMemory(ULONG size);
void *NTAPI MmAllocateContiguousMemoryEx(ULONG size, ULONG_PTR lowest_address,
	ULONG_PTR highest_address, ULONG alignment, ULONG protect);
VOID NTAPI MmFreeContiguousMemory(void *address);

/* Win32 string helpers of XAPI */
int WINAPI lstrlenA(LPCSTR string);
LPSTR WINAPI lstrcpyA(LPSTR destination, LPCSTR source);
LPSTR WINAPI lstrcpynA(LPSTR destination, LPCSTR source, int count);
/* cdecl, like the original; Win32 format rules (no floating point) */
int wsprintfA(LPSTR buffer, LPCSTR format, ...);

/* ---------- XAPI (xbox_xapi.c) */

DWORD WINAPI XGetVideoStandard(void);
DWORD WINAPI XGetVideoFlags(void);
DWORD WINAPI XGetGameRegion(void);
/* XBE sections: the port has none to load; both fail with ERROR_NOT_FOUND */
void *WINAPI XLoadSection(LPCSTR section_name);
BOOL WINAPI XFreeSection(LPCSTR section_name);

/* custom soundtracks (Xbox dashboard music): not supported, there are none */
HANDLE WINAPI XFindFirstSoundtrack(PXSOUNDTRACK_DATA data);
BOOL WINAPI XFindNextSoundtrack(HANDLE find, PXSOUNDTRACK_DATA data);
BOOL WINAPI XGetSoundtrackSongInfo(DWORD soundtrack_id, UINT index, PDWORD song_id,
	PDWORD song_length, LPWSTR name, UINT name_count);
HANDLE WINAPI XOpenSoundtrackSong(DWORD song_id, BOOL asynchronous);

#ifdef __cplusplus
}
#endif

#endif
