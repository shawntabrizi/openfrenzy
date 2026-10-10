/*
Drillers/unplaced_pads.c  <-  C:\BlitzParty\Game\Src\Drillers\ (file not known)

The input object (one static object at 0x002aaaf8, vtable 0x0026cb20): an input map per
player (in the player record: +0x024 the map, +0x028 "manfaster", +0x02c "manslower",
+0x030 "left+right" / "pad l+r", +0x034 "up+down") and one for everybody (+0x04 the map,
+0x08 "anypressed", +0x0c "reset"); the humans' steering, boost and jump. These gap
functions lie between CDrillers_Resources.cpp (0x0011e1e0) and CDrillers_Particle.cpp
(0x00123550); no assert names their file. The table entry 0x00120000 is a chunk of
0x0011fd50 (Drillers/drillers_chunks.c).
*/

#include "drillers.h"

#define PADS(off, type) DR_AT(self, off, type)
#define DR_CHANNEL_VALUE(ch) DR_I32((ch), 0x14)

/* the boost of player i (float, 0..200) */
#define DR_BOOST(i) DR_VAR(float, 0x002a54b4u + (i) * 4)
/* game +0x88: the speed bonus of variants 3 and 4 */
#define DR_SPEED_BONUS DR_VAR(float, 0x002ffa78u)
/* the latches of the single human's turns */
#define DR_LATCH_LEFT DR_VAR(int, 0x00302034u)
#define DR_LATCH_RIGHT DR_VAR(int, 0x00302030u)
/* tuning: jump (0) or boost (1); the turn speed */
#define DR_TUNE_BOOST DR_VAR(float, 0x002aae1cu)
#define DR_TUNE_TURN DR_VAR(float, 0x002aae18u)
#define DR_ENGINE_CHANNEL(i) DR_VAR(int, 0x002aaa04u + (i) * 4)

/* 0x0026cb20  vtable of the input object (rebuilt from the XBE) */
const void *const vtable_0026cb20[1] = {
	(const void *)DrPads_DeletingDtor, /* +0x00 */
};

/* the bindings of one player's four channels */
static void dr_bind(struct biChannel *ch, unsigned int input, unsigned int other)
{
	biBindInput(ch, input, other);
}

/* 0x0011fd50  Drillers (gap; file not known; the table entry 0x00120000 is its chunk)
The input maps: per player the steering axes ("left+right" / "pad l+r" in variants 3, 4,
"up+down"), "manfaster" and "manslower", bound to the player's pad (and keys); then
"anypressed" and "reset". */
void DrPads_Create(void *self)
{
	struct biChannel *lr;
	struct biChannel *ud;
	struct biChannel *faster;
	struct biChannel *slower;
	int i;

	DR_VAR(int, 0x002aa85cu) = 0;
	DR_VAR(int, 0x002aa860u) = 0;
	DR_VAR(int, 0x002aa864u) = 0;
	DR_VAR(int, 0x002aa868u) = 0;
	for (i = 0; i < 4; i++) {
		DR_PF(i, 0x24, struct BInputMap *) = biCreateMap(0x10);
		DR_PF(i, 0x30, struct biChannel *) = biCreateChannel(DR_PF(i, 0x24, struct biMap *),
			g_DrVariant < 3 ? "left+right" : "pad l+r", 3, 0, 0, 0, 0);
		DR_PF(i, 0x34, struct biChannel *) = biCreateChannel(DR_PF(i, 0x24, struct biMap *),
			"up+down", 3, 0, 0, 0, 0);
		DR_PF(i, 0x28, struct biChannel *) = biCreateChannel(DR_PF(i, 0x24, struct biMap *),
			"manfaster", 1, 0, 0, 0, 0);
		DR_PF(i, 0x2c, struct biChannel *) = biCreateChannel(DR_PF(i, 0x24, struct biMap *),
			"manslower", 1, 0, 0, 0, 0);
		lr = DR_PF(i, 0x30, struct biChannel *);
		ud = DR_PF(i, 0x34, struct biChannel *);
		faster = DR_PF(i, 0x28, struct biChannel *);
		slower = DR_PF(i, 0x2c, struct biChannel *);
		switch (i) {
		case 0:
			dr_bind(lr, 0x9d, 0x9c);
			dr_bind(ud, 0x9b, 0x9a);
			dr_bind(faster, 0x69, 0);
			dr_bind(slower, 0x67, 0);
			dr_bind(lr, 0xc0, 0xbf);
			dr_bind(ud, 0xbe, 0xbd);
			dr_bind(lr, 0xb9, 0);
			dr_bind(ud, 0xba, 0);
			dr_bind(faster, 0xc5, 0);
			dr_bind(slower, 0xc8, 0);
			dr_bind(lr, 0x2a, 0);
			dr_bind(ud, 0x2b, 0);
			dr_bind(faster, 0xa, 0);
			dr_bind(slower, 0xb, 0);
			break;
		case 1:
			dr_bind(lr, 0x100c0, 0x100bf);
			dr_bind(ud, 0x100be, 0x100bd);
			dr_bind(lr, 0x100b9, 0);
			dr_bind(ud, 0x100ba, 0);
			dr_bind(faster, 0x100c5, 0);
			dr_bind(slower, 0x100c8, 0);
			dr_bind(lr, 0x48, 0x45);
			dr_bind(ud, 0x57, 0x5b);
			dr_bind(faster, 0x68, 0);
			dr_bind(slower, 0x66, 0);
			break;
		case 2:
			dr_bind(lr, 0x200c0, 0x200bf);
			dr_bind(ud, 0x200be, 0x200bd);
			dr_bind(lr, 0x200b9, 0);
			dr_bind(ud, 0x200ba, 0);
			dr_bind(faster, 0x200c5, 0);
			dr_bind(slower, 0x200c8, 0);
			dr_bind(lr, 0x4e, 0x50);
			dr_bind(ud, 0x4f, 0x4d);
			dr_bind(faster, 0x4c, 0);
			dr_bind(slower, 0x52, 0);
			break;
		case 3:
			dr_bind(lr, 0x300c0, 0x300bf);
			dr_bind(ud, 0x300be, 0x300bd);
			dr_bind(lr, 0x300b9, 0);
			dr_bind(ud, 0x300ba, 0);
			dr_bind(faster, 0x300c5, 0);
			dr_bind(slower, 0x300c8, 0);
			dr_bind(lr, 0x88, 0x86);
			dr_bind(ud, 0x87, 0x8a);
			dr_bind(faster, 0x83, 0);
			dr_bind(slower, 0x82, 0);
			break;
		}
	}
	PADS(0x4, struct BInputMap *) = biCreateMap(2);
	PADS(0x8, struct biChannel *) = biCreateChannel(PADS(0x4, struct biMap *), "anypressed", 1, 1,
		0, 0, 0);
	biBindInput(PADS(0x8, struct biChannel *), 0x69, 0);
	biBindInput(PADS(0x8, struct biChannel *), 0xe00c5, 0);
	PADS(0xc, struct biChannel *) = biCreateChannel(PADS(0x4, struct biMap *), "reset", 1, 0, 0,
		0, 0);
	biBindInput(PADS(0xc, struct biChannel *), 0x56, 0);
}

/* 0x00120260  Drillers (gap; file not known)
Deletes the five input maps. */
void DrPads_Delete(void *self)
{
	biDeleteMap(PADS(0x4, struct biMap *));
	biDeleteMap(DR_PF(0, 0x24, struct biMap *));
	biDeleteMap(DR_PF(1, 0x24, struct biMap *));
	biDeleteMap(DR_PF(2, 0x24, struct biMap *));
	biDeleteMap(DR_PF(3, 0x24, struct biMap *));
}

/* 0x001202a0  Drillers (gap; file not known; vtable 0x0026cb20 +0x00) */
void *DrPads_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026cb20;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x001202c0  Drillers (gap; file not known)
The humans' controls while playing:
- the boost ("jump(0) boost(1)" set to 1): "manfaster" fills it to 200 (a rumble in
  variants 1, 2), else it drains 48 a frame; the engine pitch follows;
- variants 1, 2: with jump ("manfaster", tuning 0) a jump to the cell two ahead; the speed
  is 256 + boost (Erodeo half); the stick turns to the four directions (a single human
  turns left/right relative, once per push);
- variants 3, 4: the speed is 1024 + the bonus; the stick sets the heading (absolute, or
  relative for a single human); the turn is eased by the turn speed, which tilts the
  driller and sets the engine volume. */
void DrPads_Update(void *self)
{
	struct biChannel *lr;
	struct biChannel *ud;
	float t;
	float d;
	float v;
	int lrv;
	int udv;
	int a;
	int b;
	int h;
	int i;

	(void)self;
	DR_VAR(int, 0x002ffa08u) = 0;
	for (i = 0; i < 4; i++)
		DR_PF(i, 0x704, unsigned char) = DAT_002baed8[i * 10] != 0;
	if (g_DrState != 5)
		return;
	if (DR_SPEED_BONUS < 256.0f)
		DR_SPEED_BONUS = DR_SPEED_BONUS + 2.0f;
	for (i = 0; i < 4; i++) {
		if (DAT_002baed8[i * 10] == 0)
			continue;
		if (DR_TUNE_BOOST == 1.0f) {
			if (DR_PF(i, 0x20, signed char) != 0 && DR_BOOST(i) == 0.0f &&
					DR_CHANNEL_VALUE(DR_PF(i, 0x28, char *)) != 0 && g_DrVariant < 3)
				DrFx_Rumble(i, 0);
			if (DR_CHANNEL_VALUE(DR_PF(i, 0x28, char *)) == 0)
				DR_BOOST(i) = DR_BOOST(i) - 48.0f;
			else
				DR_BOOST(i) = 200.0f;
			if (200.0f < DR_BOOST(i))
				DR_BOOST(i) = 200.0f;
			if (DR_BOOST(i) < 0.0f)
				DR_BOOST(i) = 0.0f;
		}
		if (DR_BOOST(i) == 0.0f)
			bsSetChannelFrequency(DR_ENGINE_CHANNEL(i), 0);
		else
			bsSetChannelFrequency(DR_ENGINE_CHANNEL(i), DR_FTOL(DR_BOOST(i)) * 0x3c + 22000);
		lr = DR_PF(i, 0x30, struct biChannel *);
		ud = DR_PF(i, 0x34, struct biChannel *);
		if (g_DrVariant < 3) {
			if (DR_TUNE_BOOST == 0.0f && DR_CHANNEL_VALUE(DR_PF(i, 0x28, char *)) != 0 &&
					DR_PF(i, 0x708, float) == 0.0f && DR_TUNE_BOOST == 0.0f) {
				DR_PF(i, 0x708, float) = 1.0f;
				a = DR_FTOL(DR_PF(i, 0x4, float) * 0.0009765625f);
				b = DR_FTOL(DR_PF(i, 0x8, float) * 0.0009765625f);
				h = DR_FTOL(DR_PF(i, 0xc, float));
				if (h == 0xb4)
					b -= 2;
				else if (h == 0)
					b += 2;
				else if (h == 0x5a)
					a += 2;
				else if (h == 0x10e)
					a -= 2;
				DR_PF(i, 0x718, int) = b * 0x400 + 0x200;
				DR_PF(i, 0x714, int) = a * 0x400 + 0x200;
			}
			DR_PF(i, 0x18, float) = DR_BOOST(i) + 256.0f;
			if (g_DrVariant == 2)
				DR_PF(i, 0x18, float) = (DR_BOOST(i) + 256.0f) * 0.5f;
			if (dr_fbits(DR_PF(i, 0x708, float)) == 0x3f800000u || DR_VAR(signed char, 0x002ffa74u) != 0)
				continue;
			lrv = DR_CHANNEL_VALUE(lr);
			udv = DR_CHANNEL_VALUE(ud);
			if (DR_VAR(int, 0x002ffaa0u) == 0) {
				if ((float)lrv < -24570.0f)
					DR_PF(i, 0xc, float) = 90.0f;
				if (24570.0f < (float)lrv)
					DR_PF(i, 0xc, float) = 270.0f;
				if ((float)udv < -24570.0f)
					DR_PF(i, 0xc, float) = 180.0f;
				if (24570.0f < (float)udv)
					DR_PF(i, 0xc, float) = 0.0f;
			} else {
				if (DR_LATCH_LEFT == 0 && (float)lrv < -24570.0f) {
					DR_LATCH_LEFT = 1;
					DR_PF(i, 0xc, float) = DR_PF(i, 0xc, float) - 90.0f;
				}
				if (DR_LATCH_RIGHT == 0 && 24570.0f < (float)lrv) {
					DR_LATCH_RIGHT = 1;
					DR_PF(i, 0xc, float) = DR_PF(i, 0xc, float) + 90.0f;
				}
				if (-24570.0f < (float)lrv)
					DR_LATCH_LEFT = 0;
				if ((float)lrv < 24570.0f)
					DR_LATCH_RIGHT = 0;
				if (270.0f < DR_PF(i, 0xc, float))
					DR_PF(i, 0xc, float) = 0.0f;
				if (DR_PF(i, 0xc, float) < 0.0f)
					DR_PF(i, 0xc, float) = 270.0f;
			}
			continue;
		}
		DR_PF(i, 0x18, float) = DR_SPEED_BONUS + 1024.0f;
		lrv = DR_CHANNEL_VALUE(lr);
		if ((lrv == 0 && DR_CHANNEL_VALUE(ud) == 0) || DR_VAR(signed char, 0x002ffa74u) != 0)
			continue;
		if (DR_VAR(int, 0x002ffaa0u) == 0) {
			DR_PF(i, 0x10, float) = -atan2f((float)lrv, (float)DR_CHANNEL_VALUE(ud)) * 57.29578f +
				DR_VAR(float, 0x002aa84cu);
		} else {
			v = (float)lrv * 0.00024420026f + DR_PF(i, 0x10, float);
			DR_PF(i, 0x10, float) = v;
			DR_PF(i, 0xc, float) = v;
		}
		a = DR_FTOL(DR_PF(i, 0xc, float)) % 0x168;
		DR_PF(i, 0xc, float) = (float)a;
		b = DR_FTOL(DR_PF(i, 0x10, float)) % 0x168;
		DR_PF(i, 0x10, float) = (float)b;
		t = DR_TUNE_TURN - sqrtf((float)DR_CHANNEL_VALUE(lr) * (float)DR_CHANNEL_VALUE(lr) +
			(float)DR_CHANNEL_VALUE(ud) * (float)DR_CHANNEL_VALUE(ud)) / (DR_TUNE_TURN * 32.0f);
		if (t < 4.0f)
			t = 4.0f;
		d = DrAi_AngleDiff(DR_PTR(void, 0x002aadf8u), (float)a, (float)b) / t;
		h = DR_FTOL(d);
		h = abs(h) * 0x20 + 0x82;
		if (h < 0x100) {
			if (h < 0x82)
				h = 0x82;
		} else {
			h = 0xff;
		}
		bsSetChannelVolume(DR_ENGINE_CHANNEL(i), (unsigned short)h);
		v = (d - DR_PF(i, 0x710, float)) * 0.25f + DR_PF(i, 0x710, float);
		DR_PF(i, 0x710, float) = v;
		if (v < -45.0f)
			DR_PF(i, 0x710, float) = -45.0f;
		if (45.0f < DR_PF(i, 0x710, float))
			DR_PF(i, 0x710, float) = 45.0f;
		if (DR_VAR(int, 0x002ffaa0u) == 0)
			DR_PF(i, 0xc, float) = DR_PF(i, 0xc, float) - d;
	}
}
