/*
Rowing/unplaced_quads.c  <-  Rowing gap 0x001b10f0-0x001b12c0 (no file tag)

Two functions just before rowing_b's CRowing_Sprites.cpp: the camera-facing quad writer
that rowing_b's particles use (struct RowQuadSprites: +0x00 the vertices per buffer, +0x04
the vertex buffers (NULL-terminated), +0x24 the write pointer, +0x28 the vertices left,
+0x2c the buffer in use, +0x38 the corner offsets x[4], y[4], z[4]) and the sprite
manager's constructor. Each quad is 6 vertices of 0x24 bytes (a degenerate strip join:
the first and last copy their neighbours, colour 0).
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"

/* 0x001b10f0  Rowing (gap)
Adds a quad of SIZE at POS in COLOUR: moves to the next vertex buffer (unlocking the full
one) when fewer than 6 vertices are left, or drops the quad when there is none. */
void FUN_001b10f0(void *self, float *pos, float size, unsigned int colour)
{
	float *v;
	int i;

	if (RA_U32(self, 0x28) < 6) {
		bdVertexBufferUnlock((struct bdVertexBuffer *)RA_PTRF(self, 4 + RA_I32(self, 0x2c) * 4));
		if (RA_PTRF(self, 8 + RA_I32(self, 0x2c) * 4) == NULL)
			return;
		RA_I32(self, 0x2c)++;
		RA_I32(self, 0x28) = RA_I32(self, 0x0);
		RA_PTRF(self, 0x24) = bdVertexBufferLock((struct bdVertexBuffer *)RA_PTRF(self, 4 + RA_I32(self, 0x2c) * 4), 0);
	}
	v = (float *)RA_PTRF(self, 0x24);
	RA_U32(v, 0xcc) = 0;
	RA_U32(v, 0x18) = 0;
	RA_U32(v, 0xa8) = colour;
	RA_U32(v, 0x84) = colour;
	RA_U32(v, 0x60) = colour;
	RA_U32(v, 0x3c) = colour;
	for (i = 0; i < 3; i++) {
		/* vertices 1-4 are the corners; 0 and 5 repeat 1 and 4 */
		v[9 + i] = size * RA_F32(self, 0x38 + i * 0x10) + pos[i];
		v[i] = v[9 + i];
		v[18 + i] = size * RA_F32(self, 0x3c + i * 0x10) + pos[i];
		v[27 + i] = size * RA_F32(self, 0x40 + i * 0x10) + pos[i];
		v[36 + i] = size * RA_F32(self, 0x44 + i * 0x10) + pos[i];
		v[45 + i] = v[36 + i];
	}
	RA_PTRF(self, 0x24) = (char *)v + 0xd8;
	RA_I32(self, 0x28) -= 6;
}

/* 0x001b12a0  Rowing (gap)
The sprite manager's constructor (game +0x88): clears the sprite manager's .bss block
0x0030fb40 (0x3c bytes; the static manager that rowing_b's sprite functions use) and
returns SELF. */
void *FUN_001b12a0(void *self)
{
	memset(RW_ADDR(0x0030fb40), 0, 0x3c);
	return self;
}
