/*
BACTOR.H

Babel BACTOR (C:\Babel\Xbox\Src\BACTOR): actors (.obe models), their node
hierarchy, keyframe animation, lights, cameras, "extras" (particle-like
systems), the shader selector that generates vertex shaders, and "sets"
(grid-partitioned static geometry).

Data layouts are the original 32-bit ones (the guest is ILP32). Offsets come
from the decompiled code; the OF_ASSERT_* lines check them.

Owner: bactor. Other modules may include this header.
*/

#ifndef OPENFRENZY_BACTOR_H
#define OPENFRENZY_BACTOR_H

#include <stdarg.h>

/* ---------- animation key tracks (stored in the .obe file) */

/* one vector key: frame number and three quantised components */
struct BKey3
{
	uint16_t frame; /* 0x00 frame (time >> 10) */
	uint16_t value[3]; /* 0x02 value = value * scale + base */
};
OF_ASSERT_SIZE(struct BKey3, 0x8);

/* position and scale tracks (FUN_0003cae0) */
struct BKeyTrack3
{
	uint16_t unk00;
	uint16_t numKeys; /* 0x02 */
	struct BKey3 *keys; /* 0x04 */
	float base[3]; /* 0x08 */
	float scale[3]; /* 0x14 */
};
OF_ASSERT_SIZE(struct BKeyTrack3, 0x20);

/* one quaternion key: frame number and four quantised components */
struct BKey4
{
	uint16_t frame;
	uint16_t value[4];
};
OF_ASSERT_SIZE(struct BKey4, 0xa);

/* rotation track (FUN_0003cca0) */
struct BKeyTrack4
{
	int unk00;
	int numKeys; /* 0x04 */
	struct BKey4 *keys; /* 0x08 */
	int unk0c;
	float base[4]; /* 0x10 */
	float scale[4]; /* 0x20 */
};
OF_ASSERT_SIZE(struct BKeyTrack4, 0x30);

/* an animation (frame range) of an actor; times are frames << 10 */
struct BAnim
{
	int unk00;
	int startTime; /* 0x04 */
	int endTime; /* 0x08 */
	int speed; /* 0x0c default speed (time units per tick), 0 = 0x400 */
};

/* an animation event of a node (sent with bkSendMessage when the
animation passes its frame) */
struct BAnimEventDef
{
	int frame; /* 0x00 */
	int target; /* 0x04 fixed up by the loader (+ actor string base) */
	char *message; /* 0x08 fixed up by the loader (+ actor string base) */
	int unk0c;
};
OF_ASSERT_SIZE(struct BAnimEventDef, 0x10);

/* ---------- node types (BActorNodeDef.type) */

#define BNODE_BONE 1 /* data: bind matrix at 0x70, bone index at 0xb0 */
#define BNODE_MESH 2 /* data: struct BMesh at 0x70, struct BVertexStreams at 0xc0 */
#define BNODE_SET 4 /* data: 0x70 (unused by BACTOR), struct BVertexStreams at 0x80 */

/* vertex data description for baGetActorVertices (FUN_000366f0) */
struct BVertexFormat
{
	uint8_t numElements; /* 0x00 */
	uint8_t indexed; /* 0x01 0: per vertex, else per index */
	uint16_t stride; /* 0x02 */
	uint8_t elementType[0x18]; /* 0x04 */
	uint8_t elementOffset[0x18]; /* 0x1c */
	uint8_t pad34;
	uint8_t posOffset[3]; /* 0x35 0xff = none */
	uint8_t normalOffset[3]; /* 0x38 */
	uint8_t pad3b[3];
	uint8_t offsets3e[0x15]; /* 0x3e seven xyz offset triples of the indexed form */
	uint8_t pad53;
	/* 0x54: the source data follows */
};
OF_ASSERT_SIZE(struct BVertexFormat, 0x54);

struct BVertexStreams
{
	int numVerts; /* 0x00 */
	int numIndices; /* 0x04 */
	int unk08;
	int unk0c;
	struct BVertexFormat *format[4]; /* 0x10 */
};
OF_ASSERT_SIZE(struct BVertexStreams, 0x20);

/* a group of primitives that share textures */
struct BPrimGroup
{
	int numPrims; /* 0x00 */
	void *texture0; /* 0x04 */
	void *texture1; /* 0x08 */
	int flags; /* 0x0c bit 0: no per-primitive texture wrap */
};
OF_ASSERT_SIZE(struct BPrimGroup, 0x10);

/* a primitive of a mesh node */
struct BPrim
{
	uint8_t type; /* 0x00 D3DPRIMITIVETYPE */
	uint8_t flags; /* 0x01 bit 0: wrap U, bit 1: wrap V */
	uint16_t numVerts; /* 0x02 */
	int unk04;
};
OF_ASSERT_SIZE(struct BPrim, 0x8);

/* a mesh node's data (node definition + 0x70) */
struct BMesh
{
	int unk00;
	void *vertexBuffer; /* 0x04 D3DVertexBuffer, data follows at +0x10 */
	int numGroups; /* 0x08 */
	struct BPrimGroup *groups; /* 0x0c */
	struct BPrim *prims; /* 0x10 */
	int numGroupsPass[3]; /* 0x14 groups of the opaque, translucent and additive passes */
	int firstPrim[2]; /* 0x20 first primitive of the translucent and additive passes */
	int firstVertex[2]; /* 0x28 first vertex of the translucent and additive passes */
	float *shadowVerts; /* 0x30 */
	float *shadowPlanes; /* 0x34 6 floats per face: normal, vertex index, unused, facing */
	uint16_t *shadowEdges; /* 0x38 4 shorts per edge: two vertices, two faces */
	int numFaces; /* 0x3c */
	uint8_t *edgeFlags; /* 0x40 */
	int numEdges; /* 0x44 */
	int unk48;
	uint32_t flags; /* 0x4c */
};
OF_ASSERT_SIZE(struct BMesh, 0x50);

/* ---------- node definition (in the .obe file) */

struct BActorNodeDef
{
	struct BKeyTrack3 posTrack; /* 0x00 */
	struct BKeyTrack3 scaleTrack; /* 0x20 */
	struct BKeyTrack4 rotTrack; /* 0x40 */
	union
	{
		float boneMatrix[16]; /* 0x70 BNODE_BONE */
		struct BMesh mesh; /* 0x70 BNODE_MESH */
		uint8_t raw[0x70];
	} data;
	struct BActorNodeDef *next; /* 0xe0 circular sibling list */
	struct BActorNodeDef *prev; /* 0xe4 */
	struct BActorNodeDef *parent; /* 0xe8 */
	struct BActorNodeDef *child; /* 0xec */
	uint8_t type; /* 0xf0 BNODE_* */
	uint8_t padf1[3];
	uint32_t nameHash; /* 0xf4 */
	struct BAnimEventDef *events; /* 0xf8 */
	int numEvents; /* 0xfc */
};
OF_ASSERT_SIZE(struct BActorNodeDef, 0x100);
OF_ASSERT_OFFSET(struct BActorNodeDef, next, 0xe0);

#define BNODEDEF_BONE_INDEX(def) (*(int *)((char *)(def) + 0xb0))
#define BNODEDEF_STREAMS(def) \
	((def)->type == BNODE_MESH ? (struct BVertexStreams *)((char *)(def) + 0xc0) \
	                           : (struct BVertexStreams *)((char *)(def) + 0x80))

/* ---------- actor (a loaded .obe model) */

struct BActorSubObject
{
	uint32_t nameHash; /* 0x00 */
	int data[7];
};
OF_ASSERT_SIZE(struct BActorSubObject, 0x20);

/* a soft-skinned actor's mesh (actor + 0x20): one vertex buffer for the
whole actor, transformed by the bone matrices in the vertex shader */
struct BSoftSkin
{
	uint16_t numVerts; /* 0x00 */
	uint16_t numGroups; /* 0x02 */
	void *verts; /* 0x04 vertex data in the file, then its vertex buffer */
	struct BPrimGroup *groups; /* 0x08 */
	struct BSkinPrim *prims; /* 0x0c */
	uint8_t numWeights; /* 0x10 bone weights per vertex (0: not skinned) */
	uint8_t pad11[3];
	uint32_t flags; /* 0x14 bit 0: no wrapped primitives */
};
OF_ASSERT_SIZE(struct BSoftSkin, 0x18);

/* a soft-skin primitive (0x38 bytes) */
struct BSkinPrim
{
	uint8_t type; /* 0x00 D3DPRIMITIVETYPE */
	uint8_t flags; /* 0x01 bit 0: wrap U, 1: wrap V, 2: skipped by some passes */
	uint16_t numVerts; /* 0x02 */
	uint16_t unk04;
	uint16_t numPrims; /* 0x06 */
	uint8_t unk08[0x30];
};
OF_ASSERT_SIZE(struct BSkinPrim, 0x38);

struct BActor
{
	uint32_t handle; /* 0x00 package handle; bit 31: owned by the package */
	uint16_t package; /* 0x04 */
	uint16_t pad06;
	uint8_t unk08[0x18];
	struct BSoftSkin skin; /* 0x20 */
	uint8_t unk38[8];
	struct BActorNodeDef *root; /* 0x40 */
	uint32_t flags; /* 0x44 bit 0: soft skinned, bit 4: no texture 1 */
	uint8_t unk48[0xc];
	struct BActorSubObject *subObjects; /* 0x54 */
	int numSubObjects; /* 0x58 */
	uint8_t unk5c[4];
	float bboxMin[3]; /* 0x60 */
	float bboxMax[3]; /* 0x6c */
	uint8_t numBones; /* 0x78 */
	uint8_t vertexFormat; /* 0x79 */
	uint8_t pad7a[2];
	char *strings; /* 0x7c base of event targets and messages */
};
OF_ASSERT_OFFSET(struct BActor, root, 0x40);
OF_ASSERT_OFFSET(struct BActor, bboxMin, 0x60);
OF_ASSERT_OFFSET(struct BActor, strings, 0x7c);

/* ---------- animation slot (queue entry) of a node instance */

#define BANIM_PLAY 1 /* play anim */
#define BANIM_BLEND 2 /* play anim and anim2, blended by blend */
#define BANIM_BLENDIN 3 /* blend from the current pose into anim */
#define BANIM_COPY 4 /* copy the slots of another node */

/* slot flags */
#define BANIMF_LOOP 0x0001
#define BANIMF_CHAIN 0x0002 /* BANIM_BLEND/BLENDIN: continue as BANIM_PLAY when done */
#define BANIMF_CHILD 0x0004 /* queued for a child node: no callback */
#define BANIMF_NEW 0x0008
#define BANIMF_MOTION 0x0010 /* apply to the motion node */

typedef int (*BAnimCallback)(int reason, void *instance, void *node, struct BAnim *anim, void *context);

struct BAnimSlot
{
	uint8_t active; /* 0x00 */
	uint8_t pad01;
	uint16_t flags; /* 0x02 BANIMF_* */
	int mode; /* 0x04 BANIM_* */
	struct BAnim *anim; /* 0x08 */
	struct BAnim *anim2; /* 0x0c */
	float blend; /* 0x10 */
	float blendSpeed; /* 0x14 */
	int time; /* 0x18 */
	int time2; /* 0x1c */
	int speed; /* 0x20 */
	int speed2; /* 0x24 */
	BAnimCallback callback; /* 0x28 */
	void *context; /* 0x2c */
	struct BActorNode *source; /* 0x30 BANIM_COPY */
	uint8_t unk34[0xc];
};
OF_ASSERT_SIZE(struct BAnimSlot, 0x40);

/* an animation event reported by baUpdateAnimations (8 at most per call) */
struct BAnimEvent
{
	int reason; /* 0x00 0 */
	void *instance; /* 0x04 */
	struct BActorNode *node; /* 0x08 */
	struct BAnim *anim; /* 0x0c */
	void *context; /* 0x10 */
	BAnimCallback callback; /* 0x14 */
	int unk18[2];
};
OF_ASSERT_SIZE(struct BAnimEvent, 0x20);

/* ---------- node instance */

/* node flags (BActorNode.flags) */
#define BNODEF_POS 0x0001 /* has a position track */
#define BNODEF_ROT 0x0002
#define BNODEF_SCALE 0x0004
#define BNODEF_HIDDEN 0x0010
#define BNODEF_NOANIM 0x0020
#define BNODEF_POS_SET 0x0040
#define BNODEF_ROT_SET 0x0080
#define BNODEF_SCALE_SET 0x0100

struct BActorNode
{
	struct BKeyTrack3 *posTrack; /* 0x000 */
	struct BKeyTrack3 *scaleTrack; /* 0x004 */
	struct BKeyTrack4 *rotTrack; /* 0x008 */
	struct BAnimSlot *slots; /* 0x00c */
	float pos[4]; /* 0x010 */
	float scale[4]; /* 0x020 */
	float rot[4]; /* 0x030 quaternion */
	float prevPos[4]; /* 0x040 */
	float prevScale[4]; /* 0x050 */
	float prevRot[4]; /* 0x060 */
	float blend; /* 0x070 */
	int time; /* 0x074 */
	uint8_t curSlot; /* 0x078 */
	uint8_t numSlots; /* 0x079 */
	uint16_t flags; /* 0x07a BNODEF_* */
	struct BActorNodeDef *def; /* 0x07c */
	float world[16]; /* 0x080 */
	float childBase[16]; /* 0x0c0 */
	float local[16]; /* 0x100 extra local rotation */
	struct BActorNode *next; /* 0x140 circular sibling list */
	struct BActorNode *prev; /* 0x144 */
	struct BActorNode *parent; /* 0x148 */
	struct BActorNode *child; /* 0x14c */
	struct BAnimEventDef *events; /* 0x150 */
	int numEvents; /* 0x154 */
	int unk158[2];
	/* 0x160: numSlots animation slots follow */
};
OF_ASSERT_SIZE(struct BActorNode, 0x160);
OF_ASSERT_OFFSET(struct BActorNode, flags, 0x7a);
OF_ASSERT_OFFSET(struct BActorNode, world, 0x80);
OF_ASSERT_OFFSET(struct BActorNode, next, 0x140);

/* ---------- actor instance */

/* render pass modes (BActorInstance.passMode) */
#define BPASS_OFF 0
#define BPASS_NORMAL 1

struct BActorInstance
{
	float matrix[16]; /* 0x000 world matrix */
	float rotScale[16]; /* 0x040 */
	float inverse[16]; /* 0x080 inverse of the world matrix (flag 2) */
	float pos[4]; /* 0x0c0 */
	float scale[4]; /* 0x0d0 */
	float rot[4]; /* 0x0e0 quaternion */
	float rotMatrix[16]; /* 0x0f0 */
	struct BActor *actor; /* 0x130 */
	struct BActorNode *root; /* 0x134 */
	uint16_t flags; /* 0x138 bit 0: allocated, bit 1: inverse valid */
	int16_t visible; /* 0x13a */
	struct BActorSubObject *subObjects; /* 0x13c */
	int passMode[3]; /* 0x140 */
	int numSubObjects; /* 0x14c */
	int passParam[3]; /* 0x150 */
	struct BActorNode *motionNode; /* 0x15c node named "motion" */
	float *bones; /* 0x160 3x4 matrices of the bones */
	int unk164[3];
	/* 0x170: the bone matrices of an allocated instance follow */
};
OF_ASSERT_SIZE(struct BActorInstance, 0x170);
OF_ASSERT_OFFSET(struct BActorInstance, actor, 0x130);
OF_ASSERT_OFFSET(struct BActorInstance, passMode, 0x140);
OF_ASSERT_OFFSET(struct BActorInstance, motionNode, 0x15c);

/* ---------- stencil shadow volumes (pcxbActor.c) */

struct BShadowVolume
{
	float light[4]; /* 0x00 point light position */
	void *vertexBuffer; /* 0x10 */
	int allocated; /* 0x14 */
	int numVerts; /* 0x18 */
	uint32_t flags; /* 0x1c bit 0: directional */
	struct BActorInstance *inst; /* 0x20 */
	float offset[3]; /* 0x24 directional: -direction * extrude */
};
OF_ASSERT_SIZE(struct BShadowVolume, 0x30);

/* ---------- lights */

#define BLIGHT_DIRECTIONAL 1
#define BLIGHT_POINT 2
#define BLIGHT_SPOT 3

/* light flags */
#define BLIGHTF_ENABLED 1
#define BLIGHTF_ALLOCATED 2
#define BLIGHTF_SPECULAR 4

struct BLight
{
	int type; /* 0x00 BLIGHT_* */
	struct BLight *next; /* 0x04 */
	struct BLight *prev; /* 0x08 */
	int unk0c;
	uint32_t flags; /* 0x10 BLIGHTF_* */
	int color[3]; /* 0x14 0..255 (doubled by baSetLightColour) */
	int index; /* 0x20 D3D light index */
	int unk24[3];
	float pos[4]; /* 0x30 position or direction */
	float dir[4]; /* 0x40 spot: direction; point: [0] range */
	float dirNorm[4]; /* 0x50 */
	float range; /* 0x60 spot */
	float angle; /* 0x64 spot */
	float cosHalfAngle; /* 0x68 spot */
	int unk6c;
};
OF_ASSERT_SIZE(struct BLight, 0x70);

/* ---------- cameras */

/* camera point types (the variadic arguments each takes: see bReadCameraPoint) */
#define BCAM_POS 0 /* position and extra value: 4 doubles */
#define BCAM_POSPTR 1 /* a float[3] pointer and the extra value (a double) */
#define BCAM_NODE 2 /* a matrix (instance) pointer, a node pointer (or NULL), offset x y z and extra (4 doubles) */
#define BCAM_QUAT 3 /* target only: a rotation quaternion (4 doubles) */
#define BCAM_EULER 4 /* target only: 3 angles (doubles) and a rotation order (int) */
#define BCAM_CALLBACK 5 /* a function (out, extra, value) and a value */

struct BCameraPoint
{
	float v[6]; /* 0x00 values or pointers, depending on type */
	int unk18;
	int type; /* 0x1c BCAM_* */
};
OF_ASSERT_SIZE(struct BCameraPoint, 0x20);

struct BCamera
{
	struct BCameraPoint eye; /* 0x00 */
	struct BCameraPoint target; /* 0x20 */
	uint32_t flags; /* 0x40 bit 0: allocated */
	int unk44;
	float fov; /* 0x48 */
	float aspect; /* 0x4c */
	float nearZ; /* 0x50 */
	float farZ; /* 0x54 */
	struct BCamera *next; /* 0x58 */
	struct BCamera *prev; /* 0x5c */
};
OF_ASSERT_SIZE(struct BCamera, 0x60);

/* ---------- extras (systems of small fixed-size items) */

typedef int (*BExtraCallback)();

struct BExtraSystem
{
	uint32_t nameHash; /* 0x00 */
	uint32_t flags; /* 0x04 bit 0: recycle the oldest, 1: no matrix, 2: matrix dirty */
	struct BExtraSystem *next; /* 0x08 */
	struct BExtraSystem *prev; /* 0x0c */
	int maxItems; /* 0x10 */
	int itemSize; /* 0x14 */
	int numItems; /* 0x18 */
	int nextFree; /* 0x1c */
	BExtraCallback initItem; /* 0x20 (system, item, args): args is a va_list of baAddExtraItem's arguments */
	BExtraCallback update; /* 0x24 (system, numItems, items, arg) */
	BExtraCallback deleteItem; /* 0x28 (system, item): 0 = keep */
	BExtraCallback draw; /* 0x2c (system, numItems, items, arg) */
	BExtraCallback systemCallback; /* 0x30 (system, 0 = created / 1 = deleted) */
	int *used; /* 0x34 one flag per item slot */
	void **items; /* 0x38 the live items, in order */
	char *itemData; /* 0x3c */
	void *userData; /* 0x40 */
	int userDataSize; /* 0x44 */
	int maxUsed; /* 0x48 */
	int unk4c;
	float pos[4]; /* 0x50 */
	float rot[4]; /* 0x60 quaternion */
	float matrix[16]; /* 0x70 */
	float *parentMatrix; /* 0xb0 */
	struct BActorNode *parentNode; /* 0xb4 */
	int unkb8[2];
	/* 0xc0: user data, the flags, the item list and the items follow */
};
OF_ASSERT_SIZE(struct BExtraSystem, 0xc0);

/* ---------- shader selector */

/* a vertex shader code fragment (C++ class CBShaderFragment and subclasses) */
struct BShaderFragment
{
	const void *const *vtable; /* 0x00 */
	int numLines; /* 0x04 */
	int numParams; /* 0x08 */
	int numConstants; /* 0x0c */
	const char *code; /* 0x10 */
	int extra; /* 0x14 one subclass has one more field */
};

/* the fragment list a shader is built from */
struct BShaderBuild
{
	int count; /* 0x00 */
	struct BShaderFragment *fragments[0x20]; /* 0x04 */
	int constantBase[0x20]; /* 0x84 first constant register of each fragment */
};

/* the state a vertex shader is chosen by (0x118 bytes) */
struct BShaderMode
{
	int numBones; /* 0x00 */
	int pass; /* 0x04 */
	uint32_t flags; /* 0x08 */
	int passParam; /* 0x0c */
	uint32_t handle; /* 0x10 vertex shader */
	struct BShaderBuild build; /* 0x14 */
};
OF_ASSERT_SIZE(struct BShaderMode, 0x118);

/* a lighting state and its generated shaders (0x24 bytes) */
struct BLightingState
{
	int key[5]; /* 0x00 numLights?, ambient, directional, point, spot counts */
	struct BShaderMode *modes; /* 0x14 */
	int numModes; /* 0x18 */
	int lastUsed; /* 0x1c frame */
	int useCount; /* 0x20 */
};
OF_ASSERT_SIZE(struct BLightingState, 0x24);

/* ---------- pcxbSet: a grid of prebuilt geometry */

struct BSet
{
	uint32_t handle; /* 0x00 */
	uint16_t package; /* 0x04 */
	uint8_t pad06[0x1a];
	int type; /* 0x20 index into the set handler tables */
	uint8_t unk24[0xc];
	int cellsX; /* 0x30 */
	int cellsY; /* 0x34 */
	int cellsZ; /* 0x38 */
	float bounds[6]; /* 0x3c min x, max x, min y, max y, min z, max z */
	struct BSetCell *cells; /* 0x54 */
	uint32_t flags; /* 0x58 */
	int unk5c;
	void *vertexBuffer; /* 0x60 */
	uint32_t *outcodes; /* 0x64 */
	int numVerts; /* 0x68 */
	int unk6c;
	/* 0x70: the cells (0x40 bytes each), the primitives (0x18 bytes each) and the vertices follow */
};
OF_ASSERT_OFFSET(struct BSet, cells, 0x54);
OF_ASSERT_OFFSET(struct BSet, numVerts, 0x68);

struct BSetPrim
{
	int type; /* 0x00 */
	void *texture0; /* 0x04 */
	int numPrims; /* 0x08 */
	uint16_t numVerts; /* 0x0c */
	uint8_t flags; /* 0x0e */
	uint8_t pad0f;
	void *texture1; /* 0x10 */
	int unk14;
};
OF_ASSERT_SIZE(struct BSetPrim, 0x18);

struct BSetCell
{
	int numPrims; /* 0x00 */
	int unk04;
	struct BSetPrim *prims; /* 0x08 */
	int firstVertex0; /* 0x0c opaque pass */
	int numPrimsPass[3]; /* 0x10 opaque, translucent, additive */
	int firstVertex1; /* 0x1c translucent pass */
	int firstVertex2; /* 0x20 additive pass */
	uint8_t unk24[0x1c];
};
OF_ASSERT_SIZE(struct BSetCell, 0x40);

/* ---------- globals of this module (bss unless noted) */

/* 0x00284b0c: global brightness, 0x100 = full (initialised) */
extern int DAT_00284b0c;
/* 0x00284b10: ambient light colour, 0..255 (initialised to 30, 30, 30) */
extern int DAT_00284b10;
extern int DAT_00284b14;
extern int DAT_00284b18;

/* 0x002bb138, 0x002bb1a8: list heads (next at +4, prev at +8) of the
disabled and enabled lights */
extern struct BLight DAT_002bb138;
extern struct BLight DAT_002bb1a8;
#define g_lightsOff DAT_002bb138
#define g_lightsOn DAT_002bb1a8
/* 0x002bb218: number of enabled lights */
extern int DAT_002bb218;

/* 0x002bc2c0: camera list head (next at 0x58, prev at 0x5c); 0x002bc320: current camera */
extern struct BCamera DAT_002bc2c0;
extern struct BCamera *DAT_002bc320;

/* 0x002bc328: extra system list head (next at 0x08, prev at 0x0c) */
extern struct BExtraSystem DAT_002bc328;

/* 0x00284b1c: the FVF of each actor vertex format (pcxbActor.c) */
extern const uint32_t bactor_vertexFormats[4];

/* ---------- functions of this module (see bactor.decls for the prototypes) */

/* the identity matrix of the program (0x002855f0, initialised data of
another module; BACTOR only reads it) */
extern const float bactor_identity[16];

#endif
