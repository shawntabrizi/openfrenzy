/*
CharLib/CharLibShadow.c (carryon_a part)  <-  C:\BlitzParty\Game\Src\CharLib\CharLibShadow.cpp

The shadow projectors of the four players (src/charlib/CharLib/CharLibShadow.c has the
rest of the file). These nine functions lie in front of the CharLibShadow.cpp tag at
0x000be6d0, in the CarryOn block, so the module table gives them to carryon_a; they use
only CharLibShadow's data (charlib's blocks at 0x00293e80-0x00293f14) and the two
counters 0x002f6d20/0x002f6d24 (file inferred). charlib calls 0x000be4a0, 0x000be550,
0x000be5a0 and 0x000be610 by address.

Each frame the maps are rendered for two of the four players (alternating 0-1 and 2-3).
*/

#include "carryon_a.h"

/* charlib's CharLibShadow data (aliases in src/charlib/CharLib/charlib_data.c) */
extern float DAT_00293e94;              /* projector near plane (5) */
extern float DAT_00293e98;              /* projector far plane (5000) */
extern void *DAT_00293ea0[4];           /* the projectors (engine_misc bCamera) */
extern int DAT_00293eb0;                /* the light was set */
extern int DAT_00293eb4;                /* keep the pitch limits */
extern int DAT_00293eb8[4];             /* the projector of each player is in use */
extern float DAT_00293efc;              /* lowest pitch */
extern float DAT_00293f00;              /* highest pitch */
extern float DAT_00293f04;              /* projector distance */
extern int DAT_00293f08;                /* projectors in use */
extern int DAT_00293f0c;                /* passed to every projector after rendering */

/* engine_misc: FUN_001ec260 is an empty bProjector method (ret 4) */
#define co_Projector_Nothing ((void (*)(void *, int))FUN_001ec260)

/* 0x000be3f0  CharLib/CharLibShadow.cpp (file inferred)
Sets the field of view of every projector. */
void Shadows_SetFov(float fovX, float fovY)
{
	int i;

	for (i = 0; i < 4; i++)
		bCamera_SetFov(DAT_00293ea0[i], fovX, fovY, DAT_00293e94, DAT_00293e98);
}

/* 0x000be430  CharLib/CharLibShadow.cpp (file inferred) */
void Shadows_SetDistance(float distance)
{
	DAT_00293f04 = distance;
}

/* 0x000be440  CharLib/CharLibShadow.cpp (file inferred)
Sets the pitch limits and keeps them at the next Shadows_Init. */
void Shadows_SetPitchLimits(float lowest, float highest)
{
	DAT_00293eb4 = 1;
	DAT_00293efc = lowest;
	DAT_00293f00 = highest;
}

/* 0x000be460  CharLib/CharLibShadow.cpp (file inferred)
Deletes the projectors (scalar deleting destructor, vtable slot 0). */
void Shadows_DeleteAll(void)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (DAT_00293ea0[i] != NULL)
			CO_VFN(DAT_00293ea0[i], 0x0, void (*)(void *, int))(DAT_00293ea0[i], 1);
		DAT_00293ea0[i] = NULL;
	}
	DAT_00293eb0 = 0;
	DAT_00293eb4 = 0;
}

/* 0x000be4a0  CharLib/CharLibShadow.cpp (file inferred)
Renders the shadow maps of two players (alternating pairs): each player's actor, with
what it holds (AI_HELD of charlib's AI player) drawn first. */
void Shadows_RenderMaps(void)
{
	void *ai;
	void *held;
	int end;
	int i;

	end = DAT_002f6d20;
	for (i = DAT_002f6d20; i < DAT_002f6d20 + 2; i++)
	{
		if (DAT_00293eb8[i] == 0)
			continue;
		ai = DAT_002f289c[i];
		held = CO_PTR(ai, 0x10c0);
		if (held != NULL)
		{
			bProjector_Render(DAT_00293ea0[i], (unsigned char *)held + 0x10, 7, 3);
			bProjector_Render(DAT_00293ea0[i], (unsigned char *)CO_PTR(DAT_002f289c[i], 0x30) + 0x10, 7, 4);
		}
		else
			bProjector_Render(DAT_00293ea0[i], (unsigned char *)CO_PTR(ai, 0x30) + 0x10, 7, 7);
		end = DAT_002f6d20;
		if (DAT_00293f0c != 0)
		{
			co_Projector_Nothing(DAT_00293ea0[i], DAT_00293f0c);
			end = DAT_002f6d20;
		}
	}
	DAT_002f6d20 = end != 0 ? 0 : 2;
}

/* 0x000be550  CharLib/CharLibShadow.cpp (file inferred)
Applies the projectors in use as texture stages 0.. for the shadow receivers; clears
the next stage. Returns how many. */
int Shadows_ApplyStages(void)
{
	int stage = 0;
	int i;

	for (i = 0; i < 4; i++)
	{
		if (DAT_00293eb8[i] != 0)
		{
			bProjector_Apply(DAT_00293ea0[i], stage, 1);
			stage++;
		}
	}
	if (stage < 4)
		bdSetTexture(stage, NULL);
	return stage;
}

/* 0x000be5a0  CharLib/CharLibShadow.cpp (file inferred)
Restores the stages (projector vtable slot 2). */
void Shadows_RestoreStages(void)
{
	int stage = 0;
	int i;

	for (i = 0; i < 4; i++)
	{
		if (DAT_00293eb8[i] != 0)
		{
			CO_VFN(DAT_00293ea0[i], 0x8, void (*)(void *, int))(DAT_00293ea0[i], stage);
			stage++;
		}
	}
}

/* 0x000be5d0  CharLib/CharLibShadow.cpp (file inferred)
Turns the projector of player `player` on or off (and counts them). */
void Shadows_SetInUse(int player, int on)
{
	if (player >= 4)
		return;
	if (DAT_00293eb8[player] != 0 && on == 0)
		DAT_00293f08 = DAT_00293f08 - 1;
	if (DAT_00293eb8[player] == 0 && on != 0)
		DAT_00293f08 = DAT_00293f08 + 1;
	DAT_00293eb8[player] = on;
}

/* 0x000be610  CharLib/CharLibShadow.cpp (file inferred)
Shadows_RenderMaps with the held objects given by the caller (one model instance per
player, NULL: nothing held). */
void Shadows_RenderMapsFor(void **held)
{
	int end;
	int i;

	end = DAT_002f6d24;
	for (i = DAT_002f6d24; i < DAT_002f6d24 + 2; i++)
	{
		if (DAT_00293eb8[i] == 0)
			continue;
		if (held[i] != NULL)
		{
			bProjector_Render(DAT_00293ea0[i], held[i], 7, 3);
			bProjector_Render(DAT_00293ea0[i], (unsigned char *)CO_PTR(DAT_002f289c[i], 0x30) + 0x10, 7, 4);
		}
		else
			bProjector_Render(DAT_00293ea0[i], (unsigned char *)CO_PTR(DAT_002f289c[i], 0x30) + 0x10, 7, 7);
		end = DAT_002f6d24;
		if (DAT_00293f0c != 0)
		{
			co_Projector_Nothing(DAT_00293ea0[i], DAT_00293f0c);
			end = DAT_002f6d24;
		}
	}
	DAT_002f6d24 = end != 0 ? 0 : 2;
}
