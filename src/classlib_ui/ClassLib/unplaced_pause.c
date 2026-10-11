/*
ClassLib/unplaced_pause.c  (classlib_ui)

The in-game pause menu (0x00074e30-0x00078870): pause input map, the menu pages
(main, options, quit confirmation, soundtrack), the window frame, the "controller
unplugged" message and the per-frame update. The original source file is not known
(gap between COverlay.cpp and Hud.cpp); docs/ARCHITECTURE.md asks for unplaced.c, this
file is its pause-menu part, kept separate for size.

The pause object (self) is a C++ object; offsets used here:
  0x00  page to show after the window resize          0x04, 0x08  window size target
  0x18  title text index           0x1c  input map of the game (disabled while paused)
  0x20, 0x24  stick y, x actions   0x28, 0x2c, 0x30  select, back, cheat actions
  0x34  pause input map            0x38  "Pause" action per player (4)
  0x48  "Any" (menu) action per player (4)
  0x58  time (glow phase)          0x5c  paused flag (1: menu open)
  0x60, 0x64  open time and frames 0x7c  closing countdown (resume)
  0x68  pausing player             0x6c  selected item
  0x70  default item               0x74  page (0 main, 1 options, 2-4 confirm, 5 title;
        6 fade out, 8 resize, 7 fade in)
  0x78  item count                 0x80  font
  0x84  root overlay (window)      0x88-0xa4  window frame pieces (corners, edges)
  0xa8-0xb8  frame overlays hidden on page 0   0xbc-0xdc  text lines (title, items)
  0xd8, 0xdc  volume bars
*/

#include "classlib_ui.h"

#define OV_F(o, off) (*(float *)((o) + (off)))
#define OV_I(o, off) (*(int *)((o) + (off)))
#define OV_U(o, off) (*(unsigned int *)((o) + (off)))
#define OV_S(o, off) (*(short *)((o) + (off)))

/* ---------- globals (classlib_ui_data.h) */

#define clu_time_scale CLU_VAR(float, 0x0028fda8)
/* 0x002906a0  debug message texts (UTF-16 pointers, zero in the XBE) */
#define clu_debug_texts CLU_ARR(unsigned short *, 0x002906a0)
/* 0x00290960  soundtrack menu entries enabled (0: greyed) */
#define clu_soundtrack_menu CLU_VAR(int, 0x00290960)
/* 0x00290964  the controller that paused (-1: any) */
#define clu_pause_controller CLU_VAR(int, 0x00290964)
/* 0x00290994  x of the title on the main page (264.0) */
#define clu_title_x CLU_VAR(float, 0x00290994)
/* 0x0029099c / 0x002909a0  colours r, g, b, a of the selected / other items */
#define clu_item_selected_rgba CLU_ARR(unsigned char, 0x0029099c)
#define clu_item_rgba CLU_ARR(unsigned char, 0x002909a0)
/* 0x002909c4 / 0x002909c8  y and x of the debug message lines (200, -200) */
#define clu_debug_y CLU_VAR(float, 0x002909c4)
#define clu_debug_x CLU_VAR(float, 0x002909c8)
/* 0x002909d8 / 0x002909dc  glow amount and speed of the selected item */
#define clu_item_glow CLU_VAR(float, 0x002909d8)
#define clu_item_glow_speed CLU_VAR(float, 0x002909dc)
/* 0x002909e0  message buffer (UTF-16), initially the English unplugged message */
#define clu_message CLU_ARR(unsigned short, 0x002909e0)
/* 0x002ec900  custom soundtrack stream that plays now */
#define clu_music_custom CLU_VAR(int, 0x002ec900)
/* 0x002ecb68  soundtrack name buffer (128 UTF-16 characters) */
#define clu_soundtrack_name CLU_ARR(unsigned short, 0x002ecb68)
/* 0x002ecc68  music track chosen in the menu */
#define clu_menu_track CLU_VAR(int, 0x002ecc68)
/* 0x002ecc74  the language text table object (texts at +0x20, 16 bytes each) */
#define clu_language CLU_VAR(int, 0x002ecc74)
/* 0x002ecc80  song name buffer (128 UTF-16 characters) */
#define clu_song_name CLU_ARR(unsigned short, 0x002ecc80)
/* 0x002ecdec  frames since a controller was unplugged (0: none) */
#define clu_unplugged_time CLU_VAR(int, 0x002ecdec)
/* 0x002ecf00  phase of the unplugged message pulse */
#define clu_unplugged_phase CLU_VAR(float, 0x002ecf00)
/* 0x002ecfac  HUD object */
#define clu_hud CLU_VAR(int, 0x002ecfac)
/* 0x002eca3c  root overlay list */
#define clu_overlay_root CLU_VAR(int, 0x002eca3c)
/* 0x0028ffb8  music track that plays now */
#define clu_music_track CLU_VAR(int, 0x0028ffb8)

/* the language text at byte offset `off` of the text table (UTF-16) */
#define CL_TEXT(off) (*(unsigned short **)(*(int *)(clu_language + 0x20) + (off)))

/* wcscpy / wide sprintf of the engine (bkernel) */
#define CL_WStrCpy(dst, src) FUN_0004b130((short *)(dst), (short *)(src))

/* the text drawer (bdisplay): font, text, length, box, r, g, b, a, callback, context */
#define CL_DrawText(font, text, len, box, r, g, b, a) \
	FUN_00043400((struct bdFont *)(font), (const unsigned short *)(text), (int)(len), (struct bdPrintFormat *)(box), (r), (g), (b), (a), 0, 0)
#define CL_SetRenderState3(a, b, c) ((void (*)(int, int, int))FUN_00046cb0)((a), (b), (c))

/* text layout of the text drawer */
struct CLTextBox {
	float x, y;
	float width, height;
	int halign, valign;
	undefined4 box_w, box_h;
};

/* globals of other modules (tentative definitions in classlib_ui_extern.c) */
extern int DAT_002bae7c;           /* core: soundtrack shuffle on */
extern int DAT_002bae80;           /* core: custom soundtrack index (0: disc) */
extern int DAT_002bae84;           /* core: custom soundtrack in use */
extern int DAT_002bae94;           /* core: player count */
extern int DAT_002baea8;           /* core: quit to the board allowed */
extern int DAT_002baed4[40];       /* core: players (10 ints each): character at +0 */
extern int DAT_002baed8[40];       /* core: players: plugged in at +0 */
extern int DAT_002baee0[40];       /* core: players: pad port bits at +0 */
extern int DAT_002ba8b8[17];       /* core: debug message ids (16; the update clears 17 words) */
extern int DAT_002ba900;           /* core: debug message count */
extern float DAT_002ba904;         /* core: debug message timer */
extern unsigned short DAT_002ba7a8[0x88]; /* core: debug message text */
extern int DAT_002ba99c;           /* core: the controller that paused */
extern int DAT_00284408;           /* core: two or more controllers in use */
extern int DAT_002db810;           /* engine: custom soundtrack paused with the game */

/* ---------- pause input */

/* 0x00074e30  ClassLib (pause menu; thiscall)
Creates the pause input map: per player a "Pause" action (Start, 0xc1) and an "Any"
action on the face buttons. */
void CLPause_CreateInput(int self)
{
	int player;
	unsigned int pad;

	OV_I(self, 0x34) = (int)FUN_000488c0(8);
	for (player = 0; player < DAT_002bae94; player++) {
		pad = (unsigned int)DAT_002baee0[player * 10];
		OV_I(self, 0x38 + player * 4) = (int)FUN_00048a10((struct biMap *)OV_I(self, 0x34), "Pause", 1, 1, 0, 0, 0);
		OV_I(self, 0x48 + player * 4) = (int)FUN_00048a10((struct biMap *)OV_I(self, 0x34), "Any", 3, 0, 0, 0, 0);
		FUN_00048b20((struct biChannel *)OV_I(self, 0x38 + player * 4), pad | 0xc1, 0);
		FUN_00048b20((struct biChannel *)OV_I(self, 0x48 + player * 4), pad | 0xc5, pad | 0xc6);
		FUN_00048b20((struct biChannel *)OV_I(self, 0x48 + player * 4), pad | 0xc7, pad | 0xc8);
		FUN_00048b20((struct biChannel *)OV_I(self, 0x48 + player * 4), pad | 0xc9, pad | 0xca);
		FUN_00048b20((struct biChannel *)OV_I(self, 0x48 + player * 4), pad | 0xc3, pad | 0xc4);
		FUN_00048b20((struct biChannel *)OV_I(self, 0x48 + player * 4), pad | 0xcb, pad | 0xcc);
		FUN_00048b20((struct biChannel *)OV_I(self, 0x48 + player * 4), pad | 0xc2, 0);
	}
}

/* ---------- soundtrack names */

/* 0x00074f60  ClassLib (pause menu)
Copies a UTF-16 text and doubles the '@' and '\' (text escape characters); at most
about 126 characters. */
void CLPause_EscapeText(unsigned short *text, unsigned short *out)
{
	/* "@@" converts to 3 UTF-16 units with its terminator. The original had at[2] directly
	   below backslash[2] (the extra unit landed in backslash, written next), but here it
	   overwrote a caller's saved register (CGameMode_MainOptions_RefreshTexts lost its
	   format pointer and overflowed its text buffer: the Options menu fault). */
	unsigned short at[3];
	unsigned short backslash[2];
	unsigned short c;
	int written;
	int length;
	int i;

	written = 0;
	FUN_0004a330((ushort *)at, (byte *)"@@");
	FUN_0004a330((ushort *)backslash, (byte *)"\\");
	length = FUN_0004b110((short *)text);
	for (i = 0; i < length; i++) {
		if (0x7d < written)
			break;
		c = text[i];
		if (c == at[0]) {
			out[0] = at[0];
			out[1] = at[0];
			out += 2;
			written += 2;
		} else if (c == backslash[0]) {
			out[0] = backslash[0];
			out[1] = backslash[0];
			out += 2;
			written += 2;
		} else {
			*out = c;
			out++;
			written++;
		}
	}
	*out = 0;
}

/* 0x00075000  ClassLib (pause menu)
Name of a custom soundtrack (0: the game's own music), cut to `max` characters with
"...". Returns a static buffer. */
unsigned short *CLPause_SoundtrackName(int soundtrack, int max)
{
	unsigned short *name;

	if (soundtrack == 0)
		name = CL_TEXT(0x17c0);
	else
		name = (unsigned short *)FUN_00055fe0((struct BPlayList *)soundtrack);
	CLPause_EscapeText(name, clu_soundtrack_name);
	if (max < FUN_0004b110((short *)clu_soundtrack_name)) {
		clu_soundtrack_name[max] = 0;
		clu_soundtrack_name[max - 1] = '.';
		clu_soundtrack_name[max - 2] = '.';
		clu_soundtrack_name[max - 3] = '.';
	}
	return clu_soundtrack_name;
}

/* 0x00075070  ClassLib (pause menu)
Name of song `song` of a custom soundtrack (0: of the game's tracks), cut to `max`
characters with "...". Returns a static buffer. */
unsigned short *CLPause_SongName(int soundtrack, int song, int max)
{
	unsigned short *name;

	if (soundtrack == 0)
		name = *(unsigned short **)((song + 0x127) * 0x10 + *(int *)(clu_language + 0x20));
	else
		name = (unsigned short *)FUN_00055f90((struct BPlayList *)soundtrack, song);
	CLPause_EscapeText(name, clu_song_name);
	if (max < FUN_0004b110((short *)clu_song_name)) {
		clu_song_name[max] = 0;
		clu_song_name[max - 1] = '.';
		clu_song_name[max - 2] = '.';
		clu_song_name[max - 3] = '.';
	}
	return clu_song_name;
}

/* 0x000750f0  ClassLib (pause menu)
Steps to the next (step 1) or previous (-1) soundtrack that has songs (index 0 is
the game's music) and starts its music. */
int CLPause_NextSoundtrack(int step)
{
	int old_index;
	int soundtrack;
	int count;
	int result;

	old_index = DAT_002bae80;
	soundtrack = 0;
	result = 0;
	if (clu_unplugged_time != 0 && clu_hud != 0)
		OV_I(clu_hud, 0x88) = 1;
	CLSamples_PlayGeneric(7, 0x7f, 0, 0, NULL);
	count = FUN_00055b90(0, 0) + 1;
	if (DAT_002bae84 != 0) {
		if (clu_music_custom == DAT_002bae84)
			clu_music_custom = 0;
		FUN_00055e30((struct BPlayList *)DAT_002bae84);
		DAT_002bae84 = 0;
	}
	for (;;) {
		DAT_002bae80 = (count + step + DAT_002bae80) % count;
		if (DAT_002bae80 == 0)
			goto none;
		if (soundtrack != 0) {
			FUN_00055e30((struct BPlayList *)soundtrack);
			if (clu_music_custom == soundtrack)
				clu_music_custom = 0;
		}
		soundtrack = (int)FUN_00055c40(DAT_002bae80 + -1);
		if (DAT_002bae80 == 0)
			goto none;
		if (soundtrack == 0)
			continue;
		if (FUN_00055fd0((struct BPlayList *)soundtrack) != 0)
			break;
	}
	if (DAT_002bae80 != 0)
		goto done;
none:
	soundtrack = 0;
done:
	DAT_002bae84 = soundtrack;
	if (old_index != DAT_002bae80) {
		clu_menu_track = ((soundtrack != 0) - 1) & 6;
		CLMusic_Switch(clu_menu_track, 0xff, -0xff, 0x32, 0, 0);
	}
	return result;
}

/* 0x00075210  ClassLib (pause menu; thiscall)
Writes "Soundtrack: <name>" and "Song: <name>" into the menu lines 4 and 5. */
void CLPause_ShowSoundtrack(int self)
{
	int length;

	length = FUN_0004b110((short *)CL_TEXT(0x17d0));
	CL_WSprintf((unsigned short *)(OV_I(self, 200) + 0xa4), L"%S: %S", CL_TEXT(0x17d0),
		    CLPause_SoundtrackName(DAT_002bae84, 0x19 - length));
	length = FUN_0004b110((short *)CL_TEXT(0x1270));
	CL_WSprintf((unsigned short *)(OV_I(self, 0xcc) + 0xa4), L"%S: %S", CL_TEXT(0x1270),
		    CLPause_SongName(DAT_002bae84, clu_menu_track, 0x19 - length));
}

/* 0x000752c0  ClassLib (pause menu)
Resumes the music streams (clears their pause flag 0x10), a custom soundtrack and the
sound effects. */
void CLPause_ResumeSound(void)
{
	int root;
	int i;

	root = (int)FUN_0002b5b0((struct CGameModeStack *)DAT_002ba750, (const char *)"Root");
	for (i = 0; i < 2; i++) {
		if (*(int *)(root + 0x54 + i * 4) != 0)
			FUN_000565e0((struct BAudioStream *)(*(int *)(root + 0x54 + i * 4)), 0x10);
	}
	if (DAT_002bae84 != 0 && DAT_002db810 != 0)
		FUN_00055f20((struct BPlayList *)DAT_002bae84, 4);
	FUN_00054800(0);
}

/* 0x00075320  ClassLib (pause menu)
Resumes what CLPause_ResumeSound paused. */
void CLPause_PauseSound(void)
{
	int root;
	int i;

	root = (int)FUN_0002b5b0((struct CGameModeStack *)DAT_002ba750, (const char *)"Root");
	for (i = 0; i < 2; i++) {
		if (*(int *)(root + 0x54 + i * 4) != 0)
			FUN_00056590((struct BAudioStream *)(*(int *)(root + 0x54 + i * 4)), 0x10);
	}
	if (DAT_002bae84 != 0 && DAT_002db810 != 0)
		FUN_00055eb0((struct BPlayList *)DAT_002bae84, 4);
	FUN_00054800(1);
}

/* 0x00075380  ClassLib (pause menu; thiscall)
Steps the song of the soundtrack menu by `step` and shows its name. */
void CLPause_NextSong(int self, int step)
{
	int count;
	int length;

	if (clu_hud != 0)
		OV_I(clu_hud, 0x88) = 1;
	if (DAT_002bae84 == 0) {
		step = clu_menu_track + step;
		if (step == 0x1c)
			step = 6;
		else if (step == 5)
			step = 0x1b;
		CLMusic_Switch(step, 0xff, -0x14, 0x14, 0, 0);
	} else {
		count = FUN_00055fd0((struct BPlayList *)DAT_002bae84);
		step = (clu_menu_track + count + step) % count;
		CLMusic_Switch(step, 0xff, -0x14, 0x14, 0, 0);
	}
	clu_menu_track = clu_music_track;
	length = FUN_0004b110((short *)CL_TEXT(0x1270));
	CL_WSprintf((unsigned short *)(OV_I(self, 0xcc) + 0xa4), L"%S: %S", CL_TEXT(0x1270),
		    CLPause_SongName(DAT_002bae84, step, 0x19 - length));
}

/* 0x00075480  ClassLib (pause menu; thiscall)
Ends the pause: resumes the sound (unless `keep_sound`), deletes the window, gives
the game its input back and shows the overlays hidden for the pause. */
void CLPause_End(int self, int keep_sound)
{
	int node;

	CLU_DebugPrintf("End pause\n");
	FUN_0002b9d0();
	if (keep_sound == 0)
		CLPause_ResumeSound();
	COverlay_Delete(OV_I(self, 0x84));
	OV_I(self, 0x84) = 0;
	FUN_00048970((struct biMap *)OV_U(self, 0x1c));
	OV_I(self, 0x5c) = 0;
	for (node = (int)CLList_Next((unsigned int *)clu_overlay_root, NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)clu_overlay_root, (unsigned int *)node)) {
		if ((*(unsigned char *)(node + 0x2e4) & 4) != 0)
			*(short *)(node + 0x44) = *(short *)(node + 0x46);
	}
}

/* ---------- menu pages */

/* 0x00075510  ClassLib (pause menu; thiscall)
Fills the menu lines of a page: 0 main, 1 options (soundtrack), 2 quit, 3 quit
"<player>?", 4 restart, 5 the title only. */
void CLPause_SetPage(int self, int page)
{
	int count;
	unsigned short *shuffle;

	OV_I(self, 0x6c) = OV_I(self, 0x70);
	switch (page) {
	case 0:
		OV_F(OV_I(self, 0xbc), 0x310) = clu_title_x;
		OV_S(OV_I(self, 0xa8), 0x54) = 1;
		OV_S(OV_I(self, 0xac), 0x54) = 1;
		OV_S(OV_I(self, 0xb0), 0x54) = 1;
		OV_S(OV_I(self, 0xb4), 0x54) = 1;
		OV_S(OV_I(self, 0xb8), 0x54) = 1;
		if (DAT_002baea8 == 0) {
			OV_I(self, 0x78) = 5;
			OV_S(OV_I(self, 0xd4), 0x54) = 1;
		} else {
			OV_I(self, 0x78) = 6;
			OV_S(OV_I(self, 0xd4), 0x54) = 0;
		}
		CL_WStrCpy(OV_I(self, 0xbc) + 0xa4, CL_TEXT(0x60));
		CL_WStrCpy(OV_I(self, 0xc0) + 0xa4, CL_TEXT(0xa0));
		CL_WStrCpy(OV_I(self, 0xc4) + 0xa4, CL_TEXT(0x70));
		CL_WStrCpy(OV_I(self, 200) + 0xa4, CL_TEXT(0x1a40));
		swprintf((wchar_t *)(OV_I(self, 0xcc) + 0xa4), L"%s %s", CL_TEXT(0xc0),
			 CL_TEXT((DAT_002baed4[OV_I(self, 0x68) * 10] + 0x149) * 0x10));
		CL_WStrCpy(OV_I(self, 0xd0) + 0xa4, CL_TEXT(0x30));
		CL_WStrCpy(OV_I(self, 0xd4) + 0xa4, CL_TEXT(0x1280));
		OV_F(OV_I(self, 0xc4), 0x304) = 80.0f;
		OV_F(OV_I(self, 200), 0x304) = 60.0f;
		OV_S(OV_I(self, 0xbc), 0x54) = 0;
		OV_S(OV_I(self, 0xc0), 0x54) = 0;
		OV_S(OV_I(self, 0xc4), 0x54) = 0;
		OV_S(OV_I(self, 200), 0x54) = 0;
		OV_S(OV_I(self, 0xcc), 0x54) = 0;
		OV_S(OV_I(self, 0xd0), 0x54) = 0;
		OV_S(OV_I(self, 0xd8), 0x54) = 1;
		OV_S(OV_I(self, 0xdc), 0x54) = 1;
		return;
	case 1:
		OV_I(self, 0x78) = 6;
		CL_WStrCpy(OV_I(self, 0xbc) + 0xa4, CL_TEXT(0x70));
		CL_WStrCpy(OV_I(self, 0xc0) + 0xa4, CL_TEXT(0x90));
		CL_WStrCpy(OV_I(self, 0xc4) + 0xa4, CL_TEXT(0x80));
		if (clu_menu_track == -1) {
		no_track:
			if (DAT_002bae84 != 0) {
				clu_menu_track = *(int *)(DAT_002bae84 + 0x10);
				goto show;
			}
		} else {
			if (DAT_002bae84 != 0) {
				count = FUN_00055fd0((struct BPlayList *)DAT_002bae84);
				if (clu_menu_track < count)
					goto show;
				goto no_track;
			}
			if (clu_menu_track < 0x1c)
				goto show;
		}
		clu_menu_track = 6;
	show:
		CLPause_ShowSoundtrack(self);
		if (DAT_002bae7c == 0)
			shuffle = CL_TEXT(0x1740);
		else
			shuffle = CL_TEXT(0x1730);
		CL_WSprintf((unsigned short *)(OV_I(self, 0xd0) + 0xa4), L"%S %S", CL_TEXT(0x1720), shuffle);
		CL_WStrCpy(OV_I(self, 0xd4) + 0xa4, CL_TEXT(0xb0));
		OV_F(OV_I(self, 0xc4), 0x304) = 50.0f;
		OV_F(OV_I(self, 200), 0x304) = 0.0f;
		OV_S(OV_I(self, 200), 0x54) = 0;
		OV_S(OV_I(self, 0xcc), 0x54) = 0;
		OV_S(OV_I(self, 0xd0), 0x54) = 0;
		OV_S(OV_I(self, 0xd4), 0x54) = 0;
		OV_S(OV_I(self, 0xd8), 0x54) = 0;
		OV_S(OV_I(self, 0xdc), 0x54) = 0;
		return;
	case 2:
		OV_I(self, 0x78) = 2;
		OV_I(self, 0x6c) = 2;
		CL_WStrCpy(OV_I(self, 0xbc) + 0xa4, CL_TEXT(0x40));
		CL_WStrCpy(OV_I(self, 0xc0) + 0xa4, CL_TEXT(0x10));
		CL_WStrCpy(OV_I(self, 0xc4) + 0xa4, CL_TEXT(0x20));
		break;
	case 3:
		OV_I(self, 0x78) = 2;
		OV_I(self, 0x6c) = 2;
		swprintf((wchar_t *)(OV_I(self, 0xbc) + 0xa4), L"%s %s?", CL_TEXT(0x50),
			 CL_TEXT((DAT_002baed4[OV_I(self, 0x68) * 10] + 0x149) * 0x10));
		CL_WStrCpy(OV_I(self, 0xc0) + 0xa4, CL_TEXT(0x10));
		CL_WStrCpy(OV_I(self, 0xc4) + 0xa4, CL_TEXT(0x20));
		break;
	case 4:
		OV_I(self, 0x78) = 2;
		OV_I(self, 0x6c) = 2;
		CL_WStrCpy(OV_I(self, 0xbc) + 0xa4, CL_TEXT(0x1290));
		CL_WStrCpy(OV_I(self, 0xc0) + 0xa4, CL_TEXT(0x10));
		CL_WStrCpy(OV_I(self, 0xc4) + 0xa4, CL_TEXT(0x20));
		break;
	case 5:
		CL_WStrCpy(OV_I(self, 0xbc) + 0xa4, CL_TEXT(OV_I(self, 0x18) * 0x10));
		OV_I(self, 0x78) = 0;
		OV_F(OV_I(self, 0xbc), 0x310) = OV_F(OV_I(self, 0xbc), 0x310) + 200.0f;
		if (OV_I(self, 0xc0) != 0)
			OV_S(OV_I(self, 0xc0), 0x54) = 1;
		if (OV_I(self, 0xc4) != 0)
			OV_S(OV_I(self, 0xc4), 0x54) = 1;
		if (OV_I(self, 200) != 0)
			OV_S(OV_I(self, 200), 0x54) = 1;
		if (OV_I(self, 0xcc) != 0)
			OV_S(OV_I(self, 0xcc), 0x54) = 1;
		if (OV_I(self, 0xd0) != 0)
			OV_S(OV_I(self, 0xd0), 0x54) = 1;
		if (OV_I(self, 0xd4) != 0)
			OV_S(OV_I(self, 0xd4), 0x54) = 1;
		OV_S(OV_I(self, 0xa8), 0x54) = 0;
		OV_S(OV_I(self, 0xac), 0x54) = 0;
		OV_S(OV_I(self, 0xb0), 0x54) = 0;
		OV_S(OV_I(self, 0xb4), 0x54) = 0;
		OV_S(OV_I(self, 0xb8), 0x54) = 0;
		return;
	default:
		return;
	}
	/* pages 2-4: two answers */
	*(unsigned short *)(OV_I(self, 200) + 0xa4) = 0;
	*(unsigned short *)(OV_I(self, 0xcc) + 0xa4) = 0;
	OV_F(OV_I(self, 0xc4), 0x304) = 80.0f;
	OV_F(OV_I(self, 200), 0x304) = 60.0f;
	OV_S(OV_I(self, 200), 0x54) = 1;
	OV_S(OV_I(self, 0xcc), 0x54) = 1;
	OV_S(OV_I(self, 0xd0), 0x54) = 1;
	OV_S(OV_I(self, 0xd4), 0x54) = 1;
	OV_S(OV_I(self, 0xd4), 0x54) = 1;
	OV_S(OV_I(self, 0xd8), 0x54) = 1;
	OV_S(OV_I(self, 0xdc), 0x54) = 1;
}

/* sets the texture coordinates u0..u3, v0..v3 of a frame piece */
static void CLPause_SetUV(int piece, float u0, float u1, float u2, float u3, float v0, float v1, float v2, float v3)
{
	OV_F(piece, 1000) = u0;
	OV_F(piece, 0x3ec) = u1;
	OV_F(piece, 0x3f0) = u2;
	OV_F(piece, 0x3f4) = u3;
	OV_F(piece, 0x3f8) = v0;
	OV_F(piece, 0x3fc) = v1;
	OV_F(piece, 0x400) = v2;
	OV_F(piece, 0x404) = v3;
}

/* 0x00075c10  ClassLib (pause menu; thiscall)
Lays the eight frame pieces (four corners, four edges; 16 pixels) around the window
overlay at half its alpha, mirroring the textures. */
void CLPause_LayoutFrame(int self)
{
	int window = OV_I(self, 0x84);
	int piece;
	int i;

	for (i = 0; i < 8; i++)
		OV_F(OV_I(self, 0x88 + i * 4), 0x2b8) = OV_F(OV_I(self, 0x84), 0x2b8) * 0.5f;
	piece = OV_I(self, 0x88); /* top left */
	OV_F(piece, 0x310) = -16.0f;
	OV_F(piece, 0x314) = 16.0f;
	OV_F(piece, 0x318) = 0.0f;
	piece = OV_I(self, 0x8c); /* top right */
	OV_F(piece, 0x314) = 16.0f;
	OV_F(piece, 0x318) = 0.0f;
	OV_U(piece, 0x310) = OV_U(window, 0x364);
	CLPause_SetUV(OV_I(self, 0x8c), 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	piece = OV_I(self, 0x90); /* bottom left */
	OV_F(piece, 0x314) = -OV_F(OV_I(self, 0x84), 0x368);
	OV_F(piece, 0x310) = -16.0f;
	OV_F(piece, 0x318) = 0.0f;
	CLPause_SetUV(OV_I(self, 0x90), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	piece = OV_I(self, 0x94); /* bottom right */
	OV_F(piece, 0x318) = 0.0f;
	OV_U(piece, 0x310) = OV_U(OV_I(self, 0x84), 0x364);
	OV_F(piece, 0x314) = -OV_F(OV_I(self, 0x84), 0x368);
	CLPause_SetUV(OV_I(self, 0x94), 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
	piece = OV_I(self, 0x98); /* top edge */
	OV_F(piece, 0x310) = 0.0f;
	OV_F(piece, 0x314) = 16.0f;
	OV_F(piece, 0x318) = 0.0f;
	OV_U(OV_I(self, 0x98), 0x364) = OV_U(OV_I(self, 0x84), 0x364);
	OV_U(OV_I(self, 0x98), 0x36c) = OV_U(OV_I(self, 0x98), 0x364);
	piece = OV_I(self, 0x9c); /* right edge */
	OV_F(piece, 0x314) = 0.0f;
	OV_F(piece, 0x318) = 0.0f;
	OV_U(piece, 0x310) = OV_U(OV_I(self, 0x84), 0x364);
	CLPause_SetUV(OV_I(self, 0x9c), 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	OV_U(OV_I(self, 0x9c), 0x368) = OV_U(OV_I(self, 0x84), 0x368);
	piece = OV_I(self, 0xa0); /* bottom edge */
	OV_F(piece, 0x310) = 0.0f;
	OV_F(piece, 0x314) = -OV_F(OV_I(self, 0x84), 0x368);
	OV_F(piece, 0x318) = 0.0f;
	OV_U(OV_I(self, 0xa0), 0x364) = OV_U(OV_I(self, 0x84), 0x364);
	OV_U(OV_I(self, 0xa0), 0x36c) = OV_U(OV_I(self, 0xa0), 0x364);
	CLPause_SetUV(OV_I(self, 0xa0), 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
	piece = OV_I(self, 0xa4); /* left edge */
	OV_F(piece, 0x310) = -16.0f;
	OV_F(piece, 0x314) = 0.0f;
	OV_F(piece, 0x318) = 0.0f;
	OV_U(OV_I(self, 0xa4), 0x368) = OV_U(OV_I(self, 0x84), 0x368);
	CLPause_SetUV(OV_I(self, 0xa4), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
}

/* 0x00076110  ClassLib (pause menu; thiscall)
Colours the menu items: the selected one bright and glowing, greyed soundtrack
entries on the options page when the soundtrack menu is off. */
void CLPause_ColourItems(int self)
{
	int item;
	int line;

	for (item = 1; item < OV_I(self, 0x78) + 1; item++) {
		line = OV_I(self, 0xc0 + (item - 1) * 4);
		if (OV_I(self, 0x6c) == item) {
			OV_F(line, 0x2ac) = (float)clu_item_selected_rgba[0];
			OV_F(line, 0x2b0) = (float)clu_item_selected_rgba[1];
			OV_F(line, 0x2b4) = (float)clu_item_selected_rgba[2];
			if (OV_I(self, 0x74) < 6)
				OV_F(line, 0x2b8) = (float)clu_item_selected_rgba[3];
			if (clu_soundtrack_menu == 0 && OV_I(self, 0x74) == 1 && (item == 4 || item == 3))
				OV_F(line, 0x2b8) = OV_F(line, 0x2b8) * 0.45454544f;
			OV_F(line, 0x3a4) = (float)fabs(sin((double)clu_item_glow_speed * (double)OV_F(self, 0x58)) *
							(double)clu_item_glow);
		} else {
			OV_F(line, 0x2ac) = (float)clu_item_rgba[0];
			OV_F(line, 0x2b0) = (float)clu_item_rgba[1];
			OV_F(line, 0x2b4) = (float)clu_item_rgba[2];
			if (OV_I(self, 0x74) < 6)
				OV_F(line, 0x2b8) = (float)clu_item_rgba[3];
			if (clu_soundtrack_menu == 0 && OV_I(self, 0x74) == 1 && (item == 4 || item == 3)) {
				OV_F(line, 0x2ac) = OV_F(line, 0x2ac) * 0.5f;
				OV_F(line, 0x2b0) = OV_F(line, 0x2b0) * 0.5f;
				OV_F(line, 0x2b4) = OV_F(line, 0x2b4) * 0.5f;
				OV_F(line, 0x2b8) = OV_F(line, 0x2b8) * 0.45454544f;
			}
			OV_F(line, 0x3a4) = 0.0f;
		}
	}
}

/* 0x000762e0  ClassLib (pause menu; thiscall)
Draws the "controller unplugged" message (pulsing, over a darkened screen) once a
controller has been missing for 20 frames, else the debug message lines. */
void CLPause_DrawMessages(int self)
{
	struct CLTextBox box;
	float pulse;
	float spread;
	int fade;
	int found[4];
	int port;
	int player;
	int missing;
	int i;
	int len;

	clu_unplugged_phase = clu_time_scale * 0.05f + clu_unplugged_phase;
	if (clu_unplugged_phase > 3.1415925f)
		clu_unplugged_phase = clu_unplugged_phase - 3.1415925f;
	pulse = fabsf((float)sin((double)clu_unplugged_phase));
	spread = pulse * 2.5f;
	fade = 0x7f - (int)((double)pulse * (double)127.0f);
	if (clu_unplugged_time < 0x14) {
		if (clu_unplugged_time == 0)
			return;
		FUN_00043b70(0);
		CL_SetRenderState3(3, 0, 0);
		FUN_00044b00(6, 1, 0);
		FUN_00044b00(10, 0, 0);
		box.y = clu_debug_y;
		box.width = 640.0f;
		box.height = 0.0f;
		box.halign = 0;
		box.valign = 1;
		box.box_w = 0;
		box.box_h = 0;
		for (i = 0; i < DAT_002ba900; i++) {
			box.x = (float)((double)i * (double)30.0f + (double)clu_debug_x);
			len = FUN_0004b110((short *)clu_debug_texts[DAT_002ba8b8[i]]);
			CL_DrawText(OV_U(self, 0x80), clu_debug_texts[DAT_002ba8b8[i]], len, &box, 0x7f, 0x7f, 0x7f, 0x7f);
		}
		if (DAT_002ba904 > 0.0f) {
			DAT_002ba904 = DAT_002ba904 - clu_time_scale;
			box.width = 640.0f;
			box.y = clu_debug_y;
			box.height = 0.0f;
			box.halign = 2;
			box.valign = 1;
			box.box_w = 0;
			box.box_h = 0;
			box.x = -320.0f;
			len = FUN_0004b110((short *)DAT_002ba7a8);
			CL_DrawText(OV_U(self, 0x80), DAT_002ba7a8, len, &box, 0x7f, 0x7f, 0x7f, 0x7f);
		}
		FUN_00043b70(1);
		FUN_00046d00(3);
		return;
	}
	CLScreen_DrawDarken(200, 0);
	FUN_00043b70(0);
	CL_SetRenderState3(3, 0, 0);
	CL_SetRenderState3(7, 1, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 0, 0);
	box.width = 480.0f;
	box.y = 50.0f;
	box.height = 0.0f;
	box.halign = 2;
	box.valign = 1;
	box.box_w = 1;
	box.box_h = 0;
	box.x = -240.0f;
	if (DAT_00284408 == 0) {
		CL_WSprintf(clu_message, CL_TEXT(0x1af0));
	} else {
		found[0] = -1;
		found[1] = -1;
		found[2] = -1;
		found[3] = -1;
		for (i = 0; i < 4; i++) {
			port = FUN_0004a0d0(i);
			if (port != -1)
				found[port] = port;
		}
		missing = port;
		for (player = 0; player < 4; player++) {
			if (found[player] == -1 && DAT_002baed8[player * 10] != 0) {
				missing = player;
				break;
			}
		}
		if (DAT_002ba99c == clu_pause_controller)
			CL_WSprintf(clu_message, CL_TEXT(0x1bb0), missing + 1);
		else
			CL_WSprintf(clu_message, CL_TEXT(0x1ae0), missing + 1);
	}
	box.x = box.x + 1.0f;
	box.y = box.y - 1.0f;
	len = FUN_0004b110((short *)clu_message);
	CLSprite_DrawBlurred(OV_U(self, 0x80), (undefined4)clu_message, len, (float *)&box, 0x7f, 0x7f, 0x7f, fade,
			     spread, 1.0f, 0);
	len = FUN_0004b110((short *)clu_message);
	CL_DrawText(OV_U(self, 0x80), clu_message, len, &box, 0x7f, 0x7f, 0x7f, fade + 0x7f);
	FUN_00043b70(1);
	FUN_00046d00(3);
	FUN_00046d00(7);
}

/* ---------- opening the pause menu */

/* 0x0029096c  character colours (4 bytes r, g, b, 0 per character) */
#define clu_character_rgb CLU_ARR(unsigned char, 0x0029096c)
/* 0x00290984 .. 0x00290998  layout: title y 4, window y 30, first line y 14, line step 40,
title x 264, window height base 48 */
#define clu_title_y CLU_VAR(float, 0x00290984)
#define clu_window_y CLU_VAR(float, 0x00290988)
#define clu_lines_y CLU_VAR(float, 0x0029098c)
#define clu_line_step CLU_VAR(float, 0x00290990)
#define clu_window_base CLU_VAR(float, 0x00290998)

/* overlay descriptor fields (desc is the 0x3b0-byte descriptor of COverlay_Create*) */
#define DESC_I(d, off) (*(int *)((unsigned char *)(d) + (off)))
#define DESC_U(d, off) (*(unsigned int *)((unsigned char *)(d) + (off)))
#define DESC_F(d, off) (*(float *)((unsigned char *)(d) + (off)))
#define DESC_S(d, off) (*(short *)((unsigned char *)(d) + (off)))

extern int DAT_002bae68; /* core: effects volume */
extern int DAT_002bae6c; /* core: music volume */

/* 0x00076700  ClassLib (pause menu; thiscall)
Opens the pause menu for player `player`: pauses the sound, makes the menu input map,
builds the window (frame, volume bars, title and lines) and hides the overlays marked
to hide while paused. */
void CLPause_Begin(int self, int player)
{
	undefined4 desc[0xec];
	undefined4 line[0xec];
	char name[64];
	int character;
	int i;
	int node;

	memset(desc, 0, sizeof(desc));
	memset(line, 0, sizeof(line));
	CLU_DebugPrintf("Paused!\n");
	clu_menu_track = clu_music_track;
	CLPause_PauseSound();
	FUN_0002b8f0(player);
	FUN_00080230((int)CLU_ARR(int, 0x002ed324)); /* the rumble manager (CRumble.cpp) stops */
	OV_I(self, 0x7c) = 0;
	OV_I(self, 0x6c) = 1;
	OV_I(self, 0x60) = 0;
	OV_I(self, 0x64) = 0;
	FUN_00054800(1);
	OV_I(self, 0x1c) = (int)FUN_000488c0(5);
	OV_I(self, 0x20) = (int)FUN_00048a10((struct biMap *)OV_I(self, 0x1c), "y", 3, 0, 0, 0, 0);
	OV_I(self, 0x24) = (int)FUN_00048a10((struct biMap *)OV_I(self, 0x1c), "x", 3, 0, 0, 0, 0);
	OV_I(self, 0x28) = (int)FUN_00048a10((struct biMap *)OV_I(self, 0x1c), "select", 1, 1, 0, 0, 0);
	OV_I(self, 0x2c) = (int)FUN_00048a10((struct biMap *)OV_I(self, 0x1c), "back", 1, 1, 0, 0, 0);
	OV_I(self, 0x30) = (int)FUN_00048a10((struct biMap *)OV_I(self, 0x1c), "cheat", 1, 0, 0, 0, 0);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x20), DAT_002baee0[player * 10] | 0xbe, DAT_002baee0[player * 10] | 0xbd);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x24), DAT_002baee0[player * 10] | 0xc0, DAT_002baee0[player * 10] | 0xbf);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x20), DAT_002baee0[player * 10] | 0xba, 0);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x24), DAT_002baee0[player * 10] | 0xb9, 0);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x28), DAT_002baee0[player * 10] | 0xc5, 0);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x28), DAT_002baee0[player * 10] | 0xc1, 0);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x2c), DAT_002baee0[player * 10] | 0xc6, 0);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x2c), DAT_002baee0[player * 10] | 0xc2, 0);
	FUN_00048b20((struct biChannel *)OV_I(self, 0x30), DAT_002baee0[player * 10] | 0xcb, 0);
	FUN_00048d40((struct biChannel *)OV_I(self, 0x28));
	FUN_00048d40((struct biChannel *)OV_I(self, 0x2c));
	OV_I(self, 0x68) = player;

	/* the window: an untextured image in the pausing player's colour */
	memset(desc, 0, sizeof(desc));
	character = DAT_002baed4[player * 10];
	DESC_F(desc, 0x250) = (float)clu_character_rgb[character * 4];
	DESC_F(desc, 0x2a8) = clu_window_y;
	DESC_F(desc, 0x254) = (float)clu_character_rgb[character * 4 + 1];
	DESC_I(desc, 0x288) = 0;
	DESC_I(desc, 4) = 0;
	DESC_F(desc, 0x258) = (float)clu_character_rgb[character * 4 + 2];
	DESC_I(desc, 0x308) = 0;
	DESC_I(desc, 0x30c) = 0;
	DESC_U(desc, 0x2a4) = 0x80000000; /* -0.0f */
	DESC_I(desc, 0x2ac) = 0;
	DESC_I(desc, 0x25c) = 0;
	OV_I(self, 0x84) = COverlay_Create((int *)desc);

	/* the frame: four corners, four edges */
	for (i = 0; i < 8; i++) {
		if (i < 4)
			sprintf(name, "corner_pause%d.bmp", DAT_002baed4[player * 10] + 1);
		else
			sprintf(name, "edge_pause%d.bmp", DAT_002baed4[player * 10] + 1);
		DESC_I(desc, 4) = (int)FUN_0004bb30(0, name, 0, 0xdefa);
		DESC_F(desc, 0x250) = 127.0f;
		DESC_F(desc, 0x254) = 127.0f;
		DESC_F(desc, 0x258) = 127.0f;
		DESC_F(desc, 0x25c) = 100.0f;
		DESC_I(desc, 0x308) = 0;
		OV_I(self, 0x88 + i * 4) = COverlay_CreateChild(OV_I(self, 0x84), desc);
	}

	/* the music and effects volume bars */
	memset(desc, 0, sizeof(desc));
	DESC_F(desc, 0x2b8) = (float)((double)clu_line_step + (double)clu_lines_y + (double)5.0f);
	DESC_U(desc, 0x290) = 0x8000;
	DESC_I(desc, 0x288) = 0;
	DESC_F(desc, 0x2b4) = 224.0f;
	DESC_I(desc, 0x2bc) = 0;
	DESC_F(desc, 0x250) = 255.0f;
	DESC_F(desc, 0x254) = 255.0f;
	DESC_F(desc, 0x258) = 255.0f;
	DESC_I(desc, 0x25c) = 0;
	DESC_I(desc, 4) = 0;
	DESC_F(desc, 0x308) = 50.0f;
	DESC_F(desc, 0x30c) = 15.0f;
	DESC_I(desc, 0x378) = (DAT_002bae6c * 0xb) / 256;
	OV_I(self, 0xdc) = COverlay_CreateChild(OV_I(self, 0x84), desc);
	OV_U(OV_I(self, 0xdc), 0x20) = (uint32_t)FUN_00074c10;
	memset(desc, 0, sizeof(desc));
	DESC_I(desc, 0x2bc) = 0;
	DESC_I(desc, 0x25c) = 0;
	DESC_F(desc, 0x2b8) = (float)((double)clu_line_step + (double)clu_line_step + (double)clu_lines_y +
				      (double)5.0f);
	DESC_I(desc, 0x378) = (DAT_002bae68 * 0xb) / 256;
	DESC_U(desc, 0x290) = 0x8000;
	DESC_I(desc, 0x288) = 0;
	DESC_F(desc, 0x2b4) = 224.0f;
	DESC_F(desc, 0x250) = 255.0f;
	DESC_F(desc, 0x254) = 255.0f;
	DESC_F(desc, 0x258) = 255.0f;
	DESC_I(desc, 4) = 0;
	DESC_F(desc, 0x308) = 50.0f;
	DESC_F(desc, 0x30c) = 15.0f;
	OV_I(self, 0xd8) = COverlay_CreateChild(OV_I(self, 0x84), desc);
	OV_U(OV_I(self, 0xd8), 0x20) = (uint32_t)FUN_00074c10;

	/* the title */
	memset(desc, 0, sizeof(desc));
	DESC_I(desc, 0x44) = OV_I(self, 0x80);
	DESC_U(desc, 0x290) = 0x8000;
	DESC_I(desc, 0x288) = 2;
	DESC_F(desc, 0x2b4) = clu_title_x;
	DESC_F(desc, 0x2b8) = clu_title_y;
	DESC_I(desc, 0x2bc) = 0;
	DESC_S(desc, 0x24c) = 2;
	DESC_F(desc, 0x250) = (float)clu_item_selected_rgba[0];
	DESC_I(desc, 0x25c) = 0;
	DESC_F(desc, 0x254) = (float)clu_item_selected_rgba[1];
	DESC_F(desc, 0x258) = (float)clu_item_selected_rgba[2];
	CL_WStrCpy((unsigned char *)desc + 0x48, CL_TEXT(0x60));
	OV_I(self, 0xbc) = COverlay_CreateChild(OV_I(self, 0x84), desc);

	/* the six menu lines */
	memset(desc, 0, sizeof(desc));
	DESC_I(desc, 0x2bc) = 0;
	DESC_I(desc, 0x44) = OV_I(self, 0x80);
	DESC_F(desc, 0x250) = (float)clu_item_rgba[0];
	DESC_S(desc, 0x24c) = 0;
	DESC_I(desc, 0x25c) = 0;
	DESC_F(desc, 0x254) = (float)clu_item_rgba[1];
	DESC_I(desc, 0x288) = 2;
	DESC_F(desc, 0x2b4) = 4.0f;
	DESC_F(desc, 0x308) = 440.0f;
	DESC_U(desc, 0x290) = 0x18000;
	DESC_F(desc, 0x258) = (float)clu_item_rgba[2];
	DESC_F(desc, 0x2b8) = (float)((double)clu_line_step + (double)clu_lines_y);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(0xa0));
	OV_I(self, 0xc0) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_line_step + (double)clu_line_step + (double)clu_lines_y);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(0x70));
	OV_I(self, 0xc4) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_line_step * (double)3.0f + (double)clu_lines_y);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(0x1a40));
	OV_I(self, 200) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_line_step * (double)4.0f + (double)clu_lines_y);
	memcpy(line, desc, sizeof(line));
	swprintf((wchar_t *)((unsigned char *)line + 0x48), L"%s %s", CL_TEXT(0xc0),
		 CL_TEXT((DAT_002baed4[player * 10] + 0x149) * 0x10));
	OV_I(self, 0xcc) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_line_step * (double)5.0f + (double)clu_lines_y);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(0x30));
	OV_I(self, 0xd0) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_line_step * (double)6.0f + (double)clu_lines_y);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(0x1280));
	OV_I(self, 0xd4) = COverlay_CreateChild(OV_I(self, 0x84), line);

	/* the title page lines (page 5): heading and the button help */
	DESC_F(desc, 0x2b8) = (float)((double)clu_lines_y + (double)10.0f);
	DESC_F(desc, 0x250) = (float)clu_item_selected_rgba[0];
	DESC_I(desc, 0x3c) = 1;
	DESC_I(desc, 0x25c) = 0;
	DESC_F(desc, 0x254) = (float)clu_item_selected_rgba[1];
	DESC_F(desc, 0x258) = (float)clu_item_selected_rgba[2];
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(0x1240));
	OV_I(self, 0xa8) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_lines_y + (double)30.0f);
	DESC_F(desc, 0x2b4) = (float)((double)DESC_F(desc, 0x2b4) + (double)20.0f);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(OV_I(self, 0xc) * 0x10));
	OV_I(self, 0xac) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_lines_y + (double)130.0f);
	DESC_F(desc, 0x2b4) = (float)((double)DESC_F(desc, 0x2b4) - (double)20.0f);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(0x1250));
	OV_I(self, 0xb0) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_lines_y + (double)150.0f);
	DESC_F(desc, 0x2b4) = (float)((double)DESC_F(desc, 0x2b4) + (double)20.0f);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(OV_I(self, 0x14) * 0x10));
	OV_I(self, 0xb4) = COverlay_CreateChild(OV_I(self, 0x84), line);
	DESC_F(desc, 0x2b8) = (float)((double)clu_lines_y + (double)230.0f);
	memcpy(line, desc, sizeof(line));
	CL_WStrCpy((unsigned char *)line + 0x48, CL_TEXT(OV_I(self, 0x10) * 0x10));
	OV_I(self, 0xb8) = COverlay_CreateChild(OV_I(self, 0x84), line);

	OV_I(self, 0x5c) = 1;
	for (node = (int)CLList_Next((unsigned int *)clu_overlay_root, NULL); node != 0;
	     node = (int)CLList_Next((unsigned int *)clu_overlay_root, (unsigned int *)node)) {
		if ((*(unsigned char *)(node + 0x2e4) & 4) != 0) {
			*(short *)(node + 0x46) = *(short *)(node + 0x44);
			*(short *)(node + 0x44) = 1;
		}
	}
	OV_I(self, 0x74) = 6;
	OV_I(self, 0) = 0;
	OV_F(self, 4) = 268.0f;
	OV_F(self, 8) = (float)((double)((DAT_002baea8 != 0) + 5) * (double)clu_line_step + (double)clu_window_base);
	CLPause_LayoutFrame(self);
	OV_I(self, 0x70) = 1;
	CLList_Remove((int *)clu_overlay_root, (int *)(OV_I(self, 0x84) + 0x10));
	CLList_Insert((int *)clu_overlay_root, (int *)(OV_I(self, 0x84) + 0x10), 1);
}

/* ---------- the per-frame update */

/* 0x00290968  stick dead zone of the menu (9828.0) */
#define clu_stick_dead_zone CLU_VAR(float, 0x00290968)
/* 0x002909a4 / 0x002909a8  first and next repeat delay of the menu stick (30, 5 frames) */
#define clu_repeat_first CLU_VAR(float, 0x002909a4)
#define clu_repeat_next CLU_VAR(float, 0x002909a8)
/* 0x002909b0  time after which the results end by themselves (3600 frames) */
#define clu_results_timeout CLU_VAR(float, 0x002909b0)
/* 0x002909cc  fade speed of the window and lines (8.0) */
#define clu_fade_speed CLU_VAR(float, 0x002909cc)
/* 0x002909d0  per-frame ease of the window size (0.5) */
#define clu_window_ease CLU_VAR(float, 0x002909d0)
/* 0x002909d4  lines of the title page window (6.5) */
#define clu_title_page_lines CLU_VAR(float, 0x002909d4)
/* 0x002ecc70  stick repeat delay now */
#define clu_repeat_delay CLU_VAR(float, 0x002ecc70)
/* 0x002ecd80  stick repeat timer (> 0 after a move down, < 0 after a move up) */
#define clu_repeat_timer CLU_VAR(float, 0x002ecd80)
/* 0x002ecf04  frames the window has had its size */
#define clu_window_settled CLU_VAR(int, 0x002ecf04)
/* 0x002ed324  the rumble manager (CRumble.cpp); 0x002ed338 its flags */
#define clu_rumble ((int)CLU_ADDR(0x002ed324))
#define clu_rumble_flags CLU_VAR(unsigned int, 0x002ed338)

extern int DAT_0028198c;  /* core: next game mode (-1: none) */
extern float DAT_00283760; /* core_modes_b: attract timeout (frames) */
extern int DAT_0028440c;  /* core: controller that asks for the pause (-1: none) */
extern int DAT_002bae8c;  /* core: players in the game */
extern int DAT_002bae90;  /* core: players out of the game */
extern int DAT_002baeb8;  /* core: results shown */
extern int DAT_002baed0;  /* core: game over */

/* goes back to the main page with item `item` selected (from page 1-5) */
static void CLPause_BackToMain(int self, int item)
{
	int lines;

	OV_I(self, 0x74) = 6;
	OV_I(self, 0) = 0;
	lines = (DAT_002baea8 != 0) + 5;
	OV_F(self, 4) = 268.0f;
	OV_F(self, 8) = (float)((double)lines * (double)clu_line_step + (double)clu_window_base);
	OV_I(self, 0x70) = item;
	CLPause_ColourItems(self);
}

/* the "shuffle" item of the options page: toggles the shuffle, rumbles both motors of
the pausing player once when it is on (even with the rumble paused) and updates the line */
static void CLPause_ToggleShuffle(int self)
{
	unsigned int saved;
	unsigned short *shuffle;

	DAT_002bae7c = DAT_002bae7c == 0;
	CLRumble_StopAll(clu_rumble);
	if (DAT_002bae7c != 0) {
		CLRumble_Resume(clu_rumble);
		saved = clu_rumble_flags;
		clu_rumble_flags = saved & ~1u;
		CLRumble_PlayMotor(clu_rumble, OV_I(self, 0x68), 1, 0x7ff8, 200, 10, 0x5a, 0, 3);
		CLRumble_PlayMotor(clu_rumble, OV_I(self, 0x68), 0, 0x7ff8, 200, 10, 0x5a, 0, 3);
		clu_rumble_flags = (clu_rumble_flags & ~1u) | (saved & 1u);
	}
	if (DAT_002bae7c == 0)
		shuffle = CL_TEXT(0x1740);
	else
		shuffle = CL_TEXT(0x1730);
	CL_WSprintf((unsigned short *)(OV_I(self, 0xd0) + 0xa4), L"%S %S", CL_TEXT(0x1720), shuffle);
	CLSamples_PlayGeneric(7, 0x7f, 0, 0, NULL);
}

/* resumes the game from the menu: the window closes (page 6, then 8 and 7 count down) */
static void CLPause_Resume(int self)
{
	CLRumble_Resume(clu_rumble);
	OV_I(self, 0x7c) = 1;
	OV_I(self, 0x74) = 6;
	OV_F(self, 8) = 0.0f;
	OV_F(self, 4) = 0.0f;
}

/* changes a volume bar (0..10) by the sign of `x`; returns 0 at the limit */
static int CLPause_StepVolume(int bar, int x, int *volume)
{
	int level = OV_I(bar, 0x3d4);

	if (x > 0) {
		if (level >= 10)
			return 0;
		level++;
	} else {
		if (level <= 0)
			return 0;
		level--;
	}
	OV_I(bar, 0x3d4) = level;
	*volume = (OV_I(bar, 0x3d4) * 0xff) / 10;
	CLSamples_PlayGeneric(8, 0xff, 0, 0, NULL);
	return 1;
}

/* 0x000772e0  ClassLib (pause menu; thiscall)
The per-frame update of the pause object (self+0x5c: 1 while the menu is open). Opens the
menu on a pause request, ends the results screen, brings back players that press Pause
and, while open, animates the window (page 6: lines fade out, 8: window resizes, 7: lines
fade in), reads the stick and buttons and acts on the selected item. Returns 1 when the
game mode must end (results over, quit, restart), else 0. The parameter keeps the table's
pointer type for the callers in other modules. */
int CLPause_Update(undefined4 *pause)
{
	int self = (int)pause;
	float time;
	double fade;
	double ease;
	double alpha;
	float limit;
	int state;
	int player;
	int done;
	int i;
	int x;
	int y;
	int held;
	int line;

	clu_pause_controller = DAT_002ba99c;
	time = clu_time_scale + OV_F(self, 0x58);
	OV_F(self, 0x58) = time;
	state = OV_I(self, 0x5c);
	if (state >= 0x14)
		return 0;
	if (state != 1 && DAT_0028440c != -1) {
		CLPause_Begin(self, DAT_0028440c);
		return 0;
	}

	/* the results screen ends after a time or on any button */
	if (DAT_002baeb8 != 0) {
		if (time > clu_results_timeout)
			goto end_mode;
		for (player = 0; player < DAT_002bae94; player++) {
			if (OV_I(OV_I(self, 0x38 + player * 4), 0x14) != 0 ||
			    OV_I(OV_I(self, 0x48 + player * 4), 0x14) != 0) {
				DAT_00283760 = 1800.0f;
				goto end_mode;
			}
		}
	}

	/* Pause: opens the menu, or brings an out player back into the game */
	if (state != 1) {
		for (player = 0; player < DAT_002bae94; player++) {
			if (OV_I(OV_I(self, 0x38 + player * 4), 0x14) == 0)
				continue;
			if (DAT_002baed8[player * 10] != 0) {
				CLPause_Begin(self, player);
				return 0;
			}
			if (DAT_002baed0 == 0) {
				CLSamples_PlayCommentary(0x18, -1, 0xff, 0, 0, NULL);
				DAT_002baed8[player * 10] = 1;
				DAT_002bae90 = DAT_002bae90 - 1;
				DAT_002bae8c = DAT_002bae8c + 1;
				CLHud_ShowPositionMarker(player);
			}
		}
	}
	if (OV_I(self, 0x5c) != 1)
		return 0;

	OV_F(OV_I(self, 0x84), 0x2ac) = (float)clu_character_rgb[DAT_002baed4[OV_I(self, 0x68) * 10] * 4];
	OV_F(OV_I(self, 0x84), 0x2b0) = (float)clu_character_rgb[DAT_002baed4[OV_I(self, 0x68) * 10] * 4 + 1];
	OV_F(OV_I(self, 0x84), 0x2b4) = (float)clu_character_rgb[DAT_002baed4[OV_I(self, 0x68) * 10] * 4 + 2];
	CLPause_ColourItems(self);
	if (OV_I(OV_I(self, 0x30), 0x14) != 0 && OV_I(self, 0x74) < 6) {
		FUN_0002be10(); /* cheats (ECX holds the cheat action; the callee does not read it) */
		return 0;
	}
	DAT_002ba900 = 0;
	memset(DAT_002ba8b8, 0, 0x11 * 4);
	if (OV_I(self, 0x7c) != 0 && OV_I(self, 0x74) == 7) {
		OV_I(self, 0x7c) = OV_I(self, 0x7c) - 1;
		if (OV_I(self, 0x7c) != 0)
			return 0;
		CLPause_End(self, 0);
		return 0;
	}

	/* page 6: the volume bars, title page lines and menu lines fade out */
	if (OV_I(self, 0x74) == 6) {
		fade = (double)clu_fade_speed * (double)clu_time_scale;
		line = OV_I(self, 0xdc);
		OV_F(line, 0x2b8) = (float)((double)OV_F(line, 0x2b8) - (fade + fade));
		if (OV_F(OV_I(self, 0xdc), 0x2b8) <= 0.0f)
			OV_F(OV_I(self, 0xdc), 0x2b8) = 0.0f;
		line = OV_I(self, 0xd8);
		OV_F(line, 0x2b8) = (float)((double)OV_F(line, 0x2b8) - (fade + fade));
		if (OV_F(OV_I(self, 0xd8), 0x2b8) <= 0.0f)
			OV_F(OV_I(self, 0xd8), 0x2b8) = 0.0f;
		for (i = 0; i < 5; i++) {
			line = OV_I(self, 0xa8 + i * 4);
			OV_F(line, 0x2b8) = (float)((double)OV_F(line, 0x2b8) - fade);
			if (OV_F(OV_I(self, 0xa8 + i * 4), 0x2b8) <= 0.0f)
				OV_F(OV_I(self, 0xa8 + i * 4), 0x2b8) = 0.0f;
		}
		done = 0;
		for (i = 0; i < 7; i++) {
			line = OV_I(self, 0xbc + i * 4);
			OV_F(line, 0x2b8) = (float)((double)OV_F(line, 0x2b8) - fade);
			if (OV_F(OV_I(self, 0xbc + i * 4), 0x2b8) <= 0.0f) {
				OV_F(OV_I(self, 0xbc + i * 4), 0x2b8) = 0.0f;
				done++;
			}
		}
		if (done == 7) {
			CLPause_SetPage(self, OV_I(self, 0));
			OV_I(self, 0x74) = 8;
			clu_window_settled = 0;
		}
	}

	/* page 8: the window fades in (or out when closing) and eases to its size */
	if (OV_I(self, 0x74) == 8) {
		done = 0;
		fade = (double)clu_fade_speed * (double)clu_time_scale * (double)3.0f;
		line = OV_I(self, 0x84);
		if (OV_I(self, 0x7c) != 0) {
			OV_F(line, 0x2b8) = (float)((double)OV_F(line, 0x2b8) - fade);
			if (OV_F(OV_I(self, 0x84), 0x2b8) <= 0.0f) {
				OV_F(OV_I(self, 0x84), 0x2b8) = 0.0f;
				done = 1;
			}
		} else {
			OV_F(line, 0x2b8) = (float)(fade + (double)OV_F(line, 0x2b8));
			if (OV_F(OV_I(self, 0x84), 0x2b8) >= 200.0f) {
				OV_F(OV_I(self, 0x84), 0x2b8) = 200.0f;
				done = 1;
			}
		}
		line = OV_I(self, 0x84);
		if (fabs((double)OV_F(line, 0x364) - (double)OV_F(self, 4)) <= 2.0) {
			OV_F(line, 0x364) = OV_F(self, 4);
			done++;
		} else {
			ease = pow((double)clu_window_ease, (double)clu_time_scale);
			OV_F(line, 0x364) = (float)((1.0 - ease) * (double)OV_F(self, 4) + ease * (double)OV_F(line, 0x364));
		}
		line = OV_I(self, 0x84);
		if (fabs((double)OV_F(line, 0x368) - (double)OV_F(self, 8)) <= 2.0) {
			OV_F(line, 0x368) = OV_F(self, 8);
			done++;
		} else {
			ease = pow((double)clu_window_ease, (double)clu_time_scale);
			OV_F(line, 0x368) = (float)((1.0 - ease) * (double)OV_F(self, 8) + ease * (double)OV_F(line, 0x368));
		}
		line = OV_I(self, 0x84);
		OV_F(line, 0x300) = OV_F(line, 0x364) * -0.5f;
		line = OV_I(self, 0x84);
		OV_F(line, 0x304) = (float)((double)OV_F(line, 0x368) * (double)0.5f + (double)clu_window_y);
		CLPause_LayoutFrame(self);
		if (done == 3)
			clu_window_settled = clu_window_settled + 1;
		if (clu_window_settled == 3) {
			OV_I(self, 0x74) = 7;
			clu_window_settled = 0;
		}
	}

	/* page 7: the lines of the new page fade in, then the page is shown */
	if (OV_I(self, 0x74) == 7 && OV_I(self, 0x7c) == 0) {
		done = 0;
		fade = (double)clu_fade_speed * (double)clu_time_scale;
		alpha = (double)clu_item_selected_rgba[3] + (double)clu_item_selected_rgba[3];
		line = OV_I(self, 0xdc);
		OV_F(line, 0x2b8) = (float)((fade + fade) + (double)OV_F(line, 0x2b8));
		if (alpha <= (double)OV_F(OV_I(self, 0xdc), 0x2b8))
			OV_F(OV_I(self, 0xdc), 0x2b8) = (float)alpha;
		line = OV_I(self, 0xd8);
		OV_F(line, 0x2b8) = (float)((fade + fade) + (double)OV_F(line, 0x2b8));
		if (alpha <= (double)OV_F(OV_I(self, 0xd8), 0x2b8))
			OV_F(OV_I(self, 0xd8), 0x2b8) = (float)alpha;
		for (i = 0; i < 5; i++) {
			line = OV_I(self, 0xa8 + i * 4);
			OV_F(line, 0x2b8) = (float)(fade + (double)OV_F(line, 0x2b8));
			if (OV_F(OV_I(self, 0xa8 + i * 4), 0x2b8) >= 127.0f)
				OV_F(OV_I(self, 0xa8 + i * 4), 0x2b8) = 127.0f;
		}
		line = OV_I(self, 0xbc);
		OV_F(line, 0x2b8) = (float)(fade + (double)OV_F(line, 0x2b8));
		limit = (float)clu_item_selected_rgba[3];
		if (limit <= OV_F(OV_I(self, 0xbc), 0x2b8)) {
			OV_F(OV_I(self, 0xbc), 0x2b8) = limit;
			done = 1;
		}
		for (i = 1; i < 7; i++) {
			line = OV_I(self, 0xbc + i * 4);
			OV_F(line, 0x2b8) = (float)(fade + (double)OV_F(line, 0x2b8));
			if (OV_I(self, 0x6c) == i)
				limit = (float)clu_item_selected_rgba[3];
			else
				limit = (float)clu_item_rgba[3];
			if (limit <= OV_F(OV_I(self, 0xbc + i * 4), 0x2b8)) {
				OV_F(OV_I(self, 0xbc + i * 4), 0x2b8) = limit;
				done++;
			}
		}
		if (done == 7)
			OV_I(self, 0x74) = OV_I(self, 0);
	}

	OV_F(self, 0x60) = clu_time_scale + OV_F(self, 0x60);
	OV_I(self, 0x64) = OV_I(self, 0x64) + 1;
	if (clu_repeat_timer > 0.0f) {
		clu_repeat_timer = clu_repeat_timer - clu_time_scale;
		if (clu_repeat_timer < 0.0f)
			clu_repeat_timer = 0.0f;
	} else if (clu_repeat_timer < 0.0f) {
		clu_repeat_timer = clu_repeat_timer + clu_time_scale;
		if (clu_repeat_timer > 0.0f)
			clu_repeat_timer = 0.0f;
	}

	/* the stick moves the selection, with a repeat delay */
	if (OV_I(self, 0x74) <= 5) {
		y = OV_I(OV_I(self, 0x20), 0x14);
		if ((double)y <= (double)clu_stick_dead_zone && !((double)y < -(double)clu_stick_dead_zone)) {
			clu_repeat_delay = clu_repeat_first;
			clu_repeat_timer = 0.0f;
		}
		if (OV_I(self, 0x78) != 0) {
			if (!(clu_repeat_timer > 0.0f)) {
				y = OV_I(OV_I(self, 0x20), 0x14);
				if ((double)y > (double)clu_stick_dead_zone &&
				    y > abs(OV_I(OV_I(self, 0x24), 0x14)) * 2) {
					CLSamples_PlayGeneric(7, 0x7f, 0, 0, NULL);
					clu_repeat_timer = clu_repeat_delay;
					clu_repeat_delay = clu_repeat_next;
					OV_I(self, 0x6c) = OV_I(self, 0x6c) + 1;
					if (OV_I(self, 0x6c) == OV_I(self, 0x78) + 1)
						OV_I(self, 0x6c) = 1;
				}
			}
			if (!(clu_repeat_timer < 0.0f)) {
				y = OV_I(OV_I(self, 0x20), 0x14);
				if (-(double)clu_stick_dead_zone > (double)y &&
				    -y > abs(OV_I(OV_I(self, 0x24), 0x14)) * 2) {
					CLSamples_PlayGeneric(7, 0x7f, 0, 0, NULL);
					clu_repeat_timer = -clu_repeat_delay;
					clu_repeat_delay = clu_repeat_next;
					OV_I(self, 0x6c) = OV_I(self, 0x6c) - 1;
					if (OV_I(self, 0x6c) == 0)
						OV_I(self, 0x6c) = OV_I(self, 0x78);
				}
			}
		}
	}

	/* left/right on the options page: volumes, soundtrack, song, shuffle */
	x = OV_I(OV_I(self, 0x24), 0x14);
	held = OV_I(OV_I(self, 0x24), 0x20);
	if (x != 0 && abs(x) > abs(OV_I(OV_I(self, 0x20), 0x14)) * 2 &&
	    (held == 1 || (held > 0xf && held % 5 == 0)) && OV_I(self, 0x74) == 1) {
		switch (OV_I(self, 0x6c)) {
		case 1:
			CLPause_StepVolume(OV_I(self, 0xdc), x, &DAT_002bae6c);
			FUN_00054690(DAT_002bae6c);
			break;
		case 2:
			CLPause_StepVolume(OV_I(self, 0xd8), x, &DAT_002bae68);
			break;
		case 4:
			if (clu_soundtrack_menu != 0) {
				CLSamples_PlayGeneric(7, 0x7f, 0, 0, NULL);
				if (OV_I(OV_I(self, 0x24), 0x14) > 0)
					CLPause_NextSong(self, 1);
				else
					CLPause_NextSong(self, -1);
			}
			break;
		case 3:
			if (clu_soundtrack_menu != 0) {
				CLPause_NextSoundtrack(x > 0 ? 1 : -1);
				CLPause_ShowSoundtrack(self);
			}
			break;
		case 5:
			CLPause_ToggleShuffle(self);
			break;
		}
	}

	/* select */
	if (OV_I(OV_I(self, 0x28), 0x14) != 0) {
		switch (OV_I(self, 0x74)) {
		case 0: /* main page */
			CLSamples_PlayGeneric(9, 0x7f, 0, 0, NULL);
			switch (OV_I(self, 0x6c)) {
			case 1: /* resume */
				CLPause_Resume(self);
				return 0;
			case 2: /* options */
				OV_I(self, 0) = 1;
				OV_I(self, 0x74) = 6;
				OV_I(self, 0x70) = 1;
				OV_F(self, 8) = (float)((double)clu_line_step * (double)6.0f + (double)clu_window_base);
				CLPause_ColourItems(self);
				return 0;
			case 3: /* the title page */
				OV_I(self, 0x74) = 6;
				OV_I(self, 0) = 5;
				OV_F(self, 4) = 468.0f;
				OV_F(self, 8) = (float)((double)clu_title_page_lines * (double)clu_line_step +
							(double)clu_window_base);
				return 0;
			case 4: /* quit to the board */
				OV_I(self, 0) = 3;
				break;
			case 5: /* quit */
				OV_I(self, 0) = 2;
				break;
			case 6: /* restart */
				OV_I(self, 0) = 4;
				break;
			default:
				return 0;
			}
			OV_I(self, 0x74) = 6;
			OV_I(self, 0x70) = 2;
			OV_F(self, 8) = (float)((double)clu_line_step + (double)clu_line_step + (double)clu_window_base);
			CLPause_ColourItems(self);
			return 0;
		case 1: /* options page */
			switch (OV_I(self, 0x6c)) {
			case 4:
				if (clu_soundtrack_menu == 0)
					return 0;
				CLPause_NextSong(self, 1);
				CLSamples_PlayGeneric(7, 0x7f, 0, 0, NULL);
				return 0;
			case 3:
				if (clu_soundtrack_menu == 0)
					return 0;
				CLPause_NextSoundtrack(1);
				CLPause_ShowSoundtrack(self);
				return 0;
			case 6: /* back */
				CLSamples_PlayGeneric(10, 0x7f, 0, 0, NULL);
				CLPause_BackToMain(self, 2);
				return 0;
			case 5:
				CLPause_ToggleShuffle(self);
				return 0;
			}
			return 0;
		case 2: /* quit? */
			CLSamples_PlayGeneric(9, 0x7f, 0, 0, NULL);
			if (OV_I(self, 0x6c) == 1) {
				CLRumble_Resume(clu_rumble);
				goto quit;
			}
			CLPause_BackToMain(self, 5);
			return 0;
		case 3: /* quit <player>? */
			CLSamples_PlayGeneric(9, 0x7f, 0, 0, NULL);
			if (OV_I(self, 0x6c) != 1) {
				CLPause_BackToMain(self, 4);
				return 0;
			}
			DAT_002baed8[OV_I(self, 0x68) * 10] = 0;
			DAT_002bae90 = DAT_002bae90 + 1;
			DAT_002bae8c = DAT_002bae8c - 1;
			if (DAT_002bae8c == 0)
				goto quit;
			CLPause_Resume(self);
			return 0;
		case 4: /* restart? */
			CLSamples_PlayGeneric(9, 0x7f, 0, 0, NULL);
			if (OV_I(self, 0x6c) == 1) {
				CLPause_End(self, 1);
				FUN_0002b6f0((struct CGameModeStack *)DAT_002ba750, 1);
				return 1;
			}
			CLPause_BackToMain(self, 6);
			return 0;
		case 5: /* title page */
			CLSamples_PlayGeneric(10, 0x7f, 0, 0, NULL);
			CLPause_BackToMain(self, 3);
			return 0;
		}
		return 0;
	}

	/* back */
	if (OV_I(OV_I(self, 0x2c), 0x14) != 0) {
		if (OV_I(self, 0x74) <= 5)
			CLSamples_PlayGeneric(10, 0x7f, 0, 0, NULL);
		switch (OV_I(self, 0x74)) {
		case 0:
			CLPause_Resume(self);
			return 0;
		case 1:
			CLPause_BackToMain(self, 2);
			return 0;
		case 2:
			CLPause_BackToMain(self, 5);
			return 0;
		case 3:
			CLPause_BackToMain(self, 4);
			return 0;
		case 4:
			CLPause_BackToMain(self, 6);
			return 0;
		case 5: /* (the sound plays twice) */
			CLSamples_PlayGeneric(10, 0x7f, 0, 0, NULL);
			CLPause_BackToMain(self, 3);
			return 0;
		}
	}
	return 0;

quit:
	CLPause_End(self, 1);
	DAT_0028198c = -1;
	FUN_0002c2e0();
	return 1;

end_mode:
	FUN_0002b6f0((struct CGameModeStack *)DAT_002ba750, 1);
	CLSamples_ClearQueue();
	return 1;
}
