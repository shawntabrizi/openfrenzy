/*
Meta/LevelLoad.c  <-  C:\BlitzParty\Game\Src\Meta\LevelLoad.cpp

The Meta board's level loader: reads Packages\MetaLevs\<zone>_part_<n>.lnd, a text script
of keyword lines (LANDSCAPE, PLATFORM, MOVEKEY, WATER, VENT_*, SEARCHLIGHT, PUZZLE and
ACTION, ...), after a small preprocessor (#define and C comments), and builds the board's
objects. Also the height maps (<zone><n>_<nn>.hmp), key track packing and lookups.

Tagged with LevelLoad.cpp: 0x00166320, 0x00166870, 0x00166c00, 0x00166db0; the functions
between them are bracketed. 0x00166290 (gap, the define lookup) belongs to the
preprocessor (file inferred).
*/

#include "meta_b.h"
#include "meta_b_internal.h"
#include "xkernel.h"

/* ---------- this file's data */

/* the name tables of the Meta .data block (meta_a's copy, original layout; each ends with
   "***"): 0x002b2428 the puzzle actions (ACTION, 74 names: DEBRIS, DESTRUCT, SET_VAR, ...),
   0x002b2554 USE_VAR, 0x002b255c CIRCLE GRID ONE, 0x002b256c MOVE_ONE_STEP SPIN_ONE_STEP
   ONE_CYCLE, 0x002b257c crumble flags (RESET TUMBLE_X TUMBLE_Y TUMBLE_Z), 0x002b2590 path
   behaviours (STOP LOOP BOUNCE), 0x002b25a0 collectables (SPEEDUP SUPER_ATTACK
   INVINCIBILITY TOKEN RANDOM), 0x002b25b8 bomb kinds (CONTACT TIMED PROXIMITY HITBANG
   DYNAMITE), 0x002b25d0 tile kinds, 0x002b25ec button modes (TOGGLE AUTOMATIC ONEOFF),
   0x002b25fc tile flags, 0x002b2620 EXCLUSIVE IGNORE, 0x002b262c vent hazard flags
   (HURT_ALWAYS HURT_WHEN_ON HURT_WHEN_OFF USE_HURT_DIRECTION PUSH_ONLY), 0x002b2658 shadow
   kinds (POINTSOURCE POINT_TOWARDS DIRECTIONAL), 0x002b2668 emitter flags (ON_WATER). */
extern const char *DAT_002b2428[], *DAT_002b2554[], *DAT_002b255c[], *DAT_002b256c[],
	*DAT_002b257c[], *DAT_002b2590[], *DAT_002b25a0[], *DAT_002b25b8[], *DAT_002b25d0[],
	*DAT_002b25ec[], *DAT_002b25fc[], *DAT_002b2620[], *DAT_002b262c[], *DAT_002b2658[],
	*DAT_002b2668[];

/* 0x00309278  ROTATEKEY keys of the current platform (x, y, z, time) */
float DAT_00309278[100][4];
/* 0x003098b8  MOVEKEY keys of the current platform or searchlight */
float DAT_003098b8[100][4];
/* 0x0030a090  the board's ambient light, 0x00BBGGRR (AMBIENTLIGHT) */
unsigned int DAT_0030a090;

/* ---------- other modules' data */

/* 0x0030a09c  the #define list (core constructs it) */
extern char DAT_0030a09c[];
/* 0x00283c74  the zones' .lnd paths (core; 8 words per zone) */
extern char *PTR_s_Packages_MetaLevs_site_00283c74[];
/* 0x00309ef8 start heading, 0x00309f00 route heights, 0x0030a098 high-detail characters,
   0x003045c8 samples, 0x003055d4 the camera, 0x003082c0 the tile board, 0x002b057c the
   round time, rain settings (meta_a) */
extern float DAT_00309ef8;
extern int DAT_00309f00[100];
extern int DAT_0030a098;
extern int DAT_003045c8[];
extern int DAT_003055d4;
extern int DAT_003082c0;
extern char DAT_002b057c[];
extern char DAT_0030455c[], DAT_0030556c[], DAT_00305568[], DAT_003045b4[], DAT_003045c4[],
	DAT_00304570[];
extern int DAT_003055d8;
extern int DAT_003055cc;
/* fog (charlib): on, near, far, colour */
extern int DAT_002f2878;
extern float DAT_002f287c;
extern float DAT_002f2880;
extern int DAT_002f2884, DAT_002f2888, DAT_002f288c;
/* the characters' model names (0x108 each), animation segment names (0x44 each, speed at
   +0x40), camera height, rotation and origin (charlib / core) */
extern char DAT_002900c8[];
extern char DAT_00292ca0[];
extern float DAT_00294b64;
extern float DAT_00294bb8;
extern float DAT_00294ba8, DAT_00294bac, DAT_00294bb0, DAT_00294bb4;
/* the four characters' AI objects (charlib) */
extern int DAT_002f289c[4];
/* the debris type tables (0x2c bytes each): the Meta's 32 at 0x002b1e18, and 12 at
   0x00291600 (bumpercars_a) */
extern char DAT_002b1e18[];
extern char DAT_00291600[];

#define LL_FILE "C:\\BlitzParty\\Game\\Src\\Meta\\LevelLoad.cpp"

/* the debris type `name` from either table, or NULL */
static void *ll_debris_type(const char *name)
{
	void *t;

	t = (void *)Debris_FindType(name, (struct DebrisType *)DAT_002b1e18, 0x20);
	if (t == NULL)
		t = (void *)Debris_FindType(name, (struct DebrisType *)DAT_00291600, 0xc);
	return t;
}

/* 0x00166290  Meta (file inferred: LevelLoad.cpp)
The value of the #define `name` (1), or the name itself (0). */
int LevelLoad_FindDefine(const char *name, const char **value)
{
	char *def;

	for (def = MB_LIST_NEXT(DAT_0030a09c, NULL); def != NULL;
		def = MB_LIST_NEXT(DAT_0030a09c, def))
	{
		if (strcmp(def + 0x48, name) == 0)
		{
			*value = def + 8;
			return 1;
		}
	}
	*value = name;
	return 0;
}

/* in a comment-aware scan: handles the comment markers in the word `w` read at `start`;
   returns 1 when the scan goes on at *next (comment start, end or a // line) */
static int ll_comment(const char *w, char *start, int *depth, char **next)
{
	int len = (int)strlen(w);
	int i;

	if (len < 2)
		return 0;
	for (i = 0; i < len - 1; i++)
	{
		if (*depth > 0 && w[i] == '*' && w[i + 1] == '/')
		{
			(*depth)--;
			*next = start + i + 2;
			return 1;
		}
		if (w[i] == '/')
		{
			if (w[i + 1] == '*')
			{
				(*depth)++;
				*next = start + i + 2;
				return 1;
			}
			if (w[i + 1] == '/')
			{
				while (*start != '\n')
					start++;
				*next = start + 1;
				return 1;
			}
		}
	}
	return 0;
}

/* 0x00166320  Meta/LevelLoad.cpp
The preprocessor: collects the #define lines (outside comments), then writes every word
outside comments, replaced by its #define value, to a new buffer of twice the size (a word
followed by a line end gets "\r", else " "). Frees `text`; *size becomes the new length.
Returns the new buffer. */
char *LevelLoad_Preprocess(char *text, int *size)
{
	char word[256];
	char out[256];
	char *buf;
	char *end;
	char *p;
	char *cur;
	char *write;
	char *def;
	const char *value;
	const char *fmt;
	int bufSize;
	int depth = 0;

	bufSize = *size * 2;
	buf = (char *)bkHeapAlloc(bufSize, LL_FILE, 99, 0x2001);
	end = text + *size;
	write = buf;
	/* pass 1: the #define lines */
	p = text;
	while (p < end)
	{
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			p++;
		if (end <= p)
			break;
		cur = p;
		CTweakMenu_SkipWords(&cur, sscanf(p, "%s", word));
		if (ll_comment(word, p, &depth, &p))
			continue;
		p = cur;
		if (depth == 0 && strcmp(word, "#define") == 0)
		{
			def = (char *)bkHeapCalloc(0x88, 0, LL_FILE, 0xa8, 0x2001);
			MB_LIST_INSERT(DAT_0030a09c, def, 0);
			CTweakMenu_SkipWords(&cur, sscanf(cur, "%s %s", def + 0x48, def + 8));
			p = cur;
		}
	}
	/* pass 2: the words, with the defines replaced */
	p = text;
	while (p < end)
	{
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			p++;
		cur = p;
		if (end <= p)
			break;
		CTweakMenu_SkipWords(&cur, sscanf(p, "%s", word));
		if (ll_comment(word, p, &depth, &p))
			continue;
		if (depth == 0)
		{
			if (LevelLoad_FindDefine(word, &value) == 0)
				value = word;
			sprintf(out, "%s", value);
			fmt = "%s ";
			for (; p != cur; p++)
			{
				if (*p == '\n')
				{
					fmt = "%s\r";
					break;
				}
			}
			sprintf(write, fmt, out);
			write = write + strlen(out) + 1;
			if (buf + bufSize < write)
			{
				bkPrintf("*****ERROR******* lnd file has overrun!!!!!!!!!!!!\n");
				DbgBreakPoint();      /* the XDK's (xkernel) */
			}
		}
		p = cur;
	}
	MB_DELETE(text);
	*size = (int)(write - buf);
	return buf;
}

/* 0x001666e0  Meta/LevelLoad.cpp (bracketed)
Flags from a "+"-separated list of names: bit n for names[n] (names end with "***").
Changes `list` (strtok). */
unsigned int LevelLoad_ParseFlags(char *list, const char **names)
{
	char *end;
	char *word;
	unsigned int flags = 0;
	int i;

	end = list + strlen(list);
	if (end <= list)
		return 0;
	do
	{
		word = strtok(list, "+");
		list = list + strlen(word) + 1;
		for (i = 0; strcmp(names[i], "***") != 0; i++)
		{
			if (strcmp(names[i], word) == 0)
			{
				flags = flags | (1u << (i & 0x1f));
				break;
			}
		}
	} while (list < end);
	return flags;
}

/* 0x001667e0  Meta/LevelLoad.cpp (bracketed)
The number of `name` in `names` (they end with "***"), or -1 with a warning. */
int LevelLoad_Lookup(const char *name, const char **names)
{
	int i;

	for (i = 0; strcmp(names[i], "***") != 0; i++)
	{
		if (strcmp(names[i], name) == 0)
			return i;
	}
	bkPrintf("***WARNING*** unrecognised string %s\n", name);
	return -1;
}

/* 0x00166870  Meta/LevelLoad.cpp
Packs the keys (x, y, z, time) of a key track into shorts: positions relative to their
minimum in 1/65535 of the range, time / 1024. */
void LevelLoad_PackTrack(struct MbKeyTrack *track, float *keys)
{
	float maxX;
	float maxY;
	float maxZ;
	short *k;
	int i;

	if (track->numKeys == 0)
		return;
	track->keys = (short *)bkHeapAlloc((unsigned int)track->numKeys << 3, LL_FILE, 0x989, 0x2001);
	track->min[0] = keys[0];
	track->min[1] = keys[1];
	track->min[2] = keys[2];
	maxX = keys[0];
	maxY = keys[1];
	maxZ = keys[2];
	for (i = 1; i < (int)track->numKeys; i++)
	{
		if (keys[i * 4] < track->min[0])
			track->min[0] = keys[i * 4];
		if (keys[i * 4 + 1] < track->min[1])
			track->min[1] = keys[i * 4 + 1];
		if (keys[i * 4 + 2] < track->min[2])
			track->min[2] = keys[i * 4 + 2];
		if (maxX < keys[i * 4])
			maxX = keys[i * 4];
		if (maxY < keys[i * 4 + 1])
			maxY = keys[i * 4 + 1];
		if (maxZ < keys[i * 4 + 2])
			maxZ = keys[i * 4 + 2];
	}
	track->scale[0] = (maxX - track->min[0]) * 1.5259022e-05f;     /* 0x002705f0 */
	track->scale[1] = (maxY - track->min[1]) * 1.5259022e-05f;
	track->scale[2] = (maxZ - track->min[2]) * 1.5259022e-05f;
	for (i = 0; i < (int)track->numKeys; i++)
	{
		k = track->keys + i * 4;
		if (track->scale[0] <= 0.0f)
			k[1] = 0;
		else
			k[1] = (short)(int)((keys[i * 4] - track->min[0]) / track->scale[0]);
		if (track->scale[1] <= 0.0f)
			k[2] = 0;
		else
			k[2] = (short)(int)((keys[i * 4 + 1] - track->min[1]) / track->scale[1]);
		if (track->scale[2] <= 0.0f)
			k[3] = 0;
		else
			k[3] = (short)(int)((keys[i * 4 + 2] - track->min[2]) / track->scale[2]);
		k[0] = (short)(int)(keys[i * 4 + 3] * 0.0009765625f);  /* 0x002593b0 */
	}
}

/* 0x00166a60  Meta/LevelLoad.cpp (bracketed)
The goal platform nearest to `pos`, or NULL. */
void *LevelLoad_NearestGoal(float *pos)
{
	char *p;
	char *best = NULL;
	float bestDist = 0.0f;
	float d;

	for (p = MB_LIST_NEXT((char *)DAT_002ea488 + 0x18, NULL); p != NULL;
		p = MB_LIST_NEXT((char *)DAT_002ea488 + 0x18, p))
	{
		if (MB_I32(p, 0x458) == 0)
			continue;
		d = ((float (*)(float *, float *))FUN_00051500)(pos, MB_FP(p, 0xe0));
		if (best == NULL || d < bestDist)
		{
			bestDist = d;
			best = p;
		}
	}
	return best;
}

/* 0x00166ad0  Meta/LevelLoad.cpp (bracketed)
The goal platform that suits `player` best: nearest to it and furthest from the others. */
void *LevelLoad_BestGoalFor(int player)
{
	char *p;
	char *best = NULL;
	float bestScore = 0.0f;
	float score;
	float d;
	int i;

	for (p = MB_LIST_NEXT((char *)DAT_002ea488 + 0x18, NULL); p != NULL;
		p = MB_LIST_NEXT((char *)DAT_002ea488 + 0x18, p))
	{
		if (MB_I32(p, 0x458) == 0)
			continue;
		score = 0.0f;
		for (i = 0; i < 4; i++)
		{
			d = ((float (*)(float *, float *))FUN_00051500)(MB_FP(p, 0xe0),
				MB_FP(MB_I32(DAT_003075bc[i], 0x30), 0xd0));
			if (i == player)
				score = score - d;
			else
				score = d + score;
		}
		if (best == NULL || bestScore < score)
		{
			best = p;
			bestScore = score;
		}
	}
	return best;
}

/* 0x00166ba0  Meta/LevelLoad.cpp (bracketed)
VENT_HAZARD: the vent hurts or pushes players within `radius` with `strength`. */
void CMetaVent_SetHazard(struct CMetaVent *self, float radius, float strength,
	unsigned int flags)
{
	self->radius = radius;
	self->strength = strength;
	self->flags = flags;
}

/* 0x00166bd0  Meta/LevelLoad.cpp (bracketed)
VENT_HAZARD_DIRECTION: pushes along (x, y, z). */
void CMetaVent_SetHazardDirection(struct CMetaVent *self, float x, float y, float z)
{
	self->dir[0] = x;
	self->dir[2] = z;
	self->dir[1] = y;
	self->flags = self->flags | 8;
}

/* 0x00166c00  Meta/LevelLoad.cpp
Loads the zone's height maps <path without ".lnd"><part>_<00..99>.hmp that exist into
route height objects (DAT_00309f00). */
void LevelLoad_LoadHeightMaps(int zone, int part)
{
	char base[256];
	char name[256];
	unsigned int size;
	void *obj;
	void *data;
	int i;

	strcpy(base, PTR_s_Packages_MetaLevs_site_00283c74[zone * 8]);
	base[strlen(base) - 4] = '\0';
	for (i = 0; i < 100; i++)
	{
		sprintf(name, "%s%d_%02d.hmp", base, part, i);
		if (bkGetFileSize(name, 1) == 0)
			continue;
		obj = MB_NEW(8, LL_FILE, 0x953);
		if (obj == NULL)
			DAT_00309f00[i] = 0;
		else
			DAT_00309f00[i] = (int)CLRouteHeights_Ctor((struct CLRouteHeights *)obj);
		data = bkLoadFile(NULL, name, NULL, &size, NULL, 0);
		memcpy(*(void **)DAT_00309f00[i], data, size);
	}
}

/* 0x00166d40  Meta/LevelLoad.cpp (bracketed)
Packs the MOVEKEY / ROTATEKEY tracks of the platform (or the searchlight) just finished. */
void LevelLoad_FinishTracks(void *platform, struct CSearchlight *searchlight)
{
	if (platform == NULL)
	{
		if (searchlight != NULL && searchlight->path.numKeys != 0)
			LevelLoad_PackTrack((struct MbKeyTrack *)&searchlight->path, &DAT_003098b8[0][0]);
		return;
	}
	if (MB_U16(platform, 0x3da) != 0)
		LevelLoad_PackTrack((struct MbKeyTrack *)MB_ADDR(platform, 0x3d8), &DAT_003098b8[0][0]);
	if (MB_U16(platform, 0x3fa) != 0)
		LevelLoad_PackTrack((struct MbKeyTrack *)MB_ADDR(platform, 0x3f8), &DAT_00309278[0][0]);
}

/* ---------- the loader */

/* reads the line's arguments with sscanf and moves the cursor past them */
#define LL_SCAN(...) CTweakMenu_SkipWords(&cur, sscanf(cur, __VA_ARGS__))
#define LL_IS(name) (strcmp(tok, (name)) == 0)

/* an ACTION line: the action (meta_a FUN_00153080 adds it to the puzzle) and its arguments
   (offsets of meta_a's action object) */
static void ll_action(void *puzzle, char **cursor, char *tok)
{
	char *cur = *cursor;
	char *a;
	int type;
	int i0 = 0, i1 = 0, i2 = 0, i3 = 0, i4 = 0;
	short s5 = 0;
	int t0 = 0, t1 = 0, t2 = 0;
	int n130 = 0;
	float f[9];
	float v[4];
	float w[4];
	int len;

	memset(f, 0, sizeof f);
	memset(v, 0, sizeof v);
	memset(w, 0, sizeof w);
	a = (char *)(intptr_t)FUN_00153080((int)puzzle);
	LL_SCAN("%s", tok);
	type = LevelLoad_Lookup(tok, DAT_002b2428);
	MB_I32(a, 8) = type;
	switch (type)
	{
	case 0:     /* DEBRIS */
		LL_SCAN("%s %d %d", tok, &i0, &i1);
		MB_PTR(a, 0x18) = (void *)Debris_FindType(tok, (struct DebrisType *)DAT_002b1e18, 0x20);
		MB_I16(a, 0x2c) = (short)i0;
		MB_I16(a, 0x14) = (short)i1;
		bkPrintf("val's = %i %i target=%i\n", (int)MB_I16(a, 0x2c), (int)MB_I16(a, 0x2e),
			(int)(short)i1);
		break;
	case 1:
	case 0x11:
	case 0x12:
	case 0x27:
		LL_SCAN("%d", &i0);
		MB_I16(a, 0x14) = (short)i0;
		break;
	case 2:
	case 3:
	case 4:
		LL_SCAN("%d %f %s", &i0, MB_FP(a, 0x18), tok);
		if (i0 > 999)
			bkPrintf("***LND ERROR!! Puzzle variable out of range - max value is %d\n", 1000);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x4c) = (short)LevelLoad_ParseFlags(tok, DAT_002b2554);
		break;
	case 5:
		LL_SCAN("%s %f %f %f", tok, MB_FP(a, 0x2c), MB_FP(a, 0x30), MB_FP(a, 0x34));
		i0 = LevelLoad_Lookup(tok, DAT_002b25a0);
		MB_I16(a, 0x14) = 2;
		MB_F32(a, 0x18) = (float)i0;
		break;
	case 6:
		LL_SCAN("%s", tok);
		i0 = LevelLoad_Lookup(tok, DAT_002b25a0);
		MB_F32(a, 0x18) = (float)i0;
		LL_SCAN("%s", tok);
		s5 = (short)LevelLoad_Lookup(tok, DAT_002b255c);
		MB_I16(a, 0x14) = s5;
		if (s5 == 0)
			LL_SCAN("%f %f %f %f %f", MB_FP(a, 0x2c), MB_FP(a, 0x30), MB_FP(a, 0x34),
				MB_FP(a, 0x3c), MB_FP(a, 0x40));
		else if (s5 == 1)
			/* six targets for five numbers, as in the original */
			LL_SCAN("%f %f %f %f %f", MB_FP(a, 0x2c), MB_FP(a, 0x30), MB_FP(a, 0x34),
				MB_FP(a, 0x3c), MB_FP(a, 0x40), MB_FP(a, 0x44));
		else if (s5 == 2)
			LL_SCAN("%f %f %f", MB_FP(a, 0x2c), MB_FP(a, 0x30), MB_FP(a, 0x34));
		break;
	case 7:
		LL_SCAN("%d %f %s", &i0, MB_FP(a, 0x18), tok);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x4c) = (short)LevelLoad_ParseFlags(tok, DAT_002b256c);
		break;
	case 8:
	case 9:
	case 0x26:
		LL_SCAN("%d %f", &i0, MB_FP(a, 0x18));
		MB_I16(a, 0x14) = (short)i0;
		break;
	case 10:
		LL_SCAN("%s %d %d", tok, &i0, &i1);
		MB_I16(a, 0x2c) = (short)LevelLoad_Lookup(tok, DAT_002b25d0);
		MB_I16(a, 0x2e) = (short)i0;
		MB_I16(a, 0x30) = (short)i1;
		break;
	case 0xb:
		LL_SCAN("%d %s %d %d", &i0, tok, &i1, &i2);
		MB_I16(a, 0x2c) = (short)i0;
		MB_I16(a, 0x2e) = (short)LevelLoad_Lookup(tok, DAT_002b25d0);
		MB_I16(a, 0x30) = (short)i1;
		MB_I16(a, 0x32) = (short)i2;
		break;
	case 0xc:
	case 0xe:
	case 0xf:
	case 0x1e:
		LL_SCAN("%d", &i0);
		MB_I16(a, 0x2c) = (short)i0;
		break;
	case 0xd:
		LL_SCAN("%s", tok);
		MB_I16(a, 0x2c) = (short)LevelLoad_Lookup(tok, DAT_002b25d0);
		break;
	case 0x10:
		LL_SCAN("%d %s %d %d %f %d", &i0, tok, &i1, &i2, &f[0], (int *)&f[1]);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		MB_I16(a, 0x2e) = (short)i2;
		MB_F32(a, 0x3c) = f[0];
		/* the last number is read with %d into a float slot, as in the original */
		MB_F32(a, 0x40) = f[1];
		len = (int)strlen(tok);
		MB_U32(a, 0x18) = ((unsigned int (*)(const char *, int, unsigned int))FUN_0004dbf0)(
			tok, len, 0);
		break;
	case 0x13:
	case 0x14:
	case 0x17:
	case 0x30:
	case 0x31:
	case 0x33:
	case 0x35:
	case 0x48:
		LL_SCAN("%d", &i0);
		MB_I16(a, 0x14) = (short)i0;
		break;
	case 0x15:
		LL_SCAN("%d %f", &i0, &f[0]);
		MB_I16(a, 0x14) = (short)i0;
		MB_F32(a, 0x3c) = f[0];
		break;
	case 0x16:
	case 0x22:
		LL_SCAN("%d %f %f", &i0, &f[0], &f[1]);
		MB_I16(a, 0x14) = (short)i0;
		MB_F32(a, 0x3c) = f[0];
		MB_F32(a, 0x40) = f[1];
		break;
	case 0x18:
	case 0x19:
	case 0x1f:
	case 0x21:
	case 0x39:
	case 0x46:
	case 0x1b:
	case 0x40:
		LL_SCAN("%d %d", &i0, &i1);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		break;
	case 0x1a:
		break;
	case 0x1c:
		LL_SCAN("%f", MB_FP(a, 0x18));
		break;
	case 0x1d:
		LL_SCAN("%f %f %f %f %f %f %f %f %f", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5],
			&f[6], &f[7], &f[8]);
		MB_F32(a, 0x18) = f[0];
		MB_F32(a, 0x2c) = f[2];
		MB_F32(a, 0x30) = f[3];
		MB_F32(a, 0x34) = f[4];
		MB_F32(a, 0x38) = f[5];
		MB_F32(a, 0x3c) = f[1];
		MB_F32(a, 0x40) = f[6];
		MB_F32(a, 0x44) = f[7];
		MB_F32(a, 0x48) = f[8];
		break;
	case 0x20:
	case 0x49:
		LL_SCAN("%d %d", &i0, &i1);
		MB_I16(a, 0x2c) = (short)i0;
		MB_I16(a, 0x2e) = (short)i1;
		break;
	case 0x23:
	case 0x24:
		LL_SCAN("%d", &i0);
		MB_I16(a, 0x14) = (short)i0;
		break;
	case 0x25:
		LL_SCAN("%s %f %f %f", tok, MB_FP(a, 0x2c), MB_FP(a, 0x30), MB_FP(a, 0x34));
		MB_I16(a, 0x14) = (short)LevelLoad_Lookup(tok, DAT_002b25b8);
		break;
	case 0x28:
		LL_SCAN("%s %d %d %f", tok, &i0, &i1, &f[0]);
		MB_I16(a, 0x2c) = (short)LevelLoad_Lookup(tok, DAT_002b25b8);
		MB_I16(a, 0x2e) = (short)i0;
		MB_I16(a, 0x30) = (short)i1;
		MB_F32(a, 0x18) = f[0];
		break;
	case 0x29:
		LL_SCAN("%d %d %d", &i0, &i1, &i2);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		MB_I16(a, 0x36) = (short)i2;
		break;
	case 0x2a:
		LL_SCAN("%d %d %d %s %d", &i0, &i1, &i2, tok, &i3);
		s5 = (short)LevelLoad_ParseFlags(tok, DAT_002b2620);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		MB_I16(a, 0x2e) = s5;
		MB_I16(a, 0x30) = (short)i2;
		MB_I16(a, 0x36) = (short)i3;
		break;
	case 0x2b:
		LL_SCAN("%d %d %f %f %f %d", &i0, &i1, MB_FP(a, 0x2c), MB_FP(a, 0x30), MB_FP(a, 0x34),
			&i2);
		/* the flags are parsed from the previous word and thrown away */
		LevelLoad_ParseFlags(tok, DAT_002b2620);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		MB_I16(a, 0x36) = (short)i2;
		break;
	case 0x2c:
		LL_SCAN("%d %d %d %f %f %f %s %d", &i0, &i1, &i2, MB_FP(a, 0x2c), MB_FP(a, 0x30),
			MB_FP(a, 0x34), tok, &i3);
		s5 = (short)LevelLoad_ParseFlags(tok, DAT_002b2620);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		MB_I16(a, 0x2e) = s5;
		MB_I16(a, 0x30) = (short)i2;
		MB_I16(a, 0x36) = (short)i3;
		break;
	case 0x2d:
		LL_SCAN("%d %f %f", &i0, MB_FP(a, 0x3c), MB_FP(a, 0x40));
		MB_I16(a, 0x14) = (short)i0;
		break;
	case 0x2e:
		LL_SCAN("%f %f %f %d", MB_FP(a, 0x2c), MB_FP(a, 0x30), MB_FP(a, 0x34), &i0);
		MB_I16(a, 0x2c) = (short)i0;
		break;
	case 0x2f:
		LL_SCAN("%d %d %s %f %f %f", &i0, &i1, tok, MB_FP(a, 0x2c), MB_FP(a, 0x30),
			MB_FP(a, 0x34));
		MB_F32(a, 0x38) = 1.0f;
		MB_I16(a, 0x14) = (short)i0;
		MB_F32(a, 0x3c) = (float)i1;
		MB_U32(a, 0x18) = bkCRCString(tok);
		break;
	case 0x32:
	case 0x34:
		LL_SCAN("%f %f %f %d", MB_FP(a, 0x2c), MB_FP(a, 0x30), MB_FP(a, 0x34), &i0);
		MB_I16(a, 0x14) = (short)i0;
		MB_F32(a, 0x38) = 1.0f;
		break;
	case 0x36:
		LL_SCAN("%f %f %d %d %d", &f[0], &f[1], &i0, &i1, &i2);
		MB_F32(a, 0x3c) = f[0];
		MB_F32(a, 0x40) = f[1];
		MB_I16(a, 0x2c) = (short)i0;
		MB_I16(a, 0x2e) = (short)i1;
		MB_I16(a, 0x30) = (short)i2;
		break;
	case 0x37:
		LL_SCAN("%f %f %f %f %d %d %d %d", &f[0], &f[1], &f[2], &f[3], &i0, &i1, &i2, &i3);
		MB_F32(a, 0x3c) = f[0];
		MB_F32(a, 0x40) = f[1];
		MB_F32(a, 0x44) = f[2];
		MB_F32(a, 0x48) = 1.0f;
		MB_F32(a, 0x18) = f[3];
		MB_I16(a, 0x32) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		MB_I16(a, 0x2e) = (short)i2;
		MB_I16(a, 0x30) = (short)i3;
		break;
	case 0x38:
		LL_SCAN("%d %s %f %f %f %f %d %d %d %d", &i0, tok, &f[0], &f[1], &f[2], &f[3], &i1,
			&i2, &i3, &i4);
		MB_I16(a, 0x14) = (short)i0;
		MB_U32(a, 0x18) = bkCRCString(tok);
		MB_F32(a, 0x1c) = f[0];
		MB_F32(a, 0x20) = f[1];
		MB_F32(a, 0x24) = f[2];
		MB_F32(a, 0x28) = 1.0f;
		MB_F32(a, 0x3c) = f[3];
		MB_I16(a, 0x32) = (short)i1;
		MB_I16(a, 0x2c) = (short)i2;
		MB_I16(a, 0x2e) = (short)i3;
		MB_I16(a, 0x30) = (short)i4;
		break;
	case 0x3a:
		LL_SCAN("%s %d %d %d %f %f %f %d %f %f", tok, &i1, &i2, &i3, MB_FP(a, 0x2c),
			MB_FP(a, 0x30), MB_FP(a, 0x34), &i0, &f[0], &f[1]);
		MB_PTR(a, 0x18) = (void *)Debris_FindType(tok, (struct DebrisType *)DAT_002b1e18, 0x20);
		if (MB_PTR(a, 0x18) == NULL)
		{
			MB_PTR(a, 0x18) = (void *)Debris_FindType(tok, (struct DebrisType *)DAT_00291600,
				0xc);
			if (MB_PTR(a, 0x18) == NULL)
				bkPrintf("ERROR. can't find deb type %s\n", tok);
		}
		MB_F32(a, 0x3c) = f[0];
		MB_F32(a, 0x40) = f[1];
		MB_F32(a, 0x44) = (float)i0;
		/* the colour's last two channels as a short; 0 becomes -1 */
		MB_I16(a, 0x14) = (short)(((i2 & 0xff) << 8) | (i3 & 0xff));
		if ((((i2 & 0xff) << 8) | (i3 & 0xff)) == 0)
			MB_I16(a, 0x14) = -1;
		break;
	case 0x3b:
		LL_SCAN("%d %d %d %d %d %d %d %d", &i0, &i1, &i2, (int *)MB_ADDR(a, 0x4c), &i3, &i4, &t0,
			&t1);
		MB_I16(a, 0x2c) = (short)i0;
		MB_I16(a, 0x2e) = (short)i1;
		MB_I16(a, 0x30) = (short)i2;
		MB_I16(a, 0x32) = (short)i3;
		MB_I16(a, 0x34) = (short)i4;
		MB_I16(a, 0x36) = (short)t0;
		MB_I16(a, 0x14) = (short)t1;
		break;
	case 0x3c:
		LL_SCAN("%d %d %d", &i0, &i1, &i2);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		MB_I16(a, 0x2e) = (short)i2;
		break;
	case 0x3d:
		LL_SCAN("%d %d", &i0, &i1);
		MB_I16(a, 0x14) = (short)i0;
		MB_I16(a, 0x2c) = (short)i1;
		break;
	case 0x3e:
		LL_SCAN("%d %f", &i0, MB_FP(a, 0x18));
		MB_I16(a, 0x14) = (short)i0;
		break;
	case 0x3f:
		LL_SCAN("%f %f %f %f %f", &f[0], &f[1], &f[2], &f[3], &f[4]);
		MB_F32(a, 0x30) = f[1];
		MB_F32(a, 0x2c) = f[0];
		MB_F32(a, 0x38) = 1.0f;
		MB_F32(a, 0x34) = f[2];
		MB_I16(a, 0x14) = (short)(int)f[3];
		MB_F32(a, 0x3c) = f[4];
		break;
	case 0x41:
		LL_SCAN("%d %d %s %f %f %f", &i0, &i1, tok, MB_FP(a, 0x2c), MB_FP(a, 0x30),
			MB_FP(a, 0x34));
		MB_F32(a, 0x38) = 1.0f;
		MB_I16(a, 0x14) = (short)i0;
		MB_F32(a, 0x3c) = (float)i1;
		MB_U32(a, 0x18) = bkCRCString(tok);
		break;
	case 0x42:
		LL_SCAN("%d %d %d %f %f %f %f", &i0, &i1, &i2, &f[0], &f[1], &f[2], &f[3]);
		MB_F32(a, 0x3c) = f[0];
		MB_F32(a, 0x40) = f[1];
		MB_F32(a, 0x44) = f[2];
		MB_F32(a, 0x18) = f[3];
		MB_I16(a, 0x2c) = (short)i0;
		MB_I16(a, 0x2e) = (short)i1;
		MB_I16(a, 0x30) = (short)i2;
		break;
	case 0x44:
		LL_SCAN("%f %f %f %d %f %d %f %d %d %d", MB_FP(a, 0x2c), MB_FP(a, 0x30),
			MB_FP(a, 0x34), &t2, MB_FP(a, 0x3c), &n130, MB_FP(a, 0x40), &i1, &i2, &i3);
		MB_F32(a, 0x44) = (float)n130;
		MB_U8(a, 0x50) = (unsigned char)i1;
		MB_U8(a, 0x51) = (unsigned char)i2;
		MB_U8(a, 0x52) = (unsigned char)i3;
		MB_F32(a, 0x38) = 1.0f;
		MB_I16(a, 0x14) = (short)t2;
		break;
	case 0x45:
		LL_SCAN("%d %f", &i0, &f[0]);
		MB_I16(a, 0x14) = (short)i0;
		MB_F32(a, 0x18) = f[0];
		break;
	case 0x47:
		LL_SCAN("%s %d %d %d %d %f %f %f %f %f %f %f %f %f %f %f", tok, &i0, &i1, &i2, &i3,
			&v[0], &v[1], &v[2], &w[0], &w[1], &w[2], &f[0], &f[1], &f[2], &f[3], &f[4]);
		MB_F32(a, 0x1c) = v[0];
		MB_F32(a, 0x20) = v[1];
		MB_F32(a, 0x24) = v[2];
		MB_F32(a, 0x28) = v[3];
		MB_F32(a, 0x28) = 1.0f;
		Core_Vec4Copy(MB_FP(a, 0x2c), w);
		MB_F32(a, 0x38) = f[0];
		MB_PTR(a, 0x18) = (void *)Debris_FindType(tok, (struct DebrisType *)DAT_002b1e18, 0x20);
		if (MB_PTR(a, 0x18) == NULL)
		{
			MB_PTR(a, 0x18) = (void *)Debris_FindType(tok, (struct DebrisType *)DAT_00291600,
				0xc);
			if (MB_PTR(a, 0x18) == NULL)
				bkPrintf("FATAL ERROR. can't find deb type %s\n", tok);
		}
		MB_I16(a, 0x14) = (short)i0;
		MB_I32(a, 0x50) = ((i1 & 0xff) * 0x100 + (i2 & 0xff)) * 0x100 + (i3 & 0xff);
		if (MB_I32(a, 0x50) == 0)
			MB_I32(a, 0x50) = -1;
		MB_F32(a, 0x3c) = f[1];
		MB_F32(a, 0x40) = f[2];
		MB_F32(a, 0x44) = f[3];
		MB_F32(a, 0x48) = f[4];
		break;
	default:
		bkPrintf("******** LND ERROR *********\naction '%s' not recognized\n\n", tok);
		break;
	}
	/* every action ends with its delay */
	LL_SCAN("%d", &i0);
	MB_F32(a, 0x10) = (float)i0 + 1e-05f;                      /* 0x00257c6c */
	*cursor = cur;
}

/* 0x00166db0  Meta/LevelLoad.cpp
Loads part `part` of zone `zone`'s Meta board. Returns 1 when the .lnd file was read. */
int LevelLoad_Load(int zone, int part)
{
	char path[128];
	char *text;
	char *tok;
	char *tok2;
	char *cur;
	char *end;
	char *b1;
	char *b2;
	char *line;
	int size;
	int loaded = 0;
	void *platform = NULL;          /* the current PLATFORM */
	void *platformCopy = NULL;
	struct CSearchlight *searchlight = NULL;
	char *actor = NULL;             /* the current actor (LANDSCAPE, PLATFORM, ...) */
	void *puzzle = NULL;
	char *tile = NULL;
	struct CMetaEmitter *emitter = NULL;
	struct CMetaWater *water = NULL;
	struct CMetaVent *vent = NULL;
	struct CBomb *bomb = NULL;
	char *sprite = NULL;
	char *obj;
	char *trails;
	void *mem;
	unsigned int flags;
	int i0 = 0, i1 = 0, i2 = 0, i3 = 0, i4 = 0;
	int gunId = 0;
	int unusedD0 = 0;
	int nx = 0, nz = 0;
	int spriteInts[4] = { 0, 0, 0, 0 };
	int spriteFlag = 0;
	unsigned int colour;
	char seesawAxis = 0;
	float f[6];
	float v[4];
	float w[4];
	float g[4];
	float p[4];
	float sx = 0.0f, sz = 0.0f;
	float spriteF[3] = { 0.0f, 0.0f, 0.0f };
	unsigned int camSize = 0;
	void *camData;
	int *map;
	const char *msg;
	struct MbVertex *dummy;
	int k;

	(void)dummy;
	memset(f, 0, sizeof f);
	memset(v, 0, sizeof v);
	memset(w, 0, sizeof w);
	memset(g, 0, sizeof g);
	memset(p, 0, sizeof p);
	DAT_00309ef8 = 0.0f;
	LevelLoad_LoadHeightMaps(zone, part);
	sprintf(path, "%s_part_%d.lnd", PTR_s_Packages_MetaLevs_site_00283c74[zone * 8], part);
	text = (char *)bkLoadFile(NULL, path, NULL, (uint *)&size, NULL, 0x10);
	text[size] = '\n';
	text[size + 1] = '\0';
	text = LevelLoad_Preprocess(text, &size);
	DAT_00309244 = 0;
	DAT_00309248 = 0;
	DAT_002b0588 = 0x73;
	DAT_002b0584 = 0x73;
	DAT_002b0580 = 0x73;
	if (text != NULL)
	{
		loaded = 1;
		tok = (char *)bkHeapCalloc(0x100, 0, LL_FILE, 0x151, 0x2001);
		tok2 = (char *)bkHeapCalloc(0x100, 0, LL_FILE, 0x152, 0x2001);
		end = text + size;
		line = text;
		while (line < end)
		{
			while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n')
				line++;
			cur = line;
			if (end <= line)
				break;
			LL_SCAN("%s", tok);
			if (text + size < cur)
				break;
			if (LL_IS("#define"))
			{
				LL_SCAN("%s %s", tok, tok);
			}
			else if (LL_IS("FOG"))
			{
				LL_SCAN("%f %f %d %d %d", &f[0], &f[1], &i0, &i1, &i2);
				DAT_002f2880 = f[1];
				DAT_002f288c = i2;
				DAT_002f287c = f[0];
				DAT_002f2878 = 1;
				DAT_002f2884 = i0;
				DAT_002f2888 = i1;
				if (f[1] == 0.0f)
					DAT_002f2878 = 0;
			}
			else if (LL_IS("ACTOR_FOG"))
			{
				LL_SCAN("%f %f %d %d %d", &f[0], &f[1], &i0, &i1, &i2);
				if (actor != NULL)
				{
					MB_I32(actor, 0x340) = 1;
					MB_F32(actor, 0x344) = f[0];
					MB_F32(actor, 0x348) = f[1];
					MB_I32(actor, 0x34c) = i0;
					MB_I32(actor, 0x350) = i1;
					MB_I32(actor, 0x354) = i2;
				}
			}
			else if (LL_IS("ACTOR_DONT_FOG"))
			{
				if (actor != NULL)
					MB_U32(actor, 0x220) = MB_U32(actor, 0x220) | 0x20000;
			}
			else if (LL_IS("ACTOR_DONT_LIGHT"))
			{
				if (actor != NULL)
					MB_U32(actor, 0x220) = MB_U32(actor, 0x220) | 0x80000;
			}
			else if (LL_IS("ACTOR_DONT_STATIC_LIGHT"))
			{
				if (actor != NULL)
					MB_U32(actor, 0x220) = MB_U32(actor, 0x220) | 0x200000;
			}
			else if (LL_IS("ACTOR_ADDITIVE"))
			{
				if (actor != NULL)
				{
					MB_U32(actor, 0x220) = MB_U32(actor, 0x220) | 0x400;
					MB_I32(actor, 0x224) = 0x100;
					MB_I32(actor, 0x228) = 0x100;
				}
			}
			else if (LL_IS("ACTOR_DONT_ZWRITE"))
			{
				if (actor != NULL)
					MB_U32(actor, 0x220) = MB_U32(actor, 0x220) | 0x800;
			}
			else if (LL_IS("SAMPLE"))
			{
				LL_SCAN("%s %d", tok, &i0);
				DAT_003045c8[i0] = (int)bLoadResource(NULL, tok, 2, 0xdefa);
			}
			else if (LL_IS("SEAGULLS"))
			{
				LL_SCAN("%d %d %f %f %f %f %f %f", &DAT_00309244, &DAT_00309248, &DAT_00309200[8],
					&DAT_00309200[9], &DAT_00309200[10], &DAT_00309200[12], &DAT_00309200[13],
					&DAT_00309200[14]);
				DAT_00309200[11] = 1.0f;
				DAT_00309200[15] = 1.0f;
				if (DAT_00309248 == 0)
					DAT_00309250 = NULL;
				else
					DAT_00309250 = bkHeapAlloc(DAT_00309248 * 0x2c, LL_FILE, 0x1d0, 0x2001);
			}
			else if (LL_IS("SEAGULLS_OBSTACLE"))
			{
				/* always the first obstacle */
				obj = (char *)DAT_00309250;
				LL_SCAN("%f %f %f %f %f %f %f %f %f", MB_FP(obj, 0), MB_FP(obj, 4), MB_FP(obj, 8),
					MB_FP(obj, 0x10), MB_FP(obj, 0x14), MB_FP(obj, 0x18), MB_FP(obj, 0x24),
					MB_FP(obj, 0x20), MB_FP(obj, 0x28));
			}
			else if (LL_IS("RIPPLE_EMITTER"))
			{
				LL_SCAN("%f %f %f %f %f %f %d %s", &v[0], &v[2], &w[0], &w[2], &f[0], &f[1], &i0,
					tok);
				v[1] = 0.0f;
				v[3] = 1.0f;
				w[1] = 0.0f;
				w[3] = 1.0f;
				flags = LevelLoad_ParseFlags(tok, DAT_002b2668);
				mem = MB_NEW(0x70, LL_FILE, 500);
				if (mem != NULL)
					CMetaEmitter_InitWaterLine((struct CMetaEmitter *)mem, f[0], v, w, f[1],
						(int)flags, i0);
			}
			else if (LL_IS("DEBRIS_EMITTER"))
			{
				LL_SCAN("%s %d %d %d %f %f %f %f %f %f %f %d %s", tok, &i0, &i1, &i2, &v[0],
					&v[1], &v[2], &w[0], &w[1], &w[2], &f[0], &i3, tok2);
				v[3] = 1.0f;
				w[3] = 1.0f;
				flags = LevelLoad_ParseFlags(tok2, DAT_002b2668);
				obj = (char *)ll_debris_type(tok);
				if (obj != NULL)
				{
					colour = ((i0 & 0xff) * 0x100 + (i1 & 0xff)) * 0x100 + (i2 & 0xff);
					if (colour == 0)
						colour = 0xffffffffu;
					mem = MB_NEW(0x70, LL_FILE, 0x214);
					if (mem == NULL)
						emitter = NULL;
					else
						emitter = CMetaEmitter_InitLine((struct CMetaEmitter *)mem, obj, colour, v,
							w, f[0], (int)flags, i3);
				}
			}
			else if (LL_IS("DEBRIS_EMITTER_CIRCLE"))
			{
				LL_SCAN("%s %d %d %d %f %f %f %f %f %d %s", tok, &i0, &i1, &i2, &v[0], &v[1],
					&v[2], &f[0], &f[1], &i3, tok2);
				v[3] = 1.0f;
				w[3] = 1.0f;
				flags = LevelLoad_ParseFlags(tok2, DAT_002b2668);
				obj = (char *)ll_debris_type(tok);
				if (obj != NULL)
				{
					colour = ((i0 & 0xff) * 0x100 + (i1 & 0xff)) * 0x100 + (i2 & 0xff);
					if (colour == 0)
						colour = 0xffffffffu;
					mem = MB_NEW(0x70, LL_FILE, 0x235);
					if (mem == NULL)
						emitter = NULL;
					else
						emitter = CMetaEmitter_InitCircle((struct CMetaEmitter *)mem, obj, colour,
							v, f[0], f[1], (int)flags, i3);
				}
			}
			else if (LL_IS("DEBRIS_EMITTER_VEL"))
			{
				LL_SCAN("%f %f %f %f %f %f", &v[0], &v[1], &v[2], &w[0], &w[1], &w[2]);
				if (emitter != NULL)
					CMetaEmitter_SetVelocity(emitter, v, w);
			}
			else if (LL_IS("ADD_SLIDDING_TEXTURE_MODEL"))
			{
				LL_SCAN("%s %f %f %f %f %f %d", tok, &p[0], &p[1], &p[2], &f[3], &f[4], &i4);
				MB_DUAL(FUN_0015c7a0, void (*)(undefined4, undefined4 *, undefined4, undefined4,
					undefined4), void (*)(char *, float *, float, float, int),
					((undefined4)tok, (undefined4 *)p, mb_fbits(f[3]), mb_fbits(f[4]),
						(undefined4)i4),
					(tok, p, f[3], f[4], i4));
			}
			else if (LL_IS("PUZZLE"))
			{
				b1 = strtok(cur, "\r");
				cur = cur + strlen(b1) + 1;
				mem = MB_NEW(0x20, LL_FILE, 0x26b);
				if (mem == NULL)
					puzzle = NULL;
				else
					puzzle = (void *)(intptr_t)((int (*)(void *, char *))FUN_00154170)(mem, b1);
				actor = NULL;
				bkPrintf("PUZZLE \n");
			}
			else if (LL_IS("NUMTIMES"))
			{
				LL_SCAN("%d", &i0);
				MB_I16(puzzle, 0x18) = (short)i0;
			}
			else if (LL_IS("ACTION"))
			{
				ll_action(puzzle, &cur, tok);
			}
			else if (LL_IS("TILE_POSITION"))
			{
				LL_SCAN("%f %f %f %d %s", &p[0], &p[1], &p[2], &i4, tok);
				flags = LevelLoad_ParseFlags(tok, DAT_002b25fc);
				tile = (char *)((void *(*)(void *, float *, int, unsigned int))FUN_0015a040)(
					(void *)(intptr_t)DAT_003082c0, p, (int)(short)i4, flags);
			}
			else if (LL_IS("TILE_PLATFORM"))
			{
				LL_SCAN("%d", &i0);
				MB_I32(tile, 0x40) = i0;
			}
			else if (LL_IS("TILE_SHAPE"))
			{
				LL_SCAN("%d", (int *)MB_ADDR(tile, 0x10));
			}
			else if (LL_IS("TILE_ANGLE"))
			{
				LL_SCAN("%f", MB_FP(tile, 0x14));
			}
			else if (LL_IS("CAMERA_HEIGHT"))
			{
				LL_SCAN("%f", &DAT_00294b64);
			}
			else if (LL_IS("CAMERA_ROT"))
			{
				LL_SCAN("%f", MB_FP(DAT_003055d4, 0x2c0));
				DAT_00294bb8 = MB_F32(DAT_003055d4, 0x2c0);
			}
			else if (LL_IS("CAMERA_ORIGIN"))
			{
				LL_SCAN("%f %f %f", MB_FP(DAT_003055d4, 0x4b0), MB_FP(DAT_003055d4, 0x4b4),
					MB_FP(DAT_003055d4, 0x4b8));
				Core_Vec4Copy(MB_FP(DAT_003055d4, 0x4c0), MB_FP(DAT_003055d4, 0x4b0));
				DAT_00294ba8 = MB_F32(DAT_003055d4, 0x4b0);
				DAT_00294bac = MB_F32(DAT_003055d4, 0x4b4);
				DAT_00294bb0 = MB_F32(DAT_003055d4, 0x4b8);
				DAT_00294bb4 = MB_F32(DAT_003055d4, 0x4bc);
			}
			else if (LL_IS("FAR_CLIP"))
			{
				LL_SCAN("%f", MB_FP(DAT_0030564c, 0x5c));
			}
			else if (LL_IS("NEAR_CLIP"))
			{
				LL_SCAN("%f", MB_FP(DAT_0030564c, 0x58));
			}
			else if (LL_IS("GLOBAL_VENT_DROP_BALL_LIMIT"))
			{
				LL_SCAN("%f", &DAT_002b0a5c);
			}
			else if (LL_IS("RESPAWN_POINT"))
			{
				LL_SCAN("%f %f %f", &v[0], &v[1], &v[2]);
				mem = MB_NEW(0x20, LL_FILE, 0x52b);
				if (mem != NULL)
					((void *(*)(void *, float *, int))FUN_000b9b20)(mem, v,
						(int)(short)(MB_I32((char *)DAT_002ea488, 0x38) + 1));
			}
			else if (LL_IS("NOT_IN_HEIGHT_MAP"))
			{
				MB_U32(actor, 0x220) = MB_U32(actor, 0x220) | 0x80;
			}
			else if (LL_IS("PLAYER_ROT"))
			{
				LL_SCAN("%f", &f[0]);
				DAT_00309ef8 = f[0];
			}
			else if (LL_IS("PLAYER_MODEL"))
			{
				LL_SCAN("%d %s", &i0, tok);
				strcpy(DAT_002900c8 + i0 * 0x108, tok);
			}
			else if (LL_IS("PLAYER_ANIMSEG"))
			{
				LL_SCAN("%d %s", &i0, tok);
				strcpy(DAT_00292ca0 + i0 * 0x44, tok);
			}
			else if (LL_IS("PLAYER_ANIMSPEED"))
			{
				LL_SCAN("%d %f", &i0, &f[0]);
				MB_F32(DAT_00292ca0, i0 * 0x44 + 0x40) = f[0];
			}
			else if (LL_IS("SEARCHLIGHT"))
			{
				LevelLoad_FinishTracks(platform, searchlight);
				platform = NULL;
				platformCopy = NULL;
				LL_SCAN("%s %f %f %f %f %d %d", tok, &v[0], &v[1], &v[2], &f[0], &i0, &unusedD0);
				mem = MB_NEW(0xe4, LL_FILE, 0x559);
				if (mem == NULL)
					searchlight = NULL;
				else
					searchlight = CSearchlight_Construct((struct CSearchlight *)mem, tok, 0, v,
						f[0], i0, -1, 0, NULL, NULL, NULL);
				actor = (char *)searchlight->actor;
			}
			else if (LL_IS("SEARCHLIGHT_WITHTURRET"))
			{
				LevelLoad_FinishTracks(platform, searchlight);
				platform = NULL;
				platformCopy = NULL;
				b1 = (char *)bkHeapCalloc(0x100, 0, LL_FILE, 0x56e, 0x2001);
				b2 = (char *)bkHeapCalloc(0x100, 0, LL_FILE, 0x56f, 0x2001);
				LL_SCAN("%s %f %f %f %f %d %d %s %s %f %f %f", tok, &v[0], &v[1], &v[2], &f[0],
					&i0, &gunId, b1, b2, &g[0], &g[1], &g[2]);
				mem = MB_NEW(0xe4, LL_FILE, 0x572);
				if (mem == NULL)
					searchlight = NULL;
				else
					searchlight = CSearchlight_Construct((struct CSearchlight *)mem, tok, 0, v,
						f[0], i0, gunId, 0, b1, b2, g);
				actor = (char *)searchlight->actor;
				MB_DELETE(b1);
				MB_DELETE(b2);
			}
			else if (LL_IS("SEARCHLIGHT_STREAK"))
			{
				LevelLoad_FinishTracks(platform, searchlight);
				platform = NULL;
				platformCopy = NULL;
				b1 = (char *)bkHeapCalloc(0x100, 0, LL_FILE, 0x583, 0x2001);
				b2 = (char *)bkHeapCalloc(0x100, 0, LL_FILE, 0x584, 0x2001);
				LL_SCAN("%s %s %f %f %f %f %d %d", b1, b2, &v[0], &v[1], &v[2], &f[0], &i0,
					&gunId);
				mem = MB_NEW(0xe4, LL_FILE, 0x589);
				if (mem == NULL)
					searchlight = NULL;
				else
					searchlight = CSearchlight_Construct((struct CSearchlight *)mem, b1,
						(int)b2, v, f[0], i0, gunId, 1, NULL, NULL, NULL);
				searchlight->beamStartWidth = 150.0f;
				searchlight->beamStartLength = 150.0f;
				searchlight->beamEndWidth = 150.0f;
				searchlight->beamEndLength = 150.0f;
				searchlight->beamStartColour[0] = 0x46;
				searchlight->beamStartColour[1] = 0x46;
				searchlight->beamStartColour[2] = 0x46;
				searchlight->beamStartColour[3] = 0x32;
				searchlight->beamColour[0] = 0x46;
				searchlight->beamColour[1] = 0x46;
				searchlight->beamColour[2] = 0x46;
				searchlight->beamColour[3] = 0x32;
				MB_DELETE(b1);
				MB_DELETE(b2);
			}
			else if (LL_IS("TIME_LIMIT"))
			{
				LL_SCAN("%f", &searchlight->threshold);
			}
			else if (LL_IS("PLAY_TIME"))
			{
				LL_SCAN("%f", (float *)DAT_002b057c);
			}
			else if (LL_IS("TEXTURE_ANIM"))
			{
				LL_SCAN("%s %f", tok, &f[0]);
				if (actor != NULL)
				{
					mem = MB_NEW(0x18, LL_FILE, 0x5af);
					if (mem != NULL)
						AnimTex_Init((struct CAnimTex *)mem, tok,
							f[0] * 1000.0f * 0.016666668f,              /* 0x00255898, 0x00262b7c */
							(struct CLFList *)MB_ADDR(actor, 0x328));
				}
			}
			else if (LL_IS("PLAY_TEXTURE_ANIM"))
			{
				LL_SCAN("%f", &f[0]);
				if (actor != NULL)
					MB_F32(actor, 0x334) = f[0];
			}
			else if (LL_IS("PLATFORM") || LL_IS("PLATFORM_BACKGROUND"))
			{
				k = LL_IS("PLATFORM_BACKGROUND");
				LevelLoad_FinishTracks(platform, searchlight);
				searchlight = NULL;
				LL_SCAN("%s %f %f %f", tok, &v[0], &v[1], &v[2]);
				platform = Platform_Create(tok, v, k);
				actor = (char *)platform + 0x10;
				MB_I16(platform, 0x3fa) = 0;
				MB_I16(platform, 0x3da) = 0;
				MB_I16(platform, 0x3f8) = 1;
				MB_I16(platform, 0x3d8) = 1;
				bomb = NULL;
				sprite = NULL;
				platformCopy = platform;
			}
			else if (LL_IS("GOAL"))
			{
				MB_I32(platform, 0x458) = 1;
				MB_U32(platform, 0x230) = MB_U32(platform, 0x230) | 0x20;
				MB_F32(platform, 0x344) = 1.0f;
			}
			else if (LL_IS("WATER"))
			{
				LL_SCAN("%f %f %f %d %d %f %f %d", &v[0], &v[1], &v[2], &nx, &nz, &sx, &sz, &i0);
				mem = MB_NEW(0x174, LL_FILE, 0x60c);
				if (mem == NULL)
					water = NULL;
				else
					water = CMetaWater_Construct((struct CMetaWater *)mem, v, nx, nz, sx, sz,
						(short)i0);
			}
			else if (LL_IS("WATER_COLOUR"))
			{
				LL_SCAN("%d %d %d", &DAT_002b0580, &DAT_002b0584, &DAT_002b0588);
			}
			else if (LL_IS("STATIC_RIPPLE"))
			{
				LL_SCAN("%f %f %f %f %f %f", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5]);
				if (water != NULL)
					CMetaWater_AddDrip(water, f[0], f[1], f[2], f[3], f[4], f[5]);
			}
			else if (LL_IS("AGITATE"))
			{
				LL_SCAN("%f %f", &f[0], &f[1]);
				if (water != NULL)
					CMetaWater_SetDrops(water, 1.0f / f[0], f[1]);
			}
			else if (LL_IS("FLOATER"))
			{
				if (platform != NULL)
					MB_U32(platform, 0x450) = MB_U32(platform, 0x450) | 0x4000;
			}
			else if (LL_IS("PLATFORM_TRAIL"))
			{
				LL_SCAN("%s", tok);
				if (platform != NULL)
				{
					if (MB_PTR(platform, 0x44c) == NULL)
						MB_PTR(platform, 0x44c) = bkHeapCalloc(0x48, 0, LL_FILE, 0x63f, 0x2001);
					if (MB_I32(MB_PTR(platform, 0x44c), 0x40) < 8)
					{
						trails = (char *)MB_PTR(platform, 0x44c);
						MB_PTR(trails, 0x20 + MB_I32(trails, 0x40) * 4) = baFindNode(
							(struct BActorNode *)MB_PTR(platform, 0x154), tok);
						trails = (char *)MB_PTR(platform, 0x44c);
						if (MB_PTR(trails, 0x20 + MB_I32(trails, 0x40) * 4) != NULL)
						{
							mem = MB_NEW(0x584c, LL_FILE, 0x64c);
							if (mem == NULL)
								mem = NULL;
							else
								mem = CProjTrail_Init((struct CProjTrail *)mem, DAT_00290da4, 0x1e,
									DAT_00290d80, DAT_00290d90, 50.0f, 1.0f, 0);
							trails = (char *)MB_PTR(platform, 0x44c);
							MB_PTR(trails, MB_I32(trails, 0x40) * 4) = mem;
							if (MB_PTR(platform, 0x44c) == NULL)
								bkPrintf("ERROR couldn't create CMetaTrail\n");
							else
								MB_I32(MB_PTR(platform, 0x44c), 0x40) =
									MB_I32(MB_PTR(platform, 0x44c), 0x40) + 1;
						}
					}
					else
					{
						bkPrintf("WARNING! TOO many platform trails\n");
					}
				}
			}
			else if (LL_IS("RAIN"))
			{
				LL_SCAN("%d %f %f %d %f %f", (int *)DAT_0030455c, (float *)DAT_0030556c,
					(float *)DAT_00305568, (int *)DAT_003045b4, (float *)DAT_003045c4,
					(float *)DAT_00304570);
				DAT_003055d8 = 1;
			}
			else if (LL_IS("BIGREDBUTTON"))
			{
				LL_SCAN("%s %f %f %d", tok, &f[0], &f[1], &i1);
				i0 = LevelLoad_Lookup(tok, DAT_002b25ec);
				mem = MB_NEW(0x48, LL_FILE, 0x674);
				if (mem != NULL)
					CBigRedButton_Construct((struct CBigRedButton *)mem, platform, i0, f[0], f[1],
						i1);
			}
			else if (LL_IS("CRUMBLE"))
			{
				obj = (char *)bkHeapCalloc(0x20, 0, LL_FILE, 0x67a, 0x2001);
				MB_PTR(platform, 0x43c) = obj;
				LL_SCAN("%d %d %s %f %f", &i0, &i1, tok, MB_FP(obj, 8), MB_FP(obj, 0xc));
				MB_I16(MB_PTR(platform, 0x43c), 4) = (short)i0;
				MB_I16(MB_PTR(platform, 0x43c), 2) = (short)i1;
				MB_I16(MB_PTR(platform, 0x43c), 6) = (short)LevelLoad_ParseFlags(tok, DAT_002b257c);
				Core_Vec4Copy(MB_FP(MB_PTR(platform, 0x43c), 0x10), MB_FP(platform, 0xe0));
			}
			else if (LL_IS("DESTRUCT"))
			{
				MB_PTR(platform, 0x440) = bkHeapCalloc(0x34, 0, LL_FILE, 0x687, 0x2001);
			}
			else if (LL_IS("REMAINS"))
			{
				LL_SCAN("%s %f", (char *)MB_PTR(platform, 0x440), &f[0]);
				MB_F32(MB_PTR(platform, 0x440), 0x18) = f[0];
			}
			else if (LL_IS("REMAINS_NOCOLL"))
			{
				if (platform != NULL && MB_PTR(platform, 0x440) != NULL)
					MB_I32(MB_PTR(platform, 0x440), 0x30) = 1;
			}
			else if (LL_IS("TAG"))
			{
				LL_SCAN("%d", &i0);
				if (bomb != NULL)
					bomb->tag = i0;
				else if (sprite != NULL)
					MB_I32(sprite, 0x38) = i0;
				else
					MB_I32(platform, 0x3d0) = i0;
			}
			else if (LL_IS("MOVEKEY"))
			{
				if (platform == NULL)
				{
					k = searchlight->path.numKeys;
					if (k < 100)
					{
						LL_SCAN("%f %f %f %f", &DAT_003098b8[k][0], &DAT_003098b8[k][1],
							&DAT_003098b8[k][2], &DAT_003098b8[k][3]);
						DAT_003098b8[k][3] = DAT_003098b8[k][3] * 1024.0f;   /* 0x00253dfc */
						if (MB_I32(searchlight, 0x38) < (int)DAT_003098b8[k][3])
							MB_I32(searchlight, 0x38) = (int)DAT_003098b8[k][3];
						searchlight->path.numKeys = (unsigned short)(k + 1);
					}
				}
				else
				{
					k = MB_U16(platform, 0x3da);
					if (k < 100)
					{
						LL_SCAN("%f %f %f %f", &f[0], &f[1], &f[2], &f[3]);
						DAT_003098b8[k][0] = f[0];
						DAT_003098b8[k][1] = f[1];
						DAT_003098b8[k][2] = f[2];
						DAT_003098b8[k][3] = f[3];
						DAT_003098b8[k][3] = DAT_003098b8[k][3] * 1024.0f;
						if (MB_I32(platform, 0x420) < (int)DAT_003098b8[k][3])
							MB_I32(platform, 0x420) = (int)DAT_003098b8[k][3];
						MB_U16(platform, 0x3da) = (unsigned short)(k + 1);
					}
				}
			}
			else if (LL_IS("ROTATEKEY"))
			{
				k = MB_U16(platform, 0x3fa);
				if (k < 100)
				{
					LL_SCAN("%f %f %f %f", &DAT_00309278[k][0], &DAT_00309278[k][1],
						&DAT_00309278[k][2], &DAT_00309278[k][3]);
					DAT_00309278[k][0] = DAT_00309278[k][0] * 0.017453292f;   /* 0x002640f8 */
					DAT_00309278[k][1] = DAT_00309278[k][1] * 0.017453292f;
					DAT_00309278[k][2] = DAT_00309278[k][2] * 0.017453292f;
					DAT_00309278[k][3] = DAT_00309278[k][3] * 1024.0f;
					if (MB_I32(platform, 0x420) < (int)DAT_00309278[k][3])
						MB_I32(platform, 0x420) = (int)DAT_00309278[k][3];
					MB_U16(platform, 0x3fa) = (unsigned short)(k + 1);
				}
			}
			else if (LL_IS("SPEED"))
			{
				float *speed = platform == NULL ? MB_FP(searchlight, 0x34) : MB_FP(platform, 0x41c);

				LL_SCAN("%f", speed);
				*speed = *speed * 1024.0f;
			}
			else if (LL_IS("BEHAVIOUR"))
			{
				LL_SCAN("%s", tok);
				i0 = LevelLoad_Lookup(tok, DAT_002b2590);
				if (platform == NULL)
					MB_I32(searchlight, 0x3c) = i0;
				else
					MB_I32(platform, 0x424) = i0;
			}
			else if (LL_IS("SEESAW"))
			{
				obj = (char *)bkHeapCalloc(0x28, 0, LL_FILE, 0x6f8, 0x2001);
				MB_PTR(platform, 0x444) = obj;
				LL_SCAN("%c %f %f %f %f %f %f", &seesawAxis, MB_FP(obj, 0x10), MB_FP(obj, 0x14),
					MB_FP(obj, 0x18), MB_FP(obj, 8), MB_FP(obj, 4), MB_FP(obj, 0xc));
				obj = (char *)MB_PTR(platform, 0x444);
				obj[0] = (char)(seesawAxis - 'X');
				MB_F32(obj, 0x20) = MB_F32(obj, 0x14);
				MB_F32(obj, 0x24) = MB_F32(platform, 500 + (unsigned char)obj[0] * 4);
			}
			else if (LL_IS("TOPPLE"))
			{
				LL_SCAN("%f", MB_FP(MB_PTR(platform, 0x444), 0x1c));
			}
			else if (LL_IS("CONVEYOR"))
			{
				obj = (char *)bkHeapCalloc(0x38, 0, LL_FILE, 0x708, 0x2001);
				MB_PTR(platform, 0x448) = obj;
				LL_SCAN("%f %f %f %f", &v[0], &v[1], &v[2], MB_FP(obj, 0x10));
				FUN_000514a0((float *)MB_PTR(platform, 0x448), v);
				MB_F32(MB_PTR(platform, 0x448), 0x34) = MB_F32(MB_PTR(platform, 0x448), 0x10);
			}
			else if (LL_IS("INACTIVE"))
			{
				if (platform == NULL)
				{
					searchlight->state = 1;
					MB_F32(searchlight->actor, 0xe8) = 0.0f;
					MB_F32(searchlight->actor, 0xe0) = 0.0f;
				}
				else
				{
					Platform_Deactivate(platform);
				}
			}
			else if (LL_IS("COLLECTABLE"))
			{
				LL_SCAN("%s %f %f %f", tok, &v[0], &v[1], &v[2]);
				i0 = LevelLoad_Lookup(tok, DAT_002b25a0);
				mem = MB_NEW(0x250, LL_FILE, 0x722);
				if (mem != NULL)
					MB_DUAL(FUN_000b9d10, int (*)(int, int, undefined4 *, undefined4),
						int (*)(void *, int, float *, float),
						((int)mem, i0, (undefined4 *)v, 0x3f800000u),
						(mem, i0, v, 1.0f));
			}
			else if (LL_IS("BOMB"))
			{
				LL_SCAN("%s %f %f %f", tok, &v[0], &v[1], &v[2]);
				i0 = LevelLoad_Lookup(tok, DAT_002b25b8);
				mem = MB_NEW(0x5c, LL_FILE, 0x72a);
				if (mem == NULL)
					bomb = NULL;
				else
					bomb = CBomb_Construct((struct CBomb *)mem, i0, v, 0);
			}
			else if (LL_IS("COLLECTABLE_CIRCLE"))
			{
				LL_SCAN("%s %f %f %f %f %d", tok, &v[0], &v[1], &v[2], &w[1], &i1);
				i0 = LevelLoad_Lookup(tok, DAT_002b25a0);
				FUN_000ba1a0(i0, v, w[1], i1);
			}
			else if (LL_IS("COLLECTABLE_GRID"))
			{
				LL_SCAN("%s %f %f %f %f %d %d", tok, &v[0], &v[1], &v[2], &w[0], &i1, &i2);
				i0 = LevelLoad_Lookup(tok, DAT_002b25a0);
				FUN_000ba2a0(i0, v, w[0], i1, i2);
			}
			else if (LL_IS("AMBIENTLIGHT"))
			{
				LL_SCAN("%d %d %d", &i0, &i1, &i2);
				baSetAmbientLight(i0, i1, i2);
				/* the original stores the red in all three bytes */
				DAT_0030a090 = (DAT_0030a090 & 0xff000000u) | ((unsigned int)(i0 & 0xff) * 0x10101u);
			}
			else if (LL_IS("DIRECTIONAL_LIGHT"))
			{
				LL_SCAN("%d %d %d %f %f", &i0, &i1, &i2, &v[0], &v[1]);
				FUN_00051220(w, DAT_0028fdd8, v[0], v[1]);
				mem = MB_NEW(0xe8, LL_FILE, 0x74f);
				if (mem != NULL)
					Light_InitDirectional(mem, (unsigned char)i0, (unsigned char)i1,
						(unsigned char)i2, w);
			}
			else if (LL_IS("SPINNING_LIGHT"))
			{
				LL_SCAN("%d %d %d %f %f %f", &i0, &i1, &i2, &f[0], &f[1], &f[2]);
				FUN_00051220(v, DAT_0028fdd8, f[0], f[1]);
				mem = MB_NEW(0xe8, LL_FILE, 0x759);
				if (mem != NULL)
					Light_InitTurning(mem, (unsigned char)i0, (unsigned char)i1,
						(unsigned char)i2, f[2], v);
			}
			else if (LL_IS("SPOTLIGHT"))
			{
				LL_SCAN("%d %d %d %f %f %f %f %f %f %f %f", &i0, &i1, &i2, &v[0], &v[1], &v[2],
					&w[0], &w[1], &w[2], &f[0], &f[1]);
				mem = MB_NEW(0xe8, LL_FILE, 0x762);
				if (mem != NULL)
					Light_InitSpot(mem, (unsigned char)i0, (unsigned char)i1, (unsigned char)i2,
						v, w, 10.0f / (f[0] * f[0]), f[1]);      /* 0x002536d8 */
			}
			else if (LL_IS("POINTLIGHT"))
			{
				LL_SCAN("%d %d %d %f %f %f %f", &i0, &i1, &i2, &v[0], &v[1], &v[2], &f[0]);
				mem = MB_NEW(0xe8, LL_FILE, 0x769);
				if (mem != NULL)
					Light_InitPoint(mem, (unsigned char)i0, (unsigned char)i1, (unsigned char)i2,
						v, 10.0f / (f[0] * f[0]));
			}
			else if (LL_IS("INTRO_CAM_LOW_DETAIL") || LL_IS("INTRO_CAM_HIGH_DETAIL"))
			{
				k = LL_IS("INTRO_CAM_LOW_DETAIL");
				if (MB_I32(DAT_003055d4, 0x2a8) == 0)
				{
					DAT_0030a098 = 1;
					for (i4 = 0; i4 < 4; i4++)
					{
						if (DAT_002f289c[i4] != 0)
							MB_VFN(DAT_002f289c[i4], 0x40, void (*)(int, int, int))(
								DAT_002f289c[i4], k, 0x22);
					}
				}
				else
				{
					MB_I32(DAT_003055d4, 0x27c + MB_I32(DAT_003055d4, 0x2a8) * 4) = k;
				}
			}
			else if (LL_IS("INTRO_CAM"))
			{
				LL_SCAN("%s", tok);
				if (MB_I32(DAT_003055d4, 0x568) == 2)
				{
					b1 = (char *)(intptr_t)(MB_I32(DAT_003055d4, 0x2a8) * 0x40 + DAT_003055d4);
					MB_I32(DAT_003055d4, 0x2a8) = MB_I32(DAT_003055d4, 0x2a8) + 1;
					strcpy(b1, tok);
					MB_I32(DAT_003055d4, 0x27c + MB_I32(DAT_003055d4, 0x2a8) * 4) = 0;
					MB_I32(DAT_003055d4, 0x2ac) = MB_I32(DAT_0030564c, 0x74);
				}
				else
				{
					DAT_0030a098 = 0;
					MB_I32(DAT_003055d4, 0x2a8) = 0;
					camData = bkLoadFile((struct bkPackage *)MB_PTR(DAT_0030564c, 0x74), tok, NULL,
						&camSize, NULL, 0);
					if (camData == NULL)
					{
						msg = "InitialiseCamera - Could not load camera LOM file [%s]\n";
					}
					else
					{
						map = ParseMap_Load((char *)camData, 2, (int)camSize);
						if (map != NULL)
						{
							CLIntroCam_Load((int)map, (undefined4)DAT_003055d4);
							ParseMap_Free(map);
							FUN_000d7420(DAT_003055d4, 0, 2);
							goto next;
						}
						msg = "InitialiseCamera - Error parsing camera LOM file [%s]\n";
					}
					bkPrintf(msg, tok);
				}
			}
			else if (LL_IS("WINNER_CAM"))
			{
				MB_PTR(DAT_0030564c, 0x94) = bkHeapCalloc(0xa0, 0, LL_FILE, 0x7b6, 0x2001);
				if (MB_PTR(DAT_0030564c, 0x94) == NULL)
					bkPrintf("Couldn't create winner camera info struct\n");
				for (k = 0; k < 8; k++)
				{
					obj = (char *)MB_PTR(DAT_0030564c, 0x94);
					/* the first triple at +0, the others at +0x20, +0x30, ... */
					i4 = k == 0 ? 0 : 0x10 + k * 0x10;
					LL_SCAN("%f %f %f", MB_FP(obj, i4), MB_FP(obj, i4 + 4), MB_FP(obj, i4 + 8));
				}
			}
			else if (LL_IS("LANDSCAPE"))
			{
				LL_SCAN("%s %f %f %f", tok, &v[0], &v[1], &v[2]);
				actor = (char *)Actor_LoadAny(tok, v, 0, NULL, 0);
			}
			else if (LL_IS("BACKGROUND"))
			{
				LL_SCAN("%s %f %f %f", tok, &v[0], &v[1], &v[2]);
				actor = (char *)Actor_LoadAny(tok, v, 4, NULL, 0);
			}
			else if (LL_IS("SCALE"))
			{
				LL_SCAN("%f %f %f", MB_FP(actor, 500), MB_FP(actor, 0x1f8), MB_FP(actor, 0x1fc));
				Core_Vec4Copy(MB_FP(actor, 0xe0), MB_FP(actor, 500));
				baPrepActorInstance((struct BActorInstance *)MB_ADDR(actor, 0x10), 1);
			}
			else if (LL_IS("ROTATE"))
			{
				LL_SCAN("%f %f %f", MB_FP(actor, 0x1e4), MB_FP(actor, 0x1e8), MB_FP(actor, 0x1ec));
				CLQuat_FromEuler(MB_FP(actor, 0x204), MB_FP(actor, 0x1e4), 0.0f);
				Core_Vec4Copy(MB_FP(actor, 0xf0), MB_FP(actor, 0x204));
				baPrepActorInstance((struct BActorInstance *)MB_ADDR(actor, 0x10), 1);
			}
			else if (LL_IS("ANIMATE"))
			{
				LL_SCAN("%s %d %d %f %f", tok, &i0, &i1, &f[0], &f[1]);
				i3 = (int)strlen(tok);
				i2 = (int)((unsigned int (*)(const char *, int, unsigned int))FUN_0004dbf0)(tok, i3,
					0);
				MB_DUAL(FUN_001517e0, void (*)(int, undefined4, int, int, undefined4, float),
					void (*)(void *, unsigned int, int, int, float, float),
					((int)MB_ADDR(platform, 0x10), (undefined4)i2, i0, i1, mb_fbits(f[0]), f[1]),
					(MB_ADDR(platform, 0x10), (unsigned int)i2, i0, i1, f[0], f[1]));
			}
			else if (LL_IS("SPRING"))
			{
				LL_SCAN("%f %f %f %d", &f[0], &f[1], &f[2], &i0);
				MB_F32(platform, 0x42c) = f[0];
				MB_F32(platform, 0x430) = f[1];
				MB_F32(platform, 0x434) = f[2];
				if (i0 != 0)
					MB_U32(platform, 0x450) = MB_U32(platform, 0x450) | 0x200;
			}
			else if (LL_IS("PUSHER"))
			{
				LL_SCAN("%f %f %f %f", &f[0], &f[1], &f[2], &f[3]);
				MB_F32(platform, 0x42c) = f[0];
				MB_F32(platform, 0x430) = f[1];
				MB_F32(platform, 0x434) = f[2];
				MB_F32(platform, 0x438) = f[3];
				MB_U32(platform, 0x450) = MB_U32(platform, 0x450) | 0x1000;
			}
			else if (LL_IS("SKIP_MINI_GAMES"))
			{
				if (DAT_003055cc == 0)
					DAT_003055cc = 1;
			}
			else if (LL_IS("NEW_VENT"))
			{
				LL_SCAN("%s %d %d %d %d %d %f", tok, &i0, &i1, &i2, &i3, &i4, &f[0]);
				obj = (char *)ll_debris_type(tok);
				if (obj == NULL)
				{
					bkPrintf("ERROR can't find deb type %s\n", tok);
				}
				else
				{
					mem = MB_NEW(0xe4, LL_FILE, 0x833);
					if (mem == NULL)
						vent = NULL;
					else
						vent = CMetaVent_Init((struct CMetaVent *)mem, obj,
							(int)(((i0 & 0xff) * 0x100 + (i1 & 0xff)) * 0x100 + (i2 & 0xff)),
							(short)i3, (short)i4, f[0]);
				}
			}
			else if (LL_IS("VENT_DEBRIS"))
			{
				LL_SCAN("%s %d %d %d %f", tok, &i0, &i1, &i2, &f[0]);
				if (vent != NULL)
				{
					obj = (char *)ll_debris_type(tok);
					if (obj != NULL)
						CMetaVent_AddType(vent, obj,
							(int)(((i0 & 0xff) * 0x100 + (i1 & 0xff)) * 0x100 + (i2 & 0xff)), f[0]);
				}
			}
			else if (LL_IS("VENT_POS"))
			{
				LL_SCAN("%f %f %f %f %f", &v[0], &v[1], &v[2], &f[0], &f[1]);
				if (vent != NULL)
					CMetaVent_SetPosition(vent, v, f[0], f[1]);
			}
			else if (LL_IS("VENT_VEL"))
			{
				LL_SCAN("%f %f %f %f %f", &v[0], &v[1], &v[2], &f[0], &f[1]);
				if (vent != NULL)
					CMetaVent_SetVelocity(vent, v, f[0], f[1]);
			}
			else if (LL_IS("VENT_PATTERN"))
			{
				LL_SCAN("%d %d", &i0, &i1);
				if (vent != NULL)
					CMetaVent_AddCycle(vent, (short)i0, (short)i1);
			}
			else if (LL_IS("VENT_HAZARD"))
			{
				LL_SCAN("%f %f %s", &f[0], &f[1], tok);
				if (vent != NULL)
				{
					flags = LevelLoad_ParseFlags(tok, DAT_002b262c);
					CMetaVent_SetHazard(vent, f[0], f[1], flags);
				}
			}
			else if (LL_IS("VENT_HAZARD_DIRECTION"))
			{
				LL_SCAN("%f %f %f", &f[0], &f[1], &f[2]);
				if (vent != NULL)
					CMetaVent_SetHazardDirection(vent, f[0], f[1], f[2]);
			}
			else if (LL_IS("VENT_FAN"))
			{
				LL_SCAN("%f %f %f %f %f %f %s", &v[0], &v[1], &v[2], &f[0], &f[1], &f[2], tok);
				if (vent != NULL)
				{
					flags = LevelLoad_ParseFlags(tok, DAT_002b262c);
					CMetaVent_SetPush(vent, v, f[0], f[1], f[2], flags);
				}
			}
			else if (LL_IS("VENT_PLATFORM"))
			{
				LL_SCAN("%d", &i0);
				/* a platform group; resolved to the platform after the file */
				MB_I32(vent, 0xe0) = i0;
			}
			else if (LL_IS("TRAPDOOR"))
			{
				LL_SCAN("%d", &i0);
				if (platform != NULL)
				{
					if (i0 == 0)
					{
						MB_U32(platform, 0x450) = MB_U32(platform, 0x450) | 0x800;
					}
					else
					{
						i2 = (int)bkCRCString("3open");
						MB_DUAL(FUN_001517e0, void (*)(int, undefined4, int, int, undefined4, float),
							void (*)(void *, unsigned int, int, int, float, float),
							((int)MB_ADDR(platform, 0x10), (undefined4)i2, 0, 0, 0x3f800000u, 0.0f),
							(MB_ADDR(platform, 0x10), (unsigned int)i2, 0, 0, 1.0f, 0.0f));
						MB_U32(platform, 0x450) = MB_U32(platform, 0x450) | 0x400;
					}
				}
			}
			else if (LL_IS("LIGHTNING"))
			{
				LL_SCAN("%f %f %f %f %d %d", &v[0], &v[1], &v[2], &f[0], &i0, &i1);
				CLightningMgr_AddBolt(&DAT_00308460, v, f[0], (short)i0, (unsigned short)i1);
			}
			else if (LL_IS("BOMB_POS"))
			{
				LL_SCAN("%f %f %f %d %d", &v[0], &v[1], &v[2], &i0, &i1);
				MbBombGroup_AddPosition(v, (short)i0, (unsigned int)i1);
			}
			else if (LL_IS("MAKES_DUST"))
			{
				if (platform != NULL)
					MB_U32(platform, 0x450) = MB_U32(platform, 0x450) | 0x2000;
			}
			else if (LL_IS("CAMERA_ALIGNED_SPRITE"))
			{
				LL_SCAN("%f %f %f %f %f %s %d %d %d %d %f %d", &g[0], &g[1], &g[2], &spriteF[0],
					&spriteF[1], tok, &spriteInts[0], &spriteInts[1], &spriteInts[2],
					&spriteInts[3], &spriteF[2], &spriteFlag);
				sprite = (char *)(intptr_t)MB_DUAL(FUN_0015c400,
					int (*)(undefined4 *, undefined4, undefined4, undefined4, undefined4,
						undefined4, undefined4, undefined4, undefined4),
					int (*)(float *, float, float, char *, int, int, int, int, float),
					((undefined4 *)g, mb_fbits(spriteF[0]), mb_fbits(spriteF[1]), (undefined4)tok,
						(undefined4)spriteInts[0], (undefined4)spriteInts[1],
						(undefined4)spriteInts[2], (undefined4)spriteInts[3], mb_fbits(spriteF[2])),
					(g, spriteF[0], spriteF[1], tok, spriteInts[0], spriteInts[1], spriteInts[2],
						spriteInts[3], spriteF[2]));
				MB_I32(sprite, 0x3c) = spriteFlag;
			}
			else if (LL_IS("SHADOW"))
			{
				LL_SCAN("%s %f %f %f", tok, &f[0], &f[1], &f[2]);
				i0 = LevelLoad_Lookup(tok, DAT_002b2658);
				if (i0 >= 0)
					MB_DUAL(FUN_000be900, void (*)(int, undefined4, undefined4, undefined4),
						void (*)(int, float, float, float),
						(i0, mb_fbits(f[0]), mb_fbits(f[1]), mb_fbits(f[2])),
						(i0, f[0], f[1], f[2]));
			}
			else if (LL_IS("SHADOW_XROT_RANGE"))
			{
				LL_SCAN("%f %f", &f[0], &f[1]);
				MB_DUAL(FUN_000be440, void (*)(undefined4, undefined4), void (*)(float, float),
					(mb_fbits(f[0]), mb_fbits(f[1])), (f[0], f[1]));
			}
			else
			{
				bkPrintf("******** LND ERROR *********\nstring '%s' not recognized\n\n", tok);
			}
next:
			end = text + size;
			line = cur;
		}
		MB_DELETE(tok);
		MB_DELETE(tok2);
	}
	MB_DELETE(text);
	LevelLoad_FinishTracks(platform, searchlight);
	/* the defines are not needed any more */
	obj = MB_LIST_NEXT(DAT_0030a09c, NULL);
	while (obj != NULL)
	{
		mem = MB_LIST_REMOVE(DAT_0030a09c, obj);
		MB_DELETE(obj);
		obj = MB_LIST_NEXT(DAT_0030a09c, mem);
	}
	/* vents on platforms: the group becomes the platform and the offset */
	for (vent = MB_LIST_NEXT(&DAT_00308e60, NULL); vent != NULL;
		vent = MB_LIST_NEXT(&DAT_00308e60, vent))
	{
		if (vent->parent == NULL)
			continue;
		vent->parent = Platform_Find((int)(intptr_t)vent->parent);
		if (vent->parent != NULL)
		{
			vent->offset[0] = vent->pos[0] - MB_F32(vent->parent, 0xe0);
			vent->offset[1] = vent->pos[1] - MB_F32(vent->parent, 0xe4);
			vent->offset[2] = vent->pos[2] - MB_F32(vent->parent, 0xe8);
		}
	}
	FUN_00159410();
	FUN_001594e0();
	FUN_001592e0(DAT_003082c0);
	(void)platformCopy;
	return loaded;
}
