/*
BSOUND/xbPlayList.c: C:\Babel\Xbox\Src\BSOUND\xbPlayList.cpp, play lists of the
dashboard's custom soundtracks, each song a WMA stream (xbAudioStreamWMA.c).
0x00055c40, 0x00055ff0 are tagged by the path; 0x00055df0-0x00055fe0 lie between them.
0x00055ae0, 0x00055b90 (bPlayListEnumSong) and 0x00056190 (bPlayListUpdate) are in the
gaps around them and are placed here by their strings.
The function table also lists 0x00055c69: it is not a function but the middle of
0x00055c40 (Ghidra split it at a branch target), so it is not defined here.
*/

#include "engine_misc_sound.h"
#include "engine_misc_extern.h"

/* 0x002db810 (.bss): the active play list, then the two stream slots (xbAudioStream) */
struct BAudioStream *DAT_002db810[3];
#define ACTIVE_PLAYLIST (*(struct BPlayList **)&DAT_002db810[0])

static const char s_file[] = "C:\\Babel\\Xbox\\Src\\BSOUND\\xbPlayList.cpp";

/* what bPlayListEnumSong reports */
struct BPlayListEnumEvent {
	int soundtrack;
	unsigned int song;
	WCHAR *name;
	int isSong;               /* 0: the soundtrack itself */
};
typedef void (*BPlayListEnumCallback)(struct BPlayListEnumEvent *event, void *context);

/* 0x00055ae0  BSOUND (xbPlayList.cpp?) (bPlayListEnumSong)
Reports a soundtrack and then each of its songs. The original takes the soundtrack
index in ECX, the callback in EDI and the XSOUNDTRACK_DATA by value. */
void bsPlayListEnumSongs(int soundtrack, void *callbackPtr, struct EmSoundtrackData data,
	void *context)
{
	BPlayListEnumCallback callback = (BPlayListEnumCallback)callbackPtr;
	struct BPlayListEnumEvent event;
	DWORD songId, songLength;
	WCHAR name[32];
	unsigned int i;

	event.name = data.szName;
	event.soundtrack = soundtrack;
	event.song = 0;
	event.isSong = 0;
	if (callback != NULL)
		callback(&event, context);
	for (i = 0; i < data.uSongCount; i++) {
		if (XGetSoundtrackSongInfo(data.uSoundtrackId, i, &songId, &songLength, name, 0x20) == 0) {
			em_bkPrintf("bPlayListEnumSong:Unable to get song info %d of %d\n", i, data.uSongCount);
		} else {
			event.name = name;
			event.song = i;
			event.isSong = 1;
			if (callback != NULL)
				callback(&event, context);
		}
	}
}

/* 0x00055b90  BSOUND (xbPlayList.cpp?)
Enumerates every soundtrack and its songs; returns the number of soundtracks (0 when
there are none). */
int bsPlayListEnum(void *callback, void *context)
{
	struct EmSoundtrackData data;
	HANDLE find;
	int n;

	n = 0;
	find = XFindFirstSoundtrack(&data);
	if (find == INVALID_HANDLE_VALUE)
		return 0;
	bsPlayListEnumSongs(0, callback, data, context);
	while (XFindNextSoundtrack(find, &data) != 0) {
		n++;
		bsPlayListEnumSongs(n, callback, data, context);
	}
	XFindClose(find);
	return n + 1;
}

/* 0x00055c40  BSOUND/xbPlayList.cpp (bsPlayListSelect)
Makes a play list of the songs of a soundtrack; NULL when it has none. */
struct BPlayList *bsPlayListSelect(int soundtrack)
{
	struct EmSoundtrackData data;
	struct BPlayList *list;
	WCHAR (*names)[32];
	DWORD songId, songLength;
	HANDLE find;
	unsigned int i;
	int count, n;

	find = XFindFirstSoundtrack(&data);
	if (find == INVALID_HANDLE_VALUE) {
		em_bkPrintf("Can't find any soundtracks\n");
		return NULL;
	}
	count = 1;
	if (soundtrack != 0) {
		n = 0;
		do {
			if (XFindNextSoundtrack(find, &data) == 0)
				break;
			count++;
			n++;
		} while (soundtrack != n);
	}
	XFindClose(find);
	if (soundtrack != count - 1) {
		em_bkPrintf("Can't find requested soundtracks %d of %d\n", soundtrack, count);
		return NULL;
	}
	if (data.uSongCount == 0) {
		em_bkPrintf("Track contains no songs\n");
		return NULL;
	}
	list = (struct BPlayList *)em_bkAllocZero(sizeof(struct BPlayList), 0, s_file, 0x81, 0x2001);
	list->soundtrack = soundtrack;
	list->soundtrackId = data.uSoundtrackId;
	list->numSongs = data.uSongCount;
	list->songIds = (unsigned int *)em_bkAlloc(data.uSongCount * 4, s_file, 0x85, 0x2001);
	list->songNames = (WCHAR (*)[32])em_bkAlloc(list->numSongs << 6, s_file, 0x86, 0x2001);
	list->song = 0;
	list->state = 0;
	em_bkWcsCopy(list->name, data.szName);
	names = list->songNames;
	for (i = 0; i < data.uSongCount; i++) {
		if (XGetSoundtrackSongInfo(data.uSoundtrackId, i, &songId, &songLength, names[i], 0x20) == 0) {
			em_bkPrintf("bsPlayListSelect:Unable to get song info %d of %d\n", i, data.uSongCount);
			em_bkFree(list->songIds);
			list->songIds = NULL;
			em_bkFree(list->songNames);
			list->songNames = NULL;
			em_bkFree(list);
			return NULL;
		}
		list->songIds[i] = songId;
	}
	return list;
}

/* 0x00055df0  BSOUND/xbPlayList.cpp */
void bsPlayListStop(struct BPlayList *list)
{
	if (list->stream != NULL) {
		bsStopAudioStream(list->stream);
		em_bkFree(list->stream);
		list->stream = NULL;
	}
	if (list->file != NULL) {
		CloseHandle(list->file);
		list->file = NULL;
	}
	list->state = 0;
}

/* 0x00055e30  BSOUND/xbPlayList.cpp */
void bsPlayListDelete(struct BPlayList *list)
{
	if (list->stream != NULL) {
		bsStopAudioStream(list->stream);
		em_bkFree(list->stream);
		list->stream = NULL;
	}
	if (list->file != NULL) {
		CloseHandle(list->file);
		list->file = NULL;
	}
	list->state = 0;
	if (ACTIVE_PLAYLIST == list)
		ACTIVE_PLAYLIST = NULL;
	if (list->songIds != NULL) {
		em_bkFree(list->songIds);
		list->songIds = NULL;
	}
	if (list->songNames != NULL) {
		em_bkFree(list->songNames);
		list->songNames = NULL;
	}
	em_bkFree(list);
}

/* 0x00055eb0  BSOUND/xbPlayList.cpp
Sets play list flags; 4 pauses the active play list. */
int bsPlayListSetFlags(struct BPlayList *list, unsigned char flags)
{
	if (flags & 2)
		list->flags |= 2;
	if ((flags & 4) && ACTIVE_PLAYLIST->state == 1) {
		list->flags |= 4;
		if (list->stream != NULL && bsPauseAudioStream(list->stream, 0x10) == 0)
			return 0;
		ACTIVE_PLAYLIST->state = 2;
	}
	if (flags & 1)
		list->flags |= 1;
	return 1;
}

/* 0x00055f20  BSOUND/xbPlayList.cpp
Clears play list flags; 4 resumes the active play list. */
int bsPlayListClearFlags(struct BPlayList *list, unsigned char flags)
{
	if (flags & 2)
		list->flags &= ~2u;
	if (flags & 4) {
		list->flags &= ~4u;
		if (list->stream == NULL) {
			ACTIVE_PLAYLIST->state = 0;
		} else {
			if (bsResumeAudioStream(list->stream, 0x10) == 0)
				return 0;
			ACTIVE_PLAYLIST->state = 1;
		}
	}
	if (flags & 1)
		list->flags &= ~1u;
	return 1;
}

/* 0x00055f90  BSOUND/xbPlayList.cpp
The name of a song (-1: the current one). */
WCHAR *bsPlayListGetSongName(struct BPlayList *list, int song)
{
	if (song == -1)
		return list->songNames[list->song];
	if (song > -1 && song < (int)list->numSongs)
		return list->songNames[song];
	return NULL;
}

/* 0x00055fd0  BSOUND/xbPlayList.cpp */
unsigned int bsPlayListGetNumSongs(struct BPlayList *list)
{
	return list->numSongs;
}

/* 0x00055fe0  BSOUND/xbPlayList.cpp */
WCHAR *bsPlayListGetName(struct BPlayList *list)
{
	return list->name;
}

/* 0x00055ff0  BSOUND/xbPlayList.cpp
Plays a song of the list (-1: the current one) as a WMA stream. */
int bsPlayListPlay(struct BPlayList *list, int song, unsigned int flags, int volume, int pan, int frequency)
{
	struct BAudioStream *stream;
	unsigned int size;

	if (list->stream != NULL) {
		bsStopAudioStream(list->stream);
		em_bkFree(list->stream);
		list->stream = NULL;
	}
	if (list->file != NULL) {
		CloseHandle(list->file);
		list->file = NULL;
	}
	list->state = 0;
	if (ACTIVE_PLAYLIST != NULL && ACTIVE_PLAYLIST->state != 0) {
		em_bkPrintf("Another play list is active and is not idle");
		return 0;
	}
	if (song != -1)
		list->song = song;
	list->file = XOpenSoundtrackSong(list->songIds[list->song], 1);
	if (list->file == INVALID_HANDLE_VALUE) {
		em_bkPrintf("Unable to open song %d: id %d\n", list->song, list->songIds[list->song]);
		return 0;
	}
	stream = (struct BAudioStream *)em_bkAllocZero(sizeof(struct BAudioStream), 0, s_file, 0xd3, 0x2001);
	list->stream = stream;
	stream->kind = 1;
	list->stream->state = 0;
	list->stream->package = NULL;
	size = GetFileSize(list->file, NULL);
	list->stream->numBlocks = size >> 0x10;
	list->stream->formatFlags = 2;
	list->stream->flags = list->stream->formatFlags | 0x2000000;
	list->stream->formatFlags = list->stream->flags;
	list->stream->bits = 0x10;
	list->stream->frequency = 44100;
	list->stream->playFrequency = 44100;
	list->frequency = frequency;
	list->volume = volume;
	list->pan = pan;
	if (bsPlayAudioStream(list->stream, 0, volume, pan, frequency, NULL, list->file) == 0) {
		CloseHandle(list->file);
		list->file = NULL;
		em_bkPrintf("Unable to play song %d\n", list->song);
		return 0;
	}
	bsPlayListSetFlags(list, (unsigned char)flags);
	list->state = 1;
	ACTIVE_PLAYLIST = list;
	return 1;
}

/* 0x00056190  BSOUND (xbPlayList.cpp?) (bPlayListUpdate)
Per frame: when the song has ended, moves on (looping lists) or goes idle. */
void bsPlayListUpdate(void)
{
	struct BPlayList *list;
	int next, last;

	list = ACTIVE_PLAYLIST;
	if (list == NULL || list->state != 1)
		return;
	if (bsGetAudioStreamState(list->stream, NULL, NULL, NULL) != 0)
		return;
	next = 0;
	if (list->flags & 1) {
		last = (int)list->numSongs - 1 <= list->song;
		if (last) {
			list->song = 0;
			em_bkPrintf("bPlayListUpdate: Finished playlist, reset to start\n");
		} else {
			list->song = list->song + 1;
			em_bkPrintf("bPlayListUpdate: Moving onto track %d\n", list->song);
		}
		next = !last;
	}
	if (list->flags == 2)
		next = 1;
	em_bkFree(list->stream);
	list->stream = NULL;
	if (next) {
		bsPlayListPlay(list, list->song, list->flags, list->volume, list->pan, list->frequency);
		return;
	}
	list->state = 0;
}
