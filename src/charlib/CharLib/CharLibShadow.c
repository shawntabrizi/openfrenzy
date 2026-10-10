/*
CharLib/CharLibShadow.c  (charlib)
Original: C:\BlitzParty\Game\Src\CharLib\CharLibShadow.cpp (tag at 0x000be6d0, source lines
68-73).

Projected shadows of the four players: one shadow projector (engine_misc bProjector /
bCamera) per player, aimed from a light position or a fixed direction, and the draw passes
that render the shadow meshes with them. The shadow-map render functions themselves
(0x000be4a0-0x000be610) lie in front of the tag in the CarryOn block and are called here
by address.

0x000befa0-0x000befd0 follow in the same block and work on the collect effect of the
pick-ups (classlib_fx CCollectEffect: free and draw); placed here by address (inferred;
they may belong to ClassLib/CollectableEffects.cpp, which starts right after them).
*/

#include "charlib.h"
#include "charlib_actor.h"
#include "charlib_ai.h"

/* ---------- globals (charlib data2 / bss2) */

/* 0x00293e80  shadows on */
#define shadow_enabled CHL_VAR(int, 0x00293e80)
/* 0x00293e84  default projector distance (1200) */
#define shadow_default_distance CHL_VAR(float, 0x00293e84)
/* 0x00293e88  default pitch (1) */
#define shadow_default_pitch CHL_VAR(float, 0x00293e88)
/* 0x00293e8c..98  projector field of view x, y, near and far (0.3, 0.4, 5, 5000) */
#define shadow_fov_x CHL_VAR(float, 0x00293e8c)
#define shadow_fov_y CHL_VAR(float, 0x00293e90)
#define shadow_near CHL_VAR(float, 0x00293e94)
#define shadow_far CHL_VAR(float, 0x00293e98)
/* 0x00293e9c  256 x 256 shadow maps (else 128 x 128) */
#define shadow_hires CHL_VAR(int, 0x00293e9c)
/* 0x00293ea0  the projectors of the four players */
#define shadow_projectors CHL_ARR(void *, 0x00293ea0)
/* 0x00293eb0  the light was set by Shadows_SetLight before Shadows_Init */
#define shadow_light_set CHL_VAR(int, 0x00293eb0)
/* 0x00293eb4  keep the pitch limits at the next Shadows_Init */
#define shadow_keep_limits CHL_VAR(int, 0x00293eb4)
/* 0x00293eb8  projector of each player in use */
#define shadow_in_use CHL_ARR(int, 0x00293eb8)
/* 0x00293ec8  light position (modes 0 and 1) */
#define shadow_light_pos CHL_ARR(float, 0x00293ec8)
/* 0x00293ed8 / dc  fixed pitch and heading (mode 2); 0x00293ee0 / e4 */
#define shadow_fixed_pitch CHL_VAR(float, 0x00293ed8)
#define shadow_fixed_heading CHL_VAR(float, 0x00293edc)
/* 0x00293ee8  projector offset from the player (0, 100, 0, 1) */
#define shadow_offset CHL_ARR(float, 0x00293ee8)
/* 0x00293ef8  aim mode: 0 away from the light position, 1 towards it, 2 fixed angles */
#define shadow_mode CHL_VAR(int, 0x00293ef8)
/* 0x00293efc / 0x00293f00  pitch limits (1, 1.5) */
#define shadow_min_pitch CHL_VAR(float, 0x00293efc)
#define shadow_max_pitch CHL_VAR(float, 0x00293f00)
/* 0x00293f04  projector distance */
#define shadow_distance CHL_VAR(float, 0x00293f04)
/* 0x00293f08  projectors */
#define shadow_count CHL_VAR(int, 0x00293f08)
/* 0x00293f0c */
#define shadow_unused_f0c CHL_VAR(int, 0x00293f0c)
/* 0x00293f10  first draw since Shadows_Init (the maps are rendered twice) */
#define shadow_first CHL_VAR(int, 0x00293f10)
/* 0x002f6c10 / 0x002f6c18  saved texture transforms (count, matrices) */
#define shadow_saved_count CHL_VAR(int, 0x002f6c10)
#define shadow_saved_transforms CHL_ARR(D3DMATRIX, 0x002f6c18)
/* 0x002f6d1c  a slowly growing value passed as the yaw of the first setup */
#define shadow_spin CHL_VAR(float, 0x002f6d1c)

/* 0x002be588 / 0x002be58c  the display's field of view x and y (bdisplay) */
extern float DAT_002be588;
extern float DAT_002be58c;

/* engine_misc projector / camera (engine_misc.decls) */
#define sh_ProjectorConstruct ((void *(*)(void *, int, int, int, unsigned int))FUN_001ebdb0)
#define sh_ProjectorSetColour ((void (*)(void *, int))FUN_001ec160)
#define sh_CameraAttach ((void (*)(void *, void *, float *, int))FUN_001eb7c0)
#define sh_CameraSetAngles ((void (*)(void *, float, float, float, float))FUN_001eb890)
#define sh_CameraSetFov ((void (*)(void *, float, float, float, float))FUN_001eba70)
#define sh_CameraUpdate ((void (*)(void *))FUN_001eb900)
/* bdisplay: bdSetFieldOfView(x, y), bdSetDepthOffset(offset) */
#define sh_SetFieldOfView ((void (*)(float, float))FUN_00044a00)
#define sh_SetDepthOffset ((void (*)(float))FUN_00044550)
/* the shadow-map passes in the CarryOn block (carryon_a by the module table):
0x000be4a0 render the maps, 0x000be610 render them for one object, 0x000be550 set up the
receivers (returns how many), 0x000be5a0 restore */
#define sh_RenderMaps ((void (*)(void))FUN_000be4a0)
#define sh_RenderMapsFor ((void (*)(void *))FUN_000be610)
#define sh_BeginReceive ((int (*)(void))FUN_000be550)
#define sh_EndReceive ((void (*)(void))FUN_000be5a0)

/* the light parameters of Shadows_Init and Shadows_SetLight */
static void shadow_set_light(int mode, float a, float b, float c)
{
	shadow_mode = mode;
	if (mode == 0 || mode == 1) {
		shadow_light_pos[0] = a;
		shadow_light_pos[1] = b;
		shadow_light_pos[2] = c;
		shadow_light_pos[3] = 1.0f;
	} else if (mode == 2) {
		shadow_fixed_pitch = a;
		shadow_fixed_heading = b;
		CHL_F32(CHL_ADDR(0x00293ee0), 0) = 0.0f;
		CHL_F32(CHL_ADDR(0x00293ee4), 0) = 1.0f;
	}
}

/* 0x000be6d0  CharLib/CharLibShadow.cpp
Creates the four shadow projectors (once) and attaches them to the players; sets the
light unless Shadows_SetLight set it before. */
void Shadows_Init(int mode, float a, float b, float c)
{
	const char *message;
	void *mem;
	void *projector;
	int keep;
	int i;

	shadow_unused_f0c = 0;
	shadow_distance = shadow_default_distance;
	shadow_first = 1;
	for (i = 0; i < 4; i++) {
		if (shadow_projectors[i] == NULL) {
			if (shadow_hires == 0) {
				mem = chl_Alloc(0x160, "C:\\BlitzParty\\Game\\Src\\CharLib\\CharLibShadow.cpp", 0x49, 0x1001);
				projector = mem == NULL ? NULL : sh_ProjectorConstruct(mem, 1, 0x80, 0x80, 0);
				message = "128x128 SHADOWS\n";
			} else {
				mem = chl_Alloc(0x160, "C:\\BlitzParty\\Game\\Src\\CharLib\\CharLibShadow.cpp", 0x44, 0x1001);
				projector = mem == NULL ? NULL : sh_ProjectorConstruct(mem, 1, 0x100, 0x100, 0);
				message = "256x256 SHADOWS\n";
			}
			shadow_projectors[i] = projector;
			chl_Printf(message);
			sh_ProjectorSetColour(shadow_projectors[i], 0x80);
		}
		sh_CameraAttach(shadow_projectors[i], ACT_INST(CHL_PTR(chl_ai_players[i], AI_ACTOR)), shadow_offset, 1);
		sh_CameraSetAngles(shadow_projectors[i], shadow_default_pitch, shadow_spin, 0.0f, shadow_distance);
		sh_CameraSetFov(shadow_projectors[i], shadow_fov_x, shadow_fov_y, shadow_near, shadow_far);
		sh_CameraUpdate(shadow_projectors[i]);
		shadow_in_use[i] = 1;
	}
	shadow_count = 4;
	if (shadow_light_set == 0)
		shadow_set_light(mode, a, b, c);
	keep = shadow_keep_limits;
	shadow_keep_limits = 0;
	shadow_light_set = 0;
	if (keep == 0) {
		shadow_min_pitch = 1.0f;
		shadow_max_pitch = 1.5f;
	}
}

/* 0x000be900  CharLib (CharLibShadow.cpp, inferred)
Sets the shadow light (kept by the next Shadows_Init). */
void Shadows_SetLight(int mode, float a, float b, float c)
{
	shadow_mode = mode;
	shadow_light_set = 1;
	if (mode == 0 || mode == 1) {
		shadow_light_pos[0] = a;
		shadow_light_pos[1] = b;
		shadow_light_pos[2] = c;
		shadow_light_pos[3] = 1.0f;
	} else if (mode == 2) {
		shadow_fixed_pitch = a;
		shadow_fixed_heading = b;
		CHL_F32(CHL_ADDR(0x00293ee0), 0) = 0.0f;
		CHL_F32(CHL_ADDR(0x00293ee4), 0) = 1.0f;
	}
}

/* 0x000be970  CharLib (CharLibShadow.cpp, inferred) */
void Shadows_SetLightPos(float *pos)
{
	memcpy(shadow_light_pos, pos, 0x10);
}

/* the pitch from the light direction's y (asin), within the limits */
static float shadow_pitch(float y)
{
	float pitch = (float)asin((double)y);

	if (pitch < shadow_min_pitch)
		pitch = shadow_min_pitch;
	if (shadow_max_pitch < pitch)
		pitch = shadow_max_pitch;
	return pitch;
}

/* 0x000be9a0  CharLib (CharLibShadow.cpp, inferred)
Aims every player's projector: away from (mode 0) or towards (mode 1) the light position,
or at the fixed angles (mode 2). */
void Shadows_Aim(void)
{
	float d[4];
	float pitch;
	float heading;
	void *actor;
	int i;

	shadow_spin = shadow_spin + 0.01f;
	for (i = 0; i < 4; i++) {
		if (shadow_mode == 0) {
			actor = CHL_PTR(chl_ai_players[i], AI_ACTOR);
			d[0] = shadow_light_pos[0] - CHL_F32(actor, ACT_POS + 0x0);
			d[1] = shadow_light_pos[1] - CHL_F32(actor, ACT_POS + 0x4);
			d[2] = shadow_light_pos[2] - CHL_F32(actor, ACT_POS + 0x8);
			FUN_000514a0(d, d);
			heading = (float)CHL_FPATAN(-d[0], -d[2]);
			pitch = shadow_pitch(d[1]);
		} else {
			pitch = shadow_fixed_pitch;
			heading = shadow_fixed_heading;
			if (shadow_mode == 1) {
				actor = CHL_PTR(chl_ai_players[i], AI_ACTOR);
				d[0] = shadow_light_pos[0] - CHL_F32(actor, ACT_POS + 0x0);
				d[1] = shadow_light_pos[1] - CHL_F32(actor, ACT_POS + 0x4);
				d[2] = shadow_light_pos[2] - CHL_F32(actor, ACT_POS + 0x8);
				FUN_000514a0(d, d);
				heading = (float)CHL_FPATAN(d[0], d[2]);
				pitch = shadow_pitch(d[1]);
			}
		}
		sh_CameraSetAngles(shadow_projectors[i], pitch, heading, 0.0f, shadow_distance);
		sh_CameraUpdate(shadow_projectors[i]);
	}
}

/* 0x000beb70  CharLib (CharLibShadow.cpp, inferred)
Moves an actor's shadow-mesh instance to the actor. */
void Actor_UpdateShadowMesh(void *actor)
{
	void *inst = CHL_PTR(actor, ACT_SHADOW_INST);

	if (inst == NULL)
		return;
	memcpy(CHL_FIELD(char, inst, 0xc0), CHL_FIELD(char, actor, ACT_POS), 0x10);
	inst = CHL_PTR(actor, ACT_SHADOW_INST);
	memcpy(CHL_FIELD(char, inst, 0xe0), CHL_FIELD(char, actor, ACT_ORIENT), 0x10);
	chl_PrepInstance(CHL_PTR(actor, ACT_SHADOW_INST), 1);
}

/* 0x000bebf0  CharLib (CharLibShadow.cpp, inferred)
Saves the four texture transforms. */
void Shadows_SaveTransforms(void)
{
	int i;

	shadow_saved_count = 4;
	for (i = 0; i < shadow_saved_count; i++)
		D3DDevice_GetTransform((D3DTRANSFORMSTATETYPE)(i + 2), &shadow_saved_transforms[i]);
}

/* 0x000bec20  CharLib (CharLibShadow.cpp, inferred) */
void Shadows_RestoreTransforms(void)
{
	int i;

	for (i = 0; i < shadow_saved_count; i++)
		D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)(i + 2), &shadow_saved_transforms[i]);
}

/* the receiving pass of Shadows_DrawFor / Shadows_Draw */
static void shadow_receive(float fov_x, float fov_y)
{
	sh_SetFieldOfView(fov_x, fov_y);
	if (0 < sh_BeginReceive()) {
		chl_SetRenderState(4, 1, 0);
		chl_PushRenderState(5, 0, 0);
		sh_SetDepthOffset(-0.05f);
		Actor_DrawShadowMeshes();
		sh_SetDepthOffset(0.0f);
		chl_PopRenderState(5);
		sh_EndReceive();
	}
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	chl_PopRenderState(3);
	chl_PopRenderState(1);
	Shadows_RestoreTransforms();
}

/* 0x000bec50  CharLib (CharLibShadow.cpp, inferred)
Renders the shadow maps for one object (twice on the first frame) and draws the shadow
meshes that receive them. */
void Shadows_DrawFor(void *object)
{
	float fov_x;
	float fov_y;

	if (object == NULL || shadow_enabled == 0 || shadow_count == 0)
		return;
	Shadows_SaveTransforms();
	fov_y = DAT_002be58c;
	fov_x = DAT_002be588;
	chl_PushRenderState(3, 0, 0);
	chl_PushRenderState(1, 2, 0);
	if (shadow_first != 0) {
		sh_RenderMapsFor(object);
		shadow_first = 0;
	}
	sh_RenderMapsFor(object);
	shadow_receive(fov_x, fov_y);
}

/* 0x000bed50  CharLib (CharLibShadow.cpp, inferred)
The same for every object. */
void Shadows_Draw(void)
{
	float fov_x;
	float fov_y;

	if (shadow_enabled == 0 || shadow_count == 0)
		return;
	Shadows_SaveTransforms();
	fov_y = DAT_002be58c;
	fov_x = DAT_002be588;
	chl_PushRenderState(3, 0, 0);
	chl_PushRenderState(1, 2, 0);
	sh_RenderMaps();
	if (shadow_first != 0) {
		sh_RenderMaps();
		shadow_first = 0;
	}
	shadow_receive(fov_x, fov_y);
}

/* 0x000bee40  CharLib (CharLibShadow.cpp, inferred)
Renders the shadow maps and sets up the receivers for the caller's own drawing (finish
with Shadows_EndReceive). Returns the receivers set up. */
int Shadows_BeginReceive(void)
{
	float fov_x;
	float fov_y;
	int count;

	if (shadow_enabled == 0 || shadow_count == 0)
		return 0;
	Shadows_SaveTransforms();
	chl_SetRenderState(1, 2, 0);
	fov_x = DAT_002be588;
	fov_y = DAT_002be58c;
	if (shadow_first != 0) {
		sh_RenderMaps();
		shadow_first = 0;
	}
	sh_RenderMaps();
	sh_SetFieldOfView(fov_x, fov_y);
	count = sh_BeginReceive();
	sh_SetDepthOffset(-0.05f);
	return count;
}

/* 0x000beed0  CharLib (CharLibShadow.cpp, inferred) */
void Shadows_EndReceive(void)
{
	if (shadow_enabled == 0 || shadow_count == 0)
		return;
	sh_SetDepthOffset(0.0f);
	sh_EndReceive();
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	Shadows_RestoreTransforms();
}

/* 0x000bef00  CharLib (CharLibShadow.cpp, inferred)
Shadows_BeginReceive for one object (NULL: for all, returning 0). */
int Shadows_BeginReceiveFor(void *object)
{
	float fov_x;
	float fov_y;
	int count;

	if (object == NULL) {
		Shadows_BeginReceive();
		return 0;
	}
	if (shadow_enabled == 0 || shadow_count == 0)
		return 0;
	Shadows_SaveTransforms();
	chl_SetRenderState(1, 2, 0);
	fov_x = DAT_002be588;
	fov_y = DAT_002be58c;
	if (shadow_first != 0) {
		sh_RenderMapsFor(object);
		shadow_first = 0;
	}
	sh_RenderMapsFor(object);
	sh_SetFieldOfView(fov_x, fov_y);
	count = sh_BeginReceive();
	sh_SetDepthOffset(-0.05f);
	return count;
}

/* ---------- the collect effect (classlib_fx CCollectEffect, 0x98 bytes) */

/* 0x000befa0  CharLib (file unknown; next to ClassLib/CollectableEffects.cpp)
The constructor: no rays yet. */
void *CCollectEffect_Ctor(void *effect)
{
	CHL_PTR(effect, 0x00) = NULL;
	return effect;
}

/* 0x000befb0  CharLib (file unknown; next to ClassLib/CollectableEffects.cpp)
Frees the rays. */
void CCollectEffect_Free(void *effect)
{
	if (CHL_PTR(effect, 0x00) != NULL) {
		chl_Free(CHL_PTR(effect, 0x00));
		CHL_PTR(effect, 0x00) = NULL;
	}
}

/* 0x000befd0  CharLib (file unknown; next to ClassLib/CollectableEffects.cpp)
Draws the rays (0x54 bytes each: start at 0x00, end at 0x10, colour at 0x40) as beams of
width 0x2c, fading to colour 0x14, with texture 0x60. */
void CCollectEffect_Draw(void *effect)
{
	unsigned char *ray;
	int i;

	if (CHL_I32(effect, 0x28) == 0)
		return;
	chl_PushRenderState(6, 1, 0);
	chl_PushRenderState(10, 1, 0);
	chl_PushRenderState(7, 3, 0);
	chl_PushRenderState(5, 0, 0);
	for (i = 0; i < CHL_I32(effect, 0x28); i++) {
		ray = (unsigned char *)CHL_PTR(effect, 0x00) + i * 0x54;
		((int (*)(float *, float, float, int *, float *, float, float, int *, void *, float *, float))FUN_000407d0)(
			(float *)ray, CHL_F32(effect, 0x2c), CHL_F32(effect, 0x2c), (int *)(ray + 0x40), (float *)(ray + 0x10),
			1.0f, 1.0f, CHL_FIELD(int, effect, 0x14), CHL_PTR(effect, 0x60), NULL, 0.0f);
	}
	chl_PopRenderState(6);
	chl_PopRenderState(10);
	chl_PopRenderState(7);
	chl_PopRenderState(5);
}
