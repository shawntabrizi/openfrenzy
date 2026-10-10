/*
Clusteroids/unplaced.c  -  gap functions of C:\BlitzParty\Game\Src\Clusteroids\ without a
known source file (0x000dc9d0-0x000ded70, between CClust_Trails.cpp and CClust_Player.cpp).

The game's frame (ClustGame_Update), the scoring per variant, the end of the round
(winner, the camera on the winner, replay or exit), the intro and its door/spinner/grid
animations, the camera (the CIntroCam path "camera\cam<n>.lom" and the debug follow
camera), the countdown and frame timers, the sounds and the voice lines.

Globals:
  0x002f7f7c  short  state of the intro (phase 1) or of the end of the round (phase 3)
  0x002f7f7e  short  the winner from CLHud_CreateWinnerText (-1: a draw)
  0x002f9ac0  int    number of winners to show; 0x002f9ac4[4] the winners
  0x002f9ad4  int    the player the camera shows; 0x002f9adc set when it changes
  0x002f9ad8  int    fade-out alpha; 0x002f9ae0 fade-in alpha
  0x002f9ae4[4] / 0x002f9af4[4]  "Fire" / "Boost" state of the previous frame
  0x002fa328-0x002fa35c  the countdown timer, 0x002fa361-0x002fa37c / 0x002fae80-0x002fae9c
  the frame timer (both stop while the game is paused, DAT_002ecdec)
  0x002fca18[30] sample play counts, 0x002fca98[30] the samples
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define CL_ACTOR(addr) ((struct BActorInstance *)CL_ADDR(addr))
/* the animation root node of an actor instance (+0x134) */
#define CL_ACTOR_NODE(addr) CL_VAR(struct BActorNode *, (addr) + 0x134)
/* the input channel state (+0x14) */
#define CL_CHANNEL_ON(channel) CL_I32((channel), 0x14)

/* the debris types of the score pop-ups (0x2c bytes each from 0x00296d80) */
#define CL_DEBRIS_SCORE(k) ((struct DebrisType *)((char *)CL_ADDR(0x00296d80) + (k) * 0x2c))

extern LPDIRECTSOUND DAT_002db808;  /* the DirectSound object (engine_misc) */
extern int DAT_002baeb8;            /* skip the intro (attract mode / demo) (core) */
extern int DAT_002bae6c;            /* the sound effects volume (core) */
extern int DAT_002bae98;            /* the game type: 2 = single minigame (core) */
extern int DAT_002baec0[];          /* team members, [i] (core) */
extern int DAT_002baef4[];          /* the team of player i: [i * 10] (core) */
extern int DAT_002baee0[];          /* input port flags of player i: [i * 10] (core) */

/* the character colour of player i as 0x00rrggbb */
static unsigned int cl_player_colour(int player)
{
	int character = DAT_002baed4[player * 10] * 0x60;

	return ((unsigned int)DAT_00290718[character] << 16) |
		((unsigned int)DAT_00290718[character + 1] << 8) | DAT_00290718[character + 2];
}

/* 0x000dc9d0  Clusteroids (gap)
The player with the highest score among those still in, or -1 when another player has
the same score. */
int ClustGame_GetWinner(void)
{
	short best;
	short bestPlayer;
	short lowest;
	short lowestPlayer;
	short second;
	short other;
	short score;
	short i;

	best = -1;
	bestPlayer = 0;
	lowest = 99;
	lowestPlayer = 0;
	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0) {
			score = CL_I16(CL_PLAYER(i), PL_SCORE);
			if (best < score) {
				best = score;
				bestPlayer = i;
			}
			if (score < lowest) {
				lowest = score;
				lowestPlayer = i;
			}
		}
	}
	second = -1;
	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0 && bestPlayer != i &&
			second < CL_I16(CL_PLAYER(i), PL_SCORE))
			second = CL_I16(CL_PLAYER(i), PL_SCORE);
	}
	/* the original also finds the second lowest score; the result is not used */
	other = 99;
	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0 && lowestPlayer != i &&
			CL_I16(CL_PLAYER(i), PL_SCORE) < other)
			other = CL_I16(CL_PLAYER(i), PL_SCORE);
	}
	(void)other;
	if (second < best)
		return (int)bestPlayer;
	return -1;
}

/* 0x000dcab0  Clusteroids (gap)
The highest score of the players still in (-1: none). */
int ClustGame_GetHighestScore(void)
{
	short best = -1;
	short lowest = 99;
	short score;
	int i;

	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0) {
			score = CL_I16(CL_PLAYER(i), PL_SCORE);
			if (best < score)
				best = score;
			if (score < lowest)
				lowest = score;
		}
	}
	return (int)best;
}

/* 0x000dcaf0  Clusteroids (gap)
The lowest score of the players still in (99: none). */
int ClustGame_GetLowestScore(void)
{
	short best = -1;
	short lowest = 99;
	short score;
	int i;

	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_OUT) == 0) {
			score = CL_I16(CL_PLAYER(i), PL_SCORE);
			if (best < score)
				best = score;
			if (score < lowest)
				lowest = score;
		}
	}
	return (int)lowest;
}

/* a score pop-up of type `kind` (1-3) over `pos` (x at [0], z at [1]) in player's colour */
static void cl_score_popup(int player, int kind, float x, float z)
{
	float pos[4];
	float vel[4];

	pos[0] = x;
	pos[1] = 12.0f;
	pos[2] = z;
	pos[3] = 0.0f;
	vel[0] = 0.0f;
	vel[1] = 6.0f;
	vel[2] = 0.0f;
	vel[3] = 0.0f;
	Debris_Add(CL_DEBRIS_SCORE(kind), pos, vel, 0, cl_player_colour(player));
}

/* 0x000dcb30  Clusteroids (gap)
The scores of the frame, by variant:
  Tails      score = tail length (+0x650)
  Pod Hog    every 20 frames a player holding pods (+0x650 = 1, 2, 3) gains 1, 3 or 5
  Mines      every 20 frames a player with a pod gains 1
  Collector  a delivery (+0xc = 1, 2, 3) gains 1, 3 or 5, sometimes with a voice line
Scores never go below 0. */
void ClustGame_UpdateScores(void)
{
	char *pl;
	float timer;
	short kind;
	int i;
	float pos[4];

	switch (g_ClVariant) {
	case 1:
		CL_I16(CL_PLAYER(0), PL_SCORE) = CL_I16(CL_PLAYER(0), 0x650);
		CL_I16(CL_PLAYER(1), PL_SCORE) = CL_I16(CL_PLAYER(1), 0x650);
		CL_I16(CL_PLAYER(2), PL_SCORE) = CL_I16(CL_PLAYER(2), 0x650);
		CL_I16(CL_PLAYER(3), PL_SCORE) = CL_I16(CL_PLAYER(3), 0x650);
		break;
	case 2:
		for (i = 0; i < 4; i++) {
			pl = CL_PLAYER(i);
			kind = CL_I16(pl, 0x650);
			if (kind != 0) {
				timer = g_ClFrameTime + CL_F32(pl, 0x8);
				CL_F32(pl, 0x8) = timer;
				if (timer >= 20.0f) {
					CL_F32(pl, 0x8) = timer - 20.0f;
					if (kind == 1)
						CL_I16(pl, 0x4) = CL_I16(pl, 0x4) + 1;
					else if (kind == 2)
						CL_I16(pl, 0x4) = CL_I16(pl, 0x4) + 3;
					else if (kind == 3)
						CL_I16(pl, 0x4) = CL_I16(pl, 0x4) + 5;
					cl_score_popup(i, kind, CL_F32(CL_PTR(pl, 0x674), 0x10),
						CL_F32(CL_PTR(pl, 0x674), 0x14));
				}
			}
			CL_I16(pl, PL_SCORE) = (short)(CL_I16(pl, 0x4) - (short)CL_FTOL(CL_F32(pl, 0x8) * -0.05f));
		}
		break;
	case 3:
		for (i = 0; i < 4; i++) {
			pl = CL_PLAYER(i);
			kind = CL_I16(pl, 0x650);
			if (kind != 0) {
				timer = g_ClFrameTime + CL_F32(pl, 0x8);
				CL_F32(pl, 0x8) = timer;
				if (timer >= 20.0f) {
					CL_I16(pl, 0x4) = CL_I16(pl, 0x4) + 1;
					CL_F32(pl, 0x8) = timer - 20.0f;
					cl_score_popup(i, CL_I16(pl, 0x650), CL_F32(CL_PTR(pl, 0x674), 0x10),
						CL_F32(CL_PTR(pl, 0x674), 0x14));
				}
			}
			CL_I16(pl, PL_SCORE) = (short)(CL_I16(pl, 0x4) - (short)CL_FTOL(CL_F32(pl, 0x8) * -0.05f));
		}
		break;
	case 4:
		for (i = 0; i < 4; i++) {
			pl = CL_PLAYER(i);
			kind = CL_I16(pl, 0xc);
			if (kind == 0)
				continue;
			if (kind == 1)
				CL_I16(pl, PL_SCORE) = CL_I16(pl, PL_SCORE) + 1;
			else if (kind == 2)
				CL_I16(pl, PL_SCORE) = CL_I16(pl, PL_SCORE) + 3;
			else if (kind == 3)
				CL_I16(pl, PL_SCORE) = CL_I16(pl, PL_SCORE) + 5;
			if (0.9f < (float)cl_rand() * 3.0518509e-05f) {
				pos[0] = CL_F32(pl, PL_X);
				pos[2] = CL_F32(pl, PL_Z);
				pos[1] = 0.0f;
				pos[3] = 0.0f;
				ClustSound_PlayVoice(i, 4);
			}
			if (CL_I16(pl, PL_SCORE) < 0)
				CL_I16(pl, PL_SCORE) = 0;
			cl_score_popup(i, CL_I16(pl, 0xc), CL_F32(pl, PL_X), CL_F32(pl, PL_Z));
			CL_I16(pl, 0xc) = 0;
			ClustSound_Play(0xd, 100);
		}
		break;
	}
	for (i = 0; i < 4; i++) {
		if (CL_I16(CL_PLAYER(i), PL_SCORE) < 0)
			CL_I16(CL_PLAYER(i), PL_SCORE) = 0;
	}
}

/* 0x000dcfa0  Clusteroids (gap)
A frame of play (phase 2): the players, the bullets, the ships' trails, the AI,
the camera target, the explosions, then the variant's pods/mines/gates and the scores.
Tails also spawns a new pod every 10 seconds of the countdown while 0x002f7f90 lasts. */
void ClustGame_UpdatePlaying(void)
{
	int i;
	char *pl;

	for (i = 0; i < 4; i++)
		FUN_000dfc90(i);
	FUN_000e7210();
	CL_VAR(short, 0x002f8f5e) = 0;
	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		if (CL_I16(pl, PL_OUT) == 0 && ClustTrails_Add(pl, (short)(i + 1)) == NULL)
			CL_VAR(short, 0x002fa234) = 1;
	}
	FUN_000e6d70();
	ClustCamera_UpdateTarget();
	FUN_000e47f0();
	if (ClustCountdown_GetRemaining() <= 10.0f)
		FUN_001d5950();
	if (g_ClVariant == 1 || g_ClVariant == 3) {
		FUN_000e73f0();
		FUN_000e4ea0();
		FUN_000e3fb0();
		if (g_ClVariant == 1 &&
			ClustCountdown_GetRemaining() <= (float)(CL_VAR(int, 0x002f7f90) * 10) &&
			0 < CL_VAR(int, 0x002f7f90)) {
			FUN_000e39d0();
			CL_VAR(int, 0x002f7f90) = CL_VAR(int, 0x002f7f90) - 1;
		}
	} else if (g_ClVariant == 2) {
		FUN_000e73f0();
		FUN_000e4ea0();
		FUN_000e3fb0();
		ClustGame_UpdateScores();
		return;
	} else if (g_ClVariant == 4) {
		FUN_000e73f0();
		FUN_000e4bd0();
		FUN_000e3fb0();
		FUN_000e51b0();
		FUN_000e3e20();
		ClustGame_UpdateScores();
		return;
	}
	ClustGame_UpdateScores();
}

/* end of the round: the winner text and the winners' cheer */
static void cl_end_round(void)
{
	int scores[4];

	scores[0] = (int)CL_I16(CL_PLAYER(0), PL_SCORE);
	scores[1] = (int)CL_I16(CL_PLAYER(1), PL_SCORE);
	scores[2] = (int)CL_I16(CL_PLAYER(2), PL_SCORE);
	scores[3] = (int)CL_I16(CL_PLAYER(3), PL_SCORE);
	g_ClPhase = 3;
	CL_VAR(short, 0x002f7f7c) = 0;
	CL_VAR(int, 0x002f9abc) = 1;
	CL_VAR(short, 0x002f7f7e) = (short)CLHud_CreateWinnerText(DAT_002ecfac, scores, 0);
	FUN_000df7f0();
}

/* the debug buttons: put the camera above player `player` */
static int cl_debug_camera(struct biChannel *channel, int player, float height, float back)
{
	char *pl = CL_PLAYER(player);
	int on = CL_CHANNEL_ON(channel) != 0;

	if (on) {
		CL_VAR(float, 0x00295e10) = CL_F32(pl, PL_Z) - back;
		CL_VAR(float, 0x00295e08) = CL_F32(pl, PL_X);
		CL_VAR(float, 0x00295e0c) = height;
		CL_VAR(float, 0x002f9b24) = CL_F32(pl, PL_X);
		CL_VAR(float, 0x002f9b28) = 0.0f;
		CL_VAR(float, 0x002f9b2c) = CL_F32(pl, PL_Z);
	}
	return on;
}

/* 0x000dd0d0  Clusteroids (gap)
The game's frame: overlays, projectors, debris, trails, referee bot, AI tweaks, the
blinking colours, the animations, the phase (intro, play, end), the walkers and lights,
the end of the round (time up or at most one player left) and the debug cameras. */
void ClustGame_Update(void)
{
	char *pl;
	char *actor;
	int i;
	unsigned char out;

	COverlay_UpdateAll();
	CProjector_UpdateAll();
	Debris_UpdateSystems(g_ClFrameTime * 0.016666668f);
	ClustTrails_Update();
	RefBot_Update();
	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		CL_F32(pl, 0x824) = (DAT_002baed8[i * 10] != 0 ? CL_VAR(float, 0x002f7d74)
			: CL_VAR(float, 0x002f7d78)) * 32768.0f;
		CL_U32(pl, 0x828) = CL_VAR(unsigned int, 0x002f7d7c);
		CL_U32(pl, 0x82c) = CL_VAR(unsigned int, 0x002f7d80);
		CL_U32(pl, 0x830) = CL_VAR(unsigned int, 0x002f7d84);
		CL_U32(pl, 0x834) = CL_VAR(unsigned int, 0x002f7d88);
		CL_U32(pl, PL_WIND) = CL_U32(pl, PL_X);
		CL_U32(pl, PL_WIND + 4) = 0;
		CL_U32(pl, PL_WIND + 8) = CL_U32(pl, PL_Z);
		CL_U32(pl, PL_WIND + 12) = 0;
	}
	CL_VAR(float, 0x002f9098) = 0.35f;
	CL_VAR(float, 0x002f9a08) = CL_VAR(float, 0x002f9a08) + g_ClFrameTime;
	CL_VAR(float, 0x002f9094) = 0.35f;
	CL_VAR(float, 0x002f9090) = 0.35f;
	if (10.0f < CL_VAR(float, 0x002f9a08)) {
		CL_VAR(int, 0x002f9a04) = CL_VAR(int, 0x002f9a04) + 1;
		if (0xb < CL_VAR(int, 0x002f9a04))
			CL_VAR(int, 0x002f9a04) = 0;
		CL_VAR(float, 0x002f9a08) = 0.0f;
	}
	CL_VAR(float, 0x002f8f54) = CL_VAR(float, 0x002f8f54) - 0.01f;
	if (CL_VAR(float, 0x002f8f54) <= 0.0f)
		CL_VAR(float, 0x002f8f54) = 1.0f;
	CL_VAR(float, 0x002f8f58) = CL_VAR(float, 0x002f8f58) - 0.003f;
	if (CL_VAR(float, 0x002f8f58) <= 0.0f)
		CL_VAR(float, 0x002f8f58) = 1.0f;
	for (i = 0; i < 4; i++) {
		actor = CL_PLAYER(i) + PL_CHAR;
		baUpdateAnimations((struct BActorInstance *)actor, CL_AT(actor, 0x134, struct BActorNode *),
			g_ClFrameTime, NULL);
		baUpdateAnimations((struct BActorInstance *)(actor + 0x170),
			CL_AT(actor, 0x170 + 0x134, struct BActorNode *), g_ClFrameTime, NULL);
	}
	FUN_000e2d40();
	for (i = 0; i < 4; i++)
		baUpdateAnimations(CL_ACTOR(0x002f87b0 + i * 0x170), CL_ACTOR_NODE(0x002f87b0 + i * 0x170),
			g_ClFrameTime, NULL);
	baUpdateAnimations(CL_ACTOR(0x002f8dc0), CL_ACTOR_NODE(0x002f8dc0), g_ClFrameTime, NULL);

	if (g_ClPhase == 1) {
		ClustGame_UpdateIntro();
	} else if (g_ClPhase == 2) {
		if (CL_VAR(char, 0x002fa361) == 0)
			CL_VAR(float, 0x002f8730) = g_ClFrameTime * 0.01f + CL_VAR(float, 0x002f8730);
		ClustGame_UpdatePlaying();
	} else if (g_ClPhase == 3) {
		ClustGame_UpdateEnd();
	} else {
		goto no_phase;
	}
	if (CL_VAR(char, 0x002fa361) == 0)
		CL_VAR(float, 0x002f8790) = g_ClFrameTime * 7.5f + CL_VAR(float, 0x002f8790);
no_phase:
	if (CL_VAR(int, 0x002f8db4) != 0)
		ClustWalkers_Update();
	ClustArena_UpdateLights();
	if (ClustCountdown_GetRemaining() <= 0.0f) {
		if (g_ClPhase != 2)
			goto debug_cameras;
		cl_end_round();
	}
	if (g_ClPhase == 2) {
		out = CL_I16(CL_PLAYER(0), PL_OUT) != 0;
		if (CL_I16(CL_PLAYER(1), PL_OUT) != 0)
			out = out + 1;
		if (CL_I16(CL_PLAYER(2), PL_OUT) != 0)
			out = out + 1;
		if (CL_I16(CL_PLAYER(3), PL_OUT) != 0)
			out = out + 1;
		if (2 < out) {
			cl_end_round();
			if (g_ClPhase != 2)
				goto debug_cameras;
		}
		FUN_000e2210();
	}
debug_cameras:
	CL_VAR(short, 0x002fa23c) = (short)cl_debug_camera(CL_VAR(struct biChannel *, 0x002fa264), 0, 200.0f, 200.0f);
	CL_VAR(short, 0x002fa23e) = (short)cl_debug_camera(CL_VAR(struct biChannel *, 0x002fa268), 1, 100.0f, 100.0f);
	CL_VAR(short, 0x002fa240) = (short)cl_debug_camera(CL_VAR(struct biChannel *, 0x002fa26c), 2, 100.0f, 100.0f);
	CL_VAR(short, 0x002fa242) = 0;
	CL_VAR(short, 0x002fa238) = 0;
	if (cl_debug_camera(CL_VAR(struct biChannel *, 0x002fa270), 3, 100.0f, 100.0f))
		CL_VAR(short, 0x002fa240) = 1;
	CL_VAR(int, 0x002fa280) = CL_VAR(int, 0x002fa280) + 1;
}

/* 0x000dd610  Clusteroids (gap)
Draws a dark untextured quad over the top 15% of the screen (vertices at 0x002fa298,
FVF 0x152, screen size 0x002ba928 x 0x002ba92c). */
void ClustGame_DrawScreenBar(void)
{
	float width;
	float height;

	bdSetWorldMatrix(DAT_002855f0);
	bdSetProjectionMode(0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(4, 0, 0);
	bdSetRenderState(0, 1, 0);
	/* alpha 0x80, red/green/blue 0xff: 0x002bca98, 0x002bda3c, 0x002bc88c, 0x002bd214 */
	CL_VAR(unsigned int, 0x002fa2b0) = DAT_002bc898[0x80] | DAT_002bd640[0xff] |
		DAT_002bc490[0xff] | DAT_002bce18[0xff];
	CL_VAR(unsigned int, 0x002fa2d4) = CL_VAR(unsigned int, 0x002fa2b0);
	CL_VAR(unsigned int, 0x002fa2f8) = CL_VAR(unsigned int, 0x002fa2b0);
	CL_VAR(unsigned int, 0x002fa31c) = CL_VAR(unsigned int, 0x002fa2b0);
	bdSetTexture(0, NULL);
	width = (float)DAT_002ba928;
	CL_VAR(float, 0x002fa2a0) = 0.0f;
	CL_VAR(float, 0x002fa2c4) = 0.0f;
	CL_VAR(float, 0x002fa298) = width * 0.15f - width * 0.5f;
	CL_VAR(float, 0x002fa2bc) = (width - width * 0.15f) - width * 0.5f;
	height = (float)DAT_002ba92c;
	CL_VAR(float, 0x002fa29c) = 0.15f * height - (0.5f * height - 30.0f);
	CL_VAR(float, 0x002fa2e4) = (height - 0.15f * height) - 0.5f * height;
	CL_VAR(float, 0x002fa2e8) = 0.0f;
	CL_VAR(float, 0x002fa30c) = 0.0f;
	CL_VAR(float, 0x002fa2c0) = CL_VAR(float, 0x002fa29c);
	CL_VAR(float, 0x002fa2e0) = CL_VAR(float, 0x002fa298);
	CL_VAR(float, 0x002fa304) = CL_VAR(float, 0x002fa2bc);
	CL_VAR(float, 0x002fa308) = CL_VAR(float, 0x002fa2e4);
	bdDrawPrimitiveFVF(4, CL_ADDR(0x002fa298), 4, 0x152);
	bdSetProjectionMode(1);
	bdSetRenderState(5, 1, 0);
}

/* 0x000dd7a0  Clusteroids (gap)
The game camera, unless a debug camera is on: looks at the average position of the four
ships from above, the height from the spread of the ships (0x00296ee0, at least 900). */
void ClustCamera_UpdateTarget(void)
{
	char *pl;
	float maxX;
	float minX;
	float maxZ;
	float best;
	float bestZ;
	float dx;
	float dz;
	short i;

	if (CL_VAR(short, 0x002fa23c) != 0 || CL_VAR(short, 0x002fa23e) != 0 ||
		CL_VAR(short, 0x002fa240) != 0 || CL_VAR(short, 0x002fa242) != 0)
		return;
	maxX = CL_F32(CL_PLAYER(0), PL_X);
	minX = CL_F32(CL_PLAYER(0), PL_X);
	maxZ = CL_F32(CL_PLAYER(0), PL_Z);
	CL_VAR(float, 0x002f9aa4) = 0.0f;
	CL_VAR(float, 0x00295e08) = 0.0f;
	CL_VAR(float, 0x002f9ab4) = CL_F32(CL_PLAYER(0), PL_Z);
	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		CL_VAR(float, 0x00295e08) = CL_VAR(float, 0x00295e08) + CL_F32(pl, PL_X);
		CL_VAR(float, 0x002f9aa4) = CL_VAR(float, 0x002f9aa4) + CL_F32(pl, PL_Z);
		if (maxX < CL_F32(pl, PL_X))
			maxX = CL_F32(pl, PL_X);
		if (CL_F32(pl, PL_X) < minX)
			minX = CL_F32(pl, PL_X);
		if (maxZ < CL_F32(pl, PL_Z))
			maxZ = CL_F32(pl, PL_Z);
		if (CL_F32(pl, PL_Z) < CL_VAR(float, 0x002f9ab4))
			CL_VAR(float, 0x002f9ab4) = CL_F32(pl, PL_Z);
	}
	CL_VAR(float, 0x00295e08) = CL_VAR(float, 0x00295e08) * 0.25f;
	CL_VAR(float, 0x002f9aa4) = CL_VAR(float, 0x002f9aa4) * 0.25f;
	if (CL_VAR(float, 0x002f9aa4) < -50.0f)
		CL_VAR(float, 0x002f9aa4) = -50.0f;
	CL_VAR(float, 0x002f9aac) = (minX + maxX) * 0.5f;
	CL_VAR(float, 0x002f9ab4) = CL_VAR(float, 0x002f9ab4) + maxZ;
	best = 0.0f;
	CL_VAR(float, 0x002f9ab4) = CL_VAR(float, 0x002f9ab4) * 0.5f;
	bestZ = 0.0f;
	/* the ship furthest from the screen centre in screen x (+0x50) and y (+0x54) */
	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		dx = fabsf(CL_F32(pl, 0x50) - (float)DAT_002ba928 * 0.5f);
		dz = fabsf(CL_F32(pl, 0x54) - (float)DAT_002ba92c * 0.5f);
		if (best < dx) {
			CL_VAR(short, 0x002fa284) = i;
			best = dx;
		}
		if (bestZ < dz) {
			CL_VAR(short, 0x002fa28c) = i;
			bestZ = dz;
		}
	}
	CL_VAR(short, 0x002fa288) = CL_VAR(short, 0x002fa284);
	if (best * 1.6f < 2.13f * bestZ)
		CL_VAR(short, 0x002fa288) = CL_VAR(short, 0x002fa28c);
	if (100.0f < CL_VAR(float, 0x002fa290))
		CL_VAR(float, 0x002fa290) = 100.0f;
	if (CL_VAR(float, 0x00296ee0) < 900.0f)
		CL_VAR(float, 0x00296ee0) = 900.0f;
	CL_VAR(float, 0x00295e10) = -CL_VAR(float, 0x00296ee0);
	CL_VAR(float, 0x00295e0c) = CL_VAR(float, 0x00296ee0);
	CL_VAR(float, 0x002f9b28) = -400.0f;
	CL_VAR(float, 0x002f9a9c) = CL_VAR(float, 0x00295e08);
	CL_VAR(float, 0x002f9b24) = CL_VAR(float, 0x00295e08);
	CL_VAR(float, 0x002f9b2c) = CL_VAR(float, 0x002f9aa4);
}

/* 0x000dda20  Clusteroids (gap)
Animation callback (end of the spinner's spin): the grid rises, the walkers start. */
int ClustIntro_SpinnerDone(int reason)
{
	if (reason == 0) {
		CL_VAR(int, 0x002f8db0) = 1;
		CL_VAR(int, 0x002f8db4) = 1;
		baQueueAnimation(CL_ACTOR(0x002f8c00), CL_ACTOR_NODE(0x002f8c00),
			CL_VAR(struct BAnim *, 0x002f8d7c), 0x400, -1, 0, NULL, NULL);
		ClustSound_Play(0x1a, 0xff);
	}
	return 1;
}

/* 0x000dda80  Clusteroids (gap)
Animation callback (end of the spinner's intro): spin. */
int ClustIntro_SpinnerIntroDone(int reason)
{
	if (reason == 0) {
		baQueueAnimation(CL_ACTOR(0x002f8a90), CL_ACTOR_NODE(0x002f8a90),
			CL_VAR(struct BAnim *, 0x002f8d88), 0x400, -1, 0, (int (*)())ClustIntro_SpinnerDone, NULL);
		ClustSound_Play(0x1d, 0xff);
	}
	return 1;
}

/* 0x000ddad0  Clusteroids (gap)
Animation callback (doors opened): the doors stay open. */
int ClustIntro_DoorsOpened(int reason)
{
	if (reason == 0) {
		baQueueAnimation(CL_ACTOR(0x002f8920), CL_ACTOR_NODE(0x002f8920),
			CL_VAR(struct BAnim *, 0x002f8d94), 0x400, -1, 0, NULL, NULL);
		ClustSound_Play(0x1c, 0xff);
		CL_VAR(short, 0x002f8db8) = 1;
		return 1;
	}
	return 1;
}

/* 0x000ddb30  Clusteroids (gap)
Animation callback (end of the doors' intro): open them. */
int ClustIntro_DoorsIntroDone(int reason)
{
	if (reason == 0) {
		baQueueAnimation(CL_ACTOR(0x002f8920), CL_ACTOR_NODE(0x002f8920),
			CL_VAR(struct BAnim *, 0x002f8d84), 0x400, -1, 0, (int (*)())ClustIntro_DoorsOpened, NULL);
		ClustSound_Play(0x1b, 0xff);
	}
	return 1;
}

/* 0x000ddb80  Clusteroids (gap)
Collects the players with the highest score into 0x002f9ac4 (the rest -1); returns
their number. */
unsigned int ClustGame_CollectWinners(void)
{
	int best;
	unsigned int count;
	int *winners = (int *)CL_ADDR(0x002f9ac4);

	best = ClustGame_GetHighestScore();
	if (CL_I16(CL_PLAYER(0), PL_SCORE) == best)
		winners[0] = 0;
	count = (unsigned int)(CL_I16(CL_PLAYER(0), PL_SCORE) == best);
	if (CL_I16(CL_PLAYER(1), PL_SCORE) == best)
		winners[count++] = 1;
	if (CL_I16(CL_PLAYER(2), PL_SCORE) == best)
		winners[count++] = 2;
	if (CL_I16(CL_PLAYER(3), PL_SCORE) == best)
		winners[count++] = 3;
	for (best = (int)count; best < 4; best++)
		winners[best] = -1;
	return count;
}

/* 0x000ddc10  Clusteroids (gap)
May `player` skip the end-of-round display? In team games when there is no winner team
or `player` is on it; otherwise when `player` has the best score or no human has more. */
int ClustGame_MayContinue(int player)
{
	short best;
	int i;

	best = (short)ClustGame_GetHighestScore();
	if (DAT_002baebc != 0) {
		if (CL_VAR(short, 0x002f7f7e) == -1)
			return 1;
		return DAT_002baef4[player * 10] == (int)CL_VAR(short, 0x002f7f7e);
	}
	if (best != CL_I16(CL_PLAYER(player), PL_SCORE)) {
		for (i = 0; i < 4; i++) {
			if (i != player && DAT_002baed8[i * 10] != 0 &&
				CL_I16(CL_PLAYER(player), PL_SCORE) < CL_I16(CL_PLAYER(i), PL_SCORE))
				return 0;
		}
	}
	return 1;
}

/* 0x000ddcb0  Clusteroids (gap)
Did a human press Fire or Boost (newly, 2 s into the display) to skip it? */
int ClustGame_SkipPressed(void)
{
	char *pl;
	int fire;
	int i;

	for (i = 0; i < 4; i++) {
		if (DAT_002baed8[i * 10] == 0)
			continue;
		pl = CL_PLAYER(i);
		if (ClustCountdown_GetRemaining() <= (float)(CL_VAR(int, 0x002f9ac0) * 4) - 2.0f) {
			fire = CL_CHANNEL_ON(CL_PTR(pl, PL_IN_FIRE));
			if ((CL_VAR(int, 0x002f9ae4 + i * 4) != fire && fire != 0) ||
				(CL_VAR(int, 0x002f9af4 + i * 4) != fire && CL_CHANNEL_ON(CL_PTR(pl, PL_IN_BOOST)) != 0)) {
				if (ClustGame_MayContinue(i) != 0)
					return 1;
			}
		}
	}
	return 0;
}

/* the camera on winner `who` (end of the round) */
static void cl_show_winner(int who)
{
	char *pl = CL_PLAYER(who);
	double angle;

	if (CL_VAR(int, 0x002f9ad4) != who) {
		CL_VAR(int, 0x002f9adc) = 1;
		CL_VAR(int, 0x002f9ad4) = who;
	}
	FUN_000dfb30();
	CL_VAR(float, 0x002f9b24) = CL_F32(pl, PL_X);
	CL_VAR(float, 0x002f9b2c) = CL_F32(pl, PL_Z);
	CL_VAR(float, 0x002f9b28) = 0.0f;
	CL_VAR(float, 0x00295e0c) = 40.0f;
	angle = (double)3.1415925f - (double)CL_F32(pl, 0x634);
	CL_VAR(float, 0x00295e08) = (float)(sin(angle) * 80.0 + (double)CL_F32(pl, PL_X));
	CL_VAR(float, 0x00295e10) = (float)(cos(angle) * 80.0 + (double)CL_F32(pl, PL_Z));
}

/* the index of the winner to show now: 4 s per winner */
static int cl_winner_slot(void)
{
	int count = CL_VAR(int, 0x002f9ac0);
	double elapsed;

	elapsed = (double)(float)(count * 4) - (double)ClustCountdown_GetRemaining();
	return CL_FTOL(elapsed / ((double)(count * 4) / (double)count));
}

/* the commentary is done or the time is up, and nobody skipped */
static int cl_end_display_runs(void)
{
	if (CLSamples_CommentaryIdle() != 0 && ClustCountdown_GetRemaining() <= 0.0f)
		return 0;
	return ClustGame_SkipPressed() == 0;
}

/* end of the display: fade out over a second */
static void cl_start_fade_out(void)
{
	ClustCountdown_SetDuration(1.0f);
	ClustCountdown_Start();
	CL_VAR(int, 0x002f9ad8) = 0;
	CL_VAR(short, 0x002f7f7c) = 3;
}

/* fading out; at the end replay (a single minigame outside the board game) or leave */
static void cl_fade_out(void)
{
	float remaining = ClustCountdown_GetRemaining();

	CL_VAR(int, 0x002f9ad8) = 0xff - CL_FTOL(255.0f * remaining);
	if (remaining <= 0.0f) {
		CL_VAR(short, 0x002f7f7c) = 4;
		CL_VAR(int, 0x002f9ad8) = 0xff;
	}
}

static void cl_finish(void)
{
	if (CL_VAR(int, 0x002f9abc) != 0) {
		CLHud_DeleteWinnerText(DAT_002ecfac);
		CL_VAR(int, 0x002f9abc) = 0;
	}
	CL_VAR(int, 0x002f9ad8) = 0;
	if (CGameModeStack_Find(CL_MODESTACK, "Meta") == NULL && DAT_002bae98 == 2) {
		ClustGame_Reset();
		g_ClPhase = 1;
	} else {
		g_ClGameOver = 1;
	}
}

/* 0x000ddd50  Clusteroids (gap)
Phase 3, the end of the round (state 0x002f7f7c): 0 start (a single winner for 4 s, or
each of the tied winners / the winning team for 4 s each), 1 / 2 show the winner(s)
until the commentary ends or a human skips, 3 fade out, 4 replay or leave. */
void ClustGame_UpdateEnd(void)
{
	short winner;
	short slot;
	int who;
	int i;

	if (DAT_002baebc != 0) {
		/* team game */
		switch (CL_VAR(short, 0x002f7f7c)) {
		case 0:
			ClustSound_Play(0x19, 0xff);
			if (CL_VAR(short, 0x002f7f7e) != -1) {
				CL_VAR(int, 0x002f9ac0) = 2;
				CL_VAR(short, 0x002f7f7c) = 2;
				ClustCountdown_SetDuration(8.0f);
				ClustCountdown_Start();
				ClustCountdown_GetRemaining();
				CL_VAR(int, 0x002f9ad4) = -1;
			} else {
				CL_VAR(int, 0x002f9ac0) = 4;
				CL_VAR(short, 0x002f7f7c) = 2;
				ClustCountdown_SetDuration(16.0f);
				ClustCountdown_Start();
				ClustCountdown_GetRemaining();
				CL_VAR(int, 0x002f9ad4) = -1;
			}
			break;
		case 2:
			if (!cl_end_display_runs()) {
				cl_start_fade_out();
				break;
			}
			slot = (short)cl_winner_slot();
			if (CL_VAR(short, 0x002f7f7e) != -1)
				winner = (short)DAT_002baec0[slot + CL_VAR(short, 0x002f7f7e) * 2];
			else
				winner = (short)DAT_002baec0[slot];
			cl_show_winner((int)winner);
			break;
		case 3:
			cl_fade_out();
			break;
		case 4:
			cl_finish();
			break;
		}
	} else {
		switch (CL_VAR(short, 0x002f7f7c)) {
		case 0:
			ClustSound_Play(0x19, 0xff);
			winner = (short)ClustGame_GetWinner();
			if (winner != -1) {
				CL_VAR(int, 0x002f9ac0) = 1;
				CL_VAR(int, 0x002f9ac4) = (int)winner;
				CL_VAR(short, 0x002f7f7c) = 1;
				CL_VAR(int, 0x002f9ad4) = -1;
				ClustCountdown_SetDuration(4.0f);
				ClustCountdown_Start();
			} else {
				CL_VAR(int, 0x002f9ac0) = (int)ClustGame_CollectWinners();
				CL_VAR(short, 0x002f7f7c) = 2;
				ClustCountdown_SetDuration((float)CL_VAR(int, 0x002f9ac0) * 4.0f);
				ClustCountdown_Start();
				ClustCountdown_GetRemaining();
				CL_VAR(int, 0x002f9ad4) = -1;
			}
			break;
		case 1:
			if (!cl_end_display_runs()) {
				cl_start_fade_out();
				break;
			}
			cl_show_winner((int)CL_VAR(short, 0x002f9ac4));
			break;
		case 2:
			if (!cl_end_display_runs()) {
				cl_start_fade_out();
				break;
			}
			slot = (short)cl_winner_slot();
			who = (int)CL_VAR(short, 0x002f9ac4 + slot * 4);
			cl_show_winner(who);
			break;
		case 3:
			cl_fade_out();
			break;
		case 4:
			cl_finish();
			break;
		}
	}
	for (i = 0; i < 4; i++) {
		CL_VAR(int, 0x002f9ae4 + i * 4) = CL_CHANNEL_ON(CL_PTR(CL_PLAYER(i), PL_IN_FIRE));
		CL_VAR(int, 0x002f9af4 + i * 4) = CL_CHANNEL_ON(CL_PTR(CL_PLAYER(i), PL_IN_BOOST));
	}
}

/* 0x00269648  vtable of the camera object (0x00296ee4, a CIntroCam user) */
const void *const vtable_00269648[1] = {
	(const void *)ClustCamera_DeletingDtor,
};

/* 0x000de210  Clusteroids (gap)
Loads the intro camera path of the variant ("camera\cam1.lom" or "camera\cam2.lom")
from `package` into the CIntroCam player. */
int ClustCamera_Init(void *self, int package)
{
	char name[64];
	unsigned int size;
	void *file;
	int *map;

	switch (g_ClVariant) {
	case 1:
	case 4:
		sprintf(name, "camera\\cam2.lom");
		break;
	case 2:
		sprintf(name, "camera\\cam1.lom");
		break;
	case 3:
		sprintf(name, "camera\\cam1.lom");
		break;
	default:
		/* the original leaves `name` uninitialised here */
		name[0] = 0;
		break;
	}
	file = bkLoadFile((struct bkPackage *)package, name, NULL, &size, NULL, 0);
	if (file == NULL) {
		bkPrintf("InitialiseCamera - Could not load camera LOM file [%s]\n", name);
		return 0;
	}
	map = ParseMap_Load((char *)file, 2, (int)size);
	if (map != NULL) {
		CLIntroCam_Load((int)map, (undefined4)self);
		ParseMap_Free(map);
		return 1;
	}
	bkPrintf("InitialiseCamera - Error parsing camera LOM file [%s]\n", name);
	return 0;
}

/* 0x000de2f0  Clusteroids (gap)
Advances the intro camera by `step` frames. A human pressing Fire or Boost (or the
attract mode, 0x002baeb8) skips to the end of the intro: the doors, spinner and grid jump
to their final animation. Returns nonzero when the intro is over. */
int ClustCamera_Update(void *self, int unused, float step)
{
	float position[4] = { 0.0f, 0.0f, 0.0f, 0.0f };  /* the original copies stack garbage */
	float target[4] = { 0.0f, 0.0f, 0.0f, 0.0f };    /* on a camera error */
	int done;
	int i;
	char *pl;
	int actor;

	(void)self;
	(void)unused;
	done = CLIntroCam_Update(step, 0);
	if (done == 0 || DAT_002baeb8 != 0) {
		for (i = 0; i < 4; i++) {
			pl = CL_PLAYER(i);
			if ((DAT_002baed8[i * 10] != 0 || DAT_002baeb8 != 0) &&
				(CL_CHANNEL_ON(CL_PTR(pl, PL_IN_FIRE)) != 0 ||
				 CL_CHANNEL_ON(CL_PTR(pl, PL_IN_BOOST)) != 0 || DAT_002baeb8 != 0)) {
				CLIntroCam_Stop();
				done = 1;
				CL_VAR(int, 0x002f9adc) = 1;
				bsSetSfxVolume((DAT_002bae6c * 0xff) / 0xff);
				CLIntroCam_GetPosition(position, (float)CLIntroCam_GetFrameCount());
				CLIntroCam_GetTarget(target, (float)CLIntroCam_GetFrameCount());
				for (actor = 0; actor < 3; actor++) {
					baFlushAnimations(CL_ACTOR(0x002f8920 + actor * 0x170),
						CL_ACTOR_NODE(0x002f8920 + actor * 0x170), 1, 0);
					baQueueAnimation(CL_ACTOR(0x002f8920 + actor * 0x170),
						CL_ACTOR_NODE(0x002f8920 + actor * 0x170),
						CL_VAR(struct BAnim *, 0x002f8da4 + actor * 4), 0x400, -1, 0, NULL, NULL);
				}
				CL_VAR(int, 0x002f8db0) = 1;
				CL_VAR(int, 0x002f8db4) = 1;
				goto set_camera;
			}
		}
		if (done == 0) {
			CLIntroCam_GetPosition(position, CLIntroCam_GetTime());
			CLIntroCam_GetTarget(target, CLIntroCam_GetTime());
		}
	} else if (done == -1) {
		bkPrintf("***CAMERA ERROR***\n");
	} else {
		CLIntroCam_Stop();
		CLIntroCam_GetPosition(position, (float)CLIntroCam_GetFrameCount());
		CLIntroCam_GetTarget(target, (float)CLIntroCam_GetFrameCount());
	}
set_camera:
	CL_VAR(float, 0x00295e08) = position[0];
	CL_VAR(float, 0x00295e0c) = position[1];
	CL_VAR(float, 0x00295e10) = position[2];
	CL_VAR(float, 0x002f9b24) = target[0];
	CL_VAR(float, 0x002f9b28) = target[1];
	CL_VAR(float, 0x002f9b2c) = target[2];
	bdUpdateView();
	return done;
}

/* 0x000de4f0  Clusteroids (gap)
Phase 1, the intro (state 0x002f7f7c): 0 start the attack ship, doors, spinner and
traffic animations and the camera path; 1 run the camera; 2 the HUD countdown, then
play starts (phase 2, 60 s). The screen fades in over the first second. */
void ClustGame_UpdateIntro(void)
{
	float fade;
	int i;
	char *actor;

	fade = 60.0f - ClustCountdown_GetRemaining();
	if (fade > 1.0f)
		fade = 1.0f;
	CL_VAR(int, 0x002f9ae0) = 0xff - CL_FTOL(fade * 255.0f);
	if (DAT_002baeb8 != 0)
		CL_VAR(int, 0x002f9ae0) = 0;
	if (CL_VAR(short, 0x002f7f7c) == 0) {
		baQueueAnimation(CL_ACTOR(0x002f87b0), CL_ACTOR_NODE(0x002f87b0),
			CL_VAR(struct BAnim *, 0x002f8d70), 0x400, -1, 0, NULL, NULL);
		baQueueAnimation(CL_ACTOR(0x002f8920), CL_ACTOR_NODE(0x002f8920),
			CL_VAR(struct BAnim *, 0x002f8d74), 0x400, -1, 0, (int (*)())ClustIntro_DoorsIntroDone, NULL);
		baQueueAnimation(CL_ACTOR(0x002f8a90), CL_ACTOR_NODE(0x002f8a90),
			CL_VAR(struct BAnim *, 0x002f8d78), 0x400, -1, 0, (int (*)())ClustIntro_SpinnerIntroDone, NULL);
		baQueueAnimation(CL_ACTOR(0x002f8dc0), CL_ACTOR_NODE(0x002f8dc0),
			CL_VAR(struct BAnim *, 0x002f8f30), 0x400, -1, 1, NULL, NULL);
		for (i = 0; i < 4; i++) {
			actor = CL_PLAYER(i) + PL_CHAR;
			baUpdateAnimations((struct BActorInstance *)actor, CL_AT(actor, 0x134, struct BActorNode *),
				g_ClFrameTime, NULL);
			baUpdateAnimations((struct BActorInstance *)(actor + 0x170),
				CL_AT(actor, 0x170 + 0x134, struct BActorNode *), g_ClFrameTime, NULL);
		}
		CLIntroCam_Restart();
		if (ClustCamera_Update(g_ClCamera, 1, 1.0f) == 0) {
			CL_VAR(short, 0x002f7f7c) = 1;
		} else {
			CL_VAR(short, 0x002f7f7c) = 2;
			CLHud_StartBars((float *)DAT_002ecfac, 280.0f);
		}
		ClustSound_Play(0x17, 0xff);
		ClustSound_Play(0x18, 100);
	} else if (CL_VAR(short, 0x002f7f7c) == 1) {
		if (ClustCamera_Update(g_ClCamera, 1, g_ClFrameTime) != 0) {
			CL_VAR(short, 0x002f7f7c) = 2;
			CLHud_StartBars((float *)DAT_002ecfac, 280.0f);
			return;
		}
	} else if (CL_VAR(short, 0x002f7f7c) == 2 && CLHud_UpdateCountdown(DAT_002ecfac) != 0) {
		ClustCountdown_SetDuration(60.0f);
		ClustCountdown_Start();
		g_ClPhase = 2;
		CL_VAR(int, 0x002fa27c) = 0;
	}
}

/* 0x000de710  Clusteroids (gap)
The deleting destructor of the camera object (vtable 0x00269648). */
void *ClustCamera_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00269648;
	if ((flags & 1) != 0)
		CL_DELETE(self);
	return self;
}

/* ---------- the countdown (stops while the game is paused) */

#define g_CdRunning CL_VAR(char, 0x002fa328)
#define g_CdPaused CL_VAR(char, 0x002fa329)
#define g_CdStart CL_VAR(long long, 0x002fa330)
#define g_CdTicksPerSecond CL_VAR(long long, 0x002fa338)
#define g_CdPauseStart CL_VAR(long long, 0x002fa340)
#define g_CdDuration CL_VAR(float, 0x002fa350)
#define g_CdDurationMs CL_VAR(float, 0x002fa354)
#define g_CdRemaining CL_VAR(float, 0x002fa358)
#define g_CdProgress CL_VAR(float, 0x002fa35c)

/* 0x000de730  Clusteroids (gap) */
int ClustCountdown_Init(void)
{
	CL_VAR(unsigned int, 0x002fa338) = DAT_002cd7a0;
	g_CdRunning = 0;
	g_CdPaused = 0;
	CL_VAR(unsigned int, 0x002fa330) = 0;
	CL_VAR(unsigned int, 0x002fa334) = 0;
	g_CdDuration = 0.0f;
	g_CdRemaining = 0.0f;
	CL_VAR(unsigned int, 0x002fa33c) = DAT_002cd7a4;
	return 1;
}

/* 0x000de770  Clusteroids (gap)
The remaining time and the progress (0-1) of the countdown. */
void ClustCountdown_Update(void)
{
	double elapsed;
	double ticks;
	double seconds;
	double remaining;

	if (g_CdPaused != 0) {
		if (DAT_002ecdec == 0) {
			g_CdPaused = 0;
			g_CdStart = g_CdStart + (long long)bkTimerDelta((ulonglong)g_CdPauseStart, bkTimerRead());
		}
		return;
	}
	if (DAT_002ecdec != 0) {
		g_CdPaused = 1;
		g_CdPauseStart = (long long)bkTimerRead();
	}
	if (g_CdRunning != 0) {
		elapsed = (double)(long long)bkTimerDelta((ulonglong)g_CdStart, bkTimerRead());
		ticks = (double)g_CdTicksPerSecond;
		g_CdProgress = (float)(elapsed * 1000.0 / ticks / (double)g_CdDurationMs);
		seconds = elapsed / ticks;
	} else {
		seconds = 0.0;
		g_CdProgress = 0.0f;
	}
	remaining = (double)g_CdDuration - seconds;
	g_CdRemaining = (float)remaining;
	if (remaining < 0.0)
		g_CdRemaining = 0.0f;
	if (g_CdProgress > 1.0f)
		g_CdProgress = 1.0f;
}

/* 0x000de890  Clusteroids (gap) */
void ClustCountdown_SetDuration(float seconds)
{
	g_CdDuration = seconds;
	g_CdRunning = 0;
	g_CdDurationMs = seconds * 1000.0f;
}

/* 0x000de8c0  Clusteroids (gap) */
void ClustCountdown_Start(void)
{
	g_CdRunning = 1;
	g_CdStart = (long long)bkTimerRead();
}

/* 0x000de8e0  Clusteroids (gap) */
float ClustCountdown_GetRemaining(void)
{
	return g_CdRemaining;
}

/* ---------- the frame timer */

#define g_FtPaused CL_VAR(char, 0x002fa361)
#define g_FtPauseStart CL_VAR(long long, 0x002fa378)
#define g_FtTicksPerSecond CL_VAR(long long, 0x002fae80)
#define g_FtNow CL_VAR(long long, 0x002fae88)
#define g_FtLast CL_VAR(long long, 0x002fae90)
#define g_FtMilliseconds CL_VAR(float, 0x002fae98)

/* 0x000de8f0  Clusteroids (gap) */
int ClustTimer_Init(void)
{
	unsigned long long now;

	bInitTimer();
	g_ClFrameTime = 0.0f;
	g_FtMilliseconds = 0.0f;
	CL_VAR(unsigned int, 0x002fae80) = DAT_002cd7a0;
	CL_VAR(unsigned int, 0x002fae84) = DAT_002cd7a4;
	now = bkTimerRead();
	CL_VAR(unsigned int, 0x002fae8c) = (unsigned int)(now >> 32);
	CL_VAR(unsigned int, 0x002fae88) = (unsigned int)now;
	CL_VAR(unsigned int, 0x002fae90) = (unsigned int)now;
	CL_VAR(unsigned int, 0x002fa368) = 0;
	CL_VAR(unsigned int, 0x002fa36c) = 0;
	CL_VAR(unsigned int, 0x002fae94) = CL_VAR(unsigned int, 0x002fae8c);
	return 1;
}

/* 0x000de950  Clusteroids (gap)
The frame time (g_ClFrameTime, 1.0 = 1/60 s) from the timer; while the game is paused the
timer stops. The pause menu may switch players between human and computer: those
players are marked (+0x630) when the game resumes. */
void ClustTimer_UpdateFrame(void)
{
	long long delta;
	double milliseconds;
	int i;

	if (g_FtPaused != 0) {
		if (DAT_002ecdec == 0) {
			g_FtPaused = 0;
			g_FtLast = g_FtLast + (long long)bkTimerDelta((ulonglong)g_FtPauseStart, bkTimerRead());
			for (i = 0; i < 4; i++) {
				if ((int)CL_VAR(short, 0x002f7f72 + i * 2) != DAT_002baed8[i * 10])
					CL_I16(CL_PLAYER(i), PL_CONTROL) =
						(short)((CL_VAR(short, 0x002f7f72 + i * 2) != 0) + 1);
			}
		}
		return;
	}
	if (DAT_002ecdec != 0) {
		CL_VAR(short, 0x002f7f72) = (short)DAT_002baed8[0];
		CL_VAR(short, 0x002f7f74) = (short)DAT_002baed8[10];
		CL_VAR(short, 0x002f7f76) = (short)DAT_002baed8[20];
		CL_VAR(short, 0x002f7f78) = (short)DAT_002baed8[30];
		g_FtPaused = 1;
		g_FtPauseStart = (long long)bkTimerRead();
	}
	g_FtNow = (long long)bkTimerRead();
	delta = (long long)bkTimerDelta((ulonglong)g_FtLast, (ulonglong)g_FtNow);
	g_FtLast = g_FtNow;
	milliseconds = (double)delta * 1000.0 / (double)g_FtTicksPerSecond;
	g_FtMilliseconds = (float)milliseconds;
	g_ClFrameTime = (float)(milliseconds * (double)0.06f);
	CL_VAR(float, 0x002f7f80) = bkTimerRate(delta);
}

/* ---------- sounds */

/* the sample names (0x20 bytes each from 0x00296ee8, 30 samples, up to 0x002972a8) */
#define CL_SAMPLE_NAME(i) ((char *)CL_ADDR(0x00296ee8) + (i) * 0x20)
#define CL_SAMPLE(i) CL_VAR(struct BSample *, 0x002fca98 + (i) * 4)
#define CL_SAMPLE_PLAYS(i) CL_VAR(int, 0x002fca18 + (i) * 4)

/* 0x000dec00  Clusteroids (gap)
Loads the 30 samples "sounds\<name>.wav" and widens the sound fall-off distances (the
old ones are restored by ClustSound_Free). */
int ClustSound_Load(int package)
{
	char name[512];
	struct BSample *sample;
	int i;

	(void)package;
	for (i = 0; i < 30; i++) {
		_snprintf(name, 0x1ff, "sounds\\%s.wav", CL_SAMPLE_NAME(i));
		sample = (struct BSample *)bLoadResource(NULL, name, 2, 0xdefa);
		CL_SAMPLE(i) = sample;
		if (sample == NULL)
			bkPrintf("Could not find sample #%d 'sounds\\%s.wav'\n", i, CL_SAMPLE_NAME(i));
		else
			bkPrintf("sample #%d 'sounds\\%s.wav'\n", i, CL_SAMPLE_NAME(i));
		CL_SAMPLE_PLAYS(i) = 0;
	}
	CL_VAR(float, 0x002fca90) = DAT_0028ffac;
	CL_VAR(float, 0x002fcb10) = DAT_0028ffa8;
	DAT_0028ffa8 = 4000.0f;
	DAT_0028ffac = 10000.0f;
	return 1;
}

/* 0x000decb0  Clusteroids (gap)
Plays sample `index` at `volume`; returns the channel or -1. */
int ClustSound_Play(int index, int volume)
{
	struct BSample *sample;
	int channel;

	sample = CL_SAMPLE(index);
	if (sample == NULL)
		return -1;
	channel = bsPlaySample(sample, volume, 0, CL_I32(sample, 0x20), 0);
	if (channel != -1)
		CL_SAMPLE_PLAYS(index) = CL_SAMPLE_PLAYS(index) + 1;
	return channel;
}

/* 0x000ded00  Clusteroids (gap)
Prints the play counts and restores the sound fall-off distances. */
void ClustSound_Free(void)
{
	DSCAPS caps;
	int i;

	for (i = 0; i < 0x1e; i++)
		bkPrintf("sample #%d => %d\n", i, CL_SAMPLE_PLAYS(i));
	DAT_0028ffa8 = CL_VAR(float, 0x002fcb10);
	DAT_0028ffac = CL_VAR(float, 0x002fca90);
	IDirectSound_GetCaps(DAT_002db808, &caps);
	bkPrintf("Free3DBuffers = %d\n", (int)caps.dwFree3DBuffers);
}

/* 0x000ded70  Clusteroids (gap)
Now and then (7.5%) a voice line of `kind` from player's character. */
int ClustSound_PlayVoice(int player, int kind)
{
	if (player < 0 || 3 < player)
		bkPrintf("Rumble Error\n");
	if (0.925f < (float)cl_rand() * 3.0518509e-05f)
		return CLSamples_PlayVoice((char)DAT_002baed4[player * 10], kind, NULL, 0xff, 0.0f, 0, -1, 1);
	return -1;
}
