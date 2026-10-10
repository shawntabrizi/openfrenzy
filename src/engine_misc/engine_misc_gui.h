/*
ENGINE_MISC_GUI.H

Babel BGUI (C:\Babel\Xbox\Src\BGUI\gui.cpp): the debug menu system drawn straight
into the locked back buffer, and the BINPUT map/channel layouts it uses.

All GUI state lives in one block of the original .bss, 0x002caf18-0x002cb56c
(struct BGuiGlobals, defined in BGUI/unplaced.c). The original code addresses
parts of it by absolute address (the menu list sentinel at 0x002cafa4 and its
next/prev links at 0x002cb53c/0x002cb540; bdisplay's 0x00047070 clears
0x002caf18-0x002cb55c with one loop). So unplaced.c also exports every Ghidra
name of the block (DAT_002caf18 ... DAT_002cb568) as a symbol at the matching
offset of g_bgui: another module that writes DAT_002cb540 or loops from
&DAT_002caf18 reaches the same bytes. Declare them through this header.

Owner: engine_misc.
*/

#ifndef OPENFRENZY_ENGINE_MISC_GUI_H
#define OPENFRENZY_ENGINE_MISC_GUI_H

/* ---------- BINPUT (input.cpp; the functions are in module bkernel except biCreateMap) */

/* an input channel ("Left", "Select", "LeftRight", ...): 0xf0 bytes */
struct BInputChannel {
	char name[0x10];          /* 0x00 */
	int type;                 /* 0x10 1 digital, 3 analogue */
	int value;                /* 0x14 pressed (digital) or position (analogue) */
	int mode;                 /* 0x18 */
	int deadZone;             /* 0x1c (0x7ff8 by default) */
	int state;                /* 0x20 */
	unsigned char rest[0xf0 - 0x24];
};
OF_ASSERT_SIZE(struct BInputChannel, 0xf0);

/* an input map: header (0x18 bytes) followed by its channels */
struct BInputMap {
	struct BInputChannel *channels; /* 0x00 = (struct BInputChannel *)(map + 1) */
	int maxChannels;          /* 0x04 */
	int numChannels;          /* 0x08 */
	int field_0c;             /* 0x0c */
	struct BInputMap *next;   /* 0x10 circular list of all maps */
	struct BInputMap *prev;   /* 0x14 */
};
OF_ASSERT_SIZE(struct BInputMap, 0x18);

/* 0x002cc1b8: first map of the circular list of all maps */
extern struct BInputMap *DAT_002cc1b8;
/* 0x002cc1b4: the input map the GUI owns while it is active */
extern struct BInputMap *DAT_002cc1b4;

/* ---------- BGUI */

/* a menu item: 0x2c bytes */
struct BGuiItem {
	char name[0x1e];          /* 0x00 */
	unsigned short flags;     /* 0x1e BGUI_ITEM_* */
	struct BGuiMenu *submenu; /* 0x20 */
	unsigned short id;        /* 0x24 passed to the callback */
	unsigned short pad26;     /* 0x26 */
	unsigned short pad28;     /* 0x28 */
	unsigned short width;     /* 0x2a text width in pixels */
};
OF_ASSERT_SIZE(struct BGuiItem, 0x2c);

#define BGUI_ITEM_SEPARATOR 1
#define BGUI_ITEM_DISABLED 2
#define BGUI_ITEM_CHECKED 4

#define BGUI_MAX_ITEMS 0x20

/* a menu: 0x5a4 bytes */
struct BGuiMenu {
	int type;                 /* 0x000 0 = menu */
	unsigned int state;       /* 0x004 bit 2: menu bar has the focus */
	int field_08;             /* 0x008 */
	int numItems;             /* 0x00c */
	unsigned int flags;       /* 0x010 bit 0: vertical (popup) menu */
	struct BGuiItem items[BGUI_MAX_ITEMS]; /* 0x014 */
	int selected;             /* 0x594 -1 = none */
	struct BGuiMenu *next;    /* 0x598 list of all menus (sentinel g_bgui.menus) */
	struct BGuiMenu *prev;    /* 0x59c */
	struct BGuiMenu *parent;  /* 0x5a0 */
};
OF_ASSERT_SIZE(struct BGuiMenu, 0x5a4);
OF_ASSERT_OFFSET(struct BGuiMenu, selected, 0x594);

/* the callback's argument */
struct BGuiEvent {
	int type;                 /* 0 */
	unsigned int id;          /* item id */
};
typedef int (*BGuiCallback)(struct BGuiEvent *event);

/* a bitmap font: one string of '0'/'1' per glyph, height rows of width characters */
struct BGuiGlyph {
	int width;
	const char *bits;
};
struct BGuiFont {
	int height;
	struct BGuiGlyph glyph[95]; /* characters 0x20-0x7e */
};

/* 0x002caf18: the GUI's state (0x654 bytes) */
struct BGuiGlobals {
	int fgR, fgG, fgB;        /* 0x000 (0x2caf18) current text colour */
	int bgR, bgG, bgB;        /* 0x00c (0x2caf24) text background colour */
	int transparent;          /* 0x018 (0x2caf30) 1: do not draw the background */
	int textR, textG, textB;  /* 0x01c (0x2caf34) normal text colour */
	int menuR, menuG, menuB;  /* 0x028 (0x2caf40) menu colour */
	int colour4c[3];          /* 0x034 (0x2caf4c) */
	int colour58[3];          /* 0x040 (0x2caf58) */
	int hiTextR, hiTextG, hiTextB; /* 0x04c (0x2caf64) highlighted text */
	int hiBarR, hiBarG, hiBarB;    /* 0x058 (0x2caf70) highlight bar */
	struct BInputMap *inputMap;    /* 0x064 (0x2caf7c) */
	struct BInputChannel *left;    /* 0x068 (0x2caf80) */
	struct BInputChannel *right;   /* 0x06c (0x2caf84) */
	struct BInputChannel *up;      /* 0x070 (0x2caf88) */
	struct BInputChannel *down;    /* 0x074 (0x2caf8c) */
	struct BInputChannel *select;  /* 0x078 (0x2caf90) */
	struct BInputChannel *deselect; /* 0x07c (0x2caf94) */
	struct BInputChannel *next;    /* 0x080 (0x2caf98) */
	struct BInputChannel *prior;   /* 0x084 (0x2caf9c) */
	struct BInputChannel *exit;    /* 0x088 (0x2cafa0) */
	struct BGuiMenu menus;         /* 0x08c (0x2cafa4) list sentinel */
	struct BGuiMenu *rootMenu;     /* 0x630 (0x2cb548) the menu bar */
	struct BGuiMenu *curMenu;      /* 0x634 (0x2cb54c) the menu with the focus */
	int active;                    /* 0x638 (0x2cb550) */
	int visible;                   /* 0x63c (0x2cb554) */
	BGuiCallback callback;         /* 0x640 (0x2cb558) */
	int xScale;                    /* 0x644 (0x2cb55c) pixels per GUI pixel */
	int yScale;                    /* 0x648 (0x2cb560) */
	unsigned int *bits;            /* 0x64c (0x2cb564) locked back buffer */
	int pitch;                     /* 0x650 (0x2cb568) in pixels */
};
OF_ASSERT_SIZE(struct BGuiGlobals, 0x654);
OF_ASSERT_OFFSET(struct BGuiGlobals, menus, 0x8c);
OF_ASSERT_OFFSET(struct BGuiGlobals, rootMenu, 0x630);
OF_ASSERT_OFFSET(struct BGuiGlobals, callback, 0x640);

extern struct BGuiGlobals g_bgui;

/* the default font (0x00284e28) and the two 8x12 icons (0x00285124: submenu
arrow, check mark) */
extern struct BGuiFont g_bguiFont;
extern const char *const g_bguiIcons[2];

/* the Ghidra names of the block, for other modules (aliases into g_bgui) */
extern int DAT_002caf18, DAT_002caf1c, DAT_002caf20, DAT_002caf24, DAT_002caf28, DAT_002caf2c;
extern int DAT_002caf30, DAT_002caf34, DAT_002caf38, DAT_002caf3c;
extern int DAT_002caf40, DAT_002caf44, DAT_002caf48, DAT_002caf4c, DAT_002caf50, DAT_002caf54;
extern int DAT_002caf58, DAT_002caf5c, DAT_002caf60, DAT_002caf64, DAT_002caf68, DAT_002caf6c;
extern int DAT_002caf70, DAT_002caf74, DAT_002caf78;
extern struct BInputMap *DAT_002caf7c;
extern struct BInputChannel *DAT_002caf80, *DAT_002caf84, *DAT_002caf88, *DAT_002caf8c;
extern struct BInputChannel *DAT_002caf90, *DAT_002caf94, *DAT_002caf98, *DAT_002caf9c;
extern struct BInputChannel *DAT_002cafa0;
extern struct BGuiMenu DAT_002cafa4;
extern struct BGuiMenu *DAT_002cb53c, *DAT_002cb540, *DAT_002cb548, *DAT_002cb54c;
extern int DAT_002cb550, DAT_002cb554;
extern BGuiCallback DAT_002cb558;
extern int DAT_002cb55c, DAT_002cb560;
extern unsigned int *DAT_002cb564;
extern int DAT_002cb568;

#endif
