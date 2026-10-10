/*
Demolition/demolition_a_data.c

The .data tables of the Demolition game that this module's code reads
(0x00297510-0x00297870, values copied from orig/default.xbe), and the game
object pointer (.bss 0x002ff85c).
*/

#include "demolition_a.h"

/* 0x002ff85c: the game object (CDemolition_Game), made by CModuleDemolition_Init */
void *DAT_002ff85c;

/* 0x00285644: the engine's collision epsilon (0.001; Demolition's physics sets it to 0.05 before
   each sweep). engine_misc owns it but does not define it yet: weak, so the owner's definition
   wins. */
__attribute__((weak)) float DAT_00285644 = 0.001f;

/* 0x00297510: the level of each variation (the world loads "<name>\..." files) */
const char *const DAT_00297510[4] = { "frag", "remote", "mine", "cross" };

/* 0x00297520: per variation settings */
struct DA_VariationSettings DAT_00297520[4] = {
	/* 0 frag */
	{ 1, 1, 0, 1, 10, 10, 45, 5000.0f, 20000.0f, 100, 110, 127, 10, 10, 45 },
	/* 1 remote */
	{ 1, 1, 0, 1, 0, 10, 20, 5000.0f, 35000.0f, 127, 120, 96, 0, 10, 20 },
	/* 2 mine */
	{ 1, 1, 0, 1, 0, 16, 21, 5000.0f, 20000.0f, 127, 127, 127, 0, 16, 21 },
	/* 3 cross */
	{ 1, 1, 0, 1, 0, 0, 0, 0.0f, 14000.0f, 64, 64, 64, 0, 0, 0 },
};

/* 0x00297750: the sound effects */
const char *const DAT_00297750[DA_SFX_COUNT] = {
	"sfx\\explosion01.wav", "sfx\\explosion02.wav", "sfx\\explosion03.wav", "sfx\\explosion04.wav",
	"sfx\\explosion05.wav", "sfx\\bombbeep.wav", "sfx\\bombleftfoot.wav", "sfx\\bombrightfoot.wav",
	"sfx\\token.wav", "sfx\\cratesmash1.wav", "sfx\\cratesmash2.wav", "sfx\\teleportin.wav",
	"sfx\\teleportout.wav", "sfx\\streetamb1.wav", "sfx\\heavylift1.wav", "sfx\\liftcogstart.wav",
	"sfx\\liftcog.wav", "sfx\\liftloop1.wav", "sfx\\liftscrape_lp.wav", "sfx\\platformfall_lp.wav",
	"sfx\\throw.wav", "sfx\\bombdrop1.wav", "sfx\\gasflame.wav", "sfx\\indusamb.wav",
	"sfx\\gflameindividual.wav",
};

/* 0x002977b4: the crate models */
const char *const DAT_002977b4[6] = {
	"models\\crate.obe", "models\\crate_coll.obe", "models\\crate_shard01.obe",
	"models\\crate_shard02.obe", "models\\crate_shard03.obe", "models\\crate_shard04.obe",
};

/* 0x002977cc: the crate's animating collision */
const char *const DAT_002977cc[1] = { "models\\crate_ac.obe" };

/* 0x002977d0: the hazard light bases */
const char *const DAT_002977d0[3] = {
	"models\\hazbase01.obe", "models\\hazbase02.obe", "models\\hazbase03.obe",
};

/* 0x002977e0: the main light of each variation (a direction, or a position for cross) */
struct DA_Vec4 DAT_002977e0[4] = {
	{ 1.0f, 1.4f, 0.0f, 0.0f },
	{ 1.0f, 1.4f, 0.0f, 0.0f },
	{ 1.0f, 1.4f, 0.0f, 0.0f },
	{ 0.0f, 1000.0f, 0.0f, 0.0f },
};

/* 0x00297820: the textures */
const char *const DAT_00297820[DA_TEXTURE_COUNT] = {
	"spotfx\\sprites\\sparkburst.bmp", "spotfx\\sprites\\sparkburst01.bmp",
	"spotfx\\sprites\\explosion.bmp", "spotfx\\sprites\\scorch.bmp", "spotfx\\sprites\\corona01.bmp",
	"spotfx\\sprites\\corona02.bmp", "spotfx\\sprites\\shockwave.bmp", "spotfx\\sprites\\shadow.bmp",
	"spotfx\\sprites\\brakespark.bmp", "spotfx\\sprites\\glow.bmp", "spotfx\\sprites\\gasjet.bmp",
	"spotfx\\sprites\\steam_02.bmp", "tex\\brickcolour.bmp", "tex\\brickcolour01.bmp",
	"tex\\brickcolour02.bmp", "tex\\brickcolour03.bmp", "tex\\brickcolour04.bmp",
	"tex\\brickcolour05.bmp", "tex\\brickcolour06.bmp", "tex\\brickcolour07.bmp",
};
