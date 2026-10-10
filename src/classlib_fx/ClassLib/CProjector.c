/*
ClassLib/CProjector.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\CProjector.cpp

Projector rays: a CProjector follows the "traceme" bone of an actor through its
"tracey" animation and drops points into a CProjTrail, a fading textured strip.
Spiral beams (sparkly spirals between two points) build a trail directly.

The functions 0x00082fd0-0x00083370 sit in the gap before the first tagged
CProjector function; they only use the projector lists and tables, so they are
placed here (file inferred).
*/

#include "classlib_fx.h"

static const char s_CProjectorFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\CProjector.cpp";

/* data: the ray and spiral tables (0x00290cc0-0x00290df4) and the projector lists
(0x002ed3a0-0x002ed448) are in the data blocks of classlib_fx_data.c */

/* ---------- trails: drawing (file inferred) */

/* 0x00082fd0  ClassLib (gap; file inferred: CProjector.cpp)
Draws one trail: a single beam for two points, else a strip. */
void CProjTrail_Draw(CProjTrail *self)
{
	short n;
	int count;
	float w0;
	float w1;

	if (DAT_00290dc0 != 0) {
		n = self->numPoints;
		if (1 < n) {
			if (n == 2) {
				w1 = self->widths[1];
				w0 = self->widths[0];
				CLF_DrawBeam2(self->points[0], w0, w0, self->colours[0], self->points[1], w1, w1,
				              self->colours[1], self->texture, 0, 0);
				return;
			}
			if (DAT_003055e0 != 0)
				FUN_0002c5a0();
			count = ((int (*)(int, void *, void *, void *, void *, void *, int))FUN_00040cd0)(
				(int)n, self->points, self->widths, self->extra, self->colours, self->vertices, 0);
			if (2 < count) {
				FUN_000416c0(0, self->texture);
				if (DAT_00290dbc != 0)
					FUN_0003f360(4, (int)self->vertices, count);
			}
		}
	}
}

/* render states shared by the projector draw passes */
static void CProjector_BeginDraw(void)
{
	FUN_000416c0(0, 0);
	FUN_00044b00(3, 0, 0);
	FUN_00043b70(1);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 1, 0);
	FUN_00044b00(5, 0, 0);
}

/* 0x00083090  ClassLib (gap; file inferred: CProjector.cpp)
Draws the trails of list 0x002ed430. */
void CProjTrail_DrawAll(void)
{
	CProjTrail *t;

	FUN_00044b00(4, 0, 0);
	FUN_00046d40((undefined4 *)DAT_002ed3a0);
	CProjector_BeginDraw();
	FUN_000438b0();
	for (t = CLF_ListNext(&DAT_002ed430, NULL); t != NULL; t = CLF_ListNext(&DAT_002ed430, t))
		CProjTrail_Draw(t);
	FUN_00046d60(DAT_002ed3a0);
}

/* 0x00083130  ClassLib (gap; file inferred: CProjector.cpp)
Draws the trails of list 0x002ed424 (the RefBot's). */
void CProjTrail_DrawAll2(void)
{
	CProjTrail *t;

	FUN_00046d40((undefined4 *)DAT_002ed3a0);
	CProjector_BeginDraw();
	FUN_00044b00(7, 1, 0);
	FUN_000438b0();
	for (t = CLF_ListNext(&DAT_002ed424, NULL); t != NULL; t = CLF_ListNext(&DAT_002ed424, t))
		CProjTrail_Draw(t);
	FUN_00046d60(DAT_002ed3a0);
}

/* 0x000831d0  ClassLib (gap; file inferred: CProjector.cpp)
Finds the spiral beam textures. */
void CProjector_LoadSpiralTextures(void)
{
	CProjSpiralType *s;
	const char *msg;

	s = DAT_00290dc8;
	do {
		if (s->textureName != NULL) {
			s->texture = CLF_FindTexture(0, s->textureName, 0, 0);
			if (s->texture == 0)
				msg = "couldn'r find spiral texture '%s'\n";
			else
				msg = "FOUND ray texture '%s'\n";
			CLF_Printf(msg, s->textureName);
		}
		s = s + 1;
	} while (s < DAT_00290dc8 + 2);
}

/* 0x00083220  ClassLib (gap; file inferred: CProjector.cpp)
Empties the four projector lists and finds the ray textures. */
void CProjector_Reset(void)
{
	CProjRayType *r;
	const char *msg;

	CLF_ListClear(&DAT_002ed418);
	CLF_ListClear(&DAT_002ed43c);
	CLF_ListClear(&DAT_002ed430);
	CLF_ListClear(&DAT_002ed424);
	r = DAT_00290cc8;
	do {
		if (r->textureName != NULL) {
			r->texture = CLF_FindTexture(0, r->textureName, 0, 0);
			if (r->texture == 0)
				msg = "couldn'r find ray texture '%s'\n";
			else
				msg = "FOUND ray texture '%s'\n";
			CLF_Printf(msg, r->textureName);
		}
		r = r + 1;
	} while (r < DAT_00290cc8 + 3);
}

/* 0x000832a0  ClassLib (gap; file inferred: CProjector.cpp)
Draws the beams of the normal projectors, then the normal trails. */
void CProjector_DrawAll(void)
{
	CProjector *p;
	CProjRayType *type;
	float w;

	FUN_00046d40((undefined4 *)DAT_002ed3a0);
	CProjector_BeginDraw();
	FUN_000438b0();
	for (p = CLF_ListNext(&DAT_002ed418, NULL); p != NULL; p = CLF_ListNext(&DAT_002ed418, p)) {
		if (DAT_00290db8 != 0) {
			type = p->type;
			w = p->width * 5.0f;
			CLF_DrawBeam(p->source, w, w, type->beamColour0, p->target, w, w, type->beamColour1,
			             type->texture, 0, 0);
		}
	}
	CProjTrail_DrawAll();
	FUN_00046d60(DAT_002ed3a0);
}

/* 0x00083370  ClassLib (gap; file inferred: CProjector.cpp)
Draws the RefBot, then the beams of its projectors. */
void CProjector_DrawRefBotAll(void)
{
	CProjector *p;
	CProjRayType *type;
	float w;

	FUN_00046d40((undefined4 *)DAT_002ed3a0);
	CProjector_BeginDraw();
	FUN_000438b0();
	FUN_00084990();
	FUN_000438b0();
	for (p = CLF_ListNext(&DAT_002ed43c, NULL); p != NULL; p = CLF_ListNext(&DAT_002ed43c, p)) {
		if (DAT_00290db8 != 0) {
			type = p->type;
			w = p->width * 5.0f;
			CLF_DrawBeam(p->source, w, w, type->beamColour0, p->target, w, w, type->beamColour1,
			             type->texture, 0, 0);
		}
	}
	FUN_00046d60(DAT_002ed3a0);
}

/* ---------- trails */

/* 0x00083440  ClassLib/CProjector.cpp
Constructor: a trail of up to `count` points, linked into list 0x002ed424 (second != 0)
or 0x002ed430. */
CProjTrail *CProjTrail_Init(CProjTrail *self, int texture, int count, int *headColour,
                            int *tailColour, float headWidth, float tailWidth, int second)
{
	self->texture = texture;
	self->headColour[0] = headColour[0];
	self->headColour[1] = headColour[1];
	self->headColour[2] = headColour[2];
	self->headColour[3] = headColour[3];
	self->tailColour[0] = tailColour[0];
	self->tailColour[1] = tailColour[1];
	self->tailColour[2] = tailColour[2];
	self->tailColour[3] = tailColour[3];
	/* the count is converted as unsigned */
	self->fadeRate = 127.0f / (float)(uint)count;
	self->points = CLF_Alloc(count * 0x10, s_CProjectorFile, 0x277, 0x2001);
	self->colours = CLF_Alloc(count * 0x10 + 4, s_CProjectorFile, 0x279, 0x2001);
	self->widths = CLF_Alloc(count * 4 + 4, s_CProjectorFile, 0x27a, 0x2001);
	self->extra = CLF_Alloc(count * 4 + 4, s_CProjectorFile, 0x27b, 0x2001);
	self->tailWidth = tailWidth;
	self->headWidth = headWidth;
	self->maxPoints = (short)count;
	self->dirty = 0;
	self->numPoints = 0;
	self->numDrawn = 0;
	self->dying = 0;
	if (second != 0) {
		self->list = &DAT_002ed424;
		CLF_ListInsert(&DAT_002ed424, self, 0);
		return self;
	}
	self->list = &DAT_002ed430;
	CLF_ListInsert(&DAT_002ed430, self, 0);
	return self;
}

/* 0x00083580  ClassLib/CProjector.cpp
Adds a point at the head of the trail (when it is at least 0.1 from the current head). */
void CProjTrail_AddPoint(CProjTrail *self, float *p)
{
	float *head;
	int i;
	int n;

	if (self->numPoints >= 1) {
		head = self->points[0];
		if ((p[0] - head[0]) * (p[0] - head[0]) + (p[1] - head[1]) * (p[1] - head[1]) +
		        (p[2] - head[2]) * (p[2] - head[2]) < 0.01f)
			return;
	}
	if (self->maxPoints - 1 <= (int)self->numPoints)
		self->numPoints = self->maxPoints - 2;
	for (i = self->numPoints - 1; i >= 0; i--) {
		self->points[i + 1][0] = self->points[i][0];
		self->points[i + 1][1] = self->points[i][1];
		self->points[i + 1][2] = self->points[i][2];
		self->points[i + 1][3] = self->points[i][3];
	}
	self->points[0][0] = p[0];
	self->points[0][1] = p[1];
	self->points[0][2] = p[2];
	self->points[0][3] = p[3];
	self->points[0][3] = 1.0f;
	n = self->numPoints;
	self->colours[n][0] = 0;
	self->colours[n][1] = 0;
	self->colours[n][2] = 0;
	self->colours[n][3] = 0;
	self->widths[n] = 0.0f;
	self->extra[n] = 0.0f;
	self->numPoints = self->numPoints + 1;
	self->dirty = 1;
	self->fade = (float)self->headColour[3];
}

/* 0x000836b0  ClassLib/CProjector.cpp
A spiral beam from `from` to `to`: builds a dying trail around the line, `turns` turns
of `radius`, and drops sparkles every `sparkleDist`.
flags: 1 radius pulses and rises by `rise`, 2 radius grows along the line, 4 always
180 points (else 18 per 100 units, 10..180). */
void CProjector_SpiralBeam(float *from, float *to, float radius, float rise, int turns,
                           float sparkleDist, CProjSpiralType *spiral, float sparkleRate,
                           uint flags, uint colour)
{
	float len;
	float fn;
	float t;
	float f;
	float r;
	float angle;
	float angleStep;
	float dist;
	float stepLen;
	float inv;
	float p[4];          /* local_88: the point (and the rotated offset) */
	float axis[4];       /* local_54: direction + angle */
	float perp[4];       /* local_64 */
	int colours[8];      /* local_3c: tail colour [0..3], head colour [4..7] */
	float centre[3];     /* local_1c */
	CProjTrail *trail;
	void *mem;
	int n;
	int i;

	colours[4] = 0x7f;
	colours[7] = 0x7f;
	p[0] = to[0] - from[0];
	dist = 0.0f;
	colours[5] = 0x5a;
	colours[6] = 0x32;
	colours[0] = 0x3c;
	p[1] = to[1] - from[1];
	colours[1] = 0;
	colours[2] = 0;
	colours[3] = 10;
	p[3] = 1.0f;
	p[2] = to[2] - from[2];
	len = sqrtf(p[1] * p[1] + p[0] * p[0] + p[2] * p[2]);
	if (len == 0.0f)
		return;

	if ((flags & 4) == 0) {
		n = (CLF_FTOL(len) * 0xb4) / 1000;
		if (n < 0xb5) {
			if (n < 10)
				n = 10;
		} else {
			n = 0xb4;
		}
	} else {
		n = 0xb4;
	}
	mem = CLF_Alloc(sizeof(CProjTrail), s_CProjectorFile, 0x500, 0x1001);
	if (mem == NULL)
		trail = NULL;
	else
		trail = CProjTrail_Init(mem, spiral->texture, n + 1, colours + 4, colours,
		                        DAT_00290df4, DAT_00290df4, 0);
	trail->dying = 1;
	r = 0.0f;
	angle = 0.0f;
	trail->fadeRate = trail->fadeRate + trail->fadeRate;
	fn = (float)n;
	angleStep = ((float)turns * 6.2831850f) / fn;
	inv = 1.0f / len;
	axis[0] = p[0] * inv;
	axis[1] = p[1] * inv;
	axis[2] = p[2] * inv;
	perp[0] = axis[2] * DAT_0028fdc8[1] - axis[1] * DAT_0028fdc8[2];
	perp[1] = axis[0] * DAT_0028fdc8[2] - axis[2] * DAT_0028fdc8[0];
	perp[2] = axis[1] * DAT_0028fdc8[0] - axis[0] * DAT_0028fdc8[1];
	f = sqrtf(perp[0] * perp[0] + perp[1] * perp[1] + perp[2] * perp[2]);
	if (f == 0.0f) {
		perp[0] = DAT_0028fdb8[0];
		perp[1] = DAT_0028fdb8[1];
		perp[2] = DAT_0028fdb8[2];
		perp[3] = DAT_0028fdb8[3];
	} else {
		f = 1.0f / f;
		perp[0] = perp[0] * f;
		perp[1] = perp[1] * f;
		perp[2] = perp[2] * f;
	}
	stepLen = len / fn;
	for (i = 0; i < n; i++) {
		t = (float)i / fn;
		FUN_00050ff0(centre, from, 1.0f - t, to, t);
		if ((flags & 2) == 0) {
			if ((flags & 1) == 0) {
				r = radius;
			} else {
				f = (1.0f - ((float)i + (float)i) / fn) * radius;
				if (0.0f < f)
					f = f * -1.0f;
				r = (r * 4.0f + f + radius) * 0.2f;
			}
		} else {
			r = t * radius;
		}
		axis[3] = FUN_000703a0(angle);
		FUN_000707c0(p, perp, axis);
		p[0] = centre[0] + p[0] * r;
		p[1] = centre[1] + p[1] * r;
		p[2] = centre[2] + p[2] * r;
		if ((flags & 1) != 0)
			p[1] = (r / radius) * rise + p[1];
		p[3] = 1.0f;
		CProjTrail_AddPoint(trail, p);
		dist = stepLen + dist;
		if (sparkleDist < dist) {
			CLF_EmitSparkles(spiral->sparkles, p, DAT_002ea478, (CLF_FTOL(sparkleRate) * i) / n,
			                 colour);
			dist = dist - sparkleDist;
		}
		angle = angleStep + angle;
	}
}

/* ---------- projectors */

/* 0x00083b60  ClassLib/CProjector.cpp
Deletes a projector (its trail fades out on its own). Returns the previous list node. */
void *CProjector_Delete(CProjector *self)
{
	void *prev;

	prev = CLF_ListRemove(self->list, self);
	self->trail->dying = 1;
	FUN_00084930();
	CLF_Free(self);
	return prev;
}

/* 0x00083b90  ClassLib/CProjector.cpp
Constructor: traces `node` (default: the "traceme" bone) of `actor` through its "tracey"
animation, starting `frameOffset` frames in. */
CProjector *CProjector_Init(CProjector *self, int actor, int node, CProjRayType *type,
                            float *source, float life, int speed, int frameOffset,
                            int refbot, float width, int numPoints)
{
	int *anim;
	void *mem;
	CProjTrail *trail;

	self->source[0] = source[0];
	self->source[1] = source[1];
	self->source[2] = source[2];
	self->source[3] = source[3];
	self->target[0] = 0.0f;
	self->target[1] = 0.0f;
	self->target[2] = 0.0f;
	self->target[3] = 1.0f;
	self->type = type;
	self->actor = actor;
	if (node == 0)
		self->node = CLF_FindNode(*(int *)(actor + 0x134), DAT_00290cc4);
	else
		self->node = node;
	anim = CLF_FindAnim(self->actor, DAT_00290cc0);
	self->anim = anim;
	if (self->node == 0 || anim == NULL) {
		CLF_Printf("tile is wrong!\n");
		CLF_Free(self);
	} else {
		self->speed = speed;
		self->frame = anim[1] + frameOffset;
		CLF_SetFrame(self->actor, self->node, anim[1] + frameOffset, 0);
		CLF_UpdateActor(self->actor, 1);
		CLF_NodePosition(self->actor, self->node, self->target);
		self->life = life;
		if (refbot == 0)
			self->list = &DAT_002ed418;
		else
			self->list = &DAT_002ed43c;
		CLF_ListInsert(self->list, self, 0);
		self->width = width;
		mem = CLF_Alloc(sizeof(CProjTrail), s_CProjectorFile, 0x1a4, 0x1001);
		if (mem == NULL)
			trail = NULL;
		else
			trail = CProjTrail_Init(mem, self->type->texture, numPoints, self->type->trailHead,
			                        self->type->trailTail, width * 4.0f, width * 4.0f, refbot);
		self->trail = trail;
		if (trail == NULL)
			CLF_Printf("ERROR trail wasn't created!\n");
		self->step = 0x200;
	}
	return self;
}

/* moves the traced bone to `frame` and adds its position to the trail */
static void CProjector_TraceFrame(CProjector *self, int frame)
{
	CLF_SetFrame(self->actor, self->node, frame, 0);
	CLF_UpdateActor(self->actor, 1);
	CLF_NodePosition(self->actor, self->node, self->target);
	self->target[3] = 1.0f;
	CProjTrail_AddPoint(self->trail, self->target);
}

/* 0x00083d60  ClassLib/CProjector.cpp
Advances the animation, adding trail points every `step` frames. Deletes the projector
when its life ends (or a play-once animation ends). Returns the node to continue the
list walk from. */
void *CProjector_Update(CProjector *self)
{
	int frame;
	int advance;
	int last;
	int sub;
	int count;
	int f;

	frame = self->frame;
	advance = CLF_FTOL((float)self->speed * DAT_0028fda8);
	last = self->anim[2];
	if (last < frame)
		return CProjector_Delete(self);
	if ((self->type->flags & 1) != 0 && last < frame + advance)
		advance = last - frame;
	if (self->step < advance) {
		count = advance / self->step;
		if (1 < count + 1) {
			sub = advance / (count + 1);
			do {
				last = self->anim[2];
				frame = frame + sub;
				f = frame;
				if (last < frame) {
					if ((self->type->flags & 1) == 0) {
						f = frame + (self->anim[1] - last);
						CProjector_TraceFrame(self, f);
						frame = f;
					} else {
						f = last;
						if (frame - sub < last) {
							CProjector_TraceFrame(self, f);
							frame = f;
						}
					}
				} else {
					CProjector_TraceFrame(self, f);
					frame = f;
				}
				count = count - 1;
			} while (count != 0);
		}
	}
	advance = self->frame + advance;
	self->frame = advance;
	last = self->anim[2];
	if (last < advance) {
		if ((self->type->flags & 1) != 0)
			return CProjector_Delete(self);
		self->frame = (self->anim[1] - last) + advance;
	}
	CProjector_TraceFrame(self, self->frame);
	self->life = self->life - DAT_0028fda8;
	if (0.0f <= self->life &&
	    ((self->type->flags & 1) == 0 || self->frame < self->anim[2]))
		return self;
	return CProjector_Delete(self);
}

/* 0x00083f40  ClassLib/CProjector.cpp
Deletes a trail and its arrays. Returns the previous list node. */
void *CProjTrail_Delete(CProjTrail *self)
{
	void *prev;

	prev = CLF_ListRemove(self->list, self);
	CLF_Free(self->points);
	self->points = NULL;
	CLF_Free(self->colours);
	self->colours = NULL;
	CLF_Free(self->widths);
	self->widths = NULL;
	CLF_Free(self->extra);
	self->extra = NULL;
	CLF_Free(self);
	return prev;
}

/* 0x00083fa0  ClassLib/CProjector.cpp
Fades the trail (drops its last point every frame without a new point) and recomputes
the colours and widths of its points. Deletes a dying trail when it is empty. Returns the
node to continue the list walk from. */
void *CProjTrail_Update(CProjTrail *self)
{
	short n;
	int i;
	int t;
	int inv;
	int a;
	int head[4];
	int tail[4];
	int c;

	if (DAT_00290dc4 != 0) {
		n = self->numPoints;
		if (n == 0) {
			if (self->dying == 1)
				return CProjTrail_Delete(self);
		} else {
			if (self->dirty == 0 && 0 < n) {
				self->numPoints = n - 1;
				self->fade = self->fade - DAT_0028fda8 * self->fadeRate;
				if (self->fade <= 0.0f) {
					if (self->dying != 1) {
						self->numPoints = 0;
						self->numDrawn = 0;
						self->dirty = 0;
						return self;
					}
					return CProjTrail_Delete(self);
				}
			}
			for (i = 0; i < self->numPoints; i++) {
				t = (i * 1000) / (int)self->numPoints;
				inv = 1000 - t;
				a = CLF_FTOL(self->fade);
				for (c = 0; c < 4; c++) {
					head[c] = (self->headColour[c] * a) / 128;
					tail[c] = (self->tailColour[c] * a) / 128;
					if (head[c] < tail[c])
						tail[c] = head[c];
				}
				for (c = 0; c < 4; c++)
					self->colours[i][c] = (head[c] * inv + tail[c] * t) / 1000;
				self->widths[i] = ((float)t * self->tailWidth + (float)inv * self->headWidth) * 0.001f;
			}
			self->numDrawn = self->numPoints;
			self->dirty = 0;
		}
	}
	return self;
}

/* 0x00084230  ClassLib/CProjector.cpp
Updates every trail. */
void CProjTrail_UpdateAll(void)
{
	void *t;

	t = CLF_ListNext(&DAT_002ed430, NULL);
	while (t != NULL)
		t = CLF_ListNext(&DAT_002ed430, CProjTrail_Update(t));
	t = CLF_ListNext(&DAT_002ed424, NULL);
	while (t != NULL)
		t = CLF_ListNext(&DAT_002ed424, CProjTrail_Update(t));
}

/* 0x00084280  ClassLib/CProjector.cpp
Deletes every trail (CProjTrail_Delete inlined). */
void CProjTrail_DeleteAll(void)
{
	CLFList *lists[2];
	CProjTrail *t;
	void *prev;
	int k;

	lists[0] = &DAT_002ed430;
	lists[1] = &DAT_002ed424;
	for (k = 0; k < 2; k++) {
		t = CLF_ListNext(lists[k], NULL);
		while (t != NULL) {
			prev = CLF_ListRemove(t->list, t);
			CLF_Free(t->points);
			t->points = NULL;
			CLF_Free(t->colours);
			t->colours = NULL;
			CLF_Free(t->widths);
			t->widths = NULL;
			CLF_Free(t->extra);
			t->extra = NULL;
			CLF_Free(t);
			t = CLF_ListNext(lists[k], prev);
		}
	}
}

/* player colour (0xRRGGBB) for the spiral effects: grey-yellow for no player */
static uint CProjector_PlayerColour(int player)
{
	int o;

	if (player < 0)
		return 0x7f7f5a;
	o = player * 0x60;
	return ((uint)DAT_00290718[o] << 16) | ((uint)DAT_00290718[o + 1] << 8) |
	       (uint)DAT_00290718[o + 2];
}

/* 0x00084360  ClassLib/CProjector.cpp
A spiral beam rising 300 units from `pos`, coloured for `player` (-1: none), with a
burst of streaks. */
void CProjector_SpiralAt(float *pos, int player)
{
	uint colour;
	float to[4];
	float from[4];

	colour = CProjector_PlayerColour(player);
	from[0] = pos[0];
	from[1] = pos[1];
	to[1] = DAT_00290de0 + pos[1];
	from[2] = pos[2];
	from[3] = pos[3];
	to[0] = pos[0];
	to[2] = pos[2];
	to[3] = pos[3];
	CProjector_SpiralBeam(from, to, DAT_00290de4, 0.0f, DAT_00290de8, DAT_00290df0,
	                      &DAT_00290dc8[1], DAT_00290dec, 6, colour);
	/* a random angle is computed and not used */
	FUN_000703a0((float)CLF_Rand() * 0.00019175345f + 3.1415925f);
	CLF_StreakBurst(DAT_00291708, 0x23, pos, 3.0f, 12.0f, 6.0f, 0, -1);
	CLF_StreakRing(DAT_00291708, 0x23, 60.0f, pos, 24.0f, 16.0f, 0);
}

/* 0x000844a0  ClassLib/CProjector.cpp
CProjector_SpiralAt with the height, radius, turns and sparkle distance given. */
void CProjector_SpiralAtEx(float *pos, float radius, float height, int turns,
                           float sparkleDist, int player)
{
	uint colour;
	float to[4];
	float from[4];

	colour = CProjector_PlayerColour(player);
	from[0] = pos[0];
	from[1] = pos[1];
	to[1] = pos[1] + height;
	from[2] = pos[2];
	from[3] = pos[3];
	to[0] = pos[0];
	to[2] = pos[2];
	to[3] = pos[3];
	CProjector_SpiralBeam(from, to, radius, 0.0f, turns, sparkleDist, &DAT_00290dc8[1],
	                      DAT_00290dec, 6, colour);
	FUN_000703a0((float)CLF_Rand() * 0.00019175345f + 3.1415925f);
	CLF_StreakBurst(DAT_00291708, 0x23, pos, 3.0f, 12.0f, 6.0f, 0, -1);
	CLF_StreakRing(DAT_00291708, 0x23, 60.0f, pos, 24.0f, 16.0f, 0);
}

/* 0x000845d0  ClassLib/CProjector.cpp
Updates every projector, then every trail. */
void CProjector_UpdateAll(void)
{
	void *p;

	p = CLF_ListNext(&DAT_002ed418, NULL);
	while (p != NULL)
		p = CLF_ListNext(&DAT_002ed418, CProjector_Update(p));
	p = CLF_ListNext(&DAT_002ed43c, NULL);
	while (p != NULL)
		p = CLF_ListNext(&DAT_002ed43c, CProjector_Update(p));
	CProjTrail_UpdateAll();
}

/* 0x00084620  ClassLib/CProjector.cpp
Deletes every projector (CProjector_Delete inlined), then every trail. */
void CProjector_DeleteAll(void)
{
	CLFList *lists[2];
	CProjector *p;
	void *prev;
	int k;

	lists[0] = &DAT_002ed418;
	lists[1] = &DAT_002ed43c;
	for (k = 0; k < 2; k++) {
		p = CLF_ListNext(lists[k], NULL);
		while (p != NULL) {
			prev = CLF_ListRemove(p->list, p);
			p->trail->dying = 1;
			FUN_00084930();
			CLF_Free(p);
			p = CLF_ListNext(lists[k], prev);
		}
	}
	CProjTrail_DeleteAll();
}

/* 0x000846c0  ClassLib/CProjector.cpp
Creates a projector for each "traceme01", "traceme02"... bone of `actor`. With a RefBot
the beams start at its laser bone (and it plays the beam sound), else at `source`. */
void CProjector_CreateForActor(int actor, CProjRayType *type, float *source)
{
	char name[64];
	float laser[4];
	void *mem;
	int node;
	int i;

	i = 1;
	sprintf(name, "%s%02d", DAT_00290cc4, 1);
	node = CLF_FindNode(*(int *)(actor + 0x134), name);
	if (node == 0) {
		CLF_Printf("this has no projector bones present\n!");
		return;
	}
	do {
		if (DAT_002ed448 == NULL) {
			mem = CLF_Alloc(sizeof(CProjector), s_CProjectorFile, 0x158, 0x1001);
			if (mem != NULL)
				CProjector_Init(mem, actor, node, type, source, DAT_00290dac,
				                CLF_FTOL(DAT_00290db0), 0, 1, DAT_00290db4, 100);
		} else {
			CLF_NodePosition(DAT_002ed448->actor, DAT_002ed448->laserNode, laser);
			mem = CLF_Alloc(sizeof(CProjector), s_CProjectorFile, 0x154, 0x1001);
			if (mem != NULL)
				CProjector_Init(mem, actor, node, type, laser, DAT_00290dac,
				                CLF_FTOL(DAT_00290db0), 0, 1, DAT_00290db4, 100);
			FUN_00084900();
		}
		i = i + 1;
		sprintf(name, "%s%02d", DAT_00290cc4, i);
		node = CLF_FindNode(*(int *)(actor + 0x134), name);
	} while (node != 0);
}
