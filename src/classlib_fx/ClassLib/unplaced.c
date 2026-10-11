/*
ClassLib/unplaced.c  (classlib_fx)

ClassLib functions of block 2 whose original source file is not known. Gap functions
whose file could be inferred from the data they use are in that file instead
(CProjector.c, RefBot.c, ...), with a "file inferred" note.
*/

#include "classlib_fx.h"

/* ======================================================================== animated textures
0x00082be0-0x00082f60 (between the BumperCars block and CProjector.cpp) */

/* 0x00082be0  ClassLib (gap)
Advances the frame by speed * scale (frame-rate scaled); with `apply`, shows that frame. */
void AnimTex_Advance(CAnimTex *self, float scale, int apply)
{
	self->frame = DAT_0028fda8 * self->speed * scale + self->frame;
	if (apply != 0)
		FUN_00041600((struct bdTexture *)self->texture, CLF_FTOL(self->frame));
}

/* 0x00082c20  ClassLib (gap)
AnimTex_Advance for every texture of a list (inlined). */
void AnimTex_AdvanceList(CLFList *list, float scale, int apply)
{
	CAnimTex *t;

	for (t = CLF_ListNext(list, NULL); t != NULL; t = CLF_ListNext(list, t)) {
		t->frame = DAT_0028fda8 * t->speed * scale + t->frame;
		if (apply != 0)
			FUN_00041600((struct bdTexture *)t->texture, CLF_FTOL(t->frame));
	}
}

/* 0x00082c80  ClassLib (gap)
Shows the current frame of every texture of a list. */
void AnimTex_ApplyList(CLFList *list)
{
	CAnimTex *t;

	for (t = CLF_ListNext(list, NULL); t != NULL; t = CLF_ListNext(list, t))
		FUN_00041600((struct bdTexture *)t->texture, CLF_FTOL(t->frame));
}

/* 0x00082cc0  ClassLib (gap)
Sets and shows frame `frame` on every texture of a list. */
void AnimTex_SetFrameList(CLFList *list, int frame)
{
	CAnimTex *t;

	t = CLF_ListNext(list, NULL);
	if (t != NULL) {
		do {
			t->frame = (float)frame;
			FUN_00041600((struct bdTexture *)t->texture, frame);
			t = CLF_ListNext(list, t);
		} while (t != NULL);
	}
}

/* 0x00082d10  ClassLib (gap)
Writes a grey version of texture `src` into texture `dst` (they may be the same): the
average of R, G and B, alpha kept. Formats 1/2 are 32-bit ARGB, 3/4 16-bit ARGB1555;
other formats are left unchanged. Rows use the destination width. */
void Texture_CopyGreyscale(int dst, int src)
{
	uint *d32;
	uint *s32;
	ushort *d16;
	ushort *s16;
	uint px;
	uint grey;
	int width;
	int height;
	int pitch;
	int format;
	int srcWidth;
	int srcHeight;
	int srcPitch;
	int srcFormat;
	void *dpix;
	void *spix;
	int x;

	dpix = CLF_LockTexture(dst, 0, &width, &height, &pitch, &format, NULL);
	if (dst == src) {
		srcWidth = width;
		srcHeight = height;
		srcPitch = pitch;
		spix = dpix;
	} else {
		spix = CLF_LockTexture(src, 0, &srcWidth, &srcHeight, &srcPitch, &srcFormat, NULL);
	}
	(void)srcWidth;
	(void)srcHeight;
	if (format == 1 || format == 2) {
		d32 = dpix;
		s32 = spix;
		while (height != 0) {
			height = height - 1;
			for (x = width; x != 0; x = x - 1) {
				px = *s32;
				grey = ((px >> 16 & 0xff) + (px >> 8 & 0xff) + (px & 0xff)) / 3;
				*d32 = (grey << 16) | (grey << 8) | (px & 0xff000000) | grey;
				s32 = s32 + 1;
				d32 = d32 + 1;
			}
			d32 = d32 + (pitch / 4 - width);
			s32 = s32 + (srcPitch / 4 - width);
		}
	} else if (format == 3 || format == 4) {
		d16 = dpix;
		s16 = spix;
		while (height != 0) {
			for (x = width; x != 0; x = x - 1) {
				px = *s16;
				grey = ((px >> 10 & 0x1f) + (px >> 5 & 0x1f) + (px & 0x1f)) / 3;
				*d16 = (ushort)(((grey << 5 | grey) << 5) | (px & 0x8000) | grey);
				s16 = s16 + 1;
				d16 = d16 + 1;
			}
			d16 = d16 + (pitch / 2 - width);
			s16 = s16 + (srcPitch / 2 - width);
			height = height - 1;
		}
	}
	CLF_UnlockTexture(dst);
	if (dst != src)
		CLF_UnlockTexture(src);
}

/* 0x00082f10  ClassLib (gap)
Deletes every texture of a list. */
void AnimTex_DeleteList(CLFList *list)
{
	CAnimTex *t;
	void *prev;

	t = CLF_ListNext(list, NULL);
	while (t != NULL) {
		if (t->list == NULL)
			prev = NULL;
		else
			prev = CLF_ListRemove(t->list, t);
		CLF_Free(t);
		t = CLF_ListNext(list, prev);
	}
}

/* 0x00082f60  ClassLib (gap)
Constructor: finds texture `name` (frees itself and returns when missing) and links
into `list` when given. */
CAnimTex *AnimTex_Init(CAnimTex *self, char *name, float speed, CLFList *list)
{
	self->texture = CLF_FindTexture(0, name, 0, 0xdefa);
	if (self->texture == 0) {
		CLF_Printf("***WARNING*** failed to find texture %s\n", name);
		CLF_Free(self);
		return self;
	}
	self->speed = speed;
	self->frame = 0.0f;
	if (list != NULL) {
		self->list = list;
		CLF_ListInsert(list, self, 0);
	}
	return self;
}

/* ======================================================================== point sprites
0x00084f30-0x00085060 (between RefBot.cpp and SpriteVB.cpp) */

/* bdisplay vertex buffers (assumed prototypes) */
#define PS_CreateVB ((int (*)(int *, int, int, int, int))FUN_0003f500)
#define PS_LockVB ((void *(*)(int, int))FUN_0003f710)

/* 0x00084f30  ClassLib (gap)
Constructor: enough vertex buffers of at most 16000 vertices for `numVerts` vertices
(at most 28 buffers). */
CPointSprites *PointSprites_Init(CPointSprites *self, int blendMode, int texture, uint numVerts,
                                 int param94)
{
	uint buffers;
	uint i;

	memset(self, 0, 0x27 * 4);
	self->param94 = param94;
	self->vertsPerBuffer = numVerts;
	buffers = 1;
	if (16000 < numVerts) {
		buffers = numVerts / 16000 + 1;
		self->vertsPerBuffer = 16000;
		if (0x1b < buffers)
			buffers = 0x1c;
	}
	i = 0;
	if (buffers != 0) {
		do {
			if (PS_CreateVB(&self->buffers[i], self->vertsPerBuffer, 0x42, 0, 1) == 0) {
				CLF_Printf("======== VERTEX BUFFER CREATE FAILED IN THE POINTSPRITES ========\n");
				DbgBreakPoint();
				buffers = i;
			}
			i = i + 1;
		} while ((int)i < (int)buffers);
	}
	if ((int)buffers < 0x1e)
		memset(&self->buffers[buffers], 0, (0x1e - buffers) * 4);
	self->texture = texture;
	self->buffer = 0;
	self->write = NULL;
	self->blendMode = blendMode;
	self->vertsLeft = self->vertsPerBuffer;
	return self;
}

/* 0x00085000  ClassLib (gap)
Sets blend mode `mode` of the table at 0x00290e38 (D3DDevice_SetRenderState inlined:
blend op, source blend, destination blend). */
void Blend_SetMode(int mode)
{
	uint v;

	v = DAT_00290e38[mode][2];
	D3DDevice_SetRenderState_Simple(0x40350, v);
	D3D__RenderState[D3DRS_BLENDOP] = v;
	v = DAT_00290e38[mode][0];
	D3DDevice_SetRenderState_Simple(0x40344, v);
	D3D__RenderState[D3DRS_SRCBLEND] = v;
	v = DAT_00290e38[mode][1];
	D3DDevice_SetRenderState_Simple(0x40348, v);
	D3D__RenderState[D3DRS_DESTBLEND] = v;
}

/* 0x00085060  ClassLib (gap)
Locks the first buffer and sets the render states for drawing (texture on stage 3,
modulated by the vertex colour). */
void PointSprites_Begin(CPointSprites *self)
{
	int mode;

	self->write = PS_LockVB(self->buffers[0], 4);
	self->vertsLeft = self->vertsPerBuffer;
	self->buffer = 0;
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)2, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)3, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)4, 4);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)6, 2);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)7, 0);
	D3DDevice_SetTextureState_Deferred(3, (D3DTEXTURESTAGESTATETYPE)0, 4);
	FUN_000416c0(0, 0);
	FUN_000416c0(1, 0);
	FUN_000416c0(2, 0);
	if (self->texture != 0)
		FUN_000416c0(3, (struct bdTexture *)self->texture);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(4, 0, 0);
	switch (self->blendMode) {
	case 0:
		FUN_00044b00(6, 0, 0);
		FUN_00043930((undefined4 *)&DAT_002855f0);
		return;
	case 1:
		mode = 0;
		break;
	case 2:
		mode = 1;
		break;
	case 3:
		mode = 3;
		break;
	case 4:
		mode = 2;
		break;
	default:
		FUN_00043930((undefined4 *)&DAT_002855f0);
		return;
	}
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(6, 1, 0);
	Blend_SetMode(mode);
	FUN_00043930((undefined4 *)&DAT_002855f0);
}

/* ======================================================================== weighted texture frames, actor animation switch
0x000a7d40-0x000a7e30 (between CRay.cpp and CParticleSystem.cpp) */

/* 0x000a7d40  ClassLib (gap)
Shows the frame of animated texture `tex` that is current at `time`, counting from frame
`first`: each frame lasts its weight (frame records of 0x18 bytes at tex + 0x24, weight at
+0x14; frame count at tex + 0x37). Negative times count back from the end. */
void Texture_SetFrameAtTime(int tex, int time, int first)
{
	int *weight;
	int *w;
	int total;
	int count;
	int n;
	int sum;

	count = *(byte *)(tex + 0x37);
	total = 0;
	if (first < count) {
		weight = (int *)(*(int *)(tex + 0x24) + 0x14 + first * 0x18);
		n = count - first;
		w = weight;
		do {
			total = total + *w;
			w = w + 6;
			n = n + -1;
		} while (n != 0);
		if (total != 0) {
			if (time < 0)
				time = total - -time % total;
			sum = 0;
			while (sum = sum + *weight, sum <= time % total) {
				first = first + 1;
				weight = weight + 6;
				if (count <= first)
					return;
			}
			FUN_00041580((struct bdTexture *)tex, first);
		}
	}
}

/* 0x000a7dc0  ClassLib (gap)
An actor animation switch { on, ..., actor at [6], animation at [7] }: switching on plays
the animation (looping), switching off stops the actor's animations. */
void AnimSwitch_Set(int *self, int on)
{
	if (self != NULL) {
		if (on == 0) {
			if (*self == 1) {
				FUN_00038190((void *)self[6], (struct BActorNode *)(*(int *)(self[6] + 0x134)), 1, 0);
				*self = 0;
			}
		} else if (*self == 0) {
			if (self[7] != 0)
				CLF_PlayAnim(self[6], *(int *)(self[6] + 0x134), self[7], 0x400, -1, 1, NULL, 0);
			*self = 1;
		}
	}
}

/* 0x000a7e30  ClassLib (gap)
Sets the word at self + 8 of an animation switch. */
void AnimSwitch_SetValue(int self, int value)
{
	if (self != 0)
		*(int *)(self + 8) = value;
}

/* ======================================================================== AI state handlers
0x00139e00-0x0013af80 (after CWaterField.cpp). They are the conditions and states of a
minigame AI derived from the CharLib AI (charlib's AI layout: state handlers are called as
handler(ai, entered), conditions as cond(ai)); the tables that point at them are at
0x002ae728 (conditions) and 0x002ae770 (states STAND, IDLE, WALK, JUMP, LAND, ATTACK,
JUMP_ATTACK, PRESS, FLAG, FALL, HURT, TAUNT, SCORE, KNOCKOUT, THROW, PICKUP, DROWN). They
use Fox Hunt's player array (0x00302210) and sounds (0x003022c8). File unknown. */

/* fields of the AI (the base layout is charlib's AI, 0x1130 bytes; the rest is the derived
class) */
#define AI_F(ai, off) (*(float *)((byte *)(ai) + (off)))
#define AI_I(ai, off) (*(int *)((byte *)(ai) + (off)))
#define AI_S(ai, off) (*(short *)((byte *)(ai) + (off)))
#define AI_P(ai, off) (*(byte **)((byte *)(ai) + (off)))
#define AI_ACTOR(ai) AI_P(ai, 0x30)          /* the CharLib actor */
#define AI_STATE(ai) AI_S(ai, 0x36)
#define AI_PREV_STATE(ai) AI_S(ai, 0x38)
#define AI_STATE_FRAMES(ai) AI_I(ai, 0x3c)
#define AI_STATE_TIME(ai) AI_F(ai, 0x40)
#define AI_MOVE_DIR(ai) (&AI_F(ai, 0x44))      /* float[3] */
#define AI_STICK(ai) AI_F(ai, 0x54)
#define AI_PUSH(ai) (&AI_F(ai, 0x7c))          /* float[3] */
#define AI_ENEMY(ai) AI_S(ai, 0x1084)
#define AI_SPEED_SCALE(ai) AI_F(ai, 0x10b0)
#define AI_IS_FOX(ai) AI_I(ai, 0x11bc)         /* 1 selects the alternative animations */
/* the actor's player slot */
#define AI_SLOT(ai) (*(ushort *)(AI_ACTOR(ai) + 0x230))

#define AI_PlayAnim(ai, anim, loop, queue, speed, blend, sync) \
	FUN_000af690((void *)AI_ACTOR(ai), (anim), (loop), (queue), (speed), (blend), (sync))
#define AI_Sample(ai, kind) \
	FUN_00071770((char)DAT_002baed4[AI_SLOT(ai) * 10], (kind), NULL, DAT_00293928, 0, -1, 0)
#define AI_Speech(id) FUN_00133ee0((void *)DAT_003022c8, (id), 0xff, 0)
#define AI_VCALL4(ai) (((void (*)(void *))(*(void ***)(ai))[4])(ai))

/* other modules' data */
extern int DAT_002baed4[];      /* core: players, 0x28 bytes each: [0] character, [1] present, [8] team */
extern int DAT_002baed8[];
extern int DAT_002baef4[];
extern int DAT_002bae94;        /* core: number of players */
extern int DAT_002baebc;        /* core: team mode */
extern byte *DAT_002ba96c;      /* core: the game (+0x18 stage) */
extern int DAT_00302210[];      /* foxhunt: AI per player */
extern int DAT_003022c8;        /* foxhunt: speech */
extern byte *DAT_002ea488;      /* classlib_ui: an object with a list at +0x18 */
extern int DAT_00293928;        /* charlib: sample volume */
extern float DAT_002f2890;      /* charlib */
extern float DAT_002f2894;      /* charlib */
extern float DAT_0028fdd8[4];   /* classlib_ui: (0, 0, 1) */
/* this module's (data blocks) */
extern int DAT_003023cc;
extern float DAT_003023d0;
extern float DAT_002ae5b8;
extern float DAT_002ae5bc;
extern float DAT_002ae7b4;
extern float DAT_002ae7b8;
extern float DAT_002ae7bc;
extern int DAT_002ae7c0;
extern int DAT_002ae7c4;
extern float DAT_002ae7c8;
extern float DAT_002ae7cc;
extern int DAT_002ae7d0;
extern float DAT_002ae7d4;
extern float DAT_002ae7d8;
extern float DAT_002ae7dc[3];
extern float DAT_002ae7ec[3];
extern float DAT_002ae7fc;
extern float DAT_002ae800;
extern float DAT_002ae804;
extern float DAT_002ae808;
extern float DAT_002ae80c;
extern float DAT_002ae810;

/* push = moveDir * stick * speed scale */
static void AIState_PushAlongStick(void *ai)
{
	float s;

	s = AI_SPEED_SCALE(ai) * AI_STICK(ai);
	AI_PUSH(ai)[0] = s * AI_MOVE_DIR(ai)[0];
	AI_PUSH(ai)[1] = s * AI_MOVE_DIR(ai)[1];
	AI_PUSH(ai)[2] = s * AI_MOVE_DIR(ai)[2];
}

static void AIState_NoPush(void *ai)
{
	AI_PUSH(ai)[2] = 0.0f;
	AI_PUSH(ai)[1] = 0.0f;
	AI_PUSH(ai)[0] = 0.0f;
}

/* 0x00139e00  ClassLib (gap)
Condition: the stick is pushed far enough (and, outside STAND/IDLE/LAND/TAUNT, the actor
may move). */
int AICond_Walk(void *ai)
{
	switch ((ushort)AI_STATE(ai)) {
	case 0:
	case 1:
	case 4:
	case 0xb:
		if (AI_STICK(ai) < AI_F(ai, 0x10e8))
			return 0;
		break;
	default:
		if (AI_STICK(ai) < AI_F(ai, 0x10e8))
			return 0;
		if (*(short *)(AI_ACTOR(ai) + 0x234) == 0)
			return 0;
	}
	return 1;
}

/* 0x00139e70  ClassLib (gap)
Condition: the jump button is pressed while the actor stands on the ground. */
int AICond_Jump(void *ai)
{
	if (*(int *)(AI_P(ai, 0x68) + 0x14) != 0 && *(int *)(AI_ACTOR(ai) + 0x214) != 0)
		return 1;
	return 0;
}

/* 0x00139ea0  ClassLib (gap)
Condition: the attack button is pressed with a target in reach (the nearest other player
at about the same height, or an object of the list at DAT_002ea488 in front); turns the
actor towards it. Also true in the flag game when far from the flag (0, 570, 1185). */
int AICond_Attack(void *ai)
{
	byte *actor;
	byte *other;
	int *node;
	float best;
	float h;
	float d;
	float ad;
	float flag[4];
	int target;
	int i;
	int me;

	if (AI_IS_FOX(ai) == 1)
		return 0;
	if (*(int *)(AI_P(ai, 0x6c) + 0x14) != 0) {
		target = -1;
		best = 1e+06f;
		for (i = 0; i < DAT_002bae94; i++) {
			actor = AI_ACTOR(ai);
			other = *(byte **)(DAT_00302210[i] + 0x30);
			if (i != *(ushort *)(actor + 0x230) &&
			    fabsf(*(float *)(actor + 0xd4) - *(float *)(other + 0xd4)) < AI_F(ai, 0x10f8)) {
				d = FUN_00051500((float *)(actor + 0xd0), (float *)(other + 0xd0));
				if (d < best) {
					best = d;
					target = i;
				}
			}
		}
		if (target != -1) {
			if (*(short *)(DAT_00302210[target] + 0x36) == 0xd && *(int *)(DAT_002ba96c + 0x18) != 4)
				return 0;
			me = AI_SLOT(ai);
			if (DAT_002baebc != 0 && DAT_002baed8[me * 10] == 0 &&
			    DAT_002baef4[me * 10] == DAT_002baef4[target * 10])
				return 0;
			if (best < AI_F(ai, 0x10f4)) {
				h = FUN_00070de0((float *)(AI_ACTOR(ai) + 0xd0),
				                 (float *)(*(byte **)(DAT_00302210[target] + 0x30) + 0xd0));
				FUN_00070670(h, *(float *)(AI_ACTOR(ai) + 0x1e8)); /* result not used */
				goto attack;
			}
		}
		for (node = CLF_ListNext(DAT_002ea488 + 0x18, NULL); node != NULL;
		     node = CLF_ListNext(DAT_002ea488 + 0x18, node)) {
			actor = AI_ACTOR(ai);
			if (node[0x110] != 0 &&
			    fabsf(*(float *)(actor + 0xd4) - *(float *)&node[0x39]) < AI_F(ai, 0x10f8)) {
				h = FUN_00070de0((float *)(actor + 0xd0), (float *)&node[0x38]);
				ad = FUN_00070670(h, *(float *)(AI_ACTOR(ai) + 0x1e8));
				if (ad < AI_F(ai, 0x10f0) &&
				    (ad + 1.0f) * FUN_00051500((float *)(AI_ACTOR(ai) + 0xd0), (float *)&node[0x38]) <
				        AI_F(ai, 0x10f4))
					goto attack;
			}
		}
	}
	if (*(int *)(AI_P(ai, 0x6c) + 0x14) != 0 && DAT_002baed8[AI_SLOT(ai) * 10] == 1) {
		if (*(int *)(DAT_002ba96c + 0x18) < 3)
			return 1;
		flag[0] = 0.0f;
		flag[1] = 570.0f;
		flag[2] = 1185.0f;
		if (250.0f <= FUN_00051500((float *)(AI_ACTOR(ai) + 0xd0), flag))
			return 1;
	}
	return 0;

attack:
	AI_ENEMY(ai) = -1;
	*(float *)(AI_ACTOR(ai) + 0x1d8) = h;
	return 1;
}

/* 0x0013a180  ClassLib (gap)
Condition: falling (off the ground and slower than the fall threshold, or still rising
less in JUMP... any downward speed while in JUMP). */
int AICond_Fall(void *ai)
{
	byte *actor;

	actor = AI_ACTOR(ai);
	if (*(int *)(actor + 0x214) == 0 &&
	    (*(float *)(actor + 0x198) <= AI_F(ai, 0x10ec) ||
	     (AI_STATE(ai) == 3 && *(float *)(actor + 0x198) < 0.0f)))
		return 1;
	return 0;
}

/* 0x0013a1d0  ClassLib (gap)
State LAND. */
int AIState_Land(void *ai, int entered)
{
	if (entered != 0) {
		AI_PlayAnim(ai, AI_IS_FOX(ai) == 1 ? 0x2a : 5, 0, 0, 1.0f, 0.300000012f, 1);
		AI_Sample(ai, 1);
	}
	AIState_NoPush(ai);
	return 0;
}

/* 0x0013a260  ClassLib (gap)
State TAUNT. */
int AIState_Taunt(void *ai, int entered)
{
	if (entered != 0) {
		AI_Sample(ai, 0xb);
		AI_PlayAnim(ai, 7, 0, 0, 1.0f, 0.100000001f, 1);
	}
	AIState_NoPush(ai);
	return 0;
}

/* 0x0013a2e0  ClassLib (gap)
State PICKUP: after 1 second the pick-up happens; stands still for 3 seconds. */
int AIState_Pickup(void *ai, int entered)
{
	if (entered != 0)
		AI_PlayAnim(ai, 0x2d, 0, 0, 1.0f, 0.100000001f, 1);
	if (1.0f <= AI_STATE_TIME(ai))
		AI_I(ai, 0x1188) = 1;
	if (AI_STATE_TIME(ai) <= DAT_002ae80c)
		AIState_NoPush(ai);
	return 0;
}

/* 0x0013a360  ClassLib (gap)
State DROWN: sinks slowly for 60 frames, then respawns (state 9). */
int AIState_Drown(void *ai, int entered)
{
	byte *actor;

	if (entered != 0) {
		AI_PlayAnim(ai, 0x1f, 1, 0, 1.0f, 0.100000001f, 1);
		AI_I(ai, 0x11c4) = 0;
		FUN_00136470(AI_I(ai, 0x11c8));
		AI_I(ai, 0x11c8) = -1;
	}
	if (AI_STATE_TIME(ai) <= DAT_002ae810) {
		AIState_NoPush(ai);
		actor = AI_ACTOR(ai);
		*(float *)(actor + 0x198) = *(float *)(actor + 0x198) * 0.649999976f;
		return 0;
	}
	if (DAT_002ae810 <= AI_STATE_TIME(ai)) {
		FUN_00133370((int)ai);
		FUN_000b02f0(ai, 9);
		*(int *)(AI_ACTOR(ai) + 0x220) = 0x59;
		AI_I(ai, 0x11e8) = 0;
	}
	return 0;
}

/* the PRESS/ATTACK animation of the actor's character (0xd, 0xe or 0xf) */
static int AIState_CharacterAnim(void *ai)
{
	int character;

	character = DAT_002baed4[AI_SLOT(ai) * 10];
	if (character == 1 || character == 4)
		return 0xf;
	return (character == 2) + 0xd;
}

/* 0x0013a420  ClassLib (gap)
State PRESS. */
int AIState_Press(void *ai, int entered)
{
	if (entered != 0) {
		AI_PlayAnim(ai, AIState_CharacterAnim(ai), 0, 0, DAT_002ae7ec[0], DAT_002ae7dc[0], 1);
		AI_S(ai, 0x1092) = 0;
		AI_Speech(5);
	}
	if ((float)AI_I(ai, 0x1104) < AI_STATE_TIME(ai))
		AI_I(ai, 0x1184) = 0;
	return 0;
}

/* 0x0013a4c0  ClassLib (gap)
State IDLE: after 100 frames, a 1 in 30 chance per frame to go to STAND. */
int AIState_Idle(void *ai, int entered)
{
	if (entered != 0)
		AI_PlayAnim(ai, AI_IS_FOX(ai) == 1 ? 0x29 : 0xb, 1, 0, 1.0f, 0.100000001f, 1);
	AIState_NoPush(ai);
	if (DAT_002ae7b8 < AI_STATE_TIME(ai) && CLF_Rand() % 0x1e == 0)
		FUN_000b02f0(ai, 0);
	return 0;
}

/* 0x0013a570  ClassLib (gap)
State STAND: slows the actor; after 300 frames, a 1 in 30 chance per frame to go to IDLE. */
int AIState_Stand(void *ai, int entered)
{
	byte *actor;
	float f;

	if (entered != 0) {
		AI_PlayAnim(ai, AI_IS_FOX(ai) == 1 ? 0x29 : 0, 0, 0, 1.0f, 0.100000001f, 1);
		f = DAT_002ae7b4;
		actor = AI_ACTOR(ai);
		*(float *)(actor + 0x194) = f * *(float *)(actor + 0x194);
		*(float *)(actor + 0x198) = f * *(float *)(actor + 0x198);
		*(float *)(actor + 0x19c) = f * *(float *)(actor + 0x19c);
	}
	AIState_NoPush(ai);
	if (DAT_002ae7bc <= AI_STATE_TIME(ai) && CLF_Rand() % 0x1e == 0 && AI_IS_FOX(ai) != 1)
		FUN_000b02f0(ai, 1);
	return 0;
}

/* 0x0013a650  ClassLib (gap)
State WALK: walk or run animation by speed (blended in from PRESS when DAT_003023cc is
set), animation speed by ground speed, push along the stick. */
int AIState_Walk(void *ai, int entered)
{
	byte *actor;
	float f;

	if (entered != 0) {
		AI_I(ai, 0x10e4) = 1;
		if (DAT_003023cc == 0 || AI_PREV_STATE(ai) != 4) {
			AI_PlayAnim(ai, AI_IS_FOX(ai) == 1 ? 0x27 : 1, 1, 0, 0.0f, DAT_0028fda8 * 0.100000001f, 1);
		} else {
			actor = AI_ACTOR(ai);
			FUN_00038190(actor + 0x10, *(void **)(actor + 0x144), 1, 0);
			*(short *)(AI_ACTOR(ai) + 0x236) = 0;
			actor = AI_ACTOR(ai);
			if (AI_IS_FOX(ai) == 1)
				FUN_00035810((void *)(actor + 0x10), *(void **)(actor + 0x144), *(void **)(actor + 0x304),
				             *(void **)(actor + 0x30c), DAT_002ae7c0, DAT_002ae7c4, -1, -1, DAT_002ae7c8, 3,
				             NULL, NULL);
			else
				FUN_00035810((void *)(actor + 0x10), *(void **)(actor + 0x144), *(void **)(actor + 0x2c4),
				             *(void **)(actor + 0x278), DAT_002ae7c0, DAT_002ae7c4, -1, -1, DAT_002ae7c8, 3,
				             NULL, NULL);
		}
	}
	if (DAT_003023cc != 0 && AI_PREV_STATE(ai) == 4 && AI_STATE_FRAMES(ai) == DAT_002ae7d0)
		FUN_00035d70(*(void **)(AI_ACTOR(ai) + 0x144), 1, DAT_002ae7c0, DAT_002ae7c4, DAT_002ae7cc, 0.0f, 7);
	if (AI_I(ai, 0x10e4) == 0) {
		if (*(float *)(AI_ACTOR(ai) + 0x254) < DAT_002ae5bc) {
			FUN_000af580((void *)AI_ACTOR(ai), AI_IS_FOX(ai) == 1 ? 0x27 : 1, 0.0500000007f);
			AI_I(ai, 0x10e4) = 1;
		}
	} else if (DAT_002ae5b8 < *(float *)(AI_ACTOR(ai) + 0x254)) {
		FUN_000af580((void *)AI_ACTOR(ai), AI_IS_FOX(ai) == 1 ? 0x28 : 0x18, 0.0500000007f);
		AI_I(ai, 0x10e4) = 0;
	}
	actor = AI_ACTOR(ai);
	if (AI_I(ai, 0x10e4) == 0)
		f = *(float *)(*(byte **)(actor + 0x324) + 0x6a0);
	else
		f = *(float *)(*(byte **)(actor + 0x324) + 0x84);
	*(float *)(actor + 0x23c) = f * *(float *)(actor + 0x254);
	AIState_PushAlongStick(ai);
	*(float *)(AI_ACTOR(ai) + 0x1d8) = FUN_000706f0(AI_PUSH(ai));
	return 0;
}

/* 0x0013a890  ClassLib (gap)
State JUMP: takes off (with the speed of the platform under the actor), push along the
stick. */
int AIState_Jump(void *ai, int entered)
{
	byte *actor;
	byte *platform;

	if (entered != 0) {
		*(int *)(AI_ACTOR(ai) + 0x198) = AI_I(ai, 0x1170);
		actor = AI_ACTOR(ai);
		platform = *(byte **)(*(byte **)(actor + 0x18c) + 0x1b0);
		if (platform != NULL) {
			*(float *)(actor + 0x194) = *(float *)(platform + 0x1a4) + *(float *)(actor + 0x194);
			actor = AI_ACTOR(ai);
			*(float *)(actor + 0x198) =
				*(float *)(*(byte **)(*(byte **)(actor + 0x18c) + 0x1b0) + 0x1a8) + *(float *)(actor + 0x198);
			actor = AI_ACTOR(ai);
			*(float *)(actor + 0x19c) =
				*(float *)(*(byte **)(*(byte **)(actor + 0x18c) + 0x1b0) + 0x1ac) + *(float *)(actor + 0x19c);
		}
		AI_PlayAnim(ai, AI_IS_FOX(ai) == 1 ? 0x2b : 2, 0, 0, 1.0f, 0.300000012f, 1);
		if (AI_F(ai, 0x10e8) <= AI_STICK(ai))
			*(float *)(AI_ACTOR(ai) + 0x1d8) = FUN_000706f0(AI_PUSH(ai));
		if (*(short *)(AI_ACTOR(ai) + 0x230) == 0) {
			DAT_002f2894 = *(float *)(AI_ACTOR(ai) + 0xd4);
			DAT_002f2890 = 0.0f;
		}
		AI_Sample(ai, 0);
	}
	AIState_PushAlongStick(ai);
	return 0;
}

/* 0x0013a9f0  ClassLib (gap)
State ATTACK: an attack animation by character (random when not pressing), a lunge forward
between DAT_003023d0 and 3 seconds, facing the enemy; ends (vtable slot 4) between frames
[0x1104] and [0x1108] or when the hit happened. */
int AIState_Attack(void *ai, int entered)
{
	byte *actor;
	float f;
	int anim;
	uint speech;
	uint i;

	if (entered != 0) {
		if (*(int *)(DAT_002ba96c + 0x18) == 4) {
			/* both branches of the original are the same */
			if (AI_I(ai, 0x1184) == 1)
				anim = AIState_CharacterAnim(ai);
			else
				anim = (int)FUN_00011730() % 3 + 0xd;
			AI_PlayAnim(ai, anim, 0, 0, 1.0f, 0.100000001f, 1);
			AI_S(ai, 0x1092) = 0;
			if (FUN_00011750() <= 0.5f)
				speech = 7;
			else
				speech = 5;
		} else {
			if (AI_I(ai, 0x1184) == 1) {
				anim = AIState_CharacterAnim(ai);
				i = 0;
			} else {
				i = (uint)CLF_Rand() % 3;
				anim = i + 0xd;
			}
			AI_PlayAnim(ai, anim, 0, 0, DAT_002ae7ec[i], DAT_002ae7dc[i], 1);
			AI_S(ai, 0x1092) = 0;
			speech = 5;
		}
		AI_Speech(speech);
	}
	if (AI_I(ai, 0x1184) == 0) {
		if (DAT_002ae7d8 < AI_STATE_TIME(ai) || AI_STATE_TIME(ai) < DAT_003023d0) {
			AIState_NoPush(ai);
		} else {
			FUN_00051170(AI_PUSH(ai), DAT_0028fdd8, *(float *)(AI_ACTOR(ai) + 0x1d8));
			f = DAT_002ae7d4;
			AI_PUSH(ai)[0] = DAT_002ae7d4 * AI_PUSH(ai)[0];
			AI_PUSH(ai)[1] = f * AI_PUSH(ai)[1];
			AI_PUSH(ai)[2] = f * AI_PUSH(ai)[2];
		}
		if (AI_ENEMY(ai) != -1) {
			actor = AI_ACTOR(ai);
			*(float *)(actor + 0x1d8) =
				FUN_00070de0((float *)(actor + 0xd0),
				             (float *)(*(byte **)(DAT_00302210[AI_ENEMY(ai)] + 0x30) + 0xd0));
		}
		if ((float)AI_I(ai, 0x1104) <= AI_STATE_TIME(ai) &&
		    (AI_STATE_TIME(ai) <= (float)AI_I(ai, 0x1108) || AI_S(ai, 0x1092) == 0)) {
			AI_VCALL4(ai);
			return 0;
		}
	} else if (AI_I(ai, 0x1184) == 1 && (float)AI_I(ai, 0x1104) < AI_STATE_TIME(ai)) {
		AI_I(ai, 0x1184) = 0;
	}
	return 0;
}

/* 0x0013acf0  ClassLib (gap)
State THROW: stands still for 2 seconds. */
int AIState_Throw(void *ai, int entered)
{
	if (entered != 0) {
		AI_PlayAnim(ai, 0x2c, 0, 0, 1.0f, 0.100000001f, 1);
		if ((float)CLF_Rand() * 3.05185094e-05f <= 0.5f)
			AI_Speech(7);
		else
			AI_Speech(5);
	}
	if (AI_STATE_TIME(ai) <= DAT_002ae7fc)
		AIState_NoPush(ai);
	return 0;
}

/* 0x0013ada0  ClassLib (gap)
State JUMP_ATTACK: push along the stick; ends (vtable slot 4) between frames [0x110c] and
[0x1110] or when the hit happened. */
int AIState_JumpAttack(void *ai, int entered)
{
	if (entered != 0) {
		AI_PlayAnim(ai, 0x10, 0, 0, 1.0f, 0.100000001f, 1);
		AI_S(ai, 0x1092) = 0;
		AI_Speech(7);
	}
	AIState_PushAlongStick(ai);
	if ((float)AI_I(ai, 0x110c) <= AI_STATE_TIME(ai) &&
	    (AI_STATE_TIME(ai) <= (float)AI_I(ai, 0x1110) || AI_S(ai, 0x1092) == 0))
		AI_VCALL4(ai);
	return 0;
}

/* 0x0013ae50  ClassLib (gap)
State FALL: push along the stick. */
int AIState_Fall(void *ai, int entered)
{
	(void)entered;
	AIState_PushAlongStick(ai);
	return 0;
}

/* 0x0013ae80  ClassLib (gap)
State HURT. */
int AIState_Hurt(void *ai, int entered)
{
	if (entered != 0) {
		AI_Sample(ai, 2);
		AI_PlayAnim(ai, 0x11, 0, 0, 1.0f, 0.100000001f, 1);
		if (0.5f <= (float)CLF_Rand() * 3.05185094e-05f)
			AI_Speech(0x13);
		else
			AI_Speech(0x12);
		AI_Sample(ai, 2);
	}
	AIState_NoPush(ai);
	return 0;
}

/* 0x0013af80  ClassLib (gap)
State KNOCKOUT: thrown backwards for 20 frames, gets up at 70. */
int AIState_Knockout(void *ai, int entered)
{
	float f;

	if (entered != 0) {
		AI_Sample(ai, 3);
		AI_PlayAnim(ai, 0x13, 0, 0, 1.0f, 0.100000001f, 1);
		AI_I(ai, 0x1120) = 1;
		AI_Speech(0x14);
	}
	if (DAT_002ae804 <= AI_STATE_TIME(ai)) {
		AIState_NoPush(ai);
	} else {
		FUN_00051170(AI_PUSH(ai), DAT_0028fdd8, *(float *)(AI_ACTOR(ai) + 0x1d8));
		f = DAT_002ae808;
		if (AI_I(ai, 0x1120) != 0)
			f = -DAT_002ae808;
		AI_PUSH(ai)[0] = f * AI_PUSH(ai)[0];
		AI_PUSH(ai)[1] = f * AI_PUSH(ai)[1];
		AI_PUSH(ai)[2] = f * AI_PUSH(ai)[2];
	}
	if (-1 < AI_I(ai, 0x1120) && DAT_002ae800 <= AI_STATE_TIME(ai)) {
		AI_PlayAnim(ai, AI_I(ai, 0x1120) == 0 ? 0x16 : 0x15, 0, 0, 1.0f, 0.100000001f, 1);
		AI_I(ai, 0x1120) = -1;
	}
	return 0;
}
