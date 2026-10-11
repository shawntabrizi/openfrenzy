/*
MouseWhack/CGameMode_MouseWhack.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CGameMode_MouseWhack.cpp
(module mousewhack)

CGameMode_MouseWhack: the game mode of Bug Bash (vtable 0x00271da4, 0x34 bytes). Enter
starts loading the packages; when loading has finished the mode stack calls Init
(0x0016d780: textures, arena, players, tweak menus, cameras), then Start (0x0016d1d0:
HUD). Process (0x0016cf30) runs a frame of the game object; Draw (0x0016fe50) draws the
arena, the players, the bugs, the splats and the HUD; Exit (0x0016f920) frees everything.

All of these are gap functions; the file is known from the heap allocation tags in Init.
*/

#include "mousewhack.h"

const char mw_GameModeFile[] = "C:\\BlitzParty\\Game\\Src\\MouseWhack\\CGameMode_MouseWhack.cpp";

/* classlib_ui: the "game over" overlay object (CLGameOver?) and the rumble object */
extern unsigned char DAT_002ecd90[];
extern unsigned char DAT_002ed324[];
/* bdisplay: the colour channel lookup tables (alpha, red, green, blue; 256 entries each) */
extern unsigned int DAT_002bc898[256];
extern unsigned int DAT_002bd640[256];
extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bce18[256];
/* charlib: AI tuning values (CharLib data) */
extern float DAT_00294b60;
extern float DAT_00294b70;
extern float DAT_00294b78;
extern float DAT_00294ba8;
extern float DAT_00294bac;
extern float DAT_00294bb0;
extern float DAT_00294bb4;
extern float DAT_00294bb8;
/* classlib_ui: the actor list holder (+0x18 a list) */
extern unsigned char *DAT_002ea488;
/* core: frames per second, the per-player record +0x10 (score saved at exit) */
extern float DAT_002ba990;
extern int DAT_002baee8[];
/* charlib: the global animation speed factor */
extern float DAT_00293920;

/* 0x00271da4  CGameMode_MouseWhack vtable */
const void *const vtable_00271da4[13] = {
	(const void *)FUN_0016d2b0, /* [0] +0x00 scalar deleting destructor */
	(const void *)FUN_001d5950, /* [1] +0x04 suspend (empty) */
	(const void *)FUN_001d5950, /* [2] +0x08 resume (empty) */
	(const void *)FUN_0016f920, /* [3] +0x0c exit */
	(const void *)FUN_0016cee0, /* [4] +0x10 enter */
	(const void *)FUN_000d9730, /* [5] +0x14 update (polls the loading) */
	(const void *)FUN_0016cf30, /* [6] +0x18 process */
	(const void *)FUN_0016fe50, /* [7] +0x1c draw */
	(const void *)FUN_001ec260, /* [8] +0x20 post draw (empty) */
	(const void *)FUN_00012930, /* [9] +0x24 poll the loading */
	(const void *)FUN_000128e0, /* [10] +0x28 loading finished */
	(const void *)FUN_0016d780, /* [11] +0x2c init (the packages are loaded) */
	(const void *)FUN_0016d1d0, /* [12] +0x30 start */
};

/* the mode object */
#define MWM_LOADSTATE(m) MW_AI(m, 0x14)   /* CGameMode +0x14: 1 loading */
#define MWM_FLAGS(m) MW_AU(m, 0x10)
#define MWM_ARG(m) MW_AI(m, 0x28)         /* passed to the package loader (unused there) */
#define MWM_NEAR(m) MW_AF(m, 0x2c)        /* the saved minigame near/far planes */
#define MWM_FAR(m) MW_AF(m, 0x30)

/* 0x0016ceb0  (gap): CGameMode_MouseWhack::CGameMode_MouseWhack */
void *CGameMode_MouseWhack_Ctor(void *self)
{
	CGameMode_ctor((struct CGameMode *)self, "MouseWhack");
	*(const void *const **)self = vtable_00271da4;
	return self;
}

/* 0x0016ced0  (gap): CGameMode_MouseWhack::~CGameMode_MouseWhack */
void CGameMode_MouseWhack_Dtor(void *self)
{
	*(const void *const **)self = vtable_00271da4;
	CGameMode_dtor((struct CGameMode *)self);
}

/* 0x0016cee0  (gap): enter (the mode was pushed): starts loading the packages and sets the
near/far planes the minigames use (4000, 10000), saving the old ones */
void CGameMode_MouseWhack_Enter(void *self)
{
	bkPrintf("\n\nBB pushed\n");
	FUN_00172b80(mw_Game, MWM_ARG(self));
	MWM_LOADSTATE(self) = 1;
	MWM_NEAR(self) = DAT_0028ffa8;
	MWM_FAR(self) = DAT_0028ffac;
	DAT_0028ffa8 = 4000.0f;
	DAT_0028ffac = 10000.0f;
}

/* 0x0016cf30  (gap): process: frame timing, the pause/quit check, then the game frame */
void CGameMode_MouseWhack_Process(void *self, int top)
{
	float rate;
	int paused;

	(void)self;
	if (top == 0 || mw_Quit != 0)
		return;
	if (mw_DbgSpeedLock != 0)
		DAT_0028fda8 = 1.0f;
	FUN_001761a0(mw_Timer);
	rate = bkTimerRate((longlong)(int)bkTimerDelta(MW_G(0x0030a268, ulonglong), MW_G(0x0030a260, ulonglong)));
	MW_F(0x0030cdf0) = rate;
	MW_I(0x0030cdf8) = MW_I(0x0030cdf8) + 1;
	MW_F(0x0030cdf4) = MW_F(0x0030cdf4) + rate;
	MW_S(0x0030cdfc) = 0;
	MW_S(0x0030cdfe) = 1;
	MW_F(0x0030a0ac) = (50.0f - rate) + MW_F(0x0030a0ac);
	paused = DAT_002ecdec;
	if (mw_GameState != 2 || DAT_002ecdec != 0) {
		if (MW_F(0x0030eb94) == 0.0f && FUN_000772e0((undefined4 *)DAT_002ecd90) != 0) {
			mw_Quit = 1;
			return;
		}
		if ((short)paused != DAT_002ecdec)
			bkPrintf("game pause state change\n");
	}
	FUN_00175b70(mw_Game);
	if (MW_S(0x0030ce02) == 0 && DAT_002ecdec != 0)
		CLIntroCam_Update(0.0f, 0);
}

/* 0x0016d070  (gap): first draw pass of the actors in the actor list (except those flagged
0x100000), with sanity prints for bad w/scale values */
void MouseWhack_DrawActorsFirstPass(void)
{
	unsigned char *actor;

	for (actor = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_002ea488, NULL); actor != NULL;
	     actor = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_002ea488, (unsigned int *)actor)) {
		if (MW_AF(actor, 0xdc) < 1.0f)
			bkPrintf("actor with iffy w value\n");
		if (MW_AF(actor, 0xe0) == 0.0f || MW_AF(actor, 0xe4) == 0.0f || MW_AF(actor, 0xe8) == 0.0f)
			bkPrintf("actor with iffy scale value %f %f %f\n", (double)MW_AF(actor, 0xe0),
				(double)MW_AF(actor, 0xe4), (double)MW_AF(actor, 0xe8));
		if ((MW_AU(actor, 0x220) & 0x100000) == 0)
			Actor_DrawFirstPass(actor);
	}
}

/* 0x0016d150  (gap): the faded, then the opaque pass of the actors in the actor list */
void MouseWhack_DrawActors(void)
{
	unsigned char *actor;

	for (actor = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_002ea488, NULL); actor != NULL;
	     actor = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_002ea488, (unsigned int *)actor)) {
		if ((MW_AU(actor, 0x220) & 0x100000) == 0)
			Actor_DrawFaded(actor);
	}
	for (actor = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_002ea488, NULL); actor != NULL;
	     actor = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_002ea488, (unsigned int *)actor)) {
		if ((MW_AU(actor, 0x220) & 0x100000) == 0)
			Actor_DrawOpaque(actor);
	}
}

/* 0x0016d1d0  (gap): start (after init): the HUD (score boxes, a 60 s clock, bars), the
near/far planes and the start sound */
void CGameMode_MouseWhack_Start(void *self)
{
	int scores[4];

	scores[0] = 0;
	scores[1] = 0;
	scores[2] = 0;
	scores[3] = 0;
	FUN_00072720((undefined4)(uintptr_t)DAT_0030a0e4);
	CLHud_CreateScoreBoxes((int)(uintptr_t)DAT_002ecfac);
	CLHud_CreateScoreTexts((int)(uintptr_t)DAT_002ecfac, scores);
	CLHud_CreateClock((int)(uintptr_t)DAT_002ecfac, 60.0f);
	CLHud_CreateBars((int)(uintptr_t)DAT_002ecfac);
	if (mw_Variation == 2)
		CLHud_InitBarGeometry((int)(uintptr_t)DAT_002ecfac);
	MWM_FLAGS(self) |= 1;
	bdSetNearFar(40.0f, 10000.0f);
	FUN_0017cf10(mw_Variation == 2 ? 0x30 : 0x2b, 0, 0xff);
	MW_I(0x0030ebf8) = 0;
	MW_I(0x0030ebfc) = 0;
	MW_I(0x0030ec00) = 0;
	MW_I(0x0030ec04) = 0;
	MW_S(0x0030ec08) = 0;
}

/* 0x0016d2b0  (gap): scalar deleting destructor */
void *CGameMode_MouseWhack_Destroy(void *self, byte flags)
{
	CGameMode_MouseWhack_Dtor(self);
	if ((flags & 1) != 0)
		MW_DELETE(self);
	return self;
}

/* 0x0016d2d0  (gap): the bug shadows: a textured quad under each active bug (not types 7
and 8), sized by the bug type and turned with its heading. In "Tremors" (variation 1) the
quad lies on the ground found by a ray cast. Vertices: 4 x 0x24 bytes at 0x0030a138
(FVF 0x152: position, normal, colour, uv). */
void MouseWhack_DrawBugShadows(void)
{
	float corners[4][4];
	float start[4], delta[4], hit[4], normal[4];
	float angle, size, length, x, y, z;
	unsigned char *node, *actor;
	unsigned int colour;
	int high, i;
	short type;

	bdSetTexture(0, (struct bdTexture *)mw_ShadowTexture);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 3, 0);
	bdSetRenderState(5, 0, 0);
	bdSetWorldMatrix(DAT_002855f0);
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
		/* the node is at +8 of the bug */
		if (MW_AS(node, 0x11f2) == 0)
			continue;
		type = MW_AS(node, 0x11e4);
		if (type == 7 || type == 8)
			continue;
		angle = 3.1415925f - MW_AF(node, 0x1270);
		high = 0;
		switch (type) {
		case 0:
		case 5:
			size = 80.0f;
			angle = angle - MW_AF(node, 0x125c);
			length = 30.0f;
			break;
		case 1:
		case 6:
			if (MW_AS(node, 0x11e6) != 0)
				angle = angle + 1.5707963f;
			size = 100.0f;
			length = 60.0f;
			break;
		case 3:
		case 4:
			size = 80.0f;
			length = 80.0f;
			break;
		default:
			size = 0.0f;
			length = 0.0f;
			break;
		}
		actor = (unsigned char *)MW_AP(node, -8);
		if (mw_Variation == 1) {
			if (MW_S(0x0030ce00) != 2 && MW_S(0x0030ce04) != 2) {
				start[0] = MW_AF(actor, 0xd0);
				start[1] = MW_AF(actor, 0xd4);
				start[2] = MW_AF(actor, 0xd8);
				start[3] = MW_AF(actor, 0xdc);
				start[1] = MW_AF(actor, 0xd4) + 100.0f;
				delta[0] = 0.0f * 1.4f;
				delta[1] = ((MW_AF(actor, 0xd4) - 2000.0f) - start[1]) * 1.4f;
				delta[2] = 0.0f * 1.4f;
				delta[3] = start[3];
				((float (*)(float *, float *, float *, float *))FUN_000b4bd0)(start, delta, hit, normal);
				hit[1] = hit[1] + 2.0f;
				((void (*)(void *, float *, float *, float, float, float))FUN_0007b6f0)(MW_A(0x0030a138), hit, normal,
					size, 0.0f, 80.0f);
			}
		} else {
			y = MW_AF(node, 0x1228) + 2.0f;
			z = MW_AF(actor, 0xd8);
			x = MW_AF(actor, 0xd0);
			if (MW_AF(actor, 0xd4) < y)
				y = MW_AF(actor, 0xd4) + 2.0f;
			if (y > 400.0f)
				high = 1;
			memset(corners, 0, sizeof corners);
			corners[0][0] = size;
			corners[0][2] = length;
			corners[1][0] = -size;
			corners[1][2] = length;
			corners[2][0] = size;
			corners[2][2] = -length;
			corners[3][0] = -size;
			corners[3][2] = -length;
			for (i = 0; i < 4; i++)
				bmVectorRotateY(corners[i], corners[i], angle);
			if (high != 0)
				continue;
			if (mw_Variation != 1 && MW_S(0x0030ce00) != 2 && MW_S(0x0030ce04) != 2) {
				MW_F(0x0030a138) = x + corners[0][0];
				MW_F(0x0030a13c) = y;
				MW_F(0x0030a160) = y;
				MW_F(0x0030a184) = y;
				MW_F(0x0030a140) = z + corners[0][2];
				MW_F(0x0030a1a8) = y;
				MW_F(0x0030a15c) = x + corners[1][0];
				MW_F(0x0030a164) = z + corners[1][2];
				MW_F(0x0030a180) = x + corners[2][0];
				MW_F(0x0030a188) = z + corners[2][2];
				MW_F(0x0030a1a4) = x + corners[3][0];
				MW_F(0x0030a1ac) = z + corners[3][2];
			}
		}
		colour = DAT_002bc898[0x40] | DAT_002bd640[0x60] | DAT_002bc490[0x60] | DAT_002bce18[0x60];
		MW_U(0x0030a150) = colour;
		MW_U(0x0030a174) = colour;
		MW_U(0x0030a198) = colour;
		MW_U(0x0030a1bc) = colour;
		MW_F(0x0030a154) = 0.0f;
		MW_F(0x0030a158) = 1.0f;
		MW_F(0x0030a178) = 1.0f;
		MW_F(0x0030a17c) = 1.0f;
		MW_F(0x0030a19c) = 0.0f;
		MW_F(0x0030a1a0) = 0.0f;
		MW_F(0x0030a1c0) = 1.0f;
		MW_F(0x0030a1c4) = 0.0f;
		if (MW_S(0x0030ce00) != 2 && MW_S(0x0030ce04) != 2)
			bdDrawPrimitiveFVF(4, MW_A(0x0030a138), 4, 0x152);
	}
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
}

/* AI tuning of the bugs for a variation and the handicap (0x0030ec34-0x0030ec4c) */
static void mw_SetBugTuning(float a, float b, float c, float d, float e, float f, float g)
{
	MW_F(0x0030ec34) = a;
	MW_F(0x0030ec38) = b;
	MW_F(0x0030ec3c) = c;
	MW_F(0x0030ec40) = d;
	MW_F(0x0030ec44) = e;
	MW_F(0x0030ec48) = f;
	MW_F(0x0030ec4c) = g;
}

/* new CTweakMenu(name) (0xe8 bytes) */
static struct CTweakMenu *mw_NewTweakMenu(const char *name, int line)
{
	void *menu = MW_NEW(0xe8, line);

	if (menu == NULL)
		return NULL;
	return CTweakMenu_Ctor((struct CTweakMenu *)menu, name);
}

/* new charlib camera (0x574 bytes, FUN_000d63f0(this, 5)) */
static void *mw_NewCamera(int line)
{
	void *camera = MW_NEW(0x574, line);

	if (camera == NULL)
		return NULL;
	return (void *)(uintptr_t)FUN_000d63f0((void *)(uintptr_t)camera, 5);
}

/* the camera follows every player */
static void mw_CameraAddPlayers(void)
{
	int i;

	for (i = 0; i < DAT_002bae94; i++)
		FUN_000d62c0((void *)(uintptr_t)mw_Camera, (float *)(uintptr_t)MW_AV(MW_AP(mw_Players[i], 0x30), 0xd0), 0,
			(float *)(uintptr_t)DAT_002ea478);
}

/* the subway train of "Brawl" and "Pied Pipers" (an actor instance at 0x0030a340) */
static void mw_LoadTrain(int line)
{
	float matrix[16];

	mw_TrainModel = bLoadResource(NULL, "models\\train.obe", 1, 0xdefa);
	baInitActorInstance((struct BActorInstance *)MW_A(0x0030a340), (struct BActor *)mw_TrainModel, 2, mw_GameModeFile, line);
	MW_F(0x0030a400) = -2893.0f;
	MW_F(0x0030a404) = 58.0f;
	MW_F(0x0030a408) = -2910.0f;
	bmMatrixRotationY(matrix, 0.0f);
	bmMatrixToQuaternion(MW_FV(0x0030a420), matrix);
}

/* 4 random-speed idle animations queued on a player's actor */
static void mw_QueueIdleAnims(void *actor, float alt)
{
	float r;

	Actor_PlayAnim(actor, 0, 0, 0, (float)mw_rand() * 3.0517578e-05f + 0.5f, 0.1f, 1);
	r = (float)mw_rand() * 3.0517578e-05f + 0.5f;
	Actor_PlayAnim(actor, 0, 0, 1, r + r, 0.1f, 1);
	Actor_PlayAnim(actor, 0, 0, 1, (float)mw_rand() * 3.0517578e-05f + alt, 0.1f, 1);
	r = (float)mw_rand() * 3.0517578e-05f + 0.5f;
	Actor_PlayAnim(actor, 0, 0, 1, r + r, 0.1f, 1);
	Actor_PlayAnim(actor, 0, 0, 1, (float)mw_rand() * 3.0517578e-05f + alt, 0.1f, 1);
	r = (float)mw_rand() * 3.0517578e-05f + 0.5f;
	Actor_PlayAnim(actor, 0, 0, 1, r + r, 0.1f, 1);
}

/* 0x0016d780  (gap): init (the packages are loaded): resources, the arena of the
variation, the players, the tweak menus, the intro camera, the camera targets */
void CGameMode_MouseWhack_Init(void *self)
{
	float pos[4];
	float spawn[4];
	const char *wallName;
	void *object;
	int i;

	(void)self;
	DAT_00294bb0 = MW_F(0x002b2770);
	DAT_00294bac = MW_F(0x002b276c);
	DAT_00294ba8 = MW_F(0x002b2768);
	mw_Quit = 0;
	DAT_00294bb4 = MW_F(0x002b2774);
	DAT_00294bb8 = -0.5f;
	DAT_00294b60 = -120.0f;
	DAT_00294b70 = 50.0f;
	DAT_00294b78 = 200.0f;
	MW_S(0x0030ce76) = 0;
	bLoadPackageResources((struct bkPackage *)mw_LevelPackage, 0xffffffff, 0xdefa, 0);
	bLoadPackageResources((struct bkPackage *)mw_VariationPackage, 0xffffffff, 0xdefa, 0);
	FUN_000a74d0((struct BCarFloatyScore *)MW_A(0x0030ea70), 0x40);
	MW_F(0x0030eaf4) = 0.15f;
	for (i = 0; i < 5; i++)
		MW_I(0x002b2db4 + i * 0x10) = 0;
	for (i = 0; i < 5; i++)
		MW_I(0x002b2d54 + i * 0x10) = 0;
	mw_FontBold = bLoadResource(NULL, "fonts\\handgotb.ttf", 3, 0);
	MW_P(0x0030a320) = bLoadResource(NULL, "fonts\\handgotl.ttf", 3, 0);
	MW_P(0x0030a31c) = bLoadResource(NULL, "fonts\\gunship.ttf", 3, 0);
	mw_ShadowTexture = bLoadResource(NULL, "textures\\shadow.bmp", 0, 0xdefa);
	MW_P(0x0030a32c) = bLoadResource(NULL, "textures\\target.bmp", 0, 0xdefa);
	for (i = 0; i < 10; i++)
		mw_SplatTextures[i] = NULL;
	mw_SplatTextures[0] = bLoadResource(NULL, "textures\\splatgrn.gif", 0, 0xdefa);
	mw_SplatTextures[1] = bLoadResource(NULL, "textures\\splatred.gif", 0, 0xdefa);
	mw_SplatTextures[2] = bLoadResource(NULL, "textures\\splatblu.gif", 0, 0xdefa);
	mw_SplatTextures[3] = bLoadResource(NULL, "textures\\splatyel.gif", 0, 0xdefa);
	mw_SplatTextures[4] = bLoadResource(NULL, "textures\\splatorg.gif", 0, 0xdefa);
	mw_SplatTextures[5] = bLoadResource(NULL, "textures\\splatpnk.gif", 0, 0xdefa);
	mw_SplatTextures[6] = bLoadResource(NULL, "textures\\splat.gif", 0, 0xdefa);
	mw_SplatTextures[7] = bLoadResource(NULL, "textures\\splat2.gif", 0, 0xdefa);
	mw_SplatTextures[8] = bLoadResource(NULL, "textures\\splat3.gif", 0, 0xdefa);
	mw_SplatTextures[9] = bLoadResource(NULL, "textures\\splat4.gif", 0, 0xdefa);
	for (i = 0; i < 10; i++)
		bkPrintf(mw_SplatTextures[i] == NULL ? "player splat tex %d missing!\n" : "player splat tex %d found ok\n", i);
	mw_SprayTexture = NULL;
	mw_SprayTexture = bLoadResource(NULL, "textures\\spray.bmp", 0, 0xdefa);
	mw_BlastTexture = NULL;
	mw_BlastTexture = bLoadResource(NULL, "textures\\blastring.bmp", 0, 0xdefa);
	mw_SwipeTexture = NULL;
	mw_SwipeTexture = bLoadResource(NULL, "textures\\swipe.bmp", 0, 0xdefa);
	mw_ParticleTexture = NULL;
	mw_ParticleTexture = bLoadResource(NULL, "textures\\particletwo00.bmp", 0, 0xdefa);
	bkPrintf(mw_BlastTexture == NULL ? "failed to load blastTex\n" : "blastTex loaded ok\n");
	bkPrintf(mw_SprayTexture == NULL ? "failed to load sprayTex\n" : "spray Tex loaded ok\n");
	bkPrintf(mw_SwipeTexture == NULL ? "failed to load swipeTex\n" : "swipeTex loaded ok\n");
	bkPrintf(mw_ParticleTexture == NULL ? "failed to load deathTex\n" : "deathTex loaded ok\n");
	FUN_0017ce90(0);
	MW_P(0x0030a330) = bLoadResource(NULL, "textures\\wormtmp.bmp", 0, 0xdefa);
	MW_P(0x0030a334) = bLoadResource(NULL, "textures\\roachtmp.bmp", 0, 0xdefa);
	Debris_Init(NULL, 1000);
	FUN_000b9bf0();
	FUN_001839b0();
	pos[0] = 0.0f;
	pos[1] = 0.0f;
	pos[2] = 0.0f;
	pos[3] = 0.0f;
	mw_ArenaSet = NULL;
	mw_WallSet = NULL;
	wallName = NULL;
	switch (mw_Variation) {
	case 0:
		if (DAT_002bae70 == 0)
			mw_SetBugTuning(0.0f, 0.0f, 0.2f, 1.0f, 300.0f, 300.0f, -1.0f);
		else if (DAT_002bae70 == 1)
			mw_SetBugTuning(0.5f, 0.3f, 0.1f, 0.44f, 200.0f, 300.0f, 0.0f);
		else if (DAT_002bae70 == 2)
			mw_SetBugTuning(1.0f, 1.0f, 0.0f, 0.04f, 100.0f, 300.0f, 1.0f);
		break;
	case 1:
		if (DAT_002bae70 == 0)
			mw_SetBugTuning(0.0f, 0.0f, 0.2f, 1.0f, 300.0f, 300.0f, -1.0f);
		else if (DAT_002bae70 == 1)
			mw_SetBugTuning(0.5f, 1.0f, 0.15f, 0.24f, 100.0f, 300.0f, 1.0f);
		else if (DAT_002bae70 == 2)
			mw_SetBugTuning(1.0f, 0.8f, -0.09f, 0.0f, 0.0f, 250.0f, 1.0f);
		mw_ArenaActor = Actor_LoadAny("arenas\\trackmain.obe", pos, 0, NULL, 0);
		MW_AU(mw_ArenaActor, 0x220) |= 0x400000;
		mw_ArenaSet = bLoadResource(NULL, "arenas\\trackmain02.obe", 6, 0xdefa);
		bkPrintf(mw_ArenaSet == NULL ? "ERROR failed to load set!\n" : "set loaded ok\n");
		wallName = "arenas\\trackwall.obe";
		break;
	case 2:
		if (DAT_002bae70 == 0)
			mw_SetBugTuning(0.0f, 0.0f, 0.3f, 1.0f, 500.0f, 300.0f, -1.0f);
		else if (DAT_002bae70 == 1)
			mw_SetBugTuning(0.5f, 0.3f, 0.1f, 0.44f, 200.0f, 300.0f, 0.0f);
		else if (DAT_002bae70 == 2)
			mw_SetBugTuning(1.0f, 1.0f, -0.09f, 0.0f, 0.0f, 350.0f, 1.0f);
		mw_ArenaActor = Actor_LoadAny("arenas\\burgermain.obe", pos, 0, NULL, 0);
		MW_AU(mw_ArenaActor, 0x220) |= 0x480000;
		mw_ArenaSet = bLoadResource(NULL, "arenas\\burgermain02.obe", 6, 0xdefa);
		bkPrintf(mw_ArenaSet == NULL ? "ERROR failed to load set!\n" : "set loaded ok\n");
		break;
	case 3:
		if (DAT_002bae70 == 0)
			mw_SetBugTuning(0.0f, 0.0f, 0.45f, 1.0f, 300.0f, 600.0f, -1.0f);
		else if (DAT_002bae70 == 1)
			mw_SetBugTuning(0.5f, 0.3f, 0.25f, 0.44f, 200.0f, 600.0f, 0.0f);
		else if (DAT_002bae70 == 2)
			mw_SetBugTuning(1.0f, 1.0f, 0.0f, 0.04f, 100.0f, 600.0f, 1.0f);
		break;
	}
	if (mw_Variation == 0 || mw_Variation == 3) {
		mw_ArenaActor = Actor_LoadAny("arenas\\submain.obe", pos, 0, NULL, 0);
		MW_AU(mw_ArenaActor, 0x220) |= 0x400000;
		mw_ArenaSet = bLoadResource(NULL, "arenas\\submain02.obe", 6, 0xdefa);
		bkPrintf(mw_ArenaSet == NULL ? "ERROR failed to load set!\n" : "set loaded ok\n");
		wallName = "arenas\\subwall.obe";
	}
	if (wallName != NULL) {
		mw_WallSet = bLoadResource(NULL, wallName, 6, 0xdefa);
		bkPrintf(mw_WallSet == NULL ? "ERROR failed to load miniset!\n" : "miniset loaded ok\n");
	}
	FUN_00181770();
	/* four route blockers around the middle (0x20-byte charlib objects) */
	pos[3] = 0.0f;
	pos[1] = 8000.0f;
	if (mw_Variation == 2) {
		pos[0] = 300.0f;
		pos[2] = -300.0f;
		if ((object = MW_NEW(0x20, 0x5b6)) != NULL)
			FUN_000b9b20((undefined4 *)object, (float *)pos, 0);
		pos[2] = 300.0f;
		if ((object = MW_NEW(0x20, 0x5b8)) != NULL)
			FUN_000b9b20((undefined4 *)object, (float *)pos, 0);
		pos[0] = -300.0f;
		if ((object = MW_NEW(0x20, 0x5ba)) != NULL)
			FUN_000b9b20((undefined4 *)object, (float *)pos, 0);
		pos[2] = -300.0f;
		object = MW_NEW(0x20, 0x5bc);
	} else {
		pos[0] = 500.0f;
		pos[2] = -500.0f;
		if ((object = MW_NEW(0x20, 0x5c4)) != NULL)
			FUN_000b9b20((undefined4 *)object, (float *)pos, 0);
		pos[2] = 500.0f;
		if ((object = MW_NEW(0x20, 0x5c6)) != NULL)
			FUN_000b9b20((undefined4 *)object, (float *)pos, 0);
		pos[0] = -500.0f;
		if ((object = MW_NEW(0x20, 0x5c8)) != NULL)
			FUN_000b9b20((undefined4 *)object, (float *)pos, 0);
		pos[2] = -500.0f;
		object = MW_NEW(0x20, 0x5ca);
	}
	if (object != NULL)
		FUN_000b9b20((undefined4 *)object, (float *)pos, 0);
	pos[1] = 0.0f;
	/* the bug models: 6 x {model, resource, low detail model, resource} at 0x002b26b8 */
	for (i = 0; i < 6; i++) {
		uint32_t *entry = &mw_data[(0x002b26b8 - MW_DATA_LO) / 4 + i * 4];

		entry[1] = (uint32_t)(uintptr_t)bLoadResource(NULL, (const char *)(uintptr_t)entry[0], 1, 0);
		entry[3] = (uint32_t)(uintptr_t)bLoadResource(NULL, (const char *)(uintptr_t)entry[2], 1, 0);
		bkPrintf(entry[1] == 0 ? "Failed to load actor %s\n" : "Loaded actor %s\n", (const char *)(uintptr_t)entry[0]);
		bkPrintf(entry[3] == 0 ? "Failed to load actor %s\n" : "Loaded actor %s\n", (const char *)(uintptr_t)entry[2]);
	}
	/* the players */
	if (mw_Variation == 2) {
		for (i = 0; i < DAT_002bae94; i++) {
			float *start = MW_FV(0x002b2718 + i * 0x10);

			spawn[1] = start[1];
			spawn[3] = start[3];
			spawn[0] = start[0] + 160.0f;
			spawn[2] = start[2] + 160.0f;
			object = MW_NEW(0x1410, 0x5ef);
			if (object != NULL)
				object = FUN_0017d6d0(object, i, spawn);
			mw_Players[i] = object;
			MW_AF(MW_AP(object, 0x30), 0x1d8) = MW_AF(MW_AP(object, 0x30), 0x1d8) + 3.1415925f;
			DAT_002f289c[i] = mw_Players[i];
			mw_QueueIdleAnims(MW_AP(mw_Players[i], 0x30), 1.0f);
		}
	} else {
		for (i = 0; i < DAT_002bae94; i++) {
			object = MW_NEW(0x1410, 0x60c);
			if (object != NULL)
				object = FUN_0017d6d0(object, i, pos);
			mw_Players[i] = object;
			DAT_002f289c[i] = object;
		}
		for (i = 0; i < DAT_002bae94; i++) {
			unsigned char *player;
			int k;

			CLSamples_PlayGeneric(5, 1, 0, 1, NULL);
			MW_AI(mw_Players[i], 0x1124) = 0;
			MW_AI(mw_Players[i], 0x1114) = 0;
			MW_VFN(mw_Players[i], 0x24, void (*)(void *))(mw_Players[i]);
			mw_QueueIdleAnims(MW_AP(mw_Players[i], 0x30), 0.5f);
			CLSamples_PlayGeneric(5, 1, 0, 1, NULL);
			/* the position history (5 entries) starts at the spawn point */
			player = (unsigned char *)mw_Players[i];
			for (k = 0; k < 5; k++)
				memcpy(player + 0x1378 + k * 0x10, MW_AV(MW_AP(player, 0x30), 0xd0), 0x10);
			MW_AS(player, 0x13c8) = 0;
		}
	}
	/* 10 helper objects of 0x1c bytes */
	for (i = 0; i < 10; i++) {
		object = MW_NEW(0x1c, 0x632);
		if (object != NULL)
			object = FUN_00181730(object);
		mw_Helpers[i] = object;
	}
	((void (*)(int, float, float, float))FUN_000be6d0)(2, 1.1f, 3.1415925f, 0.0f);
	((void (*)(float, float))FUN_000be3f0)(0.3f, 0.45f);
	/* scores */
	mw_ScoreMenu = mw_NewTweakMenu("score Menu", 0x63c);
	switch (mw_Variation) {
	case 0:
		MW_I(0x0030ee04) = 10;
		MW_I(0x0030ee00) = 5;
		MW_I(0x0030ee08) = 15;
		MW_I(0x0030ee0c) = 40;
		MW_I(0x0030ee28) = 50;
		MW_I(0x0030ee2c) = 15;
		MW_I(0x0030ee24) = 30;
		CTweakMenu_AddInt(mw_ScoreMenu, "worm score", MW_IV(0x0030ee00), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "roach score", MW_IV(0x0030ee04), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "fly score", MW_IV(0x0030ee08), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "tremor score", MW_IV(0x0030ee0c), 1, 1, 100, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "spore damage", MW_IV(0x0030ee28), 1, 1, 100, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "spore damage sec", MW_IV(0x0030ee2c), 1, 1, 100, 0);
		break;
	case 1:
		MW_I(0x0030ee08) = 10;
		MW_I(0x0030ee20) = 10;
		MW_I(0x0030ee0c) = 40;
		MW_I(0x0030ee24) = 30;
		CTweakMenu_AddInt(mw_ScoreMenu, "fly score", MW_IV(0x0030ee08), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "tremor score", MW_IV(0x0030ee0c), 1, 1, 100, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "train damage", MW_IV(0x0030ee20), 1, 1, 100, 0);
		break;
	case 2:
		MW_I(0x0030ee04) = 10;
		MW_I(0x0030ee08) = 10;
		MW_I(0x0030ee00) = 5;
		CTweakMenu_AddInt(mw_ScoreMenu, "worm score", MW_IV(0x0030ee00), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "roach score", MW_IV(0x0030ee04), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "fly score", MW_IV(0x0030ee08), 1, 1, 50, 0);
		break;
	case 3:
		MW_I(0x0030ee18) = 10;
		MW_I(0x0030ee1c) = 8;
		MW_I(0x0030ee10) = 1;
		MW_I(0x0030ee14) = 1;
		CTweakMenu_AddInt(mw_ScoreMenu, "worm exit score", MW_IV(0x0030ee18), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "roach exit score", MW_IV(0x0030ee1c), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "worm paint score", MW_IV(0x0030ee10), 1, 1, 50, 0);
		CTweakMenu_AddInt(mw_ScoreMenu, "roach paint score", MW_IV(0x0030ee14), 1, 1, 50, 0);
		break;
	}
	FUN_0017d330();
	FUN_00170ec0(mw_Arena);
	bdSetWorldMatrix(DAT_002855f0);
	bdSetProjectionMode(1);
	MW_F(0x0030ec10) = 400.0f;
	MW_F(0x0030ec18) = 1.0f;
	MW_F(0x0030ec0c) = 450.0f;
	MW_F(0x0030ec14) = 0.6f;
	MW_F(0x0030ec1c) = 1.0f;
	MW_F(0x0030ec2c) = 20.0f;
	MW_F(0x0030ec30) = 30.0f;
	MW_F(0x0030ec24) = 10.0f;
	MW_F(0x0030ec28) = 10.0f;
	MW_F(0x0030ec20) = 70.0f;
	/* impacts between players */
	mw_ImpactMenu = mw_NewTweakMenu("Impact Menu", 0x688);
	CTweakMenu_AddFloat(mw_ImpactMenu, "Push Amount", MW_FV(0x0030ec2c), 0.5f, 1.0f, 50.0f);
	CTweakMenu_AddFloat(mw_ImpactMenu, "Push Amount Ko", MW_FV(0x0030ec30), 0.5f, 1.0f, 50.0f);
	CTweakMenu_AddFloat(mw_ImpactMenu, "Push Time", MW_FV(0x0030ec24), 0.5f, 1.0f, 50.0f);
	CTweakMenu_AddFloat(mw_ImpactMenu, "Push Time Ko", MW_FV(0x0030ec28), 0.5f, 1.0f, 50.0f);
	CTweakMenu_AddFloat(mw_ImpactMenu, "Ko time", MW_FV(0x0030ec20), 5.0f, 1.0f, 100.0f);
	if (mw_Variation == 2) {
		MW_F(0x0030ce50) = 0.2f;
		CTweakMenu_AddFloat(mw_ImpactMenu, "Burger Damage", MW_FV(0x0030ce50), 0.05f, 0.01f, 1.0f);
	}
	if (mw_Variation == 0 || mw_Variation == 1) {
		mw_TremorMenu = mw_NewTweakMenu("tremor Menu", 0x698);
		CTweakMenu_AddFloat(mw_TremorMenu, "Turn rate", MW_FV(0x0030ec14), 0.1f, 0.2f, 3.0f);
		CTweakMenu_AddFloat(mw_TremorMenu, "Bite speed", MW_FV(0x0030ec18), 0.1f, 0.2f, 3.0f);
		CTweakMenu_AddFloat(mw_TremorMenu, "Attack range", MW_FV(0x0030ec10), 20.0f, 200.0f, 700.0f);
		CTweakMenu_AddFloat(mw_TremorMenu, "start attack rng", MW_FV(0x0030ec0c), 20.0f, 200.0f, 700.0f);
		CTweakMenu_AddFloat(mw_TremorMenu, "Spawn chance mod", MW_FV(0x0030ec1c), 1.0f, 0.0f, 10.0f);
	}
	mw_DebugMenu = mw_NewTweakMenu("Bug Bash Menu", 0x6a0);
	object = MW_NEW(0x10, 0x6a2);
	if (object != NULL)
		object = FUN_00180720(object);
	mw_Bullets = object;
	mw_DbgOverlays = 1;
	mw_DbgShadows = 1;
	mw_DbgMessages = 0;
	mw_DbgFirstPerson = 0;
	mw_DbgHoldOutro = 0;
	mw_DbgFrameCounters = 0;
	mw_DbgAIState = 1;
	mw_DbgSpeedLock = 0;
	mw_DbgBugsFade = 1;
	MW_F(0x0030a0d4) = 10.0f;
	MW_F(0x0030a0d0) = 1.5f;
	CTweakMenu_AddBool(mw_DebugMenu, "debug messages", &mw_DbgMessages);
	CTweakMenu_AddBool(mw_DebugMenu, "overlay updates", &mw_DbgOverlays);
	CTweakMenu_AddBool(mw_DebugMenu, "AI state", &mw_DbgAIState);
	CTweakMenu_AddBool(mw_DebugMenu, "Game Speed lock", &mw_DbgSpeedLock);
	CTweakMenu_AddBool(mw_DebugMenu, "Shadows", &mw_DbgShadows);
	CTweakMenu_AddBool(mw_DebugMenu, "Frame Counters", &mw_DbgFrameCounters);
	CTweakMenu_AddBool(mw_DebugMenu, "Bugs fade out", &mw_DbgBugsFade);
	CTweakMenu_AddBool(mw_DebugMenu, "First person", &mw_DbgFirstPerson);
	CTweakMenu_AddBool(mw_DebugMenu, "hold outro", &mw_DbgHoldOutro);
	CTweakMenu_AddFloat(mw_DebugMenu, "Floaty Score Spd", MW_FV(0x0030a0d4), 0.1f, 0.1f, 32.0f);
	CTweakMenu_AddFloat(mw_DebugMenu, "Floaty Duration", MW_FV(0x0030a0d0), 0.1f, 0.1f, 5.0f);
	MW_F(0x0030a28c) = 1.0f;
	CTweakMenu_AddFloat(mw_DebugMenu, "Gib Amount", MW_FV(0x0030a28c), 1.0f, 1.0f, 50.0f);
	CTweakMenu_AddFloat(mw_DebugMenu, "ai awareness", MW_AV(mw_Players[0], 0x133c), 0.1f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(mw_DebugMenu, "ai smartness", MW_AV(mw_Players[0], 0x1340), 0.1f, 0.0f, 1.0f);
	CTweakMenu_AddFloat(mw_DebugMenu, "ai control", MW_AV(mw_Players[0], 0x1344), 0.1f, 0.0f, 1.0f);
	if (mw_Variation == 3) {
		MW_F(0x0030ebec) = 7.0f;
		CTweakMenu_AddFloat(mw_DebugMenu, "Num Spray Particals", MW_FV(0x0030ebec), 1.0f, 1.0f, 50.0f);
	}
	for (i = 1; i < 4; i++) {
		MW_AF(mw_Players[i], 0x133c) = MW_AF(mw_Players[0], 0x133c);
		MW_AF(mw_Players[i], 0x1340) = MW_AF(mw_Players[0], 0x1340);
		MW_AF(mw_Players[i], 0x1344) = MW_AF(mw_Players[0], 0x1344);
	}
	CLRumble_CreateMenu((int *)DAT_002ed324);
	switch (mw_Variation) {
	case 0:
		FUN_001708b0(mw_Arena, mw_LevelPackage, "introcams\\subway.lom");
		MW_S(0x0030ce02) = 0;
		mw_LoadTrain(0x6fb);
		mw_Camera = mw_NewCamera(0x704);
		mw_CameraAddPlayers();
		MW_FV(0x0030a2b0)[0] = 1300.0f;
		MW_FV(0x0030a2b0)[1] = 0.0f;
		MW_FV(0x0030a2b0)[2] = 600.0f;
		MW_FV(0x0030a2b0)[3] = 1.0f;
		MW_FV(0x0030a2c0)[0] = 1580.0f;
		MW_FV(0x0030a2c0)[1] = 0.0f;
		MW_FV(0x0030a2c0)[2] = -1000.0f;
		MW_FV(0x0030a2c0)[3] = 1.0f;
		MW_FV(0x0030a2d0)[0] = -988.0f;
		MW_FV(0x0030a2d0)[1] = 300.0f;
		MW_FV(0x0030a2d0)[2] = -950.0f;
		MW_FV(0x0030a2d0)[3] = 1.0f;
		MW_FV(0x0030a2e0)[0] = -988.0f;
		MW_FV(0x0030a2e0)[1] = 400.0f;
		MW_FV(0x0030a2e0)[2] = 800.0f;
		MW_FV(0x0030a2e0)[3] = 1.0f;
		FUN_000d62c0((void *)(uintptr_t)mw_Camera, (float *)(uintptr_t)MW_FV(0x0030a2b0), 0, (float *)(uintptr_t)DAT_002ea478);
		FUN_000d62c0((void *)(uintptr_t)mw_Camera, (float *)(uintptr_t)MW_FV(0x0030a2c0), 0, (float *)(uintptr_t)DAT_002ea478);
		FUN_000d62c0((void *)(uintptr_t)mw_Camera, (float *)(uintptr_t)MW_FV(0x0030a2d0), 0, (float *)(uintptr_t)DAT_002ea478);
		FUN_000d62c0((void *)(uintptr_t)mw_Camera, (float *)(uintptr_t)MW_FV(0x0030a2e0), 0, (float *)(uintptr_t)DAT_002ea478);
		MW_AF(mw_Camera, 0x4b0) = -2000.0f;
		MW_AF(mw_Camera, 0x4b4) = -2500.0f;
		MW_AF(mw_Camera, 0x4b8) = 500.0f;
		MW_AF(mw_Camera, 0x4bc) = 1.0f;
		break;
	case 1:
		mw_Camera = mw_NewCamera(0x746);
		FUN_001708b0(mw_Arena, mw_LevelPackage, "introcams\\track.lom");
		MW_S(0x0030ce02) = 0;
		mw_CameraAddPlayers();
		break;
	case 2:
		FUN_001708b0(mw_Arena, mw_LevelPackage, "introcams\\burger.lom");
		MW_S(0x0030ce02) = 0;
		mw_Camera = mw_NewCamera(0x75b);
		mw_CameraAddPlayers();
		break;
	case 3:
		FUN_001708b0(mw_Arena, mw_LevelPackage, "introcams\\subway.lom");
		MW_S(0x0030ce02) = 0;
		mw_LoadTrain(0x768);
		mw_Camera = mw_NewCamera(0x773);
		mw_CameraAddPlayers();
		MW_FV(0x0030a2b0)[0] = 1890.0f;
		MW_FV(0x0030a2b0)[1] = 0.0f;
		MW_FV(0x0030a2b0)[2] = 1300.0f;
		MW_FV(0x0030a2c0)[0] = -1147.0f;
		MW_FV(0x0030a2c0)[1] = 0.0f;
		MW_FV(0x0030a2c0)[2] = -1752.0f;
		FUN_000d62c0((void *)(uintptr_t)mw_Camera, (float *)(uintptr_t)MW_FV(0x0030a2b0), 0, (float *)(uintptr_t)DAT_002ea478);
		FUN_000d62c0((void *)(uintptr_t)mw_Camera, (float *)(uintptr_t)MW_FV(0x0030a2c0), 0, (float *)(uintptr_t)DAT_002ea478);
		break;
	}
	bkPrintf("initing projectors\n");
	CProjector_Reset();
	MW_F(0x0030eb94) = 255.0f;
}

/* delete[] of the route tables made by CModule_MouseWhack_Start (eh vector destructor
iterator: last to first, destructor 0x000ad880) */
static void mw_DeleteRouteTables(void)
{
	char *tables = (char *)mw_RouteTables;
	int count, i;

	if (tables == NULL)
		return;
	count = *(int *)(tables - 4);
	for (i = count - 1; i >= 0; i--)
		CLRouteHeights_Free2((struct CLRouteHeights *)(tables + i * 0x18));
	MW_DELETE(tables - 4);
}

/* closes the level and variation packages */
static void mw_ClosePackages(void)
{
	if (mw_LevelPackage != NULL) {
		bkPrintf("closing MouseWhack level package\n");
		FUN_0004db40((struct bkPackage *)mw_LevelPackage);
		mw_LevelPackage = NULL;
	}
	if (mw_VariationPackage != NULL) {
		bkPrintf("closing MouseWhack variationPakIndex package\n");
		FUN_0004db40((struct bkPackage *)mw_VariationPackage);
		mw_VariationPackage = NULL;
	}
}

/* delete of a CTweakMenu */
static void mw_DeleteTweakMenu(struct CTweakMenu *menu)
{
	if (menu != NULL) {
		CTweakMenu_Dtor(menu);
		MW_DELETE(menu);
	}
}

/* 0x0016f920  (gap): exit (the mode was popped): restores the near/far planes; if the game
was initialised (+0x04), saves the scores and frees the players, bugs, bullets, menus,
actor instances and packages */
void CGameMode_MouseWhack_Exit(void *self)
{
	unsigned char *node, *next, *bug;
	void *object;
	DSCAPS caps;
	int i;

	DAT_0028ffa8 = MWM_NEAR(self);
	DAT_0028ffac = MWM_FAR(self);
	if (MW_AI(self, 0x4) == 0) {
		mw_ClosePackages();
		mw_DeleteRouteTables();
		DAT_00292c78 = 0x240;
		return;
	}
	CLRumble_Shutdown((int *)DAT_002ed324);
	mw_Quit = 1;
	DAT_00292c78 = 0x240;
	mw_DeleteRouteTables();
	FUN_00182c10();
	CLDebris_Start();
	for (i = 0; i < 4; i++)
		DAT_002baee8[i * 10] = MW_AI(mw_Players[i], 0x1400);
	for (i = 0; i < 4; i++) {
		if ((char *)mw_Players[i] + 0x1140 != NULL)
			baDeleteActorInstance((struct BActorInstance *)((char *)mw_Players[i] + 0x1140));
	}
	for (i = 0; i < DAT_002bae94; i++) {
		if (MW_AP(mw_Players[i], 0x5c) != NULL) {
			biDeleteMap((struct biMap *)MW_AP(mw_Players[i], 0x5c));
			object = mw_Players[i];
			if (object != NULL) {
				FUN_0017d1f0(object);
				MW_DELETE(object);
			}
			DAT_002f289c[i] = NULL;
		}
	}
	object = mw_Camera;
	if (object != NULL) {
		FUN_000d64a0((void *)(uintptr_t)object);
		MW_DELETE(object);
	}
	mw_DeleteTweakMenu(mw_DebugMenu);
	if (mw_Variation == 0 || mw_Variation == 1)
		mw_DeleteTweakMenu(mw_TremorMenu);
	mw_DeleteTweakMenu(mw_ImpactMenu);
	mw_DeleteTweakMenu(mw_ScoreMenu);
	object = mw_Bullets;
	if (object != NULL) {
		FUN_00180c50(object);
		MW_DELETE(object);
	}
	baDeleteExtraSystem(NULL);
	CLIntroCam_Destroy();
	/* the smashable props (CBBash_Destructable; list 0x0030ed00, node at +0x1b8) */
	node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, NULL);
	while (node != NULL) {
		next = (unsigned char *)(uintptr_t)CLList_Remove((int *)DAT_0030ed00, (int *)node);
		if (node - 0x1b8 != NULL) {
			FUN_00181520(node - 0x1b8);
			MW_DELETE(node - 0x1b8);
		}
		node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, (unsigned int *)next);
	}
	FUN_00182c90();
	for (i = 0; i < 10; i++) {
		object = mw_Helpers[i];
		if (object != NULL) {
			FUN_001d5950();
			MW_DELETE(object);
		}
	}
	/* the bugs (list 0x0030ee30, node at +8) */
	node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL);
	while (node != NULL) {
		next = (unsigned char *)(uintptr_t)CLList_Remove((int *)DAT_0030ee30, (int *)node);
		bug = node - 8;
		if (MW_AS(bug, 0x11ec) == 8)
			Platform_Delete(MW_AP(bug, 0x4));
		else
			Actor_Delete(MW_AP(bug, 0x0));
		FUN_001817d0(bug);
		MW_DELETE(bug);
		node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)next);
	}
	if (mw_Light != NULL)
		baDeleteLight((struct BLight *)mw_Light);
	if (mw_Light2 != NULL)
		baDeleteLight((struct BLight *)mw_Light2);
	if (mw_Variation == 2) {
		for (i = 0; i < 4; i++) {
			baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030c810 + 0x1c20 + i * 0x170));
			baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030c810 + i * 0x170));
		}
		for (i = 0; i < 3; i++)
			baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030ce80 + i * 0x170));
		baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030c390));
		baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030c510));
		baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030c680));
	}
	for (i = 0; i < 3; i++)
		baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030d5b0 + i * 0x170));
	for (i = 0; i < 4; i++)
		baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030da00 + i * 0x170));
	for (i = 0; i < 3; i++)
		baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030dfc0 + i * 0x170));
	FUN_000ba430();
	FUN_000b9b80();
	Actor_DeleteAll();
	COverlay_DeleteAll();
	CProjector_DeleteAll();
	Platforms_DeleteAll();
	if (mw_WallSet != NULL)
		bkDeleteResource((struct bkResource *)mw_WallSet);
	mw_WallSet = NULL;
	bkPrintf("miniSet deleted\n");
	baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030a340));
	mw_ClosePackages();
	if (mw_Variation == 3) {
		for (i = 0; i < 4; i++) {
			baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030bda0 - 0x5e0 + i * 0x170));
			baDeleteActorInstance((struct BActorInstance *)MW_A(0x0030bda0 + i * 0x170));
		}
	}
	RefBot_Delete();
	FUN_000be460();
	IDirectSound_GetCaps((struct IDirectSound *)DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", (int)caps.dwFree3DBuffers);
}

/* 0x0016fe50  (gap): draw: the arena sets, the variation's props (burger stands, traps),
the players and their weapons, the bugs, bullets, splats, shadows, HUD and fades */
void CGameMode_MouseWhack_Draw(void *self, int top)
{
	char name[64];
	float matrix[16];
	float target[4];
	unsigned char *node, *player;
	const char *format;
	int i;

	(void)self;
	if (top == 0 || mw_Quit != 0)
		return;
	baSetAmbientLight(0x7f, 0x7f, 0x7f);
	bdSetRenderState(8, 0, 0);
	baEnableLight((struct BLight *)mw_Light, 0);
	baSetLightColour((struct BLight *)mw_Light, 0x7f, 0x7f, 0x46);
	baEnableLight((struct BLight *)mw_Light, 1);
	bdSetRenderState(3, 1, 0);
	bdSetWorldIdentity();
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(0xd, 0, 1);
	bdSetProjectionMode(1);
	if (mw_Variation == 2)
		bdSetRenderState(3, 0, 0);
	if (mw_ArenaSet != NULL) {
		bdSetRenderState(4, 1, 0);
		baDrawSet((struct BSet *)mw_ArenaSet, 1);
		baDrawSet((struct BSet *)mw_ArenaSet, 2);
		baDrawSet((struct BSet *)mw_ArenaSet, 4);
		bdSetRenderState(4, 0, 0);
	}
	if (mw_WallSet != NULL) {
		bdSetRenderState(4, 1, 0);
		baDrawSet((struct BSet *)mw_WallSet, 1);
		bdSetRenderState(4, 0, 0);
	}
	if (mw_Variation == 2)
		bdSetRenderState(3, 1, 0);
	if (mw_DbgSpeedLock != 0)
		DAT_0028fda8 = 1.0f;
	if (mw_Variation == 2) {
		/* players without a burger stand are moved out of sight while the arena draws */
		for (i = 0; i < 4; i++) {
			if (MW_S(0x0030ce6c + i * 2) == 0)
				MW_AF(MW_AP(mw_Players[i], 0x30), 0xd4) = -500.0f;
		}
	}
	for (i = 0; i < 10; i++)
		FUN_001d5950(); /* the helpers' empty draw */
	bdSetRenderState(3, 1, 0);
	bdSetWorldMatrix(DAT_002855f0);
	bdSetRenderState(5, 1, 0);
	if (mw_Variation == 2) {
		for (i = 0; i < 4; i++) {
			baPrepActorInstance((struct BActorInstance *)MW_A(0x0030e430 + i * 0x170), 1);
			baDrawActorInstance((struct BActorInstance *)MW_A(0x0030e430 + i * 0x170), 7);
		}
		baPrepActorInstance((struct BActorInstance *)MW_A(0x0030c510), 1);
		baPrepActorInstance((struct BActorInstance *)MW_A(0x0030c680), 1);
		baPrepActorInstance((struct BActorInstance *)MW_A(0x0030c390), 1);
		if (DAT_002ecdec == 0) {
			baUpdateAnimations((struct BActorInstance *)MW_A(0x0030c390), (struct BActorNode *)MW_P(0x0030c4c4), DAT_0028fda8, NULL);
			baUpdateAnimations((struct BActorInstance *)MW_A(0x0030c510), (struct BActorNode *)MW_P(0x0030c644), DAT_0028fda8, NULL);
			baUpdateAnimations((struct BActorInstance *)MW_A(0x0030c680), (struct BActorNode *)MW_P(0x0030c7b4), DAT_0028fda8, NULL);
		}
		baDrawActorInstance((struct BActorInstance *)MW_A(0x0030c390), 1);
		baDrawActorInstance((struct BActorInstance *)MW_A(0x0030c390), 6);
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 1, 0);
		baDrawActorInstance((struct BActorInstance *)MW_A(0x0030c510), 1);
		bdSetRenderState(10, 0, 0);
		baDrawActorInstance((struct BActorInstance *)MW_A(0x0030c680), 1);
		bdSetRenderState(10, 1, 0);
		baDrawActorInstance((struct BActorInstance *)MW_A(0x0030c510), 6);
		bdSetRenderState(10, 0, 0);
		baDrawActorInstance((struct BActorInstance *)MW_A(0x0030c680), 6);
	}
	Actor_PrepAll();
	bdSetWorldMatrix(DAT_002855f0);
	bdSetProjectionMode(1);
	/* the players' weapons: weapon instance (+0x1140) placed at the hand node */
	for (i = 0; i < 4; i++) {
		player = (unsigned char *)mw_Players[i];
		baPrepActorInstance((struct BActorInstance *)(player + 0x1140), 1);
		MW_AI(player, 0x1208) = 0;
		MW_AI(player, 0x1204) = 0;
		MW_AI(player, 0x1200) = 0;
		mw_MatrixMultiply(matrix, (float *)((char *)MW_AP(player, 0x12b4) + 0x80), (float *)(player + 0x1140));
		mw_MatrixMultiply((float *)(player + 0x1140), (float *)((char *)MW_AP(player, 0x30) + 0x10), matrix);
		baDrawActorInstance((struct BActorInstance *)(player + 0x1140), 1);
	}
	Actor_DrawFirstPass(mw_ArenaActor);
	MW_AU(mw_ArenaActor, 0x220) |= 0x100000;
	MouseWhack_DrawActorsFirstPass();
	MW_AU(mw_ArenaActor, 0x220) &= 0xffefffff;
	/* type 4 bugs (spores) draw their extra instance */
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
		if (MW_AS(node, 0x11f2) != 0 && MW_AS(node, 0x11e4) == 4 && MW_S(0x0030ce00) != 2)
			baDrawActorInstance((struct BActorInstance *)(node + 0x1068), 7);
	}
	if (mw_Variation == 3) {
		/* the electric traps: an animated texture and 4 pairs of instances */
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 1, 0);
		if (DAT_002ecdec == 0)
			MW_F(0x0030c374) = MW_F(0x0030c37c) * DAT_0028fda8 + MW_F(0x0030c374);
		if ((float)(int)MW_S(0x0030c378) < MW_F(0x0030c374))
			MW_F(0x0030c374) = 0.0f;
		bdTextureSetFrame((struct bdTexture *)MW_P(0x0030c370), MW_FTOL(MW_F(0x0030c374)));
		for (i = 0; i < 4; i++) {
			char *trap = MW_A(0x0030b7c0) + i * 0x170;

			baPrepActorInstance((struct BActorInstance *)trap, 1);
			baPrepActorInstance((struct BActorInstance *)(trap + 0x5e0), 1);
			if (DAT_002ecdec == 0) {
				baUpdateAnimations((struct BActorInstance *)trap, (struct BActorNode *)MW_AP(trap, 0x134), DAT_0028fda8, NULL);
				baUpdateAnimations((struct BActorInstance *)(trap + 0x5e0), (struct BActorNode *)MW_AP(trap, 0x714),
					DAT_0028fda8, NULL);
			}
			baDrawActorInstance((struct BActorInstance *)trap, 1);
			baDrawActorInstance((struct BActorInstance *)(trap + 0x5e0), 6);
		}
		bdSetRenderState(10, 0, 0);
	}
	bdSetWorldMatrix(DAT_002855f0);
	/* the AI rings */
	for (i = 0; i < 4; i++) {
		unsigned char *actor = (unsigned char *)MW_AP(mw_Players[i], 0x30);

		if (mw_Variation == 3) {
			target[0] = 0.0f;
			target[1] = 1.0f;
			target[2] = 600.0f;
			target[3] = 0.0f;
			bmVectorRotateY(target, target, MW_AF(actor, 0x1e8));
			actor = (unsigned char *)MW_AP(mw_Players[i], 0x30);
			target[0] = target[0] + MW_AF(actor, 0xd0);
			target[1] = target[1] + MW_AF(actor, 0xd4);
			target[2] = target[2] + MW_AF(actor, 0xd8);
			CLHud_DrawAIRing(i, MW_AV(actor, 0xd0), target);
		} else if (MW_S(0x0030ce6c + i * 2) != 0) {
			CLHud_DrawAIRing(i, MW_AV(actor, 0xd0), NULL);
		}
	}
	CLDebris_Stop();
	baDrawExtras(NULL, NULL);
	FUN_00170c00(mw_Arena);
	if (MW_S(0x0030ce02) == 0 && (mw_Variation == 0 || mw_Variation == 3)) {
		baPrepActorInstance((struct BActorInstance *)MW_A(0x0030a340), 1);
		baDrawActorInstance((struct BActorInstance *)MW_A(0x0030a340), 7);
	}
	bdSetRenderState(3, 1, 0);
	if (mw_Variation == 2) {
		/* the burger stands; players without one come back up */
		for (i = 0; i < 4; i++) {
			char *stand = MW_A(0x0030c810) + i * 0x170;

			if (MW_S(0x0030ce6c + i * 2) == 0)
				MW_AF(MW_AP(mw_Players[i], 0x30), 0xd4) = MW_AF(MW_AP(mw_Players[i], 0x30), 0xd4) + 1000.0f;
			if (DAT_002ecdec == 0)
				baUpdateAnimations((struct BActorInstance *)stand, (struct BActorNode *)MW_AP(stand, 0x134), DAT_0028fda8, NULL);
			baPrepActorInstance((struct BActorInstance *)stand, 1);
			baDrawActorInstance((struct BActorInstance *)stand, 7);
		}
	}
	FUN_001d5950(); /* empty */
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(6, 1, 0);
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, (unsigned int *)node))
		FUN_00181040(node - 0x1b8);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(3, 0, 0);
	for (i = 0; (short)i < 8; i++)
		FUN_001836b0((short)i);
	FUN_00183300();
	Actor_DrawOpaque(mw_ArenaActor);
	MW_AU(mw_ArenaActor, 0x220) |= 0x100000;
	MouseWhack_DrawActors();
	MW_AU(mw_ArenaActor, 0x220) &= 0xffefffff;
	bdSetRenderState(5, 1, 0);
	if (mw_DbgShadows != 0)
		FUN_00186b00();
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(4, 0, 0);
	MouseWhack_DrawBugShadows();
	bdSetRenderState(3, 0, 0);
	FUN_00182f50();
	FUN_00182ea0();
	bdSetRenderState(3, 1, 0);
	FUN_001853e0();
	FUN_00185110();
	/* the screen shot button of player 0 (its pad object +0x14) */
	if (MW_AI(MW_AP(mw_Players[0], 0x70), 0x14) != 0) {
		format = NULL;
		switch (mw_Variation) {
		case 0:
			format = "BRAWL%03d.BMP";
			break;
		case 1:
			format = "TRAIN%03d.BMP";
			break;
		case 2:
			format = "BURGE%03d.BMP";
			break;
		case 3:
			format = "PIPER%03d.BMP";
			break;
		}
		if (format != NULL)
			sprintf(name, format, (int)MW_S(0x0030ea10));
		MW_S(0x0030ea10) = MW_S(0x0030ea10) + 1;
		MW_AI(MW_AP(mw_Players[0], 0x70), 0x14) = 0;
		bdScreenShot(name);
	}
	bdSetRenderState(5, 1, 0);
	if (DAT_002ecdec == 0)
		CProjector_UpdateAll();
	if (mw_DbgOverlays != 0)
		COverlay_DrawAll(1);
	if (mw_Variation == 2)
		CLHud_DrawScoreBars((int)(uintptr_t)DAT_002ecfac, MW_F(0x0030e9f0), MW_F(0x0030e9f4), MW_F(0x0030e9f8),
			MW_F(0x0030e9fc), 1.0f);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(7, 3, 0);
	bdSetRenderState(6, 1, 0);
	if (MW_S(0x0030ce02) == 1) {
		bdSetProjectionMode(1);
		bdSetCameraLookAt(MW_AV(mw_Camera, 0x534), MW_AV(mw_Camera, 0x544), 0.0f);
		bdUpdateView();
	}
	if ((MW_S(0x0030ce00) == 2 && mw_GameState == 2) ||
	    (MW_S(0x0030ce04) == 2 && mw_GameState == 2 && DAT_002ecdec != 0))
		CLScreen_DrawDarken(MW_FTOL(MW_F(0x0030eb94)), 1);
	if (MW_S(0x0030ce00) == 0)
		CLScreen_DrawDarken(MW_FTOL(MW_F(0x0030eb94)), 1);
	if (MW_S(0x0030ce00) == 2 && mw_GameState == 1) {
		if (CLFade_Draw(DAT_0028fda8) == 1)
			MW_S(0x0030eb98) = 1;
	} else if (MW_S(0x0030ce04) == 2 && mw_GameState == 1 && DAT_002ecdec != 0) {
		if (CLFade_Draw(0.0f) == 1)
			MW_S(0x0030eb98) = 1;
	}
}
