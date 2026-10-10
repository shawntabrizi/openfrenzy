/*
BumperCars/CBCar_FloatyScore.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_FloatyScore.cpp

The floaty scores: "+5" / "-2" texts in the scorer's colour that rise from where points
were won or lost (one object, its pointer at 0x002f138c). 0x000a0c30 is tagged with
CBCar_FloatyScore.cpp; the other methods around it are gap functions of the same class.
*/

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"

/* 0x000a0b70  (gap; file inferred)
Moves the texts by `dt`; they face the camera heading `angle`. */
void BCarFloaty_Update(struct BCarFloaty *self, float dt, float angle)
{
	unsigned int bits;

	if (self->system == NULL)
		return;
	self->system->angle = angle;
	/* the update callback receives dt's 32 bits through the extra system's argument */
	memcpy(&bits, &dt, sizeof bits);
	baUpdateExtras(self->system->system, (void *)(uintptr_t)bits);
}

/* 0x000a0ba0  (gap; file inferred) */
void BCarFloaty_Draw(struct BCarFloaty *self)
{
	if (self->system != NULL)
		baDrawExtras(self->system->system, NULL);
}

/* 0x000a0bc0  (gap; file inferred)
Adds the text "%+d" of `points` at `pos` in the colour (r, g, b). */
void BCarFloaty_Add(struct BCarFloaty *self, float *pos, int points, int r, int g, int b)
{
	char text[12];

	if (self->system == NULL)
		return;
	_snprintf(text, 10, "%+d", points);                          /* 0x0026619c */
	baAddExtraItem(self->system->system, self->font, (double)pos[0], (double)pos[1],
		(double)pos[2], r, g, b, text);
}

/* 0x000a0c30  BumperCars/CBCar_FloatyScore.cpp
Constructor: the data block, the font and the extra system of `maxItems` texts, at `scale`
times the normal size. */
struct BCarFloaty *BCarFloaty_Construct(struct BCarFloaty *self, int maxItems,
	float scale)
{
	self->system = (struct BCarFloatyScore *)BCB_NEW(0x90,
		"C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_FloatyScore.cpp", 0x1c);
	if (self->system != NULL)
	{
		self->font = bLoadResource(NULL, "handgotl.ttf", 3, 0xdefa);  /* 0x00253e78 */
		BCarFloatyScore_Init(self->system, maxItems);
		self->system->scale = scale * self->system->scale;
	}
	return self;
}

/* 0x000a0c90  (gap; file inferred)
Destructor: the extra system and the data block. */
void BCarFloaty_Destruct(struct BCarFloaty *self)
{
	if (self->system != NULL)
	{
		baDeleteExtraSystem(self->system->system);
		BCB_DELETE(self->system);
		self->system = NULL;
	}
}
