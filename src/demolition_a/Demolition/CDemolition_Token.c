/*
Demolition/CDemolition_Token.c  <-  C:\BlitzParty\Game\Src\Demolition\CDemolition_Token.cpp

The token system of the Demolition game object (+0x14c8, vtable 0x0026ab34):
the bonus tokens of variation 2 (0x000eedc0-0x000ef8f0). Only 0x000ef030 is
tagged with this file (an operator new line); the other functions sit between
the overlay and the explosions and only use the tokens, so the file is
inferred. The tokens themselves (CToken, the list, drawing, collection) are
demolition_d's (0x0010c080-0x0010ce90).

A token is placed at a random point of a player's area (the variation's
spawn point shape, a demolition_c DcShape), at least 150 units (and not
within 300 on an axis) from the other tokens when possible. Its type (0, 1, 2:
1, 3 or 5 points) depends on a random number and on how much of the round has
passed.
*/

#include "demolition_a.h"

/* fields of a token (CToken, demolition_d, 0x290 bytes) */
#define TK_POS   0x180  /* vec4 */
#define TK_STATE 0x1e8  /* int: 4 = collected */
#define TK_TYPE  0x1ec  /* int 0..2 */
#define TK_STATE_COLLECTED 4

#define TK_NEXT(tok) DA_TokenNext(tok)

/* 0x0026ab34: the token system */
static const void *const vtable_0026ab34[] = {
	(const void *)CDemoTokenSystem_DeletingDtor, /* [0] */
};

/* 0x000eedc0  (file inferred) */
void *CDemoTokenSystem_Ctor(void *self)
{
	*(const void *const **)self = vtable_0026ab34;
	return self;
}

/* 0x000eedd0  (file inferred) */
void CDemoTokenSystem_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026ab34;
}

/* 0x000eede0  (file inferred)
Shutdown: a jump to demolition_d's CToken_DeleteAll. */
void CDemoTokenSystem_Shutdown(void *self)
{
	(void)self;
	((void (*)(void))FUN_0010cd10)();
}

/* 0x000eedf0  (file inferred) */
void CDemoTokenSystem_DrawGlow(void *self)
{
	struct CDemolition_TokenSystem *ts = (struct CDemolition_TokenSystem *)self;

	if (ts->enabled)
		DA_TokenDrawGlowAll();
}

/* 0x000eee00  (file inferred)
Draws the token shadows (even when the system is disabled), then resets the events. */
void CDemoTokenSystem_DrawShadows(void *self)
{
	(void)self;
	DA_TokenDrawShadowAll();
	DA_EventsReset();
}

/* 0x000eee10  (file inferred) */
void CDemoTokenSystem_Enable(void *self)
{
	((struct CDemolition_TokenSystem *)self)->enabled = 1;
}

/* 0x000eee20  (file inferred) */
void CDemoTokenSystem_Stop(void *self)
{
	((struct CDemolition_TokenSystem *)self)->stopped = 1;
}

/* 0x000eee30  (file inferred)
Returns 1 when a vertical cylinder at POINT (RADIUS, HEIGHT up from POINT) touches the cylinder
at BASE (BASERADIUS, BASEHEIGHT): the y ranges overlap and the xz distance is at most the sum of
the radii. Also used by demolition_b (bombs) and demolition_c (gas jets).

The x and z pre-tests are, in the original, fabs((float)(d > r)) == 0, i.e. only "d <= r"
(the absolute value is taken of the comparison, not of d); kept as is. */
int CDemo_InCylinder(float *point, float radius, float height, float *base, float baseRadius, float baseHeight)
{
	float r = radius + baseRadius;
	float dx = base[0] - point[0];
	float dz;

	if (fabsf((float)(dx > r)) != 0.0f)
		return 0;
	dz = base[2] - point[2];
	if (fabsf((float)(dz > r)) != 0.0f)
		return 0;
	if (point[1] + height < base[1])
		return 0;
	if (base[1] + baseHeight < point[1])
		return 0;
	/* fcompp r*r against the sum: 1 unless r*r is smaller (also 1 when unordered) */
	if (!(r * r < dx * dx + dz * dz))
		return 1;
	return 0;
}

/* 0x000eef30  (file inferred) */
void *CDemoTokenSystem_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026ab34;
	if ((flags & 1) != 0)
		DA_Delete(self);
	return self;
}

/* 0x000eef50  (file inferred)
Init: creates the events, picks a random area (0..3), the area timer (100..300 s) and the
bonus timer (50..100 s); disabled and not stopped. PACKAGE is not used. Always succeeds. */
int CDemoTokenSystem_Init(void *self, void *package)
{
	struct CDemolition_TokenSystem *ts = (struct CDemolition_TokenSystem *)self;

	(void)package;
	DA_EventsCreate(1, 0);
	ts->area = da_rand() & 3;
	ts->areaTimer = (float)da_rand() * DA_RAND_SCALE * 200.0f + 100.0f;
	ts->enabled = 0;
	ts->stopped = 0;
	ts->bonusTimer = (float)da_rand() * DA_RAND_SCALE * 50.0f + 50.0f;
	return 1;
}

/* 0x000ef030  Demolition/CDemolition_Token.cpp
Creates a token of TYPE at POS: POS is first dropped onto the ground under it, and the token
is aligned with the ground triangle's normal (+0x30 of the triangle). A static function (no
"this"). */
void CDemoToken_Spawn(float *pos, int type)
{
	void *tri = pos; /* the original reuses the POS argument slot for the triangle */
	void *tok;

	pos[1] = pos[1] - DA_GroundProbe(pos, 0.0f, &tri, NULL, 0);
	tok = DA_New(0x290, "C:\\BlitzParty\\Game\\Src\\Demolition\\CDemolition_Token.cpp", 0xb4);
	if (tok != NULL)
		DA_TokenInit(tok, pos, type, 1.0f, (float *)((char *)tri + 0x30));
}

/* Shared tail of 0x000ef0c0 and 0x000ef380 (inlined twice in the original): picks a random
point of AREA (a DcShape: x in centre.x +- half, y 2500, z in centre.z +- half) until it is
inside the shape; tries again (at most 11 more times) while a live token is within 300 on both
axes and within 150 units; then spawns a token whose type depends on the round time T
(countdown fraction) and a random F:
  T < 0.2:          F < 0.7 -> 0, else 1
  0.2 <= T < 0.66:  F < 0.1 -> 0, F < 0.8 -> 1, else 2
  T >= 0.66:        F < 0.1 -> 1, else 2 */
static void da_token_place(char *area)
{
	float pos[4];
	float dx, dz, t, f;
	char *tok;
	int tries = 0;
	int type;

	for (;;)
	{
		do
		{
			int r1 = da_rand();
			int r2 = da_rand();
			pos[0] = (DA_FIELD(float, area, 0x34) + DA_FIELD(float, area, 0x34)) * ((float)r1 * DA_RAND_SCALE) -
			         DA_FIELD(float, area, 0x34) + DA_FIELD(float, area, 0x24);
			pos[1] = 2500.0f;
			pos[2] = (DA_FIELD(float, area, 0x34) + DA_FIELD(float, area, 0x34)) * ((float)r2 * DA_RAND_SCALE) -
			         DA_FIELD(float, area, 0x34) + DA_FIELD(float, area, 0x2c);
		} while (DA_RegionContains(area, pos) == 0);

		for (tok = TK_NEXT(NULL); tok != NULL; tok = TK_NEXT(tok))
		{
			if (DA_FIELD(int, tok, TK_STATE) == TK_STATE_COLLECTED)
				continue;
			dx = pos[0] - DA_FIELD(float, tok, TK_POS);
			if (fabsf(dx) > 300.0f)
				continue;
			dz = pos[2] - DA_FIELD(float, tok, TK_POS + 8);
			if (fabsf(dz) > 300.0f)
				continue;
			if (dx * dx + dz * dz > 22500.0f)
				continue;
			break; /* too close to this token */
		}
		if (tok == NULL)
			break;
		if (tries++ > 10)
			break;
	}

	t = DA_TimerFraction(DA_GAME + DG_COUNTDOWN);
	f = (float)da_rand() * DA_RAND_SCALE;
	if (t < 0.2f)
		type = (f < 0.7f) ? 0 : 1;
	else if (t < 0.66f)
	{
		if (f < 0.1f)
			type = 0;
		else
			type = (f < 0.8f) ? 1 : 2;
	}
	else
		type = (f < 0.1f) ? 1 : 2;
	CDemoToken_Spawn(pos, type);
}

/* 0x000ef0c0  (file inferred)
Spawns a token in the player area that has the fewest live tokens (the first such area; the
count of an area stops once it exceeds the best so far). */
void CDemoTokenSystem_SpawnBonus(void *self)
{
	void *var = DA_VARIATION();
	char *best = NULL;
	int bestCount = 99999;
	int i, count;
	char *area, *tok;

	(void)self;
	for (i = 0; i < 4; i++)
	{
		area = (char *)DA_VCALL(var)->spawnPoint(var, i);
		count = 0;
		for (tok = TK_NEXT(NULL); tok != NULL; tok = TK_NEXT(tok))
		{
			if (DA_FIELD(int, tok, TK_STATE) == TK_STATE_COLLECTED)
				continue;
			if (DA_RegionContains(area, (float *)(tok + TK_POS)) != 0)
				count++;
			if (count > bestCount)
				goto next_area;
		}
		if (count < bestCount)
		{
			bestCount = count;
			best = area;
		}
	next_area:;
	}
	da_token_place(best);
}

/* 0x000ef380  (file inferred)
Spawns a token in the current player area (self->area, which the update rotates). */
void CDemoTokenSystem_SpawnInArea(void *self)
{
	struct CDemolition_TokenSystem *ts = (struct CDemolition_TokenSystem *)self;
	void *var = DA_VARIATION();

	da_token_place((char *)DA_VCALL(var)->spawnPoint(var, ts->area));
}

/* 0x000ef5d0  (file inferred)
Updates the tokens. Unless stopped: a live token touched by a player (a 100 x 100 cylinder
against the player's) is collected and gives 1, 3 or 5 points with a floating "1", "3" or "5"
in the player's colour; the area moves on when its timer runs out (new timer 100..300 s); the
token limits shrink with the round (T = countdown fraction):
  +0x0c = 5 - (int)(-20 * (1 - T)),  +0x08 = 15 - (int)(-35 * (1 - T));
below the +0x0c limit a token goes to the emptiest area, and below the +0x08 limit one goes to
the current area each time the second timer runs out (new timer 50..100 s). */
void CDemoTokenSystem_UpdateTokens(void *self, float dt)
{
	struct CDemolition_TokenSystem *ts = (struct CDemolition_TokenSystem *)self;
	static const char *const points[3] = { "1", "3", "5" }; /* 0x0026ab78, 0x0026ab7c, 0x0026ab80 */
	static const int add[3] = { 1, 3, 5 };
	char *tok, *first, *p;
	float pos[4];
	int live = 0, type;
	float t, u;

	DA_TokensUpdate();
	for (tok = TK_NEXT(NULL); tok != NULL; tok = TK_NEXT(tok))
		if (DA_FIELD(int, tok, TK_STATE) != TK_STATE_COLLECTED)
			live++;
	if (ts->stopped)
		return;

	first = DA_PLAYER_FIRST();
	for (tok = TK_NEXT(NULL); tok != NULL; tok = TK_NEXT(tok))
	{
		if (DA_FIELD(int, tok, TK_STATE) == TK_STATE_COLLECTED || first == NULL)
			continue;
		for (p = first; p != NULL; p = DA_PLAYER_NEXT(p))
		{
			if (CDemo_InCylinder((float *)(tok + TK_POS), 100.0f, 100.0f, (float *)(p + DP_POS),
			                     DA_FIELD(float, p, DP_RADIUS), DA_FIELD(float, p, DP_HEIGHT)) == 0)
				continue;
			DA_TokenCollect(tok);
			pos[0] = DA_FIELD(float, p, DP_POS);
			pos[1] = DA_FIELD(float, p, DP_POS + 4) + DA_FIELD(float, p, DP_HEIGHT);
			pos[2] = DA_FIELD(float, p, DP_POS + 8);
			pos[3] = DA_FIELD(float, p, DP_POS + 12);
			type = DA_FIELD(int, tok, TK_TYPE);
			if (type >= 0 && type <= 2)
			{
				DA_FIELD(int, p, DP_SCORE) += add[type];
				CDemoInfo_AddText(DA_GAME + DG_INFO, pos, DA_FIELD(int, p, DP_COLOUR),
				                  DA_FIELD(int, p, DP_COLOUR + 4), DA_FIELD(int, p, DP_COLOUR + 8), 0x7f,
				                  points[type]);
			}
			break; /* one player collects a token */
		}
	}

	ts->areaTimer = ts->areaTimer - dt;
	if (ts->areaTimer < 0.0f)
	{
		ts->areaTimer = (float)da_rand() * DA_RAND_SCALE * 200.0f + 100.0f;
		ts->area = (ts->area + 1) % 4;
	}

	u = 1.0f - DA_TimerFraction(DA_GAME + DG_COUNTDOWN);
	ts->maxTokens = 5 - (int)(-20.0f * u);
	ts->maxBonus = 15 - (int)(u * -35.0f);
	if (live < ts->maxTokens)
		CDemoTokenSystem_SpawnBonus(self);
	if (live < ts->maxBonus)
	{
		t = ts->bonusTimer - dt;
		ts->bonusTimer = t;
		if (t < 0.0f)
		{
			ts->bonusTimer = (float)da_rand() * DA_RAND_SCALE * 50.0f + 50.0f;
			CDemoTokenSystem_SpawnInArea(self);
		}
	}
}

/* 0x000ef8f0  (file inferred)
Update: only when enabled and in variation 2 (mine). */
void CDemoTokenSystem_Update(void *self, float dt)
{
	if (((struct CDemolition_TokenSystem *)self)->enabled && DA_FIELD(int, DA_GAME, DG_VARIATION) == 2)
		CDemoTokenSystem_UpdateTokens(self, dt);
}
