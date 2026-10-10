/*
BGUI/unplaced.c: Babel's debug menu GUI (menus drawn straight into the locked back
buffer, driven by a BINPUT map). 0x00047370 is tagged with
C:\Babel\Xbox\Src\BGUI\gui.cpp; the others (0x000473f0-0x00048790) lie in the gap
after it and are very likely the same file (they share its globals), but no assert
names it, so they stay in unplaced.c.

0x00047070 (bGuiInit) and 0x00047300 (bGuiActivate) also belong here, but the
function table gives them to module bdisplay.
*/

#include "engine_misc_gui.h"
#include "engine_misc_extern.h"
#include "bdisplay_types.h"
#include "bdisplay_globals.h"

/* ---------- globals */

/* 0x002caf18-0x002cb56c (.bss), see engine_misc_gui.h */
struct BGuiGlobals g_bgui = { 0 };

/* the Ghidra names of the block, as symbols inside g_bgui, so that other modules
writing the original globals (bdisplay's 0x00047070 initialiser) reach g_bgui. A
module-level ".set" makes NAME an alias of g_bgui + offset (Mach-O assembly from llc;
port/macos/tools/asm_convert.py strips the leading underscores). */
#define BGUI_ALIAS(name, offset) \
	__asm__(".globl _" #name "\n\t.set _" #name ", _g_bgui + " #offset "\n")
BGUI_ALIAS(DAT_002caf18, 0x000);
BGUI_ALIAS(DAT_002caf1c, 0x004);
BGUI_ALIAS(DAT_002caf20, 0x008);
BGUI_ALIAS(DAT_002caf24, 0x00c);
BGUI_ALIAS(DAT_002caf28, 0x010);
BGUI_ALIAS(DAT_002caf2c, 0x014);
BGUI_ALIAS(DAT_002caf30, 0x018);
BGUI_ALIAS(DAT_002caf34, 0x01c);
BGUI_ALIAS(DAT_002caf38, 0x020);
BGUI_ALIAS(DAT_002caf3c, 0x024);
BGUI_ALIAS(DAT_002caf40, 0x028);
BGUI_ALIAS(DAT_002caf44, 0x02c);
BGUI_ALIAS(DAT_002caf48, 0x030);
BGUI_ALIAS(DAT_002caf4c, 0x034);
BGUI_ALIAS(DAT_002caf50, 0x038);
BGUI_ALIAS(DAT_002caf54, 0x03c);
BGUI_ALIAS(DAT_002caf58, 0x040);
BGUI_ALIAS(DAT_002caf5c, 0x044);
BGUI_ALIAS(DAT_002caf60, 0x048);
BGUI_ALIAS(DAT_002caf64, 0x04c);
BGUI_ALIAS(DAT_002caf68, 0x050);
BGUI_ALIAS(DAT_002caf6c, 0x054);
BGUI_ALIAS(DAT_002caf70, 0x058);
BGUI_ALIAS(DAT_002caf74, 0x05c);
BGUI_ALIAS(DAT_002caf78, 0x060);
BGUI_ALIAS(DAT_002caf7c, 0x064);
BGUI_ALIAS(DAT_002caf80, 0x068);
BGUI_ALIAS(DAT_002caf84, 0x06c);
BGUI_ALIAS(DAT_002caf88, 0x070);
BGUI_ALIAS(DAT_002caf8c, 0x074);
BGUI_ALIAS(DAT_002caf90, 0x078);
BGUI_ALIAS(DAT_002caf94, 0x07c);
BGUI_ALIAS(DAT_002caf98, 0x080);
BGUI_ALIAS(DAT_002caf9c, 0x084);
BGUI_ALIAS(DAT_002cafa0, 0x088);
BGUI_ALIAS(DAT_002cafa4, 0x08c);
BGUI_ALIAS(DAT_002cb53c, 0x624);
BGUI_ALIAS(DAT_002cb540, 0x628);
BGUI_ALIAS(DAT_002cb548, 0x630);
BGUI_ALIAS(DAT_002cb54c, 0x634);
BGUI_ALIAS(DAT_002cb550, 0x638);
BGUI_ALIAS(DAT_002cb554, 0x63c);
BGUI_ALIAS(DAT_002cb558, 0x640);
BGUI_ALIAS(DAT_002cb55c, 0x644);
BGUI_ALIAS(DAT_002cb560, 0x648);
BGUI_ALIAS(DAT_002cb564, 0x64c);
BGUI_ALIAS(DAT_002cb568, 0x650);
OF_ASSERT_OFFSET(struct BGuiGlobals, menus.next, 0x624);
OF_ASSERT_OFFSET(struct BGuiGlobals, menus.prev, 0x628);

/* bDisplay globals (module bdisplay, src/bdisplay/bdisplay_globals.h): screen width
and height, the back buffer's pixel scale and the back buffer. Tentative definitions,
merged with bdisplay's (-fcommon), so the image links whichever module defines them. */
int DAT_002bc40c;
int DAT_002bc410;
int DAT_002bc46c;
int DAT_002bc470;
void *DAT_002bc480;

/* BINPUT: the map the GUI hands to the input system while it is active (module bkernel) */
struct BInputMap *DAT_002cc1b4;

#define BGUI_RGB(r, g, b) ((unsigned int)((((r) << 8 | (g)) << 8) | (b)))
#define BGUI_PIXEL(x, y) \
	(g_bgui.bits + (g_bgui.yScale * g_bgui.pitch * (y) + g_bgui.xScale * (x)))

/* ---------- functions */

/* 0x00047370  BGUI/gui.cpp
Creates a menu and links it at the end of the menu list. flags bit 0: vertical. */
struct BGuiMenu *bGuiCreateMenu(unsigned int flags)
{
	struct BGuiMenu *menu;

	menu = (struct BGuiMenu *)em_bkAllocNamed(sizeof(struct BGuiMenu),
		"C:\\Babel\\Xbox\\Src\\BGUI\\gui.cpp", 0x10b, 0x2001, "GUI Menu", 0);
	menu->prev = g_bgui.menus.prev;
	menu->next = &g_bgui.menus;
	g_bgui.menus.prev->next = menu;
	g_bgui.menus.prev = menu;
	menu->type = 0;
	menu->state = 0;
	menu->flags = flags & 1;
	menu->numItems = 0;
	menu->parent = NULL;
	menu->selected = -1;
	return menu;
}

/* 0x000473f0  BGUI (gui.cpp?)
Unlinks a menu, deletes its submenus (recursively) and frees it. */
void bGuiDeleteMenu(struct BGuiMenu *menu)
{
	int i;

	menu->next->prev = menu->prev;
	menu->prev->next = menu->next;
	if (menu->type == 0) {
		for (i = 0; i < menu->numItems; i++) {
			if (menu->items[i].submenu != NULL)
				bGuiDeleteMenu(menu->items[i].submenu);
		}
	}
	em_bkFree(menu);
}

/* 0x00047460  BGUI (gui.cpp?)
Makes a horizontal menu the menu bar. */
void bGuiSetRootMenu(struct BGuiMenu *menu)
{
	if ((menu->flags & 1) == 0 && menu->type == 0)
		g_bgui.rootMenu = menu;
}

/* 0x00047480  BGUI (gui.cpp?)
Appends an item; returns it, or NULL when the menu is full. */
struct BGuiItem *bGuiAddItem(struct BGuiMenu *menu, const char *name, unsigned short id,
	struct BGuiMenu *submenu, unsigned short flags)
{
	struct BGuiItem *item;
	int len, i, width;

	if (menu->numItems == BGUI_MAX_ITEMS)
		return NULL;
	item = &menu->items[menu->numItems];
	if (name == NULL)
		item->name[0] = '\0';
	else
		strcpy(item->name, name);
	item->flags = flags;
	item->id = id;
	item->submenu = submenu;
	if (submenu != NULL)
		submenu->parent = menu;
	len = (int)strlen(item->name);
	width = 0;
	for (i = 0; i < len; i++)
		width += g_bguiFont.glyph[item->name[i] - 0x20].width;
	item->width = (unsigned short)width;
	menu->numItems++;
	return item;
}

/* 0x00047520  BGUI (gui.cpp?)
Draws a vertical (popup) menu at (x, y), and the open submenu of its selection. */
void bGuiDrawPopup(struct BGuiMenu *menu, int x, int y)
{
	unsigned int maxWidth;
	int i, w, h, left, top, rowY, textY;
	struct BGuiItem *item;

	maxWidth = 0;
	for (i = 0; i < menu->numItems; i++) {
		if (maxWidth < menu->items[i].width)
			maxWidth = menu->items[i].width;
	}
	h = menu->numItems * 0xe + 4;
	w = maxWidth + 0x1c;
	g_bgui.fgR = g_bgui.textR;
	g_bgui.fgG = g_bgui.textG;
	g_bgui.transparent = 1;
	g_bgui.fgB = g_bgui.textB;
	left = x;
	if (DAT_002bc40c <= w + x)
		left = DAT_002bc40c - w;
	top = y;
	if (DAT_002bc410 <= h + y)
		top = DAT_002bc410 - h;
	bGuiFillRect(left, top, w, h, g_bgui.menuR, g_bgui.menuG, g_bgui.menuB);
	bGuiDrawBox(left, top, w, h, 0, g_bgui.menuR, g_bgui.menuG, g_bgui.menuB);
	rowY = top + 2;
	textY = top + 3;
	for (i = 0; i < menu->numItems; i++) {
		item = &menu->items[i];
		if ((item->flags & BGUI_ITEM_SEPARATOR) == 0) {
			if (menu->selected == i) {
				g_bgui.fgR = (item->flags & BGUI_ITEM_DISABLED) ? g_bgui.hiTextR >> 1 : g_bgui.hiTextR;
				g_bgui.fgG = (item->flags & BGUI_ITEM_DISABLED) ? g_bgui.hiTextG >> 1 : g_bgui.hiTextG;
				g_bgui.fgB = (item->flags & BGUI_ITEM_DISABLED) ? g_bgui.hiTextB >> 1 : g_bgui.hiTextB;
				bGuiFillRect(left + 1, rowY, maxWidth + 0x1a, 0xf,
					g_bgui.hiBarR, g_bgui.hiBarG, g_bgui.hiBarB);
				bGuiDrawText(NULL, left + 0x12, textY, item->name);
				if (item->flags & BGUI_ITEM_CHECKED)
					bGuiDrawIcon(1, left + 5, textY);
				if (item->submenu != NULL)
					bGuiDrawIcon(0, maxWidth + 0x12 + left, textY);
				g_bgui.fgR = g_bgui.textR;
				g_bgui.fgG = g_bgui.textG;
				g_bgui.fgB = g_bgui.textB;
				if (item->submenu != NULL && g_bgui.curMenu != menu)
					bGuiDrawPopup(item->submenu, maxWidth + 0x1a + left, rowY);
			} else {
				if (item->flags & BGUI_ITEM_DISABLED) {
					g_bgui.fgR = g_bgui.hiTextR >> 1;
					g_bgui.fgG = g_bgui.hiTextG >> 1;
					g_bgui.fgB = g_bgui.hiTextB >> 1;
				}
				bGuiDrawText(NULL, left + 0x12, textY, item->name);
				if (item->flags & BGUI_ITEM_CHECKED)
					bGuiDrawIcon(1, left + 5, textY);
				if (item->submenu != NULL)
					bGuiDrawIcon(0, maxWidth + 0x12 + left, textY);
				if (item->flags & BGUI_ITEM_DISABLED) {
					g_bgui.fgR = g_bgui.textR;
					g_bgui.fgG = g_bgui.textG;
					g_bgui.fgB = g_bgui.textB;
				}
			}
		} else {
			bGuiDrawSeparator(left + 3, textY + 6, maxWidth + 0x16,
				g_bgui.menuR, g_bgui.menuG, g_bgui.menuB);
		}
		rowY += 0xe;
		textY += 0xe;
	}
}

/* 0x00047830  BGUI (gui.cpp?)
Draws the menu bar along the top of the screen. The original takes the menu in EDI. */
void bGuiDrawMenuBar(struct BGuiMenu *bar)
{
	int r, g, b, i, x;
	struct BGuiItem *item;

	r = g_bgui.menuR;
	g = g_bgui.menuG;
	b = g_bgui.menuB;
	if ((bar->state & 4) == 0) {
		r = 0xbf;
		g = 0xbf;
		b = 0xbf;
	}
	bGuiFillRect(0, 0x1e, DAT_002bc40c, 0x10, r, g, b);
	g_bgui.transparent = 1;
	g_bgui.fgR = g_bgui.textR;
	g_bgui.fgG = g_bgui.textG;
	g_bgui.fgB = g_bgui.textB;
	x = 5;
	item = bar->items;
	for (i = 0; i < bar->numItems; i++, item++) {
		if (DAT_002bc40c < (int)item->width + x)
			return;
		bGuiDrawText(NULL, x, 0x1f, item->name);
		if ((bar->state & 4) != 0 && bar->selected == i) {
			bGuiDrawBox(x - 3, 0x1f, item->width + 6, 0xe, g_bgui.curMenu != bar,
				g_bgui.menuR, g_bgui.menuG, g_bgui.menuB);
			if (g_bgui.curMenu != bar && item->submenu != NULL)
				bGuiDrawPopup(item->submenu, x - 4, 0x2d);
		}
		x = item->width + 8 + x;
	}
}

/* 0x00047970  BGUI (gui.cpp?)
Moves the focus and selection from the GUI's input channels; calls the callback for a
selected leaf item. The original returns whatever is left in EAX; no caller uses it. */
void bGuiProcessInput(void)
{
	struct BGuiMenu *cur, *sub, *parent;
	struct BGuiItem *item;
	struct BGuiEvent event;
	int sel;

	/* the menu bar: left/right move, down/select open the submenu */
	cur = g_bgui.curMenu;
	if ((cur->flags & 1) == 0) {
		if (g_bgui.right->value != 0) {
			if (cur->numItems != 0)
				cur->selected = (cur->selected + 1) % cur->numItems;
		} else if (g_bgui.left->value != 0) {
			if (cur->numItems != 0 && --cur->selected < 0)
				cur->selected = cur->numItems - 1;
		} else if ((g_bgui.down->value != 0 || g_bgui.select->value != 0) &&
			cur->selected != -1 &&
			(sub = cur->items[cur->selected].submenu) != NULL) {
			g_bgui.curMenu = sub;
			sub->selected = 0;
			em_biResetChannel(g_bgui.select);
			g_bgui.down->value = 0;
		}
	}

	cur = g_bgui.curMenu;
	if (g_bgui.down->value != 0) {
		if (cur->numItems != 0) {
			do {
				cur->selected = (cur->selected + 1) % cur->numItems;
			} while (cur->items[cur->selected].flags & BGUI_ITEM_SEPARATOR);
		}
		return;
	}
	if (g_bgui.up->value != 0) {
		if (cur->numItems == 0)
			return;
		do {
			if (--cur->selected < 0)
				cur->selected = cur->numItems - 1;
		} while (cur->items[cur->selected].flags & BGUI_ITEM_SEPARATOR);
		return;
	}
	if (g_bgui.select->value != 0 && cur->selected != -1) {
		item = &cur->items[cur->selected];
		if (item->submenu != NULL && (item->flags & BGUI_ITEM_DISABLED) == 0) {
			g_bgui.curMenu = item->submenu;
			item->submenu->selected = 0;
			return;
		}
	}
	if (g_bgui.right->value != 0 && (sel = cur->selected) != -1) {
		if ((cur->flags & 1) == 0)
			return;
		item = &cur->items[sel];
		if (item->submenu == NULL) {
			/* a leaf of a popup: open the next menu of the bar */
			parent = cur->parent;
			if (parent == NULL)
				return;
			while (parent->flags & 1)
				parent = parent->parent;
			parent->selected = (parent->selected + 1) % parent->numItems;
			g_bgui.curMenu = parent->items[parent->selected].submenu;
			if (g_bgui.curMenu == NULL) {
				g_bgui.curMenu = parent;
				return;
			}
			g_bgui.curMenu->selected = 0;
			return;
		}
		if ((item->flags & BGUI_ITEM_DISABLED) == 0) {
			g_bgui.curMenu = item->submenu;
			item->submenu->selected = 0;
		}
		return;
	}
	if (g_bgui.deselect->value != 0) {
		if (cur->parent != NULL)
			g_bgui.curMenu = cur->parent;
		return;
	}
	if (g_bgui.left->value != 0 && (parent = cur->parent) != NULL) {
		if ((parent->flags & 1) == 0) {
			/* back to the bar: open the previous menu */
			if (--parent->selected < 0)
				parent->selected = parent->numItems - 1;
			g_bgui.curMenu = parent->items[parent->selected].submenu;
			if (g_bgui.curMenu != NULL) {
				g_bgui.curMenu->selected = 0;
				return;
			}
		}
		g_bgui.curMenu = parent;
		return;
	}
	if (g_bgui.select->value != 0 && (sel = cur->selected) != -1 &&
		cur->items[sel].submenu == NULL && (cur->items[sel].flags & BGUI_ITEM_DISABLED) == 0) {
		event.type = 0;
		event.id = cur->items[cur->selected].id;
		if (g_bgui.callback(&event) != 0) {
			while (g_bgui.select->value != 0 || g_bgui.select->state != 0)
				em_biUpdate();
			g_bgui.exit->value = 1;
		}
	}
}

/* 0x00047cb0  BGUI (gui.cpp?)
Per frame: input, then the menu bar into the locked back buffer. */
void bGuiUpdate(void)
{
	if (g_bgui.active == 0) {
		if (g_bgui.visible == 0)
			return;
	} else if (g_bgui.curMenu != NULL && g_bgui.curMenu->type == 0) {
		bGuiProcessInput();
	}
	bGuiLockScreen();
	if (g_bgui.active != 0 && g_bgui.rootMenu != NULL)
		bGuiDrawMenuBar(g_bgui.rootMenu);
	bGuiUnlockScreen();
	if (g_bgui.exit->value != 0) {
		DAT_002cc1b4 = NULL;
		g_bgui.active = 0;
	}
}

/* 0x00047d20  BGUI (gui.cpp?)
Removes an item (given by its address) from a menu. */
void bGuiRemoveItem(struct BGuiMenu *menu, struct BGuiItem *item)
{
	int index, n;

	index = (int)((char *)item - (char *)menu - 0x14) / (int)sizeof(struct BGuiItem);
	n = menu->numItems;
	if (index > -1 && index < n) {
		if (index < n - 1)
			memmove(&menu->items[index], &menu->items[index + 1],
				(n - index - 1) * sizeof(struct BGuiItem));
		menu->numItems--;
	}
}

/* 0x00047d80  BGUI (gui.cpp?) */
struct BGuiItem *bGuiGetItem(struct BGuiMenu *menu, int index)
{
	if (index > -1 && index < menu->numItems)
		return &menu->items[index];
	return NULL;
}

/* 0x00047da0  BGUI (gui.cpp?) */
struct BGuiItem *bGuiFindItem(struct BGuiMenu *menu, unsigned int id)
{
	int i;

	for (i = 0; i < menu->numItems; i++) {
		if (menu->items[i].id == id)
			return &menu->items[i];
	}
	return NULL;
}

/* 0x00047de0  BGUI (gui.cpp?)
Deletes every top-level menu (submenus go with their parents) and the input map. */
void bGuiShutdown(void)
{
	struct BGuiMenu *menu;

	menu = g_bgui.menus.next;
	while (menu != &g_bgui.menus) {
		if (menu->parent == NULL) {
			bGuiDeleteMenu(menu);
			menu = g_bgui.menus.next;
		} else {
			menu = menu->next;
		}
	}
	FUN_001d5950(); /* an empty function (folded "ret") */
	em_biDeleteMap(g_bgui.inputMap);
}

/* 0x00047e30  BGUI (gui.cpp?)
Binds the GUI channels to the four pads' buttons. */
void bGuiBindDefaultKeys(void)
{
	unsigned int pad;
	int i;

	for (i = 0; i < 4; i++) {
		pad = (unsigned int)i << 0x10;
		em_biBind(g_bgui.left, pad | 0xbf, 0);
		em_biBind(g_bgui.right, pad | 0xc0, 0);
		em_biBind(g_bgui.up, pad | 0xbd, 0);
		em_biBind(g_bgui.down, pad | 0xbe, 0);
		em_biBind(g_bgui.select, pad | 0xc5, 0);
		em_biBind(g_bgui.deselect, pad | 0xc6, 0);
		em_biBind(g_bgui.prior, pad | 0xcb, 0);
		em_biBind(g_bgui.next, pad | 0xcc, 0);
		em_biBind(g_bgui.exit, pad | 0xc1, 0);
	}
	g_bgui.xScale = DAT_002bc46c;
	g_bgui.yScale = DAT_002bc470;
}

/* 0x00047f30  BGUI (gui.cpp?) */
void bGuiLockScreen(void)
{
	int pitch;

	g_bgui.bits = (unsigned int *)em_bdLockSurface((int)DAT_002bc480, 0, 0, 0, 0, &pitch);
	if (g_bgui.bits == NULL) {
		em_bkPrintf("bGuiLockScreen: Lock failure\n");
		return;
	}
	g_bgui.pitch = pitch >> 2;
}

/* 0x00047f80  BGUI (gui.cpp?)
The original passes the surface to 0x001d5950, an empty (folded) function. */
void bGuiUnlockScreen(void)
{
	FUN_001d5950();
}

/* 0x00047f90  BGUI (gui.cpp?)
Draws an 8x12 icon (0: submenu arrow, 1: check mark). */
void bGuiDrawIcon(int icon, int x, int y)
{
	unsigned int fg, bg;
	unsigned int *row;
	const char *bits;
	int i, j;

	fg = BGUI_RGB(g_bgui.fgR, g_bgui.fgG, g_bgui.fgB);
	bg = BGUI_RGB(g_bgui.bgR, g_bgui.bgG, g_bgui.bgB);
	row = BGUI_PIXEL(x, y);
	bits = g_bguiIcons[icon];
	if (g_bgui.xScale != 2) {
		for (i = 0xc; i != 0; i--) {
			for (j = 0; j < 8; j++) {
				if (*bits == '1')
					row[j] = fg;
				else if (g_bgui.transparent == 0)
					row[j] = bg;
				bits++;
			}
			row += g_bgui.pitch;
		}
		return;
	}
	if (g_bgui.yScale == 2) {
		for (i = 0xc; i != 0; i--) {
			for (j = 0; j < 0x10; j += 2) {
				if (*bits == '1' || g_bgui.transparent == 0) {
					row[j + 1] = fg;
					row[j] = fg;
					row[g_bgui.pitch + j] = fg;
					row[g_bgui.pitch + j + 1] = fg;
				}
				bits++;
			}
			row += g_bgui.pitch * 2;
		}
		return;
	}
	for (i = 0xc; i != 0; i--) {
		for (j = 0; j < 0x10; j += 2) {
			if (*bits == '1' || g_bgui.transparent == 0) {
				row[j] = fg;
				row[j + 1] = fg;
			}
			bits++;
		}
		row += g_bgui.pitch;
	}
}

/* 0x000480d0  BGUI (gui.cpp?)
Draws a string with a bitmap font (NULL: the default font). */
void bGuiDrawText(struct BGuiFont *font, int x, int y, const char *text)
{
	unsigned int fg, bg;
	unsigned int *origin, *row, *p;
	const struct BGuiGlyph *glyph;
	const char *bits;
	int len, n, rows, cols, offset;

	if (font == NULL)
		font = &g_bguiFont;
	len = (int)strlen(text);
	fg = BGUI_RGB(g_bgui.fgR, g_bgui.fgG, g_bgui.fgB);
	bg = BGUI_RGB(g_bgui.bgR, g_bgui.bgG, g_bgui.bgB);
	origin = BGUI_PIXEL(x, y);
	offset = 0;
	for (n = 0; n < len; n++) {
		glyph = &font->glyph[text[n] - 0x20];
		bits = glyph->bits;
		if (g_bgui.xScale == 2) {
			if (g_bgui.yScale == 2) {
				/* the original starts this case two pixel rows down */
				row = origin + offset + g_bgui.pitch * 2;
				for (rows = font->height; rows != 0; rows--) {
					p = row;
					for (cols = glyph->width; cols != 0; cols--) {
						if (*bits == '1') {
							p[0] = fg;
							p[1] = fg;
							p[g_bgui.pitch] = fg;
							p[g_bgui.pitch + 1] = fg;
						} else if (g_bgui.transparent == 0) {
							p[0] = bg;
							p[1] = bg;
							p[g_bgui.pitch] = bg;
							p[g_bgui.pitch + 1] = bg;
						}
						bits++;
						p += 2;
					}
					row += g_bgui.pitch * 2;
				}
			} else {
				row = origin + offset + g_bgui.pitch;
				for (rows = font->height; rows != 0; rows--) {
					p = row;
					for (cols = glyph->width; cols != 0; cols--) {
						if (*bits == '1') {
							p[0] = fg;
							p[1] = fg;
						} else if (g_bgui.transparent == 0) {
							p[0] = bg;
							p[1] = bg;
						}
						bits++;
						p += 2;
					}
					row += g_bgui.pitch;
				}
			}
		} else {
			row = origin + offset + g_bgui.pitch;
			for (rows = font->height; rows != 0; rows--) {
				p = row;
				for (cols = glyph->width; cols != 0; cols--) {
					if (*bits == '1')
						*p = fg;
					else if (g_bgui.transparent == 0)
						*p = bg;
					bits++;
					p++;
				}
				row += g_bgui.pitch;
			}
		}
		offset += glyph->width * g_bgui.xScale;
	}
}

/* 0x00048310  BGUI (gui.cpp?) */
void bGuiFillRect(int x, int y, int w, int h, int r, unsigned int g, unsigned int b)
{
	unsigned int *row, *p;
	int rows, cols;

	rows = g_bgui.yScale * h;
	cols = g_bgui.xScale * w;
	row = BGUI_PIXEL(x, y);
	if (rows > 0) {
		do {
			p = row;
			if (cols > 0) {
				int n;
				for (n = cols; n != 0; n--)
					*p++ = BGUI_RGB(r, g, b);
			}
			rows--;
			row += g_bgui.pitch;
		} while (rows != 0);
	}
}

/* 0x00048380  BGUI (gui.cpp?)
Draws a box outline. style 0: raised, 1: sunken, 2: flat (colour), 3: double raised,
4: double sunken. */
void bGuiDrawBox(int x, int y, int w, int h, int style, int r, unsigned int g, unsigned int b)
{
	unsigned int *top, *row2, *p, *q;
	unsigned int light, dark;
	int lr, lg, lb, n, n2;

	if (style == 3) {
		bGuiDrawBox(x, y, w, h, 0, r, g, b);
		if (w > 2 && h > 2)
			bGuiDrawBox(x + 1, y + 1, w - 2, h - 2, 0, r, g, b);
		return;
	}
	if (style == 4) {
		bGuiDrawBox(x, y, w, h, 1, r, g, b);
		if (w > 2 && h > 2)
			bGuiDrawBox(x + 1, y + 1, w - 2, h - 2, 1, r, g, b);
		return;
	}
	top = BGUI_PIXEL(x, y);
	row2 = top + g_bgui.pitch;
	n2 = w * 2;
	if (style < 0)
		return;
	if (style < 2) {
		em_bdShadeColour(r, g, b, &lr, &lg, &lb, style != 0 ? -0x40 : 0x40);
		light = BGUI_RGB(lr, lg, lb);
		em_bdShadeColour(r, g, b, &lr, &lg, &lb, style != 0 ? 0x40 : -0x40);
		dark = BGUI_RGB(lr, lg, lb);
	} else if (style == 2) {
		light = BGUI_RGB(r, g, b);
		dark = light;
	} else {
		return;
	}

	if (g_bgui.xScale == 2) {
		if (g_bgui.yScale == 2) {
			for (n = 0; n < n2; n++) {
				top[n] = light;
				row2[n] = light;
			}
			p = top + g_bgui.pitch * 2;
			if (h * 2 - 1 > 2) {
				for (n = h * 2 - 3; n != 0; n--) {
					p[0] = light;
					p[1] = light;
					p[w * 2 - 2] = dark;
					p[w * 2 - 1] = dark;
					p += g_bgui.pitch;
				}
			}
			q = p + g_bgui.pitch;
			for (n = 0; n < n2; n++) {
				p[n] = dark;
				q[n] = dark;
			}
		} else {
			p = top;
			for (n = 0; n < n2; n++)
				p[n] = light;
			if (h > 2) {
				for (n = h - 2; n != 0; n--) {
					p[0] = light;
					p[1] = light;
					p[w * 2 - 2] = dark;
					p[w * 2 - 1] = dark;
					p += g_bgui.pitch;
				}
			}
			for (n = 0; n < n2; n++)
				p[n] = dark;
		}
	} else {
		p = top;
		for (n = 0; n < w; n++)
			p[n] = light;
		if (h > 1) {
			for (n = h - 1; n != 0; n--) {
				p[0] = light;
				p[w - 1] = dark;
				p += g_bgui.pitch;
			}
		}
		for (n = 0; n < w; n++)
			p[n] = dark;
	}
}

/* 0x00048790  BGUI (gui.cpp?)
Draws a menu separator: one dark and one light line. */
void bGuiDrawSeparator(int x, int y, int w, int r, int g, int b)
{
	unsigned int *row, *p;
	unsigned int light, dark;
	int sr, sg, sb, rows, cols, n, n2;

	row = BGUI_PIXEL(x, y);
	em_bdShadeColour(r, g, b, &sr, &sg, &sb, 0x30);
	light = BGUI_RGB(sr, sg, sb);
	em_bdShadeColour(r, g, b, &sr, &sg, &sb, -0x20);
	dark = BGUI_RGB(sr, sg, sb);
	rows = g_bgui.yScale;
	cols = g_bgui.xScale * w;
	if (rows > 0) {
		for (n = rows; n != 0; n--) {
			p = row;
			for (n2 = 0; n2 < cols; n2++)
				*p++ = dark;
			row += g_bgui.pitch;
		}
		for (n = rows; n != 0; n--) {
			p = row;
			for (n2 = 0; n2 < cols; n2++)
				*p++ = light;
			row += g_bgui.pitch;
		}
	}
}
