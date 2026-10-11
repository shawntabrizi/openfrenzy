/*
FoxHunt/unplaced_sound.c  (module foxhunt; gap functions 0x00133db0-0x00134400 of
C:\BlitzParty\Game\Src\FoxHunt\, file unknown)

The speech and sound effects (FHunt_Sound, 0xa8 bytes, at 0x003022c8: 21 sample names and
their samples), and the effects object at 0x003022cc (debris, sprites and the extra-item
system whose methods are in the hopper range 0x0013b140-0x0013b910), with the debris
ground callback. Also the loader of the three Fox Hunt prop actors.
*/

#include "foxhunt.h"

extern unsigned char DAT_00290f00[];  /* bumpercars_a: the "aisteam1" sprite defs (6) */
extern unsigned char DAT_00290ea8[];  /* classlib_fx: the debris vertex buffer */
extern char *DAT_002ba96c;            /* core: the game (+0x18 stage) */
extern char *DAT_002ed34c;            /* classlib_ui: the debris system */

/* the sound names of FHunt_Sound; [0] and [4] are the words at 0x0026cc3c and 0x0026d980 */
static const char *const s_SoundNames[21] = {
	"rain", "foxh_01a", "splash", "stealflag", "pick", "punchswipe2", "dest_02a",
	"kickswipe1", "lightening", "countdown2", "escalator", "taunt", "smash", "tag_11a",
	"tag_08a", "countdown1", "tag_07b", "tag_02b", "punchimpact1", "punchimpact2",
	"kickimpact2",
};

/* 0x00133db0  (gap): FHunt_Sound::FHunt_Sound: fills the 21 sample names */
void *FHunt_Sound_Ctor(void *self)
{
	int i;

	for (i = 0; i < 21; i++)
		FH_AP(self, i * 4) = (char *)s_SoundNames[i];
	return self;
}

/* 0x00133e50  (gap): FHunt_Sound::Initialise: loads "<name>.wav" of every sound from
`package`. Returns 1. */
int FHunt_Sound_Initialise(void *self, void *package)
{
	char name[512];
	char *sample;
	int i;

	for (i = 0; i < 0x15; i++) {
		_snprintf(name, 0x1ff, "%s.wav", FH_AP(self, i * 4));
		sample = (char *)bLoadResource((struct bkPackage *)package, name, 2, 0xdefa);
		FH_AP(self, 0x54 + i * 4) = sample;
		if (sample == NULL) {
			bkPrintf("FHunt_Sound::Initialise - can't load sound sample #%d [%s]\n", i, FH_AP(self, i * 4));
			bkPrintf("%s\n", name);   /* format at 0x00262b54 */
		}
	}
	return 1;
}

/* 0x00133ee0  (gap): FHunt_Sound::Play: plays sound `id` at `volume` (0-255; negative: not
played), `pitch` added to the sample's rate. When no voice is free, stops channel id % 64
and plays it there. Returns the channel or -1. */
int FHunt_Sound_Play(void *self, unsigned int id, int volume, int pitch)
{
	char *sample = FH_AP(self, 0x54 + id * 4);
	int frequency = 0;
	int channel;
	int result;

	if (sample == NULL)
		return -1;
	if (volume < 0)
		return -1;
	if (volume > 0xff)
		volume = 0xff;
	if (pitch != 0)
		frequency = FH_AI(sample, 0x20) + pitch;
	result = bsPlaySample((struct BSample *)sample, volume, 0, frequency, 0);
	if (result == -1) {
		channel = (int)id % 64;
		bsStopChannel(channel);
		result = bsPlaySampleOnChannel(channel, (struct BSample *)FH_AP(self, 0x54 + id * 4), volume, 0, frequency, 1);
	}
	return result;
}

/* 0x00133f80  (gap): constructor of the effects object at 0x003022cc (core's static
constructor calls it; it does nothing) */
void *FoxHunt_Effects_Ctor(void *self)
{
	Meta_ReturnThis(self);
	return self;
}

/* 0x00133f90  (not in the function table; called by the game mode's exit): frees the
effects: the two sprite def sets, the debris, the extra items and the debris vertex
buffer */
void FUN_00133f90(void *self)
{
	Sprite_FreeDefs((struct CSpriteDef *)FH_ADDR(0x002ac1c8), 0xc);
	Sprite_FreeDefs((struct CSpriteDef *)DAT_00290f00, 6);
	CLDebris_Start();
	FUN_0013b460((struct HopSparks *)self);
	SpriteVB_Release((struct CSpriteVB *)DAT_00290ea8);
}

/* 0x00133fd0  (gap): advances the debris and the extra items by `dt` */
void FoxHunt_Effects_Update(void *self, float dt)
{
	Debris_UpdateSystems(dt);
	FH_FN(FUN_0013b480, void, (void *, float))(self, dt);
}

/* 0x00133ff0  (gap): draws the debris and the extra items (called every frame by the draw) */
void FoxHunt_Effects_Draw(void *self)
{
	CLDebris_Stop();
	FUN_0013b4b0((struct HopSparks *)self);
}

/* 0x00134000  (gap, stdcall; the callers' ECX is unused): adds a debris particle of type
`type` (the table at 0x002ac8f0, 0x2c bytes per type). Returns the piece. */
char *FoxHunt_AddDebris(int type, float *pos, float *vel, int life, unsigned int colour)
{
	return (char *)Debris_Add((struct DebrisType *)(FH_A(0x002ac8f0) + type * 0x2c), pos, vel, life, colour);
}

/* 0x00134030  (gap): a jump to hopper's 0x0013b140 (adds an extra item: a sprite at `pos`
moving by `vel`) */
void FUN_00134030(undefined4 *self, float *pos, float *vel, float a, undefined4 r, undefined4 g,
	undefined4 b, undefined4 alpha, float size)
{
	FUN_0013b140((struct HopSparks *)self, pos, vel, a, r, g, b, alpha, size);
}

/* 0x00134040  (not in the function table; the debris ground callback given to Debris_Init
and hopper's 0x0013b910): casts the ray `pos` + `vel` against the level. Returns the hit
fraction with the hit point in `out` and the normal in `normal`; the parts of the level
above y 499.9 in two boxes do not count. Without a hit, below the water level (stage > 2:
the sea at y -10 near (0, -500), else the harbour water at y -299) the ray ends on the
water with a splash. */
float FUN_00134040(float *pos, float *vel, float *out, float *normal)
{
	char *tri = NULL;
	float t;
	float water[4];
	float f;
	int stage;

	pos[3] = 1.0f;
	vel[3] = 1.0f;
	out[3] = 1.0f;
	if (vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2] == 0.0f)
		return 0.0f;
	t = CRColl_RayCast(pos, vel, out, (float **)&tri, NULL);
	if (t != 0.0f && pos[1] > 499.9f) {
		if (pos[0] > -899.0f && pos[0] < 699.0f && pos[2] > 920.0f && pos[2] < 1200.0f)
			return 0.0f;
		if (pos[0] > -1196.0f && pos[0] < -935.0f && pos[2] > -1150.0f && pos[2] < 891.0f)
			return 0.0f;
	}
	if (t > 0.0f) {
		normal[0] = FH_AF(tri, 0x30);
		normal[1] = FH_AF(tri, 0x34);
		normal[2] = FH_AF(tri, 0x38);
		normal[3] = 1.0f;
		if (isnan((double)normal[0]) || isnan((double)normal[1]) || isnan((double)normal[2])) {
			/* DbgBreakPoint() (0x0003097d): a debugger break, nothing in the port */
		}
		return t;
	}
	stage = FH_AI(DAT_002ba96c, 0x18);
	if (stage > 2) {
		if (!(vel[1] < 0.0f))
			return 0.0f;
		water[0] = 0.0f;
		water[1] = -10.0f;
		water[2] = -500.0f;
		water[3] = 1.0f;
		if (bmVectorDistance(water, pos) >= 250.0f)
			return t;
		cmb_vec3_add(out, pos, vel);
		if (out[1] >= water[1])
			return t;
		f = (water[1] - pos[1]) / vel[1];
		cmb_vec3_scale(out, vel, f);
		cmb_vec3_add(out, out, pos);
		normal[0] = 0.0f;
		normal[1] = 1.0f;
		normal[2] = 0.0f;
		normal[3] = 1.0f;
		FH_VFN(FH_P(0x0030214c), 0x28, int (*)(void *, float, float, float, float, float))(
			FH_P(0x0030214c), out[0], out[2], -4.0f, 5.0f, 10.0f);
		return f;
	}
	if (stage >= 3)
		return t;
	if (!(vel[1] < 0.0f))
		return 0.0f;
	cmb_vec3_add(out, pos, vel);
	if (out[1] >= -299.0f)
		return t;
	f = (-299.0f - pos[1]) / vel[1];
	cmb_vec3_scale(out, vel, f);
	cmb_vec3_add(out, out, pos);
	normal[0] = 0.0f;
	normal[1] = 1.0f;
	normal[2] = 0.0f;
	normal[3] = 1.0f;
	FH_VFN(FH_P(0x0030214c), 0x28, int (*)(void *, float, float, float, float, float))(
		FH_P(0x0030214c), pos[0], pos[2], -2.5f, 2.5f, 3.0f);
	return f;
}

/* 0x00134360  (gap): sets up the effects for `variation`: the sprite defs, the debris
(2000 items, ground callback 0x00134040) and the extra items; in the town (variations 1-2)
the debris drifts in the wind (0.1, 0, 0.1). Returns 0 when the extra items fail. */
int FoxHunt_Effects_Init(void *self, int unused, int variation)
{
	float wind[4];

	(void)unused;
	Sprite_LoadDefs((struct CSpriteDef *)FH_ADDR(0x002ac1c8), 0xc, 0);
	Sprite_LoadDefs((struct CSpriteDef *)DAT_00290f00, 6, 0);
	Debris_Init((void *)FUN_00134040, 2000);
	if (FH_FN(FUN_0013b910, int, (void *, void *))(self, (void *)FUN_00134040) == 0)
		return 0;
	if (variation < 3) {
		wind[0] = 0.1f;
		wind[1] = 0.0f;
		wind[2] = 0.1f;
		wind[3] = 1.0f;
		DebrisSys_SetWindDirection((struct DebrisSysData *)FH_AP(DAT_002ed34c, 0x40), wind);
	}
	return 1;
}

/* 0x00134400  (gap): loads the actors of the prop table at 0x002adf38 (3 pairs of name and
actor: chairs and the like) */
void FoxHunt_LoadPropActors(void)
{
	char **entry;
	char *actor;

	bkPrintf("Loadeding object actor \n");
	for (entry = (char **)FH_ADDR(0x002adf38); entry < (char **)FH_ADDR(0x002adf50); entry += 2) {
		actor = (char *)bLoadResource(NULL, entry[0], 1, 0);
		entry[1] = actor;
		if (actor == NULL)
			bkPrintf("Failed to load actor %s\n", entry[0]);
		else
			bkPrintf("Loaded actor %s\n", entry[0]);
	}
}
