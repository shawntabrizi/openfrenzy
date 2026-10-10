/*
UNPLACED_HOPPER.C  <-  gap functions 0x0014c9a0-0x0014d890 (no source path in the XBE)

The source map puts these functions in Meta (they follow the last Hopper function), but
they are Hopper code: every one reads Hopper's globals (game phase DAT_003027a0, the
player table DAT_00302b48, DAT_003027bc, DAT_002aec00), and Hopper's constructor
0x00145cb0 stores the state tables that hold them (0x002b04a8 actions, 0x002b046c
transitions; both tables are Hopper data). The render callbacks 0x0014c9a0-0x0014ca30
sit in Hopper's shader tables at 0x0026f05c / 0x0026f088.

They stay in meta_a (the module table assigns them here); the hopper module should take
them over. Original file probably C:\BlitzParty\Game\Src\Hopper\<player>.cpp.

`ctl` is a character controller (see meta_a.h); the Hopper player adds:
  +0x1084 short target player (-1 none)   +0x1092 short flag
  +0x10b0 float speed factor              +0x10d8 float time limit
  +0x10e4 int running flag                 +0x10e8 float stick threshold
  +0x10f8 float height tolerance           +0x1104 / +0x1108 float time window (attack)
  +0x110c / +0x1110 float time window (push)   +0x1120 int landing animation
*/

#include "meta_a.h"

/* charlib FUN_000af690: play a character animation
   (character, animation, layer, loop, speed, blend time, flags) */
#define MA_CHAR_ANIM(ch, anim, layer, loop, speed, blend, flags) \
	FUN_000af690((int)(ch), (anim), (layer), (loop), (speed), (blend), (flags))

/* charlib FUN_000af580: blend to an animation (character, animation, float blend time) */
static void ma_char_blend(int ch, int anim, float blend)
{
	MA_DUAL(FUN_000af580, void (*)(int, int, undefined4), void (*)(int, int, float),
		(ch, anim, ma_fbits(blend)), (ch, anim, blend));
}

/* charlib FUN_000b0320 (thiscall controller): play a voice sample of the character (kind, volume, ...) */
#define MA_CHAR_VOICE(ctl, kind, volume) FUN_000b0320((int)(ctl), (kind), (volume), 0, 0, 0xffffffff, 0)

static void ma_zero_velocity(void *ctl)
{
	MA_I32(ctl, 0x84) = 0;
	MA_I32(ctl, 0x80) = 0;
	MA_I32(ctl, 0x7c) = 0;
}

/* velocity -= velocity * k */
static void ma_damp_velocity(void *ctl, float k)
{
	MA_F32(ctl, 0x7c) = -MA_F32(ctl, 0x7c) * k + MA_F32(ctl, 0x7c);
	MA_F32(ctl, 0x80) = -MA_F32(ctl, 0x80) * k + MA_F32(ctl, 0x80);
	MA_F32(ctl, 0x84) = -MA_F32(ctl, 0x84) * k + MA_F32(ctl, 0x84);
}

/* ---------- render callbacks (Hopper shader tables) */

/* 0x0014c9a0  (Hopper) */
void FUN_0014c9a0(void)
{
	((void (*)(int, int, int))FUN_00046cb0)(6, 1, 0);
	((void (*)(int, int, int))FUN_00046cb0)(10, 1, 0);
	((void (*)(int, int, int))FUN_00046cb0)(5, 0, 0);
}

/* 0x0014c9d0  (Hopper) */
void FUN_0014c9d0(void)
{
	FUN_00046d00(10);
	FUN_00046d00(6);
	FUN_00046d00(5);
}

/* 0x0014c9f0  (Hopper) vertex shader constant c1 = (16, 16, 64, 64) */
void FUN_0014c9f0(int param_1)
{
	float constant[4];

	constant[0] = 16.0f;
	constant[1] = 16.0f;
	constant[2] = 64.0f;
	constant[3] = 64.0f;
	D3DDevice_SetVertexShaderConstant(MA_I32(param_1, 8), constant, 1);
}

/* 0x0014ca30  (Hopper) */
void FUN_0014ca30(int param_1)
{
	D3DDevice_SetVertexShaderConstant(MA_I32(param_1, 8), DAT_00304548, 1);
}

/* ---------- action table 0x002b04a8 (index = state) */

/* 0x0014ca50  (Hopper) action [1] */
undefined4 FUN_0014ca50(int ctl, int entered)
{
	if (DAT_003027bc == 0 || MA_I16(ctl, 0x36) != 0xc) {
		if (entered != 0) {
			if (DAT_003027bc == 0) {
				MA_CHAR_ANIM(MA_CHAR(ctl), 0, 1, 0, 1.0f, 0.1f, 1);
			} else {
				MA_CHAR_ANIM(MA_CHAR(ctl), 0x1c, 0, 0, 1.0f, 0.2f, 1);
				MA_CHAR_ANIM(MA_CHAR(ctl), 0x24, 1, 1, 1.0f, 0.03f, 1);
			}
		}
		ma_zero_velocity((void *)ctl);
	}
	return 0;
}

/* ---------- transition table 0x002b046c (index = state) */

/* 0x0014caf0  (Hopper) transition [0] */
undefined4 FUN_0014caf0(int ctl)
{
	short state;

	if (MA_I32(MA_PTR(ctl, 0x6c), 0x14) == 0) {
		state = MA_I16(ctl, 0x36);
		if (state != 0xd || DAT_003027a0 < 3) {
			if (state == 2) {
				if (MA_F32(ctl, 0x54) < MA_F32(ctl, 0x10e8))
					return 1;
			} else if (state != 0xc && (state != 0xd || DAT_003027a0 != 4) &&
				MA_I16(MA_CHAR(ctl), 0x234) != 0) {
				return 1;
			}
		}
	}
	return 0;
}

/* 0x0014cb50  (Hopper) transition [2] */
undefined4 FUN_0014cb50(int ctl)
{
	if (DAT_002aec00 == 5) {
		switch (MA_U16(ctl, 0x36)) {
		case 0:
		case 4:
		case 0xb:
			if (MA_F32(ctl, 0x10e8) <= MA_F32(ctl, 0x54))
				return 1;
			break;
		case 3:
			break;
		default:
			if (MA_F32(ctl, 0x10e8) <= MA_F32(ctl, 0x54) && MA_I16(MA_CHAR(ctl), 0x234) != 0)
				return 1;
			break;
		}
	}
	return 0;
}

/* 0x0014cbd0  (Hopper) transition [3] */
undefined1 FUN_0014cbd0(int ctl)
{
	if (DAT_003027bc == 0 &&
		(DAT_003027a0 == 2 || MA_I32(MA_CHAR(ctl), 0x214) != 0) &&
		MA_I16(ctl, 0x36) != 3) {
		return MA_I32(MA_PTR(ctl, 0x68), 0x14) != 0;
	}
	return 0;
}

/* 0x0014cc10  (Hopper) transition [5]: pick a player in reach to attack */
undefined4 FUN_0014cc10(int ctl)
{
	short other_state;
	int ch;
	int other_ch;
	unsigned int player;
	float heading;
	float turn;
	float distance;

	if (DAT_003027a0 != 1 && DAT_003027a0 != 4 && MA_I32(MA_PTR(ctl, 0x6c), 0x14) != 0) {
		player = 0;
		if (0 < DAT_002bae94) {
			do {
				ch = MA_CHAR(ctl);
				if (player != MA_U16(ch, 0x230)) {
					other_state = MA_I16(DAT_00302b48[player], 0x36);
					if (other_state != 10 && other_state != 0xd) {
						other_ch = MA_I32(DAT_00302b48[player], 0x30);
						if (fabsf(MA_F32(ch, 0xd4) - MA_F32(other_ch, 0xd4)) < MA_F32(ctl, 0x10f8)) {
							heading = CLMath_HeadingTo(MA_FP(ch, 0xd0), MA_FP(other_ch, 0xd0));
							turn = CLMath_AngleDistance(heading, MA_F32(MA_CHAR(ctl), 0x1e8));
							/* 0x002640f8: degrees to radians */
							if ((double)turn < (double)DAT_002b074c * (double)0.017453292f) {
								distance = FUN_00051500(MA_FP(MA_CHAR(ctl), 0xd0),
									MA_FP(MA_I32(DAT_00302b48[player], 0x30), 0xd0));
								/* 0x00253850 = 1.0, 0x00257c78 = 0.8 */
								if (((double)turn + 1.0) * (double)distance <
									(double)*(float *)((char *)&DAT_00290728 +
										DAT_002baed4[MA_U16(MA_CHAR(ctl), 0x230) * 10] * 0x60) *
										(double)0.8f) {
									MA_I16(ctl, 0x1084) = (short)player;
									MA_F32(MA_CHAR(ctl), 0x1d8) = heading;
									return 1;
								}
							}
						}
					}
				}
				player++;
			} while ((int)player < DAT_002bae94);
		}
		MA_U16(ctl, 0x1084) = 0xffff;
		return 1;
	}
	return 0;
}

/* 0x0014cda0  (Hopper) transition [14] */
undefined4 FUN_0014cda0(int ctl)
{
	if (DAT_003027a0 != 2 && DAT_003027a0 != 3 && DAT_003027bc == 0 &&
		MA_I32(MA_PTR(ctl, 0x6c), 0x14) != 0) {
		if (DAT_003027a0 == 4)
			bsPlaySample((struct BSample *)DAT_002ae9c4, 0xfa, 0, 0, 0);
		return 1;
	}
	return 0;
}

/* 0x0014cdf0  (Hopper) action [4] */
undefined4 FUN_0014cdf0(int ctl, int entered)
{
	if (entered != 0) {
		MA_CHAR_ANIM(MA_CHAR(ctl), 5, 0, 0, 1.0f, 0.3f, 1);
		if (DAT_003027a0 != 4)
			MA_CHAR_VOICE(ctl, 1, DAT_00293928);
	}
	ma_zero_velocity((void *)ctl);
	return 0;
}

/* 0x0014ce60  (Hopper) action [10] */
undefined4 FUN_0014ce60(int ctl, int entered)
{
	if (entered != 0) {
		MA_CHAR_VOICE(ctl, 2, DAT_00293928);
		MA_CHAR_ANIM(MA_CHAR(ctl), 0x11, 0, 0, DAT_002b05a4, 0.1f, 1);
	}
	if (DAT_003027a0 == 3) {
		ma_damp_velocity((void *)ctl, 0.2f); /* 0x00257c7c */
		return 0;
	}
	if (DAT_003027a0 != 2)
		ma_zero_velocity((void *)ctl);
	return 0;
}

/* 0x0014cf20  (Hopper) action [14] */
undefined4 FUN_0014cf20(int ctl, int entered)
{
	float speed;

	if (entered != 0) {
		if (DAT_003027a0 == 4)
			speed = DAT_002aed74 * 0.2f + 0.65f; /* 0x00257c7c, 0x00265740 */
		else
			speed = 1.0f;
		MA_CHAR_ANIM(MA_CHAR(ctl), 5, 0, 0, speed, 0.1f, 1);
	}
	ma_damp_velocity((void *)ctl, 0.25f); /* 0x00257b88 */
	return 0;
}

/* 0x0014cfc0  (Hopper) action [13]: landing */
undefined4 FUN_0014cfc0(int ctl, int entered)
{
	int anim;

	MA_F32(ctl, 0x10d8) = DAT_002b07b8 + DAT_002b07b8;
	if (entered != 0) {
		if (MA_I32(DAT_00302b48[MA_U16(MA_CHAR(ctl), 0x230)], 0x1194) == 0 || DAT_003027a0 != 4)
			anim = 0x13;
		else
			anim = 0x14;
		MA_CHAR_ANIM(MA_CHAR(ctl), anim, 0, 0, DAT_002b05a8, 0.1f, 1);
		MA_I32(ctl, 0x1120) = 1;
	}
	if (DAT_003027a0 != 4)
		ma_damp_velocity((void *)ctl, 0.2f); /* 0x00257c7c */
	if (DAT_003027a0 < 3 && -1 < MA_I32(ctl, 0x1120) &&
		MA_F32(ctl, 0x10d8) <= MA_F32(ctl, 0x40)) {
		anim = MA_I32(ctl, 0x1120) == 0 ? 0x16 : 0x15;
		MA_CHAR_ANIM(MA_CHAR(ctl), anim, 0, 0, 1.0f, 0.1f, 1);
		MA_I32(ctl, 0x1120) = -1;
	}
	return 0;
}

/* 0x0014d0f0  (Hopper) action [12] */
undefined4 FUN_0014d0f0(int ctl, int entered)
{
	if (DAT_003027a0 != 4 || DAT_002b0370 < 2) {
		if (entered != 0) {
			MA_CHAR_ANIM(MA_CHAR(ctl), 0x1b, 0, 0, 1.0f, 0.1f, 1);
			MA_CHAR_ANIM(MA_CHAR(ctl), 0x23, 1, 1, 1.0f, 0.03f, 1);
		}
		ma_zero_velocity((void *)ctl);
	}
	return 0;
}

/* 0x0014d170  (Hopper) action [0]: idle */
undefined4 FUN_0014d170(int ctl, int entered)
{
	struct BAnim *anim;
	int ch;

	if (entered == 0) {
		if (4 < DAT_002aec00)
			goto done;
		anim = NULL;
		ch = MA_CHAR(ctl);
		/* a fidget: 5 in 256 frames, when the current animation is the idle one */
		if ((baIsAnimating((struct BActorInstance *)(ch + 0x10),
				(struct BActorNode *)MA_PTR(ch, 0x144), 0, &anim) != 0 &&
				(int)anim != MA_I32(MA_CHAR(ctl), 0x264)) ||
			4 < (ma_rand() & 0xff))
			goto done;
		MA_CHAR_ANIM(MA_CHAR(ctl), 0x21, 0, 0, 0.5f, 0.01f, 1);
		MA_CHAR_ANIM(MA_CHAR(ctl), 0, 1, 1, 0.5f, 0.03f, 1);
	} else {
		if (DAT_003027bc != 0) {
			MA_CHAR_ANIM(MA_CHAR(ctl), 0x1c, 0, 0, 1.0f, 0.2f, 1);
			MA_CHAR_ANIM(MA_CHAR(ctl), 0x24, 1, 1, 1.0f, 0.03f, 1);
		} else {
			MA_CHAR_ANIM(MA_CHAR(ctl), 0, 1, 1, 1.0f, 0.03f, 1);
			MA_CHAR_ANIM(MA_CHAR(ctl), 0, 1, 0, 1.0f, 0.1f, 1);
		}
	}
done:
	ma_zero_velocity((void *)ctl);
	return 0;
}

/* 0x0014d2a0  (Hopper) action [2]: run */
undefined4 FUN_0014d2a0(int ctl, int entered)
{
	int ch;
	float speed;

	if (DAT_003027bc != 0)
		return 0;
	if (DAT_003027a0 == 2 && 0 < DAT_002afef4[MA_U16(MA_CHAR(ctl), 0x230)])
		return 0;
	if (entered != 0) {
		MA_I32(ctl, 0x10e4) = 1;
		if (DAT_00305650 == 0 || MA_I16(ctl, 0x38) != 4) {
			MA_CHAR_ANIM(MA_CHAR(ctl), 1, 1, 0, 0.0f, DAT_0028fda8 * 0.1f, 1); /* 0x002536cc */
		} else {
			baFlushAnimations((void *)(MA_CHAR(ctl) + 0x10),
				(struct BActorNode *)MA_PTR(MA_CHAR(ctl), 0x144), 1, 0);
			MA_I16(MA_CHAR(ctl), 0x236) = 0;
			ch = MA_CHAR(ctl);
			baQueueBlendedAnimation((struct BActorInstance *)(ch + 0x10),
				(struct BActorNode *)MA_PTR(ch, 0x144), (struct BAnim *)MA_PTR(ch, 0x2c4),
				(struct BAnim *)MA_PTR(ch, 0x278), DAT_002b0770, DAT_002b0774, -1, -1,
				DAT_002b0778, 3, 0, 0);
		}
	}
	if (DAT_00305650 != 0 && MA_I16(ctl, 0x38) == 4 && MA_I32(ctl, 0x3c) == DAT_002b0780) {
		baChangeAnimation((struct BActorNode *)MA_PTR(MA_CHAR(ctl), 0x144), 1, DAT_002b0770,
			DAT_002b0774, DAT_002b077c, 0.0f, 7);
	}
	if (DAT_003027a0 == 4)
		MA_F32(MA_CHAR(ctl), 0x254) = DAT_002aed74 * 3.5f * MA_F32(MA_CHAR(ctl), 0x254); /* 0x00268a24 */
	if (MA_I32(ctl, 0x10e4) != 0) {
		if (DAT_002b059c < MA_F32(MA_CHAR(ctl), 0x254)) {
			ma_char_blend(MA_CHAR(ctl), 0x18, 0.05f);
			MA_I32(ctl, 0x10e4) = 0;
			goto scaled;
		}
		if (MA_I32(ctl, 0x10e4) != 0)
			goto scaled;
	}
	if (MA_F32(MA_CHAR(ctl), 0x254) < DAT_002b05a0) {
		ma_char_blend(MA_CHAR(ctl), 1, 0.05f);
		MA_I32(ctl, 0x10e4) = 1;
	}
scaled:
	if (DAT_003027a0 == 4)
		MA_F32(MA_CHAR(ctl), 0x254) = MA_F32(MA_CHAR(ctl), 0x254) * 1.5f; /* 0x002538a0 */
	ch = MA_CHAR(ctl);
	MA_F32(ch, 0x23c) = MA_F32(MA_PTR(ch, 0x324), 0x6a0) * MA_F32(ch, 0x254);
	speed = MA_F32(ctl, 0x10b0) * MA_F32(ctl, 0x54);
	MA_F32(ctl, 0x7c) = speed * MA_F32(ctl, 0x44);
	MA_F32(ctl, 0x80) = speed * MA_F32(ctl, 0x48);
	MA_F32(ctl, 0x84) = speed * MA_F32(ctl, 0x4c);
	MA_F32(MA_CHAR(ctl), 0x1d8) = CLMath_HeadingXZ(MA_FP(ctl, 0x7c));
	if (DAT_003027a0 == 4) {
		ch = MA_CHAR(ctl);
		if (DAT_002aed74 == 0.0f) { /* 0x002536e0 */
			MA_F32(ch, 0x254) = MA_F32(ch, 0x254) * 1e9f; /* 0x0026f0c0 */
			return 0;
		}
		MA_F32(ch, 0x254) = MA_F32(ch, 0x254) / (DAT_002aed74 * 5.25f); /* 0x0026f0bc */
	}
	return 0;
}

/* 0x0014d520  (Hopper) action [3]: jump */
undefined4 FUN_0014d520(int ctl, int entered)
{
	int ch;
	int platform;
	float speed;

	if (DAT_003027a0 != 2 || DAT_002afef4[MA_U16(MA_CHAR(ctl), 0x230)] < 1) {
		if (entered != 0) {
			MA_F32(MA_CHAR(ctl), 0x198) = DAT_00292c98 * 0.95f; /* 0x00268d2c */
			ch = MA_CHAR(ctl);
			platform = MA_I32(MA_PTR(ch, 0x18c), 0x1b0);
			if (platform != 0) {
				/* take over the velocity of the platform we stand on */
				MA_F32(ch, 0x194) = MA_F32(platform, 0x1a4) + MA_F32(ch, 0x194);
				ch = MA_CHAR(ctl);
				MA_F32(ch, 0x198) = MA_F32(MA_PTR(MA_PTR(ch, 0x18c), 0x1b0), 0x1a8) + MA_F32(ch, 0x198);
				ch = MA_CHAR(ctl);
				MA_F32(ch, 0x19c) = MA_F32(MA_PTR(MA_PTR(ch, 0x18c), 0x1b0), 0x1ac) + MA_F32(ch, 0x19c);
				MA_I32(MA_PTR(MA_CHAR(ctl), 0x18c), 0x1b0) = 0;
			}
			MA_CHAR_ANIM(MA_CHAR(ctl), 2, 0, 0, 1.0f, 0.3f, 1);
			if (MA_F32(ctl, 0x10e8) <= MA_F32(ctl, 0x54))
				MA_F32(MA_CHAR(ctl), 0x1d8) = CLMath_HeadingXZ(MA_FP(ctl, 0x7c));
			if (MA_I16(MA_CHAR(ctl), 0x230) == 0) {
				DAT_002f2894 = MA_F32(MA_CHAR(ctl), 0xd4);
				DAT_002f2890 = 0.0f;
			}
			CLSamples_PlayFromSet(*(char *)&DAT_002baed4[MA_U16(MA_CHAR(ctl), 0x230) * 10], 0,
				NULL, DAT_00293928, 0, -1, 0);
			MA_F32(MA_CHAR(ctl), 0x238) = 1.0f;
		}
		speed = MA_F32(ctl, 0x10b0) * MA_F32(ctl, 0x54) * 1.1764705f; /* 0x0026f0c4 */
		MA_F32(ctl, 0x7c) = speed * MA_F32(ctl, 0x44);
		MA_F32(ctl, 0x80) = speed * MA_F32(ctl, 0x48);
		MA_F32(ctl, 0x84) = speed * MA_F32(ctl, 0x4c);
	}
	return 0;
}

/* 0x0014d6c0  (Hopper) action [5]: attack lunge */
undefined4 FUN_0014d6c0(int *ctl, int entered)
{
	unsigned int kind;
	int sample;
	float heading;
	float k;
	float t;

	if (DAT_003027bc == 0) {
		if (entered != 0) {
			kind = (unsigned int)ma_rand() % 3;
			if (kind == 0)
				sample = (ma_rand() & 1) + 0xf;
			else
				sample = (ma_rand() & 1) + 0x11;
			CLSamples_PlayGeneric(sample, 0x7f, 0, 0, NULL);
			MA_CHAR_ANIM(MA_CHAR(ctl), kind + 0xd, 0, 0, DAT_002b079c[kind], DAT_002b078c[kind], 1);
			MA_I16(ctl, 0x1092) = 0;
		}
		t = MA_F32(ctl, 0x40);
		if (t > DAT_002b0788 || t < DAT_00305654) {
			ma_zero_velocity(ctl);
		} else {
			FUN_00051170(MA_FP(ctl, 0x7c), DAT_0028fdd8, MA_F32(MA_CHAR(ctl), 0x1d8));
			k = DAT_002b0784;
			MA_F32(ctl, 0x7c) = DAT_002b0784 * MA_F32(ctl, 0x7c);
			MA_F32(ctl, 0x80) = k * MA_F32(ctl, 0x80);
			MA_F32(ctl, 0x84) = k * MA_F32(ctl, 0x84);
		}
		if (MA_I16(ctl, 0x1084) != -1) {
			heading = CLMath_HeadingTo(MA_FP(MA_CHAR(ctl), 0xd0),
				MA_FP(MA_I32(DAT_00302b48[MA_I16(ctl, 0x1084)], 0x30), 0xd0));
			MA_F32(MA_CHAR(ctl), 0x1d8) = heading;
			if (entered != 0)
				MA_F32(MA_CHAR(ctl), 0x1e8) = heading;
		}
		if (MA_F32(ctl, 0x1104) <= MA_F32(ctl, 0x40) &&
			(MA_F32(ctl, 0x40) <= MA_F32(ctl, 0x1108) || MA_I16(ctl, 0x1092) == 0)) {
			MA_VFN(ctl, 0x10, int (*)(void *))(ctl);
		}
	}
	return 0;
}

/* 0x0014d890  (Hopper) action [6]: push the nearest player in front */
undefined4 FUN_0014d890(int *ctl, int entered)
{
	unsigned int best;
	unsigned int player;
	float value;
	float best_turn;

	best = 0xffffffff;
	best_turn = 1.5707963f;
	if (entered != 0) {
		MA_CHAR_ANIM(MA_CHAR(ctl), 8, 0, 0, 1.0f, 0.1f, 1);
		MA_I16(ctl, 0x1092) = 0;
		player = 0;
		do {
			if (player != MA_U16(MA_CHAR(ctl), 0x230) &&
				MA_I16(DAT_00302b48[player], 0x36) != 0xd) {
				value = CLVec3_DistanceSqXZ(MA_FP(MA_CHAR(ctl), 0xd0),
					MA_FP(MA_I32(DAT_00302b48[player], 0x30), 0xd0));
				if ((double)value < (double)DAT_002b07b0 * (double)DAT_002b07b0) {
					value = CLMath_HeadingTo(MA_FP(MA_CHAR(ctl), 0xd0),
						MA_FP(MA_I32(DAT_00302b48[player], 0x30), 0xd0));
					value = CLMath_AngleDistance(value, MA_F32(MA_CHAR(ctl), 0x1e8));
					if (best_turn == -1.0f || value < best_turn) { /* 0x00253854 */
						best_turn = value;
						best = player;
					}
				}
			}
			player++;
		} while ((int)player < 4);
		if (best == 0xffffffff) {
			MA_U16(ctl, 0x1084) = 0xffff;
		} else {
			MA_F32(MA_CHAR(ctl), 0x1d8) = CLMath_HeadingTo(MA_FP(MA_CHAR(ctl), 0xd0),
				MA_FP(MA_I32(DAT_00302b48[best], 0x30), 0xd0));
			MA_I16(ctl, 0x1084) = (short)best;
		}
	}
	MA_I32(ctl, 0x80) = 0;
	if (MA_I16(ctl, 0x1084) == -2) {
		MA_I32(ctl, 0x7c) = 0;
		MA_I32(ctl, 0x84) = 0;
		MA_F32(ctl, 0x88) = 1.0f;
	} else {
		MA_I32(ctl, 0x7c) = 0;
		MA_F32(ctl, 0x84) = DAT_002b07ac;
		FUN_00051170(MA_FP(ctl, 0x7c), MA_FP(ctl, 0x7c), MA_F32(MA_CHAR(ctl), 0x1d8));
	}
	if (MA_I16(ctl, 0x1084) != -2 &&
		MA_F32(ctl, 0x110c) <= MA_F32(ctl, 0x40) &&
		(MA_F32(ctl, 0x40) <= MA_F32(ctl, 0x1110) || MA_I16(ctl, 0x1092) == 0) &&
		MA_VFN(ctl, 0x10, int (*)(void *))(ctl) != 0) {
		MA_U16(ctl, 0x1084) = 0xfffe;
	}
	return 0;
}
