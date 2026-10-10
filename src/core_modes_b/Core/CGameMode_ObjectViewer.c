/*
CGAMEMODE_OBJECTVIEWER.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_ObjectViewer.cpp

A debug viewer: steps through every actor resource that is loaded, rotates it, plays its
animations and orbits the camera. Only 0x00025b40 is tagged with this file; the rest of
the class (vtable 0x00255dc4) is unplaced in the table.
*/

#include "core_modes_b.h"

static const char cmb_file_viewer[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_ObjectViewer.cpp";

/* bactor FUN_000380f0: advance an actor's animation (actor, node, frames, flags) */
static void cmb_actor_animate(void *actor, void *node, float frames, int flags)
{
	CMB_DUAL(FUN_000380f0, undefined4 (*)(int, undefined4, undefined4, undefined4),
		int (*)(void *, void *, float, int),
		((int)actor, (undefined4)node, cmb_fbits(frames), flags), (actor, node, frames, flags));
}

/* bkernel FUN_0004b820: the next resource of a type (0: the first) */
#define cmb_next_resource(type, resource) (((void *(*)(unsigned int, void *))FUN_0004b820)((type), (resource)))

/* 0x00255dc4  CGameMode_ObjectViewer vtable */
const void *const vtable_00255dc4[13] = {
	(const void *)FUN_00025bd0, /* [0] scalar deleting destructor */
	(const void *)FUN_001d5950, /* [1] empty */
	(const void *)FUN_00025a90, /* [2] Start */
	(const void *)FUN_00025a50, /* [3] Exit */
	(const void *)FUN_00025e70, /* [4] Enter */
	(const void *)FUN_00025bf0, /* [5] input */
	(const void *)FUN_00025aa0, /* [6] Update */
	(const void *)FUN_00025ae0, /* [7] Render */
	(const void *)FUN_001ec260, /* [8] empty */
	(const void *)FUN_00012930, /* [9] core_modes_a */
	(const void *)FUN_000128e0, /* [10] core_modes_a */
	(const void *)FUN_001d5950, /* [11] empty */
	(const void *)FUN_001d5950, /* [12] empty */
};

/* 0x00025a20  (unplaced): constructor */
struct CGameMode_ObjectViewer *CGameMode_ObjectViewer_ctor(struct CGameMode_ObjectViewer *self)
{
	((void *(*)(void *, const char *))FUN_00012800)(self, "ObjectViewer");
	self->vtable = vtable_00255dc4;
	return self;
}

/* 0x00025a40  (unplaced): destructor */
void CGameMode_ObjectViewer_dtor(struct CGameMode_ObjectViewer *self)
{
	self->vtable = vtable_00255dc4;
	((void (*)(void *))FUN_00012860)(self);
}

/* 0x00025a50  (unplaced): Exit */
void CGameMode_ObjectViewer_Exit(struct CGameMode_ObjectViewer *self)
{
	((void (*)(void *))FUN_00034930)(self->actor);
	((void (*)(void *))FUN_00048970)(self->input);
	((void *(*)(void *))FUN_0003bf10)(self->camera);
	((void (*)(void *))FUN_000384d0)(DAT_002ba4e8);
	((void (*)(void *))FUN_000384d0)(DAT_002ba4ec);
}

/* 0x00025a90  (unplaced): Start */
void CGameMode_ObjectViewer_Start(struct CGameMode_ObjectViewer *self)
{
	self->flags = 1;
}

/* 0x00025aa0  (unplaced): Update */
void CGameMode_ObjectViewer_Update(struct CGameMode_ObjectViewer *self, int active)
{
	cmb_actor_animate(self->actor, CMB_PTR(self->actor, 0x134), DAT_0028fda8, 0);
	((void (*)(float *, float, float, float))FUN_000436e0)(self->target, 0.0f, 0.0f, self->distance);
	((void (*)(void))FUN_00043790)();
}

/* 0x00025ae0  (unplaced): Render */
void CGameMode_ObjectViewer_Render(struct CGameMode_ObjectViewer *self, int active)
{
	((void (*)(int, int, int))FUN_00044b00)(3, DAT_0028398c, 0);
	((void (*)(int))FUN_00043b70)(1);
	((void (*)(void *, int))FUN_00034bf0)(self->actor, 1);
	((void (*)(void))FUN_000438b0)();
	((void (*)(int, int, int))FUN_00044b00)(4, 1, 0);
	((void (*)(int, int, int))FUN_00044b00)(6, 0, 0);
	((int (*)(void *, int))FUN_00034d50)(self->actor, 7);
	((void (*)(int, int, int))FUN_00044b00)(4, 0, 0);
}

/* 0x00025b40  Core/CGameMode_ObjectViewer.cpp: make the instance of an actor resource */
void *CGameMode_ObjectViewer_MakeActor(struct CGameMode_ObjectViewer *self, void *resource)
{
	void *actor;

	actor = ((void *(*)(void *, void *, int, const char *, int))FUN_00036b20)(NULL, resource, 2,
		cmb_file_viewer, 0x11d);
	((void (*)(void *, int, int, int))FUN_00035cc0)(actor, 0, 1, 0);
	((void (*)(void *, int, int, int))FUN_00035cc0)(actor, 1, 0, 0);
	((void (*)(void *, void *, int, int))FUN_00034e70)(actor, CMB_PTR(actor, 0x134), 0, 1);
	((void (*)(void *, void *, int, int))FUN_00038190)(actor, CMB_PTR(actor, 0x134), 1, 0);
	CMB_F32(actor, 0xc0) = 0.0f;
	CMB_F32(actor, 0xc4) = 0.0f;
	CMB_F32(actor, 0xc8) = 0.0f;
	CMB_F32(actor, 0xcc) = 1.0f;
	((void (*)(void *, int))FUN_00034bf0)(actor, 1);
	self->anim = 0;
	return actor;
}

/* 0x00025bd0  (unplaced): scalar deleting destructor */
void *CGameMode_ObjectViewer_DeletingDtor(struct CGameMode_ObjectViewer *self, unsigned char flags)
{
	CGameMode_ObjectViewer_dtor(self);
	if ((flags & 1) != 0)
		CMB_DELETE(self);
	return self;
}

/* play animation `self->anim` of the actor */
static void cmb_viewer_play(struct CGameMode_ObjectViewer *self)
{
	void *actor = self->actor;

	((void (*)(void *, void *, int, int))FUN_00038190)(actor, CMB_PTR(actor, 0x134), 1, 0);
	actor = self->actor;
	((void (*)(void *, void *, int, int, int, int, int, int))FUN_00035770)(actor,
		CMB_PTR(actor, 0x134), CMB_I32(actor, 0x13c) + (int)self->anim * 0x20, 0x400, -1, 1, 0, 0);
}

/* 0x00025bf0  (unplaced): the controls */
void CGameMode_ObjectViewer_Input(struct CGameMode_ObjectViewer *self, int active)
{
	float rotation[4];
	unsigned int count;
	void *start;
	void *previous;
	int value;

	if (CMB_I32(self->next_actor, 0x14) != 0) {
		self->resource = cmb_next_resource(1, self->resource);
		if (self->resource == NULL)
			self->resource = cmb_next_resource(1, NULL);
		((void (*)(void *))FUN_00034930)(self->actor);
		self->actor = CGameMode_ObjectViewer_MakeActor(self, self->resource);
	}
	if (CMB_I32(self->prev_actor, 0x14) != 0) {
		/* walk the whole list to find the one before (the original leaves `previous` unset
		   when the list never yields a resource; it starts at NULL here) */
		start = self->resource;
		previous = NULL;
		do {
			if (self->resource != NULL)
				previous = self->resource;
			self->resource = cmb_next_resource(1, self->resource);
		} while (self->resource != start);
		self->resource = previous;
		if (previous == NULL)
			self->resource = cmb_next_resource(1, NULL);
		((void (*)(void *))FUN_00034930)(self->actor);
		self->actor = CGameMode_ObjectViewer_MakeActor(self, self->resource);
	}
	value = CMB_I32(self->rot_x, 0x14);
	if (value != 0) {
		((void (*)(float *, float))FUN_00051e30)(rotation, (float)value * _DAT_00283990);
		((void (*)(float *, float *, float *))FUN_00051ca0)((float *)((char *)self->actor + 0xe0),
			(float *)((char *)self->actor + 0xe0), rotation);
	}
	value = CMB_I32(self->rot_y, 0x14);
	if (value != 0) {
		((void (*)(float *, float))FUN_00051e60)(rotation, (float)value * _DAT_00283994);
		((void (*)(float *, float *, float *))FUN_00051ca0)((float *)((char *)self->actor + 0xe0),
			(float *)((char *)self->actor + 0xe0), rotation);
	}
	value = CMB_I32(self->rot_z, 0x14);
	if (value != 0) {
		((void (*)(float *, float))FUN_00051e90)(rotation, (float)value * _DAT_00283998);
		((void (*)(float *, float *, float *))FUN_00051ca0)((float *)((char *)self->actor + 0xe0),
			(float *)((char *)self->actor + 0xe0), rotation);
	}
	value = CMB_I32(self->zoom, 0x14);
	if (value != 0)
		self->distance = (float)value * _DAT_0028399c + self->distance;
	if (CMB_I32(self->next_anim, 0x14) != 0) {
		count = CMB_U32(self->actor, 0x14c);
		if (count != 0) {
			self->anim = (self->anim + 1) % count;
			cmb_viewer_play(self);
		}
	}
	if (CMB_I32(self->prev_anim, 0x14) != 0) {
		count = CMB_U32(self->actor, 0x14c);
		if (count != 0) {
			self->anim = (self->anim - 1 + count) % count;
			cmb_viewer_play(self);
		}
	}
	value = CMB_I32(self->cam_y, 0x14);
	if (value != 0)
		self->target[1] = (float)value * _DAT_002839a0 + self->target[1];
}

/* make a light with grey colour and a direction */
static void *cmb_viewer_light(float a, float b)
{
	float direction[4];
	void *light;

	light = ((void *(*)(void *, unsigned int))FUN_000382f0)(NULL, 0);
	CMB_I32(light, 0x14) = 0x7f;
	CMB_I32(light, 0x18) = 0x7f;
	CMB_I32(light, 0x1c) = 0x7f;
	((void (*)(float *, const float *, float, float))FUN_00051220)(direction, DAT_0028fdd8, a, b);
	((void (*)(void *, const float *))FUN_00038550)(light, direction);
	((void (*)(void *, int))FUN_00038370)(light, DAT_0028398c);
	return light;
}

/* 0x00025e70  (unplaced): Enter */
void CGameMode_ObjectViewer_Enter(struct CGameMode_ObjectViewer *self)
{
	unsigned int pad = (unsigned int)DAT_002baed4[0][3];

#define CMB_CHANNEL(name, type, flags) \
	(((void *(*)(void *, const char *, int, unsigned int, int, int, int))FUN_00048a10)( \
		self->input, (name), (type), (flags), 0, 0, 0))
#define CMB_BIND(channel, input, other) \
	(((int (*)(void *, unsigned int, unsigned int))FUN_00048b20)((channel), (input), (other)))

	self->resource = cmb_next_resource(1, NULL);
	self->camera = ((void *(*)(void *))FUN_0003bf10)(NULL);
	self->actor = CGameMode_ObjectViewer_MakeActor(self, self->resource);
	self->target[0] = DAT_002ea478[0];
	self->target[1] = DAT_002ea478[1];
	self->target[2] = DAT_002ea478[2];
	self->target[3] = DAT_002ea478[3];
	self->distance = 1000.0f;
	self->input = ((void *(*)(int))FUN_000488c0)(0x14);
	self->next_actor = CMB_CHANNEL("NextActor", 1, 1);
	self->prev_actor = CMB_CHANNEL("PrevActor", 1, 1);
	self->zoom = CMB_CHANNEL("ZoomIn", 3, 0);
	self->rot_x = CMB_CHANNEL("RotX", 3, 0);
	self->rot_y = CMB_CHANNEL("RotY", 3, 0);
	self->rot_z = CMB_CHANNEL("RotZ", 3, 0);
	self->next_anim = CMB_CHANNEL("NextAnim", 1, 1);
	self->prev_anim = CMB_CHANNEL("PrevAnim", 1, 1);
	self->cam_y = CMB_CHANNEL("camTargetY", 3, 0);
	CMB_BIND(self->next_actor, pad | 0xc5, 0);
	CMB_BIND(self->prev_actor, pad | 0xc6, 0);
	CMB_BIND(self->zoom, pad | 0xcb, pad | 0xcc);
	CMB_BIND(self->rot_x, pad | 0xba, 0);
	CMB_BIND(self->rot_y, pad | 0xb9, 0);
	CMB_BIND(self->rot_z, pad | 0xbb, 0);
	CMB_BIND(self->next_anim, pad | 199, 0);
	CMB_BIND(self->prev_anim, pad | 200, 0);
	CMB_BIND(self->cam_y, pad | 0xbc, 0);
	DAT_002ba4e8 = cmb_viewer_light(-0.5f, 0.5f);
	DAT_002ba4ec = cmb_viewer_light(-0.8f, 2.5f);
#undef CMB_CHANNEL
#undef CMB_BIND
}
