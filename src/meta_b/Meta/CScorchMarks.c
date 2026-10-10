/*
Meta/CScorchMarks.c  <-  C:\BlitzParty\Game\Src\Meta\CScorchMarks.cpp

Scorch marks: textured quads laid on a surface (position and normal), turned to a
parent's heading or at random, that fade out. At most 100 at once (the oldest goes).
All marks are in the list at 0x00308f2c.

0x001605e0 is tagged with CScorchMarks.cpp; the functions in front of it work on the
same marks and list (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* 0x002b0a64  the two scorch mark types */
struct MbScorchType DAT_002b0a64[2] = {
	{ "scorch.bmp", NULL, 90.0f, 100.0f, 0.1f },
	{ "scorch2.bmp", NULL, 60.0f, 90.0f, 0.1f },
};

/* 0x00308f28  number of marks */
int DAT_00308f28;
/* 0x00308f2c  the marks (count at 0x00308f34) */
struct MbList DAT_00308f2c;

/* 0x0015ff80  Meta (file inferred: CScorchMarks.cpp)
Loads the textures of the mark types. */
void CScorchMarks_Init(void)
{
	struct MbScorchType *type;

	DAT_00308f28 = 0;
	for (type = DAT_002b0a64; type < DAT_002b0a64 + 2; type++)
		type->texture = bLoadResource(NULL, type->name, 0, 0xdefa);
}

/* 0x0015ffc0  Meta (file inferred: CScorchMarks.cpp)
Draws one mark: grey and alpha both from its fading alpha. */
void CScorchMark_Draw(struct CScorchMark *self)
{
	int a = (int)self->alpha;

	self->verts[0].colour = DAT_002bd640[a] | DAT_002bc898[a] | DAT_002bc490[a] | DAT_002bce18[a];
	self->verts[1].colour = DAT_002bd640[a] | DAT_002bc898[a] | DAT_002bc490[a] | DAT_002bce18[a];
	self->verts[2].colour = DAT_002bd640[a] | DAT_002bc898[a] | DAT_002bc490[a] | DAT_002bce18[a];
	self->verts[3].colour = DAT_002bd640[a] | DAT_002bc898[a] | DAT_002bc490[a] | DAT_002bce18[a];
	bdSetTexture(0, (struct bdTexture *)self->texture);
	bdDrawPrimitive(4, self->verts, 4);
}

/* 0x00160060  Meta (file inferred: CScorchMarks.cpp)
Draws every mark (subtractive blend, a small z bias). */
void CScorchMarks_DrawAll(void)
{
	struct CScorchMark *mark;

	if (DAT_00308f2c.count == 0)
		return;
	FUN_000438b0();
	FUN_00044550(-0.2f);
	((void (*)(int, int, int))FUN_00046cb0)(6, 1, 0);
	((void (*)(int, int, int))FUN_00046cb0)(10, 3, 0);
	((void (*)(int, int, int))FUN_00046cb0)(3, 0, 0);
	((void (*)(int, int, int))FUN_00046cb0)(5, 0, 0);
	((void (*)(int, int, int))FUN_00046cb0)(7, 2, 0);
	for (mark = MB_LIST_NEXT(&DAT_00308f2c, NULL); mark != NULL;
		mark = MB_LIST_NEXT(&DAT_00308f2c, mark))
		CScorchMark_Draw(mark);
	FUN_00046d00(6);
	FUN_00046d00(10);
	FUN_00046d00(3);
	FUN_00046d00(5);
	FUN_00046d00(7);
	FUN_00044550(0.0f);
}

/* 0x00160120  Meta (file inferred: CScorchMarks.cpp)
Builds the quad of a mark (4 strip vertices, positions and uv) of half size `size`
centred on `pos`, lying on the plane with `normal`, turned by `yaw`. */
void CScorchMark_BuildQuad(struct MbVertex *verts, float *pos, float *normal, float size,
	float yaw)
{
	float ax;
	float ay;
	float az;
	float ref[3];
	float u[3];
	float v[3];
	float nx;
	float ny;
	float nz;
	float dx;
	float dy;
	float dz;
	char axis = 0;

	ax = normal[0];
	if (normal[0] < 0.0f)
		ax = -ax;
	if (0.0f <= normal[1])
		ay = normal[1];
	else
		ay = -normal[1];
	az = normal[2];
	if (normal[2] < 0.0f)
		az = -az;
	if (ax <= ay && az <= ay)
		axis = 1;
	if (ay <= az && ax <= az)
		axis = 2;
	/* a reference direction across the normal's main axis */
	if (axis == 1)
	{
		ref[1] = 0.0f;
		ref[0] = 1.0f;
		ref[2] = 0.0f;
	}
	else if (axis == 2)
	{
		ref[0] = 0.0f;
		ref[1] = 1.0f;
		ref[2] = 0.0f;
	}
	else
	{
		ref[0] = 0.0f;
		ref[1] = 0.0f;
		ref[2] = 1.0f;
	}
	nx = normal[0];
	ny = normal[1];
	nz = normal[2];
	FUN_00051170(ref, ref, yaw);
	u[0] = ref[2] * ny - ref[1] * nz;
	u[1] = ref[0] * nz - ref[2] * nx;
	u[2] = ref[1] * nx - ref[0] * ny;
	v[0] = u[1] * nz - u[2] * ny;
	v[1] = u[2] * nx - u[0] * nz;
	v[2] = u[0] * ny - u[1] * nx;
	FUN_000514a0(u, u);
	FUN_000514a0(v, v);
	dx = u[0] * size + v[0] * size;
	verts[0].x = pos[0] - dx;
	dy = u[1] * size + v[1] * size;
	verts[0].y = pos[1] - dy;
	dz = u[2] * size + v[2] * size;
	verts[0].u = 0.0f;
	verts[0].v = 1.0f;
	verts[0].z = pos[2] - dz;
	verts[3].x = dx + pos[0];
	verts[3].y = dy + pos[1];
	verts[3].v = 0.0f;
	verts[3].u = 1.0f;
	verts[3].z = dz + pos[2];
	dx = u[0] * size - v[0] * size;
	verts[2].x = dx + pos[0];
	dy = u[1] * size - v[1] * size;
	verts[2].y = dy + pos[1];
	dz = u[2] * size - v[2] * size;
	verts[2].u = 1.0f;
	verts[2].v = 1.0f;
	verts[2].z = dz + pos[2];
	verts[1].x = pos[0] - dx;
	verts[1].y = pos[1] - dy;
	verts[1].u = 0.0f;
	verts[1].v = 0.0f;
	verts[1].z = pos[2] - dz;
}

/* 0x001603b0  Meta (file inferred: CScorchMarks.cpp)
Deletes every mark. */
void CScorchMarks_DeleteAll(void)
{
	void *mark;

	mark = MB_LIST_NEXT(&DAT_00308f2c, NULL);
	while (mark != NULL)
	{
		MB_LIST_REMOVE(&DAT_00308f2c, mark);
		if (mark != NULL)
		{
			DAT_00308f28--;
			MB_DELETE(mark);
		}
		mark = MB_LIST_NEXT(&DAT_00308f2c, NULL);
	}
}

/* 0x00160400  Meta (file inferred: CScorchMarks.cpp)
Constructor: a mark of `type` at `pos` on the surface with `normal`, turned to the
parent's heading (or at random without a parent). The 101st mark deletes the oldest. */
struct CScorchMark *CScorchMark_Construct(struct CScorchMark *self, float *pos, float *normal,
	void *parent, int type)
{
	void *oldest;
	float yaw;

	DAT_00308f28++;
	if (DAT_00308f28 > 100)
	{
		oldest = MB_LIST_NEXT(&DAT_00308f2c, NULL);
		if (oldest != NULL)
		{
			MB_LIST_REMOVE(&DAT_00308f2c, oldest);
			DAT_00308f28--;
			MB_DELETE(oldest);
		}
	}
	self->parent = parent;
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	self->normal[0] = normal[0];
	self->normal[1] = normal[1];
	self->normal[2] = normal[2];
	self->normal[3] = normal[3];
	self->texture = DAT_002b0a64[type].texture;
	self->size = DAT_002b0a64[type].size;
	self->alpha = DAT_002b0a64[type].alpha;
	self->fade = DAT_002b0a64[type].fade;
	if (self->parent == NULL)
		yaw = (float)mb_rand() * MB_RAND_ANGLE;
	else
		yaw = MB_F32(self->parent, 0x1f8);
	CScorchMark_BuildQuad(self->verts, self->pos, self->normal, self->size, yaw);
	MB_LIST_INSERT(&DAT_00308f2c, self, 1);
	return self;
}

/* 0x00160560  Meta (file inferred: CScorchMarks.cpp)
Fades every mark; deletes those below alpha 1. */
void CScorchMarks_UpdateAll(void)
{
	struct CScorchMark *mark;
	void *prev;

	mark = MB_LIST_NEXT(&DAT_00308f2c, NULL);
	while (mark != NULL)
	{
		mark->alpha = mark->alpha - DAT_0028fda8 * mark->fade;
		prev = mark;
		if (mark->alpha < 1.0f)
		{
			prev = MB_LIST_REMOVE(&DAT_00308f2c, mark);
			DAT_00308f28--;
			MB_DELETE(mark);
		}
		mark = MB_LIST_NEXT(&DAT_00308f2c, prev);
	}
}

/* 0x001605e0  Meta/CScorchMarks.cpp
Creates a scorch mark (type 0 or 1); NULL for another type. */
struct CScorchMark *CScorchMark_Create(float *pos, float *normal, void *parent, int type)
{
	struct CScorchMark *mark;

	if (type < 2)
	{
		mark = (struct CScorchMark *)MB_NEW(0xcc,
			"C:\\BlitzParty\\Game\\Src\\Meta\\CScorchMarks.cpp", 0x16a);
		if (mark != NULL)
			return CScorchMark_Construct(mark, pos, normal, parent, type);
	}
	else
	{
		bkPrintf("no such scorch mark type!\n");
	}
	return NULL;
}
