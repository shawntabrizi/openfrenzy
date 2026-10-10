/*
Hopper/unplaced_rules.c  <-  gap functions 0x001472d0-0x00147400 (no source path in the XBE)
(module hopper)

The rules object (0x002afe80, vtable 0x0026eeb4): the characters' models and instances and
the per player counters of the rules (offsets from 0x002afe80):
  +0x04 bkResource *[12]  the 6 characters' models and low-detail models (0x002afe84)
  +0x34 BActorInstance *[12]  their instances (0x002afeb4)
  +0x64 int[4] (0x002afee4)  variation 4: a pole hit the player (1), passed (2)
  +0x74 int[4] (0x002afef4)  frames the score flashes / variation 4: the pole hit cooldown
  +0x84 int[4] (0x002aff04)  variation 4: spin frames / poles passed
  +0x94 int[4] (0x002aff14)  frames since the last hit
  +0xa4 int[4] (0x002aff24)  the score (variation 3: lives, 3)
  +0xb4 int[4] (0x002aff34)  variation 3: the finishing place (3 until out)
  +0xc4 int[4] (0x002aff44)  variation 4: frames until the next pole counts
  +0xd4 int[4] (0x002aff54)  variation 4: knocked off the arm
  +0xe4 int[4], +0xf4 int[4], +0x104 (0x002aff84), +0x108 (0x002aff88) the next place,
  +0x10c int[4] (0x002aff8c)  variation 2: fell off
*/

#include "hopper.h"

/* 0x001472d0  (gap): resets the counters of the four players (variation 3: three lives) and
puts the first four character instances at (0, 1000, 0) */
void HopRules_Start(void *self)
{
	struct BActorInstance *inst;
	int k;

	for (k = 0; k < 4; k++) {
		inst = HOP_AT(self, 0x34 + k * 4, struct BActorInstance *);
		HOP_AF(inst, 0xc0) = 0.0f;
		HOP_AF(inst, 0xc4) = 1000.0f;
		HOP_AF(inst, 0xc8) = 0.0f;
		HOP_AI(self, 0xe4 + k * 4) = 0;
		HOP_AI(self, 0x94 + k * 4) = 0;
		HOP_AI(self, 0xf4 + k * 4) = 0;
		HOP_AI(self, 0x64 + k * 4) = 0;
		HOP_AI(self, 0xb4 + k * 4) = 3;
		HOP_AI(self, 0xc4 + k * 4) = 0;
		HOP_AI(self, 0xd4 + k * 4) = 0;
		HOP_AI(self, 0x84 + k * 4) = 0;
		HOP_AI(self, 0xa4 + k * 4) = hop_Variation == 3 ? 3 : 0;
		HOP_AI(self, 0x74 + k * 4) = 0;
		HOP_AI(self, 0x10c + k * 4) = 0;
		baSetNodeTime(inst, (struct BActorNode *)HOP_AP(inst, 0x134), 0, 1);
		baPrepActorInstance(inst, 1);
	}
	HOP_AI(self, 0x108) = 0;
}

/* 0x00147380  (gap): counts the hit timers (1..60) of the four players down (not in
variation 1) */
void HopRules_Update(void *self)
{
	int k, t;

	for (k = 0; k < 4; k++) {
		if (hop_Variation == 1)
			return;
		t = HOP_AI(self, 0x94 + k * 4);
		if (0 < t && t < 0x3d)
			HOP_AI(self, 0x94 + k * 4) = t - 1;
	}
}

/* 0x00147400  (gap): scalar deleting destructor */
void *HopRules_Destroy(void *self, byte flags)
{
	*(const void *const **)self = vtable_0026eeb4;
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026eeb4  the rules object's vtable */
const void *const vtable_0026eeb4[1] = {
	(const void *)FUN_00147400, /* [0] scalar deleting destructor */
};
