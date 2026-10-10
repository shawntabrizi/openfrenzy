/*
MouseWhack/CBBash_Destructable.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CBBash_Destructable.cpp
(module mousewhack)

Smashable props (the arena's seats, ...). The file name is in the heap/instance tag of the
constructor 0x00181550 (missing from the function table; found through the arena's calls).
0x00180f60, 0x00181040 and 0x00181520 are its methods (gap functions; the source map does
not know this file, so the table lists them without a file).

A prop (0x1d0 bytes, on the list 0x0030ed00 through its node at +0x1b8):
  +0x00 float its alpha (255), +0x04 float the fade speed, +0x08 float frames before the
  fade (100), +0x0c float[3] the half size of its collision box, +0x1c its platform
  (collision, the unbroken model), +0x20 the smashed model, +0x30 the smashed instance
  (BActorInstance; its root node at +0x164), +0xf0 float[4] position, +0x110 its
  rotation (quaternion), +0x1a0 the "smash" animation, +0x1a4 the shadow texture,
  +0x1a8 float[4] position (the shadow's), +0x1b8 the list node, +0x1c0 short,
  +0x1c2 short smash it now, +0x1c4 short state (0 standing, 1 smashed and fading, 2 gone)
*/

#include "mousewhack.h"

#define DE_S(off) MW_AS(self, off)
#define DE_I(off) MW_AI(self, off)
#define DE_F(off) MW_AF(self, off)
#define DE_V(off) MW_AV(self, off)

/* classlib_fx/bdisplay: the colour channel tables (alpha << 24, red, green, blue by 0..255) */
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bd640[256];
extern unsigned int DAT_002bc898[256];

/* 0x00180f60  (gap): a prop's frame: smashed on request (its smash animation, its
collision gone), then waits, then fades out (3 a frame) */
void CBBash_Destructable_Update(void *self)
{
	float value;

	if (DE_S(0x1c4) == 0) {
		if (DE_S(0x1c2) != 0) {
			DE_S(0x1c2) = 0;
			baFlushAnimations((char *)self + 0x30, (struct BActorNode *)MW_AP(self, 0x164), 1, 0);
			baQueueAnimation((struct BActorInstance *)((char *)self + 0x30), (struct BActorNode *)MW_AP(self, 0x164),
				(struct BAnim *)MW_AP(self, 0x1a0), 0x400, -1, 0, NULL, NULL);
			Platform_Delete(MW_AP(self, 0x1c));
			DE_F(0x1c) = 0.0f;
			DE_S(0x1c4) = 1;
		}
	} else if (DE_S(0x1c4) == 1) {
		if (0.0f < DE_F(8)) {
			value = DE_F(8) - DAT_0028fda8;
			DE_F(8) = value;
			if (value <= 0.0f) {
				DE_F(4) = 3.0f;
				DE_F(8) = 0.0f;
			}
		}
		value = DE_F(0) - DAT_0028fda8 * DE_F(4);
		DE_F(0) = value;
		if (value <= 0.0f) {
			DE_S(0x1c4) = 2;
			DE_F(0) = 0.0f;
		}
	}
}

/* the prop's shadow: a quad of its box (2 x the half depth deep), turned by the angle
at 0x002b4214, on the ground at its position (+1), in colour `colour`; the vertices are
the 4 at 0x0030ec70 (FVF 0x152: position, normal, colour, uv; 0x24 bytes each) */
static void de_DrawShadow(void *self, unsigned int colour)
{
	float corner[4][4];
	int i;

	corner[0][0] = DE_F(0xc);
	corner[0][2] = DE_F(0x14) + DE_F(0x14);
	corner[1][0] = -corner[0][0];
	corner[1][2] = corner[0][2];
	corner[2][0] = corner[0][0];
	corner[2][2] = 0.0f;
	corner[3][0] = corner[1][0];
	corner[3][2] = 0.0f;
	for (i = 0; i < 4; i++) {
		corner[i][1] = 0.0f;   /* (not set by the original; the rotation keeps it) */
		corner[i][3] = 0.0f;
		bmVectorRotateY(corner[i], corner[i], MW_F(0x002b4214));
	}
	for (i = 0; i < 4; i++) {
		unsigned int v = 0x0030ec70 + i * 0x24;

		MW_F(v + 0x00) = corner[i][0] + DE_F(0x1a8);
		MW_F(v + 0x04) = DE_F(0x1ac) + 1.0f;
		MW_F(v + 0x08) = corner[i][2] + DE_F(0x1b0);
		MW_U(v + 0x18) = colour;
	}
	MW_F(0x0030ec8c) = 0.0f;
	MW_F(0x0030ec90) = 1.0f;
	MW_F(0x0030ecb0) = 1.0f;
	MW_F(0x0030ecb4) = 1.0f;
	MW_F(0x0030ecd4) = 0.0f;
	MW_F(0x0030ecd8) = 0.0f;
	MW_F(0x0030ecf8) = 1.0f;
	MW_F(0x0030ecfc) = 0.0f;
	bdSetTexture(0, (struct bdTexture *)MW_AP(self, 0x1a4));
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 3, 0);
	bdSetRenderState(5, 0, 0);
	bdSetWorldMatrix(DAT_002855f0);
	bdDrawPrimitiveFVF(4, MW_A(0x0030ec70), 4, 0x152);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* 0x00181040  (gap): draws a prop: standing: only its shadow (the platform draws the
model); smashed: the smash animation at its alpha as brightness and a fading shadow;
gone: deletes the smashed instance */
void CBBash_Destructable_Draw(void *self)
{
	int level;

	switch (DE_S(0x1c4)) {
	case 0:
		if (MW_AP(self, 0x1a4) != NULL)
			de_DrawShadow(self, DAT_002bc898[0x40] | DAT_002bd640[0x60] | DAT_002bc490[0x60] | DAT_002bce18[0x60]);
		break;
	case 1:
		baSetBrightness(MW_FTOL(DE_F(0)));
		baUpdateAnimations((struct BActorInstance *)((char *)self + 0x30), (struct BActorNode *)MW_AP(self, 0x164), 1.0f,
			NULL);
		baPrepActorInstance((struct BActorInstance *)((char *)self + 0x30), 1);
		baDrawActorInstance((struct BActorInstance *)((char *)self + 0x30), 7);
		baSetBrightness(0x100);
		if (MW_AP(self, 0x1a4) != NULL) {
			level = MW_FTOL(DE_F(0) * 0.3846154f);
			de_DrawShadow(self, DAT_002bc898[level] | DAT_002bd640[level] | DAT_002bc490[level] | DAT_002bce18[level]);
		}
		break;
	case 2:
		baDeleteActorInstance((struct BActorInstance *)((char *)self + 0x30));
		break;
	}
}

/* 0x00181520  (gap): destroys a prop: its smashed instance and its platform */
void CBBash_Destructable_Destroy(void *self)
{
	if ((char *)self + 0x30 != NULL)
		baDeleteActorInstance((struct BActorInstance *)((char *)self + 0x30));
	if (MW_AP(self, 0x1c) != NULL) {
		Platform_Delete(MW_AP(self, 0x1c));
		DE_I(0x1c) = 0;
	}
}

/* 0x00181550  CBBash_Destructable.cpp: constructor (missing from the function table):
the unbroken model `model` as a platform at `pos`, turned by rot[1]; the smashed model
`smashed` with its animation `anim`; the shadow texture `alpha` (or none). `flag` is not
used. */
void *FUN_00181550(void *self, float *pos, float *rot, const char *model, const char *smashed, const char *anim,
	const char *alpha, int flag)
{
	float matrix[16];
	unsigned char *platform;
	void *actor;

	(void)flag;
	memset(self, 0, 0x1d0);
	if (alpha == NULL) {
		DE_I(0x1a4) = 0;
	} else {
		MW_AP(self, 0x1a4) = bLoadResource(NULL, alpha, 0, 0xdefa);
		if (MW_AP(self, 0x1a4) == NULL)
			bkPrintf("could not find platform shadow %s \n", alpha);
		else
			bkPrintf("destructable shadow %s found ok\n", alpha);
	}
	MW_AP(self, 0x1c) = Platform_Create(model, pos, 0);
	memcpy(DE_V(0x1a8), pos, 0x10);
	platform = (unsigned char *)MW_AP(self, 0x1c);
	MW_AS(platform, 0x3fa) = 0;
	MW_AS(platform, 0x3da) = 0;
	MW_AS(platform, 0x3f8) = 1;
	MW_AS(platform, 0x3d8) = 1;
	MW_AU(platform, 0x230) = MW_AU(platform, 0x230) | 0x400000;
	platform = (unsigned char *)MW_AP(self, 0x1c);
	DE_F(0xc) = (MW_AF(MW_AP(platform, 0x1a0), 0x18) - MW_AF(MW_AP(platform, 0x1a0), 0x8)) * 0.5f;
	DE_F(0x10) = (MW_AF(MW_AP(platform, 0x1a0), 0x1c) - MW_AF(MW_AP(platform, 0x1a0), 0xc)) * 0.5f;
	DE_F(0x14) = (MW_AF(MW_AP(platform, 0x1a0), 0x20) - MW_AF(MW_AP(platform, 0x1a0), 0x10)) * 0.5f;
	CLList_Insert((int *)DAT_0030ed00, (int *)((char *)self + 0x1b8), 0);
	actor = bLoadResource(NULL, smashed, 1, 0xdefa);
	MW_AP(self, 0x20) = actor;
	baInitActorInstance((struct BActorInstance *)((char *)self + 0x30), (struct BActor *)actor, 2,
		"C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBBash_Destructable.cpp", 0x35);
	memcpy(DE_V(0xf0), pos, 0x10);
	if (rot[1] != 0.0f) {
		bmMatrixRotationY(matrix, rot[1]);
		bmMatrixToQuaternion(DE_V(0x110), matrix);
	}
	MW_AP(self, 0x1a0) = baFindSubObject((struct BActorInstance *)((char *)self + 0x30), anim);
	DE_I(4) = 0;
	DE_S(0x1c4) = 0;
	DE_F(8) = 100.0f;
	DE_F(0) = 255.0f;
	return self;
}
