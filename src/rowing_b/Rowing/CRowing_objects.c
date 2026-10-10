/*
Rowing/CRowing_objects.c  <-  C:\BlitzParty\Game\Src\Rowing\CRowing_objects.cpp

The scenery objects of the Rowing level. A ring of models (struct RowModel, one per object
file, found by the file name's CRC) each holds a ring of instances (struct RowObject and its
special kinds). The model's type comes from the file name (table at 0x002b5998):

   1 crane, 2 overhead crane   RowCrane (0x38 bytes, vtable 0x002765ec): animated by a
                               cycle of animation segments, a signal light
   3-13 floating objects       RowFloaty (0xe8 bytes, vtable 0x002762c4): cow and raft,
                               raft, the three power-ups, mine, crates, jerry can, barrel,
                               buoy; they drift down the river and bounce off its banks
   14 buoy xref                RowBuoy (0x44 bytes, vtable 0x0027658c): a buoy that bobs,
                               tilts when hit and blinks
   15 race buoy                RowRaceBuoy (0xa0 bytes, vtable 0x002765d4): a gate buoy with
                               two floating ends and a light per player
   others                      RowObject (0x20 bytes, vtable 0x00276224)

0x001b6330, 0x001b6730 and 0x001b6a90 are tagged with CRowing_objects.cpp; 0x001b6270-
0x001b6b50 and the classes' methods 0x001b6fe0-0x001b9140 (gap functions) are placed here
(file inferred). The collision callbacks 0x001b7000, 0x001b71f0, 0x001b72b0, 0x001b7700 and
0x001b8680 are missing from the function table.

The special objects are reached with raw offsets (RO_*) from their RowObject base.
*/

#include "rowing_b.h"

/* the crane (0x38 bytes) */
#define RO_CRANE_TIMER 0x20         /* ms since the animation ended */
#define RO_CRANE_WAIT 0x24          /* ms to wait before the next one */
#define RO_CRANE_KIND 0x28          /* 1 crane, 2 overhead crane */
#define RO_CRANE_SEGMENT 0x2c       /* the next animation segment */
#define RO_CRANE_DONE 0x30          /* the animation ended */
#define RO_CRANE_LIGHT 0x34         /* struct RowLight * */

/* the buoy xref (0x44 bytes) */
#define RO_BUOY_TILT 0x20           /* x, z */
#define RO_BUOY_VEL 0x28            /* x, z */
#define RO_BUOY_PHASE 0x30
#define RO_BUOY_BOB 0x34
#define RO_BUOY_Y 0x38
#define RO_BUOY_BLINK 0x3c          /* ms in the blink step */
#define RO_BUOY_STATE 0x40          /* bit 0 colour, bits 1-3 the blink step */

/* the race buoy (0xa0 bytes) */
#define RO_RACE_END1 0x20           /* the two floating ends (pos[4]) */
#define RO_RACE_END2 0x30
#define RO_RACE_ROT 0x40            /* the placed rotation (quaternion) */
#define RO_RACE_TILT 0x50
#define RO_RACE_VEL 0x58
#define RO_RACE_PHASE 0x60
#define RO_RACE_BOB 0x64
#define RO_RACE_Y 0x68
#define RO_RACE_NODES 0x6c          /* olight, glight, blight, plight, slightl, slightr */
#define RO_RACE_PLAYER 0x84         /* the players' states for its gate (4) */
#define RO_RACE_GATE 0x94

/* the floating object (0xe8 bytes); +0x28 is a rowing_a physics body */
#define RO_FLOAT_BODY 0x28
#define RO_FLOAT_POS 0x34
#define RO_FLOAT_SPIN 0x80
#define RO_FLOAT_VEL 0x84
#define RO_FLOAT_MOVE 0x94          /* this frame's movement */
#define RO_FLOAT_SUNK 0xca
#define RO_FLOAT_COLLIDER 0xdc
#define RO_FLOAT_HIT 0xe0
#define RO_FLOAT_GLOW 0xe4

/* the sprite textures of the three special kinds (tex_map\sun.bmp) */
#define g_RowBuoySprite RW_VAR(void *, 0x0030fe90)
#define g_RowCraneSprite RW_VAR(void *, 0x0030fe8c)
#define g_RowRaceBuoySprite RW_VAR(void *, 0x0030fe88)

/* the model types by file name (0x002b5998) */
struct RowModelType {
	const char *name;
	int type;
};
#define g_RowModelTypes ((struct RowModelType *)RW_ADDR(0x002b5998))
#define g_RowModelTypeCount RW_VAR(int, 0x002b5a10)
/* the crane's animation segments: 4 for a crane (0x002b5b08), 2 for an overhead crane
   (0x002b5b18) */
#define g_RowCraneSegments ((const char **)RW_ADDR(0x002b5b08))
#define g_RowOverheadSegments ((const char **)RW_ADDR(0x002b5b18))

/* 0x002758a8  the characters' colours (red, green, blue ints) */
static const int rb_character_colours[6][3] = {
	{ 0x2e, 0xa7, 0x24 },
	{ 0xd2, 0x00, 0x00 },
	{ 0x20, 0x71, 0xcd },
	{ 0xf8, 0xce, 0x00 },
	{ 0xf2, 0x82, 0x1e },
	{ 0xec, 0x6b, 0xae },
};

/* 0x00276708, 0x00276718  the race buoy's ends relative to it */
static float rb_race_end1[4] = { -220.0f, 0.0f, 0.0f, 1.0f };
static float rb_race_end2[4] = { 220.0f, 0.0f, 0.0f, 1.0f };

/* the characters of players 1-4 (core: 10 ints a player) */
extern int DAT_002baed4[];

/* the body methods of rowing_a (thiscall on the floating object's +0x28) */
#define rb_body(self) RB_ADDR(self, RO_FLOAT_BODY)

/* 0x00276224  vtable of RowObject */
const void *const vtable_00276224[6] = {
	(const void *)RowObject_DeletingDtor,       /* +0x00 */
	(const void *)RowObject_Init,               /* +0x04 */
	(const void *)RowObject_Update,             /* +0x08 */
	(const void *)RowObject_Draw,               /* +0x0c */
	(const void *)FUN_001d5950,                 /* +0x10 tankedup_b: nothing */
	(const void *)FUN_001d5950,                 /* +0x14 */
};

/* 0x002762c4  vtable of RowFloaty */
const void *const vtable_002762c4[6] = {
	(const void *)RowFloaty_DeletingDtor,
	(const void *)RowObject_Init,
	(const void *)RowFloaty_Update,
	(const void *)RowFloaty_Draw,
	(const void *)FUN_001d5950,
	(const void *)RowFloaty_SetHit,
};

/* 0x0027658c  vtable of RowBuoy */
const void *const vtable_0027658c[6] = {
	(const void *)RowBuoy_DeletingDtor,
	(const void *)RowObject_Init,
	(const void *)RowBuoy_Update,
	(const void *)RowBuoy_Draw,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
};

/* 0x002765d4  vtable of RowRaceBuoy */
const void *const vtable_002765d4[6] = {
	(const void *)RowBuoy_DeletingDtor,
	(const void *)RowObject_Init,
	(const void *)RowRaceBuoy_Update,
	(const void *)RowRaceBuoy_Draw,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
};

/* 0x002765ec  vtable of RowCrane */
const void *const vtable_002765ec[6] = {
	(const void *)RowCrane_DeletingDtor,
	(const void *)RowObject_Init,
	(const void *)RowCrane_Update,
	(const void *)RowCrane_Draw,
	(const void *)FUN_001d5950,
	(const void *)RowCrane_AnimDone,
};

/* ---------- RowObject and the model ring */

/* 0x001b6270  Rowing (gap; CRowing_objects.cpp inferred)
The draw passes (bits 1, 2, 4) that a node ring and its children have meshes for. */
unsigned int RowNode_GetPasses(void *node)
{
	unsigned char *first = node;
	unsigned char *n = first;
	unsigned char *mesh;
	unsigned int passes = 0;

	do {
		if ((RB_U8(n, 0x7a) & 0x10) == 0) {
			mesh = RB_PTR(n, 0x7c);
			if (RB_U8(mesh, 0xf0) == 2) {
				if (RB_PTR(mesh, 0x84) != NULL)
					passes |= 1;
				if (RB_PTR(mesh, 0x88) != NULL)
					passes |= 2;
				if (RB_PTR(mesh, 0x8c) != NULL)
					passes |= 4;
			}
		}
		if (RB_PTR(n, 0x14c) != NULL)
			passes |= RowNode_GetPasses(RB_PTR(n, 0x14c));
		n = RB_PTR(n, 0x140);
	} while (n != first);
	return passes;
}

/* 0x001b62f0  Rowing (gap; CRowing_objects.cpp inferred) */
struct RowObject *RowObject_ctor(struct RowObject *self)
{
	self->vtable = vtable_00276224;
	self->inst = NULL;
	return self;
}

/* 0x001b6300  Rowing (gap; CRowing_objects.cpp inferred) */
void RowObject_dtor(struct RowObject *self)
{
	self->vtable = vtable_00276224;
	if (self->inst != NULL) {
		baDeleteActorInstance(self->inst);
		self->inst = NULL;
	}
}

/* 0x001b6330  Rowing/CRowing_objects.cpp
Makes the actor instance, starts its "spin" (power-ups) or "default" animation looping,
else "default_noloop" once, and finds its draw passes (all when the actor has flag 1). */
int RowObject_Init(struct RowObject *self, void *actor)
{
	const char *name;
	void *anim;
	int loop;

	if (self->inst != NULL) {
		bkPrintf("ERROR: Actor instance already present at %p\n", (void *)self->inst);
		return 0;
	}
	self->inst = baInitActorInstance(NULL, actor, 2, RB_FILE_OBJECTS, 0x58);
	if (self->type == 7 || self->type == 6 || self->type == 5)
		name = "spin";
	else
		name = "default";
	anim = baFindSubObject(self->inst, name);
	self->anim = anim;
	if (anim == NULL) {
		anim = baFindSubObject(self->inst, "default_noloop");
		self->anim = anim;
		if (anim == NULL)
			goto done;
		loop = 0;
	} else {
		loop = 1;
	}
	baQueueAnimation(self->inst, RB_PTR(self->inst, 0x134), anim, 0x400, -1, (uint32_t)loop, NULL,
		NULL);
done:
	self->flags = self->flags | 1;
	if ((RB_U8(actor, 0x44) & 1) != 0) {
		self->passes = 7;
		return 1;
	}
	self->passes = RowNode_GetPasses(RB_PTR(self->inst, 0x134));
	return 1;
}

/* 0x001b6420  Rowing/CRowing_objects.cpp (bracketed)
Moves the animation on by dt and prepares the instance. */
int RowObject_Update(struct RowObject *self, float dt)
{
	if ((self->flags & 1) != 0) {
		if (self->anim != NULL)
			baUpdateAnimations(self->inst, RB_PTR(self->inst, 0x134), dt, NULL);
		baPrepActorInstance(self->inst, 1);
	}
	return 1;
}

/* 0x001b6470  Rowing/CRowing_objects.cpp (bracketed)
Draws the instance in `pass` when it has meshes for it. */
void RowObject_Draw(struct RowObject *self, unsigned int pass)
{
	if ((self->flags & 1) != 0 && (self->passes & pass) != 0)
		baDrawActorInstance(self->inst, (int)pass);
}

/* 0x001b6490  Rowing/CRowing_objects.cpp (bracketed)
Deletes the model's instances (the first one last) and the actor (when it was loaded from
a package). */
void RowModel_Free(struct RowModel *self)
{
	struct RowObject *obj;
	struct RowObject *next;

	if (self->instances == NULL)
		return;
	next = self->instances->next;
	while (next != self->instances) {
		obj = next;
		next = next->next;
		RB_VFN(obj, 0x00, void *(*)(void *, unsigned int))(obj, 1);
	}
	if (self->shared == 0)
		bkDeleteResource(self->actor);
	if (self->instances != NULL)
		RB_VFN(self->instances, 0x00, void *(*)(void *, unsigned int))(self->instances, 1);
	self->instances = NULL;
}

/* 0x001b64e0  Rowing/CRowing_objects.cpp (bracketed)
Loads the model's actor and finds its type from the file name. Returns 0 when it cannot
be loaded. */
int RowModel_Load(struct RowModel *self, void *package, const char *name)
{
	int i;

	self->actor = bLoadResource(package, name, 1, 0xdefa);
	self->shared = package == NULL;
	if (self->actor == NULL) {
		bkPrintf("CERROR: Could not load actor [%s]\n", name);
		return 0;
	}
	self->type = 0;
	for (i = 0; i < g_RowModelTypeCount; i++) {
		if (strstr(name, g_RowModelTypes[i].name) != NULL) {
			self->type = g_RowModelTypes[i].type;
			break;
		}
	}
	self->crc = bkCRCString(name);
	return 1;
}

/* 0x001b6580  Rowing/CRowing_objects.cpp (bracketed)
The floating object's constructor (its physics body is rowing_a's). (The original guards
it with a C++ exception frame.) */
void *RowFloaty_ctor(void *self)
{
	RB_PTR(self, 0x0c) = NULL;
	RB_PTR(self, 0x00) = (void *)vtable_002762c4;
	RB_CALL(FUN_001a5750, void (*)(void *))(rb_body(self));
	return self;
}

/* 0x001b65d0  Rowing/CRowing_objects.cpp (bracketed)
Puts an instance at the end of the model's ring. */
void RowModel_AddInstance(struct RowModel *self, struct RowObject *obj)
{
	if (self->instances == NULL) {
		obj->next = obj;
		obj->prev = obj;
		self->instances = obj;
		return;
	}
	self->instances->prev->next = obj;
	obj->prev = self->instances->prev;
	obj->next = self->instances;
	self->instances->prev = obj;
}

/* 0x001b6610  Rowing/CRowing_objects.cpp (bracketed)
Updates every instance of every model. */
int RowObjects_Update(void *list, float dt)
{
	struct RowModel **head = list;
	struct RowModel *model = *head;
	struct RowObject *obj;

	if (model == NULL)
		return 1;
	do {
		obj = model->instances;
		if (obj != NULL) {
			do {
				RB_VFN(obj, 0x08, int (*)(void *, float))(obj, dt);
				obj = obj->next;
			} while (obj != model->instances);
		}
		model = model->next;
	} while (model != *head);
	return 1;
}

/* 0x001b6650  Rowing/CRowing_objects.cpp (bracketed)
Draws every instance of every model in `pass`. */
void RowObjects_Draw(void *list, unsigned int pass)
{
	struct RowModel **head = list;
	struct RowModel *model = *head;
	struct RowObject *obj;

	if (model == NULL)
		return;
	do {
		obj = model->instances;
		if (obj != NULL) {
			do {
				RB_VFN(obj, 0x0c, void (*)(void *, unsigned int))(obj, pass);
				obj = obj->next;
			} while (obj != model->instances);
		}
		model = model->next;
	} while (model != *head);
}

/* 0x001b6690  Rowing/CRowing_objects.cpp (bracketed) scalar deleting destructor */
struct RowObject *RowObject_DeletingDtor(struct RowObject *self, unsigned int flags)
{
	self->vtable = vtable_00276224;
	if (self->inst != NULL) {
		baDeleteActorInstance(self->inst);
		self->inst = NULL;
	}
	if (flags & 1)
		RB_DELETE(self);
	return self;
}

/* 0x001b66d0  Rowing/CRowing_objects.cpp (bracketed) */
void RowObject_SetPos(struct RowObject *self, float *pos)
{
	memcpy(RB_ADDR(self->inst, 0xc0), pos, 16);
}

/* 0x001b6700  Rowing/CRowing_objects.cpp (bracketed) */
void RowObject_SetRot(struct RowObject *self, float *rot)
{
	memcpy(RB_ADDR(self->inst, 0xe0), rot, 16);
}

/* 0x001b6730  Rowing/CRowing_objects.cpp
Makes an instance of the model at pos with rotation rot, of the model type's class.
Returns NULL when the actor is not loaded or the instance cannot be set up (the failed
special object stays in the ring). (The original guards the allocations with a C++
exception frame.) */
struct RowObject *RowModel_CreateInstance(struct RowModel *self, float *pos, float *rot)
{
	struct RowObject *obj = NULL;
	void *mem;

	if (self->actor == NULL) {
		bkPrintf("CERROR: Actor not loaded\n");
		return NULL;
	}
	switch (self->type) {
	case 1:
	case 2:
		mem = RB_NEW(0x38, RB_FILE_OBJECTS, 0x13c, 0x1001);
		obj = mem != NULL ? RowCrane_ctor(mem) : NULL;
		obj->type = self->type;
		RowModel_AddInstance(self, obj);
		if (RowCrane_Init(obj, self->actor, self->type) == 0)
			return NULL;
		RowObject_SetPos(obj, pos);
		RowObject_SetRot(obj, rot);
		break;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
		mem = RB_NEW(0xe8, RB_FILE_OBJECTS, 0x154, 0x1001);
		obj = mem != NULL ? RowFloaty_ctor(mem) : NULL;
		obj->type = self->type;
		RowModel_AddInstance(self, obj);
		if (RowFloaty_Init(obj, self->actor, self->type, pos, rot) == 0)
			return NULL;
		RowObject_SetPos(obj, pos);
		RowObject_SetRot(obj, rot);
		break;
	case 14:
		mem = RB_NEW(0x44, RB_FILE_OBJECTS, 0x162, 0x1001);
		obj = mem != NULL ? RowBuoy_ctor(mem) : NULL;
		obj->type = self->type;
		RowModel_AddInstance(self, obj);
		if (RowBuoy_Init(obj, self->actor, pos, rot) == 0)
			return NULL;
		RowObject_SetPos(obj, pos);
		RowObject_SetRot(obj, rot);
		break;
	case 15:
		mem = RB_NEW(0xa0, RB_FILE_OBJECTS, 0x16e, 0x1001);
		obj = mem != NULL ? RowRaceBuoy_ctor(mem) : NULL;
		obj->type = self->type;
		RowModel_AddInstance(self, obj);
		if (RowRaceBuoy_Init(obj, self->actor, pos, rot) == 0)
			return NULL;
		RowObject_SetPos(obj, pos);
		RowObject_SetRot(obj, rot);
		break;
	default:
		mem = RB_NEW(0x20, RB_FILE_OBJECTS, 0x17b, 0x1001);
		if (mem != NULL)
			obj = RowObject_ctor(mem);
		obj->type = self->type;
		RowModel_AddInstance(self, obj);
		if (RB_VFN(obj, 0x04, int (*)(void *, void *))(obj, self->actor) == 0)
			return NULL;
		memcpy(RB_ADDR(obj->inst, 0xc0), pos, 16);
		memcpy(RB_ADDR(obj->inst, 0xe0), rot, 16);
		break;
	}
	obj->type = self->type;
	return obj;
}

/* 0x001b6a70  Rowing/CRowing_objects.cpp (bracketed) the floating object's scalar deleting
destructor */
void *RowFloaty_DeletingDtor(void *self, unsigned int flags)
{
	RowFloaty_dtor(self);
	if (flags & 1)
		RB_DELETE(self);
	return self;
}

/* 0x001b6a90  Rowing/CRowing_objects.cpp
Adds an instance of the model `name` (loaded from `package` the first time; the model is
put in the ring even when it cannot be loaded) at pos with rotation rot. Returns the
instance, or NULL. */
void *RowObjects_Add(void *list, void *package, const char *name, float *pos, float *rot)
{
	struct RowModel **head = list;
	struct RowModel *model;
	unsigned int crc;

	if (*head != NULL) {
		crc = bkCRCString(name);
		model = *head;
		do {
			if (crc == model->crc)
				return RowModel_CreateInstance(model, pos, rot);
			model = model->next;
		} while (model != *head);
	}
	model = RB_NEW(0x1c, RB_FILE_OBJECTS, 0x246, 0x1001);
	if (model != NULL) {
		model->instances = NULL;
		model->crc = 0;
	}
	if (*head == NULL) {
		model->next = model;
		model->prev = model;
		*head = model;
	} else {
		(*head)->prev->next = model;
		model->prev = (*head)->prev;
		model->next = *head;
		(*head)->prev = model;
	}
	if (RowModel_Load(model, package, name) == 0)
		return NULL;
	return RowModel_CreateInstance(model, pos, rot);
}

/* 0x001b6b50  Rowing (gap; CRowing_objects.cpp inferred)
Frees every model (the first one last) and empties the ring. */
void RowObjects_Free(void *list)
{
	struct RowModel **head = list;
	struct RowModel *model;
	struct RowModel *next;
	struct RowModel *first;

	if (*head == NULL)
		return;
	model = (*head)->next;
	while (model != *head) {
		next = model->next;
		RowModel_Free(model);
		RB_DELETE(model);
		model = next;
	}
	first = *head;
	if (first != NULL) {
		RowModel_Free(first);
		RB_DELETE(first);
	}
	*head = NULL;
}

/* ---------- RowBuoy (type 14) */

/* loads tex_map\sun.bmp into `slot` when it is empty; 0 when it cannot */
static int rb_load_sun(void **slot)
{
	char name[16];

	if (*slot != NULL)
		return 1;
	memcpy(name, "tex_map\\sun.bmp", 16);
	*slot = bLoadResource(NULL, name, 0, 0xdefa);
	if (*slot == NULL) {
		bkPrintf("ERROR: Could not find sprite %s\n", name);
		return 0;
	}
	return 1;
}

/* 0x001b6fe0  Rowing (gap; CRowing_objects.cpp inferred) */
struct RowObject *RowBuoy_ctor(void *self)
{
	struct RowObject *obj = RowObject_ctor(self);

	obj->vtable = vtable_0027658c;
	g_RowBuoySprite = NULL;
	RB_F32(obj, RO_BUOY_BLINK) = 0.0f;
	RB_U32(obj, RO_BUOY_STATE) = 0;
	return obj;
}

/* the push of a boat hitting a buoy: its speeds x 0.01 (x 0.0025 for a race buoy end),
   clamped to +-0.2 */
static void rb_buoy_push(float *vel, unsigned char *boat, float scale)
{
	vel[0] = RB_F32(boat, 0x30c) * scale;
	vel[1] = RB_F32(boat, 0x304) * scale;
	if (0.200000003f < vel[0])                                      /* 0x00257c7c */
		vel[0] = 0.200000003f;
	if (0.200000003f < vel[1])
		vel[1] = 0.200000003f;
	if (vel[0] < -0.200000003f)                                     /* 0x00268aac */
		vel[0] = -0.200000003f;
	if (vel[1] < -0.200000003f)
		vel[1] = -0.200000003f;
}

/* 0x001b7000  Rowing (missing from the function table; CRowing_objects.cpp inferred)
Collision callback of a buoy: a boat that hits it pushes it (tilts it). Returns whether
it hit. */
int FUN_001b7000(void *self, void *boat, void *a, void *b, float *out)
{
	struct RowObject *obj = self;
	int hit;

	hit = rb_BoatHitsSphere(a, RB_FP(obj->inst, 0xc0), 62.4410019f, b);    /* 0x4279c396 */
	if (hit != 0) {
		out[3] = 0.0f;
		out[2] = 0.0f;
		out[1] = 0.0f;
		out[0] = 0.0f;
		rb_buoy_push(RB_FP(obj, RO_BUOY_VEL), boat, 0.00999999978f);  /* 0x0025372c */
	}
	return hit;
}

/* 0x001b70d0  Rowing (gap; CRowing_objects.cpp inferred)
The buoy's blinking light: a sprite at its "clight" node, red and white, growing and
shrinking with the blink step. */
void RowBuoy_DrawLight(struct RowObject *self)
{
	char node[8];
	float pos[4];
	int colour[4];
	float size;
	void *n;

	bdSetWorldMatrix((float *)DAT_002855f0);
	switch (RB_U32(self, RO_BUOY_STATE) >> 1) {
	case 0:
		size = 5.0f;
		break;
	case 1:
		size = RB_F32(self, RO_BUOY_BLINK) * 35.0f * 0.00200000009f;   /* 0x002630a8, 0x002672b0 */
		if (!(5.0f <= size))
			size = 5.0f;
		break;
	case 2:
		size = 35.0f;
		break;
	default:
		size = (500.0f - RB_F32(self, RO_BUOY_BLINK)) * 35.0f * 0.00200000009f;
		if (!(5.0f <= size))
			size = 5.0f;
		break;
	}
	memcpy(node, "clight", 7);
	n = baFindNode(RB_PTR(self->inst, 0x134), node);
	if (n == NULL) {
		bkPrintf("ERROR Could not find buoy node [%s]\n", node);
		return;
	}
	baGetNodePosition(self->inst, n, pos);
	colour[0] = 0xff;
	colour[1] = (RB_U8(self, RO_BUOY_STATE) & 1) ? 0 : 0x40;
	colour[2] = 0;
	colour[3] = 0xff;
	RowSprites_Add(2, pos, size, size, 0.0f, g_RowBuoySprite, colour, -0.00499999989f);
}

/* 0x001b71f0  Rowing (missing from the function table; CRowing_objects.cpp inferred)
Collision callback of a race buoy's first end. */
int FUN_001b71f0(void *self, void *boat, void *a, void *b, float *out)
{
	int hit;

	hit = rb_BoatHitsSphere(a, RB_FP(self, RO_RACE_END1), 62.4410019f, b);
	if (hit != 0) {
		out[3] = 0.0f;
		out[2] = 0.0f;
		out[1] = 0.0f;
		out[0] = 0.0f;
		rb_buoy_push(RB_FP(self, RO_RACE_VEL), boat, 0.00249999994f);  /* 0x002699d0 */
	}
	return hit;
}

/* 0x001b72b0  Rowing (missing from the function table; CRowing_objects.cpp inferred)
Collision callback of a race buoy's second end. */
int FUN_001b72b0(void *self, void *boat, void *a, void *b, float *out)
{
	int hit;

	hit = rb_BoatHitsSphere(a, RB_FP(self, RO_RACE_END2), 62.4410019f, b);
	if (hit != 0) {
		out[3] = 0.0f;
		out[2] = 0.0f;
		out[1] = 0.0f;
		out[0] = 0.0f;
		rb_buoy_push(RB_FP(self, RO_RACE_VEL), boat, 0.00249999994f);
	}
	return hit;
}

/* 0x001b7370  Rowing (gap; CRowing_objects.cpp inferred) */
struct RowObject *RowRaceBuoy_ctor(void *self)
{
	struct RowObject *obj = RowObject_ctor(self);

	obj->vtable = vtable_002765d4;
	g_RowRaceBuoySprite = NULL;
	return obj;
}

/* 0x001b7390  Rowing (gap; CRowing_objects.cpp inferred)
Reads every player's state for the buoy's gate. Returns 0 when it has no gate. */
int RowRaceBuoy_UpdateLights(struct RowObject *self)
{
	int i;

	if (RB_I32(self, RO_RACE_GATE) == -1)
		return 0;
	for (i = 0; i < DAT_002bae94; i++)
		RB_I32(self, RO_RACE_PLAYER + i * 4) =
			(int)RB_I8(g_RowPlayers[i], 0x110 + RB_I32(self, RO_RACE_GATE));
	return 1;
}

/* 0x001b73e0  Rowing (gap; CRowing_objects.cpp inferred)
The race buoy's player lights: a sprite in each player's colour at their node, large (1:
passed), none (2) or small and dim (others). */
void RowRaceBuoy_DrawLights(struct RowObject *self)
{
	float pos[4];
	int colour[4];
	float size;
	int character;
	int i;

	bdSetWorldMatrix((float *)DAT_002855f0);
	colour[3] = 0xff;
	for (i = 0; i < 4; i++) {
		void *node = RB_PTR(self, RO_RACE_NODES + i * 4);

		if (node == NULL)
			continue;
		baGetNodePosition(self->inst, node, pos);
		character = DAT_002baed4[i * 10];
		colour[0] = rb_character_colours[character][0];
		colour[1] = rb_character_colours[character][1];
		colour[2] = rb_character_colours[character][2];
		if (RB_I32(self, RO_RACE_PLAYER + i * 4) == 1) {
			size = 60.0f;
		} else {
			if (RB_I32(self, RO_RACE_PLAYER + i * 4) == 2)
				continue;
			colour[0] = colour[0] / 2;
			colour[1] = colour[1] / 2;
			colour[2] = colour[2] / 2;
			size = 30.0f;
		}
		RowSprites_Add(2, pos, size, size, 0.0f, g_RowRaceBuoySprite, colour, -0.00499999989f);
	}
}

/* ---------- RowCrane (types 1 and 2) */

/* 0x001b7700  Rowing (missing from the function table; CRowing_objects.cpp inferred)
Animation callback: tells the object (vtable slot 5) that its animation ended. */
int FUN_001b7700(int a, int b, int c, int d, void *context)
{
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	RB_VFN(context, 0x14, void (*)(void *))(context);
	return 1;
}

/* 0x001b7710  Rowing (gap; CRowing_objects.cpp inferred)
(The original constructs and destroys a temporary RowObject here, and guards the
constructor with a C++ exception frame.) */
struct RowObject *RowCrane_ctor(void *self)
{
	struct RowObject *obj = RowObject_ctor(self);
	struct RowObject temp;

	obj->vtable = vtable_002765ec;
	g_RowCraneSprite = NULL;
	RowObject_ctor(&temp);
	RowObject_dtor(&temp);
	return obj;
}

/* 0x001b7770  Rowing (gap; CRowing_objects.cpp inferred)
(The original runs the base destructor twice.) */
void RowCrane_dtor(struct RowObject *self)
{
	self->vtable = vtable_002765ec;
	RowObject_dtor(self);
	RowObject_dtor(self);
}

/* 0x001b77c0  Rowing (gap; CRowing_objects.cpp inferred) vtable slot 5 */
void RowCrane_AnimDone(struct RowObject *self)
{
	RB_I32(self, RO_CRANE_DONE) = 1;
}

/* 0x001b77d0  Rowing (gap; CRowing_objects.cpp inferred)
Plays the crane's next animation segment once (big and small turns for a crane, forward
and backward for an overhead crane). Returns 0 when it is missing. */
int RowCrane_NextAnim(struct RowObject *self)
{
	const char *name;
	int count;

	baFlushAnimations(self->inst, RB_PTR(self->inst, 0x134), 1, 0);
	if (RB_I32(self, RO_CRANE_KIND) == 1) {
		self->anim = baFindSubObject(self->inst, g_RowCraneSegments[RB_I32(self, RO_CRANE_SEGMENT)]);
		count = 4;
	} else if (RB_I32(self, RO_CRANE_KIND) == 2) {
		name = g_RowOverheadSegments[RB_I32(self, RO_CRANE_SEGMENT)];
		self->anim = baFindSubObject(self->inst, name);
		count = 2;
	} else {
		bkPrintf("ERROR: Unhandled Crane type %d\n", RB_I32(self, RO_CRANE_KIND));
		return 0;
	}
	if (self->anim == NULL) {
		/* (the original names the crane's segment in both cases) */
		bkPrintf("ERROR: Could not find anim segment [%s]\n",
			g_RowCraneSegments[RB_I32(self, RO_CRANE_SEGMENT)]);
		return 0;
	}
	RB_I32(self, RO_CRANE_SEGMENT) = RB_I32(self, RO_CRANE_SEGMENT) + 1;
	if (RB_I32(self, RO_CRANE_SEGMENT) >= count)
		RB_I32(self, RO_CRANE_SEGMENT) = 0;
	baQueueAnimation(self->inst, RB_PTR(self->inst, 0x134), self->anim, 0, -1, 0,
		(int (*)())FUN_001b7700, self);
	RB_I32(self, RO_CRANE_DONE) = 0;
	return 1;
}

/* 0x001b78d0  Rowing (gap; CRowing_objects.cpp inferred)
The crane's signal light: a sprite at its "clight" node, green while it moves, orange
while it waits; its point light follows in half the colour. */
void RowCrane_DrawLight(struct RowObject *self)
{
	char node[8];
	float pos[4];
	int colour[4];
	struct RowLight *light;
	void *n;

	memcpy(node, "clight", 7);
	bdSetWorldMatrix((float *)DAT_002855f0);
	n = baFindNode(RB_PTR(self->inst, 0x134), node);
	if (n == NULL) {
		bkPrintf("ERROR Could not find crane node [%s]\n", node);
		return;
	}
	baGetNodePosition(self->inst, n, pos);
	colour[2] = 0;
	colour[3] = 0xff;
	if (RB_I32(self, RO_CRANE_DONE) == 0) {
		colour[0] = 0x40;
		colour[1] = 0xff;
	} else {
		colour[0] = 0xff;
		colour[1] = 0x80;
	}
	RowSprites_Add(2, pos, 50.0f, 50.0f, 0.0f, g_RowCraneSprite, colour, -0.00499999989f);
	light = RB_PTR(self, RO_CRANE_LIGHT);
	if (light != NULL) {
		RowLights_SetColour(g_RowLights, light, colour[0] / 2, colour[1] / 2, colour[2] / 2);
		RowLights_SetPos(g_RowLights, light, pos);
	}
}

/* 0x001b7a00  Rowing (gap; CRowing_objects.cpp inferred)
Sinks the floating object (a collected power-up): it goes inactive far below. */
void RowFloaty_Sink(void *self)
{
	RB_U32(self, 0x18) = RB_U32(self, 0x18) & ~1u;
	RB_U8(self, RO_FLOAT_SUNK) = 1;
	RB_F32(self, 0x3c) = -8000.0f;                                  /* 0xc5fa0000 */
	rb_EnableCollider(RB_I32(self, RO_FLOAT_COLLIDER), 0);
}

/* 0x001b7a30  Rowing (gap; CRowing_objects.cpp inferred)
(The original runs the base destructor around the body's destructors, with a C++
exception frame.) */
void RowFloaty_dtor(void *self)
{
	RB_PTR(self, 0x00) = (void *)vtable_002762c4;
	RB_CALL(FUN_001a5780, void (*)(void *))(rb_body(self));
	RowObject_dtor(self);
	RB_CALL(FUN_001a5770, void (*)(void *))(rb_body(self));
	RowObject_dtor(self);
}

/* 0x001b7aa0  Rowing (gap; CRowing_objects.cpp inferred) vtable slot 5 */
void RowFloaty_SetHit(void *self)
{
	RB_I32(self, RO_FLOAT_HIT) = 1;
}

/* 0x001b7ab0  Rowing (gap; CRowing_objects.cpp inferred) scalar deleting destructor of the
buoys (0x001b7ad0 is a thunk of RowObject_dtor) */
void *RowBuoy_DeletingDtor(void *self, unsigned int flags)
{
	RowObject_dtor(self);
	if (flags & 1)
		RB_DELETE(self);
	return self;
}

/* 0x001b7ad0  Rowing (gap; CRowing_objects.cpp inferred) thunk of RowObject_dtor */
void RowObject_dtor_thunk(struct RowObject *self)
{
	RowObject_dtor(self);
}

/* 0x001b7ae0  Rowing (gap; CRowing_objects.cpp inferred)
Sets the buoy up at pos with rotation rot: its sprite, its collision sphere (radius
62.44) and a random bob phase. */
int RowBuoy_Init(struct RowObject *self, void *actor, float *pos, float *rot)
{
	if (rb_load_sun(&g_RowBuoySprite) == 0)
		return 0;
	RowObject_Init(self, actor);
	RowObject_SetPos(self, pos);
	RowObject_SetRot(self, rot);
	rb_AddCollider(4, RB_F32(RB_PTR(self->inst, 0x130), 0x5c), RB_FP(self->inst, 0xc0),
		NULL, 62.4410019f, FUN_001b7000, self);
	RB_F32(self, RO_BUOY_Y) = RB_F32(self->inst, 0xc4);
	RB_F32(self, RO_BUOY_VEL + 4) = 0.0f;
	RB_F32(self, RO_BUOY_VEL) = 0.0f;
	RB_F32(self, RO_BUOY_TILT + 4) = 0.0f;
	RB_F32(self, RO_BUOY_TILT) = 0.0f;
	RB_F32(self, RO_BUOY_BOB) = 0.0f;
	RB_F32(self, RO_BUOY_PHASE) = (float)rb_rand() * 0.000191747589f;   /* 0x00276678: 2 pi / 32767 */
	return 1;
}

/* 0x001b7c00  Rowing (gap; CRowing_objects.cpp inferred)
The buoy: the blink cycle (500 ms steps), the bob (5 up and down at 0.2 a step) with a
ripple, the tilt springing back after a push (a splash when it tips over 0.15), then the
animation. */
int RowBuoy_Update(struct RowObject *self, float dt)
{
	float axis[4];
	float q[4];
	float damp;
	float ox, oz;
	float nx, nz;
	float speed;
	unsigned int state;

	if ((self->flags & 1) == 0)
		return 0;
	RB_F32(self, RO_BUOY_BLINK) = RW_FRAME_MS() + RB_F32(self, RO_BUOY_BLINK);
	while (500.0f < RB_F32(self, RO_BUOY_BLINK)) {                  /* 0x002686fc */
		RB_F32(self, RO_BUOY_BLINK) = RB_F32(self, RO_BUOY_BLINK) - 500.0f;
		state = RB_U32(self, RO_BUOY_STATE);
		state = ((state & ~1u) + 2) ^ (state & 1);
		RB_U32(self, RO_BUOY_STATE) = state;
		if (6 < (state & ~1u))
			RB_U32(self, RO_BUOY_STATE) = (state & 1) ^ 1;
	}
	RB_F32(self, RO_BUOY_PHASE) = dt * 0.200000003f + RB_F32(self, RO_BUOY_PHASE);
	if (6.28318501f < RB_F32(self, RO_BUOY_PHASE)) {                /* 0x00262300 */
		float p = RB_F32(self, RO_BUOY_PHASE);

		do
			p = p - 6.28318501f;
		while (6.28318501f < p);
		RB_F32(self, RO_BUOY_PHASE) = p;
	}
	RB_F32(self, RO_BUOY_BOB) = sinf(RB_F32(self, RO_BUOY_PHASE)) * 5.0f;
	RB_F32(self->inst, 0xc4) = RB_F32(self, RO_BUOY_BOB) + RB_F32(self, RO_BUOY_Y);
	rb_water_ripple(RB_F32(self->inst, 0xc0), RB_F32(self->inst, 0xc8), RB_F32(self, RO_BUOY_BOB) - 2.0f,
		30.0f, 60.0f);
	damp = (float)pow(0.98000001907348633, (double)dt);             /* 0x002766a8 (double) */
	RB_F32(self, RO_BUOY_VEL) = (RB_F32(self, RO_BUOY_VEL) - RB_F32(self, RO_BUOY_TILT) * 0.0125000002f)
		* damp;                                                     /* 0x00268684 */
	RB_F32(self, RO_BUOY_VEL + 4) = (RB_F32(self, RO_BUOY_VEL + 4)
		- RB_F32(self, RO_BUOY_TILT + 4) * 0.0125000002f) * damp;
	ox = RB_F32(self, RO_BUOY_TILT);
	oz = RB_F32(self, RO_BUOY_TILT + 4);
	nx = RB_F32(self, RO_BUOY_VEL) * dt + RB_F32(self, RO_BUOY_TILT);
	RB_F32(self, RO_BUOY_TILT) = nx;
	nz = RB_F32(self, RO_BUOY_VEL + 4) * dt + RB_F32(self, RO_BUOY_TILT + 4);
	RB_F32(self, RO_BUOY_TILT + 4) = nz;
	if (ox * ox + oz * oz < 0.150000006f && !(nx * nx + nz * nz < 0.150000006f)) {     /* 0x0026573c */
		speed = bmFastSqrt(RB_F32(self, RO_BUOY_VEL) * RB_F32(self, RO_BUOY_VEL)
			+ RB_F32(self, RO_BUOY_VEL + 4) * RB_F32(self, RO_BUOY_VEL + 4)) * 1275.0f;  /* 0x002766a0 */
		if (!(speed <= 150.0f))
			speed = 150.0f;
		RowSound_Queue3D(0x11, RB_FP(self->inst, 0xc0), (int)speed);
	}
	axis[0] = RB_F32(self, RO_BUOY_TILT);
	axis[1] = 0.0f;
	axis[2] = RB_F32(self, RO_BUOY_TILT + 4);
	axis[3] = 1.0f;
	bmAxisAngleToQuaternion(q, axis);
	memcpy(RB_ADDR(self->inst, 0xe0), q, 16);
	return RowObject_Update(self, dt);
}

/* 0x001b7e60  Rowing (gap; CRowing_objects.cpp inferred) */
void RowBuoy_Draw(struct RowObject *self, unsigned int pass)
{
	if ((self->flags & 1) == 0)
		return;
	if ((pass & 4) != 0)
		RowBuoy_DrawLight(self);
	RowObject_Draw(self, pass);
}

/* 0x001b7e90  Rowing (gap; CRowing_objects.cpp inferred)
The race buoy: the bob (at 0.1 a step) and the sideways tilt springing back (a splash when
it tips over 0.15) on its placed rotation, the players' lights, spray from its side lights
every 4 frames in layout 3 (with a level), and ripples at both ends. */
int RowRaceBuoy_Update(struct RowObject *self, float dt)
{
	float axis[4];
	float q[4];
	float pos[4];
	float damp;
	float old;
	float speed;
	int result;
	int i;

	if ((self->flags & 1) == 0)
		return 0;
	RB_F32(self, RO_RACE_PHASE) = dt * 0.100000001f + RB_F32(self, RO_RACE_PHASE);
	if (6.28318501f < RB_F32(self, RO_RACE_PHASE)) {
		float p = RB_F32(self, RO_RACE_PHASE);

		do
			p = p - 6.28318501f;
		while (6.28318501f < p);
		RB_F32(self, RO_RACE_PHASE) = p;
	}
	RB_F32(self, RO_RACE_BOB) = sinf(RB_F32(self, RO_RACE_PHASE)) * 5.0f;
	RB_F32(self->inst, 0xc4) = RB_F32(self, RO_RACE_BOB) + RB_F32(self, RO_RACE_Y);
	damp = (float)pow(0.98000001907348633, (double)dt);
	RB_F32(self, RO_RACE_VEL) = (RB_F32(self, RO_RACE_VEL) - RB_F32(self, RO_RACE_TILT) * 0.0125000002f)
		* damp;
	old = RB_F32(self, RO_RACE_TILT);
	RB_F32(self, RO_RACE_TILT) = RB_F32(self, RO_RACE_VEL) * dt + RB_F32(self, RO_RACE_TILT);
	if (old * old < 0.150000006f
		&& !(RB_F32(self, RO_RACE_TILT) * RB_F32(self, RO_RACE_TILT) < 0.150000006f)) {
		speed = bmFastSqrt(RB_F32(self, RO_RACE_VEL) * RB_F32(self, RO_RACE_VEL)) * 3825.0f;    /* 0x002766b0 */
		if (!(speed <= 150.0f))
			speed = 150.0f;
		RowSound_Queue3D(0x11, RB_FP(self->inst, 0xc0), (int)speed);
	}
	axis[0] = RB_F32(self, RO_RACE_TILT);
	axis[1] = 0.0f;
	axis[2] = RB_F32(self, RO_RACE_TILT + 4);
	axis[3] = 1.0f;
	bmAxisAngleToQuaternion(q, axis);
	bmQuaternionMultiply(RB_FP(self->inst, 0xe0), q, RB_FP(self, RO_RACE_ROT));
	result = RowObject_Update(self, dt);
	RowRaceBuoy_UpdateLights(self);
	if (RW_VAR(int, 0x0030f0ec) != 0 && (g_RowStateFrames & 3) == 0) {
		for (i = 0; i < 2; i++) {
			void *node = RB_PTR(self, RO_RACE_NODES + 0x10 + i * 4);

			if (node == NULL)
				continue;
			baGetNodePosition(self->inst, node, pos);
			RowParticles_AddBlob(g_RowParticles, pos, 30.0f);
			RowParticles_AddSmoke(g_RowParticles, pos, 20.0f);
		}
	}
	rb_water_ripple(RB_F32(self, RO_RACE_END1), RB_F32(self, RO_RACE_END1 + 8), RB_F32(self, RO_RACE_BOB) - 2.0f,
		30.0f, 60.0f);
	rb_water_ripple(RB_F32(self, RO_RACE_END2), RB_F32(self, RO_RACE_END2 + 8), RB_F32(self, RO_RACE_BOB) - 2.0f,
		30.0f, 60.0f);
	return result;
}

/* 0x001b80d0  Rowing (gap; CRowing_objects.cpp inferred) */
void RowRaceBuoy_Draw(struct RowObject *self, unsigned int pass)
{
	if ((self->flags & 1) == 0)
		return;
	if ((pass & 4) != 0)
		RowRaceBuoy_DrawLights(self);
	RowObject_Draw(self, pass);
}

/* 0x001b8100  Rowing (gap; CRowing_objects.cpp inferred)
Finds the level's gate nearest to the buoy and moves its two points to the buoy's ends
(keeping the gate's direction). */
void RowRaceBuoy_FindGate(struct RowObject *self)
{
	unsigned char *world = g_RowWorld;
	struct RowPath *gates;
	float best = 99999.0f;
	float dx, dz, d;
	float *p;
	int i;

	RB_I32(self, RO_RACE_GATE) = -1;
	for (i = 0; i < RB_I32(world, 0x19c); i++) {
		gates = RB_PTR(world, 0x1a0);
		p = gates[i].points;
		dx = (p[2] + p[0]) * 0.5f - RB_F32(self->inst, 0xc0);
		dz = (p[3] + p[1]) * 0.5f - RB_F32(self->inst, 0xc8);
		d = sqrtf(dx * dx + dz * dz);
		if (d < best) {
			RB_I32(self, RO_RACE_GATE) = i;
			world = g_RowWorld;
			best = d;
		}
	}
	gates = RB_PTR(world, 0x1a0);
	p = gates[RB_I32(self, RO_RACE_GATE)].points;
	dx = p[0] - RB_F32(self, RO_RACE_END1);
	dz = p[1] - RB_F32(self, RO_RACE_END1 + 8);
	if (6400.0f <= dx * dx + dz * dz) {                             /* 0x00268730 */
		p[0] = RB_F32(self, RO_RACE_END2);
		p = ((struct RowPath *)RB_PTR(g_RowWorld, 0x1a0))[RB_I32(self, RO_RACE_GATE)].points;
		p[1] = RB_F32(self, RO_RACE_END2 + 8);
		p[2] = RB_F32(self, RO_RACE_END1);
		p[3] = RB_F32(self, RO_RACE_END1 + 8);
		return;
	}
	p[0] = RB_F32(self, RO_RACE_END1);
	p = ((struct RowPath *)RB_PTR(g_RowWorld, 0x1a0))[RB_I32(self, RO_RACE_GATE)].points;
	p[1] = RB_F32(self, RO_RACE_END1 + 8);
	p[2] = RB_F32(self, RO_RACE_END2);
	p[3] = RB_F32(self, RO_RACE_END2 + 8);
}

/* 0x001b82c0  Rowing (gap; CRowing_objects.cpp inferred) the crane's scalar deleting
destructor */
void *RowCrane_DeletingDtor(void *self, unsigned int flags)
{
	RowCrane_dtor(self);
	if (flags & 1)
		RB_DELETE(self);
	return self;
}

/* 0x001b82e0  Rowing (gap; CRowing_objects.cpp inferred)
Sets a crane (kind 1) or an overhead crane (kind 2) up: a random wait of 5-15 s, its sprite,
the first animation and, for a crane, a point light (group 2) for its signal. Returns 0
for another kind or a missing sprite. */
int RowCrane_Init(struct RowObject *self, void *actor, int kind)
{
	float origin[4];
	struct RowLight *light;

	RB_F32(self, RO_CRANE_TIMER) = 0.0f;
	RB_I32(self, RO_CRANE_KIND) = kind;
	RB_I32(self, RO_CRANE_SEGMENT) = 0;
	RB_F32(self, RO_CRANE_WAIT) = rb_randf() * 10000.0f + 5000.0f;      /* 0x002657a0, 0x0026c590 */
	if (kind != 1 && kind != 2)
		return 0;
	if (rb_load_sun(&g_RowCraneSprite) == 0)
		return 0;
	RowObject_Init(self, actor);
	RowCrane_NextAnim(self);
	if (RB_I32(self, RO_CRANE_KIND) != 1) {
		RB_PTR(self, RO_CRANE_LIGHT) = NULL;
		return 1;
	}
	origin[0] = 0.0f;
	origin[1] = 0.0f;
	origin[2] = 0.0f;
	origin[3] = 1.0f;
	light = RowLights_AddPoint(g_RowLights, 2, origin, 0x7f, 0x7f, 0x7f, 4.00000005e-07f, 1);
	RB_PTR(self, RO_CRANE_LIGHT) = light;
	RowLights_Enable(g_RowLights, light, 1);
	return 1;
}

/* 0x001b8440  Rowing (gap; CRowing_objects.cpp inferred)
The crane waits after each animation segment, then plays the next one. */
int RowCrane_Update(struct RowObject *self, float dt)
{
	if ((self->flags & 1) == 0)
		return 0;
	if (RB_I32(self, RO_CRANE_DONE) != 0) {
		RB_F32(self, RO_CRANE_TIMER) = RW_FRAME_MS() + RB_F32(self, RO_CRANE_TIMER);
		if (RB_F32(self, RO_CRANE_WAIT) <= RB_F32(self, RO_CRANE_TIMER)) {
			RB_F32(self, RO_CRANE_TIMER) = 0.0f;
			RB_F32(self, RO_CRANE_WAIT) = rb_randf() * 10000.0f + 5000.0f;
			if (RowCrane_NextAnim(self) == 0)
				return 0;
		}
	}
	baUpdateAnimations(self->inst, RB_PTR(self->inst, 0x134), RW_FRAME_MS(), NULL);
	return RowObject_Update(self, dt);
}

/* 0x001b8500  Rowing (gap; CRowing_objects.cpp inferred) */
void RowCrane_Draw(struct RowObject *self, unsigned int pass)
{
	if ((self->flags & 1) == 0)
		return;
	if ((pass & 4) != 0 && RB_I32(self, RO_CRANE_KIND) == 1)
		RowCrane_DrawLight(self);
	RowObject_Draw(self, pass);
}

/* ---------- RowFloaty (types 3-13) */

/* 0x001b8540  Rowing (gap; CRowing_objects.cpp inferred)
A boat hits the floating object (not after the race): it gets the object's velocity and
pushes it away (when it moves towards the boat); a power-up sinks. Returns whether it
hit. */
int RowFloaty_OnHit(void *self, void *boat, void *a, void *b, float *out)
{
	float d[3];
	float n[3];
	float dot;
	int hit;

	if (g_RowState == 6 || (RB_U32(self, 0x18) & 1) == 0)
		return 0;
	hit = rb_BoatHitsSphere(a, RB_FP(RB_PTR(self, 0x0c), 0xc0), 62.4410019f, b);
	if (hit == 0)
		return hit;
	memcpy(out, RB_ADDR(self, RO_FLOAT_VEL), 16);
	d[0] = RB_F32(boat, 0x2b4) - RB_F32(self, RO_FLOAT_POS);
	d[1] = RB_F32(boat, 0x2b8) - RB_F32(self, RO_FLOAT_POS + 4);
	d[2] = RB_F32(boat, 0x2bc) - RB_F32(self, RO_FLOAT_POS + 8);
	bmVectorSetLength(n, d, 1.0f);
	dot = n[2] * RB_F32(self, RO_FLOAT_VEL + 8) + n[1] * RB_F32(self, RO_FLOAT_VEL + 4)
		+ n[0] * RB_F32(self, RO_FLOAT_VEL);
	if (0.0f <= dot) {
		dot = dot + 0.5f;
		RB_F32(self, RO_FLOAT_VEL) = RB_F32(self, RO_FLOAT_VEL) - n[0] * dot;
		RB_F32(self, RO_FLOAT_VEL + 4) = RB_F32(self, RO_FLOAT_VEL + 4) - n[1] * dot;
		RB_F32(self, RO_FLOAT_VEL + 8) = RB_F32(self, RO_FLOAT_VEL + 8) - n[2] * dot;
	}
	if (RB_I32(self, 0x10) == 5 || RB_I32(self, 0x10) == 7)
		RowFloaty_Sink(self);
	return hit;
}

/* 0x001b8680  Rowing (missing from the function table; CRowing_objects.cpp inferred)
Collision callback of a floating object. */
int FUN_001b8680(void *self, void *boat, void *a, void *b, float *out)
{
	return RowFloaty_OnHit(self, boat, a, b, out);
}

/* 0x001b86a0  Rowing (gap; CRowing_objects.cpp inferred)
Sets a floating object up at pos with rotation rot: its physics body, its collision sphere
(off until it spawns), the token glow and inactive. */
int RowFloaty_Init(void *self, void *actor, int type, float *pos, float *rot)
{
	float axisAngle[4];

	(void)type;
	RowObject_Init(self, actor);
	bmQuaternionToAxisAngle(axisAngle, rot);
	RB_CALL(FUN_001a59f0, int (*)(void *, void *, float *, float *, int))
		(rb_body(self), RB_PTR(self, 0x0c), pos, axisAngle, 1);
	RB_F32(self, 0x78) = 0.600000024f;                              /* 0x3f19999a */
	RB_I32(self, RO_FLOAT_COLLIDER) = -1;
	RB_I32(self, RO_FLOAT_COLLIDER) = rb_AddCollider(6,
		RB_F32(RB_PTR(RB_PTR(self, 0x0c), 0x130), 0x5c), RB_FP(self, RO_FLOAT_POS),
		RB_FP(self, RO_FLOAT_VEL), 62.4410019f, FUN_001b8680, self);
	RB_PTR(self, RO_FLOAT_GLOW) = bLoadResource(NULL, "tokenglow.bmp", 0, 0xdefa);
	if (RB_I32(self, RO_FLOAT_COLLIDER) != -1)
		rb_EnableCollider(RB_I32(self, RO_FLOAT_COLLIDER), 0);
	RB_U32(self, 0x18) = RB_U32(self, 0x18) & ~1u;
	return 1;
}

/* 0x001b8760  Rowing (gap; CRowing_objects.cpp inferred)
Spawns an inactive floating object upstream: across the river at random, or (target set)
within 100 of a random player that may be targeted; at height 300, 1800 down the river when
`atStart` is 0, drifting at -speed/2. */
void RowFloaty_Spawn(void *self, float atStart, float speed, int target)
{
	float *pos = RB_FP(self, RO_FLOAT_POS);
	unsigned char *boat;
	float width;
	float lo, hi;
	int slot;
	int player;

	if ((RB_U32(self, 0x18) & 1) != 0)
		return;
	bkPrintf("Floaty Spawn!\n");
	width = (g_RowRiverWidth - 124.882004f) - 20.0f;                /* 0x002766b4, 0x00255530 */
	lo = g_RowRiverCentre - width * 0.5f;
	hi = width * 0.5f + g_RowRiverCentre;
	slot = RW_VAR(int, 0x002b5b20 + (rb_rand() % 16) * 4);          /* the player weights */
	player = DAT_002b9c58[slot];
	boat = g_RowPlayers[player];
	if (target != 0 && RB_U8(boat, 0x37c) != 0) {
		pos[0] = Core_RandUnit() * 200.0f - 100.0f;
		pos[0] = pos[0] + RB_F32(g_RowPlayers[player], 0x2b4);
	} else {
		pos[0] = (float)rb_rand() * width * RB_RAND01 + lo;
	}
	if (pos[0] < lo)
		pos[0] = lo;
	if (!(pos[0] <= hi))
		pos[0] = hi;
	pos[1] = 300.0f;
	if (atStart == 0.0f)
		pos[2] = 1800.0f;
	RB_F32(self, RO_FLOAT_VEL) = 0.0f;
	RB_F32(self, RO_FLOAT_VEL + 4) = 0.0f;
	RB_F32(self, RO_FLOAT_VEL + 8) = speed * -0.5f;
	RowObject_SetPos(self, pos);
	rb_ResetCollider(RB_I32(self, RO_FLOAT_COLLIDER));
	RB_U8(self, RO_FLOAT_SUNK) = 0;
	RB_U32(self, 0x18) = RB_U32(self, 0x18) | 1;
	RB_F32(self, RO_FLOAT_SPIN) = rb_randf() * 0.00999999978f - 0.00499999989f;
	rb_EnableCollider(RB_I32(self, RO_FLOAT_COLLIDER), 1);
}

/* 0x001b8950  Rowing (gap; CRowing_objects.cpp inferred)
Moves a floating object (not after the race): the cow moos now and then, the physics body
moves it, it bounces off the banks and the end of the river, a hit or a bounce changes its
spin, it makes a ripple; a sunk one goes inactive. */
int RowFloaty_Update(void *self, float dt)
{
	float half;
	float lo, hi;
	int bounced = 0;

	if ((RB_U32(self, 0x18) & 1) == 0 || g_RowState == 6)
		return 0;
	if (RB_I32(self, 0x10) == 3 && (rb_rand() & 0xff) < 2)
		RowSound_Queue3D(0x1c, RB_FP(self, RO_FLOAT_POS), 0xff);     /* the cow */
	RB_CALL(FUN_001a5f00, void (*)(void *, float))(rb_body(self), dt);
	RB_CALL(FUN_001a5ad0, void (*)(void *))(rb_body(self));
	half = g_RowRiverWidth * 0.5f;
	lo = g_RowRiverCentre - half;
	hi = half + g_RowRiverCentre;
	if (RB_F32(self, RO_FLOAT_MOVE) + RB_F32(self, RO_FLOAT_POS) - 62.4410019f < lo) {
		RB_F32(self, RO_FLOAT_MOVE) = (lo + 62.4410019f) - RB_F32(self, RO_FLOAT_POS);
		if (RB_F32(self, RO_FLOAT_VEL) < 0.0f) {
			bounced = 1;
			RB_F32(self, RO_FLOAT_VEL) = -RB_F32(self, RO_FLOAT_VEL);
		}
	}
	if (hi < RB_F32(self, RO_FLOAT_MOVE) + RB_F32(self, RO_FLOAT_POS) + 62.4410019f) {
		RB_F32(self, RO_FLOAT_MOVE) = (hi - 62.4410019f) - RB_F32(self, RO_FLOAT_POS);
		if (0.0f < RB_F32(self, RO_FLOAT_VEL)) {
			bounced = 1;
			RB_F32(self, RO_FLOAT_VEL) = -RB_F32(self, RO_FLOAT_VEL);
		}
	}
	if (RB_F32(self, RO_FLOAT_POS + 8) + 62.4410019f <= 0.0f
		&& 0.0f < RB_F32(self, RO_FLOAT_POS + 8) + RB_F32(self, RO_FLOAT_MOVE + 8) + 62.4410019f) {
		RB_F32(self, RO_FLOAT_MOVE + 8) = -62.4410019f - RB_F32(self, RO_FLOAT_POS + 8);
		if (0.0f < RB_F32(self, RO_FLOAT_VEL + 8)) {
			bounced = 1;
			RB_F32(self, RO_FLOAT_VEL + 8) = -RB_F32(self, RO_FLOAT_VEL + 8);
		}
	}
	if (RB_CALL(FUN_001a6f50, int (*)(void *, float, int))(rb_body(self), 62.4410019f,
		RB_I32(self, RO_FLOAT_COLLIDER)) != 0 || bounced) {
		RB_F32(self, RO_FLOAT_SPIN) = (rb_randf() * 0.00999999978f - 0.00499999989f)
			+ RB_F32(self, RO_FLOAT_SPIN);
		if (0.0799999982f < RB_F32(self, RO_FLOAT_SPIN))            /* 0x002766cc */
			RB_F32(self, RO_FLOAT_SPIN) = 0.0799999982f;
		if (RB_F32(self, RO_FLOAT_SPIN) < -0.0799999982f)           /* 0x002766c8 */
			RB_F32(self, RO_FLOAT_SPIN) = -0.0799999982f;
	}
	RB_F32(self, RO_FLOAT_POS) = RB_F32(self, RO_FLOAT_POS) + RB_F32(self, RO_FLOAT_MOVE);
	RB_F32(self, RO_FLOAT_POS + 4) = RB_F32(self, RO_FLOAT_POS + 4) + RB_F32(self, RO_FLOAT_MOVE + 4);
	RB_F32(self, RO_FLOAT_POS + 8) = RB_F32(self, RO_FLOAT_POS + 8) + RB_F32(self, RO_FLOAT_MOVE + 8);
	rb_water_ripple(RB_F32(self, RO_FLOAT_POS), RB_F32(self, RO_FLOAT_POS + 8), -5.0f, 32.4410019f,
		62.4410019f);                                               /* 0x4201c396, 0x4279c396 */
	if (RB_U8(self, RO_FLOAT_SUNK) != 0)
		RowFloaty_Sink(self);
	return RowObject_Update(self, dt);
}

/* 0x001b8c10  Rowing (gap; CRowing_objects.cpp inferred)
Draws a floating object (not after the race); a power-up (types 5-7) is drawn with an
extra additive texture stage in pass 1 and a token glow sprite above it in pass 4. */
void RowFloaty_Draw(void *self, unsigned int pass)
{
	float pos[4];

	if ((RB_U32(self, 0x18) & 1) == 0 || g_RowState == 6)
		return;
	if (RB_I32(self, 0x10) < 5 || 7 < RB_I32(self, 0x10)) {
		RowObject_Draw(self, pass);
		return;
	}
	if ((pass & 1) != 0) {
		D3DDevice_SetRenderState_TextureFactor(0xbe000000);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0x12);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 0xe);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xa, 1);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0xb, 1);
		D3DDevice_SetTextureState_TexCoordIndex(1, 0x30000);
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)9, 2);
	}
	RowObject_Draw(self, pass);
	if ((pass & 1) != 0) {
		D3DDevice_SetTextureState_Deferred(1, (D3DTEXTURESTAGESTATETYPE)0, 1);
		D3DDevice_SetTextureState_TexCoordIndex(1, 1);
		bdSetTexture(1, NULL);
	}
	if ((pass & 4) == 0)
		return;
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(10, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetDepthOffset(-1.5f);
	pos[0] = RB_F32(self, RO_FLOAT_POS);
	pos[1] = RB_F32(self, RO_FLOAT_POS + 4) + 140.0f;               /* 0x0026e334 */
	pos[2] = RB_F32(self, RO_FLOAT_POS + 8);
	pos[3] = RB_F32(self, RO_FLOAT_POS + 0xc);
	bdDrawSprite3D(pos, 140.0f, 140.0f, 0.0f, RB_PTR(self, RO_FLOAT_GLOW), 0, 0, 0x7f, 0x7f, 0x7f, 0x38,
		NULL, 0.0f);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(7, 3, 0);
	bdPopRenderState(5);
	bdSetDepthOffset(0.0f);
}

/* 0x001b8df0  Rowing (gap; CRowing_objects.cpp inferred)
Adds to *out the sideways pull (x) that a floating object at `other` puts on a boat (or, type
-1, the AI's look-ahead) at pos: 300 / distance (1000 when closer than 0.001), falling to 0
from 200 to 350 across, nothing for objects behind (dz > 0); for the look-ahead nothing when
dz > -20 or the object is more than 120 across, doubled when dz < -30; other objects pull a
third (mines fully), toward the object's side of the river; power-up shields (6) pull
straight toward it. */
void RowFloaty_AddPull(float *out, float *pos, float *other, int type)
{
	float dx = pos[0] - other[0];
	float dy = pos[1] - other[1];
	float dz = pos[2] - other[2];
	float half = g_RowRiverWidth * 0.5f;
	float lo = (g_RowRiverCentre - half) + 200.0f;
	float hi = (half + g_RowRiverCentre) - 200.0f;
	float dist;
	float f;

	dist = sqrtf(dy * dy + dz * dz + dx * dx);
	if (0.00100000005f <= dist)                                     /* 0x0025767c */
		f = 300.0f / dist;
	else
		f = 1000.0f;
	if (!(dx <= 350.0f) || dx < -350.0f) {
		f = 0.0f;
	} else {
		if (dx < -200.0f)
			f = (dx + 350.0f) * f * 0.00666666683f;                 /* 0x002766d4 */
		if (200.0f < dx)
			f = (350.0f - dx) * f * 0.00666666683f;
	}
	if (0.0f < dz)
		f = 0.0f;
	if (type == -1) {
		if (-20.0f < dz)
			f = 0.0f;
		if (dx < -120.0f || 120.0f < dx)
			f = 0.0f;
		if (dz < -30.0f)
			f = f + f;
	} else {
		if (type != 9)
			f = f * 0.333333343f;
		if (type == 6) {
			*out = *out - dx * f;
			return;
		}
	}
	if ((dx < 0.0f || hi < other[0]) && lo < other[0]) {
		*out = *out - f * 400.0f;
		return;
	}
	*out = f * 400.0f + *out;
}

/* 0x001b8fe0  Rowing (gap; CRowing_objects.cpp inferred)
The look-ahead pull of every active floating object at pos. */
float RowFloaty_SumPull(float *pos)
{
	unsigned char *obj;
	float pull = 0.0f;
	int i;

	for (i = 0; i < 100; i++) {
		obj = g_RowFloaties[i];
		if ((RB_U32(obj, 0x18) & 1) != 0 && RB_U8(obj, RO_FLOAT_SUNK) == 0)
			RowFloaty_AddPull(&pull, pos, RB_FP(obj, RO_FLOAT_POS), RB_I32(obj, 0x10));
	}
	return pull;
}

/* 0x001b9030  Rowing (gap; CRowing_objects.cpp inferred)
A boat at pos touches a mine (9) or a shield power-up (6) within 134.9: a mine explodes
(three explosions and a sound); either sinks. Returns its type, or -1. */
int RowFloaty_CheckTouch(float *pos)
{
	unsigned char *obj;
	float dx, dz;
	float *at;
	int i;

	for (i = 0; i < 100; i++) {
		obj = g_RowFloaties[i];
		if ((RB_U32(obj, 0x18) & 1) == 0 || RB_U8(obj, RO_FLOAT_SUNK) != 0)
			continue;
		if (RB_I32(obj, 0x10) != 9 && RB_I32(obj, 0x10) != 6)
			continue;
		dx = RB_F32(obj, RO_FLOAT_POS) - pos[0];
		dz = RB_F32(obj, RO_FLOAT_POS + 8) - pos[2];
		if (!(dx * dx + dz * dz < 18193.1543f))                    /* 0x00275a34 */
			continue;
		if (RB_I32(obj, 0x10) == 9) {
			at = RB_FP(obj, RO_FLOAT_POS);
			CParticleSystem_SetPosition(RW_VAR(struct CPSHeader *, 0x0030f164), at);
			CParticleSystem_SetPosition(RW_VAR(struct CPSHeader *, 0x0030f168), at);
			CParticleSystem_SetPosition(RW_VAR(struct CPSHeader *, 0x0030f16c), at);
			DcParticles_EmitAll(RW_VAR(struct CPSHeader *, 0x0030f164));
			DcParticles_EmitAll(RW_VAR(struct CPSHeader *, 0x0030f168));
			DcParticles_EmitAll(RW_VAR(struct CPSHeader *, 0x0030f16c));
			RB_U8(RW_VAR(void *, 0x0030f164), 0x44) = 1;
			RB_U8(RW_VAR(void *, 0x0030f168), 0x3c) = 1;
			RB_U8(RW_VAR(void *, 0x0030f16c), 0x3c) = 1;
			RowSound_Queue3D(0x1d, at, 0xff);
		}
		RB_U8(obj, RO_FLOAT_SUNK) = 1;
		RB_F32(obj, RO_FLOAT_POS + 8) = -8000.0f;
		RB_U32(obj, 0x18) = RB_U32(obj, 0x18) & ~1u;
		rb_EnableCollider(RB_I32(obj, RO_FLOAT_COLLIDER), 0);
		return RB_I32(obj, 0x10);
	}
	return -1;
}

/* 0x001b9140  Rowing (gap; CRowing_objects.cpp inferred)
Sets a race buoy up at pos with rotation rot: its two floating ends 220 to each side (with
collision spheres of radius 93.66), its sprite, its gate, a random bob phase and its light
nodes. Returns 0 when the sprite is missing. */
int RowRaceBuoy_Init(struct RowObject *self, void *actor, float *pos, float *rot)
{
	float m[16];
	float v[4];
	void *root;

	bmQuaternionToMatrix(m, rot);
	bmTransformVector4(v, m, rb_race_end1);
	RB_F32(self, RO_RACE_END1) = v[0] + pos[0];
	RB_F32(self, RO_RACE_END1 + 4) = v[1] + pos[1];
	RB_F32(self, RO_RACE_END1 + 8) = v[2] + pos[2];
	bmTransformVector4(v, m, rb_race_end2);
	RB_F32(self, RO_RACE_END2) = v[0] + pos[0];
	RB_F32(self, RO_RACE_END2 + 4) = v[1] + pos[1];
	RB_F32(self, RO_RACE_END2 + 8) = v[2] + pos[2];
	if (rb_load_sun(&g_RowRaceBuoySprite) == 0)
		return 0;
	RowObject_Init(self, actor);
	RowObject_SetPos(self, pos);
	RowObject_SetRot(self, rot);
	RB_F32(self, RO_RACE_Y) = RB_F32(self->inst, 0xc4);
	RowRaceBuoy_FindGate(self);
	rb_AddCollider(5, 93.6614990f, RB_FP(self, RO_RACE_END1), NULL, 62.4410019f, FUN_001b71f0, self);
	rb_AddCollider(5, 93.6614990f, RB_FP(self, RO_RACE_END2), NULL, 62.4410019f, FUN_001b72b0, self);
	memcpy(RB_ADDR(self, RO_RACE_ROT), rot, 16);
	RB_F32(self, RO_RACE_VEL + 4) = 0.0f;
	RB_F32(self, RO_RACE_VEL) = 0.0f;
	RB_F32(self, RO_RACE_TILT + 4) = 0.0f;
	RB_F32(self, RO_RACE_TILT) = 0.0f;
	RB_F32(self, RO_RACE_BOB) = 0.0f;
	RB_F32(self, RO_RACE_PHASE) = (float)rb_rand() * 0.000191747589f;
	root = RB_PTR(self->inst, 0x134);
	RB_PTR(self, RO_RACE_NODES + 0x00) = baFindNode(root, "olight");
	RB_PTR(self, RO_RACE_NODES + 0x04) = baFindNode(RB_PTR(self->inst, 0x134), "glight");
	RB_PTR(self, RO_RACE_NODES + 0x08) = baFindNode(RB_PTR(self->inst, 0x134), "blight");
	RB_PTR(self, RO_RACE_NODES + 0x0c) = baFindNode(RB_PTR(self->inst, 0x134), "plight");
	RB_PTR(self, RO_RACE_NODES + 0x10) = baFindNode(RB_PTR(self->inst, 0x134), "slightl");
	RB_PTR(self, RO_RACE_NODES + 0x14) = baFindNode(RB_PTR(self->inst, 0x134), "slightr");
	return 1;
}
