/*
CharLib/Actor.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\Actor.cpp (tags at 0x000adef0, 0x000af1a0 and
0x000afa10; the functions between them are bracketed).

The CharLib actor: a Babel actor instance (bactor) with position, velocity, heading,
an animation table, fading, collision and shadow meshes, and the character data of the
players. Every actor is in the list chl_actor_list (a ClassLib list, the node is the first
member of the actor). Field macros: charlib_actor.h.

0x000ae6a0 and 0x000ae780 are animation callbacks that Ghidra did not find (they are not
in src/include/of_functions.tsv; reported).
*/

#include "charlib.h"
#include "charlib_actor.h"

/* 0x002ed418  the CProjector list (a ClassLib list object, classlib_fx .bss) */
extern unsigned char DAT_002ed418[0xc];

/* 0x00292c84  gravity per frame (4) */
#define actor_gravity CHL_VAR(float, 0x00292c84)
/* 0x00292c88  lowest y an actor can reach (-2000) */
#define actor_floor_y CHL_VAR(float, 0x00292c88)
/* 0x00292c8c  frames of reduced gravity after a jump (10) */
#define actor_jump_frames CHL_VAR(float, 0x00292c8c)
/* 0x00292c90  gravity factor while the jump timer runs (0.2) */
#define actor_jump_gravity CHL_VAR(float, 0x00292c90)
/* 0x00292c94  scale the jump gravity by the time left (1) */
#define actor_jump_gravity_ramp CHL_VAR(int, 0x00292c94)
/* 0x00293920  global animation speed (1.0) */
#define actor_anim_speed CHL_VAR(float, 0x00293920)
/* 0x002f2878  fog for actors: enabled, then start, end, r, g, b (0x002f287c..0x002f288c) */
#define actor_fog_enable CHL_VAR(int, 0x002f2878)
#define actor_fog_start CHL_VAR(float, 0x002f287c)
#define actor_fog_end CHL_VAR(float, 0x002f2880)
#define actor_fog_r CHL_VAR(int, 0x002f2884)
#define actor_fog_g CHL_VAR(int, 0x002f2888)
#define actor_fog_b CHL_VAR(int, 0x002f288c)
/* 0x002f2890  highest height of a player character above actor_height_base */
#define actor_max_height CHL_VAR(float, 0x002f2890)
#define actor_height_base CHL_VAR(float, 0x002f2894)

/* the character table of classlib_ui (0x002906f8, 0x60 bytes per character); the float at
+0x2c is the per-frame ground friction of the character */
#define actor_character_friction(c) CHL_F32(CHL_ADDR_CLU_CHARS, (c) * 0x60 + 0x2c)
extern unsigned char DAT_002906f8[6 * 0x60];
#define CHL_ADDR_CLU_CHARS DAT_002906f8
/* 0x002baed4  the player table of core (0x28 bytes per player); +0x00 is the character */
extern int DAT_002baed4[];
#define actor_player_character(p) DAT_002baed4[(p) * 10]

/* 0x000adef0  CharLib/Actor.cpp
Allocates the character data of a player-type actor (flags 0x08 or 0x10) and the 0x40-byte
block at 0x188; other actors get no character data. */
void Actor_InitCharData(void *actor)
{
	void *cd;
	float *block;

	if ((CHL_U8(actor, ACT_FLAGS) & 0x18) == 0) {
		CHL_PTR(actor, ACT_CHARDATA) = NULL;
		return;
	}
	cd = chl_Calloc(0x204, 0, "C:\\BlitzParty\\Game\\Src\\CharLib\\Actor.cpp", 0x2bd, 0x2001);
	CHL_PTR(actor, ACT_CHARDATA) = cd;
	CHL_F32(cd, 0x9c) = 118.0f;
	CHL_F32(ACT_CD(actor), 0x90) = 0.0f;
	CHL_F32(ACT_CD(actor), 0x80) = 118.0f;
	CHL_F32(ACT_CD(actor), 0xa0) = 60.0f;
	CHL_F32(ACT_CD(actor), CD_STUN_TIME) = 0.0f;
	CHL_U8(ACT_CD(actor), 0x15d) = 1;
	CHL_I32(ACT_CD(actor), CD_PLATFORM_FRAMES) = 0;
	block = chl_Calloc(0x40, 0, "C:\\BlitzParty\\Game\\Src\\CharLib\\Actor.cpp", 0x2cd, 0x2001);
	CHL_PTR(actor, ACT_BLOCK188) = block;
	block[0] = 10.0f;
}

/* 0x000adfc0  CharLib/Actor.cpp (bracketed)
Deletes an actor: unlinks it from the list, from the AI players (0x10bc/0x10c0), from the
projectors that follow it and from the other actors' character data, frees its data and
instances, and frees the actor unless it is type 2 (static storage). Returns the list
node before it (to continue an iteration). */
void *Actor_Delete(void *actor)
{
	void *prev = chl_ListRemove(chl_actor_list, actor);
	void *node;
	void *cd;
	int i;

	for (i = 0; i < chl_player_count; i++) {
		void *ai = CHL_ARR(void *, 0x002f289c)[i];

		if (ai != NULL) {
			if (CHL_PTR(ai, 0x10bc) == actor)
				CHL_PTR(ai, 0x10bc) = NULL;
			if (CHL_PTR(CHL_ARR(void *, 0x002f289c)[i], 0x10c0) == actor)
				CHL_PTR(CHL_ARR(void *, 0x002f289c)[i], 0x10c0) = NULL;
		}
	}
	node = chl_ListNext(DAT_002ed418, NULL);
	while (node != NULL) {
		if (CHL_PTR(node, 0x34) == ACT_INST(actor))
			node = ((void *(*)(void *))FUN_00083b60)(node);
		node = chl_ListNext(DAT_002ed418, node);
	}
	for (node = chl_ListNext(chl_actor_list, NULL); node != NULL; node = chl_ListNext(chl_actor_list, node)) {
		if (node == actor || (cd = ACT_CD(node)) == NULL)
			continue;
		/* cut the lists of actors at 0x160 (count 0x199) and 0x17c (count 0x198) at the
		first entry that is this actor */
		for (i = 0; i < (int)CHL_U8(cd, 0x199); i++) {
			if (CHL_PTR(cd, 0x160 + i * 4) == actor)
				CHL_U8(cd, 0x199) = (unsigned char)i;
			cd = ACT_CD(node);
		}
		cd = ACT_CD(node);
		for (i = 0; i < (int)CHL_U8(cd, 0x198); i++) {
			if (CHL_PTR(cd, 0x17c + i * 4) == actor)
				CHL_U8(cd, 0x198) = (unsigned char)i;
			cd = ACT_CD(node);
		}
	}
	chl_Free(CHL_PTR(actor, ACT_CHARDATA));
	CHL_PTR(actor, ACT_CHARDATA) = NULL;
	chl_Free(CHL_PTR(actor, ACT_BLOCK188));
	CHL_PTR(actor, ACT_BLOCK188) = NULL;
	chl_AnimTexDeleteList(CHL_FIELD(void, actor, ACT_ANIMTEX));
	if (CHL_PTR(actor, ACT_COLLMESH) != NULL) {
		chl_Free(*(void **)CHL_PTR(actor, ACT_COLLMESH));
		*(void **)CHL_PTR(actor, ACT_COLLMESH) = NULL;
		chl_Free(CHL_PTR(actor, ACT_COLLMESH));
		CHL_PTR(actor, ACT_COLLMESH) = NULL;
	}
	if (CHL_PTR(actor, ACT_COLL_INST) != NULL)
		chl_DeleteInstance(CHL_PTR(actor, ACT_COLL_INST));
	if (CHL_PTR(actor, ACT_SHADOW_INST) != NULL)
		chl_DeleteInstance(CHL_PTR(actor, ACT_SHADOW_INST));
	chl_DeleteInstance(ACT_INST(actor));
	if (CHL_I32(actor, ACT_TYPE) != 2)
		chl_Free(actor);
	return prev;
}

/* 0x000ae1b0  CharLib/Actor.cpp (bracketed)
Draws the shadow-mesh pass (pass 1) of the actors of type 0 or 2 that have one (flag
0x400000): the shadow instance, or the actor's own instance. */
void Actor_DrawShadowMeshes(void)
{
	void *actor;

	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL;
	     actor = chl_ListNext(chl_actor_list, actor)) {
		if ((CHL_I32(actor, ACT_TYPE) == 0 || CHL_I32(actor, ACT_TYPE) == 2) &&
		    (CHL_U32(actor, ACT_FLAGS) & 0x400000) != 0) {
			void *inst = CHL_PTR(actor, ACT_SHADOW_INST);

			if (inst == NULL)
				inst = ACT_INST(actor);
			chl_DrawInstance(inst, 1);
		}
	}
}

/* 0x000ae210  CharLib/Actor.cpp (bracketed)
Draws the opaque pass (pass 6) of an actor that is not faded (no flag 0x600 or alpha
>= 0x100). Flags 0x80005 draw with state 3 of FUN_00046cb0; flag 0x200000 switches the
CharLib lights off around the draw. */
void Actor_DrawOpaque(void *actor)
{
	if ((CHL_U32(actor, ACT_FLAGS) & 0x600) != 0 && CHL_I32(actor, ACT_ALPHA) < 0x100)
		return;
	chl_SetBrightness(0x100);
	chl_SetRenderState(6, 1, 0);
	chl_SetRenderState(10, (CHL_U32(actor, ACT_FLAGS) & 0x400) != 0, 0);
	chl_SetRenderState(5, 0, 0);
	if ((CHL_U32(actor, ACT_FLAGS) & 0x80005) != 0) {
		chl_PushDrawState(3, 0, 0);
		chl_DrawInstance(ACT_INST(actor), 6);
		chl_PopDrawState();
		return;
	}
	if ((CHL_U32(actor, ACT_FLAGS) & 0x200000) != 0)
		Lights_EnableAll(0, 1);
	chl_DrawInstance(ACT_INST(actor), 6);
	if ((CHL_U32(actor, ACT_FLAGS) & 0x200000) != 0)
		Lights_EnableAll(1, 1);
}

/* 0x000ae2f0  CharLib/Actor.cpp (bracketed)
Draws the transparent pass (pass 7) of a faded actor (flag 0x600, alpha below 0x100): fog
from the actor (0x340) or the global actor fog, the animated textures, blending, and the
brightness set to the alpha. */
void Actor_DrawFaded(void *actor)
{
	int alpha;

	if ((CHL_U32(actor, ACT_FLAGS) & 0x600) == 0)
		return;
	if (CHL_I32(actor, ACT_ALPHA) == 0x100)
		return;
	if (CHL_I32(actor, ACT_TYPE) == 5 || (CHL_U32(actor, ACT_FLAGS) & 0x80000) != 0)
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
	chl_SetRenderState(1, 2, 0);
	chl_SetRenderState(6, 1, 0);
	alpha = CHL_I32(actor, ACT_ALPHA);
	if ((CHL_U32(actor, ACT_FLAGS) & 0x400) == 0) {
		chl_SetRenderState(10, 0, 0);
		if (0xff < alpha) {
			chl_SetBrightness(0x100);
			chl_DrawInstance(ACT_INST(actor), 7);
			goto done;
		}
	} else {
		chl_SetRenderState(10, 1, 0);
		if (0xff < alpha) {
			chl_DrawInstance(ACT_INST(actor), 7);
			goto done;
		}
	}
	chl_SetBrightness(CHL_I32(actor, ACT_ALPHA));
	chl_DrawInstance(ACT_INST(actor), 7);
	chl_SetBrightness(0x100);
done:
	if (CHL_I32(actor, ACT_TYPE) != 5 && (CHL_U32(actor, ACT_FLAGS) & 0x80000) == 0)
		return;
	chl_PopDrawState();
}

/* 0x000ae4d0  CharLib/Actor.cpp (bracketed)
Builds the orientation of every actor that is not locked (0x320) from its Euler angles
(unless flag 2: the orientation is set directly) and prepares its instance matrices.
Type 1 actors (players) face the other way (reference angle pi). */
void Actor_PrepAll(void)
{
	void *actor;

	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL;
	     actor = chl_ListNext(chl_actor_list, actor)) {
		if (CHL_I32(actor, ACT_NO_PREP) != 0)
			continue;
		if ((CHL_U8(actor, ACT_FLAGS) & 2) == 0)
			FUN_00070c10(CHL_FIELD(float, actor, ACT_ORIENT), CHL_FIELD(float, actor, ACT_EULER),
				CHL_I32(actor, ACT_TYPE) == 1 ? 3.1415925f : 0.0f);
		chl_PrepInstance(ACT_INST(actor), 1);
	}
}

/* copies `name` up to its first '.', appends `suffix`, logs the result */
static void actor_mesh_name(char *out, const char *name, const char *suffix)
{
	char *p = out;
	char c;

	for (c = *name; c != '\0' && c != '.'; c = *++name)
		*p++ = c;
	while (*suffix != '\0')
		*p++ = *suffix++;
	*p = '\0';
	chl_Printf("mesh name = %s\n", out);
}

/* 0x000ae550  CharLib/Actor.cpp (bracketed)
"<name without extension>_coll.obe": the collision mesh of an actor. */
void Actor_CollMeshName(char *out, const char *name)
{
	actor_mesh_name(out, name, "_coll.obe");
}

/* 0x000ae5d0  CharLib/Actor.cpp (bracketed)
"<name without extension>_shad.obe": the shadow mesh of an actor. */
void Actor_ShadowMeshName(char *out, const char *name)
{
	actor_mesh_name(out, name, "_shad.obe");
}

/* 0x000ae650  CharLib/Actor.cpp (bracketed)
Progress (0..1) of the animation that plays on the root node of an instance: (time -
start) / (end - start) of the node's current animation slot; 0 without one. */
float Actor_AnimProgress(void *inst)
{
	void *node = CHL_PTR(inst, 0x134);
	unsigned char *slot = (unsigned char *)CHL_PTR(node, 0xc) + CHL_U8(node, 0x78) * 0x40;
	void *anim = CHL_PTR(slot, 0x8);
	int done;
	double t;

	if (anim == NULL)
		return 0.0f;
	done = CHL_I32(slot, 0x18) - CHL_I32(anim, 0x4);
	t = (double)done;
	if (done < 0)
		t = t + 4294967296.0;
	return (float)(t / (double)(CHL_I32(anim, 0x8) - CHL_I32(anim, 0x4)));
}

/* 0x000ae6a0  CharLib/Actor.cpp (not in the function table)
Animation callback that loops: when the animation on the root node finishes, flags the
actor (0x232, 0x234) and queues the same animation again; a player actor whose AI has a
held object (0x10c0) also restarts animation 3 on the AI's hold node (0x10cc). */
int Actor_AnimLoopCallback(int reason, void *inst, void *node, void *anim, void *actor)
{
	void *ai;

	if (reason != 0 || node != CHL_PTR(inst, 0x134))
		return 0;
	CHL_U16(actor, ACT_ANIM_EVENT) = 1;
	CHL_U16(actor, ACT_ANIM_DONE) = 1;
	chl_QueueAnimation(inst, CHL_PTR(inst, 0x134), anim, 0x400, -1, 0,
		(CHLAnimCallback)Actor_AnimLoopCallback, actor);
	if (CHL_I32(actor, ACT_TYPE) == 1) {
		ai = CHL_ARR(void *, 0x002f289c)[CHL_U16(actor, ACT_PLAYER)];
		if (ai != NULL && CHL_PTR(ai, 0x10c0) != NULL) {
			chl_FlushAnimations(inst, CHL_PTR(ai, 0x10cc), 1, 0);
			if (CHL_PTR(actor, ACT_ANIMS + 3 * 4) != NULL)
				chl_QueueAnimation(inst,
					CHL_PTR(CHL_ARR(void *, 0x002f289c)[CHL_U16(actor, ACT_PLAYER)], 0x10cc),
					CHL_PTR(actor, ACT_ANIMS + 3 * 4), 0x400, -1, 0,
					(CHLAnimCallback)Actor_AnimLoopCallback, actor);
		}
	}
	return 0;
}

/* 0x000ae780  CharLib/Actor.cpp (not in the function table)
Animation callback of queued animations: when the animation on the root node finishes,
takes the next speed from the queue (0x240, 0x236 entries) into 0x23c, or, with an empty
queue, stops the actor's animation speed and sets the done flag. */
int Actor_AnimQueueCallback(int reason, void *inst, void *node, void *anim, void *actor)
{
	unsigned short queued;

	(void)anim;
	if (reason != 0 || node != CHL_PTR(inst, 0x134))
		return 0;
	queued = CHL_U16(actor, ACT_ANIM_QUEUED);
	CHL_U16(actor, ACT_ANIM_EVENT) = 1;
	if (queued == 0) {
		CHL_F32(actor, ACT_ANIM_RATE) = 0.0f;
		CHL_U16(actor, ACT_ANIM_DONE) = 1;
		return 0;
	}
	CHL_U32(actor, ACT_ANIM_RATE) = CHL_U32(actor, ACT_ANIM_QUEUE);
	CHL_U16(actor, ACT_ANIM_QUEUED) = (unsigned short)(queued - 1);
	CHL_U32(actor, ACT_ANIM_QUEUE + 0x0) = CHL_U32(actor, ACT_ANIM_QUEUE + 0x4);
	CHL_U32(actor, ACT_ANIM_QUEUE + 0x4) = CHL_U32(actor, ACT_ANIM_QUEUE + 0x8);
	CHL_U32(actor, ACT_ANIM_QUEUE + 0x8) = CHL_U32(actor, ACT_ANIM_QUEUE + 0xc);
	CHL_U32(actor, ACT_ANIM_QUEUE + 0xc) = CHL_U32(actor, ACT_ANIM_QUEUE + 0x10);
	return 0;
}

/* 0x000ae820  CharLib/Actor.cpp (bracketed)
Fades the animated-texture level (0x334) to `target` over `frames` frames (at once when
frames is 0). */
void Actor_SetFade(void *actor, float target, float frames)
{
	if (frames == 0.0f) {
		CHL_F32(actor, ACT_FADE_RATE) = 0.0f;
		CHL_F32(actor, ACT_FADE) = target;
		return;
	}
	CHL_F32(actor, ACT_FADE_TARGET) = target;
	CHL_F32(actor, ACT_FADE_RATE) = (target - CHL_F32(actor, ACT_FADE)) / frames;
}

/* 0x000ae870  CharLib/Actor.cpp (bracketed)
The per-frame update of an actor: fading, alpha steps, the death fade of player-type
actors (types 1, 0xb, 0xf), platform riding, gravity and jumps, friction, the ground
floor, turning towards the wanted heading and the collision response (FUN_000b6270).
Returns the list node to continue the update loop from (the actor, or the node before
it when the actor was moved to the end of the list or deleted). */
void *Actor_Update(void *actor)
{
	unsigned int flags;
	int target;
	int alpha;
	int step;
	float f;
	void *cd;
	void *platform;
	void **platform_slot;
	float v[4];
	float m[16];

	/* animated-texture fade */
	if (CHL_F32(actor, ACT_FADE_RATE) != 0.0f) {
		f = chl_time_scale * CHL_F32(actor, ACT_FADE_RATE);
		if (fabsf(f) <= fabsf(CHL_F32(actor, ACT_FADE) - CHL_F32(actor, ACT_FADE_TARGET)))
			CHL_F32(actor, ACT_FADE) = f + CHL_F32(actor, ACT_FADE);
		else
			CHL_F32(actor, ACT_FADE) = CHL_F32(actor, ACT_FADE_TARGET);
	}
	chl_AnimTexAdvanceList(CHL_FIELD(void, actor, ACT_ANIMTEX), CHL_F32(actor, ACT_FADE), 0);

	/* alpha steps towards ACT_ALPHA_TARGET */
	flags = CHL_U32(actor, ACT_FLAGS);
	if ((flags & 0x600) != 0 && -1 < (target = CHL_I32(actor, ACT_ALPHA_TARGET)) &&
	    CHL_I32(actor, ACT_ALPHA) != target) {
		if (CHL_I32(actor, ACT_ALPHA_STEPS) == 0)
			CHL_I32(actor, ACT_ALPHA_STEPS) = 0x14;
		alpha = CHL_I32(actor, ACT_ALPHA);
		if (alpha < target) {
			step = (target - alpha) / CHL_I32(actor, ACT_ALPHA_STEPS);
			if (step < 1)
				step = 1;
		} else {
			step = (alpha - target) / CHL_I32(actor, ACT_ALPHA_STEPS);
			if (step < 1)
				step = 1;
			step = -step;
		}
		CHL_I32(actor, ACT_ALPHA) = alpha + step;
	}

	if (CHL_I32(actor, ACT_TYPE) != 1 && CHL_I32(actor, ACT_TYPE) != 0xb && CHL_I32(actor, ACT_TYPE) != 0xf)
		return actor;
	CHL_U32(actor, ACT_FLAGS) = flags & 0xffff7fff;

	/* death timer: then fade out (flag 0x200) and delete */
	if (0.0f < CHL_F32(actor, ACT_DEATH_TIME)) {
		f = CHL_F32(actor, ACT_DEATH_TIME) - chl_time_scale;
		CHL_F32(actor, ACT_DEATH_TIME) = f;
		if (f <= 0.0f) {
			CHL_F32(actor, ACT_DEATH_TIME) = 0.001f;
			if ((flags & 0x200) == 0) {
				void *prev;

				CHL_U32(actor, ACT_FLAGS) = (flags & 0xffff7fff) | 0x200;
				prev = chl_ListRemove(chl_actor_list, actor);
				chl_ListInsert(chl_actor_list, actor, 1);
				return prev;
			}
			alpha = CHL_I32(actor, ACT_ALPHA) - CHL_FTOL(chl_time_scale * CHL_F32(actor, ACT_FADE_SPEED));
			CHL_I32(actor, ACT_ALPHA) = alpha;
			CHL_I32(actor, ACT_ALPHA_TARGET) = alpha;
			if (alpha < 1)
				return Actor_Delete(actor);
		}
	}

	/* stunned: FUN_000b1d20 moves the actor */
	cd = ACT_CD(actor);
	if (0.0f < CHL_F32(cd, CD_STUN_TIME)) {
		Actor_UpdateKnockback(actor);
		cd = ACT_CD(actor);
		if (0.0f < CHL_F32(cd, CD_STUN_TIME))
			return actor;
	}

	/* platform: take over its movement for 3 frames after leaving it */
	platform = CHL_PTR(cd, CD_PLATFORM);
	if (platform == NULL || CHL_PTR(platform, 0x448) == NULL) {
		if (0 < CHL_I32(cd, CD_PLATFORM_FRAMES))
			CHL_I32(cd, CD_PLATFORM_FRAMES) = CHL_I32(cd, CD_PLATFORM_FRAMES) - 1;
	} else {
		if (0 < CHL_I32(cd, CD_PLATFORM_FRAMES)) {
			Platform_GetVelocity(v, platform, 1);
			Platform_AddVelocity(CHL_FIELD(float, actor, ACT_VEL), v);
		}
		CHL_I32(ACT_CD(actor), CD_PLATFORM_FRAMES) = 3;
	}
	cd = ACT_CD(actor);
	platform = CHL_PTR(cd, CD_PLATFORM);
	platform_slot = CHL_FIELD(void *, cd, CD_PLATFORM);
	if (platform != NULL && platform == CHL_PTR(cd, CD_PLATFORM_PREV)) {
		/* the platform turned: rotate the actor about the platform's pivot */
		if (CHL_F32(platform, 0x1f4) != CHL_F32(platform, 0x370) ||
		    CHL_F32(platform, 0x1f8) != CHL_F32(platform, 0x374) ||
		    CHL_F32(platform, 0x1fc) != CHL_F32(platform, 0x378)) {
			v[0] = CHL_F32(actor, ACT_POS + 0x0) - CHL_F32(platform, 0x1b4);
			v[1] = CHL_F32(actor, ACT_POS + 0x4) - CHL_F32(platform, 0x1b8);
			v[2] = CHL_F32(actor, ACT_POS + 0x8) - CHL_F32(platform, 0x1bc);
			v[3] = 1.0f;
			platform = *platform_slot;
			chl_MatFromEuler(m, -(CHL_F32(platform, 0x1f4) - CHL_F32(platform, 0x370)),
				CHL_F32(platform, 0x1f8) - CHL_F32(platform, 0x374),
				-(CHL_F32(platform, 0x1fc) - CHL_F32(platform, 0x378)));
			chl_MatTransform(m, v);
			platform = CHL_PTR(ACT_CD(actor), CD_PLATFORM);
			CHL_F32(actor, ACT_POS + 0x0) = v[0] + CHL_F32(platform, 0x1b4);
			CHL_F32(actor, ACT_POS + 0x4) = v[1] + CHL_F32(platform, 0x1b8);
			CHL_F32(actor, ACT_POS + 0x8) = v[2] + CHL_F32(platform, 0x1bc);
			platform_slot = CHL_FIELD(void *, ACT_CD(actor), CD_PLATFORM);
			CHL_F32(actor, ACT_HEADING_WANTED) = (CHL_F32(*platform_slot, 0x1f8) -
				CHL_F32(*platform_slot, 0x374)) + CHL_F32(actor, ACT_HEADING_WANTED);
		}
		/* and move with it */
		platform = *platform_slot;
		v[0] = CHL_F32(platform, 0xe0) - CHL_F32(platform, 0x1b4);
		v[1] = CHL_F32(platform, 0xe4) - CHL_F32(platform, 0x1b8);
		v[2] = CHL_F32(platform, 0xe8) - CHL_F32(platform, 0x1bc);
		CHL_F32(actor, ACT_POS + 0x0) = v[0] + CHL_F32(actor, ACT_POS + 0x0);
		CHL_F32(actor, ACT_POS + 0x4) = v[1] + CHL_F32(actor, ACT_POS + 0x4);
		CHL_F32(actor, ACT_POS + 0x8) = v[2] + CHL_F32(actor, ACT_POS + 0x8);
		if (CHL_F32(*platform_slot, 0x24c) != 0.0f && (CHL_U32(*platform_slot, 0x450) & 0x800) == 0)
			Platform_RotateActor(actor);
	}
	cd = ACT_CD(actor);
	if (CHL_PTR(cd, CD_PLATFORM) != NULL) {
		/* copy the platform's matrix (rows 1-3 at 0x154 + 0x10) */
		void *pm = CHL_PTR(CHL_PTR(cd, CD_PLATFORM), 0x154);

		memcpy(CHL_FIELD(char, cd, 0x1f4), CHL_FIELD(char, pm, 0x30), 0x10);
		cd = ACT_CD(actor);
		pm = CHL_PTR(CHL_PTR(cd, CD_PLATFORM), 0x154);
		memcpy(CHL_FIELD(char, cd, 0x1d4), CHL_FIELD(char, pm, 0x10), 0x10);
		cd = ACT_CD(actor);
		pm = CHL_PTR(CHL_PTR(cd, CD_PLATFORM), 0x154);
		memcpy(CHL_FIELD(char, cd, 0x1e4), CHL_FIELD(char, pm, 0x20), 0x10);
	}

	/* gravity and jumps (flag 1: affected by gravity) */
	if (CHL_F32(ACT_CD(actor), CD_STUN_TIME) == 0.0f) {
		v[2] = CHL_F32(actor, ACT_VEL + 0x8);
		v[0] = CHL_F32(actor, ACT_VEL + 0x0);
		v[1] = chl_time_scale * CHL_F32(actor, ACT_VEL + 0x4);
		v[3] = CHL_F32(actor, ACT_VEL + 0xc);
		if ((CHL_U8(actor, ACT_FLAGS) & 1) != 0) {
			if (CHL_F32(actor, ACT_JUMP_TIME) <= 0.0f) {
				f = CHL_F32(actor, ACT_VEL + 0x4) - actor_gravity * chl_time_scale;
				CHL_F32(actor, ACT_VEL + 0x4) = f;
				if (f <= 0.0f && CHL_F32(actor, ACT_JUMP_TIME) == -1.0f) {
					CHL_F32(actor, ACT_JUMP_TIME) = actor_jump_frames;
					CHL_F32(actor, ACT_VEL + 0x4) = 0.0f;
				}
			} else {
				f = actor_jump_gravity;
				if (actor_jump_gravity_ramp != 0)
					f = 1.0f - CHL_F32(actor, ACT_JUMP_TIME) / actor_jump_frames;
				CHL_F32(actor, ACT_VEL + 0x4) = CHL_F32(actor, ACT_VEL + 0x4) -
					f * actor_gravity * chl_time_scale;
				f = CHL_F32(actor, ACT_JUMP_TIME) - chl_time_scale;
				CHL_F32(actor, ACT_JUMP_TIME) = f;
				if (f <= 0.0f)
					CHL_F32(actor, ACT_JUMP_TIME) = 0.0f;
			}
		}
	}
	if (CHL_F32(ACT_CD(actor), CD_STUN_TIME) == 0.0f) {
		if ((CHL_U8(actor, ACT_FLAGS) & 1) != 0)
			v[1] = v[1] - actor_gravity * chl_time_scale * chl_time_scale * 0.5f;
		CHL_F32(actor, ACT_POS + 0x0) = v[0] + CHL_F32(actor, ACT_POS + 0x0);
		CHL_F32(actor, ACT_POS + 0x4) = v[1] + CHL_F32(actor, ACT_POS + 0x4);
		CHL_F32(actor, ACT_POS + 0x8) = v[2] + CHL_F32(actor, ACT_POS + 0x8);
	}
	if (CHL_I32(actor, ACT_TYPE) == 1 && CHL_I16(actor, ACT_PLAYER) == 0 && CHL_I32(actor, 0x214) == 0 &&
	    actor_max_height < (f = CHL_F32(actor, ACT_POS + 0x4) - actor_height_base))
		actor_max_height = f;

	/* friction on x and z: pow(friction, time scale) */
	if (CHL_F32(ACT_CD(actor), CD_STUN_TIME) == 0.0f) {
		if (CHL_I32(actor, ACT_TYPE) == 0xb) {
			CHL_F32(actor, ACT_VEL + 0x0) = (float)(pow((double)CHL_F32(actor, ACT_FRICTION),
				(double)chl_time_scale) * (double)CHL_F32(actor, ACT_VEL + 0x0));
			CHL_F32(actor, ACT_VEL + 0x8) = (float)(pow((double)CHL_F32(actor, ACT_FRICTION),
				(double)chl_time_scale) * (double)CHL_F32(actor, ACT_VEL + 0x8));
		} else if ((CHL_U32(actor, ACT_FLAGS) & 0x1000) == 0) {
			int c = actor_player_character(CHL_U16(actor, ACT_PLAYER));

			CHL_F32(actor, ACT_VEL + 0x0) = (float)(pow((double)actor_character_friction(c),
				(double)chl_time_scale) * (double)CHL_F32(actor, ACT_VEL + 0x0));
			c = actor_player_character(CHL_U16(actor, ACT_PLAYER));
			CHL_F32(actor, ACT_VEL + 0x8) = (float)(pow((double)actor_character_friction(c),
				(double)chl_time_scale) * (double)CHL_F32(actor, ACT_VEL + 0x8));
		}
	}

	/* the floor of the world */
	if (CHL_F32(actor, ACT_POS + 0x4) < actor_floor_y) {
		int falling = CHL_F32(actor, ACT_VEL + 0x4) < 0.0f;

		CHL_F32(actor, ACT_POS + 0x4) = actor_floor_y;
		if (falling)
			CHL_F32(actor, ACT_VEL + 0x4) = 0.0f;
	}

	/* turn towards the wanted heading */
	CHL_F32(actor, ACT_HEADING_WANTED) = FUN_000703a0(CHL_F32(actor, ACT_HEADING_WANTED));
	CLMath_TurnTowards(CHL_FIELD(float, actor, ACT_HEADING), CHL_F32(actor, ACT_HEADING_WANTED), 0.4f, 2.0f);
	memcpy(CHL_FIELD(char, actor, ACT_POS_PREMOVE), CHL_FIELD(char, actor, ACT_POS), 0x10);
	CRColl_MoveActor(actor);

	/* platforms that stop the actor (0x400) or bounce it (0x800) */
	platform = CHL_PTR(ACT_CD(actor), CD_PLATFORM);
	if (platform != NULL && (flags = CHL_U32(platform, 0x450), (flags & 0xc00) != 0)) {
		if ((flags & 0x400) == 0) {
			if (CHL_I16(actor, ACT_ANIM_DONE) != 0)
				CHL_F32(actor, ACT_VEL + 0x4) = CHL_F32(actor, ACT_VEL + 0x4) + 3.0f;
		} else {
			CHL_U32(actor, ACT_FLAGS) = CHL_U32(actor, ACT_FLAGS) | 0x100;
			memcpy(CHL_FIELD(char, actor, ACT_POS), CHL_FIELD(char, actor, ACT_POS_PREMOVE), 0x10);
			f = 1.0f / chl_time_scale;
			CHL_F32(actor, ACT_VEL + 0x0) = f * (CHL_F32(actor, ACT_POS + 0x0) - CHL_F32(actor, ACT_POS_PREV + 0x0));
			CHL_F32(actor, ACT_VEL + 0x4) = f * (CHL_F32(actor, ACT_POS + 0x4) - CHL_F32(actor, ACT_POS_PREV + 0x4));
			CHL_F32(actor, ACT_VEL + 0x8) = f * (CHL_F32(actor, ACT_POS + 0x8) - CHL_F32(actor, ACT_POS_PREV + 0x8));
		}
	}
	memcpy(CHL_FIELD(char, actor, ACT_POS_PREV), CHL_FIELD(char, actor, ACT_POS), 0x10);
	return actor;
}

/* 0x000af1a0  CharLib/Actor.cpp
Creates an actor from a loaded bactor model: allocates it (unless `mem` is given),
initialises the instance (6 animation slots), puts it at `pos` (or the origin), and, with
`link`, resets the headings and adds it to the actor list (at the tail when at_tail). */
void *Actor_Create(void *model, float *pos, void *mem, int type, int at_tail, int link)
{
	void *actor = mem;
	void *inst;

	if (model == NULL) {
		chl_Printf("*** ERROR *** Could not create instance\n");
		return NULL;
	}
	if (actor == NULL) {
		actor = chl_Calloc(0x360, 0, "C:\\BlitzParty\\Game\\Src\\CharLib\\Actor.cpp", 0x286, 0x2001);
		if (actor == NULL) {
			chl_Printf("*** ERROR *** Could not allocate memory for instance\n");
			return NULL;
		}
	}
	chl_ListInit2(CHL_FIELD(void, actor, ACT_ANIMTEX));
	inst = ACT_INST(actor);
	CHL_I32(actor, ACT_TYPE) = type;
	chl_InitInstance(inst, model, 6, "C:\\BlitzParty\\Game\\Src\\CharLib\\Actor.cpp", 0x28f);
	chl_SetRenderPass(inst, 0, 1, 0);
	chl_SetRenderPass(inst, 1, 0, 0);
	chl_SetNodeTime(inst, CHL_PTR(actor, ACT_NODE), 0, 1);
	chl_FlushAnimations(inst, CHL_PTR(actor, ACT_NODE), 1, 0);
	CHL_U16(actor, ACT_ANIM_QUEUED) = 0;
	CHL_F32(actor, ACT_ANIM_RATE) = 0.0f;
	if (pos == NULL) {
		CHL_F32(actor, ACT_POS + 0x0) = 0.0f;
		CHL_F32(actor, ACT_POS + 0x4) = 0.0f;
		CHL_F32(actor, ACT_POS + 0x8) = 0.0f;
		CHL_F32(actor, ACT_POS + 0xc) = 1.0f;
	} else {
		memcpy(CHL_FIELD(char, actor, ACT_POS), pos, 0x10);
	}
	chl_PrepInstance(inst, 1);
	CHL_F32(actor, ACT_VEL + 0x0) = 0.0f;
	CHL_F32(actor, ACT_VEL + 0x4) = 0.0f;
	CHL_F32(actor, ACT_VEL + 0x8) = 0.0f;
	CHL_F32(actor, ACT_VEL + 0xc) = 1.0f;
	if (link != 0) {
		CHL_F32(actor, ACT_HEADING) = 0.0f;
		CHL_F32(actor, ACT_HEADING_WANTED) = 0.0f;
		chl_ListInsert(chl_actor_list, actor, at_tail);
	}
	memcpy(CHL_FIELD(char, actor, ACT_POS_PREV), CHL_FIELD(char, actor, ACT_POS), 0x10);
	CHL_I32(actor, ACT_ALPHA_TARGET) = -1;
	return actor;
}

/* 0x000af350  CharLib/Actor.cpp (bracketed) */
void Actor_DeleteAll(void)
{
	void *actor = chl_ListNext(chl_actor_list, NULL);

	while (actor != NULL)
		actor = chl_ListNext(chl_actor_list, Actor_Delete(actor));
}

/* 0x000af380  CharLib/Actor.cpp (bracketed)
Advances the animations of every actor (and its collision instance) by the animation
rate, then places the objects the AI players hold (0x10bc, 0x10c0) at their hold nodes
(0x10c4, 0x10c8) and moves the AI's light (0x112c) with them. */
void Actor_UpdateAnimations(void)
{
	void *actor;
	void *coll;
	void *ai;
	void *held;
	float m[16];
	float pos[4];
	int player;
	int off;
	int prepared;

	for (actor = chl_ListNext(chl_actor_list, NULL); actor != NULL;
	     actor = chl_ListNext(chl_actor_list, actor)) {
		coll = CHL_PTR(actor, ACT_COLL_INST);
		CHL_U16(actor, ACT_ANIM_EVENT) = 0;
		if (coll != NULL)
			chl_UpdateAnimations(coll, CHL_PTR(coll, 0x134),
				chl_time_scale * CHL_F32(actor, ACT_ANIM_RATE) * actor_anim_speed, NULL);
		chl_UpdateAnimations(ACT_INST(actor), CHL_PTR(actor, ACT_NODE),
			chl_time_scale * CHL_F32(actor, ACT_ANIM_RATE) * actor_anim_speed, NULL);
	}
	for (player = 0; player < chl_player_count; player++) {
		void **slot = &CHL_ARR(void *, 0x002f289c)[player];

		if (*slot == NULL)
			continue;
		prepared = 0;
		for (off = 0x10bc; off < 0x10c4; off += 4) {
			ai = *slot;
			if (CHL_PTR(ai, off) == NULL)
				continue;
			if (!prepared) {
				void *body = CHL_PTR(ai, 0x30);

				FUN_00070c10(CHL_FIELD(float, body, ACT_ORIENT), CHL_FIELD(float, body, ACT_EULER),
					3.1415925f);
				chl_PrepInstance(ACT_INST(CHL_PTR(*slot, 0x30)), 1);
				prepared = 1;
			}
			if (CHL_PTR(*slot, off + 8) == NULL)
				continue;
			chl_PrepInstance(ACT_INST(CHL_PTR(*slot, off)), 1);
			held = CHL_PTR(*slot, off);
			chl_MatMul(m, CHL_FIELD(float, CHL_PTR(*slot, off + 8), 0x80), (float *)ACT_INST(held));
			held = CHL_PTR(*slot, off);
			chl_MatMul((float *)ACT_INST(held), (float *)ACT_INST(CHL_PTR(*slot, 0x30)), m);
			held = CHL_PTR(*slot, off);
			memcpy(pos, CHL_FIELD(char, held, 0x40), 0x10);
			if (CHL_PTR(*slot, 0x112c) != NULL)
				Light_SetPosition(CHL_PTR(*slot, 0x112c), pos);
		}
	}
}

/* 0x000af580  CharLib/Actor.cpp (bracketed)
Switches to animation `index`, starting at the same relative progress as the current
one, blending in over `blend`; a player holding an object restarts animation 3 on the
hold node too. */
void Actor_SwitchAnim(void *actor, int index, float blend)
{
	void *anim;
	void *inst;
	void *ai;
	int start;
	int time;

	CHL_U16(actor, ACT_ANIM_EVENT) = 0;
	CHL_U16(actor, ACT_ANIM_DONE) = 0;
	CHL_U16(actor, ACT_ANIM_QUEUED) = 0;
	anim = CHL_PTR(actor, ACT_ANIMS + index * 4);
	start = CHL_I32(anim, 0x4);
	inst = ACT_INST(actor);
	time = CHL_FTOL((double)Actor_AnimProgress(inst) * (double)(CHL_I32(anim, 0x8) - start)) + start;
	chl_FlushAnimations(inst, CHL_PTR(actor, ACT_NODE), 1, 0);
	chl_QueueAnimationBlendIn(inst, CHL_PTR(actor, ACT_NODE), CHL_PTR(actor, ACT_ANIMS + index * 4), 0x400,
		time, blend, 0, (CHLAnimCallback)Actor_AnimLoopCallback, actor);
	if (CHL_I32(actor, ACT_TYPE) == 1 &&
	    (ai = CHL_ARR(void *, 0x002f289c)[CHL_U16(actor, ACT_PLAYER)]) != NULL &&
	    CHL_PTR(ai, 0x10c0) != NULL) {
		chl_FlushAnimations(inst, CHL_PTR(ai, 0x10cc), 1, 0);
		if (CHL_PTR(actor, ACT_ANIMS + 3 * 4) != NULL)
			chl_QueueAnimation(inst, CHL_PTR(CHL_ARR(void *, 0x002f289c)[CHL_U16(actor, ACT_PLAYER)], 0x10cc),
				CHL_PTR(actor, ACT_ANIMS + 3 * 4), 0x400, -1, 0,
				(CHLAnimCallback)Actor_AnimLoopCallback, actor);
	}
}

/* 0x000af690  CharLib/Actor.cpp (bracketed)
Plays animation `index` at `speed` times its table speed: at once (queue == 0) or after
the queued ones (up to 5). loop != 0 repeats it (Actor_AnimLoopCallback), otherwise the
queue callback runs when it ends. blend != 0 blends it in. With sync != 0, a player
holding an object also restarts animation 3 on the hold node. A missing animation just
sets the done flags. */
void Actor_PlayAnim(void *actor, int index, int loop, int queue, float speed, float blend, int sync)
{
	CHLAnimCallback callback;
	void *inst;
	void *ai;
	void *anim = CHL_PTR(actor, ACT_ANIMS + index * 4);
	float rate;

	if (anim == NULL) {
		CHL_U16(actor, ACT_ANIM_EVENT) = 1;
		CHL_U16(actor, ACT_ANIM_DONE) = 1;
		return;
	}
	CHL_U16(actor, ACT_ANIM_EVENT) = 0;
	CHL_U16(actor, ACT_ANIM_DONE) = 0;
	rate = speed * CHL_F32(CHL_PTR(actor, ACT_ANIM_INFO), 0x40 + index * 0x44);
	if (queue == 0) {
		chl_FlushAnimations(ACT_INST(actor), CHL_PTR(actor, ACT_NODE), 1, 0);
		CHL_U16(actor, ACT_ANIM_QUEUED) = 0;
		CHL_F32(actor, ACT_ANIM_RATE) = rate;
	} else if (CHL_U16(actor, ACT_ANIM_QUEUED) < 5) {
		CHL_F32(actor, ACT_ANIM_QUEUE + CHL_U16(actor, ACT_ANIM_QUEUED) * 4) = rate;
		CHL_U16(actor, ACT_ANIM_QUEUED) = (unsigned short)(CHL_U16(actor, ACT_ANIM_QUEUED) + 1);
	} else {
		chl_Printf("***WARNING*** animation queue size exceeded!!!\n");
	}
	callback = loop == 0 ? (CHLAnimCallback)Actor_AnimQueueCallback : (CHLAnimCallback)Actor_AnimLoopCallback;
	inst = ACT_INST(actor);
	if (blend == 0.0f) {
		chl_QueueAnimation(inst, CHL_PTR(actor, ACT_NODE), CHL_PTR(actor, ACT_ANIMS + index * 4), 0x400, -1, 0,
			callback, actor);
		if (sync != 0 && CHL_I32(actor, ACT_TYPE) == 1 &&
		    (ai = CHL_ARR(void *, 0x002f289c)[CHL_U16(actor, ACT_PLAYER)]) != NULL &&
		    CHL_PTR(ai, 0x10c0) != NULL) {
			chl_FlushAnimations(inst, CHL_PTR(ai, 0x10cc), 1, 0);
			if (CHL_PTR(actor, ACT_ANIMS + 3 * 4) != NULL)
				chl_QueueAnimation(inst,
					CHL_PTR(CHL_ARR(void *, 0x002f289c)[CHL_U16(actor, ACT_PLAYER)], 0x10cc),
					CHL_PTR(actor, ACT_ANIMS + 3 * 4), 0x400, -1, 0,
					(CHLAnimCallback)Actor_AnimLoopCallback, actor);
		}
	} else {
		chl_QueueAnimationBlendIn(inst, CHL_PTR(actor, ACT_NODE), CHL_PTR(actor, ACT_ANIMS + index * 4), 0x400,
			-1, blend, 0, callback, actor);
		if (sync != 0 && CHL_I32(actor, ACT_TYPE) == 1 &&
		    (ai = CHL_ARR(void *, 0x002f289c)[CHL_U16(actor, ACT_PLAYER)]) != NULL &&
		    CHL_PTR(ai, 0x10c0) != NULL) {
			chl_FlushAnimations(inst, CHL_PTR(ai, 0x10cc), 1, 0);
			if (CHL_PTR(actor, ACT_ANIMS + 3 * 4) != NULL)
				chl_QueueAnimationBlendIn(inst,
					CHL_PTR(CHL_ARR(void *, 0x002f289c)[CHL_U16(actor, ACT_PLAYER)], 0x10cc),
					CHL_PTR(actor, ACT_ANIMS + 3 * 4), 0x400, -1, blend, 0,
					(CHLAnimCallback)Actor_AnimLoopCallback, actor);
		}
	}
}

/* 0x000af8f0  CharLib/Actor.cpp (bracketed)
Texture stage 1 states that modulate by a constant alpha (the texture factor). */
void Actor_SetConstantAlphaStage(int alpha)
{
	D3DDevice_SetRenderState_TextureFactor((DWORD)alpha << 24);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)10, 1);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)11, 1);
	D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
	D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
}

/* 0x000af970  CharLib/Actor.cpp (bracketed)
Sets the wanted heading towards a point (atan2(dx, dz)); nothing when the point is at the
actor's x/z. */
void Actor_FacePoint(void *actor, float *point)
{
	float dx = point[0] - CHL_F32(actor, ACT_POS + 0x0);
	double dz = (double)point[2] - (double)CHL_F32(actor, ACT_POS + 0x8);
	double len = sqrt(dz * dz + (double)dx * (double)dx);
	double inv;

	if (len == 0.0)
		return;
	inv = 1.0 / len;
	CHL_F32(actor, ACT_HEADING_WANTED) = (float)CHL_FPATAN((double)dx * inv, inv * dz);
}

/* 0x000af9e0  CharLib/Actor.cpp (bracketed) */
void Actor_UpdateAll(void)
{
	void *actor = chl_ListNext(chl_actor_list, NULL);

	while (actor != NULL)
		actor = chl_ListNext(chl_actor_list, Actor_Update(actor));
}

/* 0x000afa10  CharLib/Actor.cpp
Loads an actor by model name from a package: the model, then (types 0, 2 and 0xe, unless
no_collision) its "_coll.obe" collision mesh, which becomes the collision data (type 2
keeps the collision instance), and its "_shad.obe" shadow mesh. no_collision sets flag
0x20. pos[3] is set to 1. */
void *Actor_Load(const char *name, void *package, float *pos, int type, void *mem, int no_collision)
{
	char mesh[64];
	void *model = chl_FindObject(package, name, 1, 0);
	void *actor;
	void *found;
	void *inst;

	pos[3] = 1.0f;
	if (model == NULL) {
		chl_Printf("*** FATAL ERROR *** Could not load actor %s\n", name);
		return NULL;
	}
	actor = Actor_Create(model, pos, mem, type, 0, 1);
	CHL_PTR(actor, ACT_COLL_INST) = NULL;
	CHL_PTR(actor, ACT_SHADOW_INST) = NULL;
	CHL_I32(actor, ACT_ALPHA) = 0x100;
	if (no_collision == 0) {
		if (type == 0 || type == 2 || type == 0xe) {
			Actor_CollMeshName(mesh, name);
			found = chl_FindObject(package, mesh, 1, 0);
			if (found == NULL) {
				chl_Printf(" couldn't find collision mesh '%s'\n", mesh);
				chl_PrepInstance(ACT_INST(actor), 1);
				CRColl_BuildMesh(actor);
			} else {
				chl_Printf(" FOUND collision mesh '%s'\n", mesh);
				inst = chl_InitInstance(NULL, found, 5, "C:\\BlitzParty\\Game\\Src\\CharLib\\Actor.cpp", 0x23b);
				CHL_PTR(actor, ACT_COLL_INST) = inst;
				chl_SetNodeTime(inst, CHL_PTR(inst, 0x134), 0, 1);
				chl_FlushAnimations(CHL_PTR(actor, ACT_COLL_INST),
					CHL_PTR(CHL_PTR(actor, ACT_COLL_INST), 0x134), 1, 0);
				inst = CHL_PTR(actor, ACT_COLL_INST);
				memcpy(CHL_FIELD(char, inst, 0xc0), CHL_FIELD(char, actor, ACT_POS), 0x10);
				chl_PrepInstance(CHL_PTR(actor, ACT_COLL_INST), 1);
				CRColl_BuildMeshFrom(actor, CHL_PTR(actor, ACT_COLL_INST));
				if (type == 0 || type == 0xe) {
					chl_DeleteInstance(CHL_PTR(actor, ACT_COLL_INST));
					CHL_PTR(actor, ACT_COLL_INST) = NULL;
				}
			}
		}
	} else {
		CHL_U32(actor, ACT_FLAGS) = CHL_U32(actor, ACT_FLAGS) | 0x20;
	}
	if (type == 0 || type == 2 || type == 0xe) {
		Actor_ShadowMeshName(mesh, name);
		found = chl_FindObject(package, mesh, 1, 0);
		if (found != NULL) {
			inst = chl_InitInstance(NULL, found, 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Actor.cpp", 0x25b);
			CHL_PTR(actor, ACT_SHADOW_INST) = inst;
			chl_SetNodeTime(inst, CHL_PTR(inst, 0x134), 0, 1);
			chl_FlushAnimations(CHL_PTR(actor, ACT_SHADOW_INST),
				CHL_PTR(CHL_PTR(actor, ACT_SHADOW_INST), 0x134), 1, 0);
			inst = CHL_PTR(actor, ACT_SHADOW_INST);
			memcpy(CHL_FIELD(char, inst, 0xc0), CHL_FIELD(char, actor, ACT_POS), 0x10);
			chl_PrepInstance(CHL_PTR(actor, ACT_SHADOW_INST), 1);
			CHL_U32(actor, ACT_FLAGS) = CHL_U32(actor, ACT_FLAGS) | 0x400000;
			return actor;
		}
		CHL_PTR(actor, ACT_SHADOW_INST) = NULL;
	}
	return actor;
}
