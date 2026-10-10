/*
ClassLib/Debris.c  <-  C:\BlitzParty\Game\Src\ClassLib\Debris.cpp   (module bumpercars_a)

The "debris" extra system (bactor extras): small sprites or streaks that fly, fall, bounce
on the ground the game provides, blow in the wind and change colour and size over three
stages. Every minigame uses it; the code sits in the BumperCars address range of the
function table, so it belongs to this module.

The file name comes from the assert path in Debris_InitItem (0x00080f40). The callbacks
0x00080bf0, 0x00080c50, 0x00080f40, 0x000812b0 and 0x00082010 are missing from the function
table (they are only passed to baCreateExtraSystem); this module defines them under their
FUN_ names. 0x00080bf0 and 0x00080c50 lie in classlib_ui's address range.
*/

#include "bumpercars_a.h"

static const char s_DebrisFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\Debris.cpp";

/* classlib_fx: the debris vertex buffer and the sprite helpers (prototypes as declared by
classlib_fx.decls; the struct types are opaque here) */
struct CSpriteVB;
struct CSpriteVBBatch;
struct CSprite;
struct CSpriteVBBatch *SpriteVB_GetBatch(struct CSpriteVB *self, int key);
struct CSprite *Sprite_Create(struct CSprite *self, float *pos, struct CSpriteAnim *anim, int user);
void Sprite_Update(struct CSprite *self, float dt);
int DebrisSys_InitWind(int self);

#define DEBRIS_VB ((struct CSpriteVB *)(void *)DAT_00290ea8)

/* the bytes of a CSpriteVBBatch this file touches */
#define BATCH_WRITE(batch) BCA_AT((void *)(uintptr_t)(batch), 0x0c, unsigned char *)
#define BATCH_START(batch) BCA_AT((void *)(uintptr_t)(batch), 0x10, unsigned char *)
#define BATCH_NUM_VERTICES(batch) BCA_I32((void *)(uintptr_t)(batch), 0x1c)

/* the sprite inside a piece (a CSprite of classlib_fx at piece + 0xa4) */
#define PIECE_SPRITE_POS(p) ((float *)&(p)->sprite[0x10])     /* 0xb4 */
#define PIECE_SPRITE_SIZE(p) BCA_F32((p)->sprite, 0x24)        /* 0xc8 */
#define PIECE_SPRITE_FLIP_U(p) BCA_U8((p)->sprite, 0x22)       /* 0xc6 */
#define PIECE_SPRITE_FLIP_V(p) BCA_U8((p)->sprite, 0x23)       /* 0xc7 */
#define PIECE_SPRITE_RGBA(p) (&(p)->sprite[0x28])              /* 0xcc r, g, b, a */
#define PIECE_SPRITE_ANGLE(p) BCA_F32((p)->sprite, 0x2c)       /* 0xd0 */
#define PIECE_SPRITE_SPIN(p) BCA_F32((p)->sprite, 0x30)        /* 0xd4 */

/* 0x00080bf0  ClassLib/Debris.cpp (missing from the table): the delete callback.
Returns 0 (keep the piece) while debris is off, or for a piece flagged 0x8000 unless
DAT_002ed348 is set. */
int FUN_00080bf0(struct BExtraSystem *system, struct DebrisPiece *piece)
{
	(void)system;
	if (DAT_002ed340 != 0)
		return 0;
	if ((piece->flags & 0x8000) != 0 && DAT_002ed348 == 0)
		return 0;
	if (piece->batch != 0)
		BCA_I32((void *)(uintptr_t)piece->batch, 0x14)--;
	DAT_002ed344--;
	if (piece->spin != NULL) {
		BCA_DELETE(piece->spin);
		piece->spin = NULL;
	}
	return 1;
}

/* 0x00080c50  ClassLib/Debris.cpp (missing from the table): the system callback
(reason 0: created, 1: deleted) */
int FUN_00080c50(struct BExtraSystem *system, int reason)
{
	struct DebrisSysData *data;

	if (DAT_002ed340 != 0)
		return 0;
	data = (struct DebrisSysData *)system->userData;
	BCA_Printf("DebrisExtraSystemInitCallback(..)\n");
	BCA_Printf("reason = %i \neSystem->sysData =%i\n\n", reason, (int)(uintptr_t)system->userData);
	if (reason == 0) {
		data->time = 1.0f;
		data->wind = NULL;
	} else if (reason == 1) {
		if (data->wind != NULL) {
			BCA_DELETE(data->wind);
			data->wind = NULL;
		}
		DAT_002ed34c = NULL;
	}
	return 0;
}

/* 0x00080d00  (gap, ClassLib/Debris.cpp by address): updates the debris and actor debris
systems; dt is the frame time (the update callbacks get its bits as their argument) */
void Debris_UpdateSystems(float dt)
{
	baUpdateExtras(DAT_002ed34c, (void *)(uintptr_t)bca_fbits(dt));
	baUpdateExtras((struct BExtraSystem *)(uintptr_t)DAT_002ed44c, (void *)(uintptr_t)bca_fbits(dt));
}

/* 0x00080d30  (gap): stops a piece: it dies at the end of this stage */
void Debris_StopPiece(struct DebrisPiece *piece)
{
	if (piece != NULL) {
		piece->time = 0.0f;
		piece->stage = 3;
		piece->flags &= 0xffff7fffu;
	}
}

/* 0x00080d60  (gap): adds a piece of debris of a type at pos with velocity vel, living
lifeFrames frames (0: the type's stage times). colour is 0x00rrggbb (0-0x7f each) or
0xffffffff; with DEBRIS_RANDOM_RGB the type's stage-1 colour is varied at random by its
stage-0 colour values. Returns the new piece, or NULL. */
void *Debris_Add(struct DebrisType *type, float *pos, float *vel, int lifeFrames, unsigned int colour)
{
	int red, green, blue, range;
	unsigned int r, g, b;

	if (DAT_002ed340 != 0)
		return NULL;
	if (type->actorDebris != 0)
		return baAddExtraItem((struct BExtraSystem *)(uintptr_t)DAT_002ed44c, type, pos, vel,
			(double)((float)lifeFrames * (1.0f / 60.0f)), colour);
	if ((type->flags & DEBRIS_RANDOM_RGB) != 0) {
		if (colour == 0xffffffffu) {
			blue = type->params->blue[1];
			red = type->params->red[1];
			green = type->params->green[1];
		} else {
			red = (int)(colour >> 16 & 0xff);
			green = (int)(colour >> 8 & 0xff);
			blue = (int)(colour & 0xff);
		}
		range = type->params->red[0];
		red += bca_rand() % (range + 1) - range / 2;
		range = type->params->green[0];
		green += bca_rand() % (range + 1) - range / 2;
		range = type->params->blue[0];
		blue += bca_rand() % (range + 1) - range / 2;
		if (red > 0x7f)
			red = 0x7f;
		if (green > 0x7f)
			green = 0x7f;
		if (blue > 0x7f)
			blue = 0x7f;
		r = red < 0 ? 0 : (unsigned int)red & 0xff;
		g = green < 0 ? 0 : (unsigned int)green & 0xff;
		b = blue < 0 ? 0 : (unsigned int)blue & 0xff;
		colour = (r * 0x100 + g) * 0x100 + b;
	}
	return baAddExtraItem(DAT_002ed34c, type, pos, vel, (double)((float)lifeFrames * (1.0f / 60.0f)),
		colour);
}

/* 0x00080f40  ClassLib/Debris.cpp (missing from the table): the item creation callback.
args: baAddExtraItem's arguments (type, pos, vel, double life in seconds, colour). */
int FUN_00080f40(struct BExtraSystem *system, struct DebrisPiece *piece, void *args)
{
	va_list ap;
	struct DebrisType *type;
	float *pos, *vel, *spinInit, size;
	double life;
	unsigned int colour;
	struct DebrisParams *params;
	struct DebrisSpin *spin;
	float start[4], hit[4], normal[4], fraction;

	if (DAT_002ed340 != 0)
		return 0;
	DAT_002ed344++;
	if (args == NULL)
		return 0;
	ap = (va_list)args;
	type = va_arg(ap, struct DebrisType *);
	pos = va_arg(ap, float *);
	vel = va_arg(ap, float *);
	life = va_arg(ap, double);
	colour = va_arg(ap, unsigned int);

	spinInit = type->spinInit;
	piece->stage = 0;
	piece->params = type->params;
	piece->system = system;
	piece->type = type;
	piece->stageTime = (float)life;
	if (life == (double)0.0f) {
		piece->time = piece->params->life[0];
		piece->stageTime = piece->params->life[0];
		piece->batch = (int)(uintptr_t)SpriteVB_GetBatch(DEBRIS_VB, (int)(uintptr_t)type);
	} else {
		piece->time = (float)life;
		piece->stage = -1;
		piece->batch = 0;
	}
	piece->age = 0.0f;
	piece->colour = colour;
	piece->pos[0] = pos[0];
	piece->pos[1] = pos[1];
	piece->pos[2] = pos[2];
	piece->pos[3] = 1.0f;
	piece->drawPos[0] = pos[0];
	piece->drawPos[1] = pos[1];
	piece->drawPos[2] = pos[2];
	piece->drawPos[3] = 1.0f;
	piece->vel[0] = vel[0];
	piece->vel[1] = vel[1];
	piece->vel[2] = vel[2];
	piece->vel[3] = 1.0f;
	piece->flags = type->flags;
	piece->scale = 1.0f;
	piece->offset[3] = 1.0f;
	piece->offset[0] = 0.0f;
	piece->offset[1] = 0.0f;
	piece->offset[2] = 0.0f;
	size = piece->params->size[0];
	piece->unknown60 = 1.0f;
	piece->size = size;
	piece->unknown64 = 0.0f;
	piece->unknown54[0] = 0.0f;
	piece->unknown54[1] = 0.0f;
	piece->unknown54[2] = 0.0f;
	Sprite_Create((struct CSprite *)piece->sprite, piece->pos, (struct CSpriteAnim *)type->spriteAnim,
		(int)bca_fbits(size));
	params = piece->params;
	PIECE_SPRITE_ANGLE(piece) = 0.0f;
	PIECE_SPRITE_SPIN(piece) = (float)bca_rand() * params->spinRandom * (1.0f / 32767.0f) + params->spin;
	PIECE_SPRITE_RGBA(piece)[3] = (unsigned char)params->alpha[0];
	PIECE_SPRITE_RGBA(piece)[0] = (unsigned char)params->red[0];
	PIECE_SPRITE_RGBA(piece)[1] = (unsigned char)params->green[0];
	PIECE_SPRITE_RGBA(piece)[2] = (unsigned char)params->blue[0];
	piece->size = params->size[0];
	if (spinInit == NULL) {
		piece->spin = NULL;
	} else {
		spin = (struct DebrisSpin *)BCA_NEW(0x30, s_DebrisFile, 0x222, 0x2001);
		piece->spin = spin;
		spin->axisAngle[0] = spinInit[0];
		spin->axisAngle[1] = spinInit[1];
		spin->axisAngle[2] = spinInit[2];
		spin->axisAngle[3] = spinInit[3];
		spin->angleRate = spinInit[4];
		spin->radius = spinInit[5];
		spin->radiusRate = spinInit[6];
		memcpy(&spin->flags, &spinInit[7], 4);
		CLVec3_Perpendicular(spin->perp, spin->axisAngle);
	}
	if ((type->flags & DEBRIS_GROUND) != 0 && DAT_002ed33c != NULL) {
		start[0] = 0.0f;
		start[1] = -1000.0f;
		start[2] = 0.0f;
		start[3] = 1.0f;
		fraction = DAT_002ed33c(piece->pos, start, hit, normal);
		if (fraction > 0.0f) {
			piece->groundY = hit[1];
			piece->groundBounce = normal[3];
			return 0;
		}
		piece->groundY = -1000.0f;
		piece->groundBounce = 1.0f;
	}
	return 0;
}

/* 0x00081210  (gap): a dying piece spawns the type's death debris */
void Debris_SpawnDeath(struct DebrisPiece *piece)
{
	struct DebrisType *onDeath;
	unsigned int colour;

	if (DAT_002ed340 != 0)
		return;
	onDeath = piece->type->onDeath;
	if (onDeath == NULL || piece->type->spinInit != NULL)
		return;
	colour = (piece->flags & 0x10000) != 0 ? piece->colour : 0xffffffffu;
	if ((piece->flags & 4) != 0)
		Debris_Add(onDeath, piece->pos, DAT_002ea478, 0, colour);
	else
		Debris_Add(onDeath, piece->pos, piece->vel, 0, colour);
}

/* 0x000812b0  ClassLib/Debris.cpp (missing from the table): the draw callback. Writes each
piece as a sprite quad (6 vertices) or a streak (10 vertices) into its batch of the debris
vertex buffer, with a copy of the first and last vertex to join the strips. */
int FUN_000812b0(struct BExtraSystem *system, int count, struct DebrisPiece **items, void *arg)
{
	struct DebrisPiece *piece;
	struct DebrisStreak *streak;
	unsigned char *vertex;
	float end[4], length, width, speed;
	int colour0[4], colour1[4];
	unsigned int r, g, b;
	int i, batch;

	(void)system;
	(void)arg;
	if (DAT_00290cb8 == 0 || DAT_002ed340 != 0 || DAT_00290ed8 == 0 || count == 0)
		return 0;
	for (i = 0; i < count; i++) {
		piece = items[i];
		piece->pos[3] = 1.0f;
		if (piece->stage < 0 || piece->stage >= 3)
			continue;
		if (piece->batch == 0) {
			BCA_Printf("bad debris\n");
			continue;
		}
		vertex = BATCH_WRITE(piece->batch);
		streak = piece->type->streak;
		if (streak == NULL) {
			PIECE_SPRITE_POS(piece)[3] = 1.0f;
			if (bdDrawSprite3D(PIECE_SPRITE_POS(piece), PIECE_SPRITE_SIZE(piece), PIECE_SPRITE_SIZE(piece),
				    PIECE_SPRITE_ANGLE(piece),
				    /* the first frame of the type's sprite: anim->def->frames[0] */
				    *(struct bdTexture **)BCA_PTR(*(void **)piece->type->spriteAnim, 0x40),
				    PIECE_SPRITE_FLIP_U(piece), PIECE_SPRITE_FLIP_V(piece), PIECE_SPRITE_RGBA(piece)[0],
				    PIECE_SPRITE_RGBA(piece)[1], PIECE_SPRITE_RGBA(piece)[2], PIECE_SPRITE_RGBA(piece)[3],
				    (float *)(vertex + 0x1c), 0.0f) == 0)
				continue;
			memcpy(vertex, vertex + 0x1c, 0x1c);
			memcpy(vertex + 0x8c, vertex + 0x70, 0x1c);
			BATCH_WRITE(piece->batch) += 0xa8;
		} else {
			r = piece->rgba[0];
			g = piece->rgba[1];
			b = piece->rgba[2];
			end[3] = 1.0f;
			if (piece->colour != 0xffffffffu) {
				b = piece->colour & 0xff;
				r = piece->colour >> 16;
				g = piece->colour >> 8;
			}
			colour0[0] = (int)(r & 0xff);
			colour0[1] = (int)(g & 0xff);
			colour0[2] = (int)b;
			colour0[3] = piece->rgba[3];
			colour1[0] = streak->alpha * colour0[0] / 0x7f;
			colour1[1] = streak->alpha * colour0[1] / 0x7f;
			colour1[2] = streak->alpha * colour0[2] / 0x7f;
			colour1[3] = streak->alpha * colour0[3] / 0x7f;
			speed = cmb_vec3_length(piece->vel);
			if (!(speed > 0.0f))
				continue;
			length = streak->length;
			if ((piece->flags & DEBRIS_STREAK_ABS) == 0)
				length = length / speed;
			cmb_vec3_scale(end, piece->vel, -length);
			cmb_vec3_add(end, end, piece->pos);
			width = piece->scale * piece->size;
			if (bdDrawBeam3D(piece->pos, width * streak->width0a, width * streak->width0b, colour0, end,
				    width * streak->width1a, width * streak->width1b, colour1, NULL, (float *)(vertex + 0x1c),
				    0.0f) == 0)
				continue;
			memcpy(vertex, vertex + 0x1c, 0x1c);
			memcpy(vertex + 0xfc, vertex + 0xe0, 0x1c);
			BATCH_WRITE(piece->batch) += 0x118;
		}
		batch = piece->batch;
		if (BATCH_NUM_VERTICES(batch) < (int)(BATCH_WRITE(batch) - BATCH_START(batch)) / 0x1c)
			BCA_Printf("ERROR OVERFLOW!!!\n");
	}
	return 0;
}

/* 0x000815f0  (gap): moves a piece by delta against the game's ground. On a hit the piece
bounces (velocity reflected and damped by the type's bounce and the surface), stops when
slow, and moves on with the rest of the delta (recursively, unless DEBRIS_KEEP_ON_STOP).
Returns 1 on a hit. */
int Debris_Collide(struct DebrisPiece *piece, float *delta)
{
	float hit[4], normal[4], rest[4];
	float fraction, bounce, left, along, along_v, old[3], now[3], moved[3];
	float *pos;

	if (DAT_002ed340 != 0 || DAT_002ed33c == NULL)
		return 0;
	if (delta[2] * delta[2] + delta[1] * delta[1] + delta[0] * delta[0] > 0.01f) {
		pos = piece->pos;
		fraction = DAT_002ed33c(pos, delta, hit, normal);
		if (fraction > 0.0f) {
			bounce = normal[3] * piece->type->params->bounce;
			normal[3] = 1.0f;
			left = 1.0f - fraction;
			along = (normal[0] * delta[0] + normal[2] * delta[2] + normal[1] * delta[1]) * left * -2.0f;
			old[0] = pos[0];
			old[1] = pos[1];
			old[2] = pos[2];
			pos[0] = hit[0];
			pos[1] = hit[1];
			pos[2] = hit[2];
			pos[3] = hit[3];
			now[0] = pos[0];
			now[1] = pos[1];
			now[2] = pos[2];
			along_v = (normal[0] * piece->vel[0] + normal[2] * piece->vel[2] + normal[1] * piece->vel[1]) * -2.0f;
			piece->vel[0] = (normal[0] * along_v + piece->vel[0]) * bounce;
			piece->vel[1] = (normal[1] * along_v + piece->vel[1]) * bounce;
			piece->vel[2] = (normal[2] * along_v + piece->vel[2]) * bounce;
			left = left * DAT_0028fda8;
			moved[0] = left * piece->vel[0];
			moved[1] = left * piece->vel[1];
			moved[2] = left * piece->vel[2];
			pos[0] = moved[0] + pos[0];
			pos[1] = moved[1] + pos[1];
			pos[2] = moved[2] + pos[2];
			rest[0] = ((normal[0] * along + old[0]) - now[0]) * bounce - moved[0];
			rest[1] = ((normal[1] * along + old[1]) - now[1]) * bounce - moved[1];
			rest[2] = ((normal[2] * along + old[2]) - now[2]) * bounce - moved[2];
			if (sqrtf(piece->vel[2] * piece->vel[2] + piece->vel[1] * piece->vel[1] +
				    piece->vel[0] * piece->vel[0]) < 0.1f) {
				piece->vel[0] = 0.0f;
				piece->vel[1] = 0.0f;
				piece->vel[2] = 0.0f;
				piece->vel[3] = 1.0f;
				piece->flags &= 0xfffffffeu;
			}
			if ((piece->type->flags & DEBRIS_KEEP_ON_STOP) == 0)
				Debris_Collide(piece, rest);
			return 1;
		}
	}
	piece->pos[0] = delta[0] + piece->pos[0];
	piece->pos[1] = delta[1] + piece->pos[1];
	piece->pos[2] = delta[2] + piece->pos[2];
	return 0;
}

/* 0x00081880  (gap): count pieces in a ring around pos, starting at a random angle; each
moves out at outSpeed and up at upSpeed, with a random jitter of +-jitter/2 per axis */
void Debris_AddRing(struct DebrisType *type, int count, float *pos, float outSpeed, float upSpeed, float jitter,
	int lifeFrames, unsigned int colour)
{
	float step, angle, half, local[4], vel[4];

	if (DAT_002ed340 != 0)
		return;
	step = 0x1.921fb4p+2f /* 0x40c90fda */ / (float)count;
	local[2] = outSpeed;
	local[1] = upSpeed;
	local[0] = 0.0f;
	local[3] = 1.0f;
	angle = (float)bca_rand() * 0x1.9222d8p-14f;
	for (; count > 0; count--) {
		FUN_00051170(vel, local, angle);
		if (jitter > 0.0f) {
			half = jitter * 0.5f;
			vel[1] = ((float)bca_rand() * jitter * (1.0f / 32767.0f) - half) + vel[1];
			vel[0] = ((float)bca_rand() * jitter * (1.0f / 32767.0f) - half) + vel[0];
			vel[2] = ((float)bca_rand() * jitter * (1.0f / 32767.0f) - half) + vel[2];
		}
		Debris_Add(type, pos, vel, lifeFrames, colour);
		angle = CLMath_WrapAngle(angle + step);
	}
}

/* 0x00081a30  (gap): count pieces from a ring of radius `radius` around pos, perpendicular
to dir: each moves along dir at speed (+ random speedRandom) and outwards at outSpeed
(+ random outRandom, scaled by 1/radius). A vertical dir makes a flat ring instead. */
void Debris_AddCone(struct DebrisType *type, int count, float *pos, float *dir, float radius, float speed,
	float outSpeed, float speedRandom, float outRandom, int lifeFrames, unsigned int colour)
{
	float perp[4], axis[4], quat[4], vel[4], at[4], matrix[16];
	float out0, inv;
	unsigned int r;

	if (dir[0] == 0.0f && dir[2] == 0.0f) {
		Debris_AddRing(type, count, pos, outSpeed, speed, speedRandom + outRandom, lifeFrames, colour);
		return;
	}
	perp[0] = DAT_0028fdc8[1] * dir[2] - DAT_0028fdc8[2] * dir[1];
	perp[1] = DAT_0028fdc8[2] * dir[0] - DAT_0028fdc8[0] * dir[2];
	perp[2] = DAT_0028fdc8[0] * dir[1] - DAT_0028fdc8[1] * dir[0];
	FUN_000514a0(perp, perp);
	FUN_000514a0(axis, dir);
	perp[0] = perp[0] * radius;
	perp[3] = 1.0f;
	perp[1] = perp[1] * radius;
	perp[2] = perp[2] * radius;
	inv = 1.0f / radius;
	out0 = outSpeed * inv;
	axis[3] = 0x1.921fb4p+2f /* 0x40c90fda */ / (float)count;
	FUN_00051df0(quat, axis);
	FUN_000515c0(matrix, quat);
	for (; count > 0; count--) {
		FUN_00050530(matrix, perp);
		at[0] = perp[0] + pos[0];
		at[1] = perp[1] + pos[1];
		at[2] = perp[2] + pos[2];
		r = (unsigned int)bca_rand();
		FUN_00050ff0(vel, axis, (float)r * speedRandom * (1.0f / 32767.0f) + speed, perp,
			inv * outRandom * (float)r * (1.0f / 32767.0f) + out0);
		Debris_Add(type, at, vel, lifeFrames, colour);
	}
}

/* 0x00081cb0  (gap): about `count` pieces spread over a sphere around pos: 20 rows of
latitude (step: the angle between rows, in units of 0.05 degrees), each with a share of
the pieces proportional to |sin| of its latitude (error diffusion between rows), at a
random start longitude; each moves out at speed + random speedRandom. */
void Debris_AddSphere(struct DebrisType *type, int count, float step, float *pos, float speed, float speedRandom,
	int lifeFrames)
{
	float weight[21], rowStep, carry, elevStep, azStep, azimuth, elevation, along, up[4], vel[4];
	double angle, total, x;
	int row, n, i;

	if (DAT_002ed340 != 0)
		return;
	rowStep = step * 0x1.c9870ep-11f;
	angle = (double)rowStep * 0.5;
	total = 0.0;
	for (i = 1; i <= 20; i++) {
		weight[i] = (float)fabs(sin(angle));
		total += fabs(sin(angle));
		angle += (double)rowStep;
	}
	carry = 0.0f;
	elevation = 0.0f;
	{
		float share = (float)(1.0 / total);
		float countf = (float)count;

		for (row = 0; row < 20; row++) {
			x = (double)share * (double)weight[row + 1] * (double)countf + (double)carry;
			n = BCA_FTOL(x);
			carry = (float)(x - (double)(float)n);
			if (n == 0)
				continue;
			{
				float invn = 1.0f / (float)n;

				elevStep = rowStep * invn;
				azStep = invn * 0x1.921fb4p+2f /* 0x40c90fda */;
				azimuth = (float)(bca_rand() % 360) * 0x1.921fb4p+1f * 0x1.6c16c2p-8f;
			}
			for (i = n; i > 0; i--) {
				along = (float)bca_rand() * speedRandom * (1.0f / 32767.0f) + speed;
				up[0] = DAT_0028fdc8[0] * along;
				up[1] = DAT_0028fdc8[1] * along;
				up[2] = DAT_0028fdc8[2] * along;
				FUN_00051220(vel, up, elevation, azimuth);
				Debris_Add(type, pos, vel, lifeFrames, 0xffffffffu);
				azimuth = azimuth + azStep;
				elevation = elevStep + elevation;
			}
		}
	}
}

/* 0x00081ed0  (gap): count pieces from pos in random directions of the upper half
(random elevation and azimuth), each at speed + random speedRandom */
void Debris_AddFountain(struct DebrisType *type, float *pos, int count, float speed, float speedRandom,
	unsigned int colour)
{
	float vel[4], along;
	int r1, r2;

	if (DAT_002ed340 != 0 || count <= 0)
		return;
	for (; count != 0; count--) {
		r1 = bca_rand();
		r2 = bca_rand();
		vel[0] = 0.0f;
		vel[1] = 1.0f;
		vel[2] = 0.0f;
		vel[3] = 1.0f;
		FUN_00051220(vel, vel, (float)r1 * 0x1.9222d8p-14f - 0x1.921fb4p+0f, (float)r2 * 0x1.9222d8p-13f);
		along = (float)bca_rand() * speedRandom * (1.0f / 32767.0f) + speed;
		vel[0] = vel[0] * along;
		vel[1] = vel[1] * along;
		vel[2] = along * vel[2];
		Debris_Add(type, pos, vel, 0, colour);
	}
}

/* 0x00082010  ClassLib/Debris.cpp (missing from the table): the update callback. arg holds
the bits of the frame time (Debris_UpdateSystems). Advances the stages (a piece dies after
stage 2, spawning its death debris), blends colour and size, applies gravity, drag, the
ground or collisions, wind and swirls, the speed limit, the spin and the trail. */
int FUN_00082010(struct BExtraSystem *system, int count, struct DebrisPiece **items, void *arg)
{
	float dt = bca_bitsf((unsigned int)(uintptr_t)arg);
	struct DebrisSysData *data;
	struct DebrisPiece *piece;
	struct DebrisParams *params;
	struct DebrisSpin *spin;
	float delta[4], prev[4], trailPos[4], dir[4], t, u, f, k, len2, drag, extra;
	int i, stage;
	unsigned char r, g, b;
	unsigned int colour;

	if (DAT_002ed340 != 0)
		return 0;
	data = (struct DebrisSysData *)system->userData;
	data->time = dt + data->time;
	if (data->wind != NULL)
		DebrisSys_UpdateWind(data);
	for (i = 0; i < count; i++) {
		piece = items[i];
		piece->time = piece->time - dt;
		piece->age = dt + piece->age;
		if (piece->time <= 0.0f) {
			do {
				if (piece->stage == 1 && (piece->flags & 0x8000) != 0) {
					piece->time = 0.0f;
					break;
				}
				if (piece->stage < 0 && piece->batch == 0)
					piece->batch = (int)(uintptr_t)SpriteVB_GetBatch(DEBRIS_VB, (int)(uintptr_t)piece->type);
				stage = piece->stage + 1;
				piece->stage = stage;
				if (stage > 2)
					break;
				piece->time = piece->type->params->life[stage] + piece->time;
				piece->stageTime = piece->type->params->life[stage];
				if (stage == 1) {
					extra = (float)bca_rand() * piece->type->params->lifeRandom * (1.0f / 32767.0f);
					piece->time = extra + piece->time;
					piece->stageTime = extra + piece->stageTime;
				}
			} while (piece->time <= 0.0f);
		}
		stage = piece->stage;
		if (stage < 0)
			continue;
		if (stage >= 3) {
			if (piece->type->onDeath != NULL)
				Debris_SpawnDeath(piece);
			goto remove;
		}
		params = piece->params;
		t = piece->time / piece->stageTime;
		u = 1.0f - t;
		piece->size = t * params->size[stage] + u * params->size[stage + 1];
		piece->rgba[3] = (unsigned char)BCA_FTOL((double)params->alpha[stage + 1] * u + (double)params->alpha[stage] * t);
		piece->rgba[0] = (unsigned char)BCA_FTOL((double)params->red[stage + 1] * u + (double)params->red[stage] * t);
		piece->rgba[1] = (unsigned char)BCA_FTOL((double)params->green[stage + 1] * u + (double)params->green[stage] * t);
		piece->rgba[2] = (unsigned char)BCA_FTOL((double)params->blue[stage + 1] * u + (double)params->blue[stage] * t);
		delta[0] = DAT_0028fda8 * piece->vel[0];
		delta[1] = DAT_0028fda8 * piece->vel[1];
		delta[2] = DAT_0028fda8 * piece->vel[2];
		if ((piece->flags & DEBRIS_GRAVITY) != 0) {
			delta[1] = delta[1] - DAT_0028fda8 * piece->params->gravity * DAT_0028fda8 * 0.5f;
			piece->vel[1] = piece->vel[1] - DAT_0028fda8 * piece->params->gravity;
		}
		drag = (float)pow((double)piece->params->drag, (double)DAT_0028fda8);
		piece->vel[0] = drag * piece->vel[0];
		if ((piece->flags & DEBRIS_NO_FALL) == 0)
			piece->vel[1] = drag * piece->vel[1];
		piece->vel[2] = drag * piece->vel[2];
		if ((piece->flags & DEBRIS_COLLIDE) != 0) {
			if (Debris_Collide(piece, delta) != 0 && (piece->type->flags & DEBRIS_KEEP_ON_STOP) != 0) {
				if (piece->type->onDeath != NULL)
					Debris_SpawnDeath(piece);
				goto remove;
			}
		} else {
			piece->pos[0] = delta[0] + piece->pos[0];
			piece->pos[1] = delta[1] + piece->pos[1];
			piece->pos[2] = delta[2] + piece->pos[2];
			if ((piece->type->flags & DEBRIS_GROUND) != 0 && piece->pos[1] < piece->groundY) {
				piece->pos[1] = (piece->groundY - piece->pos[1]) * piece->type->params->bounce *
					piece->groundBounce + piece->groundY;
				if (piece->vel[1] < 0.0f)
					piece->vel[1] = -(piece->type->params->bounce * piece->groundBounce * piece->vel[1]);
			}
		}
		if ((piece->flags & DEBRIS_WIND) != 0) {
			if (data->wind != NULL && data->wind->numSwirls > 0 &&
			    DebrisSys_SwirlForce(delta, data, piece->pos) != 0) {
				k = 1.0f / (piece->type->windResistance + 1.0f);
				piece->pos[0] = k * delta[0] + piece->pos[0];
				piece->pos[1] = k * delta[1] + piece->pos[1];
				piece->pos[2] = k * delta[2] + piece->pos[2];
			}
			k = 1.0f / (piece->type->windResistance + 1.0f);
			delta[0] = k * (data->wind->dir[0] - piece->vel[0]);
			delta[1] = k * (data->wind->dir[1] - piece->vel[1]);
			delta[2] = k * (data->wind->dir[2] - piece->vel[2]);
			piece->vel[0] = delta[0] + piece->vel[0];
			piece->vel[1] = delta[1] + piece->vel[1];
			piece->vel[2] = delta[2] + piece->vel[2];
		}
		params = piece->type->params;
		if (params->maxSpeed > 0.0f) {
			len2 = piece->vel[2] * piece->vel[2] + piece->vel[1] * piece->vel[1] + piece->vel[0] * piece->vel[0];
			if (params->maxSpeed * params->maxSpeed < len2) {
				f = 1.0f / sqrtf(len2);
				piece->vel[0] = f * params->maxSpeed * piece->vel[0];
				piece->vel[1] = f * piece->type->params->maxSpeed * piece->vel[1];
				piece->vel[2] = piece->type->params->maxSpeed * piece->vel[2] * f;
			}
		}
		prev[0] = piece->drawPos[0];
		prev[1] = piece->drawPos[1];
		prev[2] = piece->drawPos[2];
		prev[3] = piece->drawPos[3];
		piece->drawPos[0] = piece->offset[0] + piece->pos[0];
		piece->drawPos[1] = piece->pos[1] + piece->offset[1];
		piece->drawPos[2] = piece->pos[2] + piece->offset[2];
		piece->drawVel[0] = piece->drawPos[0] - prev[0];
		piece->drawVel[1] = piece->drawPos[1] - prev[1];
		piece->drawVel[2] = piece->drawPos[2] - prev[2];
		r = piece->rgba[0];
		g = piece->rgba[1];
		piece->drawVel[0] = (1.0f / DAT_0028fda8) * piece->drawVel[0];
		piece->drawVel[1] = (1.0f / DAT_0028fda8) * piece->drawVel[1];
		piece->drawPos[3] = 1.0f;
		piece->drawVel[2] = (1.0f / DAT_0028fda8) * piece->drawVel[2];
		PIECE_SPRITE_RGBA(piece)[3] = piece->rgba[3];
		colour = piece->colour;
		PIECE_SPRITE_SIZE(piece) = piece->scale * piece->size * DAT_00290cb4;
		if (colour == 0xffffffffu) {
			b = piece->rgba[2];
		} else {
			r = (unsigned char)(colour >> 16);
			g = (unsigned char)(colour >> 8);
			b = (unsigned char)colour;
		}
		PIECE_SPRITE_RGBA(piece)[0] = r;
		PIECE_SPRITE_RGBA(piece)[1] = g;
		PIECE_SPRITE_RGBA(piece)[2] = b;
		Sprite_Update((struct CSprite *)piece->sprite, dt);
		PIECE_SPRITE_POS(piece)[0] = piece->drawPos[0];
		PIECE_SPRITE_POS(piece)[1] = piece->drawPos[1];
		PIECE_SPRITE_POS(piece)[2] = piece->drawPos[2];
		PIECE_SPRITE_POS(piece)[3] = piece->drawPos[3];
		spin = piece->spin;
		if (spin != NULL) {
			if ((spin->flags & 1) != 0) {
				FUN_000514a0(dir, piece->vel);
				piece->spin->axisAngle[0] = dir[0];
				piece->spin->axisAngle[1] = dir[1];
				piece->spin->axisAngle[2] = dir[2];
				CLVec3_Perpendicular(piece->spin->perp, piece->spin->axisAngle);
			}
			spin = piece->spin;
			spin->axisAngle[3] = DAT_0028fda8 * spin->angleRate + spin->axisAngle[3];
			spin = piece->spin;
			spin->radius = DAT_0028fda8 * spin->radiusRate + spin->radius;
			CLVec3_RotateAboutAxis(piece->offset, piece->spin->perp, piece->spin->axisAngle);
			spin = piece->spin;
			piece->offset[0] = piece->offset[0] * spin->radius;
			piece->offset[1] = spin->radius * piece->offset[1];
			piece->offset[2] = spin->radius * piece->offset[2];
		}
		if (piece->type->trail != NULL) {
			piece->trailTime = DAT_0028fda8 + piece->trailTime;
			if (piece->type->trailInterval < piece->trailTime) {
				do {
					piece->trailTime = piece->trailTime - piece->type->trailInterval;
					f = piece->trailTime / DAT_0028fda8;
					FUN_00050ff0(trailPos, prev, f, piece->drawPos, 1.0f - f);
					colour = (piece->flags & 0x10000) != 0 ? piece->colour : 0xffffffffu;
					Debris_Add(piece->type->trail, trailPos, DAT_002ea478, 0, colour);
				} while (piece->type->trailInterval < piece->trailTime);
			}
		}
		continue;
remove:
		/* the original removes the item and moves on to the next index: the item that
		   moved into this slot is skipped this frame */
		baDeleteExtraItem(system, i);
		count--;
	}
	return 0;
}

/* 0x000827f0  (gap): creates the debris system (maxItems pieces) with the game's ground
test, its wind, and the actor debris system (classlib_fx 0x00086160) */
void Debris_Init(void *ground, int maxItems)
{
	if (DAT_002ed340 != 0)
		return;
	DAT_002ed348 = 0;
	DAT_002ed33c = (DebrisGroundFn)ground;
	DAT_002ed34c = baCreateExtraSystem("debris", (BExtraCallback)FUN_00080f40, (BExtraCallback)FUN_00082010,
		(BExtraCallback)FUN_00080bf0, (BExtraCallback)FUN_000812b0, (BExtraCallback)FUN_00080c50, 0xdc, maxItems,
		3, 8);
	DebrisSys_InitSystemWind(DAT_002ed34c);
	FUN_00086160();
}
