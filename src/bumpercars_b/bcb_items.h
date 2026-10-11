/*
BCB_ITEMS.H

Private to the bumpercars_b module (agent B part, 0x000a0cc0-0x000a8250): the pucks,
powerups, keepsakes, explosions, waypoints, skid tracks, headlights, the generic goals, the
floating scores and the gap helpers, and the calls into other modules they need. Shared
wrappers (sounds, lights, effects, camera...) come from bumpercars_b_internal.h.
*/

#ifndef OPENFRENZY_BCB_ITEMS_H
#define OPENFRENZY_BCB_ITEMS_H

#include "bumpercars_b_internal.h"

#define BCB_FN(fn, ret, params) ((ret(*) params)(fn))

/* ======================================================================== items */

/* a puck (CBCar_Puck.cpp, 0xec bytes, list at 0x002f0760) */
struct BCarPuckInfo {
	struct BCarEntity *entity;        /* 0x00 */
	struct BCarEntityParams params;   /* 0x04 */
	struct BActor *actor;             /* 0xd4 MODEL */
	struct BActor *disableModel;      /* 0xd8 DISABLEMODEL */
	float scale;                      /* 0xdc SCALE */
	int frame;                        /* 0xe0 character + 1 of the last car that hit it */
	struct bdTexture *colourTexture;  /* 0xe4 PL_COLOUR_TEX */
	struct BCarPuckInfo *next;        /* 0xe8 */
};
OF_ASSERT_SIZE(struct BCarPuckInfo, 0xec);

/* powerup types (BCarPowerupInfo.type) */
#define BCAR_POWERUP_SPEED 1

/* a powerup (CBCar_Powerup.cpp, 0xf4 bytes, list at 0x002f0774) */
struct BCarPowerupInfo {
	struct BCarEntity *entity;        /* 0x00 */
	int type;                         /* 0x04 TYPE */
	float data;                       /* 0x08 DATA: the speed scale of type 1 */
	struct BCarEntityParams params;   /* 0x0c */
	struct BActor *actor;             /* 0xdc MODEL */
	float scale;                      /* 0xe0 SCALE */
	float lifeLeft;                   /* 0xe4 seconds */
	float life;                       /* 0xe8 LIFE */
	struct BLight *light;             /* 0xec */
	struct BCarPowerupInfo *next;     /* 0xf0 */
};
OF_ASSERT_SIZE(struct BCarPowerupInfo, 0xf4);

/* a keepsake (CBCar_Keepsake.cpp, 0x110 bytes, list at 0x002f0770) */
struct BCarKeepsakeInfo {
	struct BCarEntity *entity;        /* 0x000 */
	struct BCarEntityParams params;   /* 0x004 */
	struct BActor *actor;             /* 0x0d4 MODEL */
	struct BActor *disableModel;      /* 0x0d8 DISABLEMODEL */
	float scale;                      /* 0x0dc SCALE */
	int frame;                        /* 0x0e0 character + 1 of the carrier */
	int colour[3];                    /* 0x0e4 ambient light while drawn */
	struct bdTexture *colourTexture;  /* 0x0f0 PL_COLOUR_TEX */
	float spin;                       /* 0x0f4 SPIN (radians per frame) */
	void *rays;                       /* 0x0f8 the orb rays (classlib_fx CRayControl) */
	float pos[4];                     /* 0x0fc the ray anchor */
	struct BCarKeepsakeInfo *next;    /* 0x10c */
};
OF_ASSERT_SIZE(struct BCarKeepsakeInfo, 0x110);

#define g_PuckList BCB_VAR(struct BCarPuckInfo *, 0x002f0760)
#define g_KeepsakeList BCB_VAR(struct BCarKeepsakeInfo *, 0x002f0770)
#define g_PowerupList BCB_VAR(struct BCarPowerupInfo *, 0x002f0774)
/* 0x002efaa4  the keepsake rays' hit test result (a CRay test context) */
#define g_KeepsakeRayHit BCB_ADDR(0x002efaa4)

/* ======================================================================== explosions */

/* an explosion (CBCar_Explosion.cpp, 0x20 bytes) */
struct BCarExplosion {
	float pos[4];          /* 0x00 */
	float duration;        /* 0x10 frames */
	float time;            /* 0x14 frames */
	float radius;          /* 0x18 */
	struct bdTexture *texture; /* 0x1c the shock wave */
};
OF_ASSERT_SIZE(struct BCarExplosion, 0x20);

struct BCarExplosionNode {
	struct BCarExplosion *explosion;  /* 0x0 */
	struct BCarExplosionNode *next;   /* 0x4 */
};

/* the explosion list (8 bytes, CBCar_Game + 0x4be8 points at it) */
struct BCarExplosions {
	struct BCarExplosionNode *first;  /* 0x0 */
	struct bdTexture *texture;        /* 0x4 debris\textures\shockwave.bmp */
};

/* ======================================================================== waypoints */

/* a waypoint gate (CBCar_Waypoints.cpp, 0x4c bytes, up to 40 at 0x002f0a78) */
struct BCarWaypoint {
	int keeperGate;        /* 0x00 (0x002f0a80) KEEPERGATE: a goal gate of the keeper variant */
	struct bdTexture *texture; /* 0x04 TEXTURE */
	float points[4][4];    /* 0x08 POINT 1-4 */
	void *goal;            /* 0x48 the generic goal of a keeper gate */
};

/* 0x002f0a70  number of waypoints; 0x002f0a74 number of keeper gates; 0x002f0a78 the active
gate, 0x002f0a7c the previous one; 0x002f0a80 the waypoints (stride 0x4c) */
#define g_NumWaypoints BCB_VAR(int, 0x002f0a70)
/* 0x002efa8c  the next finish place of the race (1 first) */
#define g_FinishPlace BCB_VAR(int, 0x002efa8c)
#define g_NumGates BCB_VAR(int, 0x002f0a74)
#define g_ActiveGate BCB_VAR(int, 0x002f0a78)
#define g_PreviousGate BCB_VAR(int, 0x002f0a7c)
#define g_Waypoints ((struct BCarWaypoint *)BCB_ADDR(0x002f0a80))
#define BCB_MAX_WAYPOINTS 40
/* 0x002f1368  the generic goals (CBCar_GenericGoal, 0x28 bytes) */
#define g_GenericGoals BCB_VAR(struct CParticleEffect *, 0x002f1368)
/* 0x002f136c  the two lap-end particle systems (classlib_fx CParticleSystem, 0xa8 bytes) */
#define g_LapEffects ((void **)BCB_ADDR(0x002f136c))
#define g_HelixTexture BCB_VAR(struct bdTexture *, 0x002f1374)
#define g_SparkTexture BCB_VAR(struct bdTexture *, 0x002f1378)
/* 0x002f137c  the active gate's shell */
#define g_GateShell BCB_VAR(struct BActorInstance *, 0x002f137c)

/* ======================================================================== the generic goal */

/* CBCar_GenericGoal (agent A, 0x000a8030-0x000a8250): the goal manager is classlib_fx's
CParticleEffect layout and a goal its CParticleEmitter (0x40 bytes: on 0x00, id 0x04,
colour 0x08, next 0x0c, scale 0x10, time 0x14, actor 0x18, anim 0x1c, systems 0x20,
texture 0x24, drift 0x28, pos 0x2c, link 0x3c). This file only passes them around. */
#define BCB_GOAL_POS(goal) ((float *)((char *)(goal) + 0x2c))

/* (skid tracks, headlights and the floating scores: agent A, 0x000a6260-0x000a74d0) */

/* ======================================================================== the free camera */

/* the debug free camera (0x26c bytes, 0x002f26c4) */
#define g_FreeCamera BCB_VAR(void *, 0x002f26c4)

/* ======================================================================== bdisplay colour tables */

/* the colour shift tables of bdisplay (256 entries): a component value i shifted into its
place of a D3DCOLOR */
extern unsigned int DAT_002bc490[];   /* green  (i << 8) */
extern unsigned int DAT_002bc898[];   /* alpha  (i << 24) */
extern unsigned int DAT_002bce18[];   /* red    (i << 16) */
extern unsigned int DAT_002bd640[];   /* blue   (i) */
/* (the code also reads single entries by their address: 0x002bc988 is alpha[0x3c],
0x002bca94 alpha[0x7f], 0x002bcc94 alpha[0xff], 0x002bc598 green[0x42], 0x002bcf30
red[0x46], 0x002bd6f8 blue[0x2e]; they are written as table entries here) */

/* a vertex of the FVF 0x152 (XYZ | NORMAL | DIFFUSE | TEX1), 0x24 bytes */
struct BCarVertex {
	float pos[3];
	float normal[3];
	unsigned int colour;
	float uv[2];
};
#define BCB_ARGB(a, r, g, b) (DAT_002bc898[a] | DAT_002bce18[r] | DAT_002bc490[g] | DAT_002bd640[b])

/* the colour bytes of a character (classlib_ui table at 0x00290718, 0x60 bytes per character) */
#define BCB_CHAR_COLOUR(character, i) (DAT_00290718[(character) * 0x60 + (i)])

/* the screen size (core) */
extern int DAT_002ba928;
extern int DAT_002ba92c;
/* the frame counter (core) */
extern int DAT_002ba99c;
/* the global ambient light (bactor) */
extern int DAT_00284b10;
extern int DAT_00284b14;
extern int DAT_00284b18;
/* the DirectSound object (engine_misc) */
extern void *DAT_002db808;

/* ======================================================================== other modules */

/* bKernel 0x00051470 (squared length of a vector) and 0x00051540 (squared distance of two
points): bkernel defines them as empty functions (bmNothing1, bmNothing2), which is wrong;
these are the original computations, kept here until bkernel fixes them. */
static inline float bcb_LengthSquared(const float *v)
{
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

static inline float bcb_DistanceSquared(const float *a, const float *b)
{
	float x = a[0] - b[0];
	float y = a[1] - b[1];
	float z = a[2] - b[2];

	return z * z + x * x + y * y;
}

/* bumpercars_a: the debris pieces (ClassLib/Debris.cpp) */
#define bcb_DebrisAdd(type, pos, vel, life, colour) \
	BCB_FN(FUN_00080d60, void *, (void *, float *, float *, int, unsigned int))((type), (pos), (vel), (life), (colour))
/* bumpercars_a: a debris effect of the arena (0x0008cba0, this = 0x002ed4b4); returns the
piece (BCarEffects_Burst in bumpercars_b_internal.h drops the result) */
static inline void *bcb_EffectsAdd(int type, float *pos, float *vel, int arg, int colour)
{
	return BCB_FN(FUN_0008cba0, void *, (void *, int, float *, float *, int, int))(BCB_EFFECTS,
		type, pos, vel, arg, colour);
}

/* bumpercars_a: rumble a pad (0x00080400, classlib_ui) */
#define bcb_PadRumble(player, motor, a3, a4, a5, a6, a7, a8) \
	BCB_FN(FUN_00080400, void, (int, int, int, int, int, int, int, int))((player), (motor), (a3), (a4), (a5), (a6), (a7), (a8))
/* bumpercars_a: CBCar_Camera (0x002efa98) */
#define bcb_CameraDrawHud(view, a3, a4) \
	BCB_FN(FUN_00091422, void, (void *, int, int, float))(BCB_CAMERA, (view), (a3), (a4))

/* classlib_fx: CParticleSystem (0xa8 bytes) and CRay */
#define bcb_ParticleConstruct(ps) BCB_FN(FUN_000a95c0, void *, (void *))(ps)
#define bcb_ParticleInit(ps, params, max) BCB_FN(FUN_000a95d0, int, (void *, void *, int))((ps), (params), (max))
#define bcb_ParticleSetTexture(ps, tex) BCB_FN(FUN_000a9540, void, (void *, void *))((ps), (tex))
#define bcb_ParticleSetTexture2(ps, tex) BCB_FN(FUN_000a8ab0, void, (void *, void *))((ps), (tex))
#define bcb_ParticleSetPosition(ps, pos) BCB_FN(FUN_000a9550, void, (void *, float *))((ps), (pos))
#define bcb_ParticleUpdate(ps, dt) BCB_FN(FUN_000a85f0, void, (void *, float))((ps), (dt))
#define bcb_ParticleDraw(ps, mode, actors) BCB_FN(FUN_000a86e0, void, (void *, int, unsigned int))((ps), (mode), (actors))
#define bcb_ParticleColour(r, g, b) BCB_FN(FUN_000aa020, void, (int, int, int))((r), (g), (b))
#define bcb_RayResetList() BCB_FN(FUN_000a7550, void, (void))()
#define bcb_RayCreateOrb(type, anchor, target, test) \
	BCB_FN(FUN_000a7800, void *, (int, float *, void *, void *))((type), (anchor), (target), (test))
#define bcb_RayAdd(control, angle, speed, length, w0, w1) \
	BCB_FN(FUN_000a76c0, void *, (void *, float, float, float, float, float))((control), (angle), (speed), (length), (w0), (w1))
#define bcb_RayDrawAll() BCB_FN(FUN_000a7900, void, (void))()
#define bcb_RayDestroy(control) BCB_FN(FUN_000a7c70, void, (void *))(control)
#define bcb_RayUpdateAll() BCB_FN(FUN_000a7cd0, void, (void))()
#define bcb_RayDeleteAll() BCB_FN(FUN_000a7d00, void, (void))()
#define bcb_AnimSwitchSet(sw, on) BCB_FN(FUN_000a7dc0, void, (void *, int))((sw), (on))

/* ======================================================================== shared draw blocks */

/* draws an instance mirrored in the floor (the block the item draws repeat): the rotation is
tilted to the ground normal, y is flipped around the ground height, then all is put back */
static inline void bcb_DrawReflected(struct BActorInstance *inst, int passes)
{
	float rot[4];
	float normal[4];
	float height;

	rot[0] = inst->rot[0];
	rot[1] = inst->rot[1];
	rot[2] = inst->rot[2];
	rot[3] = inst->rot[3];
	height = BCarWorld_GetGroundHeight(inst->pos, 0.0f, normal);
	BCarMath_TiltToNormal(inst->rot, inst->rot, normal);
	inst->scale[1] = inst->scale[1] * -1.0f;         /* 0x00253854 */
	inst->pos[1] = inst->pos[1] - (height + height);
	baPrepActorInstance(inst, 1);
	baDrawActorInstance(inst, passes);
	inst->scale[1] = inst->scale[1] * -1.0f;
	inst->pos[1] = (height + height) + inst->pos[1];
	inst->rot[0] = rot[0];
	inst->rot[1] = rot[1];
	inst->rot[2] = rot[2];
	inst->rot[3] = rot[3];
}

/* the glow pass of a moving item: nothing below half the top speed, then brighter with speed.
Returns 0 when the item is not drawn. */
static inline int bcb_SpeedBrightness(struct BCarEntity *e)
{
	float speed = sqrtf(e->vel[2] * e->vel[2] + e->vel[1] * e->vel[1] + e->vel[0] * e->vel[0]);

	if (speed < BCarWorld_GetMaxSpeed() * 0.5f)    /* 0x002536d0 */
		return 0;
	speed = speed - BCarWorld_GetMaxSpeed() * 0.5f;
	baSetBrightness(BCB_FTOL((speed / (BCarWorld_GetMaxSpeed() * 0.5f)) * 127.0f * e->alpha)); /* 0x00255348 */
	return 1;
}

/* a "Bad use" / "Invalid Command" line of a configuration file (status 0 or 2) */
static inline void bcb_BadCommand(struct BCarTextReader *reader, const char *file, int line,
	const char *function, int status, char *word, char *text)
{
	if (status == 0)
		bkPrintf("%s(%d) : %s(\"%s\", %d): Invalid Command \"%s\"\n", file, line, function,
			reader->name, reader->line, word);
	else if (status == 2)
		bkPrintf("%s(%d) : %s(\"%s\", %d): Bad use of Command \"%s\" (%s)\n", file, line + 2,
			function, reader->name, reader->line, word, text);
}

#endif
