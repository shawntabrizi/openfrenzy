/*
CharLib/unplaced_actor.c  (charlib)

CharLib functions after the last Actor.cpp tag (0x000afa10) whose source file is not
known: the actor loader without a package and the actor draw passes. They work on the
actor (charlib_actor.h) like Actor.cpp; probably the end of Actor.cpp.
*/

#include "charlib.h"
#include "charlib_actor.h"

/* fog of the actors (Actor.c) */
#define actor_fog_enable CHL_VAR(int, 0x002f2878)
#define actor_fog_start CHL_VAR(float, 0x002f287c)
#define actor_fog_end CHL_VAR(float, 0x002f2880)
#define actor_fog_r CHL_VAR(int, 0x002f2884)
#define actor_fog_g CHL_VAR(int, 0x002f2888)
#define actor_fog_b CHL_VAR(int, 0x002f288c)

/* 0x00266f68  texture 1 matrix of the "glow" actors (flag 0x10000): scale 1, offset 0.1 */
static const D3DMATRIX actor_glow_texture_matrix = {
	{ { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.1f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } }
};

/* 0x000afca0  CharLib (file unknown)
Actor_Load without a package (the name is searched in every loaded package). */
void *Actor_LoadAny(const char *name, float *pos, int type, void *mem, int no_collision)
{
	return Actor_Load(name, NULL, pos, type, mem, no_collision);
}

/* 0x000afcd0  CharLib (file unknown)
Draws the first pass of an opaque actor (alpha below 0x100 with flags 0x600 is skipped):
fog, animated textures, the glow texture stage for flag 0x10000, lighting, blending for
faded actors and the shadow-receiver states (flags 0x80000, 0x200000); type 5 actors
draw inside draw state 3. */
void Actor_DrawFirstPass(void *actor)
{
	int state;
	unsigned int value;

	if ((CHL_U32(actor, ACT_FLAGS) & 0x600) != 0 && CHL_I32(actor, ACT_ALPHA) < 0x100)
		return;
	if (CHL_I32(actor, ACT_TYPE) == 5)
		chl_PushDrawState(3, 0, 0);
	if (CHL_I32(actor, ACT_FOG_OWN) == 0) {
		if (actor_fog_enable == 0 || (CHL_U32(actor, ACT_FLAGS) & 0x20000) != 0) {
			chl_SetRenderState(8, 0, 0);
		} else {
			chl_SetRenderState(8, 1, 0);
			chl_SetFogRange(actor_fog_start, actor_fog_end);
			chl_SetFogColour(actor_fog_r, actor_fog_g, actor_fog_b);
		}
	} else {
		chl_SetRenderState(8, 1, 0);
		chl_SetFogRange(CHL_F32(actor, ACT_FOG_OWN + 0x4), CHL_F32(actor, ACT_FOG_OWN + 0x8));
		chl_SetFogColour(CHL_I32(actor, ACT_FOG_OWN + 0xc), CHL_I32(actor, ACT_FOG_OWN + 0x10),
			CHL_I32(actor, ACT_FOG_OWN + 0x14));
	}
	chl_AnimTexApplyList(CHL_FIELD(void, actor, ACT_ANIMTEX));
	chl_SetRenderState(7, 3, 0);
	if ((CHL_U32(actor, ACT_FLAGS) & 0x10000) != 0) {
		chl_SetBrightness(0x100);
		Actor_SetConstantAlphaStage(0x50);
		D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)3, &actor_glow_texture_matrix);
	}
	chl_SetRenderState(5, 1, 0);
	if ((CHL_U32(actor, ACT_FLAGS) & 0x600) == 0) {
		state = 6;
		value = 0;
	} else {
		chl_SetRenderState(6, 1, 0);
		state = 10;
		value = (CHL_U32(actor, ACT_FLAGS) & 0x400) != 0;
	}
	chl_SetRenderState(state, value, 0);
	if ((CHL_U32(actor, ACT_FLAGS) & 0x80000) == 0) {
		if ((CHL_U32(actor, ACT_FLAGS) & 0x200000) != 0)
			Lights_EnableAll(0, 1);
		chl_DrawInstance(ACT_INST(actor), 1);
		if ((CHL_U32(actor, ACT_FLAGS) & 0x200000) != 0)
			Lights_EnableAll(1, 1);
	} else {
		chl_PushDrawState(3, 0, 0);
		chl_DrawInstance(ACT_INST(actor), 1);
		chl_PopDrawState(3);
	}
	chl_SetRenderState(1, 2, 0);
	chl_SetRenderState(6, 1, 0);
	if ((CHL_U32(actor, ACT_FLAGS) & 0x10000) != 0) {
		D3DDevice_SetTextureState_TexCoordIndex(1, 1);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 0);
	}
	if (CHL_I32(actor, ACT_TYPE) == 5)
		chl_PopDrawState(3);
}

/* draws the first pass of one actor with fog unless the actor has none */
static void actor_draw_simple(void *actor)
{
	unsigned int flags = CHL_U32(actor, ACT_FLAGS);

	chl_SetRenderState(8, (actor_fog_enable == 0 || (flags & 0x20000) != 0) ? 0 : 1, 0);
	chl_AnimTexApplyList(CHL_FIELD(void, actor, ACT_ANIMTEX));
	chl_DrawInstance(ACT_INST(actor), 1);
}

/* 0x000aff10  CharLib (file unknown)
Draws every actor: first the stencil shadow casters (flag 0x80000) and the unlit actors
(flag 0x200000) with the stencil set to write, then the other actors (Actor_DrawFirstPass),
then the faded pass and the opaque pass of all actors except those with flag 0x100000. */
void Actor_DrawAll(void)
{
	void *actor;

	chl_SetRenderState(6, 0, 0);
	chl_SetRenderState(3, 0, 0);
	chl_SetRenderState(5, 1, 0);
	chl_SetRenderState(7, 3, 0);
	chl_SetBrightness(0x100);
	chl_SetTexture(0, NULL);
	chl_SetTexture(1, NULL);
	if (actor_fog_enable != 0) {
		chl_SetFogRange(actor_fog_start, actor_fog_end);
		chl_SetFogColour(actor_fog_r, actor_fog_g, actor_fog_b);
	}
	D3DDevice_SetRenderState_StencilEnable(1);
	D3DDevice_SetRenderState(D3DRS_STENCILPASS, 0);
	D3DDevice_SetRenderState(D3DRS_STENCILFUNC, 0x207);
	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL;
	     actor = chl_ListNext(chl_actor_list, actor)) {
		unsigned int flags = CHL_U32(actor, ACT_FLAGS);

		if ((flags & 0x80000) != 0 && (flags & 0x300400) == 0)
			actor_draw_simple(actor);
	}
	chl_SetRenderState(3, 1, 0);
	Lights_EnableAll(0, 1);
	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL;
	     actor = chl_ListNext(chl_actor_list, actor)) {
		unsigned int flags = CHL_U32(actor, ACT_FLAGS);

		if ((flags & 0x200000) != 0 && (flags & 0x180400) == 0)
			actor_draw_simple(actor);
	}
	Lights_EnableAll(1, 1);
	chl_SetRenderState(4, 1, 0);
	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL;
	     actor = chl_ListNext(chl_actor_list, actor)) {
		if ((CHL_U32(actor, ACT_FLAGS) & 0x380400) == 0)
			Actor_DrawFirstPass(actor);
	}
	D3DDevice_SetRenderState_StencilEnable(0);
	D3DDevice_SetRenderState(D3DRS_STENCILPASS, 0x1e00);
	chl_SetRenderState(4, 0, 0);
	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL;
	     actor = chl_ListNext(chl_actor_list, actor)) {
		if ((CHL_U32(actor, ACT_FLAGS) & 0x100000) == 0)
			Actor_DrawFaded(actor);
	}
	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL;
	     actor = chl_ListNext(chl_actor_list, actor)) {
		if ((CHL_U32(actor, ACT_FLAGS) & 0x100000) == 0)
			Actor_DrawOpaque(actor);
	}
}
