/*
ClassLib/ActorDebris.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\ActorDebris.cpp

Debris pieces drawn as actors: an "actordebris" extra system (bactor baCreateExtraSystem)
whose items fly, spin, blend colour and size through three stages like the sprite debris
of Debris.cpp (bumpercars_a), and are drawn as actors.

The file name comes from the strings of 0x00085580. The table functions here are
0x00085200, 0x000852a0, 0x00085340 and 0x00086160 (gap functions, file inferred); the
five callbacks 0x00085230, 0x00085460, 0x00085560, 0x00085580 and 0x00085a40 are not in
the function table (Ghidra missed them: they are only reached through 0x00086160).
*/

#include "classlib_fx.h"

static const char s_ActorDebrisFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\ActorDebris.cpp";

/* bactor extra systems (owner prototypes) */
#define AD_CreateSystem \
	((int (*)(const char *, void *, void *, void *, void *, void *, int, int, int, int))FUN_0003c780)
#define AD_DeleteItem ((int (*)(int, int))FUN_0003c710)
/* the ground test: (pos, delta, hit, normal) -> hit fraction */
#define AD_Ground ((float (*)(float *, float *, float *, float *))DAT_002ed33c)

#define AD_ACTOR(item) ((void *)(item)->actor)
#define AD_NODE(item) (*(void **)((item)->actor + 0x134))

/* 0x00085200  ClassLib (gap; file inferred: ActorDebris.cpp)
Deletes the actordebris system (all its pieces, even the protected ones). */
void ActorDebris_Kill(void)
{
	if (DAT_002ed340 == 0) {
		DAT_002ed450 = 1;
		if (DAT_002ed44c != 0) {
			FUN_0003c8b0((void *)DAT_002ed44c);
			DAT_002ed44c = 0;
		}
	}
}

/* 0x00085230  ClassLib (not in the function table; file inferred: ActorDebris.cpp)
Delete callback: frees a piece's actor and spin (pieces flagged 0x8000 survive unless the
system is being killed). Returns 0 to keep the piece. */
int FUN_00085230(int system, CActorDebris *item)
{
	if (DAT_002ed340 != 0)
		return 0;
	if ((item->flags & 0x8000) != 0 && DAT_002ed450 == 0)
		return 0;
	DAT_002ed344 = DAT_002ed344 + -1;
	FUN_00034930(AD_ACTOR(item));
	if (item->spin != NULL) {
		CLF_Free(item->spin);
		item->spin = NULL;
	}
	return 1;
}

/* the ambient colour of a piece: its colour override or its stage colour */
static void ActorDebris_SetLight(CActorDebris *item)
{
	uint r;
	uint g;
	uint b;
	int colour;

	item->pos[3] = 1.0f;
	colour = (int)item->colour;
	r = item->rgba[0];
	g = item->rgba[1];
	b = item->rgba[2];
	if (colour != -1) {
		r = (uint)(colour >> 0x10);
		g = (uint)(colour >> 8);
		b = (uint)colour & 0xff;
	}
	FUN_00038440(r & 0xff, g & 0xff, b);
	if (item->texture != 0)
		FUN_00041600((void *)item->texture, CLF_FTOL(item->age * 1000.0f));
}

/* 0x000852a0  ClassLib (gap; file inferred: ActorDebris.cpp; the piece comes in EAX)
Draws an opaque piece (stages 0-2 only). */
void ActorDebris_DrawOpaque(CActorDebris *item)
{
	item->pos[3] = 1.0f;
	if (-1 < item->stage && item->stage < 3) {
		ActorDebris_SetLight(item);
		FUN_00034d50(AD_ACTOR(item), 1);
	}
}

/* 0x00085340  ClassLib (gap; file inferred: ActorDebris.cpp; the piece comes in EAX)
Draws a piece with its model's blend mode; a faded piece through the brightness. */
void ActorDebris_DrawBlended(CActorDebris *item)
{
	uint flags;
	int blend;

	item->pos[3] = 1.0f;
	if (-1 < item->stage && item->stage < 3) {
		ActorDebris_SetLight(item);
		flags = item->type->model->flags;
		if ((flags & 2) == 0) {
			if ((flags & 8) == 0)
				blend = 0;
			else
				blend = 3;
		} else {
			blend = 1;
		}
		FUN_00044b00(10, blend, 0);
		if (0x7e < item->rgba[3] && (item->type->model->flags & 10) == 0) {
			FUN_00034d50(AD_ACTOR(item), 6);
			return;
		}
		FUN_00035d50((uint)item->rgba[3] << 1);
		FUN_00044b00(6, 1, 0);
		FUN_00034d50(AD_ACTOR(item), 7);
		FUN_00044b00(6, 0, 0);
		FUN_00035d50(0x100);
	}
}

/* 0x00085460  ClassLib (not in the function table; file inferred: ActorDebris.cpp)
Draw callback: the opaque pieces first, then every piece blended; restores the ambient
light. */
void FUN_00085460(int system, int count, CActorDebris **items)
{
	int r;
	int g;
	int b;
	int i;

	b = DAT_00284b18;
	g = DAT_00284b14;
	r = DAT_00284b10;
	if (DAT_00290cb8 != 0 && DAT_002ed340 == 0 && count != 0) {
		FUN_00044b00(3, 1, 0);
		FUN_00044b00(5, 1, 0);
		FUN_00044b00(7, 3, 0);
		FUN_00044b00(6, 0, 0);
		for (i = 0; i < count; i++) {
			if ((items[i]->flags & 10) == 0 && 0x7e < items[i]->rgba[3])
				ActorDebris_DrawOpaque(items[i]);
		}
		FUN_00044b00(5, 0, 0);
		FUN_00044b00(6, 1, 0);
		for (i = 0; i < count; i++)
			ActorDebris_DrawBlended(items[i]);
		FUN_00044b00(3, 0, 0);
		FUN_00038440(r, g, b);
		FUN_00044b00(5, 1, 0);
	}
}

/* 0x00085560  ClassLib (not in the function table; file inferred: ActorDebris.cpp)
System callback: forgets the system when it is deleted (reason 1). */
void FUN_00085560(int system, int reason)
{
	if (DAT_002ed340 == 0 && reason == 1)
		DAT_002ed44c = 0;
}

/* 0x00085580  ClassLib (not in the function table; file inferred: ActorDebris.cpp)
Create callback: a new piece of `args->type` at `args->pos` with `args->vel`: its actor
(or the override model DAT_00290e8c, used once), animation, animated texture, random spin,
optional spin offset and the ground height under it. */
void FUN_00085580(int system, CActorDebris *item, CDebrisArgs *args)
{
	CDebrisType *type;
	float *pos;
	float *vel;
	float *spinInit;
	double stage;
	int model;
	void *inst;
	char *name;
	float s;
	float v[4];
	float down[4];
	float hit[4];
	float normal[4];
	float life;
	int loop;
	CDebrisSpin *spin;

	if (DAT_002ed340 != 0)
		return;
	if (args == NULL)
		return;
	DAT_002ed344 = DAT_002ed344 + 1;
	pos = args->pos;
	type = args->type;
	spinInit = type->spinInit;
	vel = args->vel;
	item->stage = 0;
	item->params = type->params;
	item->system = system;
	item->type = type;
	memcpy(&stage, args->stageTime, sizeof(stage));
	item->stageTime = (float)stage;
	if (stage == 0.0) {
		item->time = item->params->life[0];
		item->stageTime = item->params->life[0];
	} else {
		item->time = (float)stage;
		item->stage = -1;
	}
	item->age = 0.0f;
	item->colour = args->colour;
	item->pos[0] = pos[0];
	item->pos[1] = pos[1];
	item->pos[2] = pos[2];
	item->pos[3] = 1.0f;
	item->drawPos[0] = pos[0];
	item->drawPos[1] = pos[1];
	item->drawPos[2] = pos[2];
	item->drawPos[3] = 1.0f;
	item->vel[0] = vel[0];
	item->vel[1] = vel[1];
	item->vel[2] = vel[2];
	item->vel[3] = 1.0f;
	item->flags = type->flags;
	item->scale = 1.0f;
	item->offset[0] = 0.0f;
	item->offset[1] = 0.0f;
	item->offset[2] = 0.0f;
	item->offset[3] = 1.0f;
	item->size = item->params->size[0];
	item->spinSpeed = 0.0f;
	item->spinAxis[0] = 0.0f;
	item->spinAxis[1] = 0.0f;
	item->spinAxis[2] = 0.0f;
	item->spinAngle = 1.0f;
	if (DAT_00290e8c < 0) {
		model = (int)FUN_0004bb30(NULL, type->model->name, 1, 0);
	} else {
		model = (int)FUN_0004bb30(NULL, DAT_00290e90[DAT_00290e8c], 1, 0);
		DAT_00290e8c = -1;
	}
	if (model == 0) {
		CLF_Printf("ERROR couldn't find '%s'\n", type->model->name);
		return;
	}
	inst = FUN_00036b20(AD_ACTOR(item), (void *)model, 5, s_ActorDebrisFile, 0xc5);
	if (inst == NULL) {
		CLF_Printf("couldn't initialize actorInstance\n");
		return;
	}
	name = type->model->animName;
	if (name != NULL) {
		item->anim = CLF_FindAnim((int)item->actor, name);
		if (item->anim != NULL)
			goto have_anim;
		CLF_Printf("DebrisExtraCreateCallback: Couldn't allocate animation segment [%s]\n", name);
	}
	item->anim = NULL;
have_anim:
	name = type->model->textureName;
	if (name != NULL) {
		item->texture = CLF_FindTexture(0, name, 0, 0xdefa);
		if (item->texture == 0) {
			CLF_Printf("DebrisExtraCreateCallback: Couldn't find animated texture [%s]\n", name);
			item->texture = 0;
		}
	}
	*(float *)(item->actor + 0xc0) = item->pos[0];
	*(float *)(item->actor + 0xc4) = item->pos[1];
	*(float *)(item->actor + 0xc8) = item->pos[2];
	*(float *)(item->actor + 0xcc) = item->pos[3];
	s = item->scale * item->size;
	*(float *)(item->actor + 0xdc) = 1.0f;
	*(float *)(item->actor + 0xd8) = s;
	*(float *)(item->actor + 0xd4) = s;
	*(float *)(item->actor + 0xd0) = s;
	if ((item->type->flags & 0x800) == 0) {
		item->spinAxis[0] = 0.0f;
		item->spinAxis[1] = 1.0f;
		item->spinAxis[2] = 0.0f;
	} else {
		v[0] = FUN_00011750() - 0.5f;
		v[1] = FUN_00011750() - 0.5f;
		v[2] = FUN_00011750() - 0.5f;
		FUN_000514a0(item->spinAxis, v);
	}
	item->spinAngle = 0.0f;
	item->spinSpeed = FUN_00011750() * item->params->spinRandom + item->params->spin;
	FUN_00035cc0(AD_ACTOR(item), 0, 1, 0);
	FUN_00035cc0(AD_ACTOR(item), 1, 0, 0);
	FUN_00034e70(AD_ACTOR(item), AD_NODE(item), 0, 1);
	FUN_00038190(AD_ACTOR(item), AD_NODE(item), 1, 0);
	if (item->anim == NULL) {
		item->animSpeed = 1.0f;
	} else {
		if ((type->model->flags & 4) == 0) {
			life = item->params->life[2] + item->params->life[1] + item->params->life[0];
			if (item->params->lifeRandom != 0.0f)
				life = item->params->lifeRandom * 0.5f + life;
			loop = 0;
			item->animSpeed = ((float)(item->anim[2] - item->anim[1]) * 0.0009765625f * 0.0166666675f) / life;
		} else {
			item->animSpeed = 1.0f;
			loop = 1;
		}
		CLF_PlayAnim((int)item->actor, (int)AD_NODE(item), (int)item->anim, 0x400, -1, loop, NULL, 0);
		CLF_UpdateAnim((int)item->actor, (int)AD_NODE(item), 0.0f, 0);
	}
	CLF_UpdateActor((int)item->actor, 1);
	if (spinInit == NULL) {
		item->spin = NULL;
	} else {
		spin = CLF_Alloc(sizeof(CDebrisSpin), s_ActorDebrisFile, 0x11f, 0x2001);
		item->spin = spin;
		FUN_000127e0(spin->axisAngle, spinInit);
		spin->angleRate = spinInit[4];
		spin->radius = spinInit[5];
		spin->radiusRate = spinInit[6];
		spin->flags = ((uint *)spinInit)[7];
		FUN_00070990(spin->perp, spin->axisAngle);
	}
	if ((type->flags & 0x80000) != 0 && DAT_002ed33c != NULL) {
		down[0] = 0.0f;
		down[1] = -1000.0f;
		down[2] = 0.0f;
		down[3] = 1.0f;
		if (0.0f < AD_Ground(item->pos, down, hit, normal)) {
			item->groundY = hit[1];
			item->groundBounce = normal[3];
			return;
		}
		item->groundY = -1000.0f;
		item->groundBounce = 1.0f;
	}
}

/* 0x00085a40  ClassLib (not in the function table; file inferred: ActorDebris.cpp)
Update callback (dt > 0): stages (a piece is deleted after stage 2), colour and size
blends, gravity, drag (drag^frame time), movement with ground bounce or collision, speed
limit, spin, animation, spin offset and trail pieces. */
void FUN_00085a40(int system, int count, CActorDebris **items, void *arg)
{
	/* baUpdateExtras passes the frame time as raw float bits in its pointer argument; a float
	   parameter would read a float register on arm64 */
	unsigned int dtBits = (unsigned int)(uintptr_t)arg;
	float dt;
	CActorDebris *item;
	CDebrisParams *p;
	CDebrisSpin *spin;
	float step[4];
	float old[4];
	float dirN[4];
	float q1[4];
	float q2[4];
	float t;
	float u;
	float f;
	float speed2;
	float bounce;
	float a;
	uint colour;
	int st;
	int i;

	memcpy(&dt, &dtBits, sizeof dt);

	if (DAT_002ed340 != 0 || !(0.0f < dt) || count < 1)
		return;
	for (i = 0; i < count; i++) {
		item = items[i];
		t = item->time - dt;
		item->time = t;
		item->age = dt + item->age;
		if (t <= 0.0f) {
			do {
				if (item->stage == 1 && (item->flags & 0x8000) != 0) {
					item->time = 0.0f;
					break;
				}
				st = item->stage + 1;
				item->stage = st;
				if (2 < st)
					break;
				item->time = item->type->params->life[st] + item->time;
				item->stageTime = item->type->params->life[st];
				if (st == 1) {
					f = (float)CLF_Rand() * item->type->params->lifeRandom * 3.05185094e-05f;
					item->time = f + item->time;
					item->stageTime = f + item->stageTime;
				}
			} while (item->time <= 0.0f);
		}
		st = item->stage;
		if (st < 0)
			continue;
		if (2 < st) {
			if (item->type->onDeath != NULL)
				FUN_00081210((void *)item);
			goto remove;
		}
		p = item->params;
		t = item->time / item->stageTime;
		u = 1.0f - t;
		item->size = t * p->size[st] + u * p->size[st + 1];
		item->rgba[3] = (byte)CLF_FTOL((float)p->alpha[st + 1] * u + (float)p->alpha[st] * t);
		item->rgba[0] = (byte)CLF_FTOL((float)p->red[st + 1] * u + (float)p->red[st] * t);
		item->rgba[1] = (byte)CLF_FTOL((float)p->green[st + 1] * u + (float)p->green[st] * t);
		item->rgba[2] = (byte)CLF_FTOL((float)p->blue[st + 1] * u + (float)p->blue[st] * t);
		step[0] = DAT_0028fda8 * item->vel[0];
		step[1] = DAT_0028fda8 * item->vel[1];
		step[2] = DAT_0028fda8 * item->vel[2];
		if ((item->flags & 1) != 0) {
			step[1] = step[1] - DAT_0028fda8 * item->params->gravity * DAT_0028fda8 * 0.5f;
			item->vel[1] = item->vel[1] - DAT_0028fda8 * item->params->gravity;
		}
		f = (float)pow((double)item->params->drag, (double)DAT_0028fda8);
		item->vel[0] = f * item->vel[0];
		if ((item->flags & 0x20000) == 0)
			item->vel[1] = f * item->vel[1];
		item->vel[2] = f * item->vel[2];
		if ((item->flags & 0x80) != 0) {
			if (FUN_000815f0((void *)item, step) != 0 && (item->type->flags & 0x200) != 0) {
				if (item->type->onDeath != NULL)
					FUN_00081210((void *)item);
				goto remove;
			}
		} else {
			item->pos[0] = step[0] + item->pos[0];
			item->pos[1] = step[1] + item->pos[1];
			item->pos[2] = step[2] + item->pos[2];
			if ((item->type->flags & 0x80000) != 0 && item->pos[1] < item->groundY) {
				bounce = item->type->params->bounce;
				item->pos[1] = (item->groundY - item->pos[1]) * bounce * item->groundBounce + item->groundY;
				if (item->vel[1] < 0.0f)
					item->vel[1] = -(item->type->params->bounce * item->groundBounce * item->vel[1]);
			}
		}
		p = item->type->params;
		if (0.0f < p->maxSpeed) {
			speed2 = item->vel[2] * item->vel[2] + item->vel[1] * item->vel[1] + item->vel[0] * item->vel[0];
			if (p->maxSpeed * p->maxSpeed < speed2) {
				f = 1.0f / sqrtf(speed2);
				item->vel[0] = f * p->maxSpeed * item->vel[0];
				item->vel[1] = item->type->params->maxSpeed * item->vel[1] * f;
				item->vel[2] = item->type->params->maxSpeed * item->vel[2] * f;
			}
		}
		old[0] = item->drawPos[0];
		old[1] = item->drawPos[1];
		old[2] = item->drawPos[2];
		old[3] = item->drawPos[3];
		item->drawPos[0] = item->pos[0] + item->offset[0];
		item->drawPos[1] = item->pos[1] + item->offset[1];
		item->drawPos[2] = item->pos[2] + item->offset[2];
		item->drawVel[0] = item->drawPos[0] - old[0];
		item->drawVel[1] = item->drawPos[1] - old[1];
		item->drawVel[2] = item->drawPos[2] - old[2];
		item->drawVel[0] = (1.0f / DAT_0028fda8) * item->drawVel[0];
		item->drawVel[1] = (1.0f / DAT_0028fda8) * item->drawVel[1];
		f = 1.0f / DAT_0028fda8;
		item->drawPos[3] = 1.0f;
		item->drawVel[2] = f * item->drawVel[2];
		f = item->scale * item->size;
		*(float *)(item->actor + 0xd8) = f;
		*(float *)(item->actor + 0xd4) = f;
		*(float *)(item->actor + 0xd0) = f;
		*(float *)(item->actor + 0xc0) = item->offset[0] + item->pos[0];
		*(float *)(item->actor + 0xc4) = item->pos[1] + item->offset[1];
		*(float *)(item->actor + 0xc8) = item->pos[2] + item->offset[2];
		*(float *)(item->actor + 0xcc) = 1.0f;
		if ((item->type->flags & 0x40) == 0) {
			a = DAT_0028fda8 * item->spinSpeed + item->spinAngle;
			item->spinAngle = a;
			item->spinAngle = FUN_000703a0(a);
			FUN_00070d70((float *)(item->actor + 0xe0), item->spinAxis);
		} else if (0.00999999978f < sqrtf(item->vel[2] * item->vel[2] + item->vel[1] * item->vel[1] +
		                                  item->vel[0] * item->vel[0])) {
			FUN_000514a0(dirN, item->vel);
			FUN_00070a90(q1, dirN);
			a = DAT_0028fda8 * item->spinSpeed + item->spinAngle;
			item->spinAngle = a;
			a = FUN_000703a0(a);
			item->spinAngle = a;
			FUN_00051e60(q2, a);
			FUN_00051ca0((float *)(item->actor + 0xe0), q2, q1);
		}
		CLF_UpdateAnim((int)item->actor, (int)AD_NODE(item), DAT_0028fda8 * item->animSpeed, 0);
		CLF_UpdateActor((int)item->actor, 1);
		spin = item->spin;
		if (spin != NULL) {
			if ((spin->flags & 1) != 0) {
				FUN_000514a0(dirN, item->vel);
				item->spin->axisAngle[0] = dirN[0];
				item->spin->axisAngle[1] = dirN[1];
				item->spin->axisAngle[2] = dirN[2];
				FUN_00070990(item->spin->perp, item->spin->axisAngle);
			}
			spin = item->spin;
			spin->axisAngle[3] = DAT_0028fda8 * spin->angleRate + spin->axisAngle[3];
			spin = item->spin;
			spin->radius = DAT_0028fda8 * spin->radiusRate + spin->radius;
			FUN_000707c0(item->offset, item->spin->perp, item->spin->axisAngle);
			spin = item->spin;
			item->offset[0] = spin->radius * item->offset[0];
			item->offset[1] = item->offset[1] * spin->radius;
			item->offset[2] = item->offset[2] * spin->radius;
		}
		if (item->type->trail != NULL) {
			f = DAT_0028fda8 + item->trailTime;
			item->trailTime = f;
			if (item->type->trailInterval < f) {
				do {
					f = item->trailTime - item->type->trailInterval;
					item->trailTime = f;
					f = f / DAT_0028fda8;
					FUN_00050ff0(step, old, f, item->drawPos, 1.0f - f);
					if ((item->flags & 0x10000) == 0)
						colour = 0xffffffff;
					else
						colour = item->colour;
					FUN_00080d60((void *)item->type->trail, step, DAT_002ea478, 0, colour);
				} while (item->type->trailInterval < item->trailTime);
			}
		}
		continue;
remove:
		AD_DeleteItem(system, i);
		count = count + -1;
	}
}

/* 0x00086160  ClassLib (gap; file inferred: ActorDebris.cpp)
Creates the actordebris system (up to 400 pieces of 0x230 bytes). */
void ActorDebris_Create(void)
{
	if (DAT_002ed340 == 0) {
		DAT_002ed450 = 0;
		DAT_002ed44c = AD_CreateSystem("actordebris", (void *)FUN_00085580, (void *)FUN_00085a40,
		                               (void *)FUN_00085230, (void *)FUN_00085460, (void *)FUN_00085560,
		                               0x230, 400, 3, 0);
	}
}
