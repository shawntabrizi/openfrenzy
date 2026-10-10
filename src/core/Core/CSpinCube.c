/*
CSPINCUBE.C  <-  C:\BlitzParty\Game\Src\Core\CSpinCube.cpp

openfrenzy core module: spinning cubes with a game picture on each face.
  CSpinCube    base, vtable 0x00257604 (destroy, update: nothing)
  CMapCube     vtable 0x0025760c (overlaps the base vtable): spins over the valid games of a
               board zone and stops on one (CMapCube::FindValidGames)
  CSelectCube  vtable 0x00257680: turns a quarter to show the next or previous picture
Decompiled from orig/default.xbe (functional, not byte-matching). 0x0002d560-0x0002db40
and 0x0002e270 are in the source-map gaps around this file; they use the cube layout and
the shared face geometry (0x002bab38), so they are placed here.
*/

#include <stdio.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"

static const char spincube_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CSpinCube.cpp";

/* a face entry, 0x14 bytes */
struct CubeFace {
	int game;      /* +0x00 */
	int variation; /* +0x04 */
	int special;   /* +0x08 */
	void *picture; /* +0x0c */
	void *blank;   /* +0x10 the texture drawn on the cube */
};

struct CSpinCube {
	const void *const *vtbl;  /* +0x00 */
	float matrix[16];         /* +0x04 */
	struct CubeFace *faces;   /* +0x44 */
	float angle;              /* +0x48 */
	float spin;               /* +0x4c */
	float glow;               /* +0x50 the front face picture, 0-127 */
	uint32_t field_54;        /* +0x54 */
	int face_index[3];        /* +0x58 the faces on the three sides */
	struct CubeFace *chosen;  /* +0x64 */
	float scale;              /* +0x68 */
	/* CMapCube */
	void *blanks[8];          /* +0x6c (CSelectCube: +0x6c direction) */
	int count;                /* +0x8c */
};
_Static_assert(sizeof(struct CSpinCube) == 0x90, "CSpinCube size");
#define CUBE_DIRECTION(self) (*(int *)&(self)->blanks[0])

extern const void *const vtable_00257604[4];  /* = CSpinCube [0..1], CMapCube [2..3] */
#define vtable_0025760c (vtable_00257604 + 2)
extern const void *const vtable_00257680[2];

/* the cube faces: 3 sides of 4 vertices (0x24 bytes), the cap, the front picture */
#define CUBE_SIDES ((uint32_t *)CORE_ADDR(0x002bab38))
#define CUBE_CAP ((uint32_t *)CORE_ADDR(0x002bace8))
#define CUBE_FRONT ((uint32_t *)CORE_ADDR(0x002bad78))
#define CUBE_CORNERS ((const float *)CORE_ADDR(0x00284450))   /* 4 x (x, y, z, w) */

#define ext_MatrixTranslate ((void (*)(float *, float, float, float))FUN_00050250)
#define ext_MatrixScale ((void (*)(float *, float, float, float))FUN_000502a0)
#define ext_MatrixRotateZ ((void (*)(float *, float))FUN_000507a0)
#define ext_MatrixRotateX ((void (*)(float *, float))FUN_00050750)
#define ext_MatrixRotateXY ((void (*)(float *, float, float))FUN_00050840)
#define ext_MatrixMulAligned ((void (*)(float *, const float *, const float *))FUN_00050150)
#define ext_MatrixMul ((void (*)(float *, const float *, const float *))FUN_0004ff30)
#define ext_Transform ((void (*)(float *, const float *, const float *))FUN_000505c0)
#define ext_SetWorld ((void (*)(const float *))FUN_00043930)

/* the two matrix products are the same; the original picks the SSE one when all three
matrices are 16-byte aligned */
static void cube_multiply(float *out, const float *a, const float *b)
{
	if ((((uintptr_t)out | (uintptr_t)a | (uintptr_t)b) & 0xf) != 0)
		ext_MatrixMul(out, a, b);
	else
		ext_MatrixMulAligned(out, a, b);
}

/* four corners of a face through MATRIX, colour COLOUR */
static void cube_face(uint32_t *face, const float *matrix, uint32_t colour)
{
	float out[4];
	int i;

	for (i = 0; i < 4; i++)
	{
		uint32_t *vertex = face + i * 9;

		vertex[6] = colour;
		ext_Transform(out, matrix, CUBE_CORNERS + i * 4);
		memcpy(vertex, out, 3 * sizeof(float));
	}
}

static void cube_face_uv(uint32_t *face)
{
	face[7] = 0;
	face[8] = 0;
	face[9 + 7] = 0x3f800000;
	face[9 + 8] = 0;
	face[18 + 7] = 0x3f800000;
	face[18 + 8] = 0x3f800000;
	face[27 + 7] = 0;
	face[27 + 8] = 0x3f800000;
}

/* 0x0002d560  (Core, CSpinCube.cpp) build the shared face geometry at the cube's scale */
void CSpinCube_BuildFaces(struct CSpinCube *self)
{
	__attribute__((aligned(16))) float scale[16];
	__attribute__((aligned(16))) float rotation[16];
	__attribute__((aligned(16))) float matrix[16];
	int side;

	ext_MatrixScale(scale, self->scale, self->scale, self->scale);
	for (side = 0; side < 3; side++)
	{
		uint32_t *face = CUBE_SIDES + side * 0x24;

		ext_MatrixRotateZ(rotation, (float)((double)-side * (double)1.5707963f));
		cube_multiply(matrix, scale, rotation);
		cube_face(face, matrix, DAT_002bc898[30] | DAT_002bc490[110] | DAT_002bce18[100] | DAT_002bd640[127]);
		cube_face_uv(face);
	}
	cube_face_uv(CUBE_CAP);
	ext_MatrixRotateX(rotation, 1.5707963f);
	cube_multiply(matrix, scale, rotation);
	cube_face(CUBE_CAP, matrix, DAT_002bc898[32] | DAT_002bd640[127] | DAT_002bc490[90] | DAT_002bce18[70]);
	cube_face_uv(CUBE_FRONT);
	cube_face(CUBE_FRONT, scale, DAT_002bc898[127] | DAT_002bc490[127] | DAT_002bce18[127] | DAT_002bd640[127]);
}

/* 0x0002d830  (Core, CSpinCube.cpp) draw the cube: seven turns of the side faces, the cap,
and the front picture while it glows */
void CSpinCube_Draw(struct CSpinCube *self)
{
	static const float eye[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	__attribute__((aligned(16))) float rotation[16];
	__attribute__((aligned(16))) float world[16];
	int turn, side;

	ext_SetCamera(eye, (const float *)CORE_ADDR(0x00284440), 0.0f);
	ext_SetClipPlanes(10.0f, 1000.0f);
	ext_SetFov(1.0471976f, 0.0f);
	ext_ApplyCamera();
	ext_SetRenderMode(1);
	ext_SetRenderState(10, 0, 0);
	ext_SetRenderState(4, 1, 0);
	ext_SetRenderState(7, 1, 0);
	ext_SetRenderState(5, 1, 0);
	ext_SetRenderState(3, 0, 0);
	ext_SetRenderState(8, 0, 0);
	for (turn = 3; turn >= -3; turn--)
	{
		ext_MatrixRotateZ(rotation, (float)((double)turn * (double)self->spin + (double)self->angle));
		cube_multiply(world, self->matrix, rotation);
		ext_SetWorld(world);
		for (side = 0; side < 3; side++)
		{
			if (self->face_index[side] >= 0)
			{
				ext_SetTexture(0, (int)(uintptr_t)self->faces[self->face_index[side]].blank);
				ext_DrawPrimitives(5, CUBE_SIDES + side * 0x24, 4);
			}
		}
		ext_SetTexture(0, (int)(uintptr_t)self->faces[0].blank);
		ext_DrawPrimitives(5, CUBE_CAP, 4);
	}
	if (self->glow != 0.0f && self->face_index[0] >= 0)
	{
		ext_MatrixRotateZ(rotation, self->angle);
		cube_multiply(world, self->matrix, rotation);
		ext_SetWorld(world);
		ext_SetTexture(0, (int)(uintptr_t)self->faces[self->face_index[0]].picture);
		ext_DrawPrimitives(5, CUBE_FRONT, 4);
	}
	ext_SetRenderState(4, 0, 0);
}

/* 0x0002da70  (Core, CSpinCube.cpp) CSelectCube: show PICTURE now */
void CSelectCube_Set(struct CSpinCube *self, int unused1, int unused2, void *picture)
{
	(void)unused1;
	(void)unused2;
	self->glow = 0.0f;
	self->faces[0].blank = picture;
	self->faces[0].picture = picture;
}

/* 0x0002da90  (Core, CSpinCube.cpp) CSelectCube: turn to PICTURE, forward or back */
void CSelectCube_Turn(struct CSpinCube *self, int unused1, int unused2, void *picture, char forward)
{
	(void)unused1;
	(void)unused2;
	if (forward != 0)
	{
		if (self->angle > 0.0f)
			memmove(self->faces, (char *)self->faces + 0x14, 0x28);
		CUBE_DIRECTION(self) = 1;
		self->glow = 0.0f;
		self->spin = 0.15f;
		self->angle = 0.0f;
		self->faces[1].blank = picture;
		self->faces[CUBE_DIRECTION(self)].picture = picture;
		return;
	}
	self->glow = 0.0f;
	self->spin = -0.15f;
	CUBE_DIRECTION(self) = -1;
	memmove((char *)self->faces + 0x14, self->faces, 0x28);
	self->angle = 1.5707963f;
	self->faces[0].blank = picture;
	self->faces[0].picture = picture;
}

/* 0x0002db40  (Core, CSpinCube.cpp) CSelectCube update (vtable +0x04): turn; glow when still */
void CSelectCube_Update(struct CSpinCube *self)
{
	float angle, glow;

	if (self->spin == 0.0f)
	{
		glow = DAT_0028fda8 * 5.0f + self->glow;
		if (glow >= 127.0f)
			glow = 127.0f;
		self->glow = glow;
		return;
	}
	angle = self->spin + self->angle;
	self->angle = angle;
	glow = self->glow - DAT_0028fda8 * 5.0f;
	if (glow <= 0.0f)
		glow = 0.0f;
	self->glow = glow;
	if (self->spin <= 0.0f)
	{
		if (angle < 0.0f)
		{
			self->spin = 0.0f;
			self->angle = 0.0f;
			CUBE_DIRECTION(self) = 0;
		}
	}
	else if (angle >= 1.5707963f)
	{
		self->spin = 0.0f;
		self->angle = 0.0f;
		CUBE_DIRECTION(self) = 0;
		memmove(self->faces, (char *)self->faces + 0x14, 0x28);
	}
}

/* 0x0002dc10  Core/CSpinCube.cpp  CSpinCube scalar deleting destructor */
struct CSpinCube *CSpinCube_Destroy(struct CSpinCube *self, int flags)
{
	self->vtbl = vtable_00257604;
	if (flags & 1)
		ext_Delete(self);
	return self;
}

/* 0x0002dc30  (Core, CSpinCube.cpp) CSpinCube::CSpinCube */
struct CSpinCube *CSpinCube_Ctor(struct CSpinCube *self)
{
	__attribute__((aligned(16))) float translation[16];
	__attribute__((aligned(16))) float rotation[16];

	self->chosen = NULL;
	self->angle = 0.0f;
	self->glow = 0.0f;
	self->vtbl = vtable_00257604;
	self->spin = 0.2f;
	self->scale = 75.0f;
	self->face_index[0] = -1;
	self->face_index[1] = -1;
	self->face_index[2] = -1;
	CSpinCube_BuildFaces(self);
	ext_MatrixTranslate(translation, CORE_VAR(float, 0x00284430), CORE_VAR(float, 0x00284434),
		CORE_VAR(float, 0x00284438));
	ext_MatrixRotateXY(rotation, 0.0f, 0.5235988f);
	cube_multiply(self->matrix, translation, rotation);
	return self;
}

/* 0x0002dcf0  (Core, CSpinCube.cpp) CMapCube::~CMapCube */
void CMapCube_Dtor(struct CSpinCube *self)
{
	self->vtbl = vtable_0025760c;
	if (self->faces != NULL)
		ext_Delete(self->faces);
	self->vtbl = vtable_00257604;
}

/* game picture prefixes (the .data table at 0x00283900; read only) */
static const char *const cube_game_prefix[13] = {
	"meta", "bump", "fire", "clust", "dem", "drill", "tag", "hop", "bug", "roll", "row",
	"scratch", "tank",
};

/* 0x0002dd50  Core/CSpinCube.cpp  CMapCube::FindValidGames: the games and variations of
board ZONE that may be played (not the last game, not played yet, enabled); when fewer than
two, a random zone (DEPTH counts the retries) */
void CMapCube_FindValidGames(struct CSpinCube *self, int zone, int depth)
{
	const unsigned char *entry = (const unsigned char *)CORE_ADDR(0x00283c70) + zone * 0x20;
	char name[0x40];
	int k, variation;

	self->count = 0;
	self->faces = ext_New(0xa0, spincube_cpp, 0x144, 0x1001);
	for (k = 0; k < *(const uint16_t *)(entry + 0xc); k++)
	{
		int game = *(const int *)(entry + 0x10 + k * 8);
		int excluded = *(const int *)(entry + 0x14 + k * 8);

		for (variation = 0; variation < 4; variation++)
		{
			int playable = ((const int *)CORE_ADDR(0x002839e4))[game * 4 + variation];
			int level, special;
			struct CubeFace *face;

			if (game == CORE_VAR(int, 0x002baeac) || excluded == 2 || playable == 0)
				continue;
			if (playable > 0 && CORE_VAR(int, 0x002839b8) == 0)   /* "Play Electric" */
				continue;
			if (playable < 0 && CORE_VAR(int, 0x002839bc) == 0)   /* "Play Enigma" */
				continue;
			if (((int *)CORE_ADDR(0x002ba564))[game * 4 + variation] != 0)  /* played */
				continue;
			face = &self->faces[self->count];
			face->game = game;
			self->faces[self->count].variation = variation;
			level = playable < 0 ? -playable : playable;
			special = 0;
			if (level == 3)
				special = 1;
			else if (level == 2 && CORE_VAR(int, 0x002ba634) != -1)
			{
				DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
				if ((int)((DAT_00285630 >> 16) & 0x7fff) % 100 < 0x32 || CORE_VAR(int, 0x002ba634) != 0)
					special = 1;
			}
			self->faces[self->count].special = special;
			sprintf(name, "%s%d.bmp", cube_game_prefix[game], variation + 1);
			self->faces[self->count].picture = ext_FindResource(0, name, 0, 0xdefa);
			self->faces[self->count].blank = self->blanks[self->count];
			((void (*)(void *, void *))FUN_00082d10)(self->faces[self->count].blank,
				self->faces[self->count].picture);
			self->count++;
		}
	}
	if (self->count >= 2)
	{
		ext_PlayVoice(0x36, -1, 0xff, 0, 0, 0);
		return;
	}
	ext_DebugPrintf("CMapCube::FindValidGames : Couldn't find ANY valid games?\n");
	DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
	CORE_VAR(int, 0x002baeac) = -1;
	CMapCube_FindValidGames(self, (int)((DAT_00285630 >> 16) & 0x7fff) % 6, depth + 1);
}

/* 0x0002dff0  Core/CSpinCube.cpp  CMapCube update (vtable +0x04): spin and slow down; each
quarter turn brings a new random game; stop on one when slow */
void CMapCube_Update(struct CSpinCube *self)
{
	if (0.02f <= self->spin)
	{
		float spin = self->spin - DAT_0028fda8 * 0.001f;

		self->spin = spin;
		self->angle = spin * DAT_0028fda8 + self->angle;
	}
	else if (self->chosen == NULL)
		self->angle = DAT_0028fda8 * 0.02f + self->angle;
	if (self->angle > 1.5707963f)
	{
		int next;

		self->angle = self->angle - 1.5707963f;
		memmove(&self->face_index[0], &self->face_index[1], 8);
		do
		{
			DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
			next = (int)((DAT_00285630 >> 16) & 0x7fff) % self->count;
			self->face_index[2] = next;
		} while (next == self->face_index[1]);
		if (self->spin < 0.02f)
		{
			self->chosen = &self->faces[self->face_index[0]];
			self->glow = 127.0f;
		}
	}
}

/* 0x0002e0d0  Core/CSpinCube.cpp  CSelectCube::CSelectCube */
struct CSpinCube *CSelectCube_Ctor(struct CSpinCube *self)
{
	CSpinCube_Ctor(self);
	self->vtbl = vtable_00257680;
	self->faces = ext_Alloc(0x3c, 0, spincube_cpp, 0x1db, 0x2001);
	self->face_index[0] = 0;
	self->face_index[1] = 1;
	self->face_index[2] = 2;
	CUBE_DIRECTION(self) = 0;
	self->spin = 0.0f;
	self->angle = 0.0f;
	ext_MatrixTranslate(self->matrix, 140.0f, 0.0f, 554.0f);
	return self;
}

/* 0x0002e160  (Core, CSpinCube.cpp) CSelectCube::~CSelectCube */
void CSelectCube_Dtor(struct CSpinCube *self)
{
	self->vtbl = vtable_00257680;
	ext_Delete(self->faces);
	self->vtbl = vtable_00257604;
}

/* 0x0002e1b0  (Core, CSpinCube.cpp) CMapCube::CMapCube(zone) */
struct CSpinCube *CMapCube_Ctor(struct CSpinCube *self, int zone)
{
	char name[0x20];
	int i;

	CSpinCube_Ctor(self);
	self->vtbl = vtable_0025760c;
	for (i = 0; i < 8; i++)
	{
		sprintf(name, "blank%d.bmp", i);
		self->blanks[i] = ext_FindResource(0, name, 0, 0xdefa);
	}
	CMapCube_FindValidGames(self, zone, 0);
	return self;
}

/* 0x0002e250  Core/CSpinCube.cpp  CMapCube scalar deleting destructor */
struct CSpinCube *CMapCube_Destroy(struct CSpinCube *self, int flags)
{
	CMapCube_Dtor(self);
	if (flags & 1)
		ext_Delete(self);
	return self;
}

/* 0x0002e270  (Core, CSpinCube.cpp) CSelectCube scalar deleting destructor */
struct CSpinCube *CSelectCube_Destroy(struct CSpinCube *self, int flags)
{
	CSelectCube_Dtor(self);
	if (flags & 1)
		ext_Delete(self);
	return self;
}

/* 0x00257604: CSpinCube {destroy, update}, followed by CMapCube's at 0x0025760c */
const void *const vtable_00257604[4] = {
	(const void *)CSpinCube_Destroy,
	(const void *)FUN_001d5950,
	(const void *)CMapCube_Destroy,
	(const void *)CMapCube_Update,
};

/* 0x00257680: CSelectCube */
const void *const vtable_00257680[2] = {
	(const void *)CSelectCube_Destroy,
	(const void *)CSelectCube_Update,
};
