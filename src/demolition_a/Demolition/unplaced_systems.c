/*
Demolition/unplaced_systems.c  <-  C:\BlitzParty\Game\Src\Demolition\ (file not known)

The small subsystems of the Demolition game object between CDemolition_Game.cpp
and CDemolition_Light.cpp in the XBE (0x000eb7b0-0x000ec540): the decal list,
the respawn and rumble helpers, the sound effects, the platforms (crates),
the floating info texts, the debug skeleton drawing, the timer/countdown
classes, the input, and two physics body helpers. The table has no source
file for them (gap functions of Demolition).

Most of these classes have a one-slot vtable (the scalar deleting destructor);
their other methods are called directly.
*/

#include <stdarg.h>

#include "demolition_a.h"
#include "xtl.h"

/* a pure virtual slot (MSVC's _purecall at 0x001e0323 ends the program) */
static void da_purecall(void)
{
	DA_Printf("R6025 pure virtual function call (demolition_a)\n");
	abort();
}

/* ---------- vtables */

/* 0x0026a37c: the object lists (decals) */
static const void *const vtable_0026a37c[] = {
	(const void *)CDemoDecals_DeletingDtor, /* [0] */
};

/* 0x0026a4d4: the object at game +0x7b20 */
static const void *const vtable_0026a4d4[] = {
	(const void *)CDemoUnk7b20_DeletingDtor, /* [0] */
};

/* 0x0026a4d8: the respawn helper */
static const void *const vtable_0026a4d8[] = {
	(const void *)CDemoRespawn_DeletingDtor, /* [0] */
};

/* 0x0026a4dc: the rumble helper */
static const void *const vtable_0026a4dc[] = {
	(const void *)CDemoRumble_DeletingDtor, /* [0] */
};

/* 0x0026a6e4: the sound effects */
static const void *const vtable_0026a6e4[] = {
	(const void *)CDemoSfx_DeletingDtor, /* [0] */
};

/* 0x0026a6fc: the direct3D helper */
static const void *const vtable_0026a6fc[] = {
	(const void *)CDemoDirect3D_DeletingDtor, /* [0] */
};

/* 0x0026a7b0: a platform (base class; the crates of demolition_d derive from it) */
static const void *const vtable_0026a7b0[] = {
	(const void *)CDemoPlatform_DeletingDtor, /* [0] +0x00 */
	(const void *)FUN_001d5950,               /* [1] +0x04 */
	(const void *)FUN_001d5950,               /* [2] +0x08 */
	(const void *)CDemoPlatform_Update,       /* [3] +0x0c */
	(const void *)FUN_001d5950,               /* [4] +0x10 */
	(const void *)FUN_001d5950,               /* [5] +0x14 */
	(const void *)da_purecall,                /* [6] +0x18 */
};

/* 0x0026a7cc: the platform system */
static const void *const vtable_0026a7cc[] = {
	(const void *)CDemoPlatforms_DeletingDtor, /* [0] */
};

/* 0x0026a7d0: the info texts */
static const void *const vtable_0026a7d0[] = {
	(const void *)CDemoInfo_DeletingDtor, /* [0] */
};

/* 0x0026a7d4: the debug text */
static const void *const vtable_0026a7d4[] = {
	(const void *)CDemoDebug_DeletingDtor, /* [0] */
};

/* 0x0026a7d8: the game clock */
static const void *const vtable_0026a7d8[] = {
	(const void *)CDemoTimer_DeletingDtor, /* [0] */
};

/* 0x0026a7dc: the countdown timer (also used by demolition_b and demolition_c) */
static const void *const vtable_0026a7dc[] = {
	(const void *)CDemoCountdown_DeletingDtor, /* [0] */
};

/* 0x0026a7e0: the input */
static const void *const vtable_0026a7e0[] = {
	(const void *)CDemoInput_DeletingDtor, /* [0] */
};

/* 0x0026a7e4: a physics body (demolition_b's entities) */
static const void *const vtable_0026a7e4[] = {
	(const void *)CDemoBody_DeletingDtor, /* [0] */
};

/* 0x0026a7e8: the simulation */
static const void *const vtable_0026a7e8[] = {
	(const void *)CDemoSimulation_DeletingDtor, /* [0] */
};

/* the scalar deleting destructor shared by the one-slot classes */
static void *da_DeleteWithVtable(void *self, unsigned int flags, const void *const *vtbl)
{
	*(const void *const **)self = vtbl;
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* ---------- the object lists (decals at game +0x7bc8) */

/* 0x000eb7b0 */
void *CDemoDecals_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a37c;
	return self;
}

/* 0x000eb7c0 */
void CDemoDecals_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a37c;
}

/* 0x000eb7d0
Updates every object of a list (slot 1 of each object). Also used by rollerball_a. */
void CDemoObjectList_Update(void *self, float dt)
{
	char *object;

	for (object = DA_FIELD(char *, self, 4); object != NULL; object = DA_FIELD(char *, object, 4))
		DA_VFN(object, 0x04, void (*)(void *, float))(object, dt);
}

/* 0x000eb800 */
void *CDemoDecals_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a37c);
}

/* 0x000eb820
Deletes every decal: its vertex array (+0xc, an array with a count before it: deleted
through its first element's vtable when the count is nonzero), its buffer (+0x18), then the
decal (slot 0). */
void CDemoDecals_Shutdown(void *self)
{
	char *decal, *next;
	char *array;

	decal = DA_FIELD(char *, self, 4);
	while (decal != NULL)
	{
		array = DA_FIELD(char *, decal, 0xc);
		next = DA_FIELD(char *, decal, 4);
		if (array != NULL)
		{
			if (DA_FIELD(int, array, -4) == 0)
				DA_Delete(array - 4);
			else
				DA_VFN(array, 0x00, void *(*)(void *, unsigned int))(array, 3);
			DA_FIELD(void *, decal, 0xc) = NULL;
		}
		if (DA_FIELD(void *, decal, 0x18) != NULL)
		{
			DA_Delete(DA_FIELD(void *, decal, 0x18));
			DA_FIELD(void *, decal, 0x18) = NULL;
		}
		DA_VFN(decal, 0x00, void *(*)(void *, unsigned int))(decal, 1);
		decal = next;
	}
}

/* 0x000eb880
Draws the decals multiplied into the scene (blend: zero, source colour). */
void CDemoDecals_Draw(void *self)
{
	char *decal;

	DA_SetWorldIdentity();
	DA_PushRenderState(6, 1, 0);
	DA_PushRenderState(2, 0xc, 0);
	DA_PushRenderState(7, 1, 0);
	DA_PushRenderState(5, 0, 0);
	D3DDevice_SetRenderState(D3DRS_BLENDOP, 0x8006);
	D3DDevice_SetRenderState(D3DRS_SRCBLEND, 0);
	D3DDevice_SetRenderState(D3DRS_DESTBLEND, 0x301);
	for (decal = DA_FIELD(char *, self, 4); decal != NULL; decal = DA_FIELD(char *, decal, 4))
		DA_VFN(decal, 0x08, void (*)(void *))(decal);
	DA_PopRenderState(2);
	DA_PopRenderState(7);
	DA_PopRenderState(5);
	DA_PopRenderState(6);
}

/* ---------- game +0x7b20 */

/* 0x000eb940 */
void *CDemoUnk7b20_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a4d4;
	return self;
}

/* 0x000eb950 */
void CDemoUnk7b20_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a4d4;
}

/* 0x000eb960 */
void *CDemoUnk7b20_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a4d4);
}

/* ---------- the respawn helper (game +0x7b1c) */

/* 0x000eb980 */
void *CDemoRespawn_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a4d8;
	return self;
}

/* 0x000eb990 */
void CDemoRespawn_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a4d8;
}

/* 0x000eb9a0
Shutdown: deletes the CharLib characters (a tail call into charlib). */
void CDemoRespawn_Shutdown(void *self)
{
	(void)self;
	DA_CharLibDeleteAll();
}

/* 0x000eb9b0
Update: updates the CharLib characters. */
void CDemoRespawn_Update(void *self, float dt)
{
	(void)self;
	(void)dt;
	DA_CharLibUpdateAll();
}

/* 0x000eb9c0 */
void *CDemoRespawn_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a4d8);
}

/* ---------- the rumble helper (game +0x7a38) */

/* 0x000eb9e0 */
void *CDemoRumble_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a4dc;
	return self;
}

/* 0x000eb9f0 */
void CDemoRumble_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a4dc;
}

/* 0x000eba00
Init: makes the rumble menu entries. Always succeeds. */
int CDemoRumble_Init(void *self, void *package)
{
	(void)self;
	(void)package;
	DA_RumbleCreate();
	return 1;
}

/* 0x000eba20
Shutdown: stops every rumble and shuts the rumble manager down. */
void CDemoRumble_Shutdown(void *self)
{
	(void)self;
	DA_RumbleStopAll();
	DA_RumbleShutdown();
}

/* the shared body of the two rumble effects */
static void da_Rumble(void *self, int kind, int player, float strength)
{
	struct CDemolition_Rumble *r = (struct CDemolition_Rumble *)self;

	if (strength < 0.5f)
		strength = 0.5f;
	r->kind = kind;
	r->strength = (int)(strength * 32767.0f);
	r->duration = 0x44c;
	r->attack = 10;
	r->release = 0x50;
	r->unk18 = 0;
	r->unk1c = 4;
	if (DA_PLAYER_PRESENT(player) != 0)
		DA_RumbleModify(player, 1, &r->kind);
}

/* 0x000eba40
Rumbles PLAYER's pad (kind 0) with STRENGTH (at least 0.5) for 1100 ms. Called by
demolition_c. */
void CDemoRumble_Shake(void *self, int player, float strength)
{
	da_Rumble(self, 0, player, strength);
}

/* 0x000ebac0
As CDemoRumble_Shake with kind 1. Called by demolition_b. */
void CDemoRumble_Shake2(void *self, int player, float strength)
{
	da_Rumble(self, 1, player, strength);
}

/* 0x000ebb40 */
void *CDemoRumble_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a4dc);
}

/* ---------- the sound effects (game +0x79d0) */

/* 0x000ebb60 */
void *CDemoSfx_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a6e4;
	return self;
}

/* 0x000ebb70 */
void CDemoSfx_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a6e4;
}

/* 0x000ebb80
Init: loads the 25 samples of DAT_00297750 from PACKAGE. Always succeeds. */
int CDemoSfx_Init(void *self, void *package)
{
	struct CDemolition_Sfx *sfx = (struct CDemolition_Sfx *)self;
	int i;

	for (i = 0; i < DA_SFX_COUNT; i++)
	{
		sfx->samples[i] = DA_LoadResource(package, DAT_00297750[i], 2, 0xdefa);
		if (sfx->samples[i] != NULL)
			DA_Printf("Loaded sample <%s>\n", DAT_00297750[i]);
	}
	return 1;
}

/* 0x000ebbe0
Shutdown: deletes the samples, then stops every sound. */
void CDemoSfx_Shutdown(void *self)
{
	struct CDemolition_Sfx *sfx = (struct CDemolition_Sfx *)self;
	int i;

	for (i = 0; i < DA_SFX_COUNT; i++)
	{
		if (sfx->samples[i] != NULL)
		{
			DA_DeleteResource(sfx->samples[i]);
			sfx->samples[i] = NULL;
		}
	}
	DA_SoundStopAll();
}

/* 0x000ebc20
Stops sound channel CHANNEL. Called by demolition_c. */
void CDemoSfx_Stop(unsigned int channel)
{
	DA_SoundStopChannel((int)channel);
}

/* 0x000ebc30 */
void *CDemoSfx_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a6e4);
}

/* ---------- the direct3D helper (game +0x79c8) */

/* 0x000ebc50 */
void *CDemoDirect3D_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a6fc;
	return self;
}

/* 0x000ebc60 */
void CDemoDirect3D_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a6fc;
}

/* 0x000ebc70 */
void *CDemoDirect3D_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a6fc);
}

/* ---------- a platform (base class of the crates, demolition_d) */

/* 0x000ebc90  Called by demolition_d's crate constructors. */
void *CDemoPlatform_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a7b0;
	return self;
}

/* 0x000ebca0  Called by demolition_d's crate destructors. */
void CDemoPlatform_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a7b0;
}

/* 0x000ebcb0
Slot 3 (Update): after a colour or position change the collision is rebuilt; the actor's
matrix is updated and, when not yet done, its inverse is computed. */
void CDemoPlatform_Update(void *self, float dt)
{
	struct CDemolition_Platform *p = (struct CDemolition_Platform *)self;
	char *matrix;

	(void)dt;
	if (p->dirty != 0)
	{
		DA_FIELD(char, p->actor, 0x3d4) = 1;
		p->dirty = 0;
		DA_UpdateAnimatingCollision(p->collision);
	}
	DA_ActorUpdateMatrix(p->actor);
	matrix = DA_FIELD(char *, p->actor, 0x190);
	if ((DA_FIELD(unsigned char, matrix, 0x138) & 2) == 0)
	{
		DA_MatrixInverseAffine((float *)(matrix + 0x80), (const float *)matrix);
		DA_FIELD(unsigned char, DA_FIELD(char *, p->actor, 0x190), 0x138) |= 2;
	}
}

/* ---------- the platform system (game +0x79a4) */

/* 0x000ebd20 */
void *CDemoPlatforms_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a7cc;
	return self;
}

/* 0x000ebd30 */
void CDemoPlatforms_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a7cc;
}

/* 0x000ebd40
Init: loads the six crate models and the crate's animating collision from PACKAGE. Returns 0
when one is missing. */
int CDemoPlatforms_Init(void *self, void *package)
{
	struct CDemolition_PlatformSystem *ps = (struct CDemolition_PlatformSystem *)self;
	int i;

	ps->first = NULL;
	for (i = 0; i < 6; i++)
	{
		ps->models[i] = DA_LoadResource(package, DAT_002977b4[i], 1, 0xdefa);
		if (ps->models[i] == NULL)
			return 0;
	}
	ps->crateAC = DA_LoadResource(package, DAT_002977cc[0], 8, 0xdefa);
	if (ps->crateAC == NULL)
		return 0;
	return 1;
}

/* 0x000ebdd0
Shutdown: every platform shuts down (slot 2) and is deleted. */
void CDemoPlatforms_Shutdown(void *self)
{
	char *p, *next;

	p = DA_FIELD(char *, self, 4);
	while (p != NULL)
	{
		next = DA_FIELD(char *, p, 4);
		DA_VFN(p, 0x08, void (*)(void *))(p);
		DA_VFN(p, 0x00, void *(*)(void *, unsigned int))(p, 1);
		p = next;
	}
}

/* 0x000ebe00
Update: every enabled platform (slot 3). */
void CDemoPlatforms_Update(void *self, float dt)
{
	char *p;

	for (p = DA_FIELD(char *, self, 4); p != NULL; p = DA_FIELD(char *, p, 4))
	{
		if (DA_FIELD(char, p, 0x1c) != 0)
			DA_VFN(p, 0x0c, void (*)(void *, float))(p, dt);
	}
}

/* 0x000ebe30
Draw: every visible platform (slot 4), with render state 3 on. */
void CDemoPlatforms_Draw(void *self)
{
	char *p;

	DA_PushRenderState(3, 1, 0);
	for (p = DA_FIELD(char *, self, 4); p != NULL; p = DA_FIELD(char *, p, 4))
	{
		if (DA_FIELD(char, p, 0x1d) != 0)
			DA_VFN(p, 0x10, void (*)(void *))(p);
	}
	DA_PopRenderState(3);
}

/* 0x000ebe70
Damage: every enabled platform that collides gets the explosion at POS (slot 6). Called by
demolition_b's explosions. */
void CDemoPlatforms_Damage(void *self, float *pos, float radius)
{
	char *p;

	for (p = DA_FIELD(char *, self, 4); p != NULL; p = DA_FIELD(char *, p, 4))
	{
		if (DA_FIELD(char, p, 0x1c) != 0 && DA_FIELD(char, p, 0x1e) != 0)
			DA_VFN(p, 0x18, void (*)(void *, float *, float))(p, pos, radius);
	}
}

/* 0x000ebeb0 */
void *CDemoPlatform_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7b0);
}

/* 0x000ebed0
Sets a platform's colour (and its actor's), marking it changed. Called by demolition_b/d. */
void CDemoPlatform_SetColour(void *self, const float *colour)
{
	struct CDemolition_Platform *p = (struct CDemolition_Platform *)self;
	float *actorColour;

	p->colour[0] = colour[0];
	p->colour[1] = colour[1];
	p->colour[2] = colour[2];
	p->colour[3] = colour[3];
	actorColour = (float *)DA_AT(p->actor, 0xe0);
	actorColour[0] = colour[0];
	actorColour[1] = colour[1];
	actorColour[2] = colour[2];
	actorColour[3] = colour[3];
	p->dirty = 1;
}

/* 0x000ebf20
Sets the platform actor's position, marking it changed. Called by demolition_b. */
void CDemoPlatform_SetPosition(void *self, const float *pos)
{
	struct CDemolition_Platform *p = (struct CDemolition_Platform *)self;
	float *actorPos = (float *)DA_AT(p->actor, 0x1f4);

	actorPos[0] = pos[0];
	actorPos[1] = pos[1];
	actorPos[2] = pos[2];
	actorPos[3] = pos[3];
	p->dirty = 1;
}

/* 0x000ebf50 */
void *CDemoPlatforms_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7cc);
}

/* ---------- the info texts (game +0x7900) */

/* 0x000ebf70 */
void *CDemoInfo_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a7d0;
	return self;
}

/* 0x000ebf80 */
void CDemoInfo_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a7d0;
}

/* 0x000ebf90
Init: the font handgotb.ttf and the text system (15 texts); texts rise at 300/s for 0.1 s and
fade over 325. Returns 0 without the font. */
int CDemoInfo_Init(void *self, void *package)
{
	void *font;

	font = DA_LoadResource(package, "handgotb.ttf", 3, 0xdefa);
	DA_FIELD(void *, self, 8) = font;
	if (font == NULL)
		return 0;
	DA_InfoSetFont(DA_AT(self, 0x10), 0xf);
	DA_FIELD(float, self, 0x94) = 0.1f;
	DA_FIELD(float, self, 0x90) = 300.0f;
	DA_FIELD(float, self, 0x98) = 325.0f;
	return 1;
}

/* 0x000ebff0
Adds a floating text at POS in colour R, G, B (A is not used) made from the printf FORMAT. */
void CDemoInfo_AddText(void *self, float *pos, int r, int g, int b, int a, const char *format, ...)
{
	char text[1024];
	va_list ap;

	(void)a;
	va_start(ap, format);
	vsprintf(text, format, ap);
	va_end(ap);
	FUN_0003c920(DA_FIELD(void *, self, 0x10), DA_FIELD(void *, self, 8), (double)pos[0], (double)pos[1],
		(double)pos[2], r, g, b, text);
}

/* 0x000ec070
Draw: every extra system's items. */
void CDemoInfo_Draw(void *self)
{
	(void)self;
	DA_DrawExtras(NULL, NULL);
}

/* 0x000ec080 */
void *CDemoInfo_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7d0);
}

/* 0x000ec0a0
Update: the texts face the camera; every extra system advances by the frame time / 60. */
void CDemoInfo_Update(void *self, float dt)
{
	char *g = DA_GAME;

	(void)dt;
	DA_FIELD(float, self, 0x14) = atan2f(DA_FIELD(float, g, DG_CAMPOS) - DA_FIELD(float, g, DG_CAMTARGET),
		DA_FIELD(float, g, DG_CAMPOS + 8) - DA_FIELD(float, g, DG_CAMTARGET + 8));
	DA_UpdateExtras(NULL, DAT_0028fda8 * 0.0166666675f);
}

/* ---------- the debug text and skeleton drawing (game +0x78f8) */

/* 0x000ec0e0 */
void *CDemoDebug_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a7d4;
	return self;
}

/* 0x000ec0f0 */
void CDemoDebug_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a7d4;
}

/* 0x000ec100
Init: the font handgotl.ttf (from any package). Returns 0 without it. */
int CDemoDebug_Init(void *self)
{
	void *font = DA_LoadResource(NULL, "handgotl.ttf", 3, 0xdefa);

	DA_FIELD(void *, self, 4) = font;
	return font != NULL;
}

/* 0x002ff860: the line the skeleton drawing uses (two vertices of 0x24 bytes: position, normal,
   diffuse colour, texture coordinates; FVF 0x152) */
struct DA_LineVertex {
	float x, y, z;       /* +0x00 */
	float nx, ny, nz;    /* +0x0c */
	uint32_t colour;     /* +0x18 */
	float u, v;          /* +0x1c */
};
static struct DA_LineVertex DAT_002ff860[2];

/* 0x000ec130
Draws the bones under NODE of actor instance INST as lines (colour R, G, B, A), recursively
for each child (the children form a ring through +0x140, starting at +0x14c). */
void CDemoDebug_DrawBones(void *self, void *inst, void *node, int r, int g, int b, int a)
{
	float pos[3];
	char *child;

	DA_GetNodePosition(inst, node, pos);
	DAT_002ff860[0].y = pos[1];
	DAT_002ff860[0].x = pos[0];
	DAT_002ff860[0].z = pos[2];
	DAT_002ff860[0].colour = DAT_002bc490[g] | DAT_002bc898[a] | DAT_002bd640[b] | DAT_002bce18[r];
	child = DA_FIELD(char *, node, 0x14c);
	if (child == NULL)
		return;
	do
	{
		DA_GetNodePosition(inst, child, pos);
		DAT_002ff860[1].y = pos[1];
		DAT_002ff860[1].x = pos[0];
		DAT_002ff860[1].z = pos[2];
		DAT_002ff860[1].colour = DAT_002bc490[g] | DAT_002bc898[a] | DAT_002bd640[b] | DAT_002bce18[r];
		DA_DrawPrimitive(1, DAT_002ff860, 2, 0x152);
		CDemoDebug_DrawBones(self, inst, child, r, g, b, a);
		child = DA_FIELD(char *, child, 0x140);
	} while (child != DA_FIELD(char *, node, 0x14c));
}

/* 0x000ec260 */
void *CDemoDebug_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7d4);
}

/* 0x000ec280
Draws the skeleton of actor instance INST (its root node at +0x134) untextured in world
space. Called by demolition_b. */
void CDemoDebug_DrawSkeleton(void *self, void *inst, int r, int g, int b, int a)
{
	void *root = DA_FIELD(void *, inst, 0x134);

	DA_SetTexture(0, NULL);
	DA_SetWorldIdentity();
	CDemoDebug_DrawBones(self, inst, root, r, g, b, a);
}

/* ---------- the game clock (game +0x7800) and the countdown timers */

/* 0x000ec2d0 */
void *CDemoTimer_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a7d8;
	return self;
}

/* 0x000ec2e0 */
void CDemoTimer_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a7d8;
}

/* 0x000ec2f0  Also used by demolition_b and demolition_c. */
unsigned int *CDemoCountdown_Ctor(unsigned int *self)
{
	*(const void *const **)self = vtable_0026a7dc;
	return self;
}

/* 0x000ec300  Also used by demolition_b and demolition_c. */
void CDemoCountdown_Dtor(unsigned int *self)
{
	*(const void *const **)self = vtable_0026a7dc;
}

/* 0x000ec310
Init: stopped, not paused, not expired, the tick frequency copied. Always succeeds. Used by
many modules (its other methods are rollerball_a's 0x0018e8e0-0x0018ea60). */
int CDemoCountdown_Init(int self)
{
	struct CDemolition_Countdown *c = (struct CDemolition_Countdown *)(uintptr_t)self;

	c->freqLo = DAT_002cd7a0;
	c->running = 0;
	c->paused = 0;
	c->expired = 0;
	c->unk10 = 0;
	c->unk14 = 0;
	c->unk28 = 0;
	c->unk30 = 0;
	c->freqHi = DAT_002cd7a4;
	return 1;
}

/* 0x000ec340 */
void *CDemoTimer_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7d8);
}

/* 0x000ec360 */
void *CDemoCountdown_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7dc);
}

/* ---------- the input (game +0x77e0) */

/* 0x000ec380 */
void *CDemoInput_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a7e0;
	return self;
}

/* 0x000ec390 */
void CDemoInput_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a7e0;
}

/* 0x000ec3a0
Init: an input map of 6 channels, one "skip" channel per player bound to the player's pad
(0xc5: the A button). Always succeeds. */
int CDemoInput_Init(void *self)
{
	struct CDemolition_Input *in = (struct CDemolition_Input *)self;
	int i;

	in->map = DA_InputCreateMap(6);
	for (i = 0; i < 4; i++)
	{
		in->channels[i] = DA_InputCreateChannel(in->map, "skip", 1, 1, 0, 0, 0);
		DA_InputBind(in->channels[i], (unsigned int)DA_PLAYER_PADBITS(i) | 0xc5, 0);
	}
	return 1;
}

/* 0x000ec410 */
void *CDemoInput_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7e0);
}

/* ---------- physics bodies and the simulation */

/* 0x000ec430  Called by demolition_b's entity constructor. */
void *CDemoBody_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a7e4;
	return self;
}

/* 0x000ec440  Called by demolition_b's entity destructor. */
void CDemoBody_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a7e4;
}

/* 0x000ec450 */
void *CDemoSimulation_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026a7e8;
	return self;
}

/* 0x000ec460 */
void CDemoSimulation_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026a7e8;
}

/* 0x000ec470 */
void *CDemoBody_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7e4);
}

/* 0x000ec490
Initialises a body state (0x70 bytes, all cleared including its first word): position POS
(+0x04), orientation ROT (+0x54), scale 1 (+0x48) and 1 / MASS (+0x64). Called by
demolition_b and rollerball_b. */
int CDemoBody_InitState(unsigned int *self, unsigned int *pos, unsigned int *rot, float mass)
{
	uint32_t *s = (uint32_t *)self;
	float inverse = 1.0f / mass;

	memset(s, 0, 0x1c * 4);
	memcpy(&s[1], pos, 16);
	memcpy(&s[0x15], rot, 16);
	DA_FIELD(float, self, 0x48) = 1.0f;
	DA_FIELD(float, self, 0x64) = inverse;
	return 1;
}

/* 0x000ec500
Sets a body's position and its previous position to POS. Called by demolition_b and
rollerball_b. */
void CDemoBody_SetPosition(void *self, const float *pos)
{
	memcpy(DA_AT(self, 0x04), pos, 16);
	memcpy(DA_AT(self, 0x14), pos, 16);
}

/* 0x000ec540 */
void *CDemoSimulation_DeletingDtor(void *self, unsigned int flags)
{
	return da_DeleteWithVtable(self, flags, vtable_0026a7e8);
}
