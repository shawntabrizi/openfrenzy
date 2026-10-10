/*
CarryOn/CFire_Resource.c  <-  C:\BlitzParty\Game\Src\CarryOn\CFire_Resource.cpp

The resources of the Fireworks game (struct CFireResource, g_fireRes): fonts, the floor
models of the variant, the 46 sounds (grouped by kind), the fog, the frame timer, and the
debug drawing and text helpers that the other CarryOn files use.

0x000c6f30 is tagged with CFire_Resource.cpp; the functions after it up to 0x000c8230 work
on the same object or are its static helpers (file inferred). The original guards the
constructor and destructor with C++ exception frames; they are left out.
*/

#include <stdarg.h>
#include "carryon_a.h"

/* a vertex colour from 0xAARRGGBB through bdisplay's channel tables */
static unsigned int co_colour(unsigned int a, unsigned int r, unsigned int g, unsigned int b)
{
	return DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bc898[a] | DAT_002bce18[r];
}

static unsigned int co_colour32(unsigned int colour)
{
	return co_colour(colour >> 24, (colour >> 16) & 0xff, (colour >> 8) & 0xff, colour & 0xff);
}

/* the floor models of each variant (level package) */
static const char *const s_floorVariant2[5] = {
	"objects\\floor_lightning.obe", "objects\\floor_comet.obe", "objects\\floor_temper.obe",
	"objects\\floor_pandemonium.obe", "objects\\floor_magic.obe",
};
static const char *const s_potVariant2[5] = {
	"objects\\pot_lightning.obe", "objects\\pot_comet.obe", "objects\\pot_temper.obe",
	"objects\\pot_pandemonium.obe", "objects\\pot_magic.obe",
};
static const char *const s_rackVariant3[7] = {
	"objects\\rack_rocket_green.obe", "objects\\rack_rocket_red.obe",
	"objects\\rack_rocket_blue.obe", "objects\\rack_rocket_yellow.obe",
	"objects\\rack_rocket_orange.obe", "objects\\rack_rocket_pink.obe",
	"objects\\carnival_special.obe",
};

/* a rumble effect's fields (classlib_ui CLRumble) */
static void co_SetBang(int effect, int strength)
{
	CO_I32((void *)(uintptr_t)effect, 0x0c) = 0;
	CO_I32((void *)(uintptr_t)effect, 0x28) = 1;
	CO_I32((void *)(uintptr_t)effect, 0x2c) = strength;
	CO_I32((void *)(uintptr_t)effect, 0x30) = 200;
	CO_I32((void *)(uintptr_t)effect, 0x34) = 0;
	CO_I32((void *)(uintptr_t)effect, 0x38) = 100;
	CO_I32((void *)(uintptr_t)effect, 0x3c) = 0;
	CO_I32((void *)(uintptr_t)effect, 0x40) = 0;
}

/* 0x000c6f30  CarryOn/CFire_Resource.cpp
Loads the fonts, reads the settings (detail, debuglines, testmesh, difficulty, allhuman),
creates the "Detail" input, loads the floor models of the variant with their instances,
the 46 sounds (counting the sounds of each kind) and the rumble effects. */
struct CFireResource *CFire_Resource_Ctor(struct CFireResource *self, void *levelPackage,
	void *genericPackage, int variant, int numPlayers)
{
	const struct CoSoundDef *def;
	char path[40];
	char *value;
	int effect;
	int last;
	int kind;
	unsigned int i;

	co_Timer_Ctor(self->timer);
	memset(self, 0, 0xf8 * 4);
	co_Timer_Init(self->timer);
	self->font = bLoadResource(genericPackage, "gunship2.ttf", 3, 0);
	self->fontAlt = bLoadResource(NULL, "handgotb.ttf", 3, 0);
	self->levelPackage = levelPackage;
	self->genericPackage = genericPackage;

	value = Settings_Get(DAT_002ba9a0, "detail");
	DAT_00294118 = value == NULL ? 100 : atol(value);
	DAT_0029411c = 1;
	value = Settings_Get(DAT_002ba9a0, "debuglines");
	DAT_002f6d60 = value == NULL ? 0 : atol(value);
	value = Settings_Get(DAT_002ba9a0, "testmesh");
	self->testMesh = value == NULL ? 0 : atol(value);
	if (variant != 1)
		self->testMesh = 0;
	value = Settings_Get(DAT_002ba9a0, "difficulty");
	if (value != NULL)
		DAT_002bae70 = atol(value);
	value = Settings_Get(DAT_002ba9a0, "allhuman");
	if (value != NULL && atol(value) == 1)
	{
		DAT_002baed4[0][1] = 1;
		DAT_002baed4[1][1] = 1;
		DAT_002baed4[2][1] = 1;
		DAT_002baed4[3][1] = 1;
	}
	self->variant = variant;
	self->numPlayers = numPlayers;
	self->inputMap = biCreateMap(1);
	self->detailChannel = biCreateChannel(self->inputMap, "Detail", 1, 1, 0, 0, 0);
	self->scoreFont = bLoadResource(genericPackage, "score.ttf", 3, 0xdefa);

	if (variant == 0)
		self->floorModel[0] = bLoadResource(levelPackage, "objects\\floor_rocket.obe", 1, 0xdefa);
	else if (variant == 2)
	{
		for (i = 0; i < 5; i++)
			self->floorModel[i] = bLoadResource(levelPackage, s_floorVariant2[i], 1, 0xdefa);
		for (i = 0; i < 5; i++)
			self->potModel[i] = bLoadResource(levelPackage, s_potVariant2[i], 1, 0xdefa);
	}
	else if (variant == 3)
	{
		for (i = 0; i < 7; i++)
			self->floorModel[i] = bLoadResource(levelPackage, s_rackVariant3[i], 1, 0xdefa);
	}

	for (i = 0; i < 7; i++)
	{
		if (self->floorModel[i] != NULL)
		{
			self->floorInst[i] = baInitActorInstance(NULL, self->floorModel[i], 2,
				CO_FILE_RESOURCE, 0x131);
			memcpy(CO_FP(self->floorInst[i], 0xc0), DAT_00294310, 4 * sizeof(float));
			self->floorAnim[i] = baFindSubObject(self->floorInst[i], "default");
		}
	}
	for (i = 0; i < 5; i++)
	{
		if (self->potModel[i] != NULL)
		{
			self->potInst[i] = baInitActorInstance(NULL, self->potModel[i], 2,
				CO_FILE_RESOURCE, 0x13b);
			memcpy(CO_FP(self->potInst[i], 0xc0), DAT_00294310, 4 * sizeof(float));
		}
	}

	self->samples = bkHeapAlloc(0x2e * 4, CO_FILE_RESOURCE, 0x142, 0x2001);
	last = -1;
	for (i = 0; i < 0x2e; i++)
	{
		def = &DAT_002941a0[i];
		sprintf(path, def->looped ? "loopedsounds\\%s.wav" : "sounds\\%s.wav", def->name);
		self->samples[i] = bLoadResource(def->generic ? genericPackage : levelPackage, path, 2,
			0xdefa);
		kind = def->kind;
		if (last == kind)
			self->numSamples[kind]++;
		else
		{
			self->numSamples[kind] = 1;
			self->firstSample[kind] = (int)i;
			last = kind;
		}
	}

	CLRumble_CreateMenu((int *)DAT_002ed324);
	CLRumble_AddEffect((int *)DAT_002ed324, "FWPunches");
	CLRumble_AddEffect((int *)DAT_002ed324, "FWPunched");
	CLRumble_AddEffect((int *)DAT_002ed324, "FWKO-ed");
	effect = CLRumble_AddEffect((int *)DAT_002ed324, "FWRocketBang");
	co_SetBang(effect, 0x4000);
	effect = CLRumble_AddEffect((int *)DAT_002ed324, "FWFactoryBang");
	co_SetBang(effect, 0x7fff);
	CLRumble_AddEffect((int *)DAT_002ed324, "FWGlitter");
	return self;
}

/* 0x000c75a0  CarryOn/CFire_Resource.cpp (file inferred)
Stops and frees the sounds, the floor models and their instances, the fonts (not the
alternative one), releases the reserved channels and the frame timer. */
void CFire_Resource_Dtor(struct CFireResource *self)
{
	int i;

	biDeleteMap(self->inputMap);
	bsStopAll();
	for (i = 0; i < 0x2e; i++)
	{
		if (self->samples[i] != NULL)
		{
			bsStopSample(self->samples[i]);
			bkDeleteResource(self->samples[i]);
		}
	}
	for (i = 0; i < 7; i++)
	{
		if (self->floorInst[i] != NULL)
		{
			baDeleteActorInstance(self->floorInst[i]);
			self->floorInst[i] = NULL;
		}
		if (self->floorModel[i] != NULL)
			bkDeleteResource(self->floorModel[i]);
	}
	for (i = 0; i < 5; i++)
	{
		if (self->potInst[i] != NULL)
		{
			baDeleteActorInstance(self->potInst[i]);
			self->potInst[i] = NULL;
		}
		if (self->potModel[i] != NULL)
			bkDeleteResource(self->potModel[i]);
	}
	bkHeapFree(self->samples);
	self->samples = NULL;
	bkDeleteResource(self->font);
	bkDeleteResource(self->scoreFont);
	co_Nothing();           /* a member's empty destructor */
	for (i = 0; i < self->numReserved; i++)
		bsReserveChannel(self->reserved[i], 0);
	co_Timer_Dtor(self->timer);
}

/* the fog of the variant (both 0x000c7700 and 0x000c7950 do this) */
static void co_SetFogFields(struct CFireResource *self)
{
	int v = self->variant;

	self->fogOn = 1;
	self->fogColour[0] = DAT_0029413c[v];
	self->fogColour[1] = DAT_0029414c[v];
	self->fogColour[2] = DAT_0029415c[v];
	self->fogNear = DAT_0029416c[v];
	self->fogFar = DAT_0029417c[v];
	if (self->fogFar <= self->fogNear)
		self->fogFar = self->fogNear * 1.00000095f;    /* 0x00268668 */
}

/* 0x000c7700  CarryOn/CFire_Resource.cpp (file inferred)
Once per frame: the frame timer, the frame count, the "Detail" input (toggles the detail
tweak) and the fog of the variant. */
void CFire_Resource_UpdateDetail(struct CFireResource *self)
{
	co_Timer_Frame(self->timer);
	self->frames++;
	if (CO_I32(self->detailChannel, 0x14) != 0)
	{
		DAT_00294118 = DAT_00294118 == 0;
		bkPrintf("Detail Toggled to %d\n", DAT_00294118);
	}
	co_SetFogFields(self);
}

/* 0x000c77c0  CarryOn/CFire_Resource.cpp (file inferred)
Plays sound `index` of a kind (0..14; -1: a random one of the kind) at `volume` and
returns its channel, or -1. */
int CFire_Resource_FindSample(struct CFireResource *self, int kind, int index, int volume)
{
	void *sample;

	if (kind < 0 || kind >= 0xf || self->numSamples[kind] == 0)
		return -1;
	if (index == -1)
	{
		if (self->numSamples[kind] < 2)
			index = 0;
		else
			index = (int)Core_Rand() % self->numSamples[kind];
	}
	sample = self->samples[index + self->firstSample[kind]];
	if (sample == NULL)
		return -1;
	return bsPlaySample(sample, volume, 0, 0, 0);
}

/* 0x000c7840  CarryOn/CFire_Resource.cpp (file inferred)
Reserves a sound channel (a looping sound) until the destructor (at most 16). */
void CFire_Resource_ReserveChannel(struct CFireResource *self, int channel)
{
	if (channel != -1 && self->numReserved < 0x10)
	{
		bsReserveChannel(channel, 1);
		self->reserved[self->numReserved] = channel;
		self->numReserved++;
	}
}

/* 0x000c7880  CarryOn/CFire_Resource.cpp (file inferred)
Rumbles the pad of a human player (-1: every player) for an event: 0 factory bang,
1 rocket bang, 2 punches, 3 punched, 4 knocked out. */
void CFire_Resource_Rumble(struct CFireResource *self, int player, int event)
{
	static const char *const names[5] = {
		"FWFactoryBang", "FWRocketBang", "FWPunches", "FWPunched", "FWKO-ed",
	};
	int i;

	if (player == -1)
	{
		for (i = 0; i < 4; i++)
			CFire_Resource_Rumble(self, i, event);
		return;
	}
	if (DAT_002baed4[player][1] == 0)
		return;
	if ((unsigned int)event <= 4)
		CLRumble_PlayNamed((int)(uintptr_t)DAT_002ed324, player, (char *)names[event]);
}

/* 0x000c7950  CarryOn/CFire_Resource.cpp (file inferred)
Sets the fog of the variant and turns fog on (render state 8) when it is on. It reads the
fields back through g_fireRes, as the original does. */
void CFire_Resource_ApplyFog(struct CFireResource *self)
{
	co_SetFogFields(self);
	if (g_fireRes->fogOn != 0)
	{
		bdSetRenderState(8, 1, 0);
		bdSetFogColour(g_fireRes->fogColour[0], g_fireRes->fogColour[1],
			g_fireRes->fogColour[2]);
		bdSetFogRange(g_fireRes->fogNear, g_fireRes->fogFar);
		return;
	}
	bdSetRenderState(8, 0, 0);
}

/* 0x000c7a20  CarryOn/CFire_Resource.cpp (file inferred)
Debug lines through `count` points (float[4] each) in one colour. Flags: 1 also joins the
first and the last point, 2 keeps the world matrix, 4 no depth offset. */
void CFire_Resource_DrawLines(float *points, int count, int flags, unsigned int colour)
{
	struct CoVertex *line = DAT_002f6d68;
	int i;

	bdSetTexture(0, NULL);
	bdSetRenderState(3, 0, 0);
	if ((flags & 2) == 0)
		bdSetWorldMatrix(DAT_002855f0);
	if ((flags & 4) == 0)
		bdSetDepthOffset(-100.0f);
	for (i = 0; i < count - 1; i++)
	{
		line[0].x = points[i * 4 + 0];
		line[0].y = points[i * 4 + 1];
		line[0].z = points[i * 4 + 2];
		line[0].colour = co_colour32(colour);
		line[1].x = points[i * 4 + 4];
		line[1].y = points[i * 4 + 5];
		line[1].z = points[i * 4 + 6];
		line[1].colour = line[0].colour;
		bdDrawPrimitiveFVF(1, line, 2, 0x152);
	}
	if ((flags & 1) != 0)
	{
		line[0].x = points[0];
		line[0].y = points[1];
		line[0].z = points[2];
		line[0].colour = co_colour32(colour);
		line[1].x = points[count * 4 - 4];
		line[1].y = points[count * 4 - 3];
		line[1].z = points[count * 4 - 2];
		line[1].colour = line[0].colour;
		bdDrawPrimitiveFVF(1, line, 2, 0x152);
	}
	bdSetDepthOffset(0.0f);
}

/* 0x000c7be0  CarryOn/CFire_Resource.cpp (file inferred)
A debug circle (80 segments) of `radius` around `pos` on the ground plane, and when
`height` is not 0 a vertical line of that height. Flag 2 keeps the world matrix. */
void CFire_Resource_DrawCircleFlags(float *pos, float radius, float height, unsigned int flags,
	unsigned int colour)
{
	struct CoVertex *circle = DAT_002f6db0;
	unsigned int c;
	double angle;
	int i;

	bdSetTexture(0, NULL);
	bdSetRenderState(3, 0, 0);
	if ((flags & 2) == 0)
		bdSetWorldMatrix(DAT_002855f0);
	c = co_colour32(colour);
	for (i = 0; i < 80; i++)
	{
		/* x87: i * 0.0125 * 6.283185 (0x00268684, 0x00262300) in extended precision */
		angle = (double)i * (double)0.0125f * (double)6.28318501f;
		circle[i].x = (float)(sin(angle) * radius + pos[0]);
		circle[i].y = pos[1];
		circle[i].colour = c;
		circle[i].z = (float)(cos(angle) * radius + pos[2]);
	}
	circle[80] = circle[0];
	bdSetDepthOffset(-100.0f);
	bdDrawPrimitiveFVF(2, circle, 0x51, 0x152);
	if (height != 0.0f)
	{
		circle[0].x = pos[0];
		circle[0].y = pos[1];
		circle[0].z = pos[2];
		circle[0].colour = co_colour32(colour);
		circle[1].x = pos[0];
		circle[1].y = height + pos[1];
		circle[1].z = pos[2];
		circle[1].colour = circle[0].colour;
		bdDrawPrimitiveFVF(1, circle, 2, 0x152);
	}
	bdSetDepthOffset(0.0f);
}

/* 0x000c7da0  CarryOn/CFire_Resource.cpp (file inferred)
Draws a line from a node to each child (first child at +0x14c, siblings in a ring at
+0x140) and recurses. The line's start vertex is shared, so after a child's subtree the
next sibling's line starts where the subtree ended (as in the original). */
void CFire_Resource_DrawBones(void *instance, void *node, int a, int r, int g, int b)
{
	struct CoVertex *bone = DAT_002f7918;
	float p[4];
	void *child;

	baGetNodePosition(instance, node, p);
	bone[0].x = p[0];
	bone[0].y = p[1];
	bone[0].z = p[2];
	bone[0].colour = co_colour((unsigned int)a, (unsigned int)r, (unsigned int)g,
		(unsigned int)b);
	child = CO_PTR(node, 0x14c);
	if (child == NULL)
		return;
	do
	{
		baGetNodePosition(instance, child, p);
		bone[1].x = p[0];
		bone[1].y = p[1];
		bone[1].z = p[2];
		bone[1].colour = co_colour((unsigned int)a, (unsigned int)r, (unsigned int)g,
			(unsigned int)b);
		bdDrawPrimitiveFVF(1, bone, 2, 0x152);
		CFire_Resource_DrawBones(instance, child, a, r, g, b);
		child = CO_PTR(child, 0x140);
	} while (child != CO_PTR(node, 0x14c));
}

/* 0x000c7ed0  CarryOn/CFire_Resource.cpp (file inferred)
Draws the skeleton of a model instance (root node at +0x134) in one colour. */
void CFire_Resource_DrawSkeleton(void *instance, unsigned int colour)
{
	void *root = CO_PTR(instance, 0x134);

	bdSetTexture(0, NULL);
	bdSetRenderState(3, 0, 0);
	bdSetWorldMatrix(DAT_002855f0);
	CFire_Resource_DrawBones(instance, root, (int)(colour >> 24), (int)((colour >> 16) & 0xff),
		(int)((colour >> 8) & 0xff), (int)(colour & 0xff));
}

/* 0x000c7f30  CarryOn/CFire_Resource.cpp (file inferred)
Prints formatted text on the screen in one colour. Flags: 1 centres it on x, 2 uses the
alternative font (when loaded). Not opaque colours draw alpha blended. */
void CFire_Resource_Printf(struct CFireResource *self, float x, float y, unsigned int colour,
	int flags, const char *format, ...)
{
	unsigned short wide[1026];
	char text[1024];
	va_list args;
	float width;
	float height;
	void *font;
	int length;

	if ((flags & 2) == 0 || (font = self->fontAlt) == NULL)
		font = self->font;
	va_start(args, format);
	vsprintf(text, format, args);      /* FUN_001e041f */
	va_end(args);
	bdSetWorldMatrix(DAT_002855f0);
	bdSetProjectionMode(0);
	bdSetRenderState(1, 1, 0);
	bdSetRenderState(3, 0, 0);
	if (colour >> 24 == 0xff)
		bdSetRenderState(6, 0, 0);
	else
	{
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 0, 0);
	}
	bkStringToWide(wide, text);
	length = bkWideStrlen(wide);
	if ((flags & 1) != 0)
	{
		bdGetStringSize(font, wide, length, &width, &height);
		x = x - width * 0.5f;
	}
	bdPrintFontLite(font, wide, length, x, y, (int)((colour >> 16) & 0xff),
		(int)((colour >> 8) & 0xff), (int)(colour & 0xff), (int)(colour >> 24), NULL, NULL);
	bdSetProjectionMode(1);
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(6, 0, 0);
}

/* 0x000c80b0  CarryOn/CFire_Resource.cpp (file inferred)
Moves `pos` by `step` towards `target`; returns 1 (and puts it on the target) when the
target is within the step. (The original is a method; ECX, the resource, is unused.) */
int CFire_Resource_MoveTowards(float *pos, float *target, float step)
{
	float d[4];
	float move[4];
	float length;

	d[0] = target[0] - pos[0];
	d[1] = target[1] - pos[1];
	d[2] = target[2] - pos[2];
	length = sqrtf(d[1] * d[1] + d[0] * d[0] + d[2] * d[2]);
	if (length <= step)
	{
		pos[0] = target[0];
		pos[1] = target[1];
		pos[2] = target[2];
		pos[3] = target[3];
		return 1;
	}
	bmVectorSetLength(move, d, step);
	pos[3] = 1.0f;
	pos[0] = move[0] + pos[0];
	pos[1] = move[1] + pos[1];
	pos[2] = move[2] + pos[2];
	return 0;
}

/* 0x000c8170  CarryOn/CFire_Resource.cpp (file inferred)
With the detail tweak on: texture stage 1 adds a second pass whose strength is the
texture factor's alpha (`factor`). */
void CFire_Resource_BeginStage(int factor)
{
	if (DAT_00294118 == 0)
		return;
	D3DDevice_SetRenderState_TextureFactor((unsigned long)factor << 24);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xa, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
}

/* 0x000c8200  CarryOn/CFire_Resource.cpp (file inferred)
Ends the pass of CFire_Resource_BeginStage. */
void CFire_Resource_EndStage(void)
{
	if (DAT_00294118 == 0)
		return;
	bdSetTexture(1, NULL);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
}

/* 0x000c8230  CarryOn/CFire_Resource.cpp (file inferred)
A debug circle with the world matrix reset (CFire_Resource_DrawCircleFlags, flags 0). */
void CFire_Resource_DrawCircle(float *pos, float radius, float height, unsigned int colour)
{
	CFire_Resource_DrawCircleFlags(pos, radius, height, 0, colour);
}
