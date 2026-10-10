/*
Meta/CMeta_Seagulls.c  <-  C:\BlitzParty\Game\Src\Meta\CMeta_Seagulls.cpp

Seagulls over the Meta board: a ClassLib CSeagulls flock (classlib_fx) of
DAT_00309244 gulls with the parameters at 0x00309200 (the level loader sets the box and the
count) and the obstacles at 0x00309250.

0x001649d0 is tagged with CMeta_Seagulls.cpp; the update and draw before it and the delete
after it use the same flock (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x00309200  the flock parameters (a CFlockParams, 0x44 bytes) */
float DAT_00309200[0x11];
/* 0x00309244  number of gulls (0: no seagulls) */
int DAT_00309244;
/* 0x00309248  number of obstacles */
int DAT_00309248;
/* 0x0030924c  the flock (CSeagulls, 0x70 bytes) */
void *DAT_0030924c;
/* 0x00309250  the obstacles (0x2c bytes each) */
void *DAT_00309250;

/* 0x00164990  Meta (file inferred: CMeta_Seagulls.cpp)
Per frame: the flock flies. */
void MetaSeagulls_Update(void)
{
	if (DAT_00309244 != 0)
		CSeagulls_Tick((struct CSeagulls *)DAT_0030924c, DAT_0028fda8);
}

/* 0x001649b0  Meta (file inferred: CMeta_Seagulls.cpp)
Draws the flock. */
void MetaSeagulls_Draw(void)
{
	if (DAT_00309244 != 0)
		CSeagulls_Draw(DAT_0030924c);
}

/* 0x001649d0  Meta/CMeta_Seagulls.cpp
Creates the flock: gull.obe, the flight parameters, the obstacles. */
void MetaSeagulls_Init(void)
{
	void *model;
	void *flock;
	int i;

	if (DAT_00309244 == 0)
		return;
	model = bLoadResource(NULL, "gull.obe", 1, 0xdefa);
	DAT_00309200[0x10] = 3.0f;          /* scale */
	DAT_00309200[0] = 0.02f;            /* turn rate */
	DAT_00309200[4] = 0.5f;             /* separation */
	DAT_00309200[1] = 0.05f;            /* bank rate */
	DAT_00309200[2] = 128.0f;           /* neighbour radius */
	DAT_00309200[7] = 2.0f;             /* maximum speed */
	DAT_00309200[6] = 0.5f;             /* minimum speed */
	DAT_00309200[3] = 32.0f;            /* cruise speed */
	DAT_00309200[5] = 0.002f;           /* climb */
	flock = MB_NEW(0x70, "C:\\BlitzParty\\Game\\Src\\Meta\\CMeta_Seagulls.cpp", 0x19);
	if (flock == NULL)
		DAT_0030924c = NULL;
	else
		DAT_0030924c = CSeagulls_Init((struct CSeagulls *)flock, DAT_00309244,
			(struct CFlockParams *)DAT_00309200, (int)model);
	if (DAT_00309248 != 0)
	{
		/* the original adds the address after the last obstacle, every time */
		for (i = 0; i < DAT_00309248; i++)
			CSeagulls_AddObstacle((struct CSeagulls *)DAT_0030924c,
				(int)((char *)DAT_00309250 + DAT_00309248 * 0x2c));
	}
}

/* 0x00164b00  Meta (file inferred: CMeta_Seagulls.cpp)
Deletes the obstacles and the flock. */
void MetaSeagulls_Delete(void)
{
	void *flock;

	if (DAT_00309250 != NULL)
	{
		MB_DELETE(DAT_00309250);
		DAT_00309250 = NULL;
	}
	flock = DAT_0030924c;
	if (DAT_0030924c != NULL)
	{
		CSeagulls_Release((struct CSeagulls *)DAT_0030924c);
		MB_DELETE(flock);
		DAT_0030924c = NULL;
	}
}
