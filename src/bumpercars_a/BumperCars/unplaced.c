/*
BumperCars/unplaced.c  (module bumpercars_a)

Gap functions of the BumperCars address range whose source file is not known.

0x00082860-0x00082870: the command line settings table (core owns the object, at the
pointer DAT_002ba9a0). 0x00082a00-0x00082b20: the four colours of the debris/effects
globals at 0x002ed350 (used by many minigames).
*/

#include "bumpercars_a.h"

/* the settings table: name/value pairs (0x5804 bytes) */
struct SettingsEntry {
	char name[0x20];   /* 0x00 */
	char value[0x90];  /* 0x20 */
};
struct Settings {
	struct SettingsEntry entries[0x80]; /* 0x0000 */
	int count;                          /* 0x5800 */
};
OF_ASSERT_SIZE(struct Settings, 0x5804);

/* 0x00082860  (gap): the settings table's constructor (empty table) */
struct Settings *Settings_Init(struct Settings *self)
{
	self->count = 0;
	return self;
}

/* 0x00082870  (gap): the value of a setting (case-insensitive name), or NULL */
char *Settings_Get(struct Settings *self, const char *key)
{
	int i;

	for (i = 0; i < self->count; i++) {
		if (strcasecmp(self->entries[i].name, key) == 0)
			return self->entries[i].value;
	}
	return NULL;
}

/* 0x00082a00  (gap): clears the four colours and their flags */
void DebrisColours_Clear(void)
{
	memset(DAT_002ed350, 0, 0x14 * 4);
}

/* 0x00082a20  (gap): sets colour 0 */
void DebrisColours_Set0(float *colour)
{
	DAT_002ed350[0][0] = colour[0];
	DAT_002ed350[0][1] = colour[1];
	DAT_002ed350[0][2] = colour[2];
	DAT_002ed350[0][3] = colour[3];
	DAT_002ed390[0] = 1;
}

/* 0x00082a60  (gap): sets colour 1 */
void DebrisColours_Set1(float *colour)
{
	DAT_002ed350[1][0] = colour[0];
	DAT_002ed350[1][1] = colour[1];
	DAT_002ed350[1][2] = colour[2];
	DAT_002ed350[1][3] = colour[3];
	DAT_002ed390[1] = 1;
}

/* 0x00082aa0  (gap): sets colour 2 */
void DebrisColours_Set2(float *colour)
{
	DAT_002ed350[2][0] = colour[0];
	DAT_002ed350[2][1] = colour[1];
	DAT_002ed350[2][2] = colour[2];
	DAT_002ed350[2][3] = colour[3];
	DAT_002ed390[2] = 1;
}

/* 0x00082ae0  (gap): sets colour 3 */
void DebrisColours_Set3(float *colour)
{
	DAT_002ed350[3][0] = colour[0];
	DAT_002ed350[3][1] = colour[1];
	DAT_002ed350[3][2] = colour[2];
	DAT_002ed350[3][3] = colour[3];
	DAT_002ed390[3] = 1;
}

/* 0x00082b20  (gap): sets all four colours */
void DebrisColours_SetAll(float *colour0, float *colour1, float *colour2, float *colour3)
{
	DebrisColours_Set0(colour0);
	DebrisColours_Set1(colour1);
	DebrisColours_Set2(colour2);
	DebrisColours_Set3(colour3);
}
