/*
BKERNEL_CORE.H

Babel bKernel (C:\Babel\Xbox\Src\bKernel): structures, globals and helpers shared by the
bkernel module. Other modules may include this header to reach bKernel data; only the
bkernel module edits it.

Layouts are the original 32-bit ones (docs/ARCHITECTURE.md section 3). Each global keeps
its original address in a comment.
*/

#ifndef OPENFRENZY_BKERNEL_CORE_H
#define OPENFRENZY_BKERNEL_CORE_H

/* the XDK headers declare __stdcall functions, which this target ignores */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
#include "xtl.h"
#include "xkernel.h"
#if __has_include("vfs_files.h")
#include "vfs_files.h"
#else
/* vfs module (src/vfs/vfs_files.h, not committed yet): Win32 GetFileInformationByHandle,
BY_HANDLE_FILE_INFORMATION (0x34 bytes) */
struct vfs_by_handle_file_information
{
	unsigned long dwFileAttributes;
	unsigned long ftCreationTime[2];
	unsigned long ftLastAccessTime[2];
	unsigned long ftLastWriteTime[2];
	unsigned long dwVolumeSerialNumber;
	unsigned long nFileSizeHigh;
	unsigned long nFileSizeLow;
	unsigned long nNumberOfLinks;
	unsigned long nFileIndexHigh;
	unsigned long nFileIndexLow;
};
int __stdcall GetFileInformationByHandle(void *file, struct vfs_by_handle_file_information *information);
#endif
/* input module (src/input/input_xpp.h) */
DWORD WINAPI XGetDevices(PXPP_DEVICE_TYPE device_type);
#pragma clang diagnostic pop
#include <float.h>
#include <stdarg.h>
#include <stdio.h>

/* BK_ALIAS(name, target, offset): makes name a global symbol at target + offset (Mach-O
assembly from llc; asm_convert.py turns it into ELF). target must be a defined (not common)
symbol. Gives other modules' Ghidra names for words inside bKernel's globals. */
#define BK_ALIAS(name, target, offset) \
	__asm__(".globl _" #name "\n\t.set _" #name ", _" #target " + " #offset "\n")

/* ======================================================================== heap (bkHeap) */

/* the context value that means "no context" (FUN_0004c280) */
#define BK_HEAP_CONTEXT_DEFAULT ((const char *)0xdefa)
#define BK_HEAP_MAGIC 0xdeadbeefu

/* allocation flags (the word at bkHeapBlock.flags): the low byte is a group, an index
into bkHeapGroupNames (0x00285340) */
#define BK_HEAP_FLAG_NEW 0x1000
#define BK_HEAP_FLAG_MALLOC 0x2000
#define BK_HEAP_GROUP_BKERNEL 6

/* the header in front of every heap block (and of every block of the malloc fallback) */
typedef struct bkHeapBlock
{
	int offset;                   /* 0x00 offset from the heap base */
	uint size;                    /* 0x04 total size, header included */
	short used;                   /* 0x08 */
	ushort flags;                 /* 0x0a BK_HEAP_FLAG_*, low byte = group */
	const char *context;          /* 0x0c context name, or BK_HEAP_CONTEXT_DEFAULT */
	struct bkHeapBlock *next;     /* 0x10 free list (address order) or used list */
	struct bkHeapBlock *prev;     /* 0x14 */
	struct bkHeapBlock *nextPhys; /* 0x18 next block in memory; 0 = from malloc */
	struct bkHeapBlock *prevPhys; /* 0x1c */
	uint reserved[3];             /* 0x20 */
	uint magic;                   /* 0x2c BK_HEAP_MAGIC */
} bkHeapBlock;
OF_ASSERT_SIZE(bkHeapBlock, 0x30);


/* ======================================================================== debug log (xbDebug.cpp) */

#define BK_LOG_ALLOCATED 0x1   /* the log record came from the heap */
#define BK_LOG_DEBUGGER 0x2    /* copy each line to OutputDebugString */
#define BK_LOG_TRUNCATE 0x4    /* empty the file when the log is created */
#define BK_LOG_TIMESTAMP 0x8   /* prefix each line with the date and time */

typedef struct bkLogFile
{
	char path[0x100]; /* 0x000 empty: no file */
	uint flags;       /* 0x100 BK_LOG_* */
} bkLogFile;
OF_ASSERT_SIZE(bkLogFile, 0x104);

/* ======================================================================== packages (package.cpp, file.cpp) */

#define BK_PACKAGE_BUILD 0x36       /* the package format version this code reads */
#define BK_PACKAGE_IN_MEMORY 0x80000000u /* bkPackage.id: the whole package is loaded */
#define BK_PACKAGE_STATIC 0x1       /* bkPackage.flags: memory is not owned by the package */

/* one file of a package (0x20 bytes, sorted by crc) */
typedef struct bkPackageEntry
{
	uint offset;     /* 0x00 in package blocks */
	uint crc;        /* 0x04 bkCRCString of the name */
	uint size;       /* 0x08 bytes */
	uint nameOffset; /* 0x0c into the filename table */
	uint tagCount;   /* 0x10 */
	uint tagOffset;  /* 0x14 bytes into the tag table */
	uint reserved[2];
} bkPackageEntry;
OF_ASSERT_SIZE(bkPackageEntry, 0x20);

/* a package (.xbp): the first 0x140 bytes of the file, then patched in memory */
typedef struct bkPackage
{
	uint id;                 /* 0x00 package id (crc), | BK_PACKAGE_IN_MEMORY */
	uint blockSize;          /* 0x04 offsets in the file are in these units */
	uint flags;              /* 0x08 BK_PACKAGE_STATIC */
	int numFiles;            /* 0x0c */
	bkPackageEntry *entries; /* 0x10 in the file: block offset of the index */
	uint *tags;              /* 0x14 in the file: block offset of the tag table */
	FILE *file;              /* 0x18 open file, or the package base when in memory */
	uint numTags;            /* 0x1c */
	uint reserved20[2];      /* 0x20 */
	uint namesOffset;        /* 0x28 block offset of the filename table (0: none) */
	uint namesSize;          /* 0x2c bytes */
	uint reserved30[2];      /* 0x30 */
	uint build;              /* 0x38 BK_PACKAGE_BUILD */
	uint reserved3c;         /* 0x3c */
	char name[0x100];        /* 0x40 */
} bkPackage;
OF_ASSERT_SIZE(bkPackage, 0x140);
OF_ASSERT_OFFSET(bkPackage, build, 0x38);

/* the in-memory copy of a package's filename table (bkLoadFilenameTable) */
typedef struct bkFilenameTable
{
	uint id;                        /* 0x00 package id */
	struct bkFilenameTable *prev;   /* 0x04 */
	struct bkFilenameTable *next;   /* 0x08 */
	int refCount;                   /* 0x0c */
	uint numFiles;                  /* 0x10 */
	char packageName[0x100];        /* 0x14 */
	/* 0x114: numFiles x { crc, nameOffset }, then the names */
	uint files[1];
} bkFilenameTable;
OF_ASSERT_OFFSET(bkFilenameTable, files, 0x114);

/* ======================================================================== resources */

#define BK_RESOURCE_TYPES 10
#define BK_RESOURCE_GROUP_DEFAULT 0xdefa
#define BK_RESOURCE_TEXTURE 0
#define BK_RESOURCE_ACTOR 1
#define BK_RESOURCE_SAMPLE 2
#define BK_RESOURCE_FONT 3
#define BK_RESOURCE_STRINGTABLE 4
#define BK_RESOURCE_SPLASH 5
#define BK_RESOURCE_SET 6
#define BK_RESOURCE_COLLISIONMESH 7
#define BK_RESOURCE_ANIMCOLLISIONMESH 8
#define BK_RESOURCE_AUDIOSTREAM 9

/* the header every resource type starts with */
typedef struct bkResource
{
	uint packageId;              /* 0x00 id of the package it came from (bit 31: in memory) */
	ushort group;                /* 0x04 */
	byte type;                   /* 0x06 BK_RESOURCE_* */
	byte pad07;
	int context;                 /* 0x08 */
	uint crc;                    /* 0x0c crc of the file name */
	struct bkResource *next;     /* 0x10 list of all resources */
	struct bkResource *prev;     /* 0x14 */
	struct bkResource *typeNext; /* 0x18 list of one type */
	struct bkResource *typePrev; /* 0x1c */
} bkResource;
OF_ASSERT_SIZE(bkResource, 0x20);

/* a string table (resource type 4), as loaded from a package and patched by
bkLoadStringTable */
typedef struct bkStringTableEntry
{
	char *text;    /* 0x00 */
	uint value;    /* 0x04 */
	void *extra;   /* 0x08 optional */
	uint reserved; /* 0x0c */
} bkStringTableEntry;

typedef struct bkStringTable
{
	bkResource resource;        /* 0x00 */
	bkStringTableEntry *entries; /* 0x20 */
	int numEntries;             /* 0x24 */
	uint reserved[2];           /* 0x28 */
} bkStringTable;
OF_ASSERT_SIZE(bkStringTable, 0x30);

/* ======================================================================== events (event.cpp) */

#define BK_EVENT_CLIENT_CALLBACK 0
#define BK_EVENT_CLIENT_QUEUE 1
#define BK_EVENT_QUEUE_LIFO 0x1    /* bkEventClient.flags: take the newest entry first */
#define BK_EVENT_QUEUE_OVERWRITE 0x2 /* when full, drop the oldest entry (else the new one is lost) */

typedef void (*bkEventCallback)(const char *eventName, const char *parameters, void *context);

typedef struct bkEventClient
{
	struct bkEventClient *next; /* 0x00 */
	struct bkEventClient *prev; /* 0x04 */
	int kind;                   /* 0x08 BK_EVENT_CLIENT_* */
	struct bkEvent *event;      /* 0x0c */
	/* callback: 0x10 function, 0x14 context; queue: 0x10 buffer, 0x14 count */
	union { bkEventCallback callback; char *queue; };
	union { void *callbackContext; int queued; };
	int queueSize;              /* 0x18 entries of 0x100 bytes */
	uint flags;                 /* 0x1c BK_EVENT_QUEUE_* */
} bkEventClient;
OF_ASSERT_SIZE(bkEventClient, 0x20);

typedef struct bkEvent
{
	char name[0x20];            /* 0x00 */
	uint crc;                   /* 0x20 */
	struct bkEvent *next;       /* 0x24 */
	struct bkEvent *prev;       /* 0x28 */
	bkEventClient clients;      /* 0x2c sentinel (next, prev used) */
	int numQueues;              /* 0x4c */
} bkEvent;
OF_ASSERT_SIZE(bkEvent, 0x50);

/* ======================================================================== background load (xbBkgload.cpp) */

#define BK_BKG_CHANNELS 3     /* DATA, AUDIO1, AUDIO2 */
#define BK_BKG_QUEUE_SIZE 16
#define BK_BKG_FLAG_STATIC 0x100  /* request flags: the buffer is not owned */
#define BK_BKG_FLAG_WRITE 0x8

typedef void (*bkBkgCallback)(int requestId);

/* one queued request (0x2a0 bytes) */
typedef struct bkBkgRequest
{
	char fileName[0x200];   /* 0x000 or "FP = 0x%X" for an open handle */
	char eventName[0x80];   /* 0x200 event raised when the request is done */
	uint flags;             /* 0x280 low byte 1: a package; BK_BKG_FLAG_* */
	void *buffer;           /* 0x284 */
	int id;                 /* 0x288 */
	uint offset;            /* 0x28c */
	uint size;              /* 0x290 */
	HANDLE handle;          /* 0x294 an open file, or 0 */
	int channel;            /* 0x298 */
	uint ioFlags;           /* 0x29c */
} bkBkgRequest;
OF_ASSERT_SIZE(bkBkgRequest, 0x2a0);

/* one I/O channel (0x4c bytes); starts with the OVERLAPPED of its transfer */
typedef struct bkBkgChannel
{
	OVERLAPPED overlapped;     /* 0x00 Offset at 0x08 */
	HANDLE handle;             /* 0x14 */
	uint offset;               /* 0x18 */
	void *buffer;              /* 0x1c */
	uint remaining;            /* 0x20 */
	uint bandwidth;            /* 0x24 max bytes per transfer */
	uint transfer;             /* 0x28 bytes of the current transfer */
	int index;                 /* 0x2c 0 DATA, 1 AUDIO1, 2 AUDIO2 */
	const char *name;          /* 0x30 */
	bkBkgCallback callback;    /* 0x34 */
	int callbackArgument;      /* 0x38 */
	uint transferred;          /* 0x3c */
	uint size;                 /* 0x40 */
	uint ioFlags;              /* 0x44 1: count bytes, 2: close handle, 8: write */
	uint busy;                 /* 0x48 bit 0 */
} bkBkgChannel;
OF_ASSERT_SIZE(bkBkgChannel, 0x4c);

/* ======================================================================== file cache (xbCache.cpp) */

#define BK_CACHE_MAX_FILES 1000
#define BK_CACHE_BLOCK 0x40000

typedef struct bkCacheEntry
{
	FILETIME time; /* 0x00 */
	uint flags;    /* 0x08 */
	uint crc;      /* 0x0c crc of the source name */
	uint group;    /* 0x10 */
	uint pad;      /* 0x14 */
} bkCacheEntry;
OF_ASSERT_SIZE(bkCacheEntry, 0x18);

typedef struct bkCacheRequest
{
	const char *source; /* 0x00 */
	int group;          /* 0x04 */
	int priority;       /* 0x08 */
} bkCacheRequest;
OF_ASSERT_SIZE(bkCacheRequest, 0xc);

/* ======================================================================== input (bInput) */

#define BI_MAX_PADS 4
#define BI_MAX_MEMORY_UNITS 8
#define BI_CHANNEL_INPUTS 6
#define BI_INPUT_MAX 0x7ff8

/* one connected port device (0x24 bytes) */
typedef struct biDevice
{
	int port;          /* 0x00 */
	int connected;     /* 0x04 */
	HANDLE handle;     /* 0x08 */
	XINPUT_STATE state; /* 0x0c */
	ushort pad22;
} biDevice;
OF_ASSERT_SIZE(biDevice, 0x24);

/* one binding of a channel: up to six inputs that must all be held (0x1c bytes) */
typedef struct biBinding
{
	uint inputs[BI_CHANNEL_INPUTS]; /* 0x00 */
	int count;                      /* 0x18 */
} biBinding;

/* a channel (0xf0 bytes) */
typedef struct biChannel
{
	char name[0x10];                       /* 0x00 */
	int type;                              /* 0x10 0 free, 1 digital, 2 analog, 3 signed axis */
	int value;                             /* 0x14 */
	uint flags;                            /* 0x18 1 once, 4 repeat, 8 snap */
	int scale;                             /* 0x1c */
	int state;                             /* 0x20 */
	biBinding bindings[BI_CHANNEL_INPUTS]; /* 0x24 */
	int numBindings;                       /* 0xcc */
	struct biMap *map;                     /* 0xd0 */
	uint padd4;                            /* 0xd4 */
	ulonglong repeatTime;                  /* 0xd8 */
	uint repeatDelay;                      /* 0xe0 */
	uint repeatRate;                       /* 0xe4 */
	int acceleration;                      /* 0xe8 */
	int deceleration;                      /* 0xec */
} biChannel;
OF_ASSERT_SIZE(biChannel, 0xf0);

typedef struct biMap
{
	biChannel *channels; /* 0x00 */
	int maxChannels;     /* 0x04 */
	int numChannels;     /* 0x08 */
	uint flags;          /* 0x0c bit 0: disabled */
	struct biMap *next;  /* 0x10 */
	struct biMap *prev;  /* 0x14 */
} biMap;

/* one rumble effect (0x30 bytes) */
typedef struct biRumbleEffect
{
	int strength;        /* 0x00 0 = slot free */
	int unused04;
	longlong duration;   /* 0x08 ticks */
	longlong period;     /* 0x10 start delay in ticks (0 once started) */
	int attack;          /* 0x18 per mille of the duration */
	int decay;           /* 0x1c */
	int waveform;        /* 0x20 */
	int unused24;
	longlong start;      /* 0x28 */
} biRumbleEffect;
OF_ASSERT_SIZE(biRumbleEffect, 0x30);

typedef struct biRumbleMotor
{
	biRumbleEffect effects[8]; /* 0x000 */
	int count;                 /* 0x180 */
	int unused;
} biRumbleMotor;
OF_ASSERT_SIZE(biRumbleMotor, 0x188);

/* ======================================================================== globals

Every bKernel global keeps its original address as its symbol name (DAT_<addr>), so that
Ghidra-shaped code in other modules that names the global reaches the same object
(uninitialised ones merge as common symbols). The readable names below are macros. */

/* heap */
extern int DAT_00285338;                 /* 0x00285338 fixed block index, -1: off */
#define bkHeapFixedBlockIndex DAT_00285338
extern const char DAT_00285340[13][13];  /* 0x00285340 allocation group names */
#define bkHeapGroupNames DAT_00285340
extern bkHeapBlock *DAT_002cce88;        /* 0x002cce88 first block */
#define bkHeapFirstBlock DAT_002cce88
extern int DAT_002cce8c;                 /* 0x002cce8c heap memory from XPhysicalAlloc */
#define bkHeapDynamic DAT_002cce8c
extern char *DAT_002cce90;               /* 0x002cce90 heap base (16 byte aligned) */
#define bkHeapBase DAT_002cce90
extern char *DAT_002cce94;               /* 0x002cce94 heap memory */
#define bkHeapMemory DAT_002cce94
extern bkHeapBlock DAT_002cce98;         /* 0x002cce98 free list sentinel */
#define bkHeapFreeList DAT_002cce98
extern int DAT_002ccec8;                 /* 0x002ccec8 bytes in use */
#define bkHeapUsed DAT_002ccec8
extern bkHeapBlock DAT_002ccecc;         /* 0x002ccecc used list sentinel */
#define bkHeapUsedList DAT_002ccecc
extern char DAT_002ccf00[2][0x400];      /* 0x002ccf00 two fixed blocks */
#define bkHeapFixedBlocks DAT_002ccf00
extern int DAT_002cd700;                 /* 0x002cd700 peak use */
#define bkHeapPeak DAT_002cd700
extern const char *DAT_002cd708[32];     /* 0x002cd708 context stack */
#define bkHeapContextStack DAT_002cd708
extern int DAT_002cd788;                 /* 0x002cd788 context stack top */
#define bkHeapContextTop DAT_002cd788
extern uint DAT_002cd78c;                /* 0x002cd78c heap size, 0: no heap yet */
#define bkHeapSize DAT_002cd78c
extern HANDLE DAT_002cd794;              /* 0x002cd794 */
#define bkHeapMutex DAT_002cd794

/* debug log */
extern bkLogFile DAT_00285158;           /* 0x00285158 default log */
#define bkDefaultLog DAT_00285158
extern bkLogFile *DAT_0028525c;          /* 0x0028525c current log */
#define bkCurrentLog DAT_0028525c
extern int DAT_002cc4f0;                 /* 0x002cc4f0 */
extern char DAT_002cc4f8[0x800];         /* 0x002cc4f8 print buffer */
#define bkPrintBuffer DAT_002cc4f8
extern int DAT_002cccf8;                 /* 0x002cccf8 delay after each print (ms) */
#define bkPrintDelayMs DAT_002cccf8

/* timer */
extern longlong DAT_002cd7a0;            /* 0x002cd7a0 timestamp ticks per second */
#define bkTimerFrequency DAT_002cd7a0
extern uint DAT_002cd7a4;                /* 0x002cd7a4 its high word (alias) */

/* kernel (xbKernel.cpp) */
extern char DAT_002d0758[0x100];         /* 0x002d0758 root folder of relative names, "d:\\" */
#define bkRootPath DAT_002d0758

/* files and packages */
extern uint DAT_002d0330[256];           /* 0x002d0330 crc table */
extern const char *DAT_002d0730[4];      /* 0x002d0730 search folders */
#define bkSearchPaths DAT_002d0730
extern int DAT_002d0740;                 /* 0x002d0740 language */
#define bkLanguage DAT_002d0740
extern int DAT_002d0744;                 /* 0x002d0744 number of search folders */
#define bkNumSearchPaths DAT_002d0744
extern int DAT_002d0748;                 /* 0x002d0748 0 root first, 1 root last, else no root */
#define bkSearchMode DAT_002d0748
extern char DAT_002d0858[8];             /* 0x002d0858 appended to every file name */
#define bkFileSuffix DAT_002d0858

/* background load */
extern int DAT_002853ec;                 /* 0x002853ec DATA channel bytes per second */
extern int DAT_002853f0;                 /* 0x002853f0 scheduler round robin */
extern ulonglong DAT_002cd7a8;           /* 0x002cd7a8 DATA transfer start time */
extern int DAT_002cd7b0[BK_BKG_CHANNELS]; /* 0x002cd7b0 queued requests per channel */
extern HANDLE DAT_002cd7bc;              /* 0x002cd7bc loader quit event */
extern bkBkgChannel DAT_002cd7c0[BK_BKG_CHANNELS]; /* 0x002cd7c0 */
#define bkBkgChannels DAT_002cd7c0
extern bkBkgRequest DAT_002cd8a8[BK_BKG_QUEUE_SIZE]; /* 0x002cd8a8 */
#define bkBkgRequests DAT_002cd8a8
extern int DAT_002d02a8;                 /* 0x002d02a8 */
#define bkBkgRequestCount DAT_002d02a8
extern uint DAT_002d02ac, DAT_002d02b0;  /* bytes loaded (counted / current request) */
extern int DAT_002d02b4, DAT_002d02b8;   /* loader running, transfer in flight */
extern HANDLE DAT_002d02bc;              /* 0x002d02bc loader thread */
#define bkBkgThread DAT_002d02bc
extern int DAT_002d02c0;
extern HANDLE DAT_002d02c4;              /* 0x002d02c4 queue mutex */
#define bkBkgQueueMutex DAT_002d02c4
extern int DAT_002d02c8, DAT_002d02cc, DAT_002d02d0, DAT_002d02d4;

/* resources */
extern const char *const DAT_002852c0[BK_RESOURCE_TYPES]; /* 0x002852c0 type names */
#define bkResourceTypeNames DAT_002852c0
extern const char *const DAT_00285290[6];  /* 0x00285290 language suffixes */
#define bkLanguageSuffixes DAT_00285290
extern bkResource DAT_002ccd00;          /* 0x002ccd00 list of all resources */
#define bkResourceList DAT_002ccd00
extern bkResource DAT_002ccd20[BK_RESOURCE_TYPES]; /* 0x002ccd20 list of each type */
#define bkResourceTypeLists DAT_002ccd20
extern uint DAT_002cce60[BK_RESOURCE_TYPES]; /* 0x002cce60 package tag of each type */
#define bkResourceTags DAT_002cce60

/* events */
extern bkEvent DAT_002d02d8;             /* 0x002d02d8 list of events */
#define bkEventList DAT_002d02d8
extern HANDLE DAT_002d0328;              /* 0x002d0328 */
#define bkEventMutex DAT_002d0328

/* kernel */
extern DWORD DAT_002d0750;               /* 0x002d0750 launch data type */
extern char DAT_002d08a8[0xc00];         /* 0x002d08a8 launch data */
extern int DAT_002d15ac;                 /* 0x002d15ac argc */
extern char **DAT_002d15b0;              /* 0x002d15b0 argv */

/* file cache */
extern int DAT_002d15b8, DAT_002d15bc;   /* copy state, copying */
extern const char *DAT_002d15c0;         /* source being copied */
extern int DAT_002d15c4, DAT_002d15c8;   /* its group and priority */
extern HANDLE DAT_002d15cc, DAT_002d15d0; /* source and destination handles */
extern int DAT_002d15d4, DAT_002d15d8, DAT_002d15dc; /* left, position, block */
extern void *DAT_002d15e0;               /* copy buffer */
extern bkCacheEntry DAT_002d15e8[BK_CACHE_MAX_FILES]; /* 0x002d15e8 cached files */
#define bkCacheEntries DAT_002d15e8
extern int DAT_002d73a8;                 /* number of cached files */
#define bkCacheCount DAT_002d73a8
extern int DAT_002d73ac;                 /* the cache is on */
#define bkCacheEnabled DAT_002d73ac
extern bkEventClient *DAT_002d73b0, *DAT_002d73b4; /* read / write event queues */
extern bkCacheRequest DAT_002d73b8[BK_CACHE_MAX_FILES]; /* copy requests */
extern int DAT_002da298;                 /* number of copy requests */
extern int DAT_002da2a0;                 /* 0x002da2a0 verbose */
#define bkCacheVerbose DAT_002da2a0
extern char DAT_002da2a4[32];

/* input */
extern int DAT_0028512c, DAT_00285150;   /* first rumble / feedback update */
extern const uint DAT_00285130[BI_MAX_MEMORY_UNITS]; /* port masks */
extern biRumbleMotor DAT_002cb570[BI_MAX_PADS * 2]; /* 0x002cb570 rumble (engine_misc too) */
#define biRumble DAT_002cb570
extern int DAT_002cc1b0;                 /* 0x002cc1b0 pads changed */
extern biMap *DAT_002cc1b4;              /* 0x002cc1b4 exclusive map */
#define biExclusiveMap DAT_002cc1b4
extern biMap *DAT_002cc1b8;              /* 0x002cc1b8 the maps */
#define biMapList DAT_002cc1b8
extern int DAT_002cc1bc;                 /* rumble pause count */
extern ulonglong DAT_002cc1c0, DAT_002cc1c8; /* rumble time, last rumble update */
extern int DAT_002cc1d0[BI_MAX_PADS][2]; /* motor speeds */
extern biDevice DAT_002cc1f0[BI_MAX_PADS]; /* 0x002cc1f0 pads */
#define biPads DAT_002cc1f0
extern int DAT_002cc280;                 /* connected pads */
#define biNumPads DAT_002cc280
extern biDevice DAT_002cc284[BI_MAX_MEMORY_UNITS]; /* memory unit slots */
extern int DAT_002cc3a4;                 /* connected memory units */
extern biDevice *DAT_002cc3a8[BI_MAX_PADS]; /* connected pads in port order */
extern DWORD DAT_002cc3b8;               /* last device check (ticks) */
extern ulonglong DAT_002cc3c0;           /* last feedback update */
extern int DAT_002cc3c8[BI_MAX_PADS];
extern XINPUT_FEEDBACK DAT_002cc3d8[BI_MAX_PADS];

/* maths */
extern uint DAT_00285630;                /* 0x00285630 random seed (rand LCG) */
#define bkRandomSeed DAT_00285630
extern float DAT_002855f0[16];           /* 0x002855f0 identity matrix (defined by bdisplay) */
#define bkIdentityMatrix DAT_002855f0

/* ======================================================================== functions not in
the function table (found through code pointers; listed in the final report) */

/* 0x0004ca60 xbBkgload.cpp: ReadFileEx/WriteFileEx completion routine */
void FUN_0004ca60(DWORD errorCode, DWORD bytesTransferred, LPOVERLAPPED overlapped);
/* 0x0004cd00 is an APC routine (PAPCFUNC); declared in bkernel.decls */
/* 0x0004cbe0 xbBkgload.cpp: the background load thread */
DWORD FUN_0004cbe0(LPVOID parameter);

#endif
