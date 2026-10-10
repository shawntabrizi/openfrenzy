/*
BACTOR/pcxbActor.c  <-  C:\Babel\Xbox\Src\BACTOR\pcxbActor.cpp

The Xbox side of actors: soft-skin fixup, node matrices, mesh drawing
(fixed-function or generated vertex shaders), and stencil shadow volumes.

0x00038d90 and 0x00039e20 are tagged with pcxbActor.cpp and the draw
callbacks are in its table at 0x00284b4c; 0x00038670, 0x00038830 and
0x00039fa0-0x0003a220 are placed here by address order (file inferred).
*/

#include "bactor.h"
#include "bactor_extern.h"

#define ALIGNED16(a, b) (((((uintptr_t)(a)) | ((uintptr_t)(b))) & 0xf) == 0)
#define ALIGNED16_3(a, b, c) (((((uintptr_t)(a)) | ((uintptr_t)(b)) | ((uintptr_t)(c))) & 0xf) == 0)

/* 0x00284b1c: the FVF of each actor vertex format */
const uint32_t bactor_vertexFormats[4] = { 0x152, 0x252, 0x352, 0x452 };
/* 0x00284b2c: the FVF of each soft-skin vertex format */
const uint32_t bactor_skinVertexFormats[4] = { 0x15c, 0x25c, 0x35c, 0x45c };
/* 0x00284b3c: texture address mode by wrap flag (bit 0: U, bit 1: V); 3 clamp, 1 wrap */
static const int s_addressModes[4] = { 3, 1, 1, 1 };

/* 0x002bb238: number of meshes drawn (64-bit) */
static uint64_t s_meshesDrawn;

/* stencil shadow construction state (0x30 bytes, on the stack of baCreateShadowVolume) */
struct BShadowBuild
{
	float light[4]; /* 0x00 position, or direction when directional */
	float extrude; /* 0x10 */
	int numVerts; /* 0x14 */
	char *start; /* 0x18 */
	char *write; /* 0x1c */
	int directional; /* 0x20 */
	float offset[3]; /* 0x24 -direction * extrude */
};

/* 0x00038670  BACTOR/pcxbActor.cpp (file inferred)
fixes up a soft-skinned actor: colour format, textures, the "all clamped"
flag, and copies its vertices into a vertex buffer */
void bFixupSoftSkin(struct BActor *actor)
{
	uint32_t *colour;
	uint32_t c;
	struct BSkinPrim *prim;
	int stride;
	int i;
	int n;
	void *vb;
	struct BPrimGroup *group;

	actor->skin.verts = (char *)actor->skin.verts + (intptr_t)actor;
	actor->skin.prims = (struct BSkinPrim *)((char *)actor->skin.prims + (intptr_t)actor);
	actor->skin.groups = (struct BPrimGroup *)((char *)actor->skin.groups + (intptr_t)actor);
	stride = ba_bdVertexSize(bactor_skinVertexFormats[actor->vertexFormat]);
	colour = (uint32_t *)((char *)actor->skin.verts + 0x2c);
	for (i = 0; i < (int)actor->skin.numVerts; i++)
	{
		c = *colour;
		*colour = DAT_002bce18[(c >> 16) & 0xff] | DAT_002bc490[(c >> 8) & 0xff] | DAT_002bc898[c >> 24] |
			DAT_002bd640[c & 0xff];
		colour = (uint32_t *)((char *)colour + stride);
	}
	prim = actor->skin.prims;
	actor->flags |= 0x10;
	actor->skin.flags = 1;
	for (i = 0; i < (int)actor->skin.numGroups; i++)
	{
		group = &actor->skin.groups[i];
		group->texture0 = ba_bkFindTexture(0, (int)(intptr_t)group->texture0, actor->handle, actor->package);
		actor->skin.groups[i].texture1 =
			ba_bkFindTexture(0, (int)(intptr_t)actor->skin.groups[i].texture1, actor->handle, actor->package);
		if (actor->skin.groups[i].texture1 != NULL)
			actor->flags &= ~0x10;
		if (actor->skin.flags & 1)
		{
			for (n = actor->skin.groups[i].numPrims; n != 0; n--)
			{
				if ((prim->flags & 3) != 0)
				{
					actor->skin.flags &= ~1;
					break;
				}
				prim++;
			}
		}
	}
	if (ba_bdCreateVertexBuffer(&vb, actor->skin.numVerts, bactor_skinVertexFormats[actor->vertexFormat], 0, 1) == 0)
	{
		ba_bkPrintf("bFixupSoftSkin: *** ERROR  Could not create vertex buffer for soft skin vertices ***\n");
		return;
	}
	memcpy(ba_bdLockVertexBuffer(vb, 4), actor->skin.verts, (unsigned int)actor->skin.numVerts * stride);
	ba_bdUnlockVertexBuffer(vb);
	actor->skin.verts = vb;
}

/* 0x00038830  BACTOR/pcxbActor.cpp (file inferred)
out = transpose(b * a), the first rows rows (3 or 4) */
void bMatMulTranspose(float *out, const float *a, const float *b, int rows)
{
	int i;
	int j;

	for (i = 0; i < 3; i++)
		for (j = 0; j < 4; j++)
			out[i * 4 + j] = b[j * 4 + 0] * a[0 * 4 + i] + b[j * 4 + 1] * a[1 * 4 + i] +
				b[j * 4 + 2] * a[2 * 4 + i] + b[j * 4 + 3] * a[3 * 4 + i];
	if (rows == 4)
	{
		for (j = 0; j < 4; j++)
			out[12 + j] = b[j * 4 + 0] * a[3] + b[j * 4 + 1] * a[7] + b[j * 4 + 2] * a[11] + b[j * 4 + 3] * a[15];
	}
}

/* 0x00038a60  BACTOR/pcxbActor.cpp
draw callback: no textures */
void bDrawGroupsUntextured(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag)
{
	int n;

	(void)flag;
	ba_bdClearTexture(0);
	ba_bdClearTexture(1);
	for (; numGroups != 0; numGroups--)
	{
		for (n = groups->numPrims; n != 0; n--)
		{
			D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prims->type, vertex, prims->numVerts);
			vertex += prims->numVerts;
			prims++;
		}
		groups++;
	}
}

/* sets texture 0 of a group; with flag, a missing texture clears the stage and
a texture's colour key mode (byte 0x3c) is applied */
static void bSetGroupTexture0(struct BPrimGroup *group, int flag)
{
	if (flag == 0)
	{
		ba_bdSetTexture(0, group->texture0);
		return;
	}
	if (group->texture0 == NULL)
	{
		ba_bdClearTexture(0);
	}
	else
	{
		ba_bdSetTexture(0, group->texture0);
		if (*((signed char *)group->texture0 + 0x3c) != -1)
			ba_bdSetState(10, (uint32_t)(int)*((signed char *)group->texture0 + 0x3c), 0);
	}
}

/* 0x00038ad0  BACTOR/pcxbActor.cpp
draw callback: texture 0 */
void bDrawGroupsTex0(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag)
{
	int n;

	ba_bdClearTexture(1);
	for (; numGroups != 0; numGroups--)
	{
		bSetGroupTexture0(groups, flag);
		for (n = groups->numPrims; n != 0; n--)
		{
			D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prims->type, vertex, prims->numVerts);
			vertex += prims->numVerts;
			prims++;
		}
		groups++;
	}
}

/* 0x00038bd0  BACTOR/pcxbActor.cpp
draw callback: textures 0 and 1 */
void bDrawGroupsTex01(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag)
{
	int n;

	for (; numGroups != 0; numGroups--)
	{
		bSetGroupTexture0(groups, flag);
		ba_bdSetTexture(1, groups->texture1);
		for (n = groups->numPrims; n != 0; n--)
		{
			D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prims->type, vertex, prims->numVerts);
			vertex += prims->numVerts;
			prims++;
		}
		groups++;
	}
}

/* 0x00038ce0  BACTOR/pcxbActor.cpp (file inferred)
loads c92-c95 with transpose(world * view * projection) of a node.
Original register arguments: edi inst, eax node. */
void bSetNodeTransformConstants(struct BActorInstance *inst, struct BActorNode *node)
{
	float world[16];
	float constants[16];

	if (DAT_002be5c8 == 0)
	{
		if (ALIGNED16_3(PTR_DAT_00284c70, DAT_002be3b0, DAT_002be370))
			ba_MatMulAligned(DAT_002be3b0, PTR_DAT_00284c70, DAT_002be370);
		else
			ba_MatMul(DAT_002be3b0, PTR_DAT_00284c70, DAT_002be370);
		DAT_002be5c8 = 1;
	}
	if (ALIGNED16_3(inst, node, world))
		ba_MatMulAligned(world, inst->matrix, node->world);
	else
		ba_MatMul(world, inst->matrix, node->world);
	bMatMulTranspose(constants, DAT_002be3b0, world, 4);
	D3DDevice_SetVertexShaderConstant(0x5c, constants, 4);
}

/* 0x00038d90  BACTOR/pcxbActor.cpp
finds the silhouette edges of the meshes of a node list for a light and
counts the shadow volume vertices */
void bFindSilhouettes(struct BActorNode *first, struct BActorInstance *inst, struct BShadowBuild *build)
{
	struct BActorNode *node = first;
	struct BMesh *mesh;
	float world[16];
	float inverse[16];
	float light[4];
	float *plane;
	float *vertex;
	uint16_t *edge;
	int n;
	int i;

	do
	{
		if ((node->flags & BNODEF_HIDDEN) == 0 && node->def->type == BNODE_MESH)
		{
			if (ALIGNED16_3(node, inst, world))
				ba_MatMulAligned(world, inst->matrix, node->world);
			else
				ba_MatMul(world, inst->matrix, node->world);
			ba_MatInverse(inverse, world);
			if (build->directional != 0)
			{
				light[0] = inverse[0] * build->light[0] + inverse[4] * build->light[1] + inverse[8] * build->light[2];
				light[1] = inverse[1] * build->light[0] + inverse[5] * build->light[1] + inverse[9] * build->light[2];
				light[2] = inverse[2] * build->light[0] + inverse[6] * build->light[1] + inverse[10] * build->light[2];
			}
			else
			{
				ba_MatTransform(light, inverse, build->light);
			}
			mesh = &node->def->data.mesh;
			plane = mesh->shadowPlanes;
			if (build->directional != 0)
			{
				for (n = mesh->numFaces; n != 0; n--)
				{
					((int *)plane)[5] = 0.0f <= light[0] * plane[0] + light[1] * plane[1] + light[2] * plane[2];
					plane += 6;
				}
			}
			else
			{
				for (n = mesh->numFaces; n != 0; n--)
				{
					vertex = mesh->shadowVerts + *(uint16_t *)&plane[3] * 3;
					((int *)plane)[5] = 0.0f <= (light[0] - vertex[0]) * plane[0] + (light[1] - vertex[1]) * plane[1] +
						(light[2] - vertex[2]) * plane[2];
					plane += 6;
				}
			}
			edge = mesh->shadowEdges;
			plane = mesh->shadowPlanes;
			mesh->edgeFlags = ba_bkAlloc(mesh->numEdges, "C:\\Babel\\Xbox\\Src\\BACTOR\\pcxbActor.cpp", 0x449, 0x2002);
			for (i = 0; i < mesh->numEdges; i++, edge += 4)
			{
				int facing0 = ((int *)plane)[edge[2] * 6 + 5];
				int facing1 = ((int *)plane)[edge[3] * 6 + 5];

				if (facing0 == facing1)
				{
					mesh->edgeFlags[i] = 0;
				}
				else
				{
					mesh->edgeFlags[i] = (uint8_t)(facing0 + 1);
					build->numVerts += 4;
				}
			}
		}
		if (node->child != NULL)
			bFindSilhouettes(node->child, inst, build);
		node = node->next;
	} while (node != first);
}

/* 0x00039000  BACTOR/pcxbActor.cpp (file inferred)
writes the shadow volume quads of the silhouette edges found by
bFindSilhouettes (FVF 0x152 vertices, 0x24 bytes) */
void bBuildShadowQuads(struct BActorNode *first, struct BActorInstance *inst, struct BShadowBuild *build)
{
	struct BActorNode *node = first;
	struct BMesh *mesh;
	float m[16];
	uint16_t *edge;
	uint16_t a;
	uint16_t b;
	float *v;
	float x, y, z;
	float dx, dy, dz;
	float scale;
	int i;
	int k;

	do
	{
		if ((node->flags & BNODEF_HIDDEN) == 0 && node->def->type == BNODE_MESH)
		{
			if (ALIGNED16_3(node, inst, m))
				ba_MatMulAligned(m, inst->matrix, node->world);
			else
				ba_MatMul(m, inst->matrix, node->world);
			mesh = &node->def->data.mesh;
			edge = mesh->shadowEdges;
			for (i = 0; i < mesh->numEdges; i++, edge += 4)
			{
				if (mesh->edgeFlags[i] == 0)
					continue;
				if (mesh->edgeFlags[i] == 2)
				{
					a = edge[0];
					b = edge[1];
				}
				else
				{
					a = edge[1];
					b = edge[0];
				}
				/* the quad: a, a extruded, b extruded, b (rotation only, as in the original) */
				for (k = 0; k < 2; k++)
				{
					v = mesh->shadowVerts + (k == 0 ? a : b) * 3;
					x = m[4] * v[1] + m[8] * v[2] + m[0] * v[0];
					y = m[5] * v[1] + m[9] * v[2] + m[1] * v[0];
					z = m[6] * v[1] + m[10] * v[2] + m[2] * v[0];
					if (k == 1)
						build->write += 0x48;
					((float *)build->write)[0] = x;
					((float *)build->write)[1] = y;
					((float *)build->write)[2] = z;
					((uint32_t *)build->write)[6] = 0x50000000;
					build->write += (k == 0) ? 0x24 : -0x24;
					if (build->directional == 0)
					{
						dx = x - build->light[0];
						dy = y - build->light[1];
						dz = z - build->light[2];
						scale = ba_FastSqrt(dx * dx + dy * dy + dz * dz);
						if (scale != 0.0f)
							scale = 1.0f / scale * build->extrude;
						((float *)build->write)[0] = dx * scale + x;
						((float *)build->write)[1] = dy * scale + y;
						((float *)build->write)[2] = scale * dz + z;
					}
					else
					{
						((float *)build->write)[0] = x + build->offset[0];
						((float *)build->write)[1] = y + build->offset[1];
						((float *)build->write)[2] = z + build->offset[2];
					}
					((uint32_t *)build->write)[6] = 0x50000000;
					if (k == 1)
						build->write += 0x48;
				}
			}
			ba_bkFree(mesh->edgeFlags);
			mesh->edgeFlags = NULL;
		}
		if (node->child != NULL)
			bBuildShadowQuads(node->child, inst, build);
		node = node->next;
	} while (node != first);
}

/* 0x000393b0  BACTOR/pcxbActor.cpp (file inferred) */
void baDeleteShadowVolume(struct BShadowVolume *shadow)
{
	if (shadow->vertexBuffer != NULL)
	{
		ba_bdReleaseVertexBuffer(shadow->vertexBuffer);
		shadow->vertexBuffer = NULL;
	}
	if (shadow->allocated != 0)
		ba_bkFree(shadow);
}

/* 0x000393e0  BACTOR/pcxbActor.cpp (file inferred)
computes the world matrices of a node list (and the bone matrices) from
the parent's matrix and scale */
void bUpdateNodeMatrices(struct BActorNode *first, const float *parentMatrix, const float *parentScale,
	struct BActorInstance *inst)
{
	struct BActorNode *node = first;
	struct BActorNodeDef *def;
	float rotation[16];
	float local[16];
	float compose[16];
	float px, py, pz;
	uintptr_t alignment;

	do
	{
		ba_MatFromQuat(rotation, node->rot);
		if (ALIGNED16_3(node, local, rotation))
			ba_MatMulAligned(local, rotation, node->local);
		else
			ba_MatMul(local, rotation, node->local);
		/* the translation is scaled by the parent's scale */
		px = node->pos[0] * parentScale[0];
		py = node->pos[1] * parentScale[1];
		pz = node->pos[2] * parentScale[2];
		ba_MatCompose(compose, px, py, pz, local, node->scale[0], node->scale[1], node->scale[2]);
		alignment = (uintptr_t)node | (uintptr_t)parentMatrix;
		if (((alignment | (uintptr_t)compose) & 0xf) == 0)
			ba_MatMulAligned(node->world, parentMatrix, compose);
		else
			ba_MatMul(node->world, parentMatrix, compose);
		def = node->def;
		if (def->type == BNODE_BONE)
			bMatMulTranspose(inst->bones + BNODEDEF_BONE_INDEX(def) * 12, node->world, def->data.boneMatrix, 3);
		if (node->child != NULL)
		{
			if (node == inst->motionNode)
			{
				memcpy(node->childBase, bactor_identity, sizeof(node->childBase));
			}
			else
			{
				/* the children inherit rotation and translation, not scale */
				local[12] = px;
				local[13] = py;
				local[14] = pz;
				if (((alignment | (uintptr_t)local) & 0xf) == 0)
					ba_MatMulAligned(node->childBase, parentMatrix, local);
				else
					ba_MatMul(node->childBase, parentMatrix, local);
			}
			bUpdateNodeMatrices(node->child, node->childBase, node->scale, inst);
		}
		node = node->next;
	} while (node != first);
}

/* 0x000395b0  BACTOR/pcxbActor.cpp (file inferred) */
void bReleaseSoftSkin(struct BActor *actor)
{
	if (actor->skin.verts != NULL)
		ba_bdReleaseVertexBuffer(actor->skin.verts);
}

/* sets the texture address modes of stage 0 from a primitive's wrap flags,
when they change */
#define SET_WRAP(flags, lastU, lastV) \
	do \
	{ \
		int mode_ = s_addressModes[(flags) & 1]; \
		if (mode_ != (lastU)) \
		{ \
			D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSU, mode_); \
			(lastU) = mode_; \
		} \
		mode_ = s_addressModes[(flags) & 2]; \
		if (mode_ != (lastV)) \
		{ \
			D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSV, mode_); \
			(lastV) = mode_; \
		} \
	} while (0)

/* 0x000395d0  BACTOR/pcxbActor.cpp
draw callback: texture 0, per-primitive wrap modes */
void bDrawGroupsTex0Wrap(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag)
{
	int lastU = 0x7fffffff;
	int lastV = 0x7fffffff;
	int n;

	ba_bdClearTexture(1);
	for (; numGroups != 0; numGroups--)
	{
		n = groups->numPrims;
		if (groups->texture0 == NULL)
		{
			ba_bdClearTexture(0);
			for (; n != 0; n--)
			{
				D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prims->type, vertex, prims->numVerts);
				vertex += prims->numVerts;
				prims++;
			}
		}
		else
		{
			ba_bdSetTexture(0, groups->texture0);
			if (flag != 0 && *((signed char *)groups->texture0 + 0x3c) != -1)
				ba_bdSetState(10, (uint32_t)(int)*((signed char *)groups->texture0 + 0x3c), 0);
			if ((groups->flags & 1) == 0)
			{
				for (; n != 0; n--)
				{
					SET_WRAP(prims->flags, lastU, lastV);
					D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prims->type, vertex, prims->numVerts);
					vertex += prims->numVerts;
					prims++;
				}
			}
			else
			{
				if (lastU != 3)
				{
					lastU = 3;
					D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSU, 3);
				}
				if (lastV != 3)
				{
					lastV = 3;
					D3DDevice_SetTextureState_Deferred(0, D3DTSS_ADDRESSV, 3);
				}
				for (; n != 0; n--)
				{
					D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prims->type, vertex, prims->numVerts);
					vertex += prims->numVerts;
					prims++;
				}
			}
		}
		groups++;
	}
}

/* 0x000398e0  BACTOR/pcxbActor.cpp
draw callback: textures 0 and 1, per-primitive wrap modes */
void bDrawGroupsTex01Wrap(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag)
{
	int lastU = 0x7fffffff;
	int lastV = 0x7fffffff;
	int n;

	for (; numGroups != 0; numGroups--)
	{
		bSetGroupTexture0(groups, flag);
		ba_bdSetTexture(1, groups->texture1);
		for (n = groups->numPrims; n != 0; n--)
		{
			SET_WRAP(prims->flags, lastU, lastV);
			D3DDevice_DrawVertices((D3DPRIMITIVETYPE)prims->type, vertex, prims->numVerts);
			vertex += prims->numVerts;
			prims++;
		}
		groups++;
	}
}

typedef void (*BDrawGroupsFn)(int numGroups, struct BPrimGroup *groups, struct BPrim *prims, int vertex, int flag);

/* 0x00284b4c: draw callbacks by (texture 1 count * 2 + wrap) */
static const BDrawGroupsFn s_drawGroups[6] = {
	bDrawGroupsUntextured,
	bDrawGroupsUntextured,
	bDrawGroupsTex0,
	bDrawGroupsTex0Wrap,
	bDrawGroupsTex01,
	bDrawGroupsTex01Wrap,
};

/* 0x00039ac0  BACTOR/pcxbActor.cpp
draws a mesh node; passes: 1 opaque, 2 translucent, 4 additive */
void bDrawMesh(struct BMesh *mesh, struct BActorInstance *inst, uint32_t passes, struct BActorNode *node)
{
	BDrawGroupsFn draw;
	float world[16];
	int useShaders;
	int numPasses;
	int pass;
	int textures;
	int wrap;
	uint32_t bit;

	if (inst->passMode[0] == 1 && inst->passMode[1] == 0 && inst->passMode[2] == 0)
	{
		useShaders = 0;
		if (ALIGNED16_3(inst, node, world))
			ba_MatMulAligned(world, inst->matrix, node->world);
		else
			ba_MatMul(world, inst->matrix, node->world);
		ba_bdSetWorldMatrix(world);
		numPasses = 1;
	}
	else
	{
		useShaders = 1;
		bSetNodeTransformConstants(inst, node);
		numPasses = 3;
	}
	for (pass = 0; pass < numPasses; pass++)
	{
		if (useShaders)
		{
			if (inst->passMode[pass] == 0 || bShaderSelect(inst, pass, mesh) == 0)
				continue;
		}
		else
		{
			DAT_002bc400 = bactor_vertexFormats[inst->actor->vertexFormat];
			D3DDevice_SetVertexShader(DAT_002bc400);
			D3DDevice_SetStreamSource(0, mesh->vertexBuffer, ba_bdVertexSize(DAT_002bc400));
			DAT_00284c10 = (struct bdVertexBuffer *)1;
		}
		if (DAT_00284b0c != 0x100)
		{
			/* stage 1 scales by the brightness (texture factor) */
			D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLOROP, 2);
			D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLORARG1, 1);
			D3DDevice_SetTextureState_Deferred(1, D3DTSS_ALPHAOP, 4);
			D3DDevice_SetTextureState_Deferred(1, D3DTSS_ALPHAARG1, 1);
			D3DDevice_SetTextureState_Deferred(1, D3DTSS_ALPHAARG2, 3);
		}
		s_meshesDrawn++;
		if (mesh->flags & 4)
		{
			textures = 0;
		}
		else
		{
			textures = (DAT_002bee34[0] != 0 || DAT_002bee80 != 0) ? 1 : 0;
			if (((mesh->flags & 2) == 0 && DAT_002bee38 != 0) || DAT_002bee84 != 0)
				textures++;
		}
		wrap = ((mesh->flags & 1) != 0 && DAT_002bee4c == 0) ? 1 : 0;
		draw = s_drawGroups[wrap + textures * 2];
		for (bit = 1; (int)bit < 8; bit <<= 1)
		{
			if ((passes & bit) == 0)
				continue;
			if (bit == 1)
			{
				if (mesh->numGroupsPass[0] != 0)
				{
					ba_bdSetState(6, DAT_00284b0c != 0x100, 0);
					draw(mesh->numGroupsPass[0], mesh->groups, mesh->prims, 0, 0);
				}
			}
			else if (bit == 2)
			{
				if (mesh->numGroupsPass[1] != 0)
				{
					ba_bdSetState(6, DAT_00284b0c != 0x100, 0);
					draw(mesh->numGroupsPass[1], mesh->groups + mesh->numGroupsPass[0], mesh->prims + mesh->firstPrim[0],
						mesh->firstVertex[0], 0);
				}
			}
			else if (bit == 4 && mesh->numGroupsPass[2] != 0)
			{
				ba_bdSetState(6, 1, 0);
				ba_bdSetState(5, 0, 0);
				draw(mesh->numGroupsPass[2], mesh->groups + mesh->numGroupsPass[1] + mesh->numGroupsPass[0],
					mesh->prims + mesh->firstPrim[1], mesh->firstVertex[1], 1);
				ba_bdSetState(5, 1, 0);
			}
		}
		if (DAT_00284b0c != 0x100)
		{
			D3DDevice_SetTextureState_Deferred(1, D3DTSS_COLOROP, 1);
			D3DDevice_SetTextureState_Deferred(1, D3DTSS_ALPHAOP, 1);
		}
		if (useShaders)
			bShaderEndPass(inst, pass);
	}
}

/* 0x00039e20  BACTOR/pcxbActor.cpp
builds a stencil shadow volume of an instance for a light (flags bit 0:
directional light) extruded by extrude; allocated when shadow is NULL */
struct BShadowVolume *baCreateShadowVolume(struct BActorInstance *inst, struct BShadowVolume *shadow,
	const float *light, float extrude, uint32_t flags)
{
	struct BShadowBuild build;
	void *data;

	if (shadow == NULL)
	{
		shadow = ba_bkAlloc(sizeof(struct BShadowVolume), "C:\\Babel\\Xbox\\Src\\BACTOR\\pcxbActor.cpp", 0x505, 0x2002);
		if (shadow == NULL)
			return NULL;
		shadow->allocated = 1;
		shadow->vertexBuffer = NULL;
	}
	else
	{
		shadow->allocated = 0;
	}
	shadow->flags = flags;
	shadow->inst = inst;
	build.extrude = extrude;
	build.numVerts = 0;
	build.directional = flags & 1;
	if (build.directional != 0)
	{
		ba_Vec3Normalize(build.light, light);
		build.offset[0] = -(build.light[0] * extrude);
		shadow->offset[0] = build.offset[0];
		build.offset[1] = -(build.light[1] * extrude);
		shadow->offset[1] = build.offset[1];
		build.offset[2] = -(build.light[2] * extrude);
		shadow->offset[2] = build.offset[2];
	}
	else
	{
		build.light[0] = light[0];
		build.light[1] = light[1];
		build.light[2] = light[2];
		build.light[3] = light[3];
		shadow->light[0] = light[0];
		shadow->light[1] = light[1];
		shadow->light[2] = light[2];
		shadow->light[3] = light[3];
	}
	bFindSilhouettes(inst->root, inst, &build);
	if (ba_bdCreateVertexBuffer(&shadow->vertexBuffer, build.numVerts, 0x152, 0, 1) == 0)
	{
		ba_bkPrintf("baCreateShadowVolume: could not create vertex buffer for %d vertices (memory is leaking...)\n",
			build.numVerts);
		return NULL;
	}
	shadow->numVerts = build.numVerts;
	data = ba_bdLockVertexBuffer(shadow->vertexBuffer, 4);
	build.start = data;
	build.write = data;
	bBuildShadowQuads(inst->root, inst, &build);
	ba_bdUnlockVertexBuffer(shadow->vertexBuffer);
	return shadow;
}

/* 0x00039fa0  BACTOR/pcxbActor.cpp (file inferred)
renders a shadow volume into the stencil buffer (z-fail-less: front faces
increment, back faces decrement) */
void baDrawShadowVolume(struct BShadowVolume *shadow)
{
	if (shadow->vertexBuffer == NULL || DAT_002bc424 == 0)
		return;
	ba_bdSetVertexShader(0x152, shadow->vertexBuffer);
	ba_bdSetTexture(0, NULL);
	D3DDevice_SetRenderState_StencilEnable(1);
	D3DDevice_SetRenderState(D3DRS_ZWRITEENABLE, 0);
	D3DDevice_SetRenderState(D3DRS_STENCILFUNC, 0x207);
	D3DDevice_SetRenderState(D3DRS_COLORWRITEENABLE, 0);
	D3DDevice_SetRenderState_CullMode(0x901);
	D3DDevice_SetRenderState(D3DRS_STENCILPASS, 0x8507);
	D3DDevice_DrawVertices(D3DPT_QUADLIST, 0, shadow->numVerts);
	D3DDevice_SetRenderState_CullMode(0x900);
	D3DDevice_SetRenderState(D3DRS_STENCILPASS, 0x8508);
	D3DDevice_DrawVertices(D3DPT_QUADLIST, 0, shadow->numVerts / 4 * 4);
	ba_bdSetState(4, DAT_002be620, 0);
	D3DDevice_SetRenderState(D3DRS_ZWRITEENABLE, 1);
	D3DDevice_SetRenderState(D3DRS_COLORWRITEENABLE, 0x1010101);
}

/* 0x0003a0e0  BACTOR/pcxbActor.cpp (file inferred)
darkens the screen where the stencil buffer is set (colour r, g, b, a) */
void baDrawShadowFill(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
	struct
	{
		float x, y, z, rhw;
		uint32_t colour;
		float u, v;
	} quad[4];
	uint32_t colour;
	int i;

	if (DAT_002bc424 == 0)
		return;
	colour = ((a << 8 | r) << 8 | g) << 8 | b;
	memset(quad, 0, sizeof(quad)); /* the original leaves u and v unset */
	for (i = 0; i < 4; i++)
	{
		quad[i].z = 0.5f;
		quad[i].rhw = 1.0f;
		quad[i].colour = colour;
	}
	quad[1].x = DAT_002be590;
	quad[2].y = DAT_002be594;
	quad[3].x = DAT_002be590;
	quad[3].y = DAT_002be594;
	ba_bdSetTexture(0, NULL);
	D3DDevice_SetRenderState(D3DRS_STENCILFUNC, 0x205);
	D3DDevice_SetRenderState(D3DRS_STENCILREF, 0);
	D3DDevice_SetRenderState_ZEnable(0);
	ba_bdSetVertexShader(0x144, NULL);
	D3DDevice_DrawVerticesUP(D3DPT_TRIANGLESTRIP, 4, quad, 0x1c);
	D3DDevice_SetRenderState_ZEnable(1);
	D3DDevice_SetRenderState_StencilEnable(0);
}

/* 0x0003a220  BACTOR/pcxbActor.cpp (file inferred) */
void bDrawNodes(struct BActorNode *first, struct BActorInstance *inst, int passes)
{
	struct BActorNode *node = first;

	do
	{
		if ((node->flags & BNODEF_HIDDEN) == 0 && node->def->type == BNODE_MESH)
			bDrawMesh(&node->def->data.mesh, inst, passes, node);
		if (node->child != NULL)
			bDrawNodes(node->child, inst, passes);
		node = node->next;
	} while (node != first);
}
