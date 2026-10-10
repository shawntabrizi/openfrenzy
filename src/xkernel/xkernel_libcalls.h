/*
XKERNEL_LIBCALLS.H

openfrenzy (xkernel module). The XBE's LIB:xapi and LIB:libc functions that game
and engine code calls (docs/LIBRARY_CALLS.md: 27 xapi + 75 libc; 2 more below), each with its
replacement. The 466 functions of these two regions are not decompiled: the
game calls the replacement by its C or XDK name.

Identified by reading export/funcs/<addr>.c, the x86 code (llvm-mc) and the
call sites; two more come from a byte scan of the game code for calls into
these regions (docs/LIBRARY_CALLS.md misses them: their callers are not
Ghidra functions). "(Ghidra)" marks a name Ghidra/FID already gave. Where the
original was a register-argument x87 intrinsic (_CIpow, _CIsin, ...), Ghidra
shows a call without arguments: the operands are the values Ghidra shows on
the x87 stack before the call (ST0 is the last one loaded).

| address    | replacement                     | notes |
|------------|---------------------------------|-------|
| xapi |||
| 0x0003055a | XGetLaunchInfo                  | (Ghidra) |
| 0x00030603 | XLaunchNewImageA                | (Ghidra); the port exits |
| 0x00030716 | CreateEventA                    | (Ghidra) |
| 0x00030797 | ResetEvent                      | (Ghidra) |
| 0x000307b5 | CreateMutexA                    | (Ghidra) |
| 0x0003080d | ReleaseMutex                    | |
| 0x0003082d | WaitForSingleObjectEx           | not in LIBRARY_CALLS: called (alertable, INFINITE) by the loader thread 0x0004cbe0 |
| 0x00030943 | WaitForSingleObject             | WaitForSingleObjectEx(h, ms, FALSE) |
| 0x0003096f | Sleep                           | SleepEx(ms, FALSE) |
| 0x0003097d | DbgBreakPoint                   | (Ghidra); the port logs and continues |
| 0x00030982 | KeQuerySystemTime               | (Ghidra) kernel import thunk |
| 0x0003099f | wsprintfA                       | cdecl, 1024 characters at most |
| 0x000309d4 | XGetLanguage                    | ExQueryNonVolatileSetting(XC_LANGUAGE) |
| 0x000309f9 | XGetVideoStandard               | XC_FACTORY_AV_REGION >> 8 & 0xff |
| 0x00030a22 | XGetVideoFlags                  | XC_VIDEO_FLAGS >> 16 & 0x5f |
| 0x00030ab6 | XGetGameRegion                  | XC_FACTORY_GAME_REGION |
| 0x00030ade | GlobalMemoryStatus              | |
| 0x00030b2d | GetDiskFreeSpaceExA             | NtQueryVolumeInformationFile; vfs module |
| 0x00030e25 | XMountUtilityDrive              | (Ghidra); vfs module (src/vfs/vfs_files.h) |
| 0x0003101e | lstrcpyA                        | SEH-guarded strcpy |
| 0x0003105c | lstrcpynA                       | copies count - 1 characters, terminates |
| 0x000310c1 | lstrlenA                        | |
| 0x00031227 | OutputDebugStringA              | (Ghidra) |
| 0x0003417a | CloseHandle                     | |
| 0x00034274 | QueueUserAPC                    | (Ghidra) |
| 0x0003439f | CreateThread                    | (Ghidra) |
| 0x0003463a | XLoadSection                    | XeLoadSection by name; the port has no sections |
| 0x00034655 | XFreeSection                    | XeUnloadSection by name |
| libc |||
| 0x001de998 | (cast)                          | _ftol: write (int)x or (long long)x |
| 0x001dee6e | sprintf                         | |
| 0x001def24 | (loop)                          | ??_M eh vector constructor iterator |
| 0x001def84 | (loop)                          | ??_L eh vector destructor iterator |
| 0x001defe6 | atol                            | (Ghidra) |
| 0x001df03e | atoi                            | Ghidra says _atol; same code on ILP32 |
| 0x001df043 | strtok                          | per-thread state in MSVC, global in musl |
| 0x001df0f0 | strstr                          | |
| 0x001df170 | (delete the call)               | _alloca_probe/_chkstk (stack probe, EAX = size) |
| 0x001df180 | swprintf                        | MSVC form without count, 16-bit wchar_t (msvc_wide.c) |
| 0x001df260 | qsort                           | MSVC algorithm in xbox_crt.c |
| 0x001df4f0 | _snwprintf                      | 16-bit wchar_t (msvc_wide.c) |
| 0x001df672 | atexit                          | (Ghidra) |
| 0x001df688 | pow(x, y)                       | _CIpow: x = ST1, y = ST0 |
| 0x001df8a0 | (a / b)                         | __alldiv, 64-bit |
| 0x001df950 | (a * b)                         | __allmul, 64-bit |
| 0x001df984 | sscanf                          | |
| 0x001df9c0 | memmove                         | MSVC memcpy/memmove (overlap safe) |
| 0x001dfded | fread                           | Ghidra: FID_conflict:_fread (calls _filbuf) |
| 0x001dfe85 | fclose                          | (Ghidra) |
| 0x001e0038 | ftell                           | (Ghidra) |
| 0x001e0108 | fseek                           | (Ghidra) |
| 0x001e01ad | fopen                           | _fsopen(name, mode, _SH_DENYNO) |
| 0x001e02d7 | fwrite                          | Ghidra: FID_conflict:_fread (calls _flsbuf) |
| 0x001e0340 | strchr                          | |
| 0x001e03fc | _strlwr                         | ASCII A-Z only, result unused by callers |
| 0x001e041f | vsprintf                        | (buffer, format, va_list) |
| 0x001e0516 | _snprintf                       | (Ghidra) |
| 0x001e056d | _stricmp                        | (Ghidra) |
| 0x001e05b0 | acos(x)                         | _CIacos: x = ST0 (fpatan(sqrt(1-x*x), x)) |
| 0x001e067c | asin(x)                         | _CIasin: x = ST0 (fpatan(x, sqrt(1-x*x))) |
| 0x001e0750 | strncat                         | (Ghidra) |
| 0x001e0873 | fputc                           | Ghidra: FID_conflict:_fputc |
| 0x001e0a24 | _clearfp                        | fnstsw; fnclex; abstract status word |
| 0x001e0a69 | _controlfp                      | (Ghidra) |
| 0x001e0a80 | (a / b)                         | __aulldiv, unsigned 64-bit |
| 0x001e0af2 | fmod(x, y)                      | _CIfmod: x = ST1, y = ST0 (table "fmod" 0x002b74f0) |
| 0x001e0b10 | (a % b)                         | __allrem, 64-bit |
| 0x001e0bc2 | floor                           | rounds with RC = down (control word 0x173f) |
| 0x001e0c90 | fprintf                         | Ghidra: FID_conflict:_fwprintf; calls the narrow _output |
| 0x001e0d41 | malloc                          | (Ghidra) |
| 0x001e0d53 | free                            | small-block heap or RtlFreeHeap |
| 0x001e0dab | realloc                         | |
| 0x001e0ea0 | strrchr                         | (Ghidra) |
| 0x001e0fa8 | atof                            | |
| 0x001e1010 | strncmp                         | (Ghidra) |
| 0x001e105c | sin(x)                          | _CIsin: x = ST0 |
| 0x001e10f8 | atan2(y, x)                     | cdecl, two doubles (table "atan2" 0x002b7c8c) |
| 0x001e1120 | cos(x)                          | _CIcos: x = ST0 |
| 0x001e1276 | _isnan                          | (Ghidra) not in LIBRARY_CALLS: called 3x from 0x00134040 (foxhunt) |
| 0x001e133e | rand                            | (Ghidra) MSVC generator in xbox_crt.c |
| 0x001e87e0 | XPhysicalAlloc                  | MmAllocateContiguousMemoryEx wrapper |
| 0x001e8823 | XPhysicalProtect                | MmSetAddressProtect when size != 0 |
| 0x001e8833 | XPhysicalFree                   | Ghidra: MmFreeContiguousMemory (jump) |
| 0x001e893c | GetTickCount                    | KeTickCount |
| 0x001e8de0 | GetLocalTime                    | system time minus the time zone bias |
| 0x001e8e5f | QueryPerformanceCounter         | |
| 0x001e8e74 | QueryPerformanceFrequency       | |
| 0x001e904c | SetEndOfFile                    | vfs module |
| 0x001e90d0 | SetFilePointer                  | vfs module |
| 0x001e91ca | GetFileInformationByHandle      | vfs module (src/vfs/vfs_files.h) |
| 0x001e9307 | ReadFileEx                      | vfs module; completion by APC |
| 0x001e9358 | WriteFileEx                     | vfs module; completion by APC |
| 0x001e93a9 | GetFileSize                     | vfs module |
| 0x001e93e4 | CreateFileA                     | vfs module |
| 0x001e95db | FindFirstFileA                  | vfs module |
| 0x001e96e8 | FindNextFileA                   | vfs module |
| 0x001e9739 | MoveFileA                       | (Ghidra) vfs module |
| 0x001e97cf | DeleteFileA                     | vfs module |
| 0x001e997f | XFindNextSoundtrack             | no soundtracks in the port |
| 0x001e999e | XFindFirstSoundtrack            | handle signature 0xbe4bea00 on the Xbox |
| 0x001e9a4d | XGetSoundtrackSongInfo          | |
| 0x001e9c76 | XOpenSoundtrackSong             | opens ...\TDATA\FFFE0000\MUSIC\%04x\%08x.WMA |
| 0x001e9d06 | XFindClose                      | closes nickname/soundtrack find handles |
| 0x001ec550 | _strnicmp                       | (Ghidra) |
| 0x001ec5e3 | _itoa                           | (Ghidra) |

The macros below let Ghidra-shaped code call the replacements under their
original names while it is cleaned up. Only functions whose arguments Ghidra
shows as in C are mapped; the x87 intrinsics, the compiler helpers and the
FID_conflict names are not (write the C expression from the table instead).
*/

#ifndef OPENFRENZY_XKERNEL_LIBCALLS_H
#define OPENFRENZY_XKERNEL_LIBCALLS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <wchar.h>
#include <xtl.h>
#include "xkernel.h"

/* xapi */
#define FUN_0003080d ReleaseMutex
#define FUN_0003082d WaitForSingleObjectEx
#define FUN_00030943 WaitForSingleObject
#define FUN_0003096f Sleep
#define FUN_0003099f wsprintfA
#define FUN_000309d4 XGetLanguage
#define FUN_000309f9 XGetVideoStandard
#define FUN_00030a22 XGetVideoFlags
#define FUN_00030ab6 XGetGameRegion
#define FUN_00030ade GlobalMemoryStatus
#define FUN_00030b2d GetDiskFreeSpaceExA
#define FUN_0003101e lstrcpyA
#define FUN_0003105c lstrcpynA
#define FUN_000310c1 lstrlenA
#define FUN_0003417a CloseHandle
#define FUN_0003463a XLoadSection
#define FUN_00034655 XFreeSection

/* libc */
#define FUN_001dee6e sprintf
#define FUN_001df043 strtok
#define FUN_001df0f0 strstr
#define FUN_001df180 swprintf
#define FUN_001df260 qsort
#define FUN_001df4f0 _snwprintf
#define FUN_001df984 sscanf
#define FUN_001df9c0 memmove
#define FUN_001e01ad fopen
#define FUN_001e0340 strchr
#define FUN_001e03fc _strlwr
#define FUN_001e041f vsprintf
#define FUN_001e0516 _snprintf
#define FUN_001e0a24 _clearfp
#define FUN_001e0bc2 floor
#define FUN_001e0d53 free
#define FUN_001e0dab realloc
#define FUN_001e0fa8 atof
#define FUN_001e10f8 atan2
#define FUN_001e87e0 XPhysicalAlloc
#define FUN_001e8823 XPhysicalProtect
#define FUN_001e893c GetTickCount
#define FUN_001e8de0 GetLocalTime
#define FUN_001e8e5f QueryPerformanceCounter
#define FUN_001e8e74 QueryPerformanceFrequency
#define FUN_001e904c SetEndOfFile
#define FUN_001e90d0 SetFilePointer
#define FUN_001e91ca GetFileInformationByHandle /* declared in src/vfs/vfs_files.h */
#define FUN_001e9307 ReadFileEx
#define FUN_001e9358 WriteFileEx
#define FUN_001e93a9 GetFileSize
#define FUN_001e93e4 CreateFileA
#define FUN_001e95db FindFirstFileA
#define FUN_001e96e8 FindNextFileA
#define FUN_001e97cf DeleteFileA
#define FUN_001e997f XFindNextSoundtrack
#define FUN_001e999e XFindFirstSoundtrack
#define FUN_001e9a4d XGetSoundtrackSongInfo
#define FUN_001e9c76 XOpenSoundtrackSong
#define FUN_001e9d06 XFindClose

/* names Ghidra's function ID gave */
#define _atol atol
#define __stricmp _stricmp
#define _strncmp strncmp
#define __strnicmp _strnicmp
#define _fclose fclose
#define _ftell ftell
#define _fseek fseek
#define _strncat strncat
#define __controlfp _controlfp
#define _atexit atexit
#define __itoa _itoa
#define _malloc malloc
#define _strrchr strrchr
#define _rand rand
#define __isnan _isnan

#endif
