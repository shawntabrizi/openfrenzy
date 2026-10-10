/*
BACTOR/unplaced.c  <-  C:\Babel\Xbox\Src\BACTOR\ (source file unknown)

The functions between extras.cpp (0x0003c780) and xbShaderSelector.cpp
(0x0003dd50) that no tag places: the set resource handlers, key track
sampling, soft-skin drawing, mesh fixup, and the D3D side of lights.
*/

#include "bactor.h"
#include "bactor_extern.h"

/* 0x002bb230: number of soft-skinned actors drawn (64-bit) */
static uint64_t s_skinsDrawn;

/* ---------- sets: handlers by set type (0x00284b64, 0x00284b6c, 0x00284b74;
type 1 has empty handlers, 0x001d5950) */

static void bSetNop(struct BSet *set)
{
	(void)set;
}
static void bSetDrawNop(struct BSet *set, int pass)
{
	(void)set;
	(void)pass;
}
static void (*const s_setInit[2])(struct BSet *set) = { bSetInit, bSetNop };
static void (*const s_setFree[2])(struct BSet *set) = { bSetFree, bSetNop };
static void (*const s_setDraw[2])(struct BSet *set, int pass) = { bSetDraw, bSetDrawNop };

/* 0x0003ca40  BACTOR/(unplaced) */
int baInitSets(void)
{
	/* FUN_000a6fa0 is a shared "return 1" function (identical-code folding) */
	return FUN_000a6fa0() != 0;
}

/* 0x0003ca50  BACTOR/(unplaced)
bKernel's loader of set resources: finds the set crc in a package */
struct BSet *baLoadSet(void *package, uint32_t crc)
{
	struct BSet *set = (struct BSet *)ba_bkFindResource(package, crc, NULL, NULL, NULL, 0);

	if (set == NULL)
		return NULL;
	set->handle = *(uint32_t *)package; /* the package id */
	s_setInit[set->type](set);
	return set;
}

/* 0x0003ca90  BACTOR/(unplaced)
bKernel's release function of set resources (resource type 6) */
void baFreeSet(struct BSet *set)
{
	s_setFree[set->type](set);
	if ((int32_t)set->handle >= 0)
		ba_bkFree(set);
}

/* 0x0003cac0  BACTOR/(unplaced) */
void baDrawSet(struct BSet *set, int pass)
{
	s_setDraw[set->type](set, pass);
}

/* 0x0003cae0  BACTOR/(unplaced)
samples a vector key track at time (frames << 10), linearly interpolated */
void bGetKey3(float *out, struct BKeyTrack3 *track, int time)
{
	struct BKey3 *keys = track->keys;
	struct BKey3 *key;
	int hi = track->numKeys - 1;
	int lo = 0;
	int mid;
	int t;
	float f;
	float a[3];

	if ((int)((unsigned int)keys[track->numKeys - 1].frame * 0x400) <= time)
	{
		key = &keys[hi];
		out[0] = (float)key->value[0] * track->scale[0] + track->base[0];
		out[1] = (float)key->value[1] * track->scale[1] + track->base[1];
		out[2] = (float)key->value[2] * track->scale[2] + track->base[2];
		return;
	}
	mid = hi >> 1;
	key = &keys[mid];
	t = (int)((unsigned int)key->frame * 0x400);
	while (t != time)
	{
		if (t >= time)
		{
			hi = mid - 1;
			if (hi < lo)
			{
				key--;
				goto interpolate;
			}
		}
		else
		{
			lo = mid + 1;
			if (hi < lo)
				goto interpolate;
		}
		mid = (lo + hi) >> 1;
		key = &keys[mid];
		t = (int)((unsigned int)key->frame * 0x400);
	}
	out[0] = (float)key->value[0] * track->scale[0] + track->base[0];
	out[1] = (float)key->value[1] * track->scale[1] + track->base[1];
	out[2] = (float)key->value[2] * track->scale[2] + track->base[2];
	return;

interpolate:
	f = (float)(int)(time - (unsigned int)key->frame * 0x400) /
		(float)(int)(((unsigned int)key[1].frame - (unsigned int)key->frame) * 0x400);
	a[0] = (float)key->value[0] * track->scale[0] + track->base[0];
	a[1] = (float)key->value[1] * track->scale[1] + track->base[1];
	a[2] = (float)key->value[2] * track->scale[2] + track->base[2];
	out[0] = (((float)key[1].value[0] * track->scale[0] + track->base[0]) - a[0]) * f + a[0];
	out[1] = (((float)key[1].value[1] * track->scale[1] + track->base[1]) - a[1]) * f + a[1];
	out[2] = (((float)key[1].value[2] * track->scale[2] + track->base[2]) - a[2]) * f + a[2];
}

/* 0x0003cca0  BACTOR/(unplaced)
samples a rotation key track at time, spherically interpolated */
void bGetKey4(float *out, struct BKeyTrack4 *track, int time)
{
	struct BKey4 *keys = track->keys;
	struct BKey4 *key;
	int hi = track->numKeys - 1;
	int lo = 0;
	int mid;
	int t;
	float a[4];
	float b[4];
	int i;

	key = &keys[hi];
	if ((int)((unsigned int)key->frame * 0x400) <= time)
		goto value;
	mid = hi >> 1;
	key = &keys[mid];
	t = (int)((unsigned int)key->frame * 0x400);
	while (t != time)
	{
		if (t >= time)
		{
			hi = mid - 1;
			if (hi < lo)
			{
				key--;
				goto interpolate;
			}
		}
		else
		{
			lo = mid + 1;
			if (hi < lo)
				goto interpolate;
		}
		mid = (hi + lo) >> 1;
		key = &track->keys[mid];
		t = (int)((unsigned int)key->frame * 0x400);
	}
value:
	for (i = 0; i < 4; i++)
		out[i] = (float)key->value[i] * track->scale[i] + track->base[i];
	return;

interpolate:
	for (i = 0; i < 4; i++)
	{
		a[i] = (float)key[0].value[i] * track->scale[i] + track->base[i];
		b[i] = (float)key[1].value[i] * track->scale[i] + track->base[i];
	}
	ba_QuatSlerp(out, a, b,
		(float)(int)(time - (unsigned int)key->frame * 0x400) /
			(float)(int)(((unsigned int)key[1].frame - (unsigned int)key->frame) * 0x400));
}

/* 0x0003ced0  BACTOR/(unplaced)
loads the transform (c92-c95) and the bone matrices (c-96 and up) of a
soft-skinned instance. Original register argument: edi inst. */
int bSetSkinConstants(struct BActorInstance *inst)
{
	float constants[16];
	unsigned int numBones;
	int rest;

	if (DAT_002be5c8 == 0)
	{
		if (((((uintptr_t)PTR_DAT_00284c70) | (uintptr_t)DAT_002be3b0 | (uintptr_t)DAT_002be370) & 0xf) == 0)
			ba_MatMulAligned(DAT_002be3b0, PTR_DAT_00284c70, DAT_002be370);
		else
			ba_MatMul(DAT_002be3b0, PTR_DAT_00284c70, DAT_002be370);
		DAT_002be5c8 = 1;
	}
	bMatMulTranspose(constants, DAT_002be3b0, inst->matrix, 4);
	D3DDevice_SetShaderConstantMode(1);
	D3DDevice_SetVertexShaderConstant(0x5c, constants, 4);
	numBones = inst->actor->numBones;
	if (numBones < 0x14)
	{
		rest = 0;
	}
	else
	{
		rest = (int)numBones - 0x13;
		numBones = 0x13;
	}
	D3DDevice_SetVertexShaderConstant(-0x60, inst->bones, numBones * 3);
	if (rest != 0)
		D3DDevice_SetVertexShaderConstant(-0x24, (char *)inst->bones + 0x390, rest * 3);
	return 1;
}

/* soft-skin draw callbacks (0x00284bf8) */
typedef void (*BDrawSkinFn)(struct BSoftSkin *skin, const int *passInfo);
static const BDrawSkinFn s_drawSkin[6] = {
	bDrawSkinUntextured,
	bDrawSkinUntexturedSkip,
	bDrawSkinTex0,
	bDrawSkinTex0Skip,
	bDrawSkinTex01,
	bDrawSkinTex01Skip,
};

/* 0x00284b80: per pass mode: texture 0 used, texture 1 used, skip flagged primitives */
static const int s_skinPassInfo[10][3] = {
	{ 0, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
	{ 0, 0, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
};

/* 0x0003cfa0  BACTOR/(unplaced)
draws a soft-skinned instance (passes bit 0) */
void bDrawSoftSkin(struct BActorInstance *inst, int passes)
{
	struct BActor *actor = inst->actor;
	const int *info;
	int pass;
	int tex0;
	int tex1;
	int skip;

	if ((passes & 1) == 0)
		return;
	s_skinsDrawn++;
	if (bSetSkinConstants(inst) == 0)
		return;
	for (pass = 0; pass < 3; pass++)
	{
		if (inst->passMode[pass] == 0 || bShaderSelect(inst, pass, NULL) == 0)
			continue;
		info = s_skinPassInfo[inst->passMode[pass]];
		tex0 = DAT_002bee34[0] == 0 ? 0 : info[0];
		if ((inst->actor->flags & 0x10) == 0 && DAT_002bee38 != 0)
			tex1 = info[1];
		else
			tex1 = 0;
		if (tex0 == 0)
			ba_bdClearTexture(0);
		if (tex1 == 0)
			ba_bdClearTexture(1);
		skip = (info[2] != 0 || DAT_002bee4c != 0) ? 1 : 0;
		s_drawSkin[skip + (tex1 + tex0) * 2](&actor->skin, info);
		bShaderEndPass(inst, pass);
	}
	D3DDevice_SetShaderConstantMode(0);
}

/* 0x0003d0d0  BACTOR/(unplaced)
(the mesh's vertex buffer is in the file: only the stream is reset) */
void bReleaseMesh(struct BActorNodeDef *def)
{
	(void)def;
	ba_bdSetVertexShader(0x152, NULL);
}

#define SKIN_PRIM_VERTS(prim) \
	(D3DPRIMITIVETOVERTEXCOUNT[(prim)->type][0] * (prim)->numPrims + D3DPRIMITIVETOVERTEXCOUNT[(prim)->type][1])

/* sets the address modes of stage 0 from a primitive's wrap flags when they
change (1 wrap, 3 clamp) */
#define SKIN_WRAP(flags, lastU, lastV) \
	do \
	{ \
		uint32_t bit_ = (flags) & 1; \
		if (bit_ != (lastU)) \
		{ \
			D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSU, ((flags) & 1) == 0 ? 3 : 1); \
			(lastU) = bit_; \
		} \
		bit_ = (flags) & 2; \
		if (bit_ != (lastV)) \
		{ \
			D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSV, ((flags) & 2) == 0 ? 3 : 1); \
			(lastV) = bit_; \
		} \
	} while (0)

/* sets texture 0 of a group through bDisplay, with its colour key mode */
static void bSkinSetTexture0(struct BPrimGroup *group)
{
	if (group->texture0 == NULL)
	{
		ba_bdSetTexture(0, NULL);
	}
	else
	{
		ba_bdSetTexture(0, group->texture0);
		if (*((signed char *)group->texture0 + 0x3c) != -1)
			ba_bdSetState(10, (uint32_t)(int)*((signed char *)group->texture0 + 0x3c), 0);
	}
}

/* 0x0003d0e0  BACTOR/(unplaced)
soft-skin draw callback: untextured */
void bDrawSkinUntextured(struct BSoftSkin *skin, const int *passInfo)
{
	struct BPrimGroup *group = skin->groups;
	struct BSkinPrim *prim = skin->prims;
	int vertex = 0;
	unsigned int g;
	int n;

	(void)passInfo;
	for (g = skin->numGroups; g != 0; g--, group++)
	{
		for (n = group->numPrims; n != 0; n--, prim++)
		{
			D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
			vertex += prim->numVerts;
		}
	}
}

/* 0x0003d150  BACTOR/(unplaced)
soft-skin draw callback: texture 0 (all groups but the last set their texture
and palette on the device directly) */
void bDrawSkinTex0(struct BSoftSkin *skin, const int *passInfo)
{
	struct BPrimGroup *group = skin->groups;
	struct BSkinPrim *prim = skin->prims;
	uint32_t lastU = 0xffffffff;
	uint32_t lastV = 0xffffffff;
	int vertex = 0;
	unsigned int g;
	char *texture;
	int n;

	(void)passInfo;
	ba_bdSetTexture(0, NULL);
	if (skin->groups[0].texture0 != NULL)
	{
		D3DDevice_SetTextureState_Deferred(0, D3DTSS_COLOROP, 5);
		D3DDevice_SetTextureState_Deferred(0, D3DTSS_ALPHAOP, 5);
		D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSU, 3);
		D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSV, 3);
		lastV = 0;
		lastU = 0;
	}
	for (g = skin->numGroups; --g != 0; group++)
	{
		texture = group->texture0;
		D3DDevice_SetTexture(0, *(D3DBaseTexture **)(texture + 0x20));
		if (*(int16_t *)(texture + 0x2c) == 5)
			D3DDevice_SetPalette(0, (D3DPalette *)(texture + 0x40));
		if (*((signed char *)group->texture0 + 0x3c) != -1)
			ba_bdSetState(10, (uint32_t)(int)*((signed char *)group->texture0 + 0x3c), 0);
		if ((skin->flags & 1) == 0)
		{
			for (n = group->numPrims; n != 0; n--, prim++)
			{
				SKIN_WRAP(prim->flags, lastU, lastV);
				D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
				vertex += prim->numVerts;
			}
		}
		else
		{
			for (n = group->numPrims; n != 0; n--, prim++)
			{
				D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
				vertex += prim->numVerts;
			}
		}
	}
	/* the last group */
	bSkinSetTexture0(group);
	if ((skin->flags & 1) == 0 && group->texture0 != NULL)
	{
		for (n = group->numPrims; n != 0; n--, prim++)
		{
			SKIN_WRAP(prim->flags, lastU, lastV);
			D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
			vertex += prim->numVerts;
		}
	}
	else
	{
		for (n = group->numPrims; n != 0; n--, prim++)
		{
			D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
			vertex += prim->numVerts;
		}
	}
}

/* 0x0003d440  BACTOR/(unplaced)
soft-skin draw callback: textures 0 and 1 */
void bDrawSkinTex01(struct BSoftSkin *skin, const int *passInfo)
{
	struct BPrimGroup *group = skin->groups;
	struct BSkinPrim *prim = skin->prims;
	uint32_t lastU = 0xffffffff;
	uint32_t lastV = 0xffffffff;
	int vertex = 0;
	unsigned int g;
	int n;

	(void)passInfo;
	for (g = skin->numGroups; g != 0; g--, group++)
	{
		bSkinSetTexture0(group);
		ba_bdSetTexture(1, group->texture1);
		for (n = group->numPrims; n != 0; n--, prim++)
		{
			SKIN_WRAP(prim->flags, lastU, lastV);
			D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
			vertex += prim->numVerts;
		}
	}
}

/* 0x0003d580  BACTOR/(unplaced)
soft-skin draw callback: untextured, skipping flagged primitives when the pass says so */
void bDrawSkinUntexturedSkip(struct BSoftSkin *skin, const int *passInfo)
{
	struct BPrimGroup *group = skin->groups;
	struct BSkinPrim *prim = skin->prims;
	int skip = passInfo[2];
	int vertex = 0;
	unsigned int g;
	int n;

	for (g = skin->numGroups; g != 0; g--, group++)
	{
		for (n = group->numPrims; n != 0; n--, prim++)
		{
			if ((prim->flags & (skip != 0 ? 4 : 0)) == 0)
				D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
			vertex += prim->numVerts;
		}
	}
}

/* 0x0003d610  BACTOR/(unplaced)
soft-skin draw callback: texture 0, skipping flagged primitives when the pass says so */
void bDrawSkinTex0Skip(struct BSoftSkin *skin, const int *passInfo)
{
	struct BPrimGroup *group = skin->groups;
	struct BSkinPrim *prim = skin->prims;
	int skip = passInfo[2];
	uint32_t lastU = 0xffffffff;
	uint32_t lastV = 0xffffffff;
	int vertex = 0;
	unsigned int g;
	int n;

	for (g = skin->numGroups; g != 0; g--, group++)
	{
		bSkinSetTexture0(group);
		for (n = group->numPrims; n != 0; n--, prim++)
		{
			if ((prim->flags & (skip != 0 ? 4 : 0)) == 0)
			{
				if (DAT_002bee4c == 0)
					SKIN_WRAP(prim->flags, lastU, lastV);
				D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
			}
			vertex += prim->numVerts;
		}
	}
}

/* 0x0003d780  BACTOR/(unplaced)
soft-skin draw callback: textures 0 and 1, skipping flagged primitives when the pass says so */
void bDrawSkinTex01Skip(struct BSoftSkin *skin, const int *passInfo)
{
	struct BPrimGroup *group = skin->groups;
	struct BSkinPrim *prim = skin->prims;
	int skip = passInfo[2];
	uint32_t lastU = 0xffffffff;
	uint32_t lastV = 0xffffffff;
	int vertex = 0;
	unsigned int g;
	int n;

	for (g = skin->numGroups; g != 0; g--, group++)
	{
		bSkinSetTexture0(group);
		ba_bdSetTexture(1, group->texture1);
		for (n = group->numPrims; n != 0; n--, prim++)
		{
			if ((prim->flags & (skip != 0 ? 4 : 0)) == 0)
			{
				if (DAT_002bee4c == 0)
					SKIN_WRAP(prim->flags, lastU, lastV);
				D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prim->type, vertex, SKIN_PRIM_VERTS(prim));
			}
			vertex += prim->numVerts;
		}
	}
}

/* 0x0003d8f0  BACTOR/(unplaced)
fixes up a mesh node: pointers, textures, the wrap and texture flags, and
registers its vertex buffer */
void bFixupMesh(char *base, struct BActorNodeDef *def, struct BActor *actor)
{
	struct BMesh *mesh = &def->data.mesh;
	struct BPrimGroup *group;
	struct BPrim *prim;
	D3DVertexBuffer *vb;
	uint32_t flags = 0;
	int numTex0 = 0;
	int numTex1 = 0;
	int g;
	int i;

	mesh->groups = (struct BPrimGroup *)((char *)mesh->groups + (intptr_t)base);
	mesh->shadowEdges = (uint16_t *)((char *)mesh->shadowEdges + (intptr_t)base);
	mesh->prims = (struct BPrim *)((char *)mesh->prims + (intptr_t)base);
	mesh->vertexBuffer = (char *)mesh->vertexBuffer + (intptr_t)base;
	mesh->shadowVerts = (float *)((char *)mesh->shadowVerts + (intptr_t)base);
	mesh->shadowPlanes = (float *)((char *)mesh->shadowPlanes + (intptr_t)base);
	ba_bdVertexSize(bactor_vertexFormats[actor->vertexFormat]); /* result unused, as in the original */
	prim = mesh->prims;
	group = mesh->groups;
	if (mesh->numGroups < 1)
	{
		flags = 4;
	}
	else
	{
		for (g = 0; g < mesh->numGroups; g++, group++)
		{
			group->texture0 = ba_bkFindTexture(0, (int)(intptr_t)group->texture0, actor->handle, actor->package);
			group->texture1 = ba_bkFindTexture(0, (int)(intptr_t)group->texture1, actor->handle, actor->package);
			numTex0 += group->texture0 != NULL;
			numTex1 += group->texture1 != NULL;
			group->flags = 1;
			for (i = 0; i < group->numPrims; i++)
			{
				if ((prim->flags & 3) != 0)
				{
					flags = 1;
					group->flags = 0;
					break;
				}
				prim++;
			}
		}
		if (numTex0 == 0)
		{
			if (numTex1 == 0)
				flags |= 4;
		}
		else if (numTex1 == 0)
		{
			flags |= 2;
		}
	}
	mesh->flags = flags;
	vb = (D3DVertexBuffer *)mesh->vertexBuffer;
	((uint32_t *)vb)[0] = 1;
	((uint32_t *)vb)[1] = 0;
	((uint32_t *)vb)[2] = 0;
	D3DResource_Register((D3DResource *)vb, (char *)vb + 0x10);
	ba_bkFlushResources();
}

/* 0x0003da90  BACTOR/(unplaced)
gives a light the lowest D3D light index no light uses (up to 1000) */
void bLightAssignIndex(struct BLight *light)
{
	struct BLight *l;
	int index;

	for (index = 0; index < 1000; index++)
	{
		for (l = g_lightsOff.next; l != &g_lightsOff; l = l->next)
			if (l->index == index)
				goto used;
		for (l = g_lightsOn.next; l != &g_lightsOn; l = l->next)
			if (l->index == index)
				goto used;
		light->index = index;
		return;
	used:;
	}
	light->index = index;
}

/* 0x0003db10  BACTOR/(unplaced)
sets the D3D light of a light */
void bLightUpdateD3D(struct BLight *light)
{
	D3DLIGHT8 d3d;

	memset(&d3d, 0, sizeof(d3d));
	d3d.Diffuse.r = DAT_002bd220[light->color[0]];
	d3d.Diffuse.g = DAT_002bd220[light->color[1]];
	d3d.Diffuse.a = 1.0f;
	d3d.Diffuse.b = DAT_002bd220[light->color[2]];
	if ((light->flags & BLIGHTF_SPECULAR) == 0)
	{
		d3d.Specular.r = 0.0f;
		d3d.Specular.g = 0.0f;
		d3d.Specular.b = 0.0f;
		d3d.Specular.a = 0.0f;
	}
	else
	{
		d3d.Specular.r = DAT_002bd220[light->color[0]];
		d3d.Specular.a = 1.0f;
		d3d.Specular.g = DAT_002bd220[light->color[1]];
		d3d.Specular.b = DAT_002bd220[light->color[2]];
	}
	switch (light->type)
	{
	case BLIGHT_DIRECTIONAL:
		d3d.Direction.x = -light->pos[0];
		d3d.Direction.y = -light->pos[1];
		d3d.Type = D3DLIGHT_DIRECTIONAL;
		d3d.Direction.z = -light->pos[2];
		D3DDevice_SetLight(light->index, &d3d);
		break;
	case BLIGHT_POINT:
		d3d.Position.x = light->pos[0];
		d3d.Position.y = light->pos[1];
		d3d.Position.z = light->pos[2];
		d3d.Attenuation1 = light->dir[0];
		d3d.Type = D3DLIGHT_POINT;
		d3d.Range = 1000000.0f;
		d3d.Falloff = 1.0f;
		D3DDevice_SetLight(light->index, &d3d);
		break;
	case BLIGHT_SPOT:
		d3d.Position.x = light->pos[0];
		d3d.Position.y = light->pos[1];
		d3d.Position.z = light->pos[2];
		d3d.Direction.x = light->dir[0];
		d3d.Direction.y = light->dir[1];
		d3d.Direction.z = light->dir[2];
		d3d.Attenuation1 = light->range;
		d3d.Phi = light->angle;
		d3d.Type = D3DLIGHT_SPOT;
		d3d.Range = 1000000.0f;
		d3d.Falloff = 1.0f;
		D3DDevice_SetLight(light->index, &d3d);
		break;
	}
}

/* 0x0003dca0  BACTOR/(unplaced) */
void bLightEnableD3D(struct BLight *light)
{
	D3DDevice_LightEnable(light->index, light->flags & BLIGHTF_ENABLED);
}

/* 0x0003dcc0  BACTOR/(unplaced)
the D3D colours are twice the light colour (each call doubles it again) */
void bLightColourChanged(struct BLight *light)
{
	light->color[0] <<= 1;
	light->color[1] <<= 1;
	light->color[2] <<= 1;
	bLightUpdateD3D(light);
}

/* 0x0003dcf0  BACTOR/(unplaced)
the D3D ambient colour: twice the ambient light, clamped to 255 */
void bSetAmbientD3D(void)
{
	uint32_t r = DAT_00284b10 > 0x7f ? 0xff : (uint32_t)(DAT_00284b10 * 2);
	uint32_t g = DAT_00284b14 > 0x7f ? 0xff : (uint32_t)(DAT_00284b14 * 2);
	uint32_t b = DAT_00284b18 > 0x7f ? 0xff : (uint32_t)(DAT_00284b18 * 2);

	D3DDevice_SetRenderState_Deferred(D3DRS_AMBIENT, ((r | 0xffffff00u) << 8 | g) << 8 | b);
}
