/*
BumperCars/CBCar_Flame.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Flame.cpp   (module bumpercars_a)

The flame jets of the arena (CBCar_Game + 0x3f28): FLAME blocks of the world file, each a
model with begin/main/end animations, warning textures, a shadow, a light, and an optional
"inactive" model (an arm that swings out while the jet is off). A jet cycles: 1 waiting
(DELAY), 2 warning (WARNTIME), 3 starting (STARTLENGTH), 4 burning (MAINLENGTH), 5 ending
(ENDLENGTH). Cars within RADIUS of a burning jet are hit (0x00094880, bumpercars_b).

0x000952e0 is tagged with this file; the gap functions 0x00094360-0x000951f0 are this
class's methods. bumpercars_b calls the loader (0x000962db) and the updates.
*/

#include "bumpercars_a.h"
#include <strings.h>

static const char s_FlameFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Flame.cpp";

/* a flame jet (0x1a0 bytes) */
struct BCarFlame {
	int state;                      /* 0x000 0 new, 1 waiting, 2 warning, 3 starting, 4 burning, 5 ending */
	int armState;                   /* 0x004 the inactive model: 0 out, 1 retract, 2 swing, 3 in */
	float time;                     /* 0x008 frames of the warning loop */
	float delay;                    /* 0x00c DELAY */
	float startLength;              /* 0x010 STARTLENGTH */
	float mainLength;               /* 0x014 MAINLENGTH */
	float endLength;                /* 0x018 ENDLENGTH */
	float warnTime;                 /* 0x01c WARNTIME */
	float animTime[3];              /* 0x020 the begin/main/end animations' lengths (seconds) */
	float radius;                   /* 0x02c RADIUS */
	float freezeTime;               /* 0x030 FREEZETIME */
	float armAngle;                 /* 0x034 */
	float armRate;                  /* 0x038 */
	float armTarget;                /* 0x03c */
	struct bdTexture *warnTex;      /* 0x040 WARNTEX */
	struct bdTexture *warnLoopTex;  /* 0x044 WARNLOOPTEX */
	struct bdTexture *activeTex;    /* 0x048 ACTIVETEX */
	int warnFrames;                 /* 0x04c */
	int warnLoopFrames;             /* 0x050 */
	struct BActor *actor;           /* 0x054 MODEL */
	struct BActorInstance *model;   /* 0x058 */
	struct BAnim *anims[3];         /* 0x05c BEGINANIM, MAINANIM, ENDANIM */
	float pos[4];                   /* 0x068 POSITION */
	struct BActor *armActor;        /* 0x078 INACTIVE_MODEL */
	struct BActorInstance *arm;     /* 0x07c */
	struct BAnim *armAnims[4];      /* 0x080 INACTIVE_OUT, _RETRACT, _IN, _EXTEND */
	float armTimes[4];              /* 0x090 their lengths (seconds) */
	unsigned char timer[0x58];      /* 0x0a0 the jet's timer (BCarTimer) */
	unsigned char armTimer[0x58];   /* 0x0f8 the arm's timer */
	struct BActor *shadowActor;     /* 0x150 SHADOWMODEL */
	struct BActorInstance *shadowModel; /* 0x154 */
	struct BShadowVolume *shadow;   /* 0x158 */
	struct BLight *light;           /* 0x15c */
	float lightPos[4];              /* 0x160 */
	int lightColour[4];             /* 0x170 r, g, b, flicker */
	void *unknown180;               /* 0x180 */
	void *unknown184;               /* 0x184 */
	int unknown188[4];              /* 0x188 */
	struct BCarFlame *next;         /* 0x198 */
	int unknown19c;                 /* 0x19c */
};
OF_ASSERT_OFFSET(struct BCarFlame, timer, 0xa0);
OF_ASSERT_OFFSET(struct BCarFlame, light, 0x15c);
OF_ASSERT_OFFSET(struct BCarFlame, next, 0x198);
OF_ASSERT_SIZE(struct BCarFlame, 0x1a0);

/* the flame system (0xc bytes in the game object) */
struct CBCar_Flames {
	struct BCarFlame *first;  /* 0x0 */
	int active;               /* 0x4 the jets may fire */
	int unknown8;             /* 0x8 */
};

/* 0x00285634  a vector (0, 0, 0, 1)? the actor instances' orientation (C library data) */
extern float DAT_00285634[4];

#define Timer_Construct(t) BCA_FN(FUN_00096320, void, (void *))(t)
#define Timer_Reset(t) BCA_FN(FUN_00096350, int, (void *))(t)
#define Timer_Tick(t) BCA_FN(FUN_000963a0, void, (void *))(t)
#define Timer_Begin(t) BCA_FN(FUN_000964d0, void, (void *))(t)
#define Timer_Fraction(t) BCA_FN(FUN_000965e0, float, (void *))(t)
#define Timer_Remaining(t) BCA_FN(FUN_000965f0, float, (void *))(t)
#define Timer_SetSeconds(t, s) BCA_FN(FUN_001c36e0, void, (void *, float))((t), (s))
#define Timer_Pause(t) BCA_FN(FUN_001c3770, void, (void *))(t)
#define READ_LINE(r, buf) BCA_FN(FUN_001386c0, char *, (void *, char *))((r), (buf))

static void restart_anim(struct BActorInstance *inst, struct BAnim *anim, int loops)
{
	baFlushAnimations(inst, *(struct BActorNode **)((char *)inst + 0x134), 1, 0);
	if (anim != NULL)
		baQueueAnimation(inst, *(struct BActorNode **)((char *)inst + 0x134), anim, 0x400, -1, loops, NULL, NULL);
}

/* 0x00094360  (gap): the flame system's constructor */
void *CBCar_Flames_Ctor(struct CBCar_Flames *self)
{
	self->first = NULL;
	self->active = 1;
	return self;
}

/* 0x00094370  (gap): a jet's constructor (its two timers) */
struct BCarFlame *CBCar_Flame_Ctor(struct BCarFlame *self)
{
	Timer_Construct(self->timer);
	Timer_Construct(self->armTimer);
	return self;
}

/* 0x000943c0  (gap): a jet's destructor (the timers' are empty) */
void CBCar_Flame_Dtor(struct BCarFlame *self)
{
	(void)self;
	FUN_001d5950();
	FUN_001d5950();
}

/* 0x00094420  (gap): the inactive arm: out, then (when its timer ends) it turns half a turn
while the jet waits, then goes back in */
void CBCar_Flame_UpdateArm(struct BCarFlame *self, float dt)
{
	float m[16], length;

	if (self->arm == NULL)
		return;
	if (self->armState == 0) {
		restart_anim(self->arm, self->armAnims[1], 0);
		Timer_SetSeconds(self->armTimer, self->armTimes[1]);
		Timer_Begin(self->armTimer);
		self->armState = 1;
		return;
	}
	if (self->armState == 1 && Timer_Remaining(self->armTimer) == 0.0f) {
		length = (self->delay - self->warnTime) - (self->armTimes[3] + self->armTimes[1]);
		self->armRate = fabsf(2.0f) / (length * 60.0f);
		self->armTarget = self->armAngle - 0x1.921fb4p+0f;
		restart_anim(self->arm, self->armAnims[2], 1);
		Timer_SetSeconds(self->armTimer, length);
		Timer_Begin(self->armTimer);
		self->armState = 2;
		return;
	}
	if (self->armState == 2) {
		if (Timer_Remaining(self->armTimer) != 0.0f) {
			self->armAngle = BCA_FN(FUN_00101f50, float, (float, float, float))(self->armAngle, self->armTarget,
				dt * self->armRate);
			bmMatrixRotationY(m, self->armAngle);
			bmMatrixToQuaternion((float *)((char *)self->arm + 0xe0), m);
			return;
		}
		if (self->armState == 2 && Timer_Remaining(self->armTimer) == 0.0f) {
			self->armAngle = self->armTarget;
			bmMatrixRotationY(m, self->armTarget);
			bmMatrixToQuaternion((float *)((char *)self->arm + 0xe0), m);
			restart_anim(self->arm, self->armAnims[3], 0);
			Timer_SetSeconds(self->armTimer, self->armTimes[3]);
			Timer_Begin(self->armTimer);
			self->armState = 3;
			return;
		}
	}
	if (self->armState == 3 && Timer_Remaining(self->armTimer) == 0.0f)
		self->armState = 0;
}

/* 0x000946d0  (gap): draws the jets: the arms (not in the reflection pass) and the burning
jets (with their reflection when environment mapping is on) */
void CBCar_Flames_Draw(struct CBCar_Flames *self, int view, int reflection)
{
	struct BCarFlame *f;
	int passes = (-(view != 0) & 5) + 1;

	bdPushRenderState(4, 0, 0);
	bdPushRenderState(7, 3, 0);
	for (f = self->first; f != NULL; f = f->next) {
		bdSetWorldIdentity();
		if (f->arm != NULL && reflection == 0) {
			bdPushRenderState(6, 1, 0);
			baPrepActorInstance(f->arm, 1);
			baDrawActorInstance(f->arm, passes);
			bdPopRenderState(6);
		}
		if (f->state > 2 && f->state < 6 && f->model != NULL) {
			bdPushRenderState(6, 1, 0);
			bdPushRenderState(3, 0, 0);
			bdPushRenderState(10, 1, 0);
			bdPushRenderState(5, 0, 0);
			bdPushRenderState(4, 1, 0);
			if (CBCar_Debug_EnvMap((struct CBCar_Debug *)g_BCarGame->debug) != 0) {
				FUN_001d6820();
				FUN_001d6860();
				BCarRender_BeginReflection(1);
				BCA_F32(f->model, 0xd4) = BCA_F32(f->model, 0xd4) * -1.0f;
				baPrepActorInstance(f->model, 1);
				baDrawActorInstance(f->model, passes);
				BCA_F32(f->model, 0xd4) = BCA_F32(f->model, 0xd4) * -1.0f;
				BCarRender_EndReflection();
				FUN_0018e640();
			}
			baPrepActorInstance(f->model, 1);
			baDrawActorInstance(f->model, passes);
			bdPopRenderState(4);
			bdPopRenderState(5);
			bdPopRenderState(10);
			bdPopRenderState(3);
			bdPopRenderState(6);
		}
	}
	bdPopRenderState(7);
	bdPopRenderState(4);
}

/* 0x00094880  (gap): the freeze time of a burning jet that reaches pos (within its radius +
`radius`, squared distance), or -1 */
float CBCar_Flames_HitTime(struct CBCar_Flames *self, float *pos, float radius)
{
	struct BCarFlame *f;
	float r;

	for (f = self->first; f != NULL; f = f->next) {
		if (f->state > 2 && f->state < 6) {
			r = radius + f->radius;
			if (BCA_FN(FUN_00051540, float, (float *, float *))(pos, f->pos) <= r * r)
				return f->freezeTime;
		}
	}
	return -1.0f;
}

/* 0x000948f0  (gap): pauses the jets' timers */
void CBCar_Flames_Pause(struct CBCar_Flames *self)
{
	struct BCarFlame *f;

	for (f = self->first; f != NULL; f = f->next) {
		if (f->state != 0)
			Timer_Pause(f->timer);
		Timer_Pause(f->armTimer);
	}
}

/* 0x00094920  (gap): restarts the jets' timers */
void CBCar_Flames_Resume(struct CBCar_Flames *self)
{
	struct BCarFlame *f;

	for (f = self->first; f != NULL; f = f->next) {
		if (f->state != 0)
			Timer_Begin(f->timer);
		Timer_Begin(f->armTimer);
	}
}

/* 0x00094950  (gap): lets the jets fire (or not: the warning ones go back to waiting) */
void CBCar_Flames_SetActive(struct CBCar_Flames *self, int active)
{
	struct BCarFlame *f;

	self->active = active;
	for (f = self->first; f != NULL; f = f->next) {
		if (self->active == 0 && f->state == 2) {
			baFlushAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 0);
			baSetNodeTime(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 1);
			if (f->warnTex != NULL)
				bdTextureSetFrame(f->warnTex, 0);
			if (f->warnLoopTex != NULL)
				bdTextureSetFrame(f->warnLoopTex, 0);
			f->state = 1;
			Timer_SetSeconds(f->timer, f->delay - f->warnTime);
			Timer_Begin(f->timer);
		}
	}
}

/* 0x00094a00  (gap): frees the jets */
void CBCar_Flames_Shutdown(struct CBCar_Flames *self)
{
	struct BCarFlame *f;

	while (self->first != NULL) {
		f = self->first;
		if (f->model != NULL)
			baDeleteActorInstance(f->model);
		if (f->arm != NULL)
			baDeleteActorInstance(f->arm);
		FUN_001d5950();
		FUN_001d5950();
		if (f->shadow != NULL)
			baDeleteShadowVolume(f->shadow);
		if (f->shadowModel != NULL)
			baDeleteActorInstance(f->shadowModel);
		if (f->light != NULL)
			baDeleteLight(f->light);
		if (f->unknown180 != NULL)
			BCA_DELETE(f->unknown180);
		if (f->unknown184 != NULL)
			BCA_DELETE(f->unknown184);
		self->first = f->next;
		if (f != NULL) {
			CBCar_Flame_Dtor(f);
			BCA_DELETE(f);
		}
	}
	self->first = NULL;
}

/* debris around the jet: 50 up, 15 high, 5 jitter, radius 500 */
static void flame_debris(struct BCarFlame *f, int count, int life)
{
	CBCar_Effects_Circle(0, f->pos, 50.0f, 15.0f, 5.0f, 500.0f, count, life);
}

/* 0x00094af0  (gap): runs the jets' cycles */
void CBCar_Flames_Update(struct CBCar_Flames *self, float dt, int playing)
{
	struct BCarFlame *f;
	int c;
	float seconds;

	if (playing == 0)
		return;
	for (f = self->first; f != NULL; f = f->next) {
		if (f->state == 0) {
			Timer_Begin(f->timer);
			f->state = 1;
			CBCar_Flame_UpdateArm(f, dt);
			continue;
		}
		Timer_Tick(f->timer);
		Timer_Tick(f->armTimer);
		f->time = dt * 16.666666f + f->time;
		if (f->arm != NULL)
			baUpdateAnimations(f->arm, *(struct BActorNode **)((char *)f->arm + 0x134), dt, NULL);
		switch (f->state) {
		case 1:
			if (Timer_Remaining(f->timer) == 0.0f && self->active != 0) {
				if (f->arm != NULL)
					restart_anim(f->arm, f->armAnims[0], 0);
				f->state = 2;
				Timer_SetSeconds(f->timer, f->warnTime);
				Timer_Begin(f->timer);
				f->time = 0.0f;
				FUN_00092120(g_BCarGame->sound, 0x18, 0xff, 0, -1);
			} else {
				CBCar_Flame_UpdateArm(f, dt);
			}
			break;
		case 2:
			if (Timer_Remaining(f->timer) == 0.0f) {
				FUN_00092120(g_BCarGame->sound, 0x16, 0xff, 0, -1);
				baFlushAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 0);
				baSetNodeTime(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 1);
				if (f->anims[0] != NULL)
					baQueueAnimation(f->model, *(struct BActorNode **)((char *)f->model + 0x134), f->anims[0], 0x400,
						-1, 0, NULL, NULL);
				if (f->warnTex != NULL)
					bdTextureSetFrame(f->warnTex, f->warnFrames - 1);
				f->state = 3;
				seconds = f->animTime[0] / f->startLength;
				Timer_SetSeconds(f->timer, seconds);
				Timer_Begin(f->timer);
				break;
			}
			if (f->warnTex != NULL)
				bdTextureSetFrame(f->warnTex, BCA_FTOL(Timer_Fraction(f->timer) * (float)(f->warnFrames - 1)));
			if (f->warnLoopTex != NULL)
				BCA_FN(FUN_000a4e50, void, (void *, int, int))(f->warnLoopTex, BCA_FTOL(f->time), 1);
			if (Timer_Fraction(f->timer) > 0.8f) {
				c = (int)Core_Rand();
				flame_debris(f, 10, c % 10);
			}
			break;
		case 3:
			if (Timer_Remaining(f->timer) == 0.0f) {
				baFlushAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 0);
				baSetNodeTime(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 1);
				if (f->anims[1] != NULL)
					baQueueAnimationBlendIn(f->model, *(struct BActorNode **)((char *)f->model + 0x134), f->anims[1],
						0x400, -1, 0.1f, 1, NULL, NULL);
				f->state = 4;
				if (f->light != NULL)
					baEnableLight(f->light, 1);
				Timer_SetSeconds(f->timer, f->mainLength);
				Timer_Begin(f->timer);
				break;
			}
			flame_debris(f, 5, bca_rand() % 10);
			if (f->anims[0] != NULL)
				baUpdateAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), dt * f->startLength,
					NULL);
			goto warn_loop;
		case 4:
			if (Timer_Remaining(f->timer) != 0.0f) {
				if (f->light != NULL) {
					f->lightColour[3] = (int)Core_Rand() % 0x32 + 0x32;
					c = (int)Core_Rand() % 10;
					f->lightColour[1] = c;
					f->lightColour[0] = c;
					f->lightColour[2] = c + 0x14;
					baSetLightColour(f->light, c, c, c + 0x14);
				}
				if (f->activeTex != NULL)
					bdTextureSetTime(f->activeTex, BCA_FTOL((f->mainLength - Timer_Remaining(f->timer)) * 1000.0f));
				flame_debris(f, 5, bca_rand() % 10);
				if (f->anims[1] != NULL)
					baUpdateAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), dt, NULL);
				goto warn_loop;
			}
			baFlushAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 0);
			baSetNodeTime(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 1);
			if (f->anims[2] != NULL)
				baQueueAnimationBlendIn(f->model, *(struct BActorNode **)((char *)f->model + 0x134), f->anims[2], 0x400,
					-1, 0.1f, 0, NULL, NULL);
			f->state = 5;
			if (f->light != NULL)
				baEnableLight(f->light, 0);
			if (f->light != NULL) {
				f->lightColour[3] = 0;
				f->lightColour[2] = 0;
				f->lightColour[1] = 0;
				f->lightColour[0] = 0;
			}
			seconds = f->animTime[2] / f->endLength;
			Timer_SetSeconds(f->timer, seconds);
			Timer_Begin(f->timer);
			break;
		case 5:
			if (Timer_Remaining(f->timer) != 0.0f) {
				flame_debris(f, 5, bca_rand() % 10);
				if (f->anims[2] != NULL)
					baUpdateAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), dt * f->endLength,
						NULL);
				goto warn_loop;
			}
			baFlushAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 0);
			baSetNodeTime(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 1);
			if (f->warnTex != NULL)
				bdTextureSetFrame(f->warnTex, 0);
			if (f->warnLoopTex != NULL)
				bdTextureSetFrame(f->warnLoopTex, 0);
			f->state = 1;
			seconds = f->delay - f->warnTime;
			Timer_SetSeconds(f->timer, seconds);
			Timer_Begin(f->timer);
			break;
		}
		continue;
warn_loop:
		if (f->warnLoopTex != NULL)
			BCA_FN(FUN_000a4e50, void, (void *, int, int))(f->warnLoopTex, BCA_FTOL(f->time), 1);
	}
}

/* 0x00095150  (gap): a jet (warning or later) within its radius + `radius` of pos (squared
distance); the heading from pos to it goes to *heading. Returns 1 for one. */
int CBCar_Flames_Near(struct CBCar_Flames *self, float *pos, float radius, float *heading)
{
	struct BCarFlame *f;
	float r;

	for (f = self->first; f != NULL; f = f->next) {
		if (f->state > 1 && f->state < 6) {
			r = radius + f->radius;
			if (BCA_FN(FUN_00051540, float, (float *, float *))(pos, f->pos) <= r * r) {
				if (heading != NULL)
					*heading = BCA_FN(FUN_000a5540, float, (float, float))(f->pos[0] - pos[0], f->pos[2] - pos[2]);
				return 1;
			}
		}
	}
	return 0;
}

/* 0x000951f0  (gap): a sphere of `radius` at pos moving by vel hits a jet (warning or
later). Returns 1 for a hit. */
int CBCar_Flames_Sweep(struct CBCar_Flames *self, float *pos, float radius, float *vel)
{
	struct BCarFlame *f;
	struct BSphere jet, mover;
	struct BCollHit hit;

	if (sqrtf(vel[2] * vel[2] + vel[1] * vel[1] + vel[0] * vel[0]) == 0.0f)
		return 0;
	for (f = self->first; f != NULL; f = f->next) {
		if (f->state > 1 && f->state < 6) {
			jet.centre[0] = f->pos[0];
			jet.centre[1] = f->pos[1];
			jet.centre[2] = f->pos[2];
			jet.centre[3] = 1.0f;
			jet.radius = f->radius;
			mover.centre[0] = pos[0];
			mover.centre[1] = pos[1];
			mover.centre[2] = pos[2];
			mover.centre[3] = 1.0f;
			mover.radius = radius;
			memset(&hit, 0, sizeof hit); /* not set in the original */
			/* engine_misc declares 0x00053af0 void; the original tests its EAX */
			if (BCA_FN(FUN_00053af0, int, (struct BSphere *, struct BSphere *, float *, struct BCollHit *))(&mover, &jet, vel, &hit) != 0)
				return 1;
		}
	}
	return 0;
}

/* the length (seconds) of an animation segment: (end - start) frames of 1/1024 */
static float anim_seconds(struct BAnim *anim)
{
	int frames = BCA_I32(anim, 8) - BCA_I32(anim, 4);

	return (float)frames * 0x1p-10f * (1.0f / 60.0f);
}

/* 0x000952e0  BumperCars/CBCar_Flame.cpp: CBCar_Flames::LoadFlameInfo: the FLAME blocks of
the world file */
void CBCar_Flames_LoadFlameInfo(struct CBCar_Flames *self, struct BCarTextReader *reader)
{
	char line[0x200], word[0x200];
	char beginAnim[0x40], mainAnim[0x40], endAnim[0x40];
	char armAnimNames[4][0x40];
	char model[0x200], armModel[0x200], activeTex[0x200], warnTex[0x200], shadowModel[0x200], warnLoopTex[0x200];
	char *text;
	struct BCarFlame *f;
	void *memory;
	int status, i, valid;
	float value, x, y, z;
	unsigned int w, h, format, frame;
	struct BAnim *anim;

	if (reader->cursor >= reader->end)
		return;
	do {
		text = READ_LINE(reader, line);
		if (strncmp(text, "//", 2) == 0 || strcasecmp(text, "FLAME {") != 0)
			continue;
		memory = BCA_NEW(0x1a0, s_FlameFile, 0x6e, 0x1001);
		f = memory != NULL ? CBCar_Flame_Ctor((struct BCarFlame *)memory) : NULL;
		if (f == NULL) {
			BCA_Printf("%s(%d) : LoadFlameInfo - Could not get memory for new flame object\n", s_FlameFile, 0x70);
			return;
		}
		f->state = 0;
		f->endLength = 0.0f;
		f->mainLength = 0.0f;
		f->startLength = 0.0f;
		f->actor = NULL;
		f->model = NULL;
		f->anims[2] = NULL;
		f->anims[1] = NULL;
		f->anims[0] = NULL;
		f->delay = 0.0f;
		f->warnFrames = 0;
		f->pos[3] = 1.0f;
		f->pos[0] = 0.0f;
		f->pos[1] = 0.0f;
		f->pos[2] = 0.0f;
		f->animTime[2] = 1.0f;
		f->animTime[1] = 1.0f;
		f->animTime[0] = 1.0f;
		f->radius = 0.0f;
		f->warnTex = NULL;
		f->warnLoopTex = NULL;
		f->activeTex = NULL;
		f->warnTime = 0.0f;
		f->freezeTime = 0.0f;
		f->armState = 0;
		f->shadowActor = NULL;
		f->shadow = NULL;
		f->shadowModel = NULL;
		f->light = NULL;
		f->unknown180 = NULL;
		f->unknown184 = NULL;
		for (i = 0; i < 4; i++) {
			f->armTimes[i] = 0.0f;
			f->armAnims[i] = NULL;
			armAnimNames[i][0] = 0;
		}
		f->armActor = NULL;
		f->arm = NULL;
		model[0] = 0;
		beginAnim[0] = 0;
		mainAnim[0] = 0;
		endAnim[0] = 0;
		warnTex[0] = 0;
		activeTex[0] = 0;
		armModel[0] = 0;
		shadowModel[0] = 0;
		warnLoopTex[0] = 0;
		while (reader->cursor < reader->end && strcasecmp(text, "}") != 0) {
			text = READ_LINE(reader, line);
			if (strncmp(text, "//", 2) == 0 || text[0] == 0)
				continue;
			status = 0;
#define NAME_COMMAND(cmd, n, dest, size) \
			if (strncasecmp(text, cmd, n) == 0) { \
				status = 2; \
				if (BCA_FN(FUN_000a4e00, int, (char *, char *, int))(text, dest, size) != 0) \
					status = 1; \
			}
#define FLOAT_COMMAND(cmd, n, dest, test) \
			if (strncasecmp(text, cmd, n) == 0) { \
				status = 2; \
				if (sscanf(text, "%*s%f", &value) == 1 && (test)) { \
					dest = value; \
					status = 1; \
				} \
			}
			NAME_COMMAND("BEGINANIM ", 10, beginAnim, 0x3f)
			NAME_COMMAND("MAINANIM ", 9, mainAnim, 0x3f)
			NAME_COMMAND("ENDANIM ", 8, endAnim, 0x3f)
			NAME_COMMAND("MODEL ", 6, model, 0x1ff)
			NAME_COMMAND("SHADOWMODEL ", 0xc, shadowModel, 0x1ff)
			NAME_COMMAND("INACTIVE_MODEL ", 0xf, armModel, 0x1ff)
			NAME_COMMAND("INACTIVE_OUT ", 0xd, armAnimNames[0], 0x3f)
			NAME_COMMAND("INACTIVE_RETRACT ", 0x11, armAnimNames[1], 0x3f)
			NAME_COMMAND("INACTIVE_IN ", 0xc, armAnimNames[2], 0x3f)
			NAME_COMMAND("INACTIVE_EXTEND ", 0x10, armAnimNames[3], 0x3f)
			NAME_COMMAND("WARNTEX ", 8, warnTex, 0x1ff)
			NAME_COMMAND("WARNLOOPTEX ", 0xc, warnLoopTex, 0x1ff)
			NAME_COMMAND("ACTIVETEX ", 10, activeTex, 0x1ff)
			FLOAT_COMMAND("STARTLENGTH ", 0xc, f->startLength, value >= 0.0f)
			FLOAT_COMMAND("MAINLENGTH ", 0xb, f->mainLength, value >= 0.0f)
			FLOAT_COMMAND("ENDLENGTH ", 10, f->endLength, value >= 0.0f)
			FLOAT_COMMAND("DELAY ", 6, f->delay, value >= 0.0f)
			FLOAT_COMMAND("WARNTIME ", 9, f->warnTime, value >= 0.0f)
			FLOAT_COMMAND("RADIUS ", 7, f->radius, value > 0.0f)
			if (strncasecmp(text, "POSITION ", 9) == 0) {
				status = 2;
				if (sscanf(text, "%*s%f%f%f", &x, &y, &z) == 3) {
					f->pos[0] = x;
					f->pos[1] = y;
					f->pos[2] = z;
					f->pos[3] = 1.0f;
					status = 1;
				}
			}
			FLOAT_COMMAND("FREEZETIME ", 0xb, f->freezeTime, value >= 0.0f)
#undef NAME_COMMAND
#undef FLOAT_COMMAND
			if (strcasecmp(text, "}") != 0) {
				sscanf(text, "%s", word);
				if (status == 0)
					BCA_Printf("%s(%d) : LoadFlameInfo(\"%s\", %d): Invalid Command \"%s\"\n", s_FlameFile, 0x192,
						reader->name, reader->line, word);
				else if (status == 2)
					BCA_Printf("%s(%d) : LoadFlameInfo(\"%s\", %d): Bad use of Command \"%s\" (%s)\n", s_FlameFile,
						0x194, reader->name, reader->line, word, text);
			}
		}
		valid = 1;
		if (f->startLength <= 0.001f)
			valid = 0;
		if (f->mainLength <= 0.001f)
			valid = 0;
		if (f->endLength <= 0.001f)
			valid = 0;
		if (f->delay <= 0.001f)
			valid = 0;
		if (f->delay < f->warnTime || valid == 0) {
			BCA_Printf("Ignoring invalid flame object\n");
			CBCar_Flame_Dtor(f);
			BCA_DELETE(f);
			continue;
		}
		if (warnTex[0] != 0) {
			f->warnTex = (struct bdTexture *)bLoadResource(NULL, warnTex, 0, 0xdefa);
			if (f->warnTex == NULL) {
				BCA_Printf("%s(%d) : LoadFlameInfo - Could not find warning texture \"%s\"\n", s_FlameFile, 0x1a9,
					warnTex);
			} else {
				bdTextureGetInfo(f->warnTex, &w, &h, &format, (unsigned int *)&f->warnFrames, &frame);
				bdTextureSetFrame(f->warnTex, 0);
			}
		}
		if (warnLoopTex[0] != 0) {
			f->warnLoopTex = (struct bdTexture *)bLoadResource(NULL, warnLoopTex, 0, 0xdefa);
			if (f->warnLoopTex == NULL) {
				BCA_Printf("%s(%d) : LoadFlameInfo - Could not find looping warning texture \"%s\"\n", s_FlameFile,
					0x1bc, warnLoopTex);
			} else {
				bdTextureGetInfo(f->warnLoopTex, &w, &h, &format, (unsigned int *)&f->warnLoopFrames, &frame);
				bdTextureSetFrame(f->warnLoopTex, 0);
			}
		}
		if (activeTex[0] != 0) {
			f->activeTex = (struct bdTexture *)bLoadResource(NULL, activeTex, 0, 0xdefa);
			if (f->activeTex == NULL)
				BCA_Printf("%s(%d) : LoadFlameInfo - Could not find activation texture \"%s\"\n", s_FlameFile, 0x1cf,
					activeTex);
		}
		if (shadowModel[0] != 0) {
			f->shadowActor = (struct BActor *)bLoadResource(NULL, shadowModel, 1, 0xdefa);
			if (f->shadowActor == NULL) {
				BCA_Printf("%s(%d) : LoadFlameInfo - Could not load shadow actor \"%s\". No shadows will be created\n",
					s_FlameFile, 0x1d8, shadowModel);
			} else {
				f->shadowModel = baInitActorInstance(NULL, f->shadowActor, 1, s_FlameFile, 0x1dd);
				if (f->shadowModel == NULL) {
					/* the original prints the MODEL name here */
					BCA_Printf("%s(%d) : LoadFlameInfo - Could not create instance of shadow model \"%s\". No shadows "
						   "will be created\n",
						s_FlameFile, 0x1df, model);
				} else {
					memcpy((char *)f->shadowModel + 0xe0, DAT_00285634, 16);
					baPrepActorInstance(f->shadowModel, 1);
					f->lightPos[0] = f->pos[0];
					f->lightPos[1] = f->pos[1];
					f->lightPos[2] = f->pos[2];
					f->lightPos[3] = f->pos[3];
					f->lightPos[1] = f->lightPos[1] + 100.0f;
					f->lightPos[3] = 1.0f;
					f->lightColour[3] = 0;
					f->lightColour[2] = 0;
					f->lightColour[1] = 0;
					f->lightColour[0] = 0;
					f->light = baCreateLight(NULL, 0);
					if (f->light != NULL) {
						baSetPointLight(f->light, f->lightPos, 0x1.a36e2ep-14f);
						baSetLightColour(f->light, f->lightColour[0], f->lightColour[1], f->lightColour[2]);
						baEnableLight(f->light, 0);
					}
					f->shadow = baCreateShadowVolume(f->shadowModel, NULL, f->lightPos, 50000.0f, 0);
					if (f->shadow == NULL)
						BCA_Printf("%s(%d) : LoadFlameInfo - Could not create shadow volume\n", s_FlameFile, 0x1f5);
				}
			}
		}
		if (model[0] != 0) {
			f->actor = (struct BActor *)bLoadResource(NULL, model, 1, 0xdefa);
			if (f->actor == NULL) {
				BCA_Printf("%s(%d) : LoadFlameInfo - Could not load actor \"%s\". Flame now invisible\n", s_FlameFile,
					0x200, model);
			} else {
				f->model = baInitActorInstance(NULL, f->actor, 2, s_FlameFile, 0x205);
				if (f->model == NULL) {
					BCA_Printf("%s(%d) : LoadFlameInfo - Could not create instance of model \"%s\". Flame now invisible\n",
						s_FlameFile, 0x207, model);
				} else {
					char *names[3];

					memcpy((char *)f->model + 0xe0, DAT_00285634, 16);
					baPrepActorInstance(f->model, 1);
					names[0] = beginAnim;
					names[1] = mainAnim;
					names[2] = endAnim;
					for (i = 0; i < 3; i++) {
						if (names[i][0] == 0)
							continue;
						anim = (struct BAnim *)baFindSubObject(f->model, names[i]);
						f->anims[i] = anim;
						if (anim == NULL)
							BCA_Printf("%s(%d) : LoadFlameInfo - Could not find animation segment \"%s\". Ignoring "
								   "animation\n",
								s_FlameFile, 0x216, names[i]);
						else
							f->animTime[i] = anim_seconds(anim);
					}
					baFlushAnimations(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 0);
					baSetNodeTime(f->model, *(struct BActorNode **)((char *)f->model + 0x134), 1, 1);
				}
			}
		}
		if (armModel[0] != 0) {
			f->armActor = (struct BActor *)bLoadResource(NULL, armModel, 1, 0xdefa);
			if (f->armActor == NULL) {
				BCA_Printf("%s(%d) : LoadFlameInfo - Could not load inactive actor [%s]\n", s_FlameFile, 0x243, armModel);
			} else {
				f->arm = baInitActorInstance(NULL, f->armActor, 4, s_FlameFile, 0x248);
				if (f->arm == NULL) {
					BCA_Printf("%s(%d) : LoadFlameInfo - Could not create instance of inactive model [%s]\n",
						s_FlameFile, 0x24a, armModel);
				} else {
					memcpy((char *)f->arm + 0xe0, DAT_00285634, 16);
					baPrepActorInstance(f->arm, 1);
					for (i = 0; i < 4; i++) {
						if (armAnimNames[i][0] == 0)
							continue;
						anim = (struct BAnim *)baFindSubObject(f->arm, armAnimNames[i]);
						f->armAnims[i] = anim;
						if (anim == NULL)
							BCA_Printf("%s(%d) : LoadFlameInfo - Could not find inactive animation segment [%s]. "
								   "Ignoring animation\n",
								s_FlameFile, 0x259, armAnimNames[i]);
						else
							f->armTimes[i] = anim_seconds(anim);
					}
					baFlushAnimations(f->arm, *(struct BActorNode **)((char *)f->arm + 0x134), 1, 0);
				}
			}
			f->armAngle = 0.0f;
		}
		f->state = 0;
		Timer_Reset(f->timer);
		Timer_SetSeconds(f->timer, f->delay - f->warnTime);
		Timer_Reset(f->armTimer);
		f->next = self->first;
		self->first = f;
	} while (reader->cursor < reader->end);
}
