/*
RollerBall/CRollerBall_Particle.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerBall_Particle.cpp (heap tag of 0x0019d150): a
spark particle system (vtable 0x00274e20) used by RollerBall and the Demolition games: each
spark is drawn as a beam from its last to its current position (screen-space vertices,
FVF 0x144) and a glow sprite. Its other virtual functions are COMDAT copies that sit in
other modules (demolition_c 0x00104a30 and 0x00102d60, rowing_a 0x0019d400 and
0x0019d200). The functions other than 0x0019d150 have no tag; placed by address order.

System: +0x0c sparks (0x68 bytes each), +0x10 spark count, +0x14 live sparks, +0x18
beam texture, +0x3c glow texture, +0x50 6 words of parameters (+0x50 byte drawn, +0x51
byte, +0x54 50, +0x58 0.97, +0x5c -0.5, +0x60 0.5, +0x64 1), +0x68 vertex buffer.
Spark: +0x00 alive, +0x08 position (float[4]), +0x18 last position (float[4]), +0x38
width, +0x48 colour at the tail and +0x50 at the head (3 shorts each, 8.7 fixed point).

0x0019d0c0 and 0x0019d110 keep their table prototypes (demolition_b/c call them).
*/

#include "rollerball_b.h"

#define RBPS_SPARKS 0x0c
#define RBPS_COUNT 0x10
#define RBPS_LIVE 0x14
#define RBPS_TEXTURE 0x18
#define RBPS_GLOW 0x3c
#define RBPS_PARAMS 0x50
#define RBPS_VB 0x68
#define RBPS_SPARK_SIZE 0x68

/* a vertex of FVF 0x144: x, y, z, rhw, colour, u, v */
#define RBPS_VERTEX_WORDS 7

/* 0x00274e20  particle system vtable */
const void *const rbb_vtable_00274e20[6] = {
	(const void *)FUN_00104a30, /* +0x00 demolition_c (COMDAT) */
	(const void *)FUN_0019d400, /* +0x04 rowing_a (COMDAT) */
	(const void *)FUN_0019ce60, /* +0x08 draw */
	(const void *)FUN_00102d60, /* +0x0c reset: demolition_c (COMDAT) */
	(const void *)FUN_0019d200, /* +0x10 start a spark: rowing_a (COMDAT) */
	(const void *)FUN_0019d130, /* +0x14 scalar deleting destructor */
};

/* 0x0019ce50  RollerBall/CRollerBall_Particle.cpp (inferred)
Constructor. */
void CRollerBallParticles_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274e20;
}

/* 0x0019ce60  RollerBall/CRollerBall_Particle.cpp (inferred)
Draw the live sparks: their beams in one strip (each beam's 8 vertices with the first and
last repeated to join the strip), then their glows (35 x 35 sprites). */
void CRollerBallParticles_Draw(void *self)
{
	uint32_t *vb;
	uint32_t *out;
	uint32_t *beam;
	int count;
	int i;

	if (RBB_C(self, RBPS_PARAMS) == 0 || RBB_I(self, RBPS_LIVE) <= 0)
		return;
	bdSetWorldIdentity();
	bdSetTexture(0, (struct bdTexture *)RBB_P(self, RBPS_TEXTURE));
	vb = bdVertexBufferLock((struct bdVertexBuffer *)RBB_P(self, RBPS_VB), 4);
	if (vb == NULL)
		return;
	out = vb;
	beam = vb + RBPS_VERTEX_WORDS;
	for (i = 0; i < RBB_I(self, RBPS_COUNT); i++) {
		char *s = (char *)RBB_P(self, RBPS_SPARKS) + i * RBPS_SPARK_SIZE;
		int tail[4];
		int head[4];
		if (RBB_C(s, 0x00) == 0)
			continue;
		tail[0] = RBB_S(s, 0x48) >> 7;
		tail[1] = RBB_S(s, 0x4a) >> 7;
		tail[2] = RBB_S(s, 0x4c) >> 7;
		tail[3] = 0x80;
		head[0] = RBB_S(s, 0x50) >> 7;
		head[1] = RBB_S(s, 0x52) >> 7;
		head[2] = RBB_S(s, 0x54) >> 7;
		head[3] = 0x80;
		RBB_F(s, 0x14) = 1.0f;
		RBB_F(s, 0x24) = 1.0f;
		if (bdDrawBeam3D(RBB_V(s, 0x18), 1.0f, 1.0f, tail, RBB_V(s, 0x08), RBB_F(s, 0x38), RBB_F(s, 0x38),
			head, NULL, (float *)beam, 0.0f) != 0) {
			/* vertex 0 = vertex 1, vertex 9 = vertex 8 */
			memcpy(out, beam, RBPS_VERTEX_WORDS * 4);
			memcpy(beam + 8 * RBPS_VERTEX_WORDS, beam + 7 * RBPS_VERTEX_WORDS, RBPS_VERTEX_WORDS * 4);
			out += 10 * RBPS_VERTEX_WORDS;
			beam += 10 * RBPS_VERTEX_WORDS;
		}
	}
	bdVertexBufferUnlock((struct bdVertexBuffer *)RBB_P(self, RBPS_VB));
	bdSetProjectionMode(1);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	count = (int)(out - vb) / RBPS_VERTEX_WORDS;
	if (count != 0)
		bdDrawVertexBuffer(4, (struct bdVertexBuffer *)RBB_P(self, RBPS_VB), 0, count);
	bdSetProjectionMode(1);
	for (i = 0; i < RBB_I(self, RBPS_COUNT); i++) {
		char *s = (char *)RBB_P(self, RBPS_SPARKS) + i * RBPS_SPARK_SIZE;
		if (RBB_C(s, 0x00) == 0)
			continue;
		RBB_F(s, 0x14) = 1.0f;
		bdDrawSprite3D(RBB_V(s, 0x08), 35.0f, 35.0f, 0.0f, (struct bdTexture *)RBB_P(self, RBPS_GLOW), 0, 0,
			RBB_S(s, 0x50) >> 9, RBB_S(s, 0x52) >> 9, RBB_S(s, 0x54) >> 9, 0x7f, NULL, 0.0f);
	}
	bdPopRenderState(5);
	bdPopRenderState(6);
}

/* 0x0019d0c0  RollerBall/CRollerBall_Particle.cpp (inferred)
Start `n` sparks in free slots (virtual +0x10). */
void FUN_0019d0c0(int *self, int n)
{
	int started = 0;
	int i;

	for (i = 0; i < self[4]; i++) {
		char *s = (char *)(intptr_t)self[3] + i * RBPS_SPARK_SIZE;
		if (*s == 0) {
			RBB_VFN(self, 0x10, void (*)(void *, void *))(self, s);
			started++;
			if (n <= started)
				return;
		}
	}
}

/* 0x0019d110  RollerBall/CRollerBall_Particle.cpp (inferred)
Set the glow texture. */
void FUN_0019d110(int self, undefined4 texture)
{
	RBB_U(self, RBPS_GLOW) = texture;
}

/* 0x0019d120  RollerBall/CRollerBall_Particle.cpp (inferred)
Destructor. */
void CRollerBallParticles_Dtor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274e20;
}

/* 0x0019d130  RollerBall/CRollerBall_Particle.cpp (inferred)
Scalar deleting destructor. */
void *CRollerBallParticles_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274e20;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x0019d150  RollerBall/CRollerBall_Particle.cpp
Set up `count` sparks with `params` (6 words; null: the defaults) and the vertex buffer
(10 vertices a spark), then reset (virtual +0x0c). Returns 0 when the buffer cannot be
made. */
int CRollerBallParticles_Init(void *self, int *params, int count)
{
	RBB_P(self, RBPS_SPARKS) = NULL;
	RBB_I(self, RBPS_LIVE) = 0;
	RBB_P(self, RBPS_TEXTURE) = NULL;
	RBB_P(self, RBPS_GLOW) = NULL;
	if (params == NULL) {
		RBB_B(self, 0x50) = 0;
		RBB_B(self, 0x51) = 1;
		RBB_F(self, 0x5c) = -0.5f;
		RBB_F(self, 0x54) = 50.0f;
		RBB_U(self, 0x58) = 0x3f7851ecu;   /* 0.97 */
		RBB_F(self, 0x60) = 0.5f;
		RBB_F(self, 0x64) = 1.0f;
	} else {
		memcpy(RBB_ADDR(self, RBPS_PARAMS), params, 6 * 4);
	}
	RBB_I(self, RBPS_COUNT) = count;
	RBB_P(self, RBPS_SPARKS) = RBB_NEW(count * RBPS_SPARK_SIZE,
		"C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerBall_Particle.cpp", 0x47);
	if (bdVertexBufferCreate((struct bdVertexBuffer **)RBB_ADDR(self, RBPS_VB), RBB_I(self, RBPS_COUNT) * 10,
		0x144, 0, 1) == 0)
		return 0;
	RBB_VFN(self, 0x0c, void (*)(void *))(self);
	return 1;
}
