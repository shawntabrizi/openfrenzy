/*
Demolition/CDemolition_Wall.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\CDemolition_Wall.cpp

CDemolition_Wall (vtable 0x0026bef0, 0x1c4 bytes): a wall of rows x cols bricks (every
other row shifted by half a brick) in one vertex buffer. Explosions knock bricks out; they
fly off, spin and fall, and the faces of their neighbours open up. demolition_c's lift and
girders use it.

Only 0x0010f5e0 (the initialiser) is tagged with this file. The other functions between the
crate and it (0x0010db80-0x0010f5b0) are methods of the same object (file inferred).
0x0010db80 and 0x0010db90 (constructor and destructor) are missing from the function table.

The methods that other modules call with an `(int)wall` argument take `int self`, so those
calls keep compiling; the value is the wall's address (ILP32).
*/

#include "demolition_d.h"

static const char s_WallFile[] = "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Wall.cpp";

/* bdisplay colour tables (index = channel value); the wall's vertex colour is
   0x2bc590 | 0x2bca94 | 0x2bcf18 | 0x2bd740 */
extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bc898[256];
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bd640[256];
/* the identity matrix (engine) */
extern float DAT_002855f0[16];

/* demolition_a's light system (game +0x6ca4): enable / set a light type */
#define WALL_LIGHTS() ((void *)(DD_GAME + 0x6ca4))
#define Wall_LightsEnable(on, type) DD_FN(FUN_000ed970, void, (void *, int, int))(WALL_LIGHTS(), (on), (type))
#define Wall_LightsSet(v, type) DD_FN(FUN_000ed9a0, void, (void *, int, int))(WALL_LIGHTS(), (v), (type))

/* the texture rectangle of the wall: u from 0.98535156 down by 0.98535156 / cols,
   v from 0.66601562 down by 0.66601562 / rows */
#define WALL_U0 0.985351562f
#define WALL_V0 0.666015625f

/* 0x0026bef0  the vtable */
const void *const vtable_0026bef0[1] = {
	(const void *)FUN_0010dff0, /* [0] scalar deleting destructor */
};

#define WALL(p) ((DemoWall *)(p))

/* 0x0010db80  Demolition (missing from the function table; file inferred)
Constructor. */
void *FUN_0010db80(void *self)
{
	WALL(self)->vtable = (const void *const *)vtable_0026bef0;
	return self;
}

/* 0x0010db90  Demolition (missing from the function table; file inferred)
Destructor. */
void FUN_0010db90(void *self)
{
	WALL(self)->vtable = (const void *const *)vtable_0026bef0;
}

/* 0x0010dba0  Demolition (gap; file inferred)
Sets the height of the wall (its position's y). */
void DemoWall_SetY(int self, float y)
{
	WALL(self)->pos[1] = y;
	WALL(self)->dirty = 1;
	WALL(self)->posDirty = 1;
}

/* 0x0010dbc0  Demolition (gap; file inferred)
Sets the wall's turn about y. */
void DemoWall_SetAngle(int self, float angle)
{
	WALL(self)->angle = angle;
	WALL(self)->dirty = 1;
	WALL(self)->angleDirty = 1;
}

/* 0x0010dbe0  Demolition (gap; file inferred)
Sets the brick texture. */
void DemoWall_SetTexture(int self, int texture)
{
	WALL(self)->texture = (void *)texture;
}

/* 0x0010dbf0  Demolition (gap; file inferred)
Draws the flying bricks (after the wall's vertices), with light type 1 switched. */
void DemoWall_DrawFlying(DemoWall *self)
{
	if (self->numFlying > 0 && self->numFlyingVerts > 0) {
		Wall_LightsSet(0, 1);
		FUN_0003f4a0(4, self->vb, self->numWallVerts, self->numFlyingVerts);
		Wall_LightsSet(1, 1);
	}
}

/* 0x0010dc60  Demolition (gap; file inferred)
Gives every vertex of the buffer the wall colour. */
void DemoWall_SetColours(DemoWall *self)
{
	DemoWallVertex *v;
	int i;

	v = (DemoWallVertex *)FUN_0003f710(self->vb, 0);
	if (v == NULL)
		return;
	self->numWallVerts = self->cols * self->rows * 0x24;
	for (i = 0; i < self->numWallVerts; i++)
		v[i].colour = DAT_002bc898[127] | DAT_002bd640[64] | DAT_002bc490[64] | DAT_002bce18[64];
	FUN_0003f790(self->vb);
}

/* 0x0010dce0  Demolition (gap; file inferred)
Puts every brick back in the wall; only the outer faces show. */
void DemoWall_ResetBricks(DemoWall *self)
{
	DemoBrick *b;
	int idx;
	int row;
	int col;

	idx = 0;
	for (row = 0; row < self->rows; row++) {
		for (col = 0; col < self->cols; col++) {
			b = &self->bricks[idx];
			b->solid = 1;
			b->flying = 0;
			b->faceTop = 1;
			b->faceBottom = 1;
			b->faceLeft = 1;
			b->faceRight = 1;
			if (row == 0)
				b->faceBottom = 0;
			else if (row == self->rows - 1)
				b->faceTop = 0;
			if (col == 0)
				b->faceLeft = 0;
			else if (col == self->cols - 1)
				b->faceRight = 0;
			idx++;
		}
	}
}

/* 0x0010ddb0  Demolition (gap; file inferred)
Opens the faces of the bricks next to knocked-out ones (a face flag of 0 draws the face;
rows are shifted by half a brick, so the brick above or below is idx +- cols and the one
beside it, +1 on even rows, -1 on odd rows). */
void DemoWall_UpdateFaces(DemoWall *self)
{
	DemoBrick *bricks;
	DemoBrick *b;
	int idx;
	int row;
	int col;
	int cols;
	int open;

	idx = 0;
	for (row = 0; row < self->rows; row++) {
		cols = self->cols;
		for (col = 0; col < cols; col++, idx++, cols = self->cols) {
			bricks = self->bricks;
			b = &bricks[idx];
			if (b->solid == 0)
				continue;
			if (row == 0) {
				if (bricks[cols + idx].solid == 0)
					b->faceTop = 0;
				if (self->bricks[self->cols + idx + 1].solid == 0)
					b->faceTop = 0;
			} else {
				if ((unsigned int)row == (unsigned int)self->rows - 1u) {
					b->faceTop = 0;
					if (self->bricks[idx - self->cols].solid == 0)
						b->faceBottom = 0;
				} else {
					if (bricks[cols + idx].solid == 0)
						b->faceTop = 0;
					if ((row & 1) == 0)
						open = self->bricks[self->cols + idx + 1].solid == 0;
					else
						open = self->bricks[self->cols + idx - 1].solid == 0;
					if (open)
						b->faceTop = 0;
					if (self->bricks[idx - self->cols].solid == 0)
						b->faceBottom = 0;
				}
				if ((row & 1) == 0)
					open = self->bricks[idx - self->cols + 1].solid == 0;
				else
					open = self->bricks[idx - self->cols - 1].solid == 0;
				if (open)
					b->faceBottom = 0;
			}
			if (col == 0) {
				b->faceLeft = 0;
				if (self->bricks[idx + 1].solid == 0)
					b->faceRight = 0;
			} else {
				if (col == self->cols - 1 || self->bricks[idx + 1].solid == 0)
					b->faceRight = 0;
				if (self->bricks[idx - 1].solid == 0)
					b->faceLeft = 0;
			}
		}
	}
}

/* 0x0010dff0  Demolition (gap; file inferred)
Scalar deleting destructor. */
DemoWall *DemoWall_Delete(DemoWall *self, unsigned int flags)
{
	self->vtable = (const void *const *)vtable_0026bef0;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x0010e010  Demolition (gap; file inferred)
Frees the vertex buffer and the bricks. */
void DemoWall_Shutdown(int self)
{
	if (WALL(self)->vb != NULL)
		FUN_0003f650(WALL(self)->vb);
	if (WALL(self)->bricks != NULL) {
		bkHeapFree(WALL(self)->bricks);
		WALL(self)->bricks = NULL;
	}
}

/* 0x0010e050  Demolition (gap; file inferred)
Sets the wall's position. */
void DemoWall_SetPosition(int self, float *pos)
{
	WALL(self)->pos[0] = pos[0];
	WALL(self)->pos[1] = pos[1];
	WALL(self)->pos[2] = pos[2];
	WALL(self)->pos[3] = pos[3];
	WALL(self)->dirty = 1;
	WALL(self)->posDirty = 1;
}

/* 0x0010e080  Demolition (gap; file inferred)
Draws the wall (light type 4 on while it is above the ground) and its flying bricks. */
void DemoWall_Draw(int self)
{
	DemoWall *w;
	int above;

	w = WALL(self);
	FUN_00044b00(6, 0, 0);
	FUN_000416c0(0, (struct bdTexture *)w->texture);
	FUN_00043930(w->world);
	above = 0.0f < w->pos[1] + w->height;
	DemoWall_DrawFlying(w);
	if (above)
		Wall_LightsEnable(1, 4);
	if (w->numWallVerts > 0)
		FUN_0003f4a0(4, w->vb, 0, w->numWallVerts);
	if (above)
		Wall_LightsEnable(0, 4);
}

/* write one face (6 vertices: v1, v1, v2, v3, v4, v4) at `f`; a NULL normal leaves the
   normals as they are (the flying bricks) */
static void wall_face(DemoWallVertex *f, const float *p1, const float *p2, const float *p3, const float *p4,
                      const float *normal, float u, float v, float du, float dv)
{
	const float *p[4];
	int i;

	p[0] = p1;
	p[1] = p2;
	p[2] = p3;
	p[3] = p4;
	for (i = 0; i < 4; i++) {
		f[i + 1].pos[0] = p[i][0];
		f[i + 1].pos[1] = p[i][1];
		f[i + 1].pos[2] = p[i][2];
		if (normal != NULL) {
			f[i + 1].normal[0] = normal[0];
			f[i + 1].normal[1] = normal[1];
			f[i + 1].normal[2] = normal[2];
		}
	}
	f[1].uv[0] = u;
	f[1].uv[1] = v;
	f[2].uv[0] = u;
	f[2].uv[1] = v + dv;
	f[3].uv[0] = du + u;
	f[3].uv[1] = v;
	f[4].uv[0] = du + u;
	f[4].uv[1] = v + dv;
	memcpy(&f[0], &f[1], sizeof(DemoWallVertex));
	memcpy(&f[5], &f[4], sizeof(DemoWallVertex));
}

/* a point (x, y, z) */
#define P3(a, x, y, z) (a[0] = (x), a[1] = (y), a[2] = (z))

/* 0x0010e120  Demolition (gap; file inferred)
Rebuilds the vertex buffer: with flag 1 the faces of the bricks in the wall (front always,
top/bottom/left/right where the face flag is 0), with flag 2 the six faces of every flying
brick (its corners turned by its matrix), after them. */
void DemoWall_Build(DemoWall *self, unsigned int flags)
{
	static const float nFront[3] = { 0.0f, 0.0f, 1.0f };
	static const float nTop[3] = { 0.0f, 1.0f, 0.0f };
	static const float nBottom[3] = { 0.0f, -1.0f, 0.0f };
	static const float nLeft[3] = { -1.0f, 0.0f, 0.0f };
	static const float nRight[3] = { 1.0f, 0.0f, 0.0f };
	DemoWallVertex *vb;
	DemoWallVertex *f;
	DemoBrick *b;
	float c[8][4];
	float a[3], bb[3], cc[3], d[3];
	float x0;
	float du;
	float dv;
	float x;
	float y;
	float u;
	float v;
	float hw;
	float hh;
	float hd;
	int idx;
	int row;
	int col;
	int k;

	x0 = -(self->width * 0.5f);
	du = -0.985351562f / (float)self->cols;
	dv = -0.666015625f / (float)self->rows;
	vb = (DemoWallVertex *)FUN_0003f710(self->vb, 0);
	if (vb == NULL)
		return;
	if ((flags & 1) != 0) {
		self->numWallVerts = 0;
		idx = 0;
		y = 0.0f;
		v = WALL_V0;
		f = vb;
		for (row = 0; row < self->rows; row++) {
			x = x0;
			u = WALL_U0;
			if ((row & 1) == 0) {
				x = x0 + self->halfW;
				u = du * 0.5f + WALL_U0;
			}
			for (col = 0; col < self->cols; col++, idx++) {
				b = &self->bricks[idx];
				if (b->solid != 0) {
					hw = self->halfW;
					hh = self->halfH;
					hd = self->halfD;
					P3(a, x - hw, y - hh, hd);
					P3(bb, x - hw, y + hh, hd);
					P3(cc, x + hw, y - hh, hd);
					P3(d, x + hw, y + hh, hd);
					wall_face(f, a, bb, cc, d, nFront, u, v, du, dv);
					f += 6;
					self->numWallVerts += 6;
					if (b->faceTop == 0) {
						P3(a, x - hw, y + hh, -hd);
						P3(bb, x - hw, y + hh, hd);
						P3(cc, x + hw, y + hh, -hd);
						P3(d, x + hw, y + hh, hd);
						wall_face(f, a, bb, cc, d, nTop, u, v, du, dv);
						f += 6;
						self->numWallVerts += 6;
					}
					if (b->faceBottom == 0) {
						P3(a, x - hw, y - hh, hd);
						P3(bb, x - hw, y - hh, -hd);
						P3(cc, x + hw, y - hh, hd);
						P3(d, x + hw, y - hh, -hd);
						wall_face(f, a, bb, cc, d, nBottom, u, v, du, dv);
						f += 6;
						self->numWallVerts += 6;
					}
					if (b->faceLeft == 0) {
						P3(a, x - hw, y - hh, hd);
						P3(bb, x - hw, y + hh, hd);
						P3(cc, x - hw, y - hh, -hd);
						P3(d, x - hw, y + hh, -hd);
						wall_face(f, a, bb, cc, d, nLeft, u, v, du, dv);
						f += 6;
						self->numWallVerts += 6;
					}
					if (b->faceRight == 0) {
						P3(a, x + hw, y - hh, -hd);
						P3(bb, x + hw, y + hh, -hd);
						P3(cc, x + hw, y - hh, hd);
						P3(d, x + hw, y + hh, hd);
						wall_face(f, a, bb, cc, d, nRight, u, v, du, dv);
						f += 6;
						self->numWallVerts += 6;
					}
				}
				u = u + du;
				x = x + self->brickW;
			}
			y = y + self->brickH;
			v = v + dv;
		}
	}
	if ((flags & 2) != 0 && self->numFlying > 0) {
		v = WALL_V0;
		f = vb + self->numWallVerts;
		self->numFlyingVerts = 0;
		idx = 0;
		for (row = 0; row < self->rows; row++) {
			if ((row & 1) == 0)
				u = du * 0.5f + WALL_U0;
			else
				u = WALL_U0;
			for (col = 0; col < self->cols; col++, idx++) {
				b = &self->bricks[idx];
				if (b->solid == 0 && b->flying != 0) {
					for (k = 0; k < 8; k++) {
						FUN_000505c0(c[k], b->matrix, self->corners[k]);
						c[k][0] = b->pos[0] + c[k][0];
						c[k][1] = b->pos[1] + c[k][1];
						c[k][2] = c[k][2] + b->pos[2];
					}
					/* corners: 0..3 top (-x-z, -x+z, +x-z, +x+z), 4..7 the same below; the
					   fourth face uses (5, 4, 6, 7) as in the original */
					wall_face(f, c[4], c[0], c[6], c[2], NULL, u, v, du, dv);
					wall_face(f + 6, c[7], c[3], c[5], c[1], NULL, u, v, du, dv);
					wall_face(f + 12, c[0], c[1], c[2], c[3], NULL, u, v, du, dv);
					wall_face(f + 18, c[5], c[4], c[6], c[7], NULL, u, v, du, dv);
					wall_face(f + 24, c[5], c[1], c[4], c[0], NULL, u, v, du, dv);
					wall_face(f + 30, c[6], c[2], c[7], c[3], NULL, u, v, du, dv);
					f += 36;
					self->numFlyingVerts += 36;
				}
				u = u + du;
			}
			v = v + dv;
		}
	}
	FUN_0003f790(self->vb);
}

/* 0x0010efd0  Demolition (gap; file inferred)
An explosion at `pos` (world) of `radius`: knocks out the bricks whose centre is within
2.25 * radius and sends them flying away from it (random speed and spin). */
void DemoWall_Blast(int self, float *pos, float radius)
{
	DemoWall *w;
	DemoBrick *b;
	float r;
	float d[4];
	float local[4];
	float centre[4];
	float dir[3];
	float x0;
	float x;
	float y;
	float s;
	int hit;
	int idx;
	int row;
	int col;

	w = WALL(self);
	r = radius * 2.25f;
	hit = 0;
	if (!(pos[1] - r <= w->height + w->pos[1]) || !(w->pos[1] <= r + pos[1]))
		return;
	d[0] = pos[0] - w->pos[0];
	d[1] = pos[1] - w->pos[1];
	d[2] = pos[2] - w->pos[2];
	d[3] = 0.0f; /* uninitialised in the original */
	FUN_00051170(local, d, -w->angle);
	if (!(local[2] - r <= w->depth) || !(-w->depth <= local[2] + r))
		return;
	y = 0.0f;
	x0 = -(w->width * 0.5f);
	idx = 0;
	for (row = 0; row < w->rows; row++) {
		x = x0;
		if ((row & 1) == 0)
			x = x0 + w->halfW;
		for (col = 0; col < w->cols; col++, idx++) {
			b = &w->bricks[idx];
			if (b->solid != 0 && local[0] - r <= x + w->halfW && x - w->halfW <= local[0] + r &&
			    local[1] - r <= y + w->halfH && y - w->halfH <= local[1] + r) {
				centre[0] = x;
				centre[1] = y;
				centre[2] = 0.0f;
				centre[3] = 0.0f; /* uninitialised in the original */
				if (FUN_00051500(local, centre) < r) {
					b->solid = 0;
					b->flying = 1;
					memcpy(b->pos, centre, 16);
					dir[0] = centre[0] - local[0];
					hit = 1;
					dir[1] = centre[1] - local[1];
					dir[2] = centre[2] - local[2];
					FUN_000514a0(dir, dir);
					s = (float)(int)FUN_00011730() * DD_RAND_SCALE * 10.0f + 5.0f;
					b->vel[1] = 0.0f;
					b->vel[0] = s * dir[0];
					s = (float)(int)FUN_00011730() * DD_RAND_SCALE * 80.0f + 30.0f;
					b->rot[0] = 0.0f;
					b->rot[1] = 0.0f;
					b->vel[2] = s * dir[2];
					b->rot[2] = 0.0f;
					b->spin[0] = (float)(int)FUN_00011730() * DD_RAND_SCALE * 0.209439516f - 0.104719758f;
					b->spin[1] = (float)(int)FUN_00011730() * DD_RAND_SCALE * 0.209439516f - 0.104719758f;
					b->spin[2] = (float)(int)FUN_00011730() * DD_RAND_SCALE * 0.209439516f - 0.104719758f;
					memcpy(b->matrix, DAT_002855f0, 64);
					w->numFlying = w->numFlying + 1;
				}
			}
			x = x + w->brickW;
		}
		y = y + w->brickH;
	}
	if (hit) {
		DemoWall_UpdateFaces(w);
		DemoWall_Build(w, 3);
	}
}

/* 0x0010f370  Demolition (gap; file inferred)
Moves the flying bricks (gravity, spin with damping); a brick below -15000 stops flying. */
void DemoWall_UpdateFlying(DemoWall *self, float dt)
{
	DemoBrick *b;
	float vy;
	int idx;
	int row;
	int col;

	idx = 0;
	for (row = 0; row < self->rows; row++) {
		for (col = 0; col < self->cols; col++, idx++) {
			b = &self->bricks[idx];
			if (b->solid != 0 || b->flying == 0)
				continue;
			if (-15000.0f <= b->pos[1]) {
				vy = dt * -3.0f + b->vel[1];
				b->vel[1] = vy;
				b->pos[0] = dt * b->vel[0] + b->pos[0];
				b->pos[1] = vy * dt + b->pos[1];
				b->pos[2] = dt * b->vel[2] + b->pos[2];
				b->rot[0] = b->spin[0] + b->rot[0];
				b->rot[1] = b->rot[1] + b->spin[1];
				b->rot[2] = b->rot[2] + b->spin[2];
				FUN_00050ba0(b->matrix, b->rot[1], b->rot[0], b->rot[2]);
				b->spin[0] = b->spin[0] * 0.99000001f;
				b->spin[1] = b->spin[1] * 0.99000001f;
				b->spin[2] = b->spin[2] * 0.99000001f;
			} else {
				b->flying = 0;
				self->numFlying = self->numFlying - 1;
			}
		}
	}
}

/* 0x0010f4c0  Demolition (gap; file inferred)
Per frame: the light direction, the flying bricks, and the world matrix when moved. */
void DemoWall_Update(int self, float dt)
{
	DemoWall *w;

	w = WALL(self);
	w->lightDir[0] = 1.0f;
	w->lightDir[1] = 1.5f;
	w->lightDir[2] = 0.5f;
	FUN_000514a0(w->lightDir, w->lightDir);
	if (w->numFlying > 0) {
		DemoWall_UpdateFlying(w, dt);
		DemoWall_Build(w, 2);
	}
	if (w->dirty != 0) {
		if (w->posDirty != 0)
			FUN_00050250(w->translation, w->pos[0], w->pos[1], w->pos[2]);
		if (w->angleDirty != 0)
			FUN_000507a0(w->rotation, w->angle);
		/* the original picks the multiply by the matrix's 16-byte alignment */
		if ((((unsigned int)self + 4u) & 0xfu) != 0) {
			FUN_0004ff30(w->world, w->translation, w->rotation);
			return;
		}
		FUN_00050150(w->world, w->translation, w->rotation);
	}
}

/* 0x0010f5b0  Demolition (gap; file inferred)
Puts every brick back and rebuilds the wall. */
void DemoWall_Reset(int self)
{
	DemoWall_ResetBricks(WALL(self));
	WALL(self)->numWallVerts = 0;
	WALL(self)->numFlyingVerts = 0;
	WALL(self)->numFlying = 0;
	DemoWall_Build(WALL(self), 1);
}

/* 0x0010f5e0  Demolition/CDemolition_Wall.cpp
Sets the wall up: size, bricks (rows x cols), the vertex buffer (36 vertices a brick), the
brick corners, the texture of the game; returns 0 when the buffer cannot be made. */
int DemoWall_Init(int self, float width, float height, float depth, int rows, int cols)
{
	DemoWall *w;
	float bw;
	float bh;
	int n;

	w = WALL(self);
	bw = 1.0f / (float)cols;
	w->width = width;
	w->rows = rows;
	w->cols = cols;
	w->height = height;
	w->depth = depth;
	w->depth2 = depth;
	bw = bw * width;
	w->brickW = bw;
	bh = 1.0f / (float)rows;
	n = rows * cols * 0x24;
	w->numWallVerts = n;
	bh = bh * height;
	w->brickH = bh;
	w->halfW = bw * 0.5f;
	w->halfH = bh * 0.5f;
	w->halfD = depth * 0.5f;
	if (FUN_0003f500(&w->vb, n, 0x152, 0, 1) == 0)
		return 0;
	w->pos[0] = 0.0f;
	w->pos[1] = 0.0f;
	w->pos[2] = 0.0f;
	w->angle = 0.0f;
	w->dirty = 1;
	w->posDirty = 1;
	w->angleDirty = 1;
	w->bricks = (DemoBrick *)bkHeapAlloc((uint)(rows * cols * 0x88), s_WallFile, 0x67, 0x1001);
	w->corners[0][3] = 1.0f;
	w->corners[0][0] = -w->halfW;
	w->corners[0][1] = w->halfH;
	w->corners[0][2] = -w->halfD;
	w->corners[1][3] = 1.0f;
	w->corners[1][0] = -w->halfW;
	w->corners[1][1] = w->halfH;
	w->corners[1][2] = w->halfD;
	w->corners[2][0] = w->halfW;
	w->corners[2][1] = w->halfH;
	w->corners[2][3] = 1.0f;
	w->corners[2][2] = -w->halfD;
	w->corners[3][0] = w->halfW;
	w->corners[3][1] = w->halfH;
	w->corners[3][3] = 1.0f;
	w->corners[3][2] = w->halfD;
	w->corners[4][3] = 1.0f;
	w->corners[4][0] = -w->halfW;
	w->corners[4][1] = -w->halfH;
	w->corners[4][2] = -w->halfD;
	w->corners[5][3] = 1.0f;
	w->corners[5][0] = -w->halfW;
	w->corners[5][1] = -w->halfH;
	w->corners[5][2] = w->halfD;
	w->corners[6][0] = w->halfW;
	w->corners[6][3] = 1.0f;
	w->corners[6][1] = -w->halfH;
	w->corners[6][2] = -w->halfD;
	w->corners[7][0] = w->halfW;
	w->corners[7][1] = -w->halfH;
	w->corners[7][3] = 1.0f;
	w->corners[7][2] = w->halfD;
	w->texture = DD_PTR(DD_GAME, DG_WALLTEXTURE);
	DemoWall_SetColours(w);
	DemoWall_ResetBricks(w);
	w->numWallVerts = 0;
	w->numFlyingVerts = 0;
	w->numFlying = 0;
	DemoWall_Build(w, 1);
	return 1;
}
