/*
CharLib/Collect.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\Collect.cpp (tags at 0x000b9d10, 0x000ba1a0 and
0x000ba2a0).

Pick-ups (power-ups the players collect: speed, attack ...) and the respawn points of the
AI players. Pick-ups are in the list at the CharLib world object + 0x0c, respawn points at
+ 0x30.

The respawn-point and pick-up helpers between the Platform.cpp code and the first
Collect.cpp tag (0x000b9a30-0x000b9c90) and the functions after the last tag up to the
CarryOn block (0x000ba3c0-0x000ba470) are placed here (inferred).
*/

#include "charlib.h"
#include "charlib_ai.h"

/* ---------- a pick-up (0x250 bytes) */

#define PU_TYPE 0x08               /* int: index into pickup_types */
#define PU_INST 0x10               /* the bactor instance (its position at +0xd0, scale +0xe0) */
#define PU_POS 0xd0                /* float[4] */
#define PU_SCALE 0xe0              /* float[3] */
#define PU_NODE 0x144              /* root node of the instance */
#define PU_GLOW_POS 0x180          /* float[4]: where the glow sprite is drawn */
#define PU_SPIN_ANIM 0x190         /* the "spin" animation */
#define PU_RADIUS 0x194            /* float: ground probe radius (20) */
#define PU_FLAGS 0x198             /* 1: falling */
#define PU_VEL 0x19c               /* float[4] */
#define PU_GLOW 0x1ac              /* float: glow size */
#define PU_GLOW_SCALE 0x1b0        /* float */
#define PU_SPRITE_ANGLE 0x1b4      /* float */
#define PU_EFFECT 0x1b8            /* the collect effect (classlib_fx CCollectEffect) */
#define PU_EFFECT_STATE 0x240      /* its state: 1 hold, 3 done */

/* a respawn point (0x20 bytes): list node, position at 0x08, player using it at 0x18
(short, -1 free), id at 0x1a (short), timer at 0x1c */

/* the world object's lists */
#define pickup_list ((void *)((char *)chl_actor_list + 0x0c))
#define respawn_list ((void *)((char *)chl_actor_list + 0x30))

/* 0x00293a04  the pick-up types: { model name, model, duration (short at +8) } x 3 */
#define pickup_types CHL_ARR(unsigned int, 0x00293a04)
/* 0x00293a3c  glow sprite size per unit of glow */
#define pickup_glow_size CHL_VAR(float, 0x00293a3c)
/* 0x00293a40  height of the glow sprite per unit of glow */
#define pickup_glow_height CHL_VAR(float, 0x00293a40)
/* 0x00293a44  collect distance */
#define pickup_collect_dist CHL_VAR(float, 0x00293a44)
/* 0x00293a48  alpha of the pick-up models */
#define pickup_alpha CHL_VAR(int, 0x00293a48)
/* 0x002f6b78  the glow texture ("tokenglow.bmp") */
#define pickup_glow_texture CHL_VAR(void *, 0x002f6b78)

/* classlib_fx: the collect effect (CCollectEffect_Init(self, pos, rays),
CCollectEffect_Update(self) -> glow) */
#define pu_EffectInit ((void (*)(void *, float *, int))FUN_000bf1f0)
#define pu_EffectUpdate ((float (*)(void *))FUN_000bf5b0)
/* bdisplay: bdSetWorldIdentity(), bdSetDepthOffset(offset), bdDrawSprite3D(pos, width,
height, angle, texture, flipU, flipV, r, g, b, a, vertices, zBias) */
#define pu_SetWorldIdentity ((void (*)(void))FUN_000438b0)
#define pu_SetDepthOffset ((void (*)(float))FUN_00044550)
#define pu_DrawSprite3D \
	((int (*)(float *, float, float, float, void *, int, int, int, int, int, int, float *, float))FUN_00040750)

/* bkernel 0x00051540: squared distance (bkernel's port has it as an empty function;
reported) */
static float pickup_dist_sq(const float *a, const float *b)
{
	float dx = a[0] - b[0];
	float dy = a[1] - b[1];
	float dz = a[2] - b[2];

	return dz * dz + dy * dy + dx * dx;
}

/* 0x000b9a30  CharLib (Collect.cpp, inferred)
A respawn point after a spawn: counts down; at the end the player is fully grown and
active again and the point is free. While it runs the player grows (scale y + 1/60 per
frame up to 1). */
void RespawnPoint_Update(void *point)
{
	float t;
	void *actor;

	if (CHL_F32(point, 0x1c) == 0.0f)
		return;
	t = CHL_F32(point, 0x1c) - chl_time_scale;
	CHL_F32(point, 0x1c) = t;
	if (t <= 0.0f) {
		if (-1 < CHL_I16(point, 0x18)) {
			CHL_F32(CHL_PTR(chl_ai_players[CHL_I16(point, 0x18)], AI_ACTOR), 0xe4) = 1.0f;
			CHL_I16(chl_ai_players[CHL_I16(point, 0x18)], AI_DISABLED) = 0;
		}
		CHL_F32(point, 0x1c) = 0.0f;
		CHL_I16(point, 0x18) = -1;
	}
	if (-1 < CHL_I16(point, 0x18)) {
		actor = CHL_PTR(chl_ai_players[CHL_I16(point, 0x18)], AI_ACTOR);
		t = chl_time_scale * 0.0166666675f + CHL_F32(actor, 0xe4);
		if (1.0f < t)
			t = 1.0f;
		CHL_F32(CHL_PTR(chl_ai_players[CHL_I16(point, 0x18)], AI_ACTOR), 0xe4) = t;
	}
}

/* 0x000b9ae0  CharLib (Collect.cpp, inferred) */
void RespawnPoints_UpdateAll(void)
{
	void *point;

	for (point = chl_ListNext(respawn_list, NULL); point != NULL; point = chl_ListNext(respawn_list, point))
		RespawnPoint_Update(point);
}

/* 0x000b9b20  CharLib (Collect.cpp, inferred) */
void *RespawnPoint_Init(void *point, float *pos, int id)
{
	memset(point, 0, 0x20);
	memcpy(CHL_FIELD(char, point, 0x8), pos, 0x10);
	CHL_F32(point, 0x14) = 1.0f;
	CHL_I16(point, 0x18) = -1;
	chl_ListInsert(respawn_list, point, 0);
	CHL_I16(point, 0x1a) = (short)id;
	CHL_F32(point, 0x1c) = 0.0f;
	return point;
}

/* 0x000b9b80  CharLib (Collect.cpp, inferred) */
void RespawnPoints_DeleteAll(void)
{
	void *point = chl_ListNext(respawn_list, NULL);
	void *prev;

	while (point != NULL) {
		prev = chl_ListRemove(respawn_list, point);
		if (point != NULL)
			chl_Free(point);
		point = chl_ListNext(respawn_list, prev);
	}
}

/* 0x000b9be0  CharLib (Collect.cpp, inferred)
Frees the collect effect of a pick-up (the member destructor). */
void Pickup_FreeEffect(void *pickup)
{
	CCollectEffect_Free(CHL_FIELD(void, pickup, PU_EFFECT));
}

/* 0x000b9bf0  CharLib (Collect.cpp, inferred)
Finds the pick-up models and the glow texture. */
void Pickups_LoadModels(void)
{
	int i;

	for (i = 0; i < 3; i++)
		pickup_types[i * 3 + 1] = (unsigned int)chl_FindObject(NULL, (const char *)pickup_types[i * 3], 1, 0);
	pickup_glow_texture = chl_FindObject(NULL, "tokenglow.bmp", 0, 0xdefa);
}

/* 0x000b9c40  CharLib (Collect.cpp, inferred)
The first pick-up of a type (-1: any). */
void *Pickup_FindType(int type)
{
	void *pickup;

	for (pickup = chl_ListNext(pickup_list, NULL); pickup != NULL; pickup = chl_ListNext(pickup_list, pickup)) {
		if (type == -1)
			return pickup;
		if (CHL_I32(pickup, PU_TYPE) == type)
			return pickup;
	}
	return NULL;
}

/* 0x000b9c90  CharLib (Collect.cpp, inferred)
The pick-up of a type (-1: any) nearest to pos. */
void *Pickup_Nearest(float *pos, int type)
{
	void *pickup;
	void *best = NULL;
	float best_dist = 1e13f;
	float d;

	for (pickup = chl_ListNext(pickup_list, NULL); pickup != NULL; pickup = chl_ListNext(pickup_list, pickup)) {
		if ((type == -1 || CHL_I32(pickup, PU_TYPE) == type) &&
		    (d = FUN_00051500(pos, CHL_FIELD(float, pickup, PU_POS))) < best_dist) {
			best_dist = d;
			best = pickup;
		}
	}
	return best;
}

/* 0x000b9d10  CharLib/Collect.cpp
A pick-up of a type (3: random of the three) at pos (100 units up), spinning, with its
collect effect. */
void *Pickup_Init(void *pickup, int type, float *pos, float glow_scale)
{
	void *inst = CHL_FIELD(void, pickup, PU_INST);

	CCollectEffect_Ctor(CHL_FIELD(void, pickup, PU_EFFECT));
	if (type == 3)
		CHL_I32(pickup, PU_TYPE) = CHL_Rand() % 3;
	else
		CHL_I32(pickup, PU_TYPE) = type;
	pos[3] = 1.0f;
	CHL_F32(pickup, PU_GLOW_SCALE) = glow_scale;
	CHL_F32(pickup, PU_SPRITE_ANGLE) = (float)CHL_Rand() * 9.58767196e-05f;
	chl_InitInstance(inst, (void *)pickup_types[CHL_I32(pickup, PU_TYPE) * 3 + 1], 2,
		"C:\\BlitzParty\\Game\\Src\\CharLib\\Collect.cpp", 0x3a);
	memcpy(CHL_FIELD(char, pickup, PU_POS), pos, 0x10);
	CHL_F32(pickup, PU_POS + 0x4) = CHL_F32(pickup, PU_POS + 0x4) + 100.0f;
	CHL_F32(pickup, PU_VEL + 0x0) = 0.0f;
	CHL_F32(pickup, PU_VEL + 0x4) = 0.0f;
	CHL_F32(pickup, PU_VEL + 0x8) = 0.0f;
	CHL_F32(pickup, PU_VEL + 0xc) = 1.0f;
	CHL_F32(pickup, PU_GLOW) = 0.0f;
	CHL_I32(pickup, PU_FLAGS) = 1;
	CHL_F32(pickup, PU_SCALE + 0x0) = 0.0f;
	CHL_F32(pickup, PU_SCALE + 0x4) = 0.0f;
	CHL_F32(pickup, PU_SCALE + 0x8) = 0.0f;
	memcpy(CHL_FIELD(char, pickup, PU_GLOW_POS), CHL_FIELD(char, pickup, PU_POS), 0x10);
	CHL_F32(pickup, PU_GLOW_POS + 0x4) = pickup_glow_height * CHL_F32(pickup, PU_GLOW) +
		CHL_F32(pickup, PU_GLOW_POS + 0x4);
	pu_EffectInit(CHL_FIELD(void, pickup, PU_EFFECT), pos, 0x3c);
	chl_PrepInstance(inst, 1);
	chl_SetNodeTime(inst, CHL_PTR(pickup, PU_NODE), 0, 1);
	chl_ListInsert(pickup_list, pickup, 0);
	CHL_F32(pickup, PU_RADIUS) = 20.0f;
	CHL_PTR(pickup, PU_SPIN_ANIM) = ((void *(*)(void *, const char *))FUN_00035c70)(inst, "spin");
	chl_FlushAnimations(inst, CHL_PTR(pickup, PU_NODE), 1, 0);
	chl_QueueAnimation(inst, CHL_PTR(pickup, PU_NODE), CHL_PTR(pickup, PU_SPIN_ANIM), 0x400, -1, 1, NULL, NULL);
	return pickup;
}

/* 0x000b9f40  CharLib/Collect.cpp (bracketed)
Draws the collect effects of the pick-ups. */
void Pickups_DrawEffects(void)
{
	void *pickup;

	if (CHL_I32(chl_actor_list, 0x14) == 0)
		return;
	pu_SetWorldIdentity();
	for (pickup = chl_ListNext(pickup_list, NULL); pickup != NULL; pickup = chl_ListNext(pickup_list, pickup))
		CCollectEffect_Draw(CHL_FIELD(void, pickup, PU_EFFECT));
}

/* 0x000b9fa0  CharLib/Collect.cpp (bracketed)
Draws the pick-up models with a constant alpha (texture stage 1). */
void Pickups_DrawModels(void)
{
	void *pickup;

	D3DDevice_SetRenderState_TextureFactor((DWORD)pickup_alpha << 24);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0x12);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)10, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)11, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
	for (pickup = chl_ListNext(pickup_list, NULL); pickup != NULL; pickup = chl_ListNext(pickup_list, pickup)) {
		chl_PrepInstance(CHL_FIELD(void, pickup, PU_INST), 1);
		chl_DrawInstance(CHL_FIELD(void, pickup, PU_INST), 7);
	}
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	chl_SetTexture(1, NULL);
	pu_SetWorldIdentity();
}

/* 0x000ba0a0  CharLib/Collect.cpp (bracketed)
Draws the glow sprites of the pick-ups (additive, depth offset -1.5). */
void Pickups_DrawGlows(void)
{
	void *pickup;
	float size;

	chl_SetRenderState(3, 0, 0);
	chl_SetRenderState(6, 1, 0);
	chl_SetRenderState(7, 2, 0);
	chl_SetRenderState(10, 1, 0);
	chl_PushRenderState(5, 0, 0);
	pu_SetDepthOffset(-1.5f);
	for (pickup = chl_ListNext(pickup_list, NULL); pickup != NULL; pickup = chl_ListNext(pickup_list, pickup)) {
		if (0.01f <= CHL_F32(pickup, PU_GLOW)) {
			size = pickup_glow_size * CHL_F32(pickup, PU_GLOW);
			pu_DrawSprite3D(CHL_FIELD(float, pickup, PU_GLOW_POS), size, size, CHL_F32(pickup, PU_SPRITE_ANGLE),
				pickup_glow_texture, 0, 0, 0x7f, 0x7f, 0x7f, 0x38, NULL, 0.0f);
		}
	}
	chl_SetRenderState(10, 0, 0);
	chl_SetRenderState(7, 3, 0);
	chl_PopRenderState(5);
	pu_SetDepthOffset(0.0f);
}

/* 0x000ba1a0  CharLib/Collect.cpp
`count` pick-ups of a type in a ring of radius `radius` around centre. */
void Pickups_SpawnRing(int type, float *centre, float radius, int count)
{
	float offset[4];
	float pos[4];
	float step;
	void *pickup;

	offset[2] = radius;
	offset[0] = 0.0f;
	offset[1] = 0.0f;
	offset[3] = 0.0f;
	if (count <= 0)
		return;
	step = 6.283185f / (float)count;
	do {
		pos[0] = offset[0] + centre[0];
		pos[1] = offset[1] + centre[1];
		pos[2] = offset[2] + centre[2];
		pickup = chl_Alloc(0x250, "C:\\BlitzParty\\Game\\Src\\CharLib\\Collect.cpp", 0x22a, 0x1001);
		if (pickup != NULL)
			Pickup_Init(pickup, type, pos, 1.0f);
		FUN_00051170(offset, offset, step);
		count--;
	} while (count != 0);
}

/* 0x000ba2a0  CharLib/Collect.cpp
A grid of nx x nz pick-ups of a type, `spacing` apart, centred on centre. */
void Pickups_SpawnGrid(int type, float *centre, float spacing, int nx, int nz)
{
	float half_x = (float)(nx - 1) * 0.5f;
	float half_z = (float)(nz - 1) * 0.5f;
	float cx = centre[0];
	float cz = centre[2];
	float pos[4];
	void *pickup;
	int i;
	int j;

	pos[1] = centre[1];
	for (i = 0; i < nx; i++) {
		pos[0] = (float)i * spacing + (cx - half_x * spacing);
		for (j = 0; j < nz; j++) {
			pos[2] = (float)j * spacing + (cz - half_z * spacing);
			pickup = chl_Alloc(0x250, "C:\\BlitzParty\\Game\\Src\\CharLib\\Collect.cpp", 0x245, 0x1001);
			if (pickup != NULL)
				Pickup_Init(pickup, type, pos, 1.0f);
		}
	}
}

/* 0x000ba3c0  CharLib (Collect.cpp, inferred)
Deletes a pick-up (the AI players that aim at it forget it); returns the list node before
it. */
void *Pickup_Delete(void *pickup)
{
	void *prev = chl_ListRemove(pickup_list, pickup);
	int i;

	for (i = 0; i < chl_player_count; i++) {
		void *ai = chl_ai_players[i];

		if (ai != NULL && CHL_PTR(ai, 0x10) == pickup)
			CHL_PTR(ai, 0x10) = NULL;
	}
	chl_DeleteInstance(CHL_FIELD(void, pickup, PU_INST));
	if (pickup != NULL) {
		Pickup_FreeEffect(pickup);
		chl_Free(pickup);
	}
	return prev;
}

/* 0x000ba430  CharLib (Collect.cpp, inferred) */
void Pickups_DeleteAll(void)
{
	void *pickup = chl_ListNext(pickup_list, NULL);

	while (pickup != NULL)
		pickup = chl_ListNext(pickup_list, Pickup_Delete(pickup));
}

/* 0x000ba460  CharLib (Collect.cpp, inferred) */
void Pickups_DrawAll(void)
{
	Pickups_DrawEffects();
	Pickups_DrawModels();
	Pickups_DrawGlows();
}

/* 0x000ba470  CharLib (Collect.cpp, inferred)
Per frame: the glow sprite turns, a falling pick-up drops to the ground, the effect
updates the glow; while the effect holds, the model scales with the glow; once it is done,
a player near enough collects it (the AI's timer of that type is set to the type's
duration, virtual slot 15 runs) and the pick-up is deleted. Returns the list node to
continue from. */
void *Pickup_Update(void *pickup)
{
	float g = CHL_VAR(float, 0x00292c84) * 0.5f;
	float h;
	float dx;
	float dy;
	float dz;
	float glow;
	int i;

	CHL_F32(pickup, PU_SPRITE_ANGLE) = chl_time_scale * 0.05f + CHL_F32(pickup, PU_SPRITE_ANGLE);
	CHL_F32(pickup, PU_SPRITE_ANGLE) = FUN_000703a0(CHL_F32(pickup, PU_SPRITE_ANGLE));
	if ((CHL_U8(pickup, PU_FLAGS) & 1) != 0) {
		h = CLColl_GroundProbe(CHL_FIELD(float, pickup, PU_POS), CHL_F32(pickup, PU_RADIUS), NULL, NULL, 0);
		dx = chl_time_scale * CHL_F32(pickup, PU_VEL + 0x0);
		dz = chl_time_scale * CHL_F32(pickup, PU_VEL + 0x8);
		dy = chl_time_scale * CHL_F32(pickup, PU_VEL + 0x4) - chl_time_scale * chl_time_scale * g * 0.5f;
		if (h <= -dy) {
			CHL_I32(pickup, PU_FLAGS) = 0;
			CHL_F32(pickup, PU_POS + 0x4) = CHL_F32(pickup, PU_POS + 0x4) - h;
		} else {
			CHL_F32(pickup, PU_VEL + 0x4) = CHL_F32(pickup, PU_VEL + 0x4) - chl_time_scale * g;
			CHL_F32(pickup, PU_POS + 0x0) = dx + CHL_F32(pickup, PU_POS + 0x0);
			CHL_F32(pickup, PU_POS + 0x4) = dy + CHL_F32(pickup, PU_POS + 0x4);
			CHL_F32(pickup, PU_POS + 0x8) = dz + CHL_F32(pickup, PU_POS + 0x8);
		}
	}
	glow = pu_EffectUpdate(CHL_FIELD(void, pickup, PU_EFFECT));
	CHL_F32(pickup, PU_GLOW) = glow * CHL_F32(pickup, PU_GLOW_SCALE);
	memcpy(CHL_FIELD(char, pickup, PU_GLOW_POS), CHL_FIELD(char, pickup, PU_POS), 0x10);
	CHL_F32(pickup, PU_GLOW_POS + 0x4) = pickup_glow_height * CHL_F32(pickup, PU_GLOW) +
		CHL_F32(pickup, PU_GLOW_POS + 0x4);
	if (CHL_I32(pickup, PU_EFFECT_STATE) == 1) {
		float s = CHL_F32(pickup, PU_GLOW);

		CHL_F32(pickup, PU_SCALE + 0x0) = s;
		CHL_F32(pickup, PU_SCALE + 0x4) = s;
		CHL_F32(pickup, PU_SCALE + 0x8) = s;
	} else if (CHL_I32(pickup, PU_EFFECT_STATE) == 3) {
		for (i = 0; i < chl_player_count; i++) {
			if (pickup_dist_sq(CHL_FIELD(float, CHL_PTR(chl_ai_players[i], AI_ACTOR), 0xd0),
				    CHL_FIELD(float, pickup, PU_POS)) <
			    pickup_collect_dist * pickup_collect_dist) {
				void *ai = chl_ai_players[i];
				int type = CHL_I32(pickup, PU_TYPE);

				CHL_F32(ai, AI_TIMERS + type * 4) = (float)(int)*(short *)&pickup_types[type * 3 + 2];
				AI_VCALL(ai, 15, void (*)(void *, int))(ai, CHL_I32(pickup, PU_TYPE));
				return Pickup_Delete(pickup);
			}
		}
	}
	chl_UpdateAnimations(CHL_FIELD(void, pickup, PU_INST), CHL_PTR(pickup, PU_NODE), chl_time_scale, NULL);
	return pickup;
}
