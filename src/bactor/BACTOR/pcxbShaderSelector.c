/*
BACTOR/pcxbShaderSelector.c  <-  C:\Babel\Xbox\Src\BACTOR\pcxbShaderSelector.cpp

The vertex shader selector: an actor pass is described by a key (skin
weights, pass, effect flags, effect parameter) under a lighting state
(numbers of lights of each type). For a new key a vertex shader is put
together from code fragments (C++ class CBShaderFragment and its
subclasses), assembled and cached (xbShaderSelector.c). Each fragment
loads its own vertex shader constants.

0x0003b700 is tagged with pcxbShaderSelector.cpp; the fragment methods
(0x0003a280-0x0003b5d0, in the fragments' vtables) and the selector
functions around it are placed here (file inferred).
*/

#include <stdio.h>
#include <string.h>

#include "bactor.h"
#include "bactor_extern.h"

/* the context the fragment methods get */
struct BShaderContext
{
	struct BActorInstance *inst; /* 0x00 */
	int pass; /* 0x04 */
	int reg; /* 0x08 first constant register of the fragment */
};

/* CBShaderFragment's virtual functions (thiscall in the original) */
typedef void (*BFragmentMethod)(struct BShaderFragment *self, struct BShaderContext *ctx);
struct BShaderFragmentVtbl
{
	BFragmentMethod setConstants; /* [0] */
	BFragmentMethod begin; /* [1] */
	BFragmentMethod end; /* [2] */
	int (*numConstants)(struct BShaderFragment *self); /* [3] */
};
#define FRAGMENT_VTBL(f) ((const struct BShaderFragmentVtbl *)(f)->vtable)

/* the effect parameter of a pass (BActorInstance.passParam) */
#define PASS_PARAM(ctx) ((int *)(intptr_t)(ctx)->inst->passParam[(ctx)->pass])

/* 0x002bb248-0x002bb29c: the fragments */
enum
{
	FRAG_OP2, /* 0x002bb248 "%0 %1, %2" */
	FRAG_OP3, /* 0x002bb24c "%0 %1, %2, %3" */
	FRAG_OP4, /* 0x002bb250 "%0 %1, %2, %3, %4" */
	FRAG_MOVA0, /* 0x002bb254 */
	FRAG_MULMAD, /* 0x002bb258 */
	FRAG_SKINPOS, /* 0x002bb25c */
	FRAG_SKINNORMAL, /* 0x002bb260 */
	FRAG_CEL, /* 0x002bb264 */
	FRAG_FLAT, /* 0x002bb268 */
	FRAG_FLATALPHA, /* 0x002bb26c */
	FRAG_BLACK, /* 0x002bb270 */
	FRAG_ENVMAP, /* 0x002bb274 */
	FRAG_AMBIENT, /* 0x002bb278 */
	FRAG_DIRLIGHT, /* 0x002bb27c */
	FRAG_POINTLIGHT, /* 0x002bb280 */
	FRAG_SPOTLIGHT, /* 0x002bb284 */
	FRAG_FOG, /* 0x002bb288 */
	FRAG_POSITION, /* 0x002bb28c */
	FRAG_DIFFUSE, /* 0x002bb290 */
	FRAG_TEXCOORD, /* 0x002bb294 */
	FRAG_COLOUR, /* 0x002bb298 */
	FRAG_NORMALIZE, /* 0x002bb29c */
	FRAG_COUNT
};
struct BShaderFragment *bactor_fragments[FRAG_COUNT];

/* 0x002bb2a0, 0x002bb2a8, 0x002bb2ac: lighting states (cache size and use),
modes per state */
int bactor_maxLightingStates;
int bactor_numLightingStates;
int bactor_maxShaderModes;
/* 0x002bb2a4: the shader mode in use */
struct BShaderMode *bactor_currentMode;
/* 0x002bc2b4: the lighting state cache; 0x002bc2b8: the current state */
struct BLightingState *bactor_lightingStates;
struct BLightingState *bactor_currentLighting;
/* 0x002bb2b0: the text of the shader being generated */
char bactor_shaderText[0x1000];

/* 0x002bb240, 0x002bb244, 0x002bc2b0: the next spot, directional and point
light the light fragments load */
static int s_spotIndex;
static int s_dirIndex;
static int s_pointIndex;
/* 0x002bc3e8: one skin weight at most */
int DAT_002bc3e8;

/* 0x001ec260 (engine_misc's empty "ret 4" function, shared by identical-code
folding): the fragments' empty methods */
static void bFragmentNop(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	(void)ctx;
}

/* the i-th light (from 0) of a type among the enabled lights; the list head
when there is none */
static struct BLight *bFindEnabledLight(int type, int index)
{
	struct BLight *light = g_lightsOn.next;
	int found = 0;

	while (light != &g_lightsOn)
	{
		if (light->type == type && ++found > index)
			break;
		light = light->next;
	}
	return light;
}

/* 0x0003a280  BACTOR/pcxbShaderSelector.cpp (file inferred)
cel shading: c0 = light direction in model space, w = texture coordinate */
void bFragCelSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	int *param = PASS_PARAM(ctx); /* texture, offset, light */
	struct BLight *light = (struct BLight *)(intptr_t)param[2];
	float c[4];
	float scale;

	(void)self;
	c[0] = light->dir[0];
	c[1] = light->dir[1];
	c[2] = light->dir[2];
	c[3] = light->dir[3];
	ba_MatTransformInPlace(ctx->inst->inverse, c);
	if (param[0] == 0)
	{
		c[3] = 0.0f;
	}
	else
	{
		scale = 1.0f / (float)*(uint16_t *)((char *)(intptr_t)param[0] + 0x30);
		c[3] = (float)param[1] * scale + scale * 0.5f;
	}
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
}

/* 0x0003a320  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragFlatBegin(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)ctx;
	self->extra = (int)DAT_002be620;
	ba_bdSetState(4, 2, 0);
}

/* 0x0003a340  BACTOR/pcxbShaderSelector.cpp (file inferred)
flat colour: c0 = colour, c1.w = offset along the normal */
void bFragFlatSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	int *param = PASS_PARAM(ctx); /* offset (float), r, g, b, a */
	float c[4];

	(void)self;
	c[0] = (float)param[1] * (1.0f / 255.0f);
	c[1] = (float)param[2] * (1.0f / 255.0f);
	c[2] = (float)param[3] * (1.0f / 255.0f);
	c[3] = (float)DAT_00284b0c * (1.0f / 255.0f) * (float)param[4] * (1.0f / 255.0f);
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
	c[0] = 0.0f;
	c[1] = 0.0f;
	c[2] = 0.0f;
	c[3] = *(float *)&param[0];
	D3DDevice_SetVertexShaderConstant(ctx->reg + 1, c, 1);
}

/* 0x0003a3e0  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragFlatEnd(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)ctx;
	ba_bdSetState(4, (uint32_t)self->extra, 0);
}

/* 0x0003a400  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragFlatAlphaBegin(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	(void)ctx;
	ba_bdPushState(6, 1, 0);
	ba_bdLockState(6);
}

/* 0x0003a420  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragFlatAlphaEnd(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	(void)ctx;
	ba_bdUnlockState(6);
	ba_bdPopState(6);
}

/* 0x0003a440  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragColourBegin(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	ba_bdPushState(5, 0, 0);
	ba_bdPushState(7, 1, 0);
	ba_bdPushState(4, 1, 0);
	ba_bdPushState(6, 0, 0);
	ba_bdClearTexture(ctx->pass); /* the stage is the pass number, as in the original */
	DAT_002bee34[ctx->pass] = 0;
}

/* 0x0003a490  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragColourEnd(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	ba_bdPopState(5);
	ba_bdPopState(7);
	ba_bdPopState(4);
	ba_bdPopState(6);
	DAT_002bee34[ctx->pass] = 1;
}

/* 0x0003a4d0  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragColourSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	int *param = PASS_PARAM(ctx); /* r, g, b */
	float c[4];

	(void)self;
	c[0] = (float)param[0] * (1.0f / 255.0f);
	c[1] = (float)param[1] * (1.0f / 255.0f);
	c[2] = (float)param[2] * (1.0f / 255.0f);
	c[3] = 1.0f;
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
}

/* 0x0003a530  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragBlackSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	static const uint32_t c[4] = { 0, 0, 0, 0x3b808081 }; /* w = 1 / 255 */

	(void)self;
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
}

/* 0x0003a570  BACTOR/pcxbShaderSelector.cpp (file inferred)
environment map: the world-view matrix and its inverse, transposed */
void bFragEnvMapSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	float m[16];

	(void)self;
	ba_MatTranspose(m, DAT_002be430);
	D3DDevice_SetVertexShaderConstant(ctx->reg, m, 3);
	ba_MatInverse(m, DAT_002be430);
	ba_MatTranspose(m, m); /* in place, as in the original */
	D3DDevice_SetVertexShaderConstant(ctx->reg + 3, m, 3);
}

/* 0x0003a5e0  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragAmbientSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	float c[4];

	(void)self;
	c[0] = (float)DAT_00284b10 * (1.0f / 255.0f);
	c[3] = 1.0f;
	c[1] = (float)DAT_00284b14 * (1.0f / 255.0f);
	c[2] = (float)DAT_00284b18 * (1.0f / 255.0f);
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
}

/* 0x0003a640  BACTOR/pcxbShaderSelector.cpp (file inferred)
point light: position in model space, colour, 1 / range */
void bFragPointLightSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	struct BLight *light = bFindEnabledLight(BLIGHT_POINT, s_pointIndex);
	float c[4];

	(void)self;
	ba_MatTransform(c, ctx->inst->inverse, light->pos);
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
	c[0] = (float)light->color[0] * (1.0f / 255.0f);
	c[1] = (float)light->color[1] * (1.0f / 255.0f);
	c[3] = 1.0f;
	c[2] = (float)light->color[2] * (1.0f / 255.0f);
	D3DDevice_SetVertexShaderConstant(ctx->reg + 1, c, 1);
	c[0] = 1.0f / light->dir[0];
	c[1] = 0.0f;
	c[2] = 0.0f;
	c[3] = 1.0f;
	D3DDevice_SetVertexShaderConstant(ctx->reg + 2, c, 1);
	s_pointIndex++;
}

/* 0x0003a730  BACTOR/pcxbShaderSelector.cpp (file inferred)
spot light: position and direction in model space, colour, cone */
void bFragSpotLightSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	struct BLight *light = bFindEnabledLight(BLIGHT_SPOT, s_spotIndex);
	float c[4];

	(void)self;
	ba_MatTransform(c, ctx->inst->inverse, light->pos);
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
	ba_MatTransform(c, ctx->inst->inverse, light->dirNorm);
	ba_Vec3Normalize(c, c);
	D3DDevice_SetVertexShaderConstant(ctx->reg + 1, c, 1);
	c[0] = (float)light->color[0] * (1.0f / 255.0f);
	c[1] = (float)light->color[1] * (1.0f / 255.0f);
	c[3] = 1.0f;
	c[2] = (float)light->color[2] * (1.0f / 255.0f);
	D3DDevice_SetVertexShaderConstant(ctx->reg + 2, c, 1);
	c[0] = light->cosHalfAngle;
	c[1] = 1.0f / light->range;
	c[3] = 0.0f;
	c[2] = 1.0f / (1.0f - light->cosHalfAngle);
	D3DDevice_SetVertexShaderConstant(ctx->reg + 3, c, 1);
	s_spotIndex++;
}

/* 0x0003a870  BACTOR/pcxbShaderSelector.cpp (file inferred)
fog: camera position, fog end and scale */
void bFragFogSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	float c[4];

	(void)self;
	c[0] = DAT_002be570[0];
	c[1] = DAT_002be570[1];
	c[2] = DAT_002be570[2];
	c[3] = 1.0f;
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
	c[0] = DAT_002be5e8;
	c[1] = DAT_002be5ec;
	c[2] = 0.0f;
	c[3] = 1.0f;
	D3DDevice_SetVertexShaderConstant(ctx->reg + 1, c, 1);
}

/* 0x0003a8f0  BACTOR/pcxbShaderSelector.cpp (file inferred)
diffuse: the global brightness as alpha */
void bFragDiffuseSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	float c[4];

	(void)self;
	c[3] = (float)DAT_00284b0c * (1.0f / 255.0f);
	c[0] = 1.0f;
	c[1] = 1.0f;
	c[2] = 1.0f;
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
}

/* 0x0003a940  BACTOR/pcxbShaderSelector.cpp (file inferred)
appends a fragment to a shader build (thiscall on build); returns its
first constant register, or -1 when full */
int bShaderBuildAdd(struct BShaderBuild *build, struct BShaderMode *mode, struct BShaderFragment *fragment)
{
	int n = build->count;
	int last;

	if (n == 0x20)
		return -1;
	build->fragments[n] = fragment;
	if (build->count == 0)
	{
		/* skinned shaders keep c0-c47 for the bone matrices */
		build->constantBase[0] = mode->numBones < 1 ? 2 : 0x30;
		build->count = 1;
		return build->constantBase[0];
	}
	last = FRAGMENT_VTBL(build->fragments[n - 1])->numConstants(build->fragments[n - 1]);
	build->constantBase[n] = last + build->constantBase[n - 1];
	build->count = n + 1;
	return build->constantBase[n];
}

/* the base class vtable (0x00257e58) */
static const struct BShaderFragmentVtbl s_vtblBase = { bFragmentNop, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };

/* 0x0003a9c0  BACTOR/pcxbShaderSelector.cpp (file inferred)
CBShaderFragment::CBShaderFragment (thiscall on self) */
void bShaderFragmentInit(struct BShaderFragment *self, int numParams, int numConstants, const char *code)
{
	const char *p;

	self->vtable = (const void *const *)&s_vtblBase;
	self->numLines = 0;
	for (p = code; *p != '\0'; p++)
		self->numLines += (*p == '\n');
	self->code = code;
	self->numParams = numParams;
	self->numConstants = numConstants;
}

/* 0x0003aa10  BACTOR/pcxbShaderSelector.cpp (file inferred)
CBShaderFragment::AppendFragment: appends the fragment's code to text, with
%0-%9 replaced by the string arguments and %c0-%c9 by its constant
registers; returns the number of lines added */
int bShaderAppendFragment(struct BShaderFragment *fragment, struct BShaderMode *mode, char *text, ...)
{
	const char *args[8];
	char constant[32];
	const char *src;
	const char *percent;
	const char *insert;
	unsigned int i;
	int base;
	va_list ap;

	if ((unsigned int)fragment->numParams > 8)
	{
		ba_bkPrintf("CBShaderFragment::AppendFragment: Code fragments must not need more than %d parameters\n", 8);
		return 0;
	}
	va_start(ap, text);
	for (i = 0; i < (unsigned int)fragment->numParams; i++)
		args[i] = va_arg(ap, const char *);
	va_end(ap);
	base = bShaderBuildAdd(&mode->build, mode, fragment);
	src = fragment->code;
	percent = strchr(src, '%');
	while (percent != NULL)
	{
		strncat(text, src, percent - src);
		percent++;
		if (*percent == 'c')
		{
			percent++;
			sprintf(constant, "c[%d]", *percent - '0' + base);
			insert = constant;
		}
		else
		{
			insert = args[*percent - '0'];
		}
		strcat(text, insert);
		src = percent + 1;
		percent = strchr(src, '%');
	}
	strcat(text, src);
	return fragment->numLines;
}

/* 0x0003ab60  BACTOR/pcxbShaderSelector.cpp (file inferred)
loads the constants of every fragment of a shader mode.
Original register argument: eax inst. */
int bShaderSetConstants(struct BActorInstance *inst, int pass, struct BShaderMode *mode)
{
	struct BShaderContext ctx;
	struct BShaderFragment *fragment;
	unsigned int i;

	if ((inst->flags & 2) == 0)
	{
		ba_MatInverse(inst->inverse, inst->matrix);
		inst->flags |= 2;
	}
	s_dirIndex = 0;
	s_pointIndex = 0;
	s_spotIndex = 0;
	ctx.inst = inst;
	ctx.pass = pass;
	for (i = 0; i < (unsigned int)mode->build.count; i++)
	{
		fragment = mode->build.fragments[i];
		ctx.reg = mode->build.constantBase[i];
		FRAGMENT_VTBL(fragment)->begin(fragment, &ctx);
		FRAGMENT_VTBL(fragment)->setConstants(fragment, &ctx);
	}
	return 1;
}

/* 0x0003ac00  BACTOR/pcxbShaderSelector.cpp (file inferred)
the shader key of a pass; returns 0 when the pass draws nothing.
Original register arguments: edx key, ebx inst, esi pass. */
int bShaderGetKey(struct BShaderMode *key, struct BActorInstance *inst, int pass)
{
	struct BActor *actor = inst->actor;
	uint32_t flags;

	memset(key, 0, sizeof(*key));
	key->numBones = actor->skin.numWeights;
	if (key->numBones != 0 && DAT_002bc3e8 != 0)
		key->numBones = 1;
	key->pass = pass;
	flags = (key->flags & ~1u) | (pass == 0);
	key->flags = flags;
	switch (inst->passMode[pass])
	{
	case 0:
		if ((flags & 1) == 0)
			return 0;
		break;
	case 2:
		key->flags = flags | 2;
		return (flags & 1) != 0;
	case 3:
		key->flags = flags | 0x10;
		return (flags & 1) != 0;
	case 4:
		key->flags = flags | 4;
		return (flags & 1) == 0;
	case 5:
		key->flags = flags | 8;
		return 1;
	case 7:
		key->passParam = inst->passParam[pass];
		break;
	case 8:
		key->flags = flags | 0x40;
		return 1;
	}
	return 1;
}

/* 0x0003ace0  BACTOR/pcxbShaderSelector.cpp (file inferred)
ends a pass: the fragments' end methods (restore the render states) */
void bShaderEndPass(struct BActorInstance *inst, int pass)
{
	struct BShaderContext ctx;
	struct BShaderFragment *fragment;
	unsigned int i;

	ctx.inst = inst;
	ctx.pass = pass;
	for (i = 0; i < (unsigned int)bactor_currentMode->build.count; i++)
	{
		ctx.reg = bactor_currentMode->build.constantBase[i];
		fragment = bactor_currentMode->build.fragments[i];
		FRAGMENT_VTBL(fragment)->end(fragment, &ctx);
	}
}

/* 0x0003ad40  BACTOR/pcxbShaderSelector.cpp (file inferred)
the lighting key: lighting on, and the numbers of enabled lights by type.
Original register argument: eax key. */
void bShaderGetLightingKey(int *key)
{
	struct BLight *light;

	key[2] = 0;
	key[3] = 0;
	key[4] = 0;
	key[0] = 0;
	key[1] = (int)DAT_002be618;
	key[5] = 0;
	if (DAT_002be618 != 0)
	{
		for (light = g_lightsOn.next; light != &g_lightsOn; light = light->next)
		{
			if (light->type == BLIGHT_DIRECTIONAL)
				key[2]++;
			else if (light->type == BLIGHT_POINT)
				key[3]++;
			else if (light->type == BLIGHT_SPOT)
				key[4]++;
		}
	}
}

/* 0x0003ada0  BACTOR/pcxbShaderSelector.cpp (file inferred)
a cache entry for a new lighting state: a free one, or else the least
recently used (its shaders are not deleted, as in the original) */
struct BLightingState *bShaderNewLightingState(const int *key)
{
	struct BLightingState *state;
	struct BLightingState *other;
	int i;

	if (bactor_numLightingStates < bactor_maxLightingStates)
	{
		state = &bactor_lightingStates[bactor_numLightingStates++];
	}
	else
	{
		state = bactor_lightingStates;
		for (i = 1; i < bactor_numLightingStates; i++)
		{
			other = &bactor_lightingStates[i];
			if ((uint32_t)other->lastUsed < (uint32_t)state->lastUsed ||
				(other->lastUsed == state->lastUsed && (uint32_t)other->useCount < (uint32_t)state->useCount))
				state = other;
		}
	}
	memcpy(state->key, key, sizeof(state->key));
	state->numModes = 0;
	state->useCount = 0;
	state->lastUsed = DAT_002bc890;
	return state;
}

/* 0x0003ae20  BACTOR/pcxbShaderSelector.cpp (file inferred)
baGenerateShader: builds, assembles and stores the shader of a key */
struct BShaderMode *baGenerateShader(struct BLightingState *state, struct BShaderMode *key,
	struct BActorInstance *inst, int pass)
{
	struct BShaderMode *mode;
	struct BShaderFragment *effect;
	struct BLight *light;
	int normals = 0; /* the shader needs the normal (r2) */
	int lighting = 0;
	int noDiffuse = 0;
	int noTexCoord = 0;
	int numBones;
	int ops;
	uint32_t flags;
	char *text = bactor_shaderText;

	if (state->numModes == bactor_maxShaderModes)
	{
		ba_bkPrintf("baGenerateShader: Couldn't add new actor description because all slots are used!\n");
		return NULL;
	}
	mode = &state->modes[state->numModes];
	mode->numBones = key->numBones;
	mode->pass = key->pass;
	mode->flags = key->flags;
	mode->passParam = key->passParam;
	mode->build.count = 0;
	flags = key->flags;
	if ((flags & 0x10) == 0)
	{
		normals = !(state->key[1] == 0 && (flags & 0xe) == 0 && key->passParam == 0 && (flags & 0x40) == 0);
		lighting = !(DAT_002be618 == 0 || (mode->flags & 4) != 0 || mode->passParam != 0 || (mode->flags & 0x40) != 0);
	}
	memcpy(text, "xvs.1.1\n\0\0\0", 12);
	numBones = mode->numBones;
	if (numBones == 0)
	{
		ops = bShaderAppendFragment(bactor_fragments[FRAG_OP2], mode, text, "mov", "r0", "v0");
		if (normals)
			ops += bShaderAppendFragment(bactor_fragments[FRAG_OP2], mode, text, "mov", "r2", "v3");
	}
	else
	{
		ops = bShaderAppendFragment(bactor_fragments[FRAG_MOVA0], mode, text, "v2.x");
		ops += bShaderAppendFragment(bactor_fragments[FRAG_SKINPOS], mode, text, "r0");
		if (normals)
			ops += bShaderAppendFragment(bactor_fragments[FRAG_SKINNORMAL], mode, text, "r2");
		if (numBones > 1)
		{
			ops += bShaderAppendFragment(bactor_fragments[FRAG_MOVA0], mode, text, "v2.y");
			ops += bShaderAppendFragment(bactor_fragments[FRAG_SKINPOS], mode, text, "r1");
			ops += bShaderAppendFragment(bactor_fragments[FRAG_MULMAD], mode, text, "r0", "v1.x", "r1", "v1.y");
			if (normals)
			{
				ops += bShaderAppendFragment(bactor_fragments[FRAG_SKINNORMAL], mode, text, "r3");
				ops += bShaderAppendFragment(bactor_fragments[FRAG_MULMAD], mode, text, "r2", "v1.x", "r3", "v1.y");
			}
		}
		if (numBones >= 3)
		{
			ops += bShaderAppendFragment(bactor_fragments[FRAG_MOVA0], mode, text, "v2.z");
			ops += bShaderAppendFragment(bactor_fragments[FRAG_SKINPOS], mode, text, "r1");
			ops += bShaderAppendFragment(bactor_fragments[FRAG_OP4], mode, text, "mad", "r0", "r1", "v1.z", "r0");
			if (normals)
			{
				ops += bShaderAppendFragment(bactor_fragments[FRAG_SKINNORMAL], mode, text, "r3");
				ops += bShaderAppendFragment(bactor_fragments[FRAG_OP4], mode, text, "mad", "r2", "r3", "v1.z", "r2");
			}
		}
		if (normals && numBones > 0)
			ops += bShaderAppendFragment(bactor_fragments[FRAG_NORMALIZE], mode, text);
	}
	if (lighting)
	{
		ops += bShaderAppendFragment(bactor_fragments[FRAG_AMBIENT], mode, text);
		for (light = g_lightsOn.next; light != &g_lightsOn; light = light->next)
		{
			if (light->type == BLIGHT_DIRECTIONAL)
				ops += bShaderAppendFragment(bactor_fragments[FRAG_DIRLIGHT], mode, text);
			else if (light->type == BLIGHT_POINT)
				ops += bShaderAppendFragment(bactor_fragments[FRAG_POINTLIGHT], mode, text);
			else if (light->type == BLIGHT_SPOT)
				ops += bShaderAppendFragment(bactor_fragments[FRAG_SPOTLIGHT], mode, text);
		}
	}
	flags = mode->flags;
	effect = NULL;
	if (flags & 2)
	{
		effect = bactor_fragments[FRAG_CEL];
	}
	else
	{
		if (flags & 4)
			effect = bactor_fragments[FRAG_FLAT];
		else if (flags & 8)
			effect = bactor_fragments[FRAG_FLATALPHA];
		else if (flags & 0x40)
			effect = bactor_fragments[FRAG_COLOUR];
		else if (flags & 0x10)
			effect = bactor_fragments[FRAG_BLACK];
		else if (flags & 0x20)
			effect = bactor_fragments[FRAG_ENVMAP];
		else if (mode->passParam != 0)
		{
			/* a custom fragment: the first field of the pass parameter */
			effect = *(struct BShaderFragment **)(intptr_t)inst->passParam[pass];
			noTexCoord = 1;
		}
		if (effect != NULL && (flags & 0x20) == 0)
			noDiffuse = 1;
	}
	if (effect != NULL)
		ops += bShaderAppendFragment(effect, mode, text);
	ops += bShaderAppendFragment(bactor_fragments[FRAG_POSITION], mode, text, "r0");
	if (!noTexCoord)
		ops += bShaderAppendFragment(bactor_fragments[FRAG_TEXCOORD], mode, text, "v5");
	if (!noDiffuse)
	{
		if (lighting)
		{
			ops += bShaderAppendFragment(bactor_fragments[FRAG_OP3], mode, text, "mul", "r11", "v4", "r3");
			ops += bShaderAppendFragment(bactor_fragments[FRAG_DIFFUSE], mode, text, "r11");
		}
		else
		{
			ops += bShaderAppendFragment(bactor_fragments[FRAG_DIFFUSE], mode, text, "v4");
		}
	}
	bAssembleShader(mode, inst);
	ba_bkPrintf("Created new vertex shader with %d opcodes\n", ops);
	state->numModes++;
	return mode;
}

/* 0x0003b310  BACTOR/pcxbShaderSelector.cpp (file inferred)
CBShaderFragment's number of constants (thiscall) */
int bShaderFragmentNumConstants(struct BShaderFragment *self)
{
	return self->numConstants;
}

/* 0x0003b320  BACTOR/pcxbShaderSelector.cpp (file inferred)
cel shading: stage 1 looks the shade up in the pass parameter's texture */
void bFragCelBegin(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_ADDRESSU, 3);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_MIPFILTER, 0);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_MAGFILTER, 1);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_MINFILTER, 1);
	ba_bdSetTexture(1, *(void **)(intptr_t)PASS_PARAM(ctx));
	DAT_002bee38 = 0;
	ba_bdLockState(2);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLOROP, 4);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLORARG1, 2);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLORARG2, 1);
}

/* 0x0003b3d0  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragCelEnd(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	(void)ctx;
	ba_bdUnlockState(2);
	DAT_002bee38 = 1;
	ba_bdSetTexture(1, NULL);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_ADDRESSU, 1);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLOROP, 1);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_MAGFILTER, 2);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_MINFILTER, 2);
}

/* 0x0003b440  BACTOR/pcxbShaderSelector.cpp (file inferred)
flat colour with alpha by facing: c0 = colour, c1 = direction to the camera */
void bFragFlatAlphaSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	int *param = PASS_PARAM(ctx); /* offset (float), r, g, b, a */
	static const float origin[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float c[4];
	float o[4];

	(void)self;
	c[0] = (float)param[1] * (1.0f / 255.0f);
	c[1] = (float)param[2] * (1.0f / 255.0f);
	c[2] = (float)param[3] * (1.0f / 255.0f);
	c[3] = (float)DAT_00284b0c * (1.0f / 255.0f) * (float)param[4] * (1.0f / 255.0f);
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
	c[1] = DAT_002be570[1];
	c[0] = DAT_002be570[0];
	c[2] = DAT_002be570[2];
	/* c[3] keeps the alpha above, as in the original */
	ba_MatTransform(c, ctx->inst->inverse, c);
	ba_MatTransform(o, ctx->inst->inverse, origin);
	c[0] = c[0] - o[0];
	c[1] = c[1] - o[1];
	c[2] = c[2] - o[2];
	ba_Vec3Normalize(c, c);
	c[3] = *(float *)&param[0];
	D3DDevice_SetVertexShaderConstant(ctx->reg + 1, c, 1);
}

/* 0x0003b570  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragEnvMapBegin(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	ba_bdSetTexture(1, *(void **)(intptr_t)PASS_PARAM(ctx));
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLOROP, 4);
}

/* 0x0003b5b0  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void bFragEnvMapEnd(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	(void)self;
	(void)ctx;
	ba_bdSetTexture(1, NULL);
	D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLOROP, 1);
}

/* 0x0003b5d0  BACTOR/pcxbShaderSelector.cpp (file inferred)
directional light: direction in model space, colour */
void bFragDirLightSetConstants(struct BShaderFragment *self, struct BShaderContext *ctx)
{
	static const float origin[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	struct BLight *light = bFindEnabledLight(BLIGHT_DIRECTIONAL, s_dirIndex);
	float c[4];
	float o[4];

	(void)self;
	ba_MatTransform(c, ctx->inst->inverse, light->dir);
	ba_MatTransform(o, ctx->inst->inverse, origin);
	c[0] = c[0] - o[0];
	c[1] = c[1] - o[1];
	c[2] = c[2] - o[2];
	ba_Vec3Normalize(c, c);
	D3DDevice_SetVertexShaderConstant(ctx->reg, c, 1);
	c[0] = (float)light->color[0] * (1.0f / 255.0f);
	c[1] = (float)light->color[1] * (1.0f / 255.0f);
	c[3] = 0.0f;
	c[2] = (float)light->color[2] * (1.0f / 255.0f);
	D3DDevice_SetVertexShaderConstant(ctx->reg + 1, c, 1);
	s_dirIndex++;
}

/* the subclass vtables (0x00257fb4-0x002587bc) */
static const struct BShaderFragmentVtbl s_vtblPlain = { bFragmentNop, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblCel = { bFragCelSetConstants, bFragCelBegin, bFragCelEnd, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblFlat = { bFragFlatSetConstants, bFragFlatBegin, bFragFlatEnd, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblFlatAlpha = { bFragFlatAlphaSetConstants, bFragFlatAlphaBegin, bFragFlatAlphaEnd, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblBlack = { bFragBlackSetConstants, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblEnvMap = { bFragEnvMapSetConstants, bFragEnvMapBegin, bFragEnvMapEnd, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblAmbient = { bFragAmbientSetConstants, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblDirLight = { bFragDirLightSetConstants, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblPointLight = { bFragPointLightSetConstants, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblSpotLight = { bFragSpotLightSetConstants, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblFog = { bFragFogSetConstants, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblDiffuse = { bFragDiffuseSetConstants, bFragmentNop, bFragmentNop, bShaderFragmentNumConstants };
static const struct BShaderFragmentVtbl s_vtblColour = { bFragColourSetConstants, bFragColourBegin, bFragColourEnd, bShaderFragmentNumConstants };

/* creates one fragment (operator new, the constructor, the subclass vtable) */
static struct BShaderFragment *bNewFragment(unsigned int size, int line, const struct BShaderFragmentVtbl *vtbl,
	int numParams, int numConstants, const char *code)
{
	struct BShaderFragment *fragment =
		ba_bkAlloc(size, "C:\\Babel\\Xbox\\Src\\BACTOR\\pcxbShaderSelector.cpp", line, 0x1002);

	if (fragment == NULL)
		return NULL;
	bShaderFragmentInit(fragment, numParams, numConstants, code);
	fragment->vtable = (const void *const *)vtbl;
	return fragment;
}

/* 0x0003b700  BACTOR/pcxbShaderSelector.cpp
creates the shader cache and the code fragments */
void baInitShaderSelector(void)
{
	bInitShaderCache();
	ba_bkPushAllocName("Shader Fragment");
	bactor_fragments[FRAG_OP2] = bNewFragment(0x14, 0x2de, &s_vtblPlain, 3, 0, "%0 %1, %2\n");
	bactor_fragments[FRAG_OP3] = bNewFragment(0x14, 0x2df, &s_vtblPlain, 4, 0, "%0 %1, %2, %3\n");
	bactor_fragments[FRAG_OP4] = bNewFragment(0x14, 0x2e0, &s_vtblPlain, 5, 0, "%0 %1, %2, %3, %4\n");
	bactor_fragments[FRAG_MOVA0] = bNewFragment(0x14, 0x2e1, &s_vtblPlain, 1, 0, "mov a0.x, %0\n");
	bactor_fragments[FRAG_MULMAD] = bNewFragment(0x14, 0x2e2, &s_vtblPlain, 4, 0, "mul %0, %0, %1\nmad %0, %2, %3, %0\n");
	bactor_fragments[FRAG_SKINPOS] = bNewFragment(0x14, 0x2e3, &s_vtblPlain, 1, 0,
		"dp4 %0.x, v0, c[a0.x + 0]\ndp4 %0.y, v0, c[a0.x + 1]\ndp4 %0.z, v0, c[a0.x + 2]\nmov %0.w, v0.w\n");
	bactor_fragments[FRAG_SKINNORMAL] = bNewFragment(0x14, 0x2e4, &s_vtblPlain, 1, 0,
		"dp3 %0.x, v3, c[a0.x + 0]\ndp3 %0.y, v3, c[a0.x + 1]\ndp3 %0.z, v3, c[a0.x + 2]\nmov %0.w, v0.w\n");
	bactor_fragments[FRAG_NORMALIZE] = bNewFragment(0x14, 0x2e5, &s_vtblPlain, 0, 0,
		"dp3 r11.w, r2, r2\nrsq r11.w, r11.w\nmul r2.xyz, r2.xyz, r11.w\n");
	bactor_fragments[FRAG_CEL] = bNewFragment(0x14, 0x2e6, &s_vtblCel, 0, 1,
		"dp3 r11.w, %c0, r2\nmax oT1.x, v5.z, r11.w\nmov oT1.y, %c0.w\n");
	bactor_fragments[FRAG_FLAT] = bNewFragment(0x18, 0x2e7, &s_vtblFlat, 0, 2,
		"mov oD0, %c0\nmad r0.xyz, r2, %c1.w, r0\n");
	bactor_fragments[FRAG_FLATALPHA] = bNewFragment(0x14, 0x2e8, &s_vtblFlatAlpha, 0, 2,
		"dp3 r11.w, r2, %c1\nmov oD0, %c0\nmul oD0.w, %c0.w, r11.w\nmad r0.xyz, r2, %c1.w, r0\n");
	bactor_fragments[FRAG_BLACK] = bNewFragment(0x14, 0x2e9, &s_vtblBlack, 0, 1, "mov oD0, %c0\n");
	bactor_fragments[FRAG_ENVMAP] = bNewFragment(0x14, 0x2ea, &s_vtblEnvMap, 0, 6,
		"dp4 r10.x, v0, %c0\ndp4 r10.y, v0, %c1\ndp4 r10.z, v0, %c2\ndp3 r10.w, r10, r10\nrsq r10.w, r10.w\n"
		"mul r11, r10, r10.w\ndp3 r10.x, v1, %c3\ndp3 r10.y, v1, %c4\ndp3 r10.z, v1, %c5\ndp3 r10.w, r10, r10\n"
		"rsq r10.w, r10.w\nmul r10, r10, r10.w\ndp3 r9, r10, r11\nadd r9, r9, r9\nmul r10, r10, r9\n"
		"add oT1.xy, r11, -r10\n");
	bactor_fragments[FRAG_AMBIENT] = bNewFragment(0x14, 0x2eb, &s_vtblAmbient, 0, 1, "mov r3, %c0\n");
	bactor_fragments[FRAG_DIRLIGHT] = bNewFragment(0x14, 0x2ec, &s_vtblDirLight, 0, 2,
		"dp3 r11.w, %c0, r2\nmax r11.w, %c1.w, r11.w\nmad r3.xyz, %c1.xyz, r11.www, r3.xyz\n");
	bactor_fragments[FRAG_POINTLIGHT] = bNewFragment(0x14, 0x2ed, &s_vtblPointLight, 0, 3,
		"sub r11, r0, %c0\ndp3 r11.w, r11, r11\nrcp r11.w, r11.w\ndp3 r10.w, r11, -r2\nmax r10.w, v5.z, r10.w\n"
		"mul r10.z, r11.w, r10.w\nmul r10.z, r10.z, %c2.x\nmad r3.xyz, %c1, r10.z, r3\n");
	bactor_fragments[FRAG_SPOTLIGHT] = bNewFragment(0x14, 0x2ee, &s_vtblSpotLight, 0, 4,
		"sub r11, r0, %c0\ndp3 r11.w, r11, r11\nrsq r9.w, r11.w\nmul r11.xyz, r11.xyz, r9.w\ndp3 r10.w, %c1, r11\n"
		"sub r10.w, r10.w, %c3.x\nmul r10.w, r10.w, %c3.z\nmax r10.w, r10.w, %c3.w\ndp3 r9.z, -r11, r2\n"
		"max r9.z, %c3.w, r9.z\nmul r10.w, r9.z, r10.w\nmul r9.z, %c3.y, r9.w\nmul r10.w, r10.w, r9.z\n"
		"mad r3.xyz, %c2, r10.w, r3\n");
	bactor_fragments[FRAG_FOG] = bNewFragment(0x14, 0x2ef, &s_vtblFog, 0, 2,
		"sub r11, %c0, r0\ndp3 r11.w, r11, r11\nrsq r11.w, r11.w\nrcp r11.w, r11.w\nsub r11.w, %c1.x, r11.w\n"
		"mul oFog.x, r11.w, %c1.y\n");
	bactor_fragments[FRAG_POSITION] = bNewFragment(0x14, 0x2f0, &s_vtblPlain, 1, 0,
		"dp4 oPos.x, %0, c[92]\ndp4 oPos.y, %0, c[93]\ndp4 oPos.z, %0, c[94]\ndp4 oPos.w, %0, c[95]\n"
		"dp4 oFog.x, %0, c[95]\n");
	bactor_fragments[FRAG_DIFFUSE] = bNewFragment(0x14, 0x2f1, &s_vtblDiffuse, 1, 1, "mul oD0, %0, %c0\n");
	bactor_fragments[FRAG_TEXCOORD] = bNewFragment(0x14, 0x2f2, &s_vtblPlain, 1, 0, "mov oT0, %0\n");
	bactor_fragments[FRAG_COLOUR] = bNewFragment(0x14, 0x2f3, &s_vtblColour, 0, 1, "mov oD0, %c0\n");
	ba_bkPopAllocName();
}

/* 0x0003bc90  BACTOR/pcxbShaderSelector.cpp (file inferred) */
void baShutdownShaderSelector(void)
{
	int i;

	bShutdownShaderCache();
	for (i = 0; i < FRAG_COUNT; i++)
		ba_bkFree(bactor_fragments[i]);
}

/* 0x0003bcc0  BACTOR/pcxbShaderSelector.cpp (file inferred)
makes the lighting state of the enabled lights current; 0 on failure */
int bShaderUpdateLightingState(void)
{
	int key[9];
	struct BLightingState *state;
	int i;

	bShaderGetLightingKey(key);
	if (bactor_currentLighting != NULL && memcmp(bactor_currentLighting->key, key, 5 * sizeof(int)) == 0)
		return 1;
	state = bactor_lightingStates;
	for (i = 0; i < bactor_numLightingStates; i++, state++)
	{
		if (memcmp(key, state->key, 5 * sizeof(int)) == 0)
			goto found;
	}
	state = bShaderNewLightingState(key);
	if (state == NULL)
		return 0;
found:
	bactor_currentLighting = state;
	state->lastUsed = DAT_002bc890;
	bactor_currentLighting->useCount++;
	return 1;
}

/* 0x0003bd60  BACTOR/pcxbShaderSelector.cpp (file inferred)
baShaderSelect: selects (generating it if needed) the vertex shader of a
pass and loads its constants; with a mesh also its vertex stream, else the
soft-skin vertex buffer. Returns 0 when the pass is not drawn. */
int bShaderSelect(struct BActorInstance *inst, int pass, struct BMesh *mesh)
{
	struct BActor *actor = inst->actor;
	struct BShaderMode key;
	struct BLightingState *state;
	uint32_t handle;
	int i;

	if (bShaderGetKey(&key, inst, pass) == 0)
		return 0;
	if (bShaderUpdateLightingState() == 0)
	{
		ba_bkPrintf("baShaderSelect: Evaluation of the lighting state failed!\n");
		return 0;
	}
	state = bactor_currentLighting;
	bactor_currentMode = state->modes;
	handle = (uint32_t)(uintptr_t)actor; /* never used: as in the original */
	for (i = 0; i < state->numModes; i++)
	{
		if (memcmp(bactor_currentMode, &key, 4 * sizeof(int)) == 0)
		{
			handle = bactor_currentMode->handle;
			break;
		}
		bactor_currentMode++;
	}
	if (i == state->numModes)
	{
		bactor_currentMode = baGenerateShader(state, &key, inst, pass);
		if (bactor_currentMode == NULL)
		{
			ba_bkPrintf("baShaderSelect: baGenerateShader FAILED!\n");
			return 0;
		}
		handle = bactor_currentMode->handle;
	}
	if (bShaderSetConstants(inst, bactor_currentMode->pass, bactor_currentMode) == 0)
		ba_bkPrintf("baShaderSelect: Set vertex shader data failed!\n");
	if (mesh != NULL)
	{
		DAT_002bc400 = handle;
		D3DDevice_SetVertexShader(handle);
		D3DDevice_SetStreamSource(0, mesh->vertexBuffer,
			ba_bdVertexSize(bactor_vertexFormats[inst->actor->vertexFormat]));
		DAT_00284c10 = (struct bdVertexBuffer *)1;
		return 1;
	}
	ba_bdSetVertexShader(handle, actor->skin.verts);
	return 1;
}
