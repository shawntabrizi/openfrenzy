/*
CTWEAKMENU.C  <-  C:\BlitzParty\Game\Src\Core\CTweakMenu.cpp

openfrenzy core module: the debug "tweak" menus. A CTweakMenu is a page of the debug menu
(engine_misc menus, 0x00047370) with up to 32 items; an item edits an int, a bool or a float
in place. Values can be loaded from and saved to a text "tweak file" ("[item_menu] value").
Decompiled from orig/default.xbe (functional, not byte-matching).

0x0002e290-0x0002ed90 and 0x0002f3e0 are in the gap between CSpinCube.cpp and
CTweakMenu.cpp in the source map; they are CTweakMenu methods (they use its layout and the
menu list at 0x002bae5c), so they are placed here.
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"
#include "core_tweak.h"

static const char tweak_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CTweakMenu.cpp";

/* the tweak-file reader state */
#define g_TweakLabels CORE_VAR(const char **, 0x002bae48)  /* passed when an item has no labels */
#define g_TweakCursor CORE_VAR(char *, 0x002bae4c)
#define g_TweakSize CORE_VAR(int, 0x002bae50)
#define g_TweakFile CORE_VAR(FILE *, 0x002bae54)
#define g_TweakBuffer CORE_VAR(void *, 0x002bae58)
#define g_TweakMenus CORE_ADDR(0x002bae5c)            /* list of all CTweakMenu */
#define g_TweakMenuCount CORE_VAR(int, 0x002bae64)
#define g_TweakNoYes ((const char **)CORE_ADDR(0x00284490))  /* "NO", "YES" */
#define g_TweakLoad CORE_VAR(int, 0x00284498)
#define g_TweakSave CORE_VAR(int, 0x0028449c)
#define g_SoundVolume CORE_VAR(int, 0x002bae6c)

static int is_tweak_space(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || (unsigned char)c == 0xff || c == '\r';
}

/* 0x0002e290  (Core, CTweakMenu.cpp) skip COUNT words of *CURSOR, and the spaces after each */
void CTweakMenu_SkipWords(char **cursor, int count)
{
	char c;

	for (; count != 0; count--)
	{
		if (**cursor != ' ')
		{
			for (;;)
			{
				c = **cursor;
				if (c == '\t' || c == '\n' || (unsigned char)c == 0xff || c == '\r')
					break;
				(*cursor)++;
				if (**cursor == ' ')
					break;
			}
		}
		while (is_tweak_space(**cursor))
			(*cursor)++;
	}
}

/* 0x0002e2f0  (Core, CTweakMenu.cpp) write every item to the open tweak file, close it, and
free the loaded file */
void CTweakMenu_SaveAll(void)
{
	char key[0x44];
	char copy[0x40];
	char value[0x40];
	struct CTweakMenu *menu;
	int i;
	char *p;

	if (g_TweakFile != NULL)
	{
		if (g_TweakSave != 0)
		{
			for (menu = ext_ListNext(g_TweakMenus, NULL); menu != NULL;
				menu = ext_ListNext(g_TweakMenus, menu))
			{
				for (i = 0; i < menu->count; i++)
				{
					struct CTweakItem *item = menu->items[i];

					sprintf(key, "[%s_%s]\n", item->name, menu->name);
					strcpy(copy, key);
					for (p = copy; *p != '\0'; p++)
					{
						if (*p == ' ')
							*p = '_';
					}
					strcpy(key, copy);
					fwrite(key, strlen(key), 1, g_TweakFile);
					item = menu->items[i];
					if (item->ivar != NULL)
						sprintf(value, "%d\n", *item->ivar);
					else if (item->fvar != NULL)
						sprintf(value, "%f\n", (double)*item->fvar);
					else if (item->bvar != NULL)
						sprintf(value, "%d\n", *item->bvar);
					else
						continue;
					fwrite(value, strlen(value), 1, g_TweakFile);
				}
			}
		}
		if (g_TweakFile != NULL)
			fclose(g_TweakFile);
	}
	if (g_TweakBuffer != NULL)
	{
		ext_Delete(g_TweakBuffer);
		g_TweakBuffer = NULL;
	}
	g_TweakCursor = NULL;
}

/* list head init, the item list at +0xdc, and the menu entry: shared by both constructors */
static void CTweakMenu_Init(struct CTweakMenu *self, const char *name, void *parent_menu, int is_top)
{
	int offset;

	ext_ListInit(self->list);
	self->menu = ext_MenuCreate(1);
	if (self->menu != NULL)
	{
		unsigned char *entries = self->menu;

		for (offset = 0; offset < 0x580; offset += 0x2c)
		{
			*(uint32_t *)(entries + offset + 0x34) = 0;
			*(uint8_t *)(entries + offset + 0x14) = 0;
			*(uint16_t *)(entries + offset + 0x32) = 0;
			*(uint16_t *)(entries + offset + 0x38) = 0xffff;
		}
		self->id = g_TweakMenuCount + 0x401;
		self->id_base = g_TweakMenuCount * 0x400 + 0x465;
		strcpy(self->name, name);
		ext_MenuAdd(parent_menu, name, self->id, self->menu, 0);
	}
	self->is_top = is_top;
	self->count = 0;
	ext_ListAdd(g_TweakMenus, self, 0);
}

/* 0x0002e510  (Core, CTweakMenu.cpp) CTweakMenu::CTweakMenu(name): a page of the root
debug menu (0x002ba988) */
struct CTweakMenu *CTweakMenu_Ctor(struct CTweakMenu *self, const char *name)
{
	CTweakMenu_Init(self, name, g_DebugMenu, 1);
	return self;
}

/* 0x0002e610  (Core, CTweakMenu.cpp) CTweakMenu::CTweakMenu(parent, name): a sub-page */
struct CTweakMenu *CTweakMenu_CtorSub(struct CTweakMenu *self, struct CTweakMenu *parent, const char *name)
{
	CTweakMenu_Init(self, name, parent->menu, 0);
	return self;
}

/* 0x0002e710  (Core, CTweakMenu.cpp) load the value of item NAME from the tweak file
("[name_menu] value"): into FVAR ("%f"), else IVAR ("%d"), else BVAR ("%d") */
void CTweakMenu_LoadValue(struct CTweakMenu *self, const char *name, int *size, float *fvar,
	int *ivar, int *bvar)
{
	char key[0x100];
	char word[0x100];
	char *start = g_TweakCursor;
	char *end;
	char *p;

	if (start == NULL || g_TweakLoad == 0)
		return;
	sprintf(key, "[%s_%s]", name, self->name);
	for (p = key; *p != '\0'; p++)
	{
		if (*p == ' ')
			*p = '_';
	}
	end = start + *size;
	while (g_TweakCursor < end)
	{
		int words;

		while (*g_TweakCursor == ' ' || *g_TweakCursor == '\t' || *g_TweakCursor == '\r'
			|| *g_TweakCursor == '\n')
			g_TweakCursor++;
		if (g_TweakCursor >= end)
			break;
		words = sscanf(g_TweakCursor, "%s", word);
		CTweakMenu_SkipWords(&g_TweakCursor, words);
		if (g_TweakCursor > end)
			break;
		if (fvar != NULL)
		{
			if (strcmp(word, key) == 0)
			{
				sscanf(g_TweakCursor, "%f", fvar);
				break;
			}
		}
		else if (ivar != NULL)
		{
			if (strcmp(word, key) == 0)
			{
				sscanf(g_TweakCursor, "%d", ivar);
				break;
			}
		}
		else if (bvar != NULL)
		{
			if (strcmp(word, key) == 0)
			{
				sscanf(g_TweakCursor, "%d", bvar);
				break;
			}
		}
	}
	g_TweakCursor = start;
}

/* the "Up / Down / Reset / Up More / Down More" sub-menu of an int or float item */
static void CTweakItem_AddStepMenu(struct CTweakItem *self)
{
	int id = self->menu->count * 0x20 + self->menu->id_base;

	ext_MenuAdd(self->submenu, "Up", id, NULL, 0);
	ext_MenuAdd(self->submenu, "Down", id + 1, NULL, 0);
	ext_MenuAdd(self->submenu, "Reset", id + 2, NULL, 0);
	ext_MenuAdd(self->submenu, "Up More", id + 3, NULL, 0);
	ext_MenuAdd(self->submenu, "Down More", id + 4, NULL, 0);
}

/* 0x0002e910  (Core, CTweakMenu.cpp) CTweakItem for a float */
struct CTweakItem *CTweakItem_CtorFloat(struct CTweakItem *self, struct CTweakMenu *menu,
	const char *name, float *var, float step, float min, float max)
{
	self->max = max;
	self->step = step;
	self->menu = menu;
	self->fvar = var;
	self->labels[0] = NULL;
	self->bvar = NULL;
	self->ivar = NULL;
	self->min = min;
	self->reset = *var;
	strcpy(self->name, name);
	sprintf(self->text, "%s: %f  ", self->name, (double)*self->fvar);
	self->submenu = ext_MenuCreate(1);
	CTweakItem_AddStepMenu(self);
	ext_MenuAdd(self->menu->menu, self->text, 0, self->submenu, 0);
	return self;
}

/* 0x0002eaa0  (Core, CTweakMenu.cpp) CTweakItem for an int, optionally with NLABELS names */
struct CTweakItem *CTweakItem_CtorInt(struct CTweakItem *self, struct CTweakMenu *menu,
	const char *name, int *var, int step, int min, int max, int nlabels, const char **labels)
{
	int i, width = 0;

	self->step = (float)step;
	self->max = (float)max;
	self->ivar = var;
	self->labels[0] = NULL;
	self->bvar = NULL;
	self->fvar = NULL;
	self->min = (float)min;
	self->menu = menu;
	self->reset = (float)*var;
	strcpy(self->name, name);
	sprintf(self->text, "%s: %d  ", self->name, *self->ivar);
	self->submenu = ext_MenuCreate(1);
	if (nlabels == 0)
		CTweakItem_AddStepMenu(self);
	else
	{
		/* one entry per label: choosing it sets the value; pad the text to the longest */
		for (i = 0; i < nlabels; i++)
		{
			int length;

			self->labels[i] = labels[i];
			ext_MenuAdd(self->submenu, self->labels[i],
				self->menu->count * 0x20 + self->menu->id_base + i, NULL, 0);
			length = (int)strlen(self->labels[i]);
			if (width < length)
				width = length;
		}
		{
			size_t end = strlen(self->text);

			if (width > 0)
				memset(self->text + end, ' ', (size_t)width);
			self->text[end + (width > 0 ? width : 0)] = '\0';
		}
	}
	ext_MenuAdd(self->menu->menu, self->text, 0, self->submenu, 0);
	return self;
}

/* 0x0002ece0  (Core, CTweakMenu.cpp) CTweakItem for a bool (YES/NO) */
struct CTweakItem *CTweakItem_CtorBool(struct CTweakItem *self, struct CTweakMenu *menu,
	const char *name, int *var)
{
	self->bvar = var;
	self->labels[0] = NULL;
	self->ivar = NULL;
	self->fvar = NULL;
	self->menu = menu;
	strcpy(self->name, name);
	if (*self->bvar != 0)
		*self->bvar = 1;
	sprintf(self->text, "%s: %s  ", self->name, g_TweakNoYes[*self->bvar]);
	ext_MenuAdd(self->menu->menu, self->text, self->menu->count * 0x20 + self->menu->id_base, NULL, 0);
	return self;
}

/* the item text after a change */
static void CTweakItem_FormatValue(struct CTweakItem *item)
{
	if (item->bvar != NULL)
		sprintf(item->text, "%s: %s", item->name, g_TweakNoYes[*item->bvar]);
	else if (item->ivar != NULL)
	{
		if (item->labels[0] != NULL)
			sprintf(item->text, "%s: %s", item->name, item->labels[*item->ivar]);
		else
			sprintf(item->text, "%s: %d", item->name, *item->ivar);
	}
	else
		sprintf(item->text, "%s: %f", item->name, (double)*item->fvar);
}

/* 0x0002ed90  (Core, CTweakMenu.cpp) a debug menu entry was chosen (SELECTED != 0): apply it
to its tweak item and return 1. SELECTED == 0: refresh every item text; returns 0. */
int CTweakMenu_HandleSelect(int id, int selected)
{
	struct CTweakMenu *menu;

	ext_SetSoundVolume(g_SoundVolume);
	for (menu = ext_ListNext(g_TweakMenus, NULL); menu != NULL; menu = ext_ListNext(g_TweakMenus, menu))
	{
		int index, action;
		struct CTweakItem *item;

		if (selected == 0)
		{
			for (index = 0; index < menu->count; index++)
			{
				item = menu->items[index];
				CTweakItem_FormatValue(item);
				strcpy(ext_MenuItemText(menu->menu, index), item->text);
			}
			continue;
		}
		if (id < menu->id_base || id >= menu->count * 0x20 + menu->id_base)
			continue;
		index = (id - menu->id_base) / 32;
		action = (id - menu->id_base) % 32;
		item = menu->items[index];
		if (item->labels[0] != NULL)
			*item->ivar = action;
		else
		{
			switch (action)
			{
			case 0: /* Up (or toggle a bool) */
				if (item->bvar != NULL)
					*item->bvar = 1 - *item->bvar;
				else if (item->ivar != NULL)
				{
					*item->ivar += (int)item->step;
					if (*item->ivar > (int)item->max)
						*item->ivar = (int)item->max;
				}
				else
				{
					*item->fvar = item->step + *item->fvar;
					if (*item->fvar > item->max)
						*item->fvar = item->max;
				}
				break;
			case 1: /* Down */
				if (item->ivar != NULL)
				{
					*item->ivar -= (int)item->step;
					if (*item->ivar < (int)item->min)
						*item->ivar = (int)item->min;
				}
				else
				{
					*item->fvar = *item->fvar - item->step;
					if (*item->fvar < item->min)
						*item->fvar = item->min;
				}
				break;
			case 2: /* Reset */
				if (item->ivar != NULL)
					*item->ivar = (int)item->reset;
				else
					*item->fvar = item->reset;
				break;
			case 3: /* Up More */
				if (item->ivar != NULL)
				{
					*item->ivar += (int)item->step * 10;
					if (*item->ivar > (int)item->max)
						*item->ivar = (int)item->max;
				}
				else
				{
					*item->fvar = item->step * 10.0f + *item->fvar;
					if (*item->fvar > item->max)
						*item->fvar = item->max;
				}
				break;
			case 4: /* Down More */
				if (item->ivar != NULL)
				{
					*item->ivar += (int)item->step * -10;
					if (*item->ivar < (int)item->min)
						*item->ivar = (int)item->min;
				}
				else
				{
					*item->fvar = *item->fvar - item->step * 10.0f;
					if (*item->fvar < item->min)
						*item->fvar = item->min;
				}
				break;
			default:
				break;
			}
		}
		CTweakItem_FormatValue(item);
		strcpy(ext_MenuItemText(menu->menu, index), item->text);
		return 1;
	}
	return 0;
}

static void CTweakMenu_Append(struct CTweakMenu *self, struct CTweakItem *item)
{
	if (item != NULL)
	{
		self->items[self->count] = item;
		self->count++;
	}
}

/* 0x0002f1a0  Core/CTweakMenu.cpp  add a float item */
struct CTweakItem *CTweakMenu_AddFloat(struct CTweakMenu *self, const char *name, float *var,
	float step, float min, float max)
{
	struct CTweakItem *item;

	CTweakMenu_LoadValue(self, name, &g_TweakSize, var, NULL, NULL);
	item = ext_New(0x1a4, tweak_cpp, 0x168, 0x1001);
	if (item == NULL)
		return NULL;
	item = CTweakItem_CtorFloat(item, self, name, var, step, min, max);
	CTweakMenu_Append(self, item);
	return item;
}

/* 0x0002f250  Core/CTweakMenu.cpp  add an int item; NLABELS names follow (const char *) */
void CTweakMenu_AddInt(struct CTweakMenu *self, const char *name, int *var, int step, int min,
	int max, int nlabels, ...)
{
	const char *labels[32];
	struct CTweakItem *item;
	va_list arguments;
	int i;

	CTweakMenu_LoadValue(self, name, &g_TweakSize, NULL, var, NULL);
	if (nlabels == 0)
	{
		item = ext_New(0x1a4, tweak_cpp, 0x188, 0x1001);
		if (item != NULL)
			item = CTweakItem_CtorInt(item, self, name, var, step, min, max, 0, g_TweakLabels);
	}
	else
	{
		item = ext_New(0x1a4, tweak_cpp, 0x184, 0x1001);
		if (item != NULL)
		{
			va_start(arguments, nlabels);
			for (i = 0; i < nlabels && i < 32; i++)
				labels[i] = va_arg(arguments, const char *);
			va_end(arguments);
			item = CTweakItem_CtorInt(item, self, name, var, step, min, max, nlabels, labels);
		}
	}
	CTweakMenu_Append(self, item);
}

/* 0x0002f340  Core/CTweakMenu.cpp  add a bool item */
struct CTweakItem *CTweakMenu_AddBool(struct CTweakMenu *self, const char *name, int *var)
{
	struct CTweakItem *item;

	CTweakMenu_LoadValue(self, name, &g_TweakSize, NULL, NULL, var);
	item = ext_New(0x1a4, tweak_cpp, 0x19c, 0x1001);
	if (item == NULL)
		return NULL;
	item = CTweakItem_CtorBool(item, self, name, var);
	CTweakMenu_Append(self, item);
	return item;
}

/* 0x0002f3e0  (Core, CTweakMenu.cpp) CTweakMenu::~CTweakMenu */
void CTweakMenu_Dtor(struct CTweakMenu *self)
{
	int i;

	ext_ListRemove(g_TweakMenus, self);
	if (self->menu != NULL)
	{
		ext_MenuRemove(g_DebugMenu, ext_MenuFind(g_DebugMenu, self->id));
		if (self->is_top != 0)
			ext_MenuFree(self->menu);
	}
	for (i = 0; i < self->count; i++)
	{
		if (self->items[i] != NULL)
			ext_Delete(self->items[i]);
	}
	ext_Nothing(self->list);   /* the item list's destructor is empty */
}
