/*
MouseWhack/CBBash_Splat.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CBBash_Splat.cpp
(module mousewhack)

The marks and bits left by hits: paint splats, hammer rings, swipes, sprays, crumbs and
flying bug bits. 0x00183ca0-0x00185d70 and 0x001839b0 are tagged with this file (heap and
instance tags); 0x00182c10-0x00183a70 and 0x00185e90-0x00186280 are gap functions that
use the same record pool (file inferred). 0x00182c90 is missing from the function table.

The pool (0x0030eec0, 3000 records of 0x2c0 bytes, 0x203a00 bytes from the heap): a free
list (0x0030eef8) and an active list (0x0030eef4) through +0. A record:
  +0x04 float[4] position (w: 0 when it rests), +0x14 its texture, +0x78 short last frame
  of the texture, +0x7a short the vertex bank / frame drawn, +0x7c float frame speed,
  +0x80 float frame, +0x84 short kind, +0x88 float size, +0x8c float time left,
  +0x90 float alpha (0..255), +0x94 float fade per frame, +0x98 float[3] spin speeds,
  +0xa8 float[4] spin angles, +0xb8 4 vertices of {x, y, z, u, v} (a quad strip),
  +0x108 an actor, +0x110 its instance (BActorInstance; +0x1d0 its position, +0x1e0 its
  scale, +0x1f0 its rotation, +0x244 its root node, +0x280 an animation),
  +0x284 short[3] colour (r, g, b), +0x28c float[4] velocity, +0x29c float[4] the
  velocity of what it rides on, +0x2ac short which shared debris model, +0x2b0 float
  its height above the ground.
Kinds: 0 a splat (texture by player, frames), 1 a crumb (stand), 2 a spray blob,
3 a bug bit, 4 a billboard, 5 a paint spray mark, 6 the spore explosion, 7 a hammer ring,
8 a swipe (an actor), 9 a player's paint drop, 10 a tremor bit, 11 a spit drop,
12 a spore drop.

The quads are drawn from 10 dynamic vertex buffers (0x0030ee48; FVF 0x152, 0x24 bytes:
position, normal, colour, uv) of 3000 vertices (0x0030ee70) with a count each (0x0030ee98).
Shared debris instances (0x170 bytes each): crumbs 0x0030ce80, bug bits 0x0030d5b0,
tremor bits 0x0030da00, spit drops 0x0030dfc0.
*/

#include "mousewhack.h"
#include "classlib_ui.h"   /* struct CLQuadVertex */

#define SP_VB(i) MW_G(0x0030ee48 + (i) * 4, struct bdVertexBuffer *)
#define SP_CAP(i) MW_I(0x0030ee70 + (i) * 4)
#define SP_COUNT(i) MW_I(0x0030ee98 + (i) * 4)
#define SP_POOL MW_G(0x0030eec0, unsigned char *)
#define SP_ACTIVE MW_G(0x0030eef4, unsigned char *)
#define SP_FREE MW_G(0x0030eef8, unsigned char *)
#define SP_NEXT(rec) MW_AT(rec, 0, unsigned char *)

#define RF(rec, off) MW_AF(rec, off)
#define RS(rec, off) MW_AS(rec, off)
#define RI(rec, off) MW_AI(rec, off)

static const char mw_SplatFile[] = "C:\\BlitzParty\\Game\\Src\\MouseWhack\\CBBash_Splat.cpp";

/* classlib_fx/bdisplay: the colour channel tables by 0..255 */
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bd640[256];
extern unsigned int DAT_002bc898[256];
/* charlib: the actor list (a CLList; the platforms' list at +0x18) */
extern unsigned char *DAT_002ea488;

/* a free record moved to the front of the active list, or NULL */
static unsigned char *sp_Take(void)
{
	unsigned char *rec = SP_FREE;

	if (rec == NULL)
		return NULL;
	SP_FREE = SP_NEXT(rec);
	SP_NEXT(rec) = SP_ACTIVE;
	SP_ACTIVE = rec;
	return rec;
}

/* the colour of the 4 channel tables at one level each */
#define SP_COLOUR(a, r, g, b) (DAT_002bc898[a] | DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bce18[r])

/* one vertex (x, y, z, colour, uv when `uv`) at index `n` of the locked buffer */
static void sp_Vertex(unsigned char *buf, int n, const float *xyzuv, unsigned int colour, int uv)
{
	unsigned char *v = buf + n * 0x24;

	MW_AF(v, 0) = xyzuv[0];
	MW_AF(v, 4) = xyzuv[1];
	MW_AF(v, 8) = xyzuv[2];
	MW_AU(v, 0x18) = colour;
	if (uv) {
		MW_AF(v, 0x1c) = xyzuv[3];
		MW_AF(v, 0x20) = xyzuv[4];
	}
}

/* a record's quad as 6 strip vertices in bank `bank`: the first and last repeated (with
`edge` colour; their uv only when `edgeUV`) to join the quads */
static void sp_Strip(unsigned char *buf, int bank, unsigned char *rec, unsigned int edge, unsigned int colour,
	int edgeUV)
{
	int k;

	sp_Vertex(buf, SP_COUNT(bank), MW_AV(rec, 0xb8), edge, edgeUV);
	SP_COUNT(bank) = SP_COUNT(bank) + 1;
	for (k = 0; k < 4; k++) {
		sp_Vertex(buf, SP_COUNT(bank), MW_AV(rec, 0xb8 + k * 0x14), colour, 1);
		SP_COUNT(bank) = SP_COUNT(bank) + 1;
	}
	sp_Vertex(buf, SP_COUNT(bank), MW_AV(rec, 0xf4), edge, edgeUV);
	SP_COUNT(bank) = SP_COUNT(bank) + 1;
}

/* unlocks the 10 banks and draws the filled ones with `texture` (set per bank by
`frames`: the bank is the texture frame) */
static void sp_DrawBanks(struct bdTexture *texture, int frames)
{
	int i;

	for (i = 0; i < 10; i++) {
		if (SP_CAP(i) < SP_COUNT(i))
			bkPrintf("ERROR tried to use more verts than avalable bank %d\n", i);
		bdVertexBufferUnlock(SP_VB(i));
		if (1 < SP_COUNT(i)) {
			if (frames)
				bdTextureSetFrame(texture, i);
			bdSetTexture(0, texture);
			bdDrawVertexBuffer(4, SP_VB(i), 0, SP_COUNT(i));
		}
	}
}

/* 0x00182c10  (gap): empties the pool: every record free (its instance deleted) */
void MW_Splat_Reset(void)
{
	int i;

	SP_ACTIVE = NULL;
	SP_FREE = SP_POOL;
	for (i = 0; i < 0x203740; i += 0x2c0) {
		baDeleteActorInstance((struct BActorInstance *)(SP_POOL + i + 0x110));
		SP_NEXT(SP_POOL + i) = SP_POOL + i + 0x2c0;
	}
	SP_NEXT(SP_POOL + 0x203740) = NULL;
	baDeleteActorInstance((struct BActorInstance *)(SP_POOL + 0x203850));
}

/* 0x00182c90  (gap): frees the vertex banks and the pool (missing from the function table) */
void FUN_00182c90(void)
{
	int i;

	for (i = 0; i < 10; i++)
		bdVertexBufferDestroy(SP_VB(i));
	if (SP_POOL != NULL) {
		bkHeapFree(SP_POOL);
		SP_POOL = NULL;
	}
}

/* 0x00182ce0  (gap), "Tremors": where a splat of `bug` would lie on the ground strips:
1 inside a gap (no splat), 2 cut by an edge (out: the z of its near and far side and their
v), 3 whole. The gaps are 4 z ranges at 0x002b4a60, the edges 4 z values at 0x002b4a80. */
int MW_Splat_ClipToGround(void *bug, float *out)
{
	float z, size, lo, hi, w, e;
	short i;

	z = MW_AF(MW_AP(bug, 0), 0xd8);
	for (i = 0; i < 7; i += 2)
		if (MW_F(0x002b4a60 + i * 4) < z && z < MW_F(0x002b4a64 + i * 4))
			return 1;
	switch (MW_AS(bug, 0x11ec)) {
	case 0:
	case 5:
		size = 75.0f;
		break;
	case 1:
	case 6:
		size = 135.0f;
		break;
	case 2:
		size = 210.0f;
		break;
	case 3:
		size = 195.0f;
		break;
	default:
		if (mw_DbgMessages != 0)
			bkPrintf("unsuported bug type %d\n", (int)MW_AS(bug, 0x11ec));
		/* (the original then uses the bug pointer's bits as the size) */
		size = mw_bitsf((unsigned int)(uintptr_t)bug);
		break;
	}
	lo = z - size;
	hi = size + z;
	if (lo <= MW_F(0x002b4a60) && -300.0f <= hi) {
		for (i = 0; i < 4; i++) {
			e = MW_F(0x002b4a80 + i * 4);
			if (e < hi && lo < e) {
				w = hi - lo;
				if (z <= e) {
					out[3] = 0.0f;
					out[1] = lo;
					out[2] = 1.0f - (w - (e - lo)) / w;
					out[0] = e;
					return 2;
				}
				out[2] = 1.0f;
				out[0] = hi;
				out[3] = (w - (hi - e)) / w;
				out[1] = e;
				return 2;
			}
		}
	}
	return 3;
}

/* 0x00182ea0  (gap): draws the swipes (kind 8: actors at their alpha as brightness) */
void MW_Splat_DrawSwipes(void)
{
	unsigned char *rec;

	MW_I(0x0030ef00) = 0;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(6, 1, 0);
	for (rec = SP_ACTIVE; rec != NULL; rec = SP_NEXT(rec)) {
		if (RS(rec, 0x84) == 8) {
			baSetBrightness(MW_FTOL(RF(rec, 0x90)));
			baDrawActorInstance((struct BActorInstance *)(rec + 0x110), 7);
			baSetBrightness(0x100);
		}
	}
	bdSetRenderState(5, 1, 0);
	bdSetWorldMatrix(DAT_002855f0);
}

/* the first bank from `from` with room left, or 10 (the original scans on past the 10
banks into the following globals when all are full; the record is skipped either way) */
static short sp_FreeBank(short from)
{
	while (from < 10 && SP_CAP(from) <= SP_COUNT(from))
		from = (short)(from + 1);
	return from;
}

/* 0x00182f50  (gap): draws the hammer rings (kind 7) in their player's colour at their
alpha, with the blast texture */
void MW_Splat_DrawRings(void)
{
	unsigned char *buf[10];
	unsigned char *rec;
	float f;
	short bank = 0;
	int i, r, g, b;

	for (i = 0; i < 10; i++) {
		SP_COUNT(i) = 0;
		buf[i] = (unsigned char *)bdVertexBufferLock(SP_VB(i), 4);
	}
	MW_I(0x0030ef00) = 0;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(7, 2, 0);
	for (rec = SP_ACTIVE; rec != NULL; rec = SP_NEXT(rec)) {
		if (RS(rec, 0x84) != 7)
			continue;
		bdSetTexture(0, (struct bdTexture *)MW_AP(rec, 0x14));
		f = RF(rec, 0x90) * 0.007874016f;
		bank = sp_FreeBank(bank);
		if (bank >= 10)
			continue;
		b = MW_FTOL((float)(int)RS(rec, 0x288) * f);
		g = MW_FTOL((float)(int)RS(rec, 0x286) * f);
		r = MW_FTOL((float)(int)RS(rec, 0x284) * f);
		sp_Strip(buf[bank], bank, rec, SP_COLOUR(0, 0, 0, 0), DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bce18[r] |
			DAT_002bc898[0xff], 1);
	}
	sp_DrawBanks((struct bdTexture *)MW_P(0x0030eec4), 0);
	bdSetRenderState(7, 3, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* 0x00183300  (gap): draws the paint spray marks (kind 5) in their colour and alpha */
void MW_Splat_DrawSprayMarks(void)
{
	unsigned char *buf[10];
	unsigned char *rec;
	short a, bank;
	int i, r, g, b;

	MW_I(0x0030ef00) = 0;
	for (i = 0; i < 10; i++) {
		SP_COUNT(i) = 0;
		buf[i] = (unsigned char *)bdVertexBufferLock(SP_VB(i), 4);
	}
	for (rec = SP_ACTIVE; rec != NULL; rec = SP_NEXT(rec)) {
		if (RS(rec, 0x84) != 5)
			continue;
		a = (short)MW_FTOL(RF(rec, 0x90));
		bank = sp_FreeBank(0);
		if (9 < bank)
			continue;
		b = MW_FTOL((float)(int)RS(rec, 0x288));
		g = MW_FTOL((float)(int)RS(rec, 0x286));
		r = MW_FTOL((float)(int)RS(rec, 0x284));
		sp_Strip(buf[bank], bank, rec, SP_COLOUR(0, 0, 0, 0), DAT_002bc490[g] | DAT_002bce18[r] | DAT_002bc898[a] |
			DAT_002bd640[b], 1);
	}
	bdSetWorldMatrix(DAT_002855f0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	sp_DrawBanks((struct bdTexture *)MW_P(0x0030ee44), 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* 0x001836b0  (gap): draws the splats (kind 0) of splat texture `index`, grey at their
alpha; each record's bank (+0x7a) is its texture frame */
void MW_Splat_DrawSplats(short index)
{
	unsigned char *buf[10];
	unsigned char *rec;
	short level, bank;
	int i;
	struct bdTexture *texture = (struct bdTexture *)MW_P(0x0030eecc + index * 4);

	MW_I(0x0030ef00) = 0;
	for (i = 0; i < 10; i++) {
		SP_COUNT(i) = 0;
		buf[i] = (unsigned char *)bdVertexBufferLock(SP_VB(i), 0);
	}
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	for (rec = SP_ACTIVE; rec != NULL; rec = SP_NEXT(rec)) {
		if (RS(rec, 0x84) != 0 || MW_AP(rec, 0x14) != MW_P(0x0030eecc + index * 4))
			continue;
		level = (short)MW_FTOL(RF(rec, 0x90));
		bank = RS(rec, 0x7a);
		/* (one check for 6 vertices, as the original) */
		if (SP_COUNT(bank) < SP_CAP(bank))
			sp_Strip(buf[bank], bank, rec, SP_COLOUR(0, 0, 0, 0),
				DAT_002bd640[level] | DAT_002bc898[level] | DAT_002bc490[level] | DAT_002bce18[level], 0);
	}
	bdSetWorldMatrix(DAT_002855f0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	sp_DrawBanks(texture, 1);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* 0x001839b0  CBBash_Splat.cpp: creates the pool (zeroed) and the 10 vertex banks, then
empties the pool */
void MW_Splat_Init(void)
{
	int i;

	SP_POOL = (unsigned char *)bkHeapAlloc(0x203a00, mw_SplatFile, 0x45, 0x2001);
	bkPrintf(SP_POOL == NULL ? "Error init splat marks\n" : "init splat marks ok\n");
	for (i = 0; i < 0x203a00; i += 0x2c0)
		memset(SP_POOL + i, 0, 0x2c0);
	for (i = 0; i < 10; i++) {
		if (bdVertexBufferCreate((struct bdVertexBuffer **)MW_ADDR(0x0030ee48 + i * 4), 3000, 0x152, 0, 1) == 0)
			bkPrintf("ERROR splats failed to create vert buffer bank %d!\n", i);
		SP_CAP(i) = 3000;
		SP_COUNT(i) = 0;
	}
	MW_Splat_Reset();
}

/* 0x00183a70  (gap): player `player` is splatted: 20 paint drops (kind 9) in his
character's colour fly out of `pos` */
void MW_Splat_PlayerDrops(float *pos, short player)
{
	float v[4];
	unsigned char *rec;
	int colour, r1, r2;
	short i;

	pos[3] = 1.0f;
	if (mw_DbgMessages != 0)
		bkPrintf("killing player %d\n", (int)player);
	for (i = 0; i < 0x14; i++) {
		rec = sp_Take();
		if (rec == NULL) {
			if (mw_DbgMessages != 0)
				bkPrintf("adding splat mark FAILED\n");
			return;
		}
		MW_AP(rec, 0x14) = MW_P(0x0030ee40);
		RF(rec, 0x88) = 75.0f;
		RF(rec, 0x90) = 125.0f;
		RF(rec, 8) = RF(rec, 8) + 50.0f;
		RF(rec, 0x8c) = 220.0f;
		RF(rec, 0x94) = 0.7f;
		colour = DAT_002baed4[player * 10] * 0x60;
		RS(rec, 0x284) = (short)DAT_00290718[colour];
		RS(rec, 0x286) = (short)DAT_00290718[colour + 1];
		RS(rec, 0x288) = (short)DAT_00290718[colour + 2];
		memcpy(MW_AV(rec, 4), pos, 0x10);
		RS(rec, 0x84) = 9;
		r1 = mw_rand();
		r2 = mw_rand();
		v[0] = (float)r1 * 0.0015259255f;
		v[1] = 0.0f;
		v[2] = 0.0f;
		v[3] = 0.0f;
		bmVectorRotateY(v, v, (float)r2 * 0.00019175345f);
		RF(rec, 0x28c) = v[0];
		RF(rec, 0x294) = v[2];
		RI(rec, 0x298) = 0;
		RI(rec, 0x10) = 0;
		RF(rec, 0x290) = (float)mw_rand() * 0.00076296274f + 10.0f;
	}
}

/* 0x00183ca0  CBBash_Splat.cpp: a swipe of player `player` at `pos` (raised 90), turned
by `angle`: his character's swipe model (name table 0x002b49a0, 0x20 bytes each),
drifting along 0x002b4990 turned by the angle */
void MW_Splat_AddSwipe(float *pos, short player, float angle)
{
	float matrix[16];
	unsigned char *rec;
	void *model;

	pos[3] = 1.0f;
	rec = sp_Take();
	if (rec == NULL) {
		if (mw_DbgMessages != 0)
			bkPrintf("adding splat mark FAILED\n");
		return;
	}
	pos[1] = pos[1] + 90.0f;
	memcpy(MW_AV(rec, 4), pos, 0x10);
	bmVectorRotateY(MW_AV(rec, 0x28c), MW_FV(0x002b4990), angle);
	model = bLoadResource(NULL, MW_A(0x002b49a0 + DAT_002baed4[player * 10] * 0x20), 1, 0xdefa);
	MW_AP(rec, 0x108) = model;
	baInitActorInstance((struct BActorInstance *)(rec + 0x110), (struct BActor *)model, 4, mw_SplatFile, 0x131);
	memcpy(MW_AV(rec, 0x1d0), MW_AV(rec, 4), 0x10);
	bmMatrixRotationY(matrix, angle);
	bmMatrixToQuaternion(MW_AV(rec, 0x1f0), matrix);
	MW_AP(rec, 0x14) = MW_P(0x0030eec8);
	RF(rec, 0x8c) = 10.0f;
	RF(rec, 0x90) = 255.0f;
	RF(rec, 0x94) = 25.5f;
	RF(rec, 0x7c) = 0.3f;
	RI(rec, 0x80) = 0;
	RS(rec, 0x84) = 8;
}

/* the 4 corners of a flat quad of half size `half` around the record's position, 1 above
it; uv (0,1) (1,1) (0,0) (1,0) */
static void sp_FlatQuad(unsigned char *rec, float half)
{
	float y = RF(rec, 8) + 1.0f;

	RF(rec, 0xb8) = RF(rec, 4) - half;
	RF(rec, 0xbc) = y;
	RF(rec, 0xc0) = RF(rec, 0xc) + half;
	RF(rec, 0xc4) = 0.0f;
	RF(rec, 0xc8) = 1.0f;
	RF(rec, 0xcc) = RF(rec, 4) + half;
	RF(rec, 0xd0) = y;
	RF(rec, 0xd4) = RF(rec, 0xc) + half;
	RF(rec, 0xd8) = 1.0f;
	RF(rec, 0xdc) = 1.0f;
	RF(rec, 0xe0) = RF(rec, 4) - half;
	RF(rec, 0xe4) = y;
	RF(rec, 0xe8) = RF(rec, 0xc) - half;
	RF(rec, 0xec) = 0.0f;
	RF(rec, 0xf0) = 0.0f;
	RF(rec, 0xf4) = RF(rec, 4) + half;
	RF(rec, 0xf8) = y;
	RF(rec, 0xfc) = RF(rec, 0xc) - half;
	RF(rec, 0x100) = 1.0f;
	RF(rec, 0x104) = 0.0f;
}

/* 0x00183e00  CBBash_Splat.cpp: a hammer ring (kind 7, 160 wide) of player `player` on
the ground at `pos` (not when the ground is more than 10 away) */
void MW_Splat_AddRing(float *pos, short player)
{
	unsigned char *rec;
	float ground;
	int colour;

	ground = mw_GroundProbe(pos, 0.0f);
	pos[3] = 1.0f;
	if (!(fabsf(ground) <= 10.0f))
		return;
	pos[1] = pos[1] - ground;
	rec = sp_Take();
	if (rec == NULL) {
		if (mw_DbgMessages != 0)
			bkPrintf("adding splat mark FAILED\n");
		return;
	}
	memcpy(MW_AV(rec, 4), pos, 0x10);
	MW_AP(rec, 0x14) = MW_P(0x0030eec4);
	RF(rec, 0x8c) = 10.0f;
	RF(rec, 0x90) = 255.0f;
	RF(rec, 0x94) = 25.5f;
	RF(rec, 0x7c) = 0.3f;
	RI(rec, 0x80) = 0;
	RS(rec, 0x84) = 7;
	RF(rec, 0x88) = 80.0f;
	colour = DAT_002baed4[player * 10] * 0x60;
	RS(rec, 0x284) = (short)DAT_00290718[colour];
	RS(rec, 0x286) = (short)DAT_00290718[colour + 1];
	RS(rec, 0x288) = (short)DAT_00290718[colour + 2];
	sp_FlatQuad(rec, 80.0f);
}

/* 0x00183ff0  CBBash_Splat.cpp: 150 spore drops (kind 12) from `pos` */
void MW_Splat_SporeDrops(float *pos)
{
	float v[4];
	unsigned char *rec;
	int r1, r2;
	short i;

	for (i = 0; i < 0x96; i++) {
		rec = sp_Take();
		if (rec == NULL) {
			if (mw_DbgMessages != 0)
				bkPrintf("adding splat mark FAILED\n");
			return;
		}
		memcpy(MW_AV(rec, 4), pos, 0x10);
		RS(rec, 0x84) = 0xc;
		r1 = mw_rand();
		r2 = mw_rand();
		v[0] = (float)r1 * 0.00061037019f;
		v[1] = 0.0f;
		v[2] = 0.0f;
		v[3] = 0.0f;
		bmVectorRotateY(v, v, (float)r2 * 0.00019175345f);
		RF(rec, 0x28c) = v[0];
		RF(rec, 0x294) = v[2];
		RF(rec, 0x290) = (float)mw_rand() * 0.00030518509f + 10.0f;
	}
}

/* 0x00184150  CBBash_Splat.cpp: 5 spit drops (kind 11) from `pos` */
void MW_Splat_SpitDrops(float *pos)
{
	unsigned char *rec;
	int r1, r2;
	short i;

	pos[3] = 1.0f;
	for (i = 0; i < 5; i++) {
		rec = sp_Take();
		if (rec == NULL) {
			if (mw_DbgMessages != 0)
				bkPrintf("adding splat mark FAILED\n");
			return;
		}
		memcpy(MW_AV(rec, 4), pos, 0x10);
		RS(rec, 0x84) = 0xb;
		(void)mw_GroundProbe(MW_AV(rec, 4), 0.0f);
		r1 = mw_rand();
		r2 = mw_rand();
		RF(rec, 0x290) = 15.0f;
		RF(rec, 0x28c) = (float)r1 * 15.0f * 3.0518509e-05f - 7.5f;
		RF(rec, 0x294) = (float)r2 * 15.0f * 3.0518509e-05f - 7.5f;
		RI(rec, 0x298) = 0;
		RI(rec, 0x10) = 0;
		RS(rec, 0x2ac) = (short)(mw_rand() % 3);
	}
}

/* bits of `bug` (kind `kind`, raised `lift`) flying out at up to 20 with a random spin;
the count is the tweak 0x0030a28c x `amount`; the frame of the shared model by `frames`
(3: % 3, 4: & 3) */
static void sp_Bits(void *bug, int amount, short kind, float lift, int frames)
{
	float v[4];
	unsigned char *rec = NULL;
	short count, i;
	int k, r1, r2;

	count = (short)(MW_FTOL(MW_F(0x0030a28c)) * amount);
	if (count < 1) {
		/* (the original then clears +0x298 and +0x10 of an unset record pointer, the count
		itself; the port leaves that out) */
		return;
	}
	for (i = 0;;) {
		rec = sp_Take();
		if (rec == NULL) {
			if (mw_DbgMessages != 0)
				bkPrintf("adding splat mark FAILED\n");
			return;
		}
		memcpy(MW_AV(rec, 4), MW_AV(MW_AP(bug, 0), 0xd0), 0x10);
		RF(rec, 8) = RF(rec, 8) + lift;
		RF(rec, 0x10) = 1.0f;
		RS(rec, 0x84) = kind;
		RF(rec, 0x2b0) = (float)((double)RF(rec, 8) - (double)mw_GroundProbe(MW_AV(rec, 4), 0.0f));
		r1 = mw_rand();
		r2 = mw_rand();
		v[0] = (float)r1 * 20.0f * 3.0518509e-05f;
		v[1] = 0.0f;
		v[2] = 0.0f;
		v[3] = 0.0f;
		bmVectorRotateY(v, v, (float)r2 * 6.2831855f * 3.0518509e-05f);
		RF(rec, 0x28c) = v[0];
		RF(rec, 0x294) = v[2];
		RF(rec, 0x290) = 20.0f;
		RI(rec, 0xa8) = 0;
		RI(rec, 0xac) = 0;
		RI(rec, 0xb0) = 0;
		RF(rec, 0xb4) = 1.0f;
		for (k = 0; k < 3; k++)
			RF(rec, 0x98 + k * 4) = (float)mw_rand() * 3.0518509e-05f - 0.5f;
		i = (short)(i + 1);
		if (frames == 3)
			RS(rec, 0x2ac) = (short)(mw_rand() % 3);
		else
			RS(rec, 0x2ac) = (short)(mw_rand() & 3);
		if (count <= i)
			break;
	}
	RI(rec, 0x298) = 0;
	RI(rec, 0x10) = 0;
}

/* 0x001842b0  CBBash_Splat.cpp: bits of a bug (kind 3), `amount` x the tweak */
void MW_Splat_BugBits(void *bug, int amount)
{
	sp_Bits(bug, amount, 3, 0.0f, 3);
}

/* 0x00184500  CBBash_Splat.cpp: bits of a tremor (kind 10), from 200 up */
void MW_Splat_TremorBits(void *bug, int amount)
{
	sp_Bits(bug, amount, 10, 200.0f, 4);
}

/* 0x00184760  CBBash_Splat.cpp: 8 crumbs (kind 1) of a bitten burger stand at `pos`
(the second argument is not used) */
void Splat_AddCrumbs(float *pos, int unused)
{
	unsigned char *rec;
	int k, r1, r2;
	short i;

	(void)unused;
	pos[3] = 1.0f;
	for (i = 0; i < 8; i++) {
		rec = sp_Take();
		if (rec == NULL) {
			if (mw_DbgMessages != 0)
				bkPrintf("adding splat mark FAILED\n");
			return;
		}
		memcpy(MW_AV(rec, 4), pos, 0x10);
		RS(rec, 0x84) = 1;
		RF(rec, 0x2b0) = mw_GroundProbe(MW_AV(rec, 4), 0.0f);
		r1 = mw_rand();
		r2 = mw_rand();
		RF(rec, 0x290) = 20.0f;
		RF(rec, 0x28c) = (float)r1 * 20.0f * 3.0518509e-05f - 10.0f;
		RF(rec, 0x294) = (float)r2 * 20.0f * 3.0518509e-05f - 10.0f;
		RS(rec, 0x2ac) = (short)(mw_rand() % 3);
		RI(rec, 0xa8) = 0;
		RI(rec, 0xac) = 0;
		RI(rec, 0xb0) = 0;
		RF(rec, 0xb4) = 1.0f;
		for (k = 0; k < 3; k++)
			RF(rec, 0x98 + k * 4) = (float)mw_rand() * 3.0518509e-05f - 0.5f;
		RI(rec, 0x298) = 0;
		RI(rec, 0x10) = 0;
	}
}

/* 0x00184930  CBBash_Splat.cpp: the paint gun of `player` sprays blobs (kind 2, the
tweak 0x0030ebec of them) from `pos` at 60 along `angle` with a random spread of 15,
carried by his velocity */
void MW_Splat_AddSpray(float *pos, void *player, float angle)
{
	float dir[4];
	unsigned char *rec;
	int r1, r2, r3;
	short i = 0;

	dir[0] = 60.0f;
	dir[1] = 0.0f;
	dir[2] = 0.0f;
	dir[3] = 0.0f;
	bmVectorRotateY(dir, dir, angle - 1.5707963f);
	if (!(0.0f < MW_F(0x0030ebec)))
		return;
	for (;;) {
		rec = sp_Take();
		if (rec == NULL) {
			if (mw_DbgMessages != 0)
				bkPrintf("adding spray mark FAILED\n");
			return;
		}
		RS(rec, 0x284) = MW_AS(player, 0x136e);
		RS(rec, 0x286) = MW_AS(player, 0x1370);
		RS(rec, 0x288) = MW_AS(player, 0x1372);
		memcpy(MW_AV(rec, 0x29c), MW_AV(MW_AP(player, 0x30), 0x194), 0x10);
		memcpy(MW_AV(rec, 4), pos, 0x10);
		RS(rec, 0x84) = 2;
		RF(rec, 0x8c) = 42.0f;
		memcpy(MW_AV(rec, 0x28c), dir, 0x10);
		r1 = mw_rand();
		r2 = mw_rand();
		r3 = mw_rand();
		RF(rec, 0x90) = 127.0f;
		RF(rec, 0x94) = 3.0f;
		i = (short)(i + 1);
		RF(rec, 0x28c) = ((float)r1 * 30.0f * 3.0518509e-05f - 15.0f) + RF(rec, 0x28c);
		RF(rec, 0x290) = ((float)r2 * 30.0f * 3.0518509e-05f - 15.0f) + RF(rec, 0x290);
		RF(rec, 0x294) = ((float)r3 * 30.0f * 3.0518509e-05f - 15.0f) + RF(rec, 0x294);
		MW_AP(rec, 0x14) = MW_P(0x0030ee44);
		RF(rec, 0x88) = 5.0f;
		if (MW_F(0x0030ebec) <= (float)(int)i)
			return;
	}
}

/* 0x00184bb0  CBBash_Splat.cpp: the splat (kind 0) of a dead `bug` in the colour of the
player who hit it, sized by the type, a random turn, texture frames; in "Tremors" only on
the ground strips (cut at their edges). Returns the record or NULL. */
void *MW_Splat_AddBugSplat(void *bug)
{
	float clip[4];
	float ground = 0.0f;
	unsigned char *rec;
	unsigned char *texture;
	int where = 3;

	if (SP_FREE == NULL) {
		if (mw_DbgMessages != 0)
			bkPrintf("adding splat mark FAILED\n");
		return NULL;
	}
	if (MW_AS(bug, 0x120c) == -1) {
		/* (the original passes no file and line for the %s and %d) */
		bkPrintf("%s(%d) : Bug detonated without being hit [indirectly] by a player\n", mw_SplatFile, 0);
		return NULL;
	}
	if (mw_Variation == 1) {
		ground = mw_GroundProbe(MW_AV(MW_AP(bug, 0), 0xd0), 0.0f);
		where = MW_Splat_ClipToGround(bug, clip);
		if (2.0f < MW_AF(MW_AP(bug, 0), 0xd4) - ground)
			return NULL;
		if ((short)where == 1)
			return NULL;
	}
	rec = sp_Take();
	memcpy(MW_AV(rec, 4), MW_AV(MW_AP(bug, 0), 0xd0), 0x10);
	RF(rec, 0x10) = 1.0f;
	RS(rec, 0x7a) = 0;
	RF(rec, 0xac) = (float)((double)Core_RandUnit() * (double)6.283185f);
	if (mw_Variation == 1)
		RF(rec, 8) = RF(rec, 8) - (ground - 1.0f);
	else
		RF(rec, 8) = 1.0f;
	if (mw_DbgMessages != 0)
		bkPrintf("adding splat at %f\n", (double)RF(rec, 8));
	RF(rec, 0x8c) = 1000.0f;
	RF(rec, 0x90) = 127.0f;
	RF(rec, 0x94) = 0.127f;
	RF(rec, 0x7c) = 0.8f;
	RI(rec, 0x80) = 0;
	RS(rec, 0x84) = 0;
	RI(rec, 0x294) = 0;
	RI(rec, 0x290) = 0;
	RI(rec, 0x28c) = 0;
	switch (MW_AS(bug, 0x11ec)) {
	case 0:
	case 5:
		RF(rec, 0x88) = 75.0f;
		break;
	case 1:
	case 6:
		(void)Core_Rand();
		RF(rec, 0x88) = 135.0f;
		break;
	case 2:
		RF(rec, 0x88) = 210.0f;
		break;
	case 3:
		RF(rec, 0x88) = 195.0f;
		break;
	default:
		if (mw_DbgMessages != 0)
			bkPrintf("unsuported bug type %d\n", (int)MW_AS(bug, 0x11ec));
	}
	texture = (unsigned char *)MW_P(0x0030eecc + MW_AS(mw_Players[MW_AS(bug, 0x120c)], 0x1374) * 4);
	MW_AP(rec, 0x14) = texture;
	RF(rec, 0xd8) = 1.0f;
	RF(rec, 0x100) = 1.0f;
	RI(rec, 0xc4) = 0;
	RI(rec, 0xec) = 0;
	RS(rec, 0x78) = (short)(MW_AB(texture, 0x37) - 1);
	if ((short)where != 2) {
		sp_FlatQuad(rec, RF(rec, 0x88));
		RI(rec, 0x298) = 0;
		RI(rec, 0x10) = 0;
		return rec;
	}
	/* cut at a ground edge: the near/far z and their v */
	sp_FlatQuad(rec, RF(rec, 0x88));
	RF(rec, 0xd4) = clip[0];
	RF(rec, 0xc0) = clip[0];
	RF(rec, 0xe8) = clip[1];
	RF(rec, 0xfc) = clip[1];
	RF(rec, 0xdc) = clip[2];
	RF(rec, 0xc8) = clip[2];
	RF(rec, 0x104) = clip[3];
	RF(rec, 0xf0) = clip[3];
	return rec;
}

/* 0x00184f40  CBBash_Splat.cpp: a paint spray mark (kind 5, 100 wide) of player
`player` at `pos`; visible only on the ground (up to 10 high) */
void MW_Splat_AddSprayMark(float *pos, short player)
{
	unsigned char *rec;
	int colour;

	if (SP_FREE == NULL) {
		if (mw_DbgMessages != 0)
			bkPrintf("adding splat mark FAILED\n");
		return;
	}
	pos[3] = 1.0f;
	rec = sp_Take();
	RS(rec, 0x84) = 5;
	MW_AP(rec, 0x14) = MW_P(0x0030ee44);
	colour = DAT_002baed4[player * 10] * 0x60;
	RS(rec, 0x284) = (short)DAT_00290718[colour];
	RS(rec, 0x286) = (short)DAT_00290718[colour + 1];
	RS(rec, 0x288) = (short)DAT_00290718[colour + 2];
	memcpy(MW_AV(rec, 4), pos, 0x10);
	RF(rec, 0x90) = pos[1] <= 10.0f ? 127.0f : 0.0f;
	RF(rec, 0x88) = 50.0f;
	RF(rec, 0x94) = RF(rec, 0x90) * 0.001f;
	sp_FlatQuad(rec, 50.0f);
	RI(rec, 0x298) = 0;
	RI(rec, 0x10) = 0;
}

/* 0x00185110  (gap): draws the debris (kinds 1, 3, 10, 11) with the shared models: each
record moves model +0x2ac of its kind to its position and spin, then draws it */
void MW_Splat_DrawDebris(void)
{
	unsigned char *rec, *inst;

	MW_I(0x0030ef00) = 0;
	for (rec = SP_ACTIVE; rec != NULL; rec = SP_NEXT(rec)) {
		switch (RS(rec, 0x84)) {
		case 1:
			bdSetRenderState(5, 1, 0);
			inst = MW_A(0x0030ce80 + RS(rec, 0x2ac) * 0x170);
			memcpy(inst + 0xc0, MW_AV(rec, 4), 0x10);
			CLQuat_FromEuler((float *)(inst + 0xe0), MW_AV(rec, 0xa8), 0.0f);
			baPrepActorInstance((struct BActorInstance *)inst, 1);
			baDrawActorInstance((struct BActorInstance *)inst, 7);
			break;
		case 3:
			bdSetRenderState(5, 1, 0);
			bdSetRenderState(10, 0, 0);
			bdSetRenderState(6, 1, 0);
			bdTextureSetFrame((struct bdTexture *)MW_P(0x0030eee8), 0);
			bdSetTexture(0, (struct bdTexture *)MW_P(0x0030eee8));
			inst = MW_A(0x0030d5b0 + RS(rec, 0x2ac) * 0x170);
			CLQuat_FromEuler((float *)(inst + 0xe0), MW_AV(rec, 0xa8), 0.0f);
			memcpy(inst + 0xc0, MW_AV(rec, 4), 0x10);
			baPrepActorInstance((struct BActorInstance *)inst, 1);
			baDrawActorInstance((struct BActorInstance *)inst, 7);
			break;
		case 10:
			bdSetRenderState(5, 1, 0);
			inst = MW_A(0x0030da00 + RS(rec, 0x2ac) * 0x170);
			CLQuat_FromEuler((float *)(inst + 0xe0), MW_AV(rec, 0xa8), 0.0f);
			memcpy(inst + 0xc0, MW_AV(rec, 4), 0x10);
			baPrepActorInstance((struct BActorInstance *)inst, 1);
			baDrawActorInstance((struct BActorInstance *)inst, 7);
			break;
		case 0xb:
			bdSetRenderState(5, 1, 0);
			inst = MW_A(0x0030dfc0 + RS(rec, 0x2ac) * 0x170);
			memcpy(inst + 0xc0, MW_AV(rec, 4), 0x10);
			baPrepActorInstance((struct BActorInstance *)inst, 1);
			baDrawActorInstance((struct BActorInstance *)inst, 7);
			break;
		}
	}
	bdSetRenderState(5, 1, 0);
}

/* a sprite of a record in its colour at its alpha (colours scaled by alpha / 127) */
static void sp_Sprite(unsigned char *rec, float size)
{
	float f = RF(rec, 0x90) * 0.007874016f;
	short a, r, g, b;

	RF(rec, 0x10) = 1.0f;
	a = (short)MW_FTOL(RF(rec, 0x90));
	b = (short)MW_FTOL((float)(int)RS(rec, 0x288) * f);
	g = (short)MW_FTOL((float)(int)RS(rec, 0x286) * f);
	r = (short)MW_FTOL((float)(int)RS(rec, 0x284) * f);
	bdDrawSprite3D(MW_AV(rec, 4), size, size, 0.0f, (struct bdTexture *)MW_AP(rec, 0x14), 0, 0, r, g, b, a, NULL, 0.0f);
}

/* 0x001853e0  (gap): draws the spray blobs (kind 2, 5 x size) and paint drops (kind 9)
as sprites, the spore explosion (kind 6) and the spore drops (kind 12, green streaks) */
void MW_Splat_DrawSprites(void)
{
	unsigned char *rec;
	int head[4], tail[4];
	float end[4];

	head[0] = 0x32;
	head[1] = 0x7f;
	head[2] = 0;
	head[3] = 0xff;
	tail[0] = 0x32;
	tail[1] = 0x7f;
	tail[2] = 0;
	tail[3] = 0;
	MW_I(0x0030ef00) = 0;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(5, 0, 0);
	for (rec = SP_ACTIVE; rec != NULL; rec = SP_NEXT(rec)) {
		switch (RS(rec, 0x84)) {
		case 2:
			bdSetRenderState(5, 0, 0);
			bdSetRenderState(10, 1, 0);
			sp_Sprite(rec, RF(rec, 0x88) * 5.0f);
			bdSetRenderState(10, 0, 0);
			break;
		case 6:
			bdSetRenderState(5, 1, 0);
			bdSetRenderState(10, 0, 0);
			bdSetRenderState(6, 1, 0);
			baDrawActorInstance((struct BActorInstance *)(rec + 0x110), 7);
			break;
		case 9:
			bdSetRenderState(5, 0, 0);
			bdSetRenderState(10, 1, 0);
			sp_Sprite(rec, RF(rec, 0x88));
			bdSetRenderState(10, 0, 0);
			break;
		case 0xc:
			bdSetRenderState(3, 0, 0);
			end[3] = RF(rec, 0x10);
			end[0] = RF(rec, 4) - (RF(rec, 0x28c) + RF(rec, 0x28c));
			end[1] = RF(rec, 8) - (RF(rec, 0x290) + RF(rec, 0x290));
			end[2] = RF(rec, 0xc) - (RF(rec, 0x294) + RF(rec, 0x294));
			bdSetRenderState(5, 1, 0);
			bdDrawBeam(MW_AV(rec, 4), 2.0f, 2.0f, head, end, 4.0f, 4.0f, tail, NULL, NULL, 0.0f);
			bdSetRenderState(3, 1, 0);
			break;
		}
	}
	bdSetRenderState(5, 1, 0);
}

/* 0x00185720  CBBash_Splat.cpp: the segment from `start` along `dir` against the
collision meshes of the actors (not ones with +0x21c set) and of the platforms (not
hidden ones, +0x230 bit 0x20; moving ones refreshed first): the hit nearest `start`
goes to `outPoint` (w: 1 on a moving platform), its triangle and platform to
`outTri`/`outPlatform`. Returns 1 - the hit's fraction from the end, or 0. */
float MW_Splat_SegmentHit(float *start, float *dir, float *outPoint, void **outTri, void **outPlatform)
{
	float end[4], lo[3], hi[3], hit[4];
	float best = 0.0f, d, t, denom;
	unsigned char *node, *tri;
	int *mesh;
	int k, pass;

	end[0] = start[0] + dir[0];
	end[1] = dir[1] + start[1];
	end[2] = dir[2] + start[2];
	for (k = 0; k < 3; k++) {
		lo[k] = start[k] < end[k] ? start[k] : end[k];
		hi[k] = end[k] < start[k] ? start[k] : end[k];
	}
	for (pass = 0; pass < 2; pass++) {
		unsigned int *list = pass == 0 ? (unsigned int *)DAT_002ea488 : (unsigned int *)(DAT_002ea488 + 0x18);

		for (node = (unsigned char *)(uintptr_t)CLList_Next(list, NULL); node != NULL;
		     node = (unsigned char *)(uintptr_t)CLList_Next(list, (unsigned int *)node)) {
			if (pass == 0) {
				if (MW_AI(node, 0x21c) != 0)
					continue;
				mesh = (int *)MW_AP(node, 0x190);
			} else {
				if (MW_AB(node, 0x3d4) != 0 && MW_AP(node, 0x1a0) != NULL)
					CRColl_RefreshMesh(node, NULL);
				if ((MW_AB(node, 0x230) & 0x20) != 0)
					continue;
				mesh = (int *)MW_AP(node, 0x1a0);
			}
			if (mesh == NULL || !(MW_AF(mesh, 0xc) <= hi[1] && lo[1] <= MW_AF(mesh, 0x1c) && MW_AF(mesh, 8) <= hi[0] &&
			    lo[0] <= MW_AF(mesh, 0x18) && MW_AF(mesh, 0x10) <= hi[2] && lo[2] <= MW_AF(mesh, 0x20)) ||
			    !(0 < mesh[1]))
				continue;
			for (k = 0; k < mesh[1]; k++) {
				tri = (unsigned char *)MW_AP(mesh, 0) + k * 0xa8;
				if (!(lo[0] <= MW_AF(tri, 0x44) && MW_AF(tri, 0x40) <= hi[0] && lo[1] <= MW_AF(tri, 0x4c) &&
				      MW_AF(tri, 0x48) <= hi[1] && lo[2] <= MW_AF(tri, 0x54) && MW_AF(tri, 0x50) <= hi[2]))
					continue;
				d = end[0] * MW_AF(tri, 0x30) + end[1] * MW_AF(tri, 0x34) + end[2] * MW_AF(tri, 0x38);
				denom = d - bVec3Dot(MW_AV(tri, 0x30), start);
				if (denom == 0.0f || !(bVec3Dot(MW_AV(tri, 0x30), dir) < 0.0f))
					continue;
				t = (d - MW_AF(tri, 0xa0)) / denom;
				if (!(0.0f < t && t < 1.0f))
					continue;
				bmVectorBlend(hit, start, t, end, 1.0f - t);
				hit[3] = 1.0f;
				if (CRColl_PointInTri((float *)tri, hit) == 0 || !(best < t))
					continue;
				if (outTri != NULL)
					*outTri = tri;
				if (pass == 0) {
					outPoint[0] = hit[0];
					outPoint[1] = hit[1];
					outPoint[2] = hit[2];
					outPoint[3] = 0.0f;
				} else {
					if (outPlatform != NULL)
						*outPlatform = node;
					outPoint[1] = hit[1];
					outPoint[0] = hit[0];
					outPoint[3] = hit[3];
					outPoint[2] = hit[2];
					outPoint[3] = MW_AF(node, 0x1a4) == 0.0f ? 0.0f : 1.0f;
				}
				best = t;
			}
		}
	}
	if (0.0f < best)
		return 1.0f - best;
	return 0.0f;
}

/* 0x00185d70  CBBash_Splat.cpp: the spore's explosion at `pos` (kind 6: the "boom" of
models\sporexplo.obe, growing for 10 frames) and its 150 drops */
void MW_Splat_SporeExplosion(float *pos)
{
	unsigned char *rec;
	void *model;

	pos[3] = 1.0f;
	rec = sp_Take();
	if (rec == NULL) {
		if (mw_DbgMessages != 0)
			bkPrintf("adding splat mark FAILED\n");
		return;
	}
	model = bLoadResource(NULL, "models\\sporexplo.obe", 1, 0xdefa);
	MW_AP(rec, 0x108) = model;
	baInitActorInstance((struct BActorInstance *)(rec + 0x110), (struct BActor *)model, 4, mw_SplatFile, 0x1ba);
	MW_AP(rec, 0x280) = baFindSubObject((struct BActorInstance *)(rec + 0x110), "boom");
	RS(rec, 0x84) = 6;
	RF(rec, 0x8c) = 10.0f;
	baFlushAnimations(rec + 0x110, (struct BActorNode *)MW_AP(rec, 0x244), 1, 0);
	baQueueAnimation((struct BActorInstance *)(rec + 0x110), (struct BActorNode *)MW_AP(rec, 0x244),
		(struct BAnim *)MW_AP(rec, 0x280), 0x1d33, -1, 1, NULL, NULL);
	memcpy(MW_AV(rec, 4), pos, 0x10);
	memcpy(MW_AV(rec, 0x1d0), pos, 0x10);
	MW_Splat_SporeDrops(pos);
}

/* 0x00185e90  (gap): the ground under a segment: the hit point to `outPoint` and the
hit triangle's normal to `outNormal` (w 1) */
void MW_Splat_GroundUnder(float *start, float *dir, float *outPoint, float *outNormal)
{
	void *tri;

	if (0.0f < MW_Splat_SegmentHit(start, dir, outPoint, &tri, NULL)) {
		memcpy(outNormal, MW_AV(tri, 0x30), 0x10);
		outNormal[3] = 1.0f;
		return;
	}
	outNormal[3] = 1.0f;
}

/* 0x00185f00  (gap): a gib splat (kind 0, splat2 texture, 74 wide) where a bug bit or
spore drop lands at `pos`; in "Tremors" laid on the ground under it (riding the train
when it lands on it) */
void MW_Splat_AddGibSplat(float *pos)
{
	struct CLQuadVertex quad[4];
	float start[4], dir[4], point[4], normal[4];
	unsigned char *rec, *node;
	int k;

	pos[3] = 1.0f;
	rec = sp_Take();
	if (rec == NULL) {
		if (mw_DbgMessages != 0)
			bkPrintf("adding splat mark FAILED\n");
		return;
	}
	memcpy(MW_AV(rec, 4), pos, 0x10);
	MW_AP(rec, 0x14) = MW_P(0x0030eee8);
	RS(rec, 0x78) = (short)(MW_AB(MW_P(0x0030eee8), 0x37) - 1);
	if (mw_Variation == 1)
		RF(rec, 8) = (float)((double)RF(rec, 8) - (double)mw_GroundProbe(MW_AV(rec, 4), 0.0f));
	else
		RF(rec, 8) = 1.0f;
	RF(rec, 0x88) = 37.0f;
	RI(rec, 0x294) = 0;
	RI(rec, 0x290) = 0;
	RI(rec, 0x28c) = 0;
	if (mw_Variation == 1) {
		start[0] = pos[0];
		start[2] = pos[2];
		start[3] = pos[3];
		start[1] = pos[1] + 100.0f;
		dir[0] = (pos[0] - start[0]) * 1.4f;
		dir[1] = ((pos[1] - 2000.0f) - start[1]) * 1.4f;
		dir[2] = (start[2] - start[2]) * 1.4f;
		dir[3] = start[3];
		memset(point, 0, sizeof point);
		MW_Splat_GroundUnder(start, dir, point, normal);
		if (point[3] == 1.0f) {
			/* on the train: the splat moves with it */
			for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
			     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
				if (MW_AS(node, 0x11f2) != 0 && MW_AS(node, 0x11e4) == 8)
					memcpy(MW_AV(rec, 0x28c), MW_AV(MW_AP(node, -4), 0x1a4), 0x10);
			}
		}
		point[1] = point[1] + 1.0f;
		CLHud_BuildGroundQuad(quad, point, normal, RF(rec, 0x88), 0.0f, 80.0f);
		for (k = 0; k < 4; k++) {
			RF(rec, 0xb8 + k * 0x14) = quad[k].x;
			RF(rec, 0xbc + k * 0x14) = quad[k].y;
			RF(rec, 0xc0 + k * 0x14) = quad[k].z;
		}
	}
	if (mw_DbgMessages != 0)
		bkPrintf("adding gibsplat at %f\n", (double)RF(rec, 8));
	MW_AP(rec, 0x14) = MW_P(0x0030eee8);
	RF(rec, 0x8c) = 1000.0f;
	RS(rec, 0x7a) = 0;
	RF(rec, 0x90) = 127.0f;
	RF(rec, 0x94) = 0.127f;
	RF(rec, 0x7c) = 0.3f;
	RI(rec, 0x80) = 0;
	RS(rec, 0x84) = 0;
	RS(rec, 0x78) = (short)(MW_AB(MW_P(0x0030eee8), 0x37) - 1);
	if (mw_Variation != 1) {
		float half = RF(rec, 0x88), y = RF(rec, 8) + 1.0f;

		RF(rec, 0xe0) = RF(rec, 4) - half;
		RF(rec, 0xb8) = RF(rec, 4) - half;
		RF(rec, 0xf4) = half + RF(rec, 4);
		RF(rec, 0xcc) = half + RF(rec, 4);
		RF(rec, 0xf8) = y;
		RF(rec, 0xe4) = y;
		RF(rec, 0xd0) = y;
		RF(rec, 0xbc) = y;
		RF(rec, 0xd4) = half + RF(rec, 0xc);
		RF(rec, 0xc0) = half + RF(rec, 0xc);
		RF(rec, 0xfc) = RF(rec, 0xc) - half;
		RF(rec, 0xe8) = RF(rec, 0xc) - half;
	}
	RI(rec, 0xec) = 0;
	RI(rec, 0xc4) = 0;
	RF(rec, 0x100) = 1.0f;
	RF(rec, 0xd8) = 1.0f;
	RF(rec, 0xdc) = 1.0f;
	RF(rec, 0xc8) = 1.0f;
	RI(rec, 0x104) = 0;
	RI(rec, 0xf0) = 0;
	RI(rec, 0x298) = 0;
	RI(rec, 0x10) = 0;
}

/* 0x00186280  (gap): the frame of every record (not while paused, not in game state 2 of
0x0030ce00): splats fade and animate and ride, debris and drops fly and fall (bug bits
and spore drops leave gib splats where they land), sprays drift, rings grow, swipes and
the explosion move and scale. Finished records go back to the free list. */
void MW_Splat_Update(void)
{
	unsigned char *prev, *rec, *next;
	float value, top, cam[3], heading;
	float rot[16], scale[16], move[16], tmp[16], world[16];
	float step[4];
	short done = 0;
	int k;

	if (DAT_002ecdec != 0 || MW_S(0x0030ce00) == 2)
		return;
	prev = MW_A(0x0030eef4);   /* the head pointer serves as the first "previous" record */
	for (rec = SP_ACTIVE; rec != NULL; rec = SP_NEXT(rec)) {
		switch (RS(rec, 0x84)) {
		case 0:
			value = RF(rec, 0x8c) - DAT_0028fda8;
			RF(rec, 0x8c) = value;
			if (value <= 0.0f) {
				done = 1;
				goto remove;
			}
			done = 0;
			value = RF(rec, 0x90) - DAT_0028fda8 * RF(rec, 0x94);
			RF(rec, 0x90) = value;
			if (value < 0.0f)
				RI(rec, 0x90) = 0;
			top = (float)(int)RS(rec, 0x78);
			if (RF(rec, 0x80) < top)
				RF(rec, 0x80) = DAT_0028fda8 * RF(rec, 0x7c) + RF(rec, 0x80);
			if (top < RF(rec, 0x80))
				RF(rec, 0x80) = top;
			RS(rec, 0x7a) = (short)MW_FTOL(RF(rec, 0x80));
			value = DAT_0028fda8 * RF(rec, 0x28c);
			for (k = 0; k < 4; k++)
				RF(rec, 0xb8 + k * 0x14) = value + RF(rec, 0xb8 + k * 0x14);
			break;
		case 1:
		case 3:
		case 10:
			/* (the original adds the spin to the angle and wraps the spin speed) */
			for (k = 0; k < 3; k++) {
				RF(rec, 0xa8 + k * 4) = RF(rec, 0xa8 + k * 4) + RF(rec, 0x98 + k * 4);
				RF(rec, 0x98 + k * 4) = CLMath_WrapAngle(RF(rec, 0x98 + k * 4));
			}
			step[0] = DAT_0028fda8 * RF(rec, 0x28c);
			step[1] = DAT_0028fda8 * RF(rec, 0x290);
			step[2] = DAT_0028fda8 * RF(rec, 0x294);
			RF(rec, 4) = step[0] + RF(rec, 4);
			RF(rec, 8) = step[1] + RF(rec, 8);
			RF(rec, 0xc) = step[2] + RF(rec, 0xc);
			RF(rec, 0x290) = RF(rec, 0x290) - DAT_0028fda8;
			value = mw_GroundProbe(MW_AV(rec, 4), 0.0f);
			if (value <= 1.0f) {
				RF(rec, 8) = (float)((double)RF(rec, 8) - ((double)value - (double)2.0f));
				if (RS(rec, 0x84) == 3)
					MW_Splat_AddGibSplat(MW_AV(rec, 4));
				done = 1;
				goto remove;
			}
			done = 0;
			break;
		case 2:
			value = RF(rec, 0x8c) - DAT_0028fda8;
			RF(rec, 0x8c) = value;
			if (!(0.0f < value)) {
				done = 1;
				goto remove;
			}
			cmb_vec3_add(MW_AV(rec, 4), MW_AV(rec, 4), MW_AV(rec, 0x28c));
			cmb_vec3_add(MW_AV(rec, 4), MW_AV(rec, 4), MW_AV(rec, 0x29c));
			RF(rec, 0x28c) = RF(rec, 0x28c) * 0.8f;
			RF(rec, 0x290) = RF(rec, 0x290) * 0.8f;
			RF(rec, 0x294) = RF(rec, 0x294) * 0.8f;
			RF(rec, 0x90) = RF(rec, 0x90) - DAT_0028fda8 * RF(rec, 0x94);
			RF(rec, 0x88) = DAT_0028fda8 * 0.3f + RF(rec, 0x88);
			RF(rec, 0x290) = RF(rec, 0x290) - 0.1f;
			done = 0;
			break;
		case 4:
			/* a billboard facing the camera, growing by 0.1 a frame */
			RF(rec, 8) = RF(rec, 8) + 10.0f;
			cam[0] = MW_AF(mw_Camera, 0x534) - MW_AF(mw_Camera, 0x544);
			cam[1] = MW_AF(mw_Camera, 0x538) - MW_AF(mw_Camera, 0x548);
			cam[2] = MW_AF(mw_Camera, 0x53c) - MW_AF(mw_Camera, 0x54c);
			heading = mw_Heading(cam[0], cam[1]);
			(void)mw_Heading(cam[1], cam[2]);
			bmMatrixRotationY(rot, -heading);
			bmMatrixScaling(scale, RF(rec, 0x88), RF(rec, 0x88), RF(rec, 0x88));
			bmMatrixTranslation(move, RF(rec, 4), RF(rec, 8), RF(rec, 0xc));
			bmMatrixMultiplyCopy(tmp, move, rot);
			bmMatrixMultiplyCopy(world, tmp, scale);
			RF(rec, 0x88) = RF(rec, 0x88) + 0.1f;
			memcpy(MW_AV(rec, 0x18), world, 0x40);
			value = RF(rec, 0x90) - 1.0f;
			RF(rec, 0x90) = value;
			if (0.0f < value) {
				done = 0;
				break;
			}
			done = 1;
			goto remove;
		case 5:
			value = RF(rec, 0x90) - DAT_0028fda8 * RF(rec, 0x94);
			RF(rec, 0x90) = value;
			done = 0;
			if (value <= 0.0f) {
				done = 1;
				goto remove;
			}
			break;
		case 6:
			value = RF(rec, 0x8c) - DAT_0028fda8;
			RF(rec, 0x8c) = value;
			if (value <= 0.0f)
				goto release;
			value = 11.0f - value;
			RF(rec, 0x1e8) = value;
			done = 0;
			RF(rec, 0x1e4) = value;
			RF(rec, 0x1e0) = value;
			baPrepActorInstance((struct BActorInstance *)(rec + 0x110), 1);
			break;
		case 7:
			value = RF(rec, 0x8c) - DAT_0028fda8;
			RF(rec, 0x8c) = value;
			if (!(0.0f < value)) {
				done = 1;
				goto remove;
			}
			done = 0;
			RF(rec, 0x88) = DAT_0028fda8 * 5.0f + RF(rec, 0x88);
			RF(rec, 0x90) = RF(rec, 0x90) - DAT_0028fda8 * RF(rec, 0x94);
			RF(rec, 0xe0) = RF(rec, 4) - RF(rec, 0x88);
			RF(rec, 0xb8) = RF(rec, 4) - RF(rec, 0x88);
			RF(rec, 0xf4) = RF(rec, 4) + RF(rec, 0x88);
			RF(rec, 0xcc) = RF(rec, 4) + RF(rec, 0x88);
			RF(rec, 0xd4) = RF(rec, 0xc) + RF(rec, 0x88);
			RF(rec, 0xc0) = RF(rec, 0xc) + RF(rec, 0x88);
			RF(rec, 0xfc) = RF(rec, 0xc) - RF(rec, 0x88);
			RF(rec, 0xe8) = RF(rec, 0xc) - RF(rec, 0x88);
			break;
		case 8:
			value = RF(rec, 0x8c) - DAT_0028fda8;
			RF(rec, 0x8c) = value;
			if (value <= 0.0f)
				goto release;
			done = 0;
			cmb_vec3_scale(step, MW_AV(rec, 0x28c), DAT_0028fda8);
			cmb_vec3_add(MW_AV(rec, 0x1d0), MW_AV(rec, 0x1d0), step);
			baPrepActorInstance((struct BActorInstance *)(rec + 0x110), 1);
			break;
		case 9:
			value = RF(rec, 0x8c) - DAT_0028fda8;
			RF(rec, 0x8c) = value;
			if (value <= 0.0f) {
				done = 1;
				goto remove;
			}
			done = 0;
			RF(rec, 0x90) = RF(rec, 0x90) - DAT_0028fda8 * RF(rec, 0x94);
			RF(rec, 0x28c) = RF(rec, 0x28c) * 0.8f;
			RF(rec, 0x294) = RF(rec, 0x294) * 0.8f;
			RF(rec, 0x290) = RF(rec, 0x290) * 0.9f - 0.6f;
			bmVectorBlend(MW_AV(rec, 4), MW_AV(rec, 4), 1.0f, MW_AV(rec, 0x28c), DAT_0028fda8);
			break;
		case 0xb:
			step[0] = DAT_0028fda8 * RF(rec, 0x28c);
			step[1] = DAT_0028fda8 * RF(rec, 0x290);
			step[2] = DAT_0028fda8 * RF(rec, 0x294);
			RF(rec, 4) = step[0] + RF(rec, 4);
			RF(rec, 8) = step[1] + RF(rec, 8);
			RF(rec, 0xc) = step[2] + RF(rec, 0xc);
			RF(rec, 0x290) = RF(rec, 0x290) - DAT_0028fda8;
			done = 0;
			if (mw_GroundProbe(MW_AV(rec, 4), 0.0f) <= 1.0f) {
				done = 1;
				goto remove;
			}
			break;
		case 0xc:
			bmVectorBlend(MW_AV(rec, 4), MW_AV(rec, 4), 1.0f, MW_AV(rec, 0x28c), DAT_0028fda8);
			value = RF(rec, 0x290) - DAT_0028fda8;
			RF(rec, 0x290) = value;
			if (RF(rec, 8) < 0.0f && value < 0.0f) {
				done = 1;
				MW_Splat_AddGibSplat(MW_AV(rec, 4));
				goto remove;
			}
			done = 0;
			break;
		default:
			if (done != 0)
				goto remove;
			break;
		}
		prev = rec;
		continue;
	release:
		/* an actor record: its instance deleted, the record cleared (but its link) */
		next = SP_NEXT(rec);
		done = 1;
		baDeleteActorInstance((struct BActorInstance *)(rec + 0x110));
		memset(rec, 0, 0x2c0);
		SP_NEXT(rec) = next;
	remove:
		SP_NEXT(prev) = SP_NEXT(rec);
		SP_NEXT(rec) = SP_FREE;
		SP_FREE = rec;
		rec = prev;
		/* prev stays: the loop continues with prev's (new) next */
	}
}
