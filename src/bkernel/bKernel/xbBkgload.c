/*
C:\Babel\Xbox\Src\bKernel\xbBkgload.cpp: background loading.

Requests (up to 16) queue on three I/O channels (DATA, AUDIO1, AUDIO2). A loader thread
waits alertably; the main thread queues the scheduler to it as an APC (QueueUserAPC).
The scheduler issues one ReadFileEx/WriteFileEx per busy channel, round robin; each
completion routine runs on the loader thread, throttles the DATA channel to
DAT_002853ec bytes per second, and kicks the scheduler again. When a request is done,
its event is raised with the file name as parameter.

Only 0x0004c9b0 names this file (an assert path). The other functions here share its
globals and lie in the same block of code, so they are placed here too. 0x0004ca60 and
0x0004cbe0 are missing from the function table (reached only through code pointers).
*/

#include "bkernel_core.h"

#include <string.h>

/* ---------- globals */

/* 0x002853ec: DATA channel bandwidth in bytes per second (100 MB/s) */
int DAT_002853ec = 0x06400000;
/* 0x002853f0: the channel the scheduler starts with, round robin */
int DAT_002853f0 = -1;
/* 0x002cd7a8: when the current DATA transfer started */
ulonglong DAT_002cd7a8;
/* 0x002cd7b0: requests queued per channel */
int DAT_002cd7b0[BK_BKG_CHANNELS];
/* 0x002cd7bc: set to stop the loader thread */
HANDLE DAT_002cd7bc;
/* 0x002cd7c0: the channels */
bkBkgChannel DAT_002cd7c0[BK_BKG_CHANNELS];
/* 0x002cd8a8: the request queue, 0x002d02a8: its length */
bkBkgRequest DAT_002cd8a8[BK_BKG_QUEUE_SIZE];
int DAT_002d02a8;
/* 0x002d02ac: bytes loaded on the DATA channel by requests that count them */
uint DAT_002d02ac;
/* 0x002d02b0: bytes of the current DATA request so far */
uint DAT_002d02b0;
/* 0x002d02b4: the loader thread runs */
int DAT_002d02b4;
/* 0x002d02b8: a transfer is in flight */
int DAT_002d02b8;
/* 0x002d02bc: the loader thread */
HANDLE DAT_002d02bc;
/* 0x002d02c0: a second round-robin counter (advanced, not read) */
int DAT_002d02c0;
/* 0x002d02c4: guards the queue */
HANDLE DAT_002d02c4;
/* 0x002d02c8: next request id */
int DAT_002d02c8;
/* 0x002d02cc: pause requested, 0x002d02d0: paused */
int DAT_002d02cc;
int DAT_002d02d0;
/* 0x002d02d4: scheduler kicks minus completions */
int DAT_002d02d4;

/* ---------- functions */

/* 0x0004c7f0 (static; the request came in EDI)
A package loaded in the background: patches its header like bkLoadPackage. */
int bkBkgPackageLoaded(struct bkBkgRequest *request, uint flags)
{
	bkPackage *package = (bkPackage *)request->buffer;
	int index;

	strcpy(package->name, request->fileName);
	if (package->numFiles == 0)
	{
		bkHeapFree(request->buffer);
		bkPrintf("bkLoadPackage: Package is empty! '%s'\n", request->fileName);
		return 0;
	}
	package->id |= BK_PACKAGE_IN_MEMORY;
	if (flags & BK_BKG_FLAG_STATIC)
		package->flags |= BK_PACKAGE_STATIC;
	package->entries = (bkPackageEntry *)((uint)package->entries * package->blockSize + (char *)request->buffer);
	if (package->numTags != 0)
		package->tags = (uint *)((uint)package->tags * package->blockSize + (char *)request->buffer);
	package->file = (FILE *)request->buffer;
	for (index = 0; index < 0x20; index++)
	{
		if (request->fileName[index] == '.')
		{
			request->fileName[index] = '\0';
			break;
		}
	}
	bkPrintf("Background loaded package '%s' OK (%d files, %s buffer)\n", request->fileName,
		package->numFiles, (package->flags & BK_PACKAGE_STATIC) ? "Static" : "Dynamic");
	return 1;
}

/* 0x0004c8b0
bkFinishRequest: a request's transfer is over: patch a package, raise the request's
event, drop it from the queue and start the next request whose channel is free. */
void bkFinishRequest(int id)
{
	bkBkgRequest *request = bkBkgRequests;
	int index;

	for (index = 0; index < bkBkgRequestCount; index++, request++)
	{
		if (request->id == id)
			break;
	}
	if ((request->flags & 0xff) == 1)
		bkBkgPackageLoaded(request, request->flags);
	bkGenerateEvent(request->eventName, request->fileName);
	WaitForSingleObject(bkBkgQueueMutex, INFINITE);
	DAT_002cd7b0[request->channel]--;
	if (index < bkBkgRequestCount - 1)
		memmove(request, request + 1, (bkBkgRequestCount - index - 1) * sizeof(bkBkgRequest));
	bkBkgRequestCount--;
	ReleaseMutex(bkBkgQueueMutex);
	for (index = 0; index < bkBkgRequestCount; index++)
	{
		if ((bkBkgChannels[bkBkgRequests[index].channel].busy & 1) == 0)
		{
			bkScheduleLoad(&bkBkgRequests[index], bkFinishRequest, bkBkgRequests[index].id);
			return;
		}
	}
}

/* 0x0004c9b0  bKernel/xbBkgload.cpp (static in the original: buffer in EAX, size in ESI)
bkEnsureAllocBkg: like bkEnsureAlloc, for background loads. */
void *bkEnsureAllocBkg(void *buffer, int size)
{
	if (buffer == NULL)
	{
		buffer = bkHeapAllocEx(size, "C:\\Babel\\Xbox\\Src\\bKernel\\xbBkgload.cpp", 0x1ba, 0x2006,
			bkGetHeapContext(), 0x80);
		if (buffer == NULL)
			bkPrintf("EnsureAllocBkg: *** Out of memory on Babel heap for file (need %d bytes) ***\n",
				size);
		return buffer;
	}
	if (bkHeapBlockSize(buffer) < size)
		return NULL;
	if (((uint)buffer & 0x7f) == 0)
		return buffer;
	bkPrintf("bEnsureAllocBkg: memory for packages must be 128 byte aligned\n");
	DbgBreakPoint();
	return NULL;
}

/* 0x0004ca20 */
int bkIsChannelBusy(int channel)
{
	return bkBkgChannels[channel].busy & 1;
}

/* 0x0004ca40 */
int bkIsChannelIdle(int channel)
{
	return DAT_002cd7b0[channel] < 1;
}

/* 0x0004ca60 (not in the function table)
The completion routine of a channel's ReadFileEx/WriteFileEx (on the loader thread). */
void FUN_0004ca60(DWORD errorCode, DWORD bytesTransferred, LPOVERLAPPED overlapped)
{
	bkBkgChannel *channel = (bkBkgChannel *)overlapped;
	uint transfer;
	ulonglong ticksPerKb;
	longlong wait;
	longlong milliseconds;

	(void)bytesTransferred;
	if (errorCode != 0)
	{
		bkPrintf("    BkgIOCompletion: IO FAILURE!\n");
		return;
	}
	transfer = channel->transfer;
	channel->remaining -= transfer;
	channel->offset += transfer;
	channel->buffer = (char *)channel->buffer + transfer;
	if (channel->index == 0)
	{
		/* throttle the DATA channel to DAT_002853ec bytes per second */
		if (channel->ioFlags & 1)
			DAT_002d02ac += transfer;
		channel->transferred += channel->transfer;
		DAT_002d02b0 = channel->transferred;
		ticksPerKb = (ulonglong)((bkTimerFrequency / (longlong)DAT_002853ec) * 0x400);
		wait = (longlong)((int)channel->transferred >> 10) * (longlong)ticksPerKb +
			(longlong)DAT_002cd7a8 - (longlong)bkTimerRead();
		milliseconds = (wait * 1000) / bkTimerFrequency;
		if ((int)milliseconds > 0)
			Sleep((int)milliseconds);
		/* the original compares the time since the start with the wait computed above
		(it reuses the registers), not with the target time */
		while ((longlong)(bkTimerRead() - DAT_002cd7a8) < wait)
			;
	}
	if (channel->remaining == 0)
	{
		if (channel->index == 0)
			bkTimerRead();
		if (channel->ioFlags & 2)
		{
			CloseHandle(channel->handle);
			channel->handle = NULL;
		}
		channel->busy &= ~1u;
		channel->callback(channel->callbackArgument);
	}
	DAT_002d02d4++;
	DAT_002d02b8 = 0;
	bkKickScheduler(0);
}

/* 0x0004cbe0 (not in the function table)
The loader thread: waits alertably (running the APCs queued to it) until the quit event. */
DWORD FUN_0004cbe0(LPVOID parameter)
{
	(void)parameter;
	DAT_002d02b4 = 1;
	while (WaitForSingleObjectEx(DAT_002cd7bc, INFINITE, TRUE) != 0)
		;
	DAT_002d02b4 = 0;
	return 0;
}

/* 0x0004cc10
bkInitBackgroundLoad: the channels, the loader thread and the queue. */
int bkInitBackgroundLoad(void)
{
	int index;

	for (index = 0; index < BK_BKG_QUEUE_SIZE; index++)
		bkBkgRequests[index].flags = 0xffffffff;
	DAT_002cd7bc = CreateEventA(NULL, FALSE, FALSE, "BkgLoadQuit");
	memset(bkBkgChannels, 0, sizeof(bkBkgChannels));
	bkBkgChannels[0].index = 0;
	bkBkgChannels[0].bandwidth = 0x40000;
	bkBkgChannels[0].name = "DATA";
	bkBkgChannels[1].index = 1;
	bkBkgChannels[1].bandwidth = 0x20000;
	bkBkgChannels[1].name = "AUDIO1";
	bkBkgChannels[2].index = 2;
	bkBkgChannels[2].bandwidth = 0x20000;
	bkBkgChannels[2].name = "AUDIO2";
	bkBkgThread = CreateThread(NULL, 0, FUN_0004cbe0, NULL, 0, NULL);
	bkBkgQueueMutex = CreateMutexA(NULL, FALSE, "bkgQueueMutex");
	DAT_002cd7b0[0] = 0;
	DAT_002cd7b0[1] = 0;
	DAT_002cd7b0[2] = 0;
	bkBkgRequestCount = 0;
	return 1;
}

/* 0x0004cd00 (an APC routine, run on the loader thread)
bkKickScheduler: starts the next transfer, the channels taken round robin. A channel whose
transfer cannot start is finished at once. */
void bkKickScheduler(ULONG_PTR unused)
{
	bkBkgChannel *channel;
	uint transfer;
	int failed;
	int pass;

	(void)unused;
	DAT_002d02d4--;
	if (DAT_002d02b8 != 0)
		return;
	if (DAT_002d02cc != 0)
	{
		DAT_002d02d0 = 1;
		return;
	}
	DAT_002853f0 = (DAT_002853f0 + 1) % 3;
	DAT_002d02d0 = 0;
	for (pass = 0; pass < 3; pass++)
	{
		channel = &bkBkgChannels[(DAT_002853f0 + pass) % 3];
		if ((channel->busy & 1) == 0)
		{
			DAT_002d02c0 = (DAT_002d02c0 + 1) % 3;
			continue;
		}
		channel->overlapped.Offset = channel->offset;
		transfer = channel->remaining;
		if ((int)transfer > (int)channel->bandwidth)
			transfer = channel->bandwidth;
		channel->transfer = transfer;
		failed = 0;
		if ((channel->ioFlags & 8) == 0)
		{
			if (!ReadFileEx(channel->handle, channel->buffer, transfer, &channel->overlapped,
					FUN_0004ca60))
			{
				bkPrintf("    KickScheduler: *** ReadFileEx failed (was filesize divisible by 2K? is the package granularity 2K?) ***\n");
				failed = 1;
			}
		}
		else
		{
			if (!WriteFileEx(channel->handle, channel->buffer, transfer, &channel->overlapped,
					FUN_0004ca60))
			{
				bkPrintf("    KickScheduler: *** WriteFileEx failed (was filesize divisible by 2K? is the package granularity 2K?) ***\n");
				failed = 1;
			}
		}
		DAT_002d02c0 = (DAT_002d02c0 + 1) % 3;
		if (!failed)
		{
			DAT_002d02b8 = 1;
			return;
		}
		if (channel->ioFlags & 2)
		{
			CloseHandle(channel->handle);
			channel->handle = NULL;
		}
		channel->busy &= ~1u;
		channel->callback(channel->callbackArgument);
	}
	DAT_002d02b8 = 0;
}

/* 0x0004ce60
bkScheduleLoad: starts a request on its channel (when the channel is free): opens the
file unless the request has a handle and queues the scheduler to the loader thread. */
int bkScheduleLoad(struct bkBkgRequest *request, void (*callback)(int), int argument)
{
	int index = request->channel;
	bkBkgChannel *channel = &bkBkgChannels[index];
	HANDLE handle;

	if (channel->busy & 1)
		return 0;
	if (index == 0 && request->handle == NULL)
		bkCacheLoadingFileNotification(request->fileName);
	channel->ioFlags = request->ioFlags;
	channel->transferred = 0;
	channel->offset = request->offset;
	channel->remaining = request->size;
	channel->buffer = request->buffer;
	channel->overlapped.Offset = request->offset;
	channel->callback = callback;
	channel->callbackArgument = argument;
	channel->size = request->size;
	DAT_002d02b0 = 0;
	if (request->handle == NULL)
	{
		handle = CreateFileA(request->fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL, NULL);
		channel->handle = handle;
		if (handle == INVALID_HANDLE_VALUE)
		{
			bkPrintf("bScheduleLoad: could not open file '%s' for reading\n", request->fileName);
			bkFinishRequest(request->id);
			return 0;
		}
		channel->ioFlags |= 2;
	}
	else
	{
		channel->handle = request->handle;
	}
	channel->busy = 1;
	if (request->channel == 0)
		DAT_002cd7a8 = bkTimerRead();
	request->ioFlags |= 4;
	QueueUserAPC(bkKickScheduler, bkBkgThread, 0);
	return 1;
}

/* 0x0004cf90
bkPauseBackgroundLoad: pause 1 asks the loader to stop after the current transfer and
returns whether it has stopped; pause 0 resumes and returns whether it runs. */
int bkPauseBackgroundLoad(int pause)
{
	if (pause != 0)
	{
		DAT_002d02cc = 1;
		QueueUserAPC(bkKickScheduler, bkBkgThread, 0);
		return DAT_002d02d0;
	}
	DAT_002d02cc = 0;
	QueueUserAPC(bkKickScheduler, bkBkgThread, 0);
	return DAT_002d02d0 == 0;
}

/* 0x0004cff0
bQueueBackgroundLoad: queues a read of size bytes at offset of a file (by name, or by an
open handle) into buffer; eventName is raised when it is done. */
int bQueueBackgroundLoad(int channel, void *buffer, HANDLE handle, const char *fileName,
	uint offset, uint size, uint ioFlags, const char *eventName, uint flags)
{
	bkBkgRequest *request;

	if (bkBkgRequestCount == BK_BKG_QUEUE_SIZE)
	{
		bkPrintf("bQueueBackgroundLoad: background load request queue is FULL\n");
		return 0;
	}
	WaitForSingleObject(bkBkgQueueMutex, INFINITE);
	request = &bkBkgRequests[bkBkgRequestCount];
	strcpy(request->eventName, eventName);
	if (handle != NULL && fileName == NULL)
		sprintf(request->fileName, "FP = 0x%X", (uint)handle);
	else
		strcpy(request->fileName, fileName);
	request->buffer = buffer;
	request->id = DAT_002d02c8++;
	request->flags = flags;
	request->size = size;
	request->offset = offset;
	request->ioFlags = ioFlags;
	bkBkgRequestCount++;
	request->handle = handle;
	request->channel = channel;
	DAT_002cd7b0[channel]++;
	ReleaseMutex(bkBkgQueueMutex);
	if (DAT_002cd7b0[request->channel] > 1 || (bkBkgChannels[request->channel].busy & 1) != 0)
		return 1;
	return bkScheduleLoad(request, bkFinishRequest, request->id) != 0;
}

/* 0x0004d130
bQueueBackgroundWrite: queues a write of size bytes at offset of an open file. */
int bQueueBackgroundWrite(int channel, void *buffer, HANDLE handle, uint offset, uint size,
	uint ioFlags, const char *eventName, uint flags)
{
	bkBkgRequest *request;

	if (bkBkgRequestCount == BK_BKG_QUEUE_SIZE)
	{
		bkPrintf("bQueueBackgroundWrite: background load request queue is FULL\n");
		return 0;
	}
	WaitForSingleObject(bkBkgQueueMutex, INFINITE);
	request = &bkBkgRequests[bkBkgRequestCount];
	strcpy(request->eventName, eventName);
	sprintf(request->fileName, "FP = 0x%X", (uint)handle);
	request->buffer = buffer;
	request->id = DAT_002d02c8++;
	request->flags = flags;
	request->offset = offset;
	request->size = size;
	bkBkgRequestCount++;
	request->ioFlags = ioFlags | BK_BKG_FLAG_WRITE;
	request->handle = handle;
	request->channel = channel;
	DAT_002cd7b0[channel]++;
	ReleaseMutex(bkBkgQueueMutex);
	if (DAT_002cd7b0[request->channel] > 1 || (bkBkgChannels[request->channel].busy & 1) != 0)
		return 1;
	return bkScheduleLoad(request, bkFinishRequest, request->id) != 0;
}

/* 0x0004d260 (static in the original: the path in EAX, the size result in EBX)
bkLoadSingleFileBackground: queues the load of a whole file on the DATA channel. */
void *bkLoadSingleFileBackground(const char *path, uint *size, void *buffer,
	const char *eventName, uint flags)
{
	FILE *file;
	char found[256];
	int length;

	if (bkFindFile(path, &file, found, sizeof(found)) == 0)
	{
		bkPrintf("LoadSingleFile: *** Could not load file '%s' ***\n", path);
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	length = (int)ftell(file);
	fclose(file);
	buffer = bkEnsureAllocBkg(buffer, length);
	if (buffer == NULL)
		return NULL;
	if (bQueueBackgroundLoad(0, buffer, NULL, found, 0, length, 1, eventName, flags) == 0)
		return NULL;
	if (size != NULL)
		*size = length;
	return buffer;
}

/* 0x0004d320
bkLoadPackageBackground: queues the load of package name (name.xbp) on the DATA channel,
from disc or from inside package. eventName is raised when it is loaded. */
void *bkLoadPackageBackground(struct bkPackage *package, const char *name, const char *eventName,
	uint *size, void *buffer)
{
	char path[0x100];
	char fileName[0x100];
	uint flags = buffer != NULL ? BK_BKG_FLAG_STATIC | 1 : 1;
	bkPackageEntry *entry;

	sprintf(fileName, "%s.xbp", name);
	if (package == NULL)
		return bkLoadSingleFileBackground(fileName, size, buffer, eventName, flags);
	entry = bkFindPackageEntryByName(package, fileName);
	if (entry == NULL)
		return NULL;
	if (size != NULL)
		*size = entry->size;
	if ((int)package->id < 0)
	{
		bkGenerateEvent(eventName, name);
		/* the original scales the offset by 0x140 (it adds to a bkPackage pointer) */
		return (bkPackage *)package->file + package->blockSize * entry->offset;
	}
	buffer = bkEnsureAllocBkg(buffer, entry->size);
	if (buffer == NULL)
		return NULL;
	strcpy(path, bkRootPath);
	strcat(path, package->name);
	strcat(path, ".xbp");
	if (bQueueBackgroundLoad(0, buffer, NULL, path, package->blockSize * entry->offset,
			entry->size, 1, eventName, flags) == 0)
		return NULL;
	return buffer;
}
