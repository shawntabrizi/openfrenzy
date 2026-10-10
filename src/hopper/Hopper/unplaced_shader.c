/*
Hopper/unplaced_shader.c  <-  gap functions 0x0014c880-0x0014c970 (no source path in the XBE)
(module hopper)

The input object (0x002b0378, vtable 0x0026f05c): a map with one "anypressed" channel
(input +4, channel +8; resume makes it, rowing_a 0x001af950 deletes the map on suspend).

Two vertex shader fragments (bactor CBShaderFragment, 0x14 bytes): a constant colour
("mov oD0, %c0", vtable 0x0026f060) and a lit colour ("mul oD0, v4, %c0", vtable
0x0026f08c). The constant is the 4 floats at 0x00304548 (HopShader_SetConstant). Their
vtables hold render callbacks that the module table gives to meta_a (0x0014c9a0-0x0014ca30,
src/meta_a/Meta/unplaced_hopper.c); the vtables are defined here, in hopper's .rdata.
*/

#include "hopper.h"

/* 0x0014c880  (gap): makes the "anypressed" input (any button: 0x69, 0xe00c5) */
void HopInput_Resume(void *self)
{
	void *map;
	void *channel;

	map = biCreateMap(8);
	HOP_AP(self, 0x4) = map;
	channel = biCreateChannel((struct biMap *)map, "anypressed", 1, 1, 0, 0, 0);
	HOP_AP(self, 0x8) = channel;
	biBindInput((struct biChannel *)channel, 0x69, 0);
	biBindInput((struct biChannel *)HOP_AP(self, 0x8), 0xe00c5, 0);
}

/* 0x0014c8d0  (gap): scalar deleting destructor */
void *HopInput_Destroy(void *self, byte flags)
{
	*(const void *const **)self = vtable_0026f05c;
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026f05c  the input object's vtable */
const void *const vtable_0026f05c[1] = {
	(const void *)FUN_0014c8d0, /* [0] scalar deleting destructor */
};

/* 0x0014c8f0  (gap): the constant colour shader fragment */
void *HopShaderColour_Ctor(void *self)
{
	bShaderFragmentInit((struct BShaderFragment *)self, 0, 1, "mov oD0, %c0\nmov oT0, v5\n");
	*(const void *const **)self = vtable_0026f060;
	return self;
}

/* 0x0014c910  (gap): shader fragment callback: no texture, render state 13 off */
void HopShader_Begin(void)
{
	bdSetTexture(0, NULL);
	bdSetRenderState(0xd, 0, 0);
}

/* 0x0014c930  (gap): shader fragment callback: render state 13 back (extra 1) */
void HopShader_End(void)
{
	bdSetRenderState(0xd, 0, 1);
}

/* 0x0014c950  (gap): the lit colour shader fragment */
void *HopShaderLit_Ctor(void *self)
{
	bShaderFragmentInit((struct BShaderFragment *)self, 0, 1, "mul oD0, v4, %c0\nmov oT0, v5\n");
	*(const void *const **)self = vtable_0026f08c;
	return self;
}

/* 0x0014c970  (gap): the shader constant c0 (0x00304548) */
void HopShader_SetConstant(float r, float g, float b, float a)
{
	HOP_F(0x00304548) = r;
	HOP_F(0x0030454c) = g;
	HOP_F(0x00304550) = b;
	HOP_F(0x00304554) = a;
}

/* 0x0026f060  the constant colour fragment's vtable */
const void *const vtable_0026f060[4] = {
	(const void *)FUN_0014c9f0, /* [0] set the constants (meta_a: c1 = 16, 16, 64, 64) */
	(const void *)FUN_0014c910, /* [1] before drawing */
	(const void *)FUN_0014c930, /* [2] after drawing */
	(const void *)FUN_0003b310, /* [3] bactor: the number of constants */
};

/* 0x0026f08c  the lit colour fragment's vtable */
const void *const vtable_0026f08c[4] = {
	(const void *)FUN_0014ca30, /* [0] set the constant (meta_a: c = 0x00304548) */
	(const void *)FUN_0014c9a0, /* [1] before drawing (meta_a) */
	(const void *)FUN_0014c9d0, /* [2] after drawing (meta_a) */
	(const void *)FUN_0003b310, /* [3] bactor: the number of constants */
};
