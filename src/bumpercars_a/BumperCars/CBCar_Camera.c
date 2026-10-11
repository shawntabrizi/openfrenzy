/*
BumperCars/CBCar_Camera.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Camera.cpp   (module bumpercars_a)

The BumperCars camera (CBCar_Game + 0x2640, 0x110 bytes). The file was built without
optimisation in the original (frame pointers everywhere), so the code below follows it
closely. It loads its settings from the variation's CAMERAINFO file (a STATIC, ZOOM,
ROTATE or FIRSTPERSON block) and the intro flight from setups\introcam.lom (ClassLib
CIntroCam), then runs the game's phases (DAT_002f1f9c, the game state at game + 0x4b44):
1 the intro flight, 2 the countdown, 3 the game camera of the mode, 4 the winner view,
5 the fade out. A debug camera (engine_tail BCamController) and a camera path editor
(bumpercars_b 0x000a5ca0) can take over.

The tags 0x0008d1f9, 0x0008dc4a and 0x0008eceb are in this file; the functions from
0x0008d1e0 to 0x0009194d are this class's methods (the gap ones by their `this`).
*/

#include "bumpercars_a.h"
#include <strings.h>

static const char s_CameraFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Camera.cpp";

/* the camera (0x110 bytes) */
struct CBCar_Camera {
	int debugWanted;         /* 0x000 the debug camera is switched on */
	int debugActive;         /* 0x004 */
	int editorActive;        /* 0x008 the path editor exists */
	float eye[4];            /* 0x00c the view: eye (kind 0) or position (kind 1) */
	struct BLight *light;    /* 0x01c */
	float target[4];         /* 0x020 kind 0 */
	float angles[4];         /* 0x030 kind 1 */
	float roll;              /* 0x040 kind 0 */
	int viewKind;            /* 0x044 0 look at, 1 position and angles */
	int mode;                /* 0x048 1 static, 2 zoom, 3 first person, 4 rotate */
	float pitch;             /* 0x04c */
	float yaw;               /* 0x050 */
	int rotating;            /* 0x054 ROTATE */
	float yaw0;              /* 0x058 */
	float lastEye[4];        /* 0x05c */
	float lastTarget[4];     /* 0x06c */
	float rotateTarget[4];   /* 0x07c ROTATE TARGET */
	float rotateAngle[2];    /* 0x08c ROTATE ANGLE (radians) */
	int unknown94[2];        /* 0x094 */
	int haveStatic;          /* 0x09c a STATIC block */
	int staticKind;          /* 0x0a0 0 POSITION/TARGET, 1 ORIENTATION */
	float position[4];       /* 0x0a4 STATIC POSITION */
	float lookAt[4];         /* 0x0b4 STATIC TARGET */
	float orientation[3];    /* 0x0c4 STATIC ORIENTATION (x, y, z radians) */
	float orientationW;      /* 0x0d0 */
	float border;            /* 0x0d4 ZOOM BORDER */
	float zoomIn;            /* 0x0d8 ZOOMINSPEED */
	float zoomOut;           /* 0x0dc ZOOMOUTSPEED */
	float unknowne0;         /* 0x0e0 */
	float zoomDistance;      /* 0x0e4 */
	float minDistance;       /* 0x0e8 MINDIST */
	int player;              /* 0x0ec FIRSTPERSON PLAYER */
	float vOffset;           /* 0x0f0 VOFFSET */
	float hOffset;           /* 0x0f4 HOFFSET */
	void *debugCamera;       /* 0x0f8 a BCamController (engine_tail) */
	int winnerText;          /* 0x0fc the HUD winner text is up */
	float dt;                /* 0x100 */
	int unknown104;          /* 0x104 */
	longlong frame;          /* 0x108 frames since Initialise */
};
OF_ASSERT_OFFSET(struct CBCar_Camera, border, 0xd4);
OF_ASSERT_OFFSET(struct CBCar_Camera, frame, 0x108);
OF_ASSERT_SIZE(struct CBCar_Camera, 0x110);

/* the game's state (DAT_002f1f9c = game + 0x4b44) and other game fields by address */
#define GAME_STATE BCG(0x4b44, int)
#define GAME_TIMER (g_BCarGame->timer)
#define PLAYER_CAR(i) BCA_AT(g_BCarGame->players[i], 0x138, unsigned char *)
#define WP_COUNT BCG(0x3618, int)
#define WP_CORNER(i, c, axis) BCG(0x3630 + (i) * 0x4c + (c) * 0x10 + (axis) * 4, float)

/* core */
extern int DAT_002ba928;      /* screen width */
extern int DAT_002ba92c;      /* screen height */
extern int DAT_002ba56c;      /* the camera path editor is on */
extern int DAT_002bae6c;      /* the sound effects volume */
extern int DAT_002bae8c;      /* the game type (1: split screen views) */
extern int DAT_002bae98;      /* tournament */
extern int DAT_002baed8[];    /* per player, 0x28 bytes */
extern int DAT_002baec0[];    /* the team members, by team */
extern int DAT_002baeb8;
extern int DAT_002baebc;      /* team game */
extern int DAT_00283ef0;      /* the views are on (core data) */
extern void *DAT_002ecfac;    /* classlib_ui: the HUD */
/* BumperCars globals after the game object (bumpercars_b's block; tentative in
ClassLib/bca_foreign.c) */
extern void *DAT_002f26c4;    /* the camera path editor */
extern float DAT_002f2028;    /* frames until the next crowd cheer */
extern int DAT_002f202c;      /* the number of winners shown (-1: teams) */
extern float DAT_002f205c;    /* the last winner-view time */
extern int DAT_002f2060;      /* the player the winner view shows */
/* bca_data.c */
extern unsigned char DAT_00292080[];  /* camera path entries: variation mask, 16 bytes */
extern float DAT_0029207c;            /* the camera light's range */
extern int DAT_00292924;              /* fade the winner view */
extern int DAT_00292928;              /* the player last shown */

#define Timer_Elapsed(t) BCA_FN(FUN_00096600, float, (void *))(t)
#define Timer_Restart(t) BCA_FN(FUN_00096630, void, (void *))(t)
#define Heading(x, z) BCA_FN(FUN_000a5540, float, (float, float))((x), (z))

/* the reader of the setup file: see CBCar_Game_LoadSettings */
#define READ_LINE(r, buf) BCA_FN(FUN_001386c0, char *, (void *, char *))((r), (buf))
#define NEXT_LINE(r) BCA_FN(FUN_00138720, void, (void *))(r)

/* the swaying of the camera: the roll and the height move with the frame count (none in
variation 3) */
static void camera_sway(struct CBCar_Camera *self, float *roll, float *height)
{
	int variation = g_BCarGame->variation;

	if (variation == 4) {
		*roll = (float)(sin((double)((float)self->frame * 0x1.1df46ap-6f * 1.0f)) * (double)0.02f + (double)*roll);
		*height = (float)(sin((double)((float)self->frame * 0x1.1df46ap-6f * 1.7f)) * (double)20.0f + (double)*height);
	} else if (variation != 3) {
		*roll = (float)(sin((double)((float)self->frame * 0x1.1df46ap-6f * 1.0f)) * (double)0.05f + (double)*roll);
		*height = (float)(sin((double)((float)self->frame * 0x1.1df46ap-6f * 1.7f)) * (double)40.0f + (double)*height);
	}
}

/* 0x0008d1e0  (gap): CBCar_Camera::CBCar_Camera */
void *CBCar_Camera_Ctor(struct CBCar_Camera *self)
{
	return self;
}

/* 0x0008d1ee  (gap): CBCar_Camera::~CBCar_Camera */
void CBCar_Camera_Dtor(struct CBCar_Camera *self)
{
	(void)self;
}

/* 0x0008d1f9  BumperCars/CBCar_Camera.cpp: CBCar_Camera::Initialise: reads the camera file
and the intro flight, and places the camera at the flight's start. Returns 1. */
int CBCar_Camera_Initialise(struct CBCar_Camera *self, struct bkPackage *package, char *file)
{
	struct BCarTextReader reader;
	char name[64];
	uint size;
	void *text;
	int *map;
	float dir[4], length, eye[4], target[4], roll;

	BCA_FN(FUN_000a5be0, void, (void *))(&reader);
	text = bkLoadFile(package, file, NULL, &size, NULL, 0);
	if (text == NULL) {
		BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
		return 0;
	}
	BCA_FN(FUN_000a5c00, void, (void *, void *, uint, const char *))(&reader, text, size, file);
	self->frame = 0;
	CBCar_Camera_LoadInformation(self, package, &reader);
	self->debugActive = 0;
	self->debugWanted = 0;
	self->debugCamera = NULL;
	self->editorActive = 0;
	if (self->mode == 0) {
		BCA_Printf("CBCar_Camera::Initialise - No camera present\n");
		BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
		return 0;
	}
	if (self->haveStatic == 0) {
		BCA_Printf("CBCar_Camera::Initialise - Static camera not present\n");
		BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
		return 0;
	}
	snprintf(name, 0x3f, "setups\\introcam.lom");
	text = bkLoadFile(package, name, NULL, &size, NULL, 0);
	if (text == NULL) {
		BCA_Printf("%s(%d) : Initialise - Could not load camera LOM file [%s]\n", s_CameraFile, 0x66, name);
		BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
		return 0;
	}
	map = ParseMap_Load((char *)text, 2, (int)size);
	if (map == NULL) {
		BCA_Printf("%s(%d) : Initialise - Error parsing camera LOM file [%s]\n", s_CameraFile, 0x6c, name);
		BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
		return 0;
	}
	CLIntroCam_Load((int)(uintptr_t)map, (undefined4)(uintptr_t)self);
	ParseMap_Free(map);
	self->winnerText = 0;
	CBCar_Camera_GetDirection(self, dir);
	length = cmb_vec3_length(self->position);
	dir[0] = dir[0] * length;
	dir[1] = dir[1] * length;
	dir[2] = dir[2] * length;
	dir[0] = dir[0] + self->position[0];
	dir[1] = dir[1] + self->position[1];
	dir[2] = dir[2] + self->position[2];
	BCA_Printf("Camera at position: [%.2f, %.2f, %.2f], with target: [%.2f, %.2f, %.2f]\n",
		(double)self->position[0], (double)self->position[1], (double)self->position[2], (double)dir[0],
		(double)dir[1], (double)dir[2]);
	self->light = NULL;
	CLIntroCam_GetPosition(eye, CLIntroCam_GetTime());
	CLIntroCam_GetTarget(target, CLIntroCam_GetTime());
	CLIntroCam_GetRoll(&roll, CLIntroCam_GetTime());
	self->yaw = Heading(target[0] - eye[0], target[2] - eye[2]);
	self->pitch = Heading(target[1] - eye[1], target[2] - eye[2]);
	camera_sway(self, &roll, &eye[1]);
	CBCar_Camera_SetLookAt(self, eye, target, roll);
	bdSetCameraLookAt(eye, target, roll);
	bdUpdateView();
	BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
	return 1;
}

/* 0x0008d7ee  (gap): removes the HUD's winner text */
void CBCar_Camera_HideWinner(struct CBCar_Camera *self)
{
	if (self->winnerText != 0) {
		CLHud_DeleteWinnerText((int)(uintptr_t)DAT_002ecfac);
		self->winnerText = 0;
	}
}

/* 0x0008d81d  (gap): shows the winner text with each player's result (variation 4: points
for the place) and stores the results in core's per-player table. Returns nothing useful
(the caller keeps EAX). */
int CBCar_Camera_ShowWinner(struct CBCar_Camera *self)
{
	int scores[4];
	int i;

	for (i = 0; i < 4; i++) {
		if (g_BCarGame->variation == 4) {
			switch (BCA_I32(PLAYER_CAR(i), 0x44)) {
			case 1:
				scores[i] = 10;
				break;
			case 2:
				scores[i] = 6;
				break;
			case 3:
				scores[i] = 3;
				break;
			case 4:
				scores[i] = 1;
				break;
			default:
				scores[i] = 0;
				break;
			}
		} else {
			scores[i] = BCA_I32(PLAYER_CAR(i), 4);
		}
		DAT_002baed8[i * 10 + 4] = scores[i]; /* 0x002baee8 */
	}
	self->winnerText = 1;
	return CLHud_CreateWinnerText((int)(uintptr_t)DAT_002ecfac, scores, 0);
}

/* 0x0008d91d  (gap): the HUD's scores and the players' bars (laps, lives or points);
the leader goes to game + 0x4b4c */
void CBCar_Camera_UpdateScores(struct CBCar_Camera *self)
{
	int shown[4];
	int value[5];
	int i, total, low;
	float v;
	unsigned char *slot, *car;

	(void)self;
	for (i = 0; i < 4; i++) {
		slot = g_BCarGame->players[i];
		car = PLAYER_CAR(i);
		if (BCA_I32(car, 0x20) == 0) {
			shown[i] = 0;
		} else if (BCA_I32(slot, 0x248) == 1) {
			shown[i] = BCA_I32(car, 4);
		} else if (BCA_I32(slot, 0x248) == 2) {
			shown[i] = BCA_I32(car, 0x30);
		} else if (BCA_I32(slot, 0x248) == 3) {
			shown[i] = BCA_I32(car, 0x40);
		} else if (BCA_I32(slot, 0x248) == 4) {
			shown[i] = FUN_0008fe4b((struct CBCar_Camera *)g_BCarGame->camera, i);
		}
		car = BCA_AT(slot, 0x138, unsigned char *);
		if (BCG(0x2598, int) < 1) {           /* LAPS */
			if (BCG(0x259c, int) < 1)         /* LIVES */
				value[i + 1] = BCA_I32(car, 4);
			else
				value[i + 1] = BCA_I32(car, 0x30);
		} else {
			value[i + 1] = BCA_I32(car, 0x40) * WP_COUNT + BCA_I32(car, 0x48);
		}
	}
	value[0] = 0;
	low = 10000000;
	for (i = 0; i < 4; i++) {
		total = value[i + 1];
		if (DAT_002baebc != 0)
			total = value[i + 1] + value[DAT_002baec0[(1 - DAT_002baed8[i * 10 + 8]) + DAT_002baed8[i * 10 + 7] * 2] + 1];
		if (BCG(0x2598, int) < 1) {
			if (BCG(0x259c, int) < 1) {
				if (value[0] < total) {
					value[0] = total;
					BCG(0x4b4c, int) = i;
				}
			} else {
				value[0] = BCG(0x259c, int);
				if (i == 0)
					BCG(0x4b4c, int) = i;
				else if (value[i] < value[i + 1])
					BCG(0x4b4c, int) = i;
			}
		} else {
			value[0] = BCG(0x2598, int) * WP_COUNT;
			if (value[0] < value[i + 1])
				value[i + 1] = value[0];
			if (BCA_I32(PLAYER_CAR(i), 0x44) == 1)
				BCG(0x4b4c, int) = i;
		}
		low = 0;
	}
	if (g_BCarGame->variation != 4)
		CLHud_SetScores((int)(uintptr_t)DAT_002ecfac, shown);
	if (value[0] == low)
		value[0] = value[0] + 1;
	for (i = 0; i < 4; i++) {
		v = (float)value[i + 1];
		if (DAT_002baebc != 0)
			v = (float)value[DAT_002baec0[(1 - DAT_002baed8[i * 10 + 8]) + DAT_002baed8[i * 10 + 7] * 2] + 1] + v;
		CLHud_SetBar((int)(uintptr_t)DAT_002ecfac, i, (v - (float)low) / (float)(value[0] - low));
	}
}

/* parses one block of the camera file ("{" ... "}"); handler returns 0 (unknown command),
1 (done) or 2 (bad use) for a line */
typedef int (*BlockHandler)(struct CBCar_Camera *self, char *text);

static void parse_block(struct CBCar_Camera *self, struct BCarTextReader *reader, char *line, char **text,
	BlockHandler handler, int lineInvalid, int lineBad)
{
	char word[512];
	int status;

	while (reader->cursor < reader->end && strcasecmp(*text, "}") != 0) {
		*text = READ_LINE(reader, line);
		NEXT_LINE(reader);
		if (strncmp(*text, "//", 2) == 0)
			continue;
		status = handler(self, *text);
		if (strcasecmp(*text, "}") != 0) {
			sscanf(*text, "%s", word);
			if (status == 0)
				BCA_Printf("%s(%d) : LoadInformation(\"%s\", %d): Invalid Command \"%s\"\n", s_CameraFile,
					lineInvalid, reader->name, reader->line, word);
			else if (status == 2)
				BCA_Printf("%s(%d) : LoadInformation(\"%s\", %d): Bad use of Command \"%s\" (%s)\n",
					s_CameraFile, lineBad, reader->name, reader->line, word, *text);
		}
	}
}

static int static_command(struct CBCar_Camera *self, char *text)
{
	float x, y;
	int status = 0;

	if (strncasecmp(text, "POSITION ", 9) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f%f%f", &self->position[0], &self->position[1], &self->position[2]) == 3)
			status = 1;
	}
	if (strncasecmp(text, "TARGET ", 7) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f%f%f", &self->lookAt[0], &self->lookAt[1], &self->lookAt[2]) == 3) {
			self->staticKind = 0;
			status = 1;
		}
	}
	if (strncasecmp(text, "ORIENTATION ", 0xc) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f%f", &x, &y) == 2) {
			self->staticKind = 1;
			self->orientation[0] = x * 0x1.1df46ap-6f;
			self->orientation[1] = y * 0x1.1df46ap-6f;
			status = 1;
		}
	}
	return status;
}

static int zoom_command(struct CBCar_Camera *self, char *text)
{
	int status = 0;

	if (strncasecmp(text, "BORDER ", 7) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f", &self->border) == 1)
			status = 1;
	}
	if (strncasecmp(text, "ZOOMINSPEED ", 0xc) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f", &self->zoomIn) == 1)
			status = 1;
	}
	if (strncasecmp(text, "ZOOMOUTSPEED ", 0xd) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f", &self->zoomOut) == 1)
			status = 1;
	}
	if (strncasecmp(text, "MINDIST ", 8) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f", &self->minDistance) == 1)
			status = 1;
	}
	return status;
}

static int rotate_command(struct CBCar_Camera *self, char *text)
{
	float x, y;
	int status = 0;

	if (strcasecmp(text, "ROTATE") == 0) {
		self->rotating = 1;
		status = 1;
	}
	if (strncasecmp(text, "MINDIST ", 8) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f", &self->minDistance) == 1)
			status = 1;
	}
	if (strncasecmp(text, "TARGET ", 7) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f%f%f", &self->rotateTarget[0], &self->rotateTarget[1], &self->rotateTarget[2]) == 3)
			status = 1;
	}
	if (strncasecmp(text, "ANGLE ", 6) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f%f", &x, &y) == 2) {
			self->rotateAngle[0] = x * 0x1.1df46ap-6f;
			self->rotateAngle[1] = y * 0x1.1df46ap-6f;
			status = 1;
		}
	}
	return status;
}

static int firstperson_command(struct CBCar_Camera *self, char *text)
{
	int player;
	int status = 0;

	if (strncasecmp(text, "PLAYER ", 7) == 0) {
		status = 2;
		if (sscanf(text, "%*s%d", &player) == 1 && player > -1 && player < 4) {
			status = 1;
			self->player = player;
		}
	}
	if (strncasecmp(text, "VOFFSET ", 8) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f", &self->vOffset) == 1)
			status = 1;
	}
	if (strncasecmp(text, "HOFFSET ", 8) == 0) {
		status = 2;
		if (sscanf(text, "%*s%f", &self->hOffset) == 1)
			status = 1;
	}
	return status;
}

/* 0x0008dc4a  BumperCars/CBCar_Camera.cpp: CBCar_Camera::LoadInformation: the camera file's
blocks (the defaults first) */
void CBCar_Camera_LoadInformation(struct CBCar_Camera *self, struct bkPackage *package, struct BCarTextReader *reader)
{
	char line[512];
	char *text;

	(void)package;
	self->mode = 0;
	self->haveStatic = 0;
	self->position[0] = 0.0f;
	self->position[1] = 0.0f;
	self->position[2] = 0.0f;
	self->position[3] = 1.0f;
	self->lookAt[0] = 0.0f;
	self->lookAt[1] = 0.0f;
	self->lookAt[2] = 0.0f;
	self->lookAt[3] = 1.0f;
	self->orientation[0] = 0.0f;
	self->orientation[1] = 0.0f;
	self->orientation[2] = 0.0f;
	self->orientationW = 1.0f;
	self->border = 0.0f;
	self->zoomIn = 0.0f;
	self->zoomOut = 0.0f;
	self->unknowne0 = 0.0f;
	self->minDistance = 400.0f;
	self->player = 0;
	self->vOffset = 0.0f;
	self->hOffset = 0.0f;
	while (reader->cursor < reader->end) {
		text = READ_LINE(reader, line);
		NEXT_LINE(reader);
		if (strncmp(text, "//", 2) == 0)
			continue;
		if (strcasecmp(text, "STATIC {") == 0) {
			self->haveStatic = 1;
			parse_block(self, reader, line, &text, static_command, 0x19b, 0x19d);
		}
		if (strcasecmp(text, "ZOOM {") == 0) {
			self->mode = 2;
			parse_block(self, reader, line, &text, zoom_command, 0x1d5, 0x1d7);
		}
		if (strcasecmp(text, "ROTATE {") == 0) {
			self->mode = 4;
			self->rotating = 0;
			parse_block(self, reader, line, &text, rotate_command, 0x218, 0x21a);
		}
		if (strcasecmp(text, "FIRSTPERSON {") == 0) {
			self->mode = 3;
			parse_block(self, reader, line, &text, firstperson_command, 0x254, 0x256);
		}
	}
	if (self->mode == 0 && self->haveStatic != 0)
		self->mode = 1;
}

/* 0x0008e7f0  (gap): CBCar_Camera::Shutdown */
void CBCar_Camera_Shutdown(struct CBCar_Camera *self)
{
	void *editor;

	if (self->debugCamera != NULL) {
		if (self->debugCamera != NULL)
			BCA_VFN(self->debugCamera, 0, void *(*)(void *, int))(self->debugCamera, 1);
		self->debugCamera = NULL;
	}
	if (self->light != NULL) {
		BCA_DELETE(self->light);
		self->light = NULL;
	}
	editor = DAT_002f26c4;
	if (editor != NULL) {
		if (editor != NULL) {
			BCA_FN(FUN_000a5c80, void, (void *))(editor);
			BCA_DELETE(editor);
		}
		DAT_002f26c4 = NULL;
	}
	CLIntroCam_Destroy();
	CBCar_Camera_HideWinner(self);
}

/* 0x0008e8d1  (gap) */
float CBCar_Camera_GetYaw(struct CBCar_Camera *self)
{
	return self->yaw;
}

/* 0x0008e8e2  (gap) */
float CBCar_Camera_GetPitch(struct CBCar_Camera *self)
{
	return self->pitch;
}

/* 0x0008e8f3  (gap): the static camera's heading */
float CBCar_Camera_StaticHeading(struct CBCar_Camera *self)
{
	if (self->staticKind == 0)
		return (float)atan2((double)(self->lookAt[0] - self->position[0]), (double)(self->lookAt[2] - self->position[2]));
	if (self->staticKind == 1)
		return self->orientation[1];
	return 0.0f;
}

/* 0x0008e974  (gap): the static camera's direction (unit) */
void CBCar_Camera_GetDirection(struct CBCar_Camera *self, float *out)
{
	float forward[3];

	if (self->staticKind == 0) {
		out[0] = self->lookAt[0] - self->position[0];
		out[1] = self->lookAt[1] - self->position[1];
		out[2] = self->lookAt[2] - self->position[2];
		FUN_000514a0(out, out);
	} else if (self->staticKind == 1) {
		forward[0] = 0.0f;
		forward[1] = 0.0f;
		forward[2] = 1.0f;
		bmVectorRotateXYZ(out, forward, self->orientation[0], self->orientation[1], 0.0f);
		FUN_000514a0(out, out);
	} else {
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
		out[3] = 1.0f;
	}
}

/* the intro flight's last frame: the camera there, the countdown starts (phase 3) */
static void start_game_phase(struct CBCar_Camera *self, int phase)
{
	float eye[4], target[4], roll;

	CLIntroCam_GetPosition(eye, (float)CLIntroCam_GetFrameCount());
	CLIntroCam_GetTarget(target, (float)CLIntroCam_GetFrameCount());
	(void)roll;
	self->yaw = Heading(target[0] - eye[0], target[2] - eye[2]);
	self->pitch = Heading(target[1] - eye[1], target[2] - eye[2]);
	GAME_STATE = phase;
	Timer_Restart(GAME_TIMER);
	self->yaw0 = self->yaw;
	DAT_002f2028 = (float)bca_rand() * (1.0f / 32767.0f) * 300.0f + 150.0f;
	CLHud_StartBars((float *)DAT_002ecfac, 280.0f);
	FUN_00096680();
	bsSetSfxVolume(DAT_002bae6c);
}

/* 0x0008ea74  (gap): skips the intro flight: the camera at its end and the game starts */
void CBCar_Camera_SkipIntro(struct CBCar_Camera *self)
{
	float eye[4], target[4], roll;

	CLIntroCam_GetPosition(eye, (float)CLIntroCam_GetFrameCount());
	CLIntroCam_GetTarget(target, (float)CLIntroCam_GetFrameCount());
	CLIntroCam_GetRoll(&roll, (float)CLIntroCam_GetFrameCount());
	self->yaw = Heading(target[0] - eye[0], target[2] - eye[2]);
	self->pitch = Heading(target[1] - eye[1], target[2] - eye[2]);
	CLIntroCam_Stop();
	camera_sway(self, &roll, &eye[1]);
	CBCar_Camera_SetLookAt(self, eye, target, roll);
	bdSetCameraLookAt(eye, target, roll);
	bdUpdateView();
	GAME_STATE = 3;
	Timer_Restart(GAME_TIMER);
	self->yaw0 = self->yaw;
	DAT_002f2028 = (float)bca_rand() * (1.0f / 32767.0f) * 300.0f + 150.0f;
	CLHud_StartBars((float *)DAT_002ecfac, 280.0f);
	FUN_00096680();
	bsSetSfxVolume(DAT_002bae6c);
	FUN_00092120((struct CBCar_Sound *)g_BCarGame->sound, 0xe, 0xff, 0, -1);
	BCG(0x254c, int) = 0; /* DAT_002ef9a4: the world is not frozen */
	self->yaw0 = self->yaw;
	Timer_Restart(GAME_TIMER);
}

/* 0x0008eceb  BumperCars/CBCar_Camera.cpp: CBCar_Camera::Update (one frame) */
void CBCar_Camera_Update(struct CBCar_Camera *self, int playing, float dt)
{
	float eye[4], target[4], roll, fov, point[4];
	int done, i, fast, winners, count;
	void *editor;
	unsigned char *entry;

	(void)playing;
	self->dt = dt;
	self->frame++;
	switch (GAME_STATE) {
	case 1: /* the intro flight */
		done = CLIntroCam_Update(dt, 1);
		CLIntroCam_GetPosition(eye, CLIntroCam_GetTime());
		CLIntroCam_GetTarget(target, CLIntroCam_GetTime());
		CLIntroCam_GetRoll(&roll, CLIntroCam_GetTime());
		CLIntroCam_GetFov(&fov, CLIntroCam_GetTime());
		camera_sway(self, &roll, &eye[1]);
		if (fov != 0.0f)
			bdSetFieldOfView(fov, 0.0f);
		bdSetCameraLookAt(eye, target, roll);
		CBCar_Camera_SetLookAt(self, eye, target, roll);
		self->yaw = Heading(target[0] - eye[0], target[2] - eye[2]);
		self->pitch = Heading(target[1] - eye[1], target[2] - eye[2]);
		if (done == 0) {
			/* a player presses a button: skip the flight */
			for (i = 0; i < 4; i++) {
				unsigned char *slot = g_BCarGame->players[i];

				if (BCA_I32(slot, 0) != 0 && (BCA_I32(BCA_PTR(slot, 0x154), 0x14) != 0 ||
								     BCA_I32(BCA_PTR(slot, 0x150), 0x14) != 0)) {
					CLIntroCam_Stop();
					start_game_phase(self, 2);
					break;
				}
			}
		} else if (done == -1) {
			BCA_Printf("%s(%d) : Update - Error in Introcam\n", s_CameraFile, 0x322);
		} else {
			CLIntroCam_Stop();
			start_game_phase(self, 2);
		}
		break;
	case 2: /* the countdown */
		CLIntroCam_GetPosition(eye, (float)CLIntroCam_GetFrameCount());
		CLIntroCam_GetTarget(target, (float)CLIntroCam_GetFrameCount());
		CLIntroCam_GetRoll(&roll, (float)CLIntroCam_GetFrameCount());
		camera_sway(self, &roll, &eye[1]);
		bdSetCameraLookAt(eye, target, roll);
		CBCar_Camera_SetLookAt(self, eye, target, roll);
		self->yaw = Heading(target[0] - eye[0], target[2] - eye[2]);
		self->pitch = Heading(target[1] - eye[1], target[2] - eye[2]);
		if (CLHud_UpdateCountdown((int)(uintptr_t)DAT_002ecfac) != 0) {
			FUN_00092120((struct CBCar_Sound *)g_BCarGame->sound, 0xe, 0xff, 0, -1);
			GAME_STATE = 3;
			BCG(0x254c, int) = 0;
			self->yaw0 = self->yaw;
			Timer_Restart(GAME_TIMER);
		}
		break;
	case 3: /* the game */
		switch (self->mode) {
		case 1:
			CBCar_Camera_Static(self);
			break;
		case 2:
			CBCar_Camera_Zoom(self);
			break;
		case 3:
			CBCar_Camera_FirstPerson(self);
			break;
		case 4:
			CBCar_Camera_Rotate(self, dt);
			break;
		}
		break;
	case 4: /* the winners */
		fast = 0;
		if (Timer_Elapsed(GAME_TIMER) > 2.0f) {
			winners = 1;
			for (i = 0; i < 4; i++) {
				if (DAT_002baed8[i * 10] != 0 && BCA_I32(PLAYER_CAR(i), 0x44) == 1)
					winners = 0;
			}
			for (i = 0; i < 4; i++) {
				unsigned char *slot = g_BCarGame->players[i];

				if (BCA_I32(slot, 0) != 0 && (BCA_I32(PLAYER_CAR(i), 0x44) == 1 || winners != 0)) {
					if (BCA_I32(BCA_PTR(slot, 0x154), 0x14) != 0 && BCA_I32(slot, 0x158) == 0)
						fast = 1;
					BCA_I32(slot, 0x158) = BCA_I32(BCA_PTR(slot, 0x154), 0x14);
					BCA_I32(slot, 0x15c) = BCA_I32(BCA_PTR(slot, 0x150), 0x14);
				}
			}
		}
		if (DAT_002baebc == 0) {
			if (DAT_002f202c == 1 && DAT_002bae98 == 1) {
				if ((Timer_Elapsed(GAME_TIMER) > (float)DAT_002f202c * 4.0f && CLSamples_CommentaryIdle() != 0) ||
				    fast != 0) {
					GAME_STATE = 5;
					Timer_Restart(GAME_TIMER);
				}
			} else if (Timer_Elapsed(GAME_TIMER) > (float)DAT_002f202c * 4.0f || fast != 0) {
				GAME_STATE = 5;
				Timer_Restart(GAME_TIMER);
			}
		} else {
			count = DAT_002f202c == -1 ? 4 : 2;
			if (Timer_Elapsed(GAME_TIMER) > (float)count * 4.0f || fast != 0) {
				GAME_STATE = 5;
				Timer_Restart(GAME_TIMER);
			}
		}
		CBCar_Camera_Winners(self);
		break;
	case 5: /* the fade out */
		if (Timer_Elapsed(GAME_TIMER) > 0.5f)
			BCA_I32(g_BCarGame->mode, 0x28) = 1;
		CBCar_Camera_Apply(self);
		break;
	}
	/* the debug camera */
	if (self->debugWanted != 0 && self->debugActive == 0) {
		void *cam = BCA_NEW(0xa8, s_CameraFile, 0x3d8, 0x1001);

		if (cam != NULL)
			cam = bCamController_Construct((struct BCamController *)cam);
		self->debugCamera = cam;
		bCamController_BindInput((struct BCamController *)self->debugCamera, 0);
		bCamController_Setup((struct BCamController *)self->debugCamera, self->position[0], self->position[1],
			self->position[2], 0.0f, 0.0f, 0.0f, 50.0f, 1);
		BCA_F32(self->debugCamera, 0x54) = 1.0f;
		BCA_F32(self->debugCamera, 0x58) = 50000.0f;
		BCA_VFN(self->debugCamera, 8, void (*)(void *))(self->debugCamera);
		self->debugActive = 1;
	} else if (self->debugWanted == 0 && self->debugActive != 0) {
		if (self->debugCamera != NULL)
			BCA_VFN(self->debugCamera, 0, void *(*)(void *, int))(self->debugCamera, 1);
		self->debugCamera = NULL;
		self->debugActive = 0;
	}
	/* the camera path editor */
	if (DAT_002ba56c != 0 && self->editorActive == 0) {
		editor = BCA_NEW(0x26c, s_CameraFile, 0x3ed, 0x1001);
		if (editor != NULL)
			editor = BCA_FN(FUN_000a5ca0, void *, (void *, int, float, int))(editor, 0, 20.0f, 1);
		(void)editor; /* the result is not kept here: the constructor (bumpercars_b) is
		                 expected to set DAT_002f26c4 */
		for (i = 0; BCA_I32(DAT_00292080, i * 0x14) != 0; i++) {
			entry = DAT_00292080 + i * 0x14;
			if ((BCA_U32(entry, 0) & (1u << ((g_BCarGame->variation - 1) & 0x1f))) != 0)
				BCA_FN(FUN_000a5f60, void, (void *, void *))(DAT_002f26c4, entry + 4);
		}
		self->editorActive = 1;
	} else if (DAT_002ba56c == 0 && self->editorActive != 0) {
		editor = DAT_002f26c4;
		if (editor != NULL) {
			BCA_FN(FUN_000a5c80, void, (void *))(editor);
			BCA_DELETE(editor);
		}
		DAT_002f26c4 = NULL;
		self->editorActive = 0;
	}
	if (self->debugCamera != NULL) {
		BCA_VFN(self->debugCamera, 4, void (*)(void *, float))(self->debugCamera, 1.0f);
		baUpdateCamera();
		BCA_Printf("Camera = [%f, %f, %f] at [%f, %f]\n", (double)BCA_F32(self->debugCamera, 0x68),
			(double)BCA_F32(self->debugCamera, 0x6c), (double)BCA_F32(self->debugCamera, 0x70),
			(double)BCA_F32(self->debugCamera, 0x78), (double)BCA_F32(self->debugCamera, 0x7c));
		point[0] = BCA_F32(self->debugCamera, 0x78);
		point[1] = BCA_F32(self->debugCamera, 0x7c);
		point[2] = 0.0f;
		point[3] = 1.0f;
		CBCar_Camera_SetAngles(self, (float *)((char *)self->debugCamera + 0x68), point);
		self->yaw = BCA_F32(self->debugCamera, 0x7c);
		self->pitch = BCA_F32(self->debugCamera, 0x78);
	} else if (DAT_002f26c4 != NULL) {
		BCA_FN(FUN_000a5fa0, void, (void *, float))(DAT_002f26c4, dt);
	}
	if (self->light != NULL)
		baSetPointLight(self->light, self->eye, DAT_0029207c);
	bdUpdateView();
}

/* 0x0008fc2e  (gap): empty */
void CBCar_Camera_Nothing(void)
{
}

/* 0x0008fc39  (gap): the screen fades of the phases (the intro fades in, the winner view
fades between players, the end fades out) */
void CBCar_Camera_DrawFades(struct CBCar_Camera *self)
{
	int alpha, a, b;

	bdPushRenderState(6, 1, 0);
	bdPushRenderState(3, 0, 0);
	switch (GAME_STATE) {
	case 1:
		alpha = 0xff - BCA_FTOL(Timer_Elapsed(GAME_TIMER) / 1.0f * 255.0f);
		if (alpha < 0)
			alpha = 0;
		CLScreen_DrawDarken(alpha, 1);
		break;
	case 3:
		if (DAT_002baeb8 != 0 && Timer_Elapsed(GAME_TIMER) < 1.0f) {
			alpha = 0xff - BCA_FTOL(Timer_Elapsed(GAME_TIMER) / 1.0f * 255.0f);
			if (alpha < 0)
				alpha = 0;
			CLScreen_DrawDarken(alpha, 1);
		}
		break;
	case 4:
		if (DAT_002f202c == 1 && DAT_002bae98 == 1) {
			if (DAT_00292924 != 0 && CLFade_Draw(self->dt) != 0)
				DAT_00292924 = 0;
		} else {
			a = BCA_FTOL(Timer_Elapsed(GAME_TIMER) / 4.0f);
			b = BCA_FTOL(DAT_002f205c / 4.0f);
			if (a != b)
				DAT_00292924 = 1;
			if (DAT_00292924 != 0 && CLFade_Draw(self->dt) != 0)
				DAT_00292924 = 0;
			DAT_002f205c = Timer_Elapsed(GAME_TIMER);
		}
		break;
	case 5:
		alpha = BCA_FTOL(Timer_Elapsed(GAME_TIMER) / 0.45f * 255.0f);
		if (alpha > 0xff)
			alpha = 0xff;
		CLScreen_DrawDarken(alpha, 1);
		break;
	}
	bdPopRenderState(6);
	bdPopRenderState(3);
}

/* 0x0008fe4b  (gap): the place of player `player` (by LAPS: the laps done, then the distance
to its waypoint); -1 when not racing */
int CBCar_Camera_RacePlace(struct CBCar_Camera *self, int player)
{
	int place, best, last, alive, i, count, closest;
	float centre[4], prevDist, bestDist, d;
	unsigned char *car;

	(void)self;
	if (BCG(0x2598, int) < 1)
		return -1;
	if (BCA_I32(PLAYER_CAR(player), 0x44) != -1)
		return BCA_I32(PLAYER_CAR(player), 0x44);
	place = 1;
	last = -1;
	for (;;) {
		best = 10000000;
		alive = 0;
		for (i = 0; i < 4; i++) {
			car = PLAYER_CAR(i);
			if (BCA_I32(car, 0x20) != 0) {
				alive++;
				if (BCA_I32(car, 0x4c) < best && last < BCA_I32(car, 0x4c))
					best = BCA_I32(car, 0x4c);
			}
		}
		if (alive == 0)
			return -1;
		if (BCA_I32(PLAYER_CAR(player), 0x4c) == best)
			break;
		for (i = 0; i < 4; i++) {
			car = PLAYER_CAR(i);
			if (BCA_I32(car, 0x20) != 0 && BCA_I32(car, 0x4c) == best)
				place++;
		}
		last = best;
	}
	count = 0;
	for (i = 0; i < 4; i++) {
		car = PLAYER_CAR(i);
		if (BCA_I32(car, 0x20) != 0 && BCA_I32(car, 0x4c) == best)
			count++;
	}
	if (count == 1)
		return place;
	{
		int w = BCA_I32(PLAYER_CAR(player), 0x48);

		centre[3] = 1.0f;
		centre[0] = (WP_CORNER(w, 0, 0) + 0.0f + WP_CORNER(w, 1, 0) + WP_CORNER(w, 2, 0) + WP_CORNER(w, 3, 0)) / 4.0f;
		centre[1] = (WP_CORNER(w, 0, 1) + 0.0f + WP_CORNER(w, 1, 1) + WP_CORNER(w, 2, 1) + WP_CORNER(w, 3, 1)) / 4.0f;
		centre[2] = (WP_CORNER(w, 0, 2) + 0.0f + WP_CORNER(w, 1, 2) + WP_CORNER(w, 2, 2) + WP_CORNER(w, 3, 2)) / 4.0f;
	}
	prevDist = -1.0f;
	closest = -1;
	for (;;) {
		bestDist = 1e7f;
		for (i = 0; i < 4; i++) {
			car = PLAYER_CAR(i);
			if (BCA_I32(car, 0x20) != 0 && BCA_I32(car, 0x4c) == best) {
				d = bmVectorDistance(centre, (float *)(car + 0x50));
				if (d < bestDist && prevDist < d) {
					closest = i;
					bestDist = d;
				}
			}
		}
		if (closest == player)
			break;
		place++;
		prevDist = bestDist;
	}
	return place;
}

/* 0x00090243  (gap): a screen point (+0x70, +0x74 of `p`) lies inside the zoom border,
30 pixels in */
int CBCar_Camera_InsideBorder(struct CBCar_Camera *self, float *p)
{
	float height = (float)DAT_002ba92c - 120.0f;
	float x = (float)DAT_002ba928 / 2.0f + BCA_F32(p, 0x70);
	float y = ((float)DAT_002ba92c / 2.0f + BCA_F32(p, 0x74)) - 120.0f;

	if ((double)x <= (double)((float)DAT_002ba928 * self->border) + 30.0 ||
	    (double)(((float)DAT_002ba928 - (float)DAT_002ba928 * self->border)) - 30.0 <= (double)x ||
	    (double)y <= (double)(height * self->border) + 30.0 || (double)(height - height * self->border) - 30.0 <= (double)y)
		return 0;
	return 1;
}

/* 0x0009032e  (gap): a screen point lies outside the zoom border, 30 pixels out */
int CBCar_Camera_OutsideBorder(struct CBCar_Camera *self, float *p)
{
	float height = (float)DAT_002ba92c - 120.0f;
	float x = (float)DAT_002ba928 / 2.0f + BCA_F32(p, 0x70);
	float y = ((float)DAT_002ba92c / 2.0f + BCA_F32(p, 0x74)) - 120.0f;

	if ((double)x < (double)((float)DAT_002ba928 * self->border) - 30.0 ||
	    (double)(((float)DAT_002ba928 - (float)DAT_002ba928 * self->border)) + 30.0 < (double)x ||
	    (double)y < (double)(height * self->border) - 30.0 || (double)(height - height * self->border) + 30.0 < (double)y)
		return 1;
	return 0;
}

/* 0x00090419  (gap): mode 1, the static camera */
void CBCar_Camera_Static(struct CBCar_Camera *self)
{
	if (self->debugCamera != NULL || DAT_002f26c4 != NULL)
		return;
	if (self->staticKind == 0) {
		bdSetCameraLookAt(self->position, self->lookAt, 0.0f);
		CBCar_Camera_SetLookAt(self, self->position, self->lookAt, 0.0f);
		self->yaw = (float)atan2((double)(self->lookAt[0] - self->position[0]), (double)(self->lookAt[2] - self->position[2]));
		self->pitch = (float)atan2((double)(self->lookAt[1] - self->position[1]), (double)(self->lookAt[2] - self->position[2]));
	} else if (self->staticKind == 1) {
		bdSetCameraPosition(self->position[0], self->position[1], self->position[2]);
		bdSetCameraAngles(self->orientation[0], self->orientation[1], self->orientation[2], 5);
		CBCar_Camera_SetAngles(self, self->position, self->orientation);
		self->yaw = self->orientation[1];
		self->pitch = self->orientation[0];
	}
}

/* 0x000905bb  (gap): mode 2, the zoom camera: from the static position towards the
players' centre (bumpercars_b 0x0009c170), closer as they gather */
void CBCar_Camera_Zoom(struct CBCar_Camera *self)
{
	float centre[4], dir[4], toCentre[4], v[4], eye[4], target[4], roll, t;

	if (self->debugCamera != NULL || DAT_002f26c4 != NULL)
		return;
	if (BCA_FN(FUN_0009c170, int, (float *))(centre) == 0) {
		CBCar_Camera_Static(self);
		return;
	}
	bdSetProjectionMode(1);
	self->zoomDistance = 1000.0f;
	t = (float)(((double)1.0f / ((double)bmVectorDistance(centre, self->position) - (double)self->minDistance)) *
		(double)self->zoomDistance);
	CBCar_Camera_GetDirection(self, dir);
	toCentre[0] = centre[0] - self->position[0];
	toCentre[1] = centre[1] - self->position[1];
	toCentre[2] = centre[2] - self->position[2];
	FUN_000514a0(toCentre, toCentre);
	bmVectorLerp(v, dir, toCentre, t);
	FUN_000514a0(v, v);
	v[0] = v[0] * self->zoomDistance;
	v[1] = v[1] * self->zoomDistance;
	v[2] = v[2] * self->zoomDistance;
	eye[0] = v[0] + self->position[0];
	eye[1] = v[1] + self->position[1];
	eye[2] = v[2] + self->position[2];
	eye[3] = 0.0f; /* uninitialised in the original */
	bmVectorLerp(v, dir, toCentre, t);
	target[0] = v[0] * 1000.0f + eye[0];
	target[1] = v[1] * 1000.0f + eye[1];
	target[2] = v[2] * 1000.0f + eye[2];
	target[3] = 0.0f; /* uninitialised in the original */
	if (g_BCarGame->variation == 4) {
		roll = (float)(sin((double)((float)self->frame * 0x1.1df46ap-6f * 1.0f)) * (double)0.02f);
		eye[1] = (float)(sin((double)((float)self->frame * 0x1.1df46ap-6f * 1.7f)) * (double)20.0f + (double)eye[1]);
	} else if (g_BCarGame->variation == 3) {
		roll = 0.0f;
	} else {
		roll = (float)(sin((double)((float)self->frame * 0x1.1df46ap-6f * 1.0f)) * (double)0.05f);
		eye[1] = (float)(sin((double)((float)self->frame * 0x1.1df46ap-6f * 1.7f)) * (double)40.0f + (double)eye[1]);
	}
	bdSetCameraLookAt(eye, target, roll);
	CBCar_Camera_SetLookAt(self, eye, target, roll);
	self->yaw = (float)atan2((double)(target[0] - eye[0]), (double)(target[2] - eye[2]));
	self->pitch = (float)atan2((double)(target[1] - eye[1]), (double)(target[2] - eye[2]));
	memcpy(self->lastEye, eye, 16);
	memcpy(self->lastTarget, target, 16);
}

/* 0x000909ab  (gap): mode 3, the first person camera behind player `player` (1-4) */
void CBCar_Camera_FirstPerson(struct CBCar_Camera *self)
{
	float offset[4], v[4], eye[4];
	unsigned char *car;
	int i;

	if (self->debugCamera != NULL || DAT_002f26c4 != NULL)
		return;
	i = self->player - 1;
	offset[0] = 0.0f;
	offset[1] = self->vOffset;
	offset[2] = self->hOffset;
	offset[3] = 1.0f;
	bmVectorRotateY(v, offset, BCA_F32(PLAYER_CAR(i), 0x80) + 0x1.921fb4p+1f);
	car = PLAYER_CAR(i);
	eye[0] = v[0] + BCA_F32(car, 0x50);
	eye[1] = v[1] + BCA_F32(car, 0x54);
	eye[2] = v[2] + BCA_F32(car, 0x58);
	eye[3] = 0.0f; /* uninitialised in the original */
	bdSetCameraPosition(eye[0], eye[1], eye[2]);
	bdSetCameraAngles(0.0f, BCA_F32(PLAYER_CAR(i), 0x80), 0.0f, 5);
	v[1] = BCA_F32(PLAYER_CAR(i), 0x80);
	v[0] = 0.0f;
	v[2] = 0.0f;
	v[3] = 1.0f;
	CBCar_Camera_SetAngles(self, eye, v);
	self->yaw = CBCar_Camera_StaticHeading(self);
	self->pitch = 0.0f;
	offset[0] = 0.0f;
	offset[1] = 0.0f;
	offset[2] = 10.0f;
	offset[3] = 1.0f;
	bmVectorRotateY(v, offset, BCA_F32(PLAYER_CAR(i), 0x80));
	self->lastTarget[0] = eye[0] + v[0];
	self->lastTarget[1] = eye[1] + v[1];
	self->lastTarget[2] = eye[2] + v[2];
	memcpy(self->lastEye, eye, 16);
}

/* 0x00090b89  (gap): phase 4, the winner view: each winner in turn (4 seconds each),
playing its win animation, seen from 650 units behind and 250 above */
void CBCar_Camera_Winners(struct CBCar_Camera *self)
{
	unsigned char *slot, *car;
	int shown, count, p;
	float target[4], eye[4], c, s;

	if (self->debugCamera != NULL || DAT_002f26c4 != NULL)
		return;
	if (GAME_STATE == 4) {
		if (DAT_002baebc == 0) {
			if (DAT_002f202c == 1 && DAT_002bae98 == 1) {
				shown = 0;
			} else {
				shown = BCA_FTOL(Timer_Elapsed(GAME_TIMER) / 4.0f);
				if (DAT_002f202c - 1 < shown)
					shown = DAT_002f202c - 1;
			}
		} else {
			count = DAT_002f202c == -1 ? 4 : 2;
			shown = BCA_FTOL(Timer_Elapsed(GAME_TIMER) / 4.0f);
			if (count - 1 < shown)
				shown = count - 1;
		}
		if (DAT_002baebc == 0) {
			for (DAT_002f2060 = 0; DAT_002f2060 < 4; DAT_002f2060++) {
				if (BCA_I32(PLAYER_CAR(DAT_002f2060), 0x44) == 1)
					shown--;
				if (shown == -1)
					break;
			}
		} else if (DAT_002f202c == -1) {
			DAT_002f2060 = DAT_002baec0[shown];
		} else {
			DAT_002f2060 = DAT_002baec0[shown + DAT_002f202c * 2];
		}
	}
	p = DAT_002f2060;
	slot = g_BCarGame->players[p];
	if (BCA_I32(slot, 0x29c) != 0 && BCA_I32(slot, 0x2b0) != 1) {
		void *actor = BCA_PTR(slot, 0x290);

		baFlushAnimations(actor, *(struct BActorNode **)((char *)actor + 0x134), 1, 0);
		baQueueAnimation((struct BActorInstance *)actor, *(struct BActorNode **)((char *)actor + 0x134),
			*(struct BAnim **)(slot + 0x29c), 0x400, -1, 1, NULL, NULL);
		BCA_I32(slot, 0x2b0) = 1;
	}
	if (DAT_00292928 != -1 && DAT_00292928 != DAT_002f2060 && BCA_I32(slot, 0x2a8) != 0 && BCA_I32(slot, 0x2b0) != 4) {
		void *actor = BCA_PTR(slot, 0x290);

		baFlushAnimations(actor, *(struct BActorNode **)((char *)actor + 0x134), 1, 0);
		baQueueAnimation((struct BActorInstance *)actor, *(struct BActorNode **)((char *)actor + 0x134),
			*(struct BAnim **)(slot + 0x2a8), 0x400, -1, 1, NULL, NULL);
		BCA_I32(slot, 0x2b0) = 4;
	}
	DAT_00292928 = DAT_002f2060;
	car = PLAYER_CAR(p);
	memcpy(target, car + 0x50, 16);
	target[1] = target[1] + BCA_F32(car, 0xf4);
	FUN_000927a0((struct CBCar_Light *)g_BCarGame->light, target);
	self->yaw = -BCA_F32(PLAYER_CAR(p), 0x80) - 0x1.921fb4p+0f;
	c = (float)cos((double)self->yaw);
	eye[0] = target[0] - c * 650.0f;
	eye[1] = target[1] + 250.0f;
	s = (float)sin((double)self->yaw);
	eye[2] = target[2] - s * 650.0f;
	eye[3] = target[3];
	self->yaw = (float)atan2((double)(target[0] - eye[0]), (double)(target[2] - eye[2]));
	self->pitch = (float)atan2((double)(target[1] - eye[1]), (double)(target[2] - eye[2]));
	bdSetCameraLookAt(eye, target, 0.0f);
	CBCar_Camera_SetLookAt(self, eye, target, 0.0f);
}

/* 0x00090fba  (gap): mode 4, the rotating camera: around ROTATE TARGET at MINDIST, turning
once over the game (GAMETIMER) when ROTATE is set */
void CBCar_Camera_Rotate(struct CBCar_Camera *self, float dt)
{
	float target[4], offset[4], v[4], eye[4], roll, yaw;

	(void)dt;
	if (self->debugCamera != NULL || DAT_002f26c4 != NULL)
		return;
	memcpy(target, self->rotateTarget, 16);
	if (self->rotating == 0)
		yaw = self->rotateAngle[1];
	else
		yaw = (float)(((double)Timer_Elapsed(GAME_TIMER) / (double)g_BCarGame->gameTime) * (double)0x1.921fb6p+2f +
			(double)self->rotateAngle[1]);
	offset[0] = 0.0f;
	offset[1] = 0.0f;
	offset[2] = self->minDistance;
	offset[3] = 1.0f;
	bmVectorRotateXYZ(v, offset, -self->rotateAngle[0], -yaw, 0.0f);
	eye[0] = target[0] + v[0];
	eye[1] = target[1] + v[1];
	eye[2] = target[2] + v[2];
	eye[3] = 0.0f; /* uninitialised in the original */
	target[2] = 0.0f;
	target[0] = 0.0f;
	self->yaw = (float)atan2((double)(0.0f - eye[0]), (double)(0.0f - eye[2]));
	self->pitch = -self->orientation[0];
	CLIntroCam_GetRoll(&roll, (float)CLIntroCam_GetFrameCount());
	camera_sway(self, &roll, &eye[1]);
	bdSetCameraLookAt(eye, target, roll);
	CBCar_Camera_SetLookAt(self, eye, target, roll);
	memcpy(self->lastEye, eye, 16);
	memcpy(self->lastTarget, target, 16);
}

/* 0x00091287  (gap): sets the display's camera from the stored view */
void CBCar_Camera_Apply(struct CBCar_Camera *self)
{
	if (self->viewKind == 0) {
		bdSetCameraLookAt(self->eye, self->target, self->roll);
	} else if (self->viewKind == 1) {
		bdSetCameraPosition(self->eye[0], self->eye[1], self->eye[2]);
		bdSetCameraAngles(self->angles[0], self->angles[1], self->angles[2], 5);
	}
	bdUpdateView();
}

/* 0x0009130b  (gap): stores a look-at view */
void CBCar_Camera_SetLookAt(struct CBCar_Camera *self, float *eye, float *target, float roll)
{
	self->viewKind = 0;
	memcpy(self->eye, eye, 16);
	memcpy(self->target, target, 16);
	self->roll = roll;
}

/* 0x0009139b  (gap): stores a position-and-angles view */
void CBCar_Camera_SetAngles(struct CBCar_Camera *self, float *position, float *angles)
{
	self->viewKind = 1;
	memcpy(self->eye, position, 16);
	memcpy(self->angles, angles, 16);
}

/* the view from a car: offset (0, up, back) in the car's frame (its node matrix at
+0x124 -> +0xe0), looking 100 units ahead, rolled with it */
static void car_view(int player, float back, float up, float *eye, float *target, float *roll)
{
	float matrix[16], v[4];
	unsigned char *car = PLAYER_CAR(player);

	FUN_000515c0(matrix, (float *)(BCA_AT(car, 0x124, unsigned char *) + 0xe0));
	v[0] = 0.0f;
	v[1] = up;
	v[2] = back;
	v[3] = 1.0f;
	bmTransformVector4InPlace(matrix, v);
	eye[0] = v[0] + BCA_F32(PLAYER_CAR(player), 0x50);
	eye[1] = v[1] + BCA_F32(PLAYER_CAR(player), 0x54);
	eye[2] = v[2] + BCA_F32(PLAYER_CAR(player), 0x58);
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = -1.0f;
	v[3] = 1.0f;
	bmTransformVector4InPlace(matrix, v);
	target[0] = eye[0] + v[0] * 100.0f;
	target[1] = eye[1] + v[1] * 100.0f;
	target[2] = eye[2] + v[2] * 100.0f;
	v[0] = 0.0f;
	v[1] = 1.0f;
	v[2] = 0.0f;
	v[3] = 1.0f;
	bmTransformVector4InPlace(matrix, v);
	v[0] = v[0] * 100.0f;
	v[1] = v[1] * 100.0f;
	v[2] = v[2] * 100.0f;
	bmVectorRotateY(v, v, -BCA_F32(PLAYER_CAR(player), 0x80));
	*roll = -Heading(v[0], v[1]);
}

/* 0x00091422  (gap): the view from player `view`'s car (the split screen views) */
void CBCar_Camera_CarView(struct CBCar_Camera *self, int view, float back, float up)
{
	float eye[4], target[4], roll;

	if (self->debugCamera != NULL || DAT_002f26c4 != NULL || view < 0 || view >= 4)
		return;
	car_view(view, back, up, eye, target, &roll);
	bdSetCameraLookAt(eye, target, roll);
	bdUpdateView();
}

/* 0x000915fd  (gap): a look-at view along dir from pos */
void CBCar_Camera_LookAlong(struct CBCar_Camera *self, float *pos, float *dir, float roll)
{
	float target[3];

	(void)self;
	target[0] = pos[0] + dir[0];
	target[1] = pos[1] + dir[1];
	target[2] = pos[2] + dir[2];
	bdSetCameraLookAt(pos, target, roll);
	bdUpdateView();
}

/* 0x00091650  (gap): the stored view mirrored in y (the reflection pass) */
void CBCar_Camera_ApplyMirrored(struct CBCar_Camera *self)
{
	float eye[4], target[4];

	if (self->viewKind == 0) {
		eye[2] = self->eye[2];
		eye[1] = -self->eye[1];
		eye[0] = self->eye[0];
		eye[3] = 1.0f;
		target[2] = self->target[2];
		target[1] = -self->target[1];
		target[0] = self->target[0];
		target[3] = 1.0f;
		bdSetCameraLookAt(eye, target, -self->roll);
	} else if (self->viewKind == 1) {
		bdSetCameraPosition(self->eye[0], -self->eye[1], self->eye[2]);
		bdSetCameraAngles(-self->angles[0], -self->angles[1], -self->angles[2], 5);
	}
	bdUpdateView();
}

/* 0x0009175a  (gap): CBCar_Camera_CarView mirrored in y */
void CBCar_Camera_CarViewMirrored(struct CBCar_Camera *self, int view, float back, float up)
{
	float eye[4], target[4], roll;

	if (self->debugCamera != NULL || DAT_002f26c4 != NULL || view < 0 || view >= 4)
		return;
	car_view(view, back, up, eye, target, &roll);
	eye[1] = -eye[1];
	target[1] = -target[1];
	roll = -roll;
	bdSetCameraLookAt(eye, target, roll);
	bdUpdateView();
}

/* 0x0009194d  (gap): the split screen views are on (game type 1, phases 2-3): the first
player in game goes to *view. Returns 1 when there is one. */
int CBCar_Camera_ViewsOn(struct CBCar_Camera *self, int *view)
{
	(void)self;
	if (DAT_00283ef0 == 0 || DAT_002bae8c != 1)
		return 0;
	if (GAME_STATE != 3 && GAME_STATE != 2)
		return 0;
	if (view == NULL)
		return 1;
	for (*view = 0; *view < 4; *view = *view + 1) {
		if (DAT_002baed8[*view * 10] != 0)
			return 1;
	}
	return 0;
}
