/*
Core/CGameMode_GameWinner.c  <-  C:\BlitzParty\Game\Src\Core\CGameMode_GameWinner.cpp

The end of a party: the four characters on the podium, a camera per place
(podiumcam_<n>.lom), the place texts, fireworks with whistles and bangs, the
crowd cheering, then a fade back. The constructor (0x0001a7e0) is unplaced in
the table; it is this class's. 0x0001b700 (the fireworks' explosion callback)
is not in the function table.
*/

#include <wchar.h>
#include "core_modes_a.h"
#include "core_modes_a_modes.h"
#include "core_modes_a_globals.h"

/* CGameMode_GameWinner (vtable 0x00255260) */
struct CGameMode_GameWinner {
	struct CGameModeMenu menu;          /* +0x000 */
	float timer;                        /* +0x098 since the last camera */
	float cheerDelay;                   /* +0x09c */
	void *fireworks;                    /* +0x0a0 classlib_ui CFireworks */
	struct CBGTypeText *placeText;      /* +0x0a4 */
	struct CBGTypeText *winnerText;     /* +0x0a8 */
	int leaving;                        /* +0x0ac */
	float fade;                         /* +0x0b0 */
	float elapsed;                      /* +0x0b4 */
	int voice;                          /* +0x0b8 announcer: -1 none, -2 played, -3 crowd, -4 end */
	int crowdVoice;                     /* +0x0bc */
	int camStage;                       /* +0x0c0 1-5 the places, 6 done */
	unsigned char lights[2][0x70];      /* +0x0c4 bactor lights */
	int package;                        /* +0x1a4 Packages\GameWinner */
	int podium;                         /* +0x1a8 */
	int background;                     /* +0x1ac */
	int actors[4];                      /* +0x1b0 in place order */
	int unk1c0[4];                      /* +0x1c0 */
	float eye[4];                       /* +0x1d0 */
	float target[4];                    /* +0x1e0 */
};
OF_ASSERT_SIZE(struct CGameMode_GameWinner, 0x1f0);

/* core_modes_b's score table (CScoreTable at 0x002b9968): +0x2d0 bonus[4], +0x2f0 by_place[4] */
extern int DAT_002b9968[0xc4];
#define CMA_SCORE_BONUS(p) DAT_002b9968[0xb4 + (p)]
#define CMA_SCORE_BY_PLACE(i) DAT_002b9968[0xbc + (i)]
extern int DAT_002ed218;            /* classlib_ui: an intro camera is playing */

/* ---------- data (0x00283668-0x0028375c) */

static const float DAT_00283668 = 1000.0f;     /* the crowd stops cheering after */
static const float DAT_0028366c = 60.0f;       /* delay between cheers */
static const char *const PTR_s_wheee1_wav_00283670[3] = { "wheee1.wav", "whizzz1.wav", "whizzz2.wav" };
static const char *const PTR_s_bang1_wav_0028367c[3] = { "bang1.wav", "bang2.wav", "bang3.wav" };
static const char *const PTR_s_crackles1_wav_00283688[6] = {
	"crackles1.wav", "crackles2.wav", "crackles3.wav", "crackles4.wav", "crackles5.wav", "crackles6.wav",
};
static const float DAT_002836a0 = 10.0f;       /* a new text's scale */
static const float DAT_002836a4 = 0.4f;        /* its shrink rate */
static const float DAT_002836a8 = 5.0f;        /* its fade-in rate */
static float DAT_002836ac[4] = { 0.0f, 800.0f, 1000.0f, 1.0f }; /* fireworks launch point */
/* the animations of the place shown at camera stage s (0x002836d0 - 4 s, 0x002836e0 - 4 s) */
static const int s_placeAnim[6] = { 0, 0x23, 0x1c, 0x1c, 0x1c, 0x1b };
static const int s_placeIdleAnim[6] = { 0x24, 0, 0x24, 0x24, 0x24, 0x23 };
/* 0x002836e0: the podium positions (first to fourth) */
static const float DAT_002836e0[16] = {
	20.0f, 1140.0f, 580.0f, 0.0f, 430.0f, 1050.0f, -350.0f, 0.0f,
	-460.0f, 1000.0f, -350.0f, 0.0f, 20.0f, 1000.0f, -580.0f, 0.0f,
};
static const uint32_t DAT_00283720 = 0x00322846;  /* ambient light, 0x00bbggrr */
static const unsigned char DAT_00283724[8] = { 0x28, 0x3c, 0x28, 0x00, 0x7f, 0x5a, 0x3c, 0x00 };
static float DAT_0028372c[8] = { 495.0f, 1328.0f, 1588.0f, 0.0f, -746.0f, 1395.0f, -1557.0f, 0.0f };
static const float DAT_00283750[4] = { 0.0f, 2.3561945f, -2.3561945f, 3.1415925f };

/* ---------- bss (0x002b9730-0x002b9968) */

int DAT_002b9730[3];                /* whistle samples */
int DAT_002b973c[6];                /* crackle samples */
int DAT_002b9754[3];                /* bang samples */
float DAT_002b9760;                 /* time to the next rocket */
wchar16 DAT_002b9768[0x100];        /* the place text */

static const char s_GameWinner_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_GameWinner.cpp";

/* classlib_ui CFireworks (thiscall): constructor, update, render, launch, explode */
/* CLFireworks_Init and CLFireworks_Launch take their sizes, lives and speeds as raw float
   bits (the original pushes 0x42480000 etc.); a float argument would go in a float register. */
#define CMA_Fireworks_ctor(mem, count, tex1, size1, life, tex2, size2) \
	((void *)CLFireworks_Init((int *)(mem), (count), (uint)(tex1), (int)cma_fbits(size1), (life), \
		(uint)(tex2), cma_fbits(size2)))
#define CMA_Fireworks_Update(fw, dt) \
	CMA_DUAL(FUN_0007fdc0, void (*)(int, undefined4), void (*)(void *, float), \
		((int)(fw), cma_fbits(dt)), ((fw), (dt)))
#define CMA_Fireworks_Render(fw) ((void (*)(void *))FUN_0007fe60)(fw)
#define CMA_Fireworks_Launch(fw, pos, vel, life, r, g, b, n, size, speed) \
	CLFireworks_Launch((int)(fw), (undefined4 *)(pos), (undefined4 *)(vel), cma_fbits(life), (r), (g), \
		(b), (n), cma_fbits(size), cma_fbits(speed))
#define CMA_Fireworks_Explode(fw, rocket) ((void (*)(void *, void *))FUN_0007fc30)((fw), (rocket))
/* classlib_ui FUN_00072440: a fade (6 arguments) */
#define CMA_Fade(a, b, c, d, e, f) FUN_00072440((a), (b), (c), (d), (e), (f))

/* 0x00255260 CGameMode_GameWinner */
const void *const vtable_00255260[] = {
	(const void *)FUN_0001d1e0,     /* core_modes_b: deleting destructor of menu modes */
	(const void *)FUN_001d5950,
	(const void *)CGameMode_GameWinner_Resume,
	(const void *)CGameMode_GameWinner_Popped,
	(const void *)CGameMode_GameWinner_Pushed,
	(const void *)FUN_0002fe60,
	(const void *)CGameMode_GameWinner_Update,
	(const void *)CGameMode_GameWinner_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)CGameMode_GameWinner_LoadingStarted,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_FullGame_HandleButton,
};

/* 0x0001a7e0  Core (unplaced; CGameMode_GameWinner) */
struct CGameMode_GameWinner *CGameMode_GameWinner_ctor(struct CGameMode_GameWinner *self)
{
	FUN_00030090((undefined4 *)self, (undefined4 *)"GameWinner");
	self->menu.base.flags |= 3;
	self->menu.repeatRate[6] = 100000000;
	self->menu.repeatRate[5] = 100000000;
	self->menu.repeatRate[4] = 100000000;
	self->menu.base.vtbl = vtable_00255260;
	return self;
}

/* 0x0001a820  Core/CGameMode_GameWinner.cpp: pushed: start loading Packages\GameWinner, lights */
void CGameMode_GameWinner_Pushed(struct CGameMode_GameWinner *self)
{
	int k;

	self->cheerDelay = 0.0f;
	self->timer = 0.0f;
	self->winnerText = 0;
	self->placeText = 0;
	self->fireworks = 0;
	DAT_002b9760 = 0.0f;
	CMA_Fade(0x1b, 0xff, 0xffffff01, 0xff, 1, 0);
	self->package = CMA_LoadPackageAsync("Packages\\GameWinner", 0);
	self->menu.base.loadState = 2;
	if (DAT_002b95d0 != 0)
		DAT_002b95d0->active = 0;
	self->fade = 265.0f;
	self->leaving = 0;
	self->elapsed = 0.0f;
	FUN_00071880(5);
	self->voice = -1;
	self->crowdVoice = -1;
	for (k = 0; k < 2; k++) {
		FUN_000382f0((undefined4 *)self->lights[k], 0);
		FUN_00038550((undefined4 *)self->lights[k], (undefined4 *)&DAT_0028372c[k * 4]);
		FUN_00038470((struct BLight *)self->lights[k], DAT_00283724[k * 4], DAT_00283724[k * 4 + 1],
			DAT_00283724[k * 4 + 2]);
	}
}

/* 0x0001a910  Core/CGameMode_GameWinner.cpp: resume */
void CGameMode_GameWinner_Resume(struct CGameMode_GameWinner *self)
{
	(void)self;
	DAT_00284408 = 1;
}

/* 0x0001a920  Core/CGameMode_GameWinner.cpp: render: background, podium, characters, fireworks,
   texts, fade */
void CGameMode_GameWinner_Render(struct CGameMode_GameWinner *self, int active)
{
	int k;

	if (active == 0)
		return;
	if (self->menu.base.loadState == 0) {
		FUN_00038440(DAT_00283720 & 0xff, DAT_00283720 >> 8 & 0xff, DAT_00283720 >> 16 & 0xff);
		for (k = 0; k < 2; k++) {
			FUN_00038550((undefined4 *)self->lights[k], (undefined4 *)&DAT_0028372c[k * 4]);
			FUN_00038470((struct BLight *)self->lights[k], DAT_00283724[k * 4], DAT_00283724[k * 4 + 1],
				DAT_00283724[k * 4 + 2]);
			FUN_00038370((struct BLight *)self->lights[k], 1);
		}
		FUN_000438b0();
		FUN_00044b00(3, 0, 0);
		FUN_00044b00(8, 0, 0);
		FUN_00044b00(7, 3, 0);
		FUN_00044b00(5, 1, 0);
		FUN_00044b00(6, 0, 0);
		FUN_00043b70(1);
		FUN_00044a00(1.2217305f, 0.0f);
		CMA_SetClipPlanes(10.0f, 100000.0f);
		FUN_00034bf0((struct BActorInstance *)self->background, 1);
		FUN_00034d50((struct BActorInstance *)self->background, 7);
		FUN_00044b00(3, 1, 0);
		FUN_00034bf0((struct BActorInstance *)self->podium, 1);
		FUN_00034d50((struct BActorInstance *)self->podium, 7);
		FUN_00044b00(6, 0, 0);
		for (k = 0; k < 4; k++) {
			FUN_00034bf0((struct BActorInstance *)self->actors[k], 1);
			FUN_00034d50((struct BActorInstance *)self->actors[k], 7);
		}
		FUN_00044b00(3, 0, 0);
		FUN_00044b00(6, 1, 0);
	}
	FUN_000438b0();
	FUN_00043b70(1);
	if (self->fireworks != 0)
		CMA_Fireworks_Render(self->fireworks);
	if (self->placeText != 0)
		CMA_VFN(self->placeText, 0x0c, void (*)(void *))(self->placeText);
	if (self->winnerText != 0)
		CMA_VFN(self->winnerText, 0x0c, void (*)(void *))(self->winnerText);
	if (self->fade != 0.0f)
		FUN_00071c20((int)self->fade, 1);
}

/* 0x0001ab10  Core/CGameMode_GameWinner.cpp: animation ANIM on the character in place SLOT */
void GameWinner_PlayAnim(struct CGameMode_GameWinner *self, int slot, int anim, int loop, int keep,
	float speed, float blend)
{
	int actor = self->actors[slot];
	int sequence = (int)FUN_00035c70((struct BActorInstance *)actor, DAT_00292ca0[anim].name);

	if (keep == 0)
		FUN_00038190((void *)self->actors[slot], (struct BActorNode *)CMA_FIELD(int, self->actors[slot], 0x134), 1, 0);
	if (sequence == 0)
		return;
	actor = self->actors[slot];
	if (blend != 0.0f)
		CMA_Actor_PlayBlended(actor, CMA_FIELD(int, actor, 0x134), sequence, (int)((double)speed * 1024.0),
			-1, blend, loop != 0, 0, 0);
	else
		FUN_00035770((struct BActorInstance *)actor, (struct BActorNode *)CMA_FIELD(int, actor, 0x134), (struct BAnim *)sequence, (int)((double)speed * 1024.0),
			0xffffffff, loop != 0, 0, 0);
}

/* 0x0001abe0  Core/CGameMode_GameWinner.cpp: load camera STAGE (podiumcam_<stage>.lom) */
void GameWinner_LoadCamera(struct CGameMode_GameWinner *self, int stage)
{
	char file[32];
	int size;
	int data;
	int lom;

	self->camStage = stage;
	sprintf(file, "podiumcam_%d.lom", stage);
	data = ((int (*)(int, const char *, int, int *, int, int))FUN_0004c6b0)(self->package, file, 0, &size, 0, 0);
	if (data == 0) {
		CMA_DebugPrintf("InitialiseCamera - Could not load camera LOM file\n");
		return;
	}
	lom = (int)FUN_0007f2f0((char *)data, 2, size);
	if (lom == 0) {
		CMA_DebugPrintf("InitialiseCamera - Error parsing camera LOM file\n");
		return;
	}
	FUN_0007e680(lom, (undefined4)self);
	FUN_0007f3a0((int *)lom);
}

/* 0x0001ac80  Core/CGameMode_GameWinner.cpp: the menu modes' destructor (a jump to core's) */
void CGameModeMenu_dtor_thunk(void *self)
{
	FUN_00030190((undefined4 *)self);
}

/* a new place text at y with TEXT, big and transparent; it shrinks and fades in */
static struct CBGTypeText *GameWinner_NewText(int line, int y, unsigned short *text, int big)
{
	void *memory = CMA_New(0x90, s_GameWinner_cpp, line, 0x1001);
	struct CBGTypeText *t = memory == 0 ? 0
		: CBGTypeText_ctor(memory, -320, y, text, (int)FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa));

	CBGTypeText_Select(t, 0.0f);
	t->style |= 8;
	if (big) {
		t->buf.text.scale = DAT_002836a0;
		t->buf.text.color[3] = 0;
		t->alpha = 0.0f;
	}
	return t;
}

/* a place text shrinks to 1 (with a clunk) and fades in; returns 1 when it landed */
static int GameWinner_AnimateText(struct CBGTypeText *t)
{
	int landed = 0;

	if (t->buf.text.scale > 1.0f) {
		t->buf.text.scale = (float)((double)t->buf.text.scale - (double)DAT_002836a4 * (double)DAT_0028fda8);
		if (t->buf.text.scale < 1.0f) {
			t->buf.text.scale = 1.0f;
			CMA_PlaySample(FUN_0004bb30(0, "clunk.wav", 2, 0xdefa), 0xff, 0, 0, 0);
			landed = 1;
		}
	}
	return landed;
}

static void GameWinner_FadeText(struct CBGTypeText *t)
{
	if (t->alpha < 127.0f) {
		t->alpha = (float)((double)DAT_002836a8 * (double)DAT_0028fda8 + (double)t->alpha);
		if (t->alpha > 127.0f)
			t->alpha = 127.0f;
		t->buf.text.color[3] = (int)t->alpha;
	}
}

/* 0x0001ac90  Core/CGameMode_GameWinner.cpp: update: fireworks, cheers, texts, voices, cameras,
   the characters and the fade */
void CGameMode_GameWinner_Update(struct CGameMode_GameWinner *self, int active)
{
	char name[32];
	int i;

	if (DAT_002ecdec != 0 || active == 0)
		return;
	if (self->cheerDelay >= 0.0f)
		self->cheerDelay = self->cheerDelay - DAT_0028fda8;
	if (self->menu.base.loadState != 0) {
		CMA_VFN(self, 0x24, void (*)(void *))(self);
		return;
	}

	if (self->fireworks != 0) {
		CMA_Fireworks_Update(self->fireworks, DAT_0028fda8);
		if (self->camStage >= 5) {
			double t = (double)DAT_0028fda8 + (double)self->timer;

			self->timer = (float)t;
			if (!(t < 1200.0))
				self->leaving = 1;
			if (DAT_002b9760 > 0.0f) {
				DAT_002b9760 = DAT_002b9760 - DAT_0028fda8;
			} else if ((int)Core_Rand() % 10 == 0) {
				float vel[4];
				float r, g, b;
				int whistle;
				int pitch;

				Core_Rand();
				vel[0] = (float)((int)Core_Rand() % 21) - 10.0f;
				vel[1] = (float)((int)Core_Rand() % 20) + 15.0f;
				vel[2] = (float)((int)Core_Rand() % 6) + 5.0f;
				vel[3] = 0.0f; /* not set in the original (read by the launch) */
				r = (float)((int)Core_Rand() % 128);
				g = (float)((int)Core_Rand() % 128);
				b = (float)((int)Core_Rand() % 128);
				CMA_Fireworks_Launch(self->fireworks, DAT_002836ac, vel, 40.0f, r, g, b, 400, 14.0f, 50.0f);
				whistle = (int)Core_Rand() % 3;
				pitch = (int)Core_Rand() % 5000 + 21000;
				CMA_PlaySample(DAT_002b9730[whistle], 0x78, pitch, 0, (int)DAT_002836ac);
				DAT_002b9760 = 15.0f;
			}
			if (self->leaving == 0 && self->cheerDelay <= 0.0f && (int)Core_Rand() % 100 < 10) {
				self->cheerDelay = DAT_0028366c;
				sprintf(name, "cheer%02d.wav", (int)Core_Rand() % 4);
				CMA_PlaySample(FUN_0004bb30(0, name, 2, 0xdefa), 0xff, 0, 0, 0);
			}
		}
	}
	if (self->placeText != 0)
		CMA_VFN(self->placeText, 0x08, int (*)(void *))(self->placeText);
	if (self->winnerText != 0)
		CMA_VFN(self->winnerText, 0x08, int (*)(void *))(self->winnerText);
	self->elapsed = self->elapsed + DAT_0028fda8;

	/* second place far behind: the loser's voice */
	if (self->voice == -1 && self->camStage == 2) {
		int last = CMA_SCORE_BY_PLACE(3);

		if ((int)((double)CMA_SCORE_BONUS(last) * 1.5) < CMA_SCORE_BONUS(CMA_SCORE_BY_PLACE(2)))
			self->voice = FUN_000719a0(DAT_002baed0[last].character + 0x25, 0xffffffff, 0xff, 0, 0, 0);
	}

	if (self->camStage >= 5) {
		if (self->placeText != 0) {
			if (GameWinner_AnimateText(self->placeText)) {
				self->winnerText = GameWinner_NewText(0x173, 150, CMA_TEXT(0x1b3), 1);
			}
			GameWinner_FadeText(self->placeText);
		}
		if (self->winnerText != 0) {
			GameWinner_AnimateText(self->winnerText);
			GameWinner_FadeText(self->winnerText);
		}
		if (self->voice >= 0) {
			if (FUN_000548d0(self->voice) == 0) {
				self->voice = -2;
				self->crowdVoice = FUN_00071770(
					(char)DAT_002baed0[CMA_SCORE_BY_PLACE(0)].character, 9, 0, 0xff, 0, -1, 0);
			}
		} else if ((self->voice == -2 && self->crowdVoice == -1) ||
			(self->crowdVoice >= 0 && FUN_000548d0(self->crowdVoice) == 0)) {
			self->crowdVoice = -1;
			self->voice = -3;
		}
	}
	if (self->voice == -3 && !(self->timer < DAT_00283668)) {
		FUN_000719a0(0x38, 0xffffffff, 0xff, 0, 0, 0);
		self->voice = -4;
	}

	/* the cameras */
	if (DAT_002ed218 != 0) {
		if (FUN_0007e270(DAT_0028fda8, 0) != 0) {
			if (self->camStage < 6) {
				FUN_0007cc90();
				FUN_0007d590();
				self->camStage = self->camStage + 1;
				if (self->placeText != 0 && self->camStage <= 5)
					CMA_VFN(self->placeText, 0, void *(*)(void *, unsigned int))(self->placeText, 1);
				if (self->camStage == 5) {
					int winner = DAT_002baed0[CMA_SCORE_BY_PLACE(0)].character;

					self->placeText = GameWinner_NewText(0x1c3, 180, CMA_TEXT(0x149 + winner), 1);
					if ((int)Core_Rand() % 100 < 50)
						self->voice = FUN_000719a0(0x17, winner, 0xff, 0, 0, 0);
					else
						self->voice = FUN_000719a0(0x19, winner, 0xff, 0, 0, 0);
				} else if (self->camStage < 5) {
					int player = CMA_SCORE_BY_PLACE(5 - self->camStage);

					((int (*)(unsigned short *, const unsigned short *, ...))FUN_0004ae80)(DAT_002b9768,
						u"%S: %S", CMA_TEXT(0x149 + DAT_002baed0[player].character),
						CMA_TEXT(0x1a5 - self->camStage));
					self->placeText = GameWinner_NewText(0x1d2, 180, DAT_002b9768, 0);
				}
				if (self->camStage < 6) {
					GameWinner_PlayAnim(self, 5 - self->camStage, s_placeAnim[self->camStage], 0, 0, 1.0f, 0.0f);
					GameWinner_PlayAnim(self, 5 - self->camStage, s_placeIdleAnim[self->camStage], 1, 1, 1.0f,
						0.03f);
				}
				GameWinner_LoadCamera(self, self->camStage);
			} else {
				FUN_0007cc30();
			}
		}
		FUN_0007dbd0(self->eye, FUN_0007cc70());
		FUN_0007df20(self->target, FUN_0007cc70());
	}
	FUN_000435f0(self->eye, self->target, 0.0f);
	FUN_00043790();
	for (i = 0; i < 4; i++)
		CMA_Actor_Update(self->actors[i], CMA_FIELD(int, self->actors[i], 0x134), DAT_0028fda8, 0);

	if (self->leaving != 0) {
		double fade = (double)DAT_0028fda8 * 6.0 + (double)self->fade;

		self->fade = (float)fade;
		if (fade > 265.0)
			CMA_ModeStack_Pop(1);
		return;
	}
	if (self->fade != 0.0f) {
		double fade = (double)self->fade - (double)DAT_0028fda8 * 6.0;

		self->fade = (float)fade;
		if (fade < 0.0)
			self->fade = 0.0f;
	}
}

/* 0x0001b700 (not in the function table): the fireworks' explosion callback (FIREWORKS, ROCKET):
   a bang at the rocket, sometimes crackles */
static void GameWinner_FireworkExploded(void *fireworks, void *rocket)
{
	int bang;
	int pitch;

	CMA_Fireworks_Explode(fireworks, rocket);
	bang = (int)Core_Rand() % 3;
	pitch = (int)Core_Rand() % 5000 + 21000;
	CMA_PlaySample(DAT_002b9754[bang], 0xff, pitch, 0, (int)((char *)rocket + 4));
	if ((int)Core_Rand() % 20 == 0) {
		int crackle = (int)Core_Rand() % 6;

		pitch = (int)Core_Rand() % 5000 + 21000;
		CMA_PlaySample(DAT_002b973c[crackle], 200, pitch, 0, (int)((char *)rocket + 4));
	}
}

/* 0x0001b820  Core/CGameMode_GameWinner.cpp: popped: free everything */
void CGameMode_GameWinner_Popped(struct CGameMode_GameWinner *self)
{
	void *fireworks;
	int k;

	FUN_00054730();
	fireworks = self->fireworks;
	if (fireworks != 0) {
		FUN_00080100((int *)fireworks);
		CMA_Delete(fireworks);
	}
	if (self->placeText != 0)
		CMA_VFN(self->placeText, 0, void *(*)(void *, unsigned int))(self->placeText, 1);
	if (self->winnerText != 0)
		CMA_VFN(self->winnerText, 0, void *(*)(void *, unsigned int))(self->winnerText, 1);
	FUN_00034930((struct BActorInstance *)self->podium);
	FUN_00034930((struct BActorInstance *)self->background);
	for (k = 0; k < 2; k++)
		FUN_000384d0((struct BLight *)self->lights[k]);
	for (k = 0; k < 4; k++)
		FUN_00034930((struct BActorInstance *)self->actors[k]);
	FUN_0007d590();
	FUN_0004db40((int *)self->package);
}

/* a model of the package, posed at its first frame */
static int GameWinner_LoadActor(int model, int line)
{
	int actor = (int)FUN_00036b20(0, (struct BActor *)model, 6, (const char *)s_GameWinner_cpp, line);

	FUN_00035cc0((struct BActorInstance *)actor, 0, 1, 0);
	FUN_00035cc0((struct BActorInstance *)actor, 1, 0, 0);
	FUN_00034e70((struct BActorInstance *)actor, (undefined4 *)CMA_FIELD(int, actor, 0x134), 0, 1);
	FUN_00038190((void *)actor, (struct BActorNode *)CMA_FIELD(int, actor, 0x134), 1, 0);
	return actor;
}

/* 0x0001b8e0  Core/CGameMode_GameWinner.cpp: the package is in: fireworks, podium, background,
   the characters on the podium, sounds, the first camera */
void CGameMode_GameWinner_LoadingStarted(struct CGameMode_GameWinner *self)
{
	void *memory;
	int glow;
	int i;

	FUN_0004b870((struct bkPackage *)self->package, 0xffffffff, 0xdefa, 0);
	glow = (int)FUN_0004bb30(0, "firework_glow.bmp", 0, 0xdefa);
	memory = CMA_New(0x1c, s_GameWinner_cpp, 0x291, 0x1001);
	self->fireworks = memory == 0 ? 0 : CMA_Fireworks_ctor(memory, 800, glow, 50.0f, 30000, glow, 50.0f);
	CMA_FIELD(void *, self->fireworks, 0xc) = (void *)GameWinner_FireworkExploded;
	self->podium = GameWinner_LoadActor((int)FUN_0004bb30(0, "podium.obe", 1, 0), 0x299);
	FUN_00034bf0((struct BActorInstance *)self->podium, 1);
	self->background = GameWinner_LoadActor((int)FUN_0004bb30(0, "background.obe", 1, 0), 0x2a5);
	FUN_00034bf0((struct BActorInstance *)self->background, 1);
	FUN_0001bd70(DAT_002b9968);
	for (i = 0; i < 4; i++) {
		int character = DAT_002baed0[CMA_SCORE_BY_PLACE(i)].character;
		float rotation[4];
		int actor;
		int sequence;

		actor = GameWinner_LoadActor((int)FUN_0004bb30(0, DAT_002900c8 + character * 0x108, 1, 0), 0x2b6);
		self->actors[i] = actor;
		CMA_FIELD(float, actor, 0xc0) = DAT_002836e0[i * 4];
		CMA_FIELD(float, actor, 0xc4) = DAT_002836e0[i * 4 + 1];
		CMA_FIELD(float, actor, 0xc8) = DAT_002836e0[i * 4 + 2];
		CMA_FIELD(float, actor, 0xcc) = DAT_002836e0[i * 4 + 3];
		if (i == 0 && DAT_002baed0[CMA_SCORE_BY_PLACE(0)].character == 4)
			CMA_FIELD(float, self->actors[0], 0xc0) = CMA_FIELD(float, self->actors[0], 0xc0) + 30.0f;
		rotation[3] = DAT_00283750[i];
		rotation[0] = 0.0f;
		rotation[1] = 1.0f;
		rotation[2] = 0.0f;
		FUN_00070d70((float *)((char *)self->actors[i] + 0xe0), rotation);
		FUN_00034bf0((struct BActorInstance *)self->actors[i], 1);
		sequence = (int)FUN_00035c70((struct BActorInstance *)self->actors[i], DAT_00292ca0[CMA_ANIM_IDLE_CHARSELECT].name);
		actor = self->actors[i];
		FUN_00038190((void *)actor, (struct BActorNode *)CMA_FIELD(int, actor, 0x134), 1, 0);
		if (sequence != 0)
			FUN_00035770((struct BActorInstance *)self->actors[i], (struct BActorNode *)CMA_FIELD(int, self->actors[i], 0x134), (struct BAnim *)sequence, 0x400,
				0xffffffff, 1, 0, 0);
	}
	for (i = 0; i < 3; i++)
		DAT_002b9730[i] = (int)FUN_0004bb30(0, (char *)PTR_s_wheee1_wav_00283670[i], 2, 0xdefa);
	for (i = 0; i < 3; i++)
		DAT_002b9754[i] = (int)FUN_0004bb30(0, (char *)PTR_s_bang1_wav_0028367c[i], 2, 0xdefa);
	for (i = 0; i < 6; i++)
		DAT_002b973c[i] = (int)FUN_0004bb30(0, (char *)PTR_s_crackles1_wav_00283688[i], 2, 0xdefa);
	self->eye[1] = 500.0f;
	self->eye[0] = 1000.0f;
	self->eye[3] = 1.0f;
	self->target[0] = 0.0f;
	self->target[1] = 0.0f;
	self->target[2] = 0.0f;
	self->target[3] = 1.0f;
	GameWinner_LoadCamera(self, 1);
}
