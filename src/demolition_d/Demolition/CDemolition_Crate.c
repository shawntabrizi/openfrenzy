/*
Demolition/CDemolition_Crate.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\CDemolition_Crate.cpp

CDemolition_Crate (vtable 0x0026be9c, 0x54 bytes): a platform (demolition_a's CDemoPlatform,
vtable 0x0026a7b0) with a CharLib platform actor. demolition_b's world loader (0x000f7b90)
creates the crates, calls Init (slot 1) and places them; demolition_a's platform system
updates, draws and damages them. A crate in the air falls and bounces until it rests; an
explosion close enough smashes it.

The first function (0x0010d5b0) is the deleting destructor of the CharLib AI base
(unplaced_charlibai.c); the table puts it in this file.
*/

#include "demolition_d.h"

static const char s_CrateFile[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Crate.cpp";

/* 0x002536d8 */
#define CRATE_PROBE_UP 10.0f
/* 0x00265628 */
#define CRATE_GRAVITY 1.2f
/* 0x002536d0 */
#define CRATE_BOUNCE 0.5f
/* 0x00253850: a slower bounce stops */
#define CRATE_MIN_BOUNCE 1.0f

/* demolition_a's sound bank (game +0x79d0): FUN_0018e450 (rollerball_a) plays a sound */
#define CRATE_SOUND_SMASH 9

#define CRATE(p) ((DemoCrate *)(p))

/* 0x0026be9c  the vtable */
static const void *const vtable_0026be9c[7] = {
	(const void *)DemoCrate_DeletingDtor, /* [0] +0x00 */
	(const void *)DemoCrate_Init,         /* [1] +0x04 */
	(const void *)DemoCrate_Shutdown,     /* [2] +0x08 */
	(const void *)DemoCrate_Update,       /* [3] +0x0c */
	(const void *)DemoCrate_Draw,         /* [4] +0x10 */
	(const void *)DemoCrate_Reset,        /* [5] +0x14 */
	(const void *)DemoCrate_Damage,       /* [6] +0x18 */
};

/* the ground height under the crate (probe 10 units above it) */
static float crate_ground(DemoCrate *c)
{
	float probe[3];

	probe[0] = c->pos[0];
	probe[1] = c->pos[1] + CRATE_PROBE_UP;
	probe[2] = c->pos[2];
	/* the original passes a fifth argument 0 that the function does not read */
	return DcWorld_GroundHeight(probe, 0.0f, NULL, NULL);
}

/* 0x0010d5b0  Demolition/CDemolition_Crate.cpp
Scalar deleting destructor of the CharLib AI base (vtable 0x0026bdc0, slot 0x48). */
void *DemoCLAI_DeletingDtor(void *self, unsigned int flags)
{
	DemoCLAI_Dtor(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x0010d5d0  Demolition/CDemolition_Crate.cpp
Constructor. */
void *DemoCrate_Ctor(void *self)
{
	CDemoPlatform_Ctor(self);
	CRATE(self)->vtable = (const void *const *)vtable_0026be9c;
	return self;
}

/* 0x0010d5f0  Demolition/CDemolition_Crate.cpp
Destructor. */
void DemoCrate_Dtor(void *self)
{
	CRATE(self)->vtable = (const void *const *)vtable_0026be9c;
	CDemoPlatform_Dtor(self);
}

/* 0x0010d600  Demolition/CDemolition_Crate.cpp
Slot 2: deletes the actor and frees the collision. */
void DemoCrate_Shutdown(void *self)
{
	DemoCrate *c = CRATE(self);

	Platform_Delete(c->actor);
	bmFreeTreeNode((int *)c->collision);
}

/* 0x0010d620  Demolition/CDemolition_Crate.cpp
A resting crate starts to fall when the ground is below it. */
void DemoCrate_CheckGround(void *self, float dt)
{
	DemoCrate *c = CRATE(self);
	float ground;

	(void)dt;
	ground = crate_ground(c);
	c->ground = ground;
	if (ground < c->pos[1])
		c->state = 1;
}

/* 0x0010d680  Demolition/CDemolition_Crate.cpp
Slot 4: draws the actor. */
void DemoCrate_Draw(void *self)
{
	baDrawActorInstance((struct BActorInstance *)(CRATE(self)->actor + 0x20), 7);
}

/* 0x0010d6a0  Demolition/CDemolition_Crate.cpp
Slot 5: enabled, visible, collides, at rest. */
void DemoCrate_Reset(void *self)
{
	DemoCrate *c = CRATE(self);

	c->enabled = 1;
	c->visible = 1;
	c->collides = 1;
	c->dirty = 1;
	c->state = 0;
	c->vel[0] = 0.0f;
	c->vel[1] = 0.0f;
	c->vel[2] = 0.0f;
	DD_U8(c->actor, 0x3d5) = 1;
}

/* 0x0010d6d0  Demolition/CDemolition_Crate.cpp
Slot 0: scalar deleting destructor. */
void *DemoCrate_DeletingDtor(void *self, unsigned int flags)
{
	DemoCrate_Dtor(self);
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x0010d6f0  Demolition/CDemolition_Crate.cpp
Slot 1: creates the CharLib platform actor from the crate model (game +0x79ac) and its
collision actor (game +0x79b0), adds it to the CharLib actor list (game +0xc8), builds the
animating collision (game +0x79c4), then resets (slot 5). */
void DemoCrate_Init(void *self)
{
	DemoCrate *c = CRATE(self);
	char *actor;
	char *coll;
	struct BActorInstance *inst;

	actor = (char *)bkHeapCalloc(0x470, 0, s_CrateFile, 0x41, 0x2001);
	c->actor = actor;
	DD_U8(actor, 0x3d5) = 1;
	c->width = 140.0f;
	c->height = 200.0f;
	baInitActorInstance((struct BActorInstance *)(c->actor + 0x20),
	                    (struct BActor *)DD_PTR(DD_GAME, DG_CRATEACTOR), 1, s_CrateFile, 0x4a);
	inst = baInitActorInstance(NULL, (struct BActor *)DD_PTR(DD_GAME, DG_CRATECOLL), 1,
	                           s_CrateFile, 0x4d);
	DD_AT(c->actor, 0x190, struct BActorInstance *) = inst;
	DD_I32(c->actor, 0x22c) = 2;
	actor = c->actor;
	DD_F32(actor, 0x100) = 0.0f;
	DD_F32(actor, 0x104) = 0.0f;
	DD_F32(actor, 0x108) = 0.0f;
	DD_F32(actor, 0x10c) = 1.0f;
	coll = DD_PTR(c->actor, 0x190);
	DD_F32(coll, 0xec) = 1.0f;
	DD_F32(coll, 0xe0) = 0.0f;
	DD_F32(coll, 0xe4) = 0.0f;
	DD_F32(coll, 0xe8) = 0.0f;
	baPrepActorInstance((struct BActorInstance *)(c->actor + 0x20), 1);
	baPrepActorInstance(DD_AT(c->actor, 0x190, struct BActorInstance *), 1);
	CRColl_BuildMeshFrom(c->actor + 0x10, DD_PTR(c->actor, 0x190));
	CLList_Insert((int *)(DD_GAME + 0xc8), (int *)c->actor, 0);
	bmCreateAnimatingCollision((struct BAnimatingCollision *)c->collision,
	                           (unsigned char *)DD_PTR(DD_GAME, DG_CRATECOLLRES),
	                           (unsigned char *)DD_PTR(c->actor, 0x190));
	DD_VFN(c, 0x14, void (*)(void *))(c);
}

/* 0x0010d830  Demolition/CDemolition_Crate.cpp
A falling crate: gravity, then on the ground it bounces with half its speed, or stops when
the bounce is slow. The platform position (and its actor's) is updated. */
void DemoCrate_Fall(void *self, float dt)
{
	DemoCrate *c = CRATE(self);

	c->ground = crate_ground(c);
	c->vel[1] = c->vel[1] - dt * CRATE_GRAVITY;
	c->pos[1] = dt * c->vel[1] + c->pos[1];
	if (c->pos[1] < c->ground) {
		c->pos[1] = c->ground;
		c->vel[0] = c->vel[0] * CRATE_BOUNCE;
		c->vel[1] = c->vel[1] * CRATE_BOUNCE;
		c->vel[2] = c->vel[2] * CRATE_BOUNCE;
		if (fabsf(c->vel[1]) > CRATE_MIN_BOUNCE) {
			c->vel[0] = -c->vel[0];
			c->vel[1] = -c->vel[1];
			c->vel[2] = -c->vel[2];
			CDemoPlatform_SetColour(c, c->pos);
			return;
		}
		c->state = 0;
		c->vel[1] = 0.0f;
	}
	CDemoPlatform_SetColour(c, c->pos);
}

/* 0x0010d960  Demolition/CDemolition_Crate.cpp
Slot 6: an explosion at POS with RADIUS. Inside the radius (around the crate's middle), the
crate disappears, its actor is smashed, sparks fly away from the explosion and a sound plays.
The original pre-tests each axis difference (signed, not absolute) against radius squared. */
void DemoCrate_Damage(void *self, float *pos, float radius)
{
	DemoCrate *c = CRATE(self);
	float rsq;
	float dx;
	float dy;
	float dz;
	float dir[3];

	rsq = radius * radius;
	dx = pos[0] - c->pos[0];
	if (dx > rsq)
		return;
	dz = pos[2] - c->pos[2];
	if (dz > rsq)
		return;
	dy = pos[1] - (c->height * 0.5f + c->pos[1]);
	if (dy > rsq)
		return;
	if (!(sqrtf(dy * dy + dz * dz + dx * dx) < radius))
		return;
	c->enabled = 0;
	c->visible = 0;
	Platform_Smash(c->actor, 1);
	dir[0] = c->pos[0] - pos[0];
	dir[1] = c->pos[1] - pos[1];
	dir[2] = c->pos[2] - pos[2];
	bmVectorNormalise(dir, dir);
	CDemoSpotFX_StartBurst(DD_GAME + 0x1510, c->pos, dir);
	/* the original advances the random seed here and does not use the value */
	(void)dd_rand();
	FUN_0018e450((int)(DD_GAME + 0x79d0), CRATE_SOUND_SMASH, 0xff, 0, 0, 0);
}

/* 0x0010db40  Demolition/CDemolition_Crate.cpp
Slot 3: resting crates check the ground, falling crates fall; then the platform update. */
void DemoCrate_Update(void *self, float dt)
{
	DemoCrate *c = CRATE(self);

	if (c->state == 0) {
		DemoCrate_CheckGround(c, dt);
	} else if (c->state == 1) {
		DemoCrate_Fall(c, dt);
		CDemoPlatform_Update(c, dt);
		return;
	}
	CDemoPlatform_Update(c, dt);
}
