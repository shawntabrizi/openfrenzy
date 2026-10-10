/*
FoxHunt/unplaced_level.c  (module foxhunt; gap functions 0x00136ef0-0x00138790 of
C:\BlitzParty\Game\Src\FoxHunt\, file unknown)

The level objects of Fox Hunt (sharks, breakable glass, escalators and the like, kept in
the lists of the .bss), the text reader of the level scripts (foxhunt.cfg: the same
reader as bumpercars_b's, vtable 0x0026e048), and the CWaterField constructor.
*/

#include "foxhunt.h"

extern const void *const vtable_0026e05c[];   /* classlib_fx: CWaterField */

/* ======================================================================== the text reader */

/* 0x0026e048  vtable of the text reader */
const void *const vtable_0026e048[1] = {
	(const void *)FUN_00138770, /* [0] scalar deleting destructor */
};

/* 0x001386a0  (gap): the text reader's constructor (layout: struct BCarTextReader of
bumpercars_b: +0x4 buffer, +0x8 cursor, +0xc end, +0x10 name, +0x210 line) */
void FoxHunt_TextReader_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026e048;
}

/* 0x001386b0  (gap): the text reader's destructor */
void FoxHunt_TextReader_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026e048;
}

/* 0x001386c0  (gap): copies the line at the cursor into `buffer` (without the line end)
and steps over one character; returns the line without its leading blanks */
char *FoxHunt_TextReader_ReadLine(void *self, char *buffer)
{
	char *out = buffer;
	char c;

	if (FH_AP(self, 0x8) < FH_AP(self, 0xc)) {
		do {
			c = *FH_AP(self, 0x8);
			if (c == '\n' || c == '\r')
				break;
			*out++ = c;
			FH_AP(self, 0x8)++;
		} while (FH_AP(self, 0x8) < FH_AP(self, 0xc));
	}
	*out = 0;
	FH_AP(self, 0x8)++;
	while (*buffer != 0 && (*buffer == ' ' || *buffer == '\r' || *buffer == '\n' || *buffer == '\t'))
		buffer++;
	return buffer;
}

/* 0x00138720  (gap): moves the cursor to the start of the next line and counts it */
void FoxHunt_TextReader_NextLine(void *self)
{
	char *end = FH_AP(self, 0xc);
	char c;

	if (FH_AP(self, 0x8) < end) {
		do {
			c = *FH_AP(self, 0x8);
			if (c == '\n' || c == '\r')
				break;
			FH_AP(self, 0x8)++;
		} while (FH_AP(self, 0x8) < FH_AP(self, 0xc));
		if (FH_AP(self, 0x8) < end) {
			do {
				c = *FH_AP(self, 0x8);
				if (c != '\n' && c != '\r')
					break;
				FH_AP(self, 0x8)++;
			} while (FH_AP(self, 0x8) < FH_AP(self, 0xc));
		}
	}
	FH_AI(self, 0x210)++;
}

/* 0x00138770  (gap): the text reader's scalar deleting destructor */
void *FoxHunt_TextReader_Destroy(void *self, unsigned char flags)
{
	*(const void *const **)self = vtable_0026e048;
	if ((flags & 1) != 0)
		FH_DELETE(self);
	return self;
}

/* ======================================================================== CWaterField */

/* 0x00138790  (gap; the class is classlib_fx's CWaterField, vtable 0x0026e05c):
CWaterField::CWaterField: clears the buffers and the 0x2400 words of edge and cell data */
void *FoxHunt_CWaterField_Ctor(void *self)
{
	unsigned int *w = (unsigned int *)self;
	int i;

	w[0x241e] = 0;
	w[0x2422] = 0;
	w[0x2421] = 0;
	w[0x11] = 0;
	w[0x2423] = 0;
	*(const void *const **)self = vtable_0026e05c;
	for (i = 0; i < 0x2400; i++)
		w[0x12 + i] = 0;
	return self;
}
